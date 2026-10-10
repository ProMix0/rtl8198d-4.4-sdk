/*
 * Copyright (c) Cortina-Access Limited 2016.  All rights reserved.
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
 * aal_idx4pe.c: VPN Offload Index for Packet Engine (IDX4PE)
 * This is tunnel indenpendent; each VPN tunnel should have self index for PE.
 * This is not related to any HW registers; pure software.
 */

#include <linux/sched.h>

#include "ca_types.h"
#include "aal_idx4pe.h"
#include "ca_mut.h"

#define TABLE_TRY_TIMEOUT	1000

static aal_table_t ipsec_idx4pe_table;
static aal_table_t macsec_idx4pe_table;
static aal_table_t l2tp_idx4pe_table;
static aal_table_t pptp_idx4pe_table;
static aal_table_t map_e_idx4pe_table;
static aal_table_t map_t_idx4pe_table;

/***************************************************************************
 * AAL table - IPSec IDX4PE
 ***************************************************************************/

static int aal_ipsec_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &ipsec_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_ipsec_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &ipsec_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_ipsec_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &ipsec_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_ipsec_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &ipsec_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_ipsec_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_ipsec_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&ipsec_idx4pe_table);
}

static void aal_ipsec_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &ipsec_idx4pe_table;
	aal_ipsec_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== IPSec IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_ipsec_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d, sa_id_encrypt %d, sa_id_decrypt %d\n",
				entry.valid, entry.sa_id_encrypt, entry.sa_id_decrypt);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t ipsec_idx4pe_table = {
	.table_id = AAL_TABLE_IPSEC_IDX4PE,
	.table_name = "IPSec IDX4PE",
	.max_entry = IPSEC_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_ipsec_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_ipsec_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_ipsec_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_ipsec_idx4pe_entry_get_by_idx,
		.table_flush = aal_ipsec_idx4pe_table_flush,
		.get_avail_count = aal_ipsec_idx4pe_get_avail_count,
		.entry_print = aal_ipsec_idx4pe_entry_print,
	},
};

int aal_ipsec_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&ipsec_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_IPSEC_IDX4PE, &ipsec_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_ipsec_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_IPSEC_IDX4PE);
}

/***************************************************************************
 * AAL table - MACSec IDX4PE
 ***************************************************************************/

static int aal_macsec_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &macsec_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_macsec_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &macsec_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_macsec_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &macsec_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_macsec_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &macsec_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_macsec_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_macsec_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&macsec_idx4pe_table);
}

static void aal_macsec_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &macsec_idx4pe_table;
	aal_macsec_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== MACSec IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_macsec_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d, sa_id_outbound %d, sa_id_inbound %d\n",
				entry.valid, entry.sa_id_outbound, entry.sa_id_inbound);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t macsec_idx4pe_table = {
	.table_id = AAL_TABLE_MACSEC_IDX4PE,
	.table_name = "macsec IDX4PE",
	.max_entry = MACSEC_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_macsec_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_macsec_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_macsec_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_macsec_idx4pe_entry_get_by_idx,
		.table_flush = aal_macsec_idx4pe_table_flush,
		.get_avail_count = aal_macsec_idx4pe_get_avail_count,
		.entry_print = aal_macsec_idx4pe_entry_print,
	},
};

int aal_macsec_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&macsec_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_MACSEC_IDX4PE, &macsec_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_macsec_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_MACSEC_IDX4PE);
}

/***************************************************************************
 * AAL table - L2TP IDX4PE
 ***************************************************************************/

static int aal_l2tp_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &l2tp_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_l2tp_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &l2tp_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_l2tp_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &l2tp_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_l2tp_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &l2tp_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_l2tp_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_l2tp_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&l2tp_idx4pe_table);
}

static void aal_l2tp_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &l2tp_idx4pe_table;
	aal_l2tp_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== L2TP IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_l2tp_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d\n",
				entry.valid);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t l2tp_idx4pe_table = {
	.table_id = AAL_TABLE_L2TP_IDX4PE,
	.table_name = "L2TP IDX4PE",
	.max_entry = L2TP_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_l2tp_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_l2tp_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_l2tp_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_l2tp_idx4pe_entry_get_by_idx,
		.table_flush = aal_l2tp_idx4pe_table_flush,
		.get_avail_count = aal_l2tp_idx4pe_get_avail_count,
		.entry_print = aal_l2tp_idx4pe_entry_print,
	},
};

int aal_l2tp_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&l2tp_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_L2TP_IDX4PE, &l2tp_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_l2tp_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_L2TP_IDX4PE);
}

/***************************************************************************
 * AAL table - PPTP IDX4PE
 ***************************************************************************/

static int aal_pptp_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &pptp_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_pptp_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &pptp_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_pptp_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &pptp_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_pptp_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &pptp_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_pptp_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_pptp_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&pptp_idx4pe_table);
}

