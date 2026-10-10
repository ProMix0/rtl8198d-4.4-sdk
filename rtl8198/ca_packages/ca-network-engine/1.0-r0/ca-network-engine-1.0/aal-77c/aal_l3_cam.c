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
 */
/*
 * aal_l3_cam.c: Hardware Abstraction Layer for L3 PP CAM to access hardware regsiters
 */

#include <generated/ca_ne_autoconf.h>
#include "aal_common.h"
#include "aal_table.h"
#include "aal_l3_cam.h"
#include "ca_mut.h"

#define TABLE_TRY_TIMEOUT	1000
#define L3_CAM_DATA_REG_NUM	5

static aal_table_t l3_cam_dport_tbl;
static aal_table_t l3_cam_sport_tbl;
static aal_table_t l3_cam_ethertype_tbl;
static aal_table_t l3_cam_mac_da_tbl;
static aal_table_t l3_cam_http_msgs_tbl;
static aal_table_t l3_cam_llc_tbl;
static aal_table_t l3_cam_pkt_len_tbl;
static aal_table_t l3_cam_ppp_code_tbl;
static aal_table_t l3_cam_ppp_prot_tbl;
static aal_table_t l3_cam_spcl_hdr_tbl;
static aal_table_t l3_cam_ip_addr_tbl;

static spinlock_t l3_cam_lock;		/* protecting AAL tables only */

static int develop_test;

static inline unsigned int cam_hw_idx_covert(ca_device_id_t device_id, aal_table_t *table, unsigned int idx)
{
	return (((table->table_id - AAL_TABLE_L3_CAM_DPORT) << 16) | idx);
}

/*************************************************************************
 * internal AAL function for hardware register
 * This should be static because only general functions need them (?)
 *************************************************************************/

