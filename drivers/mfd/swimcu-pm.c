/*
 * swimcu-pm.c  --  Device access for Sierra Wireless MCU power management.
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
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/kmod.h>
#include <linux/reboot.h>

#include <linux/gpio.h>

#include <linux/mfd/swimcu/core.h>
#include <linux/mfd/swimcu/gpio.h>
#include <linux/mfd/swimcu/pm.h>
#include <linux/mfd/swimcu/mciprotocol.h>
#include <linux/mfd/swimcu/mcidefs.h>
#include <mach/swimcu.h>

/* generate extra debug logs */
#ifdef SWIMCU_DEBUG
module_param_named(
	debug_mask, swimcu_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#endif

#define ADC_ATTR_SHOW(name)                                       	\
	static ssize_t pm_adc_##name##_attr_show(struct kobject *kobj,  \
		struct kobj_attribute *attr, char *buf)                 \
	{                                                               \
		unsigned int value = 0;                                 \
		enum swimcu_adc_index adc;                              \
		enum wusrc_index wi = find_wusrc_index_from_kobj(kobj); \
	                                                                \
		if (WUSRC_INVALID != wi) {                              \
			adc = wusrc_param[wi].id;                       \
			value = adc_trigger_config[adc].name;           \
		}                                                       \
		return scnprintf(buf, PAGE_SIZE, "%d\n", value);        \
	}

#define ADC_ATTR_STORE(name)                                      	\
	static ssize_t pm_adc_##name##_attr_store(struct kobject *kobj, \
		struct kobj_attribute *attr, const char *buf, size_t count) \
	{                                                               \
		unsigned int value = 0;                                 \
		enum swimcu_adc_index adc;                              \
		enum wusrc_index wi = find_wusrc_index_from_kobj(kobj); \
		int ret;						\
	                                                                \
		if (WUSRC_INVALID == wi) {                              \
			return -EINVAL; 			        \
		}                                                       \
		adc = wusrc_param[wi].id;				\
		ret = kstrtouint(buf, 0, &value);                       \
		if (!ret) {                                             \
			if (value <= SWIMCU_ADC_VREF) {			\
				adc_trigger_config[adc].name = value;   \
				ret = count;				\
			}						\
			else {						\
				ret = -ERANGE;				\
			}						\
		}							\
		return ret;						\
	};

/* modem power state in low power mode, default off */
static int swimcu_pm_mdm_pwr = MCI_PROTOCOL_MDM_STATE_OFF;
module_param_named(
	modem_power, swimcu_pm_mdm_pwr, int, S_IRUGO | S_IWUSR | S_IWGRP);

/* mcu reset source, for lack of a better place */
static int swimcu_reset_source = 0;
module_param_named(
	reset_source, swimcu_reset_source, int, S_IRUGO | S_IWUSR | S_IWGRP);

/* mcu fault mask, to record communication errors and irregular MCU behaviour */
module_param_named(
	fault_mask, swimcu_fault_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

/* mcu fault count, number of fault events */
module_param_named(
	fault_count, swimcu_fault_count, int, S_IRUGO | S_IWUSR | S_IWGRP);

/* we can't free the memory here as it is freed by the i2c device on exit */
static void release_kobj(struct kobject *kobj)
{
	swimcu_log(INIT, "%s: %s\n", __func__, kobj->name);
}

static struct kobj_type ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
	.release = release_kobj,
};

static const struct pin_trigger_map {
	enum mci_pin_irqc_type_e type;
	char *name;
} pin_trigger[] = {
	{MCI_PIN_IRQ_DISABLED,     "none"},
	{MCI_PIN_IRQ_DISABLED,     "off"},
	{MCI_PIN_IRQ_LOGIC_ZERO,   "low"},
	{MCI_PIN_IRQ_RISING_EDGE,  "rising"},
	{MCI_PIN_IRQ_FALLING_EDGE, "falling"},
	{MCI_PIN_IRQ_EITHER_EDGE,  "both"},
	{MCI_PIN_IRQ_LOGIC_ONE,    "high"},
};

#define MAX_WAKEUP_TIME (4294967) /* 2^32 / 1000 */
static uint32_t wakeup_time = 0;

static int pm_enable = SWIMCU_PM_OFF;

enum wusrc_index {
	WUSRC_INVALID = -1,
	WUSRC_MIN = 0,
	WUSRC_GPIO36 = WUSRC_MIN,
	WUSRC_GPIO38 = 1,
	WUSRC_TIMER = 2,
	WUSRC_ADC2 = 3,
	WUSRC_ADC3 = 4,
	WUSRC_MAX = WUSRC_ADC3,
};

