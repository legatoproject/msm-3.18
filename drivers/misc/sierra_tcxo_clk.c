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
  u8 enable;
  struct device *dev;
  struct dentry *dent_swi;
  struct dentry *debugfs_tcxo;
  struct clk *div_clk;   /* interface clock */
};
static struct swi_tcxo_debugfs{
  const char *name;
  mode_t mode;
  struct swi_tcxo *dd;
} debugfs_tcxo = {
  .name = "tcxo_en",
  .mode = S_IRUGO | S_IWUSR,
  .dd = NULL,
};
static int tcxo_debugfs_get(void *data, u64 *val)
{
  int rc = 0;
  struct swi_tcxo_debugfs *debugfs_tcxo = (struct swi_tcxo_debugfs *)data;
  struct swi_tcxo *dd = debugfs_tcxo->dd;

  *val = dd->enable;

  return rc;
}

static int tcxo_debugfs_set(void *data, u64 val)
{
  int rc = 0;
  struct swi_tcxo_debugfs *debugfs_tcxo = (struct swi_tcxo_debugfs *)data;
  struct swi_tcxo *dd = debugfs_tcxo->dd;

  if (val)
  {
    pr_debug("enable div clk\n");
    clk_prepare_enable(dd->div_clk);
    dd->enable = 1;
  }
  else
  {
    pr_debug("disable div clk\n");
    clk_disable_unprepare(dd->div_clk);
    dd->enable = 0;
  }

  return rc;
}

DEFINE_SIMPLE_ATTRIBUTE(tcxo_fops, tcxo_debugfs_get, tcxo_debugfs_set, "%llu\n");

static int sierra_debugfs_create(struct swi_tcxo *dd)
{
  dd->dent_swi = debugfs_create_dir("sierra", NULL);
  if (dd->dent_swi) {
    debugfs_tcxo.dd = dd;
    dd->debugfs_tcxo =
         debugfs_create_file(
             debugfs_tcxo.name,
             debugfs_tcxo.mode,
             dd->dent_swi,
             &debugfs_tcxo,
             &tcxo_fops);
  }

  return 0;
}

static int sierra_tcxo_clk_probe(struct platform_device *pdev)
{
  int ret = 0;
  struct swi_tcxo *data;

  pr_debug("sierra_tcxo_clk probe\n");

  dev_info(&pdev->dev, "sierra_tcxo_clk probe\n");

  data = kzalloc(sizeof(struct swi_tcxo*), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto exit_free;
  }
  data->enable = 0;
  data->dev = &pdev->dev;

  data->div_clk = devm_clk_get(&pdev->dev, "div_clk");
  if (IS_ERR(data->div_clk)) {
    pr_err("%s: unable to get iface_clk\n", __func__);
    goto exit_free;
  }

  dev_set_drvdata(&pdev->dev, data);

  /* /sys/kernel/debug/sierra_tcxo */
  ret = sierra_debugfs_create(data);
  if (ret) {
    pr_err("%s: failed to creat sierra debugfs, error code is%d\n", __func__,
      ret);
    goto exit_free;
  }

  return 0;

  exit_free:
    kfree(data);
    dev_set_drvdata(&pdev->dev, NULL);
  exit:
    pr_err("%s: failed to probe sierra tcxo clock debugfs node\n", __func__);
    return ret;
}

static int sierra_tcxo_clk_remove(struct platform_device *pdev)
{
  pr_info("sierra_tcxo_clk_remove");
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
