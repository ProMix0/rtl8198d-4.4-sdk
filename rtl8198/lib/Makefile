#
#	Makefile -- Master makefile for all libraries.
#

.EXPORT_ALL_VARIABLES:

#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_LIB=1
include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)

#
# always build these dir_
#
ifdef CONFIG_USE_RSDK_WRAPPER
dir_y  = libnet libcrypt_old
dir_n  =
dir_   =
LN     = ln
else #CONFIG_USE_RSDK_WRAPPER
dir_y  = $(LIBCDIR) libnet libcrypt_old
dir_n  =
dir_   =

ifeq ($(LIBCDIR),libc)
dir_y += libm
endif
endif #CONFIG_USE_RSDK_WRAPPER


#
# and build these if someone needs them
#
dir_$(CONFIG_LIB_LIBFFI)       += libffi
dir_$(CONFIG_LIB_LIBFFI_FORCE) += libffi
dir_$(CONFIG_LIB_SNAPGEAR)     += libsnapgear
dir_$(CONFIG_LIB_LIBAES)       += libaes
dir_$(CONFIG_LIB_LIBAES_FORCE) += libaes
dir_$(CONFIG_LIB_LIBDES)       += libdes
dir_$(CONFIG_LIB_LIBDES_FORCE) += libdes
dir_$(CONFIG_LIB_LIBSSL)       += libssl
dir_$(CONFIG_LIB_LIBSSL_FORCE) += libssl
dir_$(CONFIG_LIB_LIBNL_FORCE)  += libnl-1.1.4
dir_$(CONFIG_LIB_LIBMATRIXSSL) += libmatrixssl
dir_$(CONFIG_LIB_LIBGMP)       += libgmp
dir_$(CONFIG_LIB_LIBGMP_FORCE) += libgmp
dir_$(CONFIG_LIB_LIBG)         += libg
dir_$(CONFIG_LIB_LIBG_FORCE)   += libg
dir_$(CONFIG_LIB_LIBPAM)       += libpam
dir_$(CONFIG_LIB_LIBPAM_FORCE) += libpam
dir_$(CONFIG_LIB_LIBPCAP)      += libpcap
dir_$(CONFIG_LIB_LIBPCAP_FORCE)+= libpcap
dir_$(CONFIG_LIB_ZLIB)         += zlib
dir_$(CONFIG_LIB_ZLIB_FORCE)   += zlib
dir_$(CONFIG_LIB_LIBATM)       += libatm
dir_$(CONFIG_LIB_LIBATM_FORCE) += libatm
dir_$(CONFIG_LIB_LIBPNG)       += libpng
dir_$(CONFIG_LIB_LIBPNG_FORCE) += libpng
dir_$(CONFIG_LIB_LIBJPEG)      += libjpeg
dir_$(CONFIG_LIB_LIBJPEG_FORCE)+= libjpeg
dir_$(CONFIG_LIB_LIBSNAPGEARPP) += libsnapgear++
dir_$(CONFIG_LIB_LIBCCMALLOC)  += libccmalloc
dir_$(CONFIG_LIB_LIBMD5_FORCE) += libmd5
dir_$(CONFIG_LIB_LIBMNL)	   += libmnl
dir_$(CONFIG_LIB_LIBMD5_FORCE) += libmnl
dir_$(CONFIG_LIB_LIBNFNETLINK) += libnfnetlink
dir_$(CONFIG_LIB_LIBNFNETLINK_FORCE) += libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_LOG) += libnetfilter_log #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_LOG_FORCE) += libnetfilter_log #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_QUEUE) += libnetfilter_queue #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_QUEUE_FORCE) += libnetfilter_queue #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CONNTRACK) += libnetfilter_conntrack #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CONNTRACK_FORCE) += libnetfilter_conntrack #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CTHELPER) += libnetfilter_cthelper #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTHELPER_FORCE) += libnetfilter_cthelper #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTTIMEOUT) += libnetfilter_cttimeout #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTTIMEOUT_FORCE) += libnetfilter_cttimeout #depends on libmnl
# Mason Yu. for ipv6
dir_$(CONFIG_LIB_FLEX) += libflex
dir_$(CONFIG_LIB_NCURSES)      += ncurses
dir_$(CONFIG_LIB_NCURSES_FORCE)+= ncurses
dir_$(CONFIG_LIB_LIBTERMCAP)      += libtermcap
dir_$(CONFIG_LIB_LIBTERMCAP_FORCE)+= libtermcap
dir_$(CONFIG_LIB_LIBUDEV_FORCE) += eudev-3.2
dir_$(CONFIG_LIB_LIBUSB_FORCE) += libusb-1.0.21
dir_$(CONFIG_LIB_XZ)			+= xz-5.0.8
dir_$(CONFIG_LIB_XZ_FORCE)		+= xz-5.0.8
dir_$(CONFIG_LIB_LIBICONV)		+=libiconv