static void aal_pptp_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &pptp_idx4pe_table;
	aal_pptp_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== PPTP IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_pptp_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d\n",
				entry.valid);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t pptp_idx4pe_table = {
	.table_id = AAL_TABLE_PPTP_IDX4PE,
	.table_name = "PPTP IDX4PE",
	.max_entry = PPTP_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_pptp_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_pptp_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_pptp_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_pptp_idx4pe_entry_get_by_idx,
		.table_flush = aal_pptp_idx4pe_table_flush,
		.get_avail_count = aal_pptp_idx4pe_get_avail_count,
		.entry_print = aal_pptp_idx4pe_entry_print,
	},
};

int aal_pptp_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&pptp_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_PPTP_IDX4PE, &pptp_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_pptp_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_PPTP_IDX4PE);
}

/***************************************************************************
 * AAL table - MAP-E IDX4PE
 ***************************************************************************/

static int aal_map_e_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &map_e_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_map_e_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &map_e_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_map_e_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &map_e_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_map_e_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &map_e_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_map_e_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_map_e_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&map_e_idx4pe_table);
}

static void aal_map_e_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &map_e_idx4pe_table;
	aal_map_e_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== MAP-E IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_map_e_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d\n",
				entry.valid);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t map_e_idx4pe_table = {
	.table_id = AAL_TABLE_MAP_E_IDX4PE,
	.table_name = "MAP-E IDX4PE",
	.max_entry = MAP_E_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_map_e_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_map_e_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_map_e_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_map_e_idx4pe_entry_get_by_idx,
		.table_flush = aal_map_e_idx4pe_table_flush,
		.get_avail_count = aal_map_e_idx4pe_get_avail_count,
		.entry_print = aal_map_e_idx4pe_entry_print,
	},
};

int aal_map_e_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&map_e_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_MAP_E_IDX4PE, &map_e_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_map_e_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_MAP_E_IDX4PE);
}

/***************************************************************************
 * AAL table - MAP-T IDX4PE
 ***************************************************************************/

static int aal_map_t_idx4pe_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &map_t_idx4pe_table;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			spin_unlock(&table->lock);
			return AAL_E_OK;
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_TBLFULL;
}

static int aal_map_t_idx4pe_entry_del(unsigned int idx)
{
	aal_table_t *table = &map_t_idx4pe_table;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;
	}

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_map_t_idx4pe_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &map_t_idx4pe_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_map_t_idx4pe_table_flush(void)
{
	int i;
	aal_table_t *table = &map_t_idx4pe_table;

	for (i = 0; i < table->max_entry; i++)
		aal_map_t_idx4pe_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_map_t_idx4pe_get_avail_count(void)
{
	return generic_get_avail_count(&map_t_idx4pe_table);
}

static void aal_map_t_idx4pe_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &map_t_idx4pe_table;
	aal_map_t_idx4pe_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== MAP-T IDX4PE Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_map_t_idx4pe_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(idx4pe) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(idx4pe) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d\n",
				entry.valid);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t map_t_idx4pe_table = {
	.table_id = AAL_TABLE_MAP_T_IDX4PE,
	.table_name = "MAP-T IDX4PE",
	.max_entry = MAP_T_IDX4PE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_map_t_idx4pe_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_map_t_idx4pe_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_map_t_idx4pe_entry_del,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = aal_map_t_idx4pe_entry_get_by_idx,
		.table_flush = aal_map_t_idx4pe_table_flush,
		.get_avail_count = aal_map_t_idx4pe_get_avail_count,
		.entry_print = aal_map_t_idx4pe_entry_print,
	},
};

int aal_map_t_idx4pe_tbl_init(void)
{
	int ret;

	spin_lock_init(&map_t_idx4pe_table.lock);

	ret = __aal_table_register(AAL_TABLE_MAP_T_IDX4PE, &map_t_idx4pe_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void aal_map_t_idx4pe_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_MAP_T_IDX4PE);
}

/***************************************************************************
 * Common init/exit functions
 ***************************************************************************/

int aal_idx4pe_tbl_init(void)
{
	int ret;

	if ((ret = aal_ipsec_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize IPSec IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	if ((ret = aal_l2tp_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize L2TP IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	if ((ret = aal_pptp_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize PPTP IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	if ((ret = aal_map_e_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize MAP-E IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	if ((ret = aal_map_t_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize MAP-T IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	if ((ret = aal_macsec_idx4pe_tbl_init()) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to initialize MACSec IDX4PE table. (ret %d)\n", __func__, ret);
		return ret;
	}

	return AAL_E_OK;
}

void aal_idx4pe_tbl_exit(void)
{
	aal_ipsec_idx4pe_tbl_exit();
	aal_l2tp_idx4pe_tbl_exit();
	aal_pptp_idx4pe_tbl_exit();
	aal_map_e_idx4pe_tbl_exit();
	aal_map_t_idx4pe_tbl_exit();
}

