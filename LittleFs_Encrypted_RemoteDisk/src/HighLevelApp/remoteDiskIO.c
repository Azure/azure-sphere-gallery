/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdint.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <stdlib.h>
#include <string.h>

#include "crypt.h"
#include "remoteDiskIO.h"

static uint8_t readBuffer[STORAGE_BLOCK_SIZE + STORAGE_METADATA_SIZE + 64];

// Check there's no padding in the struct
_Static_assert(sizeof(StorageBlock) == STORAGE_BLOCK_SIZE + STORAGE_METADATA_SIZE);

// Curl stuff.
typedef struct {
	uint8_t* data;
	size_t size;
} Transfer;

static char UrlBuffer[255];

static size_t writeCallback(void* ptr, size_t size, size_t nmemb, Transfer* data)
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

static const char *readUrl = "http://%s:5000/ReadBlock?block=%u";

int readBlockData(uint32_t blockNum, StorageBlock* block)
{
	static Transfer transfer;
	snprintf(UrlBuffer, 255, readUrl, PC_HOST_IP, blockNum);

	// use fixed buffer, reduce the number of mallocs.
	transfer.size = 0;
	transfer.data = &readBuffer[0];

	CURLcode res = CURLE_OK;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, UrlBuffer);
	/* use a GET to fetch data */
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &transfer);

	// based on the libcurl sample - https://curl.se/libcurl/c/https.html 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	/* Perform the request */
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res == CURLE_OK)
	{
		if (transfer.size != STORAGE_BLOCK_SIZE + STORAGE_METADATA_SIZE) {
			memset(block, 0, sizeof(StorageBlock));
			return -1;
		}
		memcpy(block, transfer.data, STORAGE_BLOCK_SIZE + STORAGE_METADATA_SIZE);
		return 0;
	}

	return -1;
}

static size_t readCallback(char* dest, size_t size, size_t nmemb, Transfer* transfer)
{
	size_t buffer_size = size * nmemb;

	if (transfer->size) {
		/* copy as much as possible from the source to the destination */
		size_t copy_this_much = transfer->size;
		if (copy_this_much > buffer_size)
			copy_this_much = buffer_size;
		memcpy(dest, transfer->data, copy_this_much);

		transfer->data += copy_this_much;
		transfer->size -= copy_this_much;
		return copy_this_much; /* we copied this many bytes */
	}

	return 0; /* no more data left to deliver */
}

static char* writeBlockURL = "http://%s:5000/WriteBlock?block=%u";

int writeBlockData(uint32_t blockNum, const StorageBlock* sectorData)
{
	static Transfer transfer;
	CURL* curl;
	CURLcode res = -1;

	transfer.data = (void*)sectorData;
	transfer.size = sizeof(StorageBlock);

	snprintf(UrlBuffer, 255, writeBlockURL, PC_HOST_IP, blockNum);

	curl = curl_easy_init();
	if (curl)
	{
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, UrlBuffer);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &transfer);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)transfer.size);

		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)5);
		curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1);

		curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
		curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);

		struct curl_slist* hs = NULL;
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