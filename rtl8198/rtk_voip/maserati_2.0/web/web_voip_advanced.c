#include <stdio.h>
#include "web_voip.h"

char cid[CID_MAX][25] = {"FSK_BELLCORE", "FSK_ETSI", "FSK_BT", "FSK_NTT", "DTMF"};
char cid_dtmf[CID_DTMF_MAX][8] = {"DTMF_A","DTMF_B","DTMF_C","DTMF_D"};
char fax_modem_det_string[FAX_MODEM_DET_MAX][7] = {"AUTO", "FAX", "MODEM", "AUTO_2"};

void asp_jitterDelay(request * wp, int nJitterDelay)
{
	int i;

	if( nJitterDelay < 2 || nJitterDelay > 40 )
		nJitterDelay = 2;

	for( i = 2; i <= 40; i ++ ) {
		if( i == nJitterDelay ){
			boaWrite(wp, "<option value=%d selected>%d</option>", i, i * 10);
        } else{
			boaWrite(wp, "<option value=%d>%d</option>", i, i * 10);
        }
	}
}

void asp_maxDelay(request * wp, int nMaxDelay)
{
	int i;

#if 1
	if( nMaxDelay < 13 )
		nMaxDelay = 13;
	else if( nMaxDelay > 60 )
		nMaxDelay = 60;

	for(i=13; i<=60; i++)
	{
		if (nMaxDelay == i)
			boaWrite(wp, "<option value=%d selected>%d</option>", i, i * 10);
		else
			boaWrite(wp, "<option value=%d >%d</option>", i, i * 10);
	}
#else
	for(i=60; i<=180; i+=30)
	{
		if (nMaxDelay == i)
			boaWrite(wp, "<option selected>%d</option>", i);
		else
			boaWrite(wp, "<option>%d</option>", i);
	}
#endif
}

void asp_jitterFactor(request * wp, int nJitterFactor)
{
	int i;

	if( nJitterFactor < 0 || nJitterFactor > 13 )
		nJitterFactor = 7;

	for( i = 0; i <= 13; i ++ ) {
		if( i == nJitterFactor )
			boaWrite(wp, "<option value=%d selected>%d</option>", i, i );
		else
			boaWrite(wp, "<option value=%d>%d</option>", i, i );
	}
}

void asp_agc_gainup(request * wp, int nagc_db)
{
	int i;

	for(i=0; i<9; i++)
	{
		if (i == nagc_db)
			boaWrite(wp, "<option selected>%d</option>", (i+1));
		else
			boaWrite(wp, "<option>%d</option>", (i+1));
	}

}

void asp_agc_gaindown(request * wp, int nagc_db)
{
	int i;

	for(i=0; i<9; i++)
	{
		if (i == nagc_db)
			boaWrite(wp, "<option selected>-%d</option>", (i+1));
		else
			boaWrite(wp, "<option>-%d</option>", (i+1));
	}

}

#ifdef CONFIG_APP_BOA
int asp_voip_AdvancedGet(request *wp, int argc, char **argv)
#else
int asp_voip_AdvancedGet(int ejid, request *wp, int argc, char **argv)
#endif
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg;
	int i, port;
	int voip_port;
#ifdef CONFIG_RTK_VOIP_SIP_TLS
	int certNum=0;
#endif

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;
	
	rtk_GetVoIPFeature();
	
#if 0
	voip_port = atoi(boaGetVar(wp, "port", "0"));
	if (voip_port < 0 || voip_port >= g_VoIP_Ports)
		return -1;

	pCfg = &pVoIPCfg->ports[voip_port];
