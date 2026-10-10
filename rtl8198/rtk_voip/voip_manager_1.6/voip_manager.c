#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
// fsk date & time sync
#include <time.h>
#include <sys/time.h>
// fsk date & time sync
#include "voip_manager.h"
#include "voip_feature.h"
#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
#ifdef CONFIG_RTK_VOIP_DECT_SITEL_SUPPORT
#include "si_dect_api.h"
#endif
#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
#include "cmbs_api.h"
#include "appcall.h"
#include "dect_test_scenario.h"
#endif
#endif

#ifdef CONFIG_RTK_VOIP_SRTP
#include "crypto_types.h"
#endif

#define SUPPORT_RTCP
		     /* define SUPPORT_RTCP to support RTCP.
                      * It also need to define it in rtk_voip.h for kernel space.
		      * Thlin add 2006-07-04
		      */

#include "voip_ioctl.h"

#ifndef MIN
#define MIN(x,y) (x>y?y:x)
#endif

#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30))
#define in6_u __in6_u
#define u6_addr8 __u6_addr8
#define u6_addr16 __u6_addr16
#define u6_addr32 __u6_addr32
#endif

/*
 *  TAPI Dump Debug Information 
 *  Define VOIP_TAPI_DBG_DUMP flag if user wants to debug TAPI
 */
#define VOIP_TAPI_DBG_DUMP
#define VOIP_TAPI_DUMP_USE_FOPEN
		#define TAPI_LOG_NAME   "/tmp/tapi_history.log"
		#define TAPI_LOG_BUF_SIZE 1024

		static char tapi_log_buf[TAPI_LOG_BUF_SIZE];
		static int taip_wr_cnt = 0;
static int open_tapi_debug=0;
		#ifdef  VOIP_TAPI_DUMP_USE_FOPEN
	static FILE *tapi_log_file = NULL;	
			#define TAPI_DUMP(fmt, args...) 												\
				do{																			\
			if (open_tapi_debug==2){														\
					taip_wr_cnt = snprintf(tapi_log_buf, sizeof(tapi_log_buf) , 			\
						"<<%s:%d>>\t" fmt, __FUNCTION__ , __LINE__ , ## args);				\
					if( (taip_wr_cnt>0) && (tapi_log_file!=NULL) ) {						\
						fwrite( tapi_log_buf , sizeof(char) , taip_wr_cnt , tapi_log_file );\
					fflush(tapi_log_file);\
					}																		\
			}else if (open_tapi_debug==1){fprintf(stderr,										\
					 "<<%s:%d>>\t" fmt, __FUNCTION__ , __LINE__ , ## args);}																\
				}while(0)
		#else
			static int tapi_log_file;
			#define TAPI_DUMP(fmt, args...) 												\
				do{																			\
					if (open_tapi_debug==2){													\
					taip_wr_cnt = snprintf(tapi_log_buf, sizeof(tapi_log_buf) , 			\
						"<<%s:%d>>\t" fmt, __FUNCTION__ , __LINE__ , ## args);				\
					if( (taip_wr_cnt>0) && (tapi_log_file!=0) ) {							\
						write( tapi_log_file , tapi_log_buf , taip_wr_cnt );				\
					}																		\
					}else if (open_tapi_debug==1){fprintf(stderr,										\
					 "<<%s:%d>>\t" fmt, __FUNCTION__ , __LINE__ , ## args);}
				}while(0)
		#endif

#ifdef VOIP_TAPI_DBG_DUMP
	static void dump_st_ToneCfgParam( TstVoipToneCfg *pstToneCfgParam );
#endif
#define TAPI_ERROR(fmt, args...) fprintf(stderr, 									\
	"<Error!!><<%s:%d>>\t" fmt, __FUNCTION__ , __LINE__ , ## args)

char * get_voip_event_name(VoipEventID evt);
static int32 rtk_OpenRtpSession(uint32 chid, uint32 sid);


#if 0//#ifndef __mips__
// use v400 + VE890 (2S1O) as default feature for x86
#define VOIP_X86_FEATURE ((uint64) 0xd00002efc0008021ULL)
VoipFeature_t g_VoIP_Feature = VOIP_X86_FEATURE;
uint32 g_VoIP_Ports = RTK_VOIP_CH_NUM( VOIP_X86_FEATURE );
#else
// voip feature from kernel config
VoipFeature_t g_VoIP_Feature = 0;
uint32 g_VoIP_Ports = 0;
#endif
//static int no_resource_flag[VOIP_CH_NUM]={0};
int g_VoIP_Mgr_FD = -1;

/* If DAA is used by CHn, set g_DAA_used[CHn]=1, otherwise, set g_DAA_used[CHn]=0 */
//int g_DAA_used[MAX_SLIC_NUM] = {0};

void rtk_VoipOpenTapiDebug(int enable)// 0 :disable 1: dump to console 2 : dump to FILE
{
	char file[256] = {0};
	open_tapi_debug=enable;
	if (enable!=0){
		snprintf(file, sizeof(file), "%s", TAPI_LOG_NAME);
		#ifdef VOIP_TAPI_DUMP_USE_FOPEN
		if( ( tapi_log_file = fopen( file , "wr+")) == NULL )
		#else
		if( ( tapi_log_file = open( file , O_RDWR | O_CREAT | O_APPEND)) < 0 )
		#endif
	        fprintf( stderr, "<<%s:%d>> create log file failed\n" , __FUNCTION__ , __LINE__ );
	}else
{
		if ( tapi_log_file ){
		#ifdef VOIP_TAPI_DUMP_USE_FOPEN
			fclose(tapi_log_file);
		#else
			close(tapi_log_file);
		#endif
		}
}
	return;
}

#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
int g_sip_register[CON_CH_NUM] = {[0 ... CON_CH_NUM-1] = 0};

#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT

#define DECT_LED 1

pthread_mutex_t         semDectEventFifoSem;

#define MUTEXOBTAIN_semDectEventFifoSem pthread_mutex_lock( &semDectEventFifoSem );
#define MUTEXRELEASE_semDectEventFifoSem pthread_mutex_unlock( &semDectEventFifoSem );

void dect_event_mutex_init(void)
{
	pthread_mutex_init(&semDectEventFifoSem,0);
}

typedef enum
{
	HS_OCCUPY_FAIL = 0,
	HS_OCCUPY_SUCC = 1,
	HS_OCCUPY_IDLE = 2
}
HS_ENT_ST;

typedef enum
{
    FSK_PARAM_NULL         = 0,         ///< 
    FSK_PARAM_DATEnTIME    = 0x01,      ///< Date and Time
    FSK_PARAM_CLI          = 0x02,      ///< Calling Line Identify (CLI)
    FSK_PARAM_CLI_ABS      = 0x04,      ///< Reason for absence of CLI
    FSK_PARAM_CLI_NAME     = 0x07,      ///< Calling Line Identify (CLI) Name
    FSK_PARAM_CLI_NAME_ABS = 0x08,      ///< Reason for absence of (CLI) Name
    FSK_PARAM_MW           = 0x0b,      ///< Message Waiting

}TfskParaType;

#define DECT_EVENT_FIFO_SIZE 20
#define MAX_DECT_CH_SIZE 8
#define MAX_LINE_SUPPORT 4
#define MAX_HS_SUPPORT 5
#define NONE -1
static char dect_event_fifo[MAX_DECT_CH_SIZE][DECT_EVENT_FIFO_SIZE];
static int dect_event_wp[MAX_DECT_CH_SIZE]={0}, dect_event_rp[MAX_DECT_CH_SIZE]={0};
static int LineOccupyByHS[MAX_LINE_SUPPORT] = {[0 ... MAX_LINE_SUPPORT-1] = NONE};
static HS_ENT_ST HsEntSt[MAX_HS_SUPPORT] = {[0 ... MAX_HS_SUPPORT-1] = HS_OCCUPY_IDLE};

static int dect_event_init(void)
{
	int i;

	for (i=0; i<MAX_DECT_CH_SIZE; i++)
	{
		dect_event_wp[i] = 0;
		dect_event_rp[i] = 0;
	}

	for (i=0; i<MAX_LINE_SUPPORT; i++)
		LineOccupyByHS[i] = NONE;

	for (i=0; i<MAX_HS_SUPPORT; i++)
		HsEntSt[i] = HS_OCCUPY_IDLE;

	return 0;
}

int dect_event_in(uint32 line_id, uint32 hs_id, char input)
{
	//printf("dect_event_in, ch%d\n", line_id);
	uint32 ch_id;
	int key;

	MUTEXOBTAIN_semDectEventFifoSem;

	if ( LineOccupyByHS[line_id] == NONE ) // Line is IDLE
	{
		if (HsEntSt[hs_id] == HS_OCCUPY_IDLE)
		{
			if (input == 1) //off-hook
			{
				LineOccupyByHS[line_id] = hs_id;
				HsEntSt[hs_id] = HS_OCCUPY_SUCC; //IDLE-->SUCC
				printf("[DECT]: line%d occupy by HS%d, HS%d =HS_OCCUPY_SUCC\n.", line_id, LineOccupyByHS[line_id], hs_id);
#if DECT_LED
#ifdef CONFIG_RTK_VOIP_GPIO_8954C_V400
				rtk_SetDectLED(line_id, 2); // blinking
#endif
#endif
			}
		}
		else if (HsEntSt[hs_id] == HS_OCCUPY_FAIL)
		{
			if (input ==0) //on-hook
			{
				HsEntSt[hs_id] = HS_OCCUPY_IDLE;
				input += 100;
			}
			else if (input == 1) //off-hook
				printf("Error! Shout not go to here: %s, line%d\n", __FUNCTION__, __LINE__);

			printf("[DECT]: line%d occupy by HS%d, HS%d =HS_OCCUPY_IDLE\n.", line_id, LineOccupyByHS[line_id], hs_id);
		}
		else if (HsEntSt[hs_id] == HS_OCCUPY_SUCC)
		{
			printf("Error! Shout not go to here: %s, line%d\n", __FUNCTION__, __LINE__);
		}
	}
	else if ( LineOccupyByHS[line_id] != NONE ) // Line is Occupy
	{
		// Line has been used by other HS in the same line group.
		if ( LineOccupyByHS[line_id] != hs_id )
		{
			if (input == 1) //off-hook
			{
				HsEntSt[hs_id] = HS_OCCUPY_FAIL; //IDLE-->FAIL
				printf("[DECT]: line%d occupy by HS%d, HS%d =HS_OCCUPY_FAIL\n.", line_id, LineOccupyByHS[line_id], hs_id);
			}
			else if (input ==0) //on-hook
			{
				HsEntSt[hs_id] = HS_OCCUPY_IDLE; //FAIL-->IDLE
				printf("[DECT]: line%d occupy by HS%d, HS%d =HS_OCCUPY_IDLE\n.", line_id, LineOccupyByHS[line_id], hs_id);
			}

			input += 100;

		}
		else
		{
			if (input == 0) //on-hook
			{
				LineOccupyByHS[line_id] = NONE;
				HsEntSt[hs_id] = HS_OCCUPY_IDLE;//SUCC-->IDLE
				printf("[DECT]: line%d is IDLE, HS%d =HS_OCCUPY_IDLE\n.", line_id, hs_id);
#if DECT_LED
#ifdef CONFIG_RTK_VOIP_GPIO_8954C_V400
				if (g_sip_register[line_id] == 1)
					rtk_SetDectLED(line_id, 1); // VoIP LED0 On
				else
					rtk_SetDectLED(line_id, 0); // VoIP LED0 Off
#endif
#endif
			}
		}
	}

	ch_id = line_id;

        if ((dect_event_wp[ch_id]+1)%DECT_EVENT_FIFO_SIZE != dect_event_rp[ch_id])
	{
	 	dect_event_fifo[ch_id][dect_event_wp[ch_id]] = input;
                dect_event_wp[ch_id] = (dect_event_wp[ch_id]+1) % DECT_EVENT_FIFO_SIZE;
	  	//printf("dect_event_wp=%d\n", dect_event_wp[ch_id]);

#if 1 // play DTMF tone by VoIP DSP
		if (input >= '0' && input <= '9')
		{
			key = input - '0';
			rtk_SetPlayTone(ch_id, 0, key, 1, 0);
		}
		else if (input == '*')
		{
			key = 10;//DSPCODEC_TONE_STARSIGN;
			rtk_SetPlayTone(ch_id, 0, key, 1, 0);
		}
		else if (input == '#')
		{
			key = 11;//DSPCODEC_TONE_HASHSIGN;
			rtk_SetPlayTone(ch_id, 0, key, 1, 0);
		}
#endif
	}
	else
	{
		printf("dect_event FIFO overflow,(%d)\n", ch_id);
	}

	MUTEXRELEASE_semDectEventFifoSem;

	return 0;
}


char dect_event_out(uint32 line_id)
{
	char output;
	uint32 ch_id;

	MUTEXOBTAIN_semDectEventFifoSem;

	ch_id = line_id;

	if ( dect_event_wp[ch_id] == dect_event_rp[ch_id]) // FIFO empty
	{
		output = 'Z';
		//printf("output = %d\n", output);
	}
	else
	{
		output = dect_event_fifo[ch_id][dect_event_rp[ch_id]];
                dect_event_rp[ch_id] = (dect_event_rp[ch_id]+1) % DECT_EVENT_FIFO_SIZE;
		//printf("dect_event_rp=%d\n", dect_event_rp[ch_id]);
		//printf("output = %d\n", output);
	}

	MUTEXRELEASE_semDectEventFifoSem;

	return output;
}

int32 rtk_GetLineOccupyHS(uint32 line_id)
{
	return LineOccupyByHS[line_id];
}

#endif //CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT

/*
 * @ingroup VOIP_DECT
 * @brief Set DECT power
 * @param power DECT power. 0: active, 1: inactive.
 * @retval 0 Success
 */
int32 rtk_SetDectPower( uint32 power )
{
	TstVoipSingleValue stVoipSingleValue;
	memset(&stVoipSingleValue, 0, sizeof(TstVoipSingleValue));
	stVoipSingleValue.value = power;

	SETSOCKOPT(VOIP_MGR_DECT_SET_POWER, &stVoipSingleValue, TstVoipSingleValue, 1);

	dect_event_init();

	return 0;
}

/*
 * @ingroup VOIP_DECT
 * @brief Get DECT power
 * @retval DECT power. 0: active, 1: inactive.
 */
int32 rtk_GetDectPower( void )
{
	TstVoipSingleValue stVoipSingleValue;
	memset(&stVoipSingleValue, 0, sizeof(TstVoipSingleValue));
	stVoipSingleValue.value = 0;

	SETSOCKOPT(VOIP_MGR_DECT_GET_POWER, &stVoipSingleValue, TstVoipSingleValue, 1);

	return stVoipSingleValue.value;
}

/*
 * @ingroup VOIP_DECT
 * @brief Get DECT page button
 * @retval DECT page. 0: active, 1: inactive.
 */
int32 rtk_GetDectPage( void )
{
	TstVoipSingleValue stVoipSingleValue;
	memset(&stVoipSingleValue, 0, sizeof(TstVoipSingleValue));
	stVoipSingleValue.value = 0;

	SETSOCKOPT(VOIP_MGR_DECT_GET_PAGE, &stVoipSingleValue, TstVoipSingleValue, 1);

	return stVoipSingleValue.value;
}

/*
 * @ingroup VOIP_DECT
 * @brief Get DECT button event
 * @retval DECT button event.
 * @sa VEID_DECT_BUTTON_PAGE VEID_DECT_BUTTON_REGISTRATION_MODE
 * @sa VEID_DECT_BUTTON_DEL_HS VEID_DECT_BUTTON_NOT_DEFINED
 */
int32 rtk_GetDectButtonEvent( void )
{
	int ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0, sizeof(TstVoipEvent));
	stVoipEvent.ch_id = 0;
	stVoipEvent.type = VET_DECT;
	stVoipEvent.mask = 0;
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;
	
	return stVoipEvent.id;
}

#if 0	//thlin disable DECT LED temp
/*
 * @ingroup VOIP_DECT
 * @brief Set DECT LED
 * @param state 0: LED off, 1: LED on, 2: LED blinking
 * @retval 0 Success
 */
int32 rtk_SetDectLED( int chid, char state )
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.value = state;

	SETSOCKOPT(VOIP_MGR_DECT_SET_LED, &stVoipValue, stVoipValue, 1);

	return stVoipValue.value;
}
#endif

/*
 * @ingroup VOIP_DECT
 * @brief Set DECT LED
 * @param state 0: LED off, 1: LED on, 2: LED blinking
 * @retval 0 Success
 */
int32 rtk_SetDectLED( int chid, char state )
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.value = state;

	SETSOCKOPT(VOIP_MGR_DECT_SET_LED, &stVoipValue, stVoipValue, 1);

	return stVoipValue.value;
}

#endif //CONFIG_RTK_VOIP_DRIVERS_ATA_DECT

int32 rtk_InitDsp(int ch)
{
	TAPI_DUMP("ch=%d\n" , ch);

	// init rtp session
	rtk_OpenRtpSession(ch, 0);
	rtk_OpenRtpSession(ch, 1);

	// reset rtp and codec
	rtk_SetRtpSessionState(ch, 0, rtp_session_inactive);
	rtk_SetRtpSessionState(ch, 1, rtp_session_inactive);

	// reset resource
	rtk_SetTranSessionID(ch, 255);

	// Always enable FXO pcm for Caller ID detection.
	//if (ch >= RTK_VOIP_SLIC_NUM(g_VoIP_Feature))
	if( RTK_VOIP_IS_DAA_CH( ch, g_VoIP_Feature ) )
		rtk_enable_pcm(ch, 1);

    return 0;
}

int32 rtk_SetRtpConfig(rtp_config_t *cfg)
{
    TstVoipMgrSession stVoipMgrSession;
	memset(&stVoipMgrSession, 0, sizeof(TstVoipMgrSession));
#ifdef VOIP_TAPI_DBG_DUMP
    if( cfg == NULL ){
    	TAPI_DUMP( "Input is NULL\n");
    }
    else{
		TAPI_DUMP( 	
			"chid=%u "
			"sid=%u "
			"remIp=%08x "
			"remPort=%hu "
			"extIp=%08x "
			"extPort=%hu "
			"ipv6=%u "
			"remIp6=%08x:%08x:%08x:%08x "
			"extIp6=%08x:%08x:%08x:%08x "
			"rfc2833_payload_type_local=%hu "
			"rfc2833_payload_type_remote=%hu "
			"rfc2833_fax_modem_pt_local=%hu "
			"rfc2833_fax_modem_pt_remote=%hu "
			"rtp_redundant_payload_type_local=%hu "
			"rtp_redundant_payload_type_remote=%hu "
			"rtp_redundant_max_Audio=%hu "
			"rtp_redundant_max_RFC2833=%hu "
			"SID_payload_type_local=%hu "
			"SID_payload_type_remote=%hu "
			"init_randomly=%hu "
			"init_seqno=%hu "
			"init_SSRC=%u "
#ifdef SUPPORT_VOICE_QOS
			"tos=%u "
#endif
			"init_timestamp=%u\n"
			, cfg->chid
			, cfg->sid
			, cfg->remIp  
			, cfg->remPort
			, cfg->extIp  
			, cfg->extPort
			, cfg->ipv6
			, cfg->remIp6.in6_u.u6_addr32[0] , cfg->remIp6.in6_u.u6_addr32[1] 
			, cfg->remIp6.in6_u.u6_addr32[2] , cfg->remIp6.in6_u.u6_addr32[3]
			, cfg->extIp6.in6_u.u6_addr32[0] , cfg->extIp6.in6_u.u6_addr32[1] 
			, cfg->extIp6.in6_u.u6_addr32[2] , cfg->extIp6.in6_u.u6_addr32[3]
			, cfg->rfc2833_payload_type_local       
			, cfg->rfc2833_payload_type_remote      
			, cfg->rfc2833_fax_modem_pt_local       
			, cfg->rfc2833_fax_modem_pt_remote      
			, cfg->rtp_redundant_payload_type_local 
			, cfg->rtp_redundant_payload_type_remote
			, cfg->rtp_redundant_max_Audio          
			, cfg->rtp_redundant_max_RFC2833        
			, cfg->SID_payload_type_local           
			, cfg->SID_payload_type_remote          
			, cfg->init_randomly                    
			, cfg->init_seqno                       
			, cfg->init_SSRC                        
#ifdef SUPPORT_VOICE_QOS
			, cfg->tos
#endif
			, cfg->init_timestamp 
		);
	}
   	
#endif


    stVoipMgrSession.ch_id = cfg->chid;
    stVoipMgrSession.m_id = cfg->sid;
    stVoipMgrSession.ip_src_addr = cfg->remIp;
    stVoipMgrSession.udp_src_port = cfg->remPort;
    stVoipMgrSession.ip_dst_addr = cfg->extIp;
    stVoipMgrSession.udp_dst_port = cfg->extPort;
#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
    stVoipMgrSession.ip6_src_addr = cfg->remIp6;
    stVoipMgrSession.ip6_dst_addr = cfg->extIp6;
	if( cfg->ipv6 != 0 && cfg->ipv6 != 1 ){
		printf("%s(%d) unexpected flag. use ipv4 by default.\n" , __FUNCTION__ , __LINE__);
    	stVoipMgrSession.ipv6 = 0;		// use ipv4 by default.
	}
	else{
    	stVoipMgrSession.ipv6 = cfg->ipv6;		
	}
#endif
#ifdef SUPPORT_VOICE_QOS
    stVoipMgrSession.tos = cfg->tos;
#endif
    stVoipMgrSession.rfc2833_dtmf_pt_local = cfg->rfc2833_payload_type_local;
    stVoipMgrSession.rfc2833_dtmf_pt_remote = cfg->rfc2833_payload_type_remote;
    stVoipMgrSession.rfc2833_fax_modem_pt_local = cfg->rfc2833_fax_modem_pt_local;
    stVoipMgrSession.rfc2833_fax_modem_pt_remote = cfg->rfc2833_fax_modem_pt_remote;
#ifdef CONFIG_RTK_VOIP_SRTP
	if(cfg->remoteCryptAlg == HMAC_SHA1){
		memcpy(stVoipMgrSession.remoteSrtpKey, cfg->remoteSrtpKey, SRTP_KEY_LEN);
		memcpy(stVoipMgrSession.localSrtpKey, cfg->localSrtpKey, SRTP_KEY_LEN);
		stVoipMgrSession.remoteCryptAlg = cfg->remoteCryptAlg;
	}
#endif /* CONFIG_RTK_VOIP_SRTP */
#if defined(SUPPORT_RTP_REDUNDANT_APP)
	stVoipMgrSession.rtp_redundant_payload_type_local = cfg ->rtp_redundant_payload_type_local;
	stVoipMgrSession.rtp_redundant_payload_type_remote = cfg ->rtp_redundant_payload_type_remote;
	stVoipMgrSession.rtp_redundant_max_Audio = cfg ->rtp_redundant_max_Audio;
	stVoipMgrSession.rtp_redundant_max_RFC2833 = cfg ->rtp_redundant_max_RFC2833;
#elif defined(SUPPORT_RTP_REDUNDANT)
	stVoipMgrSession.rtp_redundant_payload_type_local = 0;
	stVoipMgrSession.rtp_redundant_payload_type_remote = 0;
	stVoipMgrSession.rtp_redundant_max_Audio = 0;
	stVoipMgrSession.rtp_redundant_max_RFC2833 = 0;
#endif

#if 1
	stVoipMgrSession.SID_payload_type_local = cfg ->SID_payload_type_local;
	stVoipMgrSession.SID_payload_type_remote = cfg ->SID_payload_type_remote;
#else
	stVoipMgrSession.SID_payload_type_local = 0;
	stVoipMgrSession.SID_payload_type_remote = 0;
#endif

	stVoipMgrSession.init_randomly = cfg ->init_randomly;
	stVoipMgrSession.init_seqno = cfg ->init_seqno;
	stVoipMgrSession.init_SSRC = cfg ->init_SSRC;
	stVoipMgrSession.init_timestamp = cfg ->init_timestamp;
#ifdef DATA_OVER_TCP
	if(cfg->isTcp)
	{
		stVoipMgrSession.protocol = 0x6;
	}
#else 		
    if (cfg->isTcp)
    {
        printf("==> SUPPORT UDP ONLY\n");
        return -1;
    }
#endif
    else
    {
        stVoipMgrSession.protocol = 0x11;
    }

    SETSOCKOPT(VOIP_MGR_SET_SESSION, &stVoipMgrSession, TstVoipMgrSession, 1);

	return 0;//stVoipMgrSession.ret_val;
}

int32 rtk_SetQoSConfig(int chid, int sid, SessionType type, TstQosCfg *cfg){
	
	TstRtpQosRemark stRtpQosRemark;
	memset(&stRtpQosRemark, 0, sizeof(TstRtpQosRemark));
#ifdef VOIP_TAPI_DBG_DUMP
    if( cfg == NULL ){
    	TAPI_DUMP( "chid=%d sid=%d type=%d cfg is NULL\n" , chid , sid , type );
    }
    else{
		TAPI_DUMP("chid=%d sid=%d type=%d dscpRemark=%u vlanPriorityRemark=%u queueNum=%u\n",
			chid , sid , type , cfg->dscpRemark , cfg->vlanPriorityRemark , cfg->queueNum);   	
    }
#endif
	stRtpQosRemark.chid = chid;
	stRtpQosRemark.sid = sid;
	stRtpQosRemark.sessiontype = type;
	stRtpQosRemark.qos = *cfg;
	SETSOCKOPT(VOIP_MGR_QOS_CFG, &stRtpQosRemark, TstRtpQosRemark, 1);
	return 0;

}
int32 rtk_SetT38UdpConfig(t38udp_config_t* cfg)
{
    TstVoipMgrSession stVoipMgrSession;
	memset(&stVoipMgrSession, 0, sizeof(TstVoipMgrSession));
#ifdef VOIP_TAPI_DBG_DUMP
    if( cfg == NULL ){
    	TAPI_DUMP( "cfg is NULL\n");
    }
    else{
		TAPI_DUMP( 	
			"chid=%u "
			"sid=%u "
			"remIp=%08x "
			"remPort=%hu "
			"extIp=%08x "
			"extPort=%hu "
			"ipv6=%u "
			"remIp6=%08x:%08x:%08x:%08x "
			"extIp6=%08x:%08x:%08x:%08x "
			"\n"
			, cfg->chid
			, cfg->sid
			, cfg->remIp  
			, cfg->remPort
			, cfg->extIp  
			, cfg->extPort
			, cfg->ipv6
			, cfg->remIp6.in6_u.u6_addr32[0] , cfg->remIp6.in6_u.u6_addr32[1] 
			, cfg->remIp6.in6_u.u6_addr32[2] , cfg->remIp6.in6_u.u6_addr32[3]
			, cfg->extIp6.in6_u.u6_addr32[0] , cfg->extIp6.in6_u.u6_addr32[1] 
			, cfg->extIp6.in6_u.u6_addr32[2] , cfg->extIp6.in6_u.u6_addr32[3]
		);  	
    }
#endif


    stVoipMgrSession.ch_id = cfg->chid;
    stVoipMgrSession.m_id = cfg->sid;
    stVoipMgrSession.ip_src_addr = cfg->remIp;
    stVoipMgrSession.udp_src_port = cfg->remPort;
    stVoipMgrSession.ip_dst_addr = cfg->extIp;
    stVoipMgrSession.udp_dst_port = cfg->extPort;
#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
    stVoipMgrSession.ip6_src_addr = cfg->remIp6;
    stVoipMgrSession.ip6_dst_addr = cfg->extIp6;
	if( cfg->ipv6 != 0 && cfg->ipv6 != 1 ){
		printf("%s(%d) unexpected flag. use ipv4 by default.\n" , __FUNCTION__ , __LINE__);
    	stVoipMgrSession.ipv6 = 0;		// use ipv4 by default.
	}
	else{
    	stVoipMgrSession.ipv6 = cfg->ipv6;
	}
#endif
#ifdef SUPPORT_VOICE_QOS
    stVoipMgrSession.tos = cfg->tos;
#endif
 
	stVoipMgrSession.rfc2833_dtmf_pt_local = 0;
    stVoipMgrSession.rfc2833_dtmf_pt_remote = 0;
    stVoipMgrSession.rfc2833_fax_modem_pt_local = 0;
    stVoipMgrSession.rfc2833_fax_modem_pt_remote = 0;
#ifdef CONFIG_RTK_VOIP_SRTP
	if(cfg->remoteCryptAlg == HMAC_SHA1){
		memcpy(stVoipMgrSession.remoteSrtpKey, cfg->remoteSrtpKey, SRTP_KEY_LEN);
		memcpy(stVoipMgrSession.localSrtpKey, cfg->localSrtpKey, SRTP_KEY_LEN);
		stVoipMgrSession.remoteCryptAlg = 0;
		???
	}
#endif /* CONFIG_RTK_VOIP_SRTP */
#if defined(SUPPORT_RTP_REDUNDANT_APP)
	stVoipMgrSession.rtp_redundant_payload_type_local = 0;
	stVoipMgrSession.rtp_redundant_payload_type_remote = 0;
	stVoipMgrSession.rtp_redundant_max_Audio = 0;
	stVoipMgrSession.rtp_redundant_max_RFC2833 = 0;
#elif defined(SUPPORT_RTP_REDUNDANT)
	stVoipMgrSession.rtp_redundant_payload_type_local = 0;
	stVoipMgrSession.rtp_redundant_payload_type_remote = 0;
	stVoipMgrSession.rtp_redundant_max_Audio = 0;
	stVoipMgrSession.rtp_redundant_max_RFC2833 = 0;
#endif

	stVoipMgrSession.SID_payload_type_local = 0;
	stVoipMgrSession.SID_payload_type_remote = 0;
	
	stVoipMgrSession.init_randomly = 1;
	stVoipMgrSession.init_seqno = 0;
	stVoipMgrSession.init_SSRC = 0;
	stVoipMgrSession.init_timestamp = 0;
#ifdef DATA_OVER_TCP 
	if(cfg->isTcp)
	{
		stVoipMgrSession.protocol = 0x6;
	}
#else 	   
    if (cfg->isTcp)
    {
        printf("==> SUPPORT UDP ONLY\n");
        return -1;
    }
#endif
    else
    {
        stVoipMgrSession.protocol = 0x11;
    }

    SETSOCKOPT(VOIP_MGR_SET_SESSION, &stVoipMgrSession, TstVoipMgrSession, 1);

	return 0;//stVoipMgrSession.ret_val;
}

#if 0
/*
 * @ingroup VOIP_RTP_SESSION
 * @brief Get RTP configuration
 * @param chid The channel number.
 * @param sid The session number.
 * @param cfg The RTP configuration.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetRtpConfig(uint32 chid, uint32 sid, rtp_config_t *cfg)
{
    return -1;
}
#endif

int32 rtk_SetRtcpConfig(rtcp_config_t *cfg)	//thlin+ for Rtcp
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( cfg == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP(
			"chid=%u "
			"sid=%u "
			"remIp=%08x "
			"remPort=%hu "
			"extIp=%08x "
			"extPort=%hu "
			"remIp6=%08x:%08x:%08x:%08x "
			"extIp6=%08x:%08x:%08x:%08x "
			"ipv6=%u "
			"txInterval=%u "
			"enableXR=%u "
#ifdef SUPPORT_VOICE_QOS
			"tos=%u "
#endif
			"\n"
			, cfg->chid
			, cfg->sid
			, cfg->remIp
			, cfg->remPort
			, cfg->extIp
			, cfg->extPort
			, cfg->remIp6.in6_u.u6_addr32[0], cfg->remIp6.in6_u.u6_addr32[1]
			, cfg->remIp6.in6_u.u6_addr32[2], cfg->remIp6.in6_u.u6_addr32[3]
			, cfg->extIp6.in6_u.u6_addr32[0], cfg->extIp6.in6_u.u6_addr32[1]
			, cfg->extIp6.in6_u.u6_addr32[2], cfg->extIp6.in6_u.u6_addr32[3]
			, cfg->ipv6
			, cfg ->txInterval
			, cfg->enableXR
#ifdef SUPPORT_VOICE_QOS
			, cfg->tos
#endif
		);
	}
#endif
	
#ifdef SUPPORT_RTCP

    TstVoipRtcpSession stVoipRtcpSession;
	memset(&stVoipRtcpSession, 0, sizeof(TstVoipRtcpSession));
    stVoipRtcpSession.ch_id = cfg->chid;
    stVoipRtcpSession.m_id = cfg->sid;
    stVoipRtcpSession.ip_src_addr = cfg->remIp;
    stVoipRtcpSession.rtcp_src_port = cfg->remPort;
    stVoipRtcpSession.ip_dst_addr = cfg->extIp;
    stVoipRtcpSession.rtcp_dst_port = cfg->extPort;

#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	stVoipRtcpSession.ip6_src_addr = cfg->remIp6;
    stVoipRtcpSession.ip6_dst_addr = cfg->extIp6;
	if( cfg->ipv6 != 0 && cfg->ipv6 != 1 ){
		printf("%s(%d) unexpected flag. use ipv4 by default.\n" , __FUNCTION__ , __LINE__);
    	stVoipRtcpSession.ipv6 = 0; // use ipv4 by default.
	}
	else{
    	stVoipRtcpSession.ipv6 = cfg->ipv6;
	}
#endif
    
#ifdef SUPPORT_VOICE_QOS
	stVoipRtcpSession.tos = cfg ->tos;
#endif
    
	stVoipRtcpSession.tx_interval = cfg ->txInterval;
#ifdef CONFIG_RTK_VOIP_RTCP_XR
    stVoipRtcpSession.enableXR = cfg->enableXR;
#else
    stVoipRtcpSession.enableXR = 0;
#endif
    //if (cfg->isTcp)
    //{
    //    printf("==> SUPPORT UDP ONLY\n");
    //    return -1;
    //}
    //else
#ifdef DATA_OVER_TCP
	if(cfg->isTcp)
	{
		stVoipRtcpSession.protocol = 0x6;
	} 	
	else
#endif
    {
        stVoipRtcpSession.protocol = 0x11;
    }

    SETSOCKOPT(VOIP_MGR_SET_RTCP_SESSION, &stVoipRtcpSession, TstVoipRtcpSession, 1);

	//if (stVoipRtcpSession.ret_val != 0)
	//{
    //	return stVoipRtcpSession.ret_val;
    //}

#if 0	// merge into VOIP_MGR_SET_RTCP_SESSION
    TstVoipValue stVoipValue;
    stVoipValue.ch_id = cfg->chid;
    stVoipValue.value5 = rtcp_cfg->rtcp_tx_interval; /*unit: ms*/
    /* If rtcp_tx_interval is equal to 0, then RTCP Tx is disable.*/
    SETSOCKOPT(VOIP_MGR_SET_RTCP_TX_INTERVAL, &stVoipValue, TstVoipValue, 1);
#endif
	
	return 0;//stVoipValue.ret_val;

#else
    return 0;
#endif
}

int32 rtk_GetRtcpLogger( uint32 chid, uint32 sid, TstVoipRtcpLogger *logger )
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( logger == NULL ){
		TAPI_DUMP( "chid=%u sid=%u logger is NULL\n" , chid , sid );
	}
#endif

	logger ->ch_id = chid;
	logger ->m_id = sid;

    SETSOCKOPT(VOIP_MGR_GET_RTCP_LOGGER, logger, TstVoipRtcpLogger, 1);

#ifdef VOIP_TAPI_DBG_DUMP
	if( logger != NULL ){    
	TAPI_DUMP(
		"ch_id=%u "				
		"m_id=%u "              
		"TX_packet_count=%u "   
		"TX_XR_packet_count=%u "
		"TX_loss_rate_max=%u "  
		"TX_loss_rate_min=%u "  
		"TX_loss_rate_avg=%u "  
		"TX_loss_rate_cur=%u "  
		"TX_jitter_max=%u "     
		"TX_jitter_min=%u "     
		"TX_jitter_avg=%u "     
		"TX_jitter_cur=%u "     
		"TX_round_trip_max=%u " 
		"TX_round_trip_min=%u " 
		"TX_round_trip_avg=%u " 
		"TX_round_trip_cur=%u " 
		"TX_MOS_LQ_max=%u "     
		"TX_MOS_LQ_min=%u "     
		"TX_MOS_LQ_avg=%u "     
		"TX_MOS_LQ_cur=%u "     
		"TX_MOS_LQ_max_x10=%u "
		"TX_MOS_LQ_min_x10=%u "
		"TX_MOS_LQ_avg_x10=%u "
		"TX_MOS_LQ_cur_x10=%u "
		"RX_packet_count=%u "   
		"RX_XR_packet_count=%u "
		"RX_loss_rate_max=%u "  
		"RX_loss_rate_min=%u "  
		"RX_loss_rate_avg=%u "  
		"RX_loss_rate_cur=%u "  
		"RX_jitter_max=%u "     
		"RX_jitter_min=%u "     
		"RX_jitter_avg=%u "     
		"RX_jitter_cur=%u "     
		"RX_round_trip_max=%u " 
		"RX_round_trip_min=%u " 
		"RX_round_trip_avg=%u " 
		"RX_round_trip_cur=%u " 
		"RX_MOS_LQ_max=%u "     
		"RX_MOS_LQ_min=%u "     
		"RX_MOS_LQ_avg=%u "     
		"RX_MOS_LQ_cur=%u "     
		"RX_MOS_LQ_avg_x10=%u " 
		"\n"
		, logger->ch_id				
		, logger->m_id               
		, logger->TX_packet_count    
		, logger->TX_XR_packet_count 
		, logger->TX_loss_rate_max   
		, logger->TX_loss_rate_min   
		, logger->TX_loss_rate_avg   
		, logger->TX_loss_rate_cur   
		, logger->TX_jitter_max      
		, logger->TX_jitter_min      
		, logger->TX_jitter_avg      
		, logger->TX_jitter_cur      
		, logger->TX_round_trip_max  
		, logger->TX_round_trip_min  
		, logger->TX_round_trip_avg  
		, logger->TX_round_trip_cur  
		, logger->TX_MOS_LQ_max      
		, logger->TX_MOS_LQ_min      
		, logger->TX_MOS_LQ_avg      
		, logger->TX_MOS_LQ_cur      
		, logger->TX_MOS_LQ_max_x10 
		, logger->TX_MOS_LQ_min_x10 
		, logger->TX_MOS_LQ_avg_x10 
		, logger->TX_MOS_LQ_cur_x10	
		, logger->RX_packet_count    
		, logger->RX_XR_packet_count 
		, logger->RX_loss_rate_max   
		, logger->RX_loss_rate_min   
		, logger->RX_loss_rate_avg   
		, logger->RX_loss_rate_cur   
		, logger->RX_jitter_max      
		, logger->RX_jitter_min      
		, logger->RX_jitter_avg      
		, logger->RX_jitter_cur      
		, logger->RX_round_trip_max  
		, logger->RX_round_trip_min  
		, logger->RX_round_trip_avg  
		, logger->RX_round_trip_cur  
		, logger->RX_MOS_LQ_max      
		, logger->RX_MOS_LQ_min      
		, logger->RX_MOS_LQ_avg      
		, logger->RX_MOS_LQ_cur      
		, logger->RX_MOS_LQ_avg_x10
	);
	}
#endif

	return 0;
}

int32 rtk_SetRtpPayloadType(payloadtype_config_t *cfg)
{
    TstVoipPayLoadTypeConfig stVoipPayLoadTypeConfig;
	memset(&stVoipPayLoadTypeConfig, 0, sizeof(TstVoipPayLoadTypeConfig));
#ifdef VOIP_TAPI_DBG_DUMP
	if( cfg == NULL ){
		TAPI_DUMP( "cfg is NULL\n" );
	}
	else{
		TAPI_DUMP(
			"chid=%u "
			"sid=%u "
			"local_pt=%d "
			"remote_pt=%d "
			"uLocalPktFormat=%d "
			"uRemotePktFormat=%d "
			"nG723Type=%d "
			"nLocalFramePerPacket=%d "
			"nRemoteFramePerPacket=%d "
			"bVAD=%d "
			"bPLC=%d "
			"nJitterDelay=%u "
			"nMaxDelay=%u "
			"nMaxStrictDelay=%u "
			"nJitterFactor=%u "
			"nG726Packing=%u "
			"local_pt_vbd=%d "
			"remote_pt_vbd=%d "
			"uPktFormat_vbd=%d "
			"nFramePerPacket_vbd=%d "
			"nG7111Mode=%u "
			"nPcmMode=%u\n"
			,cfg->chid
			,cfg->sid
			,cfg->local_pt
			,cfg->remote_pt
			,cfg->uLocalPktFormat
			,cfg->uRemotePktFormat
			,cfg->nG723Type
			,cfg->nLocalFramePerPacket
			,cfg->nRemoteFramePerPacket
			,cfg->bVAD
			,cfg->bPLC
			,cfg->nJitterDelay
			,cfg->nMaxDelay
			,cfg->nMaxStrictDelay
			,cfg->nJitterFactor
			,cfg->nG726Packing
			,cfg->local_pt_vbd
			,cfg->remote_pt_vbd
			,cfg->uPktFormat_vbd	
			,cfg->nFramePerPacket_vbd
			,cfg->nG7111Mode
			,cfg->nPcmMode
		);	
	}
#endif

    stVoipPayLoadTypeConfig.ch_id = cfg->chid;
    stVoipPayLoadTypeConfig.m_id = cfg->sid;
    stVoipPayLoadTypeConfig.local_pt = cfg->local_pt;
    stVoipPayLoadTypeConfig.remote_pt = cfg->remote_pt;
    stVoipPayLoadTypeConfig.uLocalPktFormat = cfg->uLocalPktFormat;
    stVoipPayLoadTypeConfig.uRemotePktFormat = cfg->uRemotePktFormat;
    stVoipPayLoadTypeConfig.nG723Type = cfg->nG723Type;
    stVoipPayLoadTypeConfig.nLocalFramePerPacket = cfg->nLocalFramePerPacket;
    stVoipPayLoadTypeConfig.nRemoteFramePerPacket = cfg->nRemoteFramePerPacket;
    stVoipPayLoadTypeConfig.bVAD = cfg->bVAD;
    stVoipPayLoadTypeConfig.bPLC = cfg->bPLC;
    stVoipPayLoadTypeConfig.nJitterDelay = cfg->nJitterDelay;
    stVoipPayLoadTypeConfig.nMaxDelay = cfg->nMaxDelay;
    stVoipPayLoadTypeConfig.nMaxStrictDelay = cfg->nMaxStrictDelay;
    stVoipPayLoadTypeConfig.nJitterFactor = cfg->nJitterFactor;
    stVoipPayLoadTypeConfig.nG726Packing = cfg->nG726Packing;
    stVoipPayLoadTypeConfig.bT38ParamEnable = 0;
    stVoipPayLoadTypeConfig.nT38MaxBuffer = 0;
    stVoipPayLoadTypeConfig.nT38RateMgt = 0;
	stVoipPayLoadTypeConfig.nT38MaxRate = 0;
	stVoipPayLoadTypeConfig.bT38EnableECM = 0;
	stVoipPayLoadTypeConfig.nT38ECCSignal = 0;
	stVoipPayLoadTypeConfig.nT38ECCData = 0;
	stVoipPayLoadTypeConfig.bT38EnableSpoof = 0;
	stVoipPayLoadTypeConfig.nT38DuplicateNum = 0;

#if 1	// V.152 
    stVoipPayLoadTypeConfig.local_pt_vbd = cfg->local_pt_vbd;
    stVoipPayLoadTypeConfig.remote_pt_vbd = cfg->remote_pt_vbd;
    stVoipPayLoadTypeConfig.uPktFormat_vbd = cfg->uPktFormat_vbd;	
    stVoipPayLoadTypeConfig.nFramePerPacket_vbd = cfg->nFramePerPacket_vbd;
#elif 0	
    stVoipPayLoadTypeConfig.local_pt_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.remote_pt_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.uPktFormat_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.nFramePerPacket_vbd = 1;
#else
    stVoipPayLoadTypeConfig.local_pt_vbd = 96;
    stVoipPayLoadTypeConfig.remote_pt_vbd = 96;
    stVoipPayLoadTypeConfig.uPktFormat_vbd = rtpPayloadPCMU;
    stVoipPayLoadTypeConfig.nFramePerPacket_vbd = 2;
#endif
    stVoipPayLoadTypeConfig.nG7111Mode = cfg->nG7111Mode;
    stVoipPayLoadTypeConfig.nPcmMode = cfg->nPcmMode;

    SETSOCKOPT(VOIP_MGR_SETRTPPAYLOADTYPE, &stVoipPayLoadTypeConfig, TstVoipPayLoadTypeConfig, 1);

	return 0;//stVoipPayLoadTypeConfig.ret_val;
}

int32 rtk_SetT38PayloadType(t38_payloadtype_config_t *cfg)
{
    TstVoipPayLoadTypeConfig stVoipPayLoadTypeConfig;
	memset(&stVoipPayLoadTypeConfig, 0, sizeof(TstVoipPayLoadTypeConfig));
#ifdef VOIP_TAPI_DBG_DUMP
	if( cfg == NULL ){
		TAPI_DUMP( "cfg is NULL\n" );
	}
	else{
		TAPI_DUMP(
			"chid=%u "            
			"sid=%u "             
			"bT38ParamEnable=%u " 
			"nT38MaxBuffer=%u "   
			"nT38RateMgt=%u "     
			"nT38MaxRate=%u "     
			"bT38EnableECM=%u "   
			"nT38ECCSignal=%u "   
			"nT38ECCData=%u "     
			"bT38EnableSpoof=%u " 
			"nT38DuplicateNum=%u "
			"nPcmMode=%u "
			"\n"
			,cfg->chid
			,cfg->sid
			,cfg->bT38ParamEnable
			,cfg->nT38MaxBuffer
			,cfg->nT38RateMgt
			,cfg->nT38MaxRate
			,cfg->bT38EnableECM
			,cfg->nT38ECCSignal
			,cfg->nT38ECCData
			,cfg->bT38EnableSpoof
			,cfg->nT38DuplicateNum
			,cfg->nPcmMode
		);	
	}
#endif
	
    stVoipPayLoadTypeConfig.ch_id = cfg->chid;
    stVoipPayLoadTypeConfig.m_id = cfg->sid;
    stVoipPayLoadTypeConfig.local_pt = 0;
    stVoipPayLoadTypeConfig.remote_pt = 0;
    stVoipPayLoadTypeConfig.uLocalPktFormat = rtpPayloadT38_Virtual;
    stVoipPayLoadTypeConfig.uRemotePktFormat = rtpPayloadT38_Virtual;
    stVoipPayLoadTypeConfig.nG723Type = 0;
    stVoipPayLoadTypeConfig.nLocalFramePerPacket = 1;
    stVoipPayLoadTypeConfig.nRemoteFramePerPacket = 1;
    stVoipPayLoadTypeConfig.bVAD = 0;
    stVoipPayLoadTypeConfig.bPLC = 0;
    stVoipPayLoadTypeConfig.nJitterDelay = 4;
    stVoipPayLoadTypeConfig.nMaxDelay = 20;
    stVoipPayLoadTypeConfig.nMaxStrictDelay = -1;
    stVoipPayLoadTypeConfig.nJitterFactor = 1;
    stVoipPayLoadTypeConfig.nG726Packing = 0;
    stVoipPayLoadTypeConfig.bT38ParamEnable = cfg->bT38ParamEnable;
    stVoipPayLoadTypeConfig.nT38MaxBuffer = cfg->nT38MaxBuffer;
    stVoipPayLoadTypeConfig.nT38RateMgt = cfg->nT38RateMgt;
	stVoipPayLoadTypeConfig.nT38MaxRate = cfg->nT38MaxRate;
	stVoipPayLoadTypeConfig.bT38EnableECM = cfg->bT38EnableECM;
	stVoipPayLoadTypeConfig.nT38ECCSignal = cfg->nT38ECCSignal;
	stVoipPayLoadTypeConfig.nT38ECCData = cfg->nT38ECCData;
	stVoipPayLoadTypeConfig.bT38EnableSpoof = cfg->bT38EnableSpoof;
	stVoipPayLoadTypeConfig.nT38DuplicateNum = cfg->nT38DuplicateNum;

    stVoipPayLoadTypeConfig.local_pt_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.remote_pt_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.uPktFormat_vbd = rtpPayloadUndefined;
    stVoipPayLoadTypeConfig.nFramePerPacket_vbd = 1;
    
    stVoipPayLoadTypeConfig.nPcmMode = cfg->nPcmMode;

    SETSOCKOPT(VOIP_MGR_SETRTPPAYLOADTYPE, &stVoipPayLoadTypeConfig, TstVoipPayLoadTypeConfig, 1);

	return 0;//stVoipPayLoadTypeConfig.ret_val;
}

#if 0
/*
 * @ingroup VOIP_RTP_SESSION
 * @brief Get RTP payload type
 * @param chid The channel number.
 * @param sid The session number.
 * @param cfg The RTP payload configuration.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetRtpPayloadType(uint32 chid, uint32 sid, payloadtype_config_t *cfg)
{
    return -1;
}
#endif

int32 rtk_SetRtpSessionState(uint32 chid, uint32 sid, RtpSessionState state)
{
	TstVoipRtpSessionState stVoipRtpSessionState;
	memset(&stVoipRtpSessionState, 0, sizeof(TstVoipRtpSessionState));
	TAPI_DUMP("chid=%u sid=%d state=%d\n" , chid , sid , state);	
	stVoipRtpSessionState.ch_id = chid;
	stVoipRtpSessionState.m_id = sid;
	stVoipRtpSessionState.state = state;
	SETSOCKOPT(VOIP_MGR_SETRTPSESSIONSTATE, &stVoipRtpSessionState, TstVoipRtpSessionState, 1);

	//if (stVoipRtpSessionState.ret_val != 0)
	//{
	//	return stVoipRtpSessionState.ret_val;
	//}

    if ( rtp_session_inactive == state )   //disable both rx and tx
    {
		rtk_SetSessionInactive( chid, sid );
#ifdef DATA_OVER_TCP
		usleep(500000);
#endif
    }
    else
    	return 0;//stVoipRtpSessionState.ret_val;

    return 0;
}

int32 rtk_SetSessionInactive( uint32 chid, uint32 sid )
{
	TstVoipValue stVoipValue;
	TstVoipCfg stVoipCfg;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u sid=%d\n" , chid , sid );	
	stVoipCfg.ch_id = chid;
	stVoipCfg.m_id = sid;
	stVoipCfg.enable = 0;
	// close RTP
	SETSOCKOPT(VOIP_MGR_UNSET_SESSION, &stVoipCfg, TstVoipCfg, 1);
	//if (stVoipCfg.ret_val != 0)
	//{
	//	return stVoipCfg.ret_val;
	//}
	
#ifdef SUPPORT_RTCP
	// close RTCP
	SETSOCKOPT(VOIP_MGR_UNSET_RTCP_SESSION, &stVoipCfg, TstVoipCfg, 1); //thlin+ for Rtcp
	//if (stVoipCfg.ret_val != 0)
	//{
	//	return stVoipCfg.ret_val;
	//}
#endif

#ifdef DATA_OVER_TCP 
	SETSOCKOPT(VOIP_MGR_UNSET_TCP_SESSION, &stVoipCfg, TstVoipCfg, 1);
#endif
	// close Codec
	stVoipValue.ch_id = chid;
	stVoipValue.m_id = sid;
	SETSOCKOPT(VOIP_MGR_DSPCODECSTOP, &stVoipValue, TstVoipValue, 1);
	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}
	
	return 0;
}

int32 rtk_SetThresholdVadCng( int32 chid, int32 mid, int32 nThresVAD, int32 nThresCNG, int32 nModeSID, int32 nLevelSID, int32 nGainSID )
{
	TstVoipThresVadCngConfig stVoipThresVadCngConfig;
	memset(&stVoipThresVadCngConfig, 0, sizeof(TstVoipThresVadCngConfig));
	TAPI_DUMP(
		"chid=%u mid=%u nThresVAD=%d nThresCNG=%d"
		"nModeSID=%d nSIDLevel=%d nSIDGain=%d \n"
		,chid , mid , nThresVAD , nThresCNG 
		,nModeSID ,nLevelSID,nGainSID );


	if((nLevelSID > 127) || (nLevelSID < 0)){
		TAPI_ERROR("nLevelSID out of range Available range: 0~127 (0dBoV ~ -127dBoVm, 0dBov is maximum value). \n");
		return -1;
	}
	
	if((nGainSID > 127) || (nGainSID < -127)){
		TAPI_ERROR("nGainSID out of range Available range: +127~-127, 0(not change), the positive/negative value is gain up/down.\n");
		return -1;
	}
	stVoipThresVadCngConfig.ch_id = chid;
	stVoipThresVadCngConfig.m_id = mid;
	stVoipThresVadCngConfig.nThresVAD = nThresVAD;
	stVoipThresVadCngConfig.nThresCNG = nThresCNG;
	stVoipThresVadCngConfig.nSIDMode = nModeSID;
	stVoipThresVadCngConfig.nSIDLevel = nLevelSID;
	stVoipThresVadCngConfig.nSIDGain = nGainSID;	

	SETSOCKOPT(VOIP_MGR_SET_VAD_CNG_THRESHOLD, &stVoipThresVadCngConfig, TstVoipThresVadCngConfig, 1);

	return 0;
}

#if 0
/*
 * @ingroup VOIP_RTP_SESSION
 * @brief Get RTP Session State
 * @param chid The channel number.
 * @param sid The session number.
 * @param pstate The RTP Session State.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetRtpSessionState(uint32 chid, uint32 sid, RtpSessionState *pstate)
{
    return -1;
}
#endif

int g_bDisableRingFXS = 0;

int32 rtk_DisableRingFxs(int bDisable)
{
	TAPI_DUMP("bDisable=%d\n" , bDisable);
	g_bDisableRingFXS = bDisable;
	return 0;
}

int32 rtk_SetRingFxs(uint32 chid, uint32 bRinging)
{
    TstVoipSlicRing stVoipSlicRing;
	memset(&stVoipSlicRing, 0, sizeof(TstVoipSlicRing));
	TAPI_DUMP("chid=%d ring_set=%d g_bDisableRingFXS=%d\n" , chid , bRinging , g_bDisableRingFXS);

	/*
	 *	In "Auto SLIC Action" case, remember to call rtk_StopCid(), 
	 *	or SLIC ring action can't be stopped.
	 */

	if (g_bDisableRingFXS)
	{
		// quiet mode
		return 0;
	}

#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
	//if (chid < RTK_VOIP_DECT_NUM(g_VoIP_Feature))	//DECT Channel
	if( RTK_VOIP_IS_DECT_CH( chid, g_VoIP_Feature ) )
	{

#ifdef CONFIG_RTK_VOIP_DECT_SITEL_SUPPORT
		if( bRinging == 1 ) {
			dect_api_S2R_call_setup( 0, chid, "12345", "noname" );
		} else {
			if( dect_api_S2R_check_handset_ringing( chid ) )
				dect_api_S2R_call_release( chid );
		}
#endif

#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
		if (bRinging == 1)
		{
			CMBS_Api_InBound_Ring_CallerID(chid, NULL, NULL);
		}
		else
		{
			if(CMBS_Api_LineStateGet(chid) == 2)//ringing
			{
				CMBS_Api_CallRelease(chid, 0);
			}
		}
#endif
		//stVoipSlicRing.ret_val = 0;
	}
	else
#endif
	//SLIC Channel
	{
		stVoipSlicRing.ch_id = chid;
		stVoipSlicRing.ring_set = bRinging;
		SETSOCKOPT(VOIP_MGR_SLIC_RING, &stVoipSlicRing, TstVoipSlicRing, 1);
	}

	return 0;//stVoipSlicRing.ret_val;
}


