#include <generated/ca_ne_autoconf.h>
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_common.h"
#include "aal_fdb.h"
#include "aal_arb.h"
#include "aal_port.h"
#include "aal_l2_tm.h"
#include "scfg.h"


ca_uint g_arb_lock = 0;

#define __ARB_LOCK()    \
        ca_spin_lock(g_arb_lock)

#define __ARB_UNLOCK()   \
        ca_spin_unlock(g_arb_lock)

/** aal_arb_init
 * @brief: init L2FE arbiter
 */
ca_status_t aal_arb_init(
    ca_device_id_t     device_id
)
{
    ca_status_t         ret = CA_E_OK;
    ca_uint32_t         flow_id = 0;
    aal_arb_ctrl_mask_t arb_ctrl_mask;
    aal_arb_ctrl_t      arb_ctrl;
    aal_arb_port_dbuf_config_mask_t port_dbuf_mask;
    aal_arb_port_dbuf_config_t port_dbuf;

    memset(&arb_ctrl, 0, sizeof(aal_arb_ctrl_t));
    memset(&port_dbuf, 0, sizeof(aal_arb_port_dbuf_config_t));

    if (!g_arb_lock) {
        ret = ca_spin_lock_init(&g_arb_lock, SPINLOCK_BH);
        CA_RET_VALUE_CHECK(ret);
    }

    /* for Bug 49937: to init dbuf tables to be zeros,
     * and,  when use the table, should sort the valid tables
     * before invalid entries
     */
    port_dbuf_mask.wrd = ~0;
    memset(&port_dbuf, 0, sizeof(port_dbuf));
    for (flow_id = 0; flow_id <= 7; flow_id++) {
        aal_arb_port_dbuf_set(device_id, flow_id, port_dbuf_mask, &port_dbuf);
    }

    arb_ctrl_mask.u32 = 0;
    arb_ctrl_mask.s.dbuf_sel = 1;
    arb_ctrl.dbuf_sel = 1;
    arb_ctrl_mask.s.keep_o_lspid_en_for_mc = 1;
    arb_ctrl.keep_o_lspid_en_for_mc = 1;
	arb_ctrl.use_hdr_a_dbuf_en = 0;	// use dbuf related table instead of header_a
	arb_ctrl_mask.s.use_hdr_a_dbuf_en = 1;
    ret = aal_arb_ctrl_set(device_id, arb_ctrl_mask, &arb_ctrl);

    /* default flow init */
    ret = scfg_read(device_id, CFG_ID_NE_DEFAULT_FLOW,
        sizeof(ca_uint32_t), (ca_uint8_t *)&flow_id);
    if (CA_E_OK == ret) {
        if (flow_id > CA_AAL_MAX_FLOW_ID) { /* invalid flow */
            REG_L2FE_FIELD_WRITE(L2FE_PLE_DEFAULT_REG, dflt_flowid, 0);
            REG_L2FE_FIELD_WRITE(L2FE_PLE_DEFAULT_REG, dflt_flow_en, 0);
        } else {/* valid flow */
            REG_L2FE_FIELD_WRITE(L2FE_PLE_DEFAULT_REG, dflt_flowid, flow_id);
            REG_L2FE_FIELD_WRITE(L2FE_PLE_DEFAULT_REG, dflt_flow_en, 1);
        }
    }

    REG_L2FE_FIELD_WRITE(L2FE_ARB_ARB_CTRL_EXT, mc_pkt_port_mmshp_chk_en, 1);

    return ret;
}

