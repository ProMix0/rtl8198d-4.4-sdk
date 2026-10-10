ifeq ($(OPENWRT_BUILD), 1)
include ${DIR_LINUX}/.config
#CFLAGS += -I$(TOPDIR)/rtk_voip/maserati_2.0/includes
LDFLAGS += -lvoip_manager

#CFLAGS += -Werror
CFLAGS += -Wno-error

DSP_DIR=./dsp
MEDIA_DIR=./media
NET_DIR=./network
PHONE_DIR=./phone
OTHER_DIR=./other

all:
### Phone ###
	$(CC) $(PHONE_DIR)/calibrate.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/calibrate
	$(CC) $(PHONE_DIR)/conference.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/conference
	$(CC) $(PHONE_DIR)/daa.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/daa
	$(CC) $(PHONE_DIR)/fxo.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo
	$(CC) $(PHONE_DIR)/fxo_dual.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo_dual
	$(CC) $(PHONE_DIR)/fxo_int.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo_int
	$(CC) $(PHONE_DIR)/fxs.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs
	$(CC) $(PHONE_DIR)/fxs_int.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_int
	$(CC) $(PHONE_DIR)/genMeter.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/genMeter
	$(CC) $(PHONE_DIR)/lb_test.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/lb_test
	$(CC) $(PHONE_DIR)/pcmloopback.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmloopback
	$(CC) $(PHONE_DIR)/mpring.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/mpring
	$(CC) $(PHONE_DIR)/offhook_polarity.c $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/offhook_polarity
	$(CC) $(PHONE_DIR)/phoneoff.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/phoneoff
	$(CC) $(PHONE_DIR)/portdetect.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/portdetect
	$(CC) $(PHONE_DIR)/proslic_set.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/proslic_set
	$(CC) $(PHONE_DIR)/pulse_dial.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pulse_dial
	$(CC) $(PHONE_DIR)/ram.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/ram
	$(CC) $(PHONE_DIR)/reg.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/reg
	$(CC) $(PHONE_DIR)/ring.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/ring
	$(CC) $(PHONE_DIR)/slic_reset.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/slic_reset
	$(CC) $(PHONE_DIR)/slicset.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/slicset
	$(CC) $(PHONE_DIR)/voip_event.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/voip_event
	$(CC) $(PHONE_DIR)/pcmread.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmread
	$(CC) $(PHONE_DIR)/pcmplay.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmplay
	$(CC) $(PHONE_DIR)/fxs_client.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_client
	$(CC) $(PHONE_DIR)/fxs_server.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_server
	$(CC) $(PHONE_DIR)/fxo8.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo8
	$(CC) $(PHONE_DIR)/fxs_lan.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_lan

### DSP ###
	$(CC) $(DSP_DIR)/caller_id.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/caller_id
	$(CC) $(DSP_DIR)/country_tone_update.c $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/country_tone_update
	$(CC) $(DSP_DIR)/cp3_measure.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/cp3_measure
	$(CC) $(DSP_DIR)/custom_tone.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/custom_tone
	$(CC) $(DSP_DIR)/dis_tone.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dis_tone
	$(CC) $(DSP_DIR)/dtmf_det_cfg.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dtmf_det_cfg
	$(CC) $(DSP_DIR)/dtmfdet_dump.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dtmfdet_dump
	$(CC) $(DSP_DIR)/fax_ex.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fax_ex
	$(CC) $(DSP_DIR)/fsk_date.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fsk_date
	$(CC) $(DSP_DIR)/fskgen.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fskgen
	$(CC) $(DSP_DIR)/fxo_callerid_det.c    $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fxo_callerid_det
	$(CC) $(DSP_DIR)/gain_test.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/gain_test
	$(CC) $(DSP_DIR)/mptone.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/mptone
	$(CC) $(DSP_DIR)/pcm_dump.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/pcm_dump
	$(CC) $(DSP_DIR)/tone.c                $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/tone
	$(CC) $(DSP_DIR)/vmwigen.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/vmwigen
	$(CC) $(DSP_DIR)/fax_lan.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fax_lan