/* Clear the entry with given index and table */
static int cam_hw_entry_clear(aal_table_t *table, unsigned int idx)
{
	int i;
	uint32_t val = 0;
	ca_device_id_t device_id;

	if (!table || !table->entry)
		return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;

	device_id = (table->table_id < __AAL_TABLE_8279_START__) ? 0 : 1;

	/* write the value to the specific data registers */
	for (i = 0; i < table->data_reg_num ; i++)
		ca_ne_reg_write(device_id, 0, table->access_reg + ((table->data_reg_num - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= cam_hw_idx_covert(device_id, table, idx);
	ca_ne_reg_write(device_id, val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
} /* cam_hw_entry_clear */

/* Set the value to HW table entry with given index and table type */
static int cam_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	int i;
	uint32_t val = 0;
	unsigned int *ptr;
	ca_device_id_t device_id;

	if (!table || !table->entry)
		return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;
	if (!table->entry[idx].data)
		return AAL_E_NULLPTR;

	device_id = (table->table_id < __AAL_TABLE_8279_START__) ? 0 : 1;

	ptr = table->entry[idx].data;
	/* write the value to the specific data registers */
	for (i = 0; i < table->data_reg_num ; i++)
		ca_ne_reg_write(device_id, ptr[i], table->access_reg + ((table->data_reg_num - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= cam_hw_idx_covert(device_id, table, idx);
	ca_ne_reg_write(device_id, val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
} /* cam_hw_entry_set */

/* Get the value from HW table entry with given index and table type */
int cam_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	int i;
	uint32_t val = 0;
	unsigned int *ptr;
	ca_device_id_t device_id;

	if (!table || !table->entry)
		return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;
	if (!data)
		return AAL_E_NULLPTR;

	device_id = (table->table_id < __AAL_TABLE_8279_START__) ? 0 : 1;

	ptr = data;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= cam_hw_idx_covert(device_id, table, idx);
	ca_ne_reg_write(device_id, val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	for (i = 0; i < table->data_reg_num; i++)
		ptr[i] = ca_ne_reg_read(device_id, table->access_reg + ((table->data_reg_num - i) << 2));

	return AAL_E_OK;
}

int cam_hw_table_flush(aal_table_t *table)
{
	int i, ret;

	if (!table || !table->entry)
		return AAL_E_TBLNOTEXIST;

	for (i = 0; i < table->max_entry; i++) {
		ret = cam_hw_entry_clear(table, i);
		if (ret != AAL_E_OK)
			return ret;
	}

	return AAL_E_OK;
} /* cam_hw_table_flush */

/*************************************************************************
 * internal AAL function for cam table entry operation
 * called/hooked by other AAL modules if they don't need specific ones
 *************************************************************************/
int cam_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	bool f_found = false;
	int ret;

	if ((!entry) || (!rslt_idx))
		return AAL_E_NULLPTR;

	if (!table->op.entry_get)
		return AAL_E_OPNOTSUPP;

	ret = table->op.entry_get(entry, rslt_idx);
	if (ret == AAL_E_OK) {
		/* already existed */
		atomic_inc(&table->entry[*rslt_idx].refcnt);
		return AAL_E_OK;
	} else if (ret == AAL_E_ENTRYNOTFOUND) {
		if (table->max_entry == table->used_entry)
			return AAL_E_TBLFULL;

		spin_lock_irqsave(&l3_cam_lock, flags);

		*rslt_idx = 0;
		do {
			if (*rslt_idx == table->max_entry) {
				spin_unlock_irqrestore(&l3_cam_lock, flags);
				return AAL_E_TBLFULL;
			}

			/* search an empty entry */

			if (!table->entry[*rslt_idx].data) {
				table->entry[*rslt_idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
				if (!table->entry[*rslt_idx].data) {
					spin_unlock_irqrestore(&l3_cam_lock, flags);
					return AAL_E_NOMEM;
				}

				memcpy(table->entry[*rslt_idx].data, entry, table->entry_size);
				atomic_set(&table->entry[*rslt_idx].refcnt, 1);
				table->used_entry++;

				/* set to HW */
				ret = cam_hw_entry_set(table, *rslt_idx);
				break;
			}

			(*rslt_idx)++;

		} while (!f_found);

		spin_unlock_irqrestore(&l3_cam_lock, flags);
	}
	return ret;
} /* cam_entry_add */

static int cam_entry_add_by_idx(aal_table_t *table, void *entry, unsigned int idx)
{
	unsigned long flags;
	int ret;
	void *tmp_entry;

	if (!entry)
		return AAL_E_NULLPTR;

	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;

	if (!table->op.entry_get_by_idx)
		return AAL_E_OPNOTSUPP;

	tmp_entry = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);

	ret = table->op.entry_get_by_idx(idx, tmp_entry);
	if (ret == AAL_E_OK) {
		/* there is an entry at same index */

		if (memcmp(entry, tmp_entry, table->entry_size) == 0) {
			/* same entry, increase the reference count */
			atomic_inc(&table->entry[idx].refcnt);
			ca_free_trace(tmp_entry);
		} else {
			/* different entry, must keep original */
			ca_free_trace(tmp_entry);
			return AAL_E_ENTRY_USED;
		}

	} else {
		/* entry at index is not used */
		ca_free_trace(tmp_entry);

		spin_lock_irqsave(&l3_cam_lock, flags);

		table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
		if (!table->entry[idx].data) {
			spin_unlock_irqrestore(&l3_cam_lock, flags);
			return AAL_E_NOMEM;
		}

		memcpy(table->entry[idx].data, entry, table->entry_size);
		atomic_set(&table->entry[idx].refcnt, 1);
		table->used_entry++;

		/* set to HW */
		ret = cam_hw_entry_set(table, idx);

		spin_unlock_irqrestore(&l3_cam_lock, flags);
	}

	return AAL_E_OK;
} /* cam_entry_add_by_idx */

int cam_entry_del(aal_table_t *table, unsigned int idx)
{
	aal_entry_t *p_entry;
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;

	p_entry = sw_entry_get(table, idx);
	if (!p_entry)
		return AAL_E_NULLPTR;

	spin_lock_irqsave(&l3_cam_lock, flags);

	if (atomic_dec_and_test(&p_entry->refcnt)) {
		ca_free_trace(p_entry->data);
		p_entry->data = NULL;
		table->used_entry--;

		ret = cam_hw_entry_clear(table, idx);
		if (ret != AAL_E_OK) {
			spin_unlock_irqrestore(&l3_cam_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&l3_cam_lock, flags);

	return AAL_E_OK;
} /* cam__entry_del */

int cam_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;

	if (!entry)
		return AAL_E_NULLPTR;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;
	if (!table->entry[idx].data)
		return AAL_E_NULLPTR;

	spin_lock_irqsave(&l3_cam_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	ret = cam_hw_entry_set(table, idx);
	if (ret != AAL_E_OK) {
		spin_unlock_irqrestore(&l3_cam_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&l3_cam_lock, flags);

	return ret;
} /* cam_entry_set */

static int cam_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_entry_t *p_entry;

	if (!entry)
		return AAL_E_NULLPTR;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;

	p_entry = sw_entry_get(table, idx);
	if (!p_entry)
		return AAL_E_NULLPTR;

	spin_lock_irqsave(&l3_cam_lock, flags);
	memcpy(entry, p_entry->data, table->entry_size);
	spin_unlock_irqrestore(&l3_cam_lock, flags);

	return AAL_E_OK;
} /* generic_entry_get_by_idx */

int cam_table_flush(aal_table_t *table)
{
	int i;
	unsigned long flags;

	if (!table->op.entry_del)
		return AAL_E_OPNOTSUPP;

	for (i = 0; i < table->max_entry; i++)
		table->op.entry_del(i);

	spin_lock_irqsave(&l3_cam_lock, flags);
	table->used_entry = 0;
	table->curr_ptr = 0;
	cam_hw_table_flush(table);
	spin_unlock_irqrestore(&l3_cam_lock, flags);

	return AAL_E_OK;
} /* generic_table_flush */

int cam_get_avail_count(aal_table_t *table)
{
	return (table->max_entry - table->used_entry);
} /* generic_get_avail_count */

/*************************************************************************
 * internal AAL function for CAM table operation
 *************************************************************************/
static int l3_cam_dport_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_dport_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_dport_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_dport_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_dport_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_dport_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_dport_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_dport_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_dport_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_dport_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "dport_low(16) = 0x%04X\n", entry.dport_low);
			printk(KERN_INFO "dport_hi(16) = 0x%04X\n", entry.dport_hi);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_dport_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	__l3_cam_dport_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_dport_tbl = {
	.table_id = AAL_TABLE_L3_CAM_DPORT,
	.table_name = "L3 CAM DPORT",
	.max_entry = L3_CAM_DPORT_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_dport_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_dport_tbl_entry_add,
		.entry_add_by_idx = l3_cam_dport_tbl_entry_add_by_idx,
		.entry_del = l3_cam_dport_tbl_entry_del,
		.entry_set = l3_cam_dport_tbl_entry_set,
		.entry_get = l3_cam_dport_tbl_entry_get,
		.entry_get_by_idx = l3_cam_dport_tbl_entry_get_by_idx,
		.table_flush = l3_cam_dport_tbl_table_flush,
		.get_avail_count = l3_cam_dport_tbl_get_avail_count,
		.entry_print = l3_cam_dport_tbl_entry_print,
	},
};

static int l3_cam_sport_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_sport_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_sport_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_sport_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_sport_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_sport_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_sport_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_sport_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_sport_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_sport_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "sport_low(16) = 0x%04X\n", entry.sport_low);
			printk(KERN_INFO "sport_hi(16) = 0x%04X\n", entry.sport_hi);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_sport_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	__l3_cam_sport_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_sport_tbl = {
	.table_id = AAL_TABLE_L3_CAM_SPORT,
	.table_name = "L3 CAM SPORT",
	.max_entry = L3_CAM_SPORT_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_sport_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_sport_tbl_entry_add,
		.entry_add_by_idx = l3_cam_sport_tbl_entry_add_by_idx,
		.entry_del = l3_cam_sport_tbl_entry_del,
		.entry_set = l3_cam_sport_tbl_entry_set,
		.entry_get = l3_cam_sport_tbl_entry_get,
		.entry_get_by_idx = l3_cam_sport_tbl_entry_get_by_idx,
		.table_flush = l3_cam_sport_tbl_table_flush,
		.get_avail_count = l3_cam_sport_tbl_get_avail_count,
		.entry_print = l3_cam_sport_tbl_entry_print,
	},
};

static int l3_cam_ethertype_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_ethertype_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_ethertype_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_ethertype_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_ethertype_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_ethertype_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_ethertype_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_ethertype_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_ethertype_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_ethertype_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "ethertype(16) = 0x%04X\n", entry.ethertype);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_ethertype_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	__l3_cam_ethertype_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_ethertype_tbl = {
	.table_id = AAL_TABLE_L3_CAM_ETHERTYPE,
	.table_name = "L3 CAM Ethertype",
	.max_entry = L3_CAM_ETHERTYPE_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_ethertype_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_ethertype_tbl_entry_add,
		.entry_add_by_idx = l3_cam_ethertype_tbl_entry_add_by_idx,
		.entry_del = l3_cam_ethertype_tbl_entry_del,
		.entry_set = l3_cam_ethertype_tbl_entry_set,
		.entry_get = l3_cam_ethertype_tbl_entry_get,
		.entry_get_by_idx = l3_cam_ethertype_tbl_entry_get_by_idx,
		.table_flush = l3_cam_ethertype_tbl_table_flush,
		.get_avail_count = l3_cam_ethertype_tbl_get_avail_count,
		.entry_print = l3_cam_ethertype_tbl_entry_print,
	},
};

static int l3_cam_mac_da_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_mac_da_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_mac_da_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_mac_da_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_mac_da_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_mac_da_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_mac_da_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_mac_da_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_mac_da_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_mac_da_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "mac(48) = %02X:%02X:%02X:%02X:%02X:%02X\n", entry.mac_0, \
				entry.mac_1, entry.mac_2, entry.mac_3, entry.mac_4, \
				entry.mac_5);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_mac_da_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;

	__l3_cam_mac_da_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_mac_da_tbl = {
	.table_id = AAL_TABLE_L3_CAM_MAC_DA,
	.table_name = "L3 CAM MAC_DA",
	.max_entry = L3_CAM_MAC_DA_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_mac_da_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_mac_da_tbl_entry_add,
		.entry_add_by_idx = l3_cam_mac_da_tbl_entry_add_by_idx,
		.entry_del = l3_cam_mac_da_tbl_entry_del,
		.entry_set = l3_cam_mac_da_tbl_entry_set,
		.entry_get = l3_cam_mac_da_tbl_entry_get,
		.entry_get_by_idx = l3_cam_mac_da_tbl_entry_get_by_idx,
		.table_flush = l3_cam_mac_da_tbl_table_flush,
		.get_avail_count = l3_cam_mac_da_tbl_get_avail_count,
		.entry_print = l3_cam_mac_da_tbl_entry_print,
	},
};

