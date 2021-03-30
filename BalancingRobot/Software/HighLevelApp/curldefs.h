#pragma once

#include <curl/curl.h>
#include <curl/easy.h>

// Curl stuff.
struct url_data {
	size_t size;
	char* data;
};

struct url_data data;

//CURL *curl;
CURLcode res;
