#include "ca_types.h"
#include "osal_common.h"
#include "aal_pon.h"
#include <soc/cortina/registers.h>
#include <linux/workqueue.h>
#include "aal_epon_intr.h"
#include "aal_mpcp.h"
#include "aal_epon_crypto.h"
#include "ca_event.h"

/********************************************************************
**            external variables and functions declaration         **
********************************************************************/
extern pon_wq_t epon_1gepn_intr_wq;
extern pon_wq_t epon_xgepn_intr_wq;     /* defined in aal_pon.c */
extern pon_wq_t epon_mpcp_intr_wq;      /* defined in aal_pon.c */

/*******************************************************************/
/*              Local Macros and Variables                         */
/*******************************************************************/

static struct _intr_tbl_s
{
    char        *name;         /* interrupt name */
    ca_uint32_t  ena_reg_addr; /* address of INTR enable register */
    ca_uint32_t  src_reg_addr; /* address of INTR source register */
    ca_uint64_t  count;        /* count of INTR */
}
aal_epon_intr_tbl[AAL_EPON_INTR_INDEX_MAX + 1] =
{
    {"epon",         EPON_EPON_GLB_ONU_EPON_INTERRUPTE,                     EPON_EPON_GLB_ONU_EPON_INTERRUPT,               0},
    {"pcs",          EPON_EPON_GLB_EXTIF_INTERRUPTE,                        EPON_EPON_GLB_EXTIF_INTERRUPT,               0},

    {"xgepn",        EPON_EPON_GLB_XGEPON_INTERRUPTE,                       EPON_EPON_GLB_XGEPON_INTERRUPT,                      0},
    {"xgpcs",        EPON_XGEPN_XGPCS_RX_RXINTE0,                           EPON_XGEPN_XGPCS_RX_RXINT,                      0},

    {"1pps",         EPON_ONU_1PPS_INTERRUPT_SOURCEE,                       EPON_ONU_1PPS_INTERRUPT_SOURCE,                 0},
    {"ptp",          EPON_GLB_PTP_INTERRUPT_ENABLE,                         EPON_GLB_PTP_INTERRUPT_SOURCE,                  0},
//    {"coresec10_rx", EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_ENABLE_0, EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_0, 0},
//    {"coresec10_tx", EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_ENABLE_1, EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_1, 0},
    {"mpcp",         EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE,               EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT,         0},
};

static aal_epon_intr_handler_list_t aal_epon_intr_handler_head[AAL_EPON_INTR_TYPE_MAX + 1];

/* INTR trace flag */
static ca_uint32_t g_epon_intr_trace = (AAL_EPON_INTR_TRACE_ALL & (~AAL_EPON_INTR_TRACE_LOG));
#define __EPON_INTR_LOG(level, args...) do { if (g_epon_intr_trace & level) ca_printf(args); }while(0)


/* interrupts report have 3 layers: GLB -> INTR_INDEX -> INTR_TYPE */

ca_status_t aal_epon_intr_debug_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  level
)
{
    g_epon_intr_trace = level;

    return CA_E_OK;
}

ca_status_t aal_epon_intr_debug_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint32_t  *level
)
{
    AAL_ASSERT_RET(level, CA_E_PARAM);

    *level = g_epon_intr_trace;

    return CA_E_OK;
}

/* handlers for each interrupt type */

/* 1G EPON interrupt handler */
void aal_epon_1gepn_link_change_handler(ca_uint32_t link_status)
{
    ca_event_epon_port_link_t link_evt;

    memset(&link_evt, 0, sizeof(link_evt));
    link_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7);
    link_evt.status = link_status;
    ca_event_send(0, CA_EVENT_EPON_PORT_LINK, (void *)&link_evt, sizeof(ca_event_epon_port_link_t));

    if (link_status)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: 1g link-up\n");
    }
    else
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: 1g link-down\n");
    }
}


