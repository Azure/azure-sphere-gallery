#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"

#include "remoteDiskIO.h"
#include "crypt.h"

// 4MB Storage.

#define PAGE_SIZE     (256)
#define SECTOR_SIZE   (16 * PAGE_SIZE)
#define BLOCK_SIZE    (16 * SECTOR_SIZE)
#define TOTAL_SIZE    (64 * BLOCK_SIZE)

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
    .read_size = 16,
    .prog_size = PAGE_SIZE,
    .block_size = SECTOR_SIZE,
    .block_count = TOTAL_SIZE / SECTOR_SIZE,
    .block_cycles = 1000000,
    .cache_size = PAGE_SIZE,
    .lookahead_size = 16,
};

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    static byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE] = { 0 };
    KeyIV iv;
    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    int result = LFS_ERR_OK;

    uint8_t* data = readBlockData(off, size);

    // if (data != NULL)
    // {
    //     if (wc_ChaCha20Poly1305_Decrypt(iv.key, iv.iv, 0, 0, data, size, authTag, buffer) != 0) {
    //         memset(buffer, 0, size);
    //         // result = LFS_ERR_INVAL;
    //     }
    // }

    memset(&iv, 0, sizeof(KeyIV));
    return result;
}

static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    static byte authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE] = { 0 };
    static byte ciphertext[PAGE_SIZE];
    KeyIV iv;

    // if (size > PAGE_SIZE) {
    //     return LFS_ERR_INVAL;
    // }

    if (Crypt_GetOrCreateKeyAndIV(&iv) != 0) {
        return LFS_ERR_INVAL;
    }

    int result = LFS_ERR_OK;
    // if (wc_ChaCha20Poly1305_Encrypt(iv.key, iv.iv, 0, 0, buffer, size, ciphertext, authTag) != 0) {
    //     result = LFS_ERR_INVAL;
    // } else {
        writeBlockData(buffer, size, off);
    // }

    memset(&iv, 0, sizeof(KeyIV));
    return result;
}

static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
    return LFS_ERR_OK;
}

static int storage_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}
