
#include "osal_common.h"
#include "aal_pon.h"
#include <soc/cortina/registers.h>
#include "aal_mpcp.h"
#include "aal_puc.h"
#include "aal_epon.h"
#include "epon.h"
#include "ca_event.h"
#include "scfg.h"

/* macro define */
#define __MIN_PROCESS_TIME_MAX           0xfff
#define __MAX_FUTURE_GRANT_TIME_MAX      0x3ff
#define __MPCP_TIMER_THRESHOLD_MAX       0x3fff
#define __MPCP_TIMER_THRESHOLD_MIN       0x01f4
#define __GATE_TIMER_THRESHOLD_MAX       0x3fff
#define __GATE_TIMER_THRESHOLD_MIN       0x01f4
#define __REPORT_TIMER_THRESHOLD_MAX     0xfff
#define __TIME_STAMP_DRIFT_THRESHOLD_MAX AAL_MPCP_TIMEDRIFT_THRESHOLD_63TQ
#define __LASER_ON_TIME_MAX              0xff
#define __LASER_OFF_TIME_MAX             0xff
#define __KEEP_SILENCE_TIME_MAX          0xffffff

#define __DEF_KEEP_SILENCE_TIME          60*1000   /* 60s */
#define __DEF_MPCP_TIMEOUT               5000       /* 500ms */
#define __DEF_MPCP_GATE_TIMEOUT          500        /* 50ms */
#define __DEF_TIME_LASER_ON              32
#define __DEF_TIME_LASER_OFF             32

#define AAL_MPCP_LLID_CHECK(llid_idx) \
    if((llid_idx) > (AAL_EPON_LLID_NUM_MAX-1)) return CA_E_PARAM;

ca_uint __aal_mpcp_lock = 0;

#define __AAL_MPCP_LOCK(dev)       ca_spin_lock(__aal_mpcp_lock)
#define __AAL_MPCP_UNLOCK(dev)     ca_spin_unlock(__aal_mpcp_lock)

ca_uint __aal_mpcp_reg_lock = 0;
#define __AAL_MPCP_REG_LOCK(dev)   ca_spin_lock(__aal_mpcp_reg_lock)
#define __AAL_MPCP_REG_UNLOCK(dev) ca_spin_unlock(__aal_mpcp_reg_lock)


/* MPCP trace flag */
static ca_uint32_t g_epon_mpcp_trace = AAL_EPON_MPCP_TRACE_CRI;

static ca_uint8_t   g_bc_llid_idx = 0x3f;
static aal_mpcp_dereg_cause_t __dereg_cause[AAL_EPON_LLID_NUM_MAX];
static aal_mpcp_llid_silence_t __keep_silence[AAL_EPON_LLID_NUM_MAX];

static ca_uint32_t __llid_num = AAL_EPON_LLID_NUM_MAX;

static ca_uint32_t __llid_reg_mode = AAL_EPON_LLID_REG_MODE_AUTO;
static ca_uint32_t __llid_reg_status_bmp = 0; /* bitmap for llid registration status */
static ca_uint32_t __llid_reg_wait_bmp = 0; /* wait for llid registration */
static ca_uint16 __llid_mapping[AAL_EPON_LLID_NUM_MAX]; /* LLID index <-> LLID value */

/* multicast LLID table */
static aal_mpcp_mc_llid_t __mc_llid[AAL_MPCP_MAX_MC_LLID_NUMS];

#define __EPON_MPCP_LOG(level, args...) do { if (g_epon_mpcp_trace & level) ca_printf(args); }while(0)

#if 0
/* hardware and software registration mode */
ca_status_t aal_mpcp_register_mode_set(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_uint32_t           mode
)
{
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, sw_mode, mode);

    return CA_E_OK;
}

ca_status_t aal_mpcp_register_mode_get(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_OUT ca_uint32_t          *mode
)
{
    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_LLID_MAPPING_CFG1, sw_mode, *mode);
    return CA_E_OK;
}

ca_status_t aal_mpcp_register_frame_get(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_OUT ca_uint8_t           *mac_index,
    CA_OUT ca_uint8_t           *flag,
    CA_OUT ca_uint16_t          *assigned_llid
)
{
    EPON_XGONU_MPCP_MPCP_REGISTER_FRAME_STS_t frame;

    frame.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_REGISTER_FRAME_STS);
    if(frame.bf.vld == 0){
        ca_printf("no register frame received\r\n");
        return CA_E_NOT_FOUND;
    }

    *mac_index = frame.bf.mac_index;
    *flag = frame.bf.flag;
    *assigned_llid = frame.bf.assigned_llid;

    return CA_E_OK;
}
#endif

#ifdef __MC_LLID
#endif
ca_status_t aal_mpcp_mc_llid_add
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_mpcp_llid_t llid,
    CA_OUT ca_llid_t     *llid_instance
)
{
    ca_uint8_t  onu_type = AAL_ONU_TYPE_1G_1G;
    aal_mpcp_llid_entry_oper_t oper = AAL_MPCP_LLID_ENTRY_ADD;
    aal_mpcp_llid_entry_t llid_entry;
    ca_uint8_t i = 0;

    if (!llid_instance)
        return CA_E_PARAM;

    for (i=0; i<AAL_MPCP_MAX_MC_LLID_NUMS; i++)
    {
        /* go through the table to check whether there is same llid */
        if (__mc_llid[i].valid && (__mc_llid[i].llid == llid))
            return CA_E_OK;
    }

    for (i=0; i<AAL_MPCP_MAX_MC_LLID_NUMS; i++)
    {
        if (!__mc_llid[i].valid)
            break;
    }

    /* no avaible mc llid resource */
    if (i >= AAL_MPCP_MAX_MC_LLID_NUMS)
        return CA_E_ERROR;

    memset((void *)&llid_entry, 0, sizeof(llid_entry));

    (void)aal_epon_mac_onu_type_get(0,0, &onu_type);

    if (g_bc_llid_idx == 0x1f)
    {
        if (onu_type == AAL_ONU_TYPE_1G_1G)
        {
            if (llid == 0x7fff || llid == 0xffff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for 1G BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
        else /* XG ONU */
        {
            if (llid == 0x7ffe || llid == 0xfffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for XG BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
    }
    else
    {
        ca_printf("aal_mpcp_mc_llid_add(): CFG_ID_PON_BC_7FFF_TO_LLID_INDEX must be 0x1f.\n");
        return CA_E_ERROR;
    }
    #if 0
    else if (g_bc_llid_idx == 0x3e)
    {
        if (onu_type == AAL_ONU_TYPE_1G_1G)
        {
            if (llid == 0xffff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 31;    /* always use interal llid 63 for 1G BC llid */
            else if (llid == 0x7fff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 30;    /* always use interal llid 62 for 1G BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
        else /* XG ONU */
        {
            if (llid == 0xfffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 31;    /* always use interal llid 63 for XG BC llid */
            else if (llid == 0x7ffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 30;    /* always use interal llid 62 for XG BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
    }
    else
    {
        ca_printf("aal_mpcp_mc_llid_add(): invalid bc llid index %d, which must be either 0x3f or 0x3e.\n", g_bc_llid_idx);
        return CA_E_PARAM;
    }
    #endif

    llid_entry.llid_value = llid;
    llid_entry.mcast      = CA_FALSE;
    llid_entry.mac_idx    = llid_entry.llid_idx;

    if (aal_mpcp_llid_entry_set(device_id, 0, oper, &llid_entry) != CA_E_OK)
    {
        ca_printf("aal_mpcp_mc_llid_add(): failed to add llid entry for mc-llid %d\n", llid);
        return CA_E_ERROR;
    }

    *llid_instance = llid_entry.llid_idx;

    /* update software mc llid table */
    __mc_llid[i].valid = CA_TRUE;
    __mc_llid[i].llid  = llid;

    return CA_E_OK;
}

ca_status_t aal_mpcp_mc_llid_del
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_mpcp_llid_t llid
)
{
    ca_uint8_t  onu_type = AAL_ONU_TYPE_1G_1G;
    aal_mpcp_llid_entry_oper_t oper = AAL_MPCP_LLID_ENTRY_DEL;
    aal_mpcp_llid_entry_t llid_entry;
    ca_uint8_t i = 0;

    for (i=0; i<AAL_MPCP_MAX_MC_LLID_NUMS; i++)
    {
        if (__mc_llid[i].valid && __mc_llid[i].llid == llid)
            break;
    }

    /* system cannot hit the llid value, just do nothing */
    if (i >= AAL_MPCP_MAX_MC_LLID_NUMS)
        return CA_E_OK;

    memset((void *)&llid_entry, 0, sizeof(llid_entry));

    (void)aal_epon_mac_onu_type_get(0,0, &onu_type);

    if (g_bc_llid_idx == 0x1f)
    {
        if (onu_type == AAL_ONU_TYPE_1G_1G)
        {
            if (llid == 0xffff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for 1G BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
        else /* XG ONU */
        {
            if (llid == 0xfffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for XG BC llid */
            else
    llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
    }
    else
    {
        ca_printf("aal_mpcp_mc_llid_add(): invalid bc llid index %d, which must be either 0x1f or 0x1e.\n", g_bc_llid_idx);
        return CA_E_PARAM;
    }

    #if 0
    else if (g_bc_llid_idx == 0x1e)
    {
        if (onu_type == AAL_ONU_TYPE_1G_1G)
        {
            if (llid == 0xffff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for 1G BC llid */
            else if (llid == 0x7fff)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 14;    /* always use interal llid 30 for 1G BC llid */
            else
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + i;
        }
        else /* XG ONU */
        {
            if (llid == 0xfffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 15;    /* always use interal llid 31 for XG BC llid */
            else if (llid == 0x7ffe)
                llid_entry.llid_idx  = AAL_MPCP_MC_LLID_OFFSET + 14;    /* always use interal llid 30 for XG BC llid */
            else
                llid_entry.llid_idx   = AAL_MPCP_MC_LLID_OFFSET + i;
        }
    }
    else
    {
        ca_printf("aal_mpcp_mc_llid_add(): invalid bc llid index %d, which must be either 0x1f or 0x1e.\n", g_bc_llid_idx);
        return CA_E_PARAM;
    }
    #endif

    llid_entry.llid_value = llid;
    llid_entry.mcast      = CA_FALSE;
    llid_entry.mac_idx    = llid_entry.llid_idx;

    if (aal_mpcp_llid_entry_set(device_id, 0, oper, &llid_entry) != CA_E_OK)
    {
        ca_printf("aal_mpcp_mc_llid_del(): failed to delete llid entry for mc-llid %d\n", llid);
        return CA_E_ERROR;
    }

    /* update software mc llid table - release the entry */
    __mc_llid[i].valid = CA_FALSE;
    __mc_llid[i].llid  = AAL_MPCP_BC_LLID;

    return CA_E_OK;
}

ca_status_t aal_mpcp_mc_llid_list
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *mc_llid_nums,
    CA_OUT ca_mpcp_llid_t *mc_llids
)
{
    ca_uint32_t nums = 0;
    ca_uint8_t  i = 0;

    if (!mc_llids || !mc_llid_nums)
        return CA_E_PARAM;

    if (*mc_llid_nums == 0)    /*at least one element is required */
        return CA_E_PARAM;

    for (i=0; i<AAL_MPCP_MAX_MC_LLID_NUMS; i++)
    {
        if (__mc_llid[i].valid)
        {
            mc_llids[nums] = __mc_llid[i].llid;
            nums++;

            if (nums >= *mc_llid_nums)
                break;
        }
    }

    *mc_llid_nums = nums;

    return CA_E_OK;
}

ca_status_t aal_mpcp_debug_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    level
)
{
    g_epon_mpcp_trace = level;

    return CA_E_OK;
}

ca_status_t aal_mpcp_debug_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *level
)
{
    AAL_ASSERT_RET(level, CA_E_PARAM);

    *level = g_epon_mpcp_trace;

    return CA_E_OK;
}


ca_status_t aal_mpcp_cfg_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN aal_mpcp_msk_t mask,
    CA_IN aal_mpcp_cfg_t *cfg
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    if (mask.s.request_frame_llid)
    {
        if(cfg->request_frame_llid > AAL_MPCP_REQ_LLID_0X7FFE)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.pending_grant)
    {
        if(cfg->pending_grant > AAL_MPCP_PENDING_GRANT_8)/*need to improve for mllid check*/
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.min_process_time)
    {
        if (cfg->min_process_time > __MIN_PROCESS_TIME_MAX)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.max_future_grant_time)
    {
        if (cfg->max_future_grant_time > __MAX_FUTURE_GRANT_TIME_MAX)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.mpcptimer_threshold)
    {
        if ( cfg->mpcptimer_threshold > __MPCP_TIMER_THRESHOLD_MAX || cfg->mpcptimer_threshold < __MPCP_TIMER_THRESHOLD_MIN )
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.gatetimer_threshold)
    {
        if ( cfg->gatetimer_threshold > __GATE_TIMER_THRESHOLD_MAX || cfg->gatetimer_threshold < __GATE_TIMER_THRESHOLD_MIN )
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.reporttimer_threshold)
    {
        if ( cfg->reporttimer_threshold > __REPORT_TIMER_THRESHOLD_MAX )
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.timedrift_threshold)
    {
        if ( cfg->timedrift_threshold > __TIME_STAMP_DRIFT_THRESHOLD_MAX )
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.llid_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, mllid_mode, cfg->llid_mode);
    }

    if(mask.s.request_frame_llid)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, req_frame_llid_sel, cfg->request_frame_llid);
    }

    if(mask.s.mpcp_report_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, report_mode_sel, cfg->mpcp_report_mode);
    }

    if(mask.s.pending_grant)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, max_pending_grant, cfg->pending_grant);
    }

    if(mask.s.grant_fifo_dereg_clr)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, flush_grtff_en, cfg->grant_fifo_dereg_clr);
    }

    if(mask.s.min_grant_size_1g)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG_1G, min_pkt_size_1g, cfg->min_grant_size_1g);
    }

    if(mask.s.fec_overhead_1g)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG_1G, fec_overhead_1g, cfg->fec_overhead_1g);
    }


    if(mask.s.tx_burst_overhead_10g)
    {
        // TODO: need anymore?
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG_10G, burst_overhd_10g, cfg->tx_burst_overhead_10g);
    }

    if(mask.s.min_grant_size_10g)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG_10G, min_grant_len_10g, cfg->min_grant_size_10g);
    }

    if(mask.s.min_process_time_check_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, dis_min_grant_check, (cfg->min_process_time_check_en==0));
    }

    if(mask.s.min_process_time)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_GRANT_TIMER, min_processing_time, cfg->min_process_time);
    }

    if(mask.s.max_future_grant_time)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_GRANT_TIMER, max_future_grant_time, cfg->max_future_grant_time);
    }

    if(mask.s.mpcptimer_threshold)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_TIMER_EXP_THRED, mpcp, cfg->mpcptimer_threshold);
    }

    if(mask.s.gatetimer_threshold)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_TIMER_EXP_THRED, gate, cfg->gatetimer_threshold);
    }

    if(mask.s.gatetimer_dereg_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, gate_timer_expir_dereg, cfg->gatetimer_dereg_en);
    }

    if(mask.s.mpcptimer_dereg_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, mpcp_timer_expir_dereg, cfg->mpcptimer_dereg_en);
    }

    if(mask.s.timedrift_dereg_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, en_ts_drift_dereg, cfg->timedrift_dereg_en);
    }

    if(mask.s.timedrift_threshold)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, ts_drift_threshold, cfg->timedrift_threshold);
    }

    if(mask.s.event_force_report_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, en_period_report, cfg->event_force_report_en);
    }

    if(mask.s.reporttimer_threshold)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_RPT_TIME_THRED, val, cfg->reporttimer_threshold);
    }

    if(mask.s.register_timeout_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_DIS_PEND_TIME_THRED, en, cfg->register_timeout_en);
    }

    if(mask.s.register_timer)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_DIS_PEND_TIME_THRED, val, cfg->register_timer);
    }

    if(mask.s.b2b_grant_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, bak_grt_en, cfg->b2b_grant_en);
    }

    if(mask.s.b2b_force_report_en)
    {
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, dis_force_rpt, (cfg->b2b_force_report_en == 0));
    }

    if(mask.s.register_initial_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, debug_mode, cfg->register_initial_mode);
    }

    if(mask.s.auto_discovery_en)
    {
        // TODO:
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, dis_auto_dcvy, (cfg->auto_discovery_en == 0));
    }

    if(mask.s.direct_registration_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, allow_direct_reg, cfg->direct_registration_en);
    }

    if(mask.s.laser_on_time)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG, laserOnTime_cfg, cfg->laser_on_time);
    }

    if(mask.s.laser_off_time)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG, laserOffTime_cfg, cfg->laser_off_time);
    }

    if(mask.s.mpcp_rx_ext_en)
    {
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL, EnRxExt, cfg->mpcp_rx_ext_en);
    }

    if(mask.s.mpcp_ext_to_cpu_en)
    {
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL, EnRxFwd, cfg->mpcp_to_cpu_en);
    }

    if(mask.s.mpcp_tx_ext_en)
    {
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_TX_EXTEND_CTRL, EnTxExt, cfg->mpcp_tx_ext_en);
    }

    if(mask.s.fake_bc_llid_pass)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, fake_bc_llid_pass, cfg->fake_bc_llid_pass);
    }

    if(mask.s.en_local_offline)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, cfg->en_local_offline);
    }

    return CA_E_OK;
}


