# pcm-mod-objs-yy will not be built

voip_drivers-y    :=
pcm-mod-objs-y    :=

#obj-$(CONFIG_RTK_VOIP_DRIVERS)  += voip_drivers.o

#CONFIG_VOIP_SDK=y

# con
pcm-mod-objs-y += voip_drivers/con_main.o
pcm-mod-objs-y += voip_drivers/con_feature.o
pcm-mod-objs-y += voip_drivers/con_register.o
pcm-mod-objs-y += voip_drivers/con_bind.o
pcm-mod-objs-y += voip_drivers/con_mux.o
pcm-mod-objs-y += voip_drivers/con_event.o
pcm-mod-objs-y += voip_drivers/con_pluse_dial.o
pcm-mod-objs-y += voip_drivers/con_ring.o
pcm-mod-objs-$(CONFIG_RTK_VOIP_QOS) += voip_drivers/con_sup_qos.o
pcm-mod-objs-$(CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF) += voip_drivers/con_bus_fifo.o voip_drivers/con_bus_handler.o voip_drivers/con_lec_buf.o

pcm-mod-objs-y += voip_drivers/con_polling.o
ifneq ($(CONFIG_RTK_VOIP_CON_POLLING_PROCESS),y)
pcm-mod-objs-y += voip_drivers/con_polling_snd.o
else
pcm-mod-objs-y += voip_drivers/con_polling_process.o
endif
#pcm-mod-objs-$(CONFIG_RTK_VOIP_CON_POLLING_PROCESS) += voip_drivers/con_polling_process.o
#pcm-mod-objs-y += $(if $(CONFIG_RTK_VOIP_CON_POLLING_PROCESS),voip_drivers/con_polling_process.o,voip_drivers/con_polling_snd.o)

pcm-mod-objs-y += voip_drivers/con_entry.o

pcm-mod-objs-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP)  += voip_drivers/con_ipc_mirror_dsp.o voip_drivers/con_ipc_rpc_dsp.o
pcm-mod-objs-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += voip_drivers/con_ipc_mirror_host.o voip_drivers/con_ipc_rpc_host.o voip_drivers/con_ipc_help_host.o

pcm-mod-objs-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_drivers/con_defer_init.o

#pcm-mod-objs-y += con_16k_sample.o

# bus
pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_IIS) += voip_drivers/bus_iis_interface.o
ifneq "$(CONFIG_RTK_VOIP_PLATFORM_7742)" "y"
pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_PCM) += voip_drivers/bus_pcm_interface.o voip_drivers/bus_pcm_law.o
else
pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_PCM) += voip_drivers/bus_pcm_law.o voip_drivers/ca_snd_audio_wrapper.o voip_drivers/ca_bus_pcm_interface.o voip_drivers/ca_snd_audio.o
EXTRA_CFLAGS += -DHOST_LINUX
endif
pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_PCM_LAW) += voip_drivers/bus_pcm_law.o

#pcm-mod-objs-y += bus_pcm_interface.o
# snd
#pcm-mod-objs-$(CONFIG_RTK_VOIP_SLIC_RELAY) += voip_drivers/snd_slic_relay.o
pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_VIRTUAL_DAA)   += voip_drivers/snd_virtual_daa.o

pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC)    += voip_drivers/snd_mux_slic.o voip_drivers/snd_help.o
#pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_FXO)   += voip_drivers/snd_mux_daa.o
pcm-mod-objs-y   += voip_drivers/snd_mux_daa.o

pcm-mod-objs-$(CONFIG_RTK_VOIP_DRIVERS_SI3050)   += voip_drivers/snd_proslic_daa.o voip_drivers/snd_si3050.o

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)" "y"
#obj-$(CONFIG_RTK_VOIP_DRIVERS_SILAB_SLIC_LINETEST) += proslic_mlt_api-v2.4.3/
include $(DIR_VOIP)/voip_drivers/proslic_mlt_api-v4.0.0/src/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SILAB_SLIC_LINETEST) += $(proslic_mlt-y)
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)" "y"
include $(DIR_VOIP)/voip_drivers/proslic_api/module.mak
voip_drivers-y += $(proslic_api-y)
voip_drivers-y += voip_drivers/snd_pin_reset.o
voip_drivers-y += voip_drivers/snd_pin_cs.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x) += voip_drivers/snd_si3228x.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) += voip_drivers/snd_si3226x.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226)  += voip_drivers/snd_si3226.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) += voip_drivers/snd_si3217x.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) += voip_drivers/snd_si3218x.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x) += voip_drivers/snd_si3219x.o
voip_drivers-y += voip_drivers/snd_proslic_base.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK)" "y"
#ZARLINK_LIB     = api_lib-2.13.1
#ZARLINK_LIB     = api_lib-2.16.1
#ZARLINK_LIB     = api_lib-2.22.0
#ZARLINK_LIB     = api_lib-2.24.0
ZARLINK_LIB      = api_lib-2.27.0
#ZARLINK_PROFILE   = profiles_HG250
ZARLINK_PROFILE  = profiles
export ZARLINK_LIB
export ZARLINK_PROFILE

