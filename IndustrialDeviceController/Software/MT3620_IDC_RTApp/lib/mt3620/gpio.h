#ifndef MT3620_REG_GPIO_H_
#define MT3620_REG_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#define MT3620_GPIO_COUNT 76

#define MT3620_PWM_MAX_INDEX 2

#define MT3620_PWM_32k 32768
#define MT3620_PWM_2M  2000000
#define MT3620_PWM_XTAL 26000000
#define MT3620_PWM_CLK_SEL_32K  0
#define MT3620_PWM_CLK_SEL_2M   1
#define MT3620_PWM_CLK_SEL_XTAL 2

#define MT3620_PWM_GLO_CTRL_DEF 0x0000E400
#define MT3620_PWM_CTRL_DEF 0x0000000C
#define MT3620_PWM_PARAM_S0_DEF 0
#define MT3620_PWM_PARAM_S1_DEF 0

typedef enum {
	PWM_CLOCK_SEL_32K  = 0,
	PWM_CLOCK_SEL_2M   = 1,
	PWM_CLOCK_SEL_XTAL = 2,
} PWM_Clock_Select;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           gpio_cnt_mode_en :  1;
        bool           gpio_cr_sw_rst_b :  1;
        bool           pwm_cr_sw_rst_b  :  1;
        bool           gpio_cnt_clk_en  :  1;
        const unsigned res_31_4         : 28;
    };

    uint32_t mask;
} mt3620_gpio_pwm_grp_global_ctrl_t;

// Prototype structs for registers with 4 bit enable, set and resets
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       reg      :  4;
        const unsigned res_31_4 : 28;
    };

    uint32_t mask;
} mt3620_gpio_pwm_reg_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       set      :  4;
        const unsigned res_31_4 : 28;
    };

    uint32_t mask;
} mt3620_gpio_pwm_reg_set_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       reset    :  4;
        const unsigned res_31_4 : 28;
    };

    uint32_t mask;
} mt3620_gpio_pwm_reg_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_din_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_dout_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_dout_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_dout_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_oe_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_oe_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_oe_reset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       gpio_pad_pu      :  4;
        const unsigned gpio_pad_pu_efal :  4;
        const unsigned res_30_8         : 23;
        unsigned       gpio_pad_pu_sw   :  1;
    };

    uint32_t mask;
} mt3620_gpio_pwm_pu_t;

typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_pu_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_pu_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_pd_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_pd_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_pd_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_sr_grp_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_sr_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_sr_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_ies_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_ies_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_ies_reset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_gpio0 :  2;
        unsigned       pad_gpio1 :  2;
        unsigned       pad_gpio2 :  2;
        unsigned       pad_gpio3 :  2;
        const unsigned res_31_8  : 24;
    };

    uint32_t mask;
} mt3620_gpio_pwm_grp_paddrv_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_gpio0 :  2;
        unsigned       pad_gpio1 :  2;
        unsigned       pad_gpio2 :  2;
        unsigned       pad_gpio3 :  2;
        const unsigned res_31_8  : 24;
    };

    uint32_t mask;
} mt3620_gpio_pwm_grp_rdsel_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       pad_gpio0 :  4;
        unsigned       pad_gpio1 :  4;
        unsigned       pad_gpio2 :  4;
        unsigned       pad_gpio3 :  4;
        const unsigned res_31_16 : 16;
    };

    uint32_t mask;
} mt3620_gpio_pwm_grp_tdsel_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_ea_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_ea_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_ea_reset_t;

typedef mt3620_gpio_pwm_reg_t mt3620_gpio_pwm_grp_rsel_t;
typedef mt3620_gpio_pwm_reg_set_t mt3620_gpio_pwm_grp_rsel_set_t;
typedef mt3620_gpio_pwm_reg_reset_t mt3620_gpio_pwm_grp_rsel_reset_t;

//NB: There are three of these registers 4 bytes apart
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       gpio_dglt_min_pulse_wid : 14;
        bool           gpio_dglt_en            :  1;
        unsigned       gpio_dglt_init_val      :  1;
        const unsigned res_31_16               : 16;
    };

    uint32_t mask;
} mt3620_gpio_cnt_static_ctrl_t;

