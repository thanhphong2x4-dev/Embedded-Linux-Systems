/*
 * File: sensor_thread.c
 * Purpose: Read sensor data from kernel driver
 * Author: Nguyen Thanh Phong - SE182514
 * Date: 2026-10-21
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>

#include "sensor_thread.h"
#include "sms_common.h"
#include "../driver/sms_sensor.h"

/* Parse CSV: timestamp_ms,temperature,humidity */
static int parse_sensor_csv(const char *csv, SensorData *sample)
{
    unsigned long long timestamp;
    double temperature;
    double humidity;

    if (sscanf(csv, "%llu,%lf,%lf",
               &timestamp, &temperature, &humidity) != 3)
        return -1;

    sample->timestamp_ms = (uint64_t)timestamp;
    sample->temperature = temperature;
    sample->humidity = humidity;

    return 0;
}

/* Sensor acquisition thread */
void *sensor_thread_func(void *arg)
{
    SensorThreadContext *ctx = arg;
    struct timespec next;
    int fd;
    unsigned long read_ok = 0;
    unsigned long eagain_count = 0;
    unsigned long eintr_count = 0;
    unsigned long other_errors = 0;

    fd = open(SENSOR_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("sensor: open");
        return NULL;
    }

    if (ioctl(fd, SMS_SET_INTERVAL, &ctx->interval_ms) < 0) {
        perror("sensor: ioctl");
        close(fd);
        return NULL;
    }

    clock_gettime(CLOCK_MONOTONIC, &next);

    while (*(ctx->running)) {
        char csv[128] = {0};
        SensorData sample;
        ssize_t n;
        int ret;

        /* Reopen to reset the per-file read offset */
        close(fd);
        fd = open(SENSOR_DEVICE, O_RDONLY);

        if (fd < 0) {
            perror("sensor: reopen");
            break;
        }

        n = read(fd, csv, sizeof(csv) - 1);

        if (n > 0) {
            read_ok++;
            csv[n] = '\0';

            if (parse_sensor_csv(csv, &sample) == 0) {
                ret = circular_buffer_push(ctx->buffer, &sample);

                if (ret == ENOSPC)
                    fprintf(stderr, "[SENSOR] Buffer overflow\n");
                else if (ret == 0 && ctx->verbose)
                    printf("[SENSOR] %s", csv);
            }
        } else if (n < 0) {
            if (errno == EAGAIN)
                eagain_count++;
            else if (errno == EINTR)
                eintr_count++;
            else {
                other_errors++;
                perror("sensor: read");
            }
        }

        /* Schedule next sampling time */
        next.tv_nsec += (long)(ctx->interval_ms % 1000) * 1000000L;
        next.tv_sec += ctx->interval_ms / 1000;

        if (next.tv_nsec >= 1000000000L) {
            next.tv_sec++;
            next.tv_nsec -= 1000000000L;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    printf("[SENSOR STATS] Successful reads: %lu\n", read_ok);
    printf("[SENSOR STATS] EAGAIN: %lu\n", eagain_count);
    printf("[SENSOR STATS] EINTR: %lu\n", eintr_count);
    printf("[SENSOR STATS] Other errors: %lu\n", other_errors);

    close(fd);
    return NULL;
}
