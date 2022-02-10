#ifndef MT3620_REG_UART_H__
#define MT3620_REG_UART_H__

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const uint8_t rbr;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_rbr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       thr;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_thr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           erbfi    :  1;
        bool           etbei    :  1;
        bool           elsi     :  1;
        bool           edssi    :  1;
        const unsigned res_4    :  1;
        bool           xoffi    :  1;
        bool           rtsi     :  1;
        bool           ctsi     :  1;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_ier_t;

typedef enum
{
    MT3620_UART_IIR_ID_MODEM_STATUS_CHANGE       = 0x00,
    MT3620_UART_IIR_ID_NO_INTERRUPT_PENDING      = 0x01,
    MT3620_UART_IIR_ID_TX_HOLDING_REGISTER_EMPTY = 0x02,
    MT3620_UART_IIR_ID_RX_DATA_RECEIVED          = 0x04,
    MT3620_UART_IIR_ID_LINE_STATUS               = 0x06,
    MT3620_UART_IIR_ID_RX_DATA_TIMEOUT           = 0x0C,
    MT3620_UART_IIR_ID_SOFTWARE_FLOW_CONTROL     = 0x10,
    MT3620_UART_IIR_ID_HARDWARE_FLOW_CONTROL     = 0x20,
} mt3620_uart_iir_id_e;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const mt3620_uart_iir_id_e iir_id   :  6;
        const unsigned             fifoe    :  2;
        const unsigned             res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_iir_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           fifoe    :  1;
        bool           clrr     :  1;
        bool           clrt     :  1;
        const unsigned res_3    :  1;
        unsigned       tftl     :  2;
        unsigned       rftl     :  2;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_fcr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       wls      :  2;
        bool           stb      :  1;
        bool           pen      :  1;
        bool           eps      :  1;
        bool           sp       :  1;
        bool           sb       :  1;
        bool           dlab     :  1;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_lcr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           dtr         :  1;
        bool           rts         :  1;
        bool           out1        :  1;
        bool           out2        :  1;
        bool           loop        :  1;
        const unsigned res_6_5     :  2;
        const bool     xoff_status :  1;
        const unsigned res_31_8    : 24;
    };

    uint32_t mask;
} mt3620_uart_mcr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     dr       :  1;
        const bool     oe       :  1;
        const bool     pe       :  1;
        const bool     fe       :  1;
        const bool     bi       :  1;
        const bool     thre     :  1;
        const bool     temt     :  1;
        const bool     fifoerr  :  1;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_lsr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           dctr     :  1;
        bool           ddsr     :  1;
        bool           teri     :  1;
        bool           ddcd     :  1;
        const bool     cts      :  1;
        const bool     dsr      :  1;
        const bool     ri       :  1;
        const bool     dcd      :  1;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_msr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       scr;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_scr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       dll;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_dll_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       dlm;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_dlm_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       sw_flow_cont :  4;
        bool           enable_e     :  1;
        const unsigned res_5        :  1;
        bool           auto_rts     :  1;
        bool           auto_cts     :  1;
        const unsigned res_31_8     : 24;
    };

    uint32_t mask;
} mt3620_uart_efr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       xon1;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_xon1_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       xon2;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_xon2_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       xoff1;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_xoff1_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       xoff2;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_xoff2_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_5_0         :  6;
        bool           ignore_ack      :  1;
        bool           rx_level_ctl_en :  1;
        const unsigned res_31_8        : 24;
    };

    uint32_t mask;
} mt3620_uart_rx_level_ctl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       speed    :  2;
        const unsigned res_31_2 : 30;
    };

    uint32_t mask;
} mt3620_uart_highspeed_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       sample_count;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_sample_count_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       sample_point;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_sample_point_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       guard_cnt :  4;
        bool           guard_en  :  1;
        const unsigned res_31_2  : 27;
    };

    uint32_t mask;
} mt3620_uart_guard_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       escape_dat;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_escape_dat_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           esc_en   :  1;
        const unsigned res_31_1 : 31;
    };

    uint32_t mask;
} mt3620_uart_escape_en_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_0           :  1;
        bool           sw_rto_limit_en :  1;
        unsigned       sw_rto_limit    :  6;
        const unsigned res_31_1        : 24;
    };

    uint32_t mask;
} mt3620_uart_swo_rto_limit_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           vfifo_en :  1;
        const unsigned res_31_1 : 31;
    };

    uint32_t mask;
} mt3620_uart_vfifo_en_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       rxtrig   :  6;
        const unsigned res_31_6 : 26;
    };

    uint32_t mask;
} mt3620_uart_rxtrig_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t       fracdiv_l;
        const uint8_t res_15_8;
        const uint8_t res_23_16;
        const uint8_t res_31_24;
    };

    uint32_t mask;
} mt3620_uart_fracdiv_l_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       fracdiv_m :  2;
        const unsigned res_31_2  : 30;
    };

    uint32_t mask;
} mt3620_uart_fracdiv_m_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     fifoe    :  1;
        const bool     clrr     :  1;
        const bool     clrt     :  1;
        const unsigned res_3    :  1;
        const unsigned tftl     :  2;
        const unsigned rftl     :  2;
        const unsigned res_31_8 : 24;
    };

    uint32_t mask;
} mt3620_uart_fcr_rd_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           rx_dma_hsk_en :  1;
        bool           tx_dma_hsk_en :  1;
        bool           tx_auto_trans :  1;
        const unsigned res_31_3      : 29;
    };

    uint32_t mask;
} mt3620_uart_extend_add_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned rx_offset :  6;
        const unsigned res_31_6  : 26;
    };

    uint32_t mask;
} mt3620_uart_rx_offset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned tx_offset :  5;
        const unsigned res_31_5  : 27;
    };

    uint32_t mask;
} mt3620_uart_tx_offset_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     prior1       :  1;
        const bool     prior2to     :  1;
        const bool     prior2       :  1;
        const bool     prior3       :  1;
        const bool     prior4       :  1;
        const bool     prior5       :  1;
        const bool     prior6       :  1;
        const bool     hwfifo_limit :  1;
        const unsigned res_31_8     : 24;
    };

    uint32_t mask;
} mt3620_uart_interupt_sta_t;


