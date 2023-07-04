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
    static char AAD[16];

    KeyIV iv;
    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }
    
    uint32_t position = (block * BLOCK_SIZE) + off;

    uint32_t storage_block_num = position / STORAGE_BLOCK_SIZE;
    uint32_t storage_block_offset = position % STORAGE_BLOCK_SIZE;

    uint32_t storage_blocks = size / STORAGE_BLOCK_SIZE;
    uint32_t storage_remainder = size % STORAGE_BLOCK_SIZE;

#ifdef ENCRYPTED_STORAGE_DEBUG
    Log_Debug("Read block %d off %d size %d\n", block, off, size);
    Log_Debug("=> Storage read %d storage_blocks (plus %d) at %d (plus %d)\n", storage_blocks, storage_remainder, storage_block_num, storage_block_offset);
#endif

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

        memset(AAD, 0, sizeof(AAD));
        snprintf(AAD, sizeof(AAD), "%d", index);

        void* dest = buffer + (STORAGE_BLOCK_SIZE * index);
        if (wc_ChaCha20Poly1305_Decrypt(iv.key, iv.iv,
                                        AAD, sizeof(AAD),
                                        storageblock.block, STORAGE_BLOCK_SIZE,
                                        storageblock.metadata, dest) != 0) {
            memset(dest, 0, STORAGE_BLOCK_SIZE);
            Log_Debug("WARN: Unable to decrypt block %d\n", storage_block_num + index);
        }
        storage_blocks --;
        index ++;
    }

exit:
    memset(&storageblock, 0, sizeof(StorageBlock));
    memset(&iv, 0, sizeof(KeyIV));
    return retval;
}

static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    static char AAD[16];
    static StorageBlock storageblock;
    KeyIV iv;

    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    uint32_t position = (block * BLOCK_SIZE) + off;

    uint32_t storage_block_num = position / STORAGE_BLOCK_SIZE;
    uint32_t storage_block_offset = position % STORAGE_BLOCK_SIZE;

    uint32_t storage_blocks = size / STORAGE_BLOCK_SIZE;
    uint32_t storage_remainder = size % STORAGE_BLOCK_SIZE;

#ifdef ENCRYPTED_STORAGE_DEBUG
    Log_Debug("Write block %d off %d size %d\n", block, off, size);
    Log_Debug("=> Storage write %d storage_blocks (plus %d) at %d (plus %d)\n", storage_blocks, storage_remainder, storage_block_num, storage_block_offset);
#endif

    if (storage_block_offset != 0 || storage_remainder != 0 || storage_blocks == 0) {
        return LFS_ERR_INVAL;
    }
    
    int retval = LFS_ERR_OK;
    size_t index = 0;
    while (storage_blocks > 0) {
        memset(AAD, 0, sizeof(AAD));
        snprintf(AAD, sizeof(AAD), "%d", index);

        const void *src = buffer + (STORAGE_BLOCK_SIZE * index);
        if (wc_ChaCha20Poly1305_Encrypt(iv.key, iv.iv,
                                        AAD, sizeof(AAD),
                                        src, STORAGE_BLOCK_SIZE,
                                        (byte*)&storageblock.block, (byte*)&storageblock.metadata) != 0) {
            Log_Debug("WARN: Unable to encrypt block %d\n", storage_block_num + index);
            retval = LFS_ERR_INVAL;
            goto exit;
        }

        int result = writeBlockData(storage_block_num + index, &storageblock);

        if (result != 0) {
            retval = LFS_ERR_INVAL;
            goto exit;
        }
        storage_blocks --;
        index ++;
    }

exit:
    memset(&storageblock, 0, sizeof(StorageBlock));
    memset(&iv, 0, sizeof(KeyIV));
    return retval;
}

static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
#ifdef ENCRYPTED_STORAGE_DEBUG
    Log_Debug("Erase block %d\n", block);
#endif
    return LFS_ERR_OK;
}

static int storage_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}
