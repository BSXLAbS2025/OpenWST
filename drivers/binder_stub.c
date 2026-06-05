// SPDX-License-Identifier: GPL-2.0
/*
 * Binder stub driver for OpenWST
 * Android IPC — нужен для общения между HAL-сервисами
 * (упрощённая версия, только чтобы не падало)
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BINDER_DEVICE_NAME "binder"
#define BINDER_CLASS_NAME  "binder"
#define BINDER_NUM_DEVICES 1

// Базовые ioctl коды (полный список огромен, эмулируем только основные)
#define BINDER_WRITE_READ   _IOWR('b', 1, struct binder_write_read)
#define BINDER_SET_IDLE     _IOW('b', 2, int64_t)
#define BINDER_SET_MAX_THREADS _IOW('b', 5, size_t)
#define BINDER_SET_IDLE_TIMEOUT _IOW('b', 6, int64_t)
#define BINDER_SET_CONTEXT_MGR _IOW('b', 7, int)
#define BINDER_THREAD_EXIT  _IOW('b', 8, int)
#define BINDER_VERSION      _IOWR('b', 9, struct binder_version)

struct binder_version {
    int32_t protocol_version;
};

struct binder_write_read {
    void __user *write_buffer;
    void __user *read_buffer;
    size_t write_size;
    size_t read_size;
    size_t write_consumed;
    size_t read_consumed;
};

static struct class *binder_class;
static dev_t binder_devt;

static int binder_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "BINDER_STUB: open() called\n");
    return 0;
}

static int binder_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "BINDER_STUB: release() called\n");
    return 0;
}

static long binder_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct binder_write_read bwr;
    struct binder_version ver;
    
    printk(KERN_DEBUG "BINDER_STUB: ioctl(cmd=0x%x)\n", cmd);
    
    switch (cmd) {
        case BINDER_WRITE_READ:
            if (copy_from_user(&bwr, (void __user *)arg, sizeof(bwr)))
                return -EFAULT;
            // Ничего не делаем, просто помечаем всё как обработанное
            bwr.write_consumed = bwr.write_size;
            bwr.read_consumed = bwr.read_size;
            if (copy_to_user((void __user *)arg, &bwr, sizeof(bwr)))
                return -EFAULT;
            return 0;
            
        case BINDER_SET_MAX_THREADS:
            // Игнорируем, но логируем
            printk(KERN_DEBUG "BINDER_STUB: set max threads = %ld\n", arg);
            return 0;
            
        case BINDER_SET_CONTEXT_MGR:
            printk(KERN_DEBUG "BINDER_STUB: set context manager\n");
            return 0;
            
        case BINDER_THREAD_EXIT:
            printk(KERN_DEBUG "BINDER_STUB: thread exit\n");
            return 0;
            
        case BINDER_VERSION:
            ver.protocol_version = 8; // Android 8+
            if (copy_to_user((void __user *)arg, &ver, sizeof(ver)))
                return -EFAULT;
            return 0;
            
        case BINDER_SET_IDLE:
        case BINDER_SET_IDLE_TIMEOUT:
            return 0;  // просто игнорируем
            
        default:
            printk(KERN_WARNING "BINDER_STUB: unknown cmd 0x%x\n", cmd);
            return 0;
    }
}

static const struct file_operations binder_fops = {
    .owner          = THIS_MODULE,
    .open           = binder_open,
    .release        = binder_release,
    .unlocked_ioctl = binder_ioctl,
};

static int __init binder_stub_init(void)
{
    int ret;
    
    printk(KERN_INFO "BINDER_STUB: initializing\n");
    
    ret = alloc_chrdev_region(&binder_devt, 0, BINDER_NUM_DEVICES, BINDER_DEVICE_NAME);
    if (ret) {
        printk(KERN_ERR "BINDER_STUB: alloc_chrdev_region failed\n");
        return ret;
    }
    
    binder_class = class_create(THIS_MODULE, BINDER_CLASS_NAME);
    if (IS_ERR(binder_class)) {
        ret = PTR_ERR(binder_class);
        goto err_class;
    }
    
    device_create(binder_class, NULL, binder_devt, NULL, BINDER_DEVICE_NAME);
    
    printk(KERN_INFO "BINDER_STUB: created /dev/binder\n");
    return 0;
    
err_class:
    unregister_chrdev_region(binder_devt, BINDER_NUM_DEVICES);
    return ret;
}

static void __exit binder_stub_exit(void)
{
    device_destroy(binder_class, binder_devt);
    class_destroy(binder_class);
    unregister_chrdev_region(binder_devt, BINDER_NUM_DEVICES);
    printk(KERN_INFO "BINDER_STUB: exited\n");
}

module_init(binder_stub_init);
module_exit(binder_stub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenWST Team");
MODULE_DESCRIPTION("Binder stub driver for Android compatibility");
MODULE_VERSION("0.1");
