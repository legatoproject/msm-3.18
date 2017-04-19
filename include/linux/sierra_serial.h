/* kernel/include/linux/sierra_serial.h
 *
 * Copyright (C) 2017 Sierra Wireless, Inc
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

#ifndef CONFIG_SIERRA_SERIAL_H
#define CONFIG_SIERRA_SERIAL_H
#include <linux/device.h>
#include <linux/sierra_bsudefs.h>

extern int8_t uart_config_str_get(uint8_t line,
				uint8_t uart_type,
				char *buf,
				uint32_t buf_size);

extern int8_t uart_config_set(uint8_t line, uint8_t uart_type);

extern int8_t uart_config_get(uint8_t line, uint8_t uart_type);

extern void uart_sysfs_symlink_set(struct device * dev,
				uint8_t uart_type);
#endif
