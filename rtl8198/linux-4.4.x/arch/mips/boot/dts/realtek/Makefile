dtb-$(CONFIG_RTK_SOC_RTL8198D) += rtl8198d_engboard.dtb
dtb-$(CONFIG_RTK_SOC_RTL9607C) += rtl9607c_engboard.dtb

obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S

$(shell cd arch/mips/boot/dts/realtek; \
	ln -f -s memory-$(CONFIG_SOC_MEM_SIZE).dts memory/memory.dts; \
	ln -f -s memory/memory.dts memory.dts)
