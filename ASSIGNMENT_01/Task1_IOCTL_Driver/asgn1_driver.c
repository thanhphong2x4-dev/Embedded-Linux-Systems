/*
 * LAB-02: Character Device Driver
 *
 * Chuc nang:
 *  - Tao character device /dev/lab2
 *  - Ho tro open, read, write, release
 *  - Xuat thong tin thong ke qua /proc/lab2_info
 *  - Xuat cac attribute qua sysfs
 *
 * Target:
 *  - ARMv7-A Cortex-A9
 *  - Linux Kernel 5.15
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("ASSIGNMENT-01: Character Driver with ioctl Interface");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 242

/*
 * major_number:
 * Major number dung de kernel anh xa /dev/lab2 den driver nay.
 */
static int major_number = DRIVER_MAJOR;

/*
 * device_buffer:
 * Bo dem noi bo nam trong kernel space.
 * Du lieu user ghi vao /dev/lab2 se duoc luu tai day.
 */
static char device_buffer[BUFFER_SIZE];

static int buffer_len = 0;
static int open_count = 0;

static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;

/* Mode 1: Echo mode (mac dinh) */
static int current_mode = 1;

static struct class *lab2_class = NULL;
static struct device *lab2_device = NULL;

/*
 * Mutex bao ve cac bien dung chung nhu:
 *  - device_buffer
 *  - buffer_len
 *  - open_count
 *
 * Mutex can thiet vi nhieu process co the truy cap /dev/lab2 cung luc.
 * Neu khong khoa, du lieu co the bi race condition.
 */
static DEFINE_MUTEX(lab2_mutex);


/* =========================================================
 * PROCFS IMPLEMENTATION
 * =========================================================
 */

/*
 * proc_entry tro den entry /proc/lab2_info.
 */
static struct proc_dir_entry *proc_entry;

/*
 * lab2_proc_show()
 *
 * Duoc seq_file API goi khi userspace thuc hien:
 *
 *      cat /proc/lab2_info
 *
 * seq_file duoc su dung thay vi tu quan ly buffer read cua /proc.
 * Kernel se tu xu ly viec chia du lieu thanh cac lan read phu hop.
 */
static int lab2_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "ASSIGNMENT-01 Driver Statistics\n");
    seq_printf(m, "======================\n");
    seq_printf(m, "Driver name  : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number : %d\n", major_number);
    seq_printf(m, "Buffer size  : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length  : %d bytes\n", buffer_len);
    seq_printf(m, "Open count   : %d\n", open_count);

    if (buffer_len > 0)
        seq_printf(m, "Last data    : [%s]\n", device_buffer);

    return 0;
}

/*
 * lab2_proc_open()
 *
 * single_open() la helper cua seq_file API.
 * Ham nay ket noi /proc/lab2_info voi lab2_proc_show().
 *
 * Moi lan file /proc duoc open, single_open() tao seq_file context
 * va goi lab2_proc_show() de sinh noi dung.
 */
static int lab2_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, lab2_proc_show, NULL);
}

/*
 * proc_ops mo ta cac thao tac cua /proc/lab2_info.
 */
static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};


/* =========================================================
 * SYSFS IMPLEMENTATION
 * =========================================================
 */

/*
 * Sysfs duoc kernel su dung de export cac thuoc tinh cua device.
 *
 * lab2_device co mot kobject ben trong:
 *
 *      lab2_device->kobj
 *
 * Cac attribute ben duoi se duoc gan vao kobject nay va hien thi
 * thanh cac file trong:
 *
 *      /sys/class/lab2_class/lab2/
 */

/*
 * buffer_len_show()
 *
 * Tra ve so byte hien dang duoc luu trong device_buffer.
 */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    return sprintf(buf, "%d\n", buffer_len);
}

/*
 * open_count_show()
 *
 * Tra ve tong so lan /dev/lab2 da duoc open.
 */
static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    return sprintf(buf, "%d\n", open_count);
}

/*
 * last_data_show()
 *
 * Tra ve du lieu cuoi cung ma userspace da ghi vao driver.
 */
static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    if (buffer_len > 0)
        return sprintf(buf, "%s\n", device_buffer);

    return sprintf(buf, "(empty)\n");
}

/*
 * DEVICE_ATTR_RO tao cac sysfs attribute READ-ONLY.
 *
 * Vi du:
 *
 *      DEVICE_ATTR_RO(buffer_len)
 *
 * Kernel se lien ket attribute buffer_len voi ham:
 *
 *      buffer_len_show()
 *
 * Do la ly do cac file nay chi doc duoc, khong co ham store().
 */
