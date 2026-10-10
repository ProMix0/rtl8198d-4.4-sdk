#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "voip_manager.h"

// test option:
// 	- FXO_DIAL_FXS0 has defined
//		* fxo ring will dial fxs0 automatically
//	- FXO_DIAL_FXS0 has not defined
//		* fxo ring will prompt user dial voip number via IVR
//  - Allow receiving fax from FXO line

#define FAX_USE_T38  //undef this for G.711 passthrough
#define FXO_DIAL_FXS0_AUTO

#define FUNCKEY_FXO_0	".0" /* keypad presses *0 to dial FXO0 */
#define FUNCKEY_FXO_1	".1"
#define FUNCKEY_FXO_2	".2"
#define FUNCKEY_FXO_3	".3"

#define FUNCKEY_FXS_0 	".4" /* keypad presses *4 to dial FXS0 */
#define FUNCKEY_FXS_1 	".5" /* keypad presses *5 to dial FXS1 */

#define CHANNEL_IS_FXS(chid)	RTK_VOIP_IS_SLIC_CH( chid, g_VoIP_Feature )
#define CHANNEL_IS_FXO(chid)	RTK_VOIP_IS_DAA_CH( chid, g_VoIP_Feature )

#define SELECT_WAIT_SEC		5
//#define DEBUG_FXO		

#define SUCCESS 0
#define FAIL -1
// global variable
int fxo_type;
int g_VoIPPorts = 0;
//channel_t channels[CON_CH_NUM];	
char dtmfbuf[64];
int dtmf_index = 0;
int onhook_count = 0;
int error_count = 0;

#ifdef DEBUG_FXO 
char * get_evt_name(VoipEventID evt);
#endif
int do_fxo_event( int chid, SIGNSTATE evt );

enum {
	FXO_NO_DAA = 0,
	FXO_VIRTUAL_DAA,
	FXO_DAA
};

enum {
	STATE_IDLE = 0,
	STATE_RING,
	STATE_CONNECT,
	STATE_DAA_RING,			// virtual daa state
	STATE_DAA_CONNECT		// virtual daa state
};

enum {
	DIAL_NONE = 0,
	DIAL_FXO_0,
	DIAL_FXO_1,
	DIAL_FXO_2,
	DIAL_FXO_3,
	DIAL_FXS_0,
	DIAL_FXS_1,
	DIAL_OTHER,
};

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
	channel_t *remote;
	//FAX
	uint32 fax;
	int fax_clear;
	uint32 fax_end;
	int fax_end_clear;
};

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
	channel->remote = NULL;
	channel->fax_clear = 1;
}

int doCompare(char dtmf[]){
	int ret = SUCCESS;
	char *dtmfstr = "*159#";
	
	//printf("dtmf [%s]\n", dtmf);
	//printf("dtmfstr [%s], strlen(dtmfstr) %d\n", dtmf, strlen(dtmfstr));
	if (strncmp(dtmf, dtmfstr, strlen(dtmfstr))){
		ret = FAIL;
		if ( onhook_count == 0 )
			printf("Please key in *159#\n");
		return ret;
	}
	return ret;
}

