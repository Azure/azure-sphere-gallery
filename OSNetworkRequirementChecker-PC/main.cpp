#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
#	define _WINSOCK_DEPRECATED_NO_WARNINGS
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#	pragma comment(lib, "Ws2_32.lib")
#	include <WS2tcpip.h>
#	include <winsock2.h>
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#   include <unistd.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#endif

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <iostream>

const char *const ntpServers[] =
{
	"time.windows.com",
	"time.sphere.azure.net",
	"prod.time.sphere.azure.net",

	"168.61.215.74",
	"20.43.94.199", "20.189.79.72",
	"40.81.94.65", "40.81.188.85", "40.119.6.228", "40.119.148.38",
	"20.101.57.9",
	"51.137.137.111", "51.145.123.29",
	"52.148.114.188", "52.231.114.183",

	// Termination element
	""
};

typedef struct
{
	const char *hostname;
	int port;
} t_endpoint;
const t_endpoint endpoints[] =
{
	{ "Device provisioning and communication with IoT Hub:", -1 }, // Group description
	{ "global.azure-devices-provisioning.net", 8883 },
	{ "global.azure-devices-provisioning.net", 443 },

	{ "Internet connection checks, certificate file downloads, and similar tasks:", -1 }, // Group description
	{ "www.msftconnecttest.com", 80 },
	{ "prod.update.sphere.azure.net", 80 },

	{ "Communication with web services and Azure Sphere Security service:", -1 }, // Group description
	{ "anse.azurewatson.microsoft.com", 443 },
	{ "prod.core.sphere.azure.net", 443 },
	{ "prod.device.core.sphere.azure.net", 443 },
	{ "prod.deviceauth.sphere.azure.net", 443 },
	{ "prod.dinsights.core.sphere.azure.net", 443 },
	{ "prod.releases.sphere.azure.net", 443 },
	{ "prodmsimg.blob.core.windows.net", 443 },
	{ "prodmsimg-secondary.blob.core.windows.net", 443 },
	{ "prodptimg.blob.core.windows.net", 443 },
	{ "prodptimg-secondary.blob.core.windows.net", 443 },
	{ "sphereblobeus.azurewatson.microsoft.com", 443 },
	{ "sphereblobweus.azurewatson.microsoft.com", 443 },
	{ "sphere.sb.dl.delivery.mp.microsoft.com", 443 },

	// Termination element
	{ "", 0}
};



#define NTP_PORT			123				// NTP standard listening port
#define NTP_PORT_OUT		124				// NTP requests from Azure Sphere are sourced through local port 124
#define NTP_TIMESTAMP_DELTA 2208988800ull	// 70 years in seconds
#define SOCKET_TIMEOUT_SEC	10				// The socket timeout for receiving (in seconds)