ca_status_t aal_mpcp_cfg_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT aal_mpcp_cfg_t *cfg
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, mllid_mode, cfg->llid_mode);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, req_frame_llid_sel, cfg->request_frame_llid);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, report_mode_sel, cfg->mpcp_report_mode);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, max_pending_grant, cfg->pending_grant);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, flush_grtff_en, cfg->grant_fifo_dereg_clr);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG_1G, min_pkt_size_1g, cfg->min_grant_size_1g);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG_1G, fec_overhead_1g, cfg->fec_overhead_1g);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG_10G, burst_overhd_10g, cfg->tx_burst_overhead_10g);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG_10G, min_grant_len_10g, cfg->min_grant_size_10g);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, dis_min_grant_check, cfg->min_process_time_check_en);
    cfg->min_process_time_check_en = !cfg->min_process_time_check_en;
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_GRANT_TIMER, min_processing_time, cfg->min_process_time);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_GRANT_TIMER, max_future_grant_time, cfg->max_future_grant_time);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_TIMER_EXP_THRED, mpcp, cfg->mpcptimer_threshold);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_TIMER_EXP_THRED, gate, cfg->gatetimer_threshold);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, gate_timer_expir_dereg, cfg->gatetimer_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, mpcp_timer_expir_dereg, cfg->mpcptimer_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, en_ts_drift_dereg, cfg->timedrift_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, ts_drift_threshold, cfg->timedrift_threshold);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, en_period_report, cfg->event_force_report_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_RPT_TIME_THRED, val, cfg->reporttimer_threshold);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DIS_PEND_TIME_THRED, en, cfg->register_timeout_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DIS_PEND_TIME_THRED, val, cfg->register_timer);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, bak_grt_en, cfg->b2b_grant_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, dis_force_rpt, cfg->b2b_force_report_en);
    cfg->b2b_force_report_en = 0; // !cfg->b2b_force_report_en;
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, debug_mode, cfg->register_initial_mode);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, dis_auto_dcvy, cfg->auto_discovery_en);
    cfg->auto_discovery_en = 0; //!cfg->auto_discovery_en;
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, allow_direct_reg, cfg->direct_registration_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG, laserOnTime_cfg, cfg->laser_on_time);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG, laserOffTime_cfg, cfg->laser_off_time);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL, EnRxExt, cfg->mpcp_rx_ext_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL, EnRxFwd, cfg->mpcp_to_cpu_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_TX_EXTEND_CTRL, EnTxExt, cfg->mpcp_tx_ext_en);
    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_LLID_MAPPING_CFG1, fake_bc_llid_pass, cfg->fake_bc_llid_pass);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, cfg->en_local_offline);

    return CA_E_OK;
}

ca_status_t aal_mpcp_rx_ext_ctrl_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   en_rx_fwd,
    CA_IN ca_boolean_t   en_rx_ext
)
{
    // TODO:
#if 0
    EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL_t ctrl;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    /* read first */
    ctrl.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL);

    ctrl.bf.EnRxFwd = (en_rx_fwd != 0);
    ctrl.bf.EnRxExt = (en_rx_ext != 0);
    AAL_PON_REG_WRITE(ctrl.wrd, EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL);
#endif
    return CA_E_OK;
}

ca_status_t aal_mpcp_rx_ext_ctrl_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_OUT ca_boolean_t  *en_rx_fwd,
    CA_OUT ca_boolean_t  *en_rx_ext
)
{
#if 0
    EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL_t ctrl;
    //AAL_ASSERT_RET(en_rx_fwd, CA_E_PARAM);
    //AAL_ASSERT_RET(en_rx_ext, CA_E_PARAM);

    ctrl.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_RX_EXTEND_CTRL);
    *en_rx_fwd = ctrl.bf.EnRxFwd;
    *en_rx_ext = ctrl.bf.EnRxExt;
 #endif
    return CA_E_OK;
}

// TODO: following aal API need to support prt llid
ca_status_t aal_mpcp_llid_cfg_set
(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t            port_id,
    CA_IN  ca_uint8_t              llid_idx,
    CA_IN  aal_mpcp_llid_msk_t mask,
    CA_IN  aal_mpcp_llid_cfg_t     *cfg
)
{
    ca_uint32_t offline;
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_MPCP_LLID_CHECK(llid_idx);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    // TODO: per llid

    if (mask.s.keep_silence_time)
    {
        if ( cfg->keep_silence_time > __KEEP_SILENCE_TIME_MAX)
            return CA_E_PARAM;
    }
    if (mask.s.register_permit)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, llid_en, cfg->register_permit);

    __AAL_MPCP_LOCK(device_id);
    if (mask.s.offline)
    {
        AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, offline);
        if (cfg->offline != 0)
        {
            offline |= (1<<llid_idx);
            __dereg_cause[llid_idx] = AAL_MPCP_DEREG_SW;    /* software disabled the registration */
        }
        else
        {
            offline &= ~(1<<llid_idx);
            __dereg_cause[llid_idx] = AAL_MPCP_DEREG_UNKNOWN;   /* default value zero */
        }
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, offline);

    }
    __AAL_MPCP_UNLOCK(device_id);

    if (mask.s.grant_permit)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, clr_permit, !(cfg->grant_permit));
    if (mask.s.gatetimer_dereg_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, gate_timer_expir_dereg, cfg->gatetimer_dereg_en);
    if (mask.s.mpcptimer_dereg_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, mpcp_timer_expir_dereg, cfg->mpcptimer_dereg_en);
    if (mask.s.timedrift_dereg_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, en_ts_drift_dereg, cfg->timedrift_dereg_en);

    if (mask.s.period_report_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG, en_period_report, cfg->period_report_en);
    if (mask.s.queue_id) // TODO:
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_RPT_LLID_COS_MAP, llid0, cfg->queue_id);
#if 0
    if(mask.s.sch_oam_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_RPT_SCHCTRL_EN, llid0_oam_pkt_sch_en, cfg->sch_oam_en);
    if(mask.s.sch_data_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_RPT_SCHCTRL_EN, llid0_data_pkt_sch_en, cfg->sch_data_en);
    if(mask.s.rpt_oam_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_RPT_RPTCTRL_EN, llid0_oam_pkt_rpt_en, cfg->rpt_oam_en);
    if(mask.s.rpt_data_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_RPT_RPTCTRL_EN, llid0_data_pkt_rpt_en, cfg->rpt_data_en);
