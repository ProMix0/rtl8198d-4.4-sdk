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
 * aal_hashlite.c: Hardware Abstraction Layer for Hash-Lite Engine to access hardware regsiters
 *
 * Flow to add hash:
 *   aal_hash_key_t *hash_key = kmalloc(sizeof(aal_hash_key_t), GFP_ATOMIC);
 *   aal_hash_value_t hash_value;
 *   aal_hash_mask_t hash_mask;
 *   hashlite_action_normal_t hash_action;
 *   aal_hash_value_t hash_value;
 *   unsigned int hash_idx;
 *   // set hash_key
 *   // set hash_action
 *   hash_value_calculate(hash_key, &HM_L3_GATEWAY, &hash_value);
 *   aal_entry_add(AAL_TABLE_HASHLITE_HASH_TBL, &hash_value, &hash_idx);
 *   aal_entry_add_by_idx(AAL_TABLE_HASH_HASH_ACTION, &hash_action, hash_idx);
 *   aal_entry_local_data_set(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, (unsigned long) hash_key);
 *
 * Flow to delete hash:
 *   aal_hash_key_t *hash_key = NULL;
 *   aal_entry_local_data_get(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, hash_key);
 *   kfree(hash_key);
 *   aal_entry_del(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx);
 *   aal_entry_del(AAL_TABLE_HASH_HASH_ACTION, hash_idx);
 *
 */

#include <generated/ca_ne_autoconf.h>
#include <linux/sched.h>
#include <soc/cortina/cortina-soc.h>
#include "aal_common.h"
#include "aal_table.h"
#include "aal_hashlite.h"
#include "aal_hashlite_init.h"
#include "ca_mut.h"
#include "ca_aal_proc.h"
#include "scfg.h"
#include "osal_misc.h"

#define DBG(x)			{ if (ca_aal_debug & AAL_DBG_HASHLITE) x; }

#define TABLE_TRY_TIMEOUT	1000
#define DATA_REG_NUM		6		/* total number of DATA registers, no matter what kind of AAL table */

static spinlock_t g_hashlite_lock;		/* protecting AAL tables only */
static unsigned int g_aging_time;		/* time unit is second */
static unsigned int g_aging_granularity;	/* Should be same as the value in HW reg */
static unsigned int g_max_aging_time = 0;	/* max aging time in second */
static unsigned int g_ht_size = HASHLITE_HT_SIZE_2K;

static aal_table_t hl_hash_table;
static aal_table_t hl_overflow_table;
static int hl_mask_tbl_entry_add( void *entry, unsigned int *rslt_idx);
static int hl_mask_tbl_entry_get(void *entry, unsigned int *rslt_idx);
static int hl_default_action_entry_add(void *entry, unsigned int *rslt_idx);
static int hl_default_action_entry_get(void *entry, unsigned int *rslt_idx);

/***************************************************************************
 * internal functions
 ***************************************************************************/

/* Set entry to HW from SW cached entry. */
static int __hashlite_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	unsigned int ext_idx = 0; /* [14:0], actual idx of all AAL tables */
	unsigned int *ptr;
	int i;
	unsigned int val = 0;
	ca_device_id_t device_id = 0;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	switch (table->table_id) {
	case AAL_TABLE_8279_HASHLITE_MASK_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_MASK_TBL:
		ext_idx |= (0 << 12); // meaningless
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_TBL:
		ext_idx |= (1 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_TBL:
		ext_idx |= (2 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_ACTION:
		ext_idx |= (3 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_ACTION:
		ext_idx |= (4 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_DEFAULT_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_DEFAULT_ACTION:
		ext_idx |= (5 << 12);
		break;

	default:
		return AAL_E_TBLNOTEXIST;
	}

	/*** handle the lower 192 bits of entry data in all tables, they are switched by bit[10] in ACCESS reg ***/

	ext_idx |= idx;

	/*** write table->entry[i].data to DATA reg, then trigger ACCESS reg to write ***/

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4), this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4); i++)
		ca_ne_reg_write(device_id, ptr[i], table->access_reg + ((DATA_REG_NUM - i) << 2));

	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/*** handle the higher 192 bits of entry data in some tables, they are switched by bit[10] in ACCESS reg ***/

	/* The entry of these tables does not have bit[10] in ACCESS reg */
	if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_HASHLITE_OVERFLOW_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL) ) {
		return AAL_E_OK;
	}

	/* When hash table size is 2K entries, the entry of these tables does not have bit[10] in ACCESS reg */
	if (g_ht_size != HASHLITE_HT_SIZE_1K) {
		/* Workaround for HW bug.
		 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
		 * overflow action or default action can't be applied.
		 *
		 * Solution:
		 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
		 * and then write full 384 bits to register.
		 */
		if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_ACTION)
		    || (table->table_id == AAL_TABLE_8279_HASHLITE_HASH_ACTION) ) {
			return AAL_E_OK;
		}
	}

	ext_idx |= (1 << 10);

	/*** write table->entry[i].data to DATA reg, then trigger ACCESS reg to write ***/

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4) - table->data_reg_num, this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4 - table->data_reg_num); i++)
		ca_ne_reg_write(device_id, ptr[i + table->data_reg_num], table->access_reg + ((DATA_REG_NUM - i) << 2));

	val = 0;
	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
}

/* Get entry from HW directly; some table entry requires to read 2 or more times. */
static int __hashlite_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	unsigned int ext_idx = 0; /* [14:0], actual idx of all AAL tables */
	int i;
	unsigned int val = 0;
	unsigned int *ptr = (unsigned int *) data;
	ca_device_id_t device_id = 0;

	if (data == NULL) return AAL_E_NULLPTR;
	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	switch (table->table_id) {
	case AAL_TABLE_8279_HASHLITE_MASK_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_MASK_TBL:
		ext_idx |= (0 << 12); // meaningless
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_TBL:
		ext_idx |= (1 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_TBL:
		ext_idx |= (2 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_ACTION:
		ext_idx |= (3 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_ACTION:
		ext_idx |= (4 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_DEFAULT_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_DEFAULT_ACTION:
		ext_idx |= (5 << 12);
		break;

	default:
		return AAL_E_TBLNOTEXIST;
	}

	/*** handle the lower 192 bits of entry data in all tables, they are switched by bit[10] in ACCESS reg ***/

	ext_idx |= idx;

	val |= 0 << 30; /* op is read */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4), this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4); i++)
		ptr[i] = ca_ne_reg_read(device_id, table->access_reg + ((DATA_REG_NUM - i) << 2));

	/*** handle the higher 192 bits of entry data in some tables, they are switched by bit[10] in ACCESS reg ***/

	/* The entry of these tables does not have bit[10] in ACCESS reg */
	if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_HASHLITE_OVERFLOW_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL) ) {
		return AAL_E_OK;
	}

	/* When hash table size is 2K entries, the entry of these tables does not have bit[10] in ACCESS reg */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_ACTION)
		    || (table->table_id == AAL_TABLE_8279_HASHLITE_HASH_ACTION) ) {
			return AAL_E_OK;
		}
	}

	ext_idx |= (1 << 10);

	val = 0;
	val |= 0 << 30; /* op is read */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4) - table->data_reg_num, this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4 - table->data_reg_num); i++)
		ptr[i + table->data_reg_num] = ca_ne_reg_read(device_id, table->access_reg + ((DATA_REG_NUM - i) << 2));

	return AAL_E_OK;
}

static int __hashlite_hw_entry_clear(aal_table_t *table, unsigned int idx)
{
	unsigned int ext_idx = 0; /* [14:0], actual idx of all AAL tables */
	unsigned int *ptr;
	int i;
	unsigned int val = 0;
	ca_device_id_t device_id = 0;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	switch (table->table_id) {
	case AAL_TABLE_8279_HASHLITE_MASK_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_MASK_TBL:
		ext_idx |= (0 << 12); // meaningless
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_TBL:
		ext_idx |= (1 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_TBL:
		ext_idx |= (2 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_HASH_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_HASH_ACTION:
		ext_idx |= (3 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_OVERFLOW_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_OVERFLOW_ACTION:
		ext_idx |= (4 << 12);
		break;

	case AAL_TABLE_8279_HASHLITE_DEFAULT_ACTION:
		device_id = 1;
	case AAL_TABLE_HASHLITE_DEFAULT_ACTION:
		ext_idx |= (5 << 12);
		break;

	default:
		return AAL_E_TBLNOTEXIST;
	}

	/*** handle the lower 192 bits of entry data in all tables, they are switched by bit[10] in ACCESS reg ***/

	ext_idx |= idx;

	/*** write table->entry[i].data to DATA reg, then trigger ACCESS reg to write ***/

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4), this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4); i++)
		ca_ne_reg_write(device_id, 0, table->access_reg + ((DATA_REG_NUM - i) << 2));

	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/*** handle the higher 192 bits of entry data in some tables, they are switched by bit[10] in ACCESS reg ***/

	/* The entry of these tables does not have bit[10] in ACCESS reg */
	if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_HASHLITE_OVERFLOW_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_HASH_TBL)
	    || (table->table_id == AAL_TABLE_8279_HASHLITE_OVERFLOW_TBL) ) {
		return AAL_E_OK;
	}

	/* When hash table size is 2K entries, the entry of these tables does not have bit[10] in ACCESS reg */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		/* Workaround for HW bug.
		 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
		 * overflow action or default action can't be applied.
		 *
		 * Solution:
		 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
		 * and then write full 384 bits to register.
		 */
		if (   (table->table_id == AAL_TABLE_HASHLITE_HASH_ACTION)
		    /* || (table->table_id == AAL_TABLE_HASHLITE_OVERFLOW_ACTION) */
		    /* || (table->table_id == AAL_TABLE_HASHLITE_DEFAULT_ACTION) */
		    || (table->table_id == AAL_TABLE_8279_HASHLITE_DEFAULT_ACTION) ) {
			return AAL_E_OK;
		}
	}

	ext_idx |= (1 << 10);

	/*** write table->entry[i].data to DATA reg, then trigger ACCESS reg to write ***/

	/* entry size may not fit into DATA reg, so the for-loop only run to the min value of:
	 *   table->data_reg_num
	 *   ceiling(table->entry_size / 4) - table->data_reg_num, this means the number of DATA reg that entry needs
	 */
	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4 - table->data_reg_num); i++)
		ca_ne_reg_write(device_id, 0, table->access_reg + ((DATA_REG_NUM - i) << 2));

	val = 0;
	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= ext_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
}

static int __hashlite_hw_table_flush(aal_table_t *table)
{
	int i;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	for (i = 0; i < table->max_entry; i++)
		__hashlite_hw_entry_clear(table, i);

	return AAL_E_OK;
}

/* For copying data into aal_hashlite_hash_action_entry_t,
 * or copying aal_hashlite_hash_action_entry_t into data.
 *
 * HW just provide total 192 or 384 bits space for hash action,
 * actual data content and size of hash action is unknown.
 *
 * data_len: actual size of hash action. Not size of aal_hashlite_hash_action_entry_t.
 */
static void *__memcpy_hash_action(void *dest, void *src, size_t data_len)
{
	size_t hash_action_len, final_len;

	/* In Venus,
	 *   if hash table size is 1K, each entry size of action table is 384 bits.
	 *   if hash table size is 2K, each entry size of action table is 192 bits.
	 */
	hash_action_len = (g_ht_size == HASHLITE_HT_SIZE_1K) ? 384 / 8 : 192 / 8;
	final_len = min(hash_action_len, data_len);

	return memcpy(dest, src, final_len);
}

/***************************************************************************
 * HashLite misc
 ***************************************************************************/

