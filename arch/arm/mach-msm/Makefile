obj-$(CONFIG_USE_OF) += board-dt.o
obj-$(CONFIG_ARCH_MDM9640) += board-9640.o
obj-$(CONFIG_ARCH_MDM9607) += board-9607.o
obj-$(CONFIG_ARCH_MDM9650) += board-9650.o
obj-$(CONFIG_ARCH_SDXHEDGEHOG) += board-hedgehog.o
obj-$(CONFIG_ARCH_MSM8916) += board-8952.o
obj-$(CONFIG_ARCH_MSM8917) += board-8917.o
obj-$(CONFIG_ARCH_MSM8920) += board-8920.o
obj-$(CONFIG_ARCH_MSM8937) += board-8937.o
obj-$(CONFIG_ARCH_MSM8940) += board-8940.o
obj-$(CONFIG_ARCH_MSM8953) += board-8953.o
obj-$(CONFIG_ARCH_MSM8909) += board-8909.o

#SMP
obj-$(CONFIG_SMP) += headsmp.o platsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o

# SWISTART
obj-y += sierra_smem.o sierra_smem_msg.o sierra_smem_errdump.o sierra_smem_mode.o
obj-y += sierra_bscommon.o sierra_serial.o
#LXSWIREF-1#obj-y += sierra_monitor.o
CFLAGS_sierra_smem_mode.o := -I$(src)
CFLAGS_sierra_bscommon.o := -I$(src)

obj-$(CONFIG_SIERRA) += sierra/
# SWISTOP

