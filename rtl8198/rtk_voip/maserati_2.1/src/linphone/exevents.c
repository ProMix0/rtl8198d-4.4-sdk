/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <ctype.h>
#include <sys/sysinfo.h>
#include "eXosip2/eXosip.h"
#include "eXosip2.h"
#include "linphonecore.h"
#include "misc.h"
#include "uglib.h"
#include "payloadtype.h"
#include "exevents.h"
#include "alarm.h"
#include "dialplanhelp.h"
#include "rcm_timer.h"
#include "../oRTP/avprofile.h"
#include "../osip2/osip_replaces.h"

#ifdef CONFIG_RTK_VOIP_TR104
#include <sys/un.h>
#include "cwmpevt.h"
#endif /*CONFIG_RTK_VOIP_TR104*/

#ifdef CONFIG_USER_DBUS_PROXY
#include "smart_proxy_common.h"

#ifndef e_dbus_proxy_signal_voip_phone_offhook_id
#define e_dbus_proxy_signal_voip_phone_offhook_id 0x800f
#endif
#ifndef e_dbus_proxy_signal_voip_phone_onhook_id
#define e_dbus_proxy_signal_voip_phone_onhook_id 0x8010
#endif
#ifndef e_dbus_proxy_signal_voip_phone_register_change_id
#define e_dbus_proxy_signal_voip_phone_register_change_id 0x8011
#endif

#endif

#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "rcm_customize.h"
#define CALLER_ID_NAME_LENGTH 51

extern void linphone_register_authentication_required(LinphoneCore *lc,eXosip_event_t *ev);
extern void linphone_invite_authentication_required(LinphoneCore *lc, int sid);
extern int linphone_call_authentication_required(LinphoneCore *lc, LinphoneCall *lcall, eXosip_event_t *ev);
extern void linphone_subscription_new(LinphoneCore *lc,int did, int sid, char *from, char* contact);
extern void linphone_notify_recv(LinphoneCore *lc,char *from,int ss_status);
extern LinphoneProxyConfig *linphone_core_get_proxy_config_from_rid(LinphoneCore *lc, int rid);
void linphone_subscription_answered(LinphoneCore *lc,char *from, int sid);
void linphone_subscription_closed(LinphoneCore *lc,char *from, int did);
//[20110805, SD6, ericchung, rcm integration
void rcm_linphone_process_sipinfo(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev);
void rcm_linphone_process_refer(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev);
void rcm_process_other_request(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev);
int GetChIDfromOptionsTid(TAPP *pApp, eXosip_event_t *ev);
void rcm_linphone_notify_rec(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev);
void rcm_send_fsk_date(int chid,char *date_str);
void rcm_arp_test(LinphoneCore *lc);
void caller_id_send_P(int chid);
void rcm_linphone_message_rec(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev);


//]

/* these are useful messages for the status */
extern gchar ready[] ;
extern gchar end[] ;
extern gchar contacting[] ;
extern gchar connected[] ;
extern gchar cancel[] ;
extern gchar contacted[] ;

//[SD6, bohungwu, exosip 3.5 integration 
extern eXosip_t eXosip;

#ifdef CONFIG_RTK_VOIP_TR104 //for e8c and tr104 status
extern cwmpEvtMsg rcm_tr104_st; 
#endif

#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
typedef struct arg_struct {
    uint32 chid;
    uint32 dtmf_mode;
    char call_id[32];
    char name[CALLER_ID_NAME_LENGTH];
    char date[32];
    time_t usec;
}arg_struct_t;

arg_struct_t args[CON_CH_NUM];
#endif


//]

//#define VMWI_TEST	/* thlin+ for test VMWI */

//======================== Callee side CID only =========================
char caller_id[21];	// hc+ 1130 for get caller ID from SIP message header
char caller_id_name[CALLER_ID_NAME_LENGTH];	// display the name of caller id.
char fsk_dsil_chk[4]={0};	// hc+ 1215 for FSK CID
char caller_id_mode[4]={0};	// CID mode, 1:FSK, 0:DTMF
char call_waiting_cid[4]={0, 0, 0, 0};// call waiting caller ID gen, 1: enable 0: disable
//===================================================================================
static int 
e8c_replace_caller_id( char *cid , char *replace_source, char *replace_target)
{
	int i = 0;
	char *loc = NULL;
	char tmp_caller_id[21] = {0};
	g_message( "before replace cid caller_id = %s replace_source=%s replace_target =%s\n" , cid , replace_source, replace_target);
	loc = strstr( cid , replace_source);
	if(( loc != NULL ) && ( loc == (char *)cid )) {
		strcpy( tmp_caller_id , replace_target );
		cid += strlen(replace_source);
		strncat( tmp_caller_id , cid , strlen( cid ));
		strncpy( caller_id , tmp_caller_id , strlen( tmp_caller_id ));
		caller_id[ strlen(tmp_caller_id) ] = '\0';
		g_message( "after replace cid caller_id = %s\n" , caller_id );
	}
	return 0;
}
#if 1 /* CLIP */
typedef struct caller_id_map {
	const char *original; 
	const char *modify  ;
	const char *txt_log ;
} caller_id_map_t ;
/*for customer , need check ISP provider */
caller_id_map_t caller_id_map_list[] =  /* smaller index, higher priority */
{ /* original , modify , txt_log */
	{ "+86750"  , ""  , "Caller ID Replace" },
	{ "+867"  , "07"  , "Caller ID Replace" },
	{ "+86"  , ""  , "Caller ID Replace" },
// 	{ "86"   , ""  , "Caller ID Replace" },
	{ "+"    , "" , "Caller ID Replace" },
};

static int 
replace_caller_id( char *cid )
{
	int i = 0;
	char *loc = NULL;
	char tmp_caller_id[21] = {0};

	caller_id_map_t *cid_map = &caller_id_map_list[0];
	const int cid_map_len = sizeof(caller_id_map_list) / sizeof(caller_id_map_t);
	
	for( i = 0 ; i < cid_map_len ; i++ ){
		loc = strstr( cid , cid_map[i].original );
		if(( loc != NULL ) && ( loc == (char *)caller_id )) {
			g_message( "before replace cid caller_id = %s\n" , caller_id );
			//printf("%s(%d) before replace caller_id = %s %p loc = %s %p\n" ,  
			//	__FUNCTION__ , __LINE__ , caller_id , caller_id , loc , loc);

			strncpy( tmp_caller_id , cid_map[i].modify , strlen( cid_map[i].modify ));
			cid += strlen( cid_map[i].original );
			strncat( tmp_caller_id , cid , strlen( cid ));
			strncpy( caller_id , tmp_caller_id , strlen( tmp_caller_id ));
			caller_id[ strlen(cid_map[i].modify)+strlen(cid) ] = '\0';
			
			g_message( "after replace cid caller_id = %s\n" , caller_id );
			//printf("%s(%d) after replace caller_id = %s\n" ,  
			//	__FUNCTION__ , __LINE__ , caller_id);

			break;
		}
	}

	return 0;
}
#endif

//[SD6, ericchung, exosip 3.5 integration
int rcm_exosip_call_ack(LinphoneCore *lc, uint32 ssid,eXosip_event_t *ev)
{

	osip_message_t *msg=NULL;
	
	g_message("rcm_exosip_call_ack , lc->chid %d\n",lc->chid);
	if (lc->call[ssid] == NULL)
	{
		g_warning("Call null[%d][%d]\n", lc->chid, ssid);
		g_warning("rcm_exosip_call_ack Call null[%d][%d]\n", lc->chid, ssid);
	
		return -1;
	}
	
	
	

	//eric: build ack for 200ok
	eXosip_call_build_ack(ev->did,&msg);
	if(lc->call[ssid]->proxy!=NULL){
		g_message("lc->call[ssid]->proxy contact is %s\n",lc->call[ssid]->proxy->contact);
	_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
	osip_message_set_contact(msg,lc->call[ssid]->proxy->contact);
	}

	eXosip_call_send_ack(ev->did,msg);

	return 0;
}