#endif
    port = 0;
    //for (port=0; port<1; port++)
    {
        pCfg = &pVoIPCfg->ports[port];
        pCfg->default_proxy = atoi(boaGetVar(wp, "default_proxy", "0"));

    	if (strcmp(argv[0], "voip_port")==0)
    	{
    		;//boaWrite(wp, "%d", voip_port);
    	}
#ifdef SUPPORT_VOICE_QOS
    	else if (strcmp(argv[0], "display_voice_qos")==0)
    		boaWrite(wp, "%s", "");
    	else if (strcmp(argv[0], "voice_qos")==0)
    	{
            for (i=0; i<DSCP_MAX ;i++)
    		{
    			if (i == pCfg->voice_qos)
    				boaWrite(wp, "<option selected>%s</option>", dscp[i]);
    			else
    				boaWrite(wp, "<option>%s</option>", dscp[i]);
    		}
    	}
#else
    	else if (strcmp(argv[0], "display_voice_qos")==0)
    		boaWrite(wp, "%s", "style=\"display:none\"");
    	else if (strcmp(argv[0], "voice_qos")==0)
    	{
            boaWrite(wp, "%s", "");
    	}
#endif
        // V.152
    	else if(strcmp(argv[0], "useV152")==0)
    		boaWrite(wp, "%s", (pCfg->useV152) ? "checked" : "");
    	else if(strcmp(argv[0], "V152_payload_type")==0)
    		boaWrite(wp, "%d", pCfg->v152_payload_type);
    	else if(strcmp(argv[0], "V152_codec_type")==0)
    		boaWrite(wp, "%d", pCfg->v152_codec_type);
    	else if(strcmp(argv[0], "V152_codec_type_options")==0)
    	{
    		static const struct {
    			unsigned char codec_type;
    			const char *codec_type_string;
    		} const v152_codec_options[] = {
    			{ 0, "PCM u-law" },
    			{ 8, "PCM a-law" },
    		};
            #define SIZE_OF_V152_OPTIONS	( sizeof( v152_codec_options ) / sizeof( v152_codec_options[ 0 ] ) )

    		for( i = 0; i < SIZE_OF_V152_OPTIONS; i ++ ) {
    			if( v152_codec_options[ i ].codec_type == pCfg->v152_codec_type ) {
    				boaWrite( wp, "<option value=%d selected>%s</option>", 
                              v152_codec_options[ i ].codec_type, v152_codec_options[ i ].codec_type_string );
                } else{
    				boaWrite( wp, "<option value=%d>%s</option>", 
                              v152_codec_options[ i ].codec_type, v152_codec_options[ i ].codec_type_string );
                }
    		}
    	}
        // T.38 (FAX) 
    	else if (strcmp(argv[0], "not_dect_port_option")==0)
    	{
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_DECT
    		if( RTK_VOIP_IS_DECT_CH( port, g_VoIP_Feature ) ) {
    			boaWrite(wp, "style=\"display:none\"");
    		}
#endif
    	}
#ifdef CONFIG_RTK_VOIP_T38	/* kernel config true */
    	else if (strcmp(argv[0], "T38_BUILD")==0)
    		boaWrite(wp, "%s", "");
    	else if(strcmp(argv[0], "useT38")==0)
    		boaWrite(wp, "%s", (pCfg->useT38) ? "checked" : "");
        /*
    	else if(strcmp(argv[0], "T38_PORT")==0)
            boaWrite(wp, "%d", pCfg->T38_port);       
    	else if(strcmp(argv[0], "t38Ports")==0)
    	{
            for (i=0; i<g_VoIP_Ports; i++)
    		{
                boaWrite(wp, "<input type=hidden id=t38Ports name=t38Ports value=\"%d\">", pVoIPCfg->ports[i].T38_port); 
            }
        } 
        */ 
        else if (strcmp(argv[0], "fax_modem_det_mode") == 0)
    	{
    		for (i=0; i<FAX_MODEM_DET_MAX; i++)
    		{
                if (i == pCfg->fax_modem_det)
    				boaWrite(wp, "<option selected>%s</option>", fax_modem_det_string[i]);
    			else
    				boaWrite(wp, "<option>%s</option>", fax_modem_det_string[i]);
    		}
    	}
        // T.38 (Customize parameters)
    	else if(strcmp(argv[0], "T38ParamEnable")==0)
    		boaWrite(wp, "%s", (pCfg->T38ParamEnable) ? "checked" : "");
    	else if(strcmp(argv[0], "T38MaxBuffer")==0)
    		boaWrite(wp, "%d", pCfg->T38MaxBuffer);
    	else if(strcmp(argv[0], "T38RateMgt")==0) 
        {
    		boaWrite(wp,
    			"<option value=1 %s>%s</option>"
    			"<option value=2 %s>%s</option>"
    			,
    			( pCfg->T38RateMgt == 1 ? "selected" : "" ),
                //multilang(LANG_LOCAL_TCF),
    			"Local TCF",
    			( pCfg->T38RateMgt != 1 ? "selected" : "" ),
                //multilang(LANG_REMOTE_TCF)
    			"Remote TCF"
    			);
    	} 
        else if(strcmp(argv[0], "T38MaxRate")==0) 
        {
    		boaWrite(wp,
    			"<option value=0 %s>2400</option>"
    			"<option value=1 %s>4800</option>"
    			"<option value=2 %s>7200</option>"
    			"<option value=3 %s>9600</option>"
    			"<option value=4 %s>12000</option>"
    			"<option value=5 %s>14400</option>"
    			,
    			( pCfg->T38MaxRate == 0 ? "selected" : "" ),
    			( pCfg->T38MaxRate == 1 ? "selected" : "" ),
    			( pCfg->T38MaxRate == 2 ? "selected" : "" ),
    			( pCfg->T38MaxRate == 3 ? "selected" : "" ),
    			( pCfg->T38MaxRate == 4 ? "selected" : "" ),
    			( pCfg->T38MaxRate >= 5 ? "selected" : "" )
    			);
    	} 
        else if(strcmp(argv[0], "T38EnableECM")==0)
    		boaWrite(wp, "%s", (pCfg->T38EnableECM) ? "checked" : ""); 
        else if(strcmp(argv[0], "T38ECCSignal")==0) 
        {
    		boaWrite(wp,
    			"<option value=0 %s>0</option>"
    			"<option value=1 %s>1</option>"
    			"<option value=2 %s>2</option>"
    			"<option value=3 %s>3</option>"
    			"<option value=4 %s>4</option>"
    			"<option value=5 %s>5</option>"
    			"<option value=6 %s>6</option>"
    			"<option value=7 %s>7</option>"
    			,
    			( pCfg->T38ECCSignal == 0 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 1 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 2 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 3 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 4 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 5 || pCfg->T38ECCSignal > 7 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 6 ? "selected" : "" ),
    			( pCfg->T38ECCSignal == 7 ? "selected" : "" )
    			);
    	} 
        else if(strcmp(argv[0], "T38ECCData")==0) 
        {
    		boaWrite(wp,
    			"<option value=0 %s>0</option>"
    			"<option value=1 %s>1</option>"
    			"<option value=2 %s>2</option>"
    			,
    			( pCfg->T38ECCData == 0 ? "selected" : "" ),
    			( pCfg->T38ECCData == 1 ? "selected" : "" ),
    			( pCfg->T38ECCData >= 2 ? "selected" : "" )
    			);
    	} 
        else if(strcmp(argv[0], "T38EnableSpoof")==0) 
    		boaWrite(wp, "%s", (pCfg->T38EnableSpoof) ? "checked" : "");
    	else if(strcmp(argv[0], "T38DuplicateNum")==0) 
        {
    		boaWrite(wp,
    			"<option value=0 %s>0</option>"
    			"<option value=1 %s>1</option>"
    			"<option value=2 %s>2</option>"
    			,
    			( pCfg->T38DuplicateNum == 0 ? "selected" : "" ),
    			( pCfg->T38DuplicateNum == 1 ? "selected" : "" ),
    			( pCfg->T38DuplicateNum >= 2 ? "selected" : "" )
    			);
    	}
#else /* CONFIG_RTK_VOIP_T38 */
        // T.38 (FAX)
    	else if (strcmp(argv[0], "T38_BUILD")==0)
    		boaWrite(wp, "%s", "style=\"display:none\"");
    	else if(strcmp(argv[0], "useT38")==0)
    		boaWrite(wp, "%s", "");
        /*
    	else if(strcmp(argv[0], "T38_PORT")==0)
    		boaWrite(wp, "%d", 5000);	 //give 5000 to pass js check. 
        */
    	else if(strcmp(argv[0], "t38Ports")==0)
    	{
    		for (i=0; i<g_VoIP_Ports; i++)
    		{
    			boaWrite(wp, "<input type=hidden id=t38Ports name=t38Ports value=\"%d\">", 5005);
    		}
    	}
        // T.38 (Customize parameters)
    	else if(strcmp(argv[0], "T38ParamEnable")==0)
    		boaWrite(wp, "%s", "");
    	else if(strcmp(argv[0], "T38MaxBuffer")==0)
    		boaWrite(wp, "%d", 500);
    	else if(strcmp(argv[0], "T38RateMgt")==0)
    		boaWrite(wp, "<option value=2 selected>remote TCF</option>");
    	else if(strcmp(argv[0], "T38MaxRate")==0) 
    		boaWrite(wp, "<option value=5 selected>14400</option>");
    	else if(strcmp(argv[0], "T38EnableECM")==0)
    		boaWrite(wp, "%s", "");
    	else if(strcmp(argv[0], "T38ECCSignal")==0)
    		boaWrite(wp, "<option value=5 selected>5</option>");
    	else if(strcmp(argv[0], "T38ECCData")==0) 
    		boaWrite(wp, "<option value=2 selected>2</option>");
    	else if(strcmp(argv[0], "T38EnableSpoof")==0) 
    		boaWrite(wp, "%s", "");
    	else if(strcmp(argv[0], "T38DuplicateNum")==0)
    		boaWrite(wp, "<option value=0 selected>0</option>");
#endif /*CONFIG_RTK_VOIP_T38*/
#if 0
        // VoIP Security
#ifdef CONFIG_RTK_VOIP_SRTP
    	else if(strcmp(argv[0], "SRTP_BUILD") == 0)
    		boaWrite(wp, "%s", "");
    	else if(strcmp(argv[0], "useSRTP") == 0)
    		boaWrite(wp, "%s", (pCfg->security_enable) ? "checked" : "");
#else
    	else if(strcmp(argv[0], "SRTP_BUILD") == 0)
    		boaWrite(wp, "%s", "style=\"display:none\"");
    	else if(strcmp(argv[0], "useSRTP") == 0)
    		boaWrite(wp, "%s", "");
#endif
        // Auth
    	else if (strcmp(argv[0], "offhook_passwd")==0)
    		boaWrite(wp, "%s", pCfg->offhook_passwd);
#endif
        // DSP
    	else if (strcmp(argv[0], "jitterDelay")==0)
    		asp_jitterDelay(wp, pCfg->jitter_delay);
    	else if (strcmp(argv[0], "maxDelay")==0)
    		asp_maxDelay(wp, pCfg->maxDelay);
    	else if (strcmp(argv[0], "jitterFactor")==0)
    		asp_jitterFactor(wp, pCfg->jitter_factor);
        else if (strcmp(argv[0], "echoTail")==0)
            boaWrite(wp, "%d", pCfg->echoTail);
    	else if (strcmp(argv[0], "useLec")==0)
    		boaWrite(wp, "%s", (pCfg->lec) ? "checked" : "");
    	else if (strcmp(argv[0], "useNlp")==0)
    		boaWrite(wp, "%s", (pCfg->nlp) ? "checked" : "");
    	else if (strcmp(argv[0], "useVad")==0)
    		boaWrite(wp, "%s", (pCfg->vad) ? "checked" : "");
    	else if (strcmp(argv[0], "Vad_threshold")==0)
    		boaWrite(wp, "%d", pCfg->vad_thr);
    	else if (strcmp(argv[0], "sid_config_enable") == 0)
    		boaWrite(wp, "%s", (pCfg->sid_gainmode == 0) ? "checked" : "");
    	else if (strcmp(argv[0], "sid_fixed_level") == 0)
    		boaWrite(wp, "%s", (pCfg->sid_gainmode == 1) ? "checked" : "");
    	else if (strcmp(argv[0], "sid_adjust_level") == 0)
    		boaWrite(wp, "%s", (pCfg->sid_gainmode == 2) ? "checked" : "");
    	else if (strcmp(argv[0], "sid_noiselevel")==0)
    		boaWrite(wp, "%d", pCfg->sid_noiselevel);
    	else if (strcmp(argv[0], "sid_noisegain")==0)
    		boaWrite(wp, "%d", pCfg->sid_noisegain);
    	else if (strcmp(argv[0], "useCng")==0)
    		boaWrite(wp, "%s", (pCfg->cng) ? "checked" : "");
    	else if (strcmp(argv[0], "Cng_threshold")==0)
    		boaWrite(wp, "%d", pCfg->cng_thr);
    	else if (strcmp(argv[0], "usePLC")==0)
    		boaWrite(wp, "%s", (pCfg->PLC) ? "checked" : "");
    	else if (strcmp(argv[0], "useRTCP")==0)
    		boaWrite(wp, "%s", (pCfg->RTCP_Interval) ? "checked" : "");
    	else if (strcmp(argv[0], "RTCPInterval")==0)
    		boaWrite(wp, "%u", pCfg->RTCP_Interval);
#ifndef CONFIG_RTK_VOIP_RTCP_XR
        else if (strcmp(argv[0], "rtcp_xr_option")==0)
            boaWrite(wp, "style=\"display:none\"");
#endif
    	else if (strcmp(argv[0], "useRTCPXR")==0)
    		boaWrite(wp, "%s", (pCfg->RTCP_XR) ? "checked" : "");
    	else if (strcmp(argv[0], "useFaxModem2833Relay")==0)
    		boaWrite(wp, "%s", (pCfg->faxmodem_rfc2833&0x1) ? "checked" : "");
    	else if (strcmp(argv[0], "useFaxModemInbandRemoval")==0)
    		boaWrite(wp, "%s", (pCfg->faxmodem_rfc2833&0x2) ? "checked" : "");
    	else if (strcmp(argv[0], "useFaxModem2833RxTonePlay")==0)
    		boaWrite(wp, "%s", (pCfg->faxmodem_rfc2833&0x4) ? "checked" : "");
    	else if (strcmp(argv[0], "CFuseSpeaker")==0)
    		boaWrite(wp, "%s", (pCfg->speaker_agc) ? "checked" : "");
    	else if (strcmp(argv[0], "CF_spk_AGC_level")==0)
    		asp_agc_gainup(wp, pCfg->spk_agc_lvl);
    	else if (strcmp(argv[0], "CF_spk_AGC_up_limit")==0)
    		asp_agc_gainup(wp, pCfg->spk_agc_gu);
    	else if (strcmp(argv[0], "CF_spk_AGC_down_limit")==0)
    		asp_agc_gaindown(wp, pCfg->spk_agc_gd);
    	else if (strcmp(argv[0], "CFuseMIC")==0)
    		boaWrite(wp, "%s", (pCfg->mic_agc) ? "checked" : "");
    	else if (strcmp(argv[0], "CF_mic_AGC_level")==0)
    		asp_agc_gainup(wp, pCfg->mic_agc_lvl);
    	else if (strcmp(argv[0], "CF_mic_AGC_up_limit")==0)
    		asp_agc_gainup(wp, pCfg->mic_agc_gu);
    	else if (strcmp(argv[0], "CF_mic_AGC_down_limit")==0)
    		asp_agc_gaindown(wp, pCfg->mic_agc_gd);
        else if (strcmp(argv[0], "caller_id")==0)
    	{
    		boaWrite(wp, "<select name=caller_id %s>", 
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "");

    		for (i=0; i<CID_MAX ;i++)
    		{
    			if (i == (pCfg->caller_id_mode & 7))
    				boaWrite(wp, "<option selected>%s</option>", cid[i]);
    			else
    				boaWrite(wp, "<option>%s</option>", cid[i]);
    		}

    		boaWrite(wp, "</select>");
    	}
    	else if (strcmp(argv[0], "FSKdatesync")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=FSKdatesync size=20 %s %s>%s",
    			(pCfg->caller_id_mode & 0x080) ? "checked" : "",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_ENABLE));
    	}
    	else if (strcmp(argv[0], "revPolarity")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=revPolarity size=20 %s %s>%s",
    			(pCfg->caller_id_mode & 0x040) ? "checked" : "",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_ENABLE));
    	}
    	else if (strcmp(argv[0], "sRing")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=sRing size=20 %s %s>%s",
    			(pCfg->caller_id_mode & 0x020) ? "checked" : "",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_ENABLE));
    	}
    	else if (strcmp(argv[0], "dualTone")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=dualTone size=20 %s %s>%s",
    			(pCfg->caller_id_mode & 0x010) ? "checked" : "",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "", multilang(LANG_ENABLE));
    	}
    	else if (strcmp(argv[0], "PriorRing")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=PriorRing size=20 %s>%s",
    			(pCfg->caller_id_mode & 0x008) ? "checked" : "", multilang(LANG_ENABLE));
    	}
    	else if (strcmp(argv[0], "cid_dtmfMode_S")==0)
    	{
    		boaWrite(wp, "<select name=cid_dtmfMode_S %s>",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "");

    		for (i=0; i<CID_DTMF_MAX; i++)
    		{
    			if (i == (pCfg->cid_dtmf_mode & 0x03))
    				boaWrite(wp, "<option selected>%s</option>", cid_dtmf[i]);
    			else
    				boaWrite(wp, "<option>%s</option>", cid_dtmf[i]);
    		}

    		boaWrite(wp, "</select>");
    	}
    	else if (strcmp(argv[0], "cid_dtmfMode_E")==0)
    	{
    		boaWrite(wp, "<select name=cid_dtmfMode_E %s>",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "");

    		for (i=0; i<CID_DTMF_MAX; i++)
    		{
    			if (i == ((pCfg->cid_dtmf_mode>>2) & 0x03))
    				boaWrite(wp, "<option selected>%s</option>", cid_dtmf[i]);
    			else
    				boaWrite(wp, "<option>%s</option>", cid_dtmf[i]);
    		}

    		boaWrite(wp, "</select>");
    	}
        else if (strcmp(argv[0], "flash_hook_time")==0)
    	{
    		boaWrite(wp, "<input type=text name=flash_hook_time_min size=4 maxlength=5 value=%d %s>" \
    			" <  %s  < " \
    			"<input type=text name=flash_hook_time size=4 maxlength=5 value=%d %s>",
    			pCfg->flash_hook_time_min,
    			!RTK_VOIP_IS_SLIC_CH( 0 /* voip_port */, g_VoIP_Feature ) ? "disabled" : "",
    			multilang(LANG_FLASH_TIME),
    			pCfg->flash_hook_time,
    			!RTK_VOIP_IS_SLIC_CH( 0 /* voip_port */, g_VoIP_Feature ) ? "disabled" : ""
    		);
    	}
