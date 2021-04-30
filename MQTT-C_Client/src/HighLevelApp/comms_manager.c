/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "comms_manager.h"

/* Maximum size for network read/write callbacks. There is also a v5 define that
   describes the max MQTT control packet size, DEFAULT_MAX_PKT_SZ. */
#define MAX_BUFFER_SIZE 128
#define SEND_BUFFER_SIZE 512
#define RECEIVE_BUFFER_SIZE 512
#define BILLION 1000000000

static void mqtt_ping_handler(EventLoopTimer* eventLoopTimer);
static void mqtt_reconnect_handler(EventLoopTimer* eventLoopTimer);
static void mqtt_set_subscriptions(void);
static void reconnect_client(struct mqtt_client* client, void** reconnect_state_vptr);

static WOLFSSL_CTX* ctx = NULL;
static WOLFSSL* ssl = NULL;
static bool wolfSslInitialized = false;
struct mqtt_client client;
int sockfd = -1;

EventRegistration* mqtt_socket_registration = NULL;

static bool mqtt_connected = false;
static void (*_mqtt_connected_cb)(void);
const char** _sub_topics = NULL;
size_t _sub_topic_count = 0;

struct reconnect_state_t {
	const char* hostname;
	const char* port;
	const char** subtopics;
	uint8_t* sendbuf;
	size_t sendbufsz;
	uint8_t* recvbuf;
	size_t recvbufsz;
};

struct reconnect_state_t reconnect_state;

uint8_t sendbuf[SEND_BUFFER_SIZE];
uint8_t recvbuf[RECEIVE_BUFFER_SIZE];

// When .period is {0,0} then the timer is a oneshot timer
DX_TIMER mqtt_reconnect_timer = { .period = {0, 0}, .name = "mqtt_reconnect_timer", .handler = mqtt_reconnect_handler };
DX_TIMER mqtt_ping_timer = { .period = {30, 0}, .name = "mqtt_ping_timer", .handler = mqtt_ping_handler };

bool is_mqtt_connected(void) {
	return mqtt_connected;
}

static void mqtt_ping_handler(EventLoopTimer* eventLoopTimer){
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	if (mqtt_connected) {
		mqtt_ping(&client);
	}
}

void publish_message(const void* data, size_t data_length, const char* topic) {
	if (strlen(topic) == 0 || !dx_isNetworkReady()) { return; }

	mqtt_mq_clean(&client.mq);

	if (client.mq.curr_sz >= data_length) {
		mqtt_publish(&client, topic, data, data_length, MQTT_PUBLISH_QOS_0);
	}

	mqtt_sync(&client);
}

/// <summary>
/// Check status of connection to MQTT Broker
/// </summary>
static void mqtt_reconnect_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	reconnect_client(&client, &client.reconnect_state);
	return;

	if (dx_isNetworkReady()) {
		mqtt_sync(&client);
	} else {
		dx_timerOneShotSet(&mqtt_reconnect_timer, &(struct timespec) { 1, 0 });
	}
}

static void msg_handler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context) {
	mqtt_sync(&client);
}

static char* get_absolute_storage_path(const char* file, const char* name) {
	char* abs_path = Storage_GetAbsolutePathInImagePackage(file);
	if (abs_path == NULL) {
		Log_Debug("ERROR: unable to open %s.\n", name);
	}
	return abs_path;
}

