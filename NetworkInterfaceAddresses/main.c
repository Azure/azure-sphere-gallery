// This minimal Azure Sphere app prints the MAC and IP address of the network interface.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)

#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>

static void PrintNetworkInterfaceAddresses(const char* interface);

// Network interface.
static const char networkInterface[] = "wlan0";
static const size_t maxMacAddrLength = 18;

/// <summary>
///     Prints the MAC and IP address of the network interface.
/// <param name="interface">The interface for which to print the network address.</param>
/// </summary>
static void PrintNetworkInterfaceAddresses(const char *interface)
{
    struct ifaddrs *addr_list;
    struct ifaddrs *it;
    int n;

    //  Get a linked list of network interfaces.
    if (getifaddrs(&addr_list) != 0) {
        Log_Debug("ERROR: getifaddrs: %d (%s)\n", errno, strerror(errno));
    } else {
        for (it = addr_list, n = 0; it != NULL; it = it->ifa_next, ++n) {
            if (it->ifa_addr == NULL) {
                continue;
            }

            if (strncmp(it->ifa_name, interface, strlen(interface)) == 0) {
                if (it->ifa_addr->sa_family == AF_INET) {
                    // Get the internet address.
                    struct sockaddr_in *addr = (struct sockaddr_in *)it->ifa_addr;
                    char *ip_address = inet_ntoa(addr->sin_addr);

                    Log_Debug("%s IP address: %s \n", interface, ip_address);
                } else if (it->ifa_addr->sa_family == AF_PACKET) {
                    char mac_string[maxMacAddrLength];

                    // Get the physical-layer address.
                    struct sockaddr_ll *mac_addr = (struct sockaddr_ll *)it->ifa_addr;
                    unsigned char *mac = mac_addr->sll_addr;

                    // Format the address.
                    snprintf(mac_string, maxMacAddrLength, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],
                            mac[1], mac[2], mac[3], mac[4], mac[5]);

                    Log_Debug("%s MAC: %s \n", interface, mac_string);
                }
            }
        }
    }

    freeifaddrs(addr_list);
}

int main(void)
{
    Log_Debug("Starting Print network Address application...\n");

    PrintNetworkInterfaceAddresses(networkInterface);

    const struct timespec sleepTime = { .tv_sec = 1, .tv_nsec = 0 };
    while (true) {
        nanosleep(&sleepTime, NULL);
    }
}