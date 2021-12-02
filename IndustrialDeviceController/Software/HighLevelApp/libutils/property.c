/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>

#include <init/globals.h>
#include <utils/memory.h>

const char PROPERTY_FILE_MAGIC[8] = {'P', 'R', 'O', 'P', ' ', 'V', '0', '1'};

struct property_file_hdr_t {
    char magic[8];
    size_t size;
};

#define PROPERTY_FILE_HDR_SIZE sizeof(struct property_file_hdr_t)

static int open_property_file(size_t *size)
{
    int fd = -1;

    *size = 0;

    if ((fd = Storage_OpenMutableFile()) < 0) return -1;

    if (lseek(fd, PROPERTY_FILE_OFFSET, SEEK_SET) < 0) return fd;

    struct property_file_hdr_t hdr;
    read(fd, &hdr, sizeof(hdr));

    if ((memcmp(hdr.magic, PROPERTY_FILE_MAGIC, sizeof(PROPERTY_FILE_MAGIC)) != 0) ||
        (hdr.size > PROPERTY_FILE_SIZE - PROPERTY_FILE_HDR_SIZE)) return fd;

    *size = hdr.size;
    return fd;
}


char *read_property(const char *key)
{
    size_t size = 0;

    int fd = open_property_file(&size);
    if (fd < 0 || size <= 0) return NULL;

    char buf[size+1];
    read(fd, buf, size);
    buf[size] = 0;
    close(fd);

    char *pair = strtok(buf, ",");
    int key_len = strlen(key);
    while (pair) {
        char *equal_sign = strchr(pair, '=');
        if (equal_sign) {
            int ckey_len = equal_sign - pair;
            if (key_len == ckey_len && strncmp(key, pair, ckey_len) == 0) {
                return STRDUP(equal_sign + 1);
            }
        }
        pair = strtok(NULL, ",");
    }
    return NULL;
}

int write_property(const char *key, char *value)
{
    size_t size = 0;
    int fd = open_property_file(&size);
    if (fd < 0) return -1;

    const int PROPERTY_FILE_BODY_SIZE = PROPERTY_FILE_SIZE - PROPERTY_FILE_HDR_SIZE;
    char buf_new[PROPERTY_FILE_BODY_SIZE+1];
    int ntotal = 0;

    ntotal += snprintf(buf_new, PROPERTY_FILE_BODY_SIZE, "%s=%s,", key, value);

    if (size > 0) {
        char buf_old[size+1];
        read(fd, buf_old, size);
        buf_old[size] = 0;

        char *pair = strtok(buf_old, ",");
        int key_len = strlen(key);
        while (pair) {
            char *equal_sign = strchr(pair, '=');
            if (equal_sign) {
                int ckey_len = equal_sign - pair;
                if (key_len != ckey_len || strncmp(key, pair, ckey_len) != 0) {
                    ntotal += snprintf(buf_new + ntotal, PROPERTY_FILE_BODY_SIZE - ntotal, "%s,", pair);
                }
            }
            pair = strtok(NULL, ",");
        }
    }

    lseek(fd, PROPERTY_FILE_OFFSET, SEEK_SET);
    struct property_file_hdr_t hdr;
    memcpy(&hdr.magic, PROPERTY_FILE_MAGIC, sizeof(PROPERTY_FILE_MAGIC));
    hdr.size = ntotal;
    write(fd, &hdr, sizeof(hdr));
    write(fd, buf_new, ntotal);
    close(fd);
    return 0;
}