static WOLFSSL* open_nb_socket(const char* addr, const char* port) {
	int ret, rv;
	char* abs_path = NULL;
	struct addrinfo hints = { 0 };

	hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Must be TCP */


	struct addrinfo* p, * servinfo;

	if (ssl != NULL) {
		wolfSSL_free(ssl);
		ssl = NULL;
	}
	if (ctx != NULL) {
		wolfSSL_CTX_free(ctx);
		ctx = NULL;
	}
	if (wolfSslInitialized) {
		wolfSSL_Cleanup();
		wolfSslInitialized = false;
	}
	if (mqtt_socket_registration != NULL) {
		EventLoop_UnregisterIo(dx_timerGetEventLoop(), mqtt_socket_registration);
	}
	if (sockfd != -1) {

		close(sockfd);
		sockfd = -1;
	}

	/* get address information */
	rv = getaddrinfo(addr, port, &hints, &servinfo);
	if (rv != 0) {
		Log_Debug("Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
		goto cleanupLabel;
	}

	/* open the first possible socket */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) continue;

		/* connect to server */
		rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (rv == -1) {
			close(sockfd);
			sockfd = -1;
			continue;
		}
		break;
	}

	/* free servinfo */
	freeaddrinfo(servinfo);

	if (sockfd == -1) {
		goto cleanupLabel;
	}

	/* Initialize wolfSSL */
	int r = wolfSSL_Init();
	if (r != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: wolfSSL_init failed\n");
		goto cleanupLabel;
	}

	wolfSslInitialized = true;

	/* Create and initialize WOLFSSL_CTX */
	ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
	if (ctx == NULL) {
		Log_Debug("ERROR: failed to create WOLFSSL_CTX\n");
		goto cleanupLabel;
	}

	/* Load root CA certificates full path */
	if ((abs_path = get_absolute_storage_path(MQTT_CA_CERTIFICATE, "MQTT_CA_CERTIFICATE")) == NULL) {
		goto cleanupLabel;
	}

	if ((ret = wolfSSL_CTX_load_verify_locations(ctx, abs_path, NULL)) != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: failed to load ca certificate\n");
		free(abs_path);
		abs_path = NULL;
		goto cleanupLabel;
	}

	free(abs_path);
	abs_path = NULL;

	/* Load private key full path */
	if ((abs_path = get_absolute_storage_path(MQTT_CLIENT_PRIVATE_KEY, "MQTT_CLIENT_PRIVATE_KEY")) == NULL) {
		goto cleanupLabel;
	}

	if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, abs_path, WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: failed to private key certificate\n");
		free(abs_path);
		abs_path = NULL;
		goto cleanupLabel;
	}

	free(abs_path);
	abs_path = NULL;


	/* Load client certificate full path */
	if ((abs_path = get_absolute_storage_path(MQTT_CLIENT_CERTIFICATE, "MQTT_CLIENT_CERTIFICATE")) == NULL) {
		goto cleanupLabel;
	}

	if ((ret = wolfSSL_CTX_use_certificate_file(ctx, abs_path, WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: failed to client certificate\n");
		free(abs_path);
		abs_path = NULL;
		goto cleanupLabel;
	}

	free(abs_path);
	abs_path = NULL;

	ssl = wolfSSL_new(ctx);
	if (ssl == NULL) {
		goto cleanupLabel;
	}

	// Associate socket with wolfSSL session.
	r = wolfSSL_set_fd(ssl, sockfd);
	if (r != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: wolfSSL_set_fd %d\n", r);
		goto cleanupLabel;
	}

	if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: wolfSSL_connect, reason = %d\n", wolfSSL_get_error(ssl, ret));
		goto cleanupLabel;
	}

	if (sockfd != -1) fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

	// https://docs.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-eventloop/function-eventloop-registerio
	mqtt_socket_registration = EventLoop_RegisterIo(dx_timerGetEventLoop(), sockfd, EventLoop_Input, msg_handler, NULL);

	return ssl;

cleanupLabel:
	if (ssl != NULL) {
		wolfSSL_free(ssl);
		ssl = NULL;
	}
	if (ctx != NULL) {
		wolfSSL_CTX_free(ctx);
		ctx = NULL;
	}
	if (wolfSslInitialized) {
		wolfSSL_Cleanup();
		wolfSslInitialized = false;
	}
	if (sockfd != -1) {
		close(sockfd);
		sockfd = -1;
	}

	return NULL;
}

static void reconnect_client(struct mqtt_client* client, void** reconnect_state_vptr) {
	struct reconnect_state_t* reconnect_state = *((struct reconnect_state_t**)reconnect_state_vptr);

	if (!dx_isNetworkReady()) {
		dx_timerOneShotSet(&mqtt_reconnect_timer, &(struct timespec) { 2, 0 });
		Log_Debug("Network not ready.\n");
		return;
	}

	/* Perform error handling here. */
	if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
		Log_Debug("reconnect_client: called while client was in error state \"%s\"\n",
			mqtt_error_str(client->error)
		);
	}

	/* Open a new socket. */
	WOLFSSL* ssl = open_nb_socket(reconnect_state->hostname, reconnect_state->port);
	if (ssl == NULL) {
		dx_timerOneShotSet(&mqtt_reconnect_timer, &(struct timespec) { 2, 0 });
		Log_Debug("Failed to open socket: ");
		return;
	}

	/* Reinitialize the client. */
	mqtt_reinit(client, ssl,
		reconnect_state->sendbuf, reconnect_state->sendbufsz,
		reconnect_state->recvbuf, reconnect_state->recvbufsz
	);

	/* Create an anonymous session */
	const char* client_id = NULL;
	/* Ensure we have a clean session */
	uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
	/* Send connection request to the broker. */
	mqtt_connect(client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags, 300);

	mqtt_set_subscriptions();
}

static void mqtt_set_subscriptions(void) {
	int i = 0;
	mqtt_connected = false;

	if (client.error == MQTT_OK) {
		for (i = 0; i < _sub_topic_count; i++) {
			mqtt_subscribe(&client, _sub_topics[i], 0);
		}

		mqtt_connected = true;
		_mqtt_connected_cb();
	}
}


/**
 * init MQTT connection and subscribe desired topics
 */
void initialize_mqtt(void (*publish_callback)(void** unused, struct mqtt_response_publish* published), void (*mqtt_connected_cb)(void), const char **sub_topics, size_t sub_topic_count) {
	_mqtt_connected_cb = mqtt_connected_cb;
	_sub_topics = sub_topics;
	_sub_topic_count = sub_topic_count;

	/* build the reconnect_state structure which will be passed to reconnect */
	reconnect_state.hostname = ALTAIR_MQTT_HOST;
	reconnect_state.port = ALTAIR_MQTT_SECURE_PORT;
	reconnect_state.subtopics = sub_topics;
	reconnect_state.sendbuf = sendbuf;
	reconnect_state.sendbufsz = sizeof(sendbuf);
	reconnect_state.recvbuf = recvbuf;
	reconnect_state.recvbufsz = sizeof(recvbuf);

	mqtt_init_reconnect(&client, reconnect_client, &reconnect_state, publish_callback);

	dx_timerStart(&mqtt_reconnect_timer);
	dx_timerStart(&mqtt_ping_timer);

	reconnect_client(&client, &client.reconnect_state);
}