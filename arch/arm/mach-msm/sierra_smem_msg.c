/* arch/arm/mach-msm/sierra_smem_msg.c
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

#include <mach/sierra_smem.h>

int sierra_smem_get_download_mode(void)
{
        struct bc_smem_message_s *b2amsgp;
        unsigned char *virtual_addr;
        int download_mode = 0;

        virtual_addr = sierra_smem_base_addr_get();
        if (virtual_addr) {

                /*  APPL mailbox */
                virtual_addr += BSMEM_MSG_APPL_MAILBOX_OFFSET;

                b2amsgp = (struct bc_smem_message_s *)virtual_addr;

                if (b2amsgp->magic_beg == BC_SMEM_MSG_MAGIC_BEG &&
                    b2amsgp->magic_end == BC_SMEM_MSG_MAGIC_END &&
                    (b2amsgp->in.flags & BC_MSG_B2A_DLOAD_MODE)) {
                        download_mode = 1;
                }
        }

        return download_mode;
}
EXPORT_SYMBOL(sierra_smem_get_download_mode);

int sierra_smem_boothold_mode_set(void)
{
        struct bc_smem_message_s *a2bmsgp;
        uint64_t a2bflags = 0;
        unsigned char *virtual_addr;

        virtual_addr = sierra_smem_base_addr_get();
        if (virtual_addr) {

                /*  APPL mailbox */
                virtual_addr += BSMEM_MSG_APPL_MAILBOX_OFFSET;
        }
        else {

                return -1;
        }

        a2bmsgp = (struct bc_smem_message_s *)virtual_addr;

        /* SWI_TBD bdu 2014:12:17 - might need lock here, need to unify with 
         * bcmsg.c defines and APIs 
         */
        if (a2bmsgp->magic_beg == BC_SMEM_MSG_MAGIC_BEG &&
            a2bmsgp->magic_end == BC_SMEM_MSG_MAGIC_END) {

                a2bflags = a2bmsgp->out.flags;
        }
        else {

                memset((void *)a2bmsgp, 0, sizeof(struct bc_smem_message_s));
                a2bmsgp->in.launchcode  = BC_MSG_LAUNCH_CODE_INVALID;
                a2bmsgp->in.recover_cnt = BC_MSG_RECOVER_CNT_INVALID;
                a2bmsgp->in.hwconfig    = BC_MSG_HWCONFIG_INVALID;
                a2bmsgp->in.usbdescp    = BC_MSG_USB_DESC_INVALID;
                a2bmsgp->out.launchcode  = BC_MSG_LAUNCH_CODE_INVALID;
                a2bmsgp->out.recover_cnt = BC_MSG_RECOVER_CNT_INVALID;
                a2bmsgp->out.hwconfig    = BC_MSG_HWCONFIG_INVALID;
                a2bmsgp->out.usbdescp    = BC_MSG_USB_DESC_INVALID;
                a2bmsgp->version   = BC_SMEM_MSG_VERSION;
                a2bmsgp->magic_beg = BC_SMEM_MSG_MAGIC_BEG;
                a2bmsgp->magic_end = BC_SMEM_MSG_MAGIC_END;
                a2bflags = 0;
        }

        a2bflags |= BC_MSG_A2B_BOOT_HOLD;
        a2bmsgp->out.flags = a2bflags;
  
        return 0;
}
EXPORT_SYMBOL(sierra_smem_boothold_mode_set);

int sierra_smem_im_recovery_mode_set(void)
{
        struct imsw_smem_im_s *immsgp;
        unsigned char *virtual_addr;

        virtual_addr = sierra_smem_base_addr_get();
        if (virtual_addr) {

                /*  IM region */
                virtual_addr += BSMEM_IM_OFFSET;
        }
        else {

                return -1;
        }

        immsgp = (struct imsw_smem_im_s *)virtual_addr;
        /* reinit IM region since boot will reconstruct it anyway */
        memset((void *)immsgp, 0, sizeof(struct imsw_smem_im_s));

        /* set magic numbers: start, end, recovery */
        immsgp->magic_beg = IMSW_SMEM_MAGIC_BEG;
        immsgp->magic_recovery = IMSW_SMEM_MAGIC_RECOVERY;
        immsgp->magic_end = IMSW_SMEM_MAGIC_END;
        /* precalcualted CRC */
        immsgp->crc32 = IMSW_SMEM_RECOVERY_CRC;

        return 0;
}
EXPORT_SYMBOL(sierra_smem_im_recovery_mode_set);
