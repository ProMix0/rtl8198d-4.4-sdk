#
# Makefile for the Realtek network device drivers.
#

# For 8198F OpenWRT workaround
ifeq ($(CONFIG_OPENWRT_SDK),y)
$(shell ln -fs $(TOPDIR)/tmp/ca-network-engine-1.0/ $(APOLLODIR)/ca-ne)
$(shell ln -fs $(TOPDIR)/tmp/ca-plat-1.0/ $(APOLLODIR)/ca-plat)
endif

obj-$(CONFIG_8139CP) += 8139cp.o
obj-$(CONFIG_8139TOO) += 8139too.o
obj-$(CONFIG_ATP) += atp.o
obj-$(CONFIG_R8169) += r8169.o

obj-$(CONFIG_RTL9607C_SERIES) += ethctl_implement.o

ifeq ($(CONFIG_MODULE_BUILD_IN),y)
obj-$(CONFIG_RTL8686_SWITCH) += rtl86900/
else
obj-m += rtl86900/
endif

obj-$(CONFIG_RTK_DEV_AP) += rtk_vlan_common.o
obj-$(CONFIG_RTK_DEV_AP) += rtk_common_utility.o
obj-$(CONFIG_RTK_DNS_TRAP) += rtk_dns_trap.o
obj-$(CONFIG_RTW_MEMPOOL) += rtk_mempool.o

# Add symbolic link to the NIC driver .h file
ifeq ($(CONFIG_RTL9600_SERIES),y)
${shell ln -fs $(NETDIR)/rtl86900/nicDriver/re8686.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9601B_SERIES),y)
${shell ln -fs $(NETDIR)/rtl86900/nicDriver/re8686_rtl9601b.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9602C_SERIES),y)
${shell ln -fs $(NETDIR)/rtl86900/nicDriver/re8686_rtl9602c.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9607C_SERIES),y)
${shell ln -fs $(NETDIR)/rtl86900/nicDriver/re8686_rtl9607c.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RG_G3_SERIES),y)
${shell ln -fs $(NETDIR)/rtl86900/nicDriver/re8686_rtl9607c.h $(NETDIR)/re8686_rtl9607c.h}
endif

ifeq ($(CONFIG_RTK_DEV_AP),y)
obj-$(CONFIG_RTK_LINUX_VLAN_SUPPORT) += rtk_linux_vlan.o
endif
obj-$(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT) += rtk_vlan_passthrough.o
obj-$(CONFIG_RTK_VLAN_BINDING_SUPPORT) += rtk_lan_vlan.o
obj-$(CONFIG_RTK_CTCAPD_FILTER_SUPPORT) += rtk_dns_filter.o

ifeq ($(CONFIG_RTL_MULTI_ETH_WAN),y)
obj-$(CONFIG_RTK_DEV_AP) += rtk_multi_wan_vlan.o
obj-$(CONFIG_RTL_SMUX_LEGACY) += smux.o
obj-$(CONFIG_RTL_SMUX_DEV) += smuxdev.o
endif

ifeq ($(CONFIG_RTL8686_SWITCH),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/include/common/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/nicDriver/
endif
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
ifeq ($(CONFIG_COMMON_RT_API),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/include/rtk/rt/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/FleetConntrackDriver/rt/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/igmpHookModule/rt_igmpHook/include/
endif
-include $(NETDIR)/rtl86900/Compiler_Flag
EXTRA_CFLAGS += $(PON_CFLAGS)
endif

ifeq ($(CONFIG_RTL_83XX_SUPPORT),y)
obj-m += rtl83xx/
endif

obj-$(CONFIG_RTL_8226B_SUPPORT) += rtl8226B/
obj-$(CONFIG_RTL_8221B_SUPPORT) += rtl8221B/

EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -Werror
