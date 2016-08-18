/*
 * swimcu-gpio.c  --  Device access for Sierra Wireless WP76xx MCU.
 *
 * adapted from:
 * wm8350-core.c  --  Device access for Wolfson WM8350
 *
 * Copyright (c) 2016 Sierra Wireless, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include<linux/string.h>

#include <linux/mfd/swimcu/core.h>
#include <linux/mfd/swimcu/gpio.h>
#include <linux/mfd/swimcu/mciprotocol.h>
#include <linux/mfd/swimcu/mcidefs.h>

struct gpio_in_cfg {
	bool pe;
	enum mci_mcu_pin_pull_select_e ps;
};

struct gpio_out_cfg {
	enum mci_mcu_pin_level_e level;
};

static struct {
	enum mci_mcu_pin_function_e mux;
	enum mci_pin_irqc_type_e irqc;
	enum mci_mcu_pin_direction_e dir;
	union {
		struct gpio_in_cfg in;
		struct gpio_out_cfg out;
	} parm;
} gpio_cfg[] = {
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
	{ MCI_MCU_PIN_FUNCTION_DISABLED, MCI_PIN_IRQ_DISABLED },
};

/* WP76xx GPIOs provided by MCU occur in the range 34 - 41
 * external gpio number is the offset from GPIO34 */
static const struct {
	int port;
	int pin;
	bool irqc_support;
} gpio_map[] = {
	{ 0, 4, false }, /* GPIO U1 = PTA4 */
	{ 0, 3, false }, /* GPIO U2 = PTA3 */
	{ 0, 0, true  }, /* GPIO U3 = PTA0 */
	{ 0, 2, false }, /* GPIO U4 = PTA2 */
	{ 1, 0, true  }, /* GPIO U5 = PTB0 */
	{ 0, 7, true  }, /* GPIO U6 = PTA7 */
	{ 0, 6, false }, /* GPIO U7 = PTA6 */
	{ 0, 5, false }  /* GPIO U8 = PTA5 */
};

/************
 *
 * Name:     swimcu_get_gpio_from_port_pin
 *
 * Purpose:  return index into gpio_map given port and pin
 *
 * Parms:    port - 0 or 1 corresponding to PTA or PTB
 *           pin - 0 to 7 for pin # on that port
 *
 * Return:   gpio index - 0 to 7 on success
 *           8 on failure
 *
 * Abort:    none
 *
 ************/
enum swimcu_gpio_index swimcu_get_gpio_from_port_pin(int port, int pin)
{
	enum swimcu_gpio_index gpio;

	for (gpio = SWIMCU_GPIO_FIRST; gpio <= SWIMCU_GPIO_LAST; gpio++) {
		if((port == gpio_map[gpio].port) && (pin == gpio_map[gpio].pin))
			break;
	}
	return gpio;
}

/************
 *
 * Name:     swimcu_gpio_callback
 *
 * Purpose:  callback for gpio irq event (stub for now)
 *
 * Parms:    swimcu - device data struct
 *           port - 0 or 1 corresponding to PTA or PTB
 *           pin - 0 to 7 for pin # on that port
 *           level - 0 or 1
 *
 * Return:   nothing
 *
 * Abort:    none
 *
 ************/
void swimcu_gpio_callback(struct swimcu *swimcu, int port, int pin, int level)
{
	enum swimcu_gpio_index gpio = swimcu_get_gpio_from_port_pin(port, pin);

	swimcu_log(GPIO, "%s: gpio %d level = %d\n", __func__, gpio, level);
}

/************
 *
 * Name:     swimcu_gpio_set_trigger
 *
 * Purpose:  set the irq trigger for a particular gpio
 *
 * Parms:    gpio index - 0 to 7
 *           irq control type (rising, falling, etc).
 *
 * Return:   0 if success
 *           -ERRNO otherwise
 *
 * Abort:    none
 *
 ************/