static const struct wusrc_param {
	enum mci_protocol_wakeup_source_type_e type;
	int id;
	uint32_t mask;
} wusrc_param[] = {
	[WUSRC_GPIO36] = {MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS, SWIMCU_GPIO_PTA0, MCI_PROTCOL_WAKEUP_SOURCE_EXT_PIN_BITMASK_PTA0},
	[WUSRC_GPIO38] = {MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS, SWIMCU_GPIO_PTB0, MCI_PROTCOL_WAKEUP_SOURCE_EXT_PIN_BITMASK_PTB0},
	[WUSRC_TIMER]  = {MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_TIMER, 0, 0},
	[WUSRC_ADC2]   = {MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC, SWIMCU_ADC_PTA12, MCI_PROTOCOL_WAKEUP_SOURCE_ADC_PIN_BITMASK_PTA12},
	[WUSRC_ADC3]   = {MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC, SWIMCU_ADC_PTB1, MCI_PROTOCOL_WAKEUP_SOURCE_ADC_PIN_BITMASK_PTB1},
};

static struct wusrc_value {
	struct kobject *kobj;
	int triggered;
} wusrc_value[] = {
	[WUSRC_GPIO36] = {NULL, 0},
	[WUSRC_GPIO38] = {NULL, 0},
	[WUSRC_TIMER]  = {NULL, 0},
	[WUSRC_ADC2]   = {NULL, 0},
	[WUSRC_ADC3]   = {NULL, 0},
};

static struct adc_trigger_config {
	unsigned int above;
	unsigned int below;
	bool select;
} adc_trigger_config[] = {
	[SWIMCU_ADC_PTA12] = {0, 1800, false},
	[SWIMCU_ADC_PTB1] = {0, 1800, false},
};

static uint32_t adc_interval = 0;
static const char * const poweroff_argv[] = {"/sbin/poweroff", NULL};

#define SWIMCU_PM_WAIT_SYNC_TIME 40000
#define PM_STATE_IDLE     0
#define PM_STATE_SYNC     1
#define PM_STATE_SHUTDOWN 2
static int swimcu_pm_state = PM_STATE_IDLE;

/************
*
* Name:     find_wusrc_index_from_kobj
*
* Purpose:  Match the wusrc index from the provided kobj
*
* Parms:    kobj - kobject ptr of the sysfs node
*
* Return:   a valid index if found
*           -1 if not found
*
* Abort:    none
*
************/
static enum wusrc_index find_wusrc_index_from_kobj(struct kobject *kobj)
{
	enum wusrc_index wi;

	for (wi = 0; wi <= WUSRC_MAX; wi++) {
		if (wusrc_value[wi].kobj == kobj) {
			break;
		}
	}

	if (wi > WUSRC_MAX) {
		pr_err("%s: fail %s\n", __func__, kobj->name);
		wi = WUSRC_INVALID;
	}

	return wi;
}

/************
*
* Name:     find_wusrc_index_from_id
*
* Purpose:  Match the wusrc index from the provided type/id
*
* Parms:    type - gpio or timer
*           id - if gpio, the gpio number (0 - 7)
*
* Return:   a valid index if found
*           -1 if not found
*
* Abort:    none
*
************/
static enum wusrc_index find_wusrc_index_from_id(enum mci_protocol_wakeup_source_type_e type, int id)
{
	enum wusrc_index wi;

	for (wi = 0; wi <= WUSRC_MAX; wi++) {
		if ((type == wusrc_param[wi].type) && (id == wusrc_param[wi].id)) {
			break;
		}
	}

	if (wi > WUSRC_MAX) {
		pr_err("%s: fail type %d id %d\n", __func__, type, id);
		wi = WUSRC_INVALID;
	}

	return wi;
}

