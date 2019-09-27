#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "mychardevdrv.h"

static dev_t first;
static struct class *my_device_class_ptr;
static struct cdev my_char_dev;

#define MAXLEN	4000
static char mybuffer[MAXLEN];

static int vdg_open(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s: vdg_open() called.\n", THIS_MODULE->name);
	dump_stack();
	return 0;
}

static int vdg_close(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s: vdg_close() called.\n", THIS_MODULE->name);
	return 0;
}

static ssize_t vdg_write(struct file *file_ptr, const char __user *buffer, size_t length, loff_t *offset)
{
	int max_bytes, bytes_to_write, nbytes;
	printk(KERN_INFO "%s: vdg_write() called.\n", THIS_MODULE->name);
	max_bytes = MAXLEN - *offset;
	if(max_bytes > length)
		bytes_to_write = length;
	else
		bytes_to_write = max_bytes;
	if(bytes_to_write == 0)
		return -ENOSPC;
	nbytes = bytes_to_write - copy_from_user(mybuffer + *offset, buffer, bytes_to_write);
	*offset = *offset + nbytes;
	return nbytes;
}

static ssize_t vdg_read(struct file *file_ptr, char __user *buffer, size_t length, loff_t *offset)
{
	int max_bytes, bytes_to_read, nbytes;
	printk(KERN_INFO "%s: vdg_read() called.\n", THIS_MODULE->name);
	max_bytes = MAXLEN - *offset;
	if(max_bytes > length)
		bytes_to_read = length;
	else
		bytes_to_read = max_bytes;
	if(bytes_to_read == 0)
		return 0;
	nbytes = bytes_to_read - copy_to_user(buffer, mybuffer + *offset, bytes_to_read);
	*offset = *offset + nbytes;
	return nbytes;
}

static loff_t vdg_lseek(struct file *file_ptr, loff_t offset, int origin)
{
	int invalid_flag = 0;
	loff_t new_pos = 0;
	printk(KERN_INFO "%s: vdg_lseek() called.\n", THIS_MODULE->name);
	switch(origin)
	{
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = file_ptr->f_pos + offset;
		break;
	case SEEK_END:
		new_pos = MAXLEN + offset;
		break;
	default:
		invalid_flag = 1;
	}
	if(invalid_flag)
		return -EINVAL;
	if(new_pos < 0)
		new_pos = 0;
	if(new_pos > MAXLEN)
		new_pos = MAXLEN;
	file_ptr->f_pos = new_pos;
	return new_pos;
}

static inline char to_upper(char ch)
{
	return (ch >= 'a' && ch <= 'z' ? ch - 32 : ch);
}

static inline char to_lower(char ch)
{
	return (ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
}

static long vdg_ioctl(struct file *file_ptr, unsigned int cmd, unsigned long param)
{
	int cas, i;
	printk(KERN_INFO "%s: vdg_ioctl() called.\n", THIS_MODULE->name);
	switch(cmd)
	{
	case VDG_CLEAR:
		printk(KERN_INFO "%s: vdg_ioctl() called -- VDG_CLEAR\n", THIS_MODULE->name);
		memset(mybuffer, 0, MAXLEN);
		return 0;
	case VDG_CHANGE_CASE:
		printk(KERN_INFO "%s: vdg_ioctl() called -- VDG_CHANGE_CASE\n", THIS_MODULE->name);
		if(copy_from_user(&cas, (void*)param, sizeof(int)))
			return -EACCES;
		if(cas == VDG_UPPER)
		{
			for(i=0; i<MAXLEN; i++)
				mybuffer[i] = to_upper(mybuffer[i]);
		}
		else if(cas == VDG_LOWER)
		{
			for(i=0; i<MAXLEN; i++)
				mybuffer[i] = to_lower(mybuffer[i]);
		}
		else
			break;
		return 0;
	}
	return -EINVAL;
}

static struct file_operations vdg_fops = {
	.owner = THIS_MODULE,
	.open = vdg_open,
	.release = vdg_close,
	.write = vdg_write,
	.read = vdg_read,
	.llseek = vdg_lseek,
	.unlocked_ioctl = vdg_ioctl
};


static int __init vdg_init(void)
{
	int major, minor;
	int ret;
	struct device *my_device_ptr;
	
	printk(KERN_INFO "%s: vdg_init() called.\n", THIS_MODULE->name);
	// alloc major/minor dev number for the device
	first = MKDEV(250, 0);
	ret = alloc_chrdev_region(&first, 0, 1, "vdg");
	if(ret < 0)
	{
		printk(KERN_INFO "%s: alloc_chrdev_region() failed\n", THIS_MODULE->name);
		return -1;
	}
	major = MAJOR(first);
	minor = MINOR(first);
	printk(KERN_INFO "%s: alloc_chrdev_region() success: device number = %d - %d\n", THIS_MODULE->name, major, minor);
	// create device class
	my_device_class_ptr = class_create(THIS_MODULE, "vdg_class");
	if(my_device_class_ptr == NULL) 
	{
		printk(KERN_INFO "%s: class_create() failed\n", THIS_MODULE->name);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	printk(KERN_INFO "%s: class_create() success: device class vdg_class created.\n", THIS_MODULE->name);
	// create device file
	my_device_ptr = device_create(my_device_class_ptr, NULL, first, NULL, "vdg_chrdev");
	if(my_device_ptr == NULL)
	{
		printk(KERN_INFO "%s: device_create() failed\n", THIS_MODULE->name);
		class_destroy(my_device_class_ptr);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	printk(KERN_INFO "%s: device_create() success: device vdg_chrdev created.\n", THIS_MODULE->name);
	// init cdev struct & register into kernel
	cdev_init(&my_char_dev, &vdg_fops);	
	ret = cdev_add(&my_char_dev, first, 1);
	if(ret < 0)
	{
		printk(KERN_INFO "%s: cdev_add() failed\n", THIS_MODULE->name);
		device_destroy(my_device_class_ptr, first);
		class_destroy(my_device_class_ptr);
		unregister_chrdev_region(first, 1);
		return -1;	
	}
	printk(KERN_INFO "%s: cdev_add() success.\n", THIS_MODULE->name);

	return 0;
}

static void __exit vdg_exit(void)
{
	printk(KERN_INFO "%s: vdg_exit() called...\n", THIS_MODULE->name);	
	// unregister cdev from kernel
	cdev_del(&my_char_dev);
	// destroy device files
	device_destroy(my_device_class_ptr, first);
	// destroy device class
	class_destroy(my_device_class_ptr);
	// release device number
	unregister_chrdev_region(first, 1);
}

module_init(vdg_init);
module_exit(vdg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vijay Gokhale <astromedicomp@yahoo.com>");
MODULE_DESCRIPTION("Psuedo char device driver demo.");

