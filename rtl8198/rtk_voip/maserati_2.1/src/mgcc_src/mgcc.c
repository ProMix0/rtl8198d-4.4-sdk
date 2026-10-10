#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdarg.h>
//#include <rtk/utility.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <arpa/inet.h>
#ifdef CONFIG_DEFAULTS_KERNEL_3_10
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#else
#include <include/linux/netlink.h>
#include <include/linux/rtnetlink.h>
#endif
#include "rcm_timer.h"
#include "mgcc.h"
//#include "uglib.h"
#include "ortp.h"
#include "rcm_sim_defs.h"
#include "built_time.h"

#ifdef CONFIG_RTK_VOIP_TR104
#include <sys/un.h>
#endif

#include "mgcc_api.h"
#include "eXosip2/eXosip.h"

// ==============================
// DEFINE & MACRO
// ==============================
#define ADDSIG 1
#define DELSIG 0
#define LOGLEVEL_EXCEP       0x01
#define LOGLEVEL_ERROR       0x02
#define LOGLEVEL_WARNING     0x04
#define LOGLEVEL_INFO        0x08
#define LOGLEVEL_DEBUG       0x10
#define LOGLEVEL_ENTER       0x20
#define LOGLEVEL_LEAVE       0x40
#define LOGLEVEL_SYNC        0x80
#define LOGLEVEL_ALL         0xff
#define LOGLEVEL_NONE        0x00
#define LOGLEVEL_1    LOGLEVEL_EXCEP|LOGLEVEL_ERROR|LOGLEVEL_WARNING
#define LOGLEVEL_2    LOGLEVEL_1|LOGLEVEL_INFO|LOGLEVEL_DEBUG
//#define CALL_WAITING_CONTINUE

// ==============================
// function prototype
// ==============================
void rcm_netlink_handler(int netlink_desc) __attribute__((weak));
void rcm_retreive_voip_if_name(char *if_name_dst, int if_name_max_len);

// ==============================
// global variable
// ==============================
MGAPP mgcc;  //mgcc ==> megaco call control
MGAPP *mgcc_ptr = &mgcc;
voipCfgParam_t *g_pVoIPCfg = NULL;
voip_state_share_t *g_shmVoIPstate=NULL;
voip_flash_share_t *g_pVoIPShare = NULL;
int g_MaxVoIPPorts = 2;
SystemParam_t g_SystemParam;
int g_wrap_client = 0;
char *FIFO_CONTROL=NULL;
char *pidfile =NULL;
/* support RCM WATCHDOG */
int RCM_WATCHDOG_pipe=0;
int RCM_WATCHDOG_period=0;
char *wdt_fifo=NULL;
unsigned int current_mgc=PRIMARY_MGC;
char megacoBuildTime[32];
int g_mgcc_log = 0;
int g_hook_state = 0; //default onhook;
int keep_alive_fd=0;
struct sockaddr_in servaddr; /* the server's full addr */
int reg_timeout = 0;

#ifdef CONFIG_RTK_VOIP_TR104
cwmpEvtMsg mg_tr104_st;
#endif

// ==============================
// static variable
// ==============================
static gboolean run=TRUE;
static int updateChid=0;
static int timerStartFlag = 0;
static rtp_config_t rtp_config;
static payloadtype_config_t codec_config;
static rtcp_config_t rtcp_config;
static int8 dtmf_code = 0;

#ifdef CONFIG_RTK_VOIP_TR104
static int ipcSocket = 0;
#endif
// ==============================

void mg_print(const gchar *fmt, ...)
{
	va_list args;
	char msg[DBG_PRINT_MAX];

	if( g_mgcc_log ){

		va_start(args, fmt);
		vsprintf(msg, fmt, args);
		va_end(args);
		printf("%s",msg);
		return;
	}
}

void restart_mg(void)
{

     app_close(&mgcc);
     run = FALSE; //break while loop then terminate this app
     updateChid=0;
}

void rtk_t38_start(int chid, char *address, int lport, int rport)
{
	t38udp_config_t t38udp_config;
	t38_payloadtype_config_t t38_config;

    //stop rtp
    rtk_SetRtpSessionState(chid, 0, rtp_session_inactive);

	// set rtp session
	t38udp_config.chid = chid;
	t38udp_config.sid = 0;
	t38udp_config.isTcp = 0;		// use udp
	t38udp_config.extIp = htonl(mgcc_ptr->bound_voip_ipaddress);
	t38udp_config.remIp = htonl(address);
	t38udp_config.extPort = htons(lport);
	t38udp_config.remPort = htons(rport);
#ifdef SUPPORT_VOICE_QOS
	t38udp_config.tos = 0;
#endif

	rtk_SetT38UdpConfig(&t38udp_config);

	// set rtp payload, and other session parameters.
	t38_config.chid = chid;
	t38_config.sid = 0;
	t38_config.bT38ParamEnable = 0;
	t38_config.nT38MaxBuffer = 0;
	t38_config.nT38RateMgt = 0;
	t38_config.nT38MaxRate = 0;
	t38_config.bT38EnableECM = 0;
	t38_config.nT38ECCSignal = 0;
	t38_config.nT38ECCData = 0;
	t38_config.bT38EnableSpoof = 0;
	t38_config.nT38DuplicateNum = 0;
	t38_config.nPcmMode = 2;	// narrow-band
	rtk_SetT38PayloadType(&t38_config);

	// start rtp
	rtk_SetRtpSessionState(chid, 0, rtp_session_sendrecv);
}

int CheckFax(int chid, char *termName)
{
    //uint32 fax = 0;
	uint32 fax_end = 0;
    uint32 fax_modem = FMS_IDLE;
    char cmd[64];

    if (g_pVoIPCfg->ports[0].T38ParamEnable == 0)
    {
        return 0;
    }

    rcm_tapi_GetFaxModemEvent(chid, &fax_modem, NULL, 0);

	if (fax_modem == FMS_V21_FLAG)
	{
		//report v21 event to MGC
        mg_print("Get FAX V21 event, ch=%d\n",chid);
        /* ctyp/dtone{ dtt=V21flag } */
        sprintf(cmd,"callCmd,{term.SendEvent} {%s} {ctyp} {dtone} {dtt} {V21flag}\n",termName);
	    rcm_send_msg(cmd);
	}

    rcm_tapi_GetFaxEndDetect(chid, &fax_end);
    if (fax_end)
    {
        mg_print("Get FAX End event, ch=%d\n", chid);
        /* fax/faxconnchange{ faxconnchng=EOF } */
        sprintf(cmd,"callCmd,{term.SendEvent} {%s} {fax} {faxconnchange} {faxconnchng} {EOF}\n",termName);
	    rcm_send_msg(cmd);
    }
    return 0;

}

void update_state(int chid, int state)
{
#ifdef CONFIG_RTK_VOIP_TR104
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);
#endif

    mg_print("====> [update_state] chid %d new state=%d\n", chid, state);
    mgcc_ptr->ports[chid].line_state = state;

#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
    if ( state == STATE_CONNECTED)
    {
        if((tr104_e8c_autotest->enable == 1) &&
           (tr104_e8c_autotest->TestType == 0) &&  /* Caller */
           (tr104_e8c_autotest->TestSelector == SIMULATE_TEST))
        {
            mg_print("send dtmf %s to peer \n", tr104_e8c_autotest->DailDTMFConfirmNumber);
            rtk_SetDtmfCfg(chid, 1, 1);
            tr104_e8c_autotest->status = E8C_TEST_DIAL_DTMF;
            tr104_e8c_autotest->dtmfindex = 0;
            rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 2000); //after 1 sec start test.
        }
    }
#endif
}

int reset_web_E8C_register_status(void)
{
    FILE *fh;
    char buf[MAX_VOIP_PORTS];

    printf("===>[%s]\n",__FUNCTION__);
    /* To show the register status on Web page. */
    fh = fopen(_PATH_TMP_E8C_STATUS, "w");
    if (!fh) {
        printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_E8C_STATUS);
        printf("\nerrno=%d\n", errno);
    }
    else {
        printf("Reset E8C_voipstatus ...\n");
        memset(buf, (int) '0', sizeof(buf));

        fwrite(buf, sizeof(buf), 1, fh);
        fclose(fh);
    }

    return 0;

}

int reset_web_register_status(void)
{
    FILE *fh;
    char buf[MAX_VOIP_PORTS];
    /* To show the register status on Web page. */
    fh = fopen(_PATH_TMP_STATUS, "w");
    if (!fh) {
        printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
        printf("\nerrno=%d\n", errno);
    }
    else {
        printf("Reset voipstatus ...\n");
        memset(buf, (int) '0', sizeof(buf));

        fwrite(buf, sizeof(buf), 1, fh);
        fclose(fh);
    }

    return 0;

}



void update_and_display_registration_status( int chid, MGAPP *mg, ProxyRegStatus newStatus )
{
    FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];
	char buf_status = '0';
	const char *pszPrompt = NULL;

    //printf("SET REG status from %d to %d.\n", mg->ports[chid].reg_status, newStatus);

    // set new status
	if( mg->ports[chid].reg_status == newStatus )
		return;

    mg->ports[chid].reg_status = newStatus;
    mg_print("chid:%d, reg_status:%d\n", chid, mg->ports[chid].reg_status);

    // decide display status and warning string
	switch( newStatus ) {
	case REGSTATUS_REG_ING:					/* registering */
		buf_status = '2';
		pszPrompt = "registration_ongoing";
		break;

	case REGSTATUS_REG_SUCCESS:				/* register success */
	case REGSTATUS_REG_SUCCESS_EXPIRED:		/* register success but expired, so it is registering */
		buf_status = '1';
		pszPrompt = "registration_success";
        printf( "### H.248 REGSTATUS_REG_SUCCESS ###\n");
		break;

	case REGSTATUS_INIT:
		buf_status = '0';
		pszPrompt = "no_register";
		break;
	case REGSTATUS_RESTART:
		buf_status = '4';
		break;
	case REGSTATUS_REG_FAIL_RETRY_S:		/* register failure (retry in short delay) */
		buf_status = '5';
		break;
	case REGSTATUS_REG_FAIL:				/* register failure (don't retry) */
	case REGSTATUS_REG_FAIL_RETRY_L:		/* register failure (retry in long delay) */
	default:
		buf_status = '3';
		pszPrompt = "registration_faillure";

		break;
	}

	// update status for web
	fh = fopen(_PATH_TMP_STATUS, "r+");
	if (!fh) {
		mg_print("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		mg_print("\nerrno=%d\n", errno);
	}
	else {
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			mg_print("%s(%d): The content of /tmp/status is NULL!!\n", pszPrompt, newStatus);
			mg_print("\nerrno=%d\n", errno);
		}
		else {
			buf[0] = buf_status;
			fseek(fh, 0L, SEEK_SET);
			fwrite(buf, sizeof(buf), 1, fh);
		}
		fclose(fh);
	}


}

void E8C_update_and_display_registration_status( int newStatus )
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS];
	char buf_status = '0';
	const char *pszPrompt = NULL;

	// decide display status and warning string
	switch( newStatus ) {
	case VOIP_STATUS_INIT:
		buf_status = '0';
		pszPrompt = "INIT";
		break;
	case VOIP_STATUS_SUCCESS:
		buf_status = '1';
		pszPrompt = "STATUS_SUCCESS";
		break;
	case VOIP_NOVALID_CONNECTION:
		buf_status = '2';
		pszPrompt = "NOVALID_CONNECTION";
		break;
	case VOIP_STATUS_IADERROR:
		buf_status = '3';
		pszPrompt = "STATUS_IADERROR";
		break;
	case VOIP_STATUS_NOROUTE:
		buf_status = '4';
		pszPrompt = "STATUS_NOROUTE";
		break;
	case VOIP_STATUS_NORESPONSE:
		buf_status = '5';
		pszPrompt = "STATUS_NORESPONSE";
		break;
	case VOIP_STATUS_ACCOUNTERR:
		buf_status = '6';
		pszPrompt = "STATUS_ACCOUNTERR";
		break;
	case VOIP_STATUS_UNKNOWN:
		buf_status = '7';
		pszPrompt = "STATUS_UNKNOWN";
		break;
	case VOIP_STATUS_LINE_DISABLED:
		buf_status = '8';
		pszPrompt = "STATUS_LINE_DISABLED";
		break;
	case VOIP_STATUS_ADDR_ERR:
		buf_status = '9';
		pszPrompt = "STATUS_ADDR_ERR";
		break;

	default :
		break;

	}

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
			buf[0] = buf_status;
			fseek(fh, 0L, SEEK_SET);
			fwrite(buf, sizeof(buf), 1, fh);
		}
		fclose(fh);
	}
}


#define MAX_CMD_EXEC_OUTPUT_LEN 128

int get_exec_output(char* cmd, char* value)
{
    FILE *pf = popen(cmd, "r");
    char buf[MAX_CMD_EXEC_OUTPUT_LEN];
    int CmdLen;

    CmdLen=strlen(cmd)-strlen("mib get ");
    memset(buf,0,MAX_CMD_EXEC_OUTPUT_LEN);

    fread(buf, MAX_CMD_EXEC_OUTPUT_LEN - 1, 1, pf);
    pclose(pf);

    if(strstr(buf,"="))
    {
        strcpy(value, buf+CmdLen+1);
    }
    else
    {
        strcpy(value, buf);
    }

    return 1;
}

int check_248_server(void)
{
    char loid[32] = {0};
    char loid_pwd[32] = {0};
    char tmp[80] = {0};
    int ret = 0;

    get_exec_output("mib get LOID", loid);
    get_exec_output("mib get LOID_PASSWD", loid_pwd);

    loid[strlen(loid)-1]=0;
    loid_pwd[strlen(loid_pwd)-1]=0;

    snprintf(tmp,sizeof(tmp),"%s%s.kmss",loid,loid_pwd);
    printf("===>[%s] loid=%s, mg_name=%s\n",__FUNCTION__,tmp,g_pVoIPCfg->mg_name);
    if ((0 == strncmp(tmp,g_pVoIPCfg->mg_name,sizeof(tmp))))
    {
        ret = 1;
    }

    memset(tmp,0,sizeof(tmp));
    snprintf(tmp,sizeof(tmp),"%s%s.YNIMS",loid,loid_pwd);
    if ((0 == strncmp(tmp,g_pVoIPCfg->mg_name,sizeof(tmp))))
    {
        ret = 1;
    }

    memset(tmp,0,sizeof(tmp));
    snprintf(tmp,sizeof(tmp),"%s%s",loid,loid_pwd);
    if ((0 == strncmp(tmp,g_pVoIPCfg->mg_name,sizeof(tmp))))
    {
        ret = 1;
    }

    return ret;
}


char *findActiveRtpTerm(int chid, MGAPP *mgcc_ptr)
{
    int i;
    for(i=0; i<2; i++)
    {
        if(mgcc_ptr->ports[chid].rtp_term[i].active)
            return mgcc_ptr->ports[chid].rtp_term[i].termName;
    }
    return "NULL";
}

int findFreeRtpTerm(int chid, MGAPP *mgcc_ptr)
{
    int i;
    for(i=0; i<2; i++)
    {
        if(mgcc_ptr->ports[chid].rtp_term[i].active == FALSE)
            return i;
    }
    return -1;
}

int do_ioctl(unsigned int cmd, struct ifreq *ifr)
{
	int skfd, ret;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}

	ret = ioctl(skfd, cmd, ifr);
	close(skfd);
	return ret;
}

int voip_get_net_link_status(const char *ifname)
{
	struct ifreq ifr;
	struct ethtool_value edata;
	int ret;

	strcpy(ifr.ifr_name, ifname);
	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&edata;

	ret = do_ioctl(SIOCETHTOOL, &ifr);
	if (ret == 0)
		ret = edata.data;
	return ret;
}


int getIdByName(char *termName, MGAPP *mgcc_ptr)
{
	int i;

	for(i=0; i < g_MaxVoIPPorts; i++)
	{
		if(strncmp(mgcc_ptr->ports[i].termName, termName, strlen(mgcc_ptr->ports[i].termName))==0)
			return	i;
	}
	return -1;
}

int ParseCommand(char *cmd, uint32 len, char *argv[MAX_ARGC])
{
    uint32 argc = 0;
    uint32 count = 0, i;

		mg_print("==> cmd=%s\n",cmd );
    for (i = 0; i < len; i++)
    {
        if (cmd[i] == '{')
        {
            if (count == 0)
            {
                argv[argc] = (cmd + i + 1);
            }
            count++;
        }
        if (cmd[i] == '}')
        {
            if (count == 0)
                return 0;
            count--;
            if (count == 0)
            {
                cmd[i] = '\0';
                argc++;
            }
        }
    }
    return argc;
}

