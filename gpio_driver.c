
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

#define GPFCON_PHY  0x56000050
#define GPFDAT_PHY  0x56000054


#define BIT_3 3
#define BIT_4 4
#define BIT_5 5


static int gpio_open(struct inode *inode, struct file *filp)
{   
	
	*gpfcon &= ~((0x3 << (BIT_3*2)) | (0x3 << (BIT_4*2)) | (0x3 << (BIT_5*2)));
	*gpfcon |= ((0x1 << (BIT_3*2)) | (0x1 << (BIT_4*2)) | (0x1 << (BIT_5*2)));

   	return 0;
}               
                
static int gpio_release(struct inode *inode, struct file *filp)
{    
	return 0;
} 


static ssize_t gpio_write(struct file *file, const char __user *buffer,                                                               
                                 size_t count, loff_t *ppos)
{      
	int res = 0;    
	unsigned char gpio_date;


	res = copy_from_user(&gpio_date, buffer, 1);
	if (res)
		return (-EFAULT);

	if(gpio_date == 1)
	{		
		*gpfdat &= ~((0x1<<BIT_3) | (0x1<<BIT_4) | (0x1<<BIT_5));
	}
	else if(gpio_date == 0)
	{
		*gpfdat |= ((0x1<<BIT_3) | (0x1<<BIT_4) | (0x1<<BIT_5));
	}
    return -EINVAL;
}  
  


static int gpio_ioctl(struct inode *inode, struct file *file,
unsigned int cmd, unsigned long arg)
{        
	// if(_IOC_TYPE(cmd) != ETRAXI2C_IOCTYPE) {
 //    	return -EINVAL;
	// }      

	// switch (_IOC_NR(cmd)) {
	// 	case I2C_WRITEREG:
	// 		/* write to an i2c slave */
	// 		D(printk("i2cw %d %d %d\n",
	// 		I2C_ARGSLAVE(arg),
	// 		I2C_ARGREG(arg),
	// 		I2C_ARGVALUE(arg)));
         
	// 		return i2c_writereg(I2C_ARGSLAVE(arg),
	// 		I2C_ARGREG(arg),
	// 		I2C_ARGVALUE(arg));
	// 	case I2C_READREG:
	// 	{
	// 		unsigned char val;
	// 		/* read from an i2c slave */
	// 		D(printk("i2cr %d %d ",
	// 		I2C_ARGSLAVE(arg),
	// 		I2C_ARGREG(arg)));
	// 		val = i2c_readreg(I2C_ARGSLAVE(arg), I2C_ARGREG(arg));
	// 		D(printk("= %d\n", val));
	// 		return val;
	// 	}
	// 	default:
	// 		return -EINVAL;
          
	// }
         
	return 0;
}   

static const struct file_operations gpio_0_1_fops = {
	.owner = THIS_MODULE,
	.ioctl = gpio_ioctl,
	.open  = gpio_open,
	.write = gpio_write,
	.release = gpio_release
};

static const struct file_operations gpio_2_fops = {
	.owner = THIS_MODULE,
	.ioctl = gpio_ioctl,
	.open  = gpio_open,
	.release = gpio_release
};

static struct class* gpio_cls = NULL;

int major = 0;
int  devid = 0;


struct cdev gpio_0_1_dev;
struct cdev gpio_2_dev;

static int __init gpio_driver_init(void)
{
	int res=-1;

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
		register_chrdev_region(devid, 3, "gpio"); 
	} else {
		/* (major,0~1) 对应 hello_fops, (major, 2~255)都不对应hello_fops */
		alloc_chrdev_region(&devid, 0, 3, "gpio"); 
		major = MAJOR(devid);                     
	}
	
	cdev_init(&gpio_0_1_dev, &gpio_0_1_fops);
	cdev_add(&gpio_0_1_dev, devid, 2);
 
	devid = MKDEV(major, 2);
	cdev_init(&gpio_2_dev, &gpio_2_fops);
	cdev_add(&gpio_2_dev, devid, 1);

#endif    
	

	gpio_cls = class_create(THIS_MODULE, "gpio");

	class_device_create(gpio_cls, NULL,  MKDEV(major, 0), NULL, "gpio_0");
	class_device_create(gpio_cls, NULL,  MKDEV(major, 1), NULL, "gpio_1");
	class_device_create(gpio_cls, NULL,  MKDEV(major, 2), NULL, "gpio_2");

	gpfcon = (volatile unsigned long*)ioremap(GPFCON_PHY, PAGE_SIZE);
	gpfdat = gpfcon + 1;

	return 0;
}

static void __exit gpio_driver_exit(void)
{

	class_device_destroy(gpio_cls, MKDEV(major, 0));
	class_device_destroy(gpio_cls, MKDEV(major, 1));
	class_device_destroy(gpio_cls, MKDEV(major, 2));

	class_destroy(gpio_cls);

	cdev_del(&gpio_2_dev);
	cdev_del(&gpio_0_1_dev);

	unregister_chrdev_region(MKDEV(major, 0), 3);

	iounmap(gpfcon); 
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_AUTHOR("David.HUANG <15118083560@163.com>");
MODULE_DESCRIPTION("Test driver for system");
MODULE_LICENSE("GPL");

