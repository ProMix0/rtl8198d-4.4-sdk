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

#include "aal_l2_vlan.h"
#include "aal_arb.h"
#include "aal_common.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_mcast.h"
#include "ca-scfg/scfg.h"
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#include "aal_pon.h"
#endif
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#include "aal_ni_l2.h"
#endif

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
__aal_l2_wan_svlan_act_cfg_set (ca_device_id_t device_id,
                ca_uint8 fib_id,
                aal_l2_vlan_fib_mask_t mask,
                aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_WAN_SVL_FIB_DATA3_t svl_map_data3;
  L2FE_VE_WAN_SVL_FIB_DATA2_t svl_map_data2;
  L2FE_VE_WAN_SVL_FIB_DATA1_t svl_map_data1;
  L2FE_VE_WAN_SVL_FIB_DATA0_t svl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
      svl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA3));
      svl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA2));
      svl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
      svl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
      svl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA3);
      svl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA2);
      svl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA1);
      svl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA0);
#endif
  }

  if (mask.s.inner_vid_lookup_ena)
    {
        svl_map_data3.bf.wan_svl_fib_do_inner_vid_lookup =
        cfg->inner_vid_lookup_ena ? 1 : 0;
    }
  if (mask.s.dpid_field_vld)
    {
      svl_map_data2.bf.wan_svl_fib_ldpid_vld = cfg->dpid_field_vld ? 1 : 0;
    }
  if (mask.s.dpid)
    {
      svl_map_data2.bf.wan_svl_fib_ldpid = cfg->dpid ;
    }
  if (mask.s.vlan_id)
    {
      svl_map_data3.bf.wan_svl_fib_vid = (cfg->vlan_id >> 4) & 0xff;
      svl_map_data2.bf.wan_svl_fib_vid = cfg->vlan_id & 0xf;
    }
  /* if(mask.s.is_share_vlan            ){
     svl_map_data2.bf.wan_svl_fib_si_ind              = cfg->is_share_vlan ? 1:0          ;
     } */
  if (mask.s.is_svlan)
    {
      svl_map_data2.bf.wan_svl_fib_sc_ind = cfg->is_svlan ? 1 : 0;
    }
  if (mask.s.top_vlan_cmd)
    {
      svl_map_data2.bf.wan_svl_fib_top_vlan_cmd = cfg->top_vlan_cmd;
    }
  if (mask.s.top_vlan_id_sel)
    {
      svl_map_data2.bf.wan_svl_fib_topvid_usel2 = cfg->top_vlan_id_sel;
    }
  if (mask.s.top_vlan_cmd_field_vld)
    {
      svl_map_data2.bf.wan_svl_fib_top_cmd_valid =
    cfg->top_vlan_cmd_field_vld ? 1 : 0;
    }
  if (mask.s.inner_vlan_cmd)
    {
      svl_map_data2.bf.wan_svl_fib_inner_vlan_cmd = cfg->inner_vlan_cmd;
    }
  if (mask.s.inner_vlan_id_sel)
    {
      svl_map_data2.bf.wan_svl_fib_innervid_usel2 = cfg->inner_vlan_id_sel;
    }
  if (mask.s.inner_vlan_cmd_field_vld)
    {
      svl_map_data2.bf.wan_svl_fib_inner_cmd_valid =
    cfg->inner_vlan_cmd_field_vld ? 1 : 0;
    }
   if(mask.s.stp_stat                 ){
     svl_map_data2.bf.wan_svl_fib_stp_mode            = cfg->stp_stat                ;
     }
  if (mask.s.is_mc_vlan)
    {
      svl_map_data2.bf.wan_svl_fib_mc_vlan_ind = cfg->is_mc_vlan ? 1 : 0;
    }
  // if(mask.s.key_is_share_vlan        ){
  //      svl_map_data2.bf.wan_svl_fib_key_si_ind          = cfg->key_is_share_vlan ? 1:0      ;
  //  }
  if (mask.s.dot1p)
    {
      svl_map_data2.bf.wan_svl_fib_802_1p = cfg->dot1p;
    }
  if (mask.s.dscp)
    {
      svl_map_data2.bf.wan_svl_fib_dscp = cfg->dscp ;
      //svl_map_data1.bf.wan_svl_fib_dscp = cfg->dscp & 0x07;
    }
  if (mask.s.cos)
    {
      svl_map_data1.bf.wan_svl_fib_cos = cfg->cos;
    }
  if (mask.s.dot1p_field_vld)
    {
      svl_map_data1.bf.wan_svl_fib_802_1p_valid =
    cfg->dot1p_field_vld ? 1 : 0;
    }
  if (mask.s.dscp_field_vld)
    {
      svl_map_data1.bf.wan_svl_fib_dscp_valid = cfg->dscp_field_vld ? 1 : 0;
    }
  if (mask.s.cos_field_vld)
    {
      svl_map_data1.bf.wan_svl_fib_cos_valid = cfg->cos_field_vld ? 1 : 0;
    }
  if (mask.s.flow_id)
    {
      svl_map_data1.bf.wan_svl_fib_flowid = cfg->flow_id;
    }
  if (mask.s.flow_field_vld)
    {
      svl_map_data1.bf.wan_svl_fib_flowid_valid = cfg->flow_field_vld ? 1 : 0;
    }
  if (mask.s.permit)
    {
      svl_map_data1.bf.wan_svl_fib_permit = cfg->permit ? 1 : 0;
    }
  if (mask.s.da_sa_swap_ena)
    {
      svl_map_data1.bf.wan_svl_fib_da_sa_swap = cfg->da_sa_swap_ena ? 1 : 0;
    }
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
      CA_NE_REG_WRITE (svl_map_data3.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA3));
      CA_NE_REG_WRITE (svl_map_data2.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA2));
      CA_NE_REG_WRITE (svl_map_data1.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
      CA_NE_REG_WRITE (svl_map_data0.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));
      WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_NE_REG_WRITE (svl_map_data3.wrd, L2FE_VE_WAN_SVL_FIB_DATA3);
      CA_8279_NE_REG_WRITE (svl_map_data2.wrd, L2FE_VE_WAN_SVL_FIB_DATA2);
      CA_8279_NE_REG_WRITE (svl_map_data1.wrd, L2FE_VE_WAN_SVL_FIB_DATA1);
      CA_8279_NE_REG_WRITE (svl_map_data0.wrd, L2FE_VE_WAN_SVL_FIB_DATA0);
      CA_8279_WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
#endif
  }

  return CA_E_OK;
}

static ca_status_t
__aal_l2_wan_svlan_act_cfg_get (ca_device_id_t device_id,
                ca_uint8 fib_id, aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_WAN_SVL_FIB_DATA3_t svl_map_data3;
  L2FE_VE_WAN_SVL_FIB_DATA2_t svl_map_data2;
  L2FE_VE_WAN_SVL_FIB_DATA1_t svl_map_data1;
  L2FE_VE_WAN_SVL_FIB_DATA0_t svl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
      svl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA3));
      svl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA2));
      svl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA1));
      svl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_SVL_FIB_ACCESS);
      svl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA3);
      svl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA2);
      svl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA1);
      svl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_FIB_DATA0);
