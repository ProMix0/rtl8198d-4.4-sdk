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
 * aal_hashlite_iros.c: HashLite Engine AAL API exported for iROS
 *
 */

#include <generated/ca_ne_autoconf.h>
#include "aal_hashlite.h"
#include "ca_aal.h"

/***************************************************************************
 * internal functions
 ***************************************************************************/

static void convert_act_l3_generic(aal_hash_action_t *act1, hl_act_l3_generic_t *act2)
{
	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 11 */
	act2->mac_da_vld = act1->mac_da_vld;
	act2->mac_da_0 = act1->mac_da_0;
	act2->mac_da_1 = act1->mac_da_1;
	act2->mac_da_2 = act1->mac_da_2;
	act2->mac_da_3 = act1->mac_da_3;
	act2->mac_da_4 = act1->mac_da_4;
	act2->mac_da_5 = act1->mac_da_5;

	/* group 13 */
	act2->mdata_w_vld_1 = act1->mdata_w_vld_1;
	act2->mdata_w_1 = act1->mdata_w_1;

	/* group 14 */
	act2->mdata_w_vld_2 = act1->mdata_w_vld_2;
	act2->mdata_w_2 = act1->mdata_w_2;

	/* group 15 */
	act2->mdata_w_vld_3 = act1->mdata_w_vld_3;
	act2->mdata_w_3 = act1->mdata_w_3;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->t2_ctrl = act1->t2_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->chk_msk_ptr = act1->chk_msk_ptr;
}

static void convert_act_l3cos(aal_hash_action_t *act1, hl_act_l3cos_t *act2)
{
	/* group 17 */
	act2->cos = act1->cos;
	act2->cos_sel = act1->cos_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_ecn_vld = act1->ip_ecn_vld;
}

static void convert_act_hgu_ds(aal_hash_action_t *act1, hl_act_hgu_ds_t *act2)
{
	/* group 9 */
	act2->vlan_vld = act1->vlan_vld;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_enc = act1->top_tpid_enc;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;
}

static void convert_act_sfu_us(aal_hash_action_t *act1, hl_act_sfu_us_t *act2)
{
	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->cos = act1->cos;
	act2->cos_sel = act1->cos_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_ecn_vld = act1->ip_ecn_vld;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;
}

static void convert_act_sfu_ds_llid(aal_hash_action_t *act1, hl_act_sfu_ds_llid_t *act2)
{
	/* group 9 */
	act2->vlan_vld = act1->vlan_vld;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_enc = act1->top_tpid_enc;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;
}

/***************************************************************************
 * API
 ***************************************************************************/

ca_status_t ca_aal_hashlite_hash_add (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	aal_hash_key_t				*hash_key,
	CA_IN	ca_uint32_t				hash_mask_idx,
	CA_IN	aal_hash_action_t			*hash_action,
	CA_IN	ca_uint32_t				actgrp_mask,
	CA_IN	aal_hashlite_aging_e			aging,
	CA_OUT	ca_uint32_t				*rslt_hash_idx )
{
	int ret = AAL_E_INVALID;
	aal_hash_mask_t hash_mask;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3_generic_t *act_l3_generic = (hl_act_l3_generic_t *) &hash_action_entry;
	hl_act_l3cos_t *act_hgu_us = (hl_act_l3cos_t *) &hash_action_entry;
	hl_act_hgu_ds_t *act_hgu_ds = (hl_act_hgu_ds_t *) &hash_action_entry;
	hl_act_sfu_us_t *act_sfu_us = (hl_act_sfu_us_t *) &hash_action_entry;
	hl_act_sfu_ds_llid_t *act_sfu_ds_llid = (hl_act_sfu_ds_llid_t *) &hash_action_entry;

	if (hash_key == NULL) return CA_E_PARAM;
	if (hash_action == NULL) return CA_E_PARAM;
	if (rslt_hash_idx == NULL) return CA_E_PARAM;
	if ((aging < HASHLITE_AGING_0) || (aging > HASHLITE_AGING_15)) return CA_E_PARAM;

	/* get hash_mask */

	ret = aal_entry_get_by_idx(AAL_TABLE_HASHLITE_MASK_TBL, hash_mask_idx, &hash_mask);
	if (ret != AAL_E_OK) return CA_E_NOT_FOUND;

	/* fill hash_action_entry according to hash_action and actgrp_mask */

	memset(&hash_action_entry, 0, sizeof(hash_action_entry));

	switch (actgrp_mask) {
	case HL_ACTGRP_L3_GENERIC:
		convert_act_l3_generic(hash_action, act_l3_generic);
		ret = aal_hashlite_hash_add(device_id, hash_key, &hash_mask, &hash_action_entry, actgrp_mask, aging, rslt_hash_idx);
		break;

	case HL_ACTGRP_L3COS:
		convert_act_l3cos(hash_action, act_hgu_us);
		ret = aal_hashlite_hash_add(device_id, hash_key, &hash_mask, &hash_action_entry, actgrp_mask, aging, rslt_hash_idx);
		break;

	case HL_ACTGRP_HGU_DS:
		convert_act_hgu_ds(hash_action, act_hgu_ds);
		ret = aal_hashlite_hash_add(device_id, hash_key, &hash_mask, &hash_action_entry, actgrp_mask, aging, rslt_hash_idx);
		break;

	case HL_ACTGRP_SFU_US:
		convert_act_sfu_us(hash_action, act_sfu_us);
		ret = aal_hashlite_hash_add(device_id, hash_key, &hash_mask, &hash_action_entry, actgrp_mask, aging, rslt_hash_idx);
		break;

	default:
		return CA_E_NOT_SUPPORT;
	}

	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_hash_add);