//make out-going call , receive peer 200ok , run call_accepted.
int linphone_call_accepted(LinphoneCore *lc, uint32 ssid, eXosip_event_t *ev)
{
//	StreamParams *audio_params;
	sdp_message_t *sdp_msg;
	LinphoneCall *call=lc->call[ssid];
	uint32 chid=lc->chid;
	int another_ssid=0;
	int no_sdp=0;
#if 1 // 3pty on server	
	char conference_uri[128];
	char refer_to[256];
#endif
	set_autocall_state(lc,YUEME_TEST_CONNECT);	
	
	g_message(" linphone_call_accepted \n");
	if( call == NULL )
	{
		g_warning( "No call to accept.\n" ) ;
		return 0 ;
	}

//	audio_params = &call->audio_params ;
	call->auth_failed = FALSE ;

	another_ssid=(ssid == 0 ? 1 : 0);
	SESSIONSTATE ss = GetSessionState( chid , another_ssid ) ;

	sdp_msg=eXosip_get_sdp_info(ev->response);

	if( sdp_msg==NULL)
	{
		g_message( "No sdp body in 200ok.\n" ) ;
		no_sdp=1;
		/* merge from claro:some sip server use 183 with sdp to set rtp , so 200ok no sdp . */
		//return 0 ;
	}else{
		sdp_message_free(sdp_msg);

	}
	g_message("\r\n call->state %d \n",call->state);
	if (GetSysState(chid) == SYS_STATE_IN3WAY_ONGOING_2)
	{	
		if (atdfr_construct_referto(lc, refer_to) == 0)
			linphone_core_transfer_call(lc, 2, refer_to);
		SetSysState(chid,SYS_STATE_IN3WAY_ONGOING_3);
		g_message("3 PTY ON SERVER SEND REFER 1 \n");
	}

	switch (call->state)
	{
	case LCStateHold:
		// receive hold response
#if 1 // 3pty on server
		if (GetSysState(chid) == SYS_STATE_IN3WAY_ONGOING)
		{
			//send 3pty
			sprintf(conference_uri,"sip:%s",g_pVoIPCfg->conference_uri);
			//sprintf(conference_uri, "sip:%s", "<username>@<sip_server>");
			g_message("send %s\n",conference_uri);
			linphone_core_invite(lc, 2, conference_uri);
			SetSysState(chid,SYS_STATE_IN3WAY_ONGOING_2);
			linphone_core_stop_media_streams(lc, ssid);
        }
		else
		{
#endif	
		SetCallState(  chid , ssid ,&call->state, LCStateHoldACK) ;

		if((lc->call[another_ssid])&&(lc->call[another_ssid]->state==LCStateBeResume)){
			g_message(" linphone_call_accepted another ssid %d call-state  is %d\n",another_ssid,lc->call[another_ssid]->state);

			if (linphone_core_offhood_event(lc, another_ssid) != 0) {
				// offhood failed, user need try again
				g_message("\r\n offhold ssid %d fail\n",another_ssid);
			}
		}else if((lc->call[another_ssid])&&(ss==SS_STATE_CALLIN)){// another call incoming , after hold ok , send 200 ok
			linphone_core_accept_dialog(lc, another_ssid, NULL);
		}
		SetActiveSession(chid, ssid == 0 ? 1 : 0, TRUE);
//		rcm_tapi_SetRtpSessionState(chid, ssid, rtp_session_sendonly);
		linphone_core_stop_media_streams(lc, ssid);
		if (GetSessionState(lc->chid, ssid == 0 ? 1 : 0) == SS_STATE_IDLE){
			//add check country
			if(g_pVoIPCfg->tone_of_country==TONE_GERMANY)
				rcm_tapi_SetPlayTone(lc->chid, ssid == 0 ? 1 : 0, DSPCODEC_TONE_STUTTERDIAL, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
			else
			rcm_tapi_SetPlayTone(lc->chid, ssid == 0 ? 1 : 0, DSPCODEC_TONE_DIAL, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
		}
#if 1 // 3pty on server
		}
#endif

		break;
	case LCStateResume:

		//call->state = LCStateAVRunning;
		SetCallState(  chid , ssid ,&call->state, LCStateAVRunning);
		g_message("\r\nlinphone linphone_call_accepted chid[%d],ssid[%d], status %d\n",chid,ssid,call->state );
		SetActiveSession(chid, ssid, TRUE);
//		rcm_tapi_SetRtpSessionState(chid, ssid, rtp_session_sendrecv);
		rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);

		/*merge from claro: some sip server un-held 200 ok with sdp , need read again */
		rcm_sdp_context_read_answer_by_ev(lc->call[ssid]->sdpctx, ev);
		linphone_core_stop_media_streams(lc, ssid);

	#ifdef CHECK_RESOURCE
		if (linphone_core_start_media_streams(lc, ssid, call) == -1)
		{
			linphone_core_terminate_dialog(lc, ssid, NULL);
			return -1;
		}
	#else
		linphone_core_start_media_streams(lc, ssid, call);
	#endif
		if (GetSysState(chid) == SYS_STATE_IN3WAY)
		{
			int i;
			TstVoipMgr3WayCfg stVoipMgr3WayCfg;

			memset(&stVoipMgr3WayCfg, 0, sizeof(stVoipMgr3WayCfg));
			stVoipMgr3WayCfg.ch_id = chid;
			stVoipMgr3WayCfg.enable = TRUE;
			for (i=0; i<MAX_SS; i++)
			{
				GetRtpCfg(&stVoipMgr3WayCfg.rtp_cfg[i], lc, i);
			}
			rcm_tapi_SetConference(&stVoipMgr3WayCfg);
		}

		if (GetSysState(chid)==SYS_STATE_MCID)
		{
			SetActiveSession(chid, ssid, TRUE);
			linphone_core_start_media_streams(lc, ssid, call);
			SetCallState(  chid , ssid ,&call->state, LCStateBeResume);
			if(linphone_core_offhood_event(lc,ssid)==0)
			{
				SetSysState(lc->chid, SYS_STATE_CONNECT);
			}else
			{
				g_message("\r\n offhold ssid %d fail\n",ssid);
				return -1;
			}
		}
		break;
	case LCStateHoldACK:
		// shouldn't occur
		printf("receive OK on HoldACK!?\n");
		break;
	case LCStateAVRunning:
		// shouldn't occur
		printf("receive OK on AVRnning...\n");
		rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
		
		if(ev->type!=EXOSIP_CALL_RINGING){
			SetSessionState( chid , ssid , SS_STATE_CALLER ) ;
		}		
		if(no_sdp==0){
				//if no sdp, not need restart media 
		rcm_sdp_context_read_answer_by_ev(lc->call[ssid]->sdpctx, ev);
		linphone_core_stop_media_streams(lc, ssid);
	#ifdef CHECK_RESOURCE
		if (linphone_core_start_media_streams(lc, ssid, call) == -1)
		{
			linphone_core_terminate_dialog(lc, ssid, NULL);
			return -1;
		}
	#else
		linphone_core_start_media_streams(lc, ssid, call);
	#endif
		}else if ((no_sdp==1)&&(lc->call[ssid]->p_early_media_stop_rtp==1)){
			g_message("180 stop rtp and 200 ok no sdp restart rtp\n");
#ifdef CHECK_RESOURCE
			if (linphone_core_start_media_streams(lc, ssid, call) == -1)
			{
				linphone_core_terminate_dialog(lc, ssid, NULL);
				return -1;
			}
#else
			linphone_core_start_media_streams(lc, ssid, call);
#endif
		}


/*bugfix:
when make outgoing call to B , 
and receive incoming call from C can't run 3way conf.
*/
			g_message("get status is %d\n",GetSysState(chid));
			switch (GetSysState(chid))
			{
			case SYS_STATE_CONNECT_EDIT:
				if(ev->type!=EXOSIP_CALL_RINGING){
					SetSysState(chid, SYS_STATE_IN2CALLS_OUT);
				}
				break;
#ifdef ATTENDED_TRANSFER
			case SYS_STATE_TRANSFER_EDIT:
#ifdef PLANET_TRANSFER
			case SYS_STATE_TRANSFER_ATTENDED:  //go through
#endif /*PLANET_TRANSFER*/
			case SYS_STATE_TRANSFER_CONNECT:

				SetSysState(chid, SYS_STATE_TRANSFER_CONNECT);
				break;
#endif
			default:

				SetSysState(chid, SYS_STATE_CONNECT);
				break;
			}


	
		break;
	case LCStateInit: // if no ringing
	case LCStateRinging:

		g_message("\r\n case LCStateInit or LCStateRinging \n");

		SetActiveSession( chid , ssid , TRUE ) ;
		rcm_sdp_context_read_answer_by_ev(lc->call[ssid]->sdpctx, ev);
		if(ev->type!=EXOSIP_CALL_RINGING){
		linphonec_display_something(lc, "Connected.\n");
		}

		if( lc->ringstream[ssid] )
		{
			lc->ringstream[ssid] = FALSE ;
			rcm_tapi_SetRingFXS( chid , FALSE ) ;
			rcm_tapi_SetPlayTone( chid , ssid , DSPCODEC_TONE_RINGING , FALSE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
		}

		g_message("get status is %d\n",GetSysState(chid));
		switch (GetSysState(chid))
		{
		case SYS_STATE_CONNECT_EDIT:
			if(ev->type!=EXOSIP_CALL_RINGING){
			SetSysState(chid, SYS_STATE_IN2CALLS_OUT);
			}
			break;
#if 1// 3pty on server			
		case SYS_STATE_IN3WAY:
		case SYS_STATE_IN3WAY_ONGOING:
		case SYS_STATE_IN3WAY_ONGOING_2:
		case SYS_STATE_IN3WAY_ONGOING_3:
		case SYS_STATE_IN3WAY_ONGOING_4:
			break;
#endif				
#ifdef ATTENDED_TRANSFER
		case SYS_STATE_TRANSFER_EDIT:
#ifdef PLANET_TRANSFER
		case SYS_STATE_TRANSFER_ATTENDED:  //go through
		case SYS_STATE_TRANSFER_CONNECT:
#endif /*PLANET_TRANSFER*/
#if 0
			SetSysState(chid, SYS_STATE_IN2CALLS_OUT);
#else
			SetSysState(chid, SYS_STATE_TRANSFER_CONNECT);
#endif
			break;
#endif
		default:
			SetSysState(chid, SYS_STATE_CONNECT);
			break;
		}

		
		if(ev->type!=EXOSIP_CALL_RINGING){
		SetSessionState( chid , ssid , SS_STATE_CALLER ) ;
		}
		if(no_sdp==0){
		linphone_core_stop_media_streams(lc, ssid);
	#ifdef CHECK_RESOURCE
		if (linphone_core_start_media_streams(lc, ssid, call) == -1)
		{
			linphone_core_terminate_dialog(lc, ssid, NULL);
			return -1;
		}
	#else
		linphone_core_start_media_streams(lc, ssid, call);
	#endif
		}
		break;
	case LCStateFax: // fax re-invite
		rcm_sdp_context_read_answer_by_ev(lc->call[ssid]->sdpctx, ev);
		linphone_core_stop_media_streams(lc, ssid);
	#ifdef CHECK_RESOURCE
		if (linphone_core_start_media_streams(lc, ssid, call) == -1)
		{
			linphone_core_terminate_dialog(lc, ssid, NULL);
			return -1;
		}
	#else
		linphone_core_start_media_streams(lc, ssid, call);
	#endif
		break;
	case LCStateUpdate: // update
		if(no_sdp==0){
		rcm_sdp_context_read_answer_by_ev(lc->call[ssid]->sdpctx, ev);
	#ifdef CHECK_RESOURCE
		if (linphone_core_start_media_streams(lc, ssid, call) == -1)
		{
			linphone_core_terminate_dialog(lc, ssid, NULL);
			return -1;
		}
	#else
		linphone_core_start_media_streams(lc, ssid, call);
	#endif
		}
		break;
	default:
		// do what?
		break;
	}

	return 0 ;
}
int linphone_call_terminated_3way_on_server(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev)
{
	//int err;
	lc->ringstream[ssid] = FALSE ;

	//[SD6, ericchung, exosip 3.5 integration
	eXosip_lock();
	eXosip_call_terminate(ev->cid, ev->did);
	eXosip_unlock();
	//]

	if(ssid==2)
		linphone_core_stop_media_streams(lc, 0);
	else if (ssid==1)
		linphone_core_stop_media_streams(lc, ssid);
	
	linphonec_display_something(lc, end);
	linphone_call_destroy(lc->call[ssid]);
	lc->call[ssid] = NULL ;

	return 0 ;
}

int linphone_call_terminated(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev)
{
	//int err;
	lc->ringstream[ssid] = FALSE ;

	set_autocall_state(lc,YUEME_TEST_FINISH);
	
	// avoid t38 not close problem
	if (lc->call[ssid]->faxflag == FMS_FAX_RUN &&
		lc->call[ssid]->t38_params.initialized)
	{
		int i;
		int fax_flag;

		for (i=0; i<10; i++)
		{
			rtk_GetFaxEndDetect(lc->chid, &fax_flag);
			if (fax_flag)
			{
				// Flush FIFO
				rtk_GetFaxModemEvent(lc->chid, &fax_flag, NULL, 1);
				lc->call[ssid]->faxflag = FMS_IDLE;
				printf("SIP go FAX_IDLE in call close\n");
				break;
			}
			usleep(100000); // 100ms
		}
	}

	//[SD6, ericchung, exosip 3.5 integration
	eXosip_lock();
	eXosip_call_terminate(ev->cid, ev->did);
	eXosip_unlock();
	//]
	linphone_core_stop_media_streams(lc, ssid);
	linphonec_display_something(lc, end);
	linphone_call_destroy(lc->call[ssid]);
	lc->call[ssid] = NULL ;

	return 0 ;
}


int linphone_call_released(LinphoneCore *lc, guint32 ssid, int cid)
{
	LinphoneCall * call = lc->call[ssid];

	if( call != NULL && call->cid == cid )
	{
		linphone_call_destroy(lc->call[ssid]);
		lc->call[ssid] = NULL;
		linphonec_display_warning(lc, _("Could not reach destination.\n"));
		voip_line_state_share_id_set(lc->chid, OMCI_SHARE_SIPCIPM_FAILEDCONNECTCOUNTER,NULL);
#if 1
        // rock: transport error
        rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
		/* start timer to play wrong tone , if user forget offhook phone */
		rcm_timerLaunch(lc->offhook_timer,((g_pVoIPCfg->BusyToneTimer)*1000)); //play wrong tone
		rcm_timerLaunch(lc->stop_alltone_timer,((g_pVoIPCfg->BusyToneTimer+g_pVoIPCfg->HangingReminderToneTimer)*1000)); 
		
		SetSessionState(lc->chid, ssid, SS_STATE_IDLE);
		switch (GetSysState(lc->chid))
		{
		case SYS_STATE_EDIT:
		case SYS_STATE_CONNECT_EDIT:
		case SYS_STATE_TRANSFER_EDIT:
#ifdef PLANET_TRANSFER
		case SYS_STATE_TRANSFER_ATTENDED:
#endif /*PLANET_TRANSFER*/
			SetSysState(lc->chid, GetSysState(lc->chid) + 1);
			break;
		default:
			// do nothing
			break;
		}
#endif
	}

	return 0 ;
}
void update_omci_call_fail_counter(int chid, int code){
	switch( code )
	{
		case -110 ://time out
			voip_line_state_share_id_set(chid, OMCI_SHARE_SIPCIPM_TIMEOUTCOUNTER,NULL);
			break;
		default://SIP UA received a failure error code
			voip_line_state_share_id_set(chid, OMCI_SHARE_SIPCIPM_FAILURERECEIVEDCOUNTER,NULL);
			break;
	}
}
int linphone_call_failure(LinphoneCore *lc, guint32 ssid, int code, eXosip_event_t *ev)
{
	const char * reason ;
	char * msg486 = _( "User is busy." ) ;
	char * msg480 = _( "User is temporarily unavailable." ) ;
	char * msg487 = _( "Request Cancelled." ) ;
	/*++added by Jack Chan 13/02/07 for T.38++*/
	char * msg488=_( "Not Acceptable Here" );
	/*--end--*/
	char * msg600 = _( "User does not want to be disturbed." ) ;
	char * msg603 = _( "Call declined." ) ;
	char * tmpmsg = msg486 ;
	int bTransferring, hs; // hs: hold session

	if (code!=401&&code!=407)
	set_autocall_state(lc,YUEME_TEST_CALL_FAIL);
	update_omci_call_fail_counter(lc->chid,code);

	hs = 0;
	bTransferring = 0;
	g_message("linphone_call_failure code is %d,ssid %d\n",code,ssid);
	if (GetSysState(lc->chid) == SYS_STATE_TRANSFERRING)
	{
		hs = (ssid == 0) ? 1 : 0;
		if (lc->call[hs] && lc->call[hs]->state != LCStateInit)
			bTransferring = 1;
	}

	if (code>200 && code<603)//for omci counts the number of calls that were terminated with cause
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_CCPM_TERMINATEFAILURES,NULL);
	
#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
	 tr104_Call_Update_Fail_Reason(lc->chid, code);
#endif
	switch( code )
	{
		case 422: /* add 422 session timer to small support */
			linphonec_display_something(lc, _("422 session to small,retry"));
			return 0; //exosip will retry it.
			break;
		case 401 :
		case 407 :
			if (linphone_call_authentication_required(lc, lc->call[ssid],ev) == 0)
				return 0; // call retry
			break ;
		case 400 :
			linphonec_display_something(lc, _("Bad request"));
			if (bTransferring)
				//[SD6, bohungwu, exosip 3.5 integration
				//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 400 Bad Request");
				rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 400 Bad Request");
				//]	
			break ;
		case 404 :
			linphonec_display_something(lc, _("User cannot be found at given address.\n"));

			
			if (bTransferring)
				//[SD6, bohungwu, exosip 3.5 integration
				//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 404 Not Found");
				rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 404 Not Found");
				//]
			break ;
		case 415 :
			linphonec_display_something(lc, _("Remote user cannot support any of proposed codecs."));
			if (bTransferring)
				//[SD6, bohungwu, exosip 3.5 integration
				//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 415 Unsupported Media Type");
				rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 415 Unsupported Media Type");
				//]
			break ;
		case 480 :
			tmpmsg = msg480 ;
		case 486 :
			linphonec_display_something(lc, tmpmsg);
			if (bTransferring)
				//[SD6, bohungwu, exosip 3.5 integration
				//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 486 Busy Here");
				rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 486 Busy Here");
				//]
			break ;
		case 487 :
			linphonec_display_something(lc, msg487);
			if (bTransferring)
				//[SD6, bohungwu, exosip 3.5 integration
				//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 487 Request Cancelled");
				rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 487 Request Cancelled");
				//]
			break ;	
		/*++added by Jack Chan 13/02/07 for T.38++*/
		case 488 :
			linphonec_display_something(lc, msg488);
			if (ssid == -1 || lc->call[ssid] == NULL)
				break;

			g_message("+++++DEBUG:lc flag is %d, call->state is %d +++++\n", 
				lc->call[ssid]->faxflag,
				lc->call[ssid]->state);

			// t.38 failed handling => change to t.30
			if (lc->call[ssid]->faxflag == FMS_FAX_RUN &&	// check fax or modem 
				lc->call[ssid]->state == LCStateFax && 
				lc->call[ssid]->t38_params.initialized)
			{
				g_message("\r\n *** linphone_call_failure , 488 FAX must implement \n");

				lc->call[ssid]->sdpctx->version++;
				lc->call[ssid]->t38_params.initialized = 0;		//reset the t38 parameters
				rcm_linphone_eXosip_on_fax(lc,ssid,0,FMS_FAX_RUN);

				return 0;
			}
			break;
		/*--end--*/
		case 600 :
			linphonec_display_something(lc, msg600);
			break ;
		case 603 :
			linphonec_display_something(lc, msg603);
			break ;
		case -110 :	/* time out, call leg is lost */
			linphonec_display_something(lc, _("Timeout."));
			break ;
		case -111 :
			linphonec_display_something(lc, _("Remote host was found but refused connection."));
			break ;
		default :
			if( code > 0 )
			{
				reason = osip_message_get_reason( code ) ;
				linphonec_display_something(lc, (char *) reason);
			}
			else
				g_warning( "failure_cb unknown code=%i\n" , code ) ;
	}

	switch (GetSysState(lc->chid))
	{
	case SYS_STATE_CONNECT:
		lc->callsDroppedCount++; //update callsDroppedCount : Total calls that were successfully connected (incoming or outgoing), but dropped unexpectedly while in progress without explicit user termination.
		break;
	case SYS_STATE_CONNECT_EDIT:
		if (lc->call[ssid] && lc->call[ssid]->state == LCStateHold)
		{
			int ssid2;

			// hold failed, recover it
			ssid2 = ssid == 0 ?  1: 0;
			switch (GetSessionState(lc->chid, ssid2))
			{
			case SS_STATE_IDLE:
				rcm_tapi_SetPlayTone(lc->chid, ssid == 0 ? 1 : 0, DSPCODEC_TONE_DIAL, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
				SetSysState(lc->chid, SYS_STATE_CONNECT);
				SetActiveSession(lc->chid, ssid, TRUE);
				//lc->call[ssid]->state = LCStateAVRunning;
				SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateAVRunning);
				g_message("\r\nlinphone linphone_call_failure chid[%d],ssid[ssid], status %d\n",lc->chid,ssid,lc->call[ssid]->state );
			
				return 0;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	if( lc->ringstream[ssid])
		lc->ringstream[ssid] = FALSE ;

	rcm_tapi_SetRingFXS(lc->chid, FALSE);


		/* play busy tone */
		rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
		/* start timer to play wrong tone , if user forget offhook phone */
		rcm_timerLaunch(lc->offhook_timer,((g_pVoIPCfg->BusyToneTimer)*1000)); //play wrong tone
		rcm_timerLaunch(lc->stop_alltone_timer,((g_pVoIPCfg->BusyToneTimer+g_pVoIPCfg->HangingReminderToneTimer)*1000)); 



	SetSessionState(lc->chid, ssid, SS_STATE_IDLE);

	// rock: stop if early media failed
	linphone_core_stop_media_streams(lc, ssid); 



	if( lc->call[ssid] != NULL )
		linphone_call_destroy( lc->call[ssid]);
	lc->call[ssid] = NULL ;

	if (GetActiveSession(lc->chid) == -1)
	{
		SetSysState(lc->chid, SYS_STATE_IDLE);
	}
	else if (bTransferring)
	{
		SetActiveSession(lc->chid, ssid == 0 ? 1 : 0, TRUE);
		SetSysState(lc->chid, SYS_STATE_CONNECT);
	}
	else
	{
		switch (GetSysState(lc->chid))
		{
		case SYS_STATE_CONNECT:
			// rock: if early media failed
			SetSysState(lc->chid, SYS_STATE_EDIT_ERROR);
			break;
		case SYS_STATE_EDIT:
		case SYS_STATE_CONNECT_EDIT:
		case SYS_STATE_TRANSFER_EDIT:
#ifdef PLANET_TRANSFER
		case SYS_STATE_TRANSFER_ATTENDED:
#endif /*PLANET_TRANSFER*/
			SetSysState(lc->chid, GetSysState(lc->chid) + 1);
			break;
		default:
			// do nothing
			break;
		}
	}

	return 0 ;
}

/*
record the register datetime and linux uptime 
*/
#define GMT_8 (8*60*60)
void rcm_recroad_register_time(LinphoneCore *lc,char *date_str)
{
	struct sysinfo info;
	struct tm reg_tm;

	char str_date[16];
	time_t timer = time(0);
	sysinfo(&info);
	//printf("Uptime = %ld\n", info.uptime);
	lc->reg_system_uptime=info.uptime;
	strptime(date_str, "%a, %d %b %Y %H:%M:%S", &reg_tm);

	if (strstr(date_str, "GMT")!=NULL)
		lc->register_datetime=mktime(&reg_tm)+GMT_8;
	else
		lc->register_datetime=mktime(&reg_tm);
	timer=lc->register_datetime;
	strftime(str_date, 9, "%m%d%H%M", localtime(&timer));
	g_message("%s %s\n",__FUNCTION__, str_date);
	//printf("register_datetime = %ld\n", lc->register_datetime);

}

/*
 send fsk date to phone (no ring)
 must use rtk_GenFskVmwi() send
 this is for e8c project.

*/
void rcm_send_fsk_date(int chid,char *date_str){
	char* allMonths[]= {"Jan", "Feb", "Mar", "Apr","May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char name[10];
	char day[2];
	int  month=0;
	char month_str[4];
	char name3[5];
	int d1,d2,d3;
	TstFskClid clid;
	int i;
	clid.ch_id = 0;
    clid.service_type = 0; 					//service type 1
    
	clid.cid_data[0].type = 0x01;			//DATE and Time
 
	//string format is  like "Sun, 05 Jun 2016 07:02:38 GMT"
	
	g_message("rcm_send_fsk_date date_str %s\n",date_str);
	sscanf(date_str,"%s %s %s %s %d:%d:%d ",name,day,month_str,name3,&d1,&d2,&d3);

	for(i = 0; i<12; i++){
       if (strcasecmp(month_str,allMonths[i])==0)
        {
           month=i;
		   break;
        }
    }
   
	sprintf(clid.cid_data[0].data,"%02d%02d%02d%02d",month+1,atoi(day),d1,d2);
 
//	strcpy(clid.cid_data[0].data, "05060708");
      	
	//Only 1 elements for VMWI data. Set other element to 0 (MUST)

	clid.cid_data[1].type = 0;	
    clid.cid_data[2].type = 0;
    clid.cid_data[3].type = 0;
    clid.cid_data[4].type = 0;
			
	rtk_GenFskVmwi(chid, &clid, 1); //only one elements
}

#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
void linphone_generate_cid_ring(void *arguments)
{
    struct timeval timer;
    arg_struct_t *p_args = (arg_struct_t *)arguments;
    unsigned long diff;
    uint32 tmp = 0;

    g_message("%s: chid:%d, call_id:%s, usec:%d, dtmf_mode:%d\n", __func__, p_args->chid, p_args->call_id, p_args->usec, p_args->dtmf_mode);

    /* ring control */
    if (p_args->dtmf_mode == 1 /* FSK */)
    {
        if (caller_id_mode[p_args->chid] & 0x020)  //short ring before caller id
        {
            g_message("%s: short ring before caller id.\n", __func__);

            if (GetSysState(p_args->chid) != SYS_STATE_RING)
                return;

            /* ring on 0.5 sec */
            rtk_SetRingFxs(p_args->chid, 1);
            timer.tv_usec = 500000;
            timer.tv_sec = 0;
            select(0, NULL, NULL, NULL, &timer);

            /* ring off 0.5 sec */
            rtk_SetRingFxs(p_args->chid, 0);
            timer.tv_usec = 500000;
            timer.tv_sec = 0;
            select(0, NULL, NULL, NULL, &timer);
        }

        if (caller_id_mode[p_args->chid] & 0x8)  //priori 1st ring
        {
            g_message("%s: caller ID before 1st ring.\n", __func__);
        }
        else
        {
            g_message("%s: caller ID after 1st ring.\n", __func__);
            if (GetSysState(p_args->chid) != SYS_STATE_RING)
                return;

            /* ring on decided timer parameter (2 sec at present) */
            rtk_SetRingFxs(p_args->chid, 1);
            timer.tv_usec = p_args->usec % 1000000;
            timer.tv_sec = p_args->usec / 1000000;
            select(0, NULL, NULL, NULL, &timer);
            rtk_SetRingFxs(p_args->chid, 0);
        }
    }

    /* if the state is not SYS_STATE_RING, don't need to generate caller ID */
    if (GetSysState(p_args->chid) != SYS_STATE_RING)
        return;

    rtk_SetFxsOnHookTransPcmOn(p_args->chid);

    if (p_args->dtmf_mode == 0) {
        // gen DTMF CID
        rtk_GenDtmfCid(p_args->chid, p_args->call_id);
    }
    else {
        // gen FSK CID
        if(strcmp(p_args->call_id, "P")==0)
        {
            g_message("caller_id_send_P\n");
            caller_id_send_P(p_args->chid);
        }
        else
        {
            if((g_pVoIPCfg->rfc_flags & SIP_DATE) && (p_args->date != NULL))
                rtk_GenFskCid(p_args->chid, p_args->call_id, p_args->date, p_args->name, 0/*FSK Type 1*/);
            else
                rtk_GenFskCid(p_args->chid, p_args->call_id, NULL, p_args->name, 0/*FSK Type 1*/);

            /* check if the FSK CID complete */
            do
			{
                if(rtk_GetFskCidState(p_args->chid, &tmp) != 0)
					break;
				usleep(50000);  // 50ms
			}
			while (tmp);
        }
    }

    /* if state is still SYS_STATE_RING, play ring */
    if (GetSysState(p_args->chid) == SYS_STATE_RING)
        rtk_SetRingFxs(p_args->chid, 1);

    return;
}

void linphone_generate_cid(uint32 chid, uint32 dtmf_mode, char *call_id, char *name, char *date, time_t usec)
{
    int ret;
    pthread_t thread_id;

    memset(args[chid].call_id, 0, sizeof(args[chid].call_id));
    memset(args[chid].name, 0, sizeof(args[chid].name));
    memset(args[chid].date, 0, sizeof(args[chid].date));

    args[chid].chid = chid;
    args[chid].usec = usec;
    args[chid].dtmf_mode = dtmf_mode;
    if (call_id != NULL)
        strcpy(args[chid].call_id, call_id);
    if (name != NULL)
        strcpy(args[chid].name, name);
    if (date != NULL)
        strcpy(args[chid].date, date);

    ret = pthread_create(&thread_id, NULL, (void *)&linphone_generate_cid_ring, (void *)&args[chid]);
    if (ret != 0)
        g_message("%s: error creating pthread!\n", __func__);
}
#endif

int linphone_inc_new_call(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev)
{
	char * barmesg;
	int status = 200 ;
	osip_message_t *answermsg;
	sdp_context_t * ctx = NULL ;
	osip_from_t * from_url = NULL ;
	osip_header_t *history_info=NULL;
	osip_header_t *diversion=NULL;
	int cid = ev->cid ;
	int did = ev->did ;
	//[SD6, bohungwu, exoisp 3.5 integration
	int tid = ev->tid ;
	//char * from = ev->remote_uri ;
	char * from=NULL;
	sdp_message_t *sdp_msg;
	//]
	int i=0;

	uint32 chid = lc->chid;
	char *displayname;
	//[SD6, bohungwu, exoisp 3.5 integration
	//char *to = ev->local_uri;
	char *to=NULL;
	//]
	osip_to_t *to_url = NULL;
	voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[lc->chid];
	char one_stage_dial_number[FW_LEN];
	LinphoneProxyConfig *used_proxy;
	/* call transfer */
	int match_cid=-1;
	osip_replaces_t *replaces=NULL;
	char *replaces_str=NULL;
	guint32 another_ssid;
	int pos = 0;
	osip_header_t *_header = NULL;	
	char * from_username =NULL;
	char * to_username =NULL;
	osip_header_t *caller_id_header=NULL;
	int alerting=0;
	char identity[64]= {'\0'};
	char *tmp=NULL;
	char *tmp_end=NULL;
	time_t timer = time(0);
	struct sysinfo info;//system uptime
	char fxs_str_date[16];
	char *str_date=NULL;

	//[SD6, bohungwu, eXosip 3.5 integration
	//osip_from_init( &from_url ) ;
	//osip_from_parse( from_url , from ) ;

	//osip_to_init(&to_url);
	//osip_to_parse(to_url, to);

	g_message("linphone_inc_new_call \n");
	memset(fxs_str_date, 0, sizeof(fxs_str_date));
	str_date=fxs_str_date;

	from_url=osip_message_get_from(ev->request);
	to_url=osip_message_get_to(ev->request);
	osip_from_to_str(from_url,&from);
	osip_from_to_str(to_url,&to);
	g_message("linphone_inc_new_call to_url is %s \n",to);
	g_message("linphone_inc_new_call from_url is %s \n",from);

	//alerting message dont need enable prack
	if (strstr(from, "ats")!=NULL)
	{
		g_message("alerting message\n");
		alerting=1;
	}
	
	/* to support Call transfer */

	//check new invite replaces head

	replaces=osip_message_get_replaces(ev->request);
	osip_replaces_to_str(replaces,&replaces_str);

	if(replaces)
	{
		g_message("linphone_inc_new_call replaces=%s\n",replaces_str);
		match_cid=eXosip_call_find_by_replaces(replaces_str);
		g_message("linphone_inc_new_call match_cid=%d\n",match_cid);
		int hs;
		if (match_cid>=0)//match 
		{
			hs = (ssid == 0) ? 1 : 0;
			linphone_core_terminate_dialog(lc, hs, NULL);
		}else{//if not match any call , need respones ?
			g_message("linphone_inc_new_call replace not match any call\n");
		}

			
	}

	//Retrieve SDP data structure and conver it to a string as requested by Linphone
	sdp_msg = eXosip_get_sdp_info(ev->request);
	
	if(sdp_msg == NULL) {
		g_message( "%s:sdp_msg is null\n", __FUNCTION__);
	}

#if 0
	/* first check if we can answer successfully to this invite */
	if( lc->presence_mode[chid] != LINPHONE_STATUS_ONLINE )
	{
		g_message( "Not present !! presence mode[%d] : %d!!\n" , chid , lc->presence_mode[chid] ) ;
		eXosip_lock() ;

		if( lc->presence_mode[chid] == LINPHONE_STATUS_BUSY )
		{
			g_warning( "The call[%d][%d] is BUSY now !!" , chid , ssid ) ;
			eXosip_answer_call( did , 486 , NULL ) ;
		}
		else if( lc->presence_mode[chid] == LINPHONE_STATUS_AWAY ||
				lc->presence_mode[chid] == LINPHONE_STATUS_BERIGHTBACK ||
				lc->presence_mode[chid] == LINPHONE_STATUS_ONTHEPHONE ||
				lc->presence_mode[chid] == LINPHONE_STATUS_OUTTOLUNCH ||
				lc->presence_mode[chid] == LINPHONE_STATUS_OFFLINE ||
				lc->presence_mode[chid] == LINPHONE_STATUS_NOT_DISTURB )
		{
			g_warning( "Temporarily Unavailable [%d]!!" , chid ) ;
			eXosip_answer_call( did , 480 , NULL ) ;
		}
		else if( lc->alt_contact != NULL && lc->presence_mode[chid] == LINPHONE_STATUS_MOVED )
		{
			g_warning( "Moved Temporarily [%d][%d]!!" , chid , ssid ) ;
			eXosip_set_redirection_address( did , lc->alt_contact ) ;
			eXosip_answer_call( did , 302 , NULL ) ;
			eXosip_set_redirection_address( did , NULL ) ;
		}
		else if( lc->alt_contact != NULL && lc->presence_mode[chid] == LINPHONE_STATUS_ALT_SERVICE )
		{
			g_warning( "Alternative Service [%d][%d]!!" , chid , ssid ) ;
			eXosip_set_redirection_address( did , lc->alt_contact ) ;
			eXosip_answer_call( did , 380 , NULL ) ;
			eXosip_set_redirection_address( did , NULL ) ;
		}
		else
		{
			g_warning( "Busy Here [%d]!!" , chid  ) ;
			eXosip_answer_call( did , 486 , NULL ) ;
		}

		eXosip_unlock() ;
		goto end ;
	}
#endif


	/* check busy */
	if (lc->call[ssid] != NULL)
	{
		g_warning("The call[%d] is BUSY now !!\n", chid);
		eXosip_lock() ;
		//[SD6, bohungwu, exosip 3.5 integration
		//eXosip_answer_call( did , 486 , NULL ) ;
		eXosip_call_send_answer( tid , 486 , NULL ) ;
		//]
		eXosip_unlock() ;
		goto end ;
	}

	/* global error */
	//eric: support delay offer , if no sdp , use 180ring +sdp(offer) send to offer
#if 0 
	if(sdp_msg == NULL)
	{
		g_warning("No sdp body [%d]!!\n", chid);
		eXosip_lock() ;
		//[SD6, bohungwu, exosip 3.5 integration
		//eXosip_answer_call( did , 603 , NULL ) ;
		eXosip_call_send_answer( tid , 603 , NULL ) ;
		//]
		eXosip_unlock() ;
		goto end ;
	}
#endif	

	used_proxy = NULL;
	
#ifdef MULTI_PROXY
	// check request come from which proxy
	if (&to_url->url.host[0] != '\0')
	{
		for (i=0; i<MAX_PROXY; i++)
		{
			// TODO: consider DNS ?
			if (lc->proxies[i] &&
				strcmp(to_url->url.host, &lc->proxies[i]->reg_proxy[4]) == 0)
			{
				// record used proxy in call
				g_message("record used proxy in call index %d\n",i);
				used_proxy = lc->proxies[i];
				break;
			}
		}
	}
#else
	if (lc->default_proxy &&
		strcmp(to_url->url->host, &lc->default_proxy->reg_proxy[4]) == 0)
	{
		used_proxy = lc->default_proxy;
	}
#endif

	// rock: check VoIP to PSTN
	//one_stage_dial_number[0] = 0;

	//[SD6, bohungwu, eXosip 3.5 integration
	//lc->call[ssid] = linphone_call_new_incoming(lc, ssid, from, ev->local_uri, cid, did);
	lc->call[ssid] = linphone_call_new_incoming(lc, ssid, from, to, from_url,to_url,cid, did);
	//]
	if (lc->call[ssid] == NULL)
	{
		g_warning("%s: create call failed\n", __FUNCTION__);
		eXosip_lock();
		//[SD6, bohungwu, exosip 3.5 integration
		//eXosip_answer_call(did, 500, NULL);
		eXosip_call_send_answer(tid, 500, NULL);
		//]
		eXosip_unlock();
		goto end ;
	}

	
	/*support delay offer */	
	if(sdp_msg == NULL)
	{
		lc->call[ssid]->delay_offer_enable=1;
	}else{
		lc->call[ssid]->delay_offer_enable=0;
	}
	
#ifdef MULTI_PROXY
	if(used_proxy == NULL){
		printf("used_proxy is null\n");
		lc->call[ssid]->proxy =lc->default_proxy;
	}else{
	lc->call[ssid]->proxy = used_proxy;
	}
#endif


	memset(caller_id,0,sizeof(caller_id));
	memset(caller_id_name,0,sizeof(caller_id_name));
	
	// username -> caller id
	if (&from_url->url.username[0] != '\0')
	{
		from_username=strchr(from,'@');
		if (from_username==NULL) //from not include '@'
		{
			if (strlen(from_url->url.host) >=3)
			{
				
				strncpy(caller_id, from_url->url.host, sizeof(caller_id) - 1);



				/* DTMF call id need all digit, but FSK. */
			    /* or private caller id (start with '-') */
				if ((lc->caller_id_mode&7) == CID_DTMF)
				{
					for (i=0; caller_id[i]; i++) 
						if (!isdigit(caller_id[i]))
						{
							caller_id[0]=0; /* reset illegal cid */
							break;
						}
				}
			}	
		}else{
			/* At least 3 digits */
			/* 2 digits may be a control signal in some cases */
			if (strlen(from_url->url.username) >=3)
			{
				strncpy(caller_id, from_url->url.username, sizeof(caller_id) - 1);

				caller_id[strlen(caller_id)] = 0;



				/* DTMF call id need all digit, but FSK. */
			    /* or private caller id (start with '-') */
				if ((lc->caller_id_mode&7) == CID_DTMF)
				{
					for (i=0; caller_id[i]; i++) 
						if (!isdigit(caller_id[i]))
						{
							caller_id[0]=0; /* reset illegal cid */
							break;
						}
				}
			}
		}
	}

	// displayname -> caller id name
	if (from_url->displayname)
	{
		// use display name as caller id name
		displayname = osip_strdup_without_quote(from_url->displayname);
		strncpy(caller_id_name, displayname, sizeof(caller_id_name) - 1);
		caller_id_name[sizeof(caller_id_name) - 1] = 0;
		osip_free(displayname);
	}

	/* if caller_id is empty */	
	/* use caller_id_name as caller id */
	if (caller_id[0] == 0)
	{
		/* At least 3 digits */
		/* 2 digits may be a control signal in some cases */
		if (strlen(caller_id_name)>=3)
		{
			strncpy(caller_id, caller_id_name, sizeof(caller_id) - 1);
			caller_id[strlen(caller_id)] = 0;

			/* DTMF call id need all digit, but FSK. */
		    /* or private caller id (start with '-') */
			if ((lc->caller_id_mode&7) == CID_DTMF)	 
			{
				for (i=0; caller_id[i]; i++) 
					if (!isdigit(caller_id[i]))
					{
						caller_id[0]=0; /* reset illegal cid */
						break;
					}
			}
		}

		// for direct ip call: from not inculde number, always display "0123456789"
		to_username=osip_uri_get_username(&ev->request->req_uri);
		//printf("\r\n to_username is %s\n",to_username);
		if((to_username==NULL)|| (strlen(to_username)==0)){
			if (!(voip_ptr->fxsport_flag & FXS_REJECT_DIRECT_IP_CALL))
			{			
				strcpy(caller_id, "0123456789");
				if (lc->parent->use_br0)
				{
					lc->call[ssid]->MP_IP_CALL=1;
				}
			}
		}		
		
		// if error use private caller id & name
		if (strlen(caller_id) == 0)
		{
			strcpy(caller_id, "P");
			strcpy(caller_id_name, "P");
		}
	}
	else if (strlen(caller_id_name) == 0)
	{
		// if error use private caller id name
		strcpy(caller_id_name, "P");
	}


	if(g_pVoIPCfg->rfc_flags & SIP_P_ASSERTED_IDENTITY){
		g_message("check P_Asserted_Identity header\n");
		get_callid_from_P_Asserted_Identity(ev, caller_id);
	}

	e8c_replace_caller_id( caller_id , "+", "");
	g_message("display_prefix_num=%d country_region_code=%s\n",g_pVoIPCfg->display_prefix_num,g_pVoIPCfg->country_region_code);
	
	if (g_pVoIPCfg->display_prefix_num== 0){
		e8c_replace_caller_id( caller_id , "86", "");
		e8c_replace_caller_id( caller_id , g_pVoIPCfg->country_region_code, "");
					}

	if (strcasecmp(caller_id,"Anonymous")==0)
	{
		g_message("caller_id Anonymous %s %d\n", __FUNCTION__, __LINE__);
		strcpy(caller_id, "P");
		strcpy(caller_id_name, "P");
	}else{
		for (i=0; caller_id[i]; i++){ 
			if (!isdigit(caller_id[i]))
			{
				// if error use private caller id & name
				strcpy(caller_id, "P");
				strcpy(caller_id_name, "P");
				g_message("if error use private caller id & name %s %d\n", __FUNCTION__, __LINE__);
				break;
			}
		}
	}

	


	g_message("caller_id=%s caller_id_name=%s\n",caller_id,caller_id_name);
	
	ctx = lc->call[ssid]->sdpctx ;
	/* get the result of the negociation */
	if (lc->call[ssid]->delay_offer_enable==0){		
	rcm_sdp_context_get_answer_by_ev( ctx , ev ) ;
	status = sdp_context_get_status( ctx ) ;
	if( GetSessionState( chid , ssid ) == SS_STATE_IDLE )
	{
		// avoid callee use hold function problem
		ctx->offer = sdp_context_generate_template( ctx ) ;
		// set version the same with current anwser
		sprintf(ctx->offer->o_sess_version, "%.10d", ctx->version);
	}

	}else{
		if (ctx->localip==NULL) {
			rcm_get_localip_for_voip(solar_ptr->current_voip_interface,&ctx->localip);
		}
		status=200;
	}
	
	eXosip_call_set_reference(did, (void *) lc->call[ssid]);

	lc->call[ssid]->cid=ev->cid;
	lc->call[ssid]->did=ev->did;
	lc->call[ssid]->tid=ev->tid;


	switch( status )
	{
		case 200 :
			{
				char * tmp ;
				from_2char_without_params( from_url , &tmp ) ;
				barmesg = g_strdup_printf( "%s is calling you.\n" , tmp) ;
				linphonec_display_something(lc, barmesg);
				g_free( barmesg ) ;
				osip_free( tmp ) ;
			}
			
			{
				eXosip_call_t *jc = NULL;
				eXosip_dialog_t *jd = NULL;

				// record dtmf mode for handling telephone-event in SDP
      			eXosip_call_dialog_find(did, &jc, &jd);
				if (jc && jc->c_ctx)
				{
					jc->c_ctx->dtmf_mode = lc->sip_conf.dtmf_type;
				}
			}
#ifdef ATTENDED_TRANSFER
			if (ev->rc != NULL||match_cid>=0)//match replace accept this invite, dont send 180, only respone 200 ok
			{
				SetSysState(chid, SYS_STATE_RING);
				SetSessionState(chid, ssid, SS_STATE_CALLIN);
				//linphone_call_set_state(lc->call[ssid], LCStateRinging);
				SetCallState(chid,ssid,&(lc->call[ssid]->state),LCStateRinging);
				linphone_core_accept_dialog(lc, ssid, NULL);
				goto end;
			}
#endif

			if (lc->parent->bAutoAnswer)
			{
				SetSysState(chid, SYS_STATE_RING);
				SetSessionState(chid, ssid, SS_STATE_CALLIN);
				//linphone_call_set_state(lc->call[ssid], LCStateRinging);
				SetCallState(chid,ssid,&(lc->call[ssid]->state),LCStateRinging);
				
				linphone_core_accept_dialog(lc, ssid, NULL);
				goto end;
			}

			/*normal case , incoming call return 180ring */
			eXosip_call_build_answer(tid,180,&answermsg);
			/* check history-info header */
			osip_message_get_history_info(ev->request,0,&history_info);
			if (history_info){
				osip_message_set_history_info(answermsg,history_info->hvalue);
			}
			/* check Diversion header */
			osip_message_get_diversion(ev->request,0,&diversion);
			if (diversion){
					osip_message_set_diversion(answermsg,diversion->hvalue);
			}
  			
			/*check incoming INVITE message support PRACK  */
			if(!(g_pVoIPCfg->rfc_flags & SIP_DISABLE_PRACK)){
				while(osip_message_get_supported(ev->request, pos, &_header)>=0){
					if((_header!=NULL)&&(_header->hvalue!=NULL)){
						g_message("supported header pos=%d _header->hvalue=[%s]\n",pos,_header->hvalue);
					 if (0==osip_strncasecmp(_header->hvalue, "100rel", 6)){
							/* if include support 100rel , and we enable prack , send 100rel */ 			
					if (alerting!=1){//alerting message dont need enable prack
				osip_message_set_require(answermsg,"100rel");
				osip_message_set_header(answermsg, "Rseq", "1"); //ericchung: add Rseq for PRACK 
								break;
							}
			}
				}						
					pos++;
			}
			}
		
			eXosip_lock() ;
			eXosip_call_send_answer( tid , 180 , answermsg ) ;
			eXosip_unlock() ;

			if(lc->no_Answer_timeout !=0)
			{
				g_message("launch no answer timer timer after %d sec",lc->no_Answer_timeout);
				rcm_timerLaunch (lc->no_Answer_timer_id, lc->no_Answer_timeout*1000);
			}	

			if (GetSysState(chid) == SYS_STATE_IDLE)
			{

                //prepare the datetmie string
				g_message("sync_phone_time=%d\n",g_pVoIPCfg->sync_phone_time);
				if (g_pVoIPCfg->sync_phone_time == 0){//0: auto					
					if(lc->register_datetime==0){
						//sip server not send datetime, use linux system time
						if(rcm_check_vsntp_success()==1){						
						strftime(str_date, 9, "%m%d%H%M", localtime(&timer));
							g_message("NTP ok use system time str_date is %s\n",str_date);
						}else{
							g_message("NTP not ready str_date =NULL\n");
                        str_date=NULL;
				}
					}else{
						//get now uptime time 
						sysinfo(&info);
						g_message("now Uptime = %ld\n", info.uptime);
						g_message("diff Uptime = %ld\n", info.uptime - lc->reg_system_uptime);						
						timer=lc->register_datetime + (info.uptime - lc->reg_system_uptime);
                    strftime(str_date, 9, "%m%d%H%M", localtime(&timer));
						g_message("use register server date str_date is %s\n",str_date);						
					}
				}
				else if (g_pVoIPCfg->sync_phone_time == 1){// 1: force
					if(lc->register_datetime==0){
						g_message("lc->register_datetime is 0\n");
						//sip server not send datetime, use linux system time
							strftime(str_date, 9, "%m%d%H%M", localtime(&timer));
						}else{
						//get now uptime time 
						sysinfo(&info);
						g_message("now Uptime = %ld\n", info.uptime);
						g_message("diff Uptime = %ld\n", info.uptime - lc->reg_system_uptime);						
						timer=lc->register_datetime + (info.uptime - lc->reg_system_uptime);
						strftime(str_date, 9, "%m%d%H%M", localtime(&timer));
						g_message("str_date is %s\n",str_date);						
						}
					g_message("str_date is %s\n",str_date);
					}
				else{// 2 : no date
					str_date=NULL;
				}


			
				// is 1st session
				SetSysState(chid, SYS_STATE_RING);

				//============= Caller ID Generation =========// //thlin+
				caller_id_mode[chid] = lc->caller_id_mode;

					#if defined(CONFIG_RTK_VOIP_TR104) && defined(CONFIG_E8B)
                if ((rcm_tr104_st.e8c_autotest[chid].enable==1) && (rcm_tr104_st.e8c_autotest[chid].TestType==1)) {
                    g_message("auto test don't send ring\n");
				} 
				else
#endif
				{
#if CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
                    time_t ring_duration = 2000000; /* 2 sec */

                    if ((caller_id_mode[chid]&7) == CID_DTMF)	 /* 0:Bellcore 1:ETSI 2:BT 3:NTT 4:DTMF */
						{
                        /* DTMF caller ID generation */
                        linphone_generate_cid(chid, 0, caller_id, NULL, NULL, ring_duration);
						}
						else
						{
                        /* FSK caller ID generation */
                        linphone_generate_cid(chid, 1, caller_id, caller_id_name, str_date, ring_duration);
						}
#else
                    //  for DTMF CID
                    if ((caller_id_mode[chid]&7) == CID_DTMF)	 /* 0:Bellcore 1:ETSI 2:BT 3:NTT 4:DTMF */
                    {
                        rcm_tapi_Gen_CID_And_FXS_Ring(chid, 0/*dtmf*/, caller_id, NULL, NULL, 0, 0);
                    }
                    else
                    {
                        //fsk send caller id, not need send caller name.
					if(strcmp(caller_id,"P")==0)
					{
						g_message("caller_id_send_P\n");
						caller_id_send_P(chid);
					}
					else
						{
						if((g_pVoIPCfg->rfc_flags&SIP_DATE) && (str_date != NULL))
								rcm_tapi_Gen_CID_And_FXS_Ring(chid, 1/*fsk*/, caller_id, str_date,  caller_id, 0/*type1*/, 0);
							else	
								rcm_tapi_Gen_CID_And_FXS_Ring(chid, 1/*fsk*/, caller_id, (void *) 0, caller_id, 0/*type1*/, 0);
						}
                    }
#endif
				}

			
#ifdef VMWI_TEST 	
				char vmwi_on = 1, vmwi_off = 0;	
				for(i=0; i<6000000; i++);
				
				/* Set VMWI message on in on_hook mode */
				rcm_tapi_Gen_FSK_VMWI( chid, &vmwi_on, 0);
				for(i=0; i<6000000; i++);
				
				/* Set VMWI message off in on_hook mode */
				rcm_tapi_Gen_FSK_VMWI( chid, &vmwi_off, 0);
				for(i=0; i<6000000; i++);
				rcm_tapi_Gen_FSK_VMWI( chid, &vmwi_off, 0);
#endif
			
				//==============================================//

			}
			else
			{
				// call waiting caller ID (FSK Type II)
				if (  (call_waiting_cid[chid])
				    &&((caller_id_mode[chid]&7) != CID_DTMF)  ){ //add for disable waiting id
					//rcm_tapi_Gen_FSK_CID( chid, caller_id, NULL, caller_id_name, 1);
					rcm_tapi_Gen_CID_And_FXS_Ring(chid, 1/*fsk*/, caller_id, NULL, caller_id_name, 1/*type2*/, 0);

				// wait fsk caller id done, then send call waiting tone
				int32 tmp=-1;
				do
				{
					if(rcm_tapi_GetFskCIDState(chid, &tmp) != 0)
						break;
					usleep(50000);  // 50ms
				}
				while (tmp);
				}


				// call waiting tone
				rcm_tapi_SetPlayTone( chid , ssid ? 0 : 1, DSPCODEC_TONE_CALL_WAITING , TRUE , DSPCODEC_TONEDIRECTION_LOCAL ) ;

			}

			/* play the ring */
			lc->ringstream[ssid] = TRUE;
			SetSessionState( chid , ssid , SS_STATE_CALLIN ) ;
//			if (lc->call[ssid == 0 ? 1 : 0] == NULL)
			if (GetSessionState(chid, ssid == 0 ? 1 : 0) == SS_STATE_IDLE) {
				StopAlarmRinging( lc );
				//rcm_tapi_SetRingFXS( chid , TRUE ) ;
			}


			SetCallState(chid,ssid,&(lc->call[ssid]->state),LCStateRinging);

			// add no answer check
			osip_gettimeofday(&lc->call[ssid]->no_answer_start, NULL);


			break ;
		default :	
			if( status == -1 )
				status = 415 ;
			g_warning( "Error during sdp negociation. status : %d\n" , status ) ;
			eXosip_lock() ;
			//[SD6, bohungwu, exosip 3.5 integration
			//eXosip_answer_call( did , status , NULL ) ;
			eXosip_call_send_answer(tid, status, NULL);
			//]
			eXosip_unlock() ;
			linphone_call_destroy(lc->call[ssid]);
			lc->call[ssid] = NULL;
			break;
	}

end :
g_message("\r\n linphone_inc_new call end !! \n");
	if(sdp_msg!=NULL)
	sdp_message_free(sdp_msg);

	osip_free(from);
	osip_free(to);

	return 0 ;
}

void linphone_do_automatic_redirect(LinphoneCore *lc, uint32 ssid, const char *contact)
{
	char * msg = g_strdup_printf( _( "Redirected to %s..." ) , contact ) ;

	linphonec_display_something(lc, msg);
	g_free( msg ) ;


	if (lc->call[ssid] != NULL)
		linphone_call_destroy(lc->call[ssid]);

	lc->call[ssid] = NULL;

		linphone_core_invite(lc, ssid, contact);
	
}

void linphone_call_redirected(LinphoneCore *lc, uint32 ssid, int cid, int code, const char *contact)
{
	switch( code )
	{
		case 380 :
			linphonec_display_url(lc, _("User is not reachable at the moment but he invites you\n" \
				"to contact him using the following alternate resource:"), (char *) contact);
			if (lc->call[ssid] != NULL)
			{
				linphone_call_destroy(lc->call[ssid]);
				lc->call[ssid] = NULL;
			}
			break ;
		case 302 :
			linphone_do_automatic_redirect(lc, ssid, contact);
			break ;
	}
}

#ifdef SUPPORT_G7111
char *rtk_g7111_modes_precedence_to_str(unsigned char *precedence)
{
	int i, idx;
	static char g7111_mode_str[256];

	g7111_mode_str[0] = 0;
	for (i=0, idx=0; i < G7111_MODES; i++)
	{
		if (precedence[i])
		{
			if (idx)
				idx += sprintf(g7111_mode_str + idx, ",%d", precedence[i]);
			else
				idx += sprintf(g7111_mode_str + idx, "mode-set=%d",
					precedence[i]);
		}
	}

	return g7111_mode_str;
}

int rtk_g7111_modes_str_to_precedence(char *modes_str, 
	unsigned char *desired_precedence, unsigned char *precedence)
{
    int i;
	char *modes;
	char *str1, *token, *saveptr1;
	char *str2, *subtoken, *saveptr2;
	unsigned char g7111_mode;

	memset(precedence, 0, sizeof(precedence[0]) * G7111_MODES);

	i = 0;
	if (modes_str)
	{
		for (str1 = modes_str; ; str1 = NULL)
		{
			token = strtok_r(str1, " ;", &saveptr1);
			if (token == NULL)
				break;

			modes = strstr(token, "mode-set=");
			if (modes == NULL)
				continue;

			modes += strlen("mode-set=");
			for (str2 = modes; i<G7111_MODES; str2 = NULL)
			{
				subtoken = strtok_r(str2, ",", &saveptr2);
				if (subtoken == NULL)
					break;

				g7111_mode = atoi(subtoken);
#if 0
				// check mode is support in local
				for (int j = 0; i<G7111_MODES && j<G7111_MODES; j++)
				{
					if (desired_precedence[j] == g7111_mode)
					{
						precedence[i++] = g7111_mode;
						break;
					}
				}
#else
				// follow remote always
				precedence[i++] = g7111_mode;
#endif
			}
		}
	}

	// use desired precedence if modes string is unspecified or unknown
	if (i == 0)
	{
		memcpy(precedence, desired_precedence,
			sizeof(precedence[0]) * G7111_MODES);
	}

	return 0;
}
#endif

//extern int enable_using_port( int chid ) ;

/* these are the SdpHandler callbacks: we are called in to be aware of the content
of the SDP messages exchanged */
/* outgoing call audio setting --> set_audio_codecs */
int linphone_set_audio_offer(sdp_context_t *ctx)
{
	LinphoneCall *call=(LinphoneCall*)sdp_context_get_user_pointer(ctx);
	LinphoneCore *lc=call->core;
	PayloadType *codec;
	GList *elem;
	sdp_payload_t payload;
	int i, s_id=0;
	int static_pt;

	for( i = 0; i < MAX_SS; i++){
		if( call == lc->call[i] ){
			s_id = i;
			break;
		}
	}

#if 0 // def SUPPORT_RTP_REDUNDANT_APP
	if( lc->sip_conf.rtp_redundant_payload_type ) 
    {
        char buff[ 16 ];
		
		sdp_payload_init(&payload);
		payload.pt = lc->sip_conf.rtp_redundant_payload_type;
		payload.localport = lc->rtp_conf.audio_rtp_port  + s_id *2;
		payload.a_rtpmap = "red/8000/1";
		
		sprintf( buff, "%d/%d", lc->sip_conf.rtp_redundant_codec_type, 
								lc->sip_conf.rtp_redundant_codec_type );
		payload.a_fmtp = buff;
		
		sdp_context_add_audio_payload(ctx,&payload);
	}
#endif

/* try the codec change here!! eric wu in 10.22 */
	elem=lc->codecs_conf.audio_codecs;
	while(elem!=NULL){
		codec=(PayloadType*) elem->data;
		if (payload_type_usable(codec) && payload_type_enabled(codec)){
			sdp_payload_init(&payload);

			if ((strcasecmp(codec->mime_type, "red") == 0)  ) {
				if( lc->sip_conf.rtp_redundant_payload_type ) {
					payload.a_rtpmap = g_strdup_printf("%s/%i/1",codec->mime_type,codec->clock_rate);
					payload.a_fmtp = g_strdup_printf("%d/%d",lc->sip_conf.rtp_redundant_codec_type, 
										lc->sip_conf.rtp_redundant_codec_type );
				}else{
					elem=g_list_next(elem);
					continue;
				}
			}else
				payload.a_rtpmap=g_strdup_printf("%s/%i/1",codec->mime_type,codec->clock_rate);
#ifdef CONFIG_RTK_VOIP_SRTP
			if( 1 == lc->sip_conf.security_enable ){
				payload.proto="RTP/SAVP";
			}
#endif /* CONFIG_RTK_VOIP_SRTP */
#ifdef DYNAMIC_PAYLOAD
			if (strcasecmp(codec->mime_type, "iLBC") == 0 &&
				lc->rtp_conf.iLBC_mode == ILBC_20MS)
			{
				payload.a_fmtp = "mode=20";
			}
			else if (strcasecmp(codec->mime_type, "SPEEX") == 0)
			{
				switch (lc->rtp_conf.speex_nb_rate)
				{
					case SPEEX_RATE2P15:
						payload.a_fmtp = "mode=1";
						break;

					case SPEEX_RATE5P95:
						payload.a_fmtp = "mode=2";
						break;

					case SPEEX_RATE8:
						payload.a_fmtp = "mode=3";
						break;

					case SPEEX_RATE11:
						payload.a_fmtp = "mode=4";
						break;

					case SPEEX_RATE15:
						payload.a_fmtp = "mode=5";
						break;

					case SPEEX_RATE18P2:
						payload.a_fmtp = "mode=6";
						break;

					case SPEEX_RATE24P6:
						payload.a_fmtp = "mode=7";
						break;

					case SPEEX_RATE3P95:
						payload.a_fmtp = "mode=8";
						break;
				}
			}
#ifdef SUPPORT_G7111
			else if (strcasecmp(codec->mime_type, "PCMU-WB") == 0 ||
					(strcasecmp(codec->mime_type, "PCMA-WB") == 0))
			{
				payload.a_fmtp = rtk_g7111_modes_precedence_to_str(
					lc->rtp_conf.g7111_precedence);
			}
#endif // SUPPORT_G7111
#endif // DYNAMIC_PAYLOAD
			static_pt=rtp_profile_get_payload_number_from_rtpmap(
				lc->local_profile,payload.a_rtpmap);
		#ifdef DYNAMIC_PAYLOAD
			payload.pt = dynamic_pt_get(static_pt);
			if (payload.pt == -1)
				payload.pt = static_pt;
		#else
			payload.pt = static_pt;
		#endif
			payload.localport = lc->rtp_conf.audio_rtp_port  + s_id *2;
		#ifdef AUTO_RESOURCE
			if (VOIP_RESOURCE_UNAVAILABLE == rcm_tapi_VoIP_resource_check(lc->chid, static_pt))
				g_warning("no resource for payload %s.\n", codec->mime_type);
			else
				sdp_context_add_audio_payload(ctx,&payload);
		#else
			sdp_context_add_audio_payload(ctx,&payload);
		#endif
			g_free(payload.a_rtpmap);
		}
		elem=g_list_next(elem);
	}
	// add srtp attribute if needed
#ifdef CONFIG_RTK_VOIP_SRTP
	if( 1 == lc->sip_conf.security_enable ){
		unsigned char localKey[30];	//local srtp key

		sdp_generate_sdes_key(localKey, 30);
		base64Encode(localKey, payload.localSDPKey);
		payload.localCryptAlg = HMAC_SHA1;
		sdp_message_a_attribute_add
								(ctx->offer, 0, osip_strdup("crypto"),
								g_strdup_printf("1 AES_CM_128_HMAC_SHA1_80 inline:%s",payload.localSDPKey));	
	}
#endif /* CONFIG_RTK_VOIP_SRTP */
	/* Mandy add for stun support */
#if 0 // not support STUN now 
	if (lc->net_conf.nat_address!=NULL && lc->net_conf.use_nat)
    {
		sdp_connection_t *conn = NULL;
        char *port=NULL;
        sdp_message_t *sdp = ctx->offer;

		// rock: get global c_connection in session
		conn = sdp_message_connection_get(sdp, -1, 0);
		// rock: check c_connection in first media line
		if (conn == NULL)
			conn = sdp_message_connection_get(sdp, 0, 0);
		// rock: shouldn't occur
		if (conn == NULL)
			g_error("conn is null?\n");

		osip_free(conn->c_addr);
		conn->c_addr=osip_strdup(lc->net_conf.nat_address);
		port = (char*)osip_malloc(10);
		snprintf(port, 9, "%i", lc->net_conf.nat_voice_port[s_id]);
		sdp_message_m_port_set(sdp, 0, port);
		g_message("Set audio offer: The external port is %d.\n", lc->net_conf.nat_voice_port[s_id]);
	}
#endif
	if (lc->sip_conf.dtmf_type == DTMF_RFC2833)		// need telephone-event only if DTMF_RFC2833
	{
		/* add telephone-event payload*/
		sdp_payload_init(&payload);
		payload.pt = lc->sip_conf.rfc2833_payload_type;
		payload.a_rtpmap="telephone-event/8000";
		payload.a_fmtp="0-15";
		sdp_context_add_audio_payload(ctx,&payload);
	}

#ifdef SUPPORT_V152_VBD	
	if( lc->sip_conf.v152_payload_type ) {
		/* add gpmd:96 vbd=yes */
		sdp_payload_init(&payload);
		payload.pt = lc->sip_conf.v152_payload_type;
		
		switch( lc->sip_conf.v152_codec_type ) {
		case rtpPayloadPCMA:
			payload.a_rtpmap = "PCMA/8000";
			break;
			
		case rtpPayloadPCMU:
		default:
			payload.a_rtpmap = "PCMU/8000";
			break;
		}
		
		payload.a_gpmd = "vbd=yes";
		sdp_context_add_audio_payload(ctx,&payload);
	}
#endif

	// add sendrecv attribute
	sdp_message_a_attribute_add(ctx->offer, 0, osip_strdup ("ptime"), osip_strdup ("20"));
	sdp_message_a_attribute_add(ctx->offer, 0, osip_strdup ("sendrecv"), NULL);

	return 0;
}

#ifdef SUPPORT_RTP_REDUNDANT_APP
static int get_supported_rtp_redundant_payload_type( const sdp_payload_t *payload )
{
	int pt_red, t;
	const char *p = payload ->a_fmtp;
	
	if( p == NULL )
		return -1;
	
	pt_red = atoi( payload ->a_fmtp );
	
	// all redundant codec should be the same 
	while( *p ) {
		
		if( *p == '/' ) {
			t = atoi( p + 1 );
			
			if( pt_red != t )
				return -1;
		}
		
		p ++;
	}
	
	// check valid codec (u-law, a-law, G.729)
	switch( pt_red ) {
	case rtpPayloadPCMU:
	case rtpPayloadPCMA:
	case rtpPayloadG729:
		break;
		
	default:
		return -1;
	}
		
	return pt_red;
}
#endif

SupportLevel linphone_payload_is_supported(sdp_payload_t *payload, RtpProfile *local_profile, 
	RtpProfile *dialog_profile)
{
	int localpt;
	SupportLevel ret;

	if (payload->a_rtpmap!=NULL){
		localpt=rtp_profile_get_payload_number_from_rtpmap(local_profile,payload->a_rtpmap);
		// check static payload type
		if (payload->pt >= 0 && 
			payload->pt < 96 &&
			localpt != payload->pt)
		{
			g_warning("invalid static payload type (%s)\n", payload->a_rtpmap);
			return Unsupported;
		}
	}else{
		// check dynamic payload type
		if (payload->pt >= 96 && 
			payload->pt < RTP_PROFILE_MAX_PAYLOADS)
		{
			g_warning("no rtpmap in dynamic payload type\n");
			return Unsupported;
		}
		localpt=payload->pt;
		g_warning("payload has no rtpmap.\n");
	}

	if (localpt >= 0 && localpt < RTP_PROFILE_MAX_PAYLOADS){
		/* this payload is understood, but does the user want to use it ?? */
		PayloadType *rtppayload;
		rtppayload=rtp_profile_get_payload(local_profile,localpt);
		if (rtppayload==NULL) {
			g_warning("strange error !!\n");
			return Unsupported;
		}

		if (strcmp(rtppayload->mime_type,"telephone-event") == 0)
		{
			ret = Supported;
		}
#ifdef SUPPORT_V152_VBD
		else if( payload ->a_gpmd && strstr( payload ->a_gpmd, "vbd=yes" ) )
		{
			ret = Supported;
		}
#endif
#ifdef SUPPORT_RTP_REDUNDANT_APP
		else if( payload ->a_rtpmap && 
				 strncmp( payload ->a_rtpmap, "red", 3 ) == 0 )
		{
			if( ( payload ->pt_red = get_supported_rtp_redundant_payload_type( payload ) )
				== -1 )
			{
				ret = Unsupported;
			} else
				ret = SupportedAndValid;
		}
#endif
		else
		{
			if (!payload_type_usable(rtppayload) ){
				g_warning("payload %s is not usable\n",rtppayload->mime_type);
				return Unsupported;
			}
			if ( !payload_type_enabled(rtppayload)) {
				g_warning("payload %s is not enabled.\n",rtppayload->mime_type);
				return Unsupported;
			}
			ret = SupportedAndValid;
		}

		if (dialog_profile!=NULL){
			/* this payload is supported in our local rtp profile, so add it to the dialog rtp
			profile */
			rtppayload=payload_type_clone(rtppayload);
			rtp_profile_set_payload(dialog_profile,payload->pt,rtppayload);
			/* add to the rtp payload type some other parameters (bandwidth) */
			if (payload->b_as_bandwidth!=0)
			{
				rtppayload->normal_bitrate=payload->b_as_bandwidth*1000;
			}
			if (payload->a_fmtp!=NULL)
			{
				rtppayload->fmtp=g_strdup(payload->a_fmtp);
			}
			else if (strcasecmp(rtppayload->mime_type,"iLBC")==0){
				/*default to 30 ms mode */
				payload->a_fmtp="ptime=30";
				rtppayload->fmtp=g_strdup(payload->a_fmtp);
			}
		}
		return ret;
	}
	return Unsupported;
}

#ifdef SUPPORT_V152_VBD
static int linphone_fill_vbd_audio_param( StreamParams *params, sdp_payload_t *payload )
{
	if( payload ->a_gpmd && strstr( payload ->a_gpmd, "vbd=yes" ) )
	{
		int pt_vbd = -1;
	
		if( payload ->a_rtpmap ) {
			if( strstr( payload ->a_rtpmap, "PCMU/8000" ) )
				pt_vbd = rtpPayloadPCMU;
			else if( strstr( payload ->a_rtpmap, "PCMA/8000" ) )
				pt_vbd = rtpPayloadPCMA;
			else
				return -1;
		}
		
		if( pt_vbd != -1 ) {
			params ->initialized_vbd = 1;
			params ->static_pt_vbd = pt_vbd;
			params ->local_pt_vbd = payload ->pt;
			params ->remote_pt_vbd = payload ->pt;
		}
	}
	
	return 1;
}
#endif

#ifdef SUPPORT_RTP_REDUNDANT_APP
static int linphone_fill_rtp_redudant_audio_param( LinphoneCore *lc, StreamParams *params, sdp_payload_t *payload )
{
	if( payload ->a_rtpmap &&
		strncmp( payload ->a_rtpmap, "red", 3 ) == 0)
	{
		if( lc->sip_conf.rtp_redundant_payload_type == 0 )
			return -1;	// disable on web 
		
		// fill rtp redundant payload type 
		params->initialized_rtp_redundant = 1;
		params->local_pt_rtp_redundant = payload ->pt;
		params->remote_pt_rtp_redundant = payload ->pt;
		
		return 1;
	}
	
	return 0;
}
#endif

/* incoming call audio setting --> accept_audio_codecs */
int linphone_accept_audio_offer(sdp_context_t *ctx,sdp_payload_t *payload)
{
	StreamParams *params;
	int supported;
	int s_id=0, i;
	LinphoneCall *call=(LinphoneCall*)sdp_context_get_user_pointer(ctx);
	LinphoneCore *lc=call->core;
#ifdef SUPPORT_RTP_REDUNDANT_APP
	int rtp_redundant_case = 0;
#endif
	
	/* get the session number */
	for( i = 0; i < MAX_SS; i++){
		if( call == lc->call[i] ){
			s_id = i;
			break;
		}
	}

	supported = linphone_payload_is_supported(payload, lc->local_profile, NULL);
	if (supported == Unsupported) 
	{
		g_message("Refusing audio codec %i (%s)\n",payload->pt,payload->a_rtpmap);
		return Unsupported;
	}

	if (supported == SupportedAndValid)	
	{
	#if 0//def CHECK_LOOPBACK	
		if (ctx->loopback == 1 && 
			payload->pt != rtpPayloadPCMU)
		{
			// Support PCMU only if Loopback
			return Unsupported;
		}
	#endif
		params=&call->audio_params;
		if (params->initialized==0){

#ifdef SUPPORT_RTP_REDUNDANT_APP
			if( ( rtp_redundant_case =
					linphone_fill_rtp_redudant_audio_param( lc, params, payload ) )
					== -1 )
			{
				return Unsupported;	// disable on web 
			}
#endif
			
			/* this is the first codec we may accept*/
			params->localport = payload->localport = lc->rtp_conf.audio_rtp_port + s_id *2;
			params->remoteport=payload->remoteport;
			params->line=payload->line;
#ifdef CONFIG_RTK_VOIP_SRTP
			if(1 == lc->sip_conf.security_enable){
				memcpy(&params->remoteSDPKey, payload->remoteSDPKey, SDP_KEY_LEN);
				memcpy(&params->localSDPKey, payload->localSDPKey, SDP_KEY_LEN);
				params->remoteCryptAlg=payload->remoteCryptAlg;
			}
#endif /* CONFIG_RTK_VOIP_SRTP */
#ifdef DYNAMIC_PAYLOAD
			/* remember local pt (the same with remote pt always if we are callee) */
			params->local_pt=payload->pt;
			/* remember remote pt */
			params->remote_pt=payload->pt; 
			
			/* remember static pt for rtk dsp */
			if (payload->pt >= 0 && payload->pt < 96) 
				params->static_pt = payload->pt;
			else
				params->static_pt = rtp_profile_get_payload_number_from_rtpmap(lc->local_profile,payload->a_rtpmap);
				
#ifdef SUPPORT_RTP_REDUNDANT_APP
			if( rtp_redundant_case ) {
				// it has to be 0, 8 or 18 
				params->local_pt = params->remote_pt = params->static_pt = payload->pt_red;
			}
#endif
						
			/* check iLBC mode */
			if (params->static_pt == rtpPayload_iLBC &&
				payload->a_fmtp &&
				strcmp(payload->a_fmtp, "mode=20") == 0)
			{
				params->static_pt = rtpPayload_iLBC_20ms;		
			}
			/* check SPEEX NB bitrate */
			else if (params->static_pt == rtpPayload_SPEEX_NB_RATE8 &&
				payload->a_fmtp )
			{
					params->static_pt = rtpPayload_SPEEX_NB_RATE8;
				if (strcmp(payload->a_fmtp, "mode=1") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE2P15;
				if (strcmp(payload->a_fmtp, "mode=2") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE5P95;
				if (strcmp(payload->a_fmtp, "mode=3") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE8;
				if (strcmp(payload->a_fmtp, "mode=4") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE11;
				if (strcmp(payload->a_fmtp, "mode=5") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE15;
				if (strcmp(payload->a_fmtp, "mode=6") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE18P2;
				if (strcmp(payload->a_fmtp, "mode=7") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE24P6;
				if (strcmp(payload->a_fmtp, "mode=8") == 0)
					params->static_pt = rtpPayload_SPEEX_NB_RATE3P95;
			}
#ifdef SUPPORT_G7111
			else if (params->static_pt == rtpPayloadPCMU_WB ||
				params->static_pt == rtpPayloadPCMA_WB)
			{
				rtk_g7111_modes_str_to_precedence(payload->a_fmtp,
					lc->rtp_conf.g7111_precedence, params->g7111_precedence);

				payload->a_fmtp = rtk_g7111_modes_precedence_to_str(
					params->g7111_precedence);
			}
#endif

#ifdef AUTO_RESOURCE
			if (VOIP_RESOURCE_UNAVAILABLE == rcm_tapi_VoIP_resource_check(lc->chid, params->static_pt))
			{
				g_warning("no resource for payload %s.\n", payload->a_rtpmap);
				return Unsupported;
			}
#endif

#else
			params->pt=payload->pt; /* remember the first payload accepted */
		#endif
//			params->remoteaddr=payload->c_addr;
			bzero(&params->remote_addr, sizeof(params->remote_addr));
			strcpy(params->remote_addr,payload->c_addr);
			params->remote_ip=inet_addr(payload->c_addr);
			
			params->initialized=1;
		}else{
			/* refuse all other audio lines*/
			if(params->line!=payload->line) return -1;
		}
	}
#ifdef SUPPORT_V152_VBD
	else if (supported == Supported && payload ->a_gpmd)
	{
		if( lc->sip_conf.v152_codec_type == rtpPayloadUndefined )
			return Unsupported;	// disable on web 
		
		if( linphone_fill_vbd_audio_param( &call->audio_params, payload ) == -1 )
			return Unsupported;
	}
#endif

	return supported;
}

/* --> get_audio_codecs */
int linphone_read_audio_answer( sdp_context_t * ctx , sdp_payload_t * payload )
{
	LinphoneCall * call = ( LinphoneCall * )sdp_context_get_user_pointer( ctx ) ;
	LinphoneCore * lc = call->core ;
	StreamParams * params ;
	SupportLevel supported;
	int s_id=0, i;
#ifdef SUPPORT_RTP_REDUNDANT_APP
	int rtp_redundant_case = 0;
#endif

	supported = linphone_payload_is_supported(payload, lc->local_profile, NULL);

	for( i = 0; i < MAX_SS; i++){
		if( call == lc->call[i] ){
			s_id = i;
			break;
		}
	}
	
	if (supported == Unsupported)
	{
		g_warning("This remote sip phone did not answer properly to my sdp offer!\n");
		return 0;
	}

	if (supported == SupportedAndValid) 
	{
		params = &call->audio_params;
		if (params->initialized == 0)
		{
#ifdef DYNAMIC_PAYLOAD
			char *offer_fmtp = NULL;
#endif

#ifdef SUPPORT_RTP_REDUNDANT_APP
			if( ( rtp_redundant_case =
					linphone_fill_rtp_redudant_audio_param( lc, params, payload ) )
					== -1 )
			{
				return Unsupported;	// disable on web 
			}
#endif

			/* this is the first codec we may accept */
			params->localport = lc->rtp_conf.audio_rtp_port + s_id *2;
			params->remoteport = payload->remoteport ;
			params->line = payload->line ;
#ifdef CONFIG_RTK_VOIP_SRTP
			if(1 == lc->sip_conf.security_enable){
				memcpy(&params->remoteSDPKey, payload->remoteSDPKey, SDP_KEY_LEN);
				memcpy(&params->localSDPKey, payload->localSDPKey, SDP_KEY_LEN);
				params->remoteCryptAlg=payload->remoteCryptAlg; 
			}
#endif /* CONFIG_RTK_VOIP_SRTP */
#ifdef DYNAMIC_PAYLOAD
			/* remember local pt */
			if (payload->pt >= 0 && payload->pt < 96)
			{
				// static payload type
				params->local_pt = payload->pt;
			}
			else 
			{
				// dynamic payload type
				int j, pt;
				char *pt_str, *rtpmap;

				/* for each m= line */
				for (i = 0; !sdp_message_endof_media(ctx->offer, i); i++)
				{
					for (j = 0; ((pt_str = sdp_message_m_payload_get(ctx->offer, i , j)) != NULL); j++)
					{
						pt = osip_atoi(pt_str);
						if (payload->pt >= 0 && payload->pt < 96)
							continue;

						rtpmap = sdp_message_a_attr_value_get_with_pt(ctx->offer, i, pt, "rtpmap");
						if (strcmp(rtpmap, payload->a_rtpmap) == 0)
						{
							params->local_pt = pt; // found!
							offer_fmtp = sdp_message_a_attr_value_get_with_pt(ctx->offer, i, pt, "fmtp");
							break;
						}
					}

					if (pt_str != NULL) // found!
						break;
				}

				if (sdp_message_endof_media(ctx->offer, i)) // not found!?
				{
					// shouldn't occur
					g_warning("unknonw rtpmap = %s\n", payload->a_rtpmap);
					return -1;
				}
			}
			/* remember remote pt */
			params->remote_pt=payload->pt; 
			/* remember static pt for rtk dsp */
			if (payload->pt >= 0 && payload->pt < 96)
				params->static_pt = payload->pt;
			else
				params->static_pt = rtp_profile_get_payload_number_from_rtpmap(lc->local_profile,payload->a_rtpmap);
			
#ifdef SUPPORT_RTP_REDUNDANT_APP
			if( rtp_redundant_case ) {
				// it has to be 0, 8 or 18 
				params->local_pt = params->remote_pt = params->static_pt = payload->pt_red;
			}
#endif
			
#ifdef SUPPORT_G7111
			if (params->static_pt == rtpPayloadPCMU_WB ||
				params->static_pt == rtpPayloadPCMA_WB)
			{
				rtk_g7111_modes_str_to_precedence(payload->a_fmtp,
					lc->rtp_conf.g7111_precedence, params->g7111_precedence);
			}
			else
#endif
			/* check iLBC mode */
			if (params->static_pt == rtpPayload_iLBC &&
				payload->a_fmtp &&
				strcmp(offer_fmtp, "mode=20") == 0 &&
				strcmp(payload->a_fmtp, "mode=20") == 0)
			{
				params->static_pt = rtpPayload_iLBC_20ms;		
			}
#else
			params->pt=payload->pt; /* remember the first payload accepted */
#endif
//			params->remoteaddr = payload->c_addr ;
			bzero(&params->remote_addr, sizeof(params->remote_addr));
			strcpy(params->remote_addr,payload->c_addr);
			params->remote_ip=inet_addr(payload->c_addr);

			params->initialized = 1 ;
		}
	}
#ifdef SUPPORT_V152_VBD
	else if (supported == Supported && payload ->a_gpmd)
	{
		if( lc->sip_conf.v152_codec_type == rtpPayloadUndefined )
			return Unsupported;	// disable on web 
		
		if( linphone_fill_vbd_audio_param( &call->audio_params, payload ) == -1 )
			return Unsupported;
	}
#endif

	return 0 ;
}

/*++added by Jack Chan 02/02/07 for t.38++*/
int linphone_read_t38_answer( sdp_context_t * ctx , sdp_payload_t * payload )
{
	LinphoneCall * call = ( LinphoneCall * )sdp_context_get_user_pointer( ctx ) ;
	LinphoneCore * lc = call->core ;
	StreamParams * params ;

	params = &call->t38_params ;
	if( params->initialized == 0 )
	{
		/* this is the first codec we may accept */
		params->localport = lc->sip_conf.T38_prot;
		params->remoteport = payload->remoteport ;
		params->line = payload->line ;
	#ifdef DYNAMIC_PAYLOAD
		params->local_pt=rtpPayloadT38_Virtual;
		params->remote_pt=rtpPayloadT38_Virtual;
		params->static_pt=rtpPayloadT38_Virtual;
	#else
		params->pt=payload->pt; /* remember the first payload accepted */
	#endif
//		params->remoteaddr = payload->c_addr ;	
		bzero(&params->remote_addr, sizeof(params->remote_addr));
		strcpy(params->remote_addr,payload->c_addr);
		params->remote_ip=inet_addr(payload->c_addr);
	
		params->initialized = 1 ;
	}
	return 0 ;
}
/*--end--*/

void linphone_call_proceeding(LinphoneCore *lc, uint32 ssid, int cid, int did)
{
	if (lc->call[ssid] == NULL)
	{
		g_warning( "Bug in call_proceeding()\n" ) ;
		return ;
	}

	lc->call[ssid]->cid = cid;
	lc->call[ssid]->did = did;

}
/*peer side ringing */
/*receive 180ring */
void linphone_call_ringing(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev)
{
	/* 20110711, SD6, bohungwu, exosip 3.5 integration */
	/* Retrieve SDP data structure and conver it to a string as requested by Linphone */
	sdp_message_t *sdp_msg;
	osip_header_t *require_header=NULL;
	osip_message_t *prack_msg=NULL;
	unsigned short oam_port_status;
	unsigned char tr069_ECI_voice_state;
	osip_header_t *p_early_media=NULL;
	int j=0;
	struct timeval now;
	int call_setup_timer=0;	
	
	set_autocall_state(lc,YUEME_TEST_RING_TONE);
		
	osip_message_header_get_byname(ev->response, "require",
						   0, &require_header);	
	/*check PRACK requrie first */		

	if (require_header==NULL || require_header->hvalue==NULL){
		/* UNCOMPLIANT UA without a require header */
	}else if (0==osip_strncasecmp(require_header->hvalue, "100rel", 6)){
		g_message("ev->tid %d, send PRACK \n",ev->tid);
		/* need return PRACK */

			eXosip_lock();
			eXosip_call_build_prack(ev->tid,&prack_msg);
			eXosip_call_send_prack(ev->tid,prack_msg);
			eXosip_unlock();

		}

	g_message("start ringback tone timeour %d sec\n",g_pVoIPCfg->NoAnswerTimer);
	if(g_pVoIPCfg->NoAnswerTimer!=0) 
	rcm_timerLaunch(lc->ringback_timeout_timer,g_pVoIPCfg->NoAnswerTimer*1000); 


	
	while (osip_message_header_get_byname(ev->response,"P-Early-Media",j,&p_early_media)>=0){
		if (p_early_media){
			g_message("P-Early-Media->hvalue is %s\n",p_early_media->hvalue);
			 if (0==osip_strncasecmp(p_early_media->hvalue, "inactive", 8)){
			 	if(osip_message_get_status_code(ev->response)==180){
					
	if (lc->audiostream[ssid])
	{
						g_message("already strart rtp");
						linphone_core_stop_media_streams(lc, ssid);
						lc->call[ssid]->p_early_media_stop_rtp=1;
					}
					
					g_message("P-Early-Media inactive \n");
			 		if( lc->ringstream[ssid] ){
						g_message( " Already ringing !\n" ) ;
					}else {
						g_message( "  ring [%d][%d]...\n" , lc->chid , ssid ) ;
						lc->ringstream[ssid] = TRUE;
						rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_RINGING, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
					}
		return ;
	}
			 }
			 else if (0==osip_strncasecmp(p_early_media->hvalue, "gated", 5)){
			 	if(osip_message_get_status_code(ev->response)==180){
					if (lc->audiostream[ssid]) 
 						return; 
			 	}
			 }else if  (0==osip_strncasecmp(p_early_media->hvalue, "sendrecv", 8)){
			 	if(osip_message_get_status_code(ev->response)==180){
					if (lc->audiostream[ssid]) 
 						return; 
			 	}
			 }
		}else{
			if(osip_message_get_status_code(ev->response)==180){
				if (lc->audiostream[ssid])
				{
					g_message("already strart rtp");
					linphone_core_stop_media_streams(lc, ssid);
					lc->call[ssid]->p_early_media_stop_rtp=1;
				}
				g_message("P-Early-Media NULL");
				if( lc->ringstream[ssid] ){
					g_message( "Already ringing !\n" ) ;
				}else {
					g_message( "P-Early-Media header NULL  ring [%d][%d]...\n" , lc->chid , ssid ) ;
					lc->ringstream[ssid] = TRUE;
					rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_RINGING, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				}
				return ;
			}
		}
		j++;
	}
	

	if (lc->audiostream[ssid])
	{
		g_message("already strart rtp");
		//linphone_core_stop_media_streams(lc, ssid);
		//lc->call[ssid]->p_early_media_stop_rtp=1;
	} 
	

	linphone_call_proceeding(lc, ssid, ev->cid, ev->did);
	lc->call[ssid]->tid = ev->tid;

	sdp_msg = eXosip_get_sdp_info(ev->response);

	if(sdp_msg==NULL)
		g_message("this ev not have sdp\n ");
	
	if( sdp_msg!=NULL )
	{
		/*	somebody reported a server who uses a 18x answer to establish mediastreams and play
			an informationnal message */
		//linphone_call_accepted(lc, ssid, ev->did, ev->sdp_body);
		sdp_message_free(sdp_msg);	

		linphone_call_accepted(lc, ssid, ev);
	}
	//else if( ev->status_code == 180 )
	else if( osip_message_get_status_code(ev->response))
	{
		
		if(osip_message_get_status_code(ev->response)==183){
			g_message( "183 ring not have dsp, not do anything!\n" ) ;
			return;
	
		}
		
		if( lc->ringstream[ssid] )
		{
			g_message( "Already ringing !\n" ) ;
			return ;
		}

		g_message( "Remote ringing [%d][%d]...\n" , lc->chid , ssid ) ;
		if (lc->ringstream[ssid] == FALSE)
		{
			lc->ringstream[ssid] = TRUE;
			rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_RINGING, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
			oam_port_status=PORT_RINGBACKTONE;
			voip_line_state_share_id_set(lc->chid, OAM_VOIP_PORTSTATUS,&oam_port_status);
			tr069_ECI_voice_state=EICVS_PORT_RINGBACKTONE;
			voip_line_state_share_id_set(lc->chid, TR069_VOIP_EIC_VOICE_STATE,&tr069_ECI_voice_state);	
		}
	}

	osip_gettimeofday(&now, NULL);
	g_message("--- now tv_sec (%d) ---\n",now.tv_sec);	
	g_message("--- now tv_usec (%d) ---\n",now.tv_usec);	
	g_message("--- Call_setup_timer_start tv_sec (%d) ---\n",lc->call[ssid]->call_setup_timer_start.tv_sec);
	g_message("--- Call_setup_timer_start tv_usec (%d) ---\n",lc->call[ssid]->call_setup_timer_start.tv_usec);
	call_setup_timer=((now.tv_sec-lc->call[ssid]->call_setup_timer_start.tv_sec)*1000)+((now.tv_usec-lc->call[ssid]->call_setup_timer_start.tv_usec)/1000);
	voip_line_state_share_id_set(lc->chid,OMCI_SHARE_CCPM_SETUPTIMER,&call_setup_timer);
}

void linphone_update_and_display_registration_status( LinphoneCore *lc, LinphoneProxyConfig *cfg, ProxyRegStatus newStatus )
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];	
	char buf_status = 0;
	const char *pszPrompt=NULL;
	unsigned char omci_state;
	g_message( "SET REG status from %d to %d(%d:%d).\n", cfg ->reg_status, newStatus, lc ->chid, cfg ->index );
	
	// set new status 
	if( cfg ->reg_status == newStatus )
		return;
	
	cfg ->reg_status = newStatus;
	
	// decide display status and warning string 
	switch( newStatus ) {
	case REGSTATUS_REG_ING:					/* registering */

		buf_status = '2';
		pszPrompt = "registration_ongoing";

		omci_state=REGISTER_ONGOING;
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_LS_VOIPVOICESERVERSTATUS,&omci_state);
			

		break;
		
	case REGSTATUS_REG_SUCCESS:				/* register success */
	case REGSTATUS_REG_SUCCESS_EXPIRED:		/* register success but expired, so it is registering */
		buf_status = '1';
		pszPrompt = "registration_success";
		omci_state=REGISTERED;
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_LS_VOIPVOICESERVERSTATUS,&omci_state);
		break;
		
	case REGSTATUS_INIT:
		buf_status = '0';
		pszPrompt = "no_register";
		omci_state=NONE_OR_INITIAL;
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_LS_VOIPVOICESERVERSTATUS,&omci_state);
		break;
	case REGSTATUS_RESTART:
		buf_status = '4';
		break;
	case REGSTATUS_REG_FAIL_RETRY_S:		/* register failure (retry in short delay) */		
		buf_status = '5';
		omci_state=FAILED_REGISTRATION_SERVER_FAIL_CODE;
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_LS_VOIPVOICESERVERSTATUS,&omci_state);
	
		break;
	case REGSTATUS_REG_FAIL:				/* register failure (don't retry) */
	case REGSTATUS_REG_FAIL_RETRY_L:		/* register failure (retry in long delay) */
	default:
		buf_status = '3';
		pszPrompt = "registration_faillure";
		omci_state=FAILED_REGISTRATION_SERVER_FAIL_CODE;
		voip_line_state_share_id_set(lc->chid,OMCI_SHARE_LS_VOIPVOICESERVERSTATUS,&omci_state);	
		break;
	}
	
	
	// update status for web 
	fh = fopen(_PATH_TMP_STATUS, "r+");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		printf("\nerrno=%d\n", errno);
	}
	else {
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("%s(%d): The content of /tmp/status is NULL!!\n", pszPrompt, newStatus);
			printf("\nerrno=%d\n", errno);
		}
		else {
			buf[lc->chid * MAX_PROXY + cfg->index] = buf_status;
			fseek(fh, 0L, SEEK_SET);
			fwrite(buf, sizeof(buf), 1, fh);
		}
		fclose(fh);
		chmod(_PATH_TMP_STATUS,0666);
	}	
}

void E8C_linphone_update_and_display_registration_status( int chid, int newStatus )
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS];	
	char buf_status = 0;
	const char * pszPrompt = NULL;
	const char * osgiStatus = "";
	int send_omd=0;
	g_message("E8C_linphone_update_and_display_registration_status ,chid is %d, newStatus is %d\n",chid,newStatus);
	// decide display status and warning string 
	switch( newStatus ) {
	case VOIP_STATUS_INIT:
		buf_status = '0';
		pszPrompt = "INIT";		
		break;
	case VOIP_STATUS_SUCCESS:
		send_E8C_alarm(ALARM_SUSESS);
		buf_status = '1';
		pszPrompt = "STATUS_SUCCESS";
		osgiStatus ="SUCCESS";
		send_omd=1;
		break;
	case VOIP_NOVALID_CONNECTION:
		buf_status = '2';
		pszPrompt = "NOVALID_CONNECTION";
		send_omd=1;
		break;
	case VOIP_STATUS_IADERROR:
		buf_status = '3';
		pszPrompt = "STATUS_IADERROR";
		osgiStatus ="IADERROR";
		send_omd=1;
		break;
	case VOIP_STATUS_NOROUTE:
		send_E8C_alarm(ALARM_SERVER_UNREACHABLE);
		buf_status = '4';
		pszPrompt = "STATUS_NOROUTE";
		osgiStatus ="NOROUTE";
		send_omd=1;
		break;
	case VOIP_STATUS_NORESPONSE:
		send_E8C_alarm(ALARM_NO_RESPONSE);
		buf_status = '5';
		pszPrompt = "STATUS_NORESPONSE";
		osgiStatus ="NORESPONSE";
		send_omd=1;
		break;
	case VOIP_STATUS_ACCOUNTERR:
		send_E8C_alarm(ALARM_AUTH_FAIL);
		buf_status = '6';
		pszPrompt = "STATUS_ACCOUNTERR";
		osgiStatus ="ACCOUNTERR";
		send_omd=1;
		break;
	case VOIP_STATUS_UNKNOWN:
		send_E8C_alarm(ALARM_OTHER_FAIL);
		buf_status = '7';
		pszPrompt = "STATUS_UNKNOWN";
		osgiStatus="UNKNOWN";
		send_omd=1;
		break;
	case VOIP_STATUS_LINE_DISABLED:
		buf_status = '8';
		pszPrompt = "STATUS_LINE_DISABLED";
		break;
	case VOIP_STATUS_ADDR_UNKNOW_HOST:
		buf_status = '9';
		pszPrompt = "STATUS_UNKNOW_HOST";
		send_omd=1;
	default :
		break;

	}
	

	voip_line_state_share_id_set(chid,OSGI_VOIP_LINESTATUS,osgiStatus);
	g_message( "SET registration status  chid %d  newStatus [index %d] %s\n", chid, newStatus, pszPrompt);
	// update status for web 
	fh = fopen(_PATH_TMP_E8C_STATUS, "r+");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_E8C_STATUS);
		printf("\nerrno=%d\n", errno);
	}
	else {
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("%s(%d): The content of /tmp/E8C_voipstatus is NULL!!\n", pszPrompt, newStatus);
			printf("\nerrno=%d\n", errno);
		}
		else {
			buf[chid] = buf_status;
			fseek(fh, 0L, SEEK_SET);
			fwrite(buf, sizeof(buf), 1, fh);
		}
		fclose(fh);
	}	
	
	if (send_omd==1)
	{
		omd_fifo_reg_command();
#ifdef CONFIG_USER_DBUS_PROXY		
		notify_dbus(e_dbus_proxy_signal_voip_phone_register_change_id, pszPrompt);
#endif
	}
}