#endif
    if (mask.s.mpcptimer_intrp_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, mpcp_timer_expiriE, cfg->mpcptimer_intrp_en);
    if (mask.s.gatetimer_intrp_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, gate_timer_expiriE, cfg->gatetimer_intrp_en);
    if (mask.s.timedrift_intrp_en)
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, ts_driftiE, cfg->timedrift_intrp_en);

    __AAL_MPCP_LOCK(device_id);

    if (mask.s.keep_silence_en)
        __keep_silence[llid_idx].enable = cfg->keep_silence_en;

    if (mask.s.keep_silence_time)
        __keep_silence[llid_idx].timer_value = cfg->keep_silence_time;

    __AAL_MPCP_UNLOCK(device_id);
    return CA_E_OK;
}

ca_status_t aal_mpcp_llid_cfg_get
(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  ca_port_id_t        port_id,
    CA_IN  ca_uint8_t          llid_idx,
    CA_OUT aal_mpcp_llid_cfg_t *cfg
)
{
    ca_uint32_t offline = 0;
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_MPCP_LLID_CHECK(llid_idx);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, llid_en, cfg->register_permit);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, offline);
    if (offline & (1 << llid_idx))
        cfg->offline = 1;
    else
        cfg->offline = 0;

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, clr_permit, cfg->grant_permit);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, gate_timer_expir_dereg, cfg->gatetimer_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, mpcp_timer_expir_dereg, cfg->mpcptimer_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, en_ts_drift_dereg, cfg->timedrift_dereg_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG, en_period_report, cfg->period_report_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_RPT_LLID_COS_MAP, llid0, cfg->queue_id);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_RPT_SCHCTRL_EN, llid0_oam_pkt_sch_en, cfg->sch_oam_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_RPT_SCHCTRL_EN, llid0_data_pkt_sch_en, cfg->sch_data_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_RPT_RPTCTRL_EN, llid0_oam_pkt_rpt_en, cfg->rpt_oam_en);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_RPT_RPTCTRL_EN, llid0_data_pkt_rpt_en, cfg->rpt_data_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, mpcp_timer_expiriE, cfg->mpcptimer_intrp_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, gate_timer_expiriE, cfg->gatetimer_intrp_en);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPTE, ts_driftiE, cfg->timedrift_intrp_en);

    cfg->keep_silence_en   = __keep_silence[llid_idx].enable;
    cfg->keep_silence_time = __keep_silence[llid_idx].timer_value;
    cfg->grant_permit = !cfg->grant_permit;

    return CA_E_OK;
}

ca_status_t aal_mpcp_llid_mode_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port,
    CA_IN ca_uint32      mode
)
{
    aal_mpcp_msk_t          mpcp_msk;
    aal_mpcp_cfg_t          mpcp_cfg;
    aal_mpcp_llid_msk_t     llid_msk;
    aal_mpcp_llid_cfg_t     llid_cfg;
    ca_uint8_t              llid_idx  = 0;
    ca_status_t             rtn = CA_E_ERROR;

    if (mode > AAL_MPCP_LLID_MODE_MULTIPLE)
    {
        //CA_LOG_MIN(CS_MOD_MPCP, "ERROR: llid mode %d\n", mode);
        return rtn;
    }

    if ((rtn = aal_mpcp_cfg_get(device_id, port, &mpcp_cfg)) != CA_E_OK)
    {
        //CS_LOG_MAJ(CS_MOD_MPCP, "Error(%d)! FILE: %s, LINE: %d\n", ret, __FILE__, __LINE__);
        return rtn;
    }

    memset((void *)&mpcp_cfg, 0, sizeof(mpcp_cfg));
    memset((void *)&llid_cfg, 0, sizeof(llid_cfg));

    mpcp_cfg.llid_mode   = (ca_uint8_t)mode;
    mpcp_msk.u64         = 0;
    mpcp_msk.s.llid_mode = 1;

    if (AAL_MPCP_LLID_MODE_MULTIPLE == mode)
    {
        if (mpcp_cfg.mpcp_report_mode < AAL_MPCP_RPT_B_1Q_1S)
        {
            mpcp_cfg.mpcp_report_mode   = AAL_MPCP_RPT_B_1Q_1S;
            mpcp_msk.s.mpcp_report_mode = 1;
        }

        llid_msk.u32               = 0;
        llid_msk.s.register_permit = 1;

        for (llid_idx=0; llid_idx<AAL_EPON_LLID_NUM_MAX; llid_idx++)
        {
            llid_cfg.register_permit = 1;  /* enable registration permiision for LLID 0 ~ 31 */

            if ((rtn = aal_mpcp_llid_cfg_set(device_id, port, llid_idx, llid_msk, &llid_cfg)) != CA_E_OK)
            {
                //CS_LOG_MAJ(CS_MOD_MPCP, "Error(%d)! FILE: %s, LINE: %d\n", ret, __FILE__, __LINE__);
                return rtn;
            }
        }
    }
    else
    {
        if (mpcp_cfg.mpcp_report_mode > AAL_MPCP_RPT_B_1Q_2S)
        {
            mpcp_cfg.mpcp_report_mode   = AAL_MPCP_RPT_B_1Q_2S;
            mpcp_msk.s.mpcp_report_mode = 1;
        }

        llid_msk.u32               = 0;
        llid_msk.s.register_permit = 1;

        for (llid_idx=0; llid_idx<AAL_EPON_LLID_NUM_MAX; llid_idx++)
        {
            if (llid_idx == 0)
               llid_cfg.register_permit = 1;  /* enable registration permiision for LLID 0 */
            else
               llid_cfg.register_permit = 0;  /* disable registration permission for LLID 1 ~ 31 */

            if ((rtn = aal_mpcp_llid_cfg_set(device_id, port, llid_idx, llid_msk, &llid_cfg)) != CA_E_OK)
            {
                //CS_LOG_MAJ(CS_MOD_MPCP, "Error(%d)! FILE: %s, LINE: %d\n", ret, __FILE__, __LINE__);
                return rtn;
            }
        }
    }

    rtn = aal_mpcp_cfg_set(device_id, port, mpcp_msk, &mpcp_cfg);
    if (rtn != CA_E_OK)
    {
        //CS_LOG_MAJ(CS_MOD_MPCP, "Error(%d)! FILE: %s, LINE: %d\n", ret, __FILE__, __LINE__);
        return rtn;
    }

    return CA_E_OK;
}

ca_status_t aal_mpcp_llid_mode_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port,
    CA_OUT ca_uint32_t   *mode
)
{
    ca_status_t rtn = CA_E_ERROR;
    ca_uint8_t llid_mode = 0;

    if (!mode)
        return rtn;

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, mllid_mode, llid_mode);

    *mode = llid_mode;

    return CA_E_OK;
}

ca_status_t aal_mpcp_mac_addr_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     llid_idx,
    CA_IN ca_mac_addr_t  mac
)
{
    ca_uint16_t mac_hi;
    ca_uint32_t mac_lo;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_MPCP_LLID_CHECK(llid_idx);

    /* All Zero MAC address is invalid */
    if(mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0)
    {
        return CA_E_PARAM;
    }

    /* Multicast and broadcast MAC address is invalid */
    if(0x1 == (mac[0]&0x1) )
    {
        return CA_E_PARAM;
    }

    mac_hi = (mac[0] << 8) | mac[1];
    mac_lo = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_XGEPN_MAC_ADDR_H, addr, mac_hi);
    AAL_PON_REG_WRITE(mac_lo, EPON_EPON_GLB_XGEPN_MAC_ADDR_L);

    return CA_E_OK;
}

ca_status_t aal_mpcp_mac_addr_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     llid_idx,
    CA_OUT ca_mac_addr_t mac
)
{
    ca_uint16_t mac_hi;
    ca_uint32_t mac_lo;

    //AAL_NULL_POINTER_CHECK(mac);
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_MPCP_LLID_CHECK(llid_idx);

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_XGEPN_MAC_ADDR_H, addr, mac_hi);
    mac_lo = AAL_PON_REG_READ(EPON_EPON_GLB_XGEPN_MAC_ADDR_L);

    mac[5] = (mac_lo & 0xff) + llid_idx;
    mac[4] = (mac_lo >> 8)& 0xff;
    mac[3] = (mac_lo >> 16)& 0xff;
    mac[2] = (mac_lo >> 24)& 0xff;
    mac[1] = mac_hi & 0xff;
    mac[0] = (mac_hi >> 8)& 0xff;

    return CA_E_OK;
}

ca_status_t aal_mpcp_report_threshold_set
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     set_id,
    CA_IN  ca_uint8_t     queue_id,
    CA_IN  ca_uint16_t    threshold
)
{
    // TODO: max num, update to macro
    if ((queue_id > 7) || (set_id > 3))
    {
        return CA_E_PARAM;
    }

    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    //AAL_PON_REG_WRITE(threshold, EPON_XGONU_RPT_RPT_THRESHOLD_Q0S1+(set_id*8+queue_id)*4);

    return CA_E_OK;
}

ca_status_t aal_mpcp_report_threshold_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     set_id,
    CA_IN  ca_uint8_t     queue_id,
    CA_OUT ca_uint16_t    *threshold
)
{
    if ((queue_id > 7) || (set_id > 3))
    {
        return CA_E_PARAM;
    }
    //AAL_NULL_POINTER_CHECK(threshold);
    //AAL_ASSERT_RET(threshold, CA_E_PARAM);

    //*threshold = AAL_PON_REG_READ(EPON_XGONU_RPT_RPT_THRESHOLD_Q0S1+(set_id*8+queue_id)*4);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_start_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     llid_idx,
    CA_IN ca_uint8_t     mac_idx
)
{
    AAL_MPCP_LLID_CHECK(llid_idx);
    AAL_MPCP_LLID_CHECK(mac_idx);

    //Using llid-idx 4 for registeration
    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_INIT_CFG, sw_index,  llid_idx+4);

    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_INIT_CFG, mac_index, mac_idx);

    /* start the registration process */
    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_START_CFG, mpcp_start, 1);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_start_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint8_t     llid_idx,
    CA_OUT ca_uint8_t     *mac_idx
)
{
    return CA_E_OK;
}


ca_status_t aal_mpcp_reg_toggle
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     llid_idx,
    CA_IN ca_uint8_t     mac_idx
)
{
    AAL_MPCP_LLID_CHECK(llid_idx);
    AAL_MPCP_LLID_CHECK(mac_idx);

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_INIT_CFG, sw_index, llid_idx);
    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_INIT_CFG, mac_index, mac_idx);
    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_START_CFG, mpcp_start, 0);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_llid_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint16_t    *llid
)
{
    ca_uint8_t  i = 0;
    EPON_EPON_GLB_LLID_TABLE_ENTRY0_t entry;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_MPCP_LLID_CHECK(llid_idx);
    AAL_ASSERT_RET(llid, CA_E_PARAM);

    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LLID_SYNCTIME, assigned_llid, *llid);

    entry.wrd = 0;

    for (i=0; i<AAL_EPON_LLID_INDEX_MAX; i++)
    {
        entry.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TABLE_ENTRY0 + i * 4);

        /* go through table and find the match for unicast, valid and mac_index */
        if (entry.bf.vld && !entry.bf.mcast && (llid_idx == entry.bf.mac_index))
        {
            *llid = entry.bf.llid;
            return CA_E_OK;
        }
    }
    *llid = AAL_MPCP_INVALID_LLID;
    return CA_E_NOT_FOUND;
}

