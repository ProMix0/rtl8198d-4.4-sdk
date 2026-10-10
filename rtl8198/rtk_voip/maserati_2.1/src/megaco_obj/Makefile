#
#	Makefile -- Build instructions for user2 level apps
#


all: 

romfs: 
ifeq ($(CONFIG_ARM64),y)
	#$(ROMFSINST) bin/mgapp_arm /bin/mgapp
	ln -rsf bin/mgapp_arm bin/mgapp
else
	ln -rsf bin/mgapp_mips bin/mgapp
	#$(ROMFSINST) bin/mgapp_mips /bin/mgapp
endif
	$(ROMFSINST) bin/mgapp /bin/mgapp
clean:
	rm -rf bin/mgapp