void linphone_registration_faillure(LinphoneCore *lc,eXosip_event_t *ev)
{
	//char *msg;
	int status_code = 0;
	const char *reason=NULL;
	voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[lc->chid];
	voipCfgProxy_t *proxy_cfg;
	proxy_cfg = &voip_ptr->proxies[0];
	osip_header_t *expire_header=NULL;
#ifdef FIX_REGISTER_TIME
	eXosip_reg_t *reg;
	osip_message_t *resp;
	osip_header_t *min_expire_header;
	int min_expire;
#endif
	LinphoneProxyConfig *cfg;
	//int i;
	ProxyRegStatus regstatus = REGSTATUS_REG_FAIL;
	int retry_register=0;

	g_message("\n.........linphone_registration_faillure...chid %d .ev->rid %d ....\n",lc->chid,ev->rid);
	if(g_pVoIPCfg->rfc_flags&SIP_FJ_CHANGE_REGISTER_TIMER){
		if(solar_ptr->bootup_slic_poweron==0){
			//enable slic power
			system("echo 125 > /proc/voip/debug");
			solar_ptr->bootup_slic_poweron=1;
		}
	}

	cfg = linphone_core_get_proxy_config_from_rid(lc, ev->rid);
	if(cfg==NULL){
		g_message("cfg is null \n");
		return;
	}
	g_return_if_fail(cfg != NULL);

	cfg->registered = FALSE;

	if (ev->response){
		status_code=osip_message_get_status_code(ev->response);
		reason=osip_message_get_reason_phrase(ev->response);
		g_message("linphone_registration_faillure status_code is %d\n",status_code);
	}
	
	if(lc->parent->bootup_unregistered==0){
		osip_message_get_expires(ev->request,0,&expire_header);

		if (expire_header!=NULL && atoi(expire_header->hvalue)!=0){
			g_message("register receive error response \n");
		}else{/*expire is 0 , for unregister */
			if (status_code==401||status_code==407)
			{
				linphone_register_authentication_required(lc,ev);
				regstatus = ( cfg->auth_failed ? REGSTATUS_REG_FAIL : 
											REGSTATUS_REG_FAIL_RETRY_S );	
				if ( cfg->auth_failed==TRUE)
					goto registration_faillure_retry;
				cfg->registered = FALSE ;
			}else
			{
				linphone_update_and_display_registration_status( lc, cfg, REGSTATUS_INIT );
				rcm_tapi_sip_register(lc->chid, 0);
				if(lc->parent->bootup_unregistered==0){
					lc->parent->bootup_unregistered=1;
					g_message("change bootup_unregistered to 1\n");
					eXosip_register_remove(ev->rid);
					rcm_linphone_register_init(solar_ptr);
				}
			}
			return;
		}
	}
	//[SD6, bohungwu, exosip 3.5 integration */
	//switch(ev->status_code){
	switch( status_code ) {
	//]
		case 401:
		case 407:
			linphone_register_authentication_required(lc,ev);
			regstatus = ( cfg->auth_failed ? REGSTATUS_REG_FAIL : 
										REGSTATUS_REG_FAIL_RETRY_S );	
			if ( cfg->auth_failed==TRUE)
			{
				retry_register=1;
				send_E8C_alarm(ALARM_AUTH_FAIL);
				goto registration_faillure_retry;
			}
			break;
#ifdef FIX_REGISTER_TIME
		case 423: // Interval too brief
			eXosip_reg_find_id(&reg, ev->rid);
			if (reg == NULL)
				goto registration_faillure_display;
	
			resp = reg->r_last_tr->last_response;
			if (resp == NULL)
				goto registration_faillure_display;

			osip_message_header_get_byname(resp, MIN_EXPIRES, 0, &min_expire_header);
			if (min_expire_header == NULL || min_expire_header->hvalue == NULL)
				goto registration_faillure_display;

			min_expire = atoi(min_expire_header->hvalue);
			cfg->expires = min_expire;					// update expires by min-expires
			regstatus = REGSTATUS_REG_FAIL_RETRY_S;
			break;
#endif			

	//	case 403:	// Forbidden  
		case 405:	// Method Not Allowed 
		/*ericchung: when receive 403, disable to retry REGISTER */
//			regstatus = REGSTATUS_REG_FAIL_RETRY_L;
			regstatus= REGSTATUS_REG_FAIL;	
			rcm_timerCancel(lc->register_retry_timer);
			eXosip_register_remove(ev->rid);
			linphone_proxy_config_enableregister(lc->proxies[cfg->index],0); //disable proxy conf
			voip_line_state_share_id_set(lc->chid, OMCI_SHARE_SIPCIPM_FAILEDAUTHENTICATECOUNTER,NULL);
			break;
			
		case 403:	// Forbidden  	//eric for some sip server, response 403, need re-try
			if ( cfg->reg_status==REGSTATUS_REG_FAIL_RETRY_S)
				send_E8C_alarm(ALARM_AUTH_FAIL);
			else
				send_E8C_alarm(ALARM_OTHER_FAIL);
			retry_register=1;
		default:
			regstatus = REGSTATUS_REG_FAIL;

registration_faillure_retry:
			if(reason==NULL||retry_register==1){
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
				if ((strlen(cfg->outbound_proxy_ip[1])!=0 )&&cfg->index_outbound_proxy==0)
				{
					eXosip_register_remove(ev->rid);
					g_message("use second dns cache ip =%s\n",cfg->outbound_proxy_ip[1]);
					//copy second dns cache ip
					cfg->index_outbound_proxy=1;
					osip_strncpy(cfg->outbound_proxy,cfg->outbound_proxy_ip[cfg->index_outbound_proxy],strlen(cfg->outbound_proxy_ip[cfg->index_outbound_proxy]));
					linphone_proxy_config_register(lc->proxies[0]);
					return;
				}
#endif
				g_message("Register no response timeout , proxy index %d\n",cfg->index);
				g_message("\r\n disable register bit\n");
				linphone_proxy_config_enableregister(cfg,0); //disable current proxy conf
				eXosip_register_remove(ev->rid);

				if(cfg->index==0){ 
					//first/main proxy register fail. check second proxy for register.
					if(( lc->proxies[1])&& (lc->proxies[1]->registered != TRUE) ){
						if(g_pVoIPCfg->rfc_flags & SIP_SERVER_REDUNDANCY){
							/*case 1 ,if enable SIP_SERVER_REDUNDANCY, 
							  	when main proxy register fail.
								if second proxy have proxy setting and not register , try use second proxy						
							*/
							g_message("try to second proxy \n");
							linphone_core_set_default_proxy(lc, lc->proxies[1]);
							linphone_proxy_config_enableregister(lc->proxies[1],1); //enable proxy conf
							linphone_proxy_config_register(lc->proxies[1]);
						}else{
							/* if no enable SIP_SERVER_REDUNDANCY , just retry register to proxy */
							rcm_timerLaunch(lc->register_retry_timer,proxy_cfg->RegisterRetryInterval*1000);
						}

					}else{
						//case 2. second proxy already registered or no second proxy, only retry 1st proxy.

						g_message("\r\n retry main proxy after 60s\n");
						rcm_timerLaunch(lc->register_retry_timer,proxy_cfg->RegisterRetryInterval*1000);	//90sec

					}

				}else{
					/* second/backup proxy fail   */				
					g_message("\r\n retry main proxy after 60s\n");
					rcm_timerLaunch(lc->register_retry_timer,proxy_cfg->RegisterRetryInterval*1000);  //90sec			

					if(g_pVoIPCfg->rfc_flags&SIP_FJ_CHANGE_REGISTER_TIMER){
						rcm_arp_test(lc);						
					}
					
				}								
			}			
//			linphonec_display_something(lc, msg);



			break;
	}
#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
	 tr104_Register_Update_Fail_Reason(lc->chid, status_code, cfg);
#endif	
registration_faillure_display:

	// update and display regitration status 
	linphone_update_and_display_registration_status( lc, cfg, regstatus );
	UpdateLedDisplayMode( lc );

	return; 

#ifdef MULTI_PROXY
  #if 1	/* pkshih: new default proxy policy */
#if 0
  	if( lc->default_proxy && lc->default_proxy->index == cfg ->index )
  	{	/* registration failure is default proxy */
  	
		// choose default proxy (registered proxy) 
		for( i = 0; i < MAX_PROXY; i ++ )
			if( lc->proxies[i] && lc->proxies[i]->registered )
				break;
	
		if( i != MAX_PROXY ) {
			// adopt a registered proxy 
			linphone_core_set_default_proxy(lc, lc->proxies[i]);
		} else {
			// For LED
			rcm_tapi_sip_register(lc->chid, 0);
			linphone_core_set_default_proxy(lc, NULL);
UpdateLedDisplayMode( lc );
		}
  	}  	
#endif
  #else
	// check first active proxy
	for (i=0; i<MAX_PROXY; i++)
	{
		if (lc->proxies[i] && !lc->proxies[i]->auth_failed)
			break;
	}

	if (i == MAX_PROXY) // all proxy failed 
	{
		// For LED
		rcm_tapi_sip_register(lc->chid, 0);
UpdateLedDisplayMode( lc );
		linphone_core_set_default_proxy(lc, NULL);
	}
	else if (cfg->index == lc->default_proxy->index)
	{		
		// change to first active proxy (not registered proxy) 
		linphone_core_set_default_proxy(lc, lc->proxies[i]);
	}
  #endif
#else
	// For LED
	if (cfg->auth_failed)
		rcm_tapi_sip_register(lc->chid, 0);
#endif
UpdateLedDisplayMode( lc );
}

