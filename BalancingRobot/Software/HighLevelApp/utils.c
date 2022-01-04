#include "utils.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool IsNetworkReady(void)
{
	bool haveNetwork = false;
	Networking_IsNetworkingReady(&haveNetwork);

	return haveNetwork;
}

void delay(int ms)
{
//#ifdef SHOW_DEBUG_MSGS
//	Log_Debug(">>> %s\n", __func__);
//#endif

	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

int genGuid(unsigned char* buf, size_t len)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	memset(buf, 0x00, len + 1);	// assume buffer is longer than the requested guid length
	struct timespec tm;
	clock_gettime(CLOCK_REALTIME, &tm);
	// setup the random seed.
	srand(tm.tv_sec);
	Log_Debug(">>> genGuid\n");
	const char* hex = "0123456789ABCDEF";
	for (int x = 0; x < len; x++)
	{
		int r = rand() % 16;
		buf[x] = hex[r];
	}

	return 0;
}