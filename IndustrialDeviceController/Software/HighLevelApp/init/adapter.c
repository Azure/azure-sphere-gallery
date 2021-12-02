/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>

#include <init/adapter.h>
#include <init/device_hal.h>
#include <init/globals.h>
#include <iot/diag.h>
#include <iot/iot.h>
#include <utils/event_loop_timer.h>
#include <utils/llog.h>
#include <utils/memory.h>
#include <utils/network.h>
#include <utils/timer.h>
#include <utils/utils.h>
#include <utils/led.h>

#include <frozen/frozen.h>
#include <safeclib/safe_lib.h>

extern volatile bool g_app_running;

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

#define MAX_CONNECTION_STRING_SIZE 100

const char PROV_FILE_MAGIC[8] = {'P', 'R', 'O', 'V', ' ', 'V', '0', '1'};

struct prov_file_hdr_t {
    char magic[8]; // "PROV V01"
    uint32_t hashcode;
    int32_t size;
};


typedef struct adapter_t adapter_t;
struct adapter_t {
    int64_t provision_epoch;
    char* name;
    char* location;
    char* source_id;

    int32_t num_device;
    int32_t num_schema;

    link_t uplink;
    link_t downlink;

    ce_device_t* devices;
    data_schema_t* schemas;
    device_driver_t *driver;
    uint32_t driver_state;

    struct timespec last_provisioned;

    pthread_t worker_tid;
    // worker thread -> main thread RESULT queue
    int result_pipe[2];

    EventRegistration *result_io;
    event_loop_timer_t *notify_timer;
    EventLoop *eloop;

    char *pending_provision;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    ce_device_t *pending_device;
    ce_device_t *ready_device;
};


static adapter_t s_adapter;


// load local provision, return hashcode and null terminated provision
// caller response to free provision
static int load_local_provision(char **p_provision)
{
    ASSERT(p_provision);

    struct prov_file_hdr_t hdr;
    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        LOGE("Failed to open mutable storage");
        return -1;
    }

    if (lseek(fd, PROVISION_FILE_OFFSET, SEEK_SET) != PROVISION_FILE_OFFSET) {
        LOGW("Prvoision file not exit");
        close(fd);
        return -1;
    }

    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        LOGE("Failed to read prov file header");
        close(fd);
        return -1;
    }

    if (memcmp(hdr.magic, PROV_FILE_MAGIC, sizeof(PROV_FILE_MAGIC)) != 0) {
        LOGW("provision file magic mismatch");
        close(fd);
        return -1;
    }

    if (hdr.size < 0 || hdr.size > PROVISION_FILE_MAX_SIZE) {
        LOGW("provision file size invalid");
        close(fd);
        return -1;
    }

    char *provision = (char *)MALLOC(hdr.size + 1);

    if (read(fd, provision, hdr.size) != hdr.size) {
        close(fd);
        LOGW("Failed to read provision content");
        FREE(provision);
        return -1;
    }

    close(fd);
    provision[hdr.size] = 0;

    if (hash(provision, hdr.size) != hdr.hashcode) {
        LOGW("provision hashcode not match");
        FREE(provision);
        return -1;
    }

    *p_provision = provision;
    LOGI("load_local_provision: hash=%x", hdr.hashcode);
    return 0;
}


static int save_local_provision(const char *provision, size_t provision_size)
{
    ASSERT(provision);

    int fd = Storage_OpenMutableFile();
    if (fd < 0) {
        LOGE("Failed to open mutable storage");
        return -1;
    }

    struct prov_file_hdr_t hdr;
    memcpy_s(hdr.magic, sizeof(hdr.magic), PROV_FILE_MAGIC, sizeof(PROV_FILE_MAGIC));
    hdr.size = provision_size;
    hdr.hashcode = hash(provision, provision_size);
    lseek(fd, PROVISION_FILE_OFFSET, SEEK_SET);

    if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        close(fd);
        LOGE("Failed to write provision header");
        return -1;
    }

    if (write(fd, provision, hdr.size) != hdr.size) {
        close(fd);
        LOGE("Failed to write provision content");
        return -1;
    }

    fsync(fd);
    close(fd);

    LOGI("save_local_provision: hash=%x", hdr.hashcode);
    return 0;
}

