/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#ifndef __MSM_SIERRA_H
#define __MSM_SIERRA_H


#include <linux/ioctl.h>

struct tzrow_data_s {
  uint32_t row_address;
  uint64_t fusedata;
};

#define TZFUSE_IOCTL_MAGIC	       0x9A

#define TZFUSE_IOC_ROW_READ            _IOWR(TZFUSE_IOCTL_MAGIC, 0x14, struct tzrow_data_s)
#define TZFUSE_IOC_ROW_WRITE           _IOW( TZFUSE_IOCTL_MAGIC, 0x15, struct tzrow_data_s)

#define SIERRA_QFUSE_DEV_PATH          "/dev/sierra_qfuse"

#endif /*__MSM_SIERRA_H*/
