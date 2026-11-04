#ifndef SD_H
#define SD_H
#include <linux/ioctl.h>

typedef struct {
    int key;
} my_ioctl_data;

#define MY_IOCTL_IN _IOC(_IOC_WRITE, 'k', 1, sizeof(my_ioctl_data))

#endif