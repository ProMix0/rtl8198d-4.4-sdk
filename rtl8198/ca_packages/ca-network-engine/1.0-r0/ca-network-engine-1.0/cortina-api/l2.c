

#include "osal_common.h"
#include "aal_common.h"
#include <soc/cortina/registers.h>
#include "l2.h"
#include "aal_fdb.h"
#include "port.h"
#include "aal_port.h"
#include "ca-scfg/scfg.h"

ca_uint32_t l2_customer_info = 0;
extern ca_int32_t using_sw_lrn;

ca_uint g_l2_lock;
/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  macro
%%____________________________________________________________________________*/


#define __L2_LOCK    ca_spin_lock(g_l2_lock);
#define __L2_UNLOCK    ca_spin_unlock(g_l2_lock);


/*
	learn_enable                 L2E_CTRL.lrn_dis, global - always enable learning globally.

	dropped_pkt_learn_enable        - global, just ignore port parameter

	cpu_special_packet_learn_enable
	To CPU packet learning enable/disable: this is supported in G3 HW (using special packet learning control)
	L2FE_PLA_SPB_DATA. spb_learn_dis (in most cases, to CPU packets are trapped and forwarded by special packet HW module)

	sa_mac_table_full_policy  - L2FE_PLC_LRN_FWD_CTRL_1.sa_limit_excd_fwd_ctrl (this is implemented by SW)
	HW will always forward to CPU. CPU SW can check port learning enable/disable and decide to replace/install latest MAC DA/port.

	G3 also support per VLAN learning control (we don't have API for this currently)
	L2FE_VE_LAN_VL_FIB_DATA0. lan_vl_fib_stp_mode

 */
ca_status_t ca_l2_learning_control_set (
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN ca_l2_learning_control_t *control
)
{
    L2FE_PLC_L2_LEARNING_CTRL_t plc_l2_lrn_ctrl;
    L2FE_PLC_LRN_FWD_CTRL_1_t   plc_lrn_fwd_ctrl;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    ca_int32_t lpid;
    ca_int32_t value;
    ca_status_t ret = CA_E_OK;

    L2_ASSERT_RET(control != NULL, CA_E_PARAM);

    CA_PORT_PHYSICAL_CHECK(port_id);

    lpid = PORT_ID(port_id);

    L2_DEBUG("learn_enable:%d, dropped_pkt_learn_enable:%d, cpu_special_packet_learn_enable:%d, sa_mac_table_full_policy:%d\r\n",
            control->learn_enable, control->dropped_pkt_learn_enable, control->cpu_special_packet_learn_enable, control->sa_mac_table_full_policy);

    plc_l2_lrn_ctrl.wrd = CA_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
    plc_lrn_fwd_ctrl.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);

    /* learn control */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    igr_msk.s.stp_mode = 1;
    if (control->learn_enable)
        igr_cfg.stp_mode = AAL_STP_STATE_FWD_AND_LEARN;
    else
        igr_cfg.stp_mode = AAL_STP_STATE_FWD_NO_LEARN;

    ret = aal_port_ilpb_cfg_set(device_id, lpid, igr_msk, &igr_cfg);

    /* dropped_pkt_learn_enable */
    plc_l2_lrn_ctrl.bf.igr_dropped_pkt_no_learn = !control->dropped_pkt_learn_enable;

    switch(control->sa_mac_table_full_policy){
        case CA_L2_MAC_FULL_FWD_NO_LEARN:
            value = SA_LRN_FWD_CTRL_FORWARD;
            break;
        case CA_L2_MAC_FULL_DROP:
            value = SA_LRN_FWD_CTRL_DROP;
            break;
        case CA_L2_MAC_FULL_FWD_LRU:
            value = SA_LRN_FWD_CTRL_REDIRECT;
            break;
        default:
            break;
    }

    plc_lrn_fwd_ctrl.bf.sa_limit_excd_fwd_ctrl = value;

    FDB_REG_WRITE(L2FE_PLC_L2_LEARNING_CTRL, plc_l2_lrn_ctrl.wrd);
    FDB_REG_WRITE(L2FE_PLC_LRN_FWD_CTRL_1, plc_lrn_fwd_ctrl.wrd);

    aal_fdb_cpu_pkt_lrn_set (device_id, control->cpu_special_packet_learn_enable);

    return CA_E_OK;
}

