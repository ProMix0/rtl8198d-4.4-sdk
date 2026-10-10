obj-y	:=	drv_dxs_alm_aclm.o drv_dxs_access.o drv_dxs_mbx.o \
        drv_dxs_outbox.o drv_dxs_fifo.o drv_dxs_init.o drv_dxs_irq.o \
        drv_dxs_bbd.o drv_dxs_gpio.o drv_dxs_alm.o drv_dxs_alm_lt.o \
        drv_dxs_ioctl.o drv_dxs_dwld.o drv_dxs_cid.o drv_dxs_cid_sm.o \
        drv_dxs_pcm.o drv_dxs_tg.o drv_dxs_dtmf.o drv_dxs_dtmf_sm.o \
        drv_dxs_utd.o drv_dxs_event.o drv_dxs_debug.o drv_dxs_linux.o drv_hal.o \
        lib/lib_bbd/src/lib_bbd.o drv_dxs_rtk_api.o


#EXTRA_CFLAGS	+= -I
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/src/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/include/linux/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/spi/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
EXTRA_CFLAGS += -I$(DIR_VOIP)/include/
EXTRA_CFLAGS += -I$(LINUXDIR)/include
EXTRA_CFLAGS += -DLINUX -DRW_MULTI_THREAD -D_REENTRANT -D_THREAD_SAFE -D_POSIX_SOURCE -D_GNU_SOURCE -DHAVE_CONFIG_H -D__KERNEL__
EXTRA_CFLAGS += -DREALTEK_PATCH_FOR_INTEL
include $(DIR_VOIP)/rules.mak