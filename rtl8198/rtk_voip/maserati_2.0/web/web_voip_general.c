#include <stdio.h>
#include <string.h>
#include "web_voip.h"
#include "rcm_customize.h"

char dtmf[DTMF_MAX][12] = {"RFC2833", "SIP INFO", "Inband", "DTMF_delete"};
char g726pack[G726_PACK_MAX][9] = {"None", "Left", "Right"};
char *g7111_modes[G7111_MODES] = {"R1", "R2A", "R2B", "R3"};

//eric add
#define SUPPORT_CUSTOMIZE_FRAME_SIZE 1

#ifdef SUPPORT_VOICE_QOS
char dscp[DSCP_MAX][25] = {"Class 0 (DSCP 0x00)",
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

char *supported_codec_string[SUPPORTED_CODEC_MAX] = {
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
#ifdef CONFIG_RTK_VOIP_AMR_WB
	"G7222-6P6",
	"G7222-8P85",
	"G7222-12P65",
	"G7222-14P25",
	"G7222-15P85",
	"G7222-18P25",
	"G7222-19P85",
	"G7222-23P05",
	"G7222-23P85",
#endif

	};

void asp_sip_codec_var(request * wp, voipCfgPortParam_t *pCfg)
{
	int i;

	for (i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		boaWrite(wp,
			"<input type=hidden id=preced_id name=preced%d value=-1>", i
			);
	}

	for (i=0; i<G7111_MODES; i++)
	{
		boaWrite(wp,
			"<input type=hidden id=g7111_modes name=%s value=-1>",
				g7111_modes[i]
			);
	}
}

void asp_sip_codec(request * wp, voipCfgPortParam_t *pCfg)
{
	int i, j;
#ifdef SUPPORT_CUSTOMIZE_FRAME_SIZE
	int step, loop;
#endif

	boaWrite(wp,
		"<tr align=center>" \
		"<td bgColor=#aaddff width=85 rowspan=2>%s</td>"
		,multilang(LANG_TYPE)
		);

// framesize is reserved
#ifdef SUPPORT_CUSTOMIZE_FRAME_SIZE
	boaWrite(wp,
		"<td bgColor=#ddeeff width=85 rowspan=2>%s</td>"
		,multilang(LANG_PACKETIZATION)
		);
#endif

	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=%d>%s</td>",
		SUPPORTED_CODEC_MAX
		,multilang(LANG_PRECEDENCE)
		);

/*support disable codec */
	boaWrite(wp,
		"<td bgColor=#ddeeff width=85 rowspan=2>%s</td>"
		,multilang(LANG_DISABLE)
		);


	boaWrite(wp, "</tr>\n");

	// Draw precedence number
	boaWrite(wp, "<tr align=center>");
	for (i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		boaWrite(wp, "<td bgColor=#ddeeff>%d</td>", i + 1);
	}
	boaWrite(wp, "</tr>\n");

	// Draw Codecs
	for (i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		// codec name
		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>%s</td>",
			supported_codec_string[i]
			);

// framesize is reserved
#ifdef SUPPORT_CUSTOMIZE_FRAME_SIZE
		// framesize
		boaWrite(wp,
			"<td bgColor=#ddeeff>" \
			"<select name=frameSize%d>",
			i
			);

		switch (i)
		{
		case SUPPORTED_CODEC_G711U:
		case SUPPORTED_CODEC_G711A:
#ifdef CONFIG_RTK_VOIP_G722
		case SUPPORTED_CODEC_G722:
#endif
			// 10, 20 ... 60 ms
			step = 10;
//			loop = 6;
			loop= 4;
			break;
#ifdef CONFIG_RTK_VOIP_G7231
		case SUPPORTED_CODEC_G723:
			// 30, 60, 90 ms
			step = 30;
			loop = 3;
			break;
#endif
#ifdef CONFIG_RTK_VOIP_AMR_WB
		case SUPPORTED_CODEC_G7222_6P6:
		case SUPPORTED_CODEC_G7222_8P85:
		case SUPPORTED_CODEC_G7222_12P65:
		case SUPPORTED_CODEC_G7222_14P25:
		case SUPPORTED_CODEC_G7222_15P85:
		case SUPPORTED_CODEC_G7222_18P25:
		case SUPPORTED_CODEC_G7222_19P85:
		case SUPPORTED_CODEC_G7222_23P05:
		case SUPPORTED_CODEC_G7222_23P85:
			step = 20;
			loop = 3;
			break;
#endif
		default:
			// 10, 20 ... 90ms
			step = 10;
			loop = 9;
			break;
		}

		for (j=0; j<loop; j++)
			boaWrite(wp,
				"<option %s value=%d>%d %s</option>",
				j == pCfg->frame_size[i] ? "selected" : "",
				j,
				step * (j + 1),
//				multilang(LANG_MS)
				"ms"
				);

		boaWrite(wp,
			"</select>" \
			"</td>\n"
			);
#endif /* SUPPORT_CUSTOMIZE_FRAME_SIZE */

		// precedence
		for (j=0; j<SUPPORTED_CODEC_MAX; j++)
		{
			boaWrite(wp,
				"<td bgColor=#ddeeff align=center>" \
				"<input type=checkbox name=precedence %s onclick=\""
				"checkPrecedence_support_disable(sipform.precedence,sipform.codec_disable, %d, %d, %d, 1)\">" \
				"</td>\n",
				j == pCfg->precedence[i] ? "checked" : "",
				i, j, SUPPORTED_CODEC_MAX
			);
		}

/*support disable codec */
		boaWrite(wp,
						"<td bgColor=#ddeeff align=center>" \
						"<input type=checkbox name=codec_disable %s></td>\n",  pCfg->precedence[i]>SUPPORTED_CODEC_MAX ? "checked" : "");


		boaWrite(wp, "</tr>\n");
	}
}

void asp_sip_codec_opt(request * wp, voipCfgPortParam_t *pCfg)
{
	int i;
#ifdef CONFIG_RTK_VOIP_G7111
	int j;
#endif

	i = 1;
#ifdef CONFIG_RTK_VOIP_G726
	i ++;
#endif
#ifdef CONFIG_RTK_VOIP_G7231
	i ++;
#endif
#ifdef CONFIG_RTK_VOIP_ILBC
	i ++;
#endif
#ifdef CONFIG_RTK_VOIP_SPEEX_NB
	i ++;
#endif
#ifdef CONFIG_RTK_VOIP_G7111
	i += 2 + G7111_MODES; // 2 header + 4 modes
#endif

	if (i == 1)
		return;

	boaWrite(wp, "<table cellSpacing=1 cellPadding=2 border=0>");

	boaWrite(wp,
		"<tr>" \
		"<td bgColor=#aaddff width=85 rowspan=%d>%s</td>"
		"</tr>", i, multilang(LANG_OPTION)
		);

#ifdef CONFIG_RTK_VOIP_G726
	boaWrite(wp,"<tr>" );
	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=1> %s" \
		"</td>", multilang(LANG_G726_PACKING_ORDER)
		);

	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=5>" \
		"<select WIDTH=300 STYLE='width: 120px' name=g726_packing >" \
		"<option %s>%s</option>" \
		"<option %s>%s</option>" \
		"</select>" \
		"</td>",
		pCfg->g726_packing == G726_PACK_LEFT ? "selected" : "",
//		multilang(LANG_LEFT),
		"Left",
		pCfg->g726_packing == G726_PACK_RIGHT ? "selected" : "",
//		multilang(LANG_RIGHT)
		"Right"
		);
	boaWrite(wp, "</tr>\n");

	//"<option %s>None</option>"
	//pCfg->g726_packing == G726_PACK_NONE ? "selected" : "",
#endif

#ifdef CONFIG_RTK_VOIP_G7231
	boaWrite(wp,"<tr>" );
			boaWrite(wp,
		"<td bgColor=#ddeeff colspan=1> %s" \
		"</td>", multilang(LANG_G723_BIT_RATE)
		);

	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=5>" \
		"<select WIDTH=300 STYLE='width: 120px' name=g7231Rate>" \
				"<option %s>6.3k</option>" \
				"<option %s>5.3k</option>" \
				"</select>" \
				"</td>",
				pCfg->g7231_rate == G7231_RATE63 ? "selected" : "",
				pCfg->g7231_rate == G7231_RATE53 ? "selected" : ""
				);

	boaWrite(wp, "</tr>\n");
#endif

#ifdef CONFIG_RTK_VOIP_ILBC
	boaWrite(wp,"<tr>" );
			boaWrite(wp,
		"<td bgColor=#ddeeff colspan=1> iLBC Frame Size" \
		"</td>"
		);

	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=5>" \
		"<select WIDTH=300 STYLE='width: 120px' name=iLBC_mode>" \
				"<option %s>30ms</option>" \
				"<option %s>20ms</option>" \
				"</select>" \
				"</td>",
				pCfg->iLBC_mode == ILBC_30MS ? "selected" : "",
				pCfg->iLBC_mode == ILBC_20MS ? "selected" : ""
				);

	boaWrite(wp, "</tr>\n");
#endif

#ifdef CONFIG_RTK_VOIP_SPEEX_NB
	boaWrite(wp,"<tr>" );
			boaWrite(wp,
		"<td bgColor=#ddeeff colspan=1> SPEEX NB Rate" \
		"</td>"
		);

	boaWrite(wp,
		"<td bgColor=#ddeeff colspan=5>" \
		"<select WIDTH=300 STYLE='width: 120px' name=speex_nb_rate>" \
				"<option %s value=0>2.15k</option>" \
				"<option %s value=1>5.95k</option>" \
				"<option %s value=2>8k</option>" \
				"<option %s value=3>11k</option>" \
				"<option %s value=4>15k</option>" \
				"<option %s value=5>18.2k</option>" \
				"<option %s value=6>24.6k</option>" \
				"<option %s value=7>3.95k</option>" \
				"</select>" \
				"</td>",
				pCfg->speex_nb_rate == SPEEX_RATE2P15 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE5P95 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE8 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE11 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE15 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE18P2 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE24P6 ? "selected" : "",
				pCfg->speex_nb_rate == SPEEX_RATE3P95 ? "selected" : ""
				);

	boaWrite(wp, "</tr>\n");
