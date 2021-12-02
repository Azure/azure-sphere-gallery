/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include <applibs/applications.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>
#include <applibs/powermanagement.h>

#include <init/globals.h>
#include <init/adapter.h>
#include <iot/diag.h>
#include <utils/event_loop_timer.h>
#include <iot/iot.h>
#include <utils/llog.h>
#include <utils/network.h>
#include <utils/timer.h>
#include <utils/utils.h>
#include <utils/led.h>
#include <safeclib/safe_lib.h>
#include <frozen/frozen.h>
//#include "watchdog.h"

#define MAX_EVENT_IN_MEMORY 500
#define MAX_MAC_ADDRESS_SIZE 20

const char EVENT_FILE_MAGIC[8] = {'D', 'I', 'A', 'G', ' ', 'V', '0', '4'};

typedef struct event_t event_t;
struct event_t {
    struct timespec ts;
    uint16_t code;
    uint16_t repeat;
};

typedef struct diag_value_t diag_value_t;
struct diag_value_t {
    char *key;
    double value;
    diag_value_t *next;
};

typedef struct event_file_hdr_t event_file_hdr_t;
struct event_file_hdr_t {
    char magic[8];
    int32_t size;
};


typedef struct diag_t diag_t;
struct diag_t {
    pthread_mutex_t lock;
    diag_value_t *values;
    event_loop_timer_t *heartbeat_timer;
    event_loop_timer_t *report_events_timer;
    event_loop_timer_t *report_twins_timer;
    event_loop_timer_t *report_telemetry_timer;
    event_loop_timer_t *report_log_timer;
    event_loop_timer_t *led_update_timer;

    struct timespec ts_app_start;
    struct timespec ts_last_d2c;
    event_t events[MAX_EVENT_IN_MEMORY];
    int num_events;
    char *reported_device_twin;
    EventLoop *eloop;
};

static diag_t s_diag;

// diag telemetry
static int printf_diag_points(struct json_out *out, va_list *ap)
{
    diag_value_t *values = va_arg(*ap, struct diag_value_t *);
    int len = json_printf(out, "[");

    for (diag_value_t *point = values; point; point = point->next) {

        if (point != values) {
            len += json_printf(out, ",");
        }
        len += json_printf(out, "[%Q,\"%.0f\"]", point->key, point->value);
    }
    len += json_printf(out, "]");
    return len;
}


static char *build_diag_telemetry_message(void)
{
    char *iot_message = json_asprintf("{timestamp:%Q,name:%Q,location:%Q,point:%M}",
                                      timespec2str(now()),
                                      adapter_get_name(),
                                      adapter_get_location(),
                                      printf_diag_points, s_diag.values);

    return iot_message;
}


static void diag_telemetry_delivered(bool delivered, void *context)
{
    if (delivered) {
        clock_gettime(CLOCK_BOOTTIME, &s_diag.ts_last_d2c);
    }
}

static void diag_report_telemetry(void)
{
    char *iot_message = build_diag_telemetry_message();
    LOGI("REPORT-TELEMETRY");
    if (iot_send_message_async(iot_message, IOT_MESSAGE_TYPE_DIAG_TELEMETRY, diag_telemetry_delivered, NULL) != 0) {
        LOGE("Failed to report telemetry");
    }
    FREE(iot_message);
}

static void diag_report_telemetry_cb(void *context)
{
    if (network_is_connected() && iot_is_connected()) {
        diag_report_telemetry();
    }
}

static diag_value_t* find_diag_value(const char* key)
{
    for (diag_value_t *p = s_diag.values; p; p = p->next) {

        if (strcmp(p->key, key) == 0) {
            return p;
        }
    }

    return NULL;
}


// diag events
//1/1/2010, any time stamp before this been regraded as invalid RTC for our solution
#define RESONABLE_START_TIME 1262304000