dir_$(CONFIG_LIB_JSON)              += json-c-0.13.1
dir_$(CONFIG_LIB_LIBUBOX)           += libubox-2018-07-25
dir_$(CONFIG_LIB_UBUS)           += ubus-2018-07-26

#for RTK created
dir_$(CONFIG_LIB_LIBRTK_TIMERLIST)			+= librtk_timerlist
dir_$(CONFIG_LIB_LIBRTK_TIMERLIST_FORCE)	+= librtk_timerlist

dir_$(CONFIG_LIB_SQLITE)                      += sqlite
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I),$(CONFIG_LIB_LIBSSL_1_1_1K)),y)
OPTIONS_SSL=no-ec_nistp_64_gcc_128 no-md2 no-rc5 no-rfc3779 no-sctp no-zlib no-zlib-dynamic no-static-engine no-tests
else
OPTIONS_SSL=no-ec_nistp_64_gcc_128 no-gmp no-jpake no-krb5 no-md2 no-rc5 no-rfc3779 no-sctp no-store no-zlib no-zlib-dynamic no-static-engine
endif

ifeq ($(CONFIG_USER_RTK_NESSUS_SCAN_FIXED),y)
OPTIONS_SSL += no-rc4
endif

.PHONY: all
all: pre_compile
ifeq ($(findstring glibc,$(LIBCDIR)),glibc)
	[ -d $(LIBCDIR) ] || mkdir $(LIBCDIR)
	if [ ! -f $(LIBCDIR)/config.cache ]; then \
		cd $(LIBCDIR); \
		export CFLAGS="$(VENDOR_CFLAGS) -DEMBED -O2"; \
		unset LDFLAGS; \
		../configure  --prefix= \
			$(GLIBC_TARGET_OPTS) --build=i386-redhat-linux \
			--with-headers=$(ROOTDIR)/$(LINUXDIR)/include \
			--without-cvs --without-gd \
			--disable-profile --disable-force-install \
			--enable-add-ons ; \
	fi