/* SD6, bohungwu, netlink integration */
void rcm_netlink_handler(int netlink_desc)
{
	struct sockaddr_nl nl_remote;
	struct nlmsghdr *nlh;
	int nl_remote_len;
	char buf[4096];
	int err;
	char *voip_new_ip=NULL;
	char voip_if_name_new[DNS_LEN] = {0};

    mg_print("Enter-- [rcm_netlink_handler] \n");
	memset(buf, 0x0, 512);
	nl_remote_len = sizeof( nl_remote );
	err = recvfrom(netlink_desc, buf, sizeof(buf), 0, (struct sockaddr *) &nl_remote, &nl_remote_len);
	if (err < 0)
	{
		mg_print("netlink recv failed \n");
		return;
	}

	if (err == 0)
	{
		mg_print("netlink: EOF\n");
		return;
	}

	if (nl_remote_len != sizeof(nl_remote))
	{
		mg_print("netlink: sender address len = %d\n", nl_remote_len);
		return;
	}

	nlh = (struct nlmsghdr *) buf;
	for (; NLMSG_OK(nlh, err); nlh = NLMSG_NEXT(nlh, err))
	{
		switch (nlh->nlmsg_type)
		{
		case NLMSG_DONE:
		case NLMSG_ERROR:
			break;
        case RTM_NEWADDR:
            mg_print("H.248: Receive IP Change Event : RTM_NEWADDR\n");
			mg_print("voip_interface is (%d): %s\n",strlen(mgcc_ptr->current_voip_interface),mgcc_ptr->current_voip_interface);

			if(strlen(g_pVoIPCfg->voip_interface)==0){
				rcm_retreive_voip_if_name(mgcc_ptr->current_voip_interface, DNS_LEN);
			}

			rcm_get_localip_for_voip(mgcc_ptr->current_voip_interface,&voip_new_ip);

			mg_print("voip_new_ip=%s;bound_voip_ipaddress=%s\n", voip_new_ip, mgcc_ptr->bound_voip_ipaddress);
			if(strcmp(voip_new_ip,mgcc_ptr->bound_voip_ipaddress)!=0)
			{
				mg_print("IP change, restart H.248\n");
				//system("sh /etc/scripts/restart_h248.sh");
				mgcc_free(voip_new_ip);
				voip_new_ip = NULL;
               restart_mg();
			}
			break;
		case RTM_DELADDR:
			break;
		case RTM_DELLINK:
		case RTM_NEWLINK:
			mg_print("netlink: link status change\n");
			mg_print("H.248: Receive link status change event :RTM_NEWLINK/RTM_DELLINK\n");

				rcm_retreive_voip_if_name(voip_if_name_new, DNS_LEN);

				mg_print("VoIP WAN interface, original=%s, new=%s\n", mgcc_ptr->current_voip_interface, voip_if_name_new);
				if(strcmp(mgcc_ptr->current_voip_interface, voip_if_name_new) !=0)
				{
					mg_print("VoIP interface changed, restart H.248\n");
					//system("sh /etc/scripts/restart_h248.sh");
					mgcc_free(voip_new_ip);
					voip_new_ip = NULL;
					restart_mg();
				}

			break;
		default:
			mg_print("Maserati/netlink: wrong msg\n");
			break;
		}
	}
	mgcc_free(voip_new_ip);
}

int rcm_get_localip_for_voip(char *interfacename,char **loc){

	struct ifreq ifr;
	int fd;

	*loc=mgcc_malloc(MAXHOSTNAMELEN);

	if((interfacename==NULL)||(strlen(interfacename)==0)){
		//printf("rcm_get_localip_for_voip , interfacename is null or length is 0 , use 127.0.0.1\n");
		//sprintf(*loc,"%s","127.0.0.1");
		//return 0;
        mg_print("rcm_get_localip_for_voip , interfacename is null or length is 0 , use br0\n");
        strcpy(ifr.ifr_name, "br0");
	}
    else
        strcpy(ifr.ifr_name, interfacename);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to interfacename */
	//strcpy(ifr.ifr_name, interfacename);
	mg_print("rcm_get_localip_for_voip , interface is %s\n",interfacename);

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			mg_print("Error openning socket when getting IP address info\n");
			return -1;
	}


	if (ioctl(fd,SIOCGIFADDR,&ifr) < 0) {
		close(fd);
		mg_print("\r\n can't get interface %s ip address \n",interfacename);
		sprintf(*loc,"%s","127.0.0.1");
		return 0;
	}

	/* display result */
	mg_print("rcm_get_localip_for_voip get %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));


	close(fd);
	sprintf(*loc,"%s",inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	return 0;
}


/*get usable voip interface name*/
void rcm_retreive_voip_if_name(char *if_name_dst, int if_name_max_len)
{
	int i;
 	struct ifreq ifr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

//	printf("list all interface\n");

	for(i=0;i<MAX_VOIP_INTERFACE;i++){
		if(strlen(g_pVoIPShare->net_cfg.gateway_interface[i])!=0){
			mg_print("voip_index %d ,interface name:%s\n",i,g_pVoIPShare->net_cfg.gateway_interface[i]);
			mg_print("get_net_link_status is %d\n",voip_get_net_link_status(g_pVoIPShare->net_cfg.gateway_interface[i]));
		}
	}

	mg_print("get interface ip address for VoIP usage\n");
	// interface list from voip_flash.c
	for(i=0;i<MAX_VOIP_INTERFACE;i++){
		mg_print("voip_index %d ,interface name:%s\n",i,g_pVoIPShare->net_cfg.gateway_interface[i]);
		if(strlen(g_pVoIPShare->net_cfg.gateway_interface[i])!=0){
			bzero(&ifr, sizeof(ifr));
			strcpy(ifr.ifr_name, g_pVoIPShare->net_cfg.gateway_interface[i]);
			if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0) {
				if (ifr.ifr_flags & IFF_UP){
					/*add check ip get ready or not for multi interface */

					/* I want to get an IPv4 IP address */
					ifr.ifr_addr.sa_family = AF_INET;
					if (ioctl(sockfd,SIOCGIFADDR,&ifr) < 0){
						/*can't get interface ip address */
						mg_print("this interface no ip \n");
					}else if (strstr(ifr.ifr_name,"ppp")!=NULL){
						mg_print("this interface is ppp,\n");
						strcpy(if_name_dst, g_pVoIPShare->net_cfg.gateway_interface[i]);
						mg_print("find voip interface : %s\n",if_name_dst);
						break;
					}else if(voip_get_net_link_status(g_pVoIPShare->net_cfg.gateway_interface[i])!=1){
						mg_print("this interface have ip , but dsl status not up \n");
					}else{
						mg_print("this interface have ip ,dsl status up\n");
						strcpy(if_name_dst, g_pVoIPShare->net_cfg.gateway_interface[i]);
						mg_print("find voip interface : %s\n",if_name_dst);
						break;
					}
				}
			}
		}
	}

	close(sockfd);

	return;

}

void rcm_dump_voip_if_name(char *if_name_dst, int if_name_max_len)
{
    int i;
    struct ifreq ifr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("get interface ip address for VoIP usage\n");
    // interface list from voip_flash.c
    for(i=0;i<MAX_VOIP_INTERFACE;i++){
        printf("voip_index %d ,interface name:%s\n",i,g_pVoIPShare->net_cfg.gateway_interface[i]);
        if(strlen(g_pVoIPShare->net_cfg.gateway_interface[i])!=0){
            bzero(&ifr, sizeof(ifr));
            strcpy(ifr.ifr_name, g_pVoIPShare->net_cfg.gateway_interface[i]);
            if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0) {
                if (ifr.ifr_flags & IFF_UP){
                    /*add check ip get ready or not for multi interface */

                    /* I want to get an IPv4 IP address */
                    ifr.ifr_addr.sa_family = AF_INET;
                    if (ioctl(sockfd,SIOCGIFADDR,&ifr) < 0){
                        /*can't get interface ip address */
                        printf("this interface no ip \n");
                    }else{
                        printf("this interface have ip \n");
                        strcpy(if_name_dst, g_pVoIPShare->net_cfg.gateway_interface[i]);
                        printf("find voip interface : %s\n",if_name_dst);
                        break;
                    }
                }
            }
        }
    }

    close(sockfd);

    return;

}

void mg_init_voip_interface(void)
{
    if (strlen(g_pVoIPCfg->voip_interface) == 0) {
        rcm_retreive_voip_if_name(mgcc_ptr->current_voip_interface, DNS_LEN);
    }else{
        strcpy(mgcc_ptr->current_voip_interface,g_pVoIPCfg->voip_interface);
    }
}

static int pidfile_acquire(char *pidfile)
{
	int pid_fd;

	if(pidfile == NULL)
		return -1;

	pid_fd = open(pidfile, O_CREAT | O_WRONLY, 0644);
	if (pid_fd < 0)
		mg_print("Unable to open pidfile %s\n", pidfile);
	else
		lockf(pid_fd, F_LOCK, 0);

	return pid_fd;
}

static void pidfile_write_release(int pid_fd)
{
	FILE *out;

	if(pid_fd < 0)
		return;

	if((out = fdopen(pid_fd, "w")) != NULL) {
		fprintf(out, "%d\n", getpid());
		fclose(out);
	}
	lockf(pid_fd, F_UNLCK, 0);
	close(pid_fd);
}

int rcm_set_mgcc_pidfile(void)
{
	int pid_fd;
	FILE *fp;
	char line[20];
	pid_t pid;

	pidfile = "/var/run/mgcc.pid";

	if ((fp = fopen(pidfile, "r")) != NULL) {
		fgets(line, sizeof(line), fp);
		if (sscanf(line, "%d", &pid)) {
			if (pid > 1)
				kill(pid, SIGTERM);
		}
		fclose(fp);
	}

	pid_fd = pidfile_acquire(pidfile);
	if (pid_fd < 0)
		return 0; //fail
	pidfile_write_release(pid_fd);

	return 1; //success
}

void parse_SDMF(char *sdmf_str, char *num, int num_len, char *date, int date_len)
{
    char dateTime[9];
    char phone_num[16];
    char *p;
    int i;
    //int cidLen;

    mg_print("sdmf_str=%s,num_len=%d,date_len=%d\n",sdmf_str,num_len,date_len);

    p = sdmf_str;
    //cidLen = strlen(sdmf_str);

    /* get date & time */
    p += 4;
    for (i = 0; i < 8; i++)
    {
        dateTime[i] = p[(i*2)+1];
    }
    dateTime[i] = '\0';
    mg_print("dateTime=%s\n",dateTime);
    strncpy(date,dateTime,date_len);

    /* get phone number */
    p += 16;
    for (i = 0; i < (strlen(p)/2) - 1; i++)
    {
        phone_num[i] = p[(i*2)+1];
    }
    phone_num[i] = '\0';
    mg_print("phone_num=%s\n",phone_num);
    strncpy(num,phone_num,num_len);

}

void send_fskCID(int chid ,char *num ,char *dateTime)
{
	TstFskClid clid;

    mg_print("==> dateTime=%s\n",dateTime);
    mg_print("==> num=%s\n",num);

	memset(&clid, 0, sizeof(TstFskClid));
	clid.ch_id = chid;
	clid.service_type = 0;  //service type 1
    //=====Set the FSK Caller ID Parameters-service type 1
    clid.service_type = 0;


	//=====Set the FSK Caller ID Parameters-DATE
    clid.cid_data[0].type = EXAMPLE_FSK_PARAM_DATEnTIME;
    strcpy(clid.cid_data[0].data, dateTime);


    //=====Set the FSK Caller ID Parameters-Calling Line Identify (CLI)
    clid.cid_data[1].type = EXAMPLE_FSK_PARAM_CLI;
    strcpy(clid.cid_data[1].data, num);


	//Only 2 elements for Caller ID data. Set other element to 0 (MUST)
    clid.cid_data[2].type = 0;
	clid.cid_data[3].type = 0;
    clid.cid_data[4].type = 0;
    //=====gen CID
    rtk_GenMdmfFskCid(chid, &clid, 2);
}


int SigHandle(uint32 chid, char *pkg, char *sig, char *val, int sigRing)
{
    char dateTime[16];
    char num[16];
#ifdef CONFIG_RTK_VOIP_TR104
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);
#endif

    mg_print("[SigHandle] chid=%d, pkg=%s, sig=%s, val=%s, ring:%s\n", chid, pkg, sig, val, sigRing?"on":"off");

    if(strncmp(pkg,"al",2) == 0)
    {
        if(strncmp(sig,"ri",2) == 0) //ringing tone
        {
#if defined(CONFIG_RTK_VOIP_TR104)
            if (tr104_e8c_autotest->status != E8C_TEST_START)
#endif
                rtk_SetRingFxs(chid, sigRing);
            mgcc_ptr->ports[chid].line_state = STATE_RINGING;
        }
    }
    else if(strncmp(pkg,"xcg",3) == 0)
    {
        if(strncmp(sig,"spec",4) == 0) //ringing tone
        {
            //rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, sigRing, DSPCODEC_TONEDIRECTION_LOCAL)
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CUSTOM_TONE1, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
            mgcc_ptr->ports[chid].line_state = STATE_RINGING;
        }
        else if(strncmp(sig,"roh",3) == 0) //offhook warning tone
        {
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_OFFHOOKWARNING, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
        }
    }
    else if(strncmp(pkg,"alert",5) == 0)
    {
        if(strncmp(sig,"ri",2) == 0) //ringing tone
        {
#if defined(CONFIG_RTK_VOIP_TR104)
            if (tr104_e8c_autotest->status != E8C_TEST_START)
#endif

                rtk_SetRingFxs(chid, sigRing);
            mgcc_ptr->ports[chid].line_state = STATE_RINGING;
        }
    }
    else if(strncmp(pkg,"cg",2) == 0)
    {
        if(strncmp(sig,"rt",2) == 0) //Ringing Tone(ring back tone)
        {
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_RINGING, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);

        }
        else if(strncmp(sig,"bt",2) == 0) //Busy Tone
        {
#if defined(CONFIG_RTK_VOIP_TR104)
            if (tr104_e8c_autotest->status > E8C_TEST_INIT && tr104_e8c_autotest->status < E8C_TEST_FINISH) {
                printf("\n[Auto Call Test] Received onhook/busy from callee! Test finished!\n");
                tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
                rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 100);
            }
#endif
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        else if(strncmp(sig,"dt",2) == 0) //Dial Tone
        {
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        else if(strncmp(sig,"ct",2) == 0) //Congestion Tone
        {
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CONGESTION, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        else if(strncmp(sig,"wt",2) == 0) //Warning Tone
        {
            rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_OFFHOOKWARNING, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
        }
        else if(strncmp(sig,"cw",2) == 0) //Call Waiting Tone
        {

            if(sigRing)
            {
                rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CALL_WAITING, sigRing, DSPCODEC_TONEDIRECTION_LOCAL);
                rcm_timerCancel(mgcc_ptr->call_waiting_timer);
#ifdef CALL_WAITING_CONTINUE

                rcm_timerLaunch(mgcc_ptr->call_waiting_timer, 60*1000);
#else
                rcm_timerLaunch(mgcc_ptr->call_waiting_timer, 20*1000);
#endif
            }
        }
    }
    else if(strncmp(pkg,"andisp",6) == 0)
    {
        uint32 tmp=-1;
        if(strncmp(sig,"dwa",3) == 0) //Ringing Tone(ring back tone)
        {
            if(strncmp(val,"NULL",4) != 0)
            {
                //display cid and play tone according to line state
                switch(mgcc_ptr->ports[chid].line_state)
                {
                case STATE_IDLE:
                    /* display caller ID and ring fxs */
                    if (sigRing)
                    {
                        parse_SDMF(val, num, sizeof(num), dateTime, sizeof(dateTime));
                        if((g_pVoIPCfg->ports[0].caller_id_mode&7) == CID_DTMF)
                        {
                            mg_print("### DTMF mode ###\n");
                            rtk_GenDtmfCid(chid, num);
                        }
                        else /*FSK*/
                        {
                            mg_print("### FSK mode ###\n");
                            send_fskCID(chid,num,dateTime);
                        }
                    }
                    mgcc_ptr->ports[chid].line_state = STATE_RINGING;
                    break;

            case STATE_CONNECTED:
                /* display caller ID and play call waiting tone */

                parse_SDMF(val, num, sizeof(num), dateTime, sizeof(dateTime));
                if((g_pVoIPCfg->ports[0].caller_id_mode&7) == CID_DTMF)
                {
                    mg_print("### Call Waiting DTMF mode ###\n");
                    rcm_tapi_Gen_CID_And_FXS_Ring(chid, 0/*DTMF*/, num, dateTime, NULL, 1/*type2*/, 0);
                }
                else
                {
                    mg_print("### Call Waiting FSK mode ###\n");
                    rcm_tapi_Gen_CID_And_FXS_Ring(chid, 1/*fsk*/, num, dateTime, NULL, 1/*type2*/, 0);

                    // wait fsk caller id done, then send call waiting tone
                    //int32 tmp=-1;
                    do
                    {
                        if(rcm_tapi_GetFskCIDState(chid, &tmp) != 0)
                            break;
                        usleep(50000);  // 50ms
                    }
                    while (tmp);
                }
                    // call waiting tone

                rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CALL_WAITING, 1, DSPCODEC_TONEDIRECTION_LOCAL);
                rcm_timerCancel(mgcc_ptr->call_waiting_timer);
#ifdef CALL_WAITING_CONTINUE
                rcm_timerLaunch(mgcc_ptr->call_waiting_timer, 60*1000);
#else
                rcm_timerLaunch(mgcc_ptr->call_waiting_timer, 20*1000);
#endif
                break;

                    default:
                        break;
                }
            }
            else
            {
                if (!sigRing)
                {
                    mg_print("### Turn off ring ###\n");
                    rtk_StopCid(chid,2);
                    rtk_SetRingFxs(chid, 0);
                }
            }
        }
    }

    return 0;
}

