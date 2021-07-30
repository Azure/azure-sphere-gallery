/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "lib/mt3620/gpt.h"

#include "SD.h"

// This is the maximum number of SD cards which can be opened at once.
#define SD_CARD_MAX       4
#define SPI_SD_TIMEOUT    200 // [ms]
#define NUM_RETRIES       65536
#define NUM_WRITE_RETRIES 3

static GPT *timer = NULL;

typedef enum {
    GO_IDLE_STATE        =  0,
    SEND_OP_COND         =  1,
    ALL_SEND_CID         =  2,
    SEND_RELATIVE_ADDR   =  3,
    SWITCH_FUNC          =  6,
    SELECT_CARD          =  7,
    SEND_IF_COND         =  8,
    SEND_CSD             =  9,
    SEND_CID             = 10,
    READ_DAT_UNTIL_STOP  = 11,
    STOP_TRANSMISSION    = 12,
    GO_INACTIVE_STATE    = 15,
    SET_BLOCKLEN         = 16,
    READ_SINGLE_BLOCK    = 17,
    READ_MULTIPLE_BLOCK  = 18,
    SET_BLOCK_COUNT      = 23,
    WRITE_BLOCK          = 24,
    WRITE_MULTIPLE_BLOCK = 25,
    PROGRAM_CSD          = 27,
    SET_WRITE_PROT       = 28,
    CLR_WRITE_PROT       = 29,
    SEND_WRITE_PROT      = 30,
    ERASE_WR_BLK_START   = 32,
    ERASE_WR_BLK_END     = 33,
    ERASE                = 38,
    LOCK_UNLOCK          = 42,
    APP_CMD              = 55,
    GEN_CMD              = 56,
    READ_OCR             = 58,
    CRC_ON_OFF           = 59,
} SD_CMD;

typedef enum {
    APP_SET_BUS_WIDTH          =  6,
    APP_SD_STATUS              = 13,
    APP_SEND_NUM_WR_BLOCKS     = 22,
    APP_SET_WR_BLK_ERASE_COUNT = 23,
    APP_SEND_OP_COND           = 41,
    APP_SET_CLR_CARD_DETECT    = 42,
    APP_SEND_SCR               = 51,
} SD_ACMD;

typedef enum {
    DATA_TOKEN_READ_SINGLE     = 0xFE,
    DATA_TOKEN_READ_MULT       = 0xFE,
    DATA_TOKEN_WRITE_SINGLE    = 0xFE,
    DATA_TOKEN_WRITE_MULT      = 0xFC,
    DATA_TOKEN_WRITE_MULT_STOP = 0xFD
} SD_DATA_TOKEN;

typedef enum {
    DATA_RESP_ACCEPTED    = 0x5,
    DATA_RESP_CRC_ERROR   = 0xB,
    DATA_RESP_WRITE_ERROR = 0xD
} SD_DATA_RESPONSE;

typedef struct __attribute__((__packed__)) {
    uint8_t  index;
    uint32_t argument;
    uint8_t  crc;
} SD_CommandFrame;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        bool     idle               : 1;
        bool     erase              : 1;
        bool     illegalCommand     : 1;
        bool     comCrcError        : 1;
        bool     eraseSequenceError : 1;
        bool     addressError       : 1;
        bool     parameterError     : 1;
        unsigned zero               : 1;
    };
    uint8_t mask;
} SD_R1;

typedef struct __attribute__((__packed__)) {
    SD_R1    r1;
    uint32_t ocr;
} SD_R3;

typedef struct __attribute__((__packed__)) {
    SD_R1 r1;

    union __attribute__((__packed__)) {
        struct __attribute__((__packed__)) {
            unsigned commandVersion  :  4;
            unsigned reserved        : 16;
            unsigned voltageAccepted :  4;
            unsigned checkPattern    :  8;
        };
        uint32_t mask;
    };
} SD_R7;


