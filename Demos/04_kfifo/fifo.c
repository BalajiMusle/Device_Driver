#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kfifo.h>

struct kfifo my_fifo;

static int __init fifo_init(void)
{
	char buf1[] = "HELLO DESD!!";
	char buf2[] = "GOD BLESS YOU!!";
	int ret;
	ret = kfifo_alloc(&my_fifo, 1024, GFP_KERNEL);
	if(ret)
	{
		printk(KERN_INFO "%s: kfifo_alloc() failed.\n", THIS_MODULE->name);
		return -ENOMEM;
	}
	
	printk(KERN_INFO "%s: my_fifo size=%d, len=%d, avail=%d.\n", THIS_MODULE->name, kfifo_size(&my_fifo), kfifo_len(&my_fifo), kfifo_avail(&my_fifo));

	printk(KERN_INFO "%s: inserting %s into my_fifo.\n", THIS_MODULE->name, buf1);
	ret = kfifo_in(&my_fifo, buf1, strlen(buf1));
	printk(KERN_INFO "%s: inserted %d elements.\n", THIS_MODULE->name, ret);
	printk(KERN_INFO "%s: my_fifo size=%d, len=%d, avail=%d.\n", THIS_MODULE->name, kfifo_size(&my_fifo), kfifo_len(&my_fifo), kfifo_avail(&my_fifo));
	
	printk(KERN_INFO "%s: inserting %s into my_fifo.\n", THIS_MODULE->name, buf2);
	ret = kfifo_in(&my_fifo, buf2, strlen(buf2));
	printk(KERN_INFO "%s: inserted %d elements.\n", THIS_MODULE->name, ret);
	printk(KERN_INFO "%s: my_fifo size=%d, len=%d, avail=%d.\n", THIS_MODULE->name, kfifo_size(&my_fifo), kfifo_len(&my_fifo), kfifo_avail(&my_fifo));
	
	return 0;
}

static void __exit fifo_exit(void)
{
	int ret;
	char buf[8] = "";
	while(!kfifo_is_empty(&my_fifo))
	{
		ret = kfifo_out(&my_fifo, buf, sizeof(buf)-1);
		buf[ret] = '\0';
		printk(KERN_INFO "%s: removed %d elements --> %s\n", THIS_MODULE->name, ret, buf);
		printk(KERN_INFO "%s: my_fifo size=%d, len=%d, avail=%d.\n", THIS_MODULE->name, kfifo_size(&my_fifo), kfifo_len(&my_fifo), kfifo_avail(&my_fifo));
	}

	kfifo_free(&my_fifo);
}

module_init(fifo_init);
module_exit(fifo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DESD SUNBEAM <desd@sunbeaminfo.com>");
MODULE_DESCRIPTION("Kfifo Demo");


