/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 81643 $
 * $Date: 2017-08-24 13:28:53 +0800 (Thu, 24 Aug 2017) $
 *
 * Purpose : Definition of L2 API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Mac address flush
 *           (2) Address learning limit
 *           (3) Parameter for L2 lookup and learning engine
 *           (4) Unicast address
 *           (5) L2 multicast
 *           (6) IP multicast
 *           (7) Multicast forwarding table
 *           (8) CPU mac
 *           (9) Port move
 *           (10) Parameter for lookup miss
 *           (11) Parameter for MISC
 *
 */



/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/rt/rt_port.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <rtk/rt/rt_l2.h>
#include <rtk/rt/rt_port.h>
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#include <rt/include/rt_ext_mapper.h>
#endif

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */

extern rt_port_uniType_t g_port_uniType[]; /*only for UNI port 0~3?*/

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
static rt_ext_mapper_t* RT_EXT_MAPPER = NULL;
#endif

/*
 * Macro Declaration
 */
static int32 _rt_l2_age_calculate(uint32 ori,uint32 *age);

/*
 * Function Declaration
 */

static int32 _rt_l2_age_calculate(uint32 ori,uint32 *age)
{
    int32   ret = RT_ERR_OK;
    uint32 agingTimeUnit = 1;

#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    if (NULL == RT_MAPPER->l2_aging_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_aging_get(&agingTimeUnit);
    RTK_API_UNLOCK();

    agingTimeUnit = (agingTimeUnit/10)/7;

    *age = ori*agingTimeUnit;
#else
    *age = ori*agingTimeUnit;
#endif

    return ret;
}


/* Function Name:
 *      rt_l2_init
 * Description:
 *      Initialize l2 module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize l2 module before calling any l2 APIs.
 */
int32
rt_l2_init(void)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->l2_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->l2_init();
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_l2_init */

/* Function Name:
 *      rt_l2_portLimitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the port.
 * Input:
 *      port     - port id
 * Output:
 *      pMacCnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 */
