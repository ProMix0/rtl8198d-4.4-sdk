/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * aal_ni.c: Hardware Abstraction Layer for NI driver to access hardware regsiters
 *
 */

#include "scfg.h"
#include "aal_port.h"
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_common.h"
#include "aal_arb.h"
#include "aal_mcast.h"
#include "aal_l3_l2lookup.h"
#include "aal_common.h"
#include "aal_phy.h"
#include "aal_ni_l2.h"
#include "eth_port.h"
#include "ca_event.h"
#include "aal_l2_te.h"

ca_uint __aal_port_lock = 0;
ca_uint8_t wan_port_id = AAL_LPORT_ETH_NI7; // wan port id will be inited by ca_load_init() in ni_drv.

#define __AAL_PORT_LOCK(dev)           ca_spin_lock(__aal_port_lock)
#define __AAL_PORT_UNLOCK(dev)         ca_spin_unlock(__aal_port_lock)

#define L2LKUP_CHECK(lspid)    (lspid == AAL_LPORT_L3_LAN ? TRUE: (lspid == AAL_LPORT_L3_WAN ? TRUE: FALSE))
#define L2LKUP_IDX(ldpid)    (ldpid == AAL_LPORT_L3_LAN ? 0: (ldpid == AAL_LPORT_L3_WAN ? 1: (ldpid == AAL_LPORT_ETH_WAN ? 2: 3)))
#define L2LKUP_LDPID(lspid)    (lspid == AAL_LPORT_L3_LAN ? AAL_LPORT_L3_WAN: AAL_LPORT_L3_LAN)
ca_uint8_t g_aal_l2vlan_outer_is_s[CFG_ID_PORT_L2VLAN_OUTER_IS_S_LEN] = {
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1
};
ca_uint8_t g_aal_l2vlan_inner_is_s[CFG_ID_PORT_L2VLAN_INNER_IS_S_LEN] = {
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1,
                                                    1,1,1,1,1,1,1,1
};

ca_uint8_t g_aal_ingr_port_is_wan[64] = {
                            0,0,0,0,0,0,0,1,
                            0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,
                            1,1,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0
                        };



static ca_uint8_t __g_cls_ingr_cls_port_entry_num[64] = {
        0x10 ,0x10 , 0x10 ,0x10 ,0  ,0x10     ,0x10     ,0x10   ,
        0   ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
        8   , 8    , 0 ,0    ,0   ,0   ,0   ,0   ,
        0   ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3
      };
static ca_uint8_t __g_cls_egr_cls_port_entry_num[64] = {
        0x10 ,0x10  , 0x10 ,0x10    ,0   ,0x10 ,0x10,0x10,
        0   ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
        8   , 8    , 0 ,0    ,0   ,0   ,0   ,0   ,
        0   ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3     ,
        3   , 3   , 3    ,3      ,3    ,3     ,3    ,3
      };

/* physical port to logical port mapping table */
ca_status_t aal_port_ippb_cfg_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t index,
                CA_IN aal_ippb_cfg_msk_t msk,
                CA_IN aal_ippb_cfg_t * cfg)
{
    L2FE_PP_IPPB_ACCESS_t oper;
    L2FE_PP_IPPB_DATA_t data;

    AAL_LPID_CHECK(index);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    __AAL_PORT_LOCK(dev);
    memset((void *)&oper, 0, sizeof(L2FE_PP_IPPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PP_IPPB_DATA_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, index, L2FE_PP_IPPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PP_IPPB_DATA);
    }

    if (msk.s.lspid) {
        data.bf.lspid = cfg->lspid;
    }

    if (dev == 0){
        CA_L2FE_REG_WRITE(data.wrd, L2FE_PP_IPPB_DATA);
        WRITE_INDIRCT_REG(dev, index, L2FE_PP_IPPB_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_ippb_cfg_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t index,
                CA_OUT aal_ippb_cfg_t * cfg)
{
    L2FE_PP_IPPB_ACCESS_t oper;
    L2FE_PP_IPPB_DATA_t data;

    AAL_LPID_CHECK(index);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    memset((void *)&oper, 0, sizeof(L2FE_PP_IPPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PP_IPPB_DATA_t));

    memset(cfg, 0, sizeof(aal_ippb_cfg_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, index, L2FE_PP_IPPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PP_IPPB_DATA);
    }

    cfg->lspid = data.bf.lspid;

    return CA_E_OK;
}

/* ingress logical port behavior configuration */
ca_status_t aal_port_ilpb_cfg_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_IN aal_ilpb_cfg_msk_t msk,
                CA_IN aal_ilpb_cfg_t * cfg)
{
    L2FE_PP_ILPB_ACCESS_t oper;
    L2FE_PP_ILPB_DATA4_t data4;
    L2FE_PP_ILPB_DATA3_t data3;
    L2FE_PP_ILPB_DATA2_t data2;
    L2FE_PP_ILPB_DATA1_t data1;
    L2FE_PP_ILPB_DATA0_t data0;
    l3fe_glb_ilpb_t l2lkup_ilpb;
    ca_uint16_t set_l2lkup, tbl_idx = 3;

    AAL_LPID_CHECK(lpid);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    __AAL_PORT_LOCK(dev);

    memset((void *)&oper, 0, sizeof(L2FE_PP_ILPB_ACCESS_t));
    memset((void *)&data4, 0, sizeof(L2FE_PP_ILPB_DATA4_t));
    memset((void *)&data3, 0, sizeof(L2FE_PP_ILPB_DATA3_t));
    memset((void *)&data2, 0, sizeof(L2FE_PP_ILPB_DATA2_t));
    memset((void *)&data1, 0, sizeof(L2FE_PP_ILPB_DATA1_t));
    memset((void *)&data0, 0, sizeof(L2FE_PP_ILPB_DATA0_t));

    memset((void *)&l2lkup_ilpb, 0, sizeof(l3fe_glb_ilpb_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PP_ILPB_ACCESS);
        data4.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA4);
        data3.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA3);
        data2.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA2);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA1);
        data0.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA0);
    }

    /* DATA4 */
    if (msk.s.wan_ind) {
        data4.bf.ilpb_wan_ind = cfg->wan_ind;
    }

    if (msk.s.rxqos_idx) {
        data4.bf.ilpb_rxqos_idx = cfg->rxqos_idx;
    }

    if (msk.s.well_known_mc_det_en) {
        data4.bf.ilpb_well_known_mc_det_en = cfg->well_known_mc_det_en;
    }

    if (msk.s.well_known_mc_idx) {
        data4.bf.ilpb_well_known_mc_idx = cfg->well_known_mc_idx;
    }

    if (msk.s.ilpb_unkwn_pol_mode) {
        data4.bf.ilpb_unkwn_pol_mode = cfg->ilpb_unkwn_pol_mode;
    }

    /* DATA3 */
    if (msk.s.ilpb_gemid_dynamic_mode) {
        data3.bf.ilpb_gemid_dynamic_mode = cfg->ilpb_gemid_dynamic_mode;
    }

    if (msk.s.ilpb_flowid_as_gemid) {
        data3.bf.ilpb_flowid_as_gemid = cfg->ilpb_flowid_as_gemid;
    }

    if (msk.s.unkwn_pol_idx) {
        data3.bf.ilpb_unkwn_pol_idx = cfg->unkwn_pol_idx;
    }

    if (msk.s.default_vlanid) {
        data3.bf.ilpb_default_vlanid = cfg->default_vlanid;
    }

    if (msk.s.sc_ind) {
        data3.bf.ilpb_sc_ind = cfg->sc_ind;
    }

    if (msk.s.vlan_cmd_sel_bm) {
        data3.bf.ilpb_vlan_cmd_sel_bm = cfg->vlan_cmd_sel_bm;
    }

    /* DATA2 */
    if (msk.s.default_top_vlan_cmd) {
         data2.bf.ilpb_default_top_vlan_cmd   = cfg->default_top_vlan_cmd;
    }

    if (msk.s.default_untag_cmd) {
        data2.bf.ilpb_default_untag_cmd = cfg->default_untag_cmd;
    }

    if (msk.s.inner_vid_sel_bm) {
         data2.bf.ilpb_inner_vid_sel_bm   = cfg->inner_vid_sel_bm;
    }

    if (msk.s.inner_vlan_cmd) {
         data2.bf.ilpb_inner_vlan_cmd   = cfg->inner_vlan_cmd;
    }

    if (msk.s.default_noninnertag_cmd) {
         data2.bf.ilpb_default_noninnertag_cmd  = cfg->default_noninnertag_cmd;
    }

    if (msk.s.drop_singletagged_packet) {
        data2.bf.ilpb_drop_singletagged_packet = cfg->drop_singletagged_packet > 0;
    }

    if (msk.s.drop_stag_packet) {
        data2.bf.ilpb_drop_stag_packet = cfg->drop_stag_packet > 0;
    }

    if (msk.s.drop_ctag_packet) {
        data2.bf.ilpb_drop_ctag_packet = cfg->drop_ctag_packet > 0;
    }

    if (msk.s.drop_untagged_packet) {
        data2.bf.ilpb_drop_untagged_packet = cfg->drop_untagged_packet > 0;
    }

    if (msk.s.drop_pritagged_packet) {
        data2.bf.ilpb_drop_prytagged_packet = cfg->drop_pritagged_packet > 0;
    }

    if (msk.s.drop_doubletagged_packet) {
        data2.bf.ilpb_drop_doubletagged_packet = cfg->drop_doubletagged_packet > 0;
    }

    if (msk.s.s_tpid_match) {
        data2.bf.ilpb_s_tpid_match = (cfg->s_tpid_match) ;
    }

    if (msk.s.c_tpid_match) {
        data2.bf.ilpb_c_tpid_match = (cfg->c_tpid_match) ;
    }

    if (msk.s.top_s_tpid_enable) {
        data2.bf.ilpb_top_s_tpid_enable = (cfg->top_s_tpid_enable > 0) ;
    }

    if (msk.s.top_c_tpid_enable) {
        data2.bf.ilpb_top_c_tpid_enable = (cfg->top_c_tpid_enable > 0) ;
    }

    if (msk.s.inner_s_tpid_enable) {
        data2.bf.ilpb_inner_s_tpid_enable = (cfg->inner_s_tpid_enable > 0) ;
    }

    if (msk.s.inner_c_tpid_enable) {
        data2.bf.ilpb_inner_c_tpid_enable = (cfg->inner_c_tpid_enable > 0) ;
    }

    if (msk.s.other_tpid_match) {
        data2.bf.ilpb_other_tpid_match = (cfg->other_tpid_match) ;
    }

    if (msk.s.stp_mode) {
        data2.bf.ilpb_stp_mode = cfg->stp_mode;
    }

    /* DATA1 */
    if (msk.s.station_move_en) {
        data1.bf.ilpb_station_move_en = cfg->station_move_en > 0;
    }

    if (msk.s.sa_move_dis_fwd) {
        data1.bf.ilpb_sa_move_dis_fwd = cfg->sa_move_dis_fwd > 0;
    }

    if (msk.s.loop_enable) {
        data1.bf.ilpb_loop_enable = cfg->loop_enable > 0;
    }

    if (msk.s.da_sa_swap_en) {
        data1.bf.ilpb_da_sa_swap_en = cfg->da_sa_swap_en;
    }

    if (msk.s.spcl_pkt_idx) {
        data1.bf.ilpb_spcl_pkt_idx = cfg->spcl_pkt_idx;
    }

    if (msk.s.igr_cls_lookup_en) {
        data1.bf.ilpb_igr_cls_lookup_en = cfg->igr_cls_lookup_en;
    }

    if (msk.s.cls_start) {
        data1.bf.ilpb_cls_start = cfg->cls_start;
    }

    if (msk.s.cls_length) {
        data1.bf.ilpb_cls_length = cfg->cls_length;
    }

    if(msk.s.flowid_sel_bm) {
        data1.bf.ilpb_flowid_sel_bm = cfg->flowid_sel_bm;
    }

    if (msk.s.top_802_1p_mode_sel) {
        data1.bf.ilpb_top_802_1p_mode_sel = cfg->top_802_1p_mode_sel;
    }

    if (msk.s.change_1p_if_pop) {
        data1.bf.ilpb_change_1p_if_pop = cfg->change_1p_if_pop;
    }

    if (msk.s.dot1p_mark_control_bm) {
        data0.bf.ilpb_802_1p_mark_control_bm =
            (cfg->dot1p_mark_control_bm & 1 );
        data1.bf.ilpb_802_1p_mark_control_bm =
            ((cfg->dot1p_mark_control_bm >> 1) & 7 );
    }

    /* DATA0 */
    if (msk.s.inner_802_1p_mode_sel) {
        data0.bf.ilpb_inner_802_1p_mode_sel = cfg->inner_802_1p_mode_sel;
    }

    if (msk.s.default_802_1p) {
        data0.bf.ilpb_default_802_1p = cfg->default_802_1p;
    }

    if (msk.s.dscp_markdown_en) {
        data0.bf.ilpb_dscp_markdown_en = cfg->dscp_markdown_en;
    }

    if (msk.s.dscp_mode_sel) {
        data0.bf.ilpb_dscp_mode_sel = cfg->dscp_mode_sel;
    }

    if (msk.s.dscp_mark_control_bm) {
        data0.bf.ilpb_dscp_mark_control_bm = cfg->dscp_mark_control_bm;
    }

    if (msk.s.default_dscp) {
        data0.bf.ilpb_default_dscp = cfg->default_dscp;
    }

    if (msk.s.cos_mode_sel) {
        data0.bf.ilpb_cos_mode_sel = cfg->cos_mode_sel;
    }

    if (msk.s.cos_control_bm) {
        data0.bf.ilpb_cos_control_bm = cfg->cos_control_bm;
    }

    if (msk.s.default_cos) {
        data0.bf.ilpb_default_cos = cfg->default_cos;
    }

    if(msk.s.ing_fltr_ena){
        data0.bf.ilpb_ing_fltr_ena  = (cfg->ing_fltr_ena>0);
    }

    if(msk.s.color_src_sel){
        data0.bf.ilpb_color_src_sel  = cfg->color_src_sel;
    }

    if(msk.s.dflt_premark){
        data0.bf.ilpb_dflt_premark  = cfg->dflt_premark;
    }

    if (dev == 0){
        CA_L2FE_REG_WRITE(data4.wrd, L2FE_PP_ILPB_DATA4);
        CA_L2FE_REG_WRITE(data3.wrd, L2FE_PP_ILPB_DATA3);
        CA_L2FE_REG_WRITE(data2.wrd, L2FE_PP_ILPB_DATA2);
        CA_L2FE_REG_WRITE(data1.wrd, L2FE_PP_ILPB_DATA1);
        CA_L2FE_REG_WRITE(data0.wrd, L2FE_PP_ILPB_DATA0);
        WRITE_INDIRCT_REG(dev, lpid, L2FE_PP_ILPB_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    /* Set L2Lookup table */
    /*
       LDPID 0x18   => L3_ILPB.LSPID = 0x19
       LDPID 0x19   => L3_ILPB.LSPID = 0x18
       LDPID 0x7    => L3_ILPB.LSPID = 0x19
       LDPID others => L3_ILPB.LSPID = 0x18
     */
    set_l2lkup = L2LKUP_CHECK(lpid);

    if (set_l2lkup == TRUE) {
        tbl_idx = L2LKUP_IDX(L2LKUP_LDPID(lpid));

        aal_l3fe_glb_ilpb_get(0, tbl_idx, &l2lkup_ilpb);

        if (msk.s.wan_ind) {
            l2lkup_ilpb.wan_ind = cfg->wan_ind;
        }
        l2lkup_ilpb.lspid = lpid;
        l2lkup_ilpb.l2lrn_en = TRUE;
        if (msk.s.default_vlanid) {
            l2lkup_ilpb.dft_vlan_id = cfg->default_vlanid;
        }
        if (msk.s.sc_ind) {
            l2lkup_ilpb.sc_ind = cfg->sc_ind;
        }
        if (msk.s.vlan_cmd_sel_bm) {
            l2lkup_ilpb.top_vlan_cmd_sel_bm = cfg->vlan_cmd_sel_bm;
        }
        if (msk.s.default_top_vlan_cmd) {
            l2lkup_ilpb.dft_top_vlan_cmd = cfg->default_top_vlan_cmd;
        }
        if (msk.s.default_untag_cmd) {
            l2lkup_ilpb.dft_untag_cmd = cfg->default_untag_cmd;
        }
        if (msk.s.inner_vid_sel_bm) {
            l2lkup_ilpb.inner_vlan_cmd_sel_bm = cfg->inner_vid_sel_bm;
        }
        if (msk.s.inner_vlan_cmd) {
            l2lkup_ilpb.dft_inner_vlan_cmd = cfg->inner_vlan_cmd;
        }
        if (msk.s.default_noninnertag_cmd) {
            l2lkup_ilpb.dft_noinnertag_cmd = cfg->default_noninnertag_cmd;
        }
        if (msk.s.drop_stag_packet) {
            l2lkup_ilpb.drop_stag_pkt = cfg->drop_stag_packet;
        }
        if (msk.s.drop_ctag_packet) {
            l2lkup_ilpb.drop_ctag_pkt = cfg->drop_ctag_packet;
        }
        if (msk.s.drop_untagged_packet) {
            l2lkup_ilpb.drop_untag_pkt = cfg->drop_untagged_packet;
        }
        if (msk.s.drop_pritagged_packet) {
            l2lkup_ilpb.drop_pritag_pkt = cfg->drop_pritagged_packet;
        }
        if (msk.s.drop_doubletagged_packet) {
            l2lkup_ilpb.drop_dbltag_pkt = cfg->drop_doubletagged_packet;
        }
        if (msk.s.s_tpid_match) {
            l2lkup_ilpb.s_tpid_match = cfg->s_tpid_match;
        }
        if (msk.s.c_tpid_match) {
            l2lkup_ilpb.c_tpid_match = cfg->c_tpid_match;
        }
        if (msk.s.top_s_tpid_enable) {
            l2lkup_ilpb.top_s_tpid_en = cfg->top_s_tpid_enable;
        }
        if (msk.s.top_c_tpid_enable) {
            l2lkup_ilpb.top_c_tpid_en = cfg->top_c_tpid_enable;
        }
        if (msk.s.inner_c_tpid_enable) {
            l2lkup_ilpb.inner_c_tpid_en = cfg->inner_c_tpid_enable;
        }
        if (msk.s.ing_fltr_ena) {
            l2lkup_ilpb.ingr_known_vlan_only = cfg->ing_fltr_ena;
        }

        aal_l3fe_glb_ilpb_set(0, tbl_idx, &l2lkup_ilpb);
        if (lpid == AAL_LPORT_L3_LAN)
            aal_l3fe_glb_ilpb_set(0, L2LKUP_IDX(AAL_LPORT_ETH_WAN), &l2lkup_ilpb);
        if (lpid == AAL_LPORT_L3_WAN)
            aal_l3fe_glb_ilpb_set(0, 3, &l2lkup_ilpb);
    }

    return CA_E_OK;
}

ca_status_t aal_port_ilpb_cfg_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_OUT aal_ilpb_cfg_t * cfg)
{
    L2FE_PP_ILPB_ACCESS_t oper;
    L2FE_PP_ILPB_DATA4_t data4;
    L2FE_PP_ILPB_DATA3_t data3;
    L2FE_PP_ILPB_DATA2_t data2;
    L2FE_PP_ILPB_DATA1_t data1;
    L2FE_PP_ILPB_DATA0_t data0;

    AAL_LPID_CHECK(lpid);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    memset((void *)&oper, 0, sizeof(L2FE_PP_ILPB_ACCESS_t));
    memset((void *)&data4, 0, sizeof(L2FE_PP_ILPB_DATA4_t));
    memset((void *)&data3, 0, sizeof(L2FE_PP_ILPB_DATA3_t));
    memset((void *)&data2, 0, sizeof(L2FE_PP_ILPB_DATA2_t));
    memset((void *)&data1, 0, sizeof(L2FE_PP_ILPB_DATA1_t));
    memset((void *)&data0, 0, sizeof(L2FE_PP_ILPB_DATA0_t));

    memset(cfg, 0, sizeof(aal_ilpb_cfg_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PP_ILPB_ACCESS);
        data4.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA4);
        data3.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA3);
        data2.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA2);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA1);
        data0.wrd = CA_L2FE_REG_READ(L2FE_PP_ILPB_DATA0);
    }

    cfg->wan_ind                = data4.bf.ilpb_wan_ind;
    cfg->rxqos_idx              = data4.bf.ilpb_rxqos_idx;
    cfg->well_known_mc_det_en   = data4.bf.ilpb_well_known_mc_det_en;
    cfg->well_known_mc_idx      = data4.bf.ilpb_well_known_mc_idx;
    cfg->ilpb_unkwn_pol_mode    = data4.bf.ilpb_unkwn_pol_mode;

    cfg->ilpb_gemid_dynamic_mode    = data3.bf.ilpb_gemid_dynamic_mode;
    cfg->ilpb_flowid_as_gemid       = data3.bf.ilpb_flowid_as_gemid;
    cfg->unkwn_pol_idx              = data3.bf.ilpb_unkwn_pol_idx;
    cfg->default_vlanid             = data3.bf.ilpb_default_vlanid;
    cfg->sc_ind                     = data3.bf.ilpb_sc_ind;
    cfg->vlan_cmd_sel_bm            = data3.bf.ilpb_vlan_cmd_sel_bm;

    cfg->default_top_vlan_cmd       = data2.bf.ilpb_default_top_vlan_cmd;
    cfg->default_untag_cmd          = data2.bf.ilpb_default_untag_cmd;
    cfg->inner_vid_sel_bm           = data2.bf.ilpb_inner_vid_sel_bm;
    cfg->inner_vlan_cmd             = data2.bf.ilpb_inner_vlan_cmd ;
    cfg->default_noninnertag_cmd    = data2.bf.ilpb_default_noninnertag_cmd;
    cfg->drop_singletagged_packet   = data2.bf.ilpb_drop_singletagged_packet;
    cfg->drop_stag_packet           = data2.bf.ilpb_drop_stag_packet;
    cfg->drop_ctag_packet           = data2.bf.ilpb_drop_ctag_packet;
    cfg->drop_untagged_packet       = data2.bf.ilpb_drop_untagged_packet;
    cfg->drop_pritagged_packet      = data2.bf.ilpb_drop_prytagged_packet;
    cfg->drop_doubletagged_packet   = data2.bf.ilpb_drop_doubletagged_packet;
    cfg->s_tpid_match               = data2.bf.ilpb_s_tpid_match;
    cfg->c_tpid_match               = data2.bf.ilpb_c_tpid_match;
    cfg->top_s_tpid_enable          = data2.bf.ilpb_top_s_tpid_enable;
    cfg->top_c_tpid_enable          = data2.bf.ilpb_top_c_tpid_enable;
    cfg->inner_s_tpid_enable        = data2.bf.ilpb_inner_s_tpid_enable ;
    cfg->inner_c_tpid_enable        = data2.bf.ilpb_inner_c_tpid_enable ;
    cfg->other_tpid_match           = data2.bf.ilpb_other_tpid_match;
    cfg->stp_mode                   = data2.bf.ilpb_stp_mode;

    cfg->station_move_en        = data1.bf.ilpb_station_move_en;
    cfg->sa_move_dis_fwd        = data1.bf.ilpb_sa_move_dis_fwd;
    cfg->loop_enable            = data1.bf.ilpb_loop_enable;
    cfg->da_sa_swap_en          = data1.bf.ilpb_da_sa_swap_en;
    cfg->spcl_pkt_idx           = data1.bf.ilpb_spcl_pkt_idx;
    cfg->igr_cls_lookup_en      = data1.bf.ilpb_igr_cls_lookup_en;
    cfg->cls_start              = data1.bf.ilpb_cls_start;
    cfg->cls_length             = data1.bf.ilpb_cls_length;
    cfg->flowid_sel_bm          = data1.bf.ilpb_flowid_sel_bm;
    cfg->top_802_1p_mode_sel    = data1.bf.ilpb_top_802_1p_mode_sel;
    cfg->change_1p_if_pop       = data1.bf.ilpb_change_1p_if_pop;
    cfg->dot1p_mark_control_bm  = (data1.bf.ilpb_802_1p_mark_control_bm << 1) | data0.bf.ilpb_802_1p_mark_control_bm;

    cfg->inner_802_1p_mode_sel  = data0.bf.ilpb_inner_802_1p_mode_sel;
    cfg->default_802_1p         = data0.bf.ilpb_default_802_1p;
    cfg->dscp_markdown_en       = data0.bf.ilpb_dscp_markdown_en;
    cfg->dscp_mode_sel          = data0.bf.ilpb_dscp_mode_sel;
    cfg->dscp_mark_control_bm   = data0.bf.ilpb_dscp_mark_control_bm;
    cfg->default_dscp           = data0.bf.ilpb_default_dscp;
    cfg->cos_mode_sel           = data0.bf.ilpb_cos_mode_sel;
    cfg->cos_control_bm         = data0.bf.ilpb_cos_control_bm;
    cfg->default_cos            = data0.bf.ilpb_default_cos;
    cfg->ing_fltr_ena           = data0.bf.ilpb_ing_fltr_ena;
    cfg->color_src_sel          = data0.bf.ilpb_color_src_sel;
    cfg->dflt_premark           = data0.bf.ilpb_dflt_premark;

    return CA_E_OK;
}

