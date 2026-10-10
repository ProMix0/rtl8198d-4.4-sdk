D_DIR = ${ROOTDIR}
JOBS ?=4
includedir:=/usr/include
libdir:=/usr/lib

dir_ca_pk = ${CA_VER}/ca-phy-mdio-access-1.0

D = $(D_DIR)/sysroot-destdir
STAGING_INCDIR=$(D_DIR)/sysroot-destdir/usr/include
S = $(shell pwd)/$(dir_ca_pk)
EXTRA_CFLAGS = -I${STAGING_INCDIR}/module/ca-plat -I${STAGING_INCDIR}/cli

.DEFAULT_GOAL := all
module_do_compile:
	$(MAKEARCH) -j $(JOBS) -C $(dir_ca_pk) KERNEL_PATH=$(KERNEL_SRC)   \
                   KERNEL_VERSION=$(KERNEL_VERSION) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
                   modules

all: module_do_compile

clean:
	$(MAKE) -C $(dir_ca_pk)  clean || exit $?; \

modules_install:
	$(MAKEARCH) -C $(dir_ca_pk)  modules_install || exit $?; \
