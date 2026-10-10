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
 * aal_hashlite_config.c: Initial Configuration of HashLite Engine profile
 *
 */

#include "aal_common.h"
#include "aal_hashlite_init.h"
#include "aal_port.h"

/***************************************************************************
 * Hash Action Group and related Hash Action
 ***************************************************************************/

static hl_act_l3_generic_t ACT_KEEP_ORIGINAL_PKT = {
	.keep_orig_pkt = 1,	// PPPoE control packets need this to remain unmodified when leaving L3FE PE
	.keep_orig_pkt_vld = 1,
};

static hl_act_l3_generic_t ACT_DROP = {
	.dpid_vld = 1,
	.permit = 0,	// drop the packet
	.mc = 0,
	.mcgid = AAL_LPORT_BLACKHOLE,
};

static hl_act_l3_generic_t ACT_DEFAULT_TO_CPU = {
	.permit = 1,	// forward the packet
	.mcgid = AAL_LPORT_CPU_0,
	.mc = 0,	// mcgid is LDPID
	.dpid_pri = 1,
	.dpid_vld = 1,
	.keep_orig_pkt = 1,
	.keep_orig_pkt_vld = 1,
	.stage3_ctrl = 0,	// apply t2 result to CPU port according to ingress interface
	.stage3_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t5_ctrl_vld = 1,
};

static void dump_HL_ACTGRP_L3_GENERIC(hl_act_l3_generic_t *act)
{
	printk(KERN_INFO "<Group 8, L3 Egress IF>\n");
	printk(KERN_INFO " ip_ttl_vld: %d, ip_ttl_zero_discard_en: %d, ip_ttl_cmd(2): %d, ip_ttl(8): 0x%02x\n",
		act->ip_ttl_vld, act->ip_ttl_zero_discard_en, act->ip_ttl_cmd, act->ip_ttl);
	printk(KERN_INFO " 6rd_ipda_ctrl_vld: %d, 6rd_ipda_from_v6: %d\n", act->six_rd_ipda_ctrl_vld, act->six_rd_ipda_from_v6);
	printk(KERN_INFO " ip_mtu_enc_vld: %d, ip_mtu_enc(2): %d\n", act->ip_mtu_enc_vld, act->ip_mtu_enc);
	printk(KERN_INFO " l2_format_vld: %d, l2_format: %d, l3_egress_if_vld: %d\n",
		act->l2_format_vld, act->l2_format, act->l3_egress_if_vld);

	printk(KERN_INFO "<Group 11, MAC DA>\n");
	printk(KERN_INFO " mac_da_vld: %d, mac_da: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_da_vld, act->mac_da_5, act->mac_da_4, act->mac_da_3, act->mac_da_2, act->mac_da_1, act->mac_da_0);

	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 14, MDATA Word 2>\n");
	printk(KERN_INFO " mdata_w_vld_2(4): 0x%x, mdata_w_2(16): 0x%04x\n", act->mdata_w_vld_2, act->mdata_w_2);

	printk(KERN_INFO "<Group 15, MDATA Word 3>\n");
	printk(KERN_INFO " mdata_w_vld_3(4): 0x%x, mdata_w_3(16): 0x%04x\n", act->mdata_w_vld_3, act->mdata_w_3);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);
}

static void dump_HL_ACTGRP_L3COS(hl_act_l3cos_t *act)
{
	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);
}

static void dump_HL_ACTGRP_HGU_DS(hl_act_hgu_ds_t *act)
{
	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d\n", act->vlan_vld, act->vlan_cnt);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);
}

static void dump_HL_ACTGRP_SFU_US(hl_act_sfu_us_t *act)
{
	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);
}

static void dump_HL_ACTGRP_SFU_DS_LLID(hl_act_sfu_ds_llid_t *act)
{
	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d\n", act->vlan_vld, act->vlan_cnt);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_dscp_update_en(2): 0x%x, dscp_table_sel: %d, ip_dscp_sel(3): 0x%x, ip_dscp(6): 0x%02x\n",
			act->ip_dscp_update_en, act->dscp_table_sel, act->ip_dscp_sel, act->ip_dscp);
	printk(KERN_INFO " qos_802_1p_table_sel: %d\n", act->qos_802_1p_table_sel);
	printk(KERN_INFO " top_802_1p_sel(3): 0x%x, top_802_1p(3): 0x%x\n", act->top_802_1p_sel, act->top_802_1p);
	printk(KERN_INFO " inner_802_1p_sel(3): 0x%x, inner_802_1p(3): 0x%x\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " cos_table_sel: %d\n", act->cos_table_sel);
	printk(KERN_INFO " cos_sel(2): 0x%x, cos(3): 0x%x\n", act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);
}

