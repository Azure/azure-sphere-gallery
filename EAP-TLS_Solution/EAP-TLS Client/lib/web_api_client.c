#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <curl/curl.h>

#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <applibs/storage.h>
#include <applibs/certstore.h>
#include <tlsutils/deviceauth_curl.h>

#include "../parson.h"
#include "_environment_config.h"
#include "web_api_client.h"


/// <summary>
/// Boolean string constants for generating JSON documents
/// </summary>
const char *const strTrue = "true";
const char *const strFalse = "false";

/// <summary>
///		JSON tokens returned by the WebAPI response, used for parsing.
///		Note: these definitions must match the ones returned by the WebAPI's JSON response!
/// </summary>
#if defined(WEBAPI_SERVER)
// Add JSON property names, specific to the WebApi's JSON request & response schema.
#endif
#if defined(WEBAPI_KESTREL)
const char *const webApiResponse_Timestamp = "timestamp";
const char *const webApiResponse_RootCACertificate = "rootCACertificate";
const char *const webApiResponse_EapTlsNetworkSsid = "eapTlsNetworkSsid";
const char *const webApiResponse_ClientIdentity = "clientIdentity";
const char *const webApiResponse_ClientPublicCertificate = "clientPublicCertificate";
const char *const webApiResponse_ClientPrivateKey = "clientPrivateKey";
const char *const webApiResponse_ClientPrivateKeyPass = "clientPrivateKeyPass";

/// <summary>
///		WebAPI GET query fields
///		Note: These definitions must match the ones used in the WebAPI's method signature!!!
/// </summary>
const char *const webApi_RootCertificateField = "needRootCACertificate";
const char *const webApi_ClientCertificateField = "needClientCertificate";
#endif

//////////////////////////////////////////////////////////////
// Logging utilities
//////////////////////////////////////////////////////////////
static void LogCurlError(const char *message, CURLcode curlErrCode)
{
	Log_Debug("%s\n(curl err=%d, '%s')\n", message, curlErrCode, curl_easy_strerror(curlErrCode));
}