include $(DIR_VOIP)/voip_drivers/zarlink/module.mak
voip_drivers-y += $(zarlink-y)
voip_drivers-y += voip_drivers/snd_pin_reset.o
voip_drivers-y += voip_drivers/snd_pin_cs.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221) += voip_drivers/snd_le88221.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88111) += voip_drivers/snd_le88111.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116) += voip_drivers/snd_le89116.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89316) += voip_drivers/snd_le89316.o voip_drivers/snd_zarlink_daa.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662)  += voip_drivers/snd_le9662.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632)  += voip_drivers/snd_le9632.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622)  += voip_drivers/snd_le9622.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652)  += voip_drivers/snd_le9652.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661)  += voip_drivers/snd_le9661.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641)  += voip_drivers/snd_le9641.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643)  += voip_drivers/snd_le9643.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653)  += voip_drivers/snd_le9653.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651)  += voip_drivers/snd_le9651.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601) += voip_drivers/snd_le88601.o
voip_drivers-y += voip_drivers/snd_zarlink_fxs.o
voip_drivers-y += voip_drivers/snd_zarlink_common.o
endif

voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_CODEC_ALC5621) += voip_drivers/snd_alc5621.o

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI)" "y"
include $(DIR_VOIP)/voip_drivers/microsemi/module.mak
voip_drivers-y += $(microsemi-y)
voip_drivers-y += voip_drivers/snd_pin_reset.o
voip_drivers-y += voip_drivers/snd_pin_cs.o
voip_drivers-y += voip_drivers/snd_microsemi.o
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL)" "y"
include $(DIR_VOIP)/voip_drivers/intel/module.mak
voip_drivers-y += $(intel-y)
voip_drivers-y += voip_drivers/snd_intel_dxs.o
voip_drivers-y += voip_drivers/snd_intel_fxs.o
voip_drivers-y += voip_drivers/snd_pin_reset.o
voip_drivers-y += voip_drivers/snd_pin_cs.o
endif

voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_MIRROR_SLIC) += voip_drivers/snd_mirror_slic.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_MIRROR_DAA) += voip_drivers/snd_mirror_daa.o

voip_drivers-$(CONFIG_RTK_VOIP_IPC_ARCH_FULLY_OFFLOAD) += voip_drivers/snd_shadow_slic.o voip_drivers/snd_shadow_daa.o

ifneq ($(CONFIG_RTK_VOIP_PLATFORM_7742),y)
ifneq "$(CONFIG_RTK_VOIP_COPROCESS_DSP_IS_DSP)" "y"
voip_drivers-y += voip_drivers/snd_interface.o
endif
endif

# dsp
ifneq "$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST)" "y"
pcm-mod-objs-$(CONFIG_REALTEK_VOIP) += voip_drivers/dsp_rtk_main.o voip_drivers/dsp_rtk_handler.o voip_drivers/dsp_rtk_isr.o voip_drivers/dsp_rtk_mux.o
pcm-mod-objs-$(CONFIG_REALTEK_VOIP) += voip_drivers/dsp_rtk_caller.o
pcm-mod-objs-$(CONFIG_AUDIOCODES_VOIP)  += voip_drivers/dsp_ac_main.o voip_drivers/dsp_ac_handler.o voip_drivers/dsp_ac_isr.o voip_drivers/dsp_ac_mux.o
pcm-mod-objs-$(CONFIG_AUDIOCODES_VOIP)  += voip_drivers/radiax_save.o
pcm-mod-objs-y += voip_drivers/dsp_define.o
pcm-mod-objs-$(CONFIG_RTK_VOIP_T38) += voip_drivers/t38_handler.o
endif
pcm-mod-objs-$(CONFIG_REALTEK_VOIP) += voip_drivers/dsp_rtk_global.o
pcm-mod-objs-$(CONFIG_AUDIOCODES_VOIP) += voip_drivers/dsp_ac_global.o

pcm-mod-objs-$(CONFIG_RTK_VOIP_T38) += voip_drivers/t38_handler_data.o

