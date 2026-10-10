#include <stdio.h>
#include <stdint.h>
#include "web_voip.h"
#include "mib.h"
#include "rcm_customize.h"
#ifdef SUPPORT_CODEC_DESCRIPTOR
#include "codec_table.h"
#endif

#define PWD_DISPLAY "**********"
char *e8c_supported_codec_string[SUPPORTED_CODEC_MAX] = {
	"G711-ulaw",
	"G711-alaw",
#ifdef CONFIG_RTK_VOIP_G729AB
	"G729",
#endif
#ifdef CONFIG_RTK_VOIP_G7231
	"G723",
#endif
#ifdef CONFIG_RTK_VOIP_G726
	"G726-16k",
	"G726-24k",
	"G726-32k",
	"G726-40k",
#endif
#ifdef CONFIG_RTK_VOIP_GSMFR
	"GSM-FR",
#endif
#ifdef CONFIG_RTK_VOIP_ILBC
	"iLBC",
#endif
#ifdef CONFIG_RTK_VOIP_G722
	"G722",
#endif
#ifdef CONFIG_RTK_VOIP_SPEEX_NB
	"SPEEX-NB",
#endif
#ifdef CONFIG_RTK_VOIP_G7111
	"G711U-WB",
	"G711A-WB",
#endif
	};
extern int restart_dnsrelay(void);
//extern int get_register(int port);

extern int mib_get(int id, void *value);
extern int cmd_set_dns_config(const char *ifname);
//extern char heartbeat_mode[H248_HEARTBEAT_MAX][16];
extern int mib_set(int id, void *value);

#ifdef SUPPORT_CODEC_DESCRIPTOR
CT_ASSERT( ( sizeof( e8c_supported_codec_string ) / sizeof( e8c_supported_codec_string[ 0 ] ) ) == SUPPORTED_CODEC_MAX );
CT_ASSERT( SUPPORTED_CODEC_MAX == NUM_CODEC_TABLE );
#endif
void asp_cmcc_codec_get(request * wp, voipCfgPortParam_t *pCfg);
void asp_cmcc_maxDelay(request *wp, int nMaxDelay);

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#define E8C_CODEC_NUM 5
//static 	const char *e8c_codec_str[E8C_CODEC_NUM] = {"G711-ulaw", "G711-alaw", "G722", "G729", "G723"};
static 	const char *e8c_codec_str[E8C_CODEC_NUM] = {"G722",  "G711-alaw", "G711-ulaw", "G729", "G723"};
#else
#define E8C_CODEC_NUM 4
static 	const char *e8c_codec_str[E8C_CODEC_NUM] = {"G711-ulaw", "G711-alaw", "G722", "G729"};
#endif
static 	const char *cmcc_style_codec_str[5] = {"G.722",  "G.711 a-law", "G.711 u-law", "G.729", "G.723"};
static 	int e8c_codec_idx[E8C_CODEC_NUM] = {0};
static	int rank[E8C_CODEC_NUM] = {0};
static  int sip_display_mode;

//e8c support
static char e8c_dscp[DSCP_MAX][25] = {"Class 0 (DSCP 0x00)",
			"Class 1 (DSCP 0x08)",
			"Class 2 (DSCP 0x10)",
			"Class 3 (DSCP 0x18)",
			"Class 4 (DSCP 0x20)",
			"Class 5 (DSCP 0x28)",
			"Class 6 (DSCP 0x30)",
			"Class 7 (DSCP 0x38)",
			"EF (DSCP 0x2e)"
			};
#define E8C_DTMF_MAX 3
char e8c_dtmf[E8C_DTMF_MAX][12] = {"RFC2833", "SIP INFO", "Inband"};
#define E8C_CID_MAX 2
char e8c_cid[E8C_CID_MAX][25] = {"FSK", "DTMF"};
char e8c_hookflash_type[2][20] = {"NONE", "SIP_INFO"};

char server_type[SERVER_TYPE_MAX][20]={"IMS SIP", "软交换NGN SIP" 
#ifdef CONFIG_USER_RTK_H248
, "H.248"
#endif
};

/* get number from login id*/
static void getNumberFromLogin_id(char *Number, const char *URI){
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

#if 1//def CONFIG_USER_RTK_H248
#include "mib.h"

char id_type[H248_ID_TYPE_MAX][16] = {"IP", "Domain Name", "Device Name"};
char encoding_type[H248_ENCODING_MAX][8] = {"ABNF", "ASN.1"};
char phy_term_method[H248_PHYTERM_METHOD_MAX][16] = {"通配模式", "线路模式"};
char heartbeat_mode[H248_HEARTBEAT_MAX][64] = {"被动心跳回应", "NOTIFY方式"};


#endif

void asp_e8c_jitterDelay(request * wp, int nJitterDelay)
{
	int i;

	if( nJitterDelay < 2 || nJitterDelay > 40 )
		nJitterDelay = 2;

	for( i = 2; i <= 40; i ++ ) {
		if( i == nJitterDelay )
			boaWrite(wp, "<option value=%d selected>%d</option>", i, i * 10);
		else
			boaWrite(wp, "<option value=%d>%d</option>", i, i * 10);
	}
}

int get_register(int port)
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];
	char p0_reg_st, p1_reg_st;
	int ret=0;	
	fh = fopen(_PATH_TMP_STATUS, "r");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		printf("\nerrno=%d\n", errno);
		return 0;
	}

	memset(buf, 0, sizeof(buf));
	if (fread(buf, sizeof(buf), 1, fh) == 0) {
		printf("Web: The content of /tmp/status is NULL!!\n");
		printf("\nerrno=%d\n", errno);
		ret=0;
	}
	else {

		p0_reg_st = buf[port * MAX_PROXY];
		p1_reg_st = buf[port * MAX_PROXY + 1];
		if((p0_reg_st == '1')||(p1_reg_st == '1'))
		{
			ret=1;
		}
		else{
			ret=0;
		}
	}
	fclose(fh);
	return ret;
}

void asp_e8c_codec_get(request * wp, voipCfgPortParam_t *pCfg)
{
	int i, j, k;
	int e8c_codec_preced[E8C_CODEC_NUM] = {0}; //For trivial sorting
	int enable_codec[E8C_CODEC_NUM]= {0};
	//Find out E8C codec index from the codec set

	for (i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		for(k=0; k<E8C_CODEC_NUM; k++)
		{
			if( (strcmp(e8c_supported_codec_string[i], e8c_codec_str[k]) == 0) )
			{
				e8c_codec_idx[k]=i;
				e8c_codec_preced[k] = pCfg->precedence[i];
				break;
			}
		}
	}

	//Sorting
	for(i=0; i<E8C_CODEC_NUM; i++)
	{
		int smallest = INT32_MAX;
		int s_idx = -1;
		for(j=0; j<E8C_CODEC_NUM; j++)
		{
			if(smallest > e8c_codec_preced[j])
			{
				s_idx = j;
				smallest = e8c_codec_preced[j];
			}
		}
		if(s_idx != -1)
		{
			rank[i] = e8c_codec_idx[s_idx];
			e8c_codec_preced[s_idx] = INT32_MAX;
		}
	}
	
	for (i=0; i<E8C_CODEC_NUM; i++)
	{

		boaWrite(wp, "<tr><td width=150px>%s</td>\n", e8c_codec_str[i]);
		boaWrite(wp, "<td><input type=\"checkbox\"  name=\"codec_%d_enable\" size=20 %s>\n", i, (!(255 == pCfg->precedence[e8c_codec_idx[i]]) ? "checked" : "" ));
		boaWrite(wp, "<td><select name=frame_size%d>\n", i);
		for (j=0; j<3; j++)
			boaWrite(wp,
				"<option %s value=%d>%d %s</option>",
				j == pCfg->frame_size[e8c_codec_idx[i]] ? "selected" : "",
				j,
				10 * (j + 1),
				"ms"
				);
		boaWrite(wp, "</select></td>\n");
		//boaWrite(wp, "<td>%d</td>\n", e8c_codec_idx[i]);
		boaWrite(wp, "<td><select name=preced%d>\n", i);
		for (j=0; j<E8C_CODEC_NUM; j++)
		{
			if(e8c_codec_idx[i] == rank[j])
			{
				boaWrite(wp, "<option value=%d selected>%d", j+1, j+1);
			}
			else
			{
				boaWrite(wp, "<option value=%d>%d", j+1, j+1);
			}
		}
		boaWrite(wp, "</select></td></tr>\n");
	}
}

void asp_e8c_codec_set(request * wp, voipCfgPortParam_t *pCfg)
{
	int i, voip_port;
	char szName[12];


	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{

	for(i=0; i<E8C_CODEC_NUM; i++)
	{
			sprintf(szName, "codec_%d_enable", i);
			if (gstrcmp(boaGetVar(wp, szName, ""), "on"))
			{
				fprintf(stderr, "---Enter %s---codec_%d_enable off \n", __FUNCTION__, i);
				 pCfg->precedence[e8c_codec_idx[i]]=255;
			}else
			{
				sprintf(szName, "preced%d", i);
				pCfg->precedence[e8c_codec_idx[i]]=atoi(boaGetVar(wp, szName, ""))-1;
	}
			sprintf(szName, "frame_size%d", i);
			pCfg->frame_size[e8c_codec_idx[i]]=atoi(boaGetVar(wp, szName, ""));
	

	}
}

}

