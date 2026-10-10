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


#define DISABLE     0
#define ENABLE      1

#define MAX_FXS     8
#define SOCK_PORT   20000
#define RTP_PORT    12345
#define BUFF_LEN    32
#define CH_NUM      1

#define INVITE      "INVITE"
#define RINGING     "RINGING"
#define OK          "OK"
#define BYE         "BYE"


#define CHANNEL_IS_FXS(chid)	RTK_VOIP_IS_SLIC_CH(chid, g_VoIP_Feature)


enum {
    STATE_IDLE = 0,
    STATE_RING_LOCAL,
    STATE_RING_REMOTE,
    STATE_CONNECT,
};

enum {
    DIAL_NONE = -1,
    DIAL_REMOTE_0,
    DIAL_REMOTE_1,
    DIAL_REMOTE_2,
    DIAL_REMOTE_3,
    DIAL_REMOTE_4,
    DIAL_REMOTE_5,
    DIAL_REMOTE_6,
    DIAL_REMOTE_7,	
    DIAL_OTHER,
};

char *channel_state[4] = {"IDLE", "RING_LOCAL", "RING_REMOTE", "CONNECT"};

char funckey[MAX_FXS][4];

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
    int remote_chid;
    channel_t *remote;
};

// global variable
int active_sid = 0;
RtpPayloadType tx_codec = rtpPayloadUndefined;
RtpPayloadType rx_codec = rtpPayloadUndefined;
channel_t channels[CON_CH_NUM];

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
    channel->ip = inet_addr("127.0.0.1");
    channel->port = SOCK_PORT;
    // reomte
    channel->remote_ip = inet_addr("127.0.0.1");
    channel->remote_port = SOCK_PORT;
    channel->remote_chid = -1;
    channel->remote = NULL;

    printf("%s: channel %d init done.\n", __FILE__, chid);
}

void channel_init_2(channel_t *channel, int chid)
{
    // phone
    channel->chid = chid;
    channel->state = STATE_IDLE;
    channel->dial_code[0] = 0;
    channel->dial_index = 0;
    // reomte
    channel->remote_chid = -1;
    channel->remote = NULL;

    printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);
}

void channle_msg_send(char* msg)
{
    int len, ret;         

    len = sizeof(remote_addr);

    ret = sendto(sock_fd, msg, BUFF_LEN, 0, (struct sockaddr*)&remote_addr, len);   
    if (ret < 0) {   
        printf("%s: sendto remote error!\n", __FILE__);
        return;
    }
    printf("%s: send:%s\n", __FILE__, msg);
 
    return;   
}

int channel_input(channel_t *channel, SIGNSTATE val)
{
    int ch;

    if (channel->dial_index >= sizeof(channel->dial_code) - 1) {
		printf("%s: out of buffer!\n", __FILE__);
		return DIAL_NONE;
    }

    if (val >= SIGN_KEY0 && val <= SIGN_KEY9) 
            channel->dial_code[channel->dial_index++] = (char) '0' + val;
    else if (val == SIGN_STAR)
            channel->dial_code[channel->dial_index++] = '.';
    else if (val == SIGN_HASH)
            return DIAL_OTHER;

    channel->dial_code[channel->dial_index] = 0;

    /* check if dialed number match */
    for (ch=0; ch<MAX_FXS; ch++)
    {
        if (strcmp(channel->dial_code, funckey[ch]) == 0) {
            printf("%s: mathced key:%s\n", __FILE__, funckey[ch]);
            return ch;
        }
    }

    return DIAL_NONE;
}

