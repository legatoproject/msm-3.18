/* kernel/include/linux/sierra_bsuproto.h
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

#ifndef BS_UPROTO_H
#define BS_UPROTO_H

extern uint8_t bsgethwtype(void);
extern uint8_t bsgethwrev(void);
extern uint64_t bsgetgpioflag(void);
extern void bsseterrcount(unsigned int err_cnt);
#endif
