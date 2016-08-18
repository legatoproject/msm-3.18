/*
 * gpio.h  --  GPIO Driver for Sierra Wireless WP76xx MCU
 *
 * adapted from:
 * gpio.h  --  GPIO Driver for Wolfson WM8350 PMIC
 *
 * Copyright (c) 2016 Sierra Wireless, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_SWIMCU_GPIO_H_
#define __LINUX_MFD_SWIMCU_GPIO_H_

#include <linux/platform_device.h>

#define SWIMCU_GPIO_NOOP	0
#define SWIMCU_GPIO_GET_DIR	1
#define SWIMCU_GPIO_SET_DIR	2
#define SWIMCU_GPIO_GET_VAL	3
#define SWIMCU_GPIO_SET_VAL	4
#define SWIMCU_GPIO_SET_PULL	5
#define SWIMCU_GPIO_SET_EDGE	6

/*
 * GPIO Port index.
 * Supported GPIOs ordered according to external GPIO mapping.
 * PTA12 and PTB1 are reserved for ADC.
 */
enum swimcu_gpio_index
{
	SWIMCU_GPIO_FIRST = 0,
	SWIMCU_GPIO_PTA4 = SWIMCU_GPIO_FIRST, /* GPIOU1 */
	SWIMCU_GPIO_PTA3, /* GPIOU2 */
	SWIMCU_GPIO_PTA0, /* GPIOU3 */
	SWIMCU_GPIO_PTA2, /* GPIOU4 */
	SWIMCU_GPIO_PTB0, /* GPIOU5 */
	SWIMCU_GPIO_PTA7, /* GPIOU6 */
	SWIMCU_GPIO_PTA6, /* GPIOU7 */
	SWIMCU_GPIO_PTA5, /* GPIO41 */
	SWIMCU_GPIO_LAST = SWIMCU_GPIO_PTA5,
	SWIMCU_NUM_GPIO = SWIMCU_GPIO_LAST+1,
	SWIMCU_GPIO_INVALID = SWIMCU_NUM_GPIO,
};

enum swimcu_gpio_irq_index
{
	SWIMCU_GPIO_PTA0_IRQ = 0, /* GPIOU3 */
	SWIMCU_GPIO_PTB0_IRQ, /* GPIOU5 */
	SWIMCU_GPIO_PTA7_IRQ, /* GPIOU6 */
	SWIMCU_NUM_GPIO_IRQ
};

/*
 * MCU GPIO map port/pin to gpio index.
 */
enum swimcu_gpio_index swimcu_get_gpio_from_port_pin(int port, int pin);

struct swimcu;

struct swimcu_gpio {
	struct platform_device *pdev;
};

void swimcu_gpio_callback(struct swimcu *swimcu, int port, int pin, int level);

int swimcu_gpio_open(struct swimcu *swimcu, int gpio);

int swimcu_gpio_close(struct swimcu *swimcu, int gpio);

void swimcu_gpio_refresh( struct swimcu *swimcu );

void swimcu_gpio_retrieve( struct swimcu *swimcu );

int swimcu_gpio_get(struct swimcu *swimcu, int action, int gpio, int *value);

int swimcu_gpio_set(struct swimcu *swimcu, int action, int gpio, int value);

#endif