ca_status_t aal_arb_ctrl_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN aal_arb_ctrl_mask_t  ctrl_mask,
    CA_IN aal_arb_ctrl_t       *ctrl
)
{
    L2FE_ARB_ARB_CTRL_t  arb_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __ARB_LOCK();


    arb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_ARB_CTRL);

    if (ctrl_mask.s.drop_redir_keep_old)
        arb_ctrl.bf.drop_redir_keep_old = ctrl->drop_redir_keep_old;
    if (ctrl_mask.s.dbuf_sel)
        arb_ctrl.bf.dbuf_sel = ctrl->dbuf_sel;
    if (ctrl_mask.s.ip_ecn_en)
        arb_ctrl.bf.ip_ecn_en = ctrl->ip_ecn_en;
    if (ctrl_mask.s.mc_idx_copy_en)
        arb_ctrl.bf.mc_idx_copy_en = ctrl->mc_idx_copy_en;
    if (ctrl_mask.s.dbuf_dpid)
        arb_ctrl.bf.dbuf_dpid = ctrl->dbuf_dpid;
    if (ctrl_mask.s.use_hdr_a_dbuf_en)
        arb_ctrl.bf.use_hdr_a_dbuf_en = ctrl->use_hdr_a_dbuf_en;

    if (ctrl_mask.s.keep_o_lspid_en_for_mc)
        arb_ctrl.bf.keep_o_lspid_en_for_mc = ctrl->keep_o_lspid_en_for_mc;

    if (ctrl_mask.s.dst_wan_dync_buf_sel){
        arb_ctrl.bf.dst_wan_dync_buf_sel = ctrl->dst_wan_dync_buf_sel;
    }
    if (ctrl_mask.s.wan_port_ldpid_low){
        arb_ctrl.bf.wan_lpid = ctrl->wan_port_ldpid_low;
    }
    if (ctrl_mask.s.wan_port_ldpid_hi){
        arb_ctrl.bf.wan_ext_lpid = ctrl->wan_port_ldpid_hi;
    }
    if (ctrl_mask.s.wan_port_tm_dpid){
        arb_ctrl.bf.tm_wan_dpid = ctrl->wan_port_tm_dpid;
    }
    if (ctrl_mask.s.tqm_port7_empty_wait_cnt){
        arb_ctrl.bf.tqm_port7_empty_wait_cnt_h = ctrl->tqm_port7_empty_wait_cnt;
    }

    CA_DEV_NE_REG_WRITE(device_id, arb_ctrl.wrd, L2FE_ARB_ARB_CTRL);
    
    if(ctrl_mask.s.mc_pkt_port_mmshp_chk_ena){
        REG_L2FE_FIELD_WRITE(L2FE_ARB_ARB_CTRL_EXT, mc_pkt_port_mmshp_chk_en, (ctrl->mc_pkt_port_mmshp_chk_ena > 0));
    }
    if(ctrl_mask.s.ttl_zero_trap_keep_old)
	REG_L2FE_FIELD_WRITE(L2FE_ARB_ARB_TTL_TRAP, keep_old, ctrl->ttl_zero_trap_keep_old);
    if(ctrl_mask.s.ttl_zero_trap_ldpid)
	REG_L2FE_FIELD_WRITE(L2FE_ARB_ARB_TTL_TRAP, ldpid, ctrl->ttl_zero_trap_ldpid);
    if(ctrl_mask.s.ttl_zero_trap_cos)
	REG_L2FE_FIELD_WRITE(L2FE_ARB_ARB_TTL_TRAP, cos, ctrl->ttl_zero_trap_cos);

    __ARB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_arb_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_arb_ctrl_t  *ctrl
)
{
    L2FE_ARB_ARB_CTRL_t  arb_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __ARB_LOCK();

    arb_ctrl.wrd  = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_ARB_CTRL);

    ctrl->drop_redir_keep_old       = arb_ctrl.bf.drop_redir_keep_old;
    ctrl->dbuf_sel                  = arb_ctrl.bf.dbuf_sel;
    ctrl->ip_ecn_en                 = arb_ctrl.bf.ip_ecn_en;
    ctrl->mc_idx_copy_en            = arb_ctrl.bf.mc_idx_copy_en;
    ctrl->dbuf_dpid                 = arb_ctrl.bf.dbuf_dpid;
    ctrl->use_hdr_a_dbuf_en         = arb_ctrl.bf.use_hdr_a_dbuf_en;
    ctrl->keep_o_lspid_en_for_mc    = arb_ctrl.bf.keep_o_lspid_en_for_mc;
    ctrl->dst_wan_dync_buf_sel      = arb_ctrl.bf.dst_wan_dync_buf_sel;
    ctrl->wan_port_ldpid_low        = arb_ctrl.bf.wan_lpid;
    ctrl->wan_port_ldpid_hi         = arb_ctrl.bf.wan_ext_lpid;
    ctrl->wan_port_tm_dpid          = arb_ctrl.bf.tm_wan_dpid;
    ctrl->tqm_port7_empty_wait_cnt  = arb_ctrl.bf.tqm_port7_empty_wait_cnt_h;


    REG_L2FE_FIELD_READ(L2FE_ARB_ARB_CTRL_EXT,mc_pkt_port_mmshp_chk_en,ctrl->mc_pkt_port_mmshp_chk_ena); 
    REG_L2FE_FIELD_READ(L2FE_ARB_ARB_TTL_TRAP, keep_old, ctrl->ttl_zero_trap_keep_old);
    REG_L2FE_FIELD_READ(L2FE_ARB_ARB_TTL_TRAP, ldpid, ctrl->ttl_zero_trap_ldpid);
    REG_L2FE_FIELD_READ(L2FE_ARB_ARB_TTL_TRAP, cos, ctrl->ttl_zero_trap_cos);


    __ARB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_arb_per_cos_dont_mark_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_arb_per_cos_dont_mark_ctrl_mask_t  ctrl_mask,
    CA_IN aal_arb_per_cos_dont_mark_ctrl_t       *ctrl
)
{
    L2FE_ARB_PER_COS_DONT_MARK_CTRL_t cos_mark_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __ARB_LOCK();

    cos_mark_ctrl.wrd  = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PER_COS_DONT_MARK_CTRL);

    if (ctrl_mask.bf.bitmap_dst_lan)
        cos_mark_ctrl.bf.bitmap_dst_lan = ctrl->bitmap_dst_lan;

    if (ctrl_mask.bf.bitmap_dst_wan)
        cos_mark_ctrl.bf.bitmap_dst_wan = ctrl->bitmap_dst_wan;

    CA_DEV_NE_REG_WRITE(device_id, cos_mark_ctrl.wrd, L2FE_ARB_PER_COS_DONT_MARK_CTRL);

    __ARB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_arb_per_cos_dont_mark_ctrl_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_arb_per_cos_dont_mark_ctrl_t       *ctrl
)
{
    L2FE_ARB_PER_COS_DONT_MARK_CTRL_t cos_mark_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __ARB_LOCK();

    cos_mark_ctrl.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PER_COS_DONT_MARK_CTRL);

    ctrl->bitmap_dst_lan = cos_mark_ctrl.bf.bitmap_dst_lan;
    ctrl->bitmap_dst_wan = cos_mark_ctrl.bf.bitmap_dst_wan;

    __ARB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_arb_redir_ldpid_config_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          ldpid,
    CA_IN aal_arb_redir_config_mask_t mask,
    CA_IN aal_arb_redir_config_t *cfg
)
{
    L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS_t access;
    L2FE_ARB_REDIR_LDPID_CONFIG_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(ldpid <= 63 && ldpid >= 0, CA_E_PARAM);


    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, ldpid, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_DATA);

    if (mask.bf.en)
        data.bf.rdir_en = cfg->en;
    if (mask.bf.cos_vld)
        data.bf.rdir_cos_vld = cfg->cos_vld;
    if (mask.bf.cos)
        data.bf.rdir_cos = cfg->cos;
    if (mask.bf.ldpid)
        data.bf.rdir_ldpid = cfg->ldpid;
    if (mask.bf.wan_dst)
        data.bf.rdir_wan_dst = cfg->wan_dst;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, ldpid, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_arb_redir_ldpid_config_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          ldpid,
    CA_OUT aal_arb_redir_config_t *cfg
)
{
    L2FE_ARB_REDIR_LDPID_CONFIG_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(ldpid <= 63 && ldpid >= 0, CA_E_PARAM);

    __ARB_LOCK();


    CA_DEV_READ_INDIRCT_REG(device_id, ldpid, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_LDPID_CONFIG_TBL_DATA);

    cfg->en = data.bf.rdir_en;
    cfg->cos_vld = data.bf.rdir_cos_vld;
    cfg->cos = data.bf.rdir_cos;
    cfg->ldpid = data.bf.rdir_ldpid;
    cfg->wan_dst = data.bf.rdir_wan_dst;

    __ARB_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_arb_redir_drop_src_config_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          drop_src,
    CA_IN aal_arb_redir_config_mask_t mask,
    CA_IN aal_arb_redir_config_t *cfg
)
{
    L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS_t access;
    L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(drop_src <= 31 && drop_src >= 0, CA_E_PARAM);


    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, drop_src, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_DATA);

    if (mask.bf.en)
        data.bf.rdir_en = cfg->en;
    if (mask.bf.cos_vld)
        data.bf.rdir_cos_vld = cfg->cos_vld;
    if (mask.bf.cos)
        data.bf.rdir_cos = cfg->cos;
    if (mask.bf.ldpid)
        data.bf.rdir_ldpid = cfg->ldpid;
    if (mask.bf.wan_dst)
        data.bf.rdir_wan_dst = cfg->wan_dst;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, drop_src, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_arb_redir_drop_src_config_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          drop_src,
    CA_OUT aal_arb_redir_config_t *cfg
)
{
    L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(drop_src <= 31 && drop_src >= 0, CA_E_PARAM);

    __ARB_LOCK();


    CA_DEV_READ_INDIRCT_REG(device_id, drop_src, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_DATA);

    cfg->en = data.bf.rdir_en;
    cfg->cos_vld = data.bf.rdir_cos_vld;
    cfg->cos = data.bf.rdir_cos;
    cfg->ldpid = data.bf.rdir_ldpid;
    cfg->wan_dst = data.bf.rdir_wan_dst;

    __ARB_UNLOCK();

    return CA_E_OK;

}


