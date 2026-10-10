dtb-$(CONFIG_ARCH_CORTINA_G3HGU) += ca8279-engboard.dtb
ifdef CONFIG_ARCH_CORTINA_VENUS
dtb-$(CONFIG_CORTINA_BOARD_FPGA) += ca8277-fpga.dtb
dtb-$(CONFIG_CORTINA_BOARD_VENUS_ENG) += ca8277B-engboard.dtb
endif
ifdef CONFIG_ARCH_REALTEK_TAURUS
dtb-$(CONFIG_REALTEK_BOARD_FPGA) += rtl8277C-fpga.dtb
dtb-$(CONFIG_REALTEK_BOARD_TAURUS_ENG) += rtl8277C-engboard.dtb
endif

always          := $(dtb-y)
subdir-y        := $(dts-dirs)
clean-files     := *.dtb

