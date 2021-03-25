# Access Azure Sphere API using AAD Device code flow
This sample code snippet can be used to access Azure Sphere public API using AAD token acquired by AAD Device code flow. This code can be used to acquire token in Web application context.

## Contents

| File/folder | Description |
|-------------|-------------|
| `DeviceCodeModel.cshtml`       | The Razor PageModel |
| `DeviceCodeModel.cshtml.cs` | The PageModel backend. |
| `README.md`   | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Concept
Please refer the following document.
- AAD Device Code Flow [Documentation](https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-device-code)
- Azure Sphere Public API - [Authentication values](https://docs.microsoft.com/en-us/rest/api/azure-sphere/#authentication-library-values)

## How to use
- Copy the razor pages directly in ASP .net application
- Otherwise, use the frontend & backend in the existing application at the corresponding place.