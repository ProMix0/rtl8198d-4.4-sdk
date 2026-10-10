D_DIR = ${ROOTDIR}

dir_ca_package = ${CA_VER}/serdes-data-1.0

CA_TARGET ?= g3
D = $(D_DIR)/sysroot-destdir/etc/serdes/

sinclude $(LINUX_CONFIG)
ifdef CONFIG_ARCH_CORTINA_G3HGU
CA_TARGET = g3hgu
$(info CA_TARGET = $(CA_TARGET))
endif #end for CONFIG_ARCH_CORTINA_G3HGU

ifdef CONFIG_ARCH_CORTINA_VENUS
CA_TARGET = venus
$(info CA_TARGET = $(CA_TARGET))
endif

ifdef CONFIG_ARCH_CORTINA_G3
CA_TARGET = g3
$(info CA_TARGET = $(CA_TARGET))
endif

ifdef  CONFIG_ARCH_CORTINA_G3LITE
CA_TARGET = g3lite
$(info CA_TARGET = $(CA_TARGET))
endif


MAKEPACK = $(MAKEARCH) 

.DEFAULT_GOAL := all

install:
	$(MAKEPACK) -C $(dir_ca_package) DEST=${D} target=${CA_TARGET} install || exit $?; 

all: 
	@echo "--------- Package : $(dir_ca_package) ---------------"
	$(MAKEPACK) install;

clean:

modules_install:

