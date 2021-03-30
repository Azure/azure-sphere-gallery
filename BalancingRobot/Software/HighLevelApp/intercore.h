#pragma once
#include "eventloop_timer_utilities.h"

void InitInterCoreCommunications(EventLoop* eventLoop);

void EnqueueIntercoreMessage(void* payload, size_t payload_size);
