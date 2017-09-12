/************
 *
 * $Id$
 *
 * Filename:  sierra_tcxo_clk.c
 *
 * Purpose:
 *
 * Copyright: (c) 2016 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/pm.h>
#include <linux/debugfs.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/qpnp/pin.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>


#define DRIVER_NAME "sierra_tcxo_clk"

struct swi_tcxo {
  uint32_t enable;
  int clk_gpio;
  struct kobject *tcxo_obj;
  struct attribute_group *attr_group;
  struct device *dev;
  struct clk *div_clk;   /* interface clock */
  struct delayed_work work;
};
struct swi_tcxo *data = NULL;

static struct qpnp_pin_cfg pm9635_gpio01_conf = {
  .mode = QPNP_PIN_MODE_DIG_OUT,
  .output_type = QPNP_PIN_OUT_BUF_CMOS,
  .invert = QPNP_PIN_INVERT_DISABLE,
  .vin_sel = QPNP_PIN_VIN2,
  .src_sel = QPNP_PIN_SEL_FUNC_1,
  .out_strength = QPNP_PIN_OUT_STRENGTH_LOW,
  .master_en = QPNP_PIN_MASTER_ENABLE,
};

static ssize_t sierra_tcxo_show(struct kobject *kobj,
  struct kobj_attribute *attr,
  const char *buf);
static ssize_t sierra_tcxo_store(struct kobject *kobj,
    struct kobj_attribute *attr,
    const char *buf,
    size_t count);

static struct kobj_attribute sierra_tcxo_attribute =
  __ATTR(tcxo_en, 0664, sierra_tcxo_show, sierra_tcxo_store);

static struct attribute *attrs[] = {
  &sierra_tcxo_attribute.attr,
  NULL,
};

static ssize_t sierra_tcxo_show(struct kobject *kobj,
  struct kobj_attribute *attr,
  const char *buf)
{
  uint8_t enabled;
  enabled = data->enable;

  return sprintf((char *)buf, "%u\n", enabled);
}

static ssize_t sierra_tcxo_store(struct kobject *kobj,
  struct kobj_attribute *attr,
  const char *buf,
  size_t count)
{
  int ret = 0;
  uint32_t enabled;

  if(kstrtoint(buf, 0, &enabled))
    return -EINVAL;
  if (!((enabled == 0) || (enabled == 1)))
    return -EINVAL;

  if(enabled && !data->enable)
  {
    pr_debug("enable div clk\n");
    ret = clk_prepare_enable(data->div_clk);
    if(ret)
    {
      pr_err("%s: unable to enable tcxo clk\n", __func__);
      return ret;
    }
  }
  else if(!enabled && data->enable)
  {
    pr_debug("disable div clk\n");
    clk_disable_unprepare(data->div_clk);
  }

  data->enable = enabled;

  return count;
}

static void clk_gpio_enable(struct work_struct *work)
{
  /* Enable tcxo clk gpio */
  qpnp_pin_config(data->clk_gpio, &pm9635_gpio01_conf);
}
static int sierra_tcxo_clk_probe(struct platform_device *pdev)
{
  int ret = 0;
  dev_info(&pdev->dev, "sierra_tcxo_clk probe\n");
  struct device_node *np = pdev->dev.of_node;

  data = (struct swi_tcxo*)kzalloc(sizeof(struct swi_tcxo*), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto exit_free;
  }
  data->enable = 0;
  data->dev = &pdev->dev;
  data->tcxo_obj = NULL;
  data->attr_group = devm_kzalloc(&pdev->dev,
        sizeof(*(data->attr_group)),
        GFP_KERNEL);
  if (!data->attr_group) {
    dev_err(&pdev->dev, "%s: malloc attr_group failed\n",
           __func__);
    ret = -ENOMEM;
    goto error_return;
  }

  data->div_clk = devm_clk_get(&pdev->dev, "div_clk");
  if (IS_ERR(data->div_clk)) {
    pr_err("%s: unable to get iface_clk\n", __func__);
    goto exit_free;
  }

  data->clk_gpio = of_get_named_gpio(np,"tcxo-output-gpio",0);
  data->attr_group->attrs = attrs;
  dev_set_drvdata(&pdev->dev, data);

  /* /sys/kernel/sierra/tcxo_en */
  data->tcxo_obj = kobject_create_and_add("sierra", kernel_kobj);
  if (!data->tcxo_obj) {
    ret = -ENOMEM;
    goto error_return;
  }

  ret = sysfs_create_group(data->tcxo_obj, data->attr_group);
  if (ret) {
  dev_err(&pdev->dev, "%s: sysfs create group failed %d\n",
          __func__, ret);
  goto error_return;
  }

  INIT_DELAYED_WORK(&data->work, clk_gpio_enable);
  schedule_delayed_work(&data->work, msecs_to_jiffies(3000));
  return 0;

  error_return:
      if (data->tcxo_obj) {
        kobject_del(data->tcxo_obj);
        data->tcxo_obj = NULL;
      }
  exit_free:
       kfree(data);
       dev_set_drvdata(&pdev->dev, NULL);

  return ret;
}

static int sierra_tcxo_clk_remove(struct platform_device *pdev)
{

  if(data->tcxo_obj)
  {
    sysfs_remove_group(data->tcxo_obj, data->attr_group);
    kobject_del(data->tcxo_obj);
    data->tcxo_obj = NULL;
  }

  kfree(data);
  pr_debug("sierra_tcxo_clk_remove");
 
  return 0;
}

static const struct of_device_id swi_tcxo_clk_dt_match[] = {
  { .compatible = "sierra,tcxo_clk" },
  {},
};

static struct platform_driver sierra_tcxo_clk_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table=swi_tcxo_clk_dt_match,
    },

    .probe = sierra_tcxo_clk_probe,
    .remove = sierra_tcxo_clk_remove,
};

static int __init sierra_tcxo_clk_init(void)
{
  return platform_driver_register(&sierra_tcxo_clk_driver);
}

static void __exit sierra_tcxo_clk_exit(void)
{
  platform_driver_unregister(&sierra_tcxo_clk_driver);
}

module_init(sierra_tcxo_clk_init);
module_exit(sierra_tcxo_clk_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sierra_tcxo_clk driver");
