/* kernel/include/linux/sierra_bsudefs.h
 *
 * Copyright (C) 2013 Sierra Wireless, Inc
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

#ifndef BS_UDEFS_H
#define BS_USDEFS_H

/************
 *
 * Name:     bshwtype
 *
 * Purpose:  To enumerate hardware types
 *
 * Members:  See below
 *
 * Notes:    None
 *
 ************/
enum bshwtype
{
  BSQCTMTP = 0,                 /* Qualcomm MTP 9x30                          */
  BSHWNONE,                     /* HW type NONE (Fuse has not been blown yet) */
  BSAR7590,                     /* 0x02 - Automotive 7590                     */
  BSAR7592,                     /* 0x03 - Automotive 7592                     */
  BSAR7594,                     /* 0x04 - Automotive 7594                     */
  BSAR7596,                     /* 0x05 - Automotive 7596                     */
  BSAR7598,                     /* 0x06 - Automotive 7598                     */
  BSHWUNKNOWN,                  /* Unknown HW                                 */
  BSHWINVALID = 0xFF            /* Invalid HW                                 */
};

/************
 *
 * Name:     bshwrev
 *
 * Purpose:  To enumerate hardware revisions
 *
 * Members:  BSHWREV0       - Revision 0
 *           BSHWREVMAX     - maximum possible HW revision
 *           BSHWREVUNKNOWN - unknown revision
 *
 * Notes:    None
 *
 ************/
enum bshwrev
{
  BSHWREV0 = 0,
  BSHWREV1,
  BSHWREV2,
  BSHWREV3,
  BSHWREV4,

  BSHWREVMAX = 63,
  BSHWREVUNKNOWN = 0xFF
};

#include "sierra_bsuproto.h"
#endif
