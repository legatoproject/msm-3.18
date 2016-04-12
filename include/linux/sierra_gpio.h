#ifndef __LINUX_SIERRA_GPIO_H
#define __LINUX_SIERRA_GPIO_H

/* Define use cases for GPIO */
#define FUNCTION_UNALLOCATED   0
#define FUNCTION_EMBEDDED_HOST 1

#define IONAME_PREFIX "gpio"
#define GPIO_NAME_MAX 4 /* max characters in ioname following the prefix, any more will be ignored */
#define IONAME_MAX (sizeof(IONAME_PREFIX) + GPIO_NAME_MAX)


struct ext_gpio_map{
	char *gpio_name;
	int gpio_num;
	unsigned function;
};

/* customer has 30 standard GPIO + PM GPIO1-4*/
#define NR_EXT_GPIOS_AR ARRAY_SIZE(ext_gpio_ar)
static struct ext_gpio_map ext_gpio_ar[]={
	{"1", 76,FUNCTION_UNALLOCATED},
	{"2", 77,FUNCTION_UNALLOCATED},
	{"3", 78,FUNCTION_UNALLOCATED},
	{"4", 79,FUNCTION_UNALLOCATED},
	{"5", 30,FUNCTION_UNALLOCATED},
	{"6", 45,FUNCTION_UNALLOCATED},
	{"7", 52,FUNCTION_UNALLOCATED},
	{"8", 43,FUNCTION_UNALLOCATED},
	{"9", 49,FUNCTION_UNALLOCATED},
	{"10", 50,FUNCTION_UNALLOCATED},
	{"11", 16,FUNCTION_UNALLOCATED},
	{"12", 17,FUNCTION_UNALLOCATED},
	{"13", 37,FUNCTION_UNALLOCATED},
	{"14", 36,FUNCTION_UNALLOCATED},
	{"M1", 1020,FUNCTION_UNALLOCATED},
	{"M2", 1021,FUNCTION_UNALLOCATED},
	{"M3", 1023,FUNCTION_UNALLOCATED},
	{"M4", 1022,FUNCTION_UNALLOCATED}
};


#endif /* __LINUX_SIERRA_GPIO_H */