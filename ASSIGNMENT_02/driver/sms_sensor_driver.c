/*
 * File: sms_sensor_driver.c
 * Purpose: Simulated temperature and humidity sensor driver
 * Author: Nguyen Thanh Phong - SE182514
 * Date: 2026-10-21
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "sms_sensor.h"

#define DEVICE_NAME "sms_sensor"
#define PROC_NAME   "sms_stats"

static DEFINE_MUTEX(sensor_mutex);

static unsigned int interval_ms = SMS_DEFAULT_INTERVAL_MS;
static unsigned long read_count;
static unsigned int last_temperature_x10;
static unsigned int last_humidity_x10;
static u64 start_time_ms;
static u64 last_read_ms;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Thanh Phong - SE182514");
MODULE_DESCRIPTION("Assignment 02 - Simulated Sensor Driver");
MODULE_VERSION("1.0");

/* Generate one sensor sample and return it as CSV */
static ssize_t sms_sensor_read(struct file *file, char __user *buf,
                               size_t len, loff_t *offset)
{
    char sample[80];
    unsigned int temp_x10, hum_x10;
    int sample_len;
    u64 timestamp;

    if (*offset > 0)
        return 0;

    mutex_lock(&sensor_mutex);

    timestamp = div_u64(ktime_get_ns(), 1000000);

    if (read_count > 0 && timestamp - last_read_ms < interval_ms) {
        mutex_unlock(&sensor_mutex);
        return -EAGAIN;
    }

    temp_x10 = 150 + get_random_u32() % 301;
    hum_x10  = 300 + get_random_u32() % 601;

    sample_len = scnprintf(sample, sizeof(sample),
        "%llu,%u.%u,%u.%u\n",
        (unsigned long long)timestamp,
        temp_x10 / 10, temp_x10 % 10,
        hum_x10 / 10, hum_x10 % 10);

    if (len < sample_len) {
        mutex_unlock(&sensor_mutex);
        return -EINVAL;
    }

    if (copy_to_user(buf, sample, sample_len)) {
        mutex_unlock(&sensor_mutex);
        return -EFAULT;
    }

    read_count++;
    last_read_ms = timestamp;
    last_temperature_x10 = temp_x10;
    last_humidity_x10 = hum_x10;

    mutex_unlock(&sensor_mutex);

    *offset += sample_len;
    return sample_len;
}

/* Configure the minimum interval between sensor reads */
static long sms_sensor_ioctl(struct file *file,
                             unsigned int cmd, unsigned long arg)
{
    unsigned int new_interval;

    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;

    if (copy_from_user(&new_interval, (void __user *)arg,
                       sizeof(new_interval)))
        return -EFAULT;

    if (new_interval == 0)
        return -EINVAL;

    mutex_lock(&sensor_mutex);
    interval_ms = new_interval;
    mutex_unlock(&sensor_mutex);

    pr_info("sms_sensor: interval set to %u ms\n", new_interval);

    return 0;
}

/* Display sensor statistics through /proc/sms_stats */
static int sms_proc_show(struct seq_file *m, void *v)
{
    u64 uptime_seconds;

    mutex_lock(&sensor_mutex);

    uptime_seconds = div_u64(div_u64(ktime_get_ns(), 1000000) - start_time_ms, 1000);

    seq_printf(m, "SMS Sensor Driver Statistics\n");
    seq_printf(m, "============================\n");
    seq_printf(m, "Read count       : %lu\n", read_count);
    seq_printf(m, "Last temperature : %u.%u C\n",
               last_temperature_x10 / 10,
               last_temperature_x10 % 10);
    seq_printf(m, "Last humidity    : %u.%u %%\n",
               last_humidity_x10 / 10,
               last_humidity_x10 % 10);
    seq_printf(m, "Driver uptime    : %llu seconds\n",
               (unsigned long long)uptime_seconds);

    mutex_unlock(&sensor_mutex);

    return 0;
}

static int sms_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* Connect device operations */
static const struct file_operations sms_fops = {
    .owner          = THIS_MODULE,
    .read           = sms_sensor_read,
    .unlocked_ioctl = sms_sensor_ioctl,
    .llseek         = no_llseek,
};

/* Register the sensor as a miscellaneous device */
static struct miscdevice sms_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &sms_fops,
    .mode  = 0666,
};

static struct proc_dir_entry *sms_proc_entry;

static int __init sms_sensor_init(void)
{
    int ret;

    start_time_ms = div_u64(ktime_get_ns(), 1000000);

    ret = misc_register(&sms_miscdev);
    if (ret) {
        pr_err("sms_sensor: misc_register failed: %d\n", ret);
        return ret;
    }

    sms_proc_entry = proc_create(PROC_NAME, 0444, NULL, &sms_proc_ops);
    if (!sms_proc_entry) {
        pr_err("sms_sensor: proc_create failed\n");
        misc_deregister(&sms_miscdev);
        return -ENOMEM;
    }

    pr_info("sms_sensor: module loaded, major=%d minor=%d\n",
            MISC_MAJOR, sms_miscdev.minor);

    return 0;
}

static void __exit sms_sensor_exit(void)
{
    proc_remove(sms_proc_entry);
    misc_deregister(&sms_miscdev);

    pr_info("sms_sensor: module unloaded\n");
}

module_init(sms_sensor_init);
module_exit(sms_sensor_exit);
