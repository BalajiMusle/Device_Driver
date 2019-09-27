#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kfifo.h>
#include <linux/slab.h>

#define DEV_NAME		"pchar"
#define DEV_CLASS_NAME	DEV_NAME"_class"

#define MAXLEN	1000

static dev_t first;
static struct class *my_device_class_ptr;

static int dev_count = 2;
module_param(dev_count, int, S_IRUGO);

typedef struct pchar_private
{
	struct kfifo my_fifo;
	struct cdev my_char_dev;
	// ...
}pchar_private_t;

static pchar_private_t *devices;

static int pchar_open(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s: pchar_open() called.\n", THIS_MODULE->name);
	file_ptr->private_data = container_of(inode_ptr->i_cdev, pchar_private_t, my_char_dev);
	return 0;
}

static int pchar_close(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s: pchar_close() called.\n", THIS_MODULE->name);
	return 0;
}

static ssize_t pchar_write(struct file *file_ptr, const char __user *buffer, size_t length, loff_t *offset)
{
	int ret, nbytes = 0;
	pchar_private_t *dev = (pchar_private_t*)file_ptr->private_data;
	printk(KERN_INFO "%s: pchar_write() called.\n", THIS_MODULE->name);
	if(!access_ok(VERIFY_READ, buffer, length))
	{
		printk(KERN_INFO "%s: pchar_write() user space buffer is not accessible.\n", THIS_MODULE->name);
		return -EACCES;
	}
	ret = kfifo_from_user(&dev->my_fifo, buffer, length, &nbytes);
	if(ret)
	{
		printk(KERN_INFO "%s: pchar_write() user space buffer access failed.\n", THIS_MODULE->name);
		return -EFAULT;
	}
	return nbytes;
}

static ssize_t pchar_read(struct file *file_ptr, char __user *buffer, size_t length, loff_t *offset)
{
	pchar_private_t *dev = (pchar_private_t*)file_ptr->private_data;
	int ret, nbytes = 0;
	printk(KERN_INFO "%s: pchar_read() called.\n", THIS_MODULE->name);
	if(!access_ok(VERIFY_WRITE, buffer, length))
	{
		printk(KERN_INFO "%s: pchar_read() user space buffer is not accessible.\n", THIS_MODULE->name);
		return -EACCES;
	}
	ret = kfifo_to_user(&dev->my_fifo, buffer, length, &nbytes);
	if(ret)
	{
		printk(KERN_INFO "%s: pchar_read() user space buffer access failed.\n", THIS_MODULE->name);
		return -EFAULT;
	}
	return nbytes;
}

static struct file_operations pchar_fops = {
	.owner = THIS_MODULE,
	.open = pchar_open,
	.release = pchar_close,
	.write = pchar_write,
	.read = pchar_read
};


static int __init pchar_init(void)
{
	int major, minor;
	int ret, i;
	struct device *my_device_ptr;
	dev_t devno;

	printk(KERN_INFO "%s: pchar_init() called.\n", THIS_MODULE->name);
	// alloc major/minor dev number for the device
	first = MKDEV(250, 0);
	ret = alloc_chrdev_region(&first, 0, dev_count, DEV_NAME);
	if(ret < 0)
	{
		printk(KERN_INFO "%s: alloc_chrdev_region() failed\n", THIS_MODULE->name);
		goto label_return_error;
	}
	major = MAJOR(first);
	minor = MINOR(first);
	printk(KERN_INFO "%s: alloc_chrdev_region() success: device number = %d - %d\n", THIS_MODULE->name, major, minor);
	// create device class
	my_device_class_ptr = class_create(THIS_MODULE, DEV_CLASS_NAME);
	if(my_device_class_ptr == NULL) 
	{
		printk(KERN_INFO "%s: class_create() failed\n", THIS_MODULE->name);
		goto label_unregister_chrdev_region;
	}
	printk(KERN_INFO "%s: class_create() success: device class pchar_class created.\n", THIS_MODULE->name);
	// create device files
	for(i=0; i<dev_count; i++)
	{
		devno = MKDEV(major, i);
		my_device_ptr = device_create(my_device_class_ptr, NULL, devno, NULL, DEV_NAME"%d", i);
		if(my_device_ptr == NULL)
		{
			printk(KERN_INFO "%s: device_create() failed\n", THIS_MODULE->name);
			goto label_destroy_device_and_class;
		}
		printk(KERN_INFO "%s: device_create() success: device pchar_chrdev created.\n", THIS_MODULE->name);
	}
	// allocate array of private struct
	devices = kmalloc(dev_count*sizeof(pchar_private_t), GFP_KERNEL);
	if(devices == NULL)
	{
		printk(KERN_INFO "%s: kmalloc() failed\n", THIS_MODULE->name);
		goto label_destroy_device_and_class;
	}
	// init cdev struct & register into kernel
	for(i=0; i<dev_count; i++)
	{
		cdev_init(&devices[i].my_char_dev, &pchar_fops);	
		devno = MKDEV(major, i);
		ret = cdev_add(&devices[i].my_char_dev, devno, 1);
		if(ret < 0)
		{
			printk(KERN_INFO "%s: cdev_add() failed\n", THIS_MODULE->name);
			goto label_cdev_del_and_kfree_devices;
		}
		printk(KERN_INFO "%s: cdev_add() success.\n", THIS_MODULE->name);
	}
	// init device private members -- kfifo
	for(i=0; i<dev_count; i++)
	{
		ret = kfifo_alloc(&devices[i].my_fifo, MAXLEN, GFP_KERNEL);
		if(ret)
		{
			printk(KERN_INFO "%s: kfifo_alloc() failed\n", THIS_MODULE->name);
			goto label_kfifo_free;
		}
		printk(KERN_INFO "%s: kfifo_alloc() success.\n", THIS_MODULE->name);
	}
	return 0;
	
label_kfifo_free:
	for(i=i-1; i>=0; i--)
		kfifo_free(&devices[i].my_fifo);
	i = dev_count;

label_cdev_del_and_kfree_devices:
	for(i=i-1; i>=0; i--)
		cdev_del(&devices[i].my_char_dev);

	kfree(devices);
	devices = NULL;
	i = dev_count;

label_destroy_device_and_class:
	for(i=i-1; i>=0; i--)
	{
		devno = MKDEV(major, i);
		device_destroy(my_device_class_ptr, devno);
	}
	class_destroy(my_device_class_ptr);

label_unregister_chrdev_region:
	unregister_chrdev_region(first, 1);

label_return_error:
	return -1;
}

static void __exit pchar_exit(void)
{
	int i;
	dev_t devno;
	int major = MAJOR(first);
	printk(KERN_INFO "%s: pchar_exit() called...\n", THIS_MODULE->name);	
	// deinit private memebers -- kfifo
	for(i=dev_count-1; i>=0; i--)
		kfifo_free(&devices[i].my_fifo);
	// unregister cdev from kernel
	for(i=dev_count-1; i>=0; i--)
		cdev_del(&devices[i].my_char_dev);
	// release private struct array
	kfree(devices);
	devices = NULL;
	// destroy device files
	for(i=dev_count-1; i>=0; i--)
	{
		devno = MKDEV(major, i);
		device_destroy(my_device_class_ptr, devno);
	}
	// destroy device class
	class_destroy(my_device_class_ptr);
	// release device number
	unregister_chrdev_region(first, dev_count);
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DESD SunBeam <desd@sunbeaminfo.com>");
MODULE_DESCRIPTION("Psuedo char device driver demo.");

