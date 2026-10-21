#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    char version[64] = {0};
    struct asgn1_stats stats = {0};

    fd = open("/dev/asgn1", O_RDWR);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, ASGN1_GET_VERSION, version) < 0) {
        perror("GET_VERSION");
    } else {
        printf("Driver version: %s\n", version);
    }

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
        perror("GET_STATS");
    } else {
        printf("Open count: %d\n", stats.open_count);
        printf("Write count: %d\n", stats.write_count);
        printf("Read count: %d\n", stats.read_count);
        printf("Buffer length: %d\n", stats.buffer_len);
        printf("Last write size: %d\n", stats.last_write_size);
    }
    /* Test SET_MODE: chuyen sang Kernel mode */
    {
        int mode = 0;

        if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0)
            perror("SET_MODE 0");
        else
            printf("SET_MODE 0: OK\n");

        if (write(fd, "Hello", 5) < 0)
            perror("write");

        lseek(fd, 0, SEEK_SET);

        char buffer[128] = {0};
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

        if (n < 0)
            perror("read");
        else
            printf("Kernel mode result: %s\n", buffer);
    }

    /* Test SET_MODE: chuyen ve Echo mode */
    {
        int mode = 1;

        if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0)
            perror("SET_MODE 1");
        else
            printf("SET_MODE 1: OK");
    }

    /* Test RESET_BUFFER */
    if (ioctl(fd, ASGN1_RESET_BUFFER) < 0)
        perror("RESET_BUFFER");
    else
        printf("\nRESET_BUFFER: OK\n");

    if (ioctl(fd, ASGN1_GET_STATS, &stats) == 0) {
        printf("After reset - Write count: %d\n", stats.write_count);
        printf("After reset - Read count: %d\n", stats.read_count);
        printf("After reset - Buffer length: %d\n", stats.buffer_len);
    }


    /* TEST_INVALID_IOCTL: lenh khong hop le phai tra ENOTTY */
    {
        int ret;

        errno = 0;
        ret = ioctl(fd, _IO('Z', 99));

        if (ret == -1 && errno == ENOTTY)
            printf("Invalid ioctl: PASS (ENOTTY)\\n");
        else
            printf("Invalid ioctl: FAIL (ret=%d, errno=%d)\\n",
                   ret, errno);
    }

    close(fd);
    return 0;
}