int hashlite_action_dump(unsigned int act_grp_mask, aal_hashlite_hash_action_entry_t *act_entry)
{
	if (act_entry == NULL) return AAL_E_NULLPTR;

	switch (act_grp_mask) {
	case HL_ACTGRP_L3_GENERIC:
		dump_HL_ACTGRP_L3_GENERIC((hl_act_l3_generic_t *) act_entry);
		break;

	case HL_ACTGRP_L3COS:
		dump_HL_ACTGRP_L3COS((hl_act_l3cos_t *) act_entry);
		break;

	case HL_ACTGRP_HGU_DS:
		dump_HL_ACTGRP_HGU_DS((hl_act_hgu_ds_t *) act_entry);
		break;

	case HL_ACTGRP_SFU_US:
		dump_HL_ACTGRP_SFU_US((hl_act_sfu_us_t *) act_entry);
		break;

	case HL_ACTGRP_SFU_DS_LLID:
		dump_HL_ACTGRP_SFU_DS_LLID((hl_act_sfu_ds_llid_t *) act_entry);
		break;

	default:
		printk(KERN_INFO "Unknown action group mask 0x%08x\n", (unsigned int) act_grp_mask);
		break;
	}

	return AAL_E_OK;
}

/***************************************************************************
 * Hash Mask
 ***************************************************************************/

aal_hash_mask_t HM_L3_NEIGHBOR;		/* Also NextHop. Hash key & action come from ARP/ICMPv6_ND. */

aal_hash_mask_t HM_HGU_US_INTF;		/* HGU upstream traffic of matched egress VID; key is egress IF_ID (mdata) */
aal_hash_mask_t HM_HGU_US_8021P;	/* HGU upstream traffic of matched egress 802.1p; key is top_802.1p and vlan_cnt */
aal_hash_mask_t HM_HGU_US_INTF_8021P;	/* HGU upstream traffic of matched egress VID and 802.1p; key is egress IF_ID (mdata) and top_802.1p */
aal_hash_mask_t HM_HGU_DS;		/* HGU downstream traffic */

aal_hash_mask_t HM_SFU_US;		/* SFU upstream traffic */
aal_hash_mask_t HM_SFU_DS_MC;		/* SFU downstream LLID based MC */
aal_hash_mask_t HM_SFU_DS_MC_ETHTYPE;	/* SFU downstream LLID based MC for specific packet type */
aal_hash_mask_t HM_SFU_DS_MC_VLAN;	/* SFU downstream LLID based MC for specific VID */
aal_hash_mask_t HM_SFU_DS_MC_IGMPV3;	/* SFU downstream LLID + IP based MC with src filter */
aal_hash_mask_t HM_SFU_DS_MC_MACDA;	/* SFU downstream LLID + MAC DA */

aal_hash_mask_t HM_SFU_DS_LLID;			/* SFU downstream LLID */
aal_hash_mask_t HM_SFU_DS_LLID_OUTVID_8021P;	/* SFU downstream LLID + outer VID + outer 802.1P */
aal_hash_mask_t HM_SFU_DS_LLID_VLAN_0_TAG;	/* SFU downstream LLID + vlan count */
aal_hash_mask_t HM_SFU_DS_LLID_VLAN_1_TAG;	/* SFU downstream LLID + vlan count + outer VID */
aal_hash_mask_t HM_SFU_DS_LLID_VLAN_2_TAG;	/* SFU downstream LLID + vlan count + outer VID + inner VID */
aal_hash_mask_t HM_SFU_DS_LLID_OUT8021P;	/* SFU downstream LLID + outer 802.1P */
aal_hash_mask_t HM_SFU_DS_LLID_ETHTYPE;		/* SFU downstream LLID + ether type */
aal_hash_mask_t HM_SFU_DS_LLID_MACDARNG;	/* SFU downstream LLID + MAC DA range */

