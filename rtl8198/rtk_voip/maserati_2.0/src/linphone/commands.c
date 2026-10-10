/***************************************************************************
 *            commands.c
 *
 *  Sun Oct 10 17:27:57 2004
 *  Copyright  2004  Simon MORLAT
 *  Email simon.morlat@linphone.org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "linphonecore.h"
#include "osip2/osip.h"
 #include "eXosip2.h"


//#define LINE_MAX_LEN 100

void linphonec_help(const gchar *arg){
	if (strlen(arg)==0){
		printf("Commands are:\n call answer terminate proxy call-logs ipv6\nType 'help <command>' for more details.\n");
		return;
	}
	if (strcmp(arg,"call")==0){
		printf("'call <sip-url>' or 'c <sip-url>' : initiate a call to the specified destination.\n");
	}else if (strcmp(arg,"answer")==0){
		printf("'answer' or 'a' : answer to (accept) an incoming call.\n");
	}else if (strcmp(arg,"terminate")==0){
		printf("'terminate' or 't' :  terminate the current call.\n");
	}else if (strcmp(arg,"quit")==0){
		printf("'quit' or 'q' : exit linphonec.\n");
	}else if (strcmp(arg,"proxy")==0){
		printf("'proxy list' : list all proxy setups.\n");
		printf("'proxy add' : add a new proxy setup.\n");
		printf("'proxy remove <index>' : remove proxy setup with number index.\n");
		printf("'proxy use <index>' : use proxy with number index as default proxy.\n");
		printf("'proxy unuse' : don't use a default proxy.\n");
	}else if (strcmp(arg,"call-logs")==0){
		printf("'call-logs' : show an history of calls.\n");
	}else if (strcmp(arg,"ipv6")==0){
		printf("'ipv6 status' : show ipv6 usage status.\n");
		printf("'ipv6 enable' : enable the use of the ipv6 network.\n");
		printf("'ipv6 disable' : do not use ipv6 network.\n");
	}
	else {
		printf("No such command.\n");
	}
}


void linphonec_proxy_add(LinphoneCore *lc)
{
	voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[lc->chid]; //chid , flash configure
	LinphoneProxyConfig *cfg; //linphone proxy config
	char proxy_addr[LINE_MAX_LEN];
	char proxy_domain_name[LINE_MAX_LEN];
	char szBuf[LINE_MAX_LEN], szIdentity[LINE_MAX_LEN];
	int i;

	for (i=0; i<MAX_PROXY; i++)
	{
		voipCfgProxy_t *proxy;

		proxy = &voip_ptr->proxies[i];
		if ((proxy->enable & PROXY_ENABLED) == 0)
			continue;

		if((proxy->addr==NULL) || (strlen(proxy->addr))==0)
			continue;

		
		if(g_pVoIPCfg->rfc_flags & SIP_IPV6_ENABLE){
			sprintf(szBuf, "sip:%s", proxy_addr);
		}else{
		if (proxy->port != 5060)
			sprintf(proxy_addr, "%s:%d", proxy->addr, proxy->port);
		else
			sprintf(proxy_addr, "%s", proxy->addr);
		}

		sprintf(szBuf, "sip:%s", proxy_addr);

		cfg = linphone_proxy_config_new(szBuf, proxy->domain_name);
		if (cfg == NULL) 
		{
			printf("Invalid sip address.\n");
			return;
		}

		cfg->index = i;
		if(proxy->outbound_enable){ 		
			strncpy(cfg->outbound_proxy,proxy->outbound_addr,strlen(proxy->outbound_addr));
			cfg->outbound_port=proxy->outbound_port;
		}else{
			cfg->outbound_port=0;
			//if IMS dont use domain name to register, use porxy ip for relay sip packet
			if( (strlen(proxy->domain_name) != 0 ))
			{
				strncpy(cfg->outbound_proxy,proxy->addr,strlen(proxy->addr));
				cfg->outbound_port=proxy->port;
			}
		}

		
		if (proxy->port != 5060){
			if( strlen(proxy->domain_name) != 0 ){
				sprintf(proxy_domain_name, "%s:%d", proxy->domain_name, proxy->port);
			}
			else{
				sprintf(proxy_domain_name, "%s:%d", proxy->addr, proxy->port);
			}
		}
		else{
			if( strlen(proxy->domain_name) != 0 ){
				sprintf(proxy_domain_name, "%s", proxy->domain_name);
			}
			else{
				sprintf(proxy_domain_name, "%s", proxy->addr);
			}
		}
		

		g_message("%s(%d) domain_name=%s\n"       , __FUNCTION__ , __LINE__ , proxy->domain_name);
		g_message("%s(%d) proxy_domain_name=%s\n" , __FUNCTION__ , __LINE__ , proxy_domain_name); 


		if (proxy->number[0])
			sprintf(szBuf, "sip:%s@%s", proxy->number, proxy_domain_name);
		else
			sprintf(szBuf, "sip:%s", proxy_domain_name);

		if (proxy->display_name[0])
			sprintf(szIdentity, "\"%s\" <%s>", proxy->display_name, szBuf);
		else
			strcpy(szIdentity, szBuf);

		linphone_proxy_config_set_identity(cfg, szIdentity);
		//[20110805, SD6, ericchung, RCM integration
		//linphone_proxy_config_enableregister(cfg, TRUE); //enable register to server 
		//ericchung , when start linphone, disable register to server first.
		linphone_proxy_config_enableregister(cfg, 0);
		//]
		
		linphone_proxy_config_expires(cfg, proxy->reg_expire);
	#if 0
		linphone_proxy_config_create_routes(cfg, voip_ptr);
	#endif
		linphone_core_add_proxy_config(lc, cfg);
		g_message("Proxy %s added.\n", proxy_addr);
#ifdef MULTI_PROXY
		//[20110805, SD6, ericchung, RCM integration
		//add point to proxycfg
		// lc->sip_conf.proxies also have this point.
		//]
		lc->proxies[i] = cfg;
#endif
#if 0	/* pkshih: new default proxy policy */
		if (lc->default_proxy == NULL)
		{
			// set first active proxy
			linphone_core_set_default_proxy(lc, cfg);
		}
		else if (i == voip_ptr->default_proxy)
		{
			// if default proxy in flash is enabled, use it to replace
			linphone_core_set_default_proxy(lc, cfg);
		}