ca_status_t ca_l2_learning_control_get (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_OUT ca_l2_learning_control_t *control
)
{
    L2FE_PLC_L2_LEARNING_CTRL_t plc_l2_lrn_ctrl;
    L2FE_PLC_LRN_FWD_CTRL_1_t   plc_lrn_fwd_ctrl;
    aal_stp_state_t igr_state = AAL_STP_STATE_FWD_AND_LEARN;
    aal_ilpb_cfg_t igr_cfg;
    ca_int32 lpid;
    ca_int32_t value;
    ca_status_t ret = CA_E_OK;

    L2_ASSERT_RET(control != NULL, CA_E_PARAM);

    CA_PORT_PHYSICAL_CHECK(port_id);

    lpid = PORT_ID(port_id);

    plc_l2_lrn_ctrl.wrd = CA_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
    plc_lrn_fwd_ctrl.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);

    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &igr_cfg);
    if (ret != CA_E_OK) {
        return ret;
    }
    igr_state = igr_cfg.stp_mode;

    control->learn_enable = (igr_state == AAL_STP_STATE_FWD_AND_LEARN ? 1 : 0);
    control->dropped_pkt_learn_enable = !plc_l2_lrn_ctrl.bf.igr_dropped_pkt_no_learn;

    switch(plc_lrn_fwd_ctrl.bf.sa_limit_excd_fwd_ctrl){
        case SA_LRN_FWD_CTRL_FORWARD:
            value = CA_L2_MAC_FULL_FWD_NO_LEARN;
            break;
        case SA_LRN_FWD_CTRL_DROP:
            value = CA_L2_MAC_FULL_DROP;
            break;
        case SA_LRN_FWD_CTRL_REDIRECT:
            value = CA_L2_MAC_FULL_FWD_LRU;
            break;
        default:
            break;
    }

    control->sa_mac_table_full_policy = value;

    aal_fdb_cpu_pkt_lrn_get (device_id, &control->cpu_special_packet_learn_enable);

    L2_DEBUG("learn_enable:%d, dropped_pkt_learn_enable:%d, cpu_special_packet_learn_enable:%d, sa_mac_table_full_policy:%d\r\n",
            control->learn_enable, control->dropped_pkt_learn_enable, control->cpu_special_packet_learn_enable, control->sa_mac_table_full_policy);

    return CA_E_OK;
}


ca_status_t ca_l2_learning_mode_set (
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_l2_learning_mode_t mode
)
{
    L2FE_L2E_CTRL_t           l2e_ctrl;

    L2_ASSERT_RET(mode < CA_L2_LEARNING_MAX, CA_E_PARAM);

    __L2_LOCK

    l2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);

    switch (mode) {
        case CA_L2_LEARNING_DISABLE:
            l2e_ctrl.bf.lrn_dis = 1;
            break;

        case CA_L2_LEARNING_SOFTWARE:
            l2e_ctrl.bf.lrn_dis = 0;
            l2e_ctrl.bf.lrn_mode = CA_AAL_FDB_LRN_MODE_SW;
            break;

        case CA_L2_LEARNING_HARDWARE:
            l2e_ctrl.bf.lrn_dis = 0;
            l2e_ctrl.bf.lrn_mode = CA_AAL_FDB_LRN_MODE_HW;
            break;

        default:
            break;
    }

    FDB_REG_WRITE(L2FE_L2E_CTRL, l2e_ctrl.wrd);

    __L2_UNLOCK

    return CA_E_OK;
}


ca_status_t ca_l2_learning_mode_get (
    CA_IN  ca_device_id_t             device_id,
    CA_OUT ca_l2_learning_mode_t   *mode)
{
    L2FE_L2E_CTRL_t           l2e_ctrl;

    L2_ASSERT_RET(mode != NULL, CA_E_PARAM);

    __L2_LOCK

    l2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);

    if (l2e_ctrl.bf.lrn_dis)
        *mode = CA_L2_LEARNING_DISABLE;
    else if (l2e_ctrl.bf.lrn_mode == CA_AAL_FDB_LRN_MODE_SW)
        *mode = CA_L2_LEARNING_SOFTWARE;
    else
        *mode = CA_L2_LEARNING_HARDWARE;

    __L2_UNLOCK

    return CA_E_OK;
}

ca_status_t ca_l2_aging_mode_set (
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_l2_aging_mode_t        mode)
{
    L2FE_L2E_CTRL_t           l2e_ctrl;

    L2_ASSERT_RET(mode == CA_L2_AGING_HARDWARE || mode == CA_L2_AGING_SOFTWARE, CA_E_PARAM);

    __L2_LOCK

    l2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);

    switch (mode) {
        case CA_L2_AGING_HARDWARE:
            l2e_ctrl.bf.aging_mode = 0;
            break;
        case CA_L2_AGING_SOFTWARE:
            l2e_ctrl.bf.aging_mode = 1;
            break;
        default:
            break;
    }

    FDB_REG_WRITE(L2FE_L2E_CTRL, l2e_ctrl.wrd);

    __L2_UNLOCK

    return CA_E_OK;

}

