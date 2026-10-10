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
 * ca_aal.c: AAL exported API
 */
#include <generated/ca_ne_autoconf.h>
#include "aal_table.h"
#include "aal_ni.h"
#include "aal_mcast.h"

#include "aal_l3_if.h"
#include "aal_l3_cam.h"
#include "aal_hash.h"
#include "aal_hashlite.h"
#include "aal_lpm.h"
#include "aal_l3_pe.h"
#include "aal_l3qm.h"
#include "aal_mcast.h"
#include "aal_l3_cls.h"

void aal_array_reverse_copy(unsigned char *dst, unsigned char *src, unsigned int length)
{
	int i;

	for (i = 0; i < length; i++) {
		dst[i] = src[length - i - 1];
	}
}

#define WORD_BITS       (sizeof(long) * 8)

void aal_array_nonaligned_reverse_copy(unsigned char *table_entry, unsigned int bit_offset, unsigned char *src, unsigned int bytes)
{
	unsigned long *cur_word = (unsigned long *) table_entry;
	unsigned long *src_word = (unsigned long *) src + bytes - sizeof(long);
	int tail_bits, body_words, head_bits, i;

	cur_word += bit_offset / WORD_BITS;

	head_bits = WORD_BITS - (bit_offset % WORD_BITS);
	body_words = (bytes * 8 - head_bits) / WORD_BITS;
	tail_bits = (bytes * 8 - head_bits) % WORD_BITS;

	/* set tail */
	*cur_word |= (*src_word << (WORD_BITS - tail_bits));
	cur_word++;

	/* set body */
	for (i = 0; i < body_words; i++) {
		*cur_word = (*src_word >> tail_bits) | (*(src_word - 1) << (WORD_BITS - tail_bits));
		cur_word++;
		src_word--;
	}

	/* set head */
	*cur_word |= (*src_word >> (WORD_BITS - head_bits));
}

int aal_entry_add(aal_table_e table_id, void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_add != NULL)
		return table->op.entry_add(entry, rslt_idx);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_add_by_idx(aal_table_e table_id, void *entry, unsigned int idx)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_add_by_idx != NULL)
		return table->op.entry_add_by_idx(entry, idx);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_del(aal_table_e table_id, unsigned int idx)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_del != NULL)
		return table->op.entry_del(idx);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_set(aal_table_e table_id, unsigned int idx, void *entry)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_set != NULL)
		return table->op.entry_set(idx, entry);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_get(aal_table_e table_id, void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_get != NULL)
		return table->op.entry_get(entry, rslt_idx);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_get_by_idx(aal_table_e table_id, unsigned int idx, void *entry)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_get_by_idx != NULL)
		return table->op.entry_get_by_idx(idx, entry);
	else
		return AAL_E_OPNOTSUPP;
}

int aal_entry_print(aal_table_e table_id, unsigned int idx, unsigned int count)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.entry_print != NULL) {
		table->op.entry_print(idx, count);
		return AAL_E_OK;
	} else {
		return AAL_E_OPNOTSUPP;
	}
}

#if defined(CONFIG_LUNA_G3_SERIES)
int aal_used_entry_print(aal_table_e table_id, unsigned int idx, unsigned int count)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.used_entry_print != NULL) {
		table->op.used_entry_print(idx, count);
		return AAL_E_OK;
	} else {
		return AAL_E_OPNOTSUPP;
	}
}
EXPORT_SYMBOL(aal_used_entry_print);
EXPORT_SYMBOL(aal_entry_add);
EXPORT_SYMBOL(aal_entry_add_by_idx);
EXPORT_SYMBOL(aal_entry_del);
EXPORT_SYMBOL(aal_entry_get);
EXPORT_SYMBOL(aal_entry_print);
#endif

int aal_entry_local_data_set(aal_table_e table_id, unsigned int idx, unsigned long local_data)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;
	if (table->entry == NULL)
		return AAL_E_NULLPTR;
	if (table->entry[idx].data == NULL)
		return AAL_E_ENTRYNOTFOUND;

	table->entry[idx].local_data = local_data;

	return AAL_E_OK;
}

int aal_entry_local_data_get(aal_table_e table_id, unsigned int idx, unsigned long *local_data)
{
	aal_table_t *table;

	if (local_data == NULL)
		return AAL_E_NULLPTR;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry)
		return AAL_E_OUTRANGE;
	if (table->entry == NULL)
		return AAL_E_NULLPTR;
	if (table->entry[idx].data == NULL)
		return AAL_E_ENTRYNOTFOUND;

	*local_data = table->entry[idx].local_data;

	return AAL_E_OK;
}

int aal_table_flush(aal_table_e table_id)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.table_flush != NULL)
		return table->op.table_flush();
	else
		return AAL_E_OPNOTSUPP;
}

int aal_table_get_avail_count(aal_table_e table_id)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	if (table->op.get_avail_count != NULL)
		return table->op.get_avail_count();
	else
		return AAL_E_OPNOTSUPP;
}

int aal_table_get_used_count(aal_table_e table_id)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	return table->used_entry;
}

int aal_table_get_max_count(aal_table_e table_id)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return AAL_E_TBLNOTEXIST;

	return table->max_entry;
}

char *aal_table_name_get(aal_table_e table_id)
{
	aal_table_t *table;

	table = __aal_table_get(table_id);
	if (table == NULL)
		return "*** N/A ***";

	return table->table_name;
}

