/* Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/percpu.h>
#include <linux/of.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/platform_device.h>
#include <soc/qcom/scm.h>
#include <soc/qcom/memory_dump.h>
#include <soc/qcom/watchdog.h>
#ifdef CONFIG_SIERRA
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#endif /*CONFIG_SIERRA*/

#define MODULE_NAME "msm_watchdog"
#define WDT0_ACCSCSSNBARK_INT 0
#define TCSR_WDT_CFG	0x30
#define WDT0_RST	0x04
#define WDT0_EN		0x08
#define WDT0_STS	0x0C
#define WDT0_BARK_TIME	0x10
#define WDT0_BITE_TIME	0x14

#define WDOG_ABSENT	0

#define EN		0
#define UNMASKED_INT_EN 1

#define MASK_SIZE		32
#define SCM_SET_REGSAVE_CMD	0x2
#define SCM_SVC_SEC_WDOG_DIS	0x7
#define MAX_CPU_CTX_SIZE	2048

static struct msm_watchdog_data *wdog_data;

static int cpu_idle_pc_state[NR_CPUS];

struct msm_watchdog_data {
	unsigned int __iomem phys_base;
	size_t size;
	void __iomem *base;
	void __iomem *wdog_absent_base;
	struct device *dev;
	unsigned int pet_time;
	unsigned int bark_time;
	unsigned int bark_irq;
	unsigned int bite_irq;
	bool do_ipi_ping;
	bool wakeup_irq_enable;
	unsigned long long last_pet;
	unsigned min_slack_ticks;
	unsigned long long min_slack_ns;
	void *scm_regsave;
	cpumask_t alive_mask;
	struct mutex disable_lock;
	bool irq_ppi;
	struct msm_watchdog_data __percpu **wdog_cpu_dd;
	struct notifier_block panic_blk;
	bool enabled;
	struct task_struct *watchdog_task;
	struct timer_list pet_timer;
	struct completion pet_complete;
};

/*
 * On the kernel command line specify
 * watchdog_v2.enable=1 to enable the watchdog
 * By default watchdog is turned on
 */
static int enable = 1;
module_param(enable, int, 0);

/*
 * On the kernel command line specify
 * watchdog_v2.WDT_HZ=<clock val in HZ> to set Watchdog
 * ticks. By default it is set to 32765.
 */
static long WDT_HZ = 32765;
module_param(WDT_HZ, long, 0);

/*
 * On the kernel command line specify
 * watchdog_v2.ipi_opt_en=1 to enable the watchdog ipi ping
 * optimization. By default it is turned off
 */
static int ipi_opt_en;
module_param(ipi_opt_en, int, 0);

#ifdef CONFIG_SIERRA
#define MSM_SOFTDOG_MAGIC 'W'
#define GET_MSM_SOFTDOG_MARGIN _IOR(MSM_SOFTDOG_MAGIC, 1, int)
#define SET_MSM_SOFTDOG_MARGIN _IOW(MSM_SOFTDOG_MAGIC, 2, int)
#define STOP_KICK_MSM_SOFTDOG _IO(MSM_SOFTDOG_MAGIC, 3)
#define START_KICK_MSM_SOFTDOG _IO(MSM_SOFTDOG_MAGIC, 4)
/*
 * By default stop to kick the msm_watchdog.
 * When the system starts ok, will start to
 * kick the msm_watchdog
 */
static int stop_kick_watchdog = 1;
#define MAX_MSM_DOGS	6	/* Maximum number of msm_watchdog devices */
#define msm_softdog_num  3
static unsigned int soft_margin = 60;	/* Default is 60 seconds */
static struct class *msm_softdog_class;
static dev_t msm_softdog_devt;
struct msm_softdog_data{
	struct timer_list softdog_timer;
	struct cdev softdog_cdev;
	int dev_id;
	bool softdog_en;
};
static struct msm_softdog_data msm_softdog[msm_softdog_num];

static void msm_softdogfire(unsigned long data)
{
	struct msm_softdog_data *softdogdd =
		(struct msm_softdog_data *)data;
	if(softdogdd == NULL)
		return;
	softdogdd->softdog_en = 0;
	pr_err("msm_watchdog%d is timeout, now system will reboot!\n", softdogdd->dev_id);
	msm_trigger_wdog_bite();
	preempt_disable();
}