ca_status_t ca_l2_aging_mode_get (
    CA_IN ca_device_id_t            device_id,
    CA_OUT ca_l2_aging_mode_t       *mode)
{
    L2FE_L2E_CTRL_t           l2e_ctrl;

    L2_ASSERT_RET(mode != NULL, CA_E_PARAM);

    __L2_LOCK

    l2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);

    if (l2e_ctrl.bf.aging_mode)
        *mode = CA_L2_AGING_SOFTWARE;
    else
        *mode = CA_L2_AGING_HARDWARE;

    __L2_UNLOCK

    return CA_E_OK;

}
ca_status_t ca_l2_mac_limit_set (
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_l2_mac_limit_type_t type,
    CA_IN ca_uint32_t            data,
    CA_IN ca_uint32_t            number
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid;
    ca_int32 i;
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t max_data;

    max_data.wrd = ~0U;

    switch (type) {
        case CA_L2_MAC_LIMIT_PORT:
            lpid = PORT_ID(data);
            ret = aal_fdb_port_limit_set(device_id, lpid, number);
            break;
        case CA_L2_MAC_LIMIT_SYSTEM:
            if (/*using_sw_lrn && */(l2_customer_info & CUSTOMER_BIT_1)){
                /* using sw learn */
                aal_fdb_port_limit_set(device_id, CA_UINT16_INVALID, number);
            }
            else{
                for (i = CA_PORT_ID_NI0; i <= CA_PORT_ID_NI7; i++) {
                    aal_fdb_port_limit_set(device_id, i, number);
                }
            }
            break;
        case CA_L2_MAC_LIMIT_DISABLE:
            for (i = CA_PORT_ID_NI0; i <= CA_PORT_ID_NI7; i++) {
                aal_fdb_port_limit_set(device_id, i, max_data.bf.val);
            }
            break;
        case CA_L2_MAC_LIMIT_VLAN:
        default:
            ret = CA_E_NOT_SUPPORT;
            break;
    }


    return ret;
}


ca_status_t ca_l2_mac_limit_get (
    CA_IN  ca_device_id_t            device_id,
    CA_IN  ca_l2_mac_limit_type_t type,
    CA_IN  ca_uint32_t            data,
    CA_OUT ca_uint32_t           *number
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid;

    L2_ASSERT_RET(number != NULL, CA_E_PARAM);

    switch (type) {
        case CA_L2_MAC_LIMIT_PORT:
            lpid = PORT_ID(data);
            ret = aal_fdb_port_limit_get(device_id, lpid, number);
            break;
        case CA_L2_MAC_LIMIT_SYSTEM:
            /* get any of lpid */
            ret = aal_fdb_port_limit_get(device_id, 0, number);
            break;
        case CA_L2_MAC_LIMIT_DISABLE:
            *number = 0x1fff;
            break;
        case CA_L2_MAC_LIMIT_VLAN:
        default:
            ret = CA_E_NOT_SUPPORT;
            break;
    }

    return ret;
}

ca_uint32_t l2_aging_time = 300;
ca_status_t ca_l2_aging_time_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t time
)
{
    CA_ASSERT_RET(time <= CA_AAL_MAX_MAC_AGING_TIME, CA_E_PARAM);

    aal_fdb_aging_time_set(device_id, time);

    l2_aging_time = time;

    return CA_E_OK;
}


ca_status_t ca_l2_aging_time_get (
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t *time
)
{
    L2_ASSERT_RET(time != NULL, CA_E_PARAM);

    aal_fdb_aging_time_get(device_id, time);

    return CA_E_OK;
}


ca_status_t ca_l2_addr_add (
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_l2_addr_entry_t *addr
)
{
    ca_status_t ret;
    aal_fdb_entry_cfg_t fdb_entry;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    /* port_id sanity check */
    /*if ((addr->port_id == CA_UINT32_INVALID && addr->mc_group_id == CA_UINT32_INVALID) ||
        (addr->port_id != CA_UINT32_INVALID && addr->mc_group_id != CA_UINT32_INVALID)){
        ca_printf("port_id mc_group_id inconsistent\r\n");
        return CA_E_PARAM;
    }
    */
    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    memcpy(fdb_entry.mac, addr->mac_addr, sizeof(ca_mac_addr_t));
    fdb_entry.key_vid = addr->vid;
    fdb_entry.lrnt_vid = addr->fwd_vid;
    fdb_entry.dot1p = addr->dot1p;
    fdb_entry.static_flag = addr->static_flag;
    fdb_entry.sa_permit = addr->sa_permit;
    fdb_entry.da_permit = addr->da_permit;

    if (addr->mac_addr[0] & 0x01){
        fdb_entry.mc_flag = 1;
        fdb_entry.port_id = addr->mc_group_id;
    }
    else
        fdb_entry.port_id = PORT_ID(addr->port_id);
/*
    if (addr->port_id != CA_UINT32_INVALID && addr->mc_group_id == CA_UINT32_INVALID)
        fdb_entry.port_id = PORT_ID(addr->port_id);

    if (addr->port_id == CA_UINT32_INVALID && addr->mc_group_id != CA_UINT32_INVALID){
        fdb_entry.mc_flag = 1;
        fdb_entry.port_id = addr->mc_group_id;
    }
*/

    fdb_entry.aging_sts = ((addr->aging_timer > 0xff || addr->aging_timer == 0) ? 0xff : addr->aging_timer);

    ret = aal_fdb_entry_add(device_id, &fdb_entry);
    return ret;
}