int hashlite_hashmask_init(void)
{
	/*** NOTE: Put all hashmasks here and initialize them ***/

	hash_mask_unmask(&HM_L3_NEIGHBOR);
	HM_L3_NEIGHBOR.hkey_id = 0;
	HM_L3_NEIGHBOR.ip_ver = 0;
	HM_L3_NEIGHBOR.ip_vld = 0;
	HM_L3_NEIGHBOR.ip_da = 128;		// Prefix 128 bits. For IPv4, the prefix 96 bits of IPDA hashkey must be 0.
	HM_L3_NEIGHBOR.mdata = ~0xfff;		// MDATA[11:0] is LPM rst_idx

	hash_mask_unmask(&HM_HGU_US_INTF);
	HM_HGU_US_INTF.mdata = ~0xfff;		// MDATA[11:0] is LPM rst_idx

	hash_mask_unmask(&HM_HGU_US_8021P);
	HM_HGU_US_8021P.vlan_cnt = 0;
	HM_HGU_US_8021P.top_8021p = 0;

	hash_mask_unmask(&HM_HGU_US_INTF_8021P);
	HM_HGU_US_INTF_8021P.mdata = ~0xfff;	// MDATA[11:0] is LPM rst_idx
	HM_HGU_US_INTF_8021P.top_8021p = 0;

	hash_mask_unmask(&HM_HGU_DS);
	HM_HGU_DS.lspid = 0;

	hash_mask_unmask(&HM_SFU_US);
	HM_SFU_US.lspid = 0;
	HM_SFU_US.ip_dscp = 0x07;		// only higher 3 bits

	hash_mask_unmask(&HM_SFU_DS_MC);
	HM_SFU_DS_MC.hkey_id= 0;
	HM_SFU_DS_MC.pol_id = 0;		// LLID

	hash_mask_unmask(&HM_SFU_DS_MC_ETHTYPE);
	HM_SFU_DS_MC_ETHTYPE.hkey_id= 0;
	HM_SFU_DS_MC_ETHTYPE.pol_id = 0;	// LLID
	HM_SFU_DS_MC_ETHTYPE.ethertype = 0;

	hash_mask_unmask(&HM_SFU_DS_MC_VLAN);
	HM_SFU_DS_MC_VLAN.hkey_id= 0;
	HM_SFU_DS_MC_VLAN.pol_id = 0;	// LLID
	HM_SFU_DS_MC_VLAN.vlan_cnt = 0;
	HM_SFU_DS_MC_VLAN.top_vid = 0;

	hash_mask_unmask(&HM_SFU_DS_MC_IGMPV3);
	HM_SFU_DS_MC_IGMPV3.hkey_id= 0;
	HM_SFU_DS_MC_IGMPV3.pol_id = 0;		// LLID
	HM_SFU_DS_MC_IGMPV3.mac_da = 0;
	HM_SFU_DS_MC_IGMPV3.vlan_cnt = 0;
	HM_SFU_DS_MC_IGMPV3.top_vid = 0;
	HM_SFU_DS_MC_IGMPV3.ip_da = 128;
	HM_SFU_DS_MC_IGMPV3.ip_sa = 128;

	hash_mask_unmask(&HM_SFU_DS_MC_MACDA);
	HM_SFU_DS_MC_MACDA.hkey_id = 0;
	HM_SFU_DS_MC_MACDA.pol_id = 0;		// LLID
	HM_SFU_DS_MC_MACDA.mac_da = 0x0;

	hash_mask_unmask(&HM_SFU_DS_LLID);
	HM_SFU_DS_LLID.hkey_id = 0;
	HM_SFU_DS_LLID.pol_id = 0;		// LLID

	hash_mask_unmask(&HM_SFU_DS_LLID_OUTVID_8021P);
	HM_SFU_DS_LLID_OUTVID_8021P.hkey_id = 0;
	HM_SFU_DS_LLID_OUTVID_8021P.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_OUTVID_8021P.top_vid = 0;
	HM_SFU_DS_LLID_OUTVID_8021P.top_8021p = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_VLAN_0_TAG);
	HM_SFU_DS_LLID_VLAN_0_TAG.hkey_id = 0;
	HM_SFU_DS_LLID_VLAN_0_TAG.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_VLAN_0_TAG.vlan_cnt = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_VLAN_1_TAG);
	HM_SFU_DS_LLID_VLAN_1_TAG.hkey_id = 0;
	HM_SFU_DS_LLID_VLAN_1_TAG.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_VLAN_1_TAG.vlan_cnt = 0;
	HM_SFU_DS_LLID_VLAN_1_TAG.top_vid = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_VLAN_2_TAG);
	HM_SFU_DS_LLID_VLAN_2_TAG.hkey_id = 0;
	HM_SFU_DS_LLID_VLAN_2_TAG.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_VLAN_2_TAG.vlan_cnt = 0;
	HM_SFU_DS_LLID_VLAN_2_TAG.top_vid = 0;
	HM_SFU_DS_LLID_VLAN_2_TAG.inner_vid = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_OUT8021P);
	HM_SFU_DS_LLID_OUT8021P.hkey_id = 0;
	HM_SFU_DS_LLID_OUT8021P.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_OUT8021P.top_8021p = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_ETHTYPE);
	HM_SFU_DS_LLID_ETHTYPE.hkey_id = 0;
	HM_SFU_DS_LLID_ETHTYPE.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_ETHTYPE.ethertype = 0;

	hash_mask_unmask(&HM_SFU_DS_LLID_MACDARNG);
	HM_SFU_DS_LLID_MACDARNG.hkey_id = 0;
	HM_SFU_DS_LLID_MACDARNG.pol_id = 0;		// LLID
	HM_SFU_DS_LLID_MACDARNG.mac_da = 0x7;		// unmask MSB 3 bytes mask

	return AAL_E_OK;
}

