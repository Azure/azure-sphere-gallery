/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <applibs/log.h>
#include <applibs/storage.h>

#include "wolfssl/wolfcrypt/chacha20_poly1305.h"

static const char* MAGIC = "ENC0";

typedef struct {
    uint32_t counter;
} State;

typedef struct {
    char magic[4];
    byte data[sizeof(State)];
    byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE];
} Storage;

_Static_assert(sizeof(Storage) == 4 + sizeof(State) + CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE);

// ************************** IMPORTANT ************************************
// This project uses hard-coded encryption Key and IV values.
// 
// THIS IS NOT SECURE. YOU SHOULD NOT DO THIS IN PRODUCTION CODE
// ************************** IMPORTANT ************************************

const byte Key[] = { 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,

    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

const byte IV[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b
};

_Static_assert(sizeof(Key) == CHACHA20_POLY1305_AEAD_KEYSIZE);
_Static_assert(sizeof(IV) == CHACHA20_POLY1305_AEAD_IV_SIZE);

// Additional authentication data for validating decryption
const char AAD[] = "Encrypted storage demo";

static State state;

static Storage storage;

enum {
    SaveState_Ok        = 0,
    SaveState_Open      = -1,
    SaveState_Encrypt   = -2,
    SaveState_Save      = -3
};

int SaveState(void )
{
    memcpy(&storage.magic, MAGIC, sizeof(storage.magic));

    if (wc_ChaCha20Poly1305_Encrypt(
            Key, IV, AAD, sizeof(AAD),
            (byte*)&state, sizeof(State),
            (byte*)&storage.data, (byte*)&storage.authTag) != 0) {
        
        Log_Debug("ERROR: Failed to encrypt state\n");
        return SaveState_Encrypt;
    };

    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Cannot open mutable storage: %s (%d)\n", strerror(errno), errno);
        return SaveState_Open;
    }

    if (write(fd, &storage, sizeof(Storage)) != sizeof(Storage)) {
        Log_Debug("ERROR: Failed to save state to mutable storage.\n");
        close(fd);
        return SaveState_Save;
    }

    close(fd);
    return SaveState_Ok;
}

enum {
    LoadState_Ok        = 0,
    LoadState_Open      = -1,
    LoadState_NotInit   = -2,
    LoadState_Magic     = -3,
    LoadState_Decrypt   = -4,
};

int LoadState(void)
{
    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Cannot open mutable storage: %s (%d)\n", strerror(errno), errno);
        return LoadState_Open;
    }

    memset(&storage, 0, sizeof(Storage));
    ssize_t bytes = read(fd, &storage, sizeof(Storage));
    close(fd);

    if (bytes != sizeof(Storage)) {
        Log_Debug("INFO: Storage not initialized\n");
        return LoadState_NotInit;
    }

    if (memcmp(storage.magic, MAGIC, sizeof(storage.magic)) != 0) {
        Log_Debug("ERROR: Storage corrupt/unreadable\n");
        return LoadState_Magic;
    }

    if (wc_ChaCha20Poly1305_Decrypt(
            Key, IV, AAD, sizeof(AAD),
            storage.data, sizeof(State), storage.authTag,
            (byte*)&state) != 0) {
        Log_Debug("ERROR: Failed to decrypt state\n");
        return LoadState_Decrypt;
    }

    return LoadState_Ok;
}

void LogState(void)
{
    Log_Debug("Counter = %d\n", state.counter);
}

int main(void)
{
    Log_Debug("Encrypted storage project\n");

    Log_Debug("Loading state...\n");
    int result = LoadState();

    if (result == LoadState_NotInit) {
        Log_Debug("Initializing state to default\n");
        memset(&state, 0, sizeof(State));
    }

    Log_Debug("State on entry:\n");
    LogState();

    state.counter ++;

    Log_Debug("State on exit:\n");
    LogState();

    Log_Debug("Saving state...\n");
    return SaveState();
}