/* call the handler of xgepn interrupt type */
void aal_epon_intr_1gepn_handler(void)
{
    EPON_EPON_GLB_EXTIF_INTERRUPT_t intr_src;
    EPON_EPON_GLB_EXTIF_INTERRUPT_t intr_clr;
    EPON_EPON_GLB_EXTIF_INTERRUPTE_t intr_ena;

    /* read interrupt status and admin state */
    aal_epon_intr_status_get(0, AAL_EPON_INTR_INDEX_1GPCS, (ca_uint32_t *)(&intr_src.wrd));
    aal_epon_intr_ena_get(0, AAL_EPON_INTR_INDEX_1GPCS, (ca_uint32_t *)(&intr_ena.wrd));

    /* disable interrupt here. interrupt will be re-enabled after message is processed over. aal_epon_intr_1gepn_msg_proc() */
    aal_epon_intr_ena_set(0, AAL_EPON_INTR_INDEX_1GPCS, 0);

    if (intr_src.bf.pcs_sync_1gi && intr_ena.bf.pcs_sync_1giE)
    {

        intr_clr.wrd = 0;
        intr_clr.bf.pcs_sync_1gi = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_1GPCS, intr_clr.wrd);   /* clear 1g interrupt */

        memset((void *)&epon_1gepn_intr_wq.work_info.data[0], 0, sizeof(epon_1gepn_intr_wq.work_info.data));
        epon_1gepn_intr_wq.work_info.data[0] = intr_src.wrd;    /* pass status to message proc      */
        epon_1gepn_intr_wq.work_info.data[1] = intr_ena.wrd;    /* pass admin state to message proc */

        queue_work(epon_1gepn_intr_wq.pon_wq, &(epon_1gepn_intr_wq.work_info.work)); /* queue the message */
    }

    return;
}

void aal_epon_intr_1gepn_msg_proc(struct work_struct *wq)
{
    EPON_EPON_GLB_EXTIF_INTERRUPT_t intr_src;
    EPON_EPON_GLB_EXTIF_INTERRUPTE_t intr_ena;
    EPON_EPON_GLB_EPON_STATUS_t status;

    pon_work_info_t *act_work = container_of(wq, pon_work_info_t, work);

    intr_src.wrd = act_work->data[0];
    intr_ena.wrd = act_work->data[1];

    __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_LOG, "ca-epon: EPON_EPON_GLB_EXTIF_INTERRUPT(0x%08x), EPON_EPON_GLB_EXTIF_INTERRUPTE(0x%08x)\n", intr_src, intr_ena);

    if(intr_src.bf.pcs_sync_1gi && intr_ena.bf.pcs_sync_1giE)
    {
        status.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_EPON_STATUS);
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_1GEPN_LINK_CHANGE, status.bf.pcs_sync_1g);
    }

    /* re-enable interrupt bit */
    intr_ena.wrd = 0;
    intr_ena.bf.pcs_sync_1giE = 1;
    aal_epon_intr_ena(0, AAL_EPON_INTR_INDEX_1GPCS, intr_ena.wrd);

    return;
}


/* 10G interrupt handler */
void aal_epon_xgepn_link_change_handler(ca_uint32_t link_status)
{
    ca_boolean_t link = FALSE;
    
    ca_event_epon_port_link_t link_evt;
    aal_epon_crypto_cfg_t cfg;

    /* when adjust attenuator, link up and link down intterrupt will be notified at same time */
    aal_epon_mac_xg_link_status_get(0, 0, &link);
    if (link != link_status)
    {
        /* pon link status is not same as indicator from interrupt */
        return;
    }

    /* send link change event to application */
    memset(&link_evt, 0, sizeof(link_evt));
    link_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7);
    link_evt.status = link_status;
    ca_event_send(0, CA_EVENT_EPON_PORT_LINK, (void *)&link_evt, sizeof(ca_event_epon_port_link_t));

    if (link_status)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: xg link-up\n");
    }
    else
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: xg link-down\n");
        
        /* workaround: onu register fail after plug out/in fiber when high rate traffic running if algo_sel!=disable */
        memset(&cfg, 0, sizeof(aal_epon_crypto_cfg_t));
        aal_epon_crypto_xg_cfg_get(0, 0, &cfg);
        if (cfg.rx_mode != AAL_EPON_CRYPTO_XG_MODE_DISABLE)
        {
            /* reset coresec10 rx module in case that encryption mode is enabled */
            AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG, soft_reset, 1);
            ca_mdelay(1);
            AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG, soft_reset, 0);
        }
    }


