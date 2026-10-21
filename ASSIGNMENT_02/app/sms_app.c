/*
 * File: sms_app.c
 * Project: Sensor Monitoring System
 * Assignment 02
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdatomic.h>
#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>

#include "sms_common.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

#define SMS_ALERT_QUEUE_NAME "/sms_alert_queue"

typedef struct {
    unsigned int interval_ms;
    double temp_threshold;
    double hum_threshold;
    unsigned int duration_sec;
    int verbose;
} AppConfig;

static void app_config_defaults(AppConfig *config)
{
    config->interval_ms = SMS_APP_DEFAULT_INTERVAL_MS;
    config->temp_threshold = SMS_DEFAULT_TEMP_THRESHOLD;
    config->hum_threshold = SMS_DEFAULT_HUM_THRESHOLD;
    config->duration_sec = 60;
    config->verbose = 0;
}

static void print_usage(const char *program)
{
    printf("Usage: %s [-i ms] [-t C] [-h %%] [-d sec] [-v]\n",
           program);
    printf("  -i  Sensor interval in milliseconds (default: 500)\n");
    printf("  -t  Temperature threshold (default: 40 C)\n");
    printf("  -h  Humidity threshold (default: 80%%)\n");
    printf("  -d  Duration in seconds (default: 60, 0 = unlimited)\n");
    printf("  -v  Verbose output\n");
}

/* Read application options from the command line */
static int parse_arguments(int argc, char *argv[], AppConfig *config)
{
    int opt;

    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        char *endptr;

        switch (opt) {
        case 'i': {
            unsigned long value = strtoul(optarg, &endptr, 10);

            if (*optarg == '-' || *optarg == '\0' ||
                *endptr != '\0' || value == 0 || value > 60000) {
                fprintf(stderr, "Invalid sampling interval\n");
                return -1;
            }

            config->interval_ms = (unsigned int)value;
            break;
        }

        case 't': {
            double value = strtod(optarg, &endptr);

            if (*optarg == '\0' || *endptr != '\0' ||
                !(value >= 15.0 && value <= 45.0)) {
                fprintf(stderr, "Invalid temperature threshold\n");
                return -1;
            }

            config->temp_threshold = value;
            break;
        }

        case 'h': {
            double value = strtod(optarg, &endptr);

            if (*optarg == '\0' || *endptr != '\0' ||
                !(value >= 30.0 && value <= 90.0)) {
                fprintf(stderr, "Invalid humidity threshold\n");
                return -1;
            }

            config->hum_threshold = value;
            break;
        }

        case 'd': {
            unsigned long value = strtoul(optarg, &endptr, 10);

            if (*optarg == '-' || *optarg == '\0' ||
                *endptr != '\0' || value > 86400) {
                fprintf(stderr, "Invalid duration\n");
                return -1;
            }

            config->duration_sec = (unsigned int)value;
            break;
        }

        case 'v':
            config->verbose = 1;
            break;

        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected positional argument\n");
        return -1;
    }

    return 0;
}

/* Create a POSIX message queue with fixed-size messages */
static mqd_t create_message_queue(const char *name, long message_size)
{
    struct mq_attr attr = {0};
    mqd_t queue;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = message_size;

    /* Remove an old queue from a previous run */
    if (mq_unlink(name) == -1 && errno != ENOENT) {
        perror("[APP] mq_unlink");
        return (mqd_t)-1;
    }

    queue = mq_open(name,
                    O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK,
                    0600,
                    &attr);

    if (queue == (mqd_t)-1)
        perror("[APP] mq_open");

    return queue;
}

/* Application running state */
static _Atomic int app_running = 1;
static _Atomic int workers_running = 1;

/* Signal handler: request a graceful shutdown */
static void handle_signal(int signo)
{
    (void)signo;
    app_running = 0;
}

/* Register SIGINT and SIGTERM handlers */
static int setup_signal_handlers(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[APP] sigaction SIGINT");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("[APP] sigaction SIGTERM");
        return -1;
    }

    return 0;
}

/* Configure Sensor Thread real-time priority */
static void configure_realtime_thread(pthread_t thread)
{
    struct sched_param param = {0};
    param.sched_priority = 50;

    int ret = pthread_setschedparam(thread, SCHED_FIFO, &param);

    if (ret != 0) {
        fprintf(stderr,
                "[RT] SCHED_FIFO priority 50 unavailable: %s\n",
                strerror(ret));
    } else {
        printf("[RT] Sensor Thread: SCHED_FIFO priority 50\n");
    }
}

int main(int argc, char *argv[])
{
    AppConfig config;
    CircularBuffer buffer;

    mqd_t alert_queue = (mqd_t)-1;
    mqd_t log_queue = (mqd_t)-1;

    app_config_defaults(&config);

    if (parse_arguments(argc, argv, &config) != 0)
        return EXIT_FAILURE;

    /* Lock memory to reduce page-fault latency */
#ifndef SMS_HOST_MOCK
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        fprintf(stderr, "[RT] mlockall failed: %s\n",
                strerror(errno));
    } else {
        printf("[RT] mlockall: SUCCESS\n");
    }
#else
    printf("[MOCK] mlockall disabled for Host memory testing\n");