# ioc
voip_drivers-y += voip_drivers/ioc_core.o
voip_drivers-$(CONFIG_RTK_VOIP_IOC_LED) += voip_drivers/ioc_led.o
voip_drivers-$(CONFIG_RTK_VOIP_IOC_SLIC_RELAY) += voip_drivers/ioc_slic_relay.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653_IO) += voip_drivers/snd_zarlink_ioc_op.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601_IO) += voip_drivers/snd_zarlink_ioc_op.o

# common driver
pcm-mod-objs-$(CONFIG_RTK_VOIP_LED)	+= voip_drivers/led.o
voip_drivers-$(CONFIG_SERIAL_SC16IS7X0)               += voip_drivers/i2c.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_CODEC_WM8510)  += voip_drivers/i2c.o
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_CODEC_ALC5621) += voip_drivers/i2c.o

ifeq "$(CONFIG_RTK_VOIP_DRIVERS)" "y"
include $(DIR_VOIP)/voip_drivers/gpio/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS) += $(voip_gpio-y)
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC)" "y"
include $(DIR_VOIP)/voip_drivers/spi/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC) += $(voip_spi-y)
endif
ifeq "$(CONFIG_RTK_VOIP_DECT_SPI_SUPPORT)" "y"
include $(DIR_VOIP)/voip_drivers/spi/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_DECT_SPI_SUPPORT) += $(voip_spi-y)
endif
ifeq "$(CONFIG_RTK_VOIP_DECT_SPI_SUPPORT)" "y"
include $(DIR_VOIP)/voip_drivers/spi/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_HARDWARE_SPI) += $(voip_spi-y)
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_IP_PHONE)" "y"
include $(DIR_VOIP)/voip_drivers/iphone/module.mak
voip_drivers-$(CONFIG_RTK_VOIP_DRIVERS_IP_PHONE) += $(iphone-y)
endif

# timer
voip_drivers-y += voip_drivers/os_timer_core.o voip_drivers/os_timer_linux.o

#g168 debut test input
#pcm-mod-objs-y   += ec128_testdatas.o

# slic cmd
voip_drivers-y += voip_drivers/snd_slic_cmd.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_MICROSEMI
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -Iarch/rlx/bsp
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/basic_op
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/util
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/v152
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/spi
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/gpio
OBJS_CFLAGS += -I$(DIR_LINUX)/drivers/soc/realtek/

ifeq "$(CONFIG_AUDIOCODES_VOIP)" "y"
OBJS_CFLAGS += -Iacmw_lx/ac_drv
OBJS_CFLAGS += -DAUDIOCODES_VOIP
OBJS_CFLAGS += -DACMW_LX
OBJS_CFLAGS += -DACL_BYPASS
OBJS_CFLAGS += -DACL_RELEASE
OBJS_CFLAGS += -DAUDIOCODES_VOIP_SIP
endif

ifeq ($(CONFIG_RTK_VOIP_T38),y)
ifeq ($(CONFIG_VOIP_SDK),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
else
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/t38/rly_src
endif
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI)" "y"
MICROSEMI_LIB = microsemi/microsemi_voicepath_api/api_lib
export MICROSEMI_LIB
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/common/
#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp580_api/
#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp790_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp880_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp890_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_voicepath_api/arch/rtl89xxb/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_voicepath_api/profiles/
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_verivoice_api/includes/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_verivoice_api/vp880_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_verivoice_api/vp886_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/microsemi/microsemi_verivoice_api/vp890_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/includes/linetest/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/$(MICROSEMI_LIB)/vp886_api/linetest/
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL)" "y"
INTEL_DRV			= intel/drv_dxs-1.3.0.11
INTEL_DRV_SRC		= $(INTEL_DRV)/src
INTEL_DRV_SRCINC 	= $(INTEL_DRV)/src/include
INTEL_DRV_INC 		= $(INTEL_DRV)/include
INTEL_TAPI			= intel/drv_tapi-4.17.0.7
INTEL_TAPI_SRC		= $(INTEL_TAPI)/src
INTEL_TAPI_SRCINC	= $(INTEL_TAPI)/src/include
INTEL_TAPI_INC		= $(INTEL_TAPI)/include
INTEL_IFXOS			= intel/lib_ifxos-1.6.9
INTEL_IFXOS_SRC		= $(INTEL_IFXOS)/src
INTEL_IFXOS_SRCINC	= $(INTEL_IFXOS)/src/include
INTEL_IFXOS_INC		= $(INTEL_IFXOS)/include
export INTEL_DRV
export INTEL_TAPI
export INTEL_IFXOS
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_DRV)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_DRV_SRC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_DRV_SRCINC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_DRV_INC)

OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_TAPI)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_TAPI_SRC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_TAPI_SRCINC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_TAPI_INC)

OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_IFXOS)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_IFXOS_SRC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_IFXOS_SRCINC)
OBJS_CFLAGS	+= -I$(DIR_VOIP)/voip_drivers/$(INTEL_IFXOS_INC)

OBJS_CFLAGS += -Iinclude/linux/
OBJS_CFLAGS += -D__KERNEL__ -D__LINUX__ -DLINUX -DRW_MULTI_THREAD -D_REENTRANT -D_THREAD_SAFE -D_POSIX_SOURCE -D_GNU_SOURCE -DHAVE_CONFIG_H
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK)" "y"
#ZARLINK_LIB   	 = api_lib-2.13.1
#ZARLINK_LIB   	 = api_lib-2.16.1
#ZARLINK_LIB   	 = api_lib-2.22.0
#ZARLINK_LIB   	 = api_lib-2.24.0
ZARLINK_LIB   	 = api_lib-2.27.0
#ZARLINK_PROFILE   = profiles_HG250
ZARLINK_PROFILE  = profiles
export ZARLINK_LIB
export ZARLINK_PROFILE
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/includes/
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/include/
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_api/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/linetest/vp886_lt/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
endif
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/$(ZARLINK_PROFILE)/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/arch/rtl89xxb/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/common/
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve880/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp880_api/
endif
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve886/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp886_api/
endif
ifeq "$(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)" "y"
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/ve890/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/zarlink/$(ZARLINK_LIB)/vp890_api/
endif
endif

ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)" "y"
OBJS_CFLAGS += -DREALTEK_PATCH_FOR_SILAB
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/config_inc/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/inc/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_api/custom/
OBJS_CFLAGS += -DPROSLIC_LINUX_KERNEL
OBJS_CFLAGS += -DPROSLICVERSION=8
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_SILAB_SLIC_LINETEST)" "y"
#OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_mlt_api-v2.4.3/inc
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/proslic_mlt_api-v3.0.0/inc
endif
endif

ifeq "$(CONFIG_VOIP_COP3_PROFILE)" "y"
OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
OBJS_CFLAGS += -DFEATURE_COP3_PCMISR
OBJS_CFLAGS += -DFEATURE_COP3_PCM_RX
OBJS_CFLAGS += -DFEATURE_COP3_LEC
#OBJS_CFLAGS += -DFEATURE_COP3_PCM_HANDLER
#OBJS_CFLAGS += -DFEATURE_COP3_DTMFDEC
endif

ifeq ($(CONFIG_RTK_VOIP_PACKAGE_865X),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8651)$(CONFIG_RTK_VOIP_DRIVERS_PCM865xC),y)
OBJS_CFLAGS += -I../../drivers/net/re865x/rtl865x
endif
endif

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -DVC6
endif
endif

ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
OBJS_CFLAGS += -DVC6
endif

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

# rock: add module name for PRINT
#OBJS_CFLAGS += -DMODULE_NAME=\"DRV\"

#voip_drivers-objs := $(pcm-mod-objs-y)
voip_drivers-y += $(pcm-mod-objs-y)

#include $(DIR_VOIP)/rules.mak