#if 0
    	else if (strcmp(argv[0], "SoftFskGen")==0)
    	{
    		boaWrite(wp, "<input type=checkbox name=SoftFskGen size=20 %s %s>Enable",
    			pCfg->cid_fsk_gen_mode ? "checked" : "",
    			!RTK_VOIP_IS_SLIC_CH( port, g_VoIP_Feature ) ? "disabled" : "");
    	}
#endif
    	else if (strcmp(argv[0], "spk_voice_gain")==0)
    		boaWrite(wp, "%d", pCfg->spk_voice_gain);
    	else if (strcmp(argv[0], "mic_voice_gain")==0)
            boaWrite(wp, "%d", pCfg->mic_voice_gain);
    	else
    	{
    		return -1;
    	}
    }//for (port=0; port<2; port++)

	return 0;
}

void asp_voip_AdvancedSet(request * wp, char *path, char *query)
{
	voipCfgParam_t *pVoIPCfg;
	voipCfgPortParam_t *pCfg[2];
	int i, port, voip_port;
	char *ptr;
	char szFrameSize[12], szPrecedence[12];
	char redirect_url[50];
	char szName[20];

	if (web_flash_get(&pVoIPCfg) != 0)
		return;
#if 0
	voip_port = atoi(boaGetVar(wp, "voipPort", "0"));
	if (voip_port < 0 || voip_port >= g_VoIP_Ports)
		return;
#endif
	rtk_GetVoIPFeature();

	for (port=0; port<g_VoIP_Ports; port++)
	{
		pCfg[port] = &pVoIPCfg->ports[port];   
		pCfg[port]->default_proxy = atoi(boaGetVar(wp, "default_proxy", "0"));

        // V.152
    	pCfg[port]->useV152 = !gstrcmp(boaGetVar(wp, "useV152", ""), "on");
    	pCfg[port]->v152_payload_type = atoi(boaGetVar(wp, "V152_payload_type", "96"));
    	pCfg[port]->v152_codec_type = atoi(boaGetVar(wp, "V152_codec_type", "0"));

        // T.38
        /* ++T.38 added by Jack Chan 24/01/07 for VoIP++ */
#ifdef CONFIG_RTK_VOIP_T38
    	pCfg[port]->useT38 = !gstrcmp(boaGetVar(wp, "useT38", ""), "on");
    	//pCfg[port]->T38_port = atoi(boaGetVar(wp, "T38_PORT", "9000"));

    	//T.38 parameters
    	pCfg[port]->T38ParamEnable = !gstrcmp(boaGetVar(wp, "T38ParamEnable", ""), "on");
    	pCfg[port]->T38MaxBuffer = atoi(boaGetVar(wp, "T38MaxBuffer", "500"));
    	pCfg[port]->T38RateMgt = atoi(boaGetVar(wp, "T38RateMgt", "2"));
    	pCfg[port]->T38MaxRate = atoi(boaGetVar(wp, "T38MaxRate", "5"));
    	pCfg[port]->T38EnableECM = !gstrcmp(boaGetVar(wp, "T38EnableECM", ""), "on");
    	pCfg[port]->T38ECCSignal = atoi(boaGetVar(wp, "T38ECCSignal", "5"));
    	pCfg[port]->T38ECCData = atoi(boaGetVar(wp, "T38ECCData", "2"));
    	pCfg[port]->T38EnableSpoof = !gstrcmp(boaGetVar(wp, "T38EnableSpoof", ""), "on");
    	pCfg[port]->T38DuplicateNum = atoi(boaGetVar(wp, "T38DuplicateNum", "0"));
#else /*CONFIG_RTK_VOIP_T38*/
    	pCfg[port]->useT38 = 0;
    	//pCfg[port]->T38_port = 0;
    	//T.38 parameters
    	pCfg[port]->T38ParamEnable = 0;
    	pCfg[port]->T38MaxBuffer = 0;
    	pCfg[port]->T38RateMgt = 0;
    	pCfg[port]->T38MaxRate = 0;
    	pCfg[port]->T38EnableECM = 0;
    	pCfg[port]->T38ECCSignal = 0;
    	pCfg[port]->T38ECCData = 0;
    	pCfg[port]->T38EnableSpoof = 0;
    	pCfg[port]->T38DuplicateNum = 0;
#endif /*#ifdef CONFIG_RTK_VOIP_T38*/

        // Fax Modem 
        ptr = boaGetVar(wp, "fax_modem_det_mode", "");
    	for(i=0; i<FAX_MODEM_DET_MAX; i++)
    	{
    		if (!gstrcmp(ptr, fax_modem_det_string[i]))
    			break;
    	}
    	if (i == FAX_MODEM_DET_MAX)
    		i = FAX_MODEM_DET_MODEM;
    	pCfg[port]->fax_modem_det = i;

		// V.152
		pCfg[port]->useV152 = !gstrcmp(boaGetVar(wp, "useV152", ""), "on");
		pCfg[port]->v152_payload_type = atoi(boaGetVar(wp, "V152_payload_type", "96"));
		pCfg[port]->v152_codec_type = atoi(boaGetVar(wp, "V152_codec_type", "0"));

#if 0
		// VoIP Security 
#ifdef CONFIG_RTK_VOIP_SRTP
    	pCfg[port]->security_enable = !gstrcmp(boaGetVar(wp, "useSRTP", ""), "on");
#else
    	pCfg[port]->security_enable = 0;
#endif /*CONFIG_RTK_VOIP_SRTP*/
        // Slic
        pCfg[port]->slic_txVolumne = atoi(boaGetVar(wp, "slic_txVolumne", "1")) - 1;
        pCfg[port]->slic_rxVolumne = atoi(boaGetVar(wp, "slic_rxVolumne", "1")) - 1;
#endif
        // DSP
        // Jitter Buffer Control
        pCfg[port]->jitter_delay = atoi(boaGetVar(wp, "jitterDelay", "4"));
        pCfg[port]->maxDelay  = atoi(boaGetVar(wp, "maxDelay", "13"));
        pCfg[port]->jitter_factor = atoi(boaGetVar(wp, "jitterFactor", "7"));

        // LEC Echo Tail
        pCfg[port]->echoTail  = atoi(boaGetVar(wp, "echoTail", "2"));
        // LEC
        pCfg[port]->lec = !gstrcmp(boaGetVar(wp, "useLec", ""), "on");
        // NLP
    	pCfg[port]->nlp = !gstrcmp(boaGetVar(wp, "useNlp", ""), "on");
        // VAD
    	pCfg[port]->vad = !gstrcmp(boaGetVar(wp, "useVad", ""), "on");
    	pCfg[port]->vad_thr = atoi(boaGetVar(wp, "Vad_threshold", "1"));
        // SID Noise Level 
        pCfg[port]->sid_gainmode = atoi(boaGetVar(wp, "sid_mode", "0"));
    	if (pCfg[port]->sid_gainmode == 1)
    		pCfg[port]->sid_noiselevel = atoi(boaGetVar(wp, "sid_noiselevel", "70"));
    	else if (pCfg[port]->sid_gainmode == 2)
    		pCfg[port]->sid_noisegain = atoi(boaGetVar(wp, "sid_noisegain", "0"));
        // CNG
        pCfg[port]->cng = !gstrcmp(boaGetVar(wp, "useCng", ""), "on");
    	pCfg[port]->cng_thr = atoi(boaGetVar(wp, "Cng_threshold", "1"));
        // PLC
        pCfg[port]->PLC = !gstrcmp(boaGetVar(wp, "usePLC", ""), "on");       
        // RTCP
        if( !gstrcmp(boaGetVar(wp, "useRTCP", ""), "on") ) 
        {
            pCfg[port]->RTCP_Interval = atoi( boaGetVar(wp, "RTCPInterval", "0") );
            pCfg[port]->RTCP_XR = !gstrcmp(boaGetVar(wp, "useRTCPXR", ""), "on");
        } 
        else 
        {
            pCfg[port]->RTCP_Interval = 0;
            pCfg[port]->RTCP_XR = 0;
        }
        // Fax/Modem
        i=0;
        i|= (!gstrcmp(boaGetVar(wp, "useFaxModem2833Relay", ""), "on"))?0x1:0;
        i|= (!gstrcmp(boaGetVar(wp, "useFaxModemInbandRemoval", ""), "on"))?0x2:0;
        i|= (!gstrcmp(boaGetVar(wp, "useFaxModem2833RxTonePlay", ""), "on"))?0x4:0;
        pCfg[port]->faxmodem_rfc2833=i;
        // Speaker AGC
        pCfg[port]->speaker_agc = !gstrcmp(boaGetVar(wp, "CFuseSpeaker", ""), "on");
        pCfg[port]->spk_agc_lvl = atoi(boaGetVar(wp, "CF_spk_AGC_level", "1")) - 1;
        pCfg[port]->spk_agc_gu = atoi(boaGetVar(wp, "CF_spk_AGC_up_limit", "6")) - 1;
        pCfg[port]->spk_agc_gd = (-(atoi(boaGetVar(wp, "CF_spk_AGC_down_limit", "-6")))) - 1 ;
        // MIC AGC
        pCfg[port]->mic_agc = !gstrcmp(boaGetVar(wp, "CFuseMIC", ""), "on");
        pCfg[port]->mic_agc_lvl = atoi(boaGetVar(wp, "CF_mic_AGC_level", "1")) - 1;
        pCfg[port]->mic_agc_gu = atoi(boaGetVar(wp, "CF_mic_AGC_up_limit", "6")) - 1;
        pCfg[port]->mic_agc_gd = (-(atoi(boaGetVar(wp, "CF_mic_AGC_down_limit", "-6")))) - 1 ;
        // Caller ID 
        ptr	 = boaGetVar(wp, "caller_id", "");
        for(i=0; i<CID_MAX; i++)
        {
            if (!gstrcmp(ptr, cid[i]))
                break;
        }
        if (i == CID_MAX)
            i = CID_DTMF;

        if(!gstrcmp(boaGetVar(wp, "FSKdatesync", ""), "on"))
            i=i | 0x80U;

        if(!gstrcmp(boaGetVar(wp, "revPolarity", ""), "on"))
            i=i | 0x40U;

        if(!gstrcmp(boaGetVar(wp, "sRing", ""), "on"))
            i=i | 0x20U;

        if(!gstrcmp(boaGetVar(wp, "dualTone", ""), "on"))
            i=i | 0x10U;

        if(!gstrcmp(boaGetVar(wp, "PriorRing", ""), "on"))
            i=i | 0x08U;

		pCfg[port]->caller_id_mode = i;

        ptr	= boaGetVar(wp, "cid_dtmfMode_S", "");
        for(i=0; i<CID_DTMF_MAX; i++)
        {
            if (!gstrcmp(ptr, cid_dtmf[i]))
                break;
        }
        if (i == CID_DTMF_MAX)
            i = CID_DTMF_D;
        pCfg[port]->cid_dtmf_mode = i;

        ptr	 = boaGetVar(wp, "cid_dtmfMode_E", "");
        for(i=0; i<CID_DTMF_MAX; i++)
        {
            if (!gstrcmp(ptr, cid_dtmf[i]))
                break;
        }
        if (i == CID_DTMF_MAX)
            i = CID_DTMF_D;
        pCfg[port]->cid_dtmf_mode |= (i<<2);

        //Flash Time Setting 
#ifndef CONFIG_RTK_VOIP_IP_PHONE
        i = atoi(boaGetVar(wp, "flash_hook_time", ""));
        if ((i >= 100) && ( i <= 2000))
            pCfg[port]->flash_hook_time = i;
        else
            pCfg[port]->flash_hook_time = 300;

        i = atoi(boaGetVar(wp, "flash_hook_time_min", ""));
        if ( i >= pCfg[port]->flash_hook_time )
            pCfg[port]->flash_hook_time_min = 0;
        else
            pCfg[port]->flash_hook_time_min = i;
#endif
        // Speaker Voice Gain 
        i = atoi(boaGetVar(wp, "spk_voice_gain", ""));
        if ((i>= -32) && (i<=31) )
            pCfg[port]->spk_voice_gain=i;
        else
            pCfg[port]->spk_voice_gain=0;
        // Speaker Voice Gain
        i = atoi(boaGetVar(wp, "mic_voice_gain", ""));
        if ((i>= -32) && (i<=31) )
            pCfg[port]->mic_voice_gain=i;
        else
            pCfg[port]->mic_voice_gain=0;

    }//for (port=0; port<2; port++)

	web_flash_set(pVoIPCfg);

#ifdef CONFIG_USER_DNSMASQ_DNSMASQ245
    cmd_set_dns_config(NULL);
    restart_dnsrelay();
#endif

#ifdef REBOOT_CHECK
#ifndef CONFIG_GENERAL_WEB
    OK_MSG("/voip_advanced.asp");
#else
	OK_MSG("/voip_advanced_new_web.asp");
#endif
#else
    web_restart_solar();
#ifndef CONFIG_GENERAL_WEB
    boaRedirect(wp, "/voip_advanced.asp");
#else
	boaRedirect(wp, "/voip_advanced_new_web.asp");
#endif
#endif
}


