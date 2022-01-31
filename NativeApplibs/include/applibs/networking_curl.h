/// \file networking_curl.h
/// \brief This header contains functions and types that interact with the networking subsystem to
/// apply the stored proxy configuration on a curl handle.
///
/// The function summaries include any required application manifest settings.
///
/// These functions are not thread safe.
#pragma once

#include <curl/curl.h>

#if !defined(AZURE_SPHERE_PUBLIC_SDK)

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Applies the proxy settings on the curl handle. The application manifest must
///     include the NetworkConfig or ReadNetworkProxyConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the NetworkConfig or
///     ReadNetworkProxyConfig capability.</para>
///     <para>EFAULT: the <paramref name="curlHandle"/> parameter is NULL.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="curlHandle">
///     A pointer to the <see cref="CURL"/> handle.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int Networking_Curl_SetDefaultProxy(CURL *curlHandle);

#ifdef __cplusplus
}
#endif

#endif // !defined(AZURE_SPHERE_PUBLIC_SDK)

#include <applibs/networking_curl_internal.h>
