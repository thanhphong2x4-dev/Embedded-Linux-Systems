/*
 * File: processor_thread.h
 * Purpose: Sensor data processing thread interface
 * Author: Nguyen Thanh Phong - SE182514
 */

#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include <mqueue.h>
#include <signal.h>

#include "circular_buffer.h"

typedef struct {
    CircularBuffer *buffer;

    mqd_t alert_queue;
    mqd_t log_queue;

    double temp_threshold;
    double hum_threshold;

    unsigned long processed_count;
    unsigned long alert_count;

    int verbose;
} ProcessorThreadContext;

void *processor_thread_func(void *arg);

#endif