static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);

/*
 * Danh sach cac attribute se duoc gan vao kobject cua lab2_device.
 */
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

/*
 * attribute_group cho phep dang ky nhieu sysfs attribute
 * chi bang mot lan goi sysfs_create_group().
 */
static struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};


/* =========================================================
 * CHARACTER DEVICE FILE OPERATIONS
 * =========================================================
 */

/*
 * lab2_open()
 *
 * Duoc kernel goi khi userspace thuc hien:
 *
 *      open("/dev/lab2", ...)
 *
 * Moi lan mo device, open_count tang len 1.
 *
 * Mutex duoc dung de tranh race condition khi nhieu process
 * cung mo device tai cung mot thoi diem.
 */
static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);

    open_count++;

    mutex_unlock(&lab2_mutex);

    pr_info("asgn1_driver: device opened (count=%d)\n",
            open_count);

    return 0;
}

/*
 * lab2_release()
 *
 * Duoc goi khi userspace close file descriptor cua /dev/lab2.
 *
 * Driver nay khong cap phat resource rieng cho tung file descriptor
 * nen release chi ghi log.
 */
static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1_driver: device closed\n");

    return 0;
}

/*
 * lab2_read()
 *
 * Sao chep du lieu tu kernel space sang user space.
 *
 * KHONG duoc dung memcpy() truc tiep voi con tro userspace.
 * Ly do:
 *  - Dia chi userspace co the khong hop le.
 *  - Memory cua user va kernel nam o hai address space khac nhau.
 *  - Truy cap truc tiep co the gay kernel fault.
 *
 * copy_to_user() la kernel API an toan de truyen du lieu:
 *
 *      kernel space -> user space
 */