int swimcu_gpio_set_trigger(int gpio, enum mci_pin_irqc_type_e irqc_type)
{
	if (gpio < SWIMCU_NUM_GPIO) {
		if (!(gpio_map[gpio].irqc_support) ||
		     /* currently exported as output */
		     ((gpio_cfg[gpio].mux == MCI_MCU_PIN_FUNCTION_GPIO) &&
		      (gpio_cfg[gpio].dir == MCI_MCU_PIN_DIRECTION_OUTPUT))) {
			swimcu_log(GPIO, "%s: failed gpio %d\n", __func__, gpio);
			return -EPERM;
		}
		else {
			gpio_cfg[gpio].irqc = irqc_type;
			swimcu_log(GPIO, "%s: gpio %d irq = %d\n", __func__, gpio, irqc_type);
			return 0;
		}
	}
	return -EINVAL;
}

/************
 *
 * Name:     swimcu_gpio_get_trigger
 *
 * Purpose:  get the irq trigger for a particular gpio
 *
 * Parms:    gpio index - 0 to 7
 *
 * Return:   irq control type (rising, falling, etc).
 *
 * Abort:    none
 *
 ************/
enum mci_pin_irqc_type_e swimcu_gpio_get_trigger(int gpio)
{
	enum mci_pin_irqc_type_e irqc_type = MCI_PIN_IRQ_DISABLED;

	if (gpio < SWIMCU_NUM_GPIO) {
		irqc_type = gpio_cfg[gpio].irqc;
		swimcu_log(GPIO, "%s: gpio %d irq = %d\n", __func__, gpio, irqc_type);
	}
	return irqc_type;
}

/************
 *
 * Name:     swimcu_gpio_get
 *
 * Purpose:  get the gpio property
 *
 * Parms:    swimcu - device data struct
 *           action - get value is only option
 *           gpio index - 0 to 7
 *           value ptr - returns value
 *
 * Return:   0 if success
 *           -ERRNO otherwise
 *
 * Abort:    none
 *
 * Notes:    called from gpio driver
 *
 ************/
int swimcu_gpio_get(struct swimcu *swimcu, int action, int gpio, int *value)
{
	struct mci_mcu_pin_state_s pin_state;
	int ret = 0;

	memset((void*) &pin_state, 0, sizeof(struct mci_mcu_pin_state_s));

	switch (action) {
		case SWIMCU_GPIO_GET_VAL:
			swimcu_log(GPIO, "%s: GET VAL gpio%d\n", __func__, gpio);
			if (MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
				swimcu_pin_states_get(swimcu, gpio_map[gpio].port, gpio_map[gpio].pin, &pin_state))
				ret = -EIO;
			else if (NULL != value)
				*value = (pin_state.level == MCI_MCU_PIN_LEVEL_LOW) ? 0 : 1;
			break;

		default:
			ret = -EINVAL;
			pr_err ("%s: unknown %d\n", __func__, gpio);
	}
	return ret;
}

/************
 *
 * Name:     swimcu_gpio_set
 *
 * Purpose:  set the gpio property
 *
 * Parms:    swimcu - device data struct
 *           action - property to set
 *           gpio index - 0 to 7
 *           value - value to set (depends on action)
 *
 * Return:   0 if success
 *           -ERRNO otherwise
 *
 * Abort:    none
 *
 * Notes:    called from gpio driver
 *
 ************/
