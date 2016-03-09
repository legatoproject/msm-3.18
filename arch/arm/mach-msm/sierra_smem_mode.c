/* arch/arm/mach-msm/sierra_smem_mode.c
 *
 * Sierra SMEM MSG region mailbox functions used to set/get flags
 * These functions don't rely on Sierra SMEM driver,
 * and can be used in early kernel start
 *
 * Copyright (c) 2015 Sierra Wireless, Inc
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

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/crc32.h>

#include <mach/sierra_smem.h>

int sierra_smem_get_factory_mode(void)
{
  struct bs_smem_mode_switch *bsmode;
  unsigned char *virtual_addr;
  int mode = 0;

  virtual_addr = sierra_smem_base_addr_get();
  if (virtual_addr) {

    /*  APPL mailbox */
    virtual_addr += BSMEM_MODE_OFFSET;

    bsmode = (struct bs_smem_mode_switch *)virtual_addr;

    if (bsmode->magic_beg == BS_SMEM_MODE_MAGIC_BEG &&
        bsmode->magic_end == BS_SMEM_MODE_MAGIC_END ) {
        /* doube check CRC */
      if (bsmode->crc32 == crc32_le(~0, (void *)bsmode, BS_MODE_CRC_SIZE)) {
             mode = (int)bsmode->mode;
      }
      else {
             printk(KERN_ERR"sierra:-%s-failed: crc error", __func__);
             return -1;
      }
    }
    else {
           printk(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
           return -1;
    }
  }
  else {
         printk(KERN_ERR"sierra:-%s-failed: get virtual_add error", __func__);
         return -1;
  }

  return mode;
}
EXPORT_SYMBOL(sierra_smem_get_factory_mode);

