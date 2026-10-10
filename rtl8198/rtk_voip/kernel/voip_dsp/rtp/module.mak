OBJS := rtp
OBJS_PATH := voip_dsp/rtp

$(OBJS)-y :=
$(OBJS)-y += $(OBJS_PATH)/RtpPacket.o
$(OBJS)-y += $(OBJS_PATH)/rtpTools.o
$(OBJS)-y += $(OBJS_PATH)/RtpReceiver.o
$(OBJS)-y += $(OBJS_PATH)/RtpTransmitter.o
$(OBJS)-y += $(OBJS_PATH)/rtpterm.o
$(OBJS)-y += $(OBJS_PATH)/rtp_main.o
$(OBJS)-y += $(OBJS_PATH)/RtcpReceiver.o
$(OBJS)-y += $(OBJS_PATH)/RtcpTransmitter.o
$(OBJS)-y += $(OBJS_PATH)/RtcpPacket.o
$(OBJS)-y += $(OBJS_PATH)/NtpTime.o
$(OBJS)-y += $(OBJS_PATH)/rfc2833trunk.o
$(OBJS)-$(CONFIG_RTK_VOIP_RTCP_XR) += $(OBJS_PATH)/RtcpTransmitterXR.o
$(OBJS)-$(CONFIG_RTK_VOIP_RTCP_XR) += $(OBJS_PATH)/RtcpReceiverXR.o
$(OBJS)-$(CONFIG_RTK_VOIP_RTCP_XR) += $(OBJS_PATH)/RtpReceiverXR.o
$(OBJS)-$(CONFIG_RTK_VOIP_RTCP_XR) += $(OBJS_PATH)/RtcpXR_RLE.o

OBJS_CFLAGS :=
#OBJS_CFLAGS += -DGNU
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/dsp_r1/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/v152
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/energy
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers/
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_manager/

ifeq ($(CONFIG_RTK_VOIP),m)
OBJS_CFLAGS += -mlong-calls
OBJS_CFLAGS += -DMODULE
endif

# rock: add module name for PRINT
#OBJS_CFLAGS += -DMODULE_NAME=\"RTP\"

#include $(DIR_VOIP)/rules.mak

CFLAGS_NtpTime.o := $(OBJS_CFLAGS)
CFLAGS_rfc2833trunk.o := $(OBJS_CFLAGS)
CFLAGS_RtcpPacket.o := $(OBJS_CFLAGS)
CFLAGS_RtcpReceiver.o := $(OBJS_CFLAGS)
CFLAGS_RtcpReceiverXR.o := $(OBJS_CFLAGS)
CFLAGS_RtcpTransmitter.o := $(OBJS_CFLAGS)
CFLAGS_RtcpTransmitterXR.o := $(OBJS_CFLAGS)
CFLAGS_RtcpXR_RLE.o := $(OBJS_CFLAGS)
CFLAGS_rtp_main.o := $(OBJS_CFLAGS)
CFLAGS_RtpPacket.o := $(OBJS_CFLAGS)
CFLAGS_RtpReceiver.o := $(OBJS_CFLAGS)
CFLAGS_RtpReceiverXR.o := $(OBJS_CFLAGS)
CFLAGS_rtpterm.o := $(OBJS_CFLAGS)
CFLAGS_rtpTools.o := $(OBJS_CFLAGS)
CFLAGS_RtpTransmitter.o := $(OBJS_CFLAGS)