struct SDCard {
    SPIMaster *interface;
    uint32_t   blockLen;
    uint32_t   tranSpeed;
    uint32_t   maxTranSpeed;
};

typedef struct {
    bool    done;
    int32_t status;
    int32_t count;
} TransferState;

static volatile TransferState transferState = {
    .done   = false,
    .status = ERROR_NONE,
    .count = 0
};

static void transferDoneCallback(int32_t status, uintptr_t dataCount)
{
    transferState.done   = true;
    transferState.status = status;
    transferState.count  = dataCount;
}

static void transferStateReset()
{
    transferState.done   = false;
    transferState.status = ERROR_NONE;
    transferState.count  = 0;
}

static uint8_t SD_Crc7(void *data, uintptr_t size)
{
    uint8_t* data_byte = data;
    uint8_t crc = 0x00;
    uintptr_t byte;
    for (byte = 0; byte < size; byte++) {
        uint8_t c = data_byte[byte];
        unsigned bit;
        for (bit = 0; bit < 8; bit++) {
            crc <<= 1;
            if ((c ^ crc) & 0x80) {
                crc ^= 0x09;
            }
            c <<= 1;
        }
        crc &= 0x7F;
    }

    return (crc << 1) | 0x01;
}

typedef enum {
    SPI_READ  = 0,
    SPI_WRITE = 1
} SPI_TRANSFER_TYPE;

static bool SPITransfer__SyncTimeout(
    SPIMaster         *interface,
    void              *data,
    uintptr_t          length,
    SPI_TRANSFER_TYPE  transferType)
{
    if (!interface) {
        return false;
    }

    SPITransfer transfer = {
        .writeData = NULL,
        .readData  = NULL,
        .length    = length,
    };

    int32_t status;
    switch (transferType) {
    case SPI_READ:
        transfer.readData = data;
        status = SPIMaster_TransferSequentialAsync(interface, &transfer, 1, transferDoneCallback);
        break;

    case SPI_WRITE:
        transfer.writeData = data;
        status = SPIMaster_TransferSequentialAsync(interface, &transfer, 1, transferDoneCallback);
        break;

    default:
        return false;
    }

    if (status != ERROR_NONE) {
        return false;
    }

    if (GPT_IsEnabled(timer)) {
        GPT_Stop(timer);
    }
    unsigned retries = NUM_RETRIES;
    while ((retries-- > 0) && (GPT_StartTimeout(
        timer, SPI_SD_TIMEOUT, GPT_UNITS_MILLISEC, NULL) == ERROR_NONE))
        ;
    if (retries == 0) {
        transferStateReset();
        return false;
    }

    while (!transferState.done) {
        __asm__("wfi");
        if (!GPT_IsEnabled(timer)) {
            // Timed out, so cancel
            SPIMaster_TransferCancel(interface);
            transferState.status = ERROR_TIMEOUT;
            break;
        }
    }

    status = transferState.status;
    transferStateReset();

    if (status != ERROR_NONE) {
        return false;
    }

    return true;
}

static bool SD_ClockBurst(SPIMaster* interface, unsigned cycles, bool select)
{
    if (cycles == 0) {
        return true;
    }

    int32_t status;
    if (!select) {
        status = SPIMaster_SelectEnable(interface, false);
        if (status != ERROR_NONE) {
            return false;
        }
    }

    // Burst the clock for a bit to allow command to process
    // We use async here so we can timeout if the SD card hangs
    uint8_t dummy[(cycles + 7) / 8];

    if (!SPITransfer__SyncTimeout(interface, &dummy, sizeof(dummy), SPI_READ)) {
        return false;
    }

    if (!select) {
        status = SPIMaster_SelectEnable(interface, true);
        if (status != ERROR_NONE) {
            return false;
        }
    }

    return true;
}

