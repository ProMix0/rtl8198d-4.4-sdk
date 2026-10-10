voip_ipc-y := voip_ipc/ipc_arch_tx.o
voip_ipc-y += voip_ipc/ipc_arch_rx.o
voip_ipc-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP) += voip_ipc/ipc_arch_help_dsp.o

ifeq ($(CONFIG_RTK_VOIP_ETHERNET_DSP),y)
include $(DIR_VOIP)/voip_ipc/ethernet_dsp/module.mak
voip_ipc-$(CONFIG_RTK_VOIP_ETHERNET_DSP) += $(ethernet_dsp-y)
endif
ifeq ($(CONFIG_RTK_VOIP_COPROCESS_DSP),y)
include $(DIR_VOIP)/voip_ipc/coprocessor_dsp/module.mak
voip_ipc-$(CONFIG_RTK_VOIP_COPROCESS_DSP) += $(coprocessor_dsp-y)
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_manager
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/

ifeq ($(CONFIG_RTK_VOIP_T38),y)
ifneq ($(CONFIG_VOIP_SDK),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/t38/rly_src
endif
endif

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

#hostprogs-y = ipc_arch_viewer
#always = $(hostprogs-y)
HOST_EXTRACFLAGS += -I$(DIR_VOIP)/include
HOST_EXTRACFLAGS += -Iinclude
ifeq ($(CONFIG_DEFAULTS_KERNEL_3_4),y)
HOST_EXTRACFLAGS += -include include/linux/kconfig.h
else
ifeq ($(CONFIG_DEFAULTS_KERNEL_3_10),y)
HOST_EXTRACFLAGS += -include include/linux/kconfig.h
else
ifeq ($(CONFIG_DEFAULTS_KERNEL_2_6),y)
HOST_EXTRACFLAGS += -include include/linux/config.h
endif
endif
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_ipc_arch_tx.o := $(OBJS_CFLAGS)
CFLAGS_ipc_arch_rx.o := $(OBJS_CFLAGS)
CFLAGS_ipc_arch_help_dsp.o := $(OBJS_CFLAGS)