typedef mt3620_gpio_cnt_static_ctrl_t mt3620_gpio_cnt_static_ctrl_0_t;
typedef mt3620_gpio_cnt_static_ctrl_t mt3620_gpio_cnt_static_ctrl_1_t;
typedef mt3620_gpio_cnt_static_ctrl_t mt3620_gpio_cnt_static_ctrl_2_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       gpio_cnt_gpio2_rst_lvl      : 1;
        bool           gpio_cap_cnt_gpio2_rst_en   : 1;
        bool           gpio_event_cnt_gpio2_rst_en : 1;
        const unsigned res_7_3                     : 5;
        unsigned       gpio_0_cap_edge_type        : 2;
        unsigned       gpio_1_cap_edge_type        : 2;
        const unsigned res_15_12                   : 4;
        unsigned       gpio_event_cnt_edge_type    : 2;
        unsigned       gpio_event_cnt_mode         : 2;
        unsigned       gpio_event_cnt_dir_def      : 1;
        unsigned       gpio_event_cnt_updown_def   : 1;
        unsigned       gpio_event_cnt_quad_def     : 1;
        const unsigned res_23                      : 1;
        unsigned       gpio_event_cnt_int_mode     : 2;
        const unsigned res_27_26                   : 2;
        unsigned       gpio_event_cnt_sat_mode     : 3;
        const unsigned res_31                      : 1;
    };

    uint32_t mask;
} mt3620_gpio_cnt_static_ctrl_3_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           gpio_event_cnt_en     :  1;
        bool           gpio_cap_cnt_en       :  1;
        const unsigned res_15_2              : 14;
        bool           gpio_event_cnt_sw_rst :  1;
        bool           gpio_cap_cnt_sw_tst   :  1;
        const unsigned res_31_18             : 14;
    };

    uint32_t mask;
} mt3620_gpio_cnt_dynamic_ctrl0_t;

//TODO: Ask MediaTek for better documentation of these registers
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           gpio_event_cnt_low_lim   : 1;
        bool           gpio_event_cnt_high_lim  : 1;
        bool           gpio_2_rst_assert        : 1;
        const unsigned res_7_3                  : 5;
        bool           gpio_event_cnt_overflow  : 1;
        bool           gpio_event_cnt_underflow : 1;
        const unsigned res_11_10                : 2;
        //The next 4 bits are unknown due to the datasheet being unclear
        bool           unknown_12               : 1;
        bool           unknown_13               : 1;
        bool           unknown_14               : 1;
        bool           unknown_15               : 1;
        bool           capture_fifo0_not_empty  : 1;
        bool           capture_fifo1_not_empty  : 1;
        const unsigned res_23_18                : 6;
        //The next 2 bits are unknown due to the datasheet being unclear
        bool           unknown_24               : 1;
        bool           unknown_25               : 1;
        const unsigned res_31_26                : 6;
    };

    uint32_t mask;
} mt3620_gpio_cnt_int_t;

typedef mt3620_gpio_cnt_int_t mt3620_gpio_cnt_int_en_t;
typedef mt3620_gpio_cnt_int_t mt3620_gpio_cnt_int_sts_clr_t;
typedef mt3620_gpio_cnt_int_t mt3620_gpio_cnt_int_sts_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned gpio_cap_fifo_0_cnt :  4;
        const unsigned gpio_cap_fifo_1_cnt :  4;
        const unsigned res_31_8            : 24;
    };

    uint32_t mask;
} mt3620_gpio_cnt_cap_fifo_sts_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       gpio_cnt_dbg_sel  :  4;
        const unsigned res_6_4           :  3;
        unsigned       gpio_cnt_dbg_swap :  1;
        const unsigned res_31_8          : 24;
    };

    uint32_t mask;
} mt3620_gpio_cnt_dbg_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           global_kick        :  1;
        unsigned       pwm_tick_clock_sel :  2;
        const unsigned res_7_3            :  5;
        unsigned       pwm0_dp_sel        :  2;
        unsigned       pwm1_dp_sel        :  2;
        unsigned       pwm2_dp_sel        :  2;
        unsigned       pwm3_dp_sel        :  2;
        const unsigned res_31_16          : 16;
    };

    uint32_t mask;
} mt3620_pwm_glo_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           kick                   :  1;
        bool           replay_mode            :  1;
        unsigned       polarity               :  1;
        unsigned       pwm_io_ctrl            :  1;
        bool           pwm_clock_en           :  1;
        bool           pwm_global_kick_enable :  1;
        const unsigned res_7_6                :  2;
        unsigned       S0_stay_cycle          : 12;
        unsigned       S1_stay_cycle          : 12;
    };

    uint32_t mask;
} mt3620_pwm_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint16_t       pwm_on_time : 16;
        uint16_t       pwm_off_time : 16;
    };

    uint32_t mask;
} mt3620_pwm_param_s_t;

