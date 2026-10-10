EXTRA_CFLAGS += -DCONFIG_SDK_KERNEL_LINUX
EXTRA_CFLAGS += -I. -I$(KDIR)

ifeq ($(CONFIG_LUNA_G3_SERIES), y)
EXTRA_CFLAGS += -I$(ROOTDIR)/ca_packages/ca-plat/1.0-r0/ca-plat-1.0/inc \
		-I$(ROOTDIR)/ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/aal-gen2/include
else
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/include
endif

MODULE_NAME  = rtl8226b
obj-$(CONFIG_RTL_8226B_MODULE) += $(MODULE_NAME).o
$(MODULE_NAME)-objs := nic_rtl8226.o nic_rtl8226_init_merge_A_B.o nic_rtl8226b_mod.o

