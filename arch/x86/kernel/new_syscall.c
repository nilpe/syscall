#include <linux/syscalls.h>

SYSCALL_DEFINE1(new_syscall, int, x)
{
	//printk("new_syscall added!!");
	return x;
}

SYSCALL_DEFINE3(hash_char, const char __user *, filename, unsigned long int,
		len, unsigned long int __user *, value)
{
	unsigned int rec_max = 255 * sizeof(char);
	char *fn = kmalloc(rec_max, GFP_KERNEL);
	if (fn == NULL) {
		//printk("kcalloc failed");
		return -ENOMEM;
	}
	int willCopyByte = (rec_max < len) ? rec_max : len * sizeof(char);
	unsigned long ok = copy_from_user(fn, filename, willCopyByte);
	if (ok != 0) {
		//printk("copy_from_user failed");
		//printk("copy_from_user failed, %ld\n", ok);
		kfree(fn);
		return -EFAULT;
	}

	long int hash = 0xcbf29ce484222325;
	for (int j = 0; j < willCopyByte; j++) {
		hash = hash ^ fn[j];
		hash = hash * 0x100000001b3;
	}
	ok = copy_to_user(value, &hash, sizeof(long int));
	if (ok != 0) {
		//printk("copy_to_user failed");
		kfree(fn);
		return -EFAULT;
	}

	return 0;
}