// double to str and trim tailing '0's
static char *double_to_str(double d)
{
    static char buf[TELEMETRY_MAX_VALUE_SIZE];
    int nchar = snprintf(buf, sizeof(buf), "%.3f", d);

    for (int i = nchar - 1; i; i--) {
        if (buf[i] == '0') {
            buf[i] = 0;
        } else if (buf[i] == '.') {
            buf[i] = 0;
            break;
        } else {
            break;
        }
    }
    return buf;
}

static int printf_points(struct json_out *out, va_list *ap)
{
    ce_device_t *device = va_arg(*ap, struct ce_device_t *);
    int force = va_arg(*ap, int);

    int len = json_printf(out, "[[%Q,\"%d\"]", "ERROR_CODE", device->err);

    for (int i = 0; i < device->schema->num_point; i++) {
        // skip unchanged value if COV flag set
        if (!force && !IS_COV(device->telemetry, i)) {
            continue;
        }

        if (IS_STR_VALUE(device->telemetry, i)) {
            len += json_printf(out, ",[%Q,%Q]", device->schema->points[i].key, device->telemetry->values[i].str);
        } else {
            if (isnan(device->telemetry->values[i].num)) {
                len += json_printf(out, ",[%Q,%s]", device->schema->points[i].key, "null");
            } else {
                len += json_printf(out, ",[%Q,%Q]", device->schema->points[i].key, double_to_str(device->telemetry->values[i].num));
            }
        }
    }

    len += json_printf(out, "]");
    return len;
}


static struct timespec calc_telemetry_timestamp(const ce_device_t *device)
{
    struct timespec ts = now();

    if (device->schema->flags & FLAG_CE_TIMESTAMP) {
        for (int i = 0; i < device->schema->num_point; i++) {
            if (IS_NUM_VALUE(device->telemetry, i) &&
                (strcasecmp(device->schema->points[i].key, "timestamp") == 0) &&
                (!isnan(device->telemetry->values[i].num))) {
                ts.tv_sec = device->telemetry->values[i].num;
                ts.tv_nsec = 0;
            }
        }
    }
    return ts;
}

static char *build_telemetry_message(const ce_device_t *device, bool force)
{
    ASSERT(device);

    return json_asprintf("{timestamp:%Q,name:%Q,location:%Q,point:%M}",
                                      timespec2str(calc_telemetry_timestamp(device)),
                                      device->name,
                                      device->location,
                                      printf_points, device, force);
}

static void telemetry_message_delivered(bool delivered, void *context)
{
    if (delivered) {
        LOGI("Telemetry delivered");
    } else {
        LOGW("Telemetry delivery failed");
        diag_log_event(EVENT_TELEMETRY_FAILED);
    }
}


static void send_telemetry_message(const ce_device_t *device, bool force)
{
    ASSERT(device);

    LOGI("[%s] Send telemetry to iothub, status=%s", err_str(device->err));
    char *message = build_telemetry_message(device, force);

    const char *message_type = IOT_MESSAGE_TYPE_TELEMETRY;

    int err = iot_send_message_async(message, message_type, telemetry_message_delivered, NULL);
    if (err != 0) {
        LOGW("Failed to send telemetry message");
        diag_log_event(EVENT_TELEMETRY_FAILED);
    }

    FREE(message);
}

// schema name format: <name>[:<offset>][:<channel>]
static data_schema_t *parse_schema(data_schema_t *schemas, const char *schema_name)
{
    char *colon = strchr(schema_name, ':');
    size_t len = 0;

    if (colon) {
        len = colon - schema_name;
    } else {
        len = strlen(schema_name);
    }

    for (data_schema_t *schema = schemas; schema; schema = schema->next) {

        if ((len == strlen(schema->name)) && strncmp(schema->name, schema_name, len) == 0) {
            return schema;
        }
    }

    return NULL;
}


