/*
 * File: circular_buffer.c
 * Purpose: Thread-safe circular buffer implementation
 * Author: Nguyen Thanh Phong - SE182514
 * Date: 2026-10-21
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "circular_buffer.h"

/*
 * Allocate the buffer once on the heap.
 * Initialize mutex and condition variable.
 */
int circular_buffer_init(CircularBuffer *cb, size_t capacity)
{
    int ret;

    if (cb == NULL || capacity == 0)
        return EINVAL;

    memset(cb, 0, sizeof(*cb));

    cb->data = calloc(capacity, sizeof(SensorData));
    if (cb->data == NULL)
        return ENOMEM;

    cb->capacity = capacity;

    ret = pthread_mutex_init(&cb->mutex, NULL);
    if (ret != 0) {
        free(cb->data);
        cb->data = NULL;
        return ret;
    }

    ret = pthread_cond_init(&cb->not_empty, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&cb->mutex);
        free(cb->data);
        cb->data = NULL;
        return ret;
    }

    return 0;
}

/* Release all allocated resources */
void circular_buffer_destroy(CircularBuffer *cb)
{
    if (cb == NULL)
        return;

    pthread_cond_destroy(&cb->not_empty);
    pthread_mutex_destroy(&cb->mutex);

    free(cb->data);
    cb->data = NULL;
}

/* Producer: add one sensor sample to the circular buffer */
int circular_buffer_push(CircularBuffer *cb, const SensorData *sample)
{
    if (cb == NULL || sample == NULL)
        return EINVAL;

    pthread_mutex_lock(&cb->mutex);

    /* Reject new samples during shutdown */
    if (cb->shutdown) {
        pthread_mutex_unlock(&cb->mutex);
        return ECANCELED;
    }

    /* Buffer is full: count overflow and discard sample */
    if (cb->size == cb->capacity) {
        cb->overflow_count++;
        pthread_mutex_unlock(&cb->mutex);
        return ENOSPC;
    }

    /* Store sample at the next write position */
    cb->data[cb->head] = *sample;

    /* Wrap around when reaching the end */
    cb->head = (cb->head + 1) % cb->capacity;

    cb->size++;
    cb->total_produced++;

    /* Wake up a consumer waiting for data */
    pthread_cond_signal(&cb->not_empty);

    pthread_mutex_unlock(&cb->mutex);

    return 0;
}

/* Consumer: wait for and retrieve a sensor sample */
int circular_buffer_pop(CircularBuffer *cb, SensorData *sample)
{
    if (cb == NULL || sample == NULL)
        return EINVAL;

    pthread_mutex_lock(&cb->mutex);

    /* Wait while the buffer is empty and app is running */
    while (cb->size == 0 && !cb->shutdown) {
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }

    /* Stop when shutdown is requested and no data remains */
    if (cb->size == 0 && cb->shutdown) {
        pthread_mutex_unlock(&cb->mutex);
        return ECANCELED;
    }

    /* Retrieve the oldest sample */
    *sample = cb->data[cb->tail];

    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->size--;
    cb->total_consumed++;

    pthread_mutex_unlock(&cb->mutex);

    return 0;
}

/* Retrieve consistent buffer statistics */
void circular_buffer_get_stats(CircularBuffer *cb, BufferStats *stats)
{
    if (cb == NULL || stats == NULL)
        return;

    pthread_mutex_lock(&cb->mutex);

    stats->current_size = cb->size;
    stats->total_produced = cb->total_produced;
    stats->total_consumed = cb->total_consumed;
    stats->overflow_count = cb->overflow_count;

    pthread_mutex_unlock(&cb->mutex);
}

/* Notify waiting threads that the application is shutting down */
void circular_buffer_shutdown(CircularBuffer *cb)
{
    if (cb == NULL)
        return;

    pthread_mutex_lock(&cb->mutex);

    cb->shutdown = 1;
    pthread_cond_broadcast(&cb->not_empty);

    pthread_mutex_unlock(&cb->mutex);
}
