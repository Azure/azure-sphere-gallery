# Extensible Authentication Protocol – Transport Layer Security (EAP-TLS) solution with Azure Sphere

This is a library & demo solution implementation for Azure Sphere-based devices connecting to Extensible Authentication Protocol – Transport Layer Security (EAP-TLS) networks. The goal of this solution is to kickstart production solutions leveraging connectivity through the EAP-TLS industrial standard.
It comprises Client and Server projects, working together to perform zero-touch authentication with an existing RADIUS server handling client authentications within an EAP-TLS network (either WiFi or Ethernet).

The overall solution architecture is summarized in the following animated presentation:

**Note**: all the (greyed-out) IT backend components are out of scope of this solution (i.e. CMS, IT Services, Directory, DB and CA).

<span style="display:block;text-align:center">![EAP-TLS&#32;solution](.\EAP-TLS&#32;solution.gif)</span>


and the related network flow is represented in the following diagram:

[](![EAP-TLS&#32;network&#32;flow](.\EAP-TLS&#32;network&#32;flow.png))
<span style="display:block;text-align:center"><img src=".\EAP-TLS&#32;network&#32;flow.png" width="80%" style="border-radius:2%"></span>


## Prerequisites

Please refer to the *Prerequisites* paragraph within each of the following  README.md files:

- [WebAPI Server - README.md](.\EAP-TLS&#32;WebAPI&#32;(Core)\README.md)
- [Azure Sphere EAP-TLS Client - README.md](.\EAP-TLS&#32;Client\README.md)

## Overview of the solution

The solution includes two main components:

1. A self-hosting WebAPI Server (aka Kestrel), written in .NET Core, performing the following tasks:

   - Mutually authenticates with the Azure Sphere client by leveraging the DAA certificate issued by the Azure Sphere Security Services.
   - Validates the Client and RootCA certificates that are sent by the Client.
   - “Virtually” calls a CMS (left to the user’s IT) to retrieve:
     - A new RADIUS private client certificate (public + private key).
     - The client's identity, corresponding to the provided Device ID.
     - Optionally, the related RADIUS RootCA public certificate.
     - Optionally (not currently implemented), the EAP-TLS network's WiFi SSID could also be returned (useful for devices that can be moved across facilities).
   - Sends back the response to the Client as a JSON document with the following structure:

      ```json
      {
          "timestamp":"2020 - 05 - 13T13:24 : 29.4567259 + 00 : 00",
          "rootCACertificate": "-----BEGIN CERTIFICATE-----[BASE64-ENCODED CERTIFICATE]---- - END CERTIFICATE----",
          "clientIdentity": "iotuser@azsphere.com",
          "clientPublicCertificate": "-----BEGIN CERTIFICATE-----[BASE64-ENCODED CERTIFICATE]---- - END CERTIFICATE----",
          "clientPrivateKey": "-----BEGIN RSA PRIVATE KEY-----[BASE64-ENCODED CERTIFICATE]---- - END RSA PRIVATE KEY---- -",
          "clientPrivateKeyPass": "(optional)"
      }
      ```

2. The Client HL-App running on Azure Sphere, including a set of structured EAP-TLS & utility APIs and a Test Suite, that handle:

   - Certificate & CertStore management
   - Wi-Fi configuration management
   - RADIUS network connectivity
   - Cloud/Server connectivity
   - A fully managed & atomic state-machine for connecting to a RADIUS network, including zero-touch provisioning of a device through a bootstrap network

## Further references

- [Tutorial: Host a RESTful API with CORS in Azure App Service](https://docs.microsoft.com/en-us/azure/app-service/app-service-web-tutorial-rest-api)
- [Kestrel web server implementation in ASP.NET Core](https://docs.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-3.1).
- [Secure enterprise Wi-Fi access: EAP-TLS on Azure Sphere](https://techcommunity.microsoft.com/t5/internet-of-things/secure-enterprise-wi-fi-access-eap-tls-on-azure-sphere/ba-p/1506375)
- [Manage certificates in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/certstore)
- [Using EAP-TLS with Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/network/eap-tls-overview)

## Project expectations

Please refer to this same paragraph from each included project's README.md, EAP-TLS WebAPI and Client.

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

For information about the licenses that apply to this solution, see [LICENSE.txt](.\LICENSE.txt)