#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include "test_module.h"
#define DRIVER_MAJOR 63
#define FIFO_SIZE 1024
//#define min(a, b) ((a) < (b) ? (a) : (b))
struct mutex my_mutex;
char FIFO[FIFO_SIZE];
int charNum = 0;
enum testdevice_state state = TESTDEVICE_STATE_ENABLE;
static int myDeviceOpen(struct inode *inode, struct file *file)
{
	if (!mutex_trylock(&my_mutex)) {
		printk("Device is busy\n");
		return -EBUSY;
	}
	printk("Device has been opened\n");

	return 0;
}
static int myDeviceClose(struct inode *inode, struct file *file)
{
	mutex_unlock(&my_mutex);
	printk("Device has been closed\n");

	return 0;
}
static ssize_t myDeviceRead(struct file *file, char __user *buffer,
			    size_t length, loff_t *offset)
{
	printk("Reading from device\n");

	if (state == TESTDEVICE_STATE_DISABLE) {
		printk("Device is disabled\n");
		return -EIO;
	}

	if (length == 0) {
		return 0;
	} else if (charNum == 0) {
		return 0;
	}
	unsigned long int Num2Read = umin(length, charNum);
	if (Num2Read == 0) {
		return 0;
	} else if (Num2Read > FIFO_SIZE) {
		Num2Read = FIFO_SIZE;
	}
	char *kbuf = kmalloc(Num2Read, GFP_KERNEL);
	if (kbuf == NULL) {
		printk("Error in kmalloc\n");
		return -ENOMEM;
	}
	for (int i = 0; i < Num2Read; i++) {
		kbuf[i] = FIFO[i];
	}
	//kbuf[Num2Read] = '\0';
	int t = copy_to_user(buffer, kbuf, Num2Read);
	if (t != 0) {
		printk("Error in copy_to_user\n");
		Num2Read = Num2Read - t;
	}
	for (int i = 0; i < charNum - Num2Read; i++) {
		if (i + Num2Read >= FIFO_SIZE) {
			break;
		}
		FIFO[i] = FIFO[i + Num2Read];
	}
	charNum = charNum - Num2Read;
	return Num2Read;
}
static ssize_t myDeviceWrite(struct file *file, const char __user *buffer,
			     size_t length, loff_t *offset)
{
	if (state == TESTDEVICE_STATE_DISABLE) {
		printk("Device is disabled\n");
		return -EIO;
	}
	char *kbuf = kmalloc(length + 1, GFP_KERNEL);
	if (kbuf == NULL) {
		printk("Error in kmalloc\n");
		return -ENOMEM;
	}
	int t = copy_from_user(kbuf, buffer, (unsigned long int)length);
	if (t != 0) {
		printk("Error in copy_from_user\n");
		return -EFAULT;
	}
	kbuf[length - t] = '\0';
	if (charNum + length + 1 > FIFO_SIZE) {
		printk("FIFO is full\n");
		return -ENOSPC;
	}
	for (int i = 0; i < length; i++) {
		FIFO[charNum + i + 1] = kbuf[i];
	}
	charNum = charNum + length;
	FIFO[charNum + 1] = '\0';
	return length - t;
}

/* ioctlテスト用に値を保持する変数 */
static struct mydevice_values stored_values;

/* ioctl時に呼ばれる関数 */
static long mydevice_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	printk("mydevice_ioctl\n");

	switch (cmd) {
	case MYDEVICE_SET_VALUES:
		printk("MYDEVICE_SET_VALUES\n");
		if (copy_from_user(&stored_values, (void __user *)arg,
				   sizeof(stored_values))) {
			return -EFAULT;
		}
		break;
	case MYDEVICE_GET_VALUES:
		printk("MYDEVICE_GET_VALUES\n");
		if (copy_to_user((void __user *)arg, &stored_values,
				 sizeof(stored_values))) {
			return -EFAULT;
		}
		break;

	case TESTDEVICE_STATE_WRITE:
		printk("TESTDEVICE_STATE_WRITE\n");
		if (copy_from_user(&state, (void __user *)arg, sizeof(state))) {
			return -EFAULT;
		}

		break;
	case TESTDEVICE_STATE_READ:
		printk("TESTDEVICE_STATE_READ\n");
		if (copy_to_user((void __user *)arg, &state, sizeof(state))) {
			return -EFAULT;
		}
		break;
	default:
		printk(KERN_WARNING "unsupported command %d\n", cmd);
		return -EFAULT;
	}
	return 0;
}

static struct file_operations s_myDeviceFops = {
	.owner = THIS_MODULE,
	.open = myDeviceOpen,
	.release = myDeviceClose,
	.read = myDeviceRead,
	.write = myDeviceWrite,
	.unlocked_ioctl = mydevice_ioctl,
	.compat_ioctl = mydevice_ioctl, // for 32-bit App
};

static int __init test_init(void)
{
	printk("Hello my module\n");
	FIFO[0] = '\0';
	FIFO[1023] = '\0';
	mutex_init(&my_mutex);
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