static int msm_softdog_open(struct inode *inode, struct file *file)
{
	struct msm_softdog_data *softdogdd;

	/* Get the corresponding watchdog device */
	softdogdd = container_of(inode->i_cdev, struct msm_softdog_data, softdog_cdev);
	if(softdogdd == NULL)
		return 1;
	if(softdogdd->softdog_en == 1){
		printk(KERN_INFO"msm_softdog%d was reopened! \n", softdogdd->dev_id);
		mod_timer(&softdogdd->softdog_timer,jiffies + soft_margin*HZ);
		file->private_data = softdogdd;
		return 0;
	}
	init_timer(&softdogdd->softdog_timer);
	softdogdd->softdog_timer.data = (unsigned long)softdogdd;
	softdogdd->softdog_timer.function = msm_softdogfire;
	softdogdd->softdog_timer.expires = jiffies + soft_margin*HZ;
	add_timer(&softdogdd->softdog_timer);
	softdogdd->softdog_en = 1;
	file->private_data = softdogdd;
	printk(KERN_INFO"msm_softdog%d was opened! \n", softdogdd->dev_id);
	return 0;
}

static ssize_t msm_softdog_write(struct file *file, const char __user *data,
						size_t len, loff_t *ppos)
{
    struct msm_softdog_data *softdogdd = file->private_data;
    if(softdogdd == NULL)
		return 0;
	mod_timer(&softdogdd->softdog_timer, jiffies+(soft_margin*HZ));
	return 1;
}

static int  msm_softdog_ioctl( struct file *file, unsigned int command, unsigned long arg)
{
	struct msm_softdog_data *softdogdd = file->private_data;
	if(softdogdd == NULL)
		return 1;

	switch(command)
	{
	case SET_MSM_SOFTDOG_MARGIN :
		soft_margin = arg;
		if(softdogdd->softdog_en == 1){
			mod_timer(&softdogdd->softdog_timer, jiffies+(soft_margin*HZ));
			printk(KERN_INFO"Set msm_softdog%d margin to %d \n", softdogdd->dev_id, arg);
		}
		break;
	case GET_MSM_SOFTDOG_MARGIN :
		arg = (softdogdd->softdog_timer.expires - jiffies)/HZ;
		printk(KERN_INFO"Get msm_softdog%d, it is %d \n", softdogdd->dev_id, arg);
		break;
	case STOP_KICK_MSM_SOFTDOG :
		if(softdogdd->softdog_en == 1){
			del_timer_sync(&softdogdd->softdog_timer);
			softdogdd->softdog_en = 0;
			printk(KERN_INFO"msm_softdog%d was stoped! \n", softdogdd->dev_id);
		}
		break;
	case START_KICK_MSM_SOFTDOG :
		if(softdogdd->softdog_en == 1){
			printk(KERN_INFO"msm_softdog%d was restarted! \n", softdogdd->dev_id);
			mod_timer(&softdogdd->softdog_timer,jiffies + soft_margin*HZ);
			file->private_data = softdogdd;
			return 0;
		}
		init_timer(&softdogdd->softdog_timer);
		softdogdd->softdog_timer.data = (unsigned long)softdogdd;
		softdogdd->softdog_timer.function = msm_softdogfire;
		softdogdd->softdog_timer.expires = jiffies + soft_margin*HZ;
		add_timer(&softdogdd->softdog_timer);
		softdogdd->softdog_en = 1;
		file->private_data = softdogdd;
		printk(KERN_INFO"msm_softdog%d was started!\n", softdogdd->dev_id);
		break;
	default :
		break ;
	}

	return 0;
}

static const struct file_operations msm_softdog_fops = {
	.owner		= THIS_MODULE,
	.write		= msm_softdog_write,
	.open		= msm_softdog_open,
	.unlocked_ioctl		= msm_softdog_ioctl,
};

static void msm_watchdog_dev_unregister(struct msm_softdog_data *softdog_dd)
{
	cdev_del(&softdog_dd->softdog_cdev);
	device_destroy(msm_softdog_class, MKDEV(MAJOR(msm_softdog_devt), softdog_dd->dev_id));
	return;
}

static int msm_watchdog_dev_register(struct msm_softdog_data *softdog_dd)
{
	int err, devno;
	struct device *msm_softdog_device;

	/* Fill in the data structures */
	devno = MKDEV(MAJOR(msm_softdog_devt), softdog_dd->dev_id);
	cdev_init(&softdog_dd->softdog_cdev, &msm_softdog_fops);
	softdog_dd->softdog_cdev.owner = THIS_MODULE;
	/* Add the device */
	err = cdev_add(&softdog_dd->softdog_cdev, devno, 1);
	if (err) {
		pr_err("msm_watchdog%d unable to add device %d:1\n",softdog_dd->dev_id,MAJOR(msm_softdog_devt));
		return err;
	}

	msm_softdog_device = device_create(msm_softdog_class, NULL, softdog_dd->softdog_cdev.dev,
					NULL, "msm_watchdog%d", softdog_dd->dev_id);
	if (IS_ERR(msm_softdog_device)) {
		msm_watchdog_dev_unregister(softdog_dd);
		err = PTR_ERR(msm_softdog_device);
		return err;
	}
	return 0;
}