#if 1//def CONFIG_LUNA_G3_SERIES
    memset(&link_evt, 0, sizeof(ca_event_epon_port_link_t));
    link_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_EPON, 7);
    link_evt.status = link_status;
    ca_event_send(0, CA_EVENT_EPON_PORT_LINK, (void *)&link_evt, sizeof(ca_event_epon_port_link_t));
#endif
}


void aal_epon_xgepn_sync_det_handler(ca_uint32_t sync_status)
{
    if (sync_status)
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: sync-detected\n");
    else
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: sync-loss\n");
}

/* call the handler of xgepn interrupt type */
void aal_epon_intr_xgepn_handler(void)
{
    EPON_EPON_GLB_XGEPON_INTERRUPT_t intr_src;
    EPON_EPON_GLB_XGEPON_INTERRUPT_t intr_clr;
    EPON_EPON_GLB_XGEPON_INTERRUPTE_t intr_ena;

    EPON_XGEPN_XGPCS_RX_RXINT_t   intr_rx_src;
    EPON_XGEPN_XGPCS_RX_RXINT_t   intr_rx_clr;
    EPON_XGEPN_XGPCS_RX_RXINTE0_t intr_rx_ena;

    /* read interrupt status and admin state */
    aal_epon_intr_status_get(0, AAL_EPON_INTR_INDEX_XGEPN, (ca_uint32_t *)(&intr_src.wrd));
    aal_epon_intr_ena_get   (0, AAL_EPON_INTR_INDEX_XGEPN, (ca_uint32_t *)(&intr_ena.wrd));

    /* disable interrupt here. interrupt will be re-enabled after message is processed over. aal_epon_intr_xgepn_msg_proc() */
    aal_epon_intr_ena_set(0, AAL_EPON_INTR_INDEX_XGEPN, 0);

    if(intr_src.bf.xgepn_xgpcs_rx && intr_ena.bf.xgepn_xgpcs_rxE){
        aal_epon_intr_status_get(0, AAL_EPON_INTR_INDEX_XGPCS, (ca_uint32_t *)(&intr_rx_src.wrd));
        aal_epon_intr_ena_get   (0, AAL_EPON_INTR_INDEX_XGPCS, (ca_uint32_t *)(&intr_rx_ena.wrd));

        /* disable interrupt here. interrupt will be re-enabled after message is processed over. aal_epon_intr_xgepn_msg_proc() */
        aal_epon_intr_ena_set(0, AAL_EPON_INTR_INDEX_XGPCS, 0);

        if (intr_rx_src.bf.linkup && intr_rx_ena.bf.linkupE)
        {
            intr_rx_clr.wrd = 0;
            intr_rx_clr.bf.linkup = 1;
            aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_XGPCS, intr_rx_clr.wrd);   /* clear link-up interrupt */
        }

        if (intr_rx_src.bf.linkdown && intr_rx_ena.bf.linkdownE)
        {
            intr_rx_clr.wrd = 0;
            intr_rx_clr.bf.linkdown = 1;
            aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_XGPCS, intr_rx_clr.wrd);   /* clear link-down interrupt */
        }

        if (intr_rx_src.bf.syncdetI && intr_rx_ena.bf.syncdetIE)
        {
            intr_rx_clr.wrd = 0;
            intr_rx_clr.bf.syncdetI = 1;
            aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_XGPCS, intr_rx_clr.wrd);   /* clear sync interrupt */
        }

        intr_clr.wrd = 0;
        intr_clr.bf.xgepn_xgpcs_rx = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_XGEPN, intr_clr.wrd);   /* clear xg interrupt */

        memset((void *)&epon_xgepn_intr_wq.work_info.data[0], 0, sizeof(epon_xgepn_intr_wq.work_info.data));
        epon_xgepn_intr_wq.work_info.data[0] = intr_rx_src.wrd;    /* pass status to message proc      */
        epon_xgepn_intr_wq.work_info.data[1] = intr_rx_ena.wrd;    /* pass admin state to message proc */

        queue_work(epon_xgepn_intr_wq.pon_wq, &(epon_xgepn_intr_wq.work_info.work)); /* queue the message */
    }

    return;
}

