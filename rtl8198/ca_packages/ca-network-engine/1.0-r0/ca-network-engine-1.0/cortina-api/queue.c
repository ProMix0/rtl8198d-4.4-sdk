#include "osal_cmn.h"
#include "aal_common.h"
#include "queue.h"
#include "port.h"
#include "aal_l2_tm.h"
#include "aal_l2_tm_cb.h"
#include "aal_l3_tm.h"
#include "aal_l2_te.h"
#include "aal_l3_te.h"
#include "aal_l3_te_cb.h"
#include "aal_l2_qm.h"
#include "aal_arb.h"
#include "aal_ni_l2.h"
#include "log.h"
#include "ca-scfg/scfg.h"


#define __QUEUE_LOG(fmt,...)  \
    do { cap_log_msg(0,CA_MOD_DRVR_DEV, CAP_LOG_DEBUG, "%s():"fmt, __FUNCTION__,##__VA_ARGS__); } while (0)

#define L3_WRED_SELECT_UC_OFFSEST 64

#ifdef CONFIG_ARCH_CORTINA_G3LITE
#define __G3_L2_TOTAL_BUFFER_SIZE  131071
#define __G3_L2_EACH_BUF_SIZE      32
#else
#define __G3_L2_TOTAL_BUFFER_SIZE  262143
#define __G3_L2_EACH_BUF_SIZE      64
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
extern ca_boolean_t g_l3fe_to_wan_flag;
#endif

extern ca_uint32_t l3_te_wred_mark_idx;
extern ca_uint32_t l3_te_wred_unmark_idx;

ca_uint16_t  __g3_pfc_src_pri_port_mapping[2] = {0xffff, 0xffff};

static ca_status_t _queue_l2_length_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  queue,
    CA_IN ca_uint32_t  length
)
{
    aal_l2_tm_cb_comb_threshold_t      voq_profile;
    aal_l2_tm_cb_comb_threshold_mask_t voq_profile_msk;
    ca_uint32_t prev_index = 0;
    ca_uint32_t index = 0;
    ca_uint32_t ref_counter = 0;
    ca_uint8_t  voq_id = 0;

    if (aal_l2_tm_voq_buff_profile_lookup(length, &index))
    {
        /* a profile entry is checked out, which can be reused by new port & queue */

        __QUEUE_LOG("VoQ buffer entry lookup ok - index %d can be resued\n", index);

        /* operate hardware port & queue based profile selection table */

        /* modify profile selection for UC */
        voq_id = port_id * 8 + queue;
        if (aal_l2_tm_cb_voq_profile_sel_set(device_id, voq_id, index) != CA_E_OK)
        {
            return CA_E_ERROR;
        }

        /* update software selection table */
        prev_index = aal_l2_tm_voq_buff_profile_sel_get(port_id, queue);
#ifdef RTK_MEM_CORRUPTTION_FIX
        if (prev_index == 0xff){
                printk("aal_l2_tm_voq_buff_profile_sel_get return 0xff\n");
                return CA_E_ERROR;
        }
#endif

        __QUEUE_LOG("prev index : %d, ref_counter : %d\n", prev_index, aal_l2_tm_voq_buff_profile_ref_counter_get(prev_index));

        /* VoQ buff has been configured before, this is just a modification operation */
        aal_l2_tm_voq_buff_profile_ref_dec(prev_index);

        if (aal_l2_tm_voq_buff_profile_ref_counter_get(prev_index) == 0)
        {
            /* the prev_index profile is free, just update hardware table accordingly */
            memset(&voq_profile, 0x0, sizeof(voq_profile));
            memset(&voq_profile_msk, 0x0, sizeof(voq_profile_msk));

            voq_profile_msk.s.threshold_hi = 1;
            voq_profile_msk.s.threshold_lo = 1;

            voq_profile.threshold_hi = 0;
            voq_profile.threshold_lo = 0;

            /* clear hardware VoQ buff profile table */
            (void)aal_l2_tm_cb_voq_threshold_profile_set(device_id, prev_index, voq_profile_msk, &voq_profile);

            /* clear software WRED profile table */
            (void)aal_l2_tm_voq_buff_profile_set(prev_index, 0);    /* buffer size 0 */
        }

        /* assign new index for this VoQ in software table */
        aal_l2_tm_voq_buff_profile_sel_set(port_id, queue, index);

        /* update software profile table */
        aal_l2_tm_voq_buff_profile_ref_inc(index); /* reference counter ++ */

    }
    else
    {
        /* no profile entry can be reused */
        prev_index  = aal_l2_tm_voq_buff_profile_sel_get(port_id, queue);
        __QUEUE_LOG("no profile entry can be reused\n");
        __QUEUE_LOG("prev_index : %d\n", prev_index);
#ifdef RTK_MEM_CORRUPTTION_FIX
	if (prev_index == 0xff){
		printk("aal_l2_tm_voq_buff_profile_sel_get return 0xff\n");
		return CA_E_ERROR;
	}
#endif
        ref_counter = aal_l2_tm_voq_buff_profile_ref_counter_get(prev_index);
        __QUEUE_LOG("ref counter : %d", ref_counter);
        if (ref_counter == 1)
        {
            __QUEUE_LOG("directly change profile\n");

            /* this profile has been configured before and reference counter is 1,
            ** just directly change profile because it is only used by this port and queue
            */
            memset(&voq_profile, 0x0, sizeof(voq_profile));
            memset(&voq_profile_msk, 0x0, sizeof(voq_profile_msk));

            voq_profile_msk.s.threshold_hi = 1;
            voq_profile_msk.s.threshold_lo = 1;

            voq_profile.threshold_hi = (length + __BM_BUF_BLOCK_SIZE - 1) / __BM_BUF_BLOCK_SIZE;  /* convert API length to 32/64B unit, ceiling(x) */
            voq_profile.threshold_lo = voq_profile.threshold_hi * 2 / 3;                          /* low threshold is 2/3 of high threshold     */

            /* operate hardware VoQ buffer profile table */
            if (aal_l2_tm_cb_voq_threshold_profile_set(device_id, prev_index, voq_profile_msk, &voq_profile) != CA_E_OK)
            {
                return CA_E_ERROR;
            }

            /* modify software profile table */
            aal_l2_tm_voq_buff_profile_set(prev_index, length);

        }
        else
        {
            /* reference counter must be larger than 1 in this case */
            if (aal_l2_tm_voq_buff_profile_allocate(&index))       /* allocate new one */
            {
                __QUEUE_LOG("allocate new entry index %d\n", index);

                memset(&voq_profile, 0x0, sizeof(voq_profile));
                memset(&voq_profile_msk, 0x0, sizeof(voq_profile_msk));

                voq_profile_msk.s.threshold_hi = 1;
                voq_profile_msk.s.threshold_lo = 1;

                voq_profile.threshold_hi = (length + __BM_BUF_BLOCK_SIZE - 1) / __BM_BUF_BLOCK_SIZE;    /* convert API length to 32/64B unit, ceiling(x) */
                voq_profile.threshold_lo = voq_profile.threshold_hi * 2 / 3;                            /* low threshold is 2/3 of high threshold     */

                /* operate hardware WRED profile table */
                if (aal_l2_tm_cb_voq_threshold_profile_set(device_id, index, voq_profile_msk, &voq_profile) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* operate hardware port & queue based profile selection table */
                voq_id = port_id * 8 + queue;
                if (aal_l2_tm_cb_voq_profile_sel_set(device_id, voq_id, index) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* update software profile table */
                aal_l2_tm_voq_buff_profile_set(index, length);
                aal_l2_tm_voq_buff_profile_ref_inc(index);      /* reference counter ++ */
                __QUEUE_LOG("index %d ref counter : %d\n", index, aal_l2_tm_voq_buff_profile_ref_counter_get(index));

                /* update software selection table */
                aal_l2_tm_voq_buff_profile_sel_set(port_id, queue, index);

                /* decrease previous profile reference counter */
                aal_l2_tm_voq_buff_profile_ref_dec(prev_index);
                __QUEUE_LOG("prev index %d ref counter : %d\n", prev_index, aal_l2_tm_voq_buff_profile_ref_counter_get(prev_index));
            }
            else
            {
                return CA_E_RESOURCE;
            }
        }
    }

    return CA_E_OK;
}

#define __CA_MAX_PACKET_SIZE 2048

ca_status_t ca_queue_length_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue,
    CA_IN ca_uint32_t    length
)
{
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;

    if (length > CA_AAL_L2_BM_MAX_VOQ_BUFF_SIZE || length < __CA_MAX_PACKET_SIZE)
        return CA_E_PARAM;

    if (queue >= CA_QUEUE_COUNT)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        return CA_E_NOT_SUPPORT;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        return CA_E_NOT_SUPPORT;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif


    if (port_type == CA_PORT_TYPE_ETHERNET ||
        port_type == CA_PORT_TYPE_EPON ||
        port_type == CA_PORT_TYPE_GPON)  /* LAN */
        return _queue_l2_length_set(device_id, port, queue, length);

    return CA_E_OK;
}

ca_status_t ca_queue_length_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint32_t  queue,
    CA_OUT ca_uint32_t  *length
)
{
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;
    ca_uint32_t index = 0;

    if (!length)
        return CA_E_PARAM;

    if (queue > CA_QUEUE_COUNT)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        return CA_E_NOT_SUPPORT;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        return CA_E_NOT_SUPPORT;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif


    if (port_type == CA_PORT_TYPE_ETHERNET ||
        port_type == CA_PORT_TYPE_EPON ||
        port_type == CA_PORT_TYPE_GPON)  /* LAN */
    {
        index = aal_l2_tm_voq_buff_profile_sel_get(port, queue);
        if (aal_l2_tm_voq_buff_profile_get(index, length) != CA_E_OK)
            return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t ca_queue_pause_threshold_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue,
    CA_IN ca_uint32_t    xon,
    CA_IN ca_uint32_t    xoff,
    CA_IN ca_uint32_t    pause_quanta
)
{
    ca_uint32_t                     logic_port_id = 0;

    aal_l2_tm_cb_src_ctrl_mask_t    src_ctrl_mask;
    aal_l2_tm_cb_src_ctrl_t         src_ctrl;
    aal_l2_tm_cb_threshold_t        cb_thrhd;
    aal_ni_xge_pfc_quanta_mask_t    xge_pfc_quanta_mask;
    aal_ni_xge_pfc_quanta_t         xge_pfc_quanta;

    ca_status_t             ret = CA_E_OK;

    CA_PORT_ID_CHECK(port_id);
    if((queue > 7) || (xon > __G3_L2_TOTAL_BUFFER_SIZE ) || (xoff>xon) || (pause_quanta > 0xffff)){
        return CA_E_PARAM;
    }

    src_ctrl_mask.u32 = 0;
    memset(&src_ctrl, 0 , sizeof(aal_l2_tm_cb_src_ctrl_t));
    memset(&cb_thrhd, 0 , sizeof(aal_l2_tm_cb_threshold_t));
    memset(&xge_pfc_quanta, 0 , sizeof(aal_ni_xge_pfc_quanta_t));

    logic_port_id = PORT_ID(port_id);

    if((__g3_pfc_src_pri_port_mapping[0] == 0xffff ) ||( __g3_pfc_src_pri_port_mapping[0] == logic_port_id)){
        __g3_pfc_src_pri_port_mapping[0] = logic_port_id;
        src_ctrl_mask.s.src0_pri_en = 1;
        src_ctrl_mask.s.src0_pri_nspid = 1;
        src_ctrl.src0_pri_en = TRUE;
        src_ctrl.src0_pri_nspid = logic_port_id;
        cb_thrhd.threshold_hi = (ca_uint16_t )(xon/__G3_L2_EACH_BUF_SIZE);
        cb_thrhd.threshold_lo = (ca_uint16_t )(xoff/__G3_L2_EACH_BUF_SIZE);
        ret = aal_l2_tm_cb_src0_pri_threshold_set(device_id, queue, &cb_thrhd);
        if(ret != CA_E_OK){
            goto end;
        }

    }else if((__g3_pfc_src_pri_port_mapping[1] == 0xffff)||( __g3_pfc_src_pri_port_mapping[1] == logic_port_id)){
        __g3_pfc_src_pri_port_mapping[1] = logic_port_id;

        src_ctrl_mask.s.src1_pri_en = 1;
        src_ctrl_mask.s.src1_pri_nspid = 1;
        src_ctrl.src1_pri_en = TRUE;
        src_ctrl.src1_pri_nspid = logic_port_id;

        cb_thrhd.threshold_hi = (ca_uint16_t )(xon/__G3_L2_EACH_BUF_SIZE);
        cb_thrhd.threshold_lo = (ca_uint16_t )(xoff/__G3_L2_EACH_BUF_SIZE);
        ret = aal_l2_tm_cb_src1_pri_threshold_set(device_id, queue, &cb_thrhd);
        if(ret != CA_E_OK){
            goto end;
        }
    }else{
        ret = CA_E_RESOURCE;
        goto end;
    }
    ret = aal_l2_tm_cb_src_ctrl_set(device_id,src_ctrl_mask,&src_ctrl);
    if(ret != CA_E_OK){
        goto end;
    }
    if(logic_port_id <= 4){
        ret = aal_ni_eth_pfc_quanta_set(device_id,logic_port_id,queue,pause_quanta);
    }else if (logic_port_id < 7){
        xge_pfc_quanta_mask.u32 = 0;
        switch(queue){
            case 0:
                xge_pfc_quanta_mask.s.xge_quanta_que0 = 1;
                xge_pfc_quanta.xge_quanta_que0 = pause_quanta;
                break;
            case 1:
                xge_pfc_quanta_mask.s.xge_quanta_que1 = 1;
                xge_pfc_quanta.xge_quanta_que1 = pause_quanta;
                break;
            case 2:
                xge_pfc_quanta_mask.s.xge_quanta_que2 = 1;
                xge_pfc_quanta.xge_quanta_que2 = pause_quanta;
                break;
            case 3:
                xge_pfc_quanta_mask.s.xge_quanta_que3 = 1;
                xge_pfc_quanta.xge_quanta_que3 = pause_quanta;
                break;
            case 4:
                xge_pfc_quanta_mask.s.xge_quanta_que4 = 1;
                xge_pfc_quanta.xge_quanta_que4 = pause_quanta;
                break;
            case 5:
                xge_pfc_quanta_mask.s.xge_quanta_que5 = 1;
                xge_pfc_quanta.xge_quanta_que5 = pause_quanta;
                break;
            case 6:
                xge_pfc_quanta_mask.s.xge_quanta_que6 = 1;
                xge_pfc_quanta.xge_quanta_que6 = pause_quanta;
                break;
            case 7:
                xge_pfc_quanta_mask.s.xge_quanta_que7 = 1;
                xge_pfc_quanta.xge_quanta_que7 = pause_quanta;
                break;
            default:
                break;
        }
            ret = aal_ni_xge_pfc_quanta_set(device_id,logic_port_id,xge_pfc_quanta_mask,&xge_pfc_quanta);
    }else{
 #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            ret = CA_E_NOT_SUPPORT;
            goto end;
 #else

 #endif
    }
end:
    return ret;
}

ca_status_t ca_queue_pause_threshold_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    queue,
    CA_OUT ca_uint32_t    *xon,
    CA_OUT ca_uint32_t    *xoff,
    CA_OUT ca_uint32_t    *pause_quanta
)
{
    ca_uint32_t                     logic_port_id = 0;
    aal_l2_tm_cb_threshold_t        cb_thrhd;
    aal_ni_xge_pfc_quanta_t         xge_pfc_quanta;

    ca_status_t             ret = CA_E_OK;

    CA_PORT_ID_CHECK(port_id);

    if((queue > 7) || (xon == NULL) || (xoff == NULL) ||(NULL==pause_quanta)){
        return CA_E_PARAM;
    }

    logic_port_id = PORT_ID(port_id);
    memset(&cb_thrhd, 0 , sizeof(aal_l2_tm_cb_threshold_t));

    if(__g3_pfc_src_pri_port_mapping[1] == logic_port_id){
        ret = aal_l2_tm_cb_src1_pri_threshold_get(device_id, queue, &cb_thrhd);
         if(ret != CA_E_OK){
             goto end;
         }
    }else{
        ret = aal_l2_tm_cb_src0_pri_threshold_get(device_id, queue, &cb_thrhd);
         if(ret != CA_E_OK){
             goto end;
         }
    }
    *xon = cb_thrhd.threshold_hi * __G3_L2_EACH_BUF_SIZE;
    *xoff = cb_thrhd.threshold_lo * __G3_L2_EACH_BUF_SIZE;
    if(logic_port_id <= 4){
        ret = aal_ni_eth_pfc_quanta_get(device_id,logic_port_id,queue,pause_quanta);
    }else if((logic_port_id > 4)&&(logic_port_id <7)){
    ret = aal_ni_xge_pfc_quanta_get(device_id,logic_port_id,&xge_pfc_quanta);
    if(ret != CA_E_OK){
        goto end;
    }
    }
    switch(queue){
        case 0:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que0;
        break;
        case 1:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que1;
        break;
        case 2:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que2;
        break;
        case 3:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que3;
        break;
        case 4:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que4;
        break;
        case 5:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que5;
        break;
        case 6:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que6;
        break;
        case 7:
            *pause_quanta = xge_pfc_quanta.xge_quanta_que7;
        break;
        default:
            *pause_quanta = 0xFFFF;
        break;
    }
end:
    return ret;
}

ca_status_t ca_queue_schedule_set
(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN ca_queue_schedule_mode_t mode,
    CA_IN ca_queue_weights_t       *weights
)
{
    ca_uint32_t i = 0, j = 0;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;

    if (!weights)
        return CA_E_PARAM;

    port      = PORT_ID(port_id);
    port_type = PORT_TYPE(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN || port != CA_PORT_ID_L3_LAN))
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN || port != CA_PORT_ID_L3_LAN))
            return CA_E_PARAM;
    }

    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
	#ifndef CONFIG_RTL_G3LITE_QOS_SUPPORT
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
	#endif
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &g_l3fe_to_wan_flag) != CA_E_OK)
        {
            ca_printf("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT.\n", __func__);
        }
