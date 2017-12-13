/* arch/arm/mach-msm/sierra_qfuse.c
 *
 * OEM QFPROM read/write functions
 *
 * Copyright (c) 2014 Sierra Wireless, Inc
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
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <soc/qcom/scm.h>
#include <asm/cacheflush.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/msm_sierra.h>

#define SCM_QFPROM_WRITE_ROW_CMD       0x3
#define SCM_QFPROM_READ_ROW_CMD        0x5
#define QFPROM_NO_ERR                  0
#define TZFUSE_CORR_ADDR_START         0x000A4000

static DEFINE_MUTEX(ioctl_lock);

uint32_t api_results_g;
uint64_t rowdata_g;

ssize_t sierra_qfuse_read_row(uint32_t row_address, uint64_t *fuse_pidp)
{
        ssize_t ret = 0;

        struct {
                uint32_t   row_address;
                int32_t    addr_type;
                uint32_t   *row_data;
                uint32_t   *qfprom_api_status;
        } fuse_read_cmd_buf;

        fuse_read_cmd_buf.row_address = row_address;

        if (row_address >= TZFUSE_CORR_ADDR_START) {
                fuse_read_cmd_buf.addr_type = 1;   /* QFPROM_ADDR_SPACE_CORR */
        }
        else {
                fuse_read_cmd_buf.addr_type = 0;   /* QFPROM_ADDR_SPACE_RAW */
        }

        fuse_read_cmd_buf.row_data = (void *)virt_to_phys(&rowdata_g);
        fuse_read_cmd_buf.qfprom_api_status = (void *)virt_to_phys(&api_results_g);

        ret = scm_call(SCM_SVC_FUSE, SCM_QFPROM_READ_ROW_CMD,
                       &fuse_read_cmd_buf, sizeof(fuse_read_cmd_buf), NULL, 0);

        /* Cache maintenance on the command and response buffers is taken care of 
         * by scm_call; however, callers are responsible for any other cached 
         * buffers passed over to the secure world
         * Invalidate cache since TZ touched this address range
         */
        dmac_inv_range((void *)&api_results_g, (void *)&api_results_g + sizeof(api_results_g));
        dmac_inv_range((void *)&rowdata_g, (void *)&rowdata_g + sizeof(rowdata_g));


        if (ret || api_results_g != QFPROM_NO_ERR) {
                pr_err("%s: read row command failed ret %d, result: %d\n", 
                       __func__, ret, api_results_g);
                ret = -EFAULT;
        }
        else {
                pr_info("%s: read row command OK %d, address  %08X: data %016llX\n", 
                        __func__, api_results_g, row_address, rowdata_g);
                *fuse_pidp = rowdata_g;
                ret = sizeof(*fuse_pidp);
        }

        return ret;
}

static ssize_t sierra_qfuse_write_row(uint32_t row_address, uint64_t rowdata)
{
        ssize_t ret = 0;

        struct {
                uint32_t   raw_row_address;
                uint32_t  *row_data;
                uint32_t   bus_clk_khz;
                uint32_t  *qfprom_api_status;
        } fuse_write_cmd_buf;

        fuse_write_cmd_buf.raw_row_address = row_address;
        fuse_write_cmd_buf.row_data = (void*)virt_to_phys(&rowdata_g);
        fuse_write_cmd_buf.bus_clk_khz = 0;
        fuse_write_cmd_buf.qfprom_api_status = (void*)virt_to_phys(&api_results_g);

        pr_info("%s: input fuse data:%016llX\n", __func__, rowdata);
        pr_info("%s: fuse data, virtual addr:%p, phy addr: %p\n", 
                __func__, &rowdata, fuse_write_cmd_buf.row_data);

        rowdata_g = rowdata;
        /* Invalidate cache so that TZ will get the real data */
        dmac_inv_range((void *)&rowdata_g, (void *)&rowdata_g + sizeof(rowdata_g));

        ret = scm_call(SCM_SVC_FUSE, SCM_QFPROM_WRITE_ROW_CMD,
                       &fuse_write_cmd_buf, sizeof(fuse_write_cmd_buf), NULL, 0);

        /* Invalidate cache since TZ touched this address range */
        dmac_inv_range((void *)&api_results_g, (void *)&api_results_g + sizeof(api_results_g));

        if (ret || api_results_g != QFPROM_NO_ERR) {
                pr_info("%s: write row command failed ret %d, result %d, address %08X\n", 
                        __func__, ret, api_results_g, row_address);
                ret = -EFAULT;
        }
        else {
                pr_info("%s: write row command OK %d, address %08X, data %016llX\n", 
                        __func__, api_results_g, row_address, rowdata_g);
                ret = sizeof(rowdata);
        }

        return ret;
}

