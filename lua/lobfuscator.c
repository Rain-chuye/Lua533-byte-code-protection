#include "lobfuscator.h"
#include "lopcodes.h"
#include "lfunc.h"
#include "ldebug.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static int seeded = 0;

void lua_security_check(void) {
    /* Anti-debugging removed to prevent crashes */
}

void lua_start_security_thread(void) {
    /* Security thread disabled */
}

static void fuse_instructions_internal(Proto *f) {
    if (f->sizecode < 2) return;

    /* Pattern: FAST_DIST (x*x + y*y)^0.5 */
    if (f->sizecode >= 4) {
        for (int i = 0; i < f->sizecode - 3; i++) {
            Instruction i1 = f->code[i];
            Instruction i2 = f->code[i+1];
            Instruction i3 = f->code[i+2];
            Instruction i4 = f->code[i+3];
            if (GET_OPCODE(i1) == OP_MUL && GETARG_B(i1) == GETARG_C(i1) &&
                GET_OPCODE(i2) == OP_MUL && GETARG_B(i2) == GETARG_C(i2) &&
                GET_OPCODE(i3) == OP_ADD && GETARG_B(i3) == GETARG_A(i1) && GETARG_C(i3) == GETARG_A(i2) &&
                GET_OPCODE(i4) == OP_POW && GETARG_B(i4) == GETARG_A(i3)) {
                int ra = GETARG_A(i4);
                int rb = GETARG_B(i1);
                int rc = GETARG_B(i2);
                f->code[i] = CREATE_ABC(OP_FAST_DIST, ra, rb, rc);
                f->code[i+1] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                f->code[i+2] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                f->code[i+3] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                i += 3;
            }
        }
    }

    for (int i = 0; i < f->sizecode - 1; i++) {
        Instruction inst1 = f->code[i];
        Instruction inst2 = f->code[i+1];
        OpCode op1 = GET_OPCODE(inst1);
        OpCode op2 = GET_OPCODE(inst2);

        if (op1 == OP_FUSE_NOP) continue;

        /* Pattern 1: Ra = Rb.Rc; Ra = Ra - Rd -> Ra = FUSE_GETSUB(Rb, Rc, Rd) */
        if (op1 == OP_GETTABLE && op2 == OP_SUB &&
            GETARG_A(inst1) == GETARG_A(inst2) && GETARG_A(inst1) == GETARG_B(inst2)) {
            int ra = GETARG_A(inst1);
            int rb = GETARG_B(inst1);
            int rc = GETARG_C(inst1);
            int rd = GETARG_C(inst2);
            f->code[i] = CREATE_ABC(OP_FUSE_GETSUB, ra, rb, rc);
            f->code[i+1] = CREATE_Ax(OP_EXTRAARG, rd);
            i++; continue;
        }
        else if (op1 == OP_GETTABLE && op2 == OP_ADD &&
            GETARG_A(inst1) == GETARG_A(inst2) && GETARG_A(inst1) == GETARG_B(inst2)) {
            int ra = GETARG_A(inst1);
            int rb = GETARG_B(inst1);
            int rc = GETARG_C(inst1);
            int rd = GETARG_C(inst2);
            f->code[i] = CREATE_ABC(OP_FUSE_GETADD, ra, rb, rc);
            f->code[i+1] = CREATE_Ax(OP_EXTRAARG, rd);
            i++; continue;
        }
        /* Pattern 2: Ra = Rb.Rc; Rd = Re.Rf; Ra = Ra - Rd -> Ra = FUSE_GETGETSUB(Rb, Rc, Re, Rf) */
        if (i < f->sizecode - 2) {
            Instruction inst3 = f->code[i+2];
            if (op1 == OP_GETTABLE && GET_OPCODE(inst2) == OP_GETTABLE && GET_OPCODE(inst3) == OP_SUB &&
                GETARG_A(inst1) == GETARG_B(inst3) && GETARG_A(inst2) == GETARG_C(inst3)) {
                int ra = GETARG_A(inst3);
                int rb = GETARG_B(inst1);
                int rc = GETARG_C(inst1);
                int re = GETARG_B(inst2);
                int rf = GETARG_C(inst2);
                f->code[i] = CREATE_ABC(OP_FUSE_GETGETSUB, ra, rb, rc);
                f->code[i+1] = CREATE_Ax(OP_EXTRAARG, (re << 9) | rf);
                f->code[i+2] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                i += 2; continue;
            }
        }
    }

    /* Pattern 5: t.k += val (Field Accumulation) */
    if (f->sizecode >= 3) {
        for (int i = 0; i < f->sizecode - 2; i++) {
            Instruction inst1 = f->code[i];
            Instruction inst2 = f->code[i+1];
            Instruction inst3 = f->code[i+2];
            if (GET_OPCODE(inst1) == OP_GETTABLE && GET_OPCODE(inst2) == OP_ADD && GET_OPCODE(inst3) == OP_SETTABLE) {
                int ra1 = GETARG_A(inst1);
                int t1 = GETARG_B(inst1);
                int k1 = GETARG_C(inst1);

                int ra2 = GETARG_A(inst2);
                int rb2 = GETARG_B(inst2);
                int rc2 = GETARG_C(inst2);

                int t3 = GETARG_A(inst3);
                int k3 = GETARG_B(inst3);
                int rv3 = GETARG_C(inst3);

                /* Case: R[ra1] = t1.k1; R[ra2] = R[ra1] + rc2; t1.k1 = R[ra2] */
                if (ra1 == rb2 && ra2 == rv3 && t1 == t3 && k1 == k3) {
                    f->code[i] = CREATE_ABC(OP_FUSE_ADD_TO_FIELD, t1, k1, rc2);
                    f->code[i+1] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    f->code[i+2] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    i += 2;
                }
                /* Case: R[ra1] = t1.k1; R[ra2] = rb2 + R[ra1]; t1.k1 = R[ra2] */
                else if (ra1 == rc2 && ra2 == rv3 && t1 == t3 && k1 == k3) {
                    f->code[i] = CREATE_ABC(OP_FUSE_ADD_TO_FIELD, t1, k1, rb2);
                    f->code[i+1] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    f->code[i+2] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    i += 2;
                }
            }
        }
    }

    /* Pattern 4: PARTICLE_DIST (Corrected match with EXTRAARG) */
    if (f->sizecode >= 7) {
        for (int i = 0; i < f->sizecode - 6; i++) {
            Instruction i1 = f->code[i];
            Instruction i4 = f->code[i+3];
            Instruction i7 = f->code[i+6];
            if (GET_OPCODE(i1) == OP_FUSE_GETGETSUB && GET_OPCODE(i4) == OP_FUSE_GETGETSUB && GET_OPCODE(i7) == OP_FAST_DIST) {
                if (GETARG_A(i1) == GETARG_B(i7) && GETARG_A(i4) == GETARG_C(i7)) {
                    int ra = GETARG_A(i7);
                    int p1 = GETARG_B(i1);
                    int p2 = GETARG_Ax(f->code[i+1]); // From EXTRAARG of first FUSE_GETGETSUB
                    int kx = GETARG_C(i1);
                    int ky = GETARG_C(i4);
                    f->code[i] = CREATE_ABC(OP_FUSE_PARTICLE_DIST, ra, (int)(p1 & 0x1FF), (int)(p2 & 0x1FF));
                    f->code[i+1] = CREATE_Ax(OP_EXTRAARG, kx);
                    f->code[i+2] = CREATE_Ax(OP_EXTRAARG, ky);
                    f->code[i+3] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    f->code[i+4] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    f->code[i+5] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    f->code[i+6] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
                    i += 6;
                }
            }
        }
    }
}

