/*
 * File: circular_buffer.h
 * Purpose: Thread-safe circular buffer interface
 * Author: Nguyen Thanh Phong - SE182514
 * Date: 2026-10-21
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stddef.h>
#include "sms_common.h"

typedef struct {
    SensorData *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;

    unsigned long total_produced;
    unsigned long total_consumed;
    unsigned long overflow_count;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    int shutdown;
} CircularBuffer;

typedef struct {
    size_t current_size;
    unsigned long total_produced;
    unsigned long total_consumed;
    unsigned long overflow_count;
} BufferStats;

int circular_buffer_init(CircularBuffer *cb, size_t capacity);
int circular_buffer_push(CircularBuffer *cb, const SensorData *sample);
int circular_buffer_pop(CircularBuffer *cb, SensorData *sample);
void circular_buffer_get_stats(CircularBuffer *cb, BufferStats *stats);
void circular_buffer_shutdown(CircularBuffer *cb);
void circular_buffer_destroy(CircularBuffer *cb);

#endif
