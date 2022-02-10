/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include <init/applibs_versions.h>

#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <init/globals.h>
#include <utils/llog.h>
#include <utils/network.h>
#include <utils/utils.h>

static int config_downlink_private_network(const char *if_name)
{
    static struct in_addr local_ip;
    static struct in_addr subnet_mask;
    static struct in_addr gateway_ip;

    // configure static IP
    Networking_IpConfig ip_config;
    Networking_IpConfig_Init(&ip_config);
    inet_aton("192.168.100.10", &local_ip);
    inet_aton("255.255.255.0", &subnet_mask);
    inet_aton("0.0.0.0", &gateway_ip);
    Networking_IpConfig_EnableStaticIp(&ip_config, local_ip, subnet_mask, gateway_ip);

    if (Networking_IpConfig_Apply(if_name, &ip_config) != 0) {
        LOGE("can't apply ip config");
        Networking_IpConfig_Destroy(&ip_config);
        return -1;
    }

    Networking_IpConfig_Destroy(&ip_config);
    LOGI("Set static IP address on network interface: %s", if_name);

    // start SNTP server
    Networking_SntpServerConfig sntp_server_config;
    Networking_SntpServerConfig_Init(&sntp_server_config);

    if (Networking_SntpServer_Start(if_name, &sntp_server_config) != 0) {
        LOGE("can't start sntp server");
        Networking_SntpServerConfig_Destroy(&sntp_server_config);
        return -1;
    }

    Networking_SntpServerConfig_Destroy(&sntp_server_config);
    LOGI("SNTP server has started on network interface: %s", if_name);

    // start DHCP server
    Networking_DhcpServerConfig dhcp_server_config;
    Networking_DhcpServerConfig_Init(&dhcp_server_config);

    struct in_addr dhcp_start_ip;
    inet_aton("192.168.100.11", &dhcp_start_ip);

    Networking_DhcpServerConfig_SetLease(&dhcp_server_config, dhcp_start_ip, 1, subnet_mask, gateway_ip, 24);
    Networking_DhcpServerConfig_SetNtpServerAddresses(&dhcp_server_config, &local_ip, 1);

    if (Networking_DhcpServer_Start(if_name, &dhcp_server_config) != 0) {
        LOGE("can't start dhcp server");
        Networking_DhcpServerConfig_Destroy(&dhcp_server_config);
        return -1;
    }

    Networking_DhcpServerConfig_Destroy(&dhcp_server_config);
    LOGD("DHCP server has started on network interface: %s", if_name);
    return 0;
}

static int config_downlink_eth(void)
{
    LOGI("Set eth0 for downlink");
    return 0;    
}

static int config_uplink_eth(void)
{
    LOGI("Set eth0 for uplink");
    return 0;
}

static const char* extract_ssid(const char *config, int *ssid_len)
{
    char *colon = strchr(config, ':');
    if (colon) {
        *ssid_len = colon - config;
    } else {
        *ssid_len = strlen(config);
    }

    return config;
}

static const char* extract_psk(const char *config, int *psk_len)
{
    char *colon = strchr(config, ':');
    if (colon) {
        *psk_len = strlen(config) - (colon - config + 1);
        return *psk_len > 0 ? colon + 1 : NULL;
    } else {
        return NULL;
    }    
}


static bool is_ssid_exist(const char* ssid, int ssid_len)
{
    ssize_t count = WifiConfig_GetStoredNetworkCount();
    WifiConfig_StoredNetwork networks[count];
    WifiConfig_GetStoredNetworks(networks, count);
    for (int i=0; i<count; i++) {
        if ((networks[i].ssidLength == ssid_len) && (memcmp(ssid, networks[i].ssid, ssid_len) == 0)) {
            return true;
        }
    }
    return false;
}


static int config_uplink_wifi(const char *config)
{
    if (!config) return -1;

    int ssid_len = 0;
    const char *ssid = extract_ssid(config, &ssid_len);

    if (is_ssid_exist(ssid, ssid_len)) {
        LOGI("ssid already exist");
        return 0;
    }

    int psk_len = 0;
    const char *psk = extract_psk(config, &psk_len);

    int network_id = -1;

    if ((network_id = WifiConfig_AddNetwork()) < 0) {
        LOGE("Failed to add network");
        return -1;
    }

    if (WifiConfig_SetSSID(network_id, ssid, ssid_len) < 0) {
        LOGE("Failed to set SSID");
        return -1;
    }

    if (WifiConfig_SetSecurityType(network_id, psk ? WifiConfig_Security_Wpa2_Psk : WifiConfig_Security_Open) < 0) {
        LOGE("Failed to set security type");
        return -1;
    }

    if (psk && (WifiConfig_SetPSK(network_id, psk, psk_len) < 0)) {
        LOGE("Failed to set PSK");
        return -1;
    }

    if (WifiConfig_SetNetworkEnabled(network_id, true) < 0) {
        LOGE("Failed to enable network");
        return -1;
    }

    if (WifiConfig_PersistConfig() < 0) {
        LOGE("Failed to persist network config");
        return -1;
    }
    return 0;
}


