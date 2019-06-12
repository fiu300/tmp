
KERN_DIR:=/home/ftp/anonymous/linux-2.6.22/
obj-m +=  button_irq.o #button_driver.o #gpio_driver.o

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean:
	-make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf Module.symvers
