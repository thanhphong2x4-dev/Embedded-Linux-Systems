/*
 * File: alert_thread.c
 * Purpose: Receive and display sensor alerts
 * Author: Nguyen Thanh Phong - SE182514
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <mqueue.h>

#include "alert_thread.h"

/* Convert alert type to readable text */
static const char *alert_type_name(AlertType type)
{
    switch (type) {
    case TEMP_HIGH:
        return "HIGH TEMPERATURE";
    case HUMID_HIGH:
        return "HIGH HUMIDITY";
    case TEMP_SPIKE:
        return "TEMPERATURE SPIKE";
    default:
        return "UNKNOWN ALERT";
    }
}

/* Receive and display alerts */
void *alert_thread_func(void *arg)
{
    AlertThreadContext *ctx = arg;
    AlertData alert;
    struct mq_attr attr;

    while (*(ctx->running) || (mq_getattr(ctx->alert_queue, &attr) == 0 && attr.mq_curmsgs > 0)) {
        struct timespec timeout;

        /* mq_timedreceive uses an absolute CLOCK_REALTIME deadline */
        if (clock_gettime(CLOCK_REALTIME, &timeout) != 0) {
            perror("[ALERT] clock_gettime");
            break;
        }

        timeout.tv_sec += 1;

        ssize_t n = mq_timedreceive(
            ctx->alert_queue,
            (char *)&alert,
            sizeof(alert),
            NULL,
            &timeout
        );

        if (n == (ssize_t)sizeof(alert)) {
            ctx->received_count++;

            printf("[ALERT] %s | Value: %.1f | Threshold: %.1f | Time: %llu ms\n",
                   alert_type_name(alert.type),
                   alert.value,
                   alert.threshold,
                   (unsigned long long)alert.timestamp_ms);

            fflush(stdout);
        } else if (n < 0) {
            if (errno == EAGAIN) {
                struct timespec delay = {0, 10000000}; /* 10 ms */
                nanosleep(&delay, NULL);
                continue;
            }

            if (errno == ETIMEDOUT || errno == EINTR)
                continue;

            perror("[ALERT] mq_timedreceive");
            break;
        }
    }

    return NULL;
}