// schema name format: <name>[:<offset>][:<channel>]
static int32_t parse_schema_offset(const char *schema_name)
{
    if (!schema_name) {
        return 0;
    }

    char *offset_str = strchr(schema_name, ':');

    if (offset_str && offset_str[1]) {
        return strtol(offset_str + 1, NULL, 10);
    }

    return 0;
}

// schema name format: <name>[:<offset>][:<channel>]
// this will be deprecated as we now get channel from device->id section
static uint32_t parse_schema_channel(const char *schema_name)
{
    if (!schema_name) {
        return 0;
    }

    char *offset_str = strchr(schema_name, ':');

    if (offset_str && offset_str[1]) {
        char *channel_str = strchr(offset_str + 1, ':');

        if (channel_str && channel_str[1]) {
            return strtol(channel_str + 1, NULL, 10);
        }
    }

    return 0u;
}


static uint32_t parse_flag(const char *flag_str, int len)
{
    if (!flag_str || len <= 0) {
        return FLAG_NONE;
    }

    if ((strlen(FLAG_NO_BATCH_STR) == len) && (strncasecmp(flag_str, FLAG_NO_BATCH_STR, len) == 0)) {
        return FLAG_NO_BATCH;
    }

    if ((strlen(FLAG_CE_TIMESTAMP_STR) == len) && (strncasecmp(flag_str, FLAG_CE_TIMESTAMP_STR, len) == 0)) {
        return FLAG_CE_TIMESTAMP;
    }

    if ((strlen(FLAG_COV_STR) == len) && (strncasecmp(flag_str, FLAG_COV_STR, len) == 0)) {
        return FLAG_COV;
    }

    return 0u;
}

static void destroy_device_telemetry(telemetry_t *telemetry)
{
    if (!telemetry) {
        return;
    }

    // free string value
    for (int i = 0; i < telemetry->num_values; i++) {
        if (IS_STR_VALUE(telemetry, i)) {
             FREE(telemetry->values[i].str);
        }
    }
    FREE(telemetry->values);
    FREE(telemetry->cov_mask);
    FREE(telemetry->str_mask);
    FREE(telemetry);
}

static telemetry_t* create_empty_device_telemetry(int num_values)
{
    telemetry_t *telemetry = CALLOC(1, sizeof(telemetry_t));
    telemetry->cov_mask = CALLOC(1, (num_values + 7) / 8);
    telemetry->str_mask = CALLOC(1, (num_values + 7) / 8);
    telemetry->values = CALLOC(num_values, sizeof(telemetry_value_t));
    telemetry->num_values = num_values;
    for (int i=0; i<num_values; i++) {
        telemetry->values[i].num = NAN;
    }
    return telemetry;
}


static void destroy_schema(data_schema_t *schema)
{
    ASSERT(schema);

    FREE(schema->name);
    if (schema->points) {
        destroy_point_table(schema->protocol, schema->points, schema->num_point);
    }
    FREE(schema);
}


static void destroy_device(ce_device_t *device)
{
    ASSERT(device);

    FREE(device->name);
    FREE(device->connection);
    FREE(device->location);

    destroy_device_telemetry(device->telemetry);
    device->telemetry = NULL;

    FREE(device);
}


static ce_device_t *get_next_device_to_run(ce_device_t *current)
{
    // nothing to schedule
    if (!s_adapter.devices) {
        return NULL;
    }

    // schedule first device
    if (!current) {
        return s_adapter.devices;
    }

    // if only one device, schedule that one again
    if (s_adapter.devices->next == NULL) {
        return s_adapter.devices;
    }

    // schedule next device with nearest schedule time, start from next device in line
    //  so when some device won't starving when not able to catch up
    ce_device_t *start = current->next ? current->next : s_adapter.devices;
    ce_device_t *device = start;
    ce_device_t *next = start;

    do {
        if (timespec_compare(&device->ts_schedule, &next->ts_schedule) < 0) {
            next = device;
        }

        device = device->next ? device->next : s_adapter.devices;
    } while (device != start);

    return next;
}


