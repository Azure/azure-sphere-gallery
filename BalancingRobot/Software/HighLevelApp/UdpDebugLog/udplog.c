/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifdef USE_SOCKET_LOG

#include <applibs/log.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>

#define PORT 1824

static int 	sock=-1;

static struct sockaddr_in broadcast_addr;
static socklen_t addr_len;
static int ret;
static char sock_buffer[500];

static bool slogInit = false;

void initSlog(void);


int Log_Debug(const char *fmt, ...)
{
	if (!slogInit)
	{
		initSlog();
	}

	memset(sock_buffer, 0x00, sizeof(sock_buffer));
	va_list args;
	va_start(args, fmt);

	// You could use the first four bytes to contain a unique value per device.
	sock_buffer[3] = 0xff;
	sock_buffer[2] = 0xff;
	sock_buffer[1] = 0xff;
	sock_buffer[0] = 0xff;

	int vsRet=vsnprintf(sock_buffer+4, sizeof(sock_buffer) - 4, fmt, args);
	va_end(args);

	if (vsRet < 0)	// encoding error
	{
		perror("Log_Debug - Cannot compose message\n");
		return -1;
	}

	ret = sendto(sock, sock_buffer, strnlen(sock_buffer + 4, sizeof(sock_buffer) - 4)+4, 0, (struct sockaddr*) &broadcast_addr, addr_len);
	return ret;
}


void initSlog(void)
{
	int yes = 1;

	slogInit = true;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("sock error");
		return;
	}
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	if (ret == -1) {
		perror("setsockopt error");
		close(sock);
		sock=-1;
		return;
	}

	addr_len = sizeof(struct sockaddr_in);

	memset((void*)&broadcast_addr, 0, addr_len);
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast_addr.sin_port = htons(PORT);
}
#endif
