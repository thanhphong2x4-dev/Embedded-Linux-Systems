/*
 * File: processor_thread.c
 * Purpose: Process sensor samples and detect alerts
 * Author: Nguyen Thanh Phong - SE182514
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "processor_thread.h"
#include "sms_common.h"

/* Rolling average of the latest 10 samples */
typedef struct {
    double temperatures[SMS_ROLLING_WINDOW];
    double humidities[SMS_ROLLING_WINDOW];

    double temp_sum;
    double hum_sum;

    unsigned int count;
    unsigned int index;
} RollingAverage;

/* Add a new sample and update the rolling sums */
static void rolling_average_update(RollingAverage *avg,
                                   const SensorData *sample)
{
    unsigned int pos = avg->index;

    /* Remove the oldest sample once the window is full */
    if (avg->count == SMS_ROLLING_WINDOW) {
        avg->temp_sum -= avg->temperatures[pos];
        avg->hum_sum -= avg->humidities[pos];
    } else {
        avg->count++;
    }

    avg->temperatures[pos] = sample->temperature;
    avg->humidities[pos] = sample->humidity;

    avg->temp_sum += sample->temperature;
    avg->hum_sum += sample->humidity;

    avg->index = (pos + 1) % SMS_ROLLING_WINDOW;
}

/* Calculate the current averages */
static void rolling_average_get(const RollingAverage *avg,
                                double *temp_avg,
                                double *hum_avg)
{
    if (avg->count == 0) {
        *temp_avg = 0.0;
        *hum_avg = 0.0;
        return;
    }

    *temp_avg = avg->temp_sum / avg->count;
    *hum_avg = avg->hum_sum / avg->count;
}

/* Send one alert through the POSIX message queue */
static int send_alert(ProcessorThreadContext *ctx,
                      const SensorData *sample,
                      AlertType type,
                      double value,
                      double threshold)
{
    AlertData alert = {
        .timestamp_ms = sample->timestamp_ms,
        .type = type,
        .value = value,
        .threshold = threshold
    };

    if (mq_send(ctx->alert_queue,
                (const char *)&alert,
                sizeof(alert), 0) == -1) {
        perror("[PROCESSOR] mq_send");
        return -1;
    }

    ctx->alert_count++;
    return 0;
}

/* Process sensor samples and detect abnormal conditions */
void *processor_thread_func(void *arg)
{
    ProcessorThreadContext *ctx = arg;
    RollingAverage avg = {0};
    SensorData sample;
    double previous_temp = 0.0;
    int has_previous = 0;

    while (circular_buffer_pop(ctx->buffer, &sample) == 0) {
        double temp_avg;
        double hum_avg;

        rolling_average_update(&avg, &sample);
        rolling_average_get(&avg, &temp_avg, &hum_avg);

        ctx->processed_count++;

        /* Forward the processed sample to Logger Thread */
        if (mq_send(ctx->log_queue,
                    (const char *)&sample,
                    sizeof(sample), 0) == -1) {
            if (errno == EAGAIN)
                fprintf(stderr, "[PROCESSOR] Log queue full\n");
            else
                perror("[PROCESSOR] log mq_send");
        }

        if (ctx->verbose) {
            printf("[PROCESSOR] Temp=%.1f C, Hum=%.1f%% "
                   "| Avg(10): %.2f C, %.2f%%\n",
                   sample.temperature,
                   sample.humidity,
                   temp_avg,
                   hum_avg);
        }

        /* Alert 1: High temperature */
        if (sample.temperature > ctx->temp_threshold) {
            send_alert(ctx, &sample, TEMP_HIGH,
                       sample.temperature, ctx->temp_threshold);
        }

        /* Alert 2: High humidity */
        if (sample.humidity > ctx->hum_threshold) {
            send_alert(ctx, &sample, HUMID_HIGH,
                       sample.humidity, ctx->hum_threshold);
        }

        /* Alert 3: Temperature spike */
        if (has_previous &&
            sample.temperature - previous_temp > 5.0) {
            send_alert(ctx, &sample, TEMP_SPIKE,
                       sample.temperature - previous_temp, 5.0);
        }

        previous_temp = sample.temperature;
        has_previous = 1;
    }

    return NULL;
}
