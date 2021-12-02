#ifndef MT3620_REG_I2S_H__
#define MT3620_REG_I2S_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MT3620_I2S_CLK_SEL_XPLL_16M = 0,
    MT3620_I2S_CLK_SEL_XPLL_26M = 1,
    MT3620_I2S_CLK_SEL_XTAL_26M = 2,
    MT3620_I2S_CLK_SEL_EXTERNAL = 3,
} mt3620_i2s_clk_sel_e;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool                 en                     : 1;
        bool                 dlfifo_en              : 1;
        bool                 ulfifo_en              : 1;
        bool                 engen_en               : 1;
        bool                 ext_io_ck              : 1;
        bool                 ext                    : 1;
        bool                 ext_lrsw               : 1;
        bool                 dl_lrsw                : 1;
        bool                 dl_mono                : 1;
        bool                 dl_mono_dup            : 1;
        bool                 mclk_output_en         : 1;
        bool                 i2s_in_clk_en          : 1;
        bool                 i2s_out_clk_en         : 1;
        const unsigned       res_13_12              : 2;
        const unsigned       global_control_rsv_b15 : 1;
        const unsigned       res_14                 : 1;
        const unsigned       global_control_rsv_b17 : 1;
        bool                 x26m_sel               : 1;
        bool                 ext_bclk_inv           : 1;
        bool                 neg_cap                : 1;
        const unsigned       global_control_rsv_b21 : 1;
        bool                 dl_empty_value_en      : 1;
        bool                 ul_empty_value_en      : 1;
        mt3620_i2s_clk_sel_e clk_sel_in             : 2;
        mt3620_i2s_clk_sel_e clk_sel_out            : 2;
        mt3620_i2s_clk_sel_e ext_mclk_sel           : 2;
        const unsigned       global_control_rsv_b30 : 1;
        bool                 loopback               : 1;
    };

    uint32_t mask;
} mt3620_i2s_global_control_t;


typedef enum {
    MT3620_I2S_WLEN_16BIT = 0,
} mt3620_i2s_wlen_t;

typedef enum {
    MT3620_I2S_SRC_MASTER = 0,
    MT3620_I2S_SRC_SLAVE  = 1,
} mt3620_i2s_src_t;

typedef enum {
    MT3620_I2S_FMT_TDM = 0,
    MT3620_I2S_FMT_I2S = 1,
} mt3620_i2s_fmt_t;

// The calculation for SR seems to be based on two, 2-bit fields
// The fields look like yyxx
// The sample rate is: ((x + 4) << (y + 1)) * 1000
#define MT3620_I2S_SR_CALC(x) (((((x) & 3) + 4) << (((x) >> 2) + 1)) * 1000U)
#define MT3620_I2S_SR_IS_OFFICIAL(x) ((((x) & 1) == 0) && (((x) >> 2) < 3))

typedef enum {
    MT3620_I2S_SR_8K  = 0x0,
    MT3620_I2S_SR_12K = 0x2,
    MT3620_I2S_SR_16K = 0x4,
    MT3620_I2S_SR_24K = 0x6,
    MT3620_I2S_SR_32K = 0x8,
    MT3620_I2S_SR_48K = 0xA,
} mt3620_i2s_sr_t;

