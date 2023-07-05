#pragma once

#include <stdint.h>

#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>

typedef struct KeyIV {
    uint8_t key[CHACHA20_POLY1305_AEAD_KEYSIZE];
    uint8_t iv[CHACHA20_POLY1305_AEAD_IV_SIZE];
} KeyIV;

int Crypt_GetOrCreateKeyAndIV(KeyIV* kv);