static long sierra_qfuse_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	ssize_t ret = 0;


        pr_info("%s: cmd %x, %lx\n", __func__, cmd, arg);

	/* Using unlocked_ioctl, we are responsible for locking (the
	 * old .ioctl should not be used)
	 */
	mutex_lock(&ioctl_lock);

	switch (cmd) {
        /* qfuse row read */
	case TZFUSE_IOC_ROW_READ:
        {
                void __user *user_in  = (void __user *)arg;
                struct tzrow_data_s rowdata;
                ssize_t len = 0;

                if (!user_in) {
                        ret = -EINVAL;
                        break;
                }
                
                if (copy_from_user((void *)&rowdata, user_in, sizeof(rowdata))) {
                        printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
                        ret = -EFAULT;
                        break;
                }

                pr_info("%s: row %08X read\n", __func__, rowdata.row_address);

                len = sierra_qfuse_read_row(rowdata.row_address, &rowdata.fusedata);

                if (len < 0) {
                        ret = len;
                        break;
                }

                pr_info("%s: row %08X read, %016llX\n", __func__, rowdata.row_address, rowdata.fusedata);

                if (copy_to_user(user_in, (void *)&rowdata, sizeof(rowdata))) {
                        printk(KERN_ERR "%s: copy_to_user failed\n", __func__);
                        ret = -EFAULT;
                        break;
                }
        }
        break;

        /* qfuse row write */
	case TZFUSE_IOC_ROW_WRITE:
        {
                void __user *user_in  = (void __user *)arg;
                struct tzrow_data_s rowdata;

                if (!user_in) {
                        ret = -EINVAL;
                        break;
                }
                
                if (copy_from_user((void *)&rowdata, user_in, sizeof(rowdata))) {
                        printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
                        ret = -EFAULT;
                        break;
                }

                pr_info("%s: row %08X write, %016llX\n", __func__, rowdata.row_address, rowdata.fusedata);

                if (0 > sierra_qfuse_write_row(rowdata.row_address, rowdata.fusedata)) {
                        ret = -EFAULT;
                        break;
                }
        }
        break;

	default:
		ret = -EINVAL;
	}

	mutex_unlock(&ioctl_lock);
	return ret;
}

static int sierra_qfuse_open(struct inode *inode, struct file *file)
{
        return 0;
}

static int sierra_qfuse_release(struct inode *inode, struct file *file)
{
        return 0;
}

static struct file_operations sierra_qfuse_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = sierra_qfuse_ioctl,
        .open = sierra_qfuse_open,
        .release = sierra_qfuse_release,
};

static struct miscdevice sierra_qfuse_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "sierra_qfuse",
        .fops = &sierra_qfuse_fops,
};

static int __init sierra_qfuse_init(void)
{
        return misc_register(&sierra_qfuse_misc);
}

static void __exit sierra_qfuse_exit(void)
{
        misc_deregister(&sierra_qfuse_misc);
}

module_init(sierra_qfuse_init);
module_exit(sierra_qfuse_exit);

MODULE_AUTHOR("M Luo <mluo@sierrawireless.com>");
MODULE_DESCRIPTION("Sierra Q-Fuse driver");
MODULE_LICENSE("GPL v2");
