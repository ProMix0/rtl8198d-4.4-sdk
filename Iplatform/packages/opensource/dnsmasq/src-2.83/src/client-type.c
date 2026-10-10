/*
 *\file		client-type.c
 *\brief	check client type base on DHCP Options.
 *
 */

/***************************************************************************/
/*                    INCLUDE FILES                     */
/***************************************************************************/
#include <string.h>
#include "dnsmasq.h"
#include "uci.h"

/***************************************************************************/
/*                    GLOBAL_FUNCTIONS                  */
/***************************************************************************/
/*
 *\fn           device_type_check
 *\brief        check client type base on DHCP Options [60,12,55,57]
 *
 *\param[in]    p_hostname            hostname(option 12)
 *\param[in]    p_vendor              vendor(option 60)
 *\param[in]    p_req_options         request_options(option 55)
 *\param[in]    max_dhcp_message_size max_dhcp_message_size(option 57)
 *
 *\return       DEVICE_TYPE
 */
DEVICE_TYPE device_type_check(const char *p_hostname, const char *p_vendor, const unsigned char *p_req_options, size_t max_dhcp_message_size)
{
	size_t i = 0;
	size_t name_len = 0;
	size_t req_options_len = 0;
	unsigned char ios_options[5] = {0x1, 0x79, 0x3, 0x6, 0xf};
	unsigned char android_options[9] ={0x1, 0x3, 0x6, 0xf, 0x1a, 0x1c, 0x33, 0x3a, 0x3b};
	unsigned char windows_options[14] ={0x1, 0x3, 0x6, 0xf, 0x1f, 0x21, 0x2b, 0x2c, 0x2e, 0x2f, 0x77, 0x79, 0xf9, 0xfc};
	char buf[32] = {0};
	int flag = 0;

	/* 
	 * device type check base on Vendor(option 60)
	 */
#ifdef SUPPORT_FING
	if(NULL != p_vendor)
	{
		if(strstr(p_vendor, "ndroid")) /* "android" or "Android" */
		{
			return DEVICE_TYPE_MOBILE;
		}
		else if(strstr(p_vendor, "MSFT")) 
		{
			return DEVICE_TYPE_COMPUTER;
		}
		else if((strstr(p_vendor, "SoC") || strstr(p_vendor, "soc"))) /* system on chip */
		{
			return DEVICE_TYPE_IOT_DEVICE;
		}
	}
#else
	if(NULL != p_vendor)
	{
		if(strstr(p_vendor, "ndroid")) /* "android" or "Android" */
		{
			return DEVICE_TYPE_PHONE;
		}
		else if(strstr(p_vendor, "MSFT")) 
		{
			return DEVICE_TYPE_PC;
		}
		else if((strstr(p_vendor, "SoC") || strstr(p_vendor, "soc"))) /* system on chip */
		{
			return DEVICE_TYPE_IOT;
		}
	}
#endif

	/* 
	 * device type check base on Request options(option 55)
	 */
	if(NULL == p_req_options)
	{
		return DEVICE_TYPE_OTHER;
	}
	req_options_len = strlen(p_req_options);
	for(i = 0; i < req_options_len; i++)
	{
		/* PC usually request NetBIOS options  */
		if(p_req_options[i] == OPTION_NBNS_OPT || p_req_options[i] == OPTION_NBNT_OPT)
		{	
#ifdef SUPPORT_FING
			return DEVICE_TYPE_COMPUTER;
#else
			return DEVICE_TYPE_PC;
#endif
		}
	}

	/* match IOS, windows, android request options  */
	if((!memcmp(p_req_options, ios_options, sizeof(ios_options)) && NULL == p_vendor ) || 
		!memcmp(p_req_options, android_options, sizeof(android_options)))
	{
#ifdef SUPPORT_FING
		return DEVICE_TYPE_MOBILE;
#else
		return DEVICE_TYPE_PHONE;
#endif		
	}

	if(!memcmp(p_req_options, windows_options, sizeof(windows_options)))
	{
#ifdef SUPPORT_FING
		return DEVICE_TYPE_COMPUTER;
#else
		return DEVICE_TYPE_PC;
#endif
	}

	if (NULL != p_vendor && (strstr(p_vendor, "linux") || strstr(p_vendor, "Linux")))
	{
		return DEVICE_TYPE_TABLET;
	}

	/*
	 * device type check base on Max dhcp massage size(option 57)
	 */
	if(max_dhcp_message_size == 576)
	{
#ifdef SUPPORT_FING
		return DEVICE_TYPE_IOT_DEVICE;
#else
		return DEVICE_TYPE_IOT;
#endif
	}

	/* some IoT devices dont have a device_name and tend to request hostname form DHCP server */
	for(i = 0; i < req_options_len; i++)
	{
		if(p_req_options[i] == OPTION_HOSTNAME)
		{
#ifdef SUPPORT_FING			
			return DEVICE_TYPE_IOT_DEVICE;
#else
		return DEVICE_TYPE_IOT;
#endif
		}
		else if(p_req_options[i] == OPTION_DOMAINNAME)
		{
			flag = 1;
		}

		if(i == (req_options_len -1) && 0 == flag)
		{
#ifdef SUPPORT_FING			
			return DEVICE_TYPE_IOT_DEVICE;
#else
		return DEVICE_TYPE_IOT;
#endif
		}
	}

	/*
	 * device type check base on Hostname(option 12)
	 */
	if(NULL != p_hostname)
	{
		memset(buf, 0, 32);

		name_len = strlen(p_hostname);
		if(name_len > 31)
		{
			name_len = 31;
		}

		strncpy(buf, p_hostname, name_len);
		for(i=0; i < sizeof(buf) ; i++)
		{
			if(buf[i] >= 'A' && buf[i] <= 'Z')
			{
				buf[i] = 'a' + (buf[i] - 'A');
			}
		}

		if(strstr(buf, "iphone") || strstr(buf, "galaxy"))
		{
#ifdef SUPPORT_FING	
			return DEVICE_TYPE_MOBILE;
#else
			return DEVICE_TYPE_PHONE;
#endif
		}
		else if(strstr(buf, "pad"))
		{
			return DEVICE_TYPE_TABLET;
		}
		else if(strstr(buf, "desktop")  || strstr(buf, "imac"))
		{
#ifdef SUPPORT_FING	
			return DEVICE_TYPE_COMPUTER;
#else
			return DEVICE_TYPE_PC;
#endif
		}
		else if(strstr(buf, "macbook"))
		{
#ifdef SUPPORT_FING	
			return DEVICE_TYPE_LAPTOP;
#else
			return DEVICE_TYPE_LABTOP;
#endif
		}
	}

	return DEVICE_TYPE_OTHER;
}


