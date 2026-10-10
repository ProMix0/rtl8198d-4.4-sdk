#include <stdio.h>
#include <string.h>
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <diag_util.h>
#include <diag_str.h>
#include <parser/cparser_priv.h>
#include <rtk/rt/rt_qos.h>

/*
 * rt_qos init
 */
cparser_result_t
cparser_cmd_rt_qos_init(
    cparser_context_t *context)
{
    int32 ret = RT_ERR_FAILED;

    DIAG_UTIL_PARAM_CHK();

    /*init rate module*/
    DIAG_UTIL_ERR_CHK(rt_qos_init(), ret);

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_qos_init */

/*
 * rt_qos get scheduling algorithm port ( <PORT_LIST:ports> | all ) queue-id <UINT:qid>
 */
cparser_result_t
cparser_cmd_rt_qos_get_scheduling_algorithm_port_ports_all_queue_id_qid(
    cparser_context_t *context,
    char * *ports_ptr,
    uint32_t  *qid_ptr)
{
    diag_portlist_t portlist;
    rt_port_t port;
    int32 ret = RT_ERR_FAILED;
    uint32 phyPort;
    uint32 reg;
    rt_qos_queue_weights_t weight;

    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_OUTPUT_INIT();

    DIAG_UTIL_ERR_CHK((RTK_MAX_NUM_OF_QUEUE<=*qid_ptr), ret);

    DIAG_UTIL_ERR_CHK(DIAG_UTIL_EXTRACT_PORTLIST(portlist, 5), ret);

    DIAG_UTIL_PORTMASK_SCAN(portlist, port)
    {
        DIAG_UTIL_ERR_CHK(rt_qos_schedulingQueue_get(port,&weight), ret);
        diag_util_mprintf("port: %d queue: %d type:%s\n",port,*qid_ptr,(weight.weights[*qid_ptr]==0)?DIAG_STR_QUEUE_STRICT:DIAG_STR_QUEUE_WFQ );
    }

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_qos_get_scheduling_algorithm_port_ports_all_queue_id_qid */

/*
 * rt_qos set scheduling algorithm port ( <PORT_LIST:ports> | all ) queue-id <UINT:qid> ( strict | wfq )
 */
cparser_result_t
cparser_cmd_rt_qos_set_scheduling_algorithm_port_ports_all_queue_id_qid_strict_wfq(
    cparser_context_t *context,
    char * *ports_ptr,
    uint32_t  *qid_ptr)
{
    diag_portlist_t portlist;
    rt_port_t port;
    int32 ret = RT_ERR_FAILED;
    uint32 phyPort;
    uint32 reg;
    uint32 type;
    rt_qos_queue_weights_t weight;


    DIAG_UTIL_ERR_CHK((RTK_MAX_NUM_OF_QUEUE<=*qid_ptr), ret);

    DIAG_UTIL_PARAM_CHK();
    if ('s' == TOKEN_CHAR(8,0))
        type = 0;  /*strict*/
    else
        type = 1;  /*wfq*/

    DIAG_UTIL_ERR_CHK(DIAG_UTIL_EXTRACT_PORTLIST(portlist, 5), ret);

    DIAG_UTIL_PORTMASK_SCAN(portlist, port)
    {
        DIAG_UTIL_ERR_CHK(rt_qos_schedulingQueue_get(port, &weight), ret);
        weight.weights[*qid_ptr] = type;
        DIAG_UTIL_ERR_CHK(rt_qos_schedulingQueue_set(port, &weight), ret);
    }

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_qos_set_scheduling_algorithm_port_ports_all_queue_id_qid_strict_wfq */


/*
 * rt_qos get scheduling queue-weight port ( <PORT_LIST:ports> | all ) queue-id <UINT:qid>
 */
cparser_result_t
cparser_cmd_rt_qos_get_scheduling_queue_weight_port_ports_all_queue_id_qid(
    cparser_context_t *context,
    char * *ports_ptr,
    uint32_t  *qid_ptr)
{
    diag_portlist_t portlist;
    rtk_port_t port;
    int32 ret = RT_ERR_FAILED;
    uint32 phyPort;
    uint32 reg,field;
    uint32 qid,phyQid;
    rtk_qos_queue_weights_t qWeight;
#ifdef CONFIG_SDK_APOLLO
    uint32 weight;
#endif

    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_OUTPUT_INIT();
    qid = *qid_ptr;

    DIAG_UTIL_ERR_CHK((RTK_MAX_NUM_OF_QUEUE<=*qid_ptr), ret);

    DIAG_UTIL_ERR_CHK(DIAG_UTIL_EXTRACT_PORTLIST(portlist, 5), ret);

    DIAG_UTIL_PORTMASK_SCAN(portlist, port)
    {
        DIAG_UTIL_ERR_CHK(rtk_qos_schedulingQueue_get(port, &qWeight), ret);
        diag_util_mprintf("port: %d queue: %d weight:%d\n",port,*qid_ptr,qWeight.weights[*qid_ptr]);
    }
    return CPARSER_OK;
}    /* end of cparser_cmd_qos_get_scheduling_queue_weight_port_ports_all_queue_id_qid */

/*
 * rt_qos set scheduling queue-weight port ( <PORT_LIST:ports> | all ) queue-id <UINT:qid> weight <UINT:weight>
 */
cparser_result_t
cparser_cmd_rt_qos_set_scheduling_queue_weight_port_ports_all_queue_id_qid_weight_weight(
    cparser_context_t *context,
    char * *ports_ptr,
    uint32_t  *qid_ptr,
    uint32_t  *weight_ptr)
{
    diag_portlist_t portlist;
    rtk_port_t port;
    int32 ret = RT_ERR_FAILED;
    uint32 phyPort;
    uint32 reg,field;
    uint32 qid,phyQid;
    rtk_qos_queue_weights_t qWeight;
    uint32 weight;

    weight = *weight_ptr;
    qid = *qid_ptr;

    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_ERR_CHK((RTK_MAX_NUM_OF_QUEUE<=*qid_ptr), ret);

    DIAG_UTIL_ERR_CHK(DIAG_UTIL_EXTRACT_PORTLIST(portlist, 5), ret);

    DIAG_UTIL_PORTMASK_SCAN(portlist, port)
    {
        DIAG_UTIL_ERR_CHK(rtk_qos_schedulingQueue_get(port, &qWeight), ret);
        qWeight.weights[qid] = weight;
        DIAG_UTIL_ERR_CHK(rtk_qos_schedulingQueue_set(port, &qWeight), ret);
        break;
    }

    return CPARSER_OK;
}    /* end of cparser_cmd_qos_set_scheduling_queue_weight_port_ports_all_queue_id_qid_weight_weight */
