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

#include <generated/ca_ne_autoconf.h>
#include "aal_l2_vlan.h"
#include "aal_arb.h"
#include "aal_common.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_mcast.h"
#include "scfg.h"

#define __AAL_L2_VLAN_MAX_FIB_INDEX         127
#define __AAL_L2_VLAN_LOGIC_PORT_NUM        64
/*#define REG_READ(REG_ADDR, VALUE)   do{\
                                        VALUE=\
                                    }while(0)*/


#define G3_APB0_ADDRESS_BASE 0xf4300000

APB0_t *g_g3_dev_ptr = (APB0_t *)G3_APB0_ADDRESS_BASE;

#define AAL_L2FE_VE_REG_ADDR(reg) L2FE_VE_##reg
#define AAL_L2FE_MEMSHIP_MAX    15

#define __L2_VLAN_LOCK()    \
    do{ \
        __l2_vlan_lock();\
    }while(0)

#define __L2_VLAN_UNLOCK()    \
    do{ \
        __l2_vlan_unlock();\
    }while(0)

extern ca_status_t aal_mc_ni_mce_indx_dump(
        CA_IN ca_device_id_t dev,
        CA_IN ca_uint16_t entry_idx
);


ca_uint g_l2_vlan_lock;
ca_uint8_t __g_l2_vlan_port_map[__AAL_L2_VLAN_LOGIC_PORT_NUM] = {
                                           0,1,2,3,4,5,6,7,0,0,0,0,0,0,0,0,
                                           8,9,0,10,11,12,13,14,15,15,0,0,0,0,0,0,
                                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
                                                               };
ca_uint8_t g_l2_vlan_unknown_port_fib_map[__AAL_L2_VLAN_LOGIC_PORT_NUM] = {
                                            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};
static ca_uint64_t __g_unkown_vlan_mc_fib_bmp = 0;

ca_uint32_t g_vlan_flooding_domain_1[CFG_ID_FLOODING_DOMAIN_1_LEN];
ca_uint32_t g_vlan_mcgid_0 = 0;
ca_uint32_t g_vlan_mcgid_1 = 0;

void  __l2_vlan_lock(void)
{
    ca_spin_lock(g_l2_vlan_lock);
}

void  __l2_vlan_unlock(void)
{
    ca_spin_unlock(g_l2_vlan_lock);
}

static ca_status_t
__aal_l2_vlan_act_cfg_set (ca_device_id_t device_id,
                ca_uint8 fib_id,
                aal_l2_vlan_fib_mask_t mask,
                aal_l2_vlan_fib_t * cfg)
{
    L2FE_VE_WAN_SVL_FIB_DATA1_t svl_map_data1;
    L2FE_VE_WAN_SVL_FIB_DATA0_t svl_map_data0;

    READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
    svl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
    svl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));

    if (mask.s.vlan_group_id)
    {
        svl_map_data1.bf.wan_svl_fib_vl_group_id = cfg->vlan_group_id;
    }
    if (mask.s.memship_bm)
    {
        svl_map_data0.bf.wan_svl_fib_membership_bm = cfg->memship_bm;
    }
    if (mask.s.untag_bm)
    {
        svl_map_data0.bf.wan_svl_fib_egress_top_untag_bm = cfg->untag_bm;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE (svl_map_data1.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
        CA_NE_REG_WRITE (svl_map_data0.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));
        WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
    }
    return CA_E_OK;
}

static ca_status_t
__aal_l2_vlan_act_cfg_get (ca_device_id_t device_id,
                ca_uint8 fib_id, aal_l2_vlan_fib_t * cfg)
{
    L2FE_VE_WAN_SVL_FIB_DATA1_t svl_map_data1;
    L2FE_VE_WAN_SVL_FIB_DATA0_t svl_map_data0;

    READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
    svl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
    svl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));

    cfg->vlan_group_id = svl_map_data1.bf.wan_svl_fib_vl_group_id;
    cfg->memship_bm = svl_map_data0.bf.wan_svl_fib_membership_bm;
    cfg->untag_bm = svl_map_data0.bf.wan_svl_fib_egress_top_untag_bm;

    return CA_E_OK;
}

ca_status_t
aal_l2_vlan_action_cfg_set (CA_IN ca_device_id_t device_id,
                 CA_IN ca_uint8 fib_id,
                CA_IN aal_l2_vlan_fib_mask_t mask,
                CA_IN aal_l2_vlan_fib_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    
    if (fib_id > __AAL_L2_VLAN_MAX_FIB_INDEX)
    {
        return CA_E_PARAM;
    }
    if (NULL == cfg)
    {
        return CA_E_PARAM;
    }
    
    __L2_VLAN_LOCK();
    
    ret = __aal_l2_vlan_act_cfg_set (device_id, fib_id, mask, cfg);
    
    __L2_VLAN_UNLOCK();
    return ret;
}

ca_status_t
aal_l2_vlan_action_cfg_get (CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8 fib_id,
                CA_OUT aal_l2_vlan_fib_t * cfg)
{
    ca_status_t ret = CA_E_OK;

    if (fib_id > __AAL_L2_VLAN_MAX_FIB_INDEX)
    {
        return CA_E_PARAM;
    }
    if (NULL == cfg)
    {
        return CA_E_PARAM;
    }
    
    __L2_VLAN_LOCK();
    ret = __aal_l2_vlan_act_cfg_get (device_id, fib_id, cfg);
    __L2_VLAN_UNLOCK();
    
    return ret;
}