static bool SD_AwaitResponse(SPIMaster *interface, uintptr_t size, void *response, unsigned retries)
{
    uint8_t byte = 0xFF;
    unsigned i;
    for (i = 0; (i < retries) && (byte == 0xFF); i++) {
        if (!SPITransfer__SyncTimeout(interface, &byte, 1, SPI_READ)) {
            return false;
        }
    }

    uint8_t* r = response;
    r[0] = byte;
    if ((byte & 0x7C) != 0) {
        // If the response contains an error, it won't have a payload.
        size = 1;
    } else if (size > 1) {
        if (!SPITransfer__SyncTimeout(interface, &r[1], (size - 1), SPI_READ)) {
            return false;
        }
    }

    return true;
}

static bool SD_CommandIncomplete(SPIMaster *interface, SD_CMD cmd, uint32_t argument,
                                 uintptr_t response_size, void* response)
{
    SD_CommandFrame frame;
    frame.index    = (0b01 << 6) | cmd;
    frame.argument = __builtin_bswap32(argument);
    frame.crc      = SD_Crc7(&frame, (sizeof(frame.index) + sizeof(frame.argument)));


    if (!SPITransfer__SyncTimeout(interface, &frame, sizeof(frame), SPI_WRITE)) {
        return false;
    }

    // Ignore first byte of response.
    if (!SD_ClockBurst(interface, 8, true)) {
        return false;
    }

    unsigned retries = 32;
    if (!SD_AwaitResponse(interface, response_size, response, retries)) {
        return false;
    }

    return true;
}

static bool SD_Command(SPIMaster *interface, SD_CMD cmd, uint32_t argument,
                       uintptr_t response_size, void* response)
{
    if (!SD_CommandIncomplete(
        interface, cmd, argument, response_size, response)) {
        return false;
    }

    // Burst the clock for a bit to allow command to process
    if (!SD_ClockBurst(interface, 32, false)) {
        return false;
    }

    return true;
}

static bool SD_ReadDataPacket(const SDCard *card, uintptr_t size, void *data)
{
    unsigned retries = NUM_RETRIES;
    uint8_t byte = 0xFF;
    unsigned i;
    for (i = 0; (i < retries) && (byte == 0xFF); i++) {
        if (!SPITransfer__SyncTimeout(card->interface, &byte, 1, SPI_READ)) {
            return false;
        }
    }
    if (byte != DATA_TOKEN_READ_SINGLE) {
        return false;
    }

    uint8_t *data_byte = data;
    uintptr_t packet = 16;
    uintptr_t remain = size;
    for (remain = size; remain; remain -= packet, data_byte += packet) {
        if (packet > remain) {
            packet = remain;
        }

        if (!SPITransfer__SyncTimeout(card->interface, data_byte, packet, SPI_READ)) {
            return false;
        }
    }

    uint16_t crc;
    if (!SPITransfer__SyncTimeout(card->interface, &crc, sizeof(crc), SPI_READ)) {
        return false;
    }

    // TODO: Verify the CRC.

    // Clock burst is required here to give the card time to recover?
    SD_ClockBurst(card->interface, 32, false);

    return true;
}

