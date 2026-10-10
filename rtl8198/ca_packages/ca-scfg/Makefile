D_DIR = ${ROOTDIR}
includedir:=/usr/include
libdir:=/usr/lib

dir_ca_package = ${CA_VER}/ca-scfg-1.0

CA_R19=${USE_CA_07190}
ifeq ($(CA_R19), y)
dir_ca_package = 1.0-r19/ca-scfg-1.0
endif

D = $(D_DIR)/sysroot-destdir
CA_TARGET ?= g3

CA_PLAT_DIR=$(D_DIR)/sysroot-destdir/usr/include/module/ca-plat
STAGING_INCDIR=$(D_DIR)/sysroot-destdir/usr/include
export CA_PLAT_DIR
EXTRA_CFLAGS += -I$(CA_PLAT_DIR) -DRTK_MEM_CORRUPTTION_FIX

sinclude $(LINUX_CONFIG)
ifdef CONFIG_ARCH_CORTINA_G3HGU
ifdef CONFIG_CORTINA_PON_MODE_DUAL
$(info ------ CONFIG_CORTINA_PON_MODE_DUAL ---)
CA_TARGET = g3hgu
endif
ifdef CONFIG_CORTINA_PON_MODE_GPON
$(info ------ CONFIG_CORTINA_PON_MODE_GPON ---)
CA_TARGET = g3hgu_gpon
endif
ifdef CONFIG_CORTINA_PON_MODE_EPON
$(info ------ CONFIG_CORTINA_PON_MODE_EPON ---)
CA_TARGET = g3hgu_epon
endif
$(info CA_TARGET = $(CA_TARGET))
endif #end for CONFIG_ARCH_CORTINA_G3HGU

MAKEPACK = $(MAKEARCH) 

.DEFAULT_GOAL := all


configure:
	$(MAKEPACK) -C $(dir_ca_package) KERNEL_PATH=$(KERNEL_SRC)  configure TARGET=${CA_TARGET}

module_do_compile: configure
	$(MAKEPACK) -C $(dir_ca_package) KERNEL_PATH=$(KERNEL_SRC)   \
                   KERNEL_VERSION=$(KERNEL_VERSION)    \
                   EXTRA_CFLAGS='$(EXTRA_CFLAGS)'


all: 
	@echo "--------- Package : $(dir_ca_package) ---------------"
	$(MAKEARCH) module_do_compile
#	$(MAKEPACK) -C $(dir_ca_package)  all || exit $?
	$(MAKEPACK) -C $(dir_ca_package) module_headers_install DEST=${D}${includedir}/module/ca-scfg
	$(MAKEPACK) -C $(dir_ca_package) user_headers_install DEST=${D}${includedir}/ca-scfg
	$(MAKEPACK) -C $(dir_ca_package) iros_headers_install DEST=${D}${includedir}/iros
	$(MAKEPACK) -C $(dir_ca_package) config_install TARGET=${CA_TARGET} DEST=${D}/etc

   

clean:
	$(MAKEPACK) -C $(dir_ca_package) clean || exit $?; \

modules_install:
	$(MAKEPACK) -C $(dir_ca_package) modules_install || exit $?; \

