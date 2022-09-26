# MutableStorageKVP

MutableStorageKVP provides a set of functions that expose Key/Value pair functions (write, read, delete) over Azure Sphere [Mutable Storage](https://learn.microsoft.com/en-us/azure-sphere/app-development/storage#using-mutable-storage).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Header and Source file for MutableStorageKVP |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

You will need an Azure Sphere development board, Visual Studio/Visual Studio Code, and the latest Azure Sphere SDK.

Your Azure Sphere development board will need development enabled `azsphere device enable-development`

## How to use

The header exposes three APIs:


`bool WriteProfileString(char *keyName, char *value);`

`ssize_t GetProfileString(char *keyName, char *returnedString, size_t Size);`

`bool DeleteProfileString(char *keyName);`

To use these functions you will need to allocate some Mutable Storage space for your application, this can be set in your projects `app_manifest.json` (example below).

```json
{
  "SchemaVersion": 1,
  "Name": "Sample",
  "ComponentId": "96CD41BD-A649-4982-86EC-DFDA354D524C",
  "EntryPoint": "/bin/app",
  "Capabilities": {
    "MutableStorage": {
      "SizeKB": 32
    }
  },
  "ApplicationType": "Default"
}
```

Using the functions is fairly straight forward, let's query for a non-existent Key/Value pair, create a Key/Value Pair, read the Key/Value pair, then delete the Key/Value pair.

```cpp

    char buffer[20];
    ssize_t valueLength = 0;

    // Delete Mutable file to start clean.
    // #include <applibs/storage.h>
    Storage_DeleteMutableFile();

    // try to read a key/value that does not exist.
    valueLength = GetProfileString("Foo", &buffer[0], 20);
    if (valueLength == -1) {
        Log_Debug("The Key/Value pair 'Foo' doesn't exist\n");
    }

    // Create the Key/Value Pair
    bool writeOK = WriteProfileString("Foo", "Bar");
    if (!writeOK) {
        Log_Debug("Failed to Write Key/Value Pair\n");
    }

    // Now read the value we've just written
    valueLength = GetProfileString("Foo", &buffer[0], 20);
    if (valueLength != -1) {
        Log_Debug("Foo:%s\n", buffer);
    }

    // Now delete the Key Value Pair.
    bool deleteOk = DeleteProfileString("Foo");
    if (!deleteOk) {
        Log_Debug("Failed to delete key\n");
    }

```

Your application will need to `#include "MutableStorageKVP.h`, and your CMakeLists.txt will need to include `MutableStorageKVP.c` and `cJSON\cJSON.c`

The functions only deal with strings, you can easily wrap other variable types by converting to/from string.

## Project expectations

* This is a set of helper functions for developers; it is not official, maintained, or production-ready code.
* This sample has not been written to consider flash wear issues.

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