static int printf_events(struct json_out *out, va_list *ap)
{
    event_t *events = va_arg(*ap, struct event_t *);
    int nevents = va_arg(*ap, int);
    int len = 0;

    for (int i = 0; i < nevents; i++) {
        if (len > 0) {
            len += json_printf(out, ",");
        }

        if (events[i].ts.tv_sec > RESONABLE_START_TIME) {
            int64_t epoch_ms = SPEC2MS(events[i].ts);
            len += json_printf(out, "[%llu, %d, %d]", epoch_ms, events[i].code & 0x7FFF, events[i].repeat);
        } else {
            // The time is get before time sync, it's not RTC, but power on seconds + epoch of 1/1/20000
            // ignore it for now unless we figure out a way to fix it
            fprintf(stderr, "skip unknown event\n");
        }
    }

    return len;
}

static void remove_events_upto(struct timespec *ts)
{
    int nremove = 0;
    for (int i = 0; i < s_diag.num_events; i++) {
        if (timespec_compare(&s_diag.events[i].ts, ts) > 0) break;
        nremove++;
    }

    if (nremove > 0) {
        memmove(s_diag.events, s_diag.events + nremove, (s_diag.num_events - nremove) * sizeof(event_t));
        s_diag.num_events -= nremove;
    }
}


static void event_message_delivered(bool delivered, void *context)
{
    struct timespec *last_event_ts = (struct timespec*)context;
    if (delivered) {
        LOGD("Events reported");        
        remove_events_upto(last_event_ts);
    } else {
        LOGD("Diag events report failed");
    }
    FREE(last_event_ts);
}

static int save_events_to_file(void)
{
    if (s_diag.num_events <= 0) return 0;

    event_file_hdr_t hdr;
    memcpy_s(hdr.magic, sizeof(EVENT_FILE_MAGIC), EVENT_FILE_MAGIC, sizeof(EVENT_FILE_MAGIC));
    hdr.size = s_diag.num_events * sizeof(event_t);

    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        LOGE("can't open mutable file");
        return -1;
    }

    lseek(fd, EVENT_FILE_OFFSET, SEEK_SET);
    write(fd, &hdr, sizeof(event_file_hdr_t));
    write(fd, s_diag.events, hdr.size);
    s_diag.num_events = 0;
    fsync(fd);
    close(fd);
    return 0;
}


static void log_event_internal(event_code_t code, bool flush)
{
    // avoid reentry
    if (pthread_mutex_trylock(&s_diag.lock) != 0) return;

    if ((s_diag.num_events > 0) && (code == s_diag.events[s_diag.num_events - 1].code)) {
        s_diag.events[s_diag.num_events - 1].repeat++;
        clock_gettime(CLOCK_REALTIME, &s_diag.events[s_diag.num_events - 1].ts);
        LOGD("log repeat event: [%d %d]", code, s_diag.events[s_diag.num_events - 1].repeat);
    } else {
        if (s_diag.num_events < MAX_EVENT_IN_MEMORY) {
            s_diag.events[s_diag.num_events].code = code;
            s_diag.events[s_diag.num_events].repeat = 1;
            clock_gettime(CLOCK_REALTIME, &s_diag.events[s_diag.num_events].ts);
            s_diag.num_events++;
            LOGD("log new event: [%d %d]", code, s_diag.events[s_diag.num_events - 1].repeat);
        } else {
            LOGD("event buffer full! ignore new event");
        }
    }

    if (flush) {
        save_events_to_file();
    }
    pthread_mutex_unlock(&s_diag.lock);
}


static void diag_report_events(void)
{
    if (s_diag.num_events <= 0) return;

    char *iot_message = json_asprintf("[%M]", printf_events, s_diag.events, s_diag.num_events);
    LOGI("DIAG EVENTS: %s", iot_message);
    struct timespec *ts = MALLOC(sizeof(struct timespec));
    *ts = s_diag.events[s_diag.num_events-1].ts;

    if (iot_send_message_async(iot_message,
        IOT_MESSAGE_TYPE_DIAG_EVENTS,  event_message_delivered, ts) == 0) {
        // a bit tracky here, flip MSB of event to we won't try to increase repeat number for event already inflight
        s_diag.events[s_diag.num_events-1].code |= 0x8000;
    } else {
        FREE(ts);
    }
    FREE(iot_message);
}

static void diag_report_events_cb(void *context)
{
    if (network_is_connected() && iot_is_connected()) {
        diag_report_events();
    }
}

