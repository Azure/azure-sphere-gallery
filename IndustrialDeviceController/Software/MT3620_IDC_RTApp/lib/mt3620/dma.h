#ifndef MT3620_REG_DMA_H_
#define MT3620_REG_DMA_H_

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       len       : 16;
        bool           rllen     :  1;
        const unsigned res_31_17 : 15;
    };

    uint32_t mask;
} mt3620_dma_count_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       size      :  2;
        bool           sinc      :  1;
        bool           dinc      :  1;
        bool           dreq      :  1;
        bool           b2w       :  1;
        const unsigned res_7_6   :  2;
        unsigned       burst     :  3;
        const unsigned res_12_11 :  2;
        bool           hiten     :  1;
        bool           toen      :  1;
        bool           iten      :  1;
        unsigned       wpsd      :  1;
        bool           wpen      :  1;
        unsigned       dir       :  1;
        const unsigned res_31_19 : 13;
    };

    uint32_t mask;
} mt3620_dma_con_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_14_0  : 15;
        bool           str       :  1;
        const unsigned res_31_16 : 16;
    };

    uint32_t mask;
} mt3620_dma_start_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_12_0  : 13;
        const bool     hint      :  1;
        const unsigned res_14    :  1;
        const bool     _int      :  1;
        const bool     toint     :  1;
        const unsigned res_31_17 : 15;
    };

    uint32_t mask;
} mt3620_dma_intsta_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_12_0  : 13;
        bool           hack      :  1;
        const unsigned res_14    :  1;
        bool           ack       :  1;
        bool           toack     :  1;
        const unsigned res_31_17 : 15;
    };

    uint32_t mask;
} mt3620_dma_ackint_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     full     :  1;
        const bool     empty    :  1;
        const bool     alt      :  1;
        const unsigned res_31_3 : 29;
    };

    uint32_t mask;
} mt3620_dma_ffsta_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       altlen    : 16;
        const unsigned res_30_16 : 15;
        bool           altscm    :  1;
    };

    uint32_t mask;
} mt3620_dma_altlen_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       hwptr      : 16;
        const bool     hwptr_wrap :  1;
        const unsigned res_31_17  : 15;
    };

    uint32_t mask;
} mt3620_dma_hwptr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       swptr      : 16;
        bool           swptr_wrap :  1;
        const unsigned res_31_17  : 15;
    };

    uint32_t mask;
} mt3620_dma_swptr_t;

typedef struct {
    void * volatile         src;
    void * volatile         dst;
    volatile uint32_t       wppt;
    void * volatile         wpto;
    volatile uint32_t       count;
    volatile uint32_t       con;
    volatile uint32_t       start;
    volatile const uint32_t intsta;
    volatile uint32_t       ackint;
    volatile const uint32_t rlct;
    volatile uint32_t       limiter;
    void * volatile         pgmaddr;
    volatile const uint32_t res_12_13[2];
    volatile uint32_t       ffcnt;
    volatile uint32_t       ffsta;
    volatile uint32_t       altlen;
    volatile uint32_t       ffsize;
    volatile const uint32_t res_18_19[2];
    volatile uint32_t       to;
    volatile const uint32_t res_21;
    volatile uint32_t       hwptr;
    volatile uint32_t       swptr;
    void * volatile         fixaddr;
    volatile const uint32_t res_25_63[39];
} mt3620_dma_t;

typedef struct {
    volatile const uint32_t glbsta0;
    volatile const uint32_t glbsta1;
    volatile const uint32_t res_2;
    volatile uint32_t       glo_con;
    volatile uint32_t       group0;
    volatile uint32_t       group1;
    volatile uint32_t       isu_vfifo;
    volatile uint32_t       isu_vfifo_err;
    volatile uint32_t       ch_en;
    volatile uint32_t       ch_en_set;
    volatile uint32_t       ch_en_clr;
    volatile const uint32_t res_11;
    volatile uint32_t       glb_pause;
    volatile const uint32_t glb_sta_pause;
} mt3620_dma_global_t;

#define MT3620_DMA_COUNT 30
static volatile mt3620_dma_t * const mt3620_dma
    = (volatile mt3620_dma_t *)0x21080000;

static volatile mt3620_dma_global_t * const mt3620_dma_global
    = (volatile mt3620_dma_global_t *)0x21084000;

#define MT3620_DMA_INTERRUPT 77

#define MT3620_DMA_FIELD_READ(index, reg, field) ((mt3620_dma_##reg##_t)mt3620_dma[index].reg).field
#define MT3620_DMA_FIELD_WRITE(index, reg, field, value) do { mt3620_dma_##reg##_t reg = { .mask = mt3620_dma[index].reg }; reg.field = value; mt3620_dma[index].reg = reg.mask; } while (0)

#define MT3620_DMA_GLOBAL_FIELD_READ(reg, field) ((mt3620_dma_global_##reg##_t)mt3620_dma_global->reg).field
#define MT3620_DMA_GLOBAL_FIELD_WRITE(reg, field, value) do { mt3620_dma_global_##reg##_t reg = { .mask = mt3620_dma_global->reg }; reg.field = value; mt3620_dma_global->reg = reg.mask; } while (0)

#endif // #ifdef MT3620_REG_DMA_H_
