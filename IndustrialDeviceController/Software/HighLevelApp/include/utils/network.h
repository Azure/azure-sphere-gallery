/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once
#include <applibs/networking.h>

typedef struct link_t {
    char *if_name;
    char *if_data;
} link_t;

/**
 * initialize network module
 * @return 0 if succeed, negative otherwise
 */
int network_init(void);

/**
 * deinitialize network module
 */
void network_deinit(void);

/**
 * get MAC address of given interface
 * @param ifa_name interface name
 * @param buf buffer to hold output MAC address
 * @param buf_size buffer size
 */
void network_get_mac(const char* ifa_name, char* buf, size_t buf_size);

/**
 * configure network uplink (how to connect to iot hub, wifi or ethernet)
 * network downlink (if we try to use network to connect to CE device), currently we support
 * direct TCP connection and private ethernet connection
 * @param uplink uplink to iot hub configuration
 * @param downlink downlink to ce device configuration
 * @return 0 if succeed or -1 if any error
 */
int network_config(link_t* uplink, link_t* downlink);

/**
 * get combined network status from all network interface
 * Since IoT SDK will try to use whatever avaiable network unless we explicitly enable/disable certain
 * network interface, we intentionally enable both wifi and eth interface so we can have flexibility to
 * remote configure wifi connection.
 * @return combined network interface status, in other word, the closest status we can get toward internet connection
 */
Networking_InterfaceConnectionStatus network_get_status(void);

/**
 * translate network status enum to human readable string
 * @param status status enum value
 * @return status string value
 */
const char *network_get_status_str(Networking_InterfaceConnectionStatus status);

/**
 * check whether network is connected to internet, through any network interface
 * @return true if connected to internet, false otherwise
 */
bool network_is_connected(void);

/**
 * check whether network is connected to internet, through given network interface
 * @param nic network interface to query
 * @return true if connected to internet, false otherwise
 */
bool network_is_interface_connected(const char *nic);