/* Reset all ages and valid bits in Hash Table and Overflow Table */
int aal_hashlite_mem_init(void)
{
	L3FE_FE_L3E_HS_LIGHT_MEM_INI_t val;
	int i;

	val.bf.req_sts = 0x3;
	ca_ne_reg_write(0, val.wrd, L3FE_FE_L3E_HS_LIGHT_MEM_INI);

	i = 0;
	do {
		val.wrd = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_MEM_INI);
	} while ((0x3 & val.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
}

int aal_hashlite_hash_table_size_set(unsigned int ht_size)
{
	if (ht_size == HASHLITE_HT_SIZE_1K) {
		ca_ne_reg_write(0, 0, L3FE_FE_L3E_HS_LIGHT_CFG);
		g_ht_size = HASHLITE_HT_SIZE_1K;
	} else if (ht_size == HASHLITE_HT_SIZE_2K) {
		ca_ne_reg_write(0, 1, L3FE_FE_L3E_HS_LIGHT_CFG);
		g_ht_size = HASHLITE_HT_SIZE_2K;
#ifdef INDEXED_INTERFACE_TABLE
	} else {
		ca_ne_reg_write(0, 0x100, L3FE_FE_L3E_HS_LIGHT_CFG);
		g_ht_size = HASHLITE_HT_SIZE_1K_192;
#endif
	}

	return AAL_E_OK;
}

int aal_hashlite_dbl_chk_fail_ctrl_set(aal_hashlite_dbl_chk_fail_ctrl_t *ctrl)
{
	L3FE_FE_L3E_HS_LIGHT_CHK_FAIL_CTRL_t val;

	memset((void *) &val, 0, sizeof(L3FE_FE_L3E_HS_LIGHT_CHK_FAIL_CTRL_t));

	val.bf.type0 = *(ca_uint8_t *) ctrl;

	CA_NE_REG_WRITE(val.wrd, L3FE_FE_L3E_HS_LIGHT_CHK_FAIL_CTRL);

	return AAL_E_OK;
}


/***************************************************************************
 * HashLite Profile
 ***************************************************************************/

/* Validate the action group.
 * The enabled action group decides the size of hash action.
 */
static int __validate_action_group(unsigned int bitmask)
{
	int bits = 0;
	int max_bits = (g_ht_size == HASHLITE_HT_SIZE_1K) ? 384 : 192;

	if (bitmask & ACTION_GROUP_00) bits += ACTION_GROUP_00_BITS;
	if (bitmask & ACTION_GROUP_01) bits += ACTION_GROUP_01_BITS;
	if (bitmask & ACTION_GROUP_02) bits += ACTION_GROUP_02_BITS;
	if (bitmask & ACTION_GROUP_03) bits += ACTION_GROUP_03_BITS;
	if (bitmask & ACTION_GROUP_04) bits += ACTION_GROUP_04_BITS;
	if (bitmask & ACTION_GROUP_05) bits += ACTION_GROUP_05_BITS;
	if (bitmask & ACTION_GROUP_06) bits += ACTION_GROUP_06_BITS;
	if (bitmask & ACTION_GROUP_07) bits += ACTION_GROUP_07_BITS;
	if (bitmask & ACTION_GROUP_08) bits += ACTION_GROUP_08_BITS;
	if (bitmask & ACTION_GROUP_09) bits += ACTION_GROUP_09_BITS;
	if (bitmask & ACTION_GROUP_10) bits += ACTION_GROUP_10_BITS;
	if (bitmask & ACTION_GROUP_11) bits += ACTION_GROUP_11_BITS;
	if (bitmask & ACTION_GROUP_12) bits += ACTION_GROUP_12_BITS;
	if (bitmask & ACTION_GROUP_13) bits += ACTION_GROUP_13_BITS;
	if (bitmask & ACTION_GROUP_14) bits += ACTION_GROUP_14_BITS;
	if (bitmask & ACTION_GROUP_15) bits += ACTION_GROUP_15_BITS;
	if (bitmask & ACTION_GROUP_16) bits += ACTION_GROUP_16_BITS;
	if (bitmask & ACTION_GROUP_17) bits += ACTION_GROUP_17_BITS;
	if (bitmask & ACTION_GROUP_18) bits += ACTION_GROUP_18_BITS;
	if (bitmask & ACTION_GROUP_19) bits += ACTION_GROUP_19_BITS;

	if (bits > max_bits) return AAL_E_OUTRANGE;

	return AAL_E_OK;
}

int __aal_hashlite_profile_set(unsigned int profile_id, aal_hashlite_profile_t *profile)
{
	L3FE_FE_L3E_HS_LIGHT_PROFILE0_EN_t profile_en;
	L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR0_t maskptr0; // for tuple 0~3
	L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR1_t maskptr1; // for tuple 4~7

	aal_hashlite_hash_action_entry_t default_action;
	unsigned int default_action_idx;
	unsigned int tmp_act_grp_mask = 0;

	unsigned int i, tpl_num = 0;
	unsigned int mask_idx;
	int ret;
	int f_add = 0;

	if ((profile_id < HASHLITE_PROFILE_0) || (profile_id > HASHLITE_PROFILE_13))
		return AAL_E_OUTRANGE;

	if (profile == NULL) return AAL_E_NULLPTR;

	if (__validate_action_group(profile->type0_act_grp_mask_hit) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type0_act_grp_mask_hit of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}
	if (__validate_action_group(profile->type1_act_grp_mask_hit) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type1_act_grp_mask_hit of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}
	if (__validate_action_group(profile->miss.type0_act_grp_mask_empty_miss) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type0_act_grp_mask_empty_miss of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}
	if (__validate_action_group(profile->miss.type0_act_grp_mask_age_miss) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type0_act_grp_mask_age_miss of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}
	if (__validate_action_group(profile->miss.type1_act_grp_mask_empty_miss) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type1_act_grp_mask_empty_miss of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}
	if (__validate_action_group(profile->miss.type1_act_grp_mask_age_miss) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! type0_act_grp_mask_age_miss of profile %d is oversized.\n", __func__, profile_id);
		return AAL_E_OUTRANGE;
	}

	memset((void *) &profile_en, 0, sizeof(profile_en));
	memset((void *) &maskptr0, 0, sizeof(maskptr0));
	memset((void *) &maskptr1, 0, sizeof(maskptr1));
	memset((void *) &default_action, 0, sizeof(default_action));

	/*** action group mask for action hit Type-0 & Type-1 ***/

	ca_ne_reg_write(0, profile->type0_act_grp_mask_hit,
		L3FE_FE_L3E_HS_LIGHT_PROFILE0_ACTION + profile_id * L3FE_FE_L3E_HS_LIGHT_PROFILE0_ACTION_STRIDE);
	ca_ne_reg_write(0, profile->type1_act_grp_mask_hit,
		L3FE_FE_L3E_HS_LIGHT_PROFILE0_ACTION_TYPE1 + profile_id * L3FE_FE_L3E_HS_LIGHT_PROFILE0_ACTION_TYPE1_STRIDE);

	/*** default hash action when hash miss (empty miss & age miss) ***/
	/* The action may be exist already; try to get at first, if fail to get, add a new one */

	/* Type-0 empty miss */
	if (profile->miss.type0_empty_miss != NULL) {
		f_add = 0;
		default_action_idx = 0;
		memset((void *) &default_action, 0, sizeof(default_action));
		__memcpy_hash_action(&default_action, profile->miss.type0_empty_miss, profile->miss.type0_empty_miss_len);
		ret = hl_default_action_entry_get(&default_action, &default_action_idx);
		if (ret == AAL_E_OK) {
			/* check if they should have same action entries and same action group masks */
			tmp_act_grp_mask = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
				+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
			if (tmp_act_grp_mask != profile->miss.type0_act_grp_mask_empty_miss) {
				f_add = 1;
			}
		} else {
			f_add = 1;
		}
		if (f_add == 1) {
			ret = hl_default_action_entry_add(&default_action, &default_action_idx);
			if (ret != AAL_E_OK) return ret;

			ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_DEFAULT_ACTION, default_action_idx, profile->miss.type0_act_grp_mask_empty_miss);
			if (ret != AAL_E_OK) return ret;
		}
		ca_ne_reg_write(0, profile->miss.type0_act_grp_mask_empty_miss,
			L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0 + default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
		profile_en.bf.default_sel_0e = default_action_idx;
	}

	/* Type-0 age miss */
	if (profile->miss.type0_age_miss != NULL) {
		f_add = 0;
		default_action_idx = 0;
		memset((void *) &default_action, 0, sizeof(default_action));
		__memcpy_hash_action(&default_action, profile->miss.type0_age_miss, profile->miss.type0_age_miss_len);
		ret = hl_default_action_entry_get(&default_action, &default_action_idx);
		if (ret == AAL_E_OK) {
			/* check if they should have same action entries and same action group masks */
			tmp_act_grp_mask = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
				+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
			if (tmp_act_grp_mask != profile->miss.type0_act_grp_mask_age_miss) {
				f_add = 1;
			}
		} else {
			f_add = 1;
		}
		if (f_add == 1) {
			ret = hl_default_action_entry_add(&default_action, &default_action_idx);
			if (ret != AAL_E_OK) return ret;

			ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_DEFAULT_ACTION, default_action_idx, profile->miss.type0_act_grp_mask_age_miss);
			if (ret != AAL_E_OK) return ret;
		}
		ca_ne_reg_write(0, profile->miss.type0_act_grp_mask_age_miss, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
			+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
		profile_en.bf.default_sel_0a = default_action_idx;
	}

	/* Type-1 empty miss */
	if (profile->miss.type1_empty_miss != NULL) {
		f_add = 0;
		default_action_idx = 0;
		memset((void *) &default_action, 0, sizeof(default_action));
		__memcpy_hash_action(&default_action, profile->miss.type1_empty_miss, profile->miss.type1_empty_miss_len);
		ret = hl_default_action_entry_get(&default_action, &default_action_idx);
		if (ret == AAL_E_OK) {
			/* check if they should have same action entries and same action group masks */
			tmp_act_grp_mask = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
				+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
			if (tmp_act_grp_mask != profile->miss.type1_act_grp_mask_empty_miss) {
				f_add = 1;
			}
		} else {
			f_add = 1;
		}
		if (f_add == 1) {
			ret = hl_default_action_entry_add(&default_action, &default_action_idx);
			if (ret != AAL_E_OK) return ret;

			ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_DEFAULT_ACTION, default_action_idx, profile->miss.type1_act_grp_mask_empty_miss);
			if (ret != AAL_E_OK) return ret;
		}
		ca_ne_reg_write(0, profile->miss.type1_act_grp_mask_empty_miss, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
			+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
		profile_en.bf.default_sel_1e = default_action_idx;
	}

	/* Type-1 age miss */
	if (profile->miss.type1_age_miss != NULL) {
		f_add = 0;
		default_action_idx = 0;
		memset((void *) &default_action, 0, sizeof(default_action));
		__memcpy_hash_action(&default_action, profile->miss.type1_age_miss, profile->miss.type1_age_miss_len);
		ret = hl_default_action_entry_get(&default_action, &default_action_idx);
		if (ret == AAL_E_OK) {
			/* check if they should have same action entries and same action group masks */
			tmp_act_grp_mask = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
				+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
			if (tmp_act_grp_mask != profile->miss.type1_act_grp_mask_age_miss) {
				f_add = 1;
			}
		} else {
			f_add = 1;
		}
		if (f_add == 1) {
			ret = hl_default_action_entry_add(&default_action, &default_action_idx);
			if (ret != AAL_E_OK) return ret;

			ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_DEFAULT_ACTION, default_action_idx, profile->miss.type1_act_grp_mask_age_miss);
			if (ret != AAL_E_OK) return ret;
		}
		ca_ne_reg_write(0, profile->miss.type1_act_grp_mask_age_miss, L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0
			+ default_action_idx * L3FE_FE_L3E_HS_LIGHT_DEFAULT_ACTION_MASK0_STRIDE);
		profile_en.bf.default_sel_1a = default_action_idx;
	}

	/*** tuple type, hash mask index ***/

	for (i = 0; i < NUM_TUPLE; i++) {
		if ((profile->hit[i].tuple_type == TUPLE_TYPE_UNUSED) || (profile->hit[i].hash_mask == NULL)) {
			continue;
		} else if (profile->hit[i].tuple_type == TUPLE_TYPE_FWD) {
			profile_en.bf.tpl_type &= ~(1 << tpl_num);
		} else if (profile->hit[i].tuple_type == TUPLE_TYPE_QOS) {
			profile_en.bf.tpl_type |= (1 << tpl_num);
		}

		ret = hl_mask_tbl_entry_get(profile->hit[i].hash_mask, &mask_idx);
		if (ret != AAL_E_OK) {
			ret = hl_mask_tbl_entry_add(profile->hit[i].hash_mask, &mask_idx);
			if (ret != AAL_E_OK) return ret;
		}

		switch (tpl_num) {
		case 0:
			maskptr0.bf.maskptr00 = mask_idx;
			break;

		case 1:
			maskptr0.bf.maskptr10 = mask_idx;
			break;

		case 2:
			maskptr0.bf.maskptr20 = mask_idx;
			break;

		case 3:
			maskptr0.bf.maskptr30 = mask_idx;
			break;

		case 4:
			maskptr1.bf.maskptr41 = mask_idx;
			break;

		case 5:
			maskptr1.bf.maskptr51 = mask_idx;
			break;

		case 6:
			maskptr1.bf.maskptr61 = mask_idx;
			break;

		case 7:
			maskptr1.bf.maskptr71 = mask_idx;
			break;
		}

		tpl_num++;
	}
	profile_en.bf.tpl_num = tpl_num;

	ca_ne_reg_write(0, maskptr0.wrd, L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR0 + profile_id * L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR0_STRIDE);
	ca_ne_reg_write(0, maskptr1.wrd, L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR1 + profile_id * L3FE_FE_L3E_HS_LIGHT_PROFILE0_MASKPTR1_STRIDE);
	ca_ne_reg_write(0, profile_en.wrd, L3FE_FE_L3E_HS_LIGHT_PROFILE0_EN + profile_id * L3FE_FE_L3E_HS_LIGHT_PROFILE0_EN_STRIDE);

	return AAL_E_OK;
}