static void inject_junk_internal(Proto *f) {
    for (int i = 0; i < f->sizecode; i++) {
        if (GET_OPCODE(f->code[i]) == OP_FUSE_NOP) {
            int r = rand() % 5;
            switch (r) {
                case 0: f->code[i] = CREATE_ABC(OP_MOVE, f->scratch_base, f->scratch_base, 0); break;
                case 1: f->code[i] = CREATE_ABC(OP_TEST, f->scratch_base, 0, 0); break;
                case 2: f->code[i] = CREATE_ABx(OP_JMP, 0, MAXARG_sBx); break;
                case 3: f->code[i] = CREATE_ABC(OP_ADD, f->scratch_base, f->scratch_base, f->scratch_base); break;
                default: break; // keep NOP
            }
        }
    }
}

static void virtualize_proto_internal(lua_State *L, Proto *f) {
    if (f->sizecode == 0) return;
    fuse_instructions_internal(f);
    inject_junk_internal(f);
    int old_sizecode = f->sizecode;
    if (old_sizecode <= 0) return;
    lu_byte *is_target = (lu_byte *)malloc(old_sizecode);
    memset(is_target, 0, old_sizecode);
    for (int i = 0; i < old_sizecode; i++) {
        Instruction inst = f->code[i];
        OpCode op = GET_OPCODE(inst);
        if (op == OP_JMP || op == OP_FORLOOP || op == OP_FORPREP || op == OP_TFORLOOP) {
            int target = i + 1 + GETARG_sBx(inst);
            if (target >= 0 && target < old_sizecode) is_target[target] = 1;
        }
        if (op == OP_EQ || op == OP_LT || op == OP_LE || op == OP_TEST || op == OP_TESTSET || op == OP_TFORCALL) {
            if (i + 1 < old_sizecode) is_target[i + 1] = 1;
        }
    }
    Instruction *temp_vcode = luaM_newvector(L, old_sizecode * 2, Instruction);
    int vcode_ptr = 0;
    for (int i = 0; i < old_sizecode; ) {
        int start = i;
        int count = 0;
        while (i < old_sizecode) {
            Instruction inst = f->code[i];
            OpCode op = GET_OPCODE(inst);
            int is_safe = (op == OP_MOVE || op == OP_LOADK || op == OP_LOADNIL || op == OP_GETUPVAL);
            if (!is_safe) break;
            if (i > start && is_target[i]) break;
            i++; count++;
            if (count >= 255) break;
        }
        if (count > 1 && vcode_ptr < (1 << 26)) {
            int vindex = vcode_ptr;
            temp_vcode[vcode_ptr++] = (Instruction)count;
            for (int j = 0; j < count; j++) {
                temp_vcode[vcode_ptr++] = f->code[start + j];
            }
            f->code[start] = CREATE_Ax(OP_VIRTUAL, vindex);
            for (int j = 1; j < count; j++) {
                f->code[start + j] = CREATE_Ax(OP_EXTRAARG, 0);
            }
        } else {
            i = start + 1;
        }
    }
    if (vcode_ptr > 0) {
        f->vcode = luaM_newvector(L, vcode_ptr, Instruction);
        memcpy(f->vcode, temp_vcode, vcode_ptr * sizeof(Instruction));
        f->sizevcode = vcode_ptr;
    }
    luaM_freearray(L, temp_vcode, old_sizecode * 2);
    free(is_target);
}

