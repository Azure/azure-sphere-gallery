/* Copyright (c) Codethink Ltd. All rights reserved.
   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This is derivative of logical-intercore.c in
// https://github.com/Azure/azure-sphere-samples/tree/master/Samples/IntercoreComms
// but rewritten to be more consistent with other high level drivers in
// sample set

#include <stdbool.h>
#include <stddef.h>

#include "lib/MBox.h"

#include "Socket.h"

#define FIFO_MSG_NEG_LEN 3

typedef struct __attribute__((__packed__)) {
    // read and write index in bytes
    uint32_t writeIndex;
    uint32_t readIndex;
    uint32_t reserved[14];
} Socket_Ringbuffer_Header;

typedef struct __attribute__((__packed__)) {
    Socket_Ringbuffer_Header header;
    uint8_t                  data[];
} Socket_Ringbuffer_Shared;

typedef struct {
    Socket_Ringbuffer_Shared *sharedData;
    uintptr_t                 capacity;
} Socket_Ringbuffer;

#define RB_WRITE_INDEX(rb) rb.sharedData->header.writeIndex
#define RB_READ_INDEX(rb)  rb.sharedData->header.readIndex

typedef struct {
    Component_Id  comp_id;
    uint32_t      reserved;
} Socket_Msg_Header;

/* Handle to socket connection containing state of shared ring buffer

   ringRemote state is updated by the A7 core and read by the M4 core
   ringLocal state is updated by the M4 core and read by the A7 core */
struct Socket {
    bool               open;
    void             (*rx_cb)(Socket*);
    MBox              *mailbox;
    Socket_Ringbuffer  ringRemote;
    Socket_Ringbuffer  ringLocal;
};

static Socket context = {0};

// Buffer descriptor commands
#define SOCKET_CMD_LOCAL_BUFFER_DESC  0xba5e0001
#define SOCKET_CMD_REMOTE_BUFFER_DESC 0xba5e0002
#define SOCKET_CMD_END_OF_SETUP       0xba5e0003

// Blocks inside the shared buffer have this alignment.
#define RB_ALIGNMENT 16
// Maximum payload size in bytes. This does not include a header which
// is prepended by
#define RB_MAX_PAYLOAD_LEN 1040

static const uint8_t SOCKET_PORT_MSG_RECV = 1;
static const uint8_t SOCKET_PORT_MSG_SENT = 0;
static const uint8_t SOCKET_PORT_FLAGS    =
    (SOCKET_PORT_MSG_RECV + 1) | (SOCKET_PORT_MSG_SENT + 1);

