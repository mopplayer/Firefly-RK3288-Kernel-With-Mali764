#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include <linux/version.h>   
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif


struct firefly_noatx8_info {
	struct platform_device	*pdev;
	int     power_gpio;
	int     power_enable_value;
};

static struct firefly_noatx8_info noatx8_info;

static int firefly_noatx8_probe(struct platform_device *pdev)
{
    int ret = -1;
    int gpio, rc,flag;
    unsigned long ddata;
	struct device_node *noatx8_node = pdev->dev.of_node;

    noatx8_info.pdev = pdev;

	gpio = of_get_named_gpio_flags(noatx8_node,"atx8-rst", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid noatx8-power: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "noatx8_power");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto fainoatx8_1;
	}
	noatx8_info.power_gpio = gpio;
	noatx8_info.power_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(noatx8_info.power_gpio, !(noatx8_info.power_enable_value));
    	
	printk("%s %d\n",__FUNCTION__,__LINE__);
	
	return 0;  //return Ok

fainoatx8_1:
	return ret;
}


static int firefly_noatx8_remove(struct platform_device *pdev)
{ 
    return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id of_rk_firefly_noatx8_match[] = {
	{ .compatible = "firefly,noatx8" },
	{ /* Sentinel */ }
};
#endif

static struct platform_driver firefly_noatx8_driver = {
	.probe		= firefly_noatx8_probe,
	.remove		= firefly_noatx8_remove,
	.driver		= {
		.name	= "atx8_unsetup",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_rk_firefly_noatx8_match,
#endif
	},

};


static int __init firefly_noatx8_init(void)
{
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
    return platform_driver_register(&firefly_noatx8_driver);
}

static void __exit firefly_noatx8_exit(void)
{
	platform_driver_unregister(&firefly_noatx8_driver);
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
}

subsys_initcall(firefly_noatx8_init);
module_exit(firefly_noatx8_exit);

MODULE_AUTHOR("zhansb <teefirefly@gmail.com>");
MODULE_DESCRIPTION("Firefly noatx8 driver");
MODULE_LICENSE("GPL");
