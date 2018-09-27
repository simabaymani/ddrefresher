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
 *
 * 4. Test automatic creation of device node (udev)
 * 	> ls -l /dev
 *
 * 5. Test file operation callbacks
 * 	> echo 1 > /dev/dyn_testdev_dev
 * 	> dmesg
 * 	> cat /dev/dyn_testdev_dev
 * 	> dmesg
 * */

#include <linux/kernel.h> // general module development
#include <linux/init.h> // general module development
#include <linux/module.h> // general module development
#include <linux/kdev_t.h> // device file macros & functions
#include <linux/fs.h> // allocation of character devices
#include <linux/device.h> // creation of device nodes
#include <linux/cdev.h> // cdev utils

// functions in this file
// module callbacks
static int __init testmod_init(void);
static void __exit testmod_exit(void);

// parameter callbacks
static int testmod_testparm_cb_set(const char *val, const struct kernel_param *kp);

// fops callbacks
static int testmod_open(struct inode * i, struct file * f);
static int testmod_release(struct inode *i, struct file * f);
static ssize_t testmod_read(struct file * f, char __user * buf, size_t bytes, loff_t * offset);
static ssize_t testmod_write(struct file * f, const char __user * buf, size_t bytes, loff_t * offset);

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
static dev_t dyn_testdev = 0;
static struct class * dyn_testdev_class = 0;
static struct device * dyn_testdev_dev = 0;
static struct cdev cdev;
static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.read           = testmod_read,
	.write          = testmod_write,
	.open           = testmod_open,
	.release        = testmod_release,
};

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

	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;

	if(cdev_add(&cdev, dyn_testdev, 1)) {
		printk(KERN_INFO "[testmod] Error when adding cdev\n");
		unregister_chrdev_region(dyn_testdev, 1);
		goto err;
	}

	// we have the device number, let's create the device node
	dyn_testdev_class = class_create (THIS_MODULE, "dyn_testdev_class");
	if(!dyn_testdev_class) {
		printk(KERN_INFO "[testmod] Error on device class creation\n");
		cdev_del(&cdev);
		unregister_chrdev_region(dyn_testdev, 1);
		goto err;

	}

	dyn_testdev_dev = device_create (dyn_testdev_class, NULL, dyn_testdev, NULL, "dyn_testdev_dev");
	if(!dyn_testdev_dev) {
		printk(KERN_INFO "[testmod] Error on device creation\n");
		class_destroy(dyn_testdev_class);
		cdev_del(&cdev);
		unregister_chrdev_region(dyn_testdev, 1);
		goto err;

	}

	return 0;
err:
	return -1;
}

static void __exit testmod_exit(void)
{
	printk(KERN_INFO "[testmod] Removing test module...\n");

	// free everything in reverse order
	// note that we don't use the struct device here!
	device_destroy(dyn_testdev_class, dyn_testdev);
	class_destroy(dyn_testdev_class);

	cdev_del(&cdev);
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

static int testmod_open(struct inode * i, struct file * f) {
	printk(KERN_INFO "[testmod] fops OPEN \n");
	return 0;
}

static int testmod_release(struct inode *i, struct file * f) {
	printk(KERN_INFO "[testmod] fops RELEASE \n");
	return 0;
}

static ssize_t testmod_read(struct file * f, char __user * buf, size_t bytes, loff_t * offset) {
	printk(KERN_INFO "[testmod] fops READ \n");
	return 0; // if I return positive value here, the call does not return. Why?
}

static ssize_t testmod_write(struct file * f, const char __user * buf, size_t bytes, loff_t * offset) {
	printk(KERN_INFO "[testmod] fops WRITE \n");
	return bytes;
}