int do_fxo_event( int chid, SIGNSTATE val )
{
	int ret = SUCCESS;
	//channel_t *channel;
	//channel = &channels[chid];

	switch (val)
	{
	case SIGN_KEY1:
		printf("DTMF KEY 1\n");
		dtmfbuf[dtmf_index] = '1';
		dtmf_index ++ ;
		break;
	case SIGN_KEY2:
		printf("DTMF KEY 2\n");
		dtmfbuf[dtmf_index] = '2';
		dtmf_index ++ ;
		break;
	case SIGN_KEY3:
		printf("DTMF KEY 3\n");
		dtmfbuf[dtmf_index] = '3';
		dtmf_index ++ ;
		break;
	case SIGN_KEY4:
		printf("DTMF KEY 4\n");
		dtmfbuf[dtmf_index] = '4';
		dtmf_index ++ ;
		break;
	case SIGN_KEY5:
		printf("DTMF KEY 5\n");
		dtmfbuf[dtmf_index] = '5';
		dtmf_index ++ ;	
		break;
	case SIGN_KEY6:
		printf("DTMF KEY 6\n");
		dtmfbuf[dtmf_index] = '6';
		dtmf_index ++ ;	
		break;
	case SIGN_KEY7:
		printf("DTMF KEY 7\n");
		dtmfbuf[dtmf_index] = '7';
		dtmf_index ++ ;	
		break;
	case SIGN_KEY8:
		printf("DTMF KEY 8\n");
		dtmfbuf[dtmf_index] = '8';
		dtmf_index ++ ;		
		break;
	case SIGN_KEY9:
		printf("DTMF KEY 9\n");
		dtmfbuf[dtmf_index] = '9';
		dtmf_index ++ ;	
		break;
	case SIGN_KEY0:
		printf("DTMF KEY 0\n");
		dtmfbuf[dtmf_index] = '0';
		dtmf_index ++ ;	
		break;
	case SIGN_STAR:
		printf("DTMF KEY *\n");
		dtmfbuf[dtmf_index] = '*';
		dtmf_index ++ ;
		break;
	case SIGN_HASH:
		printf("DTMF KEY #\n");
		dtmfbuf[dtmf_index] = '#';
		dtmf_index ++ ;
		break;
	case SIGN_ONHOOK:
		/* when phone onhook, stop play tone, disable pcm and DTMF detection */
		//rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, RTK_DISABLE, DSPCODEC_TONEDIRECTION_LOCAL);
		//usleep(100000); // [Important] sleep >= 100ms. MUST add delay for ACMW to stop tone!
		if ( onhook_count == 1 )
			onhook_count ++ ; 
		//channel_handle_onhook(channel);
		rtk_OnHookAction(chid);	// rtk_OnHookAction have to be the last step in SIGN_ONHOOK
		ret = doCompare(&dtmfbuf[0]) ;
		//printf("ret %d\n", ret);
		if ( ret == SUCCESS  ){
			rtk_SetRingFxs(chid, 1);
			onhook_count ++ ; 
		}else
			error_count ++;
		
		memset(dtmfbuf, 0x0, sizeof(dtmfbuf));
		dtmf_index = 0;		
		
		if ( onhook_count >= 2 || error_count >= 3){
			exit(0);
		}
		break;
	case SIGN_OFFHOOK:
		rtk_OffHookAction(chid);	// rtk_OffHookAction have to be the first step in SIGN_OFFHOOK
		//channel_handle_offhook(channel);
		break;
	case SIGN_FLASHHOOK:
		break;
	case SIGN_OFFHOOK_2: // off-hook but no event
		break;
	case SIGN_NONE:
		break;
	default:
		printf("unknown event(chid:%d, sign:0x%08X)\n", chid, val);
		break;
	}

	return 0;
}