ca_status_t aal_port_dpb_ctrl_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint8_t ldpid_pattern)
{
    L2FE_PP_DPB_CTRL_t cfg;

    __AAL_PORT_LOCK(dev);

    memset((void *)&cfg, 0, sizeof(L2FE_PP_DPB_CTRL_t));
    cfg.bf.ldpid_pattern = ldpid_pattern;

    if (dev == 0){
        CA_L2FE_REG_WRITE(cfg.wrd, L2FE_PP_DPB_CTRL);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_dpb_ctrl_get(CA_IN ca_device_id_t dev,
                CA_OUT ca_uint8_t * ldpid_pattern)
{
    L2FE_PP_DPB_CTRL_t cfg;

    __AAL_PORT_LOCK(dev);

    memset((void *)&cfg, 0, sizeof(L2FE_PP_DPB_CTRL_t));
    if (dev == 0){
        cfg.wrd = CA_L2FE_REG_READ(L2FE_PP_DPB_CTRL);
    }
    *ldpid_pattern = cfg.bf.ldpid_pattern;

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

/* destination port behavior configuration */
ca_status_t aal_port_dpb_cfg_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid_cos,
                CA_IN aal_dpb_cfg_msk_t msk,
                CA_IN aal_dpb_cfg_t * cfg)
{
    L2FE_PP_DPB_ACCESS_t oper;
    L2FE_PP_DPB_DATA_t data;

    AAL_LPID_CHECK(lpid_cos);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    __AAL_PORT_LOCK(dev);

    memset((void *)&oper, 0, sizeof(L2FE_PP_DPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PP_DPB_DATA_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid_cos, L2FE_PP_DPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PP_DPB_DATA);
    }

    if (msk.s.bypass) {
        data.bf.bypass = cfg->bypass > 0;
    }

    if (msk.s.ldpid) {
        data.bf.ldpid = cfg->ldpid;
    }

    if (msk.s.cos) {
        data.bf.cos = cfg->cos;
    }

    if (dev == 0){
        CA_L2FE_REG_WRITE(data.wrd, L2FE_PP_DPB_DATA);
        WRITE_INDIRCT_REG(dev, lpid_cos, L2FE_PP_DPB_ACCESS);
    }
    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_dpb_cfg_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid_cos,
                CA_OUT aal_dpb_cfg_t * cfg)
{
    L2FE_PP_DPB_ACCESS_t oper;
    L2FE_PP_DPB_DATA_t data;

    AAL_LPID_CHECK(lpid_cos);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    memset((void *)&oper, 0, sizeof(L2FE_PP_DPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PP_DPB_DATA_t));

    memset(cfg, 0, sizeof(aal_dpb_cfg_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid_cos, L2FE_PP_DPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PP_DPB_DATA);
    }

    cfg->bypass = data.bf.bypass;
    cfg->ldpid = data.bf.ldpid;
    cfg->cos = data.bf.cos;

    return CA_E_OK;
}

/* egress logical port behavior configuration */
ca_status_t aal_port_elpb_cfg_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_IN aal_elpb_cfg_msk_t msk,
                CA_IN aal_elpb_cfg_t * cfg)
{
    L2FE_PLC_ELPB_ACCESS_t oper;
    L2FE_PLC_ELPB_DATA0_t data;
    L2FE_PLC_ELPB_DATA1_t data1;
    l3fe_glb_elpb_t l2lkup_elpb;

    AAL_LPID_CHECK(lpid);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    __AAL_PORT_LOCK(dev);

    memset((void *)&oper, 0, sizeof(L2FE_PLC_ELPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PLC_ELPB_DATA0_t));
    memset((void *)&data1, 0, sizeof(L2FE_PLC_ELPB_DATA1_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PLC_ELPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PLC_ELPB_DATA0);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PLC_ELPB_DATA1);
    }

    if(msk.s.elpb_use_tx_vlan_lkup_acl)
        data1.bf.elpb_use_tx_vlan_lkup_acl = cfg->elpb_use_tx_vlan_lkup_acl;

    if(msk.s.elpb_802_1p_ctrl_sel)
        data1.bf.elpb_802_1p_ctrl_sel = cfg->elpb_802_1p_ctrl_sel;

    if(msk.s.elpb_top_802_1p_mode_sel)
        data1.bf.elpb_top_802_1p_mode_sel = cfg->elpb_top_802_1p_mode_sel;

    if(msk.s.elpb_change_1p_if_pop)
        data1.bf.elpb_change_1p_if_pop = cfg->elpb_change_1p_if_pop;

    if(msk.s.elpb_802_1p_mark_control_bm)
    {
        data1.bf.elpb_802_1p_mark_control_bm = ((cfg->elpb_802_1p_mark_control_bm >> 1) & 0x7);
        data.bf.elpb_802_1p_mark_control_bm = (cfg->elpb_802_1p_mark_control_bm & 0x1);
    }

    if(msk.s.elpb_inner_802_1p_mode_sel)
        data.bf.elpb_inner_802_1p_mode_sel = cfg->elpb_inner_802_1p_mode_sel;

    if(msk.s.elpb_dscp_markdown_en)
        data.bf.elpb_dscp_markdown_en = cfg->elpb_dscp_markdown_en;

    if(msk.s.elpb_dscp_ctrl_sel)
        data.bf.elpb_dscp_ctrl_sel = cfg->elpb_dscp_ctrl_sel;

    if(msk.s.elpb_dscp_mode_sel)
        data.bf.elpb_dscp_mode_sel = cfg->elpb_dscp_mode_sel;

    if(msk.s.elpb_dscp_mark_control_bm)
        data.bf.elpb_dscp_mark_control_bm = cfg->elpb_dscp_mark_control_bm;

    if(msk.s.egr_cls_cos_ena)
        data.bf.elpb_egr_cls_cos_sel = (cfg->egr_cls_cos_ena > 0);

    if(msk.s.egr_cls_flow_id_ena)
        data.bf.elpb_egr_cls_flowid_sel = (cfg->egr_cls_flow_id_ena > 0);

    if (msk.s.egr_cls_lookup_en)
        data.bf.elpb_egr_cls_lookup_en = cfg->egr_cls_lookup_en > 0;

    if (msk.s.egr_cls_start)
        data.bf.elpb_egr_cls_start = cfg->egr_cls_start;

    if (msk.s.egr_cls_length)
        data.bf.elpb_egr_cls_length = cfg->egr_cls_length;

    if (msk.s.egr_dest_wan)
        data.bf.elpb_egr_dest_wan = cfg->egr_dest_wan;

    if (msk.s.egr_ve_srch_en)
        data.bf.elpb_egr_ve_srch_en = cfg->egr_ve_srch_en > 0;

    if (msk.s.egr_port_stp_status)
        data.bf.elpb_egr_port_stp_status = cfg->egr_port_stp_status > 0;

    if (msk.s.egr_vlan_aware_mode)
        data.bf.elpb_egr_vlan_aware_mode = cfg->egr_vlan_aware_mode > 0;


    if (dev == 0){
        CA_L2FE_REG_WRITE(data.wrd, L2FE_PLC_ELPB_DATA0);
        CA_L2FE_REG_WRITE(data1.wrd, L2FE_PLC_ELPB_DATA1);
        WRITE_INDIRCT_REG(dev, lpid, L2FE_PLC_ELPB_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    /* Set L2Lookup ELPB */

    if (msk.s.egr_vlan_aware_mode) {
        memset((void *)&l2lkup_elpb, 0, sizeof(l3fe_glb_elpb_t));
        /* egr_vlan_aware_mode - 1: keep vlan, 0: pop all
         * l2lkup_elpb - 1: pop all, 0: keep vlan
         */

        aal_l3fe_glb_elpb_get(0, &l2lkup_elpb);
        if (cfg->egr_vlan_aware_mode == 0)
            l2lkup_elpb.ldpid_vec |= 1ll << lpid;
        else
            l2lkup_elpb.ldpid_vec &= ~(1ll << lpid);
        aal_l3fe_glb_elpb_set(0, &l2lkup_elpb);
    }

    return CA_E_OK;
}

ca_status_t aal_port_elpb_cfg_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_OUT aal_elpb_cfg_t * cfg)
{
    L2FE_PLC_ELPB_ACCESS_t oper;
    L2FE_PLC_ELPB_DATA0_t data;
    L2FE_PLC_ELPB_DATA1_t data1;

    AAL_LPID_CHECK(lpid);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    memset((void *)&oper, 0, sizeof(L2FE_PLC_ELPB_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PLC_ELPB_DATA0_t));
    memset((void *)&data1, 0, sizeof(L2FE_PLC_ELPB_DATA1_t));

    memset(cfg, 0, sizeof(aal_elpb_cfg_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PLC_ELPB_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PLC_ELPB_DATA0);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PLC_ELPB_DATA1);
    }

    cfg->elpb_use_tx_vlan_lkup_acl = data1.bf.elpb_use_tx_vlan_lkup_acl;
    cfg->elpb_802_1p_ctrl_sel = data1.bf.elpb_802_1p_ctrl_sel;
    cfg->elpb_top_802_1p_mode_sel = data1.bf.elpb_top_802_1p_mode_sel;
    cfg->elpb_change_1p_if_pop = data1.bf.elpb_change_1p_if_pop;
    cfg->elpb_802_1p_mark_control_bm = (data.bf.elpb_802_1p_mark_control_bm | (data1.bf.elpb_802_1p_mark_control_bm << 1));

    cfg->elpb_inner_802_1p_mode_sel = data.bf.elpb_inner_802_1p_mode_sel;
    cfg->elpb_dscp_markdown_en = data.bf.elpb_dscp_markdown_en;
    cfg->elpb_dscp_ctrl_sel = data.bf.elpb_dscp_ctrl_sel;
    cfg->elpb_dscp_mode_sel = data.bf.elpb_dscp_mode_sel;
    cfg->elpb_dscp_mark_control_bm = data.bf.elpb_dscp_mark_control_bm;
    cfg->egr_cls_cos_ena = data.bf.elpb_egr_cls_cos_sel;
    cfg->egr_cls_flow_id_ena = data.bf.elpb_egr_cls_flowid_sel;
    cfg->egr_cls_lookup_en = data.bf.elpb_egr_cls_lookup_en;
    cfg->egr_cls_start = data.bf.elpb_egr_cls_start;
    cfg->egr_cls_length = data.bf.elpb_egr_cls_length;
    cfg->egr_dest_wan = data.bf.elpb_egr_dest_wan;
    cfg->egr_ve_srch_en = data.bf.elpb_egr_ve_srch_en;
    cfg->egr_port_stp_status = data.bf.elpb_egr_port_stp_status;
    cfg->egr_vlan_aware_mode = data.bf.elpb_egr_vlan_aware_mode;

    return CA_E_OK;
}

