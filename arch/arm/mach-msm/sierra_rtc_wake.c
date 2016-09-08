/*  SWISTART
 *
 * $Id$
 *
 * Filename:  sierra_rtc_wake.c
 *
 * Purpose:
 *
 * Copyright: (c) 2016 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <soc/qcom/event_timer.h>
#include <soc/qcom/restart.h>
#include <linux/pm.h>
#include <mach/sierra_smem.h>


#define DEVICE_NAME "sierra_rtc_waker"


static int rtc_suspend_flag=0;
static int sleep_dur = 0;
module_param( sleep_dur, int, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC( sleep_dur, "Max. Sleep duration (seconds)" );

static int wake_dur = 15;
module_param( wake_dur, int, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC( wake_dur, "Min. Wake duration (ms)" );

static struct notifier_block snb;
static struct wakeup_source * ws;
static struct rtc_device * rtc = NULL;
static struct device_node *of_node = NULL;

struct sierra_rtc_waker_t {
	struct cdev myrtc;
	struct class * sierra_rtc_waker_class;
}* sierra_rtc_waker_p=NULL;

static int major=0;

static unsigned long susp_time;


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
static void sierra_pm_rtc_suspend( void )
{
	struct rtc_wkalrm alarm;
	int local_dur;

	/* wake up after sleep_dur or startup_timer
	 * There is a possiblity that queue_delayed_work will not be serviced
	 * due to suspend so make sure it will wake up after startup_timer
	 */
	local_dur = sleep_dur;

	if( !local_dur )
	{
		rtc_suspend_flag=0;
		return;
	}

	susp_time = now();

	rtc_time_to_tm( susp_time + local_dur, &alarm.time );
	alarm.enabled = true;

	rtc_set_alarm( rtc, &alarm );

	rtc_suspend_flag=1;

	pr_info("%s wakeup scheduled %ld\n", __FUNCTION__, susp_time+sleep_dur );
}

/* PM resume Sierra function, will be called as part of resume procedure
 */
static void sierra_pm_rtc_resume( void )
{
	pr_info("%s rtc_suspend_flag:%d wake_dur:%d\n", __FUNCTION__, rtc_suspend_flag, wake_dur);
	if(rtc_suspend_flag)
	{
		if( wake_dur )
			__pm_wakeup_event( ws, 1000*wake_dur );
	}
}