#if 0
/*
 * @ingroup VOIP_PHONE_RING
 * @brief Check which channel is ringing
 * @param chid The channel number.
 * @param pRinging The ringing state.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetRingFXS(uint32 chid, uint32 *pRinging)
{
    return -1;
}
#endif

#ifdef SUPPORT_IVR_HOLD_TONE

#define G729_HOLD_TONE		/* borrow g723 code to play */

#ifdef G729_HOLD_TONE
#define IVRCODEC		"729"
#define IVRFRAMESIZE	10
#define IVRFILLTWICE
#else
#define IVRCODEC		"723"
#define IVRFRAMESIZE	24
#undef  IVRFILLTWICE
#endif

FILE *g_fpG723[ 2 ];
int gStartG723[ 2 ];

void InitG723IvrFile( void )
{
	int i;

	for( i = 0; i < 2; i ++ ) {

		if( g_fpG723[ i ] )
			fclose( g_fpG723[ i ] );

		/* place G.723 sample file in /bin/723_raw */
		if( ( g_fpG723[ i ] = fopen( "/usr/" IVRCODEC "_raw_yueme", "rb" ) ) == NULL )
			printf( "Open /usr/" IVRCODEC "_raw fail(%d)\n", i );
	}
}

void StartG723IvrFile( int chid )
{
	gStartG723[ chid ] = 1;
}

void StopG723IvrFile( int chid )
{
	gStartG723[ chid ] = 0;
	rtk_IvrStopPlaying( chid );
}

void RefillG723IvrFile( int chid, int sid, IvrPlayDir_t dir )
{
	/* Try to refill G723 IVR buffer periodically. */
	unsigned char buff723[ IVRFRAMESIZE * 10 ];
	unsigned int count723;
	unsigned int copied;
	FILE * const fp723 = g_fpG723[ chid ];
#ifdef IVRFILLTWICE
	unsigned int time = 2;
#endif

	if( !gStartG723[ chid ] )
		return;

#ifdef IVRFILLTWICE
	while( time -- )
#endif
	{
		count723 = fread( buff723, IVRFRAMESIZE, 10, fp723 );

		if( count723 ) {
	  #ifdef G729_HOLD_TONE
	  		copied = rtk_IvrStartPlayG729( chid, sid, dir, count723, buff723 );
	  #else
			copied = rtk_IvrStartPlayG72363( chid, sid, dir, count723, buff723 );
	  #endif

			if( count723 != copied )
				fseek( fp723, -IVRFRAMESIZE * ( int )( count723 - copied ), SEEK_CUR );
		}

		if( feof( fp723 ) )
			fseek( fp723, 0, SEEK_SET );
	}
}

#if 0
void main_program_pseudo_code( void )
{
	int chid;

	/* initialize once */
	InitG723IvrFile();

	while( 1 ) {
		/* receive message */
		/* Then, process this message or receive timeout. */

		for( chid = 0; chid < 2; chid ++ ) {

			/* refill IVR buffer in a certain period. */
			RefillG723IvrFile( chid, sid, IVR_DIR_LOCAL );

			/* You may play hold tone somewhere, but it will play IVR. */
			{
				rtk_SetPlayTone( chid, sid, DSPCODEC_TONE_HOLD, 1, path );
			}
		}
	}

}
#endif
#endif /* SUPPORT_IVR_HOLD_TONE */

int32 rtk_SetPlayTone(uint32 chid, uint32 sid, DSPCODEC_TONE nTone, uint32 bFlag,
	DSPCODEC_TONEDIRECTION Path)
{
    TstVoipPlayToneConfig cfg;
	memset(&cfg, 0, sizeof(TstVoipPlayToneConfig));
    TAPI_DUMP("ch_id=%u m_id=%u nTone=%d bFlag=%u path=%d \n", chid, sid, nTone, bFlag, Path);

#ifdef SUPPORT_IVR_HOLD_TONE
    static int bPrevHoldTone[ 2 ] = { 0, 0 };

    /* hold tone use ivr to play */
    if( nTone == DSPCODEC_TONE_HOLD && bFlag ) {
    	StartG723IvrFile( chid, sid );
    	bPrevHoldTone[ chid ] = 1;
    	printf( "StartG723IvrFile(%d)\n", chid );
    	return -1;	/* don't play tone */
    } else if( bPrevHoldTone[ chid ] ) {
    	StopG723IvrFile( chid, sid );
    	bPrevHoldTone[ chid ] = 0;
    	printf( "StopG723IvrFile(%d)\n", chid);
    }
#endif /* SUPPORT_IVR_HOLD_TONE */

	/* RING is incoming ring tone, and RINGING is ring back tone. */
    //if (nTone == DSPCODEC_TONE_RING)
    //    nTone = DSPCODEC_TONE_RINGING;

    cfg.ch_id = chid;
    cfg.m_id = sid;
    cfg.nTone = nTone;
    cfg.bFlag = bFlag;
    cfg.path = Path;

    SETSOCKOPT(VOIP_MGR_SETPLAYTONE, &cfg, TstVoipPlayToneConfig, 1);

	return 0;//cfg.ret_val;

}

#if 0
/*
 * @ingroup VOIP_PHONE_TONE
 * @brief Check tone of session
 * @param chid The channel number.
 * @param sid The session number.
 * @param pTone The tone type.
 * @param pFlag The tone state.
 * @param pPath The tone direction.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetPlayTone(uint32 chid, uint32 sid, DSPCODEC_TONE *pTone, uint32 *pFlag,
	DSPCODEC_TONEDIRECTION *pPath)
{
    return -1;
}
#endif

unsigned short rtk_VoIP_resource_check(uint32 chid, int payload_type)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u payload_type=%d\n" , chid , payload_type);	
	
	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = payload_type;
	SETSOCKOPT(VOIP_MGR_VOIP_RESOURCE_CHECK, &stVoipCfg, TstVoipCfg, 1);
	return stVoipCfg.enable;
}

int32 rtk_OnHookReinit(uint32 chid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u\n" , chid);	
	stVoipCfg.ch_id = chid;

	/* when phone onhook, re-init CED detection */
	SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;

}

#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
int32 rtk_GetDectEvent(uint32 chid, SIGNSTATE *pval)
{
#ifndef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
	*pval = SIGN_NONE;
	return 0;
#else
	TstVoipCfg stVoipCfg;
	TstVoipValue stVoipValue;
#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
	int32 event;
#endif

	*pval = SIGN_NONE;

#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
	event = rtk_GetDectButtonEvent();

	if (event == VEID_DECT_BUTTON_PAGE)
	{
		CMBS_Api_HandsetPage("all");
		//printf("page all HS\n");
	}
	else if (event == VEID_DECT_BUTTON_REGISTRATION_MODE)
	{
		CMBS_Api_RegistrationOpen();
		//printf("registration mode open\n");
	}
	else if (event == VEID_DECT_BUTTON_DEL_HS)
	{
		CMBS_Api_HandsetDelet("all");
		//printf("delete all registed HS\n");
	}
#endif

	stVoipValue.value = dect_event_out(chid);

	if ('Z' != stVoipValue.value)
	{

		// HS hook event
		if (stVoipValue.value == 0)
		{
			*pval = SIGN_ONHOOK;
			/* when phone onhook, stop play tone, disable pcm and DTMF detection */
			rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);
			usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!

			stVoipCfg.ch_id = chid;
			stVoipCfg.enable = 0;
			SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);
			/* when phone onhook, re-init DSP */
			SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);
		}
		else if (stVoipValue.value == 1 || stVoipValue.value == 2)
		{
#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
			CMBS_Api_OutboundAnswerCall(chid);
#endif

			*pval = SIGN_OFFHOOK;
			/* when phone offhook, enable pcm */
			stVoipCfg.ch_id = chid;
			//stVoipCfg.enable = 1;
			stVoipCfg.enable = stVoipValue.value;

			SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);
		}
		else if (stVoipValue.value == 3)	// pkshih: change 2 to 3
		{
			*pval = SIGN_FLASHHOOK;
		}
		// HS key event
		else if (stVoipValue.value == '*')
		{
			*pval = SIGN_STAR;
		}
		else if (stVoipValue.value == '#')
		{
			*pval = SIGN_HASH;
		}
		else if (stVoipValue.value == '0')
		{
			*pval = SIGN_KEY0;
		}
		else if (stVoipValue.value >= '1' && stVoipValue.value <= '9')
		{
			*pval = SIGN_KEY1 + stVoipValue.value - '1';
		}
		else if (stVoipValue.value == 100)
		{
			//DECT on-hook of the HS which not occupy line
			*pval = SIGN_NONE;
			printf("DECT on-hook of the HS which not occupy line\n");
		}
		else if (stVoipValue.value == 101)
		{
			//DECT off-hook of the HS which not occupy line
#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT
			CMBS_Api_OutboundAnswerCall(chid);
#endif
			*pval = SIGN_NONE;
			printf("DECT off-hook of the HS which not occupy line\n");
		}
		else
		{
			*pval = SIGN_NONE;
		}


		return 0;

	}
	else
	{
		//printf("==>UNKOWN SLIC STATUS (%d)\n", chid);   /* HOOK FIFO empty also belong to this case */
		return -1;
	}
#endif
}
#endif

int32 rtk_GetDtmfEvent(uint32 chid, uint32 dir, int8 *pEvent, int8 *pEnergy, int16 *pDuration)
{
	int32 ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( (pEvent == NULL) || (pEnergy == NULL) || (pDuration == NULL) ){
		TAPI_DUMP( "pEvent=%p pEnergy=%p pDuration=%p\n" , pEvent , pEnergy , pDuration );
	}
#endif

	*pEvent = 'X';
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_DTMF;
	stVoipEvent.mask = ( dir ? VEM_DIRIP : VEM_DIRTDM );
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;
	
	if( stVoipEvent.id == VEID_NONE )
		*pEvent = 'Z';
	else
		*pEvent = ( stVoipEvent.id & VEID_DTMF_DIGIT_MASK );
	
	*pEnergy = stVoipEvent.p0;
	*pDuration = stVoipEvent.p1;
	
	#ifdef VOIP_TAPI_DBG_DUMP
	if( *pEvent != 90 )
		TAPI_DUMP("chid=%u dir=%u event=%d energy=%d duration=%d\n" , 
					chid , dir , *pEvent , *pEnergy , *pDuration);	
	#endif
	
	return 0;
}

int32 rtk_GetSlicEvent(uint32 chid, SLICEVENT *pval)
{
	int32 ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	
	*pval = SLICEVENT_NONE;
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_HOOK;
	stVoipEvent.mask = 0;
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;

	if( stVoipEvent.id == VEID_HOOK_PHONE_STILL_OFF_HOOK )
	{
		*pval = SLICEVENT_OFFHOOK_2;
	}
	else if( stVoipEvent.id == VEID_HOOK_PHONE_OFF_HOOK )
	{
		*pval = SLICEVENT_OFFHOOK; // off-hook
	}
	else if( stVoipEvent.id == VEID_HOOK_PHONE_ON_HOOK )
	{
		*pval = SLICEVENT_ONHOOK;	// on-hook
	}
	else if( stVoipEvent.id == VEID_HOOK_PHONE_FLASH_HOOK )
	{
		*pval = SLICEVENT_FLASHHOOK;
	}
	else if( stVoipEvent.id == VEID_HOOK_PHONE_STILL_ON_HOOK )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_ON )
	{
		*pval = SLICEVENT_RING_ON;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_OFF )
	{
		*pval = SLICEVENT_RING_OFF;
	}
	else
	{
		printf("==>UNKOWN SLIC STATUS (%d)=%X\n", chid, stVoipEvent.id);   /* HOOK FIFO empty also belong to this case */
		return -1;
	}
	
	#ifdef VOIP_TAPI_DBG_DUMP
	if( (*pval!=0) && (*pval!=32) )
		TAPI_DUMP("chid=%u *pval=%u\n" , chid ,*pval);
	#endif
	
	return 0;
}

