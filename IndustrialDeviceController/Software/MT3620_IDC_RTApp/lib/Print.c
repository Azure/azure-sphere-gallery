/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "Print.h"
#include <stddef.h>

#define PRINT_MAX_WIDTH 10
#define PRINT_TEMP_PRINTF_BUFFER 256
#define PRINT_FLOAT_BASE 10
#define PRINT_FLOAT_SIGDIG_DEFAULT 6

int32_t UART_Print(UART *handle, const char *msg)
{
    if (!msg) {
        return ERROR_PARAMETER;
    }
    return UART_Write(handle, (const uint8_t *)msg, __builtin_strlen(msg));
}

int32_t UART__PrintUIntBaseFiller(
    UART    *handle,
    int32_t  value,
    unsigned base,
    unsigned width,
    bool     upper,
    char     filler)
{
    if ((base == 0) || (base > 36)) {
        return ERROR_UNSUPPORTED;
    }

    if (width > PRINT_MAX_WIDTH) {
        return ERROR_UNSUPPORTED;
    }

    // Maximum decimal length is ten digits.
    char buff[PRINT_MAX_WIDTH];

    uint32_t p, w, v;
    for (p = PRINT_MAX_WIDTH, w = 0, v = value;
        (width == 0 ? ((v != 0) || (w == 0)): (w < width)); w++, v /= base) {
        unsigned digit;
        if ((value != 0) && (v == 0)) {
            digit = filler;
        }
        else {
            digit = (v % base);
            if (digit < 10) {
                digit += '0';
            } else {
                digit = (digit - 10) + (upper ? 'A' : 'a');
            }
        }

        buff[--p] = digit;
    }

    return UART_Write(handle, &buff[p], w);
}

int32_t UART_PrintUIntBase(
    UART     *handle,
    int32_t   value,
    unsigned  base,
    unsigned  width,
    bool      upper)
{
    return UART__PrintUIntBaseFiller(handle, value, base, width, upper, '0');
}

static inline int32_t UART__PrintIntBaseFiller(
    UART     *handle,
    int32_t   value,
    unsigned  base,
    unsigned  width,
    bool      upper,
    char      filler)
{
    if (value < 0) {
        int32_t error = UART_Print(handle, "-");
        if (error != ERROR_NONE) {
            return error;
        }
        value = -value;
    }

    return UART__PrintUIntBaseFiller(handle, value, base,
                                     width, upper, filler);
}

int32_t UART_PrintIntBase(
    UART     *handle,
    int32_t   value,
    unsigned  base,
    unsigned  width,
    bool      upper)
{
    return UART__PrintIntBaseFiller(handle, value, base, width, upper, '0');
}

