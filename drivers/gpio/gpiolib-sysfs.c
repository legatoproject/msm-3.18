#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>

#include "gpiolib.h"
/*SWISTART*/
#ifdef CONFIG_SIERRA
#include <linux/sierra_gpio.h>
#include <linux/sierra_bsudefs.h>
#include <../pinctrl/qcom/pinctrl-msm.h>
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

static DEFINE_IDR(dirent_idr);


/* lock protects against unexport_gpio() being called while
 * sysfs files are active.
 */
static DEFINE_MUTEX(sysfs_lock);

/*SWISTART*/
#ifdef CONFIG_SIERRA

/* for RI PIN owner flag*/
#define RI_OWNER_MODEM      0
#define RI_OWNER_APP        1

/* Product specific assignments in gpiolib_sysfs_init() */
static struct ext_gpio_map *ext_gpio = NULL;
static struct gpio_chip gpio_ext_chip = {
		.label  = "msmextgpio",
		.base   = 1,
};

static struct ext_gpio_map ext_gpio_ar[]={
		{"1", 86, FUNCTION_UNALLOCATED},
		{"2", 96, FUNCTION_UNALLOCATED},
		{"3", 98, FUNCTION_UNALLOCATED},
		{"4", 97, FUNCTION_UNALLOCATED},
		{"5",  1, FUNCTION_UNALLOCATED},
		{"6", 93, FUNCTION_UNALLOCATED},
		{"7", 82, FUNCTION_UNALLOCATED},
		{"8", 34, FUNCTION_UNALLOCATED},
		{"9", 30, FUNCTION_UNALLOCATED},
		{"10",-1, FUNCTION_EMBEDDED_HOST},
		{"11",90, FUNCTION_UNALLOCATED},
		{"12", 68,FUNCTION_UNALLOCATED},
		{"13", 53,FUNCTION_UNALLOCATED},
		{"14", 52,FUNCTION_UNALLOCATED},
		{"15", 50,FUNCTION_UNALLOCATED},
		{"16", 42,FUNCTION_UNALLOCATED},
		{"17", 88,FUNCTION_UNALLOCATED},
		{"18" ,60,FUNCTION_UNALLOCATED},
		{"19", 99,FUNCTION_UNALLOCATED},
		{"20", 94,FUNCTION_UNALLOCATED},
		{"21", -1,FUNCTION_EMBEDDED_HOST},
		{"22", -1,FUNCTION_EMBEDDED_HOST},
		{"23", 69,FUNCTION_UNALLOCATED},
		{"24", -1,FUNCTION_EMBEDDED_HOST},
		{"25", 81,FUNCTION_UNALLOCATED},
		{"26", 91,FUNCTION_UNALLOCATED},
	/* GPIO27/28 as I2C_SDA/I2C_CLK */
		{"27", 48,FUNCTION_EMBEDDED_HOST},
		{"28", 49,FUNCTION_EMBEDDED_HOST},
		{"29", 95,FUNCTION_UNALLOCATED},
	/* GPIO30-33 as UART1 function */
		{"30", 20,FUNCTION_EMBEDDED_HOST},
		{"31", 21,FUNCTION_EMBEDDED_HOST},
		{"32", 22,FUNCTION_EMBEDDED_HOST},
		{"33", 23,FUNCTION_EMBEDDED_HOST},
		{"34", 7, FUNCTION_UNALLOCATED},
		{"35", 6, FUNCTION_UNALLOCATED},
		{"36", 5, FUNCTION_EMBEDDED_HOST},
		{"37", 4, FUNCTION_EMBEDDED_HOST},
		{"38", -1,FUNCTION_EMBEDDED_HOST},
		{"39", -1,FUNCTION_EMBEDDED_HOST},
		{"40", -1,FUNCTION_EMBEDDED_HOST},
		{"41", -1,FUNCTION_EMBEDDED_HOST},
		{"42", -1,FUNCTION_EMBEDDED_HOST},
		{"43", -1,FUNCTION_EMBEDDED_HOST},
		{"44", -1,FUNCTION_EMBEDDED_HOST},
		{"45", -1,FUNCTION_EMBEDDED_HOST},
		{"46", -1,FUNCTION_EMBEDDED_HOST},
		{"47", 89,FUNCTION_UNALLOCATED},
		{"48", 87,FUNCTION_UNALLOCATED},
		{"49", 80,FUNCTION_UNALLOCATED},
		{"50", 37,FUNCTION_UNALLOCATED},
		{"M1", 1020,FUNCTION_UNALLOCATED},
		{"M2", 1021,FUNCTION_UNALLOCATED},
		{"M3", 1023,FUNCTION_UNALLOCATED},
		{"M4", 1022,FUNCTION_UNALLOCATED},
		{GPIO_NAME_RI,92,FUNCTION_UNALLOCATED}
};