void obfuscate_proto(lua_State *L, Proto *f, int encrypt_k) {
    if (f->obfuscated) return;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    // Increase stack size for instruction fusion and dynamic decryption
    f->scratch_base = f->maxstacksize;
    f->maxstacksize += 4;
    if (f->maxstacksize > 255) f->maxstacksize = 255;
    if (f->scratch_base > 251) f->scratch_base = 251;

    if (encrypt_k) {
        f->string_seed = (unsigned int)rand() | 0x1;
        for (int i = 0; i < f->sizek; i++) {
            TValue *o = &f->k[i];
            if (ttisinteger(o)) {
                // Already handled in llex.c if from literal, but might be from folding
                o->value_.i = ENCRYPT_INT(ivalue(o));
            } else if (ttisstring(o)) {
                TString *ts = tsvalue(o);
                size_t len = tsslen(ts);
                const char *src = getstr(ts);
                char *buf = malloc(len + 1);
                for (size_t j = 0; j < len; j++) {
                    buf[j] = src[j] ^ (char)((f->string_seed >> ((j % 4) * 8)) & 0xFF);
                }
                buf[len] = '\0';
                TString *nts = luaS_newlstr(L, buf, len);
                free(buf);
                setsvalue(L, o, nts);
            }
        }
    }

    virtualize_proto_internal(L, f);

    /* 1. Generate Random Opcode Map */
    f->op_map = luaM_newvector(L, NUM_OPCODES, lu_byte);
    lu_byte inv_map[NUM_OPCODES];
    for (int i = 0; i < NUM_OPCODES; i++) f->op_map[i] = (lu_byte)i;
    // Shuffle
    for (int i = NUM_OPCODES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        lu_byte t = f->op_map[i];
        f->op_map[i] = f->op_map[j];
        f->op_map[j] = t;
    }
    // Create inverse map for remapping
    for (int i = 0; i < NUM_OPCODES; i++) {
        inv_map[f->op_map[i]] = (lu_byte)i;
    }

    /* 2. Remap Opcodes in Bytecode */
    for (int i = 0; i < f->sizecode; i++) {
        OpCode op = GET_OPCODE(f->code[i]);
        SET_OPCODE(f->code[i], inv_map[op]);
    }
    int vptr = 0;
    while (vptr < f->sizevcode) {
        int count = (int)f->vcode[vptr++];
        for (int j = 0; j < count; j++) {
            OpCode op = GET_OPCODE(f->vcode[vptr]);
            SET_OPCODE(f->vcode[vptr], inv_map[op]);
            vptr++;
        }
    }

    /* 3. Transform vcode to Custom ISA with randomized bit-layout per function */
    f->inst_seed = (unsigned int)rand();
    int order = f->inst_seed % 6;

    vptr = 0;
    while (vptr < f->sizevcode) {
        int count = (int)f->vcode[vptr++];
        for (int j = 0; j < count; j++) {
            Instruction inst = f->vcode[vptr];
            OpCode op = (OpCode)((inst >> POS_OP) & 0x3F);
            int a = (inst >> POS_A) & 0xFF;
            int b = (inst >> POS_B) & 0x1FF;
            int c = (inst >> POS_C) & 0x1FF;
            /* Randomized Field Order */
            Instruction vinst = op;
            switch(order) {
                case 0: vinst |= (a << 6) | (b << 14) | (c << 23); break; // ABC
                case 1: vinst |= (a << 6) | (c << 14) | (b << 23); break; // ACB
                case 2: vinst |= (b << 6) | (a << 15) | (c << 23); break; // BAC
                case 3: vinst |= (b << 6) | (c << 15) | (a << 24); break; // BCA
                case 4: vinst |= (c << 6) | (a << 15) | (b << 23); break; // CAB
                case 5: vinst |= (c << 6) | (b << 15) | (a << 24); break; // CBA
            }
            f->vcode[vptr++] = ~vinst;
        }
    }

    /* 4. Apply Dynamic Encryption with Per-Function Seed */
    /* f->inst_seed already set in step 3 */
    /* Apply encryption after all optimizations to maintain patterns as long as possible */
    for (int i = 0; i < f->sizecode; i++) {
        f->code[i] = ENCRYPT_INST(f->code[i], i, f->inst_seed);
    }
    vptr = 0;
    while (vptr < f->sizevcode) {
        int vcount_idx = vptr;
        int count = (int)f->vcode[vptr++];
        // Encrypt the count as well to match VM decryption
        f->vcode[vcount_idx] = ENCRYPT_INST(f->vcode[vcount_idx], vcount_idx, f->inst_seed);
        for (int j = 0; j < count; j++) {
            f->vcode[vptr] = ENCRYPT_INST(f->vcode[vptr], vptr, f->inst_seed);
            vptr++;
        }
    }

    f->obfuscated = 1;

    f->linedefined = 0;
    f->lastlinedefined = 0;
    f->source = NULL;

    // Strip debug info explicitly
    if (f->lineinfo) { luaM_freearray(L, f->lineinfo, f->sizelineinfo); f->lineinfo = NULL; f->sizelineinfo = 0; }
    if (f->locvars) {
        f->sizelocvars = 0;
    }
    if (f->upvalues) {
        for (int i = 0; i < f->sizeupvalues; i++) f->upvalues[i].name = NULL;
    }

    for (int i = 0; i < f->sizep; i++) {
        obfuscate_proto(L, f->p[i], encrypt_k);
    }
}

