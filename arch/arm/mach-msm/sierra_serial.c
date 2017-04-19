/* arch/arm/mach-msm/sierra_serial.c
 *
 * Copyright (C) 2017 Sierra Wireless, Inc
 * Author: Bingo Chen <bchen@sierrawireless.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/sierra_serial.h>

/* Define below function string maximum length */
#define UART_CONFIG_SIZE_MIN  20

const static char at_func_string[] = "AT\n";
const static char dm_func_string[] = "DM\n";
const static char nmea_func_string[] = "NMEA\n";
const static char cons_func_string[] = "CONSOLE\n";
const static char app_func_string[] = "APP\n";
const static char inv_func_string[] = "UNAVAILABLE\n";
const static char dis_func_string[] = "DISABLED\n";

static int8_t uart_hsl_func[BS_UART_LINE_MAX] = {0};
static int8_t uart_hs_func[BS_UART_LINE_MAX] = {0};
static int8_t* uart_func_tbl[BS_UART_TYPE_MAX] = { uart_hsl_func, uart_hs_func };

static char* uart_hsl_func_str_pt[BS_UART_LINE_MAX] = {0};
static char* uart_hs_func_str_pt[BS_UART_LINE_MAX] = {0};
static char** uart_func_str_tbl[BS_UART_TYPE_MAX] = { uart_hsl_func_str_pt, uart_hs_func_str_pt };

const static char* uart_name_tbl[BS_UART_TYPE_MAX][BS_UART_LINE_MAX] = {
				{"ttyHSL0", "ttyHSL1"},
				{"ttyHS0", "ttyHS1" }};

const static char* uart_sym_dir_str_tbl[BS_UART_TYPE_MAX][BS_UART_LINE_MAX] = {
				{"msm_serial_hsl.0", "msm_serial_hsl.1"},
				{"msm_serial_hs.0", "msm_serial_hs.1"}};


/************
 *
 * Name:     uart_func_init
 *
 * Purpose:  init the uart function refer to its type and line
 *
 * Parms:    line - UART 0 or 1
 *           uart_type - BS_UART_TYPE_HSL or BS_UART_TYPE_HS
 *
 * Return:   none
 *
 * Abort:    none
 *
 * Notes:    none
 *
 ************/
static void uart_func_init(uint8_t line, uint8_t uart_type)
{
	int8_t uart_func = 0;

	if (line >= BS_UART_LINE_MAX || uart_type >= BS_UART_TYPE_MAX)
	{
		return;
	}

	uart_func = bs_uart_fun_get(line);

	if ( (uart_func < BS_UART_FUNC_DISABLED)
		|| (uart_func >= BS_UART_FUNC_MAX) ) {
		uart_func = BS_UART_FUNC_DISABLED;
	}
	else {
		if ( (uart_type == BS_UART_TYPE_HS) && (line == BS_UART1_LINE) ){
			switch (uart_func) {
				case BS_UART_FUNC_AT:
				case BS_UART_FUNC_NMEA:
				case BS_UART_FUNC_APP:
				case BS_UART_FUNC_DISABLED:
					break;
				default:
					uart_func = BS_UART_FUNC_INVALID;
			}
		}
		else if ( (uart_type == BS_UART_TYPE_HSL) && (line == BS_UART1_LINE) ){
			/* for UART1, the CONSOLE and DM are managed by the HSL driver,
			* other functionalities are managed by the HS driver */
			switch (uart_func) {
				case BS_UART_FUNC_CONSOLE:
				case BS_UART_FUNC_DM:
					break;
				default:
					uart_func = BS_UART_FUNC_DISABLED;
			}
		}
		else if ( (uart_type == BS_UART_TYPE_HSL) && (line == BS_UART2_LINE) ){
			switch (uart_func) {
				case BS_UART_FUNC_AT:
				case BS_UART_FUNC_DM:
				case BS_UART_FUNC_NMEA:
				case BS_UART_FUNC_APP:
				case BS_UART_FUNC_CONSOLE:
				case BS_UART_FUNC_DISABLED:
					break;
				default:
					/* UART2 is set to console by default if the mapping is invalid */
					uart_func = BS_UART_FUNC_CONSOLE;
			}
		}
		else {
			uart_func = BS_UART_FUNC_DISABLED;
		}
	}

	uart_func_tbl[uart_type][line] = uart_func;
}


/************
 *
 * Name:     uart_config_set
 *
 * Purpose:  Set UART config enum index
 *
 * Parms:    line - UART 0 or 1
 *           uart_type - BS_UART_TYPE_HSL or BS_UART_TYPE_HS
 *
 * Return:   success, return 0
 *           fail, return -1
 *
 * Abort:    none
 *
 * Notes:    none
 *
 ************/
