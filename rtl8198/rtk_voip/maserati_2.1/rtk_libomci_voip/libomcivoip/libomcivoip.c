#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<rtk/mib.h>
#include<rtk/utility.h>

#include<feature_export_api.h>
#include<omci_voice_sd.h>
#include<voip_flash.h>
#include<rtk_voip/kernel/include/voip_control.h>


//#define DEBUG
#ifdef DEBUG
#define DBG_LOG(...) fprintf(stderr, "[libomci_voip] "__VA_ARGS__)
#else
#define DBG_LOG while(0){}
#endif

unsigned int voice_cfg_parser(va_list argp)
{
    voip_flash_share_t  *ptr = NULL;
    voipCfgParam_t      **pptr = NULL;
    voip_state_share_t  **ppSts = NULL;

    int ret;
	voice_cfg_msg_t *pParam = va_arg(argp, voice_cfg_msg_t *);
	va_end(argp);

    if (!pParam)
        return FAL_FAIL;

    DBG_LOG("%s(), op = %d\n", __FUNCTION__, pParam->op_id);

	switch (pParam->op_id)
	{
    	case VOICE_OP_RESET_ALL:
    		break;
    	case VOICE_OP_SET_SHARE:
		if (voip_flash_server_update()==-1)
			ret=-1;
		else
			ret=0;
    		break;
    	case VOICE_OP_SET_SHM:
            pptr = (voipCfgParam_t **)(pParam->cfg.ppCfg);
            ret = voip_shm_set(*pptr);
    		break;
    	case VOICE_OP_INIT_VARS:
            ret = voip_line_state_init_variables();
            ret = voip_flash_server_init_variables();
    		break;
        case VOICE_OP_GET_SHARE:
            //ptr = (voip_flash_share_t *)(pParam->cfg.pFlash);
            //ret = voip_share_get(ptr);
			ret=0;
            break;
        case VOICE_OP_GET_SHM:
            pptr = (voipCfgParam_t **)(pParam->cfg.ppCfg);
            ret = voip_shm_get(pptr);
            //ret = voip_shm_get((voipCfgParam_t **)(pParam->cfg.ppCfg));
            break;
        case VOICE_OP_GET_RTP_STAT:
            rtk_GetRtpRtcpStatistics(pParam->cfg.rtpStatInfo.ch_id,
                                     pParam->cfg.rtpStatInfo.session_id,
                                     pParam->cfg.rtpStatInfo.reset_flag,
                                     pParam->cfg.rtpStatInfo.pRtp_stat);
            break;
        case VOICE_OP_GET_LINE_STATE:
            ppSts = (voip_state_share_t **)(pParam->cfg.ppSts);
            ret = voip_line_state_share_get(ppSts);
            break;
    	default:
    		DBG_LOG("Unknown command %d!\n", pParam->op_id);
		break;
	}


    if (ret != 0)
        DBG_LOG("Error: option id =%u failed!\n", pParam->op_id);
    return (0 == ret ? FAL_OK : FAL_FAIL);
}


unsigned int feature_module_init(unsigned int moduleID)
{

	if (feature_api_register(FEATURE_API_VOICE_RTK_CFG_OP, moduleID, &voice_cfg_parser) != FAL_OK)
	{
		DBG_LOG("FEATURE_API_VOICE_RTK_CFG_OP  Error: Register OMCI feature API failed!\n");
	}
    return FAL_OK;
}