void aal_epon_intr_xgepn_msg_proc(struct work_struct *wq)
{
    EPON_XGEPN_XGPCS_RX_RXINT_t   intr_src; /* from message */
    EPON_XGEPN_XGPCS_RX_RXINTE0_t intr_ena; /* from message */
    EPON_XGEPN_XGPCS_RX_RXSTATUS_t status;
    pon_work_info_t *act_work = container_of(wq, pon_work_info_t, work);
    EPON_XGEPN_XGPCS_RX_RXINTE0_t pcsrx_ena;  /* used to re-enable interrupt */
    EPON_EPON_GLB_XGEPON_INTERRUPTE_t xgepn_ena;  /* used to re-enable interrupt */

    intr_src.wrd = act_work->data[0];
    intr_ena.wrd = act_work->data[1];

    __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_LOG, "ca-epon: EPON_XGEPN_XGPCS_RX_RXINT(0x%08x), EPON_XGEPN_XGPCS_RX_RXINTE0(0x%08x)\n", intr_src, intr_ena);

    if (intr_src.bf.linkup && intr_ena.bf.linkupE)
    {
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_XGEPN_LINK_CHANGE, 1);
    }

    if (intr_src.bf.linkdown && intr_ena.bf.linkdownE)
    {
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_XGEPN_LINK_CHANGE, 0);
    }

    if (intr_src.bf.syncdetI && intr_ena.bf.syncdetIE)
    {
        status.wrd = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_RXSTATUS);
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_XGEPN_SYNC_DET, status.bf.syncdetS);
    }

    /* re-enable interrupt bit */
    pcsrx_ena.wrd = 0;
    pcsrx_ena.bf.linkupE   = 1;
    pcsrx_ena.bf.linkdownE = 1;
    //pcsrx_ena.bf.syncdetIE = 1;
    aal_epon_intr_ena(0, AAL_EPON_INTR_INDEX_XGPCS, pcsrx_ena.wrd);

    xgepn_ena.wrd = 0;
    xgepn_ena.bf.xgepn_xgpcs_rxE = 1;
    aal_epon_intr_ena(0, AAL_EPON_INTR_INDEX_XGEPN, xgepn_ena.wrd);

    return;
}

/* call the handler of mpcp interrupt type */
void aal_epon_intr_mpcp_handler(void)
{
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT_t  intr_src;
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT_t  intr_clr;
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE_t intr_ena;
    //ca_uint8 llid_idx = 0;
    //EPON_XGONU_MPCP_MPCP_REGISTER_FRAME_STS_t frame;
    //EPON_XGONU_MPCP_MPCP_GATE_TIMER_STS_t gate;

    (void)aal_epon_intr_status_get(0, AAL_EPON_INTR_INDEX_MPCP, (ca_uint32_t *)(&intr_src.wrd));
    (void)aal_epon_intr_ena_get   (0, AAL_EPON_INTR_INDEX_MPCP, (ca_uint32_t *)(&intr_ena.wrd));

    /* disable interrupt here. interrupt will be re-enabled after message is processed over. aal_epon_intr_mpcp_msg_proc() */
    aal_epon_intr_ena_set(0, AAL_EPON_INTR_INDEX_MPCP, 0);

    memset((void *)&epon_mpcp_intr_wq.work_info.data[0], 0, sizeof(epon_mpcp_intr_wq.work_info.data));
    epon_mpcp_intr_wq.work_info.data[0] = intr_src.wrd;
    epon_mpcp_intr_wq.work_info.data[1] = intr_ena.wrd;

    if (intr_src.bf.reg_status_changei && intr_ena.bf.reg_status_changeiE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.reg_status_changei = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.rx_register_framei && intr_ena.bf.rx_register_frameiE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.rx_register_framei = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);

    }
    if (intr_src.bf.gate_timer_expiri && intr_ena.bf.gate_timer_expiriE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.gate_timer_expiri = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.mpcp_timer_expiri && intr_ena.bf.mpcp_timer_expiriE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.mpcp_timer_expiri = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.ts_drifti && intr_ena.bf.ts_driftiE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.ts_drifti = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

#if 0
    if (intr_src.bf.report_timer_expiri && intr_ena.bf.report_timer_expiriE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.mpcp_timer_expiri = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.reg_req_retryi && intr_ena.bf.reg_req_retryiE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.reg_req_retryi = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.rx_extend_framei && intr_ena.bf.rx_extend_frameiE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.rx_extend_framei = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }

    if (intr_src.bf.dis_pend_tmr_expiri && intr_ena.bf.dis_pend_tmr_expiriE)
    {
        intr_clr.wrd = 0;
        intr_clr.bf.dis_pend_tmr_expiri = 1;
        aal_epon_intr_status_clr(0, AAL_EPON_INTR_INDEX_MPCP, intr_clr.wrd);
    }
#endif

    queue_work(epon_mpcp_intr_wq.pon_wq, &(epon_mpcp_intr_wq.work_info.work)); /* queue the message */

    return;
}



