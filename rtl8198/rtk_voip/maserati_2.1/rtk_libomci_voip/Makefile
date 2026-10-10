# Path for kernel module
# Path for user space library
LIBOMCIVOIP_PATH = libomcivoip

EXTRA_CFLAGS += -include $(KDIR)/include/linux/autoconf.h


all:
ifdef CONFIG_RTK_OMCI_V1 
	$(MAKE) -C $(LIBOMCIVOIP_PATH) clean
	$(MAKE) -C $(LIBOMCIVOIP_PATH) all
endif

clean:
ifdef CONFIG_RTK_OMCI_V1
	$(MAKE) -C $(LIBOMCIVOIP_PATH) clean
endif

romfs:
ifdef CONFIG_RTK_OMCI_V1
	$(MAKE) -C $(LIBOMCIVOIP_PATH) romfs
endif
