/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "httpGet.h"
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

struct url_data data;

char urlBuffer[255];

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

static const readUrl = "http://%s:5000/ReadBlockFromOffset?offset=%u&size=%u";

uint8_t * readBlockData(uint32_t offset, uint32_t size)
{
	snprintf(urlBuffer, 255, readUrl, PC_HOST_IP, offset,size);

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