ca_status_t ca_l2_addr_delete (
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_l2_addr_entry_t *addr
)
{
    ca_status_t ret;
    aal_fdb_entry_cfg_t fdb_entry;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    memcpy(fdb_entry.mac, addr->mac_addr, sizeof(ca_mac_addr_t));
    fdb_entry.key_vid = addr->vid;
    fdb_entry.lrnt_vid = addr->fwd_vid;
    fdb_entry.dot1p = addr->dot1p;
    fdb_entry.static_flag = addr->static_flag;
    fdb_entry.sa_permit = addr->sa_permit;
    fdb_entry.da_permit = addr->da_permit;
    fdb_entry.port_id = PORT_ID(addr->port_id);
    //fdb_entry. = addr->mc_group_id;
    fdb_entry.aging_sts = addr->aging_timer;

    ret = aal_fdb_entry_del(device_id, &fdb_entry);
    return ret;
}


ca_status_t ca_l2_addr_delete_by_port (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_l2_addr_operation_flags_t flag
)
{
    aal_fdb_cpu_cmd_word4_msk_t  flush_mask;
    aal_fdb_flush_cfg_t          flush_cfg;
    ca_status_t                  ret = CA_E_OK;
    ca_uint32_t                  aal_port = 0;

    CA_PORT_ID_CHECK(port_id);

    aal_port = PORT_ID(port_id);

    flush_mask.wrd = ~0U;

    switch (flag) {
    case CA_L2_ADDR_OPERATION_STATIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 1;
        break;
    case CA_L2_ADDR_OPERATION_DYNAMIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 0;
        break;
    case CA_L2_ADDR_OPERATION_BOTH:
        flush_mask.bf.static_flag = 1;
        break;
    default:
        return CA_E_PARAM;
    }

    flush_mask.bf.lpid  = 0;
    flush_cfg.lpid_high = aal_port;
    flush_cfg.lpid_low  = aal_port;

    ret = aal_fdb_scan_flush_field_set(device_id, flush_mask, &flush_cfg);

    if (CA_E_OK == ret)
        ret = aal_fdb_entry_flush(device_id);

    return ret;

}

ca_status_t ca_l2_addr_delete_by_mac (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_mac_addr_t         mac,
    CA_IN ca_l2_addr_operation_flags_t flag
)
{
    cs_aal_fdb_scan_rule_t  rule;
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_uint32 hit_addr = 0, start_addr = 0;
    ca_uint32 ret = 0, cmd_ret = 0;
//    ca_int32 item_cnt = 0;
    aal_fdb_flush_cfg_t cfg;
    aal_fdb_cpu_cmd_word4_msk_t  mask;

    L2_DEBUG("mac:0x%02x:%02x:%02x:%02x:%02x:%02x, flag:%d\r\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], flag);

    L2_ASSERT_RET(flag <= CA_L2_ADDR_OPERATION_OLDEST_DYNAMIC, CA_E_PARAM);


    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = 0xffffffff;
    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    memset(&rule, 0, sizeof(cs_aal_fdb_scan_rule_t));
    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    if (flag == CA_L2_ADDR_OPERATION_STATIC)
        rule.mode = CA_AAL_FDB_SCAN_MODE_STATIC;
    else
        rule.mode = CA_AAL_FDB_SCAN_MODE_DYNAMIC;

    while (1) {
        ret = aal_fdb_entry_scan(device_id,
                                start_addr, &hit_addr,
                                &fdb_entry, &cmd_ret);

        L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d, valid:%d, tmp:%d\r\n",
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id,
                fdb_entry.valid,
                fdb_entry.tmp_flag);

        if (ret != CA_E_OK || cmd_ret != CA_AAL_FDB_ACCESS_HIT) {
            L2_DEBUG("search end\r\n");
            break;
        }

        if (memcmp(fdb_entry.mac, mac, sizeof(ca_mac_addr_t)) == 0) {
            L2_DEBUG("find mac, check static flag:%d/%d\r\n", fdb_entry.static_flag, flag);
            if ((flag == CA_L2_ADDR_OPERATION_BOTH) ||
                (fdb_entry.static_flag == 1 && flag == CA_L2_ADDR_OPERATION_STATIC) ||
                (fdb_entry.static_flag == 0 && flag == CA_L2_ADDR_OPERATION_DYNAMIC)){

                ret = aal_fdb_entry_del(device_id, &fdb_entry);
                L2_DEBUG("ret:%d\r\n", ret);

                if (ret != CA_E_OK) {
                    L2_DEBUG("del entry fail\r\n");
                }
            }
            break;
        }
        //if (++item_cnt > 10) break;

        start_addr = hit_addr + 1;
    }

    return CA_E_OK;
}


