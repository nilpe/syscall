#include <linux/syscalls.h>

SYSCALL_DEFINE1(new_syscall, int, x)
{
	//printk("new_syscall added!!");
	return x;
}

SYSCALL_DEFINE2(hash_char, const char __user *, filename, long int __user *,
		value)
{
	//printk("test");
	//int ok = access_ok(VERIFY_READ, filename, 1);
	int rec_max = 255;
	char *fn = kmalloc(rec_max, GFP_KERNEL);
	int i;
	for (i = 0; i < rec_max; i++) {
		int ok = copy_from_user(&fn[i], &filename[i], 1);
		if (ok != 0) {
			//printk("copy_from_user failed");
			return EFAULT;
		}
		if (fn[i] == '\0') {
			break;
		}
	}
	long int hash = 0xcbf29ce484222325;
	for (int j = 0; j < i; j++) {
		hash = hash ^ fn[j];
		hash = hash * 0x100000001b3;
	}
	int ok = copy_to_user(value, &hash, sizeof(long int));
	if (ok != 0) {
		//printk("copy_to_user failed");
		return EFAULT;
	}

	return 0;
}