typedef struct
{
	union
	{
		struct
		{
			uint8_t mode : 3;	// mode - i.e. client will pick mode 3 for client
			uint8_t vn : 3;		// vn	- Version number of the protocol.
			uint8_t li : 2;		// li	- Leap indicator
		};
		uint8_t li_vn_mode;		// 8 bits. li, vn, and mode.							 
	};

	uint8_t stratum;         //  8 bits. Stratum level of the local clock.
	uint8_t poll;            //  8 bits. Maximum interval between successive messages.
	uint8_t precision;       //  8 bits. Precision of the local clock.

	uint32_t rootDelay;      // 32 bits. Total round trip delay time.
	uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
	uint32_t refId;          // 32 bits. Reference clock identifier.

	uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
	uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

	uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
	uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

	uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
	uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

	uint32_t txTm_s;         // 32 bits. Transmit time-stamp seconds.
	uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;				 // Total: 384 bits or 48 bytes.

int query_ntp_server(const char *hostname, int ntp_port, int src_port);
int resolve_hostname(const char *hostname, int port);


int getSocketErrorCode(void)
{
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
	return WSAGetLastError();
#else
	return errno;
#endif
}

int query_ntp_server(const char *hostname, int ntp_port, int src_port)
{
	int iRes = 0;

	// Build the NTP packet
	ntp_packet packet = { 0 };
	packet.li = 0;		// No warning
	packet.vn = 4;		// Version 4 (aka IPv4-only)
	packet.mode = 3;	// Client Mode


	// Log the requested hostname.
	std::cout << "- time from " << hostname;

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		iRes = getSocketErrorCode();
		std::cerr << "WSAStartup() failed with error code " << iRes << std::endl;

		WSACleanup();
		return iRes;
	}
#endif

	// Does the host exist?
	struct hostent *server = gethostbyname(hostname);
	if (server == NULL)
	{
		iRes = getSocketErrorCode();
		std::cerr << "<???> --> gethostbyname() error " << iRes << std::endl;
	}
	else
	{
		// Setup the server address
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = *(long *)(server->h_addr_list[0]);
		server_addr.sin_port = htons(ntp_port);
		char server_ip_addr[INET_ADDRSTRLEN];
		sprintf(server_ip_addr, "%s", inet_ntoa(*((struct in_addr *)server->h_addr)));

		// Log the resolved IP address.
		std::cout << "<" << server_ip_addr << "> --> ";

		// Setup the source address (for source port customization)
		struct sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		// Optionally, set the given outbound port
		if (src_port > 0)
		{
			client_addr.sin_port = htons(src_port);
		}

		// Open an UDP socket
		int sock_fd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock_fd < 0)
		{
			iRes = getSocketErrorCode();
			std::cerr << "socket() open error " << iRes << std::endl;
		}
		else
		{
			// Set a receive timeout
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
			DWORD timeout = SOCKET_TIMEOUT_SEC * 1000;
			if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
			{
				iRes = getSocketErrorCode();
				std::cerr << "setsockopt() failed with error " << iRes << std::endl;
			}
#else
			struct timeval tv;
			tv.tv_sec = SOCKET_TIMEOUT_SEC;
			tv.tv_usec = 0;
			if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0)
			{
				iRes = getSocketErrorCode();
				std::cerr << "setsockopt() failed with error " << iRes << std::endl;
			}
#endif
			// Bind it to the client w/optional desired source port
			if (bind(sock_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
			{
				iRes = getSocketErrorCode();
				std::cerr << "bind() failed with error " << iRes << std::endl;
			}
			else
			{
				// Send the NTP packet
				int sent_bytes = sendto(sock_fd, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
				if (sent_bytes < 0)
				{
					iRes = getSocketErrorCode();
					std::cerr << "sendto() failed with error " << iRes << std::endl;
				}
				else
				{
					// Receive the NTP data back
					socklen_t client_addr_len = sizeof(client_addr);
					int recv_bytes = recvfrom(sock_fd, (char *)&packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);
					if (recv_bytes < 0)
					{
						iRes = getSocketErrorCode();
						std::cerr << "recvfrom() failed with error " << iRes << std::endl;
					}
					else
					{
						// Firstly, the "endianness" of txTm_s needs to be converted from the network's big-endian to the local host's one.
						// txTm_s contains the number of seconds passed since 00:00:00 UTC Jan 1st 1900, as of when the packet left the NTP server.
						packet.txTm_s = ntohl(packet.txTm_s);

						// The Unix epoch is the number of seconds since 00:00:00 UTC Jan 1st 1970,
						// therefore we subtract 70 years worth of seconds from the time returned by the NTP server.
						time_t txTm = (time_t)((uint64_t)packet.txTm_s - NTP_TIMESTAMP_DELTA);

						// Print the time we got from the NTP server, accounting the local timezone and conversion from UTC time.
						std::cout << ctime((const time_t *)&txTm);
					}
				}
			}
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
			closesocket(sock_fd);
#else
			close(sock_fd);
#endif
		}
	}

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	WSACleanup();
#endif

	return iRes;
}

int resolve_hostname(const char *hostname, int port)
{
	int iRes = 0;


#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		iRes = getSocketErrorCode();
		std::cerr << "WSAStartup() failed with error code " << iRes << std::endl;

		WSACleanup();
		return iRes;
	}
#endif

	// Does the host exist?
	std::cout << "- Resolving " << hostname << "... ";
	struct hostent *server = gethostbyname(hostname);
	if (server == NULL)
	{
		iRes = getSocketErrorCode();
		std::cout << "FAILED (errno=" << iRes << ")" << std::endl;
	}
	else
	{
		// Retrieve the server address
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = *(long *)(server->h_addr_list[0]);

		// Let's attempt a dummy connection
		std::cout << "success --> connecting to " << inet_ntoa(*((struct in_addr *)server->h_addr)) << ":" << port << "... ";
		int sock_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0)
		{
			iRes = getSocketErrorCode();
			std::cerr << "socket() open error " << iRes << std::endl;
		}
		else
		{
			server_addr.sin_port = htons(port);
			if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
			{
				iRes = getSocketErrorCode();
				std::cout << "FAILED (errno=" << iRes << ")" << std::endl;
			}
			else
			{
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
				closesocket(sock_fd);
#else
				close(sock_fd);
#endif
				std::cout << "success!" << std::endl;
			}
		}
	}

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	WSACleanup();
#endif

	return iRes;
}

int main(int argc, char **argv)
{
	std::cout << "Azure Sphere network-checkup utility." << std::endl << std::endl;
	try
	{
		std::cout << "Querying required NTP servers..." << std::endl;
		for (int i = 0; *ntpServers[i]; i++)
		{
			query_ntp_server(ntpServers[i], NTP_PORT, NTP_PORT_OUT);
		}

		std::cout << std::endl << "Querying required endpoints..." << std::endl;
		for (int i = 0; *endpoints[i].hostname; i++)
		{
			if (-1 == endpoints[i].port)
			{
				// This entry is a group description
				std::cout << std::endl << endpoints[i].hostname << std::endl;
			}
			else
			{
				resolve_hostname(endpoints[i].hostname, endpoints[i].port);
			}
		}
		std::cout << std::endl;
	}
	catch (...)
	{
		std::cout << std::endl << "Unexpected exception executing checks!" << std::endl;
	}

	return 0;
}
