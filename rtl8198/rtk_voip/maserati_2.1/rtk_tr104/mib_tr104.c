#include <string.h>
#include <stdio.h>
//#include "apmib.h"
#include "mib_tr104.h"
#include "voip_flash.h"
#include "prmt_capabilities.h"
#include "prmt_limit.h"
#include "prmt_igd.h"
#include <voip_manager.h>
#include <rtk_voip/kernel/include/voip_control.h>
#include "utility.h"

#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include "mibtbl.h"
#endif
#include "cwmpevt.h"
#include "rcm_customize.h"
#include "prmt_apply.h"

#define IS_VOICE_PROFILE(X,NUM) (X < MIB_VOICE_PROFILE__ARG_TYPE##NUM##_END)
#define IS_PHYINTERFACE(X,NUM) 	(X > MIB_VOICE_PROFILE__ARG_TYPE##NUM##_END &&\
                                 X < MIB_PHYINTERFACE__ARG_TYPE##NUM##_END)

/* voip share memory */
voipCfgParam_t *pVoipConfig = NULL;
boolean VoIP_server_type_change = FALSE;
extern cwmpEvtMsg pEvtMsg;
boolean bDirtyBit=FALSE;

static TstVoipSLICLineVoltage voltage;
static TstVoipSLICLineResFlt flt;
static TstVoipSLICLineCap cap;

/* Init voip client */
int getVoipFlash(void){

    CWMPDBG( 3, ( stderr, "DEBUG: initialize configure\n") );
	if (voip_flash_get(&pVoipConfig) == -1){
		CWMPDBG( 0, ( stderr, "<%s:%d> ERROR: voip_flash_get failed.\n", __FUNCTION__, __LINE__) );
        return -1;        
    }

    if( pVoipConfig == NULL ){
        CWMPDBG( 0, ( stderr, "<%s:%d> ERROR: pVoipConfig is NULL\n", __FUNCTION__, __LINE__) );
        return -1;
    }
	return 0;
}

void update_VoIP_FLASH(void)
{
	int res=0;
	if( pVoipConfig == NULL) {
		//printf("pVoipConfig is null %s\n",__FUNCTION__);
		CWMPDBG( 0, ( stderr, "<%s:%d> no voip config need save.\n", __FUNCTION__, __LINE__) );
		return;
	}
	//printf("update_VoIP_FLASH ok \n");
	voip_flash_set(pVoipConfig);
	voip_flash_server_init_variables();
	res=voip_flash_server_update();
    
	if(VoIP_server_type_change==TRUE){    
    	if (pVoipConfig->X_CT_servertype == 2) { //H.248
        	system("sh /etc/scripts/SIPtoh248.sh");
    	}
    	else{
        	system("sh /etc/scripts/h248toSIP.sh");
    	}
		VoIP_server_type_change = FALSE;
	}


	if (bDirtyBit==TRUE)
	    restart_voip();
	if(res==-1){
		printf("voip_flash_server_update fail\n");
	}
}

#if 0  //unuse
/* get the sip uri. If domain is NULL, device LAN ip is replaced*/
static void getSIPURI(char *sipURI, char *username, char *domain){
	boolean buserName=TRUE;
	
	if(NULL == username || 0 == strlen(username)){
		buserName=FALSE;
	}
	
	if(NULL == domain || 0 == strlen(domain)){
		if(TRUE == buserName)
			sprintf(sipURI,"%s",username);
		else
			sprintf(sipURI,"");
	}else{
	if(TRUE == buserName)
		sprintf(sipURI,"%s@%s",username,domain);
	else
		sprintf(sipURI,"%s",domain);
	}
}
/* get number from login id*/
static void getNumberFromURI(char *Number, const char *URI){
	char *temp=NULL;
	char uri_tmp[64];
	
	strcpy(uri_tmp,URI);
        temp = strtok(uri_tmp, "@");
	if( temp!=NULL ){
		strcpy(Number,temp);	
	}else{
		strcpy(Number,URI);
	}
}
static void getDomainFromURI(char *Domain, const char *URI){
	char *temp=NULL;
	
        temp = strstr(URI, "@");
	if( temp!=NULL ){
		temp+=1;
		strcpy(Domain,temp);	
	}else{
		strcpy(Domain,"");
	}
	
	
}
#endif

/* get the flash codec index */
static int getCodecIndex(const char *codecName, int bitRate){
	
	if( NULL == codecName)
		return -1;
	
	if(0 == strcmp("G.711MuLaw",codecName)){
		return SUPPORTED_CODEC_G711U;
	}else if(0 == strcmp("G.711ALaw",codecName)){
		return SUPPORTED_CODEC_G711A;
#ifdef CONFIG_RTK_VOIP_G729AB		
	}else if(0 == strcmp("G.729",codecName)){
		return SUPPORTED_CODEC_G729;
#endif
#ifdef CONFIG_RTK_VOIP_G7231
	}else if(0 == strcmp("G.723.1",codecName) ){
		return SUPPORTED_CODEC_G723;
#endif
#ifdef CONFIG_RTK_VOIP_G726
	}else if(0 == strcmp("G.726",codecName) && 16000 == bitRate){
		return SUPPORTED_CODEC_G726_16;
	}else if(0 == strcmp("G.726",codecName) && 24000 == bitRate){
		return SUPPORTED_CODEC_G726_24;
	}else if(0 == strcmp("G.726",codecName) && 32000 == bitRate){
		return SUPPORTED_CODEC_G726_32;
	}else if(0 == strcmp("G.726",codecName) && 40000 == bitRate){
		return SUPPORTED_CODEC_G726_40;
#endif
#ifdef CONFIG_RTK_VOIP_GSMFR
	}else if(0 == strcmp("GSM-FR",codecName)){
		return SUPPORTED_CODEC_GSMFR;
#endif
#ifdef CONFIG_RTK_VOIP_ILBC
	}else if(0 == strcmp("iLBC",codecName)){
		return SUPPORTED_CODEC_ILBC;
#endif
#ifdef CONFIG_RTK_VOIP_G722
	}else if(0 == strcmp("G.722",codecName)){
		return SUPPORTED_CODEC_G722;
#endif		
	}
	return -1;
}

static unsigned int getT38BitRate(unsigned char bit_rate)
{
    unsigned int ret = 0;

    if (bit_rate == 0) 
        ret = 2400;
    else if (bit_rate == 1) 
        ret = 4800;
    else if (bit_rate == 2) 
        ret = 7200;
    else if (bit_rate == 3)
        ret = 9600;
    else if (bit_rate == 4) 
        ret = 12000;
    else if (bit_rate == 5) 
        ret = 14400;

    return ret;
}

void mib_config_clean(void){
	pVoipConfig = NULL;
}

int voip_config_method_is_omci(void){
	
	if( pVoipConfig == NULL && getVoipFlash()!=0 ) {
		return 0;
	}
	
	if (pVoipConfig->rfc_flags & SIP_CONFIG_BY_OMCI)
	{
		return 1;//omci
	}else
	{
		return 0;//tr069
	}
}
/* verify the setting values */
static int verifySettingValues(idMib_t idMib, int nPort, voipCfgParam_t *voipConfig,void *pData){
//	voipCfgProxy_t *proxy = &voipConfig->ports[nPort].proxies[voipConfig->ports[nPort].default_proxy];

	switch(idMib){
#if 0 //eric: not need check string empty		
	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER:
	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER: /*pass through*/
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY:
	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY:		
//		if(proxy->enable && (0 == strlen((char *)pData) || NULL == pData)){
		/* only test string length */
		if((0 == strlen((char *)pData) || NULL == pData)){
			CWMPDBG( 0, ( stderr, "ERROR:Proxy server name should not be empty\n") );
			return VERIFY_ERROR;
		}
		break;
#endif
	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT:
	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT: /*pass through*/
	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT: /*pass through*/
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_PORT:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_PORT:
    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MIN:
    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MAX:
		if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 65535){
			CWMPDBG( 0, ( stderr, "ERROR:Port should be in the range 0~65535\n") );
			return VERIFY_ERROR;
		}
		break;
		
	case MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRATION_PERIOD:
		if(*( ( unsigned int * )pData ) < 10 || *( ( unsigned int * )pData )  > 86400){
			CWMPDBG( 0, ( stderr, "ERROR:Registration period should be in range 10~86400\n") );
			return VERIFY_ERROR;
		}
		break;

    case MIB_VOICE_PROFILE__SIP__VLANIDMARK:
    case MIB_VOICE_PROFILE__RTP__VLANIDMARK:
        if(*( ( unsigned int * )pData ) < -1){
			CWMPDBG( 0, ( stderr, "ERROR:VLAN ID should be in equal or larger than -1.\n") );
			return VERIFY_ERROR;
		}
        break;

    case MIB_VOICE_PROFILE__SIP__DSCPMARK:
    case MIB_VOICE_PROFILE__RTP__DSCPMARK:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 63){
			CWMPDBG( 0, ( stderr, "ERROR:DSCP Mark should be in range 0~63.\n") );
			return VERIFY_ERROR;
		}
        break;
        
    case MIB_VOICE_PROFILE__SIP__REGISTERRETRYINTERVAL:
    case MIB_VOICE_PROFILE__SIP__REGISTEREXPIRES:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTERRETRYINTERVAL:
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTEREXPIRES:
        if(*( ( unsigned int * )pData ) < 1){
			CWMPDBG( 0, ( stderr, "ERROR:REGISTER Expires or Retry Interval should be in equal or larger than 1.\n") );
			return VERIFY_ERROR;
		}
        break;
        
#ifdef CONFIG_RTK_VOIP_SLIC_NUM_2
	case MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT:
	{
		int otherPort=0;
		otherPort = (nPort == 0) ? 1 : 0;
		if(*( ( unsigned int * )pData ) == voipConfig->ports[otherPort].sip_port){
			CWMPDBG( 0, ( stderr, "ERROR:Sip port of this line should not be as same as that of other line\n") );
			return VERIFY_ERROR;
		}
	}
		break;
#endif /*CONFIG_RTK_VOIP_SLIC_NUM_2*/

	case MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN:
	{
		char addr[DNS_LEN];
      
#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
        char *ptrTempAddr=NULL; 
		mib_get(MIB_IP_ADDR, addr, sizeof(addr));
		ptrTempAddr = inet_ntoa(*((struct in_addr *)addr));
		if(0 == strcmp(ptrTempAddr,(char*)pData))
#else
		getMIB2Str(MIB_ADSL_LAN_IP, addr);
		if(0 == strcmp(addr,(char*)pData))
#endif
		{
			CWMPDBG( 0, ( stderr, "ERROR:Domain should not be the LAN IP\n") );
			return VERIFY_ERROR;
		}
	}	
		break;

    case MIB_VOICE_PROFILE__RTP__TELEPHONE_EVENT_PAYLOAD_TYPE:
        if(*( ( unsigned int * )pData ) < 96 || *( ( unsigned int * )pData ) > 127) {
			CWMPDBG( 0, ( stderr, "ERROR:The value should be in range 96~127\n") );
			return VERIFY_ERROR;
		}
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__TX_REPEAT_INTERVAL:
        if(*( ( unsigned int * )pData ) < 1 || *( ( unsigned int * )pData ) > 65535){
			CWMPDBG( 0, ( stderr, "ERROR:Interval should be in the range 1~65535\n") );
			return VERIFY_ERROR;
		}
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__PAYLOAD_TYPE:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 127) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 1~127\n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_VOICE_PROFILE__FAXT38__BITRATE:
        if(*( ( unsigned int * )pData ) != 2400 && 
           *( ( unsigned int * )pData ) != 4800 &&
           *( ( unsigned int * )pData ) != 7200 &&
           *( ( unsigned int * )pData ) != 9600 &&
           *( ( unsigned int * )pData ) != 12000 &&
           *( ( unsigned int * )pData ) != 14400) 
        {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be 2400/4800/7200/9600/1200/14400 \n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_VOICE_PROFILE__FAXT38__HIGH_SPEED_REDUNDANCY:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 3) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 0~3\n") );        
            return VERIFY_ERROR;
        } 
        break;

    case MIB_VOICE_PROFILE__FAXT38__LOW_SPEED_REDUNDANCY:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 5) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 0~5\n") );        
            return VERIFY_ERROR;
        } 
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_RINGCOUNT:
        if(*( ( unsigned int * )pData ) < 1 || *( ( unsigned int * )pData ) > 64) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 1~64\n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_TIMER:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 255) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 0~255\n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_NUMBER:
    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_NUMBER:
    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_NUMBER:
    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_NUMBER:
    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_CONFERENCE_URI: 
        if (strlen((char *)pData) > 32) {
            CWMPDBG( 0, ( stderr, "ERROR:The string length should be less than 32\n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHUPTIME:
    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHDOWNTIME:
        if(*( ( unsigned int * )pData ) < 80 || *( ( unsigned int * )pData ) > 1400) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 80~1400\n") );        
            return VERIFY_ERROR;
        }
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__DATASERVICEMODE:
        if(*( ( unsigned int * )pData ) < 0 || *( ( unsigned int * )pData ) > 2) {
            CWMPDBG( 0, ( stderr, "ERROR:The value should be in the range 0~2\n") );        
            return VERIFY_ERROR;
        }
        break;

	default:
		return VERIFY_OK;

	}
	return VERIFY_OK;
}

