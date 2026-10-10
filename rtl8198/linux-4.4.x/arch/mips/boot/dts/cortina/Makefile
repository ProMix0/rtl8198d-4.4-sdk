# All DTBs
ifdef CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN
dtb-$(CONFIG_ARCH_CORTINA_SATURN_SFU)			+= titan.dtb
dtb-$(CONFIG_ARCH_CORTINA_SATURN_SFU)			+= titan_epon.dtb
dtb-$(CONFIG_ARCH_CORTINA_SATURN_SFU)			+= titan_gpon.dtb
else
dtb-$(CONFIG_CORTINA_BOARD_SATURN_SFU_ENG)		+= saturn_sfu.dtb
dtb-$(CONFIG_CORTINA_BOARD_SATURN_SFU_ENG)		+= saturn_sfu_c0.dtb
dtb-$(CONFIG_CORTINA_BOARD_SATURN_SFP_PLUS_ENG)		+= saturn_sfpplus.dtb
endif

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
