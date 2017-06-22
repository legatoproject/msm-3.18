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
  BSAR7596_NB7,                 /* 0x07 - Automotive 7596                     */
  BSAR7692,                     /* 0x08 - Automotive 7692                     */
  BSAR7694,                     /* 0x09 - Automotive 7694                     */
  BSAR7696,                     /* 0x0A - Automotive 7696                     */
  BSAR7698,                     /* 0x0B - Automotive 7698                     */
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

/************
 *
 * Name:     bsuartfunc
 *
 * Purpose:  Enumerated list of different functions supported by App processor
 *
 * Members:  BSUARTFUNC_DISABLED  - UART disabled
 *           BSUARTFUNC_AT - UART reserved for AT service
 *           BSUARTFUNC_DM - UART reserved for DM service
 *           BSUARTFUNC_NMEA - UART reserved for NMEA service
 *           BSUARTFUNC_CONSOLE - UART reserved for CONSOLE service
 *           BSUARTFUNC_APP - UART open for all application usage
 *
 * Notes:    None
 *
 ************/
enum bsuartfunc
{
  BSUARTFUNC_DISABLED = 0,
  BSUARTFUNC_AT       = 1,
  BSUARTFUNC_DM       = 2,
  BSUARTFUNC_NMEA     = 4,
  BSUARTFUNC_CONSOLE  = 16,
  BSUARTFUNC_APP      = 17,
};

/************
 *
 * Members:  BS_UART1_LINE - line number of UART1
 *           BS_UART2_LINE - line number of UART2
 *
 * Notes:    None
 *
 ************/
#define BS_UART1_LINE  0
#define BS_UART2_LINE  1

#include "sierra_bsuproto.h"
#endif
