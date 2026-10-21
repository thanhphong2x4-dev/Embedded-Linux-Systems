#ifndef ASGN1_IOCTL_H
#define ASGN1_IOCTL_H

#include <linux/ioctl.h>

/* Magic number dùng để nhận diện nhóm lệnh ioctl */
#define ASGN1_IOC_MAGIC 'A'

/* Thống kê hoạt động của driver */
struct asgn1_stats {
    int open_count;
    int write_count;
    int read_count;
    int buffer_len;
    int last_write_size;
};

/* Xóa buffer và reset bộ đếm đọc/ghi */
#define ASGN1_RESET_BUFFER \
    _IO(ASGN1_IOC_MAGIC, 0)

/* Lấy thống kê từ kernel */
#define ASGN1_GET_STATS \
    _IOR(ASGN1_IOC_MAGIC, 1, struct asgn1_stats)

/* Thay đổi chế độ hoạt động */
#define ASGN1_SET_MODE \
    _IOW(ASGN1_IOC_MAGIC, 2, int)

/* Lấy chuỗi phiên bản driver, tối đa 64 byte */
#define ASGN1_GET_VERSION \
    _IOR(ASGN1_IOC_MAGIC, 3, char[64])

#endif

