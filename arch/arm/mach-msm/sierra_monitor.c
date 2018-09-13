/*  SWISTART
 *	System monitor and periodic waker
 *
 *	Copyright (c) 2015 Sierra Wireless, Inc.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *  Note: the purposes of the module are:
 *        1. Check critical system resources after startup and force crash
 *           if a resource is missing/malfunctional
 *        2. Wake up system periodically in deep sleep so that user space
 *           timer/event can be serviced. Wake up time is configurable to
 *           meet standby power requirements
 * Usage: sleep_dur/wake_dur/startup_timer have default values below but they
 *        are set by device tree (mdm9630-sierra.dtsi)
 *        sleep_dur - periodic wakeup, may affect sleep current. Can be set to 
 *                    0 and adjusted by user space app
 *        wake_dur  - set to 0 and don't change
 *        startup_timer - set to 60s and don't change
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <soc/qcom/event_timer.h>
#include <soc/qcom/restart.h>

#include <mach/sierra_smem.h>

#define MAX_VALUE_BUF_LEN          32

static int sleep_dur = 5;
module_param( sleep_dur, int, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC( sleep_dur, "Max. Sleep duration (seconds)" );

static int wake_dur = 0;
module_param( wake_dur, int, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC( wake_dur, "Min. Wake duration (ms)" );

static int startup_timer = 60;
module_param( startup_timer, int, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC( startup_timer, "Startup timer (seconds)" );

static struct notifier_block snb;
static struct wakeup_source * ws;
static struct rtc_device * rtc = NULL;
static struct device_node *of_node = NULL;

static void sierra_startup_monitor(struct work_struct *work);
static DECLARE_DELAYED_WORK(sierra_delayed_work, sierra_startup_monitor);
static struct workqueue_struct *sierra_workqueue;

static unsigned long susp_time;
static unsigned long total_sleep_time = 0;

/* function to get current RTC time in seconds */
static unsigned long now( void )
{
	struct rtc_time rtcnow;
	long _now;

	rtc_read_time( rtc, &rtcnow );
	rtc_tm_to_time( &rtcnow, &_now );

	return _now;
}

/* PM suspend Sierra function, will be called as part of suspend procedure 
 * Sierra may set RTC alarm and force resume after certain sleep duration
 */
static void sierra_pm_suspend( void )
{
	struct rtc_wkalrm alarm;
	int local_dur;

	/* wake up after sleep_dur or startup_timer 
	 * There is a possiblity that queue_delayed_work will not be serviced
	 * due to suspend so make sure it will wake up after startup_timer
	 */
	local_dur = sleep_dur;
	if (!local_dur)
		local_dur = startup_timer;
	
	if( !local_dur )
		return;

	susp_time = now();

	rtc_time_to_tm( susp_time + local_dur, &alarm.time );
	alarm.enabled = true;

	rtc_set_alarm( rtc, &alarm );

	pr_info("%s wakeup scheduled %ld\n", __FUNCTION__, susp_time+sleep_dur );
}

/* PM resume Sierra function, will be called as part of resume procedure 
 * Will check total sleep time and startup timeout
 * May force awake for a certain duration
 */
static void sierra_pm_resume( void )
{
	unsigned long sleep_time;

	pr_info("%s\n", __FUNCTION__ );

	/* due to suspend, delayed work might not be serviced. The delay is only
	 * calculaed when system is not sleep, so check sleep time here
	 * Based on algorithm below, startup_timer may not be accurate but the
	 * max timeout time should be less than 2 * startup_timer
	 */
	if (startup_timer) {
		sleep_time =  now() - susp_time; 
		pr_info("%s, sleep time = %ld\n", __FUNCTION__, sleep_time);

		if (sleep_time <= (2 * startup_timer)) {
			/* sleep time is valid, update total sleep time */
		    	total_sleep_time += sleep_time;

			if (total_sleep_time >= startup_timer) {
				pr_err("%s, total sleep time = %ld, startup_timer expired\n", 
					__FUNCTION__, total_sleep_time);

				/* restart delayed work right away */
				cancel_delayed_work(&sierra_delayed_work);
				if (!queue_delayed_work(sierra_workqueue, 
					&sierra_delayed_work, 0))
					pr_err("%s Failed to queue work\n",  __FUNCTION__);
			}
		}
	}

	if( wake_dur )
		__pm_wakeup_event( ws, wake_dur );
}

