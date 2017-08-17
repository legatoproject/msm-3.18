/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include "board-dt.h"

#ifdef CONFIG_WL_TI
#include <linux/wl12xx.h>
#include <linux/gpio.h>
#endif

static const char *mdm9607_dt_match[] __initconst = {
	"qcom,mdm9607",
	NULL
};

static void __init mdm9607_init(void)
{
	board_dt_populate(NULL);
}

#ifdef CONFIG_WL_TI
#define MSM_WIFI_IRQ_GPIO       79
#endif

#ifdef CONFIG_WL_TI
static void __init mdm9607_wl18xx_init(void)
{
	struct wl12xx_platform_data msm_wl12xx_pdata;
	int ret;

	memset(&msm_wl12xx_pdata, 0, sizeof(msm_wl12xx_pdata));

	msm_wl12xx_pdata.irq = gpio_to_irq(MSM_WIFI_IRQ_GPIO);
	printk(KERN_ERR "wl12xx GPIO: %d\n", msm_wl12xx_pdata.irq);
	if (msm_wl12xx_pdata.irq < 0)
		goto fail;

	msm_wl12xx_pdata.use_eeprom = true;
	msm_wl12xx_pdata.board_ref_clock = WL12XX_REFCLOCK_38;
	msm_wl12xx_pdata.board_tcxo_clock = 0;

	ret = wl12xx_set_platform_data(&msm_wl12xx_pdata);
	if (ret < 0)
		goto fail;

	pr_info("wl18xx board initialization done\n");
	return;

fail:
	pr_err("%s: wl12xx board initialisation failed\n", __func__);
}
#endif


DT_MACHINE_START(MDM9607_DT,
	"Qualcomm Technologies, Inc. MDM 9607 (Flattened Device Tree)")
	.init_machine	= mdm9607_init,
	.dt_compat	= mdm9607_dt_match,
#ifdef CONFIG_WL_TI
	.init_late      = mdm9607_wl18xx_init,
#endif
MACHINE_END