int32 rtk_GetFxsEvent(uint32 chid, SIGNSTATE *pval)
{

	TstVoipCfg stVoipCfg;
	TstDtmfDetPara stDtmfDetPara;
	int8 Event;
	int8 Energy;
	int16 Duration;
	int32 ret;
	SLICEVENT SlicEvent = SLICEVENT_NONE;

	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	memset(&stDtmfDetPara, 0x0, sizeof(TstDtmfDetPara));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	
	*pval = SIGN_NONE;

#ifdef NO_SLIC
	return 0;
#endif

	if( ( ret = rtk_GetSlicEvent( chid, &SlicEvent ) ) < 0 )
		return ret;
	
	if ( SlicEvent == SLICEVENT_OFFHOOK_2 ) /* PHONE_STILL_OFF_HOOK */
	{
		// detect DTMF
		rtk_GetDtmfEvent(chid, 0, &Event, &Energy, &Duration);
		
		if (Event == 'E' || Duration == -1) 
			printf("%s: dir=0, Event=%d, energey=%ddBFS, duration=%dms\n", __func__, Event, Energy, Duration);
		
        if (Event >= '0' && Event <= '9') 
        {
            if (Duration == -1) 
                *pval = SIGN_PULSE_KEY0 + Event - '0';
            else 
                *pval = SIGN_KEY0 + Event - '0';
        }
		else if (Event == '*')
			*pval = SIGN_STAR;
		else if (Event == '#')
		    *pval = SIGN_HASH;
		else
			*pval = SIGN_OFFHOOK_2;
		
		/* Just Polling IP side DTMF FIFO to prevent from FIFO overflow */
		rtk_GetDtmfEvent(chid, 1, &Event, &Energy, &Duration);
		
		if (Event == 'E' )
			printf("dir=1, energey=%ddBFS, duration=%dms\n", Energy, Duration);

		return 0;
	}
	else if ( SlicEvent == SLICEVENT_OFFHOOK ) /* PHONE_OFF_HOOK */
	{
		*pval = SIGN_OFFHOOK; // off-hook
#if 1
		/* when phone offhook, enable pcm and DTMF detection */
		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 1;//stVoipSlicHook.hook_status;

		SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

#if 0
		if (VOIP_RESOURCE_UNAVAILABLE == rtk_VoIP_resource_check(chid, NULL))
		{
			no_resource_flag[chid] = 1;
			*pval = 0;
			usleep(500000);//after phone off-hook, wait for a second,and then play IVR.
#if 1
			char text[]={IVR_TEXT_ID_NO_RESOURCE, '\0'};
			printf("play ivr (%d)...\n", chid);
			rtk_IvrStartPlaying( chid, sid, IVR_DIR_LOCAL, text );
#else
			rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, 1, DSPCODEC_TONEDIRECTION_LOCAL);
#endif
			return 0;
		}
#endif

		stDtmfDetPara.ch_id = chid;
    	stDtmfDetPara.enable = 1;
		stDtmfDetPara.dir = 0; /*dir0*/
		SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);
		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		stVoipCfg.enable = 1;
		/* when phone offhook, enable fax detection */
		SETSOCKOPT(VOIP_MGR_FAX_OFFHOOK, &stVoipCfg, TstVoipCfg, 1);

		return 0;//stVoipCfg.ret_val;

#endif
	}
	else if ( SlicEvent == SLICEVENT_ONHOOK ) /* PHONE_ON_HOOK */
	{
		*pval = SIGN_ONHOOK;	// on-hook
#if 1
		/* when phone onhook, stop play tone, disable pcm and DTMF detection */
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);
		usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!

		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 0;//stVoipSlicHook.hook_status;
		SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);
		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

#if 0
		if (no_resource_flag[chid] == 1)
		{
			no_resource_flag[chid] = 0;
			*pval = 0;
#if 1
			printf("stop play ivr(%d)...\n", chid);
			rtk_IvrStopPlaying(chid, 0);
#else
			rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, 0, DSPCODEC_TONEDIRECTION_LOCAL);
#endif
			return 0;
		}
#endif

		stDtmfDetPara.ch_id = chid;
    	stDtmfDetPara.enable = 0;
		stDtmfDetPara.dir = 0; /*dir0*/
		SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);
		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		/* when phone onhook, re-init CED detection */
		SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);
		return 0;//stVoipCfg.ret_val;

#endif
	}
	else if (SlicEvent == SLICEVENT_FLASHHOOK) /* PHONE_FLASH_HOOK */
	{
		*pval = SIGN_FLASHHOOK;
		return 0;
	}
#if 0
	else if (stVoipSlicHook.hook_status == 3) /* PHONE_STILL_ON_HOOK */
	{
		return 0;
	}
#endif
	else if (SlicEvent == SLICEVENT_RING_ON) /* FXO_RING_ON */
	{
		*pval = SIGN_RING_ON;
		return 0;
	}
	else if (SlicEvent == SLICEVENT_RING_OFF)  /* FXO_RING_OFF */
	{
		*pval = SIGN_RING_OFF;
		return 0;
	}
	
#ifdef VOIP_TAPI_DBG_DUMP
	if( *pval != SIGN_NONE )
		TAPI_DUMP("chid=%u *pval=%u\n" , chid ,*pval);
#endif
	
	return 0;
}

int32 rtk_ProcessFxsEvent(uint32 chid, VoipEventID evt, SIGNSTATE *pval)
{
	TstVoipCfg stVoipCfg;
	TstDtmfDetPara	stDtmfDetPara;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	memset(&stDtmfDetPara, 0x0, sizeof(TstDtmfDetPara));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u evt=0x%08x pval is NULL\n" , chid , evt );
	}
#endif

	*pval = SIGN_NONE;

#ifdef NO_SLIC
	return 0;
#endif

	if ( evt == VEID_DTMF_ENERGY )
	{
		//printf("dir=0, energey=%ddBFS, duration=%dms\n", Energy, Duration);
	}
	else if (evt >= VEID_DTMF_0 && evt <= VEID_DTMF_9 )
	{
		*pval = (evt & ~VET_DTMF) - 48; 
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if (evt == VEID_DTMF_STAR )
	{
		*pval = SIGN_STAR;
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if (evt == VEID_DTMF_SHARP )
	{
		*pval = SIGN_HASH;
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if (evt == VEID_HOOK_PHONE_FLASH_HOOK ) /* PHONE_FLASH_HOOK */
	{
		*pval = SIGN_FLASHHOOK;
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if (evt == VEID_HOOK_FXO_RING_ON) /* FXO_RING_ON */
	{
		*pval = SIGN_RING_ON;
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if (evt == VEID_HOOK_FXO_RING_OFF)  /* FXO_RING_OFF */
	{
		*pval = SIGN_RING_OFF;
		TAPI_DUMP("chid=%u evt=0x%08x *pval=%u\n" , chid , evt , *pval);
		return 0;
	}
	else if ( evt == VEID_HOOK_PHONE_OFF_HOOK ) /* PHONE_OFF_HOOK */
	{
		*pval = SIGN_OFFHOOK;
		TAPI_DUMP("chid=%u evt=0x%08x\n" , chid , evt);
		/* when phone offhook, enable pcm and DTMF detection */
		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 1;//stVoipSlicHook.hook_status;

		SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);

		stDtmfDetPara.ch_id = chid;
    	stDtmfDetPara.enable = 1;
		stDtmfDetPara.dir = 0; /*dir0*/
		SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);

		stVoipCfg.enable = 1;
		/* when phone offhook, enable fax detection */
		SETSOCKOPT(VOIP_MGR_FAX_OFFHOOK, &stVoipCfg, TstVoipCfg, 1);

		return 0;//stVoipCfg.ret_val;

	}
	else if ( evt == VEID_HOOK_PHONE_ON_HOOK ) /* PHONE_ON_HOOK */
	{
		*pval = SIGN_ONHOOK;
		TAPI_DUMP("chid=%u evt=0x%08x\n" , chid , evt);
		
		/* when phone onhook, stop play tone, disable pcm and DTMF detection */
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);
		usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!

		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 0;//stVoipSlicHook.hook_status;
		SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);

		stDtmfDetPara.ch_id = chid;
    	stDtmfDetPara.enable = 0;
		stDtmfDetPara.dir = 0; /*dir0*/
		SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);

		/* when phone onhook, re-init CED detection */
		SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);
		return 0;//stVoipCfg.ret_val;
	}


	return 0;
}

int32 rtk_GetDaaEvent(uint32 chid, DAAEVENT *pval)
{
	int32 ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	
	*pval = DAAEVENT_NONE;
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_HOOK;
	stVoipEvent.mask = 0;
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;
	
	if( stVoipEvent.id == VEID_HOOK_FXO_STILL_OFF_HOOK )
	{
		*pval = DAAEVENT_OFFHOOK_2;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_ON )
	{
		*pval = DAAEVENT_RING_ON; // off-hook
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_OFF )
	{
		*pval = DAAEVENT_RING_STOP;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_BUSY_TONE )
	{
		*pval = DAAEVENT_BUSY_TONE;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_DIS_TONE )
	{
		*pval = DAAEVENT_DIST_TONE;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_OFF_HOOK )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_ON_HOOK )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_FLASH_HOOK )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_STILL_ON_HOOK )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_CALLER_ID )
	{
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_POLARITY_REVERSAL )
	{
		//printf("Get FXOEVENT_POLARITY_REVERSAL, ch=%d\n", chid);
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_CURRENT_DROP )
	{
		//printf("Get FXOEVENT_BAT_DROP_OUT, ch=%d\n", chid);
	}
	else
	{
		printf("==>UNKOWN FXO STATUS (%d) = %d\n", chid, stVoipEvent.id);
		return -1;
	}
	
	TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
	
	return 0;
}

int32 rtk_GetFxoEvent(uint32 chid, SIGNSTATE *pval)
{

	TstVoipCfg stVoipCfg;
	int8 Event;
	int8 Energy;
	int16 Duration;
	DAAEVENT DaaEvent = DAAEVENT_NONE;
	int ret;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif

	*pval = SIGN_NONE;

	if( ( ret = rtk_GetDaaEvent( chid, &DaaEvent ) ) < 0 )
		return ret;

	if ( DaaEvent == DAAEVENT_OFFHOOK_2 ) /* FXO_STILL_OFF_HOOK */
	{
		// detect DTMF
		rtk_GetDtmfEvent(chid, 0, &Event, &Energy, &Duration);
		
		if (Event == 'E' )
            printf("%s: dir=0, energey=%ddBFS, duration=%dms\n", __func__, Energy, Duration);
		
		if (Event >= '1' && Event <= '9')
			*pval = SIGN_KEY1 + Event - '1';
		else if (Event == '0')
			*pval = SIGN_KEY0;
		else if (Event == '*')
			*pval = SIGN_STAR;
		else if (Event == '#')
		    *pval = SIGN_HASH;
		else
			*pval = SIGN_OFFHOOK_2;
		
		/* Just Polling IP side DTMF FIFO to prevent from FIFO overflow */
		rtk_GetDtmfEvent(chid, 1, &Event, &Energy, &Duration);
		
		if (Event == 'E' )
			printf("dir=1, energey=%ddBFS, duration=%dms\n", Energy, Duration);
		
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if ( DaaEvent == DAAEVENT_RING_ON ) /* FXO_RING_ON */
	{
		*pval = SIGN_OFFHOOK; // off-hook

#if 0
		if (VOIP_RESOURCE_UNAVAILABLE == rtk_VoIP_resource_check(chid, NULL))
		{
			no_resource_flag[chid] = 1;
			*pval = 0;
#ifdef DAA_IVR
			usleep(500000);//after phone off-hook, wait for a second,and then play IVR.
			char text[]={IVR_TEXT_ID_NO_RESOURCE, '\0'};
			printf("play ivr...\n");
			rtk_FxoOffHook(chid);
			rtk_IvrStartPlaying( chid, 0, IVR_DIR_LOCAL, text );
			//rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, 1, DSPCODEC_TONEDIRECTION_LOCAL);
#else
#endif
			return 0;
		}
#endif

		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 1;
		 /* when phone offhook, enable fax detection */
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		SETSOCKOPT(VOIP_MGR_FAX_OFFHOOK, &stVoipCfg, TstVoipCfg, 1);
		return 0;//stVoipCfg.ret_val;
	}
	else if ( 
			DaaEvent == DAAEVENT_RING_STOP ||	 /* FXO_RING_OFF */
			DaaEvent == DAAEVENT_BUSY_TONE ||	/* FXO_BUSY_TONE */
			DaaEvent == DAAEVENT_DIST_TONE )   /* FXO_DIS_TONE */
	{
       	*pval = SIGN_ONHOOK;	// on-hook
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);
		usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!

#if 0
		if (no_resource_flag[chid] == 1)
		{
			no_resource_flag[chid] = 0;
			*pval = 0;
#ifdef DAA_IVR
			printf("stop play ivr...\n");
			rtk_IvrStopPlaying(chid, 0);
			rtk_FxoOnHook(chid);
#endif
			//rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, 0, DSPCODEC_TONEDIRECTION_LOCAL);
			return 0;
		}
#endif

		/* when phone onhook, re-init CED detection */
		stVoipCfg.ch_id = chid;
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);
		return 0;//stVoipCfg.ret_val;
	}
#if 0
	else if (stVoipSlicHook.hook_status == 7) /* FXO_OFF_HOOK */
	{
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 6) /* FXO_ON_HOOK */
	{
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 8) /* FXO_FLASH_HOOK */
	{
		*pval = SIGN_FLASHHOOK;	// flash-hook
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 9) /* FXO_STILL_ON_HOOK */
	{
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 14) /* FXO_CALLER_ID */
	{
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 17)   /* FXO_POLARITY_REVERSAL*/
	{
		//printf("Get FXOEVENT_POLARITY_REVERSAL, ch=%d\n", chid);
		return 0;
	}
	else if (stVoipSlicHook.hook_status == 18)   /* FXO_CURRENT_DROP*/
	{
		//printf("Get FXOEVENT_BAT_DROP_OUT, ch=%d\n", chid);
		return 0;
	}
#endif

	return 0;
}


int32 rtk_ProcessFxoEvent(uint32 chid, VoipEventID evt, SIGNSTATE *pval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u evt=0x%08x Input is NULL\n" , chid , evt );
	}
