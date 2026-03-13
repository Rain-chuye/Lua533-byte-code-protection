#ifndef lobfuscator_h
#define lobfuscator_h

#include "llimits.h"
#include "lua.h"
#include "lobject.h"

#define LUA_CONST_XOR 0xAB
#define LUA_INST_XOR  0x55AA55AA

#define ENCRYPT_INST(i) ((i) ^ LUA_INST_XOR)
#define DECRYPT_INST(i) ((i) ^ LUA_INST_XOR)

// MBA Keys
#define MBA_K1 0x23F81D0
#define MBA_K2 0x0B27E14
#define MBA_K3 0x15A39C2
#define MBA_K4 0x06B28F1

/* Obfuscator functions */
void lua_obfuscate_proto(lua_State *L, Proto *f);

#endif
