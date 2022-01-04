#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// #define SHOW_DEBUG_MSGS 1

#define FLOAT_TO_INT(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))

bool IsNetworkReady(void);
void delay(int ms);
int genGuid(unsigned char* buf, size_t len);

typedef void(*callback)(void);

#endif
