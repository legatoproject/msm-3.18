/* arch/arm/mach-msm/swimcu.h
 *
 * Copyright (C) 2016 Sierra Wireless, Inc
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

#ifndef __ARCH_ARM_MACH_MSM_SWIMCU_H
#define __ARCH_ARM_MACH_MSM_SWIMCU_H


/* Macro assumes SWIMCU GPIOs & ADCs start at 0 */
#define SWIMCU_GPIO_BASE                200
#define SWIMCU_GPIO_TO_SYS(mcu_gpio)    (mcu_gpio + SWIMCU_GPIO_BASE)
#define SWIMCU_NR_GPIOS                 8
#define SWIMCU_IS_GPIO(gpio)            ((gpio >= SWIMCU_GPIO_BASE) && (gpio < SWIMCU_GPIO_BASE + SWIMCU_NR_GPIOS))
#define SWIMCU_ADC_BASE                 2
#define SWIMCU_ADC_TO_SYS(mcu_adc)      (mcu_adc + SWIMCU_ADC_BASE)
#define SWIMCU_NR_ADCS                  2

/* DM: This is not used, and it's here for information only. GPIOs are moved
   to fx30 dtsi file. */
#ifdef CONFIG_SIERRA_AIRLINK_COLUMBIA

/* Macro assumes FX30EXP GPIOs (columbia gpio expander) start at 0 */
#define FX30EXP_GPIO_BASE               996
#define FX30EXP_GPIO_TO_SYS(exp_gpio)   (exp_gpio + FX30EXP_GPIO_BASE)

/* Macro assumes FX30EXP GPIOs (columbia gpio expander) start at 0 */
#define FX30SEXP_GPIO_BASE              988
#define FX30SEXP_GPIO_TO_SYS(exp_gpio)  (exp_gpio + FX30SEXP_GPIO_BASE)

#endif

#endif /*__ARCH_ARM_MACH_MSM_SWIMCU_H*/
