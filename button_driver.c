#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/watchdog.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

static const char gpio_name[] = "s3c24x0_gpio";

volatile unsigned long* gpfcon = NULL;
volatile unsigned long* gpfdat = NULL;
volatile unsigned long* gpgcon = NULL;
volatile unsigned long* gpgdat = NULL;

#define GPFCON_PHY  0x56000050
#define GPFDAT_PHY  0x56000054
#define GPGCON_PHY  0x56000060
#define GPGDAT_PHY  0x56000064

#define BIT_0 0
#define BIT_2 2
#define BIT_3 3
#define BIT_4 4
#define BIT_5 5
#define BIT_11 11

static int button_open(struct inode *inode, struct file *filp)
{   
	
	*gpfcon &= ~((0x3 << (BIT_0*2)) | (0x3 << (BIT_2*2)));
	*gpgcon &= ~((0x3 << (BIT_3*2)) | (0x3 << (BIT_11*2)));

   	return 0;
}               
                
static int button_release(struct inode *inode, struct file *filp)
{    
	return 0;
} 

static ssize_t button_read (struct file *file, char *buf, size_t len, loff_t *pos)
{                               
	unsigned char key_val[4];
	unsigned long reg_val;

	if(len > 4)
		return -EINVAL;
	
	reg_val = *gpfdat;
	key_val[0] = reg_val & (0x1 << BIT_0) ? 1 : 0;
	key_val[1] = reg_val & (0x1 << BIT_2) ? 1 : 0;

	reg_val = *gpgdat;
	key_val[2] = reg_val & (0x1 << BIT_3) ? 1 : 0;
	key_val[3] = reg_val & (0x1 << BIT_11) ? 1 : 0;

	copy_to_user(buf, key_val, len);

	return len;             
}  

  
  
static const struct file_operations button_fops = {
	.owner = THIS_MODULE,
	.open  = button_open,
	.release = button_release,
	.read = button_read
};


static struct class* button_cls = NULL;

int major = 0;
int  devid = 0;


struct cdev button_dev;

static int __init button_driver_init(void)
{
	
#if 0

	res = register_chrdev(0, gpio_name, &gpio_driver_fops);
    if(res < 0) {
    	printk(KERN_ERR "i2c: couldn't get a major number.\n");
        return res;
    }
         
#else
	if (major) {
		devid = MKDEV(major, 0);
		 /* (major,0~1) 对应 hello_fops, (major, 2~255)都不对应hello_fops */
		register_chrdev_region(devid, 4, "button"); 
	} else {
		/* (major,0~1) 对应 hello_fops, (major, 2~255)都不对应hello_fops */
		alloc_chrdev_region(&devid, 0, 4, "button"); 
		major = MAJOR(devid);                     
	}
	
	cdev_init(&button_dev, &button_fops);
	cdev_add(&button_dev, devid, 4);
 

#endif    
	

	button_cls = class_create(THIS_MODULE, "button");

	class_device_create(button_cls, NULL,  MKDEV(major, 0), NULL, "button");	


	gpfcon = (volatile unsigned long*)ioremap(GPFCON_PHY, 16);
	gpfdat = gpfcon + 1;
	gpgcon = (volatile unsigned long*)ioremap(GPGCON_PHY, 16);
	gpgdat = gpgcon + 1;

	return 0;
}

static void __exit button_driver_exit(void)
{

	class_device_destroy(button_cls, MKDEV(major, 0));

	class_destroy(button_cls);

	cdev_del(&button_dev);


	unregister_chrdev_region(MKDEV(major, 0), 1);

	iounmap(gpfcon);
	iounmap(gpgcon); 
}

module_init(button_driver_init);
module_exit(button_driver_exit);

MODULE_AUTHOR("David.HUANG <15118083560@163.com>");
MODULE_DESCRIPTION("Test driver for system");
MODULE_LICENSE("GPL");