int mib_get_type1( idMib_t idMib, unsigned int nInstNum1, void *pData )
{	
	/* VoiceService.1.VoiceProfile.{ i }. */
    /* VoiceService.1.PhyInterface.{ i }. */

	voipCfgProxy_t *proxy1;/* main proxy */
	voipCfgProxy_t *proxy2;/* backup,second proxy */
	int nPort = 0;

	if( pVoipConfig == NULL && getVoipFlash()!=0 ) {	//Alex, Modified, 110913
		return 0;
	}
    
	if( IS_VOICE_PROFILE(idMib,1) && (nInstNum1 >= MAX_PROFILE_COUNT) ){
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong Voice Profile InstNum \n", __FUNCTION__, __LINE__));
		return 0;
    } else if( IS_PHYINTERFACE(idMib,1) && (nInstNum1 >= MAX_PHYINTERFACE_COUNT)) {
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong PhyInterface InstNum \n", __FUNCTION__, __LINE__));
		return 0;
	}
        
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif

	CWMPDBG( 3, ( stderr, "<%s:%d> DEBUG: getting value with InstNum1 %d idMib %d \n",
		__FUNCTION__, __LINE__, nInstNum1, idMib) );

	proxy1 = &pVoipConfig->ports[nPort].proxies[0];
	proxy2 = &pVoipConfig->ports[nPort].proxies[1];  
    
	switch( idMib ) {
	case MIB_VOICE_PROFILE__ENABLE:
		*( ( enable_t * )pData ) = TR104_ENABLE;
		break;
		
	case MIB_VOICE_PROFILE__RESET:
		*( ( boolean * )pData ) = FALSE;
		break;
		
	case MIB_VOICE_PROFILE__NUMBER_OF_LINES:
		*( ( unsigned int * )pData ) = MAX_LINE_PER_PROFILE;
		break;
		
	case MIB_VOICE_PROFILE__NAME:
	{
		char profileName[PROFILE_NAME_LEN];
		sprintf(profileName,"%s%d","profile",nInstNum1 + 1);
		strcpy( ( char * )pData, profileName);
	}
		break;
		
	case MIB_VOICE_PROFILE__SIGNALING_PROTOCOL:
		*( ( signaling_protocol_t * )pData ) = SIP;
		break;
		
	case MIB_VOICE_PROFILE__MAX_SESSIONS:
		*( ( unsigned int * )pData ) = PROFILE_MAX_SESSION;
		break;
	
	case MIB_VOICE_PROFILE__DTMF_METHOD:
		*( ( DTMF_method_t * )pData ) = pVoipConfig ->ports[nPort].dtmf_mode;
		break;
		
	case MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME:
		strcpy( ( char * )pData, SERVICE_PROVIDER );
		break;

	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER:
		strcpy( ( char * )pData, proxy1->addr);
		break;
		
	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT:
		*( ( unsigned int * )pData ) = proxy1->port;
		break;
		
	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT:
		*( ( transport_t * )pData ) = UDP;
		break;
		
	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER:
		strcpy( ( char * )pData, proxy1->RegistrarServer_addr);
		break;
		
	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT:
		*( ( unsigned int * )pData ) = proxy1->RegistrarServer_port;
		break;
		
	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT:
		*( ( transport_t * )pData ) = UDP;
		break;
		
	case MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN:
		if(NULL == proxy1->domain_name || 0 == strlen(proxy1->domain_name)){
			char addr[DNS_LEN];
       
#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
			printf( "sizeof(addr):%d\n", sizeof(addr) );
			mib_get(MIB_IP_ADDR, addr, sizeof(addr));
			strcpy( ( char * )pData, inet_ntoa(*((struct in_addr *)addr)));
#else
			getMIB2Str(MIB_ADSL_LAN_IP, addr);
			strcpy( ( char * )pData, addr);
#endif
		}else{
			strcpy( ( char * )pData, proxy1->domain_name);
		}
		break;

	case MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT:
		*( ( unsigned int * )pData ) = pVoipConfig ->ports[nPort].sip_port;
		break;

	case MIB_VOICE_PROFILE__SIP__USER_AGENT_TRANSPORT:
		*( ( transport_t * )pData ) = UDP;
		break;

	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY:
		if(proxy1->outbound_enable==1)
		strcpy( ( char * )pData, proxy1->outbound_addr);
		else
			 strcpy( ( char * )pData, "\0");
		break;

	case MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY:

		if(proxy2->outbound_enable==1)
		strcpy( ( char * )pData, proxy2->outbound_addr);
		else
			 strcpy( ( char * )pData, "\0");
		break;
		
		break;
		

	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT:
		*( ( unsigned int * )pData ) = proxy1->outbound_port;
		break;

	case MIB_VOICE_PROFILE__SIP__ORGANIZATION:
		strcpy( ( char * )pData, ORGANIZATION_HEADER );
		break;

	case MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD:
		*( ( unsigned int * )pData ) = proxy1->reg_expire;
		break;
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRATION_PERIOD:
		*( ( unsigned int * )pData ) = proxy2->reg_expire;
		break;
		
	case MIB_VOICE_PROFILE__DIGIT_MAP_ENABLE:
		 *( ( unsigned int * )pData )= pVoipConfig ->ports[nPort].digitmap_enable ;

		break;

		break;
	case MIB_VOICE_PROFILE__DIGIT_MAP:
		strcpy( ( char * )pData, (const char *)pVoipConfig ->ports[nPort].dialplan);
		break;
	
	case MIB_VOICE_PROFILE__XCT_SERVER_TYPE:
#ifdef CONFIG_CU
        if(pVoipConfig ->X_CT_servertype==0)
			strcpy( ( char * )pData, "IMS-SIP");
		else if(pVoipConfig ->X_CT_servertype==1)
		    strcpy( ( char * )pData, "SIP");
		else if(pVoipConfig ->X_CT_servertype==2) 
		    strcpy( ( char * )pData, "H.248");   
#else	    
		*( ( unsigned int * )pData )=pVoipConfig ->X_CT_servertype;
#endif
/*
		if(pVoipConfig ->X_CT_servertype==0)
			strcpy( ( char * )pData, "0");
		else 
			strcpy( ( char * )pData, "1");
*/
		break;
	case MIB_VOICE_PROFILE__XCT_START_DIGIT_TIMER:
		*( ( unsigned int * )pData ) = pVoipConfig->off_hook_alarm;
		break;
	case MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_SHORT:
		*( ( unsigned int * )pData ) = pVoipConfig->auto_dial;
		break;		
	case MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_LONG:
		*( ( unsigned int * )pData ) = pVoipConfig->InterDigitTimerLong;
		break;

	case MIB_VOICE_PROFILE__XCT_HANGING_REMINDER_TONE_TIMER:
		*( ( unsigned int * )pData ) = pVoipConfig->HangingReminderToneTimer;
		break;		
	case MIB_VOICE_PROFILE__XCT_BUSY_TONE_TIMER:

		*( ( unsigned int * )pData ) = pVoipConfig->BusyToneTimer;
		break;	

		
	case MIB_VOICE_PROFILE__XCT_NO_ANSWER_TIMER:

		*( ( unsigned int * )pData ) = pVoipConfig->NoAnswerTimer;
		break;	
	case MIB_VOICE_PROFILE__XCT_DEFAULT_PROXY_INDEX:		
		*( ( unsigned int * )pData ) = pVoipConfig ->ports[nPort].default_proxy;
			break;	

		
		
	case MIB_VOICE_PROFILE__SIP__VLANIDMARK:
	case MIB_VOICE_PROFILE__RTP__VLANIDMARK:
//		*( ( int * )pData ) = pVoipConfig->wanVlanIdVoice;
		break;

	case MIB_VOICE_PROFILE__SIP__DSCPMARK:
		*( ( unsigned int * )pData ) = pVoipConfig->sipDscp;
		break;

		
    case MIB_VOICE_PROFILE__SIP__REGISTERRETRYINTERVAL:
		*( ( unsigned int * )pData ) =  proxy1->RegisterRetryInterval;
		break;
		
   case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTERRETRYINTERVAL:
		*( ( unsigned int * )pData ) = proxy2->RegisterRetryInterval;
		break;		
		
    case MIB_VOICE_PROFILE__SIP__REGISTEREXPIRES:
        *( ( unsigned int * )pData ) =  proxy1->reg_expire;
		break;
	
    case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTEREXPIRES:
        *( ( unsigned int * )pData ) = proxy2->reg_expire;
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER:
		strcpy( ( char * )pData,proxy2->addr);
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_PORT:
		*( ( unsigned int * )pData ) = proxy2->port;
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_TRANSPORT:
		*( ( transport_t * )pData ) = UDP;
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER:
		strcpy( ( char * )pData,proxy2->RegistrarServer_addr);
		break;
	
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_PORT:
		*( ( unsigned int * )pData ) = proxy2->RegistrarServer_port;
		break;
	
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_TRANSPORT:
		*( ( transport_t * )pData ) = UDP;
		break;
		

		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY_PORT:
		*( ( unsigned int * )pData ) = proxy2->outbound_port;		
		break;
	case MIB_VOICE_PROFILE__SIP__XCT_STB_802_1PMARK:
//		*( ( unsigned int * )pData ) = pVoipConfig->wanVlanPriorityData;		
		break;
	case MIB_VOICE_PROFILE__RTP__XCT_STB_802_1PMARK:
//		*( ( unsigned int * )pData ) = pVoipConfig->wanVlanPriorityVoice;		
		break;
		
    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATSWITCH:
		if(pVoipConfig->rfc_flags & SIP_OPTION){
			*( ( boolean* )pData )=1;
		}else{
			*( ( boolean * )pData )=0;
		}
		break;
		
    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCYCLE:
		*( ( unsigned int * )pData ) = proxy1->HeartbeatCycle;		
		break;

    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCOUNT:
		*( ( unsigned int * )pData ) = proxy1->HeartbeatCount;		
		break;
		
    case MIB_VOICE_PROFILE__SIP__INVITEEXPIRES:
		*( ( unsigned int * )pData ) = proxy1->SessionUpdateTimer;		
		break;

    case MIB_VOICE_PROFILE__SIP__XCT_INVITEEXPIRES:
		*( ( unsigned int * )pData ) = proxy2->SessionUpdateTimer;		
		break;

    case MIB_VOICE_PROFILE__SIP__RM__SIP_RESPONSE_NUMBER:
        strcpy( ( char * )pData, "\0");   //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__SIP__RM__TONE:
        strcpy( ( char * )pData, "\0");   //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__SIP__RM__XCT_DURATION:
        *( ( unsigned int * )pData ) = pVoipConfig->off_hook_alarm;
        break;

//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104

    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER:
        strcpy( ( char * )pData,pVoipConfig->mgc_ip);
		break;

    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER_PORT:
		*( ( int * )pData ) = pVoipConfig->mgc_port;
		break;

    case MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER:
        strcpy( ( char * )pData,pVoipConfig->bak_mgc_ip);
		break;

    case MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER_PORT:
		*( ( int * )pData ) = pVoipConfig->bak_mgc_port;		
		break;

    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_PORT:
		*( ( int * )pData ) = pVoipConfig->mg_port;		
		break;

    case MIB_VOICE_PROFILE__H248__MESSAGE_ENCODING_TYPE:
		strcpy( ( char * )pData,pVoipConfig->mg_encoding);		
		break;

    case MIB_VOICE_PROFILE__H248__DEVICE_ID:
		strcpy( ( char * )pData,pVoipConfig->mg_name);		
		break;

    case MIB_VOICE_PROFILE__H248__DEVICE_ID_TYPE:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_deviceIdType;		
		break;

	case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID:
		strcpy( ( char * )pData,pVoipConfig->mg_phyTermID);		
		break;	

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_CONFIG_METHOD:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_phyTermConfigMethod;		
		break;

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_PREFIX:
		strcpy( ( char * )pData,pVoipConfig->mg_prefix);		
		break;

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_ADD_LEN:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_phyTermPrefixLen;		
		break;

    case MIB_VOICE_PROFILE__H248__RTP_PREFIX:
		strcpy( ( char * )pData,pVoipConfig->mg_rtp_prefix);		
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_ADD_LEN:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_rtpPrefixLen;		
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_UNIFORM:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_eph_termid_uniform;		
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_START:
		*( ( unsigned int * )pData ) = pVoipConfig->mg_rtpPrefixStart;		
		break;

    case MIB_VOICE_PROFILE__H248__THREE_HAND_SHAKE:
		*( ( unsigned int * )pData ) = pVoipConfig->three_handshake;		
		break;

    case MIB_VOICE_PROFILE__H248__LONG_TIMER:
		*( ( unsigned int * )pData ) = pVoipConfig->longTimer;		
		break;

    case MIB_VOICE_PROFILE__H248__PENDING_TIMER_INIT:
		*( ( unsigned int * )pData ) = pVoipConfig->pending_timer_init;		
		break;

    case MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER:
		*( ( unsigned int * )pData ) = pVoipConfig->retran_timer;		
		break;

    case MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT:
		*( ( unsigned int * )pData ) = pVoipConfig->retran_count;		
		break;

	case MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME:
		*( ( int * )pData ) = pVoipConfig->RetransmissionTime;		
		break;	

	case MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE:
		*( ( int * )pData ) = pVoipConfig->RetransmissionCycle;		
		break;		

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE:
		*( ( unsigned int * )pData ) = pVoipConfig->heartbeat_mode;		
		break;

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_CYCLE:
		*( ( int * )pData ) = pVoipConfig->heartbeat_cycle;		
		break;

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_COUNT:
		*( ( int * )pData ) = pVoipConfig->heartbeat_count;		
		break;

    case MIB_VOICE_PROFILE__H248__REGISTER_COUNT:
		*( ( int * )pData ) = pVoipConfig->register_count;		
		break;

    case MIB_VOICE_PROFILE__H248__REGISTER_CYCLE:
		*( ( int * )pData ) = pVoipConfig->register_cycle;		
		break;

    case MIB_VOICE_PROFILE__H248__MANDATORY_REGISTER:
		*( ( boolean * )pData ) = pVoipConfig->mandatory_register;		
		break;

    case MIB_VOICE_PROFILE__H248__AUTHENTICATION_METH_ID:
		*( ( unsigned int * )pData ) = pVoipConfig->auth_meth_id;		
		break;

	/* H.248 CU */	
	case MIB_VOICE_PROFILE__H248__DOMAIN:
        strcpy( ( char * )pData,pVoipConfig->Domain);
		break;	
	
	case MIB_VOICE_PROFILE__H248__MIDFORMAT:
        strcpy( ( char * )pData,pVoipConfig->MIDFormat);
		break;
		
	case MIB_VOICE_PROFILE__H248__CALLAGENT1:
        strcpy( ( char * )pData,pVoipConfig->mgc_ip);
		break;		
		
	case MIB_VOICE_PROFILE__H248__CALLAGENT2:
        strcpy( ( char * )pData,pVoipConfig->bak_mgc_ip);
		break;
		
	case MIB_VOICE_PROFILE__H248__LINENAME:
        strcpy( ( char * )pData,pVoipConfig->mg_phyTermID);
		break;	
		
	case MIB_VOICE_PROFILE__H248__CALLAGENTPORT1:
        *( ( int * )pData ) = pVoipConfig->mgc_port;
		break;
		
	case MIB_VOICE_PROFILE__H248__CALLAGENTPORT2:
        *( ( int * )pData ) = pVoipConfig->bak_mgc_port;
		break;
		
	case MIB_VOICE_PROFILE__H248__LOCALPORT:
        *( ( int * )pData ) = pVoipConfig->mg_port;
		break;	
		
	case MIB_VOICE_PROFILE__H248__DEVICENAME:
        strcpy( ( char * )pData,pVoipConfig->mg_name);
		break;	
		
	case MIB_VOICE_PROFILE__H248__DSCPMARK:
        *( ( int * )pData ) = pVoipConfig->DSCPMark;
		break;
		
	case MIB_VOICE_PROFILE__H248__SOFTSWITCHVENDOR:
        *( ( int * )pData ) = pVoipConfig->SoftswitchVendor;
		break;
		
	case MIB_VOICE_PROFILE__H248__SOFTSWITCHVERSION:
        strcpy( ( char * )pData,pVoipConfig->SoftswitchVersion);
		break;					

    case MIB_VOICE_PROFILE__H248__AUTH:
        *( ( boolean * )pData ) = pVoipConfig->auth;           
		break;
		
	case MIB_VOICE_PROFILE__H248__AUTHINITKEY:
        strcpy( ( char * )pData,pVoipConfig->authInitKey);     		
		break;
		
	case MIB_VOICE_PROFILE__H248__AUTHMGID:
        strcpy( ( char * )pData,pVoipConfig->authmgid);     		
		break;	
		
	case MIB_VOICE_PROFILE__H248__AUTHHEADER:
        *( ( boolean * )pData ) = pVoipConfig->authHeader;            
		break;	
		
	case MIB_VOICE_PROFILE__H248__HEADERSECURITYTYPE:
        strcpy( ( char * )pData,pVoipConfig->HeaderSecurityType);     		
		break;	
		
	case MIB_VOICE_PROFILE__H248__RFC2833ENCRYPTKEY:
        strcpy( ( char * )pData,pVoipConfig->Rfc2833EncryptKey);     		
		break;		

	case MIB_VOICE_PROFILE__H248__MSGFORMAT:
	    strcpy( ( char * )pData,pVoipConfig->MsgFormat);
        break;	
		
	case MIB_VOICE_PROFILE__H248__RESPONSEACKCTRL:
        *( ( boolean * )pData ) = pVoipConfig->ResponseAckCtrl;               
		break;	
		
	case MIB_VOICE_PROFILE__H248__MGPROVISIONALRESPTIME:
        *( ( int * )pData ) = pVoipConfig->MGProvisionalRespTime;    
        break;	
		
	case MIB_VOICE_PROFILE__H248__MTUVALUE:
        *( ( int * )pData ) = pVoipConfig->MTUValue;	
		break;
		
	case MIB_VOICE_PROFILE__H248__MSGSEGMENTATION:
	    *( ( boolean * )pData ) = pVoipConfig->MsgSegmentation;
		break;	   
		
	case MIB_VOICE_PROFILE__H248__DMNAME:
        strcpy( ( char * )pData,pVoipConfig->DMName);   	
		break;
		
	case MIB_VOICE_PROFILE__H248__DIGITMAP:
        strcpy( ( char * )pData,pVoipConfig->DigitMap);   	
		break;
		
	case MIB_VOICE_PROFILE__H248__DIGITMAPSTARTTIMER:
        *( ( int * )pData ) = pVoipConfig->DigitMapStartTimer; 	
		break;
		
	case MIB_VOICE_PROFILE__H248__DIGITMAPSHORTTIMER:
        *( ( int * )pData ) = pVoipConfig->DigitMapShortTimer; 	
		break;
		
	case MIB_VOICE_PROFILE__H248__DIGITMAPLONGTIMER:
        *( ( int * )pData ) = pVoipConfig->DigitMapLongTimer; 	
		break;						
		
	case MIB_VOICE_PROFILE__H248__HEARTBEAT_TIMER:
        *( ( int * )pData ) = pVoipConfig->HeartBeatTimer;	
		break;
	
	case MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMER:
        *( ( int * )pData ) = pVoipConfig->HeartBeatRetransTimer;	
		break;
	
	case MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMES:
        *( ( int * )pData ) = pVoipConfig->heartbeat_count;	
		break;
		
	case MIB_VOICE_PROFILE__H248__MWD_BASE_TIME:
        *( ( int * )pData ) = pVoipConfig->MWDBaseTime;	
		break;
		
	case MIB_VOICE_PROFILE__H248__MWD:
        *( ( int * )pData ) = pVoipConfig->MWD;	
		break;			
		
	case MIB_VOICE_PROFILE__H248__MGCSWITCHMETHOD:
        strcpy( ( char * )pData,pVoipConfig->MgcSwitchMethod);     		
		break;			      		

    /* H.248 CU end */
#endif		

	case MIB_VOICE_PROFILE__RTP__DSCPMARK:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			*( ( unsigned int * )pData ) = pVoipConfig ->rtpDscp;
        }
		break;

    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MIN:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].media_port;
        break;

    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MAX:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].media_port_max;
        break;

    case MIB_VOICE_PROFILE__RTP__TELEPHONE_EVENT_PAYLOAD_TYPE:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].dtmf_2833_pt;
        break;

    case MIB_VOICE_PROFILE__RTP__XCT_WAN_NAME:
        strcpy( ( char * )pData, "\0");
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__ENABLE: 
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].RTCP_Interval_enable;
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__TX_REPEAT_INTERVAL:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].RTCP_Interval*1000;
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__XCT_RTCPXR_FLAG:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].RTCP_XR;
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__ENABLE:
        if (pVoipConfig->ports[nPort].rtp_redundant_codec < 0) 
            *((boolean *)pData) = FALSE;
        else 
            *((boolean *)pData) = TRUE;
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__PAYLOAD_TYPE:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].rtp_redundant_payload_type;
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__XCT_ENABLE_FIXED_START:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].rtp_redundant_fixed_start;
        break;

	case MIB_VOICE_PROFILE__NUMBERINGPLAN__XCT_NUMBERPLAN:
		/* e8c: NOT SUPPORT */
		strcpy( ( char * )pData, "");
		break;
    case MIB_VOICE_PROFILE__XCT_G711FAX__ENABLE:
		*( ( unsigned int * )pData ) =1;	 /* always support g711 */
		break;
	
    case MIB_VOICE_PROFILE__XCT_G711FAX__CONTROLTYPE:
		if (pVoipConfig->rfc_flags&SIP_FAX_AUTO)
			strcpy( ( char * )pData, "other");
		else	
			strcpy( ( char * )pData, "all");
		break;
		
	case MIB_VOICE_PROFILE__XCT_IADDIAG__TESTSERVER:
		*( ( unsigned int * )pData ) =pEvtMsg.voiceProfileLineStatusMsg[nPort].TestServer;
		break;
		
	case MIB_VOICE_PROFILE__XCT_IADDIAG__IADDIAGNOSTICSSTATE:
		/* e8c : TBD: need check ACS server */
		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].IADDiagnosticsState==0)
			strcpy( ( char * )pData, "None");
		else if (pEvtMsg.voiceProfileLineStatusMsg[nPort].IADDiagnosticsState==1)
			strcpy( ( char * )pData, "Requested");
		else if (pEvtMsg.voiceProfileLineStatusMsg[nPort].IADDiagnosticsState==2)
			strcpy( ( char * )pData, "Complete");			
		break;
		
	case MIB_VOICE_PROFILE__XCT_IADDIAG__REGISTRESULT:
		*( ( unsigned int * )pData ) =pEvtMsg.voiceProfileLineStatusMsg[nPort].RegistResult; /* 0 is success , 1 fail*/
		break;
		
	case MIB_VOICE_PROFILE__XCT_IADDIAG__REASON:
		  *( ( unsigned int * )pData ) =pEvtMsg.voiceProfileLineStatusMsg[nPort].Reason;
		break;
		
	case MIB_CAPABILITIES__XCT_HEARTBEAT:
		*( ( boolean * )pData ) = pVoipConfig->heartbeat_enable;
		break;

    case MIB_PHYINTERFACE__INTERFACEID:
		/*e8c : only have 1 fxs */
		*( ( unsigned int * )pData ) = 1;
		break;
   case MIB_PHYINTERFACE__PHYPORT:
		{
			char phyport[2];
			sprintf(phyport,"%d",nInstNum1 + 1);
			strcpy( ( char * )pData, phyport);
   		}
		break;