/* Set profile#0 ~ #13 */
int aal_hashlite_profile_set(ca_device_id_t device_id, unsigned int profile_id, aal_hashlite_profile_t *profile)
{
	return __aal_hashlite_profile_set(profile_id, profile);
}

/* Set profile#14 which is for direct hit without hash calculation.
 * Therefore, it only needs hash action and related action group mask.
 */
int aal_hashlite_profile_14_set(ca_device_id_t device_id, unsigned int act_grp_mask)
{
	L3FE_FE_L3E_HS_LIGHT_PROFILE14_ACTION_t val;

	if (__validate_action_group(act_grp_mask) != AAL_E_OK) {
		printk(KERN_ERR "%s: ERROR! Hash action group mask of profile 14 is oversized.\n", __func__);
		return AAL_E_OUTRANGE;
	}

	val.bf.act_mask = act_grp_mask;
	ca_ne_reg_write(device_id, val.wrd, L3FE_FE_L3E_HS_LIGHT_PROFILE14_ACTION);

	return AAL_E_OK;
}

/***************************************************************************
 * HashLite Aging
 ***************************************************************************/

/* Set global aging time in seconds. */
int aal_hashlite_aging_timer_set(unsigned int time)
{
	uint64_t quotient;
	uint32_t remainder;
	struct ca_soc_data soc_data;

	if (time > g_max_aging_time) return AAL_E_OUTRANGE;

	if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
		soc_data.core_clk = 250000000; /* assume core clk 250MHz */

	g_aging_time = time;
#if 0
	/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
	g_aging_granularity = (uint32_t) ((uint64_t) time * soc_data.core_clk / HASHLITE_AGING_UNIT / HASHLITE_AGING_START) & 0x3FFFFFFF;
#else
	ca_div((uint64_t) time * soc_data.core_clk, HASHLITE_AGING_UNIT * HASHLITE_AGING_START, &quotient, &remainder);
	g_aging_granularity = (uint32_t) quotient & 0x3FFFFFFF;
#endif
	ca_ne_reg_write(0, g_aging_granularity, L3FE_FE_L3E_HS_LIGHT_AGING_GRANULARITY);

	return AAL_E_OK;
}

/* Get global aging time in seconds.
 * it may not same as original input due to truncated calculation.
 */
int aal_hashlite_aging_timer_get(unsigned int *time)
{
	*time = g_aging_time;
	// *time = (uint32_t) ((uint64_t) g_aging_granularity * HASHLITE_AGING_UNIT * HASHLITE_AGING_START / soc_data.core_clk);

	return AAL_E_OK;
}

/* Get granularity from HW register; the unit is ticks. */
int aal_hashlite_granularity_get(unsigned int *granularity)
{
	*granularity = ca_ne_reg_read(0, L3FE_FE_L3E_HS_LIGHT_AGING_GRANULARITY);
	return AAL_E_OK;
}

/* Set the age of specified hash entry.
 * Due to HW design, only allow to set the value in aal_hashlite_aging_e.
 */
