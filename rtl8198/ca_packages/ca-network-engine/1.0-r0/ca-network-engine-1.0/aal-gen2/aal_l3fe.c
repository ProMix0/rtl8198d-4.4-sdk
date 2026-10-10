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
 * aal_l3fe.c: Hardware Abstraction Layer for Layer-3 Forwarding Engine
 *
 */

#include <generated/ca_ne_autoconf.h>
#include <linux/sched.h>

#include "scfg.h"
#include "ca_aal_proc.h"

#include "aal_table.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_l3fe.h"
#include "aal_l3_stg0.h"
#include "aal_l3_pe.h"
#include "aal_l3_cam.h"
#include "aal_l3_l2lookup.h"

/* the initila value will be got from startup config */
ca_uint8_t l3fe_classifier_profile_wan = 0;
ca_uint8_t l3fe_classifier_profile_lan = 1;
#if defined(CONFIG_LUNA_G3_SERIES)
ca_uint8_t l3fe_main_hash_profile_wan 			= 0;		// profile for UC 5 tuple WAN
ca_uint8_t l3fe_main_hash_profile_lan 				= 1;		// profile for UC 5 tuple LAN
ca_uint8_t l3fe_main_hash_profile_mc 				= 2;		// profile for MC
ca_uint8_t l3fe_main_hash_profile_2tuple_bridge 	= 3;		// profile for UC 2 tuple bridge
#else
ca_uint8_t l3fe_main_hash_profile_wan = 0;
ca_uint8_t l3fe_main_hash_profile_lan = 1;
ca_uint8_t l3fe_main_hash_profile_mc = 2;
#endif
ca_uint8_t l3fe_main_hash_profile_class = 3;
ca_uint8_t l3fe_hash_lite_profile = 0;
ca_uint8_t l3fe_nat_enable = 1;
ca_uint8_t l3fe_t5_ctrl_support = 0;
ca_uint8_t l3fe_wan_port_type_is_l3 = 0;

#if defined(CONFIG_LUNA_G3_SERIES) &&defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH)
EXPORT_SYMBOL(l3fe_main_hash_profile_mc);
#endif

uint32_t ca_bypass_l3fe = 0x0;	/* bypass L3FE, goes to CPU directly */

/***************************************************************************
 * GLB
 ***************************************************************************/

int aal_l3fe_glb_cls_stg_monitor_get(l3fe_glb_monitor_vector_e where, void *data, size_t size)
{
	L3FE_GLB_CLS_STG_MONITOR_CTRL_t val;
	unsigned int *ptr = (unsigned int *) data;
	int i, count;

	if ((where < MONITOR_DUMP_MIN) || (where > MONITOR_DUMP_MAX)) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	/* total read count is ceil(size/sizeof(int)) */
	count = (size + sizeof(*ptr) - 1) / sizeof(*ptr);
	if (count > 32) return AAL_E_INVALID; // L3FE_GLB_CLS_STG_MONITOR_RETURN format is changed?

	for (i = 0; i < count; i++) {
		/* bus_sel[7:5] is monitor vector, bus_sel[4:0] chooses data bit range */
		val.bf.enable= 1;
		val.bf.bus_sel = (where << 5) | i;
		CA_NE_REG_WRITE(val.wrd, L3FE_GLB_CLS_STG_MONITOR_CTRL);
		ptr[i] = CA_NE_REG_READ(L3FE_GLB_CLS_STG_MONITOR_RETURN);
	}

	return AAL_E_OK;
}

int aal_l3fe_glb_dbg_get(l3fe_glb_dbg_vector_e where, void *data, size_t size)
{
	L3FE_GLB_DBG_IDX_t val;
	unsigned int *ptr = (unsigned int *) data;
	int i, count;

	if ((where < DBG_DUMP_MIN) || (where > DBG_DUMP_MAX)) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	/* total read count is ceil(size/sizeof(int)) */
	count = (size + sizeof(*ptr) - 1) / sizeof(*ptr);
	if (count > 32) return AAL_E_INVALID; // L3FE_GLB_DBG_DAT format is changed?

	for (i = 0; i < count; i++) {
		/* idx[9:5] is debug vector, idx[4:0] chooses data bit range */
		val.bf.idx = (where << 5) | i;
		CA_NE_REG_WRITE(val.wrd, L3FE_GLB_DBG_IDX);
		ptr[i] = CA_NE_REG_READ(L3FE_GLB_DBG_DAT);
	}

	return AAL_E_OK;
}

int aal_l3fe_glb_dbg_latch_monitor_get(l3fe_glb_latch_monitor_vector_e where, void *data, size_t size)
{
	L3FE_GLB_L3FE_MONITOR_CTRL_t val;
	unsigned int *ptr = (unsigned int *) data;
	int i, count;

	if ((where < MONITOR_DUMP_MIN) || (where > MONITOR_DUMP_MAX)) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	/* total read count is ceil(size/sizeof(int)) */
	count = (size + sizeof(*ptr) - 1) / sizeof(*ptr);
	if (count > 32) return AAL_E_INVALID;

	for (i = 0; i < count; i++) {
		/* CTRL[7:5] is monitor vector, CTRL[4:0] chooses data bit range */
		val.bf.bus_sel = (where << 5) | i;
		CA_NE_REG_WRITE(val.wrd, L3FE_GLB_L3FE_MONITOR_CTRL);
		ptr[i] = CA_NE_REG_READ(L3FE_GLB_L3FE_MONITOR_RETURN);
	}

	return AAL_E_OK;
}

int aal_l3fe_glb_dbg_latch_trigger_set(ca_boolean_t status)
{
	L3FE_GLB_L3FE_MONITOR_TRIG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_GLB_L3FE_MONITOR_TRIG);

	if(status == CA_TRUE) {
		/*trun on the same packet latch mode*/
		val.bf.mode = 1;
		val.bf.en = !val.bf.en; // invert the value of en to triger next through Packet Parser packet lookup result latched.
	}else {
		val.bf.mode = 0;
		val.bf.en = 0;
	}
	
	CA_NE_REG_WRITE(val.wrd, L3FE_GLB_L3FE_MONITOR_TRIG);

	return AAL_E_OK;
}