#endif

#ifdef CONFIG_RTK_VOIP_G7111
	boaWrite(wp, "<tr>");
	boaWrite(wp, "<td bgColor=#ddeeff rowspan=%d> G711 WB Modes </td>",
		2 + G7111_MODES);
	boaWrite(wp, "<td bgColor=#ddeeff rowspan=2>Mode</td>");
	boaWrite(wp, "<td bgColor=#ddeeff colspan=%d> Priority </td>",
		G7111_MODES);
	boaWrite(wp, "</tr>");

	boaWrite(wp, "<tr>");
	for (i=0; i<G7111_MODES; i++)
		boaWrite(wp, "<td bgColor=#ddeeff>%d</td>", i + 1);
	boaWrite(wp, "</tr>");

	for (i=0; i<G7111_MODES; i++)
	{
		boaWrite(wp, "<tr>");
		boaWrite(wp, "<td bgColor=#ddeeff>%s</td>", g7111_modes[i]);
		for (j=0; j<G7111_MODES; j++)
		{
			boaWrite(wp, "<td bgColor=#ddeeff align=center>"
				"<input type=checkbox name=g7111_pri %s onclick=\""
				"checkPrecedence(sipform.g7111_pri, %d, %d, %d, 0)\""
				"</td>\n",
				i + 1 == pCfg->g7111_precedence[j] ? "checked" : "",
				i, j, G7111_MODES
			);
		}
		boaWrite(wp, "</tr>");
	}
#endif

	boaWrite(wp, "</table>");
}

void asp_volumne(request * wp, int nVolumne)
{
	int i;

	for(i=0; i<10; i++)
	{
		if (i == nVolumne)
			boaWrite(wp, "<option selected>%d</option>", (i+1));
		else
			boaWrite(wp, "<option>%d</option>", (i+1));
	}
}

void asp_sip_speed_dial(request * wp, voipCfgPortParam_t *pCfg)
{
	int i;
#if 0 //remove unnecessary parameter
	for (i=0; i<MAX_SPEED_DIAL; i++)
	{
		boaWrite(wp,
			"<tr bgcolor=#ddeeff>" \
			"<td align=center>%d</td>", i);

		boaWrite(wp,
			"<td><input type=text id=spd_name name=spd_name%d size=10 maxlength=%d value=\"%s\"></td>",
			i, MAX_SPEED_DIAL_NAME - 1, pCfg->speed_dial[i].name);

		boaWrite(wp,
			"<td><input type=text id=spd_url name=spd_url%d size=20 maxlength=%d value=\"%s\" onChange=\"spd_dial_edit()\"></td>",
			i, MAX_SPEED_DIAL_URL - 1, pCfg->speed_dial[i].url);

		boaWrite(wp,
			"<td align=center><input type=checkbox name=spd_sel %s></td>",
			pCfg->speed_dial[i].url[0] ? "" : "disabled");

		boaWrite(wp, "</tr>");
	}
#endif	
}

#ifdef CONFIG_APP_BOA
int asp_voip_GeneralGet(request * wp, int argc, char **argv)
#else
int asp_voip_GeneralGet(int ejid, request * wp, int argc, char **argv)
#endif
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	int voip_port;
#ifdef CONFIG_RTK_VOIP_SIP_TLS
	int certNum=0;
#endif

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;

	rtk_GetVoIPFeature();
	
	voip_port = atoi(boaGetVar(wp, "port", "0"));
	if (voip_port < 0 || voip_port >= g_VoIP_Ports)
		return -1;

	pCfg = &pVoIPCfg->ports[voip_port];

	if (strcmp(argv[0], "voip_port")==0)
	{
		boaWrite(wp, "%d", voip_port);
	}
	else if (strcmp(argv[0], "sip_number")==0)
	{
		for (i=0; i< VOIP_PORTS; i++)
		{
			if(pVoIPCfg->ports[i].proxies[0].number != NULL)
				boaWrite(wp, "<td>%s</td>", pVoIPCfg->ports[i].proxies[0].number);	
		}

	}
	else if (strcmp(argv[0], "allregisterStatus")==0)
	{
		FILE *fh;
		char buf[MAX_VOIP_PORTS * MAX_PROXY];
		
		fh = fopen(_PATH_TMP_STATUS, "r");
		if (!fh) {
			printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
			printf("\nerrno=%d\n", errno);
		}

		memset(buf, 0, sizeof(buf));
		if (fread(buf, sizeof(buf), 1, fh) == 0) {
			printf("Web: The content of /tmp/status is NULL!!\n");
			printf("\nerrno=%d\n", errno);
			boaWrite(wp, "%s", multilang(LANG_ERROR));
		}
		else {
			for (i=0; i< VOIP_PORTS; i++)
			{
				/* SD6, bohungwu, fix incorrect registration status for the secondary proxy */
				char p0_reg_st, p1_reg_st, p_reg_st;
				if((pVoIPCfg->ports[i].proxies[0].enable & PROXY_ENABLED)==0)
				{
					boaWrite(wp, "<td>%s</td>", multilang(LANG_DISABLED));	
				}
				else
				{			
					p0_reg_st = buf[i * MAX_PROXY];
					p1_reg_st = buf[i * MAX_PROXY + 1];
					if((p0_reg_st == '1')||(p0_reg_st == '2')||(p0_reg_st == '4')||(p0_reg_st == '5')){
						p_reg_st = p0_reg_st;
					}
					else{
						if((pVoIPCfg->ports[i].proxies[1].enable & PROXY_ENABLED)&&(p1_reg_st != '0'))
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
					case '0':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_NOT_REGISTERED));
						break;
					case '1':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTERED));
						break;
					case '2':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTERING));
						break;
					case '5':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTER_FAIL));
						break;
					case '3':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTER_FAIL));
						break;
					case '4':
						boaWrite(wp, "<td>%s</td>", multilang(LANG_VOIP_RESTART));
						break;
					default:
						boaWrite(wp, "<td>%s</td>", multilang(LANG_ERROR));
						break;
					}
				}
			}
		}

		fclose(fh);
	}
	// proxy
	else if (strcmp(argv[0], "proxy")==0)
	{
		if(!(pVoIPCfg->rfc_flags & SIP_SERVER_REDUNDANCY)){
#ifndef CONFIG_GENERAL_WEB
		boaWrite(wp, "<p><b>%s</b>\n"  \
			"<table>\n" \
			"<tr>\n" \
			"<td >%s</td>\n" \
			"<td>",
#else
		boaWrite(wp, "<div class=\"column\">\n" \
			"<div class=\"column_title\">\n" \
			"<div class=\"column_title_left\"></div>\n" \
			"<p>%s</p>\n"  \
			"<div class=\"column_title_right\"></div>\n" \
			"</div>\n" \
			"<div class=\"data_common\">\n" \
			"<table>\n" \
			"<tr>\n" \
			"<th>%s</th>\n" \
			"<td>",
#endif
			multilang(LANG_DEFAULT_PROXY),
			multilang(LANG_SELECT_DEFAULT_PROXY)
		);

		boaWrite(wp, "<select name=default_proxy>");
		for (i=0; i<MAX_PROXY ;i++)
		{
			boaWrite(wp, "<option value=%d %s>%s%d</option>",
				i,
				i == pCfg->default_proxy ? "selected" : "",
//				multilang(LANG_PROXY),
				"Proxy",
				i
			);
		}

		boaWrite(wp, "</select>");
		boaWrite(wp, "</td></tr></table>");
#ifdef CONFIG_GENERAL_WEB
		boaWrite(wp, "\n</div></div>");
#endif
		}

		for (i=0; i<MAX_PROXY; i++)
		{
#ifdef CONFIG_GENERAL_WEB
			boaWrite(wp, "<div class=\"column\">\n" \
			"<div class=\"column_title\">\n" \
			"<div class=\"column_title_left\"></div>\n");
#endif
			if(pVoIPCfg->rfc_flags & SIP_SERVER_REDUNDANCY){
				if(i==0){
					boaWrite(wp, "<p><b>Main Proxy</b>\n");
				}else{
					boaWrite(wp, "<p><b>Backup Proxy</b>\n");
				}
			}else{
			boaWrite(wp, "<p><b>%s%d</b>\n", multilang(LANG_PROXY), i);
			}
	
			// account
#ifndef CONFIG_GENERAL_WEB
			boaWrite(wp,
				"<table cellSpacing=1 cellPadding=2 border=0>\n" \
				"<tr>\n" \
				"<td bgColor=#aaddff width=155>%s</td>\n" \
				"<td bgColor=#ddeeff width=170>\n" \
				"<input type=text id=display name=display%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_DISPLAY_NAME), i, pCfg->proxies[i].display_name);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
 				"<td bgColor=#ddeeff>\n" \
				"<input type=text id=number name=number%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_NUMBER), i, pCfg->proxies[i].number);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=text id=loginID name=loginID%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_LOGIN_ID), i, pCfg->proxies[i].login_id);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=password id=password name=password%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_PASSWORD), i, pCfg->proxies[i].password);
			// register server
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=checkbox id=proxyEnable name=proxyEnable%d %s>%s\n" \
				"</td></tr>\n",
				multilang(LANG_PROXY), i, (pCfg->proxies[i].enable & PROXY_ENABLED) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=text id=proxyAddr name=proxyAddr%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_PROXY_ADDR), i, pCfg->proxies[i].addr);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=text id=proxyPort name=proxyPort%d size=10 maxlength=5 value=%d></td>\n" \
				"</tr>\n",
				multilang(LANG_PROXY_PORT), i, pCfg->proxies[i].port);
            boaWrite(wp,
                "<tr>\n" \
                "<td bgColor=#aaddff>%s</td>\n" \
                "<td bgColor=#ddeeff>\n" \
                "<input type=checkbox id=proxySubscribe name=proxy%d_subscribe_enable %s>%s\n" \
                "</td></tr>\n",
                multilang(LANG_PROXY_SUBSCRIBE), i, (pCfg->proxies[i].enable & PROXY_SUBSCRIBE) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>" \
				"<input type=text id=domain_name name=domain_name%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_SIP_DOMAIN), i, pCfg->proxies[i].domain_name);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>\n" \
				"<input type=text id=regExpiry name=regExpiry%d size=20 maxlength=5 value=%d></td>\n"
				"</tr>\n",
				multilang(LANG_REG_EXPIRE_SEC), i, pCfg->proxies[i].reg_expire);

