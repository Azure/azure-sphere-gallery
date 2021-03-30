/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "MutableStorageKVP.h"
#include <applibs/storage.h>
#include <string.h>
#include "cJSON.h"
#include <errno.h>

ssize_t getStorageString(char **jsonString);
bool writeJsonToStorage(cJSON *cJson);

/// <summary>  
///  Writes Key/Value pair (keyName, String) into Mutable storage (JSON).
///  Returns 'true' on success, 'false' on failure.
/// </summary>
bool WriteProfileString(char *keyName, char *value)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	char *jsonString = 0x00;
	ssize_t length = getStorageString(&jsonString);

	if (length == -1)
	{
		Log_Debug("Error: writing to mutable storage: errno %d\n", errno);
		return false;
	}

	cJSON *cJson = NULL;
	if (length == 0) // nothing in storage.
	{
		cJson = cJSON_CreateObject();
		cJSON_AddStringToObject(cJson, keyName, value);
	}
	else
	{
		// convert to JSON, read the element.
		cJson = cJSON_Parse(jsonString);

		cJSON *pItem = cJSON_GetObjectItemCaseSensitive(cJson, keyName);
		if (pItem == NULL)	// don't have the item in the JSON
		{
			cJSON_AddStringToObject(cJson, keyName, value);
		}
		else
		{
			cJSON *pNewItem = cJSON_CreateString(value);
			cJSON_ReplaceItemInObjectCaseSensitive(cJson, keyName, pNewItem);
		}
	}

	writeJsonToStorage(cJson);

	cJSON_Delete(cJson);
	free(jsonString);

	return true;
}

bool DeleteProfileString(char *keyName)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	char *jsonString = 0x00;
	ssize_t length = getStorageString(&jsonString);

	// nothing in storage, bail
	if (length == -1)
		return false;

	// convert to JSON, read the element.
	cJSON *cJson = cJSON_Parse(jsonString);

	bool ret = false;
	cJSON *pItem = cJSON_GetObjectItemCaseSensitive(cJson, keyName);
	if (pItem != NULL)	// item is in the JSON
	{
		cJSON_DeleteItemFromObject(cJson, keyName);
		writeJsonToStorage(cJson);
		ret = true;
	}

	cJSON_Delete(cJson);
	free(jsonString);

	return ret;
}

/// <summary>  
///  Gets Value from storage based on KeyName.
///  returns -1 for error or no matching Key
/// </summary>
ssize_t GetProfileString(char *keyName, char *returnedString, size_t Size)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	char *jsonString = 0x00;
	ssize_t length = getStorageString(&jsonString);

	// Log_Debug("GetProfileString JSON: %s\n", jsonString);

	if (length <= 0)
	{
		Log_Debug("Error: reading mutable storage: errno %d\n", errno);
		return -1;
	}

	ssize_t retVal = -1;
	memset(returnedString, 0x00, Size);

	// convert to JSON, read the element.
	cJSON *cJson = cJSON_Parse(jsonString);
	cJSON *pItem = cJSON_GetObjectItemCaseSensitive(cJson, keyName);
	if (pItem != NULL)
	{
		size_t sLength = strlen(pItem->valuestring);
		if (sLength <= Size)
		{
			strncpy(returnedString, pItem->valuestring, sLength);
			retVal = (ssize_t)sLength;
		}
	}

	cJSON_Delete(cJson);
	free(jsonString);

	return retVal;
}

/// <summary>  
///  Reads JSON string from storage
///  returns -1 for error or length of string on success
///  caller is responsible for releasing allocated memory
/// </summary>
ssize_t getStorageString(char **jsonString)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	int fd = Storage_OpenMutableFile();
	ssize_t ret = -1;
	if (fd != -1)
	{
		off_t length = lseek(fd, 0, SEEK_END);
		*jsonString = (char*)malloc((size_t)length + 1);

#ifdef SHOW_DEBUG_MSGS
		Log_Debug("malloc %s, 0x%lx\n", __func__, jsonString);
#endif

		memset(*jsonString, 0x00, (size_t)length + 1);
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, *jsonString, (size_t)length);
		close(fd);
	}

	return ret;
}

/// <summary>  
///  Writes JSON string to storage (extracts char * from cJSON object)
///  Returns 'true' for success, 'false' for failure.
/// </summary>
bool writeJsonToStorage(cJSON *cJson)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Debug(">>> %s\n", __func__);
#endif

	bool bRet = false;
	Storage_DeleteMutableFile();	// delete original file before writing new file
	int fd = Storage_OpenMutableFile();
	if (fd != -1)
	{
		char *data = cJSON_Print(cJson);
		size_t dataLen = strlen(data);
		lseek(fd, 0, SEEK_SET);
		ssize_t numWritten=write(fd, data, dataLen);
		close(fd);
		free(data);

		if (numWritten == (ssize_t)dataLen)
			bRet = true;
	}

#ifdef SHOW_DEBUG_MSGS
	if (bRet == false)
	{
		Log_Debug("Error: error writing json to mutable storage\n");
	}
#endif

	return bRet;
}
