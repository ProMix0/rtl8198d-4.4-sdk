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

#ifndef __RT_EPON_H__
#define __RT_EPON_H__

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>
#include <rtk/switch.h>

#define RT_EPON_KEY_SIZE 3
#define RT_EPON_MAX_QUEUE_PER_LLID 10

typedef struct rt_epon_llid_entry_s
{
    uint8          llidIdx;
    uint16         llid;
    rt_enable_t   valid;
    uint8          reportTimer;
    rt_enable_t   isReportTimeout; /*read only*/
    rt_mac_t      mac;
}rt_epon_llid_entry_t;

typedef struct rt_epon_regReq_s{
    uint8           llidIdx;
    rt_mac_t       mac;
    uint8           pendGrantNum;
    rt_enable_t    doRequest;
}rt_epon_regReq_t;

typedef struct rt_epon_churningKeyEntry_s
{
    uint8   llidIdx;
    uint8   keyIdx;
    uint8   churningKey[RT_EPON_KEY_SIZE];
    uint8   churningKey1[RT_EPON_KEY_SIZE];
    uint8   churningKey2[RT_EPON_KEY_SIZE];
}rt_epon_churningKeyEntry_t;

typedef struct rt_epon_llidCounter_s
{
    uint32  queueTxFrames[RT_EPON_MAX_QUEUE_PER_LLID];
    uint32  mpcpTxReport;
    uint32  mpcpRxGate;
    uint32  onuLlidNotBcst;
}rt_epon_llidCounter_t;

typedef struct rt_epon_counter_s
{
    rt_epon_llidCounter_t  llidIdxCnt;
    uint8   llidIdx; /*indicate LLID relative counter is get from which LLID index*/
    uint32  mpcpRxDiscGate;
    uint32  fecCorrectedBlocks;    
    uint32  fecUncorrectedBlocks;
    uint32  fecCodingVio;
    uint32  notBcstBitLlid7fff;
    uint32  notBcstBitNotOnuLlid;
    uint32  bcstBitPlusOnuLLid;
    uint32  bcstNotOnuLLid;
    uint32  crc8Err;
    uint32  mpcpTxRegRequest;
    uint32  mpcpTxRegAck;
}rt_epon_counter_t;

typedef enum rt_epon_llid_scheduler_mode_e{
    RT_EPON_LLID_QUEUE_SCHEDULER_SP = 0, /* Strict Priority */
    RT_EPON_LLID_QUEUE_SCHEDULER_WRR = 1, /* Weighted Round Robin */
} rt_epon_llid_scheduler_mode_t;

typedef struct rt_epon_queueCfg_s{
    rt_epon_llid_scheduler_mode_t      scheduleType;
    uint32                              cir;
    uint32                              pir;
    uint32                              weight;
}rt_epon_queueCfg_t;

typedef uint32 (*rt_epon_oam_rx_callback)(uint32 msgLen,uint8 *pMsg);

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
extern int32 rt_epon_init(void);

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
extern int32 rt_epon_llid_entry_get(rt_epon_llid_entry_t *pLlidEntry);  

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
extern int32 rt_epon_llid_entry_set(rt_epon_llid_entry_t *pLlidEntry);

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
extern int32 rt_epon_registerReq_get(rt_epon_regReq_t *pRegEntry);

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
extern int32 rt_epon_registerReq_set(rt_epon_regReq_t *pRegEntry);

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
extern int32 rt_epon_churningKey_get(rt_epon_churningKeyEntry_t *pEntry);

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
extern int32 rt_epon_churningKey_set(rt_epon_churningKeyEntry_t *pEntry);

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
extern int32 rt_epon_usFecState_get(rt_enable_t *pState);


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
extern int32 rt_epon_usFecState_set(rt_enable_t state);


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
extern int32 rt_epon_dsFecState_get(rt_enable_t *pState);


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
extern int32 rt_epon_dsFecState_set(rt_enable_t state);

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
extern int32 rt_epon_mibCounter_get(rt_epon_counter_t *pCounter);

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
extern int32 rt_epon_losState_get(rt_enable_t *pState);

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
extern int32 rt_epon_ponQueue_set(uint32 llid, uint32 queueId, rt_epon_queueCfg_t *pQueuecfg);

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
extern int32 rt_epon_ponQueue_get(uint32 llid, uint32 queueId, rt_epon_queueCfg_t *pQueuecfg);

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
extern int32 rt_epon_ponQueue_del(uint32 llid, uint32 queueId);

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
extern int32 rt_epon_egrBandwidthCtrlRate_get(uint32 *pRate);

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
extern int32 rt_epon_egrBandwidthCtrlRate_set(uint32 rate);

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
extern int32 rt_epon_oam_tx(uint32 msgLen,uint8 *pMsg);

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
extern int32 rt_epon_oam_rx_callback_register(rt_epon_oam_rx_callback oamRx);

#endif