#endif
	}

	// set default proxy index
	lc->default_proxy_index = voip_ptr->default_proxy;
}

void linphonec_proxy_display(LinphoneProxyConfig *cfg){
//	printf("sip address: %s\nroute: %s\nidentity: %s\nregister: %s\nexpires: %i\n",
	printf("sip address: %s\nidentity: %s\nregister: %s\nexpires: %i\n",
			cfg->reg_proxy,
//			(cfg->reg_route!=NULL)?cfg->reg_route:"",
			(cfg->reg_identity!=NULL)?cfg->reg_identity:"",
			(cfg->reg_sendregister)?"yes":"no",
			cfg->expires);
}

void linphonec_proxy_list(LinphoneCore *lc){
	GList *proxies;
	int n;
	int def=linphone_core_get_default_proxy(lc,NULL);
	
	proxies=linphone_core_get_proxy_config_list(lc);
	for(n=0;proxies!=NULL;proxies=g_list_next(proxies),n++){
		if (n==def)
			printf("****** Proxy %i - this is the default one - *******\n",n);
		else 
			printf("****** Proxy %i *******\n",n);
		linphonec_proxy_display((LinphoneProxyConfig*)proxies->data);
	}
}

void linphonec_proxy_remove(LinphoneCore *lc, int index){
	GList *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)g_list_nth_data(proxies,index);
	if (cfg==NULL){
		printf("No such proxy.");
		return;
	}
	linphone_core_remove_proxy_config(lc,cfg);
}

void linphonec_proxy_use(LinphoneCore *lc, int index){
	GList *proxies;
	LinphoneProxyConfig *cfg;
	proxies=linphone_core_get_proxy_config_list(lc);
	cfg=(LinphoneProxyConfig*)g_list_nth_data(proxies,index);
	if (cfg==NULL){
		printf("No such proxy.");
		return;
	}
	linphone_core_set_default_proxy(lc,cfg);
}

void linphonec_proxy_unuse(LinphoneCore *lc){
	linphone_core_set_default_proxy(lc,NULL);
}

void linphonec_show_logs(LinphoneCore *lc){
	GList *elem=linphone_core_get_call_logs(lc);
	for (;elem!=NULL;elem=g_list_next(elem)){
		LinphoneCallLog *cl=(LinphoneCallLog*)elem->data;
		gchar *str=linphone_call_log_to_str(cl);
		printf("%s\n",str);
		g_free(str);
	}
}

void linphonec_ipv6(LinphoneCore *lc, const char *arg1){
	if (strcmp(arg1,"status")==0){
		printf("ipv6 use enabled: %s\n",linphone_core_ipv6_enabled(lc) ? "true":"false");
	}else if (strcmp(arg1,"enable")==0){
		linphone_core_enable_ipv6( lc , TRUE ) ;
		printf("ipv6 use enabled.\n");
	}else if (strcmp(arg1,"disable")==0){
		linphone_core_enable_ipv6( lc , FALSE ) ;
		printf("ipv6 use disabled.\n");
	}
}

