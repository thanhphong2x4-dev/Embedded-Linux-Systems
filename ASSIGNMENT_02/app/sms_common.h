#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <stdint.h>

#define SENSOR_DEVICE "/dev/sms_sensor"
#define SMS_LOG_PATH "/tmp/sms.log"

#define SMS_BUFFER_CAPACITY 64
#define SMS_ROLLING_WINDOW 10

#define SMS_APP_DEFAULT_INTERVAL_MS 500
#define SMS_DEFAULT_TEMP_THRESHOLD 40.0
#define SMS_DEFAULT_HUM_THRESHOLD 80.0

typedef struct {
    uint64_t timestamp_ms;
    double temperature;
    double humidity;
} SensorData;

typedef enum {
    TEMP_HIGH,
    HUMID_HIGH,
    TEMP_SPIKE
} AlertType;

typedef struct {
    uint64_t timestamp_ms;
    AlertType type;
    double value;
    double threshold;
} AlertData;

#endif