#define MT3620_I2S_SR_COUNT 16

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool              en                    : 1;
        mt3620_i2s_wlen_t wlen                  : 1;
        mt3620_i2s_src_t  src                   : 1;
        mt3620_i2s_fmt_t  fmt                   : 1;
        const unsigned    dl_control_rsv_b4     : 1;
        bool              wsinv                 : 1;
        const unsigned    dl_control_rsv_b6     : 1;
        bool              dlfifo_2deq           : 1;
        mt3620_i2s_sr_t   sr                    : 4;
        const unsigned    dl_control_rsv_b12    : 1;
        unsigned          bit_per_s             : 2;
        bool              ws_rsync              : 1;
        const unsigned    dl_control_rsv_b16    : 1;
        unsigned          msb_offset            : 7;
        const unsigned    dl_control_rsv_b28_24 : 5;
        unsigned          ch_per_s              : 2;
        const unsigned    dl_control_rsv_b31    : 1;
    };

    uint32_t mask;
} mt3620_i2s_dl_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool              en                   : 1;
        mt3620_i2s_wlen_t wlen                 : 1;
        mt3620_i2s_src_t  src                  : 1;
        mt3620_i2s_fmt_t  fmt                  : 1;
        const unsigned    ul_control_rsv_b4    : 1;
        bool              wsinv                : 1;
        const unsigned    ul_control_rsv_b7_b6 : 2;
        unsigned          sr                   : 4;
        const unsigned    ul_control_rsv_b12   : 1;
        unsigned          bit_per_s            : 2;
        bool              ws_rsync             : 1;
        bool              down_rate            : 1;
        unsigned          msb_offset           : 7;
        unsigned          update_word          : 5;
        unsigned          ch_per_s             : 2;
        bool              lr_swap              : 1;
    };

    uint32_t mask;
} mt3620_i2s_ul_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           soft_rst     :  1;
        bool           glb_soft_rst :  1;
        const unsigned res_3_2      :  2;
        bool           dl_soft_rst  :  1;
        const unsigned res_7_5      :  3;
        bool           ul_soft_rst  :  1;
        const unsigned res_31_9     : 23;
    };

    uint32_t mask;
} mt3620_i2s_soft_reset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     afull     : 1;
        const bool     full      : 1;
        bool           clear     : 1;
        const unsigned res_7_3   : 5;
        const unsigned fifo_cnt  : 8;
        unsigned       thr       : 8;
        const bool     ready     : 1;
        const unsigned res_31_25 : 7;
    };

    uint32_t mask;
} mt3620_i2s_fifo_w_control_t;

typedef mt3620_i2s_fifo_w_control_t mt3620_i2s_dl_fifo_w_control_t;
typedef mt3620_i2s_fifo_w_control_t mt3620_i2s_ul_fifo_w_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     aempty    : 1;
        const bool     empty     : 1;
        bool           clear     : 1;
        const unsigned res_7_3   : 5;
        const unsigned fifo_cnt  : 8;
        unsigned       thr       : 8;
        const bool     ready     : 1;
        const unsigned res_31_25 : 7;
    };

    uint32_t mask;
} mt3620_i2s_fifo_r_control_t;

typedef mt3620_i2s_fifo_r_control_t mt3620_i2s_dl_fifo_r_control_t;
typedef mt3620_i2s_fifo_r_control_t mt3620_i2s_ul_fifo_r_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned  dl_empt_value_r : 16;
        unsigned  dl_empt_value_l : 16;
    };

    uint32_t mask;
} mt3620_i2s_dl_empty_value_lr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned  dl_empt_value_r1 : 16;
        unsigned  dl_empt_value_l1 : 16;
    };

    uint32_t mask;
} mt3620_i2s_dl_empty_value_l1r1_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       dbg_sel     :  4;
        const unsigned res_5_4     :  2;
        bool           dbg_swap    :  1;
        bool           dbg_sel_src :  1;
        unsigned       res_31_8    : 24;
    };

    uint32_t mask;
} mt3620_i2s_debug_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       dl_dmareq_mi_num :  2;
        const unsigned res_3_2          :  2;
        bool           dl_ahb_early_en  :  1;
        const unsigned res_14_5         : 10;
        bool           dl_dma_mode_sel  :  1;
        unsigned       ul_dmareq_mi_num :  2;
        const unsigned res_19_18        :  2;
        bool           ul_ahb_early_en  :  1;
        const unsigned res_30_21        : 10;
        bool           ul_dma_mode_sel  :  1;
    };

    uint32_t mask;
} mt3620_i2s_dma_if_control_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           dl_wfifo_full :  1;
        bool           dl_rfifo_empt :  1;
        const unsigned res_3_2       :  2;
        bool           dl_fifo_wrdy  :  1;
        bool           dl_fifo_rrdy  :  1;
        const unsigned res_7_6       :  2;
        bool           ul_wfifo_full :  1;
        bool           ul_rfifo_empt :  1;
        const unsigned res_11_10     :  2;
        bool           ul_fifo_wrdy  :  1;
        bool           ul_fifo_rrdy  :  1;
        const unsigned res_15_14     :  2;
        bool           dl_mi_ovf     :  1;
        bool           dl_mi_undr    :  1;
        bool           ul_mi_ovf     :  1;
        bool           ul_mi_undr    :  1;
        const unsigned res_31_20     : 12;
    };

    uint32_t mask;
} mt3620_i2s_global_int_t;