static int l3_cam_http_msgs_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_http_msgs_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_http_msgs_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_http_msgs_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_http_msgs_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_http_msgs_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_http_msgs_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_http_msgs_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_http_msgs_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_http_msgs_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "http_msg(64) = 0x%016llX\n", entry.http_msg);
			printk(KERN_INFO "byte_en(8) = 0x%02X\n", entry.byte_en);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_http_msgs_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;

	__l3_cam_http_msgs_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_http_msgs_tbl = {
	.table_id = AAL_TABLE_L3_CAM_HTTP_MSGS,
	.table_name = "L3 CAM HTTP Msgs",
	.max_entry = L3_CAM_HTTP_MSGS_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_http_msgs_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_http_msgs_tbl_entry_add,
		.entry_add_by_idx = l3_cam_http_msgs_tbl_entry_add_by_idx,
		.entry_del = l3_cam_http_msgs_tbl_entry_del,
		.entry_set = l3_cam_http_msgs_tbl_entry_set,
		.entry_get = l3_cam_http_msgs_tbl_entry_get,
		.entry_get_by_idx = l3_cam_http_msgs_tbl_entry_get_by_idx,
		.table_flush = l3_cam_http_msgs_tbl_table_flush,
		.get_avail_count = l3_cam_http_msgs_tbl_get_avail_count,
		.entry_print = l3_cam_http_msgs_tbl_entry_print,
	},
};

static int l3_cam_llc_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_llc_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_llc_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_llc_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_llc_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_llc_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_llc_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_llc_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_llc_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_llc_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "pkt_len_low(24) = 0x%06X\n", entry.llc);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_llc_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;

	__l3_cam_llc_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_llc_tbl = {
	.table_id = AAL_TABLE_L3_CAM_LLC,
	.table_name = "L3 CAM LLC",
	.max_entry = L3_CAM_LLC_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_llc_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_llc_tbl_entry_add,
		.entry_add_by_idx = l3_cam_llc_tbl_entry_add_by_idx,
		.entry_del = l3_cam_llc_tbl_entry_del,
		.entry_set = l3_cam_llc_tbl_entry_set,
		.entry_get = l3_cam_llc_tbl_entry_get,
		.entry_get_by_idx = l3_cam_llc_tbl_entry_get_by_idx,
		.table_flush = l3_cam_llc_tbl_table_flush,
		.get_avail_count = l3_cam_llc_tbl_get_avail_count,
		.entry_print = l3_cam_llc_tbl_entry_print,
	},
};

