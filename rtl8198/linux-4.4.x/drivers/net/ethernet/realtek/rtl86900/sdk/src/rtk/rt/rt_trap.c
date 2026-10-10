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
#include <common/rt_type.h>
#include <rtk/port.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <rtk/rt/rt_trap.h>

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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_init();
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_portIgmpMldCtrlPktAction_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_portIgmpMldCtrlPktAction_get(port, igmpMldType, pAction);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_portIgmpMldCtrlPktAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_portIgmpMldCtrlPktAction_set(port, igmpMldType, action);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_oamPduAction_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_oamPduAction_get(pAction);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_oamPduAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_oamPduAction_set(action);
    RTK_API_UNLOCK();
    return ret;
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
#if 1 /* TBD */
    if(pAction == NULL)
        return RT_ERR_NULL_POINTER;
    return RT_ERR_OK;
#else
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_omciAction_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->trap_omciAction_get(pAction);
    RTK_API_UNLOCK();
    return ret;
#endif
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
#if 1 /* TBD */
    if(action >= ACTION_END)
        return RT_ERR_FAILED;
    return RT_ERR_OK;
#else
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->trap_omciAction_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    /* TBD */
    /* ret = RT_MAPPER->trap_omciAction_set(action); */
    ret = RT_ERR_OK;
    RTK_API_UNLOCK();
    return ret;
#endif
}   /* end of rt_trap_omciAction_set */