static void query_device(ce_device_t *device)
{
    ASSERT(device);
    ASSERT(device->schema);

    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    // since we reuse schema for multiple devices, set schema offest from current device
    // before we query device with that schema
    device->schema->offset = device->schema_offset;

    if (!device->telemetry) {
        device->telemetry = create_empty_device_telemetry(device->schema->num_point);
    }

    if (s_adapter.driver_state == DRIVER_STATE_INIT) {
        LOGI("Open device driver");
        if (s_adapter.driver->driver_open(s_adapter.driver, device->id, device->timeout) != DEVICE_OK) {
            LOGE("Failed to open driver");
            return;
        }
    }

    device->err = s_adapter.driver->get_point_list(s_adapter.driver, device->id, device->schema, device->telemetry, device->timeout);

    if (device->err) {
        LOGE("[%s] Read points failed: %s", device->name, err_str(device->err));
        return;
    }
    device->poll_duration = timer_stopwatch_stop(&poll_sw);
    LOGI("[%s] Read points in %d ms", device->name, device->poll_duration);
}


static void reset_adapter(adapter_t *adapter)
{
    ASSERT(adapter);

    FREE(adapter->name);
    FREE(adapter->location);
    FREE(adapter->source_id);
    FREE(adapter->uplink.if_name);
    FREE(adapter->uplink.if_data);
    FREE(adapter->downlink.if_name);
    FREE(adapter->downlink.if_data);

    while (adapter->devices) {
        ce_device_t *device = adapter->devices;
        adapter->devices = adapter->devices->next;
        destroy_device(device);
    }

    while (adapter->schemas) {
        data_schema_t *schema = adapter->schemas;
        adapter->schemas = adapter->schemas->next;
        destroy_schema(schema);
    }

    if (adapter->driver) {
        if (adapter->driver_state & DRIVER_STATE_OPENED) {
            adapter->driver->driver_close(adapter->driver);
        }
        destroy_driver(adapter->driver);
        adapter->driver = NULL;
    }

    adapter->provision_epoch = 0;
    adapter->num_device = 0;
    adapter->num_schema = 0;
    adapter->pending_device = NULL;
    adapter->ready_device = NULL;
    adapter->driver_state = DRIVER_STATE_INIT;
}


static void scan_link(const char *str, int len, void *user_data)
{
    link_t *link = (link_t *)user_data;
    json_scanf(str, len, "{interface:%Q, data:%Q}", &link->if_name, &link->if_data);
}

static void scan_protocol(const char *str, int len, void *user_data)
{
    data_schema_t *schema = (data_schema_t*)user_data;
    char *str_protocol = STRNDUP(str, len);
    schema->protocol = str2protocol(str_protocol);
    FREE(str_protocol);
}


static void scan_flags(const char *str, int len, void *user_data)
{
    data_schema_t *schema = (data_schema_t *)user_data;
    struct json_token t_flag;

    for (int i = 0; json_scanf_array_elem(str, len, "", i, &t_flag) > 0; i++) {
        schema->flags |= parse_flag(t_flag.ptr, t_flag.len);
    }
}