int asp_voip_e8c_get(int ejid, request * wp, int argc, char **argv)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg1;
	voipCfgPortParam_t *pCfg;
	int i;
	int voip_port;
	int account_number=0;
	char tmp[128];

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;

	pCfg1 = &pVoIPCfg->ports[0];

    if (strcmp(argv[0], "sip_display")==0)
	{
		if ((pVoIPCfg->X_CT_servertype == SOFTSWITCH_SIP)||(pVoIPCfg->X_CT_servertype == IMS_SIP))
        {
			boaWrite(wp, "style=\"display:inline\"");
			sip_display_mode = 1; //SIP		
        } 
        else
        {
			boaWrite(wp, "style=\"display:none\"");
			sip_display_mode = 0; //H.248
        }     	
	}
	else if (strcmp(argv[0], "h248_display")==0)
	{
		if ((pVoIPCfg->X_CT_servertype != SOFTSWITCH_SIP)&&(pVoIPCfg->X_CT_servertype != IMS_SIP))
        {
			boaWrite(wp, "style=\"display:inline\"");
			sip_display_mode = 0;		
        } 
        else 
        {
			boaWrite(wp, "style=\"display:none\"");
			sip_display_mode = 1;
        }
	}
    else if (strcmp(argv[0], "sip_display_mark_start")==0)
	{
		if (sip_display_mode == 0){
			boaWrite(wp, "<!--");		
        }
	}
    else if (strcmp(argv[0], "sip_display_mark_end")==0)
	{
		if (sip_display_mode == 0)
            boaWrite(wp, "-->");		
	}
    else if (strcmp(argv[0], "h248_display_mark_start")==0)
	{
		if (sip_display_mode == 1)
            boaWrite(wp, "<!--");		
	}
    else if (strcmp(argv[0], "h248_display_mark_end")==0)
	{
		if (sip_display_mode == 1)
            boaWrite(wp, "-->");		
	}

	if (strcmp(argv[0], "max_voip_ports")==0)
	{
		boaWrite(wp, "%d", VOIP_PORTS);
	}
	else if (strcmp(argv[0], "sip_number")==0)
	{
		if (2 == pVoIPCfg->X_CT_servertype)
			return 0;

            boaWrite(wp, "<td class=\"hdb\">用户名(电话号码)</td>");

            for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
            {
			if(pVoIPCfg->ports[voip_port].proxies[0].number == NULL)
				return 0;

			if(0 != strlen(pVoIPCfg->ports[voip_port].proxies[0].number))
			{
                    boaWrite(wp, "<td>%s(%s)</td>",pVoIPCfg->ports[voip_port].proxies[0].login_id, pVoIPCfg->ports[voip_port].proxies[0].number);	
            }
			else
			{
				boaWrite(wp, "<td>%s</td>",pVoIPCfg->ports[voip_port].proxies[0].login_id);
			}
        }
	}
	else if (strcmp(argv[0], "cmcc_sip_number")==0)
	{
		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{
			boaWrite(wp, "<tr><td>1</td>");
			if(pVoIPCfg->ports[voip_port].proxies[0].number != NULL)
			{
				if ((pVoIPCfg->rfc_flags & REGISTER_FROM_USE_LOGINID)&&(strlen(pVoIPCfg->ports[voip_port].proxies[0].number)==0)){
					getNumberFromLogin_id(tmp,pVoIPCfg->ports[voip_port].proxies[0].login_id);
					boaWrite(wp, "<td>%s</td></tr>",tmp);
				}else{
				boaWrite(wp, "<td>%s</td></tr>",pVoIPCfg->ports[voip_port].proxies[0].number);
				}
			}
			#if 0
			boaWrite(wp, "<tr><td>2</td>");
			if(pVoIPCfg->ports[voip_port].proxies[1].number != NULL)
				boaWrite(wp, "<td>%s</td></tr>",pVoIPCfg->ports[voip_port].proxies[1].number);
			#endif
		}
	}
	else if (strcmp(argv[0], "E8C_registerStatus")==0)
	{
		FILE *fh;
		char buf[MAX_VOIP_PORTS];
		
		fh = fopen(_PATH_TMP_E8C_STATUS, "r");
		if (!fh) {
			printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_E8C_STATUS);
			printf("\nerrno=%d\n", errno);
			boaWrite(wp, "%s", "");
			return 0;
		}
		memset(buf, 0, sizeof(buf));
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("Web: The content of /tmp/E8C_voipstatus is NULL!!\n");
			printf("\nerrno=%d\n", errno);
			boaWrite(wp, "<td>%s</td>", "注册失败，未知错误");
		}
		else {
			for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
			{
				/* SD6, bohungwu, fix incorrect registration status for the secondary proxy */
				char p_reg_st;
				if((pVoIPCfg->ports[voip_port].proxies[0].enable & PROXY_ENABLED)==0)
				{
					boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", " ");	
				}
				else
				{
					if(get_register(voip_port)==1)
					{
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", " ");
					}else{
					p_reg_st=buf[voip_port];
					//fprintf(stderr,"p_reg_st=%c voip_port=%d\n", p_reg_st, voip_port);
					//fprintf(stderr, "buf is %s\n", buf);
					switch (p_reg_st) {
					case '0'://VOIP_STATUS_INIT
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", " ");
						break;
					case '1'://VOIP_STATUS_SUCCESS
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", " ");
						break;
					case '2'://VOIP_NOVALID_CONNECTION
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "无有效的VOIP连接");
						break;
					case '3'://VOIP_STATUS_IADERROR
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "IAD模块错误");
						break;
					case '4'://VOIP_STATUS_NOROUTE
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "访问路由不通");
						break;
					case '5'://VOIP_STATUS_NORESPONSE
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "访问服务器无响应");
						break;
					case '6'://VOIP_STATUS_ACCOUNTERR
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "帐号错误");
						break;
					case '7'://VOIP_STATUS_UNKNOWN
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "未知错误");
						break;
					case '8'://VOIP_STATUS_LINE_DISABLED
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "线路未使能");
						break;							
					case '9'://VOIP_STATUS_ADDR_UNKNOW_HOST
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "域名无法解析");
						break;						
					default:
						boaWrite(wp, "<td>注册失败原因</td><td>%s</td>", "未知错误");
						break;
					}
				}
			}
		}
		}
		fclose(fh);
	}
	else if (strcmp(argv[0], "registerStatus_2")==0)
	{
		boaWrite(wp, "<td>%s</td>", "");	
	}
	else if (strcmp(argv[0], "registerStatus")==0)
	{
		FILE *fh;
		char buf[MAX_VOIP_PORTS * MAX_PROXY];
		
		fh = fopen(_PATH_TMP_STATUS, "r");
		if (!fh) {
			printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
			printf("\nerrno=%d\n", errno);
			boaWrite(wp, "%s", "ERROR");
			return 0;
		}

		memset(buf, 0, sizeof(buf));
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("Web: The content of /tmp/status is NULL!!\n");
			printf("\nerrno=%d\n", errno);
			boaWrite(wp, "%s", "ERROR");
		}
		else {
			for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
			{
				/* SD6, bohungwu, fix incorrect registration status for the secondary proxy */
				char p0_reg_st, p1_reg_st, p_reg_st;
				if((pVoIPCfg->ports[voip_port].proxies[0].enable & PROXY_ENABLED)==0)
				{
					boaWrite(wp, "<td>%s</td>", "去使能");	
				}
				else
				{
				
                    if (2 == pVoIPCfg->X_CT_servertype) //h.248
                    {
                        p_reg_st = buf[0];
                    }
                    else
                    {
                       p0_reg_st = buf[voip_port * MAX_PROXY];
    					p1_reg_st = buf[voip_port * MAX_PROXY + 1];
    					if((p0_reg_st == '1')||(p0_reg_st == '2')||(p0_reg_st == '4')||(p0_reg_st == '5')){
    						p_reg_st = p0_reg_st;
    					}
    					else{
    						if((pVoIPCfg->ports[voip_port].proxies[1].enable & PROXY_ENABLED)&&(p1_reg_st != '0'))
    						{
    							p_reg_st = p1_reg_st;
    						}
    						else//sencond proxy disable, use main proxy status
    						{
    							p_reg_st = p0_reg_st;
    						}
    					}

                    }
					
					//fprintf(stderr, "buf is %s.\n", buf);
					switch (p_reg_st) {
					case '0':
						//boaWrite(wp, "<td>%s</td>", "Not Registered");
						boaWrite(wp, "<td>%s</td>", "端口未注册");
						break;
					case '1':
						//boaWrite(wp, "<td>%s</td>", "Registered");
						boaWrite(wp, "<td>%s</td>", "端口注册成功");
						break;
					case '2':
					case '5':
						//boaWrite(wp, "<td>%s</td>", "Registering");
						boaWrite(wp, "<td>%s</td>", "端口注册中");
						break;
					case '3':
						//boaWrite(wp, "<td>%s</td>", "Register Fail");
						boaWrite(wp, "<td>%s</td>", "端口注册失败");
						break;
					case '4':
						//boaWrite(wp, "<td>%s</td>", "VoIP Restart...");
						boaWrite(wp, "<td>%s</td>", "VoIP Restart...");
						break;
					default:
						boaWrite(wp, "<td>%s</td>", "ERROR");
						break;
					}
				}
                if (2 == pVoIPCfg->X_CT_servertype)
                        break; //break for loop because H.248 only display root status
			}
		}

		fclose(fh);
	}
	else if (strcmp(argv[0], "servertype")==0)
	{
		for (i=0; i<SERVER_TYPE_MAX; i++)
		{
			if (i == pVoIPCfg->X_CT_servertype)
				boaWrite(wp, "<option value=%d selected>%s</option>", i,server_type[i]);
			else
				boaWrite(wp, "<option value=%d>%s</option>", i,server_type[i]);
		}
	}
	else if (strcmp(argv[0], "serverdisplay")==0)
	{
		for (i=0; i<SERVER_TYPE_MAX; i++)
		{
			if (i == pVoIPCfg->X_CT_servertype)
				boaWrite(wp, "%s", server_type[i]);
		}
	}
	else if (strcmp(argv[0], "sessionEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->proxies[0].SessionUpdateTimer) ? "checked" : "");
	}
	else if (strcmp(argv[0], "sessionExpiry")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].SessionUpdateTimer);
	}
	else if (strcmp(argv[0], "minSessionExpiry")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->MINI_SessionExpire);
	}
	else if (strcmp(argv[0], "sip_display")==0)
	{
		if ((pVoIPCfg->X_CT_servertype == SOFTSWITCH_SIP)||(pVoIPCfg->X_CT_servertype == IMS_SIP))
			boaWrite(wp, "style=\"display:online\"");		
		else
			boaWrite(wp, "style=\"display:none\"");	
	}
	else if (strcmp(argv[0], "h248_display")==0)
	{
		if ((pVoIPCfg->X_CT_servertype != SOFTSWITCH_SIP)&&(pVoIPCfg->X_CT_servertype != IMS_SIP))
			boaWrite(wp, "style=\"display:online\"");		
		else
			boaWrite(wp, "style=\"display:none\"");
	}
	//main proxy
	else if (strcmp(argv[0], "proxy0_addr")==0)
	{
		if(pCfg1->proxies[0].addr != NULL)
		{
			boaWrite(wp, "%s", pCfg1->proxies[0].addr);
		}
		else
			printf("Proxy 0 addr is null!!!\n");
	}
	else if (strcmp(argv[0], "proxy0_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].port);
	}
	else if (strcmp(argv[0], "proxy0_register_addr")==0)
	{
		if(pCfg1->proxies[0].RegistrarServer_addr != NULL)
		{
			boaWrite(wp, "%s", pCfg1->proxies[0].RegistrarServer_addr);
		}
		else
			printf("Proxy 0 register addr is null!!!\n");
	}
	else if (strcmp(argv[0], "proxy0_register_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].RegistrarServer_port);
	}
	else if (strcmp(argv[0], "reg_proxy0_addr")==0)
	{
		if(pCfg1->proxies[0].RegistrarServer_addr != NULL)
		{
			boaWrite(wp, "%s", pCfg1->proxies[0].RegistrarServer_addr);
		}
		else
			printf("Proxy 0 addr is null!!!\n");
	}
	else if (strcmp(argv[0], "reg_proxy0_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].RegistrarServer_port);
	}
	else if (strcmp(argv[0], "proxy0_obEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->proxies[0].outbound_enable) ? "checked" : "");
	}
	else if (strcmp(argv[0], "proxy0_obAddr")==0)
	{
			boaWrite(wp, "%s", pCfg1->proxies[0].outbound_addr);
	}	
	else if (strcmp(argv[0], "proxy0_obPort")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].outbound_port);
	}
	else if (strcmp(argv[0], "proxy0_domain_name")==0)
	{
		boaWrite(wp, "%s", pCfg1->proxies[0].domain_name);
	}
	else if (strcmp(argv[0], "proxy0_reg_expire")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].reg_expire);
	}
	else if (strcmp(argv[0], "proxy0_sessionEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->proxies[0].SessionUpdateTimer) ? "checked" : "");
	}
	else if (strcmp(argv[0], "proxy0_sessionExpiry")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].SessionUpdateTimer);
	}
	else if (strcmp(argv[0], "proxy_cmcc_sessionExpiry")==0)
	{
		boaWrite(wp, "%d", (pCfg1->proxies[0].SessionUpdateTimer/60));
	}
	//second proxy
	else if (strcmp(argv[0], "proxy1_enable")==0)
	{
		if (VOIP_PORTS==1)
		{
			boaWrite(wp, "%s", (pCfg1->proxies[1].enable & PROXY_ENABLED)? "checked" : "");
		}
		else
		{
			boaWrite(wp, "%s", ((pCfg1->proxies[1].enable & PROXY_ENABLED) ||(pVoIPCfg->ports[1].proxies[1].enable & PROXY_ENABLED))? "checked" : "");
		}
		
	}
	else if (strcmp(argv[0], "proxy0_subscribe_enable")==0)
	{
		if (VOIP_PORTS==1)
		{
			boaWrite(wp, "%s", (pCfg1->proxies[0].enable & PROXY_SUBSCRIBE)? "checked" : "");
		}
		else
		{
			boaWrite(wp, "%s", ((pCfg1->proxies[0].enable & PROXY_SUBSCRIBE) ||(pVoIPCfg->ports[1].proxies[0].enable & PROXY_SUBSCRIBE))? "checked" : "");
		}
		
	}
	else if (strcmp(argv[0], "proxy1_subscribe_enable")==0)
	{
		if (VOIP_PORTS==1)
		{
			boaWrite(wp, "%s", (pCfg1->proxies[1].enable & PROXY_SUBSCRIBE)? "checked" : "");
		}
		else
		{
			boaWrite(wp, "%s", ((pCfg1->proxies[1].enable & PROXY_SUBSCRIBE) ||(pVoIPCfg->ports[1].proxies[1].enable & PROXY_SUBSCRIBE))? "checked" : "");
		}
		
	}
	else if (strcmp(argv[0], "proxy1_addr")==0)
	{
		if(pCfg1->proxies[1].addr != NULL)
			boaWrite(wp, "%s", pCfg1->proxies[1].addr);
		else
			printf("Proxy 0 addr is null!!!\n");
	}
	else if (strcmp(argv[0], "proxy1_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].port);
	}
	else if (strcmp(argv[0], "proxy1_register_addr")==0)
	{
		if(pCfg1->proxies[1].RegistrarServer_addr != NULL)
			boaWrite(wp, "%s", pCfg1->proxies[1].RegistrarServer_addr);
		else
			printf("Proxy 0 register addr is null!!!\n");
	}
	else if (strcmp(argv[0], "proxy1_register_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].RegistrarServer_port);
	}
	else if (strcmp(argv[0], "reg_proxy1_addr")==0)
	{
		if(pCfg1->proxies[1].RegistrarServer_addr != NULL)
			boaWrite(wp, "%s", pCfg1->proxies[1].RegistrarServer_addr);
		else
			printf("Proxy 0 addr is null!!!\n");
	}
	else if (strcmp(argv[0], "reg_proxy1_port")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].RegistrarServer_port);
	}	
	else if (strcmp(argv[0], "proxy1_obEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->proxies[1].outbound_enable) ? "checked" : "");
	}
	else if (strcmp(argv[0], "proxy1_obAddr")==0)
	{
		boaWrite(wp, "%s", pCfg1->proxies[1].outbound_addr);
	}
	else if (strcmp(argv[0], "proxy1_obPort")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].outbound_port);
	}	
	else if (strcmp(argv[0], "proxy1_domain_name")==0)
	{
		boaWrite(wp, "%s", pCfg1->proxies[1].domain_name);
	}
	else if (strcmp(argv[0], "proxy1_reg_expire")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].reg_expire);
	}
	else if (strcmp(argv[0], "proxy1_sessionEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->proxies[1].SessionUpdateTimer) ? "checked" : "");
	}
	else if (strcmp(argv[0], "proxy1_sessionExpiry")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[1].SessionUpdateTimer);
	}
	else if (strcmp(argv[0], "init_port_account")==0)
	{
		boaWrite(wp,"portNum = %d;\n", VOIP_PORTS);
		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{
			pCfg = &pVoIPCfg->ports[voip_port];
			boaWrite(wp,"port_line = new Array(%d, '%s','%s','%s');\n",(pCfg->proxies[0].enable & PROXY_ENABLED) ? 1 : 0,
						pCfg->proxies[0].number, pCfg->proxies[0].login_id, pCfg->proxies[0].password);
			boaWrite(wp,"portArray.push(port_line);\n");
		}
	}
	else if (strcmp(argv[0], "port_account")==0)
	{
		boaWrite(wp, "<table id=\"sip_account\" name=\"sip_account\" onClick=\"on_ctrlupdate(this)\" border=0 cellpadding=0 cellspacing=0 width=700>\n");

		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{

			pCfg = &pVoIPCfg->ports[voip_port];

#ifdef CONFIG_NEW_WEB_STYLE
			boaWrite(wp, "<div class=\"column\">" \
						   "<div class=\"column_title\">" \
						     "<div class=\"column_title_left\"></div>" \
						       "<p>Line %d Account</p>" \
						     "<div class=\"column_title_right\"></div>" \
						   "</div>" \
						   "<div class=\"data_common\">" \
						     "<table>" \
						       "<tr>\n" \
						         "<th width=\"200px\">Enable</th>" \
						         "<td><input name=port%d_account_enable type=checkbox value=enable %s ></td>" \
						       "</tr>\n", 
					  voip_port+1, voip_port+1, (pCfg->proxies[0].enable & PROXY_ENABLED) ? "checked" : "");

			boaWrite(wp, "<tr>\n" \
						   "<th>Subscriber Number:</th>" \
						   "<td><input name=port%d_number size=16 maxlength=32 style=width:140px value=%s></td>\n" \
						 "</tr>\n", 
					  voip_port+1, pCfg->proxies[0].number);

			boaWrite(wp, "<tr>\n" \
						   "<th>User Account:</th>" \
						   "<td><input name=port%d_login_id size=16 maxlength=32 style=width:140px value=%s></td>\n" \
						 "</tr>\n", 
					  voip_port+1, pCfg->proxies[0].login_id);

			boaWrite(wp, "<tr>\n" \
						   "<th>User Password:</th>" \
						   "<td><input name=port%d_password size=16 maxlength=32 type=password autocomplete=\"off\" style=width:140px value=%s></td>\n" \
						 "</tr>\n", 
					  voip_port+1, pCfg->proxies[0].password);

			boaWrite(wp, "</table>\n" \
				         "</div>\n" \
				         "</div>");
#else
			boaWrite(wp, "<tr><td><b id=\"sip_account_caption\" name==\"sip_account_caption\">线路%d 账号</b></tr></td>\n"  \
			"<tr>\n" \
			"<td width=200px>启用</td><td><input name=port%d_account_enable type=checkbox value=enable %s ></td>\n" \
			"</tr>\n" \
			, voip_port+1, voip_port+1, (pCfg->proxies[0].enable & PROXY_ENABLED) ? "checked" : "");
			
			boaWrite(wp,"<tr>\n" \
			"<td>用户号码:</td><td><input name=port%d_number size=16 maxlength=32 style=width:140px value=%s></td>\n"
			"</tr>\n" \
			,voip_port+1, pCfg->proxies[0].number);
			
			boaWrite(wp,"<tr>\n" \
			"<td>用户账号:</td><td><input name=port%d_login_id size=16 maxlength=32 style=width:140px value=%s></td>\n"
			"</tr>\n" \
			,voip_port+1, pCfg->proxies[0].login_id);
			
			if (strcmp(pCfg->proxies[0].password,"")==0)
			{
				boaWrite(wp,"<tr>\n" \
				"<td>用户密码:</td><td><input name=port%d_password size=16 maxlength=32 type=password autocomplete=\"off\" style=width:140px value=%s ></td>\n"
				"</tr>\n" , voip_port+1,  "");
			}else
			{
			boaWrite(wp,"<tr>\n" \
			"<td>用户密码:</td><td><input name=port%d_password size=16 maxlength=32 type=password autocomplete=\"off\" style=width:140px value=%s></td>\n"
				"</tr>\n" , voip_port+1,  PWD_DISPLAY);
			}

					
#endif
		}
		boaWrite(wp, "</table>\n");
	}	
	else if(strcmp(argv[0], "cmcc_port_account")==0)
	{
	
		boaWrite(wp,"<tr>\n" \
		"<td width=200px>用户号码:</td><td><input name=account_number size=16 maxlength=32 style=width:140px value=></td>\n"
		"</tr>\n" \
		 );
		
		boaWrite(wp,"<tr>\n" \
		"<td>用户账号:</td><td><input name=account_login_id size=16 maxlength=80 style=width:140px value=></td>\n"
		"</tr>\n" \
		);
		

		boaWrite(wp,"<tr>\n" \
		"<td>用户密码:</td><td><input name=account_password size=16 maxlength=32 type=password style=width:140px value= ></td>\n"
		"</tr>\n" );

	}
	else if(strcmp(argv[0], "cmcc_port_account_table")==0)
	{

		boaWrite(wp, "<tr>\n");
		boaWrite(wp, "<td align=\"center\" style=\"WIDTH: 8%\">  </td>\n");
		boaWrite(wp, "<td align=\"center\" style=\"WIDTH: 8%\"> 编号 </td>\n");
		boaWrite(wp, "<td align=\"center\" style=\"WIDTH: 14%\"> 注册用户名 </td>\n");
		boaWrite(wp, "<td align=\"center\" style=\"WIDTH: 30%\"> 鉴权用户名 </td>\n");
		boaWrite(wp, "<td align=\"center\" style=\"WIDTH: 10%\"> 密码 </td>\n");
		boaWrite(wp, "</tr>\n");

		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{
			pCfg = &pVoIPCfg->ports[voip_port];
			//for (i=0; i< MAX_PROXY; i++)
			for (i=0; i< 1; i++) //temp cmcc current only use one account
			{
				if (pCfg->proxies[i].enable & PROXY_ENABLED){
					account_number++;
					
					boaWrite(wp, "<tr><td align=\"center\"><input name=port%d_account%d_del type=checkbox value=enable %s></td>\n" \
					, voip_port+1,i+1, "");
					
					boaWrite(wp, "<td align=\"center\"><b id=\"sip_account_caption\" name==\"sip_account_caption\"> %d </b></td>\n", i+1);
					if ((pVoIPCfg->rfc_flags & REGISTER_FROM_USE_LOGINID)&&(strlen(pCfg->proxies[i].number)==0)){
						getNumberFromLogin_id(tmp,pCfg->proxies[i].login_id);
						boaWrite(wp,"<td align=\"center\">%s</td>\n",tmp);
					}else{
						boaWrite(wp,"<td align=\"center\">%s</td>\n",pCfg->proxies[i].number);
					}					 
					
					boaWrite(wp,"<td align=\"center\">%s</td>\n"
					, pCfg->proxies[i].login_id);
					
					if (strcmp(pCfg->proxies[i].password,"")==0)
					{
						boaWrite(wp,"<td align=\"center\">%s </td>\n"
							"</tr>\n" ,  "");
					}
					else
					{
						boaWrite(wp,"<td align=\"center\">%s</td>\n"
							"</tr>\n" ,   PWD_DISPLAY);
					}
				}
			}	
		}
		boaWrite(wp, "<input type=\"hidden\" name=\"account_number\" value=%d>\n" , account_number);
	}
