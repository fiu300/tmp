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
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/arch/regs-gpio.h>

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

static DECLARE_WAIT_QUEUE_HEAD(wait_queue); 
int wait_condition = 0;

static DECLARE_MUTEX(button_lock);

unsigned long pin_val = 0;

struct pin_desc {
	int pin;
	int pin_val;
};

static struct fasync_struct *button_async_queue;
static struct timer_list button_timer;


struct pin_desc pins_desc[4] = {
	{S3C2410_GPF0, 0x1},
	{S3C2410_GPF2, 0x2},
	{S3C2410_GPG3, 0x3},
	{S3C2410_GPG11, 0x4}
};

struct pin_desc* pin_data = NULL;

irqreturn_t irq_handler(int irq, void* dev_id)
{
		
	pin_data = (struct pin_desc*)dev_id;

	button_timer.expires = jiffies + (HZ / 10);
	mod_timer(&button_timer, button_timer.expires);


	return IRQ_RETVAL(IRQ_HANDLED);
}

static int button_open(struct inode *inode, struct file *filp)
{   
	int err;

	if(filp->f_flags & O_NONBLOCK)
	{
		if(down_trylock(&button_lock))
			return -EBUSY;
	}
	else
	{
		down(&button_lock);
	}

	*gpfcon &= ~((0x3 << (BIT_0*2)) | (0x3 << (BIT_2*2)));
	*gpfcon |= ((0x2 << (BIT_0*2)) | (0x2 << (BIT_2*2)));

	*gpgcon &= ~((0x3 << (BIT_3*2)) | (0x3 << (BIT_11*2)));
	*gpgcon |= ((0x2 << (BIT_3*2)) | (0x2 << (BIT_11*2)));

	err = request_irq(IRQ_EINT0, irq_handler, IRQT_BOTHEDGE, "button_2", (void*)&pins_desc[0]);
   	err = request_irq(IRQ_EINT2, irq_handler, IRQT_BOTHEDGE, "button_3", (void*)&pins_desc[1]);
	err = request_irq(IRQ_EINT11, irq_handler, IRQT_BOTHEDGE, "button_4", (void*)&pins_desc[2]);
	err = request_irq(IRQ_EINT19, irq_handler, IRQT_BOTHEDGE, "button_5", (void*)&pins_desc[3]);
   	
   	return 0;
}               


static int button_fasync(int fd, struct file *file, int on);               

static int button_release(struct inode *inode, struct file *filp)
{    

	free_irq(IRQ_EINT0, &pins_desc[0]);
	free_irq(IRQ_EINT2, &pins_desc[1]);
	free_irq(IRQ_EINT11, &pins_desc[2]);
	free_irq(IRQ_EINT19, &pins_desc[3]);
	
	button_fasync(-1, filp, 0);
	
	up(&button_lock);
	return 0;
} 

static ssize_t button_read (struct file *file, char *buf, size_t len, loff_t *pos)
{                               
	int ret;
	
	if(file->f_flags & O_NONBLOCK)
	{
		if(!wait_condition)
		{
			return -EAGAIN;	
		}
	}
	else
	{
		wait_event_interruptible(wait_queue, wait_condition);
	}

	ret = copy_to_user(buf, &pin_val, 1);

	wait_condition = 0;

	return 1;             
}  

 
static unsigned int button_poll(struct file *file, struct poll_table_struct *wait)                                                            
{                    
	int mask = 0;

	poll_wait(file, &wait_queue, wait);
	
	if(wait_condition)
		mask = POLLIN | POLLRDNORM;

	return mask;
} 

static int button_fasync(int fd, struct file *file, int on)
{
        return fasync_helper(fd, file, on, &button_async_queue);
}

static const struct file_operations button_fops = {
	.owner = THIS_MODULE,
	.open  = button_open,
	.release = button_release,
	.read = button_read,
	.poll = button_poll,
	.fasync = button_fasync,
};


static struct class* button_cls = NULL;

int major = 0;
int  devid = 0;


struct cdev button_dev;

static void button_timer_function(unsigned long dummy)
{
	if(!pin_data)
		return ;

	pin_val = s3c2410_gpio_getpin(pin_data->pin);
	if(pin_val)
	{
		pin_val = pin_data->pin_val;
	}
	else
	{
		pin_val = 0x80 | pin_data->pin_val;
	}

	wait_condition = 1;
	wake_up_interruptible(&wait_queue);

	kill_fasync(&button_async_queue, SIGIO, POLL_IN);
	
}

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

	init_timer(&button_timer);
	button_timer.function = button_timer_function;
//	button_timer.expires = jiffies + (HZ / 10);
	add_timer(&button_timer);

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


