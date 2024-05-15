#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#define DRIVER_MAJOR 63
static int myDeviceOpen(struct inode *inode, struct file *file)
{
	printk("Device has been opened\n");
	return 0;
}
static int myDeviceClose(struct inode *inode, struct file *file)
{
	printk("Device has been closed\n");
	return 0;
}
static ssize_t myDeviceRead(struct file *file, char __user *buffer,
			    size_t length, loff_t *offset)
{
	printk("Reading from device\n");
	if (length < 22) {
		return -EINVAL;
	}

	int t = copy_to_user(buffer, "Hello from the kernel", 22);
	if (t != 0) {
		printk("Error in copy_to_user\n");
		return -EFAULT;
	}
	return 22;
}
static ssize_t myDeviceWrite(struct file *file, const char __user *buffer,
			     size_t length, loff_t *offset)
{
	char *tmp = kmalloc(length, GFP_KERNEL);
	printk("Writing to device\n");
	if (length < 20) {
		return -EINVAL;
	}

	int t = copy_from_user(tmp, buffer, length);
	if (t != 0) {
		printk("Error in copy_from_user\n");
		return -EFAULT;
	}
	printk("User wrote: %s\n", tmp);
	kfree(tmp);
	return length;
}

static struct file_operations s_myDeviceFops = {
	.owner = THIS_MODULE,
	.open = myDeviceOpen,
	.release = myDeviceClose,
	.read = myDeviceRead,
	.write = myDeviceWrite,
};

static int __init test_init(void)
{
	printk("Hello my module\n");
	register_chrdev(DRIVER_MAJOR, "myDevice", &s_myDeviceFops);
	return 0;
}

static void __exit test_exit(void)
{
	printk("Bye bye my module\n");
	unregister_chrdev(DRIVER_MAJOR, "myDevice");
}

module_init(test_init);
module_exit(test_exit);
MODULE_DESCRIPTION("Test module");
MODULE_LICENSE("GPL");