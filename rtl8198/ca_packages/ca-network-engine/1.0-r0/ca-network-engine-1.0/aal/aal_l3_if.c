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
 */
/*
 * aal_l3_if.c: Hardware Abstraction Layer for L3 IF to access hardware regsiters
 */

#include "aal_table.h"
#include "aal_l3_if.h"

static ca_uint8 ifid_lpid_map[FE_L3E_IF_TBL_ENTRY_MAX];
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_uint8 hgu_8279_ifid_lpid_map[FE_L3E_IF_TBL_ENTRY_MAX];
#endif

static int develop_test;

/******************************************************************************/
int aal_l3_if_id_get(unsigned int lpid)
{
	int i;

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++)
		if (ifid_lpid_map[i] == lpid)
			return i;

	return -1;
}

/******************************************************************************/
static aal_table_t fe_l3e_if_tbl_table;

static int fe_l3e_if_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

	return generic_entry_add(table_p, entry, rslt_idx);
}

static int fe_l3e_if_tbl_entry_add_by_idx(void *entry, unsigned int if_id)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = ifid_lpid_map;
	fe_l3e_if_tbl_entry_t *eif;
	int rc;

	rc = generic_entry_add_by_idx(table_p, entry, if_id);
	if (rc == AAL_E_OK) {
		eif = (fe_l3e_if_tbl_entry_t *)entry;
		lpid_map[if_id] = eif->ldpid;
	}

	return rc;
}

static int fe_l3e_if_tbl_entry_del(unsigned int if_id)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = ifid_lpid_map;
	int rc;

	rc = generic_entry_del(table_p, if_id);
	if (rc == AAL_E_OK)
		lpid_map[if_id] = CA_UINT8_INVALID;

	return rc;
}

static int fe_l3e_if_tbl_entry_set(unsigned int if_id, void *entry)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = ifid_lpid_map;
	fe_l3e_if_tbl_entry_t *eif;
	int rc;

	rc = generic_entry_set(table_p, if_id, entry);
	if (rc == AAL_E_OK) {
		eif = (fe_l3e_if_tbl_entry_t *)entry;
		lpid_map[if_id] = eif->ldpid;
	}

	return rc;
}

static int fe_l3e_if_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int fe_l3e_if_tbl_entry_get_by_idx(unsigned int if_id, void *entry)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

	return generic_entry_get_by_idx(table_p, if_id, entry);
}

static int fe_l3e_if_tbl_table_flush(void)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = ifid_lpid_map;
	int i, rc;

	rc =  generic_table_flush(table_p);
	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++)
		lpid_map[i] = CA_UINT8_INVALID;

	return rc;
}

static int fe_l3e_if_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

	return generic_get_avail_count(table_p);
}