/* PM call back to Sierra functions */
static int waker_ntf( struct notifier_block *ntf, unsigned long msg, void * unused )
{
	switch(msg) {
	case PM_SUSPEND_PREPARE:
		sierra_pm_suspend();
		break;

	case PM_POST_SUSPEND:
		sierra_pm_resume();
		break;
	}

	return 0;
}

/* check if RTC is available */
static int has_wakealarm(struct device *dev, const void *data)
{
	struct rtc_device *candidate = to_rtc_device(dev);

	if (!candidate->ops->set_alarm)
		return 0;
	if (!device_may_wakeup(candidate->dev.parent))
		return 0;

	return 1;
}

/* Sierra critical resource check routine as part of startup monitor 
 * pathp  - resource path
 * valuep - resouce expected value (optional)
 * force_dload - if force download mode after crash 
 */
static void sierra_resource_check(const char *pathp, const char *valuep, int force_dload)
{
	int ret = 0;
	struct file *filp = (struct file *)-ENOENT;
	mm_segment_t oldfs;
	char buf[MAX_VALUE_BUF_LEN];
	int length;
	int check_failed = 0; 

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	do {
		filp = filp_open(pathp, O_RDONLY, S_IRUSR);

		if (IS_ERR(filp) || !filp->f_op) {
		    pr_err("%s: cannot open file %s\n", __FUNCTION__, pathp);
			ret = -ENOENT;
			check_failed = 1;
			break;
		}

		if (!valuep) {
			/* dont need check content of the file */
			break;
		}

		if (!filp->f_op->read) {
			ret = -EPERM;
			pr_err("%s: no read op\n", __FUNCTION__);
			break;
		}

		length = strlen(valuep);

		if (length == 0 || length >= sizeof(buf)) {
			ret = -EPERM;
			pr_err("%s: invalid len %s, %d\n", __FUNCTION__, pathp, length);
			break;
		}

		memset(buf, 0, sizeof(buf));
		ret = filp->f_op->read(filp, buf, length, &filp->f_pos);
		if (ret < 0) {
			pr_err("%s: read error %s, %d\n", __FUNCTION__, pathp, ret);
			check_failed = 1;
			break;
		}

		/* now compare the content */
		if (memcmp(valuep, buf, length)) {
			pr_err("%s: read error %s, %s\n", __FUNCTION__, valuep, buf);
			check_failed = 1;
			break;
		}

	} while (0);

	if (!IS_ERR(filp))
		filp_close(filp, NULL);

	set_fs(oldfs);

	if (check_failed) {
		pr_err("%s: resource check failed %s, panic\n", __FUNCTION__, pathp);
		if (force_dload) {
			pr_info("%s: force_dload: %d\n", __FUNCTION__, force_dload);
			/* set the flag to force device to dload mode after crash */
			msm_set_download_mode_swi(force_dload);
			/* also try to do image reocovery so that it can get out of dload */
			sierra_smem_im_recovery_mode_set();
		}
		panic("sierra resource:%s - check failed", pathp);
	}
}

/* Sierra startup monitor at startup timeout */
static void sierra_startup_monitor(struct work_struct *work)
{
	struct device_node *node = NULL;
	const char *pathp, *valuep;
	int num_levels = 0, force_dload;
	int ret;

	pr_info("%s\n", __FUNCTION__);

	if (!startup_timer) {
		/* backdoor function to set timer = 0 from user space to cancel the check */
		pr_info("%s: timer = 0 exit\n", __FUNCTION__);
		return;
	}

	/* one time monitor only */
	startup_timer = 0; 

	if (!of_node) {
		pr_err("%s: device tree information missing\n", __FUNCTION__);
		return;
	}

	for_each_child_of_node(of_node, node)
		num_levels++;

	if (!num_levels)  {
		pr_err("%s: no resource to monitor\n", __FUNCTION__);
		return;
	}


	num_levels = 0;
	for_each_child_of_node(of_node, node) {
		num_levels++;
		ret = of_property_read_string(node, "sierra,path", &pathp);
		if (ret) {
			pr_err("%s Failed to get path for %d\n",  
				__FUNCTION__, num_levels );
			pathp = NULL;
		}
		else {
			pr_debug("%s, path %s\n", __FUNCTION__, pathp );
		}

		ret = of_property_read_string(node, "sierra,value", &valuep);
		if (ret) {
			pr_debug("%s Failed to get value for %d\n",  
		         __FUNCTION__, num_levels );
			/* it is normal to not have value settings */
			valuep = NULL;
		}
		else {
			pr_debug("%s, value %s\n", __FUNCTION__, valuep );
		}

		if (of_property_read_u32(node, "sierra,force-dload", &force_dload))
			force_dload = 0;  

		if (pathp) {
			sierra_resource_check(pathp, valuep, force_dload);
		}
	}   
}