#ifdef CONFIG_USER_RTK_H248	
	else if (strcmp(argv[0], "h248_account")==0)
	{
		boaWrite(wp, "<table id=\"h248_account\" name=\"h248_account\" onClick=\"on_ctrlupdate(this)\" border=0 cellpadding=0 cellspacing=0 width=700>\n");
		
		for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
		{
			pCfg = &pVoIPCfg->ports[voip_port];

			boaWrite(wp, "<tr><td><b id=\"h248_account_caption\" name==\"h248_account_caption\">线路%d</b></tr></td>\n"  \
			"<tr>\n" \
			"<td width=200px>启用</td><td><input name=h248_port%d_account_enable type=checkbox value=enable %s ></td>\n" \
			"</tr>\n" \
			, voip_port+1, voip_port+1, (pCfg->proxies[0].enable & PROXY_ENABLED) ? "checked" : "");	
		}
		boaWrite(wp, "</table>\n");	
	}
    else if (strcmp(argv[0], "h248_port")==0)
		boaWrite(wp, "%d", pVoIPCfg->mg_port);
    else if (strcmp(argv[0], "h248_mgc0_address")==0)
		boaWrite(wp, "%s", pVoIPCfg->mgc_ip);
    else if (strcmp(argv[0], "h248_mgc0_port")==0)
		boaWrite(wp, "%d", pVoIPCfg->mgc_port);
    else if (strcmp(argv[0], "h248_mgc1_address")==0)
		boaWrite(wp, "%s", pVoIPCfg->bak_mgc_ip);
    else if (strcmp(argv[0], "h248_mgc1_port")==0)
		boaWrite(wp, "%d", pVoIPCfg->bak_mgc_port);
    else if (strcmp(argv[0], "h248_id_type")==0)
	{
		for (i=0; i<H248_ID_TYPE_MAX; i++)
		{
			if (i == pVoIPCfg->mg_deviceIdType)
				boaWrite(wp, "<option value=%d selected>%s</option>",i, id_type[i]);
			else
				boaWrite(wp, "<option value=%d>%s</option>",i, id_type[i]);
		}
	}
    else if (strcmp(argv[0], "h248_termination_id")==0)
    {
        boaWrite(wp, "%s", pVoIPCfg->mg_name);
    }
    else if (strcmp(argv[0], "h248_domain_name")==0)
    {
        boaWrite(wp, "%s", pVoIPCfg->Domain);
    }
    else if (strcmp(argv[0], "h248_encoding_type")==0)
	{
        for (i=0; i<H248_ENCODING_MAX; i++)
		{
			if(strcmp(encoding_type[i],pVoIPCfg->mg_encoding)==0)
				boaWrite(wp, "<option value=%d selected>%s</option>",i, encoding_type[i]);
			else
				boaWrite(wp, "<option value=%d>%s</option>",i, encoding_type[i]);
		}
	}
	else if (strcmp(argv[0], "h248_phy_term_config_method")==0)
	{
        for (i=0; i<H248_PHYTERM_METHOD_MAX; i++)
		{
			if (i == pVoIPCfg->mg_phyTermConfigMethod)
				boaWrite(wp, "<option value=%d selected>%s</option>",i, phy_term_method[i]);
			else
				boaWrite(wp, "<option value=%d>%s</option>",i, phy_term_method[i]);
		}
	}
    else if (strcmp(argv[0], "h248_phy_term_id")==0)
		boaWrite(wp, "%s", pVoIPCfg->mg_phyTermID);
    else if (strcmp(argv[0], "h248_phy_term_prefix")==0)
		boaWrite(wp, "%s", pVoIPCfg->mg_prefix);
    else if (strcmp(argv[0], "h248_phy_term_start")==0)
		boaWrite(wp, "%d", pVoIPCfg->mg_phyTermPrefixStart);
    else if (strcmp(argv[0], "h248_phy_term_len")==0)
		boaWrite(wp, "%d", pVoIPCfg->mg_phyTermPrefixLen);
    else if (strcmp(argv[0], "h248_rtp_term_prefix")==0)
		boaWrite(wp, "%s", pVoIPCfg->mg_rtp_prefix);
    else if (strcmp(argv[0], "h248_rtp_term_start")==0)
		boaWrite(wp, "%d", pVoIPCfg->mg_rtpPrefixStart);
    else if (strcmp(argv[0], "h248_rtp_term_len")==0)
		boaWrite(wp, "%d", pVoIPCfg->mg_rtpPrefixLen);
#endif

	//sip config 2
	else if (strcmp(argv[0], "sipPort")==0)
		boaWrite(wp, "%d", pCfg1->sip_port);
	else if (strcmp(argv[0], "rtpPort")==0)
		boaWrite(wp, "%d", pCfg1->media_port);
	else if (strcmp(argv[0], "dtmfMode")==0)
	{
		for (i=0; i<E8C_DTMF_MAX; i++)
		{
            if ((sip_display_mode == 0) && (i == 1))
                continue; 

			if (i == pCfg1->dtmf_mode)
				boaWrite(wp, "<option value=%d selected>%s</option>", i, e8c_dtmf[i]);
			else
				boaWrite(wp, "<option value=%d >%s</option>",i, e8c_dtmf[i]);
		}
	}
	else if (strcmp(argv[0], "useLec")==0)
		boaWrite(wp, "%s", (pCfg1->lec) ? "checked" : "");
	else if (strcmp(argv[0], "useVad")==0)
		boaWrite(wp, "%s", (pCfg1->vad) ? "checked" : "");
	else if (strcmp(argv[0], "useCng")==0)
		boaWrite(wp, "%s", (pCfg1->cng) ? "checked" : "");
	else if(strcmp(argv[0], "useT38")==0)
		boaWrite(wp, "%s", (pCfg1->useT38) ? "checked" : "");
    else if (strcmp(argv[0], "V152_codec_type")==0)
	{
		boaWrite(wp,
			"<option value=0 %s>PCMU</option>"
            "<option value=8 %s>PCMA</option>"
			,!(pCfg1->v152_codec_type) ? "selected" : ""
			, (pCfg1->v152_codec_type) ? "selected" : ""
		);
	}
	else if (strcmp(argv[0], "caller_id")==0)
	{
		boaWrite(wp, "<select name=caller_id >");
		if (CID_FSK_ETSI == (pCfg1->caller_id_mode & 7)){
			boaWrite(wp, "<option value=%d selected>%s</option>",CID_FSK_ETSI ,e8c_cid[0]);
			boaWrite(wp, "<option value=%d >%s</option>", CID_DTMF,e8c_cid[1]);
		}
		else{
			boaWrite(wp, "<option value=%d >%s</option>", CID_FSK_ETSI, e8c_cid[0]);
			boaWrite(wp, "<option value=%d selected>%s</option>", CID_DTMF, e8c_cid[1]);
		}
		boaWrite(wp, "</select>");
	}
	else if (strcmp(argv[0], "fax_type")==0)
	{
		boaWrite(wp, "<select name=fax_type >");

        if (sip_display_mode) 
        {
            if (pCfg1->useT38) {
                boaWrite(wp, "<option value=1 selected>T38</option>");
                boaWrite(wp, "<option value=0>T30全控</option>");
            }
            else{
                boaWrite(wp, "<option value=1>T38</option>");
                boaWrite(wp, "<option value=0 selected>T30全控</option>");
            }
        }
        else
        {
            if (pCfg1->useT38) {
                boaWrite(wp, "<option value=1 selected>T38</option>");
                boaWrite(wp, "<option value=0>T30</option>");
            }
            else{
                boaWrite(wp, "<option value=1>T38</option>");
                boaWrite(wp, "<option value=0 selected>T30</option>");
            }
        }
        boaWrite(wp, "</select>");
	}
	else if (strcmp(argv[0], "flash_hook_time")==0)
	{
		boaWrite(wp, "<input type=text name=flash_hook_time_min size=4 maxlength=5 value=%d >" \
			" <  Flash Time  < " \
			"<input type=text name=flash_hook_time size=4 maxlength=5 value=%d >",
			pCfg1->flash_hook_time_min,
			pCfg1->flash_hook_time);
	}
	else if (strcmp(argv[0], "e8c_codec")==0)
	{
		asp_e8c_codec_get(wp, pCfg1);
	}
	else if (strcmp(argv[0], "cmcc_codec")==0)
	{
		asp_cmcc_codec_get(wp, pCfg1);
	}
	else if (strcmp(argv[0], "off_hook_alarm")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->off_hook_alarm);
	}
	else if (strcmp(argv[0], "auto_dial")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->auto_dial);
	}
	else if (strcmp(argv[0], "InterDigitTimerLong")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->InterDigitTimerLong);
	}
	else if (strcmp(argv[0], "BusyToneTimer")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->BusyToneTimer);
	}
	else if (strcmp(argv[0], "HangingReminderToneTimer")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->HangingReminderToneTimer);
	}
	else if (strcmp(argv[0], "RegisterRetryInterval")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].RegisterRetryInterval);
	}
	else if (strcmp(argv[0], "HeartbeatCycle")==0)
	{
		boaWrite(wp, "%d", pCfg1->proxies[0].HeartbeatCycle);
	}
	else if (strcmp(argv[0], "NoAnswerTimer")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->NoAnswerTimer);
	}
	else if (strcmp(argv[0], "rtpDscp")==0)
	{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		boaWrite(wp, "%d", pVoIPCfg->rtpDscp);
#else		
		for (i=0; i<DSCP_MAX ;i++)
		{
			if (i == pVoIPCfg->rtpDscp)
				boaWrite(wp, "<option value=%d selected>%s</option>", i,e8c_dscp[i]);
			else
				boaWrite(wp, "<option value=%d>%s</option>", i, e8c_dscp[i]);
		}
#endif		
	}
	else if (strcmp(argv[0], "sipDscp")==0)
	{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		boaWrite(wp, "%d", pVoIPCfg->sipDscp);
#else		
		for (i=0; i<DSCP_MAX ;i++)
		{
			if (i == pVoIPCfg->sipDscp)
				boaWrite(wp, "<option value=%d selected>%s</option>", i, e8c_dscp[i]);
			else
				boaWrite(wp, "<option value=%d >%s</option>", i, e8c_dscp[i]);
		}
#endif		
	}
	else if (strcmp(argv[0], "dialplan")==0){
		boaWrite(wp, "%s", pCfg1->dialplan);
	}
	else if (strcmp(argv[0], "digitmap_enable")==0) {
    	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_enable\" value=1 %s>on",
    				( pCfg1 ->digitmap_enable ? "checked" : "" ));
    	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_enable\" value=0 %s>off",
    				( !pCfg1 ->digitmap_enable ? "checked" : "" ));
	}
	else if (strcmp(argv[0], "cmcc_digitmap_enable")==0) {
    	boaWrite(wp, "<input type=\"checkbox\"  name=\"digitmap_enable\" size=20 %s>",
    				( pCfg1 ->digitmap_enable ? "checked" : "" ));
	}
	else if (strcmp(argv[0], "digitmap_special")==0)
	{
		boaWrite(wp, "%s", pCfg1->digitmap_special);
	}	
	else if (strcmp(argv[0], "digitmap_special_enable")==0) 
	{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	boaWrite(wp, (pCfg1->digitmap_special_enable) ? "checked" : "");
#else
   	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_special_enable\" value=1 %s>on",
    				( pCfg1 ->digitmap_special_enable ? "checked" : "" ));
    	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_special_enable\" value=0 %s>off",
    				( !pCfg1 ->digitmap_special_enable ? "checked" : "" ));