/************
*
* Name:     pm_ulpm_config
*
* Purpose:  Configure MCU for ULPM and select wakeup sources
*
* Parms:    swimcu - device driver data
*	    wu_source  - bitmask of wakeup sources
*
* Return:   0 if successful
*	    -ERRNO otherwise

* Abort:    none
*
************/
static int pm_ulpm_config(struct swimcu* swimcu, u16 wu_source)
{

	struct mci_pm_profile_config_s pm_config;
	int ret = 0;
	int rc;

	pm_config.active_power_mode = MCI_PROTOCOL_POWER_MODE_RUN;
	pm_config.active_idle_time = 100;
	pm_config.standby_power_mode = MCI_PROTOCOL_POWER_MODE_VLPS;
	pm_config.standby_mdm_state = swimcu_pm_mdm_pwr;
	pm_config.standby_wakeup_sources = wu_source;
	pm_config.mdm_on_conds_bitset_any = 0;
	pm_config.mdm_on_conds_bitset_all = 0;

	swimcu_log(PM, "%s: pm prof cfg src=%x\n", __func__, wu_source);
	if(MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
	  (rc = swimcu_pm_profile_config(swimcu, &pm_config,
		MCI_PROTOCOL_PM_OPTYPE_SET))) {
		pr_err("%s: pm enable fail %d\n", __func__, rc);
		ret = -EIO;
	}
	return ret;
}

/************
*
* Name:     pm_reboot_call
*
* Purpose:  Handler for reboot notifier
*
* Parms:    this - notifier block associated with this handler
*           code - reboot code. We are only interested in SYS_POWER_OFF
*           cmd  - not used.
*
* Return:   0 always
*
* Abort:    none
*
************/
int pm_reboot_call(
	struct notifier_block *this, unsigned long code, void *cmd)
{
	int rc;
	struct swimcu* swimcu = container_of(this, struct swimcu, reboot_nb);

	if (SYS_POWER_OFF == code)
	{
		switch(swimcu_pm_state) {
		case PM_STATE_SYNC:
			/*
			 * ULPM has been configured,
			 * notify MCU that it is safe to remove power
			 */
			if(MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
			  (rc = swimcu_pm_pwr_off(swimcu))) {
				pr_err("%s: pm poweroff fail %d\n", __func__, rc);
			}
			break;

		case PM_STATE_IDLE:
			/*
			 * Userspace is already shutdown at this point,
			 * so we can set a sync wait time of 0 to shutdown
			 * immediately after ULPM is configured
			 */
			if (MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
                           (rc = swimcu_pm_wait_time_config(swimcu, 0, 0))) {
				pr_err("%s: pm wait_time_config failed %d\n", __func__, rc);
			}

			if(MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
                          (rc = pm_ulpm_config(swimcu, 0))) {
				pr_err("%s: pm ulpm_config fail %d\n", __func__, rc);
			}
			break;

		case PM_STATE_SHUTDOWN:
		default:
			break;
		}
	}
	return NOTIFY_DONE;
}

