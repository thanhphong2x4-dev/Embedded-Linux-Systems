/*
 * Host-only mock sensor for Valgrind testing.
 * Does not access /dev/sms_sensor.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#include "sensor_thread.h"

static void advance_time(struct timespec *ts, unsigned int interval_ms)
{
    ts->tv_sec += interval_ms / 1000;
    ts->tv_nsec += (long)(interval_ms % 1000) * 1000000L;

    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

void *sensor_thread_func(void *arg)
{
    SensorThreadContext *ctx = arg;
    struct timespec next;
    unsigned long produced = 0;

    if (clock_gettime(CLOCK_MONOTONIC, &next) != 0) {
        perror("[MOCK SENSOR] clock_gettime");
        return NULL;
    }

    while (*(ctx->running)) {
        SensorData sample;
        int ret;

        sample.timestamp_ms =
            (uint64_t)next.tv_sec * 1000ULL +
            (uint64_t)next.tv_nsec / 1000000ULL;

        /* Simulated values within the driver's valid ranges */
        sample.temperature = 20.0 + (produced % 26);
        sample.humidity = 40.0 + (produced % 51);

        ret = circular_buffer_push(ctx->buffer, &sample);

        if (ret == 0) {
            produced++;

            if (ctx->verbose) {
                printf("[MOCK SENSOR] Temp=%.1f Hum=%.1f\n",
                       sample.temperature, sample.humidity);
            }
        } else if (ret == ECANCELED) {
            break;
        } else if (ret != ENOSPC) {
            fprintf(stderr, "[MOCK SENSOR] push error: %d\n", ret);
            break;
        }

        advance_time(&next, ctx->interval_ms);

        do {
            ret = clock_nanosleep(
                CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL
            );
        } while (ret == EINTR && *(ctx->running));

        if (ret != 0 && ret != EINTR) {
            fprintf(stderr, "[MOCK SENSOR] sleep error: %d\n", ret);
            break;
        }
    }

    printf("[MOCK SENSOR] Samples generated: %lu\n", produced);
    return NULL;
}
