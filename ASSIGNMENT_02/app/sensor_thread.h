/*
 * File: sensor_thread.h
 * Purpose: Sensor acquisition thread interface
 * Author: Nguyen Thanh Phong - SE182514
 * Date: 2026-10-21
 */

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <pthread.h>
#include <stdatomic.h>

#include "circular_buffer.h"

typedef struct {
    CircularBuffer *buffer;
    unsigned int interval_ms;
    _Atomic int *running;
    int verbose;
} SensorThreadContext;

void *sensor_thread_func(void *arg);

#endif