#endif		
	}
	else if (strcmp(argv[0], "PBXPrefix")==0)
	{
		boaWrite(wp, "%s", pCfg1->PSTN_routing_prefix);
	}
	else if (strcmp(argv[0], "PBXPrefixEnable")==0)
	{
		boaWrite(wp, "%s", (pCfg1->func_flags&SIP_PBXPREFIX_ENABLE) ? "checked" : "");		
	}
	else if (strcmp(argv[0], "pbxdial_second_enable")==0)
	{
		if(pCfg1->func_flags & SIP_PBXDIAL_SECONDARY_ENABLE)
			boaWrite(wp, "%s",  "checked");	
		else
			boaWrite(wp, "%s",  "");
	}
	else if (strcmp(argv[0], "spk_voice_gain")==0)
		boaWrite(wp, "%d", pCfg1->spk_voice_gain);
	else if (strcmp(argv[0], "mic_voice_gain")==0)
		boaWrite(wp, "%d", pCfg1->mic_voice_gain);
	else if (strcmp(argv[0], "call_waiting")==0)
		boaWrite(wp, "%s", (pCfg1->call_waiting_enable) ? "checked" : "");
	else if (strcmp(argv[0], "3_way")==0)
		boaWrite(wp, "%s", (!(pCfg1->fxsport_flag&FXS_3WAY_DISABLE)) ? "checked" : "");
    else if (strcmp(argv[0], "Conference_server_cpe")==0)   
        boaWrite(wp, "%s", (pCfg1->fxsport_flag&FXS_3WAY_ON_SERVER) ? "checked" : "");
	else if (strcmp(argv[0], "conference_uri")==0)   
        boaWrite(wp, "%s", pVoIPCfg->conference_uri);
	else if (strcmp(argv[0], "call_transfer")==0)
		boaWrite(wp, "%s", (!(pCfg1->fxsport_flag&FXS_CALL_TRANSFER_disable)) ? "checked" : "");
	else if (strcmp(argv[0], "uc_call_forward_number")==0)
		boaWrite(wp, "%s", pCfg1->uc_forward);
	else if (strcmp(argv[0], "uc_call_forward")==0)
		boaWrite(wp, "%s", (pCfg1->uc_forward_enable) ? "checked" : "");
	else if (strcmp(argv[0], "busy_call_forward")==0)
		boaWrite(wp, "%s", (pCfg1->busy_forward_enable) ? "checked" : "");
	else if (strcmp(argv[0], "na_call_forward")==0)
		boaWrite(wp, "%s", (pCfg1->na_forward_enable) ? "checked" : "");
	else if (strcmp(argv[0], "na_call_forward_time")==0)
		boaWrite(wp, "%d", pCfg1->na_forward_time);
    else if (strcmp(argv[0], "sip_disable_prack")==0)
		boaWrite(wp, "%s", (!(pVoIPCfg->rfc_flags&SIP_DISABLE_PRACK)) ? "checked" : "");
    else if (strcmp(argv[0], "hookflash_type")==0)
    {
        if (pVoIPCfg->rfc_flags & SIP_FLASHHOOK_INFO){
            boaWrite(wp, "<option>%s</option>", e8c_hookflash_type[0]);
            boaWrite(wp, "<option selected>%s</option>", e8c_hookflash_type[1]);
        }
        else
        {
            boaWrite(wp, "<option selected>%s</option>", e8c_hookflash_type[0]);
            boaWrite(wp, "<option>%s</option>", e8c_hookflash_type[1]);
        }
    }
	else if (strcmp(argv[0], "sip_date")==0)
		boaWrite(wp, "%s", (pVoIPCfg->rfc_flags&SIP_DATE) ? "checked" : "");
	else if (strcmp(argv[0], "reverse_polariy")==0)
	{
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		boaWrite(wp,
			"<option value=0 %s>关闭</option>"
			"<option value=1 %s>启用</option>"
			,!(pVoIPCfg->rfc_flags&FXS_REVERSE_POLARIY_OFF)?  "selected" : ""
			, (pVoIPCfg->rfc_flags&FXS_REVERSE_POLARIY_OFF)?  "selected" : ""
		);
#else
		boaWrite(wp, "%s", (!(pVoIPCfg->rfc_flags&FXS_REVERSE_POLARIY_OFF)) ? "checked" : "");
#endif
	}
	else if (strcmp(argv[0], "hot_line_options")==0)
	{
		boaWrite(wp,
			"<option value=0 %s>关闭</option>"
			"<option value=1 %s>立即热线</option>"
			"<option value=2 %s>延时热线</option>"
			,pCfg1->hotline_enable == 0 ? "selected" : ""
			,pCfg1->hotline_enable == 1 ? "selected" : ""
			,pCfg1->hotline_enable == 2 ? "selected" : ""
		);
	}
	else if (strcmp(argv[0], "session_options")==0)
	{
		boaWrite(wp,
			"<option value=0 %s>UPDATE</option>"
			"<option value=1 %s>INVITE</option>"
			,!(pVoIPCfg->rfc_flags&SIP_SESSION_TIMER_USE_INVITE)?  "selected" : ""
			,(pVoIPCfg->rfc_flags&SIP_SESSION_TIMER_USE_INVITE)? "selected" : ""
		);
	}	
	else if (strcmp(argv[0], "digit_map_mode")==0)
	{
		boaWrite(wp,
			"<option value=0 %s>最大匹配</option>"
			"<option value=1 %s>最小匹配</option>"
			,!(pVoIPCfg->rfc_flags&SIP_DIGIT_MAP_MODE_MIN)? "selected" : ""
			,(pVoIPCfg->rfc_flags&SIP_DIGIT_MAP_MODE_MIN)?  "selected" : ""
		);
	}
	else if (strcmp(argv[0], "FAX_options")==0)
	{
        if (sip_display_mode) 
        {
		boaWrite(wp,
			"<option value=0 %s>协商模式</option>"
			"<option value=1 %s>自动模式</option>"
			,!(pVoIPCfg->rfc_flags&SIP_FAX_AUTO)? "selected" : ""
			,(pVoIPCfg->rfc_flags&SIP_FAX_AUTO)? "selected" : ""
		);
	}
        else
        {
            boaWrite(wp,
                "<option value=0 %s>T30全控</option>"
                "<option value=1 %s>自协商</option>"
                ,!(pVoIPCfg->rfc_flags&SIP_FAX_AUTO)? "selected" : ""
                ,(pVoIPCfg->rfc_flags&SIP_FAX_AUTO)? "selected" : ""
            );
        }
	}
	else if (strcmp(argv[0], "SUBS_options")==0)
	{
		boaWrite(wp,
			"<option value=3 %s>不订阅</option>"
			"<option value=0 %s>订阅全部</option>"
			"<option value=1 %s>订阅注册</option>"
			"<option value=2 %s>订阅业务</option>"
			,!(pCfg1->proxies[0].enable & PROXY_SUBSCRIBE) ? "selected" : ""
			,((pCfg1->proxies[0].enable & PROXY_SUBSCRIBE)&&(pVoIPCfg->subs_type==SUBS_TYPE_ALL))? "selected" : ""
			,((pCfg1->proxies[0].enable & PROXY_SUBSCRIBE)&&(pVoIPCfg->subs_type==SUBS_TYPE_REG))? "selected" : ""
			,((pCfg1->proxies[0].enable & PROXY_SUBSCRIBE)&&(pVoIPCfg->subs_type==SUBS_TYPE_EVENT))? "selected" : ""
		);
	}
	else if (strcmp(argv[0], "SUBS_time")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->subs_time);
	}
	else if (strcmp(argv[0], "hotline_number")==0)
	{
		boaWrite(wp, "%s", pCfg1->hotline_number);
	}
	else if (strcmp(argv[0], "sip_debug")==0)
	{
		boaWrite(wp, "%s",(pVoIPCfg->rfc_flags&SIP_DEBUG) ? "checked" : "");
	}
	else if (strcmp(argv[0], "RingVoltage")==0)
	{
		boaWrite(wp,
			"<option value=40 %s>40 V</option>"
			"<option value=50 %s>50 V</option>"
			"<option value=60 %s>60 V</option>"
			"<option value=70 %s>70 V</option>"
			"<option value=80 %s>80 V</option>"
			"<option value=90 %s>90 V</option>"
			,pVoIPCfg->ringVoltageFreq == 40 ? "selected" : ""
			,pVoIPCfg->ringVoltageFreq == 50 ? "selected" : ""
			,pVoIPCfg->ringVoltageFreq == 60 ? "selected" : ""
			,((pVoIPCfg->ringVoltageFreq == 0)||((pVoIPCfg->ringVoltageFreq == 70))) ? "selected" : ""
			,pVoIPCfg->ringVoltageFreq == 80 ? "selected" : ""
			,pVoIPCfg->ringVoltageFreq == 90 ? "selected" : ""

		);
	}
    else if (strcmp(argv[0], "pound_to_ascii")==0)
    {
		boaWrite(wp, "%s", (pVoIPCfg->rfc_flags&SIP_ENABLE_POUND_KEY_TO_ASCII) ? "checked" : ""); 
    } 
    else if (strcmp(argv[0], "InterDigitTimerLong2")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->InterDigitTimerLong);
	}
    else if (strcmp(argv[0], "HangingReminderToneTimer2")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->HangingReminderToneTimer);
	}
    else if (strcmp(argv[0], "JitterBufferEnable")==0)
    {
        boaWrite(wp, "%s", (pCfg1->jitter_factor) ? "checked" : "");       
    }
    else if (strcmp(argv[0], "jitterDelay")==0)
    {
        asp_e8c_jitterDelay(wp, pCfg1->jitter_delay);
    }
    else if (strcmp(argv[0], "dtmf_2833_pt")==0)
	{
		boaWrite(wp, "%d", pCfg1->dtmf_2833_pt);
	}
    else if (strcmp(argv[0], "heartbeat_mode")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->heartbeat_mode);

        for (i=0; i<H248_HEARTBEAT_MAX ;i++)
		{
			if (i == pVoIPCfg->heartbeat_mode)
				boaWrite(wp, "<option value=%d selected>%s</option>",i, heartbeat_mode[i]);
	else
				boaWrite(wp, "<option value=%d>%s</option>",i, heartbeat_mode[i]);
		}
	}
    else if (strcmp(argv[0], "sip_reg_expire")==0)
	{
        boaWrite(wp, "%d", pCfg1->proxies[0].reg_expire);
    }
    else if (strcmp(argv[0], "NtpEnable")==0)
    {
        unsigned char ntpEnabled;

        mib_get(MIB_NTP_ENABLED, &ntpEnabled);

        if (ntpEnabled == 1)
            boaWrite(wp, "%s", "checked");
    }
	else if (strcmp(argv[0], "SCportstatus")==0)
	{
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_VoiceInfoEnable_)
#ifdef TERMINAL_INSPECTION_SC
		int portstatus=0, voip_port = 0; 		  
		if (2 != pVoIPCfg->X_CT_servertype)
		{
			//for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
            if(pVoIPCfg->ports[voip_port].proxies[0].number != NULL)
			{
				extern int VoiceStateValue(unsigned int port);
				portstatus = VoiceStateValue(voip_port);
				if( 0 == portstatus)
				{
					boaWrite(wp, "<td>%s</td>","空闲"); 
				}else{
					boaWrite(wp, "<td>%s</td>","占用"); 
				}
            }
		}
#endif
#endif
	}
    else if (strcmp(argv[0], "heartbeat_mode") == 0)
    {
		if (pVoIPCfg->rfc_flags & SIP_OPTION)
		{
			boaWrite(wp, "<option value=0 selected>NOTIFY方式</option>");
			boaWrite(wp, "<option value=1>被动心跳回应</option>");
		}else
		{
			boaWrite(wp, "<option value=0 >NOTIFY方式</option>");
			boaWrite(wp, "<option value=1 selected>被动心跳回应</option>");
		}	
	}
#if defined(SUPPORT_RTP_REDUNDANT_APP)
    else if (strcmp(argv[0], "rtp_redundant_codec_options") == 0) 
    {
		boaWrite(wp,
			"<option value=-1>%s</option>"
			"<option value=0 %s>PCM u-law</option>"
			"<option value=8 %s>PCM a-law</option>"
#ifdef CONFIG_RTK_VOIP_G729AB
			"<option value=18 %s>G.729</option>"
#endif
			,"关闭"
			,pCfg1->rtp_redundant_codec == 0 ? "selected" : ""
			,pCfg1->rtp_redundant_codec == 8 ? "selected" : ""
#ifdef CONFIG_RTK_VOIP_G729AB
			,pCfg1->rtp_redundant_codec == 18 ? "selected" : ""
#endif
		);
	}
    else if (strcmp(argv[0], "rtp_redundant_payload_type") == 0)
    {
		boaWrite(wp, "%d", pCfg1->rtp_redundant_payload_type);
    }
#endif //SUPPORT_RTP_REDUNDANT_APP

    else if (strcmp(argv[0], "maxDelay") == 0)
	{
        asp_cmcc_maxDelay(wp, pCfg1->maxDelay);
	}
	
    else if (strcmp(argv[0], "jitterFactor")==0)
    {
        boaWrite(wp, "%s",(pCfg1->jitter_factor) ? "checked" : "");
    }
    else if (strcmp(argv[0], "transport_protocol") == 0)
	{
        boaWrite(wp,
			"<option value=0 %s>UDP</option>"
			"<option value=1 %s>TCP</option>"
			,pVoIPCfg->transport_protocol == 0 ? "selected" : ""
			,pVoIPCfg->transport_protocol == 1 ? "selected" : ""
		);
	}
    else if (strcmp(argv[0], "callid_from_PAI") == 0)
	{
        boaWrite(wp,
			"<option value=0 %s>强制从FROM域</option>"
			"<option value=1 %s>优先从PAI域</option>"
			,!(pVoIPCfg->rfc_flags & SIP_P_ASSERTED_IDENTITY) ? "selected" : ""
			, (pVoIPCfg->rfc_flags & SIP_P_ASSERTED_IDENTITY) ? "selected" : ""
		);
	}
    else if (strcmp(argv[0], "display_prefix_num") == 0)
	{
        boaWrite(wp,
			"<option value=0 %s>不显示国家代码/地区代码</option>"
			"<option value=1 %s>显示国家代码/地区代码</option>"
			, pVoIPCfg->display_prefix_num == 0 ? "selected" : ""
			, pVoIPCfg->display_prefix_num == 1 ? "selected" : ""
		);
	}
    else if (strcmp(argv[0], "url_type") == 0)
	{
        boaWrite(wp,
			"<option value=0 %s>sip格式</option>"
			"<option value=1 %s>tel格式</option>"
            ,!(pVoIPCfg->rfc_flags & SIP_URL_TYPE) ? "selected" : ""
            , (pVoIPCfg->rfc_flags & SIP_URL_TYPE) ? "selected" : ""

		);
	}   
#if 1 //voip parameters, need to implement
    else if (strcmp(argv[0], "max_start_time") == 0)
    {
		boaWrite(wp, "%d", pVoIPCfg->max_start_time);
    }
    else if (strcmp(argv[0], "t_timer") == 0)
    {
		boaWrite(wp, "%d", pVoIPCfg->t_timer);
    }
    else if (strcmp(argv[0], "digit_timer_mode") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>拨号短定时器</option>"
			"<option value=1 %s>拨号长定时器</option>"
			"<option value=2 %s>拨号T定时器</option>"
			,pVoIPCfg->digit_timer_mode == 0 ? "selected" : ""
			,pVoIPCfg->digit_timer_mode == 1 ? "selected" : ""
			,pVoIPCfg->digit_timer_mode == 2 ? "selected" : ""
		);
    }
    else if (strcmp(argv[0], "hotline_mode") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>上报用户号码</option>"
			"<option value=1 %s>上报NoDial</option>"
			"<option value=2 %s>上报激活码</option>"
			,pCfg1->hotline_mode == 0 ? "selected" : ""
			,pCfg1->hotline_mode == 1 ? "selected" : ""
			,pCfg1->hotline_mode == 2 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "register_wtih_port") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>不带端口号</option>"
			"<option value=1 %s>带端口号</option>"
			,pVoIPCfg->register_wtih_port == 0 ? "selected" : ""
			,pVoIPCfg->register_wtih_port == 1 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "user_equal_phone") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>不带user=phone</option>"
			"<option value=1 %s>带user=phone</option>"
			,!(pVoIPCfg->rfc_flags & SIP_USER_EQUAL_PHONE) ? "selected" : ""
			,(pVoIPCfg->rfc_flags & SIP_USER_EQUAL_PHONE) ? "selected" : ""
		);
    }
    else if (strcmp(argv[0], "hotline_activate")==0)
	{
		boaWrite(wp, "%s", pCfg1->hotline_activate);
	}
	else if (strcmp(argv[0], "call_waiting_type") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>直接切换</option>"
			"<option value=1 %s>加按数字键切换</option>"
			,pVoIPCfg->call_waiting_type == 0 ? "selected" : ""
			,pVoIPCfg->call_waiting_type == 1 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "unreg_dial_tone") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>关闭拨号音</option>"
			"<option value=1 %s>启用拨号音</option>"
			,pVoIPCfg->unreg_dial_tone == 0 ? "selected" : ""
			,pVoIPCfg->unreg_dial_tone == 1 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "codec_prior_control") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>主叫优先</option>"
			"<option value=1 %s>本地优先</option>"
			,pVoIPCfg->codec_prior_control == 0 ? "selected" : ""
			,pVoIPCfg->codec_prior_control == 1 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "business_type") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>None</option>"
			"<option value=1 %s>Local</option>"
			"<option value=2 %s>INFO</option>"
			"<option value=3 %s>CTC_IMS</option>"
			,pVoIPCfg->business_type == 0 ? "selected" : ""
			,pVoIPCfg->business_type == 1 ? "selected" : ""
			,pVoIPCfg->business_type == 2 ? "selected" : ""
			,pVoIPCfg->business_type == 3 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "terminator_type") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>终止模式</option>"
			"<option value=1 %s>规则模式</option>"
			,pVoIPCfg->terminator_type == 0 ? "selected" : ""
			,pVoIPCfg->terminator_type == 1 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "at_to_ascii") == 0)
    {
		boaWrite(wp, "%s", (pVoIPCfg->rfc_flags&SIP_ENABLE_AT_KEY_TO_ASCII) ? "checked" : ""); 
    }
	else if (strcmp(argv[0], "priority_unregister") == 0)
    {
		boaWrite(wp, "%s", (pVoIPCfg->priority_unregister) ? "checked" : ""); 
    }
	else if (strcmp(argv[0], "cal_terminal_time") == 0)
    {
		boaWrite(wp, "%s", (pVoIPCfg->cal_terminal_time) ? "checked" : ""); 
    }
	else if (strcmp(argv[0], "minSessionExpiry") == 0)
    {
		boaWrite(wp, "%d", pVoIPCfg->MINI_SessionExpire);
    }
	else if (strcmp(argv[0], "uac_uas") == 0)
    {
		boaWrite(wp,
			"<option value=0 %s>none</option>"
			"<option value=1 %s>uac</option>"
			"<option value=2 %s>uas</option>"
			,pVoIPCfg->uac_uas == 0 ? "selected" : ""
			,pVoIPCfg->uac_uas == 1 ? "selected" : ""
			,pVoIPCfg->uac_uas == 2 ? "selected" : ""
		);
    }
	else if (strcmp(argv[0], "min_onhook_time") == 0)
    {
		boaWrite(wp, "%d", pVoIPCfg->min_onhook_time);
    }
	else if (strcmp(argv[0], "sync_phone_time") == 0)
	{
		boaWrite(wp,
			"<option value=0 %s>自动</option>"
			"<option value=1 %s>强制带时间</option>"
			"<option value=2 %s>强制不带时间</option>"
			,pVoIPCfg->sync_phone_time == 0 ? "selected" : ""
			,pVoIPCfg->sync_phone_time == 1 ? "selected" : ""
			,pVoIPCfg->sync_phone_time == 2 ? "selected" : ""
		);
	}
	else if (strcmp(argv[0], "enable_802_11p") == 0)
	{
		boaWrite(wp, "%s", (pVoIPCfg->enable_802_11p) ? "checked" : ""); 
	}
#endif //voip parameters, need to implement
	else
	{
		return -1;
	}

	return 0;
}

//[SD6, bohungwu, e8c web
//#define DBG_WEB_VAR(VAR_NAME) printf(#VAR_NAME "=%s\n", websGetVar(wp, T(#VAR_NAME), "0"))
#define DBG_WEB_VAR(VAR_NAME) do {} while(0)
//#define RCM_DBG printf("%s-%d\n", __FILE__, __LINE__)
#define RCM_DBG do {} while(0)


