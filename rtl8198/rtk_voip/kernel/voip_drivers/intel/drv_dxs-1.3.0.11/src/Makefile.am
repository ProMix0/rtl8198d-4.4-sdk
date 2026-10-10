#
#
#                              Copyright (c) 2014
#                            Lantiq Deutschland GmbH
#                             http://www.lantiq.com
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#
#
if WITH_KERNEL_MODULE
if KERNEL_2_6
bin_PROGRAMS = drv_dxs.ko
# dummy file required for distcheck target
drv_dxs_ko_SOURCES = @top_srcdir@/LICENSE
else
bin_PROGRAMS = drv_dxs
endif
else
lib_LIBRARIES = libdrvdxs.a
endif WITH_KERNEL_MODULE_FALSE

## Common DUSLIC XS driver sources
drv_dxs_SOURCES = \
   drv_dxs_alm_aclm.c \
   drv_dxs_access.c \
   drv_dxs_mbx.c \
   drv_dxs_outbox.c \
   drv_dxs_fifo.c \
   drv_dxs_init.c \
   drv_dxs_irq.c \
   drv_dxs_bbd.c \
   drv_dxs_gpio.c \
   drv_dxs_alm.c \
   drv_dxs_alm_lt.c \
   drv_dxs_ioctl.c \
   drv_dxs_dwld.c \
   drv_dxs_cid.c \
   drv_dxs_cid_sm.c \
   drv_dxs_pcm.c \
   drv_dxs_tg.c \
   drv_dxs_dtmf.c \
   drv_dxs_dtmf_sm.c \
   drv_dxs_utd.c \
   drv_dxs_event.c \
   drv_dxs_debug.c \
   lib/lib_bbd/src/lib_bbd.c

if WITH_KERNEL_MODULE
drv_dxs_SOURCES+= drv_dxs_linux.c
DXS_SPEC_DIST = drv_dxs_dev_io.c
else
# library sources
libdrvdxs_a_SOURCES=$(drv_dxs_SOURCES) \
     drv_dxs_dev_io.c
DXS_SPEC_DIST = drv_dxs_linux.c
endif

EXTRA_DIST = \
   drv_dxs_alm_aclm.h \
   ../drv_dxs_version.h \
   ../include/drv_dxs_errno.h \
   ../include/drv_dxs_strerrno.h \
   ../include/drv_dxs_io.h \
   ../include/drv_dxs_io_types.h \
   ../include/drv_dxs_dev_io.h \
   ../include/lib_bbd.h \
   drv_dxs_config.h \
   drv_dxs_irq.h \
   drv_dxs_access.h \
   drv_dxs_mbx.h \
   drv_dxs_fifo.h \
   drv_dxs_bbd.h \
   drv_dxs_dwld.h \
   drv_dxs_gpio.h \
   drv_dxs_gpio_user.h \
   drv_dxs_gpio_priv.h \
   drv_dxs_tg.h \
   drv_dxs_dtmf.h \
   drv_dxs_dtmf_priv.h \
   drv_dxs_pcm.h \
   drv_dxs_pcm_priv.h \
   drv_dxs_alm.h \
   drv_dxs_alm_priv.h \
   drv_dxs_alm_lt.h \
   drv_dxs_cid.h \
   drv_dxs_cid_priv.h \
   drv_dxs_utd.h \
   drv_dxs_init.h \
   drv_dxs_os.h \
   drv_dxs.h \
   drv_dxs_api.h \
   drv_dxs_fw_headers.h \
   drv_dxs_fw_cmd_sdd.h \
   drv_dxs_fw_cmd_eop.h \
   drv_dxs_debug.h \
   drv_dxs_ll_if_version.h \
   drv_config_user.template.h \
   drv_config_user.easy3201.h \
   drv_config_user.easy3201.evs.h \
   drv_config_user.xrx500.h \
   drv_config_user.grx750.h \
   drv_config_user.usr.h \
   drv_dxs_alm_gr909.c \
   drv_dxs_alm_gr909.h

nodist_drv_dxs_SOURCES =

if EVALUATION
nodist_drv_dxs_SOURCES += \
   drv_dxs_eval.c \
   drv_dxs_evalproc.c

EXTRA_DIST += \
   drv_dxs_eval.h \
   drv_dxs_evalproc.h
endif

nodist_EXTRA_drv_dxs_SOURCES =\
   ../drv_dxs_config_defines.h

INCLUDES_NO_KERNEL = \
   -I@abs_top_builddir@ \
   -I@abs_top_srcdir@ \
   -I@abs_srcdir@ \
   -I@IFXOS_INCL_PATH@ \
   -I@abs_top_srcdir@/src \
   -I@abs_top_srcdir@/include \
   -I@abs_top_srcdir@/src/lib/lib_bbd/src

if HL_TAPI_INCL_PATH_SET
INCLUDES_NO_KERNEL += -I@HL_TAPI_INCL_PATH@ -I@HL_TAPI_INCL_PATH@/include
else
INCLUDES_NO_KERNEL += -I@abs_top_srcdir@/src/drv_tapi/include
endif