ca_status_t ca_l2_addr_delete_by_vlan (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_vlan_id_t          vid,
    CA_IN ca_l2_addr_operation_flags_t flag
)
{
    aal_fdb_cpu_cmd_word4_msk_t  flush_mask;
    aal_fdb_flush_cfg_t          flush_cfg;
    ca_status_t                  ret = CA_E_OK;

    CA_VLAN_ID_CHECK(vid);

    flush_mask.wrd = ~0U;

    switch (flag) {
    case CA_L2_ADDR_OPERATION_STATIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 1;
        break;
    case CA_L2_ADDR_OPERATION_DYNAMIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 0;
        break;
    case CA_L2_ADDR_OPERATION_BOTH:
        flush_mask.bf.static_flag = 1;
        break;
    default:
        return CA_E_PARAM;
    }

    flush_mask.bf.key_vid  = 0;

    flush_cfg.key_vid_high = vid;
    flush_cfg.key_vid_low  = vid;

    ret = aal_fdb_scan_flush_field_set(device_id, flush_mask, &flush_cfg);

    if (CA_E_OK == ret)
        ret = aal_fdb_entry_flush(device_id);

    return ret;

}

extern ca_status_t aal_fdb_entry_2_l2_addr(
    CA_OUT ca_l2_addr_entry_t *addr,
    CA_IN aal_fdb_entry_cfg_t *fdb
);

ca_status_t ca_l2_addr_get (
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_mac_addr_t      mac,
    CA_IN  ca_vlan_id_t       key_vid,
    CA_OUT ca_l2_addr_entry_t *addr
)
{
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_status_t ret;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], key_vid);

    ret = aal_fdb_entry_get(device_id, mac, key_vid, &fdb_entry);


    if (ret == CA_E_OK) {
        ret = aal_fdb_entry_2_l2_addr(addr, &fdb_entry);
    }

    return ret;
}


ca_status_t ca_l2_addr_iterate (
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_iterator_t *return_entry
)
{
    cs_aal_fdb_scan_rule_t  rule;
    aal_fdb_entry_cfg_t  fdb_entry, *fdb_p = NULL;
    ca_l2_addr_entry_t addr, *entry = NULL;
    ca_uint32_t hit_addr = 0, start_addr = 0;
    ca_uint32_t ret = CA_E_OK, cmd_ret = 0;
//    static ca_uint32_t next_addr;
    ca_uint32_t entry_size = sizeof(ca_l2_addr_entry_t);
    aal_fdb_flush_cfg_t cfg;
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    long cast_tmp;
    int i = 0;

    CA_ASSERT_RET(return_entry != NULL && return_entry->entry_data != NULL, CA_E_PARAM);
    CA_ASSERT_RET(return_entry->entry_count, CA_E_PARAM);
    CA_ASSERT_RET(return_entry->entry_count * entry_size <= return_entry->user_buffer_size, CA_E_PARAM);

#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    entry = return_entry->entry_data;
    L2_DEBUG("entry->static_flag:%d\r\n", entry->static_flag);

    if (entry->static_flag & 0x10){
        if (aal_fdb_sw_lrn_cfg_get(0) == CA_E_OK){
            if (return_entry->prev_handle == NULL || return_entry->prev_handle == 0x1000)
                start_addr = 0;
            else
                start_addr = (ca_uint32)return_entry->prev_handle;

            L2_DEBUG("sw lrn cfged start_addr:%d\r\n", start_addr);

            while (i < return_entry->entry_count){
                fdb_p = aal_fdb_sw_lrn_static_mac_next(start_addr);
                if (fdb_p){
                    aal_fdb_entry_2_l2_addr(&addr, fdb_p);
                    memcpy(return_entry->entry_data + entry_size * i, (void *)&addr, entry_size);
                    //return_entry->entry_data += entry_size;
                    i++;
                    L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x\r\n",
                            fdb_p->mac[0], fdb_p->mac[1], fdb_p->mac[2], fdb_p->mac[3], fdb_p->mac[4], fdb_p->mac[5]);

                }else{
                    L2_DEBUG("search end\r\n");
                    ret = CA_E_UNAVAIL;
                    break;
                }
                start_addr++;
            }
            return_entry->entry_count = i;
            return_entry->prev_handle = (void*)start_addr;

            if (i == 0)
                return CA_E_UNAVAIL;

            return CA_E_OK;
        }

    }
