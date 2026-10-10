ifeq "$(VERSION).$(PATCHLEVEL)" "2.4"
DIR_VOIP := $(shell /bin/pwd)
O_TARGET := rtk_voip.o
else
DIR_VOIP := $(src)
#DIR_VOIP := $(srctree)/rtk_voip/kernel
DIR_AIPC := $(PACKAGE_DIR)/aipc_char
LINUXDIR=$(srctree)
DIR_LINUX=$(LINUXDIR)
export LINUXDIR
export DIR_LINUX
export DIR_AIPC
endif

#ifneq ($(CONFIG_CORTINA_VOIP_BUILD), y)
#BUILDDIR=$(shell /bin/pwd)
#ifeq ($(CONFIG_RTK_VOIP_PLATFORM_7742),y)
#LINUXDIR := $(BUILDDIR)/source
#DIR_VOIP := $(LINUXDIR)/rtk_voip
#else
#LINUXDIR := $(BUILDDIR)
#DIR_VOIP := $(LINUXDIR)/rtk_voip
#endif
#DIR_LINUX=$(LINUXDIR)
#export DIR_LINUX
#endif
#export LINUXDIR

#$(info LINUXDIR=$(LINUXDIR))
#$(info DIR_VOIP=$(DIR_VOIP))

export DIR_VOIP

ifdef DIR_VOIP
-include $(DIR_VOIP)/../.config
-include $(LINUXDIR)/.config
ifeq ($(CONFIG_VOIP_SDK),y)
export CONFIG_VOIP_SDK=y
endif
endif

obj-m := voip_kernel.o

voip_kernel-y := kernel_module.o main.o

ifneq "$(CONFIG_RTK_VOIP_DRIVERS_11N_MP)" "y"

ifeq ($(CONFIG_RTK_VOIP_RX),y)
include $(DIR_VOIP)/voip_rx/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_RX) += $(voip_rx-y)
endif
ifeq ($(CONFIG_RTK_VOIP_TX),y)
include $(DIR_VOIP)/voip_tx/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_TX) += $(voip_tx-y)
endif
ifeq ($(CONFIG_RTK_VOIP_SRTP),y)
include $(DIR_VOIP)/srtp/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_SRTP) += $(srtp-y)
endif

ifeq ($(CONFIG_RTK_VOIP_STANDALONE_ARCH),y)
include $(DIR_VOIP)/voip_dsp/module.mak
voip_kernel-y += $(voip_dsp-y)
endif

#obj-ipchost-y-sdk-y += voip_dsp/cpu_setup.obj
obj-ipchost-y-sdk-  += voip_dsp/cpu_setup.o voip_dsp/cp3/
#obj-ipchost--ac-y-sdk-y += voip_dsp/dsp_r1/t38.obj voip_dsp/cpu_setup.obj voip_dsp/Ntt_sRing_det.obj
obj-ipchost--ac-y-sdk-  += voip_dsp/dsp_r1/t38/    voip_dsp/cpu_setup.o   voip_dsp/fsk_det/
obj-ipchost--ac-y += voip_dsp/ivr/ voip_dsp/cp3/

#voip module remove the folling folder
#obj-ipchost--ac-  += voip_dsp/

obj-$(CONFIG_RTK_VOIP_DSP) += $(obj-ipchost-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST)-sdk-$(CONFIG_VOIP_SDK))
obj-$(CONFIG_RTK_VOIP_DSP) += $(obj-ipchost-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST)-ac-$(CONFIG_AUDIOCODES_VOIP)-sdk-$(CONFIG_VOIP_SDK))
obj-$(CONFIG_RTK_VOIP_DSP) += $(obj-ipchost-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST)-ac-$(CONFIG_AUDIOCODES_VOIP))


ifeq ($(CONFIG_RTK_VOIP_MANAGER),y)
include $(DIR_VOIP)/voip_manager/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_MANAGER) += $(voip_manager-y)
endif
ifeq ($(CONFIG_RTK_VOIP_DRIVERS),y)
include $(DIR_VOIP)/voip_drivers/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_DRIVERS) += $(voip_drivers-y)
endif

include $(DIR_VOIP)/voip_fs/module.mak
voip_kernel-y += $(voip_fs-y)

ifeq ($(CONFIG_RTK_VOIP_IPC_ARCH),y)
include $(DIR_VOIP)/voip_ipc/module.mak
voip_kernel-$(CONFIG_RTK_VOIP_IPC_ARCH) += $(voip_ipc-y)
endif

endif	# CONFIG_RTK_VOIP_DRIVERS_11N_MP

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
endif
endif

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
OBJS_CFLAGS += -march=armv8-a+crypto
OBJS_AFLAGS += -march=armv8-a+crypto
endif

ifdef CONFIG_PRINTK
# no need wrap
else
EXTRA_LDFLAGS += --wrap=printk
endif

ifeq "$(CONFIG_RTK_VOIP_PACKAGE_867X)" "y"
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_PCM8672)" "y"
CFLAGS_KERNEL += -march=5181
AFLAGS_KERNEL += -march=5181
endif
ifeq "$(CONFIG_RTK_VOIP_DRIVERS_PCM8676)" "y"
CFLAGS_KERNEL += -march=5281
AFLAGS_KERNEL += -march=5281
endif
endif

include $(DIR_VOIP)/rules.mak

CFLAGS_kernel_module.o := $(OBJS_CFLAGS)
CFLAGS_main.o := $(OBJS_CFLAGS)
CFLAGS_voip_kernel.o := $(OBJS_CFLAGS)
