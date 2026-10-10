OBJS := dsp_r0
OBJS_PATH := voip_dsp/dsp_r0

$(OBJS)-y :=
$(OBJS)-y += $(OBJS_PATH)/dspcodec.o
$(OBJS)-y += $(OBJS_PATH)/dspparam.o

OBJS_CFLAGS :=

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
endif
endif

#OBJS_CFLAGS += -DGNU
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

include $(DIR_VOIP)/rules.mak

CFLAGS_dspcodec.o := $(OBJS_CFLAGS)
CFLAGS_dspparam.o := $(OBJS_CFLAGS)
