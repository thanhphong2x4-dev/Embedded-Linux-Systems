#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

/* IOCTL command family */
#define SMS_IOC_MAGIC 'S'

/* Configure minimum interval between sensor reads (milliseconds) */
#define SMS_SET_INTERVAL _IOW(SMS_IOC_MAGIC, 1, unsigned int)

#define SMS_DEFAULT_INTERVAL_MS 100

#endif
