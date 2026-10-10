#
#	Makefile -- Build instructions for user2 level apps
#

.EXPORT_ALL_VARIABLES:

#
# Include architecture specific build rules.
#

-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(LINUX_CONFIG)

ifneq ($(wildcard $(BACKPORT_CONFIG)),)
-include $(BACKPORT_CONFIG)
endif

CFLAGS += -include $(ROOTDIR)/autoconf.h
CFLAGS += -include $(ROOTDIR)/include/linux/autoconf.h

dir_y = proc_var modutils 

dir_$(CONFIG_USER_GDB_GDBSERVER)            += gdbserver
#ifeq ($(CONFIG_ARM64),y)
	dir_$(CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT) += rtk_nv
#endif
dir_$(CONFIG_USER_DHRYSTONE)                += dhrystone-master
#dir_$(CONFIG_USER_DHRYSTONE_SMP)           += dhrystone_smp tinymembench_v0.4.9
dir_$(CONFIG_USER_THREADTEST)               += threadtest_ntpl
dir_$(CONFIG_CORTINA_EUDEV) += eudev
dir_$(CONFIG_USER_G3_MTD_UTILS) += mtd-utils
dir_$(CONFIG_USER_G3_WATCHDOG) += rtk_watchdog
dir_$(CONFIG_WIFI_TEST_SCRIPTS) += wifi_script
dir_$(CONFIG_GATOR_DAEMON) += gator-daemon
dir_$(CONFIG_USER_G3_PERF) += perf
dir_$(CONFIG_USER_LMBENCH)	+= lmbench
dir_y += dhrystone_smp-bin
dir_y += rtk_upgrader

all: configure
	for i in $(dir_y) ; do \
		if [ -d $$i ]; then \
			if [ ! -f $$i/Makefile ]; then \
				cd $$i && ./configure --host=aarch64-linux --target=aarch64-linux && cd -; \
			fi; \
			$(MAKE) -C $$i || exit $?; \
		fi; \
	done	


romfs: 
	for i in $(dir_y) ; do if [ -d $$i ]; then $(MAKE) -C $$i romfs || exit $? ; fi; done


.PHONY: rcX
rcX: 

	for i in $(dir_y) ; do \
		if [ -d $$i ]; then \
		if [ "`grep "^rcX:" $$i/Makefile`" != "" ]; then \
			$(MAKE) -C $$i rcX || exit $? ; \
		fi \
		fi \
	done	


clean:
	for i in $(dir_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i clean; \
	done 



configure:
ifdef CONFIG_CORTINA_EUDEV
	$(MAKE) -C eudev configure || exit $? ; 
endif

