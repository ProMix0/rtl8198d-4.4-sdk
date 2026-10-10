D_DIR = ${ROOTDIR}
includedir:=/usr/include
libdir:=/usr/lib
WORK_DIR=$(shell pwd)
dir_ca_package = ${CA_VER}/ca-network-engine-1.0
D = $(D_DIR)/sysroot-destdir
SRC_PWD=$(WORK_DIR)/$(dir_ca_package)
CA_TARGET ?= g3
STAGING_KERNEL_BUILDDIR=$(KERNEL_SRC)
STAGING_INCDIR=$(D_DIR)/sysroot-destdir/usr/include
EXTRA_CFLAGS += -I$(CA_PLAT_DIR)
JOBS ?=4

#MODULE_EXTRA_CFLAGS = -I${STAGING_INCDIR}/module/ca-plat -I${STAGING_INCDIR}/module/ca-scfg -I${STAGING_INCDIR}/cortina-api
MODULE_EXTRA_CFLAGS = -I${SRC_PWD} -I${STAGING_INCDIR}/module/ca-plat -I${STAGING_INCDIR}/module -I${STAGING_INCDIR}/cortina-api -I${STAGING_INCDIR}/module/ca-scfg
MODULE_EXTRA_CFLAGS += -DRTK_MEM_CORRUPTTION_FIX

sinclude $(LINUX_CONFIG)
ifdef CONFIG_ARCH_CORTINA_G3HGU
ifdef CONFIG_CORTINA_PON_MODE_DUAL
$(info ------ CONFIG_CORTINA_PON_MODE_DUAL ---)
CA_TARGET=g3hgu
endif
ifdef CONFIG_CORTINA_PON_MODE_GPON
$(info ------ CONFIG_CORTINA_PON_MODE_GPON ---)
CA_TARGET=g3hgu_gpon
endif
ifdef CONFIG_CORTINA_PON_MODE_EPON
$(info ------ CONFIG_CORTINA_PON_MODE_EPON ---)
CA_TARGET=g3hgu_epon
endif
$(info ------ ca-network-engine: CA_TARGET = $(CA_TARGET) ---)
endif

MAKEUSR = $(MAKEARCH) PWD=$(SRC_PWD)  -C $(dir_ca_package)
MAKEPACK = $(MAKEUSR) 

.DEFAULT_GOAL := all

module_do_compile: do_configure
	$(MAKEPACK) -j $(JOBS)  KERNEL_SRC=$(KERNEL_SRC) KERNEL_PATH=$(KERNEL_SRC) KERNEL_VERSION=$(KERNEL_VERSION) \
	CA_PLAT_DIR=$(STAGING_INCDIR)/module/ca-plat CA_SCFG_DIR=$(STAGING_INCDIR)/module/ca-scfg EXTRA_CFLAGS='$(MODULE_EXTRA_CFLAGS)' modules 

do_configure:
	$(MAKEUSR)  KERNEL_SRC=$(KERNEL_SRC) $(CA_TARGET)_defconfig

all: 
	@echo "--------- Package : $(dir_ca_package) ---------------"
	$(MAKEARCH) module_do_compile

	mkdir -p ${D}/${includedir}
	$(MAKEUSR)  KERNEL_SRC=${STAGING_KERNEL_BUILDDIR} DEST=${D}/usr/include/module/ca-ne module_headers_install

	$(MAKEUSR)  KERNEL_SRC=${STAGING_KERNEL_BUILDDIR} DEST=${D}/usr/include/cortina-api user_headers_install

#	mkdir -p ${D}/usr/include/iros
	$(MAKEUSR)  KERNEL_SRC=${STAGING_KERNEL_BUILDDIR} DEST=${D}/usr/include/iros iros_headers_install

#	mkdir -p ${D}/usr/include/cli
	$(MAKEUSR)  KERNEL_SRC=${STAGING_KERNEL_BUILDDIR} DEST=${D}/usr/include/cli cli_headers_install

clean:
	$(MAKEUSR)   KERNEL_SRC=${STAGING_KERNEL_BUILDDIR} clean || exit $?; \
	rm -rf $(dir_ca_package)/include/generated
	rm -rf $(dir_ca_package)/include/config

modules_install:
	$(MAKEPACK)  modules_install || exit $?; \

