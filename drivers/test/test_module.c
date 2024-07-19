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
static struct mutex devMutex;
static struct mutex rwMutex;
static char FIFO[FIFO_SIZE + 1];
static int charNum = 0;
static enum testdevice_state state = TESTDEVICE_STATE_ENABLE;
static int myDevice_open(struct inode *inode, struct file *file)
{
	if (!mutex_trylock(&devMutex)) {
		pr_err("Device is busy\n");
		return -EBUSY;
	}
	pr_info("Device has been opened\n");

	return 0;
}
static int myDevice_close(struct inode *inode, struct file *file)
{
	mutex_unlock(&devMutex);
	pr_info("Device has been closed\n");

	return 0;
}
static ssize_t mydevice_read(struct file *file, char __user *buffer,
			     size_t length, loff_t *offset)
{
	pr_info("Reading from device\n");
	pr_debug("charNum = %d\n", charNum);
	if (state == TESTDEVICE_STATE_DISABLE) {
		pr_err("Device is disabled\n");

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
		pr_err("Error in kmalloc\n");
		return -ENOMEM;
	}
	mutex_lock(&rwMutex);

	for (int i = 0; i < Num2Read; i++) {
		kbuf[i] = FIFO[i];
	}
	int t = copy_to_user(buffer, kbuf, Num2Read);
	if (t != 0) {
		pr_err("Error in copy_to_user\n");
		Num2Read = Num2Read - t;
	}
	for (int i = 0; i < charNum - Num2Read; i++) {
		if (i + Num2Read >= FIFO_SIZE) {
			break;
		}
		FIFO[i] = FIFO[i + Num2Read];
	}
	charNum = charNum - Num2Read;
	mutex_unlock(&rwMutex);

	kfree(kbuf);
	return Num2Read;
}
static ssize_t mydevice_write(struct file *file, const char __user *buffer,
			      size_t length, loff_t *offset)
{
	length = umin(length, FIFO_SIZE - charNum);
	if (state == TESTDEVICE_STATE_DISABLE) {
		pr_err("Device is disabled\n");
		return -EIO;
	}
	char *kbuf = kmalloc(length + 1, GFP_KERNEL);
	if (kbuf == NULL) {
		pr_err("Error in kmalloc\n");
		return -ENOMEM;
	}
	int t = copy_from_user(kbuf, buffer, (unsigned long int)length);
	if (t != 0) {
		pr_err("Error in copy_from_user\n");
		kfree(kbuf);
		return -EFAULT;
	}
	length = length - t;
	kbuf[length] = '\0';

	pr_info("Writing to device: %s\n", kbuf);
	mutex_lock(&rwMutex);

	if (charNum + length + 1 > FIFO_SIZE) {
		pr_err("FIFO is full\n");
		kfree(kbuf);
		mutex_unlock(&rwMutex);
		return -ENOSPC;
	}
	for (int i = 0; i < length; i++) {
		FIFO[charNum + i] = kbuf[i];
	}
	charNum = charNum + length;
	FIFO[charNum] = '\0';
	mutex_unlock(&rwMutex);

	pr_info("FIFO: %s\n", FIFO);
	kfree(kbuf);
	return length - t;
}

/* ioctlテスト用に値を保持する変数 */
static struct mydevice_values stored_values;

/* ioctl時に呼ばれる関数 */
static long mydevice_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	pr_info("mydevice_ioctl\n");

	switch (cmd) {
	case MYDEVICE_SET_VALUES:
		pr_info("MYDEVICE_SET_VALUES\n");
		if (copy_from_user(&stored_values, (void __user *)arg,
				   sizeof(stored_values))) {
			return -EFAULT;
		}
		break;
	case MYDEVICE_GET_VALUES:
		pr_info("MYDEVICE_GET_VALUES\n");
		if (copy_to_user((void __user *)arg, &stored_values,
				 sizeof(stored_values))) {
			return -EFAULT;
		}
		break;

	case TESTDEVICE_STATE_WRITE:
		pr_info("TESTDEVICE_STATE_WRITE\n");
		if (copy_from_user(&state, (void __user *)arg, sizeof(state))) {
			return -EFAULT;
		}

		break;
	case TESTDEVICE_STATE_READ:
		pr_info("TESTDEVICE_STATE_READ\n");
		if (copy_to_user((void __user *)arg, &state, sizeof(state))) {
			return -EFAULT;
		}
		break;
	case TESTDEVICE_FIFO_CLEAN:
		pr_info("TESTDEVICE_FIFO_CLEAN\n");
		charNum = 0;
		FIFO[0] = '\0';
		break;
	default:
		pr_err("unsupported command %d\n", cmd);
		return -EFAULT;
	}
	return 0;
}

static struct file_operations s_myDeviceFops = {
	.owner = THIS_MODULE,
	.open = myDevice_open,
	.release = myDevice_close,
	.read = mydevice_read,
	.write = mydevice_write,
	.unlocked_ioctl = mydevice_ioctl,
	.compat_ioctl = mydevice_ioctl, // for 32-bit App
};

static int __init test_init(void)
{
	pr_info("Hello my module\n");
	FIFO[0] = '\0';
	FIFO[FIFO_SIZE - 1] = '\0';
	mutex_init(&devMutex);
	mutex_init(&rwMutex);
	int ok = register_chrdev(DRIVER_MAJOR, "myDevice", &s_myDeviceFops);
	if (ok < 0) {
		pr_err("register_chrdev failed\n");
		mutex_destroy(&devMutex);
		mutex_destroy(&rwMutex);
	}
	return ok;
}

static void __exit test_exit(void)
{
	pr_info("Bye bye my module\n");
	unregister_chrdev(DRIVER_MAJOR, "myDevice");
}

module_init(test_init);
module_exit(test_exit);
MODULE_DESCRIPTION("Test module");
MODULE_LICENSE("GPL");