#endif

    if (mode != CA_SCHEDULE_STRICT_PRIORITY && mode != CA_SCHEDULE_DEFICIT_WEIGHT_ROUND_ROBIN)
        return CA_E_NOT_SUPPORT;

    if (mode == CA_SCHEDULE_STRICT_PRIORITY)
    {
        /* weights should be ZERO for all queues */
        memset((void *)weights, 0, sizeof(ca_uint32_t) * CA_QUEUE_COUNT);
    }
    else
    {
        /*DWRR or DWRR + SP */
        for (i=0; i<CA_QUEUE_COUNT; i++)
            if (weights->weights[i] > 0xff) /* SP - 0, WRR range 1 ~ 255 */
                return CA_E_PARAM;

        /* SP always has higher priority than DRWW */
        for (i=0; i<CA_QUEUE_COUNT; i++)
        {
            if (weights->weights[i] == 0)   /* SP */
            {
                for (j=i+1; j<CA_QUEUE_COUNT; j++)
                {
                    if (weights->weights[j])    /* */
                        return CA_E_PARAM;
                }
            }
        }

    }

    switch (port_type)
    {
    case CA_PORT_TYPE_L3:
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (g_l3fe_to_wan_flag)
            {
                /* bypass L3 ES, using L2 ES */
                port = CA_AAL_L2TM_DPID_L3_LAN;
                for (i=0; i<CA_QUEUE_COUNT; i++)
                {
                    if (aal_l2_tm_es_weight_ratio_set(device_id, port, i, weights->weights[i]) != CA_E_OK)
                        return CA_E_ERROR;
                }
            }
            else
            {
                /* pass through L3 ES, using L3 ES */
                port += CA_AAL_L3_TE_NI_PORT_OFFSET;
                for (i=0; i<CA_QUEUE_COUNT; i++)
                {
                    if (aal_l3_tm_es_weight_ratio_set(device_id, port, i, weights->weights[i]) != CA_E_OK)
                        return CA_E_ERROR;
                }
            }
#else
            port += CA_AAL_L3_TE_NI_PORT_OFFSET;
            for (i=0; i<CA_QUEUE_COUNT; i++)
            {
                if (aal_l3_tm_es_weight_ratio_set(device_id, port, i, weights->weights[i]) != CA_E_OK)
                    return CA_E_ERROR;
            }
#endif
        break;

    case CA_PORT_TYPE_EPON:
    case CA_PORT_TYPE_GPON:
    case CA_PORT_TYPE_ETHERNET:
        /* convert LDPID to TM LDPID */
        if (port == CA_PORT_ID_L3_WAN)
            port = CA_AAL_L2TM_DPID_L3_WAN;
        else if (port == CA_PORT_ID_L3_LAN)
            port = CA_AAL_L2TM_DPID_L3_LAN;

        /* l2 ES */
        for (i=0; i<CA_QUEUE_COUNT; i++)
        {
            if (aal_l2_tm_es_weight_ratio_set(device_id, port, i, weights->weights[i]) != CA_E_OK)
                return CA_E_ERROR;
        }
        break;

    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;     /* CPU entry 0 ~ 7 */
        for (i=0; i<CA_QUEUE_COUNT; i++)
        {
            if (aal_l3_tm_es_weight_ratio_set(device_id, port, i, weights->weights[i]) != CA_E_OK)
                return CA_E_ERROR;
        }
        break;

    default:
        break;
    }

    return CA_E_OK;
}

