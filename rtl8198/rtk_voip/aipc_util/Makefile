ifneq ($(YOCTO_BUILD), 1)
-include $(ROOTDIR)/config/.config 
-include $(ROOTDIR)/$(LINUXDIR)/.config

AIPC_EXEC = aipc_util
AIPC_OBJS = aipc_util.o

CFLAGS += -Wall 
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char/aipc/include

all: $(AIPC_EXEC)

$(AIPC_EXEC): $(AIPC_OBJS)
	$(CC) $(LDFLAGS) $(AIPC_OBJS) -o $@ 

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) -Wa,-a,-ad $< > $(@:.o=.lst)
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)

romfs:
	$(ROMFSINST) $(AIPC_EXEC)                        /bin/$(AIPC_EXEC)
ifeq ($(CONFIG_RTK_VOIP_MODULE_BUILD),y)
	$(ROMFSINST) aipc/rc_voip_module                /etc/rc_voip_module
	$(ROMFSINST) aipc/rc_voip_module_boot_dsp       /etc/rc_voip_module_boot_dsp
endif
ifeq ($(CONFIG_MIPS),y)
ifeq ($(CONFIG_AIPC_PLATFORM_0787_5281),y)
	$(ROMFSINST) aipc/rc_boot_dsp-0787               /etc/rc_boot_dsp
	$(ROMFSINST) aipc/appimg-0787-5281.bin           /image/dsp.img
	$(ROMFSINST) aipc/rc_reset_dsp-0787              /etc/rc_reset_dsp
	$(ROMFSINST) aipc/rc_log_dsp                     /etc/rc_log_dsp
else if ($(CONFIG_AIPC_PLATFORM_0822_5281),y)
	$(ROMFSINST) aipc/rc_boot_dsp-0822               /etc/rc_boot_dsp
	$(ROMFSINST) aipc/appimg-0822-5281.bin           /image/dsp.img
else
	$(ROMFSINST) aipc/rc_boot_dsp                    /etc/rc_boot_dsp
	$(ROMFSINST) aipc/appimg.bin                     /image/dsp.img
endif
else
ifeq ($(CONFIG_ARCH_CORTINA),y)
ifeq ($(CONFIG_ARCH_CORTINA_G3HGU),y)
	$(ROMFSINST) aipc/appimg-ca8277.bin              /image/dsp.img
else ifeq ($(CONFIG_ARCH_CORTINA_VENUS),y)
	$(ROMFSINST) aipc/appimg-ca8277b.bin             /image/dsp.img
endif
	$(ROMFSINST) aipc/rc_boot_dsp-ca8277x            /etc/rc_boot_dsp
endif
endif
ifeq ($(CONFIG_RTL8686_IPC_TEST_THREAD),y)
	$(ROMFSINST) aipc/run_all.sh                     /etc/run_all.sh
	$(ROMFSINST) aipc/run_mbox.sh                    /etc/run_mbox.sh
	$(ROMFSINST) aipc/run_ctrl.sh                    /etc/run_ctrl.sh
	$(ROMFSINST) aipc/mbox_2dsp.sh                   /etc/mbox_2dsp.sh
	$(ROMFSINST) aipc/mbox_2cpu.sh                   /etc/mbox_2cpu.sh
	$(ROMFSINST) aipc/ctrl_2dsp.sh                   /etc/ctrl_2dsp.sh
	$(ROMFSINST) aipc/ctrl_2cpu.sh                   /etc/ctrl_2cpu.sh
endif

clean:
	rm -f $(AIPC_EXEC) *.o *.d *.lst

.PHONY: all clean

else
include $(KBUILD_DIR)/.config 
include ${PACKAGE_DIR}/.config

ROMFSINST=cp

AIPC_EXEC = aipc_util
AIPC_OBJS = aipc_util.o

CFLAGS += -Wall 
ifeq ($(CONFIG_ARCH_CORTINA),y)
CFLAGS += -DCONFIG_ARCH_CORTINA=1
endif
ifeq ($(CONFIG_ARCH_CORTINA_G3),y)
CFLAGS += -DCONFIG_ARCH_CORTINA_G3=1
endif
ifeq ($(CONFIG_ARCH_CORTINA_G3HGU),y)
CFLAGS += -DCONFIG_ARCH_CORTINA_G3HGU=1
endif
ifeq ($(CONFIG_ARCH_CORTINA_SATURN_SFU),y)
CFLAGS += -DCONFIG_ARCH_CORTINA_SATURN_SFU=1
endif
ifeq ($(CONFIG_ARCH_CORTINA_VENUS),y)
CFLAGS += -DCONFIG_ARCH_CORTINA_VENUS=1
endif

CFLAGS += -I ${PACKAGE_DIR}/aipc_char/include/

all: $(AIPC_EXEC)

$(AIPC_EXEC): $(AIPC_OBJS)
	$(CC) $(LDFLAGS) $(AIPC_OBJS) -o $@ 

%.o: %.c ${PACKAGE_DIR}/.config
	@echo CFLAGS=$(CFLAGS) TARGET=$(TARGET)
	$(CC) -c -o $@ $(CFLAGS) -Wa,-a,-ad $< > $(@:.o=.lst)
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)

install:
	@echo DEST=$(DEST) TARGET=$(TARGET)
	mkdir -p ${DEST}/etc
	mkdir -p ${DEST}/usr/bin
	$(ROMFSINST) aipc_util                           $(DEST)/usr/bin/aipc_util
ifeq ($(CONFIG_ARCH_CORTINA_VENUS),y)
	$(ROMFSINST) aipc/rc_boot_dsp-cortina-venus      $(DEST)/etc/rc_boot_dsp
else
	$(ROMFSINST) aipc/rc_boot_dsp-cortina            $(DEST)/etc/rc_boot_dsp
endif

install: install_${TARGET}

install_saturn-sfu install_saturn-sfpplus:
	mkdir -p ${DEST}/image
	$(ROMFSINST) aipc/appimg-ca_saturn.bin $(DEST)/image/dsp.img

install_g3 install_g3hgu:
	mkdir -p ${DEST}/image
	$(ROMFSINST) aipc/appimg-ca_g3.bin $(DEST)/image/dsp.img

install_venus:
	mkdir -p ${DEST}/image
	$(ROMFSINST) aipc/appimg-ca_venus.bin $(DEST)/image/dsp.img

clean:
	rm -f $(AIPC_EXEC) *.o *.d *.lst

.PHONY: all clean

endif