static int config_uplink(link_t *uplink)
{
    if (!uplink || !uplink->if_name) return -1;

    if (strcmp(uplink->if_name, "eth") == 0) {
        return config_uplink_eth();
    } else if (strcmp(uplink->if_name, "wifi") == 0) {
        return config_uplink_wifi(uplink->if_data);
    } else {
        LOGE("Invalid uplink interface: %s", uplink->if_name);
        return -1;
    }
}

// downlink:
// if_name = uart/pn-eth/eth
// when if_name is uart, if_data is "uart config"
// when if_name is pn-eth, hard code static IP
// when if_name is eth, ignore if_data
static int config_downlink(link_t *downlink)
{
    if (!downlink || !downlink->if_name) return -1;

    if (strcmp(downlink->if_name, "pn-eth") == 0) {
        return config_downlink_private_network(downlink->if_name);
    } else if (strcmp(downlink->if_name, "eth") == 0) {
        return config_downlink_eth();
    } else if (strcmp(downlink->if_name, "uart") == 0) {
        // nothing to config for uart from network module
        return 0;
    } else {
        LOGE("Invalid downlink interface: %s", downlink->if_name);
        return -1;
    }
}


// ----------------------- public interface ---------------------------------


// Check if any wifi SSID been configured, if yes, use WIFI otherwise fallback to
// use Ethernet.
int network_init()
{
    LOGI("network_init");
    Networking_SetInterfaceState("wlan0", true);
    Networking_SetInterfaceState("eth0", true);
    return 0;
}

void network_deinit()
{
}


int network_config(link_t *uplink, link_t *downlink)
{
    if (!uplink || !downlink) return -1;

    if (config_uplink(uplink) != 0) {
        LOGE("Failed to configure uplink");
        return -1;
    }

    if (config_downlink(downlink) != 0) {
        LOGE("Failed to configure downlink");
        return -1;
    }
    return 0;
}

const char *network_get_status_str(Networking_InterfaceConnectionStatus status)
{
    if (status & Networking_InterfaceConnectionStatus_ConnectedToInternet) {
        return "ConnectedToInternet";
    } else if (status & Networking_InterfaceConnectionStatus_IpAvailable) {
        return "IpAvailable";
    } else if (status & Networking_InterfaceConnectionStatus_ConnectedToNetwork) {
        return "ConnectedToNetwork";
    } else if (status & Networking_InterfaceConnectionStatus_InterfaceUp) {
        return "InterfaceUp";
    } else {
        return "InterfaceDown";
    }
}

Networking_InterfaceConnectionStatus network_get_status(void)
{
    Networking_InterfaceConnectionStatus wifi_status = 0;
    Networking_InterfaceConnectionStatus eth_status = 0;

    Networking_GetInterfaceConnectionStatus("wlan0", &wifi_status);
    Networking_GetInterfaceConnectionStatus("eth0", &eth_status);
    return wifi_status | eth_status;
}

void network_get_mac(const char *ifa_name, char *buf, size_t buf_size)
{
    ASSERT(buf);
    ASSERT(buf_size > 0);

    struct ifaddrs *ifap, *ifaptr;

    buf[0] = '\0';

    if (getifaddrs(&ifap) == 0) {
        for (ifaptr = ifap; ifaptr != NULL; ifaptr = ifaptr->ifa_next) {
            if (ifaptr->ifa_name && ifaptr->ifa_addr && (strcmp(ifa_name, ifaptr->ifa_name) == 0) &&
                (ifaptr->ifa_addr->sa_family == AF_PACKET)) {
                struct sockaddr_ll *s = (struct sockaddr_ll *)(ifaptr->ifa_addr);
                snprintf(buf, buf_size, "%02x:%02x:%02x:%02x:%02x:%02x", s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
                         s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
                break;
            }
        }
        freeifaddrs(ifap);
    }
}

bool network_is_connected(void)
{
    return network_is_interface_connected("eth0") || network_is_interface_connected("wlan0");
}

bool network_is_interface_connected(const char *nic)
{
    Networking_InterfaceConnectionStatus status = 0;
    Networking_GetInterfaceConnectionStatus(nic, &status);
    return (status & Networking_InterfaceConnectionStatus_ConnectedToInternet) != 0;    

}