static int l3_cam_pkt_len_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_pkt_len_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_pkt_len_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_pkt_len_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_pkt_len_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_pkt_len_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_pkt_len_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_pkt_len_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_pkt_len_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_pkt_len_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "pkt_len_low(14) = 0x%04X\n", entry.pkt_len_low);
			printk(KERN_INFO "pkt_len_hi(14) = 0x%04X\n", entry.pkt_len_hi);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_pkt_len_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	__l3_cam_pkt_len_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_pkt_len_tbl = {
	.table_id = AAL_TABLE_L3_CAM_PKT_LEN,
	.table_name = "L3 CAM PKT LEN",
	.max_entry = L3_CAM_PKT_LEN_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_pkt_len_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_pkt_len_tbl_entry_add,
		.entry_add_by_idx = l3_cam_pkt_len_tbl_entry_add_by_idx,
		.entry_del = l3_cam_pkt_len_tbl_entry_del,
		.entry_set = l3_cam_pkt_len_tbl_entry_set,
		.entry_get = l3_cam_pkt_len_tbl_entry_get,
		.entry_get_by_idx = l3_cam_pkt_len_tbl_entry_get_by_idx,
		.table_flush = l3_cam_pkt_len_tbl_table_flush,
		.get_avail_count = l3_cam_pkt_len_tbl_get_avail_count,
		.entry_print = l3_cam_pkt_len_tbl_entry_print,
	},
};

static int l3_cam_ppp_code_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_ppp_code_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_ppp_code_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_ppp_code_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_ppp_code_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_ppp_code_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_ppp_code_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_ppp_code_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_ppp_code_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_ppp_code_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "ppp_code(8) = 0x%02X\n", entry.ppp_code);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_ppp_code_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	__l3_cam_ppp_code_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_ppp_code_tbl = {
	.table_id = AAL_TABLE_L3_CAM_PPP_CODE,
	.table_name = "L3 CAM PPP Code",
	.max_entry = L3_CAM_PPP_CODE_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_ppp_code_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_ppp_code_tbl_entry_add,
		.entry_add_by_idx = l3_cam_ppp_code_tbl_entry_add_by_idx,
		.entry_del = l3_cam_ppp_code_tbl_entry_del,
		.entry_set = l3_cam_ppp_code_tbl_entry_set,
		.entry_get = l3_cam_ppp_code_tbl_entry_get,
		.entry_get_by_idx = l3_cam_ppp_code_tbl_entry_get_by_idx,
		.table_flush = l3_cam_ppp_code_tbl_table_flush,
		.get_avail_count = l3_cam_ppp_code_tbl_get_avail_count,
		.entry_print = l3_cam_ppp_code_tbl_entry_print,
	},
};

static int l3_cam_ppp_prot_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_ppp_prot_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_ppp_prot_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_ppp_prot_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_ppp_prot_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_ppp_prot_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_ppp_prot_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_ppp_prot_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_ppp_prot_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_ppp_prot_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "ppp_prot(16) = 0x%04X\n", entry.ppp_prot);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_ppp_prot_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	__l3_cam_ppp_prot_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_ppp_prot_tbl = {
	.table_id = AAL_TABLE_L3_CAM_PPP_PROT,
	.table_name = "L3 CAM PPP Protocol",
	.max_entry = L3_CAM_PPP_PROT_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_ppp_prot_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_ppp_prot_tbl_entry_add,
		.entry_add_by_idx = l3_cam_ppp_prot_tbl_entry_add_by_idx,
		.entry_del = l3_cam_ppp_prot_tbl_entry_del,
		.entry_set = l3_cam_ppp_prot_tbl_entry_set,
		.entry_get = l3_cam_ppp_prot_tbl_entry_get,
		.entry_get_by_idx = l3_cam_ppp_prot_tbl_entry_get_by_idx,
		.table_flush = l3_cam_ppp_prot_tbl_table_flush,
		.get_avail_count = l3_cam_ppp_prot_tbl_get_avail_count,
		.entry_print = l3_cam_ppp_prot_tbl_entry_print,
	},
};

static int l3_cam_spcl_hdr_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_spcl_hdr_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_spcl_hdr_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_spcl_hdr_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_spcl_hdr_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_spcl_hdr_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_spcl_hdr_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_spcl_hdr_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_spcl_hdr_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_spcl_hdr_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "key(16) = 0x%04X\n", entry.key);
			printk(KERN_INFO "mask(16) = 0x%04X\n", entry.mask);
			printk(KERN_INFO "ms_vld(1) = %d\n", entry.ms_vld);
			printk(KERN_INFO "type(3) = %d\n", entry.type);
			printk(KERN_INFO "vld(1) = %d\n", entry.vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_spcl_hdr_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	__l3_cam_spcl_hdr_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_spcl_hdr_tbl = {
	.table_id = AAL_TABLE_L3_CAM_SPCL_HDR,
	.table_name = "L3 CAM Special Header",
	.max_entry = L3_CAM_SPCL_HDR_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_spcl_hdr_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_spcl_hdr_tbl_entry_add,
		.entry_add_by_idx = l3_cam_spcl_hdr_tbl_entry_add_by_idx,
		.entry_del = l3_cam_spcl_hdr_tbl_entry_del,
		.entry_set = l3_cam_spcl_hdr_tbl_entry_set,
		.entry_get = l3_cam_spcl_hdr_tbl_entry_get,
		.entry_get_by_idx = l3_cam_spcl_hdr_tbl_entry_get_by_idx,
		.table_flush = l3_cam_spcl_hdr_tbl_table_flush,
		.get_avail_count = l3_cam_spcl_hdr_tbl_get_avail_count,
		.entry_print = l3_cam_spcl_hdr_tbl_entry_print,
	},
};

static int l3_cam_ip_addr_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_entry_add(table_p, entry, rslt_idx);
}

static int l3_cam_ip_addr_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_entry_add_by_idx(table_p, entry, idx);
}

static int l3_cam_ip_addr_tbl_entry_del(unsigned int idx)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_entry_del(table_p, idx);
}

static int l3_cam_ip_addr_tbl_entry_set(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_entry_set(table_p, idx, entry);
}

static int l3_cam_ip_addr_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return generic_entry_get(table_p, entry, rslt_idx);
}

static int l3_cam_ip_addr_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_entry_get_by_idx(table_p, idx, entry);
}

static int l3_cam_ip_addr_tbl_table_flush(void)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_table_flush(table_p);
}

static int l3_cam_ip_addr_tbl_get_avail_count(void)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return generic_get_avail_count(table_p);
}