//eric add

			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff><input type=checkbox id=obEnable name=obEnable%d %s>%s</td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY), i, (pCfg->proxies[i].outbound_enable) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>" \
				"<input type=text id=obProxyAddr name=obProxyAddr%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY_ADDR), i, pCfg->proxies[i].outbound_addr);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff>" \
				"<input type=text id=obProxyPort name=obProxyPort%d size=10 maxlength=5 value=%d></td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY_PORT), i, pCfg->proxies[i].outbound_port);
			
//sip Session

			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff><input type=checkbox id=sessionEnable name=sessionEnable%d %s>%s</td>\n" \
				"</tr>\n",
				multilang(LANG_ENABLE_SESSION_TIMER), i, (pCfg->proxies[i].SessionUpdateTimer) ? "checked" : "", multilang(LANG_ENABLE));

			boaWrite(wp,
						"<tr>\n" \
						"<td bgColor=#aaddff>%s</td>\n" \
						"<td bgColor=#ddeeff>\n" \
						"<input type=text id=sessionExpiry name=sessionExpiry%d size=6 maxlength=5 value=%d></td>\n"
						"</tr>\n",
						multilang(LANG_SESSION_EXPIRE_SEC), i, pCfg->proxies[i].SessionUpdateTimer);
#else
			boaWrite(wp, "<div class=\"column_title_right\"></div>\n" \
				"</div>\n" \
				"<div class=\"data_common\">\n");
			boaWrite(wp,
				"<table>\n" \
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=text id=display name=display%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_DISPLAY_NAME), i, pCfg->proxies[i].display_name);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
 				"<td>\n" \
				"<input type=text id=number name=number%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_NUMBER), i, pCfg->proxies[i].number);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=text id=loginID name=loginID%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_LOGIN_ID), i, pCfg->proxies[i].login_id);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=password id=password name=password%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_PASSWORD), i, pCfg->proxies[i].password);
			// register server
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=checkbox id=proxyEnable name=proxyEnable%d %s>%s\n" \
				"</td></tr>\n",
				multilang(LANG_PROXY), i, (pCfg->proxies[i].enable & PROXY_ENABLED) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=text id=proxyAddr name=proxyAddr%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_PROXY_ADDR), i, pCfg->proxies[i].addr);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=text id=proxyPort name=proxyPort%d size=10 maxlength=5 value=%d></td>\n" \
				"</tr>\n",
				multilang(LANG_PROXY_PORT), i, pCfg->proxies[i].port);
            boaWrite(wp,
                "<tr>\n" \
                "<th>%s</th>\n" \
                "<td>" \
                "<input type=checkbox id=proxySubscribe name=proxy%d_subscribe_enable %s>%s\n" \
                "</td></tr>\n",
                multilang(LANG_PROXY_SUBSCRIBE), i, (pCfg->proxies[i].enable & PROXY_SUBSCRIBE) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>" \
				"<input type=text id=domain_name name=domain_name%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_SIP_DOMAIN), i, pCfg->proxies[i].domain_name);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>\n" \
				"<input type=text id=regExpiry name=regExpiry%d size=20 maxlength=5 value=%d></td>\n"
				"</tr>\n",
				multilang(LANG_REG_EXPIRE_SEC), i, pCfg->proxies[i].reg_expire);

//eric add

			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td><input type=checkbox id=obEnable name=obEnable%d %s>%s</td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY), i, (pCfg->proxies[i].outbound_enable) ? "checked" : "", multilang(LANG_ENABLE));
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>" \
				"<input type=text id=obProxyAddr name=obProxyAddr%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY_ADDR), i, pCfg->proxies[i].outbound_addr);
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td>" \
				"<input type=text id=obProxyPort name=obProxyPort%d size=10 maxlength=5 value=%d></td>\n" \
				"</tr>\n",
				multilang(LANG_OUTBOUND_PROXY_PORT), i, pCfg->proxies[i].outbound_port);
			
//sip Session

			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td><input type=checkbox id=sessionEnable name=sessionEnable%d %s>%s</td>\n" \
				"</tr>\n",
				multilang(LANG_ENABLE_SESSION_TIMER), i, (pCfg->proxies[i].SessionUpdateTimer) ? "checked" : "", multilang(LANG_ENABLE));

			boaWrite(wp,
						"<tr>\n" \
						"<th>%s</th>\n" \
						"<td>\n" \
						"<input type=text id=sessionExpiry name=sessionExpiry%d size=6 maxlength=5 value=%d></td>\n"
						"</tr>\n",
						multilang(LANG_SESSION_EXPIRE_SEC), i, pCfg->proxies[i].SessionUpdateTimer);
#endif

			// nat traversal server
#if 0//eric
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>Outbound Proxy</td>\n" \
				"<td bgColor=#ddeeff><input type=checkbox id=obEnable name=obEnable%d %s>Enable</td>\n" \
				"</tr>\n",
				i, (pCfg->proxies[i].outbound_enable) ? "checked" : "");
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>Outbound Proxy Addr</td>\n" \
				"<td bgColor=#ddeeff>" \
				"<input type=text id=obProxyAddr name=obProxyAddr%d size=20 maxlength=39 value=%s></td>\n" \
				"</tr>\n",
				i, pCfg->proxies[i].outbound_addr);
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>Outbound Proxy Port</td>\n" \
				"<td bgColor=#ddeeff>" \
				"<input type=text id=obProxyPort name=obProxyPort%d size=10 maxlength=5 value=%d></td>\n" \
				"</tr>\n",
				i, pCfg->proxies[i].outbound_port);
			boaWrite(wp,
				"<tr>\n"	\
				"<td bgColor=#aaddff>Nortel SoftSwitch</td>\n" \
				"<td bgColor=#ddeeff>\n" \
    			"<input type=checkbox id=proxyNortel name=proxyNortel%d %s %s onclick=\"check_nortel_proxy()\">Enable\n" \
				"</td></tr>\n",
				i,
				(pCfg->proxies[i].enable & PROXY_NORTEL) ? "checked" : "",
				i == 0 ? "" : "disabled");
#endif
#ifdef CONFIG_RTK_VOIP_SIP_TLS
			if(voip_port == 0 && i == 0)
				certNum=1;
			else if(voip_port == 0 && i == 1)
				certNum=2;
			else if(voip_port == 1 && i == 0)
				certNum=3;
			else if(voip_port == 1 && i == 1)
				certNum=4;

			if(certNum >= 1 && certNum <= 4){
				boaWrite(wp,
					"<tr>\n" \
#ifndef CONFIG_GENERAL_WEB

					"<td bgColor=#aaddff>SIP TLS Enable</td>\n" \
					"<td bgColor=#ddeeff>\n" \
#else
					"<th>SIP TLS Enable</th>\n" \
					"<td>\n" \
#endif
					"<input type=checkbox id=sipTLSEnable name=sipTLSEnable%d %s>Enable</td>\n" \
					"</tr>\n",
					i, (pCfg->proxies[i].siptls_enable) ? "checked" : "");
				if(pCfg->proxies[i].siptls_enable)
					boaWrite(wp,
						"<tr>\n" \
#ifndef CONFIG_GENERAL_WEB
						"<td bgColor=#aaddff>TLS Certificate</td>\n" \
						"<td bgColor=#ddeeff>\n" \
#else
						"<th>TLS Certificate</th>\n" \
						"<td>\n" \
#endif
						"<input type=\"button\" value=\"Show Certificate\" name=\"showCert\" onClick=\"window.open('voip_tls.asp?cert=%d','SIP_TLS','height=400, left=400');\"></td>\n" \
						"</tr>\n",certNum);
				}
#endif /*CONFIG_RTK_VOIP_SIP_TLS*/
#if 0
#ifndef CONFIG_GENERAL_WEB
			boaWrite(wp,
				"<tr>\n" \
				"<td bgColor=#aaddff>%s</td>\n" \
				"<td bgColor=#ddeeff><iframe src=voip_sip_status.asp?port=%d&index=%d " \
				"frameborder=0 height=20 width=160 scrolling=no marginheight=0 marginwidth=0>\n" \
				"</iframe></td>\n" \
				"</tr>\n",
				multilang(LANG_REGISTER_STATUS), voip_port, i);
#else
			boaWrite(wp,
				"<tr>\n" \
				"<th>%s</th>\n" \
				"<td><iframe src=voip_sip_status_new_web.asp?port=%d&index=%d " \
				"frameborder=0 height=20 width=160 scrolling=no marginheight=0 marginwidth=0>\n" \
				"</iframe></td>\n" \
				"</tr>\n",
				multilang(LANG_REGISTER_STATUS), voip_port, i);
