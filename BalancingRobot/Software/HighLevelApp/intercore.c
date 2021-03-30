#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/log.h>
#include <applibs/application.h>
#include "utils.h"
#include "intercore.h"
#include "intercore_messages.h"

static const char rtAppComponentId[] = "8bad3ffb-ba15-4b81-9acb-0cc5bf5cfa2d";
int sockFd = -1;
static EventRegistration* socketEventReg = NULL;

extern struct DEVICE_STATUS device_status;

extern void SocketEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context);

void InitInterCoreCommunications(EventLoop* eventLoop)
{
	// Open connection to real-time capable application.
	sockFd = Application_Connect(rtAppComponentId);
	if (sockFd == -1) {
		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
		return;
	}

	// Set timeout, to handle case where real-time capable application does not respond.
	static const struct timeval recvTimeout = { .tv_sec = 5, .tv_usec = 0 };
	int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
	if (result == -1) {
		Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
		return;
	}

	 // Register handler for incoming messages from real-time capable application.
	socketEventReg = EventLoop_RegisterIo(eventLoop, sockFd, EventLoop_Input, SocketEventHandler, /* context */ NULL);
	if (socketEventReg == NULL) {
		Log_Debug("ERROR: Unable to register socket event: %d (%s)\n", errno, strerror(errno));
		return;
	}
}

void EnqueueIntercoreMessage(void* payload, size_t payload_size)
{
	int bytesSent = send(sockFd, payload, payload_size, 0);
	if (bytesSent == -1) {
		Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
		return;
	}
}
