ethernet_dsp-y := voip_ipc/ethernet_dsp/ethernet_dsp_tx.o
ethernet_dsp-y += voip_ipc/ethernet_dsp/ethernet_dsp_rx.o

ethernet_dsp-$(CONFIG_RTK_VOIP_ETHERNET_DSP_IS_DSP) += voip_ipc/ethernet_dsp/ethernet_dsp_help_dsp.o
OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_ipc/

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

ifeq "$(CONFIG_AUDIOCODES_VOIP)" "y"
OBJS_CFLAGS += -DAUDIOCODES_VOIP
OBJS_CFLAGS += -DACMW_LX
OBJS_CFLAGS += -DACL_BYPASS
OBJS_CFLAGS += -DACL_RELEASE
OBJS_CFLAGS += -DAUDIOCODES_VOIP_SIP
endif

#OBJS_CFLAGS += -DMODULE_NAME=\"IPC\"

#include $(DIR_VOIP)/rules.mak

CFLAGS_ethernet_dsp_tx.o := $(OBJS_CFLAGS)
CFLAGS_ethernet_dsp_rx.o := $(OBJS_CFLAGS)
CFLAGS_ethernet_dsp_help_dsp.o := $(OBJS_CFLAGS)