ca_uint8_t aal_mpcp_llid_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid
)
{
    ca_uint8_t llid_index = 0xff;    /* invalid index */
    ca_uint8_t i = 0;
    EPON_EPON_GLB_LLID_TABLE_ENTRY0_t entry;

    entry.wrd = 0;

    for (i=0; i<AAL_EPON_LLID_INDEX_MAX; i++)
    {
        entry.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TABLE_ENTRY0 + i * 4);
        if (entry.bf.vld && llid == entry.bf.llid)
        {
            llid_index = entry.bf.index;
        }
    }

    return llid_index;
}


ca_status_t aal_mpcp_reg_llid_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint16_t    llid,
    CA_OUT  ca_uint8_t    *llid_idx
)
{
    ca_uint8_t  i = 0;
    EPON_EPON_GLB_LLID_TABLE_ENTRY0_t entry;

    entry.wrd = 0;

    for (i=0; i<AAL_EPON_LLID_INDEX_MAX; i++)
    {
        entry.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TABLE_ENTRY0 + i * 4);
        if (entry.bf.vld && llid == entry.bf.llid)
        {
            *llid_idx = entry.bf.mac_index;
            return CA_E_OK;
        }
    }

    return CA_E_NOT_FOUND;
}

ca_status_t aal_mpcp_llid_entry_set
(
    CA_IN  ca_device_id_t             device_id,
    CA_IN  ca_port_id_t               port_id,
    CA_IN  aal_mpcp_llid_entry_oper_t oper,
    CA_IN  aal_mpcp_llid_entry_t      *entry
)
{
    ca_uint32_t access_flag = 0;
    ca_uint8_t  i = 0;
    EPON_EPON_GLB_LLID_TBL_ACC_DAT_t data;
    EPON_EPON_GLB_LLID_TBL_ACC_STS_t sts;

    data.wrd = 0;
    data.bf.llid      = entry->llid_value;
    data.bf.sw_index  = entry->llid_idx;
    data.bf.mc_llid   = entry->mcast;
    data.bf.op        = oper;
    data.bf.mac_index = entry->mac_idx;
    AAL_PON_REG_WRITE(data.wrd, EPON_EPON_GLB_LLID_TBL_ACC_DAT);

    //ca_printf("start to %s entry: llid %d, sw_idx %d, mc_llid %d, op %d, mac_index %d\n",
            //(oper == AAL_MPCP_LLID_ENTRY_ADD)?"add":"delete", data.bf.llid, data.bf.sw_index, data.bf.mc_llid, data.bf.op, data.bf.mac_index);

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_TBL_ACC, acc, 1);    /* write llid table */
    do {
        AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_LLID_TBL_ACC, acc, access_flag);
        if (++i >= 1000)    /* table access failure */
            return CA_E_TIMEOUT;
    }while(access_flag);

    sts.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TBL_ACC_STS);
    if (oper == AAL_MPCP_LLID_ENTRY_ADD)
    {
        if ((sts.bf.status == AAL_MPCP_LLID_ENTRY_ADD_SUCCESS) || (sts.bf.status == AAL_MPCP_LLID_ENTRY_CFG_SUCCESS))
            return CA_E_OK;
        else if(sts.bf.status == AAL_MPCP_LLID_ENTRY_ADD_FAIL_FULL)
            return CA_E_FULL;
    }
    else if(oper == AAL_MPCP_LLID_ENTRY_DEL)
    {
        if (sts.bf.status == AAL_MPCP_LLID_ENTRY_DEL_SUCCESS)
            return CA_E_OK;
        else if(sts.bf.status == AAL_MPCP_LLID_ENTRY_DEL_FAIL_NO_MATCH)
            return CA_E_NOT_FOUND;
    }
    else
    {
        /* nothing */
    }

#if 0
    for (i=0; i<1000; i++)
    {

        sts.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TBL_ACC_STS);
        //ca_printf("EPON_EPON_GLB_LLID_TBL_ACC_STS %d\r\n", sts.wrd);

        if (oper == AAL_MPCP_LLID_ENTRY_ADD)
        {
            if ((sts.bf.status == AAL_MPCP_LLID_ENTRY_ADD_SUCCESS) || (sts.bf.status == AAL_MPCP_LLID_ENTRY_CFG_SUCCESS))
            {
                return CA_E_OK;
            }
            else if(sts.bf.status == AAL_MPCP_LLID_ENTRY_ADD_FAIL_FULL)
            {
                return CA_E_FULL;
            }
        }
        else if(oper == AAL_MPCP_LLID_ENTRY_DEL)
        {
            if (sts.bf.status == AAL_MPCP_LLID_ENTRY_DEL_SUCCESS)
            {
                //__dereg_cause[entry->llid_idx] = AAL_MPCP_DEREG_SW;
                return CA_E_OK;
            }
            else if(sts.bf.status == AAL_MPCP_LLID_ENTRY_DEL_FAIL_NO_MATCH)
            {
                return CA_E_NOT_FOUND;
            }
        }
    }
#endif
    return CA_E_ERROR;
}

ca_status_t aal_mpcp_llid_entry_all_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id
)
{
    ca_uint8_t  i = 0;
    EPON_EPON_GLB_LLID_TABLE_ENTRY0_t entry;

    entry.wrd = 0;

    ca_printf("\n%-5s | %-5s | %s | %s | %s | %s", "entry", "valid", " llid", "sw_idx", "mac_idx", "mcast");
    ca_printf("\n%-5s + %-5s + %s + %s + %s + %s", "-----", "-----", "-----", "------", "-------", "-----");

    for (i=0; i<=AAL_EPON_LLID_INDEX_MAX; i++)
    {
        entry.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_LLID_TABLE_ENTRY0 + i * 4);

        ca_printf("\n%-5d | %-5s | %5d | %6d | %7d | %-5s",
            i, (entry.bf.vld ? "TRUE" : "FALSE"), entry.bf.llid, entry.bf.index, entry.bf.mac_index, (entry.bf.mcast ? "TRUE" : "FALSE"));
    }

    ca_printf("\n");

    return CA_E_OK;
}

ca_status_t aal_mpcp_cmd_llid_entry_set
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    oper,
    CA_IN  ca_uint8_t     mac_idx,
    CA_IN  ca_uint16_t    llid_value,
    CA_IN  ca_boolean_t   mc_flag
)
{
    aal_mpcp_llid_entry_t entry;

    memset(&entry, 0, sizeof(entry));

    entry.llid_idx   = mac_idx;
    entry.llid_value = llid_value;
    entry.mac_idx    = mac_idx;
    entry.mcast      = 0;
    entry.mcast      = mc_flag;

    return aal_mpcp_llid_entry_set(device_id, port_id, oper, &entry);
}

ca_status_t aal_mpcp_cmd_llid_entry_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    oper,
    CA_IN  ca_uint8_t     mac_idx,
    CA_IN  ca_uint16_t    llid_value,
    CA_OUT ca_boolean_t   *mc_flag
)
{
    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_sync_time_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint16_t    *time
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(time, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LLID_SYNCTIME, sync_time, *time);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_laser_time_status_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint16_t    *laser_on_time,
    CA_OUT ca_uint16_t    *laser_off_time
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(laser_on_time, CA_E_PARAM);
    //AAL_ASSERT_RET(laser_off_time, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LASER_TIME_STS, laserOnTime_sts, *laser_on_time);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LASER_TIME_STS, laserOffTime_sts, *laser_off_time);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_status_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_boolean_t   *reg_flag
)
{
    ca_uint32_t value  = 0;
    ca_uint32_t status = 0;
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(reg, CA_E_PARAM);

    if (!reg_flag)
        return CA_E_PARAM;

    if (llid_idx < 16)
    {
        value = AAL_PON_REG_READ(EPON_XGONU_MPCP_MAC_MPCP_STS0);
        status = (value >> (llid_idx * 2)) & 0x3;
    }
    else
    {
        value = AAL_PON_REG_READ(EPON_XGONU_MPCP_MAC_MPCP_STS1);
        status = (value >> ((llid_idx - 16) * 2)) & 0x3;
    }

    *reg_flag = (status == AAL_MPCP_STAT_SEND_ACK) ? CA_TRUE : CA_FALSE;

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_flag_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint8_t     *flag
)
{
    if (!flag)
        return CA_E_PARAM;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(flag, CA_E_PARAM);
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_PKT_FLAG, register_flag, *flag);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_count_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint16_t    *count
)
{
    if (!count)
        return CA_E_PARAM;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(count, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_MON_CNTR, reged_cnt, *count);

    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_retry_times_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     llid_idx,
    CA_OUT ca_uint16_t    *times
)
{
    if (!times)
        return CA_E_PARAM;
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(count, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_MON_STS, retry_cnt, *times);

    return CA_E_OK;
}

ca_status_t aal_mpcp_dereg_cause_get
(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_IN  ca_uint8_t             llid_idx,
    CA_OUT aal_mpcp_dereg_cause_t *cause
)
{
    //ca_uint32_t val = 0;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(dereg_case, CA_E_PARAM);
    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DEREG_CAUSE, code, val);

    *cause = __dereg_cause[llid_idx];

    return CA_E_OK;
}

ca_status_t aal_mpcp_local_time_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *time
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(time, CA_E_PARAM);

    *time = AAL_PON_REG_READ(EPON_XGONU_MPCP_TIMESTAMP);
    return CA_E_OK;
}

ca_status_t aal_mpcp_local_discovery_info_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint16_t    *info
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(info, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_LOCAL_DISINFO, onu, *info);

    return CA_E_OK;
}

ca_status_t aal_mpcp_local_remote_info_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint16_t    *info
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(info, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_REMOTE_DISINFO, olt, *info);

    return CA_E_OK;
}

ca_status_t aal_mpcp_mib_stats_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT aal_mpcp_mib_stats_t *mib
)
{
    EPON_XGONU_MPCP_MPCP_GRANT_DROP_CNTR_t drop;
    EPON_XGONU_MPCP_MPCP_GRANT_DROP_CNTR1_t drop1;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(mib, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_RXPKT_CNTR, rx_pktcnt, (mib->mpcp_pkt_rx));
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_TXPKT_CNTR, tx_pktcnt, (mib->mpcp_pkt_tx));

    drop.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_GRANT_DROP_CNTR);
    mib->grant_rx_drop[0] = drop.bf.localLTstart_cnt;
    mib->grant_rx_drop[1] = drop.bf.min_cnt;
    mib->grant_rx_drop[2] = drop.bf.max_cnt;
    mib->grant_rx_drop[3] = drop.bf.ovd_cnt;

    drop1.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_GRANT_DROP_CNTR1);
    mib->grant_rx_drop[4] = drop1.bf.llid_mismatch_drop_cnt;
    mib->grant_rx_drop[5] = drop1.bf.rd_drop_cnt;
    mib->grant_rx_drop[6] = drop1.bf.rd_empty_cnt;
    mib->grant_rx_drop[7] = drop1.bf.wr_full_cnt;

    return CA_E_OK;
}

ca_status_t aal_mpcp_registration_restart(ca_uint8_t llid_idx)
{
    ca_uint32_t offline = 0;

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_DEREG, en_local_offline, offline);
    if ((offline & (1 << llid_idx)) == 0)/* registration enabled */
    {
        aal_mpcp_reg_start_set(0, 0, llid_idx, llid_idx);
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: starting to register LLID-IDX(%d)\n", llid_idx);
    }

    return CA_E_OK;
}