void linphone_registration_success( LinphoneCore * lc , eXosip_event_t * ev )
{
	LinphoneProxyConfig * cfg ;
	//[SD6, bohungwu, exosip 3.5 integration 
#if 0
	//gchar * msg = g_strdup_printf( _( "Registration on %s sucessful." ) , ev->req_uri ) ;
	gchar * msg;
	osip_to_t *to;
	char *req_uri;
#endif
	osip_header_t *expire_header=NULL;
	osip_service_route_t * service_route=NULL;
	int pos=0;
	osip_header_t *date_header=NULL;


//[SD6, ericchung, rcm integration
#if 0
//]
	to = osip_message_get_to(ev->request);
	osip_to_to_str(to, &req_uri);	
	msg = g_strdup_printf( _( "Registration on %s sucessful." ) , req_uri ) ;
	osip_free(req_uri);
	osip_to_free(to);
	//]

	linphonec_display_something(lc, msg);
	g_free( msg ) ;
//[SD6, ericchung, exosip 3.5 integration
#endif 
//]


	cfg = linphone_core_get_proxy_config_from_rid( lc , ev->rid ) ;
	g_return_if_fail(cfg != NULL);
	if(g_pVoIPCfg->rfc_flags&SIP_FJ_CHANGE_REGISTER_TIMER){
		if(solar_ptr->bootup_slic_poweron==0){
			//enable slic power
			system("echo 125 > /proc/voip/debug");
			solar_ptr->bootup_slic_poweron=1;
		}
	}

	if(ev->response!=NULL)
	{
		osip_message_get_service_route(ev->response,0,&service_route);
		if(service_route!=NULL)
		{
			//printf("osip_message_get_service_route=%d service_route->hvalue=%s\n", i, service_route->value);
			osip_list_init(&cfg->service_routes);
			osip_list_clone(&(ev->response)->service_routes, &cfg->service_routes, (int (*)(void *, void **)) &osip_service_route_clone);
		}
	}
	osip_message_get_expires(ev->request,0,&expire_header);

	if (expire_header!=NULL && atoi(expire_header->hvalue)!=0){
		
	cfg->auth_failed = FALSE ;
	cfg->registered = TRUE ;
#ifdef BYPASS_RELAY
	cfg->timeout_count = 0;
#endif

#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
	tr104_Register_Update_Fail_Reason(lc->chid, 200,cfg);
#endif

	// update and display regitration status 
	linphone_update_and_display_registration_status( lc, cfg, REGSTATUS_REG_SUCCESS );
	
#ifdef FIX_REGISTER_TIME
	//printf("%s ev->server_expires=%d\n", __FUNCTION__, ev->server_expires);
	if ( (ev->server_expires !=0) && (ev->server_expires != cfg->expires) ) 
	{
		//printf("%s: Change cfg->expires from %d to %d\n", __FUNCTION__, cfg->expires, ev->server_expires);
		cfg->expires = ev->server_expires;
	}
#endif	

	/* check register 200ok DATE header */
	lc->register_datetime=0;

    if ((g_pVoIPCfg->rfc_flags&SIP_DATE) && (g_pVoIPCfg->sync_phone_time != 2))
	{
		osip_message_header_get_byname(ev->response,"Date",pos,&date_header);
	if (date_header){
		g_message("register success,date_header->hvalue is %s\n",date_header->hvalue);		
			rcm_recroad_register_time(lc,date_header->hvalue);
		rcm_send_fsk_date(lc->chid,date_header->hvalue);
	}
	}

	//linphone_core_set_default_proxy(lc, cfg);
	
	if(cfg->index==1){
		g_message("\r\n this is backup proxy \n");
		if(g_pVoIPCfg->rfc_flags & SIP_SERVER_REDUNDANCY){
			if(g_pVoIPCfg->rfc_flags & RETRY_MAIN_PROXY_ON){//if need retry main proxy
				g_message("\r\n  backup proxy register success, after 60s retry main proxy\n");
			rcm_timerLaunch(lc->register_retry_timer,60*1000);	//60sec
		}
		}
		
	}else{//main proxy register success
		
		if(g_pVoIPCfg->rfc_flags & SIP_SERVER_REDUNDANCY){	
			if((lc->proxies[1]!=NULL) && (lc->proxies[1]->registered==TRUE)){
				//unregister backup proxy.
				linphone_proxy_send_unregister(lc->proxies[1]);
			}
		}
		/* cancel retry register timer */
		rcm_timerCancel(lc->register_retry_timer);
			
	}

	
	if( lc->default_proxy_index == cfg ->index || lc->default_proxy == NULL )
	{
		/* 1. current is defalt proxy in flash setting */
		/* 2. no default proxy */
		linphone_core_set_default_proxy(lc, cfg);
	}

	/* if register success, send subscript, option to sip server */
	rcm_linphone_message_to_proxy(lc,cfg);
	

//#ifdef MULTI_PROXY
#if 0 //ericchung : for E8C  case , remark it  ,TBD
  #if 1	/* pkshih: new default proxy policy */
	if( lc->default_proxy_index == cfg ->index || lc->default_proxy == NULL )
	{
		/* 1. current is defalt proxy in flash setting */
		/* 2. no default proxy */
		linphone_core_set_default_proxy(lc, cfg);
	}
  #else
	if (lc->default_proxy == NULL)
	{
		linphone_core_set_default_proxy(lc, cfg);
	}
	else if (lc->default_proxy->index == lc->default_proxy_index)
	{
		if( lc->default_proxy->registered == TRUE ) {
			// default proxy in flash is first priority
		} else {
			// default proxy is not registered, so use current one temporarily. 
			linphone_core_set_default_proxy(lc, cfg);
		}
	}
	else if (cfg->index != lc->default_proxy->index)
	{
		// change to first active proxy 
		linphone_core_set_default_proxy(lc, cfg);
	}
  #endif
#endif
	// For LED
	    rcm_tapi_sip_register(lc->chid, 1);
	}else{/*expire is 0 , for unregister */
	
		cfg->registered = FALSE ;
		lc->initial_subscribes_sent=FALSE;
		linphone_update_and_display_registration_status( lc, cfg, REGSTATUS_INIT );
		rcm_tapi_sip_register(lc->chid, 0);
		if(lc->parent->bootup_unregistered==0){
			lc->parent->bootup_unregistered=1;
			g_message("change bootup_unregistered to 1\n");
			eXosip_register_remove(ev->rid);
			rcm_linphone_register_init(solar_ptr);
		}


	}
	
	UpdateLedDisplayMode( lc );
}