void asp_voip2_e8c_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	char szPrecedence[12];
	char redirect_url[50];
	int voip_port;
	char szName[64];
	char *ptr;
	int ptr_index=0;
	int caller_id_area=0;
	printf("---Enter %s---\n", __FUNCTION__);
	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	RCM_DBG;

	//E8C support SIP REDUNDANCY 
	pVoIPCfg->rfc_flags |=SIP_SERVER_REDUNDANCY;
		
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		
		strcpy((char *) pCfg->dialplan, boaGetVar(wp, "dialplan", ""));
		pCfg ->digitmap_enable =  atoi( boaGetVar(wp, "digitmap_enable", "") );
		strcpy((char *) pCfg->digitmap_special, boaGetVar(wp, "digitmap_special", ""));
		strcpy((char *) pCfg->PSTN_routing_prefix, boaGetVar(wp, "PBXPrefix", ""));
		pCfg ->digitmap_enable =  atoi( boaGetVar(wp, "digitmap_enable", "") );
		pCfg ->digitmap_special_enable =  atoi( boaGetVar(wp, "digitmap_special_enable", "") );

		if(!gstrcmp(boaGetVar(wp, "PBXPrefixEnable", ""), "on")){
			pCfg->func_flags |=SIP_PBXPREFIX_ENABLE;
		}else
			pCfg->func_flags &= ~SIP_PBXPREFIX_ENABLE;
		

		if(!gstrcmp(boaGetVar(wp, "pbxdial_second_enable", ""), "on")){
			pCfg->func_flags |=SIP_PBXDIAL_SECONDARY_ENABLE;
			//print("pbxdial_second_enable is on\n");
		}else{
			pCfg->func_flags &= ~SIP_PBXDIAL_SECONDARY_ENABLE;
			//print("pbxdial_second_enable is off\n");
		}
		

		
		//sipPort
		pCfg->sip_port = atoi(boaGetVar(wp, "sipPort", "5060"));

		//rtp port
		if(voip_port==0)
			pCfg->media_port 	= atoi(boaGetVar(wp, "rtpPort", "9000"));
		else
			pCfg->media_port = atoi(boaGetVar(wp, "rtpPort", "9000"))+4;

		//dtmf relay mode
		ptr_index = atoi (boaGetVar(wp, "dtmfMode", ""));


		pCfg->dtmf_mode = ptr_index;
		
		//echo cancellation
		i = atoi(boaGetVar(wp, "echo_cancellation", ""));
		if(i == 0)
		{
			pCfg->lec = 0;
		}
		else if(i == 1)
		{
			pCfg->lec = 1;
		}
		else
		{
			fprintf(stderr, "Error, illega echo cancelation setting=%d\n", i);
		}

		//vad
		pCfg->vad = !gstrcmp(boaGetVar(wp, "useVad", ""), "on");
		
		//T38
		//pCfg->useT38 = !gstrcmp(boaGetVar(wp, "fax_type", ""), "T38");
		pCfg->useT38 = atoi(boaGetVar(wp, "fax_type", ""));
		//caller id
#if 0		
		ptr	 = boaGetVar(wp, "caller_id", "");		
		for(i=0; i<CID_MAX; i++)
		{
			if (!gstrcmp(ptr, e8c_cid[i]))
				break;
		}
		if (i == CID_MAX)
			i = CID_DTMF;
#endif
		i=atoi(boaGetVar(wp, "caller_id", ""));
		caller_id_area=(pCfg->caller_id_mode&0xff8);
		pCfg->caller_id_mode = i;
		pCfg->caller_id_mode|=caller_id_area;
		//flash hook time

	    i = atoi(boaGetVar(wp, "flash_hook_time", ""));
		if ((i >= 100) && ( i <= 2000))
			pCfg->flash_hook_time = i;
		else
			pCfg->flash_hook_time = 300;

	        i = atoi(boaGetVar(wp, "flash_hook_time_min", ""));
		if ( i >= pCfg->flash_hook_time )
			pCfg->flash_hook_time_min = 0;
		else
			pCfg->flash_hook_time_min = i;

		//sessionEnable
		if( strcmp("enable", boaGetVar(wp, "sessionEnable", "")) == 0){
			//sessionExpiry
			pCfg->proxies[0].SessionUpdateTimer = atoi(boaGetVar(wp, "sessionExpiry", "90"));
			pCfg->proxies[1].SessionUpdateTimer = atoi(boaGetVar(wp, "sessionExpiry", "90"));
		}else{
			pCfg->proxies[0].SessionUpdateTimer =0;
			pCfg->proxies[1].SessionUpdateTimer =0;
		}
		
		pVoIPCfg->MINI_SessionExpire = atoi(boaGetVar(wp, "minSessionExpiry", ""));		
		//codec
		asp_e8c_codec_set(wp, pCfg);

		//heart beat cycle
		pCfg->proxies[0].HeartbeatCycle = atoi(boaGetVar(wp, "HeartbeatCycle", ""));
		pCfg->proxies[1].HeartbeatCycle = atoi(boaGetVar(wp, "HeartbeatCycle", ""));
		if(pCfg->proxies[0].HeartbeatCycle==0)//HeartbeatCycle=0:disable
		{
			pCfg->proxies[0].enable &= ~PROXY_OPTIONS_PING;
			pCfg->proxies[1].enable &= ~PROXY_OPTIONS_PING;
		}else{
			pCfg->proxies[0].enable |= PROXY_OPTIONS_PING;
			pCfg->proxies[1].enable |= PROXY_OPTIONS_PING;
		}
		//retry reg
		pCfg->proxies[0].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));
		pCfg->proxies[1].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));

		i = atoi(boaGetVar(wp, "spk_voice_gain", ""));
		if ((i>= -32) && (i<=31) )
			pCfg->spk_voice_gain=i;
		else
			pCfg->spk_voice_gain=0;

		i = atoi(boaGetVar(wp, "mic_voice_gain", ""));
		if ((i>= -32) && (i<=31) )
			pCfg->mic_voice_gain=i;
		else
			pCfg->mic_voice_gain=0;

		
		if(!gstrcmp(boaGetVar(wp, "3_way", ""), "on")){
			pCfg->fxsport_flag &= ~FXS_3WAY_DISABLE;
		}else
			pCfg->fxsport_flag |= FXS_3WAY_DISABLE;
		
		pCfg->call_waiting_enable = !gstrcmp(boaGetVar(wp, "call_waiting", ""), "on");

		pCfg->hotline_enable=atoi(boaGetVar(wp, "hot_line_options", "0"));
		strcpy((char *) pCfg->hotline_number, boaGetVar(wp, "hotline_number", ""));
     		
		//JitterBufferEnable
        if( strcmp("enable", boaGetVar(wp, "JitterBufferEnable", "")) == 0)
            pCfg->jitter_factor = 1;
        else
            pCfg->jitter_factor = 0;

		//min jitter delay
		pCfg->jitter_delay = atoi(boaGetVar(wp, "jitterDelay", "4"));
            
		//DTMF RFC2833 payload
		pCfg->dtmf_2833_pt = atoi(boaGetVar(wp, "dtmf_2833_pt", "96"));

        //SIP register expire...
		pCfg->proxies[0].reg_expire = atoi(boaGetVar(wp, "sip_reg_expire", "3600"));
	}

	printf("SUBS_options=%d\n", atoi(boaGetVar(wp, "SUBS_options", "")));
	if(atoi(boaGetVar(wp, "SUBS_options", ""))==3)
	{
		pCfg->proxies[0].enable &= ~PROXY_SUBSCRIBE; 
		pCfg->proxies[1].enable &= ~PROXY_SUBSCRIBE;
		pVoIPCfg->subs_type=SUBS_TYPE_ALL;
	}
	else
	{
		pCfg->proxies[0].enable |= PROXY_SUBSCRIBE; 
		pCfg->proxies[1].enable |= PROXY_SUBSCRIBE; 
		pVoIPCfg->subs_type=atoi(boaGetVar(wp, "SUBS_options", ""));
	}
	pVoIPCfg->subs_time=atoi(boaGetVar(wp, "SUBS_time", "0"));
	
	pVoIPCfg->rfc_flags &= ~SIP_FAX_AUTO;
	if (atoi(boaGetVar(wp, "FAX_options", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_FAX_AUTO;

	pVoIPCfg->rfc_flags &= ~SIP_DIGIT_MAP_MODE_MIN;
	if (atoi(boaGetVar(wp, "digit_map_mode", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_DIGIT_MAP_MODE_MIN;
	
	pVoIPCfg->rfc_flags &= ~SIP_DATE;
	if(!gstrcmp(boaGetVar(wp, "sip_date", ""), "on")){
		pVoIPCfg->rfc_flags |= SIP_DATE;
	}
	
	pVoIPCfg->rfc_flags &= ~SIP_SESSION_TIMER_USE_INVITE;
	if (atoi(boaGetVar(wp, "session_options", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_SESSION_TIMER_USE_INVITE;

	if(!gstrcmp(boaGetVar(wp, "reverse_polariy", ""), "on")){
		pVoIPCfg->rfc_flags &= ~FXS_REVERSE_POLARIY_OFF;
	}else
		pVoIPCfg->rfc_flags |= FXS_REVERSE_POLARIY_OFF;
	
    if(!gstrcmp(boaGetVar(wp, "pound_to_ascii", ""), "on")){
        pVoIPCfg->rfc_flags |= SIP_ENABLE_POUND_KEY_TO_ASCII;
	}else{
		pVoIPCfg->rfc_flags &= ~SIP_ENABLE_POUND_KEY_TO_ASCII;
    }

	pVoIPCfg->off_hook_alarm=atoi(boaGetVar(wp, "off_hook_alarm", ""));
	pVoIPCfg->auto_dial=atoi(boaGetVar(wp, "auto_dial", ""));
	pVoIPCfg->InterDigitTimerLong=atoi(boaGetVar(wp, "InterDigitTimerLong", ""));
	
	pVoIPCfg->BusyToneTimer=atoi(boaGetVar(wp, "BusyToneTimer", ""));
	pVoIPCfg->HangingReminderToneTimer=atoi(boaGetVar(wp, "HangingReminderToneTimer", ""));
	pVoIPCfg->NoAnswerTimer=atoi(boaGetVar(wp, "NoAnswerTimer", ""));

    pVoIPCfg->rtpDscp = atoi(boaGetVar(wp, "rtpDscp", "0"));
	pVoIPCfg->sipDscp = atoi(boaGetVar(wp, "sipDscp", "0"));

	pVoIPCfg->ringVoltageFreq =atoi(boaGetVar(wp, "RingVoltage", ""));
	
    //heartbeat mode
    ptr_index = atoi (boaGetVar(wp, "heartbeat_mode", ""));
	pVoIPCfg->heartbeat_mode = ptr_index;

 


    //NtpEnable
    {
        unsigned char ntpEnabled;

        if (strcmp("enable", boaGetVar(wp, "NtpEnable", "")) == 0)
            ntpEnabled = 1;
        else
            ntpEnabled = 0;
       
        mib_set(MIB_NTP_ENABLED, &ntpEnabled);     
    }
    	
	RCM_DBG;
	
	syslog(LOG_CRIT,"VoIP advanced setting.");

	web_flash_set(pVoIPCfg);


	sprintf(redirect_url, "/app_voip2.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

void asp_voip_e8c_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	char szPrecedence[12];
	char redirect_url[50];
	int voip_port;
	char szName[64];
	char *ptr;
	int ptr_index=0;
	int ori_serverType, serverType_change=0;
	
	printf("---Enter %s---\n", __FUNCTION__);
	if (web_flash_get(&pVoIPCfg) != 0)
		return;

	RCM_DBG;
	
	//E8C support SIP REDUNDANCY 
	pVoIPCfg->rfc_flags |=SIP_SERVER_REDUNDANCY;
	ori_serverType = pVoIPCfg->X_CT_servertype;
	ptr_index = atoi (boaGetVar(wp, "servertype", ""));


#ifdef CONFIG_USER_RTK_H248
    if (ptr_index != ori_serverType)
	{
        if (SERVER_H248 == ori_serverType) 
		{
			serverType_change = 2; //248 to sip
        }
        else if (ptr_index == SERVER_H248) 
		{
			serverType_change = 1; //sip to 248
        }
    }
#endif
    pVoIPCfg->X_CT_servertype = ptr_index;


	
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		

		
		for (i=0; i< MAX_PROXY; i++)
		{

			//proxy addr
			sprintf(szName, "proxy%d_addr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].addr, boaGetVar(wp, szName, ""));
			RCM_DBG;
			
			//proxy port
			sprintf(szName, "proxy%d_port", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].port = atoi(boaGetVar(wp, szName, "5060"));
			RCM_DBG;

			//register proxy addr
			sprintf(szName, "proxy%d_register_addr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].RegistrarServer_addr, boaGetVar(wp, szName, ""));
			RCM_DBG;
			
			//register proxy port
			sprintf(szName, "proxy%d_register_port", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].RegistrarServer_port = atoi(boaGetVar(wp, szName, "5060"));
			RCM_DBG;
			
			//outbound proxy enable/disable
			sprintf(szName, "proxy%d_obEnable", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].outbound_enable = !gstrcmp(boaGetVar(wp, szName, ""), "enable");
			RCM_DBG;
			
			//outbound proxy addr
			sprintf(szName, "proxy%d_obAddr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].outbound_addr, boaGetVar(wp, szName, ""));
			RCM_DBG;
			
			//outbound proxy port
			sprintf(szName, "proxy%d_obPort", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].outbound_port = atoi(boaGetVar(wp, szName, "5060"));

			//domain_name
			sprintf(szName, "proxy%d_domain_name", i);
			strcpy(pCfg->proxies[i].domain_name, boaGetVar(wp, szName, ""));


			//reg_expire
			sprintf(szName, "proxy%d_reg_expire", i);
			pCfg->proxies[i].reg_expire = atoi(boaGetVar(wp, szName, "60"));
		
			//sessionEnable
			sprintf(szName, "proxy%d_sessionEnable", i);
			if( strcmp("enable", boaGetVar(wp, szName, "")) == 0){
				//sessionExpiry
				sprintf(szName, "proxy%d_sessionExpiry", i);
				pCfg->proxies[i].SessionUpdateTimer = atoi(boaGetVar(wp, szName, "90"));

			}else{
				pCfg->proxies[i].SessionUpdateTimer =0;
			}
			
		}

//SIP
		//account_enable [main proxy enable]
		sprintf(szName, "port%d_account_enable", voip_port+1);
		pCfg->proxies[0].enable = 0;//Reset setting
		pCfg->proxies[1].enable = 0;//Reset setting
		if( strcmp("enable", boaGetVar(wp, "proxy0_subscribe_enable", "")) == 0)
			pCfg->proxies[0].enable |= PROXY_SUBSCRIBE; 
		if( strcmp("enable", boaGetVar(wp, "proxy1_subscribe_enable", "")) == 0)
			pCfg->proxies[1].enable |= PROXY_SUBSCRIBE;
		if( strcmp("enable", boaGetVar(wp, szName, "")) == 0)
		{
			pCfg->proxies[0].enable |= PROXY_ENABLED;


			
			//second proxy enable
			if( strcmp("enable", boaGetVar(wp, "proxy1_enable", "")) == 0)
			{
				pCfg->proxies[1].enable |= PROXY_ENABLED;

			}
		}

		//number
		sprintf(szName, "port%d_number", voip_port+1);
		strcpy(pCfg->proxies[0].number, boaGetVar(wp, szName, ""));
		strcpy(pCfg->proxies[1].number, boaGetVar(wp, szName, ""));
		
		//display_name:e8c display_name same as number
		strcpy(pCfg->proxies[0].display_name, boaGetVar(wp, szName, ""));
		strcpy(pCfg->proxies[1].display_name, boaGetVar(wp, szName, ""));
		
		//login_id
		sprintf(szName, "port%d_login_id", voip_port+1);
		strcpy(pCfg->proxies[0].login_id, boaGetVar(wp, szName, ""));
		strcpy(pCfg->proxies[1].login_id, boaGetVar(wp, szName, ""));
		
		//password
		sprintf(szName, "port%d_password", voip_port+1);

		if (strcmp(boaGetVar(wp, szName, ""), PWD_DISPLAY)!=0)
		{
		strcpy(pCfg->proxies[0].password, boaGetVar(wp, szName, ""));
		strcpy(pCfg->proxies[1].password, boaGetVar(wp, szName, ""));
		}

//H248
#ifdef CONFIG_USER_RTK_H248
		sprintf(szName, "h248_port%d_account_enable", voip_port+1);
		if (pVoIPCfg->X_CT_servertype==SERVER_H248)
		{
			if( strcmp("enable", boaGetVar(wp, szName, "")) == 0)
			{
				fprintf(stderr , "%s(%d) h248 enable !!\n" , __FUNCTION__ , __LINE__ );
				pCfg->proxies[0].enable |= PROXY_ENABLED;
			}
			else
			{
				fprintf(stderr , "%s(%d) h248 disable !!\n" , __FUNCTION__ , __LINE__ );
				pCfg->proxies[0].enable &= ~PROXY_ENABLED;
			}
		}
		
        //h248_port
        pVoIPCfg->mg_port = atoi(boaGetVar(wp, "h248_port", ""));
		
        //h248_mgc0_address
        strcpy(pVoIPCfg->mgc_ip, boaGetVar(wp, "h248_mgc0_address", ""));

        //h248_mgc0_port
        pVoIPCfg->mgc_port = atoi(boaGetVar(wp, "h248_mgc0_port", ""));
       
        //h248_mgc1_address
        strcpy(pVoIPCfg->bak_mgc_ip, boaGetVar(wp, "h248_mgc1_address", ""));

        //h248_mgc1_port
        pVoIPCfg->bak_mgc_port = atoi(boaGetVar(wp, "h248_mgc1_port", ""));

        //h248_termination_id
        strcpy(pVoIPCfg->mg_name, boaGetVar(wp, "h248_termination_id", ""));

        //h248_domain_name
        strcpy(pVoIPCfg->Domain, boaGetVar(wp, "h248_domain_name", ""));

        //h248_phy_term_prefix
        strcpy(pVoIPCfg->mg_prefix, boaGetVar(wp, "h248_phy_term_prefix", ""));

        //h248_phy_term_start
        pVoIPCfg->mg_phyTermPrefixStart = atoi(boaGetVar(wp, "h248_phy_term_start", ""));

        //h248_phy_term_len
        pVoIPCfg->mg_phyTermPrefixLen = atoi(boaGetVar(wp, "h248_phy_term_len", ""));

        //h248_rtp_term_prefix
        strcpy(pVoIPCfg->mg_rtp_prefix, boaGetVar(wp, "h248_rtp_term_prefix", ""));

        //h248_rtp_term_start
        pVoIPCfg->mg_rtpPrefixStart = atoi(boaGetVar(wp, "h248_rtp_term_start", ""));

        //h248_rtp_term_len
        pVoIPCfg->mg_rtpPrefixLen = atoi(boaGetVar(wp, "h248_rtp_term_len", ""));

        //h248_id_type
		ptr_index = atoi (boaGetVar(wp, "h248_id_type", ""));
		pVoIPCfg->mg_deviceIdType = ptr_index;

        //h248_encoding_type
        ptr_index = atoi (boaGetVar(wp, "h248_encoding_type", ""));
        strcpy(pVoIPCfg->mg_encoding, encoding_type[ptr_index]);
        
        //h248_phy_term_method
        ptr_index = atoi (boaGetVar(wp, "h248_phy_term_config_method", ""));
		pVoIPCfg->mg_phyTermConfigMethod = ptr_index;
		
		//h248_phy_term_id
        strcpy(pVoIPCfg->mg_phyTermID, boaGetVar(wp, "h248_phy_term_id", ""));    
#endif
	}
	RCM_DBG;

	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif	 
	sprintf(redirect_url, "/app_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else

    if (1 == serverType_change) // sip to 248
	{
		printf("===> sip to 248\n");
		system("sh /etc/scripts/SIPtoh248.sh");
    }
	else if (2 == serverType_change) // 248 to sip
	{
		printf("===> 248 to sip\n");
		system("sh /etc/scripts/h248toSIP.sh");
    }
	else //server type not change
	{
		printf("===> server not change\n");
	    web_restart_solar();
	}

	boaRedirect(wp, redirect_url);
#endif
}

void asp_debug_e8c_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	char redirect_url[50];

	printf("---Enter %s---\n", __FUNCTION__);
	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	RCM_DBG;

	pVoIPCfg->rfc_flags &= ~SIP_DEBUG;
	if(!gstrcmp(boaGetVar(wp, "sip_debug", ""), "on")){
		pVoIPCfg->rfc_flags |= SIP_DEBUG;
	}

	pVoIPCfg->ringVoltageFreq =atoi(boaGetVar(wp, "RingVoltage", ""));

	RCM_DBG;

	web_flash_set(pVoIPCfg);
	sprintf(redirect_url, "/bd/vermod.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}


char called_number[64]= {""};
int autoCallTestType = 2;
TstVoipSLICLineTest_rdloop rdloop;
TstVoipSLICLineTest_ren ren;
int lineconnected=0;

int asp_diag_voip_e8c_get(int ejid, request * wp, int argc, char **argv)
{
	voipCfgParam_t *pVoIPCfg;
    voip_state_share_t  *ppSts = NULL;
	float divider = 1000;

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;
	
	voip_line_state_share_get(&ppSts);
#if 0
	fprintf(stderr ,"vAcDiff = %d\n", voltage.vAcDiff);
	fprintf(stderr ,"vAcRing = %d\n", voltage.vAcRing);
	fprintf(stderr ,"vAcTip = %d\n", voltage.vAcTip);
	fprintf(stderr ,"vDcDiff = %d\n", voltage.vDcDiff);
	fprintf(stderr ,"vDcRing = %d\n", voltage.vDcRing);
	fprintf(stderr ,"vDcTip = %d\n", voltage.vDcTip);		
	fprintf(stderr ,"fltMask = 0x[%x]\n", voltage.fltMask);
	fprintf(stderr ,"measStatus = 0x[%x]\n", voltage.measStatus);
	
	fprintf(stderr ,"rGnd = %d\n", flt.rGnd);
	fprintf(stderr ,"rrg = %d\n", flt.rrg);
	fprintf(stderr ,"rtg = %d\n", flt.rtg);
	fprintf(stderr ,"rtr = %d\n", flt.rtr);
	fprintf(stderr ,"fltMask = 0x[%x]\n", flt.fltMask);
	fprintf(stderr ,"measStatus = 0x[%x]\n", flt.measStatus);
#endif	


	
	if (strcmp(argv[0], "called_number")==0)
	{
		boaWrite(wp, "%s", called_number);
	}
	else if  (strcmp(argv[0], "voltage")==0)
	{
		boaWrite(wp, "%f V", rdloop.vab / divider);
	}
	else if  (strcmp(argv[0], "ring_impedance")==0)
	{
		boaWrite(wp, "%f ren", (ren.ren/divider));
	}
	else if  (strcmp(argv[0], "auto_call_state")==0)
	{
		FILE *fh;
		char buf[8];
		int i=0;
		
		fh = fopen(_PATH_AUTO_CALL_STATUS, "r");
		if (!fh) {
			boaWrite(wp, "<td id=\"auto_call_state\"></td>");
			return 0;
		}

		memset(buf, 0, sizeof(buf));
		fscanf(fh,"%s",buf);		

		for (i=0; i<strlen(buf); i++)
		{
			int test_state = buf[i] - '0'; 

			//printf("buf[%d]=%c, test_state:%d\n", i, buf[i], test_state);
			switch (test_state) {
			case YUEME_TEST_IDLE:
				boaWrite(wp, "<td></td>");
				break;
			case YUEME_TEST_OFFHOOK:
				boaWrite(wp, "<tr><td>主叫通话测试摘机</td></tr>");
				break;
			case YUEME_TEST_DIAL_TONE:
				boaWrite(wp, "<tr><td>主叫通话测试放拨号音</td></tr>");
				break;
			case YUEME_TEST_RING_TONE:
				boaWrite(wp, "<tr><td>主叫通话测试听回铃音</td></tr>");
				break;
			case YUEME_TEST_CONNECT:
				boaWrite(wp, "<tr><td>主叫通话测试成功通话</td></tr>");
				break;
			case YUEME_TEST_CALL_FAIL:
				boaWrite(wp, "<tr><td>主叫通话测试通话失败</td></tr>");
				boaWrite(wp, "<tr><td id=\"auto_call_state\"></td></tr>");
				break;
			case YUEME_TEST_FINISH:
			case YUEME_TEST_ERROR:
			default:
				boaWrite(wp, "<tr><td id=\"auto_call_state\"></td></tr>");
				break;
			}	
		}
		fclose(fh);
	}
	else if  (strcmp(argv[0], "register_accept")==0)
	{
		FILE *fh;
		char buf[MAX_VOIP_PORTS * MAX_PROXY];
		
		fh = fopen(_PATH_TMP_STATUS, "r");
		if (!fh) {
			boaWrite(wp, "%s", "disabled=\"true\" value=\"语音端口未注册\"");
			return 0;
		}

		memset(buf, 0, sizeof(buf));
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			boaWrite(wp, "%s", "disabled=\"true\" value=\"语音端口未注册\"");
		}
		else {

			/* SD6, bohungwu, fix incorrect registration status for the secondary proxy */
			char p0_reg_st, p1_reg_st, p_reg_st;
			if((pVoIPCfg->ports[0].proxies[0].enable & PROXY_ENABLED)==0)
			{
				boaWrite(wp, "%s", "disabled=\"true\" value=\"语音端口未注册\"");	
			}
			else
			{
			
				p0_reg_st = buf[0 * MAX_PROXY];
				p1_reg_st = buf[0 * MAX_PROXY + 1];
				if((p0_reg_st == '1')||(p0_reg_st == '2')||(p0_reg_st == '4')||(p0_reg_st == '5')){
					p_reg_st = p0_reg_st;
				}
				else{
					if((pVoIPCfg->ports[0].proxies[1].enable & PROXY_ENABLED)&&(p1_reg_st != '0'))
					{
						p_reg_st = p1_reg_st;
					}
					else//sencond proxy disable, use main proxy status
					{
						p_reg_st = p0_reg_st;
					}
				}
				//fprintf(stderr, "buf is %s.\n", buf);
				switch (p_reg_st) {
				case '1':
					//boaWrite(wp, "<td>%s</td>", "Registered");
					boaWrite(wp, "%s", "value=\"模拟主叫通话测试\"");
					break;
				case '0':
				case '2':
				case '3':
				case '4':
				case '5':
				default:
					boaWrite(wp, "%s", "disabled=\"true\" value=\"语音端口未注册\"");
					break;
				}
			}
		}

		fclose(fh);
	}else if (strcmp(argv[0], "autoCallTestSelector")==0){
		boaWrite(wp,
			"<option value=0 %s>PhoneConnectivityTest</option>"
#ifdef CONFIG_CU
			"<option value=1 %s>X_CU_SimulateTest</option>"			
#elif defined(CONFIG_CMCC)
			"<option value=1 %s>X_CMCC_SimulateTest</option>"
#endif
			,ppSts->e8c_autocall_test[0].TestSelector == 0 ? "selected" : ""
			,ppSts->e8c_autocall_test[0].TestSelector == 1 ? "selected" : ""
		);
	}else if (strcmp(argv[0], "autoCallTestType")==0){
		boaWrite(wp,
			"<option value=0 %s>Caller</option>"
			"<option value=1 %s>Called</option>"
			"<option value=2 %s>None</option>"
			,autoCallTestType == 0 ? "selected" : ""
			,autoCallTestType == 1 ? "selected" : ""
			,autoCallTestType == 2 ? "selected" : ""
		);
	}else if (strcmp(argv[0], "autoCallCallednumber")==0){
		boaWrite(wp, "%s", ppSts->e8c_autocall_test[0].callednumber);
	}else if (strcmp(argv[0], "autoCallDailDTMFConfirmEnable")==0){
		boaWrite(wp, "%s", (ppSts->e8c_autocall_test[0].DailDTMFConfirmEnable) ? "checked" : "");
	}else if (strcmp(argv[0], "autoCallDailDTMFConfirmNumber")==0){
		boaWrite(wp, "%s", ppSts->e8c_autocall_test[0].DailDTMFConfirmNumber);
	}else if (strcmp(argv[0], "autoCallTestStatus")==0){
		if (ppSts->e8c_autocall_test[0].TestStatus==0)
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallTestStatus\">%s</td>", "None");
		else if (ppSts->e8c_autocall_test[0].TestStatus==1)
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallTestStatus\">%s</td>", "Requested");
		else if (ppSts->e8c_autocall_test[0].TestStatus==2)
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallTestStatus\">%s</td>", "Complete");
	}else if (strcmp(argv[0], "phoneConnectivity")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==0)){
			if (ppSts->e8c_autocall_test[0].PhoneConnectivity==0)
				boaWrite(wp, "<td width=\"200px\" id=\"phoneConnectivity\">%s</td>", "0 线路未接");
			else if (ppSts->e8c_autocall_test[0].PhoneConnectivity==1)
				boaWrite(wp, "<td width=\"200px\" id=\"phoneConnectivity\">%s</td>", "1 线路已接");
		}else
			boaWrite(wp, "<td width=\"200px\" id=\"phoneConnectivity\">%s</td>", "");
	}else if (strcmp(argv[0], "autoCallDailDTMFConfirmResult")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==1)&&(ppSts->e8c_autocall_test[0].DailDTMFConfirmEnable==1)){
			if (ppSts->e8c_autocall_test[0].DailDTMFConfirmResult==0)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallDailDTMFConfirmResult\">%s</td>", "0 正确");
			else if (ppSts->e8c_autocall_test[0].DailDTMFConfirmResult==1)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallDailDTMFConfirmResult\">%s</td>", "1 错误");
		}else
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallDailDTMFConfirmResult\">%s</td>", "");
	}else if (strcmp(argv[0], "autoCallSimulateStatus")==0){	
		boaWrite(wp, "<td width=\"200px\" id=\"autoCallSimulateStatus\">%s</td>", ppSts->e8c_autocall_test[0].Simulate_Status);
	}else if (strcmp(argv[0], "autoCallConclusion")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==1)){
			if (ppSts->e8c_autocall_test[0].Conclusion==0)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallConclusion\">%s</td>", "Success");
			else if (ppSts->e8c_autocall_test[0].Conclusion==1)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallConclusion\">%s</td>", "Fail");
		}else
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallConclusion\">%s</td>", "");
	}else if (strcmp(argv[0], "autoCallCallerFailReason")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==1)){
			if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_NONE)//CALLER_NONE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "Error None");
			else if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_NO_DIALTONE)//CALLER_NO_DIALTONE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "NoDialTone");
			else if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_OFFHOOK_RELEASE)//CALLER_OFFHOOK_RELEASE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "OffhookRelease");
			else if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_AFTERDIAL_RELEASE)//CALLER_AFTERDIAL_RELEASE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "AfterDialRelease");
			else if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_NO_ANSWER)//CALLER_NO_ANSWER
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "NoAnswer");
			else if (ppSts->e8c_autocall_test[0].CallerFailReason==CALLER_KEY_ERROR)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "key error");			
		}else
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallCallerFailReason\">%s</td>", "");
	}else if (strcmp(argv[0], "autoCallCalledFailReason")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==1)){
			if (ppSts->e8c_autocall_test[0].CalledFailReason==CALLER_NONE)//CALLER_NONE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "Error None");
			else if (ppSts->e8c_autocall_test[0].CalledFailReason==CALLEE_NO_INCOMINGCALL)//CALLEE_NO_INCOMINGCALL
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "NoIncomingCall");
			else if (ppSts->e8c_autocall_test[0].CalledFailReason==CALLEE_OFFHOOK_RELEASE)//CALLEE_OFFHOOK_RELEASE
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "OffhookRelease");
			else if (ppSts->e8c_autocall_test[0].CalledFailReason==CALLEE_NO_ANSWER)//CALLEE_NO_ANSWER
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "NoAnswer");
			else if (ppSts->e8c_autocall_test[0].CalledFailReason==CALLEE_KEY_ERROR)
				boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "key error");	
		}else	
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallCalledFailReason\">%s</td>", "");
	}else if (strcmp(argv[0], "autoCallFailedResponseCode")==0){
		if ((ppSts->e8c_autocall_test[0].TestStatus==2)&&(ppSts->e8c_autocall_test[0].TestSelector==1)){
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallFailedResponseCode\">%d</td>", ppSts->e8c_autocall_test[0].FailedResponseCode);
		}else
			boaWrite(wp, "<td width=\"200px\" id=\"autoCallFailedResponseCode\">%s</td>", "");
	}
	else
	{

		return -1;
	}

	return 0;
}