#endif

    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = 0xffffffff;
    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    memset(&rule, 0, sizeof(cs_aal_fdb_scan_rule_t));
    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));
    memset(&addr, 0, sizeof(ca_l2_addr_entry_t));

    if (return_entry->prev_handle == NULL)
        start_addr = 0;
    else {
	cast_tmp = (long) return_entry->prev_handle;
        start_addr = (ca_uint32)( cast_tmp & 0xffffffff);
    }

    L2_DEBUG("start_addr:%d\r\n", start_addr);

    i = 0;

    while (i < return_entry->entry_count) {
        ret = aal_fdb_entry_scan(device_id,
                                start_addr, &hit_addr,
                                &fdb_entry, &cmd_ret);

        if (ret != CA_E_OK || cmd_ret != CA_AAL_FDB_ACCESS_HIT) {
            L2_DEBUG("search end\r\n");
            ret = CA_E_UNAVAIL;
            break;
            //return CA_E_UNAVAIL;
        }

        aal_fdb_entry_2_l2_addr(&addr, &fdb_entry);
        //trav_fn(device_id, &addr, user_data);
        L2_DEBUG("i:%d, start_addr:%d, hit_addr:%d, ret:%d\r\n", i, start_addr, hit_addr, ret);
        L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d, valid:%d, tmp:%d\r\n",
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id,
                fdb_entry.valid,
                fdb_entry.tmp_flag);

        if (!fdb_entry.tmp_flag){
        memcpy(return_entry->entry_data + entry_size * i, (void *)&addr, entry_size);
        //return_entry->entry_data += entry_size;
            i++;
        }

        if (start_addr == hit_addr + 1){
            L2_DEBUG("start_addr == hit_addr + 1\r\n");
            ret = CA_E_UNAVAIL;
            break;
            //return CA_E_UNAVAIL;
        }
        start_addr = hit_addr + 1;
    }

    //next_addr = hit_addr + 1;
    L2_DEBUG("start_addr:%d, i:%d, return_entry:%p\r\n", start_addr, i, return_entry);
    return_entry->entry_count = i;
    cast_tmp = start_addr;
    return_entry->prev_handle = (void*) cast_tmp;
    L2_DEBUG("entry_count:%d, prev_handle:%p\r\n", return_entry->entry_count, return_entry->prev_handle);

    if (i == 0)
        return CA_E_UNAVAIL;
    return CA_E_OK;
}


ca_status_t ca_l2_addr_delete_all (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_l2_addr_operation_flags_t flag
)
{
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    aal_fdb_flush_cfg_t cfg;
    ca_status_t ret;

//    L2_ASSERT_RET(flag != CA_L2_ADDR_OPERATION_BOTH, CA_E_NOT_SUPPORT);

    mask.wrd = 0xffffffff;
    memset(&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    L2_DEBUG("\r\n");
    aal_fdb_cmp_cfg_load(&cfg);

    if (flag == CA_L2_ADDR_OPERATION_STATIC){
        mask.bf.static_flag = 0;  /* join */
        cfg.static_flag = 1;
    }
    else if (flag == CA_L2_ADDR_OPERATION_DYNAMIC){
        mask.bf.static_flag = 0;
        cfg.static_flag = 0;
    }
    else if (flag == CA_L2_ADDR_OPERATION_BOTH){
    /* flush all, include static */
        mask.bf.static_flag = 1;
        cfg.static_flag = 0;
    }

    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    ret = aal_fdb_entry_flush(device_id);

    if (ret == CA_E_OK && (flag == CA_L2_ADDR_OPERATION_STATIC || flag == CA_L2_ADDR_OPERATION_BOTH))
        ret = aal_fdb_tbl_del_all();

    __aal_fdb_sw_lrn_tbl_clr(flag);

    return ret;
}

ca_status_t ca_l2_addr_aged_event_get (
    CA_IN ca_device_id_t         device_id,
    CA_OUT ca_l2_addr_entry_t    *addr)
{
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_status_t ret = CA_E_OK;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    ret = aal_fdb_addr_aged_entry_get(device_id, &fdb_entry);

    if (ret == CA_E_OK) {
        ret = aal_fdb_entry_2_l2_addr(addr, &fdb_entry);
    }

    return ret;
}

ca_status_t ca_l2_mac_filter_default_set (
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t drop_flag
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    L2_DEBUG("port_id:0x%x, drop_flag:%d\r\n", port_id, drop_flag);

    CA_PORT_PHYSICAL_CHECK(port_id);

    ret = aal_fdb_mac_filter_default_set(device_id, lpid, drop_flag);
    return ret;
}


ca_status_t ca_l2_mac_filter_default_get (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t *drop_flag
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    CA_PORT_PHYSICAL_CHECK(port_id);

    L2_ASSERT_RET(drop_flag != NULL, CA_E_PARAM);
    ret = aal_fdb_mac_filter_default_get(device_id, lpid, drop_flag);
    L2_DEBUG("port_id:%d, drop_flag:%d\r\n", lpid, *drop_flag);

    return ret;
}


ca_status_t ca_l2_mac_filter_add (
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN ca_l2_mac_filter_entry_t *entry
)
{
    aal_tbl_mac_filter_entry_t filter_entry;

    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    L2_ASSERT_RET(entry != NULL, CA_E_PARAM);

    CA_PORT_PHYSICAL_CHECK(port_id);

    L2_DEBUG("port_id:%d\r\n", lpid);

    memset(&filter_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));

    filter_entry.mask = entry->mask;
    filter_entry.mac_flag = entry->mac_flag;
    if (entry->mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN)
        filter_entry.vid = entry->vid;
    filter_entry.drop_flag = entry->drop_flag;
    memcpy(&filter_entry.mac, &entry->mac, CA_ETH_ADDR_LEN);
    filter_entry.port_id = lpid;

    ret = aal_fdb_mac_filter_add(device_id, lpid, &filter_entry);

    return ret;
}