static inline int32_t Int32_Power(int32_t base, int32_t exp)
{
    int32_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

static inline int32_t UART__PrintFloatFiller(
    UART     *handle,
    float     value,
    unsigned  sigDigits,
    unsigned  width,
    char      filler)
{
    if (width > PRINT_MAX_WIDTH) {
        return ERROR_UNSUPPORTED;
    }
    int32_t error;

    if (value < 0) {
        error = UART_Print(handle, "-");
        if (error != ERROR_NONE) {
            return error;
        }
        value = -value;
    }

    if (sigDigits == 0) {
        sigDigits = PRINT_FLOAT_SIGDIG_DEFAULT;
    }

    /* Get LH and RH side of float */
    int32_t leftHand, rightHand;

    leftHand  = (int32_t)value;
    rightHand = ((value - leftHand) * Int32_Power(
        PRINT_FLOAT_BASE, sigDigits));

    int32_t lhWidth;

    if ((width == 0) || ((lhWidth = width - 1 - sigDigits) > 0)) {
        lhWidth = 0;
    }

    /* Print LH.RH */
    if ((error = UART__PrintUIntBaseFiller(handle, leftHand,
                                           PRINT_FLOAT_BASE, (unsigned)lhWidth,
                                           false, filler)) != ERROR_NONE) {
        return error;
    }

    if ((error = UART_Print(handle, ".")) != ERROR_NONE) {
        return error;
    }

    if ((error = UART_PrintUIntBase(handle, rightHand,
                                    PRINT_FLOAT_BASE, sigDigits,
                                    false)) != ERROR_NONE) {
        return error;
    }
    return ERROR_NONE;
}

int32_t UART_PrintFloatFiller(
    UART     *handle,
    float     value,
    unsigned  sigDigits,
    unsigned  width)
{
    return UART__PrintFloatFiller(handle, value, sigDigits, width, '0');
}

typedef struct {
    unsigned width;
    char     type;
    char     filler;
    unsigned sigDigits;
    int32_t  error;
} formatSpec;

static inline formatSpec parseFormatSpecifier(
    UART      *handle,
    char      *current,
    char     **end)
{
    formatSpec spec = {.width     = 0,
                       .type      = 'd',
                       .filler    = ' ',
                       .sigDigits = 0,
                       .error     = ERROR_NONE};
    if (!handle || !current) {
        spec.error = ERROR_PARAMETER;
        return spec;
    }
    bool seenType = false, seenPoint = false;
    unsigned temp = 0;
    while (!seenType) {
        if ((*current >= '0') && (*current <= '9')) {
            if ((temp == 0) && (*current == '0')) {
                // filler spec
                spec.filler = '0';
            }
            else {
                temp *= 10;
                temp += (*current) - '0';
            }
        }
        else if (*current == '.') {
            spec.width = temp;
            temp       = 0;
            seenPoint  = true;
        }
        else if ((*current >= 'a') || (*current <= 'z')) {
            if (*current == 'l') {
                // ignore long types
                current++;
                continue;
            }
            spec.type = *current;
            if (seenPoint) {
                spec.sigDigits = temp;
            }
            else {
                spec.width = temp;
            }
            seenType = true;
            *end     = current;
        }
        else {
            spec.error = ERROR_UART_PRINTF_INVALID;
            return spec;
        }
        current++;
    }

    return spec;
}

int32_t UART_vPrintf(UART *handle, const char *format, va_list args)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    char     tempBuffer[PRINT_TEMP_PRINTF_BUFFER] = {'\0'};
    unsigned tempIndex = 0;
    int32_t  error = ERROR_NONE;

    /* Loop through string and look for format specifier */
    char       *start = NULL, *end = NULL;
    formatSpec  spec;
    char        c;
    while (*format != '\0') {
        if (tempIndex >= (PRINT_TEMP_PRINTF_BUFFER - 1)) {
            error = ERROR_UART_PRINTF_INVALID;
            break;
        }

        if (*format == '%') {
            start = (char*)(format + 1);
            if (*start == '%') {
                // handle %% pseudo-char
                tempBuffer[tempIndex++] = *format;
                format += 2;
                continue;
            }
            spec = parseFormatSpecifier(
                handle, start, &end);
            if (spec.error != ERROR_NONE) {
                error = spec.error;
                break;
            }
            format = end + 1;
            if (tempIndex > 0) {
                /* Write previously globbed tempBuffer */
                tempBuffer[tempIndex] = '\0';
                tempIndex             = 0;
                if ((error = UART_Print(
                    handle, (const char*)tempBuffer)) != ERROR_NONE) {
                    break;
                }
            }
            /* Write formatted arg */
            switch (spec.type) {
            case 'd':
            case 'i':
                error = UART__PrintIntBaseFiller(
                    handle, va_arg(args, int), 10,
                    spec.width, false, spec.filler);
                break;
            case 'u':
                error = UART__PrintUIntBaseFiller(
                    handle, va_arg(args, uint32_t), 10,
                    spec.width, false, spec.filler);
                break;
            case 'x':
                error = UART__PrintUIntBaseFiller(
                    handle, va_arg(args, uint32_t), 16,
                    spec.width, false, spec.filler);
                break;
            case 'o':
                error = UART__PrintUIntBaseFiller(
                    handle, va_arg(args, uint32_t), 8,
                    spec.width, false, spec.filler);
                break;
            case 'f':
                error = UART__PrintFloatFiller(
                    handle, (float)(va_arg(args, double)), spec.sigDigits,
                    spec.width, spec.filler);
                break;
            case 's':
                error = UART_Print(
                    handle, va_arg(args, const char*));
                break;
            case 'c':
                c = (char)va_arg(args, int);
                error = UART_Print(
                    handle, (const char*)(&c));
                break;
            }
        }
        else {
            tempBuffer[tempIndex++] = *format;
            format++;
        }
    }

    if (error == ERROR_NONE) {
        /* Write previously globbed tempBuffer */
        tempBuffer[tempIndex] = '\0';
        error = UART_Print(handle, (const char*)tempBuffer);
    }

    return error;
}


int32_t UART_Printf(UART *handle, const char *format, ...)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    va_list args;
    va_start(args, format);

    int32_t error = UART_vPrintf(handle, format, args);

    va_end(args);
    return error;
}
