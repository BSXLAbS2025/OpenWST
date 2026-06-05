// SPDX-License-Identifier: GPL-2.0
/*
 * Mali GPU stub driver for OpenWST
 * Samsung Exynos 3475 (Mali-T720)
 * 
 * Этот драйвер-заглушка заставляет библиотеку libGLES_mali.so думать,
 * что GPU существует и работает. Все ioctl() возвращают успех.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

// Имя устройства (как в реальном Android)
#define MALI_DEVICE_NAME "mali0"
#define MALI_CLASS_NAME  "mali"

// Количество фиктивных устройств (на случай если нужно несколько)
#define MALI_NUM_DEVICES 1

// Некоторые ioctl коды из реального драйвера Mali (можно добавлять по мере необходимости)
// На первом этапе мы их не проверяем, просто возвращаем 0
#define MALI_IOC_JOB_SUBMIT   0x4001
#define MALI_IOC_JOB_FINISH   0x4002
#define MALI_IOC_MEM_ALLOC    0x4003
#define MALI_IOC_MEM_FREE     0x4004

// Структура для хранения состояния устройства
struct mali_device {
    struct cdev cdev;
    struct device *device;
    int id;
};

// Глобальные переменные
static struct mali_device mali_dev[MALI_NUM_DEVICES];
static struct class *mali_class;
static dev_t mali_devt;

/*
 * open() - вызывается при открытии /dev/mali0
 * Возвращает 0 (успех) всегда
 */
static int mali_open(struct inode *inode, struct file *file)
{
    struct mali_device *dev = container_of(inode->i_cdev, struct mali_device, cdev);
    
    printk(KERN_INFO "MALI_STUB: open() called for device %d\n", dev->id);
    
    // Сохраняем указатель на устройство в private_data для последующих вызовов
    file->private_data = dev;
    
    return 0;  // Всегда успешно
}

/*
 * release() - вызывается при закрытии /dev/mali0
 */
static int mali_release(struct inode *inode, struct file *file)
{
    struct mali_device *dev = file->private_data;
    
    printk(KERN_INFO "MALI_STUB: release() called for device %d\n", dev->id);
    
    return 0;
}

/*
 * ioctl() - главный обработчик команд GPU
 * На этом этапе просто логируем команду и возвращаем 0 (успех)
 */
static long mali_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct mali_device *dev = file->private_data;
    
    printk(KERN_DEBUG "MALI_STUB: ioctl(dev=%d, cmd=0x%x, arg=0x%lx)\n", 
           dev->id, cmd, arg);
    
    // Здесь в будущем можно добавить реальную обработку специфичных команд
    switch (cmd) {
        case MALI_IOC_JOB_SUBMIT:
            printk(KERN_DEBUG "MALI_STUB: JOB_SUBMIT (ignored)\n");
            break;
        case MALI_IOC_JOB_FINISH:
            printk(KERN_DEBUG "MALI_STUB: JOB_FINISH (ignored)\n");
            break;
        case MALI_IOC_MEM_ALLOC:
            printk(KERN_DEBUG "MALI_STUB: MEM_ALLOC (ignored)\n");
            break;
        case MALI_IOC_MEM_FREE:
            printk(KERN_DEBUG "MALI_STUB: MEM_FREE (ignored)\n");
            break;
        default:
            printk(KERN_DEBUG "MALI_STUB: unknown cmd 0x%x\n", cmd);
            break;
    }
    
    // Всегда возвращаем 0 (успех) — библиотека будет думать, что GPU работает
    return 0;
}

/*
 * mmap() - отображение памяти GPU в пользовательское пространство
 * Возвращаем фиктивную страницу, заполненную нулями
 */