#endif
  }

  cfg->inner_vid_lookup_ena =
    svl_map_data3.bf.wan_svl_fib_do_inner_vid_lookup;
  cfg->dpid_field_vld = svl_map_data2.bf.wan_svl_fib_ldpid_vld;
  cfg->dpid = svl_map_data2.bf.wan_svl_fib_ldpid;
  cfg->vlan_id =
    (svl_map_data3.bf.wan_svl_fib_vid << 4) | (svl_map_data2.bf.
                        wan_svl_fib_vid);
  /*cfg->is_share_vlan             = svl_map_data2.bf.wan_svl_fib_si_ind                 ; */
  cfg->is_svlan = svl_map_data2.bf.wan_svl_fib_sc_ind;
  cfg->top_vlan_cmd = svl_map_data2.bf.wan_svl_fib_top_vlan_cmd;
  cfg->top_vlan_id_sel = svl_map_data2.bf.wan_svl_fib_topvid_usel2;
  cfg->top_vlan_cmd_field_vld = svl_map_data2.bf.wan_svl_fib_top_cmd_valid;
  cfg->inner_vlan_cmd = svl_map_data2.bf.wan_svl_fib_inner_vlan_cmd;
  cfg->inner_vlan_id_sel = svl_map_data2.bf.wan_svl_fib_innervid_usel2;
  cfg->inner_vlan_cmd_field_vld =
    svl_map_data2.bf.wan_svl_fib_inner_cmd_valid;
  cfg->stp_stat                  = svl_map_data2.bf.wan_svl_fib_stp_mode               ;
  cfg->is_mc_vlan = svl_map_data2.bf.wan_svl_fib_mc_vlan_ind;
  //cfg->key_is_share_vlan         = svl_map_data2.bf.wan_svl_fib_key_si_ind             ;
  cfg->dot1p = svl_map_data2.bf.wan_svl_fib_802_1p;
  cfg->dscp =
    (svl_map_data2.bf.wan_svl_fib_dscp << 3);
  cfg->cos = svl_map_data1.bf.wan_svl_fib_cos;
  cfg->dot1p_field_vld = svl_map_data1.bf.wan_svl_fib_802_1p_valid;
  cfg->dscp_field_vld = svl_map_data1.bf.wan_svl_fib_dscp_valid;
  cfg->cos_field_vld = svl_map_data1.bf.wan_svl_fib_cos_valid;
  cfg->flow_id = svl_map_data1.bf.wan_svl_fib_flowid;
  cfg->flow_field_vld = svl_map_data1.bf.wan_svl_fib_flowid_valid;
  cfg->permit = svl_map_data1.bf.wan_svl_fib_permit;
  cfg->da_sa_swap_ena = svl_map_data1.bf.wan_svl_fib_da_sa_swap;
  cfg->vlan_group_id = svl_map_data1.bf.wan_svl_fib_vl_group_id;
  cfg->memship_bm = svl_map_data0.bf.wan_svl_fib_membership_bm;
  cfg->untag_bm = svl_map_data0.bf.wan_svl_fib_egress_top_untag_bm;

  return CA_E_OK;
}

static ca_status_t
__aal_l2_wan_cvlan_act_cfg_set (ca_device_id_t device_id,
                ca_uint8 fib_id,
                aal_l2_vlan_fib_mask_t mask,
                aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_WAN_CVL_FIB_DATA3_t cvl_map_data3;
  L2FE_VE_WAN_CVL_FIB_DATA2_t cvl_map_data2;
  L2FE_VE_WAN_CVL_FIB_DATA1_t cvl_map_data1;
  L2FE_VE_WAN_CVL_FIB_DATA0_t cvl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
      cvl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA3));
      cvl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA2));
      cvl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA1));
      cvl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
      cvl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA3);
      cvl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA2);
      cvl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA1);
      cvl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA0);
#endif
  }

  if (mask.s.inner_vid_lookup_ena)
    {
      cvl_map_data3.bf.wan_cvl_fib_reserved = cfg->inner_vid_lookup_ena;
    }
  if (mask.s.dpid_field_vld)
    {
      cvl_map_data2.bf.wan_cvl_fib_ldpid_vld = cfg->dpid_field_vld ? 1 : 0;
    }
  if (mask.s.dpid)
    {
      cvl_map_data2.bf.wan_cvl_fib_ldpid = cfg->dpid;
    }
  if (mask.s.vlan_id)
    {
      cvl_map_data3.bf.wan_cvl_fib_vid = (cfg->vlan_id >> 4) & 0xff;
      cvl_map_data2.bf.wan_cvl_fib_vid = cfg->vlan_id & 0xf;
    }
  /* if(mask.s.is_share_vlan            ){
     cvl_map_data2.bf.wan_cvl_fib_si_ind              = cfg->is_share_vlan  ? 1:0         ;
     } */
  if (mask.s.is_svlan)
    {
      cvl_map_data2.bf.wan_cvl_fib_sc_ind = cfg->is_svlan ? 1 : 0;
    }
  if (mask.s.top_vlan_cmd)
    {
      cvl_map_data2.bf.wan_cvl_fib_top_vlan_cmd = cfg->top_vlan_cmd;
    }
  if (mask.s.top_vlan_id_sel)
    {
      cvl_map_data2.bf.wan_cvl_fib_topvid_usel2 = cfg->top_vlan_id_sel;
    }
  if (mask.s.top_vlan_cmd_field_vld)
    {
      cvl_map_data2.bf.wan_cvl_fib_top_cmd_valid =
    cfg->top_vlan_cmd_field_vld ? 1 : 0;
    }
  if (mask.s.inner_vlan_cmd)
    {
      cvl_map_data2.bf.wan_cvl_fib_inner_vlan_cmd = cfg->inner_vlan_cmd;
    }
  if (mask.s.inner_vlan_id_sel)
    {
      cvl_map_data2.bf.wan_cvl_fib_innervid_usel2 = cfg->inner_vlan_id_sel;
    }
  if (mask.s.inner_vlan_cmd_field_vld)
    {
      cvl_map_data2.bf.wan_cvl_fib_inner_cmd_valid =
    cfg->inner_vlan_cmd_field_vld ? 1 : 0;
    }
   if(mask.s.stp_stat                 ){
     cvl_map_data2.bf.wan_cvl_fib_stp_mode            = cfg->stp_stat                ;
     }
  if (mask.s.is_mc_vlan)
    {
      cvl_map_data2.bf.wan_cvl_fib_mc_vlan_ind = cfg->is_mc_vlan ? 1 : 0;
    }
  // if(mask.s.key_is_share_vlan        ){
  //     cvl_map_data2.bf.wan_cvl_fib_key_si_ind          = cfg->key_is_share_vlan  ? 1:0     ;
  //  }
  if (mask.s.dot1p)
    {
      cvl_map_data2.bf.wan_cvl_fib_802_1p = cfg->dot1p;
    }
  if (mask.s.dscp)
    {
      cvl_map_data2.bf.wan_cvl_fib_dscp = cfg->dscp;
    }
  if (mask.s.cos)
    {
      cvl_map_data1.bf.wan_cvl_fib_cos = cfg->cos;
    }
  if (mask.s.dot1p_field_vld)
    {
      cvl_map_data1.bf.wan_cvl_fib_802_1p_valid =
    cfg->dot1p_field_vld ? 1 : 0;
    }
  if (mask.s.dscp_field_vld)
    {
      cvl_map_data1.bf.wan_cvl_fib_dscp_valid = cfg->dscp_field_vld ? 1 : 0;
    }
  if (mask.s.cos_field_vld)
    {
      cvl_map_data1.bf.wan_cvl_fib_cos_valid = cfg->cos_field_vld ? 1 : 0;
    }
  if (mask.s.flow_id)
    {
      cvl_map_data1.bf.wan_cvl_fib_flowid = cfg->flow_id;
    }
  if (mask.s.flow_field_vld)
    {
      cvl_map_data1.bf.wan_cvl_fib_flowid_valid = cfg->flow_field_vld ? 1 : 0;
    }
  if (mask.s.permit)
    {
      cvl_map_data1.bf.wan_cvl_fib_permit = cfg->permit ? 1 : 0;
    }
  if (mask.s.da_sa_swap_ena)
    {
      cvl_map_data1.bf.wan_cvl_fib_da_sa_swap = cfg->da_sa_swap_ena ? 1 : 0;
    }
  if (mask.s.vlan_group_id)
    {
      cvl_map_data1.bf.wan_cvl_fib_vl_group_id = cfg->vlan_group_id;
    }
  if (mask.s.memship_bm)
    {
      cvl_map_data0.bf.wan_cvl_fib_membership_bm = cfg->memship_bm;
    }
  if (mask.s.untag_bm)
    {
      cvl_map_data0.bf.wan_cvl_fib_egress_top_untag_bm = cfg->untag_bm;
    }

  if(0 == device_id){
      CA_NE_REG_WRITE (cvl_map_data3.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA3));
      CA_NE_REG_WRITE (cvl_map_data2.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA2));
      CA_NE_REG_WRITE (cvl_map_data1.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA1));
      CA_NE_REG_WRITE (cvl_map_data0.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA0));
      WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_NE_REG_WRITE (cvl_map_data3.wrd, L2FE_VE_WAN_CVL_FIB_DATA3);
      CA_8279_NE_REG_WRITE (cvl_map_data2.wrd, L2FE_VE_WAN_CVL_FIB_DATA2);
      CA_8279_NE_REG_WRITE (cvl_map_data1.wrd, L2FE_VE_WAN_CVL_FIB_DATA1);
      CA_8279_NE_REG_WRITE (cvl_map_data0.wrd, L2FE_VE_WAN_CVL_FIB_DATA0);
      CA_8279_WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