typedef mt3620_pwm_ctrl_t mt3620_pwm0_ctrl_t;
typedef mt3620_pwm_param_s_t mt3620_pwm0_param_s0_t;
typedef mt3620_pwm_param_s_t mt3620_pwm0_param_s1_t;

typedef mt3620_pwm_ctrl_t mt3620_pwm1_ctrl_t;
typedef mt3620_pwm_param_s_t mt3620_pwm1_param_s0_t;
typedef mt3620_pwm_param_s_t mt3620_pwm1_param_s1_t;

typedef mt3620_pwm_ctrl_t mt3620_pwm2_ctrl_t;
typedef mt3620_pwm_param_s_t mt3620_pwm2_param_s0_t;
typedef mt3620_pwm_param_s_t mt3620_pwm2_param_s1_t;

typedef mt3620_pwm_ctrl_t mt3620_pwm3_ctrl_t;
typedef mt3620_pwm_param_s_t mt3620_pwm3_param_s0_t;
typedef mt3620_pwm_param_s_t mt3620_pwm3_param_s1_t;

typedef struct {
    volatile uint32_t       gpio_pwm_grp_global_ctrl__din_i2s;
    volatile const uint32_t gpio_pwm_grp_din;
    volatile const uint32_t res_2;
    volatile const uint32_t gpio_pwm_grp_din_isu;
    volatile uint32_t       gpio_pwm_grp_dout;
    volatile uint32_t       gpio_pwm_grp_dout_set;
    volatile uint32_t       gpio_pwm_grp_dout_reset;
    volatile const uint32_t res_7;
    volatile uint32_t       gpio_pwm_grp_oe;
    volatile uint32_t       gpio_pwm_grp_oe_set;
    volatile uint32_t       gpio_pwm_grp_oe_reset;
    volatile const uint32_t res_11;
    volatile uint32_t       gpio_pwm_grp_pu;
    volatile uint32_t       gpio_pwm_grp_pu_set;
    volatile uint32_t       gpio_pwm_grp_pu_reset;
    volatile const uint32_t res_15;
    volatile uint32_t       gpio_pwm_grp_pd;
    volatile uint32_t       gpio_pwm_grp_pd_set;
    volatile uint32_t       gpio_pwm_grp_pd_reset;
    volatile const uint32_t res_19;
    volatile uint32_t       gpio_pwm_grp_sr;
    volatile uint32_t       gpio_pwm_grp_sr_set;
    volatile uint32_t       gpio_pwm_grp_sr_reset;
    volatile const uint32_t res_23;
    volatile uint32_t       gpio_pwm_grp_ies;
    volatile uint32_t       gpio_pwm_grp_ies_set;
    volatile uint32_t       gpio_pwm_grp_ies_reset;
    volatile const uint32_t res_27;
    volatile uint32_t       gpio_pwm_grp_paddrv;
    volatile const uint32_t res_29_31[3];
    volatile uint32_t       gpio_pwm_grp_rdsel;
    volatile const uint32_t res_33_35[3];
    volatile uint32_t       gpio_pwm_grp_tdsel;
    volatile const uint32_t res_37_39[3];
    volatile uint32_t       gpio_pwm_grp_ea;
    volatile uint32_t       gpio_pwm_grp_ea_set;
    volatile uint32_t       gpio_pwm_grp_ea_reset;
    volatile const uint32_t res_43;
    volatile uint32_t       gpio_pwm_grp_rsel;
    volatile uint32_t       gpio_pwm_grp_rsel_set;
    volatile uint32_t       gpio_pwm_grp_rsel_reset;
    volatile const uint32_t res_47_63[17];
    volatile uint32_t       gpio_cnt_static_ctrl0;
    volatile uint32_t       gpio_cnt_static_ctrl1;
    volatile uint32_t       gpio_cnt_static_ctrl2;
    volatile uint32_t       gpio_cnt_static_ctrl3;
    volatile uint32_t       gpio_cnt_dynamic_ctrl0;
    volatile const uint32_t res_69_71[3];
    volatile uint32_t       gpio_cnt_rst_val;
    volatile uint32_t       gpio_cnt_low_limit;
    volatile uint32_t       gpio_cnt_high_limit;
    volatile const uint32_t res_75;
    volatile uint32_t       gpio_cnt_int_en;
    volatile uint32_t       gpio_cnt_int_sts_clr;
    volatile const uint32_t gpio_cnt_int_sts;
    volatile const uint32_t res_79;
    volatile const uint32_t gpio_cnt_cap_fifo_sts;
    volatile const uint32_t gpio_cnt_cap_value_0;
    volatile const uint32_t gpio_cnt_cap_value_1;
    volatile const uint32_t gpio_cnt_counter_value;
    volatile uint32_t       gpio_cnt_dbg_ctrl;
    volatile const uint32_t gpio_cnt_dbg_out;
} mt3620_gpio_t;

