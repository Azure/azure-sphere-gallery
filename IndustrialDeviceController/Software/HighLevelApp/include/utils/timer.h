/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

/**
 * convert ms to timespec
 * @param ms ms to convert
 * @return timespec converted
 */
static inline struct timespec MS2SPEC(long ms) {
    struct timespec ts = {ms / 1000, ((ms) % 1000) * 1000000};
    return ts;
}

/**
 * convert timespec to ms
 * @param ts timespec to convert
 * @return ms converted
 */
static inline int64_t SPEC2MS(struct timespec ts)
{
    return ts.tv_sec * 1000ll + ts.tv_nsec / 1000000;
}

/**
 * get current RTC timespec, UTC time
 */
struct timespec now(void);

/**
 * convert boot time to RTC time
 * @param ts_bt boot time
 * @return RTC timespec converted
 */
struct timespec boottime2realtime(struct timespec ts_bt);

/**
 * convert RTC timespec to human readable string for logging purpose
 * @param spec RTC timespec
 * @retrun human readable string
 */
const char *timespec2str(struct timespec spec);

/**
 * round RTC timespec to epoch seconds
 * @param spec RTC timespec
 * @return epoch seconds
 */
int64_t timespec2epoch(struct timespec spec);

/**
 * compare two timespec, return -1, 0 or 1 when s1 < s2, s1 == s2, 1 or s1 > s2
 * @param s1 first timespec
 * @param s2 second timespec
 * @return compare result, -1, 0 or 1
 */
int timespec_compare(const struct timespec *s1, const struct timespec *s2);

/**
 * add timespec, increase s1 by s2
 * @param s1 timespec to be updated
 * @param s2 timespec to add
 */
void timespec_add(struct timespec *s1, const struct timespec *s2);


/**
 * subtract timespec, decrease s1 by s2
 * @param s1 time spec to be updated
 * @param s2 time spec to subtract
 */
void timespec_subtract(struct timespec *s1, const struct timespec *s2);


/**
 * start a stop watch, after start, everytime call timer_stopwatch_stop will return ms past after start
 * @param s timespec for stop watch
 */
static inline void timer_stopwatch_start(struct timespec *s)
{
    clock_gettime(CLOCK_MONOTONIC, s);
}

/**
 * stop start watch and return time past in ms
 * @param s timespec for stopwatch
 * @return ms since stopwatch start
 */
static inline int32_t timer_stopwatch_stop(struct timespec *s)
{
    struct timespec ts_now = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    int32_t ms = (ts_now.tv_sec - s->tv_sec) * 1000 + (ts_now.tv_nsec - s->tv_nsec) / 1000000;
    return ms;
}