#if 1 //e8c project
    case MIB_PHYINTERFACE__TESTS__TESTSTATE:
		if(pEvtMsg.e8c_autotest[nPort].TestStatus==0)
			strcpy( ( char * )pData, "None");
		else if(pEvtMsg.e8c_autotest[nPort].TestStatus==1)
			strcpy( ( char * )pData, "Requested");
		else 
			strcpy( ( char * )pData, "Complete");
		break;		
    case MIB_PHYINTERFACE__TESTS__TESTSELECTOR:
		
		if( pEvtMsg.e8c_autotest[nPort].TestSelector == 0){
				strcpy( ( char * )pData, "PhoneConnectivityTest");
		}else{ /* testselector=1 */
			strcpy( ( char * )pData, "X_"CTCOM_NAME_PREFIX"_SimulateTest");

		}
		break;
    case MIB_PHYINTERFACE__TESTS__PHONECONNECTIVITY:
		if (pEvtMsg.e8c_autotest[nPort].PhoneConnectivity==0)
			*( ( boolean * )pData ) = FALSE;
		else
			*( ( boolean * )pData ) = TRUE;
		break;
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__TESTTYPE:
		if(pEvtMsg.e8c_autotest[nPort].TestType==0)
			strcpy( ( char * )pData, "Caller");
		else if(pEvtMsg.e8c_autotest[nPort].TestType==1)
			strcpy( ( char * )pData, "Called");
		else
			strcpy( ( char * )pData, "None");
		break;
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDNUMBER:
		strcpy( ( char * )pData, pEvtMsg.e8c_autotest[nPort].callednumber);
		break;


    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMENABLE:
		*( ( unsigned int * )pData ) =  pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmEnable;	
		break;
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMNUMBER:
		if (strlen(pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmNumber)==0)
			strcpy( ( char * )pData, DIAL_DTMF_CONFIRM_NUMBER);
		else
			strcpy( ( char * )pData, pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmNumber);
		break;
	
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMRESULT:
	
		if(pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmResult==0)
			strcpy( ( char * )pData, "1");//success
		else
			strcpy( ( char * )pData, "0");//fail
		break;

    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__STATUS:

		strcpy( ( char * )pData, pEvtMsg.e8c_autotest[nPort].Simulate_Status);		
		break;
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CONCLUSION:

        if(pEvtMsg.e8c_autotest[nPort].Conclusion==0)
            strcpy( ( char * )pData, "Success");
        else
            strcpy( ( char * )pData, "Fail");
		break;

    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLERFAILREASON:

		switch(pEvtMsg.e8c_autotest[nPort].CallerFailReason){
			case CALLER_NONE:
				strcpy( ( char * )pData, "Error None");
				break;
			case CALLER_NO_DIALTONE:
				strcpy( ( char * )pData, "NoDialTone");
				break;
			case CALLER_OFFHOOK_RELEASE:
				strcpy( ( char * )pData, "OffhookRelease");
				break;
			case CALLER_AFTERDIAL_RELEASE:
				strcpy( ( char * )pData, "AfterDialRelease");
				break;
			case CALLER_NO_ANSWER:
				strcpy( ( char * )pData, "NoAnswer");
				break;
			case CALLER_KEY_ERROR:
				strcpy( ( char * )pData, "key error");
				break;				
		}	
		break;
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDFAILREASON:


		switch(pEvtMsg.e8c_autotest[nPort].CalledFailReason){
			case CALLEE_NONE:
				strcpy( ( char * )pData, "Error None");
				break;
			case CALLEE_NO_INCOMINGCALL:
				strcpy( ( char * )pData, "NoIncomingCall");
				break;
			case CALLEE_OFFHOOK_RELEASE:
				strcpy( ( char * )pData, "OffhookRelease");
				break;
			case CALLEE_NO_ANSWER:
				strcpy( ( char * )pData, "NoAnswer");
				break;
			case CALLEE_KEY_ERROR:
				strcpy( ( char * )pData, "key error");				
				break;				
		}

		break;
