/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

/* This file contains a list of "static inline" helper functions from the Azure Sphere SDK.
   Rather than reimplementing in usafe Rust, instead, each "static inline" function foo() is
   invoked by a nonstatic, non-inline foo_inline() function.  Rust code can invoke those
   functions via FFI.
*/

#include "static_inline_helpers.h"
#include "applibs/networking_curl.h"
#include <sys/timerfd.h>

// Resolve a symbol related to MUSL and unwinding
void __aeabi_unwind_bbb_pr0()
{
    while (1)
        ;
}

int timerfd_settime_inline(int fd, int flags,
                           const struct itimerspec *new_value,
                           struct itimerspec *old_value)
{
    return timerfd_settime(fd, flags, new_value, old_value);
}

int ADC_Open_inline(ADC_ControllerId id)
{
    return ADC_Open(id);
}
int ADC_Poll_inline(int fd, ADC_ChannelId channel_id, uint32_t *outSampleValue)
{
    return ADC_Poll(fd, channel_id, outSampleValue);
}
int ADC_GetSampleBitCount_inline(int fd, ADC_ChannelId channel_id)
{
    return ADC_GetSampleBitCount(fd, channel_id);
}
int ADC_SetReferenceVoltage_inline(int fd, ADC_ChannelId channel_id, float referenceVoltage)
{
    return ADC_SetReferenceVoltage(fd, channel_id, referenceVoltage);
}

size_t Applications_GetTotalMemoryUsageInKB_inline(void)
{
    return Applications_GetTotalMemoryUsageInKB();
}
size_t Applications_GetUserModeMemoryUsageInKB_inline(void)
{
    return Applications_GetUserModeMemoryUsageInKB();
}
size_t Applications_GetPeakUserModeMemoryUsageInKB_inline(void)
{
    return Applications_GetPeakUserModeMemoryUsageInKB();
}
int Applications_GetOsVersion_inline(Applications_OsVersion *outVersion)
{
    return Applications_GetOsVersion(outVersion);
}

int CertStore_GetCertificateSubjectName_inline(const char *identifier, struct CertStore_SubjectName *outSubjectName)
{
    return CertStore_GetCertificateSubjectName(identifier, outSubjectName);
}
int CertStore_GetCertificateIssuerName_inline(const char *identifier, struct CertStore_IssuerName *outIssuerName)
{
    return CertStore_GetCertificateIssuerName(identifier, outIssuerName);
}
int CertStore_GetCertificateNotBefore_inline(const char *identifier, struct tm *outNotBefore)
{
    return CertStore_GetCertificateNotBefore(identifier, outNotBefore);
}
int CertStore_GetCertificateNotAfter_inline(const char *identifier, struct tm *outNotAfter)
{
    return CertStore_GetCertificateNotAfter(identifier, outNotAfter);
}

int Networking_Curl_ProxyTypeToCurlProxyType_inline(Networking_ProxyType proxyType)
{
    return Networking_Curl_ProxyTypeToCurlProxyType(proxyType);
}

int Networking_Curl_SetDefaultProxy_inline(CURL *curlHandle)
{
    return Networking_Curl_SetDefaultProxy(curlHandle);
}
ssize_t Networking_GetInterfaces_inline(Networking_NetworkInterface *outNetworkInterfacesArray, size_t networkInterfacesArrayCount)
{
    return Networking_GetInterfaces(outNetworkInterfacesArray, networkInterfacesArrayCount);
}
int Networking_SetHardwareAddress_inline(const char *networkInterfaceName,
                                         const uint8_t *hardwareAddress,
                                         size_t hardwareAddressLength)
{
    return Networking_SetHardwareAddress(networkInterfaceName, hardwareAddress, hardwareAddressLength);
}
int Networking_GetHardwareAddress_inline(const char *networkInterfaceName,
                                         Networking_Interface_HardwareAddress *outAddress)
{
    return Networking_GetHardwareAddress(networkInterfaceName, outAddress);
}

int PowerManagement_ForceSystemReboot_inline()
{
    return PowerManagement_ForceSystemReboot();
}
int PowerManagement_ForceSystemPowerDown_inline(unsigned int maximum_residency_in_seconds)
{
    return PowerManagement_ForceSystemPowerDown(maximum_residency_in_seconds);
}
int PowerManagement_CpufreqOpen_inline(void)
{
    return PowerManagement_CpufreqOpen();
}
int PowerManagement_SetSystemPowerProfile_inline(PowerManagement_System_PowerProfile desired_profile)
{
    return PowerManagement_SetSystemPowerProfile(desired_profile);
}

int PWM_Open_inline(PWM_ControllerId pwm)
{
    return PWM_Open(pwm);
}
int PWM_Apply_inline(int pwmFd, PWM_ChannelId pwmChannel, const PwmState *newState)
{
    return PWM_Apply(pwmFd, pwmChannel, newState);
}