void asp_diag_voip_e8c_set(request * wp, char *path, char *query)
{
	char redirect_url[50];
	printf("---Enter %s---\n", __FUNCTION__);


	RCM_DBG;
	memset(&ren, 0, sizeof(TstVoipSLICLineTest_ren));
	memset(&rdloop, 0, sizeof(TstVoipSLICLineTest_rdloop));
	
	rtk_DoLineTest_rdloopcond(0, &rdloop);
	rtk_DoLineTest_REN(0, &ren);
	lineconnected=rtk_GetPortStatus(0);
	RCM_DBG;




	sprintf(redirect_url, "/diag_voip.asp");

#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	//web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}
void asp_diag_call_set(request * wp, char *path, char *query)
{
	char redirect_url[50];
	char cmd[64];
	FILE *fh;
	char buf[8];

	printf("---Enter %s---\n", __FUNCTION__);
	RCM_DBG;
	
	fh = fopen(_PATH_AUTO_CALL_STATUS, "w");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_AUTO_CALL_STATUS);
		printf("\nerrno=%d\n", errno);
	}
	else {
		printf("diag_call start...\n");
		memset(buf, 0, sizeof(buf));
		fprintf(fh, "%d", YUEME_TEST_IDLE);
		fclose(fh);
	}
	
	memset(called_number, 0 , sizeof(called_number));
	strcpy(called_number, boaGetVar(wp, "called_number", ""));

	sprintf(cmd ,"echo %s > /var/run/solar_control.fifo",called_number);

	fprintf(stderr ,"cmd=%s\n", cmd);
	system(cmd);
	
	RCM_DBG;




	sprintf(redirect_url, "/diag_voip.asp");

#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	//web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