endif
ifdef CONFIG_USE_RSDK_WRAPPER
	[ -d $(ROOTDIR)/lib/librtk ] || mkdir -p $(ROOTDIR)/lib/librtk
	[ -d $(ROOTDIR)/include/rtk ] || mkdir -p $(ROOTDIR)/include/rtk
	[ -d $(ROOTDIR)/include/rtk/linux ] || mkdir -p $(ROOTDIR)/include/rtk/linux
  ifeq ($(CONFIG_USER_JSON_C), y)
	$(LN) -fs $(ROOTDIR)/user/json-c/json.h $(ROOTDIR)/include/rtk/json.h
	$(LN) -fs $(ROOTDIR)/user/json-c/debug.h $(ROOTDIR)/include/rtk/debug.h
	$(LN) -fs $(ROOTDIR)/user/json-c/linkhash.h $(ROOTDIR)/include/rtk/linkhash.h
	$(LN) -fs $(ROOTDIR)/user/json-c/arraylist.h $(ROOTDIR)/include/rtk/arraylist.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_util.h $(ROOTDIR)/include/rtk/json_util.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_object.h $(ROOTDIR)/include/rtk/json_object.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_pointer.h $(ROOTDIR)/include/rtk/json_pointer.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_tokener.h $(ROOTDIR)/include/rtk/json_tokener.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_object_iterator.h $(ROOTDIR)/include/rtk/json_object_iterator.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_c_version.h $(ROOTDIR)/include/rtk/json_c_version.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_inttypes.h $(ROOTDIR)/include/rtk/json_inttypes.h
	$(LN) -fs $(ROOTDIR)/user/json-c/json_config.h $(ROOTDIR)/include/rtk/json_config.h
  endif
  ifeq ($(CONFIG_LIB_JSON), y)
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json.h $(ROOTDIR)/include/rtk/json.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/debug.h $(ROOTDIR)/include/rtk/debug.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/linkhash.h $(ROOTDIR)/include/rtk/linkhash.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/arraylist.h $(ROOTDIR)/include/rtk/arraylist.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_util.h $(ROOTDIR)/include/rtk/json_util.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_object.h $(ROOTDIR)/include/rtk/json_object.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_pointer.h $(ROOTDIR)/include/rtk/json_pointer.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_tokener.h $(ROOTDIR)/include/rtk/json_tokener.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_object_iterator.h $(ROOTDIR)/include/rtk/json_object_iterator.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_c_version.h $(ROOTDIR)/include/rtk/json_c_version.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_inttypes.h $(ROOTDIR)/include/rtk/json_inttypes.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/json_config.h $(ROOTDIR)/include/rtk/json_config.h
	$(LN) -fs $(ROOTDIR)/lib/json-c-0.13.1/printbuf.h $(ROOTDIR)/include/rtk/printbuf.h
  endif
  ifeq ($(CONFIG_E8B) $(CONFIG_RTK_L34_ENABLE),y y) # CONFIG_E8B and CONFIG_RTK_L34_ENABLE are defined
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/options.h $(ROOTDIR)/include/rtk/options.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/mib.h $(ROOTDIR)/include/rtk/mib.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/mibtbl.h $(ROOTDIR)/include/rtk/mibtbl.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/sysconfig.h $(ROOTDIR)/include/rtk/sysconfig.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/utility.h $(ROOTDIR)/include/rtk/utility.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/rtk_interface.h $(ROOTDIR)/include/rtk/rtk_interface.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/adslif.h $(ROOTDIR)/include/rtk/adslif.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/adsl_drv.h $(ROOTDIR)/include/rtk/adsl_drv.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/subr_dhcpv6.h $(ROOTDIR)/include/rtk/subr_dhcpv6.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/ipv6_info.h $(ROOTDIR)/include/rtk/ipv6_info.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/subr_wlan.h $(ROOTDIR)/include/rtk/subr_wlan.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/subr_net.h $(ROOTDIR)/include/rtk/subr_net.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/rtk_timer.h $(ROOTDIR)/include/rtk/rtk_timer.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/msgq.h $(ROOTDIR)/include/rtk/msgq.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/tz.h $(ROOTDIR)/include/rtk/tz.h
ifdef CONFIG_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
endif
ifdef CONFIG_CU_BASEON_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/subr_cu.h $(ROOTDIR)/include/rtk/subr_cu.h
endif
ifdef CONFIG_CTC_SDN
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_ctcsdn.h $(ROOTDIR)/include/rtk/subr_ctcsdn.h
endif
        ifdef CONFIG_LUNA
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/rtusr_rg_api.h $(ROOTDIR)/include/rtk/rtusr_rg_api.h
	$(LN) -fs $(ROOTDIR)/user/boa_e8/src/LINUX/chip_deps.h $(ROOTDIR)/include/rtk/chip_deps.h
        ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/sockmark_define.h $(ROOTDIR)/include/rtk/sockmark_define.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/fc_api.h $(ROOTDIR)/include/rtk/fc_api.h
        endif
        endif
  else
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/options.h $(ROOTDIR)/include/rtk/options.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/mib.h $(ROOTDIR)/include/rtk/mib.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/mibtbl.h $(ROOTDIR)/include/rtk/mibtbl.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/sysconfig.h $(ROOTDIR)/include/rtk/sysconfig.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/utility.h $(ROOTDIR)/include/rtk/utility.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/rtk_interface.h $(ROOTDIR)/include/rtk/rtk_interface.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/adslif.h $(ROOTDIR)/include/rtk/adslif.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/adsl_drv.h $(ROOTDIR)/include/rtk/adsl_drv.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_dhcpv6.h $(ROOTDIR)/include/rtk/subr_dhcpv6.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/ipv6_info.h $(ROOTDIR)/include/rtk/ipv6_info.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_wlan.h $(ROOTDIR)/include/rtk/subr_wlan.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_net.h $(ROOTDIR)/include/rtk/subr_net.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/rtk_timer.h $(ROOTDIR)/include/rtk/rtk_timer.h
        ifdef CONFIG_USER_XDSL_SLAVE
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_nfbi_api.h $(ROOTDIR)/include/rtk/subr_nfbi_api.h
        endif #CONFIG_USER_XDSL_SLAVE
	$(LN) -fs $(ROOTDIR)/user/boa/src/msgq.h $(ROOTDIR)/include/rtk/msgq.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/tz.h $(ROOTDIR)/include/rtk/tz.h