/************
*
* Name:     swimcu_pm_ulpm_enable
*
* Purpose:  Configure MCU with triggers and enter ultra low power mode
*
* Parms:    swimcu - device driver data
*           pm - 0 (do nothing) or 1 (initiate power down)
*
* Return:   0 if successful
*           -ERRNO otherwise
*
* Abort:    none
*
************/
static int pm_set_mcu_ulpm_enable(struct swimcu *swimcu, int pm)
{
	int ret = 0;
	enum mci_protocol_status_code_e rc;
	enum wusrc_index wi;
	struct mci_wakeup_source_config_s wu_config;
	int gpio, ext_gpio;
	int gpio_cnt = 0;
	uint32_t wu_pin_bits = 0;
	u16 wu_source = 0;
	enum swimcu_adc_index adc_wu_src = SWIMCU_ADC_INVALID;
	enum swimcu_adc_compare_mode adc_compare_mode;
	int adc_bitmask;

	if (pm == SWIMCU_PM_OFF) {
		swimcu_log(PM, "%s: disable\n", __func__);
		return 0;
	}

	if (pm == SWIMCU_PM_BOOT_SOURCE) {
		/* setup GPIO and ADC wakeup sources */
		for( wi = 0; wi < ARRAY_SIZE(wusrc_param); wi++ ) {
			if( wusrc_param[wi].type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS ) {
				gpio = wusrc_param[wi].id;
				if (swimcu_gpio_get_trigger(gpio) != MCI_PIN_IRQ_DISABLED) { /* configured for wakeup */
					ext_gpio = SWIMCU_GPIO_TO_SYS(gpio);
					ret = gpio_request(ext_gpio, KBUILD_MODNAME);
					if (ret < 0) {
						swimcu_log(PM, "%s: %d in use\n", __func__, ext_gpio);
						ret = swimcu_gpio_set(swimcu, SWIMCU_GPIO_NOOP, gpio, 0);
						if (ret < 0) {
							pr_err("%s: irqc set fail %d\n", __func__, gpio);
							goto wu_fail;
						}
					}
					else {
						swimcu_log(PM, "%s: request %d\n", __func__, ext_gpio);
					}
					wu_pin_bits |= wusrc_param[wi].mask;
					gpio_cnt++;
				}
			} else if (MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC == wusrc_param[wi].type &&
				   adc_trigger_config[wusrc_param[wi].id].select) {
					adc_wu_src = wusrc_param[wi].id;
					adc_bitmask = wusrc_param[wi].mask;
			}
		}

		if (gpio_cnt > 0) {
			wu_config.source_type = MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS;
			wu_config.args.pins = wu_pin_bits;
			if( MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
				(rc = swimcu_wakeup_source_config(swimcu, &wu_config,
				MCI_PROTOCOL_WAKEUP_SOURCE_OPTYPE_SET))) {
				pr_err("%s: ext pin wu fail %d\n", __func__, rc);
				ret = -EIO;
				goto wu_fail;
			}
			wu_source |= (u16)MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS;
			swimcu_log(PM, "%s: wu on pins 0x%x\n", __func__, wu_pin_bits);
		}

		if (wakeup_time > 0) {
			wu_config.source_type = MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_TIMER;
			wu_config.args.timeout = wakeup_time * 1000; /* convert to msec */
			if( MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
				(rc = swimcu_wakeup_source_config(swimcu, &wu_config,
				MCI_PROTOCOL_WAKEUP_SOURCE_OPTYPE_SET)) ) {
				pr_err("%s: timer wu fail %d\n", __func__, rc);
				ret = -EIO;
				goto wu_fail;
			}
			wu_source |= (u16)MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_TIMER;
			swimcu_log(PM, "%s: wu on timer %u\n", __func__, wakeup_time);
		}
		if (SWIMCU_ADC_INVALID != adc_wu_src) {
			wu_config.source_type = MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC;
			wu_config.args.channel = adc_bitmask;

			swimcu_adc_set_trigger_mode(adc_wu_src,
						   MCI_PROTOCOL_ADC_TRIGGER_MODE_HW,
					           adc_interval);
			/*
			* if above > below, then trigger when value falls in the range (0, below) or (above, 1800)
			* if above <= below, trigger when value falls in the range (above, below)
			*/
			if (adc_trigger_config[adc_wu_src].above >
				adc_trigger_config[adc_wu_src].below) {
				adc_compare_mode = MCI_PROTOCOL_ADC_COMPARE_MODE_BEYOND;
			} else {
				adc_compare_mode = MCI_PROTOCOL_ADC_COMPARE_MODE_WITHIN;
			}
			swimcu_adc_set_compare_mode(adc_wu_src,
						    adc_compare_mode,
						    adc_trigger_config[adc_wu_src].above,
						    adc_trigger_config[adc_wu_src].below);
			swimcu_adc_init_and_start(swimcu, adc_wu_src);

			if (MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
				(rc = swimcu_wakeup_source_config(swimcu, &wu_config,
					MCI_PROTOCOL_WAKEUP_SOURCE_OPTYPE_SET))) {
				pr_err("%s: adc wu fail %d\n", __func__, rc);
				ret = -EIO;
				goto wu_fail;
			}
			wu_source |= (u16)MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC;
			swimcu_log(PM, "%s: wu on adc %d\n", __func__, adc_wu_src);
		}

	}

	if ((wu_source != 0) || (pm == SWIMCU_PM_POWER_SWITCH)) {
		pr_info("%s: sending wait_time_config", __func__);
		rc = swimcu_pm_wait_time_config(swimcu,
				SWIMCU_PM_WAIT_SYNC_TIME, 0);
		if (MCI_PROTOCOL_STATUS_CODE_SUCCESS == rc) {
			swimcu_pm_state = PM_STATE_SYNC;
		}

		else if(MCI_PROTOCOL_STATUS_CODE_UNKNOWN_COMMAND == rc) {
			pr_info("%s: pm wait_time_config not recognized by MCU, \
				proceed with legacy shutdown\n", __func__);
			swimcu_pm_state = PM_STATE_SHUTDOWN;
		}

		pr_info("%s: sending ulpm_config", __func__);
		if(MCI_PROTOCOL_STATUS_CODE_SUCCESS !=
		   (rc = pm_ulpm_config(swimcu, wu_source))) {
			pr_err("%s: pm enable fail %d\n", __func__, rc);
			ret = -EIO;
			goto wu_fail;
		}

		if(PM_STATE_SYNC == swimcu_pm_state) {
			call_usermodehelper(poweroff_argv[0],
				poweroff_argv, NULL, UMH_NO_WAIT);
		}
	}
	else {
		pr_err("%s: no wake sources set\n", __func__);
		/* nothing to clean up in this case */
		return -EPERM;
	}
	return 0;

wu_fail:
  	/* free any gpio's that have been requested */
	for( wi = 0; wi < ARRAY_SIZE(wusrc_param); wi++ ) {
		if( wusrc_param[wi].type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS ) {
			gpio = wusrc_param[wi].id;
			if (swimcu_gpio_get_trigger(gpio) !=  MCI_PIN_IRQ_DISABLED) {
				/* configured for wakeup */
				ext_gpio = SWIMCU_GPIO_TO_SYS(gpio);
				gpio_free(ext_gpio);
				swimcu_log(PM, "%s: free %d\n", __func__, gpio);
			}
		}
	}

	return ret;
}

