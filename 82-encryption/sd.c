#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include "sd.h"


#define MY_MAJOR       42
#define MY_MAX_MINORS  5
// Array of custom device driver structures (supports up to 5)
static struct my_device_data devs[MY_MAX_MINORS];

// Device open call
static int my_open(struct inode *inode, struct file *file)
{
    struct my_device_data *my_data;
    // Retrieve pointer containing my_device_data
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    // store device data pointer in file private data
    file->private_data = my_data;
    return 0;
}

// Release device call
static int my_release(struct inode *inode, struct file *file) { 
    return 0;
}

// Device read call
static ssize_t my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data;
    my_data = (struct my_device_data *) file->private_data;
    ssize_t len = min(my_data->size - *offset, size);

    if (*offset >= my_data->size) return 0;
    if (len <= 0) return 0;

    // Safely copy data from my_data->buffer to user buffer
    if (copy_to_user(user_buffer, my_data->buffer + *offset, len)) return -EFAULT;

    *offset += len;
    return len;
}

// Device write call
static ssize_t my_write(struct file *file, const char __user *user_buffer,
                    size_t size, loff_t * offset)
{
    struct my_device_data *my_data = (struct my_device_data *) file->private_data;

    // reset size counter if writing from the beginning of the file offset
    if (*offset == 0)
        my_data->size = 0;
    if (*offset >= BUF_SIZE) return -ENOSPC;
    ssize_t len = min((size_t)(BUF_SIZE - *offset), size);
    if (len <= 0) return 0;

    // safely copy data from my_data->buffer
    if (copy_from_user(my_data->buffer + *offset, user_buffer, len)) return -EFAULT;

    *offset += len;

    // Update stored data size
    if (*offset > my_data->size) {
        my_data->size = *offset;
    }

    return len;
}
// Device IOCTL call
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_device_data *my_data = file->private_data;
    my_ioctl_data mid;
    int err;

    switch (cmd) {
    case MY_IOCTL_SET_KEY:
    // Copy key structure args from user space
        if (copy_from_user(&mid, (my_ioctl_data __user *)arg, sizeof(mid)))
            return -EFAULT;
        my_data->key = mid.key;
        return 0;
    case MY_IOCTL_IN:
    // Apply encryption
        apply_xor_encryption(my_data, my_data->key);
        err = apply_sha1_hash(my_data);
        if (err)
            return err;
        return apply_base64_encode(my_data);
    default:
        return -ENOTTY; // Inappropriate ioctl command error
    }
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

// Kernel module initialization
static int __init my_init_module(void) {
    int i, err;

    // Register character device region
    err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS, "my_device_driver");
    if (err != 0) return err;

    // Initialize and add each minor device
    for (i=0; i < MY_MAX_MINORS; i++) {
        devs[i].size = 0;
        devs[i].key = 0;
        cdev_init(&devs[i].cdev, &my_fops);
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
    }

    return 0;
}

// Kernel module cleanup
static void __exit my_cleanup_module(void) {
    int i;
    for (i=0; i < MY_MAX_MINORS ; i++) {
        cdev_del(&devs[i].cdev);
    }

    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS);
}

// Register module entry and exit points
module_init(my_init_module);
module_exit(my_cleanup_module);
MODULE_LICENSE("GPL");