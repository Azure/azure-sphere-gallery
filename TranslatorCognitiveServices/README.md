# TranslatorCognitiveServices

The goal of this project is to show how to use an Azure Cognitive Service from Azure Sphere

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Azure Sphere Sample App source code |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.04 SDK release.

## How to use

The project demonstrates how to detect text language, and then translate the text to another language (in the project the text is French and is translated to English).

To use the project you will need an Azure account, and have the ability to create an Azure Resource within your Azure Subscription.

You will need to create a `translator` resource within your Azure Subscription - Follow the instructions [here](https://docs.microsoft.com/azure/cognitive-services/Translator/translator-how-to-signup)

Once the `translator` resource is created you will need to get the API Key, this can be obtained from the Azure Portal translator resource page: 

* Click on `Keys and Endpoint` and then copy  `KEY 1` to the clipboard. 
* Open the Azure Sphere application then open `translator.c` - paste the API Key into the `translatorAPIKey` variable.
* Build and Deploy the application (note that the Azure Sphere device needs a network connection for the application to work).

Sample output:

```dos
Application starting.
Input text: Il pleut. Tu devrais prendre un parapluie.
Detected language: fr
Translated Text: It's raining. You should take an umbrella.
```


## Project expectations

* The code has been developed to show the common pattern used to call Azure Cognitive Services from Azure Sphere. The code is not official, maintained, or production-ready.

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

See [LICENSE.txt](./LICENCE.txt)