int swimcu_gpio_set(struct swimcu *swimcu, int action, int gpio, int value)
{
	struct mci_mcu_pin_state_s pin_state;
	enum mci_mcu_pin_direction_e direction;
	int ret = 0;

	memset((void*) &pin_state, 0, sizeof(struct mci_mcu_pin_state_s));

	pin_state.mux = gpio_cfg[gpio].mux;
	pin_state.dir = gpio_cfg[gpio].dir;
	if (MCI_MCU_PIN_DIRECTION_INPUT == pin_state.dir) {
		pin_state.params.input.pe = gpio_cfg[gpio].parm.in.pe;
		pin_state.params.input.ps = gpio_cfg[gpio].parm.in.ps;
		pin_state.params.input.pfe = false;
		pin_state.params.input.irqc_type = gpio_cfg[gpio].irqc;
	}
	else {
		pin_state.params.output.sre = MCI_MCU_PIN_SLEW_RATE_FAST;
		pin_state.params.output.dse = MCI_MCU_PIN_DRIVE_STRENGTH_HIGH;
		pin_state.level = gpio_cfg[gpio].parm.out.level;
	}

	switch (action) {
		case SWIMCU_GPIO_SET_DIR:
			swimcu_log(GPIO, "%s: SET DIR %d, port %d, pin%d\n", __func__, value, gpio_map[gpio].port, gpio_map[gpio].pin);
			/* value: 0-input, 1-output,low, 2-output,high */
			direction = (value > 0) ? MCI_MCU_PIN_DIRECTION_OUTPUT : MCI_MCU_PIN_DIRECTION_INPUT;
			if ( pin_state.dir != direction) {
				swimcu_log(GPIO, "%s: change DIR %d to %d\n", __func__, pin_state.dir, value);
			}
			else {
				swimcu_log(GPIO, "%s: no change DIR %d\n", __func__, value);
			}

			gpio_cfg[gpio].dir = pin_state.dir = direction;
			if (MCI_MCU_PIN_DIRECTION_INPUT == direction) {
				gpio_cfg[gpio].parm.in.pe = pin_state.params.input.pe = false;
				pin_state.params.input.pfe = false;
				pin_state.params.input.irqc_type = gpio_cfg[gpio].irqc;
			}
			else {
				pin_state.params.output.sre = MCI_MCU_PIN_SLEW_RATE_FAST;
				pin_state.params.output.dse = MCI_MCU_PIN_DRIVE_STRENGTH_HIGH;
				gpio_cfg[gpio].parm.out.level = pin_state.level = (value > 1) ?
					MCI_MCU_PIN_LEVEL_HIGH : MCI_MCU_PIN_LEVEL_LOW;
			}

			break;

		case SWIMCU_GPIO_SET_VAL:
			if ( pin_state.dir != MCI_MCU_PIN_DIRECTION_OUTPUT ) {
				pr_err ("%s: VAL %d (is input, illegal operation)\n", __func__, value);
				ret = -EPERM;
			}
			else if ( pin_state.level != value ) {
				swimcu_log(GPIO, "%s: change VAL %d to %d\n", __func__, pin_state.level, value);
				gpio_cfg[gpio].parm.out.level = pin_state.level =
					(enum mci_mcu_pin_level_e) value;
			}
			else {
				swimcu_log(GPIO, "%s: no change VAL %d\n", __func__, value);
			}

			break;

		case SWIMCU_GPIO_SET_PULL:
			if ( pin_state.dir != MCI_MCU_PIN_DIRECTION_INPUT ) {
				pr_err ("%s: PULL %d (is out, illegal operation)\n", __func__, value);
				ret = -EPERM;
			}
			else if ( !pin_state.params.input.pe ||
				  pin_state.params.input.ps != (enum mci_mcu_pin_pull_select_e) value ) {
				gpio_cfg[gpio].parm.in.ps = pin_state.params.input.ps = value ?
					MCI_MCU_PIN_PULL_UP : MCI_MCU_PIN_PULL_DOWN;
				if (pin_state.params.input.pe)
					swimcu_log(GPIO, "%s: change PULL %d to %d\n", __func__, pin_state.level, value);
				else
					swimcu_log(GPIO, "%s: change PULL OFF to %d\n", __func__, value);
				gpio_cfg[gpio].parm.in.pe = pin_state.params.input.pe = true;
			}
			else {
				swimcu_log(GPIO, "%s: no change PULL %d\n", __func__, value);
			}

			break;

		case SWIMCU_GPIO_NOOP:
			/* no change, just refresh MCU config with last settings */
			swimcu_log(GPIO, "%s: set %d\n", __func__, gpio);
			break;

		default:
			ret = -EINVAL;
			pr_err ("%s: unknown %d\n", __func__, value);

	}

	if (0 == ret) {
		if (MCI_PROTOCOL_STATUS_CODE_SUCCESS != swimcu_pin_config_set( swimcu, gpio_map[gpio].port, gpio_map[gpio].pin, &pin_state ))
			ret = -EIO;
	}

	return ret;
}

/************
 *
 * Name:     swimcu_gpio_open
 *
 * Purpose:  initialize MCU pin as gpio
 *
 * Parms:    swimcu - device data struct
 *           gpio index - 0 to 7
 *
 * Return:   0 if success
 *           -ERRNO otherwise
 *
 * Abort:    none
 *
 * Notes:    called from gpio driver on export
 *
 ************/
