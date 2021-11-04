## Finding a certificate

Please review [HTTPS cURL Easy](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/HTTPS/HTTPS_Curl_Easy#rebuild-the-sample-to-download-from-a-different-website) for instructions on finding and exporting an HTTPS certificate.

## Adding a new certificate

When a certificate is added, take note of the file name and edit the following files:

1. app_manifest.json: edit `CertPath` in the `CmdArgs`.
2. CMakeLists.txt: edit `azsphere_target_add_image_package` at the bottom of the file