static void scan_schema_array(const char *str, int len, void *user_data)
{
    if (!str || len <= 0 || !user_data) {
        return;
    }

    struct json_token t = {.ptr=NULL, .len=0, .type=JSON_TYPE_INVALID};
    adapter_t *adapter = (adapter_t *)user_data;

    // parse schemas array
    for (int i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
        struct json_token t_points_def = {.ptr=NULL, .len=0, .type=JSON_TYPE_INVALID};
        data_schema_t *schema = (data_schema_t *)CALLOC(1, sizeof(data_schema_t));

        json_scanf(t.ptr, t.len, "{name:%Q, protocol:%M, interval:%d, timeout:%d, flags:%M, points:%T}",
                   &schema->name,
                   scan_protocol, schema,
                   &schema->interval,
                   &schema->timeout,
                   scan_flags, schema,
                   &t_points_def);

        if (! schema->name) {
            LOGE("missing schema name");
            destroy_schema(schema);
            return;
        }

        if (schema->protocol == DEVICE_PROTOCOL_INVALID) {
            LOGE("invalid schema protocol");
            destroy_schema(schema);
            return;
        }

        if (schema->interval <= 0) {
            LOGE("invalid schema interval");
            destroy_schema(schema);
            return;
        }

        if (schema->timeout <= 0) {
            LOGE("invalid schema timeout");
            destroy_schema(schema);
            return;
        }

        if (create_point_table(schema->protocol, &t_points_def, &schema->num_point, &schema->points) != DEVICE_OK) {
            LOGE("invalid points defintions");
            destroy_schema(schema);
            return;
        }

        schema->integrity_period_ms = DEFAULT_INTEGRITY_PERIOD_MS;
        schema->next = adapter->schemas;
        adapter->schemas = schema;
        adapter->num_schema++;

        LOGI("Add schema [name=%s, protocol=%s, interval=%ld, timeout=%ld]", schema->name,
             protocol2str(schema->protocol), schema->interval, schema->timeout);
    }
}

static const char *get_connection_string(ce_device_t *device)
{
    static char conn_str[MAX_CONNECTION_STRING_SIZE];

    if (device->connection && s_adapter.downlink.if_data) {
        snprintf(conn_str, sizeof(conn_str), "%s,%s", device->connection, s_adapter.downlink.if_data);
        return conn_str;
    } else if (s_adapter.downlink.if_data) {
        return s_adapter.downlink.if_data;
    } else if (device->connection) {
       return device->connection;
    } else {
        return NULL;
    }
}

static bool is_compatible_driver(device_protocol_t protocol, device_driver_t *driver)
{
    device_protocol_t driver_protocol = driver->get_protocol(driver);

    return (protocol == driver_protocol);
}

static int find_or_create_driver(adapter_t *adapter, ce_device_t *device)
{
    if (!adapter->driver) {
        const char* conn_str = get_connection_string(device);
        if ((conn_str == NULL) || (adapter->driver = create_driver(device->protocol, conn_str)) == NULL) {
            LOGE("failed to create driver");
            return -1;
        }
    } else {
        if (!is_compatible_driver(device->protocol, adapter->driver)) {
            return -1;
        }
    }
    return 0;
}

static void scan_device_array(const char *str, int len, void *user_data)
{
    if (!str || len <= 0 || !user_data) {
        return;
    }

    adapter_t *adapter = (adapter_t *)user_data;
    struct json_token t;

    for (int i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
        char *schema_name = NULL;
        char *device_id = NULL;

        ce_device_t *device = (ce_device_t *)CALLOC(1, sizeof(ce_device_t));
        json_scanf(t.ptr, t.len, "{name:%Q, schema:%Q, id:%Q, connection:%Q, location:%Q, interval:%d, timeout:%d}",
                   &device->name,
                   &schema_name,
                   &device_id,
                   &device->connection,
                   &device->location,
                   &device->interval,
                   &device->timeout);

        // dealling with schema_name and device id first, so we don't bother to
        // worry about free them in error case
        if (schema_name) {
            device->schema = parse_schema(adapter->schemas, schema_name);
            device->schema_offset = parse_schema_offset(schema_name);
            device->id = parse_schema_channel(schema_name);
            FREE(schema_name);
        }

        if (device_id) {
            device->id = strtol(device_id, NULL, 10);
            FREE(device_id);
        }

        if (!device->name) {
            LOGE("missing device name");
            destroy_device(device);
            return;
        }

        if (!device->schema) {
            LOGE("invalid device schema");
            destroy_device(device);
            return;
        }

        if (!device->location && adapter->location) {
            device->location = STRDUP(adapter->location);
        }

        if (device->interval <= 0) {
            device->interval = device->schema->interval;
        }

        if (device->timeout <= 0) {
            device->timeout = device->schema->timeout;
        }

        device->protocol = device->schema->protocol;

        device->telemetry = NULL;

        if (find_or_create_driver(adapter, device) != 0) {
            LOGE("failed to find or create device driver");
            destroy_device(device);
            return;
        }

        device->err = DEVICE_E_INVALID;
        device->next = adapter->devices;
        adapter->devices = device;
        adapter->num_device++;

        LOGI("Add device [name=%s, schema=%s, interval=%ld, timeout=%ld]", device->name, device->schema->name,
             device->interval, device->timeout);
    }
}


