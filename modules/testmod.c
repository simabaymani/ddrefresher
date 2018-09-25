/*
 * 1. Test by
 * 	> lsmod
 *	> /sbin/insmod testmod.ko
 *	> /sbin/modinfo testmod.ko
 *	> lsmod
 *	> dmesg
 *	> /sbin/rmmod testmod
 *	> lsmod
 *	> dmesg
 * */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// functions in this file
// module callbacks
static int __init testmod_init(void);
static void __exit testmod_exit(void);

// registrations
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sima Baymani");
MODULE_DESCRIPTION("Refreshing kernel module writing");
MODULE_VERSION("0:0.1");

module_init(testmod_init);
module_exit(testmod_exit);

static int __init testmod_init(void) {
	printk(KERN_INFO "[testmod] Starting test module...\n");
	return 0;
}

static void __exit testmod_exit(void)
{
	printk(KERN_INFO "[testmod] Removing test module...\n");
}