#endif
  }
  return CA_E_OK;
}

static ca_status_t
__aal_l2_wan_cvlan_act_cfg_get (ca_device_id_t device_id,
                ca_uint8 fib_id, aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_WAN_CVL_FIB_DATA3_t cvl_map_data3;
  L2FE_VE_WAN_CVL_FIB_DATA2_t cvl_map_data2;
  L2FE_VE_WAN_CVL_FIB_DATA1_t cvl_map_data1;
  L2FE_VE_WAN_CVL_FIB_DATA0_t cvl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
      cvl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA3));
      cvl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA2));
      cvl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA1));
      cvl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_WAN_CVL_FIB_ACCESS);
      cvl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA3);
      cvl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA2);
      cvl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA1);
      cvl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_FIB_DATA0);
#endif
  }

  cfg->inner_vid_lookup_ena = cvl_map_data3.bf.wan_cvl_fib_reserved;
  cfg->dpid_field_vld = cvl_map_data2.bf.wan_cvl_fib_ldpid_vld;
  cfg->dpid = cvl_map_data2.bf.wan_cvl_fib_ldpid;
  cfg->vlan_id =
    (cvl_map_data3.bf.wan_cvl_fib_vid << 4) | (cvl_map_data2.bf.
                        wan_cvl_fib_vid);
  /* cfg->is_share_vlan             =  cvl_map_data2.bf.wan_cvl_fib_si_ind                 ;         */
  cfg->is_svlan = cvl_map_data2.bf.wan_cvl_fib_sc_ind;
  cfg->top_vlan_cmd = cvl_map_data2.bf.wan_cvl_fib_top_vlan_cmd;
  cfg->top_vlan_id_sel = cvl_map_data2.bf.wan_cvl_fib_topvid_usel2;
  cfg->top_vlan_cmd_field_vld = cvl_map_data2.bf.wan_cvl_fib_top_cmd_valid;
  cfg->inner_vlan_cmd = cvl_map_data2.bf.wan_cvl_fib_inner_vlan_cmd;
  cfg->inner_vlan_id_sel = cvl_map_data2.bf.wan_cvl_fib_innervid_usel2;
  cfg->inner_vlan_cmd_field_vld =
    cvl_map_data2.bf.wan_cvl_fib_inner_cmd_valid;
  cfg->stp_stat                  =  cvl_map_data2.bf.wan_cvl_fib_stp_mode               ;
  cfg->is_mc_vlan = cvl_map_data2.bf.wan_cvl_fib_mc_vlan_ind;
  //cfg->key_is_share_vlan         =  cvl_map_data2.bf.wan_cvl_fib_key_si_ind             ;
  cfg->dot1p = cvl_map_data2.bf.wan_cvl_fib_802_1p;
  cfg->dscp =
    (cvl_map_data2.bf.wan_cvl_fib_dscp);
  cfg->cos = cvl_map_data1.bf.wan_cvl_fib_cos;
  cfg->dot1p_field_vld = cvl_map_data1.bf.wan_cvl_fib_802_1p_valid;
  cfg->dscp_field_vld = cvl_map_data1.bf.wan_cvl_fib_dscp_valid;
  cfg->cos_field_vld = cvl_map_data1.bf.wan_cvl_fib_cos_valid;
  cfg->flow_id = cvl_map_data1.bf.wan_cvl_fib_flowid;
  cfg->flow_field_vld = cvl_map_data1.bf.wan_cvl_fib_flowid_valid;
  cfg->permit = cvl_map_data1.bf.wan_cvl_fib_permit;
  cfg->da_sa_swap_ena = cvl_map_data1.bf.wan_cvl_fib_da_sa_swap;
  cfg->vlan_group_id = cvl_map_data1.bf.wan_cvl_fib_vl_group_id;
  cfg->memship_bm = cvl_map_data0.bf.wan_cvl_fib_membership_bm;
  cfg->untag_bm = cvl_map_data0.bf.wan_cvl_fib_egress_top_untag_bm;
  return CA_E_OK;
}

static ca_status_t
__aal_l2_lan_vlan_act_cfg_set (ca_device_id_t device_id,
                   ca_uint8 fib_id,
                   aal_l2_vlan_fib_mask_t mask,
                   aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_LAN_VL_FIB_DATA3_t vl_map_data3;
  L2FE_VE_LAN_VL_FIB_DATA2_t vl_map_data2;
  L2FE_VE_LAN_VL_FIB_DATA1_t vl_map_data1;
  L2FE_VE_LAN_VL_FIB_DATA0_t vl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
      vl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA3));
      vl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA2));
      vl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA1));
      vl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
      vl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA3);
      vl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA2);
      vl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA1);
      vl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA0);
#endif
  }

  if (mask.s.inner_vid_lookup_ena)
    {
        vl_map_data3.bf.lan_vl_fib_reserved = cfg->inner_vid_lookup_ena    ;
    }
  if (mask.s.dpid_field_vld)
    {
      vl_map_data2.bf.lan_vl_fib_ldpid_vld = cfg->dpid_field_vld ? 1 : 0;
    }
  if (mask.s.dpid)
    {
      vl_map_data2.bf.lan_vl_fib_ldpid = cfg->dpid;
    }
  if (mask.s.vlan_id)
    {
      vl_map_data3.bf.lan_vl_fib_vid = (cfg->vlan_id >> 6) & 0x3f;
      vl_map_data2.bf.lan_vl_fib_vid = cfg->vlan_id & 0x3f;
    }
  /* if(mask.s.is_share_vlan            ){
     vl_map_data2.bf.lan_vl_fib_si_ind              = cfg->is_share_vlan ? 1:0          ;
     }     */
  if (mask.s.is_svlan)
    {
      vl_map_data2.bf.lan_vl_fib_sc_ind = cfg->is_svlan ? 1 : 0;
    }
  if (mask.s.top_vlan_cmd)
    {
      vl_map_data2.bf.lan_vl_fib_top_vlan_cmd = cfg->top_vlan_cmd;
    }
  if (mask.s.top_vlan_id_sel)
    {
      // vl_map_data2.bf.lan_vl_fib_topvid_usel2        = cfg->top_vlan_id_sel         ;
    }
  if (mask.s.top_vlan_cmd_field_vld)
    {
      vl_map_data2.bf.lan_vl_fib_top_cmd_valid =
    cfg->top_vlan_cmd_field_vld ? 1 : 0;
    }
  if (mask.s.inner_vlan_cmd)
    {
      vl_map_data2.bf.lan_vl_fib_inner_vlan_cmd = cfg->inner_vlan_cmd;
    }
  /*
     if(mask.s.inner_vlan_id_sel        ){
     vl_map_data2.bf.lan_vl_fib_innervid_usel2      = cfg->inner_vlan_id_sel       ;
     }
   */
  if (mask.s.inner_vlan_cmd_field_vld)
    {
      vl_map_data2.bf.lan_vl_fib_inner_cmd_valid =
    cfg->inner_vlan_cmd_field_vld ? 1 : 0;
    }
    if(mask.s.stp_stat                 ){
      vl_map_data2.bf.lan_vl_fib_stp_mode            = cfg->stp_stat                ;
    }
  if (mask.s.is_mc_vlan)
    {
      vl_map_data2.bf.lan_vl_fib_mc_vlan_ind = cfg->is_mc_vlan ? 1 : 0;
    }
  //if(mask.s.key_is_share_vlan        ){
  //    vl_map_data2.bf.lan_vl_fib_key_si_ind          = cfg->key_is_share_vlan  ? 1:0     ;
  // }
  if (mask.s.dot1p)
    {
      vl_map_data2.bf.lan_vl_fib_802_1p = cfg->dot1p;
    }
  if (mask.s.dscp)
    {
      vl_map_data2.bf.lan_vl_fib_dscp = cfg->dscp ;
    }
  if (mask.s.cos)
    {
      vl_map_data1.bf.lan_vl_fib_cos = cfg->cos;
    }
  if (mask.s.dot1p_field_vld)
    {
      vl_map_data1.bf.lan_vl_fib_802_1p_valid = cfg->dot1p_field_vld ? 1 : 0;
    }
  if (mask.s.dscp_field_vld)
    {
      vl_map_data1.bf.lan_vl_fib_dscp_valid = cfg->dscp_field_vld ? 1 : 0;
    }
  if (mask.s.cos_field_vld)
    {
      vl_map_data1.bf.lan_vl_fib_cos_valid = cfg->cos_field_vld ? 1 : 0;
    }
  if (mask.s.flow_id)
    {
      vl_map_data1.bf.lan_vl_fib_flowid = cfg->flow_id;
    }
  if (mask.s.flow_field_vld)
    {
      vl_map_data1.bf.lan_vl_fib_flowid_valid = cfg->flow_field_vld ? 1 : 0;
    }
  if (mask.s.permit)
    {
      vl_map_data1.bf.lan_vl_fib_permit = cfg->permit ? 1 : 0;
    }
  if (mask.s.da_sa_swap_ena)
    {
      vl_map_data1.bf.lan_vl_fib_da_sa_swap = cfg->da_sa_swap_ena ? 1 : 0;
    }
  if (mask.s.vlan_group_id)
    {
      vl_map_data1.bf.lan_vl_fib_vl_group_id = cfg->vlan_group_id & 0x3ff;
    }
  if (mask.s.memship_bm)
    {
      vl_map_data0.bf.lan_vl_fib_membership_bm = cfg->memship_bm;
    }
  if (mask.s.untag_bm)
    {
      vl_map_data0.bf.lan_vl_fib_egress_top_untag_bm = cfg->untag_bm;
    }

  if(0 == device_id){
      CA_NE_REG_WRITE (vl_map_data3.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA3));
      CA_NE_REG_WRITE (vl_map_data2.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA2));
      CA_NE_REG_WRITE (vl_map_data1.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA1));
      CA_NE_REG_WRITE (vl_map_data0.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA0));
      WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_NE_REG_WRITE (vl_map_data3.wrd, L2FE_VE_LAN_VL_FIB_DATA3);
      CA_8279_NE_REG_WRITE (vl_map_data2.wrd, L2FE_VE_LAN_VL_FIB_DATA2);
      CA_8279_NE_REG_WRITE (vl_map_data1.wrd, L2FE_VE_LAN_VL_FIB_DATA1);
      CA_8279_NE_REG_WRITE (vl_map_data0.wrd, L2FE_VE_LAN_VL_FIB_DATA0);
      CA_8279_WRITE_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