ifdef CONFIG_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
endif
ifdef CONFIG_CU_BASEON_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_cu.h $(ROOTDIR)/include/rtk/subr_cu.h
endif
ifdef CONFIG_E8B
	$(LN) -fs $(ROOTDIR)/user/boa/src/defs_e8.h $(ROOTDIR)/include/rtk/defs_e8.h
endif
ifdef CONFIG_CTC_SDN
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_ctcsdn.h $(ROOTDIR)/include/rtk/subr_ctcsdn.h
endif
        ifdef CONFIG_LUNA
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/chip_deps.h $(ROOTDIR)/include/rtk/chip_deps.h
        ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/sockmark_define.h $(ROOTDIR)/include/rtk/sockmark_define.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/fc_api.h $(ROOTDIR)/include/rtk/fc_api.h
        endif
        endif
  endif # ifdef CONFIG_E8B
ifdef CONFIG_CMCC
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/network_core.h $(ROOTDIR)/include/rtk/network_core.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/accessservices.h $(ROOTDIR)/include/rtk/accessservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/addressservices.h $(ROOTDIR)/include/rtk/addressservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/commservices.h $(ROOTDIR)/include/rtk/commservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/lanservices.h $(ROOTDIR)/include/rtk/lanservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/transferservices.h $(ROOTDIR)/include/rtk/transferservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/voipservices.h $(ROOTDIR)/include/rtk/voipservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/mangement.h $(ROOTDIR)/include/rtk/mangement.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/permission.h $(ROOTDIR)/include/rtk/permission.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/rtkdebug.h $(ROOTDIR)/include/rtk/rtkdebug.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/osgidebug.h $(ROOTDIR)/include/rtk/osgidebug.h
	$(LN) -fs $(ROOTDIR)/user/libosgi_ipc/osgi_ipc.h $(ROOTDIR)/include/rtk/osgi_ipc.h
endif
ifdef CONFIG_CU_BASEON_CMCC
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/network_core.h $(ROOTDIR)/include/rtk/network_core.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/accessservices.h $(ROOTDIR)/include/rtk/accessservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/addressservices.h $(ROOTDIR)/include/rtk/addressservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/commservices.h $(ROOTDIR)/include/rtk/commservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/lanservices.h $(ROOTDIR)/include/rtk/lanservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/transferservices.h $(ROOTDIR)/include/rtk/transferservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/voipservices.h $(ROOTDIR)/include/rtk/voipservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/mangement.h $(ROOTDIR)/include/rtk/mangement.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/permission.h $(ROOTDIR)/include/rtk/permission.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/rtkdebug.h $(ROOTDIR)/include/rtk/rtkdebug.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/osgidebug.h $(ROOTDIR)/include/rtk/osgidebug.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_cu.h $(ROOTDIR)/include/rtk/subr_cu.h
	$(LN) -fs $(ROOTDIR)/user/libosgi_ipc/osgi_ipc.h $(ROOTDIR)/include/rtk/osgi_ipc.h