ca_status_t
aal_l2_vlan_fib_map_set (CA_IN ca_device_id_t device_id,
             CA_IN aal_l2_vlan_type_t vlan_type,
             CA_IN ca_uint16_t vlan_id,
             CA_IN aal_l2_vlan_fib_map_mask_t mask,
             CA_IN aal_l2_vlan_fib_map_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    L2FE_VE_WAN_SVL_IDX_DATA_t vl_index;
    if (vlan_type > AAL_L2_VLAN_TYPE_MAX)
    {
        return CA_E_PARAM;
    }
    if (cfg == NULL)
    {
        return CA_E_PARAM;
    }
    if (vlan_id > 4095)
    {
        return CA_E_PARAM;
    }
    if (cfg->fib_id > __AAL_L2_VLAN_MAX_FIB_INDEX)
    {
        return CA_E_PARAM;
    }
    
    vl_index.wrd = 0;
    
    __L2_VLAN_LOCK();
    
    switch (vlan_type)
    {
        case AAL_L2_VLAN_TYPE_WAN_SVLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
            if (mask.s.fib_id)
            {
                vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
            }
            if (mask.s.valid)
            {
                vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
            }
            CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
            WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
            
        break;
        case AAL_L2_VLAN_TYPE_WAN_CVLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
            
            if (mask.s.fib_id)
            {
                vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
            }
            if (mask.s.valid)
            {
                vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
            }
            
            CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
            WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
        break;
      
        case AAL_L2_VLAN_TYPE_LAN_VLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
            
            if (mask.s.fib_id)
            {
                vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
            }
            if (mask.s.valid)
            {
                vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
            }
            CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
            WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
        
        break;
      
        default:
            ret = CA_E_PARAM;
            goto end;
    }
end:
    __L2_VLAN_UNLOCK();
    return ret;
}

ca_status_t
aal_l2_vlan_fib_map_get (CA_IN ca_device_id_t device_id,
             CA_IN aal_l2_vlan_type_t vlan_type,
             CA_IN ca_uint16_t vlan_id,
             CA_OUT aal_l2_vlan_fib_map_t * cfg)
{
    ca_status_t ret = CA_E_OK;

    L2FE_VE_WAN_SVL_IDX_DATA_t vl_index;

    if (vlan_type > AAL_L2_VLAN_TYPE_MAX)
    {
        return CA_E_PARAM;
    }
    if (cfg == NULL)
    {
        return CA_E_PARAM;
    }
    if (vlan_id > 4095)
    {
        return CA_E_PARAM;
    }
    vl_index.wrd = 0;

    __L2_VLAN_LOCK();

    switch (vlan_type)
    {
        case AAL_L2_VLAN_TYPE_WAN_SVLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
        break;
        case AAL_L2_VLAN_TYPE_WAN_CVLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
        break;
        case AAL_L2_VLAN_TYPE_LAN_VLAN:
            READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
            vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
        break;
        default:
            ret = CA_E_PARAM;
            goto end;
    }
    cfg->fib_id = vl_index.bf.wan_svl_idx_fib_idx;
    cfg->valid = vl_index.bf.wan_svl_idx_entry_vld;
end:
    __L2_VLAN_UNLOCK();
    
return ret;
}

ca_status_t
aal_l2_vlan_default_cfg_set (CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_vlan_default_cfg_mask_t mask,
                 CA_IN aal_l2_vlan_default_cfg_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    if (cfg == NULL)
    {
        return CA_E_PARAM;
    }
    
    __L2_VLAN_LOCK();
    if (mask.s.vlan_mode)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLC_VLAN_MODE_CTRL, l2e_glb_vlan_mode,
        cfg->vlan_mode);
    }
    
    if (mask.s.cmp_tpid_svlan)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_S, tpid, cfg->cmp_tpid_svlan);
    }

    if (mask.s.cmp_tpid_cvlan)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_C, tpid, cfg->cmp_tpid_cvlan);
    }

    if (mask.s.cmp_tpid_other)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_other);
    }
    if (mask.s.cmp_tpid_user)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_user);
    }
    if (mask.s.tx_tpid0)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_A, tpid, cfg->tx_tpid0);
    }
    if (mask.s.tx_tpid1)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_B, tpid, cfg->tx_tpid1);
    }

    if (mask.s.tx_tpid2)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_C, tpid, cfg->tx_tpid2);
    }

    if (mask.s.tx_tpid3)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_D, tpid, cfg->tx_tpid3);
    }

    if (mask.s.wan_memship_check_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_wan,
        cfg->wan_memship_check_ena);
    }
    if (mask.s.lan_memship_check_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_lan,
        cfg->lan_memship_check_ena);
    }
    if (mask.s.rx_unknown_vlan_memship_check_bmp)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_RX_UNKWN_VLAN_DFT, memshp_bm,
        cfg->rx_unknown_vlan_memship_check_bmp);
    }
    if (mask.s.rx_unknown_vlan_grp_id)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_RX_UNKWN_VLAN_DFT, vl_grp_id,
        cfg->rx_unknown_vlan_grp_id);
    }
    if (mask.s.tx_unknown_vlan_memship_check_bmp)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, memshp_bm,
        cfg->tx_unknown_vlan_memship_check_bmp);
    }
    if (mask.s.tx_unknown_vlan_untag_bmp)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, untag_bm,
        cfg->tx_unknown_vlan_untag_bmp);
    }
    if (mask.s.tx_unknown_vlan_stp_mode_forward_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_1, stp_mode,
        cfg->tx_unknown_vlan_stp_mode_forward_ena);
    }
    if(mask.s.unknown_vlan_fwd_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLC_DPID_FWD_CTRL, unkvlan_fwd,
        cfg->unknown_vlan_fwd_ena);
    }
    if(mask.s.vlan_4095_fwd_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLC_DPID_FWD_CTRL, vid_fff_fwd,
        cfg->vlan_4095_fwd_ena);
    }
    if(mask.s.loopback_packet_fwd_ena)
    {
        REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,
        cfg->loopback_packet_fwd_ena);
    }
       
    __L2_VLAN_UNLOCK();
    return ret;
}