static void msm_watchdog_dev_exit(void)
{
	unregister_chrdev_region(msm_softdog_devt, MAX_MSM_DOGS);
	class_destroy(msm_softdog_class);
	return;
}

static int msm_watchdog_dev_init(void)
{
	int err,i;
	msm_softdog_class = class_create(THIS_MODULE, "msm_watchdog");
	if (IS_ERR(msm_softdog_class)) {
		pr_err("couldn't create class\n");
		return PTR_ERR(msm_softdog_class);
	}

	err = alloc_chrdev_region(&msm_softdog_devt, 1, MAX_MSM_DOGS, "msm_watchdog");
	if (err < 0){
		class_destroy(msm_softdog_class);
		pr_err("watchdog: unable to allocate char dev region\n");
		return err;
	}
	for(i = 0;i < msm_softdog_num;i++){
		msm_softdog[i].dev_id = i;
		msm_softdog[i].softdog_en = 0;
	}
	return 0;
}
#endif /*CONFIG_SIERRA*/

static void dump_cpu_alive_mask(struct msm_watchdog_data *wdog_dd)
{
	static char alive_mask_buf[MASK_SIZE];
	cpulist_scnprintf(alive_mask_buf, MASK_SIZE,
						&wdog_dd->alive_mask);
	printk(KERN_INFO "cpu alive mask from last pet %s\n", alive_mask_buf);
}

static int msm_watchdog_suspend(struct device *dev)
{
	struct msm_watchdog_data *wdog_dd =
			(struct msm_watchdog_data *)dev_get_drvdata(dev);
	if (!enable)
		return 0;
	__raw_writel(1, wdog_dd->base + WDT0_RST);
	if (wdog_dd->wakeup_irq_enable) {
		/* Make sure register write is complete before proceeding */
		mb();
		wdog_dd->last_pet = sched_clock();
		return 0;
	}
	__raw_writel(0, wdog_dd->base + WDT0_EN);
	mb();
	wdog_dd->enabled = false;
	wdog_dd->last_pet = sched_clock();
	return 0;
}

static int msm_watchdog_resume(struct device *dev)
{
	struct msm_watchdog_data *wdog_dd =
			(struct msm_watchdog_data *)dev_get_drvdata(dev);
	if (!enable)
		return 0;
	if (wdog_dd->wakeup_irq_enable) {
		__raw_writel(1, wdog_dd->base + WDT0_RST);
		/* Make sure register write is complete before proceeding */
		mb();
		wdog_dd->last_pet = sched_clock();
		return 0;
	}
	__raw_writel(1, wdog_dd->base + WDT0_EN);
	__raw_writel(1, wdog_dd->base + WDT0_RST);
	mb();
	wdog_dd->enabled = true;
	wdog_dd->last_pet = sched_clock();
	return 0;
}

static int panic_wdog_handler(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	struct msm_watchdog_data *wdog_dd = container_of(this,
				struct msm_watchdog_data, panic_blk);
	if (panic_timeout == 0) {
		__raw_writel(0, wdog_dd->base + WDT0_EN);
		mb();
	} else {
		__raw_writel(WDT_HZ * (panic_timeout + 10),
				wdog_dd->base + WDT0_BARK_TIME);
		__raw_writel(WDT_HZ * (panic_timeout + 10),
				wdog_dd->base + WDT0_BITE_TIME);
		__raw_writel(1, wdog_dd->base + WDT0_RST);
	}

#ifdef CONFIG_SIERRA
	stop_kick_watchdog = 1;
#endif /*CONFIG_SIERRA*/

	return NOTIFY_DONE;
}

static void wdog_disable(struct msm_watchdog_data *wdog_dd)
{
	__raw_writel(0, wdog_dd->base + WDT0_EN);
	mb();
	if (wdog_dd->irq_ppi) {
		disable_percpu_irq(wdog_dd->bark_irq);
		free_percpu_irq(wdog_dd->bark_irq, wdog_dd->wdog_cpu_dd);
	} else
		devm_free_irq(wdog_dd->dev, wdog_dd->bark_irq, wdog_dd);
	enable = 0;
	/*Ensure all cpus see update to enable*/
	smp_mb();
	atomic_notifier_chain_unregister(&panic_notifier_list,
						&wdog_dd->panic_blk);
	del_timer_sync(&wdog_dd->pet_timer);
	/* may be suspended after the first write above */
	__raw_writel(0, wdog_dd->base + WDT0_EN);
	mb();
	wdog_dd->enabled = false;
	pr_info("MSM Apps Watchdog deactivated.\n");
}

