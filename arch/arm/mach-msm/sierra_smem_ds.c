/* arch/arm/mach-msm/sierra_smem_ds.c
 *
 * Sierra SMEM DS functions used to set/get dual system flags.
 *
 * Copyright (c) 2016 Sierra Wireless, Inc
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
#include <linux/reboot.h>

static struct ds_smem_message_s *sierra_smem_ds_get_buf(void)
{
	struct ds_smem_message_s * ds_smem_bufp = NULL;
	unsigned char *virtual_addr = NULL;

	/* Get DS SMEM region */
	virtual_addr = sierra_smem_base_addr_get();
	if (NULL != virtual_addr) {
		virtual_addr += BSMEM_DSSD_OFFSET;
		ds_smem_bufp = (struct ds_smem_message_s *)virtual_addr;
	}
	else {
		pr_debug(KERN_ERR"sierra:-%s-failed: get virtual_addr error", __func__);
		return NULL;
	}

	/* Make sure it is valid DS SMEM */
	if (ds_smem_bufp->magic_beg == DS_MAGIC_NUMBER &&
		ds_smem_bufp->magic_end == DS_MAGIC_NUMBER ) {
		/* doube check CRC */
		if (ds_smem_bufp->crc32 == crc32_le(~0, (void *)ds_smem_bufp, (sizeof(struct ds_smem_message_s) - sizeof(uint32_t)))) {
			return ds_smem_bufp;
		}
		else {
			pr_debug(KERN_ERR"sierra:-%s-failed: crc error", __func__);
		}
	}
	else {
		pr_debug(KERN_ERR"sierra:-%s-failed: smem have not initized", __func__);
	}

	return NULL;
}

int sierra_smem_ds_get_ssid(uint8_t *modem_idx, uint8_t *lk_idx, uint8_t *linux_idx)
{
	struct ds_smem_message_s * ds_smem_bufp = NULL;

	/* Get DS SMEM region */
	ds_smem_bufp = sierra_smem_ds_get_buf();
	if (NULL == ds_smem_bufp) {
		pr_debug(KERN_ERR"sierra:-%s-failed: can't get ds smem buffer", __func__);
		return -1;
	}

	/* Get SSID */
	if (NULL != modem_idx) {
		*modem_idx = ds_smem_bufp->ssid_modem_idx;
	}

	if (NULL != lk_idx) {
		*lk_idx = ds_smem_bufp->ssid_lk_idx;
	}

	if (NULL != linux_idx) {
		*linux_idx = ds_smem_bufp->ssid_linux_idx;
	}

	return 0;
}
EXPORT_SYMBOL(sierra_smem_ds_get_ssid);

/************
 *
 * Name:     sierra_ds_smem_erestore_info_set
 *
 * Purpose:  Set smem of BS_SMEM_REGION_EFS_RESTORE
 *
 * Parms:    [IN] value_type - type of the member
 *           [OUT]value      - store value get from smem
 *
 * Return:   0 - set successfully
 *          -1 - set failed
 *
 * Abort:    None
 *
 * Notes:    None
 *
 ************/