void channel_rtp_start(channel_t *channel)
{
    rtp_config_t rtp_config;
    rtcp_config_t rtcp_config;
    payloadtype_config_t codec_config;


    memset(&rtp_config, 0, sizeof(rtp_config));
    memset(&rtcp_config, 0, sizeof(rtcp_config));
    memset(&codec_config, 0, sizeof(codec_config));

    // set RTP session
    rtp_config.chid = channel->chid;
    rtp_config.sid = active_sid;
    rtp_config.isTcp = 0;		// use udp
    rtp_config.extIp = channel->ip;
    rtp_config.remIp = channel->remote_ip;
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
			
	rtcp_config.txInterval = 5000;		// send rtcp every 5s
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
    printf("%s: strat RTP\n", __FILE__); 
    rtk_SetRtpSessionState(channel->chid, active_sid, rtp_session_sendrecv);
}

void channel_rtp_stop(channel_t *channel)
{
    printf("%s: stop RTP\n", __FILE__);
    rtk_SetRtpSessionState(channel->chid, active_sid, rtp_session_inactive);
}

void channel_dial_remote(channel_t *channel, int dialto)
{
    char msg[BUFF_LEN];

    memset(msg, 0, sizeof(msg));
   
    channel->state = STATE_RING_REMOTE;
    printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);

    /* play ring back tone */
    rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_RINGING, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);

    /* send message INVITE to remote */
    if (dialto >= DIAL_REMOTE_0 && dialto <= DIAL_REMOTE_7) {
        channel->remote_chid = dialto;
        sprintf(msg, "%s to %d from %d", INVITE, channel->remote_chid, channel->chid);
        printf("%s: send msg:%s\n", __FILE__, msg);
        channle_msg_send(msg);
    } 
    else
        printf("%s: no matched number!\n", __FILE__);   
}
                             
void channel_handle_input(channel_t *channel, SIGNSTATE val)
{
    int ret;

    rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_HOLD, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);

    ret = channel_input(channel, val);
    switch (ret)
    {    
    case DIAL_REMOTE_0:
    case DIAL_REMOTE_1:
    case DIAL_REMOTE_2:
    case DIAL_REMOTE_3:
    case DIAL_REMOTE_4:
    case DIAL_REMOTE_5:
    case DIAL_REMOTE_6:
    case DIAL_REMOTE_7:    
        channel_dial_remote(channel, ret);
        break;

    case DIAL_OTHER:
        printf("%s: dialed number don't match!\n", __FILE__);
        rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_BUSY, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
        break;

    default:
        // nothing
        break;
    }
}