/* sysfs entries to set boot_source GPIO triggers for inputs PTA0 (gpio36) and PTB0 (gpio38) */
static ssize_t pm_gpio_attr_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	enum wusrc_index wi;
	enum mci_pin_irqc_type_e irqc_type;
	int ti;
	int gpio;
	int ret = -EINVAL;

	wi = find_wusrc_index_from_kobj(kobj);
	if ((wi != WUSRC_INVALID) && (wusrc_param[wi].type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS)) {
		gpio = wusrc_param[wi].id;
	}
	else {
		pr_err("%s: unrecognized GPIO %s\n", __func__, kobj->name);
		return -EINVAL;
	}

	irqc_type = swimcu_gpio_get_trigger(gpio);

	for (ti = ARRAY_SIZE(pin_trigger) - 1; ti > 0; ti--) {
		/* if never found we exit at ti == 0: "off" */
		if (irqc_type == pin_trigger[ti].type) {
			swimcu_log(PM, "%s: found gpio %d trigger %d\n", __func__, gpio, ti);
			break;
		}
	}
	ret = scnprintf(buf, PAGE_SIZE, pin_trigger[ti].name);

	if (ret > 0) {
		strlcat(buf, "\n", PAGE_SIZE);
		ret++;
	}

	return ret;
};

static ssize_t pm_gpio_attr_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	enum wusrc_index wi;
	int ti;
	int gpio;

	wi = find_wusrc_index_from_kobj(kobj);
	if ((wi != WUSRC_INVALID) &&
		(wusrc_param[wi].type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS)) {
		gpio = wusrc_param[wi].id;
	}
	else {
		pr_err("%s: unrecognized GPIO %s\n", __func__, kobj->name);
		return -EINVAL;
	}

	for (ti = ARRAY_SIZE(pin_trigger) - 1; ti >= 0; ti--) {
	/* if never found we exit at ti == -1: invalid */
		if (sysfs_streq(buf, pin_trigger[ti].name)) {
			if (swimcu_gpio_set_trigger(gpio, pin_trigger[ti].type) < 0) {
				swimcu_log(PM, "%s: failed gpio %d\n", __func__, gpio);
				return -EPERM;
			}
			wusrc_value[wi].triggered = 0;
			swimcu_log(PM, "%s: setting gpio %d to trigger %d\n", __func__, gpio, ti);
			break;
		}
	}

	if(ti < 0) {
		pr_err("%s: unknown trigger %s\n", __func__, buf);
		return -EINVAL;
	}

	return count;
};

static ssize_t pm_timer_timeout_attr_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", wakeup_time);
}

static ssize_t pm_timer_timeout_attr_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	uint32_t tmp_time;

	if (0 == (ret = kstrtouint(buf, 0, &tmp_time))) {
		if (tmp_time <= MAX_WAKEUP_TIME) {
			wakeup_time = tmp_time;
			wusrc_value[WUSRC_TIMER].triggered = 0;
			return count;
		}
		else {
			ret = -ERANGE;
		}
	}
	pr_err("%s: invalid input %s ret %d\n", __func__, buf, ret);
	return ret;
};

