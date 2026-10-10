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
 * aal_lpm_iros.c: LPM AAL API exported for iROS
 *
 */

#include "aal_lpm.h"

/***************************************************************************
 * internal functions
 ***************************************************************************/



/***************************************************************************
 * API
 ***************************************************************************/

/* Add LPM route; for prefix len > 0 */
ca_status_t ca_aal_lpm_add (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	lpm_tbl_entry_t		*entry, /*[4:4]*/
	CA_OUT	ca_uint32_t		*rslt_idx )
{
	int ret;

	ret = aal_entry_add(AAL_TABLE_LPM, entry, rslt_idx);
	if (ret != AAL_E_OK) return CA_E_ROUTE;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_add);

/* Delete LPM route; for prefix len > 0 */
ca_status_t ca_aal_lpm_delete (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		idx )
{
	int ret;

	ret = aal_entry_del(AAL_TABLE_LPM, idx);
	if (ret != AAL_E_OK) return CA_E_ROUTE;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_delete);

/* Set/Update LPM route; for prefix len > 0 */
ca_status_t ca_aal_lpm_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		idx,
	CA_IN	lpm_tbl_entry_t		*entry/*[4:4]*/ )
{
	int ret;

	ret = aal_entry_set(AAL_TABLE_LPM, idx, entry);
	if (ret != AAL_E_OK) return CA_E_ROUTE;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_set);

/* Set LPM default route */
ca_status_t ca_aal_lpm_default_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	lpm_action_ctrl_t	*entry )
{
	int ret;

	ret = aal_lpm_default_action_set(afi, entry);
	if (ret != AAL_E_OK) return CA_E_ROUTE;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_default_set);

/* Get LPM default route */
ca_status_t ca_aal_lpm_default_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_OUT	lpm_action_ctrl_t	*entry )
{
	int ret;

	ret = aal_lpm_default_action_get(afi, entry);
	if (ret != AAL_E_OK) return CA_E_ROUTE;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_default_get);

ca_status_t ca_aal_lpm_default_t4ctrl_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	ca_uint32_t		t4ctrl_0,
	CA_IN	ca_uint32_t		t4ctrl_1 )
{
	int ret;

	ret = aal_lpm_default_t4ctrl_set(afi, t4ctrl_0, t4ctrl_1);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_default_t4ctrl_set);

ca_status_t ca_aal_lpm_default_t4ctrl_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_OUT	ca_uint32_t		*t4ctrl_0,
	CA_OUT	ca_uint32_t		*t4ctrl_1 )
{
	int ret;

	ret = aal_lpm_default_t4ctrl_get(afi, t4ctrl_0, t4ctrl_1);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_default_t4ctrl_get);

ca_status_t ca_aal_lpm_first_row_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	ca_uint32_t		profile,
	CA_IN	ca_uint32_t		row )
{
	int ret;

	ret = aal_lpm_first_row_set(afi, profile, row);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_first_row_set);

ca_status_t ca_aal_lpm_first_row_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	ca_uint32_t		profile,
	CA_OUT	ca_uint32_t		*row )
{
	int ret;

	ret = aal_lpm_first_row_get(afi, profile, row);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_first_row_get);

ca_status_t ca_aal_lpm_last_row_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	ca_uint32_t		profile,
	CA_IN	ca_uint32_t		row )
{
	int ret;

	ret = aal_lpm_last_row_set(afi, profile, row);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_last_row_set);

ca_status_t ca_aal_lpm_last_row_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ip_afi_t		afi,
	CA_IN	ca_uint32_t		profile,
	CA_OUT	ca_uint32_t		*row )
{
	int ret;

	ret = aal_lpm_last_row_get(afi, profile, row);
	if (ret != AAL_E_OK) return CA_E_ERROR;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_aal_lpm_last_row_get);

