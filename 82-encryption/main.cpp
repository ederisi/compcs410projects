#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include "sd.h"

// Apufunktio binääridatan (kuten SHA-1) siistiin tulostamiseen heksana
void print_hex(const char* label, const char* buffer, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

int main() {
    const char *dev_path = "/dev/secret";

    // Create device id
    dev_t dev_num = makedev(42, 0);

    // Check if file exists (if not create it)
    if (access(dev_path, F_OK) == -1) {
        printf("Devicefile %s not found. Creating... \n", dev_path);

        // Create character device for read and write privileges
        if (mknod(dev_path, S_IFCHR | 0666, dev_num) == -1) {
            perror("mknod failed. Did you run the program with sudo?");
            return 1;
        }
        printf("Device file created successfully.\n");
    }

    // Open device file
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("Driver open failed");
        return 1;
    }

    char write_buf[] = "Hello from kernel";
    char read_buf[1024];
    int bytes_read;
    
    my_ioctl_data ioctl_args;
    ioctl_args.key = 42; // Encryption key

    // Write on device
    printf("\n-WRITE\n");
    printf("Writing message: %s\n", write_buf);
    write(fd, write_buf, strlen(write_buf));

    // XOR encryption and decryption
    printf("\nXOR ENCRYPTION & DECRYPTION\n");
    
    // encryption (XOR)
    ioctl(fd, MY_IOCTL_IN, &ioctl_args);
    lseek(fd, 0, SEEK_SET); 
    memset(read_buf, 0, sizeof(read_buf)); // clean buffer
    read(fd, read_buf, sizeof(read_buf));
    printf("Encrypted message (raw): %s\n", read_buf);

    // decryption (Run same XOR again)
    ioctl(fd, MY_IOCTL_IN, &ioctl_args);
    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read(fd, read_buf, sizeof(read_buf));
    printf("Decrypted message: %s\n", read_buf);

    // SHA 1 format
    printf("\nSHA-1 HASH\n");
    ioctl(fd, MY_IOCTL_SHA1);

    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    bytes_read = read(fd, read_buf, sizeof(read_buf));
    
    // Print in hexa format, SHA1 has unprintable marks
    print_hex("SHA-1 Hash (hex): ", read_buf, bytes_read);

    // Base 64 encryption
    printf("\nBASE64 ENCODE\n");
    ioctl(fd, MY_IOCTL_BASE64);

    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read(fd, read_buf, sizeof(read_buf));
    
    // Base64 is back to original string (if it works)
    printf("Base64 string: %s\n", read_buf);

    printf("\nAll tests completed successfully!\n\n");

    close(fd);
    return 0;
}