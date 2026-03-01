#ifndef lobfuscator_h
#define lobfuscator_h

#include "llimits.h"
#include "lua.h"
#include "lobject.h"

/*
** Commercial Obfuscation Keys
*/
#define LUA_CUSTOM_SIGNATURE "\x1bLUAX"

#include <stdint.h>

#define ENCRYPT_INST(i, idx, f) encrypt_instruction(i, idx, generate_proto_key(f))
#define DECRYPT_INST(i, idx, f) decrypt_instruction(i, idx, generate_proto_key(f))

#define GET_PROTO_FEATURES(f) ((uint32_t)((f)->sizecode ^ (f)->sizek ^ ((f)->numparams << 16)))

static inline uint32_t generate_proto_key(const Proto *f) {
  uint32_t ts = PROTO_TS(f);
  uint32_t salt = PROTO_SALT(f);
  uint32_t seed = f->inst_seed;
  uint32_t features = GET_PROTO_FEATURES(f);
  uint32_t k = seed ^ ts ^ salt ^ features;
  k = (k ^ (k >> 16)) * 0x85ebca6b;
  k = (k ^ (k >> 13)) * 0xc2b2ae35;
  k ^= (k >> 16);
  return k;
}

static inline uint32_t encrypt_instruction(uint32_t i, int idx, uint32_t key) {
  uint32_t k = key ^ ((uint32_t)idx * 0x9E3779B9U);
  i ^= k;
  i += 0x12345678;
  i = (i << 13) | (i >> 19);
  i ^= k;
  i -= 0x12345678;
  i = (i << 7) | (i >> 25);
  i ^= 0x55555555;
  return i;
}

static inline uint32_t decrypt_instruction(uint32_t i, int idx, uint32_t key) {
  uint32_t k = key ^ ((uint32_t)idx * 0x9E3779B9U);
  i ^= 0x55555555;
  i = (i >> 7) | (i << 25);
  i += 0x12345678;
  i ^= k;
  i = (i >> 13) | (i << 19);
  i -= 0x12345678;
  i ^= k;
  return i;
}

/* Numerical Obfuscation (Dynamic) */
static inline uint64_t get_int_key(const Proto *f) {
  uint64_t k = ((uint64_t)PROTO_TS(f) << 32) | PROTO_SALT(f);
  k ^= 0xDEADBEEFCAFEBABEULL;
  return k;
}

static inline uint64_t encrypt_int_hidden(lua_Integer i, const Proto *f) {
  uint64_t k = get_int_key(f);
  uint64_t v = (uint64_t)i;
  v = (v * 3) + 0x123456789ABCDEF0ULL;
  return v ^ k;
}

static inline lua_Integer decrypt_int_hidden(uint64_t i, const Proto *f) {
  uint64_t k = get_int_key(f);
  uint64_t v = i ^ k;
  v = (v - 0x123456789ABCDEF0ULL) * 0xaaaaaaaaaaaaaaabULL;
  return (lua_Integer)v;
}

#define ENCRYPT_INT(i, f) encrypt_int_hidden(i, f)
#define DECRYPT_INT(i, f) decrypt_int_hidden(i, f)

static inline lua_Number encrypt_float_hidden(lua_Number n, const Proto *f) {
  union { lua_Number f; uint64_t i; } u;
  uint64_t k = get_int_key(f) ^ 0xFEEDFACEDEADBEEFULL;
  u.f = n * 3.14159265 + 12345.6789;
  u.i ^= k;
  return u.f;
}

static inline lua_Number decrypt_float_hidden(lua_Number n, const Proto *f) {
  union { lua_Number f; uint64_t i; } u;
  uint64_t k = get_int_key(f) ^ 0xFEEDFACEDEADBEEFULL;
  u.f = n;
  u.i ^= k;
  return (u.f - 12345.6789) / 3.14159265;
}

#define ENCRYPT_FLT_VAL(n, f) encrypt_float_hidden(n, f)
#define DECRYPT_FLT_VAL(n, f) decrypt_float_hidden(n, f)

/* Obfuscator functions */
void obfuscate_proto(lua_State *L, Proto *f, int encrypt_strings);
void lua_security_check(void);
void lua_start_security_thread(void);

#endif