#endif
  }

  return CA_E_OK;

}

static ca_status_t
__aal_l2_lan_vlan_act_cfg_get (ca_device_id_t device_id,
                   ca_uint8 fib_id, aal_l2_vlan_fib_t * cfg)
{
  L2FE_VE_LAN_VL_FIB_DATA3_t vl_map_data3;
  L2FE_VE_LAN_VL_FIB_DATA2_t vl_map_data2;
  L2FE_VE_LAN_VL_FIB_DATA1_t vl_map_data1;
  L2FE_VE_LAN_VL_FIB_DATA0_t vl_map_data0;

  if(0 == device_id){
      READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
      vl_map_data3.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA3));
      vl_map_data2.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA2));
      vl_map_data1.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA1));
      vl_map_data0.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_FIB_DATA0));
  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_READ_INDIRCT_REG (device_id, fib_id, L2FE_VE_LAN_VL_FIB_ACCESS);
      vl_map_data3.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA3);
      vl_map_data2.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA2);
      vl_map_data1.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA1);
      vl_map_data0.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_FIB_DATA0);
#endif
  }
  //cfg->inner_vid_lookup_ena      =  vl_map_data3.bf.lan_vl_fib_do_inner_vid_lookup    ;
  cfg->dpid_field_vld = vl_map_data2.bf.lan_vl_fib_ldpid_vld;
  cfg->dpid = vl_map_data2.bf.lan_vl_fib_ldpid;
  cfg->vlan_id =
    (vl_map_data3.bf.lan_vl_fib_vid << 6) | (vl_map_data2.bf.lan_vl_fib_vid);
  /*cfg->is_share_vlan             =  vl_map_data2.bf.lan_vl_fib_si_ind                 ;     */
  cfg->is_svlan = vl_map_data2.bf.lan_vl_fib_sc_ind;
  cfg->top_vlan_cmd = vl_map_data2.bf.lan_vl_fib_top_vlan_cmd;
  //cfg->top_vlan_id_sel           =  vl_map_data2.bf.lan_vl_fib_topvid_usel2           ;
  cfg->top_vlan_cmd_field_vld = vl_map_data2.bf.lan_vl_fib_top_cmd_valid;
  cfg->inner_vlan_cmd = vl_map_data2.bf.lan_vl_fib_inner_vlan_cmd;
  //cfg->inner_vlan_id_sel         =  vl_map_data2.bf.lan_vl_fib_innervid_usel2         ;
  cfg->inner_vlan_cmd_field_vld = vl_map_data2.bf.lan_vl_fib_inner_cmd_valid;
  cfg->stp_stat                  =  vl_map_data2.bf.lan_vl_fib_stp_mode               ;
  cfg->is_mc_vlan = vl_map_data2.bf.lan_vl_fib_mc_vlan_ind;
  //cfg->key_is_share_vlan         =  vl_map_data2.bf.lan_vl_fib_key_si_ind             ;
  cfg->dot1p = vl_map_data2.bf.lan_vl_fib_802_1p;
  cfg->dscp =
    (vl_map_data2.bf.lan_vl_fib_dscp);
  cfg->cos = vl_map_data1.bf.lan_vl_fib_cos;
  cfg->dot1p_field_vld = vl_map_data1.bf.lan_vl_fib_802_1p_valid;
  cfg->dscp_field_vld = vl_map_data1.bf.lan_vl_fib_dscp_valid;
  cfg->cos_field_vld = vl_map_data1.bf.lan_vl_fib_cos_valid;
  cfg->flow_id = vl_map_data1.bf.lan_vl_fib_flowid;
  cfg->flow_field_vld = vl_map_data1.bf.lan_vl_fib_flowid_valid;
  cfg->permit = vl_map_data1.bf.lan_vl_fib_permit;
  cfg->da_sa_swap_ena = vl_map_data1.bf.lan_vl_fib_da_sa_swap;
  cfg->vlan_group_id = vl_map_data1.bf.lan_vl_fib_vl_group_id;
  cfg->memship_bm = vl_map_data0.bf.lan_vl_fib_membership_bm;
  cfg->untag_bm = vl_map_data0.bf.lan_vl_fib_egress_top_untag_bm;

  return CA_E_OK;
}