static bool SD_WriteDataPacket(SDCard *card, uintptr_t size, const void *data)
{
    // Clock burst for >= 1 byte
    SD_ClockBurst(card->interface, 16, false);

    // Write data token
    static uint8_t write_token = DATA_TOKEN_WRITE_SINGLE;
    if (!SPITransfer__SyncTimeout(card->interface, &write_token, 1, SPI_WRITE)) {
        return false;
    }

    // Write data
    uint8_t *data_byte = (uint8_t*)data;
    uintptr_t packet = 16;
    uintptr_t remain = size;
    for (remain = size; remain; remain -= packet, data_byte += packet) {
        if (packet > remain) {
            packet = remain;
        }

        if (!SPITransfer__SyncTimeout(
            card->interface, data_byte, packet, SPI_WRITE))
        {
            return false;
        }
    }

    // Write crc
    // TODO: implement 16 bit crc calc (SPI mode SD cards ignore CRC)
    static uint16_t blank_crc = 0xFFFF;
    if (!SPITransfer__SyncTimeout(
        card->interface, &blank_crc, sizeof(blank_crc), SPI_WRITE))
    {
        return false;
    }

    // Read data response
    unsigned retries = NUM_RETRIES;
    uint8_t byte = 0xFF;
    unsigned i;
    for (i = 0; (i < retries) && (byte == 0xFF); i++) {
        if (!SPITransfer__SyncTimeout(card->interface, &byte, 1, SPI_READ)) {
            return false;
        }
    }
    if ((byte & 0xF) != DATA_RESP_ACCEPTED) {
        return false;
    }

    // Wait while card holds MISO low (busy)
    unsigned busy_waits = NUM_RETRIES;
    byte = 0x00;
    for (i = 0; (i < busy_waits) && (byte == 0x00); i++) {
        if (!SPITransfer__SyncTimeout(card->interface, &byte, 1, SPI_READ)) {
            return false;
        }
    }

    if (byte == 0x00) {
        return false;
    }

    return true;
}

static bool SD_ReadCSD(SDCard *card)
{
    SD_R1 response;
    if (!SD_CommandIncomplete(card->interface, SEND_CSD, 0, sizeof(response), &response)) {
        return false;
    }
    if ((response.mask & 0xC0) != 0) {
        return false;
    }

    uint8_t csd[16];
    if (!SD_ReadDataPacket(card, sizeof(csd), csd)) {
        return false;
    }

    uint8_t tranSpeedRaw = csd[3];

    unsigned tranSpeedUnitRaw = (tranSpeedRaw & 0x07);
    unsigned tranSpeedUnit = 10000;
    for (; tranSpeedUnitRaw > 0; tranSpeedUnit *= 10, tranSpeedUnitRaw--);

    unsigned tranSpeedValueRaw = ((tranSpeedRaw >> 3) & 0xF);
    static unsigned tranSpeedValueTable[16] = {
         0, 10, 12, 13,
        15, 20, 25, 30,
        35, 40, 45, 50,
        55, 60, 70, 80,
    };
    unsigned tranSpeedValue = tranSpeedValueTable[tranSpeedValueRaw];

    if ((tranSpeedValue != 0)
        && ((tranSpeedRaw & 0x80) == 0)) {
        card->maxTranSpeed = tranSpeedValue * tranSpeedUnit;
    }

    return true;
}


static bool SD_GoIdleState(SPIMaster *interface, unsigned retries)
{
    unsigned i;
    for (i = 0; i < retries; i++) {
        SD_R1 response;
        if (!SD_Command(interface, GO_IDLE_STATE, 0, sizeof(response), &response)) {
            return false;
        }

        if (response.mask == 0x01) {
            break;
        }
    }
    return (i < retries);
}

static bool SD_SendIfCond(SPIMaster *interface)
{
    SD_R7 response7;
    if (!SD_Command(interface, SEND_IF_COND, 0x000001AA, sizeof(response7), &response7)) {
        return false;
    }

    SD_R1 response = response7.r1;
    if (response.mask == 0x01) {
        if (__builtin_bswap32(response7.mask) != 0x000001AA) {
            return false;
        }
    } else if (response.mask != 0x05) {
        return false;
    }

    return true;
}

static bool SD_SendOpCond(SPIMaster *interface, unsigned retries)
{
    SD_R1 response;
    if (!SD_Command(interface, APP_CMD, 0, sizeof(response), &response)) {
        return false;
    }

    if (response.mask == 0x01) {
        if (!SD_Command(interface, APP_SEND_OP_COND, 0x40000000, sizeof(response), &response)) {
            return false;
        }

        unsigned i;
        for (i = 1; (i < retries) && (response.mask == 0x01); i++) {
            if (!SD_Command(interface, APP_CMD, 0, sizeof(response), &response)
                || !SD_Command(interface, APP_SEND_OP_COND, 0x40000000, sizeof(response), &response)) {
                return false;
            }
        }
    } else if (response.mask == 0x05) {
        unsigned i;
        for (i = 0; (i == 0) || (response.mask == 0x01); i++) {
            if (!SD_Command(interface, SEND_OP_COND, 0, sizeof(response), &response)) {
                return false;
            }
        }
    }

    return (response.mask == 0x00);
}