static struct ext_gpio_map ext_gpio_ar7594_rev4[]={
		{"1", 86, FUNCTION_UNALLOCATED},
		{"2", 96, FUNCTION_UNALLOCATED},
		{"3", 98, FUNCTION_UNALLOCATED},
		{"4", 97, FUNCTION_UNALLOCATED},
		{"5",  1, FUNCTION_UNALLOCATED},
		{"6", 93, FUNCTION_UNALLOCATED},
		{"7", 82, FUNCTION_UNALLOCATED},
		{"8", 34, FUNCTION_UNALLOCATED},
		{"9", 30, FUNCTION_UNALLOCATED},
	/* GPIO10 <--> msmgpio29 in REV4 */
		{"10",29, FUNCTION_UNALLOCATED},
		{"11",90, FUNCTION_UNALLOCATED},
		{"12", 68,FUNCTION_UNALLOCATED},
		{"13", 53,FUNCTION_UNALLOCATED},
		{"14", 52,FUNCTION_UNALLOCATED},
		{"15", 50,FUNCTION_UNALLOCATED},
		{"16", 42,FUNCTION_UNALLOCATED},
		{"17", 88,FUNCTION_UNALLOCATED},
		{"18" ,60,FUNCTION_UNALLOCATED},
		{"19", 99,FUNCTION_UNALLOCATED},
		{"20", 94,FUNCTION_UNALLOCATED},
		{"21", -1,FUNCTION_EMBEDDED_HOST},
		{"22", -1,FUNCTION_EMBEDDED_HOST},
		{"23", 69,FUNCTION_UNALLOCATED},
		{"24", -1,FUNCTION_EMBEDDED_HOST},
		{"25", 81,FUNCTION_UNALLOCATED},
		{"26", 91,FUNCTION_UNALLOCATED},
	/* GPIO27/28 as I2C_SDA/I2C_CLK */
		{"27", 48,FUNCTION_EMBEDDED_HOST},
		{"28", 49,FUNCTION_EMBEDDED_HOST},
		{"29", 95,FUNCTION_UNALLOCATED},
	/* GPIO30-33 as UART1 function */
		{"30", 20,FUNCTION_EMBEDDED_HOST},
		{"31", 21,FUNCTION_EMBEDDED_HOST},
		{"32", 22,FUNCTION_EMBEDDED_HOST},
		{"33", 23,FUNCTION_EMBEDDED_HOST},
		{"34", 7, FUNCTION_UNALLOCATED},
		{"35", 6, FUNCTION_UNALLOCATED},
		{"36", 5, FUNCTION_EMBEDDED_HOST},
		{"37", 4, FUNCTION_EMBEDDED_HOST},
		{"38", -1,FUNCTION_EMBEDDED_HOST},
		{"39", -1,FUNCTION_EMBEDDED_HOST},
		{"40", -1,FUNCTION_EMBEDDED_HOST},
		{"41", -1,FUNCTION_EMBEDDED_HOST},
		{"42", -1,FUNCTION_EMBEDDED_HOST},
		{"43", -1,FUNCTION_EMBEDDED_HOST},
		{"44", -1,FUNCTION_EMBEDDED_HOST},
		{"45", -1,FUNCTION_EMBEDDED_HOST},
		{"46", -1,FUNCTION_EMBEDDED_HOST},
		{"47", 89,FUNCTION_UNALLOCATED},
		{"48", 87,FUNCTION_UNALLOCATED},
		{"49", 80,FUNCTION_UNALLOCATED},
		{"50", 37,FUNCTION_UNALLOCATED},
		{"M1", 1020,FUNCTION_UNALLOCATED},
		{"M2", 1021,FUNCTION_UNALLOCATED},
		{"M3", 1023,FUNCTION_UNALLOCATED},
		{"M4", 1022,FUNCTION_UNALLOCATED},
		{GPIO_NAME_RI,92,FUNCTION_UNALLOCATED}
};

static struct ext_gpio_map ext_gpio_mft[]={
	{"101", 1020,FUNCTION_UNALLOCATED},
	{"102", 1021,FUNCTION_UNALLOCATED},
	{"103", 1023,FUNCTION_UNALLOCATED},
	{"104", 1022,FUNCTION_UNALLOCATED}
};

