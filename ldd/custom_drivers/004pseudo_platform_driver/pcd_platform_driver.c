#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>
#include "platform.h"
#include<linux/slab.h>
#include<stddef.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s :"fmt,__func__


#define MAX_DEVICES 	10

/* Device Private data */
struct pcdev_private_data {
	struct pcdev_platform_data pdata;
	char *buffer;
	dev_t dev_num;
	struct cdev cdev;
};

/* Driver private data structure */
struct pcdrv_private_data {
	int total_devices;
	dev_t device_base_num;
	struct class *pcd_class;
	struct device *pcd_device;
};

/* Driver private data variable instance */
struct pcdrv_private_data pcdrv_data;

/* lseek function */
loff_t pcd_lseek(struct file *pfile, loff_t offset, int whence)
{
	return 0;
}

/* read function */
ssize_t pcd_read(struct file *pfile, char __user *buffer, size_t count, loff_t *f_pos)
{
	return 0;
}

/* write function */
ssize_t pcd_write(struct file *pfile, const char __user *buffer, size_t count, loff_t *f_pos)
{
	return 0;
}

int check_permission(int device_perm, int acc_mode)
{
	/* Ensure Read Write access */
	if(device_perm == RDWR)
		return 0;

	/* Ensure Read Only Access */
	if((device_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;

	/* Ensure WriteONly access */
	if((device_perm == WRONLY) &&( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;


	return -EPERM;
}
/* open function */
int pcd_open(struct inode *pinode, struct file *pfile)
{

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

/* probe function gets called when matching name device is loaded */
int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;

	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data *pdata;
	/* Get the platform data */
	pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
	if(!pdata){
		pr_err("No platform data available \n");
		ret = -EINVAL;
		goto out;
		}
	
	/* Dynamically allocate the memory for device private data */	
	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if(!dev_data){
			pr_err("Memory cannot allocated\n");
			ret = -ENOMEM;
			goto out;
		}
	
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;
	
	pr_info("size : %d\n",pdata->size);
	pr_info("per :%d\n ",pdata->perm);
	pr_info("serial_number:%s\n",pdata->serial_number);
	

	/* Dynamically allocate the memory for the device buffer using size information from the platform data */
	dev_data->buffer = kzalloc(dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data->buffer){
		pr_info("Memory allocation failed\n");
		ret = -ENOMEM;
		goto dev_data_free;
		}

	/* Get the device number */
	dev_data->dev_num = pcdrv_data.device_base_num + pdev->id;

	/* Do cdev init and cdev add */
	cdev_init(&dev_data->cdev,&pcd_fops);
	
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto buffer_free;
		}
	/* Crete device file for the detected device */
	pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class,NULL,dev_data->dev_num,NULL,"pcdev:%d",pdev->id);
	if(IS_ERR(pcdrv_data.pcd_device)){
	
		pr_err("device file creation was failed\n");
		ret = PTR_ERR(pcdrv_data.pcd_device);
		goto cdev_del;	
		}

	/* Save device private data pointer into platform device structure */
	dev_set_drvdata(&pdev->dev,dev_data);

	/* Error Handling */
	
	pr_info("probe was successful\n");
	return 0;


cdev_del:
	cdev_del(&dev_data->cdev);
buffer_free:
	kzfree(dev_data->buffer);
dev_data_free :
	kzfree(dev_data);	

out :	pr_info("Device probe function failed\n");
	return ret;
}

/* remove function gets called when matching name device is unloaded */
int pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
	
	/* destroy the device created by the device_create() */
	device_destroy(pcdrv_data.pcd_class,dev_data->dev_num);

	/* Remove cdev entry from the sys */
	cdev_del(&dev_data->cdev);

	/* free the memory held by the device */
	kfree(dev_data->buffer);
	kfree(dev_data);
	pr_info("pseudo-char-device removed\n");
	return 0;
}

/* Init driver function probe and remove */
struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver = {
		.name = "pcd-char-device"	
	}
};




static int pcd_platform_driver_init(void)
{
	int ret;

	/* 1. Allocate the devicenumber */
	ret = alloc_chrdev_region(&pcdrv_data.device_base_num,0,MAX_DEVICES,"pcdevs");
	if(ret < 0){
		pr_err("device number allocation failed\n");
		return ret;
		}

	/* 2. Create the class in /sys/class */
	pcdrv_data.pcd_class = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.pcd_class)){
			pr_err("Class creation is failed\n");
			ret = PTR_ERR(pcdrv_data.pcd_class);	
			unregister_chrdev_region(pcdrv_data.device_base_num,MAX_DEVICES);
			return ret;
			}
	/* 3.Register the platform driver */
	
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd_platform_drive is loaded\n");
	
	return 0;
}

static void pcd_platform_driver_exit(void)
{
	/* Remove the platform driver */
	platform_driver_unregister(&pcd_platform_driver);	
	
	/* Destroy the class */
	class_destroy(pcdrv_data.pcd_class);
	
	/* Free the device number */
	unregister_chrdev_region(pcdrv_data.device_base_num,MAX_DEVICES);
	pr_info("pcd_platform_driver is unloaded\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar");
MODULE_DESCRIPTION("pseudo char driver for multiple device");


