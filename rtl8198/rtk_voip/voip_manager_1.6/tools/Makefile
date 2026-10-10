
all: 
	make -C misc 

romfs:
	make -C misc romfs  

ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL),y)
	$(ROMFSINST) slic/fw_dxs-2.1.3.bin              /image/fw_dxs-2.1.3.bin
	$(ROMFSINST) slic/R600.BIN                		/image/R600.BIN	
endif
	
clean:
	make -C misc clean  