static int mali_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct mali_device *dev = file->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;
    void *dummy_page;
    int ret;
    
    printk(KERN_INFO "MALI_STUB: mmap() called for device %d, size=%lu\n", 
           dev->id, size);
    
    // Проверяем, что запрашиваемый размер не слишком большой (максимум 4MB)
    if (size > 4 * 1024 * 1024) {
        printk(KERN_WARNING "MALI_STUB: mmap size too big: %lu\n", size);
        return -EINVAL;
    }
    
    // Выделяем фиктивную страницу (заполненную нулями)
    dummy_page = get_zeroed_page(GFP_KERNEL);
    if (!dummy_page) {
        printk(KERN_ERR "MALI_STUB: failed to allocate dummy page\n");
        return -ENOMEM;
    }
    
    // Отображаем фиктивную страницу в пользовательское пространство
    ret = remap_pfn_range(vma, vma->vm_start,
                          virt_to_phys(dummy_page) >> PAGE_SHIFT,
                          size, vma->vm_page_prot);
    
    if (ret) {
        printk(KERN_ERR "MALI_STUB: remap_pfn_range failed\n");
        free_page((unsigned long)dummy_page);
        return ret;
    }
    
    // Сохраняем указатель, чтобы позже освободить (но mmap может быть вызван много раз)
    // В реальном драйвере нужно хранить список всех отображённых страниц
    // Для заглушки просто игнорируем
    
    printk(KERN_INFO "MALI_STUB: mmap() completed successfully\n");
    return 0;
}

// Операции файла (что умеет наше устройство)
static const struct file_operations mali_fops = {
    .owner          = THIS_MODULE,
    .open           = mali_open,
    .release        = mali_release,
    .unlocked_ioctl = mali_ioctl,
    .mmap           = mali_mmap,
};

/*
 * Инициализация модуля при загрузке ядра
 */
static int __init mali_stub_init(void)
{
    int ret, i;
    
    printk(KERN_INFO "MALI_STUB: Initializing Mali stub driver\n");
    
    // Динамически выделяем номер устройства (major/minor)
    ret = alloc_chrdev_region(&mali_devt, 0, MALI_NUM_DEVICES, MALI_DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "MALI_STUB: failed to allocate device number\n");
        return ret;
    }
    
    // Создаём класс устройств (появится в /sys/class/mali/)
    mali_class = class_create(THIS_MODULE, MALI_CLASS_NAME);
    if (IS_ERR(mali_class)) {
        ret = PTR_ERR(mali_class);
        printk(KERN_ERR "MALI_STUB: failed to create class\n");
        goto err_class;
    }
    
    // Создаём каждое устройство
    for (i = 0; i < MALI_NUM_DEVICES; i++) {
        dev_t devno = MKDEV(MAJOR(mali_devt), i);
        
        // Инициализируем cdev
        cdev_init(&mali_dev[i].cdev, &mali_fops);
        mali_dev[i].cdev.owner = THIS_MODULE;
        mali_dev[i].id = i;
        
        // Добавляем cdev в систему
        ret = cdev_add(&mali_dev[i].cdev, devno, 1);
        if (ret) {
            printk(KERN_ERR "MALI_STUB: failed to add cdev for device %d\n", i);
            goto err_cdev;
        }
        
        // Создаём устройство в /dev/mali0
        mali_dev[i].device = device_create(mali_class, NULL, devno, NULL,
                                           MALI_DEVICE_NAME "%d", i);
        if (IS_ERR(mali_dev[i].device)) {
            ret = PTR_ERR(mali_dev[i].device);
            printk(KERN_ERR "MALI_STUB: failed to create device %d\n", i);
            goto err_device;
        }
        
        printk(KERN_INFO "MALI_STUB: created /dev/mali%d\n", i);
    }
    
    printk(KERN_INFO "MALI_STUB: initialization complete\n");
    return 0;
    
err_device:
    cdev_del(&mali_dev[i].cdev);
err_cdev:
    for (--i; i >= 0; i--) {
        device_destroy(mali_class, MKDEV(MAJOR(mali_devt), i));
        cdev_del(&mali_dev[i].cdev);
    }
    class_destroy(mali_class);
err_class:
    unregister_chrdev_region(mali_devt, MALI_NUM_DEVICES);
    return ret;
}

/*
 * Завершение модуля (при выгрузке)
 */
static void __exit mali_stub_exit(void)
{
    int i;
    
    printk(KERN_INFO "MALI_STUB: exiting\n");
    
    // Удаляем все устройства
    for (i = 0; i < MALI_NUM_DEVICES; i++) {
        device_destroy(mali_class, MKDEV(MAJOR(mali_devt), i));
        cdev_del(&mali_dev[i].cdev);
    }
    
    class_destroy(mali_class);
    unregister_chrdev_region(mali_devt, MALI_NUM_DEVICES);
    
    printk(KERN_INFO "MALI_STUB: exited\n");
}

module_init(mali_stub_init);
module_exit(mali_stub_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenWST Team");
MODULE_DESCRIPTION("Mali GPU stub driver for Samsung Exynos 3475");
MODULE_VERSION("0.1");