static void __fe_l3e_if_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	fe_l3e_if_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry)) {
		end_idx = table_p->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = table_p->op.entry_get_by_idx(i, &entry);
		} else {
			ret = hw_entry_get(table_p, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "mtu_vld(1) = %d\n", entry.mtu_vld);
			printk(KERN_INFO "mtu_enc(2) = %d\n", entry.mtu_enc);
			printk(KERN_INFO "inner_802_1p_sel(3) = %d\n", entry.inner_802_1p_sel);
			printk(KERN_INFO "inner_802_1p(3) = %d\n", entry.inner_802_1p);
			printk(KERN_INFO "top_802_1p_sel(3) = %d\n", entry.top_802_1p_sel);
			printk(KERN_INFO "top_802_1p(3) = %d\n", entry.top_802_1p);
			printk(KERN_INFO "qos_802_1p_table_sel(1) = %d\n", entry.qos_802_1p_table_sel);
			printk(KERN_INFO "vlan_vld(1) = %d\n", entry.vlan_vld);
			printk(KERN_INFO "inner_dei_sel(2) = %d\n", entry.inner_dei_sel);
			printk(KERN_INFO "inner_dei(1) = %d\n", entry.inner_dei);
			printk(KERN_INFO "inner_vid(12) = %d\n", entry.inner_vid);
			printk(KERN_INFO "inner_tpid_enc(3) = %d\n", entry.inner_tpid_enc);
			printk(KERN_INFO "top_dei_sel(2) = %d\n", entry.top_dei_sel);
			printk(KERN_INFO "top_dei(1) = %d\n", entry.top_dei);
			printk(KERN_INFO "top_vid(12) = %d\n", entry.top_vid);
			printk(KERN_INFO "top_tpid_enc(3) = %d\n", entry.top_tpid_enc);
			printk(KERN_INFO "vlan_cnt(2) = %d\n", entry.vlan_cnt);
			printk(KERN_INFO "pppoe_set(1) = %d\n", entry.pppoe_set);
			printk(KERN_INFO "pppoe_vld(1) = %d\n", entry.pppoe_vld);
			printk(KERN_INFO "pppoe_session_id(16) = %d\n", entry.pppoe_session_id);
			printk(KERN_INFO "mac_da_vld(1) = %d\n", entry.mac_da_vld);
			printk(KERN_INFO "mac_da_an_sel(4) = %d\n", entry.mac_da_an_sel);
			printk(KERN_INFO "mac_sa_vld(1) = %d\n", entry.mac_sa_vld);
			printk(KERN_INFO "mac_sa_an_sel(4) = %d\n", entry.mac_sa_an_sel);
			printk(KERN_INFO "l2_format_vld(1) = %d\n", entry.l2_format_vld);
			printk(KERN_INFO "l2_format(1) = %d\n", entry.l2_format);
			printk(KERN_INFO "ldpid_vld(1) = %d\n", entry.ldpid_vld);
			printk(KERN_INFO "ldpid(6) = %d\n", entry.ldpid);
			printk(KERN_INFO "deep_q(1) = %d\n", entry.deep_q);
			printk(KERN_INFO "stage3_ctrl_vld(1) = %d\n", entry.stage3_ctrl_vld);
			printk(KERN_INFO "stage3_ctrl(1) = %d\n", entry.stage3_ctrl);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
	}
}

static void fe_l3e_if_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

	__fe_l3e_if_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t fe_l3e_if_tbl_table = {
	.table_id = AAL_TABLE_FE_L3E_IF_TBL,
	.table_name = "L3 Interface Table",
	.max_entry = FE_L3E_IF_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(fe_l3e_if_tbl_entry_t),
	.access_reg = L3FE_GLB_EGRESS_L3_IF_TBL_ACCESS,
	.data_reg_num = 3,
	.op = {
		.entry_add = fe_l3e_if_tbl_entry_add,
		.entry_add_by_idx = fe_l3e_if_tbl_entry_add_by_idx,
		.entry_del = fe_l3e_if_tbl_entry_del,
		.entry_set = fe_l3e_if_tbl_entry_set,
		.entry_get = fe_l3e_if_tbl_entry_get,
		.entry_get_by_idx = fe_l3e_if_tbl_entry_get_by_idx,
		.table_flush = fe_l3e_if_tbl_table_flush,
		.get_avail_count = fe_l3e_if_tbl_get_avail_count,
		.entry_print = fe_l3e_if_tbl_entry_print,
	},
};

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static aal_table_t hgu_8279_fe_l3e_if_tbl_table;

static int hgu_8279_fe_l3e_if_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;

	return generic_entry_add(table_p, entry, rslt_idx);
}

static int hgu_8279_fe_l3e_if_tbl_entry_add_by_idx(void *entry, unsigned int if_id)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = hgu_8279_ifid_lpid_map;
	fe_l3e_if_tbl_entry_t *eif;
	int rc;

	rc = generic_entry_add_by_idx(table_p, entry, if_id);
	if (rc == AAL_E_OK) {
		eif = (fe_l3e_if_tbl_entry_t *)entry;
		lpid_map[if_id] = eif->ldpid;
	}

	return rc;
}

static int hgu_8279_fe_l3e_if_tbl_entry_del(unsigned int if_id)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = hgu_8279_ifid_lpid_map;
	int rc;

	rc = generic_entry_del(table_p, if_id);
	if (rc == AAL_E_OK)
		lpid_map[if_id] = CA_UINT8_INVALID;

	return rc;
}

