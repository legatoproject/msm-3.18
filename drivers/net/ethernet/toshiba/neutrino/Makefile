#  Makefile for the Toshiba neutrino network device drivers..
#

obj-$(CONFIG_NEUTRINO_NET) += DWC_ETH_QOS.o

DWC_ETH_QOS-objs := DWC_ETH_QOS_dev.o \
        DWC_ETH_QOS_drv.o  \
        DWC_ETH_QOS_desc.o   \
        DWC_ETH_QOS_ethtool.o   \
        DWC_ETH_QOS_pci.o      \
        DWC_ETH_QOS_mdio.o  \
        DWC_ETH_QOS_eee.o      \
        DWC_ETH_QOS_plt_init.o

DWC_ETH_QOS-$(CONFIG_NEUTRINO_PTPSUPPORT_OBJ) += DWC_ETH_QOS_ptp.o
ccflags-$(CONFIG_NEUTRINO_PTPSUPPORT_OBJ) += -DCONFIG_PTPSUPPORT_OBJ
DWC_ETH_QOS-$(CONFIG_NEUTRINO_IPA_OFFLOAD) += DWC_ETH_QOS_ipa.o
ccflags-$(CONFIG_NEUTRINO_IPA_OFFLOAD) += -DDWC_ETH_QOS_ENABLE_IPA


