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
#include <mach/sierra_bsidefs.h>

/* RAM Copies of HW type, rev, etc. */
/* note that there is a copy for the bootloader and another for the app */
bool bshwconfigread = false;
union bshwconfig bscfg;

/* Local structures and functions */
/************
 *
 * Name:     bsreadhwconfig
 *
 * Purpose:  To get the hardware configuration from gpio
 *
 * Parms:    none
 *
 * Return:   uint32 bitmask of hardware configuration
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
static ssize_t bsreadhwconfig(void)
{
  struct bc_smem_message_s *msgp;
  unsigned char *virtual_addr;

  virtual_addr = sierra_smem_base_addr_get();
  if (virtual_addr)
  {
    /*  APPL mailbox */
    virtual_addr += BSMEM_MSG_OFFSET;

    msgp = (struct bc_smem_message_s *)virtual_addr;

    if (msgp->magic_beg == BC_SMEM_MSG_MAGIC_BEG &&
        msgp->magic_end == BC_SMEM_MSG_MAGIC_END )
    {
      /* doube check CRC */
      if (msgp->crc32 == crc32_le(~0, (void *)msgp, BC_MSG_CRC_SZ))
      {
        if (BC_MSG_HWCONFIG_INVALID != msgp->in.hwconfig)
        {
          return msgp->in.hwconfig;
        }
      }
      else
      {
        pr_err(KERN_ERR"sierra:-%s-failed: crc error", __func__);
        return 0;
      }
    }
    else
    {
      pr_err(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
      return 0;
    }
  }
  else
  {
    pr_err(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
    return 0;
  }
}

/************
 *
 * Name:     bsgethwtype
 *
 * Purpose:  Returns hardware type read from QFPROM /GPIO
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
uint8_t bsgethwtype(
  void)
{
  if (bshwconfigread == false)
  {
    bscfg.all = bsreadhwconfig();
    bshwconfigread = true;
  }

  return bscfg.hw.type;
}
EXPORT_SYMBOL(bsgethwtype);

/************
 *
 * Name:     bsgethwrev
 *
 * Purpose:  Returns hardware revision read from QFPROM /GPIO
 *
 * Parms:    none
 *
 * Return:   hardware ID
 *
 * Abort:    none
 *
 * Notes:
 *
 ************/
uint8_t bsgethwrev(
  void)
{
  if (bshwconfigread == false)
  {
    bscfg.all = bsreadhwconfig();
    bshwconfigread = true;
  }

  return bscfg.hw.rev;
}
EXPORT_SYMBOL(bsgethwrev);

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
  uint8_t *p;

  virtual_addr = sierra_smem_base_addr_get();
  if (virtual_addr)
  {
    /*  APPL mailbox */
    virtual_addr += BSMEM_COWORK_OFFSET;

    mp = (struct bccoworkmsg *)virtual_addr;

    if (mp->magic_beg == BS_SMEM_COWORK_MAGIC_BEG &&
        mp->magic_end == BS_SMEM_COWORK_MAGIC_END )
    {
      /* doube check CRC */
      if (mp->crc32 == crc32_le(~0, (void *)mp, BS_COWORK_CRC_SIZE))
      {
        /* Add lower bits */
        result = (uint64_t)(mp->bcgpioflag & 0x0000FFFF);

        /* Add the extension flags, bits 16-47 */
        p = &mp->bcgpioflag_ext;
        result += (uint64_t)*p++ << 16;
        result += (uint64_t)*p++ << 24;
        result += (uint64_t)*p++ << 32;
        result += (uint64_t)*p++ << 40;
      }
      else
      {
        pr_err(KERN_ERR"sierra:-%s-failed: crc error", __func__);
        return 0;
      }
    }
    else
    {
      pr_err(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
      return 0;
    }
  }
  else
  {
    pr_err(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
    return 0;
  }

  return result;
}
EXPORT_SYMBOL(bsgetgpioflag);