void channel_handle_onhook(channel_t *channel)
{
    char msg[BUFF_LEN];

    memset(msg, 0, sizeof(msg));
    printf("%s: channel state %s\n", __FILE__, channel_state[channel->state]);

    switch (channel->state)
    {
    case STATE_RING_REMOTE:
    case STATE_CONNECT:
        channel_rtp_stop(channel);
        if (channel->remote_chid >= 0 && channel->remote_chid < MAX_FXS) {
            sprintf(msg, "%s to %d from %d\n", BYE, channel->remote_chid, channel->chid);
            channle_msg_send(msg);
        }
        break;

    case STATE_RING_LOCAL:
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
    char msg[BUFF_LEN];

    memset(msg, 0, sizeof(msg));
    printf("%s: channel state %s\n", __FILE__, channel_state[channel->state]);

    /* active session 0, and record resource + 1 */
    rtk_SetTranSessionID(channel->chid, active_sid);

    switch (channel->state)
    {
    case STATE_IDLE:
        if (CHANNEL_IS_FXS(channel->chid)) {
            // fxs
            rtk_SetPlayTone(channel->chid, active_sid, DSPCODEC_TONE_DIAL, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        break;

    case STATE_RING_LOCAL:
        if (CHANNEL_IS_FXS(channel->chid))
        {
            // fxs
            rtk_SetRingFxs(channel->chid, DISABLE);            
            channel->state = STATE_CONNECT;
            channel_rtp_start(channel);

            printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);

            if (channel->remote_chid >= 0 && channel->remote_chid <= 7) {
                sprintf(msg, "%s to %d from %d\n", OK, channel->remote_chid, channel->chid);
                channle_msg_send(msg);
            }
        }
        break;

    case STATE_RING_REMOTE:
    default:
        break;
    }
}
 
int channel_socket_create(char *local_ip, char *remote_ip, char *conn_type)
{
    int ret = 0, chid = 0; 
    int num = 0, sock = 0;   

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("%s: create socket fail!\n", __FILE__);
        return -1;
    }

    printf("%s: local_ip:%s, remote_ip:%s, SOCK_PORT:%d\n", __FILE__, local_ip, remote_ip, SOCK_PORT);

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(local_ip);   
    local_addr.sin_port = htons(SOCK_PORT);             

    ret = bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if (ret < 0) {
        printf("%s: socket bind fail!\n", __FILE__);
        return -1;
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip);
    remote_addr.sin_port = htons(SOCK_PORT);

    if (strcmp(conn_type, "s") == 0)    /* server */
    {
        /* set callee phone number */
        for (chid=0, num=90; chid<MAX_FXS; chid++, num++) 
        {
            sprintf(funckey[chid], ".%d", num);
            printf("%s: funckey[%d]:%s\n", __FILE__, chid, funckey[chid]);
        }
    }
    else if (strcmp(conn_type, "c") == 0)   /* client */ 
    {
        /* set caller phone number */
        for (chid=0, num=80; chid<MAX_FXS; chid++, num++)
        {
            sprintf(funckey[chid], ".%d", num);
            printf("%s: funckey[%d]:%s\n", __FILE__, chid, funckey[chid]);
        }          
    }

    return sock;
}

int channel_msg_handle(void)
{
    int len, bytes;
    int chid = 0, remote_chid = 0; 
    channel_t *channel;
    char buf[BUFF_LEN]; 
    char *ptr = NULL; 
    char msg[BUFF_LEN];

    len = sizeof(remote_addr);
    memset(&remote_addr, 0, sizeof(remote_addr));
    memset(buf, 0, sizeof(buf));
    memset(msg, 0, sizeof(msg));

    bytes = recvfrom(sock_fd, buf, BUFF_LEN, 0, (struct sockaddr *)&remote_addr, (socklen_t *)&len);  
    if (bytes < 0) {   
        //printf("%s: receive from remote error!\n", __FILE__);
        return -1;
    }
    printf("%s: received msg:%s\n", __FILE__, buf);

    /* search the remote chid */
    ptr = strstr(buf, "from");
    if (ptr != 0) {
        ptr += 5;           
        remote_chid = atoi(ptr);
        //printf("%s: remote chid:%s\n", __FILE__, ptr);
    }
    else {           
        printf("%s: Not including <from>\n", __FILE__);
    }

    /* search active local channel id */
    ptr = strstr(buf, "to"); 
    if (ptr != NULL) {
        ptr += 3;       
        chid = atoi(ptr);
        //printf("%s: chid:%d, CH_NUM:%d\n", __FILE__, chid, CH_NUM);
        if (chid > (CH_NUM - 1)) {
            sprintf(msg, "%s to %d\n", BYE, remote_chid);
            channle_msg_send(msg);
            printf("%s: Not matched number!\n", __FILE__);
            return 0;
        }
    }
    else {
        printf("%s: Not including <to>!\n", __FILE__);
        return -1;
    }
             
    channel = &channels[chid];
     
    /* state */
    if (strstr(buf, INVITE) != NULL) 
    {     
        printf("%s: receive:%s\n", __FILE__, buf);
        printf("%s: channel state:%s\n", __FILE__, channel_state[channel->state]);
        rtk_SetRingFxs(chid, ENABLE);
        channel->remote_chid = remote_chid;
        channel->state = STATE_RING_LOCAL;
        printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);

        if (channel->remote_chid >= 0 && channel->remote_chid <= 7) {
            sprintf(msg, "%s to %d from %d\n", RINGING, channel->remote_chid, channel->chid);
            channle_msg_send(msg);
        }
    }
    else if (strstr(buf, RINGING) != NULL) 
    {
        printf("%s: receive:%s\n", __FILE__, buf);
        printf("%s: channela state:%s\n", __FILE__, channel_state[channel->state]);
        //rtk_SetPlayTone(chid, active_sid, DSPCODEC_TONE_RINGING, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
    }
    else if (strstr(buf, OK) != NULL) 
    {
        printf("%s receive:%s\n", __FILE__, buf); 
        printf("%s: channel state:%s\n", __FILE__, channel_state[channel->state]);     
        rtk_SetPlayTone(chid, active_sid, DSPCODEC_TONE_RINGING, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);
        channel->state = STATE_CONNECT;
        printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);
        /* create local rtp */
        channel_rtp_start(channel);        
    }
    else if (strstr(buf, BYE) != NULL) 
    {
        printf("%s: receive:%s\n", __FILE__, buf);
        printf("%s: channel state:%s\n", __FILE__, channel_state[channel->state]);
        if (channel->state == STATE_CONNECT) {
            rtk_SetPlayTone(chid, active_sid, DSPCODEC_TONE_BUSY, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
            channel->state = STATE_IDLE;
            channel_rtp_stop(channel);       
        }
        else if (channel->state == STATE_RING_REMOTE) {
            rtk_SetPlayTone(chid, active_sid, DSPCODEC_TONE_RINGING, DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);
            rtk_SetPlayTone(chid, active_sid, DSPCODEC_TONE_BUSY, ENABLE, DSPCODEC_TONEDIRECTION_LOCAL);
            channel->state = STATE_IDLE;
        }
        else if (channel->state == STATE_RING_LOCAL) {
            rtk_SetRingFxs(chid, DISABLE);
            channel->state = STATE_IDLE;
        }
        else {
            //nothing
        }
        printf("%s: channel state change to %s\n", __FILE__, channel_state[channel->state]);
    }
    else {
        //nothing
    }

    return 0;
}