#endif  //CONFIG_GENERAL_WEB
#endif
			boaWrite(wp, "</table>");
#ifdef CONFIG_GENERAL_WEB
			boaWrite(wp, "</div></div>");
#endif
		}
	}
	#if 0//eric 
	else if (strcmp(argv[0], "stun")==0)
	{
		boaWrite(wp,
			"<tr>\n" \
			"<td bgColor=#aaddff>Stun</td>\n" \
			"<td bgColor=#ddeeff><input type=checkbox id=stunEnable name=stunEnable %s>Enable</td>\n" \
			"</tr>\n",
			(pCfg->stun_enable) ? "checked" : "");
		boaWrite(wp,
			"<tr>\n" \
			"<td bgColor=#aaddff>Stun Server Addr</td>\n" \
			"<td bgColor=#ddeeff>\n" \
			"<input type=text id=stunAddr name=stunAddr size=20 maxlength=39 value=%s></td>\n" \
			"</tr>\n",
			pCfg->stun_addr);
		boaWrite(wp,
			"<tr>\n" \
			"<td bgColor=#aaddff>Stun Server Port</td>\n" \
			"<td bgColor=#ddeeff>\n" \
			"<input type=text id=stunPort name=stunPort size=10 maxlength=5 value=%d></td>\n" \
			"</tr>\n",
			pCfg->stun_port);
	}
	#endif
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
			for (voip_port=0; voip_port<VOIP_PORTS; voip_port++)
			{
                boaWrite(wp, "<tr>");
                boaWrite(wp, "<td>%d</td>", voip_port+1);

                /* SD6, bohungwu, fix incorrect registration status for the secondary proxy */
				if((pVoIPCfg->ports[voip_port].proxies[0].enable & PROXY_ENABLED)==0 &&
                   (pVoIPCfg->ports[voip_port].proxies[1].enable & PROXY_ENABLED)==0 )
				{
					boaWrite(wp, "<td></td>");
					boaWrite(wp, "<td>%s</td>", multilang(LANG_DISABLED));	
				}
				else 
				{
					char p0_reg_st, p1_reg_st, p_reg_st;

                    if (pVoIPCfg->ports[voip_port].proxies[0].enable & PROXY_ENABLED)
                        boaWrite(wp, "<td>%s</td>", pVoIPCfg->ports[voip_port].proxies[0].number);
                    else
                        boaWrite(wp, "<td>%s</td>", pVoIPCfg->ports[voip_port].proxies[1].number);
					
                    if (2 == pVoIPCfg->X_CT_servertype) //h.248
                    {
                        p_reg_st = buf[0];
                    }
                    else 
                    {
                        p0_reg_st = buf[voip_port * MAX_PROXY];
                        p1_reg_st = buf[voip_port * MAX_PROXY + 1];

                        if((p0_reg_st == '1')||(p0_reg_st == '2')||(p0_reg_st == '4')||(p0_reg_st == '5')) {
                            p_reg_st = p0_reg_st;
                        }
                        else {
                            if((pVoIPCfg->ports[voip_port].proxies[1].enable & PROXY_ENABLED)&&(p1_reg_st != '0'))
                            {
                                p_reg_st = p1_reg_st;
                            }
                            else //sencond proxy disable, use main proxy status
                            {
                                p_reg_st = p0_reg_st;
                            }
                        }
                        
                        //fprintf(stderr, "buf is %s.\n", buf);
                        switch (p_reg_st) {
                        case '0':
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_NOT_REGISTERED));
                            break;
                        case '1':
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTERED));
                            break;
                        case '2':
                        case '5':
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTERING));
                            break;
                        case '3':
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_REGISTER_FAIL));
                            break;
                        case '4':
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_VOIP_RESTART));
                            break;
                        default:
                            boaWrite(wp, "<td>%s</td>", multilang(LANG_ERROR));
                            break;
                        }
                    }

                    if (2 == pVoIPCfg->X_CT_servertype)
                        break; //break for loop because H.248 only display root status
				}
                boaWrite(wp, "</tr>");					
			}
		}

		fclose(fh);
	}
	// SIP Advanced
	else if (strcmp(argv[0], "sipPort")==0)
		boaWrite(wp, "%d", pCfg->sip_port);
	else if (strcmp(argv[0], "sipPorts")==0)
	{
		for (i=0; i<g_VoIP_Ports; i++)
		{
			boaWrite(wp,
				"<input type=hidden id=sipPorts name=sipPorts value=\"%d\">",
				pVoIPCfg->ports[i].sip_port);
		}
	}
	else if (strcmp(argv[0], "rtpPort")==0)
		boaWrite(wp, "%d", pCfg->media_port);
	else if (strcmp(argv[0], "rtpPorts")==0)
	{
		for (i=0; i<g_VoIP_Ports; i++)
		{
			boaWrite(wp,
				"<input type=hidden id=rtpPorts name=rtpPorts value=\"%d\">",
				pVoIPCfg->ports[i].media_port);
		}
	}
	else if (strcmp(argv[0], "dtmfMode")==0)
	{
		for (i=0; i<DTMF_MAX; i++)
		{
			if (i == pCfg->dtmf_mode)
				boaWrite(wp, "<option selected>%s</option>", dtmf[i]);
			else
				boaWrite(wp, "<option>%s</option>", dtmf[i]);
		}
	}
	else if (strcmp(argv[0], "dtmf_2833_pt")==0)
	{
		if (pCfg->dtmf_mode == DTMF_RFC2833)
			boaWrite(wp, "%d", pCfg->dtmf_2833_pt);
		else
			boaWrite(wp, "%d disabled", pCfg->dtmf_2833_pt);
	}
	else if (strcmp(argv[0], "dtmf_2833_pi")==0)
	{
		if (pCfg->dtmf_mode == DTMF_RFC2833)
			boaWrite(wp, "%d", pCfg->dtmf_2833_pi);
		else
			boaWrite(wp, "%d disabled", pCfg->dtmf_2833_pi);
	}
	else if (strcmp(argv[0], "fax_modem_2833_pt_same_dtmf")==0)
		boaWrite(wp, "%s", (pCfg->fax_modem_2833_pt_same_dtmf) ? "checked" : "");
	else if (strcmp(argv[0], "fax_modem_2833_pt")==0)
	{
		if (pCfg->dtmf_mode == DTMF_RFC2833)
			boaWrite(wp, "%d", pCfg->fax_modem_2833_pt);
		else
			boaWrite(wp, "%d disabled", pCfg->fax_modem_2833_pt);
	}
	else if (strcmp(argv[0], "fax_modem_2833_pi")==0)
	{
		if (pCfg->dtmf_mode == DTMF_RFC2833)
			boaWrite(wp, "%d", pCfg->fax_modem_2833_pi);
		else
			boaWrite(wp, "%d disabled", pCfg->fax_modem_2833_pi);
	}
    else if (strcmp(argv[0], "sipInfo_duration")==0)
	{
		if (pCfg->dtmf_mode == DTMF_SIPINFO)
			boaWrite(wp, "%d", pCfg->sip_info_duration);
		else
			boaWrite(wp, "%d disabled", pCfg->sip_info_duration);
	}
	else if (strcmp(argv[0], "call_waiting")==0)
		boaWrite(wp, "%s", (pCfg->call_waiting_enable) ? "checked" : "");
	else if (strcmp(argv[0], "call_waiting_cid")==0)
	{
		if (pCfg->call_waiting_enable == 0)
			boaWrite(wp, "%s", (pCfg->call_waiting_cid) ? "checked disabled" : "disabled");
		else
			boaWrite(wp, "%s", (pCfg->call_waiting_cid) ? "checked" : "");
	}
	else if (strcmp(argv[0], "reject_direct_ip_call")==0){
		if(	pCfg->fxsport_flag & FXS_REJECT_DIRECT_IP_CALL){
			boaWrite(wp, "%s", "checked");
		}else{
			boaWrite(wp, "%s", "");
			}
	}
	/* caller id hidden */
	else if (strcmp(argv[0], "caller_id_hidden")==0){
		if(	pCfg->fxsport_flag & FXS_CALLERID_HIDDEN){
			boaWrite(wp, "%s", "checked");
		}else{
			boaWrite(wp, "%s", "");
			}
	}	
	// call transfer
	else if (strcmp(argv[0], "Call_Transfer")==0){
		if(	pCfg->fxsport_flag & FXS_CALL_TRANSFER_disable){
			boaWrite(wp, "%s", "");
		}else{
			boaWrite(wp, "%s", "checked");
			}
	}
	// 3 way conference
	else if (strcmp(argv[0], "3PTY_Conference")==0){
		if(	pCfg->fxsport_flag & FXS_3WAY_DISABLE){
			boaWrite(wp, "%s", "");
		}else{
			boaWrite(wp, "%s", "checked");
			}
	}
	else if (strcmp(argv[0], "Conference_server_cpe")==0){
		if(	pCfg->fxsport_flag & FXS_3WAY_ON_SERVER){
	   		boaWrite(wp, "<input type=\"radio\" name=\"Conference_server_cpe\" value=0 checked>server");
	   		boaWrite(wp, "<input type=\"radio\" name=\"Conference_server_cpe\" value=1 >CPE");
		}else{
	   		boaWrite(wp, "<input type=\"radio\" name=\"Conference_server_cpe\" value=0 >server");
	   		boaWrite(wp, "<input type=\"radio\" name=\"Conference_server_cpe\" value=1 checked>CPE");
		}
	}
	// 3pty on sip server
	else if (strcmp(argv[0], "conference_uri")==0){
		boaWrite(wp, "%s", pVoIPCfg->conference_uri);
	}	
	// forward
	else if (strcmp(argv[0], "CFAll")==0)
	{
   		boaWrite(wp, "<input type=\"radio\" name=\"CFAll\" value=0 %s>%s",
   			pCfg->uc_forward_enable == 0 ? "checked" : "", multilang(LANG_OFF));
   		boaWrite(wp, "<input type=\"radio\" name=\"CFAll\" value=1 %s>%s",
   			pCfg->uc_forward_enable == 1 ? "checked" : "", multilang(LANG_VOIP));
		boaWrite(wp, "<input type=\"radio\" name=\"CFAll\" value=2 %s %s>PSTN",
			pCfg->uc_forward_enable == 2 ? "checked" : "",
			!RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ) ? "disabled" : "");
	}
	else if (strcmp(argv[0], "CFAll_No")==0)
	{
		boaWrite(wp, "<input type=text name=CFAll_No size=20 maxlength=39 value=%s>",
			pCfg->uc_forward);

	}
	else if (strcmp(argv[0], "CFBusy")==0)
	{
   		boaWrite(wp, "<input type=\"radio\" name=\"CFBusy\" value=0 %s %s>%s",
 			pCfg->busy_forward_enable == 0 ? "checked" : "",
			RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_OFF));
		boaWrite(wp, "<input type=\"radio\" name=\"CFBusy\" value=1 %s %s>%s",
    		pCfg->busy_forward_enable == 1 ? "checked" : "",
			RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_VOIP));
	}
	else if (strcmp(argv[0], "CFBusy_No")==0)
	{
		boaWrite(wp, "<input type=text name=CFBusy_No size=20 maxlength=39 value=\"%s\" %s>",
			pCfg->busy_forward,
			RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ) ? "disabled=true" : "");
	}
	else if (strcmp(argv[0], "CFNoAns")==0 && !RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ))
	{
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>%s</td>" \
			"<td bgColor=#ddeeff>" \
   			"<input type=\"radio\" name=\"CFNoAns\" value=0 %s>%s" \
   			"<input type=\"radio\" name=\"CFNoAns\" value=1 %s>%s" \
			"</td>" \
			"</tr>",
			multilang(LANG_NO_ANSWER_FORWARD_TO),
			pCfg->na_forward_enable == 0 ? "checked" : "",
			multilang(LANG_OFF),
			pCfg->na_forward_enable == 1 ? "checked" : "",
			multilang(LANG_VOIP)
		);
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>%s</td>" \
			"<td bgColor=#ddeeff>" \
			"<input type=text name=CFNoAns_No size=20 maxlength=39 value=\"%s\">" \
			"</td>" \
			"</tr>",
			multilang(LANG_NO_ANSWER_NUMBER),
			pCfg->na_forward
		);
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>%s</td>" \
			"<td bgColor=#ddeeff>" \
			"<input type=text name=CFNoAns_Time size=20 maxlength=39 value=%d>" \
			"</td>" \
			"</tr>",
			multilang(LANG_NO_ANSWER_TIME_SEC),
			pCfg->na_forward_time
		);
	#ifdef FXO_REDIAL
		if (voip_port == 0) // if FXS0
		{
	   		boaWrite(wp,
				"<tr>" \
				"<td bgColor=#aaddff>No Answer Forward for PSTN</td>" \
				"<td bgColor=#ddeeff>" \
   				"<select name=PSTN_CFNoAns>" \
				"<option %s value=0>Off</option>" \
				"<option %s value=1>2 Stage Dialing</option>" \
				"<option %s value=2>Direct Forward</option>" \
				"</select>" \
				"</td>" \
				"</tr>",
				pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_enable == 0 ? "selected" : "",
				pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_enable == 1 ? "selected" : "",
				pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_enable == 2 ? "selected" : ""
			);
   			boaWrite(wp,
				"<tr>" \
				"<td bgColor=#aaddff>No Answer Number for PSTN</td>" \
				"<td bgColor=#ddeeff>" \
				"<input type=text name=PSTN_CFNoAns_No size=20 maxlength=39 value=\"%s\">" \
				"</td>" \
				"</tr>",
				pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward
			);
   			boaWrite(wp,
				"<tr>" \
				"<td bgColor=#aaddff>No Answer Time for PSTN</td>" \
				"<td bgColor=#ddeeff>" \
				"<input type=text name=PSTN_CFNoAns_Time size=20 maxlength=39 value=%d>" \
				"</td>" \
				"</tr>",
				pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_time
			);
		}
	#endif
	}
	else if (strcmp(argv[0], "CFNoAns")==0 && RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ))
	{
	#ifdef FXO_REDIAL
		// do nothing
	#else
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>No Answer Forward to</td>" \
			"<td bgColor=#ddeeff>" \
   			"<input type=\"radio\" name=\"CFNoAns\" value=0 %s disabled=true>Off" \
   			"<input type=\"radio\" name=\"CFNoAns\" value=1 %s disabled=true>VoIP" \
			"</td>" \
			"</tr>",
			pCfg->na_forward_enable == 0 ? "checked" : "",
			pCfg->na_forward_enable == 1 ? "checked" : ""
		);
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>No Answer Number</td>" \
			"<td bgColor=#ddeeff>" \
			"<input type=text name=CFNoAns_No size=20 maxlength=39 value=\"%s\" disabled=true>" \
			"</td>" \
			"</tr>",
			pCfg->na_forward
		);
   		boaWrite(wp,
			"<tr>" \
			"<td bgColor=#aaddff>No Answer Time (sec)</td>" \
			"<td bgColor=#ddeeff>" \
			"<input type=text name=CFNoAns_Time size=20 maxlength=39 value=%d disabled=true>" \
			"</td>" \
			"</tr>",
			pCfg->na_forward_time
		);
	#endif
	}
	// Speed dial
	else if (strcmp(argv[0], "speed_dial_display_title") == 0)
	{
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		/* FXS channel has no speed_dial, but FXO still need it. */
		if( RTK_VOIP_IS_SLIC_CH( voip_port, g_VoIP_Feature ) )
			;
		else
#endif
		{
			boaWrite(wp, "<p>\n<b>%s</b>\n", multilang(LANG_SPEED_DIAL));
		}
	}
	else if (strcmp(argv[0], "speed_dial_display") == 0)
	{
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		/* FXS channel has no speed_dial, but FXO still need it. */
		if( RTK_VOIP_IS_SLIC_CH( voip_port, g_VoIP_Feature ) )
			boaWrite(wp, "style=\"display:none\"");
#endif
	}
