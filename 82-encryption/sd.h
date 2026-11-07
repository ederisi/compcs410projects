#ifndef SD_H
#define SD_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define BUF_SIZE 1024
#define SHA1_LENGTH 20

struct my_device_data {
    struct cdev cdev;
    char buffer[BUF_SIZE];
    size_t size;
};

typedef struct {
    int key;
} my_ioctl_data;

#define MY_IOCTL_IN _IOC(_IOC_WRITE, 'k', 1, sizeof(my_ioctl_data))
#define MY_IOCTL_OUT _IOC(_IOC_READ, 'k', 1, sizeof(my_ioctl_data))
#define MY_IOCTL_SHA1 _IOC(_IOC_NONE, 'k', 2, 0)
#define MY_IOCTL_BASE64 _IOC(_IOC_NONE, 'k', 3, 0)

// Helper functions
void apply_xor_encryption(struct my_device_data *data, int key);
int apply_sha1_hash(struct my_device_data *data);
int apply_base64_encode(struct my_device_data *data);

#endif