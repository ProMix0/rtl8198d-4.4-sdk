/*
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 * *
 * $Revision: $
 * $Date: $
 *
 * Purpose : EPON MAC register access APIs
 *
 * Feature : Provide the APIs to access EPON MAC
 *
 */


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <rtk/rt/rt_epon.h>
#include <rtk/rt/rt_rate.h>

#define RT_SCHTYPE_TO_RTK_SCHTYPE(__rt_schType) (__rt_schType ==RT_EPON_LLID_QUEUE_SCHEDULER_SP?STRICT_PRIORITY:WFQ_WRR_PRIORITY)

/* Function Name:
 *      rt_epon_init
 * Description:
 *      epon register level initial function
 * Input:
 *
 * Output:
 *      
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_init(void)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_init();
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_init */

/* Function Name:
 *      rt_epon_llid_entry_get
 * Description:
 *      Get llid entry
 * Input:
 *      llidIdx: LLID table index
 * Output:
 *      pLlidEntry: LLID entry 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_llid_entry_get(rt_epon_llid_entry_t *pLlidEntry)  
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_llid_entry_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_llid_entry_get((rtk_epon_llid_entry_t *)pLlidEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_llid_entry_get */

/* Function Name:
 *      rt_epon_llid_entry_set
 * Description:
 *      Set llid entry
 * Input:
 *      llidIdx: LLID table index
 * Output:
 *      pLlidEntry: LLID entry 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_llid_entry_set(rt_epon_llid_entry_t *pLlidEntry)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_llid_entry_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_llid_entry_set((rtk_epon_llid_entry_t *)pLlidEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_llid_entry_set */

/* Function Name:
 *      rt_epon_registerReq_get
 * Description:
 *      Get register request relative parameter
 * Input:
 *      pRegEntry : register request relative parament
 * Output:
 *       None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_registerReq_get(rt_epon_regReq_t *pRegEntry)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_registerReq_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_registerReq_get((rtk_epon_regReq_t *)pRegEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_registerReq_get */

/* Function Name:
 *      rt_epon_registerReq_set
 * Description:
 *      Set register request relative parameter
 * Input:
 *       None
 * Output:
 *       pRegEntry : register request relative parament 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_registerReq_set(rt_epon_regReq_t *pRegEntry)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_registerReq_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_registerReq_set((rtk_epon_regReq_t *)pRegEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_registerReq_set */

/* Function Name:
 *      rt_epon_churningKey_get
 * Description:
 *      Get churning key entry
 * Input:
 *       None
 * Output:
 *       pEntry : churning key relative parameter 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_churningKey_get(rt_epon_churningKeyEntry_t *pEntry)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_churningKey_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_churningKey_get((rtk_epon_churningKeyEntry_t *)pEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_churningKey_get */

/* Function Name:
 *      rt_epon_churningKey_set
 * Description:
 *      Set churning key entry
 * Input:
 *       pEntry : churning key relative parameter 
 * Output:
 *       None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_churningKey_set(rt_epon_churningKeyEntry_t *pEntry)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_churningKey_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_churningKey_set((rtk_epon_churningKeyEntry_t *)pEntry);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_churningKey_set */

/* Function Name:
 *      rt_epon_usFecState_get
 * Description:
 *      Get upstream fec state
 * Input:
 *       None
 * Output:
 *       *pState : upstream FEC state 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_usFecState_get(rt_enable_t *pState)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_usFecState_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_usFecState_get((rtk_enable_t *)pState);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_usFecState_get */


/* Function Name:
 *      rt_epon_usFecState_set
 * Description:
 *      Set upstream fec state
 * Input:
 *       state : upstream FEC state
 * Output:
 *       None 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_usFecState_set(rt_enable_t state)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_usFecState_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_usFecState_set(state);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_usFecState_set */


/* Function Name:
 *      rt_epon_dsFecState_get
 * Description:
 *      Get down-stream fec state
 * Input:
 *       None
 * Output:
 *       *pState : down-stream FEC state 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_dsFecState_get(rt_enable_t *pState)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_dsFecState_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_dsFecState_get((rtk_enable_t *)pState);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_dsFecState_get */


/* Function Name:
 *      rt_epon_dsFecState_set
 * Description:
 *      Set down-stream fec state
 * Input:
 *       state : down-stream FEC state
 * Output:
 *       None 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_dsFecState_set(rt_enable_t state)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_dsFecState_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_dsFecState_set(state);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_dsFecState_set */

/* Function Name:
 *      rt_epon_dsFecState_set
 * Description:
 *      Set down-stream fec state
 * Input:
 *       None
 * Output:
 *       pCounter : EPON mib counter 
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 rt_epon_mibCounter_get(rt_epon_counter_t *pCounter)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_mibCounter_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_mibCounter_get((rtk_epon_counter_t *)pCounter);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_mibCounter_get */

/* Function Name:
 *      rt_epon_losState_get
 * Description:
 *      Get laser lose of signal state.
 * Input:
 *      pState LOS state
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
rt_epon_losState_get(rt_enable_t *pState)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->epon_losState_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_losState_get((rtk_enable_t *)pState);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_losState_get */

