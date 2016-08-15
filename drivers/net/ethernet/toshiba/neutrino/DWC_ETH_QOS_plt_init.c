/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

/*!@file: DWC_ETH_QOS_plt_init.c
 * @brief: Driver functions.
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pm.h>
#include <linux/pm_wakeup.h>
#include <linux/sched.h>
#include <linux/pm_qos.h>
#include <linux/pm_runtime.h>
#include <linux/esoc_client.h>
#include <linux/pinctrl/consumer.h>
#include "DWC_ETH_QOS_yheader.h"

#ifdef CONFIG_PCI_MSM
#include <linux/msm_pcie.h>
#else
#include <mach/msm_pcie.h>
#endif

#define NTN_VREG_CORE_NAME 	"vdd-ntn-core"
#define NTN_VREG_PHY_NAME 	"vdd-ntn-phy"
#define NTN_VREG_HSIC_NAME 	"vdd-ntn-hsic"
#define NTN_VREG_PCI_NAME 	"vdd-ntn-pci"
#define NTN_VREG_IO_NAME 	"vdd-ntn-io"
#define NTN_VREG_CORE_MIN	1100000
#define NTN_VREG_CORE_MAX	1100000
#define NTN_VREG_PHY_MIN	2500000
#define NTN_VREG_PHY_MAX	2500000
#define NTN_VREG_HSIC_MIN	1200000
#define NTN_VREG_HSIC_MAX	1200000
#define NTN_VREG_PCI_MIN	1800000
#define NTN_VREG_PCI_MAX	1800000
#define NTN_VREG_IO_MIN		1800000
#define NTN_VREG_IO_MAX		1800000
#define NTN_RESET_GPIO_NAME	"ntn-rst-gpio"

extern int DWC_ETH_QOS_init_module(void);
extern void DWC_ETH_QOS_exit_module(void);

extern BOOL disable_platform_init;

static int DWC_ETH_QOS_pcie_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int ntn_rst_delay=100,rc_num=1;
	struct device *idev = &pdev->dev;
	struct regulator *ntn_reg = NULL;
	int gpio_num=-1;

	NMSGPR_ALERT( "Entry:%s.\n", __func__);

	/* Neutrino board init */

	/* Configure power rails Neutrino*/

	if (of_get_property(idev->of_node,NTN_VREG_HSIC_NAME"-supply", NULL))
	{
		ntn_reg = regulator_get(idev, NTN_VREG_HSIC_NAME);
		if (IS_ERR(ntn_reg))
		{
			ret = PTR_ERR(ntn_reg);

			if (ret == -EPROBE_DEFER)
			{
				NMSGPR_ALERT( "%s: ntn_reg_hsic probe deferred!\n", __func__);
			}
			else
			{
				NMSGPR_ALERT( "%s: Get ntn_reg_hsic failed!\n", __func__);
			}
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			ret = regulator_set_voltage(ntn_reg, NTN_VREG_HSIC_MIN, NTN_VREG_HSIC_MAX);
			if (ret)
			{
				NMSGPR_ALERT( "%s: Set ntn_reg_hsic failed!\n", __func__);
				goto DWC_ETH_QOS_pcie_probe_exit;
			}
			else
			{
				ret = regulator_enable(ntn_reg);
				if (ret)
				{
					NMSGPR_ALERT( "%s: Enable ntn_reg_hsic failed!\n", __func__);
					goto DWC_ETH_QOS_pcie_probe_exit;
				}
			}
		}
	}
	else
	{
		NMSGPR_ALERT( "%s: power supply %s not found!\n", __func__,NTN_VREG_HSIC_NAME);
	}

	if (of_get_property(idev->of_node,NTN_VREG_PCI_NAME"-supply", NULL))
	{
		ntn_reg = regulator_get(idev, NTN_VREG_PCI_NAME);
		if (IS_ERR(ntn_reg))
		{
			ret = PTR_ERR(ntn_reg);

			if (ret == -EPROBE_DEFER)
			{
				NMSGPR_ALERT( "%s: ntn_reg_pci probe deferred!\n", __func__);
			}
			else
			{
				NMSGPR_ALERT( "%s: Get ntn_reg_pci failed!\n", __func__);
			}
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			ret = regulator_set_voltage(ntn_reg, NTN_VREG_PCI_MIN, NTN_VREG_PCI_MAX);
			if (ret)
			{
				NMSGPR_ALERT( "%s: Set ntn_reg_pci failed!\n", __func__);
				goto DWC_ETH_QOS_pcie_probe_exit;
			}
			else
			{
				ret = regulator_enable(ntn_reg);
				if (ret)
				{
					NMSGPR_ALERT( "%s: Enable ntn_reg_pci failed!\n", __func__);
					goto DWC_ETH_QOS_pcie_probe_exit;
				}
			}
		}
	}
	else
	{
		NMSGPR_ALERT( "%s: power supply %s not found!\n", __func__,NTN_VREG_PCI_NAME);
	}

	if (of_get_property(idev->of_node,NTN_VREG_IO_NAME"-supply", NULL))
	{
		ntn_reg = regulator_get(idev, NTN_VREG_IO_NAME);
		if (IS_ERR(ntn_reg))
		{
			ret = PTR_ERR(ntn_reg);

			if (ret == -EPROBE_DEFER)
			{
				NMSGPR_ALERT( "%s: ntn_reg_io probe deferred!\n", __func__);
			}
			else
			{
				NMSGPR_ALERT( "%s: Get ntn_reg_io failed!\n", __func__);
			}
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			ret = regulator_set_voltage(ntn_reg, NTN_VREG_IO_MIN, NTN_VREG_IO_MAX);
			if (ret)
			{
				NMSGPR_ALERT( "%s: Set ntn_reg_io failed!\n", __func__);
				goto DWC_ETH_QOS_pcie_probe_exit;
			}
			else
			{
				ret = regulator_enable(ntn_reg);
				if (ret)
				{
					NMSGPR_ALERT( "%s: Enable ntn_reg_io failed!\n", __func__);
					goto DWC_ETH_QOS_pcie_probe_exit;
				}
			}
		}
	}
	else
	{
		NMSGPR_ALERT( "%s: power supply %s not found!\n", __func__,NTN_VREG_IO_NAME);
	}



	if (of_get_property(idev->of_node,NTN_VREG_CORE_NAME"-supply", NULL))
	{
		ntn_reg = regulator_get(idev, NTN_VREG_CORE_NAME);
		if (IS_ERR(ntn_reg))
		{
			ret = PTR_ERR(ntn_reg);

			if (ret == -EPROBE_DEFER)
			{
				NMSGPR_ALERT( "%s: ntn_reg_core probe deferred!\n", __func__);
			}
			else
			{
				NMSGPR_ALERT( "%s: Get ntn_reg_core failed!\n", __func__);
			}
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			ret = regulator_set_voltage(ntn_reg, NTN_VREG_CORE_MIN, NTN_VREG_CORE_MAX);
			if (ret)
			{
				NMSGPR_ALERT( "%s: Set ntn_reg_core failed!\n", __func__);
				goto DWC_ETH_QOS_pcie_probe_exit;
			}
			else
			{
				ret = regulator_enable(ntn_reg);
				if (ret)
				{
					NMSGPR_ALERT( "%s: Enable ntn_reg_core failed!\n", __func__);
					goto DWC_ETH_QOS_pcie_probe_exit;
				}
			}
		}
	}
	else
	{
		NMSGPR_ALERT( "%s: power supply %s not found!\n", __func__,NTN_VREG_CORE_NAME);
	}


	if (of_get_property(idev->of_node,NTN_VREG_PHY_NAME"-supply", NULL))
	{
		ntn_reg = regulator_get(idev, NTN_VREG_PHY_NAME);
		if (IS_ERR(ntn_reg))
		{
			ret = PTR_ERR(ntn_reg);

			if (ret == -EPROBE_DEFER)
			{
				NMSGPR_ALERT( "%s: ntn_reg_phy probe deferred!\n", __func__);
			}
			else
			{
				NMSGPR_ALERT( "%s: Get ntn_reg_phy failed!\n", __func__);
			}
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			ret = regulator_set_voltage(ntn_reg, NTN_VREG_PHY_MIN, NTN_VREG_PHY_MAX);
			if (ret)
			{
				NMSGPR_ALERT( "%s: Set ntn_reg_phy failed!\n", __func__);
				goto DWC_ETH_QOS_pcie_probe_exit;
			}
			else
			{
				ret = regulator_enable(ntn_reg);
				if (ret)
				{
					NMSGPR_ALERT( "%s: Enable ntn_reg_phy failed!\n", __func__);
					goto DWC_ETH_QOS_pcie_probe_exit;
				}
			}
		}
	}
	else
	{
		NMSGPR_ALERT( "%s: power supply %s not found!\n", __func__,NTN_VREG_PHY_NAME);
	}

	/* Configure GPIOs required for the Neutrino reset */
	if ( !of_find_property(idev->of_node, NTN_RESET_GPIO_NAME, NULL) )
	{
		NMSGPR_ALERT( "can't find gpio %s",NTN_RESET_GPIO_NAME);
		goto DWC_ETH_QOS_pcie_probe_exit;
	}

	gpio_num = ret = of_get_named_gpio(idev->of_node, NTN_RESET_GPIO_NAME, 0);
	if (ret >= 0)
	{
		NMSGPR_ALERT( "%s: Found %s %d\n",__func__, NTN_RESET_GPIO_NAME, gpio_num);

		ret = gpio_request(gpio_num, NTN_RESET_GPIO_NAME);
		if (ret)
		{
			NMSGPR_ALERT( "%s: Can't get GPIO %s, ret = %d\n", __func__, NTN_RESET_GPIO_NAME, gpio_num);
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			NMSGPR_ALERT( "%s: gpio_request successful for GPIO %s, ret = %d\n", __func__, NTN_RESET_GPIO_NAME, gpio_num);
		}

		ret = gpio_direction_output(gpio_num, 0x1);
		if (ret)
		{
			NMSGPR_ALERT( "%s: Can't set GPIO %s direction, ret = %d\n",__func__, NTN_RESET_GPIO_NAME, ret);
			gpio_free(gpio_num);
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else
		{
			NMSGPR_ALERT( "%s: gpio_direction_output suffessful for GPIO %s, ret = %d\n",__func__, NTN_RESET_GPIO_NAME, ret);
		}

		gpio_set_value(gpio_num, 0x1);
	}
	else
	{
		if (ret == -EPROBE_DEFER)
			NMSGPR_ALERT( "get NTN_RESET_GPIO_NAME probe defer\n");
		else
			NMSGPR_ALERT( "can't get gpio %s ret %d",NTN_RESET_GPIO_NAME, ret);
		goto DWC_ETH_QOS_pcie_probe_exit;
	}

	/* add delay for neutrino reset propagation*/
	ret = of_property_read_u32(idev->of_node, "qcom,ntn-rst-delay-msec", &ntn_rst_delay);
	if (ret) {
		NMSGPR_ALERT( "%s: Failed to find ntn_rst_delay value, hardcoding to 100 msec\n", __func__);
		ntn_rst_delay = 500;
	}
	else
	{
		NMSGPR_ALERT( "%s: ntn_rst_delay = %d msec\n", __func__,ntn_rst_delay);
	}
	msleep(ntn_rst_delay);

	/* issue PCIe enumeration */
	ret = of_property_read_u32(idev->of_node, "qcom,ntn-rc-num", &rc_num);
	if (ret) {
		NMSGPR_ALERT( "%s: Failed to find PCIe RC number!, RC=%d\n", __func__,rc_num);
		goto DWC_ETH_QOS_pcie_probe_exit;
	}
	else {
		NMSGPR_ALERT( "%s: Found PCIe RC number! %d\n", __func__,rc_num);
		ret = msm_pcie_enumerate(rc_num);
		if (ret) {
			NMSGPR_ALERT( "%s: Failed to enable PCIe RC%x!\n", __func__, rc_num);
			goto DWC_ETH_QOS_pcie_probe_exit;
		}
		else {
			NMSGPR_ALERT( "%s: PCIe enumeration for RC number %d successful!\n", __func__,rc_num);
		}
	}

	/* register Ethernet modules */
	DWC_ETH_QOS_init_module();

DWC_ETH_QOS_pcie_probe_exit:

	NMSGPR_ALERT( "Exit:%s.\n", __func__);
	return ret;
}

