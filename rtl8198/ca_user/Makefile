ifndef ROOTDIR
ROOTDIR=..
endif

PATH := $(ROOTDIR)/tools/usr/bin:$(PATH)
HOST_CPU_NUM ?= 4
JOBS=$(HOST_CPU_NUM)
export JOBS

KMOD_DIR = $(shell pwd)
MAKEARCH ?= make

sinclude $(LINUX_CONFIG)
########################
ifdef CONFIG_RTK_DEV_AP
else
ifdef CONFIG_RTK_USE_CA_IROS
dir_c_usr = tcl
dir_c_usr += libnl
dir_c_usr += ncurses
dir_c_usr += readline
dir_c_usr += tclreadline
dir_c_usr += ca-iros
dir_c_usr += ca-iros-scripts
dir_c_usr_kmod += ca-iros
endif
endif
## This need libpcap library, etc.
## So need to some work to compile it.
ifdef CORTINA_OMCI
dir_c_usr += ca-omci
dir_c_usr += ca-oam
dir_c_usr += ca-app
endif


all:
	@echo "xxxxxxxxxx  ca_packages  xxxxxxxxxxxxxxxxxxx"
	for i in $(dir_c_usr); do \
		$(MAKEARCH) -C $$i || exit $?; \
		$(MAKEARCH) -C $$i install || exit $?; \
	done

clean:
	for i in $(dir_c_usr); do \
		$(MAKEARCH) -C $$i clean || exit $?; \
	done

modules:
	for i in $(dir_c_usr_kmod); do \
		$(MAKEARCH) -C $$i modules || exit $?; \
	done

modules_install:
	for i in $(dir_c_usr_kmod); do \
		$(MAKEARCH) -C $$i INSTALL_MOD_STRIP=1 modules_install || exit $?; \
	done
