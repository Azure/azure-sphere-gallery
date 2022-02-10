/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <ctype.h>
#include <float.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <init/globals.h>
#include <init/applibs_versions.h>

#include <utils/llog.h>
#include <utils/utils.h>
#include <frozen/frozen.h>
#include <safeclib/safe_lib.h>
#include <applibs/log.h>


#define NUM_PROPS 10

struct json_property {
    struct json_token key;
    struct json_token val;
    bool visited;
};

static char FIRMWARE_VERSION[21] = { 0 };

const char* app_version(void)
{
    return FIRMWARE_VERSION;
}

void set_app_version(const char* version)
{
    strncpy_s(FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION), version, sizeof(FIRMWARE_VERSION) - 1);
}

const char *hex(const unsigned char *data, size_t len)
{
    static char buf[1000];    
    int nprint = snprintf(buf, sizeof(buf), "[");
    int n = 0;

    for (size_t i = 0; i < len; i++) {
        if ((n = snprintf(buf + nprint, sizeof(buf) - nprint, "%02x ", data[i])) < 0) {
            return NULL;
        }
        nprint += n;
    }

    snprintf(buf + nprint, sizeof(buf) - nprint, "]");
    return buf;
}



char *trim(char *s)
{
    int l = strlen(s);

    while (l && isspace(s[l - 1])) {
        --l;
    }

    while (l && *s && isspace(*s)) {
        ++s, --l;
    }

    return l > 0 ? STRNDUP(s, l) : NULL;
}


uint32_t hash(const unsigned char *buf, int32_t len)
{
    uint32_t hash = 5381;

    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + buf[i]; /* hash * 33 + c */
    }

    return hash;
}

bool is_double_equal(double a, double b)
{
    if (isnan(a) && isnan(b)) {
        return true;
    } else {
        return fabs(a - b) < DBL_EPSILON;
    }
}


bool strequal(const char* s1, const char* s2)
{
    if (s1 == NULL && s2 == NULL) {
        return true;
    } else if (s1 && s2) {
        return (strcmp(s1, s2) == 0);
    } else {
        return false;
    }
}