ca_status_t
aal_l2_vlan_action_cfg_set (CA_IN ca_device_id_t device_id,
                CA_IN aal_l2_vlan_type_t vlan_type,
                CA_IN ca_uint8 fib_id,
                CA_IN aal_l2_vlan_fib_mask_t mask,
                CA_IN aal_l2_vlan_fib_t * cfg)
{
  ca_status_t ret = CA_E_OK;

  if (vlan_type > AAL_L2_VLAN_TYPE_MAX)
    {
      return CA_E_PARAM;
    }
  if (fib_id > __AAL_L2_VLAN_MAX_FIB_INDEX)
    {
      return CA_E_PARAM;
    }
  if (NULL == cfg)
    {
      return CA_E_PARAM;
    }
  __L2_VLAN_LOCK();
  switch (vlan_type)
    {
    case AAL_L2_VLAN_TYPE_WAN_SVLAN:
      ret = __aal_l2_wan_svlan_act_cfg_set (device_id, fib_id, mask, cfg);
      break;
    case AAL_L2_VLAN_TYPE_WAN_CVLAN:
      ret = __aal_l2_wan_cvlan_act_cfg_set (device_id, fib_id, mask, cfg);
      break;
    case AAL_L2_VLAN_TYPE_LAN_VLAN:
      ret = __aal_l2_lan_vlan_act_cfg_set (device_id, fib_id, mask, cfg);
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
aal_l2_vlan_action_cfg_get (CA_IN ca_device_id_t device_id,
                CA_IN aal_l2_vlan_type_t vlan_type,
                CA_IN ca_uint8 fib_id,
                CA_OUT aal_l2_vlan_fib_t * cfg)
{
  ca_status_t ret = CA_E_OK;

  if (vlan_type > AAL_L2_VLAN_TYPE_MAX)
    {
      return CA_E_PARAM;
    }
  if (fib_id > __AAL_L2_VLAN_MAX_FIB_INDEX)
    {
      return CA_E_PARAM;
    }
  if (NULL == cfg)
    {
      return CA_E_PARAM;
    }

  __L2_VLAN_LOCK();
  switch (vlan_type)
    {
    case AAL_L2_VLAN_TYPE_WAN_SVLAN:
      ret = __aal_l2_wan_svlan_act_cfg_get (device_id, fib_id, cfg);
      break;
    case AAL_L2_VLAN_TYPE_WAN_CVLAN:
      ret = __aal_l2_wan_cvlan_act_cfg_get (device_id, fib_id, cfg);
      break;
    case AAL_L2_VLAN_TYPE_LAN_VLAN:
      ret = __aal_l2_lan_vlan_act_cfg_get (device_id, fib_id, cfg);
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
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_IDX_DATA);
#endif
      }

      if (mask.s.fib_id)
    {
      vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
    }
      if (mask.s.valid)
    {
      vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
    }
      if(0 == device_id){
          CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
          WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_NE_REG_WRITE (vl_index.wrd, L2FE_VE_WAN_SVL_IDX_DATA);
          CA_8279_WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
#endif
      }
      break;
    case AAL_L2_VLAN_TYPE_WAN_CVLAN:
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_IDX_DATA);
#endif
      }

      if (mask.s.fib_id)
    {
      vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
    }
      if (mask.s.valid)
    {
      vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
    }
      if(0 == device_id){
          CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
          WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_NE_REG_WRITE (vl_index.wrd, L2FE_VE_WAN_CVL_IDX_DATA);
          CA_8279_WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
#endif
      }
      break;
    case AAL_L2_VLAN_TYPE_LAN_VLAN:
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_IDX_DATA);
#endif
      }

      if (mask.s.fib_id)
    {
      vl_index.bf.wan_svl_idx_fib_idx = cfg->fib_id;
    }
      if (mask.s.valid)
    {
      vl_index.bf.wan_svl_idx_entry_vld = cfg->valid;
    }

      if(0 == device_id){
          CA_NE_REG_WRITE (vl_index.wrd, AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
          WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_NE_REG_WRITE (vl_index.wrd, L2FE_VE_LAN_VL_IDX_DATA);
          CA_8279_WRITE_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
#endif
      }
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
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_SVL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_SVL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_SVL_IDX_DATA);
#endif
      }
      break;
    case AAL_L2_VLAN_TYPE_WAN_CVLAN:
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (WAN_CVL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_WAN_CVL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_WAN_CVL_IDX_DATA);
#endif
      }
      break;
    case AAL_L2_VLAN_TYPE_LAN_VLAN:
      if(0 == device_id){
          READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
          vl_index.wrd = CA_NE_REG_READ (AAL_L2FE_VE_REG_ADDR (LAN_VL_IDX_DATA));
      }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
          CA_8279_READ_INDIRCT_REG (device_id, vlan_id, L2FE_VE_LAN_VL_IDX_ACCESS);
          vl_index.wrd = CA_8279_NE_REG_READ (L2FE_VE_LAN_VL_IDX_DATA);
#endif
      }
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

  if(0 == device_id){

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
  }else if(1 == device_id){

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      if (mask.s.vlan_mode)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLC_VLAN_MODE_CTRL, l2e_glb_vlan_mode,
                   cfg->vlan_mode);
        }

      if (mask.s.cmp_tpid_svlan)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_S, tpid, cfg->cmp_tpid_svlan);
        }

      if (mask.s.cmp_tpid_cvlan)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_C, tpid, cfg->cmp_tpid_cvlan);
        }

      if (mask.s.cmp_tpid_other)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_other);
        }
      if (mask.s.tx_tpid0)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_A, tpid, cfg->tx_tpid0);
        }
      if (mask.s.tx_tpid1)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_B, tpid, cfg->tx_tpid1);
        }

      if (mask.s.tx_tpid2)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_C, tpid, cfg->tx_tpid2);
        }

      if (mask.s.tx_tpid3)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PE_CFG_VLAN_TPID_D, tpid, cfg->tx_tpid3);
        }

      if (mask.s.wan_memship_check_ena)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_wan,
                   cfg->wan_memship_check_ena);
        }
      if (mask.s.lan_memship_check_ena)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_lan,
                   cfg->lan_memship_check_ena);
        }
      if (mask.s.rx_unknown_vlan_memship_check_bmp)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_RX_UNKWN_VLAN_DFT, memshp_bm,
                   cfg->rx_unknown_vlan_memship_check_bmp);
        }
      if (mask.s.rx_unknown_vlan_grp_id)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_RX_UNKWN_VLAN_DFT, vl_grp_id,
                   cfg->rx_unknown_vlan_grp_id);
        }
      if (mask.s.tx_unknown_vlan_memship_check_bmp)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, memshp_bm,
                   cfg->tx_unknown_vlan_memship_check_bmp);
        }
      if (mask.s.tx_unknown_vlan_untag_bmp)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, untag_bm,
                   cfg->tx_unknown_vlan_untag_bmp);
        }
      if (mask.s.tx_unknown_vlan_stp_mode_forward_ena)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_TX_UNKWN_VLAN_DFT_1, stp_mode,
                   cfg->tx_unknown_vlan_stp_mode_forward_ena);
        }
      if(mask.s.unknown_vlan_fwd_ena)
        {
            CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLC_DPID_FWD_CTRL, unkvlan_fwd,
                 cfg->unknown_vlan_fwd_ena);
        }
      if(mask.s.vlan_4095_fwd_ena)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLC_DPID_FWD_CTRL, vid_fff_fwd,
               cfg->vlan_4095_fwd_ena);
        }
    if(mask.s.loopback_packet_fwd_ena)
        {
          CA_8279_REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,
               cfg->loopback_packet_fwd_ena);
        }
#endif
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

  if(0 == device_id){
      REG_L2FE_FIELD_READ (L2FE_PLC_VLAN_MODE_CTRL, l2e_glb_vlan_mode,
              cfg->vlan_mode);
      REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_S, tpid, cfg->cmp_tpid_svlan);
      REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_C, tpid, cfg->cmp_tpid_cvlan);
      REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_other);
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

  }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLC_VLAN_MODE_CTRL, l2e_glb_vlan_mode,
              cfg->vlan_mode);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_S, tpid, cfg->cmp_tpid_svlan);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_C, tpid, cfg->cmp_tpid_cvlan);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PP_TPID_CMP_O, tpid, cfg->cmp_tpid_other);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_A, tpid, cfg->tx_tpid0);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_B, tpid, cfg->tx_tpid1);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_C, tpid, cfg->tx_tpid2);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PE_CFG_VLAN_TPID_D, tpid, cfg->tx_tpid3);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_wan,
              cfg->wan_memship_check_ena);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, txvlan_membshp_chk_en_lan,
              cfg->lan_memship_check_ena);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_RX_UNKWN_VLAN_DFT, memshp_bm,
              cfg->rx_unknown_vlan_memship_check_bmp);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_RX_UNKWN_VLAN_DFT, vl_grp_id,
              cfg->rx_unknown_vlan_grp_id);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, memshp_bm,
              cfg->tx_unknown_vlan_memship_check_bmp);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_2, untag_bm,
              cfg->tx_unknown_vlan_untag_bmp);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_TX_UNKWN_VLAN_DFT_1, stp_mode,
              cfg->tx_unknown_vlan_stp_mode_forward_ena);

      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLC_DPID_FWD_CTRL, unkvlan_fwd,
           cfg->unknown_vlan_fwd_ena);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLC_DPID_FWD_CTRL, vid_fff_fwd,
           cfg->vlan_4095_fwd_ena);
      CA_8279_REG_L2FE_FIELD_READ (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,
           cfg->loopback_packet_fwd_ena);