#endif

	*pval = SIGN_NONE;

	if ( evt == VEID_DTMF_ENERGY )
	{
		//printf("dir=0, energey=%ddBFS, duration=%dms\n", Energy, Duration);
	}
	else if (evt >= VEID_DTMF_1 && evt <= VEID_DTMF_9 )
	{
		*pval = (evt & ~VET_DTMF) - '0'; 
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if (evt == VEID_DTMF_0 )
	{
		*pval = SIGN_KEY0;
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if (evt == VEID_DTMF_STAR )
	{
		*pval = SIGN_STAR;
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if (evt == VEID_DTMF_SHARP )
	{
		*pval = SIGN_HASH;
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if (evt == VEID_HOOK_PHONE_FLASH_HOOK ) /* PHONE_FLASH_HOOK */
	{
		*pval = SIGN_FLASHHOOK;
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if ( evt == VEID_HOOK_FXO_RING_ON ) /* FXO_RING_ON */
	{
		*pval = SIGN_OFFHOOK; // off-hook

		stVoipCfg.ch_id = chid;
		stVoipCfg.enable = 1;
		 /* when phone offhook, enable fax detection */
		SETSOCKOPT(VOIP_MGR_FAX_OFFHOOK, &stVoipCfg, TstVoipCfg, 1);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;//stVoipCfg.ret_val;
	}
	else if ( 
			evt == VEID_HOOK_FXO_RING_OFF ||	/* FXO_RING_OFF */
			evt == VEID_HOOK_FXO_BUSY_TONE ||	/* FXO_BUSY_TONE */
			evt == VEID_HOOK_FXO_DIS_TONE )   	/* FXO_DIS_TONE */
	{
       	*pval = SIGN_ONHOOK;	// on-hook
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, RTK_DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);
		usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!

		/* when phone onhook, re-init CED detection */
		stVoipCfg.ch_id = chid;
		SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;//stVoipCfg.ret_val;
	}

	return 0;
}

#ifdef DATA_OVER_TCP
int rtk_CheckTCPEvent(uint32 chid)
{
	int32 ret = 0;
	TstVoipEvent stVoipEvent;

#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u \n" , chid);
#endif
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_TCP;
	stVoipEvent.mask = 0;

	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 ){
		TAPI_DUMP("[%s]Get TCP Event fail, ret = %d.\n", __FUNCTION__, ret);
		return ret;
	}

	if( stVoipEvent.id == VEID_TCP_FAILURE_CONNECT ){
		TAPI_DUMP("[%s]unset ch-%u all sessions.\n", __FUNCTION__, chid);
		rtk_SetRtpSessionState(chid, 0, rtp_session_inactive);
		rtk_SetRtpSessionState(chid, 1, rtp_session_inactive);
	}
	return ret;
}
#endif

int32 rtk_GetRealFxoEvent(uint32 chid, FXOEVENT *pval)
{
	int32 ret;
	TstVoipEvent stVoipEvent;
	int8 Event;
	int8 Energy;
	int16 Duration;
	memset(&stVoipEvent, 0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	
	*pval = FXOEVENT_NONE;
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_HOOK;
	stVoipEvent.mask = 0;
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;

	if( stVoipEvent.id == VEID_HOOK_FXO_STILL_OFF_HOOK )
	{
		// detect DTMF
		rtk_GetDtmfEvent(chid, 0, &Event, &Energy, &Duration);
		
		if (Event == 'E' )
			printf("%s: dir=0, energey=%ddBFS, duration=%dms\n", __func__, Energy, Duration);
		
		if (Event >= '1' && Event <= '9')
			*pval = SIGN_KEY1 + Event - '1';
		else if (Event == '0')
			*pval = SIGN_KEY0;
		else if (Event == '*')
			*pval = SIGN_STAR;
		else if (Event == '#')
		    *pval = SIGN_HASH;
		else
			*pval = SIGN_OFFHOOK_2;
		
		/* Just Polling IP side DTMF FIFO to prevent from FIFO overflow */
		rtk_GetDtmfEvent(chid, 1, &Event, &Energy, &Duration);

		if (Event == 'E' )
			printf("dir=1, energey=%ddBFS, duration=%dms\n", Energy, Duration);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_ON )
	{
		*pval = FXOEVENT_RING_START;
		printf("Get FXOEVENT_RING_START, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_RING_OFF )
	{
		*pval = FXOEVENT_RING_STOP;
		printf("Get FXOEVENT_RING_STOP, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_BUSY_TONE )
	{
		*pval = FXOEVENT_BUSY_TONE;
		printf("Get FXOEVENT_BUSY_TONE, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_POLARITY_REVERSAL )
	{
		*pval = FXOEVENT_POLARITY_REVERSAL;
		printf("Get FXOEVENT_POLARITY_REVERSAL, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_CURRENT_DROP )
	{
		*pval = FXOEVENT_BAT_DROP_OUT;
		printf("Get FXOEVENT_BAT_DROP_OUT, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_DIS_TONE )
	{
		*pval = FXOEVENT_DIST_TONE;
		printf("Get FXOEVENT_DIST_TONE, ch=%d\n", chid);
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_OFF_HOOK )
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_ON_HOOK )
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_FLASH_HOOK )
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_STILL_ON_HOOK )
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else if( stVoipEvent.id == VEID_HOOK_FXO_CALLER_ID )
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		return 0;
	}
	else
	{
		TAPI_DUMP("chid=%u *pval=%u\n" , chid , *pval);
		printf("==>UNKOWN FXO STATUS (%d) = %d\n", chid, stVoipEvent.id);
		return -1;
	}
}

int32 rtk_GetVoIPEvent( TstVoipEvent *stVoipEvent )
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( stVoipEvent == NULL ){
		TAPI_DUMP( "stVoipEvent is NULL\n" );
	}
#endif

	SETSOCKOPT( VOIP_MGR_GET_VOIP_EVENT, stVoipEvent, TstVoipEvent, 1 );

#ifdef VOIP_TAPI_DBG_DUMP
	if( (stVoipEvent->id != VEID_NONE) && 
		(stVoipEvent->id != VEID_HOOK_PHONE_STILL_ON_HOOK) && 
		(stVoipEvent->id != VEID_HOOK_PHONE_STILL_OFF_HOOK)){
		TAPI_DUMP("id=0x%08x %s\n" , stVoipEvent->id , get_voip_event_name(stVoipEvent->id));
		//fprintf(stderr , "%s(%d) id=0x%08x %s\n"  , __FUNCTION__ , __LINE__ , stVoipEvent->id , get_voip_event_name(stVoipEvent->id));
	}
#endif
	
	return 0;
}

int32 rtk_FlushVoIPEvent( TstFlushVoipEvent *stFlushVoipEvent )
{
	SETSOCKOPT( VOIP_MGR_FLUSH_VOIP_EVENT, stFlushVoipEvent, TstFlushVoipEvent, 1 );
	
	return 0;
}

/*
 * @ingroup VOIP_RTP_SESSION
 * @brief Init the RTP session
 * @param chid The channel number.
 * @param sid The session number.
 * @retval 0 Success
 * @note internal use
 */
int32 rtk_OpenRtpSession(uint32 chid, uint32 sid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u sid=%u\n" , chid , sid);	
	
	stVoipCfg.ch_id = chid;
	stVoipCfg.m_id = sid;
	stVoipCfg.enable = 1;
	SETSOCKOPT(VOIP_MGR_CTRL_RTPSESSION, &stVoipCfg, TstVoipCfg, 1);
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetTranSessionID(uint32 chid, uint32 sid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u sid=%u\n" , chid , sid);	
	
	stVoipCfg.ch_id = chid;
	stVoipCfg.m_id = sid;
	stVoipCfg.enable = 1;
	SETSOCKOPT(VOIP_MGR_CTRL_TRANSESSION_ID, &stVoipCfg, TstVoipCfg, 1);
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetTranSessionID(uint32 chid, uint32* psid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( psid == NULL ){
		TAPI_DUMP( "psid is NULL\n" );
	}
#endif
	
	stVoipCfg.ch_id = chid;
//        stVoipCfg.m_id = sid;
	stVoipCfg.enable = 0;
	SETSOCKOPT(VOIP_MGR_CTRL_TRANSESSION_ID, &stVoipCfg, TstVoipCfg, 1);
	*psid = stVoipCfg.t_id;
	TAPI_DUMP("chid=%u psid=%u\n" , chid , *psid);
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetConference(TstVoipMgr3WayCfg *stVoipMgr3WayCfg)
{
#ifdef VOIP_TAPI_DBG_DUMP
	int i=0;

	if( stVoipMgr3WayCfg == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP("ch_id=%u enable=%u\n" , stVoipMgr3WayCfg->ch_id , stVoipMgr3WayCfg->enable);

		for( i=0 ; i<2 ; i++ ){
			TAPI_DUMP( 	
			"ch_id=%u "			
			"m_id=%u "			
			"ip_src_addr=%u "		
			"ip_dst_addr=%u "		
			"udp_src_port=%u "	
			"udp_dst_port=%u "	
			"rtcp_src_port=%u "	
			"rtcp_dst_port=%u "	
			"protocol=%u "		
			"tos=%u "				
			"rfc2833_payload_type_local=%u "	
			"rfc2833_payload_type_remote=%u "	
			"local_pt=%d " 		
			"remote_pt=%d " 		
			"uLocalPktFormat=%d " 		
			"uRemotePktFormat=%d " 		
			"local_pt_vbd=%d " 			
			"remote_pt_vbd=%d " 			
			"uPktFormat_vbd=%d " 		
			"nG723Type=%d "				
			"nLocalFramePerPacket=%d "	
			"nRemoteFramePerPacket=%d "	
			"nFramePerPacket_vbd=%d "	
			"bVAD=%d "			
			"bPLC=%d "			
			"nJitterDelay=%d "	
			"nMaxDelay=%d "		
			"nMaxStrictDelay=%d "
			"nJitterFactor=%d "	
			"state=%d "			
			"result=%d "			
			"ip6_src_addr=%08x:%08x:%08x:%08x "
			"ip6_dst_addr=%08x:%08x:%08x:%08x "
			"ipv6=%u "
			"\n"			
			,stVoipMgr3WayCfg->rtp_cfg[i].ch_id			
			,stVoipMgr3WayCfg->rtp_cfg[i].m_id			
			,stVoipMgr3WayCfg->rtp_cfg[i].ip_src_addr		
			,stVoipMgr3WayCfg->rtp_cfg[i].ip_dst_addr		
			,stVoipMgr3WayCfg->rtp_cfg[i].udp_src_port	
			,stVoipMgr3WayCfg->rtp_cfg[i].udp_dst_port	
			,stVoipMgr3WayCfg->rtp_cfg[i].rtcp_src_port	
			,stVoipMgr3WayCfg->rtp_cfg[i].rtcp_dst_port	
			,stVoipMgr3WayCfg->rtp_cfg[i].protocol		
			,stVoipMgr3WayCfg->rtp_cfg[i].tos				
			,stVoipMgr3WayCfg->rtp_cfg[i].rfc2833_payload_type_local	
			,stVoipMgr3WayCfg->rtp_cfg[i].rfc2833_payload_type_remote	
			,stVoipMgr3WayCfg->rtp_cfg[i].local_pt 		
			,stVoipMgr3WayCfg->rtp_cfg[i].remote_pt 		
			,stVoipMgr3WayCfg->rtp_cfg[i].uLocalPktFormat 		
			,stVoipMgr3WayCfg->rtp_cfg[i].uRemotePktFormat 		
			,stVoipMgr3WayCfg->rtp_cfg[i].local_pt_vbd 			
			,stVoipMgr3WayCfg->rtp_cfg[i].remote_pt_vbd 			
			,stVoipMgr3WayCfg->rtp_cfg[i].uPktFormat_vbd 		
			,stVoipMgr3WayCfg->rtp_cfg[i].nG723Type				
			,stVoipMgr3WayCfg->rtp_cfg[i].nLocalFramePerPacket	
			,stVoipMgr3WayCfg->rtp_cfg[i].nRemoteFramePerPacket	
			,stVoipMgr3WayCfg->rtp_cfg[i].nFramePerPacket_vbd	
			,stVoipMgr3WayCfg->rtp_cfg[i].bVAD			
			,stVoipMgr3WayCfg->rtp_cfg[i].bPLC			
			,stVoipMgr3WayCfg->rtp_cfg[i].nJitterDelay	
			,stVoipMgr3WayCfg->rtp_cfg[i].nMaxDelay		
			,stVoipMgr3WayCfg->rtp_cfg[i].nMaxStrictDelay
			,stVoipMgr3WayCfg->rtp_cfg[i].nJitterFactor	
			,stVoipMgr3WayCfg->rtp_cfg[i].state			
			,stVoipMgr3WayCfg->rtp_cfg[i].result			
			,stVoipMgr3WayCfg->rtp_cfg[i].ip6_src_addr.in6_u.u6_addr32[0], stVoipMgr3WayCfg->rtp_cfg[i].ip6_src_addr.in6_u.u6_addr32[1]
			,stVoipMgr3WayCfg->rtp_cfg[i].ip6_src_addr.in6_u.u6_addr32[2], stVoipMgr3WayCfg->rtp_cfg[i].ip6_src_addr.in6_u.u6_addr32[3]
			,stVoipMgr3WayCfg->rtp_cfg[i].ip6_dst_addr.in6_u.u6_addr32[0], stVoipMgr3WayCfg->rtp_cfg[i].ip6_dst_addr.in6_u.u6_addr32[1]
			,stVoipMgr3WayCfg->rtp_cfg[i].ip6_dst_addr.in6_u.u6_addr32[2], stVoipMgr3WayCfg->rtp_cfg[i].ip6_dst_addr.in6_u.u6_addr32[3]
			,stVoipMgr3WayCfg->rtp_cfg[i].ipv6
			);
		}		
	}	
#endif
	
	SETSOCKOPT(VOIP_MGR_SETCONFERENCE, stVoipMgr3WayCfg, TstVoipMgr3WayCfg, 1);
	return 0;//stVoipMgr3WayCfg->ret_val;
}

// 0:rfc2833  1: sip info  2: inband  3: DTMF delete 4: rfc2833 & inband
int32 rtk_SetDtmfMode(uint32 chid, uint32 mode)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u mode=%u\n" , chid , mode);

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = mode;
	if((mode > 4) || (mode < 0)){
		TAPI_ERROR("mode out of range[ 0:rfc2833  1: sip info  2: inband  3: DTMF delete 4: rfc2833 & inband]\n");
		return -1;
	}
	SETSOCKOPT(VOIP_MGR_SET_DTMF_MODE, &stVoipCfg, TstVoipCfg, 1);
	return 0;//stVoipCfg.ret_val;

}

#if 0
/*
 * @ingroup VOIP_RTP_DTMF
 * @brief Get the DTMF mode
 * @param chid The channel number.
 * @param sid The session number.
 * @param pmode The DTMF mode.
 * @retval 0 Success
 * @note Not implement.
 */
int32 rtk_GetDTMFMODE(uint32 chid, uint32 sid, uint32 *pmode)
{
	return -1;
}
#endif

int32 rtk_SetEchoTail(uint32 chid, uint32 echo_tail, uint32 nlp, uint32 at_stepsize, uint32 rt_stepsize, uint32 cng_flag)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	TAPI_DUMP( "chid=%u echo_tail=%u nlp=%u at_stepsize=%u "
		"rt_stepsize=%u cng_flag=%u\n"
		,chid ,echo_tail ,nlp ,at_stepsize ,rt_stepsize ,cng_flag );

	stVoipValue.ch_id = chid;
	stVoipValue.value5 = echo_tail;
	stVoipValue.value1 = nlp;
	stVoipValue.value2 = at_stepsize;
	stVoipValue.value3 = rt_stepsize;
	stVoipValue.value4 = cng_flag;

	SETSOCKOPT(VOIP_MGR_SET_ECHO_TAIL_LENGTH, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;

}

int32 rtk_SetG168Lec(uint32 chid, uint32 support_lec)	/* This function can turn on/off G168 LEC. */
{
        TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
        TAPI_DUMP("chid=%u support_lec=%u\n" , chid , support_lec);	
        stVoipCfg.ch_id = chid;
        stVoipCfg.enable = support_lec;

        SETSOCKOPT(VOIP_MGR_SET_G168_LEC_CFG, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetVbdEc(uint32 chid, uint32 vbd_high_ec_mode, uint32 vbd_low_ec_mode, uint32 ec_restore_val)
{
        TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
        TAPI_DUMP( "chid=%u vbd_high_ec_mode=%u "
                "vbd_low_ec_mode=%u ec_restore_val=%u\n"
                ,chid ,vbd_high_ec_mode ,vbd_low_ec_mode ,ec_restore_val);

        stVoipCfg.ch_id = chid;
        stVoipCfg.cfg = vbd_high_ec_mode;
        stVoipCfg.cfg2 = vbd_low_ec_mode;
        stVoipCfg.cfg3 = ec_restore_val;

        SETSOCKOPT(VOIP_MGR_SET_VBD_EC, &stVoipCfg, TstVoipCfg, 1);
        
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_Set_GET_EC_DEBUG(TstVoipEcDebug* pstVoipEcDebug)
{
	SETSOCKOPT(VOIP_MGR_GET_EC_DEBUG, pstVoipEcDebug, TstVoipEcDebug, 1);

	return 0;//pstVoipEcDebug->ret_val;
}

#if 1
// thlin 2006-05-04
/**
 * @ingroup VOIP_PHONE_FXS
 * @brief Set the Tx Gain of FXS
 * @param chid The channel number.
 * @param gain List in the SLIC_GAIN_TBL structure
 * @retval 0 Success
 */
int32 rtk_Set_SLIC_Tx_Gain(uint32 chid, SLIC_GAIN_TBL gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_TX_GAIN, &stVoipValue, TstVoipValue, 1);

	return 0;
}

// thlin 2006-05-04
/**
 * @ingroup VOIP_PHONE_FXS
 * @brief Set the Rx Gain of FXS
 * @param chid The channel number.
 * @param gain List in the SLIC_GAIN_TBL structure
 * @retval 0 Success
 */
int32 rtk_Set_SLIC_Rx_Gain(uint32 chid, SLIC_GAIN_TBL gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_RX_GAIN, &stVoipValue, TstVoipValue, 1);

	return 0;
}
#endif

int32 rtk_SetSlicRingCadence(uint32 chid, uint16 cad_on_msec, uint16 cad_off_msec)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	TAPI_DUMP( "chid=%u cad_on_msec=%u cad_off_msec=%u\n ", 
		chid , cad_on_msec ,  cad_off_msec);

	stVoipValue.ch_id = chid;
	stVoipValue.value5 = cad_on_msec;
	stVoipValue.value6 = cad_off_msec;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_RING_CADENCE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetSlicRingFreqAmp(uint32 chid, uint8 preset)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	TAPI_DUMP( "chid=%u preset=%u\n ", chid , preset);

	stVoipValue.ch_id = chid;
	stVoipValue.value = preset;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_RING_FRQ_AMP, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetMultiRingCadence(uint32 chid, TstVoipCadence* pRingCad)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pRingCad == NULL ){
		TAPI_DUMP( "chid=%u pRingCad is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u m_id=%u cadon1=%u cadoff1=%u "
			"cadon2=%u cadoff2=%u cadon3=%u "	
			"cadoff3=%u cadon4=%u cadoff4=%u \n"		
			, pRingCad->ch_id, pRingCad->m_id, pRingCad->cadon1, pRingCad->cadoff1
			, pRingCad->cadon2, pRingCad->cadoff2, pRingCad->cadon3	         
			, pRingCad->cadoff3, pRingCad->cadon4, pRingCad->cadoff4);	
	}
#endif

	pRingCad->ch_id = chid;

	SETSOCKOPT(VOIP_MGR_SET_MULTI_RING_CADENCE, pRingCad, TstVoipCadence, 1);
	//SETSOCKOPT(VOIP_MGR_SET_MULTI_RING_CADENCE, &stVoipCadence, TstVoipCadence, 1);

	return 0;//stVoipCadence.ret_val;
}

// flag: line voltage flag. 0: zero voltage, 1: normal voltage, 2: reverse voltage
int32 rtk_SetSlicLineVoltage(uint32 chid, uint8 flag)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	TAPI_DUMP( "chid=%u flag=%u\n ", chid , flag);

        stVoipValue.ch_id = chid;
        stVoipValue.value = flag;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_LINE_VOLTAGE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

// voltage: ring voltage.
int32 rtk_SetSlicRingVoltageFreq(uint32 chid, SLICRING_VOLTAGE_FREQ index)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0, sizeof(TstVoipValue));
	TAPI_DUMP( "chid=%u index=%u\n ", chid , index);

	if ( index <= SLIC_RING_INIT || index >= SLIC_RING_MAX )
		return -1;

	stVoipValue.ch_id = chid;
	stVoipValue.value5 = index;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_RING_VOLTAGE_FREQ, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_GenSlicCpc(uint32 chid, uint32 cpc_ms)
{
        TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0, sizeof(TstVoipCfg));
        TAPI_DUMP( "chid=%u cpc_ms=%u\n ", chid , cpc_ms);
        stVoipCfg.ch_id = chid;
        stVoipCfg.cfg = cpc_ms;
        SETSOCKOPT(VOIP_MGR_GEN_SLIC_CPC, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetFxoRingDetection(uint16 ring_on_msec, uint16 first_ringoff_msec, uint16 ring_off_msec)
{
        TstVoipValue stVoipValue;
		memset(&stVoipValue, 0, sizeof(TstVoipValue));
        TAPI_DUMP( "ring_on_msec=%u first_ringoff_msec=%u ring_off_msec=%u\n ", 
        	ring_on_msec, first_ringoff_msec, ring_off_msec);

        stVoipValue.value5 = ring_on_msec;
        stVoipValue.value6 = first_ringoff_msec;
        stVoipValue.value7 = ring_off_msec;

        SETSOCKOPT(VOIP_MGR_SET_RING_DETECTION, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

// thlin 2006-05-10
int32 rtk_SetDaaTxGain(uint32 gain)
{
    TAPI_DUMP( "gain=%u\n ", gain );
	
	if((gain > 11) || (gain < 0)){
		TAPI_ERROR("gain out of range The gain value (0..11) (0 ~ -11 dB) Available range: 0 ~ -11 \n");
		return -1;
	}
	if ((g_VoIP_Feature & DAA_TYPE_MASK) == REAL_DAA || (g_VoIP_Feature & DAA_TYPE_MASK) == REAL_DAA_NEGO)
	{
		TstVoipValue stVoipValue;

		stVoipValue.value = gain;

		SETSOCKOPT(VOIP_MGR_SET_DAA_TX_GAIN, &stVoipValue, TstVoipValue, 1);
		return 0;//stVoipValue.ret_val;
	}
	return 0;

}

// thlin 2006-05-10
int32 rtk_SetDaaRxGain(uint32 gain)
{
	TAPI_DUMP( "gain=%u\n ", gain );
	
	if((gain > 11) || (gain < 0)){
		TAPI_ERROR("gain out of range The gain value (0..11) (0 ~ -11 dB) Available range: 0 ~ -11 \n");
		return -1;
	}
	if ((g_VoIP_Feature & DAA_TYPE_MASK) == REAL_DAA || (g_VoIP_Feature & DAA_TYPE_MASK) == REAL_DAA_NEGO)
	{
		TstVoipValue stVoipValue;

		stVoipValue.value = gain;

		SETSOCKOPT(VOIP_MGR_SET_DAA_RX_GAIN, &stVoipValue, TstVoipValue, 1);

		return 0;//stVoipValue.ret_val;
	}
	return 0;

}

int32 rtk_SetDaaHybrid(uint32 chid, uint8 index)
{
	TAPI_DUMP( "chid=%u index=%u\n ", chid , index );
	rtk_GetVoIPFeature();

	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipValue stVoipValue;

		stVoipValue.ch_id = chid;
		stVoipValue.value = index;

		SETSOCKOPT(VOIP_MGR_SET_DAA_HYBRID, &stVoipValue, TstVoipValue, 1);
		
		return 0;//stVoipValue.ret_val;
	}
	return 0;

}

int32 rtk_SetFxoTune(uint32 chid, uint32 control)
{
	TAPI_DUMP( "chid=%u index=%u\n ", chid , control );
	rtk_GetVoIPFeature();

	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;

		stVoipCfg.ch_id = chid;
		stVoipCfg.cfg = control;

		SETSOCKOPT(VOIP_MGR_SET_FXO_TUNE, &stVoipCfg, TstVoipCfg, 1);
		
		return 0;//stVoipCfg.ret_val;
	}
	return 0;

}

// thlin 2006-06-07
int32 rtk_SetFlashHookTime(uint32 chid, uint32 min_time, uint32 time, uint32 on_time)
{
	TstVoipHook stHookTime;
	memset(&stHookTime, 0, sizeof(TstVoipHook));
	TAPI_DUMP( "chid=%u min_time=%u time=%u on_time=%u\n ", chid , min_time , time, on_time );

	stHookTime.ch_id = chid;
	stHookTime.on_time= on_time;
	stHookTime.flash_time_min = min_time;
	stHookTime.flash_time = time;

	SETSOCKOPT(VOIP_MGR_SET_FLASH_HOOK_TIME, &stHookTime, TstVoipHook, 1);

	return 0;//stHookTime.ret_val;
}

int32 rtk_SetOffHookTime(uint32 chid, uint32 offhook_time)
{
	TstVoipOffHook stOffHookTime;
	memset(&stOffHookTime, 0, sizeof(TstVoipOffHook));
	TAPI_DUMP( "chid=%u offhook_min_time=%u\n ", chid , offhook_time);

	stOffHookTime.ch_id = chid;
	stOffHookTime.offhook_min = offhook_time;

	SETSOCKOPT(VOIP_MGR_SET_OFF_HOOK_TIME, &stOffHookTime, TstVoipOffHook, 1);

	return 0;//stHookTime.ret_val;
}

int32 rtk_GetCalibrationCoeff(uint32 chid, TstVoipCalibrationCoeff *stVoipCalibrationCoeff )
{
	//TstVoipOffHook stOffHookTime;
	TAPI_DUMP( "chid=%u\n", chid);

	SETSOCKOPT(VOIP_MGR_GET_CALIBRATION_COEFFICIENTS, stVoipCalibrationCoeff, TstVoipCalibrationCoeff, 1);

	return 0;
}

int32 rtk_SetCalibrationCoeff(uint32 chid, TstVoipCalibrationCoeff *stVoipCalibrationCoeff )
{
	TAPI_DUMP( "chid=%u\n", chid);

	SETSOCKOPT(VOIP_MGR_SET_CALIBRATION_COEFFICIENTS, stVoipCalibrationCoeff, TstVoipCalibrationCoeff, 1);

	return 0;
}

int32 rtk_DoCalibration(uint32 chid, TstVoipDoCalibration *stVoipDoCalibration)
{
	TAPI_DUMP( "chid=%u mode=%u\n ", chid , stVoipDoCalibration->mode);

	SETSOCKOPT(VOIP_MGR_DO_CALIBRATION, stVoipDoCalibration, TstVoipDoCalibration, 1);

	return 0;
}

int32 rtk_EnableABCDSignal(uint32 chid, int bEnable)
{
	TstVoip2833 stRFC2833;
	memset(&stRFC2833, 0, sizeof(TstVoip2833));
	TAPI_DUMP( "chid=%u bEnable=%d\n ", chid , bEnable );

	stRFC2833.ch_id = chid;
	stRFC2833.bEnable = bEnable;
	SETSOCKOPT(VOIP_MGR_SET_RFC2833_ENABLE_ABCDSIGNAL, &stRFC2833, stRFC2833, 1);
	return 0;
}

int32 rtk_GenerateABCDSignal(uint32 chid, unsigned char signal)
{
	TstVoip2833 stRFC2833;
	memset(&stRFC2833, 0, sizeof(TstVoip2833));
	TAPI_DUMP( "chid=%u signal=%u\n ", chid , signal );

	stRFC2833.ch_id = chid;
	stRFC2833.digit = signal;

	SETSOCKOPT(VOIP_MGR_SET_RFC2833_GENERATE_ABCDSIGNAL, &stRFC2833, stRFC2833, 1);
	return 0;

}

int32 rtk_GenMeteringPulse(uint32 chid, uint16 hz, uint16 onTime, uint16 offTime, uint16 numMeters)
{
	TstVoipSLICMeterPulse stVoipSLICMeterPulse;
	memset(&stVoipSLICMeterPulse, 0, sizeof(TstVoipSLICMeterPulse));
	TAPI_DUMP( "chid=%u hz=%u onTime=%u offTime=%u numMeters=%u\n",
		chid, hz, onTime, offTime, numMeters);		
	
	stVoipSLICMeterPulse.chid = chid;
	stVoipSLICMeterPulse.hz = hz;
	stVoipSLICMeterPulse.onTime = onTime;
	stVoipSLICMeterPulse.offTime = offTime;
	stVoipSLICMeterPulse.numMeters = numMeters;
	
	SETSOCKOPT(VOIP_MGR_SET_MERTEPULSE, &stVoipSLICMeterPulse, TstVoipSLICMeterPulse, 1);
	
	return 0;
}

int32 rtk_GetPortStatus(uint32 chid)
{
	TstVoipSLICPortDetect stTstVoipSLICPortDetect;
	TAPI_DUMP( "chid=%u \n", chid );

	memset(&stTstVoipSLICPortDetect, 0, sizeof(stTstVoipSLICPortDetect));
	stTstVoipSLICPortDetect.chid = chid;
	stTstVoipSLICPortDetect.status = 0;
	
	SETSOCKOPT(VOIP_MGR_GET_PORTDETECT, &stTstVoipSLICPortDetect, TstVoipSLICPortDetect, 1);
	
	return stTstVoipSLICPortDetect.status;
}

int32 rtk_GetLineRoh(uint32 chid, TstVoipSLICLineRoh *data)
{
	TstVoipSLICLineRoh stTstVoipSLICLineRoh;
#ifdef VOIP_TAPI_DBG_DUMP
	if( data == NULL ){
		TAPI_DUMP( "chid=%u data is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u \n", chid );	
	}
#endif

	//int *roh = (int *)data;	
	
	memset(&stTstVoipSLICLineRoh, 0, sizeof(TstVoipSLICLineRoh));
	stTstVoipSLICLineRoh.chid = chid;
	
	SETSOCKOPT(VOIP_MGR_GET_LINEROH, &stTstVoipSLICLineRoh, TstVoipSLICLineRoh, 1);
	
	data->fltMask = stTstVoipSLICLineRoh.fltMask;
	data->measStatus = stTstVoipSLICLineRoh.measStatus;
	data->rLoop1 = stTstVoipSLICLineRoh.rLoop1;
	data->rLoop2 = stTstVoipSLICLineRoh.rLoop2;
		
	return 0;
}

int32 rtk_GetLineVoltage(uint32 chid, TstVoipSLICLineVoltage *data)
{
	TstVoipSLICLineVoltage stTstVoipSLICLineVoltage;
#ifdef VOIP_TAPI_DBG_DUMP
	if( data == NULL ){
		TAPI_DUMP( "chid=%u data is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u \n", chid );
	}
#endif

	//int *voltage = (int *)data;
	
	memset(&stTstVoipSLICLineVoltage, 0, sizeof(TstVoipSLICLineVoltage));
	stTstVoipSLICLineVoltage.chid = chid;
	
	SETSOCKOPT(VOIP_MGR_GET_LINEVOLTAGE, &stTstVoipSLICLineVoltage, TstVoipSLICLineVoltage, 1);
	
	data->vAcDiff = stTstVoipSLICLineVoltage.vAcDiff;
	data->vAcRing = stTstVoipSLICLineVoltage.vAcRing;
	data->vAcTip = stTstVoipSLICLineVoltage.vAcTip;
	data->vDcDiff = stTstVoipSLICLineVoltage.vDcDiff;	
	data->vDcRing = stTstVoipSLICLineVoltage.vDcRing;
	data->vDcTip = stTstVoipSLICLineVoltage.vDcTip;	
	
	return 0;
}

int32 rtk_GetLineResflt(uint32 chid, TstVoipSLICLineResFlt *data)
{
	TstVoipSLICLineResFlt stTstVoipSLICLineResFlt;
#ifdef VOIP_TAPI_DBG_DUMP
	if( data == NULL ){
		TAPI_DUMP( "chid=%u data is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u \n", chid );
	}
#endif
	//int *flt = (int *)data;

	memset(&stTstVoipSLICLineResFlt, 0, sizeof(TstVoipSLICLineResFlt));
	stTstVoipSLICLineResFlt.chid = chid;
	
	SETSOCKOPT(VOIP_MGR_GET_LINERESFLT, &stTstVoipSLICLineResFlt, TstVoipSLICLineResFlt, 1);
	
	data->rGnd = stTstVoipSLICLineResFlt.rGnd;
	data->rrg = stTstVoipSLICLineResFlt.rrg;
	data->rtg = stTstVoipSLICLineResFlt.rtg;
	data->rtr = stTstVoipSLICLineResFlt.rtr;
	data->fltMask = stTstVoipSLICLineResFlt.fltMask;
	data->measStatus = stTstVoipSLICLineResFlt.measStatus;	
	
	return 0;
}

int32 rtk_GetLineCap(uint32 chid, TstVoipSLICLineCap *data)
{
	TstVoipSLICLineCap stVoipSLICLineCap;
#ifdef VOIP_TAPI_DBG_DUMP
	if( data == NULL ){
		TAPI_DUMP( "chid=%u data is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u \n", chid );	
	}
#endif
	
	memset(&stVoipSLICLineCap, 0, sizeof(TstVoipSLICLineCap));
	stVoipSLICLineCap.chid = chid;
	
	SETSOCKOPT(VOIP_MGR_GET_LINECAP, &stVoipSLICLineCap, TstVoipSLICLineCap, 1);
	
	data->crg = stVoipSLICLineCap.crg;
	data->ctg = stVoipSLICLineCap.ctg;
	data->ctr = stVoipSLICLineCap.ctr;
	data->fltMask = stVoipSLICLineCap.fltMask;
	data->measStatus = stVoipSLICLineCap.measStatus;	
		
	return 0;
}

int32 rtk_GetLineLoopCurrent(uint32 chid, TstVoipSLICLineLoopCurrent *data)
{
	TstVoipSLICLineLoopCurrent stVoipSLICLineLoopCurrent;
#ifdef VOIP_TAPI_DBG_DUMP
	if( data == NULL ){
		TAPI_DUMP( "chid=%u data is NULL\n" , chid );
	}
	else{
		TAPI_DUMP( "chid=%u \n", chid );	
	}
#endif
	
	memset(&stVoipSLICLineLoopCurrent, 0, sizeof(TstVoipSLICLineLoopCurrent));
	stVoipSLICLineLoopCurrent.chid = chid;
	
	SETSOCKOPT(VOIP_MGR_GET_LINELOOPCURRENT, &stVoipSLICLineLoopCurrent, TstVoipSLICLineLoopCurrent, 1);
	
	data->iTestLoad = stVoipSLICLineLoopCurrent.iTestLoad;
	data->rTestLoad = stVoipSLICLineLoopCurrent.rTestLoad;
	data->vTestLoad = stVoipSLICLineLoopCurrent.vTestLoad;
	data->fltMask = stVoipSLICLineLoopCurrent.fltMask;
	data->measStatus = stVoipSLICLineLoopCurrent.measStatus;	
		
	return 0;
}


int32 rtk_DoLineTest_REN(uint32 chid, TstVoipSLICLineTest_ren *data)
{
	TstVoipSLICLineTest	stVoipSLICLineTest;
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u\n", chid);
#endif
	
	memset(&stVoipSLICLineTest, 0, sizeof(TstVoipSLICLineTest));
	stVoipSLICLineTest.chid = chid;
	stVoipSLICLineTest.type = LT_TID_RINGERS;
	
	SETSOCKOPT(VOIP_MGR_DO_LINETEST, &stVoipSLICLineTest, TstVoipSLICLineTest, 1);
	
	memcpy(&data[0], &stVoipSLICLineTest.data[0], sizeof(TstVoipSLICLineTest_ren));
	return 0;
}

int32 rtk_DoLineTest_rdloopcond(uint32 chid, TstVoipSLICLineTest_rdloop *data)
{
	TstVoipSLICLineTest	stVoipSLICLineTest;
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u\n", chid);
#endif
	
	memset(&stVoipSLICLineTest, 0, sizeof(TstVoipSLICLineTest));
	stVoipSLICLineTest.chid = chid;
	stVoipSLICLineTest.type = LT_TID_RD_LOOP_COND;
	
	SETSOCKOPT(VOIP_MGR_DO_LINETEST, &stVoipSLICLineTest, TstVoipSLICLineTest, 1);

	memcpy(&data[0], &stVoipSLICLineTest.data[0], sizeof(TstVoipSLICLineTest_rdloop));
	
	return 0;
}

int32 rtk_DoLineTest_dcvoltage(uint32 chid, TstVoipSLICLineTest_dcvoltage *data)
{
	TstVoipSLICLineTest	stVoipSLICLineTest;
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u\n", chid);	
#endif
	
	memset(&stVoipSLICLineTest, 0, sizeof(TstVoipSLICLineTest));
	stVoipSLICLineTest.chid = chid;
	stVoipSLICLineTest.type = LT_TID_DC_VOLTAGE;
	
	SETSOCKOPT(VOIP_MGR_DO_LINETEST, &stVoipSLICLineTest, TstVoipSLICLineTest, 1);

	memcpy(&data[0], &stVoipSLICLineTest.data[0], sizeof(TstVoipSLICLineTest_dcvoltage));
	
	return 0;
}

int32 rtk_DoLineTest_ringerst(uint32 chid, TstVoipSLICLineTest_ringerst *data)
{
	TstVoipSLICLineTest	stVoipSLICLineTest;
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u\n", chid);	
#endif
	
	memset(&stVoipSLICLineTest, 0, sizeof(TstVoipSLICLineTest));
	stVoipSLICLineTest.chid = chid;
	stVoipSLICLineTest.type = LT_TID_RINGING_ST;
	
	SETSOCKOPT(VOIP_MGR_DO_LINETEST, &stVoipSLICLineTest, TstVoipSLICLineTest, 1);

	memcpy(&data[0], &stVoipSLICLineTest.data[0], sizeof(TstVoipSLICLineTest_ringerst));
	
	return 0;
}

int32 rtk_DoLineTest_rdBattery(uint32 chid, TstVoipSLICLineTest_rdbattery *data)
{
	TstVoipSLICLineTest	stVoipSLICLineTest;
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "chid=%u\n", chid);	
#endif
	
	memset(&stVoipSLICLineTest, 0, sizeof(TstVoipSLICLineTest));
	stVoipSLICLineTest.chid = chid;
	stVoipSLICLineTest.type = LT_TID_RD_BAT_COND;
	
	SETSOCKOPT(VOIP_MGR_DO_LINETEST, &stVoipSLICLineTest, TstVoipSLICLineTest, 1);

	memcpy(&data[0], &stVoipSLICLineTest.data[0], sizeof(TstVoipSLICLineTest_rdbattery));
	
	return 0;
}


int32 rtk_SetRFC2833SendByAP(uint32 chid, uint32 config)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u config=%u\n", chid , config );

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = config;
	SETSOCKOPT(VOIP_MGR_SEND_RFC2833_BY_AP, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetRFC2833TxConfig(uint32 chid, uint32 tx_mode, RFC2833_VOLUME_MODE volume_mode, RFC2833_VOLUME volume) //tx_mode: 0: DSP mode, 1: AP mode
{
	TstVoip2833 stRFC2833;
	TstVoipCfg stVoipCfg;
	memset(&stRFC2833, 0x0, sizeof(TstVoip2833));
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u tx_mode=%u volume_mode=%u volume=%u\n", chid , tx_mode , volume_mode , volume);

	if (volume_mode == RFC2833_VOLUME_DSP_ATUO)
	{
		if (tx_mode == 1) //send by AP
			printf("rtk_SetRFC2833TxConfig: invalid setting. TX mode is AP, but volume is DSP auto mode. ch=%d\n", chid);
		
		return -3;
	}
	
	// TX mode must be set prior to TX volume.
	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = tx_mode;
	SETSOCKOPT(VOIP_MGR_SEND_RFC2833_BY_AP, &stVoipCfg, TstVoipCfg, 1);
	
	stRFC2833.ch_id = chid;
	stRFC2833.bEnable = volume_mode;
	stRFC2833.volume = volume;
	SETSOCKOPT(VOIP_MGR_SET_RFC2833_TX_VOLUME, &stRFC2833, TstVoip2833, 1);

	return 0; //(stRFC2833.ret_val | stVoipCfg.ret_val);
}

int32 rtk_SetRtpRfc2833(uint32 chid, uint32 sid, uint32 event, unsigned int duration)
{
	TstVoip2833 stRFC2833;
	memset(&stRFC2833, 0x0, sizeof(TstVoip2833));
	TAPI_DUMP( "chid=%u sid=%u event=%u duration=%u\n", chid , sid , event , duration);

	stRFC2833.ch_id = chid;
	stRFC2833.m_id = sid;
	stRFC2833.digit = event;
	stRFC2833.duration = duration; //unit: msec
	SETSOCKOPT(VOIP_MGR_SEND_RFC2833_PKT_CFG, &stRFC2833, TstVoip2833, 1);

	return 0;//stRFC2833.ret_val;
}

int32 rtk_LimitMaxRfc2833DtmfDuration(uint32 chid, uint32 duration_in_ms, uint8 bEnable)
{
	TstVoip2833 stRFC2833;
	memset(&stRFC2833, 0x0, sizeof(TstVoip2833));
	TAPI_DUMP( "chid=%u duration_in_ms=%u bEnable=%u\n", chid , duration_in_ms , bEnable );
	stRFC2833.ch_id = chid;
	stRFC2833.duration = duration_in_ms;
	stRFC2833.bEnable = bEnable;

	SETSOCKOPT(VOIP_MGR_LIMIT_MAX_RFC2833_DTMF_DURATION, &stRFC2833, TstVoip2833, 1);

	return 0;//stRFC2833.ret_val;
}

int32 rtk_GetRfc2833RxEvent(uint32 chid, uint32 mid, VoipEventID *pent)
{
	int ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pent == NULL ){
		TAPI_DUMP( "chid=%u mid=%u pent is NULL\n" , chid , mid );
	}
#endif
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_RFC2833;
	stVoipEvent.mask = ( mid ? VEM_MID1 : VEM_MID0 );
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;
	
	*pent = stVoipEvent.id;
	
	#ifdef VOIP_TAPI_DBG_DUMP
	if( *pent != 0 )
		TAPI_DUMP( "chid=%u mid=%u *pent=%u\n", chid , mid , *pent );
	#endif
	
	return 0;
}

int32 rtk_SetFaxModemRfc2833(uint32 chid, uint32 relay_flag, uint32 removal_flag, uint32 tone_gen_flag)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u relay_flag=%u removal_flag=%u tone_gen_flag=%u\n", 
		chid , relay_flag , removal_flag , tone_gen_flag );

	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = relay_flag;
	stVoipCfg.cfg2 = removal_flag;
	stVoipCfg.cfg3 = tone_gen_flag;
	
	SETSOCKOPT(VOIP_MGR_FAX_MODEM_RFC2833_CFG, &stVoipCfg, TstVoipCfg, 1);
	
	return 0;
}

int32 rtk_SetRfc2833PacketInterval(uint32 chid, uint32 mid, uint32 type, uint32 interval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u mid=%u type=%u interval=%u\n", 
		chid , mid , type , interval );

	stVoipCfg.ch_id = chid;
	stVoipCfg.m_id = mid;
	stVoipCfg.cfg = type;		//0: DTMF, 1: Fax/Modem
	stVoipCfg.cfg2 = interval;	//msec: must be multiple of 10msec

	SETSOCKOPT(VOIP_MGR_RFC2833_PKT_INTERVAL_CFG, &stVoipCfg, TstVoipCfg, 1);
	
	return 0;
}

/*
 * @ingroup VOIP_PHONE_CALLERID
 * @brief Generate CallerID via DTMF
 * @param chid The channel number.
 * @param pval The Caller ID.
 * @retval 0 Success
 * @note internal use
 */
int32 rtk_SetDtmfCidString(uint32 chid, const char *str_cid)
{
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
	TAPI_DUMP( "chid=%u str_cid=%s \n", chid , str_cid );

	stCIDstr.ch_id = chid;
	strcpy(stCIDstr.string, str_cid);
	SETSOCKOPT(VOIP_MGR_DTMF_CID_GEN_CFG, &stCIDstr, TstVoipCID, 1);

	return 0;//stCIDstr.ret_val;
}

/*
 * @ingroup VOIP_PHONE_CALLERID
 * @brief Check the CallerID generation via DTMF is done
 * @param chid The channel number.
 * @param pstate 0 if done
 * @retval 0 Success
 * @note internal use
 */
int32 rtk_GetDtmfCIDState(uint32 chid, uint32 *cid_state)
{
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
#ifdef VOIP_TAPI_DBG_DUMP
	if( cid_state == NULL ){
		TAPI_DUMP( "cid_state is NULL\n" );
	}
#endif

	stCIDstr.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_GET_CID_STATE_CFG, &stCIDstr, TstVoipCID, 1);
	*cid_state = stCIDstr.cid_state;

	TAPI_DUMP( "chid=%u str_state=%u \n", chid , *cid_state );
	return 0;//stCIDstr.ret_val;
}

int32 rtk_GetFaxModemEvent(uint32 chid, uint32 *pval, VoipEventID *pevent, uint32 flush)
{
	int ret;
#ifdef VOIP_TAPI_DBG_DUMP
	if( (pval == NULL) || (pevent == NULL) ){
		TAPI_DUMP( "chid=%u pval=%p pevent=%p flush=%u\n" , chid , pval , pevent , flush);
	}
#endif
	
	if( flush ) {
		
		TstFlushVoipEvent stFlushVoipEvent;
		memset(&stFlushVoipEvent, 0x0, sizeof(TstFlushVoipEvent));
		stFlushVoipEvent.ch_id = chid;
		stFlushVoipEvent.type = VET_FAXMDM;
		stFlushVoipEvent.mask = 0;
		
		if( ( ret = rtk_FlushVoIPEvent( &stFlushVoipEvent ) ) < 0 )
			return ret;
			
	} else {
	
		TstVoipEvent stVoipEvent;
		memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
		stVoipEvent.ch_id = chid;
		stVoipEvent.type = VET_FAXMDM;
		stVoipEvent.mask = 0;
		
		if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
			return ret;	
		
		if( pval )
		*pval = stVoipEvent.p1;		// p1 is translated ID in FAX/Modem event 
		
		if( pevent )
			*pevent = stVoipEvent.id;
	}
	
#ifdef VOIP_TAPI_DBG_DUMP
	if(  pval && pevent ) {
		if( *pval && *pevent ) {
			TAPI_DUMP( "chid=%u *pval=%u *pevent=0x08%x flush=%u\n", chid , *pval , *pevent , flush );
		}
	}
#endif
	
	return 0;
}

int32 rtk_GetFaxEndDetect(uint32 chid, uint32 *pval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	stVoipCfg.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_FAX_END_DETECT, &stVoipCfg, TstVoipCfg, 1);
	*pval = stVoipCfg.enable;
	//t.38 fax end detect, 1:fax end.

	TAPI_DUMP( "chid=%u *pval=%u\n", chid , *pval );
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetFaxDisDetect(uint32 chid, uint32 *pval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid);
	}
#endif
	stVoipCfg.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_FAX_DIS_DETECT, &stVoipCfg, TstVoipCfg, 1);
	*pval = stVoipCfg.enable;
	//t.38 v21 dis detect, 1:dis(Digital Identification Signal) detected.

	TAPI_DUMP( "chid=%u *pval=%u\n", chid , *pval );
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetFaxDisTxDetect(uint32 chid, uint32 *pval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u Input is NULL\n" , chid );
	}
#endif
	stVoipCfg.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_FAX_DIS_TX_DETECT, &stVoipCfg, TstVoipCfg, 1);
	*pval = stVoipCfg.enable;
	//t.38 v21 dis tx direction detect, 1:dis(Digital Identification Signal) detected.

	TAPI_DUMP( "chid=%u *pval=%u\n", chid , *pval );
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetFaxDisRxDetect(uint32 chid, uint32 *pval)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	stVoipCfg.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_FAX_DIS_RX_DETECT, &stVoipCfg, TstVoipCfg, 1);
	*pval = stVoipCfg.enable;
	//t.38 v21 dis rx direction detect, 1:dis(Digital Identification Signal) detected.
	
	TAPI_DUMP( "chid=%u *pval=%u\n", chid , *pval );

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetFxoLineVoltage(uint32 chid, uint32 *pval)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pval == NULL ){
		TAPI_DUMP( "chid=%u pval is NULL\n" , chid );
	}
#endif
	
	stVoipValue.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_GET_DAA_LINE_VOLTAGE, &stVoipValue, TstVoipValue, 1);
	*pval = stVoipValue.value5;

	TAPI_DUMP( "chid=%u *pval=%u\n", chid , *pval );	
	return 0;//stVoipValue.ret_val;
}

