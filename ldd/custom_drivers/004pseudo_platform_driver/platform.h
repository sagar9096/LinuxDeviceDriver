// structure for platform device data

struct pcdev_platform_data
{
	int size;/*size of memory */
	int perm;/* access permission */
	const char *serial_number; /* serial number for the device */

};

#undef pr_fmt
#define pr_fmt(fmt) "%s :"fmt,__func__

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11
