#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :"fmt,__func__

/* Holds the device number */
static dev_t devicenumber;

/* Holds base minor number */
static uint32_t minornumber = 0;

/* Number of devices */
static uint32_t count = 1;

/* Name of the driver */
static char *drivername = "Pschedo Char Driver\n";

/* Cdev variable */
static struct cdev pcd_cdev;

static struct class *pcd_class;
static struct device *pcd_device;

/* lseek function */
loff_t pcd_lseek(struct file *pfile, loff_t offset, int whence)
{
	pr_info("lseek is requested\n");
	return 0;
}

/* read function */
ssize_t pcd_read(struct file *pfile, char __user *buffer, size_t count, loff_t *offset)
{
	pr_info("Read is requested by :%zu\n",count);
	return 0;
}

/* write function */
ssize_t pcd_write(struct file *pfile, const char __user *buffer, size_t count, loff_t *offset)
{
	pr_info("Write is requested by :%zu\n",count);
	return 0;
}

/* open function */
int pcd_open(struct inode *pfile, struct file *offset)
{
	pr_info("Open was succesful\n");
	return 0;
}

/* release function */
int pcd_release(struct inode *pfile, struct file *offset)
{
	pr_info("Release is requested\n");
	return 0;
}

/* File Operations variable */
static struct file_operations pcd_fops = 
{
	.open = pcd_open,
	.llseek = pcd_lseek,
	.write = pcd_write,
	.read = pcd_read,
	.release = pcd_release

};
static int __init init_hello_world(void)
{

	pr_info("Ini module:%s\n",__func__);

	/* 1. Allocate the device number Dynamically */
	alloc_chrdev_region(&devicenumber, minornumber, count, drivername);
	
	pr_info("major:minor = %d:%d\n",MAJOR(devicenumber),MINOR(devicenumber));

	/* 2. Initialize the cdev variable with file operations and module owner */
	cdev_init(&pcd_cdev, &pcd_fops);
	
	/* 3. Register char driver with the VFS */
	pcd_cdev.owner = THIS_MODULE;
	cdev_add(&pcd_cdev,devicenumber,count);
	
	/* 4. Create the /sys/class entry */
	pcd_class = class_create(THIS_MODULE,"pcd_class");

	/* 5. Create the device file */
	pcd_device = device_create(pcd_class, NULL, devicenumber, NULL, "pcd");

	pr_info("pcd module init is successful\n");
	return 0;
}

static void __exit exit_hello_world(void)
{
	pr_info("Removing the module\n");
	
	/* Destroy the device file */
	device_destroy(pcd_class,devicenumber);
	
	/* Destroy the class */
	class_destroy(pcd_class);

	/* Destroy the Cdev structure */
	cdev_del(&pcd_cdev);
	
	/* Destroy the devicenumber */
	unregister_chrdev_region(devicenumber,count);

	pr_info("Good Bye\n");
}

module_init(init_hello_world);
module_exit(exit_hello_world);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar");



