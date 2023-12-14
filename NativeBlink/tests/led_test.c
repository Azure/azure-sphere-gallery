#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "hw/mt3620_rdb.h"
#include "../led.h"

int main(void)
{
    int fd = LED_Open(MT3620_RDB_LED1_RED);
    if (fd == -1) {
        fprintf(stderr, "FAIL: LED_Open failed: %s", strerror(errno));
        return -1;
    }

    int onResult = LED_On(fd);
    if (onResult == -1) {
        fprintf(stderr, "FAIL: LED_On failed: %s", strerror(errno));
        return -1;
    }

    int offResult = LED_Off(fd);
    if (offResult == -1) {
        fprintf(stderr, "FAIL: LED_Off failed: %s", strerror(errno));
        return -1;
    }

    int closeResult = LED_Close(fd);
    if (closeResult == -1) {
        fprintf(stderr, "FAIL: LED_Close failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}