/***************************************************************************
 * Profiles
 ***************************************************************************/

#if defined(CONFIG_NE_L3COS)
static aal_hashlite_profile_t profile_0 = {
	.type0_act_grp_mask_hit =	HL_ACTGRP_L3_GENERIC,
	.type1_act_grp_mask_hit =	HL_ACTGRP_L3COS,

	.miss = {
		.type0_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type0_empty_miss =			&ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		HL_ACTGRP_L3_GENERIC,
		.type0_age_miss =			&ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type1_empty_miss =			&ACT_DROP,
		.type1_empty_miss_len =			sizeof(ACT_DROP),

		.type1_act_grp_mask_age_miss =		HL_ACTGRP_L3_GENERIC,
		.type1_age_miss =			&ACT_DROP,
		.type1_age_miss_len =			sizeof(ACT_DROP),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HM_L3_NEIGHBOR },
		{ TUPLE_TYPE_QOS,	&HM_HGU_DS },
		{ TUPLE_TYPE_QOS,	&HM_HGU_US_INTF },
		{ TUPLE_TYPE_QOS,	&HM_HGU_US_8021P },
		{ TUPLE_TYPE_QOS,	&HM_HGU_US_INTF_8021P },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};
#else
static aal_hashlite_profile_t profile_0 = {
	.type0_act_grp_mask_hit =	HL_ACTGRP_L3_GENERIC,
	.type1_act_grp_mask_hit =	HL_ACTGRP_L3_GENERIC,

	.miss = {
		.type0_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type0_empty_miss =			&ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		HL_ACTGRP_L3_GENERIC,
		.type0_age_miss =			&ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type1_empty_miss =			&ACT_DEFAULT_TO_CPU,
		.type1_empty_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_age_miss =		HL_ACTGRP_L3_GENERIC,
		.type1_age_miss =			&ACT_DEFAULT_TO_CPU,
		.type1_age_miss_len =			sizeof(ACT_DEFAULT_TO_CPU),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HM_L3_NEIGHBOR },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};
#endif /* CONFIG_NE_L3COS */

static aal_hashlite_profile_t profile_1 = {
	.type0_act_grp_mask_hit =	HL_ACTGRP_SFU_US,
	.type1_act_grp_mask_hit =	HL_ACTGRP_SFU_DS_LLID,

	.miss = {
		.type0_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type0_empty_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type0_empty_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type0_act_grp_mask_age_miss = 		HL_ACTGRP_L3_GENERIC,
		.type0_age_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type0_age_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type1_act_grp_mask_empty_miss =	HL_ACTGRP_L3_GENERIC,
		.type1_empty_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type1_empty_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type1_act_grp_mask_age_miss =		HL_ACTGRP_L3_GENERIC,
		.type1_age_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type1_age_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HM_SFU_US },
		{ TUPLE_TYPE_QOS,	&HM_SFU_DS_MC },
		{ TUPLE_TYPE_QOS,	&HM_SFU_DS_MC_ETHTYPE },
		{ TUPLE_TYPE_QOS,	&HM_SFU_DS_MC_VLAN },
		{ TUPLE_TYPE_QOS,	&HM_SFU_DS_MC_IGMPV3 },
		{ TUPLE_TYPE_QOS,	&HM_SFU_DS_MC_MACDA },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_2 = {
	.type0_act_grp_mask_hit =	HL_ACTGRP_SFU_DS_LLID,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	HL_ACTGRP_SFU_DS_LLID,
		.type0_empty_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type0_empty_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type0_act_grp_mask_age_miss = 		HL_ACTGRP_SFU_DS_LLID,
		.type0_age_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type0_age_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type1_act_grp_mask_empty_miss =	HL_ACTGRP_SFU_DS_LLID,
		.type1_empty_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type1_empty_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),

		.type1_act_grp_mask_age_miss =		HL_ACTGRP_SFU_DS_LLID,
		.type1_age_miss =			&ACT_KEEP_ORIGINAL_PKT,
		.type1_age_miss_len =			sizeof(ACT_KEEP_ORIGINAL_PKT),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_VLAN_0_TAG },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_VLAN_1_TAG },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_VLAN_2_TAG },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_OUT8021P },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_OUTVID_8021P },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_ETHTYPE },
		{ TUPLE_TYPE_FWD,	&HM_SFU_DS_LLID_MACDARNG },
	},
};

