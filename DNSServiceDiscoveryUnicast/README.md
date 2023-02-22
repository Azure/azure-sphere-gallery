# Unicast DNS service discovery

This application demonstrates how to perform [DNS service discovery](https://learn.microsoft.com/azure-sphere/app-development/service-discovery) by sending DNS-SD queries to a configured DNS server. It is based on the [Multicast DNS service discovery sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/DNSServiceDiscovery).

The application queries a configured DNS server for **PTR** records that identify instances of the `_http._tcp` service. If available, the application then queries the server for the **SRV**, **TXT**, and **A** records that contain the DNS details for the service instance. Once complete, the Azure Sphere firewall allows the application to connect to the discovered host names, and a simple curl HTTP fetch is performed to the hostname returned by the **SRV** record.

A container is provided for a simple, minimal DNS server, configured to serve the `home` domain, with an `_http._tcp` service pointing at `HelloWorld._http._tcp.home`, which in turn is configured to resolve to `www.dns-sd.org`.

Unlike multicast service discovery, the discovered service may exist outside of the local network for the device.

## Contents

| File/folder | Description |
|-------------|-------------|
| `.vscode/` | Settings for VSCode |
| `container/` | Dockerfile and supporting content for DNS server container |
| `main.c`, `dns-sd.c`, `dns-sd.h`       | source code. |
| `eventloop_timer_utilities.c`, `eventloop_timer_utilities.h` | common library source code |
| `app_manifest.json` | The application manifest |
| `CMakeLists.txt` | Contains the project information and produces the build. |
| `CMakeSettings.json` | Configures CMake with the correct command-line options. |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites

The project requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits)
- (optional) a machine running Docker to run the DNS server container

## Setup

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 22.11 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://learn.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.

1. Set up a DNS service. A container is provided that runs a minimal `bind9` on Ubuntu 22.04:

   ```console
   $ docker build ./container -t dns-sd-example
   $ docker run --rm -it -p 53:53/udp 5353:5353/udp dns-sd-example
   * Starting domain name service... named                  [ OK ] 
   root@ef71614eec1c:/#
   ```

   **Note on networking:** The `docker run` command above will bind UDP ports 53 and 5353 from the container to the same ports on the host, so when configuring the Azure Sphere application, you should use the Docker host's IP address as the DNS server.
   
   If the Docker host has an existing DNS server running on port 53 (for example, a Linux system such as Ubuntu that uses systemd is likely to have systemd-resolved running for local name resolution), you will see a **Port in use** error when launching the container. You will need to disable the existing DNS server (and configure an alternative for local resolution, if required). Alternatively, you can disable the port binding (remove the `-p` options) and ensure the docker container is visible to the outside network; see the [Docker documentation](https://docs.docker.com/network/) for further details.

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

### Configuring your own DNS server

The supplied Docker image uses **bind9**, but the sample code should work with any correctly configured DNS server (including dedicated DNS-SD servers like **Avahi**, or DNS caches like **dnsmasq**).

When configuring the DNS server, you should ensure that the time-to-live (TTL) for returned SRV/PTR records is **non-zero**: records with a TTL of zero are considered to be expiring immediately, and as such the Azure Sphere operating system will not open the firewall for them.

#### dnsmasq example

For example, **dnsmasq** sets a TTL of zero for all its responses as it is primarily a DNS cache - a zero TTL indicates a record should not be cached by downstream clients. To ensure that locally-configured SRV/PTR records have a non-zero TTL, you should modify `dnsmasq.conf` to include:

```
local-ttl=10 # Or other non-zero value (in seconds)
```

## Running the code

To build and run this project, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

On running the application, after checking that there is a connection to the internet, you should see, every ten seconds, the application perform the DNS-SD requests, followed by the download from www.dns-sd.org:

```
INFO: DNS Service Discovery sample starting.
INFO: Network interface eth0 status: 0x0f
INFO: Sending DNS query to resolve domain name [_http._tcp.home]...
INFO: DNS Service Discovery has found an instance: HelloWorld._http._tcp.home.
INFO: Requesting SRV and TXT details for the instance.
INFO: Sending DNS query to resolve domain name [HelloWorld._http._tcp.home]...
www.dns-sd.org
	Name: HelloWorld._http._tcp.home
	Host: www.dns-sd.org
	IPv4 Address: 255.255.255.255
	Port: 80
	TXT Data: path=/Success.html
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN"
        "http://www.w3.org/TR/1998/REC-html40-19980424/loose.dtd">
<HTML>
...
```

### Troubleshooting

If you see:

```
ERROR: Invalid DNS server address or address family specified.
```

ensure you have configured the DNS servers as described above.

## Key concepts

Unlike multicast service discovery, the discovered service may exist outside of the local network for the device. This means that DNS service discovery can be used to discover and connect to endpoints on the wider internet - for example, Azure IoT hubs or other dynamically-assigned endpoints - without needing to specify them in the application manifest.

## Project expectations

This code is presented for informational purposes only; it is based on the existing DNS Service Discovery sample, with minimal modification. It is not intended for production scenarios, and it has not been rigorously checked for security issues or other errors.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this project, please open a GitHub issue within this repo.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

For details on license, see LICENSE.txt in this directory.