ca_status_t ca_queue_schedule_get
(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_OUT ca_queue_schedule_mode_t *mode,
    CA_OUT ca_queue_weights_t       *weights
)
{
    ca_uint32_t  i = 0;
    ca_boolean_t sp_flag = TRUE;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;

    if (!weights)
        return CA_E_PARAM;

    if (!mode)
        return CA_E_PARAM;

    port      = PORT_ID(port_id);
    port_type = PORT_TYPE(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN || port != CA_PORT_ID_L3_LAN))
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN || port != CA_PORT_ID_L3_LAN))
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
	#ifndef CONFIG_RTL_G3LITE_QOS_SUPPORT
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
	#endif
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &g_l3fe_to_wan_flag) != CA_E_OK)
    {
        ca_printf("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT.\n", __func__);
    }
#endif


    switch (port_type)
    {
    case CA_PORT_TYPE_L3:
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (g_l3fe_to_wan_flag)
            {
                /* bypass L3 ES, using L2 ES */
                port = CA_AAL_L2TM_DPID_L3_LAN;
                for (i=0; i<CA_QUEUE_COUNT; i++)
                {
                    if (aal_l2_tm_es_weight_ratio_get(device_id, port, i, &weights->weights[i]) != CA_E_OK)
                        return CA_E_ERROR;

                    if (weights->weights[i])
                        sp_flag = FALSE;
                }
            }
            else
            {
                /* pass through L3 ES, using L3 ES */
                port += CA_AAL_L3_TE_NI_PORT_OFFSET;
                for (i=0; i<CA_QUEUE_COUNT; i++)
                {
                    if (aal_l3_tm_es_weight_ratio_get(device_id, port, i, &weights->weights[i]) != CA_E_OK)
                        return CA_E_ERROR;

                    if (weights->weights[i])    /* anyone non-zero means DWRR or SP+DWRR */
                        sp_flag = FALSE;
                }
            }
#else
            port += CA_AAL_L3_TE_NI_PORT_OFFSET;
            for (i=0; i<CA_QUEUE_COUNT; i++)
            {
                if (aal_l3_tm_es_weight_ratio_get(device_id, port, i, &weights->weights[i]) != CA_E_OK)
                    return CA_E_ERROR;

                if (weights->weights[i])    /* anyone non-zero means DWRR or SP+DWRR */
                    sp_flag = FALSE;
            }
#endif
        break;

    case CA_PORT_TYPE_EPON:
    case CA_PORT_TYPE_GPON:
    case CA_PORT_TYPE_ETHERNET:
        /* convert LDPID to TM LDPID */
        if (port == CA_PORT_ID_L3_WAN)
            port = CA_AAL_L2TM_DPID_L3_WAN;
        else if (port == CA_PORT_ID_L3_LAN)
            port = CA_AAL_L2TM_DPID_L3_LAN;

        /* l2 ES */
        for (i=0; i<CA_QUEUE_COUNT; i++)
        {
            if (aal_l2_tm_es_weight_ratio_get(device_id, port, i, &weights->weights[i]) != CA_E_OK)
                return CA_E_ERROR;

            if (weights->weights[i])
                sp_flag = FALSE;
        }
        break;

    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;
        for (i=0; i<CA_QUEUE_COUNT; i++)
        {
            if (aal_l3_tm_es_weight_ratio_get(device_id, port, i, &weights->weights[i]) != CA_E_OK)
                return CA_E_ERROR;

            if (weights->weights[i])    /* anyone non-zero means DWRR or SP+DWRR */
                sp_flag = FALSE;
        }
        break;

    default:
        break;
    }

    if (sp_flag)
        *mode = CA_SCHEDULE_STRICT_PRIORITY;
    else
        *mode = CA_SCHEDULE_DEFICIT_WEIGHT_ROUND_ROBIN;

    return CA_E_OK;
}


