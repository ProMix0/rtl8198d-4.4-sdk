include dir.mk

ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
LOCAL_IROS_HEADERS-y = \
		$(API_DIR)/include/api_common.h \
		$(API_DIR)/include/eth_port.h  \
		$(API_DIR)/include/port.h  \
		$(API_DIR)/include/qos.h  \
		$(API_DIR)/include/l2.h \
		$(API_DIR)/include/rx.h \
		$(API_DIR)/include/pkt_tx.h \
		$(API_DIR)/include/special_packet.h \
		$(API_DIR)/include/vlan.h \
		$(API_DIR)/include/flow.h \
		$(API_DIR)/include/mcast.h \
		$(API_DIR)/include/mirror.h \
		$(API_DIR)/include/classifier.h \
		$(API_DIR)/include/rate.h \
		$(API_DIR)/include/queue.h \
		$(API_DIR)/include/ptp.h \
		$(API_DIR)/include/sys.h \
		$(API_DIR)/include/macsec.h \
		$(AAL_DIR)/include/aal_ni_cputag.h
else
LOCAL_IROS_HEADERS-y =  \
		$(API_DIR)/include/api_common.h \
		$(API_DIR)/include/classifier.h \
		$(API_DIR)/include/eth_port.h \
		$(API_DIR)/include/l2.h \
		$(API_DIR)/include/mcast.h \
		$(API_DIR)/include/mirror.h \
		$(API_DIR)/include/nat.h \
		$(API_DIR)/include/pkt_tx.h \
		$(API_DIR)/include/port.h \
		$(API_DIR)/include/ptp.h \
		$(API_DIR)/include/qos.h \
		$(API_DIR)/include/queue.h \
		$(API_DIR)/include/rate.h \
		$(API_DIR)/include/route.h \
		$(API_DIR)/include/rx.h \
		$(API_DIR)/include/special_packet.h \
		$(API_DIR)/include/sys.h \
		$(API_DIR)/include/tunnel.h \
		$(API_DIR)/include/vlan.h \
		$(API_DIR)/include/flow.h \
		$(API_DIR)/include/macsec.h \
		$(AAL_DIR)/include/aal_ni_cputag.h
endif

LOCAL_IROS_HEADERS-$(CONFIG_NE_WELL_KNOWN_MULTICAST)	+=  $(API_DIR)/include/well_known_mcast_flooding.h
LOCAL_IROS_HEADERS-$(CONFIG_NE_SW_SHAPER)		+=  $(API_DIR)/include/sw_shaper.h
LOCAL_IROS_HEADERS-$(CONFIG_NE_US_RATE_MANAGER)		+=  $(API_DIR)/include/ca_us_rate_manager.h
LOCAL_IROS_HEADERS-$(CONFIG_NE_TPUT_TEST)		+=  $(API_DIR)/include/tput.h

LOCAL_IROS_HEADERS-$(CONFIG_NE_PON) += $(API_DIR)/include/epon.h \
				       $(API_DIR)/include/onu.h \
				       $(API_DIR)/include/gpon.h \
				       $(API_DIR)/include/xgpon.h \
				       $(API_DIR)/include/xgpon_aes.h \
				       $(API_DIR)/include/pon.h

LOCAL_IROS_HEADERS-$(CONFIG_NE_KERNEL_HOOK) += $(KHOOK_DIR)/khook.h

IROS_INCLUDE_HEADERS = ./$(API_DIR)/include/sys.h


IROS_HEADERS=$(addprefix $(PKG_DIR)/, $(LOCAL_IROS_HEADERS-y))

iros_headers_install:  | ${DEST} ${DEST}/include
	cp $(IROS_HEADERS) ${DEST}
	echo "$(IROS_HEADERS)" >/$(DEST)/iros_headers
	cp ${IROS_INCLUDE_HEADERS} ${DEST}/include

