#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <applibs/storage.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "parson.h"

bool setWiFiConfigurationFromAppResource(void)
{
	int wifiFd = Storage_OpenFileInImagePackage("WiFiConfig.json");
	if (wifiFd == -1)
	{
		Log_Debug("Failed to open Embedded Resource\n");
		return false;
	}

	// load the WiFiConfig

	off_t length = lseek(wifiFd, 0, SEEK_END);	// get the length of the file

	char* wifiConfigJson = (char*)calloc((size_t)length + 1, 1);	// allocate buffer space for the json
	if (wifiConfigJson == NULL)
	{
		Log_Debug("Failed to allocate memory for Embedded Resource\n");
		return false;
	}

	lseek(wifiFd, 0, SEEK_SET);
	read(wifiFd, wifiConfigJson, (size_t)length);
	close(wifiFd);

	Log_Debug("Read: %s\n", wifiConfigJson);

	// now get the ssid and network key.
	JSON_Value* rootValue = json_parse_string(wifiConfigJson);
	JSON_Object* rootObject = json_value_get_object(rootValue);
	const char* ssidString = json_object_get_string(rootObject, "ssid");
	const char* networkKeyString = json_object_get_string(rootObject, "networkKey");

	bool retVal = false;

	if (ssidString != 0x00 && strlen(ssidString) < WIFICONFIG_SSID_MAX_LENGTH
		&& networkKeyString != 0x00 && strlen(networkKeyString) < WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE)
	{
		int networkId = WifiConfig_AddNetwork();
		WifiConfig_SetSSID(networkId, ssidString, strlen(ssidString));
		WifiConfig_SetSecurityType(networkId, WifiConfig_Security_Wpa2_Psk);
		WifiConfig_SetPSK(networkId, networkKeyString, strlen(networkKeyString));
		WifiConfig_SetNetworkEnabled(networkId, true);
		WifiConfig_PersistConfig();
		retVal = true;
	}

	free(wifiConfigJson);

	return retVal;
}