int sierra_ds_smem_erestore_info_set(uint32_t value_type, uint8_t value)
{
	struct ds_smem_erestore_info *efs_restore = NULL;
	unsigned char *virtual_addr = NULL;

	virtual_addr = sierra_smem_base_addr_get();
	/* Check if get the region */
	if(NULL != virtual_addr)
	{
		virtual_addr += BSMEM_EFS_RESTORE_OFFSET;
		/* Get the region */
		efs_restore = (struct ds_smem_erestore_info*)virtual_addr;
		/* Check if wether initialized. If not, initialize first */
		if((DS_MAGIC_EFSB != efs_restore->magic_beg) ||
			(DS_MAGIC_EFSE != efs_restore->magic_end) ||
			(crc32_le(~0, (void *)efs_restore, DS_ERESTORE_CRC_SZ) != efs_restore->crc32))
		{
			efs_restore->magic_beg          = DS_MAGIC_EFSB;
			efs_restore->magic_end          = DS_MAGIC_EFSE;
			efs_restore->erestore_t         = BL_RESTORE_INFO_INVALID_VALUE;
			efs_restore->errorcount         = BL_RESTORE_INFO_INVALID_VALUE;
			efs_restore->restored_flag      = BL_RESTORE_INFO_INVALID_VALUE;
			efs_restore->beroption          = BL_RESTORE_INFO_INVALID_VALUE;
			efs_restore->crc32              = crc32_le(0, (void *)efs_restore, DS_ERESTORE_CRC_SZ);
		}

		if((BL_RESTORE_INFO_MIN > value_type) || (BL_RESTORE_INFO_MAX < value_type))
		{
			/* Bad paramaters */
			pr_debug(KERN_ERR"sierra:-%s-failed: invalid bl_erestore_info_type %d: %d", __func__, value_type, value);
			return -1;
		}
		else
		{
			if((BL_RESTORE_INFO_RESTORE_TYPE == value_type) && (DS_RESTORE_EFS_TYPE_MIN < value) && (DS_RESTORE_EFS_TYPE_MAX > value))
			{
			/* Set efs restore info BL_RESTORE_INFO_RESTORE_TYPE */
			efs_restore->magic_beg  = DS_MAGIC_EFSB;
			efs_restore->magic_end  = DS_MAGIC_EFSE;
			efs_restore->erestore_t = value;
			efs_restore->crc32      = crc32_le(~0, (void *)efs_restore, DS_ERESTORE_CRC_SZ);
			pr_debug("sierra:-%s-successed: BL_RESTORE_INFO_RESTORE_TYPE: %d", __func__, value);
			}
			else if ((BL_RESTORE_INFO_RESTORE_DONE == value_type) && (BL_RESTORE_INFO_STAGE_MIN > value) && (BL_RESTORE_INFO_STAGE_MIN < value))
			{
			/* Set efs restore info BL_RESTORE_INFO_RESTORE_TYPE */
			efs_restore->magic_beg  = DS_MAGIC_EFSB;
			efs_restore->magic_end  = DS_MAGIC_EFSE;
			efs_restore->restored_flag  = value;
			efs_restore->crc32      = crc32_le(~0, (void *)efs_restore, DS_ERESTORE_CRC_SZ);
			pr_debug("sierra:-%s-successed: BL_RESTORE_INFO_RESTORE_DONE: %d", __func__, value);
			}
			else if (BL_RESTORE_INFO_ECOUNT_BUF == value_type)
			{
			/* Set efs restore info BL_RESTORE_INFO_ECOUNT_BUF */
			efs_restore->magic_beg  = DS_MAGIC_EFSB;
			efs_restore->magic_end  = DS_MAGIC_EFSE;
			efs_restore->errorcount = value;
			efs_restore->crc32      = crc32_le(~0, (void *)efs_restore, DS_ERESTORE_CRC_SZ);
			pr_debug("sierra:-%s-successed: BL_RESTORE_INFO_ECOUNT_BUF: %d", __func__, value);
			}
			else if (BL_RESTORE_INFO_BEROPTION_BUF == value_type)
			{
				/* Set efs restore info BL_RESTORE_INFO_BEROPTION_BUF */
				efs_restore->magic_beg  = DS_MAGIC_EFSB;
				efs_restore->magic_end  = DS_MAGIC_EFSE;
				efs_restore->beroption  = value;
				efs_restore->crc32      = crc32_le(~0, (void *)efs_restore, DS_ERESTORE_CRC_SZ);
				pr_debug("sierra:-%s-successed: BL_RESTORE_INFO_BEROPTION_BUF: %d\n", __func__, value);
			}
			else
			{
				pr_err("sierra:-%s-failed: invalid parameter bl_erestore_info_type %d: %d", __func__, value_type, value);
			}

			return 0;
		}
	}
	else
	{
		pr_err("sierra:-%s-failed: get virtual_addr error", __func__);
		return -1;
	}
}

