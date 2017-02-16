/* arch/arm/mach-msm/sierra_bscommon.c
 *
 * Copyright (C) 2016 Sierra Wireless, Inc
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
				result = (mp->bcfunctions & BSFUNCTIONS_HSIC)? 1: 0;
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
 * Name:     bs_support_get
 *
 * Purpose:  To check if the hardware supports a particular feature
 *
 * Parms:    feature - feature to check
 *
 * Return:   TRUE if hardware supports this feature
 *           FALSE otherwise
 *
 * Abort:    none
 *
 * Notes:    Use this function to keep hardware variant dependencies
 *           in a central location.
 *
 ************/
 bool bs_support_get(enum bsfeature feature)
 {
	bool supported = false;
	uint32_t hwconfig;
	uint8_t prod_family, prod_instance;

	hwconfig = sierra_smem_get_hwconfig();
	if (hwconfig == BC_MSG_HWCONFIG_INVALID) {
		pr_err("sierra_smem_get_hwconfig invalid\n");
		return supported;
	}
	prod_family  =  hwconfig & 0xff;
	prod_instance  =  (hwconfig >> 8) & 0xff;

	switch (feature) {
	case BSFEATURE_INTERNALCODEC:
			/* if not a AR family ,assume it doesn't support internal codec*/
			if (prod_family == BS_PROD_FAMILY_AR) {
				switch (prod_instance) {
				case BSAR7582:
				case BSAR7584:
				case BSAR7586:
				case BSAR7586_NB7_NB28:
				case BSAR7588:
				case BSAR8582:
				case BSAR7584_NB28A:
				case BSAR7582_NB13:
					supported = true;
					break;
				}
			}

			pr_debug("hwconfig=%x, prod_family=%d, prod_instance=%d\n",
				hwconfig, prod_family, prod_instance);
		break;

	default:
		break;
	}

	return supported;
 }
EXPORT_SYMBOL(bs_support_get);

/************
 *
 * Name:     bsseterrcount()
 *
 * Purpose:  Set error count
 *
 * Parms:    error count
 *
 * Return:   none
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
void bsseterrcount(unsigned int err_cnt)
{
	struct bc_smem_message_s *b2amsgp;
	unsigned char *virtual_addr;

	virtual_addr = sierra_smem_base_addr_get();
	if (virtual_addr)
	{
		/*  APPL mailbox */
		virtual_addr += BSMEM_MSG_APPL_MAILBOX_OFFSET;
		b2amsgp = (struct bc_smem_message_s *)virtual_addr;

		b2amsgp->out.recover_cnt = err_cnt;
		b2amsgp->crc32 = crc32(~0, (void *)b2amsgp, BC_MSG_CRC_SZ);
	}

	return;
}
EXPORT_SYMBOL(bsseterrcount);

/************
 *
 * Name:     bsgetresettypeflag()
 *
 * Purpose:  Get reset type flag
 *
 * Parms:    none
 *
 * Return:   reset type flag
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
uint32_t bsgetresettypeflag()
{
	struct bc_smem_message_s *b2amsgp;
	unsigned char *virtual_addr;
	unsigned int reset_type_flag = 0;

	virtual_addr = sierra_smem_base_addr_get();
	if (virtual_addr)
	{
	  /*  MODM mailbox */
	  virtual_addr += BSMEM_MSG_MODM_MAILBOX_OFFSET;

	  b2amsgp = (struct bc_smem_message_s *)virtual_addr;

	  if (b2amsgp->magic_beg == BC_SMEM_MSG_MAGIC_BEG &&
		  b2amsgp->magic_end == BC_SMEM_MSG_MAGIC_END &&
		  (b2amsgp->version < BC_SMEM_MSG_CRC32_VERSION_MIN ||
		   b2amsgp->crc32 == crc32(~0, (void *)b2amsgp, BC_MSG_CRC_SZ)))
	  {
		reset_type_flag = b2amsgp->in.brstsetflg;
	  }
	}

	return reset_type_flag;
}
EXPORT_SYMBOL(bsgetresettypeflag);

/************
 *
 * Name:     bssetresettype
 *
 * Purpose:  set reset type
 *
 * Parms:    reset type
 *
 * Return:   none
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
void bssetresettype(unsigned int reset_type)
{
  struct bc_smem_message_s *b2amsgp;
  unsigned char *virtual_addr;

  virtual_addr = sierra_smem_base_addr_get();
  if (virtual_addr)
  {
    /*  APPL mailbox */
    virtual_addr += BSMEM_MSG_APPL_MAILBOX_OFFSET;
    b2amsgp = (struct bc_smem_message_s *)virtual_addr;

    b2amsgp->out.reset_type = reset_type;
    b2amsgp->out.brstsetflg = BS_BCMSG_RTYPE_IS_SET;
    b2amsgp->crc32 = crc32(~0, (void *)b2amsgp, BC_MSG_CRC_SZ);
  }

  return;
}
EXPORT_SYMBOL(bssetresettype);

/************
 *
 * Name:     bsgetriowner()
 *
 * Purpose:  Provide to get RI owner seting
 *
 * Parms:    none
 *
 * Return:   RI owner
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
int8_t bsgetriowner(void)
{
  struct bccoworkmsg *mp;
  unsigned char *virtual_addr;

  virtual_addr = sierra_smem_base_addr_get();
  if (virtual_addr == NULL)
  {
    pr_err(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
    return (int8_t)-1;
  }

  virtual_addr += BSMEM_COWORK_OFFSET;
  mp = (struct bccoworkmsg *)virtual_addr;

  if (mp->magic_beg != BS_SMEM_COWORK_MAGIC_BEG ||
      mp->magic_end != BS_SMEM_COWORK_MAGIC_END )
  {
    pr_err(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
    return (int8_t)-1;
  }

  if (mp->crc32 != crc32_le(~0, (void *)mp, BS_COWORK_CRC_SIZE))
  {
    pr_err(KERN_ERR"sierra:-%s-failed: crc error", __func__);
    return (int8_t)-1;
  }

  return (int8_t)mp->bcriowner;
}

EXPORT_SYMBOL(bsgetriowner);

/************
 *
 * Name:     bs_product_is_ar8582
 *
 * Purpose:  To check if product is AR8582
 *
 * Parms:    none
 *
 * Return:   TRUE if product is AR8582
 *           FALSE otherwise
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
bool bs_product_is_ar8582(void)
{
	bool ret = false;
	uint32_t hwconfig;
	uint8_t prod_family, prod_instance;

	hwconfig = sierra_smem_get_hwconfig();
	if (hwconfig == BC_MSG_HWCONFIG_INVALID) {
		pr_err("sierra_smem_get_hwconfig invalid\n");
		return ret;
	}
	prod_family = hwconfig & 0xff;
	prod_instance = (hwconfig >> 8) & 0xff;

	if (prod_family == BS_PROD_FAMILY_AR) {
		switch (prod_instance) {
		case BSAR8582:
		case BSAR8582_NC:
			ret = true;
			break;
		default:
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL(bs_product_is_ar8582);