static ssize_t wdog_disable_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	struct msm_watchdog_data *wdog_dd = dev_get_drvdata(dev);

	mutex_lock(&wdog_dd->disable_lock);
	ret = snprintf(buf, PAGE_SIZE, "%d\n", enable == 0 ? 1 : 0);
	mutex_unlock(&wdog_dd->disable_lock);
	return ret;
}

static ssize_t wdog_disable_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	u8 disable;
	struct msm_watchdog_data *wdog_dd = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 10, &disable);
	if (ret) {
		dev_err(wdog_dd->dev, "invalid user input\n");
		return ret;
	}
	if (disable == 1) {
		mutex_lock(&wdog_dd->disable_lock);
		if (enable == 0) {
			pr_info("MSM Apps Watchdog already disabled\n");
			mutex_unlock(&wdog_dd->disable_lock);
			return count;
		}
		disable = 1;
		if (!is_scm_armv8()) {
			ret = scm_call(SCM_SVC_BOOT, SCM_SVC_SEC_WDOG_DIS,
				       &disable, sizeof(disable), NULL, 0);
		} else {
			struct scm_desc desc = {0};
			desc.args[0] = 1;
			desc.arginfo = SCM_ARGS(1);
			ret = scm_call2(SCM_SIP_FNID(SCM_SVC_BOOT,
					SCM_SVC_SEC_WDOG_DIS), &desc);
		}
		if (ret) {
			dev_err(wdog_dd->dev,
					"Failed to deactivate secure wdog\n");
			mutex_unlock(&wdog_dd->disable_lock);
			return -EIO;
		}
		wdog_disable(wdog_dd);
		mutex_unlock(&wdog_dd->disable_lock);
	} else {
		pr_err("invalid operation, only disable = 1 supported\n");
		return -EINVAL;
	}
	return count;
}

static DEVICE_ATTR(disable, S_IWUSR | S_IRUSR, wdog_disable_get,
							wdog_disable_set);

#ifdef CONFIG_SIERRA
static ssize_t wdog_barktime_get(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	long barktime;
	static long WDTHZ = 32765;
	int ret;

	barktime = __raw_readl(wdog_data->base + WDT0_BARK_TIME)/WDTHZ;
	ret = snprintf(buf, PAGE_SIZE, "%d\n",barktime);
	return ret;
}

static ssize_t wdog_barktime_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const char *p;
	long barktime;
	static long WDTHZ = 32765;

	p = memchr(buf, '\n', count);
	barktime = simple_strtol(buf, p, 10);

	__raw_writel((barktime * WDTHZ), wdog_data->base + WDT0_BARK_TIME);
	mb();
	__raw_writel(((barktime+3) * WDTHZ ), wdog_data->base + WDT0_BITE_TIME);
	mb();
	return count;
}

static DEVICE_ATTR(barktime, S_IWUSR | S_IRUSR, wdog_barktime_get, wdog_barktime_set);

static ssize_t wdog_stopautokick_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const char *p;
	int original_count = count;

	p = memchr(buf, '\n', count);

	if (p)
		count = p - buf;

	if (!strncasecmp(buf, "1", count)){
		stop_kick_watchdog = 1;
	}
	else if (!strncasecmp(buf, "0", count)){
		stop_kick_watchdog = 0;
	}

	__raw_writel(1, wdog_data->base + WDT0_RST);

	return original_count;
}

static DEVICE_ATTR(stopautokick, S_IWUSR, NULL, wdog_stopautokick_set);

static ssize_t wdog_kick(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const char *p;
	int original_count = count;

	p = memchr(buf, '\n', count);
	if (p)
		count = p - buf;
	if (!strncasecmp(buf, "1", count)){
		__raw_writel(1, wdog_data->base + WDT0_RST);
	}

	return original_count;
}

static DEVICE_ATTR(kick, S_IWUSR, NULL, wdog_kick);
#endif  /*CONFIG_SIERRA*/

static void pet_watchdog(struct msm_watchdog_data *wdog_dd)
{
	int slack, i, count, prev_count = 0;
	unsigned long long time_ns;
	unsigned long long slack_ns;
	unsigned long long bark_time_ns = wdog_dd->bark_time * 1000000ULL;

#ifdef CONFIG_SIERRA
	if(stop_kick_watchdog == 1){
		pr_info("Stop pet MSM watchdog.\n");
		return;
	}
#endif  /*CONFIG_SIERRA*/

	for (i = 0; i < 2; i++) {
		count = (__raw_readl(wdog_dd->base + WDT0_STS) >> 1) & 0xFFFFF;
		if (count != prev_count) {
			prev_count = count;
			i = 0;
		}
	}
	slack = ((wdog_dd->bark_time * WDT_HZ) / 1000) - count;
	if (slack < wdog_dd->min_slack_ticks)
		wdog_dd->min_slack_ticks = slack;
	__raw_writel(1, wdog_dd->base + WDT0_RST);
	time_ns = sched_clock();
	slack_ns = (wdog_dd->last_pet + bark_time_ns) - time_ns;
	if (slack_ns < wdog_dd->min_slack_ns)
		wdog_dd->min_slack_ns = slack_ns;
	wdog_dd->last_pet = time_ns;
}