static ssize_t lab2_read(struct file *file,
                         char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    ssize_t bytes_read;

    mutex_lock(&lab2_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&lab2_mutex);
        pr_err("asgn1_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;

    if (bytes_read > 0)
        read_count++;

    mutex_unlock(&lab2_mutex);

    pr_info("asgn1_driver: sent %zd bytes to user\n",
            bytes_read);

    return bytes_read;
}

/*
 * lab2_write()
 *
 * Nhan du lieu tu userspace va luu vao device_buffer.
 */

static ssize_t lab2_write(struct file *file,
                          const char __user *buf,
                          size_t len,
                          loff_t *offset)
{
    const char prefix[] = "[KERNEL]";
    size_t prefix_len = sizeof(prefix) - 1;
    size_t copy_len;
    size_t pos = 0;

    mutex_lock(&lab2_mutex);

    if (current_mode == 0) {
        memcpy(device_buffer, prefix, prefix_len);
        pos = prefix_len;
    }

    copy_len = min_t(size_t, len, BUFFER_SIZE - 1 - pos);

    if (copy_from_user(device_buffer + pos, buf, copy_len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    buffer_len = pos + copy_len;
    device_buffer[buffer_len] = '\0';

    write_count++;
    last_write_size = copy_len;

    mutex_unlock(&lab2_mutex);

    pr_info("asgn1_driver: write completed, %zu bytes\n",
            copy_len);

    return copy_len;
}

/*
 * file_operations la bang anh xa system call cua VFS
 * den cac ham driver.
 *
 * open()  -> lab2_open()
 * read()  -> lab2_read()
 * write() -> lab2_write()
 * close() -> lab2_release()
 */

/*
 * asgn1_ioctl()
 *
 * Xu ly cac lenh dieu khien tu userspace.
 * cmd: ma lenh ioctl.
 * arg: tham so truyen tu userspace.
 *
 * Return: 0 khi thanh cong, ma loi am khi that bai.
 */
static long asgn1_ioctl(struct file *file,
                       unsigned int cmd,
                       unsigned long arg)
{
    switch (cmd) {
    case ASGN1_GET_STATS:
    {
        struct asgn1_stats stats;

        mutex_lock(&lab2_mutex);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&lab2_mutex);

        if (copy_to_user((void __user *)arg,
                         &stats, sizeof(stats)))
            return -EFAULT;

        pr_info("asgn1_driver: GET_STATS called\n");
        return 0;
    }

    case ASGN1_SET_MODE:
    {
        int mode;

        /* Lay gia tri mode tu userspace */
        if (copy_from_user(&mode, (void __user *)arg,
                           sizeof(mode)))
            return -EFAULT;

        /* Chi chap nhan Mode 0 hoac Mode 1 */
        if (mode != 0 && mode != 1)
            return -EINVAL;

        mutex_lock(&lab2_mutex);
        current_mode = mode;
        mutex_unlock(&lab2_mutex);

        pr_info("asgn1_driver: SET_MODE called, mode=%d\n", mode);
        return 0;
    }

    case ASGN1_GET_VERSION:
    {
        const char version[64] = "asgn1_driver v1.0 - ARMv7";

        if (copy_to_user((void __user *)arg,
                         version, sizeof(version)))
            return -EFAULT;

        pr_info("asgn1_driver: GET_VERSION called\n");
        return 0;
    }

    case ASGN1_RESET_BUFFER:
        mutex_lock(&lab2_mutex);

        device_buffer[0] = '\0';
        buffer_len = 0;
        read_count = 0;
        write_count = 0;
        last_write_size = 0;

        mutex_unlock(&lab2_mutex);

        pr_info("asgn1_driver: RESET_BUFFER called\n");
        return 0;

    default:
        return -ENOTTY;
    }
}

static struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .release = lab2_release,
    .read    = lab2_read,
    .write   = lab2_write,
    .unlocked_ioctl = asgn1_ioctl,
};


/* =========================================================
 * MODULE INITIALIZATION
 * =========================================================
 */

/*
 * lab2_init()
 *
 * Duoc kernel goi khi module duoc nap bang:
 *
 *      insmod lab2_driver.ko
 *
 * Qua trinh:
 *
 *  1. Dang ky character device major 240.
 *  2. Tao device class.
 *  3. Tao device trong sysfs.
 *  4. Tao cac sysfs attributes.
 *  5. Tao /proc/lab2_info.
 */
static int __init lab2_init(void)
{
    int ret;

    pr_info("asgn1_driver: initializing module\n");

    /*
     * Dang ky character device voi kernel.
     */
    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &lab2_fops);

    if (ret < 0)
    {
        pr_err("asgn1_driver: register_chrdev failed: %d\n",
               ret);

        return ret;
    }

    /*
     * Tao class:
     *
     *      /sys/class/lab2_class/
     */
    lab2_class = class_create(THIS_MODULE,
                              CLASS_NAME);

    if (IS_ERR(lab2_class))
    {
        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return PTR_ERR(lab2_class);
    }

    /*
     * Tao device:
     *
     *      /sys/class/lab2_class/lab2/
     *
     * MKDEV(240, 0) tao device number:
     *
     *      major = 240
     *      minor = 0
     */
    lab2_device = device_create(lab2_class,
                                NULL,
                                MKDEV(major_number, 0),
                                NULL,
                                DEVICE_NAME);

    if (IS_ERR(lab2_device))
    {
        class_destroy(lab2_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return PTR_ERR(lab2_device);
    }

    /*
     * Gan nhom sysfs attribute vao kobject cua lab2_device.
     */
    ret = sysfs_create_group(&lab2_device->kobj,
                             &lab2_attr_group);

    if (ret)
        pr_warn("asgn1_driver: sysfs_create_group failed\n");

    /*
     * Tao:
     *
     *      /proc/lab2_info
     *
     * Mode 0444: chi doc.
     */
    proc_entry = proc_create("asgn1_info",
                             0444,
                             NULL,
                             &lab2_proc_fops);

    if (!proc_entry)
    {
        pr_warn("asgn1_driver: failed to create /proc/lab2_info\n");
    }
    else
    {
        pr_info("asgn1_driver: /proc/asgn1_info created\n");
    }

    pr_info("asgn1_driver: loaded, major=%d\n",
            major_number);

    return 0;
}


/* =========================================================
 * MODULE CLEANUP
 * =========================================================
 */

/*
 * lab2_exit()
 *
 * Duoc kernel goi khi:
 *
 *      rmmod lab2_driver
 *
 * Tat ca resource da tao trong lab2_init() phai duoc
 * huy theo thu tu phu hop:
 *
 *  1. Xoa procfs entry.
 *  2. Xoa sysfs attributes.
 *  3. Xoa device.
 *  4. Xoa class.
 *  5. Huy dang ky character device.
 */
static void __exit lab2_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    sysfs_remove_group(&lab2_device->kobj,
                       &lab2_attr_group);

    device_destroy(lab2_class,
                   MKDEV(major_number, 0));

    class_destroy(lab2_class);

    unregister_chrdev(major_number,
                      DEVICE_NAME);

    pr_info("asgn1_driver: module unloaded\n");
}


/*
 * Khai bao entry point cua kernel module.
 */
module_init(lab2_init);
module_exit(lab2_exit);