static ssize_t enable_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int tmp_enable;
	struct swimcu *swimcu = container_of(kobj, struct swimcu, pm_boot_source_kobj);

	if (0 == (ret = kstrtoint(buf, 0, &tmp_enable))) {
		if ((tmp_enable < SWIMCU_PM_OFF) || (tmp_enable > SWIMCU_PM_MAX)) {
			ret = -ERANGE;
		}
		else if ((pm_enable > SWIMCU_PM_OFF) ||
			/* ULPM already initiated */
			(0 == (ret = pm_set_mcu_ulpm_enable(swimcu, tmp_enable)))) {
			/* send ULPM command */
			pm_enable = tmp_enable;
			ret = count;
		}
	}
	if (ret < 0)
		pr_err("%s: invalid input %s ret %d\n", __func__, buf, ret);
	return ret;
};

static ssize_t update_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct swimcu *swimcu = container_of(kobj, struct swimcu, pm_firmware_kobj);

	/* transition the MCU to boot mode, reset then required to continue firmware update */
	if (MCI_PROTOCOL_STATUS_CODE_SUCCESS == swimcu_to_boot_transit(swimcu)) {
		ret = count;
	}
	else {
		ret = -EIO;
		pr_err("%s: invalid input %s\n", __func__, buf);
	}
	return ret;
};

static ssize_t version_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct swimcu *swimcu = container_of(kobj, struct swimcu, pm_firmware_kobj);

	swimcu_ping(swimcu);
	return scnprintf(buf, PAGE_SIZE, "%03d.%03d\n", swimcu->version_major, swimcu->version_minor);
}

static ssize_t triggered_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int triggered = 0;
	enum wusrc_index wi = find_wusrc_index_from_kobj(kobj);

	if (wi != WUSRC_INVALID)
		triggered = wusrc_value[wi].triggered;

	swimcu_log(PM, "%s: %d = %d\n", __func__, wi, triggered);

	return scnprintf(buf, PAGE_SIZE, "%d\n", triggered);
}

ADC_ATTR_SHOW(above)
ADC_ATTR_SHOW(below)
ADC_ATTR_SHOW(select)

ADC_ATTR_STORE(above)
ADC_ATTR_STORE(below)

static ssize_t pm_adc_select_attr_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int select = 0;
	enum swimcu_adc_index adc;
	enum wusrc_index wi = find_wusrc_index_from_kobj(kobj);
	int ret = -EINVAL;
	int i;

	if (WUSRC_INVALID == wi) {
		return ret;
	}

	adc = wusrc_param[wi].id;
	if (0 == (ret = kstrtouint(buf, 0, &select))) {
		if (select > 1) {
			ret = -EINVAL;
		}
	}

	for (i = 0; i < SWIMCU_NUM_ADC; i++) {
		if (select && adc_trigger_config[i].select && (i != adc)) {
			pr_err("%s: cannot select more than 1 adc as boot_source", __func__);
			ret = -EPERM;
		}
	}

	if (!ret) {
		adc_trigger_config[adc].select = select;
		ret = count;
	}
	return ret;
};

static ssize_t pm_adc_interval_attr_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", adc_interval);
}

static ssize_t pm_adc_interval_attr_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int interval;

	if (0 == (ret = kstrtouint(buf, 0, &interval))) {
		if (interval <= MAX_WAKEUP_TIME) {
			adc_interval = interval;
			return count;
		}
		else {
			ret = -ERANGE;
		}
	}
	pr_err("%s: invalid input %s ret %d\n", __func__, buf, ret);
	return ret;
};

/************
*
* Name:     swimcu_set_wakeup_source
*
* Purpose:  Store the wakeup source to be read from triggered node
*
* Parms:    type - type of the wakeup source
*           value - information of the wakeup source (type-dependent)
*
* Return:   Nothing
*
* Abort:    none
*
************/
void swimcu_set_wakeup_source(enum mci_protocol_wakeup_source_type_e type, u16 value)
{
	enum wusrc_index wi = WUSRC_INVALID;

	swimcu_log(PM, "%s: type %d val 0x%x\n", __func__, type, value);

	if (type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_TIMER) {
		wi = WUSRC_TIMER;
	}
	else if (type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_ADC) {
		enum swimcu_adc_index adc = swimcu_get_adc_from_chan(value);
		wi = find_wusrc_index_from_id(type, adc);
	}
	else if (type == MCI_PROTOCOL_WAKEUP_SOURCE_TYPE_EXT_PINS){
		int port = GET_WUSRC_PORT(value);
		int pin = GET_WUSRC_PIN(value);
		enum swimcu_gpio_index gpio = swimcu_get_gpio_from_port_pin(port, pin);
		wi = find_wusrc_index_from_id(type, gpio);
	}
	if (wi != WUSRC_INVALID) {
		swimcu_log(PM, "%s: %d\n", __func__, wi);
		wusrc_value[wi].triggered = 1;
	}
	else {
		pr_err("%s: unknown wakeup pin 0x%x\n", __func__, value);
	}
}