ca_status_t aal_mpcp_misc_init
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id
)
{
    aal_mpcp_msk_t misc_mask;
    aal_mpcp_cfg_t misc_cfg;
    aal_mpcp_llid_msk_t     llid_cfg_mask;
    aal_mpcp_llid_cfg_t     llid_cfg;
    ca_status_t rtn = CA_E_OK;
    ca_uint32_t i = 0;
    ca_uint8_t llid_mode = AAL_MPCP_LLID_MODE_SINGLE;
    ca_uint16_t sync_time = 0;
    ca_uint8_t laser_on_time = 0, laser_off_time = 0;
    ca_uint8_t report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    rtn = scfg_read(0, CFG_ID_PON_REPORT_MODE, sizeof(report_mode), &report_mode);
    if (rtn != CA_E_OK)
    {
        ca_printf("ca-mpcp: failed to get CFG_ID_PON_REPORT_MODE.\n");
        report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;
    }

    /* configure BC LLID to internal llid mapping */
    rtn = scfg_read(0, CFG_ID_PON_BC_7FFF_TO_LLID_INDEX, sizeof(g_bc_llid_idx), &g_bc_llid_idx);
    if (rtn != CA_E_OK)
    {
        ca_printf("ca-mpcp: failed to get CFG_ID_PON_BC_7FFF_TO_LLID_INDEX.\n");
        g_bc_llid_idx = 0x3f;
    }

    ca_printf("ca-mpcp: mapping bc-llid(0xffff/0xfffe/0x7fff/0x7ffe) to internal llid index(%d).\n", g_bc_llid_idx);

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, bcast_idx,      g_bc_llid_idx);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, fake_bcast_idx, g_bc_llid_idx);

    /* end - configure BC LLID to internal llid mapping */

    if (report_mode == CA_EPON_MPCP_REPORT_MODE_NTT)
    {
        ca_printf("ca-mpcp: enable SIEPON-B mode.\n");
        AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, sieponB_mode, 1);

        //g_toggle_mpcp_start_flag = CA_TRUE;
    }

    rtn = scfg_read(0, CFG_ID_PON_LLID_MODE, sizeof(llid_mode), &llid_mode);
    if (rtn != CA_E_OK)
        llid_mode = AAL_MPCP_LLID_MODE_SINGLE;

    ca_printf("ca-mpcp: llid mode - %s\n", (llid_mode == AAL_MPCP_LLID_MODE_SINGLE) ? "S-LLID" : "M-LLID");

    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_SYNCTIME_CFG, sync_time_dis, sync_time);

    if (llid_mode == AAL_MPCP_LLID_MODE_SINGLE)
    {
        sync_time |= 0xf800;    /* single llid : bit 11 ~ 15 1*/
    }
    else
    {
        /*
        ** multiple llid mode
        ** 2-llids,  5'b11110
        ** 4-llids,  5'b11100
        ** 8-llids,  5'b11000
        ** 16-llids, 5'b10000
        ** 32-llids, 5'b00000'
        */
        if (__llid_num <= 2)
            sync_time = (sync_time & 0x7ff) | 0xf000;
        else if (__llid_num <= 4)
            sync_time = (sync_time & 0x7ff) | 0xe000;
        else if (__llid_num <= 8)
            sync_time = (sync_time & 0x7ff) | 0xc000;
        else if (__llid_num <= 16)
            sync_time = (sync_time & 0x7ff) | 0x8000;
        else
            sync_time = (sync_time & 0x7ff);

    }

    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_SYNCTIME_CFG, sync_time_dis, sync_time);

    /* in order to be compatible UBI OLT, using padding field  */
    #if 0 /* need MACRO to match OLT vendor */
    AAL_PON_REG_WRITE(0x00200020, EPON_XGONU_MPCP_MPCP_REG_REQ_RESV0);
    AAL_PON_REG_WRITE(0x60014102, EPON_XGONU_MPCP_MPCP_REG_REQ_RESV1);
    #endif

    rtn = scfg_read(0, CFG_ID_PON_LASER_ON_TIME, sizeof(laser_on_time), &laser_on_time);
    if (rtn != CA_E_OK)
        laser_on_time = __DEF_TIME_LASER_ON;
    rtn = scfg_read(0, CFG_ID_PON_LASER_OFF_TIME, sizeof(laser_off_time), &laser_off_time);
    if (rtn != CA_E_OK)
        laser_off_time = __DEF_TIME_LASER_OFF;

    // mpcp misc init
    misc_cfg.llid_mode                 = llid_mode;
    misc_cfg.request_frame_llid        = AAL_MPCP_REQ_LLID_ASSIGNED;
    misc_cfg.mpcp_report_mode          = AAL_MPCP_RPT_S_8Q_2S;
    misc_cfg.pending_grant             = AAL_MPCP_PENDING_GRANT_4;  /* DEFAULT is 8 */
    misc_cfg.grant_fifo_dereg_clr      = 1;                         /* Just valid at SLLID */
    misc_cfg.mpcptimer_dereg_en        = 1;                         /* DEFAULT is 0 */
    misc_cfg.mpcptimer_threshold       = __DEF_MPCP_TIMEOUT;        /* 500ms. DEFAULT is 1000ms */
    misc_cfg.gatetimer_dereg_en        = 1;                         /* DEFAULT is 0 */
    misc_cfg.gatetimer_threshold       = __DEF_MPCP_GATE_TIMEOUT;   /* 50ms*/
    misc_cfg.timedrift_dereg_en        = 1;                         /* DEFAULT is 0 */
    misc_cfg.timedrift_threshold       = AAL_MPCP_TIMEDRIFT_THRESHOLD_8TQ;
    misc_cfg.register_timeout_en       = 1;                         /* wait OLT response */
    misc_cfg.register_timer            = 0xffff;                    /* 6.5s.  DEFAULT is 0xfffff(104s)*/
    misc_cfg.register_initial_mode     = AAL_MPCP_REG_INIT_SW;      /* DEFAULT is 0. ASIC suggust use default 0.*/
    misc_cfg.fec_overhead_1g           = 0xf;                       /* ASIC: (length of SOF+EOF (14) +celling *16)/2, celling = (len of frame +  IPG)/239  */
    misc_cfg.min_grant_size_1g         = 0x2a;
    misc_cfg.tx_burst_overhead_10g     = 0x2;
    misc_cfg.min_grant_size_10g        = 0xc;
    misc_cfg.auto_discovery_en         = 0;
    misc_cfg.direct_registration_en    = 0;

    /* increase lsr-on and lsr-off time for verification */
    misc_cfg.laser_on_time             = laser_on_time;
    misc_cfg.laser_off_time            = laser_off_time;

    misc_cfg.fake_bc_llid_pass         = 1;
    misc_mask.u64                      = 0;
    misc_mask.s.llid_mode              = 1;
    misc_mask.s.request_frame_llid     = 1;
    misc_mask.s.mpcp_report_mode       = 1;
    misc_mask.s.pending_grant          = 1;
    misc_mask.s.grant_fifo_dereg_clr   = 1;
    misc_mask.s.mpcptimer_dereg_en     = 1;
    misc_mask.s.mpcptimer_threshold    = 1;
    misc_mask.s.gatetimer_dereg_en     = 1;
    misc_mask.s.gatetimer_threshold    = 1;
    misc_mask.s.timedrift_dereg_en     = 1;
    misc_mask.s.timedrift_threshold    = 1;
    misc_mask.s.register_timeout_en    = 1;
    misc_mask.s.register_timer         = 1;
    misc_mask.s.register_initial_mode  = 1;
    misc_mask.s.fec_overhead_1g        = 1;
    misc_mask.s.min_grant_size_1g      = 1;
    misc_mask.s.tx_burst_overhead_10g  = 1;
    misc_mask.s.min_grant_size_10g     = 1;
    misc_mask.s.auto_discovery_en      = 1;
    misc_mask.s.direct_registration_en = 1;
    misc_mask.s.laser_on_time          = 1;
    misc_mask.s.laser_off_time         = 1;
    misc_mask.s.fake_bc_llid_pass      = 1;
    rtn = aal_mpcp_cfg_set(device_id, port_id,misc_mask, &misc_cfg);
    if (rtn != CA_E_OK)
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "aal_mpcp_cfg_set(): return %d, FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);


    /* disable all LLIDs */
    llid_cfg_mask.u32               = 0;
    llid_cfg_mask.s.register_permit = 1;
    llid_cfg_mask.s.keep_silence_en   = 1;
    llid_cfg_mask.s.keep_silence_time = 1;

    llid_cfg.register_permit        = 0;
    llid_cfg.keep_silence_en        = 1;                         /* DEFAULT is 0 */
    llid_cfg.keep_silence_time      = __DEF_KEEP_SILENCE_TIME;   /* 60s*/

    for (i=0; i<AAL_EPON_LLID_NUM_MAX; i++)
    {
        rtn = aal_mpcp_llid_cfg_set(device_id, port_id, i, llid_cfg_mask, &llid_cfg);
        if (rtn != CA_E_OK)
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "aal_mpcp_llid_cfg_set(): return %d, FILE: %s, LINE: %d\n", rtn, __FILE__, __LINE__);
    }

    return CA_E_OK;
}


ca_status_t aal_mpcp_pca_olt_mac_addr_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_mac_addr_t  mac
)
{
    ca_uint32_t mac_hi = 0;
    ca_uint32_t mac_lo = 0;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    mac_hi = ((mac[0] << 8) | (mac[1]));
    mac_lo = ((mac[2] << 24) | (mac[3] << 16) | (mac[4] <<8 ) | (mac[5]));

    AAL_PON_REG_WRITE(mac_hi, EPON_XGEPN_XGPCS_TX_SEED_D1);
    AAL_PON_REG_WRITE(mac_lo, EPON_XGEPN_XGPCS_TX_SEED_D0);

    return CA_E_OK;
}

ca_status_t aal_mpcp_pca_olt_mac_addr_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_mac_addr_t  mac
)
{
    ca_uint32_t mac_hi = 0;
    ca_uint32_t mac_lo = 0;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_NULL_POINTER_CHECK(mac);

    mac_hi = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_TX_SEED_D1);
    mac_lo = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_TX_SEED_D0);

    mac[0] = ((mac_hi >>  8) & 0xff);
    mac[1] = ((mac_hi >>  0) & 0xff);
    mac[2] = ((mac_lo >> 24) & 0xff);
    mac[3] = ((mac_lo >> 16) & 0xff);
    mac[4] = ((mac_lo >>  8) & 0xff);
    mac[5] = ((mac_lo >>  0) & 0xff);

    return CA_E_OK;
}

ca_status_t aal_mpcp_olt_mac_addr_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_mac_addr_t  mac
)
{
    ca_uint32_t mac_hi = 0;
    ca_uint32_t mac_lo = 0;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_NULL_POINTER_CHECK(mac);

    mac_hi = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_OLTMACH);
    mac_lo = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_OLTMACL);

    mac[0] = ((mac_hi >> 16) & 0xff);
    mac[1] = ((mac_hi >>  8) & 0xff);
    mac[2] = ((mac_hi >>  0) & 0xff);
    mac[3] = ((mac_lo >> 16) & 0xff);
    mac[4] = ((mac_lo >>  8) & 0xff);
    mac[5] = ((mac_lo >>  0) & 0xff);

    return CA_E_OK;
}