ca_status_t aal_port_mmshp_check_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_IN ca_uint64_t bmp)
{
    L2FE_PLE_PORT_MMSHP_CHK_ACCESS_t oper;
    L2FE_PLE_PORT_MMSHP_CHK_DATA1_t data1;
    L2FE_PLE_PORT_MMSHP_CHK_DATA0_t data0;

    AAL_LPID_CHECK(lpid);

    __AAL_PORT_LOCK(dev);

    //ca_printf("mmshp_check port %d, bmp 0x%x\r\n", lpid, bmp);
    memset((void *)&oper, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_ACCESS_t));
    memset((void *)&data1, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_DATA1_t));
    memset((void *)&data0, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_DATA0_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PLE_PORT_MMSHP_CHK_ACCESS);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PLE_PORT_MMSHP_CHK_DATA1);
        data0.wrd = CA_L2FE_REG_READ(L2FE_PLE_PORT_MMSHP_CHK_DATA0);
    }

    data1.bf.ldpid_bm = (bmp >> 32) & 0x00000000ffffffff;
    data0.bf.ldpid_bm = bmp & 0x00000000ffffffff;

    if (dev == 0){
        CA_L2FE_REG_WRITE(data1.wrd, L2FE_PLE_PORT_MMSHP_CHK_DATA1);
        CA_L2FE_REG_WRITE(data0.wrd, L2FE_PLE_PORT_MMSHP_CHK_DATA0);
        WRITE_INDIRCT_REG(dev, lpid, L2FE_PLE_PORT_MMSHP_CHK_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_mmshp_check_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t lpid,
                CA_OUT ca_uint64_t * bmp)
{
    L2FE_PLE_PORT_MMSHP_CHK_ACCESS_t oper;
    L2FE_PLE_PORT_MMSHP_CHK_DATA1_t data1;
    L2FE_PLE_PORT_MMSHP_CHK_DATA0_t data0;

    AAL_LPID_CHECK(lpid);

    __AAL_PORT_LOCK(dev);

    memset((void *)&oper, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_ACCESS_t));
    memset((void *)&data1, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_DATA1_t));
    memset((void *)&data0, 0, sizeof(L2FE_PLE_PORT_MMSHP_CHK_DATA0_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, lpid, L2FE_PLE_PORT_MMSHP_CHK_ACCESS);
        data1.wrd = CA_L2FE_REG_READ(L2FE_PLE_PORT_MMSHP_CHK_DATA1);
        data0.wrd = CA_L2FE_REG_READ(L2FE_PLE_PORT_MMSHP_CHK_DATA0);
    }

    *bmp = (((ca_uint64_t) (data1.bf.ldpid_bm) << 32) & 0xffffffff00000000) | (((ca_uint64_t) (data0.bf.ldpid_bm) & 0x00000000ffffffff));

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_trunk_group_hash_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t group_id,
                CA_IN aal_trunk_hash_key_t key)
{
    L2FE_PLE_TRUNK_GRP_CTRL_0_t cfg;

    AAL_TRUNK_CHECK(group_id);

    __AAL_PORT_LOCK(dev);

    memset((void *)&cfg, 0, sizeof(L2FE_PLE_TRUNK_GRP_CTRL_0_t));
    if (key.field_sel_cos == 1) {
        cfg.bf.trunk_hash_field_sel_cos = 1;
    }

    if (key.field_sel_l4_dp == 1) {
        cfg.bf.trunk_hash_field_sel_l4_dp = 1;
    }

    if (key.field_sel_l4_sp == 1) {
        cfg.bf.trunk_hash_field_sel_l4_sp = 1;
    }

    if (key.field_sel_ip_da == 1) {
        cfg.bf.trunk_hash_field_sel_ip_da = 1;
    }

    if (key.field_sel_ip_sa == 1) {
        cfg.bf.trunk_hash_field_sel_ip_sa = 1;
    }

    if (key.field_sel_tx_top_vid == 1) {
        cfg.bf.trunk_hash_field_sel_tx_top_vid = 1;
    }

    if (key.field_sel_mac_da == 1) {
        cfg.bf.trunk_hash_field_sel_mac_da = 1;
    }

    if (key.field_sel_mac_sa == 1) {
        cfg.bf.trunk_hash_field_sel_mac_sa = 1;
    }
    if (dev == 0){
        if (group_id == 1) {
            CA_L2FE_REG_WRITE(cfg.wrd, L2FE_PLE_TRUNK_GRP_CTRL_1);
        } else {
            CA_L2FE_REG_WRITE(cfg.wrd, L2FE_PLE_TRUNK_GRP_CTRL_0);
        }
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_trunk_group_hash_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t group_id,
                CA_OUT aal_trunk_hash_key_t * key)
{
    L2FE_PLE_TRUNK_GRP_CTRL_0_t cfg;

    AAL_TRUNK_CHECK(group_id);
    AAL_ASSERT_RET(key, CA_E_PARAM);

    if (dev == 0){
        if (group_id == 1) {
            cfg.wrd = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_CTRL_1);
        } else {
            cfg.wrd = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_CTRL_0);
        }
    }

    memset(key, 0, sizeof(aal_trunk_hash_key_t));

    if (cfg.bf.trunk_hash_field_sel_cos == 1) {
        key->field_sel_cos = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_l4_dp == 1) {
        key->field_sel_l4_dp = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_l4_sp == 1) {
        key->field_sel_l4_sp = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_ip_da == 1) {
        key->field_sel_ip_da = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_ip_sa == 1) {
        key->field_sel_ip_sa = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_tx_top_vid == 1) {
        key->field_sel_tx_top_vid = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_mac_da == 1) {
        key->field_sel_mac_da = 1;
    }

    if (cfg.bf.trunk_hash_field_sel_mac_sa == 1) {
        key->field_sel_mac_sa = 1;
    }

    return CA_E_OK;
}

ca_status_t aal_port_trunk_group_member_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t group_id,
                CA_IN ca_uint64_t bmp)
{
    L2FE_PLE_TRUNK_GRP_0_MEMBER_0_t cfg0;
    L2FE_PLE_TRUNK_GRP_0_MEMBER_1_t cfg1;

    AAL_TRUNK_CHECK(group_id);

    __AAL_PORT_LOCK(dev);

    memset((void *)&cfg0, 0, sizeof(L2FE_PLE_TRUNK_GRP_0_MEMBER_0_t));
    memset((void *)&cfg1, 0, sizeof(L2FE_PLE_TRUNK_GRP_0_MEMBER_1_t));

    cfg0.wrd = (ca_uint32_t) (bmp & 0xFFFFFFFF);
    cfg1.wrd = (ca_uint32_t) (bmp >> 32 & 0xFFFFFFFF);

    if (dev == 0){
        if (group_id == 1) {
            CA_L2FE_REG_WRITE(cfg0.wrd, L2FE_PLE_TRUNK_GRP_1_MEMBER_0);
            CA_L2FE_REG_WRITE(cfg1.wrd, L2FE_PLE_TRUNK_GRP_1_MEMBER_1);

        } else {
            CA_L2FE_REG_WRITE(cfg0.wrd, L2FE_PLE_TRUNK_GRP_0_MEMBER_0);
            CA_L2FE_REG_WRITE(cfg1.wrd, L2FE_PLE_TRUNK_GRP_0_MEMBER_1);
        }
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_trunk_group_member_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t group_id,
                CA_OUT ca_uint64_t * bmp)
{
    ca_uint32_t cfg0, cfg1;

    AAL_TRUNK_CHECK(group_id);
    AAL_ASSERT_RET(bmp, CA_E_PARAM);

    if (dev == 0){
        if (group_id == 1) {
            cfg0 = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_1_MEMBER_0);
            cfg1 = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_1_MEMBER_1);
        } else {
            cfg0 = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_0_MEMBER_0);
            cfg1 = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_GRP_0_MEMBER_1);
        }
    }
    else
        return CA_E_PARAM;

    *bmp = (ca_uint64_t) cfg1 << 32;
    *bmp |= (ca_uint64_t) cfg0;

    return CA_E_OK;
}