static void __l3_cam_ip_addr_tbl_entry_print(aal_table_t *table_p, unsigned int start_idx, int count)
{
	l3_cam_ip_addr_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table_p->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table_p->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table_p->max_entry))
		end_idx = table_p->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== %s ==================================================|\n", table_p->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache)
			ret = cam_entry_get_by_idx(table_p, i, &entry);
		else
			ret = cam_hw_entry_get(table_p, i, &entry);

		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table_p->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "ip_addr_0(32) = 0x%08X\n", entry.ip_addr_0);
			printk(KERN_INFO "ip_addr_1(32) = 0x%08X\n", entry.ip_addr_1);
			printk(KERN_INFO "ip_addr_2(32) = 0x%08X\n", entry.ip_addr_2);
			printk(KERN_INFO "ip_addr_3(32) = 0x%08X\n", entry.ip_addr_3);
			printk(KERN_INFO "ip_mask(8) = 0x%02X\n", entry.ip_mask);
			printk(KERN_INFO "ipv4_flg(1) = %d\n", entry.ipv4_flg);
			printk(KERN_INFO "left_mask_flg(1) = %d\n", entry.left_mask_flg);
			printk(KERN_INFO "ip_sa_vld(1) = %d\n", entry.ip_sa_vld);
			printk(KERN_INFO "ip_da_vld(1) = %d\n", entry.ip_da_vld);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static void l3_cam_ip_addr_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	__l3_cam_ip_addr_tbl_entry_print(table_p, start_idx, count);
}

static aal_table_t l3_cam_ip_addr_tbl = {
	.table_id = AAL_TABLE_L3_CAM_IP_ADDR,
	.table_name = "L3 CAM IP Addr",
	.max_entry = L3_CAM_IP_ADDR_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cam_ip_addr_tbl_entry_t),
	.access_reg = L3FE_PP_FIELD_CAM_ACCESS,
	.data_reg_num = L3_CAM_DATA_REG_NUM,
	.op = {
		.entry_add = l3_cam_ip_addr_tbl_entry_add,
		.entry_add_by_idx = l3_cam_ip_addr_tbl_entry_add_by_idx,
		.entry_del = l3_cam_ip_addr_tbl_entry_del,
		.entry_set = l3_cam_ip_addr_tbl_entry_set,
		.entry_get = l3_cam_ip_addr_tbl_entry_get,
		.entry_get_by_idx = l3_cam_ip_addr_tbl_entry_get_by_idx,
		.table_flush = l3_cam_ip_addr_tbl_table_flush,
		.get_avail_count = l3_cam_ip_addr_tbl_get_avail_count,
		.entry_print = l3_cam_ip_addr_tbl_entry_print,
	},
};