typedef struct {
    union {
        const volatile uint32_t rbr;
        volatile uint32_t       thr;
        volatile uint32_t       dll;
    };

    union {
        volatile uint32_t ier;
        volatile uint32_t dlm;
    };

    union {
        volatile const uint32_t iir;
        volatile uint32_t       fcr;
        volatile uint32_t       efr;
    };

    volatile uint32_t lcr;

    union {
        volatile uint32_t mcr;
        volatile uint32_t xon1;
    };

    union {
        const volatile uint32_t lsr;
        volatile uint32_t       xon2;
    };

    union {
        volatile uint32_t msr;
        volatile uint32_t xoff1;
    };

    union {
        volatile uint32_t scr;
        volatile uint32_t xoff2;
    };

    volatile uint32_t       rx_level_ctl;
    volatile uint32_t       highspeed;
    volatile uint32_t       sample_count;
    volatile uint32_t       sample_point;
    const volatile uint32_t res_12_14[3];
    volatile uint32_t       guard;
    volatile uint32_t       escape_dat;
    volatile uint32_t       escape_en;
    volatile uint32_t       sw_rto_limit;
    volatile uint32_t       vfifo_en;
    volatile uint32_t       rxtrig;
    volatile uint32_t       fracdiv_l;
    volatile uint32_t       fracdiv_m;
    const volatile uint32_t fcr_rd;
    const volatile uint32_t res_24;
    volatile uint32_t       extend_add;
    const volatile uint32_t rx_offset;
    const volatile uint32_t tx_offset;
    const volatile uint32_t interrupt_sta;
} mt3620_uart_t;


#define MT3620_UART_TX_FIFO_DEPTH 16
#define MT3620_UART_RX_FIFO_DEPTH 16

#define MT3620_UART_CLOCK     26000000
#define MT3620_UART_MAX_SPEED  3000000

#define MT3620_UART_INTERRUPT(x) ((x) == 0 ? 4 : (47 + (((x) - 1) * 4)))

#define MT3620_UART_DMA_TX(x) ((x) == 0 ? 0 : (13 + (((x) - 1) * 2)))
#define MT3620_UART_DMA_RX(x) ((x) == 0 ? 0 : (14 + (((x) - 1) * 2)))

#define MT3620_UART_COUNT 7
static volatile mt3620_uart_t * const mt3620_uart[MT3620_UART_COUNT] = {
    (volatile mt3620_uart_t *)0x21040000,
    (volatile mt3620_uart_t *)0x38070500,
    (volatile mt3620_uart_t *)0x38080500,
    (volatile mt3620_uart_t *)0x38090500,
    (volatile mt3620_uart_t *)0x380a0500,
    (volatile mt3620_uart_t *)0x380b0500,
    (volatile mt3620_uart_t *)0x380c0500,
};

#define MT3620_UART_FIELD_READ(index, reg, field) \
    ((mt3620_uart_##reg##_t)mt3620_uart[index]->reg).field
#define MT3620_UART_FIELD_WRITE(index, reg, field, value) \
    do { mt3620_uart_##reg##_t reg = { .mask = mt3620_uart[index]->reg }; reg.field = value; \
    mt3620_uart[index]->reg = reg.mask; } while (0)

static const uint16_t mt3620_uart_fract_lut[10] = {
    0b0000000000,
    0b0000000001,
    0b0000100001,
    0b0001001001,
    0b0010100101,
    0b0101010101,
    0b0110110111,
    0b0111101111,
    0b0111111111,
};

#endif // #ifndef MT3620_REG_UART_H__