### Media ###
	$(CC) $(MEDIA_DIR)/rfc2833.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rfc2833
	$(CC) $(MEDIA_DIR)/rtcp_logger.c     $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtcp_logger
	$(CC) $(MEDIA_DIR)/rtcp_statistic.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtcp_statistic
	$(CC) $(MEDIA_DIR)/rtp.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp
	$(CC) $(MEDIA_DIR)/rtp_red.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_red
	$(CC) $(MEDIA_DIR)/rtp_rtcp_stats.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_rtcp_stats
	$(CC) $(MEDIA_DIR)/rtp2.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp2
	$(CC) $(MEDIA_DIR)/send_2833.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/send_2833
	$(CC) $(MEDIA_DIR)/t38_get.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/t38_get
#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	$(CC) $(MEDIA_DIR)/rtp_v6.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_v6
	$(CC) $(MEDIA_DIR)/t38_v6.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/t38_v6
#endif
#ifdef CONFIG_RTK_VOIP_AMR_WB
	$(CC) $(MEDIA_DIR)/fxo_int_amr_wb.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/fxo_int_amr_wb
#endif
	make -C $(MEDIA_DIR)/ivr_file_play/test_g711
	make -C $(MEDIA_DIR)/ivr_file_play/test_g723
	make -C $(MEDIA_DIR)/ivr_file_play/test_g729
	make -C $(MEDIA_DIR)/ivr_file_play/test_g722

### Network ###
#	$(CC) $(NET_DIR)/example.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(NET_DIR)/example

### Other ###
	$(CC) $(OTHER_DIR)/dbg.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/dbg
	$(CC) $(OTHER_DIR)/detbuf2_dump.c $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/detbuf2_dump
	$(CC) $(OTHER_DIR)/ec_test.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/ec_test
	$(CC) $(OTHER_DIR)/gpio.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/gpio 
	$(CC) $(OTHER_DIR)/led_ctrl.c     $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/led_ctrl
	$(CC) $(OTHER_DIR)/phone_test.c   $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/phone_test
	$(CC) $(OTHER_DIR)/rtptalk.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/rtptalk
	$(CC) $(OTHER_DIR)/sip_reg_led.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/sip_reg_led
	$(CC) $(OTHER_DIR)/slic_test.c    $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/slic_test

clean:
	rm -rf *.o \
	$(PHONE_DIR)/calibrate \
	$(PHONE_DIR)/conference \
	$(PHONE_DIR)/daa \
	$(PHONE_DIR)/fxo \
	$(PHONE_DIR)/fxo_dual \
	$(PHONE_DIR)/fxo_int \
	$(PHONE_DIR)/fxs \
	$(PHONE_DIR)/fxs_int \
	$(PHONE_DIR)/genMeter \
	$(PHONE_DIR)/lb_test \
	$(PHONE_DIR)/pcmloopback \
	$(PHONE_DIR)/mpring \
	$(PHONE_DIR)/offhook_polarity \
	$(PHONE_DIR)/phoneoff \
	$(PHONE_DIR)/portdetect \
	$(PHONE_DIR)/proslic_set \
	$(PHONE_DIR)/pulse_dial \
	$(PHONE_DIR)/ram \
	$(PHONE_DIR)/reg \
	$(PHONE_DIR)/ring \
	$(PHONE_DIR)/slic_reset \
	$(PHONE_DIR)/slicset \
	$(PHONE_DIR)/voip_event \
	$(PHONE_DIR)/pcmread \
	$(PHONE_DIR)/pcmplay \
	$(PHONE_DIR)/fxs_client \
	$(PHONE_DIR)/fxs_server \
	$(PHONE_DIR)/fxo8 \
	$(PHONE_DIR)/fxs_lan \
	$(DSP_DIR)/caller_id \
	$(DSP_DIR)/country_tone_update \
	$(DSP_DIR)/cp3_measure \
	$(DSP_DIR)/custom_tone \
	$(DSP_DIR)/dis_tone \
	$(DSP_DIR)/dtmf_det_cfg \
	$(DSP_DIR)/dtmfdet_dump \
	$(DSP_DIR)/fax_ex \
	$(DSP_DIR)/fsk_date \
	$(DSP_DIR)/fskgen \
	$(DSP_DIR)/fxo_callerid_det \
	$(DSP_DIR)/gain_test \
	$(DSP_DIR)/mptone \
	$(DSP_DIR)/pcm_dump \
	$(DSP_DIR)/tone \
	$(DSP_DIR)/vmwigen \
	$(DSP_DIR)/fax_lan \
	$(MEDIA_DIR)/rfc2833 \
	$(MEDIA_DIR)/rtcp_logger \
	$(MEDIA_DIR)/rtcp_statistic \
	$(MEDIA_DIR)/rtp \
	$(MEDIA_DIR)/rtp_red \
	$(MEDIA_DIR)/rtp_rtcp_stats \
	$(MEDIA_DIR)/rtp2 \
	$(MEDIA_DIR)/send_2833 \
	$(MEDIA_DIR)/t38_get \
	$(MEDIA_DIR)/rtp_v6 \
	$(MEDIA_DIR)/t38_v6 \
	$(MEDIA_DIR)/fxo_int_amr_wb \
	$(OTHER_DIR)/dbg \
	$(OTHER_DIR)/detbuf2_dump \
	$(OTHER_DIR)/ec_test \
	$(OTHER_DIR)/gpio \
	$(OTHER_DIR)/led_ctrl \
	$(OTHER_DIR)/phone_test \
	$(OTHER_DIR)/rtptalk \
	$(OTHER_DIR)/sip_reg_led \
	$(OTHER_DIR)/slic_test \	
	