ca_status_t
aal_l2_vlan_default_cfg_get (CA_IN ca_device_id_t device_id,
                 CA_OUT aal_l2_vlan_default_cfg_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    if (cfg == NULL)
    {
        return CA_E_PARAM;
    }
    __L2_VLAN_LOCK();
    
    REG_L2FE_FIELD_READ (L2FE_PLC_VLAN_MODE_CTRL, l2e_glb_vlan_mode,
        cfg->vlan_mode);
    REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_S, tpid, cfg->cmp_tpid_svlan);
    REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_C, tpid, cfg->cmp_tpid_cvlan);
    REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_other);
    REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_USER, tpid, cfg->cmp_tpid_user);
    REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_A, tpid, cfg->tx_tpid0);
    REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_B, tpid, cfg->tx_tpid1);
    REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_C, tpid, cfg->tx_tpid2);
    REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_D, tpid, cfg->tx_tpid3);
    REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_wan,
        cfg->wan_memship_check_ena);
    REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_lan,
        cfg->lan_memship_check_ena);
    REG_L2FE_FIELD_READ (L2FE_PLE_RX_UNKWN_VLAN_DFT, memshp_bm,
        cfg->rx_unknown_vlan_memship_check_bmp);
    REG_L2FE_FIELD_READ (L2FE_PLE_RX_UNKWN_VLAN_DFT, vl_grp_id,
        cfg->rx_unknown_vlan_grp_id);
    REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, memshp_bm,
        cfg->tx_unknown_vlan_memship_check_bmp);
    REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, untag_bm,
        cfg->tx_unknown_vlan_untag_bmp);
    REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_1, stp_mode,
        cfg->tx_unknown_vlan_stp_mode_forward_ena);

    REG_L2FE_FIELD_READ (L2FE_PLC_DPID_FWD_CTRL, unkvlan_fwd,
        cfg->unknown_vlan_fwd_ena);
    REG_L2FE_FIELD_READ (L2FE_PLC_DPID_FWD_CTRL, vid_fff_fwd,
        cfg->vlan_4095_fwd_ena);
    REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,
        cfg->loopback_packet_fwd_ena);

    __L2_VLAN_UNLOCK();

    return ret;
}