int8_t uart_config_set(uint8_t line, uint8_t uart_type)
{
	char ** uart_func_str_pt = NULL;
	int8_t uart_func = 0;

	if (line >= BS_UART_LINE_MAX || uart_type >= BS_UART_TYPE_MAX)
	{
		return -1;
	}

	uart_func_init(line, uart_type);
	uart_func_str_pt = uart_func_str_tbl[uart_type];
	uart_func = uart_func_tbl[uart_type][line];

	switch (uart_func) {
		case BS_UART_FUNC_AT:
			pr_info("%s is reserved for AT service.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)at_func_string;
			break;
		case BS_UART_FUNC_DM:
			pr_info("%s is reserved for DM service.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)dm_func_string;
			break;
		case BS_UART_FUNC_NMEA:
			pr_info("%s is reserved for NMEA service.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)nmea_func_string;
			break;
		case BS_UART_FUNC_APP:
			pr_info("%s could be used as generic serial port.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)app_func_string;
			break;
		case BS_UART_FUNC_CONSOLE:
			pr_info("%s is reserved for CONSOLE service.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)cons_func_string;
			break;
		case BS_UART_FUNC_DISABLED:
			pr_info("%s is disabled.\n", uart_name_tbl[uart_type][line]);
			uart_func_str_pt[line] = (char *)dis_func_string;
			return -1;
		default:
			pr_info("%s function %d is not valid on application processor.\n",
				uart_name_tbl[uart_type][line], uart_func);
			uart_func_str_pt[line] = (char *)inv_func_string;
			return -1;
	}
	return 0;
}
EXPORT_SYMBOL(uart_config_set);

/************
 *
 * Name:     uart_config_get
 *
 * Purpose:  Get UART config enum index
 *
 * Parms:    line - UART 0 or 1
 *           uart_type - BS_UART_TYPE_HSL or BS_UART_TYPE_HS
 *
 * Return:   success, return UART config enum index
 *           fail, return -1
 *
 * Abort:    none
 *
 * Notes:    none
 *
 ************/
int8_t uart_config_get(uint8_t line, uint8_t uart_type)
{
	if (line >= BS_UART_LINE_MAX || uart_type >= BS_UART_TYPE_MAX)
	{
		return -1;
	}

	return uart_func_tbl[uart_type][line];
}
EXPORT_SYMBOL(uart_config_get);


/************
 *
 * Name:     uart_config_str_get
 *
 * Purpose:  Fill UART config in buffer
 *
 * Parms:    line - UART 0 or 1
 *           uart_type - BS_UART_TYPE_HSL or BS_UART_TYPE_HS
 *           buf - buffer pointer
 *           buf_size - buffer size
 *
 * Return:   success, return buffer used size
 *           fail, return -1
 *
 * Abort:    none
 *
 * Notes:    none
 *
 ************/
int8_t uart_config_str_get(uint8_t line,
				uint8_t uart_type,
				char *buf,
				uint32_t buf_size)
{
	char ** uart_func_str_pt = NULL;
	if ( (buf == NULL) || (buf_size < UART_CONFIG_SIZE_MIN ) || (uart_type >= BS_UART_TYPE_MAX) )
	{
		pr_info("Invalid parameters");
		return -1;
	}

	memset (buf, 0, UART_CONFIG_SIZE_MIN);

	if (line >= BS_UART_LINE_MAX)
	{
		pr_info("invalid line number %d", line);
		strcpy(buf, inv_func_string);
		return strlen(inv_func_string) + 1;
	}

	uart_func_str_pt = uart_func_str_tbl[uart_type];

	if (uart_func_str_pt[line])
	{
		strcpy(buf, uart_func_str_pt[line]);
		return strlen((const char*)uart_func_str_pt[line]) + 1;
	}
	else
	{
		strcpy(buf, inv_func_string);
		return strlen(inv_func_string) + 1;
	}
}
EXPORT_SYMBOL(uart_config_str_get);


/************
 *
 * Name:     uart_sysfs_symlink_set
 *
 * Purpose:  create a symlink for UART devices on SYSFS
 *
 * Parms:    dev - struct device pointer
 *           uart_type - BS_UART_TYPE_HSL or BS_UART_TYPE_HS
 *
 * Return:   none
 *
 * Abort:    none
 *
 * Notes:    With DTS implementation, the information of UART is shown under
 *           /sys/devices. In order to be back-compatible with initscripts,
 *           create a symlink under /sys/devices/platform
 ************/
void uart_sysfs_symlink_set(struct device * dev,
				uint8_t uart_type)
{
	int ret = 0;
	u32 line;
	struct kobject *platform_kobj;

	struct platform_device *pdev = to_platform_device(dev);

	line = pdev->id;

	platform_kobj = kset_find_obj(pdev->dev.kobj.kset, "platform");

	if ( (line < BS_UART_LINE_MAX) && (uart_type < BS_UART_TYPE_MAX) && (platform_kobj != NULL ))
	{
		ret = sysfs_create_link(platform_kobj, &pdev->dev.kobj, uart_sym_dir_str_tbl[uart_type][line]);
		pr_info ("symlink dir is %s, ret is %d\n", uart_sym_dir_str_tbl[uart_type][line], ret);
	}
}
EXPORT_SYMBOL(uart_sysfs_symlink_set);