else ifeq ($(YOCTO_BUILD), 1)
include ${PACKAGE_DIR}/.config
CFLAGS += $(EXTRA_CFLAGS) -I ../../includes
LDFLAGS += -L ../.. -lvoip_manager

CFLAGS += -Werror

DSP_DIR=./dsp
MEDIA_DIR=./media
NET_DIR=./network
PHONE_DIR=./phone
OTHER_DIR=./other

PHONE_TARGETS-y :=
PHONE_TARGETS-y += $(PHONE_DIR)/calibrate
PHONE_TARGETS-y += $(PHONE_DIR)/conference
PHONE_TARGETS-y += $(PHONE_DIR)/daa
PHONE_TARGETS-y += $(PHONE_DIR)/fxo8
PHONE_TARGETS-y += $(PHONE_DIR)/fxo
PHONE_TARGETS-y += $(PHONE_DIR)/fxo_dual
PHONE_TARGETS-y += $(PHONE_DIR)/fxo_int
PHONE_TARGETS-y += $(PHONE_DIR)/fxs
PHONE_TARGETS-y += $(PHONE_DIR)/fxs_client
PHONE_TARGETS-y += $(PHONE_DIR)/fxs_int
#PHONE_TARGETS-y += $(PHONE_DIR)/fxs_lan
PHONE_TARGETS-y += $(PHONE_DIR)/fxs_server
PHONE_TARGETS-y += $(PHONE_DIR)/genMeter
PHONE_TARGETS-y += $(PHONE_DIR)/lb_test
PHONE_TARGETS-y += $(PHONE_DIR)/mpring
PHONE_TARGETS-y += $(PHONE_DIR)/offhook_polarity
PHONE_TARGETS-y += $(PHONE_DIR)/pcmloopback
PHONE_TARGETS-y += $(PHONE_DIR)/pcmplay
PHONE_TARGETS-y += $(PHONE_DIR)/pcmread
PHONE_TARGETS-y += $(PHONE_DIR)/phoneoff
PHONE_TARGETS-y += $(PHONE_DIR)/portdetect
PHONE_TARGETS-y += $(PHONE_DIR)/proslic_set
PHONE_TARGETS-y += $(PHONE_DIR)/pulse_dial
PHONE_TARGETS-y += $(PHONE_DIR)/ram
PHONE_TARGETS-y += $(PHONE_DIR)/reg
PHONE_TARGETS-y += $(PHONE_DIR)/ring
PHONE_TARGETS-y += $(PHONE_DIR)/slic_reset
PHONE_TARGETS-y += $(PHONE_DIR)/slicset
PHONE_TARGETS-y += $(PHONE_DIR)/voip_event
PHONE_TARGETS-y += $(PHONE_DIR)/voipmp 