static int hgu_8279_fe_l3e_if_tbl_entry_set(unsigned int if_id, void *entry)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = hgu_8279_ifid_lpid_map;
	fe_l3e_if_tbl_entry_t *eif;
	int rc;

	rc = generic_entry_set(table_p, if_id, entry);
	if (rc == AAL_E_OK) {
		eif = (fe_l3e_if_tbl_entry_t *)entry;
		lpid_map[if_id] = eif->ldpid;
	}

	return rc;
}

static int hgu_8279_fe_l3e_if_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int hgu_8279_fe_l3e_if_tbl_entry_get_by_idx(unsigned int if_id, void *entry)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;

	return generic_entry_get_by_idx(table_p, if_id, entry);
}

static int hgu_8279_fe_l3e_if_tbl_table_flush(void)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;
	ca_uint8 *lpid_map = hgu_8279_ifid_lpid_map;
	int i, rc;

	rc =  generic_table_flush(table_p);
	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++)
		lpid_map[i] = CA_UINT8_INVALID;

	return rc;
}

static int hgu_8279_fe_l3e_if_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;

	return generic_get_avail_count(table_p);
}

static void hgu_8279_fe_l3e_if_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &hgu_8279_fe_l3e_if_tbl_table;

	__fe_l3e_if_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t hgu_8279_fe_l3e_if_tbl_table = {
	.table_id = AAL_TABLE_8279_FE_L3E_IF_TBL,
	.table_name = "HGU 8279 L3 Interface Table",
	.max_entry = FE_L3E_IF_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(fe_l3e_if_tbl_entry_t),
	.access_reg = SATURN_L3FE_GLB_EGRESS_L3_IF_TBL_ACCESS,
	.data_reg_num = 3,
	.op = {
		.entry_add = hgu_8279_fe_l3e_if_tbl_entry_add,
		.entry_add_by_idx = hgu_8279_fe_l3e_if_tbl_entry_add_by_idx,
		.entry_del = hgu_8279_fe_l3e_if_tbl_entry_del,
		.entry_set = hgu_8279_fe_l3e_if_tbl_entry_set,
		.entry_get = hgu_8279_fe_l3e_if_tbl_entry_get,
		.entry_get_by_idx = hgu_8279_fe_l3e_if_tbl_entry_get_by_idx,
		.table_flush = hgu_8279_fe_l3e_if_tbl_table_flush,
		.get_avail_count = hgu_8279_fe_l3e_if_tbl_get_avail_count,
		.entry_print = hgu_8279_fe_l3e_if_tbl_entry_print,
	},
};
#endif