static void scan_provision(const char *str, int len, void *user_data)
{
    if (!str || !len || !user_data) {
        LOGE("invalid input");
        return;
    }

    adapter_t *adapter = (adapter_t *)user_data;

    json_scanf(str, len, "{name:%Q,location:%Q,sourceId:%Q,uplink:%M,downlink:%M}",
               &adapter->name,
               &adapter->location,
               &adapter->source_id,
               scan_link, &adapter->uplink,
               scan_link, &adapter->downlink);

    // some of the schema, device field depend on adapter properties, so make
    // sure they been scan first
    json_scanf(str, len, "{schemas:%M,devices:%M}",
               scan_schema_array, adapter,
               scan_device_array, adapter);
}


static bool is_adapter_valid(adapter_t *adapter)
{
    if (!adapter) {
        return false;
    }

    if (!adapter->name) {
        LOGE("missing adapter name");
        return false;
    }

    if (!adapter->location) {
        LOGE("missing adapter location");
        return false;
    }

    if (!adapter->source_id) {
        LOGE("missing adapter source id");
        return false;
    }

    return true;
}


static void notify_worker_locked(void *device)
{
    s_adapter.pending_device = device;
    pthread_cond_signal(&s_adapter.cond);
}

static void notify_worker_callback(void *device)
{
    pthread_mutex_lock(&s_adapter.mutex);
    notify_worker_locked(device);
    pthread_mutex_unlock(&s_adapter.mutex);
}

static void distribute_device_query_time(void)
{
    struct timespec ts_start;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    for (ce_device_t *device = s_adapter.devices; device; device = device->next) {
        device->ts_schedule = ts_start;
        device->last_flush_ts.tv_nsec = device->last_flush_ts.tv_sec = 0;
        struct timespec ts_timeout = MS2SPEC(200);
        timespec_add(&ts_start, &ts_timeout);
    }
}


static int64_t consume_epoch_from_result_pipe(int fd)
{
    int64_t epoch = 0;
    if (read(fd, &epoch, sizeof(epoch)) < 0) {
        LOGE("Failed to retrive device result");
        return -1;
    }

    return epoch;
}

static void report_device_telemetry(ce_device_t *device)
{
    bool force = false;

    // force flush all data points if reach integrity period or don't support COV
    if (device->schema->flags & FLAG_COV) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        int64_t ms_now = SPEC2MS(ts);
        int64_t ms_last = SPEC2MS(device->last_flush_ts);
        if (!ms_last || (ms_now - ms_last > device->schema->integrity_period_ms)) {
            force = true;
            device->last_flush_ts = ts;
        }
    } else {
        force = true;
    }

    send_telemetry_message(device, force);

    // if we don't need to support COV for this device, free up all telemetry
    if (!(device->schema->flags & FLAG_COV)) {
        destroy_device_telemetry(device->telemetry);
        device->telemetry = NULL;
    }
}