ca_status_t ca_l2_mac_filter_delete (
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN ca_l2_mac_filter_entry_t *entry
)
{
    aal_tbl_mac_filter_entry_t filter_entry;
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    L2_ASSERT_RET(entry != NULL, CA_E_PARAM);

    L2_DEBUG("port_id:%d\r\n", lpid);

    CA_PORT_PHYSICAL_CHECK(port_id);

    memset(&filter_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));

    filter_entry.mask = entry->mask;
    filter_entry.mac_flag = entry->mac_flag;
    if (entry->mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN)
        filter_entry.vid = entry->vid;
    filter_entry.drop_flag = entry->drop_flag;
    memcpy(&filter_entry.mac, &entry->mac, CA_ETH_ADDR_LEN);

    ret = aal_fdb_mac_filter_delete(device_id, lpid, &filter_entry);

    return ret;
}


ca_status_t ca_l2_mac_filter_delete_all (
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    CA_PORT_PHYSICAL_CHECK(port_id);

    ret = aal_fdb_mac_filter_delete_all(device_id, lpid);
    L2_DEBUG("port_id:%d\r\n", lpid);

    return ret;
}

ca_status_t ca_l2_mac_filter_iterate (
    CA_IN  ca_device_id_t                  device_id,
    CA_OUT ca_iterator_t                *return_entry
)
{
    aal_tbl_mac_filter_entry_t *entry_p = NULL;
    ca_uint32_t entry_size = sizeof(ca_l2_mac_filter_entry_iterator_t);
    int i, search_item = 0;
    ca_l2_mac_filter_entry_iterator_t entry_iterator ;

    CA_ASSERT_RET(return_entry != NULL && return_entry->entry_data != NULL, CA_E_PARAM);
    CA_ASSERT_RET(return_entry->entry_count, CA_E_PARAM);
    CA_ASSERT_RET(return_entry->entry_count * entry_size <= return_entry->user_buffer_size, CA_E_PARAM);

    L2_DEBUG("enter, entry_count:%d\r\n", return_entry->entry_count);
    memset(&entry_iterator , 0, sizeof(ca_l2_mac_filter_entry_iterator_t));

    i = 0;
    while (i < return_entry->entry_count) {
        if (return_entry->prev_handle == NULL)
            entry_p = aal_fdb_mac_filter_getfirst();
        else{
            entry_p = aal_fdb_mac_filter_getfirst();
            while(++search_item < 1000){
                L2_DEBUG("entry_p:%p\r\n", entry_p);
                if (entry_p == return_entry->prev_handle){
                    entry_p = aal_fdb_mac_filter_getnext();
                    L2_DEBUG("find next\r\n");
                    break;
                }
                else{
                    entry_p = aal_fdb_mac_filter_getnext();
                    if (entry_p == NULL){
                        L2_DEBUG("no find prev_handle:%p\r\n", return_entry->prev_handle);
                        break;
                    }
                }
            }
        }

        L2_DEBUG("entry_p:%p, prev_handle:%p\r\n", entry_p, return_entry->prev_handle);

        if (entry_p == NULL)
            break;
            //return CA_E_UNAVAIL;

        L2_DEBUG("mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, mask:%d, mac_flag:%d, drop_flag:%d, port_id:%d, entry:%d, rule:%d\r\n",
            entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
            entry_p->vid, entry_p->mask, entry_p->mac_flag, entry_p->drop_flag, entry_p->port_id, entry_p->entry_id, entry_p->rule_id);

        entry_iterator.port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, entry_p->port_id);
        entry_iterator.mac_filter.mask = entry_p->mask;
        memcpy(entry_iterator.mac_filter.mac,entry_p->mac,sizeof(ca_mac_addr_t));
        entry_iterator.mac_filter.mac_flag = entry_p->mac_flag;
        entry_iterator.mac_filter.vid = entry_p->vid;
        entry_iterator.mac_filter.drop_flag = entry_p->drop_flag;

        memcpy(return_entry->entry_data + entry_size * i , &entry_iterator, sizeof(ca_l2_mac_filter_entry_iterator_t));
        return_entry->prev_handle = (void *)entry_p;
        i++;
    }

    return_entry->entry_count = i;

    if (i == 0){
        L2_DEBUG("return %d\r\n", CA_E_UNAVAIL);
        return CA_E_UNAVAIL;
    }
    //return_entry->prev_handle = (void *)entry_p;

    return CA_E_OK;
}