int main(void)
{
	int i;
	int ret;
	int chid;
	fd_set fdset;
	struct timeval timeout;
	TstFlushVoipEvent stFlushVoipEvent;
	TstVoipEvent stVoipEvent;
	SIGNSTATE val;

	ret = system("killall voip_gwdt");
	sleep(2);
//	printf("Fxo_int use select mode to get slic event, timeout for select is %d\n", SELECT_WAIT_SEC);
//	printf("==off hook and dial FUNCTION KEY==\n");
//	printf("*4: dial to fxs 0\n");
//	printf("*5: dial to fxs 1\n");
	memset(dtmfbuf, 0x0, sizeof(dtmfbuf));
	
	rtk_GetVoIPFeature();
	
	if ((g_VoIP_Feature & DAA_TYPE_MASK) == NO_DAA)
		fxo_type = FXO_NO_DAA;
	else if ((g_VoIP_Feature & DAA_TYPE_MASK) == VIRTUAL_DAA)
		fxo_type = FXO_VIRTUAL_DAA;
	else
		fxo_type = FXO_DAA;

	g_VoIPPorts = RTK_VOIP_SLIC_NUM(g_VoIP_Feature);
	//RTK_VOIP_SLIC_NUM( g_VoIP_Feature )

	// init 
	for (chid=0; chid<CON_CH_NUM; chid++)
	{
		// init dsp
		rtk_InitDsp(chid);

		// init phone
		rtk_SetVoiceGain(chid, 0, 0);	// 0db
		//rtk_SetEchoTail(chid, 64, 1, 5, 5, 0);			// 4ms, NLP on
		rtk_SetEchoTail(chid, 32, 1, 5, 5, 0);			// 4ms, NLP on

		// init fxs & fxo
		if (CHANNEL_IS_FXS(chid))
		{
			// fxs
			rtk_SetFlashHookTime(chid, 100, 300,800);		// 100~300 ms
			//rtk_SetSlicRingCadence(chid, 1500, 1500);	// 1500 ms
			rtk_SetDtmfCidParam(chid, DTMF_DIGIT_A, DTMF_DIGIT_C, 0xf, 80, 80, 300, 300);	
			// priori 1st ring, auto AC, auto ring, digit duration = 80ms, inter digit pause = 80ms
		}
		else
		{
/*
cid_det_mode  The Caller ID Mode for Caller ID Detection (available only when the auto_det=0) 
Available range: 0 ~ 4 
0: Bellcore FSK 
1: ETSI FSK 
2: BT FSK 
3: NTT FSK 
4: DTMF 
Default: 4  

*/
			// fxo
			rtk_SetFxoRingDetection(300, 300, 4500);
			rtk_SetCidDetMode(chid,
				2,										// auto detect mode (NOT NTT)
				4
			);
		}

		// set dtmf mode
		rtk_SetDtmfMode(chid, DTMF_INBAND);

		// set fax modem det mode
		//rtk_SetFaxModemDet(chid, 0); //0:auto-hi-speed-fax
		rtk_SetFaxModemDet(chid, 3); //3:auto_2

		// set fax event detection
		//rtk_SetAnswerToneDet(chid , 0x31212|0x3c0000 , 0 , 10);
		rtk_SetAnswerToneDet(chid , 0x3f1212 , 0 , 10);

		// flush kernel fifo before app run
		rtk_SetFlushFifo(chid);

		// init local data
		//channel_init(&channels[chid], chid);

		rtk_OnHookReinit(chid);

		// flush all voip events 
		stFlushVoipEvent.ch_id = chid;
		stFlushVoipEvent.type = VET_ALL;
		stFlushVoipEvent.mask = VEM_ALL;
		rtk_FlushVoIPEvent( &stFlushVoipEvent );
	}

	#ifdef DEBUG_FXO
	printf( "Wait %d seconds...", SELECT_WAIT_SEC );
	fflush( stdout );
	#endif
	printf("Voip MPtest\n");
	// main loop
	while (1) 
	{

		// set fd for select 
		FD_ZERO( &fdset );
		FD_SET( g_VoIP_Mgr_FD, &fdset );
		
		// set timeout for select
		timeout.tv_sec = SELECT_WAIT_SEC;
		timeout.tv_usec = 0;
		
		// select 
		ret = select( g_VoIP_Mgr_FD + 1, &fdset, NULL, NULL, &timeout );
		
		if( ret == 0 ) {
			//printf( "timeout\n" );
			continue;
		} 

		#ifdef DEBUG_FXO
		printf( "got it\n" );
		#endif
		
		// fill input data & get it 
		for ( chid = 0 ; chid <CON_CH_NUM ; chid ++ )
		{
			// read all events
			for ( i = 0 ; ; i ++ )
			{
				memset ((void*)&stVoipEvent, 0, sizeof(TstVoipEvent));
				stVoipEvent.ch_id = chid;
				stVoipEvent.type = VET_ALL;
				stVoipEvent.mask = VEM_ALL;
				
				rtk_GetVoIPEvent( &stVoipEvent );
				
				// no more data 
				if( stVoipEvent.id == VEID_NONE )
					break;

				// print out 
				#if 0 //def DEBUG_FXO
				printf( "[%d] on chid=%d\n", i, stVoipEvent.ch_id );
				printf( "\ttype=%08X mask=%08X\n", stVoipEvent.type, stVoipEvent.mask );
				printf( "\tID=%s(%08X), p0=%u p1=%u @%08X\n", get_evt_name(stVoipEvent.id), stVoipEvent.id, 
						stVoipEvent.p0, stVoipEvent.p1, stVoipEvent.time );
				#endif

				if (CHANNEL_IS_FXS(chid))
					rtk_ProcessFxsEvent( chid, stVoipEvent.id, &val );
				else
					rtk_ProcessFxoEvent( chid, stVoipEvent.id, &val );

				do_fxo_event( stVoipEvent.ch_id, val );
			}
		}
	} /* while(1) main loop */

	return 0;
}


#ifdef DEBUG_FXO
char * get_evt_name(VoipEventID evt)
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
		default:
			return("unknown");
	}

	return("unknown");

}
#endif

