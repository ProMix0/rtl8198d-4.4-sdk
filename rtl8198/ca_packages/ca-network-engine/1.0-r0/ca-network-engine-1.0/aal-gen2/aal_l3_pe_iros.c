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
 * aal_l3_pe_iros.c: L3FE Packet Editor AAL API exported for iROS
 *
 */

#include <generated/ca_ne_autoconf.h>
#include "aal_l3_pe.h"

ca_status_t ca_aal_l3pe_mtu_size_set (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_uint32_t			idx,
	CA_IN		ca_uint32_t			mtu )
{
	int ret;

	ret = aal_l3pe_mtu_size_set(idx, mtu);
	if (ret != AAL_E_OK) return CA_E_PARAM;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_l3pe_mtu_size_set);

ca_status_t ca_aal_l3pe_mtu_size_get (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_uint32_t			idx,
	CA_OUT		ca_uint32_t			*mtu )
{
	int ret;

	ret = aal_l3pe_mtu_size_get(idx, mtu);
	if (ret != AAL_E_OK) return CA_E_PARAM;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_l3pe_mtu_size_get);

ca_status_t ca_aal_sixrd_add (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		l3pe_sixrd_tbl_entry_t		*entry,
	CA_OUT		ca_uint32_t			*rslt_idx )
{
	int ret;

	ret = aal_entry_add(AAL_TABLE_L3PE_SIXRD_TBL, entry, rslt_idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_sixrd_add);

ca_status_t ca_aal_sixrd_add_by_idx (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		l3pe_sixrd_tbl_entry_t		*entry,
	CA_IN		ca_uint32_t			idx )
{
	int ret;

	ret = aal_entry_add_by_idx(AAL_TABLE_L3PE_SIXRD_TBL, entry, idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_sixrd_add_by_idx);

ca_status_t ca_aal_sixrd_delete (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_uint32_t			idx )
{
	int ret;

	ret = aal_entry_del(AAL_TABLE_L3PE_SIXRD_TBL, idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_sixrd_delete);

ca_status_t ca_aal_dsl_add (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		l3pe_dsl_tbl_entry_t		*entry,
	CA_OUT		ca_uint32_t			*rslt_idx )
{
	int ret;

	ret = aal_entry_add(AAL_TABLE_L3PE_DSL_TBL, entry, rslt_idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_dsl_add);

ca_status_t ca_aal_dsl_add_by_idx (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		l3pe_dsl_tbl_entry_t		*entry,
	CA_IN		ca_uint32_t			idx)
{
	int ret;

	ret = aal_entry_add_by_idx(AAL_TABLE_L3PE_DSL_TBL, entry, idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_dsl_add_by_idx);

ca_status_t ca_aal_dsl_delete (
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_uint32_t			idx )
{
	int ret;

	ret = aal_entry_del(AAL_TABLE_L3PE_DSL_TBL, idx);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_dsl_delete);



