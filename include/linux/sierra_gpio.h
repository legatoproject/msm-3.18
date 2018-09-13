#ifndef __LINUX_SIERRA_GPIO_H
#define __LINUX_SIERRA_GPIO_H

/* Define use cases for GPIO */
#define FUNCTION_UNALLOCATED   0
#define FUNCTION_EMBEDDED_HOST 1

#define IONAME_PREFIX "gpio"
#define GPIO_NAME_MAX 4 /* max characters in ioname following the prefix, any more will be ignored */
#define IONAME_MAX (sizeof(IONAME_PREFIX) + GPIO_NAME_MAX)
#define GPIO_NAME_RI "RI"

#define MFT_PMGPIO_NAME_MIN 101
#define MFT_PMGPIO_NAME_MAX 104
#define MFT_PMGPIO_NUM_MIN 1020
#define MFT_PMGPIO_NUM_MAX 1023

struct ext_gpio_map{
	char *gpio_name;
	int gpio_num;
	unsigned function;
};

/* customer has 50 standard GPIO + PM GPIO1-4*/
#define NR_EXT_GPIOS_AR ARRAY_SIZE(ext_gpio_ar)
#define NR_EXT_GPIOS_AR_REV4 ARRAY_SIZE(ext_gpio_ar)
#define NR_EXT_GPIOS_MFT ARRAY_SIZE(ext_gpio_ar7594_rev4)
#define NR_EXT_GPIOS_AR769X ARRAY_SIZE(ext_gpio_ar769X)

#endif /* __LINUX_SIERRA_GPIO_H */