static int DWC_ETH_QOS_pcie_remove(struct platform_device *pdev)
{
	int ret = 0;
	NMSGPR_ALERT( "Entry:%s.\n", __func__);

	DWC_ETH_QOS_exit_module();

	NMSGPR_ALERT( "Exit:%s.\n", __func__);
	return ret;
}

static struct of_device_id DWC_ETH_QOS_pcie_match[] = {
	{	.compatible = "qcom,ntn_avb",
	},
	{}
};

static struct platform_driver DWC_ETH_QOS_pcie_driver = {
	.probe	= DWC_ETH_QOS_pcie_probe,
	.remove	= DWC_ETH_QOS_pcie_remove,
	.driver	= {
		.name		= "ntn_avb",
		.owner		= THIS_MODULE,
		.of_match_table	= DWC_ETH_QOS_pcie_match,
	},
};

static int __init DWC_ETH_QOS_pcie_init(void)
{
	int ret = 0;

        NMSGPR_ALERT("Entry:%s.\n", __func__);
        if (disable_platform_init) {
                DWC_ETH_QOS_init_module();
        } else {
                ret = platform_driver_register(&DWC_ETH_QOS_pcie_driver);
        }
        NMSGPR_ALERT( "Exit:%s.\n", __func__);
	return ret;
}

static void __exit DWC_ETH_QOS_pcie_exit(void)
{
	NMSGPR_ALERT( "Entry:%s.\n", __func__);
        if (disable_platform_init) {
                DWC_ETH_QOS_exit_module();
        } else {
                platform_driver_unregister(&DWC_ETH_QOS_pcie_driver);
        }
	NMSGPR_ALERT( "Exit:%s.\n", __func__);
}

module_init(DWC_ETH_QOS_pcie_init);
module_exit(DWC_ETH_QOS_pcie_exit);
