#include <stdio.h>

#define LUA_INT_XOR       0xDEADBEEFCAFEBABEULL
#define LUA_INT_ADD       0x123456789ABCDEF0ULL
#define LUA_INT_MUL       3ULL
#define LUA_INT_MUL_INV   0xaaaaaaaaaaaaaaabULL

#define ENCRYPT_INT(i) (((((unsigned long long)(i)) * LUA_INT_MUL) + LUA_INT_ADD) ^ LUA_INT_XOR)
#define DECRYPT_INT(i) (((((unsigned long long)(i)) ^ LUA_INT_XOR) - LUA_INT_ADD) * LUA_INT_MUL_INV)

int main() {
    long long original = 10;
    unsigned long long encrypted = ENCRYPT_INT(original);
    long long decrypted = (long long)DECRYPT_INT(encrypted);
    printf("Original: %lld\n", original);
    printf("Encrypted: %llu (hex: %llx)\n", encrypted, encrypted);
    printf("Decrypted: %lld\n", decrypted);

    original = 15;
    encrypted = ENCRYPT_INT(original);
    decrypted = (long long)DECRYPT_INT(encrypted);
    printf("Original: %lld, Decrypted: %lld\n", original, decrypted);
    return 0;
}
