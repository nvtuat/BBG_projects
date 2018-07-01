/*********************************************************************************************************************
**                                                                                               **
**To run:                                                                                                           **
** echo "out" > /sys/my_gpio_module/direction  to set pin mode is output                                            **
** echo 1 > /sys/my_gpio_module/value          to set pin high                                                      **
** echo 0 > /sys/my_gpio_module/value          to set pin low                                                       **
**                                                                                                                  **
**********************************************************************************************************************
*********************************************************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/io.h>

/* define board */
#define BBG

#ifdef BBG // Beagle Bone green
#define GPIO_OE 77 // 0x134 = 308 , 308/4=77
#define GPIO_DATAIN 78 // 0x138 = 312
#define GPIO_DATAOUT 79 // 0x13C = 316
#define GPIO0_ADDR 0x44E07000
#define GPIO1_ADDR 0x4804C000
#define GPIO2_ADDR 0x481AC000
#define GPIO3_ADDR 0x481AE000
#define GPIO_PIN 17
#endif

#ifdef RASP3 // Rasperry Pi 3
#define GPIO_ADDR_BASE			0x3F200000
#define GPIO_PIN			17
#define GPFSEL1			1
#define GPSET0			7
#define GPCLR0			10
#define GPLEV0			13
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TUAT");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("CONTROL LED MODULE");

static unsigned int *gpio_base;
static int value_get_from_user = 0;
static char message[5] = {0};

unsigned int *set_mode;
unsigned int *set_high;
unsigned int *set_low;
unsigned int *set_value;

static struct kobject *kobj;

static int get_value_pin(void)
{
#if defined(RASP3)
	return (*((unsigned int*)(gpio_base + GPLEV0))) & (1 << GPIO_PIN);
#elif defined(BBG)
	return (*((unsigned int*)(gpio_base + GPIO_DATAOUT))) & (1 << GPIO_PIN);
#endif
}

static int get_direction(void)
{
#if defined(RASP3)
	return (*((unsigned int *)(gpio_base + GPFSEL1))) & (1 << 21);
#elif defined(BBG)
	return !((*((unsigned int *)(gpio_base + GPIO_OE))) & (1 << GPIO_PIN));
#endif
}

static void set_pin_mode(char *mode)
{
#if defined(RASP3)
	set_mode = (unsigned int *)gpio_base + GPFSEL1;
#elif defined(BBG)
  set_mode = (unsigned int *)gpio_base + GPIO_OE;
#endif

	/*mode = 1 set output, mode = 0 set input*/
	if(!strcmp(mode, "in"))
	{
		printk(KERN_INFO "set mode in\n");
#if defined(RASP3)
		(*set_mode) = ((*set_mode) & (~(7 << 21))) | (0 << 21);
#elif defined(BBG)
		(*set_mode) = (*set_mode) | (1 << GPIO_PIN);
#endif
	}
	else if(!strcmp(mode, "out"))
	{
		printk(KERN_INFO "set mode out\n");
#if defined(RASP3)
		(*set_mode) = ((*set_mode) & (~(7 << 21))) | (1 << 21);
#elif defined(BBG)
		(*set_mode) = (*set_mode) & (~(1 << GPIO_PIN));
#endif
	}
	else
	{
		printk(KERN_ALERT "String not suiltable\n");
		return;
	}
	printk(KERN_INFO "value of set mode reg after set is %d\n", *set_mode);
}

static void set_pin_value(int value)
{
#if defined(RASP3)
	set_high = (unsigned int *)(gpio_base + GPSET0);
	set_low = (unsigned int *)(gpio_base + GPCLR0);
	/*value = 1 set high, value = 0 set low*/
	if(value)
	{
		printk(KERN_INFO "set pin high\n");
		(*set_high) = 1 << GPIO_PIN;
	}
	else
	{
		printk(KERN_INFO "set pin low\n");
		(*set_low) = 1 << GPIO_PIN;
	}
	printk(KERN_INFO "Value of SET reg is: %d, CLR reg is: %d\n", *set_high, *set_low);
#elif defined(BBG)
	set_value = (unsigned int *)(gpio_base + GPIO_DATAOUT);
	if(value)
	{
		printk(KERN_INFO "set pin high\n");
		(*set_value) = (*set_value) | (1 << GPIO_PIN);
	}
	else
	{
		printk(KERN_INFO "set pin low\n");
		(*set_value) = (*set_value) & (~(1 << GPIO_PIN));
	}
	printk(KERN_INFO "Value of OUT reg is: %d\n",*set_value);
#endif
}


static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int val;
	val = get_value_pin();
	if(val)
		val = 1;

	printk(KERN_INFO "value of pin is: %d\n", val);

	return sprintf(buf, "%d\n", val);
}
static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "Val_store function is called\n");

	sscanf(buf, "%d", &value_get_from_user);
	printk(KERN_INFO "get from user: %d\n", value_get_from_user);
	if(get_direction()) //check if pin mode is output
	{
		set_pin_value(value_get_from_user);
	}
	else
	{
		printk(KERN_ALERT "direction is input can not set pin value\n");
	}
        return count;
}

static ssize_t direction_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if(get_direction())
	{
		return sprintf(buf, "%s", "out\n");
	}
        return sprintf(buf, "%s", "in\n");
}

static ssize_t direction_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "direction store function is called\n");
	sscanf(buf, "%s", message);
	printk(KERN_INFO "get from user: %s\n", message);
	set_pin_mode(message);
        return count;
}

static struct kobj_attribute val_kobj_attr = __ATTR(value, 0644, val_show, val_store);
static struct kobj_attribute dir_kobj_attr = __ATTR(direction, 0644, direction_show, direction_store);

static struct attribute *attr[] = {
	&val_kobj_attr.attr,
	&dir_kobj_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attr,
};

static int __init exam_init(void)
{
	int ret;

	printk(KERN_INFO "Call from init function\n");

#if defined(RASP3)
	gpio_base = (unsigned int *)ioremap(GPIO_ADDR_BASE, 0x100);
#elif defined(BBG)
	gpio_base = (unsigned int *)ioremap(GPIO1_ADDR, 0x200);
#endif


	if(gpio_base == NULL)
	{
		printk(KERN_ALERT "Can not map to to virtual addr\n");
		return -ENOMEM;
	}

	/*default pin mode is out*/
	 set_pin_mode("out");

	/*default value is high*/
	set_pin_value(1);

	kobj = kobject_create_and_add("my_gpio_module", kernel_kobj->parent);
	if(kobj == NULL)
	{
		printk(KERN_ALERT "can not create kobject\n");
		return -1;
	}
	printk(KERN_INFO "create successfully kobject\n");

	ret = sysfs_create_group(kobj, &attr_group);
	if(ret)
	{
		printk(KERN_ALERT "can not create group files\n");
		kobject_put(kobj);
	}
	printk(KERN_INFO "created group file\n");
	return 0;
}

static void __exit exam_exit(void)
{
	printk(KERN_INFO "goodbye\n");
	// kobject_put(kobj);
	// set_pin_value(0);
	iounmap(gpio_base);
	printk(KERN_INFO "exit\n");
}

module_init(exam_init);
module_exit(exam_exit);