aal_direct_table_t l3fe_ilpb_table = {
	.max_entry = L3FE_GLB_ILPB_01_COUNT,
	.first_reg = L3FE_GLB_ILPB_01,
	.reg_num = L3FE_GLB_ILPB_01_STRIDE / sizeof(ca_uint32_t),
	.reg_stride = L3FE_GLB_ILPB_01_STRIDE
};

ca_status_t aal_l3fe_glb_ilpb_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3fe_glb_ilpb_t *entry
)
{
	return aal_direct_table_get(&l3fe_ilpb_table, entry_idx, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_l3fe_glb_ilpb_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3fe_glb_ilpb_t *entry
)
{
	return aal_direct_table_set(&l3fe_ilpb_table, entry_idx, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_l3fe_glb_ilpb_ldpid_get(
	CA_IN ca_device_id_t dev,
	CA_OUT l3fe_glb_ilpb_ldpid_t *entry
)
{
	*(ca_uint32_t *)entry = CA_NE_REG_READ(L3FE_GLB_ILPB_LDPID);
	return AAL_E_OK;
}

ca_status_t aal_l3fe_glb_ilpb_ldpid_set(
	CA_IN ca_device_id_t dev,
	CA_IN l3fe_glb_ilpb_ldpid_t *entry
)
{
	CA_NE_REG_WRITE(*(ca_uint32_t *)entry, L3FE_GLB_ILPB_LDPID);
	return AAL_E_OK;
}

aal_direct_table_t l3fe_elpb_table = {
	.max_entry = 1,
	.first_reg = L3FE_GLB_ELPB1,
	.reg_num = 2,
	.reg_stride = 0
};

ca_status_t aal_l3fe_glb_elpb_get(
	CA_IN ca_device_id_t dev,
	CA_OUT l3fe_glb_elpb_t *entry
)
{
	return aal_direct_table_get(&l3fe_elpb_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_l3fe_glb_elpb_set(
	CA_IN ca_device_id_t dev,
	CA_IN l3fe_glb_elpb_t *entry
)
{
	return aal_direct_table_set(&l3fe_elpb_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

aal_direct_table_t l3fe_deepq_vld_table = {
	.max_entry = 1,
	.first_reg = L3FE_GLB_ELPB_DEEPQ_VLD1,
	.reg_num = 2,
	.reg_stride = 0
};

ca_status_t aal_l3fe_glb_elpb_deepq_vld_get(
	CA_IN ca_device_id_t dev,
	CA_OUT l3fe_glb_deepq_vld_t *entry
)
{
	return aal_direct_table_get(&l3fe_deepq_vld_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_l3fe_glb_elpb_deepq_vld_set(
	CA_IN ca_device_id_t dev,
	CA_IN l3fe_glb_deepq_vld_t *entry
)
{
	return aal_direct_table_set(&l3fe_deepq_vld_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

aal_direct_table_t l3fe_deepq_table = {
	.max_entry = 1,
	.first_reg = L3FE_GLB_ELPB_DEEPQ1,
	.reg_num = 2,
	.reg_stride = 0
};

ca_status_t aal_l3fe_glb_elpb_deepq_get(
	CA_IN ca_device_id_t dev,
	CA_OUT l3fe_glb_deepq_t *entry
)
{
	return aal_direct_table_get(&l3fe_deepq_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_l3fe_glb_elpb_deepq_set(
	CA_IN ca_device_id_t dev,
	CA_IN l3fe_glb_deepq_t *entry
)
{
	return aal_direct_table_set(&l3fe_deepq_table, 0, entry, AAL_TABLE_DIR_HI_TO_LOW);
}

static void aal_l3fe_l2lookup_init(void)
{
	L3FE_GLB_ILPB_LDPID_t ilpb_ldpid;
	L3FE_GLB_VE_t ve;
	l3fe_glb_t5_opt_t t5_opt = {0};
	l3fe_glb_deepq_vld_t deepq_vld = {0};
	l3fe_glb_deepq_t deepq = {0};

	/* The LSPID for L3FE to L2FE are 0x18/0x19 */
	ilpb_ldpid.wrd = 0;
	ilpb_ldpid.bf.valid0 = 1;
	ilpb_ldpid.bf.ldpid0 = AAL_LPORT_L3_LAN;
	ilpb_ldpid.bf.valid1 = 1;
	ilpb_ldpid.bf.ldpid1 = AAL_LPORT_L3_WAN;
	ilpb_ldpid.bf.valid2 = 1;
	ilpb_ldpid.bf.ldpid2 = AAL_LPORT_ETH_WAN;
	ilpb_ldpid.bf.valid3 = 1;

	CA_NE_REG_WRITE(ilpb_ldpid.wrd, L3FE_GLB_ILPB_LDPID);

	/* Set ctag/stag: 1:0x8100, 2:0x88a8, 3:0x9100, 4:0x9200 */
	ve.wrd = 0;
	ve.bf.stag_tpid_enc = 2;
	ve.bf.ctag_tpid_enc = 1;
	CA_NE_REG_WRITE(ve.wrd, L3FE_GLB_VE);

	/* Bypass VE result when vlan cnt == 0 and don't care mcgid[9:6] of L2E result */
	t5_opt.igr_mcgid_hdr = 1;
	t5_opt.igr_ve_reslt = 1;
	CA_NE_REG_WRITE(*(ca_uint32_t *)&t5_opt, L3FE_GLB_T5_OPTION);

	/* Set DeepQ for LDPID
	 * Overwrite DeepQ setting for LDPID 0~7 and 0x18, 0x19.
	 * Turn on DeepQ for LDPID 0~6 and turn off for 7, 0x18 and 0x19.
	 */
	deepq_vld.deepq_ldpid_valid_vec = 0x7F | 1 << AAL_LPORT_ETH_WAN |
		1 << AAL_LPORT_L3_WAN | 1 << AAL_LPORT_L3_LAN;
	deepq.deepq_ldpid_vec = 0x7F | 1 << AAL_LPORT_ETH_WAN;
	aal_l3fe_glb_elpb_deepq_vld_set(0, &deepq_vld);
	aal_l3fe_glb_elpb_deepq_set(0, &deepq);

	return;
}

static void aal_hash_ttl_zero_ctrl(void) 
{
	L3FE_GLB_RES_CTRL_t res_ctrl;
	int ttl_0_behavior = 0; /* The value will get from scfg.txt */

	if(ttl_0_behavior == 1) {
		res_ctrl.bf.ttl_0_cpu_en = 1; /* 0: Drop, 1: Forward to cpu */
		res_ctrl.bf.ttl_0_cpu_ldpid = AAL_LPORT_CPU_0;
		res_ctrl.bf.ttl_0_cpu_deepq = 0;
		res_ctrl.bf.ttl_0_cpu_deepq_vld = 0;
		res_ctrl.bf.ttl_0_cpu_keep_org_pkt = 1;
		res_ctrl.bf.ttl_0_cpu_keep_org_pkt_vld = 1;
		res_ctrl.bf.ttl_0_cpu_ctrl_bypass = 1;
	} else {
		res_ctrl.bf.ttl_0_cpu_en = 0; /* 0: Drop, 1: Forward to cpu */
		res_ctrl.bf.ttl_0_cpu_ldpid = 0;
		res_ctrl.bf.ttl_0_cpu_deepq = 0;
		res_ctrl.bf.ttl_0_cpu_deepq_vld = 0;
		res_ctrl.bf.ttl_0_cpu_keep_org_pkt = 0;
		res_ctrl.bf.ttl_0_cpu_keep_org_pkt_vld = 0;
		res_ctrl.bf.ttl_0_cpu_ctrl_bypass = 0;
	}

	CA_NE_REG_WRITE(*(ca_uint32_t *)&res_ctrl, L3FE_GLB_RES_CTRL);

	return;
}

static void aal_l3fe_axi_reo_init(ca_device_id_t device_id)
{
	AXI_REO_RD_ORIG_ID_t orig_id;
	AXI_REO_RD_NEW_ID_t new_id;
	AXI_REO_RD_TOP_ADDR0_t top_addr;
	AXI_REO_RD_TOP_ADDR_MASK0_t top_addr_mask;
	AXI_REO_RD_NEW_ID0_t new_id_0;

	orig_id.wrd = 0;
	orig_id.bf.axi_id = 2;

	new_id.wrd = 0;
	new_id.bf.axi_id = 8;
	new_id.bf.axi_id_valid = 1;

	top_addr.bf.id_addr = 0x10000000;
	top_addr_mask.bf.id_mask = 0x10000000;

	new_id_0.wrd = 0;
	new_id_0.bf.axi_id = 9;
	new_id_0.bf.axi_id_valid = 1;

	CA_NE_AXI_REO_REG_WRITE(orig_id.wrd, AXI_REO_RD_ORIG_ID + APB2_L3FE_AXI_REO_STRIDE);
	CA_NE_AXI_REO_REG_WRITE(new_id.wrd,AXI_REO_RD_NEW_ID + APB2_L3FE_AXI_REO_STRIDE);
	CA_NE_AXI_REO_REG_WRITE(top_addr.wrd, AXI_REO_RD_TOP_ADDR0 + APB2_L3FE_AXI_REO_STRIDE);
	CA_NE_AXI_REO_REG_WRITE(top_addr_mask.wrd, AXI_REO_RD_TOP_ADDR_MASK0 + APB2_L3FE_AXI_REO_STRIDE);
	CA_NE_AXI_REO_REG_WRITE(new_id_0.wrd, AXI_REO_RD_NEW_ID0 + APB2_L3FE_AXI_REO_STRIDE);
}

EXPORT_SYMBOL(aal_l3fe_glb_ilpb_get);
EXPORT_SYMBOL(aal_l3fe_glb_ilpb_set);
EXPORT_SYMBOL(aal_l3fe_glb_ilpb_ldpid_get);
EXPORT_SYMBOL(aal_l3fe_glb_ilpb_ldpid_set);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_get);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_set);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_deepq_vld_get);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_deepq_vld_set);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_deepq_get);
EXPORT_SYMBOL(aal_l3fe_glb_elpb_deepq_set);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_l3fe_glb_dbg_latch_trigger_set);
#endif



/***************************************************************************
 * STG0
 ***************************************************************************/

int aal_l3fe_stg0_ctrl_set(ca_device_id_t device_id, L3FE_STG0_CTRL_t *ctrl)
{
	uint32_t *ptr = (uint32_t *) ctrl;

	if (ctrl == NULL) return AAL_E_NULLPTR;

	CA_NE_REG_WRITE(*ptr, L3FE_STG0_CTRL);

	return AAL_E_OK;
}

int aal_l3fe_stg0_ctrl_get(ca_device_id_t device_id, L3FE_STG0_CTRL_t *ctrl)
{
	uint32_t *ptr = (uint32_t *) ctrl;

	if (ctrl == NULL) return AAL_E_NULLPTR;

	*ptr = CA_NE_REG_READ(L3FE_STG0_CTRL);

	return AAL_E_OK;
}

#define CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT		300
#define __DATA_MASK_SET(data, mask, offset, value)	(data) = ((data) & ~((mask)<<(offset))) | (((value)&(mask))<<(offset))

/* table index 0 - WAN port, 1 - L3_WAN port, 2 - L3 LAN port */
int aal_l3fe_stg0_get_lpb_table_index(ca_uint8_t lspid)
{
	int i;
	L3FE_STG0_LPB_IDX_TBL_ACCESS_t	lpb_idx_tbl_access;
	L3FE_STG0_LPB_IDX_TBL_DATA_t	lpb_idx_tbl_data;

	lpb_idx_tbl_access.wrd = 0;
	lpb_idx_tbl_access.bf.access = 1;
	lpb_idx_tbl_access.bf.rbw = 0;
	lpb_idx_tbl_access.bf.addr = lspid;
	CA_NE_REG_WRITE(lpb_idx_tbl_access.wrd, L3FE_STG0_LPB_IDX_TBL_ACCESS);

	for (i = 0; i < CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT; i++) {
		lpb_idx_tbl_access.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_IDX_TBL_ACCESS);
		if (!lpb_idx_tbl_access.bf.access) {
			break;
		}
	}
	if (i == CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT) {
		printk("%s: CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT!!!\n", __func__);
		return -1;
	}
	lpb_idx_tbl_data.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_IDX_TBL_DATA);
	return lpb_idx_tbl_data.bf.idx;
}

ca_status_t aal_l3fe_stg0_set_lpb_table_index(ca_uint8_t lspid, int index)
{
	int i;
	L3FE_STG0_LPB_IDX_TBL_ACCESS_t  lpb_idx_tbl_access;
	L3FE_STG0_LPB_IDX_TBL_DATA_t    lpb_idx_tbl_data;

	lpb_idx_tbl_access.wrd = 0;
	lpb_idx_tbl_access.bf.access = 1;
	lpb_idx_tbl_access.bf.rbw = 0;
	lpb_idx_tbl_access.bf.addr = lspid;
	CA_NE_REG_WRITE(lpb_idx_tbl_access.wrd, L3FE_STG0_LPB_IDX_TBL_ACCESS);

	for (i = 0; i < CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT; i++) {
		lpb_idx_tbl_access.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_IDX_TBL_ACCESS);
		if (!lpb_idx_tbl_access.bf.access) {
			break;
		}
	}
	if (i == CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT) {
		printk("%s: CA_L3FE_STG0_LPB_IDX_TBL_TIMEOUT!!!\n", __func__);
		return CA_E_ERROR;
	}

	lpb_idx_tbl_data.wrd = 0;
	lpb_idx_tbl_data.bf.idx = index;
	CA_NE_REG_WRITE(lpb_idx_tbl_data.wrd, L3FE_STG0_LPB_IDX_TBL_DATA);

	return CA_E_OK;
}

ca_status_t aal_l3_stg0_ldpid_map_get (
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	l3_stg0_ldpid_map_t	*config )
{
	*(ca_uint32_t*)config = CA_NE_REG_READ(L3FE_STG0_LDPID_MAP);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3_stg0_ldpid_map_get);

ca_status_t aal_l3_stg0_ldpid_map_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	l3_stg0_ldpid_map_t	*config)
{
	CA_NE_REG_WRITE(*(ca_uint32_t*)config, L3FE_STG0_LDPID_MAP);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3_stg0_ldpid_map_set);

ca_status_t aal_l3_stg0_lpb_tbl_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		tbl_idx,
	CA_OUT	aal_l3fe_lpb_tbl_cfg_t	*config )
{
	uint32_t *ptr = (uint32_t *) config;
	int i;

	AAL_ASSERT_RET(tbl_idx < AAL_L3FE_LPB_TBL_NUM, CA_E_PARAM);
	AAL_ASSERT_RET(config, CA_E_PARAM);

	for (i = 0; i < 3; i++) {
		*(ptr + i) = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_LOW_0 + (i * 4) + (tbl_idx * 12));
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3_stg0_lpb_tbl_get);

ca_status_t aal_l3_stg0_lpb_tbl_set (
	CA_IN	ca_device_id_t			device_id,
	CA_IN	ca_uint32_t			tbl_idx,
	CA_IN	aal_l3fe_lpb_tbl_cfg_mask_t	mask,
	CA_OUT	aal_l3fe_lpb_tbl_cfg_t		*config )
{
	aal_l3fe_lpb_tbl_cfg_t orig_config;
	uint32_t *ptr = (uint32_t *) &orig_config;
	int i;

	AAL_ASSERT_RET(tbl_idx < AAL_L3FE_LPB_TBL_NUM, CA_E_PARAM);
	AAL_ASSERT_RET(config, CA_E_PARAM);

	for (i = 0; i < 3; i++) {
		*(ptr + i) = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_LOW_0 + (i * 4) + (tbl_idx * 12));
	}

	if (mask.s.lspid)
		orig_config.lspid = config->lspid;

	if (mask.s.default_802_1p)
		orig_config.default_802_1p = config->default_802_1p;

	if (mask.s.spcl_pkt_en)
		orig_config.spcl_pkt_en = config->spcl_pkt_en;

	if (mask.s.spcl_pkt_sel)
		orig_config.spcl_pkt_sel = config->spcl_pkt_sel;

	if (mask.s.mac_da_match_en)
		orig_config.mac_da_match_en = config->mac_da_match_en;

	if (mask.s.mac_da_an_mask)
		orig_config.mac_da_an_mask = config->mac_da_an_mask;

	if (mask.s.default_ldpid)
		orig_config.default_ldpid = config->default_ldpid;

	if (mask.s.default_cos_tbl_sel)
		orig_config.default_cos_tbl_sel = config->default_cos_tbl_sel;

	if (mask.s.default_cos_sel)
		orig_config.default_cos_sel = config->default_cos_sel;

	if (mask.s.default_cos)
		orig_config.default_cos = config->default_cos;

	if (mask.s.mrr_en)
		orig_config.mrr_en = config->mrr_en;

	if (mask.s.mrr_vld)
		orig_config.mrr_vld = config->mrr_vld;

	if (mask.s.pol_en)
		orig_config.pol_en = config->pol_en;

	if (mask.s.pol_id)
		orig_config.pol_id = config->pol_id;

	if (mask.s.pol_grp_id)
		orig_config.pol_grp_id = config->pol_grp_id;

	if (mask.s.t1_ctrl)
		orig_config.t1_ctrl = config->t1_ctrl;

	if (mask.s.t5_ctrl)
		orig_config.t5_ctrl = config->t5_ctrl;

	if (mask.s.mru_len)
		orig_config.mru_len = config->mru_len;

	if (mask.s.mru_redir_en)
		orig_config.mru_redir_en = config->mru_redir_en;

	if (mask.s.mru_redir_ldpid)
		orig_config.mru_redir_ldpid = config->mru_redir_ldpid;

	if (mask.s.mru_redir_pol_cos)
		orig_config.mru_redir_pol_cos = config->mru_redir_pol_cos;

	if (mask.s.mru_redir_pol_en)
		orig_config.mru_redir_pol_en = config->mru_redir_pol_en;

	if (mask.s.mru_redir_pol_id)
		orig_config.mru_redir_pol_id = config->mru_redir_pol_id;

	if (mask.s.mru_redir_pol_grp_id)
		orig_config.mru_redir_pol_grp_id = config->mru_redir_pol_grp_id;

	for (i = 0; i < 3; i++) {
		CA_NE_REG_WRITE(*(ptr + i), L3FE_STG0_LPB_TBL_LOW_0 + (i * 4) + (tbl_idx * 12));
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3_stg0_lpb_tbl_set);

ca_status_t aal_l3fe_lpb_idx_tbl_get(
		CA_IN ca_device_id_t device_id,
		CA_IN ca_uint32_t  lspid,
		CA_OUT ca_uint8_t *lpb_profile)
{
	L3FE_STG0_LPB_IDX_TBL_DATA_t  data;

	AAL_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);
	AAL_ASSERT_RET(lpb_profile, CA_E_PARAM);

	READ_INDIRCT_REG(device_id, lspid, L3FE_STG0_LPB_IDX_TBL_ACCESS);
	data.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_IDX_TBL_DATA);

	*lpb_profile = data.bf.idx;

	return CA_E_OK;
}