void RtpSetAttr(int chid, char *attr)
{
    char *a = NULL, *b = NULL, *c = NULL;
    char str[8];
    int i = 0;
    int pt = 0;
    voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[chid];

    if (attr == NULL || strlen(attr) == 0)
        return;

    mg_print("chid = %d, attr = %s\n", chid, attr);

    /* find rtpmap then set */
    a = strstr(attr, "rtpmap");
    if (!a)
        return;
    else
    {
        b = strstr(a, ":");
        if (!b)
            return;

        c = strstr(b, " ");
        if (!c)
            return;

        for (i=0; i<=(c-b); i++) {
            b++;
            str[i] = *b;
        }

        str[i] = '\0';
        pt = atoi(str);
        voip_ptr->dtmf_2833_pt = (unsigned short)pt;

        mg_print("dtmf_2833_pt = %d\n", voip_ptr->dtmf_2833_pt);
    }
}

int updateRtp(int chid,int l_pt, int r_pt, char *address, int lport, int rport)
{
    //rtp_config_t rtp_config;
    //payloadtype_config_t codec_config;
    //rtcp_config_t rtcp_config;
    voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[chid];

    if (NULL == address || strlen(address) == 0 || rport == 0 )
    {
        mg_print("################# [updateRtp] invalid remote IP=%s, remote port=%d\n" , address , rport);
        return 1;
    }

    rtk_SetRtpSessionState(chid, 0, rtp_session_inactive); //stop rtp first
    	// set rtp session
	rtp_config.chid = chid;
	rtp_config.sid = 0;
	rtp_config.isTcp = 0;		// use udp
	//rtp_config.extIp = inet_addr("127.0.0.1");
	rtp_config.extIp = inet_addr(mgcc_ptr->bound_voip_ipaddress);
	rtp_config.remIp = inet_addr(address);

	//struct in_addr ip_addr;
	//char remoteip[16];
	//memcpy(&ip_addr , &rtp_config.remIp , sizeof(struct in_addr));
	//(remoteip, inet_ntoa(ip_addr) , 16);
	mg_print("################# [updateRtp] local_pt=%d, remote_pt=%d\n" , l_pt , r_pt);
    mg_print("################# [updateRtp] remote IP=%s, remote port=%d\n" , address , rport);

	rtp_config.extPort = htons(lport);
	rtp_config.remPort = htons(rport);
#ifdef SUPPORT_VOICE_QOS
	rtp_config.tos = 0;
#endif
	if(voip_ptr->dtmf_mode == 0)  //0:rfc2833  1: sip info  2: inband  3: DTMF delete
	{
		printf("###### RFC2833 payload type=%d\n",voip_ptr->dtmf_2833_pt);
		rtp_config.rfc2833_payload_type_local = voip_ptr->dtmf_2833_pt;
		rtp_config.rfc2833_payload_type_remote = voip_ptr->dtmf_2833_pt;
	}
	else
	{
		rtp_config.rfc2833_payload_type_local = 0;		// use in-band
		rtp_config.rfc2833_payload_type_remote = 0;		// use in-band
	}
	//rtp_config.rfc2833_fax_modem_pt_local = 0;		// use in-band
	//rtp_config.rfc2833_fax_modem_pt_remote = 0;		// use in-band
	rtp_config.rfc2833_fax_modem_pt_local = voip_ptr->fax_modem_2833_pt;
	rtp_config.rfc2833_fax_modem_pt_remote = voip_ptr->fax_modem_2833_pt;
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
	//rtk_SetRtpConfig(&rtp_config);

	// set RTCP session
	rtcp_config.chid			= rtp_config.chid;
	rtcp_config.sid				= rtp_config.sid;
	rtcp_config.remIp			= rtp_config.remIp;
	rtcp_config.remPort			= rtp_config.remPort + 1;
	rtcp_config.extIp			= rtp_config.extIp;
	rtcp_config.extPort			= rtp_config.extPort + 1;
#ifdef SUPPORT_VOICE_QOS
	rtcp_config.tos				= rtp_config.tos;
#endif
	//rtcp_config.txInterval		= 500;		// send rtcp every 500ms
	//rtcp_config.enableXR		= 1;
	rtcp_config.txInterval = voip_ptr->RTCP_Interval * 1000;/*ms*/
	rtcp_config.enableXR = voip_ptr->RTCP_XR;
#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	rtcp_config.ipv6			= 0;
#endif
	//rtk_SetRtcpConfig(&rtcp_config);

	// set rtp payload, and other session parameters.
	codec_config.chid = chid;
	codec_config.sid = 0;
	codec_config.local_pt = l_pt;
	codec_config.remote_pt = r_pt;
	codec_config.uLocalPktFormat = l_pt;
	codec_config.uRemotePktFormat = r_pt;
	codec_config.local_pt_vbd = rtpPayloadUndefined;
	codec_config.remote_pt_vbd = rtpPayloadUndefined;
	codec_config.uPktFormat_vbd = rtpPayloadUndefined;
	codec_config.nG723Type = 0;
	codec_config.nLocalFramePerPacket = 2;
	codec_config.nRemoteFramePerPacket = 2;
	codec_config.nFramePerPacket_vbd = 1;
	codec_config.bVAD = voip_ptr->vad;
	codec_config.bPLC = voip_ptr->PLC;
	codec_config.nJitterDelay = voip_ptr->jitter_delay;
	codec_config.nMaxDelay = voip_ptr->maxDelay;
	codec_config.nMaxStrictDelay = ( g_SystemParam.max_strict_delay ?
									g_SystemParam.max_strict_delay : 1000 );
	codec_config.nJitterFactor = voip_ptr->jitter_factor;
	codec_config.nG723Type = voip_ptr->g7231_rate;
	codec_config.nG726Packing = voip_ptr->g726_packing;
	//codec_config.bVAD = 0;
	//codec_config.bPLC = 1;
	//codec_config.nJitterDelay = 4;
	//codec_config.nMaxDelay = 13;
	//codec_config.nMaxStrictDelay = 10000;
	//codec_config.nJitterFactor = 7;
	//codec_config.nG726Packing = 2;//pack right
	if ((codec_config.uLocalPktFormat == rtpPayloadG722) ||
	    (codec_config.uRemotePktFormat == rtpPayloadG722) )
		codec_config.nPcmMode = 3;	// wide-band
	else
		codec_config.nPcmMode = 2;	// narrow-band
	rtk_SetRtpConfig(&rtp_config);
	rtk_SetRtcpConfig(&rtcp_config);
	rtk_SetRtpPayloadType(&codec_config);
	return 0;
}

int RtpHandle(int chid, char *address, int lport, int rport, int mode)
{
	//rtp_config_t rtp_config;
	//payloadtype_config_t codec_config;
	//rtcp_config_t rtcp_config;

	if(chid < 0)
	{
		mg_print("Invalid chid %d\n",chid);
		return -1;
	}

    //rtk_SetRtpConfig(&rtp_config);
    //rtk_SetRtcpConfig(&rtcp_config);
    //rtk_SetRtpPayloadType(&codec_config);

	if(mode == SDPMODE_INACTIVE || mode == SDPMODE_NOTSET)
	{
		rtk_SetRtpSessionState(chid, 0, rtp_session_inactive);
        memset(&rtp_config, 0, sizeof(rtp_config_t));
        memset(&codec_config, 0, sizeof(payloadtype_config_t));
        memset(&rtcp_config, 0, sizeof(rtcp_config_t));

		mg_print("################# Disable ch %d RTP\n" , chid);
        update_state(chid, STATE_IDLE);
		return 0;
	}

    if((mode == SDPMODE_SENDRECV) && g_hook_state)
	{
		mg_print("################# Start ch %d RTP\n" , chid);
        update_state(chid, STATE_CONNECTED);
	}

	if(lport == 0 || rport == 0 || address == NULL)
	{ //update SDP mode only
		mg_print("################# set ch %d RTP mode %d\n" , chid,mode);
		rtk_SetRtpSessionState(chid, 0, mode);
		return 0;
	}
#if 0
	// set rtp session
	rtp_config.chid = chid;
	rtp_config.sid = 0;
	rtp_config.isTcp = 0;		// use udp
	//rtp_config.extIp = inet_addr("127.0.0.1");
	rtp_config.extIp = inet_addr(mgcc_ptr->bound_voip_ipaddress);
	rtp_config.remIp = inet_addr(address);

	struct in_addr ip_addr;
	char remoteip[16];
	memcpy(&ip_addr , &rtp_config.remIp , sizeof(struct in_addr));
	strncpy(remoteip, inet_ntoa(ip_addr) , 16);
	printf("#################%s %s\n" , remoteip , inet_ntoa(ip_addr));

	rtp_config.extPort = htons(lport);
	rtp_config.remPort = htons(rport);
#ifdef SUPPORT_VOICE_QOS
	rtp_config.tos = 0;
#endif
	rtp_config.rfc2833_payload_type_local = 0;		// use in-band
	rtp_config.rfc2833_payload_type_remote = 0;		// use in-band
	rtp_config.rfc2833_fax_modem_pt_local = 0;		// use in-band
	rtp_config.rfc2833_fax_modem_pt_remote = 0;		// use in-band
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
	rtcp_config.chid			= rtp_config.chid;
	rtcp_config.sid				= rtp_config.sid;
	rtcp_config.remIp			= rtp_config.remIp;
	rtcp_config.remPort			= rtp_config.remPort + 1;
	rtcp_config.extIp			= rtp_config.extIp;
	rtcp_config.extPort			= rtp_config.extPort + 1;
#ifdef SUPPORT_VOICE_QOS
	rtcp_config.tos				= rtp_config.tos;
#endif
	rtcp_config.txInterval		= 500;		// send rtcp every 500ms
	rtcp_config.enableXR		= 1;
#if 1 //#ifdef CONFIG_RTK_VOIP_IPV6_SUPPORT
	rtcp_config.ipv6			= 0;
#endif
	rtk_SetRtcpConfig(&rtcp_config);

	// set rtp payload, and other session parameters.
	codec_config.chid = chid;
	codec_config.sid = 0;
	codec_config.local_pt = 0;
	codec_config.remote_pt = 0;
	codec_config.uLocalPktFormat = rtpPayloadPCMU;
	codec_config.uRemotePktFormat = rtpPayloadPCMU;
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
#endif
	rtk_SetRtpSessionState(chid, 0, rtp_session_sendrecv);

	return 0;
}

void mgcc_abort(int sig)
{
	mg_print("mgcc: abort\n");
	app_close(&mgcc);

	exit(0);
}

