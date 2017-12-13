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
extern int bsgetgpioflag(uint64_t *gpiomask1, uint64_t *gpiomask2);
extern void bsseterrcount(unsigned int err_cnt);
extern uint32_t bsgetresettypeflag(void);
extern bool bscheckapplresettypeflag(void);
extern void bssetresettype(unsigned int reset_type);
extern int8_t bsgetriowner(void);
extern struct bs_resin_timer bsgetresintimer(void);
extern int8_t bs_uart_fun_get (uint uart_num);
extern bool bsgetpowerfaultflag(void);
extern void bsclearpowerfaultflag(void);
extern bool bsgetbsfunction(uint32_t bitmask);
extern void bsclearbsfunction(uint32_t bitmask);
extern bool bsgetwarmresetflag(void);
extern void bsclearwarmresetflag(void);
#endif
