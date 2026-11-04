#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include "sd.h"

int main() {
    const char *dev_path = "/dev/secret";

    // Create device id
    dev_t dev_num = makedev(42, 0);

    // Check if file exists (if not create it)
    if (access(dev_path, F_OK) == -1) {
        printf("Devicefile %s not found. Creating... \n", dev_path);

        // Create characted device for read and write privileges
        if (mknod(dev_path, S_IFCHR | 0666, dev_num) == -1) {
            perror("mknod failed. Did you drive program with sudo?");
            return 1;
        }
        printf("Devicefile created successfully.\n");
    }

    // Open devicefile
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("Driver open failed");
        return 1;
    }

    // Added test logic
    char write_buf[] = "Hello from kernel";
    char read_buf[1024] = {0};
    my_ioctl_data ioctl_args;
    ioctl_args.key = 42; // Encryption key

    printf("Write message: %s\n", write_buf);
    write(fd, write_buf, strlen(write_buf));

    printf("Encrypt message with key %d...\n", ioctl_args.key);
    ioctl(fd, MY_IOCTL_IN, &ioctl_args);

    lseek(fd, 0, SEEK_SET); // Return to devicedriver file's beginning
    read(fd, read_buf, sizeof(read_buf));
    printf("Read encrypted message: %s\n", read_buf);

    close(fd);
    return 0;
}