/************
*
* Name:     swimcu_set_reset_source
*
* Purpose:  Store the reset source to be read from reset_source parameter
*
* Parms:    value - reset source bit mask (mci_protocol_reset_source_e)
*
* Return:   Nothing
*
* Abort:    none
*
************/
void swimcu_set_reset_source(enum mci_protocol_reset_source_e value)
{
	swimcu_log(INIT, "%s: 0x%x\n", __func__, value);
	swimcu_reset_source = value;
}

/* MCU has 2 interruptible GPIOs, PTA0 and PTB0 that map to index 2 and 4
 * respectively on gpiochip200, which in turn appear as GPIO36 and 38 on the WP76xx */
static const struct kobj_attribute pm_gpio_edge_attr[] = {
	__ATTR(edge,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_gpio_attr_show,
		&pm_gpio_attr_store),
};

static const struct kobj_attribute pm_triggered_attr = __ATTR_RO(triggered);

/* sysfs entries to set boot_source timer timeout value */
static const struct kobj_attribute pm_timer_timeout_attr[] = {
	__ATTR(timeout,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_timer_timeout_attr_show,
		&pm_timer_timeout_attr_store),
};

/* sysfs entries to set boot_source enable */
static const struct kobj_attribute pm_enable_attr = __ATTR_WO(enable);

/* sysfs entries to initiate firmware upgrade */
static const struct kobj_attribute fw_update_attr = __ATTR_WO(update);

/* sysfs entry to read current mcu firmware version */
static const struct kobj_attribute fw_version_attr = __ATTR_RO(version);

static const struct kobj_attribute pm_adc_trig_attr[] = {
	__ATTR(below,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_adc_below_attr_show,
		&pm_adc_below_attr_store),

	__ATTR(above,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_adc_above_attr_show,
		&pm_adc_above_attr_store),

	__ATTR(select,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_adc_select_attr_show,
		&pm_adc_select_attr_store),
};

static const struct kobj_attribute pm_adc_interval_attr = \
	__ATTR(interval,
		S_IRUGO | S_IWUSR | S_IWGRP,
		&pm_adc_interval_attr_show,
		&pm_adc_interval_attr_store);

/************
*
* Name:     swimcu_pm_sysfs_deinit
*
* Purpose:  Remove sysfs tree under /sys/module/swimcu_pm
*
* Parms:    swimcu - device data ptr
*
* Return:   nothing
*
* Abort:    none
*
************/
void swimcu_pm_sysfs_deinit(struct swimcu *swimcu)
{
	if (swimcu->pm_firmware_kobj.state_initialized)
		kobject_put(&swimcu->pm_firmware_kobj);
	if (swimcu->pm_boot_source_kobj.state_initialized)
		kobject_put(&swimcu->pm_boot_source_kobj);
}