#ifdef SUPPORT_FING	
int uci_get_state(const char * pkgName, const char * sectName, const char * optName, char * value, int vlen)
{
    struct uci_context *ctx  = NULL;
    struct uci_ptr ptr;

    if(pkgName == NULL || sectName == NULL || optName == NULL)
    {
        return -1;
    }

    if(NULL == (ctx = uci_alloc_context()))
    {
        //printErr("uci_alloc_context failed.\n");
        return -1;
    }

    uci_set_savedir(ctx, "/var/state");

    memset(&ptr, 0, sizeof(struct uci_ptr));
    ptr.package = pkgName;
    ptr.section = sectName;
    ptr.option = optName;

    /*first find last state.*/
    if (uci_lookup_ptr(ctx, &ptr, NULL, true) == UCI_OK && ptr.o)
    {
        strncpy(value, ptr.o->v.string, vlen - 1);
        value[vlen - 1] = '\0';
    }

    uci_unload(ctx, ptr.p);
    uci_free_context(ctx);
    ctx = NULL;

    return 0;
}

int uci_set_state(const char * pkgName, const char * sectName, const char * optName, const char * value)
{
    struct uci_context *ctx  = NULL;
    struct uci_ptr ptr;

	if(pkgName == NULL || sectName == NULL || optName == NULL || value == NULL)
	{
		return -1;
	}
	
	if(NULL == (ctx = uci_alloc_context()))
	{
		printf("uci_alloc_context failed.\n");
		return -1;
	}

	uci_add_delta_path(ctx, ctx->savedir);
	uci_set_savedir(ctx, "/var/state");
	
	memset(&ptr, 0, sizeof(struct uci_ptr));
	ptr.package = pkgName;
	ptr.section = sectName;
	ptr.option = optName;

	/*first find last state.*/
	if (uci_lookup_ptr(ctx, &ptr, NULL, true) == UCI_OK && ptr.o)
	{
		/*then compare value.*/
		if (!strcmp(ptr.o->v.string, value))
		{
			goto finish;
		}
		/* revert when unequal */
		uci_revert(ctx, &ptr);
	}
	else
	{
		uci_unload(ctx, ptr.p);
	}

	/*last set new state.*/
	memset(&ptr, 0, sizeof(struct uci_ptr));
	ptr.package = pkgName;
	ptr.section = sectName;
	ptr.option = optName;
	ptr.value = value;
	
	uci_set(ctx, &ptr);
	uci_save(ctx, ptr.p);

finish:
	uci_free_context(ctx);
	ctx = NULL;

	return 0;
}

int uci_delete_section()
{
	FILE *fp = NULL;
	char line[512] = {0};
	char fing_str[16] = {0};
	char mac_str[16] = {0};
	char state_str[512] = {0};
	char delete_sec[64] = {0};

	fp = fopen("/var/state/fing", "r");
	if (fp == NULL)
	{
		return -1;
	}
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%[^.].%[^.=]=%5s", fing_str, mac_str, state_str);
		if (!strcmp(fing_str, "fing") && !strcmp(state_str, "state") && strlen(mac_str) == 12)
		{
			sprintf(delete_sec, "uci -P /var/state revert fing.%s", mac_str);
			system(delete_sec);
			break;
		}
	}
	fclose(fp);
	return 0;
}

int check_section_state(const char * pkgName, const char * sectName)
{
	char cmd[512] = {0};
	char res[32] = {0};
	int mac_num = 0;
	char mac_num_str[8] = {0};
	FILE *stream;
	if(pkgName == NULL || sectName == NULL)
	{
		return -1;
	}
	sprintf(cmd, "uci -P /var/state get %s.%s", pkgName, sectName);

	stream = popen(cmd, "r");
	if (stream == NULL)
	{
		return -1;
	}
	memset(res, 0, sizeof(res));
	fread(res, sizeof(char), sizeof(res), stream);

	if (strncmp(res, "state", 5))
	{
		pclose(stream);
		sprintf(cmd, "uci -P /var/state set \"%s.%s=state\"", pkgName, sectName);
		system(cmd);
		uci_get_state("fing", "global", "mac_num", mac_num_str, sizeof(mac_num_str));
		mac_num = atoi(mac_num_str);
		if (mac_num == 200)
		{
			uci_delete_section();
		}
		else
		{
			mac_num++;
			sprintf(mac_num_str, "%d", mac_num);
			uci_set_state("fing", "global", "mac_num", mac_num_str);
		}
		return 0;
	}
	pclose(stream);
	return 1;
}

#endif
