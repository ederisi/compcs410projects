#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include "sd.h"


#define MY_MAJOR       42
#define MY_MAX_MINORS  5
#define BUF_SIZE       1024

#define MY_IOCTL_OUT _IOC( _IOC_READ, 'k', 1, sizeof(my_ioctl_data))


// Device's struct
struct my_device_data {
    struct cdev cdev;
    char buffer[BUF_SIZE];
    size_t size;
};
// Device board
static struct my_device_data devs[MY_MAX_MINORS];

// Open call
static int my_open(struct inode *inode, struct file *file)
{
    struct my_device_data *my_data;
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    // validate access to device
    file->private_data = my_data;
    return 0;
}

// Release call
static int my_release(struct inode *inode, struct file *file) { 
    return 0;
}

// Read call
static ssize_t my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;
    my_data = (struct my_device_data *) file->private_data;
    ssize_t len = min(my_data->size - *offset, size);

    if (*offset >= my_data->size) return 0;
    if (len <= 0) return 0;

    // read data from my_data->buffer to user buffer
    if (copy_to_user(user_buffer, my_data->buffer + *offset, len)) return -EFAULT;

    *offset += len;
    return len;
}

// Write call
static ssize_t my_write(struct file *file, const char __user *user_buffer,
                    size_t size, loff_t * offset)
{
    struct my_device_data *my_data = (struct my_device_data *) file->private_data;

    if (*offset >= BUF_SIZE) return -ENOSPC;
    ssize_t len = min((size_t)(BUF_SIZE - *offset), size);
    if (len <= 0) return 0;

    /* read data from user buffer to my_data->buffer */
    if (copy_from_user(my_data->buffer + *offset, user_buffer, len)) return -EFAULT;

    *offset += len;

    if (*offset > my_data->size) {
        my_data->size = *offset;
    }

    return len;
}

// IOCTL-call
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_device_data *my_data = (struct my_device_data*) file->private_data;
    my_ioctl_data mid;

    switch(cmd) {
        case MY_IOCTL_IN:
            if (copy_from_user(&mid, (my_ioctl_data __user *) arg, sizeof(my_ioctl_data)))
                return -EFAULT;
        
            // Actualy encryption XOR
            for (int i = 0; i < my_data->size; i++) {
                my_data->buffer[i] ^= mid.key;
            }
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

// File operations
static const struct file_operations my_fops ={
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl
};

// Module initialization
static int __init my_init_module(void) {
    int i, err;

    err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS, "my_device_driver");
    if (err != 0) return err;

    for (i=0; i < MY_MAX_MINORS; i++) {
        devs[i].size = 0;
        cdev_init(&devs[i].cdev, &my_fops);
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
    }

    return 0;
}

// Erase module
static void __exit my_cleanup_module(void) {
    int i;
    for (i=0; i < MY_MAX_MINORS ; i++) {
        cdev_del(&devs[i].cdev);
    }

    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS);
}

// Macros for module load
module_init(my_init_module);
module_exit(my_cleanup_module);
MODULE_LICENSE("GPL");