static void keep_alive_response(void *info)
{
	int cpu = smp_processor_id();
	struct msm_watchdog_data *wdog_dd = (struct msm_watchdog_data *)info;
	cpumask_set_cpu(cpu, &wdog_dd->alive_mask);
	smp_mb();
}

/*
 * If this function does not return, it implies one of the
 * other cpu's is not responsive.
 */
static void ping_other_cpus(struct msm_watchdog_data *wdog_dd)
{
	int cpu;
	cpumask_clear(&wdog_dd->alive_mask);
	smp_mb();
	for_each_cpu(cpu, cpu_online_mask) {
		if (!cpu_idle_pc_state[cpu])
			smp_call_function_single(cpu, keep_alive_response,
						 wdog_dd, 1);
	}
}

static void pet_task_wakeup(unsigned long data)
{
	struct msm_watchdog_data *wdog_dd =
		(struct msm_watchdog_data *)data;
	complete(&wdog_dd->pet_complete);
}

static __ref int watchdog_kthread(void *arg)
{
	struct msm_watchdog_data *wdog_dd =
		(struct msm_watchdog_data *)arg;
	unsigned long delay_time = 0;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO-1};

	sched_setscheduler(current, SCHED_FIFO, &param);
	while (!kthread_should_stop()) {
		while (wait_for_completion_interruptible(
			&wdog_dd->pet_complete) != 0)
			;
		reinit_completion(&wdog_dd->pet_complete);
		if (enable) {
			delay_time = msecs_to_jiffies(wdog_dd->pet_time);
			if (wdog_dd->do_ipi_ping)
				ping_other_cpus(wdog_dd);
			pet_watchdog(wdog_dd);
		}
		/* Check again before scheduling *
		 * Could have been changed on other cpu */
		mod_timer(&wdog_dd->pet_timer, jiffies + delay_time);
	}
	return 0;
}

