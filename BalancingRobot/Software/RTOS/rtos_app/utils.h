#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "timer.h"
#include "os_hal_i2c.h"
#include "printf.h"

// #define SHOW_DEBUG_MSGS 1

#define Log_Entry() printf(">>> %s\r\n",__func__);
typedef void(*callback)(void);

#define memset __builtin_memset
#define memcpy __builtin_memcpy

#define delay tx_thread_sleep

// Interrupt callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode* next;
    void (*cb)(void);
} CallbackNode;
void EnqueueCallback(CallbackNode* node);

// Intercore communication
void EnqueueIntercoreMessage(void* payload, size_t payload_size);

unsigned long millis(void);
void EnumI2CDevices(i2c_num driver);

void DumpBuffer(uint8_t* buffer, uint16_t length);

#endif
