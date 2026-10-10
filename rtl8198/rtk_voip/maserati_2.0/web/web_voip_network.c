#include <stdio.h>
#include <string.h>
#include "web_voip.h"
#include "rcm_customize.h"

#ifdef CONFIG_USER_PPPOMODEM
#include "mib.h"
//add for 3g card
#define TO_IFINDEX(x,y,z)			((x<<16) | (y<<8) | z)
#endif

extern char *ifGetName(int ifindex, char *buffer, unsigned int len) ;

char hwnat[2][10]={"Disable", "Enable"};

#ifdef SUPPORT_DSCP
static char dscp[DSCP_MAX][25] = {"Class 0 (DSCP 0x00)",
			"Class 1 (DSCP 0x08)",
			"Class 2 (DSCP 0x10)",
			"Class 3 (DSCP 0x18)",
			"Class 4 (DSCP 0x20)",
			"Class 5 (DSCP 0x28)",
			"Class 6 (DSCP 0x30)",
			"Class 7 (DSCP 0x38)",
			"EF (DSCP 0x2e)"
			};
#endif

void asp_voip_NetSet(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	int i ;
	char* ptr;

//eric merge
	unsigned itf_idx = 0;
	char itf_name[16] = {0};
	
	if (web_flash_get(&pVoIPCfg) != 0)
		return;

//ericchung: support voip interface select
#if defined (CONFIG_RTK_VOIP_MULTI_WAN_CTYPE) && (VOIP_SUPPORT_MULTI_WAN)
	itf_idx = atoi(boaGetVar(wp, "voip_itf", ""));

	if(itf_idx==1){
		//any wan 
		//printf("set voip interface is null\n");
			pVoIPCfg->voip_interface[0] = '\0';
	}
    else if(itf_idx == 2){
		memset(pVoIPCfg->voip_interface, 0, DNS_LEN);
		//printf("set voip interface is br0\n");
		strcpy(pVoIPCfg->voip_interface, "br0" );
    }
#ifdef CONFIG_USER_PPPOMODEM
    else if(itf_idx == TO_IFINDEX(MEDIA_3G, MODEM_PPPIDX_FROM, 0)){ //3g card 
		memset(pVoIPCfg->voip_interface, 0, DNS_LEN);
	//	printf("set voip interface is ppp8\n");
		strcpy(pVoIPCfg->voip_interface, "ppp8" );	
    }    	
#endif	

#ifdef CONFIG_USER_ZM8620_ECMMODE
	else if(itf_idx == 404040){ //4g card 
		memset(pVoIPCfg->voip_interface, 0, DNS_LEN);
	//	printf("set voip interface is usb0\n");
		strcpy(pVoIPCfg->voip_interface, "usb0" );	
	}
#endif
	else{
		ifGetName(itf_idx, itf_name, sizeof(itf_name));
		//printf("save , itf_name %s, itf_idx = %d\n",itf_name,itf_idx);
		memset(pVoIPCfg->voip_interface, 0, DNS_LEN);
		strncpy(pVoIPCfg->voip_interface, itf_name , DNS_LEN);
	}
#endif

#ifdef SUPPORT_DSCP
	pVoIPCfg->rtpDscp = atoi(boaGetVar(wp, "rtpDscp", "0"));
	pVoIPCfg->sipDscp = atoi(boaGetVar(wp, "sipDscp", "0"));

	/*
	ptr	 = boaGetVar(wp, "rtpDscp", "");
	for(i=0; i<DSCP_MAX; i++)
	{
		if (!gstrcmp(ptr, dscp[i]))
			break;
	}
	if (i == DSCP_MAX)
		i = DSCP_CS0;
	pVoIPCfg->rtpDscp = i;

	ptr	 = boaGetVar(wp, "sipDscp", "");
	for(i=0; i<DSCP_MAX; i++)
	{
		if (!gstrcmp(ptr, dscp[i]))
			break;
	}
	if (i == DSCP_MAX)
		i = DSCP_CS0;
	pVoIPCfg->sipDscp = i;
	*/
#endif

	web_flash_set(pVoIPCfg);

#ifdef REBOOT_CHECK
#ifndef CONFIG_GENERAL_WEB
	OK_MSG("/voip_network.asp");
#else
	OK_MSG("/voip_network_new_web.asp");
#endif
#else
#ifdef CONFIG_RTL865XC
#ifdef CONFIG_DEFAULTS_KERNEL_2_6
	extern int run_init_script_flag;
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif
#else
	web_restart_solar();
#endif
#ifndef CONFIG_GENERAL_WEB
	boaRedirect(wp, "/voip_network.asp");
#else
	boaRedirect(wp, "/voip_network_new_web.asp");
#endif
#endif
}


#ifdef CONFIG_APP_BOA
int asp_voip_NetGet(request * wp, int argc, char **argv)
#else
int asp_voip_NetGet(int ejid, request * wp, int argc, char **argv)
#endif
{
	voipCfgParam_t *pVoIPCfg;
	int index;

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;

#ifdef SUPPORT_DSCP
	if (strcmp(argv[0], "rtpDscp")==0)
    {
		boaWrite(wp, "%d", pVoIPCfg->rtpDscp);
		/*
        for (index=0; index<DSCP_MAX ;index++)
        {
			if (index == pVoIPCfg->rtpDscp)
                boaWrite(wp, "<option selected>%s</option>", dscp[index]);
            else
                boaWrite(wp, "<option>%s</option>", dscp[index]);
        }
		*/
    }
	else if (strcmp(argv[0], "sipDscp")==0)
	{
		boaWrite(wp, "%d", pVoIPCfg->sipDscp);
		/*
		for (index=0; index<DSCP_MAX ;index++)
		{
			if (index == pVoIPCfg->sipDscp)
				boaWrite(wp, "<option selected>%s</option>", dscp[index]);
			else
				boaWrite(wp, "<option>%s</option>", dscp[index]);
		}
		*/
	}
#endif
#if defined (CONFIG_RTK_VOIP_MULTI_WAN_CTYPE) && (VOIP_SUPPORT_MULTI_WAN)
	else if (strcmp(argv[0], "display_voip_interface_select")==0)
			 boaWrite(wp, "%s", "");	
#else
	else if (strcmp(argv[0], "display_voip_interface_select")==0)
			 boaWrite(wp, "%s", "style=\"display:none\"");
#endif    	
	else
	{
		return -1;
	}

	return 0;
}