static bool SD_Initialize(SPIMaster *interface)
{
    // Transfer 74 or more clock pulses to initialize card.
    return (SD_ClockBurst(interface, 74, false)
        && SD_GoIdleState(interface, 5)
        && SD_SendIfCond(interface)
        && SD_SendOpCond(interface, 256));
}


SDCard *SD_Open(SPIMaster *interface)
{
    static SDCard SD_Cards[SD_CARD_MAX] = {0};
    SDCard *card = NULL;
    unsigned c;
    for (c = 0; c < SD_CARD_MAX; c++) {
        if (!SD_Cards[c].interface) {
            card = &SD_Cards[c];
            break;
        }
    }
    if (!card) {
        return NULL;
    }

    if (!(timer = GPT_Open(
        MT3620_UNIT_GPT3, MT3620_GPT_3_LOW_SPEED, GPT_MODE_ONE_SHOT))) {
        return NULL;
    }


    // Configure SPI Master to 400 kHz.
    SPIMaster_Configure(interface, 0, 0, 400000);

    unsigned retries = 5;
    unsigned i;
    for (i = 0; i < retries; i++) {
        if (SD_Initialize(interface)) {
            break;
        }
    }
    if (i >= retries) {
        return NULL;
    }

    card->interface    = interface;
    card->blockLen     = 512;
    card->maxTranSpeed = 400000;
    card->tranSpeed    = 400000;

    if (SD_ReadCSD(card) && (card->maxTranSpeed != card->tranSpeed)) {
        if (SPIMaster_Configure(card->interface, 0, 0, card->maxTranSpeed) == ERROR_NONE) {
            card->tranSpeed = card->maxTranSpeed;
        }
    }

    return card;
}


void SD_Close(SDCard *card)
{
    card->interface = NULL;
    GPT_Close(timer);
}


uint32_t SD_GetBlockLen(const SDCard *card)
{
    return (card ? card->blockLen : 0);
}


bool SD_SetBlockLen(SDCard *card, uint32_t len)
{
    if (!card || (len == 0)) {
        return false;
    }

    SD_R1 response;
    if (!SD_Command(card->interface, SET_BLOCKLEN, len, sizeof(response), &response)) {
        return false;
    }

    if (response.mask != 0x00) {
        return false;
    }

    card->blockLen = len;
    return true;
}


bool SD_ReadBlock(const SDCard *card, uint32_t addr, void *data)
{
    if (!card || !data) {
        return false;
    }

    SD_R1 response;
    if (!SD_CommandIncomplete(card->interface, READ_SINGLE_BLOCK, addr, sizeof(response), &response)) {
        return false;
    }

    if (response.mask != 0x00) {
        return false;
    }

    return SD_ReadDataPacket(card, card->blockLen, data);
}


bool SD_WriteBlock(SDCard *card, uint32_t addr, const void *data)
{
    if (!card || !data) {
        return false;
    }

    static unsigned num_retries = NUM_WRITE_RETRIES;

    SD_R1 response;
    if (!SD_CommandIncomplete(card->interface, WRITE_BLOCK, addr, sizeof(response), &response)) {
        return false;
    }

    if (response.mask != 0x00) {
        return false;
    }

    if (!SD_WriteDataPacket(card, card->blockLen, data)) {
        if (num_retries > 0) {
            num_retries--;
            return SD_WriteBlock(card, addr, data);
        }
        else {
            return false;
        }
    }
    else {
        num_retries = NUM_WRITE_RETRIES;
        return true;
    }
}
