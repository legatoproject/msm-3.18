/************
 *
 * $Id$
 *
 * Filename:  sierra_safe_pwr_remove.c
 *
 * Purpose:
 *
 * Copyright: (c) 2017 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/reboot.h>

#define DRIVER_NAME "sierra_safe_pwr_remove"

struct sfpr_data {
	int gpio;
	struct notifier_block reboot_nb;
};

static int sfpr_reboot_handler(struct notifier_block *this,
				unsigned long code, void* cmd)
{
	struct sfpr_data* data;

	data = container_of(this, struct sfpr_data, reboot_nb);
	if(code == SYS_POWER_OFF) {
		gpio_set_value(data->gpio, 1);
	}
	return NOTIFY_DONE;
}

static int sierra_sfpr_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sfpr_data *data;

	data = (struct sfpr_data*)devm_kzalloc(&pdev->dev,
		sizeof(struct sfpr_data*), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->gpio = of_get_named_gpio(pdev->dev.of_node, "sfpr-gpio", 0);
	dev_set_drvdata(&pdev->dev, (void*)data);
	ret = gpio_request(data->gpio, "sfpr-gpio");

	if(!ret) {
		gpio_set_value(data->gpio, 0);
		data->reboot_nb.notifier_call = sfpr_reboot_handler;
		register_reboot_notifier(&data->reboot_nb);
	}
	else {
		devm_kfree(&pdev->dev, data);
	}
	return ret;
}

static int sierra_sfpr_remove(struct platform_device *pdev)
{
	struct sfpr_data* data;

	data = dev_get_drvdata(&pdev->dev);
	gpio_free(data->gpio);
	return 0;
}

static const struct of_device_id swi_sfpr_dt_match[] = {
	{ .compatible = "sierra,safe_pwr_remove" },
	{},
};

static struct platform_driver sierra_sfpr_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = swi_sfpr_dt_match,
	},

	.probe = sierra_sfpr_probe,
	.remove = sierra_sfpr_remove,
};

static int __init sierra_sfpr_init(void)
{
	return platform_driver_register(&sierra_sfpr_driver);
}

static void __exit sierra_sfpr_exit(void)
{
	platform_driver_unregister(&sierra_sfpr_driver);
}

module_init(sierra_sfpr_init);
module_exit(sierra_sfpr_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sierra_safe_pwr_remove driver");