ca_status_t aal_l2_vlan_port_memship_map_set(CA_IN ca_device_id_t device_id,CA_IN ca_uint32_t logic_port, CA_IN ca_uint32_t mapped_port)
{
    if(mapped_port > AAL_L2FE_MEMSHIP_MAX){
        return CA_E_PARAM;
    }
    
    AAL_LPID_CHECK(logic_port);
    
    __L2_VLAN_LOCK();
    
    CA_NE_REG_WRITE(mapped_port, L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
    WRITE_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
    
    __L2_VLAN_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_l2_vlan_port_memship_map_get(CA_IN ca_device_id_t device_id,CA_IN ca_uint32_t logic_port, CA_OUT ca_uint32_t *mapped_port)
{
    if(mapped_port == NULL){
        return CA_E_PARAM;
    }
    
    AAL_LPID_CHECK(logic_port);
    
    __L2_VLAN_LOCK();
    
     READ_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
    *mapped_port = CA_NE_REG_READ(L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
    
     __L2_VLAN_UNLOCK();
    
    return CA_E_OK;
}


ca_status_t aal_l2_vlan_init(ca_device_id_t device_id)
{
    aal_l2_vlan_fib_mask_t l2_vlan_fib_mask;
    aal_l2_vlan_fib_t      l2_vlan_fib;
    aal_l2_vlan_default_cfg_mask_t def_cfg_msk;
    aal_l2_vlan_default_cfg_t def_cfg;
    repl_key_t		repl_key;
    ca_uint8_t		fib_id = 0;
    ca_int16_t		mc_fib_id = 0;
    ca_uint32_t		lpid = 0;
    aal_l2_vlan_insert_tag_num_t insert_tag_num;
    ca_uint32_t i = 0;
    ni_mce_indx_tbl_entry_t key;

    aal_l2_vlan_port_tx_tpid_sel_mask_t             tx_tpid_sel_mask;
    aal_l2_vlan_port_tx_tpid_sel_t                  tx_tpid_sel;

    memset(&l2_vlan_fib, 0, sizeof(aal_l2_vlan_fib_t));

    l2_vlan_fib_mask.u32 = 0;
    l2_vlan_fib_mask.s.stp_stat = 1;
    l2_vlan_fib.stp_stat = AAL_L2_VLAN_STP_STAT_FORWARD_AND_LEARN;

    if(CA_OSAL_OK != ca_spin_lock_init(&g_l2_vlan_lock, SPINLOCK_CMN)){
        ca_printf("%s: init spinlock fail\n", __func__);
        return CA_E_PARAM;
    }
    for(fib_id = 0; fib_id<128; fib_id++){
         aal_l2_vlan_action_cfg_set(device_id, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
     }
    //__g_l2_vlan_port_map[AAL_LPORT_L3_WAN] = __g_l2_vlan_port_map[AAL_LPORT_ETH_WAN];
    for(lpid = 0; lpid<__AAL_L2_VLAN_LOGIC_PORT_NUM; lpid++){
        aal_l2_vlan_port_memship_map_set(device_id, lpid, __g_l2_vlan_port_map[lpid]);
         tx_tpid_sel_mask.u32 = 0;
        tx_tpid_sel_mask.s.svlan = 1;
        tx_tpid_sel.svlan = AAL_L2_VLAN_TX_TPID_SEL_OLD_CSR_A;
        tx_tpid_sel_mask.s.cvlan = 1;
        tx_tpid_sel.cvlan = AAL_L2_VLAN_TX_TPID_SEL_OLD_CSR_A;
        aal_l2_vlan_tx_tpid_sel_set(device_id, lpid, tx_tpid_sel_mask, &tx_tpid_sel);
    }
    insert_tag_num.tag_num = 2;
    for(lpid = 0; lpid<12; lpid++){
        aal_l2_vlan_insert_tag_num_set(device_id, lpid, &insert_tag_num);
    }

    memset(&def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
    def_cfg_msk.u32 = 0;

    def_cfg_msk.s.vlan_mode = 1;
    def_cfg_msk.s.rx_unknown_vlan_memship_check_bmp = 1;
    def_cfg_msk.s.tx_unknown_vlan_memship_check_bmp = 1;
    def_cfg_msk.s.tx_unknown_vlan_stp_mode_forward_ena = 1;
    def_cfg_msk.s.wan_memship_check_ena = 1;
    def_cfg_msk.s.lan_memship_check_ena = 1;
    def_cfg_msk.s.unknown_vlan_fwd_ena = 1;
    def_cfg_msk.s.rx_unknown_vlan_grp_id = 1;
    def_cfg_msk.s.tx_tpid0 = 1;
    def_cfg_msk.s.tx_tpid1 = 1;
    def_cfg_msk.s.tx_tpid2 = 1;
    def_cfg_msk.s.tx_tpid3 = 1;

    def_cfg.vlan_mode = AAL_L2_VLAN_MODE_SVL;
    def_cfg.tx_unknown_vlan_stp_mode_forward_ena = 1;
    def_cfg.wan_memship_check_ena = TRUE;
    def_cfg.lan_memship_check_ena = TRUE;
    def_cfg.unknown_vlan_fwd_ena  = TRUE;
    def_cfg.tx_tpid0 = 0x8100;
    def_cfg.tx_tpid1 = 0x8100;
    def_cfg.tx_tpid2 = 0x8100;
    def_cfg.tx_tpid3 = 0x8100;


    /* Create and config default flooding domain for UUC/BC/UMC */
    if (CA_E_OK != scfg_read(0, CFG_ID_FLOODING_DOMAIN_1,
        CFG_ID_FLOODING_DOMAIN_1_LEN * sizeof(ca_uint32_t),
        (ca_uint8_t *) g_vlan_flooding_domain_1)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_FLOODING_DOMAIN_1);
    }


    for (i = 0; i < CFG_ID_FLOODING_DOMAIN_1_LEN; i++){
        if (g_vlan_flooding_domain_1[i] != 0xFF){
            memset(&repl_key, 0, sizeof(repl_key_t));
            {
                repl_key.ldpid = g_vlan_flooding_domain_1[i];
            }
#ifdef  MC_FIB_COS_CMD
            repl_key.cos = 1;
            repl_key.cos_cmd = 1;
#endif

            mc_fib_id = aal_mc_repl_bit_alloc(&repl_key);

            g_l2_vlan_unknown_port_fib_map[g_vlan_flooding_domain_1[i]] = mc_fib_id;
            __g_unkown_vlan_mc_fib_bmp |= 1<<mc_fib_id;

            def_cfg.tx_unknown_vlan_memship_check_bmp |= (1 << __g_l2_vlan_port_map[g_vlan_flooding_domain_1[i]]);
            def_cfg.rx_unknown_vlan_memship_check_bmp |= (1 << __g_l2_vlan_port_map[g_vlan_flooding_domain_1[i]]);
        }
    }
    def_cfg.tx_unknown_vlan_memship_check_bmp |= (1 << __g_l2_vlan_port_map[0x18]);
    def_cfg.rx_unknown_vlan_memship_check_bmp |= (1 << __g_l2_vlan_port_map[0x18]);
    key.mc_vec = __g_unkown_vlan_mc_fib_bmp;

    g_vlan_mcgid_0 = aal_mcgid_alloc(&key);
    if (g_vlan_mcgid_0 == -1) {
        printk("%s ERROR in alloc mcgid \n", __func__);
        return CA_E_PARAM;
    }

    def_cfg.rx_unknown_vlan_grp_id = g_vlan_mcgid_0;

    aal_l2_vlan_default_cfg_set(device_id, def_cfg_msk, &def_cfg);
 
    l2_vlan_fib_mask.u32 = 0;
    l2_vlan_fib_mask.s.vlan_group_id = 1;
    l2_vlan_fib.vlan_group_id = g_vlan_mcgid_1;

    aal_l2_vlan_action_cfg_set(device_id, 127, l2_vlan_fib_mask, &l2_vlan_fib);
    REG_L2FE_FIELD_WRITE(L2FE_PP_DEBUG_BUS, ni2fe_pkt_cnt_wrap_en, 1);
    REG_L2FE_FIELD_WRITE(L2FE_PE_DEBUG, fe2tm_pkt_cnt_wrap_en , 1);
    
    REG_L2FE_FIELD_WRITE(L2FE_PLE_CTL_REG, pon_mode, 1);
    REG_L2FE_FIELD_WRITE(L3FE_GLB_GLB_CFG, l3fe_pon_mode, 1);
    REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,1);
    REG_L2FE_FIELD_WRITE (L2FE_PE_CTRL, enhance_tpid_mode,1);
    ca_printf("init L2FE,L3FE vlan done.\n");

    return CA_E_OK;
}

ca_status_t aal_l2_fe_pp_heada_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_heada_t *heada)
{
    if(heada == NULL){
        return CA_E_PARAM;
    }

    heada->heada_hi         = CA_NE_REG_READ(L2FE_PP_HEADER_A_HI);
    heada->heada_mid        = CA_NE_REG_READ(L2FE_PP_HEADER_A_MID);
    heada->heada_low        = CA_NE_REG_READ(L2FE_PP_HEADER_A_LOW);
    heada->cpu_flag         = (heada->heada_hi>>31)&1;
    heada->deep_q           = (heada->heada_hi>>30)&1;
    heada->policy_ena       = (heada->heada_hi>>16)&3;
    heada->policy_group_id  = (heada->heada_hi>>27)&7;
    heada->policy_id        = (heada->heada_hi>>18)&0x1ff;
    heada->marked           = (heada->heada_hi>>15)&1;
    heada->mirror           = (heada->heada_hi>>14)&1;
    heada->no_drop          = (heada->heada_hi>>13)&1;
    heada->rx_packet_type   = (heada->heada_hi>>11)&3;
    heada->drop_code        = (heada->heada_hi>>8)&7;
    heada->mc_group_id      = (heada->heada_hi>>0)&0xff;
    heada->header_type      = (heada->heada_mid>>30)&3;
    heada->fe_bypass        = (heada->heada_mid>>29)&1;
    heada->packet_size      = (heada->heada_mid>>15)&0x3fff;
    heada->cos              = (heada->heada_mid >> 0)&0x7;
    heada->logic_spid       = (heada->heada_mid>>9) &0x3f;
    heada->logic_dpid       = (heada->heada_mid>>3)&0x3f;

    return CA_E_OK;
}


ca_status_t aal_l2_fe_pp_nierr_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_ni_status_t *err)
{
    if(err == NULL){
        return CA_E_PARAM;
    }

    err->sop_eop_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_0);
    err->runt_l2_packet     = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_1);
    err->sob_eob_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_2);
    err->sop_eop_gap        = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_3);
    err->pp_pingpong_overflow = CA_NE_REG_READ(L2FE_PP_PINGPONG_STS)&1;
    err->ready              = CA_NE_REG_READ(L2FE_PP_NI_INTF_STS)&1;
    err->drop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT)&0xffff;
    err->sop_cnt            = (CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)>>16)&0xffff;
    err->eop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)&0xffff;
    
    return CA_E_OK;
}

