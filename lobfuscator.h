#ifndef lobfuscator_h
#define lobfuscator_h
#include "llimits.h"
#include "lua.h"
#include "lobject.h"
#define LUA_CONST_XOR 0x8F
#define LUA_CUSTOM_SIGNATURE "\x1bLUAX"
#include <stdint.h>
#define ENCRYPT_INST(i, idx, seed) encrypt_instruction(i, idx, seed)
static inline uint32_t encrypt_instruction(uint32_t i, int idx, uint32_t seed) {
  uint32_t k = seed ^ ((uint32_t)idx * 0x9E3779B9U);
  i ^= k; i = (i << 13) | (i >> 19); i += k;
  return i;
}
#define DECRYPT_INST(i, idx, seed) decrypt_instruction(i, idx, seed)
static inline uint32_t decrypt_instruction(uint32_t i, int idx, uint32_t seed) {
  uint32_t k = seed ^ ((uint32_t)idx * 0x9E3779B9U);
  i -= k; i = (i >> 13) | (i << 19); i ^= k;
  return i;
}
static inline uint32_t murmurhash3_x86_32(const void *key, int len, uint32_t seed) {
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 4; uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51; const uint32_t c2 = 0x1b873593;
    const uint32_t *blocks = (const uint32_t *)(data);
    for (int i = 0; i < nblocks; i++) {
        uint32_t k1 = blocks[i]; k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2;
        h1 ^= k1; h1 = (h1 << 13) | (h1 >> 19); h1 = h1 * 5 + 0xe6546b64;
    }
    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0]; k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h1 ^= h1 >> 16; h1 *= 0x85ebca6b; h1 ^= h1 >> 13; h1 *= 0xc2b2ae35; h1 ^= h1 >> 16;
    return h1;
}
#define LUA_INT_XOR 0xDEADBEEFCAFEBABEULL
#define LUA_INT_ADD 0x123456789ABCDEF0ULL
#define LUA_INT_MUL 3ULL
#define LUA_INT_MUL_INV 0xaaaaaaaaaaaaaaabULL
#define ENCRYPT_INT(i) (((((unsigned long long)(i)) * LUA_INT_MUL) + LUA_INT_ADD) ^ LUA_INT_XOR)
#define DECRYPT_INT(i) (((((unsigned long long)(i)) ^ LUA_INT_XOR) - LUA_INT_ADD) * LUA_INT_MUL_INV)
void obfuscate_proto(lua_State *L, Proto *f, int encrypt_strings);
void lua_security_check(void);
void lua_start_security_thread(void);
#endif
