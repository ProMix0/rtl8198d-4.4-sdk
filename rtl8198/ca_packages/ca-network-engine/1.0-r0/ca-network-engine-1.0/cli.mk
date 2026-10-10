include dir.mk

LOCAL_CLI_HEADERS-y= \
	$(AAL_DIR)/include/aal_arb.h \
	$(AAL_DIR)/include/aal_fdb.h \
	$(AAL_DIR)/include/aal_l2_cls.h \
	$(AAL_DIR)/include/aal_l2_qm.h \
	$(AAL_DIR)/include/aal_l2_specpkt.h \
	$(AAL_DIR)/include/aal_l2_te.h \
	$(AAL_DIR)/include/aal_l2_tm_cb.h \
	$(AAL_DIR)/include/aal_l2_tm.h \
	$(AAL_DIR)/include/aal_l2_vlan.h \
	$(AAL_DIR)/include/aal_l3_cam.h \
	$(AAL_DIR)/include/aal_l3_cls.h \
	$(AAL_DIR)/include/aal_l3_if.h \
	$(AAL_DIR)/include/aal_l3_l2lookup.h \
	$(AAL_DIR)/include/aal_l2_qos.h \
	$(AAL_DIR)/include/aal_l3_specpkt.h \
	$(AAL_DIR)/include/aal_l3_stg0.h \
	$(AAL_DIR)/include/aal_l3_te_cb.h \
	$(AAL_DIR)/include/aal_l3_te.h \
	$(AAL_DIR)/include/aal_l3_tm.h \
	$(AAL_DIR)/include/aal_mcast.h \
	$(AAL_DIR)/include/aal_mir.h \
	$(AAL_DIR)/include/aal_ni_cputag.h \
	$(AAL_DIR)/include/aal_ni_l2.h \
	$(AAL_DIR)/include/aal_port.h \
	$(AAL_DIR)/include/aal_phy.h

ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
LOCAL_CLI_HEADERS-y += $(AAL_DIR)/include/aal_l3_qos.h
endif

ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
LOCAL_CLI_HEADERS-y += $(AAL_DIR)/include/aal_uni_macsec.h \
					 $(AAL_DIR)/include/aal_uni_macsec_intr.h
endif

ifdef CONFIG_ARCH_CORTINA_VENUS
LOCAL_CLI_HEADERS-y += ${AAL_DIR}/include/aal_l3_ddos.h
LOCAL_CLI_HEADERS-y += ${AAL_DIR}/include/aal_l2fe.h
endif

CLI_INCLUDE_HEADERS-y += $(AAL_DIR)/include/aal_phy.h

LOCAL_CLI_HEADERS-$(CONFIG_NE_AAL_XFI) += $(AAL_DIR)/include/aal_xfi.h


LOCAL_CLI_HEADERS-$(CONFIG_NE_PON) += $(AAL_DIR)/include/aal_puc.h \
				      $(AAL_DIR)/include/aal_pdc.h \
				      $(AAL_DIR)/include/aal_xcvr.h \
				      $(AAL_DIR)/include/aal_psds.h \
				      $(AAL_DIR)/include/aal_epon_mac.h \
				      $(AAL_DIR)/include/aal_epon.h \
				      $(AAL_DIR)/include/aal_epon_fec.h \
				      $(AAL_DIR)/include/aal_epon_crypto.h \
				      $(AAL_DIR)/include/aal_mpcp.h \
				      $(AAL_DIR)/include/aal_epon_intr.h \
				      $(AAL_DIR)/include/aal_ploam.h \
				      $(AAL_DIR)/include/aal_gpon.h \
				      $(AAL_DIR)/include/aal_gpon_intr.h \
				      $(AAL_DIR)/include/aal_xgpon_actp.h \
				      $(AAL_DIR)/include/aal_xgpon_fec.h \
				      $(AAL_DIR)/include/aal_xgpon_intr.h \
				      $(AAL_DIR)/include/aal_xgpon.h

CLI_INCLUDE_HEADERS-$(CONFIG_NE_PON) += $(AAL_DIR)/include/aal_pon.h


CLI_HEADERS =$(addprefix $(PKG_DIR)/, $(LOCAL_CLI_HEADERS-y))

cli_headers_install :  | ${DEST} ${DEST}/include
	cp $(CLI_HEADERS) ${DEST}
	echo "$(CLI_HEADERS)" >$(DEST)/cli_headers
	cp ${CLI_INCLUDE_HEADERS-y} ${DEST}/include

