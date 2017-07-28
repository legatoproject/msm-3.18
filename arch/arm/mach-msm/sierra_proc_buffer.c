/* arch/arm/mach-msm/sierra_proc_buffer.c
 *
 * Copyright (c) 2012 Sierra Wireless, Inc
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <mach/sierra_proc_buffer.h>

char global_buffer[SIERRA_PROC_STRING_MAX_LEN+1];

struct proc_dir_entry *proc_file;

static int sierra_proc_buf_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", global_buffer);
	return 0;
}

static int sierra_proc_buffer_open(struct inode *inode, struct file *file)
{
	return single_open(file, sierra_proc_buf_show, NULL);
}

int proc_write_buf(struct file *file, const char __user *buffer, unsigned long count,
				void *data) {
	int len;

	if (count >= SIERRA_PROC_STRING_MAX_LEN)
		len = SIERRA_PROC_STRING_MAX_LEN - 1;
	else
		len = count;

	/*
	* copy_from_user copy user space to kernel space
	* save user write data in global_buffer
	*/
	copy_from_user(global_buffer, buffer, len);
	global_buffer[len] = '\0';
	return len;
}

static const struct file_operations sierra_proc_buf_fops = {
	.owner = THIS_MODULE,
	.open =  sierra_proc_buffer_open,
	.read  = seq_read,
	.write = proc_write_buf,
};

static int __init proc_test_init(void) {
	proc_file = proc_create("proc_buffer", S_IRUGO, NULL, &sierra_proc_buf_fops);

	return 0;
}

static void __exit proc_test_exit(void) {
	remove_proc_entry("proc_buffer", NULL);
}

int sierra_get_dual_system_proc_buffer(char * proc_buffer, int len)
{
	int length = SIERRA_PROC_STRING_MAX_LEN;

	if(NULL == proc_buffer)
	{
		pr_err( "%s:proc_buffer is null\n",__func__);
		return -1;
	}
	pr_info( "%s:proc_buffer starts to fill value\n",__func__);

	if(length >= len)
	{
		memcpy(proc_buffer, global_buffer, SIERRA_PROC_STRING_MAX_LEN);
	}
	else
	{
		pr_err( "%s:copy buffer len can't match\n",__func__);
	}

	return 0;
}
EXPORT_SYMBOL(sierra_get_dual_system_proc_buffer);

MODULE_LICENSE("GPL");
module_init(proc_test_init);
module_exit(proc_test_exit);