int create_sip_url(const char *src_ptr, char *sip_url, const char chDot)
{
	int i, idx;
	int state, found;

	// 3880..192.168.1.1..5061 => 3880@192.168.1.1:5061
	state = 0;
	found = 0;

	if(src_ptr[0]=='#') /*if dial start char is # , should not add sip: */
		return -1;
	
	idx = sprintf(sip_url, "sip:");
	for (i=0; src_ptr[i]; i++)
	{
		switch (state)
		{
		case 0:
			if (src_ptr[i] == chDot)
			{
				if (i == 0)
					return -1;

				if (src_ptr[i - 1] == chDot)	// check "..."
					return -1;

				if (found == 3)				// not allow '.' in finding port
					return -1;

				state = 1;
				continue;
			}
			
			sip_url[idx++] = src_ptr[i];
			break;
		case 1:
			if (src_ptr[i] == chDot)
			{
				if (found == 0)
				{
					sip_url[idx++] = '@';
					found = 1;
				}
				else
				{
					sip_url[idx++] = ':';
					found = 3;
				}
			}
			else
			{
				found = 2;
				sip_url[idx++] = '.';
				sip_url[idx++] = src_ptr[i];
			}

			state = 0;
			break;
		}
	}

	// pasre done
	sip_url[idx] = 0;
	return 0;
}

const unsigned char * speed_dial_check( int chid, 
										const gchar *pDialCode, 
										gchar *pSpeedDialCode )
{
#if 0 //remove unnecessary parameter
	if (pDialCode[1] == 0)
	{
		int i, num;

		for (i=0; i<MAX_SPEED_DIAL; i++)
		{
			num = pDialCode[0] - '0';
			if (num == i && g_pVoIPCfg->ports[chid].speed_dial[i].url[0])
			{
				// do speed dial replace
				if( pSpeedDialCode )
					strcpy(pSpeedDialCode, g_pVoIPCfg->ports[chid].speed_dial[i].url);
				return g_pVoIPCfg->ports[chid].speed_dial[i].url;
			}
		}
	}
#endif
	return NULL;
}