static int wdog_cpu_pm_notify(struct notifier_block *self,
			      unsigned long action, void *v)
{
	int cpu;

	cpu = raw_smp_processor_id();

	switch (action) {
	case CPU_PM_ENTER:
		cpu_idle_pc_state[cpu] = 1;
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		cpu_idle_pc_state[cpu] = 0;
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block wdog_cpu_pm_nb = {
	.notifier_call = wdog_cpu_pm_notify,
};

static int msm_watchdog_remove(struct platform_device *pdev)
{
	struct msm_watchdog_data *wdog_dd =
			(struct msm_watchdog_data *)platform_get_drvdata(pdev);
#ifdef CONFIG_SIERRA
	int i;
#endif /*CONFIG_SIERRA*/

	if (ipi_opt_en)
		cpu_pm_unregister_notifier(&wdog_cpu_pm_nb);

	mutex_lock(&wdog_dd->disable_lock);
	if (enable) {
		wdog_disable(wdog_dd);
	}
	mutex_unlock(&wdog_dd->disable_lock);
	device_remove_file(wdog_dd->dev, &dev_attr_disable);
	if (wdog_dd->irq_ppi)
		free_percpu(wdog_dd->wdog_cpu_dd);
	printk(KERN_INFO "MSM Watchdog Exit - Deactivated\n");
	del_timer_sync(&wdog_dd->pet_timer);
	kthread_stop(wdog_dd->watchdog_task);
	kfree(wdog_dd);
#ifdef CONFIG_SIERRA
	for(i = 0;i < msm_softdog_num;i++){
		if(msm_softdog[i].softdog_en == 1){
			del_timer_sync(&msm_softdog[i].softdog_timer);
		}
		msm_watchdog_dev_unregister(&msm_softdog[i]);
	}
	msm_watchdog_dev_exit();
#endif /*CONFIG_SIERRA*/
	return 0;
}

void msm_trigger_wdog_bite(void)
{
	if (!wdog_data)
		return;
	pr_info("Causing a watchdog bite!");
	__raw_writel(1, wdog_data->base + WDT0_BITE_TIME);
	mb();
	__raw_writel(1, wdog_data->base + WDT0_RST);
	mb();
	/* Delay to make sure bite occurs */
	mdelay(10000);
	pr_err("Wdog - STS: 0x%x, CTL: 0x%x, BARK TIME: 0x%x, BITE TIME: 0x%x",
		__raw_readl(wdog_data->base + WDT0_STS),
		__raw_readl(wdog_data->base + WDT0_EN),
		__raw_readl(wdog_data->base + WDT0_BARK_TIME),
		__raw_readl(wdog_data->base + WDT0_BITE_TIME));
}

static irqreturn_t wdog_bark_handler(int irq, void *dev_id)
{
	struct msm_watchdog_data *wdog_dd = (struct msm_watchdog_data *)dev_id;
	unsigned long nanosec_rem;
	unsigned long long t = sched_clock();

	nanosec_rem = do_div(t, 1000000000);
	printk(KERN_INFO "Watchdog bark! Now = %lu.%06lu\n", (unsigned long) t,
		nanosec_rem / 1000);

	nanosec_rem = do_div(wdog_dd->last_pet, 1000000000);
	printk(KERN_INFO "Watchdog last pet at %lu.%06lu\n", (unsigned long)
		wdog_dd->last_pet, nanosec_rem / 1000);
	if (wdog_dd->do_ipi_ping)
		dump_cpu_alive_mask(wdog_dd);
	msm_trigger_wdog_bite();
	panic("Failed to cause a watchdog bite! - Falling back to kernel panic!");
	return IRQ_HANDLED;
}

static irqreturn_t wdog_ppi_bark(int irq, void *dev_id)
{
	struct msm_watchdog_data *wdog_dd =
			*(struct msm_watchdog_data **)(dev_id);
	return wdog_bark_handler(irq, wdog_dd);
}

static void configure_bark_dump(struct msm_watchdog_data *wdog_dd)
{
	int ret;
	struct msm_client_dump cpu_dump_entry;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *cpu_data;
	int cpu;
	void *cpu_buf;
	struct {
		unsigned addr;
		int len;
	} cmd_buf;
	struct scm_desc desc = {0};

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) == 1) {
		wdog_dd->scm_regsave = (void *)__get_free_page(GFP_KERNEL);
		if (wdog_dd->scm_regsave) {
			/* scm_regsave may be a phys address > 4GB */
			desc.args[0] = virt_to_phys(wdog_dd->scm_regsave);
			cmd_buf.addr = virt_to_phys(wdog_dd->scm_regsave);
			desc.args[1] = cmd_buf.len  = PAGE_SIZE;
			desc.arginfo = SCM_ARGS(2, SCM_RW, SCM_VAL);

			if (!is_scm_armv8())
				ret = scm_call(SCM_SVC_UTIL,
					       SCM_SET_REGSAVE_CMD, &cmd_buf,
					       sizeof(cmd_buf), NULL, 0);
			else
				ret = scm_call2(SCM_SIP_FNID(SCM_SVC_UTIL,
						SCM_SET_REGSAVE_CMD), &desc);
			if (ret)
				pr_err("Setting register save address failed.\n"
				       "Registers won't be dumped on a dog "
				       "bite\n");
			cpu_dump_entry.id = MSM_CPU_CTXT;
			cpu_dump_entry.start_addr =
					virt_to_phys(wdog_dd->scm_regsave);
			cpu_dump_entry.end_addr = cpu_dump_entry.start_addr +
						  PAGE_SIZE;
			ret = msm_dump_tbl_register(&cpu_dump_entry);
			if (ret)
				pr_err("Setting cpu dump region failed\n"
				"Registers wont be dumped during cpu hang\n");
		} else {
			pr_err("Allocating register save space failed\n"
			       "Registers won't be dumped on a dog bite\n");
			/*
			 * No need to bail if allocation fails. Simply don't
			 * send the command, and the secure side will reset
			 * without saving registers.
			 */
		}
	} else {
		cpu_data = kzalloc(sizeof(struct msm_dump_data) *
				   num_present_cpus(), GFP_KERNEL);
		if (!cpu_data) {
			pr_err("cpu dump data structure allocation failed\n");
			goto out0;
		}
		cpu_buf = kzalloc(MAX_CPU_CTX_SIZE * num_present_cpus(),
				  GFP_KERNEL);
		if (!cpu_buf) {
			pr_err("cpu reg context space allocation failed\n");
			goto out1;
		}

		for_each_cpu(cpu, cpu_present_mask) {
			cpu_data[cpu].addr = virt_to_phys(cpu_buf +
							cpu * MAX_CPU_CTX_SIZE);
			cpu_data[cpu].len = MAX_CPU_CTX_SIZE;
			dump_entry.id = MSM_DUMP_DATA_CPU_CTX + cpu;
			dump_entry.addr = virt_to_phys(&cpu_data[cpu]);
			ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS,
						     &dump_entry);
			/*
			 * Don't free the buffers in case of error since
			 * registration may have succeeded for some cpus.
			 */
			if (ret)
				pr_err("cpu %d reg dump setup failed\n", cpu);
		}
	}

	return;
out1:
	kfree(cpu_data);
out0:
	return;
}


