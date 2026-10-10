
include dir.mk

LOCAL_API_HEADERS-y = \
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
		$(API_DIR)/include/macsec.h

LOCAL_API_HEADERS-$(CONFIG_NE_SW_SHAPER) += $(API_DIR)/include/sw_shaper.h
LOCAL_API_HEADERS-$(CONFIG_NE_US_RATE_MANAGER) += $(API_DIR)/include/ca_us_rate_manager.h
LOCAL_API_HEADERS-$(CONFIG_NE_TPUT_TEST) += $(API_DIR)/include/tput.h


LOCAL_API_HEADERS-$(CONFIG_NE_PON) += $(API_DIR)/include/epon.h \
				      $(API_DIR)/include/gpon.h \
				      $(API_DIR)/include/onu.h \
				      $(API_DIR)/include/pon.h \
				      $(API_DIR)/include/xgpon.h \
				      $(API_DIR)/include/xgpon_aes.h \
				      $(PLOAM_DIR)/adapt/include/ploam_adapt.h \
				      $(PLOAM_DIR)/cmn/ploam_api.h \
				      $(PLOAM_DIR)/cmn/ploam_cmn.h \
				      $(PLOAM_DIR)/std/include/ploam_gpon.h \
				      $(PLOAM_DIR)/std/include/ploam_main.h \
				      $(PLOAM_DIR)/std/include/ploam_ngpon2.h \
				      $(PLOAM_DIR)/std/include/ploam_pkt.h \
				      $(AAL_DIR)/include/aal_pon.h

USER_HEADERS= $(ADAPT_DIR)/l2fp/*.h

API_HEADERS = $(addprefix $(PKG_DIR)/, $(LOCAL_API_HEADERS-y))
module_headers_install:  | ${DEST}
	echo "$(API_HEADERS)" >/$(DEST)/api_headers
	cp $(API_HEADERS) ${USER_HEADERS} ${DEST}

user_headers_install: | ${DEST}
	cp $(API_HEADERS) ${DEST}