void __aal_epon_crypto_pcs_xg_olt_mac_cfg_set(void)
{
    ca_uint32_t olt_mac_high        = 0;
    ca_uint32_t olt_mac_lo            = 0;
    EPON_XGEPN_XGPCS_TX_SEED_D1_t tx_seed1;
    EPON_XGEPN_XGPCS_TX_SEED_D0_t tx_seed0;

    olt_mac_high = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_OLTMACH);
    olt_mac_lo   = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_OLTMACL);
    tx_seed1.wrd = (olt_mac_high >> 8)&0xFFFF;
    tx_seed0.wrd = (((olt_mac_high << 24)&0xFF000000) | (olt_mac_lo &0xFFFFFF));

    AAL_PON_REG_WRITE( tx_seed1.wrd,EPON_XGEPN_XGPCS_TX_SEED_D1);
    AAL_PON_REG_WRITE( tx_seed0.wrd,EPON_XGEPN_XGPCS_TX_SEED_D0);

    return;
}

ca_status_t aal_mpcp_reg_llid_add_all(ca_device_id_t device_id)
{
    ca_uint32_t i = 0;
    ca_uint32_t llid_mode = AAL_MPCP_LLID_MODE_SINGLE;
    ca_uint8_t  llid_num = 1;
    ca_uint8_t  reg_mode = AAL_EPON_LLID_REG_MODE_AUTO;
    ca_uint32_t llid_bmp = 0;

    reg_mode = aal_mpcp_reg_mode_get(0);

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: CFG_ID_EN_REGISTER_AFTER_BOOT(%d)\n", reg_mode);

    if (reg_mode != AAL_EPON_LLID_REG_MODE_MANUAL)
    {
        if (reg_mode == AAL_EPON_LLID_REG_MODE_AUTO)
        {
            aal_mpcp_llid_mode_get(0, 0, &llid_mode);
            if (llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
            { /* multi llid mode */
                llid_num = __llid_num;
            }
        }
        else
        {
            llid_num = 1;
        }

        for (i=0; i<llid_num; i++)
        {
            if (__keep_silence[i].timer_id)
            {/* keep silence is started */
                __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: llid_idx(%02d) is in keep silence state.\n", i);
            }
            else
            {
                llid_bmp |= (1<<i);
            }
        }
    }
    __AAL_MPCP_REG_LOCK();
    __llid_reg_wait_bmp = llid_bmp;
    __AAL_MPCP_REG_UNLOCK();

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: add all llids, llid_bmp(0x%08x), __llid_reg_wait_bmp(0x%08x)\n",
        llid_bmp, __llid_reg_wait_bmp);
    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_llid_del_all(ca_device_id_t  device_id)
{
    __AAL_MPCP_REG_LOCK();
    __llid_reg_wait_bmp = 0;
    __AAL_MPCP_REG_UNLOCK();
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: remove all llids, __llid_reg_wait_bmp(0x%08x)\n",
        __llid_reg_wait_bmp);
    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_llid_add(ca_device_id_t  device_id, ca_uint32_t llid_bmp)
{
    ca_uint32_t i = 0;

    for (i=0; i<__llid_num; i++)
    {
        if (__keep_silence[i].timer_id)
        {/* keep silence is started */
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: llid_idx(%02d) is in keep silence state.\n", i);
            llid_bmp &= ~(1<<i);
        }
    }

    __AAL_MPCP_REG_LOCK();
    __llid_reg_wait_bmp |= llid_bmp;
    __AAL_MPCP_REG_UNLOCK();

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: add llid_bmp(0x%08x) to waiting list, __llid_reg_wait_bmp(0x%08x)\n",
        llid_bmp, __llid_reg_wait_bmp);
    return CA_E_OK;
}

ca_status_t aal_mpcp_reg_llid_del(ca_device_id_t  device_id, ca_uint32_t llid_bmp)
{
    __AAL_MPCP_REG_LOCK();
    __llid_reg_wait_bmp &= ~llid_bmp;
    __AAL_MPCP_REG_UNLOCK();
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: remove llid_bmp(0x%08x) from waiting list, __llid_reg_wait_bmp(0x%08x)\n",
        llid_bmp, __llid_reg_wait_bmp);
    return CA_E_OK;
}

void aal_mpcp_reg_handler(void *data)
{
    ca_uint32_t i = 0;
    //ca_boolean_t link = FALSE;
    aal_epon_status_t status;
    ca_boolean_t reg_status = FALSE;

	ca_boolean_t silence_status = FALSE;

    if (__llid_reg_wait_bmp == 0)
        return;

    __PON_LOCK();
    //aal_epon_mac_xg_link_status_get(0, 0, &link);
    (void)aal_epon_status_get(0, 0, &status);
    __PON_UNLOCK();

    //if (link == 0)  /* PON link down */
    if ((!status.pcs_sync_xg && (status.speed > AAL_EPON_SPEED_MODE_DS_1G_US_1G))
            || (!status.pcs_sync_1g && (status.speed <= AAL_EPON_SPEED_MODE_DS_1G_US_1G)))
        return;

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: aal_mpcp_reg_handler() - __llid_reg_wait_bmp(0x%08x)\n", __llid_reg_wait_bmp);

    for (i=0; i<__llid_num; i++)
    {
        if (__llid_reg_wait_bmp & (1 << i))
        {
			silence_status = 0;
			aal_mpcp_llid_silence_status_get(0, i, &silence_status);
			if(silence_status == 1)
				continue;
            aal_mpcp_reg_status_get(0, 0, i, &reg_status);

            if (!reg_status)
            {
                __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: aal_mpcp_reg_handler() - start mpcp registration for llid_idx(%02d)\n", i);
                aal_mpcp_reg_start_set(0, 0, i, i);
            }
            else
            {
                __AAL_MPCP_REG_LOCK();
                __llid_reg_wait_bmp &= ~(1<<i);
                __AAL_MPCP_REG_UNLOCK();
            }

            return;
        }
    }

}

static ca_status_t __mpcp_keep_silence_stop(ca_uint8_t llid_idx)
{
	ca_event_silence_stat_t silence_stat;
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: stop silence timer for llid_idx(%02d), timer_id(%d)\n",
            llid_idx, __keep_silence[llid_idx].timer_id);

    ca_timer_del(__keep_silence[llid_idx].timer_id);
    __keep_silence[llid_idx].timer_id = 0;
	__keep_silence[llid_idx].status = 0;

	memset(&silence_stat, 0, sizeof(ca_event_silence_stat_t));
	silence_stat.llid_idx = llid_idx;
	silence_stat.status = __keep_silence[llid_idx].status;
	ca_event_send(0, CA_EVENT_EPON_SILENCE_STATUS, (void *)&silence_stat, sizeof(ca_event_silence_stat_t));
    return CA_E_OK;
}

void aal_mpcp_keep_silence_timeout_handler(void *llid_idx)
{
    ca_uint8_t idx = (long)llid_idx & 0xff;

    if ((idx) > (AAL_EPON_LLID_NUM_MAX-1))
        return;

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: silence timeout - restart registration for llid_idx(%02d)\n", idx);

    __AAL_MPCP_LOCK(0);
    __mpcp_keep_silence_stop(idx);
    __AAL_MPCP_UNLOCK(0);

    aal_mpcp_reg_llid_add(0, idx);
}

static ca_status_t __mpcp_keep_silence_start(ca_uint8_t llid_idx)
{
	ca_event_silence_stat_t silence_stat;

    AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_START_CFG, mpcp_start, 0);

	__keep_silence[llid_idx].status = 1;

	if(__keep_silence[llid_idx].timer_id != 0)
	{
		__EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "EPON-MPCP: re-start silence timer for llid_idx %d. fisrt stop timer_id = %d\n",
				llid_idx, __keep_silence[llid_idx].timer_id);

		ca_timer_del(__keep_silence[llid_idx].timer_id);
		__keep_silence[llid_idx].timer_id = 0;
	}
    __keep_silence[llid_idx].timer_id = ca_timer_add(__keep_silence[llid_idx].timer_value, aal_mpcp_keep_silence_timeout_handler, (void*)llid_idx);
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: start silence timer for llid_idx %d. timer_value = %d, timer_id = %d\n",
                llid_idx, __keep_silence[llid_idx].timer_value, __keep_silence[llid_idx].timer_id);

    if (__keep_silence[llid_idx].timer_id == 0)
    {
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: keep silence timer add failed for llid_idx %d\n", llid_idx);
		__keep_silence[llid_idx].status = 0;
    }
	memset(&silence_stat, 0, sizeof(ca_event_silence_stat_t));
	silence_stat.llid_idx = llid_idx;
	silence_stat.status = __keep_silence[llid_idx].status;
	ca_event_send(0, CA_EVENT_EPON_SILENCE_STATUS, (void *)&silence_stat, sizeof(ca_event_silence_stat_t));
    return CA_E_OK;
}

static void __mpcp_llid_reg_event_send(ca_uint8_t llid_idx, ca_boolean_t reg_status)
{
    ca_uint16_t llid = 0xff;
    ca_mac_addr_t olt_mac;
    ca_mac_addr_t onu_mac;
    ca_event_reg_stat_t reg_event;
    aal_mpcp_dereg_cause_t cause = AAL_MPCP_DEREG_UNKNOWN;
    ca_int8_t *str_cause[] =
    {
        "no deregistration",
        "time drift",
        "remote deregistration",
        "local deregistration",
        "RE-REG flag received",
        "MPCP timeout",
        "GATE timeout",
        "software dergistration"
    };

    memset((void *)&olt_mac[0], 0, sizeof(olt_mac));
    memset((void *)&onu_mac[0], 0, sizeof(onu_mac));

    //ca_printf("aal_epon_mpcp_llid_reg_event llid_idx %d, status %d\r\n", llid_idx, reg_status);
    aal_mpcp_reg_llid_get(0, 0, llid_idx, &llid);
    aal_mpcp_mac_addr_get(0, 0, llid_idx, onu_mac);

    if (reg_status) /* LLID is registered */
    {
        aal_mpcp_olt_mac_addr_get(0, 0, olt_mac);
        #if 0
        aal_crypto_xg_mode_get(0, 0, &crypto_mode);
        if (CA_CRYPTO_XG_MODE_AES == crypto_mode)
        {
            aal_mpcp_pcs_olt_mac_addr_set(0,0,olt_mac);
            aal_crypto_mpcp_time_adjust_set(0,0,0x13);
        }
        #endif
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: ONU-REG, llid(%05d)--llid_idx(%02d)--MAC(%02x-%02x-%02x-%02x-%02x-%02x) was registered to OLT(%02x-%02x-%02x-%02x-%02x-%02x)\n",
            llid,
            llid_idx,
            onu_mac[0], onu_mac[1], onu_mac[2], onu_mac[3], onu_mac[4], onu_mac[5],
            olt_mac[0], olt_mac[1], olt_mac[2], olt_mac[3], olt_mac[4], olt_mac[5]);
        __aal_epon_crypto_pcs_xg_olt_mac_cfg_set();
    }
    else /* LLID is de-registered */
    {
        aal_mpcp_dereg_cause_get(0, 0, llid_idx, &cause);
        #if 0
        /* clear llid encryption valid when deregister */
        cs_aal_crypto_xg_enable_set(0, 0, llid_idx, FALSE);
        #endif
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: ONU-DE-REG, llid(%05d)--llid_idx(%02d)--MAC(%02x-%02x-%02x-%02x-%02x-%02x) was de-registered due to %s.\n",
            llid, llid_idx,
            onu_mac[0], onu_mac[1], onu_mac[2], onu_mac[3], onu_mac[4], onu_mac[5], str_cause[cause]);
    }

    memset(&reg_event, 0, sizeof(ca_event_reg_stat_t));
    reg_event.llid_idx   = llid_idx;
    reg_event.llid_value = llid;
    reg_event.status     = reg_status;
    ca_event_send(0, CA_EVENT_EPON_REG_STAT_CHNG, (void *)&reg_event, sizeof(ca_event_reg_stat_t));
}

