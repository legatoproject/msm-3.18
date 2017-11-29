#ifndef __LINUX_SIERRA_GPIO_H
#define __LINUX_SIERRA_GPIO_H

/* Define use cases for GPIO */
#define FUNCTION_UNALLOCATED   0
#define FUNCTION_EMBEDDED_HOST 1

#define IONAME_PREFIX "gpio"
#define GPIO_NAME_MAX 4 /* max characters in ioname following the prefix, any more will be ignored */
#define IONAME_MAX (sizeof(IONAME_PREFIX) + GPIO_NAME_MAX)
#define GPIO_NAME_RI "RI"

extern int sierra_smem_get_factory_mode(void);

struct ext_gpio_map{
	char *gpio_name;
	int gpio_num;
	unsigned function;
};

#endif /* __LINUX_SIERRA_GPIO_H */