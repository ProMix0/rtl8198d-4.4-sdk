#
# Makefile for the IEEE 802.1d ethernet bridging layer.
#

ifeq ($(CONFIG_RTL_A4_STA_SUPPORT),y)
        EXTRA_CFLAGS += -DA4_STA
endif		


ifeq ($(CONFIG_RTL_MESH_SUPPORT),y)
        EXTRA_CFLAGS += -DCONFIG_RTK_MESH
endif

ifeq ($(CONFIG_RTL_MULTI_REPEATER_MODE_SUPPORT),y)
        EXTRA_CFLAGS += -DCONFIG_RTK_GUEST_ZONE
endif		

obj-$(CONFIG_BRIDGE) += bridge.o

bridge-y	:= br.o br_device.o br_fdb.o br_forward.o br_if.o br_input.o \
			br_ioctl.o br_stp.o br_stp_bpdu.o \
			br_stp_if.o br_stp_timer.o br_netlink.o

bridge-$(CONFIG_SYSFS) += br_sysfs_if.o br_sysfs_br.o

bridge-$(CONFIG_BRIDGE_RTK_IGMPV3_SNOOPING) += rtk_br_igmpv3_hook.o

bridge-$(subst m,y,$(CONFIG_BRIDGE_NETFILTER)) += br_nf_core.o

br_netfilter-y := br_netfilter_hooks.o
br_netfilter-$(subst m,y,$(CONFIG_IPV6)) += br_netfilter_ipv6.o
obj-$(CONFIG_BRIDGE_NETFILTER) += br_netfilter.o

bridge-$(CONFIG_BRIDGE_IGMP_SNOOPING) += br_multicast.o br_mdb.o

bridge-$(CONFIG_BRIDGE_VLAN_FILTERING) += br_vlan.o

obj-$(CONFIG_NETFILTER) += netfilter/

obj-$(CONFIG_BRIDGE_RTK_IGMPV3_SNOOPING) += rtk_igmpv3_database/
#for loopback detection include "rtk_rg_struct.h"
#if defined(CONFIG_E8B) || defined(CONFIG_EPON_FEATURE)
ifneq ($(or CONFIG_E8B,CONFIG_EPON_FEATURE),)
ifeq ($(CONFIG_ARCH_CORTINA_G3HGU),y)
EXTRA_CFLAGS += -include $(APOLLODIR)/ca-ne/include/generated/ca_ne_autoconf.h
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-ne/ni-drv
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-ne/aal/include
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-plat/inc
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-plat/osal/osal/linux_kernel
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-plat/osal/include
EXTRA_CFLAGS += -I$(APOLLODIR)/ca-ne/cortina-api/include
endif
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/system/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/romeDriver/

EXTRA_AFLAGS += $(EXTRA_CFLAGS)
endif
bridge-$(CONFIG_BRIDGE_VLAN_TP) += br_vlan_tp.o