static void clear_event_file(int fd)
{
    event_file_hdr_t hdr;
    memcpy_s(hdr.magic, sizeof(EVENT_FILE_MAGIC), EVENT_FILE_MAGIC, sizeof(EVENT_FILE_MAGIC));
    hdr.size = 0;
    lseek(fd, EVENT_FILE_OFFSET, SEEK_SET);
    write(fd, &hdr, sizeof(event_file_hdr_t));
    fdatasync(fd);
}

// return number of event loaded from file
static int load_event_from_file(int fd)
{
    event_file_hdr_t hdr;

    if (lseek(fd, EVENT_FILE_OFFSET, SEEK_SET) != EVENT_FILE_OFFSET) {
        LOGW("event file not exist");
        return 0;
    }

    if (read(fd, &hdr, sizeof(event_file_hdr_t)) != sizeof(event_file_hdr_t)) {
        LOGW("Failed to read event file hdr");
        return 0;
    }

    if (memcmp(hdr.magic, EVENT_FILE_MAGIC, sizeof(EVENT_FILE_MAGIC)) != 0) {
        LOGW("event file magic mismatch");
        return 0;
    }

    if ((hdr.size < 0) || (hdr.size > EVENT_FILE_SIZE) || (hdr.size % sizeof(event_t) != 0)) {
        LOGW("event file size incorrect");
        return 0;
    }

    if (read(fd, s_diag.events, hdr.size) != hdr.size) {
        LOGW("Failed to load event from file");
        return 0;
    }

    s_diag.num_events = hdr.size/sizeof(event_t);
    LOGD("loaded %d event from file", s_diag.num_events);

    return s_diag.num_events;
}



static int network_status_to_event(Networking_InterfaceConnectionStatus status)
{
    if (status & Networking_InterfaceConnectionStatus_ConnectedToInternet) {
        return EVENT_NETWORK_INTERNET;
    } else if (status & Networking_InterfaceConnectionStatus_IpAvailable) {
        return EVENT_NETWORK_IP_AVAILABLE;
    } else if (status & Networking_InterfaceConnectionStatus_ConnectedToNetwork) {
        return EVENT_NETWORK_LOCAL;
    } else if (status & Networking_InterfaceConnectionStatus_InterfaceUp) {
        return EVENT_NETWORK_INTERFACE_UP;
    } else {
        return EVENT_NETWORK_NO_INTERFACE;
    }
}


// log
static void diag_report_log_cb(void *context)
{
    if (llog_remote_log_enabled() && network_is_connected() && iot_is_connected()) {
        llog_upload();
    }
}

// device twin
static int32_t printf_provisions(struct json_out *out, va_list *ap)
{
    int32_t num = 0;
    int32_t len = json_printf(out, "{");

    for (ce_device_t *device = va_arg(*ap, struct ce_device_t *); device; device = device->next) {

        if (num++ > 0) {
            len += json_printf(out, ",");
        }

        len += json_printf(out, "%Q:%d", device->name, device->err);
    }

    len += json_printf(out, "}");
    return len;
}

static char *build_device_twin_to_report(void)
{
    if (!adapter_get_source_id()) return NULL;

    char wifiMac[MAX_MAC_ADDRESS_SIZE];
    char ethMac[MAX_MAC_ADDRESS_SIZE];

    network_get_mac("wlan0", wifiMac, sizeof(wifiMac));
    network_get_mac("eth0", ethMac, sizeof(ethMac));

    bool wifi_connected = network_is_interface_connected("wlan0");
    bool eth_connected = network_is_interface_connected("eth0");

    struct timespec ts_boot = {.tv_sec = 0, .tv_nsec = 0};
    int64_t boot_epoch = timespec2epoch(boottime2realtime(ts_boot));
    int64_t app_start_epoch = timespec2epoch(boottime2realtime(s_diag.ts_app_start));
    int64_t online_epoch = timespec2epoch(boottime2realtime(iot_last_online()));
    int64_t offline_epoch = timespec2epoch(boottime2realtime(iot_last_offline()));
    int64_t provisioned_epoch = timespec2epoch(boottime2realtime(adapter_last_provisioned()));

    char *twin = json_asprintf("{"
                      "name:%Q,"
                      "sourceId:%Q,"
                      "firmwareVersion:%Q,"
                      "lastBoot:%"PRId64","
                      "lastAppStart:%"PRId64","
                      "lastOnline:%"PRId64","
                      "lastOffline:%"PRId64","
                      "lastProvision:%"PRId64","
                      "wifiMac:%Q,"
                      "ethMac:%Q,"
                      "wifiConnected:%B,"
                      "ethConnected:%B,"
                      "provision:%M"
                      "}",
                      adapter_get_name(),
                      adapter_get_source_id(),
                      app_version(),
                      boot_epoch,
                      app_start_epoch,
                      online_epoch,
                      offline_epoch,
                      provisioned_epoch,
                      wifiMac,
                      ethMac,
                      wifi_connected,
                      eth_connected,
                      printf_provisions, adapter_get_devices());

    return twin;
}

