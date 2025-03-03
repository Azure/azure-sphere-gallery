# HTTPS Mutual Auth

This project demonstrates HTTPS mutual auth with cURL on an Azure Sphere device using custom certificates for device authentication (rather than Azure Sphere device certificates).

It is based on the [HTTPS_Curl_Easy](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/HTTPS/HTTPS_Curl_Easy) sample from the [azure-sphere-samples](https://github.com/Azure/azure-sphere-samples/) repository. Please refer to the [README](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/HTTPS/HTTPS_Curl_Easy/README.md) from that project for detailed instructions.

A simple Python script is provided to run an https server using the generated server cert/key pair.

## Setup

This project uses Python 3, OpenSSL and bash - it is easiest to build and run in a Linux environment; it may work in other environments but is untested.

Please refer to the HTTPS_Curl_Easy sample for detailed setup instructions.

1. Copy your certs and keys as follows (or use the `make-certs.sh` script to generate test certs and keys - see below)

   - `Azsphere/certs/ca-bundle.pem` - a bundle containing the CA chain the device will use to verify the server
   - `Azsphere/certs/device-cert.pem` - the device certificate
   - `Azsphere/certs/device-key.pem` - the device private key

   - `server-certs/rootca-cert.pem` - a bundle containing the CA cert chain for verifying the device certificate
   - `server-certs/server-cert.pem` - the server certificate
   - `server-certs/server-key.pem` - the private key for the server certificate


1. Launch the server:
   ```
   ./https-server.py
   ```

   - (Optional) Test the server:
     ```
     ./make-request.sh <SERVER_ADDRESS>
     ```
     This will make three requests to the server - one with both the device certs and CA bundle configured (which should succeed), one with only the device certs (which should fail) and one with only the CA bundle (which should also fail).

1. Update the **AllowedConnections** capability of the [app_manifest.json](https://learn.microsoft.com/azure-sphere/app-development/app-manifest) file to allow the device to talk to the server. For example, if the server is running on `192.168.1.1`:

    ```json
    "Capabilities": {
        "AllowedConnections": [ "192.168.1.1" ],
      },
    ```

1. Modify `main.c` to point to your server: change line 274, for example:
   ```c
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, "https://192.168.1.1:5000/")) != CURLE_OK) {
   ```
   And delete the `#warning` on line 263.

1. Build and run the project.

   In the device output, you should see a successful HTTP request (with a directory listing from the server)

### Generating test certificates

The script `make-certs.sh` builds a certificate chain as follows:

```
Root CA -----> Intermediate CA -----> Server Certificate
          \
           \----> Device Certificate
```

Invoke it with the address of the HTTPS server you will use, for example:

```
./make-certs.sh 192.168.1.1
```