ca_status_t __mpcp_rx_register_frame_get(ca_uint8 *llid_idx, ca_uint8_t *reg_flag)
{
    ca_status_t ret = CA_E_OK;
    EPON_XGONU_MPCP_MPCP_REGISTER_FRAME_STS_t frame;
    //EPON_XGONU_MPCP_MPCP_PKT_FLAG_t flag;

    frame.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_REGISTER_FRAME_STS);

    if (frame.bf.vld)
    {
        __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: register frame sts. vld = %d, assigned_llid = %d, flag = %d, mac_index = %d\n",
            frame.bf.vld,frame.bf.assigned_llid, frame.bf.flag, frame.bf.mac_index);
        *llid_idx = frame.bf.mac_index;
        *reg_flag = frame.bf.flag;
    }
    else
    {
        return CA_E_ERROR;
    }

    return ret;
}

void __mpcp_llid_entry_del(ca_uint32_t llid_bmp)
{
    ca_uint32_t i = 0;
    ca_uint16_t llid = 0xff;
    aal_mpcp_llid_entry_t entry;

    for (i=0; i<AAL_EPON_LLID_NUM_MAX; i++)
    {
        if ((llid_bmp & (1 << i)) != 0)
        {
            if (aal_mpcp_reg_llid_get(0, 0, i, &llid) != CA_E_OK)
                continue;

            memset(&entry, 0, sizeof(entry));
            entry.llid_idx   = i;
            entry.llid_value = llid;
            entry.mac_idx    = i;
            entry.mcast      = 0;
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: llid(%05d)--llid_idx(%02d) entry was deleted.\n",
                llid, i);
            aal_mpcp_llid_entry_set(0, 0, AAL_MPCP_LLID_ENTRY_DEL, &entry);
        }
    }
}

void __mpcp_reg_status_change_llid_get(ca_uint32_t *llid_reg_bmp, ca_uint32_t *llid_dereg_bmp)
{
    ca_uint8_t i = 0;
    ca_uint32_t new_status = 0;
    ca_uint32_t old_status = 0;
    EPON_XGONU_MPCP_MAC_REG_STATUS_t reg_status;

    *llid_reg_bmp = 0;
    *llid_dereg_bmp = 0;

    reg_status.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MAC_REG_STATUS);
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: EPON_XGONU_MPCP_MAC_REG_STATUS(0x%08x), previous status(0x%08x)\n",
        reg_status.wrd, __llid_reg_status_bmp);

    if (reg_status.wrd == __llid_reg_status_bmp)
        return;

    for (i=0; i<__llid_num; i++)
    {
        new_status = (reg_status.wrd) & (1<<i);
        old_status = __llid_reg_status_bmp & (1<<i);
        if (new_status != old_status)
        {
            if (new_status != 0)
                *llid_reg_bmp |= (1 << i);
            else
                *llid_dereg_bmp |= (1 << i);
        }
    }

    __llid_reg_status_bmp = reg_status.wrd;
}

/*
Once ONU receives one MPCP Register frame, it will extract some major information and, if the flag is not NACK, push
them into a 32-entry FIFO. Every time software reads out this register, it actually reads out the current FIFO value
and then updates FIFO after the read. Since Register frames with NAC flag are filtered out, you cannot see the
NACK flag from this CSR, *REGISTER_FRAME_STS.

So keep silence feature can't support yet.
*/
void aal_epon_mpcp_reg_change_handler(ca_uint32_t cookie)
{
    ca_uint8_t i = 0;
    ca_uint8_t reg_flag = 0;
    ca_uint32_t llid_reg_bmp = 0;
    ca_uint32_t llid_dereg_bmp = 0;
    ca_uint32_t dereg_cause = 0;

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: REG-STATUS changed event - cookie(%d)\n", cookie);

    /* 1. get reg status change llid, reg_status, dereg_cause, and do keep silence if need */
    __mpcp_reg_status_change_llid_get(&llid_reg_bmp, &llid_dereg_bmp);
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: status was changed - llid_reg_bmp(0x%08x), llid_dereg_bmp(0x%08x)\n", llid_reg_bmp, llid_dereg_bmp);

    dereg_cause = cookie;

    /* 2. update llid_reg_wait, update dereg_cause, send llid reg event to APP */
    if (llid_reg_bmp != 0)
    {
        aal_mpcp_reg_llid_del(0, llid_reg_bmp); /* remove new registered bitmap from waitting list */

        for (i=0; i<__llid_num; i++)
        {
            if ((llid_reg_bmp & (1 << i)) != 0)
            {
#if 0
                __AAL_MPCP_LOCK(0);
                if(__keep_silence[llid_idx].timer_id != 0){
                    __mpcp_keep_silence_stop(llid_idx);
                }
                __AAL_MPCP_UNLOCK(0);
#endif
                __AAL_MPCP_LOCK(0);
                /* dereg_cause will be valid once the llid was registered */
                __dereg_cause[i] = AAL_MPCP_DEREG_UNKNOWN;
                __AAL_MPCP_UNLOCK(0);

                __mpcp_llid_reg_event_send(i, TRUE);
            }
        }
    }

    if (llid_dereg_bmp != 0)
    {
#if 0
        if(reg_flag == 4 && (__keep_silence[llid_idx].enable)){ /* LLIS needs keep silence */
            /* start keep silence */
            __AAL_MPCP_LOCK(0);
            __mpcp_keep_silence_start(llid_idx);
            __AAL_MPCP_UNLOCK(0);
        }
#endif

        /* report to upper application */
        for (i=0; i<__llid_num; i++)
        {
            if ((llid_dereg_bmp & (1 << i)) != 0)
            {
                /* if it is not 0, it has been set by API which can cause LLID de-register */
                __AAL_MPCP_LOCK(0);
                if (__dereg_cause[i] != AAL_MPCP_DEREG_SW)
                    __dereg_cause[i] = dereg_cause;

                __AAL_MPCP_UNLOCK(0);

                __mpcp_llid_reg_event_send(i, FALSE);
            }
        }

        /* llid entry should be deleted by SW when received NACK, timeout or ts drift */
        if (reg_flag == 4 ||
            cookie == AAL_MPCP_DEREG_GATE || cookie == AAL_MPCP_DEREG_TIMEOUT || cookie == AAL_MPCP_DEREG_TIMEDRFT)
        {
            __mpcp_llid_entry_del(llid_dereg_bmp);
        }

        if ((__llid_reg_mode == AAL_EPON_LLID_REG_MODE_AUTO))
        {
            aal_mpcp_reg_llid_add(0, llid_dereg_bmp);
        }
        else if ((__llid_reg_mode == AAL_EPON_LLID_REG_MODE_OAM))
        { /* only enable llid#0 */
            if (llid_dereg_bmp & 1)
                aal_mpcp_reg_llid_add(0, 1);
        }

        #if 0
        for (i=0; i<__llid_num; i++)
        {
            if ((llid_dereg_bmp & (1<<i)) != 0)
            {
                /* if it is not 0, it has been set by API which can cause LLID de-register */
                __AAL_MPCP_LOCK(0);
                if (__dereg_cause[i] != AAL_MPCP_DEREG_SW)
                    __dereg_cause[i] = dereg_cause;

                __AAL_MPCP_UNLOCK(0);

                __mpcp_llid_reg_event_send(i, FALSE);
            }
        }
        #endif
    }

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: llid registration status bitmap 0x%x\n", __llid_reg_status_bmp);
}

#if 0
void aal_epon_mpcp_reg_change_handler(ca_uint32_t cookie)
{
    ca_status_t ret;
    ca_uint8_t i = 0;
    ca_uint8_t llid_idx = 0;
    ca_uint8_t reg_flag = 0;
    ca_uint16_t llid = 0xff;
    ca_uint32_t llid_reg_bmp = 0;
    ca_uint32_t llid_dereg_bmp = 0;
    ca_uint32_t dereg_cause = 0;
    EPON_XGONU_MPCP_MPCP_PKT_FLAG_t flag;

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: REG-STATUS changed event. cookie %d\r\n", cookie);

    /* 1. get reg status change llid, reg_status, dereg_cause, and do keep silence if need */

    if(cookie == AAL_MPCP_DEREG_OLT){ /* reg status changed cause by receiving regstration packets */

        /* read REGISTER FARME registers multi-times, in case reg_status_changei missed */
        /* for example: 1. deregister llids by OLT commands at one time: deregister <llid_value> <llid_num> 2 */
        /* for example: 2. change tx polarity disable, the ONU will be deregistered by OLT */
        while(__mpcp_rx_register_frame_get(&llid_idx, &reg_flag) == CA_E_OK){

            if(reg_flag == 3){

                dereg_cause = AAL_MPCP_DEREG_UNKNOWN;

                __AAL_MPCP_LOCK(0);
                if(__keep_silence[llid_idx].timer_id != 0){
                    __mpcp_keep_silence_stop(llid_idx);
                }
                __AAL_MPCP_UNLOCK(0);

                llid_reg_bmp |= (1<<llid_idx);
            }
            else if(reg_flag == 2 || reg_flag == 4){

                dereg_cause = AAL_MPCP_DEREG_OLT;

                if(reg_flag == 4 && (__keep_silence[llid_idx].enable)){ /* LLIS needs keep silence */
                    /* start keep silence */
                    __AAL_MPCP_LOCK(0);
                    __mpcp_keep_silence_start(llid_idx);
                    __AAL_MPCP_UNLOCK(0);
                }
                llid_dereg_bmp |= (1<<llid_idx);
            }
        }

    }
    else{/* reg status changed cause by timeout or ts drift */

        if(cookie == AAL_MPCP_DEREG_GATE)/* GATE TIMER EXPIRE */
        {
            llid_dereg_bmp = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_GATE_TIMER_STS);
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: gate timer timeout. bitmap = 0x%x\n", llid_dereg_bmp);
        }
        if(cookie == AAL_MPCP_DEREG_TIMEOUT)/* MPCP TIMER EXPIRE & TIMESTAMP DRIFT */
        {
            llid_dereg_bmp = __llid_reg_status_bmp;
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: mpcp timeout. bitmap = 0x%x\n", llid_dereg_bmp);
        }
        if(cookie == AAL_MPCP_DEREG_TIMEDRFT)/* MPCP TIMER EXPIRE & TIMESTAMP DRIFT */
        {
            llid_dereg_bmp = __llid_reg_status_bmp;
            __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: timestamp drift. bitmap = 0x%x\n", llid_dereg_bmp);

        }

        dereg_cause = cookie;

    }

    /* 2. update llid_reg_wait, update dereg_cause, send llid reg event to APP */
    if(llid_reg_bmp != 0){
        aal_mpcp_reg_llid_del(0, llid_reg_bmp);
        __llid_reg_status_bmp |= llid_reg_bmp;

        for (i=0; i<__llid_num; i++){
            if ((llid_reg_bmp & (1<<i)) != 0){
                /* if it is not 0, it has been set by API which can cause LLID de-register */
                __AAL_MPCP_LOCK(0);
                __dereg_cause[i] = AAL_MPCP_DEREG_UNKNOWN;
                __AAL_MPCP_UNLOCK(0);

                __mpcp_llid_reg_event_send(i, TRUE);
            }
        }
    }

    if(llid_dereg_bmp != 0){

        /* llid entry should be deleted by SW when received NACK, timeout or ts drift */
        if(reg_flag == 4 ||
            cookie == AAL_MPCP_DEREG_GATE || cookie == AAL_MPCP_DEREG_TIMEOUT || cookie == AAL_MPCP_DEREG_TIMEDRFT){

            __mpcp_llid_entry_del(llid_dereg_bmp);
        }

        aal_mpcp_reg_llid_add(0, llid_dereg_bmp);

        __llid_reg_status_bmp &= (~llid_dereg_bmp);

        for (i=0; i<__llid_num; i++){
            if ((llid_dereg_bmp & (1<<i)) != 0){
                /* if it is not 0, it has been set by API which can cause LLID de-register */
                __AAL_MPCP_LOCK(0);
                if(__dereg_cause[i] != AAL_MPCP_DEREG_SW){
                    __dereg_cause[i] = dereg_cause;
                }
                __AAL_MPCP_UNLOCK(0);

                __mpcp_llid_reg_event_send(i, FALSE);
            }
        }
    }

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "ca-mpcp: __llid_reg_status_bmp = 0x%x\r\n\r\n", __llid_reg_status_bmp);
}
#endif

