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
 * aal_sa_id.c: Security Association (SA ID) management for Packet Engine
 * This is not related to any HW registers; pure software.
 */

#include <linux/sched.h>

#include "ca_types.h"
#include "aal_sa_id.h"
#include "ca_mut.h"

#define TABLE_TRY_TIMEOUT	1000

static aal_table_t sa_id_table;

/***************************************************************************
 * internal functions
 ***************************************************************************/


/***************************************************************************
 * AAL table
 ***************************************************************************/

static int aal_sa_id_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &sa_id_table;
	aal_sa_id_entry_t *data = entry;
	int i, start, end;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	if (data->is_encrypted) {
		start = SA_ID_CRYPTO_MIN;
		end = SA_ID_CRYPTO_MAX;
	} else {
		start = SA_ID_PLAINTEXT_MIN;
		end = SA_ID_PLAINTEXT_MIN;
	}

	spin_lock(&table->lock);

	for (i = start; i <= end; i++) {
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

static int aal_sa_id_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table = &sa_id_table;
	aal_sa_id_entry_t *data = entry;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	if (data->is_encrypted) {
		if ((idx < SA_ID_CRYPTO_MIN) || (idx > SA_ID_CRYPTO_MAX))
			return AAL_E_OUTRANGE;
	} else {
		if ((idx < SA_ID_PLAINTEXT_MIN) || (idx > SA_ID_PLAINTEXT_MAX))
			return AAL_E_OUTRANGE;
	}

	if (table->entry[idx].data != NULL) return AAL_E_ENTRY_USED;

	spin_lock(&table->lock);

        table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
        memcpy(table->entry[idx].data, entry, table->entry_size);
        atomic_set(&table->entry[idx].refcnt, 1);
        table->used_entry++;

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_sa_id_entry_del(unsigned int idx)
{
	aal_table_t *table = &sa_id_table;

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

static int aal_sa_id_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table = &sa_id_table;
	aal_sa_id_entry_t *data = entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	if (data->is_encrypted) {
		if ((idx < SA_ID_CRYPTO_MIN) || (idx > SA_ID_CRYPTO_MAX))
			return AAL_E_OUTRANGE;
	} else {
		if ((idx < SA_ID_PLAINTEXT_MIN) || (idx > SA_ID_PLAINTEXT_MAX))
			return AAL_E_OUTRANGE;
	}

	spin_lock(&table->lock);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_sa_id_entry_get(void *entry, unsigned int *rslt_idx)
{
	int i;
	aal_table_t *table = &sa_id_table;
	aal_sa_id_entry_t *data = entry;
	unsigned int remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (data == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock(&table->lock);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock(&table->lock);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock(&table->lock);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock(&table->lock);

	return AAL_E_ENTRYNOTFOUND;
}

static int aal_sa_id_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table = &sa_id_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

        spin_lock(&table->lock);
        memcpy(entry, table->entry[idx].data, table->entry_size);
        spin_unlock(&table->lock);

	return AAL_E_OK;
}

static int aal_sa_id_table_flush(void)
{
	int i;
	aal_table_t *table = &sa_id_table;

	for (i = 0; i < table->max_entry; i++)
		aal_sa_id_entry_del(i);

	table->used_entry = 0;
	table->curr_ptr = 0;

	return AAL_E_OK;
}

static int aal_sa_id_get_avail_count(void)
{
	return generic_get_avail_count(&sa_id_table);
}

static void aal_sa_id_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &sa_id_table;
	aal_sa_id_entry_t entry;
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

	printk(KERN_INFO "|===== SA ID Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = aal_sa_id_entry_get_by_idx(i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index(sa_id) %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index(sa_id) %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d, direction(1) %d, is_encrypted(1) %d, tunnel_type %d\n",
				entry.valid, entry.direction, entry.is_encrypted, entry.tunnel_type);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t sa_id_table = {
	.table_id = AAL_TABLE_SA_ID,
	.table_name = "Security Association ID (SA ID)",
	.max_entry = SA_ID_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_sa_id_entry_t),
	.access_reg = 0,
	.data_reg_num = 0,
	.op = {
		.entry_add = aal_sa_id_entry_add,
		.entry_add_by_idx = aal_sa_id_entry_add_by_idx,
		.entry_del = aal_sa_id_entry_del,
		.entry_set = aal_sa_id_entry_set,
		.entry_get = aal_sa_id_entry_get,
		.entry_get_by_idx = aal_sa_id_entry_get_by_idx,
		.table_flush = aal_sa_id_table_flush,
		.get_avail_count = aal_sa_id_get_avail_count,
		.entry_print = aal_sa_id_entry_print,
	},
};

int __aal_sa_id_tbl_init(void)
{
	int ret;

	spin_lock_init(&sa_id_table.lock);

	ret = __aal_table_register(AAL_TABLE_SA_ID, &sa_id_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __aal_sa_id_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_SA_ID);
}

