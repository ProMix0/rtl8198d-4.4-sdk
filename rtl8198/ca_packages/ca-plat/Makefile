D_DIR = ${ROOTDIR}
JOBS ?=4
includedir:=/usr/include
libdir:=/usr/lib

dir_ca_plat = ${CA_VER}/ca-plat-1.0

CA_R19=${USE_CA_07190}
ifeq ($(CA_R19), y)
dir_ca_plat = 1.0-r19/ca-plat-1.0
endif

D = $(D_DIR)/sysroot-destdir
S = $(shell pwd)/$(dir_ca_plat)
CFLAGS += -DRTK_MEM_CORRUPTTION_FIX
#G3/R0.5.6.0/g3-eng-major-image/g3-eng.major-image.R0.5.6.0.sdk/meta-oe-yocto-cortina/meta-ca/recipes-kernel/ca-plat
.DEFAULT_GOAL := all
module_do_compile:
	$(MAKEARCH) -j $(JOBS) -C $(dir_ca_plat) KERNEL_PATH=$(KERNEL_SRC)   \
                   KERNEL_VERSION=$(KERNEL_VERSION)    \
                   modules


all: module_do_compile
	$(MAKEARCH) -C $(S)/osal/osal/linux_user SUBDIRS="${S}" all
	$(MAKEARCH) -C $(dir_ca_plat)  module_headers_install DEST=${D}${includedir}/module/ca-plat
	$(MAKEARCH) -C $(dir_ca_plat)  user_headers_install DEST=${D}${includedir}/ca-plat
	$(MAKEARCH) -C $(dir_ca_plat)  iros_headers_install DEST=${D}${includedir}/iros
	$(MAKEARCH) -C $(dir_ca_plat)  user_libraries_install DEST=${D}/${libdir}


clean:
	$(MAKE) -C $(dir_ca_plat)  clean || exit $?;
	@find ./ -name "*.d" -exec rm {} \;

modules_install:
	$(MAKEARCH) -C $(dir_ca_plat)  modules_install || exit $?;