ca_status_t aal_l2_fe_pe_heada_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_heada_t *heada)
{
    if(heada == NULL){
        return CA_E_PARAM;
    }
    
    heada->heada_hi         = CA_NE_REG_READ(L2FE_PE_HEADER_A_HI);
    heada->heada_mid        = CA_NE_REG_READ(L2FE_PE_HEADER_A_MID);
    heada->heada_low        = CA_NE_REG_READ(L2FE_PE_HEADER_A_LOW);
    heada->cpu_flag         = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>31)&1;
    heada->deep_q           = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>30)&1;
    heada->policy_ena       = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>16)&3;
    heada->policy_group_id  = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>27)&7;
    heada->policy_id        = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>18)&0x1ff;
    heada->marked           = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>15)&1;
    heada->mirror           = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>14)&1;
    heada->no_drop          = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>13)&1;
    heada->rx_packet_type   = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>11)&3;
    heada->drop_code        = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>8)&7;
    heada->mc_group_id      = (CA_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>0)&0xff;
    heada->header_type      = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>30)&3;
    heada->fe_bypass        = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>29)&1;
    heada->packet_size      = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>15)&0x3fff;
    heada->cos              = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID) >> 0)&0x7;
    heada->logic_spid       = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>9) &0x3f;
    heada->logic_dpid       = (CA_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>3)&0x3f;
    
    return CA_E_OK;
}


ca_status_t aal_l2_fe_drop_source_cnt_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_drop_source_t *drop_source_cnt)
{
    ca_uint8_t drop_source_id;
    ca_uint32_t *p = (ca_uint32_t*)drop_source_cnt;
    
    if(drop_source_cnt == NULL){
        return CA_E_PARAM;
    }

     for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
        READ_INDIRCT_REG (device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
        *p =  CA_NE_REG_READ(L2FE_PE_DROP_STTS_DATA);
        p++;
    }
     
    for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
        CA_NE_REG_WRITE(0,L2FE_PE_DROP_STTS_DATA);
        WRITE_INDIRCT_REG (device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
    }
    
    return CA_E_OK;
}
ca_status_t aal_l2_pe2tm_pkt_cnt_set(CA_IN  ca_device_id_t device_id,
                                      CA_IN ca_pe2tm_cnt_mask_t mask,
                                      CA_IN ca_pe2tm_cnt_t *cnt)
{

    if(NULL == cnt){
        return CA_E_PARAM;
    }

    //temp_value = CA_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
    CA_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
    
    return CA_E_OK;
}


ca_status_t aal_l2_pe2tm_pkt_cnt_get(CA_IN  ca_device_id_t device_id,
                                      CA_OUT ca_pe2tm_cnt_t *cnt)
{
    ca_uint32_t temp_value = 0;

    if(NULL == cnt){
        return CA_E_PARAM;
    }
    
    temp_value = CA_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
    cnt->tm2fe_ready = (temp_value >>31) &1;
    cnt->sop_cnt     = (temp_value>>16) &0x7fff;
    cnt->eop_cnt     = (temp_value>>0) &0x7fff;
    CA_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
    
    return CA_E_OK;
}



ca_status_t aal_l2_ni2pp_pkt_cnt_set(CA_IN  ca_device_id_t device_id,
                                      CA_IN ca_ni2pp_cnt_t *cnt)
{

    if(NULL == cnt){
        return CA_E_PARAM;
    }

    //temp_value = CA_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
    CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
    CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
    
    return CA_E_OK;
}


ca_status_t aal_l2_ni2pp_pkt_cnt_get(CA_IN  ca_device_id_t device_id,
                                      CA_OUT ca_ni2pp_cnt_t *cnt)
{
    ca_uint32_t temp_value = 0;

    if(NULL == cnt){
        return CA_E_PARAM;
    }

    cnt->drp_cnt = CA_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT);
    temp_value = CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT);

    cnt->sop_cnt     = (temp_value>>16) &0x7fff;
    cnt->eop_cnt     = (temp_value>>0) &0x7fff;
    CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
    CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
    
    return CA_E_OK;
}


