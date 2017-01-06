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

#include "sierra_bsuproto.h"
#endif
