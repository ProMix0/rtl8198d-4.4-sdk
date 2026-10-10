OBJS := ivr-objs
OBJS_PATH := voip_dsp/ivr

OBJS_AFLAGS :=
OBJS_AFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include -fno-pic

$(OBJS)-y :=
#CONFIG_VOIP_SDK=y

ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y += ivr.obj
ifeq ($(CONFIG_RTK_VOIP_IVR_DEV),y)
#obj-y += ivr_dev.obj
endif
ifeq ($(CONFIG_RTK_VOIP_G7231),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
#obj-y += ivr_g723sup.obj
endif
endif
ifeq ($(CONFIG_RTK_VOIP_G729AB),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
#obj-y += ivr_g729sup.obj
endif
endif
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
#obj-y += ivr_g711sup.obj
endif
ifeq ($(CONFIG_RTK_VOIP_IVR_LINEAR_8K),y)
#obj-y += ivr_linear8ksup.obj
endif
ifeq ($(CONFIG_RTK_VOIP_IVR_LINEAR_16K),y)
#obj-y += ivr_linear16ksup.obj
endif
ifeq ($(CONFIG_RTK_VOIP_G722),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
#obj-y += ivr_g722sup.obj
endif
endif
else
$(OBJS)-y += $(OBJS_PATH)/ivr.o
ifeq ($(CONFIG_RTK_VOIP_IVR_DEV),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_dev.o
endif
ifeq ($(CONFIG_RTK_VOIP_G7231),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_g723sup.o
endif
endif
ifeq ($(CONFIG_RTK_VOIP_G729AB),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_g729sup.o
endif
endif
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_g711sup.o
endif
ifeq ($(CONFIG_RTK_VOIP_IVR_LINEAR_8K),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_linear8ksup.o
endif
ifeq ($(CONFIG_RTK_VOIP_IVR_LINEAR_16K),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_linear16ksup.o
endif
ifeq ($(CONFIG_RTK_VOIP_G722),y)
ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_g722sup.o
endif
endif
endif

$(OBJS)-y += $(OBJS_PATH)/ivr_dummy.o

ifeq ($(CONFIG_RTK_VOIP_IVR_TEXT),y)
$(OBJS)-y += $(OBJS_PATH)/ivr_speech.o
endif

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include

ifneq ($(CONFIG_AUDIOCODES_VOIP),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/g729/include

ifeq ($(CONFIG_RTK_VOIP_G729AB),y)
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/util
endif
endif
ifeq "$(CONFIG_AUDIOCODES_VOIP)" "y"
OBJS_CFLAGS += -DAUDIOCODES_VOIP
OBJS_CFLAGS += -DACMW_LX
OBJS_CFLAGS += -DACL_BYPASS
OBJS_CFLAGS += -DACL_RELEASE
OBJS_CFLAGS += -DAUDIOCODES_VOIP_SIP
endif

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
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

include $(DIR_VOIP)/rules.mak

CFLAGS_ivr.o := $(OBJS_CFLAGS)
CFLAGS_ivr_dev.o := $(OBJS_CFLAGS)
CFLAGS_ivr_dummy.o := $(OBJS_CFLAGS)
CFLAGS_ivr_g711sup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_g722sup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_g723sup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_g729sup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_linear16ksup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_linear8ksup.o := $(OBJS_CFLAGS)
CFLAGS_ivr_speech.o := $(OBJS_CFLAGS)
CFLAGS_ivr_speech_table.o := $(OBJS_CFLAGS)