ca_status_t ca_l2_flooding_rate_set(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_l2_flooding_type_t ptype,
    CA_IN ca_boolean_t          pps,
    CA_IN ca_uint32_t           rate
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    //L2_ASSERT_RET(rate != NULL, CA_E_PARAM);

    L2_DEBUG("lpid:%d, ptype:%d, rate:%d\r\n", lpid, ptype, rate);

    CA_PORT_PHYSICAL_CHECK(port_id);

    if (rate / 1000 > 0xffff) rate = CA_UINT32_INVALID;

    ret = aal_fdb_ilpb_unknw_pol_idx_map_set(device_id, lpid, ptype, pps, CA_UINT32_INVALID);

    if (rate != CA_UINT32_INVALID)
        ret = aal_fdb_ilpb_unknw_pol_idx_map_set(device_id, lpid, ptype, pps, rate);

    return ret;
}

ca_status_t ca_l2_flooding_rate_get(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_IN  ca_l2_flooding_type_t ptype,
    CA_OUT ca_boolean_t          *pps,
    CA_OUT ca_uint32_t           *rate
)
{
    ca_status_t ret = CA_E_OK;
    ca_int32 lpid = PORT_ID(port_id);

    L2_ASSERT_RET(rate != NULL, CA_E_PARAM);

    L2_DEBUG("lpid:%d, ptype:%d\r\n", lpid, ptype);

    CA_PORT_PHYSICAL_CHECK(port_id);

    ret = aal_fdb_ilpb_unknw_pol_idx_map_get(device_id, lpid, ptype, pps, rate);

    return ret;
}


ca_status_t ca_l2_fdb_init(
    ca_device_id_t     device_id
)
{
    ca_int32 ret = 0;
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    ca_device_config_tlv_t scfg_tlv;
    ca_int32 pid;
    ca_uint32_t rate = 2000;
#endif
    ret = ca_spin_lock_init(&g_l2_lock, SPINLOCK_BH);

    if (ret != 0) {
        ca_printf("create l2 spin lock %s fail\r\n", CA_AAL_FDB_MUTEX_NAME);
    }

    aal_fdb_init(device_id);
   // aal_fdb_mac_filter_init(device_id);
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    for (pid = AAL_LPORT_ETH_NI3; pid <= AAL_LPORT_ETH_NI6; pid++){
        ca_l2_flooding_rate_set(0,
                                CA_PORT_ID(CA_PORT_TYPE_ETHERNET, pid),
                                CA_L2_FLOODING_TYPE_BC,
                                0,
                                rate);
        ca_l2_flooding_rate_set(0,
                                CA_PORT_ID(CA_PORT_TYPE_ETHERNET, pid),
                                CA_L2_FLOODING_TYPE_UUC,
                                0,
                                rate);
    }

    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_CUSTOMER_BIT;
    scfg_tlv.len  = sizeof(ca_uint32_t);
    ret = ca_device_config_tlv_get(0, &scfg_tlv);
    if (CA_E_OK == ret)
        memcpy(&l2_customer_info, scfg_tlv.value, sizeof(ca_uint32_t));

#endif
    return CA_E_OK;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(ca_l2_mac_limit_get);
EXPORT_SYMBOL(ca_l2_aging_time_get);
EXPORT_SYMBOL(ca_l2_mac_filter_default_get);
EXPORT_SYMBOL(ca_l2_addr_delete_all);
EXPORT_SYMBOL(ca_l2_aging_time_set);
EXPORT_SYMBOL(ca_l2_mac_filter_add);
EXPORT_SYMBOL(ca_l2_mac_filter_default_set);
EXPORT_SYMBOL(ca_l2_learning_mode_set);
EXPORT_SYMBOL(ca_l2_addr_delete_by_vlan);
EXPORT_SYMBOL(ca_l2_flooding_rate_set);
EXPORT_SYMBOL(ca_l2_learning_control_get);
EXPORT_SYMBOL(ca_l2_addr_delete_by_mac);
EXPORT_SYMBOL(ca_l2_mac_filter_delete_all);
EXPORT_SYMBOL(ca_l2_learning_control_set);
EXPORT_SYMBOL(ca_l2_addr_get);
EXPORT_SYMBOL(ca_l2_mac_limit_set);
EXPORT_SYMBOL(ca_l2_mac_filter_delete);
EXPORT_SYMBOL(ca_l2_addr_add);
EXPORT_SYMBOL(ca_l2_flooding_rate_get);
EXPORT_SYMBOL(ca_l2_addr_delete_by_port);
EXPORT_SYMBOL(ca_l2_addr_delete);
EXPORT_SYMBOL(ca_l2_addr_iterate);
EXPORT_SYMBOL(ca_l2_mac_filter_iterate);
EXPORT_SYMBOL(ca_l2_learning_mode_get);
EXPORT_SYMBOL(ca_l2_addr_aged_event_get);
EXPORT_SYMBOL(ca_l2_aging_mode_get);
EXPORT_SYMBOL(ca_l2_aging_mode_set);
#endif

