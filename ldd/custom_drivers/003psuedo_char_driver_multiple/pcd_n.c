#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>


#undef pr_fmt
#define pr_fmt(fmt) "%s :"fmt,__func__

#define NO_OF_DEVICES 	 4

#define MAX_SIZE_PCDEV1	 512  // Max size is 512 bytes
#define MAX_SIZE_PCDEV2	 1024 // Max size is 1024 bytes
#define MAX_SIZE_PCDEV3	 512  // Max size is 512 bytes
#define MAX_SIZE_PCDEV4	 1024 // Max size is 1024 bytes

/* Device Memory */
char device_buffer_pcdev1[MAX_SIZE_PCDEV1];
char device_buffer_pcdev2[MAX_SIZE_PCDEV2];
char device_buffer_pcdev3[MAX_SIZE_PCDEV3];
char device_buffer_pcdev4[MAX_SIZE_PCDEV4];

/* Device private data structure */
struct pcdev_private_data
{
	char *buffer;
	uint32_t size;
	const char *serial_number;
	int32_t perm;
	struct cdev cdev;
};

/* Driver private data structure */
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_number;
	/* Holds base minor number */
	uint32_t minornumber;
	/* Number of devices */
	uint32_t count;
	struct class *pcd_class;
	struct device *pcd_device;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

#define RDONLY  0x01
#define WRONLY  0x10
#define RDWR	0x11
struct pcdrv_private_data pcdrv_data = 
{
	.total_devices = NO_OF_DEVICES,
	.minornumber = 0,
	.count = 4,
	.pcdev_data = {
	
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = MAX_SIZE_PCDEV1,
			.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY /* READONLY */
		      },
		[1] = {
			.buffer = device_buffer_pcdev2,
			.size = MAX_SIZE_PCDEV2,
			.serial_number = "PCDEV2XYZ123",
			.perm = WRONLY /* WRITEONLY */
		      },
		[2] = {
			.buffer = device_buffer_pcdev3,
			.size = MAX_SIZE_PCDEV3,
			.serial_number = "PCDEV3XYZ123",
			.perm = RDWR /* READWRITE */
		      },
		[3] = {
			.buffer = device_buffer_pcdev4,
			.size = MAX_SIZE_PCDEV4,
			.serial_number = "PCDEV4XYZ123",
			.perm = RDWR /* READWRITE */
		      }
		   }
};


/* Name of the driver */
static char *pcdrv_name = "Pschedo Char Driver\n";



/* lseek function */
loff_t pcd_lseek(struct file *pfile, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)pfile->private_data;
	int max_size = pcdev_data->size;
	loff_t temp;

	pr_info("lseek is requested\n");
	pr_info("Current f_pos:%lld\n",pfile->f_pos);
	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			pfile->f_pos = offset;
		break;
		case SEEK_CUR:
			temp = pfile->f_pos + offset;
			if((temp > max_size) || (temp <0))
				return -EINVAL;
			pfile->f_pos = temp;
		break;
		case SEEK_END:
			temp = pfile->f_pos + offset;
			if((temp > max_size) || (temp < 0 ))
				return -EINVAL;
			pfile->f_pos = temp;
		break;
		default:
			return -EINVAL;
	}

	pr_info("updated file pos:%lld\n", pfile->f_pos);
	/* Return updated file position */
	return pfile->f_pos;
}

/* read function */
ssize_t pcd_read(struct file *pfile, char __user *buffer, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*) pfile->private_data;
	int max_size = pcdev_data->size;
	pr_info("Requested read %zu bytes\n",count);
	pr_info("f_pos:%lld\n",*f_pos);

	// Adjust the count	
	if((*f_pos + count) > max_size){
		count = max_size - *f_pos;
	}
	
	// copy to user
	if(copy_to_user(buffer,(pcdev_data->buffer + *f_pos),count)){
		return -EFAULT;
	}
	
	// Update current file position 
	*f_pos = *f_pos + count;
	
	//
	pr_info("Successfully Read Bytes:%zu\n",count);
	pr_info("Updated f_pos:%lld\n",*f_pos);

	// Return successfully read counts
	return count;
}

/* write function */
ssize_t pcd_write(struct file *pfile, const char __user *buffer, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*) pfile->private_data;
	int max_size = pcdev_data->size;

	pr_info("Write is requested by :%zu\n",count);
	pr_info("f_pos:%lld\n",*f_pos);
	/* Adjust the count */
	if((*f_pos + count) > max_size){
		count = max_size - *f_pos;
	}
	/* Check if no memory left */
	if(!count)
		return -ENOMEM;
	
	/* Copy_from_user */
	if(copy_from_user((pcdev_data->buffer + *f_pos),buffer,count)){
		return -EFAULT;
	}

	/* update the f_pos */
	*f_pos += count;

	pr_info("Successfully written bytes:%zu\n",count);
	pr_info("updated f_pos:%lld\n",*f_pos);
	/* Return successfully written bytes */
	return count;
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
	int ret;
	int minor_n;
	struct pcdev_private_data *pcdev_data;
	/* Get the minor number from inode structure */
	minor_n = MINOR(pinode->i_rdev);
	pr_info("minor_n:%d\n",minor_n);
	
	/*Get the device private data structure */
	pcdev_data = container_of(pinode->i_cdev,struct pcdev_private_data,cdev);

	/*store the pcdev_data to private data section into file structure to get access all functions */
	pfile->private_data = pcdev_data;

	ret = check_permission(pcdev_data->perm,pfile->f_mode);

	(!ret)?pr_info("Open was succesful\n"):pr_info("Open was failed\n");
	return ret;
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
	int ret;
	int i;
	
	/*  Allocate the device number Dynamically */
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0/* Base Minor */,NO_OF_DEVICES, pcdrv_name);
	if(ret < 0){
		pr_err("device number registration failed\n");
		goto out;
		}
	/* Create the /sys/class entry */
	pcdrv_data.pcd_class = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.pcd_class)){
		pr_err("class create is failed\n");
		ret = PTR_ERR(pcdrv_data.pcd_class);
		goto unreg_chrdev;
		}

	for(i = 0; i < NO_OF_DEVICES; i++){
		pr_info("major:minor = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));
	
		/*  Initialize the cdev variable with file operations and module owner */
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
	
		/*  Register char driver with the VFS */
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);
		if(ret < 0){
			pr_err("cdev add failed\n");
			goto cdev_del;
			}


		/*  Create the device file */
		pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, pcdrv_data.device_number+i, NULL, "pcd%d",i);
		if(IS_ERR(pcdrv_data.pcd_device)){
			pr_err("Device creation is failed\n");
			ret = PTR_ERR(pcdrv_data.pcd_device);
			goto class_del;
			}
	} 
	pr_info("pcd module init is successful\n");
	return 0;

cdev_del:
class_del:
	for(; i>=0; i--){
		device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
		}
	class_destroy(pcdrv_data.pcd_class);
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number,pcdrv_data.count);

out: 
	pr_info("pcd module initialization is failed\n");
	return ret;
}

static void exit_hello_world(void)
{
	int i;
	pr_info("Removing the module\n");
		
	for(i=0; i<NO_OF_DEVICES; i++){

	/* Destroy the device file */
	device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number+i);
	
	/* Destroy the Cdev structure */
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}

	/* Destroy the class */
	class_destroy(pcdrv_data.pcd_class);

	/* Destroy the devicenumber */
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);

	pr_info("Good Bye\n");
}

module_init(init_hello_world);
module_exit(exit_hello_world);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar");
MODULE_DESCRIPTION("pseudo char driver for multiple device");


