#
# Include architecture specific build rules.
#

VER = 1.0
PACKAGE = $(VER)-r0/ca-iros-scripts-$(VER)
WORK_DIR = $(shell pwd)/
BUILD_DIR = $(shell pwd)/
S = $(WORK_DIR)/$(PACKAGE)
SYS_D = $(WORK_DIR)/sysroot-destdir
SYSROOT = $(ROOTDIR)/sysroot-destdir
DEST = $(SYS_D)/etc/cortina/iros


all:

install: 
	rm -rf $(SYS_D)
	make -C $(PACKAGE) install DEST=$(DEST)
	cp -arf $(SYS_D)/* $(SYSROOT)
clean:
	rm -rf $(SYS_D)

configure:
	