ca_status_t aal_l2_vlan_tx_tpid_sel_set(CA_IN ca_device_id_t device_id,
                                        CA_IN ca_uint32_t port_id,
                                        CA_IN aal_l2_vlan_port_tx_tpid_sel_mask_t mask,
                                        CA_IN aal_l2_vlan_port_tx_tpid_sel_t  *cfg)
{
    L2FE_PE_TPID_SEL_CONFIG_TBL_DATA_t tpid_sel;

    if(NULL == cfg){
        return CA_E_PARAM;
    }

    if(port_id>63){
        return CA_E_PARAM;
    }

    __L2_VLAN_LOCK();

    READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
    tpid_sel.wrd = CA_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);

    if(mask.s.cvlan){
        tpid_sel.bf.ctag = cfg->cvlan;
    }
    if(mask.s.svlan){
        tpid_sel.bf.stag = cfg->svlan;
    }
    
    CA_NE_REG_WRITE(tpid_sel.wrd,L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
    WRITE_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
    
    __L2_VLAN_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_l2_vlan_tx_tpid_sel_get(CA_IN ca_device_id_t device_id,
                                        CA_IN ca_uint32_t port_id,
                                        CA_OUT aal_l2_vlan_port_tx_tpid_sel_t  *cfg)
{
    L2FE_PE_TPID_SEL_CONFIG_TBL_DATA_t tpid_sel;

    if(NULL == cfg){
        return CA_E_PARAM;
    }

    if(port_id>63){
        return CA_E_PARAM;
    }

    __L2_VLAN_LOCK();

    READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
    tpid_sel.wrd = CA_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
    cfg->cvlan = tpid_sel.bf.ctag;
    cfg->svlan = tpid_sel.bf.stag;
    
    __L2_VLAN_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_l2_vlan_insert_tag_num_set(CA_IN ca_device_id_t device_id,
                                        CA_IN ca_uint32_t port_id,
                                        CA_IN aal_l2_vlan_insert_tag_num_t *cfg)
{
    NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG_t ni_vlan_insert;
    if(NULL == cfg){
        return CA_E_PARAM;
    }

    //TBD0518: Need to check
    if(port_id>11){
        return CA_E_PARAM;
    }

    ni_vlan_insert.wrd = CA_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
    ni_vlan_insert.wrd &= ~(3 << (port_id*2));
    ni_vlan_insert.wrd |= cfg->tag_num << (port_id*2);

    CA_NE_REG_WRITE(ni_vlan_insert.wrd,NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
    
    return CA_E_OK;
}

ca_status_t aal_l2_vlan_insert_tag_num_get(CA_IN ca_device_id_t device_id,
                                        CA_IN ca_uint32_t port_id,
                                        CA_OUT aal_l2_vlan_insert_tag_num_t *cfg)
{
    NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG_t ni_vlan_insert;
    if(NULL == cfg){
        return CA_E_PARAM;
    }

    if(port_id>11){
        return CA_E_PARAM;
    }
    
    ni_vlan_insert.wrd = CA_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
    cfg->tag_num =  (ni_vlan_insert.wrd >> (port_id*2)) & 3;

    return CA_E_OK;
}

ca_status_t aal_l2_unknown_vlan_mc_fib_bmp_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint64_t *bmp)
{
    if(bmp == NULL){
        return CA_E_PARAM;
    }
    *bmp = __g_unkown_vlan_mc_fib_bmp;
    return CA_E_OK;
}

ca_status_t aal_l2_unknown_vlan_mc_fib_add(ca_device_id_t device_id, ca_int16_t mc_fib_idx)
{
    ca_int16_t new_mcgid;
    aal_l2_vlan_default_cfg_mask_t def_cfg_msk;
    aal_l2_vlan_default_cfg_t def_cfg;
    ni_mce_indx_tbl_entry_t key;

    key.mc_vec = __g_unkown_vlan_mc_fib_bmp |= 1<<mc_fib_idx;

    new_mcgid = aal_mcgid_alloc(&key);
    if (new_mcgid == -1) {
		return CA_E_RESOURCE;
    }
    memset(&def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
    def_cfg_msk.u32 = 0;

    def_cfg_msk.s.rx_unknown_vlan_grp_id = 1;
    def_cfg.rx_unknown_vlan_grp_id = new_mcgid;
    aal_l2_vlan_default_cfg_set(device_id, def_cfg_msk, &def_cfg);

	g_vlan_mcgid_0 = new_mcgid;
	__g_unkown_vlan_mc_fib_bmp = key.mc_vec;

	return CA_E_OK;
}

ca_status_t aal_l2_unknown_vlan_mc_fib_delete(ca_device_id_t device_id, ca_int16_t mc_fib_idx)
{
    ca_int16_t new_mcgid;
    aal_l2_vlan_default_cfg_mask_t def_cfg_msk;
    aal_l2_vlan_default_cfg_t def_cfg;
    ni_mce_indx_tbl_entry_t key;

    key.mc_vec = __g_unkown_vlan_mc_fib_bmp ^= 1<<mc_fib_idx;
    new_mcgid = aal_mcgid_alloc(&key);
    if (new_mcgid == -1) {
		return CA_E_RESOURCE;
    }
    memset(&def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
    def_cfg_msk.u32 = 0;

    def_cfg_msk.s.rx_unknown_vlan_grp_id = 1;
    def_cfg.rx_unknown_vlan_grp_id = new_mcgid;
    aal_l2_vlan_default_cfg_set(device_id, def_cfg_msk, &def_cfg);

	g_vlan_mcgid_0 = new_mcgid;
	__g_unkown_vlan_mc_fib_bmp = key.mc_vec;

	return CA_E_OK;
}

ca_status_t aal_l2_vlan_debug_set(
                CA_IN ca_device_id_t device_id,
                CA_IN ca_boolean_t add_del,
                CA_IN ca_uint16_t vlan,
                CA_IN ca_uint16_t lan_port)
{
    return CA_E_OK;
}

ca_status_t aal_l2_vlan_vlan_fib_dump(ca_device_id_t device_id, aal_l2_vlan_fib_t *pcfg)
{
    if(!pcfg)
	return CA_E_PARAM;

    ca_printf("%-30s:  0x%08x \r\n","vlan_group_id",pcfg->vlan_group_id);
    aal_mc_ni_mce_indx_dump(device_id, pcfg->vlan_group_id);
    if(pcfg->memship_bm)
        ca_printf("%-30s:  0x%08x \r\n","memship_bm",pcfg->memship_bm);
    if(pcfg->untag_bm)
        ca_printf("%-30s:  0x%08x \r\n\r\n","untag_bm",pcfg->untag_bm);
    return CA_E_OK;
}


ca_status_t aal_l2_vlan_debug_get(
                CA_IN ca_device_id_t device_id,
                CA_IN ca_boolean_t add_del,
                CA_IN ca_uint16_t vlan,
                CA_OUT ca_uint16_t *lan_port)
{
    aal_l2_vlan_fib_map_t           vlan_fib_map;
    aal_l2_vlan_fib_t  		    cfg;
    ca_status_t                     ret = CA_E_OK;
    ca_uint32_t 		    i = 0, j = 0;


    for (j = 0; j < AAL_L2_VLAN_TYPE_MAX + 1; j++) {

        switch (j)
	{
	    case AAL_L2_VLAN_TYPE_WAN_SVLAN:
	        ca_printf("\r\nDev %d, WAN SVLAN: \r\n", device_id);
	        for (i = 0; i < 4095; i++) {

                    memset(&cfg,0x00,sizeof(cfg));
                    memset(&vlan_fib_map,0x00,sizeof(vlan_fib_map));
	            ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,i,&vlan_fib_map);
		    if(CA_E_OK != ret) {
		        continue;
		    }
		    if(!vlan_fib_map.valid)
		        continue;
		    ca_printf("vlan %d, fib_id %d\r\n", i, vlan_fib_map.fib_id);
	            ret = __aal_l2_vlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
		    if(CA_E_OK == ret)
		        aal_l2_vlan_vlan_fib_dump(device_id, &cfg);
		}
	        break;
	    case AAL_L2_VLAN_TYPE_WAN_CVLAN:
	        ca_printf("\r\nDev %d, WAN CVLAN: \r\n", device_id);
	        for (i = 0; i < 4095; i++) {
	            ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,i,&vlan_fib_map);
		    if(CA_E_OK != ret)
		        continue;
		    if(!vlan_fib_map.valid)
		        continue;
		    ca_printf("vlan %d, fib_id %d\r\n", i, vlan_fib_map.fib_id);
	            ret = __aal_l2_vlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
		    if(CA_E_OK == ret)
		        aal_l2_vlan_vlan_fib_dump(device_id, &cfg);
		}
	        break;
	    case AAL_L2_VLAN_TYPE_LAN_VLAN:
	        ca_printf("\r\nDev %d, LAN VLAN: \r\n", device_id);
	        for (i = 0; i < 4095; i++) {
	            ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,i,&vlan_fib_map);
		    if(CA_E_OK != ret)
		        continue;
		    if(!vlan_fib_map.valid)
		        continue;
		    ca_printf("vlan %d, fib_id %d\r\n", i, vlan_fib_map.fib_id);
	            ret = __aal_l2_vlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
		    if(CA_E_OK == ret)
		        aal_l2_vlan_vlan_fib_dump(device_id, &cfg);
		}
	        break;
	    default:
	        break;
	  }
    }
    return CA_E_OK;
}

ca_status_t aal_l2_drop_source_debug_get(
                CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    aal_l2_fe_drop_source_t  drop_source_cnt;

    memset(&drop_source_cnt,0x00,sizeof(drop_source_cnt));

    ret = aal_l2_fe_drop_source_cnt_get(device_id,&drop_source_cnt);
    if(ret != CA_E_OK){
        ca_printf("aal_l2_fe_drop_source_cnt_get failed, return = %d\n", ret );
        return ret;
    }

    if(drop_source_cnt.drop_source_cnt_00)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_00",drop_source_cnt.drop_source_cnt_00);
    if(drop_source_cnt.ipv4_cs_chk_drp_01)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "ipv4_cs_chk_drp_01",drop_source_cnt.ipv4_cs_chk_drp_01);
    if(drop_source_cnt.dpid_blackhole_drp_02)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "dpid_blackhole_drp_02",drop_source_cnt.dpid_blackhole_drp_02);
    if(drop_source_cnt.ilpb_stp_chk_drp_03)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "ilpb_stp_chk_drp_03",drop_source_cnt.ilpb_stp_chk_drp_03);
    if(drop_source_cnt.vlan_stp_chk_drp_04)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "vlan_stp_chk_drp_04",drop_source_cnt.vlan_stp_chk_drp_04);
    if(drop_source_cnt.ilpb_vlan_type_drp_05)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "ilpb_vlan_type_drp_05",drop_source_cnt.ilpb_vlan_type_drp_05);
    if(drop_source_cnt.igr_cls_pmt_stp_drp_06)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "igr_cls_pmt_stp_drp_06",drop_source_cnt.igr_cls_pmt_stp_drp_06);
    if(drop_source_cnt.vid4095_drp_07)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "vid4095_drp_07",drop_source_cnt.vid4095_drp_07);
    if(drop_source_cnt.unknown_vlan_drp_08)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "unknown_vlan_drp_08",drop_source_cnt.unknown_vlan_drp_08);
    if(drop_source_cnt.ve_deny_mc_flag_drp_09)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "ve_deny_mc_flag_drp_09",drop_source_cnt.ve_deny_mc_flag_drp_09);
    if(drop_source_cnt.l2e_da_deny_drp_10)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "l2e_da_deny_drp_10",drop_source_cnt.l2e_da_deny_drp_10);
    if(drop_source_cnt.non_std_sa_drp_11)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "non_std_sa_drp_11",drop_source_cnt.non_std_sa_drp_11);
    if(drop_source_cnt.sw_learn_err_drp_12)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "sw_learn_err_drp_12",drop_source_cnt.sw_learn_err_drp_12);
    if(drop_source_cnt.l2e_sa_deny_drp_13)
        ca_printf("%dev %d -30s:  0x%08x \r\n", device_id, "l2e_sa_deny_drp_13",drop_source_cnt.l2e_sa_deny_drp_13);
    if(drop_source_cnt.sa_mis_or_sa_limit_drp_14)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "sa_mis_or_sa_limit_drp_14",drop_source_cnt.sa_mis_or_sa_limit_drp_14);
    if(drop_source_cnt.uuc_umc_bc_drp_15)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "uuc_umc_bc_drp_15",drop_source_cnt.uuc_umc_bc_drp_15);
    if(drop_source_cnt.spid_dpid_not_in_rx_vlan_mem_drp_16)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "spid_dpid_not_in_rx_vlan_mem_drp_16",drop_source_cnt.spid_dpid_not_in_rx_vlan_mem_drp_16);
    if(drop_source_cnt.dpid_not_in_port_mem_drp_17)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "dpid_not_in_port_mem_drp_17",drop_source_cnt.dpid_not_in_port_mem_drp_17);
    if(drop_source_cnt.spid_dpid_not_in_tx_vlan_mem_drp_18)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "spid_dpid_not_in_tx_vlan_mem_drp_18",drop_source_cnt.spid_dpid_not_in_tx_vlan_mem_drp_18);
    if(drop_source_cnt.tx_vlan_stp_drp_19)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "tx_vlan_stp_drp_19",drop_source_cnt.tx_vlan_stp_drp_19);
    if(drop_source_cnt.egr_cls_deny_drp_20)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "egr_cls_deny_drp_20",drop_source_cnt.egr_cls_deny_drp_20);
    if(drop_source_cnt.loopback_drp_21)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "loopback_drp_21",drop_source_cnt.loopback_drp_21);
    if(drop_source_cnt.elpb_stp_drp_22)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "elpb_stp_drp_22",drop_source_cnt.elpb_stp_drp_22);
    if(drop_source_cnt.drop_source_cnt_23)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_23",drop_source_cnt.drop_source_cnt_23);
    if(drop_source_cnt.drop_source_cnt_24)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_24",drop_source_cnt.drop_source_cnt_24);
    if(drop_source_cnt.mcfib_vlan_drp_25)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "mcfib_vlan_drp_25",drop_source_cnt.mcfib_vlan_drp_25);
    if(drop_source_cnt.drop_source_cnt_26)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_26",drop_source_cnt.drop_source_cnt_26);
    if(drop_source_cnt.mcfib_dpid_equal_spid_drp_27)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "mcfib_dpid_equal_spid_drp_27",drop_source_cnt.mcfib_dpid_equal_spid_drp_27);
    if(drop_source_cnt.drop_source_cnt_28)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_28",drop_source_cnt.drop_source_cnt_28);
    if(drop_source_cnt.drop_source_cnt_29)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_29",drop_source_cnt.drop_source_cnt_29);
    if(drop_source_cnt.drop_source_cnt_30)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_30",drop_source_cnt.drop_source_cnt_30);
    if(drop_source_cnt.drop_source_cnt_31)
        ca_printf("dev %d %-30s:  0x%08x \r\n", device_id, "drop_source_cnt_31",drop_source_cnt.drop_source_cnt_31);

    return ret;
}