ca_status_t aal_port_trunk_dest_map_set(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t trunk_idx,
                CA_IN ca_uint32_t ldpid)
{
    L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS_t oper;
    L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA_t data;

    AAL_LPID_CHECK(ldpid);
    if (trunk_idx > 32 * AAL_PORT_NUM_TRUNK) {
        return CA_E_PARAM;
    }

    __AAL_PORT_LOCK(dev);

    memset((void *)&oper, 0, sizeof(L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA_t));

    if (dev == 0){
        READ_INDIRCT_REG(dev, trunk_idx, L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA);

        data.bf.ldpid = ldpid;
        CA_L2FE_REG_WRITE(data.wrd, L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA);
        WRITE_INDIRCT_REG(dev, trunk_idx, L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_trunk_dest_map_get(CA_IN ca_device_id_t dev,
                CA_IN ca_uint32_t trunk_idx,
                CA_OUT ca_uint32_t * ldpid)
{
    L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS_t oper;
    L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA_t data;

    AAL_ASSERT_RET(ldpid, CA_E_PARAM);

    if (trunk_idx > 32 * AAL_PORT_NUM_TRUNK) {
        return CA_E_PARAM;
    }

    memset((void *)&oper, 0, sizeof(L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS_t));
    memset((void *)&data, 0, sizeof(L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA_t));
    *ldpid = 0;

    if (dev == 0){
        READ_INDIRCT_REG(dev, trunk_idx, L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS);
        data.wrd = CA_L2FE_REG_READ(L2FE_PLE_TRUNK_DEST_MAP_TBL_DATA);
    }

    *ldpid = data.bf.ldpid;

    return CA_E_OK;
}

ca_status_t aal_port_arb_ldpid_pdpid_map_set(
    CA_IN ca_device_id_t dev,
    CA_IN ca_uint32_t  my_mac_flag,
    CA_IN ca_uint32_t  dbuf_flag,
    CA_IN ca_uint32_t ldpid,
    CA_IN ca_uint32_t pdpid
)
{
    ca_uint32_t temp_ind = 0;
    AAL_LPID_CHECK(ldpid);
    if(pdpid >15){
        return CA_E_PARAM;
    }

    __AAL_PORT_LOCK(dev);
    if (dev == 0){
        CA_L2FE_REG_WRITE(pdpid, L2FE_ARB_PDPID_MAP_TBL_DATA);
    }

    temp_ind = (((my_mac_flag > 0) << 7) | ((dbuf_flag > 0) << 6) |ldpid);

    if (dev == 0){
        WRITE_INDIRCT_REG(dev, temp_ind, L2FE_ARB_PDPID_MAP_TBL_ACCESS);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;
}

ca_status_t aal_port_arb_ldpid_pdpid_map_get(
    CA_IN ca_device_id_t dev,
    CA_IN ca_uint32_t  my_mac_flag,
    CA_IN ca_uint32_t  dbuf_flag,
    CA_IN ca_uint32_t ldpid,
    CA_OUT ca_uint32_t *pdpid
)
{
    ca_uint32_t temp_ind = 0;

    AAL_LPID_CHECK(ldpid);

    AAL_ASSERT_RET(pdpid, CA_E_PARAM);

    __AAL_PORT_LOCK(dev);

    temp_ind = (((my_mac_flag > 0) << 7) | ((dbuf_flag > 0) << 6) |ldpid);
    if (dev == 0){
        READ_INDIRCT_REG(dev, temp_ind, L2FE_ARB_PDPID_MAP_TBL_ACCESS);
        *pdpid = CA_L2FE_REG_READ(L2FE_ARB_PDPID_MAP_TBL_DATA);
    }

    __AAL_PORT_UNLOCK(dev);

    return CA_E_OK;

}

void __aal_port_change_1p_if_pop_en(ca_device_id_t dev, ca_uint8_t enable)
{
    L2FE_GLB_CTRL_t cfg;

    if (dev == 0){
        cfg.wrd = CA_L2FE_REG_READ(L2FE_GLB_CTRL);
    }

    cfg.bf.change_ip_if_pop_en = enable > 0;

    if (dev == 0){
        CA_L2FE_REG_WRITE(cfg.wrd, L2FE_GLB_CTRL);
    }


}
ca_port_type_t aal_port_type_get(ca_uint32_t lpid)
{
    ca_port_type_t port_type = CA_PORT_TYPE_ETHERNET;

    if((lpid >= CA_PORT_ID_NI0) && (lpid <= CA_PORT_ID_NI7))
        port_type = CA_PORT_TYPE_ETHERNET;
    else if(lpid >= CA_PORT_ID_CPU0 && lpid <= CA_PORT_ID_CPU7)
        port_type = CA_PORT_TYPE_CPU;
    else if(lpid == CA_PORT_ID_L3_WAN || lpid == CA_PORT_ID_L3_LAN)
        port_type = CA_PORT_TYPE_L2RP;

    return port_type;
}
void __aal_port_pipeline_clk_num_set(ca_uint32_t clk_num)
{
#if 1//def CONFIG_ARCH_CORTINA_G3LITE
    L2FE_GLB_CTRL_t fe_glb_ctrl;
    NI_HV_GLB_RXMUX_CTRL_CFG_t ni_glb_nimux_ctrl;

    fe_glb_ctrl.wrd = CA_L2FE_REG_READ(L2FE_GLB_CTRL);
    fe_glb_ctrl.bf.pipeline_clk_num = clk_num;
    CA_L2FE_REG_WRITE(fe_glb_ctrl.wrd, L2FE_GLB_CTRL);
    ni_glb_nimux_ctrl.wrd = CA_NE_REG_READ(NI_HV_GLB_RXMUX_CTRL_CFG);
    ni_glb_nimux_ctrl.bf.next_sop_delay = clk_num;
    CA_NE_REG_WRITE(ni_glb_nimux_ctrl.wrd, NI_HV_GLB_RXMUX_CTRL_CFG);
#endif
}

#define __MAX_ETH_PORT_NUM             8 /* MAX Ethernet port number per Chip */
#define __MAX_ETH_PORT_ID              (__MAX_ETH_PORT_NUM-1) /* MAX Ethernet port index per Chip */

#define __ETH_IS_XGE(aal_port)  ((aal_port) >= AAL_NI_PORT_XGE0 && (aal_port) <= AAL_NI_PORT_XGE2)
#define __ETH_IS_GE(aal_port)   ((aal_port) >= AAL_NI_PORT_GE0 && (aal_port) <= AAL_NI_PORT_GE4)
#define __FOR_ALL_ETH_PORTS_DO(port)   \
        for((port)=0; (port)<=8; ++(port)) \
            if ((1<<port) & g_eth_port_drv.eth_port_bmp)

#define __FOR_ALL_GE_PORTS_DO(port)   \
        for((port)=0; (port)<=AAL_NI_GE_PORT_END; ++(port)) \
            if ((1<<port) & g_eth_port_drv.ge_port_bmp)

#define __FOR_ALL_XE_PORTS_DO(port)   \
        for((port)=0; (port)<=__MAX_ETH_PORT_ID; ++(port)) \
            if ((1<<port) & g_eth_port_drv.xe_port_bmp)

#define __FOR_ALL_MDIO_DO(port)   \
        for((port)=1; (port)<=CA_MDIO_ADDR_MAX; ++(port))

#define __ETH_INTF_IS_USXGMII(aal_port)  (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_USXGMII)
#define __ETH_INTF_IS_SGMII(aal_port)    (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_GE_SGMII)
#define __ETH_INTF_IS_HISGMII(aal_port)  (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_HISGMII)
#define __ETH_INTF_IS_XFI(aal_port)      (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_XFI)
#define __ETH_INTF_IS_RGMII(aal_port)    (g_eth_port_drv.eth_if_mode[aal_port] == AAL_NI_IF_MODE_GE_RGMII)

#define __ETH_PHY_ADDR(aal_port)    (g_eth_port_drv.eth_mdio_addr[aal_port])
#define __ETH_DETECTED_PHY(aal_port)  (g_eth_port_drv.ext_phy_detected[aal_port])/* check PHY existance by AAL port */

static ca_eth_port_speed_t __phy2api_speed_map[] = {
    CA_PORT_SPEED_10M, CA_PORT_SPEED_100M, CA_PORT_SPEED_1G, 
    CA_PORT_SPEED_10G, CA_PORT_SPEED_AUTO, CA_PORT_SPEED_2_5G, CA_PORT_SPEED_5G
};

static aal_phy_speed_mode_t __api2phy_speed_map[] = {
    AAL_PHY_SPEED_10, AAL_PHY_SPEED_10, AAL_PHY_SPEED_100, AAL_PHY_SPEED_1000,
    AAL_PHY_SPEED_2500, AAL_PHY_SPEED_10000, AAL_PHY_SPEED_5000, AAL_PHY_SPEED_10000 
};

#define _SPEED2PHY(speed)     ((speed) <= CA_PORT_SPEED_AUTO  ? __api2phy_speed_map[speed] : AAL_PHY_SPEED_10)
#define _PHY2SPEED(phy_speed) ((phy_speed) <= AAL_PHY_SPEED_5000  ? __phy2api_speed_map[phy_speed] : CA_PORT_SPEED_AUTO)

#define _SPEED2MAC(speed)     ((speed) > CA_PORT_SPEED_INVALID  ? (speed) - 1: (speed))
#define _MAC2SPEED(mac_speed) ((mac_speed)+1)

#define _PHY_SPEED2MAC_SPEED(phy_speed) _SPEED2MAC(_PHY2SPEED(phy_speed))
#define _MAC_SPEED2PHY_SPEED(mac_speed) _SPEED2PHY(_MAC2SPEED(mac_speed))

#define __ETH_PORT_SCAN_IS_ENABLE(aal_port)  (0x1 & (g_eth_port_drv.eth_port_scan_enable >> (aal_port)))

#ifdef CONFIG_CORTINA_BOARD_FPGA
#define __ETH_IS_CONNECT_SWITCH(aal_port)  (FALSE)
#else
#define __ETH_IS_CONNECT_SWITCH(aal_port)  (FALSE)
#endif

#define __ETH_EEE_IS_ENABLE(aal_port)  g_eth_port_drv.eee_enable[aal_port]

#define __ETH_PORT_LINK_SCAN_INTERVAL  1000 /* 1 seconds */

#define __ETH_INIT_CHK(dev)  do {\
                                if (!g_eth_port_init_done) {\
                                    ca_printf("ETH Port API is NOT initialized ready\r\n"); \
                                    return CA_E_INIT; \
                                }\
                             }while(0)

typedef struct {
    ca_uint      eth_port_lock; /* driver spin lock */
    ca_boolean_t eth_xfi_autoreset; /* auto reset XFI when no link received */
    ca_boolean_t eth_port_scan_stop; /* timer or thread id */
    ca_uint8_t   eth_dynamic_deep_queue; /* enable dynamic deep Queue feature or NOT for etherent ports */
    ca_uint32_t  eth_port_scan_enable; /* port scan bitmap */
    ca_uint32_t  ge_port_bmp; /* GE port bitmap */
    ca_uint32_t  xe_port_bmp; /* XE port bitmap */
    ca_uint32_t  eth_port_bmp; /* port bitmap */
    ca_uint8_t   port_enable[__MAX_ETH_PORT_NUM]; /* port admin state */
    ca_uint8_t   eth_mdio_addr[__MAX_ETH_PORT_NUM]; /* port PHY MIDO addr, if it is 0xff, means valid */
    ca_uint8_t   ext_phy_detected[__MAX_ETH_PORT_NUM]; /* port connected to external PHY or NOT */
    ca_uint8_t   disable_phy2mac_sync[__MAX_ETH_PORT_NUM]; /* disable PHY to MAC auto-sync*/
    ca_uint8_t   eth_if_mode[__MAX_ETH_PORT_NUM]; /* port PMD interface */
    ca_uint8_t   autoneg[__MAX_ETH_PORT_NUM]; /* port autoneg configure state */
    ca_uint8_t   speed[__MAX_ETH_PORT_NUM]; /* port speed configure state */
    ca_uint8_t   eee_enable[__MAX_ETH_PORT_NUM];  /* port EEE enable state */
    ca_eth_port_ability_t  adv[__MAX_ETH_PORT_NUM]; /* port advertize configure state */
} __eth_port_drv_t;

ca_uint8_t g_eth_port_init_done = 0;/* ETH drv init successfully */
__eth_port_drv_t g_eth_port_drv;

 extern int ni_initialized; /* ni driver init done flag */

#ifdef CONFIG_CORTINA_BOARD_FPGA
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M,
    AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M, AAL_NI_MAC_SPEED_100M
};
#else
ca_uint32_t __default_mac_speed[] = {
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_1000M,
    AAL_NI_MAC_SPEED_1000M, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_10G, AAL_NI_MAC_SPEED_10G
};
#endif

static ca_status_t __eth_chk_port_bmp(ca_device_id_t device_id, ca_uint32_t *ge_bmp, ca_uint32_t *xe_bmp)
{
    ca_status_t              ret = CA_E_OK;
    aal_ni_eth_capability_t  cap;

    memset(&cap, 0, sizeof(cap));

    ret = aal_ni_eth_capability_get(device_id, &cap);
    CA_RET_VALUE_CHECK(ret);

    if ((*ge_bmp | cap.ge_bmp) != cap.ge_bmp) {
        ca_printf("SCFG GE bmp %#x is ilegal, Chip GE capability is %#x\n", *ge_bmp, cap.ge_bmp);
        *ge_bmp &= cap.ge_bmp;
    }

    if ((*xe_bmp | cap.xe_bmp) != cap.xe_bmp) {
        ca_printf("SCFG XE bmp %#x is ilegal, Chip XE capability is %#x\n", *xe_bmp, cap.xe_bmp);
        *xe_bmp &= cap.xe_bmp;
    }

    return CA_E_OK;
}

static ca_status_t __eth_port_scfg_load(ca_device_id_t device_id)
{
    ca_uint8_t  scfg_array[CFG_ID_PHY_INTERFACE_LEN + CFG_ID_PHY_BASE_ADDR_LEN] = {0};
    ca_status_t ret = CA_E_OK;
    ca_uint32_t phy_id = 0;
    ca_uint32_t default_mdio[] = {/* FPGA board has 8 Eth Ports */
    1, 2, 3, 4, 16, 17, 18, 19
    };

/**
    Port Interface Configuration
FOR G3:
    LAN Port 0~3 ASIC: support GMII_GE/MII mode1.
    In GMII_GE mode, support 1Gbps and full-duplex2.
    In MII mode, support 10/100Mbps and full/half-duplex
    LAN Port 0~3 FPGA: support RGMII_GE/RGMII_FE1.
    In RGMII_GE mode, support 1Gbps and full-duplex2.
    In RGMII_FE mode, support 10/100Mpbs and half/full-duplex
    LAN Port 4 ASIC: support GMII_GE/RGMII_GE/RGMII_FE mode
    1. In GMII_GE mode, support 1000/100/10Mbps;
    2. In GMII_GE mode, support full-duplex at 1000Mbps;
    3. In GMII_GE mode, support half/full-duplex at 10/100Mbps;
    4. In RGMII_GE mode, support full-duplex at 1000Mbps;
    5. In RGMII_FE mode, support 10/100Mbps and full/half-duplex
    LAN Port 4 FPGA: support RGMII_GE/RGMII_FE mode
    1. In RGMII_GE mode, support full-duplex at 1000Mbps;
    2. In RGMII_FE mode, support 10/100Mbps and full/half-duplex
    Note: when Port 4 connects to SGMII PCS, it should set to GMII_GE mode.
    In thismode,port 4 can run 1000Mbps/full-duplex, 100Mbps/10Mbps/full or half-duplex.
FOR Raptor:Port 0~3 can be configured as 10/100Mbps MII mode or 1000Mbps GMII-like SerDesmode
    set int_cfg == GMII_GE for GMII-like SerDes mode
    set int_cfg == MII for 10/100M MII mode
    Port 4 can be configured as 10/100Mbps MII mode or RGMII_GE/RGMII_FE mode
    set int_cfg == MII for 10/100M MII mode
    set int_cfg == RGMII_GE for 1000M RGMII_GE mode
    set int_cfg == RGMII_FE for 100/10M RGMII_FE mode
*/
#ifdef CONFIG_CORTINA_BOARD_FPGA
    ca_uint32_t default_if_mode[] = {
        AAL_NI_IF_MODE_FE_RGMII, /* p0 */
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII,
        AAL_NI_IF_MODE_FE_RGMII, /* p4 FE */
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI    /* p7 */
    };
#else  /* CONFIG_CORTINA_BOARD_FPGA */
    ca_uint32_t default_if_mode[] = {
        AAL_NI_IF_MODE_GE_GMII, /* p0 */
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII,
        AAL_NI_IF_MODE_GE_GMII, /* p4 GMII, check with Aaron/Lidong for G3/HGU Port 4? */       
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI   ,
        AAL_NI_IF_MODE_RXAUI    /* p7 */
    };
#endif /* CONFIG_CORTINA_BOARD_FPGA */

    /* NOTE: should Load ethernet port bitmap firstly */
#ifdef CONFIG_CORTINA_BOARD_FPGA
    g_eth_port_drv.ge_port_bmp = 0x3f;
    g_eth_port_drv.xe_port_bmp = 0x00;
#else
    g_eth_port_drv.ge_port_bmp = 0;
    g_eth_port_drv.xe_port_bmp = 0;
    ret = scfg_read(device_id, CFG_ID_GE_PORT_BIT_MAP, sizeof(ca_uint32_t), &g_eth_port_drv.ge_port_bmp);
    if (CA_E_OK != ret) {
        printk("!Err: read CA_CFG_ID_GE_PORT_BIT_MAP fail %#x!.\n", ret);
        g_eth_port_drv.ge_port_bmp = 0x3C;
    }
    printk("load CA_CFG_ID_GE_PORT_BIT_MAP: %#x\n", g_eth_port_drv.ge_port_bmp);
    

    ret = scfg_read(device_id, CFG_ID_XGE_PORT_BIT_MAP, sizeof(ca_uint32_t), &g_eth_port_drv.xe_port_bmp);
    if (CA_E_OK != ret) {
        printk("!Err: read CA_CFG_ID_XGE_PORT_BIT_MAP fail %#x!\n", ret);
        g_eth_port_drv.xe_port_bmp = 0;
    }
    printk("load CA_CFG_ID_XGE_PORT_BIT_MAP: %#x.\n", g_eth_port_drv.xe_port_bmp);
#endif

    (void)__eth_chk_port_bmp(device_id, &g_eth_port_drv.ge_port_bmp, &g_eth_port_drv.xe_port_bmp);

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_WAN_PORT_INDEX)
    if(CONFIG_RG_G3_WAN_PORT_INDEX!=AAL_PPORT_NI7){
        g_eth_port_drv.ge_port_bmp |= (1<<CONFIG_RG_G3_WAN_PORT_INDEX); /* hacking */
    }
#endif
    
    g_eth_port_drv.eth_port_bmp = g_eth_port_drv.ge_port_bmp | g_eth_port_drv.xe_port_bmp;
    if (0 == g_eth_port_drv.eth_port_bmp) {
        printk("!!!critical error: No Ethernet port exist, please check your scfg.txt!");
        return CA_E_ERROR;
    }

    printk("Valid Ethernet ports bitmap : %#x\r\n", g_eth_port_drv.eth_port_bmp);

#ifdef CONFIG_CORTINA_BOARD_FPGA

    printk("FPGA Board: use default MDIO configurations\n");
    __FOR_ALL_ETH_PORTS_DO(phy_id) {
        g_eth_port_drv.eth_mdio_addr[phy_id] = default_mdio[phy_id];
    }

    printk("FPGA Board: use default I/F mode configurations\n");
    __FOR_ALL_ETH_PORTS_DO(phy_id) {
        g_eth_port_drv.eth_if_mode[phy_id] = default_if_mode[phy_id];
    }

#else /* CONFIG_CORTINA_BOARD_FPGA */

    /* Read PHY MDIO address from STARTUP config */
    memset(&scfg_array[0], 0, sizeof(scfg_array));
    ret = scfg_read(device_id, CFG_ID_PHY_BASE_ADDR, CFG_ID_PHY_BASE_ADDR_LEN, &scfg_array[0]);
    if (CA_E_OK != ret) {
        printk("!Err: read PHY_BASE_ADDR fail %#x!\n", ret);
        /* Use following default MDIO configuration for ETH ports */
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            g_eth_port_drv.eth_mdio_addr[phy_id] = default_mdio[phy_id];
        }
    } else {
        memcpy(g_eth_port_drv.eth_mdio_addr, scfg_array, CFG_ID_PHY_BASE_ADDR_LEN);
    }

    /* Read ETH Interface mode */
    memset(&scfg_array[0], 0, sizeof(scfg_array));
    ret = scfg_read(device_id, CFG_ID_PHY_INTERFACE, CFG_ID_PHY_INTERFACE_LEN, &scfg_array[0]);
    if (CA_E_OK != ret) {
        printk("!Err: read CA_CFG_ID_PHY_INTERFACE fail %#x!\n",
              ret);
        /* Use following default I/F configuration for ETH ports */
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            g_eth_port_drv.eth_if_mode[phy_id] = default_if_mode[phy_id];
        }
    } else {
        memcpy(g_eth_port_drv.eth_if_mode, scfg_array, CFG_ID_PHY_INTERFACE_LEN);
        __FOR_ALL_ETH_PORTS_DO(phy_id) {
            printk("ethernet port %d interface type:  %s\n", phy_id,
                __eth_if_str_get(g_eth_port_drv.eth_if_mode[phy_id]));
            if (g_eth_port_drv.eth_if_mode[phy_id] > AAL_NI_IF_MODE_MAX) {
                g_eth_port_drv.eth_port_bmp &= ~(1 << phy_id); /* invalid port */
            }
        }
    }