/* PM call back to Sierra functions */
static int waker_ntf( struct notifier_block *ntf, unsigned long msg, void * unused )
{
	switch(msg) {
	case PM_SUSPEND_PREPARE:
		sierra_pm_rtc_suspend();
		break;

	case PM_POST_SUSPEND:
		sierra_pm_rtc_resume();
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

static int rtc_wake_open(struct inode *inode, struct file *file)
{
	file->private_data=sierra_rtc_waker_p;
	return 0;
}

static int mypow(int data, int n)
{
	int multi=1;
	int i=0;
	for(i;i<n;i++)
		multi=multi*data;
	return multi;
}
static int rtc_wake_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int rtc_wake_write(struct file *file, const char __user *data,
		       size_t len, loff_t *ppos)
{
	unsigned long ret;
	int i=0;
	int sleep_time=0;
	char * str=kmalloc(len,GFP_KERNEL);

	ret = copy_from_user(str, data, len);
	if (ret)
	{
		kfree(str);
		return -EFAULT;
	}
	for(i=0;i<len-1;i++)
		sleep_time+=(str[i]-'0')*mypow(10,len-2-i);

	pr_info("%s:sleep_time=%d\n", __FUNCTION__,sleep_time);

	sleep_dur=sleep_time;
	kfree(str);

	return len;
}

static const struct file_operations rtc_wake_fops = {
	.write = rtc_wake_write,
	.open = rtc_wake_open,
	.release=rtc_wake_release,
};

/* Following are standard module functions */
static int sierra_rtc_wake_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __FUNCTION__ );

	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device * devrtc;
	int local_dur;
	int result = 0;
	struct device *device_handle;

	dev_t c_dev = MKDEV(major, 0);

	if (major)
		result = register_chrdev_region(c_dev, 1, DEVICE_NAME);
	else {
		result = alloc_chrdev_region(&c_dev, 0, 1, DEVICE_NAME);
		major = MAJOR(c_dev);
	}

	if (result < 0) {
		pr_err("%s: Registering sierra_rtc_waker device failed\n",
			__FUNCTION__);
		return -EFAULT;
	}

	sierra_rtc_waker_p=kmalloc(sizeof(struct sierra_rtc_waker_t),GFP_KERNEL);
	if(!sierra_rtc_waker_p)
	{
		pr_err("%s: kmalloc sierra_rtc_waker_p failed\n",
			__FUNCTION__);
		return -ENOMEM;
	}

	sierra_rtc_waker_p->sierra_rtc_waker_class = class_create(THIS_MODULE, "sierra_rtc_wake");

	if (IS_ERR(sierra_rtc_waker_p->sierra_rtc_waker_class)) {
		result = PTR_ERR(sierra_rtc_waker_p->sierra_rtc_waker_class);
		pr_err("%s: Error creating sierra_rtc_wake class: %d\n",
			__FUNCTION__, result);
		return -EFAULT;
	}

	cdev_init(&sierra_rtc_waker_p->myrtc, &rtc_wake_fops);
	result = cdev_add(&sierra_rtc_waker_p->myrtc, c_dev, 1);

	if (result < 0) {
		pr_err("%s: Registering file operations failed\n",
			__FUNCTION__);
		return -ENODEV;
	}

	device_handle = device_create(sierra_rtc_waker_p->sierra_rtc_waker_class,
			NULL, sierra_rtc_waker_p->myrtc.dev, NULL, "sierra_rtc_wake");
	if (IS_ERR(device_handle)) {
		result = PTR_ERR(device_handle);
		pr_err("%s: device_create failed: %d\n", __FUNCTION__, result);
		return -EFAULT;
	}

	if (!node) {
		pr_err("device tree information missing\n");
		return -ENODEV;
	}

	of_node = node;

	if (!of_property_read_u32(node, "sierra,sleep-time", &local_dur))
		sleep_dur = local_dur;

	if (!of_property_read_u32(node, "sierra,wake-time", &local_dur))
		wake_dur = local_dur;

	pr_info("%s rtc_wake times: %d, %d\n", __FUNCTION__,
	         sleep_dur, wake_dur);


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

	ws = wakeup_source_register( "rtc_waker" );
	if( !ws )
	{
		pr_err("Failed to create wakeup source\n" );
		return -ENOMEM;
	}
	snb.notifier_call = waker_ntf;
	snb.priority = 0;
	register_pm_notifier( &snb );

	return 0;
}

static int sierra_rtc_wake_remove(struct platform_device *pdev)
{
	pr_debug("%s", __FUNCTION__ );
	unregister_pm_notifier( &snb );

	rtc_class_close( rtc );

	wakeup_source_unregister( ws );

	return 0;
}

static struct of_device_id sierra_rtc_wake_dt_info[] = {
	{
		.compatible = "sierra,sierra-rtc_wake",
	},
	{}
};

MODULE_DEVICE_TABLE(of, sierra_rtc_wake_dt_info);

static struct platform_driver sierra_rtc_wake_driver = {
	.probe        = sierra_rtc_wake_probe,
	.remove       = sierra_rtc_wake_remove,
	.driver        = {
		.name        = "sierra_rtc_wake",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sierra_rtc_wake_dt_info),
		},
};


static int __init rtc_wake_init(void)
{
	return(platform_driver_register(&sierra_rtc_wake_driver));
}

static void __exit rtc_wake_exit(void)
{
	platform_driver_unregister( &sierra_rtc_wake_driver );
}

late_initcall( rtc_wake_init );
module_exit( rtc_wake_exit );


MODULE_DESCRIPTION("Sierra rtc wake");

MODULE_LICENSE("GPL");

/* SWISTOP */

