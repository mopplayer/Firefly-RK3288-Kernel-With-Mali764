#include <asm/cacheflush.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nsproxy.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/kernel.h>

#define TCHIP_REMOTE_CTRL_DEBUG
#ifdef TCHIP_REMOTE_CTRL_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)
#endif

#define MY_UP 0
#define MY_DOWN 1
#define MY_REPEAT 2
#define MAX_FINGER_NUM 10
/*此两值需要根据具体的UI界面的widthPixels heightPixels来设置，否则位置将不准*/
#define SCREEN_MAX_X 		1920    
#define SCREEN_MAX_Y 		1080
#define PRESS_MAX	255

#define CMD_BYTE_LENGTH 100
#define CMD_ARGS_LENGTH 8

#define GSENSOR_DATA_MULTIPLE 1000000
#define G_MAX (GSENSOR_DATA_MULTIPLE * 128)

static struct input_dev * input_key;
static struct input_dev * input_abs;
static struct input_dev * input_gsensor;

int tchip_remote_gsensor_register_device(void)
{	
	int error = -1;
	input_gsensor = input_allocate_device();
	if (!input_gsensor)
		return 1;
	
	input_gsensor->name = "tchip_remote_gsensor";
	// input_gsensor->phys = "gpio-keys/input0";
	// input_gsensor->id.bustype = BUS_HOST;
	// input_gsensor->id.vendor = 0x0001;
	// input_gsensor->id.product = 0x0001;
	// input_gsensor->id.version = 0x0100;

	set_bit(EV_ABS, input_gsensor->evbit);

	/* x-axis acceleration */
	input_set_abs_params(input_gsensor, ABS_X, -G_MAX, G_MAX, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(input_gsensor, ABS_Y, -G_MAX, G_MAX, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(input_gsensor, ABS_Z, -G_MAX, G_MAX, 0, 0);

	error = input_register_device(input_gsensor);	
	if (error) 
	{
		printk(KERN_ERR "%s:Unable to register input device: %s\n", __func__, input_gsensor->name);	
		input_unregister_device(input_gsensor);					 
		return 1;
	}				
	printk(KERN_INFO "tchip_remote_gsensor_register_device is here!\n");
	return 0;
		
} 


int tchip_remote_gsensor_unregister_device(void)
{	
	input_unregister_device(input_gsensor);	
	input_free_device(input_gsensor);
	return 0;
}

int tchip_remote_abs_register_device(void)
{	
	input_abs = input_allocate_device();
	if (!input_abs)
		return 1;
	
	input_abs->name = "tchip_remote_touch";
	input_abs->phys = "gpio-keys/input0";
	input_abs->id.bustype = BUS_HOST;
	input_abs->id.vendor = 0x0001;
	input_abs->id.product = 0x0001;
	input_abs->id.version = 0x0100;

	__set_bit(EV_ABS, input_abs->evbit);
	__set_bit(EV_KEY, input_abs->evbit);
	__set_bit(EV_REP, input_abs->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_abs->propbit);
	input_mt_init_slots(input_abs, (MAX_FINGER_NUM+1),0);

	set_bit(ABS_MT_POSITION_X, input_abs->absbit);
	set_bit(ABS_MT_POSITION_Y, input_abs->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_abs->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_abs->absbit);

	input_set_abs_params(input_abs,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_abs,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_abs,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_abs,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	input_register_device(input_abs);					
	printk(KERN_INFO "tchip_remote_abs_register_device is here!\n");
	return 0;
		
} 


int tchip_remote_abs_unregister_device(void)
{	
	input_unregister_device(input_abs);	
	input_free_device(input_abs);
	return 0;
}

int tchip_remote_key_register_device(void)
{	
	int i;	

	input_key = input_allocate_device();
	if (!input_key)
		return 1;
	
	input_key->name = "tchip_remote_key";
	input_key->phys = "gpio-keys/input0";
	input_key->id.bustype = BUS_HOST;
	input_key->id.vendor = 0x0001;
	input_key->id.product = 0x0001;
	input_key->id.version = 0x0100;

	input_key->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL) | BIT_MASK(EV_MSC);
	input_key->keybit[BIT_WORD(BTN_MOUSE)] =
				BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_key->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);
	input_key->mscbit[0] = BIT_MASK(MSC_SCAN) | BIT_MASK(MSC_SERIAL) | BIT_MASK(MSC_RAW);


	for(i=0;i<KEY_MAX;i++)
	{
		input_set_capability(input_key,EV_KEY,i);
	}
	input_set_capability(input_key, EV_MSC, MSC_SCAN);

	input_register_device(input_key);					
	printk(KERN_INFO "tchiprm_register_device is here!\n");
	return 0;
		
} 


int tchip_remote_key_unregister_device(void)
{	
	input_unregister_device(input_key);	
	input_free_device(input_key);
	return 0;
}


void my_input_report_key(int keycode,int press)
{
	input_event(input_key, EV_MSC, MSC_SCAN, keycode);
	print_info("keycode:%d, press:%d\n", keycode, press);
	input_event(input_key, EV_KEY, keycode, press);
	// input_report_key(input_key,keycode,press);

	if(press != MY_REPEAT)
	{
		input_sync(input_key);
	}
	else
	{
		input_event(input_key, EV_SYN, SYN_REPORT, 1);
	}
	
}

void my_input_report_gsensor(char* args[])
{
	int x;
	int y;
	int z;
	kstrtoint(args[1], 10, &x);
	kstrtoint(args[2], 10, &y);
	kstrtoint(args[3], 10, &z);		
	print_info("x:%d\ty:%d\tz:%d\n", x, y, z);
	input_report_abs(input_gsensor, ABS_X, x);
	input_report_abs(input_gsensor, ABS_Y, y);
	input_report_abs(input_gsensor, ABS_Z, z);
	input_sync(input_gsensor);	
}

ssize_t tchip_remote_write(struct file * filp,char __user *buff,size_t count,loff_t * offp)
{
	char arg[CMD_BYTE_LENGTH];
	int argn = 0;
	char* args[CMD_ARGS_LENGTH];	
	char* token;
	char* ps_cur=arg;
	int keycode;
	int rel_x;
	int rel_y;	
	int wheel_y;
	int id;
	int pressure;
	int x;
	int y;
	
	memset(arg, 0, CMD_BYTE_LENGTH);
	copy_from_user(arg, buff, count);
	print_info("arg:%s\n",arg);
	memset(args, 0, sizeof(args));

	while((token=strsep(&ps_cur, " "))&& (*token != '\0' && argn<CMD_ARGS_LENGTH)){
		args[argn++]=token;
	}
	if(!strcmp(args[0],"key")){		

		kstrtoint(args[2], 10, &keycode);
		if(!strcmp(args[1],"down")){
			my_input_report_key(keycode,MY_DOWN);
		}else if(!strcmp(args[1],"up")){
			my_input_report_key(keycode,MY_UP);
		}else if(!strcmp(args[1],"repeat")){
			my_input_report_key(keycode,MY_REPEAT);
		}else if(!strcmp(args[1],"click")){
			my_input_report_key(keycode,MY_DOWN);
			my_input_report_key(keycode,MY_UP);						
		}
	}else if(!strcmp(args[0],"move")){
		
		kstrtoint(args[1], 10, &rel_x);
		kstrtoint(args[2], 10, &rel_y);
		if (rel_x > 3000 || rel_x < -3000 || rel_y > 3000 || rel_y < -3000)
		{
			printk(KERN_INFO "tchiprm move x:%d,y:%d err!\n", rel_x, rel_y);
			return -1;
		}
		input_report_rel(input_key, REL_X, rel_x);
		input_report_rel(input_key, REL_Y, rel_y);
		input_sync(input_key);
	}else if(!strcmp(args[0],"wheel")){
		
		kstrtoint(args[1], 10, &wheel_y);
		input_report_rel(input_key, REL_WHEEL, wheel_y);
		input_sync(input_key);	
	}else if(!strcmp(args[0],"abs")){
		kstrtoint(args[1], 10, &id);
		kstrtoint(args[2], 10, &pressure);
		kstrtoint(args[3], 10, &x);
		kstrtoint(args[4], 10, &y);
		input_mt_slot(input_abs, id);	

		if(x>=SCREEN_MAX_X||y>=SCREEN_MAX_Y)
		{	
			return -1;
		}
		if(MY_DOWN == pressure || MY_REPEAT == pressure)
		{				
			input_report_abs(input_abs, ABS_MT_TRACKING_ID, id);
			input_report_abs(input_abs, ABS_MT_TOUCH_MAJOR, 10);
			input_report_abs(input_abs, ABS_MT_POSITION_X, x);
			input_report_abs(input_abs, ABS_MT_POSITION_Y, y);	
			input_report_abs(input_abs, ABS_MT_WIDTH_MAJOR, 1);
		}else{	
			input_report_abs(input_abs, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(input_abs, MT_TOOL_FINGER, false);
		}
		input_sync(input_abs);		
	}else if(!strcmp(args[0],"gsensor")){
		my_input_report_gsensor(args);
	}
	
	return count;
}

static struct file_operations tchip_rm_fops = {
        .owner = THIS_MODULE,
        // .open = tchiprm_open,
        .write = tchip_remote_write,
        // .release = tchiprm_release,
    };


static struct miscdevice tchip_rm_miscdev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "tchip_rm",
        .fops = &tchip_rm_fops
};

static int __init tchip_rm_init(void)
{
    int ret;
	printk(KERN_INFO "tchip_rm_init is here!\n");
	ret = tchip_remote_key_register_device();
	if(ret != 0)
	{
		return ret;
	}
	ret = tchip_remote_abs_register_device();
	if(ret != 0)
	{
		goto tchip_remote_key_unregister;
	}
	ret = tchip_remote_gsensor_register_device();
	if(ret != 0)
	{
		goto tchip_remote_abs_unregister;
	}
	
    ret = misc_register(&tchip_rm_miscdev);
	if(ret == 0)
	{
		return ret;
	}

	tchip_remote_gsensor_unregister_device();
tchip_remote_abs_unregister:
	tchip_remote_abs_unregister_device();
tchip_remote_key_unregister:
	tchip_remote_key_unregister_device();
	return ret;
	
}

static int __init tchip_rm_exit(void)
{
    int ret;
	printk(KERN_INFO "tchip_rm_exit is here!\n");
    ret = misc_deregister(&tchip_rm_miscdev);
    tchip_remote_key_unregister_device();
    tchip_remote_abs_unregister_device();
    tchip_remote_gsensor_unregister_device();
	return ret;
}

__initcall(tchip_rm_init);
__exitcall(tchip_rm_exit);