int linphone_find_chid_ssid(TAPP *pApp, int cid, int *p_chid, int *p_ssid)
{
	int i, j;
	LinphoneCore *lc;

	for (i=0; i<g_MaxVoIPPorts; i++)
	{
		lc = &pApp->ports[i];
		for (j=0; j<(MAX_SS+1); j++)
		{
			if (lc->call[j] &&
				lc->call[j]->cid == cid)
			{
				*p_chid = i;
				*p_ssid = j;
				return 0;
			}
		}
	}

	return -1;
}

int linphone_find_chid_ssid_terminated(TAPP *pApp, int cid, int *p_chid, int *p_ssid)
{
	int i, j;
	LinphoneCore *lc;

	for (i=0; i<g_MaxVoIPPorts; i++)
	{
		lc = &pApp->ports[i];
		for (j=0; j<MAX_SS; j++)
		{
			if (lc->call_terminated[j] &&
				lc->call_terminated[j]->cid == cid)
			{
				*p_chid = i;
				*p_ssid = j;
				return 0;
			}
		}
	}

	return -1;
}

int GetChannelFromRequest(TAPP *pApp, eXosip_event_t *ev)
{
	// TODO: check request URI
	LinphoneCore *lc;
	int i;
	GList *elem;
	osip_from_t *from,*to;
	char *tmp;
	char *to_username=NULL;
	voipCfgPortParam_t *voip_ptr;
	voipCfgProxy_t *proxy;
	char *temp=NULL;
	char uri_tmp[64];
	char Number[64];

	
	from=osip_message_get_from(ev->request);
	to=osip_message_get_to(ev->request);
	
	osip_from_to_str(from,&tmp);
	//printf("get request from is %s\n",tmp);
	osip_free(tmp);

	osip_from_to_str(to,&tmp);
	//printf("get request to is %s\n",tmp);
	osip_free(tmp);

	//username=osip_uri_get_username(ev->request->to->url);
//	printf("\r\n aa get username is %s\n",username);
		
	to_username=osip_uri_get_username(&ev->request->req_uri);
	//printf("\r\n to_username is %s\n",to_username);
	if((to_username==NULL)|| (strlen(to_username)==0)){
		voip_ptr = &g_pVoIPCfg->ports[0];
		if ((voip_ptr->fxsport_flag & FXS_REJECT_DIRECT_IP_CALL))
//		if (voip_ptr->direct_ip_call == 0)
		{
			g_warning("reject direct ip call\n");
			eXosip_lock();
			//[SD6, bohungwu, exosip 3.5 integration
			//eXosip_answer_call(did, 603, NULL);
			eXosip_call_send_answer(ev->tid, 603, NULL);
			//]
			eXosip_unlock();
			return -1;
		}
		
		return 0; //always use chid 0 , for direct ip call
	}


/* check internal call */
/*
case 1: fxs internall call
      from_url: fxs1@ip
      to_url : fxs2@ip

case 2: fxs dial to fxo (2 stage)
       from_url: fxstofxo@ip
       to_url: fxo@ip

case 3: fxs dial pstn_route_prefix+number to fxo (1 stage)
        from_url: fxstofxo@ip
        to_url: number@ip

*/

	 if (strcmp(to_username, "fxs2") == 0) {
	 	g_message("\n match fxs2, dial to chid 1\n");
		//return chid 1 => fxs 2
	 	return 1;

	 }else if(strcmp(to_username, "fxs1") == 0) {
		 g_message("\n match fxs1, dial to chid 0\n");
		return 0;
	 }
	 else if(strcmp(from->url.username,"fxstofxo")==0){
		g_message(" dial to fxo \n");
		return RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature);
	 }

	for (i=0; i<g_MaxVoIPPorts; i++){
			
		lc=&pApp->ports[i];
		
		for(elem=linphone_core_get_proxy_config_list(lc);elem!=NULL;elem=g_list_next(elem)){
			
			LinphoneProxyConfig *cfg=(LinphoneProxyConfig*) elem->data;
			
			if (cfg->registered)//already register 
			{//match
				voip_ptr = &g_pVoIPCfg->ports[cfg->lc->chid];
				proxy = &voip_ptr->proxies[cfg->index];
				//	printf("check chid %d,proxy index %d\n",cfg->lc->chid,cfg->index);
					
				//printf("check  proxy->number %s\n",proxy->number);
				//printf("check  proxy->login_id %s\n",proxy->login_id);
		
				//printf("check proxy->password %s\n",proxy->password);
				strcpy(uri_tmp,proxy->login_id);
			        temp = strtok(uri_tmp, "@");
				if( temp!=NULL ){
					strcpy(Number,temp);	
				}else{
					strcpy(Number,proxy->login_id);
				}
				
				if (proxy->number[0])
				{
				if (0 == strcmp(proxy->number, to_username)){
						g_message("match  proxy->number %s chid %d\n",proxy->number,cfg->lc->chid);
						return cfg->lc->chid;
					}else{
						g_message("proxy->number=[%s] not match any number to_username =[%s]  chid %d \n",proxy->number, to_username,cfg->lc->chid);
				}
				}
				else
				{
					if (g_pVoIPCfg ->X_CT_servertype ==SOFTSWITCH_SIP)
					{
						if (0 == strcmp(proxy->login_id, to_username)){
								g_message("SOFTSWITCH_SIP match  proxy->login_id %s chid %d\n",proxy->login_id,cfg->lc->chid);
								return cfg->lc->chid;

						}else{
							g_message(" SOFTSWITCH_SIP proxy->login_id=[%s] not match any number to_username =[%s] \n",proxy->login_id, to_username);
						}
					}
						
				}
				
				if (0 == strcmp(Number, to_username)){
					g_message("match  proxy->login_id %s chid %d\n",Number,cfg->lc->chid);
					return cfg->lc->chid;

				}else{
					g_message("proxy->login_id=[%s] not match any number to_username =[%s] \n",Number, to_username);
				}
				
//				return (LinphoneProxyConfig*)elem->data;
			}else
				g_message("not match any number chid  %d cfg not registered \n",cfg->lc->chid);
		}




	}

	g_message("not match any number \n");
	return -1;



	
	//printf("*******GetChannelFromRequest nodify modify \n");

}

int GetChIDfromOptionsTid(TAPP *pApp, eXosip_event_t *ev)
{
	LinphoneCore *lc;
	int i;

	for (i=0; i<g_MaxVoIPPorts; i++){	
		lc=&pApp->ports[i];
		if(lc->options_tid==ev->tid){
			return lc->chid;
		}			
	}

//	printf("GetChIDfromOptionsTid no match any tid \n");
	return -1;

}
#ifdef TIMECMP
#error "TIMECMP has defined!?"
#endif
#define TIMECMP(h1, m1, cmp, h2, m2) ((h1) == (h2) ? (m1) cmp (m2) : (h1) cmp (h2))
int CheckDND(int chid)
{
	if (g_pVoIPCfg->ports[chid].dnd_mode == 2)	// dnd always
	{
		return 1;
	}
	else if (g_pVoIPCfg->ports[chid].dnd_mode == 1)	// dnd period
	{
		time_t cur_time;
		struct tm *cur_tm;

		time(&cur_time);
		cur_tm = localtime(&cur_time);
		if (TIMECMP(
			g_pVoIPCfg->ports[chid].dnd_to_hour, g_pVoIPCfg->ports[chid].dnd_to_min, >,
			g_pVoIPCfg->ports[chid].dnd_from_hour, g_pVoIPCfg->ports[chid].dnd_from_min))
		{
			return TIMECMP(cur_tm->tm_hour, cur_tm->tm_min, >, 
				g_pVoIPCfg->ports[chid].dnd_from_hour, g_pVoIPCfg->ports[chid].dnd_from_min) &&
				TIMECMP(cur_tm->tm_hour, cur_tm->tm_min, <, 
				g_pVoIPCfg->ports[chid].dnd_to_hour, g_pVoIPCfg->ports[chid].dnd_to_min);
		}
		else
		{
			return !TIMECMP(cur_tm->tm_hour, cur_tm->tm_min, >, 
				g_pVoIPCfg->ports[chid].dnd_from_hour, g_pVoIPCfg->ports[chid].dnd_from_min) &&
				TIMECMP(cur_tm->tm_hour, cur_tm->tm_min, <, 
				g_pVoIPCfg->ports[chid].dnd_to_hour, g_pVoIPCfg->ports[chid].dnd_to_min);
		}
	}

	return 0;
}
#undef TIMECMP

int linphone_process_call(TAPP *pApp, int *p_chid, int *p_ssid, eXosip_event_t *ev)
{
	LinphoneCore *lc;
	const voipCfgPortParam_t *voip_ptr;


	if (linphone_find_chid_ssid(pApp, ev->cid, p_chid, p_ssid) == 0)
		return 0;


	g_message("\r\n linphone_process_call p_chid[%d]ssid[%d]\n",*p_chid,*p_ssid);

#ifdef ATTENDED_TRANSFER
		if (ev->rc != NULL)
		{
			if (linphone_find_chid_ssid(pApp,ev->rc->c_id , p_chid, p_ssid) == 0)
			{
				//step 1. terminate the 1st call				
				rcm_tapi_SetRtpSessionState( *p_chid , *p_ssid , rtp_session_inactive ) ;
				linphone_core_terminate_dialog(&pApp->ports[*p_chid], *p_ssid, NULL);
				//the call might be holded by remote, cancel the holding sound
				rcm_tapi_SetPlayTone(*p_chid, *p_ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_BOTH);
				SetSessionState(*p_chid , *p_ssid , SS_STATE_IDLE ) ;
				SetActiveSession( *p_chid , *p_ssid, FALSE ) ;
				*p_chid = GetChannelFromRequest(pApp, ev);//assign chid
				*p_ssid = (p_ssid) ? 0 : 1;
				return 0;
			}
		}
#endif


	// couldn't find match call
	// 1. new call: pre-process if need
	// 2. error
	//[SD6, bohungwu, exosip 3.5 integration
	//if (ev->type == EXOSIP_CALL_NEW)
	if (ev->type == EXOSIP_CALL_INVITE)
	//]
	{

		unsigned char szForward[ 100 ] = { 0 };
		const unsigned char *forward = szForward;

		int active_session;

		*p_chid = GetChannelFromRequest(pApp, ev);
        if(*p_chid<0){
            g_message("No channel number is matched from request\n");
            return -1;
        }
        
		lc = &pApp->ports[*p_chid];
		voip_ptr = &g_pVoIPCfg->ports[ lc ->chid ];
		
		
		if( voip_ptr ->uc_forward_enable == 1 && voip_ptr ->uc_forward[ 0 ] )
		{
			// uncoditional forward
			g_message("unconditional forward\n");
			forward = voip_ptr ->uc_forward;
		}
		else 
		{
			int bBusy;

#ifdef ATTENDED_TRANSFER
			// rock: force terminate call transfer if last transfer not terminated
			if (GetSysState(lc->chid) == SYS_STATE_TRANSFER_ONHOOK)
			{
				int i;

				for (i = 0; i<MAX_SS; i++)
					linphone_core_terminate_dialog(lc, i ,NULL);

				SetSysState(lc->chid, SYS_STATE_IDLE);
			}
#endif

			bBusy = CheckDND(lc->chid);
			if (bBusy == 0)
			switch (GetSysState(lc->chid))
			{
			case SYS_STATE_IDLE:
				*p_ssid = GetIdleSession(lc->chid);
				break;
			case SYS_STATE_CONNECT:
				active_session = GetActiveSession(lc->chid);
//ericchung : baudtac project, when enable busy forwarding, busy_forward priority > call waiting 
// todo : if uc_forward and busy_forward both enable ... need check behavior.

				if( voip_ptr ->busy_forward_enable && voip_ptr ->busy_forward[ 0 ] )
				{
					bBusy = 1;
				}
				else
				if ((!lc->parent->bAutoAnswer) && 
					(lc->sip_conf.call_waiting_enable) &&
					(lc->call[active_session]->state == LCStateAVRunning) &&
					//(lc->chid < RTK_VOIP_SLIC_NUM(g_VoIP_Feature)) &&
					RTK_VOIP_IS_SLIC_CH( lc->chid, g_VoIP_Feature ) &&
					(voip_ptr->uc_forward_enable != 2)){
					*p_ssid = GetIdleSession(lc->chid);
					}
				else
					bBusy = 1;
				break;
			case SYS_STATE_RING:
				bBusy = 1;
				break;
			case SYS_STATE_DAA_CONNECT:
				if ((g_VoIP_Feature & DAA_TYPE_MASK) == REAL_DAA_NEGO)
				{
					*p_ssid = GetIdleSession(lc->chid);
				}
				else
				{
					bBusy = 1;
				}
				break;
			default:
				bBusy = 1;
				break;
			}

			if (bBusy || (*p_ssid) < 0)
			{
				// I am busy 
				if( voip_ptr ->busy_forward_enable && voip_ptr ->busy_forward[ 0 ] )
				{
					// busy forward
					g_message("busy forward\n");
					forward = voip_ptr ->busy_forward;
				}

				else
				{
					// repsonse busy
					g_warning("busy here [%d]\n", *p_chid);
					eXosip_lock();
					//[SD6, bohungwu, exosip 3.5 integration
					//eXosip_answer_call(ev->did, 486, NULL);
					eXosip_call_send_answer(ev->tid, 486, NULL);
					//]
					eXosip_unlock();
					/*+++++add by Jack for adding busy call log+++++*/
					{
						time_t t;
						LinphoneCallLog *cl=g_new0(LinphoneCallLog,1);
						/* SD6, bohungwu, exosip 3.5 integration */
						osip_from_t * from_url = NULL ;
						osip_from_t * to_url = NULL ;
						/* Ends here*/

						cl->lc=lc;
						cl->dir=LinphoneCallIncoming;
						cl->status=LinphoneCallMissed;
						time(&t);
						cl->start_date=g_strdup(ctime(&t));
						//cl->from=g_strdup(ev->local_uri);
						from_url = osip_message_get_from(ev->request);
						if(from_url != NULL) {
							osip_from_to_str(from_url, &(cl->from));		
						}

						//cl->to=g_strdup(ev->remote_uri);
						to_url = osip_message_get_to(ev->request);
						if(to_url != NULL) {
							osip_from_to_str(from_url, &(cl->to));

						}
						

						lc->call_logs=g_list_append(lc->call_logs,(gpointer)cl);
						if (g_list_length(lc->call_logs) > lc->max_call_logs){
							GList *elem;
							elem=g_list_first(lc->call_logs);
							linphone_call_log_destroy((LinphoneCallLog*)elem->data);
#ifdef FIX_MEMORY_LEAK
							lc->call_logs=g_list_delete_link(lc->call_logs,elem);
#else
							lc->call_logs=g_list_remove_link(lc->call_logs,elem);
#endif
						}
					}
			/*-----end-----*/
					return -1;
				}
			}
		}

		if (forward[0])
		{
          	osip_message_t *msg;
          	int code=302;

			/* 
			 * Catenate forward string from 'forward' to 'szForward', 
			 * and the string will become 'sip: xxxxx@xxxxx.xxxx.xxx'.
			 */
			CallForwardByDialPlan( lc, forward, szForward, "." );
			
			/* Now, point 'forward' to 'szForward' */
			forward = szForward;			

			g_message("forward to %s", forward);

 			/* response 302, Moved Temporarily (redirect) */ 
          	eXosip_lock();
          	eXosip_call_build_answer(ev->tid,code,&msg);
          	osip_message_set_contact(msg,forward);
          	eXosip_call_send_answer(ev->tid,code,msg);
          	eXosip_unlock();

			return -1;
		}
		else
		{
			// got chid & ssid, ringing and wait answer
			return 0;
		}
	}
	else if (ev->type == EXOSIP_CALL_RELEASED)
	{
		#if 0 //eric mark 
		*p_chid = GetChannelFromRequest(pApp, ev);

		lc = &pApp->ports[*p_chid];
		g_message("+++++@@@DEBUG:EXOSIP_CALL_RELEASED,state:%d+++++\n",GetSysState(lc->chid));

		/*++Modify by Jack Chan 29/01/07 for VOIP.
			free the dialog that is not freed in eXosip_terminate_call()++*/
		/*if( NULL != ev->jd ){
			osip_dialog_free(ev->jd->d_dialog);
			ev->jd->d_dialog = NULL;
		}*/
		/*--end--*/
		// its not ok!
		// session != transaction, release later always
		#endif
		return -1;
	}else if(ev->type == EXOSIP_MESSAGE_NEW){

	
		*p_chid = GetChannelFromRequest(pApp, ev);
		if(*p_chid!=-1)
			lc = &pApp->ports[*p_chid];
		else
			return -1;

		*p_ssid = GetIdleSession(lc->chid);
		
		return 0;

	}
	else
	{
		g_warning("invalid event (%d): cid = %d, did = %d\n",
			ev->type, ev->cid, ev->did);
		return -1;
	}
}