static void l3_cam_early_develop_dport_test(aal_table_t *table_p)
{
	l3_cam_dport_tbl_entry_t dport_entry;
	unsigned int idx;
	int ret;

	dport_entry.dport_low = 0x3210;
	dport_entry.dport_hi = 0x7654;
	dport_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&dport_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	dport_entry.dport_low = ~dport_entry.dport_low;
	dport_entry.dport_hi = ~dport_entry.dport_hi;
	dport_entry.vld = ~dport_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&dport_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_sport_test(aal_table_t *table_p)
{
	l3_cam_sport_tbl_entry_t sport_entry;
	unsigned int idx;
	int ret;

	sport_entry.sport_low = 0xCDEF;
	sport_entry.sport_hi = 0x89AB;
	sport_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&sport_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	sport_entry.sport_low = ~sport_entry.sport_low;
	sport_entry.sport_hi = ~sport_entry.sport_hi;
	sport_entry.vld = ~sport_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&sport_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_ethertype_test(aal_table_t *table_p)
{
	l3_cam_ethertype_tbl_entry_t ethertype_entry;
	unsigned int idx;
	int ret;

	ethertype_entry.ethertype = 0xA5A5;
	ethertype_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&ethertype_entry, idx);
	printk(KERN_INFO "l3_cam_ethertype_tbl_entry_add_by_idx(%d) ret %d\n", idx, ret);

	ethertype_entry.ethertype = ~ethertype_entry.ethertype;
	ethertype_entry.vld = ~ethertype_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&ethertype_entry, idx);
	printk(KERN_INFO "l3_cam_ethertype_tbl_entry_add_by_idx(%d) ret %d\n", idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_mac_da_test(aal_table_t *table_p)
{
	l3_cam_mac_da_tbl_entry_t mac_entry;
	unsigned int idx;
	int ret;

	mac_entry.mac_0 = 0x00;
	mac_entry.mac_1 = 0x13;
	mac_entry.mac_2 = 0x25;
	mac_entry.mac_3 = 0x01;
	mac_entry.mac_4 = 0x02;
	mac_entry.mac_5 = 0x03;
	mac_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&mac_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	mac_entry.mac_5 = ~mac_entry.mac_5;
	mac_entry.mac_4 = ~mac_entry.mac_4;
	mac_entry.mac_3 = ~mac_entry.mac_3;
	mac_entry.mac_2 = ~mac_entry.mac_2;
	mac_entry.mac_1 = ~mac_entry.mac_1;
	mac_entry.mac_0 = ~mac_entry.mac_0;
	mac_entry.vld = ~mac_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&mac_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_http_msgs_test(aal_table_t *table_p)
{
	l3_cam_http_msgs_tbl_entry_t http_msg_entry;
	unsigned int idx;
	int ret;

	http_msg_entry.http_msg = 0x0123456789ABCDEF;
	http_msg_entry.byte_en = 0x7F;
	http_msg_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&http_msg_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	http_msg_entry.http_msg = ~http_msg_entry.http_msg;
	http_msg_entry.byte_en = ~http_msg_entry.byte_en;
	http_msg_entry.vld = ~http_msg_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&http_msg_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);

}

static void l3_cam_early_develop_llc_test(aal_table_t *table_p)
{
	l3_cam_llc_tbl_entry_t llc_entry;
	unsigned int idx;
	int ret;

	llc_entry.llc = 0x020406;
	llc_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&llc_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	llc_entry.llc = ~llc_entry.llc;
	llc_entry.vld = ~llc_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&llc_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_pkt_len_test(aal_table_t *table_p)
{
	l3_cam_pkt_len_tbl_entry_t pkt_len_entry;
	unsigned int idx;
	int ret;

	pkt_len_entry.pkt_len_low = 0x3ABC;
	pkt_len_entry.pkt_len_hi = 0x3210;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&pkt_len_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	pkt_len_entry.pkt_len_low = ~pkt_len_entry.pkt_len_low;
	pkt_len_entry.pkt_len_hi = ~pkt_len_entry.pkt_len_hi;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&pkt_len_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_ppp_code_test(aal_table_t *table_p)
{
	l3_cam_ppp_code_tbl_entry_t ppp_code_entry;
	unsigned int idx;
	int ret;

	ppp_code_entry.ppp_code = 0xFF;
	ppp_code_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&ppp_code_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	ppp_code_entry.ppp_code = ~ppp_code_entry.ppp_code;
	ppp_code_entry.vld = ~ppp_code_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&ppp_code_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_ppp_prot_test(aal_table_t *table_p)
{
	l3_cam_ppp_prot_tbl_entry_t ppp_prot_entry;
	unsigned int idx;
	int ret;

	ppp_prot_entry.ppp_prot = 0x1357;
	ppp_prot_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&ppp_prot_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	ppp_prot_entry.ppp_prot = ~ppp_prot_entry.ppp_prot;
	ppp_prot_entry.vld = ~ppp_prot_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&ppp_prot_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_spcl_hdr_test(aal_table_t *table_p)
{
	l3_cam_spcl_hdr_tbl_entry_t spcl_hdr_entry;
	unsigned int idx;
	int ret;

	spcl_hdr_entry.key = 0x2468;
	spcl_hdr_entry.mask = 0xFFFF;
	spcl_hdr_entry.ms_vld = 1;
	spcl_hdr_entry.type = 2;
	spcl_hdr_entry.vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&spcl_hdr_entry, idx);
	printk(KERN_INFO "l3_cam_spcl_hdr_tbl_entry_add_by_idx(%d) ret %d\n", idx, ret);

	spcl_hdr_entry.key = ~spcl_hdr_entry.key;
	spcl_hdr_entry.mask = ~spcl_hdr_entry.mask;
	spcl_hdr_entry.ms_vld = ~spcl_hdr_entry.ms_vld;
	spcl_hdr_entry.type = ~spcl_hdr_entry.type;
	spcl_hdr_entry.vld = ~spcl_hdr_entry.vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&spcl_hdr_entry, idx);
	printk(KERN_INFO "l3_cam_spcl_hdr_tbl_entry_add_by_idx(%d) ret %d\n", idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_ip_addr_test(aal_table_t *table_p)
{
	l3_cam_ip_addr_tbl_entry_t ip_addr_entry;
	unsigned int idx;
	int ret;

	ip_addr_entry.ip_addr_0 = 0xA5A5A5A5;
	ip_addr_entry.ip_addr_1 = 0x5A5A5A5A;
	ip_addr_entry.ip_addr_2 = 0x12345678;
	ip_addr_entry.ip_addr_3 = 0x0FEDCBA9;
	ip_addr_entry.ip_mask = 0xA5;
	ip_addr_entry.left_mask_flg = 0;
	ip_addr_entry.ipv4_flg = 1;
	ip_addr_entry.ip_sa_vld = 0;
	ip_addr_entry.ip_da_vld = 1;
	idx = 0;
	ret = table_p->op.entry_add_by_idx(&ip_addr_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	ip_addr_entry.ip_addr_0 = ~ip_addr_entry.ip_addr_0;
	ip_addr_entry.ip_addr_1 = ~ip_addr_entry.ip_addr_1;
	ip_addr_entry.ip_addr_2 = ~ip_addr_entry.ip_addr_2;
	ip_addr_entry.ip_addr_3 = ~ip_addr_entry.ip_addr_3;
	ip_addr_entry.ip_mask = ~ip_addr_entry.ip_mask;
	ip_addr_entry.left_mask_flg = ~ip_addr_entry.left_mask_flg;
	ip_addr_entry.ipv4_flg = ~ip_addr_entry.ipv4_flg;
	ip_addr_entry.ip_sa_vld = ~ip_addr_entry.ip_sa_vld;
	ip_addr_entry.ip_da_vld = ~ip_addr_entry.ip_da_vld;
	idx = table_p->max_entry - 1;
	ret = table_p->op.entry_add_by_idx(&ip_addr_entry, idx);
	printk("%s: entry_add_by_idx(%d) ret %d\n", table_p->table_name, idx, ret);

	table_p->op.entry_print(0, 2);
	table_p->op.entry_print(table_p->max_entry - 1, 1);

	table_p->op.entry_del(0);
	table_p->op.entry_del(table_p->max_entry - 1);
}

static void l3_cam_early_develop_test(void)
{
	l3_cam_early_develop_dport_test(&l3_cam_dport_tbl);
	l3_cam_early_develop_sport_test(&l3_cam_sport_tbl);
	l3_cam_early_develop_ethertype_test(&l3_cam_ethertype_tbl);
	l3_cam_early_develop_mac_da_test(&l3_cam_mac_da_tbl);
	l3_cam_early_develop_http_msgs_test(&l3_cam_http_msgs_tbl);
	l3_cam_early_develop_llc_test(&l3_cam_llc_tbl);
	l3_cam_early_develop_pkt_len_test(&l3_cam_pkt_len_tbl);
	l3_cam_early_develop_ppp_code_test(&l3_cam_ppp_code_tbl);
	l3_cam_early_develop_ppp_prot_test(&l3_cam_ppp_prot_tbl);
	l3_cam_early_develop_spcl_hdr_test(&l3_cam_spcl_hdr_tbl);
	l3_cam_early_develop_ip_addr_test(&l3_cam_ip_addr_tbl);
}

int aal_l3_cam_mac_da_lookup(ca_uint8_t mac[6], unsigned int *idx)
{
	aal_entry_t *p_entry;
	l3_cam_mac_da_tbl_entry_t *mac_da_entry;
	int i;

	for (i = 0; i < L3_CAM_MAC_DA_TBL_ENTRY_MAX; i++) {
		p_entry = sw_entry_get(&l3_cam_mac_da_tbl, i);
		if (p_entry) {
			mac_da_entry = (l3_cam_mac_da_tbl_entry_t *)p_entry->data;
			if ((mac[0] == mac_da_entry->mac_0) &&
			    (mac[1] == mac_da_entry->mac_1) &&
			    (mac[2] == mac_da_entry->mac_2) &&
			    (mac[3] == mac_da_entry->mac_3) &&
			    (mac[4] == mac_da_entry->mac_4) &&
			    (mac[5] == mac_da_entry->mac_5) &&
			    (mac_da_entry->vld == 1)) {
				*idx = i;
				return AAL_E_OK;
			}
		}
	}

	*idx = (unsigned int)-1;
	return AAL_E_ENTRYNOTFOUND;
}

int aal_l3_cam_tbls_init(void)
{
	aal_table_t *table_p;
	int ret;

	spin_lock_init(&l3_cam_lock);

	table_p = &l3_cam_dport_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_sport_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_ethertype_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_mac_da_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_http_msgs_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_llc_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_pkt_len_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_ppp_code_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_ppp_prot_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_spcl_hdr_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	table_p = &l3_cam_ip_addr_tbl;
	ret = __aal_table_register(table_p->table_id, table_p);
	if (ret != AAL_E_OK)
		return ret;
	aal_table_flush(table_p->table_id);

	if (develop_test)
		l3_cam_early_develop_test();

	return AAL_E_OK;
}

void aal_l3_cam_tbls_exit(void)
{
	__aal_table_unregister(l3_cam_dport_tbl.table_id);
	__aal_table_unregister(l3_cam_sport_tbl.table_id);
	__aal_table_unregister(l3_cam_ethertype_tbl.table_id);
	__aal_table_unregister(l3_cam_mac_da_tbl.table_id);
	__aal_table_unregister(l3_cam_http_msgs_tbl.table_id);
	__aal_table_unregister(l3_cam_llc_tbl.table_id);
	__aal_table_unregister(l3_cam_pkt_len_tbl.table_id);
	__aal_table_unregister(l3_cam_ppp_code_tbl.table_id);
	__aal_table_unregister(l3_cam_ppp_prot_tbl.table_id);
	__aal_table_unregister(l3_cam_spcl_hdr_tbl.table_id);
	__aal_table_unregister(l3_cam_ip_addr_tbl.table_id);
}

/*******************************************************************************
 * APP CLI APIs
 ******************************************************************************/
ca_status_t aal_l3_cam_dport_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_dport_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_dport_get);

ca_status_t aal_l3_cam_dport_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_dport_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_dport_tbl;
	l3_cam_dport_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_dport_set);

ca_status_t aal_l3_cam_sport_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_sport_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_sport_get);

ca_status_t aal_l3_cam_sport_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_sport_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_sport_tbl;
	l3_cam_sport_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_sport_set);

ca_status_t aal_l3_cam_ethertype_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_ethertype_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ethertype_get);

ca_status_t aal_l3_cam_ethertype_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_ethertype_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ethertype_tbl;
	l3_cam_ethertype_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ethertype_set);

ca_status_t aal_l3_cam_mac_da_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_mac_da_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_mac_da_get);

