# Littlefs Encrypted-at-rest Remote Disk project

The goal of this project is to show how to add support for an authenticated, encrypted-at-rest filesystem to
Azure Sphere. It uses littlefs as the file system, and a remote HTTP server as the backing store.

The secrets used for encryption are generated using wolfCrypt's random number generator, and stored to the device's
mutable storage - as this storage is in-package flash, retrieving the key/iv to decrypt the externally-stored data
would require a physical attack on the chip package, or an attack on the running software.

## Authenticated Encryption

Authenticated encryption is a form of encryption that supports the use of additional data to authenticate the
successful decryption of the ciphertext. The generated ciphertext is combined with additional authentication data
(AAD) to produce an authentication tag, which is stored alongside the ciphertext. When decrypting, this same AAD
can be used with the decrypted data and authentication tag to verify the data was correctly decrypted. wolfCrypt
provides a combined ChaCha20-Poly1305 function to encrypt and decrypt data (using ChaCha20 encryption), and to generate
or verify authentication data (using the Poly1305 authentication algorithm).

In the application, encryption is performed at the level of littlefs filesystem blocks. The backing store consists of a
pool of fixed-size blocks, each with an associated metadata block used to hold the authentication tag for the encrypted
block. Each block is encrypted with the generated key/IV values, with the block index used as additional authentication
data (AAD) for the Poly1305 authentication. 

For more information on ChaCha20-Poly1305 encryption and authentication, see
[here](https://en.wikipedia.org/wiki/ChaCha20-Poly1305).

## Other notes

The project is based on the [Littlefs Remote project](https://github.com/Azure/azure-sphere-gallery/tree/main/LittleFs_RemoteDisk)

Here's a link to the [Littlefs project on Github](https://github.com/littlefs-project/littlefs).

## Project Contents

| File/folder | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source code |
| `src\PyDiskHost`       | Python app that hosts a 4MB Remote Disk |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://learn.microsoft.com/azure-sphere/install/overview) for more information).

This project requires the 23.05 or later SDK, and a device running the 23.05 (or later) Azure Sphere OS.

## How to use

### Python 3 Remote Storage app
The project contains a Python 3 Flask application (PyDiskHost.py) that supports 4MB storage (matching the defined storage
layout of the high-level Azure Sphere application). The Python application supports HTTP GET (read), and POST (Write) 
functions. The 4MB of storage is represented by an array of 256byte blocks, each with a 16 byte metadata block used for
the authentication tag.

The storage size is defined on lines 24-27:

```python
DISK_SIZE = 4 * 1024 * 1024
BLOCK_SIZE = 256 
METADATA_SIZE = 16
BLOCK_COUNT = DISK_SIZE // BLOCK_SIZE
```

Ensure you have installed the required dependencies, and start the Python 3 application before running the
Azure Sphere application.

```
pip install -r requirements.txt
python PyDiskHost.py
```

If you are running the Python application on Windows, you may need to open the firewall to allow access - from the
Windows search, type "Allow an app through Windows firewall", select "Change settings" and ensure the appropriate
public/private settings are checked for the Python executable.

### Azure Sphere High Level Application

The high level application sets up the storage size (in `constants.h`) - these values should match those in the Python storage
server:

```cpp
// 4MB Storage.
#define STORAGE_SIZE (4 * 1024 * 1024)
#define STORAGE_BLOCK_SIZE (256)
#define STORAGE_BLOCK_COUNT (STORAGE_SIZE / STORAGE_BLOCK_SIZE)
#define STORAGE_METADATA_SIZE 16
```

These storage block sizes match the littlefs block sizes (defined in `encrypted_storage.c`, lines 28-34), for simplicity.

The code to generate, store and retrieve key/iv values is in `crypt.c`/`.h`. Note that, throughout, the key/IV values are zeroed
out after use. The wolfCrypt `wc_RNG_GenerateBlock` function used is a pseudo-random number generator seeded with hardware-based
entropy.

The LittleFS function implementations for storing and reading blocks are in `encrypted_storage.c` - these assume whole block reads
and writes, and will return an error if this is not the case. Blocks are encrypted/decrypted before writing/reading to storage using
the wolfCrypt `wc_ChaCha20Poly1305_Encrypt` and `wc_ChaCha20Poly1305_Decrypt` functions respectively.

You will need to modify the high level application app_manifest.json to provide the IP address of the PC running the Python 
application - modify **AllowedConnections** to include the PC host IP address, an example is below.

```json
{
  "SchemaVersion": 1,
  "Name": "LittleFs_HighLevelApp",
  "ComponentId": "9fe91e75-94ad-4ca5-a875-1909ee229163",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "AllowedConnections": ["192.168.1.65"],
    "MutableStorage": { "SizeKB": 4 }
  },
  "ApplicationType": "Default"
}
```

You will also need to set the IP address of the PC running the Python application in the projects CMakeLists.txt, modify `line 25` per the example below:

```makefile
# // TODO: define the PC HOST IP Address
add_compile_definitions(PC_HOST_IP="192.168.1.65")
```

The application will initialize and format the storage, create a file, write some test data, rewind the file pointer, read the data and print to the debug console, and then close the file.

When the application runs you should see the following output in the Visual Studio/Code debug window:

```
INFO: No data found in mutable storage; generating key and IV
WARN: Unable to decrypt block 0
WARN: Unable to decrypt block 1
WARN: Unable to decrypt block 0
WARN: Unable to decrypt block 1
C:/Users/chwhitwo/source/accel/azure-sphere-gallery/LittleFs_Encrypted_RemoteDisk/src/HighLevelApp/littlefs/lfs.c:1070:error: Corrupted dir pair at {0x0, 0x1}
Format and Mount
WARN: Unable to decrypt block 1
Open File 'test.txt'
Write to the file
Seek to the start of the file
Read from the file
Read 442 bytes of content = Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute iruredolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat nonproident, sunt in culpa qui officia deserunt mollit anim id est laborum.
Close the file
```

Note that the remote storage begins initialized to zeroes, and as such it cannot be successfully decrypted until it has been written to. Thus, the initial attempt by littlefs
to mount to storage will fail until the storage is formatted. Subsequent runs will not produce these warnings (assuming the storage server is not restarted).

### Curl memory tracing

The application can also track Curl memory allocations, to enable this simply uncomment `line 22` in CMakeLists.txt

```makefile
add_compile_definitions(ENABLE_CURL_MEMORY_TRACE)
```

Enabling Curl memory tracing combined with the [Gallery Heap Tracker project](https://github.com/Azure/azure-sphere-gallery/tree/main/HeapTracker) enables you to analyze memory usage within your application, specifically when using Curl API calls.

With Curl memory tracking enabled you will see several memory tracking Log_Debug messages, this includes output from the following functions (malloc, free, calloc, strdup, realloc).

## Project expectations

* The code has been developed to demonstrate integration of encrypted-at-rest storage using a remote file system into an Azure Sphere project
  It is not official, maintained, or production-ready code.

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

Littlefs is included via a submodule. The license for Littlefs can be [found here](https://github.com/littlefs-project/littlefs/blob/master/LICENSE.md).