typedef mt3620_i2s_global_int_t mt3620_i2s_global_int_en_t;
typedef mt3620_i2s_global_int_t mt3620_i2s_global_int_sts_clr_t;
typedef mt3620_i2s_global_int_t mt3620_i2s_global_int_sts_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           pad_i2s_tx   :  1;
        bool           pad_i2s_mclk :  1;
        bool           pad_i2s_fs   :  1;
        bool           pad_i2s_rx   :  1;
        bool           pad_i2s_bclk :  1;
        const unsigned res_31_5     : 27;
    };

    uint32_t mask;
} mt3620_i2s_gpio_t;

typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_din_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_dout_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_dout_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_dout_reset_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_oe_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_oe_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_oe_reset_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pu_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pu_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pu_reset_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pd_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pd_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_pd_reset_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_sr_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_sr_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_sr_reset_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_ies_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_ies_set_t;
typedef mt3620_i2s_gpio_t mt3620_i2s_gpio_ies_reset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_i2s_tx   :  2;
        unsigned       pad_i2s_mclk :  2;
        unsigned       pad_i2s_fs   :  2;
        unsigned       pad_i2s_rx   :  2;
        unsigned       pad_i2s_bclk :  2;
        const unsigned res_31_10    : 22;
    };

    uint32_t mask;
} mt3620_i2s_gpio_paddrv_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_i2s_tx   :  2;
        unsigned       pad_i2s_mclk :  2;
        unsigned       pad_i2s_fs   :  2;
        unsigned       pad_i2s_rx   :  2;
        unsigned       pad_i2s_bclk :  2;
        const unsigned res_31_10    : 22;
    };

    uint32_t mask;
} mt3620_i2s_gpio_rdsel_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_i2s_tx   :  4;
        unsigned       pad_i2s_mclk :  4;
        unsigned       pad_i2s_fs   :  4;
        unsigned       pad_i2s_rx   :  4;
        unsigned       pad_i2s_bclk :  4;
        const unsigned res_31_20    : 12;
    };

    uint32_t mask;
} mt3620_i2s_gpio_tdsel_t;

typedef struct {
    volatile uint32_t       global_control;
    volatile uint32_t       dl_control;
    volatile uint32_t       ul_control;
    volatile uint32_t       soft_reset;
    volatile uint32_t       dl_fifo_w_control;
    volatile uint32_t       dl_fifo_r_control;
    volatile uint32_t       ul_fifo_w_control;
    volatile uint32_t       ul_fifo_r_control;
    volatile uint32_t       dl_empty_value_lr;
    volatile uint32_t       dl_empty_value_l1r1;
    volatile uint32_t       debug_control;
    volatile const uint32_t debug_probe;
    volatile uint32_t       dma_if_control;
    volatile const uint32_t res_16_14[3];
    volatile uint32_t       global_int_en;
    volatile uint32_t       global_int_sts_clr;
    volatile uint32_t       global_int_sts;
} mt3620_i2s_t;

#define MT3620_I2S_INTERRUPT(x) (68 + (x))

#define MT3620_I2S_DMA_TX(x) (25 + ((x) * 2))
#define MT3620_I2S_DMA_RX(x) (26 + ((x) * 2))

#define MT3620_I2S_STREAM_COUNT 2

#define MT3620_I2S_COUNT 2
static volatile mt3620_i2s_t * const mt3620_i2s[MT3620_I2S_COUNT] = {
    (volatile mt3620_i2s_t *)0x380d0000,
    (volatile mt3620_i2s_t *)0x380e0000,
};

static volatile uint32_t * const mt3620_i2s_fifo[MT3620_I2S_COUNT] = {
    (volatile uint32_t *)0x380f0000,
    (volatile uint32_t *)0x38100000,
};

#define MT3620_I2S_FIELD_READ(index, reg, field) \
        ((mt3620_i2s_##reg##_t)mt3620_i2s[index]->reg).field
#define MT3620_I2S_FIELD_WRITE(index, reg, field, value) \
        do { mt3620_i2s_##reg##_t reg = { .mask = mt3620_i2s[index]->reg }; reg.field = value; \
        mt3620_i2s[index]->reg = reg.mask; } while (0)

#endif