static void infer_driver_state(void)
{
    int ndevice_in_state[DEVICE_E_LAST];
    memset(&ndevice_in_state, 0, sizeof(ndevice_in_state));

    for (ce_device_t *dev = s_adapter.devices; dev; dev = dev->next) {
        ndevice_in_state[dev->err]++;
    }
    if (ndevice_in_state[DEVICE_E_INVALID] == s_adapter.num_device) {
        s_adapter.driver_state = DRIVER_STATE_INIT;
    } else if (ndevice_in_state[DEVICE_OK] == s_adapter.num_device) {
        s_adapter.driver_state = DRIVER_STATE_NORMAL;
    } else if (ndevice_in_state[DEVICE_E_TIMEOUT] == s_adapter.num_device || ndevice_in_state[DEVICE_E_BROKEN] > 0) {
        s_adapter.driver_state = DRIVER_STATE_OPENED;
    } else {
        s_adapter.driver_state = DRIVER_STATE_PARTIAL;
    }
}

static void schedule_next_device(ce_device_t *current)
{
    ce_device_t *next = get_next_device_to_run(current);

    if (next) {
        struct timespec ts_next_task, ts_now;
        clock_gettime(CLOCK_MONOTONIC, &ts_now);

        if (timespec_compare(&next->ts_schedule, &ts_now) > 0) {
            // when schedule task, data pointed by param will not been copy over, maintain a static copy
            // so it's still avaiable when actual posting
            ts_next_task = next->ts_schedule;
            timespec_subtract(&ts_next_task, &ts_now);
            event_loop_set_timer_and_context(s_adapter.notify_timer, &ts_next_task, NULL, next);
        } else {
            // when timer running late, reset next run to current time and schedule immediately
            // no need to catch up, this to ensure next poll happens with specified interval
            LOGW("Timer for %s running late", next->name);
            next->ts_schedule = ts_now;
            notify_worker_locked(next);
        }
    }
}

static void handle_device_result(EventLoop *eloop, int fd, EventLoop_IoEvents events, void *context)
{
    if (consume_epoch_from_result_pipe(fd) != s_adapter.provision_epoch) {
        LOGW("Stale result, ignore");
        return;
    }

    pthread_mutex_lock(&s_adapter.mutex);

    if (s_adapter.ready_device) {
        report_device_telemetry(s_adapter.ready_device);

        infer_driver_state();

        if (s_adapter.ready_device->err == DEVICE_OK) {
            diag_log_value(s_adapter.ready_device->name, s_adapter.ready_device->poll_duration);
        }
    }

    schedule_next_device(s_adapter.ready_device);
    s_adapter.ready_device = NULL;

    pthread_mutex_unlock(&s_adapter.mutex);
    MEMORY_REPORT(0);
}


static void post_epoch_to_result_pipe(void)
{
    if (write(s_adapter.result_pipe[PIPE_WRITE_END],
            &s_adapter.provision_epoch, sizeof(s_adapter.provision_epoch)) <= 0) {
        LOGE("Failed to post result");
    }
}

static void *device_worker_thread(void *vargp)
{
    while (g_app_running) {
        pthread_mutex_lock(&s_adapter.mutex);
        if (!s_adapter.pending_device) {
            pthread_cond_wait(&s_adapter.cond, &s_adapter.mutex);
        }
        if (s_adapter.pending_device) {
            // use device->schedule instead of now() to avoid drifting
            struct timespec ts_interval = MS2SPEC(s_adapter.pending_device->interval);
            timespec_add(&s_adapter.pending_device->ts_schedule, &ts_interval);
            query_device(s_adapter.pending_device);

            s_adapter.ready_device = s_adapter.pending_device;
            s_adapter.pending_device = NULL;

            post_epoch_to_result_pipe();
        }
        pthread_mutex_unlock(&s_adapter.mutex);
    }
    return NULL;
}

static void apply_local_provision(void)
{
    s_adapter.provision_epoch = 0;
    char *local_provision = NULL;
    if (load_local_provision(&local_provision) == 0) {
        adapter_provision(local_provision, strlen(local_provision), false);
        FREE(local_provision);
    }
}
// ---------------------------- public interface ------------------------------