void aal_epon_mpcp_timeout_handler(ca_uint32_t cookie)
{
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: mpcp timer expired.\n");
}

void aal_epon_mpcp_gate_timeout_handler(ca_uint32_t cookie)
{
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: gate timer expired.\n");
}

void aal_epon_mpcp_timestamp_drift_handler(ca_uint32_t cookie)
{
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: timestamp drift detected.\n");
}

/*only support llid 0 */
void aal_epon_mpcp_rx_reg_frame_handler(ca_uint32_t cookie)
{
    EPON_XGONU_MPCP_MPCP_PKT_FLAG_t flag;

    flag.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_PKT_FLAG);
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_LOG, "EPON_XGONU_MPCP_MPCP_PKT_FLAG: ack_flag = %d, request_flag = %d, register_flag = %d\n",
        flag.bf.ack_flag,flag.bf.request_flag,flag.bf.register_flag);

    if(flag.bf.register_flag == 4 && (__keep_silence[0].enable)){
        /* start keep silence */
        __AAL_MPCP_LOCK(0);
        __mpcp_keep_silence_start(0);
        __AAL_MPCP_UNLOCK(0);
    }

    if(flag.bf.register_flag == 3){
        __AAL_MPCP_LOCK(0);
            if(__keep_silence[0].timer_id != 0){
                __mpcp_keep_silence_stop(0);
        }
        __AAL_MPCP_UNLOCK(0);
    }
}


ca_uint8_t aal_mpcp_reg_mode_get(CA_IN ca_device_id_t  device_id)
{
    ca_uint32_t ret = 0;
    ca_uint8_t reg_mode = AAL_EPON_LLID_REG_MODE_AUTO;

    ret = scfg_read(0, CFG_ID_EN_REGISTER_AFTER_BOOT, sizeof(reg_mode), (ca_uint8_t *)&reg_mode);
    if(ret == 0){
        return reg_mode;
    }
    return AAL_EPON_LLID_REG_MODE_AUTO;
}

ca_uint8_t aal_mpcp_llid_num_get(CA_IN ca_device_id_t  device_id)
{
    ca_uint32_t ret = 0;
    ca_uint8_t llid_num = AAL_EPON_LLID_NUM_MAX;

    ret = scfg_read(0, CFG_ID_PON_LLID_NUM, sizeof(llid_num), (ca_uint8_t *)&llid_num);
    if (ret == 0)
        return llid_num;

    return AAL_EPON_LLID_NUM_MAX;
}

ca_status_t aal_mpcp_init
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id
)
{
    ca_status_t   rtn  = CA_E_ERROR;
    ca_uint32_t   llid = 0;
    ca_uint8      i = 0;
    ca_mac_addr_t onu_mac;
    ca_uint8_t    llid_idx = 0;
    aal_mpcp_llid_cfg_t llid_cfg;
    aal_mpcp_llid_msk_t llid_msk;

#if 0//defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN)
    /* increase laser ON/OFF time to 0x30 TQ for FPGA verification */
    EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG_t lsr_time_cfg;

    lsr_time_cfg.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG);
    lsr_time_cfg.bf.laserOnTime_cfg  = 0x30;
    lsr_time_cfg.bf.laserOffTime_cfg = 0x30;
    AAL_PON_REG_WRITE(lsr_time_cfg.wrd, EPON_XGONU_MPCP_MPCP_LASER_TIME_CFG);
#endif

    memset(__dereg_cause, 0, sizeof(__dereg_cause));
    memset(__keep_silence, 0, sizeof(__keep_silence));

    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: being initialized ... \n");

    if (!__aal_mpcp_lock)
        ca_spin_lock_init(&__aal_mpcp_lock, SPINLOCK_BH);

    if (!__aal_mpcp_reg_lock)
        ca_spin_lock_init(&__aal_mpcp_reg_lock, SPINLOCK_BH);

    __llid_num = aal_mpcp_llid_num_get(0);
    __llid_reg_mode = aal_mpcp_reg_mode_get(0);

    ca_circle_timer_add(1000, aal_mpcp_reg_handler, NULL);

    /* 1. set default MPCP configure */
    aal_mpcp_misc_init(device_id, port_id);

    /* 2. set ONU PON mac address from start configure file */
    memset(&onu_mac, 0, sizeof(ca_mac_addr_t));
    if (scfg_read(0, CFG_ID_MAC_ADDRESS, CA_ETH_ADDR_LEN, onu_mac) != CA_E_OK)
        return CA_E_ERROR;
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "ca-mpcp: ONU-MAC(%02x:%02x:%02x:%02x:%02x:%02x)\n",
        onu_mac[0], onu_mac[1], onu_mac[2], onu_mac[3], onu_mac[4], onu_mac[5]);

    // for saturn, only need to config the basic MAC addr
    if ((rtn = aal_mpcp_mac_addr_set(device_id, port_id, 0, onu_mac)) != CA_E_OK)
        goto mpcp_init_exit;

    /* enable LLID 0 and disable LLID 1 ~ 31 by default */
    memset(&llid_msk, 0, sizeof(llid_msk));
    memset(&llid_cfg, 0, sizeof(llid_cfg));
    llid_msk.s.register_permit = 1;

    for (llid_idx = 0; llid_idx < AAL_EPON_LLID_NUM_MAX; llid_idx++)
    {
        llid_cfg.register_permit = 1; //((llid_idx) ? 0 : 1);
        (void)aal_mpcp_llid_cfg_set(0, 0, llid_idx, llid_msk, &llid_cfg);
    }

    /* initialize software MC llid table */
    for (i=0; i<AAL_MPCP_MAX_MC_LLID_NUMS; i++)
    {
        __mc_llid[i].valid = CA_FALSE;
        __mc_llid[i].llid  = AAL_MPCP_BC_LLID;
    }


mpcp_init_exit:
    __EPON_MPCP_LOG(AAL_EPON_MPCP_TRACE_CRI, "%s\n", (rtn == CA_E_OK) ? "ca-mpcp: initialization was done." : "ca-mpcp: initialization was failed.");

    return CA_E_OK;
}

void aal_mpcp_state_dump(ca_device_id_t  device_id)
{
    ca_uint8_t  i = 0;
    ca_uint16_t llid = 0;
    ca_boolean_t  reg_status = 0;
    ca_mac_addr_t mac_addr;

    for (i=0; i<AAL_EPON_LLID_NUM_MAX; i++)
    {
        if ((1 << i))
        {
            aal_mpcp_reg_status_get(0, 0, i, &reg_status);
            if (reg_status)
            {
                aal_mpcp_reg_llid_get(0, 0, i, &llid);
                aal_mpcp_mac_addr_get(0, 0, i, mac_addr);
                ca_printf("LLID-IDX(%02d) LLID(%05d) MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                    i, llid,
                    mac_addr[0],mac_addr[1],mac_addr[2],
                    mac_addr[3], mac_addr[4],mac_addr[5]);
            }
        }
    }
}
EXPORT_SYMBOL(aal_mpcp_state_dump);

#if 0
EXPORT_SYMBOL(aal_mpcp_register_mode_set);
EXPORT_SYMBOL(aal_mpcp_register_mode_get);
EXPORT_SYMBOL(aal_mpcp_register_frame_get);
#endif

EXPORT_SYMBOL(aal_mpcp_mc_llid_add);
EXPORT_SYMBOL(aal_mpcp_mc_llid_del);
EXPORT_SYMBOL(aal_mpcp_mc_llid_list);
EXPORT_SYMBOL(aal_mpcp_debug_set);
EXPORT_SYMBOL(aal_mpcp_debug_get);
EXPORT_SYMBOL(aal_mpcp_cfg_set);
EXPORT_SYMBOL(aal_mpcp_cfg_get);
EXPORT_SYMBOL(aal_mpcp_rx_ext_ctrl_set);
EXPORT_SYMBOL(aal_mpcp_rx_ext_ctrl_get);
EXPORT_SYMBOL(aal_mpcp_llid_cfg_set);
EXPORT_SYMBOL(aal_mpcp_llid_cfg_get);
EXPORT_SYMBOL(aal_mpcp_llid_mode_set);
EXPORT_SYMBOL(aal_mpcp_llid_mode_get);
EXPORT_SYMBOL(aal_mpcp_mac_addr_set);
EXPORT_SYMBOL(aal_mpcp_mac_addr_get);
EXPORT_SYMBOL(aal_mpcp_report_threshold_set);
EXPORT_SYMBOL(aal_mpcp_report_threshold_get);
EXPORT_SYMBOL(aal_mpcp_reg_start_set);
EXPORT_SYMBOL(aal_mpcp_reg_start_get);
EXPORT_SYMBOL(aal_mpcp_reg_toggle);
EXPORT_SYMBOL(aal_mpcp_reg_llid_get);
EXPORT_SYMBOL(aal_mpcp_llid_index_get);
EXPORT_SYMBOL(aal_mpcp_reg_llid_index_get);
EXPORT_SYMBOL(aal_mpcp_cmd_llid_entry_set);
EXPORT_SYMBOL(aal_mpcp_cmd_llid_entry_get);
EXPORT_SYMBOL(aal_mpcp_llid_entry_all_get);
EXPORT_SYMBOL(aal_mpcp_reg_status_get);
EXPORT_SYMBOL(aal_mpcp_reg_flag_get);
EXPORT_SYMBOL(aal_mpcp_dereg_cause_get);
EXPORT_SYMBOL(aal_mpcp_local_time_get);
EXPORT_SYMBOL(aal_mpcp_reg_sync_time_get);
EXPORT_SYMBOL(aal_mpcp_reg_laser_time_status_get);
EXPORT_SYMBOL(aal_mpcp_reg_retry_times_get);
EXPORT_SYMBOL(aal_mpcp_local_discovery_info_get);
EXPORT_SYMBOL(aal_mpcp_local_remote_info_get);
EXPORT_SYMBOL(aal_mpcp_mib_stats_get);
EXPORT_SYMBOL(aal_mpcp_pca_olt_mac_addr_set);
EXPORT_SYMBOL(aal_mpcp_pca_olt_mac_addr_get);
EXPORT_SYMBOL(aal_mpcp_olt_mac_addr_get);
EXPORT_SYMBOL(aal_epon_mpcp_reg_change_handler);
EXPORT_SYMBOL(aal_epon_mpcp_timeout_handler);
EXPORT_SYMBOL(aal_epon_mpcp_gate_timeout_handler);
EXPORT_SYMBOL(aal_epon_mpcp_timestamp_drift_handler);
EXPORT_SYMBOL(aal_epon_mpcp_rx_reg_frame_handler);
EXPORT_SYMBOL(aal_mpcp_misc_init);

EXPORT_SYMBOL(aal_mpcp_init);

ca_status_t aal_mpcp_llid_silence_status_get(ca_device_id_t device_id,ca_uint8_t llid_idx, ca_boolean_t *status)
{
    AAL_MPCP_LLID_CHECK(llid_idx);

    *status = __keep_silence[llid_idx].status;
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_mpcp_llid_silence_status_get);