DSP_TARGETS-y :=
DSP_TARGETS-y += $(DSP_DIR)/caller_id
DSP_TARGETS-y += $(DSP_DIR)/country_tone_update
DSP_TARGETS-y += $(DSP_DIR)/cp3_measure
DSP_TARGETS-y += $(DSP_DIR)/custom_tone
DSP_TARGETS-y += $(DSP_DIR)/dis_tone
DSP_TARGETS-y += $(DSP_DIR)/dtmf_det_cfg
DSP_TARGETS-y += $(DSP_DIR)/dtmfdet_dump
DSP_TARGETS-y += $(DSP_DIR)/fax_ex
DSP_TARGETS-y += $(DSP_DIR)/fax_lan
DSP_TARGETS-y += $(DSP_DIR)/fsk_date
DSP_TARGETS-y += $(DSP_DIR)/fskgen
DSP_TARGETS-y += $(DSP_DIR)/fxo_callerid_det
DSP_TARGETS-y += $(DSP_DIR)/gain_test
DSP_TARGETS-y += $(DSP_DIR)/mptone
DSP_TARGETS-y += $(DSP_DIR)/pcm_dump
DSP_TARGETS-y += $(DSP_DIR)/tone
#DSP_TARGETS-y += $(DSP_DIR)/vmwi
DSP_TARGETS-y += $(DSP_DIR)/vmwigen

MEDIA_TARGETS-y :=
MEDIA_TARGETS-$(CONFIG_RTK_VOIP_AMR_WB) += $(MEDIA_DIR)/fxo_int_amr_wb
MEDIA_TARGETS-y += $(MEDIA_DIR)/rfc2833
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtcp_logger
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtcp_statistic
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtp2
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtp
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtp_red
MEDIA_TARGETS-y += $(MEDIA_DIR)/rtp_rtcp_stats
MEDIA_TARGETS-$(CONFIG_RTK_VOIP_IPV6_SUPPORT) += $(MEDIA_DIR)/rtp_v6
MEDIA_TARGETS-y += $(MEDIA_DIR)/send_2833
MEDIA_TARGETS-y += $(MEDIA_DIR)/t38_get
MEDIA_TARGETS-$(CONFIG_RTK_VOIP_IPV6_SUPPORT) += $(MEDIA_DIR)/t38_v6

NET_TARGETS-y :=

OTHER_TARGETS-y :=
OTHER_TARGETS-y += $(OTHER_DIR)/dbg
OTHER_TARGETS-y += $(OTHER_DIR)/detbuf2_dump
OTHER_TARGETS-y += $(OTHER_DIR)/ec_test
#OTHER_TARGETS-y += $(OTHER_DIR)/gpio
OTHER_TARGETS-y += $(OTHER_DIR)/led_ctrl
#OTHER_TARGETS-y += $(OTHER_DIR)/liner2alaw
OTHER_TARGETS-y += $(OTHER_DIR)/phone_test
OTHER_TARGETS-y += $(OTHER_DIR)/rtptalk
OTHER_TARGETS-y += $(OTHER_DIR)/sip_reg_led
OTHER_TARGETS-y += $(OTHER_DIR)/slic_test

TARGETS := $(PHONE_TARGETS-y) $(DSP_TARGETS-y) $(MEDIA_TARGETS-y) $(NET_TARGETS-y) $(OTHER_TARGETS-y)

all: $(TARGETS)
	make -C cortina

% : %.c
	$(CC) $(filter %.c,$^) $(CFLAGS) $(LIBS) $(LDFLAGS) -o $@
#	echo $@ >> .gitignore

$(TARGETS): ${PACKAGE_DIR}/.config

install: $(TARGETS)
	mkdir -p ${DEST}/usr/bin
	cp $^ ${DEST}/usr/bin
	make -C cortina install

clean:
	rm $(TARGETS)
	make -C cortina clean

else
ifndef DIR_LINUX
DIR_LINUX=$(ROOTDIR)/$(LINUXDIR)
endif
include ${DIR_LINUX}/.config

CFLAGS += -g -I$(DIR_USERS)/rtk_voip/includes -I$(DIR_LINUX)/rtk_voip/kernel/include  -I$(ROOTDIR)/rtk_voip/voip_manager_1.6/includes
#LDFLAGS += --static
#LIBS = ../../../voip_manager/libvoip_manager.a
#LDFLAGS += -L../../../voip_manager -lvoip_manager
LDFLAGS += -L$(ROOTDIR)/rtk_voip/voip_manager_1.6 -lvoip_manager -lpthread

ifdef CONFIG_RTK_VOIP_IP_PHONE
CFLAGS += -I../../../ui/include
endif

ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
LDFLAGS += -lpthread -ldl
endif

CFLAGS += $(ECOS_SHELL_CFLAGS)

#CFLAGS += -Werror
CFLAGS += -Wno-error

DSP_DIR=./dsp
MEDIA_DIR=./media
NET_DIR=./network
PHONE_DIR=./phone
OTHER_DIR=./other


#----------------------------------------------------------------------
all:
### Phone ###
	$(CC) $(PHONE_DIR)/voipmp.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/voipmp
	$(CC) $(PHONE_DIR)/calibrate.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/calibrate
	$(CC) $(PHONE_DIR)/conference.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/conference
	$(CC) $(PHONE_DIR)/daa.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/daa
	$(CC) $(PHONE_DIR)/fxo.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo
	$(CC) $(PHONE_DIR)/fxo_dual.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo_dual
	$(CC) $(PHONE_DIR)/fxo_int.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo_int
	$(CC) $(PHONE_DIR)/fxs.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs
	$(CC) $(PHONE_DIR)/fxs_int.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_int
	$(CC) $(PHONE_DIR)/genMeter.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/genMeter
	$(CC) $(PHONE_DIR)/lb_test.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/lb_test
	$(CC) $(PHONE_DIR)/pcmloopback.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmloopback
	$(CC) $(PHONE_DIR)/mpring.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/mpring
	$(CC) $(PHONE_DIR)/offhook_polarity.c $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/offhook_polarity
	$(CC) $(PHONE_DIR)/phoneoff.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/phoneoff
	$(CC) $(PHONE_DIR)/portdetect.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/portdetect
	$(CC) $(PHONE_DIR)/proslic_set.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/proslic_set
	$(CC) $(PHONE_DIR)/pulse_dial.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pulse_dial
	$(CC) $(PHONE_DIR)/ram.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/ram
	$(CC) $(PHONE_DIR)/reg.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/reg
	$(CC) $(PHONE_DIR)/ring.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/ring
	$(CC) $(PHONE_DIR)/slic_reset.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/slic_reset
	$(CC) $(PHONE_DIR)/slicset.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/slicset
	$(CC) $(PHONE_DIR)/voip_event.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/voip_event
	$(CC) $(PHONE_DIR)/pcmread.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmread
	$(CC) $(PHONE_DIR)/pcmplay.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/pcmplay
	$(CC) $(PHONE_DIR)/fxs_client.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_client
	$(CC) $(PHONE_DIR)/fxs_server.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_server
	$(CC) $(PHONE_DIR)/fxo8.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxo8
	$(CC) $(PHONE_DIR)/fxs_lan.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(PHONE_DIR)/fxs_lan

### DSP ###
	$(CC) $(DSP_DIR)/caller_id.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/caller_id
	$(CC) $(DSP_DIR)/country_tone_update.c $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/country_tone_update
	$(CC) $(DSP_DIR)/cp3_measure.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/cp3_measure
	$(CC) $(DSP_DIR)/custom_tone.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/custom_tone
	$(CC) $(DSP_DIR)/dis_tone.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dis_tone
	$(CC) $(DSP_DIR)/dtmf_det_cfg.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dtmf_det_cfg
	$(CC) $(DSP_DIR)/dtmfdet_dump.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/dtmfdet_dump
	$(CC) $(DSP_DIR)/fax_ex.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fax_ex
	$(CC) $(DSP_DIR)/fsk_date.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fsk_date
	$(CC) $(DSP_DIR)/fskgen.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fskgen
	$(CC) $(DSP_DIR)/fxo_callerid_det.c    $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fxo_callerid_det
	$(CC) $(DSP_DIR)/gain_test.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/gain_test
	$(CC) $(DSP_DIR)/mptone.c              $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/mptone
	$(CC) $(DSP_DIR)/pcm_dump.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/pcm_dump
	$(CC) $(DSP_DIR)/tone.c                $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/tone
	$(CC) $(DSP_DIR)/vmwigen.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/vmwigen
	$(CC) $(DSP_DIR)/fax_lan.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DSP_DIR)/fax_lan

### Media ###
	$(CC) $(MEDIA_DIR)/rfc2833.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rfc2833
	$(CC) $(MEDIA_DIR)/rtcp_logger.c     $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtcp_logger
	$(CC) $(MEDIA_DIR)/rtcp_statistic.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtcp_statistic
	$(CC) $(MEDIA_DIR)/rtp.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp
	$(CC) $(MEDIA_DIR)/rtp_red.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_red
	$(CC) $(MEDIA_DIR)/rtp_rtcp_stats.c  $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_rtcp_stats
	$(CC) $(MEDIA_DIR)/rtp2.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp2
	$(CC) $(MEDIA_DIR)/send_2833.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/send_2833
	$(CC) $(MEDIA_DIR)/t38_get.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/t38_get
#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	$(CC) $(MEDIA_DIR)/rtp_v6.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/rtp_v6
	$(CC) $(MEDIA_DIR)/t38_v6.c          $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/t38_v6
#endif
#ifdef CONFIG_RTK_VOIP_AMR_WB
	$(CC) $(MEDIA_DIR)/fxo_int_amr_wb.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(MEDIA_DIR)/fxo_int_amr_wb
#endif
	make -C $(MEDIA_DIR)/ivr_file_play/test_g711
	make -C $(MEDIA_DIR)/ivr_file_play/test_g722
	make -C $(MEDIA_DIR)/ivr_file_play/test_g723
	make -C $(MEDIA_DIR)/ivr_file_play/test_g729

### Network ###
#	$(CC) $(NET_DIR)/example.c           $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(NET_DIR)/example

### Other ###
	$(CC) $(OTHER_DIR)/dbg.c             $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/dbg
	$(CC) $(OTHER_DIR)/detbuf2_dump.c    $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/detbuf2_dump
	$(CC) $(OTHER_DIR)/ec_test.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/ec_test
	$(CC) $(OTHER_DIR)/gpio.c            $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/gpio 
	$(CC) $(OTHER_DIR)/led_ctrl.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/led_ctrl
	$(CC) $(OTHER_DIR)/phone_test.c      $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/phone_test
	$(CC) $(OTHER_DIR)/rtptalk.c         $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/rtptalk
	$(CC) $(OTHER_DIR)/sip_reg_led.c     $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/sip_reg_led
	$(CC) $(OTHER_DIR)/slic_test.c       $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(OTHER_DIR)/slic_test

ecos-shell:
	$(CC) $(MEDIA_DIR)/rtp.c $(CFLAGS) -c -o rtp.o 

romfs:
### Phone ###
	$(ROMFSINST) $(PHONE_DIR)/voipmp           /bin/voipmp
	$(ROMFSINST) $(PHONE_DIR)/calibrate        /bin/calibrate
	$(ROMFSINST) $(PHONE_DIR)/conference       /bin/conference
	$(ROMFSINST) $(PHONE_DIR)/daa              /bin/daa
	$(ROMFSINST) $(PHONE_DIR)/fxo              /bin/fxo
	$(ROMFSINST) $(PHONE_DIR)/fxo_dual         /bin/fxo_dual
	$(ROMFSINST) $(PHONE_DIR)/fxo_int          /bin/fxo_int
	$(ROMFSINST) $(PHONE_DIR)/fxs              /bin/fxs
	$(ROMFSINST) $(PHONE_DIR)/fxs_int          /bin/fxs_int
	$(ROMFSINST) $(PHONE_DIR)/genMeter         /bin/genMeter
	$(ROMFSINST) $(PHONE_DIR)/lb_test          /bin/lb_test
	$(ROMFSINST) $(PHONE_DIR)/pcmloopback      /bin/pcmloopback
	$(ROMFSINST) $(PHONE_DIR)/mpring           /bin/mpring
	$(ROMFSINST) $(PHONE_DIR)/offhook_polarity /bin/offhook_polarity
	$(ROMFSINST) $(PHONE_DIR)/phoneoff         /bin/phoneoff
	$(ROMFSINST) $(PHONE_DIR)/portdetect       /bin/portdetect
	$(ROMFSINST) $(PHONE_DIR)/proslic_set      /bin/proslic_set
	$(ROMFSINST) $(PHONE_DIR)/pulse_dial       /bin/pulse_dial
	$(ROMFSINST) $(PHONE_DIR)/ram              /bin/ram
	$(ROMFSINST) $(PHONE_DIR)/reg              /bin/reg
	$(ROMFSINST) $(PHONE_DIR)/ring             /bin/ring
	$(ROMFSINST) $(PHONE_DIR)/slic_reset       /bin/slic_reset
	$(ROMFSINST) $(PHONE_DIR)/slicset          /bin/slicset
	$(ROMFSINST) $(PHONE_DIR)/voip_event       /bin/voip_event
	$(ROMFSINST) $(PHONE_DIR)/pcmread          /bin/pcmread
	$(ROMFSINST) $(PHONE_DIR)/pcmplay          /bin/pcmplay
	$(ROMFSINST) $(PHONE_DIR)/fxs_client       /bin/fxs_client
	$(ROMFSINST) $(PHONE_DIR)/fxs_server       /bin/fxs_server
	$(ROMFSINST) $(PHONE_DIR)/fxo8             /bin/fxo8
	$(ROMFSINST) $(PHONE_DIR)/fxs_lan          /bin/fxs_lan