static void inject_junk_code(lua_State *L, Proto *f) {
    if (f->sizecode < 5) return;
    int junk_count = f->sizecode / 10 + 1;
    if (junk_count > 10) junk_count = 10;

    int new_size = f->sizecode + junk_count * 3;
    Instruction *new_code = luaM_newvector(L, new_size, Instruction);
    int *new_lineinfo = luaM_newvector(L, new_size, int);

    int ni = 0;
    for (int i = 0; i < f->sizecode; i++) {
        new_code[ni] = f->code[i];
        new_lineinfo[ni] = (f->lineinfo ? f->lineinfo[i] : 0);
        ni++;

        if (junk_count > 0 && (rand() % 15 == 0)) {
            /* Insert: JMP +2; junk; junk; */
            new_code[ni] = CREATE_AsBx(OP_JMP, 0, 2);
            new_lineinfo[ni] = 0;
            ni++;
            new_code[ni] = CREATE_ABC(OP_MOVE, f->scratch_base, f->scratch_base, 0);
            new_lineinfo[ni] = 0;
            ni++;
            new_code[ni] = CREATE_ABC(OP_FUSE_NOP, 0, 0, 0);
            new_lineinfo[ni] = 0;
            ni++;
            junk_count--;

            /* Update jumps following this point */
            for (int j = 0; j < f->sizecode; j++) {
                Instruction inst = f->code[j];
                OpCode op = GET_OPCODE(inst);
                if (op == OP_JMP || op == OP_FORLOOP || op == OP_FORPREP || op == OP_TFORLOOP) {
                    int target = j + 1 + GETARG_sBx(inst);
                    if (j < i && target > i) {
                        SETARG_sBx(f->code[j], GETARG_sBx(inst) + 3);
                    } else if (j >= i && target <= i) {
                        // backward jump from after to before
                        // handled by re-mapping later?
                        // Actually it's easier to just shift everything and then fix all jumps.
                    }
                }
            }
        }
    }
    // This simple approach is risky with jumps.
    // A better way is to insert NOPs and then replace them.
    luaM_freearray(L, new_code, new_size);
    luaM_freearray(L, new_lineinfo, new_size);
}