int main(int argc, char **argv)
{
    int chid = 0;
    SIGNSTATE val;
    channel_t *channel;
    char src_ip[32];
    char dst_ip[32];
    struct timeval authtime;

    // Usage
    if (argc < 4) {
        printf("usage: %s <c/s> <src_ip> <dest_ip> [<tx_codec> <rx_codec>]\n", argv[0]);
        printf("       c:client, s:server\n");
        printf("       src_ip:local IP, dest_ip:remote IP\n");
        printf("       [<tx_codec> <rx_codec>] is optional, the default codec is g.711\n");
        return -1;
    }

    rtk_GetVoIPFeature();

    // init 
    for (chid=0; chid<CH_NUM; chid++)
    {
        channel = &channels[chid];

        // init dsp
        rtk_InitDsp(chid);

        // init phone
        rtk_SetVoiceGain(chid, 0, 0);              // 0db
        rtk_SetEchoTail(chid, 32, 1, 5, 5, 0);     // 4ms, NLP on

        // init FXS
        if (CHANNEL_IS_FXS(chid)) {
            // FXS
            rtk_SetFlashHookTime(chid, 100, 300,800);       // 100~300 ms
            rtk_SetDtmfCidParam(chid, DTMF_DIGIT_A, DTMF_DIGIT_C, 0xf, 80, 80, 300, 300);	// priori 1st ring, auto AC, auto ring, digit duration = 80ms, inter digit pause = 80ms
        }

        // set dtmf mode
        rtk_SetDtmfMode(chid, DTMF_INBAND);

        // set fax modem det mode		
        //rtk_SetFaxModemDet(chid, 3);   //3:auto_2

        // set fax event detection
        //rtk_SetAnswerToneDet(chid , 0x3f1212 , 0 , 10);

        // flush kernel fifo before app run
        rtk_SetFlushFifo(chid);

        // init local data
        channel_init(channel, chid);
               
        // re-init on hook 
        rtk_OnHookReinit(chid);

        rtk_SetPulseDigitDet(chid, 1, 450, 40, 80, 20, 60);  /* min_break_ths, max_break_ths, min_make_ths, max_make_ths*/
               
        // set the entered parameters      
        channel->ip = inet_addr(argv[2]);           //local ip
        channel->port = RTP_PORT;                   //local port      
        channel->remote_ip = inet_addr(argv[3]);    //remote ip
        channel->remote_port = RTP_PORT;            //remote port        
    }

    /* set codec */
    if (argc == 6) {
        tx_codec = atoi(argv[4]);
        rx_codec = atoi(argv[5]);
    }
    else {
        tx_codec = rtpPayloadPCMU;
        rx_codec = rtpPayloadPCMU;
    }
    
    rtk_SetCountryImpedance(TONE_TW);
    rtk_SetCountryTone(TONE_TW);
    
    /* TAPI debug */
    if (argc == 7 && strcmp(argv[6], "d") == 0) {
        printf("%s: enable TAPI debug\n", __FILE__);
        rtk_VoipOpenTapiDebug(atoi(argv[6]));
        printf("%s: rtk_debug(1)\n", __FILE__);
        rtk_debug(1); //kernel debug msg
    }
    
    /* create socket */
    strcpy(src_ip, argv[2]);
    strcpy(dst_ip, argv[3]);
    sock_fd = channel_socket_create(src_ip, dst_ip, argv[1]);
    if (sock_fd < 0) {
        printf("%s: channel_socket_create fail!\n", __FILE__);
        return -1;
    }

    while (1) 
    {
        authtime.tv_usec = 100000;  //100ms
        authtime.tv_sec = 0;
        FD_ZERO(&readfds);              
        FD_SET(sock_fd, &readfds);      

        if (select(sock_fd+1, &readfds, NULL, NULL, &authtime) < 0)
            return -1;

        if (FD_ISSET(sock_fd, &readfds) != 0) 
            channel_msg_handle();

        for (chid=0; chid<CH_NUM; chid++)
        {
            channel = &channels[chid];

            if (CHANNEL_IS_FXS(chid)) 
                rtk_GetFxsEvent(chid, &val);

            if (val >= SIGN_PULSE_KEY0 && val <= SIGN_PULSE_KEY9) 
                val -= SIGN_PULSE_KEY0;

            switch (val)
            {
            case SIGN_KEY0:
            case SIGN_KEY1:
            case SIGN_KEY2:
            case SIGN_KEY3:
            case SIGN_KEY4:
            case SIGN_KEY5:
            case SIGN_KEY6:
            case SIGN_KEY7:
            case SIGN_KEY8:
            case SIGN_KEY9:
            case SIGN_STAR:
            case SIGN_HASH:
                if (channel->state == STATE_IDLE) {
                    printf("%s: FXS Event: Key:%d, chid:%d\n", __FILE__, val, chid);
                    channel_handle_input(channel, val);
                }
                break;

            case SIGN_ONHOOK:
                printf("%s: FXS Event: ON-HOOK, chid:%d\n", __FILE__, chid);
                channel_handle_onhook(channel);
                /* rtk_OnHookAction have to be the last step in SIGN_ONHOOK */
                rtk_OnHookAction(chid);   
                break;

            case SIGN_OFFHOOK:
                printf("%s: FXS Event: OFF-HOOK, chid:%d\n", __FILE__, chid);
                /* rtk_OffHookAction have to be the first step in SIGN_OFFHOOK */
                rtk_OffHookAction(chid);
                channel_handle_offhook(channel);
                break;

            case SIGN_FLASHHOOK:
            case SIGN_OFFHOOK_2:    // off-hook but no event
            case SIGN_NONE:
                break;

            default:
                printf("%s: unknown(%d)\n", __FILE__, val);
                break;
            }
        }
    }   
    close(sock_fd);

    printf("%s: terminal! T_T\n", __FILE__);

    return 0;
}
