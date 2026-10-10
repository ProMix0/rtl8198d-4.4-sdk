D_DIR = ${ROOTDIR}
includedir:=/usr/include
libdir:=/usr/lib
JOBS ?=4
dir_ca_package = ${CA_VER}/ca-cli-$(CA_MAIN_VER)
CA_R19=${USE_CA_07190}
ifeq ($(CA_R19), y)
dir_ca_package = 1.0-r19/ca-cli-1.0
endif


S = $(dir_ca_package)
D = $(D_DIR)/sysroot-destdir
CA_TARGET ?= g3
#CFLAGS += -L$(D)/$(libdir)
CA_PLAT_DIR=$(D_DIR)/sysroot-destdir/usr/include/module/ca-plat
CA_PLAT_USRDIR=$(D_DIR)/sysroot-destdir/usr/include/ca-plat
STAGING_INCDIR=$(D_DIR)/sysroot-destdir/usr/include
STAGING_KERNEL_DIR=$(KERNEL_DIR)
export CA_PLAT_DIR
EXTRA_CFLAGS += -I$(CA_PLAT_USRDIR) -DRTK_MEM_CORRUPTTION_FIX
EXTRA_LDFLAGS = -L$(D)/$(libdir) -lca-plat-user -lrt -lpthread  -shared-libgcc
#LDFLAGS += -L$(D)/$(libdir)
#export LDFLAGS EXTRA_CFLAGS
#G3/R0.5.6.0/g3-eng-major-image/g3-eng.major-image.R0.5.6.0.sdk/meta-oe-yocto-cortina/meta-ca/recipes-kernel/ca-cli
#MAKE_TARGETS = OSAL_DIR=${STAGING_INCDIR} OSAL_KDIR=${STAGING_INCDIR}/module/ca-plat AAL_H_DIR=${STAGING_INCDIR}/cli modules

MAKEPACK = $(MAKEARCH) $(MAKE_TARGETS)

.DEFAULT_GOAL := all

.PHONY: install

do_compile:
	$(MAKEARCH) -j $(JOBS) -C ${S} KERNEL_SRC=${STAGING_KERNEL_DIR}  EXTRA_CFLAGS='${EXTRA_CFLAGS}' \
	EXTRA_LDFLAGS='${EXTRA_LDFLAGS}' \
	AAL_H_DIR=${STAGING_INCDIR}/cli  user_app

module_do_compile:
	$(MAKEARCH) -j $(JOBS) -C ${S}  KERNEL_SRC=${STAGING_KERNEL_DIR} KERNEL_PATH=${STAGING_KERNEL_DIR}   \
	KERNEL_VERSION=${KERNEL_VERSION} EXTRA_CFLAGS='-I$(CA_PLAT_DIR)'  AAL_H_DIR=${STAGING_INCDIR}/cli modules
	

all:
	@echo "--------- Package : $(dir_ca_package) ---------------"
	$(MAKE) do_compile
	@echo "--------- module_do_compile ---------------"
	$(MAKE) module_do_compile
	$(MAKEARCH) install
	
   
install:
	mkdir -p ${D}/usr/bin
	cp ${S}/app-cli/app-cli  ${D}/usr/bin
	mkdir -p ${D}${includedir}
	cp ${S}/include/cli_api.h ${D}${includedir}
	cp ${S}/include/cli_cmn.h ${D}${includedir}
	cp ${S}/include/cli_product.h ${D}${includedir}
	mkdir -p ${D}${libdir}
	cp ${S}/app-cli/libca-cli.so ${D}${libdir}

clean:
	$(MAKE) -C $(dir_ca_package) clean || exit $?; \

modules_install:
	$(MAKEPACK) -C $(dir_ca_package) modules_install || exit $?; \