typedef struct {
    volatile uint32_t       pwm_glo_ctrl;
    volatile const uint32_t res_1_63[63];
    volatile uint32_t       pwm0_ctrl;
    volatile uint32_t       pwm0_param_s0;
    volatile uint32_t       pwm0_param_s1;
    volatile const uint32_t res_67;
    volatile uint32_t       pwm1_ctrl;
    volatile uint32_t       pwm1_param_s0;
    volatile uint32_t       pwm1_param_s1;
    volatile const uint32_t res_71;
    volatile uint32_t       pwm2_ctrl;
    volatile uint32_t       pwm2_param_s0;
    volatile uint32_t       pwm2_param_s1;
    volatile const uint32_t res_75;
    volatile uint32_t       pwm3_ctrl;
    volatile uint32_t       pwm3_param_s0;
    volatile uint32_t       pwm3_param_s1;
    volatile const uint32_t res_79;
} mt3620_pwm_t;

typedef enum {
    MT3620_GPIO_BLOCK_ADC,
    MT3620_GPIO_BLOCK_0,
    MT3620_GPIO_BLOCK_1,
    MT3620_GPIO_BLOCK_2,
    MT3620_GPIO_BLOCK_3,
    MT3620_GPIO_BLOCK_4,
    MT3620_GPIO_BLOCK_5,
    MT3620_GPIO_BLOCK_ISU_0,
    MT3620_GPIO_BLOCK_ISU_1,
    MT3620_GPIO_BLOCK_ISU_2,
    MT3620_GPIO_BLOCK_ISU_3,
    MT3620_GPIO_BLOCK_ISU_4,
    MT3620_GPIO_BLOCK_I2S_0,
    MT3620_GPIO_BLOCK_I2S_1,
    MT3620_GPIO_BLOCK_COUNT,

    MT3620_GPIO_BLOCK_NOT_MAPPED
} mt3620_gpio_block;

static uint32_t mt3620_gpioBlockStart[MT3620_GPIO_BLOCK_COUNT + 1] = {
    [MT3620_GPIO_BLOCK_ADC]   = 41,
    [MT3620_GPIO_BLOCK_0]     =  0,
    [MT3620_GPIO_BLOCK_1]     =  4,
    [MT3620_GPIO_BLOCK_2]     =  8,
    [MT3620_GPIO_BLOCK_3]     = 12,
    [MT3620_GPIO_BLOCK_4]     = 16,
    [MT3620_GPIO_BLOCK_5]     = 20,
    [MT3620_GPIO_BLOCK_ISU_0] = 26,
    [MT3620_GPIO_BLOCK_ISU_1] = 31,
    [MT3620_GPIO_BLOCK_ISU_2] = 36,
    [MT3620_GPIO_BLOCK_ISU_3] = 66,
    [MT3620_GPIO_BLOCK_ISU_4] = 71,
    [MT3620_GPIO_BLOCK_I2S_0] = 56,
    [MT3620_GPIO_BLOCK_I2S_1] = 61,
};