int aal_hashlite_age_set(aal_table_e table_id, unsigned int idx, aal_hashlite_aging_e age)
{
	ca_device_id_t device_id;
	aal_table_t *table = NULL;
	aal_hashlite_age_t ages;
	unsigned int bucket, slot;
	unsigned int val = 0;
	int i;
	unsigned long flags;

	DBG(printk(KERN_INFO "%s: table_id=%d, idx=%u, age=%d.\n", __func__, table_id, idx, age));

	if (table_id == AAL_TABLE_HASHLITE_HASH_TBL) {
		device_id = 0;
		table = &hl_hash_table;
		val |= (6 << 12); /* Hash Age Table */
		bucket = (idx >> 4) & 0x7F;
		slot = idx & 0xF;
	} else if (table_id == AAL_TABLE_HASHLITE_OVERFLOW_TBL) {
		device_id = 0;
		table = &hl_overflow_table;
		val |= (7 << 12); /* Overflow Age Table */
		bucket = 0;
		slot = idx & 0x1F;
	} else {
		return AAL_E_OPNOTSUPP;
	}

	if (age > HASHLITE_AGING_STATIC) return AAL_E_OUTRANGE;
	if (idx > table->max_entry) return AAL_E_OUTRANGE;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* set bucket[6:0] to ACCESS reg, then read ages from DATA reg */

	val |= 0 << 30; /* op is read */
	val |= 1 << 31;
	val |= bucket;

	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1, and so on. */
	ages.wrd = ca_ne_reg_read(device_id, table->access_reg + ((DATA_REG_NUM - (slot >> 3)) << 2));

	/* update age into ages */

	switch (slot & 0x7) {
	case 0:
		ages.bf.slot_0 = age;
		break;

	case 1:
		ages.bf.slot_1 = age;
		break;

	case 2:
		ages.bf.slot_2 = age;
		break;

	case 3:
		ages.bf.slot_3 = age;
		break;

	case 4:
		ages.bf.slot_4 = age;
		break;

	case 5:
		ages.bf.slot_5 = age;
		break;

	case 6:
		ages.bf.slot_6 = age;
		break;

	case 7:
		ages.bf.slot_7 = age;
		break;
	};

	/* write ages to DATA reg, then set bucket[6:0] to ACCESS reg */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1, and so on. */
	ca_ne_reg_write(device_id, ages.wrd, table->access_reg + ((DATA_REG_NUM - (slot >> 3)) << 2));

	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= bucket;

	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

/* Get the age of specified hash entry.
 * It also returns the approximated time in seconds.
 */
int aal_hashlite_age_get(aal_table_e table_id, unsigned int idx, aal_hashlite_aging_e *age, unsigned int *time)
{
	ca_device_id_t device_id;
	aal_table_t *table = NULL;
	aal_hashlite_age_t ages;
	unsigned int granularity;
	unsigned int bucket, slot;
	unsigned int val = 0;
	int i;
	unsigned long flags;

	uint64_t quotient;
	uint32_t remainder;
	struct ca_soc_data soc_data;

	DBG(printk(KERN_INFO "%s: table_id=%d, idx=%u.\n", __func__, table_id, idx));

	if (table_id == AAL_TABLE_HASHLITE_HASH_TBL) {
		device_id = 0;
		table = &hl_hash_table;
		val |= (6 << 12); /* Hash Age Table */
		bucket = (idx >> 4) & 0x7F;
		slot = idx & 0xF;
	} else if (table_id == AAL_TABLE_HASHLITE_OVERFLOW_TBL) {
		device_id = 0;
		table = &hl_overflow_table;
		val |= (7 << 12); /* Overflow Age Table */
		bucket = 0;
		slot = idx & 0x1F;
	} else {
		return AAL_E_OPNOTSUPP;
	}

	if (idx > table->max_entry) return AAL_E_OUTRANGE;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	aal_hashlite_granularity_get(&granularity);

	/* set bucket[6:0] to ACCESS reg, then read ages from DATA reg */

	val |= 0 << 30; // op is read
	val |= 1 << 31;
	val |= bucket;

	ca_ne_reg_write(device_id, val, table->access_reg);

	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1, and so on. */
	ages.wrd = ca_ne_reg_read(device_id, table->access_reg + ((DATA_REG_NUM - (slot >> 3)) << 2));

	/* update age into ages */

	switch (slot & 0x7) {
	case 0:
		*age = ages.bf.slot_0;
		break;

	case 1:
		*age = ages.bf.slot_1;
		break;

	case 2:
		*age = ages.bf.slot_2;
		break;

	case 3:
		*age = ages.bf.slot_3;
		break;

	case 4:
		*age = ages.bf.slot_4;
		break;

	case 5:
		*age = ages.bf.slot_5;
		break;

	case 6:
		*age = ages.bf.slot_6;
		break;

	case 7:
		*age = ages.bf.slot_7;
		break;
	};

	/* calculate the time */

	if ((*age == HASHLITE_AGING_STATIC) || (*age == HASHLITE_AGING_INVALID)) {
		*time = 0;
	} else {
		if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
			soc_data.core_clk = 250000000; /* assume core clk 250MHz */
#if 0
		/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
		*time = (uint32_t) ((uint64_t) ((*age - 1) * g_aging_granularity + granularity) * HASHLITE_AGING_UNIT / soc_data.core_clk);
#else
		ca_div((uint64_t) ((*age - 1) * g_aging_granularity + granularity) * HASHLITE_AGING_UNIT, soc_data.core_clk, &quotient, &remainder);
		*time = (unsigned int) quotient;
#endif
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	DBG(printk(KERN_INFO "%s: Got age. table_id=%d, idx=%u, age=%u.\n", __func__, table_id, idx, *age));

	return AAL_E_OK;
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Mask Table
 ***************************************************************************/

static aal_table_t hl_mask_table;

static int __hl_mask_tbl_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int i, ret;
	aal_entry_t *p_entry;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* search if entry exists already */

	for (i = 0; i < table->max_entry; i++) {
		p_entry = sw_entry_get(table, i);
		if ( (p_entry != NULL) && (p_entry->data != NULL) && (!memcmp(p_entry->data, entry, table->entry_size)) ) {
			*rslt_idx = i;
			atomic_inc(&table->entry[i].refcnt);
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
		}
	}

	/* search empty space for entry */

	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			if (table->entry[i].data == NULL) {
				spin_unlock_irqrestore(&table->lock, flags);
				return AAL_E_NOMEM;
			}

			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			/* set to HW */
			ret = __hashlite_hw_entry_set(table, *rslt_idx);

			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hl_mask_tbl_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_mask_tbl_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_mask_tbl_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hashlite_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_mask_tbl_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_mask_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_mask_tbl_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_mask_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_mask_tbl_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hash_mask_t entry;
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

	printk(KERN_INFO "|===== Hash-Lite Mask Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __hl_mask_tbl_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);

			printk(KERN_INFO "Hash Control:\n");
			printk(KERN_INFO "  table_id(1) %d, hkey_id(1) %d, ctrl_set_id(1) %d\n", entry.table_id, entry.hkey_id, entry.ctrl_set_id);

			printk(KERN_INFO "Source Port ID\n");
			printk(KERN_INFO "  lspid(1) %d, orig_lspid(1) %d, mc_idx_vld(1) %d\n", entry.lspid, entry.orig_lspid, entry.mc_idx_vld);

			printk(KERN_INFO "Dest Port ID\n");
			printk(KERN_INFO "  mc(1) %d, mcgid(10) 0x%03x\n", entry.mc, entry.mcgid);

			printk(KERN_INFO "COS:\n");
			printk(KERN_INFO "  cos(1) %d\n", entry.cos);

			printk(KERN_INFO "POL ID:\n");
			printk(KERN_INFO "  pol_id(1) %d, pol_grp_id(1) %d, qos_premark(1) %d\n", entry.pol_id, entry.pol_grp_id, entry.qos_premark);

			printk(KERN_INFO "MDATA:\n");
			printk(KERN_INFO "  mdata(64) 0x%016llx\n", entry.mdata);

			printk(KERN_INFO "Special Packet:\n");
			printk(KERN_INFO "  spcl_pkt_enc(1) %d, spcl_pkt_hdr_mtch(8) 0x%02x\n", entry.spcl_pkt_enc, entry.spcl_pkt_hdr_mtch);

			printk(KERN_INFO "L2:\n");
			printk(KERN_INFO "  mac_da(6) 0x%02x, mac_da_an_sel(1) %d, mac_da_ip_mc(1) %d\n",
				entry.mac_da, entry.mac_da_an_sel, entry.mac_da_ip_mc);
			printk(KERN_INFO "  mac_da_rng(1) %d, mac_da_rsvd(1) %d\n", entry.mac_da_rng, entry.mac_da_rsvd);
			printk(KERN_INFO "  mac_sa(6) 0x%02x\n", entry.mac_sa);
			printk(KERN_INFO "  ethertype(1) %d, ethertype_enc(1) %d\n", entry.ethertype, entry.ethertype_enc);

			printk(KERN_INFO "L2 Format:\n");
			printk(KERN_INFO "  len_encoded(1) %d, pktlen_rng_match_vec(4) 0x%x, llc_snap(1) %d, llc_type_enc(1) %d\n",
				entry.len_encoded, entry.pktlen_rng_match_vec, entry.llc_snap, entry.llc_type_enc);

			printk(KERN_INFO "VLAN:\n");
			printk(KERN_INFO "  vlan_cnt(1) %d\n", entry.vlan_cnt);
			printk(KERN_INFO "  top_tpid_enc(1) %d, top_vid(1) %d, top_8021p(1) %d, top_dei(1) %d\n",
				entry.top_tpid_enc, entry.top_vid, entry.top_8021p, entry.top_dei);
			printk(KERN_INFO "  inner_tpid_enc(1) %d, inner_vid(1) %d, inner_8021p(1) %d, inner_dei(1) %d\n",
				entry.inner_tpid_enc, entry.inner_vid, entry.inner_8021p, entry.inner_dei);

			printk(KERN_INFO "PPP / PPPoE:\n");
			printk(KERN_INFO "  pppoe_type(1) %d, pppoe_code_enc(1) %d, pppoe_session_id(1) %d, ppp_protocol_enc(1) %d\n",
				entry.pppoe_type, entry.pppoe_code_enc, entry.pppoe_session_id, entry.ppp_protocol_enc);

			printk(KERN_INFO "L3:\n");
			printk(KERN_INFO "  ip_vld(1) %d, ip_ver(1) %d, ip_dscp(6) 0x%02x, ip_ecn(2) %d\n",
				entry.ip_vld, entry.ip_ver, entry.ip_dscp, entry.ip_ecn);
			printk(KERN_INFO "  ip_protocol(1) %d, ip_l4_type(1) %d\n", entry.ip_protocol, entry.ip_l4_type);
			printk(KERN_INFO "  ip_sa(9) 0x%03x, ip_da(9) 0x%03x\n", entry.ip_sa, entry.ip_da);
			printk(KERN_INFO "  ipv6_flow_lbl(1) %d, ip_ttl(2) %d, ip_options(1) %d, ip_fragment_flag(1) %d\n",
				entry.ipv6_flow_lbl, entry.ip_ttl, entry.ip_options, entry.ip_fragment_flag);
			printk(KERN_INFO "  ipv6_hbh(1) %d, ipv6_rh(1) %d, ipv6_doh(1) %d\n", entry.ipv6_hbh, entry.ipv6_rh, entry.ipv6_doh);
			printk(KERN_INFO "  icmp_vld(1) %d, icmp_type(1) %d\n", entry.icmp_vld, entry.icmp_type);
			printk(KERN_INFO "  spi_vld(1) %d, spi(1) %d\n", entry.spi_vld, entry.spi);
			printk(KERN_INFO "  l3_chksum_err(1) %d\n", entry.l3_chksum_err);

			printk(KERN_INFO "L4:\n");
			printk(KERN_INFO "  l4_sp_exact_range(17) 0x%05x, l4_dp_exact_range(17) 0x%05x\n", entry.l4_sp_exact_range, entry.l4_dp_exact_range);
			printk(KERN_INFO "  tcp_rdp_ctrl(9) 0x%03x, l4_chksum_zero(1) %d\n", entry.tcp_rdp_ctrl, entry.l4_chksum_zero);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_mask_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	return __hl_mask_tbl_entry_add(&hl_mask_table, entry, rslt_idx);
}

static int hl_mask_tbl_entry_del(unsigned int idx)
{
	return __hl_mask_tbl_entry_del(&hl_mask_table, idx);
}

static int hl_mask_tbl_entry_set(unsigned int idx, void *entry)
{
	return __hl_mask_tbl_entry_set(&hl_mask_table, idx, entry);
}

static int hl_mask_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_mask_tbl_entry_get(&hl_mask_table, entry, rslt_idx);
}

static int hl_mask_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_mask_tbl_entry_get_by_idx(&hl_mask_table, idx, entry);
}

static int hl_mask_tbl_flush(void)
{
	return __hl_mask_tbl_flush(&hl_mask_table);
}

static int hl_mask_tbl_get_avail_count(void)
{
	return __hl_mask_tbl_get_avail_count(&hl_mask_table);
}

static void hl_mask_tbl_entry_print(unsigned int start_idx, int count)
{
	return __hl_mask_tbl_entry_print(&hl_mask_table, start_idx, count);
}

static aal_table_t hl_mask_table = {
	.table_id = AAL_TABLE_HASHLITE_MASK_TBL,
	.table_name = "HashLiteEngine Mask Table",
	.max_entry = HASHLITE_MASK_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hash_mask_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 4, /* only use 128 bits */
	.op = {
		.entry_add = hl_mask_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = hl_mask_tbl_entry_del,
		.entry_set = hl_mask_tbl_entry_set,
		.entry_get = hl_mask_tbl_entry_get,
		.entry_get_by_idx = hl_mask_tbl_entry_get_by_idx,
		.table_flush = hl_mask_tbl_flush,
		.get_avail_count = hl_mask_tbl_get_avail_count,
		.entry_print = hl_mask_tbl_entry_print,
	},
};

int __hashlite_mask_tbl_init(void)
{
	int ret;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_MASK_TBL, &hl_mask_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __hashlite_mask_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_MASK_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Hash Table
 ***************************************************************************/

static aal_table_t hl_hash_table;

static int __hl_hash_tbl_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int start, end;
	unsigned int mask = (g_ht_size == HASHLITE_HT_SIZE_2K) ? 0xFE00 : 0x7E00;
	aal_hash_value_t *orig_data;
	aal_hash_value_t *new_data = entry;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	start = (new_data->crc16 & mask) >> 5;
	end = ((new_data->crc16 & mask) >> 5) | 0xF;

	DBG(printk(KERN_INFO "%s: crc32=0x%08x, crc16=0x%04x, start=%u, end=%u\n", __func__, new_data->crc32, new_data->crc16, start, end));

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* Search the bucket to see if entry is added already.
	 * Only compare the crc32 value because HW only stores crc32.
	 */
	for (i = start; i <= end; i++) {
		orig_data = table->entry[i].data;
		if ((orig_data != NULL) && (orig_data->crc32 == new_data->crc32)) {
#if 0
			*rslt_idx = i;
			atomic_inc(&table->entry[i].refcnt);
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			DBG(printk(KERN_INFO "%s: Found same crc32 at index %u, increase refcnt.\n", __func__, i));
			return AAL_E_OK;
#else
			*rslt_idx = i; /* still return the existing hash entry index */
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			DBG(printk(KERN_INFO "%s: Found same crc32 at index %u, existing...\n", __func__, i));
			return AAL_E_EXIST;
#endif
		}
	}

	/* search empty entry to add */
	for (i = start; i <= end; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;
			DBG(printk(KERN_INFO "%s: Found empty entry at index %u\n", __func__, *rslt_idx));

			/* set to HW */
			ret = __hashlite_hw_entry_set(table, i);

			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hl_hash_tbl_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_tbl_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	// TODO: should check crc16?
	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_tbl_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int start, end;
	unsigned int mask = (g_ht_size == HASHLITE_HT_SIZE_2K) ? 0xFE00 : 0x7E00;
	aal_hash_value_t *iter_data;
	aal_hash_value_t *data = entry;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	start = (data->crc16 & mask) >> 5;
	end = ((data->crc16 & mask) >> 5) | 0xF;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = start; i <= end; i++) {
		iter_data = table->entry[i].data;
		if ((iter_data != NULL) && (iter_data->crc32 == data->crc32)) {
			*rslt_idx = i;
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_hash_tbl_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_hash_tbl_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_hash_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_hash_tbl_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hash_value_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;
	aal_hash_key_t *hash_key = NULL;
	aal_hashlite_aging_e age;
	unsigned int time_sec;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== Hash-Lite Hash Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		/* get aging of each entry */
		if ((ret = aal_hashlite_age_get(table->table_id, i, &age, &time_sec)) != AAL_E_OK) {
			printk(KERN_INFO "ERROR! Fail to get aging of hash_idx %d. (ret=%d)\n", i, ret);
		}

		/* get each hash entry */
		if (ca_aal_table_sw_cache) {
			ret = __hl_hash_tbl_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "crc32 0x%08x\n", entry.crc32);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "age %d [%s], approximate %u seconds\n",
				age,
				(age == HASHLITE_AGING_STATIC) ? "STATIC" : ((age == HASHLITE_AGING_INVALID) ? "EXPIRED" : "AGING"),
				time_sec);

			hash_key = (aal_hash_key_t *) table->entry[i].local_data;
			if (hash_key != NULL)
				hash_key_dump(hash_key);
			else
				printk(KERN_INFO "\nLocal data of this entry does not contain hash key.\n");
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_hash_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	return __hl_hash_tbl_entry_add(&hl_hash_table, entry, rslt_idx);
}

static int hl_hash_tbl_entry_del(unsigned int idx)
{
	return __hl_hash_tbl_entry_del(&hl_hash_table, idx);
}

static int hl_hash_tbl_entry_set(unsigned int idx, void *entry)
{
	return __hl_hash_tbl_entry_set(&hl_hash_table, idx, entry);
}

static int hl_hash_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_hash_tbl_entry_get(&hl_hash_table, entry, rslt_idx);
}

static int hl_hash_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_hash_tbl_entry_get_by_idx(&hl_hash_table, idx, entry);
}