static struct ext_gpio_map ext_gpio_ar769X[]={
		{"1",   85, FUNCTION_EMBEDDED_HOST},
		{"2",   86, FUNCTION_UNALLOCATED},
		{"3",   -1, FUNCTION_EMBEDDED_HOST},
		{"4",    1, FUNCTION_UNALLOCATED},
		{"5",   84, FUNCTION_EMBEDDED_HOST},
		{"6",   -1, FUNCTION_EMBEDDED_HOST},
		{"7",   82, FUNCTION_UNALLOCATED},
		{"8",   34, FUNCTION_UNALLOCATED},
		{"9",   -1, FUNCTION_EMBEDDED_HOST},
		{"10",  -1, FUNCTION_EMBEDDED_HOST},
		{"11",  -1, FUNCTION_EMBEDDED_HOST},
		{"12",  -1, FUNCTION_EMBEDDED_HOST},
		{"13",  68, FUNCTION_UNALLOCATED},
		{"14",  -1, FUNCTION_EMBEDDED_HOST},
		{"15",  -1, FUNCTION_EMBEDDED_HOST},
		{"16",  -1, FUNCTION_EMBEDDED_HOST},
		{"17",  87, FUNCTION_EMBEDDED_HOST},
		{"18",  80, FUNCTION_EMBEDDED_HOST},
		{"19",  89, FUNCTION_EMBEDDED_HOST},
		{"20",  -1, FUNCTION_EMBEDDED_HOST},
		{"21",  83, FUNCTION_UNALLOCATED},
		{"22",  93, FUNCTION_UNALLOCATED},
		{"23",  94, FUNCTION_UNALLOCATED},
		{"24",  30, FUNCTION_UNALLOCATED},
		{"25",  50, FUNCTION_UNALLOCATED},
		{"26",  -1, FUNCTION_EMBEDDED_HOST},
		{"27",  -1, FUNCTION_EMBEDDED_HOST},
		{"28",  96, FUNCTION_UNALLOCATED},
		{"29",  97, FUNCTION_UNALLOCATED},
		{"30",  98, FUNCTION_UNALLOCATED},
		{"31",  99, FUNCTION_UNALLOCATED},
		{"32",  69, FUNCTION_UNALLOCATED},
		{"33",  60, FUNCTION_UNALLOCATED},
		{"34",  53, FUNCTION_UNALLOCATED},
		{"35",  52, FUNCTION_UNALLOCATED},
		{"36",  90, FUNCTION_UNALLOCATED},
		{"37",  91, FUNCTION_UNALLOCATED},
		{"38",  42, FUNCTION_UNALLOCATED},
		{"39",  88, FUNCTION_UNALLOCATED},
		{"40",  29, FUNCTION_UNALLOCATED},
		{"41",  -1, FUNCTION_EMBEDDED_HOST},
		{"42",  81, FUNCTION_UNALLOCATED},
		{"43",  -1, FUNCTION_EMBEDDED_HOST},
		{"44",  -1, FUNCTION_EMBEDDED_HOST},
		{"45",  -1, FUNCTION_EMBEDDED_HOST},
		{"46",  -1, FUNCTION_EMBEDDED_HOST},
		{"47",  -1, FUNCTION_EMBEDDED_HOST},
		{"48",  -1, FUNCTION_EMBEDDED_HOST},
		{"49",  -1, FUNCTION_EMBEDDED_HOST},
		{"50",  -1, FUNCTION_EMBEDDED_HOST},
		{"51",  -1, FUNCTION_EMBEDDED_HOST},
		{"52",  -1, FUNCTION_EMBEDDED_HOST},
		{"53",  -1, FUNCTION_EMBEDDED_HOST},
		{"54",  22, FUNCTION_EMBEDDED_HOST},
		{"55",  23, FUNCTION_EMBEDDED_HOST},
		{"56",  21, FUNCTION_EMBEDDED_HOST},
		{"57",  20, FUNCTION_EMBEDDED_HOST},
		{"58",  -1, FUNCTION_EMBEDDED_HOST},
		{"59",  -1, FUNCTION_EMBEDDED_HOST},
		{"60",  -1, FUNCTION_EMBEDDED_HOST},
		{"61",  -1, FUNCTION_EMBEDDED_HOST},
		{"62",  -1, FUNCTION_EMBEDDED_HOST},
		{"63",  -1, FUNCTION_EMBEDDED_HOST},
		{"64",  -1, FUNCTION_EMBEDDED_HOST},
		{"65",  -1, FUNCTION_EMBEDDED_HOST},
		{"66",  -1, FUNCTION_EMBEDDED_HOST},
		{"67",  -1, FUNCTION_EMBEDDED_HOST},
		{"68",  -1, FUNCTION_EMBEDDED_HOST},
		{"69",  -1, FUNCTION_EMBEDDED_HOST},
		{"70",  -1, FUNCTION_EMBEDDED_HOST},
		{"71",  -1, FUNCTION_EMBEDDED_HOST},
		{"72",  -1, FUNCTION_EMBEDDED_HOST},
		{"73",  -1, FUNCTION_EMBEDDED_HOST},
		{"74",  -1, FUNCTION_EMBEDDED_HOST},
		{"75",  37, FUNCTION_EMBEDDED_HOST},
		{"76",   5, FUNCTION_EMBEDDED_HOST},
		{"77",   4, FUNCTION_EMBEDDED_HOST},
		{"78",   6, FUNCTION_EMBEDDED_HOST},
		{"79",   7, FUNCTION_EMBEDDED_HOST},
		{"80",  -1, FUNCTION_EMBEDDED_HOST},
		{"81",  -1, FUNCTION_EMBEDDED_HOST},
		{"82",  -1, FUNCTION_EMBEDDED_HOST},
		{"83",  -1, FUNCTION_EMBEDDED_HOST},
		{"84",  -1, FUNCTION_EMBEDDED_HOST},
		{"85",  -1, FUNCTION_EMBEDDED_HOST},
		{"86",  -1, FUNCTION_EMBEDDED_HOST},
		{"87",  -1, FUNCTION_EMBEDDED_HOST},
		{"88",  -1, FUNCTION_EMBEDDED_HOST},
		{"89",  -1, FUNCTION_EMBEDDED_HOST},
		{"90",  -1, FUNCTION_EMBEDDED_HOST},
		{"91",  -1, FUNCTION_EMBEDDED_HOST},
		{"92",  -1, FUNCTION_EMBEDDED_HOST},
		{"93",  -1, FUNCTION_EMBEDDED_HOST},
		{"94",  -1, FUNCTION_EMBEDDED_HOST},
		{"95",  -1, FUNCTION_EMBEDDED_HOST},
		{"96",  -1, FUNCTION_EMBEDDED_HOST},
		{"97",  -1, FUNCTION_EMBEDDED_HOST},
		{"98",  -1, FUNCTION_EMBEDDED_HOST},
		{"99",  -1, FUNCTION_EMBEDDED_HOST},
		{"100", -1, FUNCTION_EMBEDDED_HOST},
		{"101", -1, FUNCTION_EMBEDDED_HOST},
		{"102", -1, FUNCTION_EMBEDDED_HOST},
		{"103", -1, FUNCTION_EMBEDDED_HOST},
		{"104", -1, FUNCTION_EMBEDDED_HOST},
		{"105", -1, FUNCTION_EMBEDDED_HOST},
		{"106", -1, FUNCTION_EMBEDDED_HOST},
		{"107", -1, FUNCTION_EMBEDDED_HOST},
		{"108", -1, FUNCTION_EMBEDDED_HOST},
		{"109", -1, FUNCTION_EMBEDDED_HOST},
		{"110", -1, FUNCTION_EMBEDDED_HOST},
		{"111", -1, FUNCTION_EMBEDDED_HOST},
		{"112", -1, FUNCTION_EMBEDDED_HOST},
		{"113", -1, FUNCTION_EMBEDDED_HOST},
		{"114", -1, FUNCTION_EMBEDDED_HOST},
		{"115", -1, FUNCTION_EMBEDDED_HOST},
		{"116", -1, FUNCTION_EMBEDDED_HOST},
		{"117", -1, FUNCTION_EMBEDDED_HOST},
		{"118", -1, FUNCTION_EMBEDDED_HOST},
		{"119", -1, FUNCTION_EMBEDDED_HOST},
		{"120", -1, FUNCTION_EMBEDDED_HOST},
		{"121", -1, FUNCTION_EMBEDDED_HOST},
		{"122", -1, FUNCTION_EMBEDDED_HOST},
		{"123", -1, FUNCTION_EMBEDDED_HOST},
		{"124", -1, FUNCTION_EMBEDDED_HOST},
		{"125", 95, FUNCTION_UNALLOCATED},
		{"126", -1, FUNCTION_EMBEDDED_HOST},
		{"127", -1, FUNCTION_EMBEDDED_HOST},
		{"128", -1, FUNCTION_EMBEDDED_HOST},
		{"M1", 1020,FUNCTION_UNALLOCATED},
		{"M2", 1021,FUNCTION_UNALLOCATED},
		{"M3", 1023,FUNCTION_UNALLOCATED},
		{"M4", 1022,FUNCTION_UNALLOCATED},
		{GPIO_NAME_RI,92,FUNCTION_UNALLOCATED}
};

/**
 * set_gpio_bit_mask() - set the gpio bit mask in AP
 *
 *
 * Returns nothing
 *
 */
static void set_gpio_bit_mask(void)
{
	/**
	 * customer has 128 standard GPIO.
	 * The Linux Sysfs GPIO mask node:
	 * if bit="1" means available, bit="0" means unavailable.
	 *
	 */
	gpio_ext_chip.mask[0] &= 0xFFFFFFFFFFFFFFFF;
	gpio_ext_chip.mask[0] ^= 0xFFFFFFFFFFFFFFFF;
	gpio_ext_chip.mask[1] &= 0xFFFFFFFFFFFFFFFF;
	gpio_ext_chip.mask[1] ^= 0xFFFFFFFFFFFFFFFF;
}

/**
 * gpio_map_name_to_num() - Return the internal GPIO number for an
 *                         external GPIO name
 * @*buf: The external GPIO name (may include a trailing <lf>)
 * @*alias: pointer to return whether this name is an alias for another table entry
 * Context: After gpiolib_sysfs_init has setup the gpio device
 *
 * Returns a negative number if the gpio_name is not mapped to a number
 * or if the access to the GPIO is prohibited.
 *
 */