ca_status_t aal_arb_non_known_pol_map_set(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_uint16_t            lspid_pol_idx,
    CA_IN aal_arb_pkt_fwd_type_t pkt_type,
    CA_IN ca_uint16_t            flooding_pol_id
)
{
    L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS_t access;
    L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA_t data;
    ca_uint32_t addr = (lspid_pol_idx << 2) | pkt_type;

    AAL_ASSERT_RET(lspid_pol_idx < AAL_ILPB_UNKWN_POL_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(pkt_type < AAL_ARB_PKT_FWD_TYPE_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(flooding_pol_id < AAL_ARB_FLOOD_POL_ID_NUM, CA_E_PARAM);

    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA);

    data.bf.flooding_pol_id = flooding_pol_id;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, addr, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_arb_non_known_pol_map_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint16_t             lspid_pol_idx,
    CA_IN aal_arb_pkt_fwd_type_t  pkt_type,
    CA_OUT ca_uint16_t           *flooding_pol_id
)
{
    L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA_t data;
    ca_uint32_t addr = (lspid_pol_idx << 2) | pkt_type;

    AAL_ASSERT_RET(flooding_pol_id != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(pkt_type < AAL_ARB_PKT_FWD_TYPE_MAX && pkt_type >= 0, CA_E_PARAM);

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA);

    __ARB_UNLOCK();

    *flooding_pol_id = data.bf.flooding_pol_id;

    return CA_E_OK;

}



ca_status_t aal_arb_dscp_te_mark_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         ipv6_type,
    CA_IN ca_boolean_t         wan_dst_ind,
    CA_IN ca_uint8_t           tx_dscp,
    CA_IN ca_uint8_t           mark_value
)
{
    L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS_t access;
    L2FE_ARB_DSCP_TE_MARK_TBL_DATA_t data;
    ca_uint32_t addr = (ipv6_type << 7) | (wan_dst_ind << 6) | tx_dscp;

    AAL_ASSERT_RET(ipv6_type <= 1 && wan_dst_ind <= 1, CA_E_PARAM);
    AAL_ASSERT_RET(tx_dscp <= 63 && mark_value <= 63, CA_E_PARAM);

    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_DSCP_TE_MARK_TBL_DATA);

    data.bf.mark_value = mark_value;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_DSCP_TE_MARK_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, addr, L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_arb_dscp_te_mark_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         ipv6_type,
    CA_IN ca_boolean_t         wan_dst_ind,
    CA_IN ca_uint8_t           tx_dscp,
    CA_OUT ca_uint8_t         *mark_value
)
{
    L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS_t access;
    L2FE_ARB_DSCP_TE_MARK_TBL_DATA_t data;
    ca_uint32_t addr = (ipv6_type << 7) | (wan_dst_ind << 6) | tx_dscp;

    AAL_ASSERT_RET(ipv6_type <= 1 &&
        wan_dst_ind <= 1 &&
        tx_dscp <= 63, CA_E_PARAM);

    AAL_ASSERT_RET(mark_value != NULL, CA_E_PARAM);

    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_DSCP_TE_MARK_TBL_DATA);

    __ARB_UNLOCK();

    *mark_value = data.bf.mark_value;

    return CA_E_OK;

}