EXPORT_SYMBOL(aal_l2_unknown_vlan_mc_fib_bmp_get);
EXPORT_SYMBOL(aal_l2_fe_pp_nierr_get);
EXPORT_SYMBOL(aal_l2_ni2pp_pkt_cnt_set);
EXPORT_SYMBOL(aal_l2_vlan_fib_map_get);
EXPORT_SYMBOL(aal_l2_ni2pp_pkt_cnt_get);
EXPORT_SYMBOL(aal_l2_vlan_action_cfg_get);
EXPORT_SYMBOL(aal_l2_vlan_default_cfg_set);
EXPORT_SYMBOL(aal_l2_vlan_port_memship_map_get);
EXPORT_SYMBOL(aal_l2_vlan_action_cfg_set);
EXPORT_SYMBOL(aal_l2_vlan_fib_map_set);
EXPORT_SYMBOL(aal_l2_vlan_port_memship_map_set);
EXPORT_SYMBOL(aal_l2_fe_drop_source_cnt_get);
EXPORT_SYMBOL(aal_l2_vlan_tx_tpid_sel_set);
EXPORT_SYMBOL(aal_l2_vlan_default_cfg_get);
EXPORT_SYMBOL(aal_l2_vlan_tx_tpid_sel_get);
EXPORT_SYMBOL(aal_l2_fe_pp_heada_get);
EXPORT_SYMBOL(aal_l2_pe2tm_pkt_cnt_set);
EXPORT_SYMBOL(aal_l2_fe_pe_heada_get);
EXPORT_SYMBOL(aal_l2_pe2tm_pkt_cnt_get);
EXPORT_SYMBOL(aal_l2_vlan_insert_tag_num_set);
EXPORT_SYMBOL(aal_l2_vlan_insert_tag_num_get);
EXPORT_SYMBOL(aal_l2_vlan_debug_set);
EXPORT_SYMBOL(aal_l2_vlan_debug_get);
EXPORT_SYMBOL(aal_l2_drop_source_debug_get);
