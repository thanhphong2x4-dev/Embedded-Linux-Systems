#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>

#define SAMPLE_COUNT 1000
#define PERIOD_NS 10000000L

static double jitter_us[SAMPLE_COUNT];

static int compare_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static void add_ns(struct timespec *ts, long ns)
{
    ts->tv_nsec += ns;

    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

static double difference_us(const struct timespec *actual,
                            const struct timespec *expected)
{
    return (actual->tv_sec - expected->tv_sec) * 1000000.0
         + (actual->tv_nsec - expected->tv_nsec) / 1000.0;
}

int main(int argc, char *argv[])
{
    struct timespec next, actual;
    struct sched_param param = {0};
    int policy;
    int histogram[4] = {0};
    double min, max, mean = 0.0;

    if (argc != 2 ||
        (strcmp(argv[1], "other") != 0 &&
         strcmp(argv[1], "fifo") != 0)) {
        fprintf(stderr, "Usage: %s other|fifo\n", argv[0]);
        return EXIT_FAILURE;
    }

    policy = strcmp(argv[1], "fifo") == 0
           ? SCHED_FIFO : SCHED_OTHER;

    param.sched_priority = policy == SCHED_FIFO ? 50 : 0;

    if (sched_setscheduler(0, policy, &param) == -1) {
        perror("sched_setscheduler");
        return EXIT_FAILURE;
    }

    if (sched_getscheduler(0) != policy) {
        fprintf(stderr, "Scheduling policy verification failed\n");
        return EXIT_FAILURE;
    }

    printf("Policy: %s\n", argv[1]);
    printf("Priority: %d\n", param.sched_priority);
    printf("Samples: %d\n", SAMPLE_COUNT);
    printf("Period: 10 ms\n");

    if (clock_gettime(CLOCK_MONOTONIC, &next) == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        add_ns(&next, PERIOD_NS);

        int ret;
        do {
            ret = clock_nanosleep(CLOCK_MONOTONIC,
                                  TIMER_ABSTIME,
                                  &next, NULL);
        } while (ret == EINTR);

        if (ret != 0) {
            fprintf(stderr, "clock_nanosleep: %s\n",
                    strerror(ret));
            return EXIT_FAILURE;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &actual) == -1) {
            perror("clock_gettime");
            return EXIT_FAILURE;
        }

        double jitter = difference_us(&actual, &next);

        /* Early wakeups are represented by zero lateness. */
        if (jitter < 0.0)
            jitter = 0.0;

        jitter_us[i] = jitter;
        mean += jitter;

        if (jitter < 10.0)
            histogram[0]++;
        else if (jitter < 50.0)
            histogram[1]++;
        else if (jitter < 100.0)
            histogram[2]++;
        else
            histogram[3]++;
    }

    mean /= SAMPLE_COUNT;

    qsort(jitter_us, SAMPLE_COUNT,
          sizeof(jitter_us[0]), compare_double);

    min = jitter_us[0];
    max = jitter_us[SAMPLE_COUNT - 1];

    printf("\n=== Jitter Statistics ===\n");
    printf("Min:  %.3f us\n", min);
    printf("Mean: %.3f us\n", mean);
    printf("Max:  %.3f us\n", max);
    printf("P99:  %.3f us\n", jitter_us[989]);

    printf("\n=== Histogram ===\n");
    printf("< 10 us:      %d\n", histogram[0]);
    printf("10-50 us:     %d\n", histogram[1]);
    printf("50-100 us:    %d\n", histogram[2]);
    printf(">= 100 us:    %d\n", histogram[3]);

    return EXIT_SUCCESS;
}
