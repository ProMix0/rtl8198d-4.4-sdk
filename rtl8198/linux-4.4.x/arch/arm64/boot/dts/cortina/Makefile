ifeq ($(CONFIG_VOIP),y)
dtb-$(CONFIG_ARCH_CORTINA_G3) += ca7774-eng-voip.dtb
dtb-$(CONFIG_ARCH_CORTINA_G3HGU) += ca8279-eng-voip.dtb
else
dtb-$(CONFIG_ARCH_CORTINA_G3) += ca7774-engboard.dtb ca7774-ft2board.dtb
dtb-$(CONFIG_ARCH_CORTINA_G3HGU) += ca8279-engboard.dtb ca8279-ft2board.dtb
dtb-$(CONFIG_ARCH_CORTINA_VENUS) += ca8277-fpga.dtb
endif
 
dtb-$(CONFIG_CORTINA_BOARD_VENUS_ENG) += ca8289-engboard.dtb ca8289-ft2board.dtb

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb
