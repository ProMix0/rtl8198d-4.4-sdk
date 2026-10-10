#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>  
#include <arpa/inet.h>    
#include <sys/time.h>   
#include "voip_manager.h"


#define FUNCKEY_REMOTE  ".9"

#define CHANNEL_NUM 1
#define DISABLE     0
#define ENABLE      1

#define SOCK_PORT   8888
#define RTP_PORT    12345
#define BUFF_LEN    16

#define INVITE      "INVITE"
#define RINGING     "RINGING"
#define OK          "OK"
#define BYE         "BYE"


#define CHANNEL_IS_FXS(chid)	RTK_VOIP_IS_SLIC_CH(chid, g_VoIP_Feature)


enum {
	FXO_NO_DAA = 0,
	FXO_VIRTUAL_DAA,
	FXO_DAA
};

enum {
	STATE_IDLE = 0,
	STATE_RING,
	STATE_CONNECT,
};

enum {
	DIAL_NONE = 0,
    DIAL_REMOTE,	
	DIAL_OTHER,
};

char *channel_state[3] = {"IDLE", "RING", "CONNECT"};

typedef struct channel_s channel_t;

struct channel_s {
	// phone info
	int chid;
	int state;
	char dial_code[101];
	int dial_index;
	// rtp info
	unsigned long ip;
	unsigned short port;
	// remote channel (just peer simulation)
    unsigned long remote_ip;
	unsigned short remote_port;
	channel_t *remote;
};

// global variable
int fxo_type = 0;
int active_sid = 0;
RtpPayloadType tx_codec = rtpPayloadPCMU;
RtpPayloadType rx_codec = rtpPayloadPCMU;
channel_t channels[CHANNEL_NUM];

int sock_fd;
fd_set readfds;
struct sockaddr_in local_addr;
struct sockaddr_in remote_addr;


void channel_init(channel_t *channel, int chid)
{
	memset(channel, 0, sizeof(*channel));

	// phone
	channel->chid = chid;
	channel->state = STATE_IDLE;
	channel->dial_code[0] = 0;
	channel->dial_index = 0;
	// rtp
	channel->ip = ntohl(inet_addr("127.0.0.1"));
	channel->port = 9000 + chid;
	// reomte
    channel->remote_ip = ntohl(inet_addr("127.0.0.1"));
	channel->remote_port = 9000 + chid; 
	channel->remote = NULL;

    printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
}

void channel_init_2(channel_t *channel, int chid)
{
	// phone
	channel->chid = chid;
	channel->state = STATE_IDLE;
	channel->dial_code[0] = 0;
	channel->dial_index = 0;
	// reomte
	channel->remote = NULL;

    printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
}

void channle_msg_send(char* msg)
{
    int len, ret;         

    len = sizeof(remote_addr);

    ret = sendto(sock_fd, msg, BUFF_LEN, 0, (struct sockaddr*)&remote_addr, len);   
    if (ret < 0) {   
        printf("sendto remote error!\n");
        return;
    }
    printf("send:%s\n", msg);
 
    return;   
}

int channel_input(channel_t *channel, SIGNSTATE val)
{
	if (channel->dial_index >= sizeof(channel->dial_code) - 1) 
	{
		printf("%s: out of buffer\n", __FUNCTION__);
		return DIAL_NONE;
	}

	if (val >= SIGN_KEY1 && val <= SIGN_KEY9) 
		channel->dial_code[channel->dial_index++] = (char) '0' + val;
	else if (val == SIGN_KEY0)
		channel->dial_code[channel->dial_index++] = '0';
	else if (val == SIGN_STAR)
		channel->dial_code[channel->dial_index++] = '.';
	else if (val == SIGN_HASH)
		return DIAL_OTHER;

	channel->dial_code[channel->dial_index] = 0;

	// check remote dial
	if (strcmp(channel->dial_code, FUNCKEY_REMOTE) == 0){
		return DIAL_REMOTE;	
    }

	return DIAL_NONE;
}

void channel_rtp_start(channel_t *channel)
{
	rtp_config_t rtp_config;
    rtcp_config_t rtcp_config;
	payloadtype_config_t codec_config;


	// set rtp session
	rtp_config.chid = channel->chid;
	rtp_config.sid = active_sid;
	rtp_config.isTcp = 0;		// use udp
	rtp_config.extIp = htonl(channel->ip);
	rtp_config.remIp = htonl(channel->remote_ip);
	rtp_config.extPort = htons(channel->port);
	rtp_config.remPort = htons(channel->remote_port);
#ifdef SUPPORT_VOICE_QOS
	rtp_config.tos = 0;
#endif

#ifdef SUPPORT_RFC2833
    rtp_config.rfc2833_payload_type_local  = 96;
    rtp_config.rfc2833_payload_type_remote = 96;
    rtp_config.rfc2833_fax_modem_pt_local = 101;
    rtp_config.rfc2833_fax_modem_pt_remote = 101;
#else
    rtp_config.rfc2833_payload_type_local  = 0;
    rtp_config.rfc2833_payload_type_remote = 0;
    rtp_config.rfc2833_fax_modem_pt_local = 0;
    rtp_config.rfc2833_fax_modem_pt_remote = 0;
#endif
#if defined(SUPPORT_RTP_REDUNDANT_APP)
	rtp_config.rtp_redundant_payload_type_local = 0;
	rtp_config.rtp_redundant_payload_type_remote = 0;
	rtp_config.rtp_redundant_max_Audio = 0;
	rtp_config.rtp_redundant_max_RFC2833 = 0;
#elif defined(SUPPORT_RTP_REDUNDANT)
	rtp_config.rtp_redundant_payload_type_local = 0;
	rtp_config.rtp_redundant_payload_type_remote = 0;
	rtp_config.rtp_redundant_max_Audio = 0;
	rtp_config.rtp_redundant_max_RFC2833 = 0;
#endif
	rtp_config.SID_payload_type_local = 0;
	rtp_config.SID_payload_type_remote = 0;
	rtp_config.init_randomly = 1;
	rtp_config.init_seqno = 0;
	rtp_config.init_SSRC = 0;
	rtp_config.init_timestamp = 0;
#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
    rtp_config.ipv6 = 0;
#endif
	rtk_SetRtpConfig(&rtp_config);

    // set RTCP session
    rtcp_config.chid	= rtp_config.chid;
    rtcp_config.sid		= rtp_config.sid;
    rtcp_config.remIp	= rtp_config.remIp;
    rtcp_config.remPort	= htons(channel->port+1);
    rtcp_config.extIp	= rtp_config.extIp;
    rtcp_config.extPort	= htons(channel->remote_port+1);
#ifdef SUPPORT_VOICE_QOS
    rtcp_config.tos		= rtp_config.tos;
#endif
			
    rtcp_config.txInterval = 500;	// send rtcp every 500ms
    rtcp_config.enableXR = 1;

#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
    rtcp_config.ipv6	= 0;
#endif
    rtk_SetRtcpConfig(&rtcp_config);

	// set rtp payload, and other session parameters.
	codec_config.chid = channel->chid;
	codec_config.sid = 0;
	codec_config.local_pt = tx_codec;
	codec_config.remote_pt = rx_codec;
	codec_config.uLocalPktFormat = tx_codec;
	codec_config.uRemotePktFormat = rx_codec;
	codec_config.local_pt_vbd = rtpPayloadUndefined;
	codec_config.remote_pt_vbd = rtpPayloadUndefined;
	codec_config.uPktFormat_vbd = rtpPayloadUndefined;
	codec_config.nG723Type = 0;
	codec_config.nLocalFramePerPacket = 1;
	codec_config.nRemoteFramePerPacket = 1;
	codec_config.nFramePerPacket_vbd = 1;
	codec_config.bVAD = 0;
	codec_config.bPLC = 1;
	codec_config.nJitterDelay = 4;
	codec_config.nMaxDelay = 13;
	codec_config.nMaxStrictDelay = 10000;
	codec_config.nJitterFactor = 7;
	codec_config.nG726Packing = 2;//pack right

	if ((codec_config.uLocalPktFormat == rtpPayloadG722) ||
	    (codec_config.uRemotePktFormat == rtpPayloadG722) )
		codec_config.nPcmMode = 3;	// wide-band
	else
		codec_config.nPcmMode = 2;	// narrow-band

	rtk_SetRtpPayloadType(&codec_config);

	// start rtp
    printf("Strat RTP\n"); 
	rtk_SetRtpSessionState(channel->chid, active_sid, rtp_session_sendrecv);
}

void channel_rtp_stop(channel_t *channel)
{
    printf("Stop RTP\n");
    rtk_SetRtpSessionState(channel->chid, active_sid, rtp_session_inactive);
}

void channel_dial_remote(channel_t *channel)
{
    /* create local rtp */
    channel_rtp_start(channel);
    channel->state = STATE_RING;
    printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);

    /* play ring back tone */
    rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_RINGING, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);

    /* send message INVITE to remote */
    channle_msg_send(INVITE);
}                             

void channel_handle_input(channel_t *channel, SIGNSTATE val)
{
	int ret;

	rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_HOLD, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);

	ret = channel_input(channel, val);
	switch (ret)
	{
    case DIAL_REMOTE:
		channel_dial_remote(channel);
		break;

    case DIAL_OTHER:
        rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_BUSY, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
        break;

    default:
		// nothing
		break;
	}
}

void channel_handle_onhook(channel_t *channel)
{
    printf("CHANNEL STATE: %s\n", channel_state[channel->state]);

    switch (channel->state)
    {
    case STATE_CONNECT:
        channel_rtp_stop(channel);
        channle_msg_send(BYE);
        break;

    default:
        break;
    }

    rtk_SetTranSessionID(channel->chid, 255);	// clear resource count
    rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_HOLD, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);	// clear tone
    channel_init_2(channel, channel->chid);

    return;
}

void channel_handle_offhook(channel_t *channel)
{
    // active session 0, and record resource + 1
    rtk_SetTranSessionID(channel->chid, active_sid);
    printf("CHANNEL STATE: %s\n", channel_state[channel->state]);

    switch (channel->state)
    {
    case STATE_IDLE:
        if (CHANNEL_IS_FXS(channel->chid))
        {
            // fxs
            rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_DIAL, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        break;

    case STATE_RING:
        if (CHANNEL_IS_FXS(channel->chid))
        {
            // fxs
            rtk_SetRingFxs(channel->chid, DISABLE);
            channel_rtp_start(channel);
            channel->state = STATE_CONNECT;
            printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
            channle_msg_send(OK);
        }
        break;

    default:
        break;
    }
}

int channel_socket_create(char *local_ip, char *remote_ip)
{
    int ret;   

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

#if 1 //server
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(local_ip);   
    local_addr.sin_port = htons(SOCK_PORT);             

    ret = bind(sock_fd, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
#else
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip);
    remote_addr.sin_port = htons(SOCK_PORT);           
#endif
    return 0;
}

int channle_msg_handle(channel_t *channel)
{
    int len, bytes;
    char buf[BUFF_LEN];        

    len = sizeof(remote_addr);
    memset(buf, 0, sizeof(buf));
    memset(&remote_addr, 0, sizeof(remote_addr));

    bytes = recvfrom(sock_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&remote_addr, (socklen_t *)&len);  
    if (bytes < 0) {   
        //printf("receive from remote error!\n");
        return 0;
    }

    if (strcmp(buf, INVITE) == 0) {
        printf("receive:%s\n", buf);
        printf("CHANNEL STATE: %s\n", channel_state[channel->state]);
        rtk_SetRingFxs(channel->chid, ENABLE);
        channel->state = STATE_RING;
        printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
        channle_msg_send(RINGING);
    }
    else if (strcmp(buf, RINGING) == 0) {
        printf("receive:%s\n", buf);
        printf("CHANNEL STATE: %s\n", channel_state[channel->state]);
        //rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_RINGING, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
    }
    else if (strcmp(buf, OK) == 0) {
        printf("receive:%s\n", buf);
        printf("CHANNEL STATE: %s\n", channel_state[channel->state]);
        channel->state = STATE_CONNECT;
        printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
        rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_RINGING, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);
    }
    else if (strcmp(buf, BYE) == 0) {
        printf("receive:%s\n", buf);
        printf("CHANNEL STATE: %s\n", channel_state[channel->state]);
        if (channel->state == STATE_CONNECT) {
            rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_BUSY, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
            channel->state = STATE_IDLE;
            channel_rtp_stop(channel);       
        }
        else if (channel->state == STATE_RING) {
            rtk_SetRingFxs(channel->chid, DISABLE);
            channel->state = STATE_IDLE;
        }
        else{
            //nothing
        }
        printf("Change CHANNEL STATE to %s\n", channel_state[channel->state]);
    }
    else {
        //nothing
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i=0, ret=0;
    SIGNSTATE val;
    channel_t *channel;
    char src_ip[32];
    char dst_ip[32];
    struct timeval authtime;

    // Usage
    if (argc != 5){
		printf("usage: %s <src_ip> <dest_ip> <tx_codec> <rx_codec>\n", argv[0]);
		return -1;
	}

    rtk_GetVoIPFeature();

    if ((g_VoIP_Feature & DAA_TYPE_MASK) == NO_DAA)
        fxo_type = FXO_NO_DAA;
    else if ((g_VoIP_Feature & DAA_TYPE_MASK) == VIRTUAL_DAA)
        fxo_type = FXO_VIRTUAL_DAA;
    else
        fxo_type = FXO_DAA;

    // init 
    for (i=0; i<CHANNEL_NUM; i++)
    {
        channel = &channels[i];

        // init dsp
        rtk_InitDsp(i);

        // init phone
        rtk_SetVoiceGain(i, 0, 0);              // 0db
        rtk_SetEchoTail(i, 32, 1, 5, 5, 0);     // 4ms, NLP on

        // init FXS
        if (CHANNEL_IS_FXS(i))
        {
            // FXS
            rtk_SetFlashHookTime(i, 100, 300,800);          // 100~300 ms
            //rtk_SetSlicRingCadence(i, 1500, 1500);    // 1500 ms
            rtk_SetDtmfCidParam(i, DTMF_DIGIT_A, DTMF_DIGIT_C, 0xf, 80, 80, 300, 300);	// priori 1st ring, auto AC, auto ring, digit duration = 80ms, inter digit pause = 80ms
        }

        // set dtmf mode
        rtk_SetDtmfMode(i, DTMF_INBAND);

        // set fax modem det mode		
        //rtk_SetFaxModemDet(i, 3);   //3:auto_2

        // set fax event detection
        //rtk_SetAnswerToneDet(i , 0x3f1212 , 0 , 10);

        // flush kernel fifo before app run
        rtk_SetFlushFifo(i);

        // init local data
        channel_init(channel, i);
               
        // set the entered parameters      
        channel->ip = ntohl(inet_addr(argv[1]));            //client ip
        channel->port = RTP_PORT;                           //client port
        tx_codec = atoi(argv[3]);                           //tx codec
        channel->remote_ip = ntohl(inet_addr(argv[2]));     //server ip
        channel->remote_port = RTP_PORT;                    //server port
        rx_codec = atoi(argv[4]);                           //rx codec      

        rtk_OnHookReinit(i);
    }
    
    /* create socket */
    strcpy(src_ip, argv[1]);
    strcpy(dst_ip, argv[2]);
    //printf("%s >>>>> src_ip:%s, dst_ip:%s\n", __func__, src_ip, dst_ip);
    ret = channel_socket_create(src_ip, dst_ip);
    if (ret < 0) {
        printf("channel_socket_create fail!\n");
        return -1;
    }

    while (1) 
    {
        authtime.tv_usec = 100000;
        authtime.tv_sec = 0;
        FD_ZERO(&readfds);              
        FD_SET(sock_fd, &readfds);      

        if (select(sock_fd+1, &readfds, NULL, NULL, &authtime) < 0)
            return -1;

        for (i=0; i<CHANNEL_NUM; i++)
        {
            channel = &channels[i];
            
            if (FD_ISSET(sock_fd, &readfds) != 0) {
                //printf("%s >>>>> channle_msg_handle\n", __func__);
                channle_msg_handle(channel);
            }

            if (CHANNEL_IS_FXS(i)) {
                rtk_GetFxsEvent(i, &val);
            }      

            switch (val)
            {
            case SIGN_KEY1:
            case SIGN_KEY2:
            case SIGN_KEY3:
            case SIGN_KEY4:
            case SIGN_KEY5:
            case SIGN_KEY6:
            case SIGN_KEY7:
            case SIGN_KEY8:
            case SIGN_KEY9:
            case SIGN_KEY0:
            case SIGN_STAR:
            case SIGN_HASH:
                if (channel->state == STATE_IDLE)
                {
                    channel_handle_input(channel, val);
                }
                break;

            case SIGN_ONHOOK:
                printf("FXS Event: ON-HOOK\n");
                channel_handle_onhook(channel);
                rtk_OnHookAction(i);	// rtk_OnHookAction have to be the last step in SIGN_ONHOOK
                break;

            case SIGN_OFFHOOK:
                printf("FXS Event: OFF-HOOK\n");
                rtk_OffHookAction(i);	// rtk_OffHookAction have to be the first step in SIGN_OFFHOOK
                channel_handle_offhook(channel);
                break;

            case SIGN_FLASHHOOK:
                printf("FXS Event: FLASH-HOOK\n");
                break;
            case SIGN_OFFHOOK_2:    // off-hook but no event
            case SIGN_NONE:
                break;

            default:
                printf("unknown(%d)\n", val);
                break;
            }

            //usleep(100000); // 100ms
        }
    }
    close(sock_fd);

    return 0;
}
