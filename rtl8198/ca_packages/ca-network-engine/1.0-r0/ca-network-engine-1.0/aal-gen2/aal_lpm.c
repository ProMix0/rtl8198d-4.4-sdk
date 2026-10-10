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
 * aal_lpm.c: Hardware Abstraction Layer for LPM to access hardware regsiters
 *
 */

#include <generated/ca_ne_autoconf.h>
#include <linux/sched.h>

#include "ca_types.h"
#include "aal_table.h"
#include "aal_lpm.h"
#include "aal_hashlite.h"
#include "aal_port.h"
#include "ca_mut.h"
#include "scfg.h"

#define TABLE_TRY_TIMEOUT	1000

static aal_table_t lpm_tbl_table;

static int __aal_lpm_tbl_entry_get_by_idx(unsigned int idx, void *entry);

/***************************************************************************
 * internal functions to write LPM HW entry
 ***************************************************************************/

/* Clear the entry with given index and table */
static int lpm_hw_entry_clear(aal_table_t *table, unsigned int idx)
{
	int i;
	uint32_t val = 0;
	unsigned int hw_idx;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	hw_idx = idx;

	/* write the value to the specific data registers */
	for (i = 0; i < table->data_reg_num ; i++)
		CA_NE_REG_WRITE(0, table->access_reg + ((table->data_reg_num - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= hw_idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
} /* lpm_hw_entry_clear */

/* Set the value to HW table entry with given index and table type */
static int lpm_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	int i;
	uint32_t val = 0;
	unsigned int *ptr;
	unsigned int hw_idx;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_NULLPTR;

	hw_idx = idx;

	ptr = table->entry[idx].data;
	/* write the value to the specific data registers */
	for (i = 0; i < table->data_reg_num ; i++)
		CA_NE_REG_WRITE(ptr[i], table->access_reg + ((table->data_reg_num - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= hw_idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return AAL_E_OK;
} /* lpm_hw_entry_set */

/* Get the value from HW table entry with given index and table type */
static int lpm_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	int i;
	uint32_t val = 0;
	unsigned int *ptr;
	unsigned int hw_idx;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	hw_idx = idx;

	ptr = data;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= hw_idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	for (i = 0; i < table->data_reg_num; i++)
		ptr[i] = CA_NE_REG_READ(table->access_reg + ((table->data_reg_num - i) << 2));

	return AAL_E_OK;
} /* lpm_hw_entry_get */

/***************************************************************************
 * internal functions
 ***************************************************************************/

/* Validate the entry[], mask the entry[].data according to entry[].mask */
static int lpm_entry_validate(lpm_tbl_entry_t *entry)
{
	int i;

	if (entry == NULL)
		return AAL_E_NULLPTR;

	switch (entry[0].profile) {
	case LPM_PROFILE_0:
	case LPM_PROFILE_1:
	case LPM_PROFILE_2:
	case LPM_PROFILE_3:
		break;
	default:
		return AAL_E_INVALID;
	}

	if (   ((entry[0].attr == ATTR_IPV4) || (entry[0].attr == ATTR_IPV6_LEN_0_32))
	    && (entry[0].mask > 32) )
		return AAL_E_INVALID;

	if (entry[0].attr == ATTR_IPV6_LEN_33_64) {
		if (entry[1].attr != ATTR_IPV6_LEN_33_64)
			return AAL_E_INVALID;

		/* entry[1] contains MSB of IPv6 addr */
		if ((entry[1].mask != 32) || (entry[0].mask > 32) || (entry[0].mask == 0))
			return AAL_E_INVALID;
	}

	if (entry[0].attr == ATTR_IPV6_LEN_65_128) {
		if (   (entry[1].attr != ATTR_IPV6_LEN_65_128)
		    || (entry[2].attr != ATTR_IPV6_LEN_65_128)
		    || (entry[3].attr != ATTR_IPV6_LEN_65_128) )
			return AAL_E_INVALID;

		/* entry[3] contains MSB of IPv6 addr */
		if ((entry[3].mask != 32) || (entry[2].mask != 32) || (entry[1].mask > 32) || (entry[0].mask > 32))
			return AAL_E_INVALID;

		if ((entry[1].mask != 32) && (entry[0].mask != 0))
			return AAL_E_INVALID;
	}

	/* according to entry[].mask, mask entry[].data for better representation and comparison */
	for (i = 0; i < 4; i++)
		entry[i].data &= 0xffffffff << (32 - entry[i].mask);

	return AAL_E_OK;
}

/* According to attr and profile, search 1 or 2 or 4 empty entries and return the index. */
static int lpm_empty_entry_get(unsigned int attr, unsigned int profile, unsigned int *rslt_idx)
{
	unsigned int first_row, last_row;
	unsigned int start, end;
	int i = 0;

	switch (attr) {
	case ATTR_IPV4:
		aal_lpm_first_row_get(CA_IPV4, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV4, profile, &last_row);
		start = first_row << 5;
		end = (last_row << 5) | 0x1f;
		break;
	case ATTR_IPV6_LEN_0_32:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = (first_row << 5) | 0x1f;
		end = last_row << 5;
		break;
	case ATTR_IPV6_LEN_33_64:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = (first_row << 5) | 0x1f;
		end = last_row << 5;
		break;
	case ATTR_IPV6_LEN_65_128:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = (first_row << 5) | 0x1f;
		end = last_row << 5;
		break;
	}

	if (attr == ATTR_IPV4) {
		for (i = start; i <= end; i++) {
			if (lpm_tbl_table.entry[i].data == NULL) {
				*rslt_idx = i;
				return AAL_E_OK;
			}
		}
	} else if (attr == ATTR_IPV6_LEN_0_32) {
		for (i = start; i >= end; i--) {
			if (lpm_tbl_table.entry[i].data == NULL) {
				*rslt_idx = i;
				return AAL_E_OK;
			}
		}
	} else if (attr == ATTR_IPV6_LEN_33_64) {
		for (i = start - 1; i >= end; i -= 2) {
			if (   (lpm_tbl_table.entry[i].data == NULL)
			    && (lpm_tbl_table.entry[i + 1].data == NULL) ) {
				*rslt_idx = i;
				return AAL_E_OK;
			}
		}
	} else if (attr == ATTR_IPV6_LEN_65_128) {
		for (i = start - 3; i >= end; i -= 4) {
			if (   (lpm_tbl_table.entry[i].data == NULL)
			    && (lpm_tbl_table.entry[i + 1].data == NULL)
			    && (lpm_tbl_table.entry[i + 2].data == NULL)
			    && (lpm_tbl_table.entry[i + 3].data == NULL) ) {
				*rslt_idx = i;
				return AAL_E_OK;
			}
		}
	} else {
		return AAL_E_OPNOTSUPP;
	}

	return AAL_E_TBLFULL;
}

/* Search all entry to see if there is a similar entry.
 * Similar entry means same data/attr/mask, but others may be different.
 */
static int lpm_same_entry_search(unsigned int profile, lpm_tbl_entry_t *data)
{
	aal_table_t *table = &lpm_tbl_table;
	unsigned int first_row, last_row;
	unsigned int start, end, gap = 1;
	int i = 0;
	int ret = AAL_E_OK;

	switch (data[0].attr) {
	case ATTR_IPV4:
		aal_lpm_first_row_get(CA_IPV4, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV4, profile, &last_row);
		start = first_row << 5;
		end = (last_row << 5) | 0x1f;
		gap = 1;
		break;
	case ATTR_IPV6_LEN_0_32:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = last_row << 5; // search from last to first because last# < first#
		end = (first_row << 5) | 0x1f;
		gap = 1;
		break;
	case ATTR_IPV6_LEN_33_64:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = last_row << 5; // search from last to first because last# < first#
		end = (first_row << 5) | 0x1f;
		gap = 2;
		break;
	case ATTR_IPV6_LEN_65_128:
		aal_lpm_first_row_get(CA_IPV6, profile, &first_row);
		aal_lpm_last_row_get(CA_IPV6, profile, &last_row);
		start = last_row << 5; // search from last to first because last# < first#
		end = (first_row << 5) | 0x1f;
		gap = 4;
		break;
	}

	if ((data[0].attr == ATTR_IPV4) || (data[0].attr == ATTR_IPV6_LEN_0_32)) {
		data[0].profile = 0; // reset profile which is not set to register
		for (i = start; i <= end; i += gap) {
			if (table->entry[i].data == NULL) continue;
			if (   (((lpm_tbl_entry_t *) table->entry[i].data)->attr == data[0].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->mask == data[0].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->data == data[0].data) ) {
				ret = AAL_E_EXIST;
				goto _error_same_entry_search;
			}
		}
	}
	if (data[0].attr == ATTR_IPV6_LEN_33_64) {
		data[0].profile = data[1].profile = 0; // reset profile which is not set to register
		for (i = start; i <= end; i += gap) {
			if (table->entry[i].data == NULL) continue;
			if (   (((lpm_tbl_entry_t *) table->entry[i].data)->attr == data[0].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->mask == data[0].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->data == data[0].data)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->attr == data[1].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->mask == data[1].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->data == data[1].data) ) {
				ret = AAL_E_EXIST;
				goto _error_same_entry_search;
			}
		}
	}
	if (data[0].attr == ATTR_IPV6_LEN_65_128) {
		data[0].profile = data[1].profile = data[2].profile = data[3].profile = 0; // reset profile which is not set to register
		for (i = start; i <= end; i += gap) {
			if (table->entry[i].data == NULL) continue;
			if (   (((lpm_tbl_entry_t *) table->entry[i].data)->attr == data[0].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->mask == data[0].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i].data)->data == data[0].data)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->attr == data[1].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->mask == data[1].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i + 1].data)->data == data[1].data)
			    && (((lpm_tbl_entry_t *) table->entry[i + 2].data)->attr == data[2].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i + 2].data)->mask == data[2].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i + 2].data)->data == data[2].data)
			    && (((lpm_tbl_entry_t *) table->entry[i + 3].data)->attr == data[3].attr)
			    && (((lpm_tbl_entry_t *) table->entry[i + 3].data)->mask == data[3].mask)
			    && (((lpm_tbl_entry_t *) table->entry[i + 3].data)->data == data[3].data) ) {
				ret = AAL_E_EXIST;
				goto _error_same_entry_search;
			}
		}
	}