static uint32_t RoundUp(uint32_t value, uint32_t alignment)
{
    // alignment must be a power of two.
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static Socket_Ringbuffer Socket_Ringbuffer__Parse_Desc(uint32_t buffer_desc)
{
    Socket_Ringbuffer buffer;
    // The buffer size is encoded as a power of two in the bottom five bits.
    buffer.capacity = (1U << (buffer_desc & 0x1F)) - sizeof(Socket_Ringbuffer_Header);
    // The buffer header is a 32-byte aligned pointer which is stored in the
    // top 27 bits.
    buffer.sharedData = (Socket_Ringbuffer_Shared*)(buffer_desc & ~0x1F);

    return buffer;
}

static void Socket__Msg_Available(void *user_data, uint8_t port)
{
    if ((port != SOCKET_PORT_MSG_RECV) || !user_data ||
        (port >= MBOX_SW_INT_PORT_COUNT))
    {
        return;
    }

    Socket *handle = (Socket*)user_data;

    handle->rx_cb(handle);
}

Socket* Socket_Open(void (*rx_cb)(Socket*))
{
    if (context.open) {
        return NULL;
    }

    // Initialise MBox and FIFO
    MBox *mbox;
    if ((mbox = MBox_FIFO_Open(
        MT3620_UNIT_MBOX_CA7, NULL, NULL, NULL, &context, -1, -1)) == NULL) {
        return NULL;
    }

    context.mailbox = mbox;
    if (Socket_Negotiate(&context) != ERROR_NONE) {
        Socket_Close(&context);
        return NULL;
    }

    // Setup SW Interrupts
    if (MBox_SW_Interrupt_Setup(
            context.mailbox, SOCKET_PORT_FLAGS,
            Socket__Msg_Available) != ERROR_NONE)
    {
        Socket_Close(&context);
        return NULL;
    }

    // Update context
    context.rx_cb = rx_cb;
    context.open  = true;

    return &context;
}

int32_t Socket_Close(Socket *socket)
{
    if (!socket || !socket->open) {
        return ERROR_PARAMETER;
    }

    MBox_SW_Interrupt_Teardown(socket->mailbox);
    MBox_FIFO_Close(socket->mailbox);
    socket->open = false;

    return ERROR_NONE;
}


bool Socket_NegotiationPending(Socket *socket)
{
    if (!socket)    {
        return false;
    }

    return (MBox_FIFO_Reads_Available(socket->mailbox) != 0);
}

int32_t Socket_Negotiate(Socket *socket)
{
    if (!socket) {
        return ERROR_SOCKET_NEGOTIATION;
    }

    // Get buffer descriptors from MBox FIFO
    uint32_t  cmd[FIFO_MSG_NEG_LEN], data[FIFO_MSG_NEG_LEN];

    // Block and wait for A7 core to negotiate buffer descriptors
    if (MBox_FIFO_ReadSync(socket->mailbox, cmd, data, FIFO_MSG_NEG_LEN) != ERROR_NONE)
    {
        MBox_FIFO_Close(socket->mailbox);
        return ERROR_SOCKET_NEGOTIATION;
    }

    // Parse buffer descriptors
    Socket_Ringbuffer ringRemote, ringLocal;
    unsigned parsed = 0;

    for (unsigned i = 0; i < FIFO_MSG_NEG_LEN; i++) {
        switch (cmd[i]) {
        case SOCKET_CMD_LOCAL_BUFFER_DESC:
            ringLocal = Socket_Ringbuffer__Parse_Desc(data[i]);
            parsed |= 1;
            break;

        case SOCKET_CMD_REMOTE_BUFFER_DESC:
            ringRemote = Socket_Ringbuffer__Parse_Desc(data[i]);
            parsed |= 2;
            break;

        case SOCKET_CMD_END_OF_SETUP:
            parsed |= 4;
            break;

        default:
            break;
        }
    }

    if ((parsed != 7) ||
       (ringLocal.capacity == 0) ||
       (ringRemote.capacity == 0))
    {
        return ERROR_SOCKET_NEGOTIATION;
    }

    socket->ringRemote = ringRemote;
    socket->ringLocal  = ringLocal;

    return ERROR_NONE;
}


void Socket_Reset(Socket *socket)
{
    if (!socket) {
        return;
    }

    MBox_FIFO_Reset(socket->mailbox, true);
}

static void Socket__Signal(Socket *socket, uint8_t port)
{
    // Ensure memory writes have completed (not just been sent) before raising interrupt.
    // "no instruction that appears in program order after the DSB instruction can execute until the
    // DSB completes" ARMv7M Architecture Reference Manual, ARM DDI 0403E.d S A3.7.3
    __asm__ volatile("dsb");
    MBox_SW_Interrupt_Trigger(socket->mailbox, port);
}

// Helper function for Socket_Write. Writes data to the local ringbuffer,
// and wraps around to start of buffer if required. Returns updated write position.
static uint32_t Socket__Write_RB(
    const Socket_Ringbuffer *rb, uint32_t startPos, const void *src, size_t size)
{
    uint32_t spaceToEnd = rb->capacity - startPos;

    uint32_t writeToEnd = size;
    // If the new data would wrap around the end of the buffer then only write
    // spaceToEnd bytes before subsequently writing to the start of the buffer.
    if (size > spaceToEnd) {
        writeToEnd = spaceToEnd;
    }

    const uint8_t *src8 = (const uint8_t *)src;

    __builtin_memcpy(&(rb->sharedData->data[startPos]), src8, writeToEnd);
    // If not enough space to write all data before end of buffer, then write remainder at start.
    __builtin_memcpy(&(rb->sharedData->data[0]), src8 + writeToEnd, size - writeToEnd);

    uint32_t finalPos = startPos + size;
    if (finalPos > rb->capacity) {
        finalPos -= rb->capacity;
    }
    return finalPos;
}

int32_t Socket_Write(
    Socket             *socket,
    const Component_Id *recipient,
    const void         *data,
    uint32_t            size)
{
    if (!socket || !recipient || !data || (size == 0)) {
        return ERROR_PARAMETER;
    }

    if (size > RB_MAX_PAYLOAD_LEN) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // Last position read by HLApp. Corresponding release occurs on
    // high-level core.
    uint32_t remoteReadPosition;
    __atomic_load(&(RB_READ_INDEX(socket->ringRemote)),
        &remoteReadPosition, __ATOMIC_ACQUIRE);
    // Last position written to by RTApp.
    uint32_t localWritePosition = RB_WRITE_INDEX(socket->ringLocal);

    // Sanity check read and write positions.
    if ((remoteReadPosition >= socket->ringLocal.capacity) ||
        ((remoteReadPosition % RB_ALIGNMENT) != 0) ||
        (localWritePosition >= socket->ringLocal.capacity) ||
        ((localWritePosition % RB_ALIGNMENT) != 0)) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // If the read pointer is behind the write pointer, then the free space
    // wraps around, and the used space doesn't.
    uint32_t availSpace;
    if (remoteReadPosition <= localWritePosition) {
        availSpace = remoteReadPosition - localWritePosition +
            socket->ringLocal.capacity;
    } else {
        availSpace = remoteReadPosition - localWritePosition;
    }

    // Check whether there is enough space to enqueue the next block.
    uint32_t reqBlockSize = sizeof(uint32_t) + sizeof(Socket_Msg_Header) + size;

    if (availSpace < reqBlockSize + RB_ALIGNMENT) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // The value in the block size field does not include the space taken by the
    // block size field itself.
    uint32_t blockSizeExcSizeField = reqBlockSize - sizeof(uint32_t);
    localWritePosition = Socket__Write_RB(
        &(socket->ringLocal), localWritePosition, &blockSizeExcSizeField,
        sizeof(blockSizeExcSizeField));

    // Write header
    Socket_Msg_Header msg_header = {0};
    msg_header.comp_id = *recipient;
    localWritePosition = Socket__Write_RB(
        &(socket->ringLocal), localWritePosition,
        &msg_header, sizeof(Socket_Msg_Header));

    // Write data
    localWritePosition = Socket__Write_RB(
        &(socket->ringLocal), localWritePosition, data, size);

    // Advance write position to start of next possible block.
    localWritePosition = RoundUp(localWritePosition, RB_ALIGNMENT);
    if (localWritePosition >= socket->ringLocal.capacity) {
        localWritePosition -= socket->ringLocal.capacity;
    }

    // Ensure write position update is seen after new content has been written.
    // Corresponding acquire is on high-level core.
    __atomic_store(
        &(RB_WRITE_INDEX(socket->ringLocal)),
        &localWritePosition, __ATOMIC_RELEASE);

    Socket__Signal(socket, SOCKET_PORT_MSG_SENT);

    return ERROR_NONE;
}

// Helper function for Socket_Read. Reads data from the remote ring buffer,
// and wraps around to start of buffer if required. Returns updated read position.
static uint32_t Socket__Read_RB(
    const Socket_Ringbuffer *rb, uint32_t startPos, void *dest, size_t size)
{
    uint32_t availToEnd = rb->capacity - startPos;

    uint32_t readFromEnd = size;
    // If the available data wraps around the end of the buffer then only read
    // availToEnd bytes before subsequently reading from the start of the buffer.
    if (size > availToEnd) {
        readFromEnd = availToEnd;
    }

    uint8_t *dest8 = (uint8_t *)dest;
    __builtin_memcpy(dest, &(rb->sharedData->data[startPos]), readFromEnd);

    // If block wrapped around the end of the buffer, then read remainder from start.
    __builtin_memcpy(dest8 + readFromEnd, &(rb->sharedData->data[0]), size - readFromEnd);

    uint32_t finalPos = startPos + size;
    if (finalPos > rb->capacity) {
        finalPos -= rb->capacity;
    }
    return finalPos;
}

int32_t Socket_Read(
    Socket       *socket,
    Component_Id *sender,
    void         *data,
    uint32_t     *size)
{
    if (!socket || !sender || !data || !size) {
        return ERROR_PARAMETER;
    }
    // Don't read message content until have seen that remote write position has been updated.
    // Corresponding release occurs on high-level core.
    uint32_t remoteWritePosition;
    __atomic_load(&(RB_WRITE_INDEX(socket->ringRemote)), &remoteWritePosition, __ATOMIC_ACQUIRE);
    // Last position read from by this RTApp.
    uint32_t localReadPosition = RB_READ_INDEX(socket->ringLocal);

    // Sanity check read and write positions.
    if ((remoteWritePosition >= socket->ringRemote.capacity) ||
        ((remoteWritePosition % RB_ALIGNMENT) != 0) ||
        (localReadPosition >= socket->ringRemote.capacity) ||
        ((localReadPosition % RB_ALIGNMENT) != 0)) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // Get the maximum amount of available data. The actual block size may be
    // smaller than this.

    uint32_t availData;
    // If data is contiguous in buffer then difference between write and read positions...
    if (remoteWritePosition >= localReadPosition) {
        availData = remoteWritePosition - localReadPosition;
    }
    // ...else data wraps around end and resumes at start of buffer
    else {
        availData = remoteWritePosition - localReadPosition + socket->ringRemote.capacity;
    }

    // The amount of available data must be at least enough to hold the block size.
    // If not, caller will assume that no message was available.
    const size_t blockSizeSize = sizeof(uint32_t);
    // The block size must be stored in four contiguous bytes before wraparound.
    uint32_t dataToEnd = socket->ringRemote.capacity - localReadPosition;
    if ((availData < blockSizeSize) || (blockSizeSize > dataToEnd)) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // The block size followed by the actual block can be no longer than the available data.
    uint32_t blockSize;
    localReadPosition = Socket__Read_RB(
        &(socket->ringRemote), localReadPosition, &blockSize, sizeof(blockSize));
    uint32_t totalBlockSize;

    totalBlockSize = blockSizeSize + blockSize;
    if (totalBlockSize > availData) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    if (blockSize < sizeof(Socket_Msg_Header)) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // The caller-supplied buffer must be large enough to contain the
    // payload in the buffer, excluding component ID and reserved word.
    size_t senderPayloadSize = blockSize - sizeof(Socket_Msg_Header);
    if (senderPayloadSize > *size) {
        return ERROR_SOCKET_INSUFFICIENT_SPACE;
    }

    // Tell the caller the actual block size.
    *size = senderPayloadSize;

    // Read the sender header. This may wraparound to the start of the buffer.
    localReadPosition = Socket__Read_RB(
        &(socket->ringRemote), localReadPosition,
        sender, sizeof(Socket_Msg_Header));

    // Read data
    localReadPosition = Socket__Read_RB(
        &(socket->ringRemote),
        localReadPosition, data, senderPayloadSize);

    // Align read position to next possible location for next buffer.
    // This may wrap around.
    localReadPosition = RoundUp(localReadPosition, RB_ALIGNMENT);
    if (localReadPosition >= socket->ringRemote.capacity) {
        localReadPosition -= socket->ringRemote.capacity;
    }

    // The message content must have been retrieved before the high-level core
    // sees the read position has been updated. Corresponding acquire occurs
    // on high-level core.
    __atomic_store(
        &(RB_READ_INDEX(socket->ringLocal)),
        &localReadPosition, __ATOMIC_RELEASE);

    Socket__Signal(socket, SOCKET_PORT_MSG_RECV);

    return ERROR_NONE;
}