### DSP ###
	$(ROMFSINST) $(DSP_DIR)/caller_id           /bin/caller_id
	$(ROMFSINST) $(DSP_DIR)/country_tone_update /bin/country_tone_update
	$(ROMFSINST) $(DSP_DIR)/cp3_measure         /bin/cp3_measure
	$(ROMFSINST) $(DSP_DIR)/custom_tone         /bin/custom_tone
	$(ROMFSINST) $(DSP_DIR)/dis_tone            /bin/dis_tone
	$(ROMFSINST) $(DSP_DIR)/dtmf_det_cfg        /bin/dtmf_det_cfg
	$(ROMFSINST) $(DSP_DIR)/dtmfdet_dump        /bin/dtmfdet_dump
	$(ROMFSINST) $(DSP_DIR)/fax_ex              /bin/fax_ex
	$(ROMFSINST) $(DSP_DIR)/fsk_date            /bin/fsk_date
	$(ROMFSINST) $(DSP_DIR)/fskgen              /bin/fskgen
	$(ROMFSINST) $(DSP_DIR)/fxo_callerid_det    /bin/fxo_callerid_det
	$(ROMFSINST) $(DSP_DIR)/gain_test           /bin/gain_test
	$(ROMFSINST) $(DSP_DIR)/mptone              /bin/mptone
	$(ROMFSINST) $(DSP_DIR)/pcm_dump            /bin/pcm_dump
	$(ROMFSINST) $(DSP_DIR)/tone                /bin/tone
	$(ROMFSINST) $(DSP_DIR)/vmwigen             /bin/vmwigen
	$(ROMFSINST) $(DSP_DIR)/fax_lan             /bin/fax_lan

### Media ###
	$(ROMFSINST) $(MEDIA_DIR)/rfc2833        /bin/rfc2833
	$(ROMFSINST) $(MEDIA_DIR)/rtcp_logger    /bin/rtcp_logger
	$(ROMFSINST) $(MEDIA_DIR)/rtcp_statistic /bin/rtcp_statistic
	$(ROMFSINST) $(MEDIA_DIR)/rtp            /bin/rtp
	$(ROMFSINST) $(MEDIA_DIR)/rtp_red        /bin/rtp_red
	$(ROMFSINST) $(MEDIA_DIR)/rtp_rtcp_stats /bin/rtp_rtcp_stats
	$(ROMFSINST) $(MEDIA_DIR)/rtp2           /bin/rtp2
	$(ROMFSINST) $(MEDIA_DIR)/send_2833      /bin/send_2833
	$(ROMFSINST) $(MEDIA_DIR)/t38_get        /bin/t38_get
#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	$(ROMFSINST) $(MEDIA_DIR)/rtp_v6         /bin/rtp_v6
	$(ROMFSINST) $(MEDIA_DIR)/t38_v6         /bin/t38_v6
#endif
#ifdef CONFIG_RTK_VOIP_AMR_WB
	$(ROMFSINST) $(MEDIA_DIR)/fxo_int_amr_wb     /bin/fxo_int_amr_wb
#endif

	make -C $(MEDIA_DIR)/ivr_file_play/test_g711 romfs
	make -C $(MEDIA_DIR)/ivr_file_play/test_g722 romfs
	make -C $(MEDIA_DIR)/ivr_file_play/test_g723 romfs
	make -C $(MEDIA_DIR)/ivr_file_play/test_g729 romfs

### Network ###
#	$(ROMFSINST) $(NET_DIR)/example          /bin/example

