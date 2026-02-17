#ifndef lobfuscator_h
#define lobfuscator_h

#include "llimits.h"
#include "lua.h"
#include "lobject.h"

/*
** Linear MBA Obfuscation Keys
*/
#define LUA_INST_MUL      3U
#define LUA_INST_MUL_INV  0xaaaaaaabU
#define LUA_INST_ADD      0x12345678U
#define LUA_INST_XOR      0xAB8271C3U

#define LUA_CONST_XOR     0x8F

#define ENCRYPT_INST(i)   ((((unsigned int)(i)) * LUA_INST_MUL + LUA_INST_ADD) ^ LUA_INST_XOR)
#define DECRYPT_INST(i)   ((((unsigned int)(i) ^ LUA_INST_XOR) - LUA_INST_ADD) * LUA_INST_MUL_INV)

/* Numerical Obfuscation */
#define LUA_INT_XOR       0xDEADBEEFCAFEBABEULL
#define LUA_INT_ADD       0x123456789ABCDEF0ULL
#define LUA_INT_MUL       3ULL
#define LUA_INT_MUL_INV   0xaaaaaaaaaaaaaaabULL

#define ENCRYPT_INT(i) (((((unsigned long long)(i)) * LUA_INT_MUL) + LUA_INT_ADD) ^ LUA_INT_XOR)
#define DECRYPT_INT(i) (((((unsigned long long)(i)) ^ LUA_INT_XOR) - LUA_INT_ADD) * LUA_INT_MUL_INV)

#define LUA_FLT_XOR       0xFEEDFACEDEADBEEFULL
#define LUA_FLT_ADD       12345.6789
#define LUA_FLT_MUL       3.14159265

#define DECRYPT_FLT_VAL(n) decrypt_float_obf(n)
#define ENCRYPT_FLT_VAL(n) encrypt_float_obf(n)

static lua_Number decrypt_float_obf(lua_Number n) {
  union { lua_Number f; unsigned long long i; } u;
  u.f = n;
  u.i ^= LUA_FLT_XOR;
  return (u.f - LUA_FLT_ADD) / LUA_FLT_MUL;
}

static lua_Number encrypt_float_obf(lua_Number n) {
  union { lua_Number f; unsigned long long i; } u;
  u.f = n * LUA_FLT_MUL + LUA_FLT_ADD;
  u.i ^= LUA_FLT_XOR;
  return u.f;
}

/* Obfuscator functions */
void obfuscate_proto(lua_State *L, Proto *f, int encrypt_strings);

#endif
