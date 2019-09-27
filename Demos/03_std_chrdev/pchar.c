#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DEV_NAME		"pchar"
#define DEV_CLASS_NAME	DEV_NAME"_class"

#define DEVICE_COUNT	4
#define MAXLEN	1000

static dev_t first;
static struct class *my_device_class_ptr;

typedef struct pchar_private
{
	char buffer[MAXLEN];
	struct cdev my_char_dev;
	// ...
}pchar_private_t;

static pchar_private_t devices[DEVICE_COUNT];

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
	pchar_private_t *dev = (pchar_private_t*)file_ptr->private_data;
	int max_bytes, bytes_to_write, nbytes = length;
	printk(KERN_INFO "%s: pchar_write() called.\n", THIS_MODULE->name);
	max_bytes = MAXLEN - *offset;
	if(max_bytes > length)
		bytes_to_write = length;
	else
		bytes_to_write = max_bytes;
	if(bytes_to_write == 0)
		return -ENOSPC;
	nbytes = bytes_to_write - copy_from_user(dev->buffer + *offset, buffer, bytes_to_write);
	*offset = *offset + nbytes;
	return nbytes;
}

static ssize_t pchar_read(struct file *file_ptr, char __user *buffer, size_t length, loff_t *offset)
{
	pchar_private_t *dev = (pchar_private_t*)file_ptr->private_data;
	int max_bytes, bytes_to_read, nbytes = 0;
	printk(KERN_INFO "%s: pchar_read() called.\n", THIS_MODULE->name);

	max_bytes = MAXLEN - *offset;
	if(max_bytes > length)
		bytes_to_read = length;
	else
		bytes_to_read = max_bytes;
	if(bytes_to_read == 0)
		return 0;
	nbytes = bytes_to_read - copy_to_user(buffer, dev->buffer + *offset, bytes_to_read);
	*offset = *offset + nbytes;
	return nbytes;
}

static loff_t pchar_lseek(struct file *file_ptr, loff_t offset, int origin)
{
	int invalid_flag = 0;
	loff_t new_pos = 0;
	printk(KERN_INFO "%s: pchar_lseek() called.\n", THIS_MODULE->name);
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

static struct file_operations pchar_fops = {
	.owner = THIS_MODULE,
	.open = pchar_open,
	.release = pchar_close,
	.write = pchar_write,
	.read = pchar_read,
	.llseek = pchar_lseek
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
	ret = alloc_chrdev_region(&first, 0, DEVICE_COUNT, "pchar");
	if(ret < 0)
	{
		printk(KERN_INFO "%s: alloc_chrdev_region() failed\n", THIS_MODULE->name);
		return -1;
	}
	major = MAJOR(first);
	minor = MINOR(first);
	printk(KERN_INFO "%s: alloc_chrdev_region() success: device number = %d - %d\n", THIS_MODULE->name, major, minor);
	// create device class
	my_device_class_ptr = class_create(THIS_MODULE, DEV_CLASS_NAME);
	if(my_device_class_ptr == NULL) 
	{
		printk(KERN_INFO "%s: class_create() failed\n", THIS_MODULE->name);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	printk(KERN_INFO "%s: class_create() success: device class pchar_class created.\n", THIS_MODULE->name);
	// create device file
	for(i=0; i<DEVICE_COUNT; i++)
	{
		devno = MKDEV(major, i);
		my_device_ptr = device_create(my_device_class_ptr, NULL, devno, NULL, DEV_NAME"%d", i);
		if(my_device_ptr == NULL)
		{
			printk(KERN_INFO "%s: device_create() failed\n", THIS_MODULE->name);
			for(i=i-1; i>=0; i--)
			{
				devno = MKDEV(major, i);
				device_destroy(my_device_class_ptr, devno);
			}
			class_destroy(my_device_class_ptr);
			unregister_chrdev_region(first, 1);
			return -1;
		}
		printk(KERN_INFO "%s: device_create() success: device pchar_chrdev created.\n", THIS_MODULE->name);
	}
	// init cdev struct & register into kernel
	for(i=0; i<DEVICE_COUNT; i++)
	{
		cdev_init(&devices[i].my_char_dev, &pchar_fops);	
		devno = MKDEV(major, i);
		ret = cdev_add(&devices[i].my_char_dev, devno, 1);
		if(ret < 0)
		{
			printk(KERN_INFO "%s: cdev_add() failed\n", THIS_MODULE->name);
			for(i=i-1; i>=0; i--)
				cdev_del(&devices[i].my_char_dev);
			for(i=DEVICE_COUNT-1; i>=0; i--)
			{
				devno = MKDEV(major, i);
				device_destroy(my_device_class_ptr, devno);
			}
			class_destroy(my_device_class_ptr);
			unregister_chrdev_region(first, 1);
			return -1;	
		}
		printk(KERN_INFO "%s: cdev_add() success.\n", THIS_MODULE->name);
	}

	return 0;
}

static void __exit pchar_exit(void)
{
	int i;
	dev_t devno;
	int major = MAJOR(first);
	printk(KERN_INFO "%s: pchar_exit() called...\n", THIS_MODULE->name);	
	// unregister cdev from kernel
	for(i=DEVICE_COUNT-1; i>=0; i--)
		cdev_del(&devices[i].my_char_dev);
	// destroy device files
	for(i=DEVICE_COUNT-1; i>=0; i--)
	{
		devno = MKDEV(major, i);
		device_destroy(my_device_class_ptr, devno);
	}
	// destroy device class
	class_destroy(my_device_class_ptr);
	// release device number
	unregister_chrdev_region(first, DEVICE_COUNT);
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DESD SunBeam <desd@sunbeaminfo.com>");
MODULE_DESCRIPTION("Psuedo char device driver demo.");