#endif /* else of CONFIG_CORTINA_BOARD_FPGA */


        g_eth_port_drv.eth_dynamic_deep_queue = 1;


    printk("load Ethernet port SCFG OK\n");

    return CA_E_OK;

}

static ca_status_t __eth_port_global_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    memset(&g_eth_port_drv, 0, sizeof(g_eth_port_drv));
    memset(&g_eth_port_drv.eth_mdio_addr[0], 0xFF, sizeof(ca_uint8_t) * __MAX_ETH_PORT_NUM); /* init MDIO address as invalid */

    /* Initialize Ethernet port API lock */
    if (CA_E_OK != ca_spin_lock_init(&g_eth_port_drv.eth_port_lock, SPINLOCK_BH)) {
        return CA_E_RESOURCE;
    }

    /* Initialize MDIO global resource */
    ret = aal_mdio_global_init(device_id);
    if (CA_E_OK != ret) {
        printk("MDIO global init fail, return %d\r\n", ret);
        return ret;
    }
    /* Load startup configuration into global var */
    ret = __eth_port_scfg_load(device_id);

    /* enable ethernet port scan for G3 ASIC */
    g_eth_port_drv.eth_port_scan_enable = ~0;

    return ret;
}

static ca_status_t __xge_port_enable(ca_device_id_t device_id, ca_port_id_t port_id, ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_xge_cmn_cfg_mask_t      xge_mask;
    aal_ni_xge_cmn_cfg_t           xge_config;

    printk("set XE port %d mac enable to %d \r\n", port_id, enable);

    xge_mask.u32 = 0;
    xge_mask.s.tx_rst = 1;
    xge_mask.s.rx_rst = 1;

    xge_config.tx_rst  = !enable;
    xge_config.rx_rst  = !enable;

    if (AAL_NI_XAUI == port_id  && ((1 << port_id) & g_eth_port_drv.eth_port_bmp)) {
        aal_ni_xaui_mac_tx_cfg_mask_t  xaui_mac_tx_mask;
        aal_ni_xaui_mac_tx_cfg_t       xaui_mac_tx;
        aal_ni_xaui_mac_rx_cfg_mask_t  xaui_mac_rx_mask;
        aal_ni_xaui_mac_rx_cfg_t       xaui_mac_rx;
        xaui_mac_tx_mask.u32 = 0;
        xaui_mac_rx_mask.u32 = 0;
        memset(&xaui_mac_tx, 0, sizeof(xaui_mac_tx));
        memset(&xaui_mac_rx, 0, sizeof(xaui_mac_rx));
        xaui_mac_tx_mask.s.mac_en  = 1;
        xaui_mac_rx_mask.s.mac_en  = 1;
        xaui_mac_tx.mac_en = enable;
        xaui_mac_rx.mac_en = enable;

        ret = aal_ni_xaui_mac_tx_config_set(device_id, xaui_mac_tx_mask, &xaui_mac_tx);
        ret |= aal_ni_xaui_mac_rx_config_set(device_id, xaui_mac_rx_mask, &xaui_mac_rx);

    } else {
        ret = aal_ni_xge_cmn_config_set(device_id, port_id - AAL_NI_PORT_XGE1, xge_mask, &xge_config);
    }

    return ret;
}

static ca_status_t __ge_port_enable(ca_device_id_t device_id, ca_port_id_t port_id, ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;

    printk("set port %d mac enable to %d \r\n", port_id, enable);

    /* Only set rx, when tx is always enable to prevent buffer overflow */
    eth_mac_mask.u32 = 0;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    eth_mac_mask.s.mac_rx_rst = 1;
    eth_mac_mask.s.rx_en      = 1;

    eth_mac_config.mac_rx_rst = !enable;
    eth_mac_config.rx_en      = enable;

    ret = aal_ni_eth_port_mac_set(device_id, port_id, eth_mac_mask, &eth_mac_config);

    return ret;
}

static ca_status_t __eth_ports_enable(ca_device_id_t device_id, ca_boolean_t enable)
{
    ca_port_id_t port_id = 0;
    ca_status_t  ret = CA_E_OK;

    __FOR_ALL_ETH_PORTS_DO(port_id) {

        printk("port_id=%d enable = %d \n", port_id, enable);

        if (__ETH_IS_XGE(port_id)) {/* XGE */
            ret =__xge_port_enable(device_id, port_id, enable);
        } else {
            ret =__ge_port_enable(device_id, port_id, enable);
        }

        if (CA_E_OK != ret && CA_E_PARAM != ret) {
            printk("%s: (return code=%d)\n", __func__, ret);
            return CA_E_ERROR;
        }
    }

    return CA_E_OK;
}

void aal_eth_port_exit(CA_IN ca_device_id_t device_id)
{
    g_eth_port_drv.eth_port_scan_stop = 1;

    if (g_eth_port_drv.eth_port_lock) {
        ca_spin_lock_destroy(g_eth_port_drv.eth_port_lock);
        g_eth_port_drv.eth_port_lock = 0;
    }

    aal_mdio_global_exit(device_id);
    g_eth_port_init_done = 0;
}

static inline void __eth_phy_auto_detect(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t addr = 0, phy_count = 0, phy_id = 0;

    printk("auto detect Ethernet PHY start ...\n");

    __FOR_ALL_MDIO_DO(addr) {
        ret = aal_eth_phy_init(device_id, addr);
        if (CA_E_OK == ret) {
            phy_count++;
            for (phy_id = 0; phy_id < __MAX_ETH_PORT_NUM; phy_id ++) {
                if (g_eth_port_drv.eth_mdio_addr[phy_id] == addr) {
                    g_eth_port_drv.ext_phy_detected[phy_id] = TRUE;
                    printk("attch PHY addr %d to port %d driver\n", addr, phy_id);
                }
            }
        }
    }

    printk("detect Ethernet PHY done, find %d PHYs\n", phy_count);

}

static ca_status_t __eth_port_phy_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port_id = 0;
    aal_phy_autoneg_adv_t  adv;
    ca_boolean_t    eth_port3_active_flag = TRUE;

    if ( eth_port3_active_flag  == TRUE){
        ca_uint32_t  board_info = 0;
        ca_uint32_t  custer_bit = 0;
        ret = scfg_read(device_id, CFG_ID_CUSTOMER_BIT, sizeof(ca_uint32), (ca_uint8_t *)&custer_bit);
        ret |= scfg_read(device_id, CFG_ID_BOARD_INFO_BIT, sizeof(ca_uint32), (ca_uint8_t *)&board_info);
        if ((CA_E_OK == ret) && (custer_bit&CUSTOMER_BIT_13) &&
            (board_info == BOARD_INFO_BIT_3 || board_info == BOARD_INFO_BIT_1)  &&  
            __ETH_INTF_IS_XFI(AAL_NI_PORT_XGE0)) {
            g_eth_port_drv.disable_phy2mac_sync[AAL_NI_PORT_XGE0] = TRUE; /* for Ubiqous board with AQR107 PHY, neednt sync */
            ca_printf("ca-ne: detect 10G PHY interface with XFI rate matching mode\r\n");
        }

    AAL_FOR_ALL_GE_DO(port_id) {
        aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(port_id), &adv);

        adv.adv_pause = 0;
        adv.adv_asym_pause = 0;
        adv.adv_100base_tx_half = 1;
        adv.adv_100base_tx_full = 1;
        adv.adv_10base_t_half   = 1;
        adv.adv_10base_t_full   = 1;

#ifdef CONFIG_CORTINA_BOARD_FPGA
        adv.adv_1000base_t_half = 0;
        adv.adv_1000base_t_full = 0;
#else
        adv.adv_1000base_t_half = 1;
        adv.adv_1000base_t_full = 1;
#endif
            //aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(port_id), adv);
        aal_eth_phy_duplex_set(device_id, __ETH_PHY_ADDR(port_id), TRUE); /* FULL */

#ifdef CONFIG_CORTINA_BOARD_FPGA
        aal_eth_phy_speed_set(device_id, __ETH_PHY_ADDR(port_id), AAL_PHY_SPEED_100);
        aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), FALSE);
        g_eth_port_drv.autoneg[port_id] = FALSE;
#else
        aal_eth_phy_speed_set(device_id, __ETH_PHY_ADDR(port_id), _MAC_SPEED2PHY_SPEED(__default_mac_speed[port_id]));
        aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), TRUE);
        g_eth_port_drv.autoneg[port_id] = TRUE;
#endif
        g_eth_port_drv.adv[port_id].half_10  = 1;
        g_eth_port_drv.adv[port_id].full_10  = 1;
        g_eth_port_drv.adv[port_id].half_100 = 1;
        g_eth_port_drv.adv[port_id].full_100 = 1;
        g_eth_port_drv.adv[port_id].full_1g  = 1;
        g_eth_port_drv.adv[port_id].full_10g = 0;
        g_eth_port_drv.adv[port_id].flow_ctrl = 0;
        g_eth_port_drv.adv[port_id].asym_flow_ctrl  = 0;
        g_eth_port_drv.adv[port_id].eee_100_capable = 0;
        g_eth_port_drv.adv[port_id].eee_1g_capable  = 0;
        g_eth_port_drv.adv[port_id].eee_10g_capable = 0;
        //ca_printf("%s(): init ge %d done\r\n", __func__, port_id);
    }
    
    AAL_FOR_ALL_XE_DO(port_id) {
            memset(&g_eth_port_drv.adv[port_id], 0, sizeof(ca_eth_port_ability_t));
            if (__ETH_DETECTED_PHY(port_id)) {
                aal_eth_phy_ability_adv_get(device_id, __ETH_PHY_ADDR(port_id), &adv);

                adv.adv_pause = 0;
                adv.adv_asym_pause = 0;
                adv.adv_100base_tx_half = 1;
                adv.adv_100base_tx_full = 1;
                adv.adv_10base_t_half   = 1;
                adv.adv_10base_t_full   = 1;
                adv.adv_1000base_t_half = 1;
                adv.adv_1000base_t_full = 1;
                adv.adv_2500base_t_full = 1;
                adv.adv_5000base_t_full = 1;
                adv.adv_10000base_t_full = 1;
                adv.adv_10gbase_t_eee   = 0;
                adv.adv_1000base_t_eee  = 0;
                adv.adv_100base_tx_eee  = 0;
                adv.adv_5gbase_t_eee    = 0;
                adv.adv_2d5gbase_t_eee  = 0;
                aal_eth_phy_ability_adv_set(device_id, __ETH_PHY_ADDR(port_id), adv);
            aal_eth_phy_auto_neg_set(device_id, __ETH_PHY_ADDR(port_id), TRUE);
            g_eth_port_drv.autoneg[port_id] = TRUE;

        } else {
            g_eth_port_drv.autoneg[port_id] = FALSE;
        }
            g_eth_port_drv.adv[port_id].full_10g  = 1;
            g_eth_port_drv.adv[port_id].full_5g   = 1;
            g_eth_port_drv.adv[port_id].full_2d5g = 1;
            g_eth_port_drv.adv[port_id].full_1g   = 1;
            g_eth_port_drv.adv[port_id].full_1g   = 1;
            g_eth_port_drv.adv[port_id].full_100  = 1;
            g_eth_port_drv.adv[port_id].half_100  = 1;
            g_eth_port_drv.adv[port_id].half_10  = 1;
            g_eth_port_drv.adv[port_id].full_10  = 1;
            g_eth_port_drv.adv[port_id].eee_100_capable = 0;
            g_eth_port_drv.adv[port_id].eee_1g_capable  = 0;
            g_eth_port_drv.adv[port_id].eee_10g_capable = 0;
    }

