#include "lobfuscator.h"
#include "lopcodes.h"
#include "lfunc.h"
#include "lmem.h"
#include "lstate.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int start;
    int end; /* inclusive */
    int new_start;
} Block;

static void virtualize_proto_internal(lua_State *L, Proto *f) {
    int i, j;
    int size = f->sizecode;
    if (size <= 0 || f->obfuscated) return;

    for (i = 0; i < f->sizep; i++) {
        virtualize_proto_internal(L, f->p[i]);
    }

    /* 1. Identify basic blocks */
    char *is_target = (char *)calloc(size, 1);
    is_target[0] = 1;
    for (i = 0; i < size; i++) {
        Instruction inst = f->code[i];
        OpCode op = GET_OPCODE(inst);
        if (getOpMode(op) == iAsBx) {
            int target = i + 1 + GETARG_sBx(inst);
            if (target >= 0 && target < size) is_target[target] = 1;
            if (i + 1 < size) is_target[i + 1] = 1;
        } else if (op == OP_RETURN || op == OP_TAILCALL) {
            if (i + 1 < size) is_target[i + 1] = 1;
        } else if (testTMode(op)) {
            if (i + 2 < size) is_target[i + 2] = 1;
        }
    }

    int block_count = 0;
    for (i = 0; i < size; i++) if (is_target[i]) block_count++;

    Block *blocks = (Block *)malloc(block_count * sizeof(Block));
    int curr_block = 0;
    for (i = 0; i < size; i++) {
        if (is_target[i]) {
            blocks[curr_block].start = i;
            if (curr_block > 0) blocks[curr_block - 1].end = i - 1;
            curr_block++;
        }
    }
    blocks[block_count - 1].end = size - 1;

    /* 2. Shuffle blocks */
    int *shuf = (int *)malloc(block_count * sizeof(int));
    for (i = 0; i < block_count; i++) shuf[i] = i;
    unsigned int seed = (unsigned int)time(NULL) + (unsigned int)(size_t)f;
    for (i = block_count - 1; i > 0; i--) {
        seed = seed * 1103515245 + 12345;
        int k = (seed / 65536) % (i + 1);
        int temp = shuf[i];
        shuf[i] = shuf[k];
        shuf[k] = temp;
    }

    /* 3. Reconstruct code with shuffled blocks and fix jumps */
    int new_code_size = 0;
    int *has_extra_jmp = (int *)calloc(block_count, sizeof(int));
    for (i = 0; i < block_count; i++) {
        int idx = shuf[i];
        blocks[idx].new_start = new_code_size;
        new_code_size += (blocks[idx].end - blocks[idx].start + 1);
        OpCode last_op = GET_OPCODE(f->code[blocks[idx].end]);
        if (last_op != OP_JMP && last_op != OP_RETURN && last_op != OP_TAILCALL &&
            last_op != OP_FORLOOP && last_op != OP_TFORLOOP && blocks[idx].end < size - 1) {
            new_code_size++;
            has_extra_jmp[idx] = 1;
        }
    }

    Instruction *rearranged_code = luaM_newvector(L, new_code_size, Instruction);
    int curr_pos = 0;
    for (i = 0; i < block_count; i++) {
        int idx = shuf[i];
        int b_len = blocks[idx].end - blocks[idx].start + 1;
        memcpy(&rearranged_code[curr_pos], &f->code[blocks[idx].start], b_len * sizeof(Instruction));
        curr_pos += b_len;
        if (has_extra_jmp[idx]) {
            rearranged_code[curr_pos++] = CREATE_AsBx(OP_JMP, 0, 0);
        }
    }

    /* Fix JMPs in rearranged_code */
    curr_pos = 0;
    for (i = 0; i < block_count; i++) {
        int idx = shuf[i];
        int b_len = blocks[idx].end - blocks[idx].start + 1;
        for (j = 0; j < b_len; j++) {
            Instruction *pinst = &rearranged_code[curr_pos + j];
            if (getOpMode(GET_OPCODE(*pinst)) == iAsBx) {
                int old_target = (blocks[idx].start + j) + 1 + GETARG_sBx(*pinst);
                int target_block = -1;
                for (int k = 0; k < block_count; k++) {
                    if (old_target >= blocks[k].start && old_target <= blocks[k].end) {
                        target_block = k; break;
                    }
                }
                if (target_block != -1) {
                    int new_target = blocks[target_block].new_start + (old_target - blocks[target_block].start);
                    SETARG_sBx(*pinst, new_target - (curr_pos + j + 1));
                }
            }
        }
        curr_pos += b_len;
        if (has_extra_jmp[idx]) {
            Instruction *pjmp = &rearranged_code[curr_pos - 1];
            int old_next = blocks[idx].end + 1;
            int target_block = -1;
            for (int k = 0; k < block_count; k++) {
                if (old_next >= blocks[k].start && old_next <= blocks[k].end) {
                    target_block = k; break;
                }
            }
            if (target_block != -1) {
                SETARG_sBx(*pjmp, blocks[target_block].new_start - curr_pos);
            }
        }
    }

    /* 4. Full Instruction Virtualization with shuffled storage */
    Instruction *final_code = luaM_newvector(L, new_code_size * 2, Instruction);
    int *inst_shuf = (int *)malloc(new_code_size * sizeof(int));
    int *old_to_final_pos = (int *)malloc(new_code_size * sizeof(int));
    for (i = 0; i < new_code_size; i++) inst_shuf[i] = i;
    for (i = new_code_size - 1; i > 0; i--) {
        seed = seed * 1103515245 + 12345;
        int k = (seed / 65536) % (i + 1);
        int temp = inst_shuf[i];
        inst_shuf[i] = inst_shuf[k];
        inst_shuf[k] = temp;
    }

    for (i = 0; i < new_code_size; i++) {
        int original_idx = inst_shuf[i];
        old_to_final_pos[original_idx] = new_code_size + i;
        final_code[new_code_size + i] = rearranged_code[original_idx];
    }

    for (i = 0; i < new_code_size; i++) {
        Instruction inst = rearranged_code[i];
        OpCode op = GET_OPCODE(inst);

        if (op == OP_EXTRAARG) {
            final_code[i] = inst;
        } else if (i > 0 && testTMode(GET_OPCODE(rearranged_code[i-1]))) {
            final_code[i] = inst;
        } else {
            unsigned int target = (unsigned int)old_to_final_pos[i];
            unsigned int encoded = ((target + 0x1A2B3C) ^ 0x4D5E6F) + 0x7A8B9C;
            final_code[i] = CREATE_Ax(OP_VIRTUAL, encoded & 0x3FFFFFF);
        }
    }

    luaM_freearray(L, f->code, f->sizecode);
    f->code = final_code;
    f->sizecode = new_code_size * 2;
    f->obfuscated = 1;

    free(is_target);
    free(blocks);
    free(shuf);
    free(has_extra_jmp);
    free(inst_shuf);
    free(old_to_final_pos);
    luaM_freearray(L, rearranged_code, new_code_size);
}

void obfuscate_proto(lua_State *L, Proto *f, int encrypt_k) {
    virtualize_proto_internal(L, f);
}
