/* arch/arm/mach-msm/sierra_smem.c
 *
 * Copyright (c) 2012 Sierra Wireless, Inc
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
#include <linux/io.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#include <mach/sierra_smem.h>

#include <linux/kmsg_dump.h>
#include <linux/slab.h>
struct kmsg_dumper kmsg_dumper;
static unsigned char * smem_base = NULL;

unsigned char * sierra_smem_base_addr_get(void)
{
        if (!smem_base) {
                smem_base = (unsigned char *)ioremap_nocache(SIERRA_SMEM_BASE_PHY, SIERRA_SMEM_SIZE);
                if (!smem_base) {
                        pr_err("sierra_smem_base_addr_get error");
                }
        }
        return smem_base;
}

static ssize_t sierra_smem_read(struct file *fp, char __user *buf,
                                size_t count, loff_t *posp)
{
        unsigned char * memp = smem_base;
        loff_t  pos = *posp;
        ssize_t retval = -EFAULT;

        if (!memp) {
                pr_err("%s: failed to aquire memory region\n", __func__);
                retval = -ENOMEM;
        }
        else if (pos <= SIERRA_SMEM_SIZE) {

                /* truncate to the end of the region, if needed */
                ssize_t len = min(count, (size_t)(SIERRA_SMEM_SIZE - pos));
                memp += pos;

                retval = (ssize_t)copy_to_user(buf, memp, len);

                if (retval) {
                        pr_err("%s: failed to copy %d bytes\n", __func__, (int)retval);
                }
                else {
                        *posp += len;
                        retval = len;
                }
        }

        return retval;
}

/* the write function will be used mainly to write errdump related fields
 * offset field will indicate which field to update
 */
static ssize_t sierra_smem_write(struct file *fp, const char __user *buf,
                                 size_t count, loff_t *posp)
{
        unsigned char * memp = smem_base;
        loff_t  pos = *posp;
        ssize_t retval = -EFAULT;

        if (!memp) {
                pr_err("%s: failed to aquire memory region\n", __func__);
                retval = -ENOMEM;
        }
        else if (pos <= SIERRA_SMEM_SIZE) {
                /* truncate to the end of the region, if needed */
                ssize_t len = min(count, (size_t)(SIERRA_SMEM_SIZE - pos));
                memp += pos;

                retval = (ssize_t)copy_from_user(memp, buf, len);

                if (retval) {
                        pr_err("%s: failed to copy %d bytes\n", __func__, (int)retval);
                }
                else {
                        *posp += len;
                        retval = len;
                }
        }

        return retval;
}

static int sierra_smem_open(struct inode *inode, struct file *file)
{
        if(!smem_base) {
                return -EFAULT;
        }
        else {
                return 0;
        }
}

static int sierra_smem_release(struct inode *inode, struct file *file)
{
        return 0;
}

static struct file_operations sierra_smem_fops = {
        .owner = THIS_MODULE,
        .read = sierra_smem_read,
        .write = sierra_smem_write,
        .llseek = default_llseek,
        .open = sierra_smem_open,
        .release = sierra_smem_release,
};

static struct miscdevice sierra_smem_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "sierra_smem",
        .fops = &sierra_smem_fops,
};

static void sierra_kmsg_dump_cb(struct kmsg_dumper *dumper,
                             enum kmsg_dump_reason reason)
{
        static int crash_counter = 0;
        char *buf,*str,*tmp,*er_base,*kmsgp;
        char error_string[ERROR_STRING_LEN];
        unsigned int len,stack_pre,stack_pos;
        struct sER_DATA *errdatap = (struct sER_DATA *)(smem_base +
                        BSMEM_ERR_OFFSET + BS_SMEM_ERR_DUMP_SIZE);

        crash_counter++;
        if ((!errdatap) || (crash_counter > 1)) {
                pr_err("%s:sierra smem already set kerne crash info,%d\n",
                        __func__,crash_counter);
                return;
        }

        er_base = smem_base + BSMEM_LKC_OFFSET;