#endif
  }
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

    if(0 == device_id){
        CA_NE_REG_WRITE(mapped_port, L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
        WRITE_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(mapped_port, L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
        CA_8279_WRITE_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
#endif
    }
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

    if(0 == device_id){
        READ_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
        *mapped_port = CA_NE_REG_READ(L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
    }else if( 1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_READ_INDIRCT_REG (device_id, logic_port, L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS);
        *mapped_port = CA_8279_NE_REG_READ(L2FE_PLE_MMSHP_CHK_ID_MAP_DATA);
#endif
    }
    __L2_VLAN_UNLOCK();

    return CA_E_OK;
}


void aal_l2_vlan_init(ca_device_id_t device_id)
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
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint8_t  pon_mode = 0;
    aal_arb_mc_fib_mask_t     mc_fib_mask;
    aal_arb_mc_fib_t          mc_fib_cfg;
    ca_status_t ret = CA_E_OK;
#endif
    ni_mce_indx_tbl_entry_t key;

    aal_l2_vlan_port_tx_tpid_sel_mask_t             tx_tpid_sel_mask;
    aal_l2_vlan_port_tx_tpid_sel_t                  tx_tpid_sel;

    memset(&l2_vlan_fib, 0, sizeof(aal_l2_vlan_fib_t));

    l2_vlan_fib_mask.u32 = 0;
    l2_vlan_fib_mask.s.stp_stat = 1;
    l2_vlan_fib.stp_stat = AAL_L2_VLAN_STP_STAT_FORWARD_AND_LEARN;

    if(CA_OSAL_OK != ca_spin_lock_init(&g_l2_vlan_lock, SPINLOCK_CMN)){
        ca_printf("%s: init spinlock fail\n", __func__);
        return;
    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
          printk("<%s, %d> read CFG_ID_PON_MAC_MODE fail\r\n", __func__, __LINE__);
    }
#endif
    for(fib_id = 0; fib_id<128; fib_id++){

        aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_WAN_SVLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
        aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_WAN_CVLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
        aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_LAN_VLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_l2_vlan_action_cfg_set(1, AAL_L2_VLAN_TYPE_WAN_SVLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
        aal_l2_vlan_action_cfg_set(1, AAL_L2_VLAN_TYPE_WAN_CVLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
        aal_l2_vlan_action_cfg_set(1, AAL_L2_VLAN_TYPE_LAN_VLAN, fib_id, l2_vlan_fib_mask, &l2_vlan_fib);
#endif
    }
    //__g_l2_vlan_port_map[AAL_LPORT_L3_WAN] = __g_l2_vlan_port_map[AAL_LPORT_ETH_WAN];
    for(lpid = 0; lpid<__AAL_L2_VLAN_LOGIC_PORT_NUM; lpid++){
        aal_l2_vlan_port_memship_map_set(device_id, lpid, __g_l2_vlan_port_map[lpid]);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_l2_vlan_port_memship_map_set(1, lpid, __g_l2_vlan_port_map[lpid]);
#endif
        tx_tpid_sel_mask.u32 = 0;
        tx_tpid_sel_mask.s.svlan = 1;
        tx_tpid_sel.svlan = AAL_L2_VLAN_TX_TPID_SEL_OLD_CSR_A;
        tx_tpid_sel_mask.s.cvlan = 1;
        tx_tpid_sel.cvlan = AAL_L2_VLAN_TX_TPID_SEL_OLD_CSR_A;
        aal_l2_vlan_tx_tpid_sel_set(device_id, lpid, tx_tpid_sel_mask, &tx_tpid_sel);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_l2_vlan_tx_tpid_sel_set(1, lpid, tx_tpid_sel_mask, &tx_tpid_sel);
#endif
    }
    insert_tag_num.tag_num = 2;
    for(lpid = 0; lpid<12; lpid++){
        aal_l2_vlan_insert_tag_num_set(device_id, lpid, &insert_tag_num);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        aal_l2_vlan_insert_tag_num_set(1, lpid, &insert_tag_num);
#endif
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
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
            if((aal_pon_mac_mode_get(device_id) >= ONU_PON_MAC_EPON_1G) &&
                (aal_pon_mac_mode_get(device_id) <= ONU_PON_MAC_EPON_BI10G)&&
                (g_vlan_flooding_domain_1[i] == 7)){
                repl_key.ldpid = 0x20;
            }else
#endif
            {
                repl_key.ldpid = g_vlan_flooding_domain_1[i];
            }
#ifdef  MC_FIB_COS_CMD
            repl_key.cos = 1;
            repl_key.cos_cmd = 1;
#endif

#if defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
            repl_key.keep_o_lspid = 1;
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
		return;
    }

    def_cfg.rx_unknown_vlan_grp_id = g_vlan_mcgid_0;

    aal_l2_vlan_default_cfg_set(device_id, def_cfg_msk, &def_cfg);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    def_cfg.unknown_vlan_fwd_ena  = TRUE;
    def_cfg.tx_unknown_vlan_memship_check_bmp |=((1 << __g_l2_vlan_port_map[6]) | (1 << __g_l2_vlan_port_map[7]));
    def_cfg.rx_unknown_vlan_memship_check_bmp |=((1 << __g_l2_vlan_port_map[6]) | (1 << __g_l2_vlan_port_map[7]));
    aal_l2_vlan_default_cfg_set(1, def_cfg_msk, &def_cfg);
	/*
	 * In G3HGU PON module, set the default flooding member to port#6
	 */
    aal_ni_mc_lkup_set(1, 0, (ca_uint64_t)1);

    mc_fib_mask.u32 = 0;
    mc_fib_mask.s.ldpid = 1;
    memset(&mc_fib_cfg, 0, sizeof(aal_arb_mc_fib_t));
    mc_fib_cfg.ldpid = 6;
    aal_arb_mc_fib_set(1,0,mc_fib_mask,&mc_fib_cfg);
#endif

    l2_vlan_fib_mask.u32 = 0;
    l2_vlan_fib_mask.s.vlan_group_id = 1;
    l2_vlan_fib.vlan_group_id = g_vlan_mcgid_1;

    aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_WAN_SVLAN, 127, l2_vlan_fib_mask, &l2_vlan_fib);
    aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_WAN_CVLAN, 127, l2_vlan_fib_mask, &l2_vlan_fib);
    aal_l2_vlan_action_cfg_set(device_id, AAL_L2_VLAN_TYPE_LAN_VLAN, 127, l2_vlan_fib_mask, &l2_vlan_fib);
    REG_L2FE_FIELD_WRITE(L2FE_PP_DEBUG_BUS, ni2fe_pkt_cnt_wrap_en, 1);
    REG_L2FE_FIELD_WRITE(L2FE_PE_DEBUG, fe2tm_pkt_cnt_wrap_en , 1);
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    REG_L2FE_FIELD_WRITE(L2FE_PLE_CTL_REG, pon_mode, 1);
    REG_L2FE_FIELD_WRITE(L3FE_GLB_GLB_CFG, l3fe_pon_mode, 1);
    REG_L2FE_FIELD_WRITE (L2FE_PLE_CTL_REG, skip_port_lpbk_chk,1);
    REG_L2FE_FIELD_WRITE (L2FE_PE_CTRL, enhance_tpid_mode,1);
    ca_printf("init SFU SATURN L2FE,L3FE done.\n");

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    CA_8279_REG_L2FE_FIELD_WRITE(L2FE_PLE_CTL_REG, pon_mode, 1);
    CA_8279_REG_L2FE_FIELD_WRITE(L3FE_GLB_GLB_CFG, l3fe_pon_mode, 1);
    CA_8279_REG_L2FE_FIELD_WRITE(L2FE_PE_CTRL, enhance_tpid_mode, 1);
    ca_printf("init HGU SATURN L2FE,L3FE done.\n");
#endif
}

ca_status_t aal_l2_fe_pp_heada_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_heada_t *heada)
{
    if(heada == NULL){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        heada->heada_hi         = CA_NE_REG_READ(L2FE_PP_HEADER_A_HI);
        heada->heada_mid        = CA_NE_REG_READ(L2FE_PP_HEADER_A_MID);
        heada->heada_low        = CA_NE_REG_READ(L2FE_PP_HEADER_A_LOW);
#if defined(CONFIG_LUNA_G3_SERIES)	
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
#else
        heada->cpu_flag         = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>31)&1;
        heada->deep_q           = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>30)&1;
        heada->policy_ena       = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>16)&3;
        heada->policy_group_id  = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>27)&7;
        heada->policy_id        = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>18)&0x1ff;
        heada->marked           = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>15)&1;
        heada->mirror           = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>14)&1;
        heada->no_drop          = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>13)&1;
        heada->rx_packet_type   = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>11)&3;
        heada->drop_code        = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>8)&7;
        heada->mc_group_id      = (CA_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>0)&0xff;
        heada->header_type      = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>30)&3;
        heada->fe_bypass        = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>29)&1;
        heada->packet_size      = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>15)&0x3fff;
        heada->cos              = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID) >> 0)&0x7;
        heada->logic_spid       = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>9) &0x3f;
        heada->logic_dpid       = (CA_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>3)&0x3f;