ca_status_t aal_l3_cam_mac_da_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_mac_da_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_mac_da_tbl;
	l3_cam_mac_da_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_mac_da_set);

ca_status_t aal_l3_cam_http_msgs_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_http_msgs_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_http_msgs_get);

ca_status_t aal_l3_cam_http_msgs_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_http_msgs_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_http_msgs_tbl;
	l3_cam_http_msgs_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_http_msgs_set);

ca_status_t aal_l3_cam_llc_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_llc_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_llc_get);

ca_status_t aal_l3_cam_llc_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_llc_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_llc_tbl;
	l3_cam_llc_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_llc_set);

ca_status_t aal_l3_cam_pkt_len_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_pkt_len_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_pkt_len_get);

ca_status_t aal_l3_cam_pkt_len_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_pkt_len_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_pkt_len_tbl;
	l3_cam_pkt_len_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_pkt_len_set);

ca_status_t aal_l3_cam_ppp_code_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_ppp_code_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ppp_code_get);

ca_status_t aal_l3_cam_ppp_code_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_ppp_code_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ppp_code_tbl;
	l3_cam_ppp_code_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ppp_code_set);

ca_status_t aal_l3_cam_ppp_prot_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_ppp_prot_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ppp_prot_get);

ca_status_t aal_l3_cam_ppp_prot_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_ppp_prot_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ppp_prot_tbl;
	l3_cam_ppp_prot_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ppp_prot_set);

ca_status_t aal_l3_cam_spcl_hdr_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_spcl_hdr_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_spcl_hdr_get);

ca_status_t aal_l3_cam_spcl_hdr_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_spcl_hdr_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_spcl_hdr_tbl;
	l3_cam_spcl_hdr_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
		return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
		return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_spcl_hdr_set);

ca_status_t aal_l3_cam_ip_addr_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cam_ip_addr_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;

	return cam_hw_entry_get(table_p, entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ip_addr_get);

ca_status_t aal_l3_cam_ip_addr_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cam_ip_addr_tbl_entry_t *entry
)
{
	aal_table_t *table_p = &l3_cam_ip_addr_tbl;
	l3_cam_ip_addr_tbl_entry_t tmp_entry;

	if (AAL_E_OK != table_p->op.entry_get_by_idx(entry_idx, &tmp_entry))
	        return table_p->op.entry_add_by_idx(entry, entry_idx);
	else
	        return table_p->op.entry_set(entry_idx, entry);
}
EXPORT_SYMBOL(aal_l3_cam_ip_addr_set);