ca_status_t aal_arb_flow_voq_dbuf_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint16_t                   flow_id,
    CA_IN aal_arb_flow_voq_dq_ena_t     *ena
)
{
    L2FE_ARB_FLOW_DBUF_TBL_DATA_t voq_data;
    ca_status_t ret = CA_E_OK;

    if((ena == NULL) || (flow_id>=512)){
        return CA_E_PARAM;
    }

    __ARB_LOCK();
    CA_DEV_READ_INDIRCT_REG(device_id,(flow_id/4),L2FE_ARB_FLOW_DBUF_TBL_ACCESS);
    voq_data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_FLOW_DBUF_TBL_DATA);
    switch(flow_id&0x3){
        case 0:
            voq_data.bf.dbuf_flg_0 = ena->ena;
            break;
        case 1:
            voq_data.bf.dbuf_flg_1 = ena->ena;
            break;
        case 2:
            voq_data.bf.dbuf_flg_2 = ena->ena;
            break;
        case 3:
            voq_data.bf.dbuf_flg_3 = ena->ena;
            break;
       default:
            break;
    }
    CA_DEV_NE_REG_WRITE(device_id, voq_data.wrd,L2FE_ARB_FLOW_DBUF_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id,(flow_id/4),L2FE_ARB_FLOW_DBUF_TBL_ACCESS);

    __ARB_UNLOCK();

    return ret;
}

