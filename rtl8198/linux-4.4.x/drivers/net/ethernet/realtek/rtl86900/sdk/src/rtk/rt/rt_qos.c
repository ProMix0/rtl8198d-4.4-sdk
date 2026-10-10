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
 * $Revision: 42383 $
 * $Date: 2013-08-28 14:10:44 +0800 (Wed, 28 Aug 2013) $
 *
 * Purpose : Definition of QoS API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Ingress Priority Decision
 *           (2) Egress Remarking
 *           (3) Queue Scheduling
 *           (4) Congestion avoidance
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
#include <rtk/rt/rt_qos.h>

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
 *      rt_qos_init
 * Description:
 *      Configure QoS initial settings
 * Input:
 *      None.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_QUEUE_NUM - Invalid queue number
 * Note:
 *
 *      The initialization does the following actions:
 *      - set input bandwidth control parameters to default values
 *      - set priority decision parameters
 *      - set scheduling parameters
 *      - disable port remark ability
 *      - CPU port init 8 using prioroty to queue mapping index 0
 *      - Other port init 1 queue using prioroty to queue mapping index 1
 */
int32
rt_qos_init(void)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->qos_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->qos_init();
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_qos_init */

/* Module Name    : QoS              */
/* Sub-module Name: Queue scheduling */

/* Function Name:
 *      rt_qos_schedulingQueue_get
 * Description:
 *      Get the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 *      port      - port id
 * Output:
 *      pQweights - the array of weights for WRR/WFQ queue (valid:1~128, 0 for STRICT_PRIORITY queue)
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      The types of queue are: WFQ_WRR_PRIORITY or STRICT_PRIORITY.
 *      If the weight is 0 then the type is STRICT_PRIORITY, else the type is WFQ_WRR_PRIORITY.
 */
int32
rt_qos_schedulingQueue_get(rt_port_t port, rt_qos_queue_weights_t *pQweights)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->qos_schedulingQueue_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->qos_schedulingQueue_get(port, (rtk_qos_queue_weights_t *)pQweights);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_qos_schedulingQueue_get */

/* Function Name:
 *      rt_qos_schedulingQueue_set
 * Description:
 *      Set the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 *      port      - port id
 *      pQweights - the array of weights for WRR/WFQ queue (valid:1~128, 0 for STRICT_PRIORITY queue)
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID          - Invalid port id
 *      RT_ERR_QOS_QUEUE_WEIGHT - Invalid queue weight
 * Note:
 *      The types of queue are: WFQ_WRR_PRIORITY or STRICT_PRIORITY.
 *      If the weight is 0 then the type is STRICT_PRIORITY, else the type is WFQ_WRR_PRIORITY.
 */
int32
rt_qos_schedulingQueue_set(rt_port_t port, rt_qos_queue_weights_t *pQweights)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->qos_schedulingQueue_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->qos_schedulingQueue_set(port, (rtk_qos_queue_weights_t *)pQweights);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_qos_schedulingQueue_set */


