/* arch/arm/mach-msm/sierra_bsidefs.h
 *
 * Copyright (C) 2016 Sierra Wireless, Inc
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
#ifndef BS_IDEFS_H
#define BS_IDEFS_H
/* Local constants and enumerated types */

/* RAM Copies of HW type, rev, etc. */
extern bool bshwconfigread;


/* Structures */

/************
 *
 * Name:    bshwconfig
 *
 * Purpose: to allow easy access to fields of the hardware configuration
 *
 * Members:
 *          all             - single uint32 containing all fields
 *          hw.type         - hardware type
 *          hw.rev          - hardware revision
 *          hw.mfgmode      - manufacturing mode
 *          hw.spare        - spare
 *
 * Notes:
 *
 ************/
union bshwconfig
{
  uint32_t all;
  struct __packed
  {
    uint8_t type;
    uint8_t rev;
    uint8_t mfgmode;
    uint8_t spare;
  } hw;
};

#endif
