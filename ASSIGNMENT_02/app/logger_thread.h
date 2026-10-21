/*
 * File: logger_thread.h
 * Purpose: Logger thread interface
 * Author: Nguyen Thanh Phong - SE182514
 */

#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <pthread.h>
#include <mqueue.h>
#include <stdatomic.h>

#include "sms_common.h"

#define SMS_LOG_QUEUE_NAME "/sms_log_queue"
#define SMS_LOG_MAX_SIZE (1024 * 1024)
#define SMS_LOG_FLUSH_INTERVAL 5

typedef struct {
    mqd_t log_queue;
    _Atomic int *running;

    unsigned long logged_count;
    unsigned long rotation_count;

    int verbose;
} LoggerThreadContext;

void *logger_thread_func(void *arg);

#endif