void aal_epon_intr_mpcp_msg_proc(struct work_struct *wq)
{
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT_t  intr_src; /* from message */
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE_t intr_ena; /* from message */
    pon_work_info_t *act_work = container_of(wq, pon_work_info_t, work);
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE_t tmp_ena;  /* used to re-enable interrupt */
    aal_mpcp_dereg_cause_t dereg_cause = AAL_MPCP_DEREG_UNKNOWN;

    intr_src.wrd = act_work->data[0];
    intr_ena.wrd = act_work->data[1];
    tmp_ena.wrd  = 0;

    __EPON_INTR_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-epon: EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT(0x%08x), EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE(0x%08x)\n", intr_src, intr_ena);

    if (intr_src.bf.gate_timer_expiri && intr_ena.bf.gate_timer_expiriE)
    {
        dereg_cause = AAL_MPCP_DEREG_GATE;  /* possible de-register cause */
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_MPCP_GATE_TIMEOUT, 0);
    }

    if (intr_src.bf.mpcp_timer_expiri && intr_ena.bf.mpcp_timer_expiriE)
    {
        dereg_cause = AAL_MPCP_DEREG_TIMEOUT;  /* possible de-register cause */
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_MPCP_MPCP_TIMEOUT, 0);
    }

    if (intr_src.bf.ts_drifti && intr_ena.bf.ts_driftiE)
    {
        dereg_cause = AAL_MPCP_DEREG_TIMEDRFT;  /* possible de-register cause */
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_MPCP_TIMESTAMP_DRIFT, 0);
    }

    if (intr_src.bf.reg_status_changei && intr_ena.bf.reg_status_changeiE)
    {
        dereg_cause = AAL_MPCP_DEREG_UNKNOWN;
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_MPCP_REG_CHANGE, dereg_cause);
    }
  
    if (intr_src.bf.rx_register_framei && intr_ena.bf.rx_register_frameiE)
    {
        aal_epon_intr_handler(AAL_EPON_INTR_TYPE_MPCP_RX_REG_FRAME, 0);
    }



    /* re-enable MPCP related interrupts */
    tmp_ena.bf.reg_status_changeiE = 1;
    tmp_ena.bf.rx_register_frameiE = 1;
    tmp_ena.bf.gate_timer_expiriE  = 1;
    tmp_ena.bf.mpcp_timer_expiriE  = 1;
    tmp_ena.bf.ts_driftiE = 1;

    (void)aal_epon_intr_ena(0, AAL_EPON_INTR_INDEX_MPCP, tmp_ena.wrd);

    return;
}