ca_status_t aal_arb_flow_voq_dbuf_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint16_t                   flow_id,
    CA_OUT aal_arb_flow_voq_dq_ena_t     *ena
)
{
    L2FE_ARB_FLOW_DBUF_TBL_DATA_t voq_data;
    ca_status_t ret = CA_E_OK;

    if((ena == NULL) || (flow_id>=512)){
        return CA_E_PARAM;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id,(flow_id/4),L2FE_ARB_FLOW_DBUF_TBL_ACCESS);
    voq_data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_FLOW_DBUF_TBL_DATA);

    switch(flow_id&0x3){
        case 0:
            ena->ena = voq_data.bf.dbuf_flg_0;
            break;
        case 1:
            ena->ena = voq_data.bf.dbuf_flg_1;
            break;
        case 2:
            ena->ena = voq_data.bf.dbuf_flg_2;
            break;
        case 3:
            ena->ena = voq_data.bf.dbuf_flg_3;
            break;
       default:
            break;
    }

    __ARB_UNLOCK();

    return ret;
}



ca_status_t aal_arb_port_dbuf_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint8_t           dbuf_id,
    CA_IN aal_arb_port_dbuf_config_mask_t mask,
    CA_IN aal_arb_port_dbuf_config_t *cfg
)
{
    L2FE_ARB_PORT_DBUF_TBL_ACCESS_t access;
    L2FE_ARB_PORT_DBUF_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(dbuf_id <= 7, CA_E_PARAM);


    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PORT_DBUF_TBL_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, dbuf_id, L2FE_ARB_PORT_DBUF_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PORT_DBUF_TBL_DATA);

    if (mask.bf.dbuf_flg)
        data.bf.dbuf_flg = cfg->dbuf_flg;
    if (mask.bf.ldpid_vld)
        data.bf.ldpid_vld = cfg->ldpid_vld;
    if (mask.bf.ldpid)
        data.bf.ldpid = cfg->ldpid;
    if (mask.bf.lspid_vld)
        data.bf.lspid_vld = cfg->lspid_vld;
    if (mask.bf.lspid)
        data.bf.lspid = cfg->lspid;


    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_PORT_DBUF_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, dbuf_id, L2FE_ARB_PORT_DBUF_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_arb_port_dbuf_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint8_t           dbuf_id,
    CA_OUT aal_arb_port_dbuf_config_t *cfg
)
{
    L2FE_ARB_PORT_DBUF_TBL_DATA_t data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(dbuf_id <= 7, CA_E_PARAM);

    __ARB_LOCK();


    CA_DEV_READ_INDIRCT_REG(device_id, dbuf_id, L2FE_ARB_PORT_DBUF_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PORT_DBUF_TBL_DATA);

    cfg->dbuf_flg = data.bf.dbuf_flg;
    cfg->ldpid_vld = data.bf.ldpid_vld;
    cfg->ldpid = data.bf.ldpid;
    cfg->lspid_vld = data.bf.lspid_vld;
    cfg->lspid = data.bf.lspid;

    __ARB_UNLOCK();

    return CA_E_OK;

}