if EVENT_LOGGER_DEBUG
INCLUDES_NO_KERNEL += -I@EVENT_LOGGER_INCL_PATH@
endif


INCLUDES = \
   $(INCLUDES_NO_KERNEL)

if WITH_KERNEL_MODULE
INCLUDES += \
   -I@KERNEL_INCL_PATH@ \
   -I@KERNEL_INCL_PATH@/asm/mach-generic \
   -I@KERNEL_BUILD_PATH@/include \
   -I@KERNEL_BUILD_PATH@/include2 \
   -I@BOARD_INCL_PATH@
endif

libdrvdxs_additional_cflags=\
	-DLINUX \
	-DRW_MULTI_THREAD \
	-D_REENTRANT \
	-D_THREAD_SAFE \
	-D_POSIX_SOURCE -D_GNU_SOURCE \
	-DIFXOS_USE_DEV_IO=1

if WARNINGS
AM_CFLAGS = -Wall -Wimplicit -Wreturn-type -Wunused -Wswitch -Wcomment -Wparentheses
#-Wpointer-arith
else
AM_CFLAGS = -Wall
endif

if WITH_KERNEL_MODULE
drv_dxs_CFLAGS =-D__LINUX__ -DLINUX -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB $(AM_CFLAGS)
if KERNEL_2_6
drv_dxs_CFLAGS += -fno-common
endif

drv_dxs_LDFLAGS = -r -nostdlib
endif WITH_KERNEL_MODULE

# -H
ghs:
	ccppc $(addprefix @srcdir@/,$(drv_dxs_SOURCES)) $(INCLUDES_NO_KERNEL) \
	-c -gcc -Wall -bigendian -D__BYTE_ORDER=__BIG_ENDIAN \
	-DHAVE_CONFIG_H -DGENERIC_OS -DGREENHILLS_CHECK

lint:
	@flint @top_srcdir@/std_lx.lnt $(INCLUDES) \
	-I$(shell dirname `$(CC) -print-file-name=include`)/include \
	$(drv_dxs_SOURCES) $(nodist_drv_dxs_SOURCES)

rsm:
	rsm -fa -n -s -Ta $(drv_dxs_SOURCES) $(EXTRA_DIST)

install-exec-hook: $(bin_PROGRAMS)
	@for p in $(bin_PROGRAMS); do \
		$(STRIP) --strip-debug $(DESTDIR)$(bindir)/$$p; \
		echo "  $(STRIP) --strip-debug $(DESTDIR)$(bindir)/$$p"; \
	done

if WITH_KERNEL_MODULE
if KERNEL_2_6
drv_dxs_OBJS  = "$(subst .c,.o, $(drv_dxs_SOURCES))"
if EVALUATION
drv_dxs_OBJS += "$(subst .c,.o, $(nodist_drv_dxs_SOURCES))"
endif

drv_dxs.ko: $(drv_dxs_SOURCES) $(nodist_drv_dxs_SOURCES) $(nodist_EXTRA_drv_dxs_SOURCES) $(EXTRA_DIST)
	@echo -e "Making Linux 2.6.x kernel object"
	@for f in $(drv_dxs_SOURCES) ; do \
		if test ! -e $(PWD)/$$f; then \
			echo "  LN      $$f" ; \
			mkdir -p $(PWD)/`dirname $$f`/; \
			ln -s @abs_srcdir@/$$f $(PWD)/$$f; \
		fi; \
	done;
if EVALUATION
	@for f in $(nodist_drv_dxs_SOURCES) ; do \
		if test ! -e $(PWD)/$$f; then \
			echo "  LN      $$f" ; \
			mkdir -p $(PWD)/`dirname $$f`/; \
			ln -s @abs_srcdir@/$$f $(PWD)/$$f; \
		fi; \
	done;
endif

	@echo -e "# drv_dxs: Generated to build Linux 2.6.x kernel object" > $(PWD)/Kbuild
	@echo -e "obj-m := $(subst .ko,.o,$@)"			>> $(PWD)/Kbuild
	@echo -e "$(subst .ko,,$@)-y := $(drv_dxs_OBJS)"	>> $(PWD)/Kbuild
	@echo -e "EXTRA_CFLAGS := -DHAVE_CONFIG_H  $(drv_dxs_CFLAGS) $(INCLUDES)"	>> $(PWD)/Kbuild
	$(MAKE) ARCH=@KERNEL_ARCH@ -C @KERNEL_BUILD_PATH@ O=@KERNEL_BUILD_PATH@ M=$(PWD) modules

clean-generic:
	@echo "drv_dxs: Cleanup Linux 2.6.x kernel object build"
	@- find . -name ".*.cmd" | xargs rm -f
	@- find . -name "*.o" | xargs rm -f
	@- rm -f Module.symvers Kbuild modules.order
	@- rm -rf .tmp_versions drv_dxs.mod.c
endif

else
# library flags
libdrvdxs_a_CFLAGS = $(libdrvdxs_additional_cflags) \
	$(AM_CFLAGS)

endif WITH_KERNEL_MODULE_FALSE
