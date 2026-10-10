include $(DIR_VOIP)/voip_ipc/coprocessor_dsp/script/module.mak
coprocessor_dsp-y := $(ipc_script-y)
coprocessor_dsp-y += voip_ipc/coprocessor_dsp/coprocessor_dsp_tx.o
coprocessor_dsp-y += voip_ipc/coprocessor_dsp/coprocessor_dsp_rx.o

ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP),y)
coprocessor_dsp-y += voip_ipc/coprocessor_dsp/aipc_hook.o
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_ipc/
ifdef APP_DIR  # eCos
OBJS_CFLAGS += -I$(APP_DIR)/aipc/include
else  # Linux
ifeq ($(OPENWRT_BUILD),1)
export DIR_LINUX := $(srctree)
OBJS_CFLAGS += -I$(DIR_LINUX)/drivers/char/aipc/include
else
ifneq ($(CONFIG_CORTINA_VOIP_BUILD), y)
#OBJS_CFLAGS += -I$(DIR_LINUX)/drivers/char/aipc/include
#OBJS_CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char/aipc/include
else
#OBJS_CFLAGS += -I$(LINUXDIR)/rtk_voip/aipc_char/include
endif
OBJS_CFLAGS += -I$(DIR_AIPC)/include
endif
endif
OBJS_CFLAGS += -D_AIPC_DSP_

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

#OBJS_CFLAGS += -DMODULE_NAME=\"IPC\"

#include $(DIR_VOIP)/rules.mak
ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP),y)
ifneq ($(CONFIG_RTK_VOIP_IPC_ARCH_DSP_IN_DUAL_LINUX),y)
include $(APP_DIR)/.config
endif
endif

ifeq ($(CONFIG_RTL8686_IPC_IDX_USE_DRAM),y)
OBJS_CFLAGS += -DCONFIG_RTL8686_IPC_IDX_USE_DRAM
else
ifeq ($(CONFIG_RTL8686_IPC_IDX_USE_SRAM),y)
OBJS_CFLAGS += -DCONFIG_RTL8686_IPC_IDX_USE_SRAM
endif
endif

CFLAGS_coprocessor_dsp_tx.o := $(OBJS_CFLAGS)
CFLAGS_coprocessor_dsp_rx.o := $(OBJS_CFLAGS)
CFLAGS_aipc_hook.o := $(OBJS_CFLAGS)
