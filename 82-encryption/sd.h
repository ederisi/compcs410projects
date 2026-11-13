#ifndef SD_H
#define SD_H

#include <linux/ioctl.h>

// Define maximum data buffer size and fixed SHA-1 length
#define BUF_SIZE 1024
#define SHA1_LENGTH 20

#ifdef __KERNEL__
#include <linux/cdev.h>
// Structure holding state data and buffer storage
struct my_device_data {
    struct cdev cdev;
    char buffer[BUF_SIZE];
    size_t size;
    int key;
};
// Kernel side cryptographic and encoding function prototypes
void apply_xor_encryption(struct my_device_data *data, int key);
int apply_sha1_hash(struct my_device_data *data);
int apply_base64_encode(struct my_device_data *data);
#endif

// Datastructure passed between user space and kernel space (via ioctl calls)
typedef struct {
    int key;
} my_ioctl_data;

// IOCTL control command definitions
#define MY_IOCTL_IN _IO('k', 1)
#define MY_IOCTL_SET_KEY _IOW('k', 4, my_ioctl_data)
#define MY_IOCTL_OUT _IOC(_IOC_READ, 'k', 1, sizeof(my_ioctl_data))
#define MY_IOCTL_SHA1 _IOC(_IOC_NONE, 'k', 2, 0)
#define MY_IOCTL_BASE64 _IOC(_IOC_NONE, 'k', 3, 0)

#endif