int keep_alive_if(void)
{
    struct sockaddr_in myaddr;   /* address that client uses */


     /*  Get a socket into UDP
     */
    if ((keep_alive_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("socket failed!");
        exit(1);
    }
    /*
     * Bind to an arbitrary return address.
     */
    bzero((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if (bind(keep_alive_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0) {
        perror("bind failed!");
        exit(1);
    }
    /*
     * Fill in the server's UDP/IP address
     */
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(g_pVoIPCfg->mg_port);
    servaddr.sin_addr.s_addr=inet_addr(mgcc_ptr->bound_voip_ipaddress);

    return 0;
}

void keep_alive_send(void)
{
    int size;    /* the length of servaddr */
    int ret;
    char sendbuf[] = "!/1 <KEEP_ALIVE>:2944 T=139473134{C=-{AV=ROOT{AT{}}}}";

    //printf("len sendbuf =%d\n",strlen(sendbuf));
    //printf("[keep_alive_send] send =%s\n",sendbuf);
    size = sizeof(servaddr);
    ret = sendto(keep_alive_fd, sendbuf, strlen(sendbuf), 0, (struct sockaddr*)&servaddr, size);
    if (ret == -1) {
        perror("write to server error !");
    }

    rcm_timerLaunch(mgcc_ptr->keep_alive_timer,10*1000);
}

void mg_GetLineEvent(int chid, SIGNSTATE *keyin)
{
    VoipEventID veid;
    int8 Event, Energy;
    int16 Duration;
    uint32 mid = 0;
#ifdef CONFIG_RTK_VOIP_TR104
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);
#endif

    for (mid=0; mid<2; mid++)
    {
        rtk_GetRfc2833RxEvent(chid, mid, &veid);
        if (veid != VEID_NONE)
        {
            mg_print("RX RFC2833 Event 0x%08X, ch%d, mid%d\n", veid, chid, mid);

            if((veid >= VEID_RFC2833_RX_DTMF_0) && (veid <= VEID_RFC2833_RX_DTMF_POUND))
            {
                if (veid == VEID_RFC2833_RX_DTMF_STAR)
                    dtmf_code = '*';
                else if (veid == VEID_RFC2833_RX_DTMF_POUND)
                    dtmf_code = '#';
                else
                    dtmf_code = '0' + veid;
#ifdef CONFIG_RTK_VOIP_TR104
                if (tr104_e8c_autotest->enable == 1)
                    mg_tr104_Auto_Call_Test_Recode_DTMF(chid, (char) dtmf_code);    /* int to char */
#endif
            }
        }
    }

    rtk_GetDtmfEvent(chid, 1, &Event, &Energy, &Duration);
    if ((Event >= '0' && Event <= '9')|| (Event == '*') || (Event == '#')) {
        dtmf_code = Event;
        mg_print("Get DSP Event=0x%08X, ch%d, dtmf_code:%c\n", Event, chid, dtmf_code);
    }

    if (Event == 'E' ) {
        mg_print("dir=1, energey=%ddBFS, duration=%dms, dtmf_code=%c\n", Energy, Duration, (char)dtmf_code);
#ifdef CONFIG_RTK_VOIP_TR104
        if (tr104_e8c_autotest->enable == 1 && Energy > -20)                /* Energy > -20, filter the local dtmf */
            mg_tr104_Auto_Call_Test_Recode_DTMF(chid, (char) dtmf_code);    /* int to char */
#endif
    }

    rtk_GetFxsEvent(chid, keyin);
    if (*keyin >= SIGN_PULSE_KEY0 && *keyin <= SIGN_PULSE_KEY9)
        *keyin = *keyin - SIGN_PULSE_KEY0;
}

void mg_send_dtmf(int chid, uint32 ssid, SIGNSTATE digit)	// hc$ DTMF mode process
{
    DSPCODEC_TONE nTone;
    voipCfgPortParam_t *voip_ptr = &g_pVoIPCfg->ports[chid];
#ifdef CONFIG_RTK_VOIP_TR104
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);
#endif

    mg_print("%s, chid:%d, ssid:%d, dtmf_mode:%d\n", __func__, chid, ssid, voip_ptr->dtmf_mode);

    if (digit < SIGN_KEY0 || digit > SIGN_HASH) {
        mg_print("%s, unknown digit:%d\n", __func__, digit);
        return;
    }

    if (voip_ptr->dtmf_mode == DTMF_RFC2833) {
        rtk_SetRtpRfc2833(chid, ssid, digit, 200);
    }
    else if (voip_ptr->dtmf_mode == DTMF_INBAND) {
        nTone = digit;
#if defined (CONFIG_RTK_VOIP_TR104) && defined (CONFIG_E8B)
        if (tr104_e8c_autotest->status == E8C_TEST_DIAL_DTMF)
            rtk_SipPlayToneRemote(chid, ssid, nTone, 200);
#endif
    }
    else
        mg_print("%s, unknown dtmf mode:%d\n", __func__, voip_ptr->dtmf_mode);
}

#ifdef CONFIG_RTK_VOIP_TR104
void MgSendResponseToCwmp(cwmpEvtMsg *evtMsg)
{
	int i, sendResponse=0;

	mg_print("MgSendResponseToCwmp\n");

	switch(cwmpEvtMsgGetEvent(evtMsg))
	{
		case EVT_VOICEPROFILE_LINE_GET_STATUS:
			for (i=0; i<g_MaxVoIPPorts; i++)
			{
				/* read SIP IMS server setting ,from SUBSCRIBE/NOTIFY*/
				evtMsg->voiceProfileLineStatusMsg[i].featureFlags=mg_tr104_st.voiceProfileLineStatusMsg[i].featureFlags;
				evtMsg->voiceProfileLineStatusMsg[i].IADDiagnosticsState=mg_tr104_st.voiceProfileLineStatusMsg[i].IADDiagnosticsState;
				evtMsg->voiceProfileLineStatusMsg[i].TestServer=mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer;
				evtMsg->voiceProfileLineStatusMsg[i].RegistResult=mg_tr104_st.voiceProfileLineStatusMsg[i].RegistResult;
				evtMsg->voiceProfileLineStatusMsg[i].Reason=mg_tr104_st.voiceProfileLineStatusMsg[i].Reason;
				memcpy(&evtMsg->PoorQuality, &mg_tr104_st.PoorQuality, MAX_POORQUALITYLIST_COUNT*sizeof(E8C_PoorQuality));
				evtMsg->voiceProfileLineStatusMsg[i].Voip_port_session_type=mg_tr104_st.voiceProfileLineStatusMsg[i].Voip_port_session_type;
				strcpy(evtMsg->voiceProfileLineStatusMsg[i].last_register_error, mg_tr104_st.voiceProfileLineStatusMsg[i].last_register_error);
				mg_print("[Register Test GET] get voiceProfileLineStatusMsg port %d\n", i);
				mg_print("[Register Test GET]IADDiagnosticsState is %d\n",mg_tr104_st.voiceProfileLineStatusMsg[i].IADDiagnosticsState);
				mg_print("[Register Test GET]TestServer is %d\n",mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer);
				mg_print("[Register Test GET]RegistResult is %d\n",mg_tr104_st.voiceProfileLineStatusMsg[i].RegistResult);
				mg_print("[Register Test GET]Reason is %d\n",mg_tr104_st.voiceProfileLineStatusMsg[i].Reason);
				mg_print("last_register_error=%s\n", mg_tr104_st.voiceProfileLineStatusMsg[i].last_register_error);

				/* copy tr104 auto test result to evtMsg*/
				memcpy(&evtMsg->e8c_autotest[i], &(mg_tr104_st.e8c_autotest[i]), sizeof(LinphoneTR104Test_t));
				mg_print("e8c_autotest.enable is %d\n",mg_tr104_st.e8c_autotest[i].enable);
				mg_print("e8c_autotest.callednumber is %s\n",mg_tr104_st.e8c_autotest[i].callednumber);
				mg_print("e8c_autotest.TestType is %d\n",mg_tr104_st.e8c_autotest[i].TestType);
				mg_print("e8c_autotest.TestSelector is %d\n",mg_tr104_st.e8c_autotest[i].TestSelector);
				mg_print("e8c_autotest.DailDTMFConfirmEnable is %d\n",mg_tr104_st.e8c_autotest[i].DailDTMFConfirmEnable);
				mg_print("e8c_autotest.DailDTMFConfirmNumber is %s\n",mg_tr104_st.e8c_autotest[i].DailDTMFConfirmNumber);
			}

			sendResponse=1;
			cwmpEvtMsgSetEvent(evtMsg, EVT_VOICEPROFILE_LINE_GET_STATUS);

			break;
		case EVT_VOICEPROFILE_LINE_SET_STATUS:
			/* check tr104 auto */
			memcpy(&mg_tr104_st, evtMsg, sizeof(cwmpEvtMsg));

			for (i=0; i<g_MaxVoIPPorts; i++)
			{
				/* E8C Diagnosis Register Test */
				printf("[Register Test SET]get voiceProfileLineStatusMsg port %d\n", i);
				printf("[Register Test SET]IADDiagnosticsState is %d\n", mg_tr104_st.voiceProfileLineStatusMsg[i].IADDiagnosticsState);
				printf("[Register Test SET]TestServer is %d\n", mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer);
				printf("[Register Test SET]RegistResult is %d\n", mg_tr104_st.voiceProfileLineStatusMsg[i].RegistResult);
				printf("[Register Test SET]Reason is %d\n", mg_tr104_st.voiceProfileLineStatusMsg[i].Reason);

				if (mg_tr104_Register_Test_Enable(i))
				{
					if (mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer == 1)//main proxy
					{
						if (strcmp(g_pVoIPCfg->mgc_ip, "") != 0) {
							printf("tr104 [Register Test SET] Main proxy\n");
							rcm_timerCancel(mgcc_ptr->check_reg_timer);
							//mg_stop();
							mg_send_serviceChange();
						}
						else {
							mg_tr104_Register_Test_Finish(i, E8C_TEST_RESULT_FAIL, FAIL_REASON_DEST_UNREACHABLE);
						}
					}
					else if (mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer == 2)//backup proxy
					{
						if (strcmp(g_pVoIPCfg->bak_mgc_ip, "") != 0) {
							mg_print("tr104 [Register Test SET] backup proxy\n");
							rcm_timerCancel(mgcc_ptr->check_reg_timer);;
							//mg_stop();
							mg_send_serviceChange();
						}
						else {
							mg_tr104_Register_Test_Finish(i, E8C_TEST_RESULT_FAIL, FAIL_REASON_DEST_UNREACHABLE);
						}
					}
					else{
						mg_print("error [Register Test SET] = %d\n", mg_tr104_st.voiceProfileLineStatusMsg[i].TestServer);
					}
				}

				/* for special spec */
				if (strlen(mg_tr104_st.e8c_autotest[i].DailDTMFConfirmNumber) == 0)
					strcpy(mg_tr104_st.e8c_autotest[i].DailDTMFConfirmNumber, DIAL_DTMF_CONFIRM_NUMBER);

				/* E8C Diagnosis auto call test */
				mg_tr104_Auto_Call_Test_Start(i);
			}
			break;
		case EVT_VOICEPROFILE_LINE_SET_DIAGNOSTICS:
			sendResponse = 1;
			cwmpEvtMsgSetEvent(evtMsg, EVT_VOICEPROFILE_LINE_SET_DIAGNOSTICS);
			break;
		default:
			break;
	}

	if(sendResponse)
	{
		int sendSock = 0;
		struct sockaddr_un addr;

		printf("### sendResponse to CWMP Client ###\n");

		/* Prepare to send response back to cwmpClient */
		sendSock = socket(PF_LOCAL, SOCK_DGRAM, 0);
		bzero(&addr, sizeof(addr));
		addr.sun_family = PF_LOCAL;
		strncpy(addr.sun_path, CWMP_CLINET_PATH, sizeof(CWMP_CLINET_PATH));
		sendto(sendSock, (void*)evtMsg, cwmpEvtMsgSizeof(), 0, (struct sockaddr*)&addr, sizeof(addr));
		close(sendSock);
	}
}

void mg_Diagnosis_Auto_Test(int chid)
{
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    if (!e8c_autotest->enable)
        return;

    mg_tr104_Auto_Call_Test_Init(chid);

    if(e8c_autotest->TestSelector == SIMULATE_TEST)
    {
        printf("[Auto Call Test] Simulate Test\n");
        tr104_e8c_autotest->enable = e8c_autotest->enable;
        strcpy(tr104_e8c_autotest->callednumber, e8c_autotest->callednumber);
        tr104_e8c_autotest->TestType = e8c_autotest->TestType;
        tr104_e8c_autotest->TestSelector = e8c_autotest->TestSelector;
        tr104_e8c_autotest->DailDTMFConfirmEnable = e8c_autotest->DailDTMFConfirmEnable;
        strcpy(tr104_e8c_autotest->DailDTMFConfirmNumber, e8c_autotest->DailDTMFConfirmNumber);
    }
    else if (e8c_autotest->TestSelector == PHONE_CONN_TEST)
    {
        printf("[Auto Call Test] PhoneConnectivity Test\n");
        tr104_e8c_autotest->TestSelector = e8c_autotest->TestSelector;
        tr104_e8c_autotest->enable = e8c_autotest->enable;
    }

    mg_tr104_Auto_Call_Test_Start(chid);
    e8c_autotest->enable = 0;  //reset
}

void mg_tr104_Auto_Call_Test_Start(int chid)
{
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    if (!tr104_e8c_autotest->enable) {
        printf("[Auto Call Test] Disable, %s\n", __func__);
        return;
    }

    /* E8C Diagnosis auto call Test */
    printf("[Auto Call Test] Start\n");
    printf("\t callednumber:%s\n", tr104_e8c_autotest->callednumber);
    printf("\t TestType:%s\n", tr104_e8c_autotest->TestType ? "Callee" : "Caller");
    printf("\t TestSelector:%s\n", tr104_e8c_autotest->TestSelector ? "Simulate Test" : "PhoneConnectivity Test");
    printf("\t DailDTMFConfirmEnable:%d\n", tr104_e8c_autotest->DailDTMFConfirmEnable);
    printf("\t DailDTMFConfirmNumber:%s\n", tr104_e8c_autotest->DailDTMFConfirmNumber);

    if (tr104_e8c_autotest->TestSelector == SIMULATE_TEST)  /* SimulateTest */
    {
        if (tr104_e8c_autotest->TestType == 0) {    /* Caller */
            printf("\r\n[Auto Call Test] start test from Caller \n");
            tr104_e8c_autotest->status = E8C_TEST_START;
            rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 200);
            /* TBD */
        }
        else {
            printf("\r\n[Auto Call Test] start test from Callee \n");
            if (strcmp(g_pVoIPCfg->mgc_ip, "") == 0 || mgcc_ptr->ports[chid].reg_status != REGSTATUS_REG_SUCCESS) {
                /* not register success */
                mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLEE_NO_INCOMINGCALL, E8C_TEST_RESULT_FAIL, 0);
                return;
            }
            /* callee test case, waiting incoming call */
            tr104_e8c_autotest->status = E8C_TEST_WAIT_INCOMING;
            rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 30*1000);
        }
    }
    else if(tr104_e8c_autotest->TestSelector == PHONE_CONN_TEST)  /* PhoneConnectivity Test */
    {
        if (mgcc_ptr->ports[chid].line_state != STATE_IDLE)
            tr104_e8c_autotest->PhoneConnectivity = 1;      //fxs 0 already on usage, slic should plugin phone.
        else
            tr104_e8c_autotest->PhoneConnectivity = rtk_GetPortStatus(chid);    //hard code get port 0

        tr104_e8c_autotest->enable = 0;
        tr104_e8c_autotest->TestStatus = E8C_TEST_COMPLETE;     //Complete, finish test.

        printf("[Auto Call Test] Finish\n");
        printf("\t PhoneConnectivity:%s \n", tr104_e8c_autotest->PhoneConnectivity ? "Connected" : "Not Connected");

        if (e8c_autotest->testFrom == TEST_FROM_TR104)
        {
            printf("[Auto Call Test] Do event send back....\n");
            cwmpEvtMsgSetEvent(&mg_tr104_st, EVT_VOICEPROFILE_LINE_SET_DIAGNOSTICS);
            MgSendResponseToCwmp(&mg_tr104_st);
        }
        else
        {
            e8c_autotest->PhoneConnectivity = tr104_e8c_autotest->PhoneConnectivity;
            e8c_autotest->enable = tr104_e8c_autotest->enable;
            e8c_autotest->TestStatus = tr104_e8c_autotest->TestStatus;
        }
    }
}

void mg_tr104_Auto_Call_Test_Recode_DTMF(int chid, char digit)
{
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    if (!tr104_e8c_autotest->enable) {
        printf("[Auto Call Test] Disable, %s\n", __func__);
        return;
    }

    tr104_e8c_autotest->ReceiveDTMFNumber[tr104_e8c_autotest->receive_dtmf_index] = digit;
    tr104_e8c_autotest->receive_dtmf_index++;
    strcpy(tr104_e8c_autotest->Simulate_Status, "Receiving");

    if ((e8c_autotest->testFrom == TEST_FROM_WEB) ||
        (e8c_autotest->testFrom == TEST_FROM_DBUS))
    {
        strcpy(e8c_autotest->Simulate_Status, "Receiving");
    }

    printf("[Auto Call Test] ReceiveDTMFNumber = %s\n", tr104_e8c_autotest->ReceiveDTMFNumber);

    if (strlen(tr104_e8c_autotest->DailDTMFConfirmNumber) == strlen(tr104_e8c_autotest->ReceiveDTMFNumber))
    {
        tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
        rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 100);	//finish
        printf("\r\n[Auto Call Test] receive number length = dial number length\n");
    }
}

void mg_tr104_Auto_Call_Test_Init(int chid)
{
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    printf("[Auto Call Test] Init\n");

    tr104_e8c_autotest->Conclusion = 0;
    tr104_e8c_autotest->status = E8C_TEST_INIT;
    tr104_e8c_autotest->dtmfindex = 0;
    tr104_e8c_autotest->receive_dtmf_index = 0;
    tr104_e8c_autotest->FailedResponseCode = 0;
    tr104_e8c_autotest->CallerFailReason = CALLER_NONE;
    tr104_e8c_autotest->CalledFailReason = CALLEE_NONE;
    tr104_e8c_autotest->DailDTMFConfirmResult = 0;
    tr104_e8c_autotest->FailedResponseCode = 0;
    memset(tr104_e8c_autotest->ReceiveDTMFNumber, 0, TEST_STRING_LEN);
    bzero(tr104_e8c_autotest->Simulate_Status, TEST_STRING_LEN);
}

void mg_tr104_Auto_Call_Test_Finish(int chid, int callResult, int failReason, int DailDTMFConfirmResult, int FailedResponseCode)
{
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    if (!tr104_e8c_autotest->enable) {
        printf("[Auto Call Test] Disable, %s\n", __func__);
        return;
    }

    printf("[Auto Call Test] Finish\n");
    printf("\t chid:%d\n ", chid);
    printf("\t callResult:%s\n ", callResult ? "Fail" : "Success");
    printf("\t failReason:%d\n ", failReason);
    printf("\t DailDTMFConfirmResult:%s\n", DailDTMFConfirmResult ? "Fail" : "Success");
    printf("\t FailedResponseCode:%d\n", FailedResponseCode);

    tr104_e8c_autotest->enable = 0;
    tr104_e8c_autotest->TestStatus = E8C_TEST_COMPLETE; /* Complete */
    tr104_e8c_autotest->Conclusion = callResult;
    tr104_e8c_autotest->receive_dtmf_index = 0;
    tr104_e8c_autotest->DailDTMFConfirmResult = DailDTMFConfirmResult;
    tr104_e8c_autotest->FailedResponseCode = FailedResponseCode;
    strcpy(tr104_e8c_autotest->Simulate_Status, "Testend");

    tr104_e8c_autotest->status = E8C_TEST_FINISH; //finish
    if (tr104_e8c_autotest->TestType == 0) /* Caller */
    {
        tr104_e8c_autotest->CallerFailReason = failReason;
        tr104_e8c_autotest->CalledFailReason = CALLEE_NONE;
    }
    else
    {
        tr104_e8c_autotest->CallerFailReason = CALLER_NONE;
        tr104_e8c_autotest->CalledFailReason = failReason;
    }

    if (e8c_autotest->testFrom == TEST_FROM_TR104)  //from tr104
    {
        //send to cwmpclient. //Alex
        printf("[Auto Call Test] Do event send back....\n");
        cwmpEvtMsgSetEvent(&mg_tr104_st, EVT_VOICEPROFILE_LINE_SET_DIAGNOSTICS);
        MgSendResponseToCwmp(&mg_tr104_st);
    }
    else //from web or dbus
    {
        e8c_autotest->enable = tr104_e8c_autotest->enable;
        e8c_autotest->TestStatus = tr104_e8c_autotest->TestStatus;
        e8c_autotest->Conclusion = tr104_e8c_autotest->Conclusion;
        e8c_autotest->DailDTMFConfirmResult = tr104_e8c_autotest->DailDTMFConfirmResult;
        e8c_autotest->FailedResponseCode = tr104_e8c_autotest->FailedResponseCode;
        strcpy(e8c_autotest->Simulate_Status, tr104_e8c_autotest->Simulate_Status);
        e8c_autotest->CallerFailReason = tr104_e8c_autotest->CallerFailReason;
        e8c_autotest->CalledFailReason = tr104_e8c_autotest->CalledFailReason;
    }
}

