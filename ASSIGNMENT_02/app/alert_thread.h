/*
 * File: alert_thread.h
 * Purpose: Alert handling thread interface
 * Author: Nguyen Thanh Phong - SE182514
 */

#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include <mqueue.h>
#include <stdatomic.h>

#include "sms_common.h"

typedef struct {
    mqd_t alert_queue;
    _Atomic int *running;
    unsigned long received_count;
    int verbose;
} AlertThreadContext;

void *alert_thread_func(void *arg);

#endif