static ca_status_t _queue_l2_wred_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t            port_id,
    CA_IN ca_uint32_t             queue,
    CA_IN ca_queue_wred_profile_t *profile
)
{
    aal_l2_te_wred_profile_t         wred_profile;
    aal_l2_te_wred_profile_msk_t     wred_profile_msk;
    aal_l2_te_wred_profile_sel_t     sel;
    aal_l2_te_wred_profile_sel_msk_t sel_msk;
    ca_uint32_t prev_index = 0;
    ca_uint32_t index = 0;
    ca_uint32_t ref_counter = 0;
    ca_uint32_t i = 0;
    ca_uint32_t k = 0;
    aal_l2_te_glb_cfg_t glb_cfg;
    aal_l2_te_glb_cfg_msk_t glb_msk;
    ca_int8_t sz_mark_info[256];
    ca_int8_t sz_unmark_info[256];
    ca_int8_t sz_mark_dp[16];
    ca_int8_t sz_unmark_dp[16];
    ca_uint32_t zero_dp[CA_AAL_WRED_MAX_DROP_PROBABILTY];
    ca_uint32_t non_congestion_threshold = CA_AAL_L2_QM_NON_CONGESTION_THRESHOLD;
    aal_l2_qm_buff_cfg_t     buff_cfg;
    aal_l2_qm_buff_cfg_msk_t buff_msk;


    if (!profile)
        return CA_E_PARAM;

    memset((void *)&zero_dp[0], 0, sizeof(zero_dp));

    memset((void *)&sz_mark_info[0], 0, sizeof(sz_mark_info));
    memset((void *)&sz_unmark_info[0], 0, sizeof(sz_unmark_info));


    sprintf(sz_mark_info,   "%s", "marked_dp   : ");
    sprintf(sz_unmark_info, "%s", "unmarked_dp : ");

    /* check and convert DP point to the value that G3 ASCI can recognize */
    for (i=0; i<CA_QUEUE_WRED_POINT_COUNT; i++)
    {
        if (profile->marked_dp[i] > 100 || profile->unmarked_dp[i] > 100)
            return CA_E_PARAM;

        sprintf(sz_mark_dp,   "%02x ", profile->marked_dp[i]);
        strcat(sz_mark_info, sz_mark_dp);
        sprintf(sz_unmark_dp, "%02x ", profile->unmarked_dp[i]);
        strcat(sz_unmark_info, sz_unmark_dp);
    }

    printk("%s\n", sz_mark_info);
    printk("%s\n", sz_unmark_info);

    if (aal_l2_te_wred_info_lookup(profile->marked_dp, profile->unmarked_dp, &index))
    {
        /* a profile entry is checked out, which can be reused by new port & queue */

        __QUEUE_LOG("WRED entry lookup ok - index %d can be resued\n", index);

        prev_index = aal_l2_te_wred_sel_info_get(port_id, queue);
        if (index == prev_index)
        {
            /*
            ** this case means same configurations are confgured to one Queue twice, so we do nothing.
            ** for example,
            ** configure q0 with unmarked DP 50, 50, .... 50
            ** and then configure q0 with unmarked DP 50, 50, ... 50 again.
            ** this branch will be asserted.
            */
            return CA_E_OK;
        }

        /* operate hardware port & queue based profile selection table */
        sel.profile_id[queue] = index;
        sel_msk.u32 = (1 << queue);

        /* modify profile selection for UC */
        if (aal_l2_te_wred_profile_sel_set(device_id, port_id, sel_msk, &sel) != CA_E_OK)
        {
            return CA_E_ERROR;
        }

        /* modify profile selection for ~UC */
        if (aal_l2_te_wred_profile_sel_set(device_id, port_id + 16, sel_msk, &sel) != CA_E_OK)
        {
            return CA_E_ERROR;
        }

        /* update software selection table */
        prev_index = aal_l2_te_wred_sel_info_get(port_id, queue);
#ifdef RTK_MEM_CORRUPTTION_FIX
        if (prev_index == 0xff){
                printk("aal_l2_te_wred_sel_info_get return 0xff\n");
                return CA_E_ERROR;
        }
#endif
        __QUEUE_LOG("prev index : %d, ref_counter : %d\n", prev_index, aal_l2_te_wred_profile_info_ref_counter_get(prev_index));

        /* WRED info has been configured before, this is just a modification operation */
        aal_l2_te_wred_profile_info_ref_dec(prev_index);

        if (aal_l2_te_wred_profile_info_ref_counter_get(prev_index) == 0)
        {
            /* the prev_index profile is free, just update hardware table accordingly */
            memset(&wred_profile, 0x0, sizeof(wred_profile));
            memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

            wred_profile_msk.s.mark_idx    = 1;
            wred_profile_msk.s.unmark_idx  = 1;
            wred_profile_msk.s.mark_dp     = 1;
            wred_profile_msk.s.unmark_dp   = 1;

            /* clear hardware WRED profile table */
            (void)aal_l2_te_wred_profile_set(device_id, prev_index, wred_profile_msk, &wred_profile);

            /* clear software WRED profile table */
            (void)aal_l2_te_wred_profile_info_set(prev_index, zero_dp, zero_dp);
        }

        aal_l2_te_wred_sel_info_set(port_id, queue, index);

        /* update software profile table */
        aal_l2_te_wred_profile_info_ref_inc(index); /* reference counter ++ */

    }
    else
    {
        /* no profile entry can be reused */
        prev_index  = aal_l2_te_wred_sel_info_get(port_id, queue);
        __QUEUE_LOG("no profile entry can be reused\n");
        __QUEUE_LOG("prev_index : %d\n", prev_index);
#ifdef RTK_MEM_CORRUPTTION_FIX
        if (prev_index == 0xff){
                printk("aal_l2_te_wred_sel_info_get return 0xff\n");
                return CA_E_ERROR;
        }
#endif
        ref_counter = aal_l2_te_wred_profile_info_ref_counter_get(prev_index);
        __QUEUE_LOG("ref counter : %d", ref_counter);
        if (ref_counter == 1)
        {
            __QUEUE_LOG("directly change profile\n");

            /* this profile has been configured before and reference counter is 1,
            ** just directly change profile because it is only used by this port and queue
            */
            memset(&wred_profile, 0x0, sizeof(wred_profile));
            memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

            wred_profile_msk.s.mark_idx    = 1;
            wred_profile_msk.s.unmark_idx  = 1;
            wred_profile_msk.s.mark_dp     = 1;
            wred_profile_msk.s.unmark_dp   = 1;

            #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
            wred_profile.mark_idx     = (CA_AAL_L2_WRED_MARK_IDX + 1);      // 8K buffer
            wred_profile.unmark_idx   = (CA_AAL_L2_WRED_UNMARK_IDX + 1);
            #else
            wred_profile.mark_idx     = CA_AAL_L2_WRED_MARK_IDX;            // 4k buffer
            wred_profile.unmark_idx   = CA_AAL_L2_WRED_UNMARK_IDX;
            #endif

            /* convert DP value from 100 based to 63 based */
            for (k=0; k<CA_AAL_WRED_MAX_DROP_PROBABILTY; k++)
            {
                wred_profile.mark_dp[k]   = (ca_uint32_t)(profile->marked_dp[k] * 100 / 158);
                wred_profile.unmark_dp[k] = (ca_uint32_t)(profile->unmarked_dp[k] * 100 / 158);
            }

            /* operate hardware WRED profile table */
            if (aal_l2_te_wred_profile_set(device_id, prev_index, wred_profile_msk, &wred_profile) != CA_E_OK)
            {
                return CA_E_ERROR;
            }

            /* modify software profile table */
            aal_l2_te_wred_profile_info_set(prev_index, profile->marked_dp, profile->unmarked_dp);

        }
        else
        {
            /* reference counter must be larger than 1 in this case */
            if (aal_l2_te_wred_profile_info_allocate(&index))       /* allocate new one */
            {
                __QUEUE_LOG("allocate new entry index %d\n", index);

                memset(&wred_profile, 0x0, sizeof(wred_profile));
                memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

                wred_profile_msk.s.mark_idx    = 1;
                wred_profile_msk.s.unmark_idx  = 1;
                wred_profile_msk.s.mark_dp     = 1;
                wred_profile_msk.s.unmark_dp   = 1;

                #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                wred_profile.mark_idx     = (CA_AAL_L2_WRED_MARK_IDX + 1);      // 8K buffer
                wred_profile.unmark_idx   = (CA_AAL_L2_WRED_UNMARK_IDX + 1);
                #else
                wred_profile.mark_idx     = CA_AAL_L2_WRED_MARK_IDX;            // 4k buffer
                wred_profile.unmark_idx   = CA_AAL_L2_WRED_UNMARK_IDX;
                #endif

                /* convert DP value from 100 based to 63 based */
                for (k=0; k<CA_AAL_WRED_MAX_DROP_PROBABILTY; k++)
                {
                    wred_profile.mark_dp[k]   = (ca_uint32_t)(profile->marked_dp[k] * 100 / 158);
                    wred_profile.unmark_dp[k] = (ca_uint32_t)(profile->unmarked_dp[k] * 100 / 158);
                }

                /* operate hardware WRED profile table */
                if (aal_l2_te_wred_profile_set(device_id, index, wred_profile_msk, &wred_profile) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* operate hardware port & queue based profile selection table */
                memset(&sel, 0x0, sizeof(sel));
                memset(&sel_msk, 0x0, sizeof(sel_msk));

                sel.profile_id[queue] = index;
                sel_msk.u32 = (1 << queue);

                /* configure profile selection for UC */
                if (aal_l2_te_wred_profile_sel_set(device_id, port_id, sel_msk, &sel) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* configure profile selection for ~UC */
                if (aal_l2_te_wred_profile_sel_set(device_id, port_id + 16, sel_msk, &sel) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* update software profile table */
                aal_l2_te_wred_profile_info_set(index, profile->marked_dp, profile->unmarked_dp);
                aal_l2_te_wred_profile_info_ref_inc(index); /* reference counter ++ */
                __QUEUE_LOG("index %d ref counter : %d\n", index, aal_l2_te_wred_profile_info_ref_counter_get(index));

                /* update software selection table */
                aal_l2_te_wred_sel_info_set(port_id, queue, index);

                /* decrease previous profile reference counter */
                aal_l2_te_wred_profile_info_ref_dec(prev_index);
                __QUEUE_LOG("prev index %d ref counter : %d\n", prev_index, aal_l2_te_wred_profile_info_ref_counter_get(prev_index));
            }
            else
            {
                return CA_E_RESOURCE;
            }
        }
    }

    /* initialize buff_cfg and buff_msk */
    memset((void *)&buff_cfg, 0x0, sizeof(buff_cfg));
    buff_msk.u32 = 0;
    buff_msk.s.non_cong_threshold = 1;


    /* L2 TE tail drop control */
    glb_msk.u32 = 0;
    memset((void *)&glb_cfg, 0, sizeof(glb_cfg));

    glb_msk.s.tail_drop_ena = 1;

    if (aal_l2_te_wred_info_empty())
    {
        glb_cfg.tail_drop_ena = TRUE;   /* turn on tail drop in case system does not have WRED profile configured */

        /* use startup configuration value if TD is enabled */

        /* read non-congestion buffer threshold from startup config file */
        if (aal_scfg_read(CFG_ID_L2TM_NON_CONGESTION_BUFF_THRESHOLD, 4, &non_congestion_threshold) != CA_E_OK)
        {
            ca_printf("QUEUE : failed to read CFG_ID_L2TM_NON_CONGESTION_BUFF_THRESHOLD, using 0x4000 as default value.\n");
            non_congestion_threshold = CA_AAL_L2_QM_NON_CONGESTION_THRESHOLD;  /* using default value */
        }

        buff_cfg.non_cong_threshold = non_congestion_threshold;
        (void)aal_l2_qm_buff_cfg_set(device_id, buff_msk, &buff_cfg);
    }
    else
    {
        glb_cfg.tail_drop_ena = FALSE;  /* turn off tail drop */

        buff_cfg.non_cong_threshold = CA_AAL_L2_QM_NON_CONGESTION_THRESHOLD;   /* use 0x4000 default value if TD is disabled, WRED will manage all of buffers */
        (void)aal_l2_qm_buff_cfg_set(device_id, buff_msk, &buff_cfg);
    }

    (void)aal_l2_te_glb_cfg_set(device_id, glb_msk, &glb_cfg);

    return CA_E_OK;
}

#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
static ca_status_t _queue_l3_wred_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t            port_id,
    CA_IN ca_uint32_t             queue,
    CA_IN ca_queue_wred_profile_t *profile
)
{
    aal_l3_te_wred_profile_t         wred_profile;
    aal_l3_te_wred_profile_msk_t     wred_profile_msk;
    aal_l3_te_wred_profile_sel_t     sel;
    aal_l3_te_wred_profile_sel_msk_t sel_msk;
    ca_uint32_t prev_index = 0;
    ca_uint32_t index = 0;
    ca_uint32_t ref_counter = 0;
    ca_uint32_t i = 0;
    ca_uint32_t k = 0;
    aal_l3_te_glb_cfg_t glb_cfg;
    aal_l3_te_glb_cfg_msk_t glb_msk;
    aal_l3_te_cb_ctrl_t cb_cfg;
    aal_l3_te_cb_ctrl_mask_t cb_msk;
    ca_int8_t sz_mark_info[256];
    ca_int8_t sz_unmark_info[256];
    ca_int8_t sz_mark_dp[16];
    ca_int8_t sz_unmark_dp[16];
    ca_uint32_t zero_dp[CA_AAL_WRED_MAX_DROP_PROBABILTY];

    if (!profile)
        return CA_E_PARAM;

    memset((void *)&zero_dp[0], 0, sizeof(zero_dp));

    memset((void *)&sz_mark_info[0], 0, sizeof(sz_mark_info));
    memset((void *)&sz_unmark_info[0], 0, sizeof(sz_unmark_info));


    sprintf(sz_mark_info,   "%s", "marked_dp   : ");
    sprintf(sz_unmark_info, "%s", "unmarked_dp : ");

    /* check and convert DP point to the value that G3 ASCI can recognize */
    for (i=0; i<CA_QUEUE_WRED_POINT_COUNT; i++)
    {
        if (profile->marked_dp[i] > 100 || profile->unmarked_dp[i] > 100)
            return CA_E_PARAM;

        sprintf(sz_mark_dp,   "%02x ", profile->marked_dp[i]);
        strcat(sz_mark_info, sz_mark_dp);
        sprintf(sz_unmark_dp, "%02x ", profile->unmarked_dp[i]);
        strcat(sz_unmark_info, sz_unmark_dp);
    }

    printk("%s\n", sz_mark_info);
    printk("%s\n", sz_unmark_info);

    if (aal_l3_te_wred_info_lookup(profile->marked_dp, profile->unmarked_dp, &index))
    {
        /* a profile entry is checked out, which can be reused by new port & queue */

        __QUEUE_LOG("WRED entry lookup ok - index %d can be resued\n", index);

        prev_index = aal_l3_te_wred_sel_info_get(port_id, queue);
        if (index == prev_index)
        {
            /*
            ** this case means same configurations are confgured to one Queue twice, so we do nothing.
            ** for example,
            ** configure q0 with unmarked DP 50, 50, .... 50
            ** and then configure q0 with unmarked DP 50, 50, ... 50 again.
            ** this branch will be asserted.
            */
            return CA_E_OK;
        }

        /* operate hardware port & queue based profile selection table */
        sel.profile_id[queue] = index;
        sel_msk.u32 = (1 << queue);

        /* modify profile selection for UC */
        if (aal_l3_te_wred_profile_sel_set(device_id, port_id, sel_msk, &sel) != CA_E_OK)
        {
            return CA_E_ERROR;
        }

        /* modify profile selection for ~UC */
        if (aal_l3_te_wred_profile_sel_set(device_id, port_id + L3_WRED_SELECT_UC_OFFSEST, sel_msk, &sel) != CA_E_OK)
        {
            return CA_E_ERROR;
        }

        /* update software selection table */
        prev_index = aal_l3_te_wred_sel_info_get(port_id, queue);
#ifdef RTK_MEM_CORRUPTTION_FIX
        if (prev_index == 0xff){
                printk("aal_l3_te_wred_sel_info_get return 0xff\n");
                return CA_E_ERROR;
        }
#endif
        __QUEUE_LOG("prev index : %d, ref_counter : %d\n", prev_index, aal_l3_te_wred_profile_info_ref_counter_get(prev_index));

        /* WRED info has been configured before, this is just a modification operation */
        aal_l3_te_wred_profile_info_ref_dec(prev_index);

        if (aal_l3_te_wred_profile_info_ref_counter_get(prev_index) == 0)
        {
            /* the prev_index profile is free, just update hardware table accordingly */
            memset(&wred_profile, 0x0, sizeof(wred_profile));
            memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

            wred_profile_msk.s.mark_idx    = 1;
            wred_profile_msk.s.unmark_idx  = 1;
            wred_profile_msk.s.mark_dp     = 1;
            wred_profile_msk.s.unmark_dp   = 1;

            /* clear hardware WRED profile table */
            (void)aal_l3_te_wred_profile_set(device_id, prev_index, wred_profile_msk, &wred_profile);

            /* clear software WRED profile table */
            (void)aal_l3_te_wred_profile_info_set(prev_index, zero_dp, zero_dp);
        }

        aal_l3_te_wred_sel_info_set(port_id, queue, index);

        /* update software profile table */
        aal_l3_te_wred_profile_info_ref_inc(index); /* reference counter ++ */

    }
    else
    {
        /* no profile entry can be reused */
        prev_index  = aal_l3_te_wred_sel_info_get(port_id, queue);
        __QUEUE_LOG("no profile entry can be reused\n");
        __QUEUE_LOG("prev_index : %d\n", prev_index);
#ifdef RTK_MEM_CORRUPTTION_FIX
        if (prev_index == 0xff){
                printk("aal_l3_te_wred_sel_info_get return 0xff\n");
                return CA_E_ERROR;
        }
#endif
        ref_counter = aal_l3_te_wred_profile_info_ref_counter_get(prev_index);
        __QUEUE_LOG("ref counter : %d", ref_counter);
        if (ref_counter == 1)
        {
            __QUEUE_LOG("directly change profile\n");

            /* this profile has been configured before and reference counter is 1,
            ** just directly change profile because it is only used by this port and queue
            */
            memset(&wred_profile, 0x0, sizeof(wred_profile));
            memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

            wred_profile_msk.s.mark_idx    = 1;
            wred_profile_msk.s.unmark_idx  = 1;
            wred_profile_msk.s.mark_dp     = 1;
            wred_profile_msk.s.unmark_dp   = 1;

            wred_profile.mark_idx     = l3_te_wred_mark_idx;    /* 0 ~ 8191 buffers                       */
            wred_profile.unmark_idx   = l3_te_wred_unmark_idx;    /* 0 ~ 8191 buffers                       */

            /* convert DP value from 100 based to 63 based */
            for (k=0; k<CA_AAL_L3_WRED_MAX_DROP_PROBABILTY; k++)
            {
                wred_profile.mark_dp[k]   = (ca_uint32_t)(profile->marked_dp[k] * 100 / 158);
                wred_profile.unmark_dp[k] = (ca_uint32_t)(profile->unmarked_dp[k] * 100 / 158);
            }

            /* operate hardware WRED profile table */
            if (aal_l3_te_wred_profile_set(device_id, prev_index, wred_profile_msk, &wred_profile) != CA_E_OK)
            {
                return CA_E_ERROR;
            }

            /* modify software profile table */
            aal_l3_te_wred_profile_info_set(prev_index, profile->marked_dp, profile->unmarked_dp);

        }
        else
        {
            /* reference counter must be larger than 1 in this case */
            if (aal_l3_te_wred_profile_info_allocate(&index))       /* allocate new one */
            {
                __QUEUE_LOG("allocate new entry index %d\n", index);

                memset(&wred_profile, 0x0, sizeof(wred_profile));
                memset(&wred_profile_msk, 0x0, sizeof(wred_profile_msk));

                wred_profile_msk.s.mark_idx    = 1;
                wred_profile_msk.s.unmark_idx  = 1;
                wred_profile_msk.s.mark_dp     = 1;
                wred_profile_msk.s.unmark_dp   = 1;

                wred_profile.mark_idx     = l3_te_wred_mark_idx;    /* 0 ~ 4095 buffers                       */
                wred_profile.unmark_idx   = l3_te_wred_unmark_idx;    /* 0 ~ 4095 buffers                       */

                /* convert DP value from 100 based to 63 based */
                for (k=0; k<CA_AAL_L3_WRED_MAX_DROP_PROBABILTY; k++)
                {
                    wred_profile.mark_dp[k]   = (ca_uint32_t)(profile->marked_dp[k] * 100 / 158);
                    wred_profile.unmark_dp[k] = (ca_uint32_t)(profile->unmarked_dp[k] * 100 / 158);
                }

                /* operate hardware WRED profile table */
                if (aal_l3_te_wred_profile_set(device_id, index, wred_profile_msk, &wred_profile) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* operate hardware port & queue based profile selection table */
                memset(&sel, 0x0, sizeof(sel));
                memset(&sel_msk, 0x0, sizeof(sel_msk));

                sel.profile_id[queue] = index;
                sel_msk.u32 = (1 << queue);

                /* configure profile selection for UC */
                if (aal_l3_te_wred_profile_sel_set(device_id, port_id, sel_msk, &sel) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* configure profile selection for ~UC */
                if (aal_l3_te_wred_profile_sel_set(device_id, port_id + L3_WRED_SELECT_UC_OFFSEST, sel_msk, &sel) != CA_E_OK)
                {
                    return CA_E_ERROR;
                }

                /* update software profile table */
                aal_l3_te_wred_profile_info_set(index, profile->marked_dp, profile->unmarked_dp);
                aal_l3_te_wred_profile_info_ref_inc(index); /* reference counter ++ */
                __QUEUE_LOG("index %d ref counter : %d\n", index, aal_l3_te_wred_profile_info_ref_counter_get(index));

                /* update software selection table */
                aal_l3_te_wred_sel_info_set(port_id, queue, index);

                /* decrease previous profile reference counter */
                aal_l3_te_wred_profile_info_ref_dec(prev_index);
                __QUEUE_LOG("prev index %d ref counter : %d\n", prev_index, aal_l3_te_wred_profile_info_ref_counter_get(prev_index));
            }
            else
            {
                return CA_E_RESOURCE;
            }
        }
    }

    /* L3 TE tail drop control */
    glb_msk.u32 = 0;
    memset((void *)&glb_cfg, 0, sizeof(glb_cfg));

    glb_msk.s.tail_drop_ena = 1;

    /* L3 TE WRED scan control */
    cb_msk.u32 = 0;
    memset((void *)&cb_cfg, 0, sizeof(cb_cfg));

    cb_msk.s.scan_ena = 1;

    if (aal_l3_te_wred_info_empty())
    {
        glb_cfg.tail_drop_ena = TRUE;   /* turn on tail drop in case system does not have WRED profile configured */
        cb_cfg.scan_ena       = FALSE;  /* turn off WRED */
    }
    else
    {
        glb_cfg.tail_drop_ena = FALSE;  /* turn off tail drop */
        cb_cfg.scan_ena       = TRUE;   /* turn on WRED       */
    }

    (void)aal_l3_te_glb_cfg_set(device_id, glb_msk, &glb_cfg);
    (void)aal_l3_te_cb_ctrl_set(device_id, cb_msk, &cb_cfg);

    return CA_E_OK;
}
#endif

ca_status_t ca_queue_wred_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t            port_id,
    CA_IN ca_uint32_t             queue,
    CA_IN ca_queue_wred_profile_t *profile
)
{
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;

    if (!profile)
        return CA_E_PARAM;

    if (queue > 7)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON || port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON || port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;    /* total 128 entries for L3 WRED */
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        #if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (port >= CA_PORT_ID_CPU0 && port <= CA_PORT_ID_CPU7)
                port -= CA_PORT_ID_CPU0;
            else if (port >= CA_PORT_ID_CPU_MQ_MIN && port <= CA_PORT_ID_CPU_MQ_MAX)
                ;
            else
                return CA_E_PARAM;
        #else
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;

            port -= CA_PORT_ID_CPU0;
        #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;    /* total 128 entries for L3 WRED */
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
    #if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (port >= CA_PORT_ID_CPU0 && port <= CA_PORT_ID_CPU7)
                port -= CA_PORT_ID_CPU0;
            else if (port >= CA_PORT_ID_CPU_MQ_MIN && port <= CA_PORT_ID_CPU_MQ_MAX)
                ;
            else
                return CA_E_PARAM;
    #else
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;

            port -= CA_PORT_ID_CPU0;
    #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    _queue_l2_wred_set(device_id, port, queue, profile);
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_CPU)
    {
        _queue_l3_wred_set(device_id, port, queue, profile);
    }
    else if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        _queue_l3_wred_set(device_id, port, queue, profile);
        _queue_l2_wred_set(device_id, port - CA_AAL_L3_TE_NI_PORT_OFFSET, queue, profile);
    }
    else
    {
        _queue_l2_wred_set(device_id, port, queue, profile);
    }
#else
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON || port_type == CA_PORT_TYPE_L3 )
    {
        _queue_l3_wred_set(device_id, port, queue, profile);
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        _queue_l3_wred_set(device_id, port, queue, profile);
        // _queue_l2_wred_set(device_id, CA_AAL_TM_SPID_CPU, queue, profile);
    }
    else
    {
        _queue_l2_wred_set(device_id, port, queue, profile);
    }
#endif

    return CA_E_OK;
}