void mg_tr104_Register_Test_Finish(int chid, int registResult, int reason)
{
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    voiceProfileLineStatus *tr104_voice_line_status = &(mg_tr104_st.voiceProfileLineStatusMsg[chid]);

    printf("[Register Test] Finish:\n");
    printf("\t chid=%d\n", chid);
    printf("\t registResult=%s\n", registResult ? "Fail" : "Success");
    printf("\t reason=%d\n", reason);

    tr104_voice_line_status->IADDiagnosticsState = E8C_TEST_COMPLETE;
    tr104_voice_line_status->RegistResult = registResult;
    tr104_voice_line_status->Reason = reason;

    if (e8c_autotest->testFrom == TEST_FROM_TR104)  //from tr104
    {
        printf("[Register Test] Do event send back....\n");
        cwmpEvtMsgSetEvent(&mg_tr104_st, EVT_VOICEPROFILE_LINE_SET_DIAGNOSTICS);
        MgSendResponseToCwmp(&mg_tr104_st);
    }
    else //from web or dbus
    {
        e8c_autotest->IADDiagnosticsState = E8C_TEST_COMPLETE;
        e8c_autotest->RegistResult = registResult;
        e8c_autotest->Reason = reason;
    }
}

int mg_tr104_Register_Test_Enable(int chid)
{
    voiceProfileLineStatus *tr104_voice_line_status = &(mg_tr104_st.voiceProfileLineStatusMsg[chid]);

    if (tr104_voice_line_status->IADDiagnosticsState == E8C_TEST_REQUESTED)
    {
        printf("[Register Test] Enable\n");
        return 1;
    }
    else
    {
        printf("[Register Test] Disable\n");
        return 0;
    }
}

/*______________________________________________________________________________
**	e8c_dtmf_test
**
**	descriptions: e8c_dtmf_test.
**	parameters: chid
**____________________________________________________________________________*/

#define NO_DTMF_WAITING_TIME (5*1000)//wait 5 sec
#define DTMF_WAITING_TIME (30*1000)
void mg_e8c_dtmf_test(void *arg)
{
    int chid = (int)arg; //*(int *)arg;
    int i = 0;
    SIGNSTATE digit;
    e8c_autocall_test_t *e8c_autotest = &(g_shmVoIPstate->e8c_autocall_test[chid]);
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);

    printf("[Auto Call Test] chid:%d, TestType:%s, status:%d\n",
           chid, tr104_e8c_autotest->TestType ? "Callee" : "Caller", tr104_e8c_autotest->status);

    switch (tr104_e8c_autotest->TestType)
    {
        case 0: //caller
            printf("[Auto Call Test] status:%d\n", tr104_e8c_autotest->status);
            if(tr104_e8c_autotest->status == E8C_TEST_START)
            {
                /* caller start test, step 1, offhook and dial number */
                /* TODO */
                mgccEventEx(mgcc_ptr->ports[chid].termName, SIGN_OFFHOOK);
                usleep(500000); //TODO, need to waiting for dt event

                printf("[Auto Call Test] callednumber:%s\n", tr104_e8c_autotest->callednumber);
                while (tr104_e8c_autotest->callednumber[i] != '\0') {
                    if (tr104_e8c_autotest->callednumber[i] == '*')
                        digit = SIGN_STAR;
                    else if (tr104_e8c_autotest->callednumber[i] == '#')
                        digit = SIGN_HASH;
                    else
                        digit = SIGN_KEY0 + tr104_e8c_autotest->callednumber[i] - '0';
                    mgccEventEx(mgcc_ptr->ports[chid].termName, digit);
                    i++;
                    usleep(100000);
                }

				/* offhook */
				printf("\r\n [Auto Call Test] try offhook, dial to %s\n", tr104_e8c_autotest->callednumber);
				tr104_e8c_autotest->status = E8C_TEST_WAIT_CALLEE_OFFHOOK;

				if (strcmp(g_pVoIPCfg->mgc_ip, "") == 0 || mgcc_ptr->ports[chid].reg_status != REGSTATUS_REG_SUCCESS){
					/* not register success */
					mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLER_NO_DIALTONE, E8C_TEST_RESULT_FAIL, 0);
					break;
				}
				rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 60*1000);// wait 60s callee accept call
				break;
			}
			else if (tr104_e8c_autotest->status == E8C_TEST_WAIT_CALLEE_OFFHOOK)
			{
				mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLER_NO_ANSWER, E8C_TEST_RESULT_FAIL, 0);
			}
			else if (tr104_e8c_autotest->status == E8C_TEST_DIAL_DTMF)
			{
				if (tr104_e8c_autotest->DailDTMFConfirmEnable == 0)
				{
					tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
					rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, NO_DTMF_WAITING_TIME); //wait 5 sec
					printf("[Auto Call Test] DailDTMFConfirmEnable disalbe \n");

				}
				else if (tr104_e8c_autotest->dtmfindex < strlen(tr104_e8c_autotest->DailDTMFConfirmNumber))
				{
					/* step2 , dial dtmf to called */
					if (tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] == '*')
						digit = SIGN_STAR;
					else if  (tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] == '#')
						digit = SIGN_HASH;
					else
						digit = SIGN_KEY0 + tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] - '0';

					printf("\r\n[Auto Call Test] auto dial dtmf %d\n", digit);

					mgccEventEx(mgcc_ptr->ports[chid].termName, digit);

					tr104_e8c_autotest->dtmfindex++;
					strcpy(tr104_e8c_autotest->Simulate_Status, "Connected");
					if ((e8c_autotest->testFrom == TEST_FROM_WEB)||
						(e8c_autotest->testFrom == TEST_FROM_DBUS))
					{
						strcpy(e8c_autotest->Simulate_Status, "Connected");
					}
					rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 1*1000); //every 1 sec, dial one digit
				}
				else
				{
					tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
					if (strlen(tr104_e8c_autotest->DailDTMFConfirmNumber) == 0)
						rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, NO_DTMF_WAITING_TIME); //wait 5 sec
					else
						rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, DTMF_WAITING_TIME);//wait 20 sec
				}
			}
			else if (tr104_e8c_autotest->status == E8C_TEST_CMPDTMF)
			{
				/* step3. no dtmf need dial, onhook it*/
				if (tr104_e8c_autotest->DailDTMFConfirmEnable) {
					printf("\r\n[Auto Call Test] e8c_autotest.ReceiveDTMFNumber is %s\n", tr104_e8c_autotest->ReceiveDTMFNumber);
					if (strcmp(tr104_e8c_autotest->DailDTMFConfirmNumber, tr104_e8c_autotest->ReceiveDTMFNumber) != 0) /* fail case */
						mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLEE_NONE, E8C_TEST_RESULT_FAIL, 0);
					else /* test ok */
						mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_SUCESS, CALLEE_NONE, E8C_TEST_RESULT_SUCESS, 0);
				}
				else {
					printf("[Auto Call Test] DailDTMFConfirmEnable is disable\n");
					mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_SUCESS, CALLEE_NONE, E8C_TEST_RESULT_SUCESS, 0);
				}

				printf("\r\n[Auto Call Test] auto onhook \n");
				rtk_SetDtmfCfg(chid, 0, 1);
				mgccEventEx(mgcc_ptr->ports[chid].termName, SIGN_ONHOOK);
			}
			break;
		case 1: //callee
			if(tr104_e8c_autotest->status == E8C_TEST_WAIT_INCOMING)
			{
				mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLEE_NO_INCOMINGCALL, E8C_TEST_RESULT_FAIL, 0);
			}
			else if(tr104_e8c_autotest->status == E8C_TEST_START)
			{
				rtk_SetDtmfCfg(chid, 1, 1);
				printf("\r\n [Auto Call Test] auto answer the call, line_state:%d \n", mgcc_ptr->ports[chid].line_state);

				if (mgcc_ptr->ports[chid].line_state == STATE_RINGING)
					mgccEventEx(mgcc_ptr->ports[chid].termName, SIGN_OFFHOOK);

				tr104_e8c_autotest->status = E8C_TEST_DIAL_DTMF; //start send dtmf
				rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 1*1500); //start send dtmf
			}
			else if (tr104_e8c_autotest->status == E8C_TEST_DIAL_DTMF)
			{
				printf("\r\n [Auto Call Test] DailDTMFConfirmEnable:%d\n", tr104_e8c_autotest->DailDTMFConfirmEnable);
				printf("\r\n [Auto Call Test] dtmfindex:%d\n", tr104_e8c_autotest->dtmfindex);

				if (tr104_e8c_autotest->DailDTMFConfirmEnable == 0)
				{
					tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
					rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, NO_DTMF_WAITING_TIME); //wait 20 sec
				}
				else if(tr104_e8c_autotest->dtmfindex < strlen(tr104_e8c_autotest->DailDTMFConfirmNumber))
				{
					if (tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] == '*')
						digit = SIGN_STAR;
					else if  (tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] == '#')
						digit = SIGN_HASH;
					else
						digit = SIGN_KEY0 + tr104_e8c_autotest->DailDTMFConfirmNumber[tr104_e8c_autotest->dtmfindex] - '0';

					printf("\r\n[Auto Call Test] auto dial dtmf %d \n", digit);

					mgccEventEx(mgcc_ptr->ports[chid].termName, digit);

					tr104_e8c_autotest->dtmfindex++;
					rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 1*1000); //every 1 sec, dial one digit
				}
				else//all dtmf sent finish, wait 20 sec receive remote dtmf
				{
					tr104_e8c_autotest->status = E8C_TEST_CMPDTMF;
					if (strlen(tr104_e8c_autotest->DailDTMFConfirmNumber) == 0)
						rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, NO_DTMF_WAITING_TIME); //wait 5 sec
					else
						rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, DTMF_WAITING_TIME);//wait 20 sec
				}
			}
			else if (tr104_e8c_autotest->status == E8C_TEST_CMPDTMF)
			{
                /* no dtmf need dial, onhook it*/
                printf("\r\n[Auto Call Test] auto onhook \n");
                if (tr104_e8c_autotest->DailDTMFConfirmEnable) {
                    printf("\r\n[Auto Call Test] e8c_autotest.ReceiveDTMFNumber is %s\n", tr104_e8c_autotest->ReceiveDTMFNumber);
                    if(strcmp(tr104_e8c_autotest->DailDTMFConfirmNumber, tr104_e8c_autotest->ReceiveDTMFNumber) != 0) /* fail case */
                        mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_FAIL, CALLEE_NONE, E8C_TEST_RESULT_FAIL, 0);
                    else  /* test ok */
                        mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_SUCESS, CALLEE_NONE, E8C_TEST_RESULT_SUCESS, 0);
                }
                else {
                    printf("[Auto Call Test] DailDTMFConfirmEnable is disable\n");
                    mg_tr104_Auto_Call_Test_Finish(chid, E8C_TEST_RESULT_SUCESS, CALLEE_NONE, E8C_TEST_RESULT_SUCESS, 0);
                }

				rtk_SetDtmfCfg(chid, 0, 1);
				mgccEventEx(mgcc_ptr->ports[chid].termName, SIGN_ONHOOK);
			}
			break;
		default:
			break;
	}
}
#endif

#if 1
int rcm_init_msg_if(void)
{
    struct sockaddr_un addr;
    int client_sock=0;
    int len;
    int retry=3;

    if (g_wrap_client > 0)
    {
        mg_print("[%s] wrapper socket was created, just return\n",__FUNCTION__);
        return 0;
    }

    //create socket
    if((client_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
       mg_print("open wrapper socket fail: %s\n", strerror(errno));
       goto fail;
    }

try_again:
    //connect to msg wrapper
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCK);
    len = sizeof(addr.sun_family) + strlen(addr.sun_path);
    if (connect(client_sock, (struct sockaddr *)&addr, len) < 0)
    {
        perror("connect");
        if (retry > 0)
        {
            retry--;
            sleep(1);
            goto try_again;
        }
        goto fail;
    }
    else
    {
        g_wrap_client = client_sock;
        return 0;
    }


fail:
    if (client_sock >= 0)
    {
        close(client_sock);
        return -1;
    }

    return 0;
}
#else
int rcm_init_msg_if()
{
	struct sockaddr_in server;
	int sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_port = htons(3555);
  inet_pton(AF_INET, "127.0.0.1", &server.sin_addr.s_addr);

  if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
      perror("connect");
      goto fail;
  }
  else
  {
      g_wrap_client = sock;
      return 0;
  }


fail:
    if (sock >= 0)
    {
        close(sock);
        return -1;
    }
}
#endif

int rcm_send_msg(char *msg)
{
	int ret;

	mg_print("[rcm_send_msg] %s\n", msg);

	ret = send(g_wrap_client, msg, strlen(msg), 0);
	if( ret == -1)
	{
		mg_print("send to wrapper socket fail: %s\n", strerror(errno));
	}

	return ret;
}

