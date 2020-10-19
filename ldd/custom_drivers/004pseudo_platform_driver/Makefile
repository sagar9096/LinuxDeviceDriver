obj-m := pcd_platform_device_setup.o pcd_platform_driver.o

KERN_DIR=/lib/modules/$(shell uname -r)/build/

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C $(KERN_DIR) M=$(shell pwd) clean

app:
	gcc dev_read.c -o dev