#endif
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        heada->heada_hi         = CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI);
        heada->heada_mid        = CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID);
        heada->heada_low        = CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_LOW);
        heada->cpu_flag         = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>31)&1;
        heada->deep_q           = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>30)&1;
        heada->policy_ena       = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>16)&3;
        heada->policy_group_id  = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>27)&7;
        heada->policy_id        = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>18)&0x1ff;
        heada->marked           = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>15)&1;
        heada->mirror           = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>14)&1;
        heada->no_drop          = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>13)&1;
        heada->rx_packet_type   = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>11)&3;
        heada->drop_code        = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>8)&7;
        heada->mc_group_id      = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_HI)>>0)&0xff;
        heada->header_type      = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>30)&3;
        heada->fe_bypass        = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>29)&1;
        heada->packet_size      = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>15)&0x3fff;
        heada->cos              = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID) >> 0)&0x7;
        heada->logic_spid       = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>9) &0x3f;
        heada->logic_dpid       = (CA_8279_NE_REG_READ(L2FE_PP_HEADER_A_MID)>>3)&0x3f;
#endif
    }
    return CA_E_OK;
}


ca_status_t aal_l2_fe_pp_nierr_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_ni_status_t *err)
{
    if(err == NULL){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        err->sop_eop_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_0);
        err->runt_l2_packet     = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_1);
        err->sob_eob_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_2);
        err->sop_eop_gap        = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_3);
        err->pp_pingpong_overflow = CA_NE_REG_READ(L2FE_PP_PINGPONG_STS)&1;
        err->ready              = CA_NE_REG_READ(L2FE_PP_NI_INTF_STS)&1;
        err->drop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT)&0xffff;
        err->sop_cnt            = (CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)>>16)&0xffff;
        err->eop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)&0xffff;
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        err->sop_eop_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_0);
        err->runt_l2_packet     = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_1);
        err->sob_eob_mismatch   = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_2);
        err->sop_eop_gap        = CA_NE_REG_READ(L2FE_PP_NI_INTF_ERR_STTS_3);
        err->pp_pingpong_overflow = CA_NE_REG_READ(L2FE_PP_PINGPONG_STS)&1;
        err->ready              = CA_NE_REG_READ(L2FE_PP_NI_INTF_STS)&1;
        err->drop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT)&0xffff;
        err->sop_cnt            = (CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)>>16)&0xffff;
        err->eop_cnt           = CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT)&0xffff;
#endif
    }
    return CA_E_OK;
}

ca_status_t aal_l2_fe_pe_heada_get(CA_IN  ca_device_id_t device_id,
                                   CA_OUT aal_l2_fe_heada_t *heada)
{
    if(heada == NULL){
        return CA_E_PARAM;
    }
    if(0 == device_id){
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
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        heada->heada_hi         = CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI);
        heada->heada_mid        = CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID);
        heada->heada_low        = CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_LOW);
        heada->cpu_flag         = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>31)&1;
        heada->deep_q           = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>30)&1;
        heada->policy_ena       = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>16)&3;
        heada->policy_group_id  = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>27)&7;
        heada->policy_id        = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>18)&0x1ff;
        heada->marked           = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>15)&1;
        heada->mirror           = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>14)&1;
        heada->no_drop          = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>13)&1;
        heada->rx_packet_type   = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>11)&3;
        heada->drop_code        = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>8)&7;
        heada->mc_group_id      = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_HI)>>0)&0xff;
        heada->header_type      = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>30)&3;
        heada->fe_bypass        = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>29)&1;
        heada->packet_size      = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>15)&0x3fff;
        heada->cos              = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID) >> 0)&0x7;
        heada->logic_spid       = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>9) &0x3f;
        heada->logic_dpid       = (CA_8279_NE_REG_READ(L2FE_PE_HEADER_A_MID)>>3)&0x3f;
#endif
    }
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

    if(0 == device_id){
        for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
            READ_INDIRCT_REG (device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
            *p =  CA_NE_REG_READ(L2FE_PE_DROP_STTS_DATA);
            p++;
        }
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
            CA_8279_READ_INDIRCT_REG (device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
            *p =  CA_8279_NE_REG_READ(L2FE_PE_DROP_STTS_DATA);
            p++;
        }
#endif
    }
    if(0 == device_id){
        for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
            CA_NE_REG_WRITE(0,L2FE_PE_DROP_STTS_DATA);
            WRITE_INDIRCT_REG (device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
        }
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    for(drop_source_id = 0; drop_source_id<32; drop_source_id++){
            CA_8279_NE_REG_WRITE(0,L2FE_PE_DROP_STTS_DATA);
            CA_8279_WRITE_INDIRCT_REG(device_id, drop_source_id, L2FE_PE_DROP_STTS_ACCESS);
        }
#endif
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
    if(0 == device_id){
        CA_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
#endif
    }
    return CA_E_OK;
}


ca_status_t aal_l2_pe2tm_pkt_cnt_get(CA_IN  ca_device_id_t device_id,
                                      CA_OUT ca_pe2tm_cnt_t *cnt)
{
    ca_uint32_t temp_value = 0;

    if(NULL == cnt){
        return CA_E_PARAM;
    }
    if(0 == device_id){
        temp_value = CA_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        temp_value = CA_8279_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
#endif
    }
    cnt->tm2fe_ready = (temp_value >>31) &1;
    cnt->sop_cnt     = (temp_value>>16) &0x7fff;
    cnt->eop_cnt     = (temp_value>>0) &0x7fff;
    if(0 == device_id){
        CA_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(0,L2FE_PE_TM_INTERFACE_PKT_CNT);
#endif
    }
    return CA_E_OK;
}



ca_status_t aal_l2_ni2pp_pkt_cnt_set(CA_IN  ca_device_id_t device_id,
                                      CA_IN ca_ni2pp_cnt_t *cnt)
{

    if(NULL == cnt){
        return CA_E_PARAM;
    }

    //temp_value = CA_NE_REG_READ(L2FE_PE_TM_INTERFACE_PKT_CNT);
    if(0 == device_id){
        CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
        CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
        CA_8279_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
#endif
    }
    return CA_E_OK;
}


ca_status_t aal_l2_ni2pp_pkt_cnt_get(CA_IN  ca_device_id_t device_id,
                                      CA_OUT ca_ni2pp_cnt_t *cnt)
{
    ca_uint32_t temp_value = 0;

    if(NULL == cnt){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        cnt->drp_cnt = CA_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT);
        temp_value = CA_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        cnt->drp_cnt = CA_8279_NE_REG_READ(L2FE_PP_NI_INTF_DROP_CNT);
        temp_value = CA_8279_NE_REG_READ(L2FE_PP_NI_INTF_PKT_CNT);
#endif
    }

    cnt->sop_cnt     = (temp_value>>16) &0x7fff;
    cnt->eop_cnt     = (temp_value>>0) &0x7fff;
    if(0 == device_id){
        CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
        CA_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(0,L2FE_PP_NI_INTF_DROP_CNT);
        CA_8279_NE_REG_WRITE(0,L2FE_PP_NI_INTF_PKT_CNT);
#endif
    }
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

    if(0 == device_id){
        READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
        tpid_sel.wrd = CA_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
        tpid_sel.wrd = CA_8279_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
#endif
    }

    if(mask.s.cvlan){
        tpid_sel.bf.ctag = cfg->cvlan;
    }
    if(mask.s.svlan){
        tpid_sel.bf.stag = cfg->svlan;
    }
#if defined CONFIG_ARCH_CORTINA_G3LITE || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if(mask.s.inner_cvlan){
        tpid_sel.bf.inner_ctag = cfg->inner_cvlan;
    }
    if(mask.s.inner_svlan){
        tpid_sel.bf.inner_stag = cfg->inner_svlan;
    }