#if 0 //remove unnecessary parameter	
	else if (strcmp(argv[0], "speed_dial")==0)
		asp_sip_speed_dial(wp, pCfg);
    // abbreviated dial
    else if (strcmp(argv[0], "abbreviated_dial")==0)
    {
        for( i = 0; i < MAX_ABBR_DIAL_NUM; i ++ ) {
            boaWrite(wp, "<tr bgcolor=#ddeeff>\n" );
            boaWrite(wp, "<td algin=center><input type=text name=abbr_name%d size=10 maxlength=4 value=\"%s\"></td>\n", i, pCfg->abbr_dial[ i ].name );
            boaWrite(wp, "<td algin=center><input type=text name=abbr_url%d size=20 maxlength=60 value=\"%s\"></td>\n", i, pCfg->abbr_dial[ i ].url );
            boaWrite(wp, "</tr>\n" );
        }
    }
#endif	
    // dial plan
#ifdef CONFIG_RTK_VOIP_DIALPLAN
	else if (strcmp(argv[0], "display_dialplan_title")==0) 
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		/* FXS channel has no dialplan, but FXO still need it. */
		if( RTK_VOIP_IS_SLIC_CH( voip_port, g_VoIP_Feature ) )
			;
		else
#endif
		{
			boaWrite(wp, "<p><b>%s</b>", multilang(LANG_DIAL_PLAN));
		}
	}
    else if (strcmp(argv[0], "display_dialplan")==0) 
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		/* FXS channel has no dialplan, but FXO still need it. */
		if( RTK_VOIP_IS_SLIC_CH( voip_port, g_VoIP_Feature ) )
			boaWrite(wp, "style=\"display:none\"");
		else
#endif
		{
			boaWrite(wp, "%s", "");
		}
	} 
    else if (strcmp(argv[0], "dialplan")==0)
		boaWrite(wp, "%s", pCfg->dialplan);
	else if (strcmp(argv[0], "ReplaceRuleOption")==0) 
    {
    	boaWrite(wp, "<input type=\"radio\" name=\"ReplaceRuleOption\" value=1 %s >%s",
    				( pCfg ->replace_rule_option ? "checked" : "" ), multilang(LANG_ON));
    	boaWrite(wp, "<input type=\"radio\" name=\"ReplaceRuleOption\" value=0 %s>%s",
    				( !pCfg ->replace_rule_option ? "checked" : "" ), multilang(LANG_OFF));
	}
    else if (strcmp(argv[0], "digitmap_enable")==0) 
    {
    	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_enable\" value=1 %s>%s",
    				( pCfg ->digitmap_enable ? "checked" : "" ), multilang(LANG_ON));
    	boaWrite(wp, "<input type=\"radio\" name=\"digitmap_enable\" value=0 %s>%s",
    				( !pCfg ->digitmap_enable ? "checked" : "" ), multilang(LANG_OFF));
	} 
    else if (strcmp(argv[0], "ReplaceRuleSource")==0)
		boaWrite(wp, "%s", pCfg->replace_rule_source);
	else if (strcmp(argv[0], "ReplaceRuleTarget")==0)
		boaWrite(wp, "%s", pCfg->replace_rule_target);
	else if (strcmp(argv[0], "AutoPrefix")==0)
		boaWrite(wp, "%s", pCfg->auto_prefix);
	else if (strcmp(argv[0], "PrefixUnsetPlan")==0)
		boaWrite(wp, "%s", pCfg->prefix_unset_plan);