ca_status_t ca_queue_wred_get
(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t            port_id,
    CA_IN  ca_uint32_t             queue,
    CA_OUT ca_queue_wred_profile_t *profile
)
{
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;
    ca_uint32_t index = 0;

    if (!profile)
        return CA_E_PARAM;

    if (queue > 7)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON || port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON || port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;    /* total 128 entries for L3 WRED */
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        #if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (port >= CA_PORT_ID_CPU0 && port <= CA_PORT_ID_CPU7)
                port -= CA_PORT_ID_CPU0;
            else if (port >= CA_PORT_ID_CPU_MQ_MIN && port <= CA_PORT_ID_CPU_MQ_MAX)
                ;
            else
                return CA_E_PARAM;
        #else
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;

            port -= CA_PORT_ID_CPU0;
        #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;    /* total 128 entries for L3 WRED */
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
    #if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (port >= CA_PORT_ID_CPU0 && port <= CA_PORT_ID_CPU7)
            port -= CA_PORT_ID_CPU0;
        else if (port >= CA_PORT_ID_CPU_MQ_MIN && port <= CA_PORT_ID_CPU_MQ_MAX)
            ;
        else
            return CA_E_PARAM;
    #else
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;

        port -= CA_PORT_ID_CPU0;
    #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#endif


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    index = aal_l2_te_wred_sel_info_get(port, queue);
    if (aal_l2_te_wred_profile_info_get(index, profile->marked_dp, profile->unmarked_dp) != CA_E_OK)
        return CA_E_ERROR;
#else
    if (port_type == CA_PORT_TYPE_EPON ||
        port_type == CA_PORT_TYPE_GPON ||
        port_type == CA_PORT_TYPE_L3 ||
        port_type == CA_PORT_TYPE_CPU)  /* WAN */
    {
        index = aal_l3_te_wred_sel_info_get(port, queue);
        if (aal_l3_te_wred_profile_info_get(index, profile->marked_dp, profile->unmarked_dp) != CA_E_OK)
            return CA_E_ERROR;
    }
    else
    {
        index = aal_l2_te_wred_sel_info_get(port, queue);
        if (aal_l2_te_wred_profile_info_get(index, profile->marked_dp, profile->unmarked_dp) != CA_E_OK)
            return CA_E_ERROR;
    }
#endif

    return CA_E_OK;
}