endif
ifdef CONFIG_DEV_xDSL
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/realtek/867x_sar/ra867x_atm.h $(ROOTDIR)/include/rtk/linux/ra867x_atm.h ;
endif

ifdef CONFIG_ATM
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/net/rtl/rtl_atm.h $(ROOTDIR)/include/rtk/linux/rtl_atm.h
endif

endif #CONFIG_USE_RSDK_WRAPPER
	for i in $(dir_y) ; do \
		if [ -d "$$i" ]; then \
			if [ ! -f $$i/Makefile ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			fi; \
			$(MAKE) -C $$i; \
		fi \
	done
ifeq ($(CONFIG_LIB_LIBSSL_FORCE),y)
	if [ -e libssl/libcrypto.so ]; then \
		cp libssl/libcrypto.so librtk; \
	fi
	if [ -e libssl/libssl.so ]; then \
		cp libssl/libssl.so librtk; \
	fi
endif

romfs:
	for i in $(dir_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done
	-$(STRIP) $(ROMFSDIR)/lib/*

%_menuconfig:
	[ ! -d "$(@:_menuconfig=)" ] || $(MAKE) -C $(@:_menuconfig=) menuconfig

%_only:
	if [ -d "$(@:_only=)" ]; then \
		if [ ! -f $(@:_only=)/Makefile ]; then \
			cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
		fi; \
		$(MAKE) -C $(@:_only=); \
	fi

%_romfs:
	[ ! -d "$(@:_romfs=)" ] || $(MAKE) -C $(@:_romfs=) romfs

%_clean:
	[ ! -d "$(@:_clean=)" ] || $(MAKE) -C $(@:_clean=) clean
#if define CONFIG_LIB_LIBSSL or CONFIG_LIB_LIBSSL_FORCE
ifeq ($(or $(CONFIG_LIB_LIBSSL),$(CONFIG_LIB_LIBSSL_FORCE)), y)
	if [ "$(@:_clean=)" = "libssl" -a -f libssl/config_done ]; then \
		rm libssl/config_done ; \
	fi
endif

clean:
ifdef CONFIG_USE_RSDK_WRAPPER
	-rm -rf $(ROOTDIR)/lib/librtk
endif #CONFIG_USE_RSDK_WRAPPER
ifeq ($(findstring glibc,$(LIBCDIR)),glibc)
	-rm -rf $(LIBCDIR)
endif
	-for i in $(dir_y) $(dir_n) $(dir_); do \
		if [ $$i = "sqlite" ]; then \
			[ ! -d $$i ] || $(MAKE) -C $$i distclean; \
			[ ! -d sqlite ] || rm -rf sqlite; \
		else \
			[ ! -d $$i ] || $(MAKE) -C $$i clean ; \
		fi; \
	done
#if define CONFIG_LIB_LIBSSL or CONFIG_LIB_LIBSSL_FORCE
ifeq ($(or $(CONFIG_LIB_LIBSSL),$(CONFIG_LIB_LIBSSL_FORCE)), y)
	[ ! -f libssl/config_done ] || rm libssl/config_done;
endif

ifeq ($(CONFIG_LIB_LIBUBOX),y)
	[ ! -d $(ROOTDIR)/user/lib ] || rm -rf $(ROOTDIR)/user/lib;
endif

ifeq ($(CONFIG_LIB_UBUS),y)
	[ ! -e ubus-2018-07-26/Makefile ] || rm ubus-2018-07-26/Makefile;
endif


pre_compile:
ifeq ($(CONFIG_LIB_LIBSSL),y)
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I),$(CONFIG_LIB_LIBSSL_1_1_1K)),y)
	if [ ! -f libssl/config_done ]; then \
		cd libssl; \
		./Configure shared $(OPTIONS_SSL) uClinux-dist CC=gcc LD=ld AR=ar LDFLAGS=-pthread; \
		cat Makefile.rtk >> Makefile; \
		$(MAKE) depend;touch config_done; \
		cd - ;\
	fi
else
	if [ ! -f libssl/config_done ]; then \
	cd libssl;./Configure shared $(OPTIONS_SSL) uClinux-dist; \
	$(MAKE) depend;touch config_done; \
	sed -i 's/^MAKEDEPPROG= ccache/MAKEDEPPROG= /g' Makefile; \
	fi
endif
endif

ifeq ($(CONFIG_LIB_LIBSSL_FORCE),y)
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I),$(CONFIG_LIB_LIBSSL_1_1_1K)),y)
	if [ ! -f libssl/config_done ]; then \
		cd libssl; \
		./Configure shared  $(OPTIONS_SSL) uClinux-dist CC=gcc LD=ld AR=ar LDFLAGS=-pthread; \
		cat Makefile.rtk >> Makefile; \
		$(MAKE) depend;touch config_done; \
	fi
else
	if [ ! -f libssl/config_done ]; then \
	cd libssl;./Configure shared $(OPTIONS_SSL) uClinux-dist; \
	$(MAKE) depend;touch config_done; \
	fi
endif
endif
	@echo "finished pre_compile"

#cd libssl;./Configure no-zlib no-asm no-hw no-dso no-comp no-engine shared no-ssl2 no-ssl3 no-sse2 no-aes no-bf no-cast no-dh  no-dsa no-ec no-ecdh no-ecdsa no-idea no-md2 no-md4 no-mdc2 no-rc2 no-rc5 no-ripemd no-rmd160 no-krb5  mips-linux;$(MAKE) depend;touch config_done; \



ifeq ($(CONFIG_LIB_JSON),y)
	@echo "Generate lib";
	if [ ! -d $(ROOTDIR)/user/lib ]; then \
		mkdir -p $(ROOTDIR)/user/lib; \
	fi; \
	#json-c
	@echo "build json-c"
	@if [ ! -d $(ROOTDIR)/user/lib/include/json-c ]; then \
		rm -rf json-c-0.13.1;\
		tar -xvzf json-c-0.13.1.tar.gz; \
		cd ./json-c-0.13.1; \
		LD="$(CROSS_COMPILE)ld" \
		./configure --prefix="$(ROOTDIR)/user/lib" --host=mips-linux; \
		echo "romfs:	" >> Makefile; \
		$(MAKE) V=s; \
		$(MAKE) install; \
		cd -; \
	fi
endif



#libubox-2014-08-04 git version dffbc09baf71b294185a36048166d00066d433b5
ifeq ($(CONFIG_LIB_LIBUBOX),y)
	@echo "build libubox"
	@if [ ! -d $(ROOTDIR)/user/lib/include/libubox ]; then \
		rm -rf libubox-2018-07-25;\
		tar -xvzf libubox-2018-07-25.tar.gz; \
		cd ./libubox-2018-07-25; \
		sed 's/OPTION(BUILD_LUA \"build Lua plugin\" ON)/OPTION(BUILD_LUA \"build Lua plugin\" OFF)/g' -i CMakeLists.txt; \
		sed 's/PKG_CHECK_MODULES(JSONC json-c json)/PKG_CHECK_MODULES(JSONC json-c)/g' -i CMakeLists.txt; \
		sed 's/find_library(json NAMES json-c json)/find_library(json NAMES libjson-c.a libjson.a)/g' -i CMakeLists.txt; \
		LDFLAGS="-lm" PKG_CONFIG_PATH="$(ROOTDIR)/user/lib/lib/pkgconfig" cmake CFLAGS="-fPIC -fPIE" -DCMAKE_INSTALL_PREFIX="$(ROOTDIR)/user/lib/" -DCMAKE_FIND_ROOT_PATH="$(ROOTDIR)/user/lib/" .; \
		echo "romfs:    " >> Makefile; \
		$(MAKE); \
		$(MAKE) install; \
		cd -; \
	fi
endif

#ubus-2014-09-17 git version 4c4f35cf2230d70b9ddd87638ca911e8a563f2f3
ifeq ($(CONFIG_LIB_UBUS),y)
	@echo "build ubus"
	@if [ ! -e ./ubus-2018-07-26/Makefile ]; then \
		rm -rf ubus-2018-07-26; \
		tar -xvzf ubus-2018-07-26.tar.gz; \
		cd ./ubus-2018-07-26; \
		sed 's/OPTION(BUILD_LUA \"build Lua plugin\" ON)/OPTION(BUILD_LUA \"build Lua plugin\" OFF)/g' -i CMakeLists.txt; \
		sed 's/OPTION(ENABLE_SYSTEMD \"systemd support\" ON)/OPTION(ENABLE_SYSTEMD \"systemd support\" OFF)/g' -i CMakeLists.txt; \
		sed 's/find_library(json NAMES json-c json)/find_library(json NAMES libjson-c.a libjson.a)/g' -i CMakeLists.txt; \
		sed '16a'"INCLUDE_DIRECTORIES($(ROOTDIR)/user/lib/include)" -i CMakeLists.txt; \
		sed '16a'"INCLUDE_DIRECTORIES($(ROOTDIR)/user/lib/include/json-c)" -i CMakeLists.txt; \
		sed '16a'"LINK_DIRECTORIES($(ROOTDIR)/user/lib/lib)" -i CMakeLists.txt; \
		sed '16a'"ADD_DEFINITIONS(-DJSONC)" -i CMakeLists.txt; \
		LDFLAGS="-lm" PKG_CONFIG_PATH="$(ROOTDIR)/user/lib/lib/pkgconfig" cmake -DCMAKE_INSTALL_PREFIX="$(ROOTDIR)/user/lib/" -DCMAKE_FIND_ROOT_PATH="$(ROOTDIR)/user/lib" .; \
		echo "romfs:    " >> Makefile; \
		echo "	$(ROMFSINST) ubusd /bin" >> Makefile; \
		echo "	$(ROMFSINST) ubus /bin" >> Makefile; \
		echo "	$(ROMFSINST) examples/client /bin" >> Makefile; \
		echo "	$(ROMFSINST) examples/server /bin" >> Makefile; \
		echo "	$(ROMFSINST) \"*.so\" /lib" >> Makefile; \
		echo "	$(ROMFSINST) ../libubox-2018-07-25/\"*.so\" /lib" >> Makefile; \
		echo "	$(ROMFSINST) $(ROOTDIR)/user/lib/lib/libjson-c.so.4.0.0 /lib" >> Makefile; \
		echo "	$(ROMFSINST) -s libjson-c.so.4.0.0 /lib/libjson-c.so.4" >> Makefile; \
		echo "	$(ROMFSINST) -s libjson-c.so.4.0.0 /lib/libjson-c.so" >> Makefile; \
		cd -; \
	fi
endif

#sqlite3
ifeq ($(CONFIG_LIB_SQLITE),y)
	@echo "build sqlite3"
	@if [ ! -d $(ROOTDIR)/lib/sqlite ]; then \
		unzip sqlite.zip; \
	fi
	@if [ ! -f $(ROOTDIR)/lib/sqlite/Makefile ]; then \
		cd sqlite; \
		./configure --disable-tcl --host=$(CROSS_HOST) --target=$(CROSS_TARGET); \
		echo "romfs:	" >> Makefile; \
		echo "	$(ROMFSINST) $(ROOTDIR)/lib/sqlite/sqlite3 /bin/sqlite3" >> Makefile; \
		echo "	$(ROMFSINST) -s libsqlite3.so.0.8.6 /lib/libsqlite3.so.0" >> Makefile; \
		echo "	$(ROMFSINST) -s libsqlite3.so.0 /lib/libsqlite3.so" >> Makefile; \
		echo "	$(ROMFSINST) $(ROOTDIR)/lib/sqlite/.libs/libsqlite3.so.0.8.6 /lib/libsqlite3.so.0.8.6" >> Makefile; \
		cd -; \
	fi	
endif
