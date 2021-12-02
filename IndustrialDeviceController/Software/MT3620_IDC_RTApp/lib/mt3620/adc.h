#ifndef MT3620_REG_ADC_H_
#define MT3620_REG_ADC_H_

#include <stdbool.h>
#include <stdint.h>

#define MT3620_ADC_INTERRUPT (70)

#define MT3620_ADC_DMA_CHANNEL 29

/* Interrupt types */
#define NO_INTERRUPT_PENDING 1
#define INTERRUPT_CLEAR 2
#define RX_DATA_RECEIVED 4
#define RX_DATA_TIMEOUT 12

/* Register defaults */
#define ADC_FIFO_IER_DEF 0x00000000
#define ADC_FIFO_TRI_LVL_DEF 0x0000001C

//ADC DMA channel number
#define MT3620_ADC_DMA_CHANNEL 29

#define MT3620_ADC_COUNT 1

//V_ref ranges for RG_AUXADC[31] (vcm_azure_en)
#define ADC_VREF_1V8_MAX 1980
#define ADC_VREF_1V8_MIN 1620
#define ADC_VREF_2V5_MAX 2750
#define ADC_VREF_2V5_MIN 2250

/* RG_AUXADC[31] (vcm_azure_en) default setting in the range between ADC_VREF_1V8_MAX */
/* and ADC_VREF_2V5_MIN. */
#define ADC_VCM_AZURE_EN_DEF 1

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool     adc_cr_sw_rst_b :  1;
        unsigned rsvoo_b31_b1    : 31;
    };

    uint32_t mask;
} mt3620_adc_global_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool     adc_fsm_en   :  1;
        unsigned reg_avg_mode :  3;
        unsigned reg_t_ch     :  4;
        bool     pmode_en     :  1;
        unsigned reg_t_init   :  7;
        uint16_t reg_ch_map   : 16;
    };

    uint32_t mask;
} mt3620_adc_ctl0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_19_0               : 20;
        unsigned       reg_adc_data_sync_mode :  1;
        bool           reg_adc_timestamp_en   :  1;
        const unsigned res_31_22              : 10;
    };

    uint32_t mask;
} mt3620_adc_ctl2_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       comp_time_delay     :  2;
        unsigned       comp_preamp_current :  2;
        bool           comp_preamp_en      :  1;
        const unsigned res_5               :  1;
        bool           dither_en           :  1;
        const unsigned res_7               :  1;
        unsigned       dither_step_size    :  2;
        const unsigned res_10              :  1;
        unsigned       auxadc_in_mux_en    :  1;
        const unsigned res_12              :  1;
        bool           vcm_gen_en          :  1;
        const unsigned res_14              :  1;
        bool           auxadc_clk_gen_en   :  1;
        unsigned       auxadc_pmu_clk_inv  :  1;
        unsigned       auxadc_clk_src      :  1;
        const unsigned res_30_18           : 13;
        bool           vcm_azure_en        :  1;
    };

    uint32_t mask;
} mt3620_adc_ctl3_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool     rxfen    :  1;
        unsigned res_1    :  1;
        bool     rxten    :  1;
        unsigned res_31_3 : 29;
    };
    uint32_t mask;
} mt3620_adc_fifo_ier_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           rx_dma_en      :  1;
        const unsigned res_1          :  1;
        bool           to_cnt_autorst :  1;
        const unsigned res_31_3       : 29;
    };

    uint32_t mask;
} mt3620_adc_fifo_dma_en_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_2_1    :  2;
        unsigned       rx_tri_lvl :  5;
        bool           adc_loop   :  1;
        const unsigned res_31_8   : 24;
    };

    uint32_t mask;
} mt3620_adc_fifo_tri_lvl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       wat_time_1 :  3;
        unsigned       wat_time_2 :  3;
        const unsigned res_31_6   : 26;
    };

    uint32_t mask;
} mt3620_adc_fifo_wat_time_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           handshake_en  :  1;
        bool           high_speed_en :  1;
        bool           rto_ext       :  1;
        const unsigned res_31_3      : 29;
    };

    uint32_t mask;
} mt3620_adc_fifo_handshake_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned read_ptr  :  5;
        const unsigned write_ptr :  5;
        const unsigned res_31_10 : 22;
    };

    uint32_t mask;
} mt3620_adc_fifo_debug16_t;


typedef struct {
    // NB: GPIO registers are mostly common for ISU, ADC, GPIO and I2S
    //     so, users should use the GPIO API.
    volatile uint32_t       adc_global_ctrl;
    volatile const uint32_t res_1_63[63];
    volatile uint32_t       adc_ctl0;
    volatile uint32_t       reg_period;
    volatile uint32_t       adc_ctl2;
    volatile uint32_t       adc_ctl3;
    volatile const uint32_t adc_ctl4;
    volatile uint32_t       res_68_126[59];

    volatile const uint32_t adc_fifo_rbr;
    volatile uint32_t       adc_fifo_ier;
    volatile uint32_t       adc_iir;
    volatile uint32_t       adc_fifo_fakelcr;
    volatile const uint32_t res_131;
    volatile const uint32_t adc_fifo_lsr;
    volatile const uint32_t res_133_144[12];
    volatile uint32_t       adc_fifo_sleep_en;
    volatile uint32_t       adc_fifo_dma_en;
    volatile const uint32_t res_147;
    volatile uint32_t       adc_fifo_rtocnt;
    volatile const uint32_t res_149_150[2];
    volatile uint32_t       adc_fifo_tri_lvl;
    volatile uint32_t       adc_fifo_wak;
    volatile uint32_t       adc_fifo_wat_time;
    volatile uint32_t       adc_fifo_handshake;
    volatile const uint32_t adc_fifo_debug0;
    volatile const uint32_t adc_fifo_debug1;
    volatile const uint32_t adc_fifo_debug2;
    volatile const uint32_t adc_fifo_debug3;
    volatile const uint32_t adc_fifo_debug4;
    volatile const uint32_t adc_fifo_debug5;
    volatile const uint32_t adc_fifo_debug6;
    volatile const uint32_t adc_fifo_debug7;
    volatile const uint32_t adc_fifo_debug8;
    volatile const uint32_t adc_fifo_debug9;
    volatile const uint32_t adc_fifo_debug10;
    volatile const uint32_t adc_fifo_debug11;
    volatile const uint32_t adc_fifo_debug12;
    volatile const uint32_t adc_fifo_debug13;
    volatile const uint32_t adc_fifo_debug14;
    volatile const uint32_t adc_fifo_debug15;
    volatile const uint32_t res_172_180[9];
    volatile const uint32_t adc_fifo_debug16;
} mt3620_adc_t;

static volatile mt3620_adc_t * const mt3620_adc
    = (volatile mt3620_adc_t *)0x38000000;

#define MT3620_ADC_FIELD_READ(reg, field) \
    ((mt3620_##reg##_t)mt3620_adc->reg).field
#define MT3620_ADC_FIELD_WRITE(reg, field, value) \
    do { mt3620_##reg##_t reg = { .mask = mt3620_adc->reg }; \
    reg.field = value; mt3620_adc->reg = reg.mask; } while (0)

#endif