static int hl_hash_tbl_flush(void)
{
	return __hl_hash_tbl_flush(&hl_hash_table);
}

static int hl_hash_tbl_get_avail_count(void)
{
	return __hl_hash_tbl_get_avail_count(&hl_hash_table);
}

static void hl_hash_tbl_entry_print(unsigned int start_idx, int count)
{
	return __hl_hash_tbl_entry_print(&hl_hash_table, start_idx, count);
}

static aal_table_t hl_hash_table = {
	.table_id = AAL_TABLE_HASHLITE_HASH_TBL,
	.table_name = "HashLiteEngine Hash Table",
	.max_entry = HASHLITE_HASH_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hash_value_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 1, /* only use 32 bits */
	.op = {
		.entry_add = hl_hash_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = hl_hash_tbl_entry_del,
		.entry_set = hl_hash_tbl_entry_set,
		.entry_get = hl_hash_tbl_entry_get,
		.entry_get_by_idx = hl_hash_tbl_entry_get_by_idx,
		.table_flush = hl_hash_tbl_flush,
		.get_avail_count = hl_hash_tbl_get_avail_count,
		.entry_print = hl_hash_tbl_entry_print,
	},
};

int __hashlite_hash_tbl_init(void)
{
	int ret;
	unsigned int ht_size;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_HASH_TBL, &hl_hash_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	ret = scfg_read(0, CFG_ID_HASHLITE_HT_SIZE, sizeof(ht_size), (ca_uint8_t *) &ht_size);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_HASHLITE_HT_SIZE. (ret=%d)\n", __func__, ret);
		return AAL_E_OPNOTSUPP;
	}

	aal_hashlite_hash_table_size_set(ht_size);

	return AAL_E_OK;
}

void __hashlite_hash_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_HASH_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Overflow Table
 ***************************************************************************/

static aal_table_t hl_overflow_table;

static int __hl_overflow_tbl_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
#if 0
			*rslt_idx = i;
			atomic_inc(&table->entry[i].refcnt);
			DBG(printk(KERN_INFO "%s: Found same entry at index %u, increase refcnt.\n", __func__, i));
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
#else
			*rslt_idx = i; /* still return the existing hash entry index */
			DBG(printk(KERN_INFO "%s: Found same entry at index %u, existing...\n", __func__, i));
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_EXIST;
#endif
		}
	}

	/* search empty entry to add */
	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			/* set to HW */
			ret = __hashlite_hw_entry_set(table, i);

			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hl_overflow_tbl_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_tbl_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_tbl_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hashlite_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_overflow_tbl_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_overflow_tbl_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_overflow_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_overflow_tbl_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hash_value_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;
	aal_hash_key_t *hash_key = NULL;
	aal_hashlite_aging_e age;
	unsigned int time_sec;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== Hash-Lite Overflow Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		/* get aging of each entry */
		if ((ret = aal_hashlite_age_get(table->table_id, i, &age, &time_sec)) != AAL_E_OK) {
			printk(KERN_INFO "ERROR! Fail to get aging of hash_idx %d. (ret=%d)\n", i, ret);
		}

		/* get each hash entry */
		if (ca_aal_table_sw_cache) {
			ret = __hl_overflow_tbl_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid %d, crc16 0x%04x, crc32 0x%08x\n", entry.valid, entry.crc16, entry.crc32);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "age %d [%s], approximate %u seconds\n",
				age,
				(age == HASHLITE_AGING_STATIC) ? "STATIC" : ((age == HASHLITE_AGING_INVALID) ? "EXPIRED" : "AGING"),
				time_sec);

			hash_key = (aal_hash_key_t *) table->entry[i].local_data;
			if (hash_key != NULL)
				hash_key_dump(hash_key);
			else
				printk(KERN_INFO "\nLocal data of this entry does not contain hash key.\n");
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_overflow_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	return __hl_overflow_tbl_entry_add(&hl_overflow_table, entry, rslt_idx);
}

static int hl_overflow_tbl_entry_del(unsigned int idx)
{
	return __hl_overflow_tbl_entry_del(&hl_overflow_table, idx);
}

static int hl_overflow_tbl_entry_set(unsigned int idx, void *entry)
{
	return __hl_overflow_tbl_entry_set(&hl_overflow_table, idx, entry);
}

static int hl_overflow_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_overflow_tbl_entry_get(&hl_overflow_table, entry, rslt_idx);
}

static int hl_overflow_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_overflow_tbl_entry_get_by_idx(&hl_overflow_table, idx, entry);
}

static int hl_overflow_tbl_flush(void)
{
	return __hl_overflow_tbl_flush(&hl_overflow_table);
}

static int hl_overflow_tbl_get_avail_count(void)
{
	return __hl_overflow_tbl_get_avail_count(&hl_overflow_table);
}

static void hl_overflow_tbl_entry_print(unsigned int start_idx, int count)
{
	return __hl_overflow_tbl_entry_print(&hl_overflow_table, start_idx, count);
}

static aal_table_t hl_overflow_table = {
	.table_id = AAL_TABLE_HASHLITE_OVERFLOW_TBL,
	.table_name = "HashLiteEngine Overflow Table",
	.max_entry = HASHLITE_OVERFLOW_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hash_value_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 2, /* only use 48 bits */
	.op = {
		.entry_add = hl_overflow_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = hl_overflow_tbl_entry_del,
		.entry_set = hl_overflow_tbl_entry_set,
		.entry_get = hl_overflow_tbl_entry_get,
		.entry_get_by_idx = hl_overflow_tbl_entry_get_by_idx,
		.table_flush = hl_overflow_tbl_flush,
		.get_avail_count = hl_overflow_tbl_get_avail_count,
		.entry_print = hl_overflow_tbl_entry_print,
	},
};

int __hashlite_overflow_tbl_init(void)
{
	int ret;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_OVERFLOW_TBL, &hl_overflow_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __hashlite_overflow_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_OVERFLOW_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Hash Action
 ***************************************************************************/

static aal_table_t hl_hash_action_table;

static int __hl_hash_action_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
#if 0
			*rslt_idx = i;
			atomic_inc(&table->entry[i].refcnt);
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
#else
			*rslt_idx = i; /* still return the existing hash action entry index */
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_EXIST;
#endif
		}
	}

	/* search empty entry to add */
	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			/* set to HW */
			ret = __hashlite_hw_entry_set(table, i);

			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hl_hash_action_entry_add_by_idx(aal_table_t *table, void *entry, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (table->entry[idx].data != NULL) {
		if (!memcmp(table->entry[idx].data, entry, table->entry_size)) {
			/* same entry */
#if 0
			atomic_inc(&table->entry[idx].refcnt);
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
#else
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_EXIST;
#endif
		} else {
			/* different entry */
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRY_USED;
		}
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_action_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_action_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_action_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hashlite_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_hash_action_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_hash_action_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_hash_action_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_hash_action_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_hash_action_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hashlite_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash-Lite Hash Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __hl_hash_action_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx\n",
				entry.reserved_5, entry.reserved_4, entry.reserved_3,
				entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hashlite_action_dump((unsigned int) table->entry[i].local_data, &entry);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_hash_action_entry_add(void *entry, unsigned int *rslt_idx)
{
	return __hl_hash_action_entry_add(&hl_hash_action_table, entry, rslt_idx);
}

static int hl_hash_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	return __hl_hash_action_entry_add_by_idx(&hl_hash_action_table, entry, idx);
}

static int hl_hash_action_entry_del(unsigned int idx)
{
	return __hl_hash_action_entry_del(&hl_hash_action_table, idx);
}

static int hl_hash_action_entry_set(unsigned int idx, void *entry)
{
	return __hl_hash_action_entry_set(&hl_hash_action_table, idx, entry);
}

static int hl_hash_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_hash_action_entry_get(&hl_hash_action_table, entry, rslt_idx);
}

static int hl_hash_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_hash_action_entry_get_by_idx(&hl_hash_action_table, idx, entry);
}

static int hl_hash_action_tbl_flush(void)
{
	return __hl_hash_action_tbl_flush(&hl_hash_action_table);
}

static int hl_hash_action_tbl_get_avail_count(void)
{
	return __hl_hash_action_tbl_get_avail_count(&hl_hash_action_table);
}

static void hl_hash_action_entry_print(unsigned int start_idx, int count)
{
	return __hl_hash_action_entry_print(&hl_hash_action_table, start_idx, count);
}

static aal_table_t hl_hash_action_table = {
	.table_id = AAL_TABLE_HASHLITE_HASH_ACTION,
	.table_name = "HashLiteEngine Hash Action Table",
	.max_entry = HASHLITE_HASH_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hashlite_hash_action_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 6,
	.op = {
		.entry_add = hl_hash_action_entry_add,
		.entry_add_by_idx = hl_hash_action_entry_add_by_idx,
		.entry_del = hl_hash_action_entry_del,
		.entry_set = hl_hash_action_entry_set,
		.entry_get = hl_hash_action_entry_get,
		.entry_get_by_idx = hl_hash_action_entry_get_by_idx,
		.table_flush = hl_hash_action_tbl_flush,
		.get_avail_count = hl_hash_action_tbl_get_avail_count,
		.entry_print = hl_hash_action_entry_print,
	},
};

int __hashlite_hash_action_init(void)
{
	int ret;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_HASH_ACTION, &hl_hash_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __hashlite_hash_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_HASH_ACTION);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Overflow Action
 ***************************************************************************/

static aal_table_t hl_overflow_action_table;

static int __hl_overflow_action_entry_add_by_idx(aal_table_t *table, void *entry, unsigned int idx)
{
	unsigned long flags;
	int ret;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (table->entry[idx].data != NULL) {
		if (!memcmp(table->entry[idx].data, entry, table->entry_size)) {
			/* same entry */
#if 0
			atomic_inc(&table->entry[idx].refcnt);
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_OK;
#else
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_EXIST;
#endif
		} else {
			/* different entry */
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRY_USED;
		}
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_action_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_action_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_action_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int i, remaining_used = table->used_entry;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hashlite_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_overflow_action_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_overflow_action_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_overflow_action_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_overflow_action_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_overflow_action_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hashlite_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash-Lite Overflow Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __hl_overflow_action_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx\n",
				entry.reserved_5, entry.reserved_4, entry.reserved_3,
				entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hashlite_action_dump((unsigned int) table->entry[i].local_data, &entry);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_overflow_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	return __hl_overflow_action_entry_add_by_idx(&hl_overflow_action_table, entry, idx);
}

static int hl_overflow_action_entry_del(unsigned int idx)
{
	return __hl_overflow_action_entry_del(&hl_overflow_action_table, idx);
}

static int hl_overflow_action_entry_set(unsigned int idx, void *entry)
{
	return __hl_overflow_action_entry_set(&hl_overflow_action_table, idx, entry);
}

static int hl_overflow_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_overflow_action_entry_get(&hl_overflow_action_table, entry, rslt_idx);
}

static int hl_overflow_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_overflow_action_entry_get_by_idx(&hl_overflow_action_table, idx, entry);
}

static int hl_overflow_action_tbl_flush(void)
{
	return __hl_overflow_action_tbl_flush(&hl_overflow_action_table);
}

static int hl_overflow_action_tbl_get_avail_count(void)
{
	return __hl_overflow_action_tbl_get_avail_count(&hl_overflow_action_table);
}

