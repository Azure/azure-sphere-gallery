# DeviceTenantFinder

DeviceTenantFinder is a utility that makes it easy to find the Azure Sphere tenant name/Id within your tenants for a given Device Id. The utility is written in C#, based on .NET Core 3.1.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\DeviceTenantFinder`       | DeviceTenantFinder utility |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

You should have at least one [Azure Sphere tenant](https://docs.microsoft.com/en-us/azure-sphere/deployment/create-tenant), and one or more devices claimed into your tenants.

## How to use

DeviceTenantFinder is a console application, the application takes one command line argument `device id`, which can be obtained using the Azure Sphere CLI command `azsphere dev show-attached`.

* Build and run the `DeviceTenantFinder` application on your PC

You will be prompted to login using your Azure Sphere tenant credentials

If found the utility will display the Device Id, Tenant Id, Tenant Name, Product Id (if the device is linked to a product, or 'None'), and Device Group Id.

```dos
DeviceTenantFinder device-id
Device Found   : device-id
Tenant Id      : tenant-id
Tenant Name    : tenant-name
Product Id     : product-id
Device Group Id: device-group-id
```

## Project expectations

* The utility has been developed to support developers using devices across multiple Azure Sphere tenants.

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
