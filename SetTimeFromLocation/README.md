# SetTimeFromLocation

Some IoT devices need to know the current time for the device's location (including appropriate daylight savings offsets). For example, a washing machine might want to display to the user the time that a running program would complete. This project demonstrates how to obtain a device location using Reverse IP lookup, use the device location to obtain the current local time, and then set the device system time.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Sample application |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

You will need an Azure Sphere development board, Visual Studio/Visual Studio Code, and the latest Azure Sphere SDK.

Your Azure Sphere development board will need development enabled `azsphere device enable-development`

## How to use

The project uses two online services with REST APIs to obtain location data and to get local time based on location. There are many online services with similar REST APIs that can be used to achieve the same result. The services used in this project are there as an example rather than a recommendation.

The two REST services used in this project are:

* https://www.geojs.io/ - Reverse IP lookup
* https://timezonedb.com/ - Get time information for location (requires an API Key)

The project implements a simple HTTP GET function called `getHttpData` (implemented in httpGet.h/.c) which takes a URL as a parameter, and returns a `char *` string (which will need to be parsed), or NULL in the case of failure to obtain the data. The function that calls the `getHttpData` is responsible for freeing the pointer returned from `getHttpData`.

Note that `main( )` will wait until your board has a network connection before getting location/time data.

To obtain location data there is a function called `GetLocationData` (implemented in location_from_ip.h/.c), which returns a `location_info` structure containing the country code, latitude, and longitude, or NULL if the data couldn't be retrieved.

To obtain current time for a latitude/longitude location there is a function called `SetLocalTime` (implemented in settime.h/.c) that requires latitude and longitude. 

**Note**: You will need to obtain an API Key from https://timezonedb.com for this function to work.  In settime.c the URL template has a placeholder `<YOUR_API_KEY_HERE>` which will need to be replaced by your timezonedb.com API key.

Setting the system time requires that you have enabled the `SystemTime` capability in the app_manifest.json for your application. You also need to include the two REST API services that the application uses in the `AllowedConnections` list. A sample app_manifest.json is below:

```json
{
  "SchemaVersion": 1,
  "Name": "SetTimeFromLocation_HighLevelApp",
  "ComponentId": "6105E5DA-30C9-49ED-B5A1-2D55B61EE78B",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "AllowedConnections": [ "api.timezonedb.com", "get.geojs.io" ],
    "SystemTime": true
  },
  "ApplicationType": "Default"
}

```

The snippet below is a simplified version of the sample project.

```cpp
#include "location_from_ip.h"
#include "settime.h"

void main(void)
{
    struct location_info *locInfo = GetLocationData();
    if (locInfo != NULL)
    {
        SetLocalTime(locInfo->lat, locInfo->lng);
    }    
}

```

You may want to consider calling the APIs at an appropriate frequency to ensure that daylight savings changes are reflected correctly for the device.

More information on managing system time and use of the Real Time Clock can be found in the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/app-development/rtc).

## Project expectations

* SetTimeFromLocation has been written to show how to set a device system time based on location; it is not official, maintained, or production-ready code.

### Expected support for the code

This code is not formally maintained, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this code, please open a GitHub issue against this repo.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

See [LICENSE.txt](./LICENSE.txt)
