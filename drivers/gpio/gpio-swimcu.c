/*
 * gpiolib support for Sierra Wireless WPx5 MCU
 *
 * adapted from:
 * gpiolib support for Wolfson WM835x PMICs
 *
 * Copyright 2016 Sierra Wireless
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>

#include <linux/mfd/swimcu/core.h>
#include <linux/mfd/swimcu/gpio.h>

struct swimcu_gpio_data {
	struct swimcu *swimcu;
	struct gpio_chip gpio_chip;
};

static inline struct swimcu *to_swimcu(struct gpio_chip *chip)
{
	struct swimcu_gpio_data *swimcu_gpio = container_of(chip, struct swimcu_gpio_data, gpio_chip);
	return swimcu_gpio->swimcu;
}

static int swimcu_gpio_set_direction_in(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_SET_DIR, gpio, 0);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d IN\n", __func__, gpio);
	}

	return ret;
}

static int swimcu_gpio_set_direction_out(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_SET_DIR, gpio, 1+value);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d OUT\n", __func__, gpio);
	}

	return ret;
}

static int swimcu_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int value;
	int ret;

	ret = swimcu_gpio_get(swimcu, SWIMCU_GPIO_GET_VAL, gpio, &value);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
		return ret;
	}

	swimcu_log(GPIO, "%s: gpio%d get level %d\n", __func__, gpio, value);

	return value;
}

static void swimcu_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_SET_VAL, gpio, value);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d = %d\n", __func__, gpio, value);
	}
}

static int swimcu_gpio_set_pull_up(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_SET_PULL, gpio, 1);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d UP\n", __func__, gpio);
	}

	return ret;
}

static int swimcu_gpio_set_pull_down(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_SET_PULL, gpio, 0);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d DOWN\n", __func__, gpio);
	}

	return ret;
}

static int swimcu_gpio_request(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_open(swimcu, gpio);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d\n", __func__, gpio);
	}

	return ret;
}

static void swimcu_gpio_free(struct gpio_chip *chip, unsigned gpio)
{
	struct swimcu *swimcu = to_swimcu(chip);
	int ret;

	ret = swimcu_gpio_close(swimcu, gpio);

	if (ret < 0) {
		pr_err("%s: gpio%d error ret=%d\n", __func__, gpio, ret);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d\n", __func__, gpio);
	}
}

static struct gpio_chip template_chip = {
	.label			= "swimcu",
	.owner			= THIS_MODULE,
	.request    		= swimcu_gpio_request,
	.free       		= swimcu_gpio_free,
	.direction_input	= swimcu_gpio_set_direction_in,
	.get			= swimcu_gpio_get_value,
	.direction_output	= swimcu_gpio_set_direction_out,
	.set			= swimcu_gpio_set_value,
	.pull_up    		= swimcu_gpio_set_pull_up,
	.pull_down  		= swimcu_gpio_set_pull_down,
	.to_irq		  	= NULL,
	.can_sleep		= true,
};

static int swimcu_gpio_probe(struct platform_device *pdev)
{
	struct swimcu *swimcu = dev_get_drvdata(pdev->dev.parent);
	struct swimcu_platform_data *pdata = dev_get_platdata(swimcu->dev);
	struct swimcu_gpio_data *swimcu_gpio;
	int ret;

	if ((pdata != NULL) && (pdata->nr_gpio > 0)) {
		swimcu_log(GPIO, "%s: start, base %d, nr %d\n", __func__, pdata->gpio_base, pdata->nr_gpio);
	}
	else {
		pr_err("%s: no gpio\n", __func__);
		return -ENODEV;
	}

	swimcu_gpio = devm_kzalloc(&pdev->dev, sizeof(*swimcu_gpio),
				   GFP_KERNEL);
	if (swimcu_gpio == NULL)
		return -ENOMEM;

	swimcu_gpio->swimcu = swimcu;
	swimcu_gpio->gpio_chip = template_chip;
	swimcu_gpio->gpio_chip.ngpio = pdata->nr_gpio;
	swimcu_gpio->gpio_chip.dev = &pdev->dev;
	swimcu_gpio->gpio_chip.base = pdata->gpio_base;

	ret = gpiochip_add(&swimcu_gpio->gpio_chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register gpiochip, %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, swimcu_gpio);

	return ret;
}

static int swimcu_gpio_remove(struct platform_device *pdev)
{
	struct swimcu_gpio_data *swimcu_gpio = platform_get_drvdata(pdev);
	gpiochip_remove(&swimcu_gpio->gpio_chip);
	return 0;
}

static struct platform_driver swimcu_gpio_driver = {
	.driver.name	= "swimcu-gpio",
	.driver.owner	= THIS_MODULE,
	.probe		= swimcu_gpio_probe,
	.remove		= swimcu_gpio_remove,
};

static int __init swimcu_gpio_init(void)
{
	swimcu_log(GPIO, "%s: start\n", __func__);
	return platform_driver_register(&swimcu_gpio_driver);
}
subsys_initcall(swimcu_gpio_init);

static void __exit swimcu_gpio_exit(void)
{
	platform_driver_unregister(&swimcu_gpio_driver);
}
module_exit(swimcu_gpio_exit);

MODULE_DESCRIPTION("GPIO interface for Sierra Wireless MCU");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:swimcu-gpio");