#else
	else if (strcmp(argv[0], "display_dialplan_title")==0)
		boaWrite(wp, "%s", "");
  	else if (strcmp(argv[0], "display_dialplan")==0)
  		boaWrite(wp, "%s", "style=\"display:none\"");
  	else if (strcmp(argv[0], "dialplan")==0)
		boaWrite(wp, "%s", "");
  	else if (strcmp(argv[0], "ReplaceRuleOption")==0)
    	boaWrite(wp, "%s", "");
  	else if (strcmp(argv[0], "digitmap_enable")==0)
	   	boaWrite(wp, "%s", "");
	else if (strcmp(argv[0], "ReplaceRuleSource")==0)
		boaWrite(wp, "%s", "");
	else if (strcmp(argv[0], "ReplaceRuleTarget")==0)
		boaWrite(wp, "%s", "");
	else if (strcmp(argv[0], "AutoPrefix")==0)
		boaWrite(wp, "%s", "");
	else if (strcmp(argv[0], "PrefixUnsetPlan")==0)
		boaWrite(wp, "%s", "");
#endif /* CONFIG_RTK_VOIP_DIALPLAN */

#if 0  // PSTN Routing Prefix
	else if (strcmp(argv[0], "PSTNRoutingPrefix")==0)
		boaWrite(wp, "%s", pCfg->PSTN_routing_prefix );
	else if (strcmp(argv[0], "PSTNRoutingPrefixDisabled")==0)
		boaWrite(wp, "%s", ( RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ) ? "disabled" : "") );
#endif

	// DSP
	else if (strcmp(argv[0], "codec_var") == 0)
		asp_sip_codec_var(wp, pCfg);
	else if (strcmp(argv[0], "codec") == 0)
		asp_sip_codec(wp, pCfg);
	else if (strcmp(argv[0], "codec_opt") == 0)
		asp_sip_codec_opt(wp, pCfg);
	// RTP Redundant
#if defined(SUPPORT_RTP_REDUNDANT_APP)
	else if(strcmp(argv[0], "RTP_RED_BUILD") == 0)
		boaWrite(wp, "");
	else if(strcmp(argv[0], "rtp_redundant_codec_options") == 0) {
		boaWrite(wp,
			"<option value=-1>%s</option>"
			"<option value=0 %s>PCM u-law</option>"
			"<option value=8 %s>PCM a-law</option>"
#ifdef CONFIG_RTK_VOIP_G729AB
			"<option value=18 %s>G.729</option>"
#endif
//			,multilang(LANG_DISABLE)
			,"Disabled"
			,pCfg->rtp_redundant_codec == 0 ? "selected" : ""
			,pCfg->rtp_redundant_codec == 8 ? "selected" : ""
#ifdef CONFIG_RTK_VOIP_G729AB
			,pCfg->rtp_redundant_codec == 18 ? "selected" : ""
#endif
		);
	} else if(strcmp(argv[0], "rtp_redundant_payload_type") == 0)
		boaWrite(wp, "%d", pCfg->rtp_redundant_payload_type);
#else
	else if(strcmp(argv[0], "RTP_RED_BUILD") == 0)
		boaWrite(wp, "style=\"display:none\"");
	else if(strcmp(argv[0], "rtp_redundant_codec_options") == 0)
		boaWrite(wp, "");
	else if(strcmp(argv[0], "rtp_redundant_payload_type") == 0)
		boaWrite(wp, "%d", pCfg->rtp_redundant_payload_type);
#endif /* SUPPORT_RTP_REDUNDANT_APP */
	// Hot Line
    else if (strcmp(argv[0], "hotline_option_display_title") == 0)
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
    	/* FXS channel has no hotline option, but FXO still need it. */
    	if( RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) )
    		;
    	else
#endif
    	{
    		boaWrite(wp, "<p>\n<b>%s</b>\n", multilang(LANG_HOT_LINE));
    	}
    }
    else if (strcmp(argv[0], "hotline_option_display") == 0)
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
    	/* FXS channel has no hotline option, but FXO still need it. */
    	if( RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) )
    		boaWrite(wp, "style=\"display:none\"");
#endif
    }
    else if (strcmp(argv[0], "hotline_enable") == 0)
    	boaWrite(wp, "%s", (pCfg->hotline_enable) ? "checked" : "");
    else if (strcmp(argv[0], "hotline_number") == 0)
    	boaWrite(wp, "%s", pCfg->hotline_number);
    // DND 
    else if (strcmp(argv[0], "not_ipphone_option_start")==0)
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
    	boaWrite( wp, "<!--\n" );
#endif
    }
    else if (strcmp(argv[0], "not_dect_port_option")==0)
    {
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
    	if( RTK_VOIP_IS_DECT_CH( port, g_VoIP_Feature ) )
    		boaWrite(wp, "style=\"display:none\"");
#endif
    }
    else if (strcmp(argv[0], "not_ipphone_option_end")==0)
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
    	boaWrite( wp, "-->\n" );
#endif
    }
    else if (strcmp(argv[0], "not_ipphone_table")==0)
    {
#ifdef CONFIG_RTK_VOIP_IP_PHONE
    	boaWrite(wp, "style=\"display:none\"");
#endif
    }
    else if (strcmp(argv[0], "dnd_always") == 0)
    	boaWrite(wp, "%s", (pCfg->dnd_mode == 2) ? "checked" : "");
    else if (strcmp(argv[0], "dnd_enable") == 0)
    	boaWrite(wp, "%s", (pCfg->dnd_mode == 1) ? "checked" : "");
    else if (strcmp(argv[0], "dnd_disable") == 0)
   		boaWrite(wp, "%s", (pCfg->dnd_mode == 0) ? "checked" : "");
   	else if (strcmp(argv[0], "dnd_from_hour") == 0)
    	boaWrite(wp, "%.2d", pCfg->dnd_from_hour);
   	else if (strcmp(argv[0], "dnd_from_min") == 0)
  		boaWrite(wp, "%.2d", pCfg->dnd_from_min);
   	else if (strcmp(argv[0], "dnd_to_hour") == 0)
    	boaWrite(wp, "%.2d", pCfg->dnd_to_hour);
   	else if (strcmp(argv[0], "dnd_to_min") == 0)
    	boaWrite(wp, "%.2d", pCfg->dnd_to_min);
    // Alarm
    else if (strcmp(argv[0], "alarm_enable")==0)
    	boaWrite(wp, "%s", ( pCfg->alarm_enable ? "checked" : "") );
    else if (strcmp(argv[0], "alarm_disabled")==0)
    	boaWrite(wp, "%s", ( !RTK_VOIP_IS_SLIC_CH( voip_port, g_VoIP_Feature ) ? "disabled" : "") );
    else if (strcmp(argv[0], "alarm_hh")==0)
    	boaWrite(wp, "%u", pCfg->alarm_time_hh);
   	else if (strcmp(argv[0], "alarm_mm")==0)
    	boaWrite(wp, "%u", pCfg->alarm_time_mm);