ca_status_t aal_arb_pdpid_map_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         my_mac,
    CA_IN ca_boolean_t         dbuf_flag,
    CA_IN ca_uint8_t           ldpid,
    CA_IN ca_uint8_t           pdpid
)
{
    L2FE_ARB_PDPID_MAP_TBL_DATA_t data;
    ca_uint32_t addr = (my_mac << 7) | (dbuf_flag << 6) | ldpid;

    AAL_ASSERT_RET(my_mac <= 1 && dbuf_flag <= 1, CA_E_PARAM);
    AAL_ASSERT_RET(ldpid < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);
    AAL_ASSERT_RET(pdpid < AAL_PORT_NUM_PHYSICAL, CA_E_PARAM);

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_PDPID_MAP_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PDPID_MAP_TBL_DATA);

    data.bf.pdpid = pdpid;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_ARB_PDPID_MAP_TBL_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, addr, L2FE_ARB_PDPID_MAP_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_arb_pdpid_map_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         my_mac,
    CA_IN ca_boolean_t         dbuf_flag,
    CA_IN ca_uint8_t           ldpid,
    CA_OUT ca_uint8_t         *pdpid
)
{
    L2FE_ARB_PDPID_MAP_TBL_DATA_t data;
    ca_uint32_t addr = (my_mac << 7) | (dbuf_flag << 6) | ldpid;

    AAL_ASSERT_RET(my_mac <= 1 && dbuf_flag <= 1, CA_E_PARAM);
    AAL_ASSERT_RET(ldpid < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);
    AAL_ASSERT_RET(pdpid != NULL, CA_E_PARAM);

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_ARB_PDPID_MAP_TBL_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_PDPID_MAP_TBL_DATA);

    __ARB_UNLOCK();

    *pdpid = data.bf.pdpid;

    return CA_E_OK;

}

ca_status_t aal_arb_mc_fib_set(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_uint32_t           fib_idx,
    CA_IN  aal_arb_mc_fib_mask_t mask,
    CA_IN  aal_arb_mc_fib_t      *cfg
)
{

    L2FE_ARB_MC_FIB_TBL_DATA1_t data1;
    L2FE_ARB_MC_FIB_TBL_DATA0_t data0;


    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(fib_idx <= AAL_ARB_MC_FIB_MAX, CA_E_PARAM);

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, fib_idx, L2FE_ARB_MC_FIB_TBL_ACCESS);
    data1.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_MC_FIB_TBL_DATA1);
    data0.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_MC_FIB_TBL_DATA0);

    if (mask.s.keep_o_lspid       )
        data1.bf.keep_o_lspid   = cfg->keep_o_lspid ;
    if (mask.s.ip_ttl_dec_cmd)
        data1.bf.ip_ttl_dec_cmd = cfg->ip_ttl_dec_cmd;
    if (mask.s.ip_ttl_zero_fwd_ctrl)
        data1.bf.ip_ttl_zero_fwd_ctrl = cfg->ip_ttl_zero_fwd_ctrl;
    if (mask.s.pol_en        )
        data1.bf.pol_en         = cfg->pol_en        ;
    if (mask.s.cos_cmd           )
        data1.bf.cos_cmd        = cfg->cos_cmd       ;
    if (mask.s.cos           )
        data1.bf.cos            = cfg->cos           ;
    if (mask.s.ldpid         )
        data1.bf.ldpid          = cfg->ldpid         ;
    if (mask.s.mac_sa_sel    )
        data1.bf.mac_sa_sel     = cfg->mac_sa_sel    ;
    if (mask.s.mac_sa_cmd    )
        data1.bf.mac_sa_cmd     = cfg->mac_sa_cmd    ;
    if (mask.s.permit_spid_en)
        data1.bf.permit_spid_en = cfg->permit_spid_en;
    if (mask.s.wan_dst       )
        data1.bf.wan_dst        = cfg->wan_dst       ;
    if (mask.s.vid           ) 
        data0.bf.vid            = cfg->vid       ;
    if (mask.s.wan_dst       )
        data1.bf.wan_dst        = cfg->wan_dst       ;
    if (mask.s.vlan_cmd      )
        data0.bf.vlan_cmd       = cfg->vlan_cmd      ;
    if (mask.s.pol_id        )
        data0.bf.pol_id         = cfg->pol_id        ;
    if (mask.s.pol_grp_id    )
        data1.bf.pol_grp_id     = cfg->pol_grp_id    ;
    if (mask.s.dei           )
        data0.bf.dei            = cfg->dei           ;
    if (mask.s.dei_cmd       )
        data0.bf.dei_cmd        = cfg->dei_cmd       ;
    if (mask.s.dot1p         )
        data0.bf.dot1p          = cfg->dot1p         ;
    if (mask.s.dot1p_cmd     )
        data0.bf.dot1p_cmd      = cfg->dot1p_cmd     ;
    if (mask.s.sc_ind        )
        data0.bf.sc_ind         = cfg->sc_ind        ;

    CA_NE_REG_WRITE(data1.wrd, L2FE_ARB_MC_FIB_TBL_DATA1);
    CA_NE_REG_WRITE(data0.wrd, L2FE_ARB_MC_FIB_TBL_DATA0);

    CA_DEV_WRITE_INDIRCT_REG(device_id, fib_idx, L2FE_ARB_MC_FIB_TBL_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;

}


