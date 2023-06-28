#include <applibs/log.h>

#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"

#include "constants.h"
#include "remoteDiskIO.h"
#include "crypt.h"

// 4MB Storage.

#define PAGE_SIZE     (STORAGE_BLOCK_SIZE)
#define SECTOR_SIZE   (16 * PAGE_SIZE)
#define BLOCK_SIZE    (16 * SECTOR_SIZE)
#define TOTAL_SIZE    (64 * BLOCK_SIZE)

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
    .read_size = PAGE_SIZE,
    .prog_size = PAGE_SIZE,
    .block_size = PAGE_SIZE,
    .block_count = TOTAL_SIZE / SECTOR_SIZE,
    .block_cycles = 1000000,
    .cache_size = PAGE_SIZE,
    .lookahead_size = 16,
};

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    static StorageBlock storageblock;
    // static byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE] = { 0 };
    KeyIV iv;
    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    unsigned int block_num = off / STORAGE_BLOCK_SIZE;
    unsigned int block_offset = off % STORAGE_BLOCK_SIZE;

    unsigned int blocks = size / STORAGE_BLOCK_SIZE;
    unsigned int remainder = size % STORAGE_BLOCK_SIZE;

    Log_Debug("Read %d blocks (plus %d) at %d (plus %d)", blocks, remainder, block_num, block_offset);

    if (block_offset != 0 || remainder != 0 || blocks == 0) {
        return LFS_ERR_INVAL;
    }
    
    int retval = LFS_ERR_OK;

    size_t index = 0;
    while (blocks > 0) {
        int result = readBlockData(block_num + index, &storageblock);
        if (result != 0) {

            retval = LFS_ERR_INVAL;
            goto exit;
        }
        memcpy(buffer + (STORAGE_BLOCK_SIZE * index), &storageblock.block, STORAGE_BLOCK_SIZE);
        blocks --;
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
    // static byte ciphertext[PAGE_SIZE];
    KeyIV iv;

    // if (size > PAGE_SIZE) {
    //     return LFS_ERR_INVAL;
    // }

    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    unsigned int block_num = off / STORAGE_BLOCK_SIZE;
    unsigned int block_offset = off % STORAGE_BLOCK_SIZE;

    unsigned int blocks = size / STORAGE_BLOCK_SIZE;
    unsigned int remainder = size % STORAGE_BLOCK_SIZE;

    Log_Debug("Write %d blocks (plus %d) at %d (plus %d)", blocks, remainder, block_num, block_offset);

    if (block_offset != 0 || remainder != 0 || blocks == 0) {
        return LFS_ERR_INVAL;
    }
    
    int retval = LFS_ERR_OK;
    size_t index = 0;
    while (blocks > 0) {
        memcpy(&storageblock.block, buffer + (STORAGE_BLOCK_SIZE * index), STORAGE_BLOCK_SIZE);

        int result = writeBlockData(block_num + index, &storageblock);

        if (result != 0) {
            retval = LFS_ERR_INVAL;
            goto exit;
        }
        blocks --;
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