int sierra_smem_ds_write_bad_image_and_swap(uint64_t bad_image_mask)
{
	struct ds_smem_message_s * ds_smem_bufp = NULL;
	uint8_t need_erestore_type = DS_RESTORE_EFS_TYPE_MIN;

	/* Get DS SMEM region */
	ds_smem_bufp = sierra_smem_ds_get_buf();
	if (NULL == ds_smem_bufp) {
		pr_debug(KERN_ERR"sierra:-%s-failed: can't get ds smem buffer", __func__);
		return -1;
	}

	/* Swap system next boot up as bad image detected */
	if (DS_SSID_SUB_SYSTEM_1 == ds_smem_bufp->ssid_modem_idx) {
		ds_smem_bufp->ssid_modem_idx = DS_SSID_SUB_SYSTEM_2;
	}
	else {
		ds_smem_bufp->ssid_modem_idx = DS_SSID_SUB_SYSTEM_1;
	}

	if (DS_SSID_SUB_SYSTEM_1 == ds_smem_bufp->ssid_lk_idx) {
		ds_smem_bufp->ssid_lk_idx = DS_SSID_SUB_SYSTEM_2;
	}
	else {
		ds_smem_bufp->ssid_lk_idx = DS_SSID_SUB_SYSTEM_1;
	}

	if (DS_SSID_SUB_SYSTEM_1 == ds_smem_bufp->ssid_linux_idx) {
		ds_smem_bufp->ssid_linux_idx = DS_SSID_SUB_SYSTEM_2;
	}
	else {
		ds_smem_bufp->ssid_linux_idx = DS_SSID_SUB_SYSTEM_1;
	}

	if (DS_IMAGE_CLEAR_FLAG == bad_image_mask) {
		ds_smem_bufp->bad_image = DS_IMAGE_CLEAR_FLAG;
	}
	else if (DS_IMAGE_FLAG_NOT_SET == bad_image_mask) {
		/* Do nothing */
	}
	else {
		ds_smem_bufp->bad_image |= bad_image_mask;
	}

	ds_smem_bufp->swap_reason = DS_SWAP_REASON_BAD_IMAGE;
	ds_smem_bufp->is_changed = DS_BOOT_UP_CHANGED;
	ds_smem_bufp->crc32 = crc32_le(~0, (void *)ds_smem_bufp, (sizeof(struct ds_smem_message_s) - sizeof(uint32_t)));

	need_erestore_type = DS_RESTORE_EFS_SANITY_FROM_KERNEL;
	sierra_ds_smem_erestore_info_set(BL_RESTORE_INFO_RESTORE_TYPE,need_erestore_type);

	return 0;
}
EXPORT_SYMBOL(sierra_smem_ds_write_bad_image_and_swap);

/**
 * ubi_check_bad_image_and_swap - check badiamge and swap.
 *
 *
 * This function check bad image in ubi partition,include
 * modem, rootfd and legato
 */
void ubi_check_bad_image_and_swap(char *ubi_name)
{
	uint64_t bad_image_mask = DS_IMAGE_FLAG_NOT_SET;
	uint8_t linux_index = 0;
	uint8_t modem_index = 0;

	if (0 == sierra_smem_ds_get_ssid(&modem_index, NULL, &linux_index)) {
			if (0 == strcmp(ubi_name, "rootfs")) {
				if (DS_SSID_SUB_SYSTEM_2 == linux_index) {
					bad_image_mask = DS_IMAGE_SYSTEM_2;
				}
				else {
					bad_image_mask = DS_IMAGE_SYSTEM_1;
				}
			}
			else if (0 == strcmp(ubi_name, "modem")) {
				if (DS_SSID_SUB_SYSTEM_2 == modem_index) {
					bad_image_mask = DS_IMAGE_MODEM_2;
				}
				else {
					bad_image_mask = DS_IMAGE_MODEM_1;
				}
			}
			else if (0 == strcmp(ubi_name, "legato")) {
				if (DS_SSID_SUB_SYSTEM_2 == linux_index) {
					bad_image_mask = DS_IMAGE_USERDATA_2;
				}
				else {
					bad_image_mask = DS_IMAGE_USERDATA_1;
				}
			}
			if (DS_IMAGE_FLAG_NOT_SET != bad_image_mask) {
				sierra_smem_ds_write_bad_image_and_swap(bad_image_mask);
				emergency_restart();
			}
	}
	else {
		panic("smem can't visit, system crash");
	}

	return;
}

