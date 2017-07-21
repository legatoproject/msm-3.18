/************
 *
 * $Id$
 *
 * Filename:  gpio_wake_n.c
 *
 * Purpose:
 *
 * Copyright: (c) 2016 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sierra_gpio.h>

#define NUM_WAKE_GPIOS 1
#define DRIVER_NAME "wake-n_gpio"

struct wake_n_pdata {
	int gpio;
	char name[64];
	int irq;
	struct wakeup_source ws;
	struct platform_device *pdev;
	struct work_struct check_work;
} wake_n_pdata;

static void gpio_check_and_wake(struct work_struct *work)
{
	int err, gpioval;
	struct wake_n_pdata *w;
	char event[16], *envp[2];

	w = container_of(work, struct wake_n_pdata, check_work);
	gpioval = gpio_get_value(w->gpio);
	sprintf(event, "STATE=%s", (gpioval ? "SLEEP" : "WAKEUP"));
	pr_info("%s: %s %s\n", __func__, w->name, event);

	envp[0] = event;
	envp[1] = NULL;
	kobject_get(&w->pdev->dev.kobj);
	if ((err = kobject_uevent_env(&w->pdev->dev.kobj, KOBJ_CHANGE, envp)))
		pr_debug("%s: error %d signaling uevent\n", __func__, err);
	kobject_put(&w->pdev->dev.kobj);
	if (gpioval)
		__pm_relax(&w->ws);
}

static irqreturn_t gpio_wake_input_irq_handler(int irq, void *dev_id)
{
	struct wake_n_pdata *w = (struct wake_n_pdata*)dev_id;

	/*
	 * The gpio_check_and_wake routine calls kobject_uevent_env(),
	 * which might sleep, so cannot call it from interrupt context.
	 */
	__pm_stay_awake(&w->ws);
	schedule_work(&w->check_work);
	return IRQ_HANDLED;
}

static int wake_n_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	dev_info(&pdev->dev, "wake_n probe\n");

	wake_n_pdata.gpio = of_get_named_gpio(np, "wake-n-gpio", 0);
	if (!gpio_is_valid(wake_n_pdata.gpio))
		return ret;

	ret = gpio_request(wake_n_pdata.gpio, "WAKE_N_GPIO");
	if (ret) {
		pr_err("%s: failed to get GPIO%d, error code is%d\n", __func__,
			wake_n_pdata.gpio,ret);
		return ret;
	}

	snprintf(wake_n_pdata.name, sizeof(wake_n_pdata.name), "wake-n_gpio%d",
		wake_n_pdata.gpio);
	if (gpio_direction_input(wake_n_pdata.gpio)) {
		pr_err("%s: failed to set GPIO%d to input\n", __func__,
			wake_n_pdata.gpio);
		goto release_gpio;
	}

	if (gpio_pull_up(gpio_to_desc(wake_n_pdata.gpio))) {
		pr_err("%s: failed to set GPIO wake_N to pull-up\n", __func__);
		goto release_gpio;
	}

	wake_n_pdata.irq = gpio_to_irq(wake_n_pdata.gpio);
	if(wake_n_pdata.irq < 0){
		pr_err("%s: no IRQ associated with GPIO%d\n", __func__,
			wake_n_pdata.gpio);
		goto release_gpio;
	}

	ret = request_irq(wake_n_pdata.irq, gpio_wake_input_irq_handler,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      wake_n_pdata.name, &wake_n_pdata);
	if (ret) {
		pr_err("%s: request_irq failed for GPIO%d (IRQ%d)\n", __func__,
			wake_n_pdata.gpio, wake_n_pdata.irq);
		goto release_gpio;
	}

	ret = enable_irq_wake(wake_n_pdata.irq);
	if (ret) {
		pr_err("%s: enable_irq failed for GPIO%d\n", __func__,
			wake_n_pdata.gpio);
		goto free_irq;
	}

	wakeup_source_init(&wake_n_pdata.ws, "wake-n_GPIO");
	wake_n_pdata.pdev = pdev;
	INIT_WORK(&wake_n_pdata.check_work, gpio_check_and_wake);
	__pm_stay_awake(&wake_n_pdata.ws);
	schedule_work(&wake_n_pdata.check_work);

	return 0;

free_irq:
	free_irq(wake_n_pdata.irq, NULL);
release_gpio:
	gpio_free(wake_n_pdata.gpio);
	return ret;
}

static int wake_n_remove(struct platform_device *pdev)
{
	pr_info("wake_n_remove");
	gpio_free(wake_n_pdata.gpio);
	wakeup_source_trash(&wake_n_pdata.ws);
	return 0;
}

static const struct of_device_id sierra_gpio_wake_n[] = {
	{ .compatible = "sierra,gpio_wake_n" },
	{},
};
MODULE_DEVICE_TABLE(of, sierra_gpio_wake_n);

static struct platform_driver wake_n_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table=of_match_ptr(sierra_gpio_wake_n),
		},
	.probe = wake_n_probe,
	.remove = wake_n_remove,
};

static int __init wake_n_init(void)
{
	return platform_driver_register(&wake_n_driver);
}

static void __exit wake_n_exit(void)
{
	platform_driver_unregister(&wake_n_driver);
}

module_init(wake_n_init);
module_exit(wake_n_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("GPIO wake_n pin driver");