ca_status_t aal_epon_intr_glb_handler(ca_device_id_t device_id, ca_uint32_t irq, ca_uint32_t epon_intr)
{
    EPON_EPON_GLB_INTERRUPT_t  intr_src;
    EPON_EPON_GLB_EXTIF_INTERRUPT_t intr_extif_src;

    //EPON_EPON_GLB_INTERRUPTE_t intr_ena;
    //EPON_EPON_GLB_INTERRUPTE_t tmp_ena;

    /* handle EPON GLB interrupt here */
    intr_src.wrd = epon_intr;//AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPT);
    //intr_ena.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPTE);

    intr_extif_src.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_EXTIF_INTERRUPT);

#if 0
    /* disable EPON global interrupt */
    tmp_ena.wrd = 0;
    tmp_ena.bf.xgepn_intE = 0;
    tmp_ena.bf.mpcp_intE  = 0;
    AAL_PON_REG_WRITE(tmp_ena.wrd, EPON_EPON_GLB_INTERRUPTE);
#endif
    /* handle INTR_INDEX interrupt here */
    //if (intr_extif_src.bf.pcs_sync_1gi || intr_extif_src.bf.sig_LOSSi)
    if (intr_extif_src.bf.pcs_sync_1gi)
    {
        aal_epon_intr_1gepn_handler(); /* handle INTR_TYPE interrupt here */
        aal_epon_intr_tbl[AAL_EPON_INTR_INDEX_1GPCS].count++;
    }


    if (intr_src.bf.xgepn_int)
    {
        aal_epon_intr_xgepn_handler(); /* handle INTR_TYPE interrupt here */
        aal_epon_intr_tbl[AAL_EPON_INTR_INDEX_XGEPN].count++;
    }

    if (intr_src.bf.mpcp_int)
    {
        aal_epon_intr_mpcp_handler(); /* handle INTR_TYPE interrupt here */
        aal_epon_intr_tbl[AAL_EPON_INTR_INDEX_MPCP].count++;
    }
#if 0
    /* re-enable EPON global interrupt */
    tmp_ena.wrd = 0;
    tmp_ena.bf.xgepn_intE = 1;
    tmp_ena.bf.mpcp_intE  = 1;
    AAL_PON_REG_WRITE(tmp_ena.wrd, EPON_EPON_GLB_INTERRUPTE);
#endif

    return CA_E_OK;
}

/*******************************************************************/
/*                    Local Functions                              */
/*******************************************************************/
/* one interrupt type can reg multi-handlers */
ca_status_t aal_epon_intr_handler_reg
(
    CA_IN aal_epon_intr_type_t    type,
    CA_IN aal_epon_intr_handler_f handler
)
{
    aal_epon_intr_handler_list_t *p = &aal_epon_intr_handler_head[type];
    aal_epon_intr_handler_list_t *temp = NULL;

    if (type > AAL_EPON_INTR_TYPE_MAX)
        return CA_E_PARAM;

    if (NULL == handler)
        return CA_E_PARAM;


    while (p->next != NULL)
        p = p->next;

    temp = (aal_epon_intr_handler_list_t *)ca_malloc(sizeof(aal_epon_intr_handler_list_t));
    if (NULL == temp)
        return CA_E_RESOURCE;

    temp->func = handler;
    temp->next = NULL;
    p->next    = temp;

    return CA_E_OK;
}

void aal_epon_intr_handler(aal_epon_intr_type_t type, ca_uint32_t cookie)
{
    aal_epon_intr_handler_list_t *handler_list;

    if (type > AAL_EPON_INTR_TYPE_MAX)
        return;

    handler_list = &aal_epon_intr_handler_head[type];

    while (NULL != handler_list->next)
    {
        handler_list = handler_list->next;

        if (NULL!= handler_list->func)
            handler_list->func(cookie);
    }
}

