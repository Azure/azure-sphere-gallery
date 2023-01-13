# Unicast DNS service discovery

This application demonstrates how to perform [DNS service discovery](https://learn.microsoft.com/azure-sphere/app-development/service-discovery) by sending DNS-SD queries to a configured DNS server. It is based on the [Multicast DNS service discovery sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/DNSServiceDiscovery).

The application queries a configured DNS server for **PTR** records that identify instances of the `_http._tcp` service. If available, the application then queries the server for the **SRV**, **TXT**, and **A** records that contain the DNS details for the service instance. Once complete, the Azure Sphere firewall allows the application to connect to the discovered host names, and a simple curl HTTP fetch is performed to the hostname returned by the **SRV** record.

A container is provided for a simple, minimal DNS server, configured to serve the `home` domain, with an `_http._tcp` service pointing at `HelloWorld._http._tcp.home`, which in turn is configured to resolve to `www.dns-sd.org`.

Unlike multicast service discovery, the discovered service may exist outside of the local network for the device.

## Prerequisites

The sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits)
- (optional) a machine running Docker to run the DNS server container

## Setup

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 22.11 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://learn.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.

1. Set up a DNS service. A container is provided that runs a minimal `bind9` on Ubuntu 22.04

   ```console
   $ docker build ./container -t dns-sd-example
   $ docker run --rm -it -p 53:53/udp 5353:5353/udp dns-sd-example
   * Starting domain name service... named                  [ OK ] 
   root@ef71614eec1c:/#
   ```

   Note that if you have an existing DNS server running on port 53 of the Docker host (for example, a Linux system such as Ububtu that uses systemd is likely to have systemd-resolved running for local name resolution) you will need to disable this and configure an alternative (for example, specify a nameserver directly in `/etc/resolv.conf`)

1. Configure the application - specify the IP address of your nameserver and a secondary in `main.c`:

   ```c
   static const char DnsSDServerIp[] = "10.0.0.1"; // Replace this with your DNS server for service discovery
   static const char OtherDnsServerIp[] = "10.0.0.2"; // Replace this with a second DNS server for normal resolution
   ```

### Use Ethernet instead of Wi-Fi

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, complete the following steps:

1. Follow the [Ethernet setup instructions](https://learn.microsoft.com/azure-sphere/network/connect-ethernet).
1. Ensure that the global constant **networkInterface** is set to "eth0". Find the following line of code in `main.c` and  replace `wlan0` with `eth0`:

    ```c
    char networkInterface[] = "wlan0";
    ```

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

### Test the sample

When you run the sample, every 10 seconds the application will send a DNS query. When it receives a response, it displays the name, host, IPv4 address, port, and TXT data from the query response. The application will then connect to the discovered server and do an HTTP GET.