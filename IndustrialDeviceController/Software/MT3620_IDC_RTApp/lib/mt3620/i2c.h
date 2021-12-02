#ifndef MT3620_REG_I2C_H_
#define MT3620_REG_I2C_H_

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        uint8_t x, y, z, w;
    };

    uint32_t mask;
} mt3620_i2c_vec4_t;

typedef mt3620_i2c_vec4_t mt3620_i2c_mm_cnt_val_phl_t;
typedef mt3620_i2c_vec4_t mt3620_i2c_mm_cnt_val_phh_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           mm_int_sta  :  1;
        bool           mm_int_en   :  1;
        bool           mm_int_msk  :  1;
        const unsigned res_3       :  1;
        bool           s_int_sta   :  1;
        bool           s_int_en    :  1;
        bool           s_int_msk   :  1;
        const unsigned res_31_7    : 25;
    };

    uint32_t mask;
} mt3620_i2c_int_ctrl_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       de_cnt      :  5;
        const unsigned res_6_5     :  2;
        bool           sync_en     :  1;
        const unsigned res_31_8    : 24;
    };

    uint32_t mask;
} mt3620_i2c_mm_pad_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           mm_pack_rw0 :  1;
        bool           mm_pack_rw1 :  1;
        bool           mm_pack_rw2 :  1;
        const unsigned res_3       :  1;
        unsigned       mm_pack_val :  2;
        const unsigned res_31_6    : 26;
    };

    uint32_t mask;
} mt3620_i2c_mm_pack_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned mm_ack_data :  8;
        const unsigned mm_ack_id   :  4;
        const unsigned res_31_12   : 20;
    };

    uint32_t mask;
} mt3620_i2c_mm_ack_val_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           mm_start_en :  1;
        const unsigned res_13_1    : 13;
        bool           mm_gmode    :  1;
        bool           master_en   :  1;
        const unsigned res_31_16   : 16;
    };

    uint32_t mask;
} mt3620_i2c_mm_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     bus_busy        :  1;
        bool           mm_arb_had_lose :  1;
        const bool     mm_start_ready  :  1;
        const unsigned res_31_3        : 29;
    };

    uint32_t mask;
} mt3620_i2c_mm_status_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool           rx_fifo_clr :  1;
        bool           tx_fifo_clr :  1;
        const unsigned res_31_2    : 30;
    };

    uint32_t mask;
} mt3620_i2c_fifo_con0_t;

typedef mt3620_i2c_fifo_con0_t mt3620_i2c_mm_fifo_con0_t;
typedef mt3620_i2c_fifo_con0_t mt3620_i2c_s_fifo_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     rx_fifo_emp  :  1;
        const bool     rx_fifo_full :  1;
        const bool     rx_fifo_undr :  1;
        const bool     rx_fifo_ovf  :  1;
        const bool     tx_fifo_emp  :  1;
        const bool     tx_fifo_full :  1;
        const bool     tx_fifo_undr :  1;
        const bool     tx_fifo_ovf  :  1;
        const unsigned res_31_8     : 24;
    };

    uint32_t mask;
} mt3620_i2c_fifo_status_t;

typedef mt3620_i2c_fifo_status_t mt3620_i2c_mm_fifo_status_t;
typedef mt3620_i2c_fifo_status_t mt3620_i2c_s_fifo_status_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned rx_fifo_rptr :  4;
        const unsigned rx_fifo_wptr :  4;
        const unsigned tx_fifo_rptr :  4;
        const unsigned tx_fifo_wptr :  4;
        const unsigned res_31_16    : 16;
    };

    uint32_t mask;
} mt3620_i2c_fifo_ptr_t;