EXPORT_SYMBOL(ubi_check_bad_image_and_swap);

int sierra_smem_handle_bad_partition_name(char * partition_name)
{
	uint64_t bad_image_mask = DS_IMAGE_FLAG_NOT_SET;
	uint8_t linux_index = 0;
	uint8_t modem_index = 0;
	uint8_t swap_flag = 0;
	int ret = 0;

	if(NULL == partition_name) {
		pr_err(" %s partition name is null",__func__);
		return -1;
	}

	if (0 == sierra_smem_ds_get_ssid(&modem_index, NULL, &linux_index)) {
		if (0 == strcmp(partition_name, "system")) {
			pr_err("sierra:-%s-failed: system", __func__);
			bad_image_mask = DS_IMAGE_SYSTEM_1;
			if (DS_SSID_SUB_SYSTEM_1 == linux_index) {
				swap_flag = 1;
			}
		}
		else if (0 == strcmp(partition_name, "system2")) {
			pr_err("sierra:-%s-failed: system2", __func__);
			bad_image_mask = DS_IMAGE_SYSTEM_2;
			if (DS_SSID_SUB_SYSTEM_2 == linux_index) {
				swap_flag = 1;
			}
		}
		else if (0 == strcmp(partition_name, "modem")) {
			pr_err("sierra:-%s-failed: modem", __func__);
			bad_image_mask = DS_IMAGE_MODEM_1;
			if (DS_SSID_SUB_SYSTEM_1 == modem_index) {
				swap_flag = 1;
			}
		}
		else if (0 == strcmp(partition_name, "modem2")) {
			pr_err("sierra:-%s-failed: modem2", __func__);
			bad_image_mask = DS_IMAGE_MODEM_2;
			if (DS_SSID_SUB_SYSTEM_2 == modem_index) {
				swap_flag = 1;
			}
		}
		else if (0 == strcmp(partition_name, "lefwkro")) {
			pr_err("sierra:-%s-failed: lefwkro", __func__);
			bad_image_mask = DS_IMAGE_USERDATA_1;
			if (DS_SSID_SUB_SYSTEM_1 == linux_index) {
				swap_flag = 1;
			}
		}
		else if (0 == strcmp(partition_name, "lefwkro2")) {
			pr_err("sierra:-%s-failed: lefwkro2", __func__);
			bad_image_mask = DS_IMAGE_USERDATA_2;
			if (DS_SSID_SUB_SYSTEM_2 == linux_index) {
				swap_flag = 1;
			}
		}

		/* It is APP's response to deal with bad image mask when bad image raised in UD system.*/
		if (swap_flag) {
			if (DS_IMAGE_FLAG_NOT_SET != bad_image_mask) {
				sierra_smem_ds_write_bad_image_and_swap(bad_image_mask);
				emergency_restart();
			}
		}
		else {
			pr_err("sierra:-%s-failed: UD system is not current system", __func__);
			ret = -EIO;
		}
	}
	else {
		panic("sierra_smem_handle_bad_partition_name smem can't visit, system crash");
	}

	return ret;
}

EXPORT_SYMBOL(sierra_smem_handle_bad_partition_name);

