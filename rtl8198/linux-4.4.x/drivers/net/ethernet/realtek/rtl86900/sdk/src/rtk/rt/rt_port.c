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
 * $Revision: 82413 $
 * $Date: 2017-09-22 16:38:09 +0800 (Fri, 22 Sep 2017) $
 *
 * Purpose : Definition of Port API
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Parameter settings for the port-based view
 *           (2) RTCT
 */

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>
#include <rtk/init.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <rtk/rt/rt_port.h>
#include <rtk/rt/rt_common.h>
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#include <rt/include/rt_ext_mapper.h>
#endif

/*
 * Symbol Definition
 */
rt_port_uniType_t g_port_uniType[RTK_MAX_NUM_OF_PORTS];

/*
 * Data Declaration
 */

/*
 * Function Declaration
 */

/* Function Name:
 *      rt_port_init
 * Description:
 *      Initialize port module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Module must be initialized before using all of APIs in this module
 */
int32
rt_port_init(void)
{
    int32   ret;
    rt_port_t port;
    /* function body */
    if (NULL == RT_MAPPER->port_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_init();
    RTK_API_UNLOCK();

    for(port=0;port<RTK_MAX_NUM_OF_PORTS;port++)
        g_port_uniType[port] = RT_PORT_UNI_TYPE_VEIP;

    HAL_SCAN_ALL_PORT(port)
    {
        if(HAL_IS_GE_PORT(port) || HAL_IS_FE_PORT(port))
        {
            if(!HAL_IS_PON_PORT(port))
                rt_port_uniType_set(port,RT_PORT_UNI_TYPE_VEIP);
        }
    }

    return ret;
}   /* end of rt_port_init */

/* Function Name:
 *      rt_port_link_get
 * Description:
 *      Get the link status of the specific port
 * Input:
 *      port    - port id
 * Output:
 *      pLinkStatus - pointer to the link status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      The link status of the port is as following:
 *      - LINKDOWN
 *      - LINKUP
 */
int32
rt_port_link_get(rt_port_t port, rt_port_linkStatus_t *pLinkStatus)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_link_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_link_get(port, (rtk_port_linkStatus_t *)pLinkStatus);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_link_get */

/* Function Name:
 *      rt_port_speedDuplex_get
 * Description:
 *      Get the negotiated port speed and duplex status of the specific port
 * Input:
 *      port    - port id
 * Output:
 *      pSpeed  - pointer to the port speed
 *      pDuplex - pointer to the port duplex
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID       - invalid unit id
 *      RT_ERR_PORT_ID       - invalid port id
 *      RT_ERR_NULL_POINTER  - input parameter may be null pointer
 *      RT_ERR_PORT_LINKDOWN - link down port status
 * Note:
 *      (1) The speed type of the port is as following:
 *          - PORT_SPEED_10M
 *          - PORT_SPEED_100M
 *          - PORT_SPEED_1000M
 *
 *      (2) The duplex mode of the port is as following:
 *          - HALF_DUPLEX
 *          - FULL_DUPLEX
 */
int32
rt_port_speedDuplex_get(
    rt_port_t        port,
    rt_port_speed_t  *pSpeed,
    rt_port_duplex_t *pDuplex)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_speedDuplex_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_speedDuplex_get(port, (rtk_port_speed_t *)pSpeed, (rtk_port_duplex_t *)pDuplex);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_speedDuplex_get */

/* Function Name:
 *      rt_port_flowctrl_get
 * Description:
 *      Get the negotiated flow control status of the specific port
 * Input:
 *      port      - port id
 * Output:
 *      pTxStatus - pointer to the negotiation result of the Tx flow control
 *      pRxStatus - pointer to the negotiation result of the Rx flow control
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID       - invalid port id
 *      RT_ERR_NULL_POINTER  - input parameter may be null pointer
 *      RT_ERR_PORT_LINKDOWN - link down port status
 * Note:
 *      None
 */
int32
rt_port_flowctrl_get(
    rt_port_t  port,
    uint32      *pTxStatus,
    uint32      *pRxStatus)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_flowctrl_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_flowctrl_get(port, pTxStatus, pRxStatus);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_flowctrl_get */

/* Function Name:
 *      rt_port_phyAutoNegoEnable_get
 * Description:
 *      Get PHY ability of the specific port
 * Input:
 *      port    - port id
 * Output:
 *      pEnable - pointer to PHY auto negotiation status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_phyAutoNegoEnable_get(rt_port_t port, rt_enable_t *pEnable)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyAutoNegoEnable_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyAutoNegoEnable_get(port, pEnable);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyAutoNegoEnable_get */

/* Function Name:
 *      rt_port_phyAutoNegoEnable_set
 * Description:
 *      Set PHY ability of the specific port
 * Input:
 *      port   - port id
 *      enable - enable PHY auto negotiation
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID - invalid unit id
 *      RT_ERR_PORT_ID - invalid port id
 *      RT_ERR_INPUT   - input parameter out of range
 * Note:
 *      - ENABLED : switch to PHY auto negotiation mode
 *      - DISABLED: switch to PHY force mode
 *      - Once the abilities of both auto-nego and force mode are set,
 *        you can freely swtich the mode without calling ability setting API again
 */
int32
rt_port_phyAutoNegoEnable_set(rt_port_t port, rt_enable_t enable)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyAutoNegoEnable_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyAutoNegoEnable_set(port, enable);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyAutoNegoEnable_set */

/* Function Name:
 *      rt_port_phyAutoNegoAbility_get
 * Description:
 *      Get PHY auto negotiation ability of the specific port
 * Input:
 *      port    - port id
 * Output:
 *      pAbility - pointer to the PHY ability
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_phyAutoNegoAbility_get(
    rt_port_t              port,
    rt_port_phy_ability_t  *pAbility)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyAutoNegoAbility_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyAutoNegoAbility_get(port, (rtk_port_phy_ability_t *)pAbility);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyAutoNegoAbility_get */

/* Function Name:
 *      rt_port_phyAutoNegoAbility_set
 * Description:
 *      Set PHY auto negotiation ability of the specific port
 * Input:
 *      port     - port id
 *      pAbility - pointer to the PHY ability
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      You can set these abilities no matter which mode PHY currently stays on
 */
int32
rt_port_phyAutoNegoAbility_set(
    rt_port_t              port,
    rt_port_phy_ability_t  *pAbility)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyAutoNegoAbility_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyAutoNegoAbility_set(port, (rtk_port_phy_ability_t *)pAbility);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyAutoNegoAbility_set */

/* Function Name:
 *      rt_port_phyForceModeAbility_get
 * Description:
 *      Get PHY ability status of the specific port
 * Input:
 *      port         - port id
 * Output:
 *      pSpeed       - pointer to the port speed
 *      pDuplex      - pointer to the port duplex
 *      pFlowControl - pointer to the flow control enable status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_phyForceModeAbility_get(
    rt_port_t          port,
    rt_port_speed_t    *pSpeed,
    rt_port_duplex_t   *pDuplex,
    rt_enable_t        *pFlowControl)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyForceModeAbility_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyForceModeAbility_get(port, (rtk_port_speed_t *)pSpeed, (rtk_port_duplex_t *)pDuplex, (rtk_enable_t *)pFlowControl);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyForceModeAbility_get */

/* Function Name:
 *      rt_port_phyForceModeAbility_set
 * Description:
 *      Set the port speed/duplex mode/pause/asy_pause in the PHY force mode
 * Input:
 *      port        - port id
 *      speed       - port speed
 *      duplex      - port duplex mode
 *      flowControl - enable flow control
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_PHY_SPEED  - invalid PHY speed setting
 *      RT_ERR_PHY_DUPLEX - invalid PHY duplex setting
 *      RT_ERR_INPUT      - invalid input parameter
 * Note:
 *      (1) You can set these abilities no matter which mode PHY currently stays on
 *
 *      (2) The speed type of the port is as following:
 *          - PORT_SPEED_10M
 *          - PORT_SPEED_100M
 *
 *      (3) The duplex mode of the port is as following:
 *          - HALF_DUPLEX
 *          - FULL_DUPLEX
 */
int32
rt_port_phyForceModeAbility_set(
    rt_port_t          port,
    rt_port_speed_t    speed,
    rt_port_duplex_t   duplex,
    rt_enable_t        flowControl)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyForceModeAbility_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyForceModeAbility_set(port, speed, duplex, flowControl);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyForceModeAbility_set */

/* Function Name:
 *      rt_port_phyReg_get
 * Description:
 *      Get PHY register data of the specific port
 * Input:
 *      port  - port id
 *      page  - page id
 *      reg   - reg id
 * Output:
 *      pData - pointer to the PHY reg data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_PHY_PAGE_ID  - invalid page id
 *      RT_ERR_PHY_REG_ID   - invalid reg id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_phyReg_get(
    rt_port_t          port,
    uint32              page,
    rt_port_phy_reg_t  reg,
    uint32              *pData)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyReg_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyReg_get(port, page, reg, pData);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyReg_get */

/* Function Name:
 *      rt_port_phyReg_set
 * Description:
 *      Set PHY register data of the specific port
 * Input:
 *      port - port id
 *      page - page id
 *      reg  - reg id
 *      data - reg data
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID     - invalid port id
 *      RT_ERR_PHY_PAGE_ID - invalid page id
 *      RT_ERR_PHY_REG_ID  - invalid reg id
 * Note:
 *      None
 */
int32
rt_port_phyReg_set(
    rt_port_t          port,
    uint32              page,
    rt_port_phy_reg_t  reg,
    uint32              data)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_phyReg_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_phyReg_set(port, page, reg, data);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_phyReg_set */

/* Function Name:
 *      rt_port_adminEnable_get
 * Description:
 *      Get port admin status of the specific port
 * Input:
 *      port    - port id
 * Output:
 *      pEnable - pointer to the port admin status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_adminEnable_get(rt_port_t port, rt_enable_t *pEnable)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_adminEnable_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_adminEnable_get(port, pEnable);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_adminEnable_get */

/* Function Name:
 *      rt_port_adminEnable_set
 * Description:
 *      Set port admin status of the specific port
 * Input:
 *      port    - port id
 *      enable  - port admin status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 * Note:
 *      None
 */
int32
rt_port_adminEnable_set(rt_port_t port, rt_enable_t enable)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_adminEnable_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_adminEnable_set(port, enable);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_adminEnable_set */

/* Function Name:
 *      rt_port_isolationEntry_get
 * Description:
 *      Get Port isolation portmask
 * Input:
 *      mode            - Configuration 0 or 1
 *      port            - Ingress port
 * Output:
 *      pPortmask       - Isolation portmask for specified ingress port.
 *      pExtPortmask    - Isolation extension portmask for specified ingress port.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER
 * Note:
 *      None.
 */
int32
rt_port_isolationEntry_get(rt_port_isoConfig_t mode, rt_port_t port, rt_portmask_t *pPortmask, rt_portmask_t *pExtPortmask)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->port_isolationEntry_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_isolationEntry_get(mode, port, pPortmask, pExtPortmask);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_port_isolationEntry_get */

/* Function Name:
 *      rt_port_isolationEntry_set
 * Description:
 *      Set Port isolation portmask
 * Input:
 *      mode            - Configuration 0 or 1
 *      port            - Ingress port
 *      pPortmask       - Isolation portmask for specified ingress port.
 *      pExtPortmask    - Isolation extension portmask for specified ingress port.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER
 * Note:
 *      pExtPortmask is the extension egress portmask toward CPU port.
 *      If users specify an empty extension portmask and CPU port is set in pPortmask,
 *      the packets will be restricted to be forwarded to CPU.
 *      Likewise, If users specify an non-empty extension portmask and CPU port is not set in pPortmask,
 *      the packets will be restricted to be forwarded to CPU. too.
 */
int32
rt_port_isolationEntry_set(rt_port_isoConfig_t mode, rt_port_t port, rt_portmask_t *pPortmask, rt_portmask_t *pExtPortmask)
{
    int32   ret;
#if 0
    rtk_acl_ingress_entry_t aclRuleEntry;
    static uint singleGroup[RTK_MAX_NUM_OF_PORTS]={0};
    int count,i;
#endif
    /* function body */
    if (NULL == RT_MAPPER->port_isolationEntry_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->port_isolationEntry_set(mode, port, pPortmask, pExtPortmask);
    RTK_API_UNLOCK();

#if 0
    if(ret != RT_ERR_OK)
        return ret;
    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return ret;
    count = 0;
    for(i=0;i<RTK_MAX_NUM_OF_PORTS;i++)
    {
        if((HAL_IS_GE_PORT(i) || HAL_IS_FE_PORT(i))&& !HAL_IS_PON_PORT(i) && ((pPortmask->bits[0] & (1<<i)) > 0 ))
            count++;
    }
    if(count == 0)
        singleGroup[port] = 1;
    else
        singleGroup[port] = 0;

    memset(&aclRuleEntry,0,sizeof(rtk_acl_ingress_entry_t));
    aclRuleEntry.pFieldHead = NULL;
    aclRuleEntry.valid = ENABLED;
    for(i=0;i<RTK_MAX_NUM_OF_PORTS;i++)
    {
        aclRuleEntry.activePorts.bits[0] |= singleGroup[i] << i;
    }
    if(aclRuleEntry.activePorts.bits[0] == 0)
    {
         if (NULL == RT_MAPPER->acl_igrRuleEntry_del)
            return RT_ERR_DRIVER_NOT_FOUND;

        RTK_API_LOCK();
        ret = RT_MAPPER->acl_igrRuleEntry_del(RT_COMMON_PORT_ISOLATION_ACL_INDEX);
        RTK_API_UNLOCK();
    }
    else
    {
        aclRuleEntry.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
        aclRuleEntry.act.forwardAct.act = ACL_IGR_FORWARD_REDIRECT_ACT;
        aclRuleEntry.act.forwardAct.portMask.bits[0] |= 0x20;  /*pon port*/
        aclRuleEntry.index = RT_COMMON_PORT_ISOLATION_ACL_INDEX;
        if (NULL == RT_MAPPER->acl_igrRuleEntry_add)
            return RT_ERR_DRIVER_NOT_FOUND;

        RTK_API_LOCK();
        ret = RT_MAPPER->acl_igrRuleEntry_add( &aclRuleEntry);
        RTK_API_UNLOCK();
    }
    
#endif
    return ret;
}   /* end of rt_port_isolationEntry_set */

/* Function Name:
 *      rt_port_uniType_get
 * Description:
 *      Get port uni type of the specific port
 * Input:
 *      port     - port id
 * Output:
 *      pUniType - pointer to the port uni type
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
rt_port_uniType_get(rt_port_t port, rt_port_uniType_t *pUniType)
{
    int32   ret=RT_ERR_OK;

    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

    if(HAL_IS_PON_PORT(port))
        return RT_ERR_PORT_ID;

    /* function body */
    *pUniType = g_port_uniType[port];

    return ret;
}   /* end of rt_port_uniType_get */

/* Function Name:
 *      rt_port_uniType_set
 * Description:
 *      Set port uni type of the specific port
 * Input:
 *      port    - port id
 *      uniType - port type
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 * Note:
 *      None
 */
int32
rt_port_uniType_set(rt_port_t port, rt_port_uniType_t uniType)
{
    int32 ret=RT_ERR_OK;
    int32 portIndex;
    rtk_portmask_t flood_portmask;
#if defined(CONFIG_SDK_RTL9607C)
    uint32 learningCount;
    uint32 action;
    rtk_action_t fwdAction;
    rtk_enable_t agingOut;
    uint32 addr = 0, index = 0;
    rtk_l2_addr_table_t l2table;
    rtk_l2_ucastAddr_t l2Addr;

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	rt_ext_mapper_t* rt_ext_mapper = rt_ext_mapper_get();
#endif
#endif
    if(!HAL_IS_GE_PORT(port) && !HAL_IS_FE_PORT(port))
        return RT_ERR_PORT_ID;

    if(HAL_IS_PON_PORT(port))
        return RT_ERR_PORT_ID;
    RT_PARAM_CHK(uniType>=RT_PORT_UNI_TYPE_END,RT_ERR_INPUT);

    /* function body */
    g_port_uniType[port] = uniType;
    memset(&flood_portmask,0,sizeof(rtk_portmask_t));
    for(portIndex=0;portIndex<RTK_MAX_NUM_OF_PORTS;portIndex++)
    {
        if(g_port_uniType[portIndex] == RT_PORT_UNI_TYPE_PPTP)
        {
            flood_portmask.bits[0] |= 1<<portIndex;
        }
    }
    if (NULL == RT_MAPPER->l2_lookupHitUnicastAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    if (NULL == RT_MAPPER->l2_lookupMissFloodPortMask_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    if (NULL == RT_MAPPER->l2_portLimitLearningCnt_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    if (NULL == RT_MAPPER->l2_illegalPortMoveAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    if (NULL == RT_MAPPER->l2_portAgingEnable_set)
        return RT_ERR_DRIVER_NOT_FOUND;
#if defined(CONFIG_SDK_RTL9607C)
    if(uniType == RT_PORT_UNI_TYPE_PPTP)
    {
        fwdAction = ACTION_FORWARD;
        agingOut = ENABLED;
    }
    else
    {
        fwdAction = ACTION_TRAP2CPU;
        agingOut = DISABLED;
    }
    if(flood_portmask.bits[0] != 0)
    {
        flood_portmask.bits[0] |= 0x20;  /*pon port*/
        action = 0;
        learningCount = 2112;
    }
    else
    {
        flood_portmask.bits[0] |= 0x200; /*cpu port*/
        action = 1;
        learningCount = 0;
    }
    RTK_API_LOCK();
    ret = RT_MAPPER->l2_lookupHitUnicastAction_set(action);
    if(ret != RT_ERR_OK)
    {
        RTK_API_UNLOCK();
        return ret;
    }
    ret = RT_MAPPER->l2_lookupMissFloodPortMask_set( DLF_TYPE_UCAST, &flood_portmask);
    if(ret != RT_ERR_OK)
    {
        RTK_API_UNLOCK();
        return ret;
    }
    ret = RT_MAPPER->l2_portLimitLearningCnt_set( 5, learningCount);
    if(ret != RT_ERR_OK)
    {
        RTK_API_UNLOCK();
        return ret;
    }
    ret = RT_MAPPER->l2_illegalPortMoveAction_set( port, fwdAction);
    if(ret != RT_ERR_OK)
    {
        RTK_API_UNLOCK();
        return ret;
    }
    ret = RT_MAPPER->l2_portAgingEnable_set( port, agingOut);
    if(ret != RT_ERR_OK)
    {
        RTK_API_UNLOCK();
        return ret;
    }

    if(action == 1)
    {
        while(1)
        {
            memset(&l2table, 0x0, sizeof(rtk_l2_addr_table_t));
            l2table.method = LUT_READ_METHOD_ADDRESS;
            l2table.entryType = RTK_LUT_END;
            index = addr;
            if(RT_ERR_OK == RT_MAPPER->l2_nextValidEntry_get(&addr,&l2table))
            {
                if(addr < index)
                    break;
                if (HAL_IS_PON_PORT(l2table.entry.l2UcEntry.port) && ((l2table.entry.l2UcEntry.flags & RTK_L2_UCAST_FLAG_STATIC) == 0))
                {
                
                    memset(&l2Addr, 0x00, sizeof(rtk_l2_ucastAddr_t));
                    l2Addr.fid = 0;
                    memcpy(l2Addr.mac.octet, l2table.entry.l2UcEntry.mac.octet, ETHER_ADDR_LEN);
                    RT_MAPPER->l2_addr_del(&l2Addr);
                }
            }
            else
            {
                if(addr > HAL_L2_LEARN_LIMIT_CNT_MAX())
                {
                    break;
                }
            }
            addr++;
        }
    }

    RTK_API_UNLOCK();

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
    if(NULL != rt_ext_mapper) { 
    	if (NULL != rt_ext_mapper->internal_l3PortUniType_set) {
    		ret = rt_ext_mapper->internal_l3PortUniType_set(port, uniType);
    	}
    }   
#endif

#else
    /* Not support now */
#endif
    return ret;
}   /* end of rt_port_uniType_set */