char *tolower_clrspace(char *str)
{
	int i, j;
	char *new_str;
	
	new_str = osip_strdup(str);
	if (new_str)
	{
		for (i=0, j=0; str[i]; i++)
		{
			if (str[i] == ' ') continue;
			new_str[j++] = tolower(str[i]);
		}
		new_str[j] = 0;
	}

	return new_str;
}

void linphone_core_handle_update(LinphoneCore *lc, int chid, int ssid, eXosip_event_t *ev)
{
	char *answer;
	sdp_context_t *ctx;
	int status;
	sdp_message_t *sdp_msg;
	osip_message_t *msg=NULL;
	int i;
	int codec_changed=0;
	voipCfgProxy_t *proxy_cfg = &g_pVoIPCfg->ports[lc->chid].proxies[lc->default_proxy_index];

	//eric add , display audio_params
	StreamParams *audio_params;
	StreamParams *t38_params;
	LinphoneCall *call=lc->call[ssid];
	StreamParams old_audio_params;
	StreamParams old_t38_params;
	
	sdp_msg = eXosip_get_sdp_info(ev->request);//receive invite is request

	if(sdp_msg == NULL){
		g_message("update no sdp \n");

		eXosip_lock();
		eXosip_call_build_answer(ev->tid,200,&msg);
		msg->chid=lc->chid;
		eXosip_call_send_answer(ev->tid,200,msg);
		eXosip_unlock();

		return;
	}
	sdp_message_free(sdp_msg);	//Alex Hung, 20111114, valgrind

	/*copy old sdp params */
	audio_params = &call->audio_params ;
	t38_params = &call->t38_params;

	memset(&old_audio_params, 0, sizeof(old_audio_params));
	memcpy(&old_audio_params,audio_params,sizeof(StreamParams));
	memset(&old_t38_params, 0, sizeof(old_audio_params));
	memcpy(&old_t38_params,t38_params,sizeof(StreamParams));

	ctx = lc->call[ssid]->sdpctx;

	//get answer(sdpbody) for return response
	answer = rcm_sdp_context_get_answer_by_ev(ctx, ev);

	status = sdp_context_get_status(ctx);
	g_message("+++++DEBUG:linphone_core_handle_update:negoc_status%d+++++\n",ctx->negoc_status);

	if(488 == status || 415 == status){
		eXosip_lock();
		eXosip_call_send_answer(ev->tid,status,NULL);
		eXosip_unlock();
		g_message("+++++DEBUG:linphone_core_handle_update:status%d+++++\n",status);
		return;
	}
	if (answer == NULL)
	{
		g_warning("linphone_core_handle_update get sdp answer failed [%d] [%d] [%d]\n", 
			ev->type, ev->cid, ev->did);
		return;
	}



	//[SD6, bohungwu, exosip 3.5 integration
	//exoisp 3.5 doesn't support EXOSIP_CALL_UPDATE, EXOSIP_CALL_HOLD and EXOSIP_CALL_OFFHOLD events

	g_message("linphone_core_handle_update ctx->remotedir is %d\n",ctx->remote_dir);

	/*re-assign sdp point */
	audio_params = &call->audio_params ;
	t38_params = &call->t38_params;

//	printf("new audio ip  is %ld\n",inet_addr(audio_params->remoteaddr));

	if(memcmp(&old_audio_params,audio_params,sizeof(StreamParams))!=0){
		printf("\r\nlinphone_core_handle_update audio codec change \n");
		codec_changed=1;
	}
	if(memcmp(&old_t38_params,t38_params,sizeof(StreamParams))!=0){
		printf("\r\nlinphone_core_handle_update t38 codec change \n");
		codec_changed=1;
	}	


	if(ctx->remote_dir == SalStreamSendRecv){ //reinvite is call update or call resume.
		// update for fax issue or other
		g_message("linphone_core_handle_update[%d][%d]\n", chid, ssid);
		//lc->call[ssid]->state = LCStateUpdate;
		g_message("\r\n 001 should change linphone to LCStateUpdate ???\n");

		osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_SENDRECV);


		if (lc->call[ssid]->state == LCStateAVRunning)
		{
//			if(codec_changed){
	//			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateUpdate);
		//	}
			g_message("\r\nlinphone a linphone_core_handle_update chid[%d],ssid[%d], status %d\n",chid,ssid,lc->call[ssid]->state );
		}
		else if (lc->call[ssid]->state != LCStateBeResume)
		{
			lc->call[ssid]->state = LCStateBeResume;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state) ,LCStateBeResume);
			g_message("\r\nlinphone a linphone_core_handle_update chid[%d],ssid[%d], status %d\n",chid,ssid,lc->call[ssid]->state );
		}
	}else if(ctx->remote_dir == SalStreamSendOnly){ //reinvite for call hold
		g_message("linphone_core_handle_update for CALL_HOLD[%d][%d]: state = %d\n", chid, ssid, lc->call[ssid]->state);
		osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_RECVONLY);
		
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			//lc->call[ssid]->state = LCStateBeHold;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}else if(ctx->remote_dir == SalStreamInactive){ //reinvite for call hold
		g_message("anson: linphone_core_handle_update for CALL_HOLD[%d][%d]: state = %d\n", chid, ssid, lc->call[ssid]->state);
		osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_INACTIVE);
	
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			//lc->call[ssid]->state = LCStateBeHold;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}
			
	if (ctx->answerstr)
	{
		osip_free(ctx->answerstr);
		sdp_message_to_str(ctx->answer, &answer);
		ctx->answerstr = answer;
	}

	lc->call[ssid]->tid=ev->tid;
	i = eXosip_call_build_answer(lc->call[ssid]->tid, 200, &msg);
	if(lc->call[ssid]->proxy!=NULL){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,lc->call[ssid]->proxy->contact);
	}
	//eric.chung ,if support session timer 


	if(proxy_cfg->SessionUpdateTimer!=0){
		osip_message_set_supported(msg, "timer");
	}


	osip_message_set_allow(msg, "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO, UPDATE");


	eXosip_lock();
	if(i != 0)
	{
		eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
	}
	else
	{
		i = osip_message_set_body(msg, ctx->answerstr, strlen(ctx->answerstr));
		if(i == 0)
		{
			i = osip_message_set_content_type(msg, "application/sdp");
			if(i == 0)
			{
				char size[7];

				sprintf(size,"%i",strlen(ctx->answerstr));
				i = osip_message_set_content_length(msg, size);
				if(i == 0)
				{
							msg->chid=lc->chid;
							eXosip_call_send_answer(lc->call[ssid]->tid, 200, msg) ;
				}
				else
				{
					eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
					osip_message_free(msg);
				}
			}
			else
			{
				eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
				osip_message_free(msg);
			}
		}
		else
		{
			eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
			osip_message_free(msg);
		}
	}
	eXosip_unlock();

	if(codec_changed){
		g_message("\r\nlinphone_core_handle_update audio codec change start media streams\n");
		linphone_core_start_media_streams(lc, ssid, lc->call[ssid]);
	}

}
void linphone_core_handle_reinvite(LinphoneCore *lc, int chid, int ssid, eXosip_event_t *ev)
{
	char *answer;
	sdp_context_t *ctx;
	int status;
	sdp_message_t *sdp_msg;
	osip_message_t *msg=NULL;
	int i;
	int codec_changed=0;
	voipCfgProxy_t *proxy_cfg = &g_pVoIPCfg->ports[lc->chid].proxies[lc->default_proxy_index];
	//char szbuf[LINE_MAX_LEN];
	char * sdpmesg ;

	//eric add , display audio_params
	StreamParams *audio_params;
	StreamParams *t38_params;
	LinphoneCall *call=lc->call[ssid];
	StreamParams old_audio_params;
	StreamParams old_t38_params;
	
	sdp_msg = eXosip_get_sdp_info(ev->request);//receive invite is request

	if(sdp_msg == NULL){
		g_message("reinvite no sdp \n");

		eXosip_lock();
		eXosip_call_build_answer(ev->tid,200,&msg);
		//configure contact header
		if(lc->call[ssid]->proxy!=NULL){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,lc->call[ssid]->proxy->contact);
		}

		if(proxy_cfg->SessionUpdateTimer!=0){
			osip_message_set_supported(msg, "timer");
		}
		osip_message_set_allow(msg, "INVITE,ACK,CANCEL,BYE,UPDATE,OPTIONS,PRACK,NOTIFY,MESSAGE,REFER,INFO");
		g_message("response 200k offer for re-invite chid %d ssid %d\n",chid,ssid);
		ctx = lc->call[ssid]->sdpctx;
		sdpmesg = sdp_context_get_offer( ctx ) ;

		osip_message_set_body(msg, sdpmesg, strlen(sdpmesg));
		osip_message_set_content_type(msg, "application/sdp");
		rcm_osip_message_set_content_length(msg, strlen(sdpmesg));

		msg->chid=lc->chid;
		eXosip_call_send_answer(ev->tid,200,msg);
		eXosip_unlock();

		return;
	}

    sdp_message_free(sdp_msg);	//Alex Hung, 20111114, valgrind

	/*copy old sdp params */
	audio_params = &call->audio_params ;
	t38_params = &call->t38_params;

	memset(&old_audio_params, 0, sizeof(old_audio_params));
	memcpy(&old_audio_params,audio_params,sizeof(StreamParams));
	memset(&old_t38_params, 0, sizeof(old_audio_params));
	memcpy(&old_t38_params,t38_params,sizeof(StreamParams));

	ctx = lc->call[ssid]->sdpctx;

	//get answer(sdpbody) for return response
	answer = rcm_sdp_context_get_answer_by_ev(ctx, ev);

	status = sdp_context_get_status(ctx);
	g_message("+++++DEBUG:linphone_core_handle_reinvite:negoc_status%d+++++\n",ctx->negoc_status);

	if(488 == status || 415 == status){
		eXosip_lock();
		eXosip_call_send_answer(ev->tid,status,NULL);
		eXosip_unlock();
	g_message("+++++DEBUG:linphone_core_handle_reinvite:status%d+++++\n",status);
		return;
	}
	if (answer == NULL)
	{
		g_warning("get sdp answer failed [%d] [%d] [%d]\n", 
			ev->type, ev->cid, ev->did);
		return;
	}

#ifdef NEW_STUN
#if 0
	// stop media for stun
	if (g_pVoIPCfg->ports[lc->chid].stun_enable)
	{
		rcm_tapi_SetRtpSessionState(chid, ssid, rtp_session_inactive);
	}
#endif
#endif

#if 0 //STUN
	/* Mandy: Keep NAT entry alive */
	if (lc->net_conf.nat_address!=NULL && lc->net_conf.use_nat)
	{
		char *port;

		port = (char*)osip_malloc(10);
		if (lc->call[ssid]->t38_params.initialized)
		{
			linphone_core_set_use_info_for_stun(lc, ssid, STUN_UPDATE_T38);
			g_message("SDP get answer: The nat_t38_port is %d.\n", lc->net_conf.nat_t38_port);
			snprintf(port, 9, "%i", lc->net_conf.nat_t38_port);
		}
		else
		{
			linphone_core_set_use_info_for_stun(lc, ssid, STUN_UPDATE_RTP);
			g_message("SDP get answer: The nat_voice_port is %d.\n", lc->net_conf.nat_voice_port[ssid]);
			snprintf(port, 9, "%i", lc->net_conf.nat_voice_port[ssid]);
		}
		sdp_message_m_port_set(ctx->answer, 0, port);
		//[SD6, ericchung, rcm integration
		//sdp_message_to_str(ctx->answer, &answer);
		//osip_free(ctx->answerstr);
		//ctx->answerstr = answer;
		//]
	}
#endif

	//[SD6, bohungwu, exosip 3.5 integration
	//exoisp 3.5 doesn't support EXOSIP_CALL_UPDATE, EXOSIP_CALL_HOLD and EXOSIP_CALL_OFFHOLD events

	g_message("ctx->remotedir is %d\n",ctx->remote_dir);

	/*re-assign sdp point */
	audio_params = &call->audio_params ;
	t38_params = &call->t38_params;

//	printf("new audio ip  is %ld\n",inet_addr(audio_params->remoteaddr));

	if(memcmp(&old_audio_params,audio_params,sizeof(StreamParams))!=0){
		printf("\r\n audio codec change \n");
		codec_changed=1;
	}
	if(memcmp(&old_t38_params,t38_params,sizeof(StreamParams))!=0){
		printf("\r\n t38 codec change \n");
		codec_changed=1;
	}	

#if 0
	// TODO: fax handling
	if (ev->type == EXOSIP_CALL_FAXON)
	{
		g_message("EXOSIP_CALL_FAXON[%d][%d]\n", chid, ssid);
		lc->call[ssid]->state = LCStateBeFax;
		lc->call[ssid]->faxflag = FMS_BEFAX_RUN;
	}
	else if (ev->type == EXOSIP_CALL_FAXOFF)
	{
		g_message("EXOSIP_CALL_FAXOFF[%d][%d]\n", chid, ssid);
		lc->call[ssid]->state = LCStateBeFax;
		lc->call[ssid]->faxflag = FMS_IDLE;
	}
	else if (ev->type == EXOSIP_CALL_MODEM)
	{
		g_message("EXOSIP_CALL_MODEM[%d][%d]\n", chid, ssid);
		lc->call[ssid]->state = LCStateBeFax;
		lc->call[ssid]->faxflag = FMS_BEMODEM_RUN;
	}
#endif

	if(ctx->remote_dir == SalStreamSendRecv){ //reinvite is call update or call resume.
		// update for fax issue or other
		g_message("EXOSIP_CALL_REINVITE[%d][%d]\n", chid, ssid);
		//lc->call[ssid]->state = LCStateUpdate;
		g_message("\r\n 001 should change linphone to LCStateUpdate ???\n");

			osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_SENDRECV);


			if (lc->call[ssid]->state == LCStateAVRunning)
			{
			if(codec_changed){
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateUpdate);
			}
			g_message("\r\nlinphone a linphone_core_handle_reinvite chid[%d],ssid[ssid], status %d\n",chid,ssid,lc->call[ssid]->state );
			
			}
			else if (lc->call[ssid]->state != LCStateBeResume)
			{
				lc->call[ssid]->state = LCStateBeResume;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state) ,LCStateBeResume);
			g_message("\r\nlinphone a linphone_core_handle_reinvite chid[%d],ssid[ssid], status %d\n",chid,ssid,lc->call[ssid]->state );
		}
	}else if(ctx->remote_dir == SalStreamSendOnly){ //reinvite for call hold
		g_message("EXOSIP_CALL_REINVITE for CALL_HOLD[%d][%d]: state = %d\n", chid, ssid, lc->call[ssid]->state);
		osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_RECVONLY);
		
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			//lc->call[ssid]->state = LCStateBeHold;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}else if(ctx->remote_dir == SalStreamInactive){ //reinvite for call hold
		g_message("EXOSIP_CALL_REINVITE for CALL_HOLD[%d][%d]: state = %d\n", chid, ssid, lc->call[ssid]->state);
		osip_negotiation_sdp_message_put_media_mode(ctx->answer, MEDIA_INACTIVE);
	
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			//lc->call[ssid]->state = LCStateBeHold;
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}
			
	if (ctx->answerstr)
	{
		osip_free(ctx->answerstr);
		sdp_message_to_str(ctx->answer, &answer);
		ctx->answerstr = answer;
	}

	lc->call[ssid]->tid=ev->tid;
	i = eXosip_call_build_answer(lc->call[ssid]->tid, 200, &msg);
	if(lc->call[ssid]->proxy!=NULL){
	_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
	osip_message_set_contact(msg,lc->call[ssid]->proxy->contact);
	}
	//eric.chung ,if support session timer 


	if(proxy_cfg->SessionUpdateTimer!=0){
		//for re-invite response , not need here code
		//sprintf(szbuf, "%d", proxy_cfg->SessionUpdateTimer);
		//osip_message_set_header(msg, "Session-expires",  szbuf);
		//osip_message_set_header(msg, "Min-SE", "90");
	osip_message_set_supported(msg, "timer");
	}


	osip_message_set_allow(msg, "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO, UPDATE");


	eXosip_lock();
	if(i != 0)
	{
		eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
	}
	else
	{
		i = osip_message_set_body(msg, ctx->answerstr, strlen(ctx->answerstr));
		if(i == 0)
		{
			i = osip_message_set_content_type(msg, "application/sdp");
			if(i == 0)
			{
				char size[7];

				sprintf(size,"%i",strlen(ctx->answerstr));
				i = osip_message_set_content_length(msg, size);
				if(i == 0)
				{
							msg->chid=lc->chid;
							eXosip_call_send_answer(lc->call[ssid]->tid, 200, msg) ;
				}
				else
				{
					eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
					osip_message_free(msg);
				}
			}
			else
			{
				eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
				osip_message_free(msg);
			}
		}
		else
		{
			eXosip_call_send_answer(lc->call[ssid]->tid, 400, NULL);
			osip_message_free(msg);
		}
	}
	eXosip_unlock();


}