/******************************************************************************/
static void __fe_l3e_if_early_develop_test(aal_table_t *table_p)
{
	fe_l3e_if_tbl_entry_t entry;
	unsigned int idx;
	int ret;

	printk("%s()\n", __func__);

	entry.mtu_vld = 1;		/* : 1; */
	entry.mtu_enc = 2;		/* : 2; */
	entry.inner_802_1p_sel = 6;	/* : 3; */
	entry.inner_802_1p = 5;		/* : 3; */
	entry.top_802_1p_sel = 4;	/* : 3; */
	entry.top_802_1p = 3;		/* : 3; */
	entry.qos_802_1p_table_sel = 1;	/* : 1; */
	entry.vlan_vld = 0;		/* : 1; */
	entry.inner_dei_sel = 2;	/* : 2; */
	entry.inner_dei	= 1;		/* : 1; */
	entry.inner_vid	= 2560;		/* : 12; */
	entry.inner_tpid_enc = 2;	/* : 3; */
	entry.top_dei_sel = 3;		/* : 2; */
	entry.top_dei = 1;		/* : 1; */
	entry.top_vid = 1536;		/* : 12; */
	entry.top_tpid_enc = 7;		/* : 3; */
	entry.vlan_cnt = 1;		/* : 2; */
	entry.pppoe_set = 0;		/* : 1; */
	entry.pppoe_vld = 1;		/* : 1; */
	entry.pppoe_session_id = 32768;	/* : 16; */
	entry.mac_da_vld = 0;		/* : 1; */
	entry.mac_da_an_sel = 12;	/* : 4; */
	entry.mac_sa_vld = 1;		/* : 1; */
	entry.mac_sa_an_sel = 13;	/* : 4; */
	entry.l2_format_vld = 1;	/* : 1; */
	entry.l2_format	= 0;		/* : 1; */
	entry.ldpid_vld	= 1;		/* : 1; */
	entry.ldpid = 48;		/* : 6; */
	entry.deep_q = 0;		/* : 1; */
	entry.stage3_ctrl_vld = 1;	/* : 1; */
	entry.stage3_ctrl = 1;		/* : 1; */

	idx = 0;
	ret = table_p->op.entry_add_by_idx(&entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	entry.mtu_vld = ~entry.mtu_vld;
	entry.mtu_enc = ~entry.mtu_enc;
	entry.inner_802_1p_sel = ~entry.inner_802_1p_sel;
	entry.inner_802_1p = ~entry.inner_802_1p;
	entry.top_802_1p_sel = ~entry.top_802_1p_sel;
	entry.top_802_1p = ~entry.top_802_1p;
	entry.qos_802_1p_table_sel = ~entry.qos_802_1p_table_sel;
	entry.vlan_vld = ~entry.vlan_vld;
	entry.inner_dei_sel = ~entry.inner_dei_sel;
	entry.inner_dei	= ~entry.inner_dei;
	entry.inner_vid	= ~entry.inner_vid;
	entry.inner_tpid_enc = ~entry.inner_tpid_enc;
	entry.top_dei_sel = ~entry.top_dei_sel;
	entry.top_dei = ~entry.top_dei;
	entry.top_vid = ~entry.top_vid;
	entry.top_tpid_enc = ~entry.top_tpid_enc;
	entry.vlan_cnt = ~entry.vlan_cnt;
	entry.pppoe_set = ~entry.vlan_cnt;
	entry.pppoe_vld = ~entry.pppoe_vld;
	entry.pppoe_session_id = ~entry.pppoe_session_id;
	entry.mac_da_vld = ~entry.mac_da_vld;
	entry.mac_da_an_sel = ~entry.mac_da_an_sel;
	entry.mac_sa_vld = ~entry.mac_sa_vld;
	entry.mac_sa_an_sel = ~entry.mac_sa_an_sel;
	entry.l2_format_vld = ~entry.l2_format_vld;
	entry.l2_format	= ~entry.l2_format;
	entry.ldpid_vld	= ~entry.ldpid_vld;
	entry.ldpid = ~entry.ldpid;
	entry.deep_q = ~entry.deep_q;
	entry.stage3_ctrl_vld = ~entry.stage3_ctrl_vld;
	entry.stage3_ctrl = ~entry.stage3_ctrl;

	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

int aal_fe_l3e_if_tbl_init(void)
{
	aal_table_t *table_p;
	ca_uint8 *lpid_map;
	int i, ret;

	table_p = &fe_l3e_if_tbl_table;
	lpid_map = ifid_lpid_map;

	spin_lock_init(&table_p->lock);

	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++)
		lpid_map[i] = CA_UINT8_INVALID;

	if (develop_test)
		__fe_l3e_if_early_develop_test(table_p);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	table_p = &hgu_8279_fe_l3e_if_tbl_table;
	lpid_map = hgu_8279_ifid_lpid_map;

	spin_lock_init(&table_p->lock);

	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++)
		lpid_map[i] = CA_UINT8_INVALID;

	if (develop_test)
		__fe_l3e_if_early_develop_test(table_p);
#endif

	return AAL_E_OK;
}

void aal_fe_l3e_if_tbl_exit(void)
{
	aal_table_t *table_p;

	table_p = &fe_l3e_if_tbl_table;
	__aal_table_unregister(table_p->table_id);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	table_p = &hgu_8279_fe_l3e_if_tbl_table;
	__aal_table_unregister(table_p->table_id);
#endif
}

/*******************************************************************************
 * APP CLI APIs
 ******************************************************************************/
ca_status_t aal_l3_if_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT fe_l3e_if_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1)
		table_p = &hgu_8279_fe_l3e_if_tbl_table;
#endif

	return hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_if_get);

ca_status_t aal_l3_if_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN fe_l3e_if_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &fe_l3e_if_tbl_table;
	fe_l3e_if_tbl_entry_t tmp_entry;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1)
		table_p = &hgu_8279_fe_l3e_if_tbl_table;
#endif

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_if_set);

