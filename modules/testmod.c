/*
 * 1. Initial tests for module load/unload
 * 	> lsmod
 *	> /sbin/insmod testmod.ko
 *	> /sbin/modinfo testmod.ko
 *	> lsmod
 *	> dmesg
 *	> /sbin/rmmod testmod
 *	> lsmod
 *	> dmesg
 *
 * 2. Test simple parameters, notifications
 * 	> /sbin/insmod testmod.ko testparm_arg=3
 * 	> cat /sys/module/testmod/parameters/testparm_arg
 * 	> echo 4 > /sys/module/testmod/parameters/testparm_arg
 * 	> cat /sys/module/testmod/parameters/testparm_arg
 * 	> echo 5 > /sys/module/testmod/parameters/testparm_cb
 * 	> dmesg
 *
 * 3. Test allocation of major number
 * 	> cat /proc/devices
 * */

#include <linux/kernel.h> // general module development
#include <linux/init.h> // general module development
#include <linux/module.h> // general module development
#include <linux/kdev_t.h> // device file macros & functions
#include <linux/fs.h> // allocation of character devices

// functions in this file
// module callbacks
static int __init testmod_init(void);
static void __exit testmod_exit(void);

// parameter callbacks
static int testmod_testparm_cb_set(const char *val, const struct kernel_param *kp);

// module parameters
// simple module parameter, just set it when loading module
// can be set later as well, but there will be no notification callback
// i.e. use for stuff where you check the value just before using it
static int testparm_arg = 1; // can set default value here

// module parameter with callback
// note: module_param and module_param_cb both register the module parameter!
// i.e. you don't register the parameter first and then the callback, you do
// both at the same time
static int testparm_cb = 2;
// operations for callback parameter
static const struct kernel_param_ops testparm_cb_ops = {
//	.flags = 0,
	.set = &testmod_testparm_cb_set,
	.get = &param_get_int, // needed in order to be able to cat parameter sysfs file
//	.free = NULL
};

// devices
// dev_t stat_testdev = MKDEV(235, 0); // static device
dev_t dyn_testdev = 0;

// registrations
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sima Baymani");
MODULE_DESCRIPTION("Refreshing kernel module writing");
MODULE_VERSION("0:0.1");
MODULE_PARM_DESC(testparm_arg, "Testmod parameter that takes arguments");
MODULE_PARM_DESC(testparm_cb, "Testmod parameter that has notifications");

module_init(testmod_init);
module_exit(testmod_exit);

// simple parameter, no notification, can be set at any time (module load, run time etc)
module_param(testparm_arg, int, S_IRUSR | S_IWUSR);
// parameter with a setter, can set after module load and get notifications for changes
module_param_cb(testparm_cb, &testparm_cb_ops, &testparm_cb, S_IRUSR | S_IWUSR);


static int __init testmod_init(void) {
	printk(KERN_INFO "[testmod] Starting test module...\n");

	// Static allocation of device number, give your first wanted device and the range
	// if(register_chrdev_region( stat_testdev, 1, "Testmod statically allocated dev")) {
	//	printk(KERN_INFO "[testmod] Error static allocation of major/minor numbers\n");
	//	goto err;
	// }
	// printk(KERN_INFO "[testmod] Successful static allocation of major %d minor %d\n",
	//		MAJOR(stat_testdev), MINOR(stat_testdev));

	// Dynamic allocation of device number
	if(alloc_chrdev_region(&dyn_testdev, 0, 1,
				"Testmod dynamically allocated dev")) {
		printk(KERN_INFO "[testmod] Error dynamic allocation of major/minor numbers\n");
		goto err;
	}
	printk(KERN_INFO "[testmod] Successful dynamic allocation of major %d minor %d\n",
			MAJOR(dyn_testdev), MINOR(dyn_testdev));

	return 0;
err:
	return -1;
}

static void __exit testmod_exit(void)
{
	printk(KERN_INFO "[testmod] Removing test module...\n");
	// unregister_chrdev_region(stat_testdev, 1);
	unregister_chrdev_region(dyn_testdev, 1);
}

static int testmod_testparm_cb_set(const char *val, const struct kernel_param *kp) {
	printk(KERN_INFO "[testmod] Testmod testparm_cb value changed\n");
	printk(KERN_INFO "[testmod] Testmod *val = %s, kp->name = %s &testparm_cb = %p, kp->arg = %p\n", 
			val, kp->name, &testparm_cb, kp->arg);
	// val = string input through sysfs
	// kp->name = name of the kernel parameter to be changed (provided when parameter was registered)
	// kp->arg = pointer to the kernel parameter if simple type (not string or array)
	// utility function that sets our kernel parameter to the input value received
	param_set_int(val, kp);

	return 0;
}