static int gpio_map_name_to_num(const char *buf, bool *alias)
{
	int i;
	int gpio_num = -1;
	char gpio_name[GPIO_NAME_MAX+1];
	int len;

	len = min( strlen(buf), sizeof(gpio_name)-1 );
	memcpy(gpio_name, buf, len);
	if ((len > 0) && (gpio_name[len-1] < 0x20))
		len--; /* strip trailing <0x0a> from buf for compare ops */
	gpio_name[len] = 0;

	if (ext_gpio != NULL)
	{
		for(i = 0; i < gpio_ext_chip.ngpio; i++)
		{
			if( strncasecmp( gpio_name, ext_gpio[i].gpio_name, GPIO_NAME_MAX ) == 0 )
			{
				/* the multi-function GPIO is used as another feature, cannot export */
				if(FUNCTION_EMBEDDED_HOST == ext_gpio[i].function)
				{
					return -1;
				}
				gpio_num = ext_gpio[i].gpio_num;
				pr_debug("%s: find GPIO %d\n", __func__, gpio_num);
				return gpio_num;
			}
		}
	}
	pr_debug("%s: Can not find GPIO %s\n", __func__, gpio_name);
	return -1;
}

/**
 * gpio_map_num_to_name() - Return the external GPIO name for an
 *                         internal GPIO number
 * @gpio_num: The internal (i.e. MDM) GPIO pin number
 * @alias: Return the second entry if 2 names are mapped to the same internal GPIO number
 * Context: After gpiolib_sysfs_init has setup the gpio device
 *
 * Returns NULL if the gpio_num is not mapped to a name
 * or if the access to the GPIO is prohibited.
 *
 */
static char *gpio_map_num_to_name(int gpio_num, bool alias)
{
	int i;

	if (ext_gpio != NULL)
	{
		for(i = 0; i < gpio_ext_chip.ngpio; i++)
		{
			if(gpio_num == ext_gpio[i].gpio_num)
			{
				if(FUNCTION_EMBEDDED_HOST == ext_gpio[i].function)
				{
					return NULL;
				}
				return ext_gpio[i].gpio_name;
			}
		}
	}
	pr_debug("%s: Can not find GPIO %d\n", __func__, gpio_num);
	return NULL;
}

static int gRmode = -1;
extern int sierra_smem_get_factory_mode(void);

static int gpio_ri = -1;

/**
 * gpio_sync_ri() - sync gpio RI function with riowner
 * Context: After ext_gpio and gpio_ri have been set.
 *
 * Returns 1 if apps, 0 if modem, or -1 if RI not found.
 */
static int gpio_sync_ri(void)
{
	int ri_owner = -1;

	if (gpio_ri >= 0) {
		/* Check if RI gpio is owned by APP core
		 * In this case, set that gpio for RI management
		 * RI owner: 1 APP , 0 Modem. See AT!RIOWNER
		 */
		ri_owner = bsgetriowner();
		if (RI_OWNER_APP == ri_owner) {
			if (ext_gpio[gpio_ri].function != FUNCTION_UNALLOCATED) {
				pr_debug("%s: RI owner is APP\n", __func__);
				ext_gpio[gpio_ri].function = FUNCTION_UNALLOCATED;
			}
		} else {
			if (ext_gpio[gpio_ri].function != FUNCTION_EMBEDDED_HOST) {
				pr_debug("%s: RI owner is Modem\n", __func__);
				ext_gpio[gpio_ri].function = FUNCTION_EMBEDDED_HOST;
			}
		}
	}

	return ri_owner;
}
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

/*
 * /sys/class/gpio/gpioN... only for GPIOs that are exported
 *   /direction
 *      * MAY BE OMITTED if kernel won't allow direction changes
 *      * is read/write as "in" or "out"
 *      * may also be written as "high" or "low", initializing
 *        output value as specified ("out" implies "low")
 *   /value
 *      * always readable, subject to hardware behavior
 *      * may be writable, as zero/nonzero
 *   /edge
 *      * configures behavior of poll(2) on /value
 *      * available only if pin can generate IRQs on input
 *      * is read/write as "none", "falling", "rising", or "both"
 *   /active_low
 *      * configures polarity of /value
 *      * is read/write as zero/nonzero
 *      * also affects existing and subsequent "falling" and "rising"
 *        /edge configuration
 */

