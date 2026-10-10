#ifndef MGCC_H
#define MGCC_H

//#include "voip_manager.h"
#include "rcm_tapi.h"

#ifdef CONFIG_RTK_VOIP_TR104
#include "cwmpevt.h"
#endif
#include "uglib.h"

#define MAXHOSTNAMELEN 128
#define LINE_MAX_LEN	100		/* Don't modify */
#define TRUE 1
#define FALSE 0
#define MAX_ARGC 8
#define CHANNEL_IS_FXS(chid)	RTK_VOIP_IS_SLIC_CH( chid, g_VoIP_Feature )
#define SERVER_SOCK "/tmp/mg_server.sock"
#define _PATH_TMP_STATUS "/tmp/voipstatus"
#define _PATH_TMP_E8C_STATUS "/tmp/E8C_voipstatus"

#ifdef CONFIG_RTK_VOIP_TR104
#define SOLAR_CHANNEL_PATH "/var/solar.chanl"
#define CWMP_CLINET_PATH "/var/cwmpclient.chanl"
#endif

typedef enum
{
    SDPMODE_NOTSET    = 0,
    SDPMODE_SENDONLY,       /* "sendonly" */
    SDPMODE_RECVONLY,       /* "recvonly" */
    SDPMODE_SENDRECV,       /* "sendrecv" */
    SDPMODE_INACTIVE        /* "inactive" */
} SdpMode;

typedef enum
{
    DIGITMODE_DD,
    DIGITMODE_TONEDET    
} DigitMode;


typedef enum
{
    STATE_IDLE,
    STATE_OFFHOOK,
    STATE_RINGING,
    STATE_CONNECTED
} LineState;

typedef enum _ProxyRegStatus {
	REGSTATUS_INIT,
	REGSTATUS_REG_ING,				/* registering */
	REGSTATUS_REG_SUCCESS,			/* register success */
	REGSTATUS_REG_SUCCESS_EXPIRED,	/* register success but expired, so it is registering */
	REGSTATUS_REG_FAIL,				/* register failure (don't retry) */
	REGSTATUS_REG_FAIL_RETRY_S,		/* register failure (short delay retry) */
	REGSTATUS_REG_FAIL_RETRY_L,		/* register failure (long delay retry) */
	REGSTATUS_RESTART				/* restart call manager */
} ProxyRegStatus;

enum{
	VOIP_STATUS_INIT,
	VOIP_STATUS_SUCCESS,
	VOIP_NOVALID_CONNECTION,	
	VOIP_STATUS_IADERROR,	
	VOIP_STATUS_NOROUTE,
	VOIP_STATUS_NORESPONSE,	
	VOIP_STATUS_ACCOUNTERR,
	VOIP_STATUS_UNKNOWN,	
	VOIP_STATUS_LINE_DISABLED,	
	VOIP_STATUS_ADDR_UNKNOW_HOST,
	VOIP_STATUS_ADDR_ERR
};


typedef enum
{
    PRIMARY_MGC    = 0,
    SECONDARY_MGC      
} Mgc_list;


typedef enum
{
	EXAMPLE_FSK_PARAM_NULL = 0,					///<
	EXAMPLE_FSK_PARAM_DATEnTIME = 0x01,			///< Date and Time
	EXAMPLE_FSK_PARAM_CLI = 0x02,				///< Calling Line Identify (CLI)
	EXAMPLE_FSK_PARAM_CLI_ABS = 0x04,			///< Reason for absence of CLI
	EXAMPLE_FSK_PARAM_CLI_NAME = 0x07,			///< Calling Line Identify (CLI) Name
	EXAMPLE_FSK_PARAM_CLI_NAME_ABS = 0x08,		///< Reason for absence of (CLI) Name
	EXAMPLE_FSK_PARAM_MW = 0x0b,				///< Message Waiting

}EXAMPLE_TfskParaType;


typedef struct rtp_config
{
	guint16 audio_rtp_port;
	gint audio_jitt_comp;  /*jitter compensation*/
	guint8 g7231_rate;
	guint8 iLBC_mode;
	guint8 speex_nb_rate;
	guint8 bVAD;
	guint8 nVAD_thr;
	guint8 bCNG;
	guint8 nCNG_thr;
	guint8 nSID_mode;
	guint8 nSID_level;
	gint16 nSID_gain;
	guint8 bPLC;
	guint8 nRtcpInterval;
	guint8 bRtcpXR;
	guint32 nJitterDelay;
	guint32 nMaxDelay;
	guint32 nMaxStrictDelay;
	guint32 nJitterFactor;
	guint32 nG726Packing;
#ifdef SUPPORT_G7111
	guint8 g7111_precedence[G7111_MODES];
#endif
#ifdef SUPPORT_VOICE_QOS
	guint8 tos;
#endif
	guint8 *pFrameSize;

} user_rtp_config_t;

typedef struct net_config
{
	double bandwidth; 		/* the bandwidth in bit/s of the selected interface*/ /*unused*/
	gchar *nat_address;
	/* Mandy add for stun support */
	gint nat_port;
//	gint nat_voice_port[MAX_SS];
	gint nat_t38_port;
	/* Mandy: Keep NAT entry alive */
	int udp_keepAlive_frequency;
	int natEntry_build_time;
	gboolean use_nat;
	gint con_type;			/* type of connection 28k 56k 128k adsl ...*/
}net_config_t;

//typedef struct codecs_config
//{
//	GList *audio_codecs;  /* list of audio codecs in order of preference*/
//}codecs_config_t;


struct _MgCore;

