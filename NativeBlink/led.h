#pragma once

#include <applibs/gpio.h>

int LED_Open(GPIO_Id id);

int LED_Close(int fd);

int LED_On(int fd);

int LED_Off(int fd);