#include "lobfuscator.h"
#include "lopcodes.h"
#include "lobject.h"
#include "lmem.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void lua_obfuscate_proto(lua_State *L, Proto *f) {
  int i;
  int old_size = f->sizecode;
  static int seeded = 0;

  for (i = 0; i < f->sizep; i++) {
    lua_obfuscate_proto(L, f->p[i]);
  }

  if (f->obfuscated || old_size <= 0) return;

  if (!seeded) {
    srand((unsigned int)time(NULL));
    seeded = 1;
  }

  f->obfuscated = 1;
  f->op_xor = (lu_byte)(rand() % 64);
  f->const_xor = (lu_byte)(rand() % 256);

  // Group instructions
  int *group_start = luaM_newvector(L, old_size, int);
  int *group_size = luaM_newvector(L, old_size, int);
  int num_groups = 0;

  for (i = 0; i < old_size; ) {
    group_start[num_groups] = i;
    int size = 1;
    OpCode op = GET_OPCODE(f->code[i]);
    if (i + 1 < old_size) {
      OpCode next_op = GET_OPCODE(f->code[i+1]);
      if (next_op == OP_EXTRAARG || (op == OP_TFORCALL && next_op == OP_TFORLOOP)) {
        size = 2;
      }
    }
    group_size[num_groups] = size;
    num_groups++;
    i += size;
  }

  // Shuffle groups
  int *map_group = luaM_newvector(L, num_groups, int);
  for (i = 0; i < num_groups; i++) map_group[i] = i;
  for (i = num_groups - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = map_group[i]; map_group[i] = map_group[j]; map_group[j] = temp;
  }

  // New PC mapping
  int *old_pc_to_new_pc = luaM_newvector(L, old_size + 1, int);
  int curr_pc = 0;
  for (i = 0; i < num_groups; i++) {
    int g_idx = map_group[i];
    int old_start = group_start[g_idx];
    int g_size = group_size[g_idx];
    old_pc_to_new_pc[old_start] = curr_pc;
    if (g_size == 2) old_pc_to_new_pc[old_start + 1] = curr_pc + 1;
    curr_pc += g_size + 1; // +1 for the JMP
  }
  old_pc_to_new_pc[old_size] = curr_pc;

  int expanded_size = curr_pc;
  Instruction *shuffled_proxies = luaM_newvector(L, expanded_size, Instruction);
  Instruction *hidden_pool = luaM_newvector(L, expanded_size, Instruction);

  int *map_hidden = luaM_newvector(L, expanded_size, int);
  for (i = 0; i < expanded_size; i++) map_hidden[i] = i;
  for (i = expanded_size - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = map_hidden[i]; map_hidden[i] = map_hidden[j]; map_hidden[j] = temp;
  }

  for (i = 0; i < num_groups; i++) {
    int g_idx = map_group[i];
    int old_start = group_start[g_idx];
    int g_size = group_size[g_idx];
    int new_start = old_pc_to_new_pc[old_start];

    for (int j = 0; j < g_size; j++) {
      Instruction inst = f->code[old_start + j];
      OpCode op = GET_OPCODE(inst);

      if (op == OP_JMP || op == OP_FORLOOP || op == OP_FORPREP || op == OP_TFORLOOP) {
        int target = old_start + j + 1 + GETARG_sBx(inst);
        int new_target = old_pc_to_new_pc[target];
        SETARG_sBx(inst, new_target - (new_start + j) - 1);
      }

      int h_idx = map_hidden[new_start + j];
      hidden_pool[h_idx] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, inst));

      Instruction vinst = CREATE_ABC(OP_VIRTUAL, 0, 0, 0);
      unsigned int ax = ((( (unsigned int)(expanded_size + h_idx) + MBA_K4) ^ MBA_K3) + MBA_K2) ^ MBA_K1;
      SETARG_Ax(vinst, ax & 0x3FFFFFF);
      shuffled_proxies[new_start + j] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, vinst));
    }

    // Add JMP to next group
    Instruction jmp = CREATE_ABC(OP_JMP, 0, 0, 0);
    int next_old_pc = old_start + g_size;
    int new_target = old_pc_to_new_pc[next_old_pc];
    SETARG_sBx(jmp, new_target - (new_start + g_size) - 1);

    int h_idx_jmp = map_hidden[new_start + g_size];
    hidden_pool[h_idx_jmp] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, jmp));

    Instruction vinst_jmp = CREATE_ABC(OP_VIRTUAL, 0, 0, 0);
    unsigned int ax_jmp = ((( (unsigned int)(expanded_size + h_idx_jmp) + MBA_K4) ^ MBA_K3) + MBA_K2) ^ MBA_K1;
    SETARG_Ax(vinst_jmp, ax_jmp & 0x3FFFFFF);
    shuffled_proxies[new_start + g_size] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, vinst_jmp));
  }

  Instruction *final_code = luaM_newvector(L, expanded_size * 2, Instruction);
  memcpy(final_code, shuffled_proxies, expanded_size * sizeof(Instruction));
  memcpy(final_code + expanded_size, hidden_pool, expanded_size * sizeof(Instruction));

  luaM_freearray(L, f->code, f->sizecode);
  f->code = final_code;
  f->sizecode = expanded_size * 2;

  if (f->lineinfo) {
    luaM_freearray(L, f->lineinfo, f->sizelineinfo);
    f->lineinfo = NULL;
    f->sizelineinfo = 0;
  }
  f->sizelocvars = 0;

  luaM_freearray(L, group_start, old_size);
  luaM_freearray(L, group_size, old_size);
  luaM_freearray(L, map_group, num_groups);
  luaM_freearray(L, old_pc_to_new_pc, old_size + 1);
  luaM_freearray(L, shuffled_proxies, expanded_size);
  luaM_freearray(L, hidden_pool, expanded_size);
  luaM_freearray(L, map_hidden, expanded_size);
}