int rcm_msg_handler(void)
{
    char buffer[1024], tmpBuf[1024];
    char *argv[MAX_ARGC];
    char *p=NULL, *pch=NULL;
    char *delim = "\n";
    int len, argc;
    int chid=0;
#ifdef CONFIG_RTK_VOIP_TR104
    LinphoneTR104Test_t *tr104_e8c_autotest = &(mg_tr104_st.e8c_autotest[chid]);
#endif

    memset(argv, 0, sizeof(argv));
    memset(buffer, 0, sizeof(buffer));
    memset(tmpBuf, 0, sizeof(tmpBuf));

    if((len = recv(g_wrap_client, buffer, sizeof(buffer), 0)) < 0 ){
        mg_print("receive from wrapper socket fail: %s\n", strerror(errno));
    }
    memcpy(tmpBuf, buffer, sizeof(tmpBuf));
    //printf("[%s] %s @@@\n", __func__, buffer);

    pch = strtok(tmpBuf, delim);
    while (pch != NULL)
    {
        //printf ("%s\n",pch);

        if(strstr(pch, "Connection established"))
        {
            if(mg_init()<0)
            {
                mg_print("mg_init failed\n");
                update_and_display_registration_status( chid, mgcc_ptr, REGSTATUS_REG_FAIL );
                return -1;
            }
            if(mg_start()<0)
            {
                mg_print("mg_start failed\n");
                return -1;
            }

            timerStartFlag = 0;
            rcm_timerCancel(mgcc_ptr->check_reg_timer);
        }
        else if((p = strstr(pch, "UpdateBuildTime")))
        {
            mg_print("#### UpdateBuildTime\n");
            p += (strlen("UpdateBuildTime")+1);
            argc = ParseCommand(p, strlen(p), argv);

            strncpy(megacoBuildTime,argv[0],sizeof(megacoBuildTime));
            mg_print("megaco built time %s\n",megacoBuildTime);

        }
        else if(strstr(pch, "Registered"))
        {
            timerStartFlag = 0;
            rcm_timerCancel(mgcc_ptr->check_reg_timer);

            if(updateChid < g_MaxVoIPPorts)
            {
                mg_addTerm();

                /* Update registered status on web */
                update_and_display_registration_status( chid, mgcc_ptr, REGSTATUS_REG_SUCCESS );

                //update line state
                int i;
                for (i=0; i<g_MaxVoIPPorts; i++)
                {
                    update_state(i,STATE_IDLE);
                }
                rtk_Set_LED_Display( chid, 0, LED_ON );
            }
        }
        else if(strstr(pch, "{Registering}"))
        {
            if(reg_timeout) // second serviceChange
            {
                if (current_mgc == PRIMARY_MGC)
                {
                    mg_print("Registering ==> PRIMARY_MGC timeout\n");
                    if (strcmp(g_pVoIPCfg->bak_mgc_ip,"") == 0) //no backup MGC
                    {
                        mg_print("try  PRIMARY_MGC again\n");
                        //rcm_timerCancel(mgcc_ptr->check_reg_timer);
                        mg_stop();
                        rcm_timerLaunch(mgcc_ptr->check_reg_timer,1000); //1s
                    }
                    else
                    {
                        mg_print("try  PRIMARY_MGC  ==> SECONDARY_MGC, RegisterRetryInterval=%d\n",g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
                        //rcm_timerCancel(mgcc_ptr->check_reg_timer);
                        mg_stop();
                        rcm_timerLaunch(mgcc_ptr->check_reg_timer,g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
                    }
                }
                else
                {
                    mg_print("Registering ==> SECONDARY_MGC timeout\n");
                    //rcm_timerCancel(mgcc_ptr->check_reg_timer);
                    mg_stop();
                    rcm_timerLaunch(mgcc_ptr->check_reg_timer,g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
                    mg_print("Registering ==> SECONDARY_MGC timeout RegisterRetryInterval=%d\n",g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
                }
                reg_timeout = 0;
            }
            else
                reg_timeout = 1; //first serviceChange
            //if (timerStartFlag == 0)
            //{
            //    mg_print("Registering ==> rcm_timerLaunch\n");
            //    rcm_timerLaunch(mgcc_ptr->check_reg_timer,g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
            //    timerStartFlag=1;
            //}
            /* Update registered status on web */
            update_and_display_registration_status( chid, mgcc_ptr, REGSTATUS_REG_ING );
        }
        else if(strstr(pch, "Unregistered"))
        {
            /* Update registered status on web */
            update_and_display_registration_status( chid, mgcc_ptr, REGSTATUS_INIT );
            rtk_Set_LED_Display( chid, 0, LED_OFF );
        }
        else if(strstr(pch, "mgStop"))
        {
            //mgapp stop done, restart mg call control

            //init call control app
            app_init(&mgcc);

            if(mg_init()<0)
            {
                mg_print("mg_init failed\n");
                update_and_display_registration_status( chid, mgcc_ptr, REGSTATUS_REG_FAIL );
                return -1;
            }
            //start mg
            if(mg_start()<0)
            {
                mg_print("mg_start failed\n");
                return -1;
            }
        }
        else if((p = strstr(pch, "AddTerm")))
        {
#if defined(CONFIG_RTK_VOIP_TR104)
            /* ericchung  TR104 auto test , callee test*/
            if((tr104_e8c_autotest->enable == 1) &&
               (tr104_e8c_autotest->TestType == 1) && /* Callee */
                tr104_e8c_autotest->status == E8C_TEST_WAIT_INCOMING)
            {
                /* offhook to answer */
                printf("[Auto Call Test] start callee test off hook after 2 sec\n");
                tr104_e8c_autotest->status = E8C_TEST_START;
                rcm_timerLaunch(mgcc_ptr->ports[chid].e8c_tr104_timer, 2*1000); //after 2 sec start test.
            }
#endif
            mg_print("#### AddTerm\n");
            p += (strlen("AddTerm")+1);
            argc = ParseCommand(p, strlen(p), argv);
            mg_print("#### AddTerm argc=%d, argv[0]=%s, argv[1]=%s\n",argc, argv[0], argv[1]);
            if(argc == 2)
            {
                if(updateChid < g_MaxVoIPPorts)
                {
                    strncpy(mgcc_ptr->ports[updateChid].termName, argv[0], sizeof(mgcc_ptr->ports[updateChid].termName));
                    mg_print("ports[%d] termName %s\n",updateChid,mgcc_ptr->ports[updateChid].termName);
                    updateChid++;
                }
            }
            else
            {
                if(strncmp(argv[1],"rtp",3)==0)
                {
                    int free_id;

                    if((free_id = findFreeRtpTerm(chid, mgcc_ptr)) <0)
                    {
                        printf("No free rtp term\n");
                        return -1;
                    }
                    printf("#### AddTerm free_id=%d\n",free_id);
                    chid = getIdByName(argv[0],mgcc_ptr);
                    mgcc_ptr->ports[chid].rtp_term[free_id].chid = chid;
                    mgcc_ptr->ports[chid].rtp_term[free_id].active = TRUE;
                    strncpy(mgcc_ptr->ports[chid].rtp_term[free_id].termName,argv[1],128);
                }
            }
        }
        else if((p = strstr(pch, "DelTerm")))
        {
            int i;

            mg_print("#### DelTerm\n");
            p += (strlen("DelTerm")+1);
            argc = ParseCommand(p, strlen(p), argv);
            mg_print("#### DelTerm argc=%d, argv[0]=%s, argv[1]=%s\n",argc, argv[0], argv[1]);

            chid = getIdByName(argv[0],mgcc_ptr);
            for(i=0; i<2; i++)
            {
                if(strcmp(argv[1],mgcc_ptr->ports[chid].rtp_term[i].termName)==0)
                {
                    mg_print("#### DelTerm free %d,name=%s\n",i,argv[1]);
                    mgcc_ptr->ports[chid].rtp_term[i].active = FALSE;
                    strcpy(mgcc_ptr->ports[chid].rtp_term[i].termName,"");
                }
            }
        }
        else if((p = strstr(pch, "AddSig")))
        {
            mg_print("#### AddSig\n");
            p += (strlen("AddSig")+1);
            argc = ParseCommand(p, strlen(p), argv);
            mg_print("argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s, argv[3]=%s\n",argc, argv[0], argv[1], argv[2], argv[3]);
            /* argv 0 ==> term name
               argv 1 ==> package
               argv 2 ==> signal
               argv 3 ==> val
            */
            chid = getIdByName(argv[0],mgcc_ptr);
            //chid=0;
            if(chid < 0)
            {
                mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
                return -1;
            }
            SigHandle(chid, argv[1], argv[2], argv[3], ADDSIG);
        }
        else if((p = strstr(pch, "DelSig")))
        {
            mg_print("#### DelSig\n");
            p += (strlen("DelSig")+1);
            argc = ParseCommand(p, strlen(p), argv);
            /* argv 0 ==> term name
               argv 1 ==> ??
               argv 2 ==> package
               argv 3 ==> signal*/
            chid = getIdByName(argv[0],mgcc_ptr);
            //chid=0;
            if(chid < 0)
            {
                mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
                return -1;
            }
            SigHandle(chid, argv[2], argv[3],"NULL", DELSIG);
        }
        else if((p = strstr(pch, "AddEv")))
        {
            mg_print("#### AddEv\n");
            p += (strlen("AddEv")+1);
            argc = ParseCommand(p, strlen(p), argv);
            mg_print("argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s\n",argc, argv[0], argv[1], argv[2]);
            /* argv 0 ==> term name
               argv 1 ==> package
               argv 2 ==> event*/
            chid = getIdByName(argv[0],mgcc_ptr);

            if(chid < 0)
            {
                mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
                return -1;
            }
            //update report digit method
            if (strcmp(argv[1],"dd")==0 && strcmp(argv[2],"ce")==0)
            {
                mgcc_ptr->ports[chid].digitReport = DIGITMODE_DD;
            }
            else if (strcmp(argv[1],"tonedet")==0 && strcmp(argv[2],"std")==0)
            {
                mgcc_ptr->ports[chid].digitReport = DIGITMODE_TONEDET;
            }
        }
        else if((p = strstr(pch, "SetRtp")))
        {
            mg_print("#### SetRtp\n");
            p += (strlen("SetRtp")+1);
            argc = ParseCommand(p, strlen(p), argv);
            /* argv 0 ==> term name
               argv 1 ==> address
               argv 2 ==> local port
               argv 3 ==> remote port
               argv 4 ==> mode*/
            //chid = getIdByName(argv[0],mgcc_ptr);
            //mg_print("===> chid %d\n",chid);
            chid=0;
            //if(chid < 0)
            //{
            //	mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
            //	return 0;
            //}
            if(argc == 6)
            {
                RtpSetAttr(chid,argv[5]);
            }
            else
                RtpHandle(chid, argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        }
        else if((p = strstr(pch, "StartT38")))
        {
            mg_print("#### StartT38\n");
            p += (strlen("StartT38")+1);
            argc = ParseCommand(p, strlen(p), argv);
            /* argv 0 ==> term name
               argv 1 ==> address
               argv 2 ==> local port
               argv 3 ==> remote port
            */
            chid = getIdByName(argv[0],mgcc_ptr);
            mg_print("===> chid %d\n",chid);

            if(chid < 0)
            {
                mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
                return 0;
            }
            rtk_t38_start(chid, argv[1], atoi(argv[2]), atoi(argv[3]));
        }
        else if((p = strstr(pch, "UpdateRtp")))
        {
            mg_print("#### UpdateRtp\n");
            p += (strlen("UpdateRtp")+1);
            argc = ParseCommand(p, strlen(p), argv);
            /* argv 0 ==> term name
               argv 1 ==> local payload type
               argv 2 ==> remote payload type
               argv 3 ==> address
               argv 4 ==> local port
               argv 5 ==> remote port
               argv 6 ==> attribute string*/
            //chid = getIdByName(argv[0],mgcc_ptr);
            //mg_print("===> chid %d\n",chid);
            chid=0;
            //if(chid < 0)
            //{
            //	mg_print("[%s] Invalid term name!!\n",__FUNCTION__);
            //	return 0;
            //}

            RtpSetAttr(chid,argv[6]);
            updateRtp(chid,atoi(argv[1]),atoi(argv[2]),argv[3],atoi(argv[4]),atoi(argv[5]));
        }
        else
        {
            if((p = strstr(pch, "test:Log")))
            {
                p += (strlen("test:Log")+1);
                mg_print("[megaco stack]: %s\n",p);
            }
        }
        pch = strtok (NULL, delim);
    }

    return 0;
}

int mgccEventEx(char *termName, SIGNSTATE pval)
{
	char cmd[64];
	char pkgStr[32];
	int ret=0;
	int chid;

	if(pval < SIGN_KEY0 || pval > SIGN_FLASHHOOK)
		return -1;

	chid = getIdByName(termName, mgcc_ptr);
	if(chid < 0)
	{
		mg_print("[%s] Invalid chid %d\n",chid);
	}

	if (mgcc_ptr->ports[chid].digitReport == DIGITMODE_TONEDET)
	{
		sprintf(pkgStr,"{tonedet} {std} {tid}");
	}
	else if (mgcc_ptr->ports[chid].digitReport == DIGITMODE_DD)
	{
		sprintf(pkgStr,"{dd}");
	}
	else
		mg_print("[mgccEventEx] Invalid digit method!!\n");
#if 0
	if (mgcc_ptr->ports[chid].line_state == STATE_CONNECTED)
	{
		sprintf(pkgStr,"{tonedet} {std} {tid}");
	}
	else
	{
		sprintf(pkgStr,"{dd}");
	}
#endif

	if (mgcc_ptr->ports[chid].line_state == STATE_CONNECTED && (pval >= SIGN_KEY0 && pval <= SIGN_HASH))
	{
		mg_print("send dtmf %d\n", pval);
		mg_send_dtmf(chid, 0, pval);
	}

	switch(pval)
	{
	case SIGN_ONHOOK:
		rtk_Set_LED_Display( chid, 0, LED_ON );
		rtk_SetPlayTone(0, 0, DSPCODEC_TONE_CALL_WAITING, 0, DSPCODEC_TONEDIRECTION_LOCAL);
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} {al} {on}\n",termName);
		g_hook_state = 0;
		mg_print("[SIGN_ONHOOK] g_hook_state=%d\n",g_hook_state);
		break;

	case SIGN_OFFHOOK:
		rtk_Set_LED_Display( chid, 0, LED_BLINKING );
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} {al} {of}\n",termName);
		g_hook_state = 1;
		mg_print("[SIGN_OFFHOOK] g_hook_state=%d\n", g_hook_state);
		break;

	case SIGN_FLASHHOOK:
		//sprintf(cmd,"callCmd,{term.SendEvent} {`0`%s} {al} {fl}\n",findActiveRtpTerm(chid));
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} {al} {fl}\n",termName);
		g_hook_state = !g_hook_state;
		mg_print("[SIGN_FLASHHOOK] g_hook_state=%d\n",g_hook_state);
		break;

	case SIGN_KEY1:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d1}\n",termName,pkgStr);
		break;
	case SIGN_KEY2:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d2}\n",termName,pkgStr);
		break;
	case SIGN_KEY3:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d3}\n",termName,pkgStr);
		break;
	case SIGN_KEY4:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d4}\n",termName,pkgStr);
		break;
	case SIGN_KEY5:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d5}\n",termName,pkgStr);
		break;
	case SIGN_KEY6:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d6}\n",termName,pkgStr);
		break;
	case SIGN_KEY7:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d7}\n",termName,pkgStr);
		break;
	case SIGN_KEY8:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d8}\n",termName,pkgStr);
		break;
	case SIGN_KEY9:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d9}\n",termName,pkgStr);
		break;
	case SIGN_KEY0:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {d0}\n",termName,pkgStr);
		break;
	case SIGN_STAR:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {ds}\n",termName,pkgStr);
		break;
	case SIGN_HASH:
		sprintf(cmd,"callCmd,{term.SendEvent} {%s} %s {do}\n",termName,pkgStr);
		break;
	default:
		break;
	}

	ret = rcm_send_msg(cmd);
	return ret;

}

void send_log_cmd(int mask)
{
    char tmpCmd[128];

    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,PORT} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SELECT} {%d}\n",mask);
    rcm_send_msg(tmpCmd);
}

void set_debug(int level)
{
    //char tmpCmd[128];

    switch (level)
	{
        case 0:
            send_log_cmd(LOGLEVEL_NONE);
            break;
        case 1:
            send_log_cmd(LOGLEVEL_1);
            break;
        case 2:
            send_log_cmd(LOGLEVEL_2);
            break;
        case 3:
            send_log_cmd(LOGLEVEL_ALL);
            break;
        default:
            break;
    }
}

int mg_init(void)
{
	int i;
	char tmpCmd[128];
	char reg_type[32];

	printf("interface=%s\n", g_pVoIPCfg->voip_interface);
	printf("mg_name=%s\n", g_pVoIPCfg->mg_name);

	set_debug(0);

	//mib_get(PROVINCE_DEV_REG_TYPE, &reg_type);
	get_exec_output("mib get PROVINCE_DEV_REG_TYPE", reg_type);
	printf("@@@  PROVINCE_DEV_REG_TYPE=%d\n", atoi(reg_type));

	if(4 == atoi(reg_type)) //4: Yunnan
	{
		reset_web_E8C_register_status();
		if (!check_248_server())
		{
			printf("### server not match, skip register!!!\n");
			E8C_update_and_display_registration_status(VOIP_STATUS_ADDR_ERR);
			return -1;
		}
	}

#if 0
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,PORT,all} {%d}\n",1);
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,all} {%d}\n",1);
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,all} {%d}\n",1);
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,all} {%d}\n",1);
	rcm_send_msg(tmpCmd);


    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,PORT,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);


    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,exp} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,err} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,wrn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,inf} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,dbg} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,ent} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SDP,lve} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);

    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,exp} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,err} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,wrn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,inf} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,dbg} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,ent} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,lve} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
#if 0
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,exp} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,err} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,wrn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,inf} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,dbg} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);


    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,exp} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,err} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,wrn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,inf} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,dbg} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,ent} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,HOST,lve} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);

     snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,exp} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,err} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,wrn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,inf} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,dbg} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
     snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,ent} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,lve} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
#endif
    //snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,ent} {%d}\n",1);
	//rcm_send_msg(tmpCmd);usleep(50000);
    //snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,lve} {%d}\n",1);
	//rcm_send_msg(tmpCmd);usleep(50000);


	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,TermMgr,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,MEGACO,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,SOCKET,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {logFilter,APP,syn} {%d}\n",1);
	rcm_send_msg(tmpCmd);usleep(50000);
