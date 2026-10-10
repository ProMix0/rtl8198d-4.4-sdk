D_DIR = ${ROOTDIR}
ROMFSDIR ?= ../../../romfs
WORK_DIR = $(shell pwd)
includedir:=/usr/include
libdir:=/usr/lib
CA_VER ?= 1.0-r0
dir_ca_pkg = ${CA_VER}/ca-iros-1.0
dir_ca_pkg_build = ${CA_VER}/ca-iros-1.0_build
D = $(D_DIR)/sysroot-destdir
CA_PLAT_USR=$(D_DIR)/sysroot-destdir/usr/include/ca-plat
CA_PLAT_MOD=$(D_DIR)/sysroot-destdir/usr/include/module/ca-plat
CA_IROS_USR=$(D_DIR)/sysroot-destdir/usr/include/iros
CA_LIBNL_USR=$(D_DIR)/sysroot-destdir/usr/include/libnl3
CA_LIB=$(D_DIR)/sysroot-destdir/usr/lib
CA_API=$(D)/usr/include/cortina-api
STAGING_INCDIR=$(D_DIR)/sysroot-destdir/usr/include/
STAGING_KERNEL_BUILDDIR=$(KERNEL_SRC)
SDK_HEADER_DIR=$(CA_IROS_USR)
KERNEL_VERSION ?= 4.4.3.g3-R0.5.7.0
EXTRA_LDFLAGS = -ltcl8.6 -ltclreadline-2.1.0 -lreadline -lncurses -lnl-3 -shared-libgcc -lca-plat-user -lrt
EXTRA_LDFLAGS_APP = -L$(CA_LIB) -L$(D_DIR)/sysroot-destdir/lib $(EXTRA_LDFLAGS)

SYS_D = $(WORK_DIR)/sysroot

CFLAGS += -O2 -pipe 
export CFLAGS

.DEFAULT_GOAL := all

do_compile:
# shared library
	@echo "------------------ $(dir_ca_pkg) : shared library -----------"
	$(MAKEARCH) -j $(JOBS) -C $(dir_ca_pkg_build) KCONFIG_FILE=$(STAGING_KERNEL_BUILDDIR)/include/generated/autoconf.h SDK_HEADER_DIR=$(STAGING_INCDIR)/iros \
	EXTRA_CFLAGS='-I${STAGING_INCDIR}/ca-plat -I${STAGING_INCDIR}/ca-scfg -I${STAGING_INCDIR}/libnl3'  user_libraries

# application
	@echo "------------------ $(dir_ca_pkg) : application -----------"
	$(MAKEARCH) -j $(JOBS) -C $(dir_ca_pkg_build) KCONFIG_FILE=${STAGING_KERNEL_BUILDDIR}/include/generated/autoconf.h  \
	SDK_HEADER_DIR=$(STAGING_INCDIR)/iros \
	EXTRA_CFLAGS='-I${STAGING_INCDIR}/ca-plat  -I${STAGING_INCDIR}/ca-scfg -I${STAGING_INCDIR}/tcl8.6'  EXTRA_LDFLAGS='$(EXTRA_LDFLAGS_APP)' user_app

module_do_compile:
	$(MAKEARCH) -j $(JOBS) -C $(dir_ca_pkg_build)  KERNEL_SRC=$(KERNEL_SRC)  KERNEL_PATH=$(KERNEL_SRC) KERNEL_VERSION=${KERNEL_VERSION} \
	KCONFIG=${STAGING_KERNEL_BUILDDIR}/.config SDK_HEADER_DIR=${STAGING_INCDIR}/iros \
	EXTRA_CFLAGS='-I${STAGING_INCDIR}/module/ca-plat -I${STAGING_INCDIR}/module/ca-scfg'  modules

all:
	@echo "------------------ $(dir_ca_pkg) -----------"
	cp -ar $(dir_ca_pkg) $(dir_ca_pkg_build)
	$(MAKE) do_compile
	$(MAKE) module_do_compile

install:
	$(MAKEARCH) -C $(dir_ca_pkg_build)  user_libraries_install DEST=${SYS_D}/usr/lib KCONFIG=${STAGING_KERNEL_BUILDDIR}/.config
	$(MAKEARCH) -C $(dir_ca_pkg_build) user_app_install DEST=${SYS_D} KCONFIG=${STAGING_KERNEL_BUILDDIR}/.config

	cp -arf $(SYS_D)/* $(D)
	
clean:
	rm -rf sysroot
	$(MAKE) -C $(dir_ca_pkg) KERNEL_SRC=$(KERNEL_SRC) KCONFIG=$(KERNEL_SRC)/.config clean || exit $?; 
	rm -rf $(dir_ca_pkg_build)

modules:
	$(MAKE) module_do_compile

modules_install:
	if [ -e $(dir_ca_pkg_build) ]; then \
		$(MAKE) -C $(dir_ca_pkg_build) KERNEL_SRC=$(KERNEL_SRC) KCONFIG=$(KERNEL_SRC)/.config modules_install; \
	fi
