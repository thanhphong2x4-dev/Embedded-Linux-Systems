#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "counter"
#define PROC_NAME   "counter_info"

static atomic_t counter = ATOMIC_INIT(0);
static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Student");
MODULE_DESCRIPTION("Assignment 01 - Atomic Counter Driver");
MODULE_VERSION("1.0");

/* Read current counter value, then increment it */
static ssize_t counter_read(struct file *file, char __user *buf,
                            size_t len, loff_t *offset)
{
    char temp[32];
    int value;
    int text_len;

    /* Return EOF after one successful read per open file */
    if (*offset > 0)
        return 0;

    /* Atomically get current value and increment counter */
    value = atomic_fetch_inc(&counter);

    text_len = scnprintf(temp, sizeof(temp), "%d\n", value);

    if (len < text_len)
        return -EINVAL;

    if (copy_to_user(buf, temp, text_len))
        return -EFAULT;

    *offset += text_len;
    atomic_inc(&read_count);

    pr_info("counter_driver: read value=%d\n", value);

    return text_len;
}

/* Set counter to a non-negative integer */
static ssize_t counter_write(struct file *file, const char __user *buf,
                             size_t len, loff_t *offset)
{
    char temp[32];
    int value;
    int ret;

    if (len == 0 || len >= sizeof(temp))
        return -EINVAL;

    if (copy_from_user(temp, buf, len))
        return -EFAULT;

    temp[len] = '\0';

    ret = kstrtoint(strim(temp), 10, &value);
    if (ret)
        return ret;

    if (value < 0)
        return -EINVAL;

    atomic_set(&counter, value);
    atomic_inc(&write_count);

    pr_info("counter_driver: counter set to %d\n", value);

    return len;
}

/* Display counter statistics through procfs */
static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Counter Driver Statistics\n");
    seq_printf(m, "=========================\n");
    seq_printf(m, "Current counter : %d\n", atomic_read(&counter));
    seq_printf(m, "Read count      : %d\n", atomic_read(&read_count));
    seq_printf(m, "Write count     : %d\n", atomic_read(&write_count));

    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* Connect device operations */
static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .read  = counter_read,
    .write = counter_write,
    .llseek = no_llseek,
};

/* Register a miscellaneous device: major 10 */
static struct miscdevice counter_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &counter_fops,
    .mode  = 0666,
};

static struct proc_dir_entry *counter_proc_entry;

static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_miscdev);
    if (ret) {
        pr_err("counter_driver: misc_register failed: %d\n", ret);
        return ret;
    }

    counter_proc_entry = proc_create(PROC_NAME, 0444, NULL, &counter_proc_ops);
    if (!counter_proc_entry) {
        pr_err("counter_driver: proc_create failed\n");
        misc_deregister(&counter_miscdev);
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded, major=%d minor=%d\n",
            MISC_MAJOR, counter_miscdev.minor);

    return 0;
}

static void __exit counter_exit(void)
{
    proc_remove(counter_proc_entry);
    misc_deregister(&counter_miscdev);
    pr_info("counter_driver: module unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);
