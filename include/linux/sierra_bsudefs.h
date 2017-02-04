/* kernel/include/linux/sierra_bsudefs.h
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

#ifndef BS_UDEFS_H
#define BS_UDEFS_H
/************
 *
 * Name:     bsfeature
 *
 * Purpose:  Enumerated list of different features supported by different
 *               hardware variants
 *
 * Members:  see below
 *
 * Notes:    None
 *
 ************/
enum bsfeature {
	BSFEATURE_INTERNALCODEC,        /* if INTERNAL CODDEC is supported */
	BSFEATURE_MAX                   /* Used for bounds checking */
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
