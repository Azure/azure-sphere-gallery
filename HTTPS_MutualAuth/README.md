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

   If you are using a device certificate _chain_ rather than a single certificate, please refer to the instructions below

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
   And delete the `#warning` on line 273.

1. Build and run the project.

   In the device output, you should see a successful HTTP request (with a directory listing from the server)

### Using a device certificate chain

Some configurations may require the use of a certificate chain rather than a single device certificate (for example, if the
device certificate is not signed by the required root authority, but is rather signed by an intermediate authority, which has itself
been signed by the root authority required by the server).

In this case, you cannot use the `curl_easy_setopt(curlHandle, CURLOPT_SSLCERT, clientCertPath);` function to set the client certificate;
rather, you must use `wolfSSL_CTX_use_certificate_chain_file` from within the SSL context callback:

1. At the top of main.c, add:
   ```c
   #include "wolfssl/ssl.h"
   ```

1. Add the following function somewhere above `PerformWebPageDownload()`:
   ```c
   CURLcode wolfssl_ctx_callback(CURL* curlHandle, void* wolfssl_ctx, void* client)
   {
      char* clientCertPath= Storage_GetAbsolutePathInImagePackage("certs/device-cert.pem");
      if (clientCertPath == NULL) {
         Log_Debug("The client certificate path could not be resolved: errno=%d (%s)\n", errno,
                     strerror(errno));
         return CURLE_SSL_CONNECT_ERROR;
      }

      int r = wolfSSL_CTX_use_certificate_chain_file(wolfssl_ctx, clientCertPath);
      if (r != WOLFSSL_SUCCESS) {
         Log_Debug("Error loading cert chain for client\n");
         return CURLE_SSL_CONNECT_ERROR;
      }

      return CURLE_OK;
   }
   ```

1. In `PerformWebPageDownload()`, remove the lines:

   ```c
   clientCertPath= Storage_GetAbsolutePathInImagePackage("certs/device-cert.pem");
   if (clientCertPath == NULL) {
       Log_Debug("The client certificate path could not be resolved: errno=%d (%s)\n", errno,
                 strerror(errno));
       goto cleanupLabel;
   }

   if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSLCERT, clientCertPath)) != CURLE_OK) {
       LogCurlError("curl_easy_setopt CURLOPT_SSLCERT", res);
       goto cleanupLabel;
   }
   ```

   and replace them with:
   
   ```c
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_CTX_FUNCTION, wolfssl_ctx_callback)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_SSL_CTX_FUNCTION", res);
        goto cleanupLabel;
    }
   ```


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