static void device_twin_reported(bool delivered, void *context)
{
    char *twin_to_report = (char*)context;
    if (delivered) {
        FREE(s_diag.reported_device_twin);
        s_diag.reported_device_twin = twin_to_report;
    } else {
        FREE(twin_to_report);
    }
}

static void diag_report_twins(void)
{
    char *twin_to_report = build_device_twin_to_report();

    if (!twin_to_report) return;

    if (s_diag.reported_device_twin == NULL || strcmp(twin_to_report, s_diag.reported_device_twin) != 0) {
        LOGI("TWIN-UPDATE: %s", twin_to_report);
        if (iot_report_device_twin_async(twin_to_report, device_twin_reported, twin_to_report) != 0) {
            LOGE("Failed to report device twin");
            FREE(twin_to_report);
        }
    } else {
        FREE(twin_to_report);
    }
}

static void diag_report_twins_cb(void *context)
{
    if (network_is_connected() && iot_is_connected()) {
        diag_report_twins();
    }
}

static void report_network_status_change(Networking_InterfaceConnectionStatus status)
{
    static Networking_InterfaceConnectionStatus last_network_status = 0;
    if (status != last_network_status) {
        last_network_status = status;
        diag_log_event(network_status_to_event(status));
        LOGI("network status [%u] %s", status, network_get_status_str(status));
    }
}

// led
static void update_network_led(Networking_InterfaceConnectionStatus status)
{
    static bool network_led_on = false;
    int color = LED_OFF;
    bool flash = false;

    if (status & Networking_InterfaceConnectionStatus_ConnectedToInternet) {
        color = iot_is_connected() ? LED_GREEN : LED_YELLOW;
        flash = false;
    } else if (status & Networking_InterfaceConnectionStatus_IpAvailable) {
        color = LED_YELLOW;
        flash = true;
    } else if (status & Networking_InterfaceConnectionStatus_ConnectedToNetwork) {
        color = LED_RED;
        flash = false;
    } else if (status & Networking_InterfaceConnectionStatus_InterfaceUp) {
        color = LED_RED;
        flash = true;
    } else {
        color = LED_OFF;
        flash = false;
    }

    if (flash && network_led_on) {
        led_set_color(NETWORK_LED, LED_OFF);
        network_led_on = false;
    } else {
        led_set_color(NETWORK_LED, color);
        network_led_on = true;
    }
}

static void update_app_led(void)
{
    const int LED_COLOR_FOR_DRIVER_STATE[] = {
        LED_RED,    // DRIVER_STATE_INIT
        LED_RED,    // DRIVER_STATE_OPENED
        LED_YELLOW, // DRIVER_STATE_PARTIAL
        LED_GREEN   // DRIVER_STATE_NORMAL
    };

    static bool app_led_on = false;

    if (app_led_on) {
        led_set_color(APP_LED, LED_OFF);
        app_led_on = false;
    } else {
        uint32_t state = adapter_get_driver_state();
        led_set_color(APP_LED, LED_COLOR_FOR_DRIVER_STATE[state]);
        app_led_on = true;
    }
}

static void diag_led_update_cb(void *context)
{
    Networking_InterfaceConnectionStatus status = network_get_status();
    report_network_status_change(status);
    update_network_led(status);
    update_app_led();
}