ca_status_t ca_queue_wred_ecn_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t   ecn_support
)
{
    aal_l2_te_glb_cfg_t     l2_te_cfg;
    aal_l2_te_glb_cfg_msk_t l2_te_msk;
    aal_l3_te_glb_cfg_t     l3_te_cfg;
    aal_l3_te_glb_cfg_msk_t l3_te_msk;
    aal_arb_ctrl_t arb_ctrl;
    aal_arb_ctrl_mask_t arb_msk;

    memset((void *)&l2_te_cfg, 0, sizeof(l2_te_cfg));
    memset((void *)&l3_te_cfg, 0, sizeof(l3_te_cfg));

    l2_te_msk.u32 = 0;
    l3_te_msk.u32 = 0;

    l2_te_cfg.wred_ecn_ena = ecn_support ? 1 : 0;
    l2_te_msk.s.wred_ecn_ena = 1;
    (void)aal_l2_te_glb_cfg_set(device_id, l2_te_msk, &l2_te_cfg);

    l3_te_cfg.wred_ecn_ena = ecn_support ? 1 : 0;
    l3_te_msk.s.wred_ecn_ena = 1;
    (void)aal_l3_te_glb_cfg_set(device_id, l3_te_msk, &l3_te_cfg);

    memset((void *)&arb_ctrl, 0, sizeof(arb_ctrl));
    arb_msk.u32 = 0;

    arb_msk.s.ip_ecn_en = 1;
    arb_ctrl.ip_ecn_en  = ecn_support ? 1 : 0;

    (void)aal_arb_ctrl_set(device_id, arb_msk, &arb_ctrl);

    return CA_E_OK;
}

ca_status_t ca_queue_wred_ecn_get
(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_boolean_t   *ecn_support
)
{
    aal_l2_te_glb_cfg_t l2_te_cfg;

    if (!ecn_support)
        return CA_E_PARAM;

    memset((void *)&l2_te_cfg, 0, sizeof(l2_te_cfg));

    if (aal_l2_te_glb_cfg_get(device_id, &l2_te_cfg) == CA_E_OK)
    {
        *ecn_support = l2_te_cfg.wred_ecn_ena;

        return CA_E_OK;
    }
    else
    {
        return CA_E_ERROR;
    }
}


