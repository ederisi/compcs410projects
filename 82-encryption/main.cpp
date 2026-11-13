#include <iostream>
#include <cerrno>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include "sd.h"

int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc < 3 || (strcmp(argv[1], "--encrypt") != 0 && strcmp(argv[1], "--setkey") != 0)) {
        std::cerr << "Usage: " << argv[0] << " --setkey <integer> | --encrypt <message> [message ...]\n";
        return 2;
    }

    const char *dev_path = "/dev/secret"; // Path to the driver

    // Check if driver exists, else try creating it
    dev_t dev_num = makedev(42, 0);
    if (access(dev_path, F_OK) == -1) {
        if (mknod(dev_path, S_IFCHR | 0666, dev_num) == -1) {
            std::cerr << "mknod failed; create the device node with sufficient privileges: " << std::strerror(errno) << '\n';
            return 1;
        }
    }

    // Open the device file descriptor with read/write permission
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        std::cerr << "Driver open failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // Handle '--setkey' command to update the encryption key inside the kernel
    if (strcmp(argv[1], "--setkey") == 0) {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " --setkey <integer>\n";
            close(fd);
            return 2;
        }
        char *end = NULL;
        long parsed_key = strtol(argv[2], &end, 10);

        // Validate that provided key is a valid integer
        if (end == argv[2] || *end != '\0' ||
            parsed_key < -2147483648L || parsed_key > 2147483647L) {
            std::cerr << "Invalid key: " << argv[2] << '\n';
            close(fd);
            return 2;
        }
        my_ioctl_data args = { .key = (int)parsed_key };
        if (ioctl(fd, MY_IOCTL_SET_KEY, &args) == -1) {
            std::cerr << "Setting driver key failed: " << std::strerror(errno) << '\n';
            close(fd);
            return 1;
        }
        std::cout << "Driver key updated.\n";
        close(fd);
        return 0;
    }

    // Try encrypting each string in lauch params (except the method name)
    char read_buf[BUF_SIZE + 1];
    int result = 0;
    for (int i = 2; i < argc; ++i) {
        size_t message_len = strlen(argv[i]);
        if (message_len > BUF_SIZE) {
            std::cerr << "Message " << i - 1 << " exceeds " << BUF_SIZE << " bytes\n";
            return 1;
        }

        // Reopenthe devide file, once per message. This resets the device file position to zero.
        int fd = open(dev_path, O_RDWR);
        if (fd < 0) {
            std::cerr << "Driver open failed: " << std::strerror(errno) << '\n';
            return 1;
        }

        // Write the plain text msg buffer into kernel driver
        if (write(fd, argv[i], message_len) != static_cast<ssize_t>(message_len)) {
            std::cerr << "Writing message failed: " << std::strerror(errno) << '\n';
            close(fd);
            return 1;
        }

        // Trigger the kernel-side encryption routine via ioctl
        if (ioctl(fd, MY_IOCTL_IN) == -1) {
            std::cerr << "Encrypting message failed: " << std::strerror(errno) << '\n';
            close(fd);
            return 1;
        }

        // Reset device position without lseek; this driver has no llseek handler.
        close(fd);
        fd = open(dev_path, O_RDONLY);
        if (fd < 0) {
            std::cerr << "Reopening driver failed: " << std::strerror(errno) << '\n';
            return 1;
        }

        // Read resulting ecrypted/hashed/encoded output from the driver buffer
        ssize_t bytes_read = read(fd, read_buf, BUF_SIZE);
        if (bytes_read < 0) {
            std::cerr << "Reading encrypted message failed: " << std::strerror(errno) << '\n';
            close(fd);
            return 1;
        }

        // Null-terminate and print the processed output
        read_buf[bytes_read] = '\0';
        std::cout << read_buf << '\n';
        close(fd);
    }

    close(fd);
    return result;
}