static ssize_t gpio_direction_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags)) {
		status = -EIO;
	} else {
		gpiod_get_direction(desc);
		status = sprintf(buf, "%s\n",
			test_bit(FLAG_IS_OUT, &desc->flags)
				? "out" : "in");
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_direction_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else if (sysfs_streq(buf, "high"))
		status = gpiod_direction_output_raw(desc, 1);
	else if (sysfs_streq(buf, "out") || sysfs_streq(buf, "low"))
		status = gpiod_direction_output_raw(desc, 0);
	else if (sysfs_streq(buf, "in"))
		status = gpiod_direction_input(desc);
	else
		status = -EINVAL;

	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static /* const */ DEVICE_ATTR(direction, 0644,
		gpio_direction_show, gpio_direction_store);

/* SWISTART */
#ifdef CONFIG_SIERRA
static const struct {
	const char *name;
	unsigned long flags;
} pull_types[] = {
	{ "down",  0 },
	{ "up",       BIT(FLAG_PULL_FUNC_SEL1) },
	{ "nopull",   BIT(FLAG_PULL_FUNC_SEL2) },
	{ "keeper",   BIT(FLAG_PULL_FUNC_SEL1) | BIT(FLAG_PULL_FUNC_SEL2) },
};

static ssize_t gpio_pull_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		int i;

		status = 0;
		for (i = 0; i < ARRAY_SIZE(pull_types); i++)
			if ((desc->flags & GPIO_PULL_MASK)
					== pull_types[i].flags) {
				status = sprintf(buf, "%s\n",
							pull_types[i].name);
				break;
			}
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_pull_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else if (sysfs_streq(buf, "nopull"))
		status = gpio_set_pull(desc, MSM_GPIO_NO_PULL);
	else if (sysfs_streq(buf, "down"))
		status = gpio_set_pull(desc, MSM_GPIO_PULL_DOWN);
	else if (sysfs_streq(buf, "keeper"))
		status = gpio_set_pull(desc, MSM_GPIO_PULL_KEEPER);
	else if (sysfs_streq(buf, "up"))
		status = gpio_set_pull(desc, MSM_GPIO_PULL_UP);
	else
		status = -EINVAL;

	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static /* const */ DEVICE_ATTR(pull, 0644,
		gpio_pull_show, gpio_pull_store);
#endif
/* SWISTOP */

static ssize_t gpio_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%d\n", gpiod_get_value_cansleep(desc));

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else if (!test_bit(FLAG_IS_OUT, &desc->flags))
		status = -EPERM;
	else {
		long		value;

		status = kstrtol(buf, 0, &value);
		if (status == 0) {
			gpiod_set_value_cansleep(desc, value);
			status = size;
		}
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static DEVICE_ATTR(value, 0644,
		gpio_value_show, gpio_value_store);

static irqreturn_t gpio_sysfs_irq(int irq, void *priv)
{
	struct kernfs_node	*value_sd = priv;

	sysfs_notify_dirent(value_sd);
	return IRQ_HANDLED;
}

static int gpio_setup_irq(struct gpio_desc *desc, struct device *dev,
		unsigned long gpio_flags)
{
	struct kernfs_node	*value_sd;
	unsigned long		irq_flags;
	int			ret, irq, id;

	if ((desc->flags & GPIO_TRIGGER_MASK) == gpio_flags)
		return 0;

	irq = gpiod_to_irq(desc);
	if (irq < 0)
		return -EIO;

	id = desc->flags >> ID_SHIFT;
	value_sd = idr_find(&dirent_idr, id);
	if (value_sd)
		free_irq(irq, value_sd);

	desc->flags &= ~GPIO_TRIGGER_MASK;

	if (!gpio_flags) {
		gpio_unlock_as_irq(desc->chip, gpio_chip_hwgpio(desc));
		ret = 0;
		goto free_id;
	}

	irq_flags = IRQF_SHARED;
	if (test_bit(FLAG_TRIG_FALL, &gpio_flags))
		irq_flags |= test_bit(FLAG_ACTIVE_LOW, &desc->flags) ?
			IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
	if (test_bit(FLAG_TRIG_RISE, &gpio_flags))
		irq_flags |= test_bit(FLAG_ACTIVE_LOW, &desc->flags) ?
			IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;

	if (!value_sd) {
		value_sd = sysfs_get_dirent(dev->kobj.sd, "value");
		if (!value_sd) {
			ret = -ENODEV;
			goto err_out;
		}

		ret = idr_alloc(&dirent_idr, value_sd, 1, 0, GFP_KERNEL);
		if (ret < 0)
			goto free_sd;
		id = ret;

		desc->flags &= GPIO_FLAGS_MASK;
		desc->flags |= (unsigned long)id << ID_SHIFT;

		if (desc->flags >> ID_SHIFT != id) {
			ret = -ERANGE;
			goto free_id;
		}
	}

	ret = request_any_context_irq(irq, gpio_sysfs_irq, irq_flags,
				"gpiolib", value_sd);
	if (ret < 0)
		goto free_id;

	ret = gpio_lock_as_irq(desc->chip, gpio_chip_hwgpio(desc));
	if (ret < 0) {
		gpiod_warn(desc, "failed to flag the GPIO for IRQ\n");
		goto free_id;
	}

	desc->flags |= gpio_flags;
	return 0;

free_id:
	idr_remove(&dirent_idr, id);
	desc->flags &= GPIO_FLAGS_MASK;
free_sd:
	if (value_sd)
		sysfs_put(value_sd);
err_out:
	return ret;
}

static const struct {
	const char *name;
	unsigned long flags;
} trigger_types[] = {
	{ "none",    0 },
	{ "falling", BIT(FLAG_TRIG_FALL) },
	{ "rising",  BIT(FLAG_TRIG_RISE) },
	{ "both",    BIT(FLAG_TRIG_FALL) | BIT(FLAG_TRIG_RISE) },
};

static ssize_t gpio_edge_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		int i;

		status = 0;
		for (i = 0; i < ARRAY_SIZE(trigger_types); i++)
			if ((desc->flags & GPIO_TRIGGER_MASK)
					== trigger_types[i].flags) {
				status = sprintf(buf, "%s\n",
						 trigger_types[i].name);
				break;
			}
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_edge_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;
	int			i;

	for (i = 0; i < ARRAY_SIZE(trigger_types); i++)
		if (sysfs_streq(trigger_types[i].name, buf))
			goto found;
	return -EINVAL;

found:
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		status = gpio_setup_irq(desc, dev, trigger_types[i].flags);
		if (!status)
			status = size;
	}

	mutex_unlock(&sysfs_lock);

	return status;
}

static DEVICE_ATTR(edge, 0644, gpio_edge_show, gpio_edge_store);

static int sysfs_set_active_low(struct gpio_desc *desc, struct device *dev,
				int value)
{
	int			status = 0;

	if (!!test_bit(FLAG_ACTIVE_LOW, &desc->flags) == !!value)
		return 0;

	if (value)
		set_bit(FLAG_ACTIVE_LOW, &desc->flags);
	else
		clear_bit(FLAG_ACTIVE_LOW, &desc->flags);

	/* reconfigure poll(2) support if enabled on one edge only */
	if (dev != NULL && (!!test_bit(FLAG_TRIG_RISE, &desc->flags) ^
				!!test_bit(FLAG_TRIG_FALL, &desc->flags))) {
		unsigned long trigger_flags = desc->flags & GPIO_TRIGGER_MASK;

		gpio_setup_irq(desc, dev, 0);
		status = gpio_setup_irq(desc, dev, trigger_flags);
	}

	return status;
}

static ssize_t gpio_active_low_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%d\n",
				!!test_bit(FLAG_ACTIVE_LOW, &desc->flags));

	mutex_unlock(&sysfs_lock);

	return status;
}

static ssize_t gpio_active_low_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags)) {
		status = -EIO;
	} else {
		long		value;

		status = kstrtol(buf, 0, &value);
		if (status == 0)
			status = sysfs_set_active_low(desc, dev, value != 0);
	}

	mutex_unlock(&sysfs_lock);

	return status ? : size;
}

static DEVICE_ATTR(active_low, 0644,
		gpio_active_low_show, gpio_active_low_store);

static struct attribute *gpio_attrs[] = {
	&dev_attr_value.attr,
	&dev_attr_active_low.attr,
/* SWISTART */
#ifdef CONFIG_SIERRA
	&dev_attr_pull.attr,
#endif
/* SWISTART */
	NULL,
};
ATTRIBUTE_GROUPS(gpio);