_error_same_entry_search:
	return ret;
}

/* Some LPM data requires 4 entries to be meaningful, but this function only get 1 entry. */
static int lpm_sw_entry_get_one(unsigned int idx, lpm_tbl_entry_t *entry)
{
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t *data = NULL;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	data = table->entry[idx].data;
	if (data == NULL) return AAL_E_ENTRYNOTFOUND;

	memcpy(entry, table->entry[idx].data, table->entry_size);

	return AAL_E_OK;
}

/***************************************************************************
 * function for direct access registers - initialization
 ***************************************************************************/

int aal_lpm_mem_init(void)
{
	L3FE_FE_L3E_LPM_MEM_INI_t val;
	int i;

	val.bf.req_sts = 0x1;
	CA_NE_REG_WRITE(val.wrd, L3FE_FE_L3E_LPM_MEM_INI);

	i = 0;
	do {
		val.wrd = CA_NE_REG_READ(L3FE_FE_L3E_LPM_MEM_INI);
	} while ((0x1 & val.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;      /* ACCESS can't complete */

	return AAL_E_OK;
}

/***************************************************************************
 * function for direct access registers - default action
 ***************************************************************************/

/* Set default t4_ctrl_0 and t4_ctrl_1.
 * These two settings are for LPM search instead of default action.
 * T4ctrl chooses the profile of HashLite Engine after LPM is hit.
 */
int aal_lpm_default_t4ctrl_set(ca_ip_afi_t afi, unsigned int t4ctrl_0, unsigned int t4ctrl_1)
{
	L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL1_t val; /* IPv6 uses same structure */
	unsigned int reg;

	switch (afi) {
	case CA_IPV4:
		reg = L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL1;
		break;

	case CA_IPV6:
		reg = L3FE_FE_L3E_LPM_IPV6_ACTION_CTRL1;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	val.wrd = CA_NE_REG_READ(reg);
	val.bf.t4_ctrl_01 = t4ctrl_0;
	val.bf.t4_ctrl_11 = t4ctrl_1;
	CA_NE_REG_WRITE(val.wrd, reg);

	return AAL_E_OK;
}

/* Get default t4_ctrl_0 and t4_ctrl_1.
 * These two settings are for LPM search instead of default action.
 * T4ctrl chooses the profile of HashLite Engine after LPM is hit.
 */
int aal_lpm_default_t4ctrl_get(ca_ip_afi_t afi, unsigned int *t4ctrl_0, unsigned int *t4ctrl_1)
{
	L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL1_t val; /* IPv6 uses same structure */
	unsigned int reg;

	if ((t4ctrl_0 == NULL) || (t4ctrl_1 == NULL)) return AAL_E_NULLPTR;

	switch (afi) {
	case CA_IPV4:
		reg = L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL1;
		break;

	case CA_IPV6:
		reg = L3FE_FE_L3E_LPM_IPV6_ACTION_CTRL1;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	val.wrd = CA_NE_REG_READ(reg);
	*t4ctrl_0 = val.bf.t4_ctrl_01;
	*t4ctrl_1 = val.bf.t4_ctrl_11;

	return AAL_E_OK;
}

/* Set default LPM entry; it is used while all LPM search fails */
int aal_lpm_default_action_set(ca_ip_afi_t afi, lpm_action_ctrl_t *val)
{
	uint32_t *ptr = (uint32_t *) val;

	if (val == NULL) return AAL_E_NULLPTR;

	switch (afi) {
	case CA_IPV4:
		CA_NE_REG_WRITE(*ptr, L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL0);
		break;

	case CA_IPV6:
		CA_NE_REG_WRITE(*ptr, L3FE_FE_L3E_LPM_IPV6_ACTION_CTRL0);
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

/* Get default LPM entry; it is used while all LPM search fails */
int aal_lpm_default_action_get(ca_ip_afi_t afi, lpm_action_ctrl_t *val)
{
	uint32_t *ptr = (uint32_t *) val;

	if (val == NULL) return AAL_E_NULLPTR;

	switch (afi) {
	case CA_IPV4:
		*ptr = CA_NE_REG_READ(L3FE_FE_L3E_LPM_IPV4_ACTION_CTRL0);
		break;

	case CA_IPV6:
		*ptr = CA_NE_REG_READ(L3FE_FE_L3E_LPM_IPV6_ACTION_CTRL0);
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

/***************************************************************************
 * function for direct access registers - LPM common
 ***************************************************************************/

/* Set first used row of LPM table into FE_L3E_LPM_CTRL_PF# */
int aal_lpm_first_row_set(ca_ip_afi_t afi, unsigned int profile, unsigned int row)
{
	L3FE_FE_L3E_LPM_CTRL_PF0_t val;
	unsigned int addr;

	if (profile >= LPM_PROFILE_MAX) return AAL_E_OUTRANGE;
	if (row >= 32) return AAL_E_OUTRANGE;

	addr = L3FE_FE_L3E_LPM_CTRL_PF0 + profile * 4;
	val.wrd = CA_NE_REG_READ(addr);

	switch (afi) {
	case CA_IPV4:
		val.bf.v4_inc_1st_row = row;
		break;

	case CA_IPV6:
		val.bf.v6_dec_1st_row = row;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	CA_NE_REG_WRITE(val.wrd, addr);

	return AAL_E_OK;
}

/* Get last used row of LPM table from FE_L3E_LPM_CTRL_PF# */
int aal_lpm_first_row_get(ca_ip_afi_t afi, unsigned int profile, unsigned int *row)
{
	L3FE_FE_L3E_LPM_CTRL_PF0_t val;

	if (profile >= LPM_PROFILE_MAX) return AAL_E_OUTRANGE;

	val.wrd = CA_NE_REG_READ(L3FE_FE_L3E_LPM_CTRL_PF0 + profile * 4);

	switch (afi) {
	case CA_IPV4:
		*row = val.bf.v4_inc_1st_row;
		break;

	case CA_IPV6:
		*row = val.bf.v6_dec_1st_row;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

/* Set last used row of LPM table into FE_L3E_LPM_CTRL_PF# */
int aal_lpm_last_row_set(ca_ip_afi_t afi, unsigned int profile, unsigned int row)
{
	L3FE_FE_L3E_LPM_CTRL_PF0_t val;
	unsigned int addr;

	if (profile >= LPM_PROFILE_MAX) return AAL_E_OUTRANGE;
	if (row >= 32) return AAL_E_OUTRANGE;

	addr = L3FE_FE_L3E_LPM_CTRL_PF0 + profile * 4;
	val.wrd = CA_NE_REG_READ(addr);

	switch (afi) {
	case CA_IPV4:
		val.bf.v4_inc_last_row = row;
		break;

	case CA_IPV6:
		val.bf.v6_dec_last_row = row;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	CA_NE_REG_WRITE(val.wrd, addr);

	return AAL_E_OK;
}

/* Get last used row of LPM table from FE_L3E_LPM_CTRL_PF# */
int aal_lpm_last_row_get(ca_ip_afi_t afi, unsigned int profile, unsigned int *row)
{
	L3FE_FE_L3E_LPM_CTRL_PF0_t val;

	if (profile >= LPM_PROFILE_MAX) return AAL_E_OUTRANGE;

	val.wrd = CA_NE_REG_READ(L3FE_FE_L3E_LPM_CTRL_PF0 + profile * 4);

	switch (afi) {
	case CA_IPV4:
		*row = val.bf.v4_inc_last_row;
		break;

	case CA_IPV6:
		*row = val.bf.v6_dec_last_row;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

bool aal_lpm_hit_status_get(unsigned int idx)
{
	uint32_t val = 0;

	if (idx >= LPM_TBL_ENTRY_MAX)
		return false;

	val = CA_NE_REG_READ(L3FE_FE_L3E_LPM_STS0 - (((idx >> 5) & 0x1f) << 2));	// row is idx[9:5]

	if (val & (1 << (idx & 0x1f)))	// col is idx[4:0]
		return true;

	return false;
}

uint32_t aal_lpm_misc_status_get(void)
{
	/* The returned value refers to LPM_MISC_STS_* */
	return CA_NE_REG_READ(L3FE_FE_L3E_LPM_DBG0);
}

/***************************************************************************
 * AAL table, L3FE_FE_L3E_LPM_TBL
 ***************************************************************************/

static int __aal_lpm_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int ret;
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t *data = entry;
	unsigned int profile = 0;

	if ((ret = lpm_entry_validate(data)) != AAL_E_OK) return ret;

	profile = data[0].profile;

	spin_lock_irqsave(&table->lock, flags);

	if ((ret = lpm_same_entry_search(profile, data)) != AAL_E_OK) {
		ret = AAL_E_EXIST;
		goto _error_entry_add;
	}

	/* no similar existing entry, start to add entry */

	if ((ret = lpm_empty_entry_get(data[0].attr, profile, rslt_idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&table->lock, flags);
		return ret;
	}

	/* handle data[0] */
	if ((table->entry[*rslt_idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true)) == NULL) {
		ret = AAL_E_NOMEM;
		goto _error_alloc_0;
	}
	memcpy(table->entry[*rslt_idx].data, &data[0], table->entry_size);
	atomic_set(&table->entry[*rslt_idx].refcnt, 1);
	table->used_entry++;

	lpm_hw_entry_set(table, *rslt_idx);

	/* handle data[1] */
	if ((data[0].attr == ATTR_IPV6_LEN_33_64) || (data[0].attr == ATTR_IPV6_LEN_65_128)) {
		if ((table->entry[(*rslt_idx) + 1].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true)) == NULL) {
			ret = AAL_E_NOMEM;
			goto _error_alloc_1;
		}
		memcpy(table->entry[(*rslt_idx) + 1].data, &data[1], table->entry_size);
		atomic_set(&table->entry[(*rslt_idx) + 1].refcnt, 1);
		table->used_entry++;

		lpm_hw_entry_set(table, *rslt_idx + 1);
	}

	/* handle data[2] and data[3] */
	if (data[0].attr == ATTR_IPV6_LEN_65_128) {
		if ((table->entry[(*rslt_idx) + 2].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true)) == NULL) {
			ret = AAL_E_NOMEM;
			goto _error_alloc_2;
		}
		memcpy(table->entry[(*rslt_idx) + 2].data, &data[2], table->entry_size);
		atomic_set(&table->entry[(*rslt_idx) + 2].refcnt, 1);
		table->used_entry++;

		lpm_hw_entry_set(table, *rslt_idx + 2);

		if ((table->entry[(*rslt_idx) + 3].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true)) == NULL) {
			ret = AAL_E_NOMEM;
			goto _error_alloc_3;
		}
		memcpy(table->entry[(*rslt_idx) + 3].data, &data[3], table->entry_size);
		atomic_set(&table->entry[(*rslt_idx) + 3].refcnt, 1);
		table->used_entry++;

		lpm_hw_entry_set(table, *rslt_idx + 3);
	}

	spin_unlock_irqrestore(&table->lock, flags);

	return AAL_E_OK;

_error_alloc_3:
	ca_free_trace(table->entry[(*rslt_idx) + 3].data);
	atomic_dec(&table->entry[(*rslt_idx) + 2].refcnt);
	table->used_entry--;
_error_alloc_2:
	ca_free_trace(table->entry[(*rslt_idx) + 2].data);
	atomic_dec(&table->entry[(*rslt_idx) + 1].refcnt);
	table->used_entry--;
_error_alloc_1:
	ca_free_trace(table->entry[(*rslt_idx) + 1].data);
	atomic_dec(&table->entry[*rslt_idx].refcnt);
	table->used_entry--;
_error_alloc_0:
	ca_free_trace(table->entry[*rslt_idx].data);
_error_entry_add:
	spin_unlock_irqrestore(&table->lock, flags);
	return ret;
}

static int __aal_lpm_tbl_entry_del(unsigned int idx)
{
	unsigned long flags;
	int i, ret;
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t *data = table->entry[idx].data;
	int round = 1;

	if (idx >= LPM_TBL_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_ENTRYNOTFOUND;
	if ((data->attr == ATTR_IPV6_LEN_33_64) && (idx % 2 != 0)) return AAL_E_ENTRYNOTFOUND;
	if ((data->attr == ATTR_IPV6_LEN_65_128) && (idx % 4 != 0)) return AAL_E_ENTRYNOTFOUND;

	if (data->attr == ATTR_IPV6_LEN_33_64) round = 2;
	if (data->attr == ATTR_IPV6_LEN_65_128) round = 4;

	spin_lock_irqsave(&table->lock, flags);

	for (i = 0; i < round; i++) {
		if (atomic_dec_and_test(&table->entry[idx + i].refcnt)) {
			ca_free_trace(table->entry[idx + i].data);
			table->entry[idx + i].data = NULL;
			table->used_entry--;

			/* clear HW reg */
			if ((ret = lpm_hw_entry_clear(table, idx + i)) != AAL_E_OK) {
				spin_unlock_irqrestore(&table->lock, flags);
				return ret;
			}
		}
	}

	spin_unlock_irqrestore(&table->lock, flags);

	return AAL_E_OK;
}

/* Only allow to set rst_idx & rst_ctrl; it's hard to update different data / mask / attr / profile. */
static int __aal_lpm_tbl_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &lpm_tbl_table;
	int ret;
	lpm_tbl_entry_t *data = entry;
	lpm_tbl_entry_t copied_data[4]; /* copy from parameter 'entry' */
	lpm_tbl_entry_t orig_data[4];
	unsigned int profile = 0;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;
	if ((ret = lpm_entry_validate(data)) != AAL_E_OK) return ret;

	profile = data[0].profile;

	ret = __aal_lpm_tbl_entry_get_by_idx(idx, orig_data);
	if (ret != AAL_E_OK) return ret;

	memcpy(copied_data, data, sizeof(lpm_tbl_entry_t) * 4);

	/* Reset rst_idx/rst_ctrl/valid/profile within copied_data[] and orig_data
	 * because only compare data/mask/attr.
	 */
	copied_data[0].rst_idx = copied_data[1].rst_idx = copied_data[2].rst_idx = copied_data[3].rst_idx = 0;
	copied_data[0].rst_ctrl = copied_data[1].rst_ctrl = copied_data[2].rst_ctrl = copied_data[3].rst_ctrl = 0;
	orig_data[0].rst_idx = orig_data[1].rst_idx = orig_data[2].rst_idx = orig_data[3].rst_idx = 0;
	orig_data[0].rst_ctrl = orig_data[1].rst_ctrl = orig_data[2].rst_ctrl = orig_data[3].rst_ctrl = 0;

	if (memcmp(copied_data, orig_data, sizeof(lpm_tbl_entry_t) * 4) != 0) {
		return AAL_E_INVALID;
	}

	spin_lock_irqsave(&table->lock, flags);

	/* handle data[0] */
	memcpy(table->entry[idx].data, &data[0], table->entry_size);
	lpm_hw_entry_set(table, idx);

	/* handle data[1] */
	if ((data[0].attr == ATTR_IPV6_LEN_33_64) || (data[0].attr == ATTR_IPV6_LEN_65_128)) {
		memcpy(table->entry[idx + 1].data, &data[1], table->entry_size);
		lpm_hw_entry_set(table, idx + 1);
	}

	/* handle data[2] and data[3] */
	if (data[0].attr == ATTR_IPV6_LEN_65_128) {
		memcpy(table->entry[idx + 2].data, &data[2], table->entry_size);
		lpm_hw_entry_set(table, idx + 2);

		memcpy(table->entry[idx + 3].data, &data[3], table->entry_size);
		lpm_hw_entry_set(table, idx + 3);
	}

	spin_unlock_irqrestore(&table->lock, flags);

	return AAL_E_OK;
}

static int __aal_lpm_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	int i, ret;
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t *data = entry;

	if ((ret = lpm_entry_validate(data)) != AAL_E_OK) return ret;

	spin_lock_irqsave(&table->lock, flags);

	ret = AAL_E_ENTRYNOTFOUND;

	if (data[0].attr == ATTR_IPV4) {
		data[0].profile = 0; // reset profile which is not set to register
		for (i = 0; i < table->max_entry; i++) {
			if ((table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, &data[0], table->entry_size))) {
				*rslt_idx = i;
				ret = AAL_E_OK;
				goto _exit_entry_get;
			}
		}
	}

	if (data[0].attr == ATTR_IPV6_LEN_0_32) {
		data[0].profile = 0; // reset profile which is not set to register
		for (i = table->max_entry - 1; i >= 0; i--) {
			if ((table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, &data[0], table->entry_size))) {
				*rslt_idx = i;
				ret = AAL_E_OK;
				goto _exit_entry_get;
			}
		}
	}

	if (data[0].attr == ATTR_IPV6_LEN_33_64) {
		data[0].profile = data[1].profile = 0; // reset profile which is not set to register
		for (i = table->max_entry - 2; i >= 0; i -= 2) {
			if (   (table->entry[i].data != NULL) && !memcmp(table->entry[i].data, &data[0], table->entry_size)
			    && (table->entry[i + 1].data != NULL) && !memcmp(table->entry[i + 1].data, &data[1], table->entry_size) ) {
				*rslt_idx = i;
				ret = AAL_E_OK;
				goto _exit_entry_get;
			}
		}
	}

	if (data[0].attr == ATTR_IPV6_LEN_65_128) {
		data[0].profile = data[1].profile = data[2].profile = data[3].profile = 0; // reset profile which is not set to register
		for (i = table->max_entry - 4; i >= 0; i -= 4) {
			if (   (table->entry[i].data != NULL) && !memcmp(table->entry[i].data, &data[0], table->entry_size)
			    && (table->entry[i + 1].data != NULL) && !memcmp(table->entry[i + 1].data, &data[1], table->entry_size)
			    && (table->entry[i + 2].data != NULL) && !memcmp(table->entry[i + 2].data, &data[2], table->entry_size)
			    && (table->entry[i + 3].data != NULL) && !memcmp(table->entry[i + 3].data, &data[3], table->entry_size) ) {
				*rslt_idx = i;
				ret = AAL_E_OK;
				goto _exit_entry_get;
			}
		}
	}

_exit_entry_get:
	spin_unlock_irqrestore(&table->lock, flags);
	return ret;
}

static int __aal_lpm_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t *data = entry;
	lpm_tbl_entry_t *orig_data = table->entry[idx].data;
	int i, round = 1;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (orig_data == NULL) return AAL_E_ENTRYNOTFOUND;
	if ((orig_data->attr == ATTR_IPV6_LEN_33_64) && (idx % 2 != 0)) return AAL_E_ENTRYNOTFOUND;
	if ((orig_data->attr == ATTR_IPV6_LEN_65_128) && (idx % 4 != 0)) return AAL_E_ENTRYNOTFOUND;

	if (orig_data->attr == ATTR_IPV6_LEN_33_64) round = 2;
	if (orig_data->attr == ATTR_IPV6_LEN_65_128) round = 4;

	spin_lock_irqsave(&table->lock, flags);

	for (i = 0; i < round; i++) {
		memcpy(&data[i], table->entry[idx + i].data, table->entry_size);
	}

	spin_unlock_irqrestore(&table->lock, flags);

	return AAL_E_OK;
}

static int __aal_lpm_tbl_table_flush(void)
{
	return generic_table_flush(&lpm_tbl_table);
}

static int __aal_lpm_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&lpm_tbl_table);
}

static void __aal_lpm_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &lpm_tbl_table;
	lpm_tbl_entry_t entry;
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

	printk(KERN_INFO "|===== LPM Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = lpm_sw_entry_get_one(i, &entry);
		} else {
			ret = lpm_hw_entry_get(&lpm_tbl_table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %d, data(32) 0x%08x, mask(6) 0x%02x, attr(2) %d\n", entry.valid, entry.data, entry.mask, entry.attr);
			printk(KERN_INFO "rst_idx(12) 0x%03x, rst_ctrl(1) %d\n", entry.rst_idx, entry.rst_ctrl);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static aal_table_t lpm_tbl_table = {
	.table_id = AAL_TABLE_LPM,
	.table_name = "LPM",
	.max_entry = LPM_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(lpm_tbl_entry_t),
	.access_reg = L3FE_FE_L3E_LPM_TBL_ACCESS,
	.data_reg_num = 2,
	.op = {
		.entry_add = __aal_lpm_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = __aal_lpm_tbl_entry_del,
		.entry_set = __aal_lpm_tbl_entry_set,
		.entry_get = __aal_lpm_tbl_entry_get,
		.entry_get_by_idx = __aal_lpm_tbl_entry_get_by_idx,
		.table_flush = __aal_lpm_tbl_table_flush,
		.get_avail_count = __aal_lpm_tbl_get_avail_count,
		.entry_print = __aal_lpm_tbl_entry_print,
	},
};

int __aal_lpm_tbl_init(void)
{
	int ret;

	spin_lock_init(&lpm_tbl_table.lock);

	ret = __aal_table_register(AAL_TABLE_LPM, &lpm_tbl_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void __aal_lpm_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_LPM);
}

/***************************************************************************
 * LPM Initialization
 ***************************************************************************/

int aal_lpm_init(void)
{
	int ret;
	lpm_action_ctrl_t default_route;
	unsigned int first_row, last_row;
	unsigned int t4ctrl_0, t4ctrl_1;

	ret = aal_lpm_mem_init();
	if (ret != AAL_E_OK) return ret;

	/* default route */
	default_route.default_idx = 0xFFF;			// an invalid intf_id to avoid hitting hash entry
	default_route.default_ctrl = 0xF;			// t4_ctrl
	default_route.default_dpid = AAL_LPORT_BLACKHOLE;	// default is to drop packet
	default_route.default_dpid_vld = 1;
	default_route.default_stg = 2;				// set stage3_ctrl to 1; force to ignore T2 result
	default_route.default_stg_ud = 2;			// enable bit 1 of default_stg
	aal_lpm_default_action_set(CA_IPV4, &default_route);
	aal_lpm_default_action_set(CA_IPV6, &default_route);

	/* IPv4 profile#0 */
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_0_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_0_START. (ret=%d)\n", __func__, ret);
		first_row = 0;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_0_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_0_END. (ret=%d)\n", __func__, ret);
		last_row = 3;
	}
	aal_lpm_first_row_set(CA_IPV4, LPM_PROFILE_0, first_row);
	aal_lpm_last_row_set(CA_IPV4, LPM_PROFILE_0, last_row);

	/* IPv4 profile#1 */
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_1_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_1_START. (ret=%d)\n", __func__, ret);
		first_row = 4;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_1_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_1_END. (ret=%d)\n", __func__, ret);
		last_row = 7;
	}
	aal_lpm_first_row_set(CA_IPV4, LPM_PROFILE_1, first_row);
	aal_lpm_last_row_set(CA_IPV4, LPM_PROFILE_1, last_row);

	/* IPv4 profile#2 */
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_2_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_2_START. (ret=%d)\n", __func__, ret);
		first_row = 8;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_2_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_2_END. (ret=%d)\n", __func__, ret);
		last_row = 11;
	}
	aal_lpm_first_row_set(CA_IPV4, LPM_PROFILE_2, first_row);
	aal_lpm_last_row_set(CA_IPV4, LPM_PROFILE_2, last_row);

	/* IPv4 profile#3 */
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_3_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_3_START. (ret=%d)\n", __func__, ret);
		first_row = 12;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV4_PROFILE_3_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_PROFILE_3_END. (ret=%d)\n", __func__, ret);
		last_row = 15;
	}
	aal_lpm_first_row_set(CA_IPV4, LPM_PROFILE_3, first_row);
	aal_lpm_last_row_set(CA_IPV4, LPM_PROFILE_3, last_row);

	/* IPv6 profile#0 */
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_0_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_0_START. (ret=%d)\n", __func__, ret);
		first_row = 31;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_0_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_0_END. (ret=%d)\n", __func__, ret);
		last_row = 28;
	}
	aal_lpm_first_row_set(CA_IPV6, LPM_PROFILE_0, first_row);
	aal_lpm_last_row_set(CA_IPV6, LPM_PROFILE_0, last_row);

	/* IPv6 profile#1 */
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_1_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_1_START. (ret=%d)\n", __func__, ret);
		first_row = 27;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_1_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_1_END. (ret=%d)\n", __func__, ret);
		last_row = 24;
	}
	aal_lpm_first_row_set(CA_IPV6, LPM_PROFILE_1, first_row);
	aal_lpm_last_row_set(CA_IPV6, LPM_PROFILE_1, last_row);

	/* IPv6 profile#2 */
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_2_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_2_START. (ret=%d)\n", __func__, ret);
		first_row = 23;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_2_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_2_END. (ret=%d)\n", __func__, ret);
		last_row = 20;
	}
	aal_lpm_first_row_set(CA_IPV6, LPM_PROFILE_2, first_row);
	aal_lpm_last_row_set(CA_IPV6, LPM_PROFILE_2, last_row);

	/* IPv6 profile#3 */
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_3_START, sizeof(first_row), (ca_uint8_t *) &first_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_3_START. (ret=%d)\n", __func__, ret);
		first_row = 19;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV6_PROFILE_3_END, sizeof(last_row), (ca_uint8_t *) &last_row);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_PROFILE_3_END. (ret=%d)\n", __func__, ret);
		last_row = 16;
	}
	aal_lpm_first_row_set(CA_IPV6, LPM_PROFILE_3, first_row);
	aal_lpm_last_row_set(CA_IPV6, LPM_PROFILE_3, last_row);

	/* IPv4 default t4ctrl */
	ret = scfg_read(0, CFG_ID_LPM_IPV4_DEFAULT_T4CTRL_0, sizeof(t4ctrl_0), (ca_uint8_t *) &t4ctrl_0);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_DEFAULT_T4CTRL_0. (ret=%d)\n", __func__, ret);
		t4ctrl_0 = 0;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV4_DEFAULT_T4CTRL_1, sizeof(t4ctrl_1), (ca_uint8_t *) &t4ctrl_1);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV4_DEFAULT_T4CTRL_1. (ret=%d)\n", __func__, ret);
		t4ctrl_1 = 14;
	}
	aal_lpm_default_t4ctrl_set(CA_IPV4, t4ctrl_0, t4ctrl_1);

	/* IPv6 default t4ctrl */
	ret = scfg_read(0, CFG_ID_LPM_IPV6_DEFAULT_T4CTRL_0, sizeof(t4ctrl_0), (ca_uint8_t *) &t4ctrl_0);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_DEFAULT_T4CTRL_0. (ret=%d)\n", __func__, ret);
		t4ctrl_0 = 0;
	}
	ret = scfg_read(0, CFG_ID_LPM_IPV6_DEFAULT_T4CTRL_1, sizeof(t4ctrl_1), (ca_uint8_t *) &t4ctrl_1);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_LPM_IPV6_DEFAULT_T4CTRL_1. (ret=%d)\n", __func__, ret);
		t4ctrl_1 = 14;
	}
	aal_lpm_default_t4ctrl_set(CA_IPV6, t4ctrl_0, t4ctrl_1);

	return AAL_E_OK;
}