int SPIMaster_InitConfig_inline(SPIMaster_Config *config)
{
    return SPIMaster_InitConfig(config);
}
int SPIMaster_Open_inline(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId, const SPIMaster_Config *config)
{
    return SPIMaster_Open(interfaceId, chipSelectId, config);
}
ssize_t SPIMaster_WriteThenRead_inline(int fd, const uint8_t *writeData, size_t lenWriteData, uint8_t *readData, size_t lenReadData)
{
    return SPIMaster_WriteThenRead(fd, writeData, lenWriteData, readData, lenReadData);
}
int SPIMaster_InitTransfers_inline(SPIMaster_Transfer *transfers, size_t transferCount)
{
    return SPIMaster_InitTransfers(transfers, transferCount);
}
ssize_t SPIMaster_TransferSequential_inline(int fd, const SPIMaster_Transfer *transfers, size_t transferCount)
{
    return SPIMaster_TransferSequential(fd, transfers, transferCount);
}

void UART_InitConfig_inline(UART_Config *uartConfig)
{
    UART_InitConfig(uartConfig);
}
int UART_Open_inline(UART_Id uartId, const UART_Config *uartConfig)
{
    return UART_Open(uartId, uartConfig);
}

// deprecated: int WifiConfig_ForgetNetwork_inline(const WifiConfig_StoredNetwork *storedNetwork);
ssize_t WifiConfig_GetStoredNetworks_inline(WifiConfig_StoredNetwork *storedNetworkArray, size_t storedNetworkArrayCount)
{
    return WifiConfig_GetStoredNetworks(storedNetworkArray, storedNetworkArrayCount);
}
int WifiConfig_GetCurrentNetwork_inline(WifiConfig_ConnectedNetwork *connectedNetwork)
{
    return WifiConfig_GetCurrentNetwork(connectedNetwork);
}
ssize_t WifiConfig_GetScannedNetworks_inline(WifiConfig_ScannedNetwork *scannedNetworkArray, size_t scannedNetworkArrayCount)
{
    return WifiConfig_GetScannedNetworks(scannedNetworkArray, scannedNetworkArrayCount);
}
int WifiConfig_SetSSID_inline(int networkId, const uint8_t *ssid, size_t ssidLength)
{
    return WifiConfig_SetSSID(networkId, ssid, ssidLength);
}
int WifiConfig_SetSecurityType_inline(int networkId, WifiConfig_Security_Type securityType)
{
    return WifiConfig_SetSecurityType(networkId, securityType);
}
int WifiConfig_SetNetworkEnabled_inline(int networkId, bool enabled)
{
    return WifiConfig_SetNetworkEnabled(networkId, enabled);
}
int WifiConfig_PersistConfig_inline(void)
{
    return WifiConfig_PersistConfig();
}
int WifiConfig_ReloadConfig_inline(void)
{
    return WifiConfig_ReloadConfig();
}
int WifiConfig_SetPSK_inline(int networkId, const char *psk, size_t pskLength)
{
    return WifiConfig_SetPSK(networkId, psk, pskLength);
}
int WifiConfig_SetClientIdentity_inline(int networkId, const char *identity)
{
    return WifiConfig_SetClientIdentity(networkId, identity);
}
int WifiConfig_SetClientCertStoreIdentifier_inline(int networkId, const char *certStoreIdentifier)
{
    return WifiConfig_SetClientCertStoreIdentifier(networkId, certStoreIdentifier);
}
int WifiConfig_SetRootCACertStoreIdentifier_inline(int networkId, const char *certStoreIdentifier)
{
    return WifiConfig_SetRootCACertStoreIdentifier(networkId, certStoreIdentifier);
}
int WifiConfig_SetConfigName_inline(int networkId, const char *configName)
{
    return WifiConfig_SetConfigName(networkId, configName);
}
int WifiConfig_SetTargetedScanEnabled_inline(int networkId, bool enabled)
{
    return WifiConfig_SetTargetedScanEnabled(networkId, enabled);
}
int WifiConfig_GetNetworkDiagnostics_inline(int networkId, WifiConfig_NetworkDiagnostics *networkDiagnostics)
{
    return WifiConfig_GetNetworkDiagnostics(networkId, networkDiagnostics);
}
int WifiConfig_GetClientIdentity_inline(int networkId, WifiConfig_ClientIdentity *outIdentity)
{
    return WifiConfig_GetClientIdentity(networkId, outIdentity);
}
int WifiConfig_GetClientCertStoreIdentifier_inline(int networkId, CertStore_Identifier *outIdentifier)
{
    return WifiConfig_GetClientCertStoreIdentifier(networkId, outIdentifier);
}
int WifiConfig_GetRootCACertStoreIdentifier_inline(int networkId, CertStore_Identifier *outIdentifier)
{
    return WifiConfig_GetRootCACertStoreIdentifier(networkId, outIdentifier);
}
int WifiConfig_SetPowerSavingsEnabled_inline(bool enabled)
{
    return WifiConfig_SetPowerSavingsEnabled(enabled);
}