/*
 * /sys/class/gpio/gpiochipN/
 *   /base ... matching gpio_chip.base (N)
 *   /label ... matching gpio_chip.label
 *   /ngpio ... matching gpio_chip.ngpio
 */

static ssize_t chip_base_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", chip->base);
}
static DEVICE_ATTR(base, 0444, chip_base_show, NULL);

static ssize_t chip_label_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", chip->label ? : "");
}
static DEVICE_ATTR(label, 0444, chip_label_show, NULL);

static ssize_t chip_ngpio_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", chip->ngpio);
}
static DEVICE_ATTR(ngpio, 0444, chip_ngpio_show, NULL);
/*SWISTART*/
#ifdef CONFIG_SIERRA
static ssize_t chip_mask_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "0x%08x%08x\n0x%08x%08x\n", (u32)(chip->mask[0]>>32)&0xFFFFFFFF,
		(u32)chip->mask[0]&0xFFFFFFFF, (u32)(chip->mask[1]>>32)&0xFFFFFFFF, (u32)chip->mask[1]&0xFFFFFFFF);
}
static DEVICE_ATTR(mask, 0444, chip_mask_show, NULL);
#endif
/*SWISTOP*/
static struct attribute *gpiochip_attrs[] = {
	&dev_attr_base.attr,
	&dev_attr_label.attr,
	&dev_attr_ngpio.attr,
/*SWISTART*/
#ifdef CONFIG_SIERRA
	&dev_attr_mask.attr,
#endif
/*SWISTOP*/
	NULL,
};
ATTRIBUTE_GROUPS(gpiochip);

/*
 * /sys/class/gpio/export ... write-only
 *	integer N ... number of GPIO to export (full access)
 * /sys/class/gpio/unexport ... write-only
 *	integer N ... number of GPIO to unexport
 */
static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long			gpio;
	struct gpio_desc	*desc;
	int			status;

/*SWISTART*/
#ifdef CONFIG_SIERRA
	bool alias = false;
	gRmode = sierra_smem_get_factory_mode();
	if(gRmode != 1)
	{
		gpio_sync_ri();
		status = gpio = gpio_map_name_to_num(buf, &alias);
		pr_debug("%s: sierra--find GPIO: %d \n", __func__, (int)gpio);
	}
	else
	{
		status = kstrtol(buf, 0, &gpio);
		if((gpio >= MFT_PMGPIO_NAME_MIN) && (gpio <= MFT_PMGPIO_NAME_MAX))
		{
			ext_gpio = ext_gpio_mft;
			gpio_ext_chip.ngpio = NR_EXT_GPIOS_MFT;
			status = gpio = gpio_map_name_to_num(buf, &alias);
			pr_debug("%s: sierra--find PM-GPIO: %d \n", __func__, (int)gpio);
		}
	}
#else
	status = kstrtol(buf, 0, &gpio);
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

	if (status < 0)
		goto done;

	desc = gpio_to_desc(gpio);
	/* reject invalid GPIOs */
	if (!desc) {
		pr_warn("%s: invalid GPIO %ld\n", __func__, gpio);
		return -EINVAL;
	}

	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	 */

	status = gpiod_request(desc, "sysfs");
	if (status < 0) {
		if (status == -EPROBE_DEFER)
			status = -ENODEV;
		goto done;
	}
	status = gpiod_export(desc, true);
	if (status < 0)
		gpiod_free(desc);
	else
		set_bit(FLAG_SYSFS, &desc->flags);

done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static ssize_t unexport_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long			gpio;
	struct gpio_desc	*desc;
	int			status;

/*SWISTART*/
#ifdef CONFIG_SIERRA
	bool alias = false;
	gRmode = sierra_smem_get_factory_mode();
	if(gRmode != 1)
	{
		status = gpio = gpio_map_name_to_num(buf, &alias);
		pr_debug("%s: sierra--unexport GPIO: %d \n", __func__, (int)gpio);
	}
	else
	{
		status = kstrtol(buf, 0, &gpio);
		if((gpio >= MFT_PMGPIO_NAME_MIN) && (gpio <= MFT_PMGPIO_NAME_MAX))
		{
			ext_gpio = ext_gpio_mft;
			gpio_ext_chip.ngpio = NR_EXT_GPIOS_MFT;
			status = gpio = gpio_map_name_to_num(buf, &alias);
			pr_debug("%s: sierra--unexport PM-GPIO: %d \n", __func__, (int)gpio);
		}
	}
#else
	status = kstrtol(buf, 0, &gpio);
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

	if (status < 0)
		goto done;

	desc = gpio_to_desc(gpio);
	/* reject bogus commands (gpio_unexport ignores them) */
	if (!desc) {
		pr_warn("%s: invalid GPIO %ld\n", __func__, gpio);
		return -EINVAL;
	}

	status = -EINVAL;

	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	 */
	if (test_and_clear_bit(FLAG_SYSFS, &desc->flags)) {
		status = 0;
		gpiod_free(desc);
	}
done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static struct class_attribute gpio_class_attrs[] = {
	__ATTR(export, 0200, NULL, export_store),
	__ATTR(unexport, 0200, NULL, unexport_store),
	__ATTR_NULL,
};

static struct class gpio_class = {
	.name =		"gpio",
	.owner =	THIS_MODULE,

	.class_attrs =	gpio_class_attrs,
};


/**
 * gpiod_export - export a GPIO through sysfs
 * @gpio: gpio to make available, already requested
 * @direction_may_change: true if userspace may change gpio direction
 * Context: arch_initcall or later
 *
 * When drivers want to make a GPIO accessible to userspace after they
 * have requested it -- perhaps while debugging, or as part of their
 * public interface -- they may use this routine.  If the GPIO can
 * change direction (some can't) and the caller allows it, userspace
 * will see "direction" sysfs attribute which may be used to change
 * the gpio's direction.  A "value" attribute will always be provided.
 *
 * Returns zero on success, else an error.
 */