static void hl_overflow_action_entry_print(unsigned int start_idx, int count)
{
	return __hl_overflow_action_entry_print(&hl_overflow_action_table, start_idx, count);
}

static aal_table_t hl_overflow_action_table = {
	.table_id = AAL_TABLE_HASHLITE_OVERFLOW_ACTION,
	.table_name = "HashLiteEngine Overflow Action Table",
	.max_entry = HASHLITE_OVERFLOW_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hashlite_hash_action_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 6,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = hl_overflow_action_entry_add_by_idx,
		.entry_del = hl_overflow_action_entry_del,
		.entry_set = hl_overflow_action_entry_set,
		.entry_get = hl_overflow_action_entry_get,
		.entry_get_by_idx = hl_overflow_action_entry_get_by_idx,
		.table_flush = hl_overflow_action_tbl_flush,
		.get_avail_count = hl_overflow_action_tbl_get_avail_count,
		.entry_print = hl_overflow_action_entry_print,
	},
};

int __hashlite_overflow_action_init(void)
{
	int ret;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_OVERFLOW_ACTION, &hl_overflow_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __hashlite_overflow_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_OVERFLOW_ACTION);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_LIGHT_IND_X for Default Action
 ***************************************************************************/

static aal_table_t hl_default_action_table;

static int __hl_default_action_entry_add(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int i, ret;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	/* NOTE:
	 * Skip searching the same entry.
	 *
	 * The default actions of all profiles share the total 16 entries of default action table.
	 * However, it is possible that there are two same 'default action entry' but their 'default action group mask' are different
	 * even the chance is very low.
	 * This condition can't be detected in AAL. Programmer must take care of this by following the steps:
	 *   1. Call __default_action_entry_get() to check if same entry exists.
	 *   2. If yes, compare the 'default action group mask'.
	 *   3. Reuse the same default action entry only when the 'default action group mask' is same.
	 */
#if 0
	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
			*rslt_idx = i; /* still return the existing default action entry index */
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_EXIST;
		}
	}
#endif

	/* search empty entry to add */
	for (i = 0; i < table->max_entry; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			/* set to HW */
			ret = __hashlite_hw_entry_set(table, i);

			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hl_default_action_entry_add_by_idx(aal_table_t *table, void *entry, unsigned int idx)
{
	unsigned long flags;
	int ret;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (table->entry[idx].data != NULL) {
		/* Should check if it is same entry? */
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return AAL_E_ENTRY_USED;
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_default_action_entry_del(aal_table_t *table, unsigned int idx)
{
	unsigned long flags;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hashlite_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_default_action_entry_set(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;
	int ret;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = __hashlite_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hashlite_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_default_action_entry_get(aal_table_t *table, void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	unsigned int i, remaining_used = table->used_entry;
	aal_hashlite_hash_action_entry_t *hash_action = entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	/* Workaround for HW bug.
	 * If Hash Table size is set to 2K, and hash entry of index 1024 or higher was hit,
	 * overflow action or default action can't be applied.
	 *
	 * Solution:
	 * For overflow action and defaction entry, copy the lower 192 bits to higher 192 bits,
	 * and then write full 384 bits to register.
	 */
	if (g_ht_size == HASHLITE_HT_SIZE_2K) {
		hash_action->reserved_3 = hash_action->reserved_0;
		hash_action->reserved_4 = hash_action->reserved_1;
		hash_action->reserved_5 = hash_action->reserved_2;
	}

	spin_lock_irqsave(&g_hashlite_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hashlite_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hashlite_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hl_default_action_entry_get_by_idx(aal_table_t *table, unsigned int idx, void *entry)
{
	unsigned long flags;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hashlite_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hashlite_lock, flags);

	return AAL_E_OK;
}

static int __hl_default_action_tbl_flush(aal_table_t *table)
{
	int i, ret;

	for (i = 0; i < table->max_entry; i++) {
		ret = __hl_default_action_entry_del(table, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hl_default_action_tbl_get_avail_count(aal_table_t *table)
{
	return generic_get_avail_count(table);
}

static void __hl_default_action_entry_print(aal_table_t *table, unsigned int start_idx, int count)
{
	aal_hashlite_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash-Lite Default Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __hl_default_action_entry_get_by_idx(table, i, &entry);
		} else {
			ret = __hashlite_hw_entry_get(table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx\n",
				entry.reserved_5, entry.reserved_4, entry.reserved_3,
				entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hashlite_action_dump((unsigned int) table->entry[i].local_data, &entry);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static int hl_default_action_entry_add(void *entry, unsigned int *rslt_idx)
{
	return __hl_default_action_entry_add(&hl_default_action_table, entry, rslt_idx);
}

static int hl_default_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	return __hl_default_action_entry_add_by_idx(&hl_default_action_table, entry, idx);
}

static int hl_default_action_entry_del(unsigned int idx)
{
	return __hl_default_action_entry_del(&hl_default_action_table, idx);
}

static int hl_default_action_entry_set(unsigned int idx, void *entry)
{
	return __hl_default_action_entry_set(&hl_default_action_table, idx, entry);
}

static int hl_default_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	return __hl_default_action_entry_get(&hl_default_action_table, entry, rslt_idx);
}

static int hl_default_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	return __hl_default_action_entry_get_by_idx(&hl_default_action_table, idx, entry);
}

static int hl_default_action_tbl_flush(void)
{
	return __hl_default_action_tbl_flush(&hl_default_action_table);
}

static int hl_default_action_tbl_get_avail_count(void)
{
	return __hl_default_action_tbl_get_avail_count(&hl_default_action_table);
}

static void hl_default_action_entry_print(unsigned int start_idx, int count)
{
	return __hl_default_action_entry_print(&hl_default_action_table, start_idx, count);
}

static aal_table_t hl_default_action_table = {
	.table_id = AAL_TABLE_HASHLITE_DEFAULT_ACTION,
	.table_name = "HashLiteEngine Default Action Table",
	.max_entry = HASHLITE_DEFAULT_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hashlite_hash_action_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 6,
	.op = {
		.entry_add = hl_default_action_entry_add,
		.entry_add_by_idx = hl_default_action_entry_add_by_idx,
		.entry_del = hl_default_action_entry_del,
		.entry_set = hl_default_action_entry_set,
		.entry_get = hl_default_action_entry_get,
		.entry_get_by_idx = hl_default_action_entry_get_by_idx,
		.table_flush = hl_default_action_tbl_flush,
		.get_avail_count = hl_default_action_tbl_get_avail_count,
		.entry_print = hl_default_action_entry_print,
	},
};

int __hashlite_default_action_init(void)
{
	int ret;

	spin_lock_init(&g_hashlite_lock);

	ret = __aal_table_register(AAL_TABLE_HASHLITE_DEFAULT_ACTION, &hl_default_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __hashlite_default_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASHLITE_DEFAULT_ACTION);
}

/***************************************************************************
 * HashLite wrap functions
 ***************************************************************************/

/* Set address of hash key to AAL local_data.
 * Parameter:
 *   hash_idx: This is also nexthop_id. It represents the hash_key is in Hash Table or Overflow Table.
 *   hash_key: Pointer of hash key.
 */
static int __aal_hashlite_hash_key_set(unsigned int hash_idx, aal_hash_key_t *hash_key)
{
	aal_table_t *table = NULL;
	unsigned int idx = hash_idx & HASHLITE_INDEX_MASK;

	table = (hash_idx & HASHLITE_OVERFLOW_FLAG) ? &hl_overflow_table : &hl_hash_table;

	if (table->entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_ENTRYNOTFOUND;

	table->entry[idx].local_data = (unsigned long) hash_key;

	return AAL_E_OK;
}

/* Get hash key from AAL local_data.
 * Parameter:
 *   hash_idx: This is also nexthop_id. It represents the hash_key is in Hash Table or Overflow Table.
 *   hash_key: Returned hash key.
 */
static int __aal_hashlite_hash_key_get(unsigned int hash_idx, aal_hash_key_t **hash_key)
{
	aal_table_t *table = NULL;
	unsigned int idx = hash_idx & HASHLITE_INDEX_MASK;

	table = (hash_idx & HASHLITE_OVERFLOW_FLAG) ? &hl_overflow_table : &hl_hash_table;

	if (hash_key == NULL) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_ENTRYNOTFOUND;

	*hash_key = (aal_hash_key_t *) table->entry[idx].local_data;

	return AAL_E_OK;
}


/* Add hash entry.
 * What this function does:
 * 1. Validate hash_key according to hash_mask.
 * 2. Calculate hash_value (crc32).
 * 3. Add hash value to Hash Table. If overflowed, add to Overflow Table.
 * 4. Add hash action to Hash Action Table. If overflowed, add to Overflow Action Table.
 * 5. Set aging to enable hash.
 * 6. Return rslt_idx.
 *
 * NOTE:
 * 1. hash_key must not freed by caller.
 * 2. After hash is added successfully,
 *    caller MUST call aal_entry_local_data_set() to save addr of hash_key.
 */
static int __aal_hashlite_hash_add(
	aal_hash_key_t *hash_key, aal_hash_mask_t *hash_mask,
	aal_hashlite_hash_action_entry_t *hash_action, unsigned int actgrp_mask,
	aal_hashlite_aging_e aging, unsigned int *rslt_hash_idx)
{
	int ret = AAL_E_OK, ret2;
	bool overflowed = false;
	aal_hash_key_t *tmp_hash_key = NULL;
	aal_hash_value_t hash_value;
	unsigned int idx;

	if ((hash_key == NULL) || (hash_mask == NULL) || (hash_action == NULL) || (rslt_hash_idx == NULL))
		return AAL_E_NULLPTR;

	if ((aging < HASHLITE_AGING_0) || (aging > HASHLITE_AGING_15))
		return AAL_E_OUTRANGE;

	/* Fill hash_key->hkey_id if it is required */
	if (hash_mask->hkey_id == 0) {
		ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, hash_mask, &idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
			return ret;
		}
		hash_key->hkey_id = idx;
	}

	/* calculate hash value */

	memset(&hash_value, 0, sizeof(hash_value));
	if ((ret = hash_value_calculate(hash_key, hash_mask, &hash_value)) != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to calculate hash_value. (ret=%d)\n", __func__, ret);
		return ret;
	}

	if ((tmp_hash_key = ca_malloc_trace(sizeof(aal_hash_key_t), GFP_ATOMIC, true)) == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory.\n", __func__);
		return AAL_E_NOMEM;
	}
	memcpy(tmp_hash_key, hash_key, sizeof(aal_hash_key_t));

	/* add hash value to Hash Table */
	ret = hl_hash_tbl_entry_add(&hash_value, &idx);
	if (ret != AAL_E_OK) {
		if (ret == AAL_E_TBLFULL) {
			/* add hash value to Overflow Table */
			overflowed = true;
			ret = hl_overflow_tbl_entry_add(&hash_value, &idx);
			if (ret == AAL_E_OK) {
				ret2 = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, idx, aging);
				if (ret2 != AAL_E_OK)
					printk(KERN_ERR "%s: Fail to set aging value %d to existing overflow index %u. (ret %d)\n", __func__, aging, idx, ret2);
			} else if (ret == AAL_E_EXIST) {
				ret2 = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, idx, aging);
				if (ret2 != AAL_E_OK)
					printk(KERN_ERR "%s: Fail to set aging value %d to existing overflow index %u. (ret %d)\n", __func__, aging, idx, ret2);
				*rslt_hash_idx = idx | HASHLITE_OVERFLOW_FLAG;
				goto _error_hash_add;
			} else {
				printk(KERN_ERR "%s: Fail to add hash (crc32 0x%08x, crc16 0x%04x) to overflow table. (ret %d)\n",
					__func__, hash_value.crc32, hash_value.crc16, ret);
				goto _error_hash_add;
			}
		} else if (ret == AAL_E_EXIST) {
			ret2 = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, idx, aging);
			if (ret2 != AAL_E_OK)
				printk(KERN_ERR "%s: Fail to set aging value %d to existing hash index %u. (ret %d)\n", __func__, aging, idx, ret2);
			*rslt_hash_idx = idx;
			goto _error_hash_add;
		} else {
			printk(KERN_ERR "%s: Fail to add hash (crc32 0x%08x, crc16 0x%04x) to hash table. (ret %d)\n",
				__func__, hash_value.crc32, hash_value.crc16, ret);
			goto _error_hash_add;
		}
	}

	if (overflowed == false) {

		/* add hash action to Hash Action Table */
		ret = hl_hash_action_entry_add_by_idx(hash_action, idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to add hash action at index %u. (ret %d)\n", __func__, idx, ret);
			hl_hash_tbl_entry_del(idx);
			goto _error_hash_add;
		}

		/* set action group mask to local_data */
		ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_HASH_ACTION, idx, actgrp_mask);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set local data for hash action idx %u. (ret=%d)\n", __func__, idx, ret);
			hl_hash_tbl_entry_del(idx);
			hl_hash_action_entry_del(idx);
			goto _error_hash_add;
		}

		/* set age to enable the hash entry in Hash Table */
		ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, idx, aging);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set aging value %d to hash index %u. (ret %d)\n", __func__, aging, idx, ret);
			hl_hash_tbl_entry_del(idx);
			hl_hash_action_entry_del(idx);
			goto _error_hash_add;
		}

		*rslt_hash_idx = idx;

		ret = __aal_hashlite_hash_key_set(*rslt_hash_idx, tmp_hash_key);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set hash_key %p to hash index %u. (ret %d)\n", __func__, tmp_hash_key, *rslt_hash_idx, ret);
			hl_hash_tbl_entry_del(idx);
			hl_hash_action_entry_del(idx);
			goto _error_hash_add;
		}

	} else {

		/* add hash action to Overflow Action Table */
		ret = hl_overflow_action_entry_add_by_idx(hash_action, idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to add overflow action at index %u. (ret %d)\n", __func__, idx, ret);
			hl_overflow_tbl_entry_del(idx);
			goto _error_hash_add;
		}

		/* set action group mask to local_data */
		ret = aal_entry_local_data_set(AAL_TABLE_HASHLITE_OVERFLOW_ACTION, idx, actgrp_mask);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set local data for overflow action idx %u. (ret=%d)\n", __func__, idx, ret);
			hl_overflow_tbl_entry_del(idx);
			hl_overflow_action_entry_del(idx);
			goto _error_hash_add;
		}

		/* set age to enable the hash entry in Overflow Table */
		ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_OVERFLOW_TBL, idx, aging);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set aging value %d to overflow index %u. (ret %d)\n", __func__, aging, idx, ret);
			hl_overflow_tbl_entry_del(idx);
			hl_overflow_action_entry_del(idx);
			goto _error_hash_add;
		}

		*rslt_hash_idx = idx | HASHLITE_OVERFLOW_FLAG;

		if ((ret = __aal_hashlite_hash_key_set(*rslt_hash_idx, tmp_hash_key)) != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set hash_key %p to overflow index %u. (ret %d)\n", __func__, tmp_hash_key, *rslt_hash_idx, ret);
			hl_overflow_tbl_entry_del(idx);
			hl_overflow_action_entry_del(idx);
			goto _error_hash_add;
		}
	}

	if (overflowed == true) {
		DBG(printk(KERN_INFO "Hash is added to HashLite Overflow Table / Overflow Action Table, index=%u, %u.\n", idx, *rslt_hash_idx));
	} else {
		DBG(printk(KERN_INFO "Hash is added to HashLite Hash Table / Action Table, index=%u.\n", idx));
	}

	return ret;

