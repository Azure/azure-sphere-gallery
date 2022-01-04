/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "settime.h"
#include "parson.h"
#include "httpGet.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <applibs/log.h>
// used to set the local time.

const char* TimeZoneAPITemplate = "https://api.timezonedb.com/v2.1/get-time-zone?key=<YOUR_API_KEY_HERE>&format=json&by=position&lat=%f&lng=%f";

char urlWithParameters[128];	// buffer to hold REST API URL including lat/long.

void SetLocalTime(double lat, double lng)
{
	char localTimeBuffer[80];

	int ret = snprintf(urlWithParameters, sizeof(urlWithParameters), TimeZoneAPITemplate, lat, lng);
	if (ret >= sizeof(urlWithParameters))	// buffer isn't large enough.
	{
		Log_Debug("SetLocalTime, buffer is too small - cannot set local time\n");
		return;
	}

	char *data = getHttpData(urlWithParameters);

	if (data != NULL)
	{
		JSON_Value* rootValue = json_parse_string(data);
		JSON_Object* rootObject = json_value_get_object(rootValue);
		time_t timestamp = (time_t)json_object_get_number(rootObject, "timestamp");

		json_value_free(rootValue);

		struct timespec tv;

		tv.tv_nsec = 0;
		tv.tv_sec = timestamp;	// replace with contents of json.

		clock_settime(CLOCK_REALTIME, &tv);

		// show local time.
		struct tm  ts;
		ts = *localtime(&timestamp);
		if (strftime(localTimeBuffer, sizeof(localTimeBuffer), "%a %Y-%m-%d %H:%M:%S", &ts) != 0)
		{
			Log_Debug("Time set to: %s\n", localTimeBuffer);
		}

		free(data);
	}
}