/* Following are standard module functions */
static int sierra_monitor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device * devrtc;
	int local_dur;

	pr_debug("%s", __FUNCTION__ );

	if (!node) {
		pr_err("device tree information missing\n");
		return -ENODEV;
	}

	of_node = node;

	if (!of_property_read_u32(node, "sierra,startup-time", &local_dur))
		startup_timer = local_dur;  
    
	if (!of_property_read_u32(node, "sierra,sleep-time", &local_dur))
		sleep_dur = local_dur;  

	if (!of_property_read_u32(node, "sierra,wake-time", &local_dur))
		wake_dur = local_dur;  

	pr_info("%s monitor times: %d, %d, %d\n", __FUNCTION__, 
	        startup_timer, sleep_dur, wake_dur);


	devrtc = class_find_device( rtc_class, NULL, NULL, has_wakealarm );
	if( devrtc )
	{

		rtc = rtc_class_open( (char *)dev_name(devrtc) );
		if( !rtc )
		{
			printk( KERN_ERR "Failed to open RTC %s\n", dev_name(devrtc) );
			return -EIO;
		}
	}
	else
	{
		pr_info("Not RTC with wakeup capabilities found\n" );
		return -ENOENT;
	}

	ws = wakeup_source_register( "waker" );
	if( !ws )
	{
		pr_err("Failed to create wakeup source\n" );
		return -ENOMEM;
	}
	snb.notifier_call = waker_ntf;
	snb.priority = 0;
	register_pm_notifier( &snb );

	sierra_workqueue = create_singlethread_workqueue("sierra_monitor_wq");
	if (!sierra_workqueue)
	{
		pr_err("%s Failed to create workqueue\n",  __FUNCTION__);
		return -EFAULT;
	}

	if (startup_timer)
	{
		if (!queue_delayed_work(sierra_workqueue,
				        &sierra_delayed_work, 
				        msecs_to_jiffies(1000 * startup_timer))) {
		    pr_err("%s Failed to queue work\n",  __FUNCTION__);
		    return -EFAULT;
		}
	}

	return 0;
}

static int sierra_monitor_remove(struct platform_device *pdev)
{
	pr_debug("%s", __FUNCTION__ );
	unregister_pm_notifier( &snb );

	rtc_class_close( rtc );

	wakeup_source_unregister( ws );

	destroy_workqueue(sierra_workqueue);

	return 0;
}

static struct of_device_id sierra_monitor_dt_info[] = {
  {
    .compatible = "sierra,sierra-monitor",
  },
  {}
};

static struct platform_driver sierra_monitor_driver = {
    .probe        = sierra_monitor_probe,
    .remove       = sierra_monitor_remove,
    .driver        = {
        .name        = "swimonitor",
        .owner = THIS_MODULE,
        .of_match_table = sierra_monitor_dt_info,
    },
};


static int __init monitor_init(void)
{
    int ret;
    ret = platform_driver_register(&sierra_monitor_driver);
    return ret;
}

static void __exit monitor_exit(void)
{
    platform_driver_unregister( &sierra_monitor_driver );
}

late_initcall( monitor_init );
module_exit( monitor_exit );

MODULE_AUTHOR("SWI RMD M2M FW");
MODULE_DESCRIPTION("Sierra System Monitor");
MODULE_VERSION("1.0");

MODULE_LICENSE("GPL");

/* SWISTOP */

