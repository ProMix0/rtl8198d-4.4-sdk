#obj-y += rtk_proc.o netdev_reset.o
#obj-$(CONFIG_RTK_NETLOG)		+= netlog.o
#obj-$(CONFIG_USB)	+= rtk_usb_phy.o
obj-$(CONFIG_RTK_KERNEL_OMD) += rtl_signalCapture.o
#obj-$(CONFIG_RTK_HS_UART) += hs_uart.o
ifeq ($(CONFIG_RTK_WLAN_EVENT_INDICATE),y)
ifeq ($(CONFIG_RTK_DEV_AP),y)
obj-y += rtk_wlan_event_netlink_for_dev_ap.o
else
ifeq ($(CONFIG_YUEME),y)
obj-y += rtk_wlan_event_netlink.o
else
ifeq ($(CONFIG_CU_BASEON_YUEME),y)
obj-y += rtk_wlan_event_netlink.o nf_traffic_monitor.o nf_traffic_detail_netlink.o  nf_traffic_detail_process.o nf_traffic_detail_process_h3.o
else
ifeq ($(CONFIG_CMCC), y)
obj-y += rtk_wlan_event.o nf_traffic_detail_netlink.o  nf_traffic_detail_process.o nf_traffic_detail_process_h3.o
else
ifeq ($(CONFIG_CU_BASEON_CMCC), y)
obj-y += rtk_wlan_event.o nf_traffic_detail_netlink.o  nf_traffic_detail_process.o nf_traffic_detail_process_h3.o
endif
endif
endif
endif
endif
endif
obj-y += framework_protector/

ifdef CONFIG_SYSTEM_CAPABILITY
obj-y += sys_capability.o
endif
ifdef CONFIG_UNI_CAPABILITY
obj-y += uni_capability.o
endif
obj-$(CONFIG_RTK_VBUS_CHAIN) += vbus_chain.o
obj-$(CONFIG_RTK_ARM64_WATCHPOINT_DEBUG) += rtk_arm64_data_breakpoint.o