static void detect_offline_recover_reboot(void)
{
    struct timespec ts_now;
    clock_gettime(CLOCK_BOOTTIME, &ts_now);
    if (ts_now.tv_sec - s_diag.ts_last_d2c.tv_sec < DIAG_OFFLINE_SECOND_TO_REBOOT) return;

    diag_log_event_to_file(EVENT_RECOVER_REBOOT);
    LOGI("Try recover, force system reboot in 2s...");
    sleep(2);
    PowerManagement_ForceSystemReboot();
}

static void diag_heartbeat_cb(void *context)
{
    static bool reported_memory_usage = false;
    int heartbeat = diag_log_count_value("heartbeat");
    LOGI("~~~~~heartbeat~~~~~~: %d", heartbeat);

    // log memory usage if the peak usermode memory appoaches to the limit 256k
    size_t peak_usermode_memory = Applications_GetPeakUserModeMemoryUsageInKB();
    if (peak_usermode_memory >= DIAG_PEAK_USERMODE_MEMORY_WATERMARK) {
        diag_log_value("peak_usermode_memory", peak_usermode_memory);
        diag_log_value("usermode_memory", Applications_GetUserModeMemoryUsageInKB());
        diag_log_value("total_memory", Applications_GetTotalMemoryUsageInKB());
        reported_memory_usage = true;
    }
    else if (reported_memory_usage) {
        diag_remove_value("peak_usermode_memory");
        diag_remove_value("usermode_memory");
        diag_remove_value("total_memory");
        reported_memory_usage = false;
    }

    detect_offline_recover_reboot();
}


static int init_diag_event(void)
{
    s_diag.num_events = 0;

    if (pthread_mutex_init(&s_diag.lock, NULL) != 0) {
        LOGE("mutex init has failed");
        return -1;
    }

    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        LOGE("can't open mutable file");
        return -1;
    }

    if (load_event_from_file(fd) > 0) {
        clear_event_file(fd);
    }
    close(fd);
    return 0;
}

static void detect_system_reboot(void)
{
    struct timespec ts_uptime;
    clock_gettime(CLOCK_BOOTTIME, &ts_uptime);

    // If the boot timestamp is less than DIAG_SYSTEM_BOOT_TIME, there may be a kernel reboot.
    if (ts_uptime.tv_sec <= DIAG_SYSTEM_BOOT_TIME) {
        LOGI("System Reboot!");
        diag_log_event(EVENT_SYSTEM_REBOOT);
    }
}


static int schedule_heartbeat(void)
{
    struct timespec ts_heartbeat = MS2SPEC(DIAG_HEARTBEAT_MS);
    s_diag.heartbeat_timer = event_loop_register_timer(s_diag.eloop, &ts_heartbeat, &ts_heartbeat, diag_heartbeat_cb, NULL);
    return s_diag.heartbeat_timer ? 0 : -1;
}

static int schedule_report_events(void)
{
    struct timespec ts_event = MS2SPEC(DIAG_EVENT_REPORT_MS);
    s_diag.report_events_timer = event_loop_register_timer(s_diag.eloop, &ts_event, &ts_event, diag_report_events_cb, NULL);
    return s_diag.report_events_timer ? 0 : -1;
}

static int schedule_report_twin(void)
{
    struct timespec ts_twin = MS2SPEC(DIAG_TWIN_REPORT_MS);
    s_diag.report_twins_timer = event_loop_register_timer(s_diag.eloop, &ts_twin, &ts_twin, diag_report_twins_cb, NULL);
    return s_diag.report_twins_timer ? 0 : -1;
}

static int schedule_report_telemetry(void)
{
    struct timespec ts_telemetry = MS2SPEC(DIAG_TELEMETRY_REPORT_MS);
    s_diag.report_telemetry_timer = event_loop_register_timer(s_diag.eloop, &ts_telemetry, &ts_telemetry, diag_report_telemetry_cb, NULL);
    return s_diag.report_telemetry_timer ? 0 : -1;
}

