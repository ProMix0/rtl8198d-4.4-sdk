#
# Makefile for the DSP and PCM driver
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

ifeq ($(CONFIG_RTK_VOIP_DSP), y)
include $(DIR_VOIP)/voip_dsp/v152/module.mak
voip_dsp-y := $(v152-y)

export-objs =
dsp-mod-objs :=
#CONFIG_VOIP_SDK=y

    ifeq ($(CONFIG_VOIP_SDK),y)
        #voip_dsp-y += voip_dsp/dsp_main.obj
        #voip_dsp-y += voip_dsp/pcm_handler.obj
        #voip_dsp-y += voip_dsp/lec_g168.obj
        #voip_dsp-y += voip_dsp/dtmf.obj
        #voip_dsp-y += voip_dsp/fax.obj
        #voip_dsp-y += voip_dsp/ced_det.obj
        #voip_dsp-y += voip_dsp/fsk_gen.obj
        #voip_dsp-y += voip_dsp/voice_gain.obj
        #voip_dsp-y += voip_dsp/codec_descriptor.obj
        voip_dsp-y += voip_dsp/codec_descriptor.o
        voip_dsp-y += voip_dsp/dsp_init_var.o
        #voip_dsp-y += voip_dsp/cpu_setup.obj
        #voip_dsp-y += voip_dsp/energy.obj
        
        #ifeq ($(CONFIG_RTK_VOIP_DRIVERS_IP_PHONE),y)
        #obj-$(CONFIG_RTK_VOIP_DRIVERS_IIS) += resampler.obj
        #voip_dsp-y += voip_dsp/aec_g168.obj
        #voip_dsp-y += voip_dsp/nr.obj
        #else
        #voip_dsp-y += voip_dsp/aec_g168.obj
        #endif
        
        #ifeq ($(CONFIG_RTK_VOIP_DRIVERS_DAA_SUPPORT),y)
        #voip_dsp-y += voip_dsp/tone_detection.obj
        #voip_dsp-y += voip_dsp/fsk_det.obj
        #endif
        
        #obj-$(CONFIG_RTK_VOIP_WIDEBAND_SUPPORT) += resampler.obj
        
        #voip_dsp-y += voip_dsp/dtmf_cid.obj
        #voip_dsp-y += voip_dsp/silence_det.obj
      
    else #CONFIG_VOIP_SDK
        voip_dsp-y += voip_dsp/dsp_main.o
        voip_dsp-y += voip_dsp/pcm_handler.o
        voip_dsp-y += voip_dsp/codec_descriptor.o
        voip_dsp-y += voip_dsp/dsp_init_var.o
        voip_dsp-y += voip_dsp/cpu_setup.o
        #voip_dsp-y += voip_dsp/agc.o
        
        include $(DIR_VOIP)/voip_dsp/dtmf_dec/module.mak
        voip_dsp-y += $(dtmf_dec-objs-y)
        include $(DIR_VOIP)/voip_dsp/fax_v21/module.mak
        voip_dsp-y += $(fax_v21-objs-y)
        include $(DIR_VOIP)/voip_dsp/ced/module.mak
        voip_dsp-y += $(ced-y)
        include $(DIR_VOIP)/voip_dsp/fsk_gen/module.mak
        voip_dsp-y += $(fsk_gen-y)
        include $(DIR_VOIP)/voip_dsp/lec/module.mak
        voip_dsp-y += $(lec-y)
        #include $(DIR_VOIP)/voip_dsp/aec/module.mak
        voip_dsp-y += $(aec-y)
        include $(DIR_VOIP)/voip_dsp/energy/module.mak
        voip_dsp-y += $(energy-y)
        
        ifeq ("y",$(subst "yy","y","$(SUPPORT_RESAMPLER_G722)$(CONFIG_RTK_VOIP_DRIVERS_IIS)$(CONFIG_RTK_VOIP_WIDEBAND_SUPPORT)"))
        include $(DIR_VOIP)/voip_dsp/resampler/module.mak
        voip_dsp-y += $(resampler-y)
        else
        subdir-y += resampler/
        endif
        
        ifeq ($(CONFIG_RTK_VOIP_DRIVERS_IP_PHONE),y)
        include $(DIR_VOIP)/voip_dsp/nr/module.mak
        voip_dsp-y += $(nr-y)
        else
        include $(DIR_VOIP)/voip_dsp/nr/module.mak
        voip_dsp-y += $(nr-y)
        endif
        
        include $(DIR_VOIP)/voip_dsp/voice_gain/module.mak
        voip_dsp-y += $(voice_gain-y)
        
        
        ifeq ($(CONFIG_RTK_VOIP_DRIVERS_DAA_SUPPORT),y)
        include $(DIR_VOIP)/voip_dsp/tone_det/module.mak
        voip_dsp-y += $(tone_det-y)
        include $(DIR_VOIP)/voip_dsp/fsk_det/module.mak
        voip_dsp-y += $(fsk_det-y)
        ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SI3050),y)
        include $(DIR_VOIP)/voip_dsp/fxo_tune/module.mak
        voip_dsp-y += $(fxo_tune-y)
        endif
        else
        include $(DIR_VOIP)/voip_dsp/tone_det/module.mak
        voip_dsp-y += $(tone_det-y)
        include $(DIR_VOIP)/voip_dsp/fsk_det/module.mak
        voip_dsp-y += $(fsk_det-y)
        ifeq ($(CONFIG_RTK_VOIP_DRIVERS_SI3050),y)
        include $(DIR_VOIP)/voip_dsp/fxo_tune/module.mak
        voip_dsp-y += $(fxo_tune-y)
        endif
        endif
        
        include $(DIR_VOIP)/voip_dsp/dtmf_cid/module.mak
        voip_dsp-y += $(dtmf_cid-y)
        voip_dsp-y += voip_dsp/silence_det.o
    
    endif #CONFIG_VOIP_SDK
  
    include $(DIR_VOIP)/voip_dsp/ivr/module.mak
    voip_dsp-y += $(ivr-objs-y)
    include $(DIR_VOIP)/voip_dsp/rtp/module.mak
    voip_dsp-y += $(rtp-y)
    include $(DIR_VOIP)/voip_dsp/dsp_r1/module.mak
    voip_dsp-y += $(dsp_r1-y)
    include $(DIR_VOIP)/voip_dsp/dsp_r0/module.mak
    voip_dsp-y += $(dsp_r0-y)
    ifeq ($(FEATURE_COP3_PROFILE),y)
    include $(DIR_VOIP)/voip_dsp/cp3/module.mak
    voip_dsp-y += $(cp3-y)
    endif
  
    ifeq ($(CONFIG_RTK_VOIP_SOC_8686_CPU0),y)
        ifeq ($(CONFIG_RTL9602C_SERIES), y)
            ifeq ($(CONFIG_DEFAULTS_KERNEL_3_10), y)
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5281_K318_9602C.obj
            else
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5281_9602C.obj
            endif
        else
            ifeq ($(CONFIG_DEFAULTS_KERNEL_3_10), y)
                ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA), y)
                    ifeq ($(CONFIG_DEFAULTS_KERNEL_4_4), y)
                        voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_MIPS_IAUP_K44.obj
                    else
                        voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_MIPS_IAUP.obj
                    endif
                else
                    voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5281_K318.obj
                endif
            else
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5281.obj
            endif
        endif
    else
        ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM), y)
            ifeq ($(CONFIG_RTK_VOIP_CODEC_OBJECT_KERNEL_VERSION_4_14),y)
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_arm_ca_K414.obj
            else
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_arm_ca_K44.obj
            endif
        else
            ifeq ($(CONFIG_RTK_VOIP_SOC_8686_CPU1),y)
                voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5181.obj
            else
                ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8676),y)
                    voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5281.obj
                else
                    ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8954E_CPU1),y)
                        voip_dsp-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules_5181.obj
                    else
                        $(error check config)
                    endif
                endif
            endif
        endif
    endif
  
    #obj-$(CONFIG_VOIP_SDK) += voip_dsp/rtk_modules.obj
    
    #obj-y$(CONFIG_USE_ROMCODE) += voip_dsp/dsplib.obj # lib have to be the last