_error_hash_add:
	if (tmp_hash_key != NULL)
		ca_free_trace(tmp_hash_key);

	return ret;
}

/* Delete hash entry.
 * What this function does:
 * 1. Delete hash entry in Hash Table or Overflow Table.
 * 2. Delete hash action in Action Table or Overflow Action Table.
 *
 * NOTE:
 * 1. Before calling this function, caller MUST call aal_entry_local_data_get()
 *    to get addr of hash_key, and then free hash_key.
 */
static int __aal_hashlite_hash_delete(unsigned int hash_idx)
{
	unsigned int idx = hash_idx & HASHLITE_INDEX_MASK;
	int ret;
	aal_hash_key_t *tmp_hash_key = NULL;

	if ((ret = __aal_hashlite_hash_key_get(hash_idx, &tmp_hash_key)) != AAL_E_OK) {
		DBG(printk(KERN_INFO "Can not get hash_key, index=%u.\n", hash_idx));
	}
	if (tmp_hash_key != NULL) {
		ca_free_trace(tmp_hash_key);
		__aal_hashlite_hash_key_set(hash_idx, NULL);
	}

	if (hash_idx & HASHLITE_OVERFLOW_FLAG) {

		DBG(printk(KERN_INFO "Delete hash in HashLite Overflow Table / Overflow Action Table, index=%u.\n", idx));

		aal_hashlite_age_set(AAL_TABLE_HASHLITE_OVERFLOW_TBL, idx, HASHLITE_AGING_INVALID);

		if ((ret = hl_overflow_tbl_entry_del(idx)) != AAL_E_OK)
			return ret;

		aal_entry_local_data_set(AAL_TABLE_HASHLITE_OVERFLOW_ACTION, idx, 0);

		if ((ret = hl_overflow_action_entry_del(idx)) != AAL_E_OK)
			return ret;

	} else {

		DBG(printk(KERN_INFO "Delete hash in HashLite Hash Table / Action Table, index=%u.\n", idx));

		aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, idx, HASHLITE_AGING_INVALID);

		if ((ret = hl_hash_tbl_entry_del(idx)) != AAL_E_OK)
			return ret;

		aal_entry_local_data_set(AAL_TABLE_HASHLITE_HASH_ACTION, idx, 0);

		if ((ret = hl_hash_action_entry_del(idx)) != AAL_E_OK)
			return ret;

	}

	return AAL_E_OK;
}

int aal_hashlite_hash_add(
	ca_device_id_t device_id,
	aal_hash_key_t *hash_key, aal_hash_mask_t *hash_mask,
	aal_hashlite_hash_action_entry_t *hash_action, unsigned int actgrp_mask,
	aal_hashlite_aging_e aging, unsigned int *rslt_hash_idx)
{
	return __aal_hashlite_hash_add(hash_key, hash_mask, hash_action, actgrp_mask, aging, rslt_hash_idx);
}

int aal_hashlite_hash_delete(ca_device_id_t device_id, unsigned int hash_idx)
{
	return __aal_hashlite_hash_delete(hash_idx);
}

int aal_hashlite_hash_key_set(ca_device_id_t device_id, unsigned int hash_idx, aal_hash_key_t *hash_key)
{
	return __aal_hashlite_hash_key_set(hash_idx, hash_key);
}

int aal_hashlite_hash_key_get(ca_device_id_t device_id, unsigned int hash_idx, aal_hash_key_t **hash_key)
{
	return __aal_hashlite_hash_key_get(hash_idx, hash_key);
}

/***************************************************************************
 * Hash-Lite Engine Initialization
 ***************************************************************************/

int aal_hashlite_init(void)
{
	uint64_t quotient;
	uint32_t remainder;
	int ret;
	unsigned int aging_time;
	struct ca_soc_data soc_data;

	aal_hashlite_mem_init();

	if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
		soc_data.core_clk = 250000000; /* assume core clk is 250MHz */
#if 0
	/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
	g_max_aging_time = ((uint64_t) 0x3FFFFFFF * HASHLITE_AGING_UNIT * HASHLITE_AGING_START / soc_data.core_clk);
#else
	ca_div((uint64_t) 0x3FFFFFFF * HASHLITE_AGING_UNIT * HASHLITE_AGING_START, soc_data.core_clk, &quotient, &remainder);
	g_max_aging_time = (unsigned int) quotient;
#endif

	ret = scfg_read(0, CFG_ID_NEXTHOP_AGING_TIME, sizeof(aging_time), (ca_uint8_t *) &aging_time);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_NEXTHOP_AGING_TIME. (ret=%d)\n", __func__, ret);
		aging_time = 300; /* seconds */
	}
	aal_hashlite_aging_timer_set(aging_time);

	hashlite_hashmask_init();

#if defined(CONFIG_LUNA_G3_SERIES)

#else
	if ((ret = hashlite_profile_init()) != AAL_E_OK)
		return ret;
#endif

	if ((ret = hashlite_dbl_chk_fail_init()) != AAL_E_OK)
		return ret;

	return AAL_E_OK;
}

#ifdef INDEXED_INTERFACE_TABLE
#define IDX_INTF_IPV4_INDEX_MAX (HASHLITE_IDX_INTF_ENTRY_MAX / 4)

static aal_table_t idx_intf_mac_table;
static aal_table_t idx_intf_ipv4_table;
static aal_table_t idx_intf_ipv6_table;
spinlock_t idx_intf_lock;
unsigned char free_idx_intf_entry_pool[HASHLITE_IDX_INTF_ENTRY_MAX]; /*4 bits: 0x0~0xF, 0: free, 1: used*/

enum {
	 MAC_TBL_TYPE,
	 IPV4_TBL_TYPE,
	 IPV6_TBL_TYPE
};

/* Set the value to HW table entry with given index and table type */
int idx_intf_hw_entry_set(aal_table_t *table, unsigned int idx, int table_type)
{
	int i, skip_reg_num;
	uint32_t val = 0;
	unsigned int *ptr;
	ca_device_id_t device_id;
	unsigned int hw_idx;
	idx_intf_entry_t hw_entry;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_NULLPTR;

	device_id = (table->table_id < __AAL_TABLE_8279_START__) ? 0 : 1;

	/* read entire HW entry first */
	hw_idx = (table_type != IPV4_TBL_TYPE)? idx: (idx >> 2);
	table->hw_entry_get(table, idx, &hw_entry);

	switch (table_type) {
	case MAC_TBL_TYPE:
		skip_reg_num = 0;
		break;
	case IPV4_TBL_TYPE:
		skip_reg_num = 2 + 3 - (idx % 4);
		break;
	case IPV6_TBL_TYPE:
		skip_reg_num = 2;
		break;
	default:
		return AAL_E_OUTRANGE;
	}
	ptr = table->entry[idx].data;
	/* write the value to the specific data registers */
	for (i = 0; i < table->data_reg_num ; i++)
		ca_ne_reg_write(device_id, ptr[i], table->access_reg + ((table->data_reg_num - i + skip_reg_num) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= 8 << 12;		/* Indexed Interface */
	val |= hw_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
}

int idx_intf_hw_entry_get(aal_table_t *table, unsigned int idx, void *data, int table_type)
{
	int i, skip_reg_num;
	uint32_t val = 0;
	unsigned int *ptr;
	ca_device_id_t device_id;
	unsigned int hw_idx;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	device_id = (table->table_id < __AAL_TABLE_8279_START__) ? 0 : 1;

	hw_idx = (table_type != IPV4_TBL_TYPE)? idx: (idx >> 2);

	switch (table_type) {
	case MAC_TBL_TYPE:
		skip_reg_num = 0;
		break;
	case IPV4_TBL_TYPE:
		skip_reg_num = 2 + 3 - (idx % 4);
		break;
	case IPV6_TBL_TYPE:
		skip_reg_num = 2;
		break;
	default:
		return AAL_E_OUTRANGE;
	}
	ptr = data;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= 8 << 12;		/* Indexed Interface */
	val |= hw_idx;
	ca_ne_reg_write(device_id, val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = ca_ne_reg_read(device_id, table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* HW limitation, read all first */
	for (i = 0; i < 6; i++)
		ca_ne_reg_read(device_id, table->access_reg + ((6 - i) << 2));

	for (i = 0; i < table->data_reg_num; i++)
		ptr[i] = ca_ne_reg_read(device_id, table->access_reg + ((table->data_reg_num - i + skip_reg_num) << 2));

	return AAL_E_OK;
}

int idx_intf_mac_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	return idx_intf_hw_entry_set(table, idx, MAC_TBL_TYPE);
}

int idx_intf_ipv4_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	return idx_intf_hw_entry_set(table, idx, IPV4_TBL_TYPE);
}

int idx_intf_ipv6_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	return idx_intf_hw_entry_set(table, idx, IPV6_TBL_TYPE);
}

int idx_intf_mac_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	return idx_intf_hw_entry_get(table, idx, data, MAC_TBL_TYPE);
}

