# MQTT project

The goal of this project is to show how to add MQTT support to an Azure Sphere project, in this case using the [Eclipse Mosquitto MQTT Broker](https://mosquitto.org/) and a PC-based Python application.

## Contents

| Folder\file | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source code |
| `src\HighLevelApp\Certs`       | placeholder folder for MQTT certs |
| `src\PyMqttHost`       | Python app that subscribes and publishes messages to a device.  |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).
- Install Python 3 from [Python.org](www.python.org).
- Install the ```paho-mqtt``` library using ```pip3 install -r requirements.txt```.


## How the sample works

This sample is made up of two applications, an Azure Sphere MQTT Client application and a Python MQTT host application. The sample uses the public **mosquitto.org** MQTT broker.

1. On startup, the Azure Sphere MQTT-C client connects to the mosquitto.org MQTT broker and subscribes to MQTT messages on topic **azuresphere/sample/device**. Then every second, the app publishes a message on topic **azuresphere/sample/host**.
2. The Python host application connects to the public mosquitto.org MQTT broker and subscribes to messages on topic **azuresphere/sample/host**. When a message published by the Azure Sphere mqtt client arrives on the **azuresphere/sample/host** topic the Python app rotates the message left by one character and publishes the updated message on topic **azuresphere/sample/device**.
3. The Azure Sphere MQTT client subscribed to messages on topic **azuresphere/sample/device**, it receives the updated message published by the Python app. The Azure Sphere then publishes the updated message back on to topic **azuresphere/sample/host**.
4. And the cycle repeats every one second. From the Azure Sphere debug output window, you can view the echoed message received from the Python app being shifted left one character at a time.


## Mosquitto Certificates

This MQTT sample is going to connect to the [mosquitto.org](https://mosquitto.org) public test MQTT broker. For the Azure Sphere MQTT and Python host MQTT apps to securely connect to the mosquitto.org broker you need to download and copy the ```mosquitto.org``` CA certificate and generated client certificates into the Azure Sphere application **MQTT-C_Client\src\HighLevelApp\Certs** folder.

When you have completed the certificate step you should have **three** certificate files in the **MQTT-C_Client\src\HighLevelApp\Certs** folder. 

They will be named:

1. mosquitto.org.crt
2. client.key
3. client.crt

### Install openssl

For Linux users, you may need to install **openssl** if it is not installed by default. You can install openssl using your Linux distribution's package manager. For example, on Ubuntu ```sudo apt-get install openssl```.

For Windows users either download ```openssl``` from [openssl.org](https://wiki.openssl.org/index.php/Binaries), or use [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10) (Windows Subsystem for Linux). Depending on the WSL Linux distribution you installed you may need to install **openssl**. You can install openssl using your Linux distribution's package manager. For example, on Ubuntu ```sudo apt-get install openssl```.

### Download the Mosquitto Certificate Authority certificate

Download the Mosquitto Certificate Authority certificate [mosquitto.org.crt (PEM format)](https://test.mosquitto.org/) and save it in the MQTT-C_Client\src\HighLevelApp\Certs folder.

### Generate the client certificates with openssl

1. Generate a Certificate Signing Request (CSR) using the openssl utility. Generate a private key:

    ```bash
    openssl genrsa -out client.key
    ```

2. Generate the CSR:

    ```bash
    openssl req -out client.csr -key client.key -new
    ```

3. Open the client.csr with your favorite text editor and copy and paste the CSR into the [Generate a TLS client certificate form](https://test.mosquitto.org/ssl/). After you submit the form, the certificate will be generated for you to download. The certificates are valid for 90 days.

4. Save the ```client.key``` and the downloaded ```client.crt``` into the MQTT-C_Client\src\HighLevelApp\Certs folder

### Python MQTT Host app

The project contains a Python application `PyMqttHost.py` that supports two channels (azuresphere/sample/host and azuresphere/sample/device) one is used by the device to send messages to the host, the other is used by the host to send messages back to the device - feel free to modify the channel names, note that you will need to change the channel names in the HighLevelApp and the Python app.

Start the Python application before running the Azure Sphere application.

### Azure Sphere high-level application

The high-level application will connect to the Mosquitto broker and send a message - the host application will receive the message, left shift the message by one character, and return the message to the high-level application. You should see the high-level application output in the Visual Studio/Code debug output window.

### Project expectations

The code has been developed to show how to integrate MQTT into an Azure Sphere project -  It is not official, maintained, or production-ready code.

### Expected support for the code

This code is not formally maintained, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this code, please open a GitHub issue against this repo.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

See [LICENSE.txt](./LICENCE.txt)

- MQTT-C is included via a submodule. The license for MQTT-C can be [found here](https://github.com/LiamBindle/MQTT-C/blob/master/LICENSE).
- AzureSphereDevX is include via a submodule. The license for AzureSphereDevX can be [found here](https://github.com/gloveboxes/AzureSphereDevX/blob/main/LICENSE).