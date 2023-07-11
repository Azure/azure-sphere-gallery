#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <applibs/storage.h>
#include <applibs/log.h>

#include "crypt.h"

typedef struct StorageHeader {
    char header[4];
} StorageHeader;

static const char* HeaderMagic = "KV00";

const size_t StorageHeaderSize = sizeof(StorageHeader);
const size_t KeyIVOffset = StorageHeaderSize;
const size_t KeyIVSize = sizeof(KeyIV);
const size_t StorageTotalSize = StorageHeaderSize + KeyIVSize;

static int openAndCheckStorage(bool* hasKeyIV)
{
    int storageFd = Storage_OpenMutableFile();
    *hasKeyIV = false;

    if (storageFd == -1) {
        Log_Debug("ERROR: Cannot open mutable storage - have you enabled the correct app permission in the manifest?\n");
        return -1;
    }
    
    off_t size = lseek(storageFd, 0, SEEK_END);
    
    if (size > 0 && size < sizeof(StorageTotalSize)) {
        Log_Debug("ERROR: Storage is incorrect size - cannot retrieve key and IV\n");
        return -1;
    }

    if (size == 0) {
        goto exit;
    }

    StorageHeader sh;
    lseek(storageFd, 0, SEEK_SET);
    read(storageFd, &sh, sizeof(StorageHeader));

    if (memcmp(&sh.header, HeaderMagic, sizeof(sh.header))) {
        Log_Debug("ERROR: Storage header does not match expected magic - cannot retrieve key and IV\n");
        return -1;
    }
    *hasKeyIV = true;

exit:
    lseek(storageFd, 0, SEEK_SET);
    return storageFd;
}

static int createKeyAndIV(int storageFd)
{
    if (storageFd <= 0) {
        return -1;
    }

    WC_RNG *rng = wc_rng_new(NULL, 0, NULL);

    lseek(storageFd, 0, SEEK_SET);

    StorageHeader sh;
    memcpy(&sh.header, HeaderMagic, sizeof(sh.header));

    KeyIV keyIV;
    
    memset(&keyIV, 0, sizeof(keyIV));
    wc_RNG_GenerateBlock(rng, (byte*)&keyIV.key, CHACHA20_POLY1305_AEAD_KEYSIZE);
    wc_RNG_GenerateBlock(rng, (byte*)&keyIV.iv, CHACHA20_POLY1305_AEAD_IV_SIZE);

    write(storageFd, &sh, sizeof(StorageHeader));
    write(storageFd, &keyIV, sizeof(KeyIV));

    // Don't leave the key/IV hanging around on the stack
    memset(&keyIV, 0, sizeof(keyIV));

    return 0;
}

static int getKeyAndIV(int storageFd, KeyIV* kv)
{
    if (storageFd <= 0) {
        return -1;
    }

    if (kv == NULL) {
        return -1;
    }

    off_t pos = lseek(storageFd, KeyIVOffset, SEEK_SET);

    if (pos != KeyIVOffset) { 
        Log_Debug("ERROR: Could not seek to key/IV offset in storage\n");
        return -1;
    }

    if (read(storageFd, kv, sizeof(KeyIV)) != sizeof(KeyIV)) {
        memset(kv, 0, sizeof(KeyIV));
        Log_Debug("ERROR: Error reading key/IV: %s (%d)", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int Crypt_GetOrCreateKeyAndIV(KeyIV* kv)
{
    bool hasKeyIV;
    int storageFd = openAndCheckStorage(&hasKeyIV);

    if (storageFd == -1) {
        Log_Debug("ERROR: Failed to open/check storage; cannot retrieve key and IV\n");
        return -1;
    }

    if (!hasKeyIV) {
        Log_Debug("INFO: No data found in mutable storage; generating key and IV\n");
        int ret = createKeyAndIV(storageFd);
        if (ret != 0) { 
            close(storageFd);
            return ret;
        }
    }

    int result = getKeyAndIV(storageFd, kv) != 0;

    close(storageFd);

    return result;
}