int idx_intf_ipv4_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	return idx_intf_hw_entry_get(table, idx, data, IPV4_TBL_TYPE);
}

int idx_intf_ipv6_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	return idx_intf_hw_entry_get(table, idx, data, IPV6_TBL_TYPE);
}

int idx_intf_hw_entry_clear(aal_table_t *table, unsigned int idx, void *data)
{
	/* Do nothing, don't need to clear data. */
	return AAL_E_OK;
}

int aal_idx_intf_init(void)
{
	spin_lock_init(&idx_intf_lock);

	__aal_table_register(AAL_TABLE_IDX_INTF_MAC, &idx_intf_mac_table);
	__aal_table_register(AAL_TABLE_IDX_INTF_IPV4, &idx_intf_ipv4_table);
	__aal_table_register(AAL_TABLE_IDX_INTF_IPV6, &idx_intf_ipv6_table);

	return 0;
}

void aal_idx_intf_exit(void)
{
	__aal_table_unregister(AAL_TABLE_IDX_INTF_MAC);
	__aal_table_unregister(AAL_TABLE_IDX_INTF_IPV4);
	__aal_table_unregister(AAL_TABLE_IDX_INTF_IPV6);
}

int aal_idx_intf_mac_entry_add(void *entry, unsigned int *rslt_idx)
{
	DBG(printk("%s: %d\n", __func__, __LINE__));
	return generic_entry_add(&idx_intf_mac_table, entry, rslt_idx);
}

int aal_idx_intf_mac_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&idx_intf_mac_table, entry, rslt_idx);
}

int aal_idx_intf_mac_entry_del(unsigned int idx)
{
	DBG(printk("%s: %d\n", __func__, __LINE__));
	return generic_entry_del(&idx_intf_mac_table, idx);
}

int aal_idx_intf_mac_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&idx_intf_mac_table, idx, entry);
}

void aal_idx_intf_mac_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &idx_intf_mac_table;
	idx_intf_mac_entry_t entry;
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

	printk(KERN_INFO "|===== %s ==================================================|\n", table->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = aal_idx_intf_mac_entry_get_by_idx(i, &entry);
		} else {
			ret = table->hw_entry_get(&idx_intf_mac_table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "  |- MAC DA %pMR\n", entry.mac_da);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t idx_intf_mac_table= {
	.table_id = AAL_TABLE_IDX_INTF_MAC,
	.table_name = "Indexed Interface MAC DA",
	.max_entry = HASHLITE_IDX_INTF_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(idx_intf_mac_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 2,
	.op = {
		.entry_add = aal_idx_intf_mac_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_idx_intf_mac_entry_del,
		.entry_set = NULL,
		.entry_get = aal_idx_intf_mac_entry_get,
		.entry_get_by_idx = aal_idx_intf_mac_entry_get_by_idx,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = aal_idx_intf_mac_entry_print,
	},
	.shared_lock = &idx_intf_lock,
	.hw_entry_set = idx_intf_mac_hw_entry_set,
	.hw_entry_get = idx_intf_mac_hw_entry_get,
	.hw_entry_clear = idx_intf_hw_entry_clear
};

static int aal_idx_intf_entry_alloc(int table_type)
{
	int i, bit, idx;
	unsigned long flags;

	spin_lock_irqsave(&idx_intf_lock, flags);

	if (table_type == IPV4_TBL_TYPE) {
		for (i = 0; i < IDX_INTF_IPV4_INDEX_MAX; i++) {
			if (free_idx_intf_entry_pool[i] < 0xF) {
				bit = 0xF - free_idx_intf_entry_pool[i];
				bit = (bit & 0x1)?0:(bit & 0x2)?1:(bit & 0x4)?2:(bit & 0x8)?3:0;
				idx = i * 4 + bit;
				free_idx_intf_entry_pool[i] |= 1 << bit;
				spin_unlock_irqrestore(&idx_intf_lock, flags);
				return idx;
			}
		}
	} else {
		for (i = IDX_INTF_IPV4_INDEX_MAX; i < HASHLITE_IDX_INTF_ENTRY_MAX; i++) {
			if (free_idx_intf_entry_pool[i] == 0) {
				free_idx_intf_entry_pool[i] = 1;
				spin_unlock_irqrestore(&idx_intf_lock, flags);
				return i;
			}
		}
		for (i = 0; i < IDX_INTF_IPV4_INDEX_MAX; i++) {
			if (free_idx_intf_entry_pool[i] == 0) {
				free_idx_intf_entry_pool[i] = 0xF;
				spin_unlock_irqrestore(&idx_intf_lock, flags);
				return i;
			}
		}
	}

	spin_unlock_irqrestore(&idx_intf_lock, flags);

	return -1;
}

static int aal_idx_intf_entry_free(int table_type, int idx)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&idx_intf_lock, flags);

	if (table_type == IPV4_TBL_TYPE) {
		free_idx_intf_entry_pool[idx / 4] &= ~(1 << (idx %4));
	} else {
		free_idx_intf_entry_pool[idx] = 0;
	}

	spin_unlock_irqrestore(&idx_intf_lock, flags);

	return 0;
}

int aal_idx_intf_ipv4_entry_add(void *entry, unsigned int *rslt_idx)
{
	int ret;
	DBG(printk("%s: %d\n", __func__, __LINE__));

	ret = generic_entry_get(&idx_intf_ipv4_table, entry, rslt_idx);
	if (ret == AAL_E_OK) {
		ret = generic_entry_add_by_idx(&idx_intf_ipv4_table, entry, *rslt_idx);
	} else {
		*rslt_idx = aal_idx_intf_entry_alloc(IPV4_TBL_TYPE);
		if (*rslt_idx >= 0)
			ret = generic_entry_add_by_idx(&idx_intf_ipv4_table, entry, *rslt_idx);
		else
			ret = AAL_E_TBLFULL;
	}

	return ret;
}

int aal_idx_intf_ipv4_entry_del(unsigned int idx)
{
	int ret;

	ret = generic_entry_del(&idx_intf_ipv4_table, idx);

	if (ret == AAL_E_OK) {
		aal_idx_intf_entry_free(IPV4_TBL_TYPE, idx);
	}
	return ret;
}

int aal_idx_intf_ipv4_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&idx_intf_ipv4_table, entry, rslt_idx);
}

int aal_idx_intf_ipv4_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&idx_intf_ipv4_table, idx, entry);
}

void aal_idx_intf_ipv4_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &idx_intf_ipv4_table;
	idx_intf_ipv4_entry_t entry;
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

	printk(KERN_INFO "|===== %s ==================================================|\n", table->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = aal_idx_intf_ipv4_entry_get_by_idx(i, &entry);
		} else {
			ret = table->hw_entry_get(&idx_intf_ipv4_table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "  |- IPv4 Addr %pI4hl\n", &entry.ipv4);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t idx_intf_ipv4_table= {
	.table_id = AAL_TABLE_IDX_INTF_IPV4,
	.table_name = "Indexed Interface IPV4 Address",
	.max_entry = HASHLITE_IDX_INTF_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(idx_intf_ipv4_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 1,
	.op = {
		.entry_add = aal_idx_intf_ipv4_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_idx_intf_ipv4_entry_del,
		.entry_set = NULL,
		.entry_get = aal_idx_intf_ipv4_entry_get,
		.entry_get_by_idx = aal_idx_intf_ipv4_entry_get_by_idx,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = aal_idx_intf_ipv4_entry_print,
	},
	.shared_lock = &idx_intf_lock,
	.hw_entry_set = idx_intf_ipv4_hw_entry_set,
	.hw_entry_get = idx_intf_ipv4_hw_entry_get,
	.hw_entry_clear = idx_intf_hw_entry_clear
};

int aal_idx_intf_ipv6_entry_add(void *entry, unsigned int *rslt_idx)
{
	int ret;

	DBG(printk("%s: %d\n", __func__, __LINE__));
	ret = generic_entry_get(&idx_intf_ipv6_table, entry, rslt_idx);
	if (ret == AAL_E_OK) {
		ret = generic_entry_add_by_idx(&idx_intf_ipv6_table, entry, *rslt_idx);
	} else {
		*rslt_idx = aal_idx_intf_entry_alloc(IPV6_TBL_TYPE);
		if (*rslt_idx >= 0)
			ret = generic_entry_add_by_idx(&idx_intf_ipv6_table, entry, *rslt_idx);
		else
			ret = AAL_E_TBLFULL;
	}

	return ret;
}

int aal_idx_intf_ipv6_entry_del(unsigned int idx)
{
	DBG(printk("%s: %d\n", __func__, __LINE__));
	aal_idx_intf_entry_free(IPV6_TBL_TYPE, idx);
	return generic_entry_del(&idx_intf_ipv6_table, idx);
}

int aal_idx_intf_ipv6_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&idx_intf_ipv6_table, entry, rslt_idx);
}

int aal_idx_intf_ipv6_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&idx_intf_ipv6_table, idx, entry);
}

void aal_idx_intf_ipv6_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &idx_intf_ipv6_table;
	idx_intf_ipv6_entry_t entry;
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

	printk(KERN_INFO "|===== %s ==================================================|\n", table->table_name);

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = aal_idx_intf_ipv6_entry_get_by_idx(i, &entry);
		} else {
			ret = table->hw_entry_get(&idx_intf_ipv6_table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "  |- IPv6 Addr %08x %08x %08x %08x\n",
					entry.ipv6_3, entry.ipv6_2, entry.ipv6_1, entry.ipv6_0);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t idx_intf_ipv6_table= {
	.table_id = AAL_TABLE_IDX_INTF_IPV6,
	.table_name = "Indexed Interface IPV6 Address",
	.max_entry = HASHLITE_IDX_INTF_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(idx_intf_ipv6_entry_t),
	.access_reg = L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS,
	.data_reg_num = 4,
	.op = {
		.entry_add = aal_idx_intf_ipv6_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = aal_idx_intf_ipv6_entry_del,
		.entry_set = NULL,
		.entry_get = aal_idx_intf_ipv6_entry_get,
		.entry_get_by_idx = aal_idx_intf_ipv6_entry_get_by_idx,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = aal_idx_intf_ipv6_entry_print,
	},
	.shared_lock = &idx_intf_lock,
	.hw_entry_set = idx_intf_ipv6_hw_entry_set,
	.hw_entry_get = idx_intf_ipv6_hw_entry_get,
	.hw_entry_clear = idx_intf_hw_entry_clear
};
#endif /* INDEXED_INTERFACE_TABLE */

#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_hashlite_profile_set);
EXPORT_SYMBOL(aal_hashlite_aging_timer_set);
EXPORT_SYMBOL(aal_hashlite_hash_add);
EXPORT_SYMBOL(aal_hashlite_hash_delete);
#endif