ca_status_t aal_arb_mc_fib_get(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_uint32_t         fib_idx,
    CA_OUT aal_arb_mc_fib_t   *cfg
)
{
    L2FE_ARB_MC_FIB_TBL_DATA1_t data1;
    L2FE_ARB_MC_FIB_TBL_DATA0_t data0;


    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(fib_idx <= AAL_ARB_MC_FIB_MAX, CA_E_PARAM);

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, fib_idx, L2FE_ARB_MC_FIB_TBL_ACCESS);
    data1.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_MC_FIB_TBL_DATA1);
    data0.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_ARB_MC_FIB_TBL_DATA0);

    __ARB_UNLOCK();

    cfg->keep_o_lspid   =  data1.bf.keep_o_lspid;
    cfg->ip_ttl_dec_cmd  = data1.bf.ip_ttl_dec_cmd;
    cfg->ip_ttl_zero_fwd_ctrl = data1.bf.ip_ttl_zero_fwd_ctrl;

    cfg->pol_en          =  data1.bf.pol_en         ;
    cfg->cos_cmd         =  data1.bf.cos_cmd        ;
    cfg->cos             =  data1.bf.cos            ;
    cfg->ldpid           =  data1.bf.ldpid          ;
    cfg->mac_sa_sel      =  data1.bf.mac_sa_sel     ;
    cfg->mac_sa_cmd      =  data1.bf.mac_sa_cmd     ;
    cfg->permit_spid_en  =  data1.bf.permit_spid_en ;
    cfg->wan_dst         =  data1.bf.wan_dst        ;
    cfg->vid             =  data0.bf.vid            ;
    cfg->vlan_cmd        =  data0.bf.vlan_cmd       ;
    cfg->pol_id          =  data0.bf.pol_id         ;
    cfg->pol_grp_id      =  data1.bf.pol_grp_id     ;
    cfg->dei             =  data0.bf.dei            ;
    cfg->dei_cmd         =  data0.bf.dei_cmd        ;
    cfg->dot1p           =  data0.bf.dot1p          ;
    cfg->dot1p_cmd       =  data0.bf.dot1p_cmd      ;
    cfg->sc_ind          =  data0.bf.sc_ind         ;
    
    return CA_E_OK;

}

