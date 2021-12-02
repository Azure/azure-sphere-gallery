#ifndef MT3620_REG_MBOX_H_
#define MT3620_REG_MBOX_H_

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           soft_rst         :  1;
        bool           soft_rst_myself  :  1;
        const unsigned res_31_2         : 30;
    };

    uint32_t mask;
} mt3620_mbox_gen_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint32_t       rd_dly     :  8;
        uint32_t       rcs_dly    :  2;
        const unsigned res_31_10  : 22;
    };

    uint32_t mask;
} mt3620_mbox_fifo_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           int_fifo_rd :  1;
        bool           int_fifo_nf :  1;
        bool           int_fifo_wr :  1;
        bool           int_fifo_ne :  1;
        const unsigned res_31_4    : 28;
    };

    uint32_t mask;
} mt3620_mbox_int_en_t;

typedef mt3620_mbox_int_en_t mt3620_mbox_int_sts_t;

typedef struct {
    volatile uint32_t       mbox_ver;
    volatile uint32_t       mbox_gen_ctrl;
    volatile uint32_t       mbox_dbg_idx;
    volatile uint32_t       mbox_dbg_probe;
    volatile const uint32_t res_4;
    volatile uint32_t       sw_tx_int_port;
    volatile uint32_t       sw_rx_int_en;
    volatile uint32_t       sw_rx_int_sts;
    volatile uint32_t       mbox_fifo_ctrl;
    volatile const uint32_t res_9_11[3];
    volatile uint32_t       mbox_nf_thrs;
    volatile uint32_t       mbox_ne_thrs;
    volatile uint32_t       mbox_int_en;
    volatile uint32_t       mbox_int_sts;
    volatile uint32_t       cmd_push;
    volatile uint32_t       data_push;
    volatile uint32_t       fifo_push_cnt;
    volatile const uint32_t res_19;
    volatile uint32_t       cmd_pop;
    volatile uint32_t       data_pop;
    volatile uint32_t       fifo_pop_cnt;
    volatile const uint32_t res_23;
    volatile uint32_t       semaphore_p;
} mt3620_mbox_t;

/* Note: MT3620_MBOX_CM4 has no semaphore */
typedef enum {
    MT3620_MBOX_CA7 = 0,
    MT3620_MBOX_CM4 = 1,
    MT3620_MBOX_COUNT
} mt3620_mbox_unit;

/* Note: Low power wakeup interrupts not yet supported */
typedef enum {
    MT3620_MBOX_INT_TX_CONFIRMED = 0,
    MT3620_MBOX_INT_TX_FIFO_NF   = 1,
    MT3620_MBOX_INT_RX           = 2,
    MT3620_MBOX_INT_TX_FIFO_NE   = 3,

    MT3620_MBOX_INT_FIFO_INT     = 4,

    MT3620_MBOX_INT_SW_INT       = 5,
    MT3620_MBOX_INT_COUNT
} mt3620_mbox_int;

#define MT3620_MBOX_INTERRUPT(index, int_type) \
    (6 + (index * MT3620_MBOX_INT_COUNT) + int_type)

#define MT3620_MBOX_FIFO_COUNT_MAX 15

static volatile mt3620_mbox_t * const mt3620_mbox[MT3620_MBOX_COUNT] = {
    [MT3620_MBOX_CA7] = (volatile mt3620_mbox_t *)0x21050000,
    [MT3620_MBOX_CM4] = (volatile mt3620_mbox_t *)0x21060000
};

#define MT3620_MBOX_FIELD_READ(i, reg, field) ((mt3620_##reg##_t)mt3620_mbox[i]->reg).field
#define MT3620_MBOX_FIELD_WRITE(i, reg, field, value) do { \
    mt3620_##reg##_t reg = { .mask = mt3620_mbox[i]->reg }; \
    reg.field = value; mt3620_mbox[i]->reg = reg.mask; } while (0)

#endif // #ifndef MT3620_REG_MBOX_H_