static void init_watchdog_data(struct msm_watchdog_data *wdog_dd)
{
	unsigned long delay_time;
	uint32_t val;
	int error;
	u64 timeout;
	int ret;
#ifdef CONFIG_SIERRA
	int i;
#endif /*CONFIG_SIERRA*/
	/*
	 * Disable the watchdog for cluster 1 so that cluster 0 watchdog will
	 * be mapped to the entire sub-system.
	 */
	if (wdog_dd->wdog_absent_base)
		__raw_writel(2, wdog_dd->wdog_absent_base + WDOG_ABSENT);

	if (wdog_dd->irq_ppi) {
		wdog_dd->wdog_cpu_dd = alloc_percpu(struct msm_watchdog_data *);
		if (!wdog_dd->wdog_cpu_dd) {
			dev_err(wdog_dd->dev, "fail to allocate cpu data\n");
			return;
		}
		*raw_cpu_ptr(wdog_dd->wdog_cpu_dd) = wdog_dd;
		ret = request_percpu_irq(wdog_dd->bark_irq, wdog_ppi_bark,
					"apps_wdog_bark",
					wdog_dd->wdog_cpu_dd);
		if (ret) {
			dev_err(wdog_dd->dev, "failed to request bark irq\n");
			free_percpu(wdog_dd->wdog_cpu_dd);
			return;
		}
	} else {
		ret = devm_request_irq(wdog_dd->dev, wdog_dd->bark_irq,
				wdog_bark_handler, IRQF_TRIGGER_RISING,
						"apps_wdog_bark", wdog_dd);
		if (ret) {
			dev_err(wdog_dd->dev, "failed to request bark irq\n");
			return;
		}
	}
	delay_time = msecs_to_jiffies(wdog_dd->pet_time);
	wdog_dd->min_slack_ticks = UINT_MAX;
	wdog_dd->min_slack_ns = ULLONG_MAX;
	configure_bark_dump(wdog_dd);
	timeout = (wdog_dd->bark_time * WDT_HZ)/1000;
	__raw_writel(timeout, wdog_dd->base + WDT0_BARK_TIME);
	__raw_writel(timeout + 3*WDT_HZ, wdog_dd->base + WDT0_BITE_TIME);

	wdog_dd->panic_blk.notifier_call = panic_wdog_handler;
	atomic_notifier_chain_register(&panic_notifier_list,
				       &wdog_dd->panic_blk);
	mutex_init(&wdog_dd->disable_lock);
	init_completion(&wdog_dd->pet_complete);
	wake_up_process(wdog_dd->watchdog_task);
	init_timer(&wdog_dd->pet_timer);
	wdog_dd->pet_timer.data = (unsigned long)wdog_dd;
	wdog_dd->pet_timer.function = pet_task_wakeup;
	wdog_dd->pet_timer.expires = jiffies + delay_time;
	add_timer(&wdog_dd->pet_timer);

	val = BIT(EN);
	if (wdog_dd->wakeup_irq_enable)
		val |= BIT(UNMASKED_INT_EN);
	__raw_writel(val, wdog_dd->base + WDT0_EN);
	__raw_writel(1, wdog_dd->base + WDT0_RST);
	wdog_dd->last_pet = sched_clock();
	wdog_dd->enabled = true;
	error = device_create_file(wdog_dd->dev, &dev_attr_disable);
	if (error)
		dev_err(wdog_dd->dev, "cannot create sysfs attribute\n");
	if (wdog_dd->irq_ppi)
		enable_percpu_irq(wdog_dd->bark_irq, 0);
	if (ipi_opt_en)
		cpu_pm_register_notifier(&wdog_cpu_pm_nb);
	dev_info(wdog_dd->dev, "MSM Watchdog Initialized\n");
#ifdef CONFIG_SIERRA
	error = device_create_file(wdog_dd->dev, &dev_attr_stopautokick);
	if (error)
		printk( "cannot create sysfs attribute stopautokick\n");

	error = device_create_file(wdog_dd->dev, &dev_attr_barktime);
	if (error)
		printk( "cannot create sysfs attribute barktime\n");

	error = device_create_file(wdog_dd->dev, &dev_attr_kick);
	if (error)
		printk( "cannot create sysfs attribute kick\n");

	error = msm_watchdog_dev_init();
	if(error){
		pr_err("msm_watchdog init failed\n");
		return;
	}
	for(i = 0;i < msm_softdog_num;i++){
		error = msm_watchdog_dev_register(&msm_softdog[i]);
		if(error){
			pr_err("msm_watchdog%d unable to register\n",i);
		}
	}
#endif /*CONFIG_SIERRA*/
	return;
}

static struct of_device_id msm_wdog_match_table[] = {
	{ .compatible = "qcom,msm-watchdog" },
	{}
};

