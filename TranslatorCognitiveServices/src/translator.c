/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/log.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include "parson.h"
#include <string.h>
#include <stdlib.h>


static char* translatorAPIKey = "";	// Get the Translator key from the Azure Portal - in your translator resource, look at 'Keys and Endpoint'

#define TRANSLATOR_TOKEN_LENGTH	1024
static char translatorToken[TRANSLATOR_TOKEN_LENGTH] = { 0 };
static char* tokenURL = "https://api.cognitive.microsoft.com/sts/v1.0/issuetoken";
static char* detectLanguageURL = "https://api.cognitive.microsofttranslator.com/detect?api-version=3.0";
static char* translateLanguageURL = "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0";

static char jsonBuffer[256] = { 0 };

static char* HttpPost(char* postURL, char* data);
static int GetTranslatorToken(void);

// Curl stuff.
struct url_data {
	size_t size;
	char* data;
};

CURL* curl;
CURLcode res;

struct url_data postData;

static size_t write_data(void* ptr, size_t size, size_t nmemb, struct url_data* data)
{
	size_t index = data->size;
	size_t n = (size * nmemb);
	char* tmp;

	data->size += (size * nmemb);

	tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

	if (tmp) {
		data->data = tmp;
	}
	else {
		if (data->data) {
			free(data->data);
		}
		return 0;
	}

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return size * nmemb;
}

int Translator_DetectLanguage(char* inputString, char* detectedLanguage, size_t detectedLanguageLen)
{
	if (strlen(translatorToken) == 0)
	{
		int result = GetTranslatorToken();
		if (result == -1)
			return -1;
	}

	snprintf(jsonBuffer, 256, "[{ \"Text\": \"%s\" }]", inputString);

	char *data = HttpPost(detectLanguageURL, jsonBuffer);
	if (data == NULL)
		return -1;

	// get the language type.
	size_t i = 0;
	JSON_Value* root_value = json_parse_string(data);
	JSON_Array* langInfo = json_value_get_array(root_value);
	JSON_Object* langObject = json_array_get_object(langInfo, i);	// get first element
	const char* lang = json_object_dotget_string(langObject, "language");
	strncpy(detectedLanguage, lang, detectedLanguageLen);
	json_value_free(root_value);
	free(data);

	return 0;
}

int Translator_Translate(char* inputString, char* FromLang, char* outputString, size_t outputStringLen, char* ToLang)
{
	if (strlen(translatorToken) == 0)
	{
		int result = GetTranslatorToken();
		if (result == -1)
			return -1;
	}

	char translateURL[260] = { 0 };
	snprintf(translateURL, 260, "%s&from=%s&to=%s", translateLanguageURL, FromLang, ToLang);

	char jsonBuffer[256] = { 0 };
	snprintf(jsonBuffer, 256, "[{ \"Text\": \"%s\" }]", inputString);

	char* data = HttpPost(translateURL, jsonBuffer);

	if (data == NULL)
		return -1;

	size_t i = 0;
	JSON_Value* root_value = json_parse_string(data);
	JSON_Array* translationArray = json_value_get_array(root_value);
	JSON_Object* langObject = json_array_get_object(translationArray, i);	// get first element
	JSON_Array* translations = json_object_dotget_array(langObject, "translations");
	JSON_Object* translation = json_array_get_object(translations, i);	// get first element
	const char* translatedText = json_object_dotget_string(translation, "text");
	strncpy(outputString, translatedText, outputStringLen);
	free(data);
	json_value_free(root_value);

	return 0;
}

/// <summary>
/// Get a Translator Cognitive Services token (use in detect/translate APIs)
/// </summary>
static int GetTranslatorToken(void)
{
	if (strlen(translatorAPIKey) == 0)
	{
		Log_Debug("ERROR: translator API Key needs to be set (currently empty)\n");
		return -1;
	}

	char *result=HttpPost(tokenURL, NULL);
	if (result == NULL)
	{
		return -1;
	}

	memset(translatorToken, 0x00, TRANSLATOR_TOKEN_LENGTH);
	if (strlen(result) > TRANSLATOR_TOKEN_LENGTH)
	{
		Log_Debug("ERROR: Translator Token length > %d\n", TRANSLATOR_TOKEN_LENGTH);
		free(result);
		return -1;
	}

	strncpy(translatorToken, result, TRANSLATOR_TOKEN_LENGTH);
	free(result);

	return 0;
}

static char * HttpPost(char *postURL, char *data)
{
	if (strlen(translatorAPIKey) == 0)
	{
		Log_Debug("ERROR: You need to set the translatorAPIKey\n");
		return NULL;
	}

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	postData.size = 0;
	postData.data = malloc(1);

	curl_easy_setopt(curl, CURLOPT_URL, postURL);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &postData);

	if (data != NULL)
	{
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
	}

	char authHeader[128] = { 0 };
	snprintf(authHeader, 128, "Ocp-Apim-Subscription-Key: %s", translatorAPIKey);

	struct curl_slist* hs = NULL;
	hs = curl_slist_append(hs, "Content-Type: application/json");
	hs = curl_slist_append(hs, authHeader);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

	/* Perform the request */
	res = curl_easy_perform(curl);
	curl_slist_free_all(hs);

	if (res == CURLE_OK)
	{
		return postData.data;	// caller is responsible for freeing this.
	}

	free(postData.data);
	return NULL;
}
