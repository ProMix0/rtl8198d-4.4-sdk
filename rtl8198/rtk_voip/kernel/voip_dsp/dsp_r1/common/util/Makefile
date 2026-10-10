EXTRA_AFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include -fno-pic -mno-abicalls

#obj-m += codec_mem_api.o codec_mem.o

# 2.6 SDK don't build these .o 
ifneq "$(CONFIG_VOIP_SDK)" "y"
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
obj-y += oper_32b.o pre_proc.o 
obj-y += dspfunc2.o
obj-y$(CONFIG_USE_ROMCODE) += dspfunc.o dspfunc_tab.o utility.o
else
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
obj-y += oper_32b.o pre_proc.o 
obj-y += dspfunc2.o
obj-y$(CONFIG_USE_ROMCODE) += dspfunc.o dspfunc_tab.o utility.o
else
obj-y += oper_32b.o pre_proc.o pre_procs.o \
	dmem_stack.o my_head.o
obj-y += dspfunc2.o
obj-y$(CONFIG_USE_ROMCODE) += dspfunc.o dspfuncs.o dspfunc_tab.o oper_32bs.o memset.o utility.o utilitys.o loop.o
endif
endif
endif


ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
EXTRA_CFLAGS += -mdsp
EXTRA_CFLAGS += -DVC6
endif
endif

ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
EXTRA_CFLAGS += -DVC6
endif

EXTRA_CFLAGS += -DGNU
EXTRA_CFLAGS += -I$(DIR_VOIP)/include
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
EXTRA_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/basic_op

include $(DIR_VOIP)/rules.mak