int gpiod_export(struct gpio_desc *desc, bool direction_may_change)
{
	struct gpio_chip	*chip;
	unsigned long		flags;
	int			status;
	const char		*ioname = NULL;
	struct device		*dev;
	int			offset;
/*SWISTART*/
#ifdef CONFIG_SIERRA
	char ioname_buf[IONAME_MAX+1] = IONAME_PREFIX;
	int tmpGpio;
#endif
/*SWISTOP*/

	/* can't export until sysfs is available ... */
	if (!gpio_class.p) {
		pr_debug("%s: called too early!\n", __func__);
		return -ENOENT;
	}

	if (!desc) {
		pr_debug("%s: invalid gpio descriptor\n", __func__);
		return -EINVAL;
	}

	chip = desc->chip;

	mutex_lock(&sysfs_lock);

	/* check if chip is being removed */
	if (!chip || !chip->exported) {
		status = -ENODEV;
		goto fail_unlock;
	}

	spin_lock_irqsave(&gpio_lock, flags);
	if (!test_bit(FLAG_REQUESTED, &desc->flags) ||
	     test_bit(FLAG_EXPORT, &desc->flags)) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		gpiod_dbg(desc, "%s: unavailable (requested=%d, exported=%d)\n",
				__func__,
				test_bit(FLAG_REQUESTED, &desc->flags),
				test_bit(FLAG_EXPORT, &desc->flags));
		status = -EPERM;
		goto fail_unlock;
	}

	if (!desc->chip->direction_input || !desc->chip->direction_output)
		direction_may_change = false;
	spin_unlock_irqrestore(&gpio_lock, flags);

	offset = gpio_chip_hwgpio(desc);
	if (desc->chip->names && desc->chip->names[offset])
		ioname = desc->chip->names[offset];

/*SWISTART*/
#ifdef CONFIG_SIERRA
	gRmode = sierra_smem_get_factory_mode();
	if(gRmode != 1)
	{
		strncat(ioname_buf, gpio_map_num_to_name(desc_to_gpio(desc), false), GPIO_NAME_MAX);
		ioname = ioname_buf;
		pr_debug("%s: sierra--find GPIO,chipdev = %d,chipngpio = %d,chipbase = %d\n",
			__func__, (int)desc->chip->dev, (int)desc->chip->ngpio, (int)desc->chip->base);
	}
	else
	{
		tmpGpio = desc_to_gpio(desc);
		if((tmpGpio >= MFT_PMGPIO_NUM_MIN) && (tmpGpio <= MFT_PMGPIO_NUM_MAX))
		{
			ext_gpio = ext_gpio_mft;
			gpio_ext_chip.ngpio = NR_EXT_GPIOS_MFT;
			strncat(ioname_buf, gpio_map_num_to_name(tmpGpio, false), GPIO_NAME_MAX);
			ioname = ioname_buf;
			pr_debug("%s: sierra--find PM-GPIO,chipdev = %d,chipngpio = %d,chipbase = %d\n",
				__func__, (int)desc->chip->dev, (int)desc->chip->ngpio, (int)desc->chip->base);
		}
	}
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

	dev = device_create_with_groups(&gpio_class, desc->chip->dev,
					MKDEV(0, 0), desc, gpio_groups,
					ioname ? ioname : "gpio%u",
					desc_to_gpio(desc));
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		goto fail_unlock;
	}

	if (direction_may_change) {
		status = device_create_file(dev, &dev_attr_direction);
		if (status)
			goto fail_unregister_device;
	}

	if (gpiod_to_irq(desc) >= 0 && (direction_may_change ||
				       !test_bit(FLAG_IS_OUT, &desc->flags))) {
		status = device_create_file(dev, &dev_attr_edge);
		if (status)
			goto fail_remove_attr_direction;
	}

	set_bit(FLAG_EXPORT, &desc->flags);
	mutex_unlock(&sysfs_lock);
	return 0;

fail_remove_attr_direction:
	device_remove_file(dev, &dev_attr_direction);
fail_unregister_device:
	device_unregister(dev);
fail_unlock:
	mutex_unlock(&sysfs_lock);
	gpiod_dbg(desc, "%s: status %d\n", __func__, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpiod_export);

static int match_export(struct device *dev, const void *data)
{
	return dev_get_drvdata(dev) == data;
}

/**
 * gpiod_export_link - create a sysfs link to an exported GPIO node
 * @dev: device under which to create symlink
 * @name: name of the symlink
 * @gpio: gpio to create symlink to, already exported
 *
 * Set up a symlink from /sys/.../dev/name to /sys/class/gpio/gpioN
 * node. Caller is responsible for unlinking.
 *
 * Returns zero on success, else an error.
 */
int gpiod_export_link(struct device *dev, const char *name,
		      struct gpio_desc *desc)
{
	int			status = -EINVAL;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&sysfs_lock);

	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		struct device *tdev;

		tdev = class_find_device(&gpio_class, NULL, desc, match_export);
		if (tdev != NULL) {
			status = sysfs_create_link(&dev->kobj, &tdev->kobj,
						name);
			put_device(tdev);
		} else {
			status = -ENODEV;
		}
	}

	mutex_unlock(&sysfs_lock);

	if (status)
		gpiod_dbg(desc, "%s: status %d\n", __func__, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpiod_export_link);

/**
 * gpiod_sysfs_set_active_low - set the polarity of gpio sysfs value
 * @gpio: gpio to change
 * @value: non-zero to use active low, i.e. inverted values
 *
 * Set the polarity of /sys/class/gpio/gpioN/value sysfs attribute.
 * The GPIO does not have to be exported yet.  If poll(2) support has
 * been enabled for either rising or falling edge, it will be
 * reconfigured to follow the new polarity.
 *
 * Returns zero on success, else an error.
 */
int gpiod_sysfs_set_active_low(struct gpio_desc *desc, int value)
{
	struct device		*dev = NULL;
	int			status = -EINVAL;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&sysfs_lock);

	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		dev = class_find_device(&gpio_class, NULL, desc, match_export);
		if (dev == NULL) {
			status = -ENODEV;
			goto unlock;
		}
	}

	status = sysfs_set_active_low(desc, dev, value);
	put_device(dev);
unlock:
	mutex_unlock(&sysfs_lock);

	if (status)
		gpiod_dbg(desc, "%s: status %d\n", __func__, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpiod_sysfs_set_active_low);

/**
 * gpiod_unexport - reverse effect of gpio_export()
 * @gpio: gpio to make unavailable
 *
 * This is implicit on gpio_free().
 */
void gpiod_unexport(struct gpio_desc *desc)
{
	int			status = 0;
	struct device		*dev = NULL;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return;
	}

	mutex_lock(&sysfs_lock);

	if (test_bit(FLAG_EXPORT, &desc->flags)) {

		dev = class_find_device(&gpio_class, NULL, desc, match_export);
		if (dev) {
			gpio_setup_irq(desc, dev, 0);
			clear_bit(FLAG_EXPORT, &desc->flags);
		} else
			status = -ENODEV;
	}

	mutex_unlock(&sysfs_lock);

	if (dev) {
		device_remove_file(dev, &dev_attr_edge);
		device_remove_file(dev, &dev_attr_direction);
		device_unregister(dev);
		put_device(dev);
	}

	if (status)
		gpiod_dbg(desc, "%s: status %d\n", __func__, status);
}
EXPORT_SYMBOL_GPL(gpiod_unexport);