#endif
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__FAILEDRESPONSECODE:
	      *( ( unsigned int * )pData ) = pEvtMsg.e8c_autotest[nPort].FailedResponseCode;
        	break;

    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ISTESTONBUSY:   //run time, no MIB
        *( ( boolean* )pData ) = FALSE;
        break;

    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__CONCLUSION:     //run time, no MIB
        strcpy( ( char * )pData, "Noraml");
        break;

    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGACVOLTAGE:    //run time, no MIB
        memset(&voltage, 0, sizeof(TstVoipSLICLineVoltage));
        rtk_GetLineVoltage(nPort, &voltage);
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGACVOLTAGE:    //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABACVOLTAGE:    //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGDCVOLTAGE:    //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGDCVOLTAGE:    //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABDCVOLTAGE:    //run time, no MIB
    {
        if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGACVOLTAGE) {
            printf("vAcTip = %d\n", voltage.vAcTip);
            *( ( int * )pData ) = voltage.vAcTip;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGACVOLTAGE) {
            printf("vAcRing = %d\n", voltage.vAcRing);
            *( ( int * )pData ) = voltage.vAcRing;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABACVOLTAGE) {
            printf("vAcDiff = %d\n", voltage.vAcDiff);
            *( ( int * )pData ) = voltage.vAcDiff;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGDCVOLTAGE) {
            printf("vDcTip = %d\n", voltage.vDcTip);
            *( ( int * )pData ) = voltage.vDcTip;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGDCVOLTAGE) {
            printf("vDcRing = %d\n", voltage.vDcRing);
            *( ( int * )pData ) = voltage.vDcRing;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABDCVOLTAGE) {
            printf("vDcDiff = %d\n", voltage.vDcDiff);
            *( ( int * )pData ) = voltage.vDcDiff;
        }
    }
        break;

    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGINSULATIONRESISTANCE:     //run time, no MIB
        memset(&flt, 0, sizeof(TstVoipSLICLineResFlt));
        rtk_GetLineResflt(nPort, &flt);
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGINSULATIONRESISTANCE:     //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABINSULATIONRESISTANCE:     //run time, no MIB
    {
        if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGINSULATIONRESISTANCE) {
            printf("AGINSULATIONRESISTANCE = %d\n", flt.rtg);
            *( ( unsigned int * )pData ) = (unsigned int) flt.rtg;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGINSULATIONRESISTANCE) {
            printf("BGINSULATIONRESISTANCE = %d\n", flt.rrg);
            *( ( unsigned int * )pData ) = (unsigned int) flt.rrg;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABINSULATIONRESISTANCE) {
            printf("ABINSULATIONRESISTANCE = %d\n", flt.rtr);
            *( ( unsigned int * )pData ) = (unsigned int) flt.rtr;
        }
    } 
        break;

    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGCAPACITANCE:              //run time, no MIB
        memset(&cap, 0, sizeof(TstVoipSLICLineCap));
        rtk_GetLineCap(nPort, &cap);
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGCAPACITANCE:              //run time, no MIB
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABCAPACITANCE:              //run time, no MIB
    {
        if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGCAPACITANCE) {
            printf("AGCAPACITANCE = %d\n", cap.ctg);
            *( ( unsigned int * )pData ) = (unsigned int) cap.ctg;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGCAPACITANCE) {
            printf("BGCAPACITANCE = %d\n", cap.crg);
            *( ( unsigned int * )pData ) = (unsigned int) cap.crg;
        }
        else if (idMib == MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABCAPACITANCE) {
            printf("ABCAPACITANCE = %d\n", cap.ctr);
            *( ( unsigned int * )pData ) = (unsigned int) cap.ctr;
        }
    }
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__DIALMODE:  //need to confirm => First, DTMF, Pulse
        if (pVoipConfig->pulse_dial_det == 0)
            strcpy( ( char * )pData, "First");
        else if (pVoipConfig->pulse_dial_det == 1)
            strcpy( ( char * )pData, "Pulse");
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHUPTIME:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].flash_hook_time;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHDOWNTIME:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].flash_hook_time_min;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__REVERSEPOLEPULSE:
        if (pVoipConfig->rfc_flags & FXS_REVERSE_POLARIY_OFF) 
            *( ( boolean* )pData ) = FALSE;
        else
            *( ( boolean* )pData ) = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__KCTYPE:
        if (pVoipConfig->ports[nPort].kc_charge_mode == 0) 
            strcpy( ( char * )pData, "16Kc");
        else
            strcpy( ( char * )pData, "12Kc");;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPTRANSWHEN:
        if (pVoipConfig->ports[nPort].caller_id_mode & 0x008) 
            strcpy((char *)pData, "BeforeRing");
        else
            strcpy((char *)pData, "AfterRing");
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPREVERSEPOLE:
        if (pVoipConfig->ports[nPort].caller_id_mode & 0x040)
            *( ( boolean* )pData ) = TRUE;
        else
            *( ( boolean* )pData ) = FALSE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__MWIRINGFLAG:     
        *( ( boolean* )pData ) = pVoipConfig->ports[nPort].mwi_ring_enable;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPFORMAT:
        if (pVoipConfig->ports[nPort].cid_format == 0) 
            strcpy((char *)pData, "SDMF-FSK");
        else if (pVoipConfig->ports[nPort].cid_format == 1)
            strcpy((char *)pData, "MDMF-FSK"); 
        else
            strcpy((char *)pData, "DTMF"); 
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__RINGDCVOLTAGEOVERLAPPED:     //mib not ready yet
        *( ( unsigned int * )pData ) = 0;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__DATASERVICEMODE:             //it is not a suitable option
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].fax_modem_det;
        break;

	case MIB_VOICE_PROFILE__FAXT38__ENABLE:
		  *( ( boolean* )pData )=pVoipConfig ->ports[nPort].useT38 ;
		break;

    case MIB_VOICE_PROFILE__FAXT38__BITRATE:
        *( ( unsigned int * )pData ) = getT38BitRate(pVoipConfig->ports[nPort].T38MaxRate);
        break;

    case MIB_VOICE_PROFILE__FAXT38__TCF_METHOD:
        if (pVoipConfig->ports[nPort].T38RateMgt == 2) 
            strcpy( ( char * )pData, "Network");
        else 
            strcpy( ( char * )pData, "Local");       
        break;

    case MIB_VOICE_PROFILE__FAXT38__HIGH_SPEED_REDUNDANCY:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].T38DuplicateNum;
        break;

    case MIB_VOICE_PROFILE__FAXT38__LOW_SPEED_REDUNDANCY:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].T30DuplicateNum;
        break;
	
    case MIB_VOICE_PROFILE__FAXT38__XCT_PORT_ADD2:
        *( ( boolean* )pData ) = pVoipConfig->ports[nPort].FaxPortAdd2;
        break;

	case MIB_VOICE_PROFILE__XCT_DIGIT_MAP_MATCH_MODE:
#ifdef CONFIG_CU
		if (pVoipConfig->rfc_flags&SIP_DIGIT_MAP_MODE_MIN)	
			strcpy( ( char * )pData, "Min");//min
		else	
			strcpy( ( char * )pData, "Max");//max
#else
		if (pVoipConfig->rfc_flags&SIP_DIGIT_MAP_MODE_MIN)
			strcpy( ( char * )pData, "1");//min
		else	
			strcpy( ( char * )pData, "0");//max
#endif			
		break;

	case MIB_VOICE_PROFILE__XCT_RING_VOLTAGE:
		*( ( unsigned int * )pData ) =pVoipConfig->ringVoltageFreq;
		break;
	case MIB_VOICE_PROFILE__XCT_CALL_ID_MSG_TYPE:
		strcpy( ( char * )pData, "MDMF"); //temp
		break;

    case MIB_VOICE_PROFILE__XCT_WAN_NAME:
        strcpy( ( char * )pData, "\0");       //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__XCT_INTERFACE_STATE:
        strcpy( ( char * )pData, "\0");       //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__XCT_FAXMODEM__FAXNEGO:
        if (pVoipConfig->rfc_flags & SIP_FAX_AUTO)
			*( ( int * )pData ) = 0;
		else 
			*( ( int * )pData ) = 1;
        break;

	case MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL:
	 	strcpy( ( char * )pData, (char *)pVoipConfig ->ports[nPort].digitmap_special);
		break;
	case MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL_ENABLE:
			*( ( boolean * )pData ) =  pVoipConfig ->ports[nPort].digitmap_special_enable; 
		break;		
		
	case MIB_VOICE_PROFILE__SIP__XCT_CALL_ID_TIME_POLICY:
        if (pVoipConfig->sync_phone_time == 0)                       
			*( ( unsigned int * )pData ) = 0;
        else if (pVoipConfig->sync_phone_time == 1)
            *( ( unsigned int * )pData ) = 1; 
		else 
			*( ( unsigned int * )pData ) = 2;
		break;

	default:
		return 0;	/* range error */
		break;
	}

	return 1;
}

