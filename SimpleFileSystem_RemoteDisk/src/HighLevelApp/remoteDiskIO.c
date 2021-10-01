/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "remoteDiskIO.h"
#include "stdint.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <string.h>

static uint8_t readBuffer[4096];

// Curl stuff.
struct url_data {
	size_t size;
	uint8_t* data;
};

struct WriteThis {
	const char* readptr;
	size_t sizeleft;
};

struct url_data data;

static char urlBuffer[255];

static size_t write_data(void* ptr, size_t size, size_t nmemb, struct url_data* data)
{
	size_t index = data->size;
	size_t n = (size * nmemb);

	// bug out if the data returned is too large.
	if (data->size + n > sizeof(readBuffer))
		return 0;

	data->size += n;

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return n;
}

static const char *readUrl = "http://%s:5000/ReadBlockFromOffset?offset=%u&size=%u";

uint8_t* readBlockData(uint32_t offset, uint32_t size)
{
	snprintf(urlBuffer, 255, readUrl, PC_HOST_IP, offset, size);

	// use fixed buffer, reduce the number of mallocs.
	data.size = 0;
	data.data = &readBuffer[0];

	CURLcode res = CURLE_OK;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, urlBuffer);
	/* use a GET to fetch data */
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	// based on the libcurl sample - https://curl.se/libcurl/c/https.html 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	/* Perform the request */
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res == CURLE_OK)
	{
		// caller is responsible for freeing this.
		return data.data;
	}

	return NULL;
}

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

static char* writeBlockURL = "http://%s:5000/WriteBlockFromOffset";

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