int32 rtk_EnablePcm(uint32 chid, uint32 val)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u val=%u\n", chid , val );

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = val;
	SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetBusFormat(uint32 chid, AP_BUS_DATA_FORMAT format)
{
	TstVoipBusDataFormat stVoipBusDataFormat;
	memset(&stVoipBusDataFormat, 0x0, sizeof(TstVoipBusDataFormat));
	TAPI_DUMP( "chid=%u format=%d\n", chid , format );

	stVoipBusDataFormat.ch_id = chid;
	stVoipBusDataFormat.format = format;
	SETSOCKOPT(VOIP_MGR_SET_BUS_DATA_FORMAT, 
				&stVoipBusDataFormat, TstVoipBusDataFormat, 1);
	
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetPcmTimeslot(uint32 chid, uint32 timeslot1, uint32 timeslot2)
{
	TstVoipPcmTimeslot stVoipPcmTimeslot;
	memset(&stVoipPcmTimeslot, 0x0, sizeof(TstVoipPcmTimeslot));
	TAPI_DUMP( "chid=%u timeslot1=%u  timeslot2=%u\n", chid , timeslot1 , timeslot2 );

	if((timeslot1 > 31) || (timeslot1 < 0)){
		TAPI_ERROR("timeslot1 out of range Available range: 0 to 31. \n");
		return -1;
	}
	
	if((timeslot2 > 31) || (timeslot2 < 0)){
		TAPI_ERROR("timeslot2 out of range Available range: 0 to 31. \n");
		return -1;
	}
	stVoipPcmTimeslot.ch_id = chid;
	stVoipPcmTimeslot.timeslot1 = timeslot1;
	stVoipPcmTimeslot.timeslot2 = timeslot2;
	SETSOCKOPT(VOIP_MGR_SET_PCM_TIMESLOT, 
				&stVoipPcmTimeslot, TstVoipPcmTimeslot, 1);
	
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_StopCid(uint32 chid, char cid_mode)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP( "chid=%u cid_mode=%d\n", chid , cid_mode );

	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = cid_mode;
	SETSOCKOPT(VOIP_MGR_STOP_CID, &stVoipCfg, TstVoipCfg, 1);
	
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetDtmfCidParam(uint32 chid, DTMF_DIGIT start_digit, DTMF_DIGIT end_digit, uint32 auto_ctrl, uint32 dtmf_on_ms, uint32 dtmf_pause_ms, uint32 dtmf_pre_silence, uint32 dtmf_end_silence)
{
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
	TAPI_DUMP(
	"chid=%d start_digit=%d end_digit=%d auto_ctrl=%u dtmf_on_ms=%u dtmf_pause_ms=%u dtmf_pre_silence=%u dtmf_end_silence=%u\n"
	,chid, start_digit, end_digit, auto_ctrl, dtmf_on_ms, dtmf_pause_ms, dtmf_pre_silence, dtmf_end_silence);

	stCIDstr.ch_id = chid;
	stCIDstr.cid_dtmf_start = start_digit;
	stCIDstr.cid_dtmf_end = end_digit;
	stCIDstr.cid_dtmf_auto = auto_ctrl;
	stCIDstr.cid_dtmf_on_ms = dtmf_on_ms;
	stCIDstr.cid_dtmf_pause_ms = dtmf_pause_ms;
	stCIDstr.cid_dtmf_pre_silence_ms = dtmf_pre_silence;
	stCIDstr.cid_dtmf_end_silence_ms = dtmf_end_silence;

	SETSOCKOPT(VOIP_MGR_SET_CID_DTMF_MODE, &stCIDstr, TstVoipCID, 1);

	return 0;//stCIDstr.ret_val;
}

int32 rtk_GenDtmfCid(uint32 chid, const char *str_cid)
{
	int i = 0;
	TAPI_DUMP( "chid=%d str_cid=%s\n" , chid , str_cid );
	rtk_EnablePcm(chid, 1); // enable PCM before generating dtmf caller id
	// set cid_state
	if (str_cid[0] == 0)               // not null str
		rtk_SetDtmfCidString(chid, "0123456789");   // replace "0123456789" with From CID
	else
		rtk_SetDtmfCidString(chid, str_cid);

	// polling cid_state until be clear
	uint32 tmp=-1;
	do
	{
		if(rtk_GetDtmfCIDState(chid, &tmp) != 0)
			break;
		usleep(50000);  // 50ms
		if ((i++) > 120 )// wait 6 sec, if not get cid state =0, then break.
			break;
	}
	while (tmp);

        //printf("Get DTMF CID state = 0\n");
	//rtk_EnablePcm(chid, 0); // disable PCM after generating dtmf caller id

	return 0;
}

int32 rtk_GenFskCid(uint32 chid, char *str_cid, char *str_date, char *str_cid_name, char mode)
{
	//time_t timer = time(0);		// fsk date & time sync
	TstVoipCID stCIDstr;

	uint32 tmp=-1;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
	TAPI_DUMP( "chid=%d str_cid=%s str_date=%s str_cid_name=%s mode=%d\n" , 
		chid , str_cid , str_date , str_cid_name , mode );
	rtk_GetFskCidState(chid, &tmp);

	if (tmp)
	{
		//printf("not end");
		return -2; /* don't double send caller id when sending */
	}

	if(!mode)			// on-hook
		rtk_EnablePcm(chid, 1); // enable PCM before generating fsk caller id

	stCIDstr.ch_id = chid;
	stCIDstr.cid_mode = mode;       //0:on-hook
	// set cid string
	if (str_cid[0] == 0)               // not null str
		strcpy(stCIDstr.string, "0123456789");   // replace "0123456789" with From CID
	else
		strcpy(stCIDstr.string, str_cid);

	if (str_date && str_date[0])
	{
		strcpy(stCIDstr.string2, str_date);
	}
	else
	{
	//strftime(stCIDstr.string2, 9, "%m%d%H%M", gmtime(&timer));
	//strftime(stCIDstr.string2, 9, "%m%d%H%M", localtime(&timer));
	strcpy(stCIDstr.string2, "");
	}
	if(str_cid_name)
		strcpy(stCIDstr.cid_name, str_cid_name);
	SETSOCKOPT(VOIP_MGR_FSK_CID_GEN_CFG, &stCIDstr, TstVoipCID, 1);

	return 0;//stCIDstr.ret_val;
/*
	int32 tmp=-1;
	do
	{
		if(rtk_GetFskCidState(chid, &tmp) != 0)
			break;
		usleep(50000);  // 50ms
	}
	while (tmp);
	if(!mode)			//on-hook
		rtk_eanblePCM(chid, 0); // disable PCM after generating fsk caller id
*/
}

int32 rtk_GenMdmfFskCid(uint32 chid, TstFskClid* pClid, uint32 num_clid_element)
{
	uint32 tmp=-1;
	
#ifdef VOIP_TAPI_DBG_DUMP
	int i ;
	if( pClid == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP( "chid=%u pClid->ch_id=%u pClid->service_type=%u\n" , chid , pClid->ch_id , pClid->service_type );
		for( i=0 ; i<FSK_MDMF_SIZE ; i++ )
			TAPI_DUMP( "pClid->cid_data[%d].type=0x%08x pClid->cid_data[%d].data=%s \n" ,
				i , pClid->cid_data[i].type , i , pClid->cid_data[i].data);
	}
#endif

	rtk_GetFskCidState(chid, &tmp);

	if (tmp)
	{
		return -2; /* don't double send caller id when sending */
	}

	if( pClid->service_type == 0 )	// on-hook
		rtk_EnablePcm(chid, 1); // enable PCM before generating fsk caller id

	SETSOCKOPT(VOIP_MGR_FSK_CID_MDMF_GEN, pClid, TstFskClid, 1);

	return 0;//pClid->ret_val;

}

int32 rtk_GenCidAndFxsRing(uint32 chid, char cid_mode, char *str_cid, char *str_date, char *str_cid_name, char fsk_type, uint32 bRinging)
{

	TstVoipSlicRing stVoipSlicRing;
    TstFskClid clid;
    int32 ret = 0;

	memset(&stVoipSlicRing, 0x0, sizeof(TstVoipSlicRing));
    TAPI_DUMP("chid=%u cid_mode=%d str_cid=%s str_date=%s str_cid_name=%s fsk_type=%d bRinging=%u\n",
    	chid , cid_mode , str_cid , str_date , str_cid_name , fsk_type , bRinging);
	TAPI_DUMP("g_bDisableRingFXS=%d\n" , g_bDisableRingFXS);

#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
	//if (chid < RTK_VOIP_DECT_NUM(g_VoIP_Feature))	//DECT Channel
	if( RTK_VOIP_IS_DECT_CH( chid, g_VoIP_Feature ) )
	{

#ifdef CONFIG_RTK_VOIP_DECT_SITEL_SUPPORT
		if (str_cid[0] != 0)//Send CID and Ring FXS by DSP
		{
			dect_api_S2R_call_setup( 0, chid, str_cid, str_cid_name );
		}
		else
		{
			if( dect_api_S2R_check_handset_ringing( chid ) )
				dect_api_S2R_call_release( chid );
		}
#endif

#ifdef CONFIG_RTK_VOIP_DECT_DSPG_SUPPORT

		if (str_cid[0] != 0)//Send CID and Ring FXS by DSP
		{
			if (0 == (strcmp(str_cid, "-")))
			{
				//strcpy(str_cid, "P");
				CMBS_Api_InBound_Ring_CallerID(chid, NULL, str_cid_name);
			}
			else
				CMBS_Api_InBound_Ring_CallerID(chid, str_cid, str_cid_name);

		}
		else	// Just Ring FXS
		{
			if (g_bDisableRingFXS)
			{
				// quiet mode
				return 0;
			}

			if (bRinging == 1)
			{
				CMBS_Api_InBound_Ring_CallerID(chid, NULL, NULL);
			}
			else
			{
				if(CMBS_Api_LineStateGet(chid) == 2)//ringing
				{
					CMBS_Api_CallRelease(chid, 0);
				}
			}
		}
#endif

		return 0;
	}
	else
#endif
	//SLIC Channel
	{
	if (str_cid[0] != 0)//Send CID and Ring FXS by DSP
	{
		if (cid_mode == 0)	// DTMF CID
		{
			ret = rtk_GenDtmfCid(chid, str_cid);
		}
#if 0
		else	// FSK CID
		{
			ret = rtk_GenFskCid(chid, str_cid, str_date, str_cid_name, fsk_type);
		}
#else 
        else  // FSK CID + Display name
        {
            /* Set the FSK Caller ID Parameters */
			memset(&clid, 0, sizeof(TstFskClid));
			clid.ch_id = chid;

			/* Set the FSK Caller ID Parameters-service type 1 */
			clid.service_type = fsk_type; 

			/* Set the FSK Caller ID Parameters-DATE */
			clid.cid_data[0].type = FSK_PARAM_DATEnTIME;
            if (str_date != NULL) 
                strcpy(clid.cid_data[0].data, str_date);

			/* Set the FSK Caller ID Parameters-Calling Line Identify (CLI) */
			clid.cid_data[1].type = FSK_PARAM_CLI;
            if (str_cid != NULL)
                strcpy(clid.cid_data[1].data, str_cid);	

			/* Set the FSK Caller ID Parameters-Calling Line Identify (CLI) Name */
			clid.cid_data[2].type = FSK_PARAM_CLI_NAME;
            if (str_cid_name != NULL)
                strcpy(clid.cid_data[2].data, str_cid_name);	
			
			/* Only 3 elements for Caller ID data. Set other element to 0 (MUST) */
			clid.cid_data[3].type = 0;
			clid.cid_data[4].type = 0;

            rtk_GenMdmfFskCid(chid, &clid, 3);
        }
#endif

		return ret;
	}
	else	// Just Ring FXS
	{
		if (g_bDisableRingFXS)
		{
			// quiet mode
			return 0;
		}

		stVoipSlicRing.ch_id = chid;
		stVoipSlicRing.ring_set = bRinging;
		SETSOCKOPT(VOIP_MGR_SLIC_RING, &stVoipSlicRing, TstVoipSlicRing, 1);

		return 0;//stVoipSlicRing.ret_val;
	}
	}

	return 0;
}


#if 0
// no action reacts to this function
int32 rtk_Gen_FSK_ALERT(uint32 chid, char *str_cid)
{
	TstVoipCID stCIDstr;

	stCIDstr.ch_id = chid;
	strcpy(stCIDstr.string, str_cid);
	SETSOCKOPT(VOIP_MGR_FSK_ALERT_GEN_CFG, &stCIDstr, TstVoipCID, 1);

	return 0;
}
#endif

int32 rtk_GenFskVmwi(uint32 chid, TstFskClid* pVmwi, uint32 num_vmwi_element)
{
	uint32 tmp=-1;

#ifdef VOIP_TAPI_DBG_DUMP
	int i ;

	if( pVmwi==NULL ){
		TAPI_DUMP( "chid=%u num_vmwi_element=%u pVmwi is NULL\n" , chid , num_vmwi_element );
	}
	else{
		TAPI_DUMP( "chid=%u \n" , chid );
		for( i=0 ; i<FSK_MDMF_SIZE ; i++ )
			TAPI_DUMP( "pVmwi->cid_data[%d].type=0x%08x pVmwi->cid_data[%d].data=%s \n" ,
				i , pVmwi->cid_data[i].type , i , pVmwi->cid_data[i].data);
	}
#endif

	rtk_GetFskCidState(chid, &tmp);

	if (tmp)
	{
		//printf("not end");
		return -2; /* don't double send caller id/vmwi when sending */
	}

	if( pVmwi->service_type == 0 )	// on-hook
		rtk_EnablePcm(chid, 1); // enable PCM before generating VMWI

	SETSOCKOPT(VOIP_MGR_SET_FSK_VMWI_STATE, pVmwi, TstFskClid, 1);

	return 0;//stCIDstr.ret_val;

}

int32 rtk_SetFskArea(uint32 chid, uint32 area)   /* area -> 0:Bellcore 1:ETSI 2:BT 3:NTT */
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u area=%u\n" , chid , area);

	/*
	 *	In "Auto SLIC Action" case, remember to call rtk_StopCid(), 
	 *	or SLIC ring action can't be stopped.
	 */

	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = area;

	SETSOCKOPT(VOIP_MGR_SET_FSK_AREA, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stCIDstr.ret_val;

}

int32 rtk_Set_FSK_CLID_Para(TstVoipFskPara* para)
{
	TstVoipFskPara stVoipFskPara;
	memset(&stVoipFskPara, 0x0, sizeof(TstVoipFskPara));
#ifdef VOIP_TAPI_DBG_DUMP
	if( para==NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP(
		"ch_id=%u "
		"area=%d "
		"CS_cnt=%u "
		"mark_cnt=%u "
		"mark_gain=%d "
		"space_gain=%d "
		"type2_expected_ack_tone=%u "
		"delay_after_1st_ring=%u "
		"delay_before_2nd_ring=%u "
		"silence_before_sas=%u "
		"sas_time=%u "
		"delay_after_sas=%u "
		"cas_time=%u "		
		"type1_delay_after_cas=%u "
		"ack_waiting_time=%u "
		"delay_after_ack_recv=%u "
		"delay_after_type2_fsk=%u "
		"RPAS_Duration=%u "
		"RPAS2FSK_Period=%u "
		"\n"
		,para->ch_id					
		,para->area					
		,para->CS_cnt					
		,para->mark_cnt				
		,para->mark_gain				
		,para->space_gain				
		,para->type2_expected_ack_tone	
		,para->delay_after_1st_ring	
		,para->delay_before_2nd_ring	
		,para->silence_before_sas		
		,para->sas_time				
		,para->delay_after_sas			
		,para->cas_time				
		,para->type1_delay_after_cas	
		,para->ack_waiting_time		
		,para->delay_after_ack_recv	
		,para->delay_after_type2_fsk	
		,para->RPAS_Duration			
		,para->RPAS2FSK_Period
		);
	}
#endif


	memcpy(&stVoipFskPara, para, sizeof(TstVoipFskPara));

	SETSOCKOPT(VOIP_MGR_SET_FSK_CLID_PARA, &stVoipFskPara, TstVoipFskPara, 1);

	return 0;//stVoipFskPara.ret_val;

}

int32 rtk_Get_FSK_CLID_Para(TstVoipFskPara* para)
{
	TstVoipFskPara stVoipFskPara;
	memset(&stVoipFskPara, 0x0, sizeof(TstVoipFskPara));
#ifdef VOIP_TAPI_DBG_DUMP
	if( para == NULL ){
		TAPI_DUMP( "para is NULL\n" );
	}
#endif

	stVoipFskPara.ch_id = para->ch_id;
	stVoipFskPara.area = para->area;

	SETSOCKOPT(VOIP_MGR_GET_FSK_CLID_PARA, &stVoipFskPara, TstVoipFskPara, 1);
	memcpy(para, &stVoipFskPara, sizeof(TstVoipFskPara));

#ifdef VOIP_TAPI_DBG_DUMP
	if( para != NULL ){
		TAPI_DUMP(
		"ch_id=%u "
		"area=%d "
		"CS_cnt=%u "
		"mark_cnt=%u "
		"mark_gain=%d "
		"space_gain=%d "
		"type2_expected_ack_tone=%u "
		"delay_after_1st_ring=%u "
		"delay_before_2nd_ring=%u "
		"silence_before_sas=%u "
		"sas_time=%u "
		"delay_after_sas=%u "
		"cas_time=%u "		
		"type1_delay_after_cas=%u "
		"ack_waiting_time=%u "
		"delay_after_ack_recv=%u "
		"delay_after_type2_fsk=%u "
		"RPAS_Duration=%u "
		"RPAS2FSK_Period=%u "
		"\n"
		,para->ch_id					
		,para->area					
		,para->CS_cnt					
		,para->mark_cnt				
		,para->mark_gain				
		,para->space_gain				
		,para->type2_expected_ack_tone	
		,para->delay_after_1st_ring	
		,para->delay_before_2nd_ring	
		,para->silence_before_sas		
		,para->sas_time				
		,para->delay_after_sas			
		,para->cas_time				
		,para->type1_delay_after_cas	
		,para->ack_waiting_time		
		,para->delay_after_ack_recv	
		,para->delay_after_type2_fsk	
		,para->RPAS_Duration			
		,para->RPAS2FSK_Period
		);
	}
#endif

	return 0;//stVoipFskPara.ret_val;

}

#if 0
/*
 * unused
 */
int32 rtk_SetRunRing(char* number)
{
	return -1;
}
#endif

int32 rtk_HoldRtp(uint32 chid, uint32 sid, uint32 enable)
{
    TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    TAPI_DUMP("chid=%u sid=%u enable=%u\n" , chid , sid , enable);

#if 0
    stVoipCfg.ch_id = chid;
    stVoipCfg.m_id = sid;
    stVoipCfg.enable = enable;
    SETSOCKOPT(VOIP_MGR_HOLD, &stVoipCfg, TstVoipCfg, 1);
#else
    stVoipCfg.ch_id = chid;
    stVoipCfg.m_id = sid;
    stVoipCfg.enable = !enable;
    SETSOCKOPT(VOIP_MGR_RTP_CFG, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;

#endif

}

int32 rtk_enable_pcm(uint32 chid, int32 bEnable)
{
    TstVoipCfg stVoipCfg;
    TstDtmfDetPara stDtmfDetPara;
	memset(&stDtmfDetPara, 0x0, sizeof(TstDtmfDetPara));
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    TAPI_DUMP("chid=%u bEnable=%d\n" , chid , bEnable);

    stVoipCfg.ch_id = chid;
    stVoipCfg.enable = bEnable;
    SETSOCKOPT(VOIP_MGR_PCM_CFG, &stVoipCfg, TstVoipCfg, 1);
	//if (stVoipCfg.ret_val != 0)
	//{
	//	return stVoipCfg.ret_val;
	//}

	stDtmfDetPara.ch_id = chid;
	stDtmfDetPara.enable = bEnable;
    stDtmfDetPara.dir = 0; /*dir0*/
    SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);
	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetDtmfCfg(uint32 chid, int32 bEnable, uint32 dir)
{
    TstDtmfDetPara stDtmfDetPara;
	memset(&stDtmfDetPara, 0x0, sizeof(TstDtmfDetPara));
    TAPI_DUMP("chid=%u bEnable=%d dir=%u\n" , chid , bEnable , dir);
	if((dir > 2) || (dir < 0)){
		TAPI_ERROR("dir out of range Available range: 0~2 \n");
		return -1;
	}

    stDtmfDetPara.ch_id = chid;
    stDtmfDetPara.enable = bEnable;
    stDtmfDetPara.dir = dir;
    SETSOCKOPT(VOIP_MGR_DTMF_CFG, &stDtmfDetPara, TstDtmfDetPara, 1);

    return 0;//stDtmfDetPara.ret_val;
}


int32 rtk_SetDtmfDetParam(uint32 chid, uint32 dir, int32 threshold, uint32 on_time_10ms, uint32 fore_twist, uint32 rev_twist,uint32 M) /* Threshold: 0 ~ 40, it means 0 ~ -40 dBm */
{
	TstDtmfDetPara stDtmfDetPara;
	memset(&stDtmfDetPara, 0x0, sizeof(TstDtmfDetPara));
	TAPI_DUMP("chid=%d dir=%u threshold=%d on_time_10ms=%u fore_twist=%u rev_twist=%u\n" , 
		chid , dir , threshold , on_time_10ms , fore_twist , rev_twist);
	if((threshold > 40) || (threshold < 0)){
		TAPI_ERROR("threshold out of range Available range: 0 to 40 ( 0 ~ -40 dBm) \n");
		return -1;
	}
	if((fore_twist > 12) || (fore_twist < 1)){
		TAPI_ERROR("fore_twist out of range Available range: 1 to 12  \n");
		return -1;
	}
	if((rev_twist > 12) || (rev_twist < 1)){
		TAPI_ERROR("rev_twist out of range Available range: 1 to 12  \n");
		return -1;
	}	
	stDtmfDetPara.ch_id = chid;
	stDtmfDetPara.dir = dir;
	stDtmfDetPara.thres_upd = 1;
	stDtmfDetPara.thres = threshold;
	stDtmfDetPara.on_time_upd = 1;
	stDtmfDetPara.on_time = on_time_10ms;
	stDtmfDetPara.twist_upd = 1;
	stDtmfDetPara.fore_twist = fore_twist;
	stDtmfDetPara.rev_twist = rev_twist;
	stDtmfDetPara.freq_offset_upd = 0; // no update freq.-offset flag
	stDtmfDetPara.M_upd= 1;
	stDtmfDetPara.M_offset= M; // no update freq.-offset flag
	
	SETSOCKOPT(VOIP_MGR_DTMF_DET_PARAM, &stDtmfDetPara, TstDtmfDetPara, 1);

	return 0;//stDtmfDetPara.ret_val;
}

int32 rtk_SetDtmfDetParamUpdate(TstDtmfDetPara* stDtmfDetPara)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( stDtmfDetPara == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP(
			"ch_id=%u m_id=%u t_id=%u enable=%u "
			"dir=%u thres_upd=%u thres=%u on_time_upd=%u "
			"on_time=%u twist_upd=%u fore_twist=%u rev_twist=%u "
			"freq_offset_upd=%u freq_offset=%u\n"
			,stDtmfDetPara->ch_id
			,stDtmfDetPara->m_id
			,stDtmfDetPara->t_id
			,stDtmfDetPara->enable
			,stDtmfDetPara->dir
			,stDtmfDetPara->thres_upd
			,stDtmfDetPara->thres
			,stDtmfDetPara->on_time_upd
			,stDtmfDetPara->on_time
			,stDtmfDetPara->twist_upd
			,stDtmfDetPara->fore_twist
			,stDtmfDetPara->rev_twist
			,stDtmfDetPara->freq_offset_upd
			,stDtmfDetPara->freq_offset
		);	
	}
#endif

	SETSOCKOPT(VOIP_MGR_DTMF_DET_PARAM, stDtmfDetPara, TstDtmfDetPara, 1);

	return 0;//stDtmfDetPara.ret_val;
}

uint32 rtk_GetSlicRegVal(uint32 chid, uint32 reg, uint8 *regdata)
{
	TstVoipSlicReg stSlicReg;
	memset(&stSlicReg, 0x0, sizeof(TstVoipSlicReg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( regdata == NULL ){
		TAPI_DUMP( "chid=%u reg=%u regdata is NULL\n" , chid , reg );
	}
#endif

	stSlicReg.ch_id = chid;
	stSlicReg.reg_num = reg;
	stSlicReg.reg_len = sizeof(stSlicReg.reg_ary);

	SETSOCKOPT(VOIP_MGR_GET_SLIC_REG_VAL, &stSlicReg, TstVoipSlicReg, 1);

	memcpy(regdata,stSlicReg.reg_ary,MIN(16,stSlicReg.reg_len));

	//if (stSlicReg.ret_val != 0)
	//{
	//	return stSlicReg.ret_val;
	//}
	return stSlicReg.reg_len;
}

int rtk_GetDaaUsedByWhichSlic(uint32 chid)
{

	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u \n" , chid);

	stVoipValue.ch_id = chid;

	SETSOCKOPT(VOIP_MGR_GET_DAA_USED_BY_WHICH_SLIC, &stVoipValue, TstVoipValue, 1);

	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}

	return 0;//stVoipValue.value;

}

int rtk_DaaOnHook(uint32 chid)// for virtual DAA on-hook(channel ID is FXS channel)
{
	TAPI_DUMP("chid=%u \n" , chid);
	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	    	stVoipCfg.ch_id = chid;

		SETSOCKOPT(VOIP_MGR_DAA_ON_HOOK, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}
        }
	return 0;
}

int rtk_DaaOffHook(uint32 chid)// for virtual DAA off-hook(channel ID is FXS channel)
{
	TAPI_DUMP("chid=%u \n" , chid);
	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	    	stVoipCfg.ch_id = chid;

		SETSOCKOPT(VOIP_MGR_DAA_OFF_HOOK, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		return stVoipCfg.enable; /* 1: success, 0xff: line not connect or busy or not support */
	}
	else
	{
		printf("API rtk_DaaOffHook usage error.\n");
		return 0xff;
	}
}

int rtk_DaaRing(uint32 chid)
{
	TAPI_DUMP("chid=%u \n" , chid);
	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;
	    	stVoipCfg.ch_id = chid;

		SETSOCKOPT(VOIP_MGR_DAA_RING, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		return stVoipCfg.enable;
	}
	else
	{
		return 0;
	}
}

int32 rtk_SetCountry(uint8  tone_of_country)
{
	TstVoipValue stVoipValue;
	char country;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));

  	country = tone_of_country;
  	
  	TAPI_DUMP("country=%d \n" , country);

	stVoipValue.value = country;
	SETSOCKOPT(VOIP_MGR_SET_COUNTRY, &stVoipValue, TstVoipValue, 1);

	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}

	if (country == TONE_CUSTOMER)
	{
#if 0
		stVoipValue.value1 = voip_ptr->tone_of_custdial;
		stVoipValue.value2 = voip_ptr->tone_of_custring;
		stVoipValue.value3 = voip_ptr->tone_of_custbusy;
		stVoipValue.value4 = voip_ptr->tone_of_custwaiting;
#endif		
		stVoipValue.value1 = 0;
		stVoipValue.value2 = 1;
		stVoipValue.value3 = 2;
		stVoipValue.value4 = 3;
		SETSOCKOPT(VOIP_MGR_USE_CUST_TONE, &stVoipValue, TstVoipValue, 1);

		//if (stVoipValue.ret_val != 0)
		//{
		//	return stVoipValue.ret_val;
		//}
	}

	return 0;
}

int32 rtk_SetImpedanceIndex(SLIC_IMPEDANCE_INDEX index)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("index=%d \n" , index);

	if ( index <= SLIC_IMPEDANCE_INIT || index >= SLIC_IMPEDANCE_MAX )
		return -1;

	stVoipValue.value5 = index;
	SETSOCKOPT(VOIP_MGR_SET_IMPEDANCE_INDEX, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}