ca_status_t ca_queue_stats_get
(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t     port_id,
    CA_IN  ca_uint32_t      queue,
    CA_IN  ca_boolean_t     read_clear,
    CA_OUT ca_queue_stats_t *stats
)
{
    aal_l2_te_pm_egress_t pm_stats;
    aal_l3_te_pm_egress_t l3_pm_stats;
    ca_uint32_t voq_id = 0;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;
    ca_queue_stats_t tmp_stats;

    read_clear = read_clear; /* avoid PC-lint warning */

    if (queue > 7)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0  || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
            if (port != CA_PORT_ID_NI0)
                return CA_E_PARAM;
        #else
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        #endif

    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0  || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

    memset((void *)&pm_stats, 0, sizeof(pm_stats));
    memset((void *)&l3_pm_stats, 0, sizeof(l3_pm_stats));
    memset((void *)&tmp_stats, 0, sizeof(tmp_stats));

    if (port_type == CA_PORT_TYPE_CPU)
    {
        port -= CA_PORT_ID_CPU0;            /* convert to 0 ~ 7 */

        voq_id = port * 8 + queue;          /* CPU 0 ~ 7, NI 8 ~ 15 */

        if (aal_l3_te_pm_egress_voq_get(device_id, voq_id, &l3_pm_stats) != CA_E_OK)
            return CA_E_ERROR;

        //stats->all_dropped      = l3_pm_stats.td_drop_pkt + l3_pm_stats.wred_mark_pkt + l3_pm_stats.wred_unmark_pkt;
        stats->tail_dropped           = l3_pm_stats.td_drop_pkt;
        stats->marked_dropped         = l3_pm_stats.wred_mark_pkt;
        stats->unmarked_dropped       = l3_pm_stats.wred_unmark_pkt;
        stats->forwarded             = l3_pm_stats.total_pkt - (l3_pm_stats.td_drop_pkt + l3_pm_stats.wred_mark_pkt + l3_pm_stats.wred_unmark_pkt);
        stats->tail_dropped_bytes     = l3_pm_stats.td_drop_bytes;
        stats->marked_dropped_bytes   = l3_pm_stats.wred_mark_bytes;
        stats->unmarked_dropped_bytes = l3_pm_stats.wred_unmark_bytes;
        stats->forwarded_bytes       = l3_pm_stats.total_bytes - (l3_pm_stats.td_drop_bytes + l3_pm_stats.wred_mark_bytes + l3_pm_stats.wred_unmark_bytes);

        /*
        ** fix bug 49806, only read L3 PM for both L2 & L3 traffic
        ** 1st path : L2FE, L2TE, L2TM, L3TM, L3QM to CPU
        ** 2nd path : L2FE, L2TE, L2TM, L3FE, L3TM, L3QM to CPU
        ** forwarded : only L3 PM
        ** dropped   : L2PM + L3PM
        */
        #if 1
        /* fix bug 49003 */
        voq_id = CA_AAL_TM_SPID_CPU * 8 + queue;

        if (aal_l2_te_pm_egress_voq_get(device_id, voq_id, &pm_stats) != CA_E_OK)
            return CA_E_ERROR;

        //tmp_stats.all_dropped      = pm_stats.td_drop_pkt + pm_stats.wred_mark_pkt + pm_stats.wred_unmark_pkt;
        tmp_stats.tail_dropped           = pm_stats.td_drop_pkt;
        tmp_stats.marked_dropped         = pm_stats.wred_mark_pkt;
        tmp_stats.unmarked_dropped       = pm_stats.wred_unmark_pkt;
        tmp_stats.forwarded             = 0; // pm_stats.total_pkt - tmp_stats.all_dropped;
        tmp_stats.tail_dropped_bytes     = pm_stats.td_drop_bytes;
        tmp_stats.marked_dropped_bytes   = pm_stats.wred_mark_bytes;
        tmp_stats.unmarked_dropped_bytes = pm_stats.wred_unmark_bytes;
        tmp_stats.forwarded_bytes       = 0;


        //stats->all_dropped      += tmp_stats.all_dropped;
        stats->tail_dropped           += tmp_stats.tail_dropped;
        stats->marked_dropped         += tmp_stats.marked_dropped;
        stats->unmarked_dropped       += tmp_stats.unmarked_dropped;
        stats->forwarded             += tmp_stats.forwarded;
        stats->tail_dropped_bytes     += tmp_stats.tail_dropped_bytes;
        stats->marked_dropped_bytes   += tmp_stats.marked_dropped_bytes;
        stats->unmarked_dropped_bytes += tmp_stats.unmarked_dropped_bytes;
        stats->forwarded_bytes       += tmp_stats.forwarded_bytes;

        #endif
    }
    else if (port_type == CA_PORT_TYPE_L3)   /* WAN port */
    {
        voq_id = (port + CA_AAL_L3_TE_NI_PORT_OFFSET) * 8 + queue; /* CPU 0 ~ 7, NI 8 ~ 15 */
        if (aal_l3_te_pm_egress_voq_get(device_id, voq_id, &l3_pm_stats) != CA_E_OK)
            return CA_E_ERROR;

        //stats->all_dropped      = l3_pm_stats.td_drop_pkt + l3_pm_stats.wred_mark_pkt + l3_pm_stats.wred_unmark_pkt;
        stats->tail_dropped     = l3_pm_stats.td_drop_pkt;
        stats->marked_dropped   = l3_pm_stats.wred_mark_pkt;
        stats->unmarked_dropped = l3_pm_stats.wred_unmark_pkt;
        stats->forwarded       = l3_pm_stats.total_pkt - (l3_pm_stats.td_drop_pkt + l3_pm_stats.wred_mark_pkt + l3_pm_stats.wred_unmark_pkt);
        stats->tail_dropped_bytes     = l3_pm_stats.td_drop_bytes;
        stats->marked_dropped_bytes   = l3_pm_stats.wred_mark_bytes;
        stats->unmarked_dropped_bytes = l3_pm_stats.wred_unmark_bytes;
        stats->forwarded_bytes       = l3_pm_stats.total_bytes - (l3_pm_stats.td_drop_bytes + l3_pm_stats.wred_mark_bytes + l3_pm_stats.wred_unmark_bytes);
    }
    else
    {
        voq_id = port * 8 + queue;

        if (aal_l2_te_pm_egress_voq_get(device_id, voq_id, &pm_stats) != CA_E_OK)
            return CA_E_ERROR;

        //stats->all_dropped      = pm_stats.td_drop_pkt + pm_stats.wred_mark_pkt + pm_stats.wred_unmark_pkt;
        stats->tail_dropped           = pm_stats.td_drop_pkt;
        stats->marked_dropped         = pm_stats.wred_mark_pkt;
        stats->unmarked_dropped       = pm_stats.wred_unmark_pkt;
        stats->forwarded             = pm_stats.total_pkt - (pm_stats.td_drop_pkt + pm_stats.wred_mark_pkt + pm_stats.wred_unmark_pkt);
        stats->tail_dropped_bytes     = pm_stats.td_drop_bytes;
        stats->marked_dropped_bytes   = pm_stats.wred_mark_bytes;
        stats->unmarked_dropped_bytes = pm_stats.wred_unmark_bytes;
        stats->forwarded_bytes       = pm_stats.total_bytes - (pm_stats.td_drop_bytes + pm_stats.wred_mark_bytes + pm_stats.wred_unmark_bytes);
    }

    return CA_E_OK;
}

ca_status_t ca_queue_stats_clear
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue
)
{
    return CA_E_NOT_SUPPORT;
}

ca_status_t ca_queue_flush
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  queue
)
{
    aal_l2_tm_bm_cfg_t     bm_cfg;
    aal_l2_tm_bm_cfg_msk_t bm_msk;
    ca_uint32_t voq_id = 0;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t port = 0;
    ca_status_t rtn = CA_E_ERROR;

    if (queue >= CA_QUEUE_COUNT)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
     #endif

        port += CA_AAL_L3_TE_NI_PORT_OFFSET;
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

    if (port_type == CA_PORT_TYPE_ETHERNET)  /* LAN */
    {
        /* drop all traffic destinated to the port and voq*/
        memset((void *)&bm_cfg, 0, sizeof(bm_cfg));
        memset((void *)&bm_msk, 0, sizeof(bm_msk));

        bm_msk.s.rx_drop_ena  = 1;
        bm_msk.s.rx_drop_port = 1;
        bm_msk.s.rx_drop_cos  = 1;

        bm_cfg.rx_drop_ena  = TRUE;        /* full match for PID + VoQ */
        bm_cfg.rx_drop_port = port;
        bm_cfg.rx_drop_cos  = voq_id;

        (void)aal_l2_tm_bm_cfg_set(device_id, bm_msk, &bm_cfg);

        rtn = aal_l2_tm_es_voq_flush_set(device_id, port, queue);

        /* permit all ingress traffic for the VoQ */
        bm_cfg.rx_drop_ena  = FALSE;
        (void)aal_l2_tm_bm_cfg_set(device_id, bm_msk, &bm_cfg);
    }
    else if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        #if defined(CONFIG_ARCH_CORTINA_G3HGU)
            // port = CA_AAL_L2TM_DPID_L3_WAN;    /* port remapping for G3HGU, TM_DPID is 0xa for WAN */
            port = port;
        #else
            port = port; /* port no is unchagned for SATURN which doesn't have L3TM engine */
        #endif

        /* drop all traffic destinated to the port and voq*/
        memset((void *)&bm_cfg, 0, sizeof(bm_cfg));
        memset((void *)&bm_msk, 0, sizeof(bm_msk));

        bm_msk.s.rx_drop_ena  = 1;
        bm_msk.s.rx_drop_port = 1;
        bm_msk.s.rx_drop_cos  = 1;

        bm_cfg.rx_drop_ena  = TRUE;        /* full match for PID + VoQ */
        bm_cfg.rx_drop_port = port;
        bm_cfg.rx_drop_cos  = voq_id;

        (void)aal_l2_tm_bm_cfg_set(device_id, bm_msk, &bm_cfg);

        rtn = aal_l2_tm_es_voq_flush_set(device_id, port, queue);

        /* permit all ingress traffic for the VoQ */
        bm_cfg.rx_drop_ena  = FALSE;
        (void)aal_l2_tm_bm_cfg_set(device_id, bm_msk, &bm_cfg);

    }
    else
    {
        (void)aal_l3_tm_voq_drop_ena_set(device_id, port, voq_id, TRUE); /* drop all ingress traffic for the VoQ at first */

        rtn =  aal_l3_tm_es_voq_flush_set(device_id, port, queue);

        (void)aal_l3_tm_voq_drop_ena_set(device_id, port, voq_id, FALSE); /* permit all ingress traffic for the VoQ then */
    }

    return rtn;
}

