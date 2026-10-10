OBJS := v152
OBJS_PATH := voip_dsp/v152

OBJS_AFLAGS :=
OBJS_AFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include -fno-pic

$(OBJS)-y :=
$(OBJS)-y += $(OBJS_PATH)/v152_api.o
$(OBJS)-y += $(OBJS_PATH)/v152_proc.o

OBJS_CFLAGS :=

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
endif
endif

OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/rtp

#ifeq ($(CONFIG_RTK_VOIP),m)
#OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
#endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_v152_api.o := $(OBJS_CFLAGS)
CFLAGS_v152_proc.o := $(OBJS_CFLAGS)
