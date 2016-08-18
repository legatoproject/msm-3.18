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
#define BS_UDEFS_H


enum bshwtype
{
	BSQCTMTP,						/* Qualcomm MTP 9x30 */
	BSHWNONE,						/* HW type NONE (Fuse has not been blown yet) */
	BSAR7582,						/* 0x02 - Automotive 7582 */
	BSAR7584,						/* 0x03 - Automotive 7584 */
	BSAR7586,						/* 0x04 - Automotive 7586 */
	BSAR7588,						/* 0x05 - Automotive 7588 */
	BSAR8582,						/* 0x06 - Automotive 8582 */
	BSAR7582_NC,				/* 0x07 - Automotive 7582 without codec */
	BSAR7584_NC,				/* 0x08 - Automotive 7584 without codec */
	BSAR7586_NC,				/* 0x09 - Automotive 7586 without codec */
	BSAR7588_NC,				/* 0x0A - Automotive 7588 without codec */
	BSAR8582_NC,				/* 0x0B - Automotive 8582 without codec */
	BSWP7601,						/* 0x0C - WP7601 */
	BSWP7603,						/* 0x0D - WP7603 */
	BSHWUNKNOWN,				/* Unknown HW */
	BSHWINVALID = 0xFF	/* Invalid HW */
};

/************
 *
 * Name:		 bs_prod_family_e
 *
 * Purpose:  To enumerate product family types
 *
 * Members:  See below
 *
 * Notes:		 None
 *
 ************/
enum bs_prod_family_e
{
	BS_PROD_FAMILY_QCTMTP,				/* Qualcomm MTP 9x30 */
	BS_PROD_FAMILY_NONE,					/* product family type NONE */
	BS_PROD_FAMILY_EM,						/* product family Embedded Module */
	BS_PROD_FAMILY_MC,						/* product family MiniCard */
	BS_PROD_FAMILY_AR,						/* product family Automotive */
	BS_PROD_FAMILY_WP,						/* product family WP */
	BS_PROD_FAMILY_UNKNOWN,				/* product family Unknown */
	BS_PROD_FAMILY_INVALID = 0xFF	/* product family Invalid */
};

/************
 *
 * Name:		 bsfeature
 *
 * Purpose:  Enumerated list of different features supported by different hardware variants
 *
 * Members:  see below
 *
 * Notes:		 None
 *
 ************/
enum bsfeature
{
	BSFEATURE_MINICARD,				/* if the hardware is a MiniCard */
	BSFEATURE_EM,							/* if the device is EM product */
	BSFEATURE_AR,							/* if the hardware is an AR product */
	BSFEATURE_WP,							/* if the hardware is a WP product */
	BSFEATURE_W_DISABLE,			/* if W_DISABLE is supported */
	BSFEATURE_VOICE,					/* if voice is supported */
	BSFEATURE_HSUPA,					/* if the hardware supports HSUPA */
	BSFEATURE_GPIOSAR,				/* if GPIO controlled SAR backoff is supported */
	BSFEATURE_RMAUTOCONNECT,	/* if auto-connect feature is device centric */
	BSFEATURE_UART,						/* if the hardware support UART */
	BSFEATURE_ANTSEL,					/* if the hardware supports ANTSEL */
	BSFEATURE_INSIM,					/* Internal SIM supported (eSIM) */
	BSFEATURE_OOBWAKE,				/* if has OOB_WAKE GPIO */
	BSFEATURE_CDMA,						/* if the hardware supports CDMA/1x */
	BSFEATURE_GSM,						/* if the hardware supports GSM/EDGE */
	BSFEATURE_WCDMA,					/* if the hardware supports WCDMA */
	BSFEATURE_LTE,						/* if the hardware supports LTE */
	BSFEATURE_TDSCDMA,				/* if the hardware supports TDSCDMA */
	BSFEATURE_UBIST,					/* if Dell UBIST is supported */
	BSFEATURE_GPSSEL,					/* if GPS antenna selection is supported */
	BSFEATURE_SIMHOTSWAP,			/* if hardware supports SIM detection via GPIO */
	BSFEATURE_WM8944,					/* if WM8944 codec is supported */
	BSFEATURE_SEGLOADING,			/* if Segment Loading Feature enabled*/
	BSFEATURE_WWANLED,				/* if WWANLED is supported */
	BSFEATURE_POWERFAULT,			/* if POWERFAULT is supported */
	BSFEATURE_MAX							/* Used for bounds checking */
};

#include "sierra_bsuproto.h"
#endif
