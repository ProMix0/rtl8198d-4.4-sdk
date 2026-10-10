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
 * $Revision: 79393 $
 * $Date: 2017-06-05 13:37:38 +0800 (Mon, 05 Jun 2017) $
 *
 * Purpose : Definition of TRAP API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Configuration for traping packet to CPU
 *           (2) RMA
 *           (3) User defined RMA
 *           (4) System-wise management frame
 *           (5) System-wise user defined management frame
 *           (6) Per port user defined management frame
 *           (7) Packet with special flag or option
 *           (8) CFM and OAM packet
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
#include <common/rt_type.h>
#include <rtk/port.h>


/*
 * Symbol Definition
 */


/*
 * Data Declaration
 */

/*
 * Function Declaration
 */

/* Module Name    : Trap                                    */
/* Sub-module Name: Configuration for traping packet to CPU */
/* Function Name:
 *      rt_trap_init
 * Description:
 *      Initial the trap module of the specified device..
 * Input:
 *      unit - unit id
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
rt_trap_init(void)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* function body */
    SETSOCKOPT(RTDRV_RT_TRAP_INIT, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_trap_init */

/* Function Name:
 *      rt_trap_portIgmpMldCtrlPktAction_get
 * Description:
 *      Get the configuration about MLD control packets Action
 * Input:
 *      port        - The ingress port ID.
 *      igmpMldType - IGMP/MLD protocol type;
 * Output:
 *      pAction     - Action of IGMP/MLD control packet
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID
 *      RT_ERR_INPUT
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      None.
 */
int32
rt_trap_portIgmpMldCtrlPktAction_get(rt_port_t port, rt_trap_igmpMld_type_t igmpMldType, rt_action_t *pAction)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_trap_cfg.port, &port, sizeof(rt_port_t));
    osal_memcpy(&rt_trap_cfg.igmpMldType, &igmpMldType, sizeof(rt_trap_igmpMld_type_t));
    GETSOCKOPT(RTDRV_RT_TRAP_PORTIGMPMLDCTRLPKTACTION_GET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);
    osal_memcpy(pAction, &rt_trap_cfg.action, sizeof(rt_action_t));

    return RT_ERR_OK;
}   /* end of rt_trap_portIgmpMldCtrlPktAction_get */

/* Function Name:
 *      rt_trap_portIgmpMldCtrlPktAction_set
 * Description:
 *      Set the configuration about MLD control packets Action
 * Input:
 *      port        - The ingress port ID.
 *      igmpMldType - IGMP/MLD protocol type;
 *      action      - Action of IGMP/MLD control packet
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID
 *      RT_ERR_INPUT
 * Note:
 *      None.
 */
int32
rt_trap_portIgmpMldCtrlPktAction_set(rt_port_t port, rt_trap_igmpMld_type_t igmpMldType, rt_action_t action)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* function body */
    osal_memcpy(&rt_trap_cfg.port, &port, sizeof(rt_port_t));
    osal_memcpy(&rt_trap_cfg.igmpMldType, &igmpMldType, sizeof(rt_trap_igmpMld_type_t));
    osal_memcpy(&rt_trap_cfg.action, &action, sizeof(rt_action_t));
    SETSOCKOPT(RTDRV_RT_TRAP_PORTIGMPMLDCTRLPKTACTION_SET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_trap_portIgmpMldCtrlPktAction_set */

/* Module Name    : Trap       */
/* Sub-module Name: OAM packet */

/* Function Name:
 *      rt_trap_oamPduAction_get
 * Description:
 *      Get forwarding action of trapped oam PDU on specified port.
 * Input:
 *      None.
 * Output:
 *      pAction - pointer to forwarding action of trapped oam PDU
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_TRAP2CPU
 */
int32
rt_trap_oamPduAction_get(rt_action_t *pAction)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    /* function body */
    GETSOCKOPT(RTDRV_RT_TRAP_OAMPDUACTION_GET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);
    osal_memcpy(pAction, &rt_trap_cfg.action, sizeof(rt_action_t));

    return RT_ERR_OK;
}   /* end of rt_trap_oamPduAction_get */

/* Function Name:
 *      rt_trap_oamPduAction_set
 * Description:
 *      Set forwarding action of trapped oam PDU on specified port.
 * Input:
 *      action - forwarding action of trapped oam PDU
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
 *      - ACTION_TRAP2CPU
 */
int32
rt_trap_oamPduAction_set(rt_action_t action)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* function body */
    osal_memcpy(&rt_trap_cfg.action, &action, sizeof(rt_action_t));
    SETSOCKOPT(RTDRV_RT_TRAP_OAMPDUACTION_SET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_trap_oamPduAction_set */

/* Function Name:
 *      rt_trap_omciAction_get
 * Description:
 *      Get forwarding action of OMCI.
 * Input:
 *      None.
 * Output:
 *      pAction - pointer to forwarding action of OMCI
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_TRAP2CPU
 */
int32
rt_trap_omciAction_get(rt_action_t *pAction)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    /* function body */
    GETSOCKOPT(RTDRV_RT_TRAP_OMCIACTION_GET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);
    osal_memcpy(pAction, &rt_trap_cfg.action, sizeof(rt_action_t));

    return RT_ERR_OK;
}   /* end of rt_trap_omciAction_get */

/* Function Name:
 *      rt_trap_omciAction_set
 * Description:
 *      Set forwarding action of OMCI.
 * Input:
 *      action - forwarding action of OMCI
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
 *      - ACTION_TRAP2CPU
 */
int32
rt_trap_omciAction_set(rt_action_t action)
{
    rtdrv_rt_trapCfg_t rt_trap_cfg;

    /* function body */
    osal_memcpy(&rt_trap_cfg.action, &action, sizeof(rt_action_t));
    SETSOCKOPT(RTDRV_RT_TRAP_OMCIACTION_SET, &rt_trap_cfg, rtdrv_rt_trapCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_trap_omciAction_set */