int swimcu_gpio_open( struct swimcu *swimcu, int gpio )
{
	if (gpio > SWIMCU_GPIO_LAST) {
		return -EINVAL;
	}

	if (MCI_MCU_PIN_FUNCTION_GPIO != gpio_cfg[gpio].mux) {
		gpio_cfg[gpio].mux = MCI_MCU_PIN_FUNCTION_GPIO;
		gpio_cfg[gpio].dir = MCI_MCU_PIN_DIRECTION_INPUT;
		gpio_cfg[gpio].parm.in.pe = false;
		gpio_cfg[gpio].parm.in.ps = MCI_MCU_PIN_PULL_DOWN;
		/* note: irqc not initialized here. it is set independently from boot_source */
		swimcu_log(GPIO, "%s: gpio%d init\n", __func__, gpio);
	}
	else {
		swimcu_log(GPIO, "%s: gpio%d dir %d\n", __func__, gpio, gpio_cfg[gpio].dir);
	}

	return swimcu_gpio_set(swimcu, SWIMCU_GPIO_NOOP, gpio, 0);
}

/************
 *
 * Name:     swimcu_gpio_refresh
 *
 * Purpose:  refresh MCU gpios with last settings
 *
 * Parms:    swimcu - device data struct
 *
 * Return:   nothing
 *
 * Abort:    none
 *
 * Notes:    called on MCU reset
 *
 ************/
void swimcu_gpio_refresh( struct swimcu *swimcu )
{
	int gpio;

	for( gpio = SWIMCU_GPIO_FIRST; gpio <= SWIMCU_GPIO_LAST; gpio++ ) {
		if (MCI_MCU_PIN_FUNCTION_DISABLED != gpio_cfg[gpio].mux) {
			swimcu_gpio_set(swimcu, SWIMCU_GPIO_NOOP, gpio, 0);
		}
	}

	swimcu_log(GPIO, "%s\n", __func__);
}

/************
 *
 * Name:     swimcu_gpio_retrieve
 *
 * Purpose:  retrieve current gpio settings from MCU
 *
 * Parms:    swimcu - device data struct
 *
 * Return:   nothing
 *
 * Abort:    none
 *
 * Notes:    called once on device init
 *
 ************/
void swimcu_gpio_retrieve( struct swimcu *swimcu )
{
	int gpio;
	struct mci_mcu_pin_state_s pin_state;
	int fail_cnt = 0;

	for( gpio = SWIMCU_GPIO_FIRST; gpio <= SWIMCU_GPIO_LAST; gpio++ ) {
		if (MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
			swimcu_pin_states_get(swimcu, gpio_map[gpio].port, gpio_map[gpio].pin, &pin_state)) {
			gpio_cfg[gpio].mux = MCI_MCU_PIN_FUNCTION_DISABLED;
			fail_cnt++;
		}
		else {
			gpio_cfg[gpio].mux = pin_state.mux;
			gpio_cfg[gpio].dir = pin_state.dir;
			if (MCI_MCU_PIN_DIRECTION_INPUT == pin_state.dir) {
				gpio_cfg[gpio].parm.in.pe =  pin_state.params.input.pe;
				gpio_cfg[gpio].parm.in.ps = pin_state.params.input.ps;
				gpio_cfg[gpio].irqc = pin_state.params.input.irqc_type;
			}
			else {
				gpio_cfg[gpio].parm.out.level = pin_state.level;
				gpio_cfg[gpio].irqc = MCI_PIN_IRQ_DISABLED;
			}
		}
	}

	swimcu_log(INIT, "%s %d\n", __func__, fail_cnt);
}

/************
 *
 * Name:     swimcu_gpio_close
 *
 * Purpose:  return MCU pin to uninitialized
 *
 * Parms:    swimcu - device data struct
 *           gpio index - 0 to 7
 *
 * Return:   0 if success
 *           -ERRNO otherwise
 *
 * Abort:    none
 *
 * Notes:    called from gpio driver on unexport
 *
 ************/
int swimcu_gpio_close( struct swimcu *swimcu, int gpio )
{
	if (gpio > SWIMCU_GPIO_LAST) {
		return -EINVAL;
	}

	gpio_cfg[gpio].mux = MCI_MCU_PIN_FUNCTION_DISABLED;

	swimcu_log(GPIO, "%s: gpio %d\n", __func__, gpio);

	return swimcu_gpio_set(swimcu, SWIMCU_GPIO_NOOP, gpio, 0);
}