#endif

    if (setup_signal_handlers() != 0)
        return EXIT_FAILURE;

    printf("=== Sensor Monitoring System ===\n");
    printf("Interval: %u ms\n", config.interval_ms);
    printf("Temperature threshold: %.1f C\n", config.temp_threshold);
    printf("Humidity threshold: %.1f%%\n", config.hum_threshold);
    printf("Duration: %u seconds\n", config.duration_sec);

    /* Initialize the shared circular buffer */
    int ret = circular_buffer_init(&buffer, SMS_BUFFER_CAPACITY);

    if (ret != 0) {
        fprintf(stderr, "[APP] Buffer initialization failed: %s\n",
                strerror(ret));
        return EXIT_FAILURE;
    }

    /* Create the alert message queue */
    alert_queue = create_message_queue(
        SMS_ALERT_QUEUE_NAME,
        sizeof(AlertData)
    );

    if (alert_queue == (mqd_t)-1) {
        circular_buffer_destroy(&buffer);
        return EXIT_FAILURE;
    }

    /* Create the logger message queue */
    log_queue = create_message_queue(
        SMS_LOG_QUEUE_NAME,
        sizeof(SensorData)
    );

    if (log_queue == (mqd_t)-1) {
        mq_close(alert_queue);
        mq_unlink(SMS_ALERT_QUEUE_NAME);
        circular_buffer_destroy(&buffer);
        return EXIT_FAILURE;
    }

    printf("[APP] Circular buffer initialized\n");
    printf("[APP] Alert queue initialized\n");
    printf("[APP] Log queue initialized\n");


    /* Configure Sensor Thread */
    SensorThreadContext sensor_ctx = {
        .buffer = &buffer,
        .interval_ms = config.interval_ms,
        .running = &app_running,
        .verbose = config.verbose
    };

    /* Configure Processor Thread */
    ProcessorThreadContext processor_ctx = {
        .buffer = &buffer,
        .alert_queue = alert_queue,
        .log_queue = log_queue,
        .temp_threshold = config.temp_threshold,
        .hum_threshold = config.hum_threshold,
        .processed_count = 0,
        .alert_count = 0,
        .verbose = config.verbose
    };

    /* Configure Alert Thread */
    AlertThreadContext alert_ctx = {
        .alert_queue = alert_queue,
        .running = &workers_running,
        .received_count = 0,
        .verbose = config.verbose
    };

    /* Configure Logger Thread */
    LoggerThreadContext logger_ctx = {
        .log_queue = log_queue,
        .running = &workers_running,
        .logged_count = 0,
        .rotation_count = 0,
        .verbose = config.verbose
    };

    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;


    /* Start consumer threads before producing sensor data */
    int alert_started = 0;
    int logger_started = 0;
    int processor_started = 0;
    int sensor_started = 0;

    ret = pthread_create(&alert_tid, NULL,
                         alert_thread_func, &alert_ctx);
    if (ret != 0) {
        fprintf(stderr, "[APP] Alert thread: %s\n", strerror(ret));
        goto shutdown;
    }
    alert_started = 1;

    ret = pthread_create(&logger_tid, NULL,
                         logger_thread_func, &logger_ctx);
    if (ret != 0) {
        fprintf(stderr, "[APP] Logger thread: %s\n", strerror(ret));
        goto shutdown;
    }
    logger_started = 1;

    ret = pthread_create(&processor_tid, NULL,
                         processor_thread_func, &processor_ctx);
    if (ret != 0) {
        fprintf(stderr, "[APP] Processor thread: %s\n", strerror(ret));
        goto shutdown;
    }
    processor_started = 1;

    ret = pthread_create(&sensor_tid, NULL,
                         sensor_thread_func, &sensor_ctx);
    if (ret != 0) {
        fprintf(stderr, "[APP] Sensor thread: %s\n", strerror(ret));
        goto shutdown;
    }
    sensor_started = 1;

    configure_realtime_thread(sensor_tid);

    printf("[APP] All 4 threads started\n");
    fflush(stdout);


    /* Run until duration expires or Ctrl+C is pressed */
    {
        struct timespec start, now;

        clock_gettime(CLOCK_MONOTONIC, &start);

        while (app_running) {
            struct timespec pause_time = {
                .tv_sec = 0,
                .tv_nsec = 100000000L
            };

            nanosleep(&pause_time, NULL);

            if (config.duration_sec == 0)
                continue;

            clock_gettime(CLOCK_MONOTONIC, &now);

            if (now.tv_sec - start.tv_sec >=
                (time_t)config.duration_sec) {
                printf("[APP] Duration reached\n");
                break;
            }
        }
    }

shutdown:
    printf("[APP] Shutting down...\n");

    /* Step 1: Stop sensor acquisition */
    app_running = 0;

    if (sensor_started)
        pthread_join(sensor_tid, NULL);

    /* Step 2: Drain circular buffer */
    circular_buffer_shutdown(&buffer);

    if (processor_started)
        pthread_join(processor_tid, NULL);

    /* Step 3: Drain alert and log queues */
    workers_running = 0;

    if (alert_started)
        pthread_join(alert_tid, NULL);

    if (logger_started)
        pthread_join(logger_tid, NULL);

    /* Print final statistics */
    BufferStats stats;
    circular_buffer_get_stats(&buffer, &stats);

    printf("\n=== SMS Final Statistics ===\n");
    printf("Samples produced: %lu\n", stats.total_produced);
    printf("Samples consumed: %lu\n", stats.total_consumed);
    printf("Buffer overflows: %lu\n", stats.overflow_count);
    printf("Samples processed: %lu\n", processor_ctx.processed_count);
    printf("Alerts generated: %lu\n", processor_ctx.alert_count);
    printf("Alerts received: %lu\n", alert_ctx.received_count);
    printf("Samples logged: %lu\n", logger_ctx.logged_count);
    printf("Log rotations: %lu\n", logger_ctx.rotation_count);

    /* Release POSIX message queues */
    mq_close(alert_queue);
    mq_close(log_queue);

    mq_unlink(SMS_ALERT_QUEUE_NAME);
    mq_unlink(SMS_LOG_QUEUE_NAME);

    circular_buffer_destroy(&buffer);

    printf("[APP] Shutdown complete\n");

    return EXIT_SUCCESS;
}
