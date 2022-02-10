/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdint.h>
#include <stdio.h>

#include <utils/timer.h>

struct timespec boottime2realtime(struct timespec ts_bt)
{
    struct timespec now_rt;
    clock_gettime(CLOCK_REALTIME, &now_rt);

    struct timespec now_bt;
    clock_gettime(CLOCK_BOOTTIME, &now_bt);

    struct timespec rt;
    rt.tv_sec = now_rt.tv_sec + (ts_bt.tv_sec - now_bt.tv_sec);
    rt.tv_nsec = now_rt.tv_nsec + (ts_bt.tv_nsec - now_bt.tv_nsec);

    if (rt.tv_nsec < 0) {
        rt.tv_sec--;
        rt.tv_nsec += 1e9;
    } else if (rt.tv_nsec > 1e9) {
        rt.tv_sec++;
        rt.tv_nsec -= 1e9;
    }

    return rt;
}

struct timespec now(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec;
}

const char *timespec2str(struct timespec spec)
{
    static char buf[40];
    time_t s = spec.tv_sec;
    long ms = (spec.tv_nsec + 5e5) / 1e6;

    if (ms > 999) {
        s++;
        ms = 0;
    }

    struct tm tm;
    gmtime_r(&s, &tm);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03ld", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, ms);

    return buf;
}


int64_t timespec2epoch(struct timespec spec)
{
    return (int64_t)spec.tv_sec + (spec.tv_nsec + 5e8) / 1e9;
}


// Time function utilities.
int timespec_compare(const struct timespec *s1, const struct timespec *s2)
{
    if (s1->tv_sec > s2->tv_sec) {
        return 1;
    } else if (s1->tv_sec < s2->tv_sec) {
        return -1;
    } else {
        if (s1->tv_nsec > s2->tv_nsec) {
            return 1;
        }
        else if (s1->tv_nsec < s2->tv_nsec) {
            return -1;
        }
        else {
            return 0;
        }
    }
}

// add s2 to s1
void timespec_add(struct timespec *s1, const struct timespec *s2)
{
    s1->tv_sec += s2->tv_sec;
    s1->tv_nsec += s2->tv_nsec;

    if (s1->tv_nsec > 1e9) {
        s1->tv_sec++;
        s1->tv_nsec -= 1e9;
    }
}


// subtract s2 from s1
void timespec_subtract(struct timespec *s1, const struct timespec *s2)
{
    s1->tv_sec -= s2->tv_sec;
    s1->tv_nsec -= s2->tv_nsec;

    if (s1->tv_nsec < 0) {
        s1->tv_sec--;
        s1->tv_nsec += 1e9;
    }
}