#endif
	switch(g_pVoIPCfg->mg_deviceIdType)
	{
		case 0://IP
			snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,myName} {%s}\n",mgcc_ptr->bound_voip_ipaddress);
			break;
		case 1://Domain name
			snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,myName} {%s}\n",g_pVoIPCfg->Domain);
			break;
		case 2://Device Name
		default:
			snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,myName} {%s}\n",g_pVoIPCfg->mg_name);
			break;
	}
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,myPort} {%d}\n",g_pVoIPCfg->mg_port);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,mgcName} {%s}\n",g_pVoIPCfg->mgc_name);
	rcm_send_msg(tmpCmd);
	if((g_pVoIPCfg->ports[0].proxies[0].enable & PROXY_ENABLED)!=0){ //must set mgcIp to megaco stack, or it will skip register procedure
		snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,mgcIp} {%s}\n",g_pVoIPCfg->mgc_ip);
	}else
		printf("### H.248 not enable, skip register!!!\n");

	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,mgcPort} {%d}\n",g_pVoIPCfg->mgc_port);
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,mgIp} {%s}\n",mgcc_ptr->bound_voip_ipaddress);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,bak_mgcIp} {%s}\n",g_pVoIPCfg->bak_mgc_ip);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,bak_mgcName} {%s}\n",g_pVoIPCfg->bak_mgc_name);
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,bak_mgcPort} {%d}\n",g_pVoIPCfg->bak_mgc_port);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,transport} {%d}\n",0);// 0:udp, 1:tcp
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,rtp_base_port} {%d}\n",g_pVoIPCfg->ports[0].media_port);
	rcm_send_msg(tmpCmd);

	//snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,codec} {%s}\n","0,8,18,4");
    char precedence_codec[16];
    int codec[4];

    for (i=0;i<4;i++)
    {
        mg_print("precedence[%d]=%d\n",i,g_pVoIPCfg->ports[0].precedence[i]);
        if (g_pVoIPCfg->ports[0].precedence[i] == 0) //g.711u
        {
            codec[i] = 0;
        }
        else if (g_pVoIPCfg->ports[0].precedence[i] == 1) //g.711a
        {
            codec[i] = 8;
        }
        else if (g_pVoIPCfg->ports[0].precedence[i] == 2) //g.729
        {
            codec[i] = 18;
        }
        else if (g_pVoIPCfg->ports[0].precedence[i] == 3) //g.722
        {
            codec[i] = 9;
        }
    }
    snprintf(precedence_codec, sizeof(precedence_codec), "%d,%d,%d,%d",codec[0],codec[1],codec[2],codec[3]);
    snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,codec} {%s}\n",precedence_codec);
    rcm_send_msg(tmpCmd);

    int encoding = 0;
    if (strcmp(g_pVoIPCfg->mg_encoding,"ABNF") == 0)
    {
        encoding = 0;
    }
    else if (strcmp(g_pVoIPCfg->mg_encoding,"ASN.1") == 0)
    {
        encoding = 1;
    }

    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,three_handshake} {%d}\n", g_pVoIPCfg->three_handshake); //
	rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,longTimer} {%d}\n", g_pVoIPCfg->longTimer); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,pending_timer_init} {%d}\n", g_pVoIPCfg->pending_timer_init); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,retran_timer} {%d}\n", g_pVoIPCfg->retran_timer); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,retran_count} {%d}\n", g_pVoIPCfg->retran_count); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,RetransmissionTime} {%d}\n", g_pVoIPCfg->RetransmissionTime); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,RetransmissionCycle} {%d}\n", g_pVoIPCfg->RetransmissionCycle); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,register_count} {%d}\n", g_pVoIPCfg->register_count); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,register_cycle} {%d}\n", g_pVoIPCfg->register_cycle); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,HeartbeatMode} {%d}\n", g_pVoIPCfg->heartbeat_mode); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,HeartbeatCycle} {%d}\n", g_pVoIPCfg->heartbeat_cycle); //
    rcm_send_msg(tmpCmd);
    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,HeartbeatCount} {%d}\n", g_pVoIPCfg->heartbeat_count); //
    rcm_send_msg(tmpCmd);


    snprintf(tmpCmd, sizeof(tmpCmd), "varUpd {config,encoding} {%d}\n", encoding); //0: ABNF, 1: ASN.1
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,threadPriority} {%d}\n",0);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,timerThreadPriority} {%d}\n",0);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,sendDelay} {%d}\n",1);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,maxStartDelay} {%d}\n",0);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,minCtxId} {}\n");
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,maxCtxId} {}\n");
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,simulateV1} {%d}\n",0);
	rcm_send_msg(tmpCmd);;
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {config,numOfThreads} {%d}\n",1);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {options,catchStackLog} {%d}\n",1);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {options,minRetrans} {%d}\n",0);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {options,blinkMessage} {%d}\n",0);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {options,blockUnauthorizedMgc} {%d}\n",0);
	rcm_send_msg(tmpCmd);

	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,phyTermConfigMethod} {%d}\n",g_pVoIPCfg->mg_phyTermConfigMethod);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,phyTermID} {%s}\n",g_pVoIPCfg->mg_phyTermID);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,prefix} {%s}\n",g_pVoIPCfg->mg_prefix);
	rcm_send_msg(tmpCmd);

	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,phy_prefix_star} {%d}\n",g_pVoIPCfg->mg_phyTermPrefixStart);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,phy_prefix_len} {%d}\n",g_pVoIPCfg->mg_phyTermPrefixLen);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,rtp_prefix} {%s}\n",g_pVoIPCfg->mg_rtp_prefix);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,rtp_prefix_len} {%d}\n",g_pVoIPCfg->mg_rtpPrefixLen);
	rcm_send_msg(tmpCmd);
	snprintf(tmpCmd,sizeof(tmpCmd),"varUpd {term,idMod} {}\n");
	rcm_send_msg(tmpCmd);

	return 0;
}

int mg_addTerm(void)
{
	if(rcm_send_msg("callCmd,{term.Add}\n") < 0)
		return -1;

	return 0;

}

int mg_start(void)
{
	if(rcm_send_msg("callCmd,{test.Start}\n") < 0)
		return -1;

	return 0;

}

int mg_stop(void)
{
	if(rcm_send_msg("callCmd,{test.Stop}\n") < 0)
		return -1;

	return 0;

}

int mg_quit(void)
{
	if(rcm_send_msg("callCmd,{test.Quit}\n") < 0)
		return -1;

	return 0;

}


int mg_startBak(void)
{
	if(rcm_send_msg("callCmd,{test.StartBak}\n") < 0)
		return -1;

	return 0;

}

int mg_send_serviceChange(void)
{
	char cmd[64];

	sprintf(cmd,"callCmd,{test.sendServiceChange}\n");
	if(rcm_send_msg(cmd) < 0)
		return -1;

	return 0;
}


void mg_offhook_dialtimeout(void *arg)
{
	int nPort =	(int)arg; //*(int *)arg;

	mg_print("offhook_dialtimeout %d\n", nPort);

	//TODO
}

void rcm_mg_watchdog_hander(void *arg)
{
	write(RCM_WATCHDOG_pipe, "123", 4);
	if(RCM_WATCHDOG_period > 0){
		rcm_timerLaunch(mgcc_ptr->rcm_mg_watchdog_timer, ((RCM_WATCHDOG_period*2)/5)*1000);
	}

}
int rcm_call_waiting_handler(void)
{
    //stop special call waiting tone
    rtk_SetPlayTone(0, 0, DSPCODEC_TONE_CALL_WAITING, 0, DSPCODEC_TONEDIRECTION_LOCAL);
    return 0;
}


int rcm_mg_check_reg_handler(void)
{
    mg_print("~~~~~~~~~ rcm_mg_check_reg_handler ~~~~~~~~~~~\n ");

    if (strcmp(g_pVoIPCfg->bak_mgc_ip,"") == 0)
    {
        mg_print("[rcm_mg_check_reg_handler] no backup mgc, re-register primary MGC\n");
        //rcm_send_msg("callCmd,{test.Start}\n");
        mg_start();
        return 0;
    }

    //3. Try to register to another MGC
    if (current_mgc == PRIMARY_MGC)
    {
        //rcm_send_msg("callCmd,{test.Swap} {1}\n");
        mg_startBak();
        current_mgc =  SECONDARY_MGC;
    }
    else
    {
        //rcm_send_msg("callCmd,{test.Swap} {0}\n");
        mg_start();
        current_mgc =  PRIMARY_MGC;
    }
    timerStartFlag = 0;
    //rcm_timerLaunch(mgcc_ptr->check_reg_timer, g_pVoIPCfg->ports[0].proxies[0].RegisterRetryInterval*1000);
    return 0;
}

void mg_core_init(MGAPP *pApp, MgCore *mg, int chid)
{
	mg->parent = pApp;
	mg->chid = chid;
	mg->dial_code[0] = 0;
	mg->digit_index = 0;
	mg->reg_status = REGSTATUS_INIT;

	//net_config_read(lc);//read WAN type and bandwidth.
	//rtp_config_read(lc);//copy rtp configure from flash struct to linphone struct
	//codecs_config_read(lc); //read codec
}

int app_init(MGAPP *pApp)
{
	int i;
	char *voip_bound_ipaddress=NULL;

	mg_print("%s starts\n", __FUNCTION__);

	//Edison rcm_reset_web_register_status();

	ortp_init();

	memset(pApp, 0, sizeof(MGAPP));

	mg_init_voip_interface();
	mg_print("########## interface is %s\n", mgcc_ptr->current_voip_interface);

	// get port link status
	rcm_tapi_GetPortLinkStatus( (uint32 *) &pApp->port_link_status);

	if(rcm_get_localip_for_voip(mgcc_ptr->current_voip_interface, &voip_bound_ipaddress)==-1){
		mg_print("\r\n can't get VoIP interface IP address \n");
		exit(0);
	}
	strcpy(pApp->bound_voip_ipaddress, voip_bound_ipaddress);
	mg_print("voip_bound_ipaddress is %s\n",pApp->bound_voip_ipaddress);

	//start timer system
	rcm_timer_subsystem_init();

	//init wrapper client socket
	if(rcm_init_msg_if() < 0)
	{
		mg_print("MG init msg if fail!!\n");
		//return -1;
	}

#ifdef CONFIG_RTK_VOIP_TR104
	{
		struct sockaddr_un ipcAddr;

		ipcSocket = socket(PF_LOCAL, SOCK_DGRAM, 0);
		if (0 > ipcSocket)
			mg_print("open IPC socket fail: %s\n", strerror(errno));
		unlink(SOLAR_CHANNEL_PATH);
		bzero(&ipcAddr, sizeof(ipcAddr));
		ipcAddr.sun_family = PF_LOCAL;
		strncpy(ipcAddr.sun_path, SOLAR_CHANNEL_PATH, sizeof(SOLAR_CHANNEL_PATH));
		if (bind(ipcSocket, (struct sockaddr*)&ipcAddr, sizeof(ipcAddr)) == -1) {
			close(ipcSocket);
			mg_print("bind IPC socket fail: %s\n", strerror(errno));
		}
	}
#endif /*CONFIG_RTK_VOIP_TR104*/

	for (i=0; i<g_MaxVoIPPorts; i++)
	{
		mg_print(" mg_core_init %d\n",i);
		mg_core_init(pApp, &pApp->ports[i], i);

		pApp->ports[i].offhook_timer = rcm_timerCreate(mg_offhook_dialtimeout, (void *) i);
#if defined(CONFIG_RTK_VOIP_TR104)
		pApp->ports[i].e8c_tr104_timer = rcm_timerCreate(mg_e8c_dtmf_test, (void *) i);
#endif

		if( RTK_VOIP_IS_SLIC_CH( i, g_VoIP_Feature ) )
		{/* FXS */
			int mid;
			SLICRING_VOLTAGE_FREQ ring_index;

			/* Get flash hook time from flash and set to kernel space.*/
			rcm_tapi_Set_Flash_Hook_Time(i, g_pVoIPCfg->ports[i].flash_hook_time_min, g_pVoIPCfg->ports[i].flash_hook_time,800);//time unit: 10ms

            /* Set DTMF mode*/
            rtk_SetDtmfMode(i, g_pVoIPCfg->ports[i].dtmf_mode);
            rtk_SetRFC2833SendByAP(i, 1);

            /* Set line echo cancellation */
            rcm_tapi_Set_G168_LEC(i, g_pVoIPCfg->ports[i].lec);

            rtk_Set_VBD_EC(i, 1, 2, g_pVoIPCfg->ports[i].lec);

            if (g_pVoIPCfg->ports[i].nlp == 1)
                rcm_tapi_Set_echoTail(i, g_pVoIPCfg->ports[i].echoTail, 1,5,5,0); // 1:nlp_mute mode 2: nlp_shift mode
            else if (g_pVoIPCfg->ports[i].nlp == 0)
                rcm_tapi_Set_echoTail(i, g_pVoIPCfg->ports[i].echoTail, 0,5,5,0); // NLP disable

            /* Get Speaker AGC flag from flash and set. */
            rcm_tapi_Set_SPK_AGC(i, g_pVoIPCfg->ports[i].speaker_agc, 55);

            /* Get Speaker AGC require level from flash and set. */
            rcm_tapi_Set_SPK_AGC_LVL(i, g_pVoIPCfg->ports[i].spk_agc_lvl);

            /* Get Speaker AGC gain up from flash and set. */
            rcm_tapi_Set_SPK_AGC_GUP(i, g_pVoIPCfg->ports[i].spk_agc_gu);

            /* Get Speaker AGC gain down from flash and set. */
            rcm_tapi_Set_SPK_AGC_GDOWN(i, g_pVoIPCfg->ports[i].spk_agc_gd);

            /* Get MIC AGC flag from flash and set. */
            rcm_tapi_Set_MIC_AGC(i, g_pVoIPCfg->ports[i].mic_agc, 55);

            /* Get MIC AGC require level from flash and set. */
            rcm_tapi_Set_MIC_AGC_LVL(i, g_pVoIPCfg->ports[i].mic_agc_lvl);

            /* Get MIC AGC gain up from flash and set. */
            rcm_tapi_Set_MIC_AGC_GUP(i, g_pVoIPCfg->ports[i].mic_agc_gu);

            /* Get MIC AGC gain down from flash and set. */
            rcm_tapi_Set_MIC_AGC_GDOWN(i, g_pVoIPCfg->ports[i].mic_agc_gd);

            /* Set DSP tx/rx gain */
            rcm_tapi_Set_Voice_Gain(i, g_pVoIPCfg->ports[i].spk_voice_gain, g_pVoIPCfg->ports[i].mic_voice_gain);

            /* Get Caller ID mode from flash and set */
            if ((g_pVoIPCfg->ports[i].caller_id_mode&7) == CID_DTMF)
            {
                mg_print("Set Caller ID Mode to DTMF\n");
                if (g_pVoIPCfg->ports[i].caller_id_mode&8) //priori 1st ring
                {
                    rtk_SetDtmfCidParam(i, DTMF_DIGIT_A + (g_pVoIPCfg->ports[i].cid_dtmf_mode&0x3), DTMF_DIGIT_A + ((g_pVoIPCfg->ports[i].cid_dtmf_mode>>2)&0x3), 0xF, 80, 80, 300, 300);
                }
                else
                {
                    rtk_SetDtmfCidParam(i, DTMF_DIGIT_A + (g_pVoIPCfg->ports[i].cid_dtmf_mode&0x3), DTMF_DIGIT_A + ((g_pVoIPCfg->ports[i].cid_dtmf_mode>>2)&0x3), 0xB, 80, 80, 300, 300);
                }
            }
            else
            {
                mg_print("Set Caller ID Mode to FSK\n");
                rtk_Set_FSK_Area(i, g_pVoIPCfg->ports[i].caller_id_mode|0x100);  /* area -> 0:Bellcore 1:ETSI 2:BT 3:NTT, 0x100: auto SLIC action */
            }

            for (mid=0; mid<2; mid++)
            {
                rcm_tapi_SetThresholdVadCng(i, mid, g_pVoIPCfg->ports[i].vad_thr,
                                                    g_pVoIPCfg->ports[i].cng_thr,
                                                    g_pVoIPCfg->ports[i].sid_gainmode,
                                                    g_pVoIPCfg->ports[i].sid_noiselevel,
                                                    g_pVoIPCfg->ports[i].sid_noisegain);
            }

            switch(g_pVoIPCfg->ringVoltageFreq)
            {
                case 40:
                        ring_index=SLIC_RING_40V_25HZ;
                        break;
                case 50:
                        ring_index=SLIC_RING_50V_25HZ;
                        break;
                case 60:
                        ring_index=SLIC_RING_60V_25HZ;
                        break;
                case 70:
                        ring_index=SLIC_RING_70V_25HZ;
                        break;
                case 80:
                        ring_index=SLIC_RING_80V_25HZ;
                        break;
                case 90:
                        ring_index=SLIC_RING_90V_25HZ;
                        break;
                default:
                        ring_index=SLIC_RING_70V_25HZ;
                        break;
            }

            for (i=0; i<VOIP_PORTS; i++)
                rtk_SetSlicRingVoltageFreq(i, ring_index);
		}
	}

	/* RTP DSCP init */
	rtk_qos_reset_dscp_priority();
	rtk_QosSetDscpPriority(g_pVoIPCfg->rtpDscp, 7);
	rtk_SetRtpDscp(g_pVoIPCfg->rtpDscp);

	pApp->rcm_mg_watchdog_timer=rcm_timerCreate(rcm_mg_watchdog_hander,NULL);
	pApp->check_reg_timer=rcm_timerCreate(rcm_mg_check_reg_handler,NULL);
	pApp->call_waiting_timer=rcm_timerCreate(rcm_call_waiting_handler,NULL);
	//pApp->keep_alive_timer=rcm_timerCreate(keep_alive_send,NULL);

	// flush kernel used fifo
	for (i=0; i<g_MaxVoIPPorts; i++)
		rcm_tapi_Set_flush_fifo(i);
#if 0
	if( g_pVoIPCfg->tone_of_country >= TONE_EXT1 ){
		rcm_country_para_update(g_pVoIPCfg);
	}
	else{
		rcm_tapi_Set_Country(g_pVoIPCfg);
		rtk_SetCountryImpedance(g_pVoIPCfg->tone_of_country);
	}
#else// only for china
	rtk_SetCountry(g_pVoIPCfg->tone_of_country);
	rtk_SetCountryImpedance(g_pVoIPCfg->tone_of_country);
#endif

	static TstVoipToneCfg special_dial_tone			 = {0, 0, 1, 400,	  40,   0,	  0, 0, 0, 0, 0, 0, 1, 450,   0, 0, 0,	7,	0, 0, 0};
	rtk_SetUpdateTone(TONE_CHINA, DSPCODEC_TONE_CUSTOM_TONE1, &special_dial_tone);
#ifdef	CALL_WAITING_CONTINUE
	//static st_ToneCfgParam call_waiting_tone_continue         = {0, 0, 3,  100,  100,   100,  100, 100, 100, 0, 0, 0, 1, 440, 0, 0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg call_waiting_tone_continue         = {0, 0, 3,  400,  4000,   400, 4000,400,4000, 0, 0, 0, 1, 450, 0, 0, 0,  7,  7, 0, 0};
	rtk_SetUpdateTone(TONE_CHINA, DSPCODEC_TONE_CALL_WAITING, &call_waiting_tone_continue);
#else
	//static st_ToneCfgParam call_waiting_tone         = {0, 0, 3,  100,  100,   100,  100, 100, 10000, 0, 0, 0, 1, 440, 0, 0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg call_waiting_tone         = {0, 0, 3,  400,  4000,   400, 4000,400,10000, 0, 0, 0, 1, 450, 0, 0, 0,  7,  7, 0, 0};
	rtk_SetUpdateTone(TONE_CHINA, DSPCODEC_TONE_CALL_WAITING, &call_waiting_tone);
#endif
	return 0;
}