#ifndef CONFIG_CORTINA_BOARD_FPGA /* ASIC internal GPHY init */
    aal_internal_phy_init(device_id);

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_SWITCH_INIT_LINKDOWN)
    aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(port_id), 0);
#endif

#endif
   }

#if defined(CONFIG_NE_AAL_XFI)
    aal_xfi_init(device_id);
    AAL_FOR_ALL_XE_DO(port_id) {
        if (__ETH_DETECTED_PHY(port_id) && __ETH_IS_XFI(port_id)) { /* reset the XFI SERDES interfaces between PHY and MAC */
            aal_eth_phy_reset(device_id, __ETH_PHY_ADDR(port_id));
            aal_xfi_reset(device_id, aal_xfi_port2xfi(port_id));
        }
    }
#endif

    return ret;
}

static ca_status_t __eth_port_mac_init(ca_device_id_t device_id)
{
    aal_ni_xge_mac_frame_len_config_mask_t xe_len_mask;
    aal_ni_xge_mac_frame_len_config_t xe_len_config;
    aal_ni_pkt_len_config_mask_t  ge_len_mask;
    aal_ni_pkt_len_config_t       ge_len_config;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
    aal_ni_eth_if_config_mask_t eth_if_mask;
    aal_ni_eth_if_config_t eth_if_cfg;
    ca_uint32_t port_id = 0;
    ca_status_t ret = CA_E_OK;

    eth_mac_mask.u32 = 0;
    eth_mac_mask.s.duplex = 1;
    eth_mac_mask.s.speed = 1;
    eth_mac_mask.s.mac_crc_calc_en = 1;
    eth_mac_mask.s.tx_en = 1;
    eth_mac_mask.s.rx_pfc_disable = 1;

    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    eth_mac_config.duplex = AAL_NI_MAC_DUPLEX_FULL; /* FULL Duplex */
    eth_mac_config.mac_crc_calc_en = 1; /* enable CRC caculation */
    eth_mac_config.tx_en = 1;   /* Enable */
    eth_mac_config.rx_pfc_disable = 1;  /* Disable PFC function */

    eth_if_mask.u32 = 0;
    eth_if_mask.s.int_cfg = 1;
    eth_if_mask.s.phy_mode = 1;

    memset(&eth_if_cfg, 0, sizeof(eth_if_cfg));


    AAL_FOR_ALL_GE_DO(port_id) {
        eth_if_cfg.int_cfg = g_eth_port_drv.eth_if_mode[port_id];
        eth_if_cfg.phy_mode = AAL_NI_PHY_MODE_MAC;  /* connect to PHY */
        eth_mac_config.speed = __default_mac_speed[port_id];
        g_eth_port_drv.speed[port_id] = (ca_uint8_t)_MAC2SPEED(__default_mac_speed[port_id]);

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        if (__ETH_IS_CONNECT_SWITCH(port_id)) {
            eth_if_cfg.phy_mode = AAL_NI_PHY_MODE_PHY;  /* Ethernet interface connect to MAC */
            ca_printf ("ca-ne: device is connected to switch chip\r\n" );
        }
#endif

        ret = aal_ni_eth_if_set(device_id, port_id,
                    eth_if_mask, &eth_if_cfg);

        if (CA_E_OK != ret) {
            ca_printf("port %u interface init fail %#x.", port_id, ret);
            return ret;
        }

        ret = aal_ni_eth_port_mac_set(device_id, port_id,
                          eth_mac_mask, &eth_mac_config);

        if (CA_E_OK != ret) {
            ca_printf("port %u MAC init fail %#x.", port_id, ret);
            return ret;
        }
    }

    /* Init GE MAX frame size */
    memset(&ge_len_mask, 0, sizeof(aal_ni_pkt_len_config_mask_t));
    memset(&ge_len_config, 0, sizeof(aal_ni_pkt_len_config_t));
    
    ge_len_mask.s.max_pkt_size = 1;
    ge_len_mask.s.max_store_pkt_len = 1;
    ge_len_config.max_pkt_size = CA_ETH_MAX_FRAME_SIZE_DEFAULT;
    ge_len_config.max_store_pkt_len = CA_ETH_MAX_FRAME_SIZE_MAX;
    (void)aal_ni_pkt_len_set(device_id, ge_len_mask, &ge_len_config);

    /* Init XGE MAX frame size */
    AAL_FOR_ALL_XE_DO(port_id) {
        memset(&xe_len_mask, 0, sizeof(aal_ni_xge_mac_frame_len_config_mask_t));
        memset(&xe_len_config, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));
        
        xe_len_mask.s.max_len = 1;
        xe_len_config.max_len = CA_ETH_MAX_FRAME_SIZE_DEFAULT;
        (void)aal_ni_xge_mac_frame_len_set(device_id, port_id - AAL_NI_XE_START, xe_len_mask, &xe_len_config);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        (void)__xge_pause_set(device_id, port_id, 0, 0, 0);
#endif
    }

#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
    AAL_FOR_ALL_XE_DO(port_id) {
        g_eth_port_drv.speed[port_id] = (ca_uint8_t)CA_PORT_SPEED_10G;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
    __eth_port_gphy_ctrl_init(device_id);
#endif

    return ret;

}

static ca_status_t aal_eth_port_init(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    if (g_eth_port_init_done) {
        printk("Ethernet port driver has already initialized.\r\n");
        return CA_E_OK;
    }
     /* Global data init */
    __eth_port_global_init(device_id);

    /* Disable all ports' Rx */
    ret = __eth_ports_enable(device_id, FALSE);
    if (CA_E_OK != ret) {
        printk("__eth_ports_enable fail, return code %#x.\n", ret);
    } else {
        printk("disalbe all ethernet ports OK\n");
    }

    if (g_eth_port_drv.eth_port_bmp) {
        /* ETH PHY init */
        __eth_phy_auto_detect(device_id);
    }

    ret =__eth_port_phy_init(device_id);
    if (CA_E_OK != ret) {
        printk("__eth_port_phy_init fail, return code %#x.\n", ret);
    }

    /* ETH MAC init */
    ret = __eth_port_mac_init(device_id);
    if (CA_E_OK != ret) {
        printk("__eth_port_mac_init fail, return code %#x.\n", ret);
    }
    /* disable unused ports */
    aal_ni_reset_intf(device_id, g_eth_port_drv.eth_port_bmp);

    if (CA_E_OK != ret) {
        aal_eth_port_exit(device_id);
        printk("!!!error: eth_port_init fail, return %#x!!!\r\n", ret);
    } else {
        g_eth_port_init_done = 1;
    }

    return ret;
}

static ca_status_t  __port_link_evt_send(ca_device_id_t device_id, ca_port_id_t port_id,  ca_boolean_t  link_status)
{
    ca_event_port_link_t link_event;
    ca_status_t          ret = CA_E_OK;
    
    memset(&link_event, 0, sizeof(ca_event_port_link_t));
    link_event.port_id = port_id;
    link_event.status  = link_status ? CA_PORT_LINK_UP : CA_PORT_LINK_DOWN ;
    ret = (ca_status_t)ca_event_send(device_id, CA_EVENT_ETH_PORT_LINK, 
        (void *)&link_event, sizeof(ca_event_port_link_t));

    return ret;
}

static ca_status_t __xge_intf_link_status_get(
    ca_device_id_t device_id,
    ca_uint32_t aal_port,
    aal_phy_link_status_t *status)
{
    ca_status_t  ret = CA_E_OK;

ERROR_XGE_INTF_LINK_STATUS_GET:
    return ret;

}

static ca_status_t __ge_intf_link_status_get(
    ca_device_id_t device_id,
    ca_uint32_t aal_port,
    aal_phy_link_status_t *status)
{
    ca_status_t  ret = CA_E_OK;
    aal_eth_port_status_t  mac_status;

    if ( g_eth_port_drv.eth_if_mode[aal_port] != AAL_NI_IF_MODE_GE_RGMII &&
        g_eth_port_drv.eth_if_mode[aal_port] != AAL_NI_IF_MODE_FE_RGMII) {
        /* !NOTE: Curernt ASIC ONLY support get MAC status under RGMII mode */
        memset(status, 0, sizeof(aal_phy_link_status_t));
        return CA_E_OK;
    }

    /* Read the status synced from PHY under RGMII mode */
    memset(&mac_status, 0, sizeof(mac_status));
    ret = aal_ni_eth_port_status_get(device_id, aal_port, &mac_status);
    if (CA_E_OK == ret) {
        status->link_up = mac_status.link_status;
        status->duplex = mac_status.duplex;
        status->speed = _MAC_SPEED2PHY_SPEED(mac_status.speed);
    }

        return ret;
}

static ca_status_t __eth_link_status_get(
    ca_device_id_t         device_id,
    ca_uint32_t            aal_port,
    aal_phy_link_status_t *link_status)
{
#ifdef RTK_MEM_CORRUPTTION_FIX
    if ( aal_port < __MAX_ETH_PORT_NUM) {
#endif
        if (__ETH_DETECTED_PHY(aal_port)) { /* retrieve link status from external/intergrated PHY */
            return aal_eth_phy_link_status_get(device_id, __ETH_PHY_ADDR(aal_port), link_status);
        } else if (__ETH_IS_XGE(aal_port)) { /* retrieve link status from XGE interface like SerDes or PCS */
            return __xge_intf_link_status_get(device_id, aal_port, link_status);
        } else { /* retrieve link status from GE interface like SerDes or PCS */
            return __ge_intf_link_status_get(device_id, aal_port, link_status);
        }

            return CA_E_OK;
#ifdef RTK_MEM_CORRUPTTION_FIX
    } else {
        ca_printf("aal_port > __MAX_ETH_PORT_NUM\n ");
        return CA_E_ERROR;
    }
#endif

}

#if defined(CONFIG_NE_AAL_XFI)
static ca_status_t __xge_phy2mac_sync(
    ca_device_id_t device_id,
    ca_port_id_t aal_port,
    aal_phy_link_status_t *link_status)
{

    ca_status_t     ret = CA_E_OK;
    ca_uint32_t     xfi_idx = 0;
    ca_boolean_t    xfi_link = 0;


    xfi_idx = aal_xfi_port2xfi(aal_port);
    if (__ETH_INTF_IS_USXGMII(aal_port)) {
        (void)aal_xfi_autoneg_set(device_id, xfi_idx, 1);
        ret = aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
        printk("USXGMII %d re-autoneg link %s!\n", xfi_idx, xfi_link ? "up" : "down");
    } else {
    (void)aal_xfi_speed_set(device_id, xfi_idx, link_status->speed);
        ca_mdelay(20);
        ret = aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
    if (ret == CA_E_OK && !xfi_link) {
            aal_eth_phy_reset(device_id, __ETH_PHY_ADDR(aal_port));
        ca_mdelay(10);
            aal_xfi_reset(device_id, xfi_idx);
        ca_mdelay(10);
        (void)aal_xfi_link_status_get(device_id, xfi_idx, &xfi_link);
            printk("WARNING: XFI %d link %s with PHY interface!\n", xfi_idx, xfi_link ? "up" : "down");
        }
    }
    return ret;
}
#endif

static ca_status_t __ge_phy2mac_sync(
    ca_device_id_t device_id,
    ca_port_id_t aal_port,
    aal_phy_link_status_t *link_status)
{
    ca_status_t  ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t eth_mac_mask;
    aal_ni_eth_mac_config_t eth_mac_config;
    aal_phy_autoneg_adv_t lp_adv;
    aal_ni_eth_mac_config_t tmp_eth_mac_config;
    aal_l2_te_policer_cfg_msk_t l2_te_policer_cfg_msk;
    aal_l2_te_policer_cfg_t l2_te_policer_cfg;

    eth_mac_mask.u32      = 0;
    eth_mac_mask.s.speed  = 1;
    eth_mac_mask.s.duplex = 1;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));
    memset(&lp_adv,0x00,sizeof( aal_phy_autoneg_adv_t ));
    /*
    **  Get LP's adv_ability
    **  if LP.adv_pause is 1, enable tx_flow_control on mac. 
    **  if LP.adv_pause is 0, disable tx_flow_control on mac. 
    */
    aal_eth_phy_partner_ability_adv_get(device_id,__ETH_PHY_ADDR(aal_port),&lp_adv);
    aal_ni_eth_port_mac_get(device_id, aal_port, &tmp_eth_mac_config);

    l2_te_policer_cfg_msk.u32 = 0;
    memset(&l2_te_policer_cfg,0,sizeof(aal_l2_te_policer_cfg_t));
    l2_te_policer_cfg_msk.s.bypass_red = 1;

    if ( lp_adv.adv_pause == 1 && tmp_eth_mac_config.tx_pause_en == 1)
    {
        l2_te_policer_cfg.bypass_red = 1;
    }
    else
    {
        l2_te_policer_cfg.bypass_red = 0;
    }
    aal_l2_te_policer_port_cfg_set(device_id,aal_port,l2_te_policer_cfg_msk,&l2_te_policer_cfg);
    /* sync MAC speed/duplex with PHY */
    eth_mac_config.speed = _PHY_SPEED2MAC_SPEED(link_status->speed);
    eth_mac_config.duplex = link_status->duplex;
    ret = aal_ni_eth_port_mac_set(device_id, aal_port, eth_mac_mask,
                    &eth_mac_config);

    return ret;

}

static ca_status_t __port_phy2mac_sync(
    ca_device_id_t device_id,
    ca_port_id_t aal_port,
    aal_phy_link_status_t *link_status)
{
    ca_status_t  ret = CA_E_OK;

    if (!link_status || !link_status->link_up) {
        return CA_E_OK;
    }

    if (__ETH_IS_XGE(aal_port)) { /* XGE */
#if defined(CONFIG_NE_AAL_XFI)
        ret =__xge_phy2mac_sync(device_id, aal_port, link_status);
#endif
    } else { /* GE */
        ret = __ge_phy2mac_sync(device_id, aal_port, link_status);
    }
    printk("INFO: Sync Eth PHY speed %d(%sbps) duplex %d to MAC Port %u\n",
        link_status->speed, aal_eth_phy_speed_str_get(device_id, link_status->speed), link_status->duplex, aal_port);

    return ret;
}

static ca_status_t __port_eee_trigger(
    ca_device_id_t device_id,
    ca_port_id_t   aal_port,
    ca_boolean_t   link_status)
{
/*
All EEE state-machines are implemented in EEE PHY. EEE configurations can be 
set/get via MDIO/MMD registers (HW advised not to make any changes)

G3 just control the MAC:
1.  EEE_enable/disable - configurable via API (need to enable EEE in PHY via MMD)
2.  LPI_tx_enable/disable sequence:
    a.  TX_LPI control behavior :  (PHY link change INT trigger or polling trigger ) 
    b.  [1] Default  : disable TXLPI , (cpu_lpi_deassert= 1) 
    c.  [2] When detect PHY = link down , disable TXLPI , (cpu_lpi_deassert= 1) 
    d.  [3] When detect PHY = link up , should wait more than 1 second then enable  TXLPI , (cpu_lpi_deassert= 0)  
3.  LPI_data_mode - use HW default - no need to change, in case we need to change this during debugging
4.  txfifo_empty_cnt - use HW default (NOTE: value should be longer than ts_timeout in PHY)
5.  lpi_min_assert_cnt - user HW default
6.  lpi_min_wakeup_cnt (NOE: value should be longer than tw_timeout in PHY)) , set to be 30us enough, 300
7.  lpi_mac_dis_wait_cnt - user HW default
8.  stop_txclk_en_cnt - user HW default

*/
    aal_ni_eee_mask_t     eee_msk;
    aal_ni_eee_cfg_t      eee_cfg;

    if (__ETH_IS_GE(aal_port)) {
        eee_msk.u32 = 0;
        memset(&eee_cfg, 0, sizeof(eee_cfg));

        eee_msk.s.cpu_lpi_deassert = 1;
        eee_cfg.cpu_lpi_deassert = !link_status;
        if (link_status) {
            eee_msk.s.lpi_min_wakeup_cnt = 1;
            eee_cfg.lpi_min_wakeup_cnt = 300;
        }
        return aal_ni_eee_cfg_set(device_id, aal_port, eee_msk, &eee_cfg);
    } else if (__ETH_IS_XGE(aal_port)) {
        aal_ni_xge_eee_mask_t  xge_eee_mask;
        aal_ni_xge_eee_cfg_t   xge_eee_cfg;

        xge_eee_mask.u32 = 0;
        xge_eee_mask.s.tx_tql_val = 1;
        xge_eee_cfg.tx_tql_val = 12500;
        return aal_ni_xge_eee_cfg_set(device_id, aal_port, xge_eee_mask, &xge_eee_cfg);

    }

    return CA_E_OK;

}