typedef mt3620_i2c_fifo_ptr_t mt3620_i2c_mm_fifo_ptr_t;
typedef mt3620_i2c_fifo_ptr_t mt3620_i2c_s_fifo_ptr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_3_1         :  4;
        bool           dma_hs_en       :  1;
        unsigned       dma_hs_sel      :  1; /* 0 = Master, 1 = Slave */
        const unsigned res_31_6        : 26;
    };

    uint32_t mask;
} mt3620_i2c_dma_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_10_0    : 11;
        bool           s_ind_en    :  1;
        const unsigned res_13_12   :  2;
        bool           s_mute_mode :  1;
        bool           slave_en    :  1;
        const unsigned res_31_16   : 16;
    };

    uint32_t mask;
} mt3620_i2c_s_con0_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned s_received_id   :  7;
        const bool     s_received_read :  1;
        const unsigned res_31_8        : 24;
    };

    uint32_t mask;
} mt3620_i2c_s_id_receive_t;

typedef mt3620_i2c_s_id_receive_t mt3620_i2c_s_id_receive0_t;
typedef mt3620_i2c_s_id_receive_t mt3620_i2c_s_id_receive1_t;

typedef struct {
    volatile uint32_t       int_ctrl;
    volatile const uint32_t res_1_15[15];
    volatile uint32_t       mm_pad_con0;
    volatile uint32_t       mm_cnt_val_phl;
    volatile uint32_t       mm_cnt_val_phh;
    volatile const uint32_t res_19_20[2];
    volatile uint32_t       mm_cnt_byte_val_pk[3];
    volatile uint32_t       mm_slave_id;
    volatile const uint32_t res_25;
    volatile uint32_t       mm_pack_con0;
    volatile const uint32_t mm_ack_val;
    volatile uint32_t       mm_con0;
    volatile uint32_t       mm_status;
    volatile uint32_t       mm_fifo_con0;
    volatile const uint32_t res_31;
    volatile const uint32_t mm_fifo_status;
    volatile const uint32_t mm_fifo_ptr;
    volatile const uint32_t res_34_35[2];
    volatile uint32_t       mm_fifo_data;
    volatile const uint32_t res_37_47[11];
    volatile uint32_t       dma_con0;
    volatile uint32_t       s_con0;
    volatile uint32_t       s_slave_id;
    volatile const uint32_t s_id_receive0;
    volatile const uint32_t s_id_receive1;
    volatile const uint32_t res_53;
    volatile uint32_t       s_fifo_con0;
    volatile const uint32_t res_55;
    volatile const uint32_t s_fifo_status;
    volatile const uint32_t s_fifo_ptr;
    volatile const uint32_t res_58_59[2];
    volatile uint32_t       s_fifo_data;
} mt3620_i2c_t;

#define MT3620_I2C_QUEUE_DEPTH 3
#define MT3620_I2C_TX_FIFO_DEPTH 8
#define MT3620_I2C_RX_FIFO_DEPTH 8
#define MT3620_I2C_PACKET_SIZE_MAX 65535

#define MT3620_I2C_CLOCK     26000000
#define MT3620_I2C_MAX_SPEED  1000000

#define MT3620_I2C_INTERRUPT(x) (44 + ((x) * 4))

#define MT3620_I2C_DMA_TX(x) (0 + ((x) * 2))
#define MT3620_I2C_DMA_RX(x) (1 + ((x) * 2))

#define MT3620_I2C_COUNT 6
static volatile mt3620_i2c_t * const mt3620_i2c[MT3620_I2C_COUNT] = {
    (volatile mt3620_i2c_t *)0x38070200,
    (volatile mt3620_i2c_t *)0x38080200,
    (volatile mt3620_i2c_t *)0x38090200,
    (volatile mt3620_i2c_t *)0x380a0200,
    (volatile mt3620_i2c_t *)0x380b0200,
    (volatile mt3620_i2c_t *)0x380c0200,
};

#define MT3620_I2C_FIELD_READ(index, reg, field) ((mt3620_i2c_##reg##_t)mt3620_i2c[index]->reg).field
#define MT3620_I2C_FIELD_WRITE(index, reg, field, value) do { mt3620_i2c_##reg##_t reg = { .mask = mt3620_i2c[index]->reg }; reg.field = value; mt3620_i2c[index]->reg = reg.mask; } while (0)

#endif // #ifdef MT3620_REG_I2C_H_