int app_close(MGAPP *pApp)
{

	//TODO unregister

	ortp_destroy();

	//close warpper socket fd
	//close(g_wrap_client);
#ifdef CONFIG_RTK_VOIP_TR104
	close(ipcSocket);
#endif

	mg_quit();

	return 0;
}

void rcm_set_mg_conctrol_fifo(void){

	//FIFO_CONTROL="/var/run/mgcc_control.fifo";
	FIFO_CONTROL="/var/run/solar_control.fifo";
}

static int app_run(MGAPP *pApp)
{
	MgCore *opm;
	int err;
	fd_set fdset;
	struct timeval timeout;
	struct timeval *timerlisthead_p;
	//gboolean run=TRUE;
	int ActiveLine = 1;
	int h_control;
	int h_max=0;
	// timer check variable
	struct timeval now;
	int h_nl;
	struct sockaddr_nl nl_local;
	SIGNSTATE keyin = SIGN_NONE;

	rcm_set_mg_conctrol_fifo();

	if(g_wrap_client == 0)
		return -1;


	// Use FIFO to do restart and auto-testing
    if (access(FIFO_CONTROL, F_OK) == -1)
	{
        if (mkfifo(FIFO_CONTROL, 0755) == -1)
	        mg_print("access %s failed: %s\n", FIFO_CONTROL, strerror(errno));
	}

    h_control = open(FIFO_CONTROL, O_RDWR);
    if (h_control == -1)
        mg_print("open %s failed\n", FIFO_CONTROL);


	h_nl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (h_nl <0) //robustness consideration
	{
		mg_print("netlink: open socket failed\n");
		return -1;
	}
	bzero(&nl_local, sizeof(nl_local));
	nl_local.nl_family = AF_NETLINK;
	nl_local.nl_pad = 0;
	nl_local.nl_pid = getpid();
	nl_local.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_NOTIFY | RTMGRP_LINK;

	err = bind(h_nl, (struct sockaddr *) &nl_local, sizeof(nl_local));
	if (err == -1){
		mg_print("netlink: bind failed\n");
		return -1;
	}


	mg_print("app_run 002\n");

	while (run)
	{
		ActiveLine = (ActiveLine + 1) % g_MaxVoIPPorts;

		opm = &pApp->ports[ActiveLine];

		// set descriptor, and find max
		FD_ZERO(&fdset);

        if (g_wrap_client > 0)
        {
            //printf("g_wrap_client=%d\n",g_wrap_client);
             FD_SET(g_wrap_client, &fdset);
             if(h_max < g_wrap_client)
                 h_max = g_wrap_client;
        }

		if(h_control >1024)
			mg_print("WRONG:fdset %s %d  > 1024\n",__FILE__,__LINE__);

		FD_SET(h_control, &fdset);
        if(h_max < h_control)
			h_max = h_control;

#ifdef CONFIG_RTK_VOIP_TR104
		FD_SET(ipcSocket, &fdset);
		if(h_max < ipcSocket)
			h_max = ipcSocket;
#endif

		FD_SET(h_nl, &fdset);

		if(h_max < h_nl)
			h_max = h_nl;

		/* check RCM timer list for select timeout */

		timerlisthead_p = rcm_getHeadTimer();

		if(timerlisthead_p==NULL){

			timeout.tv_sec= 0;
			timeout.tv_usec = 100000/g_MaxVoIPPorts; // 100ms

		} else {
			gettimeofday(&now, NULL);
			timersub(timerlisthead_p,&now,&timeout);

			if(timeout.tv_sec < 0){
                /* thread has been time up, no need to wait*/
   				timeout.tv_sec = 0;
				timeout.tv_usec = 0;
			}

			if((timeout.tv_sec * 1000000 + timeout.tv_usec)>(100000/g_MaxVoIPPorts)){
				timeout.tv_sec= 0;
				timeout.tv_usec = 100000/g_MaxVoIPPorts; // 100ms
			}
		}

		// wait event by select
		if((timeout.tv_usec<0) || (timeout.tv_usec>1000000))
			mg_print("\r\n apprun , time usec error !!!!!!!\n");

		if((h_max+1) > 1024)
			mg_print("WRONG: %s %d  > 1024\n",__FILE__,__LINE__);

		err = select(h_max + 1, &fdset, NULL, NULL, &timeout);

		if (FD_ISSET(g_wrap_client, &fdset))
		{
			//printf("g_wrap_client=%d fd_isset\n",g_wrap_client);
			//receive msg from wrapprer
			rcm_msg_handler();
		}

		if (FD_ISSET(h_control, &fdset))
		{
			char buffer[256];
			char *s;

			err = read(h_control, buffer, sizeof(buffer));
			if (err == 0)
			{
				// EOF !?
				close(h_control);
				h_control = open(FIFO_CONTROL, O_RDWR);
				if (h_control == -1)
					mg_print("open %s failed\n", FIFO_CONTROL);
			}
			else if (err > 0)
			{
				if (err == sizeof(buffer))
					mg_print("buffer is full\n");
				else
					buffer[err] = 0; // add null terminated

				//fprintf(stderr, "buffer = %s (%d)\n", buffer, strlen(buffer));
				s = strtok(buffer, "\n");
				while (s)
				{
                    if (strlen(s) == 2 && s[0] == 'e') // DSP event command
					{
						// 'ef': event FAX
						//printf( "DSP Event\n" );

						//CheckFaxOnCall(opm);
					}
                    else  if (s[1] == 0) // command
					{
						switch (s[0])
						{
						    case 'h':
								printf("H.248 command usage\n");
								printf("x , Restart VoIP\n");
								printf("c , enable debug to console\n");
								printf("g , disable debug to console\n");
								printf("v , call mgnt version\n");
								printf("k , enable Kernel DSP debug to Console \n");
								printf("t , Enable tapi_debug \n");
								printf("T , Disable tapi_debug\n");
								printf("I , show voip interface \n");

								break;

						    case 'X':
                            case 'x':
								mg_print("\r\nreceive %s command ,VoIP call manager restart\n", s);
								//printf("RCM_WATCHDOG_period is %d\n",RCM_WATCHDOG_period);
                                restart_mg();

                                break;
                            case 'v':
                                printf("\r\nmgcc: %s\n",built_time);
                                printf("megaco: %s\n",megacoBuildTime);
                                break;

                            case 'c': //mgapp log enable

                                g_mgcc_log = 1;
                                mg_print("g_mgcc_log =%d\n",g_mgcc_log);
                                break;

                             case 'g': //mgapp log disable


                                g_mgcc_log = 0;
                               printf("g_mgcc_log =%d\n",g_mgcc_log);
                                break;

                            case 'k':
								printf("enable Kernel DSP debug to Console\n");
								rcm_tapi_debug(1);
								break;

							case 'I':
								printf("voip interface info\n");
								rcm_dump_voip_if_name(mgcc_ptr->current_voip_interface, DNS_LEN);
								break;

							case 't':
								printf("Enable tapi_debug\n");
								rtk_VoipOpenTapiDebug(1);
								break;

							case 'T':
								printf("Disable tapi_debug\n");
								rtk_VoipOpenTapiDebug(0);
								break;

                            default:
                                break;
                        }
                    }
                    else  if (s[2] == 0) // debug command
					{
                        switch (s[0])
						{
                            case 'd': //log level
                                if (s[1] >= '0' && s[1] <= '3')
                                {
                                    int level;

                                    level = s[1] - '0';
                                    set_debug(level);
                                }

                                break;

                            default:
                                break;
                        }
                    }
					//TODO
					s = strtok(NULL, "\n");
				}

			}
			else
			{
				mg_print("read %s failed\n", FIFO_CONTROL);
			}
		}

#ifdef CONFIG_RTK_VOIP_TR104
			if(FD_ISSET(ipcSocket, &fdset))
			{
				void *buf = NULL;
				if((buf = (void *)malloc(cwmpEvtMsgSizeof())) != NULL){
					cwmpEvtMsg *evtMsg;
					if(recvfrom(ipcSocket, buf, cwmpEvtMsgSizeof(), 0, NULL, NULL)>1){
						evtMsg = (cwmpEvtMsg*) buf;
						mg_print("+++++get the status of line %d+++++\n",evtMsg->event);
						MgSendResponseToCwmp(evtMsg);
					}
					free(buf);
				}
			}

#endif /*CONFIG_RTK_VOIP_TR104*/

		if(FD_ISSET(h_nl, &fdset))
		{
			rcm_netlink_handler(h_nl);
		}

		//CheckFaxOnCall(opm); 	/* Check FAX status */

		rcm_TimerProc(); 		/* process timer system */

        mg_GetLineEvent(opm->chid,  &keyin); /* read event from kernel/dsp */

        if (keyin > SIGN_NONE && keyin != SIGN_OFFHOOK_2)
        {
        	mg_print("chid=%d keyin=%d\n", opm->chid, keyin);

            mgccEventEx(opm->termName, keyin);
            //send fxs event and ActiveLine to msg wrapper
        }
        keyin = SIGN_NONE; //reset keyin

        /*************** FAX Det ****************/
        CheckFax(opm->chid, opm->termName);

        mg_Diagnosis_Auto_Test(ActiveLine);
	}

	mg_print("\n");
	return 0;
}

int main (int argc, char *argv[])
{
	int i;
	int cad_idx;

	mg_print("@@@ MEGACO start @@@ \n");

	// destroy old process and create a PID file
	if(rcm_set_mgcc_pidfile()==0){
		mg_print("create pid file fail \n");
			return 0;
	}

	rcm_tapi_Get_VoIP_Feature();
	g_MaxVoIPPorts = RTK_VOIP_CH_NUM(g_VoIP_Feature);

	if (g_MaxVoIPPorts > MAX_VOIP_PORTS || g_MaxVoIPPorts <=0 )
	{
		fprintf(stderr, "Incorrect g_MaxVoIPPorts=%d, MAX_VOIP_PORTS=%d !?\n", g_MaxVoIPPorts, MAX_VOIP_PORTS);
		return -1;
	}

#ifdef VOIP_FLASH_ARCH	//Alex, original architecture
	if (rcm_voip_flash_client_init(&g_pVoIPShare, VOIP_FLASH_WRITE_CLIENT_SOLAR) == -1)
	{
		fprintf(stderr, "voip_flash_client_init failed.\n");
		return -1;
	}

	g_pVoIPCfg = &g_pVoIPShare->voip_cfg;

	if(g_pVoIPCfg->rfc_flags & SIP_DEBUG)
	{
		g_mgcc_log = 1;
	}
#else
	if (rcm_voip_flash_client_init(&g_pVoIPCfg, VOIP_FLASH_WRITE_CLIENT_SOLAR) == -1)
	{
		fprintf(stderr, "voip_flash_client_init failed.\n");
		return -1;
	}
#endif

	voip_line_state_init_variables();
	voip_line_state_share_get(&g_shmVoIPstate);

	signal(SIGINT, mgcc_abort);

	if (argc == 4) {
		if (strcmp(argv[1], "-wdt")==0) {
			// ex_app runs in pipe-mode.
			// the handle was passed into
			mg_print("watch dog pipe mode \n");
			RCM_WATCHDOG_pipe = atoi(argv[2]);
			if ((RCM_WATCHDOG_pipe < 0) || (RCM_WATCHDOG_pipe > 4096)) {
				mg_print("VoIP:Error: a strange pipe-number = %d\n", RCM_WATCHDOG_pipe);
			}
		}
		else if (strcmp(argv[1], "-wdt_fifo") == 0) {
			// ex_app runs in fifo-mode
			mg_print("VoIP:watch dog fifo  mode \n");
			wdt_fifo = argv[2];
			// open the fifo in order of the succession of gwdt
			if ((RCM_WATCHDOG_pipe=open(wdt_fifo, O_WRONLY)) < 0) {
				mg_print("VoIP:can't open fifo '%s'\n", wdt_fifo);
				return 1;
			}
		}
		RCM_WATCHDOG_period = atoi(argv[3]);
		mg_print("RCM_WATCHDOG_period is %d\n",RCM_WATCHDOG_period);
		if ((RCM_WATCHDOG_period < 0) || (RCM_WATCHDOG_period > 86400)) {
			mg_print("VoIP:Watchdog: an illegal timeout = %d\n", RCM_WATCHDOG_period);
		}
	}

	for (i=0; i<g_MaxVoIPPorts; i++)
	{
		/* init */
		//rcm_tapi_InitDSP(i);
		rtk_InitDsp(i);

		// fxs
		rtk_SetFlashHookTime(i, 100, 300,800);		// 100~300 ms

		cad_idx = g_pVoIPCfg->ring_cad;
		if (cad_idx == 8) // default case
			rcm_tapi_Set_SLIC_Ring_Cadence(i, 1000, 4000);
		else{
			//printf("===========> %d on %d off\n", g_pVoIPCfg->ring_cadon[cad_idx],g_pVoIPCfg->ring_cadoff[cad_idx]);
			rcm_tapi_Set_SLIC_Ring_Cadence(i, g_pVoIPCfg->ring_cadon[cad_idx], g_pVoIPCfg->ring_cadoff[cad_idx]); /* (chid, cad_on_msec, cad_off_msec) */
		}

		// priori 1st ring, auto AC, auto ring, digit duration = 80ms, inter digit pause = 80ms
		//rtk_SetDtmfCidParam(i, DTMF_DIGIT_A, DTMF_DIGIT_C, 0xf, 80, 80, 300, 300);

		// set fax modem det mode
		//rtk_SetFaxModemDet(i, 0); //0:auto-hi-speed-fax
		rtk_SetFaxModemDet(i, 3); //3:auto_2

		// set fax event detection
		rtk_SetAnswerToneDet(i , 0x3f1212 , 0 , 10);

		// flush kernel fifo before app run
		rtk_SetFlushFifo(i);

		// init local data
		//channel_init(&channels[i], i);

		rtk_OnHookReinit(i);
	}

	//start megaco stack and mgapp
	system("killall mgapp");
	system("mgapp &");

	if(app_init(&mgcc)<0)
		exit(1);

	/* support RCM Watch dog */
	if(RCM_WATCHDOG_period > 0) {
		rcm_timerLaunch(mgcc_ptr->rcm_mg_watchdog_timer, ((RCM_WATCHDOG_period*2)/5)*1000);
	}

	app_run(&mgcc);

	app_close(&mgcc);

	return 0 ;
}
