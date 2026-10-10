dsp_r1-y :=

#obj-y    :=

#CONFIG_VOIP_SDK=y

ifeq ($(CONFIG_VOIP_SDK),y)

#dsp_r1-y += voip_dsp/dsp_r1/r1_main.obj
#dsp_r1-y += voip_dsp/dsp_r1/dtmf.obj
#dsp_r1-y += voip_dsp/dsp_r1/aes.obj
#dsp_r1-y += voip_dsp/dsp_r1/table.obj
#dsp_r1-y += voip_dsp/dsp_r1/jbc.obj
#dsp_r1-y += voip_dsp/dsp_r1/jbc2.obj

#dsp_r1-y += voip_dsp/dsp_r1/g7111.obj
#dsp_r1-y += voip_dsp/dsp_r1/common/common.obj
#dsp_r1-y += voip_dsp/dsp_r1/wsola.obj

#dsp_r1-y += voip_dsp/dsp_r1/r1_helpG711.obj

else

dsp_r1-y += voip_dsp/dsp_r1/r1_main.o
dsp_r1-y += voip_dsp/dsp_r1/dtmf.o
dsp_r1-y += voip_dsp/dsp_r1/aes.o
dsp_r1-y += voip_dsp/dsp_r1/table.o
dsp_r1-y += voip_dsp/dsp_r1/jbc.o
dsp_r1-y += voip_dsp/dsp_r1/jbc2.o

include $(DIR_VOIP)/voip_dsp/dsp_r1/g711/module.mak
dsp_r1-y += $(g711-y)
include $(DIR_VOIP)/voip_dsp/dsp_r1/common/module.mak
dsp_r1-y += $(common-y)
include $(DIR_VOIP)/voip_dsp/dsp_r1/wsola/module.mak
dsp_r1-y += $(wsola-y)
include $(DIR_VOIP)/voip_dsp/dsp_r1/help/module.mak
dsp_r1-y += $(help-y)

endif

#subdir-y += common/util
#obj-y += common/util/codec_mem_api.o
#obj-y += common/util/codec_mem.o

dsp_r1-y += voip_dsp/dsp_r1/common/util/codec_mem_api.o
dsp_r1-y += voip_dsp/dsp_r1/common/util/codec_mem.o

# For SDK not including g729/7231/726/722