ca_status_t ca_aal_hashlite_hash_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_uint32_t				hash_idx )
{
	int ret;

	ret = aal_hashlite_hash_delete(device_id, hash_idx);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_hash_delete);

ca_status_t ca_aal_hashlite_hashmask_add (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	aal_hash_mask_t				*hash_mask,
	CA_OUT	ca_uint32_t				*rslt_hash_mask_idx )
{
	int ret;

	ret = aal_entry_add(AAL_TABLE_HASHLITE_MASK_TBL, hash_mask, rslt_hash_mask_idx);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_hashmask_add);

ca_status_t ca_aal_hashlite_hashmask_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_uint32_t				hash_mask_idx )
{
	int ret;

	ret = aal_entry_del(AAL_TABLE_HASHLITE_MASK_TBL, hash_mask_idx);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_hashmask_delete);

/* Set global aging time in seconds. */
ca_status_t ca_aal_hashlite_aging_timer_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_uint32_t				time )
{
	int ret;

	ret = aal_hashlite_aging_timer_set(time);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_aging_timer_set);

/* Get global aging time in seconds. */
ca_status_t ca_aal_hashlite_aging_timer_get (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_uint32_t				*time )
{
	int ret;

	ret = aal_hashlite_aging_timer_get(time);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_aging_timer_get);

ca_status_t ca_aal_hashlite_granularity_get (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_uint32_t				*granularity )
{
	int ret;

	ret = aal_hashlite_granularity_get(granularity);
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_granularity_get);

/* Set the age of specified hash entry.
 * Due to HW design, only allow to set the value in aal_hashlite_aging_e.
 */
ca_status_t ca_aal_hashlite_age_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_uint32_t				hash_idx,
	CA_IN	aal_hashlite_aging_e			age )
{
	int ret;

	if (hash_idx & HASHLITE_OVERFLOW_FLAG) {
		ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_OVERFLOW_TBL, hash_idx & HASHLITE_INDEX_MASK, age);
	} else {
		ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, age);
	}
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_age_set);

/* Get the age of specified hash entry.
 * It also returns the approximated time in seconds.
 */
ca_status_t ca_aal_hashlite_age_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_uint32_t				hash_idx,
	CA_OUT	aal_hashlite_aging_e			*age,
	CA_OUT	ca_uint32_t				*time )
{
	int ret;

	if (hash_idx & HASHLITE_OVERFLOW_FLAG) {
		ret = aal_hashlite_age_get(AAL_TABLE_HASHLITE_OVERFLOW_TBL, hash_idx & HASHLITE_INDEX_MASK, age, time);
	} else {
		ret = aal_hashlite_age_get(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, age, time);
	}
	if (ret != AAL_E_OK) return CA_E_NEXTHOP;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_hashlite_age_get);

#ifdef INDEXED_INTERFACE_TABLE
ca_status_t ca_aal_idx_intf_mac_entry_add(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_mac_addr_t				mac,
	CA_OUT	ca_uint32_t				*rslt_idx)
{
	idx_intf_mac_entry_t mac_entry;
	int ret;

	memset(&mac_entry, 0, sizeof(mac_entry));
	aal_array_reverse_copy(mac_entry.mac_da, mac, 6);

	ret = aal_entry_add(AAL_TABLE_IDX_INTF_MAC, &mac_entry, rslt_idx);
	if (ret == AAL_E_TBLFULL)
		return CA_E_FULL;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_mac_entry_add);

ca_status_t ca_aal_idx_intf_mac_entry_delete(
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_uint32_t				idx)
{
	aal_entry_del(AAL_TABLE_IDX_INTF_MAC, idx);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_mac_entry_delete);

ca_status_t ca_aal_idx_intf_ipv4_entry_add(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_ip_address_t				addr,
	CA_OUT	ca_uint32_t				*rslt_idx)
{
	aal_entry_add(AAL_TABLE_IDX_INTF_IPV4, &addr.ip_addr.ipv4_addr, rslt_idx);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_ipv4_entry_add);

ca_status_t ca_aal_idx_intf_ipv4_entry_delete(
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_uint32_t				idx)
{
	aal_entry_del(AAL_TABLE_IDX_INTF_IPV4, idx);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_ipv4_entry_delete);

ca_status_t ca_aal_idx_intf_ipv6_entry_add(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_ip_address_t				addr,
	CA_OUT	ca_uint32_t				*rslt_idx)
{
	ca_uint32_t ipv6_addr[4];

	ipv6_addr[3] = addr.ip_addr.ipv6_addr[0];
	ipv6_addr[2] = addr.ip_addr.ipv6_addr[1];
	ipv6_addr[1] = addr.ip_addr.ipv6_addr[2];
	ipv6_addr[0] = addr.ip_addr.ipv6_addr[3];

	aal_entry_add(AAL_TABLE_IDX_INTF_IPV6, ipv6_addr, rslt_idx);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_ipv6_entry_add);

ca_status_t ca_aal_idx_intf_ipv6_entry_delete(
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_uint32_t				idx)
{
	aal_entry_del(AAL_TABLE_IDX_INTF_IPV6, idx);

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_idx_intf_ipv6_entry_delete);
#endif /* INDEXED_INTERFACE_TABLE */
