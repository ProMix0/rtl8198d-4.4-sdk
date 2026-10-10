ifndef ROOTDIR
ROOTDIR=..
endif
KMOD_DIR = $(shell pwd)
MAKEARCH ?= make
########################
CA_MAIN_VER = 1.0
CA_VER = $(CA_MAIN_VER)-r0
export CA_VER CA_MAIN_VER
#######################
CFLAGS += -O2 -pipe 
export CFLAGS
dir_c =
dir_c += ca-plat
dir_c += ca-scfg
### ca-pon-ma depends on ca-plat ca-scfg.
sinclude $(LINUX_CONFIG)
CA_TARGET =


ifdef CONFIG_ARCH_CORTINA_VENUS
CA_TARGET = venus
endif

ifdef CONFIG_ARCH_REALTEK_TAURUS
CA_TARGET = 77c
endif


ifdef CONFIG_ARCH_CORTINA_G3HGU
CA_TARGET = g3hgu
endif

ifdef CONFIG_ARCH_RTL8198F
CA_TARGET = g3lite
endif

dir_c += ca-network-engine
dir_c += ca-cli
dir_c += serdes-data
dir_c += ca-phy-mdio-access

HOST_CPU_NUM ?= 4
JOBS=$(HOST_CPU_NUM)
export JOBS


ifdef CONFIG_ARCH_CORTINA
$(info ----------------[ca_packages] CA_TARGET=$(CA_TARGET) --------------------------)
ifdef CONFIG_RTK_CA_PACKAGES_SKIP
dir_c =
endif
all:
	@echo "xxxxxxxxxx  ca_packages  xxxxxxxxxxxxxxxxxxx"
	for i in $(dir_c); do \
		$(MAKEARCH) -C $$i -j $(USER_JOB_NUM) || exit $?; \
	done

%_ca:
	$(MAKEARCH) -C $(@:_ca=) -j $(USER_JOB_NUM) 

clean:
	for i in $(dir_c); do \
		$(MAKEARCH) -C $$i clean ; \
	done

modules_install:
	for i in $(dir_c); do \
		$(MAKEARCH) -C $$i  INSTALL_MOD_STRIP=1 modules_install || exit $?; \
	done
else
all:
%_ca:
clean:
modules_install:
endif