### Other ###
	$(ROMFSINST) $(OTHER_DIR)/dbg            /bin/dbg
	$(ROMFSINST) $(OTHER_DIR)/detbuf2_dump   /bin/detbuf2_dump
	$(ROMFSINST) $(OTHER_DIR)/ec_test        /bin/ec_test
	$(ROMFSINST) $(OTHER_DIR)/gpio           /bin/gpio
	$(ROMFSINST) $(OTHER_DIR)/led_ctrl       /bin/led_ctrl
	$(ROMFSINST) $(OTHER_DIR)/phone_test     /bin/phone_test
	$(ROMFSINST) $(OTHER_DIR)/rtptalk        /bin/rtptalk
	$(ROMFSINST) $(OTHER_DIR)/sip_reg_led    /bin/sip_reg_led
	$(ROMFSINST) $(OTHER_DIR)/slic_test      /bin/slic_test

clean:
	rm -rf *.o \
	$(PHONE_DIR)/voipmp \
	$(PHONE_DIR)/calibrate \
	$(PHONE_DIR)/conference \
	$(PHONE_DIR)/daa \
	$(PHONE_DIR)/fxo \
	$(PHONE_DIR)/fxo_dual \
	$(PHONE_DIR)/fxo_int \
	$(PHONE_DIR)/fxs \
	$(PHONE_DIR)/fxs_int \
	$(PHONE_DIR)/genMeter \
	$(PHONE_DIR)/lb_test \
	$(PHONE_DIR)/pcmloopback \
	$(PHONE_DIR)/mpring \
	$(PHONE_DIR)/offhook_polarity \
	$(PHONE_DIR)/phoneoff \
	$(PHONE_DIR)/portdetect \
	$(PHONE_DIR)/proslic_set \
	$(PHONE_DIR)/pulse_dial \
	$(PHONE_DIR)/ram \
	$(PHONE_DIR)/reg \
	$(PHONE_DIR)/ring \
	$(PHONE_DIR)/slic_reset \
	$(PHONE_DIR)/slicset \
	$(PHONE_DIR)/voip_event \
	$(PHONE_DIR)/pcmread \
	$(PHONE_DIR)/pcmplay \
	$(PHONE_DIR)/fxs_client \
	$(PHONE_DIR)/fxs_server \
	$(PHONE_DIR)/fxo8 \
	$(PHONE_DIR)/fxs_lan \
	$(DSP_DIR)/caller_id \
	$(DSP_DIR)/country_tone_update \
	$(DSP_DIR)/cp3_measure \
	$(DSP_DIR)/custom_tone \
	$(DSP_DIR)/dis_tone \
	$(DSP_DIR)/dtmf_det_cfg \
	$(DSP_DIR)/dtmfdet_dump \
	$(DSP_DIR)/fax_ex \
	$(DSP_DIR)/fsk_date \
	$(DSP_DIR)/fskgen \
	$(DSP_DIR)/fxo_callerid_det \
	$(DSP_DIR)/gain_test \
	$(DSP_DIR)/mptone \
	$(DSP_DIR)/pcm_dump \
	$(DSP_DIR)/tone \
	$(DSP_DIR)/vmwigen \
	$(DSP_DIR)/fax_lan \
	$(MEDIA_DIR)/rfc2833 \
	$(MEDIA_DIR)/rtcp_logger \
	$(MEDIA_DIR)/rtcp_statistic \
	$(MEDIA_DIR)/rtp \
	$(MEDIA_DIR)/rtp_red \
	$(MEDIA_DIR)/rtp_rtcp_stats \
	$(MEDIA_DIR)/rtp2 \
	$(MEDIA_DIR)/send_2833 \
	$(MEDIA_DIR)/t38_get \
	$(MEDIA_DIR)/rtp_v6 \
	$(MEDIA_DIR)/t38_v6 \
	$(MEDIA_DIR)/fxo_int_amr_wb \
	$(OTHER_DIR)/dbg \
	$(OTHER_DIR)/detbuf2_dump \
	$(OTHER_DIR)/ec_test \
	$(OTHER_DIR)/gpio \
	$(OTHER_DIR)/led_ctrl \
	$(OTHER_DIR)/phone_test \
	$(OTHER_DIR)/rtptalk \
	$(OTHER_DIR)/sip_reg_led \
	$(OTHER_DIR)/slic_test \

	make -C $(MEDIA_DIR)/ivr_file_play/test_g711 clean
	make -C $(MEDIA_DIR)/ivr_file_play/test_g722 clean
	make -C $(MEDIA_DIR)/ivr_file_play/test_g723 clean
	make -C $(MEDIA_DIR)/ivr_file_play/test_g729 clean

endif # $(OPENWRT_BUILD)