ca_status_t aal_epon_intr_ena_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_id,
    CA_IN ca_uint32_t  intr_en
)
{
    if (intr_id > AAL_EPON_INTR_INDEX_MAX)
        return CA_E_PARAM;

    AAL_PON_REG_WRITE(intr_en, aal_epon_intr_tbl[intr_id].ena_reg_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_intr_ena_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_id,
    CA_OUT ca_uint32_t *intr_en
)
{
    if ((NULL == intr_en) || (intr_id > AAL_EPON_INTR_INDEX_MAX))
        return CA_E_PARAM;

    *intr_en = AAL_PON_REG_READ(aal_epon_intr_tbl[intr_id].ena_reg_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_intr_ena
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_id,
    CA_IN ca_uint32_t  enable_bmp
)
{
    ca_uint32_t intr_en = 0;

    if (intr_id > AAL_EPON_INTR_INDEX_MAX)
        return CA_E_PARAM;

    intr_en = AAL_PON_REG_READ(aal_epon_intr_tbl[intr_id].ena_reg_addr);
    intr_en |= enable_bmp;
    AAL_PON_REG_WRITE(intr_en, aal_epon_intr_tbl[intr_id].ena_reg_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_intr_dis
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_id,
    CA_IN ca_uint32_t  dis_bmp
)
{
    ca_uint32_t intr_en = 0;

    if (intr_id > AAL_EPON_INTR_INDEX_MAX)
        return CA_E_PARAM;

    intr_en = AAL_PON_REG_READ(aal_epon_intr_tbl[intr_id].ena_reg_addr);
    intr_en &= (~dis_bmp);
    AAL_PON_REG_WRITE(intr_en, aal_epon_intr_tbl[intr_id].ena_reg_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_intr_status_clr
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_id,
    CA_IN  ca_uint32_t  clr_bmp)
{
    if (intr_id > AAL_EPON_INTR_INDEX_MAX)
        return CA_E_PARAM;

    AAL_PON_REG_WRITE(clr_bmp, aal_epon_intr_tbl[intr_id].src_reg_addr); /* Write ones to clear */

    return CA_E_OK;
}

ca_status_t aal_epon_intr_status_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_id,
    CA_OUT ca_uint32_t  *status_bmp)
{
    if (intr_id > AAL_EPON_INTR_INDEX_MAX || NULL == status_bmp)
        return CA_E_PARAM;

    *status_bmp = AAL_PON_REG_READ(aal_epon_intr_tbl[intr_id].src_reg_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_intr_statistics_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint32_t intr_id,
    CA_OUT ca_uint64_t *stats
)
{
    if (intr_id > AAL_EPON_INTR_INDEX_MAX || NULL == stats)
        return CA_E_PARAM;

    *stats = aal_epon_intr_tbl[intr_id].count;

    return CA_E_OK;
}

void __epon_intr_handler_hook(void)
{
    ca_status_t rtn = CA_E_ERROR;

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_1GEPN_LINK_CHANGE, aal_epon_1gepn_link_change_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg: ret %d\n", rtn, __FILE__, __LINE__);
    }


    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_XGEPN_LINK_CHANGE, aal_epon_xgepn_link_change_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg: ret %d\n", rtn, __FILE__, __LINE__);
    }

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_XGEPN_SYNC_DET, aal_epon_xgepn_sync_det_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg: ret %d\n", rtn, __FILE__, __LINE__);
    }

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_MPCP_REG_CHANGE, aal_epon_mpcp_reg_change_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg return %d. FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_MPCP_MPCP_TIMEOUT, aal_epon_mpcp_timeout_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg return %d. FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_MPCP_GATE_TIMEOUT, aal_epon_mpcp_gate_timeout_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg return %d. FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }

    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_MPCP_TIMESTAMP_DRIFT, aal_epon_mpcp_timestamp_drift_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: aal_intr_handler_reg return %d. FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }
    rtn = aal_epon_intr_handler_reg(AAL_EPON_INTR_TYPE_MPCP_RX_REG_FRAME, aal_epon_mpcp_rx_reg_frame_handler);
    if (CA_E_OK != rtn)
    {
        __EPON_INTR_LOG(AAL_EPON_MPCP_TRACE_CRI, "EPON-INTR: aal_intr_handler_reg return %d. FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }
}

void __epon_intr_src_enable(ca_device_id_t  device_id)
{
    ca_uint32_t i = 0;
    EPON_EPON_GLB_EXTIF_INTERRUPTE_t pcs_intr;
    EPON_EPON_GLB_XGEPON_INTERRUPTE_t xgepn_intr;
    EPON_XGEPN_XGPCS_RX_RXINTE0_t xgpcs_intr;
    EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE_t mpcp_intr;
    EPON_EPON_GLB_INTERRUPTE_t glb_intr;

    /* disable all EPON interrupts and clear interrupt source */
    for (i=0; i<=AAL_EPON_INTR_INDEX_MAX; i++)
    {
        aal_epon_intr_ena_set(device_id, i, 0);         /* disable at first  */
        aal_epon_intr_status_clr(device_id, i, ~0); /* clear status then */
    }

    /* enable link-up and link-down interrupt for 1G-EPON */
    pcs_intr.wrd = 0;
    pcs_intr.bf.pcs_sync_1giE = 1;  // unplug fiber, sometimes there's no pcs_sync_1gi
    //pcs_intr.bf.sig_LOSSiE = 1;     // unplug fiber, sometimes there's no pcs_sync_1gi
    aal_epon_intr_ena(device_id, AAL_EPON_INTR_INDEX_1GPCS, pcs_intr.wrd);


    /* enable link-up and link-down interrupt for XG-EPON */
    xgpcs_intr.wrd = 0;
    xgpcs_intr.bf.linkupE   = 1;
    xgpcs_intr.bf.linkdownE = 1;
    //pcsrx_intr.bf.syncdetIE = 1;
    aal_epon_intr_ena(device_id, AAL_EPON_INTR_INDEX_XGPCS, xgpcs_intr.wrd);

    /* enable pcs rx interrupt for XG-EPON */
    xgepn_intr.wrd = 0;
    xgepn_intr.bf.xgepn_xgpcs_rxE = 1;
    aal_epon_intr_ena(device_id, AAL_EPON_INTR_INDEX_XGEPN, xgepn_intr.wrd);

    /* enable registration status change and gate timer interrupt for MPCP */
    mpcp_intr.wrd = 0;
    mpcp_intr.bf.reg_status_changeiE = 1;
    mpcp_intr.bf.rx_register_frameiE = 1;
    mpcp_intr.bf.gate_timer_expiriE  = 1;
    mpcp_intr.bf.mpcp_timer_expiriE  = 1;
    mpcp_intr.bf.ts_driftiE          = 1;

    aal_epon_intr_ena(device_id, AAL_EPON_INTR_INDEX_MPCP, mpcp_intr.wrd);

    /* enable EPON global interrupt */
    glb_intr.wrd = 0;
    //glb_intr.bf.extif_intE = 1;
    glb_intr.bf.xgepn_intE = 1;
    glb_intr.bf.mpcp_intE  = 1;
    AAL_PON_REG_WRITE(glb_intr.wrd, EPON_EPON_GLB_INTERRUPTE);

}

ca_status_t aal_epon_intr_init(ca_device_id_t  device_id)
{
    /* Global var init */
    memset(&aal_epon_intr_handler_head[0], 0, sizeof(aal_epon_intr_handler_head));

    __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "ca-epon: interrup is being initialized ... ");

    /* hook epon interrupts */
    __epon_intr_handler_hook();

    /* Hook the global epon interrupt handler entry  */
    //ca_osal_intr_reg(aal_epon_intr_glb_handler);

    /* enable epon interrupts */
    __epon_intr_src_enable(device_id);

    __EPON_INTR_LOG(AAL_EPON_INTR_TRACE_CRI, "done.\n");

    return CA_E_OK;
}

EXPORT_SYMBOL(aal_epon_intr_debug_set);
EXPORT_SYMBOL(aal_epon_intr_debug_get);
EXPORT_SYMBOL(aal_epon_intr_ena_set);
EXPORT_SYMBOL(aal_epon_intr_ena_get);
EXPORT_SYMBOL(aal_epon_intr_status_get);
EXPORT_SYMBOL(aal_epon_intr_statistics_get);