static mt3620_gpio_block mt3620_gpioPinMap[MT3620_GPIO_COUNT] = {
    [ 0] = MT3620_GPIO_BLOCK_0,
    [ 1] = MT3620_GPIO_BLOCK_0,
    [ 2] = MT3620_GPIO_BLOCK_0,
    [ 3] = MT3620_GPIO_BLOCK_0,

    [ 4] = MT3620_GPIO_BLOCK_1,
    [ 5] = MT3620_GPIO_BLOCK_1,
    [ 6] = MT3620_GPIO_BLOCK_1,
    [ 7] = MT3620_GPIO_BLOCK_1,

    [ 8] = MT3620_GPIO_BLOCK_2,
    [ 9] = MT3620_GPIO_BLOCK_2,
    [10] = MT3620_GPIO_BLOCK_2,
    [11] = MT3620_GPIO_BLOCK_2,

    [12] = MT3620_GPIO_BLOCK_3,
    [13] = MT3620_GPIO_BLOCK_3,
    [14] = MT3620_GPIO_BLOCK_3,
    [15] = MT3620_GPIO_BLOCK_3,

    [16] = MT3620_GPIO_BLOCK_4,
    [17] = MT3620_GPIO_BLOCK_4,
    [18] = MT3620_GPIO_BLOCK_4,
    [19] = MT3620_GPIO_BLOCK_4,

    [20] = MT3620_GPIO_BLOCK_5,
    [21] = MT3620_GPIO_BLOCK_5,
    [22] = MT3620_GPIO_BLOCK_5,
    [23] = MT3620_GPIO_BLOCK_5,

    [24] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [25] = MT3620_GPIO_BLOCK_NOT_MAPPED,

    [26] = MT3620_GPIO_BLOCK_ISU_0,
    [27] = MT3620_GPIO_BLOCK_ISU_0,
    [28] = MT3620_GPIO_BLOCK_ISU_0,
    [29] = MT3620_GPIO_BLOCK_ISU_0,
    [30] = MT3620_GPIO_BLOCK_ISU_0,

    [31] = MT3620_GPIO_BLOCK_ISU_1,
    [32] = MT3620_GPIO_BLOCK_ISU_1,
    [33] = MT3620_GPIO_BLOCK_ISU_1,
    [34] = MT3620_GPIO_BLOCK_ISU_1,
    [35] = MT3620_GPIO_BLOCK_ISU_1,

    [36] = MT3620_GPIO_BLOCK_ISU_2,
    [37] = MT3620_GPIO_BLOCK_ISU_2,
    [38] = MT3620_GPIO_BLOCK_ISU_2,
    [39] = MT3620_GPIO_BLOCK_ISU_2,
    [40] = MT3620_GPIO_BLOCK_ISU_2,

    [41] = MT3620_GPIO_BLOCK_ADC,
    [42] = MT3620_GPIO_BLOCK_ADC,
    [43] = MT3620_GPIO_BLOCK_ADC,
    [44] = MT3620_GPIO_BLOCK_ADC,
    [45] = MT3620_GPIO_BLOCK_ADC,
    [46] = MT3620_GPIO_BLOCK_ADC,
    [47] = MT3620_GPIO_BLOCK_ADC,
    [48] = MT3620_GPIO_BLOCK_ADC,

    [49] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [50] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [51] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [52] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [53] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [54] = MT3620_GPIO_BLOCK_NOT_MAPPED,
    [55] = MT3620_GPIO_BLOCK_NOT_MAPPED,

    [56] = MT3620_GPIO_BLOCK_I2S_0,
    [57] = MT3620_GPIO_BLOCK_I2S_0,
    [58] = MT3620_GPIO_BLOCK_I2S_0,
    [59] = MT3620_GPIO_BLOCK_I2S_0,
    [60] = MT3620_GPIO_BLOCK_I2S_0,

    [61] = MT3620_GPIO_BLOCK_I2S_1,
    [62] = MT3620_GPIO_BLOCK_I2S_1,
    [63] = MT3620_GPIO_BLOCK_I2S_1,
    [64] = MT3620_GPIO_BLOCK_I2S_1,
    [65] = MT3620_GPIO_BLOCK_I2S_1,

    [66] = MT3620_GPIO_BLOCK_ISU_3,
    [67] = MT3620_GPIO_BLOCK_ISU_3,
    [68] = MT3620_GPIO_BLOCK_ISU_3,
    [69] = MT3620_GPIO_BLOCK_ISU_3,
    [70] = MT3620_GPIO_BLOCK_ISU_3,

    [71] = MT3620_GPIO_BLOCK_ISU_4,
    [72] = MT3620_GPIO_BLOCK_ISU_4,
    [73] = MT3620_GPIO_BLOCK_ISU_4,
    [74] = MT3620_GPIO_BLOCK_ISU_4,
    [75] = MT3620_GPIO_BLOCK_ISU_4,
};