gboolean
linphonec_parse_command_line(LinphoneCore *lc, uint32 ssid, const gchar *cl)
{
	int cllen = strlen( cl ) ;
	gchar * command = g_malloc0( cllen + 1) ;
	gchar * arg1 = g_malloc0( cllen + 100 ) ;
	gchar * arg2 = g_malloc0( cllen + 1) ;
	gboolean cont = TRUE ;
	int err_ret=0;


	sscanf( cl , "%s %s %s" , command , arg1 , arg2 ) ;
	if( strcmp( command , "call" ) == 0 )
	{
		const char * const url = arg1;


		if (lc->call[ssid])
			linphone_core_terminate_dialog(lc, ssid, NULL);

		// speed dial check

		/* Dial plan does followings in CallInviteByDialPlan() */
		//ericchung , if dial self phonenumber , must play busy tone.
		if(strcmp(g_pVoIPCfg->ports[lc->chid].proxies->login_id,arg1)==0){
			printf("dial self phonenumber , play busy tone\n");
			SetSessionState(lc->chid, ssid, SS_STATE_IDLE);
			rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);

			switch (GetSysState(lc->chid))
			{
			case SYS_STATE_EDIT:
			case SYS_STATE_CONNECT_EDIT:
			case SYS_STATE_TRANSFER_EDIT:

				SetSysState(lc->chid, GetSysState(lc->chid) + 1);
				break;
			default:

				// do nothing
				break;
			}
			g_free( command ) ;
			g_free( arg1 ) ;
			g_free( arg2 ) ;
			return 1;
		}

		if (linphone_core_invite(lc, ssid, url) == -1)
		{
			SetSessionState(lc->chid, ssid, SS_STATE_IDLE);

			rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);

			switch (GetSysState(lc->chid))
			{
			case SYS_STATE_EDIT:
			case SYS_STATE_CONNECT_EDIT:
			case SYS_STATE_TRANSFER_EDIT:

				SetSysState(lc->chid, GetSysState(lc->chid) + 1);
				break;
			default:

				// do nothing
				break;
			}
		}
	}else if( strcmp( command , "localcall" ) == 0 ){

			const char * const url = arg1;

/* internal call */
			if (lc->call[ssid])
				linphone_core_terminate_dialog(lc, ssid, NULL);

			//ericchung , if dial self internal phonenumber , must play busy tone.
 			//todo , internal phonebook must create, current is hard copy.

			/* TODO how to handle more then 2 FXS? */
			switch(lc->chid){
				case 0:
				if(strcmp(LC_FUNCKEY_FXS_0,arg1)==0){
						err_ret=1;
						}else if(strcmp(LC_FUNCKEY_FXS_1,arg1)==0){
							rcm_linphone_dial_local(lc,ssid,1,NULL); //dial to chid 1 (fxs 1 call fxs2)

					}else{
							//printf("RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature) is %d\n",RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature));				
							rcm_linphone_dial_local(lc,ssid,RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature),url); //dial to chid for fxo

					}
					break;
				case 1:
					if(strcmp(LC_FUNCKEY_FXS_1,arg1)==0){
						err_ret=1;
						}else if(strcmp(LC_FUNCKEY_FXS_0,arg1)==0){
							rcm_linphone_dial_local(lc,ssid,0,NULL); //dial to chid 0 (fxs2 call fxs 1)
					}else{
							//printf("RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature) is %d\n",RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature));				
							rcm_linphone_dial_local(lc,ssid,RTK_VOIP_DAA_CH_OFFSET(g_VoIP_Feature),url); //dial to chid for fxo
					}	
					break;
				}	

			
			if(err_ret==1){
					printf("dial self internal phone , play busy tone\n");
					SetSessionState(lc->chid, ssid, SS_STATE_IDLE);
					rcm_tapi_SetPlayTone(lc->chid, ssid, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
					switch (GetSysState(lc->chid))
					{
					case SYS_STATE_EDIT:
					case SYS_STATE_CONNECT_EDIT:
					case SYS_STATE_TRANSFER_EDIT:
						SetSysState(lc->chid, GetSysState(lc->chid) + 1);
						break;
					default:
						// do nothing
						break;
					}
					g_free( command ) ;
					g_free( arg1 ) ;
					g_free( arg2 ) ;
					return 1;
				}
	
		
	}
	else if( strcmp( command , "terminate" ) == 0 )
	{
//		linphone_core_terminate_dialog(lc, 0, NULL);
//		linphone_core_terminate_dialog(lc, 1, NULL);
	}
	else if( strcmp( command , "terminate2" ) == 0 )
	{
//		linphone_core_terminate_dialog(lc, 0, ( const char * )1 );
		rcm_tapi_SetPlayTone(lc->chid, 0, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
//		linphone_core_terminate_dialog(lc, 1, ( const char * )1 );
		rcm_tapi_SetPlayTone(lc->chid, 1, DSPCODEC_TONE_BUSY, TRUE, DSPCODEC_TONEDIRECTION_LOCAL);
	}
	else if( strcmp( command , "answer" ) == 0 )
		linphone_core_accept_dialog(lc, ssid, NULL);
	else if( strcmp( command , "answer2" ) == 0 )
			linphone_core_accept_dialog(lc, 1, NULL);	
	else if( strcmp( command , "quit" ) == 0 )
		cont = FALSE ;
	else if( strcmp( command , "help" ) == 0 )
		linphonec_help( arg1 ) ;
	else if( strcmp( command , "proxy" ) == 0 )
	{
		if( strcmp( arg1 , "add" ) == 0 )
			linphonec_proxy_add( lc );
		else if( strcmp( arg1 , "list" ) == 0 )
			linphonec_proxy_list( lc ) ;
		else if( strcmp( arg1 , "remove" ) == 0 )
			linphonec_proxy_remove( lc , atoi( arg2 ) ) ;
		else if( strcmp( arg1 , "use" ) == 0 )
			linphonec_proxy_use( lc , atoi( arg2 ) ) ;
		else if( strcmp( arg1 , "unuse" ) == 0 )
			linphonec_proxy_unuse( lc ) ;
	}
	else if( strcmp( command , "call-logs" ) == 0 )
		linphonec_show_logs( lc ) ;
	else if( strcmp( command , "ipv6" ) == 0 )
		linphonec_ipv6( lc , arg1 ) ;
	else if( strlen( command ) == 1 )
	{
		switch( command[0] )
		{
			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' :
			case '#' :
			case '*' :
				linphone_core_send_dtmf(lc, ssid, command[0]);
				break ;
		}
	}
	else if( strlen( command ) == 0 )
	{
		/* nothing to do !! */
	}
	else
	{
		gchar * tmp = g_strdup( cl ) ;
		gchar * eol ;
		if( ( eol = strchr( tmp , '\n' ) ) != NULL )
			eol[0] = '\0' ;
//		g_warning( "'%s' : Cannot understand this.\n" , tmp ) ;
		g_free( tmp ) ;
	}
	g_free( command ) ;
	g_free( arg1 ) ;
	g_free( arg2 ) ;
	return cont ;
}
