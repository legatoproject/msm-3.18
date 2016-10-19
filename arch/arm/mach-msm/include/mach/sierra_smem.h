/* arch/arm/mach-msm/sierra_smem.h
 *
 * Copyright (C) 2012 Sierra Wireless, Inc
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

#ifndef SIERRA_SMEM_H
#define SIERRA_SMEM_H

void sierra_smem_errdump_save_start(void);
void sierra_smem_errdump_save_timestamp(uint32_t time_stamp);
void sierra_smem_errdump_save_errstr(char *errstrp);
void sierra_smem_errdump_save_auxstr(char *errstrp);
void sierra_smem_errdump_save_frame(void *taskp, void *framep);
int  sierra_smem_get_download_mode(void);
int sierra_smem_boothold_mode_set(void);
int sierra_smem_warm_reset_cmd_get(void);
int sierra_smem_im_recovery_mode_set(void);
unsigned char * sierra_smem_base_addr_get(void);
bool sierra_bs_voice_feature_get(void);
unsigned char *sierra_smem_usb_addr_get(void);

#endif /* SIERRA_SMEM_H */
