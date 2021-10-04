#pragma once

#include "_environment_config.h"
#include "eap_tls_lib.h"

//////////////////////////////////////////////////////////////
// WebAPI related functions
//////////////////////////////////////////////////////////////

//#define MDM_NEEDS_REGISTRATION	// Uncomment if the MDM WebApi has an initial device registration step required.

/// <summary>
///     Parses the response from the WebAPI call.
/// </summary>
/// <param name="responseBlock">A valid pointer to a 'MemoryBlock' struct, pointing to the raw-byte response from the WebAPI.</param>
/// <param name="outResponse">A pre-allocated struct of type 'WebApiResponse', into which the output will be transposed</param>
/// <returns>
///     <para>EapTlsResult_Success, the JSON response was successfully parsed and transposed into the provided outResponse struct.</para>
///     <para>EapTlsResult_Error, error parsing the JSON response.</para>
///     <para>EapTlsResult_CertStoreFull, not enough space for this certificate, in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_ParseMdmWebApiResponse(MemoryBlock *responseBlock, WebApiResponse *outResponse);

/// <summary>
/// 
/// </summary>
/// <param name="url">A pointer to the web API's URL string.</param>
/// <param name="queryString">A pointer to the webAPI's query string, which will be used to call the web API. Can be NULL if not needed</param>
/// <param name="putString">A pointer to the webAPI's PUT fields, which will be used to call the web API.. Can be NULL if not needed</param>
/// <param name="webApiRootCACertiRelativePath">The relative path in the App's image RootCA certificate Id that will be used to authenticate the Web API Server.</param>
/// <param name="responseBlock">A valid pointer to a 'MemoryBlock' struct, into which the allocated raw-byte response from the WebAPI will be stored in.</param>
/// <returns>
///     <para>EapTlsResult_Success, succeeded calling the WebAPI, and <paramref name="responseBlock"/> contains a pointer the raw-byte response.</para>
///     <para>EapTlsResult_Error, error setting-up libcurl.</para>
///     <para>EapTlsResult_FailedConnectingToMdmWebApi, libcurl failed connecting to the WebAPI.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_CallWebApi(const char *url, const char *queryString, const char *putString, const char *webApiRootCACertiRelativePath, MemoryBlock *responseBlock);

/// <summary>
///     Calls the authentication WebAPI, and returns the parsed response.
/// </summary>
/// <param name="eapTlsConfig">A valid pointer to a pre-configured 'EapTlsConfig' struct, containing all the connection details</param>
/// <param name="getRootCACertificate">True if the RootCA certificate is required.</param>
/// <param name="getClientCertificate">True if the Client certificate is required.</param>
/// <param name="responseBlock">A valid pointer to a 'MemoryBlock' struct, into which the allocated raw-byte response from the WebAPI will be stored in.</param>
/// <returns>
///     <para>EapTlsResult_Success, succeeded calling the WebAPI, and <paramref name="responseBlock"/> contains a pointer the raw-byte response.</para>
///     <para>EapTlsResult_Error, error setting-up libcurl.</para>
///     <para>EapTlsResult_FailedConnectingToMdmWebApi, libcurl failed connecting to the WebAPI.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_CallMdmWebApi(EapTlsConfig *eapTlsConfig, bool getRootCACertificate, bool getClientCertificate, MemoryBlock *responseBlock);

/// <summary>
///		Calls the registration WebAPI, and returns the result of the registration process.
/// </summary>
/// <param name="eapTlsConfig">A valid pointer to a pre-configured 'EapTlsConfig' struct, containing all the connection details</param>
/// <returns></returns>
/// <returns>
///     <para>EapTlsResult_Success, succeeded calling the WebAPI, and <paramref name="responseBlock"/> contains a pointer the raw-byte response.</para>
///     <para>EapTlsResult_Error, error setting-up libcurl.</para>
///     <para>EapTlsResult_FailedConnectingToMdmWebApi, libcurl failed connecting to the WebAPI.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_WebApiRegisterDevice(EapTlsConfig *eapTlsConfig);