int32
rt_l2_portLimitLearningCnt_get(rt_port_t port, int32 *pMacCnt)
{
    int32   ret = RT_ERR_OK;

    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

    if(HAL_IS_PON_PORT(port))
        return RT_ERR_PORT_ID;

    if(pMacCnt == NULL)
        return RT_ERR_NULL_POINTER;

    /* function body */
    if (NULL == RT_MAPPER->l2_portLimitLearningCnt_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    if(g_port_uniType[port] == RT_PORT_UNI_TYPE_PPTP)
    {
#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
        RTK_API_LOCK();
        ret = RT_MAPPER->l2_portLimitLearningCnt_get(port, pMacCnt);
        RTK_API_UNLOCK();
        if(*pMacCnt==2112)
        {
            *pMacCnt = -1;
        }
#else
        /*not support now*/
#endif
    }
    else
    {
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
        if(NULL == RT_EXT_MAPPER)
        {
            RT_EXT_MAPPER = rt_ext_mapper_get();
            if(RT_EXT_MAPPER == NULL)
                return RT_ERR_DRIVER_NOT_FOUND;
        }

        if(RT_EXT_MAPPER->internal_macAddr_learning_limit_get == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;

        RTK_API_LOCK();
        ret = RT_EXT_MAPPER->internal_macAddr_learning_limit_get(port, pMacCnt);
        RTK_API_UNLOCK();
#endif
    }
    return ret;
}   /* end of rt_l2_portLimitLearningCnt_get */


/* Function Name:
 *      rt_l2_portLimitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the port.
 * Input:
 *      port    - port id
 *      macCnt  - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 *      (2) Set the macCnt to 0 mean disable learning in the port.
 */
int32
rt_l2_portLimitLearningCnt_set(rt_port_t port, int32 macCnt)
{
    int32   ret = RT_ERR_OK;

    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

    if(HAL_IS_PON_PORT(port))
        return RT_ERR_PORT_ID;

    /* function body */
    if (NULL == RT_MAPPER->l2_portLimitLearningCnt_set)
        return RT_ERR_DRIVER_NOT_FOUND;

    if(g_port_uniType[port] == RT_PORT_UNI_TYPE_PPTP)
    {
#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
        if(macCnt == -1)
        {
            macCnt = 2112;
        }
        RTK_API_LOCK();
        ret = RT_MAPPER->l2_portLimitLearningCnt_set(port, macCnt);
        RTK_API_UNLOCK();
#else
        if(macCnt)
        {
            /*not support now*/
        }
#endif
    }
    else
    {
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
        if(NULL == RT_EXT_MAPPER)
        {
            RT_EXT_MAPPER = rt_ext_mapper_get();
            if(RT_EXT_MAPPER == NULL)
                return RT_ERR_DRIVER_NOT_FOUND;
        }

        if(RT_EXT_MAPPER->internal_macAddr_learning_limit_set == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;

        RTK_API_LOCK();
        ret = RT_EXT_MAPPER->internal_macAddr_learning_limit_set(port, macCnt);
        RTK_API_UNLOCK();
#endif
    }

    return ret;
}   /* end of rt_l2_portLimitLearningCnt_set */

/* Function Name:
 *      rt_l2_newMacOp_get
 * Description:
 *      Get learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pLrnMode   - pointer to learning mode
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 */
int32
rt_l2_newMacOp_get(rt_port_t port,rt_action_t *pFwdAction)
{
    int32   ret;
#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    rtk_l2_newMacLrnMode_t mode;
#endif

    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

    if(pFwdAction == NULL)
        return RT_ERR_NULL_POINTER;

#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    if (NULL == RT_MAPPER->l2_newMacOp_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->l2_newMacOp_get(port, &mode, pFwdAction);
    RTK_API_UNLOCK();
#else
        /*not support now*/
#endif
    return ret;
}

/* Function Name:
 *      rt_l2_newMacOp_set
 * Description:
 *      Set learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port      - port id
 *      lrnMode   - learning mode
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_INPUT      - invalid input parameter
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 */
int32
rt_l2_newMacOp_set(rt_port_t port,rt_action_t fwdAction)
{
    int32   ret;
    
    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    if(fwdAction != ACTION_FORWARD && fwdAction != ACTION_DROP)
        return RT_ERR_INPUT;

    if (NULL == RT_MAPPER->l2_newMacOp_set)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_newMacOp_set(port, HARDWARE_LEARNING, fwdAction);
    RTK_API_UNLOCK();
#else
    if(fwdAction)
    {
        /*not support now*/
    }
#endif
    return ret;
}

/* Function Name:
 *      rt_l2_addr_add
 * Description:
 *      Add L2 entry to ASIC.
 * Input:
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 entry before add it.
 */
int32
rt_l2_addr_add(rt_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtk_l2_ucastAddr_t l2Addr;
    
    if(pL2Addr == NULL)
        return RT_ERR_NULL_POINTER;

    if(!HAL_IS_GE_PORT(pL2Addr->port) && !HAL_IS_FE_PORT(pL2Addr->port))
        return RT_ERR_PORT_ID;

    memset(&l2Addr,0x00,sizeof(rtk_l2_ucastAddr_t));

    /* function body */
    if (NULL == RT_MAPPER->l2_addr_add)
        return RT_ERR_DRIVER_NOT_FOUND;

    l2Addr.fid = 0;
    l2Addr.efid = 0;
    l2Addr.flags = 0;
    l2Addr.port = pL2Addr->port;
    l2Addr.age = 1;
    memcpy(l2Addr.mac.octet,pL2Addr->mac.octet,ETHER_ADDR_LEN);

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_addr_add(&l2Addr);
    RTK_API_UNLOCK();

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
    if(NULL == RT_EXT_MAPPER)
    {
        RT_EXT_MAPPER = rt_ext_mapper_get();
        if(RT_EXT_MAPPER == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;
    }

    if(RT_EXT_MAPPER->internal_l2_addr_add == NULL)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_EXT_MAPPER->internal_l2_addr_add(pL2Addr);
    RTK_API_UNLOCK();
#endif
    
    return ret;
}

/* Function Name:
 *      rt_l2_addr_del
 * Description:
 *      Delete a L2 unicast address entry.
 * Input:
 *      pL2Addr  - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the mac has existed in the LUT, it will be deleted. Otherwise, it will return RT_ERR_L2_ENTRY_NOTFOUND.
 */
int32
rt_l2_addr_del(rt_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtk_l2_ucastAddr_t l2Addr;

    if(pL2Addr == NULL)
        return RT_ERR_NULL_POINTER;

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
    if(NULL == RT_EXT_MAPPER)
    {
        RT_EXT_MAPPER = rt_ext_mapper_get();
        if(RT_EXT_MAPPER == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;
    }

    if(RT_EXT_MAPPER->internal_l2_addr_del == NULL)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_EXT_MAPPER->internal_l2_addr_del(pL2Addr);
    RTK_API_UNLOCK();
#endif

    memset(&l2Addr,0x00,sizeof(rtk_l2_ucastAddr_t));    

    if (NULL == RT_MAPPER->l2_addr_del)
        return RT_ERR_DRIVER_NOT_FOUND;

    l2Addr.fid = 0;
    l2Addr.efid = 0;
    memcpy(l2Addr.mac.octet,pL2Addr->mac.octet,ETHER_ADDR_LEN);

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_addr_del(&l2Addr);
    RTK_API_UNLOCK();

    return ret;
}

/* Function Name:
 *      rt_l2_addr_get
 * Description:
 *      Get L2 entry based on MAC address
 * Input:
 *      None
 * Output:
 *      pL2Addr - pointer to L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the unicast mac address existed in LUT, it will return the port and fid where
 *      the mac is learned. Otherwise, it will return a RT_ERR_L2_ENTRY_NOTFOUND error.
 */
int32
rt_l2_addr_get(rt_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtk_l2_ucastAddr_t l2Addr;
    
    if(pL2Addr == NULL)
        return RT_ERR_NULL_POINTER;

    memset(&l2Addr,0x00,sizeof(rtk_l2_ucastAddr_t));    

    if (NULL == RT_MAPPER->l2_addr_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    l2Addr.fid = 0;
    l2Addr.efid = 0;
    memcpy(l2Addr.mac.octet,pL2Addr->mac.octet,ETHER_ADDR_LEN);

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_addr_get(&l2Addr);
    RTK_API_UNLOCK();
    if(ret != RT_ERR_OK)
        return ret;

    ret = _rt_l2_age_calculate(l2Addr.age,&pL2Addr->age);
    if(ret != RT_ERR_OK)
        return ret;

    pL2Addr->port = l2Addr.port;
    pL2Addr->staticFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_STATIC)?ENABLED:DISABLED;
    pL2Addr->filterFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_SA_BLOCK)?ENABLED:DISABLED;
    
    return ret;
}

/* Function Name:
 *      rt_l2_nextValidAddr_get
 * Description:
 *      Get next valid L2 unicast address entry from the specified device.
 * Input:
 *      pScanIdx       - currently scan index of l2 table to get next.
 * Output:
 *      pL2Addr        - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
int32
rt_l2_nextValidAddr_get(
    int32               *pScanIdx,
    rt_l2_ucastAddr_t   *pL2Addr)
{
    int32   ret;
    rtk_l2_ucastAddr_t l2Addr;

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
    int32 tmSwScanIdx = *pScanIdx;
    int32 tmHwScanIdx = *pScanIdx;
    rt_l2_ucastAddr_t swL2Addr;
    rt_enable_t swHitFlag = DISABLED;
    rt_enable_t hwHitFlag = DISABLED;
    rt_enable_t useSwFlag = DISABLED;

    if(NULL == RT_EXT_MAPPER)
    {
        RT_EXT_MAPPER = rt_ext_mapper_get();
        if(RT_EXT_MAPPER == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;
    }

    if(RT_EXT_MAPPER->internal_l2_addr_get == NULL)
        return RT_ERR_DRIVER_NOT_FOUND;

    if (NULL == RT_MAPPER->l2_nextValidAddr_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    //Getting software address
    RTK_API_LOCK();
    ret = RT_EXT_MAPPER->internal_l2_addr_get(&tmSwScanIdx, &swL2Addr);
    RTK_API_UNLOCK();
    if(ret == 0)
        swHitFlag = ENABLED;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_nextValidAddr_get(&tmHwScanIdx, &l2Addr);
    RTK_API_UNLOCK();
    if(ret == RT_ERR_OK)
        hwHitFlag = ENABLED;

    if(swHitFlag == ENABLED && hwHitFlag == ENABLED)
    {
        if(tmHwScanIdx < tmSwScanIdx)
            useSwFlag = DISABLED;
        else
            useSwFlag = ENABLED;
    }
    else if(swHitFlag == ENABLED)
    {
        useSwFlag = ENABLED;
    }
    else if(hwHitFlag == ENABLED)
    {
        useSwFlag = DISABLED;
    }
    else
    {
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    if(useSwFlag == ENABLED)
    {
        *pScanIdx = tmSwScanIdx;
        memcpy(pL2Addr->mac.octet,swL2Addr.mac.octet,ETHER_ADDR_LEN);
        pL2Addr->port = swL2Addr.port;
        pL2Addr->age = swL2Addr.age;
        pL2Addr->staticFlag = swL2Addr.staticFlag;
        pL2Addr->filterFlag = swL2Addr.filterFlag;
    }
    else
    {
        *pScanIdx = tmHwScanIdx;
        ret = _rt_l2_age_calculate(l2Addr.age,&pL2Addr->age);
        if(ret != RT_ERR_OK)
            return ret;

        memcpy(pL2Addr->mac.octet,l2Addr.mac.octet,ETHER_ADDR_LEN);
        pL2Addr->port = l2Addr.port;
        pL2Addr->staticFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_STATIC)?ENABLED:DISABLED;
        pL2Addr->filterFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_SA_BLOCK)?ENABLED:DISABLED;
    }
#else

    if (NULL == RT_MAPPER->l2_nextValidAddr_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_nextValidAddr_get(pScanIdx, &l2Addr);
    RTK_API_UNLOCK();
    if(ret != RT_ERR_OK)
        return ret;

    ret = _rt_l2_age_calculate(l2Addr.age,&pL2Addr->age);
    if(ret != RT_ERR_OK)
        return ret;

    memcpy(pL2Addr->mac.octet,l2Addr.mac.octet,ETHER_ADDR_LEN);
    pL2Addr->port = l2Addr.port;
    pL2Addr->staticFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_STATIC)?ENABLED:DISABLED;
    pL2Addr->filterFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_SA_BLOCK)?ENABLED:DISABLED;
#endif

    return ret;
} /* end of rt_l2_nextValidAddr_get */

/* Function Name:
 *      rt_l2_nextValidAddrOnPort_get
 * Description:
 *      Get next valid L2 unicast address entry from specify port.
 * Input:
 *      port          - speific port
 *      pScanIdx      - currently scan index of l2 table to get next.
 * Output:
 *      pL2Addr       - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
int32
rt_l2_nextValidAddrOnPort_get(
    rt_port_t           port,
    int32               *pScanIdx,
    rt_l2_ucastAddr_t   *pL2Addr)
{
    int32   ret;
    rtk_l2_ucastAddr_t l2Addr;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
    int32 tmSwScanIdx = *pScanIdx;
    int32 tmHwScanIdx = *pScanIdx;
    rt_l2_ucastAddr_t swL2Addr;
    rt_enable_t swHitFlag = DISABLED;
    rt_enable_t hwHitFlag = DISABLED;
    rt_enable_t useSwFlag = DISABLED;

    if(NULL == RT_EXT_MAPPER)
    {
        RT_EXT_MAPPER = rt_ext_mapper_get();
        if(RT_EXT_MAPPER == NULL)
            return RT_ERR_DRIVER_NOT_FOUND;
    }

    if(RT_EXT_MAPPER->internal_l2_addr_get == NULL)
        return RT_ERR_DRIVER_NOT_FOUND;

    if (NULL == RT_MAPPER->l2_nextValidAddr_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    //Getting software address
    while(1)
    {
        RTK_API_LOCK();
        ret = RT_EXT_MAPPER->internal_l2_addr_get(&tmSwScanIdx, &swL2Addr);
        RTK_API_UNLOCK();

        if(ret != 0)
            break;
        else
        {
 //printk("tmSwScanIdx=%d swL2Addr.port=%d\n",tmSwScanIdx,swL2Addr.port);
            if(swL2Addr.port != port)
            {
                tmSwScanIdx++;
                continue;
            }
            break;
        }
    }

    if(ret == 0)
        swHitFlag = ENABLED;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_nextValidAddrOnPort_get(port, &tmHwScanIdx, &l2Addr);
    RTK_API_UNLOCK();
    if(ret == RT_ERR_OK)
        hwHitFlag = ENABLED;

//    printk("hwHitFlag=%d tmHwScanIdx=%d swHitFlag=%d tmSwScanIdx=%d\n"
//        ,hwHitFlag,tmHwScanIdx,swHitFlag,tmSwScanIdx);

    if(swHitFlag == ENABLED && hwHitFlag == ENABLED)
    {
        if(tmHwScanIdx < tmSwScanIdx)
            useSwFlag = DISABLED;
        else
            useSwFlag = ENABLED;
    }
    else if(swHitFlag == ENABLED)
    {
        useSwFlag = ENABLED;
    }
    else if(hwHitFlag == ENABLED)
    {
        useSwFlag = DISABLED;
    }
    else
    {
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    if(useSwFlag == ENABLED)
    {
        *pScanIdx = tmSwScanIdx;
        memcpy(pL2Addr->mac.octet,swL2Addr.mac.octet,ETHER_ADDR_LEN);
        pL2Addr->port = swL2Addr.port;
        pL2Addr->age = swL2Addr.age;
        pL2Addr->staticFlag = swL2Addr.staticFlag;
        pL2Addr->filterFlag = swL2Addr.filterFlag;
    }
    else
    {
        *pScanIdx = tmHwScanIdx;
        ret = _rt_l2_age_calculate(l2Addr.age,&pL2Addr->age);
        if(ret != RT_ERR_OK)
            return ret;

        memcpy(pL2Addr->mac.octet,l2Addr.mac.octet,ETHER_ADDR_LEN);
        pL2Addr->port = l2Addr.port;
        pL2Addr->staticFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_STATIC)?ENABLED:DISABLED;
        pL2Addr->filterFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_SA_BLOCK)?ENABLED:DISABLED;
    }
#else
    /* function body */
    if (NULL == RT_MAPPER->l2_nextValidAddrOnPort_get)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->l2_nextValidAddrOnPort_get(port, pScanIdx, &l2Addr);
    RTK_API_UNLOCK();
    if(ret != RT_ERR_OK)
        return ret;

    ret = _rt_l2_age_calculate(l2Addr.age,&pL2Addr->age);
    if(ret != RT_ERR_OK)
        return ret;

    memcpy(pL2Addr->mac.octet,l2Addr.mac.octet,ETHER_ADDR_LEN);
    pL2Addr->port = l2Addr.port;
    pL2Addr->staticFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_STATIC)?ENABLED:DISABLED;
    pL2Addr->filterFlag = (l2Addr.flags & RTK_L2_UCAST_FLAG_SA_BLOCK)?ENABLED:DISABLED;
#endif
    return ret;
} /* end of rt_l2_nextValidAddrOnPort_get */

/* Function Name:
 *      rt_l2_illegalPortMoveAction_get
 * Description:
 *      Get forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
rt_l2_illegalPortMoveAction_get(rt_port_t port,rt_action_t *pFwdAction)
{
    int32   ret;

    if(pFwdAction == NULL)
        return RT_ERR_NULL_POINTER;

#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    /* function body */
    if (NULL == RT_MAPPER->l2_illegalPortMoveAction_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->l2_illegalPortMoveAction_get( port, pFwdAction);
    RTK_API_UNLOCK();
#else
    if(port)
    {
        /*not support now*/
    }
#endif
    return ret;
} /* end of rt_l2_illegalPortMoveAction_get */

/* Function Name:
 *      rt_l2_illegalPortMoveAction_set
 * Description:
 *      Set forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port      - port id
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
rt_l2_illegalPortMoveAction_set(rt_port_t port,rt_action_t fwdAction)
{
    int32   ret;

#if defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
    /* function body */
    if (NULL == RT_MAPPER->l2_illegalPortMoveAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->l2_illegalPortMoveAction_set( port, fwdAction);
    RTK_API_UNLOCK();
#else
    if(port || fwdAction)
    {
        /*not support now*/
    }
#endif
    return ret;
} /* end of rt_l2_illegalPortMoveAction_set */