int gpiochip_export(struct gpio_chip *chip)
{
	int		status;
	struct device	*dev;

	/* Many systems register gpio chips for SOC support very early,
	 * before driver model support is available.  In those cases we
	 * export this later, in gpiolib_sysfs_init() ... here we just
	 * verify that _some_ field of gpio_class got initialized.
	 */
	if (!gpio_class.p)
		return 0;

	/* use chip->base for the ID; it's already known to be unique */
	mutex_lock(&sysfs_lock);
	dev = device_create_with_groups(&gpio_class, chip->dev, MKDEV(0, 0),
					chip, gpiochip_groups,
					"gpiochip%d", chip->base);
	if (IS_ERR(dev))
		status = PTR_ERR(dev);
	else
		status = 0;
	chip->exported = (status == 0);
	mutex_unlock(&sysfs_lock);

	if (status)
		chip_dbg(chip, "%s: status %d\n", __func__, status);

	return status;
}

void gpiochip_unexport(struct gpio_chip *chip)
{
	int			status;
	struct device		*dev;
	struct gpio_desc *desc;
	unsigned int i;

	mutex_lock(&sysfs_lock);
	dev = class_find_device(&gpio_class, NULL, chip, match_export);
	if (dev) {
		put_device(dev);
		device_unregister(dev);
		/* prevent further gpiod exports */
		chip->exported = false;
		status = 0;
	} else
		status = -ENODEV;
	mutex_unlock(&sysfs_lock);

	if (status)
		chip_dbg(chip, "%s: status %d\n", __func__, status);

	/* unregister gpiod class devices owned by sysfs */
	for (i = 0; i < chip->ngpio; i++) {
		desc = &chip->desc[i];
		if (test_and_clear_bit(FLAG_SYSFS, &desc->flags))
			gpiod_free(desc);
	}
}

static int __init gpiolib_sysfs_init(void)
{
	int		status;
	unsigned long	flags;
	struct gpio_chip *chip;
/*SWISTART*/
#ifdef CONFIG_SIERRA
	enum bshwtype hwtype;
	enum bshwrev hwrev;
	unsigned int gpio;
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

	status = class_register(&gpio_class);
	if (status < 0)
		return status;
/*SWISTART*/
#ifdef CONFIG_SIERRA
	/* Assign product specific GPIO mapping */
	gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR_REV4;
	ext_gpio = ext_gpio_ar7594_rev4;
	hwtype = bsgethwtype();
	hwrev = bsgethwrev();
	switch (hwtype)
	{
		case BSAR7592:
			if (hwrev < BSHWREV2)
			{
				ext_gpio = ext_gpio_ar;
				gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR;
			}
			break;
		case BSAR7594:
			if (hwrev < BSHWREV4)
			{
				ext_gpio = ext_gpio_ar;
				gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR;
			}
			break;
		case BSAR7596:
			if (hwrev < BSHWREV3)
			{
				ext_gpio = ext_gpio_ar;
				gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR;
			}
			break;
		case BSAR7598:
			if (hwrev < BSHWREV2)
			{
				ext_gpio = ext_gpio_ar;
				gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR;
			}
			break;
		case BSAR7692:
		case BSAR7694:
		case BSAR7696:
		case BSAR7698:
			gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR769X;
			ext_gpio = ext_gpio_ar769X;
		break;
		default:
			pr_err( "%s: No sysfs entries for gpio on unsupported product type:%d.\n", __func__, hwtype);
			gpio_ext_chip.ngpio = NR_EXT_GPIOS_AR_REV4;
			ext_gpio = ext_gpio_ar7594_rev4;
			break;
	}

	status = bsgetgpioflag(&(gpio_ext_chip.mask[0]), &(gpio_ext_chip.mask[1]));
	if (status < 0)
		return status;

	for (gpio = 0; gpio < gpio_ext_chip.ngpio; gpio++)
	{
		if (gpio < 64)
		{
			if (gpio_ext_chip.mask[0] & (0x1ULL << gpio))
			{
				ext_gpio[gpio].function = FUNCTION_EMBEDDED_HOST;
			}
			else
			{
				ext_gpio[gpio].function = FUNCTION_UNALLOCATED;
			}
		}
		else
		{
			if (gpio_ext_chip.mask[1] & (0x1ULL << (gpio - 64)))
			{
				ext_gpio[gpio].function = FUNCTION_EMBEDDED_HOST;
			}
			else
			{
			ext_gpio[gpio].function = FUNCTION_UNALLOCATED;
			}
		}
		if (strcasecmp(ext_gpio[gpio].gpio_name, GPIO_NAME_RI) == 0)
		{
			gpio_ri = gpio;
			gpio_sync_ri();
			break;
		}
	}
	set_gpio_bit_mask();
	status = gpiochip_export(&gpio_ext_chip);

	/* we move sierra code away from gpio_lock here because the code
	 * don't acquire a mutex or spin lock.
	 *
	 * __might_sleep() will dump out the error stack if sleeping function
	 *  is called from invaid context(spin_lock). e.g. bsreadhwconfig()
	 */
#endif /*CONFIG_SIERRA*/
/*SWISTOP*/

	/* Scan and register the gpio_chips which registered very
	 * early (e.g. before the class_register above was called).
	 *
	 * We run before arch_initcall() so chip->dev nodes can have
	 * registered, and so arch_initcall() can always gpio_export().
	 */
	spin_lock_irqsave(&gpio_lock, flags);

	list_for_each_entry(chip, &gpio_chips, list) {
		if (chip->exported)
			continue;

		/*
		 * TODO we yield gpio_lock here because gpiochip_export()
		 * acquires a mutex. This is unsafe and needs to be fixed.
		 *
		 * Also it would be nice to use gpiochip_find() here so we
		 * can keep gpio_chips local to gpiolib.c, but the yield of
		 * gpio_lock prevents us from doing this.
		 */
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = gpiochip_export(chip);
		spin_lock_irqsave(&gpio_lock, flags);
	}
	spin_unlock_irqrestore(&gpio_lock, flags);


	return status;
}
postcore_initcall(gpiolib_sysfs_init);