ca_status_t ca_pie_config_set
(
    ca_device_id_t                device_id,
    ca_pie_global_configuration_t *config
)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    aal_l3_te_pie_dp_scaling_msk_t scaling_msk;
    aal_l3_te_pie_dp_scaling_t     scaling_cfg;
    aal_l3_te_pie_dp_glb_msk_t     dp_glb_msk;
    aal_l3_te_pie_dp_glb_cfg_t     dp_glb_cfg;
    aal_l3_te_pie_timer_msk_t      timer_msk;
    aal_l3_te_pie_timer_cfg_t      timer_cfg;

    if (!config)
        return CA_E_PARAM;

    if (config->mask.qfull_onethird)
    {
        if (config->qfull_onethird > 0x1fffff)  /* up to 2MB size */
            return CA_E_PARAM;

        if (aal_l3_te_pie_buff_threshold_set(device_id, config->qfull_onethird) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.dp_weight_A)
    {
        scaling_msk.u32 = 0;
        memset((void*)&scaling_cfg, 0, sizeof(scaling_cfg));

        scaling_msk.s.value_a = 1;
        scaling_cfg.value_a   = config->dp_weight_A;

        if (aal_l3_te_pie_dp_scaling_set(device_id, scaling_msk, &scaling_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.dp_weight_B)
    {
        scaling_msk.u32 = 0;
        memset((void*)&scaling_cfg, 0, sizeof(scaling_cfg));

        scaling_msk.s.value_b = 1;
        scaling_cfg.value_b   = config->dp_weight_B;

        if (aal_l3_te_pie_dp_scaling_set(device_id, scaling_msk, &scaling_cfg) != CA_E_OK)
            return CA_E_ERROR;

    }

    if (config->mask.dp_ceiling)
    {
        if (aal_l3_te_pie_dp_ceiling_set(device_id, config->dp_ceiling) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.dp_high_threshold)
    {
        dp_glb_msk.u32 = 0;
        memset((void *)&dp_glb_cfg, 0, sizeof(dp_glb_cfg));

        dp_glb_msk.s.high = 1;
        dp_glb_cfg.high = config->dp_high_threshold;

        if (aal_l3_te_pie_dp_glb_cfg_set(device_id, dp_glb_msk, &dp_glb_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.dp_low_threshold)
    {
        dp_glb_msk.u32 = 0;
        memset((void *)&dp_glb_cfg, 0, sizeof(dp_glb_cfg));

        dp_glb_msk.s.low = 1;
        dp_glb_cfg.low = config->dp_low_threshold;

        if (aal_l3_te_pie_dp_glb_cfg_set(device_id, dp_glb_msk, &dp_glb_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.max_burst)
    {
        dp_glb_msk.u32 = 0;
        memset((void *)&dp_glb_cfg, 0, sizeof(dp_glb_cfg));

        dp_glb_msk.s.burst = 1;
        dp_glb_cfg.burst = config->max_burst;

        if (aal_l3_te_pie_dp_glb_cfg_set(device_id, dp_glb_msk, &dp_glb_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.mean_pkt_size)
    {
        if (aal_l3_te_pie_mean_pkt_set(device_id, config->mean_pkt_size) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.voq_high_water_mark)
    {
        timer_msk.u32 = 0;
        memset((void *)&timer_cfg, 0, sizeof(timer_cfg));

        timer_msk.s.scalar = 1;
        timer_cfg.scalar   = config->voq_high_water_mark;

        if (aal_l3_te_pie_timer_set(device_id, timer_msk, &timer_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.interval)
    {
        timer_msk.u32 = 0;
        memset((void *)&timer_cfg, 0, sizeof(timer_cfg));

        timer_msk.s.interval = 1;
        timer_cfg.interval   = config->interval;

        if (aal_l3_te_pie_timer_set(device_id, timer_msk, &timer_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }

    if (config->mask.burst_decrement)
    {
        timer_msk.u32 = 0;
        memset((void *)&timer_cfg, 0, sizeof(timer_cfg));

        timer_msk.s.burst_dec = 1;
        timer_cfg.burst_dec   = config->burst_decrement;

        if (aal_l3_te_pie_timer_set(device_id, timer_msk, &timer_cfg) != CA_E_OK)
            return CA_E_ERROR;
    }
#endif

    return CA_E_OK;
}

ca_status_t ca_pie_config_get
(
    ca_device_id_t                device_id,
    ca_pie_global_configuration_t *config
)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    aal_l3_te_pie_dp_scaling_t     scaling_cfg;
    aal_l3_te_pie_dp_glb_cfg_t     dp_glb_cfg;
    aal_l3_te_pie_timer_cfg_t      timer_cfg;
    ca_uint32_t one_third = 0;
    ca_uint32_t ceiling   = 0;
    ca_uint32_t mean_pkt_size = 0;

    if (!config)
        return CA_E_OK;

    if (aal_l3_te_pie_buff_threshold_get(device_id, &one_third) != CA_E_OK)
        return CA_E_ERROR;
    config->qfull_onethird = one_third;

    if (aal_l3_te_pie_dp_ceiling_get(device_id, &ceiling) != CA_E_OK)
        return CA_E_ERROR;
    config->dp_ceiling = ceiling;

    if (aal_l3_te_pie_mean_pkt_get(device_id, &mean_pkt_size) != CA_E_OK)
        return CA_E_ERROR;
    config->mean_pkt_size = mean_pkt_size;

    if (aal_l3_te_pie_dp_scaling_get(device_id, &scaling_cfg) != CA_E_OK)
        return CA_E_ERROR;
    config->dp_weight_A = scaling_cfg.value_a;
    config->dp_weight_B = scaling_cfg.value_b;

    if (aal_l3_te_pie_dp_glb_cfg_get(device_id, &dp_glb_cfg) != CA_E_OK)
        return CA_E_ERROR;
    config->dp_high_threshold = dp_glb_cfg.high;
    config->dp_low_threshold  = dp_glb_cfg.low;
    config->max_burst         = dp_glb_cfg.burst;

    if (aal_l3_te_pie_timer_get(device_id, &timer_cfg) != CA_E_OK)
        return CA_E_ERROR;
    config->voq_high_water_mark = timer_cfg.scalar;
    config->interval            = timer_cfg.interval;
    config->burst_decrement     = timer_cfg.burst_dec;
#endif

    return CA_E_OK;
}

ca_status_t ca_pie_queue_set
(
    ca_device_id_t             device_id,
    ca_pie_voq_control_block_t *config
)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    aal_l3_te_pie_dp_msk_t dp_msk;
    aal_l3_te_pie_dp_cfg_t dp_cfg;
    ca_uint32_t pie_id = 0;

    if (!config)
        return CA_E_PARAM;

    pie_id = config->pie_voq_idx;

    dp_msk.u32 = 0;
    memset((void *)&dp_cfg, 0, sizeof(dp_cfg));

    if (config->mask.latency_high)
    {
        dp_cfg.latency_threshold_hi = config->latency_high;
    }

    if (config->mask.latency_low)
    {
        dp_cfg.latency_threshold_lo = config->latency_low;
    }

    if (config->mask.latency_target)
    {
        dp_cfg.latency_target= config->latency_target;
    }

    if (config->mask.burst_reset_timeout)
    {
        dp_cfg.burst_reset_timeout= config->burst_reset_timeout;
    }

    if (config->mask.peak_rate)
    {
        dp_cfg.peak_rate= config->peak_rate;
    }

    if (config->mask.msr)
    {
        dp_cfg.max_service_rate = config->msr;
    }

    if (aal_l3_te_pie_dp_cfg_set(device_id, pie_id, dp_msk, &dp_cfg) != CA_E_OK)
        return CA_E_ERROR;

#endif

    return CA_E_OK;
}

ca_status_t ca_pie_queue_get
(
    ca_device_id_t             device_id,
    ca_pie_voq_control_block_t *config
)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#endif
    return CA_E_OK;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(ca_queue_length_set);
EXPORT_SYMBOL(ca_queue_length_get);
EXPORT_SYMBOL(ca_queue_pause_threshold_set);
EXPORT_SYMBOL(ca_queue_pause_threshold_get);
EXPORT_SYMBOL(ca_queue_schedule_set);
EXPORT_SYMBOL(ca_queue_schedule_get);
EXPORT_SYMBOL(ca_queue_wred_set);
EXPORT_SYMBOL(ca_queue_wred_get);
EXPORT_SYMBOL(ca_queue_wred_ecn_set);
EXPORT_SYMBOL(ca_queue_wred_ecn_get);
EXPORT_SYMBOL(ca_queue_stats_clear);
EXPORT_SYMBOL(ca_queue_stats_get);
EXPORT_SYMBOL(ca_queue_flush);
EXPORT_SYMBOL(ca_pie_config_set);
EXPORT_SYMBOL(ca_pie_config_get);
EXPORT_SYMBOL(ca_pie_queue_set);
EXPORT_SYMBOL(ca_pie_queue_get);

#endif
