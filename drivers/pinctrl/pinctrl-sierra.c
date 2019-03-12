/************
 *
 * $Id$
 *
 * Filename:  pinctrl-sierra.c
 *
 * Purpose:   Complete probe and remove function for bitmask ownership.
 *
 * Copyright: (c) 2019 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/slab.h>

#include "qcom/pinctrl-msm.h"
#include <linux/sierra_bsudefs.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <../gpio/gpiolib.h>

int sierra_pinctrl_probe(struct platform_device *pdev)
{
	struct gpio_chip	*chip = msm_pinctrl_get_gpio_chip(pdev);
	uint8_t			bshwtype = -1;
	uint8_t			bshwrev = -1;
	char			*bshwtypestr;
	struct device_node*	np = chip->of_node;
	int			i;
	int			nmap;
	int			igpio;
	int			ibit;
	char			*gpiobitmapstr[2] = { "gpio-bit-map", "gpio-bit-map-rev4" };

	bshwtype = sierra_smem_get_factory_mode() == 1 ? 0xFF : bsgethwtype();
	bshwrev = bsgethwrev();

	bshwtypestr = gpiobitmapstr[1];
	switch (bshwtype) {
		case BSAR7592:
				if (bshwrev < BSHWREV2)
					bshwtypestr = gpiobitmapstr[0];
				break;
		case BSAR7594:
				if (bshwrev < BSHWREV4)
					bshwtypestr = gpiobitmapstr[0];
				break;
		case BSAR7596:
				if (bshwrev < BSHWREV3)
					bshwtypestr = gpiobitmapstr[0];
				break;
		case BSAR7598:
				if (bshwrev < BSHWREV2)
					bshwtypestr = gpiobitmapstr[0];
				break;
		case BSAR7692:
		case BSAR7694:
		case BSAR7696:
		case BSAR7698:
				bshwtypestr = gpiobitmapstr[0];
				break;
		default:
				dev_err(&pdev->dev,
					"Unsupported product type: %d. Back down to feature \"%s\"\n",
					bshwtype, bshwtypestr);
				break;
	}
	dev_info(&pdev->dev, "Feature \"%s\" (%d)\n", bshwtypestr, bshwtype);
	chip->mask = kzalloc((ARCH_NR_GPIOS + sizeof(u64)*8 - 1) / (sizeof(u64)*8), GFP_KERNEL);
	if (!chip->mask)
		return -ENOMEM;

	for (igpio = 0; igpio < chip->ngpio; igpio++) {
		struct gpio_desc	*desc;

		desc = gpio_to_desc(igpio);
		if (desc)
			desc->bit_in_mask = -1;
	}
	nmap = of_property_count_u32_elems(np, bshwtypestr);
	dev_err(&pdev->dev, "%s = %d\n", bshwtypestr, nmap);
	for (i = 0; i < nmap; i += 2) {
		struct gpio_desc	*desc;

		/*
		 * Read GPIO num and bit to test
		 */
		of_property_read_u32_index(np, bshwtypestr, i, &igpio);
		ibit = -1;
		of_property_read_u32_index(np, bshwtypestr, i+1, &ibit);
		desc = gpio_to_desc(igpio);
		if (desc && (ibit >= 0) && (ibit < chip->ngpio))
			desc->bit_in_mask = ibit;
		dev_dbg(&pdev->dev, "%s = <%d %d>\n", bshwtypestr, igpio, ibit);
	}
	igpio = -1;
	if (0 == (nmap = of_property_read_u32_index(np, "gpio-RI", 0, &igpio))) {
		struct gpio_desc	*desc;

		desc = gpio_to_desc(igpio);
		if (desc) {
			set_bit(FLAG_RING_INDIC, &desc->flags);
			dev_info(&pdev->dev, "RI is GPIO %d\n", igpio);
		}
		else
			dev_err(&pdev->dev, "Invalid GPIO %d for RI\n", igpio);
	}
	if (1 == bsgetgpioflag(&(chip->mask[0]), &(chip->mask[1]))) {
		/*
		 * AR set the bit to 1 when the GPIO if not usable or allocatable to APP core
		 * But mask and mask_v2 show 1 if the GPIO is usable by APP core
		 */
		chip->mask[0] = ~chip->mask[0];
		chip->mask[1] = ~chip->mask[1];
		chip->max_bit = chip->ngpio - chip->base - 1;
		chip->bitmask_valid = true;
		dev_info(&pdev->dev, "Cores GPIO mask 0x%llx 0x%llx, maxbit = %d\n",
			chip->mask[0], chip->mask[1], chip->max_bit);
	}
	else
		dev_err(&pdev->dev, "Failed to get GPIO shared mask\n");

	return 0;
}

int sierra_pinctrl_remove(struct platform_device *pdev)
{
	struct gpio_chip	*chip = msm_pinctrl_get_gpio_chip(pdev);

	if (chip->bitmask_valid) {
		kfree(chip->mask);
		chip->mask = NULL;
		chip->bitmask_valid = false;
	}
	return 0;
}
