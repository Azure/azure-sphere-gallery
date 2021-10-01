/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// <summary>
/// Structure defining the ring buffer handle type, to be used 
/// in handling all the object-data related to a ring buffer.
/// </summary>
typedef struct {

	uint8_t *bufferBase;

	volatile uint32_t bufferHead;
	volatile uint32_t bufferTail;
	volatile uint32_t bufferCount;
	uint32_t bufferMaxSize;

} ringBuffer_t;


/// <summary>
/// 
/// </summary>
/// <param name="rb">Pointer to a 'ringBuffer_t' type, where all the ring buffer variables are maintained</param>
/// <param name="bufferBase">The address of the memory block that will be used</param>
/// <param name="maxSize">The size of the memory block</param>
void ring_buffer_init(ringBuffer_t *rb, uint8_t *bufferBase, uint32_t maxSize);

/// <summary>
/// Returns the current bytes stored in the ring buffer.
/// </summary>
/// <param name="rb">Pointer to a 'ringBuffer_t' type, where all the ring buffer variables are maintained</param>
/// <returns>
///	The current byte-count stored in the ring buffer.
/// </returns>
uint32_t ring_buffer_count(ringBuffer_t *rb);

/// <summary>
/// Checks if the current bytes stored in the ring buffer have reached the maximum capacity.
/// </summary>
/// <param name="rb">Pointer to a 'ringBuffer_t' type, where all the ring buffer variables are maintained.</param>
/// <returns>'true' is the ring buffer is full, 'false' otherwise.</returns>
bool ring_buffer_isFull(ringBuffer_t *rb);

/// <summary>
/// Stores a byte-buffer starting from the ring buffer's head pointer.
/// </summary>
/// <param name="rb">Pointer to a 'ringBuffer_t' type, where all the ring buffer variables are maintained.</param>
/// <param name="buffer">Pointer to a byte-buffer, from where the data will be read.</param>
/// <param name="length">Length of the given byte-buffer.</param>
/// <returns>
/// '-1' on failure (i.e. buffer overrun)'
/// 'number of bytes stored' on success.
/// </returns>
int ring_buffer_push_bytes(ringBuffer_t *rb, uint8_t *buffer, uint32_t length);

/// <summary>
/// Retrieves a byte-buffer starting from the ring buffer's tail pointer.
/// </summary>
/// <param name="rb">Pointer to a 'ringBuffer_t' type, where all the ring buffer variables are maintained.</param>
/// <param name="buffer">Pointer to a byte-buffer, where the data will be copied.</param>
/// <param name="length">Length of the given byte-buffer.</param>
/// <returns>
/// '-1' on failure (i.e. buffer overrun)'
/// 'number of bytes read' on success.
/// </returns>
int ring_buffer_pop_bytes(ringBuffer_t *rb, uint8_t *buffer, uint32_t length);

