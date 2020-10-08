#include<linux/module.h>


static int __init init_hello_world(void)
{

	pr_info("In module:%s\n",__func__);
	return 0;
}

static void __exit exit_hello_world(void)
{

	pr_info("Good Bye\n");
}

module_init(init_hello_world);
module_exit(exit_hello_world);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar");



