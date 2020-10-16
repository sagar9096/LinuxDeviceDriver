#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/platform_device.h>
#include "platform.h"

void pcdev_release(struct device *dev)
{
	pr_info("pcdev is released \n");	
}

/* Create 2 platform data for device */
struct pcdev_platform_data pcdev_pdata[2] = {
	[0] = {.size = 512, .perm = RDWR, .serial_number = "PCDEV_XYZ_1234"},
	[1] = {.size = 512, .perm = RDWR, .serial_number = "PCDEV_ABC_9876"}
};

/* Create 2 platform devices */
struct platform_device platform_pcdev_1 ={
	.name = "psuedo-char-device",
	.id = 0,
	.dev = {
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev_2 = {
	.name = "psuedo-char-device",
	.id = 1,
	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release
	}
};


static int __init pcdev_platform_init(void)
{
	platform_device_register(&platform_pcdev_1);
	platform_device_register(&platform_pcdev_2);
	
	pr_info("pcdev platform device module inserted\n");

	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
	pr_info("pcdev platform device module removed\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which init the platform device");