static void dump_pdata(struct msm_watchdog_data *pdata)
{
	dev_dbg(pdata->dev, "wdog bark_time %d", pdata->bark_time);
	dev_dbg(pdata->dev, "wdog pet_time %d", pdata->pet_time);
	dev_dbg(pdata->dev, "wdog perform ipi ping %d", pdata->do_ipi_ping);
	dev_dbg(pdata->dev, "wdog base address is 0x%lx\n", (unsigned long)
								pdata->base);
}

static int msm_wdog_dt_to_pdata(struct platform_device *pdev,
					struct msm_watchdog_data *pdata)
{
	struct device_node *node = pdev->dev.of_node;
	struct resource *res;
	int ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wdt-base");
	if (!res)
		return -ENODEV;
	pdata->size = resource_size(res);
	pdata->phys_base = res->start;
	if (unlikely(!(devm_request_mem_region(&pdev->dev, pdata->phys_base,
					       pdata->size, "msm-watchdog")))) {

		dev_err(&pdev->dev, "%s cannot reserve watchdog region\n",
								__func__);
		return -ENXIO;
	}
	pdata->base  = devm_ioremap(&pdev->dev, pdata->phys_base,
							pdata->size);
	if (!pdata->base) {
		dev_err(&pdev->dev, "%s cannot map wdog register space\n",
				__func__);
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "wdt-absent-base");
	if (res) {
		pdata->wdog_absent_base  = devm_ioremap(&pdev->dev, res->start,
							 resource_size(res));
		if (!pdata->wdog_absent_base) {
			dev_err(&pdev->dev,
				"cannot map wdog absent register space\n");
			return -ENXIO;
		}
	} else {
		dev_info(&pdev->dev, "wdog absent resource not present\n");
	}

	pdata->bark_irq = platform_get_irq(pdev, 0);
	pdata->bite_irq = platform_get_irq(pdev, 1);
	ret = of_property_read_u32(node, "qcom,bark-time", &pdata->bark_time);
	if (ret) {
		dev_err(&pdev->dev, "reading bark time failed\n");
		return -ENXIO;
	}
	ret = of_property_read_u32(node, "qcom,pet-time", &pdata->pet_time);
	if (ret) {
		dev_err(&pdev->dev, "reading pet time failed\n");
		return -ENXIO;
	}
	pdata->do_ipi_ping = of_property_read_bool(node, "qcom,ipi-ping");
	if (!pdata->bark_time) {
		dev_err(&pdev->dev, "%s watchdog bark time not setup\n",
								__func__);
		return -ENXIO;
	}
	if (!pdata->pet_time) {
		dev_err(&pdev->dev, "%s watchdog pet time not setup\n",
								__func__);
		return -ENXIO;
	}
	pdata->wakeup_irq_enable = of_property_read_bool(node,
							 "qcom,wakeup-enable");

	pdata->irq_ppi = irq_is_percpu(pdata->bark_irq);
	dump_pdata(pdata);
	return 0;
}

static int msm_watchdog_probe(struct platform_device *pdev)
{
	int ret;
	struct msm_watchdog_data *wdog_dd;

	if (!pdev->dev.of_node || !enable)
		return -ENODEV;
	wdog_dd = kzalloc(sizeof(struct msm_watchdog_data), GFP_KERNEL);
	if (!wdog_dd)
		return -EIO;
	ret = msm_wdog_dt_to_pdata(pdev, wdog_dd);
	if (ret)
		goto err;

	wdog_data = wdog_dd;
	wdog_dd->dev = &pdev->dev;
	platform_set_drvdata(pdev, wdog_dd);
	cpumask_clear(&wdog_dd->alive_mask);
	wdog_dd->watchdog_task = kthread_create(watchdog_kthread, wdog_dd,
			"msm_watchdog");
	if (IS_ERR(wdog_dd->watchdog_task)) {
		ret = PTR_ERR(wdog_dd->watchdog_task);
		goto err;
	}
	init_watchdog_data(wdog_dd);
	return 0;
err:
	kzfree(wdog_dd);
	return ret;
}

static const struct dev_pm_ops msm_watchdog_dev_pm_ops = {
	.suspend_noirq = msm_watchdog_suspend,
	.resume_noirq = msm_watchdog_resume,
};

static struct platform_driver msm_watchdog_driver = {
	.probe = msm_watchdog_probe,
	.remove = msm_watchdog_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &msm_watchdog_dev_pm_ops,
		.of_match_table = msm_wdog_match_table,
	},
};

static int init_watchdog(void)
{
	return platform_driver_register(&msm_watchdog_driver);
}

pure_initcall(init_watchdog);
MODULE_DESCRIPTION("MSM Watchdog Driver");
MODULE_LICENSE("GPL v2");
