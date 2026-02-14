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
  int *map;
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

  Instruction *new_code = luaM_newvector(L, old_size * 2, Instruction);

  map = luaM_newvector(L, old_size, int);
  for (i = 0; i < old_size; i++) map[i] = i;

  // High-strength instruction-level shuffling
  for (i = old_size - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = map[i]; map[i] = map[j]; map[j] = temp;
  }

  for (i = 0; i < old_size; i++) {
    Instruction inst = f->code[i];
    int target_idx = old_size + map[i];

    // Store real instruction (randomized and encrypted) in the hidden pool
    new_code[target_idx] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, inst));

    // Create Proxy in the main segment
    Instruction vinst = CREATE_ABC(OP_VIRTUAL, 0, 0, 0);

    // Complex MBA Transformation for the index
    unsigned int idx = (unsigned int)target_idx;
    unsigned int ax = (((idx + MBA_K4) ^ MBA_K3) + MBA_K2) ^ MBA_K1;
    ax &= 0x3FFFFFF;

    SETARG_Ax(vinst, ax);
    new_code[i] = ENCRYPT_INST(LUA_OP_ENCODE(f->op_xor, vinst));
  }

  luaM_freearray(L, f->code, f->sizecode);
  f->code = new_code;
  f->sizecode = old_size * 2;

  // Stripping debug information for extra protection
  if (f->lineinfo) {
    luaM_freearray(L, f->lineinfo, f->sizelineinfo);
    f->lineinfo = NULL;
    f->sizelineinfo = 0;
  }
  f->sizelocvars = 0; // Effectively strip locals

  luaM_freearray(L, map, old_size);
}