#if 0
	else if (strcmp(argv[0], "slic_txVolumne")==0)
		asp_volumne(wp, pCfg->slic_txVolumne);
	else if (strcmp(argv[0], "slic_rxVolumne")==0)
		asp_volumne(wp, pCfg->slic_rxVolumne);

	// ANSTONE
	else if (strcmp(argv[0], "useANSTONE_CNG_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x1) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_CNG_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x100) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANS_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x2) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANS_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x200) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSAM_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x4) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSAM_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x400) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSBAR_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x8) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSBAR_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x800) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSAMBAR_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x10) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_ANSAMBAR_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x1000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELLANS_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x20) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELLANS_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x2000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V22ANS_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x40) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V22ANS_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x4000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V8bis_Cre_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x80) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V8bis_Cre_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x8000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21flag_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x10000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21flag_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x20000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21DIS_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x40000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21DIS_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x80000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21DCN_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x100000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21DCN_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x200000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21CH2_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x400000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21CH2_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x800000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21CH1_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x1000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V21CH1_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x2000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V23_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x4000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_V23_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x8000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELL202_AP_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x10000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELL202_AP_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x20000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELL202_CP_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x40000000) ? "checked" : "");
	else if (strcmp(argv[0], "useANSTONE_BELL202_CP_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone&0x80000000) ? "checked" : "");

	// ECM mode setting
	else if (strcmp(argv[0], "useECM_PPSMPS_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x1) ? "checked" : "");
	else if (strcmp(argv[0], "useECM_PPSMPS_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x2) ? "checked" : "");
	else if (strcmp(argv[0], "useECM_PPSEOP_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x4) ? "checked" : "");
	else if (strcmp(argv[0], "useECM_PPSEOP_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x8) ? "checked" : "");
	else if (strcmp(argv[0], "useECM_RNR_TDM")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x10) ? "checked" : "");
	else if (strcmp(argv[0], "useECM_RNR_IP")==0)
		boaWrite(wp, "%s", (pCfg->anstone_2&0x20) ? "checked" : "");
#endif	
	else if (strcmp(argv[0], "not_ipphone_option_start")==0)
	{
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		boaWrite( wp, "<!--\n" );
#endif
	}
	else if (strcmp(argv[0], "not_ipphone_option_end")==0)
	{
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		boaWrite( wp, "-->\n" );
#endif
	}
	else if (strcmp(argv[0], "not_ipphone_table")==0)
	{
#ifdef CONFIG_RTK_VOIP_IP_PHONE
		boaWrite(wp, "style=\"display:none\"");
#endif
	}
	else
	{
		return -1;
	}

	return 0;
}

void asp_voip_GeneralSet(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i;
	char *ptr;
	char szFrameSize[12], szPrecedence[12];
	char redirect_url[50];
	int voip_port;
	char szName[20];

	if (web_flash_get(&pVoIPCfg) != 0)
		return;
	
	rtk_GetVoIPFeature();
	
	voip_port = atoi(boaGetVar(wp, "voipPort", "0"));
	if (voip_port < 0 || voip_port >= g_VoIP_Ports)
		return;

	pCfg = &pVoIPCfg->ports[voip_port];

	pCfg->default_proxy = atoi(boaGetVar(wp, "default_proxy", "0"));

	/* Sip Proxy */
	for (i=0; i< MAX_PROXY; i++)
	{
		/* Sip Account */
		sprintf(szName, "display%d", i);
		strcpy(pCfg->proxies[i].display_name, boaGetVar(wp, szName, ""));
		sprintf(szName, "number%d", i);
		strcpy(pCfg->proxies[i].number, boaGetVar(wp, szName, ""));
		sprintf(szName, "loginID%d", i);
		strcpy(pCfg->proxies[i].login_id, boaGetVar(wp, szName, ""));
		sprintf(szName, "password%d", i);
		strcpy(pCfg->proxies[i].password, boaGetVar(wp, szName, ""));

		/* Register Server */
		pCfg->proxies[i].enable = 0;

		sprintf(szName, "proxyEnable%d", i);
		if (gstrcmp(boaGetVar(wp, szName, ""), "on") == 0)
			pCfg->proxies[i].enable |= PROXY_ENABLED;

		sprintf(szName, "proxyNortel%d", i);
		if (gstrcmp(boaGetVar(wp, szName, ""), "on") == 0)
			pCfg->proxies[i].enable |= PROXY_NORTEL;

		sprintf(szName, "proxyAddr%d", i);
		strcpy(pCfg->proxies[i].addr, boaGetVar(wp, szName, ""));

		sprintf(szName, "proxyPort%d", i);
		pCfg->proxies[i].port = atoi(boaGetVar(wp, szName, "5060"));
		if (pCfg->proxies[i].port == 0)
			pCfg->proxies[i].port = 5060;

        sprintf(szName, "proxy%d_subscribe_enable", i);
		if (gstrcmp(boaGetVar(wp, szName, ""), "on") == 0)
			pCfg->proxies[i].enable |= PROXY_SUBSCRIBE; 
        else
            pCfg->proxies[i].enable &= ~PROXY_SUBSCRIBE;

		sprintf(szName, "domain_name%d", i);
		strcpy(pCfg->proxies[i].domain_name, boaGetVar(wp, szName, ""));

		sprintf(szName, "regExpiry%d", i);
		pCfg->proxies[i].reg_expire = atoi(boaGetVar(wp, szName, "60"));

#ifdef CONFIG_RTK_VOIP_SIP_TLS
		sprintf(szName, "sipTLSEnable%d", i);
		if (gstrcmp(boaGetVar(wp, szName, ""), "on") == 0)
			pCfg->proxies[i].siptls_enable=1;
		else
			pCfg->proxies[i].siptls_enable=0;
#endif /*CONFIG_RTK_VOIP_SIP_TLS*/

		/* NAT Travsersal Server */
		sprintf(szName, "obEnable%d", i);
		pCfg->proxies[i].outbound_enable = !gstrcmp(boaGetVar(wp, szName, ""), "on");
		sprintf(szName, "obProxyPort%d", i);
		pCfg->proxies[i].outbound_port = atoi(boaGetVar(wp, szName, "5060"));
		if (pCfg->proxies[i].outbound_port == 0)
			pCfg->proxies[i].outbound_port = 5060;

		sprintf(szName, "obProxyAddr%d", i);
		strcpy(pCfg->proxies[i].outbound_addr, boaGetVar(wp, szName, ""));
	
        // Session Timer on/ff
		sprintf(szName, "sessionEnable%d", i);
		if (gstrcmp(boaGetVar(wp, szName, ""), "on") == 0){
			//get vaule
			sprintf(szName, "sessionExpiry%d", i);
			pCfg->proxies[i].SessionUpdateTimer = atoi(boaGetVar(wp, szName, "90"));

		}else{
			pCfg->proxies[i].SessionUpdateTimer =0;

		}	
	}
#if 0
	/* NAT Traversal */
	pCfg->stun_enable = !gstrcmp(boaGetVar(wp, "stunEnable", ""), "on");
	pCfg->stun_port	= atoi(boaGetVar(wp, "stunPort", "3478"));
	strcpy(pCfg->stun_addr, boaGetVar(wp, "stunAddr", ""));
#endif
	/* Advanced */
	pCfg->sip_port 	= atoi(boaGetVar(wp, "sipPort", "5060"));
	pCfg->media_port 	= atoi(boaGetVar(wp, "rtpPort", "9000"));
	pCfg->T38_port = atoi(boaGetVar(wp, "rtpPort", "9000"));

	ptr	 = boaGetVar(wp, "dtmfMode", "");
	for(i=0; i<DTMF_MAX; i++)
	{
		if (!gstrcmp(ptr, dtmf[i]))
			break;
	}
	if (i == DTMF_MAX)
		i = DTMF_INBAND;

	pCfg->dtmf_mode 		= i;
	pCfg->dtmf_2833_pt 		= atoi(boaGetVar(wp, "dtmf_2833_pt", "96"));
	pCfg->dtmf_2833_pi 		= atoi(boaGetVar(wp, "dtmf_2833_pi", "10"));
	pCfg->fax_modem_2833_pt_same_dtmf = !gstrcmp(boaGetVar(wp, "fax_modem_2833_pt_same_dtmf", ""), "on");
	pCfg->fax_modem_2833_pt		= atoi(boaGetVar(wp, "fax_modem_2833_pt", "101"));
	pCfg->fax_modem_2833_pi		= atoi(boaGetVar(wp, "fax_modem_2833_pi", "10"));
	pCfg->sip_info_duration		= atoi(boaGetVar(wp, "sipInfo_duration", "250"));
	pCfg->call_waiting_enable = !gstrcmp(boaGetVar(wp, "call_waiting", ""), "on");
	pCfg->call_waiting_cid = !gstrcmp(boaGetVar(wp, "call_waiting_cid", ""), "on");

	pCfg->fxsport_flag &= ~FXS_REJECT_DIRECT_IP_CALL;

//	fprintf(stderr, "reject_direct_ip_call %d \n",atoi(boaGetVar(wp, "reject_direct_ip_call", "")));
//	fprintf(stderr, "caller_id_hidden %d \n",atoi(boaGetVar(wp, "caller_id_hidden", "")));
//	fprintf(stderr, "reject_direct_ip_call %s \n",boaGetVar(wp, "reject_direct_ip_call", ""));
//	fprintf(stderr, "caller_id_hidden %s \n",boaGetVar(wp, "caller_id_hidden", ""));
		
	if(!gstrcmp(boaGetVar(wp, "reject_direct_ip_call", ""), "on")){
		pCfg->fxsport_flag |= FXS_REJECT_DIRECT_IP_CALL;
	}
	
	pCfg->fxsport_flag &= ~FXS_CALLERID_HIDDEN;
	if(!gstrcmp(boaGetVar(wp, "caller_id_hidden", ""), "on")){
		pCfg->fxsport_flag |= FXS_CALLERID_HIDDEN;
	}

	pCfg->fxsport_flag |= FXS_CALL_TRANSFER_disable;
	if(!gstrcmp(boaGetVar(wp, "Call_Transfer", ""), "on")){
		pCfg->fxsport_flag &= ~FXS_CALL_TRANSFER_disable;
	}

	pCfg->fxsport_flag |=FXS_3WAY_DISABLE;
	if(!gstrcmp(boaGetVar(wp, "3PTY_Conference", ""), "on")){
		pCfg->fxsport_flag &= ~FXS_3WAY_DISABLE;
	}
	#if 1
	if( (atoi( boaGetVar(wp, "Conference_server_cpe", "")))==0)// on server
	{
		pCfg->fxsport_flag |= FXS_3WAY_ON_SERVER;
	}else //on CPE
	{
		pCfg->fxsport_flag &= ~FXS_3WAY_ON_SERVER;
	}
	#endif
	strcpy(pVoIPCfg->conference_uri, boaGetVar(wp, "conference_uri", ""));	
		
//	pCfg->direct_ip_call = gstrcmp(boaGetVar(wp, "reject_direct_ip_call", ""), "on");

	/* Forward Mode */
	//pCfg->uc_forward_enable = !gstrcmp(boaGetVar(wp, "CFAll", ""), "on");
	pCfg ->uc_forward_enable = atoi( boaGetVar(wp, "CFAll", "") );
	strcpy(pCfg->uc_forward, boaGetVar(wp, "CFAll_No", ""));

	//pCfg->busy_forward_enable = !gstrcmp(boaGetVar(wp, "CFBusy", ""), "on");
	pCfg ->busy_forward_enable = atoi( boaGetVar(wp, "CFBusy", "") );
	strcpy(pCfg->busy_forward, boaGetVar(wp, "CFBusy_No", ""));

#ifdef FXO_REDIAL
	if (!RTK_VOIP_IS_DAA_CH( voip_port, g_VoIP_Feature ))
	{
		pCfg->na_forward_enable = atoi(boaGetVar(wp, "CFNoAns", ""));
		pCfg->na_forward_time = atoi(boaGetVar(wp, "CFNoAns_Time", ""));
		strcpy(pCfg->na_forward, boaGetVar(wp, "CFNoAns_No", ""));
		if (voip_port == 0) // if FXS0
		{
			pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_enable = atoi(boaGetVar(wp, "PSTN_CFNoAns", ""));
			strcpy(pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward, boaGetVar(wp, "PSTN_CFNoAns_No", ""));
			pVoIPCfg->ports[RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature )].na_forward_time = atoi(boaGetVar(wp, "PSTN_CFNoAns_Time", ""));
		}
	}
#else
	//pCfg->na_forward_enable = !gstrcmp(boaGetVar(wp, "CFNoAns", ""), "on");
	pCfg ->na_forward_enable = atoi( boaGetVar(wp, "CFNoAns", "") );
	pCfg->na_forward_time	  	= atoi(boaGetVar(wp, "CFNoAns_Time", ""));
	strcpy(pCfg->na_forward, boaGetVar(wp, "CFNoAns_No", ""));
#endif
#if 0 //remove unnecessary parameter
	/* Speed Dial */
	for (i=0; i<MAX_SPEED_DIAL; i++)
	{
		char szBuf[20];

		sprintf(szBuf, "spd_name%d", i);
		strcpy(pCfg->speed_dial[i].name, boaGetVar(wp, szBuf, ""));
		sprintf(szBuf, "spd_url%d", i);
		strcpy(pCfg->speed_dial[i].url, boaGetVar(wp, szBuf, ""));
	}
#endif
#ifdef CONFIG_RTK_VOIP_DIALPLAN
	/* Dial Plan */
	strcpy((char *) pCfg->dialplan, boaGetVar(wp, "dialplan", ""));
	//pCfg ->replace_rule_option = atoi( boaGetVar(wp, "ReplaceRuleOption", "") );
	pCfg ->digitmap_enable =  atoi( boaGetVar(wp, "digitmap_enable", "") );
	//strcpy((char *) pCfg->replace_rule_source, boaGetVar(wp, "ReplaceRuleSource", ""));
	//strcpy((char *) pCfg->replace_rule_target, boaGetVar(wp, "ReplaceRuleTarget", ""));
	//strcpy((char *) pCfg->auto_prefix, boaGetVar(wp, "AutoPrefix", ""));
	//strcpy((char *) pCfg->prefix_unset_plan, boaGetVar(wp, "PrefixUnsetPlan", ""));
#endif

	/* PSTN Routing Prefix */
	strcpy((char *) pCfg->PSTN_routing_prefix, boaGetVar(wp, "PSTNRoutingPrefix", ""));

	/* DSP */
	for(i=0; i<SUPPORTED_CODEC_MAX; i++)
	{
		sprintf(szFrameSize, "frameSize%d", i);
		pCfg->frame_size[i] = atoi(boaGetVar(wp, szFrameSize, "0"));
		sprintf(szPrecedence, "preced%d", i);
		pCfg->precedence[i] = atoi(boaGetVar(wp, szPrecedence, "-1"));
	}

	/*
	 * Codec Options
	 */
#ifdef CONFIG_RTK_VOIP_G7231
	pCfg->g7231_rate = (gstrcmp(boaGetVar(wp, "g7231Rate", ""), "5.3k")) ? G7231_RATE63 :  G7231_RATE53;
#endif
#ifdef CONFIG_RTK_VOIP_G726
	ptr = boaGetVar(wp, "g726_packing", "");
	for(i=0; i<G726_PACK_MAX; i++)
	{
		if (!gstrcmp(ptr, g726pack[i]))
			break;
	}
	if (i == G726_PACK_MAX)
		i = G726_PACK_RIGHT;

	pCfg->g726_packing = i;
#endif
#ifdef CONFIG_RTK_VOIP_ILBC
	pCfg->iLBC_mode = (gstrcmp(boaGetVar(wp, "iLBC_mode", ""), "30ms") == 0) ? ILBC_30MS : ILBC_20MS ;
#endif
#ifdef CONFIG_RTK_VOIP_SPEEX_NB
	pCfg->speex_nb_rate = atoi(boaGetVar(wp, "speex_nb_rate", "2"));
#endif
#ifdef CONFIG_RTK_VOIP_G7111
	memset(pCfg->g7111_precedence, 0, sizeof(pCfg->g7111_precedence));
	for (i=0; i<G7111_MODES; i++)
	{
		int pri;

		pri = atoi(boaGetVar(wp, g7111_modes[i], "-1"));
		if (pri >=0 && pri < G7111_MODES)
			pCfg->g7111_precedence[pri] = i + 1;
	}
#endif

	/*
	 * RTP Redundant
	 */
#ifdef SUPPORT_RTP_REDUNDANT_APP
	pCfg->rtp_redundant_codec = atoi( boaGetVar(wp, "rtp_redundant_codec", "-1"));
	pCfg->rtp_redundant_payload_type = atoi( boaGetVar(wp, "rtp_redundant_payload_type", "121"));
#endif

    // Hot line
	pCfg->hotline_enable = strcmp(boaGetVar(wp, "hotline_enable", ""), "on") == 0;
	if (pCfg->hotline_enable)
		strcpy(pCfg->hotline_number, boaGetVar(wp, "hotline_number", ""));

	// DND
	pCfg->dnd_mode = atoi(boaGetVar(wp, "dnd_mode", "0"));
	if (pCfg->dnd_mode == 1)
	{
		pCfg->dnd_from_hour = atoi(boaGetVar(wp, "dnd_from_hour", "0"));
		pCfg->dnd_from_min = atoi(boaGetVar(wp, "dnd_from_min", "0"));
		pCfg->dnd_to_hour = atoi(boaGetVar(wp, "dnd_to_hour", "0"));
		pCfg->dnd_to_min = atoi(boaGetVar(wp, "dnd_to_min", "0"));
	}

    // Alarm
	pCfg->alarm_enable = strcmp(boaGetVar(wp, "alarm_enable", ""), "on") == 0;
	if( pCfg->alarm_enable ) {
		pCfg->alarm_time_hh = atoi(boaGetVar(wp, "alarm_hh", "0"));
		pCfg->alarm_time_mm = atoi(boaGetVar(wp, "alarm_mm", "0"));
		//pCfg->alarm_ring_last_day = 0;
		//pCfg->alarm_ring_defer = 0;
	} else {
		pCfg->alarm_time_hh = 0;
		pCfg->alarm_time_mm = 0;
 		//pCfg->alarm_ring_last_day = 0;
		//pCfg->alarm_ring_defer = 0;
	}

#if 0
	i=0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_CNG_TDM", ""), "on"))?0x1:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_CNG_IP", ""), "on"))?0x100:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANS_TDM", ""), "on"))?0x2:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANS_IP", ""), "on"))?0x200:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSAM_TDM", ""), "on"))?0x4:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSAM_IP", ""), "on"))?0x400:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSBAR_TDM", ""), "on"))?0x8:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSBAR_IP", ""), "on"))?0x800:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSAMBAR_TDM", ""), "on"))?0x10:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_ANSAMBAR_IP", ""), "on"))?0x1000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELLANS_TDM", ""), "on"))?0x20:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELLANS_IP", ""), "on"))?0x2000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V22ANS_TDM", ""), "on"))?0x40:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V22ANS_IP", ""), "on"))?0x4000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V8bis_Cre_TDM", ""), "on"))?0x80:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V8bis_Cre_IP", ""), "on"))?0x8000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21flag_TDM", ""), "on"))?0x10000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21flag_IP", ""), "on"))?0x20000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21DIS_TDM", ""), "on"))?0x40000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21DIS_IP", ""), "on"))?0x80000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21DCN_TDM", ""), "on"))?0x100000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21DCN_IP", ""), "on"))?0x200000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21CH2_TDM", ""), "on"))?0x400000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21CH2_IP", ""), "on"))?0x800000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21CH1_TDM", ""), "on"))?0x1000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V21CH1_IP", ""), "on"))?0x2000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V23_TDM", ""), "on"))?0x4000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_V23_IP", ""), "on"))?0x8000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELL202_AP_TDM", ""), "on"))?0x10000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELL202_AP_IP", ""), "on"))?0x20000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELL202_CP_TDM", ""), "on"))?0x40000000:0;
	i|= (!gstrcmp(boaGetVar(wp, "useANSTONE_BELL202_CP_IP", ""), "on"))?0x80000000:0;
	pCfg->anstone=i;

	i = 0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_PPSMPS_TDM", ""), "on"))?0x1:0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_PPSMPS_IP", ""), "on"))?0x2:0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_PPSEOP_TDM", ""), "on"))?0x4:0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_PPSEOP_IP", ""), "on"))?0x8:0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_RNR_TDM", ""), "on"))?0x10:0;
	i|= (!gstrcmp(boaGetVar(wp, "useECM_RNR_IP", ""), "on"))?0x20:0;	

	pCfg->anstone_2=i;

	pCfg->cid_fsk_gen_mode = !gstrcmp(boaGetVar(wp, "SoftFskGen", ""), "on");
