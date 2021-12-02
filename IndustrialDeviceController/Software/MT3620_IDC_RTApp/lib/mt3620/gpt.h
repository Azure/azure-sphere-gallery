#ifndef MT3620_REG_GPT_H_
#define MT3620_REG_GPT_H_

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           gpt0_int  :  1;
        bool           gpt1_int  :  1;
        const unsigned res_31_2  : 30;
    };

    uint32_t mask;
} mt3620_gpt_isr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           gpt0_int_en :  1;
        bool           gpt1_int_en :  1;
        const unsigned res_31_2    : 30;
    };

    uint32_t mask;
} mt3620_gpt_ier_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           en       :  1;
        bool           mode     :  1;
        bool           speed    :  1;
        bool           restart  :  1;
        const unsigned res_31_4 : 28;
    };

    uint32_t mask;
} mt3620_gpt0_ctrl_t;

typedef mt3620_gpt0_ctrl_t mt3620_gpt1_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           en       :  1;
        bool           speed    :  1;
        const unsigned res_31_2 : 30;
    };

    uint32_t mask;
} mt3620_gpt2_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           en          :  1;
        const unsigned res_14_1    : 14;
        bool           gpt3_iclr   :  1;
        unsigned       osc_cnt_1us :  6;
        const unsigned res_31_22   : 10;
    };

    uint32_t mask;
} mt3620_gpt3_ctrl_t;

typedef mt3620_gpt2_ctrl_t mt3620_gpt4_ctrl_t;

// TODO: should I split these out into one struct per timer?
//       the interface isn't the same, but I'd be able to make an
//       indexable list, rather than having loads of switches.
typedef struct {
    volatile uint32_t       gpt_isr;
    volatile uint32_t       gpt_ier;
    volatile const uint32_t res_2_3[2];
    volatile uint32_t       gpt0_ctrl;
    volatile uint32_t       gpt0_icnt;
    volatile const uint32_t res_6_7[2];
    volatile uint32_t       gpt1_ctrl;
    volatile uint32_t       gpt1_icnt;
    volatile const uint32_t res_10_11[2];
    volatile uint32_t       gpt2_ctrl;
    volatile uint32_t       gpt2_cnt;
    volatile const uint32_t res_14_15[2];
    volatile uint32_t       gpt0_cnt;
    volatile uint32_t       gpt1_cnt;
    volatile const uint32_t res_18_19[2];
    volatile uint32_t       gpt3_ctrl;
    volatile uint32_t       gpt3_init;
    volatile uint32_t       gpt3_cnt;
    volatile uint32_t       gpt3_expire;
    volatile uint32_t       gpt4_ctrl;
    volatile uint32_t       gpt4_init;
    volatile uint32_t       gpt4_cnt;
} mt3620_gpt_t;

static volatile mt3620_gpt_t * const mt3620_gpt
    = (volatile mt3620_gpt_t *)0x21030000;

#define ROUND_DIVIDE(x, y) ((x + (y / 2)) / y)

// frequencies in Hz
#define MT3620_GPT_BUS_CLOCK 26000000.0f
#define MT3620_GPT_012_HIGH_SPEED 32768.0f
#define MT3620_GPT_012_LOW_SPEED ROUND_DIVIDE(MT3620_GPT_012_HIGH_SPEED, 33) // (should be 32/33 kHz - according to datasheet)
#define MT3620_GPT_3_SRC_CLK_HZ MT3620_GPT_BUS_CLOCK
#define MT3620_GPT_3_LOW_SPEED ROUND_DIVIDE(MT3620_GPT_BUS_CLOCK, 26)
#define MT3620_GPT_3_HIGH_SPEED MT3620_GPT_BUS_CLOCK
#define MT3620_GPT_3_SPEED_RANGE (MT3620_GPT_3_HIGH_SPEED - MT3620_GPT_3_LOW_SPEED)

#define MT3620_GPT_MAX_COUNT UINT32_MAX

#define MT3620_GPT_FIELD_READ(reg, field) ((mt3620_##reg##_t)mt3620_gpt->reg).field
#define MT3620_GPT_FIELD_WRITE(reg, field, value) do { \
    mt3620_##reg##_t reg = { .mask = mt3620_gpt->reg }; \
    reg.field = value; mt3620_gpt->reg = reg.mask; } while (0)

#endif // #ifdef MT3620_REG_GPT_H_
