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
#include <mach/sierra_bsidefs.h>
#include <mach/sierra_smem.h>
#include <linux/sierra_bsudefs.h>

/* RAM Copies of HW type, rev, etc. */
/* note that there is a copy for the bootloader and another for the app */
union bs_hwconfig_s bs_hwcfg;
bool bs_hwcfg_read = false;

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
				result = (uint64_t)(mp->bcgpioflag[0]) | ((uint64_t)(mp->bcgpioflag[1]) << 32);
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

/************
 *
 * Name:     bs_hwtype_get
 *
 * Purpose:  Returns hardware type read from QFPROM
 *
 * Parms:    none
 *
 * Return:   hardware type
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
enum bshwtype bs_hwtype_get (void)
{
	if (!bs_hwcfg_read)
	{
		bs_hwcfg.all = sierra_smem_get_hwconfig();
		bs_hwcfg_read = true;
	}

	return (enum bshwtype) bs_hwcfg.hw.type;
}
EXPORT_SYMBOL(bs_hwtype_get);

/************
 *
 * Name:     bs_prod_family_get
 *
 * Purpose:  Returns product family read from QFPROM / SMEM
 *
 * Parms:    none
 *
 * Return:   product family
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
enum bs_prod_family_e bs_prod_family_get (void)
{
	if (!bs_hwcfg_read)
	{
		bs_hwcfg.all = sierra_smem_get_hwconfig();
		bs_hwcfg_read = true;
	}

	return (enum bs_prod_family_e) bs_hwcfg.hw.family;
}
EXPORT_SYMBOL(bs_prod_family_get);


/************
 *
 * Name:     bs_support_get
 *
 * Purpose:  To check if the hardware supports a particular feature
 *
 * Parms:    feature - feature to check
 *
 * Return:   true if hardware supports this feature
 *           false otherwise
 *
 * Abort:    none
 *
 * Notes:    This function is primarily designed to keep hardware variant
 *           checks to a central location.
 *
 ************/
bool bs_support_get (enum bsfeature feature)
{
	bool supported = false;
	enum bs_prod_family_e prodfamily;
	enum bshwtype hwtype;

	prodfamily = bs_prod_family_get();
	hwtype = bs_hwtype_get();

	switch (feature)
	{
		case BSFEATURE_AR:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_AR:
					supported = true;
					break;
				default:
					break;
			}
			break;

		case BSFEATURE_WP:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_WP:
					supported = true;
					break;
				default:
					break;
			}
			break;
/*SWI_TBD BChen LXSWI9X28-9 [2016-12-08] sync code from modem_proc/sierra/bs/src/bsuser.c
 *disable the redundant judgement ATM
 */
#if 0
		case BSFEATURE_MINICARD:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_MC:
					supported = true;
					break;
				default:
					break;
			}
			break;

		case BSFEATURE_EM:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_EM:
					supported = true;
					break;
				default:
					break;
			}
			break;

		case BSFEATURE_W_DISABLE:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_EM:
				case BS_PROD_FAMILY_MC:
				case BS_PROD_FAMILY_WP:
					supported = true;
					break;
				default:
					break;
			}
			break;

		case BSFEATURE_WWANLED:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_WP:
					supported = true;
					break;
				default:
					break;
			}
			break;

		case BSFEATURE_VOICE:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_HSUPA:
			supported = true;
			break;

		case BSFEATURE_GPIOSAR:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_RMAUTOCONNECT:
			supported = true;
			break;

		case BSFEATURE_UART:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_AR:
				case BS_PROD_FAMILY_WP:
					supported = true;
					break;

				default:
					break;
			}
			break;

		case BSFEATURE_ANTSEL:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_INSIM:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_OOBWAKE:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_WP:
					supported = true;
					break;

				default:
					break;
			}
			break;

		case BSFEATURE_CDMA:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_GSM:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_WCDMA:
			supported = true;
			break;

		case BSFEATURE_LTE:
			supported = true;
			break;

		case BSFEATURE_TDSCDMA:
			switch (hwtype)
			{
					case BSAR7586:
						supported = true;
						break;
					default:
						supported = FALSE;
						break;
			}
			break;

		case BSFEATURE_UBIST:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_GPSSEL:
			switch (hwtype)
			{
				default:
					break;
			}
			break;

		case BSFEATURE_SIMHOTSWAP:
			switch (hwtype)
			{
				default:
					supported = true;
					break;
			}
			break;

		case BSFEATURE_SEGLOADING:
			switch (hwtype)
			{
				case BSAR7586:
					supported = true;
					break;
				default:
					supported = FALSE;
					break;
			}
			break;

		case BSFEATURE_POWERFAULT:
			switch (prodfamily)
			{
				case BS_PROD_FAMILY_AR:
					supported = true;
					break;

				default:
					supported = FALSE;
					break;
			}
			break;
#endif
		default:
			break;
	}

	return supported;
}
EXPORT_SYMBOL(bs_support_get);

/************
 *
 * Name:     bs_uart_fun_get()
 *
 * Purpose:  Provide to get UARTs function seting
 *
 * Parms:    uart Number
 *
 * Return:   UART function
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
int8_t bs_uart_fun_get (uint uart_num)
{
	struct bccoworkmsg *mp;
	unsigned char *virtual_addr;

	if (uart_num >= BS_UART_LINE_MAX) {
		return -1;
	}

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
				return (int8_t)mp->bcuartfun[uart_num];
			} else {
				printk(KERN_ERR"sierra:-%s-failed: crc error", __func__);
				return -1;
			}
		} else {
			printk(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
			return -1;
		}
	} else {
		printk(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
		return -1;
	}
}
EXPORT_SYMBOL(bs_uart_fun_get);