ca_status_t aal_ple_dft_fwd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          lspid,
    CA_IN aal_ple_nonkwn_type_t    type,
    CA_IN aal_ple_dft_fwd_ctrl_mask_t mask,
    CA_IN aal_ple_dft_fwd_ctrl_t *cfg
)
{
    L2FE_PLE_DFT_FWD_ACCESS_t access;
    L2FE_PLE_DFT_FWD_DATA_t data;
    ca_uint32_t addr = 0;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(lspid <= 63, CA_E_PARAM);

    /* If busy, quit */
    access.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_PLE_DFT_FWD_ACCESS);

    if (access.bf.access) {
        return CA_E_BUSY;
    }

    addr = (lspid << 2) | type;

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_PLE_DFT_FWD_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_PLE_DFT_FWD_DATA);

    if (mask.bf.dft_fwd_deny)
        data.bf.dft_fwd_deny = cfg->dft_fwd_deny;
    if (mask.bf.dft_fwd_mc_group_id)
        data.bf.dft_fwd_mc_group_id = cfg->dft_fwd_mc_group_id;
    if (mask.bf.dft_fwd_redir_en)
        data.bf.dft_fwd_redir_en = cfg->dft_fwd_redir_en;
    if (mask.bf.dft_fwd_mc_group_id_valid)
        data.bf.dft_fwd_mc_group_id_valid = cfg->dft_fwd_mc_group_id_valid;

    CA_DEV_NE_REG_WRITE(device_id, data.wrd, L2FE_PLE_DFT_FWD_DATA);
    CA_DEV_WRITE_INDIRCT_REG(device_id, addr, L2FE_PLE_DFT_FWD_ACCESS);

    __ARB_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ple_dft_fwd_get(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          lspid,
    CA_IN aal_ple_nonkwn_type_t    type,
    CA_OUT aal_ple_dft_fwd_ctrl_t *cfg
)
{
    L2FE_PLE_DFT_FWD_DATA_t data;
    ca_uint32_t addr = 0;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(lspid <= 63, CA_E_PARAM);


    addr = (lspid << 2) | type;

    __ARB_LOCK();

    CA_DEV_READ_INDIRCT_REG(device_id, addr, L2FE_PLE_DFT_FWD_ACCESS);
    data.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_PLE_DFT_FWD_DATA);

    cfg->dft_fwd_mc_group_id = data.bf.dft_fwd_mc_group_id;
    cfg->dft_fwd_deny = data.bf.dft_fwd_deny;
    cfg->dft_fwd_redir_en = data.bf.dft_fwd_redir_en;
    cfg->dft_fwd_mc_group_id_valid = data.bf.dft_fwd_mc_group_id_valid;

    __ARB_UNLOCK();

    return CA_E_OK;

}

#ifdef __KERNEL__
EXPORT_SYMBOL(aal_arb_dscp_te_mark_get);
EXPORT_SYMBOL(aal_arb_per_cos_dont_mark_ctrl_get);
EXPORT_SYMBOL(aal_arb_dscp_te_mark_set);
EXPORT_SYMBOL(aal_arb_redir_drop_src_config_get);
EXPORT_SYMBOL(aal_arb_redir_ldpid_config_get);
EXPORT_SYMBOL(aal_arb_redir_ldpid_config_set);
EXPORT_SYMBOL(aal_arb_pdpid_map_set);
EXPORT_SYMBOL(aal_arb_init);
EXPORT_SYMBOL(aal_arb_port_dbuf_set);
EXPORT_SYMBOL(aal_arb_pdpid_map_get);
EXPORT_SYMBOL(aal_arb_per_cos_dont_mark_ctrl_set);
EXPORT_SYMBOL(aal_arb_non_known_pol_map_set);
EXPORT_SYMBOL(aal_arb_redir_drop_src_config_set);
EXPORT_SYMBOL(aal_arb_ctrl_set);
EXPORT_SYMBOL(aal_arb_port_dbuf_get);
EXPORT_SYMBOL(aal_arb_non_known_pol_map_get);
EXPORT_SYMBOL(aal_arb_ctrl_get);
EXPORT_SYMBOL(aal_ple_dft_fwd_set);
EXPORT_SYMBOL(aal_ple_dft_fwd_get);
EXPORT_SYMBOL(aal_arb_mc_fib_set);
EXPORT_SYMBOL(aal_arb_mc_fib_get);
EXPORT_SYMBOL(aal_arb_flow_voq_dbuf_set);
EXPORT_SYMBOL(aal_arb_flow_voq_dbuf_get);
#endif

