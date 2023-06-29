#include <applibs/log.h>

#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"

#include "constants.h"
#include "remoteDiskIO.h"
#include "crypt.h"

// 4MB Storage.

#define BLOCK_SIZE    (STORAGE_BLOCK_SIZE)
#define TOTAL_SIZE    (STORAGE_SIZE)

_Static_assert (TOTAL_SIZE <= STORAGE_SIZE, "LittleFS total size exceeds backing storage size");

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int storage_erase(const struct lfs_config* c, lfs_block_t block);
static int storage_sync(const struct lfs_config* c);

const struct lfs_config g_littlefs_config = {
    // block device operations
    .read = storage_read,
    .prog = storage_program,
    .erase = storage_erase,
    .sync = storage_sync,
    .read_size = BLOCK_SIZE,
    .prog_size = BLOCK_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = TOTAL_SIZE / BLOCK_SIZE,
    .block_cycles = 1000000,
    .cache_size = BLOCK_SIZE,
    .lookahead_size = BLOCK_SIZE,
};

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    static StorageBlock storageblock;
    // static byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE] = { 0 };
    KeyIV iv;
    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }
    
    unsigned int position = (block * BLOCK_SIZE) + off;

    unsigned int storage_block_num = position / STORAGE_BLOCK_SIZE;
    unsigned int storage_block_offset = position % STORAGE_BLOCK_SIZE;

    unsigned int storage_blocks = size / STORAGE_BLOCK_SIZE;
    unsigned int storage_remainder = size % STORAGE_BLOCK_SIZE;

    Log_Debug("Read block %d off %d size %d\n", block, off, size);
    Log_Debug("=> Storage read %d storage_blocks (plus %d) at %d (plus %d)\n", storage_blocks, storage_remainder, storage_block_num, storage_block_offset);

    if (storage_block_offset != 0 || storage_remainder != 0 || storage_blocks == 0) {
        return LFS_ERR_INVAL;
    }
    
    int retval = LFS_ERR_OK;

    size_t index = 0;
    while (storage_blocks > 0) {
        int result = readBlockData(storage_block_num + index, &storageblock);
        if (result != 0) {

            retval = LFS_ERR_INVAL;
            goto exit;
        }
        memcpy(buffer + (STORAGE_BLOCK_SIZE * index), &storageblock.block, STORAGE_BLOCK_SIZE);
        storage_blocks --;
        index ++;
    }

    // if (data != NULL)
    // {
    //     if (wc_ChaCha20Poly1305_Decrypt(iv.key, iv.iv, 0, 0, data, size, authTag, buffer) != 0) {
    //         memset(buffer, 0, size);
    //         // result = LFS_ERR_INVAL;
    //     }
    // }

exit:
    memset(&storageblock, 0, sizeof(StorageBlock));
    memset(&iv, 0, sizeof(KeyIV));
    return retval;
}

static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    static StorageBlock storageblock;
    // static byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE] = { 0 };
    // static byte ciphertext[BLOCK_SIZE];
    KeyIV iv;

    // if (size > BLOCK_SIZE) {
    //     return LFS_ERR_INVAL;
    // }

    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    unsigned int position = (block * BLOCK_SIZE) + off;

    unsigned int storage_block_num = position / STORAGE_BLOCK_SIZE;
    unsigned int storage_block_offset = position % STORAGE_BLOCK_SIZE;

    unsigned int storage_blocks = size / STORAGE_BLOCK_SIZE;
    unsigned int storage_remainder = size % STORAGE_BLOCK_SIZE;

    Log_Debug("Write block %d off %d size %d\n", block, off, size);
    Log_Debug("=> Storage write %d storage_blocks (plus %d) at %d (plus %d)\n", storage_blocks, storage_remainder, storage_block_num, storage_block_offset);

    if (storage_block_offset != 0 || storage_remainder != 0 || storage_blocks == 0) {
        return LFS_ERR_INVAL;
    }
    
    int retval = LFS_ERR_OK;
    size_t index = 0;
    while (storage_blocks > 0) {
        memcpy(&storageblock.block, buffer + (STORAGE_BLOCK_SIZE * index), STORAGE_BLOCK_SIZE);

        int result = writeBlockData(storage_block_num + index, &storageblock);

        if (result != 0) {
            retval = LFS_ERR_INVAL;
            goto exit;
        }
        storage_blocks --;
        index ++;
    }
    // if (wc_ChaCha20Poly1305_Encrypt(iv.key, iv.iv, 0, 0, buffer, size, ciphertext, authTag) != 0) {
    //     result = LFS_ERR_INVAL;
    // } else {
    // }

exit:
    memset(&storageblock, 0, sizeof(StorageBlock));
    memset(&iv, 0, sizeof(KeyIV));
    return retval;
}

static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
    return LFS_ERR_OK;
}

static int storage_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}
