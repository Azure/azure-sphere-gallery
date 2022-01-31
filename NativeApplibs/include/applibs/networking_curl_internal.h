#pragma once

#include <errno.h>

#include <applibs/networking.h>

#if !defined(AZURE_SPHERE_PUBLIC_SDK)

#ifdef __cplusplus
extern "C" {
#endif

static inline int Networking_Curl_ProxyTypeToCurlProxyType(Networking_ProxyType proxyType)
{
    switch (proxyType) {
    case Networking_ProxyType_HTTP:
        return CURLPROXY_HTTP;
    }
    return -1; // Will not be reached for valid proxyType
}

static inline int Networking_Curl_SetDefaultProxy(CURL *curlHandle)
{
    if (curlHandle == NULL) {
        errno = EFAULT;
        return -1;
    }

    Networking_ProxyConfig *proxyConfig = Networking_Proxy_Create();
    if (proxyConfig == NULL) {
        return -1;
    }

    int err = 0;
    do {
        err = Networking_Proxy_Get(proxyConfig);
        if (err) {
            if (errno == ENOENT) {
                // No proxy is configured
                err = 0;
                curl_easy_setopt(curlHandle, CURLOPT_PROXY, NULL);
            }

            break;
        }

        Networking_ProxyOptions proxyOptions = 0;
        err = Networking_Proxy_GetProxyOptions(proxyConfig, &proxyOptions);
        if (err) {
            break;
        }

        if (proxyOptions & Networking_ProxyOptions_Enabled) {
            // Proxy is enabled
            uint16_t proxyPort = 0;
            err = Networking_Proxy_GetProxyPort(proxyConfig, &proxyPort);
            if (err) {
                break;
            }

            Networking_ProxyType proxyType = Networking_Proxy_GetProxyType(proxyConfig);
            Networking_ProxyAuthType proxyAuthType = Networking_Proxy_GetAuthType(proxyConfig);
            const char *proxyAddress = Networking_Proxy_GetProxyAddress(proxyConfig);
            const char *noProxyAddresses = Networking_Proxy_GetNoProxyAddresses(proxyConfig);

            curl_easy_setopt(curlHandle, CURLOPT_PROXYTYPE,
                             Networking_Curl_ProxyTypeToCurlProxyType(proxyType));
            curl_easy_setopt(curlHandle, CURLOPT_PROXY, proxyAddress);
            curl_easy_setopt(curlHandle, CURLOPT_PROXYPORT, proxyPort);
            curl_easy_setopt(curlHandle, CURLOPT_NOPROXY, noProxyAddresses);
            if (proxyAuthType == Networking_ProxyAuthType_Basic) {
                const char *username = Networking_Proxy_GetProxyUsername(proxyConfig);
                const char *password = Networking_Proxy_GetProxyPassword(proxyConfig);

                curl_easy_setopt(curlHandle, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
                curl_easy_setopt(curlHandle, CURLOPT_PROXYUSERNAME, username);
                curl_easy_setopt(curlHandle, CURLOPT_PROXYPASSWORD, password);
            } else {
                curl_easy_setopt(curlHandle, CURLOPT_PROXYAUTH, CURLAUTH_NONE);
            }
        } else {
            // Proxy configuration exists but is not enabled
            curl_easy_setopt(curlHandle, CURLOPT_PROXY, NULL);
        }

    } while (0);

    Networking_Proxy_Destroy(proxyConfig);

    return err;
}

#ifdef __cplusplus
}
#endif

#endif // !defined(AZURE_SPHERE_PUBLIC_SDK)