/************
*
* Name:     swimcu_pm_sysfs_init
*
* Purpose:  Setup sysfs tree under /sys/module/swimcu_pm
*
* Parms:    swimcu - device data ptr contains kobj data so attr functions
*                    can retrieve this structure to access the driver (e.g. i2c)
*           func_flags - select which attributes to init
*                        SWIMCU_FUNC_FLAG_FWUPD - firmware
*                        SWIMCU_FUNC_FLAG_PM - boot_source
*
* Return:   0 if successful
*           -ERRNO otherwise
*
* Abort:    none
*
************/
int swimcu_pm_sysfs_init(struct swimcu *swimcu, int func_flags)
{
	struct kobject *module_kobj;
	struct boot_sources_s {
		struct kobject **kobj;
		struct kobject *kobj_parent;
		const struct kobj_attribute *custom_attr;
		char *name;
		unsigned int num_cust_kobjs;
	} boot_source[] = {
		{&wusrc_value[WUSRC_GPIO36].kobj, &swimcu->pm_boot_source_kobj, pm_gpio_edge_attr, "gpio36", ARRAY_SIZE(pm_gpio_edge_attr)},
		{&wusrc_value[WUSRC_GPIO38].kobj, &swimcu->pm_boot_source_kobj, pm_gpio_edge_attr, "gpio38", ARRAY_SIZE(pm_gpio_edge_attr)},
		{&wusrc_value[WUSRC_TIMER].kobj, &swimcu->pm_boot_source_kobj, pm_timer_timeout_attr, "timer", ARRAY_SIZE(pm_timer_timeout_attr)},
		{&wusrc_value[WUSRC_ADC2].kobj, &swimcu->pm_boot_source_adc_kobj, pm_adc_trig_attr, "adc2", ARRAY_SIZE(pm_adc_trig_attr)},
		{&wusrc_value[WUSRC_ADC3].kobj, &swimcu->pm_boot_source_adc_kobj, pm_adc_trig_attr, "adc3", ARRAY_SIZE(pm_adc_trig_attr)},
	};

	int i;
	int j;
	int ret;

	module_kobj = kset_find_obj(module_kset, KBUILD_MODNAME);
	if (!module_kobj) {
		pr_err("%s: cannot find kobject for module %s\n",
			__func__, KBUILD_MODNAME);
		ret = -ENOENT;
		goto sysfs_add_exit;
	}

	if (func_flags & SWIMCU_FUNC_FLAG_FWUPD) {
	/* firmware object */

		ret = kobject_init_and_add(&swimcu->pm_firmware_kobj, &ktype, module_kobj, "firmware");
		if (ret) {
			pr_err("%s: cannot create firmware kobject\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}

		ret = sysfs_create_file(&swimcu->pm_firmware_kobj, &fw_version_attr.attr);
		if (ret) {
			pr_err("%s: cannot create version\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}

		ret = sysfs_create_file(&swimcu->pm_firmware_kobj, &fw_update_attr.attr);
		if (ret) {
			pr_err("%s: cannot create update\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}
		kobject_uevent(&swimcu->pm_firmware_kobj, KOBJ_ADD);
	}

	if (func_flags & SWIMCU_FUNC_FLAG_PM) {
	/* boot_source object */

		ret = kobject_init_and_add(&swimcu->pm_boot_source_kobj, &ktype, module_kobj, "boot_source");
		if (ret) {
			pr_err("%s: cannot create boot_source kobject\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}
		ret = kobject_init_and_add(&swimcu->pm_boot_source_adc_kobj, &ktype, &swimcu->pm_boot_source_kobj, "adc");
		if (ret) {
			pr_err("%s: cannot create adc kobject for boot_source\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}
		ret = sysfs_create_file(&swimcu->pm_boot_source_adc_kobj, &pm_adc_interval_attr.attr);
		if (ret) {
			pr_err("%s: cannot create interval file for adc\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}
		/* populate boot_source sysfs tree */
		for (i = 0; i < ARRAY_SIZE(boot_source); i++) {
			swimcu_log(PM, "%s: create kobj %d for %s", __func__, i, boot_source[i].name);
			*boot_source[i].kobj = kobject_create_and_add(boot_source[i].name, boot_source[i].kobj_parent);
			if (!*boot_source[i].kobj) {
				pr_err("%s: cannot create boot_source kobject for %s\n", __func__, boot_source[i].name);
				ret = -ENOMEM;
				goto sysfs_add_exit;
			}
			ret = sysfs_create_file(*boot_source[i].kobj, &pm_triggered_attr.attr);
			if (ret) {
				pr_err("%s: cannot create triggered file for %s\n", __func__, boot_source[i].name);
				ret = -ENOMEM;
				goto sysfs_add_exit;
			}
			for (j = 0; j < boot_source[i].num_cust_kobjs; j++) {
				ret = sysfs_create_file(*boot_source[i].kobj, &boot_source[i].custom_attr[j].attr);
				if (ret) {
					pr_err("%s: cannot create custom file for %s\n", __func__, boot_source[i].name);
					ret = -ENOMEM;
					goto sysfs_add_exit;
				}
			}
		}

		ret = sysfs_create_file(&swimcu->pm_boot_source_kobj, &pm_enable_attr.attr);
		if (ret) {
			pr_err("%s: cannot create enable\n", __func__);
			ret = -ENOMEM;
			goto sysfs_add_exit;
		}
		kobject_uevent(&swimcu->pm_boot_source_kobj, KOBJ_ADD);
	}

	swimcu_log(INIT, "%s: success func %d\n", __func__, func_flags);
	return 0;

sysfs_add_exit:
	swimcu_log(INIT, "%s: fail func %d, ret %d\n", __func__, func_flags, ret);
	swimcu_pm_sysfs_deinit(swimcu);
	return ret;
}