static void 
__eth_port_link_scan(
    ca_device_id_t device_id)
{
    ca_uint32_t aal_port = 0;
    aal_phy_link_status_t link_status;
#if defined(CONFIG_NE_AAL_XFI)
    aal_xfi_status_t      xfi_sts;
#endif
    static ca_boolean_t port_link_up[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_speed[__MAX_ETH_PORT_NUM] = { 0 };
    static ca_boolean_t port_link_duplex[__MAX_ETH_PORT_NUM] = { 0 };

    __FOR_ALL_ETH_PORTS_DO(aal_port) {
        if (!__ETH_PORT_SCAN_IS_ENABLE(aal_port)) {
            continue;
        }

        if (__ETH_IS_CONNECT_SWITCH(aal_port)) {
#if defined(CONFIG_NE_AAL_XFI)
        if (__ETH_IS_XFI(aal_port)) {
            __internal_xfi_update(device_id, aal_port);
        }
#endif
            continue;
        }

        memset(&link_status, 0, sizeof(link_status));

        /* ONLY scan port when port enable, otherwise regard port as link down */
        if (g_eth_port_drv.port_enable[aal_port]) {
            if (CA_E_OK != __eth_link_status_get(device_id, aal_port, &link_status)) {
                printk("__eth_link_status_get fail!\n");
                continue; /* Link status get FAIL */
            }
        }

        if ((!link_status.link_up && !port_link_up[aal_port]) ||
            (link_status.link_up && port_link_up[aal_port]
            && (port_link_speed[aal_port] == link_status.speed 
            && port_link_duplex[aal_port] == link_status.duplex))) {

#if defined(CONFIG_NE_AAL_XFI)
            if (!port_link_up[aal_port]) {
                if(!__ETH_IS_XFI(aal_port) || __ETH_DETECTED_PHY(aal_port)) {
                    continue;
                }
                aal_xfi_status_get(device_id,aal_xfi_port2xfi(aal_port),&xfi_sts);
                if(!xfi_sts.rxidle) {
                    aal_xfi_link_toggle_rx_en(device_id, aal_xfi_port2xfi(aal_port));
                }
            }
#endif
            continue; /* Link status and speed/duplex NOT change */
        }

        /*
         * Handle the port link change in the following
         */
        if(link_status.link_up && __ETH_DETECTED_PHY(aal_port)
           && !g_eth_port_drv.disable_phy2mac_sync[aal_port]) {
            /* PHY status sync to MAC config */
            __port_phy2mac_sync(device_id,aal_port,&link_status);
        }

        /* EEE handle */
        if (__ETH_EEE_IS_ENABLE(aal_port) && __ETH_DETECTED_PHY(aal_port)) {
            (void)__port_eee_trigger(device_id, aal_port, link_status.link_up);
        }
        /* save Link status */
        port_link_up[aal_port] = link_status.link_up;

        if (link_status.link_up) {
#if !defined(CONFIG_RTL_MULTI_LAN_DEV)
            if (!__ETH_IS_XGE(aal_port))
                __ETH_LED_ON(aal_port, link_status.speed);
#endif
            port_link_speed[aal_port] = link_status.speed;
            port_link_duplex[aal_port] = link_status.duplex;
#if defined(CONFIG_NE_AAL_XFI)
            if(__ETH_IS_XFI(aal_port) && !__ETH_INTF_IS_USXGMII(aal_port)) {
                if(!g_eth_port_drv.disable_phy2mac_sync[aal_port])
                    aal_xfi_reset(device_id,aal_xfi_port2xfi(aal_port));
            }
#endif
            ca_printf("INFO: Device %d Ethernet Port %u Link UP %sbps/%s\n", 
                device_id, aal_port, 
                aal_eth_phy_speed_str_get(device_id, link_status.speed), link_status.duplex ? "FULL" : "HALF");
        } else {
#if !defined(CONFIG_RTL_MULTI_LAN_DEV)
            if (!__ETH_IS_XGE(aal_port))
                __ETH_LED_OFF(aal_port);
#endif
            port_link_speed[aal_port]  = AAL_PHY_SPEED_10;
            port_link_duplex[aal_port] = FALSE;
            printk("INFO: Device %d Ethernet Port %u Link DOWN\n", device_id, aal_port);
        }

        /* Send port link change event */
        __port_link_evt_send(device_id,
        CA_PORT_ID(CA_PORT_TYPE_ETHERNET, aal_port), link_status.link_up);
    }
}

static void __eth_port_link_scan_timer_func(void *cookie)
{
    printk("ca-ne: port scan thread create successfully.\n");

    while(!g_eth_port_drv.eth_port_scan_stop) {

        ca_sleep(__ETH_PORT_LINK_SCAN_INTERVAL);
        if (!ni_initialized) {
            continue;
        }
        __eth_port_link_scan(0);
    }

    printk("ca-ne: port scan thread exit successfully.\n");

}

static ca_status_t __eth_port_scan_start(ca_device_id_t device_id)
{
    ca_uint  tid = 0;
    ca_int   ret = 0;

    /* Port scan thread */
    g_eth_port_drv.eth_port_scan_stop = 0;
    g_eth_port_drv.eth_xfi_autoreset = 1;

    ret = ca_thread_create(&tid, "port_scan", __eth_port_link_scan_timer_func, NULL, 10240, 15, 0);
    if (CA_E_OK != ret) {
        printk("error: create port_scan thread fail %d.\n", ret);
        return ret;
    }

    return CA_E_OK;
}

static ca_status_t aal_eth_ports_restore(CA_IN ca_device_id_t device_id)
{
    ca_port_id_t port_id   = 0;
    ca_boolean_t enable    = 0;
    ca_port_id_t xge_start = AAL_NI_XE_START;
    ca_status_t ret = CA_E_OK;
    aal_ni_eth_mac_config_mask_t  eth_mac_mask;
    aal_ni_eth_mac_config_t        eth_mac_config;
    aal_ni_xge_cmn_cfg_mask_t      xge_mask;
    aal_ni_xge_cmn_cfg_t           xge_config;
    ca_uint8_t uni_connect[CFG_ID_UNI_CONNECTION_LEN] = {0};

    printk("start restore Ethernet ports configuration from SCFG\n");

    ret = scfg_read( device_id, CFG_ID_UNI_CONNECTION, CFG_ID_UNI_CONNECTION_LEN, uni_connect);
    if (CA_E_OK != ret) {
        ca_printf("ca-ne: Load %s fail, ret = %d, so set UNI default to be enable\n", CFG_ID_UNI_CONNECTION, ret);
        memset(uni_connect, 0x01, CFG_ID_UNI_CONNECTION_LEN);
    }

    xge_mask.u32 = 0;
    xge_mask.s.tx_rst = 1;
    xge_mask.s.rx_rst = 1;

    /* Only set rx, when tx is always enable to prevent buffer hang */
    eth_mac_mask.u32 = 0;
    eth_mac_mask.s.mac_rx_rst = 1;
    eth_mac_mask.s.mac_tx_rst = 1;
    eth_mac_mask.s.rx_en = 1;
    memset(&eth_mac_config, 0, sizeof(eth_mac_config));

    __FOR_ALL_ETH_PORTS_DO(port_id) {
        enable = uni_connect[port_id];

        if (__ETH_IS_XGE(port_id)) {/* XGE */

#if defined(CONFIG_NE_AAL_XFI)
            if (__ETH_IS_XFI(port_id)) {
                aal_xfi_cfg_mask_t   cfg_mask;
                aal_xfi_cfg_t        cfg;

                cfg_mask.data32 = 0;
                memset(&cfg, 0, sizeof(cfg));

                cfg_mask.s.pdown = 1;
                cfg_mask.s.rx_en = 1;
                cfg.pdown = !enable;
                cfg.rx_en = enable ? 1 : 0;
                ret = aal_xfi_cfg_set(device_id,
                    aal_xfi_port2xfi(port_id),
                    cfg_mask, &cfg);
            }
#endif
        } else {
            /* Leave GPHY power on always */
            eth_mac_config.mac_rx_rst = !enable;
            eth_mac_config.mac_tx_rst = !enable;
            eth_mac_config.rx_en = enable;

            ret = aal_ni_eth_port_mac_set(device_id, port_id,
                               eth_mac_mask, &eth_mac_config);
        }

        if (ret != CA_E_OK) {
            printk("%s: restore fail(retrun code = %d)\n", __func__, ret);
        } else {
            printk("%s: %s port %d OK\n", __func__, "Enable", port_id );
        }

        g_eth_port_drv.port_enable[port_id] = enable;
    }

    (void)__eth_port_link_scan(device_id); /* sync the hardware link state to software in the initialization */
    (void)__eth_port_scan_start(device_id); /* start the port scan thread */

    return CA_E_OK;

}

ca_status_t  aal_port_enable_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_OUT ca_boolean_t * enable)

{
    ca_uint32_t aal_port = PORT_ID(port_id);

    __ETH_INIT_CHK(device_id);
    CA_NULL_PTR_CHECK(enable);

    if (aal_port >= __MAX_ETH_PORT_NUM) {
        printk("the port %#x is invalid\n", port_id);
        return CA_E_PARAM;
    }

    *enable = g_eth_port_drv.port_enable[aal_port];

    return CA_E_OK;


}

ca_status_t  aal_port_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_boolean_t    enable)
{
    ca_uint32_t aal_port = PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;

    __ETH_INIT_CHK(device_id);
    CA_BOOL_VAL_CHECK(enable);

    if (aal_port >= __MAX_ETH_PORT_NUM) {
        printk("the port %#x is invalid\n", port_id);
        return CA_E_PARAM;
    }

    if (__ETH_DETECTED_PHY(aal_port)) { /* external PHY */
        printk("set ext phy %d enable to %d \r\n", aal_port, enable);
        ret = aal_eth_phy_power_set(device_id, __ETH_PHY_ADDR(aal_port), enable);
#if defined(CONFIG_NE_AAL_XFI)
    } else if (__ETH_IS_XFI(aal_port)) { /* XFI serdes */
        aal_xfi_cfg_mask_t msk;
        aal_xfi_cfg_t cfg;
        ca_uint32_t xfi_idx = 0;

        xfi_idx = aal_xfi_port2xfi(aal_port);
        memset(&cfg, 0, sizeof(cfg));
        msk.data32 = 0;
        msk.s.pdown = 1;
        msk.s.rx_en = 1;
        cfg.pdown = !enable;
        cfg.rx_en = enable ? 1 : 0;
        printk("set xfi %d enable to %d \r\n", xfi_idx, enable);
        ret = aal_xfi_cfg_set(device_id, xfi_idx, msk, &cfg);
#endif
    } 
    
    /* MAC */
    if (__ETH_IS_GE(aal_port)) {
        ret = __ge_port_enable(device_id, aal_port, enable);
    } else if (__ETH_IS_XGE(aal_port)) {
        ret = __xge_port_enable(device_id, aal_port, enable);
    }

    if (CA_E_OK == ret) {
        g_eth_port_drv.port_enable[aal_port] = enable;
    }

    return ret;
}