        kmsgp = kzalloc(LKC_KMSG_LEN, GFP_KERNEL);
        if (kmsgp == NULL) {
                pr_err("%s: malloc buffer failed!\n", __func__);
                return;
        }
        buf = kmsgp;

        /* get kmsg from log_buf,no need to get all of it, just fetch 16K*/
        kmsg_dump_get_buffer(dumper, true, buf, LKC_KMSG_LEN, &len);

        memset(error_string, 0, ERROR_STRING_LEN);

        /* find out the key word of kernel panic*/
        str = strstr(buf, LKC_PANIC_STR);
        if (str != NULL) {
                if ((str - LKC_STR_EXTR_LEN) > buf)
                        buf = str - LKC_STR_EXTR_LEN;
        }
        else {
                pr_err("%s:can't find the start point str,len=%d\n",
                        LKC_PANIC_STR, len);
                strcpy(error_string, LKC_PANIC_STR);
                /*if kmsg is bigger the smem buffer, get the last  kmsg*/
                if (len > BS_SMEM_LKC_SIZE) {
                        buf = buf + len - BS_SMEM_LKC_SIZE;
                        len = BS_SMEM_LKC_SIZE;
                }
                goto Exit;
        }

        /*format the error string,remove useless string*/
        tmp = strstr(str, "\n");
        if (tmp == NULL)
                goto Exit;
        if ((tmp - str) > ERROR_STRING_LEN)
                memcpy(error_string, str, ERROR_STRING_LEN -1);
        else
                memcpy(error_string, str, tmp - str);

        /*get rid of Stack info, already have in sER_DATA*/
        str = strstr(buf, "Stack:");
        if (str)
        {
                tmp = strstr(str, "[<");
                if (tmp == NULL)
                        tmp = strstr(str, "Code:");
                if (tmp)
                {
                        stack_pre = str - buf -15;
                        stack_pos = buf + len - tmp +15 -1;
                        if (stack_pre > BS_SMEM_LKC_SIZE)
                                stack_pre = BS_SMEM_LKC_SIZE;
                        memcpy(er_base, buf,stack_pre);
                        /*make sure kmsg will not overlay SMEM */
                        if ((stack_pre + stack_pos) > BS_SMEM_LKC_SIZE)
                                stack_pos = BS_SMEM_LKC_SIZE - stack_pre;
                        memcpy(er_base+stack_pre, tmp - 15, stack_pos);
                        
                        /* add err string , most importantly set the marker;
                        * prevent other err info writting.
                        */
                        sierra_smem_errdump_save_errstr(error_string);
                        kfree(kmsgp);
                        return;
                }
        }
Exit:
        sierra_smem_errdump_save_errstr(error_string);

        memset(er_base, 0, BS_SMEM_LKC_SIZE);
        /*check the length of kmsg,and write to SMEM */
        if (len > BS_SMEM_LKC_SIZE)
                len = BS_SMEM_LKC_SIZE;
        memcpy(er_base,buf,len - 1);
        kfree(kmsgp);
}

static int __init sierra_smem_init(void)
{
        (void)sierra_smem_base_addr_get();

        kmsg_dumper.max_reason = KMSG_DUMP_OOPS;
        kmsg_dumper.dump = sierra_kmsg_dump_cb;
        if (kmsg_dump_register(&kmsg_dumper))
                pr_err( "%s:kmsg_dumper register failed\n",__func__);

        return misc_register(&sierra_smem_misc);
}

static void __exit sierra_smem_exit(void)
{
        misc_deregister(&sierra_smem_misc);
}

module_init(sierra_smem_init);
module_exit(sierra_smem_exit);

MODULE_AUTHOR("Brad Du <bdu@sierrawireless.com>");
MODULE_DESCRIPTION("Sierra SMEM driver");
MODULE_LICENSE("GPL v2");

/* export a compatibilty string to allow this driver to reserve the
 * needed physical region via the dev tree
 */
//EXPORT_COMPAT("qcom,sierra-smem");
