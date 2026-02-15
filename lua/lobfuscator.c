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

#ifdef __linux__
#include <sys/ptrace.h>
#include <unistd.h>
#endif

#define GET_OPCODE_PLAIN(i)	(cast(OpCode, (luaP_op_decode[cast(lu_byte, ((i)>>POS_OP) & MASK1(SIZE_OP,0))]) ^ LUA_OP_XOR))

void lua_security_check(void) {
#ifdef __linux__
    // 1. ptrace check
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
        exit(0);
    }
    // 2. TracerPid check
    FILE* fp = fopen("/proc/self/status", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "TracerPid:", 10) == 0) {
                int pid = atoi(&line[10]);
                if (pid != 0) exit(0);
                break;
            }
        }
        fclose(fp);
    }
#endif
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
                           op == OP_GETUPVAL || op == OP_SETUPVAL);

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

    // Increase stack size to provide slots for dynamic decryption (RK values)
    if (f->maxstacksize <= 253) f->maxstacksize += 2;
    else f->maxstacksize = 255;

    if (encrypt_k) {
        for (int i = 0; i < f->sizek; i++) {
            TValue *o = &f->k[i];
            if (ttisinteger(o)) {
                o->value_.i = ENCRYPT_INT(o->value_.i);
            } else if (ttisfloat(o)) {
                o->value_.n = ENCRYPT_FLT_VAL(o->value_.n);
            }
        }
    }

    virtualize_proto_internal(L, f);
    f->obfuscated = 1;

    // Aggressive Metadata Destruction
    if (f->lineinfo) {
        luaM_freearray(L, f->lineinfo, f->sizelineinfo);
        f->lineinfo = NULL;
        f->sizelineinfo = 0;
    }
    if (f->locvars) {
        // We don't necessarily want to free the array if it's still needed by something,
        // but stripping it is standard for production.
        for (int i = 0; i < f->sizelocvars; i++) f->locvars[i].varname = NULL;
        f->sizelocvars = 0;
    }
    for (int i = 0; i < f->sizeupvalues; i++) {
        f->upvalues[i].name = NULL;
    }
    f->source = NULL;

    for (int i = 0; i < f->sizep; i++) {
        obfuscate_proto(L, f->p[i], encrypt_k);
    }
}
