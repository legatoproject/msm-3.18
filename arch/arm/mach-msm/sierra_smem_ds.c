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

int sierra_smem_ds_write_bad_image_and_swap(uint64_t bad_image_mask)
{
	struct ds_smem_message_s * ds_smem_bufp = NULL;

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

	return 0;
}
EXPORT_SYMBOL(sierra_smem_ds_write_bad_image_and_swap);