CFLAGS_ca_bus_pcm_interface.o := $(OBJS_CFLAGS) -DHOST_LINUX
CFLAGS_ca_snd_audio.o := $(OBJS_CFLAGS) -DHOST_LINUX
CFLAGS_ca_snd_audio_wrapper.o := $(OBJS_CFLAGS) -DHOST_LINUX
CFLAGS_con_main.o := $(OBJS_CFLAGS)
CFLAGS_con_feature.o := $(OBJS_CFLAGS)
CFLAGS_con_register.o := $(OBJS_CFLAGS)
CFLAGS_con_bind.o := $(OBJS_CFLAGS)
CFLAGS_con_mux.o := $(OBJS_CFLAGS)
CFLAGS_con_event.o := $(OBJS_CFLAGS)
CFLAGS_con_pluse_dial.o := $(OBJS_CFLAGS)
CFLAGS_con_ring.o := $(OBJS_CFLAGS)
CFLAGS_con_sup_qos.o := $(OBJS_CFLAGS)
CFLAGS_con_bus_fifo.o := $(OBJS_CFLAGS)
CFLAGS_con_bus_handler.o := $(OBJS_CFLAGS)
CFLAGS_con_lec_buf.o := $(OBJS_CFLAGS)
CFLAGS_con_polling.o := $(OBJS_CFLAGS)
CFLAGS_con_polling_snd.o := $(OBJS_CFLAGS)
CFLAGS_con_polling_process.o := $(OBJS_CFLAGS)
CFLAGS_con_entry.o := $(OBJS_CFLAGS)
CFLAGS_con_ipc_mirror_dsp.o := $(OBJS_CFLAGS)
CFLAGS_con_ipc_rpc_dsp.o := $(OBJS_CFLAGS)
CFLAGS_con_ipc_mirror_host.o := $(OBJS_CFLAGS)
CFLAGS_con_ipc_rpc_host.o := $(OBJS_CFLAGS)
CFLAGS_con_ipc_help_host.o := $(OBJS_CFLAGS)
CFLAGS_con_defer_init.o := $(OBJS_CFLAGS)
CFLAGS_con_16k_sample.o := $(OBJS_CFLAGS)
CFLAGS_bus_iis_interface.o := $(OBJS_CFLAGS)
CFLAGS_bus_pcm_interface.o := $(OBJS_CFLAGS)
CFLAGS_bus_pcm_law.o := $(OBJS_CFLAGS)
CFLAGS_bus_pcm_law.o := $(OBJS_CFLAGS)
CFLAGS_bus_pcm_interface.o := $(OBJS_CFLAGS)
CFLAGS_snd_slic_relay.o := $(OBJS_CFLAGS)
CFLAGS_snd_virtual_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_mux_slic.o := $(OBJS_CFLAGS)
CFLAGS_snd_help.o := $(OBJS_CFLAGS)
CFLAGS_snd_mux_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_mux_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_proslic_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3050.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_cs.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3228x.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3226.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3217x.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3218x.o := $(OBJS_CFLAGS)
CFLAGS_snd_si3219x.o := $(OBJS_CFLAGS)
CFLAGS_snd_proslic_base.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_reset.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_cs.o := $(OBJS_CFLAGS)
CFLAGS_snd_le88221.o := $(OBJS_CFLAGS)
CFLAGS_snd_le88111.o := $(OBJS_CFLAGS)
CFLAGS_snd_le89116.o := $(OBJS_CFLAGS)
CFLAGS_snd_le89316.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9662.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9632.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9622.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9652.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9661.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9641.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9643.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9653.o := $(OBJS_CFLAGS)
CFLAGS_snd_le9651.o := $(OBJS_CFLAGS)
CFLAGS_snd_le88601.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_fxs.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_common.o := $(OBJS_CFLAGS)
CFLAGS_snd_alc5621.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_reset.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_cs.o := $(OBJS_CFLAGS)
CFLAGS_snd_intel_dxs.o := $(OBJS_CFLAGS)
CFLAGS_snd_intel_fxs.o := $(OBJS_CFLAGS)
CFLAGS_snd_pin_reset.o := $(OBJS_CFLAGS)
CFLAGS_snd_mirror_slic.o := $(OBJS_CFLAGS)
CFLAGS_snd_mirror_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_shadow_slic.o := $(OBJS_CFLAGS)
CFLAGS_snd_shadow_daa.o := $(OBJS_CFLAGS)
CFLAGS_snd_interface.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_main.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_handler.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_isr.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_mux.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_caller.o := $(OBJS_CFLAGS)
CFLAGS_dsp_ac_main.o := $(OBJS_CFLAGS)
CFLAGS_dsp_ac_handler.o := $(OBJS_CFLAGS)
CFLAGS_dsp_ac_isr.o := $(OBJS_CFLAGS)
CFLAGS_dsp_ac_mux.o := $(OBJS_CFLAGS)
CFLAGS_radiax_save.o := $(OBJS_CFLAGS)
CFLAGS_dsp_define.o := $(OBJS_CFLAGS)
CFLAGS_t38_handler.o := $(OBJS_CFLAGS)
CFLAGS_dsp_rtk_global.o := $(OBJS_CFLAGS)
CFLAGS_dsp_ac_global.o := $(OBJS_CFLAGS)
CFLAGS_t38_handler_data.o := $(OBJS_CFLAGS)
CFLAGS_ioc_core.o := $(OBJS_CFLAGS)
CFLAGS_ioc_led.o := $(OBJS_CFLAGS)
CFLAGS_ioc_slic_relay.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_snd_zarlink_ioc_op.o := $(OBJS_CFLAGS)
CFLAGS_led.o := $(OBJS_CFLAGS)
CFLAGS_i2c.o := $(OBJS_CFLAGS)
CFLAGS_os_timer_core.o := $(OBJS_CFLAGS)
CFLAGS_os_timer_linux.o := $(OBJS_CFLAGS)
CFLAGS_ec128_testdatas.o := $(OBJS_CFLAGS)
CFLAGS_snd_slic_cmd.o := $(OBJS_CFLAGS)
