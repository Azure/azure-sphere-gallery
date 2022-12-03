/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// This file contains a list of "static inline" helper functions from the Azure Sphere SDK.
// Rather than reimplementing in usafe Rust, instead, each "static inline" function foo() is
// invoked by a nonstatic, non-inline foo_inline() function.  Rust code can invoke those
// functions via FFI.

#define _AZSPHERE_DEPRECATED_BY(x)

#include "applibs/adc.h"
#include "applibs/applications.h"
#include "applibs/certstore.h"
#include "applibs/networking.h"
#include "applibs/pwm.h"
#include "applibs/spi.h"
#include "applibs/uart.h"
#include "applibs/powermanagement.h"
#include "applibs/wificonfig.h"
#include "curl/curl.h"

int timerfd_settime_inline(int fd, int flags,
                           const struct itimerspec *new_value,
                           struct itimerspec *old_value);

int ADC_Open_inline(ADC_ControllerId id);
int ADC_Poll_inline(int fd, ADC_ChannelId channel_id, uint32_t *outSampleValue);
int ADC_GetSampleBitCount_inline(int fd, ADC_ChannelId channel_id);
int ADC_SetReferenceVoltage_inline(int fd, ADC_ChannelId channel_id, float referenceVoltage);

size_t Applications_GetTotalMemoryUsageInKB_inline(void);
size_t Applications_GetUserModeMemoryUsageInKB_inline(void);
size_t Applications_GetPeakUserModeMemoryUsageInKB_inline(void);
int Applications_GetOsVersion_inline(Applications_OsVersion *outVersion);

int CertStore_GetCertificateSubjectName_inline(const char *identifier, struct CertStore_SubjectName *outSubjectName);
int CertStore_GetCertificateIssuerName_inline(const char *identifier, struct CertStore_IssuerName *outIssuerName);
int CertStore_GetCertificateNotBefore_inline(const char *identifier, struct tm *outNotBefore);
int CertStore_GetCertificateNotAfter_inline(const char *identifier, struct tm *outNotAfter);

int Networking_Curl_ProxyTypeToCurlProxyType_inline(Networking_ProxyType proxyType);
int Networking_Curl_SetDefaultProxy_inline(CURL *curlHandle);
ssize_t Networking_GetInterfaces_inline(Networking_NetworkInterface *outNetworkInterfacesArray, size_t networkInterfacesArrayCount);
int Networking_SetHardwareAddress_inline(const char *networkInterfaceName,
                                         const uint8_t *hardwareAddress,
                                         size_t hardwareAddressLength);
int Networking_GetHardwareAddress_inline(const char *networkInterfaceName,
                                         Networking_Interface_HardwareAddress *outAddress);

int PowerManagement_ForceSystemReboot_inline();
int PowerManagement_ForceSystemPowerDown_inline(unsigned int maximum_residency_in_seconds);
int PowerManagement_CpufreqOpen_inline(void);
int PowerManagement_SetSystemPowerProfile_inline(PowerManagement_System_PowerProfile desired_profile);

int PWM_Open_inline(PWM_ControllerId pwm);
int PWM_Apply_inline(int pwmFd, PWM_ChannelId pwmChannel, const PwmState *newState);

int SPIMaster_InitConfig_inline(SPIMaster_Config *config);
int SPIMaster_Open_inline(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId, const SPIMaster_Config *config);
ssize_t SPIMaster_WriteThenRead_inline(int fd, const uint8_t *writeData, size_t lenWriteData, uint8_t *readData, size_t lenReadData);
int SPIMaster_InitTransfers_inline(SPIMaster_Transfer *transfers, size_t transferCount);
ssize_t SPIMaster_TransferSequential_inline(int fd, const SPIMaster_Transfer *transfers, size_t transferCount);
int SPIMaster_InitConfig_inline(SPIMaster_Config *config);
int SPIMaster_Open_inline(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId, const SPIMaster_Config *config);
int SPIMaster_InitTransfers_inline(SPIMaster_Transfer *transfers, size_t transferCount);
ssize_t SPIMaster_TransferSequential_inline(int fd, const SPIMaster_Transfer *transfers, size_t transferCount);
ssize_t SPIMaster_WriteThenRead_inline(int fd, const uint8_t *writeData, size_t lenWriteData, uint8_t *readData, size_t lenReadData);

void UART_InitConfig_inline(UART_Config *uartConfig);
int UART_Open_inline(UART_Id uartId, const UART_Config *uartConfig);
void UART_InitConfig_inline(UART_Config *uartConfig);
int UART_Open_inline(UART_Id uartId, const UART_Config *uartConfig);

// deprecated: int WifiConfig_ForgetNetwork_inline(const WifiConfig_StoredNetwork *storedNetwork);
ssize_t WifiConfig_GetStoredNetworks_inline(WifiConfig_StoredNetwork *storedNetworkArray, size_t storedNetworkArrayCount);
int WifiConfig_GetCurrentNetwork_inline(WifiConfig_ConnectedNetwork *connectedNetwork);
ssize_t WifiConfig_GetScannedNetworks_inline(WifiConfig_ScannedNetwork *scannedNetworkArray, size_t scannedNetworkArrayCount);
int WifiConfig_SetSSID_inline(int networkId, const uint8_t *ssid, size_t ssidLength);
int WifiConfig_SetSecurityType_inline(int networkId, WifiConfig_Security_Type securityType);
int WifiConfig_SetNetworkEnabled_inline(int networkId, bool enabled);
int WifiConfig_PersistConfig_inline(void);
int WifiConfig_ReloadConfig_inline(void);
int WifiConfig_SetPSK_inline(int networkId, const char *psk, size_t pskLength);
int WifiConfig_SetClientIdentity_inline(int networkId, const char *identity);
int WifiConfig_SetClientCertStoreIdentifier_inline(int networkId, const char *certStoreIdentifier);
int WifiConfig_SetRootCACertStoreIdentifier_inline(int networkId, const char *certStoreIdentifier);
int WifiConfig_SetConfigName_inline(int networkId, const char *configName);
int WifiConfig_SetTargetedScanEnabled_inline(int networkId, bool enabled);
int WifiConfig_GetNetworkDiagnostics_inline(int networkId, WifiConfig_NetworkDiagnostics *networkDiagnostics);
int WifiConfig_GetClientIdentity_inline(int networkId, WifiConfig_ClientIdentity *outIdentity);
int WifiConfig_GetClientCertStoreIdentifier_inline(int networkId, CertStore_Identifier *outIdentifier);
int WifiConfig_GetRootCACertStoreIdentifier_inline(int networkId, CertStore_Identifier *outIdentifier);
int WifiConfig_SetPowerSavingsEnabled_inline(bool enabled);