typedef struct _StreamParams
{
	int initialized;
	int line;
	int localport;
	int remoteport;
//	char *remoteaddr;
	unsigned long remote_ip;
	char remote_addr[16]; //xxx.xxx.xxx.xxx
#ifdef DYNAMIC_PAYLOAD
	int local_pt;
	int remote_pt;
	int static_pt;	// static pt record codec in rtk dsp
	int rfc2833_local_pt;
	int rfc2833_remote_pt;
#else
	int pt;
#endif
#ifdef SUPPORT_V152_VBD
	int initialized_vbd;
	int local_pt_vbd;
	int remote_pt_vbd;
	int static_pt_vbd;	// static pt record codec in rtk dsp
#endif
#ifdef SUPPORT_RTP_REDUNDANT_APP
	int initialized_rtp_redundant;
	int local_pt_rtp_redundant;
	int remote_pt_rtp_redundant;
#endif
#ifdef CONFIG_RTK_VOIP_SRTP
	char remoteSDPKey[SDP_KEY_LEN];
	char localSDPKey[SDP_KEY_LEN];
	int remoteCryptAlg;
	int localCryptAlg;
#endif /*CONFIG_RTK_VOIP_SRTP*/
#ifdef SUPPORT_G7111
	guint8 g7111_precedence[G7111_MODES];
#endif
} StreamParams;

typedef struct SystemParam_s {
	// 3 way conference command
	unsigned char conference_code[ 20 ];
	unsigned char stutter_tone;
	unsigned char fxo_redial;
	int proxy_timeout_count_limit;
	int proxy_timeout_retry_sec;
#ifdef SUPPORT_G7111
	int g711u_wb_pt;
	int g711a_wb_pt;
#endif
	int g726_16_pt;
	int g726_24_pt;
	int g726_32_pt;
	int g726_40_pt;
	int rtp_init;
	unsigned long rtp_init_seqno;
	unsigned long rtp_init_ssrc;
	unsigned long rtp_init_timestamp;
	int red_demo;
	int red_audio;
	int red_rfc2833;
	int fpp_demo;
	int fpp;
	int fppvbd;
	unsigned long silence_demo;
	unsigned long silence_dbov;
	unsigned long silence_period;
	unsigned char modem_lec;
	unsigned long ivr_delay;
	unsigned long max_strict_delay;
	int qos_dscp;
} SystemParam_t;


typedef struct _MGAPP MGAPP;

typedef struct _RtpTerm
{
    int         chid;
    int         ssid; 
    gboolean    active;
    char        termName[128];
}RtpTerm;

typedef struct _MgCore
{
	MGAPP *parent;
	int chid;
	net_config_t net_conf;
	user_rtp_config_t rtp_conf;
//	codecs_config_t codecs_conf;
//	ui_config_t ui_conf;
	char dial_code[LINE_MAX_LEN]; //record dial number 
	int	digit_index;

#ifdef CONFIG_RTK_VOIP_DIALPLAN
	unsigned int dial_data_index;
	char dial_data[LINE_MAX_LEN]; //for dialplan
#endif
	int offhook_timer; //timeout play WARNING (alert) tone
//	int busytone_timer; //timeout play busy tone
	int dialtimeout_timer; // dial interval timeout 
	int flashtimeout_timer; //press flash timeout 
	int stop_alltone_timer;
	int register_retry_timer; //retry register server timer
	int e8c_tr104_timer;	//only for e8c test 
	// caller id/name from fxo
	char caller_id[21];
	char caller_id_name[51];
	gboolean caller_id_hidden; // 0: display callerid ,  1: anonymous call 
	
	int hotline_timer;	/* E8C , delay sometime to dial hotline */
	int no_Answer_timer_id;
	int no_Answer_timeout;
	int rtpTermNum;
	char termName[128];
	RtpTerm rtp_term[2];
    int line_state;
    int digitReport;
    int reg_status;
} MgCore;

struct _MGAPP
{
	MgCore ports[MAX_VOIP_PORTS];
	int in_main_thread;
	int bAutoAnswer;
	unsigned long port_link_status;
	int rcm_mg_reset_timer;	//for ProxyRegStatusconfigure change , need wait all voip call disconnect.
	int rcm_mg_watchdog_timer; /*for watch dog timer */
	char bound_voip_ipaddress[MAXHOSTNAMELEN];
	char current_voip_interface[MAXHOSTNAMELEN];
	int heartBeat_timer;
    int check_reg_timer;
    int keep_alive_timer;
    int call_waiting_timer;
    int reg_status;
};



int app_init(MGAPP *pApp);
int app_close(MGAPP *pApp);
int rcm_send_msg(char *msg);
int mg_send_serviceChange(void);
int mg_init(void);
int mg_start(void);
int mg_addTerm(void);
int mg_stop(void);
int mgccEventEx(char *termName, SIGNSTATE pval);
void mg_send_dtmf(int chid, uint32 ssid, SIGNSTATE digit);

void mg_Diagnosis_Auto_Test(int chid);
void mg_tr104_Auto_Call_Test_Init(int chid);
void mg_tr104_Auto_Call_Test_Start(int chid);
void mg_tr104_Auto_Call_Test_Recode_DTMF(int chid , char ent);
void mg_tr104_Auto_Call_Test_Finish(int chid, int callResult,int failReason, int DailDTMFConfirmResult , int FailedResponseCode);
void mg_tr104_Register_Test_Finish(int chid, int registResult, int reason);
int  mg_tr104_Register_Test_Enable(int chid);

void mg_e8c_dtmf_test(void *arg);


#endif
