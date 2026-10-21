/*
 * Test SMS_SET_INTERVAL ioctl
 * Nguyen Thanh Phong - SE182514
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "sms_sensor.h"

int main(void)
{
    int fd;
    unsigned int interval = 1000;
    char buffer[128];

    fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        perror("SMS_SET_INTERVAL");
        close(fd);
        return 1;
    }

    printf("SMS_SET_INTERVAL: PASS (1000 ms)\n");

    if (read(fd, buffer, sizeof(buffer) - 1) < 0) {
        perror("First read");
        close(fd);
        return 1;
    }

    printf("First read: PASS\n");

    close(fd);

    fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("Second open");
        return 1;
    }

    if (read(fd, buffer, sizeof(buffer) - 1) < 0) {
        if (errno == EAGAIN)
            printf("Immediate second read: PASS (EAGAIN)\n");
        else {
            perror("Second read");
            close(fd);
            return 1;
        }
    } else {
        printf("Immediate second read: FAIL (expected EAGAIN)\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
