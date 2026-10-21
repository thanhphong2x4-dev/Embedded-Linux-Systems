/*
 * File: logger_thread.c
 * Purpose: Sensor log writing and rotation
 * Author: Nguyen Thanh Phong - SE182514
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>

#include "logger_thread.h"

/* Open log file in append mode */
static FILE *open_log_file(void)
{
    FILE *fp = fopen(SMS_LOG_PATH, "a");

    if (fp == NULL)
        perror("[LOGGER] fopen");

    return fp;
}

/* Rotate log when its size exceeds 1 MB */
static FILE *rotate_log_if_needed(FILE *fp,
                                  LoggerThreadContext *ctx)
{
    long size;

    if (fp == NULL)
        return NULL;

    /* Determine the current file size */
    if (fseek(fp, 0, SEEK_END) != 0)
        return fp;

    size = ftell(fp);

    if (size < 0 || size <= SMS_LOG_MAX_SIZE)
        return fp;

    if (fclose(fp) != 0)
        perror("[LOGGER] fclose");

    /* Keep one previous log file */
    if (rename(SMS_LOG_PATH, "/tmp/sms.log.1") != 0) {
        perror("[LOGGER] rename");
        return open_log_file();
    }

    ctx->rotation_count++;

    printf("[LOGGER] Log rotated (%lu times)\n",
           ctx->rotation_count);

    return fopen(SMS_LOG_PATH, "w");
}

/* Logger thread: receive sensor samples and write them to disk */
void *logger_thread_func(void *arg)
{
    LoggerThreadContext *ctx = arg;
    FILE *fp = open_log_file();
    time_t last_flush = time(NULL);
    struct mq_attr attr;

    if (fp == NULL)
        return NULL;

    while (*(ctx->running) || (mq_getattr(ctx->log_queue, &attr) == 0 && attr.mq_curmsgs > 0)) {
        SensorData sample;
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout) != 0) {
            perror("[LOGGER] clock_gettime");
            break;
        }

        timeout.tv_sec += 1;

        ssize_t n = mq_timedreceive(
            ctx->log_queue,
            (char *)&sample,
            sizeof(sample),
            NULL,
            &timeout
        );

        if (n == (ssize_t)sizeof(sample)) {
            if (fprintf(fp, "%llu,%.1f,%.1f\n",
                        (unsigned long long)sample.timestamp_ms,
                        sample.temperature,
                        sample.humidity) < 0) {
                perror("[LOGGER] fprintf");
                break;
            }

            ctx->logged_count++;

            fp = rotate_log_if_needed(fp, ctx);
            if (fp == NULL) {
                fprintf(stderr, "[LOGGER] Cannot reopen log file\n");
                break;
            }
        } else if (n < 0) {
            if (errno == EAGAIN) {
                struct timespec delay = {0, 10000000}; /* 10 ms */
                nanosleep(&delay, NULL);
            } else if (errno != ETIMEDOUT && errno != EINTR) {
                perror("[LOGGER] mq_timedreceive");
                break;
            }
        }

        /* Flush buffered log data every 5 seconds */
        time_t now = time(NULL);

        if (now != (time_t)-1 &&
            now - last_flush >= SMS_LOG_FLUSH_INTERVAL) {
            if (fflush(fp) != 0)
                perror("[LOGGER] fflush");

            last_flush = now;
        }
    }

    /* Flush remaining data before closing */
    if (fflush(fp) != 0)
        perror("[LOGGER] final fflush");

    if (fclose(fp) != 0)
        perror("[LOGGER] fclose");

    return NULL;
}
