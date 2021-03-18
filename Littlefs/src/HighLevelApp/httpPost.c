/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "httpPost.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <applibs/log.h>
#include <applibs/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct WriteThis {
	const char* readptr;
	size_t sizeleft;
};

extern char urlBuffer[256];

static size_t read_callback(char* dest, size_t size, size_t nmemb, void* userp)
{
	struct WriteThis* wt = (struct WriteThis*)userp;
	size_t buffer_size = size * nmemb;

	if (wt->sizeleft) {
		/* copy as much as possible from the source to the destination */
		size_t copy_this_much = wt->sizeleft;
		if (copy_this_much > buffer_size)
			copy_this_much = buffer_size;
		memcpy(dest, wt->readptr, copy_this_much);

		wt->readptr += copy_this_much;
		wt->sizeleft -= copy_this_much;
		return copy_this_much; /* we copied this many bytes */
	}

	return 0; /* no more data left to deliver */
}

static char *writeBlockURL = "http://%s:5000/WriteBlockFromOffset";

int writeBlockData(uint8_t* sectorData, uint32_t size, uint32_t offset)
{
	CURL* curl;
	CURLcode res = -1;

	struct WriteThis wt;

	wt.readptr = sectorData;
	wt.sizeleft = size;

	snprintf(urlBuffer, 255, writeBlockURL, PC_HOST_IP);

	curl = curl_easy_init();
	if (curl)
	{
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, urlBuffer);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);

		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)5);
		curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1);

		curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
		curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);

		struct curl_slist* hs = NULL;
		char tBuff[50];
		memset(&tBuff[0], 0x00, 50);
		snprintf(tBuff, 50, "offset: %d", offset);
		hs = curl_slist_append(hs, tBuff);
		hs = curl_slist_append(hs, "Content-Type: application/octet-stream");
		hs = curl_slist_append(hs, "Expect:");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

		res = curl_easy_perform(curl);

		curl_slist_free_all(hs);
		curl_easy_cleanup(curl);
	}

	if (res == CURLE_OK)
		return 0;

	return -1;
}

int writeTrackData(uint8_t trackNum, uint8_t* trackData, uint16_t length)
{
	uint16_t crc = 0;
	for (int x = 0; x < length; x++)
	{
		crc += trackData[x];
		crc &= 0xffff;
	}

	CURL* curl;
	CURLcode res = -1;

	struct WriteThis wt;

	wt.readptr = trackData;
	wt.sizeleft = length;

	curl = curl_easy_init();
	if (curl)
	{

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, writeBlockURL);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);

		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)5);
		curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1);

		curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
		curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);

		struct curl_slist* hs = NULL;
		char tBuff[50];
		memset(&tBuff[0], 0x00, 50);
		snprintf(tBuff, 50, "track: %d", trackNum);
		hs = curl_slist_append(hs, tBuff);
		hs = curl_slist_append(hs, "Content-Type: application/octet-stream");
		hs = curl_slist_append(hs, "Expect:");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

		res = curl_easy_perform(curl);

		curl_slist_free_all(hs);
		curl_easy_cleanup(curl);
	}

	if (res == CURLE_OK)
		return 0;

	return -1;
}
