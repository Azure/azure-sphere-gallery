#ifndef MT3620_REG_SPI_H__
#define MT3620_REG_SPI_H__

#include <stdbool.h>
#include <stdint.h>

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       mosi_byte_cnt    : 4;
        unsigned       miso_byte_cnt    : 4;
        bool           spi_master_start : 1;
        const unsigned res_15_9         : 7;
        const bool     spi_master_busy  : 1;
        const unsigned res_18_17        : 2;
        unsigned       spi_addr_size    : 2;
        const unsigned res_23_21        : 3;
        unsigned       spi_addr_ext     : 8;
    };

    uint32_t mask;
} mt3620_spi_stcsr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned spi_opcode :  8;
        unsigned spi_addr   : 24;
    };

    uint32_t mask;
} mt3620_spi_soar_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const unsigned res_1_0                    :  2;
        bool           more_buf_mode              :  1;
        bool           lsb_first                  :  1;
        bool           cpol                       :  1;
        bool           cpha                       :  1;
        const unsigned res_9_6                    :  3;
        bool           int_en                     :  1;
        bool           both_directional_data_mode :  1;
        const unsigned res_15_11                  :  5;
        unsigned       rs_clk_sel                 : 12;
        bool           clk_mode                   :  1;
        unsigned       rs_slave_sel               :  3;
    };

    uint32_t mask;
} mt3620_spi_smmr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       mosi_bit_cnt : 9;
        const unsigned res_11_9     : 3;
        unsigned       miso_bit_cnt : 9;
        const unsigned res_23_21    : 3;
        unsigned       cmd_bit_cnt  : 6;
        const unsigned res_31_30    : 2;
    };

    uint32_t mask;
} mt3620_spi_smbcr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        const bool     spi_ok       :  1;
        const bool     spi_write_ok :  1;
        const bool     spi_read_ok  :  1;
        const unsigned res_31_3     : 29;
    };

    uint32_t mask;
} mt3620_spi_scsr_t;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned       cs_polar      :  8;
        bool           dma_mode      :  1;
        const unsigned res_11_9      :  3;
        unsigned       cmd_delay_sel :  4;
        unsigned       end_delay_sel :  4;
        const unsigned res_31_20     : 12;
    };

    uint32_t mask;
} mt3620_spi_cspol_t;


typedef struct __attribute__((__packed__)) {
    mt3620_spi_soar_t  soar;
    uint32_t           sdor[8];
    mt3620_spi_smmr_t  smmr;
    mt3620_spi_smbcr_t smbcr;
    mt3620_spi_stcsr_t stcsr;
    // These 4 bytes of dummy data satisfy mt3620 errata 5.1.
    uint32_t           dummy;
} mt3620_spi_dma_cfg_t;

typedef struct {
    volatile uint32_t       stcsr;
    volatile uint32_t       soar;
    volatile uint32_t       sdor[8];
    volatile uint32_t       smmr;
    volatile uint32_t       smbcr;
    volatile const uint32_t rsv;
    volatile uint32_t       scsr;
    volatile uint32_t       cspol;
    volatile const uint32_t res_15;
    volatile uint32_t       dataport;
    volatile const uint32_t res_17;
    volatile const uint32_t sdir[8];
} mt3620_spi_t;

// Datasheet claims that half-duplex supports 32-byte read/write but it doesn't.
#define MT3620_SPI_BUFFER_SIZE_HALF_DUPLEX 16
#define MT3620_SPI_BUFFER_SIZE_FULL_DUPLEX 16
#define MT3620_SPI_OPCODE_SIZE_FULL_DUPLEX 4

#define MT3620_SPI_INTERRUPT(x) (45 + ((x) * 4))

#define MT3620_SPI_DMA_TX(x) (0 + ((x) * 2))
#define MT3620_SPI_DMA_RX(x) (1 + ((x) * 2))

#define MT3620_SPI_COUNT 6
static volatile mt3620_spi_t * const mt3620_spi[MT3620_SPI_COUNT] = {
    (volatile mt3620_spi_t *)0x38070300,
    (volatile mt3620_spi_t *)0x38080300,
    (volatile mt3620_spi_t *)0x38090300,
    (volatile mt3620_spi_t *)0x380a0300,
    (volatile mt3620_spi_t *)0x380b0300,
    (volatile mt3620_spi_t *)0x380c0300,
};

#define MT3620_SPI_FIELD_READ(index, reg, field) \
    ((mt3620_spi_##reg##_t)mt3620_spi[index]->reg).field
#define MT3620_SPI_FIELD_WRITE(index, reg, field, value) \
    do { mt3620_spi_##reg##_t reg = { .mask = mt3620_spi[index]->reg }; reg.field = value; \
    mt3620_spi[index]->reg = reg.mask; } while (0)

// Datasheet says this is 80MHz but it's actually 79.04MHz.
#define MT3620_SPI_HCLK (79040000)
#define MT3620_CS_NULL  (7)
#define MT3620_CS_MAX   (1)

#endif // #ifndef MT3620_REG_SPI_H__
