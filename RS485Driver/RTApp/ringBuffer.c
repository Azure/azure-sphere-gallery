/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#include "ringBuffer.h"


void ring_buffer_init(ringBuffer_t *rb, uint8_t *bufferBase, uint32_t maxSize)
{
	rb->bufferBase = bufferBase;
	rb->bufferHead = rb->bufferTail = rb->bufferCount = 0;
	rb->bufferMaxSize = maxSize;
}

inline uint32_t ring_buffer_count(ringBuffer_t *cb)
{
	return cb->bufferCount;
}

inline bool ring_buffer_isFull(ringBuffer_t *cb)
{
	return (cb->bufferCount >= cb->bufferMaxSize);
}

int ring_buffer_push_bytes(ringBuffer_t *rb, uint8_t *buffer, uint32_t length)
{
	if (rb->bufferCount + length < rb->bufferMaxSize)
	{
		for (unsigned i = 0; i < length; i++) {

			rb->bufferBase[rb->bufferHead++] = buffer[i];
			rb->bufferHead %= rb->bufferMaxSize;
		}
		rb->bufferCount += length;
		return length;
	}

	return -1;
}

int ring_buffer_pop_bytes(ringBuffer_t *rb, uint8_t *buffer, uint32_t length)
{
	if (rb->bufferCount > 0)
	{
		for (unsigned i = 0; i < length; i++) {

			buffer[i] = rb->bufferBase[rb->bufferTail++];
			rb->bufferTail %= rb->bufferMaxSize;
		}
		rb->bufferCount -= length;
		return length;
	}

	return -1;
}