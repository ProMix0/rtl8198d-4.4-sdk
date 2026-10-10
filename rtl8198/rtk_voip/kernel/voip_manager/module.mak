voip_manager-y := voip_manager/voip_mgr.o
voip_manager-y += voip_manager/voip_mgr_netfilter.o
voip_manager-y += voip_manager/voip_mgr_do_protocol.o
voip_manager-y += voip_manager/voip_mgr_do_dsp.o
voip_manager-y += voip_manager/voip_mgr_do_driver.o
voip_manager-y += voip_manager/voip_mgr_do_misc.o
voip_manager-y += voip_manager/voip_mgr_do_ipphone.o
voip_manager-y += voip_manager/voip_mgr_do_debug.o

voip_manager-y += voip_manager/voip_mgr_transfer_id.o
voip_manager-y += voip_manager/voip_mgr_events.o
voip_manager-$(CONFIG_RTK_VOIP_LOG_IOCTL) += voip_manager/voip_mgr_log.o

voip_manager-$(CONFIG_REALTEK_VOIP) += voip_manager/voip_resource_check.o

ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH),y)
ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST),y)
voip_manager-y += voip_manager/voip_mgr_help_host.o
endif
ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP),y)
voip_manager-y += voip_manager/voip_mgr_help_dsp.o
endif
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_manager
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/v152
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_ipc

ifeq "$(CONFIG_AUDIOCODES_VOIP)" "y"
OBJS_CFLAGS += -Iacmw_lx/ac_drv
OBJS_CFLAGS += -Iacmw_lx/include
OBJS_CFLAGS += -DAUDIOCODES_VOIP
OBJS_CFLAGS += -DACMW_LX
OBJS_CFLAGS += -DACL_BYPASS
OBJS_CFLAGS += -DACL_RELEASE
OBJS_CFLAGS += -DAUDIOCODES_VOIP_SIP
endif

ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8651),y)
OBJS_CFLAGS += -Idrivers/net/re865x/rtl865x
endif

ifeq ($(CONFIG_RTK_VOIP_SRTP),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/srtp/crypto/include
endif

ifeq ($(CONFIG_RTK_VOIP_T38),y)
ifneq ($(CONFIG_VOIP_SDK),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/t38/rly_src
endif
endif

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

#voip_manager-objs := $(voip-manager-mod-objs-y)

# rock: add module name for PRINT
#OBJS_CFLAGS += -DMODULE_NAME=\"MGR\"

#include $(DIR_VOIP)/rules.mak

CFLAGS_voip_mgr.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_netfilter.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_protocol.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_dsp.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_driver.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_misc.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_ipphone.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_do_debug.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_transfer_id.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_events.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_log.o := $(OBJS_CFLAGS)
CFLAGS_voip_resource_check.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_help_host.o := $(OBJS_CFLAGS)
CFLAGS_voip_mgr_help_dsp.o := $(OBJS_CFLAGS)