//////////////////////////////////////////////////////////////
// WebAPI related functions
//////////////////////////////////////////////////////////////
EapTlsResult EapTls_ParseMdmWebApiResponse(MemoryBlock *responseBlock, WebApiResponse *outResponse)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != outResponse && NULL != responseBlock && NULL != responseBlock->data)
	{
		memset(outResponse, 0, sizeof(WebApiResponse));

		size_t nullTerminatedJsonSize = responseBlock->size + 1;
		char *nullTerminatedJsonString = (char *)malloc(nullTerminatedJsonSize);
		if (nullTerminatedJsonString == NULL)
		{
			Log_Debug("ERROR: could not allocate buffer for parsing the WebAPI response.\n");
		}
		else
		{
			// Copy the provided buffer to a null terminated buffer.
			memcpy(nullTerminatedJsonString, responseBlock->data, responseBlock->size);
			nullTerminatedJsonString[nullTerminatedJsonSize - 1] = 0; // Add the null terminator at the end.

			// Parse the response
			JSON_Value *rootProperties = json_parse_string(nullTerminatedJsonString);
			if (rootProperties == NULL)
			{
				Log_Debug("WARNING: Cannot parse the response as JSON content.\n");
			}
			else
			{
				JSON_Object *rootObject = json_value_get_object(rootProperties);
#if defined(WEBAPI_SERVER)
				// TBD, depending on the WebApi's JSON response schema
#endif
#if defined(WEBAPI_KESTREL)
				const char *s = json_object_get_string(rootObject, webApiResponse_Timestamp);
				if (NULL != s)
				{
					strncpy(outResponse->timestamp, s, sizeof(outResponse->timestamp) - 1);
					s = json_object_get_string(rootObject, webApiResponse_RootCACertificate);
					if (NULL != s)
					{
						strncpy(outResponse->rootCACertficate, s, sizeof(outResponse->rootCACertficate) - 1);
						s = json_object_get_string(rootObject, webApiResponse_EapTlsNetworkSsid);
						if (NULL != s)
						{
							strncpy(outResponse->eapTlsNetworkSsid, s, sizeof(outResponse->eapTlsNetworkSsid) - 1);
							s = json_object_get_string(rootObject, webApiResponse_ClientIdentity);
							if (NULL != s)
							{
								strncpy(outResponse->clientIdentity, s, sizeof(outResponse->clientIdentity) - 1);
								s = json_object_get_string(rootObject, webApiResponse_ClientPublicCertificate);
								if (NULL != s)
								{
									strncpy(outResponse->clientPublicCertificate, s, sizeof(outResponse->clientPublicCertificate) - 1);
									s = json_object_get_string(rootObject, webApiResponse_ClientPrivateKey);
									if (NULL != s)
									{
										strncpy(outResponse->clientPrivateKey, s, sizeof(outResponse->clientPrivateKey) - 1);
										s = json_object_get_string(rootObject, webApiResponse_ClientPrivateKeyPass);
										if (NULL != s)
										{
											strncpy(outResponse->clientPrivateKeyPass, s, sizeof(outResponse->clientPrivateKeyPass) - 1);
											iRes = EapTlsResult_Success;
										}
									}
								}
							}
						}
					}
				}
#endif
			}

			if (EapTlsResult_Success != iRes)
			{
				Log_Debug("ERROR parsing response.\n");
			}

		// Release the allocated memory.
		json_value_free(rootProperties);
	}

	// Release the allocated memory.
	free(nullTerminatedJsonString);
}
	else
	{
	Log_Debug("ERROR, bad parameters.\n");
	}

	return iRes;
}
static size_t EapTls_StoreDownloadedDataCallback(void *chunks, size_t chunkSize, size_t chunksCount, void *memBlock)
{
	MemoryBlock *block = (MemoryBlock *)memBlock;
	size_t additionalDataSize = chunkSize * chunksCount;

	block->data = realloc(block->data, block->size + additionalDataSize + 1);
	if (NULL == block->data)
	{
		Log_Debug("Out of memory, realloc returned NULL: errno=%d (%s)'n", errno, strerror(errno));
		//abort();
		return 0;
	}
	else
	{
		memcpy(block->data + block->size, chunks, additionalDataSize);
		block->size += additionalDataSize;
		block->data[block->size] = 0; // Ensure the block of memory is null terminated.
	}

	return additionalDataSize;
}
static CURLcode EapTls_DeviceAuth_CurlSslFunc(CURL *curl, void *sslctx, void *userCtx)
{
	DeviceAuthSslResult err = DeviceAuth_SslCtxFunc(sslctx);
	switch (err)
	{
		case DeviceAuthSslResult_Success:
			Log_Debug("DeviceAuthSslResult_Success (%d)\n", err);
			break;
		case DeviceAuthSslResult_GetTenantIdError:
			Log_Debug("DeviceAuthSslResult_GetTenantIdError (%d - '%s')\n",
				err,
				"Failed to access the current application's tenant id");
			break;
		case DeviceAuthSslResult_GetTenantCertificateError:
			Log_Debug("DeviceAuthSslResult_GetTenantCertificateError (%d - '%s')\n",
				err,
				"Failed to load the device authentication certificate for the tenant");
			break;
		case DeviceAuthSslResult_EnableHwSignError:
			Log_Debug("DeviceAuthSslResult_EnableHwSignError (%d - '%s')\n",
				err,
				"Failed to enable hardware signing");
			break;
	}

	return CURLE_OK;
}
EapTlsResult EapTls_CallWebApi(const char *url, const char *queryString, const char *putString, const char *webApiRootCACertRelativePath, MemoryBlock *responseBlock)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != url && NULL != webApiRootCACertRelativePath && NULL != responseBlock)
	{
		// https://curl.haxx.se/libcurl/c/https.html
		curl_global_init(CURL_GLOBAL_ALL);
		CURL *curlHandle = curl_easy_init();
		if (curlHandle)
		{
			CURLcode res;
			// Set up for DAA mutual authentication
			// Device: https://learn.microsoft.com/en-us/azure-sphere/app-development/curl
			// WebAPI: https://learn.microsoft.com/en-us/azure/app-service/app-service-web-configure-tls-mutual-auth#special-considerations-for-certificate-validation

			// Set up the WebAPI's URL with the proper GET query
			char call_url[MAX_URL_LEN + 1];
			memset(call_url, 0, sizeof(call_url));
			if (NULL != queryString)
			{
				snprintf(call_url, sizeof(call_url), "%s?%s", url, queryString);
			}
			else
			{
				strncpy(call_url, url, sizeof(call_url) - 1);
			}

			// Activate verbose logging
			if ((res = curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L)) != CURLE_OK)
			{
				LogCurlError("FAILED curl_easy_setopt CURLOPT_VERBOSE", res);
			}
			if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, call_url)) != CURLE_OK)
			{
				LogCurlError(" FAILED curl_easy_setopt CURLOPT_URL", res);
			}
			else
			{
				// Set up the PUT fields, if any
				if (NULL != putString)
				{
					if ((res = curl_easy_setopt(curlHandle, CURLOPT_POST, 1L)) != CURLE_OK)
					{
						LogCurlError(" FAILED curl_easy_setopt CURLOPT_POST", res);
					}
					else
					{
						struct curl_slist *hs = NULL;
						hs = curl_slist_append(hs, "Content-Type: application/json");
						if ((res = curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, hs)) != CURLE_OK)
						{
							LogCurlError(" FAILED curl_easy_setopt CURLOPT_HTTPHEADER", res);
						}
						else
						{
							char hdrTemp[MAX_URL_LEN + 1];
							snprintf(hdrTemp, sizeof(hdrTemp), "Content-Length: %zd", strlen(putString));
							hs = curl_slist_append(hs, hdrTemp);
							if ((res = curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, hs)) != CURLE_OK)
							{
								LogCurlError(" FAILED curl_easy_setopt CURLOPT_HTTPHEADER", res);
							}
							else
							{
								if ((res = curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, putString)) != CURLE_OK)
								{
									LogCurlError(" FAILED curl_easy_setopt CURLOPT_POSTFIELDS", res);
								}
							}
						}
					}
				}
				//if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 0)) != CURLE_OK)
				//{
				//	LogCurlError("curl_easy_setopt CURLOPT_SSL_VERIFYHOST", res);
				//}
				if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0)) != CURLE_OK)
				{
					LogCurlError("FAILED curl_easy_setopt CURLOPT_SSL_VERIFYPEER", res);
				}
				else
				{
					// The simplest way to perform device authentication is to configure DeviceAuth_CurlSslFunc as the callback function for curl SSL authentication
					if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_CTX_FUNCTION, EapTls_DeviceAuth_CurlSslFunc)) != CURLE_OK)
					{
						LogCurlError("curl_easy_setopt CURLOPT_SSL_CTX_FUNCTION", res);
					}
					else
					{
						// libcurl on Azure Sphere supports TLS 1.2 and has deprecated TLS 1.0 and TLS 1.1 in alignment with the broader Microsoft TLS security strategy
						if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2)) != CURLE_OK)
						{
							LogCurlError("FAILED curl_easy_setopt CURLOPT_SSLVERSION", res);
						}
						else
						{
							// Get the full path to the certificate file used to authenticate the WebAPI's server identity
							// #NOTE: currently APIs do not support passing libcurl a certificate Id
							char *certificatePath = Storage_GetAbsolutePathInImagePackage(webApiRootCACertRelativePath);
							if (certificatePath == NULL)
							{
								Log_Debug("The certificate path could not be resolved: errno=%d (%s)\n", errno, strerror(errno));
							}
							else
							{
								// Set the path for the certificate file that cURL uses to validate the server certificate
								if ((res = curl_easy_setopt(curlHandle, CURLOPT_CAINFO, certificatePath)) != CURLE_OK)
								{
									LogCurlError("curl_easy_setopt CURLOPT_CAINFO", res);
								}
								else
								{
									// Let cURL follow any HTTP 3xx redirects. Important: any redirection to different domain names
									// requires that domain name to be added to app_manifest.json
									if ((res = curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK)
									{
										LogCurlError("FAILED curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
									}
									else
									{
										// Specify a user agent
										if ((res = curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != CURLE_OK)
										{
											LogCurlError("FAILED curl_easy_setopt CURLOPT_USERAGENT", res);
										}
										else
										{
											// Set the custom parameter of the callback to the memory block
											if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)responseBlock)) != CURLE_OK)
											{
												LogCurlError("FAILED curl_easy_setopt CURLOPT_WRITEDATA", res);
											}
											else
											{
												// Set up callback for cURL to use when downloading data
												if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, EapTls_StoreDownloadedDataCallback)) != CURLE_OK)
												{
													LogCurlError("FAILED curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
												}
												else
												{
													Log_Debug("Connecting to %s...\n", call_url);
													if ((res = curl_easy_perform(curlHandle)) == CURLE_OK)
													{
														// #NOTE: SSL renegotiation is not currently supported by Azure Sphere
														Log_Debug("\n -===- Downloaded content (%zu bytes): -===-\n", responseBlock->size);
														Log_Debug("%s\n", responseBlock->data);
														iRes = EapTlsResult_Success;
													}
													else
													{
														LogCurlError("FAILED curl_easy_perform", res);
														iRes = EapTlsResult_FailedConnectingToMdmWebApi;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			curl_easy_cleanup(curlHandle);
		}
		else
		{
			Log_Debug("FAILED initializing cURL!");
			iRes = EapTlsResult_Error;
		}
	}
	else
	{
		Log_Debug("ERROR: bad parameters!");
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_CallMdmWebApi(EapTlsConfig *eapTlsConfig, bool getRootCACertificate, bool getClientCertificate, MemoryBlock *responseBlock)
{
	EapTlsResult iRes = EapTlsResult_Error;

#if USE_TEST_WEB_API_RESPONSE

	// Mimic libcurl: just like in curl_easy_perform, the allocated memory has to be free-ed by the caller
	size_t size = strlen(test_web_api_response_json)
		responseBlock->data = malloc(size + 1);
	if (NULL != responseBlock->data)
	{
		memcpy(responseBlock->data, test_web_api_response_json, size);
		responseBlock->size = size;
		iRes = EapTlsResult_SUCCESS;
	}
#else
	if (NULL != eapTlsConfig && NULL != responseBlock)
	{
		// Set up the WebAPI's URL with the proper GET query, so to ask just for the specific certificates we need
		char query_str[MAX_URL_LEN + 1];
		memset(query_str, 0, sizeof(query_str));

		if (EapTlsResult_Error == EapTls_IsCertificateInstalled(eapTlsConfig->eapTlsRootCertificate.id))
			getRootCACertificate = true;

		if (EapTlsResult_Error == EapTls_ValidateCertificates(eapTlsConfig->eapTlsRootCertificate.id, NULL, NULL))
			getClientCertificate = true;

#if defined(WEBAPI_SERVER)
		// Call MDM WebApi "auth" method
		iRes = EapTls_CallWebApi(eapTlsConfig->mdmWebApiInterfaceUrl, NULL, "{}", eapTlsConfig->mdmWebApiRootCertificate.relativePath, responseBlock);
#endif
#if defined(WEBAPI_KESTREL)
		snprintf(query_str, sizeof(query_str) - 1, "%s=%s&%s=%s", webApi_RootCertificateField, getRootCACertificate ? strTrue : strFalse, webApi_ClientCertificateField, getClientCertificate ? strTrue : strFalse);
		iRes = EapTls_CallWebApi(eapTlsConfig->mdmWebApiInterfaceUrl, query_str, NULL, eapTlsConfig->mdmWebApiRootCertificate.relativePath, responseBlock);
#endif
	}
	else
	{
		Log_Debug("ERROR: EAP-TLS configuration and/or response MemoryBlock are NULL!");
		iRes = EapTlsResult_BadParameters;
	}
#endif

	return iRes;
}
EapTlsResult EapTls_WebApiRegisterDevice(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != eapTlsConfig)
	{
#if defined(WEBAPI_SERVER)
#	if defined(MDM_NEEDS_REGISTRATION)
		MemoryBlock responseBlock = { .data = NULL, .size = 0 };

		// Add further specific parameters here (currently set as an empty JSON body).
		iRes = EapTls_CallWebApi(g_webApiInterfaceRegisterUrl, NULL, "{}", eapTlsConfig->mdmWebApiRootCertificate.relativePath, &responseBlock);

		size_t nullTerminatedJsonSize = responseBlock.size + 1;
		char *nullTerminatedJsonString = (char *)malloc(nullTerminatedJsonSize);
		if (nullTerminatedJsonString == NULL)
		{
			Log_Debug("ERROR: could not allocate buffer for the WebAPI response.\n");
		}
		else
		{
			// Copy the provided buffer to a null terminated buffer.
			memcpy(nullTerminatedJsonString, responseBlock.data, responseBlock.size);
			nullTerminatedJsonString[nullTerminatedJsonSize - 1] = 0; // Add the null terminator at the end.

			// Parse the response
			JSON_Value *rootProperties = json_parse_string(nullTerminatedJsonString);
			if (rootProperties == NULL)
			{
				iRes = EapTlsResult_Error;
				Log_Debug("ERROR parsing response.\n");
			}
			else
			{
				JSON_Object *rootObject = json_value_get_object(rootProperties);

				// Add further specific returned JSON parsing here.
			}

			// Release the allocated memory.
			json_value_free(rootProperties);

		}

		// Release the allocated memory.
		free(nullTerminatedJsonString);
		free(responseBlock.data);
#	else
		iRes = EapTlsResult_Success; // No registretion required for the Kestrel sample WebApi.
#	endif
#endif
#if defined(WEBAPI_KESTREL)
		iRes = EapTlsResult_Success; // No registretion required for the Kestrel sample WebApi.
#endif
	}
	else
	{
		Log_Debug("ERROR: EAP-TLS configuration and/or response MemoryBlock are NULL!");
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}