static int schedule_report_log(void)
{
    struct timespec ts_log = MS2SPEC(DIAG_LOG_REPORT_MS);
    s_diag.report_log_timer = event_loop_register_timer(s_diag.eloop, &ts_log, &ts_log, diag_report_log_cb, NULL);
    return s_diag.report_log_timer ? 0 : -1;
}

static int schedule_update_led(void)
{
    struct timespec ts_led = MS2SPEC(DIAG_LED_UPDATE_MS);
    s_diag.led_update_timer = event_loop_register_timer(s_diag.eloop, &ts_led, &ts_led, diag_led_update_cb, NULL);
    return s_diag.led_update_timer ? 0 : -1;
}

static void free_diag_values(void)
{
    while (s_diag.values) {
        diag_value_t *p = s_diag.values;
        s_diag.values = p->next;
        FREE(p->key);
        FREE(p);
    }
}

// ---------------------------- public interface ------------------------------

int diag_init(EventLoop *eloop)
{
    LOGI("diag init");

    memset(&s_diag, 0, sizeof(s_diag));
    s_diag.eloop = eloop;

    clock_gettime(CLOCK_BOOTTIME, &s_diag.ts_app_start);
    clock_gettime(CLOCK_BOOTTIME, &s_diag.ts_last_d2c);

    // need to be initialized before try to log any event
    if (init_diag_event() < 0) {
        LOGE("failed to init diag event");
        return -1;
    }

    detect_system_reboot();

    diag_log_event(EVENT_RESTART);    

    if (schedule_heartbeat() != 0) {
        LOGE("failed to schedule heart beat");
        return -1;
    }

    if (schedule_report_events() != 0) {
        LOGE("failed to schedule report events");
        return -1;
    }

    if (schedule_report_twin() != 0) {
        LOGE("failed to schedule report twin");
        return -1;
    }

    if (schedule_report_telemetry() != 0) {
        LOGE("failed to schedule report telemetry");
        return -1;
    }

    if (schedule_report_log() != 0) {
        LOGE("failed to schedule report log");
        return -1;
    }

    if (schedule_update_led() != 0) {
        LOGE("failed to schedule update led");
        return -1;
    }

    return 0;
}


void diag_deinit()
{
    free_diag_values();

    event_loop_unregister_timer(s_diag.eloop, s_diag.heartbeat_timer);
    event_loop_unregister_timer(s_diag.eloop, s_diag.report_events_timer);
    event_loop_unregister_timer(s_diag.eloop, s_diag.report_twins_timer);
    event_loop_unregister_timer(s_diag.eloop, s_diag.report_telemetry_timer);
    event_loop_unregister_timer(s_diag.eloop, s_diag.report_log_timer);
    event_loop_unregister_timer(s_diag.eloop, s_diag.led_update_timer);
    pthread_mutex_destroy(&s_diag.lock);

    FREE(s_diag.reported_device_twin);
}


double diag_get_value(const char *key)
{
    diag_value_t *p = find_diag_value(key);
    return p ? p->value : NAN;
}


void diag_remove_value(const char* key)
{
    diag_value_t* prev = NULL;
    diag_value_t* cur = s_diag.values;
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            break;
        }
        prev = cur;
        cur = cur->next;
    }

    if (cur) {
        if (prev) {
            prev->next = cur->next;
        }
        else {
            s_diag.values = cur->next;
        }
        FREE(cur->key);
        FREE(cur);
    }
}


void diag_log_value(const char *key, double value)
{
    diag_value_t *p = find_diag_value(key);

    if (p) {
        p->value = value;
    } else {
        p = (diag_value_t *)MALLOC(sizeof(diag_value_t));
        p->key = STRDUP(key);
        p->value = value;
        p->next = s_diag.values;
        s_diag.values = p;
    }
}


int diag_log_count_value(const char *key)
{
    double count = diag_get_value(key);
    count = isnan(count) ? 1 : count + 1;
    diag_log_value(key, count);
    return count;
}

int diag_get_count_value(const char *key)
{
    double count = diag_get_value(key);
    return isnan(count) ? 0 : (int)count;
}

void diag_log_event(event_code_t code)
{
    log_event_internal(code, false);
}


void diag_log_event_to_file(event_code_t code)
{
    log_event_internal(code, true);
}