void asp_diag_e8c_auto_call_set(request * wp, char *path, char *query)
{
	char redirect_url[50];
    	voip_state_share_t  *ppSts = NULL;
	//printf("---Enter %s---\n", __FUNCTION__);
		


	voip_line_state_share_get(&ppSts);
	RCM_DBG;
	if (ppSts->e8c_autocall_test[0].TestStatus==1)//requested
		return;
	ppSts->e8c_autocall_test[0].TestSelector =atoi(boaGetVar(wp, "autoCallTestSelector", ""));
	

	//ppSts->e8c_autocall_test[0].enable=1;

	//ppSts->e8c_autocall_test[0].TestStatus=1;

	ppSts->e8c_autocall_test[0].DailDTMFConfirmEnable=!gstrcmp(boaGetVar(wp, "autoCallDailDTMFConfirmEnable", ""), "on");
	autoCallTestType=atoi(boaGetVar(wp, "autoCallTestType", ""));
	if (autoCallTestType!=2)// 2:None
	{
		ppSts->e8c_autocall_test[0].TestType=autoCallTestType;
		ppSts->e8c_autocall_test[0].enable=1;
		ppSts->e8c_autocall_test[0].TestStatus=1;
		ppSts->e8c_autocall_test[0].testFrom=TEST_FROM_WEB;//from web
	}
	//ppSts->e8c_autocall_test[0].TestType=atoi(boaGetVar(wp, "autoCallTestType", ""));
	strncpy(ppSts->e8c_autocall_test[0].callednumber,boaGetVar(wp, "autoCallCallednumber", ""),24);
	strncpy(ppSts->e8c_autocall_test[0].DailDTMFConfirmNumber,boaGetVar(wp, "autoCallDailDTMFConfirmNumber", ""),24);

	strcpy(ppSts->e8c_autocall_test[0].Simulate_Status,"");//reset Simulate_Status
	RCM_DBG;

	syslog(LOG_CRIT,"VoIP diagnistic\n");

	sprintf(redirect_url, "/auto_call_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	boaRedirect(wp, redirect_url);
#endif
}

int asp_voip_flags_get(int ejid, request *wp, int argc, char **argv)
{
    voipCfgParam_t *pVoIPCfg;
	char rfc_flags[16];
	int i =0;
	
    if (web_flash_get(&pVoIPCfg) != 0) {
        printf("get flash fail!\n", __func__);
		return -1;
    }

    RCM_DBG;

	for (i=0;i<RFC_FLAGS_NUMBER_MAX;i++)
	{
		sprintf(rfc_flags,"rfc_flags_%d",i);
		
		if (strcmp(argv[0], rfc_flags) == 0)    
		{
			boaWrite(wp, "%s", (pVoIPCfg->rfc_flags & (1<<i)) ? "checked" : "");
			return 0;
		}
	}

	printf("%s: get voip flags fail! %s \n", __func__);
	return -1;
}   

void asp_voip_flags_set(request *wp, char *path, char *query)
{
    voipCfgParam_t *pVoIPCfg;
	char rfc_flags[16];
	int i =0;

    if (web_flash_get(&pVoIPCfg) != 0) {
        printf("get flash fail!\n", __func__);
		return;
    }

    RCM_DBG;

	for (i=0;i<RFC_FLAGS_NUMBER_MAX;i++)
	{
		sprintf(rfc_flags,"rfc_flags_%d",i);
		
		if(!gstrcmp(boaGetVar(wp, rfc_flags, ""), "on"))    //it is a disable flag		
			pVoIPCfg->rfc_flags |= (1<<i);
		else
			pVoIPCfg->rfc_flags &= ~(1<<i);		
	}
    RCM_DBG;

    web_flash_set(pVoIPCfg);

#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, "/voip_flags.asp");
#endif
}

void asp_cmcc_maxDelay(request *wp, int nMaxDelay)
{
	int i;

	if( nMaxDelay < 13 )
		nMaxDelay = 13;
	else if( nMaxDelay > 60 )
		nMaxDelay = 60;

	for(i=13; i<=60; i++)
	{
		if (nMaxDelay == i)
			boaWrite(wp, "<option value=%d selected>%d</option>", i, i*10);
		else
			boaWrite(wp, "<option value=%d >%d</option>", i, i*10);
	}
}

void asp_cmcc_codec_get(request * wp, voipCfgPortParam_t *pCfg)
{
	int i, j, k;
	int e8c_codec_preced[E8C_CODEC_NUM] = {0}; //For trivial sorting
	int enable_codec[E8C_CODEC_NUM]= {0};
	//Find out E8C codec index from the codec set

	for (i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		for(k=0; k<E8C_CODEC_NUM; k++)
		{
			if( (strcmp(e8c_supported_codec_string[i], e8c_codec_str[k]) == 0) )
			{
				e8c_codec_idx[k]=i;
				e8c_codec_preced[k] = pCfg->precedence[i];
				break;
			}
		}
	}

	//Sorting
	for(i=0; i<E8C_CODEC_NUM; i++)
	{
		int smallest = INT32_MAX;
		int s_idx = -1;
		for(j=0; j<E8C_CODEC_NUM; j++)
		{
			if(smallest > e8c_codec_preced[j])
			{
				s_idx = j;
				smallest = e8c_codec_preced[j];
			}
		}
		if(s_idx != -1)
		{
			rank[i] = e8c_codec_idx[s_idx];
			e8c_codec_preced[s_idx] = INT32_MAX;
		}
	}

	for (i=0; i<E8C_CODEC_NUM; i++)
	{

		boaWrite(wp, "<tr><td width=200px>%s</td>\n", cmcc_style_codec_str[i]);
		boaWrite(wp, "<td><input type=\"checkbox\" name=\"codec_%d_enable\" onClick=\"checkbox_gray()\" size=20 %s>\n", i, (!(255 == pCfg->precedence[e8c_codec_idx[i]]) ? "checked" : "" ));
		boaWrite(wp, "<td><select name=frame_size%d>\n", i);
		for (j=0; j<3; j++)
			boaWrite(wp,
				"<option %s value=%d>%d %s</option>",
				j == pCfg->frame_size[e8c_codec_idx[i]] ? "selected" : "",
				j,
				10 * (j + 1),
				"ms"
				);
		boaWrite(wp, "</select></td>\n");
		boaWrite(wp, "<td><select name=preced%d>\n", i);

		for (j=0; j<E8C_CODEC_NUM; j++)
		{
			if(e8c_codec_idx[i] == rank[j])
			{
				boaWrite(wp, "<option value=%d selected>%d", j+1, j+1);
			}
			else
			{
				boaWrite(wp, "<option value=%d>%d", j+1, j+1);
			}
		}
		boaWrite(wp, "</select></td></tr>\n");
	}
}

void asp_voip_cmcc_codec_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	int voip_port;
	char szName[64];
	char redirect_url[50];


	if (web_flash_get(&pVoIPCfg) != 0)
		return;

	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		
		for (i=0; i<E8C_CODEC_NUM; i++)
		{
			sprintf(szName, "codec_%d_enable", i);
			if (gstrcmp(boaGetVar(wp, szName, ""), "on"))
			{
				fprintf(stderr, "---Enter %s---codec_%d_enable off \n", __FUNCTION__, i);
				 pCfg->precedence[e8c_codec_idx[i]]=255;
			}else
			{
				sprintf(szName, "preced%d", i);
				pCfg->precedence[e8c_codec_idx[i]]=atoi(boaGetVar(wp, szName, ""))-1;
			}
			sprintf(szName, "frame_size%d", i);
			pCfg->frame_size[e8c_codec_idx[i]]=atoi(boaGetVar(wp, szName, ""));
		}
	}
	
	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif

	sprintf(redirect_url, "/cmcc_app_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

void asp_voip_cmcc_account_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	int voip_port;
	char szName[64];
	char redirect_url[50];


	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		for (i=0; i< MAX_PROXY; i++)
		{
			//if (!(pCfg->proxies[i].enable & PROXY_ENABLED)){
				//number
				sprintf(szName, "account_number");
				strcpy(pCfg->proxies[i].number, boaGetVar(wp, szName, ""));
				strcpy(pCfg->proxies[i].display_name, boaGetVar(wp, szName, ""));
				
				//login_id
				sprintf(szName, "account_login_id");
				strcpy(pCfg->proxies[i].login_id, boaGetVar(wp, szName, ""));

				//password
				sprintf(szName, "account_password");

				strcpy(pCfg->proxies[i].password, boaGetVar(wp, szName, ""));
				
				pCfg->proxies[i].enable |= PROXY_ENABLED;
			//}

		}
	}
	syslog(LOG_CRIT,"VoIP add account.");
	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif

	sprintf(redirect_url, "/cmcc_app_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}
void asp_voip_cmcc_account_del(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	int voip_port;
	char szName[64];
	char redirect_url[50];


	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		for (i=0; i< MAX_PROXY; i++)
		{
			//sprintf(szName, "port%d_account%d_del", voip_port+1, i+1 );
			sprintf(szName, "port%d_account%d_del", voip_port+1, 1 );//current only use account 1
			if( strcmp("enable", boaGetVar(wp, szName, "")) == 0){
				printf("asp_voip_cmcc_account_del del %d %d", voip_port, i);
				pCfg->proxies[i].enable &= ~PROXY_ENABLED;
				strcpy(pCfg->proxies[i].number, "");
				strcpy(pCfg->proxies[i].display_name,"");
				strcpy(pCfg->proxies[i].login_id, "");
				strcpy(pCfg->proxies[i].password, "");
			}
		}
	}
	syslog(LOG_CRIT,"VoIP delete account.");
	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif

	sprintf(redirect_url, "/cmcc_app_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

void asp_voip_cmcc_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	char redirect_url[50];
	int voip_port;
	char szName[64];
    char *ptr;
	int ori_serverType, serverType_change=0;

	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	RCM_DBG;

	
	//E8C support SIP REDUNDANCY 
	pVoIPCfg->rfc_flags |=SIP_SERVER_REDUNDANCY;
		
	ori_serverType = pVoIPCfg->X_CT_servertype;

	i = atoi(boaGetVar(wp, "servertype", ""));

#ifdef CONFIG_USER_RTK_H248
    if (i != ori_serverType)
	{
        if (SERVER_H248 == ori_serverType) 
		{
			serverType_change = 2; //248 to sip
        }
        else if (i == SERVER_H248) 
		{
			serverType_change = 1; //sip to 248
        }
    }
#endif
    pVoIPCfg->X_CT_servertype = i;
		
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		
#if 0
		strcpy((char *) pCfg->dialplan, boaGetVar(wp, "dialplan", ""));
		pCfg ->digitmap_enable=!gstrcmp(boaGetVar(wp, "digitmap_enable", ""), "on");
#endif

		//retry reg
		pCfg->proxies[0].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));
		pCfg->proxies[1].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));
		
		//sessionExpiry (sec)
		pCfg->proxies[0].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy0_sessionExpiry", "")));
		pCfg->proxies[1].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy0_sessionExpiry", "")));

		/* 
		//sessionExpiry (min)
		pCfg->proxies[0].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
		pCfg->proxies[1].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
		*/ 
	
		for (i=0; i< MAX_PROXY; i++)
		{
			//proxy addr
			sprintf(szName, "proxy%d_addr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].addr, boaGetVar(wp, szName, ""));
			RCM_DBG;
			
			//proxy port
			sprintf(szName, "proxy%d_port", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].port = atoi(boaGetVar(wp, szName, "5060"));
			RCM_DBG;

			//proxy addr
			sprintf(szName, "reg_proxy%d_addr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].RegistrarServer_addr, boaGetVar(wp, szName, ""));
			RCM_DBG;
			
			//proxy port
			sprintf(szName, "reg_proxy%d_port", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].RegistrarServer_port = atoi(boaGetVar(wp, szName, "5060"));
			RCM_DBG;
#if 0 //temp remove , if set addr auto enable 			
			//outbound proxy enable/disable
			sprintf(szName, "proxy%d_obEnable", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].outbound_enable = !gstrcmp(boaGetVar(wp, szName, ""), "enable");
			RCM_DBG;
#endif			
			//outbound proxy addr
			sprintf(szName, "proxy%d_obAddr", i);
			DBG_WEB_VAR(szName);
			strcpy(pCfg->proxies[i].outbound_addr, boaGetVar(wp, szName, ""));
			if (strlen(pCfg->proxies[i].outbound_addr) !=0)
			{
				pCfg->proxies[i].outbound_enable=1;
			}
			RCM_DBG;
			
			//outbound proxy port
			sprintf(szName, "proxy%d_obPort", i);
			DBG_WEB_VAR(szName);
			pCfg->proxies[i].outbound_port = atoi(boaGetVar(wp, szName, "5060"));

			//domain_name
			sprintf(szName, "proxy%d_domain_name", i);
			strcpy(pCfg->proxies[i].domain_name, boaGetVar(wp, szName, ""));

			//reg_expire
			sprintf(szName, "proxy%d_reg_expire", i);
			pCfg->proxies[i].reg_expire = atoi(boaGetVar(wp, szName, "60"));
		}
#ifdef CONFIG_USER_RTK_H248
		sprintf(szName, "h248_port%d_account_enable", voip_port+1);
		if (pVoIPCfg->X_CT_servertype==SERVER_H248)
		{
			if( strcmp("enable", boaGetVar(wp, szName, "")) == 0)
			{
				fprintf(stderr , "%s(%d) h248 enable !!\n" , __FUNCTION__ , __LINE__ );
            	pCfg->proxies[0].enable |= PROXY_ENABLED;
			}
			else
			{
				fprintf(stderr , "%s(%d) h248 disable !!\n" , __FUNCTION__ , __LINE__ );
				pCfg->proxies[0].enable &= ~PROXY_ENABLED;
			}
		}
#endif
	}
#ifdef CONFIG_USER_RTK_H248
    //h248_port
    pVoIPCfg->mg_port = atoi(boaGetVar(wp, "h248_port", ""));
    
    //h248_mgc0_address
    strcpy(pVoIPCfg->mgc_ip, boaGetVar(wp, "h248_mgc0_address", ""));

    //h248_mgc0_port
    pVoIPCfg->mgc_port = atoi(boaGetVar(wp, "h248_mgc0_port", ""));
   
    //h248_mgc1_address
    strcpy(pVoIPCfg->bak_mgc_ip, boaGetVar(wp, "h248_mgc1_address", ""));

    //h248_mgc1_port
    pVoIPCfg->bak_mgc_port = atoi(boaGetVar(wp, "h248_mgc1_port", ""));

    //h248_termination_id
    strcpy(pVoIPCfg->mg_name, boaGetVar(wp, "h248_termination_id", ""));

    //h248_domain_name
    strcpy(pVoIPCfg->Domain, boaGetVar(wp, "h248_domain_name", ""));

    //h248_phy_term_prefix
    strcpy(pVoIPCfg->mg_prefix, boaGetVar(wp, "h248_phy_term_prefix", ""));

    //h248_phy_term_start
    pVoIPCfg->mg_phyTermPrefixStart = atoi(boaGetVar(wp, "h248_phy_term_start", ""));

    //h248_phy_term_len
    pVoIPCfg->mg_phyTermPrefixLen = atoi(boaGetVar(wp, "h248_phy_term_len", ""));

    //h248_rtp_term_prefix
    strcpy(pVoIPCfg->mg_rtp_prefix, boaGetVar(wp, "h248_rtp_term_prefix", ""));

    //h248_rtp_term_start
    pVoIPCfg->mg_rtpPrefixStart = atoi(boaGetVar(wp, "h248_rtp_term_start", ""));

    //h248_rtp_term_len
    pVoIPCfg->mg_rtpPrefixLen = atoi(boaGetVar(wp, "h248_rtp_term_len", ""));

    //h248_id_type
    pVoIPCfg->mg_deviceIdType = atoi(boaGetVar(wp, "h248_id_type", ""));

    //h248_encoding_type
	strcpy(pVoIPCfg->mg_encoding, boaGetVar(wp, "h248_encoding_type", ""));
        
    //h248_phy_term_method
    pVoIPCfg->mg_phyTermConfigMethod = atoi(boaGetVar(wp, "h248_phy_term_config_method", ""));
    
    //h248_phy_term_id
    strcpy(pVoIPCfg->mg_phyTermID, boaGetVar(wp, "h248_phy_term_id", ""));        
#endif
	
    //Transport Protocol
    pVoIPCfg->transport_protocol = atoi(boaGetVar(wp, "transport_protocol", "0"));

	RCM_DBG;

	syslog(LOG_CRIT,"VoIP basic setting.");
	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif

	sprintf(redirect_url, "/cmcc_app_voip.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	if (1 == serverType_change) // sip to 248
	{
		printf("===> sip to 248\n");
		system("sh /etc/scripts/SIPtoh248.sh");
	}
	else if (2 == serverType_change) // 248 to sip
	{
		printf("===> 248 to sip\n");
		system("sh /etc/scripts/h248toSIP.sh");
	}
	else //server type not change
	{
		printf("===> server not change\n");
		web_restart_solar();
	}

	boaRedirect(wp, redirect_url);
#endif
}
	
void asp_voip2_cmcc_set(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	char szPrecedence[12];
	char redirect_url[50];
	int voip_port;
	char szName[64];
	char *ptr;
	int caller_id_area=0;

	printf("---Enter %s---\n", __FUNCTION__);

	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	RCM_DBG;

	//E8C support SIP REDUNDANCY 
	pVoIPCfg->rfc_flags |= SIP_SERVER_REDUNDANCY;
		
	for (voip_port=0; voip_port< VOIP_PORTS; voip_port++)
	{
		pCfg = &pVoIPCfg->ports[voip_port];
		
		pCfg->digitmap_enable = !gstrcmp(boaGetVar(wp, "digitmap_enable", ""), "on");
		
		//sipPort
		pCfg->sip_port = atoi(boaGetVar(wp, "sipPort", "5060"));

		//rtp port
		if(voip_port==0)
			pCfg->media_port = atoi(boaGetVar(wp, "rtpPort", "9000"));
		else
			pCfg->media_port = atoi(boaGetVar(wp, "rtpPort", "9000"))+4;

		//dtmf relay mode
        pCfg->dtmf_mode = atoi(boaGetVar(wp, "dtmfMode", ""));

        //LEC
        pCfg->lec = !gstrcmp(boaGetVar(wp, "useLec", ""), "on");

		//VAD
		pCfg->vad = !gstrcmp(boaGetVar(wp, "useVad", ""), "on");

		//CNG
		pCfg->cng = !gstrcmp(boaGetVar(wp, "useCng", ""), "on");

        //Jitter Buffer Control
        pCfg->maxDelay = atoi(boaGetVar(wp, "maxDelay", "13"));

        //Jitter Factor
        pCfg->jitter_factor = !gstrcmp(boaGetVar(wp, "jitterFactor", ""), "on");
				
		//T38
		pCfg->useT38 = !gstrcmp(boaGetVar(wp, "useT38", ""), "on");

		// V.152
    	pCfg->v152_codec_type = atoi(boaGetVar(wp, "V152_codec_type", "0"));
		
		//caller id
#if 0		
		ptr	 = boaGetVar(wp, "caller_id", "");		
		for(i=0; i<CID_MAX; i++)
		{
			if (!gstrcmp(ptr, e8c_cid[i]))
				break;
		}
		if (i == CID_MAX)
			i = CID_DTMF;
#endif
		i=atoi(boaGetVar(wp, "caller_id", ""));
		caller_id_area=(pCfg->caller_id_mode&0xff8);
		pCfg->caller_id_mode = i;
		pCfg->caller_id_mode|=caller_id_area;

		//flash hook time
	        i = atoi(boaGetVar(wp, "flash_hook_time", ""));
		if ((i >= 100) && ( i <= 2000))
			pCfg->flash_hook_time = i;
		else
			pCfg->flash_hook_time = 300;

	        i = atoi(boaGetVar(wp, "flash_hook_time_min", ""));
		if ( i >= pCfg->flash_hook_time )
			pCfg->flash_hook_time_min = 0;
		else
			pCfg->flash_hook_time_min = i;
		
		
		//codec
		//asp_e8c_codec_set(wp, pCfg);

		//heart beat cycle
		pCfg->proxies[0].HeartbeatCycle = atoi(boaGetVar(wp, "HeartbeatCycle", ""));
		pCfg->proxies[1].HeartbeatCycle = atoi(boaGetVar(wp, "HeartbeatCycle", ""));

        //reg_expire
		pCfg->proxies[0].reg_expire = atoi(boaGetVar(wp, "proxy0_reg_expire", "60"));
		pCfg->proxies[1].reg_expire = atoi(boaGetVar(wp, "proxy0_reg_expire", "60"));

		//retry reg
		pCfg->proxies[0].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));
		pCfg->proxies[1].RegisterRetryInterval = atoi(boaGetVar(wp, "RegisterRetryInterval", ""));
#if 1
		//sessionExpiry (sec)
		pCfg->proxies[0].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy0_sessionExpiry", "")));
		pCfg->proxies[1].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy0_sessionExpiry", "")));
#else      
		//sessionExpiry (min)
		pCfg->proxies[0].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
		pCfg->proxies[1].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
#endif
#if 0
		//sessionEnable
		if( strcmp("enable", boaGetVar(wp, "proxy0_sessionEnable", "")) == 0)
        {
			//sessionExpiry
			pCfg->proxies[0].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
			pCfg->proxies[1].SessionUpdateTimer = (atoi(boaGetVar(wp, "proxy_cmcc_sessionExpiry", ""))*60);
		}
        else
        {
			pCfg->proxies[0].SessionUpdateTimer = 0;
			pCfg->proxies[1].SessionUpdateTimer = 0;
		}
#endif
		i = atoi(boaGetVar(wp, "spk_voice_gain", ""));
		if ((i>= -32) && (i<=31) )
			pCfg->spk_voice_gain=i;
		else
			pCfg->spk_voice_gain=0;

		i = atoi(boaGetVar(wp, "mic_voice_gain", ""));
		if ((i>= -32) && (i<=31) )
			pCfg->mic_voice_gain=i;
		else
			pCfg->mic_voice_gain=0;

		if(!gstrcmp(boaGetVar(wp, "3_way", ""), "on"))
			pCfg->fxsport_flag &= ~FXS_3WAY_DISABLE;
		else
			pCfg->fxsport_flag |= FXS_3WAY_DISABLE;

        if(!gstrcmp(boaGetVar(wp, "Conference_server_cpe", ""), "on"))
			pCfg->fxsport_flag |= FXS_3WAY_ON_SERVER; //on server
		else 
			pCfg->fxsport_flag &= ~FXS_3WAY_ON_SERVER; //on CPE	
       		
		pCfg->call_waiting_enable = !gstrcmp(boaGetVar(wp, "call_waiting", ""), "on");
			
		pCfg->hotline_enable = atoi(boaGetVar(wp, "hot_line_options", "0"));
		strcpy((char *) pCfg->hotline_number, boaGetVar(wp, "hotline_number", ""));

		if(!gstrcmp(boaGetVar(wp, "call_transfer", ""), "on"))
			pCfg->fxsport_flag &= ~FXS_CALL_TRANSFER_disable;
		else
			pCfg->fxsport_flag |= FXS_CALL_TRANSFER_disable;

		strcpy(pCfg->uc_forward, boaGetVar(wp, "uc_call_forward_number", ""));		
		pCfg->uc_forward_enable = !gstrcmp(boaGetVar(wp, "uc_call_forward", ""), "on");

		/* set "unconditional call forward number" as a busy call forward number */
		strcpy(pCfg->busy_forward, boaGetVar(wp, "uc_call_forward_number", "")); 
		pCfg->busy_forward_enable = !gstrcmp(boaGetVar(wp, "busy_call_forward", ""), "on");

		/* set "unconditional call forward number" as a no answer call forward number */
		strcpy(pCfg->na_forward, boaGetVar(wp, "uc_call_forward_number", "")); 
		pCfg->na_forward_enable = !gstrcmp(boaGetVar(wp, "na_call_forward", ""), "on");
		pCfg->na_forward_time = atoi(boaGetVar(wp, "na_call_forward_time", "0"));
		
#ifdef SUPPORT_RTP_REDUNDANT_APP
		pCfg->rtp_redundant_codec = atoi(boaGetVar(wp, "rtp_redundant_codec_options", "-1"));
		pCfg->rtp_redundant_payload_type = atoi(boaGetVar(wp, "rtp_redundant_payload_type", "121"));
#endif
        strcpy((char *) pCfg->dialplan, boaGetVar(wp, "dialplan", ""));
		pCfg->digitmap_enable = !gstrcmp(boaGetVar(wp, "digitmap_enable", ""), "on");

		strcpy((char *) pCfg->digitmap_special, boaGetVar(wp, "digitmap_special", ""));
		pCfg->digitmap_special_enable = !gstrcmp(boaGetVar(wp, "digitmap_special_enable", ""), "on");

		strcpy((char *) pCfg->PSTN_routing_prefix, boaGetVar(wp, "PBXPrefix", ""));
		if(!gstrcmp(boaGetVar(wp, "PBXPrefixEnable", ""), "on"))
			pCfg->func_flags |= SIP_PBXPREFIX_ENABLE;
		else
			pCfg->func_flags &= ~SIP_PBXPREFIX_ENABLE;
		
		if(!gstrcmp(boaGetVar(wp, "pbxdial_second_enable", ""), "on"))
			pCfg->func_flags |= SIP_PBXDIAL_SECONDARY_ENABLE;
		else
			pCfg->func_flags &= ~SIP_PBXDIAL_SECONDARY_ENABLE;

		if(atoi(boaGetVar(wp, "SUBS_options", ""))==3)
		{
			pCfg->proxies[0].enable &= ~PROXY_SUBSCRIBE; 
			pCfg->proxies[1].enable &= ~PROXY_SUBSCRIBE;
			pVoIPCfg->subs_type = SUBS_TYPE_ALL;
		}
		else
		{
			pCfg->proxies[0].enable |= PROXY_SUBSCRIBE; 
			pCfg->proxies[1].enable |= PROXY_SUBSCRIBE; 
			pVoIPCfg->subs_type = atoi(boaGetVar(wp, "SUBS_options", ""));
		}

        //hotline_mode
		pCfg->hotline_mode = atoi(boaGetVar(wp, "hotline_mode", "")); 

		//hotline_activate
		strcpy((char *)pCfg->hotline_activate, boaGetVar(wp, "hotline_activate", ""));
	}

	pVoIPCfg->subs_time=atoi(boaGetVar(wp, "SUBS_time", "0"));

	pVoIPCfg->rfc_flags &= ~SIP_FAX_AUTO;
	if (atoi(boaGetVar(wp, "FAX_options", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_FAX_AUTO;

	pVoIPCfg->rfc_flags &= ~SIP_DIGIT_MAP_MODE_MIN;
	if (atoi(boaGetVar(wp, "digit_map_mode", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_DIGIT_MAP_MODE_MIN;
	
	pVoIPCfg->rfc_flags &= ~SIP_DATE;
	if(!gstrcmp(boaGetVar(wp, "sip_date", ""), "on")){
		pVoIPCfg->rfc_flags |= SIP_DATE;
	}
	
	pVoIPCfg->rfc_flags &= ~SIP_SESSION_TIMER_USE_INVITE;
	if (atoi(boaGetVar(wp, "session_options", "0"))==1)
		pVoIPCfg->rfc_flags |= SIP_SESSION_TIMER_USE_INVITE;

    if(!gstrcmp(boaGetVar(wp, "sip_disable_prack", ""), "on"))
		pVoIPCfg->rfc_flags &= ~SIP_DISABLE_PRACK;
	else
		pVoIPCfg->rfc_flags |= SIP_DISABLE_PRACK;

	if(atoi(boaGetVar(wp, "reverse_polariy", "0"))==1)		
    	pVoIPCfg->rfc_flags |= FXS_REVERSE_POLARIY_OFF;
	else
		pVoIPCfg->rfc_flags &= ~FXS_REVERSE_POLARIY_OFF;
 	
    if(!gstrcmp(boaGetVar(wp, "pound_to_ascii", ""), "on"))
        pVoIPCfg->rfc_flags |= SIP_ENABLE_POUND_KEY_TO_ASCII;
	else 
        pVoIPCfg->rfc_flags &= ~SIP_ENABLE_POUND_KEY_TO_ASCII;

	pVoIPCfg->off_hook_alarm=atoi(boaGetVar(wp, "off_hook_alarm", ""));
	pVoIPCfg->auto_dial=atoi(boaGetVar(wp, "auto_dial", ""));
	pVoIPCfg->InterDigitTimerLong=atoi(boaGetVar(wp, "InterDigitTimerLong", ""));

	pVoIPCfg->BusyToneTimer=atoi(boaGetVar(wp, "BusyToneTimer", ""));
	pVoIPCfg->HangingReminderToneTimer=atoi(boaGetVar(wp, "HangingReminderToneTimer", ""));
	pVoIPCfg->NoAnswerTimer=atoi(boaGetVar(wp, "NoAnswerTimer", ""));

    pVoIPCfg->rtpDscp = atoi(boaGetVar(wp, "rtpDscp", "0"));
	pVoIPCfg->sipDscp = atoi(boaGetVar(wp, "sipDscp", "0"));

	pVoIPCfg->ringVoltageFreq = atoi(boaGetVar(wp, "RingVoltage", ""));
	
	//heartbeat mode
	if(atoi(boaGetVar(wp, "heartbeat_mode", "")) == 0)
		pVoIPCfg->rfc_flags |= SIP_OPTION;
	else 
		pVoIPCfg->rfc_flags &= ~SIP_OPTION;
	
	
    //Support flash hook by SIP INFO
	ptr = boaGetVar(wp, "hookflash_type", "");

	for (i=0; i<2; i++)
	{
		if (!gstrcmp(ptr, e8c_hookflash_type[i]))
			break;
	}

	if (i == 1) 
        pVoIPCfg->rfc_flags |= SIP_FLASHHOOK_INFO;
	else
		pVoIPCfg->rfc_flags &= ~SIP_FLASHHOOK_INFO;	

    //conference-uri
    strcpy(pVoIPCfg->conference_uri, boaGetVar(wp, "conference_uri", ""));

    //Get call id from P-Asserted-Identity
    if(atoi(boaGetVar(wp, "callid_from_PAI", "")) == 1)
        pVoIPCfg->rfc_flags |= SIP_P_ASSERTED_IDENTITY;
	else 
        pVoIPCfg->rfc_flags &= ~SIP_P_ASSERTED_IDENTITY;

    //Display prefix country/region code 
    pVoIPCfg->display_prefix_num = atoi(boaGetVar(wp, "display_prefix_num", ""));

    //URL type
    if(atoi(boaGetVar(wp, "url_type", "")) == 1)
        pVoIPCfg->rfc_flags |= SIP_URL_TYPE;
	else 
        pVoIPCfg->rfc_flags &= ~SIP_URL_TYPE;

#if 1 //voip parameters, need to implement
	//max_start_time
	pVoIPCfg->max_start_time = atoi(boaGetVar(wp, "max_start_time", ""));

	//t_timer
	pVoIPCfg->t_timer = atoi(boaGetVar(wp, "t_timer", ""));

	//digit_timer_mode
    pVoIPCfg->digit_timer_mode = atoi(boaGetVar(wp, "digit_timer_mode", ""));

	//register_wtih_port
    pVoIPCfg->register_wtih_port = atoi(boaGetVar(wp, "register_wtih_port", ""));

	//user_equal_phone
    if(atoi(boaGetVar(wp, "user_equal_phone", "")))
        pVoIPCfg->rfc_flags |= SIP_USER_EQUAL_PHONE;
	else 
        pVoIPCfg->rfc_flags &= ~SIP_USER_EQUAL_PHONE;

	//call_waiting_type
    pVoIPCfg->call_waiting_type = atoi(boaGetVar(wp, "call_waiting_type", ""));
	
	//unreg_dial_tone
    pVoIPCfg->unreg_dial_tone = atoi(boaGetVar(wp, "unreg_dial_tone", ""));

	//codec_prior_control
    pVoIPCfg->codec_prior_control = atoi(boaGetVar(wp, "codec_prior_control", ""));

	//business_type
    pVoIPCfg->business_type = atoi(boaGetVar(wp, "business_type", ""));

	//terminator_type
    pVoIPCfg->terminator_type = atoi(boaGetVar(wp, "terminator_type", ""));

	//at_to_ascii
	if(!gstrcmp(boaGetVar(wp, "at_to_ascii", ""), "on"))
        pVoIPCfg->rfc_flags |= SIP_ENABLE_AT_KEY_TO_ASCII;
	else 
        pVoIPCfg->rfc_flags &= ~SIP_ENABLE_AT_KEY_TO_ASCII;
	
	//priority_unregister
	pVoIPCfg->priority_unregister = !gstrcmp(boaGetVar(wp, "priority_unregister", ""), "on");
		
	//cal_terminal_time
	pVoIPCfg->cal_terminal_time = !gstrcmp(boaGetVar(wp, "cal_terminal_time", ""), "on");
	
	//minSessionExpiry
	pVoIPCfg->MINI_SessionExpire = atoi(boaGetVar(wp, "minSessionExpiry", ""));
	
	//uac_uas
    pVoIPCfg->uac_uas = atoi(boaGetVar(wp, "uac_uas", ""));

	//min_onhook_time
	pVoIPCfg->min_onhook_time = atoi(boaGetVar(wp, "min_onhook_time", ""));
	
	//sync_phone_time
    pVoIPCfg->sync_phone_time = atoi(boaGetVar(wp, "sync_phone_time", ""));

    //802.1p
	pVoIPCfg->enable_802_11p = !gstrcmp(boaGetVar(wp, "enable_802_11p", ""), "on");
#endif //need to implement

	RCM_DBG;

	syslog(LOG_CRIT,"VoIP advanced setting.");

	web_flash_set(pVoIPCfg);
	
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	restart_dnsrelay();
#endif

	sprintf(redirect_url, "/cmcc_app_voip2.asp");
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

