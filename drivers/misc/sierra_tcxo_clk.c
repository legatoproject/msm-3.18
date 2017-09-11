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

#define DRIVER_NAME "sierra_tcxo_clk"

struct swi_tcxo {
  uint32_t enable;
  struct kobject *tcxo_obj;
  struct attribute_group *attr_group;
  struct device *dev;
  struct clk *div_clk;   /* interface clock */
};
struct swi_tcxo *data = NULL;

static ssize_t sierra_tcxo_show(struct kobject *kobj,
  struct kobj_attribute *attr,
  char *buf);
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
  char *buf)
{
  uint8_t enabled;
  enabled = data->enable;

  return sprintf(buf, "%u\n", enabled);
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
  
  if(!data->enable && enabled)
  {
    pr_debug("enable div clk\n");
    ret = clk_prepare_enable(data->div_clk);
    if(ret)
    {
      pr_err("%s: unable to enable tcxo clk\n", __func__);
      return ret;
    }
  }
  else if (data->enable && !enabled)
  {
    pr_debug("disable div clk\n");
    clk_disable_unprepare(data->div_clk);
  }

  data->enable = enabled;
  
  return count;
}

static int sierra_tcxo_clk_probe(struct platform_device *pdev)
{
  int ret = 0;

  pr_debug("sierra_tcxo_clk probe\n");
  dev_info(&pdev->dev, "sierra_tcxo_clk probe\n");

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

  data->div_clk = devm_clk_get(&pdev->dev, "tcxo_clk");
  if (IS_ERR(data->div_clk)) {
    pr_err("%s: unable to get iface_clk\n", __func__);
    goto exit_free;
  }

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
