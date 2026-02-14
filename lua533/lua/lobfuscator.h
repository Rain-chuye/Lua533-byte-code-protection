#ifndef lobfuscator_h
#define lobfuscator_h

#include "llimits.h"
#include "lua.h"
#include "lobject.h"

/*
** Commercial Obfuscation Keys
*/
#define LUA_INST_KEY  0x3F2A1B4CU
#define LUA_OP_XOR    0x27U
#define LUA_CONST_XOR 0x5E

#define ENCRYPT_INST(i) ((((i) + 0x5D4C3B2A) ^ LUA_INST_KEY) + 0x1A2B3C4D)
#define DECRYPT_INST(i) ((((i) - 0x1A2B3C4D) ^ LUA_INST_KEY) - 0x5D4C3B2A)

/* Obfuscator functions */
void obfuscate_proto(lua_State *L, Proto *f, int encrypt_strings);

#endif