/* Set initial value in CAM table */
int aal_cam_init(void)
{
	int ret;
	l3_cam_ethertype_tbl_entry_t ethertype;
	l3_cam_ppp_prot_tbl_entry_t ppp_prot;
	ca_uint32_t rslt_idx;

	/* Ethertype 0x8864 (PPPoE Session) */
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8864;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &rslt_idx);
	if (ret != AAL_E_OK) {
		printk("%s: Fail to add Ethertype 0x8864 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	/* PPP Protocol 0x0021 (IPv4) */
	memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
	ppp_prot.vld = 1;
	ppp_prot.ppp_prot = 0x0021;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &rslt_idx);
	if (ret != AAL_E_OK) {
		printk("%s: Fail to add PPP Protocol into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	/* PPP Protocol 0x0057 (IPv6) */
	memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
	ppp_prot.vld = 1;
	ppp_prot.ppp_prot = 0x0057;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &rslt_idx);
	if (ret != AAL_E_OK) {
		printk("%s: Fail to add PPP Protocol into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	return AAL_E_OK;
}

#define L3FE_PP_TPID_NUM 4
static ca_int32_t aal_l3fe_pp_tpid_find(ca_uint32_t tpid, ca_uint8_t *index)
{
	L3FE_PP_TPID_0_t tpid_0;
	L3FE_PP_TPID_1_t tpid_1;

	tpid_0.wrd = CA_NE_REG_READ(L3FE_PP_TPID_0);
	tpid_1.wrd = CA_NE_REG_READ(L3FE_PP_TPID_1);

	if (tpid_0.bf.tpid_0 == tpid) {
		*index = 0;
		return AAL_E_OK;
	}
	if (tpid_0.bf.tpid_1 == tpid) {
		*index = 1;
		return AAL_E_OK;
	}
	if (tpid_1.bf.tpid_2 == tpid) {
		*index = 2;
		return AAL_E_OK;
	}
	if (tpid_1.bf.tpid_3 == tpid) {
		*index = 3;
		return AAL_E_OK;
	}
	return AAL_E_ENTRYNOTFOUND;
}

ca_int32_t aal_l3fe_pp_top_tpid_get(ca_uint32_t tpid, ca_uint8_t *index)
{
	L3FE_PP_TPID_CTRL_t tpid_ctrl;

	if (AAL_E_OK != aal_l3fe_pp_tpid_find(tpid, index))
		return AAL_E_ENTRYNOTFOUND;

	tpid_ctrl.wrd = CA_NE_REG_READ(L3FE_PP_TPID_CTRL);
	if ((tpid_ctrl.bf.top_vlan_en & (0x1 << *index)) == 0)
		return AAL_E_ENTRYNOTFOUND;
	*index += 1;
	return AAL_E_OK;
}

ca_int32_t aal_l3fe_pp_inner_tpid_get(ca_uint32_t tpid, ca_uint8_t *index)
{
	L3FE_PP_TPID_CTRL_t tpid_ctrl;

	if (AAL_E_OK != aal_l3fe_pp_tpid_find(tpid, index))
		return AAL_E_ENTRYNOTFOUND;

	tpid_ctrl.wrd = CA_NE_REG_READ(L3FE_PP_TPID_CTRL);
	if ((tpid_ctrl.bf.inner_vlan_en & (0x1 << *index)) == 0)
		return AAL_E_ENTRYNOTFOUND;
	*index += 1;
	return AAL_E_OK;
}

ca_int32_t aal_l3fe_pp_tpid_set(ca_uint32_t top_nums, ca_uint32_t *top_tpid, ca_uint32_t inner_nums, ca_uint32_t *inner_tpid)
{
	ca_uint8 i, j, k, find = 0, top_vlan_en=0, inner_vlan_en=0;
	ca_uint16_t tmp[L3FE_PP_TPID_NUM];
	L3FE_PP_TPID_0_t tpid_0;
	L3FE_PP_TPID_1_t tpid_1;
	L3FE_PP_TPID_CTRL_t tpid_ctrl;

	memset(tmp, 0, L3FE_PP_TPID_NUM*sizeof(ca_uint16_t));
	/* l3fe only supports 4 tpid */
	/* add top_tpid */
	for (i = 0, j = 0; i < top_nums; i++) {
		if (top_tpid[i] == 0)
			continue;
		if (j >= L3FE_PP_TPID_NUM)
			break;
		tmp[j] = (ca_uint16_t)top_tpid[i];
		top_vlan_en |= (0x1 << j);
		j++;
	}
	/* add extra inner tpid */
	for (i = 0; i < inner_nums; i++) {
		if (inner_tpid[i] == 0)
			continue;
		/* find existed tpid */
		find = 0;
		for (k = 0; k < j; k++) {
			if (inner_tpid[i] != tmp[k])
				continue;
			find = 1;
			break;
		}
		if (find) {
			inner_vlan_en |= (0x1 << k);
		} else {
			if (j >= L3FE_PP_TPID_NUM)
				continue;
			tmp[j] = (ca_uint16_t)inner_tpid[i];
			inner_vlan_en |= (0x1 << j);
			j++;
		}
	}

	/* set tpid entries and ctrl */
	tpid_0.bf.tpid_0 = tmp[0];
	tpid_0.bf.tpid_1 = tmp[1];
	tpid_1.bf.tpid_2 = tmp[2];
	tpid_1.bf.tpid_3 = tmp[3];
	tpid_ctrl.bf.top_vlan_en = top_vlan_en;
	tpid_ctrl.bf.inner_vlan_en = inner_vlan_en;
	CA_NE_REG_WRITE(tpid_0.wrd, L3FE_PP_TPID_0);
	CA_NE_REG_WRITE(tpid_1.wrd, L3FE_PP_TPID_1);
	CA_NE_REG_WRITE(tpid_ctrl.wrd, L3FE_PP_TPID_CTRL);

	return AAL_E_OK;
}

EXPORT_SYMBOL(aal_l3_cam_mac_da_lookup);

