# Access Azure Sphere Public REST API using AAD Device code flow
This sample code snippet can be used to access Azure Sphere public API using AAD token acquired by AAD Device code flow. This code can be used to acquire token in Web application context.

## Contents

| File/folder | Description |
|-------------|-------------|
| `DeviceCodeModel.cshtml`       | The Razor PageModel |
| `DeviceCodeModel.cshtml.cs` | The PageModel backend. |
| `LICENSE.txt`   | The license for the project. |
| `README.md`   | This README file. |

## Concept
Please refer the following document.
- AAD Device Code Flow [Documentation](https://docs.microsoft.com/azure/active-directory/develop/v2-oauth2-device-code)
- Azure Sphere Public API - [Authentication values](https://docs.microsoft.com/rest/api/azure-sphere/#authentication-library-values)

## How to use
- Copy the razor pages directly in ASP .net application
- Otherwise, use the frontend & backend in the existing application at the corresponding place.

## Project expectations
This code is an illustrative snippet only, it has not been reviewed for production-readiness.

### Expected support for the code
This code is not formally maintained, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue
If you run into an issue with this code, please open a GitHub issue against this repo.