ca_status_t aal_l3fe_lpb_idx_tbl_set(
		CA_IN ca_device_id_t device_id,
		CA_IN ca_uint32_t lspid,
		CA_IN ca_uint8_t  lpb_profile)
{
	L3FE_STG0_LPB_IDX_TBL_DATA_t  data;

	AAL_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);
	AAL_ASSERT_RET(lpb_profile < 4, CA_E_PARAM);

	READ_INDIRCT_REG(device_id, lspid, L3FE_STG0_LPB_IDX_TBL_ACCESS);
	data.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_IDX_TBL_DATA);
	data.bf.idx = lpb_profile;
	WRITE_INDIRCT_REG(device_id, lspid, L3FE_STG0_LPB_IDX_TBL_ACCESS);

	return CA_E_OK;
}

ca_status_t aal_l3fe_dump_lpb(ca_device_id_t device_id)
{
	int ret;
	L3FE_STG0_CTRL_t stg0_ctrl;
	L3FE_STG0_LPB_TBL_HI_t  stg0_lpb_tbl_hi;
	L3FE_STG0_LPB_TBL_MID_t stg0_lpb_tbl_mid;
	L3FE_STG0_LDPID_MAP_t   ldpid_map;

	if ((ret = aal_l3fe_stg0_ctrl_get(device_id, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}
	printk("=== device_id %u ===\n", device_id);
	printk("=== L3FE_STG0_CTRL : 0x%08x ===\n", stg0_ctrl.wrd);
	printk("lpb_idx_mode=%u\n", stg0_ctrl.bf.lpb_idx_mode);
	printk("mac_fltr_fail_pkt_mru_chk_en=%u\n", stg0_ctrl.bf.mac_fltr_fail_pkt_mru_chk_en);
	printk("mru_chk_fail_pkt_l2_lkup_dis=%u\n", stg0_ctrl.bf.mru_chk_fail_pkt_l2_lkup_dis);
	printk("other_mac_pkt_keep_lspid_en=%u\n", stg0_ctrl.bf.other_mac_pkt_keep_lspid_en);
	printk("wan_lspid=0x%02x\n", stg0_ctrl.bf.wan_lspid);
	printk("use_lspid_ovwr_o_lspid=%u\n", stg0_ctrl.bf.use_lspid_ovwr_o_lspid);
	printk("spcl_pkt_keep_old=%u\n", stg0_ctrl.bf.spcl_pkt_keep_old);
	printk("wan_lspid_1=0x%02x\n", stg0_ctrl.bf.wan_lspid_1);
	printk("msk_my_mac_match_en_for_lspid_upd=%u\n", stg0_ctrl.bf.msk_my_mac_match_en_for_lspid_upd);
	printk("keep_lspid_unchange=%u\n", stg0_ctrl.bf.keep_lspid_unchange);
	printk("my_mac_fail_pkt_bypass_cls=%u\n", stg0_ctrl.bf.my_mac_fail_pkt_bypass_cls);
	printk("chk_mymac_for_lan=%u\n", stg0_ctrl.bf.chk_mymac_for_lan);

	ldpid_map.wrd = CA_NE_REG_READ(L3FE_STG0_LDPID_MAP);
	printk("=== L3FE_STG0_LDPID_MAP : 0x%08x ===\n", ldpid_map.wrd);
	printk("rule_0=0x%02x, lpb_idx_0=%u\n", ldpid_map.bf.rule_0, ldpid_map.bf.lpb_idx_0);
	printk("rule_1=0x%02x, lpb_idx_1=%u\n", ldpid_map.bf.rule_1, ldpid_map.bf.lpb_idx_1);
	printk("rule_2=0x%02x, lpb_idx_2=%u\n", ldpid_map.bf.rule_2, ldpid_map.bf.lpb_idx_2);
	printk("default, lpb_idx_3=%u\n", ldpid_map.bf.lpb_idx_3);

	stg0_lpb_tbl_hi.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_HIGH_0);
	stg0_lpb_tbl_mid.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_MID_0);
	printk("=== L3FE_STG0_LPB_TBL_0 : 0x%08x ===\n", stg0_lpb_tbl_hi.wrd);
	printk("lspid=0x%02x, default_ldpid=0x%02x\n", stg0_lpb_tbl_hi.bf.lspid, stg0_lpb_tbl_hi.bf.default_ldpid);
	printk("special_en=%u, special_sel=%u\n", stg0_lpb_tbl_hi.bf.special_en, stg0_lpb_tbl_hi.bf.special_sel);
	printk("mac_da_match_en=%u, mac_da_match_mask=%u, t1_ctrl=%u, t5_ctrl=%u\n",
			stg0_lpb_tbl_hi.bf.mac_da_match_en, stg0_lpb_tbl_hi.bf.mac_da_an_mask, stg0_lpb_tbl_mid.bf.t1_ctrl, stg0_lpb_tbl_mid.bf.t5_ctrl);

	stg0_lpb_tbl_hi.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_HIGH_1);
	stg0_lpb_tbl_mid.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_MID_1);
	printk("=== L3FE_STG0_LPB_TBL_1 : 0x%08x ===\n", stg0_lpb_tbl_hi.wrd);
	printk("lspid=0x%02x, default_ldpid=0x%02x\n", stg0_lpb_tbl_hi.bf.lspid, stg0_lpb_tbl_hi.bf.default_ldpid);
	printk("special_en=%u, special_sel=%u\n", stg0_lpb_tbl_hi.bf.special_en, stg0_lpb_tbl_hi.bf.special_sel);
	printk("mac_da_match_en=%u, mac_da_match_mask=%u, t1_ctrl=%u, t5_ctrl=%u\n",
			stg0_lpb_tbl_hi.bf.mac_da_match_en, stg0_lpb_tbl_hi.bf.mac_da_an_mask, stg0_lpb_tbl_mid.bf.t1_ctrl, stg0_lpb_tbl_mid.bf.t5_ctrl);

	stg0_lpb_tbl_hi.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_HIGH_2);
	stg0_lpb_tbl_mid.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_MID_2);
	printk("=== L3FE_STG0_LPB_TBL_2 : 0x%08x ===\n", stg0_lpb_tbl_hi.wrd);
	printk("lspid=0x%02x, default_ldpid=0x%02x\n", stg0_lpb_tbl_hi.bf.lspid, stg0_lpb_tbl_hi.bf.default_ldpid);
	printk("special_en=%u, special_sel=%u\n", stg0_lpb_tbl_hi.bf.special_en, stg0_lpb_tbl_hi.bf.special_sel);
	printk("mac_da_match_en=%u, mac_da_match_mask=%u, t1_ctrl=%u, t5_ctrl=%u\n",
			stg0_lpb_tbl_hi.bf.mac_da_match_en, stg0_lpb_tbl_hi.bf.mac_da_an_mask, stg0_lpb_tbl_mid.bf.t1_ctrl, stg0_lpb_tbl_mid.bf.t5_ctrl);

	stg0_lpb_tbl_hi.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_HIGH_3);
	stg0_lpb_tbl_mid.wrd = CA_NE_REG_READ(L3FE_STG0_LPB_TBL_MID_3);
	printk("=== L3FE_STG0_LPB_TBL_3 : 0x%08x ===\n", stg0_lpb_tbl_hi.wrd);
	printk("stg0_lpb_tbl_hi.wrd=0x%08x\n", stg0_lpb_tbl_hi.wrd);
	printk("lspid=0x%02x, default_ldpid=0x%02x\n", stg0_lpb_tbl_hi.bf.lspid, stg0_lpb_tbl_hi.bf.default_ldpid);
	printk("special_en=%u, special_sel=%u\n", stg0_lpb_tbl_hi.bf.special_en, stg0_lpb_tbl_hi.bf.special_sel);
	printk("mac_da_match_en=%u, mac_da_match_mask=%u, t1_ctrl=%u, t5_ctrl=%u\n",
			stg0_lpb_tbl_hi.bf.mac_da_match_en, stg0_lpb_tbl_hi.bf.mac_da_an_mask, stg0_lpb_tbl_mid.bf.t1_ctrl, stg0_lpb_tbl_mid.bf.t5_ctrl);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3fe_dump_lpb);