#endif
    if(0 == device_id){
        CA_NE_REG_WRITE(tpid_sel.wrd,L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
        WRITE_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_NE_REG_WRITE(tpid_sel.wrd,L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
#endif
    }
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

    if(0 == device_id){
        READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
        tpid_sel.wrd = CA_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        CA_8279_READ_INDIRCT_REG(device_id,port_id, L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS);
        tpid_sel.wrd = CA_8279_NE_REG_READ(L2FE_PE_TPID_SEL_CONFIG_TBL_DATA);
#endif
    }
    cfg->cvlan = tpid_sel.bf.ctag;
    cfg->svlan = tpid_sel.bf.stag;
#if defined CONFIG_ARCH_CORTINA_G3LITE || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    cfg->inner_cvlan = tpid_sel.bf.inner_ctag;
    cfg->inner_svlan = tpid_sel.bf.inner_stag;
#endif
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

    if(port_id>11){
        return CA_E_PARAM;
    }

    if( 0 == device_id){
        ni_vlan_insert.wrd = CA_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
        ni_vlan_insert.wrd &= ~(3 << (port_id*2));
        ni_vlan_insert.wrd |= cfg->tag_num << (port_id*2);

        CA_NE_REG_WRITE(ni_vlan_insert.wrd,NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        ni_vlan_insert.wrd = CA_8279_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
        ni_vlan_insert.wrd &= ~(3 << (port_id*2));
        ni_vlan_insert.wrd |= cfg->tag_num << (port_id*2);

        CA_8279_NE_REG_WRITE(ni_vlan_insert.wrd,NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
#endif
    }
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
    if(0 == device_id){
        ni_vlan_insert.wrd = CA_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
    }else if(1 == device_id){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        ni_vlan_insert.wrd = CA_8279_NE_REG_READ(NI_HV_GLB_RX_SEG_VLAN_TAG_INSERT_CFG);
#endif
    }
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


#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
ca_status_t __xgpon_setup_vlan_traffic(
    ca_boolean_t is_add,
    ca_uint16_t vlan,
    ca_uint16_t lan_port)
{
    aal_l2_vlan_fib_mask_t     fib_mask;
    aal_l2_vlan_fib_t          fib;
    aal_l2_vlan_fib_map_mask_t map_mask;
    aal_l2_vlan_fib_map_t      map;
    ca_status_t                ret = CA_E_OK;
    ca_uint8_t                 fib_id = lan_port + 100;

    fib_mask.u32 = 0;
    map_mask.u32 = 0;

    memset(&fib, 0, sizeof(fib));
    memset(&map, 0, sizeof(map));


    fib_mask.s.vlan_id = 1;
    fib_mask.s.permit = 1;
    fib_mask.s.stp_stat = 1;
    fib_mask.s.dpid_field_vld = 1;
    fib_mask.s.dpid = 1;
    fib_mask.s.memship_bm = 1;
    fib_mask.s.is_svlan = 1;

    fib.vlan_id = is_add ? vlan : 0;
    fib.permit = 1;
    fib.stp_stat = 1;
    fib.dpid_field_vld = 1;
    fib.dpid = 4; /* The port 4 is used to connect Saturn to G3 */
    fib.memship_bm = 0xFF;
    fib.is_svlan = 1;

    ret = aal_l2_vlan_action_cfg_set (0,
                AAL_L2_VLAN_TYPE_LAN_VLAN,
                fib_id,
                fib_mask,
                &fib);

    map_mask.s.fib_id = 1;
    map_mask.s.valid  = 1;

    map.fib_id = fib_id;
    map.valid  = is_add;

    if (CA_E_OK == ret) {

        ret = aal_l2_vlan_fib_map_set(0,
                AAL_L2_VLAN_TYPE_LAN_VLAN,
                vlan,
                map_mask,
                &map);
    }

    if (CA_E_OK != ret) {
        ca_printf("setup upstream fail, return %d\r\n", ret);
        return ret;
    }

    fib_id = lan_port;
    fib.dpid = lan_port;

    ret = aal_l2_vlan_action_cfg_set (0,
                AAL_L2_VLAN_TYPE_WAN_SVLAN,
                fib_id,
                fib_mask,
                &fib);

    if (CA_E_OK == ret) {

        ret = aal_l2_vlan_fib_map_set(0,
                AAL_L2_VLAN_TYPE_WAN_SVLAN,
                vlan,
                map_mask,
                &map);
    }

    if (CA_E_OK != ret) {
        ca_printf("setup downstream fail, return %d\r\n", ret);
        return ret;
    }

    ca_printf("setup vlan %u lanport %u OK, UPS use fib %u, DNS use fib %u.\n",
        vlan, lan_port, lan_port + 100, lan_port);
    return ret;

}
#endif

ca_status_t aal_l2_vlan_debug_set(
                CA_IN ca_device_id_t device_id,
                CA_IN ca_boolean_t add_del,
                CA_IN ca_uint16_t vlan,
                CA_IN ca_uint16_t lan_port)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    __xgpon_setup_vlan_traffic(add_del, vlan, lan_port);
#endif
    return CA_E_OK;
}

ca_status_t aal_l2_vlan_vlan_fib_dump(ca_device_id_t device_id, aal_l2_vlan_fib_t *pcfg)
{
    if(!pcfg)
	return CA_E_PARAM;

    if(pcfg->inner_vid_lookup_ena)
        ca_printf("%-30s:  0x%08x \r\n","inner_vid_lookup_ena",pcfg->inner_vid_lookup_ena);
    if(pcfg->dpid_field_vld) {
        ca_printf("%-30s:  0x%08x \r\n","dpid_field_vld",pcfg->dpid_field_vld);
        ca_printf("%-30s:  0x%08x \r\n","dpid",pcfg->dpid);
    }
    if(pcfg->is_share_vlan)
        ca_printf("%-30s:  0x%08x \r\n","is_share_vlan",pcfg->is_share_vlan);
    if(pcfg->is_svlan)
        ca_printf("%-30s:  0x%08x \r\n","is_svlan",pcfg->is_svlan);
    if(pcfg->top_vlan_cmd_field_vld) {
        ca_printf("%-30s:  0x%08x \r\n","vlan_id",pcfg->vlan_id);
        ca_printf("%-30s:  0x%08x \r\n","top_vlan_cmd",pcfg->top_vlan_cmd);
        ca_printf("%-30s:  0x%08x \r\n","top_vlan_id_sel",pcfg->top_vlan_id_sel);
    }
    if(pcfg->inner_vlan_cmd_field_vld) {
        ca_printf("%-30s:  0x%08x \r\n","vlan_id",pcfg->vlan_id);
        ca_printf("%-30s:  0x%08x \r\n","inner_vlan_cmd",pcfg->inner_vlan_cmd);
        ca_printf("%-30s:  0x%08x \r\n","inner_vlan_id_sel",pcfg->inner_vlan_id_sel);
    }
    ca_printf("%-30s:  0x%08x \r\n","stp_stat",pcfg->stp_stat);
    if(pcfg->is_mc_vlan)
        ca_printf("%-30s:  0x%08x \r\n","is_mc_vlan",pcfg->is_mc_vlan);
    if(pcfg->key_is_share_vlan)
        ca_printf("%-30s:  0x%08x \r\n","key_is_share_vlan",pcfg->key_is_share_vlan);
    if(pcfg->dot1p_field_vld)
        ca_printf("%-30s:  0x%08x \r\n","dot1p",pcfg->dot1p);
    if(pcfg->dscp_field_vld)
        ca_printf("%-30s:  0x%08x \r\n","dscp",pcfg->dscp);
    if(pcfg->cos_field_vld)
        ca_printf("%-30s:  0x%08x \r\n","cos",pcfg->cos);
    if(pcfg->flow_field_vld)
        ca_printf("%-30s:  0x%08x \r\n","flow_id",pcfg->flow_id);
    ca_printf("%-30s:  0x%08x \r\n","permit",pcfg->permit);
    if(pcfg->da_sa_swap_ena)
        ca_printf("%-30s:  0x%08x \r\n","da_sa_swap_ena",pcfg->da_sa_swap_ena);
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
	            ret = __aal_l2_wan_svlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
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
	            ret = __aal_l2_wan_cvlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
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
	            ret = __aal_l2_lan_vlan_act_cfg_get (device_id, vlan_fib_map.fib_id, &cfg);
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
