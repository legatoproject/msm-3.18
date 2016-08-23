/* arch/arm/mach-msm/sierra_bscommon.c
 *
 * Copyright (C) 2013 Sierra Wireless, Inc
 * Author: Alex Tan <atan@sierrawireless.com>
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

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <mach/sierra_smem.h>
#include <linux/sierra_bsudefs.h>

/************
 *
 * Name:     bsgetgpioflag()
 *
 * Purpose:  Returns the concatenation of external gpio owner flags
 *
 * Parms:    none
 *
 * Return:   Extern GPIO owner flag
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
uint64_t bsgetgpioflag(void)
{
	struct bccoworkmsg *mp;
	unsigned char *virtual_addr;
	uint64_t result = 0;

	virtual_addr = sierra_smem_base_addr_get();
	if (virtual_addr) {
	 /*  APPL mailbox */
	virtual_addr += BSMEM_COWORK_OFFSET;

	 mp = (struct bccoworkmsg *)virtual_addr;

	if (mp->magic_beg == BS_SMEM_COWORK_MAGIC_BEG &&
		mp->magic_end == BS_SMEM_COWORK_MAGIC_END ) {
	 		/* doube check CRC */
			if (mp->crc32 == crc32_le(~0, (void *)mp, BS_COWORK_CRC_SIZE)) {
				/*get gpio flag*/
				result = (uint64_t)(mp->bcgpioflag[0]) | (uint64_t)(mp->bcgpioflag[1] << 32);
			} else {
				printk(KERN_ERR"sierra:-%s-failed: crc error", __func__);
				return 0;
			}
    		} else {
           		printk(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
           		return 0;
		}
	} else {
		printk(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
		return 0;
	}

	return result;
}
EXPORT_SYMBOL(bsgetgpioflag);

/************
 *
 * Name:     bsgethsicflag()
 *
 * Purpose:  Returns the hsic is enabled or not
 *
 * Parms:    none
 *
 * Return:   returns whether hsic is enabled or not
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
bool bsgethsicflag(void)
{
	struct bccoworkmsg *mp;
	unsigned char *virtual_addr;
	uint32_t result = 0;

	virtual_addr = sierra_smem_base_addr_get();
	if (virtual_addr) {
	 /*  APPL mailbox */
	virtual_addr += BSMEM_COWORK_OFFSET;

	mp = (struct bccoworkmsg *)virtual_addr;

	if (mp->magic_beg == BS_SMEM_COWORK_MAGIC_BEG &&
		mp->magic_end == BS_SMEM_COWORK_MAGIC_END ) {
	 		/* doube check CRC */
			if (mp->crc32 == crc32_le(~0, (void *)mp, BS_COWORK_CRC_SIZE)) {
				/*get HSIC flag*/
				result = (mp->bcfunctions)?1:0;
			} else {
				printk(KERN_ERR"sierra:-%s-failed: crc error", __func__);
				return 0;
			}
    		} else {
           		printk(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
           		return 0;
		}
	} else {
		printk(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
		return 0;
	}

	return result;
}
EXPORT_SYMBOL(bsgethsicflag);

