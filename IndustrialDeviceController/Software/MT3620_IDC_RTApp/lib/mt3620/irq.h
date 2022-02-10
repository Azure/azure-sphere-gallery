#ifndef MT3620_REG_IRQ_H_
#define MT3620_REG_IRQ_H_

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       cnt      :  4;
        unsigned       prescal  :  3;
        bool           dual     :  1;
        bool           pol      :  1;
        bool           en       :  1;
        const unsigned res_31_9 : 22;
    };

    uint32_t mask;
} mt3620_irq_dbnc_con_t;

#define MT3620_IRQ_EINT_CNT      24
#define MT3620_IRQ_EINT_TEMP_CNT  8

#define MT3620_IRQ_CFG_IRQ_CNT 4
#define MT3620_IRQ_WIC_CNT     4
#define MT3620_IRQ_TEST_CNT    4

typedef struct {
    volatile uint32_t       dbnc_con        [MT3620_IRQ_EINT_CNT];
    volatile uint32_t       dbnc_con_temp   [MT3620_IRQ_EINT_TEMP_CNT];
    volatile const uint32_t res_32_128      [96];
    volatile uint32_t       irq_sens        [MT3620_IRQ_CFG_IRQ_CNT];
    volatile const uint32_t res_132_192     [61];
    volatile uint32_t       wic_rel_pend_val[MT3620_IRQ_WIC_CNT];
    volatile const uint32_t res_197_200     [4];
    volatile uint32_t       wic_rel_en_val  [MT3620_IRQ_WIC_CNT];
    volatile const uint32_t res_205_207     [3];
    volatile uint32_t       wic_rel_ctrl;
    volatile const uint32_t res_210_216     [7];
    volatile uint32_t       wic_pend_stat   [MT3620_IRQ_WIC_CNT];
    volatile const uint32_t res_221_224     [4];
    volatile uint32_t       wic_en_stat     [MT3620_IRQ_WIC_CNT];
    volatile const uint32_t res_229_231     [3];
    volatile uint32_t       wic_wake_stat;
    volatile const uint32_t res_233_383     [151];
    volatile uint32_t       irq_disable     [MT3620_IRQ_CFG_IRQ_CNT];
    volatile const uint32_t res_388_391     [4];
    volatile uint32_t       wic_disable     [MT3620_IRQ_WIC_CNT];
    volatile const uint32_t res_396_447     [52];
    volatile uint32_t       irq_test        [MT3620_IRQ_CFG_IRQ_CNT];
    volatile const uint32_t res_452_455     [4];
    volatile uint32_t       irq_test_ctrl;
    volatile const uint32_t res_457_575     [119];
    volatile uint32_t       soft_rst;
} mt3620_irq_t;

static volatile mt3620_irq_t * const mt3620_irq
    = (volatile mt3620_irq_t *)0x21000000;

#define MT3620_IRQ_EINT_INTERRUPT(index) (index + 20)

#define MT3620_IRQ_DBNC_FIELD_READ(reg, field, index) \
    ((mt3620_irq_dbnc_con_t)(mt3620_irq->reg[index])).field

#define MT3620_IRQ_DBNC_FIELD_WRITE(reg, field, index, value) do { \
    mt3620_irq_dbnc_con_t reg = { .mask = mt3620_irq->reg[index] }; \
    reg.field = value; mt3620_irq->reg[index] = reg.mask; } while (0)

#endif // #ifdef MT3620_REG_IRQ_H_
