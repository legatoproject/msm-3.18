/* kernel/include/linux/sierra_bsuproto.h
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
 
#ifndef BS_UPROTO_H
#define BS_UPROTO_H

extern uint64_t bsgetgpioflag(void);
extern bool bsgethsicflag(void);
extern void bsseterrcount(unsigned int err_cnt);
extern uint32_t bsgetresettypeflag(void);
extern bool bscheckapplresettypeflag(void);
extern void bssetresettype(unsigned int reset_type);
extern int8_t bsgetriowner(void);
extern bool bs_product_is_ar8582(void);

extern enum bshwtype bs_hwtype_get(void);
extern enum bs_prod_family_e bs_prod_family_get (void);
extern bool bs_support_get (enum bsfeature feature);
extern int8_t bs_uart_fun_get (uint uart_num);

extern struct bs_resin_timer bsgetresintimer(void);
extern bool bsgetpowerfaultflag(void);
extern bool bsclearpowerfaultflag(void);
extern bool bsgetbsfunction(uint32_t bitmask);
extern bool bsclearbsfunction(uint32_t bitmask);
#endif
