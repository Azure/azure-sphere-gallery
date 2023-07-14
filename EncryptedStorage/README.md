
# Encrypted Storage

This project shows a simple use of the wolfCrypt ChaCha20-Poly1305 authenticated encryption functions to encrypt/decrypt data
stored on-device in mutable storage.

**This code does not demonstrate security best practises.** In particular, it uses hard-coded encryption keys, and makes no attempt
to prevent sensitive information leaking through uninitialized/uncleared memory. You should **not** use this code as-is.

## Contents

| File/folder | Description |
|-------------|-------------|
| `main.c` | Source code |
| `app_manifest.json`, `CMakeLists.txt`, `CMakePresets.json` | Project files |
| `README.md` | This README file. |
| `LICENSE.txt` | The license for the project. |

## Prerequisites

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quick-starts to set up your Azure Sphere device](https://learn.microsoft.com/en-us/azure-sphere/install/overview) for more information).

## Running the code

The code should build and run without modification.

When run, the code attempts to read a structure, `Storage` (defined in main.c lines 21-25), from the device's mutable storage. If the header
matches the expected header, it proceeds to attempt to decrypt the rest of the structure using the wolfCrypt ChaCha20-Poly1305 decrypt
function. If the data decrypts and authenticates correctly, it will represent a structure containing a count of the number of times the
application has been run. This count is incremented, and the data is re-encrypted and written back to mutable storage.

On first run, you will see output on the debug console as follows:

```
Encrypted storage project
Loading state...
INFO: Storage not initialized
Initializing state to default
State on entry:
Counter = 0
State on exit:
Counter = 1
Saving state...

Child exited with status 0
```

## Key concepts

This project demonstrates the use of the wolfCrypt ChaCha20-Poly1305 authenticated encryption functions. These functions perform
symmetric, authenticated encryption and decryption of data.

Authenticated encryption is a form of encryption that supports the use of additional data to authenticate the
successful decryption of the ciphertext. The generated ciphertext is combined with additional authentication data
(AAD) to produce an authentication tag, which is stored alongside the ciphertext. When decrypting, this same AAD
can be used with the decrypted data and authentication tag to verify the data was correctly decrypted. wolfCrypt
provides a combined ChaCha20-Poly1305 function to encrypt and decrypt data (using ChaCha20 encryption), and to generate
or verify authentication data (using the Poly1305 authentication algorithm).

## Next steps

For more information on ChaCha20-Poly1305 encryption and authentication, see
[here](https://en.wikipedia.org/wiki/ChaCha20-Poly1305).

For an example of using these functions to store data off-device encrypted-at-rest, please see the
[LittleFS Encrypted Remote Disk](../LittleFs_Encrypted_RemoteDisk/) project.

## Project expectations

### **This code is not secure**

In this project, the key, IV and AAD used in encryption/decryption of the data are hard-coded into the application. **You should
not do this in production code**. Key and IV values should be generated using a cryptographically-secure random number generator,
and stored separately from the encrypted data.

The code may also leak secure information. It has not been subject to a security analysis, and is presented purely as-is to demonstrate
the use of the encryption/decryption functions.

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

For details on license, see LICENSE.txt in this directory.
