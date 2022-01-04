# EAP-TLS solution: Azure Sphere client

This is an EAP-TLS Client solution for Azure Sphere-based devices, running an an HL-Core App.

It comprises an HL-App and a specifically-designed EAP-TLS library that fully manages the device's connectivity and will continuously attempt to connect to the configured RADIUS network. The connection will be established through reusable and atomic logic, implemented as a state-machine and documented in following diagram:

[](![EAP-TLS&#32;Client&#32;State-machine](./EAP-TLS&#32;Client&#32;State-machine.png))
<span style="display:block;text-align:center"><img src="./EAP-TLS&#32;Client&#32;State-machine.png" width="80%" style="border-radius:2%"></span>

## Contents

| File/folder | Description |
|-------------|-------------|
| `.vscode` | Contains the `launch.json` and `settings.json` files, that configure Visual Studio Code to use CMake with the correct options, and how to deploy and debug the application. |
| `CMakeLists.txt` | Contains the project information and produces the build, along with the memory-specific wrapping directives. |
| `CMakeSettings.json` | Configures CMake with the correct command-line options. |
| `app_manifest.json` | The sample App's manifest file. |
| `main.c`    | The sample App's source file. |
| `lib\_environment_config.h`    | Header source  file for the global environment variables for the sample App and test code. |
| `lib\_environment_config.c` **NOT PUBLISHED**   | Implementation source file containing the actual instances of the global environment variables for the sample App and test code. **NOTE:** this file is intentionally not included in the repository for security policies (but is already pre-configured in CMakeLists.txt, and a commented template is included for convenience at the bottom of [\lib\_environment_config.h](./lib/_environment_config.h)). See below for more details.|
| `lib\eap_tls_lib.h`    | Header source file for the EAP-TLS library. |
| `lib\eap_tls_lib.c`    | Implementation source file for the EAP-TLS library. |
| `lib\web_api_client.h`    | Header source file for the Web API client library. |
| `lib\web_api_client.c`    | Implementation source file for the Web API client library. |
| `tests\tests.h`    | Header source file for the client library Test suite. |
| `tests\tests.c`    | Implementation source file for the client library Test suite. |
| `README.md` | This readme file. |


## Prerequisites
The solution requires the following assets:

- Azure Sphere-based device.

    **Note:** by default, this solution targets [MT3620 reference development board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) hardware, such as the MT3620 development kit from Seeed Studios. To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see [Use hardware definitions](https://docs.microsoft.com/en-us/azure-sphere/app-development/manage-hardware-dependencies).
- A **RADIUS Client**, a router capable of *"Enterprise/RADIUS"* Wi-Fi, which will act as the access gateway for all devices accessing the EAP-TLS network.
- A **RADIUS Server**, a physical machine or VM which will be authenticating devices accessing the EAP-TLS Wi-Fi network through the RADIUS client.
- Access to an open Wi-Fi WPA2 (Wi-Fi Protected Access II) network, here referred as "**bootstrap network**" and an EAP-TLS network, is required. It is possible to configure the *bootstrap network* to redirect over the Ethernet interface, if available. In this case, no Wi-Fi credentials would be required.
- Make sure to copy the following certificates into the `EAP-TLS Client\certs` folder (all need to be in PEM format), and configure them in the [CMakeLists.txt](./CMakeLists.txt) file as follows:
  - The WebAPI Server's RootCA.
  - *Optionally* (only required to run the *'*pre-provisioned'* and/or the *'authentication failure'* testing scenarios):
    - The RADIUS Server's Root CA certificate.
    - A valid client certificate and its related private key.
    - A non-valid client certificate and its related private key.
  
  **Note**: if you don't have certificates, follow the steps in [How to generate certificates for testing](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Certificates/Cert_HighLevelApp/get-certificates.md) to create the certificates.



### Running the sample EAP-TLS Client HL-App

Before its usage, the client solution needs a few configuration steps to be executed:

1. As a first requirement, make sure to configure, in [lib\eap_tls_lib.h](./lib/eap_tls_lib.h), whether or not the library will use a hard-coded password (in the EapTlsConfig object) for decrypting the Client certificate's private key, or if it should use the one that could be provided through the response from the WebAPI. This is simply done by commenting/uncommenting the following preprocessor definition:

    ```c
    /// <summary>
    ///     Client certificate's Private Key decryption method.
    /// 
    ///     Configure if the library will use a hard-coded password
    ///     (in the EapTlsConfig object) for decrypting the Client
    ///     certificate's private key, or if it should use the one that could
    ///     be provided through the WebAPI.
    /// </summary>
    #define USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI
    ```

2. Enable running the full API tests by uncommenting, in [main.c](./main.c), the following preprocessor definitions, which include if the device needs to be completely reset (i.e. in order to simulate zero-touch provision scenarios):

    ```c
    // Global conditional compiling sections
    #define RUN_TESTS                // Run test cases in tests.c
    #define START_FROM_CLEAN_DEVICE  // This is for testing zero-touch provisioning (when not running tests)
    ```

3. Edit the [lib\\_environment_config.h](./lib/_environment_config.h) source file, and uncomment the WebAPI type to be used (i.e. `WEBAPI_KESTREL` in this self-hosted scenario):
    ```c
    /////////////////////////////////////
    // WebAPI implementation selection //
    /////////////////////////////////////
    //#define WEBAPI_SERVER  // Use 3rd party MDM Server WebApi
    #define WEBAPI_KESTREL   // Use the self-hosted WebApi on a Kestrel web-host
    ```

4. The required `lib\_environment_config.c` file is not included in the repository for security policies (but is already included in CMakeLists.txt, and a template is included for convenience as a comment at the bottom in [lib\\_environment_config.h](./lib/_environment_config.h)). Simply create an empty one in the project's root folder, containing the following constant definitions to reflect your network/certificates setup, as described in the [Prerequisites](#Prerequisites) section above:

    |Variable|Purpose|
    |-|-|
    |`const char *const g_webApiInterfaceUrl = "https://192.168.0.2:44378/api/certs";`|Change `https://192.168.0.2:44378/api/certs` to the endpoint URL exposed by the WebAPI service.
    |`const char *const g_webApiRootCaCertificatePath = "certs/az-CA.pem";`|Change `certs/az-CA.pem` to reflect relative path and file name of the *public* part of the root CA certificate, with which the client will authenticate the WebAPI service.
    |`const char *const bootstrapNetworkName = "cfgBootstrap";`|Change `cfgBootstrap` to the desired configuration name  to be assigned to the Wi-Fi network that has internet access.
    |`const char *const g_bootstrapNetworkSsid = "<Your Wi-Fi SSID>";`|Change `<Your Wi-Fi SSID>` with the Bootstrap Wi-Fi's SSID (if Wi-Fi is used). |
    |`const char *const g_bootstrapNetworkPassword = "<Your Wi-Fi password>";`|Change `<Your Wi-Fi password>` with the Bootstrap Wi-Fi password. As mentioned above, if the *bootstrap network* is over Ethernet, a valid password is not required. |
    |`const char *const g_eapTlsClientPrivateKeyPassword = "<Optional client-key password>";`|If the client key is encrypted, this variable should contain the password for decrypting it, otherwise it simply needs to be empty. **Note**: this would normally be returned by the WebAPI and will be stored in Mutable storage as a DeviceConfiguration object, therefore initializing it would be for testing purposes only. |
    |`const char *const g_eapTlsNetworkName = "cfgRADIUS";`|Change `cfgRADIUS` to the desired network configuration name to be assigned to the EAP-TLS Wi-Fi network (aka RADIUS network). |
    |`const char *const g_eapTlsNetworkSsid = "<Your RADIUS Wi-Fi SSID>";`|Change `<Your RADIUS Wi-Fi SSID>` to the SSID of the EAP-TLS Wi-Fi network (aka RADIUS network). **Note**: this would normally be returned by the WebAPI and will be stored in Mutable storage as a DeviceConfiguration object, therefore initializing it would be for testing purposes only. |
    |`const char *const g_eapTlsRootCaCertificateId = "RadiusRootCA";`|Change `RadiusRootCA` to the desired identifier for the root CA certificate. |
    |`const char *const g_eapTlsRootCaCertificateRelativePath = "certs/az-CA.pem";`|Change `certs/az-CA.pem` to reflect relative path and file name of the *public* part of the RootCA certificate. |
    |`const char *const g_eapTlsClientCertificateId = "RadiusClient";`|Change `RadiusClient` to the desired identifier for the client certificate. |
    |`const char *const g_eapTlsClientIdentity = "iotuser@azsphere.com";`|Change `iotuser@azsphere.com` to the client identity associated to the device. **Note**: this would normally be returned by the WebAPI and will be stored in Mutable storage as a DeviceConfiguration object, therefore initializing it would be for testing purposes only. |
    |`const char *const g_eapTlsClientCertificateRelativePath = "certs/iotuser_public.pem";`|Change `certs/iotuser_public.pem` to reflect relative path and file name of the *public* part of the client certificate.  **Note**: this would normally be returned by the WebAPI and will be stored in Mutable storage as a DeviceConfiguration object, therefore initializing it would be for testing purposes only.|
    |`const char *const g_eapTlsClientPrivateKeyRelativePath = "certs/iotuser_private.pem";`|Change `certs/iotuser_private.pem` to reflect relative path and file name of the *private key* of the client certificate.  **Note**: this would normally be returned by the WebAPI and will be stored in Mutable storage as a DeviceConfiguration object, therefore initializing it would be for testing purposes only.|
     |`const char *const g_eapTlsClientPrivateKeyPassword = "<optional private key password>";`|If  `USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI` is **not** defined, set this value to reflect the password to be used for decrypting the client certificate's private key. |
    #
    **NOTE**: all the above certificate names refer to the sample certificates described in the in [Prerequisites](#Prerequisites) section above.

5. In [tests\tests.c](./tests/tests.c), modify the following constant definitions to reflect your network/certificates setup, as described in the [Prerequisites](#Prerequisites) section above:

    |Variable|Purpose|
    |-|-|
    |`const char *const g_eapTlsInvalidRootCaCertificateRelativePath = "certs/bad-az-CA.pem";`|Change `certs/bad-az-CA.pem` to reflect relative path and file name of the *public* part of a RootCA certificate, with which the client will **not** be able to be authenticated by the RADIUS server, which will be used to test an authorization failure.|
    |`const char *const g_eapTlsInvalidClientIdentity = "extuser@azsphere.com";`|Change `extuser@azsphere.com` to a client identity that is the one to which the `extuser_public.pem` and `certs/extuser_private.pem` were issued to, which will be used to test an authorization failure. |
    |`const char *const g_eapTlsInvalidClientCertificateRelativePath = "certs/extuser_public.pem";`|Change `certs/extuser_public.pem` to reflect relative path and file name of the *public* part of the client certificate, which will be used to test an authorization failure. |
    |`const char *const g_eapTlsInvalidClientPrivateKeyRelativePath = "certs/extuser_private.pem";`|Change `certs/extuser_private.pem` to reflect relative path and file name of the *private key* of the client certificate, which will be used to test an authorization failure. |
    #


### Using the client EAP-TLS library standalone

In the [tests\tests.c](./tests/tests.c) source file, the `TestEapTlsLib_InitializeConfiguration` function shows how to set up the *bootstrap network* (over Wi-Fi or Ethernet) and how to fully initialize a local `struct` variable of type `EapTlsConfig`, with which many of the `EapTls_*` APIs work with.
Although, not all the fields in the `EapTlsConfig` structure are mandatory, therefore within a production application's code, make sure to define at least the following members, in a local variable of type `EapTlsConfig`:

|`EapTlsConfig` member|Purpose|
|-|-|
|`bootstrapNetworkInterfaceType`|Must contain the network interface to which the bootstrap network is exposed. Values must be among the ones defined in the `NetworkInterfaceType` enum type, declared in in [lib\eap_tls_lib.h](./lib/eap_tls_lib.h).|
|`bootstrapNetworkName`|Must contain the desired configuration to be assigned to the Wi-Fi network that has internet access (not required if the *bootstrap network* is over Ethernet).
|`bootstrapNetworkSsid`|Must contain the SSID of the Wi-Fi network that has internet access (not required if the *bootstrap network* is over Ethernet).
|`mdmWebApiInterfaceUrl`|Must contain the URL endpoint exposed by the WebAPI service.|
`mdmWebApiRootCertificate.relativePath`|Must contain the relative path of the WebAPI service's pre-provisioned certificate, for proper TLS server-side validation.|
|`eapTlsNetworkName`|Must contain the desired configuration name to be assigned to the EAP-TLS Wi-Fi network (aka RADIUS network).|
|`eapTlsRootCertificate.id`|Must contain the desired identifier of the RADIUS Server's RootCA certificate which the client, Azure Sphere, will use to authenticate the RADIUS Server.|
|`eapTlsClientCertificate.id`|Must contain the desired identifier of the client certificate.|
#

The App can then simply call the `EapTls_RunConnectionManager` API and pass a local `EapTlsConfig` structure to initiate and establish the connection to the EAP-TLS network with the configured RADIUS network.

The solution also includes a number of network and CertStore helpers, to easily manipulate network configurations and certificates, should the developer need to implement specific custom logic.

## Further references

- [Secure enterprise Wi-Fi access: EAP-TLS on Azure Sphere](https://techcommunity.microsoft.com/t5/internet-of-things/secure-enterprise-wi-fi-access-eap-tls-on-azure-sphere/ba-p/1506375)
- [Manage certificates in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/certstore)
- [Using EAP-TLS with Azure SPhere](https://docs.microsoft.com/en-us/azure-sphere/network/eap-tls-overview)


## Project expectations

This EAP-TLS Client implementation can be used to accelerate the development of Azure Sphere applications that rely on an EAP-TLS connection. The included Test Suite showcases how to establish connections and handle issues, while the main application implements the core tasks to establish a connection and keep it running by automatically retrying upon disconnections.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this library, please open a GitHub issue within this repo.

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

For information about the licenses that apply to this project, see [LICENSE.txt](../LICENSE.txt)