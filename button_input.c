#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <asm/machdep.h>
#include <asm/io.h>



 
struct pin_desc {        
         char* name;      
        int irq;         
        int pin;         
        int pin_val;     
}; 
    
struct pin_desc pin_data[] = {
        { "key_1", , , 0},
         { "key_2", , , 0},
         { "key_3", , , 0},
        { "key_4", , , 0}
 };                                                                                                                                            

struct input_dev *button_input_dev;

static irqreturn_t button_key_irq(int irq, void *dev_id)
{
	
}

static int  __init button_input_init(void)
{
	int err;
	int ret;

	struct input_device *button_input_dev;

	button_input_dev = input_allocate_device();
	if (!input_dev)
		return -ENOMEM;

	button_input_dev->name = "s3c2410_button";

	set_bit(EV_KEY, button_input_dev->evbit);
	set_bit (EV_REP, button_input_dev->evbit);
	set_bit(EV_KEY1, button_input_dev->keybit);

	err = input_register_device(button_input_dev);
	if (err)
	{
		return err;
	}

	for(int i=0; i<4; i++)
	{
		request_irq(irq, button_key_irq, IRQF_TRIGGER_RISING| IRQF_TRIGGER_FALLING, , NULL)
	}
		
}

static void __exit button_input_exit(void)
{

}



MODULE_AUTHOR("david.huang <15118083560@163.com>");
MODULE_DESCRIPTION("button input subsytem driver");
MODULE_LICENSE("GPL");

