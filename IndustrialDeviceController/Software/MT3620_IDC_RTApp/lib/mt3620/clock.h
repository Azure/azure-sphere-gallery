#ifndef MT3620_REG_CLOCK_H__
#define MT3620_REG_CLOCK_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MT3620_CLOCK_CRYSTAL  = 0,
    MT3620_CLOCK_32K      = 1,
    MT3620_CLOCK_PLL_200M = 2,
    MT3620_CLOCK_COUNT
} mt3620_clock_t;

static const unsigned mt3620_clock_freq[MT3620_CLOCK_COUNT] = {
    [MT3620_CLOCK_CRYSTAL ] =  26000000,
    [MT3620_CLOCK_32K     ] =     32768,
    [MT3620_CLOCK_PLL_200M] = 197600000,
};

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       unknown_7_0       :  8;
        mt3620_clock_t hclk_clock_source :  2;
        unsigned       unknown_31_10     : 22;
    };

    uint32_t mask;
} mt3620_io_cm4_rgu_t;

static volatile mt3620_io_cm4_rgu_t * const mt3620_io_cm4_rgu
    = (volatile mt3620_io_cm4_rgu_t *)0x2101000c;

static inline mt3620_clock_t mt3620_hclk_clock_source_get(void)
{
    mt3620_io_cm4_rgu_t rgu = { .mask = mt3620_io_cm4_rgu->mask };
    return rgu.hclk_clock_source;
}

static inline void mt3620_hclk_clock_source_set(mt3620_clock_t src)
{
    mt3620_io_cm4_rgu_t rgu = { .mask = mt3620_io_cm4_rgu->mask };
    rgu.hclk_clock_source = src;
    mt3620_io_cm4_rgu->mask = rgu.mask;
}

#endif
