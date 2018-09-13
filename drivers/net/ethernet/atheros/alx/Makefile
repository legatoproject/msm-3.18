obj-$(CONFIG_ALX) += alx.o
alx-objs := alx_main.o alx_ethtool.o alc_cb.o alc_hw.o alf_cb.o alf_hw.o
#SWISTART
#ccflags-$(CONFIG_ALX)  := -DMDM_PLATFORM
ifdef CONFIG_ALX
ccflags-y  := -DMDM_PLATFORM
endif
#SWISTOP
ccflags-y += -D__CHECK_ENDIAN__