ca_status_t aal_port_init(CA_IN ca_device_id_t dev)
{
    ca_uint32_t i = 0;
    ca_uint32_t lpid = 0;
    aal_ippb_cfg_msk_t ippb_msk;
    aal_ippb_cfg_t ippb_cfg;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_elpb_cfg_msk_t egr_msk;
    aal_elpb_cfg_t egr_cfg;
    ca_uint64_t bmp = 0;
    ca_uint32_t my_mac = 0;
    /*ca_uint32_t flooding_domain_1[CFG_ID_FLOODING_DOMAIN_LEN];
    ca_uint32_t flooding_domain_2[CFG_ID_FLOODING_DOMAIN_LEN];
    ca_uint32_t mcgid_0, mcgid_1;
    aal_ple_dft_fwd_ctrl_mask_t fwd_msk;
    aal_ple_dft_fwd_ctrl_t fwd_cfg;*/
    ca_uint8_t wan_port_type = 0;
    ca_uint8_t l2_cls_entry_id  =  0;
    ca_uint8_t l2_egr_cls_entry_id  =  0;
    ca_uint8_t l2fe_use_32_entry = 0;
    ca_status_t ret = CA_E_OK;

    ret = aal_eth_port_init(dev);
    if (CA_E_OK != ret) {
        printk("ETH Port init fail, return %d\r\n", ret);
        return ret;
    }

    if (!__aal_port_lock)
        ca_spin_lock_init(&__aal_port_lock, SPINLOCK_BH);
    // memset(g_aal_l2vlan_outer_is_s, 0, CFG_ID_L2FE_VLAN_IS_S_LEN);
    // memset(g_aal_l2vlan_inner_is_s, 0, CFG_ID_L2FE_VLAN_IS_S_LEN);

     /* init IPPB table, ppid to lpid */
    memset(&ippb_msk, 0, sizeof(aal_ippb_cfg_msk_t));
    memset(&ippb_cfg, 0, sizeof(aal_ippb_cfg_t));
    ippb_msk.s.lspid = 1;
    for (i = 0; i < AAL_PORT_NUM_LOGICAL; i++) {
        ippb_cfg.lspid = i;
        aal_port_ippb_cfg_set(dev, i, ippb_msk, &ippb_cfg);
    }

    /* STP mode configuration: forward and learning
       Station move enable
       Port isolation disable  */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    igr_msk.s.station_move_en = 1;
    igr_cfg.station_move_en = 1;
    igr_msk.s.stp_mode = 1;
    for(i = 0; i < AAL_PORT_NUM_LOGICAL; i++)
    {
        bmp = ~(1ULL<<i);  /* disable port isolation */
        aal_port_mmshp_check_set(dev, i, bmp);
    }

    /* station move enable */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    igr_msk.s.station_move_en = 1;
    igr_cfg.station_move_en = 1;
    for(i = AAL_LPORT_ETH_NI0; i <= AAL_LPORT_ETH_NI6; i++){
        aal_port_ilpb_cfg_set(dev, i, igr_msk, &igr_cfg);
    }
    for(i = AAL_LPORT_CPU_0; i <= AAL_LPORT_CPU_7; i++){
        aal_port_ilpb_cfg_set(dev, i, igr_msk, &igr_cfg);
    }
    memset(&egr_msk, 0, sizeof(aal_elpb_cfg_msk_t));
    memset(&egr_cfg, 0, sizeof(aal_elpb_cfg_t));
    egr_msk.s.egr_port_stp_status = 1;
    egr_cfg.egr_port_stp_status = 1;
    egr_msk.s.egr_vlan_aware_mode = 1;
    egr_cfg.egr_vlan_aware_mode = TRUE;
    egr_msk.s.egr_ve_srch_en = 1;
    egr_cfg.egr_ve_srch_en = TRUE;
    egr_msk.s.egr_dest_wan = 1;
    egr_msk.s.egr_cls_cos_ena = 1;
    egr_msk.s.egr_cls_flow_id_ena = 1;
    egr_cfg.egr_cls_cos_ena = TRUE;
    egr_cfg.egr_cls_flow_id_ena = TRUE;

    aal_scfg_read(CFG_ID_WAN_PORT_TYPE_IS_L3,1,&wan_port_type);

    if (CA_E_OK != scfg_read(0, CFG_ID_WAN_PORT_ID,
        1,&wan_port_id)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_WAN_PORT_ID);
    }

    for(i = 0; i < AAL_PORT_NUM_LOGICAL; i++){

        if(AAL_LPORT_L3_WAN == i){
            if(wan_port_type == 1){
                egr_cfg.egr_ve_srch_en = FALSE;
            }
        }else{
             egr_cfg.egr_ve_srch_en = TRUE;
        }
        if(i == wan_port_id || (i >= AAL_PORT_LLID_TYPE_START)){
            egr_cfg.egr_dest_wan = TRUE;
        }else{
            egr_cfg.egr_dest_wan = FALSE;
        }

        aal_port_elpb_cfg_set(dev, i, egr_msk, &egr_cfg);
    }

    do
    {
        /* egress destination WAN of L3_LAN port should be 1, other ports uses default value 0 */
        aal_elpb_cfg_msk_t elpb_msk;
        aal_elpb_cfg_t elpb_cfg;

        memset(&elpb_msk, 0, sizeof(elpb_msk));
        memset(&elpb_cfg, 0, sizeof(elpb_cfg));

        elpb_msk.s.egr_dest_wan = 1;
        elpb_cfg.egr_dest_wan   = 1;

        (void)aal_port_elpb_cfg_set(dev, AAL_LPORT_L3_LAN, elpb_msk, &elpb_cfg);
    } while(0);

    if (CA_E_OK != scfg_read(0, CFG_ID_PORT_L2VLAN_OUTER_IS_S,
        CFG_ID_PORT_L2VLAN_OUTER_IS_S_LEN,g_aal_l2vlan_outer_is_s)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_PORT_L2VLAN_OUTER_IS_S);
    }

    if (CA_E_OK != scfg_read(0, CFG_ID_PORT_L2VLAN_INNER_IS_S,
        CFG_ID_PORT_L2VLAN_INNER_IS_S_LEN,g_aal_l2vlan_inner_is_s)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_PORT_L2VLAN_INNER_IS_S);
    }

    if (CA_E_OK != scfg_read(0, CFG_ID_L2FE_PORT_ILPB_WAN_IND_PORTS,
        CFG_ID_L2FE_PORT_ILPB_WAN_IND_PORTS_LEN,g_aal_ingr_port_is_wan)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_L2FE_PORT_ILPB_WAN_IND_PORTS);
    }

    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    igr_msk.s.top_s_tpid_enable = 1;
    igr_msk.s.inner_s_tpid_enable = 1;
    igr_msk.s.top_c_tpid_enable = 1;
    igr_msk.s.inner_c_tpid_enable = 1;
    igr_msk.s.c_tpid_match = 1;
    igr_msk.s.s_tpid_match = 1;
    igr_msk.s.other_tpid_match = 1;
    igr_msk.s.vlan_cmd_sel_bm = 1;
    igr_msk.s.dot1p_mark_control_bm = 1;
    igr_msk.s.top_802_1p_mode_sel = 1;
    igr_msk.s.inner_802_1p_mode_sel = 1;
    igr_msk.s.change_1p_if_pop = 1;
    igr_msk.s.sa_move_dis_fwd     = 1;
    igr_msk.s.flowid_sel_bm       = 1;
    igr_msk.s.dscp_mode_sel = 1;
    igr_msk.s.inner_vid_sel_bm = 1;
    igr_msk.s.ing_fltr_ena = 1;


    igr_cfg.vlan_cmd_sel_bm = 0xf;
    igr_cfg.dot1p_mark_control_bm = 0x7;
    igr_cfg.top_802_1p_mode_sel = 0;
    igr_cfg.inner_802_1p_mode_sel = 2;
    igr_cfg.change_1p_if_pop = 1;
    igr_cfg.sa_move_dis_fwd     = FALSE;
    igr_cfg.flowid_sel_bm       = 7;
    igr_cfg.dscp_mode_sel       = 2; /* from Rx DSCP Map */
    igr_cfg.inner_vid_sel_bm = 0xf;


    for(i = 0; i < AAL_PORT_NUM_LOGICAL; i++){
        if(i == AAL_LPORT_MC){ // for UUC packets
            igr_cfg.top_s_tpid_enable = g_aal_l2vlan_outer_is_s[i];
            igr_cfg.top_c_tpid_enable = (g_aal_l2vlan_outer_is_s[i] == 0);
            igr_cfg.inner_s_tpid_enable = g_aal_l2vlan_inner_is_s[i];
            igr_cfg.inner_c_tpid_enable = (g_aal_l2vlan_inner_is_s[i] == 0);

            igr_cfg.c_tpid_match = 0xf;
            igr_cfg.s_tpid_match = 0xf;
            igr_cfg.other_tpid_match = 0xf;
            igr_cfg.ing_fltr_ena = TRUE;
        }else if(i<32){
            igr_cfg.top_s_tpid_enable = g_aal_l2vlan_outer_is_s[i];
            igr_cfg.top_c_tpid_enable = (g_aal_l2vlan_outer_is_s[i] == 0);
            igr_cfg.inner_s_tpid_enable = g_aal_l2vlan_inner_is_s[i];
            igr_cfg.inner_c_tpid_enable = (g_aal_l2vlan_inner_is_s[i] == 0);
            igr_cfg.ing_fltr_ena = TRUE;

            if (i == AAL_LPORT_L3_WAN || i == AAL_LPORT_L3_LAN)
            {
                igr_cfg.stp_mode = AAL_STP_STATE_FWD_NO_LEARN;
                igr_msk.s.stp_mode = 1;
            }else{
                igr_cfg.stp_mode = AAL_STP_STATE_FWD_AND_LEARN;
                igr_msk.s.stp_mode = 1;
            }

            /* set 0x8100 as default comparing tpid */
            igr_cfg.c_tpid_match = (1 << AAL_VLAN_TPID_INDEX_8100);
            igr_cfg.s_tpid_match = (1 << AAL_VLAN_TPID_INDEX_8100);
            igr_cfg.other_tpid_match = (1 << AAL_VLAN_TPID_INDEX_8100);
        }else{
            igr_cfg.top_s_tpid_enable = 1;
            igr_cfg.top_c_tpid_enable = 0;
            igr_cfg.inner_s_tpid_enable = 1;
            igr_cfg.inner_c_tpid_enable = 0;
            igr_cfg.ing_fltr_ena = TRUE;
        }
        igr_msk.s.wan_ind = 1;
        igr_cfg.wan_ind = g_aal_ingr_port_is_wan[i];
        aal_port_ilpb_cfg_set(dev, i, igr_msk, &igr_cfg);
    }


    __aal_port_change_1p_if_pop_en(dev, 1);

    if (CA_E_OK != scfg_read(0, CFG_ID_L2FE_CLS_PORT_ENTRY_NUM,
        CFG_ID_L2FE_CLS_PORT_ENTRY_NUM_LEN,__g_cls_ingr_cls_port_entry_num)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_L2FE_CLS_PORT_ENTRY_NUM);
    }

    if (CA_E_OK != scfg_read(0, CFG_ID_L2FE_CLS_PORT_EGR_ENTRY_NUM,
        CFG_ID_L2FE_CLS_PORT_EGR_ENTRY_NUM_LEN,__g_cls_egr_cls_port_entry_num)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_L2FE_CLS_PORT_EGR_ENTRY_NUM);
    }

    /* the classification rules allocation for 4 UNI */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    memset(&egr_msk, 0, sizeof(aal_elpb_cfg_msk_t));
    memset(&egr_cfg, 0, sizeof(aal_elpb_cfg_t));

    igr_msk.s.igr_cls_lookup_en = 1;
    igr_msk.s.cls_start = 1;
    igr_msk.s.cls_length = 1;


    egr_msk.s.egr_cls_lookup_en = 1;
    egr_msk.s.egr_cls_start = 1;
    egr_msk.s.egr_cls_length = 1;
    egr_msk.s.egr_cls_flow_id_ena = 1;
    egr_msk.s.egr_cls_cos_ena = 1;

    egr_cfg.egr_cls_lookup_en = 1;
    egr_cfg.egr_cls_length = AAL_ELPB_CLS_LENGTH;
    egr_cfg.egr_cls_flow_id_ena = TRUE;
    egr_cfg.egr_cls_cos_ena = TRUE;

    for (lpid = AAL_PORT_START_FOR_CLS; lpid <= AAL_LPORT_LLID_GEM_INDEX_31; lpid++) {
        if(__g_cls_ingr_cls_port_entry_num[lpid] != 0){
            igr_cfg.igr_cls_lookup_en = 1;
        }else{
            igr_cfg.igr_cls_lookup_en = 0;
        }


        if(lpid >= AAL_LPORT_LLID_GEM_INDEX_0){

            igr_cfg.cls_start =
                        AAL_ILPB_CLS_START + l2_cls_entry_id;

            igr_cfg.cls_length = __g_cls_ingr_cls_port_entry_num[lpid];

            l2_cls_entry_id += __g_cls_ingr_cls_port_entry_num[lpid];

            aal_port_ilpb_cfg_set(dev, lpid, igr_msk, &igr_cfg);

        }else{
            igr_cfg.cls_length = __g_cls_ingr_cls_port_entry_num[lpid];
            igr_cfg.cls_start =
                AAL_ILPB_CLS_START + l2_cls_entry_id;
            l2_cls_entry_id += __g_cls_ingr_cls_port_entry_num[lpid];
            aal_port_ilpb_cfg_set(dev, lpid, igr_msk, &igr_cfg);
        }
        if(__g_cls_ingr_cls_port_entry_num[lpid]){
          //   printk("port %d ingress cls start %d length %d\n", lpid, igr_cfg.cls_start, igr_cfg.cls_length);
         }

        if(__g_cls_egr_cls_port_entry_num[lpid] != 0){
            egr_cfg.egr_cls_lookup_en = 1;
        }else{
            egr_cfg.egr_cls_lookup_en = 0;
        }

        if(lpid >= AAL_LPORT_LLID_GEM_INDEX_0){

            egr_cfg.egr_cls_start =  AAL_ELPB_CLS_START + l2_egr_cls_entry_id;
            egr_cfg.egr_cls_length = __g_cls_egr_cls_port_entry_num[lpid];

            l2_egr_cls_entry_id += __g_cls_egr_cls_port_entry_num[lpid];
            aal_port_elpb_cfg_set(dev, lpid, egr_msk, &egr_cfg);
        }else{
            egr_cfg.egr_cls_start =  AAL_ELPB_CLS_START + l2_egr_cls_entry_id;
            l2_egr_cls_entry_id += __g_cls_egr_cls_port_entry_num[lpid];
            egr_cfg.egr_cls_length = __g_cls_egr_cls_port_entry_num[lpid];
            aal_port_elpb_cfg_set(dev, lpid, egr_msk, &egr_cfg);
        }
        if(__g_cls_egr_cls_port_entry_num[lpid]){
            //printk("port %d egress cls start %d length %d\n", lpid, egr_cfg.egr_cls_start, egr_cfg.egr_cls_length);
        }
    }


    /* ARB LDPID mapping */
    for(my_mac = 0; my_mac <= 1; my_mac++)
    {
        for (lpid = AAL_LPORT_ETH_NI0; lpid <= AAL_LPORT_ETH_NI7; lpid++) {
            aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, lpid, lpid);
        }
        for (lpid = AAL_LPORT_DEEPQ_0; lpid <= AAL_LPORT_DEEPQ_7; lpid++) {
            aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 1, lpid, AAL_PPORT_QM);
        }
        for (lpid = AAL_LPORT_9QUEUE_NI0; lpid <= AAL_LPORT_9QUEUE_NI7; lpid++) {
            aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, lpid, AAL_PPORT_OAM);
        }
        for (lpid = AAL_LPORT_CPU_0; lpid <= AAL_LPORT_CPU_7; lpid++) {
            aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, lpid, AAL_PPORT_CPU);
        }

        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, 7, AAL_PPORT_BLACKHOLE);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 1, 7, AAL_PPORT_BLACKHOLE);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_L3_WAN, AAL_PPORT_L3_WAN);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_L3_LAN, AAL_PPORT_L3_LAN);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_OAM, AAL_PPORT_OAM);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_MC, AAL_PPORT_MC);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_CPU_Q, AAL_PPORT_CPU);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_SPCL, AAL_PPORT_BLACKHOLE);
        aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, AAL_LPORT_BLACKHOLE, AAL_PPORT_BLACKHOLE);
        for (lpid = AAL_LPORT_CPU_MQ_0; lpid <= AAL_LPORT_CPU_MQ_31; lpid++) {
            aal_port_arb_ldpid_pdpid_map_set(dev, my_mac, 0, lpid, AAL_PPORT_NI7);
        }
    }

    aal_scfg_read(CFG_ID_L2FE_CLASSIFIER_USE_32_ENTRY,1,&l2fe_use_32_entry);
    if(l2fe_use_32_entry == 1){
        __aal_port_pipeline_clk_num_set(0xC);
    }else{
        __aal_port_pipeline_clk_num_set(0xA);
    }

    ret = aal_eth_ports_restore(dev);
    if (CA_E_OK != ret) {
        printk("ETH Port global init fail, return %d\r\n", ret);
        return ret;
    }

    return CA_E_OK;
}

EXPORT_SYMBOL(aal_port_mmshp_check_set);
EXPORT_SYMBOL(aal_port_ilpb_cfg_set);
EXPORT_SYMBOL(aal_port_trunk_group_hash_get);
EXPORT_SYMBOL(aal_port_mmshp_check_get);
EXPORT_SYMBOL(aal_port_ilpb_cfg_get);
EXPORT_SYMBOL(aal_port_trunk_group_member_set);
EXPORT_SYMBOL(aal_port_elpb_cfg_get);
EXPORT_SYMBOL(aal_port_trunk_group_member_get);
EXPORT_SYMBOL(aal_port_ippb_cfg_set);
EXPORT_SYMBOL(aal_port_dpb_cfg_set);
EXPORT_SYMBOL(aal_port_arb_ldpid_pdpid_map_get);
EXPORT_SYMBOL(aal_port_init);
EXPORT_SYMBOL(aal_port_elpb_cfg_set);
EXPORT_SYMBOL(aal_port_dpb_ctrl_get);
EXPORT_SYMBOL(aal_port_ippb_cfg_get);
EXPORT_SYMBOL(aal_port_trunk_group_hash_set);
EXPORT_SYMBOL(aal_port_trunk_dest_map_set);
EXPORT_SYMBOL(aal_port_arb_ldpid_pdpid_map_set);
EXPORT_SYMBOL(aal_port_dpb_ctrl_set);
EXPORT_SYMBOL(aal_port_dpb_cfg_get);
EXPORT_SYMBOL(aal_port_trunk_dest_map_get);
EXPORT_SYMBOL(aal_port_enable_get);
EXPORT_SYMBOL(aal_port_enable_set);