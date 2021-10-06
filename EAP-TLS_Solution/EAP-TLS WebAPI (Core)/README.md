# EAP-TLS solution: WebAPI Server

This is an EAP-TLS WebAPI Server solution, developed on top of the [ASP.NET Core Framework 3.1](https://docs.microsoft.com/en-us/aspnet/core/?view=aspnetcore-3.1), which is "acting-as" the publicly-exposed endpoint to which Clients (i.e. Azure Sphere devices) can connect for their first provisioning and/or certificate renewals. It substantially mimics an MdM Server, and it's internals showcase to developers how to extract Azure Sphere's DAA certificate directly from the TLS handshaking, without any pre-provisioning.

## Prerequisites

- The web server can be hosted on a dedicated machine, or self-hosted for easy development purposes. When self-hosted, the WebAPI's endpoint, `api/certs`, will be located at the Host machine's IP Address, typically on port 44378 (i.e. `https://192.168.0.2:44378/api/certs`, if the development Host machine's IP address is `192.168.0.2`).
- The Host machine must be accessible from the devices that will be running the [Azure Sphere EAP-TLS&#32;Client](..\EAP-TLS&#32;Client) solution.
- A test-WebAPI private certificate, in **PFX** format (i.e. `eap-tls-webapi.pfx`).
- A test-RADIUS RootCA public certificate, in **PEM** format (i.e. `az-CA.pem`), for authenticating RADIUS Server.
- A test-RADIUS Client private certificate, split in two different files, both in **PEM** format: the *public* portion (i.e. `iotuser_public.pem`) and the *private key* portion (i.e. `iotuser_private.pem`).

  **Note**: if you don't have certificates, follow the steps in [How to generate certificates for testing](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/Certificates/Cert_HighLevelApp/get-certificates.md) to create the certificates.

## Configuring the WebAPI Server

For demo purposes, the WebAPI Server handles just one Client, while a full solution should rely on a proper CMS (Certificate Management System) to identify multiple incoming device IDs, and issue the corresponding certificates and identities. See the WebAPI's controller source code ([CertsController.cs](.\/Controllers/CertsController.cs)) for full details and recommendations within the comments.

Make sure to:

- Copy th following certificates in the [EAP-TLS&#32;WebAPI&#32;(Core)\certs](.\EAP-TLS&#32;WebAPI&#32;(Core)/certs) folder:
  - The WebAPI's private certificate, in PFX format (i.e. `eap-tls-webapi.pfx`).
  - The RADIUS public RootCA certificate, in PEM format (i.e. `az-CA.pem`).
  - The RADIUS public Client certificate and related private key, both in PEM format (i.e. `iotuser_public.pem`).
- In order for the DAA process to succeed, on the machine in which the WebAPI will be running, you must download and register (*) the public certificate of the AS3 Tenant in which the Azure Sphere device was claimed in:

    ```cmd
    azsphere tenant download-ca-certificate-chain --output CA-cert-chain.p7b
    ```

    (*) See [Installing the trusted root certificate](https://docs.microsoft.com/en-us/skype-sdk/sdn/articles/installing-the-trusted-root-certificate) for full instructions on how to install the resulting *CA-cert-chain.p7b* in your machine's *Current User/Trusted Certificate Authorities*.

- Configure all the certificate file names, WiFi SSID, WebAPI's certificate password and client identity within the [appsettings.json](.\appsettings.json) configuration file:

  ```json
  {
    ...

    "certificateStorePath": "certs",
    "webApiPrivateCertificateFileName": "eap-tls-webapi.pfx",
    "webApiPrivateKeyPassword": "1234",
    "radiusRootCAPublicCertificateFileName": "az-CA.pem",
    "eapTlsNetworkSsid": "GTsRADIUS",
    "clientIdentity": "iotuser@azsphere.com",
    "clientPublicCertificateFileName": "iotuser_public.pem",
    "clientPrivateKeyFileName": "iotuser_private.pem",
    "clientPrivateKeyPassword": "<if not provisioned on the device>",
    ...
  }
  ```

## Executing the self-hosted WebAPI Server

Simply run the project from Visual Studio 2019.

## Further references

- [Kestrel web server implementation in ASP.NET Core](https://docs.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-3.1).


## Project expectations

This WebAPI should be used as a sample only, and is designed to highlight the key steps that a production WebAPI (or 3rd party product) should perform is order to:

- Successfully perform the TLS connection with Azure Sphere devices.
- Extract the Device ID of the Azure Sphere device that has established the TLS connection.
- Initiate the backend verifications with the Corporate's IT Services, in order to issue a client certificate and other required metadata that a production architecture will require.

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

For information about the licenses that apply to this sample project, see [LICENSE.txt](..\LICENSE.txt)
