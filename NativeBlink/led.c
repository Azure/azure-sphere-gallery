#include <unistd.h>

#include "led.h"

int LED_Open(GPIO_Id id)
{
    return GPIO_OpenAsOutput(id, GPIO_OutputMode_PushPull, GPIO_Value_High);
}

int LED_Close(int fd)
{
    return close(fd);
}

int LED_On(int fd)
{
    return GPIO_SetValue(fd, GPIO_Value_Low);
}

int LED_Off(int fd)
{
    return GPIO_SetValue(fd, GPIO_Value_High);
}