ifeq ($(CONFIG_RTK_VOIP_G722),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += g7222.obj
#obj-y    += r1_helpG722.obj
else
ifeq ($(CONFIG_RTK_VOIP_G722_ITU_USE),y)
include $(DIR_VOIP)/voip_dsp/dsp_r1/g722_itu/module.mak
dsp_r1-y += $(g722_itu-y)
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/g722/module.mak
dsp_r1-y += $(g722-y)
endif
endif
endif

ifeq ($(CONFIG_RTK_VOIP_G7111),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y	+= g711.1.obj
#obj-y   += r1_helpG711.1.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/g711.1/module.mak
dsp_r1-y += $(g711.1-y)
endif
endif

ifeq ($(CONFIG_RTK_VOIP_G7231),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += g723.obj
#obj-y    += r1_helpG723.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/g723/module.mak
dsp_r1-y += $(g723-y)
endif
endif

ifeq ($(CONFIG_RTK_VOIP_G729AB),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += g729.obj
#obj-y    += r1_helpG729.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/g729/module.mak
dsp_r1-y += $(g729-y)
endif
endif
#obj-y	+= g7291_itu_c_codeC/

ifeq ($(CONFIG_RTK_VOIP_G726),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += g726.obj
#obj-y    += r1_helpG726.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/g726/module.mak
dsp_r1-y += $(g726-y)
endif
else
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += g726_api.obj
endif
endif

ifeq ($(CONFIG_RTK_VOIP_GSMFR),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += gsm_fr.obj
#obj-y    += r1_helpGSMfr.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/gsm_fr/module.mak
dsp_r1-y += $(gsm_fr-y)
endif
endif

ifeq ($(CONFIG_RTK_VOIP_AMR_NB),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += amr_nb.obj
#obj-y    += r1_helpAMRNB.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/amr_nb/module.mak
dsp_r1-y += $(amr_nb-y)
endif
endif

#subdir-y += speex
#obj-y    += speex/speex_out.o
ifeq ($(CONFIG_RTK_VOIP_SPEEX_NB),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y	+= speex.obj
#obj-y	+= r1_helpSPEEXNB.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/speex/module.mak
dsp_r1-y += $(speex-y)
endif
endif


ifeq ($(CONFIG_RTK_VOIP_ILBC),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += iLBC.obj
#obj-y    += r1_helpiLBC.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/iLBC/module.mak
dsp_r1-y += $(iLBC-y)
endif
endif

ifeq ($(CONFIG_RTK_VOIP_T38),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += t38.obj
#obj-y    += r1_helpT38.obj
else
include $(DIR_VOIP)/voip_dsp/dsp_r1/t38/module.mak
dsp_r1-y += $(t38-y)
endif
endif


ifeq ($(CONFIG_RTK_VOIP_SILENCE),y)
ifeq ($(CONFIG_VOIP_SDK),y)
#obj-y    += r1_helpSilence.obj
endif
endif

#subdir-y += speex
#obj-y    += speex/speex_out.o

# tone_.o is not used now!
#obj-y += tone_.o

###### RULES

#.PHONY: all clean

#all: r1_main

#clean:
#	-rm -f r1_main.exe r1_main.bin *.o
#	-rm -f *.o

OBJS_CFLAGS :=

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
OBJS_CFLAGS += -DVC6
endif
endif

OBJS_CFLAGS += -DGNU
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
OBJS_CFLAGS += -DVC6
endif
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/gsm_fr/include
ifeq ($(CONFIG_VOIP_SDK),y)
else
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/basic_op
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/util
CFLAGS_dtmf.o = -I$(DIR_VOIP)/voip_dsp/dsp_r0
endif
OBJS_CFLAGS +=	-I$(DIR_VOIP)/voip_dsp/cp3

ifeq "$(CONFIG_VOIP_COP3_PROFILE)" "y"
OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
OBJS_CFLAGS += -DFEATURE_COP3_G711DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G722DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G729DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7231DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G726DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_GSMFRDECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G711ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G722ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G729ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7231ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G726ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_GSMFRENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_ILBC30MSENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_ILBC30MSDECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_ILBC20MSENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_ILBC20MSDECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_T38ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_T38DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_AMRNBENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_AMRNBDECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_SPEEX_NB_ENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_SPEEX_NB_DECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7111NBENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7111NBDECPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7111WBENCPHASE
OBJS_CFLAGS += -DFEATURE_COP3_G7111WBDECPHASE
#OBJS_CFLAGS += -DFEATURE_COP3_G729JBC
#OBJS_CFLAGS += -DFEATURE_COP3_G729DEC
#OBJS_CFLAGS += -DFEATURE_COP3_PRE_PROCESS
#OBJS_CFLAGS += -DFEATURE_COP3_AES
#OBJS_CFLAGS += -DFEATURE_COP3_PLAYTONE
endif

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

#include $(DIR_VOIP)/rules.mak

CFLAGS_aes.o := $(OBJS_CFLAGS)
CFLAGS_dtmf.o := $(OBJS_CFLAGS) -I$(DIR_VOIP)/voip_dsp/dsp_r0
CFLAGS_jbc2.o := $(OBJS_CFLAGS)
CFLAGS_jbc.o := $(OBJS_CFLAGS)
CFLAGS_r1_main.o := $(OBJS_CFLAGS)
CFLAGS_table.o := $(OBJS_CFLAGS)
CFLAGS_tone_.o := $(OBJS_CFLAGS)

CFLAGS_codec_mem_api.o := $(OBJS_CFLAGS)
CFLAGS_codec_mem.o := $(OBJS_CFLAGS)