static volatile mt3620_gpio_t * const mt3620_gpio[MT3620_GPIO_BLOCK_COUNT] = {
    [MT3620_GPIO_BLOCK_ADC]   = (volatile mt3620_gpio_t *)0x38000000,
    [MT3620_GPIO_BLOCK_0]     = (volatile mt3620_gpio_t *)0x38010000,
    [MT3620_GPIO_BLOCK_1]     = (volatile mt3620_gpio_t *)0x38020000,
    [MT3620_GPIO_BLOCK_2]     = (volatile mt3620_gpio_t *)0x38030000,
    [MT3620_GPIO_BLOCK_3]     = (volatile mt3620_gpio_t *)0x38040000,
    [MT3620_GPIO_BLOCK_4]     = (volatile mt3620_gpio_t *)0x38050000,
    [MT3620_GPIO_BLOCK_5]     = (volatile mt3620_gpio_t *)0x38060000,
    [MT3620_GPIO_BLOCK_ISU_0] = (volatile mt3620_gpio_t *)0x38070000,
    [MT3620_GPIO_BLOCK_ISU_1] = (volatile mt3620_gpio_t *)0x38080000,
    [MT3620_GPIO_BLOCK_ISU_2] = (volatile mt3620_gpio_t *)0x38090000,
    [MT3620_GPIO_BLOCK_ISU_3] = (volatile mt3620_gpio_t *)0x380a0000,
    [MT3620_GPIO_BLOCK_ISU_4] = (volatile mt3620_gpio_t *)0x380b0000,
    [MT3620_GPIO_BLOCK_I2S_0] = (volatile mt3620_gpio_t *)0x380d0100,
    [MT3620_GPIO_BLOCK_I2S_1] = (volatile mt3620_gpio_t *)0x380e0100,
};  

#define MT3620_PWM_BLOCK_COUNT 3
static volatile mt3620_pwm_t * const mt3620_pwm[MT3620_PWM_BLOCK_COUNT] = {
    (volatile mt3620_pwm_t *)0x38011000,
    (volatile mt3620_pwm_t *)0x38021000,
    (volatile mt3620_pwm_t *)0x38031000,
};

#define MT3620_GPIO_FIELD_READ(index, reg, field) \
    ((mt3620_##reg##_t)mt3620_gpio[index]->reg).field
#define MT3620_GPIO_FIELD_WRITE(index, reg, field, value) \
    do { mt3620_##reg##_t reg = { .mask = mt3620_gpio[index]->reg }; \
    reg.field = value; mt3620_gpio[index]->reg = reg.mask; } while (0)

#define MT3620_PWM_FIELD_READ(index, reg, field) \
    ((mt3620_##reg##_t)mt3620_pwm[index]->reg).field
#define MT3620_PWM_FIELD_WRITE(index, reg, field, value) \
    do { mt3620_##reg##_t reg = { .mask = mt3620_pwm[index]->reg }; \
    reg.field = value; mt3620_pwm[index]->reg = reg.mask; } while (0)

#endif //#ifndef MT3620_REG_GPIO_H_