int adapter_init(EventLoop *eloop)
{
    LOGI("adapter init");

    s_adapter.eloop = eloop;

    if (pipe(s_adapter.result_pipe) == -1) {
        LOGE("Failed to create device result queue");
        return -1;
    }

    s_adapter.result_io = EventLoop_RegisterIo(eloop, s_adapter.result_pipe[PIPE_READ_END], EventLoop_Input, handle_device_result, NULL);
    if (s_adapter.result_io < 0) {
        LOGE("Failed to register event loop for device result");
        return -1;
    }

    s_adapter.notify_timer = event_loop_register_timer(eloop, NULL, NULL, notify_worker_callback, NULL);
    if (!s_adapter.notify_timer) {
        LOGE("Failed to register notify timer for device");
        return -1;
    }

    if (pthread_mutex_init(&s_adapter.mutex, NULL) != 0) {
        LOGE("Failed to create mutex");
        return -1;
    }

    if (pthread_cond_init(&s_adapter.cond, NULL) != 0) {
        perror("pthread_cond_init() error");
        return -1;
    }

    // thread need to be created after pipe open
    if (pthread_create(&s_adapter.worker_tid, NULL, device_worker_thread, &s_adapter) != 0) {
        LOGE("Failed to create worker thread");
        return -1;
    }

    apply_local_provision();

    return 0;
}


void adapter_deinit()
{
    pthread_cond_signal(&s_adapter.cond);
    pthread_join(s_adapter.worker_tid, NULL);
    pthread_mutex_destroy(&s_adapter.mutex);
    pthread_cond_destroy(&s_adapter.cond);

    EventLoop_UnregisterIo(s_adapter.eloop, s_adapter.result_io);
    close(s_adapter.result_pipe[PIPE_READ_END]);
    close(s_adapter.result_pipe[PIPE_WRITE_END]);
    event_loop_unregister_timer(s_adapter.eloop, s_adapter.notify_timer);

    reset_adapter(&s_adapter);
}

void adapter_provision(const char *provision, size_t provision_size, bool flush)
{
    ASSERT(provision);

    int64_t epoch;
    json_scanf(provision, provision_size, "{epoch:%lld}", &epoch);
    LOGD("adapter_provision: epoch=%lld", epoch);

    iot_report_device_twin_async("{\"provision\":null}", NULL, NULL);

    pthread_mutex_lock(&s_adapter.mutex);

    if (epoch == s_adapter.provision_epoch) {
        diag_log_event(EVENT_PROVISION);
        LOGI("provision is not changed");
    }
    else {
        reset_adapter(&s_adapter);
        json_scanf(provision, provision_size, "{data:%M}", scan_provision, &s_adapter);
        event_loop_cancel_timer(s_adapter.notify_timer);

        if (is_adapter_valid(&s_adapter)) {
            network_config(&s_adapter.uplink, &s_adapter.downlink);
            if (flush) {
                save_local_provision(provision, provision_size);
            }
            s_adapter.provision_epoch = epoch;

            if (s_adapter.num_device > 0) {
                distribute_device_query_time();
                notify_worker_locked(s_adapter.devices);
            }
            diag_log_event(EVENT_PROVISION);
            LOGI("provision succeed");
        }
        else {
            reset_adapter(&s_adapter);
            diag_log_event(EVENT_PROVISION_FAILED);
            LOGE("provision failed");
        }
    }

    clock_gettime(CLOCK_BOOTTIME, &s_adapter.last_provisioned);
    pthread_mutex_unlock(&s_adapter.mutex);
    MEMORY_REPORT(0);
}

const char *adapter_get_name(void)
{
    return s_adapter.name;
}

const char* adapter_get_location(void)
{
    return s_adapter.location;
}

const char* adapter_get_source_id(void)
{
    return s_adapter.source_id;
}


ce_device_t *adapter_get_devices(void)
{
    return s_adapter.devices;
}

struct timespec adapter_last_provisioned(void)
{
    return s_adapter.last_provisioned;
}


uint32_t adapter_get_driver_state(void)
{
    return s_adapter.driver_state;
}