/***************************************************************************
 * STG1
 ***************************************************************************/

int aal_l3fe_stg1_ipnip_ctrl_set(l3fe_stg1_ipnip_ctrl_reg_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_IPNIP_CTRL_REG);
	return AAL_E_OK;
}

int aal_l3fe_stg1_ipnip_ctrl_get(l3fe_stg1_ipnip_ctrl_reg_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_IPNIP_CTRL_REG);
	return AAL_E_OK;
}

int aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(l3fe_stg1_ecn_fail_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_ECN_FAIL_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(l3fe_stg1_ecn_fail_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_ECN_FAIL_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(l3fe_stg1_ipnip_fail_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_IPNIP_FAIL_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(l3fe_stg1_ipnip_fail_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_IPNIP_FAIL_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_set(l3fe_stg1_sixrd_ipmc_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_SIXRD_IPMC_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get(l3fe_stg1_sixrd_ipmc_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_SIXRD_IPMC_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_set(l3fe_stg1_dsl_ipmc_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_DSL_IPMC_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get(l3fe_stg1_dsl_ipmc_fwd_ctrl_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_DSL_IPMC_FWD_CTRL);
	return AAL_E_OK;
}

int aal_l3fe_stg1_sixrd_ipmc_new_mac_da_set(l3fe_stg1_sixrd_ipmc_new_mac_da_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_SIXRD_IPMC_NEW_MAC_DA);
	return AAL_E_OK;
}

int aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get(l3fe_stg1_sixrd_ipmc_new_mac_da_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_SIXRD_IPMC_NEW_MAC_DA);
	return AAL_E_OK;
}

int aal_l3fe_stg1_dsl_ipmc_new_mac_da_set(l3fe_stg1_dsl_ipmc_new_mac_da_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	CA_NE_REG_WRITE(data->wrd, L3FE_STG1_DSL_IPMC_NEW_MAC_DA);
	return AAL_E_OK;
}

int aal_l3fe_stg1_dsl_ipmc_new_mac_da_get(l3fe_stg1_dsl_ipmc_new_mac_da_t *data)
{
	if (data == NULL) return AAL_E_NULLPTR;
	data->wrd = CA_NE_REG_READ(L3FE_STG1_DSL_IPMC_NEW_MAC_DA);
	return AAL_E_OK;
}

/***************************************************************************
 * PP (Packet Parser)
 ***************************************************************************/

void aal_l3fe_spcl_pkt_detection_cfg(void)
{
	L3FE_PP_SPCL_PKT_DETECTION_CFG_t	spcl_pkt_det;

	spcl_pkt_det.bf.check_arp_target_ip 	= 1;
	spcl_pkt_det.bf.mpcp_opcode_en 		= 0;
	spcl_pkt_det.bf.my_mac_enable 		= 1;
	spcl_pkt_det.bf.type_0_en		= 0;
	spcl_pkt_det.bf.type_1_en		= 0;
	spcl_pkt_det.bf.udp_lite_enable		= 1;
	spcl_pkt_det.bf.user_def_spcl_0_en	= 0;
	spcl_pkt_det.bf.user_def_spcl_1_en	= 0;
	spcl_pkt_det.bf.user_def_spcl_2_en	= 0;
	spcl_pkt_det.bf.user_def_spcl_3_en	= 0;
	spcl_pkt_det.bf.l2_spcl_hdr_en		= 1;
	spcl_pkt_det.bf.l3_spcl_hdr_en		= 1;
	spcl_pkt_det.bf.l4_spcl_hdr_en		= 1;
	spcl_pkt_det.bf.user_define_hi_pri	= 0;
	spcl_pkt_det.bf.rdp_v1_detection_en	= 0;
	spcl_pkt_det.bf.ipv4_chksum_chk_enable	= 1;
	spcl_pkt_det.bf.l4_chksum_chk_enable	= 1;
	spcl_pkt_det.bf.l4_chksum_rh_chk_enable	= 0;
	spcl_pkt_det.bf.chksum_err_drop		= 1;
	spcl_pkt_det.bf.bpdu_mac_08_en		= 1;
	spcl_pkt_det.bf.subtype_en		= 0;

	CA_NE_REG_WRITE(spcl_pkt_det.wrd, L3FE_PP_SPCL_PKT_DETECTION_CFG);
}

void aal_l3fe_l4_chksum(ca_uint8_t enable)
{
	L3FE_PP_SPCL_PKT_DETECTION_CFG_t	spcl_pkt_det;

	spcl_pkt_det.wrd = CA_NE_REG_READ(L3FE_PP_SPCL_PKT_DETECTION_CFG);
	spcl_pkt_det.bf.l4_chksum_chk_enable = enable;
	CA_NE_REG_WRITE(spcl_pkt_det.wrd, L3FE_PP_SPCL_PKT_DETECTION_CFG);
}

int aal_l3fe_pp_header_a_get(L3FE_HDR_A_t *hdr_a)
{
	uint32_t *val = NULL;

	if (hdr_a == NULL)
		return AAL_E_NULLPTR;

	val = (uint32_t *)hdr_a;
	*val = CA_NE_REG_READ(L3FE_PP_HEADER_A_LOW);

	val++;
	*val = CA_NE_REG_READ(L3FE_PP_HEADER_A_HI);

	return AAL_E_OK;
}

/***************************************************************************
 * Initialization
 ***************************************************************************/

int aal_l3fe_stg0_set_normal(void)
{
	L3FE_STG0_CTRL_t stg0_ctrl;
	l3_stg0_ldpid_map_t stg0_ldpid_map;
	aal_l3fe_lpb_tbl_cfg_mask_t lpb_tbl_cfg_mask;
	aal_l3fe_lpb_tbl_cfg_t stg0_lpb_cfg;
	int ret;

	/*** Global LPB Configurations ***/

	if ((ret = aal_l3fe_stg0_ctrl_get(0, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}
	stg0_ctrl.bf.use_lspid_ovwr_o_lspid = 0;
	stg0_ctrl.bf.wan_lspid = 7;			/* WAN port ldpid */
	stg0_ctrl.bf.other_mac_pkt_keep_lspid_en = 1;
	stg0_ctrl.bf.mru_chk_fail_pkt_l2_lkup_dis = 1;
	stg0_ctrl.bf.mac_fltr_fail_pkt_mru_chk_en = 0;
	/* When lpb_idx_mode is
	 *   0: Use HDR_A.ldpid to index in reg L3FE_STG0_LDPID_MAP. PP copies it to HDR_I.lspid.
	 *   1: Use HDR_A.lspid to index in reg L3FE_STG0_LPB_IDX_TBL. PP copies it to HDR_I.o_lspid.
	 */
	stg0_ctrl.bf.lpb_idx_mode = 0;
	if ((ret = aal_l3fe_stg0_ctrl_set(0, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to set STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/*** LDPID Profile Mapping ***/

	if ((ret = aal_l3_stg0_ldpid_map_get(0, &stg0_ldpid_map)) != CA_E_OK) {
		ca_printf("%s: Fail to get STG0 LDPID map. (ret=%d)\n", __func__, ret);
		return ret;
	}

#ifdef CONFIG_CORTINA_BOARD_FPGA
	stg0_ldpid_map.rule_0 = 0; /* When packet comes from WAN port 0x7, the HDR_A.ldpid is 0 */
#else
	stg0_ldpid_map.rule_0 = AAL_LPORT_ETH_NI7;
#endif
	stg0_ldpid_map.lpb_idx_0 = 0; /* LPB profile# */

	stg0_ldpid_map.rule_1 = AAL_LPORT_L3_LAN;
	stg0_ldpid_map.lpb_idx_1 = 1;

	stg0_ldpid_map.rule_2 = AAL_LPORT_L3_WAN;
	stg0_ldpid_map.lpb_idx_2 = 2;

	stg0_ldpid_map.lpb_idx_3 = 3;

	if ((ret = aal_l3_stg0_ldpid_map_set(0, &stg0_ldpid_map)) != CA_E_OK) {
		ca_printf("%s: Fail to get STG0 LDPID map. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/* LDPID profiles (configure and use 2 profiles only) */
	/* L3FE_STG0_LPB_TBL_LOW/MED/HI_0-1, only use 2 profiles (7, 0x18=>profile 0, 0x19+CPU=>profile 1) */
	/* make sure to configure L3FE_PP_FIELD_CAM_ACCESS/ MAC_DA, 1st entry = MY MAC) */
	/* L3FE_PP_FIELD_CAM_ACCESS.19:16 = MAC_DA_CAM "0100" */
	/* L3FE_PP_FIELD_CAM_ACCESS.7:0 = MAC_DA: 0 ~ 6;  (can setup 6 MY MACs) */
	/* Entry size is 128 bits, 48 bits LSB used for MAC DA */

	lpb_tbl_cfg_mask.u32 = 0xffffffff;

	/*** LPB Profile 0 (for WAN port) ***/

	memset(&stg0_lpb_cfg, 0, sizeof(stg0_lpb_cfg));
	stg0_lpb_cfg.lspid = AAL_LPORT_L3_WAN;	/* STG0 sets this into HDR_I.lspid */
	stg0_lpb_cfg.mac_da_match_en = 0;
	// stg0_lpb_cfg.mac_da_an_mask = 0x2;	/* MAC addres of WAN port locates at CAM table index 1, so mask bit 1 should be turned on */
	stg0_lpb_cfg.default_ldpid = AAL_LPORT_L3_LAN;
	stg0_lpb_cfg.t1_ctrl = l3fe_classifier_profile_wan;
	stg0_lpb_cfg.t5_ctrl = T5_CTRL_BYPASS;
	stg0_lpb_cfg.spcl_pkt_en = 1;
	stg0_lpb_cfg.spcl_pkt_sel = 0;
	if ((ret = aal_l3_stg0_lpb_tbl_set(0, 0, lpb_tbl_cfg_mask, &stg0_lpb_cfg)) != CA_E_OK) {
		ca_printf("%s: Fail to set STG0 LPB table profile#0. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/*** LPB Profile 1 ***/

	memset(&stg0_lpb_cfg, 0, sizeof(stg0_lpb_cfg));
	stg0_lpb_cfg.lspid = AAL_LPORT_L3_LAN;	/* STG0 sets this into HDR_I.lspid */
	stg0_lpb_cfg.mac_da_match_en = 0;
	// stg0_lpb_cfg.mac_da_an_mask = 0x4;	/* MAC addres of LAN port locates at CAM table index 2, so mask bit 1 should be turned on */
	stg0_lpb_cfg.default_ldpid = AAL_LPORT_L3_WAN;
	stg0_lpb_cfg.t1_ctrl = l3fe_classifier_profile_lan;
	stg0_lpb_cfg.t5_ctrl = T5_CTRL_BYPASS;
	stg0_lpb_cfg.spcl_pkt_en = 1;
	stg0_lpb_cfg.spcl_pkt_sel = 1;
	if ((ret = aal_l3_stg0_lpb_tbl_set(0, 1, lpb_tbl_cfg_mask, &stg0_lpb_cfg)) != CA_E_OK) {
		ca_printf("%s: Fail to set STG0 LPB table profile#1. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/*** LPB Profile 2 ***/

	memset(&stg0_lpb_cfg, 0, sizeof(stg0_lpb_cfg));
	stg0_lpb_cfg.lspid = AAL_LPORT_L3_WAN;	/* STG0 sets this into HDR_I.lspid */
	stg0_lpb_cfg.mac_da_match_en = 0;
	// stg0_lpb_cfg.mac_da_an_mask = 0x80;
	stg0_lpb_cfg.default_ldpid = AAL_LPORT_L3_LAN;
	stg0_lpb_cfg.t1_ctrl = l3fe_classifier_profile_wan;
	stg0_lpb_cfg.t5_ctrl = T5_CTRL_BYPASS;
	stg0_lpb_cfg.spcl_pkt_en = 1;
	stg0_lpb_cfg.spcl_pkt_sel = 0;
	if ((ret = aal_l3_stg0_lpb_tbl_set(0, 2, lpb_tbl_cfg_mask, &stg0_lpb_cfg)) != CA_E_OK) {
		ca_printf("%s: Fail to set STG0 LPB table profile#2. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/*** LPB Profile 3 ***/

	memset(&stg0_lpb_cfg, 0, sizeof(stg0_lpb_cfg));
	stg0_lpb_cfg.lspid = AAL_LPORT_OAM;	/* STG0 sets this into HDR_I.lspid */
	stg0_lpb_cfg.mac_da_match_en = 0;
	stg0_lpb_cfg.mac_da_an_mask = 0xFF;
	stg0_lpb_cfg.default_ldpid = AAL_LPORT_L3_LAN;
	stg0_lpb_cfg.t1_ctrl = l3fe_classifier_profile_lan;
	stg0_lpb_cfg.t5_ctrl = T5_CTRL_BYPASS;
	stg0_lpb_cfg.spcl_pkt_en = 1;
	stg0_lpb_cfg.spcl_pkt_sel = 1;
	if ((ret = aal_l3_stg0_lpb_tbl_set(0, 3, lpb_tbl_cfg_mask, &stg0_lpb_cfg)) != CA_E_OK) {
		ca_printf("%s: Fail to set STG0 LPB table profile#3. (ret=%d)\n", __func__, ret);
		return ret;
	}

	return AAL_E_OK;
}

static int aal_l3fe_stg0_init(void)
{
	return aal_l3fe_stg0_set_normal();
}

static void aal_l3fe_init_startup_config(void)
{
	aal_scfg_read(CFG_ID_L3_CLASSIFIER_PROFILE_WAN, 1, &l3fe_classifier_profile_wan);
	aal_scfg_read(CFG_ID_L3_CLASSIFIER_PROFILE_LAN, 1, &l3fe_classifier_profile_lan);
	aal_scfg_read(CFG_ID_L3_MAIN_HASH_PROFILE_WAN, 1, &l3fe_main_hash_profile_wan);
	aal_scfg_read(CFG_ID_L3_MAIN_HASH_PROFILE_LAN, 1, &l3fe_main_hash_profile_lan);
	aal_scfg_read(CFG_ID_L3_MAIN_HASH_PROFILE_MC, 1, &l3fe_main_hash_profile_mc);
	aal_scfg_read(CFG_ID_L3_MAIN_HASH_PROFILE_CLASS, 1, &l3fe_main_hash_profile_class);
	aal_scfg_read(CFG_ID_L3_HASH_LITE_PROFILE, 1, &l3fe_hash_lite_profile);
	aal_scfg_read(CFG_ID_NAT_ENABLE, 1, &l3fe_nat_enable);
	aal_scfg_read(CFG_ID_L3FE_T5_CTRL_SUPPORT, 1, &l3fe_t5_ctrl_support);
	aal_scfg_read(CFG_ID_WAN_PORT_TYPE_IS_L3, 1, &l3fe_wan_port_type_is_l3);
}

int aal_l3fe_init(void)
{
	aal_l3fe_init_startup_config();

	aal_l3fe_stg0_init();
	aal_l3fe_dump_lpb(0);

	aal_l3fe_l2lookup_init();

	aal_hash_ttl_zero_ctrl();
	//aal_l3fe_axi_reo_init(0);

	aal_l3pe_init();

	return AAL_E_OK;
}

