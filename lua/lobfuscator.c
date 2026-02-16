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

static void virtualize_proto_internal(lua_State *L, Proto *f) {
    if (f->sizecode == 0) return;

    int old_sizecode = f->sizecode;

    lu_byte *is_target = luaM_newvector(L, old_sizecode, lu_byte);
    memset(is_target, 0, old_sizecode);

    // 1. Identify jump targets
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
        if (op == OP_LOADKX || op == OP_SETLIST) {
            if (op == OP_LOADKX || (op == OP_SETLIST && GETARG_C(inst) == 0)) {
                if (i + 1 < old_sizecode) is_target[i + 1] = 1;
            }
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

            // Strictly safe instructions that never yield or skip instructions
            int is_safe = (op == OP_MOVE || op == OP_LOADK || op == OP_LOADNIL ||
                           op == OP_GETUPVAL);

            if (!is_safe) break;
            if (i > start && is_target[i]) break;

            i++;
            count++;
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
    luaM_freearray(L, is_target, old_sizecode);
}

void obfuscate_proto(lua_State *L, Proto *f, int encrypt_k) {
    if (f->obfuscated) return;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    // Increase stack size to provide slots for dynamic decryption (RK values)
    f->scratch_base = f->maxstacksize;
    f->maxstacksize += 2;
    if (f->maxstacksize > 255) f->maxstacksize = 255;
    if (f->scratch_base > 253) f->scratch_base = 253;

    if (encrypt_k) {
        for (int i = 0; i < f->sizek; i++) {
            TValue *o = &f->k[i];
            if (ttisinteger(o)) {
                o->value_.i = ENCRYPT_INT(o->value_.i);
            } else if (ttisfloat(o)) {
                // Skip float encryption to avoid precision/NaN issues
                // o->value_.n = ENCRYPT_FLT_VAL(o->value_.n);
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
    for (int i = 0; i < f->sizevcode; i++) {
        // VCode for OP_VIRTUAL stores raw instructions after the first element (count)
        // Wait, virtualize_proto_internal stores raw instructions in temp_vcode.
        // Let's check the structure of vcode.
        // It starts with 'count', then 'count' instructions.
    }
    // Re-check virtualize_proto_internal logic for vcode.
    int vptr = 0;
    while (vptr < f->sizevcode) {
        int count = (int)f->vcode[vptr++];
        for (int j = 0; j < count; j++) {
            OpCode op = GET_OPCODE(f->vcode[vptr]);
            SET_OPCODE(f->vcode[vptr], inv_map[op]);
            vptr++;
        }
    }

    /* 3. Apply Dynamic Encryption with Per-Function Seed */
    f->inst_seed = (unsigned int)rand();
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


    for (int i = 0; i < f->sizep; i++) {
        obfuscate_proto(L, f->p[i], encrypt_k);
    }
}