void linphone_core_process_event(TAPP *pApp, eXosip_event_t * ev )
{
	int chid = -1, ssid = -1;
	LinphoneCore *lc = NULL;
	int bTransferring = 0;
	int ret=0;
	sdp_message_t *sdp_msg;
	//char *sdp_body_str;
	int pos=0;
	osip_header_t *event_header=NULL;	
	osip_header_t *SubscriptionState_header=NULL;	
	
	g_message("\r\n linphone_core_process_event ev->type is %d ev->rid %d\n",ev->type,ev->rid);

	if(pApp==NULL){
		g_warning("\r\n linphone_core_process_event pApp is null\n");
		return;
	}

	g_message("get ev->type %d\n",ev->type);
	g_message("get ev->tid %d\n",ev->tid);
	g_message("get ev->rid %d\n",ev->rid);
	g_message("get ev->sid %d\n",ev->sid);
	g_message("get ev->nid %d\n",ev->nid);
	g_message("get ev->did %d\n",ev->did);
	g_message("get ev->cid %d\n",ev->cid);

	g_message("get ev->textinfo %s\n",ev->textinfo);

	if (ev->type >= EXOSIP_REGISTRATION_NEW && 
		ev->type <= EXOSIP_REGISTRATION_TERMINATED)
	{
		int i;
		LinphoneProxyConfig *cfg;
	
		for (i=0; i<g_MaxVoIPPorts; i++)
		{
			cfg = linphone_core_get_proxy_config_from_rid(&pApp->ports[i], ev->rid);
			if (cfg != NULL)
			{
				g_message("find lc index %d cfg->rid is %d\n",i, cfg->rid);
				lc = &pApp->ports[i];
				g_message("find lc->chid %d\n", lc->chid);
				break;
			}
		}

		if (lc == NULL)
		{
			//[SD6, ericchung, exosip 3.5 integration
			//eXosip_event_free(ev);
			//]
			return;
		}
		else {
			g_message("222 find lc-> chid %d\n", lc->chid);
		}
	}
	//[SD6, bohunguw, exosip 3.5 integration
	//EXOSIP_CALL_REFER_REQUESTFAILURE and EXOSIP_INFO_NEW are not supported.
	//else if ((ev->type >= EXOSIP_CALL_NOANSWER && ev->type <= EXOSIP_CALL_REFER_REQUESTFAILURE)
	//	|| (ev->type == EXOSIP_INFO_NEW))
	else if ((ev->type >= EXOSIP_CALL_NOANSWER && ev->type <= EXOSIP_CALL_TIMEOUT)
		|| (ev->type == EXOSIP_MESSAGE_NEW))
	{
//[SD6, ericchung, exosip 3.5 integration
#if 0
		if (linphone_process_call(pApp, &chid, &ssid, ev) < 0)
		{
			// rock: handle BYE-auth after On-Hook
			if (linphone_find_chid_ssid_terminated(pApp, ev->cid, &chid, &ssid) == 0)
			{
				lc = &pApp->ports[chid];
				if ((ev->type == EXOSIP_CALL_REQUESTFAILURE) &&
				//[SD6, bohungwu, exosip 3.5 integration
				//	(ev->status_code == 401 || ev->status_code == 407))
					(osip_message_get_status_code(ev->response) == 401 || osip_message_get_status_code(ev->response) == 407))
				//]
				{
					//[SD6, bohungwu, exosip 3.5 integration
					//if (linphone_call_authentication_required(lc, lc->call_terminated[ssid]) != 0)
					if (linphone_call_authentication_required(lc, lc->call_terminated[ssid], ev) != 0)
					//]
						linphone_core_close_terminate_call(lc, ssid);
				}
				//[SD6, bohunguw, exosip 3.5 integration
				//else if (ev->type == EXOSIP_CALL_EXIT)
				else if (ev->type == EXOSIP_CALL_CLOSED)
				//]
				{
					linphone_core_close_terminate_call(lc, ssid);
				}
			}

			eXosip_event_free( ev ) ;
			return;
		}

		if (chid != ev->nPort)
			g_message("call is not match with event!\n");
		else
			lc = &pApp->ports[chid];
//[SD6, ericchung, exosip 3.5 integration
#endif //#if 0

	}
#ifdef SIP_PING
	else if (ev->type == EXOSIP_PING_SUCCESS || ev->type == EXOSIP_PING_CLOSED)
	{
		lc = &pApp->ports[ev->nPort];
	}
#endif
#if 0
	if (lc == NULL)
	{
		g_warning("counldn't find match call\n");
	}
#endif
	switch( ev->type )
	{
		case EXOSIP_CALL_ANSWERED : /*receive 200ok for outgoing call (INVITE) , must return ACK */

			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_ANSWERED linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
			
			lc = &pApp->ports[chid];

			g_message( "EXOSIP_CALL_ANSWERED[%d][%d]\n" , chid , ssid ) ;

			rcm_timerCancel(lc->ringback_timeout_timer); 
			
			if (GetSysState(lc->chid) == SYS_STATE_TRANSFERRING)
				bTransferring = 1;

			// rock add:
			// avoid auto-answer skip ringing problem
			if (lc->ringstream[ssid] == FALSE) {
				linphone_call_proceeding(lc, ssid, ev->cid, ev->did);
				lc->call[ssid]->tid = ev->tid;
			}
			rcm_exosip_call_ack(lc,ssid,ev);

		#ifdef CHECK_RESOURCE
		//run here
			/* [SD6, bohungwu, exosip 3.5 integration */
			{
#if 0 //ericchung , not need sdp here.	
				sdp_message_t *sdp_msg;
				char *sdp_body_str;

				sdp_msg = eXosip_get_sdp_info(ev->response);
				if(sdp_msg!=NULL){
					sdp_message_to_str(sdp_msg, &sdp_body_str);
					sdp_message_free(sdp_msg);	
				}else{
					printf("no sdp !!\n");
				}
#endif
				//if (linphone_call_accepted(lc, ssid, ev->did, ev->sdp_body) == -1)
				//[SD6, ericchung, exosip 3.5 integration
				//if (linphone_call_accepted(lc, ssid, ev->did, sdp_body_str) == -1)
				if (linphone_call_accepted(lc, ssid, ev) == -1)
				{
					//free(sdp_body_str);
				break;
				}
			}
			//]

		#else
			//[SD6, ericchung, exosip 3.5 integration
			//linphone_call_accepted(lc, ssid, ev->did, ev->sdp_body);
			//old api
			linphone_call_accepted(lc, ssid, ev);
		#endif
			if (!(g_pVoIPCfg->rfc_flags & FXS_REVERSE_POLARIY_OFF))
			{
				g_message("\nset FXS_REVERSE_POLARIY \n");
				rtk_OffHookAction(lc->chid);
			}
					

/*+++added by Jack for Planet transfer+++*/
#ifdef PLANET_TRANSFER
			if(eXosip.bIVR_transfer){
				if(GetSysState(lc->chid) == SYS_STATE_TRANSFER_EDIT){
					int another_session;
			
					another_session = ssid ? 0 : 1;
					rcm_tapi_SetPlayTone(lc->chid , another_session, DSPCODEC_TONE_DIAL, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
				}
			}
#endif /*PLANET_TRANSFER*/
			/*-----end-----*/
		#if defined(ATTENDED_TRANSFER)
			if (bTransferring)
			{
				int hs; // hold session

				hs = (ssid == 0) ? 1 : 0;
				if (lc->call[hs] && lc->call[hs]->state != LCStateInit)
				{
					//[SD6, bohugnwu, exosip 3.5 integration
					//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 200 OK");
					rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 200 OK");
					//]
				}
			}
		#endif
/* Ericchung , TR104 auto test */
#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
			/* caller test , start send dtmf to peer */
			g_message("e8c_autotest port id =%d \n",chid);
			g_message("e8c_autotest.enable is %d\n",rcm_tr104_st.e8c_autotest[chid].enable);
			g_message("e8c_autotest.TestType is %d\n",rcm_tr104_st.e8c_autotest[chid].TestType);
			g_message("e8c_autotest.TestSelector is %d\n",rcm_tr104_st.e8c_autotest[chid].TestSelector);
			if((rcm_tr104_st.e8c_autotest[chid].enable==1) && (rcm_tr104_st.e8c_autotest[chid].TestType==0) &&(rcm_tr104_st.e8c_autotest[chid].TestSelector==1)){
//				if(rcm_tr104_st.e8c_autotest[chid].DailDTMFConfirmEnable==1){
					g_message("send dtmf %s to peer \n",rcm_tr104_st.e8c_autotest[chid].DailDTMFConfirmNumber);
					rtk_SetDtmfCfg(lc->chid, 1, 1);
					rcm_tr104_st.e8c_autotest[chid].status=E8C_TEST_DIAL_DTMF;
					rcm_timerLaunch(lc->e8c_tr104_timer,1000); //after 1 sec start test.
//				}
			}
#endif
			break ;
		case EXOSIP_CALL_CLOSED :
		case EXOSIP_CALL_CANCELLED :
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n close,cancel linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
			lc = &pApp->ports[chid];
		{
			int another_session;
			SESSIONSTATE another_session_state;

			if( ev->type == EXOSIP_CALL_CLOSED )
				g_message( "EXOSIP_CALL_CLOSED[%d][%d]\n" , chid , ssid ) ;
			else
				g_message( "EXOSIP_CALL_CANCELLED[%d][%d]\n" , chid , ssid ) ;

			if( (three_way_on_server(chid))&&(lc->call[2]!=NULL))
			{
				g_message("linphone_call_terminated_3way_on_server ,ssid =%d\n",ssid);
				linphone_call_terminated_3way_on_server(lc, ssid, ev);
			}
			else
            {
			linphone_call_terminated(lc, ssid, ev);
            }

			rcm_timerCancel(lc->no_Answer_timer_id);
			if (GetSysState(chid) == SYS_STATE_IDLE)
			{
				// no answer forward: break here to avoid busy tone 
				break;
			}

			rcm_tapi_SetRingFXS( chid , FALSE ) ;	
			SetSessionState( chid , ssid , SS_STATE_IDLE ) ;

			another_session = ssid ? 0 : 1;
			another_session_state = GetSessionState( chid , another_session ) ;
			// check another session state
			// close		| another
			// n/a			| A   (Idle, CallIn, CallOut, Caller, Callee)
			// A			| n/a (Idle, CallIn, CallOut, Caller, Callee)
			if (GetSysState(chid) == SYS_STATE_CONNECT_DAA_RING)
			{
				SetActiveSession(lc->chid, 0, TRUE);
				if (rcm_tapi_DAA_off_hook(lc->chid) == 0xff)
				{
					/* PSTN line not connect or line busy */
					SetSysState(lc->chid, SYS_STATE_EDIT_ERROR);
				    rcm_tapi_SetPlayTone(lc->chid, 0, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				}
				else
				{
					SetSysState(lc->chid, SYS_STATE_DAA_CONNECT);
					SetSessionState(lc->chid, 0, SS_STATE_CALLER);
				}
			}
			/*++added by Jack Chan  for transfer++*/
			else if (GetSysState(chid) == SYS_STATE_TRANSFERRING||GetSysState(chid) == SYS_STATE_TRANSFER_FINISH)
			{
				g_message("system state SYS_STATE_TRANSFERRING\n");
			}
			/*--end--*/
#ifdef ATTENDED_TRANSFER
			else if (GetSysState(chid) == SYS_STATE_TRANSFER_ONHOOK)
			{
				// rock: check call transfer done ?
				if (lc->call[0] || lc->call[1])
				{
					// there are still 2 calls, after this bye, this is still one , so keep in this state.
					g_message("system state SYS_STATE_ATT_TRANSFR_ONHOOK\n");
				}
				else
				{
					// all sessions are closed
					g_message("system state SYS_STATE_IDLE\n");
					rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);	
					SetSysState(chid, SYS_STATE_IDLE);
				}
			}
#endif			
			else if (GetActiveSession(chid) == -1)
			{
				// missed call, do nothing here
				SetSysState(chid, SYS_STATE_IDLE);				
			}
			else if (lc->parent->bAutoAnswer)
			{
				// auto answer mode will be in on-hook status always
				SetSysState(chid, SYS_STATE_IDLE);
			}
			else if (GetSysState(chid) == SYS_STATE_IN3WAY)
			{
				if (three_way_on_server(chid))
				{
					if(ssid==2)
					{
						rcm_tapi_SetPlayTone( chid , 0 , DSPCODEC_TONE_BUSY , TRUE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
						g_message("system state SYS_STATE_IN3WAY\n");
						SetSysState(chid, SYS_STATE_IDLE);
					}
					g_message("system state SYS_STATE_IN3WAY\n");
				}
                else
                {
				TstVoipMgr3WayCfg stVoipMgr3WayCfg;

				SetSysState(chid, SYS_STATE_CONNECT);
				
				memset(&stVoipMgr3WayCfg, 0, sizeof(stVoipMgr3WayCfg));
				stVoipMgr3WayCfg.ch_id = chid;
				stVoipMgr3WayCfg.enable = FALSE;
				GetRtpCfg(&stVoipMgr3WayCfg.rtp_cfg[another_session], lc, another_session);
				rcm_tapi_SetConference(&stVoipMgr3WayCfg);
				SetActiveSession(chid, another_session, TRUE); 
				//linphone_core_stop_media_streams(lc, another_session);
			#ifdef CHECK_RESOURCE
				if (linphone_core_start_media_streams(lc, another_session, lc->call[another_session]) == -1)
				{
					linphone_core_terminate_dialog(lc, another_session, NULL);
					break;
				}
			#else
				linphone_core_start_media_streams(lc, another_session, lc->call[another_session]);
			#endif
			}
			}
			else if (GetActiveSession(chid) == another_session)
			{
				// closed session is not active
				switch (another_session_state)
				{
				case SS_STATE_IDLE:
					// active session is idle: playing dial tone
					SetSysState( chid , SYS_STATE_EDIT ) ;
					break;
				case SS_STATE_CALLIN:
					// active session is callin: never occur!
					g_error("process_event: SS_STATE_CALLIN in active session\n");
					break;
				case SS_STATE_CALLOUT:
					// active session dial out: maybe play remote ring
					SetSysState( chid , SYS_STATE_EDIT ) ;
					break;
				default: // caller or callee
					// active session is not connected: 
					// 1. close hold session 
					// 2. close pre-conference
					SetSysState( chid , SYS_STATE_CONNECT ) ;
					// close call waiting tone
					rcm_tapi_SetPlayTone(chid, another_session, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
					break;
				}
			}
			else
			{
				// closed session is active
				switch (another_session_state)
				{
				case SS_STATE_IDLE:
					// another session is idle
				
						rcm_tapi_SetPlayTone( chid , ssid , DSPCODEC_TONE_BUSY , TRUE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
					/* start timer to play wrong tone , if user forget offhook phone */
					rcm_timerLaunch(lc->offhook_timer,((g_pVoIPCfg->BusyToneTimer)*1000)); //play wrong tone
					rcm_timerLaunch(lc->stop_alltone_timer,((g_pVoIPCfg->BusyToneTimer+g_pVoIPCfg->HangingReminderToneTimer)*1000)); 
					SetSysState(chid, SYS_STATE_EDIT_ERROR);
					break;
				case SS_STATE_CALLIN:
					// another session is callin (in call waiting/caller id)
					SetSysState( chid , SYS_STATE_RING ) ;
					//stop call waiting tone
					rcm_tapi_SetPlayTone( lc->chid , ssid, DSPCODEC_TONE_CALL_WAITING , FALSE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
			
					//printf("linphone_core_accept_dialog 003\n");
					linphone_core_accept_dialog(lc, another_session, NULL);
					// switch to another(new) session
					SetActiveSession( chid , another_session, TRUE ) ;
					break;
				case SS_STATE_CALLOUT:
					// another session is callout
					SetSysState( chid , SYS_STATE_EDIT ) ;
					// switch to another(new) session
					SetActiveSession( chid , another_session, TRUE ) ;
					if (lc->ringstream[another_session]) // if remote ringing
						rcm_tapi_SetPlayTone( chid , another_session , DSPCODEC_TONE_RINGING , TRUE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
					break;
				default: // caller or callee
					// another session is connected: 
					// 1. close active session 
					// 2. close pre-conference
					// 3. resume hold session
					if( (three_way_on_server(chid))&&(lc->call[2]!=NULL)){					
						g_message("3 way ongoing receive BYE and another session in caller or callee state\n");
					}else{
					SetSysState( chid , SYS_STATE_CONNECT ) ;
					// switch to active session
					rcm_tapi_SetPlayTone( lc->chid , ssid, DSPCODEC_TONE_CALL_WAITING , FALSE , DSPCODEC_TONEDIRECTION_LOCAL ) ;
					printf(" switch to active session \n");
					linphone_core_offhood_event(lc, another_session);
					}
					break;
				}
			}
			rtk_StopCid(chid,2);
			rcm_tapi_SetRingFXS( chid , FALSE ) ;	
#if defined(CONFIG_RTK_VOIP_TR104) && defined(CONFIG_E8B)
			if((rcm_tr104_st.e8c_autotest[chid].enable==1) ){				
					g_message("receive bye end auto call test\n");
					rcm_tr104_st.e8c_autotest[chid].status=E8C_TEST_CMPDTMF;
					rcm_timerLaunch(lc->e8c_tr104_timer,100);
				
			}		

#endif
		}
			break;
		case EXOSIP_CALL_TIMEOUT :
		case EXOSIP_CALL_NOANSWER :
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_TIMEOUT/NOANSWER  linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
			
			lc = &pApp->ports[chid];
			if( ev->type == EXOSIP_CALL_TIMEOUT )
				g_message( "EXOSIP_CALL_TIMEOUT[%d][%d]\n" , chid , ssid ) ;
			else
				g_message( "EXOSIP_CALL_NOANSWER[%d][%d]\n" , chid , ssid ) ;
			linphone_call_failure(lc, ssid, -110, ev);
			break;
		case EXOSIP_CALL_REQUESTFAILURE :
		case EXOSIP_CALL_GLOBALFAILURE :
		case EXOSIP_CALL_SERVERFAILURE :

			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n 	EXOSIP_CALL_xxx_FAILURE  return %d chid %d ssid %d\n",ret,chid,ssid);		
			if(ret==-1)
				break;
			lc = &pApp->ports[chid];
			
			if( ev->type == EXOSIP_CALL_REQUESTFAILURE )
				g_message( "EXOSIP_CALL_REQUESTFAILURE[%d][%d]\n" , chid , ssid ) ;
			else if( ev->type == EXOSIP_CALL_GLOBALFAILURE )
				g_message( "EXOSIP_CALL_GLOBALFAILURE[%d][%d]\n" , chid , ssid ) ;
			else
				g_message( "EXOSIP_CALL_SERVERFAILURE[%d][%d]\n" , chid , ssid ) ;
			//[SD6, bohungwu, exoisp 3.5 integration 
			//linphone_call_failure(lc, ssid, ev->did, ev->status_code);
			if(lc==NULL)
				g_message("lc is null\n");

			g_message("status code is %d\n",osip_message_get_status_code(ev->response));

			if((lc->call[ssid]->faxflag!=FMS_IDLE)&&(g_pVoIPCfg->rfc_flags & SIP_FAX_AUTO))
			{
				printf("\n\n\n In fax reinvite recevie error response\n \n \n ");
			}else
				linphone_call_failure(lc, ssid, osip_message_get_status_code(ev->response), ev);
	
			break ;

		case EXOSIP_CALL_INVITE : /* receive incoming call INVITE */
			//g_message( "EXOSIP_CALL_NEW[%d][%d]\n" , chid , ssid ) ;
			g_message( "EXOSIP_CALL_INVITE[%d][%d]\n" , chid , ssid ) ;

			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n 	linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);		
			if(ret==-1)
			break ;
			lc = &pApp->ports[chid];

			/* Mandy add for stun support */
			//[SD6, ericchung, exosip 3.5 integration
			//linphone_core_set_use_info_for_stun(lc, -1, STUN_UPDATE_SIP); // update RTP in 200 ok
			//]

			/* CALL_NEW is used twice in qos mode : 
			 * when you receive invite (textinfo = "With QoS" or "Without QoS")
			 * and when you receive update (textinfo = "New Call") */
			linphone_inc_new_call(lc, ssid, ev);
#if defined(CONFIG_RTK_VOIP_TR104) && defined(CONFIG_E8B)

			/* ericchung  TR104 auto test , callee test*/
			if((rcm_tr104_st.e8c_autotest[chid].enable==1) && (rcm_tr104_st.e8c_autotest[chid].TestType==1)){
				/* offhook to answer */				
					g_message("start callee test off hook after 2 sec");
					rcm_tr104_st.e8c_autotest[chid].status=E8C_TEST_START;
					rcm_timerLaunch(lc->e8c_tr104_timer,2*1000); //after 2 sec start test.
				
			}		

#endif
			break ;
		case EXOSIP_CALL_ACK :
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n 	linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);		
			if(ret==-1)
				break;
			
			lc = &pApp->ports[chid];

			g_message( "EXOSIP_CALL_ACK[%d][%d]\n" , chid , ssid ) ;

			//ericchung: add delayed offer to support handle ack + sdp		

			sdp_msg = eXosip_get_sdp_info(ev->ack);
			if(sdp_msg!= NULL) {
				g_message( "ACK have SDP, must update media\n");
			
				rcm_sdp_context_read_answer_by_sdp(	lc->call[ssid]->sdpctx , ev->ack);	

				// sdp_message_to_str(sdp_msg, &sdp_body_str);
				// printf("sdp: %s\n",sdp_body_str);
				sdp_message_free(sdp_msg);
				linphone_core_stop_media_streams(lc, ssid);
				linphone_core_start_media_streams(lc, ssid, lc->call[ssid]);
				//linphone_core_handle_reinvite_ack_sdp(lc,chid,ssid,lc->call[ssid]->sdpctx);
				rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);

			}

		#if 1
			// its can be 
			// 1. UA response ok
			// 2. UA response re-invite (hold, resume, fax..)
			switch (lc->call[ssid]->state)
			{
			case LCStateRinging:
			{

				switch (GetSysState(chid))
				{
				case SYS_STATE_CONNECT:
					SetSysState(chid, SYS_STATE_IN2CALLS_IN);
					break;
				default:
					SetSysState(chid, SYS_STATE_CONNECT);
					break;
				}

				SetSessionState( chid , ssid , SS_STATE_CALLEE ) ;
				SetActiveSession( chid, ssid, TRUE );
			#ifdef CHECK_RESOURCE
				if (linphone_core_start_media_streams(lc, ssid, lc->call[ssid]) == -1) // move down for AC
				{
					linphone_core_terminate_dialog(lc, ssid, NULL);
					break;
				}
			#else
				linphone_core_start_media_streams(lc, ssid, lc->call[ssid]); // move down for AC
			#endif
				break;
			}
			case LCStateBeHold:
				//lc->call[ssid]->state = LCStateBeHoldACK;
				SetCallState(lc->chid,ssid,&(lc->call[ssid]->state) ,LCStateBeHoldACK);

				//rcm_tapi_SetRtpSessionState(chid, ssid, rtp_session_recvonly);
				linphone_core_stop_media_streams(lc, ssid);
				if ((GetSysState(chid) != SYS_STATE_IN3WAY) && (GetActiveSession(chid) == ssid))
					rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HOLD, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				break;
			case LCStateBeResume:

				//lc->call[ssid]->state = LCStateAVRunning;
				SetCallState(chid,ssid,&(lc->call[ssid]->state),LCStateAVRunning);

				g_message("\r\nlinphone-state aa chid[%d],ssid[ssid], status %d\n",chid,ssid,lc->call[ssid]->state );

				//rcm_tapi_SetRtpSessionState(chid, ssid, rtp_session_sendrecv);
			#ifdef CHECK_RESOURCE
				if (linphone_core_start_media_streams(lc, ssid, lc->call[ssid]) == -1)
				{
					linphone_core_terminate_dialog(lc, ssid, NULL);
					break;
				}
			#else
				linphone_core_start_media_streams(lc, ssid, lc->call[ssid]);
			#endif
				rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
#if 0			
				if (GetSysState(chid) != SYS_STATE_IN3WAY && GetActiveSession(chid) != ssid)
				{
					LinphoneCall *call = lc->call[ssid];	

					// let me hold peer device
					call->state = LCStateHold; // start hold
					call->sdpctx->version++;
					eXosip_lock();
					eXosip_on_hold_call(call->did, call->sdpctx->version);
					eXosip_unlock();
				}
#endif			
				digitcode_init(lc);
				break;
			case LCStateUpdate:
			case LCStateBeFax:
				// reinvite (fax...)
				//lc->call[ssid]->state = LCStateAVRunning;
				SetCallState(chid,ssid,&(lc->call[ssid]->state),LCStateAVRunning);
				//linphone_core_stop_media_streams(lc, ssid);
			#ifdef CHECK_RESOURCE
				if (linphone_core_start_media_streams(lc, ssid, lc->call[ssid]) == -1)
				{
					linphone_core_terminate_dialog(lc, ssid, NULL);
					break;
				}
			#else
				linphone_core_start_media_streams(lc, ssid, lc->call[ssid]);
			#endif
				break;
			default:
				// do what?
				break;
			}
		#endif
			if (lc->call[ssid]->MP_IP_CALL)
			{
				TstVoipToneCfg MP_tone             = {0, 0, 1, 2000,    0,   0,     0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0};
				rtk_SetUpdateTone(TONE_CHINA, DSPCODEC_TONE_EXTEND_1,	&MP_tone);
				
				rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_EXTEND_1, TRUE, DSPCODEC_TONEDIRECTION_BOTH);
			}
			break ;
		case EXOSIP_CALL_REDIRECTED :
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_REDIRECTED  linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
			
			lc = &pApp->ports[chid];
			
			g_message( "EXOSIP_CALL_REDIRECTED[%d][%d]\n" , chid , ssid ) ;
			if (lc->audiostream[ssid])
				linphone_core_stop_media_streams(lc, ssid);
			
			eXosip_default_action(ev);
			
			//]	
			break ;
		case EXOSIP_CALL_PROCEEDING :
			
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_PROCEEDING linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;

	
			g_message( "EXOSIP_CALL_PROCEEDING[%d][%d]\n" , chid , ssid ) ;
			lc = &pApp->ports[chid];
				
			linphone_call_proceeding(lc, ssid, ev->cid, ev->did);
			//[SD6, ericchung, exosip 3.5 integration
			lc->call[ssid]->tid = ev->tid;

			break ;
		case EXOSIP_CALL_RINGING :
			ret=linphone_process_call(pApp, &chid, &ssid, ev);

			g_message("\r\n EXOSIP_CALL_RINGING linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;

			lc = &pApp->ports[chid];

			
			g_message( "EXOSIP_CALL_RINGING[%d][%d]\n" , chid , ssid ) ;
			linphone_call_ringing(lc, ssid, ev);
			break ;
		case EXOSIP_IN_SUBSCRIPTION_NEW :
			g_message( "EXOSIP_IN_SUBSCRIPTION_NEW\n" ) ;
			eXosip_message_send_answer(ev->tid,501,NULL);
#if 0
			linphone_subscription_new( lc , ev->did , ev->nid , ev->remote_uri , ev->remote_contact ) ;
#endif
			break ;
		case EXOSIP_SUBSCRIPTION_NOTIFY :
			g_message( "EXOSIP_SUBSCRIPTION_NOTIFY\n" ) ;
			g_message("\r\n TBD need handle Notify \n");
			#if 0
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
				
			g_message("\r\n EXOSIP_SUBSCRIPTION_NOTIFY linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
		
			lc = &pApp->ports[chid];
			g_message( "EXOSIP_SUBSCRIPTION_NOTIFY[%d][%d]\n" , chid , ssid ) ;



			rcm_linphone_notify_rec(lc,ssid,ev);
			#else
			for (chid=0; chid<g_MaxVoIPPorts; chid++)
			{
				lc = &pApp->ports[chid];

				if (lc==NULL)
				{
					g_message("lc==NULL \n");
					break;
				}

				if (lc->reg_sid==ev->sid)//event reg
				{
					notify_event_reg_rec(lc,ssid,ev);

				}else if (lc->sid==ev->sid)//event ua-profile
				{
					rcm_linphone_notify_rec(lc,ssid,ev);
				}else//unknow event 
					g_message("SUBSCRIPTION_NOTIFY unknow event \n");
			}
			eXosip_message_send_answer(ev->tid,200,NULL);
			#endif
			
			break ;
		case EXOSIP_SUBSCRIPTION_ANSWERED :
			g_message( "EXOSIP_SUBSCRIPTION_ANSWERED\n" ) ;
#if 0
			linphone_subscription_answered( lc , ev->remote_uri , ev->did ) ;
#endif
			break ;
		case EXOSIP_SUBSCRIPTION_CLOSED :
			g_message( "EXOSIP_SUBSCRIPTION_CLOSED\n" ) ;
#if 0
			linphone_subscription_closed( lc , ev->remote_uri , ev->did ) ;
#endif
			break ;
		//[SD6, bohungwu, exoisp 3.5 integration 
		//case EXOSIP_CALL_STARTAUDIO :
		//	g_message( "EXOSIP_CALL_STARTAUDIO[%d][%d]\n" , chid , ssid ) ;
		//	break ;
		//]
		case EXOSIP_CALL_RELEASED :
			//[SD6, ericchung, exosip 3.5 integration
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_RELEASED linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;

			lc = &pApp->ports[chid];
			g_message( "EXOSIP_CALL_RELEASED[%d][%d]\n" , chid , ssid ) ;
			linphone_call_released(lc, ssid, ev->cid);
			if (chid != -1 && ssid !=-1 &&
				GetSysState(lc->chid) == SYS_STATE_TRANSFERRING)
			{
				int hs; // hold session

				hs = (ssid == 0) ? 1 : 0;
				if (lc->call[hs] && lc->call[hs]->state != LCStateInit)
				{
					SetActiveSession(lc->chid, hs, TRUE);

					SetSysState(lc->chid, SYS_STATE_CONNECT);
					//[SD6, bohungwu, exosip 3.5 integration
					//eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 400 Bad Request");
					rcm_eXosip_transfer_send_notify(lc->call[hs]->did, EXOSIP_SUBCRSTATE_TERMINATED, "SIP/2.0 400 Bad Request");
					//]
				}
			}
			break ;
		case EXOSIP_REGISTRATION_FAILURE :
			g_message( "EXOSIP_REGISTRATION_FAILURE\n" ) ;
			linphone_registration_faillure( lc , ev ) ;
			break ;
		case EXOSIP_REGISTRATION_SUCCESS :
			g_message( "EXOSIP_REGISTRATION_SUCCESS\n" ) ;
			linphone_registration_success( lc , ev ) ;
			break ;
		case EXOSIP_MESSAGE_NEW :
			g_message( "EXOSIP_MESSAGE_NEW\n" ) ;
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			g_message("\r\n EXOSIP_MESSAGE_NEW linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			/* no dialog, use default lc for response*/
			if (ret==-1){
				lc = &pApp->ports[0];
			}else{
				lc = &pApp->ports[chid];
			}
			if(MSG_IS_NOTIFY(ev->request)){		
				g_message("lc->reg_sid %d\n",lc->reg_sid);
				g_message("ev->sid %d\n",ev->sid);
									
			}

			
			rcm_process_other_request(lc,ssid,ev);
			break ;

		case EXOSIP_CALL_MESSAGE_NEW:

			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			
			g_message("\r\n EXOSIP_CALL_MESSAGE_NEW linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;

			lc = &pApp->ports[chid];
			g_message( "EXOSIP_CALL_MESSAGE_NEW[%d][%d]\n" , chid , ssid ) ;

			if (ev->request){
				if (MSG_IS_INFO(ev->request)){
					rcm_linphone_process_sipinfo(lc,ssid,ev);
	
				}else if(MSG_IS_REFER(ev->request)){
					rcm_linphone_process_refer(lc,ssid,ev);					

				}else if(MSG_IS_UPDATE(ev->request)){
					linphone_core_handle_update(lc, chid, ssid, ev);
				}else if(MSG_IS_PRACK(ev->request)){ //add for prack 
					osip_message_t *ans=NULL;
					eXosip_lock();
					eXosip_call_build_answer(ev->tid,200,&ans);
					if (ans)
					{
						ans->chid=lc->chid;
						eXosip_call_send_answer(ev->tid,200,ans);
					}
					eXosip_unlock();					
				}else if (MSG_IS_OPTIONS(ev->request)){
					osip_message_t *ans=NULL;
					eXosip_lock();
					eXosip_call_build_answer(ev->tid,200,&ans);
					if (ans){
						osip_message_set_allow(ans,"INVITE, ACK, BYE, CANCEL, OPTIONS, MESSAGE, SUBSCRIBE, NOTIFY, INFO, UPDATE");
						osip_message_set_accept(ans,"application/sdp");
						ans->chid=lc->chid;
						eXosip_call_send_answer(ev->tid,200,ans);
					}
					eXosip_unlock();
				}else if (MSG_IS_NOTIFY(ev->request)){
					//for call transfer , need check subscription-state:terminated;reason=completed
					osip_message_t *ans=NULL;
					eXosip_lock();
					eXosip_call_build_answer(ev->tid,200,&ans);
					if (ans){
						ans->chid=lc->chid;
						eXosip_call_send_answer(ev->tid,200,ans);
					}
					eXosip_unlock();
					if (GetSysState(chid)==SYS_STATE_TRANSFER_ONHOOK)
					{	
						SetSysState(chid, SYS_STATE_TRANSFER_FINISH);
						rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);	
			
					}
					else if (GetSysState(chid)==SYS_STATE_TRANSFER_FINISH)
					{
						
						linphone_core_terminate_dialog(lc,ssid,NULL);
						SetSysState(chid, SYS_STATE_IDLE);
						SetActiveSession(chid, -1, -1);	
			
					}			
					else if(GetSysState(chid)==SYS_STATE_IN3WAY_ONGOING_3)
					{
						SetSysState(chid,SYS_STATE_IN3WAY_ONGOING_4);
					}
					else if(GetSysState(chid)==SYS_STATE_IN3WAY_ONGOING_4)
					{
						char refer_to[256];
						g_message("3 PTY ON SERVER SEND REFER 2 \n");
						if (atdfr_construct_referto_2(lc, refer_to) == 0)
						{
							linphone_core_transfer_call(lc, 2, refer_to);
						}else
							g_message("SEND REFER 2 fail  \n");
						
						SetSysState(chid,SYS_STATE_IN3WAY);
					}

				}else if (MSG_IS_BYE(ev->request)){			
					g_message("MSG_IS_BYE on EXOSIP_CALL_MESSAGE_NEW\n");
					//printf("GetSysState(chid) is %d\n",GetSysState(chid));
					//printf("GetSessionState is %d\n",GetSessionState(chid,ssid));
					if((GetSysState(chid)==SYS_STATE_RING) && (GetSessionState(chid,ssid)==SS_STATE_CALLIN)) {
						/*ericchung: early dialog receive bye, send 487 to sip server */
						eXosip_lock();
						eXosip_call_send_answer(lc->call[ssid]->tid,487,NULL);
						eXosip_unlock();
					}
				}else{
					g_message("Other message , need implement \n");
				}
			}
			g_message("\r\n if receive EXOSIP_CALL_MESSAGE_NEW , message should parse \n");

			break ;
			
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
			
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			g_message("\r\n EXOSIP_CALL_MESSAGE_REQUESTFAILURE linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;
	
			lc = &pApp->ports[chid];

			if (ev->request){				
				if(MSG_IS_UPDATE(ev->request)){
					//printf("EXOSIP_CALL_MESSAGE_REQUESTFAILURE	request UPDATE fail.\n");
					linphone_core_terminate_dialog(lc, ssid, NULL);
					linphone_call_failure(lc, ssid, -110, ev);
					
				}
			}
			break;
#if 0			
		case EXOSIP_MESSAGE_REQUESTFAILURE:
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
				g_message("\r\n EXOSIP_MESSAGE_REQUESTFAILURE linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
				if (ret==-1)
					break;
			
				lc = &pApp->ports[chid];
			
				if (ev->request){	
					if(MSG_IS_OPTIONS(ev->request)){

						
					}
				}
		

			break;
	#endif		
		case EXOSIP_CALL_REINVITE:
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			g_message("\r\n EXOSIP_CALL_REINVITE linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			if (ret==-1)
				break;

			lc = &pApp->ports[chid];
			linphone_core_handle_reinvite(lc, chid, ssid, ev);
			break;
		case EXOSIP_MESSAGE_ANSWERED:
		case EXOSIP_MESSAGE_REQUESTFAILURE:
		case EXOSIP_MESSAGE_SERVERFAILURE:
		case EXOSIP_MESSAGE_GLOBALFAILURE:			
			if (ev->response){
				if (ev->request && strcmp(osip_message_get_method(ev->request),"OPTIONS")==0){				
					g_message("receive OPTIONS response , tid is  %d\n",ev->tid);
					chid = GetChIDfromOptionsTid(pApp,ev);
					if(chid!=-1){
						voipCfgPortParam_t *voip_ptr;
						lc = &pApp->ports[chid];
						lc->OptionsPingStatus=Option_Next_Ping; 
						voip_ptr = &g_pVoIPCfg->ports[lc->chid];
				
						rcm_timerLaunch(lc->options_ping_timer,voip_ptr->proxies[0].HeartbeatCycle*1000); //next send option time
					}				
				}
			}

			break;
		case EXOSIP_CALL_NEED_UPDATE:
			ret=linphone_process_call(pApp, &chid, &ssid, ev);
			g_message("\r\n EXOSIP_CALL_NEED_UPDATE linphone_process_call return %d chid %d ssid %d\n",ret,chid,ssid);
			lc = &pApp->ports[chid];
			rcm_eXosip_on_update_call_answer_sdp(lc->call[ssid]->did, lc->call[ssid]->sdpctx->version++);
			
			break;
		default :
			g_message( "default[%d][%d]\n" , chid , ssid ) ;
			break ;
	}
}

void rcm_process_other_request(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev){

	int pos=0;
	osip_header_t *SubscriptionState_header=NULL;	
	
	if (ev->request==NULL) 
	{
		g_message("%s[%d]\n", __FUNCTION__,__LINE__);
		return;
	}
	if (lc==NULL)
	{
		g_message("%s[%d]\n", __FUNCTION__,__LINE__);
		return;
	}
	if (MSG_IS_OPTIONS(ev->request)){
		osip_message_t *options=NULL;
		eXosip_options_build_answer(ev->tid,200,&options);
		//printf("\r\n Message is OPTION\n");
		osip_message_set_allow(options,"INVITE, ACK, BYE, CANCEL, OPTIONS, MESSAGE, SUBSCRIBE, NOTIFY, INFO, UPDATE");
		osip_message_set_accept(options,"application/sdp");
		eXosip_options_send_answer(ev->tid,200,options);
	}else if (MSG_IS_NOTIFY(ev->request)){
		osip_message_header_get_byname(ev->request,"Subscription-State",pos,&SubscriptionState_header);
		if(SubscriptionState_header){
			if (strcmp(SubscriptionState_header->hvalue, "terminated")==0){//Subscription-State: terminated
				g_message("Subscription-State: terminated\n");
				g_message("start register_retry_timer 60 s\n");
				lc->initial_subscribes_sent=FALSE;
				lc->proxies[0]->registered=FALSE;
				rcm_timerLaunch(lc->register_retry_timer,60*1000);//retry register after 60s
			}
		}
			/* response for NOTIFY */
			rcm_linphone_notify_rec(lc,ssid,ev);
		
			eXosip_message_send_answer(ev->tid,200,NULL);
	}else if (MSG_IS_MESSAGE(ev->request)){
		rcm_linphone_message_rec(lc,ssid,ev);
		
		eXosip_message_send_answer(ev->tid,200,NULL);
	}else{

		/* ericchung : need check other request , TBD */ 
		char *tmp=NULL;
		size_t msglen=0;
		osip_message_to_str(ev->request,&tmp,&msglen);
		if (tmp){
			printf("Unsupported request received:\n%s",tmp);
			osip_free(tmp);
		}
		/*answer with a 501 Not implemented*/
		eXosip_message_send_answer(ev->tid,501,NULL);

		g_message("rcm_process_other_request: receive not other request, need check \n");

	}
}
	

//[SD6, ericchung, rcm integration
/* Process incoming SIP INFO */
void rcm_linphone_process_sipinfo(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev){

	osip_content_type_t *ct;
	osip_message_t *ans=NULL;
	osip_body_t *body=NULL;
	char signal[10], duration[10];
	
	ct=osip_message_get_content_type(ev->request);
	if (ct && ct->subtype){
		if (strcmp(ct->subtype,"dtmf-relay")==0){
			g_message("\r\n receive dtmf-relay\n");
			osip_message_get_body(ev->request,0,&body);
			
			if (body && body->body){
				char *new_body;
				new_body = tolower_clrspace(body->body);
				if (new_body){
					sscanf(new_body, "signal=%s\r\nduration=%s\r\n", signal, duration);
					osip_free(new_body);
				}
			}
			
			// TODO: duration ?
			if (signal[0] && signal[1] == 0){					
#if 1	/* kernel play tone with duration */
				if (signal[0] == '*')
					rcm_tapi_SIP_INFO_play_tone(lc->chid, ssid, DSPCODEC_TONE_STARSIGN, atoi(duration));
				else if (signal[0] == '#')
					rcm_tapi_SIP_INFO_play_tone(lc->chid, ssid, DSPCODEC_TONE_HASHSIGN, atoi(duration));
				else if (signal[0] >= '0' && signal[0] <= '9')
					rcm_tapi_SIP_INFO_play_tone(lc->chid, ssid, signal[0] - '0' + DSPCODEC_TONE_0, atoi(duration));
						
#else	/* IO Ctrl play tone without duration */
				if (signal[0] == '*')
					rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_STARSIGN, 
						TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				else if (signal[0] == '#')
					rcm_tapi_SetPlayTone(chid, ssid, DSPCODEC_TONE_HASHSIGN, 
						TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				else if (signal[0] >= '0' && signal[0] <= '9')
					rcm_tapi_SetPlayTone(chid, ssid, signal[0] - '0' + DSPCODEC_TONE_0, 
						TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
				
#endif
				/*ericchung : if E8C auto test ,must record dtmf string */
#if defined(CONFIG_RTK_VOIP_TR104) && defined(CONFIG_E8B)
				tr104_Auto_Call_Test_Recode_DTMF(lc->chid, signal[0]);

#endif				
			}
		}
	}

	//response 200 ok for info
	eXosip_lock();
	eXosip_call_build_answer(ev->tid,200,&ans);
	if (ans){
		//configure contact header
		if(lc->call[ssid]->proxy!=NULL){
		_osip_list_set_empty(&ans->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(ans,lc->call[ssid]->proxy->contact);
}
		ans->chid=lc->chid;
		eXosip_call_send_answer(ev->tid,200,ans);
	}
	eXosip_unlock();
	return;
}

//process REFER message , for call transfer 
void rcm_linphone_process_refer(LinphoneCore *lc, guint32 ssid, eXosip_event_t *ev){

	osip_header_t *h=NULL;
	osip_message_t *ans=NULL;
	int si; // idle session
	osip_header_t *referred_by=NULL;

	osip_message_header_get_byname(ev->request,"Refer-To",0,&h);
	if (h){
		g_message("rcm_linphone_process_refer h->hvalue is %s\n",h->hvalue);
	}
	osip_message_header_get_byname(ev->request,"Referred-By",0,&referred_by);
	if (referred_by && referred_by->hvalue && referred_by->hvalue[0]!='\0'){
		g_message("rcm_linphone_process_refer referred_by->hvalue is %s\n",referred_by->hvalue);
	}

	eXosip_lock();
	eXosip_call_build_answer(ev->tid,202,&ans);
	if (ans)
		eXosip_call_send_answer(ev->tid,202,ans);
	eXosip_unlock();

	
	rcm_eXosip_transfer_send_notify(lc->call[ssid]->did, EXOSIP_SUBCRSTATE_ACTIVE, "SIP/2.0 200 OK");
	
	si = ssid == 1 ? 0 : 1;
	if (GetSessionState(lc->chid, si) == SS_STATE_IDLE)
	{
		g_message("rcm_linphone_process_refer 002\n");
		rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
		//rcm_tapi_SetPlayTone(lc->chid, si, DSPCODEC_TONE_HOLD, FALSE, DSPCODEC_TONEDIRECTION_LOCAL);
		SetActiveSession(lc->chid , si, TRUE);
		//rcm_linphone_core_refer_invite(lc, si, h->hvalue,ev);
		linphone_core_invite(lc, si, h->hvalue);
		SetSysState(lc->chid, SYS_STATE_TRANSFERRING);
	}
	else
	{
		g_message("EXOSIP_CALL_REFERED: no idle session\n");
	}
		


	return;
}


void rcm_linphone_notify_rec(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev){

	osip_body_t *body=NULL;
	char *p=NULL;
	char *q=NULL;
	char *r=NULL;
	char time[16]={"\0"};	

	if (ev->request==NULL) 
		return;

	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		g_message("No body in NOTIFY");
		return;
	}else{
		g_message("++++ notidy body +++ \n%s\n",body->body);
	}
#ifdef CONFIG_RTK_VOIP_TR104	
	//rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags=0;
	//reset lc->fxsfeatureFlags
	//lc->fxsfeatureFlags=0;
	lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
	lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
	rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
	rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;	
	
	if (strstr(body->body,"special-condition-tone")!=NULL){
		g_message("special condition tone\n");
		lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_SPECIAL_DIAL_TONE;
	}

	if (strstr(body->body,"standard-dial-tone")!=NULL){
		g_message("standard-dial-tone\n");
		lc->fxsfeatureFlags&=~FXS_SPECIAL_DIAL_TONE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_SPECIAL_DIAL_TONE;
	}

	
	if (strstr(body->body,"<Second-Dial-Tone>yes")!=NULL){
		g_message("Second-Dial-Tone enable\n");
		if ((strstr(body->body,"<Intra-Group-Outgoing-Call-Prefix>9"))!=NULL){
			lc->fxsfeatureFlags|=FXS_SECOND_DAIL_TONE_ACTIVATE;
		}
	}
	if (strstr(body->body,"<Second-Dial-Tone>no")!=NULL){
		g_message("Second-Dial-Tone disable\n");
		lc->fxsfeatureFlags&=~FXS_SECOND_DAIL_TONE_ACTIVATE;
	}
	
	
	if (strstr(body->body,"immediateCallSetup")!=NULL){
		g_message("immediateCallSetup (hotline)\n");
		lc->fxsfeatureFlags|=FXS_IMM_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_IMMEDIATE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
	}	
	if (strstr(body->body,"deferredCallSetup")!=NULL){
		g_message("deferredCallSetup (delay hotline)\n");
		lc->fxsfeatureFlags|=FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		lc->hotline_timeout=5;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}	
	if (strstr(body->body,"rejectCall")!=NULL){
		g_message("rejectCall (delay hotline)\n");
		lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}	

		
	if (strstr(body->body,"cw-service-provisioned")!=NULL){
		g_message("call waiting enable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_CALL_WAITING_ACTIVATE;
	}	
	if (strstr(body->body,"cw-service-withdrawn")!=NULL){
		g_message("call waiting disable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_CALL_WAITING_ACTIVATE;
	}	


	if (strstr(body->body,"three-pty-service-provisioned")!=NULL){
		g_message("enable 3 way conference\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CONFERENCE_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_THREE_PTY_ACTIVATE;
	}	
	if (strstr(body->body,"three-pty-service-withdrawn")!=NULL){
		g_message("disable 3 way conference\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_CONFERENCE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_THREE_PTY_ACTIVATE;
	}	

	if (strstr(body->body,"mcid-service-provisioned")!=NULL){
		g_message("enable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_MCID_ACTIVATE;
	}	
	if (strstr(body->body,"mcid-service-withdrawn")!=NULL){
		g_message("disable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_MCID_ACTIVATE;
	}




	if (strstr(body->body,"call-waiting active=\"true")!=NULL){
		g_message("call waiting enable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_CALL_WAITING_ACTIVATE;
	}	

	if (strstr(body->body,"three-party-conference active=\"true")!=NULL){
		g_message("enable 3 way conference\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CONFERENCE_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_THREE_PTY_ACTIVATE;
	}	

	if (strstr(body->body,"malicious-call-identify active=\"true")!=NULL){
		g_message("enable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_MCID_ACTIVATE;
	}	
#endif
	/* add parse IMS  sip server forwarding tone */
	//lc->fxsfeatureFlags &= ~FXS_SPECIAL_DIAL_TONE;

	if (strstr(body->body,"<alert-info>pattern6")!=NULL){
			g_message("enable forwarding \n");
			 lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;

	}
	
	/*only play one time */	
	if (strstr(body->body,"Messages-Waiting: yes")!=NULL){
			g_message("enable voicemail box tone \n");
			 lc->fxsfeatureFlags|=FXS_VOICEMAIL_TONE;

	}
	
	p=strstr(body->body,"<hot-line-service>");
	if(p!=NULL){
		g_message("hot line enable \n");
		q=strstr(p,"<addr>");
		if (q==NULL)
		{
			lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
			lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
			return;
		}
		q+=6;
		r=strstr(p,"</addr>");
		if(r!=NULL)
		{
			strncpy(g_pVoIPCfg->ports[lc->chid].hotline_number, q,r-q);
			//g_message("g_pVoIPCfg->ports[lc->chid].hotline_number=%s\n", g_pVoIPCfg->ports[lc->chid].hotline_number);
		}else{
			g_pVoIPCfg->ports[lc->chid].hotline_enable=0;
			return;
		}
		q=strstr(p,"<timeout>");
		if (q==NULL)//no timeout immediate call out
		{
			g_message("immediateCallSetup (hotline)\n");
			lc->fxsfeatureFlags|=FXS_IMM_HOTLINE_ACTIVATE;
			lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_IMMEDIATE_CALLSETUP;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
			return;
		}
		q+=9;
		r=strstr(p,"</timeout>");
		if(r!=NULL)
		{
			strncpy(time, q,r-q);
			//g_message("!!!!timeout =%s\n",time);
			lc->hotline_timeout=atoi(time);
			//g_message("timeout=%d\n", lc->hotline_timeout);
		}else//no timeout immediate call out
		{
			g_message("immediateCallSetup (hotline)\n");
			lc->fxsfeatureFlags|=FXS_IMM_HOTLINE_ACTIVATE;
			lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_IMMEDIATE_CALLSETUP;
			rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
			return;
		}
		lc->fxsfeatureFlags|=FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}
	//TBD
	
}
void rcm_linphone_message_rec(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev){

	osip_body_t *body=NULL;
	

	if (ev->request==NULL) 
		return;

	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		g_message("No body in Message");
		return;
	}else{
		g_message("++++ notidy body +++ \n%s\n",body->body);
	}

	
	if (strstr(body->body,"special-condition-tone")!=NULL){
		g_message("get nodify special condition tone\n");
		lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_SPECIAL_DIAL_TONE;
	}

	if (strstr(body->body,"standard-dial-tone")!=NULL){
		g_message("get nodify special condition tone\n");
		lc->fxsfeatureFlags&=~FXS_SPECIAL_DIAL_TONE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_SPECIAL_DIAL_TONE;
	}

	
	if (strstr(body->body,"Second-Dial-Tone:yes")!=NULL){
		g_message("Second-Dial-Tone enable\n");
		if ((strstr(body->body,"<Intra-Group-Outgoing-Call-Prefix>9"))!=NULL){
			lc->fxsfeatureFlags|=FXS_SECOND_DAIL_TONE_ACTIVATE;
		}
	}
	if (strstr(body->body,"Second-Dial-Tone:no")!=NULL){
		g_message("Second-Dial-Tone disable\n");
		lc->fxsfeatureFlags&=~FXS_SECOND_DAIL_TONE_ACTIVATE;
	}
	
	
	if (strstr(body->body,"Hot Line:ACTIVE")!=NULL){
		g_message("immediateCallSetup (hotline)\n");
		//lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;
		lc->fxsfeatureFlags|=FXS_IMM_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_IMMEDIATE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
	}	
	if (strstr(body->body,"Warm Line:ACTIVE")!=NULL){
		g_message("deferredCallSetup (delay hotline)\n");
		lc->fxsfeatureFlags|=FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}	
	if (strstr(body->body,"Warm Line:DEACTIVE")!=NULL){
		g_message("Warm Line:DEACTIVE \n");
		//lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;
		lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}	

	if (strstr(body->body,"Hot Line:DEACTIVE")!=NULL){
		g_message("Hot Line:DEACTIVE \n");
		//lc->fxsfeatureFlags|=FXS_SPECIAL_DIAL_TONE;
		lc->fxsfeatureFlags&=~FXS_HOTLINE_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_IMM_HOTLINE_ACTIVATE;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_DEFERE_CALLSETUP;
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_IMMEDIATE_CALLSETUP;
	}
		
	if (strstr(body->body,"CW:ACTIVE")!=NULL){
		g_message("call waiting enable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_CALL_WAITING_ACTIVATE;
	}	
	if (strstr(body->body,"CW:DEACTIVE")!=NULL){
		g_message("call waiting disable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_CALL_WAITING_ACTIVATE;
	}	

	if (strstr(body->body,"MCID:ACTIVE")!=NULL){
		g_message("enable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_MCID_ACTIVATE;
	}	


	if (strstr(body->body,"MCID:DEACTIVE")!=NULL){
		g_message("disable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags&=~IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags&=~FXS_MCID_ACTIVATE;
	}


	if (strstr(body->body,"call-waiting active=\"true")!=NULL){
		g_message("call waiting enable\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CALL_WAITING_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_CALL_WAITING_ACTIVATE;
	}	

	if (strstr(body->body,"three-party-conference active=\"true")!=NULL){
		g_message("enable 3 way conference\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_CONFERENCE_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_THREE_PTY_ACTIVATE;
	}	

	if (strstr(body->body,"malicious-call-identify active=\"true")!=NULL){
		g_message("enable MCID\n");
		rcm_tr104_st.voiceProfileLineStatusMsg[lc->chid].featureFlags|=IMS_MCID_ACTIVATE;
		lc->fxsfeatureFlags|=FXS_MCID_ACTIVATE;
	}	

}

int three_way_enable(int chid)
{

	if (g_pVoIPCfg->ports[chid].fxsport_flag & FXS_3WAY_DISABLE)
	return 0 ;

	return 1 ;
}
void linphone_core_handle_reinvite_ack_sdp(LinphoneCore *lc, int chid, int ssid, sdp_context_t *ctx)
{
		g_message("linphone_core_handle_reinvite_ack_sdp[%d][%d]\n", chid, ssid);
		g_message("\r\n 001 should change linphone to ctx->[remote_dir=%d]\n", ctx->remote_dir);
	if(ctx->remote_dir == SalStreamSendRecv){ //reinvite is call update or call resume.


		if (lc->call[ssid]->state == LCStateAVRunning)
		{

			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateUpdate);	
		}
		else if (lc->call[ssid]->state != LCStateBeResume)
		{
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state) ,LCStateBeResume);
		}
	}else if(ctx->remote_dir == SalStreamSendOnly){ //reinvite for call hold
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}else if(ctx->remote_dir == SalStreamInactive){ //reinvite for call hold
		if (lc->call[ssid]->state != LCStateBeHold && 
			lc->call[ssid]->state != LCStateBeHoldACK)
		{
			SetCallState(lc->chid,ssid,&(lc->call[ssid]->state),LCStateBeHold);
		}
	}
}

void caller_id_send_P(int chid){
	
	//=====Set the FSK Caller ID Parameters. 
	TstFskClid clid;
	memset(&clid, 0, sizeof(TstFskClid));
	clid.ch_id = chid;

	//=====Set the FSK Caller ID Parameters-service type 1
	clid.service_type = 0;  

	//=====Set the FSK Caller ID Parameters-Calling Line Identify (CLI)
	clid.cid_data[0].type = 0x02;//EXAMPLE_FSK_PARAM_CLI
	strcpy(clid.cid_data[0].data, "P");	

	//Only 1 elements for Caller ID data. Set other element to 0 (MUST)
	clid.cid_data[1].type = 0;        		
	clid.cid_data[2].type = 0;
	clid.cid_data[3].type = 0;
	clid.cid_data[4].type = 0;

	//=====gen CID
	rtk_GenMdmfFskCid(chid, &clid, 1);
}

void rcm_arp_test(LinphoneCore *lc){

//1. get the voip interface gateway ip
//2. send arp to check gateway alive
//3 . if gateway no response ,  restar dhcp client
	FILE *fp=NULL;
	char cmdStr[128];
	char voip_gateway[20];
	g_message("rcm_arp_test\n");
	snprintf(cmdStr, sizeof(cmdStr), "ip route list dev %s | awk \' /^default/ {print $3}\' > /tmp/voip_gateway",solar_ptr->current_voip_interface);
	g_message("cmdStr is %s\n",cmdStr);
	system(cmdStr);
	
	memset(cmdStr, 0, sizeof(cmdStr));		
	
	fp = fopen("/tmp/voip_gateway", "r");
	if (fp==NULL) {
		printf("rcm_arp_test file open error\n");
		return;
	}

	fscanf(fp, "%s", voip_gateway);
	fclose(fp);

	if((voip_gateway==NULL) ||	(strlen(voip_gateway)==0))
		return;

	
	g_message("voip gateway is %s\n",voip_gateway);

	snprintf(cmdStr, sizeof(cmdStr), "arping -I %s -c 2 %s > /tmp/voip_arp_result",solar_ptr->current_voip_interface,voip_gateway);
	system(cmdStr);
	g_message("start e8c_arp_result_timer\n");
	rcm_timerLaunch(lc->e8c_arp_result_timer ,6*1000);//after 6 sec, check result


}
void notify_event_reg_rec(LinphoneCore *lc, guint32 ssid,eXosip_event_t *ev){
	osip_header_t *event_header=NULL;	
	osip_header_t *SubscriptionState_header=NULL;	
	int pos=0;

	osip_message_header_get_byname(ev->request,"Subscription-State",pos,&SubscriptionState_header);
	if (SubscriptionState_header){
		g_message("[%s] Subscription-State: %s\n", __FUNCTION__,SubscriptionState_header->hvalue);
		if(strstr(SubscriptionState_header->hvalue, "active")){//Subscription-State: active
		
			g_message("Subscription-State: active\n");					
			if (g_pVoIPCfg->subs_type==SUBS_TYPE_ALL)
				rcm_linphone_core_send_initial_subscribes(lc,lc->default_proxy,SUBSCIRBE_EVENT_CONTENT, g_pVoIPCfg->subs_time);
			
		}else if (strstr(SubscriptionState_header->hvalue, "terminated")){//Subscription-State: terminated
		
			g_message("Subscription-State: terminated\n");
			g_message("start register_retry_timer 60 s\n");
			lc->initial_subscribes_sent=FALSE;
			lc->proxies[0]->registered=FALSE;
			rcm_timerLaunch(lc->register_retry_timer,60*1000);//retry register after 60s
			
		}else{//unkonw Subscription-State value
			g_message("unkonw Subscription-State value\n");
		}
	}else
		g_message("SUBSCRIPTION_NOTIFY without  Subscription-State hdader\n");
	
}
void get_callid_from_P_Asserted_Identity(eXosip_event_t *ev, char* p_caller_id){
	osip_header_t *caller_id_header=NULL;
	osip_from_t * pai_url = NULL ;
	char *tmp=NULL;
	char *tmp_end=NULL;
	char identity[64]= {'\0'};
	int i=0, j=0;
	
	while (osip_message_header_get_byname(ev->request,"P-Asserted-Identity",j,&caller_id_header)>=0){
		if (caller_id_header){
			g_message("P-Asserted-Identity->hvalue is %s\n",caller_id_header->hvalue);
			tmp=strstr(caller_id_header->hvalue, "<tel:");
			if(tmp!=NULL)
			{
				tmp+=5;
				tmp_end=strchr(tmp, '>');
				if(tmp_end!=NULL)
				{
					if(strchr(tmp, ';')!=NULL)
						tmp_end=strchr(tmp, ';');
					
					if(strchr(tmp, '+')!=NULL) //<tel:+86xxxxxxxx> remove + for isdigit check
					{
						tmp+=1;
						g_message("caller_id <tel:+xxxxxxxxxx> remove + tmp=%s\n", tmp);
					}
					
					if ((tmp_end-tmp)>=64)
					{
						g_message("[P-Asserted-Identity]identity=%s len =%d too large\n",identity, tmp_end-tmp);
						break;
					}
					strncpy(identity,tmp,tmp_end-tmp);
					identity[tmp_end-tmp]= '\0';
					g_message("[P-Asserted-Identity]identity=%s\n",identity);
					


					if(identity[0]!=0)
					{
						g_message("[P-Asserted-Identity]tel: replease caller id =%s\n",identity);
						strcpy(p_caller_id, identity);
						break;
					}
				}
			}
			
			tmp=strstr(caller_id_header->hvalue, "<sip:");
			if(tmp!=NULL)
			{
				osip_from_init( &pai_url ) ;
				if (osip_from_parse( pai_url , tmp ) ==OSIP_SUCCESS)
				{
					// username -> caller id
					if (&pai_url->url.username[0] != '\0')
					{
						if(strchr(pai_url->url.username, ';')!=NULL)// <sip:+8618124790187;cpc=ordinary@10.189.84.137;transport=udp;user=phone> remove string after ;
						{
							tmp_end=strchr(pai_url->url.username, ';');
							strncpy(identity, pai_url->url.username, tmp_end-pai_url->url.username);
						}else
							strncpy(identity, pai_url->url.username, sizeof(identity)-1);
						
						identity[strlen(identity)] = 0;

						g_message("sip: identity is %s\n",identity);


						if(identity[0]!=0)
						{
							g_message("[P-Asserted-Identity]tel: replease caller id =%s\n",identity);
							strcpy(p_caller_id, identity);
							break;
						}
						break;
					}
				}
			}
		}
		j++;
	}
	}