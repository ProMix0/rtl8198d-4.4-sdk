obj-m := ca-phy-mdio-access.o

SRC := $(shell pwd)

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) $@