#endif

	web_flash_set(pVoIPCfg);
#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
	cmd_set_dns_config(NULL);
	
	restart_dnsrelay();
#endif	

	voip_setup_iptable(0);
	setupDMZ();
	
#ifdef NEW_IP_QOS_SUPPORT
	take_qos_effect();
#endif
#ifdef CONFIG_USER_IP_QOS_3
	take_qos_effect_v3();
#endif
#ifndef CONFIG_GENERAL_WEB
	sprintf(redirect_url, "/voip_general.asp?port=%d", voip_port);
#else
	sprintf(redirect_url, "/voip_general_new_web.asp?port=%d", voip_port);
#endif
#ifdef REBOOT_CHECK
	OK_MSG(redirect_url);
#else
	web_restart_solar();
	boaRedirect(wp, redirect_url);
#endif
}

#ifdef CONFIG_APP_BOA
int asp_voip_DialPlanGet(request * wp, int argc, char **argv)
#else
int asp_voip_DialPlanGet(int ejid, request * wp, int argc, char **argv)
#endif
{
	return 0;
}

void asp_voip_DialPlanSet(request * wp, char *path, char *query)
{
#ifdef REBOOT_CHECK
	OK_MSG("/dialplan.asp");
#else
	boaRedirect(wp, "/dialplan.asp");
#endif
}

