obj-y	:=	drv_tapi_dial.o drv_tapi_event.o \
        drv_tapi_debug.o drv_tapi_ioctl.o drv_tapi_meter.o \
        drv_tapi_misc.o drv_tapi_opcontrol.o drv_tapi_ppd.o \
        drv_tapi_pcm.o drv_tapi_ring.o drv_tapi_tone.o drv_tapi_cid.o \
        drv_tapi_voice.o drv_tapi_init.o drv_tapi_stream.o \
        drv_tapi_polling.o drv_tapi_fxo.o drv_tapi_stat.o \
        drv_tapi_srtp.o lib/lib_fifo.o lib/lib_bufferpool.o \
		drv_tapi_rtk_api.o drv_tapi_rtk_linux.o drv_tapi_dev_io.o
        #drv_tapi_linux.o drv_tapi_dev_io.o

#EXTRA_CFLAGS	+= -I
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/src/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_dxs-1.3.0.11/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/src/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/lib_ifxos-1.6.9/include/linux/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/src/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/intel/drv_tapi-4.17.0.7/include/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/spi/
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
EXTRA_CFLAGS += -I$(DIR_VOIP)/include/
EXTRA_CFLAGS += -I$(LINUXDIR)/include
EXTRA_CFLAGS += -DLINUX -DTAPI_HAVE_TIMERS	-DHAVE_CONFIG_H -D__KERNEL__ -DRW_MULTI_THREAD -D_REENTRANT -D_THREAD_SAFE -D_POSIX_SOURCE -D_GNU_SOURCE -Wall -Wimplicit -Wreturn-type -Wunused -Wswitch -Wcomment -Wparentheses
EXTRA_CFLAGS += -DREALTEK_PATCH_FOR_INTEL

include $(DIR_VOIP)/rules.mak