/* Function Name:
 *      rt_epon_ponQueue_set
 * Description:
 *      Set the pon queue
 * Input:
 *      llid    - logical llid index
 *      queueId  - logical llid queue id
 *      pQueuecfg  - pointer to the queue configuration
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - Invalid input parameters.
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 */
int32 
rt_epon_ponQueue_set(uint32 llid, uint32 queueId, rt_epon_queueCfg_t *pQueuecfg)
{
    int32   ret;
    rtk_ponmac_queue_t queue;
    rtk_ponmac_queueCfg_t queueCfg;

    /* function body */
    if (NULL == RT_MAPPER->ponmac_queue_add)
        return RT_ERR_DRIVER_NOT_FOUND;

    queue.schedulerId = llid;
    queue.queueId = queueId;

    queueCfg.type = RT_SCHTYPE_TO_RTK_SCHTYPE(pQueuecfg->scheduleType);

    if(pQueuecfg->cir == RT_MAX_RATE)
        queueCfg.cir = RT_RATE_PON_EGR_PIR_CIR_RATE_MAX;
    else
        queueCfg.cir = pQueuecfg->cir;

    if(pQueuecfg->pir == RT_MAX_RATE)
        queueCfg.pir = RT_RATE_PON_EGR_PIR_CIR_RATE_MAX;
    else
        queueCfg.pir = pQueuecfg->pir;

    queueCfg.weight = pQueuecfg->weight;
    queueCfg.egrssDrop  = DISABLED;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_queue_add(&queue,&queueCfg);
    RTK_API_UNLOCK();

    return ret;
}   /* end of rt_epon_ponQueue_set */

/* Function Name:
 *      rt_epon_ponQueue_get
 * Description:
 *      Get the pon queue
 * Input:
 *      llid    - logical llid index
 *      queueId  - logical llid queue id
 * Output:
 *      pQueuecfg - pointer to the queue configuration
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - Invalid input parameters.
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 */
int32 
rt_epon_ponQueue_get(uint32 llid, uint32 queueId, rt_epon_queueCfg_t *pQueuecfg)
{
    int32   ret;
    rtk_ponmac_queue_t queue;
    rtk_ponmac_queueCfg_t queueCfg;

    /* function body */
    if (NULL == RT_MAPPER->ponmac_queue_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    queue.schedulerId = llid;
    queue.queueId = queueId;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_queue_get(&queue,&queueCfg);
    RTK_API_UNLOCK();

    pQueuecfg->scheduleType = queueCfg.type;
    pQueuecfg->cir = queueCfg.cir;
    pQueuecfg->pir = queueCfg.pir;
    pQueuecfg->weight = queueCfg.weight;

    return ret;
}   /* end of rt_epon_ponQueue_get */

/* Function Name:
 *      rt_epon_ponQueue_del
 * Description:
 *      Delete the pon queue
 * Input:
 *      llid    - logical llid index
 *      queueId  - logical llid queue id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - Invalid input parameters.
 * Note:
 */
int32 
rt_epon_ponQueue_del(uint32 llid, uint32 queueId)
{
    int32   ret;
    rtk_ponmac_queue_t queue;

    /* function body */
    if (NULL == RT_MAPPER->ponmac_queue_del)
        return RT_ERR_DRIVER_NOT_FOUND;

    queue.schedulerId = llid;
    queue.queueId = queueId;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_queue_del(&queue);
    RTK_API_UNLOCK();

    return ret;
}   /* end of rt_epon_ponQueue_del */

/* Function Name:
 *      rt_epon_egrBandwidthCtrlRate_get
 * Description:
 *      Get the egress bandwidth control rate.
 * Input:
 *      None
 * Output:
 *      pRate - egress bandwidth control rate
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
int32
rt_epon_egrBandwidthCtrlRate_get(uint32 *pRate)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->ponmac_egrBandwidthCtrlRate_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_egrBandwidthCtrlRate_get(pRate);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_egrBandwidthCtrlRate_get */

/* Function Name:
 *      rt_epon_egrBandwidthCtrlRate_set
 * Description:
 *      Set the pon port egress bandwidth control rate.
 * Input:
 *      rate - egress bandwidth control rate
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
int32
rt_epon_egrBandwidthCtrlRate_set(uint32 rate)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->ponmac_egrBandwidthCtrlRate_set)
        return RT_ERR_DRIVER_NOT_FOUND;

    if(rate == RT_MAX_RATE)
        rate = RT_RATE_PON_EGR_RATE_MAX;

    RTK_API_LOCK();
    ret = RT_MAPPER->ponmac_egrBandwidthCtrlRate_set(rate);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_epon_egrBandwidthCtrlRate_set */


/* Function Name:
 *      rt_epon_oam_tx
 * Description:
 *      Send OAM Packet.
 * Input:
 *      msgLen - length of OAM Packet
 *      pMsg - content of OAM Packet
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
rt_epon_oam_tx(uint32 msgLen,uint8 *pMsg)
{
    int32   ret = RT_ERR_OK;
    if(msgLen == 0||pMsg == NULL)
        return RT_ERR_FAILED;
#if 0
    /* function body */
    if (NULL == RT_MAPPER->epon_oam_tx)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_oam_tx(msgLen, pMsg);
    RTK_API_UNLOCK();
#endif
    return ret;
}   /* end of rt_epon_oam_tx */

/* Function Name:
 *      rt_epon_oam_tx
 * Description:
 *      Send OAM Packet.
 * Input:
 *      oamRx - call back function of OAM Packet
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32 
rt_epon_oam_rx_callback_register(rt_epon_oam_rx_callback oamRx)
{
    int32   ret = RT_ERR_OK;

    if(oamRx == NULL)
        return RT_ERR_FAILED;
#if 0
    /* function body */
    if (NULL == RT_MAPPER->epon_oam_rx_callback_register)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->epon_oam_rx_callback_register(oamRx);
    RTK_API_UNLOCK();
#endif
    return ret;
}   /* end of rt_epon_oam_rx_callback_register */