int32 rtk_SetCountryImpedance(_COUNTRY_ country)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("country=%d \n" , country);

	stVoipValue.value = country;
	SETSOCKOPT(VOIP_MGR_SET_COUNTRY_IMPEDANCE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetCountryTone(_COUNTRY_ country)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("country=%d \n" , country);

	stVoipValue.value = country;
	SETSOCKOPT(VOIP_MGR_SET_COUNTRY_TONE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetImpedance(uint16 preset)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("present=%u \n" , preset);

	stVoipValue.value = preset;
	SETSOCKOPT(VOIP_MGR_SET_IMPEDANCE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetDisTonePara(TstVoipdistonedet_parm *distonedet_ptr)
{
	TstVoipdistonedet_parm stVoipdistonedet_parm;
	memset(&stVoipdistonedet_parm, 0x0, sizeof(TstVoipdistonedet_parm));
#ifdef VOIP_TAPI_DBG_DUMP
	if( distonedet_ptr == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{
		TAPI_DUMP(
		"distone_num=%u d1Freq1=%u d1Freq2=%u d1Accur=%u "    
		"d1Level=%u d1ONup=%u d1ONlow=%u d1OFFup=%u "    
		"d1OFFlow=%u d2Freq1=%u d2Freq2=%u d2Accur=%u "
		"d2Level=%u d2ONup=%u d2ONlow=%u d2OFFup=%u "    
		"d2OFFlow=%u \n"
		,distonedet_ptr->distone_num , distonedet_ptr->tone1_frequency1 , distonedet_ptr->tone1_frequency2 , distonedet_ptr->tone1_accuracy    
		,distonedet_ptr->tone1_level , distonedet_ptr->tone1_distone_on_up_limit , distonedet_ptr->tone1_distone_on_low_limit , distonedet_ptr->tone1_distone_off_up_limit    
		,distonedet_ptr->tone1_distone_off_low_limit , distonedet_ptr->tone2_frequency1 , distonedet_ptr->tone2_frequency2	, distonedet_ptr->tone2_accuracy    
		,distonedet_ptr->tone2_level	, distonedet_ptr->tone2_distone_on_up_limit , distonedet_ptr->tone2_distone_on_low_limit , distonedet_ptr->tone2_distone_off_up_limit    
		,distonedet_ptr->tone2_distone_off_low_limit	);	
	}
#endif


	stVoipdistonedet_parm.distone_num = distonedet_ptr->distone_num;

	stVoipdistonedet_parm.tone1_frequency1 = distonedet_ptr->tone1_frequency1;
	stVoipdistonedet_parm.tone1_frequency2 = distonedet_ptr->tone1_frequency2;
//	if ( distonedet_ptr->d1freqnum < 2)
//		stVoipdistonedet_parm.tone1_frequency2 = 0;
	stVoipdistonedet_parm.tone1_accuracy = distonedet_ptr->tone1_accuracy;
	stVoipdistonedet_parm.tone1_level = distonedet_ptr->tone1_level;
	stVoipdistonedet_parm.tone1_distone_on_up_limit = distonedet_ptr->tone1_distone_on_up_limit;
	stVoipdistonedet_parm.tone1_distone_on_low_limit = distonedet_ptr->tone1_distone_on_low_limit;
	stVoipdistonedet_parm.tone1_distone_off_up_limit = distonedet_ptr->tone1_distone_off_up_limit;
	stVoipdistonedet_parm.tone1_distone_off_low_limit = distonedet_ptr->tone1_distone_off_low_limit;

	stVoipdistonedet_parm.tone2_frequency1 = distonedet_ptr->tone2_frequency1;
	stVoipdistonedet_parm.tone2_frequency2 = distonedet_ptr->tone2_frequency2;
//	if ( distonedet_ptr->d2freqnum < 2)
	//	stVoipdistonedet_parm.tone2_frequency2 = 0;
	stVoipdistonedet_parm.tone2_accuracy = distonedet_ptr->tone2_accuracy;
	stVoipdistonedet_parm.tone2_level = distonedet_ptr->tone2_level;
	stVoipdistonedet_parm.tone2_distone_on_up_limit = distonedet_ptr->tone2_distone_on_up_limit;
	stVoipdistonedet_parm.tone2_distone_on_low_limit = distonedet_ptr->tone2_distone_on_low_limit;
	stVoipdistonedet_parm.tone2_distone_off_up_limit = distonedet_ptr->tone2_distone_off_up_limit;
	stVoipdistonedet_parm.tone2_distone_off_low_limit = distonedet_ptr->tone2_distone_off_low_limit;

	SETSOCKOPT(VOIP_MGR_SET_DIS_TONE_DET, &stVoipdistonedet_parm, TstVoipdistonedet_parm, 1);

	return 0;//stVoipdistonedet_parm.ret_val;

}

int32 rtk_SetCustomTone(uint8 custom, TstVoipToneCfg *pstToneCfgParam)
{
	TstVoipValue stVoipValue;
	TstVoipToneCfg stVoipToneCfg;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	memset(&stVoipToneCfg, 0x0, sizeof(TstVoipToneCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstToneCfgParam==NULL ){
		TAPI_DUMP( "custom=%u Input is NULL\n" , custom );
	}
	else{
		TAPI_DUMP( "custom=%u\n" , custom );
		dump_st_ToneCfgParam( pstToneCfgParam );
	}
#endif

	stVoipValue.value = custom;
	memcpy(&stVoipToneCfg, pstToneCfgParam, sizeof(TstVoipToneCfg));
	SETSOCKOPT(VOIP_MGR_SET_TONE_OF_CUSTOMIZE, &stVoipValue, TstVoipValue, 1);

	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}

	SETSOCKOPT(VOIP_MGR_SET_CUST_TONE_PARAM, &stVoipToneCfg, TstVoipToneCfg, 1);

	return 0;//stVoipToneCfg.ret_val;

}

 /*
 TstVoipValue.value1 Customer dial tone
 TstVoipValue.value2 Customer ringing tone
 TstVoipValue.value3 Customer busy tone
 TstVoipValue.value4 Customer call waiting tone
 */

int32 rtk_UseCustomTone(uint8 dial, uint8 ringback, uint8 busy, uint8 waiting)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP(
		"dial=%u ringback=%u busy=%u waiting=%u \n"
		, dial , ringback , busy , waiting );

	stVoipValue.value1 = dial;
	stVoipValue.value2 = ringback;
	stVoipValue.value3 = busy;
	stVoipValue.value4 = waiting;

	SETSOCKOPT(VOIP_MGR_USE_CUST_TONE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetUpdateTone(uint8 update_country,uint8 update_tone, TstVoipToneCfg *pstToneCfgParam)
{
	TstVoipValue stVoipValue;
	TstVoipToneCfg stVoipToneCfg;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	memset(&stVoipToneCfg, 0x0, sizeof(TstVoipToneCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	TAPI_DUMP( "update_country=%u update_tone=%u\n" , update_country , update_tone );
	dump_st_ToneCfgParam( pstToneCfgParam );
#endif

	stVoipValue.value = update_country;
	stVoipValue.value1 = update_tone;
	memcpy(&stVoipToneCfg, pstToneCfgParam, sizeof(TstVoipToneCfg));
	SETSOCKOPT(VOIP_MGR_SET_TONE_OF_UPDATE, &stVoipValue, TstVoipValue, 1);

	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}

	SETSOCKOPT(VOIP_MGR_SET_UPDATE_TONE_PARAM, &stVoipToneCfg, TstVoipToneCfg, 1);

	return 0;//stVoipToneCfg.ret_val;

}

int rtk_SetSlicRegVal(int chid, int reg, int len, uint8 *regdata)
{
	//int i;
	TstVoipSlicReg stSlicReg;
	memset(&stSlicReg, 0x0, sizeof(TstVoipSlicReg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( regdata==NULL ){
		TAPI_DUMP( "chid=%d reg=%d len=%d reg is NULL\n" , chid , reg , len );
	}
#endif

	stSlicReg.ch_id = chid;
	stSlicReg.reg_num = reg;
	//stSlicReg.reg_val = value;
	stSlicReg.reg_len = len;
	memcpy(stSlicReg.reg_ary,regdata,MIN(len,sizeof(stSlicReg.reg_ary)));

	//for (i=0; i<MIN(argc,sizeof(stSlicReg.reg_ary));i++)
	//	stSlicReg.reg_ary[i] = atoi(argv[i]);

	SETSOCKOPT(VOIP_MGR_SET_SLIC_REG_VAL, &stSlicReg, TstVoipSlicReg, 1);

	return 0;//stSlicReg.ret_val;
}

int rtk_reset_slic(int chid, unsigned int law)
{
	TstVoipSlicRestart stVoipSlicRestart;
	memset(&stVoipSlicRestart, 0x0, sizeof(TstVoipSlicRestart));
	TAPI_DUMP("chid=%d law=%u\n" , chid , law );

	stVoipSlicRestart.ch_id = chid;
	stVoipSlicRestart.codec_law = law; // 0: linear, 1: A-law, 2:u-law
	SETSOCKOPT(VOIP_MGR_SLIC_RESTART, &stVoipSlicRestart, TstVoipSlicRestart, 1);

	return 0;//stVoipSlicRestart.ret_val;
}

int rtk_Set_PCM_Loop_Mode(char group, char mode, char main_ch, char mate_ch) //mode: 0- Not loop mode, 1- loop mode, 2- loop mode with VoIP
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("group=%d mode=%d main_ch=%d mate_ch=%d\n"
		, group , mode , main_ch , mate_ch );

	stVoipValue.value = group;
	stVoipValue.value1 = mode;
	stVoipValue.value2 = main_ch;
	stVoipValue.value3 = mate_ch;

	SETSOCKOPT(VOIP_MGR_SET_PCM_LOOP_MODE, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int rtk_SetFxsFxoLoopback(unsigned int chid, unsigned int enable)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u enable=%u\n" , chid , enable);

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = enable;

	SETSOCKOPT(VOIP_MGR_SET_FXS_FXO_LOOPBACK, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int rtk_SetFxsOnHookTransPcmOn(unsigned int chid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u \n" , chid);

	stVoipCfg.ch_id = chid;

	SETSOCKOPT(VOIP_MGR_SET_SLIC_ONHOOK_TRANS_PCM_START, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int rtk_debug_with_watchdog(int dbg_flag, int watchdog)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("dbg_flag=%d watchdog=%d\n" , dbg_flag , watchdog);

	stVoipValue.value = dbg_flag;
	stVoipValue.value1 = watchdog;	// 0 --> off, 1 --> on, others --> don't care
	SETSOCKOPT(VOIP_MGR_DEBUG, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int rtk_debug(int dbg_flag)
{
	TAPI_DUMP("dbg_flag=%d\n" , dbg_flag);
	return rtk_debug_with_watchdog( dbg_flag, 2 );
}

#if 1 //def CONFIG_RTK_VOIP_IVR
int rtk_IvrStartPlaying( unsigned int chid, unsigned int sid, IvrPlayDir_t dir, char *pszText2Speech )
{
	TstVoipPlayIVR_Text stVoipPlayIVR_Text;
	memset(&stVoipPlayIVR_Text, 0x0, sizeof(TstVoipPlayIVR_Text));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pszText2Speech==NULL ){
		TAPI_DUMP( "chid=%u sid=%u dir=%x pszText2Speech is NULL\n" , chid , sid ,dir );
	}
#endif

	stVoipPlayIVR_Text.ch_id = chid;
	stVoipPlayIVR_Text.m_id = sid;
	stVoipPlayIVR_Text.type = IVR_PLAY_TYPE_TEXT;
	stVoipPlayIVR_Text.dir = dir;
	memcpy( stVoipPlayIVR_Text.szText2speech, pszText2Speech, MAX_LEN_OF_IVR_TEXT );
	stVoipPlayIVR_Text.szText2speech[ MAX_LEN_OF_IVR_TEXT ] = '\x0';

	SETSOCKOPT( VOIP_MGR_PLAY_IVR, &stVoipPlayIVR_Text, TstVoipPlayIVR_Text, 1 );

	return ( int )stVoipPlayIVR_Text.playing_period_10ms;
}

int rtk_IvrStartPlayG72363( unsigned int chid, unsigned int sid, IvrPlayDir_t dir, unsigned int nFrameCount, const unsigned char *pData )
{
	TstVoipPlayIVR_G72363 stVoipPlayIVR;
	memset(&stVoipPlayIVR, 0x0, sizeof(TstVoipPlayIVR_G72363));
	TAPI_DUMP("chid=%u sid=%u dir=%d nFrameCount=%u pData=%p\n"
		, chid , sid , dir , nFrameCount , pData);

	if( nFrameCount > MAX_FRAMES_OF_G72363 )
		nFrameCount = MAX_FRAMES_OF_G72363;

	stVoipPlayIVR.ch_id = chid;
	stVoipPlayIVR.m_id = sid;
	stVoipPlayIVR.type = IVR_PLAY_TYPE_G723_63;
	stVoipPlayIVR.dir = dir;
	stVoipPlayIVR.nFramesCount = nFrameCount;
	memcpy( stVoipPlayIVR.data, pData, nFrameCount * 24 );

    SETSOCKOPT(VOIP_MGR_PLAY_IVR, &stVoipPlayIVR, TstVoipPlayIVR_G72363, 1);

    return stVoipPlayIVR.nRetCopiedFrames;
}

int rtk_IvrStartPlayG729( unsigned int chid, unsigned int sid, IvrPlayDir_t dir, unsigned int nFrameCount, const unsigned char *pData )
{
	TstVoipPlayIVR_G729 stVoipPlayIVR;
	memset(&stVoipPlayIVR, 0x0, sizeof(TstVoipPlayIVR_G729));
	TAPI_DUMP("chid=%u sid=%u dir=%d nFrameCount=%u pData=%p\n"
		, chid , sid , dir , nFrameCount , pData);


	if( nFrameCount > MAX_FRAMES_OF_G729 )
		nFrameCount = MAX_FRAMES_OF_G729;

	stVoipPlayIVR.ch_id = chid;
	stVoipPlayIVR.m_id = sid;
	stVoipPlayIVR.type = IVR_PLAY_TYPE_G729;
	stVoipPlayIVR.dir = dir;
	stVoipPlayIVR.nFramesCount = nFrameCount;
	memcpy( stVoipPlayIVR.data, pData, nFrameCount * 10 );

    SETSOCKOPT(VOIP_MGR_PLAY_IVR, &stVoipPlayIVR, TstVoipPlayIVR_G729, 1);

    return stVoipPlayIVR.nRetCopiedFrames;
}

int rtk_IvrPollPlaying( unsigned int chid, unsigned int sid)
{
	TstVoipPollIVR stVoipPollIVR;
	memset(&stVoipPollIVR, 0x0, sizeof(TstVoipPollIVR));
	TAPI_DUMP("chid=%u sid=%u\n" , chid , sid);

	stVoipPollIVR.ch_id = chid;
	stVoipPollIVR.m_id = sid;

	SETSOCKOPT( VOIP_MGR_POLL_IVR, &stVoipPollIVR, TstVoipPollIVR, 1 );

	return ( int )stVoipPollIVR.bPlaying;
}

int rtk_IvrStopPlaying( unsigned int chid, unsigned int sid)
{
	TstVoipStopIVR stVoipStopIVR;
	memset(&stVoipStopIVR, 0x0, sizeof(TstVoipStopIVR));
	TAPI_DUMP("chid=%u sid=%u\n" , chid , sid);

	stVoipStopIVR.ch_id = chid;
	stVoipStopIVR.m_id = sid;

	SETSOCKOPT( VOIP_MGR_STOP_IVR, &stVoipStopIVR, TstVoipStopIVR, 1 );

	return 0;
}
#endif /* CONFIG_RTK_VOIP_IVR */

int rtk_SipRegister(unsigned int chid, unsigned int isOK)
{
   	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
   	TAPI_DUMP("chid=%u isOK=%u\n" , chid , isOK);

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = isOK;
#if 0 //#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
	g_sip_register[chid] = isOK;
#endif
	SETSOCKOPT(VOIP_MGR_SIP_REGISTER, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SipInfoPlayTone(unsigned int chid, unsigned int ssid, DSPCODEC_TONE tone, unsigned int duration)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u ssid=%u tone=%d duration=%u\n" , chid , ssid , tone , duration);

	stVoipValue.ch_id = chid;
	stVoipValue.m_id = ssid;
	stVoipValue.value = tone;
	stVoipValue.value5 = duration;
	stVoipValue.value7=0;//2 0: local 2:remote


	SETSOCKOPT(VOIP_MGR_PLAY_SIP_INFO, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SipPlayToneRemote(unsigned int chid, unsigned int ssid, DSPCODEC_TONE tone, unsigned int duration)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u ssid=%u tone=%d duration=%u \n" , chid , ssid , tone , duration );

	stVoipValue.ch_id = chid;
	stVoipValue.m_id = ssid;
	stVoipValue.value = tone;
	stVoipValue.value5 = duration;
	stVoipValue.value7=2;//2 0: local 2:remote

	
	SETSOCKOPT(VOIP_MGR_PLAY_SIP_INFO, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetSpkAgc(uint32 chid, uint32 support_gain, uint32 adaptive_threshold)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u support_gain=%u adaptive_threshold=%u\n" , 
		chid , support_gain , adaptive_threshold);
	if((adaptive_threshold > 70) || (adaptive_threshold < 0)){
		TAPI_ERROR("adaptive_threshold out of range Available range: 0 to 70 (0 ~ -70dB) \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = support_gain;
	stVoipValue.value1 = adaptive_threshold;

	SETSOCKOPT(VOIP_MGR_SET_SPK_AGC, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetSpkAgcLvl(uint32 chid, uint32 level)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u level=%u\n" , chid , level);
	if((level > 8) || (level < 0)){
		TAPI_ERROR("level out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = level;

	SETSOCKOPT(VOIP_MGR_SET_SPK_AGC_LVL, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}


int32 rtk_SetSpkAgcGup(uint32 chid, uint32 gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u gain=%u\n" , chid , gain);

	if((gain > 8) || (gain < 0)){
		TAPI_ERROR("gain out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_SPK_AGC_GUP, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}


int32 rtk_SetSpkAgcGdown(uint32 chid, uint32 gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u gain=%u\n" , chid , gain);
	if((gain > 8) || (gain < 0)){
		TAPI_ERROR("gain out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_SPK_AGC_GDOWN, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}


int32 rtk_SetMicAgc(uint32 chid, uint32 support_gain, uint32 adaptive_threshold)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u support_gain=%u adaptive_threshold=%u\n" 
		, chid , support_gain , adaptive_threshold);
	if((adaptive_threshold > 70) || (adaptive_threshold < 0)){
		TAPI_ERROR("adaptive_threshold out of range Available range: 0 to 70 (0 ~ -70dB) \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = support_gain;
	stVoipValue.value1 = adaptive_threshold;

	SETSOCKOPT(VOIP_MGR_SET_MIC_AGC, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetMicAgcLvl(uint32 chid, uint32 level)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u level=%u\n" , chid , level);
	if((level > 8) || (level < 0)){
		TAPI_ERROR("level out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = level;

	SETSOCKOPT(VOIP_MGR_SET_MIC_AGC_LVL, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetMicAgcGup(uint32 chid, uint32 gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u gain=%u\n" , chid , gain);
	if((gain > 8) || (gain < 0)){
		TAPI_ERROR("gain out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_MIC_AGC_GUP, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}


int32 rtk_SetMicAgcGdown(uint32 chid, uint32 gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u gain=%u\n" , chid , gain);
	if((gain > 8) || (gain < 0)){
		TAPI_ERROR("gain out of range Available range: 0~8 \n");
		return -1;
	}
	stVoipValue.ch_id = chid;
	stVoipValue.value = gain;

	SETSOCKOPT(VOIP_MGR_SET_MIC_AGC_GDOWN, &stVoipValue, TstVoipValue, 1);


	return 0;//stVoipValue.ret_val;
}

#if 0

int32 rtk_GetDaaIsrFlow(unsigned int chid ,unsigned int mid)
{
	unsigned int flow;
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.m_id = mid;

	SETSOCKOPT(VOIP_MGR_GET_DAA_ISR_FLOW, &stVoipValue, TstVoipValue, 1);
	flow = stVoipValue.value;

	return flow;
}

/* Usage:
	  rtk_SetDaaIsrFlow(chid, DAA_FLOW_NORMAL)
	  rtk_SetDaaIsrFlow(chid, DAA_FLOW_3WAY_CONFERENCE)
	  rtk_SetDaaIsrFlow(chid, DAA_FLOW_CALL_FORWARD)
*/
int32 rtk_SetDaaIsrFlow(unsigned int chid, unsigned int mid, unsigned int flow)
{
	TstVoipValue stVoipValue;
	int res;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	stVoipValue.ch_id = chid;
	stVoipValue.m_id = mid;
	stVoipValue.value = flow;


	if( flow == 0 ) /* Normal */
	{
		if (rtk_GetDaaIsrFlow(chid, mid) == DAA_FLOW_CALL_FORWARD)
			rtk_EnablePcm(chid, 0);
		rtk_DaaOnHook(chid);
		res = 1;
	}
	else if ( flow == 1 ) /* PSTN 3-way conference */
	{
		res = rtk_DaaOffHook(chid);

		if (res == 0xff)
			stVoipValue.value = DAA_FLOW_NORMAL;
	}
	else if ( flow == 2 ) /* PSTN call forward */
	{
		rtk_EnablePcm(chid, 1);
		res = rtk_DaaOffHook(chid);

		if (res == 0xff)
		{
			rtk_EnablePcm(chid, 0);
			stVoipValue.value = DAA_FLOW_NORMAL;
		}
	}

	SETSOCKOPT(VOIP_MGR_SET_DAA_ISR_FLOW, &stVoipValue, TstVoipValue, 1);


	return res; /* 1: success, 0xff: line not connect or busy or not support */
}

#endif

int32 rtk_DialPstnCallForward(uint32 chid, uint32 sid, char *cf_no_str)
{
   	char cf_no[21];
   	int len = strlen(cf_no_str), i;

	TAPI_DUMP("chid=%u sid=%u cf_no_str=%p\n" , chid , sid , cf_no_str);

   	strcpy(cf_no, cf_no_str);

   	//usleep(200000);  // 200ms
   	usleep(250000);	// ok
        printf("PSTN Call Forward Dial: ");
   	for(i = 0; i < len; i++)
   	{
   		rtk_SetPlayTone(chid, sid, cf_no[i]-'0', 1, DSPCODEC_TONEDIRECTION_LOCAL);
                printf("%d ", cf_no[i]-'0');
   		usleep(100000);  // 100ms
   		rtk_SetPlayTone(chid, sid, cf_no[i]-'0', 0, DSPCODEC_TONEDIRECTION_LOCAL);
   		usleep(50000);  // 50ms
   	}
        printf("\n");

	return 0;
}

#if 0

int32 rtk_SetPstnHoldCfg(unsigned int slic_chid, unsigned int daa_chid, unsigned int config)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	stVoipValue.ch_id = slic_chid;
	stVoipValue.value2 = daa_chid;
	stVoipValue.value = config; /* 1: Hold, 0: Un-Hold*/


	SETSOCKOPT(VOIP_MGR_SET_DAA_PCM_HOLD_CFG, &stVoipValue, TstVoipValue, 1);


	return 0;
}

int32 rtk_GetDaaBusyToneStatus(unsigned int daa_chid)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	int busy_flag;	/* Busy tone is  1: Detected, 0: NOT detected. */

	stVoipValue.ch_id = daa_chid;

	SETSOCKOPT(VOIP_MGR_GET_DAA_BUSY_TONE_STATUS, &stVoipValue, TstVoipValue, 1);
	busy_flag = stVoipValue.value;

	return busy_flag;
}

#endif

int32 rtk_GetDaaCallerID(uint32 chid, char *str_cid, char *str_date, char *str_name)
{
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
	TAPI_DUMP("chid=%u str_cid=%s str_date=%s str_name=%s\n"
		,chid , str_cid , str_date , str_name);

	stCIDstr.daa_id = chid;

	SETSOCKOPT(VOIP_MGR_GET_DAA_CALLER_ID, &stCIDstr, TstVoipCID, 1);

	//if (stCIDstr.ret_val != 0)
	//{
	//	return stCIDstr.ret_val;
	//}

	strcpy(str_cid, stCIDstr.string);
	strcpy(str_date, stCIDstr.string2);
	strcpy(str_name, stCIDstr.cid_name);

	return 0;
}

int32 rtk_GetVoIPFeature(void)
{
#if 0
	if( g_VoIP_Feature )
		return 0;	// If not zero, ignore it.
#endif

	//printf( "VOIP_MGR_GET_FEATURE=%d\n", VOIP_MGR_GET_FEATURE );
	
#if 0	
   	TstVoipFeature stVoipFeature;
	SETSOCKOPT(VOIP_MGR_GET_FEATURE, &stVoipFeature, TstVoipFeature, 1);
	memcpy( &g_VoIP_Feature, &stVoipFeature, sizeof( TstVoipFeature ) );
#else
	VoipFeature_t feature = 0;
	uint32 slic = 0, daa = 0, dect = 0, phone = 0;

	// platform 
	feature |= RTK_VOIP_PLATFORM_FEATURE;
	
	// misc 
	feature |= RTK_VOIP_ONE_ARM_ROUTER_FEATURE;
	
	slic = CONFIG_RTK_VOIP_CON_CH_NUM;

	feature |= ( ( slic  << SLIC_NUM_SHIFT  ) & SLIC_NUM_MASK );
	feature |= ( ( daa   << DAA_NUM_SHIFT   ) & DAA_NUM_MASK  );
	feature |= ( ( dect  << DECT_NUM_SHIFT  ) & DECT_NUM_MASK );
	feature |= ( ( phone << PHONE_NUM_SHIFT ) & PHONE_NUM_MASK );
	
	feature |= RTK_VOIP_DAA_TYPE_FEATURE;
	
	// dsp
	feature |= RTK_VOIP_IVR_FEATURE;
	feature |= RTK_VOIP_MW_REALTEK_FEATURE;
	feature |= RTK_VOIP_MW_AUDIOCODES_FEATURE;
	
	// arch
	feature |= RTK_VOIP_IPC_ARCH_FEATURE;
	feature |= RTK_VOIP_IPC_ARCH_TYPE_FEATURE;
	feature |= RTK_VOIP_IPC_ARCH_ROLE_FEATURE;
	feature |= RTK_VOIP_DSP_DEVICE_NUM_FEATURE;
	
	// codec 
	feature |= RTK_VOIP_CODEC_FEATURE;
	
	g_VoIP_Feature = feature;
#endif	

	g_VoIP_Ports = RTK_VOIP_CH_NUM( g_VoIP_Feature );

	//printf("rtk_GetVoIPFeature: 0x%llx \n", *( ( uint64 * )( void * )&g_VoIP_Feature ) );

#if 0
	printf( "\tRTK_VOIP_SLIC_NUM=%d\n", RTK_VOIP_SLIC_NUM( g_VoIP_Feature ) );
	printf( "\tRTK_VOIP_DAA_NUM=%d\n", RTK_VOIP_DAA_NUM( g_VoIP_Feature ) );
	printf( "\tRTK_VOIP_DECT_NUM=%d\n", RTK_VOIP_DECT_NUM( g_VoIP_Feature ) );
	printf( "\tRTK_VOIP_CH_NUM=%d\n", RTK_VOIP_CH_NUM( g_VoIP_Feature ) );
	if( RTK_VOIP_PLATFORM_CHK_IS8672( g_VoIP_Feature ) )
		printf( "\tPLATFORM 8672\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS865xC( g_VoIP_Feature ) )
		printf( "\tPLATFORM 865xC\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS8972B( g_VoIP_Feature ) )
		printf( "\tPLATFORM 8972B\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS89xxC( g_VoIP_Feature ) )
		printf( "\tPLATFORM 89xxC\n" );
	else
		printf( "\tPLATFORM unknown: %llX\n", ((g_VoIP_Feature) & RTK_VOIP_PLATFORM_MASK) );
#endif

#if 0
	TAPI_DUMP( "\tRTK_VOIP_SLIC_NUM=%d\n", RTK_VOIP_SLIC_NUM( g_VoIP_Feature ) );
	TAPI_DUMP( "\tRTK_VOIP_DAA_NUM=%d\n", RTK_VOIP_DAA_NUM( g_VoIP_Feature ) );
	TAPI_DUMP( "\tRTK_VOIP_DECT_NUM=%d\n", RTK_VOIP_DECT_NUM( g_VoIP_Feature ) );
	TAPI_DUMP( "\tRTK_VOIP_CH_NUM=%d\n", RTK_VOIP_CH_NUM( g_VoIP_Feature ) );
	if( RTK_VOIP_PLATFORM_CHK_IS8672( g_VoIP_Feature ) )
		TAPI_DUMP( "\tPLATFORM 8672\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS865xC( g_VoIP_Feature ) )
		TAPI_DUMP( "\tPLATFORM 865xC\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS8972B( g_VoIP_Feature ) )
		TAPI_DUMP( "\tPLATFORM 8972B\n" );
	else if( RTK_VOIP_PLATFORM_CHK_IS89xxC( g_VoIP_Feature ) )
		TAPI_DUMP( "\tPLATFORM 89xxC\n" );
	else
		TAPI_DUMP( "\tPLATFORM unknown: %llX\n", ((g_VoIP_Feature) & RTK_VOIP_PLATFORM_MASK) );
#endif

	return 0;
}

int32 rtk_SetCidDetMode(uint32 chid, int auto_det, int cid_det_mode)
{
   	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
   	TAPI_DUMP("chid=%u auto_det=%d cid_det_mode=%d\n"
   		,chid , auto_det , cid_det_mode);

   	stVoipCfg.ch_id = chid;
   	stVoipCfg.enable = auto_det; /* 0: disable 1: enable(NTT) 2: enable (NOT NTT) */
   	stVoipCfg.cfg = cid_det_mode;

	SETSOCKOPT(VOIP_MGR_SET_CID_DET_MODE, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_GetFskCidState(uint32 chid, uint32 *cid_state)
{
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
#ifdef VOIP_TAPI_DBG_DUMP
	if( cid_state==NULL ){
		TAPI_DUMP("chid=%u , cid_state=%p\n" , chid , cid_state);		
	}
	else
		TAPI_DUMP("chid=%u , *cid_state=%u\n" , chid , *cid_state);		
#endif

	stCIDstr.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_GET_FSK_CID_STATE_CFG, &stCIDstr, TstVoipCID, 1);
	*cid_state = stCIDstr.cid_state;

	return 0;//stCIDstr.ret_val;
}

int32 rtk_SetCidFskGenMode(unsigned int chid, unsigned int isOK)
{
   	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
   	TAPI_DUMP("chid=%u isOK=%u\n" , chid , isOK);

	stVoipCfg.ch_id = chid;
	stVoipCfg.enable = isOK;
	SETSOCKOPT(VOIP_MGR_SET_CID_FSK_GEN_MODE, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_SetVoiceGain(uint32 chid, int spk_gain, int mic_gain)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u spk_gain=%d mic_gain=%d\n" , chid , spk_gain , mic_gain);
	if((spk_gain > 31) || (spk_gain < -32)){
		TAPI_ERROR("spk_gain out of range Available range: -32~31  \n");
		return -1;
	}
	if((mic_gain > 31) || (mic_gain < -32)){
		TAPI_ERROR("mic_gain out of range Available range: -32~31  \n");
		return -1;
	}	
	stVoipValue.ch_id = chid;
	stVoipValue.value = spk_gain;
	stVoipValue.value1 = mic_gain;

	SETSOCKOPT(VOIP_MGR_SET_VOICE_GAIN, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

//int32 rtk_SetAnswerToneDet(uint32 chid, uint32 config, int32 threshold)
int32 rtk_SetAnswerToneDet(uint32 chid, uint32 config_1, uint32 config_2, int32 threshold)	// duncan
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u config_1=%u config_2=%u threshold=%d\n" 
		, chid , config_1 , config_2 , threshold);
	if((threshold > 13) || (threshold < 0)){
		TAPI_ERROR("threshold out of range The answer tone detecting threshold. Available range: 0 ~ 13  \n");
		return -1;
	}	
	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = config_1;
	stVoipCfg.cfg2 = config_2;
	stVoipCfg.cfg3 = threshold;
	
	SETSOCKOPT(VOIP_MGR_SET_ANSWERTONE_DET, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_SetSilenceDetThreshold(uint32 chid, uint32 energy, uint32 period)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u energey=%u period=%u\n" , chid , energy , period);
	if((energy > 127) || (energy < 0)){
		TAPI_ERROR("energy out of range Available range: 0 ~ 127 (indicates 0dB ~ -127dB)  \n");
		return -1;
	}	
	// mid = 0
	stVoipCfg.ch_id = chid;   
	stVoipCfg.m_id = 0;
	stVoipCfg.cfg = energy;
	stVoipCfg.cfg2 = period;
	
	SETSOCKOPT(VOIP_MGR_SET_FAX_SILENCE_DET, &stVoipCfg, TstVoipCfg, 1);

	// mid = 1
	stVoipCfg.ch_id = chid;   
	stVoipCfg.m_id = 1;
	stVoipCfg.cfg = energy;
	stVoipCfg.cfg2 = period;
	
	SETSOCKOPT(VOIP_MGR_SET_FAX_SILENCE_DET, &stVoipCfg, TstVoipCfg, 1);

	return 0;
}

int32 rtk_GenFskCidVmwi(uint32 chid, char *str_cid, char *str_date, char *str_cid_name, char mode, char msg_type)
{
#if 1
	printf("Error, API %s is not workable.\n", __FUNCTION__);
	return -1;
#else
	TstVoipCID stCIDstr;
	memset(&stCIDstr, 0x0, sizeof(TstVoipCID));
	uint32 tmp=-1;

	rtk_GetFskCidState(chid, &tmp);

	if (tmp)
	{
		//printf("not end");
		return -2; /* don't double send caller id/vmwi when sending */
	}

	if(!mode)			// on-hook
		rtk_EnablePcm(chid, 1); // enable PCM before generating VMWI

	stCIDstr.ch_id = chid;
	stCIDstr.cid_mode = mode;       //0:on-hook
	stCIDstr.cid_msg_type = msg_type;	// FSK_MSG_CALLSETUP:cid or FSK_MSG_MWSETUP:vmwi
	if (str_cid[0] == 0)               // not null str
		strcpy(stCIDstr.string, "0123456789");   // replace "0123456789" with From CID
	else
		strcpy(stCIDstr.string, str_cid);
	strcpy(stCIDstr.string2, str_date);
	strcpy(stCIDstr.cid_name, str_cid_name);

	SETSOCKOPT(VOIP_MGR_FSK_CID_VMWI_GEN_CFG, &stCIDstr, TstVoipCID, 1);
	// remember set slic in transmit mode, enable DSP pcm.
#endif
	return 0;//stCIDstr.ret_val;
}

int32 rtk_GetDspEvent(uint32 chid, uint32 mid, VoipEventID *pEvent, uint32 *pData)
{
	int ret;
	TstVoipEvent stVoipEvent;
	memset(&stVoipEvent, 0x0, sizeof(TstVoipEvent));
#ifdef VOIP_TAPI_DBG_DUMP
	if( (pEvent==NULL) || (pData==NULL) ){
		TAPI_DUMP("chid=%u mid=%u\n", chid , mid);
	}
#endif
	
	stVoipEvent.ch_id = chid;
	stVoipEvent.type = VET_DSP;
	stVoipEvent.mask = ( mid ? VEM_MID1 : VEM_MID0 );
	
	if( ( ret = rtk_GetVoIPEvent( &stVoipEvent ) ) < 0 )
		return ret;
	
	*pEvent = stVoipEvent.id;

	if (pData)
		*pData = stVoipEvent.p0;
		
#ifdef VOIP_TAPI_DBG_DUMP
	if( *pEvent != 0 )
		TAPI_DUMP("chid=%u mid=%u *pEvent=%d *pData=%u\n"
			, chid , mid , *pEvent , *pData);
#endif
	
	return 0;
}

int32 rtk_GetPhoneState(TstVoipCfg* pstVoipCfg)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstVoipCfg==NULL ){
		TAPI_DUMP( "pstVoipCfg is NULL\n" );
	}
#endif
	SETSOCKOPT(VOIP_MGR_GET_SLIC_STAT, pstVoipCfg, TstVoipCfg, 1);

	return 0;//pstVoipCfg->ret_val;
}

int32 rtk_Set_GETDATA_Mode(TstVoipdataget_o* pstVoipdataget_o)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstVoipdataget_o==NULL ){
		TAPI_DUMP( "pstVoipdataget_o is NULL\n" );
	}
#endif
	SETSOCKOPT(VOIP_MGR_SET_GETDATA_MODE, pstVoipdataget_o, TstVoipdataget_o, 1);

	return 0;//pstVoipdataget_o->ret_val;
}

int32 rtk_Set_Voice_Play(TstVoipdataput_o* pstVoipdataput_o)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstVoipdataput_o==NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
#endif
	SETSOCKOPT(VOIP_MGR_VOICE_PLAY, pstVoipdataput_o, TstVoipdataput_o, 1);

	return 0;//pstVoipdataput_o->ret_val;
}

int32 rtk_GetRtpRtcpStatistics( uint32 chid, uint32 mid, uint32 bReset, TstRtpRtcpStatistics *pstRtpRtcpStatistics )
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstRtpRtcpStatistics == NULL ){
		TAPI_DUMP( "chid=%u mid=%u bReset=%u pstRtpRtcpStatistics is NULL\n" , chid , mid , bReset );
	}
	else{
		TAPI_DUMP("chid=%u mid=%u bReset=%u\n" , chid , mid , bReset);	
	}		
#endif

	pstRtpRtcpStatistics ->ch_id = chid;
	pstRtpRtcpStatistics ->m_id = mid;
	pstRtpRtcpStatistics ->bResetStatistics = bReset;

	SETSOCKOPT( VOIP_MGR_GET_RTP_RTCP_STATISTICS, pstRtpRtcpStatistics, TstRtpRtcpStatistics, 1 );

#ifdef VOIP_TAPI_DBG_DUMP
	if( pstRtpRtcpStatistics != NULL ){
	TAPI_DUMP(
	"ch_id=%u "
	"m_id=%u "
	"bResetStatistics=%u "
	"nTxPkts=%u "
	"nTxBytes=%u "
	"nRxPkts=%u "
	"nRxBytes=%u "
	"nLost=%u "
	"nMaxFractionLost=%u "
	"nMinFractionLost=%u "
	"nAvgFractionLost=%u "
	"nCurFractionLost=%u "
	"nDiscarded=%u "
	"nTxRtcpPkts=%u "
	"nRxRtcpPkts=%u "
	"nTxRtcpXrPkts=%u "
	"nRxRtcpXrPkts=%u "
	"nOverRuns=%u "
	"nUnderRuns=%u "
	"nMaxJitter=%u "
	"nMinJitter=%u "
	"nAvgJitter=%u "
	"nCurJitter=%u "
	"nMaxRtcpTime=%u "
	"\n"
	,pstRtpRtcpStatistics->ch_id
	,pstRtpRtcpStatistics->m_id
	,pstRtpRtcpStatistics->bResetStatistics
	,pstRtpRtcpStatistics->nTxPkts
	,pstRtpRtcpStatistics->nTxBytes
	,pstRtpRtcpStatistics->nRxPkts
	,pstRtpRtcpStatistics->nRxBytes
	,pstRtpRtcpStatistics->nLost
	,pstRtpRtcpStatistics->nMaxFractionLost
	,pstRtpRtcpStatistics->nMinFractionLost
	,pstRtpRtcpStatistics->nAvgFractionLost
	,pstRtpRtcpStatistics->nCurFractionLost
	,pstRtpRtcpStatistics->nDiscarded
	,pstRtpRtcpStatistics->nTxRtcpPkts
	,pstRtpRtcpStatistics->nRxRtcpPkts
	,pstRtpRtcpStatistics->nTxRtcpXrPkts
	,pstRtpRtcpStatistics->nRxRtcpXrPkts
	,pstRtpRtcpStatistics->nOverRuns
	,pstRtpRtcpStatistics->nUnderRuns
	,pstRtpRtcpStatistics->nMaxJitter
	,pstRtpRtcpStatistics->nMinJitter
	,pstRtpRtcpStatistics->nAvgJitter
	,pstRtpRtcpStatistics->nCurJitter
	,pstRtpRtcpStatistics->nMaxRtcpTime
	);
	}
#endif

	return 0;//pstVoipRtpStatistics->ret_val;
}

int32 rtk_Get_Session_Statistics( uint32 chid, uint32 sid, uint32 bReset, TstVoipSessionStatistics *pstVoipSessionStatistics )
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( pstVoipSessionStatistics==NULL ){
		TAPI_DUMP( "chid=%u sid=%u bReset=%u Input is NULL\n" , chid , sid , bReset );
	}
	else{
		TAPI_DUMP("chid=%u sid=%u bReset=%u\n" , chid , sid , bReset);			
	}
#endif

	
	pstVoipSessionStatistics ->ch_id = chid;
	pstVoipSessionStatistics ->m_id = sid;
	pstVoipSessionStatistics ->bResetStatistics = bReset;

	SETSOCKOPT( VOIP_MGR_GET_SESSION_STATISTICS, pstVoipSessionStatistics, TstVoipSessionStatistics, 1 );

#ifdef VOIP_TAPI_DBG_DUMP
	if( pstVoipSessionStatistics != NULL ){
	TAPI_DUMP(
	"ch_id=%u "			    	
	"m_id=%u "				
	"bResetStatistics=%u "	
	"nRxSilencePacket=%u "   
	"nTxSilencePacket=%u "   
	"nAvgPlayoutDelay=%u "   
	"nCurrentJitterBuf=%u "  
	"nEarlyPacket=%u "       
	"nLatePacket=%u "        
	"nSilenceSpeech=%u "     
	"\n"
	,pstVoipSessionStatistics->ch_id					
	,pstVoipSessionStatistics->m_id						
	,pstVoipSessionStatistics->bResetStatistics			
	,pstVoipSessionStatistics->nRxSilencePacket      
	,pstVoipSessionStatistics->nTxSilencePacket      
	,pstVoipSessionStatistics->nAvgPlayoutDelay      
	,pstVoipSessionStatistics->nCurrentJitterBuf     
	,pstVoipSessionStatistics->nEarlyPacket          
	,pstVoipSessionStatistics->nLatePacket           
	,pstVoipSessionStatistics->nSilenceSpeech        
	);
	}
#endif
	

	return 0;//pstVoipSessionStatistics->ret_val;
}

int32 rtk_QosSetDscpPriority(int32 dscp, int32 priority)
{
	int _dscp;
	TAPI_DUMP("dscp=%d priority=%d\n" , dscp , priority);
	if((priority > 7) || (priority < 0)){
		TAPI_ERROR("priority out of range Available range: 0 to 7 (7 is the highest priority).   \n");
		return -1;
	}
	_dscp = (dscp & 0x00FF)<<8 | (priority & 0xFF);
	SETSOCKOPT(VOIP_MGR_SET_DSCP_PRIORITY, &_dscp, int32, 1);
	return 0;
}

int32 rtk_qos_reset_dscp_priority(void)
{
	int dscp = 0xFF;
	TAPI_DUMP("\n");
	SETSOCKOPT(VOIP_MGR_SET_DSCP_PRIORITY, &dscp, int32, 1);
	return 0;
}

int32 rtk_SetRtpTos(int32 rtp_tos)
{
	TAPI_DUMP("rtp_tos=%d\n" , rtp_tos);
	SETSOCKOPT(VOIP_MGR_SET_RTP_TOS, &rtp_tos, int32, 1);
	return 0;
}

int32 rtk_SetRtpDscp(int32 rtp_dscp)
{
	TAPI_DUMP("rtp_dscp=%d\n" , rtp_dscp);
	SETSOCKOPT(VOIP_MGR_SET_RTP_DSCP, &rtp_dscp, int32, 1);
	return 0;
}


int32 rtk_SetSipTos(int32 sip_tos)
{
	TAPI_DUMP("sip_tos=%d\n" , sip_tos);
	SETSOCKOPT(VOIP_MGR_SET_SIP_TOS, &sip_tos, int32, 1);
	return 0;
}

int32 rtk_SetSipDscp(int32 sip_dscp)
{
	TAPI_DUMP("sip_dscp=%d\n" , sip_dscp);
	SETSOCKOPT(VOIP_MGR_SET_SIP_DSCP, &sip_dscp, int32, 1);
	return 0;
}

#ifdef CONFIG_RTK_VOIP_DRIVERS_IP_PHONE
int rtk_Set_IPhone(unsigned int function_type, unsigned int reg, unsigned int value)
{
	IPhone_test iphone;

	iphone.function_type = function_type;
	iphone.reg = reg;
	iphone.value = value;
	SETSOCKOPT(VOIP_MGR_IPHONE_TEST, &iphone, IPhone_test, 1);
	return 0;
}
#endif

/******************** New Add for AudioCodes Solution ****************/
int32 rtk_OnHookAction(uint32 chid)
{
    	TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    	TAPI_DUMP("chid=%u\n" , chid);
        stVoipCfg.ch_id = chid;
        SETSOCKOPT(VOIP_MGR_SLIC_ONHOOK_ACTION, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_OffHookAction(uint32 chid)
{
    	TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    	TAPI_DUMP("chid=%u\n" , chid);

        stVoipCfg.ch_id = chid;
        SETSOCKOPT(VOIP_MGR_SLIC_OFFHOOK_ACTION, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

/**********************************************************************/

#ifdef CONFIG_RTK_VOIP_IP_PHONE
int rtk_GetIPPhoneHookStatus( uint32 *pHookStatus )
{
	TstKeypadHookStatus stKeypadHookStatus;
	memset(&stKeypadHookStatus, 0x0, sizeof(TstKeypadHookStatus));
	stKeypadHookStatus.cmd = KEYPAD_CMD_HOOK_STATUS;
	SETSOCKOPT( VOIP_MGR_CTL_KEYPAD, &stKeypadHookStatus, TstKeypadHookStatus, 1);

	if( stKeypadHookStatus.status )
		printf( "-------------------------------------------\nOff-hook\n" );
	else
		printf( "-------------------------------------------\nOn-hook\n" );

	return 0;
}
#endif /* CONFIG_RTK_VOIP_IP_PHONE */

int rtk_SetFlushFifo(uint32 chid)
{
	int ret;
	TstFlushVoipEvent stFlushVoipEvent;
	memset(&stFlushVoipEvent, 0x0, sizeof(TstFlushVoipEvent));
	TAPI_DUMP("chid=%u\n" , chid);
	
	stFlushVoipEvent.ch_id = chid;
	stFlushVoipEvent.type = VET_ALL;
	stFlushVoipEvent.mask = VEM_ALL;
	
	if( ( ret = rtk_FlushVoIPEvent( &stFlushVoipEvent ) ) < 0 )
		return ret;
	
	return 0;//stVoipValue.ret_val;
}

/*	for FXS:
	0: Phone dis-connect,
	1: Phone connect,
	2: Phone off-hook,
	3: Check time out ( may connect too many phone set => view as connect),
	4: Can not check, Linefeed should be set to active state first.

	for FXO:
	0: PSTN Line connect,
	1: PSTN Line not connect,
	2: PSTN Line busy
*/
int rtk_LineCheck(uint32 chid)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u\n" , chid);

	stVoipValue.ch_id = chid;
	SETSOCKOPT(VOIP_MGR_LINE_CHECK, &stVoipValue, TstVoipValue, 1);

	//if (stVoipValue.ret_val != 0)
	//{
	//	return stVoipValue.ret_val;
	//}

	return stVoipValue.value;
}

int rtk_FxoOffHook(uint32 chid)// for real DAA off-hook(channel is FXO channel)
{
	TAPI_DUMP("chid=%u\n" , chid);
	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	    	stVoipCfg.ch_id = chid;

		SETSOCKOPT(VOIP_MGR_FXO_OFF_HOOK, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		return 1;
	}
	else
	{
		printf("API rtk_FxoOffHook usage error.\n");
		return 0;
	}
}

int rtk_FxoOnHook(uint32 chid)// for real DAA on-hook(channel is FXO channel)
{
	TAPI_DUMP("chid=%u\n" , chid);
	if ((g_VoIP_Feature & DAA_TYPE_MASK) != NO_DAA )
	{
		TstVoipCfg stVoipCfg;
		memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	    	stVoipCfg.ch_id = chid;

		SETSOCKOPT(VOIP_MGR_FXO_ON_HOOK, &stVoipCfg, TstVoipCfg, 1);

		//if (stVoipCfg.ret_val != 0)
		//{
		//	return stVoipCfg.ret_val;
		//}

		// Re-init DSP
		SETSOCKOPT(VOIP_MGR_ON_HOOK_RE_INIT, &stVoipCfg, TstVoipCfg, 1);

		return 0;//stVoipCfg.ret_val;
	}
        else
		return 0;
}

int rtk_FxoRingOn(uint32 chid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u\n" , chid);
	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = 11; //FXO_RING_ON
	stVoipCfg.cfg2 = VEID_HOOK_FXO_RING_ON;
	
	SETSOCKOPT(VOIP_MGR_HOOK_FIFO_IN, &stVoipCfg, TstVoipCfg, 1);
	
	return 0;//stVoipCfg.ret_val;
}

int rtk_FxoBusy(uint32 chid)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u\n" , chid);
	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = 13; //FXO_BUSY_TONE
	stVoipCfg.cfg2 = VEID_HOOK_FXO_BUSY_TONE;
	
	SETSOCKOPT(VOIP_MGR_HOOK_FIFO_IN, &stVoipCfg, TstVoipCfg, 1);
	
	return 0;//stVoipCfg.ret_val;
}

int rtk_gpio(unsigned int action, unsigned int pid, unsigned int value, unsigned int *ret_value)
{
	TstVoipGPIO stVoipGPIO;
	memset(&stVoipGPIO, 0x0, sizeof(TstVoipGPIO));
#ifdef VOIP_TAPI_DBG_DUMP
	if( ret_value==NULL ){
		TAPI_DUMP( "action=%u pid=%u value=%u ret_value is NULL\n" , action , pid , value );
	}
	else{
		TAPI_DUMP("action=%u pid=%u value=%u *ret_value=%u\n" , action , pid , value , *ret_value);		
	}
#endif


	stVoipGPIO.action = action;
	stVoipGPIO.pid = pid;
	stVoipGPIO.value = value;
   	SETSOCKOPT(VOIP_MGR_GPIO, &stVoipGPIO, TstVoipGPIO, 1);

   	//if (stVoipGPIO.ret_val != 0)
   	//{
	//	return stVoipGPIO.ret_val;
	//}

	if (ret_value)
		*ret_value = stVoipGPIO.value;

	return stVoipGPIO.result;
}

int rtk_Set_LED_Display( uint32 chid, uint32 LED_ID, LedDisplayMode mode )
{
	TstVoipLedDisplay stVoipLedDisplay;
	memset(&stVoipLedDisplay, 0x0, sizeof(TstVoipLedDisplay));
	TAPI_DUMP("chid=%u LED_ID=%u mode=%d\n" , chid , LED_ID , mode);

	if( LED_ID >= 2 )	// LED_ID has to be 0 or 1
		return -1;

	stVoipLedDisplay.ch_id = chid;
	stVoipLedDisplay.led_id = LED_ID;
	stVoipLedDisplay.mode = mode;

	SETSOCKOPT( VOIP_MGR_SET_LED_DISPLAY, &stVoipLedDisplay, TstVoipLedDisplay, 1 );

	return 0;
}

int rtk_Get_LED_Status( uint32 chid, uint32 LED_ID, LedDisplayMode *mode )
{
	TstVoipLedDisplay stVoipLedDisplay;
	memset(&stVoipLedDisplay, 0x0, sizeof(TstVoipLedDisplay));
#ifdef VOIP_TAPI_DBG_DUMP
	if( mode==NULL ){
		TAPI_DUMP( "chid=%u LED_ID=%u mode is NULL\n" , chid , LED_ID );
	}
	else{
		TAPI_DUMP("chid=%u LED_ID=%u mode=%d\n" , chid , LED_ID , *mode);
	}
#endif


	if( LED_ID >= 2 )	// LED_ID has to be 0 or 1
		return -1;

	stVoipLedDisplay.ch_id = chid;
	stVoipLedDisplay.led_id = LED_ID;
	

	SETSOCKOPT( VOIP_MGR_GET_LED_STATUS, &stVoipLedDisplay, TstVoipLedDisplay, 1 );

	*mode = stVoipLedDisplay.mode;
	return 0;
}

int rtk_Set_SLIC_Relay( uint32 chid, uint32 close1 )
{
	TstVoipSlicRelay stVoipSlicRelay;
	memset(&stVoipSlicRelay, 0x0, sizeof(TstVoipSlicRelay));
	TAPI_DUMP("chid=%u close1=%u\n" , chid , close1);

	stVoipSlicRelay.ch_id = chid;
	stVoipSlicRelay.close = close1;

	SETSOCKOPT( VOIP_MGR_SET_SLIC_RELAY, &stVoipSlicRelay, TstVoipSlicRelay, 1 );

	return 0;
}

int rtk_SetPulseDigitDet(uint32 chid, uint32 enable, uint32 pause_time, uint32 min_break_ths, uint32 max_break_ths, uint32 min_make_ths, uint32 max_make_ths) /* 0: disable 1: enable Pulse Digit Detection */
{
	TAPI_DUMP("chid=%d enable=%u pause_time=%u min_break_ths=%u max_break_ths=%u min_make_ths=%u max_make_ths=%u\n"
		, chid, enable, pause_time, min_break_ths, max_break_ths, min_make_ths, max_make_ths);
	
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    	stVoipCfg.ch_id = chid;
    	stVoipCfg.enable = enable;
    	stVoipCfg.cfg = pause_time;
    	stVoipCfg.cfg2 = min_break_ths;
    	stVoipCfg.cfg3 = max_break_ths;
    	stVoipCfg.cfg4 = min_make_ths;
    	stVoipCfg.cfg5 = max_make_ths;

    	SETSOCKOPT(VOIP_MGR_SET_PULSE_DIGIT_DET, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int rtk_SetDailMode(uint32 chid, uint32 mode) /* 0: disable 1: enable Pulse dial */
{
	TAPI_DUMP("chid=%u mode=%u\n" , chid , mode);
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
    	stVoipCfg.ch_id = chid;
    	stVoipCfg.cfg = mode;

    	SETSOCKOPT(VOIP_MGR_SET_DIAL_MODE, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int rtk_GetDailMode(uint32 chid) /* 0: disable 1: enable Pulse dial */
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u\n" , chid);
    	stVoipCfg.ch_id = chid;

    	SETSOCKOPT(VOIP_MGR_GET_DIAL_MODE, &stVoipCfg, TstVoipCfg, 1);

    // 	if (stVoipCfg.ret_val != 0)
    // 	{
	//	return stVoipCfg.ret_val;
	//}

    	return 	stVoipCfg.cfg;
}

int rtk_PulseDialGenCfg(char pps, short make_duration, short interdigit_duration)
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("pps=%d make_duration=%d interdigit_duration=%d\n"
		, pps , make_duration , interdigit_duration);

	stVoipValue.value = pps;
	stVoipValue.value5 = make_duration;
	stVoipValue.value6 = interdigit_duration;

	SETSOCKOPT(VOIP_MGR_PULSE_DIAL_GEN_CFG, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

int rtk_GenPulseDial(uint32 chid, char digit) /* digit: 0 ~ 9 */
{
	TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
	TAPI_DUMP("chid=%u digit=%d\n" , chid , digit);

	stVoipValue.ch_id = chid;
	stVoipValue.value = digit;

	SETSOCKOPT(VOIP_MGR_GEN_PULSE_DIAL, &stVoipValue, TstVoipValue, 1);

	return 0;//stVoipValue.ret_val;
}

uint32 rtk_GetSlicRamVal(uint8 chid, uint16 reg)
{
	TstVoipSlicRam stSlicReg;
	memset(&stSlicReg, 0x0, sizeof(TstVoipSlicRam));
	TAPI_DUMP("chid=%u reg=%u\n" , chid , reg);

	stSlicReg.ch_id = chid;
	stSlicReg.reg_num = reg;
	SETSOCKOPT(VOIP_MGR_GET_SLIC_RAM_VAL, &stSlicReg, TstVoipSlicRam, 1);

	//if (stSlicReg.ret_val != 0)
	//{
	//	return stSlicReg.ret_val;
	//}

	return stSlicReg.reg_val;
}

int rtk_SetSlicRamVal(uint8 chid, uint16 reg, uint32 value)
{
	TstVoipSlicRam stSlicReg;
	memset(&stSlicReg, 0x0, sizeof(TstVoipSlicRam));
	TAPI_DUMP("chid=%u reg=%u value=%u\n" , chid , reg , value);

	stSlicReg.ch_id = chid;
	stSlicReg.reg_num = reg;
	stSlicReg.reg_val = value;
	SETSOCKOPT(VOIP_MGR_SET_SLIC_RAM_VAL, &stSlicReg, TstVoipSlicRam, 1);

	return 0;//stSlicReg.ret_val;
}

int32 rtk_SetFaxModemDet(uint32 chid, uint32 mode)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u mode=%u\n" , chid , mode);

	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = mode;

	SETSOCKOPT(VOIP_MGR_SET_FAX_MODEM_DET, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipCfg.ret_val;
}

int32 rtk_Set_RTP_PT_checker(uint32 chid, uint32 enable, uint32 sync, uint32 report, uint32 pkt_cnt_thres)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
	TAPI_DUMP("chid=%u enable=%u sync=%u report=%u pkt_cnt_thres=%u\n"
		,chid ,enable , sync , report , pkt_cnt_thres);

	stVoipCfg.ch_id = chid;   
	stVoipCfg.enable = enable;
	stVoipCfg.cfg = sync;
	stVoipCfg.cfg2 = report;
	stVoipCfg.cfg3 = pkt_cnt_thres;
	
	SETSOCKOPT(VOIP_MGR_SET_RTP_PT_CHECKER, &stVoipCfg, TstVoipCfg, 1);

	return 0;//stVoipValue.ret_val;
}

int32 rtk_GetPortLinkStatus( uint32 *pstatus )
{
	TstVoipPortLinkStatus stVoipPortLinkStatus;
	memset(&stVoipPortLinkStatus, 0x0, sizeof(TstVoipPortLinkStatus));
	SETSOCKOPT(VOIP_MGR_GET_PORT_LINK_STATUS, &stVoipPortLinkStatus, TstVoipPortLinkStatus, 1);

	*pstatus = stVoipPortLinkStatus.status;
	TAPI_DUMP("*pstatus=%u\n" , *pstatus);

	return 0;
}

int32 rtk_WTD_Reboot(int reboot)
{
	SETSOCKOPT(VOIP_MGR_SET_FW_UPDATE, &reboot, int, 1);

	return 0;
}

int rtk_Print(int level, char *module, char *msg)
{

#if 0	// print to stdout
	printf(msg);
	return 0;
#else
	rtk_print_cfg cfg;
#ifdef VOIP_TAPI_DBG_DUMP
	if( (module==NULL) || (msg==NULL) ){
		TAPI_DUMP( "module=%p msg=%p\n" , module , msg );
	}
#endif

	cfg.level = level;

	strncpy(cfg.module, module, sizeof(cfg.module) - 1);
	cfg.module[sizeof(cfg.module) - 1] = '\0';

	strncpy(cfg.msg, msg, sizeof(cfg.msg) - 1);
	cfg.msg[sizeof(cfg.msg) - 1] = '\0';

	SETSOCKOPT(VOIP_MGR_PRINT, &cfg, rtk_print_cfg, 1);

	return 0;//cfg.ret_val;
#endif
}

int rtk_cp3_measure(st_CP3_VoIP_param* cp3_voip_param)
{
#ifdef VOIP_TAPI_DBG_DUMP
	if( cp3_voip_param==NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
#endif

	SETSOCKOPT(VOIP_MGR_COP3_CONIFG, cp3_voip_param, st_CP3_VoIP_param, 1);

	return 0;//cp3_voip_param->ret_val;
}

int32 rtk_SetDspIdToDsp(unsigned char cpuid)
{
    TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
    TAPI_DUMP("cpuid=%d\n" , cpuid);

    stVoipValue.value = cpuid;

    SETSOCKOPT(VOIP_MGR_SET_DSP_ID_TO_DSP, &stVoipValue, TstVoipValue, 1);

    return 0;
}

int32 rtk_SetDspPhyId(unsigned char cpuid)
{
    TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
    TAPI_DUMP("cpuid=%u\n" , cpuid);

    stVoipValue.value = cpuid;

    SETSOCKOPT(VOIP_MGR_SET_DSP_PHY_ID, &stVoipValue, TstVoipValue, 1);

    return 0;
}

int32 rtk_CheckDspAllSoftwareReady(unsigned char cpuid)
{
    TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
    TAPI_DUMP("cpuid=%u\n" , cpuid);

    stVoipValue.value = cpuid;

    SETSOCKOPT(VOIP_MGR_CHECK_DSP_ALL_SW_READY, &stVoipValue, TstVoipValue, 1);

    return stVoipValue.value1;
}

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
uint8 rtk_CompleteDeferInitialzation( TstVoipSLICFWInfo fwInfo)
#else
uint8 rtk_CompleteDeferInitialzation( void )
#endif
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
	TstVoipSLICFWInfo stVoipValue;
    TAPI_DUMP("\n");

	memset(&stVoipValue, 0, sizeof(stVoipValue));
	
	stVoipValue.fwSize = fwInfo.fwSize;
	stVoipValue.BBDSize = fwInfo.BBDSize;
	strncpy(&stVoipValue.fwPath[0], &fwInfo.fwPath[0], sizeof(stVoipValue.fwPath) - 1);
	strncpy(&stVoipValue.BBDPath[0], &fwInfo.BBDPath[0], sizeof(stVoipValue.BBDPath) - 1);
	
    SETSOCKOPT(VOIP_MGR_COMPLETE_DEFER_INIT, &stVoipValue, TstVoipSLICFWInfo, 1);	
#else
    TstVoipValue stVoipValue;
	memset(&stVoipValue, 0x0, sizeof(TstVoipValue));
    TAPI_DUMP("\n");

	// Don't need any data!

    SETSOCKOPT(VOIP_MGR_COMPLETE_DEFER_INIT, &stVoipValue, TstVoipValue, 1);
#endif
	return 0;
}

int32 rtkSetProslicParam(uint32 chid, ProslicType slic_type, ProslicParamType param_type, void* pParam, uint32 param_size)
{
	TstVoipCfg stVoipCfg;
	memset(&stVoipCfg, 0x0, sizeof(TstVoipCfg));
#ifdef VOIP_TAPI_DBG_DUMP
	if( pParam==NULL ){
		TAPI_DUMP("chid=%u slic_type=%d param_type=%d param_size=%d\n" , 
			chid , slic_type , param_type , param_size);
	}
	else{
		TAPI_DUMP("chid=%u slic_type=%d param_type=%d param_size=%d\n" , 
			chid , slic_type , param_type , param_size);
	}		
#endif
		
	if (slic_type != PROSLIC_TYPE_SI3226X)
	{
		printf("%s doesn't support SLIC type: %d\n", __FUNCTION__, slic_type);
		return -1;
	}
	
	if (param_type >= PROSLIC_PARAM_TYPE_MAX)
	{
		printf("%s doesn't support param type: %d\n", __FUNCTION__, param_type);
		return -1;
	}
	
	stVoipCfg.ch_id = chid;
	stVoipCfg.cfg = slic_type;
	stVoipCfg.cfg2 = param_type;
	stVoipCfg.cfg3 = param_size;

	SETSOCKOPT(VOIP_MGR_SET_PROSLIC_PARAM_STEP1, &stVoipCfg, TstVoipCfg, 1);
	SETSOCKOPT(VOIP_MGR_SET_PROSLIC_PARAM_STEP2, pParam, char, param_size);
	
	return 0;
}

static int sem_id = -1;
int voip_tapi_sem_wait(const int sem_num)
{
	struct sembuf sem_b;

	sem_b.sem_num = sem_num;
	sem_b.sem_op = -1;
	sem_b.sem_flg = SEM_UNDO;
	return semop(sem_id, &sem_b, 1);
}

int voip_tapi_sem_signal(const int sem_num)
{
	struct sembuf sem_b;

	sem_b.sem_num = sem_num;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;
	return semop(sem_id, &sem_b, 1);
}

#ifdef __ECOS
void voip_manager_init( void )
{
	extern int linux_wrapper_chrdev_open( int major, int minor );
	
	g_VoIP_Mgr_FD = linux_wrapper_chrdev_open( 243, 1 );
	
	rtk_GetVoIPFeature();
}

void voip_manager_fini( void )
{
	extern int linux_wrapper_chrdev_close( int handle );
	
	linux_wrapper_chrdev_close( g_VoIP_Mgr_FD );
}
#else

#ifdef YOCTO_BUILD
#define VOIP_TAPIPATHNAME "/usr/lib/libvoip_manager.so"
#else
#define VOIP_TAPIPATHNAME "/lib/libvoip_manager.so"
#endif
static void __attribute__ ((constructor)) voip_manager_init(void)
{
//#ifdef __mips__
	if( ( g_VoIP_Mgr_FD = open( VOIP_MGR_IOCTL_DEV_NAME, O_RDWR ) ) < 0 )
		fprintf( stderr, "Open " VOIP_MGR_IOCTL_DEV_NAME " fail, ret = %d, errno = %d\n", g_VoIP_Mgr_FD , errno);
//#endif	
	key_t key;

//	rtk_GetVoIPFeature();
	key = ftok(VOIP_TAPIPATHNAME, VOIP_DSP_VER);
	if (key == -1)
	{
		fprintf(stderr, "ftok failed: %s\n", strerror(errno));
		return;
	}

	sem_id = semget(key, 1, 0);
	//printf("!!!!!!!!!!!!!!!! key=%d sem_id=%d getpid=%d\n", key, sem_id, getpid());

	if (sem_id == -1){

		sem_id = semget(key, 1, 0666 | IPC_CREAT);
		//printf("!!!!!!!!!!!!!!!!  sem_id=%d getpid=%d\n", sem_id, getpid());
		if (sem_id == -1)
		{
			fprintf(stderr, "semget failed: %s\n", strerror(errno));
			return;
		}
		// init mutex semaphore
		if (semctl(sem_id, VOIP_TAPI_MUTEX, SETVAL, 1) == -1)
		{
			fprintf(stderr, "semctl SETVAL failed: %s\n", strerror(errno));
			return;
		}


	}

	rtk_GetVoIPFeature();
}

static void __attribute__ ((destructor)) voip_manager_fini(void)
{
#if 1//#ifdef __mips__
	if ( g_VoIP_Mgr_FD > 0 )
	close( g_VoIP_Mgr_FD );
#endif
}
#endif

#ifdef VOIP_TAPI_DBG_DUMP
static inline void 
dump_st_ToneCfgParam( TstVoipToneCfg *pstToneCfgParam )
{
	if( pstToneCfgParam == NULL ){
		TAPI_DUMP( "Input is NULL\n" );
	}
	else{

	TAPI_DUMP(
	"toneType=%u "
	"cycle=%u "
	"cadNUM=%u "
	"PatternOff=%u "
	"ToneNUM=%u \n"
	,pstToneCfgParam->toneType
	,pstToneCfgParam->cycle
	,pstToneCfgParam->cadNUM
	,pstToneCfgParam->PatternOff
	,pstToneCfgParam->ToneNUM 
	);

	TAPI_DUMP(
	"CadOn0 =%u "   
	"CadOff0=%u "  
	"CadOn1 =%u "  
	"CadOff1=%u " 
	"CadOn2 =%u "   
	"CadOff2=%u "  
	"CadOn3 =%u "  
	"CadOff3=%u " 
	"Freq0=%u "     
	"Freq1=%u "    
	"Freq2=%u "    
	"Freq3=%u "   
	"Gain0=%d "     
	"Gain1=%d "    
	"Gain2=%d "    
	"Gain3=%d "
	"C1_Freq0=%u "  
	"C1_Freq1=%u " 
	"C1_Freq2=%u " 
	"C1_Freq3=%u "
	"C1_Gain0=%d "  
	"C1_Gain1=%d " 
	"C1_Gain2=%d " 
	"C1_Gain3=%d "
	"C2_Freq0=%u "  
	"C2_Freq1=%u " 
	"C2_Freq2=%u " 
	"C2_Freq3=%u "
	"C2_Gain0=%d "  
	"C2_Gain1=%d " 
	"C2_Gain2=%d " 
	"C2_Gain3=%d "
	"C3_Freq0=%u "  
	"C3_Freq1=%u " 
	"C3_Freq2=%u " 
	"C3_Freq3=%u "
	"C3_Gain0=%d "  
	"C3_Gain1=%d " 
	"C3_Gain2=%d " 
	"C3_Gain3=%d "
	"\n"
	,pstToneCfgParam->CadOn0      
	,pstToneCfgParam->CadOff0     
	,pstToneCfgParam->CadOn1      
	,pstToneCfgParam->CadOff1 
	,pstToneCfgParam->CadOn2  
	,pstToneCfgParam->CadOff2 
	,pstToneCfgParam->CadOn3  
	,pstToneCfgParam->CadOff3 
	,pstToneCfgParam->Freq0   
	,pstToneCfgParam->Freq1   
	,pstToneCfgParam->Freq2   
	,pstToneCfgParam->Freq3   
	,pstToneCfgParam->Gain0   
	,pstToneCfgParam->Gain1   
	,pstToneCfgParam->Gain2   
	,pstToneCfgParam->Gain3   
	,pstToneCfgParam->C1_Freq0
	,pstToneCfgParam->C1_Freq1
	,pstToneCfgParam->C1_Freq2
	,pstToneCfgParam->C1_Freq3
	,pstToneCfgParam->C1_Gain0
	,pstToneCfgParam->C1_Gain1
	,pstToneCfgParam->C1_Gain2
	,pstToneCfgParam->C1_Gain3
	,pstToneCfgParam->C2_Freq0
	,pstToneCfgParam->C2_Freq1
	,pstToneCfgParam->C2_Freq2
	,pstToneCfgParam->C2_Freq3
	,pstToneCfgParam->C2_Gain0
	,pstToneCfgParam->C2_Gain1
	,pstToneCfgParam->C2_Gain2
	,pstToneCfgParam->C2_Gain3
	,pstToneCfgParam->C3_Freq0
	,pstToneCfgParam->C3_Freq1
	,pstToneCfgParam->C3_Freq2
	,pstToneCfgParam->C3_Freq3
	,pstToneCfgParam->C3_Gain0
	,pstToneCfgParam->C3_Gain1
	,pstToneCfgParam->C3_Gain2
	,pstToneCfgParam->C3_Gain3
	);	

	TAPI_DUMP(
	" CadOn4=%u "   
	"CadOff4=%u "  
	" CadOn5=%u "  
	"CadOff5=%u " 
	" CadOn6=%u "   
	"CadOff6=%u "  
	" CadOn7=%u "  
	"CadOff7=%u " 
	"C4_Freq0=%u "     
	"C4_Freq1=%u "    
	"C4_Freq2=%u "    
	"C4_Freq3=%u "   
	"C4_Gain0=%d "     
	"C4_Gain1=%d "    
	"C4_Gain2=%d "    
	"C4_Gain3=%d "
	"C5_Freq0=%u "  
	"C5_Freq1=%u " 
	"C5_Freq2=%u " 
	"C5_Freq3=%u "
	"C5_Gain0=%d "  
	"C5_Gain1=%d " 
	"C5_Gain2=%d " 
	"C5_Gain3=%d "
	"C6_Freq0=%u "  
	"C6_Freq1=%u " 
	"C6_Freq2=%u " 
	"C6_Freq3=%u "
	"C6_Gain0=%d "  
	"C6_Gain1=%d " 
	"C6_Gain2=%d " 
	"C6_Gain3=%d "
	"C7_Freq0=%u "  
	"C7_Freq1=%u " 
	"C7_Freq2=%u " 
	"C7_Freq3=%u "
	"C7_Gain0=%d "  
	"C7_Gain1=%d " 
	"C7_Gain2=%d " 
	"C7_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn4      
	,pstToneCfgParam->CadOff4     
	,pstToneCfgParam-> CadOn5      
	,pstToneCfgParam->CadOff5 
	,pstToneCfgParam-> CadOn6  
	,pstToneCfgParam->CadOff6 
	,pstToneCfgParam-> CadOn7  
	,pstToneCfgParam->CadOff7 
	,pstToneCfgParam->C4_Freq0   
	,pstToneCfgParam->C4_Freq1   
	,pstToneCfgParam->C4_Freq2   
	,pstToneCfgParam->C4_Freq3   
	,pstToneCfgParam->C4_Gain0   
	,pstToneCfgParam->C4_Gain1   
	,pstToneCfgParam->C4_Gain2   
	,pstToneCfgParam->C4_Gain3   
	,pstToneCfgParam->C5_Freq0
	,pstToneCfgParam->C5_Freq1
	,pstToneCfgParam->C5_Freq2
	,pstToneCfgParam->C5_Freq3
	,pstToneCfgParam->C5_Gain0
	,pstToneCfgParam->C5_Gain1
	,pstToneCfgParam->C5_Gain2
	,pstToneCfgParam->C5_Gain3
	,pstToneCfgParam->C6_Freq0
	,pstToneCfgParam->C6_Freq1
	,pstToneCfgParam->C6_Freq2
	,pstToneCfgParam->C6_Freq3
	,pstToneCfgParam->C6_Gain0
	,pstToneCfgParam->C6_Gain1
	,pstToneCfgParam->C6_Gain2
	,pstToneCfgParam->C6_Gain3
	,pstToneCfgParam->C7_Freq0
	,pstToneCfgParam->C7_Freq1
	,pstToneCfgParam->C7_Freq2
	,pstToneCfgParam->C7_Freq3
	,pstToneCfgParam->C7_Gain0
	,pstToneCfgParam->C7_Gain1
	,pstToneCfgParam->C7_Gain2
	,pstToneCfgParam->C7_Gain3
	);

	TAPI_DUMP(
	" CadOn8=%u "   
	"CadOff8=%u "  
	" CadOn9=%u "  
	"CadOff9=%u " 
	" CadOn10=%u "   
	"CadOff10=%u "  
	" CadOn11=%u "  
	"CadOff11=%u " 
	"C8_Freq0=%u "     
	"C8_Freq1=%u "    
	"C8_Freq2=%u "    
	"C8_Freq3=%u "   
	"C8_Gain0=%d "     
	"C8_Gain1=%d "    
	"C8_Gain2=%d "    
	"C8_Gain3=%d "
	"C9_Freq0=%u "  
	"C9_Freq1=%u " 
	"C9_Freq2=%u " 
	"C9_Freq3=%u "
	"C9_Gain0=%d "  
	"C9_Gain1=%d " 
	"C9_Gain2=%d " 
	"C9_Gain3=%d "
	"C10_Freq0=%u "  
	"C10_Freq1=%u " 
	"C10_Freq2=%u " 
	"C10_Freq3=%u "
	"C10_Gain0=%d "  
	"C10_Gain1=%d " 
	"C10_Gain2=%d " 
	"C10_Gain3=%d "
	"C11_Freq0=%u "  
	"C11_Freq1=%u " 
	"C11_Freq2=%u " 
	"C11_Freq3=%u "
	"C11_Gain0=%d "  
	"C11_Gain1=%d " 
	"C11_Gain2=%d " 
	"C11_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn8      
	,pstToneCfgParam->CadOff8     
	,pstToneCfgParam-> CadOn9      
	,pstToneCfgParam->CadOff9 
	,pstToneCfgParam-> CadOn10  
	,pstToneCfgParam->CadOff10 
	,pstToneCfgParam-> CadOn11  
	,pstToneCfgParam->CadOff11 
	,pstToneCfgParam->C8_Freq0   
	,pstToneCfgParam->C8_Freq1   
	,pstToneCfgParam->C8_Freq2   
	,pstToneCfgParam->C8_Freq3   
	,pstToneCfgParam->C8_Gain0   
	,pstToneCfgParam->C8_Gain1   
	,pstToneCfgParam->C8_Gain2   
	,pstToneCfgParam->C8_Gain3   
	,pstToneCfgParam->C9_Freq0
	,pstToneCfgParam->C9_Freq1
	,pstToneCfgParam->C9_Freq2
	,pstToneCfgParam->C9_Freq3
	,pstToneCfgParam->C9_Gain0
	,pstToneCfgParam->C9_Gain1
	,pstToneCfgParam->C9_Gain2
	,pstToneCfgParam->C9_Gain3
	,pstToneCfgParam->C10_Freq0
	,pstToneCfgParam->C10_Freq1
	,pstToneCfgParam->C10_Freq2
	,pstToneCfgParam->C10_Freq3
	,pstToneCfgParam->C10_Gain0
	,pstToneCfgParam->C10_Gain1
	,pstToneCfgParam->C10_Gain2
	,pstToneCfgParam->C10_Gain3
	,pstToneCfgParam->C11_Freq0
	,pstToneCfgParam->C11_Freq1
	,pstToneCfgParam->C11_Freq2
	,pstToneCfgParam->C11_Freq3
	,pstToneCfgParam->C11_Gain0
	,pstToneCfgParam->C11_Gain1
	,pstToneCfgParam->C11_Gain2
	,pstToneCfgParam->C11_Gain3
	);

	TAPI_DUMP(
	" CadOn12=%u "   
	"CadOff12=%u "  
	" CadOn13=%u "  
	"CadOff13=%u " 
	" CadOn14=%u "   
	"CadOff14=%u "  
	" CadOn15=%u "  
	"CadOff15=%u " 
	"C12_Freq0=%u "     
	"C12_Freq1=%u "    
	"C12_Freq2=%u "    
	"C12_Freq3=%u "   
	"C12_Gain0=%d "     
	"C12_Gain1=%d "    
	"C12_Gain2=%d "    
	"C12_Gain3=%d "
	"C13_Freq0=%u "  
	"C13_Freq1=%u " 
	"C13_Freq2=%u " 
	"C13_Freq3=%u "
	"C13_Gain0=%d "  
	"C13_Gain1=%d " 
	"C13_Gain2=%d " 
	"C13_Gain3=%d "
	"C14_Freq0=%u "  
	"C14_Freq1=%u " 
	"C14_Freq2=%u " 
	"C14_Freq3=%u "
	"C14_Gain0=%d "  
	"C14_Gain1=%d " 
	"C14_Gain2=%d " 
	"C14_Gain3=%d "
	"C15_Freq0=%u "  
	"C15_Freq1=%u " 
	"C15_Freq2=%u " 
	"C15_Freq3=%u "
	"C15_Gain0=%d "  
	"C15_Gain1=%d " 
	"C15_Gain2=%d " 
	"C15_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn12      
	,pstToneCfgParam->CadOff12     
	,pstToneCfgParam-> CadOn13      
	,pstToneCfgParam->CadOff13 
	,pstToneCfgParam-> CadOn14  
	,pstToneCfgParam->CadOff14 
	,pstToneCfgParam-> CadOn15  
	,pstToneCfgParam->CadOff15 
	,pstToneCfgParam->C12_Freq0   
	,pstToneCfgParam->C12_Freq1   
	,pstToneCfgParam->C12_Freq2   
	,pstToneCfgParam->C12_Freq3   
	,pstToneCfgParam->C12_Gain0   
	,pstToneCfgParam->C12_Gain1   
	,pstToneCfgParam->C12_Gain2   
	,pstToneCfgParam->C12_Gain3   
	,pstToneCfgParam->C13_Freq0
	,pstToneCfgParam->C13_Freq1
	,pstToneCfgParam->C13_Freq2
	,pstToneCfgParam->C13_Freq3
	,pstToneCfgParam->C13_Gain0
	,pstToneCfgParam->C13_Gain1
	,pstToneCfgParam->C13_Gain2
	,pstToneCfgParam->C13_Gain3
	,pstToneCfgParam->C14_Freq0
	,pstToneCfgParam->C14_Freq1
	,pstToneCfgParam->C14_Freq2
	,pstToneCfgParam->C14_Freq3
	,pstToneCfgParam->C14_Gain0
	,pstToneCfgParam->C14_Gain1
	,pstToneCfgParam->C14_Gain2
	,pstToneCfgParam->C14_Gain3
	,pstToneCfgParam->C15_Freq0
	,pstToneCfgParam->C15_Freq1
	,pstToneCfgParam->C15_Freq2
	,pstToneCfgParam->C15_Freq3
	,pstToneCfgParam->C15_Gain0
	,pstToneCfgParam->C15_Gain1
	,pstToneCfgParam->C15_Gain2
	,pstToneCfgParam->C15_Gain3
	);

	TAPI_DUMP(
	" CadOn16=%u "
	"CadOff16=%u "
	" CadOn17=%u "
	"CadOff17=%u "
	" CadOn18=%u "
	"CadOff18=%u "
	" CadOn19=%u "
	"CadOff19=%u "
	"C16_Freq0=%u "
	"C16_Freq1=%u "
	"C16_Freq2=%u "
	"C16_Freq3=%u "
	"C16_Gain0=%d "
	"C16_Gain1=%d "
	"C16_Gain2=%d "
	"C16_Gain3=%d "
	"C17_Freq0=%u "
	"C17_Freq1=%u "
	"C17_Freq2=%u "
	"C17_Freq3=%u "
	"C17_Gain0=%d "
	"C17_Gain1=%d "
	"C17_Gain2=%d "
	"C17_Gain3=%d "
	"C18_Freq0=%u "
	"C18_Freq1=%u "
	"C18_Freq2=%u "
	"C18_Freq3=%u "
	"C18_Gain0=%d "
	"C18_Gain1=%d "
	"C18_Gain2=%d "
	"C18_Gain3=%d "
	"C19_Freq0=%u "
	"C19_Freq1=%u "
	"C19_Freq2=%u "
	"C19_Freq3=%u "
	"C19_Gain0=%d "
	"C19_Gain1=%d "
	"C19_Gain2=%d "
	"C19_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn16
	,pstToneCfgParam->CadOff16
	,pstToneCfgParam-> CadOn17
	,pstToneCfgParam->CadOff17
	,pstToneCfgParam-> CadOn18
	,pstToneCfgParam->CadOff18
	,pstToneCfgParam-> CadOn19
	,pstToneCfgParam->CadOff19
	,pstToneCfgParam->C16_Freq0
	,pstToneCfgParam->C16_Freq1
	,pstToneCfgParam->C16_Freq2
	,pstToneCfgParam->C16_Freq3
	,pstToneCfgParam->C16_Gain0
	,pstToneCfgParam->C16_Gain1
	,pstToneCfgParam->C16_Gain2
	,pstToneCfgParam->C16_Gain3
	,pstToneCfgParam->C17_Freq0
	,pstToneCfgParam->C17_Freq1
	,pstToneCfgParam->C17_Freq2
	,pstToneCfgParam->C17_Freq3
	,pstToneCfgParam->C17_Gain0
	,pstToneCfgParam->C17_Gain1
	,pstToneCfgParam->C17_Gain2
	,pstToneCfgParam->C17_Gain3
	,pstToneCfgParam->C18_Freq0
	,pstToneCfgParam->C18_Freq1
	,pstToneCfgParam->C18_Freq2
	,pstToneCfgParam->C18_Freq3
	,pstToneCfgParam->C18_Gain0
	,pstToneCfgParam->C18_Gain1
	,pstToneCfgParam->C18_Gain2
	,pstToneCfgParam->C18_Gain3
	,pstToneCfgParam->C19_Freq0
	,pstToneCfgParam->C19_Freq1
	,pstToneCfgParam->C19_Freq2
	,pstToneCfgParam->C19_Freq3
	,pstToneCfgParam->C19_Gain0
	,pstToneCfgParam->C19_Gain1
	,pstToneCfgParam->C19_Gain2
	,pstToneCfgParam->C19_Gain3
	);

	TAPI_DUMP(
	" CadOn20=%u "
	"CadOff20=%u "
	" CadOn21=%u "
	"CadOff21=%u "
	" CadOn22=%u "
	"CadOff22=%u "
	" CadOn23=%u "
	"CadOff23=%u "
	"C20_Freq0=%u "
	"C20_Freq1=%u "
	"C20_Freq2=%u "
	"C20_Freq3=%u "
	"C20_Gain0=%d "
	"C20_Gain1=%d "
	"C20_Gain2=%d "
	"C20_Gain3=%d "
	"C21_Freq0=%u "
	"C21_Freq1=%u "
	"C21_Freq2=%u "
	"C21_Freq3=%u "
	"C21_Gain0=%d "
	"C21_Gain1=%d "
	"C21_Gain2=%d "
	"C21_Gain3=%d "
	"C22_Freq0=%u "
	"C22_Freq1=%u "
	"C22_Freq2=%u "
	"C22_Freq3=%u "
	"C22_Gain0=%d "
	"C22_Gain1=%d "
	"C22_Gain2=%d "
	"C22_Gain3=%d "
	"C23_Freq0=%u "
	"C23_Freq1=%u "
	"C23_Freq2=%u "
	"C23_Freq3=%u "
	"C23_Gain0=%d "
	"C23_Gain1=%d "
	"C23_Gain2=%d "
	"C23_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn20
	,pstToneCfgParam->CadOff20
	,pstToneCfgParam-> CadOn21
	,pstToneCfgParam->CadOff21
	,pstToneCfgParam-> CadOn22
	,pstToneCfgParam->CadOff22
	,pstToneCfgParam-> CadOn23
	,pstToneCfgParam->CadOff23
	,pstToneCfgParam->C20_Freq0
	,pstToneCfgParam->C20_Freq1
	,pstToneCfgParam->C20_Freq2
	,pstToneCfgParam->C20_Freq3
	,pstToneCfgParam->C20_Gain0
	,pstToneCfgParam->C20_Gain1
	,pstToneCfgParam->C20_Gain2
	,pstToneCfgParam->C20_Gain3
	,pstToneCfgParam->C21_Freq0
	,pstToneCfgParam->C21_Freq1
	,pstToneCfgParam->C21_Freq2
	,pstToneCfgParam->C21_Freq3
	,pstToneCfgParam->C21_Gain0
	,pstToneCfgParam->C21_Gain1
	,pstToneCfgParam->C21_Gain2
	,pstToneCfgParam->C21_Gain3
	,pstToneCfgParam->C22_Freq0
	,pstToneCfgParam->C22_Freq1
	,pstToneCfgParam->C22_Freq2
	,pstToneCfgParam->C22_Freq3
	,pstToneCfgParam->C22_Gain0
	,pstToneCfgParam->C22_Gain1
	,pstToneCfgParam->C22_Gain2
	,pstToneCfgParam->C22_Gain3
	,pstToneCfgParam->C23_Freq0
	,pstToneCfgParam->C23_Freq1
	,pstToneCfgParam->C23_Freq2
	,pstToneCfgParam->C23_Freq3
	,pstToneCfgParam->C23_Gain0
	,pstToneCfgParam->C23_Gain1
	,pstToneCfgParam->C23_Gain2
	,pstToneCfgParam->C23_Gain3
	);

	TAPI_DUMP(
	" CadOn24=%u "
	"CadOff24=%u "
	" CadOn25=%u "
	"CadOff25=%u "
	" CadOn26=%u "
	"CadOff26=%u "
	" CadOn27=%u "
	"CadOff27=%u "
	"C24_Freq0=%u "
	"C24_Freq1=%u "
	"C24_Freq2=%u "
	"C24_Freq3=%u "
	"C24_Gain0=%d "
	"C24_Gain1=%d "
	"C24_Gain2=%d "
	"C24_Gain3=%d "
	"C25_Freq0=%u "
	"C25_Freq1=%u "
	"C25_Freq2=%u "
	"C25_Freq3=%u "
	"C25_Gain0=%d "
	"C25_Gain1=%d "
	"C25_Gain2=%d "
	"C25_Gain3=%d "
	"C26_Freq0=%u "
	"C26_Freq1=%u "
	"C26_Freq2=%u "
	"C26_Freq3=%u "
	"C26_Gain0=%d "
	"C26_Gain1=%d "
	"C26_Gain2=%d "
	"C26_Gain3=%d "
	"C27_Freq0=%u "
	"C27_Freq1=%u "
	"C27_Freq2=%u "
	"C27_Freq3=%u "
	"C27_Gain0=%d "
	"C27_Gain1=%d "
	"C27_Gain2=%d "
	"C27_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn24
	,pstToneCfgParam->CadOff24
	,pstToneCfgParam-> CadOn25
	,pstToneCfgParam->CadOff25
	,pstToneCfgParam-> CadOn26
	,pstToneCfgParam->CadOff26
	,pstToneCfgParam-> CadOn27
	,pstToneCfgParam->CadOff27
	,pstToneCfgParam->C24_Freq0
	,pstToneCfgParam->C24_Freq1
	,pstToneCfgParam->C24_Freq2
	,pstToneCfgParam->C24_Freq3
	,pstToneCfgParam->C24_Gain0
	,pstToneCfgParam->C24_Gain1
	,pstToneCfgParam->C24_Gain2
	,pstToneCfgParam->C24_Gain3
	,pstToneCfgParam->C25_Freq0
	,pstToneCfgParam->C25_Freq1
	,pstToneCfgParam->C25_Freq2
	,pstToneCfgParam->C25_Freq3
	,pstToneCfgParam->C25_Gain0
	,pstToneCfgParam->C25_Gain1
	,pstToneCfgParam->C25_Gain2
	,pstToneCfgParam->C25_Gain3
	,pstToneCfgParam->C26_Freq0
	,pstToneCfgParam->C26_Freq1
	,pstToneCfgParam->C26_Freq2
	,pstToneCfgParam->C26_Freq3
	,pstToneCfgParam->C26_Gain0
	,pstToneCfgParam->C26_Gain1
	,pstToneCfgParam->C26_Gain2
	,pstToneCfgParam->C26_Gain3
	,pstToneCfgParam->C27_Freq0
	,pstToneCfgParam->C27_Freq1
	,pstToneCfgParam->C27_Freq2
	,pstToneCfgParam->C27_Freq3
	,pstToneCfgParam->C27_Gain0
	,pstToneCfgParam->C27_Gain1
	,pstToneCfgParam->C27_Gain2
	,pstToneCfgParam->C27_Gain3
	);

	TAPI_DUMP(
	" CadOn28=%u "
	"CadOff28=%u "
	" CadOn29=%u "
	"CadOff29=%u "
	" CadOn30=%u "
	"CadOff30=%u "
	" CadOn31=%u "
	"CadOff31=%u "
	"C28_Freq0=%u "
	"C28_Freq1=%u "
	"C28_Freq2=%u "
	"C28_Freq3=%u "
	"C28_Gain0=%d "
	"C28_Gain1=%d "
	"C28_Gain2=%d "
	"C28_Gain3=%d "
	"C29_Freq0=%u "
	"C29_Freq1=%u "
	"C29_Freq2=%u "
	"C29_Freq3=%u "
	"C29_Gain0=%d "
	"C29_Gain1=%d "
	"C29_Gain2=%d "
	"C29_Gain3=%d "
	"C30_Freq0=%u "
	"C30_Freq1=%u "
	"C30_Freq2=%u "
	"C30_Freq3=%u "
	"C30_Gain0=%d "
	"C30_Gain1=%d "
	"C30_Gain2=%d "
	"C30_Gain3=%d "
	"C31_Freq0=%u "
	"C31_Freq1=%u "
	"C31_Freq2=%u "
	"C31_Freq3=%u "
	"C31_Gain0=%d "
	"C31_Gain1=%d "
	"C31_Gain2=%d "
	"C31_Gain3=%d "
	"\n"
	,pstToneCfgParam-> CadOn28
	,pstToneCfgParam->CadOff28
	,pstToneCfgParam-> CadOn29
	,pstToneCfgParam->CadOff29
	,pstToneCfgParam-> CadOn30
	,pstToneCfgParam->CadOff30
	,pstToneCfgParam-> CadOn31
	,pstToneCfgParam->CadOff31
	,pstToneCfgParam->C28_Freq0
	,pstToneCfgParam->C28_Freq1
	,pstToneCfgParam->C28_Freq2
	,pstToneCfgParam->C28_Freq3
	,pstToneCfgParam->C28_Gain0
	,pstToneCfgParam->C28_Gain1
	,pstToneCfgParam->C28_Gain2
	,pstToneCfgParam->C28_Gain3
	,pstToneCfgParam->C29_Freq0
	,pstToneCfgParam->C29_Freq1
	,pstToneCfgParam->C29_Freq2
	,pstToneCfgParam->C29_Freq3
	,pstToneCfgParam->C29_Gain0
	,pstToneCfgParam->C29_Gain1
	,pstToneCfgParam->C29_Gain2
	,pstToneCfgParam->C29_Gain3
	,pstToneCfgParam->C30_Freq0
	,pstToneCfgParam->C30_Freq1
	,pstToneCfgParam->C30_Freq2
	,pstToneCfgParam->C30_Freq3
	,pstToneCfgParam->C30_Gain0
	,pstToneCfgParam->C30_Gain1
	,pstToneCfgParam->C30_Gain2
	,pstToneCfgParam->C30_Gain3
	,pstToneCfgParam->C31_Freq0
	,pstToneCfgParam->C31_Freq1
	,pstToneCfgParam->C31_Freq2
	,pstToneCfgParam->C31_Freq3
	,pstToneCfgParam->C31_Gain0
	,pstToneCfgParam->C31_Gain1
	,pstToneCfgParam->C31_Gain2
	,pstToneCfgParam->C31_Gain3
	);
	
	}
}
#endif



char * get_voip_event_name(VoipEventID evt)
{

	switch (evt)
	{
		case VEID_MASK_ID: 
			return("VEID_MASK_ID"); 
		case VEID_MASK_TYPE: 
			return("VEID_MASK_TYPE"); 
		case VEID_NONE: 
			return("VEID_NONE");
		case VEID_DTMF_WILDCARD: 
			return("VEID_DTMF_WILDCARD"); 
		case VEID_DTMF_DIGIT_MASK: 
			return("VEID_DTMF_DIGIT_MASK");
		case VEID_DTMF_0: 
			return("VEID_DTMF_0");
		case VEID_DTMF_1: 
			return("VEID_DTMF_1");
		case VEID_DTMF_2: 
			return("VEID_DTMF_2");
		case VEID_DTMF_3: 
			return("VEID_DTMF_3");
		case VEID_DTMF_4: 
			return("VEID_DTMF_4");
		case VEID_DTMF_5: 
			return("VEID_DTMF_5");
		case VEID_DTMF_6: 
			return("VEID_DTMF_6");
		case VEID_DTMF_7: 
			return("VEID_DTMF_7");
		case VEID_DTMF_8: 
			return("VEID_DTMF_8");
		case VEID_DTMF_9: 
			return("VEID_DTMF_9");
		case VEID_DTMF_STAR: 
			return("VEID_DTMF_STAR");
		case VEID_DTMF_SHARP: 
			return("VEID_DTMF_SHARP");
		case VEID_DTMF_A: 
			return("VEID_DTMF_A");
		case VEID_DTMF_B: 
			return("VEID_DTMF_B");
		case VEID_DTMF_C: 
			return("VEID_DTMF_C");
		case VEID_DTMF_D: 
			return("VEID_DTMF_D");
		case VEID_DTMF_ENERGY: 
			return("VEID_DTMF_ENERGY");
		case VEID_HOOK_PHONE_ON_HOOK: 
			return("VEID_HOOK_PHONE_ON_HOOK");
		case VEID_HOOK_PHONE_OFF_HOOK: 
			return("VEID_HOOK_PHONE_OFF_HOOK");
		case VEID_HOOK_PHONE_FLASH_HOOK: 
			return("VEID_HOOK_PHONE_FLASH_HOOK");
		case VEID_HOOK_PHONE_STILL_ON_HOOK: 
			return("VEID_HOOK_PHONE_STILL_ON_HOOK");
		case VEID_HOOK_PHONE_STILL_OFF_HOOK: 
			return("VEID_HOOK_PHONE_STILL_OFF_HOOK");
		case VEID_HOOK_FXO_ON_HOOK: 
			return("VEID_HOOK_FXO_ON_HOOK");
		case VEID_HOOK_FXO_OFF_HOOK: 
			return("VEID_HOOK_FXO_OFF_HOOK");
		case VEID_HOOK_FXO_FLASH_HOOK: 
			return("VEID_HOOK_FXO_FLASH_HOOK");
		case VEID_HOOK_FXO_STILL_ON_HOOK: 
			return("VEID_HOOK_FXO_STILL_ON_HOOK");
		case VEID_HOOK_FXO_STILL_OFF_HOOK: 
			return("VEID_HOOK_FXO_STILL_OFF_HOOK");
		case VEID_HOOK_FXO_RING_ON: 
			return("VEID_HOOK_FXO_RING_ON");
		case VEID_HOOK_FXO_RING_OFF: 
			return("VEID_HOOK_FXO_RING_OFF");
		case VEID_HOOK_FXO_BUSY_TONE: 
			return("VEID_HOOK_FXO_BUSY_TONE");
		case VEID_HOOK_FXO_CALLER_ID: 
			return("VEID_HOOK_FXO_CALLER_ID");
		case VEID_HOOK_FXO_RING_TONE_ON: 
			return("VEID_HOOK_FXO_RING_TONE_ON");
		case VEID_HOOK_FXO_RING_TONE_OFF: 
			return("VEID_HOOK_FXO_RING_TONE_OFF");
		case VEID_HOOK_FXO_POLARITY_REVERSAL: 
			return("VEID_HOOK_FXO_POLARITY_REVERSAL");
		case VEID_HOOK_FXO_CURRENT_DROP: 
			return("VEID_HOOK_FXO_CURRENT_DROP");
		case VEID_HOOK_FXO_DIS_TONE: 
			return("VEID_HOOK_FXO_DIS_TONE");
		case VEID_HOOK_OUTBAND_FLASH_EVENT: 
			return("VEID_HOOK_OUTBAND_FLASH_EVENT");
		case VEID_ENERGY: 
			return("VEID_ENERGY");
		case VEID_DECT_BUTTON_PAGE: 
			return("VEID_DECT_BUTTON_PAGE");
		case VEID_DECT_BUTTON_REGISTRATION_MODE: 
			return("VEID_DECT_BUTTON_REGISTRATION_MODE");
		case VEID_DECT_BUTTON_DEL_HS: 
			return("VEID_DECT_BUTTON_DEL_HS");
		case VEID_DECT_BUTTON_NOT_DEFINED: 
			return("VEID_DECT_BUTTON_NOT_DEFINED");
		case VEID_FAXMDM_AUDIOCODES_FAX: 
			return("VEID_FAXMDM_AUDIOCODES_FAX");
		case VEID_FAXMDM_AUDIOCODES_MODEM: 
			return("VEID_FAXMDM_AUDIOCODES_MODEM");
		case VEID_FAXMDM_LEC_AUTO_RESTORE: 
			return("VEID_FAXMDM_LEC_AUTO_RESTORE");
		case VEID_FAXMDM_LEC_AUTO_ON: 
			return("VEID_FAXMDM_LEC_AUTO_ON");
		case VEID_FAXMDM_LEC_AUTO_OFF: 
			return("VEID_FAXMDM_LEC_AUTO_OFF");
		case VEID_FAXMDM_FAX_CED: 
			return("VEID_FAXMDM_FAX_CED");
		case VEID_FAXMDM_FAX_DIS_TX: 
			return("VEID_FAXMDM_FAX_DIS_TX");
		case VEID_FAXMDM_FAX_DIS_RX: 
			return("VEID_FAXMDM_FAX_DIS_RX");
		case VEID_FAXMDM_FAX_DCN_TX: 
			return("VEID_FAXMDM_FAX_DCN_TX");
		case VEID_FAXMDM_FAX_DCN_RX: 
			return("VEID_FAXMDM_FAX_DCN_RX");
		case VEID_FAXMDM_MODEM_LOCAL: 
			return("VEID_FAXMDM_MODEM_LOCAL");
		case VEID_FAXMDM_MODEM_LOCAL_DELAY: 
			return("VEID_FAXMDM_MODEM_LOCAL_DELAY");
		case VEID_FAXMDM_MODEM_REMOTE: 
			return("VEID_FAXMDM_MODEM_REMOTE");
		case VEID_FAXMDM_ANSTONE_CNG_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_CNG_LOCAL");
		case VEID_FAXMDM_ANSTONE_ANS_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_ANS_LOCAL");
		case VEID_FAXMDM_ANSTONE_ANSAM_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_ANSAM_LOCAL");
		case VEID_FAXMDM_ANSTONE_ANSBAR_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_ANSBAR_LOCAL");
		case VEID_FAXMDM_ANSTONE_ANSAMBAR_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_ANSAMBAR_LOCAL");
		case VEID_FAXMDM_ANSTONE_BELLANS_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_BELLANS_LOCAL");
		case VEID_FAXMDM_ANSTONE_V22_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_V22_LOCAL");
		case VEID_FAXMDM_ANSTONE_V8BIS_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_V8BIS_LOCAL");
		case VEID_FAXMDM_V21FLAG_LOCAL: 
			return("VEID_FAXMDM_V21FLAG_LOCAL");
		case VEID_FAXMDM_HS_FAX_SEND_V21FLAG_LOCAL: 
			return("VEID_FAXMDM_HS_FAX_SEND_V21FLAG_LOCAL");
		case VEID_FAXMDM_HS_FAX_RECV_V21FLAG_LOCAL: 
			return("VEID_FAXMDM_HS_FAX_RECV_V21FLAG_LOCAL");
		case VEID_FAXMDM_ANSTONE_OFF_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_OFF_LOCAL");
		case VEID_FAXMDM_ANSTONE_V21C2_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_V21C2_LOCAL");
		case VEID_FAXMDM_ANSTONE_V21C1_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_V21C1_LOCAL");
		case VEID_FAXMDM_ANSTONE_V23_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_V23_LOCAL");
		case VEID_FAXMDM_ANSTONE_BELL202ANS_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_BELL202ANS_LOCAL");
		case VEID_FAXMDM_ANSTONE_BELL202CP_LOCAL: 
			return("VEID_FAXMDM_ANSTONE_BELL202CP_LOCAL");
		case VEID_FAXMDM_ANSTONE_CNG_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_CNG_REMOTE");
		case VEID_FAXMDM_ANSTONE_ANS_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_ANS_REMOTE");
		case VEID_FAXMDM_ANSTONE_ANSAM_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_ANSAM_REMOTE");
		case VEID_FAXMDM_ANSTONE_ANSBAR_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_ANSBAR_REMOTE");
		case VEID_FAXMDM_ANSTONE_ANSAMBAR_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_ANSAMBAR_REMOTE");
		case VEID_FAXMDM_ANSTONE_BELLANS_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_BELLANS_REMOTE");
		case VEID_FAXMDM_ANSTONE_V22_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_V22_REMOTE");
		case VEID_FAXMDM_ANSTONE_V8BIS_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_V8BIS_REMOTE");
		case VEID_FAXMDM_V21FLAG_REMOTE: 
			return("VEID_FAXMDM_V21FLAG_REMOTE");
		case VEID_FAXMDM_HS_FAX_SEND_V21FLAG_REMOTE: 
			return("VEID_FAXMDM_HS_FAX_SEND_V21FLAG_REMOTE");
		case VEID_FAXMDM_HS_FAX_RECV_V21FLAG_REMOTE: 
			return("VEID_FAXMDM_HS_FAX_RECV_V21FLAG_REMOTE");
		case VEID_FAXMDM_ANSTONE_OFF_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_OFF_REMOTE");
		case VEID_FAXMDM_ANSTONE_V21C2_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_V21C2_REMOTE");
		case VEID_FAXMDM_ANSTONE_V21C1_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_V21C1_REMOTE");
		case VEID_FAXMDM_ANSTONE_V23_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_V23_REMOTE");
		case VEID_FAXMDM_ANSTONE_BELL202ANS_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_BELL202ANS_REMOTE");
		case VEID_FAXMDM_ANSTONE_BELL202CP_REMOTE: 
			return("VEID_FAXMDM_ANSTONE_BELL202CP_REMOTE");
		case VEID_FAXMDM_FAST_FAXTONE_LOCAL: 
			return("VEID_FAXMDM_FAST_FAXTONE_LOCAL");
		case VEID_FAXMDM_FAST_MODEMTONE_LOCAL: 
			return("VEID_FAXMDM_FAST_MODEMTONE_LOCAL");
		case VEID_FAXMDM_FAST_FAXTONE_REMOTE: 
			return("VEID_FAXMDM_FAST_FAXTONE_REMOTE");
		case VEID_FAXMDM_FAST_MODEMTONE_REMOTE: 
			return("VEID_FAXMDM_FAST_MODEMTONE_REMOTE");
		case VEID_FAXMDM_V152_RTP_VBD: 
			return("VEID_FAXMDM_V152_RTP_VBD");
		case VEID_FAXMDM_V152_RTP_AUDIO: 
			return("VEID_FAXMDM_V152_RTP_AUDIO");
		case VEID_FAXMDM_V152_SIG_CED: 
			return("VEID_FAXMDM_V152_SIG_CED");
		case VEID_FAXMDM_V152_TDM_SIG_END: 
			return("VEID_FAXMDM_V152_TDM_SIG_END");
		case VEID_FAXMDM_V152_BI_SILENCE: 
			return("VEID_FAXMDM_V152_BI_SILENCE");
		case VEID_FAXMDM_V150_SIG_MODEM: 
			return("VEID_FAXMDM_V150_SIG_MODEM");
		case VEID_FAXMDM_SILENCE_TDM: 
			return("VEID_FAXMDM_SILENCE_TDM");
		case VEID_FAXMDM_SILENCE_IP: 
			return("VEID_FAXMDM_SILENCE_IP");
		case VEID_FAXMDM_SILENCE_TDM_IP: 
			return("VEID_FAXMDM_SILENCE_TDM_IP");
		case VEID_RFC2833_RX_WILDCARD: 
			return("VEID_RFC2833_RX_WILDCARD");

		#if 0 // duplicate case value as VEID_DTMF_DIGIT_MASK
		case VEID_RFC2833_RX_MASK: 
			return("VEID_RFC2833_RX_MASK");
		#endif

		#if 0 // duplicate case value as VEID_RFC2833_RX_WILDCARD
		case VEID_RFC2833_RX_DTMF_0: 
			returreturn("VEID_RFC2833_RX_DTMF_0");
		#endif

		case VEID_RFC2833_RX_DTMF_1: 
			return("VEID_RFC2833_RX_DTMF_1");
		case VEID_RFC2833_RX_DTMF_2: 
			return("VEID_RFC2833_RX_DTMF_2");
		case VEID_RFC2833_RX_DTMF_3: 
			return("VEID_RFC2833_RX_DTMF_3");
		case VEID_RFC2833_RX_DTMF_4: 
			return("VEID_RFC2833_RX_DTMF_4");
		case VEID_RFC2833_RX_DTMF_5: 
			return("VEID_RFC2833_RX_DTMF_5");
		case VEID_RFC2833_RX_DTMF_6: 
			return("VEID_RFC2833_RX_DTMF_6");
		case VEID_RFC2833_RX_DTMF_7: 
			return("VEID_RFC2833_RX_DTMF_7");
		case VEID_RFC2833_RX_DTMF_8: 
			return("VEID_RFC2833_RX_DTMF_8");
		case VEID_RFC2833_RX_DTMF_9: 
			return("VEID_RFC2833_RX_DTMF_9");
		case VEID_RFC2833_RX_DTMF_STAR: 
			return("VEID_RFC2833_RX_DTMF_STAR");
		case VEID_RFC2833_RX_DTMF_POUND: 
			return("VEID_RFC2833_RX_DTMF_POUND");
		case VEID_RFC2833_RX_DTMF_A: 
			return("VEID_RFC2833_RX_DTMF_A");
		case VEID_RFC2833_RX_DTMF_B: 
			return("VEID_RFC2833_RX_DTMF_B");
		case VEID_RFC2833_RX_DTMF_C: 
			return("VEID_RFC2833_RX_DTMF_C");
		case VEID_RFC2833_RX_DTMF_D: 
			return("VEID_RFC2833_RX_DTMF_D");
		case VEID_RFC2833_RX_DTMF_FLASH: 
			return("VEID_RFC2833_RX_DTMF_FLASH");
		case VEID_RFC2833_RX_DATA_FAX_ANS: 
			return("VEID_RFC2833_RX_DATA_FAX_ANS");
		case VEID_RFC2833_RX_DATA_FAX_ANSBAR: 
			return("VEID_RFC2833_RX_DATA_FAX_ANSBAR");
		case VEID_RFC2833_RX_DATA_FAX_ANSAM: 
			return("VEID_RFC2833_RX_DATA_FAX_ANSAM");
		case VEID_RFC2833_RX_DATA_FAX_ANSAMBAR: 
			return("VEID_RFC2833_RX_DATA_FAX_ANSAMBAR");
		case VEID_RFC2833_RX_DATA_FAX_CNG: 
			return("VEID_RFC2833_RX_DATA_FAX_CNG");
		case VEID_RFC2833_RX_DATA_FAX_V21_CH1_B0: 
			return("VEID_RFC2833_RX_DATA_FAX_V21_CH1_B0");
		case VEID_RFC2833_RX_DATA_FAX_V21_CH1_B1: 
			return("VEID_RFC2833_RX_DATA_FAX_V21_CH1_B1");
		case VEID_RFC2833_RX_DATA_FAX_V21_CH2_B0: 
			return("VEID_RFC2833_RX_DATA_FAX_V21_CH2_B0");
		case VEID_RFC2833_RX_DATA_FAX_V21_CH2_B1: 
			return("VEID_RFC2833_RX_DATA_FAX_V21_CH2_B1");
		case VEID_RFC2833_RX_DATA_FAX_CRDI: 
			return("VEID_RFC2833_RX_DATA_FAX_CRDI");
		case VEID_RFC2833_RX_DATA_FAX_CRDR: 
			return("VEID_RFC2833_RX_DATA_FAX_CRDR");
		case VEID_RFC2833_RX_DATA_FAX_CRE: 
			return("VEID_RFC2833_RX_DATA_FAX_CRE");
		case VEID_RFC2833_RX_DATA_FAX_ESI: 
			return("VEID_RFC2833_RX_DATA_FAX_ESI");
		case VEID_RFC2833_RX_DATA_FAX_ESR: 
			return("VEID_RFC2833_RX_DATA_FAX_ESR");
		case VEID_RFC2833_RX_DATA_FAX_MRDI: 
			return("VEID_RFC2833_RX_DATA_FAX_MRDI");
		case VEID_RFC2833_RX_DATA_FAX_MRDR: 
			return("VEID_RFC2833_RX_DATA_FAX_MRDR");
		case VEID_RFC2833_RX_DATA_FAX_MRE: 
			return("VEID_RFC2833_RX_DATA_FAX_MRE");
		case VEID_RFC2833_RX_DATA_FAX_CT: 
			return("VEID_RFC2833_RX_DATA_FAX_CT");
		case VEID_DSP_DTMF_CLID_END: 
			return("VEID_DSP_DTMF_CLID_END");
		case VEID_DSP_FSK_CLID_END: 
			return("VEID_DSP_FSK_CLID_END");
		case VEID_DSP_FSK_CLID_TYPE2_NO_ACK: 
			return("VEID_DSP_FSK_CLID_TYPE2_NO_ACK");
		case VEID_DSP_LOCAL_TONE_END: 
			return("VEID_DSP_LOCAL_TONE_END");
		case VEID_DSP_REMOTE_TONE_END: 
			return("VEID_DSP_REMOTE_TONE_END");
		case VEID_DSP_LOCAL_TONE_USER_STOP: 
			return("VEID_DSP_LOCAL_TONE_USER_STOP");
		case VEID_DSP_REMOTE_TONE_USER_STOP: 
			return("VEID_DSP_REMOTE_TONE_USER_STOP");
#ifdef DATA_OVER_TCP 
		case VEID_TCP_FAILURE_CONNECT:
			return("VEID_TCP_FAILURE_CONNECT");
#endif
		default:
			return("unknown");
	}

	return("unknown");

}
