// SPDX-License-Identifier: GPL-2.0
/*
 * Ashmem stub driver for OpenWST
 * Android Shared Memory — нужно для графики и IPC
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define ASHMEM_DEVICE_NAME "ashmem"
#define ASHMEM_CLASS_NAME  "ashmem"
#define ASHMEM_NUM_DEVICES 1

// ioctl команды (из реального Android ashmem)
#define ASHMEM_SET_NAME     _IOW('A', 0x01, char[ASHMEM_NAME_LEN])
#define ASHMEM_GET_NAME     _IOR('A', 0x02, char[ASHMEM_NAME_LEN])
#define ASHMEM_SET_SIZE     _IOW('A', 0x03, size_t)
#define ASHMEM_GET_SIZE     _IOR('A', 0x04, size_t)
#define ASHMEM_PIN          _IO('A', 0x05)
#define ASHMEM_UNPIN        _IO('A', 0x06)
#define ASHMEM_GET_PIN_STATUS _IOR('A', 0x07, int)

#define ASHMEM_NAME_LEN     256

struct ashmem_area {
    char name[ASHMEM_NAME_LEN];
    size_t size;
    int pinned;
};

static struct ashmem_area dummy_areas[ASHMEM_NUM_DEVICES];
static struct class *ashmem_class;
static dev_t ashmem_devt;

static int ashmem_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    printk(KERN_INFO "ASHMEM_STUB: open() called for minor %d\n", minor);
    
    if (minor >= ASHMEM_NUM_DEVICES)
        return -ENODEV;
    
    file->private_data = &dummy_areas[minor];
    return 0;
}

static int ashmem_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "ASHMEM_STUB: release() called\n");
    return 0;
}

static long ashmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ashmem_area *area = file->private_data;
    char __user *name_ptr;
    size_t __user *size_ptr;
    char local_name[ASHMEM_NAME_LEN];
    
    printk(KERN_DEBUG "ASHMEM_STUB: ioctl(cmd=0x%x)\n", cmd);
    
    switch (cmd) {
        case ASHMEM_SET_NAME:
            name_ptr = (char __user *)arg;
            if (copy_from_user(local_name, name_ptr, ASHMEM_NAME_LEN))
                return -EFAULT;
            local_name[ASHMEM_NAME_LEN-1] = '\0';
            strncpy(area->name, local_name, ASHMEM_NAME_LEN);
            printk(KERN_DEBUG "ASHMEM_STUB: set name = %s\n", area->name);
            return 0;
            
        case ASHMEM_GET_NAME:
            name_ptr = (char __user *)arg;
            if (copy_to_user(name_ptr, area->name, ASHMEM_NAME_LEN))
                return -EFAULT;
            return 0;
            
        case ASHMEM_SET_SIZE:
            size_ptr = (size_t __user *)arg;
            if (copy_from_user(&area->size, size_ptr, sizeof(size_t)))
                return -EFAULT;
            printk(KERN_DEBUG "ASHMEM_STUB: set size = %zu\n", area->size);
            return 0;
            
        case ASHMEM_GET_SIZE:
            size_ptr = (size_t __user *)arg;
            if (copy_to_user(size_ptr, &area->size, sizeof(size_t)))
                return -EFAULT;
            return 0;
            
        case ASHMEM_PIN:
            area->pinned = 1;
            return 0;
            
        case ASHMEM_UNPIN:
            area->pinned = 0;
            return 0;
            
        case ASHMEM_GET_PIN_STATUS:
            if (put_user(area->pinned, (int __user *)arg))
                return -EFAULT;
            return 0;
            
        default:
            printk(KERN_WARNING "ASHMEM_STUB: unknown cmd 0x%x\n", cmd);
            return 0;  // всегда успех
    }
}

static int ashmem_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct ashmem_area *area = file->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;
    void *dummy_page;
    
    printk(KERN_INFO "ASHMEM_STUB: mmap(size=%lu)\n", size);
    
    if (area->size == 0)
        area->size = size;  // авто-установка размера
    
    if (size > area->size) {
        printk(KERN_WARNING "ASHMEM_STUB: mmap size %lu > area size %zu\n", 
               size, area->size);
        return -EINVAL;
    }
    
    dummy_page = get_zeroed_page(GFP_KERNEL);
    if (!dummy_page)
        return -ENOMEM;
    
    return remap_pfn_range(vma, vma->vm_start,
                           virt_to_phys(dummy_page) >> PAGE_SHIFT,
                           size, vma->vm_page_prot);
}

static const struct file_operations ashmem_fops = {
    .owner          = THIS_MODULE,
    .open           = ashmem_open,
    .release        = ashmem_release,
    .unlocked_ioctl = ashmem_ioctl,
    .mmap           = ashmem_mmap,
};

static int __init ashmem_stub_init(void)
{
    int ret;
    
    printk(KERN_INFO "ASHMEM_STUB: initializing\n");
    
    ret = alloc_chrdev_region(&ashmem_devt, 0, ASHMEM_NUM_DEVICES, ASHMEM_DEVICE_NAME);
    if (ret) {
        printk(KERN_ERR "ASHMEM_STUB: alloc_chrdev_region failed\n");
        return ret;
    }
    
    ashmem_class = class_create(THIS_MODULE, ASHMEM_CLASS_NAME);
    if (IS_ERR(ashmem_class)) {
        ret = PTR_ERR(ashmem_class);
        goto err_class;
    }
    
    // Инициализируем фиктивные области
    memset(dummy_areas, 0, sizeof(dummy_areas));
    
    // Создаём устройство /dev/ashmem
    device_create(ashmem_class, NULL, ashmem_devt, NULL, ASHMEM_DEVICE_NAME);
    
    printk(KERN_INFO "ASHMEM_STUB: created /dev/ashmem\n");
    return 0;
    
err_class:
    unregister_chrdev_region(ashmem_devt, ASHMEM_NUM_DEVICES);
    return ret;
}

static void __exit ashmem_stub_exit(void)
{
    device_destroy(ashmem_class, ashmem_devt);
    class_destroy(ashmem_class);
    unregister_chrdev_region(ashmem_devt, ASHMEM_NUM_DEVICES);
    printk(KERN_INFO "ASHMEM_STUB: exited\n");
}

module_init(ashmem_stub_init);
module_exit(ashmem_stub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenWST Team");
MODULE_DESCRIPTION("Ashmem stub driver for Android compatibility");
