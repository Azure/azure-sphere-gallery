#pragma once
#include "../lib/eap_tls_lib.h"

EapTlsResult TestEapTlsLib_InitializeConfiguration(EapTlsConfig *eapTlsConfig);
EapTlsResult TestEapTlsLib_All(EapTlsConfig *eapTlsConfig, NetworkInterfaceType bootstrapNetworkInterfaceType);