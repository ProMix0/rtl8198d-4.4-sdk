#
# Makefile for the Linux networking core.
#

obj-y := sock.o request_sock.o skbuff.o datagram.o stream.o scm.o \
	 gen_stats.o gen_estimator.o net_namespace.o secure_seq.o flow_dissector.o

obj-$(CONFIG_SYSCTL) += sysctl_net_core.o

obj-y		     += dev.o ethtool.o dev_addr_lists.o dst.o netevent.o \
			neighbour.o rtnetlink.o utils.o link_watch.o filter.o \
			sock_diag.o dev_ioctl.o tso.o

obj-$(CONFIG_XFRM) += flow.o
obj-y += net-sysfs.o
obj-$(CONFIG_PROC_FS) += net-procfs.o
obj-$(CONFIG_NET_PKTGEN) += pktgen.o
obj-$(CONFIG_NETPOLL) += netpoll.o
obj-$(CONFIG_FIB_RULES) += fib_rules.o
obj-$(CONFIG_TRACEPOINTS) += net-traces.o
obj-$(CONFIG_NET_DROP_MONITOR) += drop_monitor.o
obj-$(CONFIG_NETWORK_PHY_TIMESTAMPING) += timestamping.o
obj-$(CONFIG_NET_PTP_CLASSIFY) += ptp_classifier.o
obj-$(CONFIG_CGROUP_NET_PRIO) += netprio_cgroup.o
obj-$(CONFIG_CGROUP_NET_CLASSID) += netclassid_cgroup.o
obj-$(CONFIG_LWTUNNEL) += lwtunnel.o
obj-$(CONFIG_DST_CACHE) += dst_cache.o

obj-$(CONFIG_RTK_DOS) += rtk_dos.o

ifeq ($(CONFIG_RTL8686_SWITCH),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/system/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/nicDriver/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/include/
ifeq ($(CONFIG_LUNA_G3_SERIES),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-plat/1.0-r0/ca-plat-1.0/inc/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-plat/1.0-r0/ca-plat-1.0/osal/osal/linux_kernel/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-plat/1.0-r0/ca-plat-1.0/osal/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-plat/1.0-r0/ca-plat-1.0/peri/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/cortina-api/include/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/include/generated/
ifeq ($(CONFIG_RTL8198F_SERIES),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/ni-drv/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/aal/include/
endif
ifeq ($(CONFIG_CA8279_SERIES),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/ni-drv-gen1/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/aal-gen1/include/
endif
ifeq ($(CONFIG_CA8277B_SERIES),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/ni-drv-gen2/
EXTRA_CFLAGS += -I$(DIR_LINUX)/../ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/aal-gen2/include/
endif
endif
endif
ifeq ($(CONFIG_COMMON_RT_API),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/sdk/include/rtk/rt/
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/FleetConntrackDriver/rt/include/
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/FleetConntrackDriver/
endif
endif

ifeq ($(CONFIG_RTK_IGMP_MLD_SNOOPING_MODULE),m)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/igmpHookModule/
ifeq ($(CONFIG_COMMON_RT_API),y)
EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/igmpHookModule/rt_igmpHook/include/
endif
endif
EXTRA_AFLAGS += $(EXTRA_CFLAGS)
