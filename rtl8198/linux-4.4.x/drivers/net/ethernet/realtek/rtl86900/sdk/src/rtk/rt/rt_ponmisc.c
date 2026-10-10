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
#include <common/rt_type.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <rtk/rt/rt_ponmisc.h>
#include <rtk/rt/rt_i2c.h>
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#include <rt/include/rt_ext_mapper.h>
#endif
/*
 * Symbol Definition
 */
rt_ponmisc_ponMode_t PON_MODE = RT_GPON_MODE;
rt_ponmisc_ponSpeed_t PON_SPEED = RT_1G25G_SPEED;

static uint32 rt_transceiver_addrs[][2]={
    {20,16},
    {68,16},
    {40,16},
    {96,2},
    {98,2},
    {100,2},
    {102,2},
    {104,2},
};

/*
 * Data Declaration
 */
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
static rt_ext_mapper_t* RT_EXT_MAPPER = NULL;
#endif

#ifdef CONFIG_LUNA_G3_SERIES
#define MAXSIDNUM 256
rt_ponmisc_sidInfo_t g_sidInfo[MAXSIDNUM];
#endif

/*
 * Macro Declaration
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
    int32 ret = RT_ERR_OK;

#ifdef CONFIG_LUNA_G3_SERIES
    if (NULL == RT_MAPPER->rt_ponmisc_init)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->rt_ponmisc_init();
    RTK_API_UNLOCK();

    memset(g_sidInfo,0,sizeof(rt_ponmisc_sidInfo_t)*MAXSIDNUM);
#endif

    return ret;
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
    int32 ret = RT_ERR_OK;
    
    RT_PARAM_CHK((NULL == pPonMode), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pPonSpeed), RT_ERR_NULL_POINTER);

#ifdef CONFIG_LUNA_G3_SERIES
    if (NULL == RT_MAPPER->rt_ponmisc_mode_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->rt_ponmisc_mode_get(pPonMode,pPonSpeed);
    RTK_API_UNLOCK();
#else
    rtk_ponmac_mode_t mode;

    if (NULL == RT_MAPPER->ponmac_mode_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_mode_get(&mode);
    RTK_API_UNLOCK();

    if(mode == PONMAC_MODE_GPON)
        PON_MODE = RT_GPON_MODE;
    else if(mode == PONMAC_MODE_EPON)
        PON_MODE = RT_EPON_MODE;
    else
        return RT_ERR_FEATURE_NOT_SUPPORTED;

    PON_SPEED = RT_1G25G_SPEED;
#endif
    return ret;
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
    int32 ret = RT_ERR_OK;

    RT_PARAM_CHK((NULL == pSidInfo), RT_ERR_NULL_POINTER);

    memset(pSidInfo,0,sizeof(rt_ponmisc_sidInfo_t));

#ifdef CONFIG_LUNA_G3_SERIES
    if(sid < MAXSIDNUM)
        memcpy(pSidInfo,&g_sidInfo[sid],sizeof(rt_ponmisc_sidInfo_t));
#else
    rt_gpon_usFlow_t flow_attr;

    ret = rt_gpon_usFlow_get(sid,&flow_attr);
    if(ret == RT_ERR_OK)
    {
        pSidInfo->enFlag = ENABLED;
        pSidInfo->dqFlag = ENABLED;
        pSidInfo->dstPort = 5;
        pSidInfo->pri = flow_attr.tcQueueId;
        pSidInfo->flowId = sid;
    }
    else
    {
        pSidInfo->enFlag = DISABLED;
        pSidInfo->dqFlag = DISABLED;
        pSidInfo->dstPort = 5;
        pSidInfo->pri = 0;
        pSidInfo->flowId = 0;
        ret = RT_ERR_OK;
    }      
#endif

    return ret;
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
    int32 ret = RT_ERR_OK;

#ifdef CONFIG_LUNA_G3_SERIES
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#ifdef CONFIG_SDK_CA8277B
    if(NULL == RT_EXT_MAPPER)
    {
        RT_EXT_MAPPER = rt_ext_mapper_get();
        if(RT_EXT_MAPPER == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;
    }

    if (NULL == RT_EXT_MAPPER->internal_pon_sid_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    
    RTK_API_LOCK();
    ret = RT_EXT_MAPPER->internal_pon_sid_set(sid,sidInfo);
    RTK_API_UNLOCK()
#endif
#endif
    if(sid < MAXSIDNUM)
        memcpy(&g_sidInfo[sid],&sidInfo,sizeof(rt_ponmisc_sidInfo_t));
#else
    //Not support
#endif

    return ret;
}   /* end of rt_ponmisc_sid_set */

/* Function Name:
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
    int32   ret;
    uint32  devID = 0, idx;
    uint32 transceiver_data[TRANSCEIVER_LEN];

    osal_memset(transceiver_data, 0, TRANSCEIVER_LEN);

    /* function body */
    if (NULL == RT_MAPPER->i2c_read)
        return RT_ERR_DRIVER_NOT_FOUND;

    if (RT_TRANSCEIVER_PARA_TYPE_MAX == type)
        return RT_ERR_INPUT;

    if  ((type == RT_TRANSCEIVER_PARA_TYPE_VENDOR_NAME)||(type == RT_TRANSCEIVER_PARA_TYPE_SN)|| (type == RT_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM))
    {
        devID = TRANSCEIVER_A0;
    }else if ((type > RT_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM)  && (type < RT_TRANSCEIVER_PARA_TYPE_MAX))
    {
        devID = TRANSCEIVER_A2;
    }

    for (idx = rt_transceiver_addrs[type][0]; idx < rt_transceiver_addrs[type][0]+rt_transceiver_addrs[type][1]; idx++)
    {
        ret = rt_i2c_read(TRANSCEIVER_PORT, devID, idx, &(transceiver_data[idx-rt_transceiver_addrs[type][0]]));
        pData->buf[idx-rt_transceiver_addrs[type][0]]=transceiver_data[idx-rt_transceiver_addrs[type][0]];
    }
    return ret;
}   /* end of rt_ponmisc_transceiver_get */