int mib_set_type1( idMib_t idMib, unsigned int nInstNum1, void *pData )
{	
	/* VoiceService.1.VoiceProfile.{ i }. */
	//voipCfgParam_t voipConfig;
	//voipCfgParam_t * const pVoipConfig = &g_pVoIPShare ->voip_cfg;
	voipCfgProxy_t *proxy1; // main proxy
	voipCfgProxy_t *proxy2; // second proxy

	
	int nPort=0;
	bDirtyBit = FALSE;
	//VoIP_server_type_change = FALSE;

	if( pVoipConfig == NULL && getVoipFlash()!=0 ) {	//Alex, Modified, 110913
		return 0;
	}    
    
	if( IS_VOICE_PROFILE(idMib,1) && (nInstNum1 >= MAX_PROFILE_COUNT) ){
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong Voice Profile InstNum \n", __FUNCTION__, __LINE__));
		return 0;
    } else if( IS_PHYINTERFACE(idMib,1) && (nInstNum1 >= MAX_PHYINTERFACE_COUNT)) {
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong PhyInterface InstNum \n", __FUNCTION__, __LINE__));
		return 0;
	}
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif
 	proxy1 = &pVoipConfig ->ports[nPort].proxies[0];
	proxy2 = &pVoipConfig ->ports[nPort].proxies[1];

	
	switch( idMib ) {	
	case MIB_VOICE_X_CTCOM_NAME_PREFIX_SERVICELIST:
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__DTMF_METHOD:
		pVoipConfig ->ports[nPort].dtmf_mode = *( ( DTMF_method_t * )pData );
		bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			strcpy( proxy1->addr, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy1->port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT:
    case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT:
    case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_TRANSPORT:
    case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_TRANSPORT:
   		// unable to set
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;

	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			strcpy( proxy1->RegistrarServer_addr, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy1->RegistrarServer_port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;
	
	case MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			strcpy( proxy1->domain_name, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			pVoipConfig ->ports[nPort].sip_port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY:
			strcpy( proxy1->outbound_addr, ( char * )pData );
			if(0==strlen((char *)pData)){
				proxy1->outbound_enable=0;
			}else{
				proxy1->outbound_enable=1;
		}
            bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY:
		strcpy( proxy2->outbound_addr, ( char * )pData );
		if(0==strlen((char *)pData)){
			proxy2->outbound_enable=0;
		}else{
			proxy2->outbound_enable=1;
		}
		bDirtyBit = TRUE; 
		break;
	case MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy1->outbound_port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__SIP__ORGANIZATION:
   		//unable to set
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;
		
	case MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy1->reg_expire = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRATION_PERIOD:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy2->reg_expire = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;
		
	case MIB_VOICE_PROFILE__DIGIT_MAP_ENABLE:

		pVoipConfig ->ports[nPort].digitmap_enable =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
		
	case MIB_VOICE_PROFILE__DIGIT_MAP:		

		if(strlen((char *)pData)<MAX_DIALPLAN_LENGTH){
			strcpy( (char *)pVoipConfig ->ports[nPort].dialplan, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;
		
	case MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL_ENABLE:

		pVoipConfig ->ports[nPort].digitmap_special_enable =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
		
	case MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL:		

		if(strlen((char *)pData)<PSTN_ROUTING_PREFIX_LEN){
			strcpy( (char *)pVoipConfig ->ports[nPort].digitmap_special, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;
		
	case MIB_VOICE_PROFILE__XCT_SERVER_TYPE:
		{
	/*
			if(strcmp(( char * )pData ,"0")==0)
				pVoipConfig ->X_CT_servertype=0;
			else
	            pVoipConfig->X_CT_servertype = 1;
	*/
			unsigned char inform_type = 0;
#ifdef CONFIG_CU
            int old_servertype=pVoipConfig ->X_CT_servertype;
            char new_servertype[16];
      
            strncpy(new_servertype,( char * )pData,sizeof(new_servertype));
						
			mib_get(PROVINCE_CWMP_INFORM_TYPE, &inform_type);
			//sip not need change,   only sip to h248 need restart call manager.
			if(old_servertype<=1){
				if(strcmp(new_servertype, "H.248")==0){
					VoIP_server_type_change = TRUE;	
				}
			}
		
			if(old_servertype==2){
				if(strcmp(new_servertype, "IMS-SIP")==0 || strcmp(new_servertype, "SIP")==0){
					VoIP_server_type_change = TRUE;
				}
			}

            if(strcmp(( char * )pData, "IMS-SIP")==0)
    			pVoipConfig ->X_CT_servertype = 0;
    		else if(strcmp(( char * )pData, "SIP")==0)
    		    pVoipConfig ->X_CT_servertype = 1;
    		else if(strcmp(( char * )pData, "H.248")==0) 
    		    pVoipConfig ->X_CT_servertype = 2;
		
#else	    
			int old_servertype=pVoipConfig ->X_CT_servertype;
			int new_servertype=*( ( unsigned int * )pData );
			/* 0 = ims , 1=softswitch sip , 2=h248 */
						
			mib_get(PROVINCE_CWMP_INFORM_TYPE, &inform_type);
			//sip not need change,   only sip to h248 need restart call manager.
			if(old_servertype<=1){
				if(new_servertype == 2){
					VoIP_server_type_change = TRUE;	
				}
			}
			
			if(old_servertype==2){
				if(new_servertype<=1){
					VoIP_server_type_change = TRUE;
				}
			}

			pVoipConfig ->X_CT_servertype= *( ( unsigned int * )pData );
#endif	
			
			if (inform_type == CWMP_INFORM_TYPE_GUD) {
				if(*( ( unsigned int * )pData ) == IMS_SIP){
					proxy1->enable|=PROXY_SUBSCRIBE;
					proxy2->enable|=PROXY_SUBSCRIBE;
				}else{
					proxy1->enable &= ~PROXY_SUBSCRIBE;
		            		proxy2->enable &= ~PROXY_SUBSCRIBE;
				}
			}
		}
		break;
	case MIB_VOICE_PROFILE__XCT_START_DIGIT_TIMER:
		pVoipConfig->off_hook_alarm =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_SHORT:
		pVoipConfig ->auto_dial =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_LONG:
		pVoipConfig ->InterDigitTimerLong =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_HANGING_REMINDER_TONE_TIMER:
		pVoipConfig ->HangingReminderToneTimer =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
		
	case MIB_VOICE_PROFILE__XCT_BUSY_TONE_TIMER:
		pVoipConfig ->BusyToneTimer =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_NO_ANSWER_TIMER:
		pVoipConfig ->NoAnswerTimer =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_DEFAULT_PROXY_INDEX:
			pVoipConfig ->ports[nPort].default_proxy =  *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
			break;	

	case MIB_VOICE_PROFILE__RTP__VLANIDMARK:
	case MIB_VOICE_PROFILE__SIP__VLANIDMARK:
    	if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){	
//			pVoipConfig ->wanVlanIdVoice =  *( ( int * )pData );
			bDirtyBit = TRUE;
        }
		break;
	
	case MIB_VOICE_PROFILE__SIP__DSCPMARK:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			pVoipConfig ->sipDscp =  *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
        }
		break;
	
    case MIB_VOICE_PROFILE__SIP__REGISTERRETRYINTERVAL:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy1->RegisterRetryInterval = *( ( unsigned int * )pData );
            //	proxy2->RegisterRetryInterval = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
        }
		break;
	
    case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTERRETRYINTERVAL:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy2->RegisterRetryInterval = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
        }
		break;
		
    case MIB_VOICE_PROFILE__SIP__REGISTEREXPIRES:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
            /* main and backup proxy use the same value */
			proxy1->reg_expire = *( ( unsigned int * )pData );
            //	proxy2->reg_expire = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
        }
		break;
		

    case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTEREXPIRES:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
            /* main and backup proxy use the same value */
			proxy2->reg_expire = *( ( unsigned int * )pData );
            //	proxy2->reg_expire = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
        }
		break;
		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER:		
		/* backup proxy server */
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			strcpy( proxy2->addr, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER:
		/* backup proxy server */
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			strcpy( proxy2->RegistrarServer_addr, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;
	case MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_PORT:		
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy2->port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;		
	case MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy2->RegistrarServer_port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;		


	case MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY_PORT:
		if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)){
			proxy2->outbound_port = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
		break;

	case MIB_VOICE_PROFILE__RTP__XCT_STB_802_1PMARK:	
	case MIB_VOICE_PROFILE__SIP__XCT_STB_802_1PMARK:
		/* Need check */
	//	pVoipConfig ->wanVlanPriorityVoice =  *( ( int * )pData );
		bDirtyBit = TRUE;
		break;
		
    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATSWITCH:
		/* main and backup proxy use the same value  , for OPTION message */
		if(*( ( int * )pData ) == 1){
			pVoipConfig->rfc_flags |= SIP_OPTION;
		}else{
			pVoipConfig->rfc_flags &= ~ SIP_OPTION;
		}
		bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCYCLE:
		/* main and backup proxy use the same value */
		proxy1->HeartbeatCycle = *( ( int * )pData );
        //	proxy2->HeartbeatCycle = *( ( int * )pData );
		bDirtyBit = TRUE;
		break;
		
    case MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCOUNT:
		/* main and backup proxy use the same value */
		proxy1->HeartbeatCount = *( ( int * )pData );
        //	proxy2->HeartbeatCount = *( ( int * )pData );
		bDirtyBit = TRUE;
		break;
		
    case MIB_VOICE_PROFILE__SIP__INVITEEXPIRES:
		/* main and backup proxy use the same value */
		proxy1->SessionUpdateTimer = *( ( int * )pData );
		bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__SIP__XCT_INVITEEXPIRES:
		/* main and backup proxy use the same value */
		proxy2->SessionUpdateTimer = *( ( int * )pData );
		bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__SIP__RM__SIP_RESPONSE_NUMBER:
        //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__SIP__RM__TONE:
        //run time, don't need MIB
        break;

    case MIB_VOICE_PROFILE__SIP__RM__XCT_DURATION:
        pVoipConfig->off_hook_alarm = *( ( unsigned int * )pData );
        bDirtyBit = TRUE;
        break;

//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104
    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER:
        strcpy( pVoipConfig->mgc_ip, ( char * )pData );
        if( pVoipConfig->X_CT_servertype != 2){
            pVoipConfig->X_CT_servertype = 2; //H.248
            VoIP_server_type_change=1;
        }
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER_PORT:
		pVoipConfig->mgc_port = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER:
        strcpy( pVoipConfig->bak_mgc_name,( char * )pData);
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER_PORT:
		 pVoipConfig->bak_mgc_port = *( ( int * )pData );
         bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_PORT:
		 pVoipConfig->mg_port = *( ( int * )pData );  
         bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__MESSAGE_ENCODING_TYPE:
		strcpy( pVoipConfig->mg_encoding,( char * )pData);	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__DEVICE_ID:
		strcpy( pVoipConfig->mg_name,( char * )pData);	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__DEVICE_ID_TYPE:
		 pVoipConfig->mg_deviceIdType = *( ( unsigned int * )pData );	
         bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID:
		 strcpy( pVoipConfig->mg_phyTermID,( char * )pData);	
        if (strlen(pVoipConfig->mg_phyTermID) > 0)
            pVoipConfig->mg_phyTermConfigMethod = 1;
		bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_CONFIG_METHOD:
		 pVoipConfig->mg_phyTermConfigMethod = *( ( unsigned int * )pData );	
         bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_PREFIX:
		strcpy( pVoipConfig->mg_prefix,( char * )pData);	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_ADD_LEN:
		 pVoipConfig->mg_phyTermPrefixLen = *( ( unsigned int * )pData );	
         bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__RTP_PREFIX:
		strcpy( pVoipConfig->mg_rtp_prefix,( char * )pData);
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_ADD_LEN:
		 pVoipConfig->mg_rtpPrefixLen = *( ( unsigned int * )pData );	
         bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_UNIFORM:
		pVoipConfig->mg_eph_termid_uniform = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_START:
		pVoipConfig->mg_rtpPrefixStart = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__THREE_HAND_SHAKE:
		pVoipConfig->three_handshake = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__LONG_TIMER:
		pVoipConfig->longTimer = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__PENDING_TIMER_INIT:
		pVoipConfig->pending_timer_init = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER:
		pVoipConfig->retran_timer = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT:
		pVoipConfig->retran_count = *( ( unsigned int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME:
		pVoipConfig->RetransmissionTime = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE:
		pVoipConfig->RetransmissionCycle = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;	

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE:
		pVoipConfig->heartbeat_mode = *( ( unsigned int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_CYCLE:
		pVoipConfig->heartbeat_cycle = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__HEARTBEAT_COUNT:
		pVoipConfig->heartbeat_count = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__REGISTER_COUNT:
		pVoipConfig->register_count = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__REGISTER_CYCLE:
		pVoipConfig->register_cycle = *( ( int * )pData );	
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__MANDATORY_REGISTER:
		pVoipConfig->mandatory_register = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__AUTHENTICATION_METH_ID:
		pVoipConfig->auth_meth_id = *( ( unsigned int * )pData );
        bDirtyBit = TRUE;
		break;

	/* H.248 CU */
	case MIB_VOICE_PROFILE__H248__DOMAIN:
        strcpy( pVoipConfig->Domain,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MIDFORMAT:
        strcpy( pVoipConfig->MIDFormat, ( char * )pData );
        /* mapping to mg_deviceIdType for customer testing */
        if (strcmp(pVoipConfig->MIDFormat, "IP") == 0) {
            pVoipConfig->mg_deviceIdType = 0;
        }
        else if (strcmp(pVoipConfig->MIDFormat, "DomainName") == 0) {
            pVoipConfig->mg_deviceIdType = 1;
        }
        else if (strcmp(pVoipConfig->MIDFormat, "DeviceName") == 0) {
            pVoipConfig->mg_deviceIdType = 2;
        }
        else
            pVoipConfig->mg_deviceIdType = 0;
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__CALLAGENT1:
        strcpy( pVoipConfig->mgc_ip, ( char * )pData );
        if( pVoipConfig->X_CT_servertype != 2){
            pVoipConfig->X_CT_servertype = 2; //H.248
            VoIP_server_type_change=1;
        }
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__CALLAGENT2:
        strcpy( pVoipConfig->bak_mgc_ip,( char * )pData);
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__H248__LINENAME:
        strcpy( pVoipConfig->mg_phyTermID,( char * )pData);
        if (strlen(pVoipConfig->mg_phyTermID) > 0)
            pVoipConfig->mg_phyTermConfigMethod = 1;
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__CALLAGENTPORT1:
        pVoipConfig->mgc_port = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__CALLAGENTPORT2:
        pVoipConfig->bak_mgc_port = *( ( int * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__LOCALPORT:
        pVoipConfig->mg_port = *( ( int * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__DEVICENAME:
        strcpy( pVoipConfig->mg_name,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DSCPMARK:
        pVoipConfig->DSCPMark = *( ( int * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__SOFTSWITCHVENDOR:
        pVoipConfig->SoftswitchVendor = *( ( int * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__SOFTSWITCHVERSION:
        strcpy( pVoipConfig->SoftswitchVersion,( char * )pData);
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__H248__AUTH:
        pVoipConfig->auth = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__AUTHINITKEY:
        strcpy( pVoipConfig->authInitKey,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__AUTHMGID:
        strcpy( pVoipConfig->authmgid,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__AUTHHEADER:
        pVoipConfig->authHeader = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__HEADERSECURITYTYPE:
        strcpy( pVoipConfig->HeaderSecurityType,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__RFC2833ENCRYPTKEY:
        strcpy( pVoipConfig->Rfc2833EncryptKey,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MSGFORMAT:
        strcpy( pVoipConfig->MsgFormat,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__RESPONSEACKCTRL:
        pVoipConfig->ResponseAckCtrl = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MGPROVISIONALRESPTIME:
        pVoipConfig->MGProvisionalRespTime = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MTUVALUE:
        pVoipConfig->MTUValue = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MSGSEGMENTATION:
        pVoipConfig->MsgSegmentation = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DMNAME:
        strcpy( pVoipConfig->DMName,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DIGITMAP:
        strcpy( pVoipConfig->DigitMap,( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DIGITMAPSTARTTIMER:
        pVoipConfig->DigitMapStartTimer = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DIGITMAPSHORTTIMER:
        pVoipConfig->DigitMapShortTimer = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__DIGITMAPLONGTIMER:
        pVoipConfig->DigitMapLongTimer = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__HEARTBEAT_TIMER:
        pVoipConfig->HeartBeatTimer = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMER:
        pVoipConfig->HeartBeatRetransTimer = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMES:
        pVoipConfig->heartbeat_count = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MWD_BASE_TIME:
        pVoipConfig->MWDBaseTime = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MWD:
        pVoipConfig->MWD = *( ( int * )pData );
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__H248__MGCSWITCHMETHOD:
        strcpy( pVoipConfig->MgcSwitchMethod,( char * )pData);  
        bDirtyBit = TRUE; 	
		break;			  	
    /* H.248 CU end */
#endif
		
	case MIB_VOICE_PROFILE__RTP__DSCPMARK:
        if(VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
		pVoipConfig ->rtpDscp =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
        }
		break;

    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MIN:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].media_port = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
		break;

    case MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MAX:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].media_port_max = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__RTP__TELEPHONE_EVENT_PAYLOAD_TYPE:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].dtmf_2833_pt = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__ENABLE:
        pVoipConfig->ports[nPort].RTCP_Interval_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__TX_REPEAT_INTERVAL:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].RTCP_Interval = *( ( unsigned int * )pData )/1000;
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__RTP__RTCP__XCT_RTCPXR_FLAG:
        pVoipConfig->ports[nPort].RTCP_XR = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__ENABLE:
        if (*((boolean *)pData) == FALSE)
            pVoipConfig->ports[nPort].rtp_redundant_codec = -1;
        else
            pVoipConfig->ports[nPort].rtp_redundant_codec = 0;  //it is not a suitable option
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__PAYLOAD_TYPE:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].rtp_redundant_payload_type = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__RTP__REDUNDANCY__XCT_ENABLE_FIXED_START:
        if (*((boolean *)pData) == TRUE)
            pVoipConfig->ports[nPort].rtp_redundant_fixed_start = 1;
        else
            pVoipConfig->ports[nPort].rtp_redundant_fixed_start = 0;  
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__FAXT38__ENABLE:
		pVoipConfig ->ports[nPort].useT38 =  *( ( boolean * )pData );
		bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__FAXT38__BITRATE:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) 
        {
            if (*((unsigned int *)pData) == 2400) 
                pVoipConfig->ports[nPort].T38MaxRate = 0;
            else if (*((unsigned int *)pData) == 4800)
                pVoipConfig->ports[nPort].T38MaxRate = 1;
            else if (*((unsigned int *)pData) == 7200)
                pVoipConfig->ports[nPort].T38MaxRate = 2;
            else if (*((unsigned int *)pData) == 9600)
                pVoipConfig->ports[nPort].T38MaxRate = 3;
            else if (*((unsigned int *)pData) == 12000)
                pVoipConfig->ports[nPort].T38MaxRate = 4;
            else if (*((unsigned int *)pData) == 14400)
                pVoipConfig->ports[nPort].T38MaxRate = 5;
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__FAXT38__TCF_METHOD:
        if (strcmp(( char * )pData, "Network") == 0)       
			pVoipConfig->ports[nPort].T38RateMgt = 2;             
        else if (strcmp(( char * )pData, "Local") == 0)
            pVoipConfig->ports[nPort].T38RateMgt = 1;
        bDirtyBit = TRUE;         
        break;

    case MIB_VOICE_PROFILE__FAXT38__HIGH_SPEED_REDUNDANCY:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].T38DuplicateNum = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__FAXT38__LOW_SPEED_REDUNDANCY:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].T30DuplicateNum = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__FAXT38__XCT_PORT_ADD2:
        if (*((boolean *)pData) == TRUE)
            pVoipConfig->ports[nPort].FaxPortAdd2 = 1;
        else
            pVoipConfig->ports[nPort].FaxPortAdd2 = 0;  
        bDirtyBit = TRUE;     
        break;

    case MIB_PHYINTERFACE__INTERFACEID:
        //FIXME, Alex
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;
        
    case MIB_PHYINTERFACE__PHYPORT:
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;
	case MIB_PHYINTERFACE__X_STATS__RESETSTATISTICS:
		if(*( ( int * )pData ) == 1)
			pEvtMsg.PoorQualityResetStatistics[nPort]=1;
		else
			pEvtMsg.PoorQualityResetStatistics[nPort]=0;
		break;
#if 1//e8c project
    case MIB_PHYINTERFACE__TESTS__TESTSTATE:
		/*When writing, the only allowed value is Requested.*/
		if (strcmp(( char * )pData ,"Requested")==0){
			pEvtMsg.e8c_autotest[nPort].TestStatus=1;
			pEvtMsg.e8c_autotest[nPort].enable=1;
		}else{
			pEvtMsg.e8c_autotest[nPort].TestStatus=0; /*None*/
		}
		break;
	
    case MIB_PHYINTERFACE__TESTS__TESTSELECTOR:

		if(strcmp(( char * )pData ,"PhoneConnectivityTest")==0){
			pEvtMsg.e8c_autotest[nPort].TestSelector=0;
		}else if (strcmp(( char * )pData ,"X_"CTCOM_NAME_PREFIX"_SimulateTest")==0){
			pEvtMsg.e8c_autotest[nPort].TestSelector=1;
		}
		break;	
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__TESTTYPE:
		if(strcmp(( char * )pData,"Caller")==0){
			pEvtMsg.e8c_autotest[nPort].TestType=0;
		}else if(strcmp(( char * )pData,"Called")==0){
		/* called must enable */
			pEvtMsg.e8c_autotest[nPort].enable=1;
			pEvtMsg.e8c_autotest[nPort].TestType=1;
		}else if(strcmp(( char * )pData,"None")==0){/*None*/
			pEvtMsg.e8c_autotest[nPort].TestType=2;
		}else{
			pEvtMsg.e8c_autotest[nPort].enable=0;
		}
		break;

		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDNUMBER:
		strcpy( pEvtMsg.e8c_autotest[nPort].callednumber,( char * )pData);
		break;
		
		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMENABLE:
		pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmEnable=*( ( unsigned int * )pData );
		break;


		
    case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMNUMBER:
		strcpy( pEvtMsg.e8c_autotest[nPort].DailDTMFConfirmNumber,( char * )pData);
		break;
#endif
    case MIB_VOICE_PROFILE__XCT_G711FAX__CONTROLTYPE:
		if(strcmp(( char * )pData,"all")==0){
			pVoipConfig->rfc_flags &= ~SIP_FAX_AUTO;
		}else if(strcmp(( char * )pData,"other")==0){
			pVoipConfig->rfc_flags |= SIP_FAX_AUTO;
		}
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_IADDIAG__IADDIAGNOSTICSSTATE:
		/*When writing, the only allowed value is Requested.*/
		if (strcmp(( char * )pData ,"Requested")==0){
			pEvtMsg.voiceProfileLineStatusMsg[nPort].IADDiagnosticsState=1;
		}else{
			pEvtMsg.voiceProfileLineStatusMsg[nPort].IADDiagnosticsState=0; /*None*/
		}
		break;
	case MIB_VOICE_PROFILE__XCT_IADDIAG__TESTSERVER:
		pEvtMsg.voiceProfileLineStatusMsg[nPort].TestServer =  *( ( unsigned int * )pData ); /*0 None 1: main proxy 2: backup proxy*/
		break;
	case MIB_VOICE_PROFILE__XCT_DIGIT_MAP_MATCH_MODE:
#ifdef CONFIG_CU
		if(strcmp(( char * )pData,"Max")==0){
			pVoipConfig->rfc_flags &= ~SIP_DIGIT_MAP_MODE_MIN;
		}else if(strcmp(( char * )pData,"Min")==0){
			pVoipConfig->rfc_flags |= SIP_DIGIT_MAP_MODE_MIN;
		}
#else
		if(strcmp(( char * )pData,"0")==0){//Max
			pVoipConfig->rfc_flags &= ~SIP_DIGIT_MAP_MODE_MIN;
		}else if(strcmp(( char * )pData,"1")==0){//Min
			pVoipConfig->rfc_flags |= SIP_DIGIT_MAP_MODE_MIN;
		}
#endif		
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__XCT_RING_VOLTAGE:
		pVoipConfig->ringVoltageFreq=*( ( unsigned int * )pData );
		break;
	case MIB_VOICE_PROFILE__XCT_CALL_ID_MSG_TYPE:
		CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;

    case MIB_VOICE_PROFILE__XCT_FAXMODEM__FAXNEGO:
        if (*( ( int * )pData ) == 0)
            pVoipConfig->rfc_flags |= SIP_FAX_AUTO;
		else 
            pVoipConfig->rfc_flags &= ~SIP_FAX_AUTO;
        break;

	case MIB_VOICE_PROFILE__SIP__XCT_CALL_ID_TIME_POLICY:
		if(*( ( int * )pData ) == 0){
            pVoipConfig->sync_phone_time = 0;
		}else if(*( ( int * )pData ) == 1){
            pVoipConfig->sync_phone_time = 1;
		}else if(*( ( int * )pData ) == 2){
            pVoipConfig->sync_phone_time = 2;
		}
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__NUMBERINGPLAN__XCT_NUMBERPLAN:
		/* ebc board not need support fxo */
	case MIB_VOICE_PROFILE__XCT_G711FAX__ENABLE:
	case MIB_VOICE_PROFILE__XCT_IADDIAG__REGISTRESULT:
	case MIB_VOICE_PROFILE__XCT_IADDIAG__REASON:	
	case MIB_VOICE_PROFILE__ENABLE:
	case MIB_VOICE_PROFILE__RESET:
	case MIB_VOICE_PROFILE__NUMBER_OF_LINES:
	case MIB_VOICE_PROFILE__NAME:
	case MIB_VOICE_PROFILE__SIGNALING_PROTOCOL:
	case MIB_VOICE_PROFILE__MAX_SESSIONS:
	case MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME:
	case MIB_PHYINTERFACE__TESTS__PHONECONNECTIVITY:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMRESULT:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__STATUS:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CONCLUSION:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLERFAILREASON:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDFAILREASON:
	case MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__FAILEDRESPONSECODE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ISTESTONBUSY:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__CONCLUSION:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGACVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGACVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABACVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGDCVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGDCVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABDCVOLTAGE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGINSULATIONRESISTANCE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGINSULATIONRESISTANCE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABINSULATIONRESISTANCE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGCAPACITANCE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGCAPACITANCE:
    case MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABCAPACITANCE:
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		// unable to set, //Read Only, we might needn't to write
		break;
        
    case MIB_PHYINTERFACE__XCT_EXTEND__DIALMODE:  //need to confirm => First, DTMF, Pulse
        if (strcmp((char *)pData, "First") == 0) 
            pVoipConfig->pulse_dial_det = 0;
        else if (strcmp((char *)pData, "DTMF") == 0) 
            pVoipConfig->pulse_dial_det = 0;
        else if (strcmp((char *)pData, "Pulse") == 0) 
            pVoipConfig->pulse_dial_det = 1;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHUPTIME:  
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].flash_hook_time = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHDOWNTIME:  
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].flash_hook_time_min = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__REVERSEPOLEPULSE:
        if (*( ( boolean* )pData ) == FALSE)
            pVoipConfig->rfc_flags |= FXS_REVERSE_POLARIY_OFF;
        else
            pVoipConfig->rfc_flags &= ~FXS_REVERSE_POLARIY_OFF;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__KCTYPE:     
        if (strcmp((char *)pData, "16Kc") == 0) 
            pVoipConfig->ports[nPort].kc_charge_mode = 0;
        else if (strcmp((char *)pData, "12Kc") == 0) 
            pVoipConfig->ports[nPort].kc_charge_mode = 1;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPTRANSWHEN:   
        if (strcmp((char *)pData, "BeforeRing") == 0) 
            pVoipConfig->ports[nPort].caller_id_mode |= 0x008;
        else if (strcmp((char *)pData, "AfterRing") == 0) 
            pVoipConfig->ports[nPort].caller_id_mode &= ~0x008;
        bDirtyBit = TRUE;
        break;
        
    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPREVERSEPOLE:
        if (*( ( boolean* )pData ) == TRUE)
            pVoipConfig->ports[nPort].caller_id_mode |= 0x040;
        else
            pVoipConfig->ports[nPort].caller_id_mode &= ~0x040;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__MWIRINGFLAG: 
        pVoipConfig->ports[nPort].mwi_ring_enable = *( ( boolean* )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__CLIPFORMAT:
        if (strcmp((char *)pData, "SDMF-FSK") == 0) 
            pVoipConfig->ports[nPort].cid_format = 0;
        else if (strcmp((char *)pData, "MDMF-FSK") == 0) 
            pVoipConfig->ports[nPort].cid_format = 1;
        else if (strcmp((char *)pData, "MDMF-FSK") == 0) 
            pVoipConfig->ports[nPort].cid_format = 2;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__RINGDCVOLTAGEOVERLAPPED:     //mib not ready yet
        //*( ( unsigned int * )pData ) = 0;
        bDirtyBit = TRUE;
        break;

    case MIB_PHYINTERFACE__XCT_EXTEND__DATASERVICEMODE:             //it is not suitable option
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            pVoipConfig->ports[nPort].fax_modem_det = *( ( unsigned int * )pData );
            bDirtyBit = TRUE;
        }
        break;
      
	default:
		return 0;	/* range error */
		break;
	}

	if(TRUE == bDirtyBit){
//        voip_flash_set(pVoipConfig); //move to cwmp-client


        apply_add( CWMP_PRI_N, apply_voip, CWMP_RESTART, 0, NULL, 0 ); 
    }
	return 0;
}

int mib_get_type2( idMib_t idMib, unsigned int nInstNum1, 
				   unsigned int nInstNum2,
				   void *pData )
{
	/* VoiceService.1.VoiceProfile.{ i }.Line.{i}. */
    /* VoiceService.1.PhyInterface.{ i }.X_Stats.PoorQualityList.{ i }. */

	//voipCfgParam_t voipConfig;
	//voipCfgParam_t * const pVoipConfig = &g_pVoIPShare ->voip_cfg;
	voipCfgProxy_t *proxy1;
	voipCfgProxy_t *proxy2;
	int nPort = 0;
	//char sipURI[100];

    if( pVoipConfig == NULL && getVoipFlash()!=0 ) {    //Alex, Modified, 110913
        return 0;
    }

	if( IS_VOICE_PROFILE(idMib,1) && (nInstNum1 >= MAX_PROFILE_COUNT || nInstNum2 >= MAX_LINE_PER_PROFILE ) ){
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong Voice Profile or Line InstNum \n", __FUNCTION__, __LINE__));
		return 0;
    } else if( IS_PHYINTERFACE(idMib,1) && (nInstNum1 >= MAX_PHYINTERFACE_COUNT || nInstNum2 >= MAX_POORQUALITYLIST_COUNT)) {
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong PhyInterface or PoorQualityList InstNum \n", __FUNCTION__, __LINE__));
		return 0;
	}
	
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif
 	proxy1 = &pVoipConfig ->ports[nPort].proxies[0];
	proxy2 = &pVoipConfig ->ports[nPort].proxies[1];

	//getSIPURI(sipURI, proxy1->login_id, proxy1->domain_name);

   	CWMPDBG( 3, ( stderr, "<%s:%d> DEBUG: getting value with InstNum1 %d InstNum2 %d idMib %d \n",
		__FUNCTION__, __LINE__, nInstNum1, nInstNum2, idMib) );
    
	switch( idMib ) {
	case MIB_VOICE_PROFILE__LINE__ENABLE:
		if(proxy1->enable & PROXY_ENABLED){
			*( ( enable_t * )pData ) = TR104_ENABLE;
		}else{
			*( ( enable_t * )pData ) = TR104_DISABLE;
		}
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_ENABLE:
		if(proxy2->enable & PROXY_ENABLED){
			*( ( enable_t * )pData ) = TR104_ENABLE;
		}else{
			*( ( enable_t * )pData ) = TR104_DISABLE;
		}
		break;

	case MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER:
		strcpy( pData, proxy1->number);
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_DIRECTORY_NUMBER:
		strcpy( pData, proxy2->number);
		break;		

	case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLERIDNAME:
		strcpy( pData, proxy1->display_name);
		break;
		
	case MIB_VOICE_PROFILE__LINE__XCT_CALLING_FEATURES_CALLERIDNAME:
		strcpy( pData, proxy2->display_name);
		break;		

	case MIB_VOICE_PROFILE__LINE__CALL_STATE:
		switch(	pEvtMsg.voiceProfileLineStatusMsg[nPort].Voip_port_session_type){
			case 0:
				strcpy((char *)pData, "Idle");
				break;
			case 1:
				strcpy((char *)pData, "Calling");
				break;

			case 2:
				strcpy((char *)pData, "Ringing");
				break;

			case 3:
				strcpy((char *)pData, "Connecting");
				break;

			case 4:
				strcpy((char *)pData, "InCall");
				break;
			case 5:
				strcpy((char *)pData, "Hold");
				break;	

			case 6:
				strcpy((char *)pData, "Disconnecting");
				break;
		}		

		break;
	case MIB_VOICE_PROFILE__LINE__STATUS:
	{
		//TBD:run time update 
//		*( ( line_status_t * )pData ) = UP;
		//*( ( line_status_t * )pData ) = pEvtMsg.voiceProfileLineStatusMsg[nPort].LineStatus;
		FILE *fh;
		char buf[MAX_VOIP_PORTS * MAX_PROXY];
		int regfail=0;


		if(pEvtMsg.e8c_autotest[nPort].enable==1){
			*( ( line_status_t * )pData )=TESTING;
			printf("in auto test mode\n");
			break;			
		}

		if ((proxy1->enable & PROXY_ENABLED) == 0) {
			*( ( line_status_t * )pData )=LINE_STATUS_DISABLED;
			printf("proxy 0 disable\n");
			break;
		}

		fh = fopen(_PATH_TMP_STATUS, "r");
		if (!fh) {
			printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
			printf("\nerrno=%d\n", errno);
			*( ( line_status_t * )pData )=ERROR;
			break;
		}

		memset(buf, 0, sizeof(buf));
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("tr104: _PATH_TMP_STATUS The content of /tmp/status is NULL!!\n");
			printf("\nerrno=%d\n", errno);
			*( ( line_status_t * )pData )=ERROR;
			break;
		}
		else {
			fprintf(stderr, "buf is %s.\n", buf);
			switch (buf[nPort * MAX_PROXY ]) {//proxy 1 status
				case '0'://Not Registered
					*( ( line_status_t * )pData )=INITIALIZING;
					break;
				case '1'://Registered
					*( ( line_status_t * )pData )=UP;
					break;
				case '2'://Registering
					*( ( line_status_t * )pData )=REGISTERING;
					break;
#if 0					
				case '5'://Register Fail
					*( ( line_status_t * )pData )=REGISTERING;
					break;
				case '3'://Register Fail
					*( ( line_status_t * )pData )=ERROR;
					break;
#endif
				case '3':
				case '5':
					//check register fail status
					regfail=atoi( pEvtMsg.voiceProfileLineStatusMsg[nPort].last_register_error);
					if(regfail==FAIL_REASON_ACCOUNT_ERROR){
						*( ( line_status_t * )pData )=AuthFailed;
					}else if((regfail==FAIL_REASON_DEST_UNREACHABLE)||(regfail==FAIL_REASON_DEST_NORESPONSE)){
						*( ( line_status_t * )pData )=SendFailed;
					}else
						*( ( line_status_t * )pData )=ERROR;
					
					break;
				case '4'://VoIP Restart...
					*( ( line_status_t * )pData )=INITIALIZING;
					break;
				default://error
					*( ( line_status_t * )pData )=ERROR;
					break;
			}
		}

		fclose(fh);
		break;
	}

	case MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME:
		strcpy( (char *)pData, proxy1->login_id);
		break;

	case MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_USER_NAME:
		strcpy( (char *)pData, proxy2->login_id);
		break;
		

	case MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD:
		//strcpy( (char *)pData, proxy1->password);
#if defined (CONFIG_E8B)		
       strcpy( (char *)pData, "********");//password show empty
#else
	strcpy( (char *)pData, "");//password show empty
#endif
		break;

	case MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_PASSWORD:
		//strcpy( (char *)pData, proxy2->password);
#if defined (CONFIG_E8B)		
       strcpy( (char *)pData, "********");//password show empty
#else
	strcpy( (char *)pData, "");//password show empty
#endif
		break;		

	case MIB_VOICE_PROFILE__LINE__SIP__URI:

		//memset(sipURI, 0, sizeof(sipURI));
		//getSIPURI(sipURI, proxy1->number, proxy1->domain_name);
		strcpy( (char *)pData, proxy1->number);
		
		break;

	case MIB_VOICE_PROFILE__LINE__SIP__XCT_URI:
		//memset(sipURI, 0, sizeof(sipURI));
		//getSIPURI(sipURI, proxy2->number, proxy2->domain_name);
		strcpy( (char *)pData, proxy2->number);
		break;

	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_SENT:
	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__BYTES_SENT:
	case MIB_VOICE_PROFILE__LINE__STATS__BYTES_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_LOST:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_ANSWERED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_CONNECTED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_FAILED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_ATTEMPTED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_ANSWERED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_CONNECTED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_FAILED:
    case MIB_VOICE_PROFILE__LINE__STATS__TOTAL_CALL_TIME:
    case MIB_VOICE_PROFILE__LINE__STATS__RESET_STATISTICS:
        //Those get from linphone
        *( ( unsigned int * )pData )=0;
		break;

	case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__TRANSMIT_GAIN:
		 *( ( unsigned int * )pData )=pVoipConfig ->ports[nPort].spk_voice_gain;
		break;

		
    case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__RECEIVE_GAIN:
		*( ( unsigned int * )pData )=pVoipConfig ->ports[nPort].mic_voice_gain;
		   break;


		
    case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_ENABLE:
	case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_INUSE:

		*( ( unsigned int * )pData )=pVoipConfig ->ports[nPort].lec;
		break;

		
	case MIB_VOICE_PROFILE__LINE__XCT_IMS__HOTLINEURI:/* run time update , get from call mgnt */
			strcpy((char *)pData,pVoipConfig ->ports[nPort].hotline_number);
			break;

    case MIB_VOICE_PROFILE__LINE__XCT_IMS__DIALTONEPATTERN: /* run time update , get from call mgnt */
			if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_SPECIAL_DIAL_TONE)
				*( ( unsigned int * )pData )=1; /* must play special dial tone */
			else
				*( ( unsigned int * )pData )=0;
			break;


		
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__MCIDSERVICE:/* run time update , get from call mgnt */
		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_MCID_ACTIVATE)
				*( ( unsigned int * )pData )=0; /* provisioned , enable */
			else
				*( ( unsigned int * )pData )=1; /*  withdrawn , disable */
			break;


	
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__NODIALINGBEHAVIOUR:/* run time update , get from call mgnt */

		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_IMMEDIATE_CALLSETUP){
			*( ( unsigned int * )pData )=1; /* hotline */
		}else if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_DEFERE_CALLSETUP){
			*( ( unsigned int * )pData )=2; /* delay - hotline */
		}else{
			*( ( unsigned int * )pData )=0; /*rejectcall*/
		}
	
		break;
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__HOLDSERVICE:/* run time update , get from call mgnt */

		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_HOLD_SERVICE_ACTIVATE)
				*( ( unsigned int * )pData )=0; /* provisioned , enable */
			else
				*( ( unsigned int * )pData )=1; /*  withdrawn , disable */
			break;
		
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__THREEPARTYSERVICE:/* run time update , get from call mgnt */

		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_CONFERENCE_ACTIVATE)
				*( ( unsigned int * )pData )=0; /* provisioned , enable */
			else
				*( ( unsigned int * )pData )=1; /*  withdrawn , disable */
			break;

		
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__CONFERENCEURI:/* run time update , get from call mgnt */
		strcpy((char *)pData, pVoipConfig ->conference_uri);
		break;
		
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__CWSERVICE:/* run time update , get from call mgnt */
		if(pEvtMsg.voiceProfileLineStatusMsg[nPort].featureFlags & IMS_CALL_WAITING_ACTIVATE)
				*( ( unsigned int * )pData )=0; /* provisioned , enable */
			else
				*( ( unsigned int * )pData )=1; /*  withdrawn , disable */
			break;

#ifdef CONFIG_E8B
	case MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON:
		strcpy((char *)pData, pEvtMsg.voiceProfileLineStatusMsg[nPort].last_register_error);
		break;
#endif

    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__STATTIME:	

		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].StatTime;
		CWMPDBG( 0, ( stderr, "<%s:%d> now.tv_sec %lu \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].StatTime));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__TXPACKETS:
		*( ( unsigned int * )pData ) =pEvtMsg.PoorQuality[nInstNum1][nInstNum2].TxPackets;
		CWMPDBG( 0, ( stderr, "<%s:%d> nTxPkts %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].TxPackets));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__RXPACKETS:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].RxPackets;
		CWMPDBG( 0, ( stderr, "<%s:%d> nRxPkts %u \n", __FUNCTION__, __LINE__ ,pEvtMsg.PoorQuality[nInstNum1][nInstNum2].RxPackets));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANDELAY:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MeanDelay;
		CWMPDBG( 0, ( stderr, "<%s:%d> meandelay %hu \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MeanDelay));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANJITTER:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MeanJitter;
		CWMPDBG( 0, ( stderr, "<%s:%d> meanjitter %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MeanJitter));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FRACTIONLOSS:
		*( ( unsigned int * )pData ) =pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FractionLoss;
		CWMPDBG( 0, ( stderr, "<%s:%d> fractionloss %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FractionLoss));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALIPADDRESS:

		strcpy( (char *)pData , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].LocalIPAddress);
		CWMPDBG( 0, ( stderr, "<%s:%d> local_ip_address %s \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].LocalIPAddress));
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALUDPPORT:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].LocalUDPPort;
		CWMPDBG( 0, ( stderr, "<%s:%d> local_udp_port %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].LocalUDPPort));
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDIPADDRESS:
		strcpy( (char *)pData , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FarEndIPAddress);
		CWMPDBG( 0, ( stderr, "<%s:%d> farend_ip_address %s \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FarEndIPAddress));
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDUDPPORT:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FarEndUDPPort;
		CWMPDBG( 0, ( stderr, "<%s:%d> farend_udp_port %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].FarEndUDPPort));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MOSLQ:
		*( ( unsigned int * )pData ) = pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MosLq;
		CWMPDBG( 0, ( stderr, "<%s:%d> moslq %u \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].MosLq));	
				break;
		    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__CODEC:
		strcpy( (char *)pData, pEvtMsg.PoorQuality[nInstNum1][nInstNum2].Codec );
		CWMPDBG( 0, ( stderr, "<%s:%d> codec_string %s \n", __FUNCTION__, __LINE__ , pEvtMsg.PoorQuality[nInstNum1][nInstNum2].Codec));	
				break;

	case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_TAIL:
		*( ( unsigned int * )pData )=pVoipConfig ->ports[nPort].echoTail;
		break;	

	case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLWAITING_ENABLE:
		*( ( boolean * )pData ) = pVoipConfig->ports[nPort].call_waiting_enable;
		break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_ENABLE:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].uc_forward_enable;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_NUMBER:
        strcpy( (char *)pData, pVoipConfig->ports[nPort].uc_forward );
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_ENABLE:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].busy_forward_enable;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_NUMBER:
        strcpy( (char *)pData, pVoipConfig->ports[nPort].busy_forward );
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_ENABLE:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].na_forward_enable;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_NUMBER:
        strcpy( (char *)pData, pVoipConfig->ports[nPort].na_forward );
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_RINGCOUNT:  //need to transfer
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].na_forward_time;     
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLTRANSFER_ENABLE:
        if (pVoipConfig->ports[nPort].fxsport_flag & FXS_CALL_TRANSFER_disable) 
            *( ( boolean * )pData ) = FALSE;
        else
            *( ( boolean * )pData ) = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_MWI_ENABLE:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].mwi_enable;    
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_ENABLE:
        *( ( boolean * )pData ) = pVoipConfig->ports[nPort].hotline_enable;
		break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_NUMBER:
        strcpy( (char *)pData, pVoipConfig->ports[nPort].hotline_number );
		break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_TIMER:
        *( ( unsigned int * )pData ) = pVoipConfig->ports[nPort].hotline_timer;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_CONFERENCE_URI:
        strcpy( (char *)pData, pVoipConfig->conference_uri );
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_SBUS_UA:
		if(proxy1->enable & PROXY_SUBSCRIBE)
			*( ( boolean * )pData ) = TRUE;
		else
			*( ( boolean * )pData ) = FALSE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_SUBSREG:
		if (pVoipConfig->rfc_flags&SIP_SUBSREG)
			*( ( boolean * )pData ) = TRUE;
		else
			*( ( boolean * )pData ) = FALSE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX_ENABLE:
		if(pVoipConfig->ports[nPort].func_flags & SIP_PBXPREFIX_ENABLE)
			*( ( boolean * )pData ) = TRUE;
		else
			*( ( boolean * )pData ) = FALSE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIXSECONDARY_ENABLE:
		if(pVoipConfig->ports[nPort].func_flags & SIP_PBXDIAL_SECONDARY_ENABLE)
			*( ( boolean * )pData ) = TRUE;
		else
			*( ( boolean * )pData ) = FALSE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX:
		//strcpy( (char *)pData, "");//eric temp empty //PSTN_routing_prefix
		strcpy( ( char * )pData, (const char *)pVoipConfig ->ports[nPort].PSTN_routing_prefix);
		break;
	default:
		return 0;	/* range error */
		break;
	}

	return 1;
}

int mib_set_type2( idMib_t idMib, unsigned int nInstNum1, 
				   unsigned int nInstNum2,
				   void *pData )
{
	/* VoiceService.1.VoiceProfile.{ i }.Line.{i}. */
	//voipCfgParam_t voipConfig;
	//voipCfgParam_t * const pVoipConfig = &g_pVoIPShare ->voip_cfg;
	voipCfgProxy_t *proxy1;
	voipCfgProxy_t *proxy2;
	int nPort=0;

	bDirtyBit=FALSE;
	if( pVoipConfig == NULL && getVoipFlash()!=0 ) {	//Alex, Modified, 110913
		return 0;
	}       
    
	if( IS_VOICE_PROFILE(idMib,1) && (nInstNum1 >= MAX_PROFILE_COUNT || nInstNum2 >= MAX_LINE_PER_PROFILE ) ){
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong Voice Profile or Line InstNum \n", __FUNCTION__, __LINE__));
		return 0;
    } else if( IS_PHYINTERFACE(idMib,1) && (nInstNum1 >= MAX_PHYINTERFACE_COUNT || nInstNum2 >= MAX_POORQUALITYLIST_COUNT)) {
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong PhyInterface or PoorQualityList InstNum \n", __FUNCTION__, __LINE__));
		return 0;
	}
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif
 	proxy1 = &pVoipConfig ->ports[nPort].proxies[0];
	proxy2 = &pVoipConfig ->ports[nPort].proxies[1];


    CWMPDBG( 3, ( stderr, "<%s:%d> DEBUG: setting value with InstNum1 %d InstNum2 %d idMib %d \n",
            __FUNCTION__, __LINE__, nInstNum1, nInstNum2, idMib) );

	switch( idMib ) {
	case MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER:
		strcpy( proxy1->number, pData );
		bDirtyBit=TRUE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_DIRECTORY_NUMBER:
		strcpy( proxy2->number, pData );
		bDirtyBit=TRUE;
		break;
	case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLERIDNAME:
		strcpy( proxy1->display_name, pData );
		bDirtyBit=TRUE;
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_CALLING_FEATURES_CALLERIDNAME:
		strcpy( proxy2->display_name, pData );
		bDirtyBit=TRUE;
		break;

	case MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME:
		strcpy( proxy1->login_id, pData );
#ifdef CONFIG_E8B
		strcpy( proxy2->login_id, pData );
#endif
		bDirtyBit=TRUE;
		break;
		
	case MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_USER_NAME:
		strcpy( proxy2->login_id, pData );

		bDirtyBit=TRUE;
		break;

	case MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD:
		strcpy( proxy1->password, pData );
#ifdef CONFIG_E8B		
		strcpy( proxy2->password, pData );
#endif		
		bDirtyBit=TRUE;
		break;
	case MIB_VOICE_PROFILE__LINE__SIP__URI:
		
		strcpy( proxy1->number, pData );
		strcpy( proxy2->number, pData );
		
		bDirtyBit=TRUE;
		
		break;
	case MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_PASSWORD:
		strcpy( proxy2->password, pData );
		bDirtyBit=TRUE;
		break;
			

	case MIB_VOICE_PROFILE__LINE__ENABLE:
		if(*( ( enable_t * )pData ) == TR104_DISABLE){
			proxy1->enable &= ~PROXY_ENABLED;
			proxy2->enable &= ~PROXY_ENABLED;
		}else{
			proxy1->enable |= PROXY_ENABLED;
			proxy2->enable |= PROXY_ENABLED;
		}
        	bDirtyBit=TRUE;
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_ENABLE:
		if(*( ( enable_t * )pData ) == TR104_DISABLE){
			proxy2->enable &= ~PROXY_ENABLED;
		}else{
			proxy2->enable |= PROXY_ENABLED;
		}
		bDirtyBit=TRUE;
		break;
			

    case MIB_VOICE_PROFILE__LINE__STATS__RESET_STATISTICS:
		pEvtMsg.voiceProfileLineStatusMsg[nPort].resetStatistics = *( ( unsigned int * )pData );
		break;
	case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__TRANSMIT_GAIN:
		pVoipConfig ->ports[nPort].spk_voice_gain =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
		
    case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__RECEIVE_GAIN:
		pVoipConfig ->ports[nPort].mic_voice_gain =  *( ( unsigned int * )pData );
		bDirtyBit = TRUE;
		break;
		
    case MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_ENABLE:
		pVoipConfig ->ports[nPort].lec= *( ( unsigned int * )pData );
        bDirtyBit = TRUE;
		break;
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__HOTLINEURI:
		strcpy(pVoipConfig ->ports[nPort].hotline_number, ( char * )pData);
        bDirtyBit = TRUE;
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX_ENABLE:
		if(*( ( enable_t * )pData ) == TR104_DISABLE){
			pVoipConfig ->ports[nPort].func_flags &= ~SIP_PBXPREFIX_ENABLE;
		}else{
			pVoipConfig ->ports[nPort].func_flags |= SIP_PBXPREFIX_ENABLE;
		}
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIXSECONDARY_ENABLE:
		if(*( ( enable_t * )pData ) == TR104_DISABLE){
			pVoipConfig ->ports[nPort].func_flags &= ~SIP_PBXDIAL_SECONDARY_ENABLE;		
		}else{
			pVoipConfig ->ports[nPort].func_flags |= SIP_PBXDIAL_SECONDARY_ENABLE;	
		}
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX:
		if(strlen((char *)pData)<PSTN_ROUTING_PREFIX_LEN){
			strcpy( (char *)pVoipConfig ->ports[nPort].PSTN_routing_prefix, ( char * )pData );
			bDirtyBit = TRUE;
		}
		break;
	case MIB_VOICE_PROFILE__LINE__STATUS:		
	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_SENT:
	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__BYTES_SENT:
	case MIB_VOICE_PROFILE__LINE__STATS__BYTES_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__PACKETS_LOST:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_RECEIVED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_ANSWERED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_CONNECTED:
	case MIB_VOICE_PROFILE__LINE__STATS__INCOMING_CALLS_FAILED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_ATTEMPTED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_ANSWERED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_CONNECTED:
	case MIB_VOICE_PROFILE__LINE__STATS__OUTGOING_CALLS_FAILED:
    case MIB_VOICE_PROFILE__LINE__STATS__TOTAL_CALL_TIME:    
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__STATTIME:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__TXPACKETS:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__RXPACKETS:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANDELAY:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANJITTER:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FRACTIONLOSS:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALIPADDRESS:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALUDPPORT:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDIPADDRESS:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDUDPPORT:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MOSLQ:
        CWMPDBG( 0, ( stderr, "<%s:%d>\n", __FUNCTION__, __LINE__));
		break;
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__DIALTONEPATTERN:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__MCIDSERVICE:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__NODIALINGBEHAVIOUR:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__HOLDSERVICE:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__THREEPARTYSERVICE:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__CONFERENCEURI:
    case MIB_VOICE_PROFILE__LINE__XCT_IMS__CWSERVICE:
    case MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__CODEC:        
        // unable to set	//Read Only, we might needn't to write
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;

#ifdef CONFIG_E8B
	case MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON:
		CWMPDBG( 0, ( stderr, "<%s:%d> %d set %s\n" , __FUNCTION__ , __LINE__ , idMib , ( char * )pData));

		switch(nPort){  //TBD
			case 0:
				CWMPDBG( 0, ( stderr, "<%s:%d> %d MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON\n", __FUNCTION__, __LINE__ , nPort));	
				break;
			case 1:
				CWMPDBG( 0, ( stderr, "<%s:%d> %d MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON\n", __FUNCTION__, __LINE__ , nPort));	
				break;
			default:
				break;
			}
		break;
#endif
		
    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLWAITING_ENABLE:
		pVoipConfig ->ports[nPort].call_waiting_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_ENABLE:
        pVoipConfig->ports[nPort].uc_forward_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_NUMBER:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            strcpy( pVoipConfig->ports[nPort].uc_forward, (char *)pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_ENABLE:
        pVoipConfig->ports[nPort].busy_forward_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
		break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_NUMBER:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            strcpy( pVoipConfig->ports[nPort].busy_forward, (char *)pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_ENABLE:
        pVoipConfig->ports[nPort].na_forward_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_NUMBER:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            strcpy( pVoipConfig->ports[nPort].na_forward, (char *)pData );
        bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_RINGCOUNT:  //need to transfer
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
			pVoipConfig->ports[nPort].na_forward_time = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}    
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLTRANSFER_ENABLE:
        if (*( ( boolean * )pData ) == FALSE)
            pVoipConfig->ports[nPort].fxsport_flag |= FXS_CALL_TRANSFER_disable;
        else
            pVoipConfig->ports[nPort].fxsport_flag &= ~FXS_CALL_TRANSFER_disable;
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_MWI_ENABLE:
        pVoipConfig->ports[nPort].mwi_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_ENABLE:
        pVoipConfig->ports[nPort].hotline_enable = *( ( boolean * )pData );
        bDirtyBit = TRUE;
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_NUMBER:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            strcpy( pVoipConfig->ports[nPort].hotline_number, (char *)pData );
            bDirtyBit = TRUE;
        }
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_TIMER:   
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
			pVoipConfig->ports[nPort].hotline_timer = *( ( unsigned int * )pData );
			bDirtyBit = TRUE;
		}
        break;

    case MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_CONFERENCE_URI:
        if (VERIFY_OK == verifySettingValues(idMib, nPort, pVoipConfig, pData)) {
            strcpy( pVoipConfig->conference_uri, (char *)pData );
            bDirtyBit = TRUE;
        }
		break;

	case MIB_VOICE_PROFILE__LINE__XCT_SBUS_UA:
		if(*( ( int * )pData ) == 1){ //enable
			if (proxy1->enable & PROXY_SUBSCRIBE){
				if (pVoipConfig->subs_type==SUBS_TYPE_REG||pVoipConfig->subs_type==SUBS_TYPE_ALL){
					pVoipConfig->subs_type=SUBS_TYPE_ALL;
				}
				else{
					pVoipConfig->subs_type=SUBS_TYPE_EVENT;
				}
			}else{
				proxy1->enable |= PROXY_SUBSCRIBE;
				proxy2->enable |= PROXY_SUBSCRIBE;
				pVoipConfig->subs_type=SUBS_TYPE_EVENT;
			}
		}else if(*( ( int * )pData ) == 0){ //disable
			if (proxy1->enable & PROXY_SUBSCRIBE){
				if (pVoipConfig->subs_type==SUBS_TYPE_ALL){
					pVoipConfig->subs_type=SUBS_TYPE_REG;
				}else if (pVoipConfig->subs_type==SUBS_TYPE_EVENT){
					proxy1->enable &= ~ PROXY_SUBSCRIBE;
					proxy2->enable &= ~ PROXY_SUBSCRIBE;
				}
			}
		}
		bDirtyBit = TRUE;
		break;
	case MIB_VOICE_PROFILE__LINE__XCT_SUBSREG:
		if(*( ( int * )pData ) == 1){ //enable
			if (proxy1->enable & PROXY_SUBSCRIBE){
				if (pVoipConfig->subs_type==SUBS_TYPE_EVENT||pVoipConfig->subs_type==SUBS_TYPE_ALL){
					pVoipConfig->subs_type=SUBS_TYPE_ALL;
				}
				else{
					pVoipConfig->subs_type=SUBS_TYPE_REG;
				}
			}else{
				proxy1->enable |= PROXY_SUBSCRIBE;
				proxy2->enable |= PROXY_SUBSCRIBE;
				pVoipConfig->subs_type=SUBS_TYPE_REG;
			}
		}else if(*( ( int * )pData ) == 0){ //disable
			if (proxy1->enable & PROXY_SUBSCRIBE){
				if (pVoipConfig->subs_type==SUBS_TYPE_ALL){
					pVoipConfig->subs_type=SUBS_TYPE_EVENT;
				}else if (pVoipConfig->subs_type==SUBS_TYPE_REG){
					proxy1->enable &= ~ PROXY_SUBSCRIBE;
					proxy2->enable &= ~ PROXY_SUBSCRIBE;
				}
			}
		}
		bDirtyBit = TRUE;
		break;
	default:
		return 0;	/* range error */
		break;
	}

	if(TRUE == bDirtyBit){
		//voip_flash_set(pVoipConfig);//move to cwmp-client

        apply_add( CWMP_PRI_N, apply_voip, CWMP_RESTART, 0, NULL, 0 );
    }
	return 0;

}

int mib_get_type3( idMib_t idMib, unsigned int nInstNum1, 
				   unsigned int nInstNum2, unsigned int nInstNum3,
				   void *pData )
{
	/* VoiceService.1.VoiceProfile.{ i }.Line.{i}.Codec.List.{i}. */
	//voipCfgParam_t voipConfig;
	//voipCfgParam_t * const pVoipConfig = &g_pVoIPShare ->voip_cfg;
	int nPort=0;

    if( pVoipConfig == NULL && getVoipFlash()!=0 ) {    //Alex, Modified, 110913
        return 0;
    }

	if( ( nInstNum1 >= MAX_PROFILE_COUNT ) ||
		( nInstNum2 >= MAX_LINE_PER_PROFILE ) ||
		( nInstNum3 >= MAX_CODEC_LIST ) )
	{
   		CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Wrong InstNum \n", __FUNCTION__, __LINE__));
		return 0;
	}
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif

    CWMPDBG( 3, ( stderr, "<%s:%d> DEBUG: getting value with InstNum1 %d InstNum2 %d InstNum3 %d idMib %d \n",
                __FUNCTION__, __LINE__, nInstNum1, nInstNum2, nInstNum3, idMib) );

	switch( idMib ) {
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__ENTRY_ID:
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__CODEC:
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__BITRATE:
		break;
		
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PACKETIZATION_PERIOD:
//		strcpy( pData, lstCodecs[nInstNum3].pszPacketizationPeriod );
	{
		int codecIndex=0;
        char packetizationPeriod[64];
        codecIndex = getCodecIndex(lstCodecs[nInstNum3].pszCodec, lstCodecs[nInstNum3].nBitRate);
        if(-1 != codecIndex){
             /* transform to 10 base*/
             //printf(" Debug: Codec Index: %d\n", codecIndex);
             sprintf(packetizationPeriod, "%d", 10 * (pVoipConfig ->ports[nPort].frame_size[codecIndex] + 1));
             //printf(" Debug: Packetization Period: %s\n", packetizationPeriod);
             strcpy( (char *) pData, packetizationPeriod );
        }
	}
		break;
		
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__SILENCE_SUPPRESSION:
		(*( boolean*)pData) = pVoipConfig ->ports[nPort].vad;
		break;
		
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PRIORITY:
	{
		int codecIndex=0;
		codecIndex = getCodecIndex(lstCodecs[nInstNum3].pszCodec, lstCodecs[nInstNum3].nBitRate);
		if(-1 != codecIndex)
            /* transform to 1 base*/
			*( ( unsigned int * )pData ) = 1 + pVoipConfig ->ports[nPort].precedence[codecIndex];
	}
		break;
		
	default:
		return 0;	/* range error */
		break;
	}
	return 1;
}

int mib_set_type3( idMib_t idMib, unsigned int nInstNum1, 
				   unsigned int nInstNum2, unsigned int nInstNum3,
				   void *pData )
{
	/* VoiceService.1.VoiceProfile.{ i }.Line.{i}.Codec.List.{i}. */
	//voipCfgParam_t voipConfig;
	//voipCfgParam_t * const pVoipConfig = &g_pVoIPShare ->voip_cfg;
	int nPort=0;
	bDirtyBit=FALSE;

	if( pVoipConfig == NULL && getVoipFlash()!=0 ) {	//Alex, Modified, 110913
		return 0;
	}    
    
	nPort = nInstNum1;
//#ifdef CONFIG_RTK_VOIP_PACKAGE_8186
//	mib_get(MIB_VOIP_CFG, (void*)&voipConfig, sizeof(voipConfig));
//#else /*CONFIG_RTK_VOIP_PACKAGE_8186*/
//	mib_get(CWMP_VOIP, (void*)&voipConfig);
//#endif
    CWMPDBG( 3, ( stderr, "<%s:%d> DEBUG: getting value with InstNum1 %d InstNum2 %d InstNum3 %d idMib %d \n",
                __FUNCTION__, __LINE__, nInstNum1, nInstNum2, nInstNum3, idMib) );

	switch( idMib ) {
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__ENTRY_ID:
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__CODEC:
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__BITRATE:
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is read-only or not supported to configure now. \n", __FUNCTION__, __LINE__));
		break;
		
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PACKETIZATION_PERIOD:

    {
		int codecIndex=0;
        codecIndex = getCodecIndex(lstCodecs[nInstNum3].pszCodec, lstCodecs[nInstNum3].nBitRate);
   		if(-1 != codecIndex){
            int frame_size = 0;
            /* "10, 20, 30" ¡A"5-40" ¡A"5-10, 20, 30" not supported now */
			frame_size = atoi((char *)pData);

            /* The unit is 10ms now */
            if(frame_size > 0 && frame_size <= 60 && (frame_size % 10)==0 ){
                //printf("Debug: Setting %d to codec index %d\n", frame_size, codecIndex);
	            pVoipConfig ->ports[nPort].frame_size[codecIndex] = ((frame_size/10) - 1);
				bDirtyBit=TRUE;
			}            
        }        
    }
        break;

	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__SILENCE_SUPPRESSION: 
		//pVoipConfig ->ports[nPort].vad = (*( boolean*)pData);
        //bDirtyBit = TRUE;
        CWMPDBG( 0, ( stderr, "<%s:%d>Warning: Parameter is not supported to configure now. \n", __FUNCTION__, __LINE__));
	    break;
        
	case MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PRIORITY:
	{
		int codecIndex=0, swapIndex=0, i;
	
		codecIndex = getCodecIndex(lstCodecs[nInstNum3].pszCodec, lstCodecs[nInstNum3].nBitRate);
		if(-1 != codecIndex){
			for(i=_CODEC_G711U; i <  _CODEC_MAX; i++){
				if(pVoipConfig ->ports[nPort].precedence[i]==(*( unsigned int * )pData)-1){             /* transform to 0 base*/
					swapIndex = i;
					break;
				}
			}
			pVoipConfig ->ports[nPort].precedence[swapIndex] = pVoipConfig ->ports[nPort].precedence[codecIndex];
            /* transform to 0 base*/
			pVoipConfig ->ports[nPort].precedence[codecIndex]=(* ( unsigned int * )pData )-1;
			bDirtyBit=TRUE;
		}
	}
		break;
		
	default:
		return 0;	/* range error */
		break;
	}

	if(TRUE == bDirtyBit){
		//voip_flash_set(pVoipConfig);//move to cwmp-client
	
        apply_add( CWMP_PRI_N, apply_voip, CWMP_RESTART, 0, NULL, 0 );
    }
	return 0;
}