static aal_hashlite_profile_t profile_3 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_4 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_5 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_6 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_7 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_8 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_9 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_10 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_11 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_12 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t profile_13 = {
	.type0_act_grp_mask_hit =	0,
	.type1_act_grp_mask_hit =	0,

	.miss = {
		.type0_act_grp_mask_empty_miss =	0,
		.type0_empty_miss =			NULL,
		.type0_empty_miss_len =			0,

		.type0_act_grp_mask_age_miss = 		0,
		.type0_age_miss =			NULL,
		.type0_age_miss_len =			0,

		.type1_act_grp_mask_empty_miss =	0,
		.type1_empty_miss =			NULL,
		.type1_empty_miss_len =			0,

		.type1_act_grp_mask_age_miss =		0,
		.type1_age_miss =			NULL,
		.type1_age_miss_len =			0,
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
		{ TUPLE_TYPE_UNUSED,	NULL },
	},
};

static aal_hashlite_profile_t *generic_profiles[14] = {
	&profile_0, &profile_1, &profile_2, &profile_3,
	&profile_4, &profile_5, &profile_6, &profile_7,
	&profile_8, &profile_9, &profile_10, &profile_11,
	&profile_12, &profile_13
};

int hashlite_profile_init(void)
{
	int ret;

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_0, &profile_0)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 0. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_1, &profile_1)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 1. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_2, &profile_2)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 2. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_3, &profile_3)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 3. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_4, &profile_4)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 4. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_set(0, HASHLITE_PROFILE_5, &profile_5)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 5. (ret %d)\n", ret);
	}

	if ((ret = aal_hashlite_profile_14_set(0, HL_ACTGRP_L3_GENERIC)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to initialize HashLite profile 14. (ret %d)\n", ret);
	}

	return ret;
}

int aal_hashlite_dft_act_set(ca_device_id_t device_id, unsigned int profile_id, void *dft_act, int act_len)
{
	int ret;
	aal_hashlite_profile_t *profile;

	if (profile_id < 0 || profile_id >= 14)
		return AAL_E_OUTRANGE;

	profile = generic_profiles[profile_id];
	profile->miss.type0_empty_miss = dft_act;
	profile->miss.type0_empty_miss_len = act_len;
	profile->miss.type0_age_miss = dft_act;
	profile->miss.type0_age_miss_len = act_len;
	profile->miss.type1_empty_miss = dft_act;
	profile->miss.type1_empty_miss_len = act_len;
	profile->miss.type1_age_miss = dft_act;
	profile->miss.type1_age_miss_len = act_len;

	if ((ret = aal_hashlite_profile_set(device_id, profile_id, profile)) != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to set default action of HashLite profile %d. (ret %d)\n", profile_id, ret);
	}

	return ret;
}
/***************************************************************************
 * HashLite Double Check Fail
 ***************************************************************************/

aal_hashlite_dbl_chk_fail_ctrl_t type0_dbl_chk_fail = {
	.dpid = AAL_LPORT_CPU_0,
	.dpid_vld = 1,
	.bypass_next = 1,
};

int hashlite_dbl_chk_fail_init(void)
{
	int ret;

	ret = aal_hashlite_dbl_chk_fail_ctrl_set(&type0_dbl_chk_fail);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to set HashLite Double Check Fail. (ret %d)\n", ret);
	}

	return ret;
}