OBJS_CFLAGS :=

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -mdsp
OBJS_CFLAGS += -DVC6
ifeq ($(CONFIG_DEFAULTS_KERNEL_3_18),y)
OBJS_CFLAGS += -gdwarf-2
endif
endif
endif

ifeq ($(CONFIG_RTL8686_IPC_SINGLE_CPU),y)
ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_MIPS_IA),y)
OBJS_CFLAGS += -DVC6
endif
endif

ifeq ($(CONFIG_RTK_VOIP_CODEC_ON_ARM),y)
OBJS_CFLAGS += -DVC6
endif

OBJS_CFLAGS += -DGNU
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/cp3
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/energy/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/rtp
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/basic_op/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/common/util

ifeq ($(CONFIG_RTK_VOIP_PACKAGE_865X),y)
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8651)$(CONFIG_RTK_VOIP_DRIVERS_PCM865xC),y)
OBJS_CFLAGS += -Idrivers/net/re865x/rtl865x
endif
endif

ifeq "$(CONFIG_VOIP_COP3_PROFILE)" "y"
OBJS_CFLAGS += -DFEATURE_COP3_PROFILE
#OBJS_CFLAGS += -DFEATURE_COP3_DTMFDEC
#OBJS_CFLAGS += -DFEATURE_COP3_LEC
#OBJS_CFLAGS += -DFEATURE_COP3_DSPPROCESS
endif

#obj-m   := $(O_TARGET)

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

# rock: add module name for PRINT
#OBJS_CFLAGS += -DMODULE_NAME=\"DSP\"

include $(DIR_VOIP)/rules.mak

CFLAGS_agc.o := $(OBJS_CFLAGS)
CFLAGS_codec_descriptor.o := $(OBJS_CFLAGS)
CFLAGS_cpu_setup.o := $(OBJS_CFLAGS)
CFLAGS_dspapi.o := $(OBJS_CFLAGS)
CFLAGS_dsp_init_var.o := $(OBJS_CFLAGS)
CFLAGS_dsp_main.o := $(OBJS_CFLAGS)
CFLAGS_pcm_handler.o := $(OBJS_CFLAGS)
CFLAGS_playtone_unittest.o := $(OBJS_CFLAGS)
CFLAGS_silence_det.o := $(OBJS_CFLAGS)

endif # CONFIG_RTK_VOIP_DSP
