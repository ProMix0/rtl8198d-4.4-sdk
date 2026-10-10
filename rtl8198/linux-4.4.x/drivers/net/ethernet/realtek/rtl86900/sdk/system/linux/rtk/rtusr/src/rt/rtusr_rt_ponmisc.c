/*
 * Copyright (C) 2019 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : Definition of PON MISC API
 *
 * Feature : The file includes the following modules and sub-modules
 *           
 */

/*
 * Include Files
 */
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <osal/lib.h>
#include <rtk/rtusr/include/rtusr_util.h>
#include <rtdrv/rtdrv_netfilter.h>
#include <rtk/rt/rt_ponmisc.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */

 /*
 * Function Declaration
 */
/* Function Name:
 *      rt_ponmisc_init
 * Description:
 *      Initialize pon misc interface.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_FEATURE_NOT_SUPPORTED
 * Note:
 *      Must initialize before calling any other APIs.
 */
int32
rt_ponmisc_init(void)
{
    rtdrv_rt_ponmiscCfg_t rt_ponmisc_cfg;

    /* function body */
    SETSOCKOPT(RTDRV_RT_PONMISC_INIT, &rt_ponmisc_cfg, rtdrv_rt_ponmiscCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_ponmisc_init */

/* Function Name:
 *      rt_ponmisc_modeSpeed_get
 * Description:
 *      get pon misc mode speed inforamtion.
 * Input:
 *      None
 * Output:
 *      pPonMode    - current running PON mode
 *      pPonSpeed   - current running PON speed
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      
 */
int32
rt_ponmisc_modeSpeed_get(rt_ponmisc_ponMode_t *pPonMode,rt_ponmisc_ponSpeed_t *pPonSpeed)
{
    rtdrv_rt_ponmiscCfg_t rt_ponmisc_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pPonMode), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pPonSpeed), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_ponmisc_cfg.ponSpeed, pPonSpeed, sizeof(rt_ponmisc_ponSpeed_t));
    GETSOCKOPT(RTDRV_RT_PONMISC_MODESPEED_GET, &rt_ponmisc_cfg, rtdrv_rt_ponmiscCfg_t, 1);
    osal_memcpy(pPonMode, &rt_ponmisc_cfg.ponMode, sizeof(rt_ponmisc_ponMode_t));
    osal_memcpy(pPonSpeed, &rt_ponmisc_cfg.ponSpeed, sizeof(rt_ponmisc_ponSpeed_t));

    return RT_ERR_OK;
}   /* end of rt_ponmisc_modeSpeed_get */

/* Function Name:
 *      rt_ponmisc_sid_get
 * Description:
 *      get pon misc sid inforamtion.
 * Input:
 *      sid         - stream id
 * Output:
 *      pSidInfo    - corresponding sid infotmation
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT          - Invalid input parameters.
 *      RT_ERR_NULL_POINTER   - input parameter may be null pointer
 *      RT_ERR_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      
 */
int32
rt_ponmisc_sid_get(uint16 sid,rt_ponmisc_sidInfo_t *pSidInfo)
{
    rtdrv_rt_ponmiscCfg_t rt_ponmisc_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pSidInfo), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_ponmisc_cfg.sid, &sid, sizeof(uint16));
    GETSOCKOPT(RTDRV_RT_PONMISC_SID_GET, &rt_ponmisc_cfg, rtdrv_rt_ponmiscCfg_t, 1);
    osal_memcpy(pSidInfo, &rt_ponmisc_cfg.sidInfo, sizeof(rt_ponmisc_sidInfo_t));

    return RT_ERR_OK;
}   /* end of rt_ponmisc_sid_get */

/* Function Name:
 *      rt_ponmisc_sid_set
 * Description:
 *      set pon misc sid inforamtion.
 * Input:
 *      sid         - stream id
 *      sidInfo     - corresponding sid infotmation
 * Output:
 *      none
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT        - Invalid input parameters.
 * Note:
 *      
 */
int32
rt_ponmisc_sid_set(uint16 sid,rt_ponmisc_sidInfo_t sidInfo)
{
    rtdrv_rt_ponmiscCfg_t rt_ponmisc_cfg;

    /* function body */
    osal_memcpy(&rt_ponmisc_cfg.sid, &sid, sizeof(uint16));
    osal_memcpy(&rt_ponmisc_cfg.sidInfo, &sidInfo, sizeof(rt_ponmisc_sidInfo_t));
    SETSOCKOPT(RTDRV_RT_PONMISC_SID_SET, &rt_ponmisc_cfg, rtdrv_rt_ponmiscCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_ponmisc_sid_set *//* Function Name:
*      rt_ponmisc_transceiver_get
* Description:
*      Get transceiver value by the specific type
* Input:
*      type            - the transceiver parameter type
* Output:
*      pData           - the pointer of data for the specific transceiver parameter
* Return:
*      RT_ERR_OK
*      RT_ERR_FAILED
*      RT_ERR_INPUT        - Invalid input parameters.
*      RT_ERR_NULL_POINTER - input parameter may be null pointer
* Note:
*
*/
int32
rt_ponmisc_transceiver_get(rt_transceiver_parameter_type_t type, rt_transceiver_data_t *pData)
{
    rtdrv_rt_ponmiscCfg_t rt_ponmisc_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pData), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memset(&rt_ponmisc_cfg, 0, sizeof(rtdrv_rt_ponmiscCfg_t));
    osal_memcpy(&rt_ponmisc_cfg.trxDataType, &type, sizeof(rt_transceiver_parameter_type_t));
    GETSOCKOPT(RTDRV_RT_PONMISC_TRX_GET, &rt_ponmisc_cfg, rtdrv_rt_ponmiscCfg_t, 1);
    osal_memcpy(pData, &rt_ponmisc_cfg.trxData, sizeof(rt_transceiver_data_t));

    return RT_ERR_OK;
}   /* end of rt_ponmisc_transceiver_get */
