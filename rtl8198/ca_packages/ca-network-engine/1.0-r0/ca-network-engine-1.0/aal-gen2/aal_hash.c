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
 * aal_hash.c: Hardware Abstraction Layer for Hash Engine to access hardware regsiters
 *
 */
#include <generated/ca_ne_autoconf.h>
#include <linux/crc32.h>
#include <linux/crc16.h>
#include <linux/sched.h>
#include <soc/cortina/cortina-soc.h>
#include "scfg.h"
#include "asm/io.h"
#include "aal_common.h"
#include "aal_table.h"
#include "aal_l3qm.h"
#include "aal_hash.h"
#include "ca_mut.h"
#include "ca_aal_proc.h"
#include "osal_misc.h"

typedef struct aal_default_action_s {
	ca_uint32_t type0_empty_miss_idx;
	ca_uint32_t type0_age_miss_idx;
	ca_uint32_t type1_empty_miss_idx;
	ca_uint32_t type1_age_miss_idx;
} aal_default_action_t;

#define DBG(x) {if (ca_aal_debug & AAL_DBG_HASH) x;}
#define TABLE_TRY_TIMEOUT	1000

#define ACTGRP_NAT_TO_WAN	(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_03 | ACTION_GROUP_05 | ACTION_GROUP_06 | \
				 ACTION_GROUP_17 | ACTION_GROUP_18 | ACTION_GROUP_19)

#define ACTGRP_NAT_TO_WAN_1	(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_17 | ACTION_GROUP_18 | ACTION_GROUP_19 | \
				 ACTION_GROUP_20)

#define ACTGRP_NAT_TO_LAN	(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_03 | ACTION_GROUP_04 | ACTION_GROUP_05 | \
				 ACTION_GROUP_08 | ACTION_GROUP_09 | ACTION_GROUP_17 | ACTION_GROUP_18 | ACTION_GROUP_19)

#define ACTGRP_NAT_TO_LAN_1	(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_08 | ACTION_GROUP_09 | ACTION_GROUP_17 | \
				 ACTION_GROUP_18 | ACTION_GROUP_19 | ACTION_GROUP_20)

#define ACTGRP_MULTICAST	(ACTION_GROUP_07 | ACTION_GROUP_08 | ACTION_GROUP_09 | ACTION_GROUP_10 | ACTION_GROUP_11 | \
				 ACTION_GROUP_17 | ACTION_GROUP_18 | ACTION_GROUP_19)

#define ACTGRP_FLOW		(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_03 | ACTION_GROUP_05 | ACTION_GROUP_07 | \
				 ACTION_GROUP_08 | ACTION_GROUP_09 | ACTION_GROUP_10 | ACTION_GROUP_11 | ACTION_GROUP_12 | \
				 ACTION_GROUP_13 | ACTION_GROUP_14 | ACTION_GROUP_15 | ACTION_GROUP_16 | ACTION_GROUP_17 | \
				 ACTION_GROUP_18 | ACTION_GROUP_19 | ACTION_GROUP_22)

#define	ACTGRP_FLOW_SHORT	(ACTION_GROUP_01 | ACTION_GROUP_02 | ACTION_GROUP_17 | ACTION_GROUP_19 | ACTION_GROUP_20)

#if defined(CONFIG_LUNA_G3_SERIES)

#include "aal_l3fe.h"

#define ACTGRP_FLOW_RTK_DS (ACTION_GROUP_01 | ACTION_GROUP_09 | ACTION_GROUP_13 | ACTION_GROUP_16 | ACTION_GROUP_17 | \
				  ACTION_GROUP_18 | ACTION_GROUP_19 | ACTION_GROUP_20 | ACTION_GROUP_22)		// WAN TO LAN: 254 bits (<256)

#define ACTGRP_FLOW_RTK_US (ACTION_GROUP_02 | ACTION_GROUP_09 | ACTION_GROUP_13 | ACTION_GROUP_16 | ACTION_GROUP_17 | \
				 ACTION_GROUP_18 | ACTION_GROUP_19 | ACTION_GROUP_20 | ACTION_GROUP_22)		// LAN TO WAN: 254 bits (<256)

#define ACTGRP_FLOW_RTK_MC (ACTION_GROUP_01 | ACTION_GROUP_07 | ACTION_GROUP_09 | ACTION_GROUP_16 | ACTION_GROUP_17 | ACTION_GROUP_18 | \
				 ACTION_GROUP_19 | ACTION_GROUP_20 | ACTION_GROUP_22)		// WAN TO LAN multicast: 252 bits (<256)
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
#define ACTGRP_L3_DEFAULT      (ACTION_GROUP_13 | ACTION_GROUP_14 | ACTION_GROUP_18 | ACTION_GROUP_19 | ACTION_GROUP_22)
#else
#define ACTGRP_L3_DEFAULT	(ACTION_GROUP_18 | ACTION_GROUP_19)
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
#define ACTGRP_CLS_HASH_FIB	0 // type-1 not used
#else
#define ACTGRP_CLS_HASH_FIB	(ACTION_GROUP_08 | ACTION_GROUP_09 | ACTION_GROUP_11 | ACTION_GROUP_12 | ACTION_GROUP_13 | \
				 ACTION_GROUP_14 | ACTION_GROUP_15 | ACTION_GROUP_16 | ACTION_GROUP_17 | ACTION_GROUP_18 | \
				 ACTION_GROUP_19)
#endif

static spinlock_t g_hash_lock;			/* protecting AAL tables only */
static unsigned int g_aging_time;		/* time unit is second */
static unsigned int g_aging_granularity;	/* Should be same as the value in HW reg */
static unsigned int g_max_aging_time;		/* max aging time in second */
static unsigned int g_hw_ht_size = 0;
static unsigned int g_ht_size = HASH_HT_SIZE_64K;
static unsigned int g_hb_size = HASH_HB_SIZE_32;
static unsigned int g_ha_width = HASH_HA_WIDTH_1024;
static unsigned int g_ct_size = HASH_CT_SIZE_2K;
static unsigned int g_ca_width = HASH_HA_WIDTH_1024; // should be same as g_ha_width
static unsigned int g_da_width = HASH_HA_WIDTH_1024;
static unsigned int g_oa_width = HASH_HA_WIDTH_1024;

/* HW provide disable, 256, 512, 1024, 2048-entry cache for cache table size */
static const size_t ct_size_array[5] = /* entrys */
	{0 /* disable */, 256 /* 256-entry cache*/, 512 /* 512-entry cache*/, 1024 /* 1024-entry cache*/, 2048 /* 2048-entry cache*/};

/* HW provide 64, 128, 192, 256, 384, 512, 768, 1024 bits space for hash action */
static const size_t ha_width_array[8] = /* bytes */
	{8 /* 64/8 */, 16 /* 128/8 */, 24 /* 192/8 */, 32 /* 256/8 */,
	48 /* 384/8 */, 64 /* 512/8 */, 96 /* 768/8 */, 128 /* 1024/8 */};

#if defined(CONFIG_LUNA_G3_SERIES)
ca_uint8_t current_profile_count = 4;
#else
ca_uint8_t current_profile_count = 3;
#endif

static aal_table_t hash_tbl_table;
static aal_table_t cache_hash_table;
static aal_table_t cache_hash_action_table;
static aal_table_t overflow_tbl_table;
static aal_table_t overflow_action_table;
static aal_table_t hash_action_table;
static aal_table_t hash_age_table;
static aal_table_t mask_tbl_table;
static aal_table_t default_action_table;

static aal_l3qm_l3fe_main_hash_info_t l3fe_main_hash_info;

static ca_uint_t hash_overflow_hash_fib_virtual_addr;
static ca_uint32_t hash_overflow_hash_fib_phy_addr;
static ca_uint16_t hash_overflow_hash_fib_length;
static ca_uint16_t hash_overflow_hash_fib_count;

static ca_uint_t hash_default_hash_fib_virtual_addr;
static ca_uint32_t hash_default_hash_fib_phy_addr;
static ca_uint16_t hash_default_hash_fib_length;
static ca_uint16_t hash_default_hash_fib_count;

static ca_uint_t hash_cache_fib_virtual_addr;
static ca_uint32_t hash_cache_fib_phy_addr;
static ca_uint16_t hash_cache_fib_length;
static ca_uint16_t hash_cache_fib_count;

static ca_uint_t hash_table_virtual_addr;
static ca_uint32_t hash_table_phy_addr;
static ca_uint16_t hash_table_entry_length;
static ca_uint32_t hash_table_entry_count;

static ca_uint_t hash_action_table_virtual_addr;
static ca_uint32_t hash_action_table_phy_addr;
static ca_uint16_t hash_action_table_entry_length;
static ca_uint32_t hash_action_table_entry_count;
//static ca_uint32_t hash_key_select = 0;

ca_uint32_t hash_key_select[HASH_PROFILE_MAX] = {0};
ca_uint32_t hash_tuple_sp_cfg[HASH_PROFILE_MAX] = {0};
ca_uint32_t hash_tuple_dp_cfg[HASH_PROFILE_MAX] = {0};
ca_uint32_t hash_tuple_sip_cfg[HASH_PROFILE_MAX] = {0};
ca_uint32_t hash_tuple_dip_cfg[HASH_PROFILE_MAX] = {0};

static ca_boolean_t cache_hash_enabled = FALSE; /* Stage 3 Allocation Enable */
static ca_boolean_t cache_hash_replacement_enabled = FALSE;
static ca_boolean_t cache_hash_stage3_loc = FALSE;
static ca_boolean_t cache_hash_stage2_allocation_enabled = FALSE; /* Stage 2 Allocation Enable */
static ca_uint8_t cache_hash_stage2_allocation_pri = 0; /* s2_ra_pri[1:0] Stage 2 Allocation Priority Bits*/

static ca_uint16_t cache_hash_age_ut_t0 = 0xffff;
static ca_uint16_t cache_hash_age_ut_t1 = 0xffff;
static ca_uint16_t cache_hash_age_ut_t2 = 0xffff;
static ca_uint16_t cache_hash_age_ut_t3 = 0xffff;

static ca_uint8_t axi_read_outtrans_num = 0xf;
static ca_uint8_t axi_write_outtrans_num = 0xf;

static ca_uint32_t ip_ecn_support = 0;
static ca_uint8_t flow_support = 0;
static ca_uint8_t l3fe_hash_indexed_mode = 0;

static aal_default_action_t def_action[HASH_PROFILE_MAX];

static int __mask_tbl_entry_add(void *entry, unsigned int *rslt_idx);
static int __mask_tbl_entry_get(void *entry, unsigned int *rslt_idx);
static int __mask_tbl_entry_get_by_idx(unsigned int idx, void *entry);
static int __mask_tbl_entry_del(unsigned int idx);
static int __default_action_entry_add(void *entry, unsigned int *rslt_idx);
static int __default_action_entry_get(void *entry, unsigned int *rslt_idx);
static int __hash_tbl_entry_get_by_idx(unsigned int idx, void *entry);

int aal_hash_cache_init(unsigned int ct_size, unsigned int ca_width);
int aal_hash_cache_action_init(void);
int aal_hash_default_action_da_width_set(unsigned int da_width);
int aal_hash_overflow_action_oa_width_set(unsigned int oa_width);

/* crc16_mask[HB_SIZE][HT_SIZE]
 *   Mask with hash's crc16 for the bucket index.
 *   HB_SIZE: 0~3 for bucket size 4, 8, 16, 32.
 *   HT_SIZE: 0~7 for hash table size 512, 1k, 2k, 4k, 8k, 16k, 32k, 64k.
 *   See the MAS file, figure 19: Configuration of Main Hash Table Sizes, for detail.
 */
static u16 crc16_mask[4][8] = {
	{ 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc },
	{ 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8 },
	{ 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0 },
	{ 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0 }};


/***************************************************************************
 * Hash Engine Initialization
 ***************************************************************************/

aal_hash_mask_t HASH_MASK_NAT;
aal_hash_mask_t HASH_MASK_MULTICAST;
aal_hash_mask_t HASH_MASK_IP_PROT;

unsigned int hash_mask_nat_idx;
unsigned int hash_mask_multicast_idx;
unsigned int hash_mask_ip_prot_idx;

#if defined(CONFIG_LUNA_G3_SERIES) &&defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH)
aal_hash_mask_t HASH_MASK_MULTICAST_FORWARD;
unsigned int hash_mask_multicast_forward_idx;
EXPORT_SYMBOL(hash_mask_multicast_idx);
EXPORT_SYMBOL(hash_mask_multicast_forward_idx);
#endif


hash_default_action_t HASH_ACT_DROP = {
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
	.t3_ctrl_vld = 1,
	.t3_ctrl = 0xf,
	.stage3_ctrl_vld = 1,
	.stage3_ctrl = 0,
	.permit = 0,	// drop the packet
	.dpid_vld = 1,
	.dpid_pri = 1,
};

hash_default_action_t WAN_NAT_HASH_ACT_DEFAULT_TO_CPU = {
#if defined(CONFIG_LUNA_G3_SERIES)
	/*To prevent interface counter counts packets which are trapped to SW.
	   Change ingress inferface_index and egress inferface_index to 0 of hash miss packet.*/
	.mdata_w_vld_1 = 0xf,
	.mdata_w_1 = 0,
	.mdata_w_vld_2 = 0x8,
	.mdata_w_2 = 0x2000,	//RXINFO_REF_TRAP_RSN_FLOWMISS
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
#else
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
#endif
	.t3_ctrl_vld = 1,
	.t3_ctrl = 0xf,
	.stage3_ctrl_vld = 1,
	.stage3_ctrl = 0,
	.permit = 1,	// forward the packet
	.dpid_vld = 1,
	.dpid_pri = 1,
	.mcgid = AAL_LPORT_CPU_0,
	.mc = 0,	// mc = 0 means "mcgid is LDPID"
	.keep_orig_pkt = 1,
	.keep_orig_pkt_vld = 1,
};

hash_default_action_t LAN_NAT_HASH_ACT_DEFAULT_TO_CPU = {
#if defined(CONFIG_LUNA_G3_SERIES)
	/*To prevent interface counter counts packets which are trapped to SW.
	   Change ingress inferface_index and egress inferface_index to 0 of hash miss packet.*/
	.mdata_w_vld_1 = 0xf,
	.mdata_w_1 = 0,
	.mdata_w_vld_2 = 0x8,
	.mdata_w_2 = 0x2000,	//RXINFO_REF_TRAP_RSN_FLOWMISS
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
#else
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
#endif
	.t3_ctrl_vld = 1,
	.t3_ctrl = 0xf,
	.stage2_ctrl_vld = 1,
	.stage2_ctrl = 0,
	.stage3_ctrl_vld = 1,
	.stage3_ctrl = 0,
	.permit = 1,	// forward the packet
	.dpid_vld = 1,
	.dpid_pri = 1,
#if defined(CONFIG_LUNA_G3_SERIES)
	.mcgid = AAL_LPORT_CPU_0,
#else
	.mcgid = AAL_LPORT_CPU_1,
#endif
	.mc = 0,	// mc = 0 means "mcgid is LDPID"
	.keep_orig_pkt = 1,
	.keep_orig_pkt_vld = 1,
};

/* TYPE1 hash default action: define later */
hash_default_action_t WAN_NAT_TYPE1_HASH_DEFAULT_ACT = {
	.t5_ctrl_vld = 0,
	.t5_ctrl = 0,
	.t4_ctrl_vld = 0,
	.t4_ctrl = 0,
	.t3_ctrl_vld = 0,
	.t3_ctrl = 0,
	.dpid_vld = 0,
	.dpid_pri = 0,
	.keep_orig_pkt = 0,
	.keep_orig_pkt_vld = 0,
};
/* TYPE1 hash default action: define later */
hash_default_action_t LAN_NAT_TYPE1_HASH_DEFAULT_ACT = {
	.t5_ctrl_vld = 0,
	.t5_ctrl = 0,
	.t4_ctrl_vld = 0,
	.t4_ctrl = 0,
	.t3_ctrl_vld = 0,
	.t3_ctrl = 0,
	.dpid_vld = 0,
	.dpid_pri = 0,
	.keep_orig_pkt = 0,
	.keep_orig_pkt_vld = 0,
};
/* TYPE1 hash default action: define later */
hash_default_action_t MULTICAST_TYPE1_HASH_DEFAULT_ACT = {
	.t5_ctrl_vld = 0,
	.t5_ctrl = 0,
	.t4_ctrl_vld = 0,
	.t4_ctrl = 0,
	.t3_ctrl_vld = 0,
	.t3_ctrl = 0,
	.dpid_vld = 0,
	.dpid_pri = 0,
	.keep_orig_pkt = 0,
	.keep_orig_pkt_vld = 0,
};

#if defined(CONFIG_LUNA_G3_SERIES)
//Set a new hash default action setting for hash profile 2 (MC) for avoiding use the same default action entry with others.
hash_default_action_t MC_HASH_ACT_DEFAULT_TO_CPU = {
	.mdata_w_vld_1 = 0xf,
	.mdata_w_1 = 0,
	.mdata_w_vld_2 = 0x8,
	.mdata_w_2 = 0x2000,	//RXINFO_REF_TRAP_RSN_FLOWMISS
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
	.t3_ctrl_vld = 1,
	.t3_ctrl = 0xf,
	.stage3_ctrl_vld = 1,
	.stage3_ctrl = 0,
	.permit = 1,	// forward the packet
	.dpid_vld = 1,
	.dpid_pri = 1,
	.mcgid = AAL_LPORT_CPU_0,
	.mc = 0,	// mc = 0 means "mcgid is LDPID"
	.keep_orig_pkt = 1,
	.keep_orig_pkt_vld = 1,
	.keep_ts_vld = 1, //added for variating with others
	.keep_ts_en = 0, //added for variating with others
};

hash_default_action_t CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL = {
	.pol2_id_msb = 0,
	.pol2_msb_en = 1,
	.pol23_vld = 1,
	.t5_ctrl_vld = 1,
	.t5_ctrl = 0xf,
	.t4_ctrl_vld = 1,
	.t4_ctrl = 0xf,
	.t3_ctrl_vld = 1,
	.t3_ctrl = 0xf,
	.stage3_ctrl_vld = 1,
	.stage3_ctrl = 0,
	.permit = 1,	// forward the packet
	.dpid_vld = 1,
	.dpid_pri = 1,
	.mcgid = AAL_LPORT_CPU_0,
	.mc = 0,	// mc = 0 means "mcgid is LDPID"
	.keep_orig_pkt = 1,
	.keep_orig_pkt_vld = 1,
	.keep_ts_vld = 1, //added for variating with others
	.keep_ts_en = 0, //added for variating with others
};


/* ===============================================================	*/
/* Instead of CA 2 hash profiles, RTK use following hash profiles to meet flow-based architecture.	*/
/* 0. Unicast 5 tuple hash for WAN														*/
/* 1. Unicast 5 tuple hash for LAN															*/
/* 2. Multicast 																		*/
/* 3. Unicast 2 tuple hash for upstream and downstream (bridge only)							*/
/* ===============================================================	*/
// UC 5 tuple downstream
aal_hash_profile_t hash_profile_0 = {
	.type0_act_grp_mask_hit =	ACTGRP_FLOW_RTK_DS,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_empty_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_age_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

// UC 5 tuple upstream
aal_hash_profile_t hash_profile_1 = {
	.type0_act_grp_mask_hit =	ACTGRP_FLOW_RTK_US,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_empty_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_age_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

// MC
aal_hash_profile_t hash_profile_2 = {
	.type0_act_grp_mask_hit =	ACTGRP_FLOW_RTK_MC,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&MC_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(MC_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&MC_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(MC_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&MC_HASH_ACT_DEFAULT_TO_CPU,
		.type1_empty_miss_len =	sizeof(MC_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&MC_HASH_ACT_DEFAULT_TO_CPU,
		.type1_age_miss_len =	sizeof(MC_HASH_ACT_DEFAULT_TO_CPU),
	},

#if defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH)
	.hit = {
		{ TUPLE_TYPE_FWD,	&HASH_MASK_MULTICAST, 1, 0 },
		{ TUPLE_TYPE_FWD,	&HASH_MASK_MULTICAST_FORWARD, 1, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},

#else
	.hit = {
		{ TUPLE_TYPE_FWD,	&HASH_MASK_MULTICAST, 1, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
},

#endif

};

// UC 2 tuple upstream and downstream bridge
aal_hash_profile_t hash_profile_3 = {
	.type0_act_grp_mask_hit =	ACTGRP_FLOW_RTK_US,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_empty_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type1_age_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

//[8277B] for packet trap with group policer by L3 CLS, need Hash fib to enable pol2_en
aal_hash_profile_t hash_profile_4 = {
	.type0_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB, // no rules, only default action
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL,
		.type0_empty_miss_len =	sizeof(CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL,
		.type0_age_miss_len =	sizeof(CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL,
		.type1_empty_miss_len =	sizeof(CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL,
		.type1_age_miss_len =	sizeof(CLS_HASH_ACT_DEFAULT_TRAP_GRPPOL),
	},

	.hit = {
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

#else
/* WAN NAT profile */
aal_hash_profile_t hash_profile_0 = {
	.type0_act_grp_mask_hit =	ACTGRP_NAT_TO_LAN,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&WAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(WAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&WAN_NAT_TYPE1_HASH_DEFAULT_ACT,
		.type1_empty_miss_len =	sizeof(WAN_NAT_TYPE1_HASH_DEFAULT_ACT),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&WAN_NAT_TYPE1_HASH_DEFAULT_ACT,
		.type1_age_miss_len =	sizeof(WAN_NAT_TYPE1_HASH_DEFAULT_ACT),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HASH_MASK_NAT, 1, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

/* LAN NAT profile */
aal_hash_profile_t hash_profile_1 = {
	.type0_act_grp_mask_hit =	ACTGRP_NAT_TO_WAN,
	.type1_act_grp_mask_hit =	ACTGRP_CLS_HASH_FIB,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_empty_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&LAN_NAT_HASH_ACT_DEFAULT_TO_CPU,
		.type0_age_miss_len =	sizeof(LAN_NAT_HASH_ACT_DEFAULT_TO_CPU),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&LAN_NAT_TYPE1_HASH_DEFAULT_ACT,
		.type1_empty_miss_len =	sizeof(LAN_NAT_TYPE1_HASH_DEFAULT_ACT),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&LAN_NAT_TYPE1_HASH_DEFAULT_ACT,
		.type1_age_miss_len =	sizeof(LAN_NAT_TYPE1_HASH_DEFAULT_ACT),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HASH_MASK_NAT, 1, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};

/* Multicast profile */
aal_hash_profile_t hash_profile_2 = {
	.type0_act_grp_mask_hit =	ACTGRP_MULTICAST,
	.type1_act_grp_mask_hit =	ACTGRP_MULTICAST,

	.miss = {
		.type0_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type0_empty_miss =	&HASH_ACT_DROP,
		.type0_empty_miss_len =	sizeof(HASH_ACT_DROP),

		.type0_act_grp_mask_age_miss = 		ACTGRP_L3_DEFAULT,
		.type0_age_miss =	&HASH_ACT_DROP,
		.type0_age_miss_len =	sizeof(HASH_ACT_DROP),

		.type1_act_grp_mask_empty_miss =	ACTGRP_L3_DEFAULT,
		.type1_empty_miss =	&MULTICAST_TYPE1_HASH_DEFAULT_ACT,
		.type1_empty_miss_len =	sizeof(MULTICAST_TYPE1_HASH_DEFAULT_ACT),

		.type1_act_grp_mask_age_miss =		ACTGRP_L3_DEFAULT,
		.type1_age_miss =	&MULTICAST_TYPE1_HASH_DEFAULT_ACT,
		.type1_age_miss_len =	sizeof(MULTICAST_TYPE1_HASH_DEFAULT_ACT),
	},

	.hit = {
		{ TUPLE_TYPE_FWD,	&HASH_MASK_IP_PROT, 1, 0 },
		{ TUPLE_TYPE_FWD,	&HASH_MASK_MULTICAST, 1, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
		{ TUPLE_TYPE_UNUSED,	NULL, 0, 0 },
	},
};
#endif

#define UNIT_TEST 1

#if UNIT_TEST
ca_status_t aal_hash_crc_sw_hw_calc_check (
CA_IN	aal_hash_key_t *hash_key,
CA_IN	ca_uint32_t	mask_id,
CA_IN	ca_uint32_t	sw_crc32,
CA_IN	ca_uint16_t	sw_crc16);
#endif

ca_uint32_t hash_idx_umc_icmpv6;

/***************************************************************************
 * internal functions
 ***************************************************************************/
static void dump_HS_ACTGRP_L3_DEFAULT(hash_default_action_t *act)
{
#if defined(CONFIG_LUNA_G3_SERIES)
	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 14, MDATA Word 2>\n");
	printk(KERN_INFO " mdata_w_vld_2(4): 0x%x, mdata_w_2(16): 0x%04x\n", act->mdata_w_vld_2, act->mdata_w_2);
#endif
	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);

#if defined(CONFIG_LUNA_G3_SERIES)
	printk(KERN_INFO "<Group 22, Pol2>\n");
	printk(KERN_INFO " pol23_vld(1): %d\n", act->pol23_vld);
	printk(KERN_INFO " pol3_en(1): %d, pol3_id(6): %d\n", act->pol3_en, act->pol3_id);
	printk(KERN_INFO " pol2_msb_en(1): %d, pol2_id_msb(6): %d\n", act->pol2_msb_en, act->pol2_id_msb);
#endif
}

static void dump_HS_ACTGRP_NAT_TO_LAN(hash_action_to_lan_t *act)
{
	printk(KERN_INFO "<Group 1, L4 DP>\n");
	printk(KERN_INFO " l4_dp_vld: %d, l4_dp(16): 0x%04x\n", act->l4_dp_vld, act->l4_dp);

	printk(KERN_INFO "<Group 2, L4 SP>\n");
	printk(KERN_INFO " l4_sp_vld: %d, l4_sp(16): 0x%04x\n", act->l4_sp_vld, act->l4_sp);

	printk(KERN_INFO "<Group 3, IP DA Low 32 bits>\n");
	printk(KERN_INFO " ip_da_l_vld: %d, ip_da_0(32): 0x%08x\n", act->ip_da_l_vld, act->ip_da_0);

	printk(KERN_INFO "<Group 4, IP DA High 96 bits>\n");
	printk(KERN_INFO " ip_da_h_vld: %d, ip_da_3(32): 0x%08x, ip_da_2(32): 0x%08x, ip_da_1(32): 0x%08x\n",
		act->ip_da_h_vld, act->ip_da_3, act->ip_da_2, act->ip_da_1);

	printk(KERN_INFO "<Group 5, IP SA Low 32 bits>\n");
	printk(KERN_INFO " ip_sa_l_vld: %d, ip_sa_0(32): 0x%08x\n", act->ip_sa_l_vld, act->ip_sa_0);

	printk(KERN_INFO "<Group 8, l3_egress_if>\n");
	printk(KERN_INFO " ip_ttl_vld: %d, ip_ttl_zero_discard_en: 0x%x\n", act->ip_ttl_vld, act->ip_ttl_zero_discard_en);
	printk(KERN_INFO " ip_ttl_cmd: 0x%x, ip_ttl: 0x%x\n", act->ip_ttl_cmd, act->ip_ttl);
	printk(KERN_INFO " six_rd_ipda_ctrl_vld: %d, six_rd_ipda_from_v6: 0x%x\n", act->six_rd_ipda_ctrl_vld, act->six_rd_ipda_from_v6);
	printk(KERN_INFO " ip_mtu_enc_vld: %d, ip_mtu_enc: 0x%x\n", act->ip_mtu_enc_vld, act->ip_mtu_enc);
	printk(KERN_INFO " l2_format_vld: %d, l2_format: 0x%x\n", act->l2_format_vld, act->l2_format);
	printk(KERN_INFO " l3_egress_if_vld: %d\n", act->l3_egress_if_vld);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);
}

static void dump_HS_ACTGRP_NAT_TO_WAN(hash_action_to_wan_t *act)
{
	printk(KERN_INFO "<Group 1, L4 DP>\n");
	printk(KERN_INFO " l4_dp_vld: %d, l4_dp(16): 0x%04x\n", act->l4_dp_vld, act->l4_dp);

	printk(KERN_INFO "<Group 2, L4 SP>\n");
	printk(KERN_INFO " l4_sp_vld: %d, l4_sp(16): 0x%04x\n", act->l4_sp_vld, act->l4_sp);

	printk(KERN_INFO "<Group 3, IP DA Low 32 bits>\n");
	printk(KERN_INFO " ip_da_l_vld: %d, ip_da_0(32): 0x%08x\n", act->ip_da_l_vld, act->ip_da_0);
	printk(KERN_INFO "<Group 5, IP SA Low 32 bits>\n");
	printk(KERN_INFO " ip_sa_l_vld: %d, ip_sa_0(32): 0x%08x\n", act->ip_sa_l_vld, act->ip_sa_0);

	printk(KERN_INFO "<Group 6, IP SA High 96 bits>\n");
	printk(KERN_INFO " ip_sa_h_vld: %d, ip_sa_3(32): 0x%08x, ip_sa_2(32): 0x%08x, ip_sa_1(32): 0x%08x\n",
		act->ip_sa_h_vld, act->ip_sa_3, act->ip_sa_2, act->ip_sa_1);
	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);
}

static void dump_HS_ACTGRP_MULTICAST(hash_action_multicast_t *act)
{
	printk(KERN_INFO "<Group 7, PPPoE>\n");
	printk(KERN_INFO " pppoe_vld: %d, pppoe_session_id(16): 0x%08x\n", act->pppoe_vld, act->pppoe_session_id);
	printk(KERN_INFO " pppoe_set: %d \n", act->pppoe_set);

	printk(KERN_INFO "<Group 8, l3_egress_if>\n");
	printk(KERN_INFO " ip_ttl_vld: %d, ip_ttl_zero_discard_en: 0x%x \n", act->ip_ttl_vld, act->ip_ttl_zero_discard_en);
	printk(KERN_INFO " ip_ttl_cmd: 0x%x, ip_ttl: 0x%x \n", act->ip_ttl_cmd, act->ip_ttl);
	printk(KERN_INFO " six_rd_ipda_ctrl_vld: %d, six_rd_ipda_from_v6: 0x%x \n", act->six_rd_ipda_ctrl_vld, act->six_rd_ipda_from_v6);
	printk(KERN_INFO " ip_mtu_enc_vld: %d, ip_mtu_enc: 0x%x \n", act->ip_mtu_enc_vld, act->ip_mtu_enc);
	printk(KERN_INFO " l2_format_vld: %d, l2_format: 0x%x \n", act->l2_format_vld, act->l2_format);
	printk(KERN_INFO " l3_egress_if_vld: %d \n", act->l3_egress_if_vld);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 10, MAC SA>\n");
	printk(KERN_INFO " mac_sa_vld: %d, mac_sa: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_sa_vld, act->mac_sa_5, act->mac_sa_4, act->mac_sa_3, act->mac_sa_2, act->mac_sa_1, act->mac_sa_0);

	printk(KERN_INFO "<Group 11, MAC DA>\n");
	printk(KERN_INFO " mac_da_vld: %d, mac_da: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_da_vld, act->mac_da_5, act->mac_da_4, act->mac_da_3, act->mac_da_2, act->mac_da_1, act->mac_da_0);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);
}

static void dump_HS_ACTGRP_TYPE1_CLS_FIB(hash_action_type1_cls_hash_t*act)
{
	printk(KERN_INFO "<Group 8, L3 Egress IF>\n");
	printk(KERN_INFO " ip_ttl_vld: %d, ip_ttl_zero_discard_en: %d, ip_ttl_cmd(2): %d, ip_ttl(8): 0x%02x\n",
			act->ip_ttl_vld, act->ip_ttl_zero_discard_en, act->ip_ttl_cmd, act->ip_ttl);
	printk(KERN_INFO " 6rd_ipda_ctrl_vld: %d, 6rd_ipda_from_v6: %d\n", act->six_rd_ipda_ctrl_vld, act->six_rd_ipda_from_v6);
	printk(KERN_INFO " ip_mtu_enc_vld: %d, ip_mtu_enc(2): %d\n", act->ip_mtu_enc_vld, act->ip_mtu_enc);
	printk(KERN_INFO " l2_format_vld: %d, l2_format: %d, l3_egress_if_vld: %d\n",
		act->l2_format_vld, act->l2_format, act->l3_egress_if_vld);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 11, MAC DA>\n");
	printk(KERN_INFO " mac_da_vld: %d, mac_da: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_da_vld, act->mac_da_5, act->mac_da_4, act->mac_da_3, act->mac_da_2, act->mac_da_1, act->mac_da_0);

	printk(KERN_INFO "<Group 12, MDATA Word 0>\n");
	printk(KERN_INFO " mdata_w_vld_0(4): 0x%x, mdata_w_0(16): 0x%04x\n", act->mdata_w_vld_0, act->mdata_w_0);

	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 14, MDATA Word 2>\n");
	printk(KERN_INFO " mdata_w_vld_2(4): 0x%x, mdata_w_2(16): 0x%04x\n", act->mdata_w_vld_2, act->mdata_w_2);

	printk(KERN_INFO "<Group 15, MDATA Word 3>\n");
	printk(KERN_INFO " mdata_w_vld_3(4): 0x%x, mdata_w_3(16): 0x%04x\n", act->mdata_w_vld_3, act->mdata_w_3);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);

}

static void dump_HS_ACTGRP_FLOW(hash_action_flow_t *act)
{
	printk(KERN_INFO "<Group 1, L4 DP>\n");
	printk(KERN_INFO " l4_dp_vld: %d, l4_dp(16): 0x%04x\n", act->l4_dp_vld, act->l4_dp);

	printk(KERN_INFO "<Group 2, L4 SP>\n");
	printk(KERN_INFO " l4_sp_vld: %d, l4_sp(16): 0x%04x\n", act->l4_sp_vld, act->l4_sp);

	printk(KERN_INFO "<Group 3, IP DA Low 32 bits>\n");
	printk(KERN_INFO " ip_da_l_vld: %d, ip_da_0(32): 0x%08x\n", act->ip_da_l_vld, act->ip_da_0);

	printk(KERN_INFO "<Group 5, IP SA Low 32 bits>\n");
	printk(KERN_INFO " ip_sa_l_vld: %d, ip_sa_0(32): 0x%08x\n", act->ip_sa_l_vld, act->ip_sa_0);

	printk(KERN_INFO "<Group 7, PPPoE fields>\n");
	printk(KERN_INFO " pppoe_vld: %d, pppoe_set: %d, pppoe_session_id(16): 0x%04x\n",
		act->pppoe_vld, act->pppoe_set, act->pppoe_session_id);

	printk(KERN_INFO "<Group 8, L3 Egress IF>\n");
	printk(KERN_INFO " ip_ttl_vld: %d, ip_ttl_zero_discard_en: %d, ip_ttl_cmd(2): %d, ip_ttl(8): 0x%02x\n",
		act->ip_ttl_vld, act->ip_ttl_zero_discard_en, act->ip_ttl_cmd, act->ip_ttl);
	printk(KERN_INFO " 6rd_ipda_ctrl_vld: %d, 6rd_ipda_from_v6: %d\n", act->six_rd_ipda_ctrl_vld, act->six_rd_ipda_from_v6);
	printk(KERN_INFO " ip_mtu_enc_vld: %d, ip_mtu_enc(2): %d\n", act->ip_mtu_enc_vld, act->ip_mtu_enc);
	printk(KERN_INFO " l2_format_vld: %d, l2_format: %d, l3_egress_if_vld: %d\n",
		act->l2_format_vld, act->l2_format, act->l3_egress_if_vld);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 10, MAC SA>\n");
	printk(KERN_INFO " mac_sa_vld: %d, mac_sa: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_sa_vld, act->mac_sa_5, act->mac_sa_4, act->mac_sa_3, act->mac_sa_2, act->mac_sa_1, act->mac_sa_0);

	printk(KERN_INFO "<Group 11, MAC DA>\n");
	printk(KERN_INFO " mac_da_vld: %d, mac_da: %02x-%02x-%02x-%02x-%02x-%02x\n",
		act->mac_da_vld, act->mac_da_5, act->mac_da_4, act->mac_da_3, act->mac_da_2, act->mac_da_1, act->mac_da_0);

	printk(KERN_INFO "<Group 12, MDATA Word 0>\n");
	printk(KERN_INFO " mdata_w_vld_0(4): 0x%x, mdata_w_0(16): 0x%04x\n", act->mdata_w_vld_0, act->mdata_w_0);

	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 14, MDATA Word 2>\n");
	printk(KERN_INFO " mdata_w_vld_2(4): 0x%x, mdata_w_2(16): 0x%04x\n", act->mdata_w_vld_2, act->mdata_w_2);

	printk(KERN_INFO "<Group 15, MDATA Word 3>\n");
	printk(KERN_INFO " mdata_w_vld_3(4): 0x%x, mdata_w_3(16): 0x%04x\n", act->mdata_w_vld_3, act->mdata_w_3);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);
}
#if defined(CONFIG_LUNA_G3_SERIES)
static void dump_HS_ACTGRP_FLOW_RTK_DS(hash_action_flow_rtk_ds_t *act)
{
	printk(KERN_INFO "<Group 1, L4 DP>\n");
	printk(KERN_INFO " l4_dp_vld: %d, l4_dp(16): 0x%04x\n", act->l4_dp_vld, act->l4_dp);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);

	printk(KERN_INFO "<Group 20, Indexed>\n");
	printk(KERN_INFO " mdata_byte_vld(1): %d, mdata_byte_idx(1): %d, mdata_byte(8): 0x%02x\n", act->mdata_byte_vld, act->mdata_byte_idx, act->mdata_byte);
	printk(KERN_INFO " l3_egress_if_vld1(1): %d\n", act->l3_egress_if_vld1);
	printk(KERN_INFO " smac_trans(1): %d\n", act->smac_trans);
	printk(KERN_INFO " ip_ttl_dec1(1): %d, ip_ttl_zero_discard_en1(1): %d\n", act->ip_ttl_dec1, act->ip_ttl_zero_discard_en1);
	printk(KERN_INFO " ip_idx_vld(1): %d, ip_type(1): %d, ip_idx(10): %d\n", act->ip_idx_vld, act->ip_type, act->ip_idx);
	printk(KERN_INFO " mac_da_idx_vld(1): %d, mac_da_idx(10): %d\n", act->mac_da_idx_vld, act->mac_da_idx);

	printk(KERN_INFO "<Group 22, Pol2>\n");
	printk(KERN_INFO " pol23_vld(1): %d\n", act->pol23_vld);
	printk(KERN_INFO " pol3_en(1): %d, pol3_id(6): %d\n", act->pol3_en, act->pol3_id);
	printk(KERN_INFO " pol2_msb_en(1): %d, pol2_id_msb(6): %d\n", act->pol2_msb_en, act->pol2_id_msb);
}

static void dump_HS_ACTGRP_FLOW_RTK_US(hash_action_flow_rtk_us_t *act)
{
	printk(KERN_INFO "<Group 2, L4 SP>\n");
	printk(KERN_INFO " l4_sp_vld: %d, l4_sp(16): 0x%04x\n", act->l4_sp_vld, act->l4_sp);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 13, MDATA Word 1>\n");
	printk(KERN_INFO " mdata_w_vld_1(4): 0x%x, mdata_w_1(16): 0x%04x\n", act->mdata_w_vld_1, act->mdata_w_1);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);

	printk(KERN_INFO "<Group 20, Indexed>\n");
	printk(KERN_INFO " mdata_byte_vld(1): %d, mdata_byte_idx(1): %d, mdata_byte(8): 0x%02x\n", act->mdata_byte_vld, act->mdata_byte_idx, act->mdata_byte);
	printk(KERN_INFO " l3_egress_if_vld1(1): %d\n", act->l3_egress_if_vld1);
	printk(KERN_INFO " smac_trans(1): %d\n", act->smac_trans);
	printk(KERN_INFO " ip_ttl_dec1(1): %d, ip_ttl_zero_discard_en1(1): %d\n", act->ip_ttl_dec1, act->ip_ttl_zero_discard_en1);
	printk(KERN_INFO " ip_idx_vld(1): %d, ip_type(1): %d, ip_idx(10): %d\n", act->ip_idx_vld, act->ip_type, act->ip_idx);
	printk(KERN_INFO " mac_da_idx_vld(1): %d, mac_da_idx(10): %d\n", act->mac_da_idx_vld, act->mac_da_idx);

	printk(KERN_INFO "<Group 22, Pol2>\n");
	printk(KERN_INFO " pol23_vld(1): %d\n", act->pol23_vld);
	printk(KERN_INFO " pol3_en(1): %d, pol3_id(6): %d\n", act->pol3_en, act->pol3_id);
	printk(KERN_INFO " pol2_msb_en(1): %d, pol2_id_msb(6): %d\n", act->pol2_msb_en, act->pol2_id_msb);
}
static void dump_HS_ACTGRP_FLOW_RTK_MC(hash_action_flow_rtk_mc_t *act)
{
	printk(KERN_INFO "<Group 1, L4 DP>\n");
	printk(KERN_INFO " l4_dp_vld: %d, l4_dp(16): 0x%04x\n", act->l4_dp_vld, act->l4_dp);

	printk(KERN_INFO "<Group 7, PPPoE fields>\n");
	printk(KERN_INFO " pppoe_vld: %d, pppoe_set: %d, pppoe_session_id(16): 0x%04x\n",
		act->pppoe_vld, act->pppoe_set, act->pppoe_session_id);

	printk(KERN_INFO "<Group 9, VLAN>\n");
	printk(KERN_INFO " vlan_vld: %d, vlan_cnt(2): %d, inner_vlan_cmd(2): %d\n", act->vlan_vld, act->vlan_cnt, act->inner_vlan_cmd);
	printk(KERN_INFO " top_tpid_enc(3): %d, top_tpid_sel(2): %d, top_vid(12): 0x%03x, top_dei: %d, top_dei_sel(2): %d\n",
		act->top_tpid_enc, act->top_tpid_sel, act->top_vid, act->top_dei, act->top_dei_sel);
	printk(KERN_INFO " inner_tpid_enc(3): %d, inner_tpid_sel(2): %d, inner_vid(12): 0x%03x, inner_dei: %d, inner_dei_sel(2): %d\n",
		act->inner_tpid_enc, act->inner_tpid_sel, act->inner_vid, act->inner_dei, act->inner_dei_sel);

	printk(KERN_INFO "<Group 16, Policer>\n");
	printk(KERN_INFO " pol_vld: %d, qos_premark: %d\n", act->pol_vld, act->qos_premark);
	printk(KERN_INFO " pol_all_bypass: %d, pol_en(2): %d\n", act->pol_all_bypass, act->pol_en);
	printk(KERN_INFO " pol_base(9): 0x%03x, pol_sel(3): %d\n", act->pol_base, act->pol_sel);
	printk(KERN_INFO " pol_table_sel: %d, pol_grp_en: %d, pol_grp_id(3): %d\n", act->pol_table_sel, act->pol_grp_en, act->pol_grp_id);

	printk(KERN_INFO "<Group 17, QoS>\n");
	printk(KERN_INFO " ip_ecn_vld: %d, ip_ecn_en: %d, ip_tos_vld: %d, ip_tos_6: %d\n",
		act->ip_ecn_vld, act->ip_ecn_en, act->ip_tos_vld, act->ip_tos_6);
	printk(KERN_INFO " ip_dscp_markdown_vld: %d, ip_dscp_marked_down(6): 0x%02x, ip_dscp_markdown_en: %d\n",
		act->ip_dscp_markdown_vld, act->ip_dscp_marked_down, act->ip_dscp_markdown_en);
	printk(KERN_INFO " ip_dscp_update_en(2): %d, ip_dscp_sel(3): %d, ip_dscp(6): 0x%02x, dscp_table_sel: %d\n",
		act->ip_dscp_update_en, act->ip_dscp_sel, act->ip_dscp, act->dscp_table_sel);
	printk(KERN_INFO " inner_802_1p_sel(3): %d, inner_802_1p(3): %d\n", act->inner_802_1p_sel, act->inner_802_1p);
	printk(KERN_INFO " top_802_1p_sel(3): %d, top_802_1p(3): %d, qos_802_1p_table_sel: %d\n",
		act->top_802_1p_sel, act->top_802_1p, act->qos_802_1p_table_sel);
	printk(KERN_INFO " cos_table_sel: %d, cos_sel(2): %d, cos(3): %d\n", act->cos_table_sel, act->cos_sel, act->cos);

	printk(KERN_INFO "<Group 18, Dest Port ID>\n");
	printk(KERN_INFO " mrr_vld: %d, mrr_en: %d, no_drop_vld: %d, no_drop: %d\n",
		act->mrr_vld, act->mrr_en, act->no_drop_vld, act->no_drop);
	printk(KERN_INFO " dpid_vld: %d, dpid_pri: %d, permit: %d, deepq: %d, mcgid/ldpid(10): 0x%03x, mc: %d\n",
		act->dpid_vld, act->dpid_pri, act->permit, act->deepq, act->mcgid, act->mc);

	printk(KERN_INFO "<Group 19, Control>\n");
	printk(KERN_INFO " chk_msk_ptr(6): 0x%02x, cache_ctrl(2): %d\n", act->chk_msk_ptr, act->cache_ctrl);
	printk(KERN_INFO " pop_l3_vld: %d, pop_l3_chk_ecn_en: %d, pop_l3_en: %d\n", act->pop_l3_vld, act->pop_l3_chk_ecn_en, act->pop_l3_en);
	printk(KERN_INFO " keep_ts_vld: %d, keep_ts_en: %d\n", act->keep_ts_vld, act->keep_ts_en);
	printk(KERN_INFO " keep_orig_pkt_vld: %d, keep_orig_pkt: %d\n", act->keep_orig_pkt_vld, act->keep_orig_pkt);
	printk(KERN_INFO " t2_ctrl_vld: %d, t2_ctrl: 0x%x\n", act->t2_ctrl_vld, act->t2_ctrl);
	printk(KERN_INFO " t3_ctrl_vld: %d, t3_ctrl: 0x%x\n", act->t3_ctrl_vld, act->t3_ctrl);
	printk(KERN_INFO " t4_ctrl_vld: %d, t4_ctrl: 0x%x\n", act->t4_ctrl_vld, act->t4_ctrl);
	printk(KERN_INFO " t5_ctrl_vld: %d, t5_ctrl: 0x%x\n", act->t5_ctrl_vld, act->t5_ctrl);
	printk(KERN_INFO " stage2_ctrl_vld: %d, stage2_ctrl: %d\n", act->stage2_ctrl_vld, act->stage2_ctrl);
	printk(KERN_INFO " stage3_ctrl_vld: %d, stage3_ctrl: %d\n", act->stage3_ctrl_vld, act->stage3_ctrl);

	printk(KERN_INFO "<Group 20, Indexed>\n");
	printk(KERN_INFO " mdata_byte_vld(1): %d, mdata_byte_idx(1): %d, mdata_byte(8): 0x%02x\n", act->mdata_byte_vld, act->mdata_byte_idx, act->mdata_byte);
	printk(KERN_INFO " l3_egress_if_vld1(1): %d\n", act->l3_egress_if_vld1);
	printk(KERN_INFO " smac_trans(1): %d\n", act->smac_trans);
	printk(KERN_INFO " ip_ttl_dec1(1): %d, ip_ttl_zero_discard_en1(1): %d\n", act->ip_ttl_dec1, act->ip_ttl_zero_discard_en1);
	printk(KERN_INFO " ip_idx_vld(1): %d, ip_type(1): %d, ip_idx(10): %d\n", act->ip_idx_vld, act->ip_type, act->ip_idx);
	printk(KERN_INFO " mac_da_idx_vld(1): %d, mac_da_idx(10): %d\n", act->mac_da_idx_vld, act->mac_da_idx);

	printk(KERN_INFO "<Group 22, Pol2>\n");
	printk(KERN_INFO " pol23_vld(1): %d\n", act->pol23_vld);
	printk(KERN_INFO " pol3_en(1): %d, pol3_id(6): %d\n", act->pol3_en, act->pol3_id);
	printk(KERN_INFO " pol2_msb_en(1): %d, pol2_id_msb(6): %d\n", act->pol2_msb_en, act->pol2_id_msb);
}
#endif

int hash_action_dump(unsigned int act_grp_mask, hash_hash_action_entry_t *act_entry)
{
	if (act_entry == NULL) return AAL_E_NULLPTR;

	switch (act_grp_mask) {
	case ACTGRP_L3_DEFAULT:
		dump_HS_ACTGRP_L3_DEFAULT((hash_default_action_t *) act_entry);
		break;

	case ACTGRP_NAT_TO_LAN:
		dump_HS_ACTGRP_NAT_TO_LAN((hash_action_to_lan_t *) act_entry);
		break;

	case ACTGRP_NAT_TO_WAN:
		dump_HS_ACTGRP_NAT_TO_WAN((hash_action_to_wan_t *) act_entry);
		break;

	case ACTGRP_MULTICAST:
		dump_HS_ACTGRP_MULTICAST((hash_action_multicast_t *) act_entry);
		break;

	case ACTGRP_CLS_HASH_FIB:
		dump_HS_ACTGRP_TYPE1_CLS_FIB((hash_action_type1_cls_hash_t *) act_entry);
		break;

	case ACTGRP_FLOW:
		dump_HS_ACTGRP_FLOW((hash_action_flow_t *) act_entry);
		break;

#if defined(CONFIG_LUNA_G3_SERIES)
	case ACTGRP_FLOW_RTK_DS:
		dump_HS_ACTGRP_FLOW_RTK_DS((hash_action_flow_rtk_ds_t *) act_entry);
		break;

	case ACTGRP_FLOW_RTK_US:
		dump_HS_ACTGRP_FLOW_RTK_US((hash_action_flow_rtk_us_t *) act_entry);
		break;

	case ACTGRP_FLOW_RTK_MC:
		dump_HS_ACTGRP_FLOW_RTK_MC((hash_action_flow_rtk_mc_t *) act_entry);
		break;

/* crc16_mask[HB_SIZE][HT_SIZE]
 *   Mask with hash's crc16 for the bucket index.
 *   HB_SIZE: 0~3 for bucket size 4, 8, 16, 32.
 *   HT_SIZE: 0~7 for hash table size 512, 1k, 2k, 4k, 8k, 16k, 32k, 64k.
 *   See the MAS file, figure 19: Configuration of Main Hash Table Sizes, for detail.
 */
static u16 crc16_mask[4][8] = {
	{ 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc },
	{ 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8 },
	{ 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0 },
	{ 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0 }};


#endif

	default:
		printk(KERN_INFO "Unknown action group mask 0x%08x\n", (unsigned int) act_grp_mask);
		break;
	}

	return AAL_E_OK;
}

static int hash_action_group(ca_uint32_t profile_id, aal_hash_tuple_type_e hash_type, aal_hash_action_t *hash_act, ca_uint32_t *act_grp)
{
	ca_uint32_t mask=0;

#if defined(CONFIG_LUNA_G3_SERIES)
	ca_boolean allowSA_act=TRUE;
	ca_boolean allowDA_act=TRUE;
	ca_boolean allowCMN_act=TRUE;

	if((profile_id==l3fe_main_hash_profile_mc)){
		// profiles(2) use reduced flow action
		allowSA_act = TRUE;
		allowDA_act = TRUE;
		allowCMN_act = TRUE;
	}
	else if((profile_id==l3fe_main_hash_profile_wan)){
		// profiles(0,3) use reduced flow action
		allowSA_act = FALSE;
		allowDA_act = TRUE;
		allowCMN_act = FALSE;
	}
	else if((profile_id==l3fe_main_hash_profile_lan)
		|| (profile_id==l3fe_main_hash_profile_2tuple_bridge) ){
		// profiles(1) use reduced flow action
		allowSA_act = TRUE;
		allowDA_act = FALSE;
		allowCMN_act = FALSE;
	}
	else{
		DBG(printk("%s: Profile %d tuple type %d is not supported!\n", __func__, profile_id, hash_type));
		goto ERROR;
	}


#endif

	if (hash_act == NULL || act_grp == NULL)
		return AAL_E_NULLPTR;

	if(hash_act->ip_idx_vld || hash_act->mac_da_idx_vld || hash_act->l3_egress_if_vld1 || hash_act->mdata_byte_vld || hash_act->smac_trans)
		mask |= ACTION_GROUP_20;

	if (hash_act->t2_ctrl_vld || hash_act->t3_ctrl_vld || hash_act->t4_ctrl_vld || hash_act->t5_ctrl_vld ||
		hash_act->stage2_ctrl_vld || hash_act->stage3_ctrl_vld || hash_act->keep_orig_pkt_vld ||
		hash_act->keep_ts_vld || hash_act->pop_l3_vld)
		mask |= ACTION_GROUP_19;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowCMN_act)
#endif
	if (hash_act->dpid_vld || hash_act->no_drop_vld || hash_act->mrr_vld)
		mask |= ACTION_GROUP_18;

	if (hash_act->cos_sel || hash_act->top_802_1p_sel || hash_act->inner_802_1p_sel || hash_act->ip_dscp_update_en ||
		hash_act->ip_dscp_markdown_vld || hash_act->ip_tos_vld || hash_act->ip_ecn_vld)
		mask |= ACTION_GROUP_17;

	if (hash_act->pol_vld)
		mask |= ACTION_GROUP_16;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowCMN_act)
#endif
	if (hash_act->mdata_w_vld_3)
		mask |= ACTION_GROUP_15;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowCMN_act)
#endif
	if (hash_act->mdata_w_vld_2)
		mask |= ACTION_GROUP_14;

	if (hash_act->mdata_w_vld_1)
		mask |= ACTION_GROUP_13;

	if (hash_act->mdata_w_vld_0)
		mask |= ACTION_GROUP_12;

	if (hash_act->mac_da_vld)
		mask |= ACTION_GROUP_11;

	if (hash_act->mac_sa_vld)
		mask |= ACTION_GROUP_10;

	if (hash_act->vlan_vld)
		mask |= ACTION_GROUP_09;

	if (hash_act->l3_egress_if_vld || hash_act->l2_format_vld || hash_act->ip_mtu_enc_vld || hash_act->six_rd_ipda_ctrl_vld ||
		hash_act->ip_ttl_vld)
		mask |= ACTION_GROUP_08;

	if (hash_act->pppoe_vld)
		mask |= ACTION_GROUP_07;

	if (hash_act->ip_sa_h_vld)
		mask |= ACTION_GROUP_06;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowSA_act)
#endif
	if (hash_act->ip_sa_l_vld)
		mask |= ACTION_GROUP_05;

	if (hash_act->ip_da_h_vld)
		mask |= ACTION_GROUP_04;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowDA_act)
#endif
	if (hash_act->ip_da_l_vld)
		mask |= ACTION_GROUP_03;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowSA_act)
#endif
	if (hash_act->l4_sp_vld)
		mask |= ACTION_GROUP_02;

#if defined(CONFIG_LUNA_G3_SERIES)
	if(allowDA_act)
#endif
	if (hash_act->l4_dp_vld)
		mask |= ACTION_GROUP_01;


	/* check action group */
	switch (profile_id) {
		case HASH_PROFILE_0:
			if (hash_type == TUPLE_TYPE_FWD && (mask == (mask & hash_profile_0.type0_act_grp_mask_hit)))
				*act_grp = hash_profile_0.type0_act_grp_mask_hit;
			else if (hash_type == TUPLE_TYPE_QOS && (mask == (mask & hash_profile_0.type1_act_grp_mask_hit)))
				*act_grp = hash_profile_0.type1_act_grp_mask_hit;
			else
				goto ERROR;
			break;
		case HASH_PROFILE_1:
			if (hash_type == TUPLE_TYPE_FWD && (mask == (mask & hash_profile_1.type0_act_grp_mask_hit)))
				*act_grp = hash_profile_1.type0_act_grp_mask_hit;
			else if (hash_type == TUPLE_TYPE_QOS && (mask == (mask & hash_profile_1.type1_act_grp_mask_hit)))
				*act_grp = hash_profile_1.type1_act_grp_mask_hit;
			else
				goto ERROR;
			break;
		case HASH_PROFILE_2:
			if (hash_type == TUPLE_TYPE_FWD && (mask == (mask & hash_profile_2.type0_act_grp_mask_hit)))
				*act_grp = hash_profile_2.type0_act_grp_mask_hit;
			else if (hash_type == TUPLE_TYPE_QOS && (mask == (mask & hash_profile_2.type1_act_grp_mask_hit)))
				*act_grp = hash_profile_2.type1_act_grp_mask_hit;
			else
				goto ERROR;
			break;
#if defined(CONFIG_LUNA_G3_SERIES)
		case HASH_PROFILE_3:
			if (hash_type == TUPLE_TYPE_FWD && (mask == (mask & hash_profile_3.type0_act_grp_mask_hit)))
				*act_grp = hash_profile_3.type0_act_grp_mask_hit;
			else if (hash_type == TUPLE_TYPE_QOS && (mask == (mask & hash_profile_3.type1_act_grp_mask_hit)))
				*act_grp = hash_profile_3.type1_act_grp_mask_hit;
			else
				goto ERROR;
			break;
		case HASH_PROFILE_4:
			if (hash_type == TUPLE_TYPE_FWD && (mask == (mask & hash_profile_4.type0_act_grp_mask_hit)))
				*act_grp = hash_profile_4.type0_act_grp_mask_hit;
			else if (hash_type == TUPLE_TYPE_QOS && (mask == (mask & hash_profile_4.type1_act_grp_mask_hit)))
				*act_grp = hash_profile_4.type1_act_grp_mask_hit;
			else
				goto ERROR;
			break;
#endif
		default:
			goto ERROR;

	}

	return AAL_E_OK;

ERROR:
	DBG(printk("%s: Profile %d tuple type %d cannot find action group (0x%08x)\n", __func__, profile_id, hash_type, mask));
	return CA_E_NOT_SUPPORT;
}

static void convert_act_nat_to_wan(aal_hash_action_t *act1, hash_action_to_wan_t *act2)
{

	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 3 */
	act2->ip_da_l_vld = act1->ip_da_l_vld;
	act2->ip_da_0 = act1->ip_da_0;

	/* group 5 */
	act2->ip_sa_l_vld = act1->ip_sa_l_vld;
	act2->ip_sa_0 = act1->ip_sa_0;

	/* group 6 */
	act2->ip_sa_h_vld = act1-> ip_sa_h_vld;
	act2->ip_sa_1 = act1-> ip_sa_1;
	act2->ip_sa_2 = act1-> ip_sa_2;
	act2->ip_sa_3 = act1-> ip_sa_3;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;
#if 0
	/* group 21 */
	act2->chk_hash_vld = act1->chk_hash_vld;
	act2->chk_hash_val = act1->chk_hash_val;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;
#endif
}

static void convert_act_nat_to_wan_1(aal_hash_action_t *act1, hash_action_to_wan_1_t *act2)
{

	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mac_da_idx = act1->mac_da_idx ;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld ;
	act2->ip_idx = act1->ip_idx ;
	act2->ip_type = act1->ip_type;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->smac_trans = act1->smac_trans;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->mdata_byte = act1->mdata_byte;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte_vld = act1->mdata_byte_vld;

}

static void convert_act_nat_to_lan(aal_hash_action_t *act1, hash_action_to_lan_t *act2)
{

	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 3 */
	act2->ip_da_l_vld = act1->ip_da_l_vld;
	act2->ip_da_0 = act1->ip_da_0;

	/* group 4 */
	act2->ip_da_h_vld = act1-> ip_da_h_vld;
	act2->ip_da_1 = act1-> ip_da_1;
	act2->ip_da_2 = act1-> ip_da_2;
	act2->ip_da_3 = act1-> ip_da_3;

	/* group 5 */
	act2->ip_sa_l_vld = act1->ip_sa_l_vld;
	act2->ip_sa_0 = act1->ip_sa_0;

	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

#if 0
	/* group 21 */
	act2->chk_hash_vld = act1->chk_hash_vld;
	act2->chk_hash_val = act1->chk_hash_val;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;
#endif
}

static void convert_act_nat_to_lan_1(aal_hash_action_t *act1, hash_action_to_lan_1_t *act2)
{
	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mac_da_idx = act1->mac_da_idx ;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld ;
	act2->ip_idx = act1->ip_idx ;
	act2->ip_type = act1->ip_type;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->smac_trans = act1->smac_trans;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->mdata_byte = act1->mdata_byte;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte_vld = act1->mdata_byte_vld;
}

static void convert_act_type1_cls_hash(aal_hash_action_t *act1, hash_action_type1_cls_hash_t *act2)
{
	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 11 */
	act2->mac_da_vld = act1->mac_da_vld;
	act2->mac_da_0 = act1->mac_da_0;
	act2->mac_da_1 = act1->mac_da_1;
	act2->mac_da_2 = act1->mac_da_2;
	act2->mac_da_3 = act1->mac_da_3;
	act2->mac_da_4 = act1->mac_da_4;
	act2->mac_da_5 = act1->mac_da_5;

	/* group 12 */
	act2->mdata_w_vld_0 = act1->mdata_w_vld_0;
	act2->mdata_w_0 = act1->mdata_w_0;

	/* group 13 */
	act2->mdata_w_vld_1 = act1->mdata_w_vld_1;
	act2->mdata_w_1 = act1->mdata_w_1;

	/* group 14 */
	act2->mdata_w_vld_2 = act1->mdata_w_vld_2;
	act2->mdata_w_2 = act1->mdata_w_2;

	/* group 15 */
	act2->mdata_w_vld_3 = act1->mdata_w_vld_3;
	act2->mdata_w_3 = act1->mdata_w_3;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

}

static void convert_act_flow_short(aal_hash_action_t *act1, hash_action_flow_short_t *act2)
{
	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mac_da_idx = act1->mac_da_idx ;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld ;
	act2->ip_idx = act1->ip_idx ;
	act2->ip_type = act1->ip_type;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->smac_trans = act1->smac_trans;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->mdata_byte = act1->mdata_byte;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte_vld = act1->mdata_byte_vld;
}

static void convert_act_flow(aal_hash_action_t *act1, hash_action_flow_t *act2)
{
	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 3 */
	act2->ip_da_l_vld = act1->ip_da_l_vld;
	act2->ip_da_0 = act1->ip_da_0;

	/* group 5 */
	act2->ip_sa_l_vld = act1->ip_sa_l_vld;
	act2->ip_sa_0 = act1->ip_sa_0;

	/* group 7 */
	act2->pppoe_vld = act1->pppoe_vld;
	act2->pppoe_set = act1->pppoe_set;
	act2->pppoe_session_id = act1->pppoe_session_id;

	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 10 */
	act2->mac_sa_vld = act1->mac_sa_vld;
	act2->mac_sa_0 = act1->mac_sa_0;
	act2->mac_sa_1 = act1->mac_sa_1;
	act2->mac_sa_2 = act1->mac_sa_2;
	act2->mac_sa_3 = act1->mac_sa_3;
	act2->mac_sa_4 = act1->mac_sa_4;
	act2->mac_sa_5 = act1->mac_sa_5;

	/* group 11 */
	act2->mac_da_vld = act1->mac_da_vld;
	act2->mac_da_0 = act1->mac_da_0;
	act2->mac_da_1 = act1->mac_da_1;
	act2->mac_da_2 = act1->mac_da_2;
	act2->mac_da_3 = act1->mac_da_3;
	act2->mac_da_4 = act1->mac_da_4;
	act2->mac_da_5 = act1->mac_da_5;

	/* group 12 */
	act2->mdata_w_vld_0 = act1->mdata_w_vld_0;
	act2->mdata_w_0 = act1->mdata_w_0;

	/* group 13 */
	act2->mdata_w_vld_1 = act1->mdata_w_vld_1;
	act2->mdata_w_1 = act1->mdata_w_1;

	/* group 14 */
	act2->mdata_w_vld_2 = act1->mdata_w_vld_2;
	act2->mdata_w_2 = act1->mdata_w_2;

	/* group 15 */
	act2->mdata_w_vld_3 = act1->mdata_w_vld_3;
	act2->mdata_w_3 = act1->mdata_w_3;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;

}
#if defined(CONFIG_LUNA_G3_SERIES)

static void convert_act_flow_rtk_ds(aal_hash_action_t *act1, hash_action_flow_rtk_ds_t *act2)
{
	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 13 */
	act2->mdata_w_vld_1 = act1->mdata_w_vld_1;
	act2->mdata_w_1 = act1->mdata_w_1;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mdata_byte_vld = act1->mdata_byte_vld;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte = act1->mdata_byte;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->smac_trans = act1->smac_trans;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_type = act1->ip_type;
	act2->ip_idx = act1->ip_idx;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld;
	act2->mac_da_idx = act1->mac_da_idx;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;
}

static void convert_act_flow_rtk_us(aal_hash_action_t *act1, hash_action_flow_rtk_us_t *act2)
{
	/* group 2 */
	act2->l4_sp = act1->l4_sp;
	act2->l4_sp_vld = act1->l4_sp_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 13 */
	act2->mdata_w_vld_1 = act1->mdata_w_vld_1;
	act2->mdata_w_1 = act1->mdata_w_1;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mdata_byte_vld = act1->mdata_byte_vld;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte = act1->mdata_byte;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->smac_trans = act1->smac_trans;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_type = act1->ip_type;
	act2->ip_idx = act1->ip_idx;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld;
	act2->mac_da_idx = act1->mac_da_idx;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;
}
static void convert_act_flow_rtk_mc(aal_hash_action_t *act1, hash_action_flow_rtk_mc_t *act2)
{
	/* group 1 */
	act2->l4_dp = act1->l4_dp;
	act2->l4_dp_vld = act1->l4_dp_vld;

	/* group 7 */
	act2->pppoe_vld = act1->pppoe_vld;
	act2->pppoe_set = act1->pppoe_set;
	act2->pppoe_session_id = act1->pppoe_session_id;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 16 */
	act2->pol_vld = act1->pol_vld;
	act2->qos_premark = act1->qos_premark;
	act2->pol_all_bypass = act1->pol_all_bypass;
	act2->pol_en = act1->pol_en;
	act2->pol_base = act1->pol_base;
	act2->pol_sel = act1->pol_sel;
	act2->pol_table_sel = act1->pol_table_sel;
	act2->pol_grp_en = act1->pol_grp_en;
	act2->pol_grp_id = act1->pol_grp_id;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

	/* group 20 */
	act2->mdata_byte_vld = act1->mdata_byte_vld;
	act2->mdata_byte_idx = act1->mdata_byte_idx;
	act2->mdata_byte = act1->mdata_byte;
	act2->l3_egress_if_vld1 = act1->l3_egress_if_vld1;
	act2->smac_trans = act1->smac_trans;
	act2->ip_ttl_dec1 = act1->ip_ttl_dec1;
	act2->ip_ttl_zero_discard_en1 = act1->ip_ttl_zero_discard_en1;
	act2->ip_idx_vld = act1->ip_idx_vld;
	act2->ip_type = act1->ip_type;
	act2->ip_idx = act1->ip_idx;
	act2->mac_da_idx_vld = act1->mac_da_idx_vld;
	act2->mac_da_idx = act1->mac_da_idx;

	/* group 22 */
	act2->pol23_vld = act1->pol23_vld;
	act2->pol3_en = act1->pol3_en;
	act2->pol3_id = act1->pol3_id;
	act2->pol2_msb_en = act1->pol2_msb_en;
	act2->pol2_id_msb = act1->pol2_id_msb;
}

#endif

static void convert_act_multicast(aal_hash_action_t *act1, hash_action_multicast_t *act2)
{
	/* group 7 */
	act2->pppoe_vld = act1->pppoe_vld;
	act2->pppoe_set = act1->pppoe_set;
	act2->pppoe_session_id = act1->pppoe_session_id;

	/* group 8 */
	act2->ip_ttl_vld = act1->ip_ttl_vld;
	act2->ip_ttl_zero_discard_en = act1->ip_ttl_zero_discard_en;
	act2->ip_ttl_cmd = act1->ip_ttl_cmd;
	act2->ip_ttl = act1->ip_ttl;
	act2->six_rd_ipda_ctrl_vld = act1->six_rd_ipda_ctrl_vld;
	act2->six_rd_ipda_from_v6 = act1->six_rd_ipda_from_v6;
	act2->ip_mtu_enc_vld = act1->ip_mtu_enc_vld;
	act2->ip_mtu_enc = act1->ip_mtu_enc;
	act2->l2_format_vld = act1->l2_format_vld;
	act2->l2_format = act1->l2_format;
	act2->l3_egress_if_vld = act1->l3_egress_if_vld;

	/* group 9 */
	act2->inner_dei_sel = act1->inner_dei_sel;
	act2->inner_dei = act1->inner_dei;
	act2->inner_vid = act1->inner_vid;
	act2->inner_tpid_sel = act1->inner_tpid_sel;
	act2->inner_tpid_enc = act1->inner_tpid_enc;
	act2->top_dei_sel = act1->top_dei_sel;
	act2->top_dei = act1->top_dei;
	act2->top_vid = act1->top_vid;
	act2->top_tpid_sel = act1->top_tpid_sel;
	act2->top_tpid_enc = act1->top_tpid_enc;
	act2->inner_vlan_cmd = act1->inner_vlan_cmd;
	act2->vlan_cnt = act1->vlan_cnt;
	act2->vlan_vld = act1->vlan_vld;

	/* group 10 */
	act2->mac_sa_vld = act1->mac_sa_vld;
	act2->mac_sa_0 = act1->mac_sa_0;
	act2->mac_sa_1 = act1->mac_sa_1;
	act2->mac_sa_2 = act1->mac_sa_2;
	act2->mac_sa_3 = act1->mac_sa_3;
	act2->mac_sa_4 = act1->mac_sa_4;
	act2->mac_sa_5 = act1->mac_sa_5;

	/* group 11 */
	act2->mac_da_vld = act1->mac_da_vld;
	act2->mac_da_0 = act1->mac_da_0;
	act2->mac_da_1 = act1->mac_da_1;
	act2->mac_da_2 = act1->mac_da_2;
	act2->mac_da_3 = act1->mac_da_3;
	act2->mac_da_4 = act1->mac_da_4;
	act2->mac_da_5 = act1->mac_da_5;

	/* group 17 */
	act2->ip_ecn_vld = act1->ip_ecn_vld;
	act2->ip_ecn_en = act1->ip_ecn_en;
	act2->ip_tos_vld = act1->ip_tos_vld;
	act2->ip_tos_6 = act1->ip_tos_6;
	act2->ip_dscp_markdown_vld = act1->ip_dscp_markdown_vld;
	act2->ip_dscp_marked_down = act1->ip_dscp_marked_down;
	act2->ip_dscp_markdown_en = act1->ip_dscp_markdown_en;
	act2->ip_dscp_update_en = act1->ip_dscp_update_en;
	act2->ip_dscp_sel = act1->ip_dscp_sel;
	act2->ip_dscp = act1->ip_dscp;
	act2->dscp_table_sel = act1->dscp_table_sel;
	act2->inner_802_1p_sel = act1->inner_802_1p_sel;
	act2->inner_802_1p = act1->inner_802_1p;
	act2->top_802_1p_sel = act1->top_802_1p_sel;
	act2->top_802_1p = act1->top_802_1p;
	act2->qos_802_1p_table_sel = act1->qos_802_1p_table_sel;
	act2->cos_table_sel = act1->cos_table_sel;
	act2->cos_sel = act1->cos_sel;
	act2->cos = act1->cos;

	/* group 18 */
	act2->mrr_vld = act1->mrr_vld;
	act2->mrr_en = act1->mrr_en;
	act2->no_drop_vld = act1->no_drop_vld;
	act2->no_drop = act1->no_drop;
	act2->dpid_vld = act1->dpid_vld;
	act2->dpid_pri = act1->dpid_pri;
	act2->permit = act1->permit;
	act2->deepq = act1->deepq;
	act2->mcgid = act1->mcgid;
	act2->mc = act1->mc;

	/* group 19 */
	act2->chk_msk_ptr = act1->chk_msk_ptr;
	act2->cache_ctrl = act1->cache_ctrl;
	act2->pop_l3_vld = act1->pop_l3_vld;
	act2->pop_l3_chk_ecn_en = act1->pop_l3_chk_ecn_en;
	act2->pop_l3_en = act1->pop_l3_en;
	act2->keep_ts_vld = act1->keep_ts_vld;
	act2->keep_ts_en = act1->keep_ts_en;
	act2->keep_orig_pkt_vld = act1->keep_orig_pkt_vld;
	act2->keep_orig_pkt = act1->keep_orig_pkt;
	act2->stage3_ctrl_vld = act1->stage3_ctrl_vld;
	act2->stage3_ctrl = act1->stage3_ctrl;
	act2->stage2_ctrl_vld = act1->stage2_ctrl_vld;
	act2->stage2_ctrl = act1->stage2_ctrl;
	act2->t5_ctrl_vld = act1->t5_ctrl_vld;
	act2->t5_ctrl = act1->t5_ctrl;
	act2->t4_ctrl_vld = act1->t4_ctrl_vld;
	act2->t4_ctrl = act1->t4_ctrl;
	act2->t3_ctrl_vld = act1->t3_ctrl_vld;
	act2->t3_ctrl = act1->t3_ctrl;
	act2->t2_ctrl_vld = act1->t2_ctrl_vld;
	act2->t2_ctrl = act1->t2_ctrl;

}


static int convert_to_hash_action_entry(ca_uint32_t act_grp_mask, aal_hash_action_t *act, hash_hash_action_entry_t *hash_action_entry)
{
	if (act == NULL || hash_action_entry == NULL)
		return AAL_E_NULLPTR;

	if (act_grp_mask == ACTGRP_NAT_TO_WAN) {
		hash_action_to_wan_t *ptr = (hash_action_to_wan_t *)hash_action_entry;
		convert_act_nat_to_wan(act, ptr);
	} else if (act_grp_mask == ACTGRP_NAT_TO_WAN_1) {
		hash_action_to_wan_1_t *ptr = (hash_action_to_wan_1_t *)hash_action_entry;
		convert_act_nat_to_wan_1(act, ptr);
	} else if (act_grp_mask == ACTGRP_NAT_TO_LAN) {
		hash_action_to_lan_t *ptr = (hash_action_to_lan_t *)hash_action_entry;
		convert_act_nat_to_lan(act, ptr);
	} else if (act_grp_mask == ACTGRP_NAT_TO_LAN_1) {
		hash_action_to_lan_1_t *ptr = (hash_action_to_lan_1_t *)hash_action_entry;
		convert_act_nat_to_lan_1(act, ptr);
	} else if (act_grp_mask == ACTGRP_MULTICAST) {
		hash_action_multicast_t *ptr = (hash_action_multicast_t *)hash_action_entry;
		convert_act_multicast(act, ptr);
	} else if (act_grp_mask == ACTGRP_CLS_HASH_FIB) {
		hash_action_type1_cls_hash_t *ptr = (hash_action_type1_cls_hash_t *)hash_action_entry;
		convert_act_type1_cls_hash(act, ptr);
	} else if (act_grp_mask == ACTGRP_FLOW) {
		hash_action_flow_t *ptr = (hash_action_flow_t *)hash_action_entry;
		convert_act_flow(act, ptr);
	} else if (act_grp_mask == ACTGRP_FLOW_SHORT) {
		hash_action_flow_short_t *ptr = (hash_action_flow_short_t *)hash_action_entry;
		convert_act_flow_short(act, ptr);
#if defined(CONFIG_LUNA_G3_SERIES)
	} else if (act_grp_mask == ACTGRP_FLOW_RTK_DS) {
		hash_action_flow_rtk_ds_t *ptr = (hash_action_flow_rtk_ds_t *)hash_action_entry;
		convert_act_flow_rtk_ds(act, ptr);
	} else if (act_grp_mask == ACTGRP_FLOW_RTK_US) {
		hash_action_flow_rtk_us_t *ptr = (hash_action_flow_rtk_us_t *)hash_action_entry;
		convert_act_flow_rtk_us(act, ptr);
	} else if (act_grp_mask == ACTGRP_FLOW_RTK_MC) {
		hash_action_flow_rtk_mc_t *ptr = (hash_action_flow_rtk_mc_t *)hash_action_entry;
		convert_act_flow_rtk_mc(act, ptr);
#endif
	} else {
		return CA_E_NOT_SUPPORT;
	}
	return AAL_E_OK;
}

/* Set entry to HW from SW cached entry. */
static int _hash_hw_entry_set(aal_table_t *table, unsigned int idx)
{
	unsigned int *ptr;
	int i, j;
	unsigned int val = 0;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL ||
		table->table_id == AAL_TABLE_HASH_OVERFLOW_TBL)
		;
	else
		return AAL_E_TBLNOTEXIST;

	for (i = 0; i < table->data_reg_num; i++)
		CA_NE_REG_WRITE(ptr[i], table->access_reg + ((table->data_reg_num - i) << 2));

	j = i; /* keep ptr index.*/

	val |= 1 << 30; /* op is write */
	val |= 1 << 31; /* status */
	val |= idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL) {
		/* mask needs to write upper 128-bit */
		for (i = 0; i < table->data_reg_num; i++, j++)
			CA_NE_REG_WRITE(ptr[j], table->access_reg + ((table->data_reg_num - i) << 2));
		val = 0;
		val |= 1 << 30; /* op is write */
		val |= 1 << 31; /* status */
		val |= 1 << 6; /* addr[6] = 1 for upper 128-bit */
		val |= idx;
		CA_NE_REG_WRITE(val, table->access_reg);

		i = 0;
		do {
			val = CA_NE_REG_READ(table->access_reg);
		} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
		if (i >= TABLE_TRY_TIMEOUT)
			return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	}

	return AAL_E_OK;
}

/* Get entry from HW directly; some table entry requires to read 2 or more times. */
static int __hash_hw_entry_get(aal_table_t *table, unsigned int idx, void *data)
{
	int i, j;
	unsigned int val = 0;
	unsigned int *ptr = (unsigned int *) data;

	if (data == NULL) return AAL_E_NULLPTR;
	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL ||
		table->table_id == AAL_TABLE_HASH_OVERFLOW_TBL ||
		table->table_id == AAL_TABLE_CACHE_HASH_TBL ||
		table->table_id == AAL_TABLE_HASH_AGE_TBL)
		;
	else
		return AAL_E_TBLNOTEXIST;

	val |= 0 << 30; /* op is read */
	val |= 1 << 31;
	val |= idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	for (i = 0; (i < table->data_reg_num) && (i < (table->entry_size + 4 - 1) / 4); i++)
		ptr[i] = CA_NE_REG_READ(table->access_reg + ((table->data_reg_num - i) << 2));

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL) {
		j = i; /* keep ptr index.*/
		val = 0;
		val |= 0 << 30; /* op is read */
		val |= 1 << 31;
		val |= 1 << 6; /* addr[6] = 1 for upper 128-bit */
		val |= idx;
		CA_NE_REG_WRITE(val, table->access_reg);

		i = 0;
		do {
			val = CA_NE_REG_READ(table->access_reg);
		} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
		if (i >= TABLE_TRY_TIMEOUT)
			return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

		for (i = 0; (i < table->data_reg_num) && ((i + 4) < (table->entry_size + 4 - 1) / 4); i++) {
			ptr[j++] = CA_NE_REG_READ(table->access_reg + ((table->data_reg_num - i) << 2));
		}
	}

	return AAL_E_OK;
}

static int __hash_hw_entry_clear(aal_table_t *table, unsigned int idx)
{
	unsigned int *ptr;
	int i;
	unsigned int val = 0;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL ||
		table->table_id == AAL_TABLE_HASH_OVERFLOW_TBL)
		;
	else
		return AAL_E_TBLNOTEXIST;

	/*** write DATA reg, then trigger ACCESS reg to write ***/

	//debug_Aaron
	DBG(printk("%s: table->table_id=%d, table->access_reg=0x%x, table->data_reg_num=%d\n", __func__,
		table->table_id, table->access_reg, table->data_reg_num));

	for (i = 0; i < table->data_reg_num; i++) {
		DBG(printk("%s: i=%d, addr=0x%x\n", __func__, i, table->access_reg + ((table->data_reg_num - i) << 2)));
		CA_NE_REG_WRITE(0, table->access_reg + ((table->data_reg_num - i) << 2));
	}

	val |= 1 << 30; /* op is write */
	val |= 1 << 31;
	val |= idx;
	CA_NE_REG_WRITE(val, table->access_reg);

	i = 0;
	do {
		val = CA_NE_REG_READ(table->access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	if (table->table_id == AAL_TABLE_HASH_MASK_TBL) {
		for (i = 0; i < table->data_reg_num; i++) {
			DBG(printk("%s: i=%d, addr=0x%x\n", __func__, i, table->access_reg + ((table->data_reg_num - i) << 2)));
			CA_NE_REG_WRITE(0, table->access_reg + ((table->data_reg_num - i) << 2));
		}

		val = 0;
		val |= 1 << 30; /* op is write */
		val |= 1 << 31;
		val |= 1 << 6; /* addr[6] = 1 for upper 128-bit */
		val |= idx;
		CA_NE_REG_WRITE(val, table->access_reg);

		i = 0;
		do {
			val = CA_NE_REG_READ(table->access_reg);
		} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
		if (i >= TABLE_TRY_TIMEOUT)
			return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	}

	return AAL_E_OK;
}

/*static int __hash_hw_table_flush(aal_table_t *table)
{
	int i;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	for (i = 0; i < table->max_entry; i++)
		__hash_hw_entry_clear(table, i);

	return AAL_E_OK;
}*/

static int _hash_ddr_entry_set(aal_table_t *table, unsigned int idx)
{
	unsigned int *ptr;
	void *src, *dest_virt;
	size_t data_len;
	ca_uint32_t *main_hash_table_ptr;
	aal_hash_value_t *hash_entry;
	u64 offset = 0;
	unsigned int action_width;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	if(idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	switch (table->table_id) {
	case AAL_TABLE_HASH_HASH_TBL:
		main_hash_table_ptr = (ca_uint32_t *) hash_table_virtual_addr;
		hash_entry = (aal_hash_value_t *) ptr;
		//debug_Aaron
		DBG(printk("%s: hash_entry->crc32 = 0x%x, hash_table_phy_addr = 0x%x, main_hash_table_ptr = %p\n", __func__,
			hash_entry->crc32, hash_table_phy_addr, main_hash_table_ptr));
		main_hash_table_ptr[idx] = hash_entry->crc32;
		return AAL_E_OK;
	case AAL_TABLE_HASH_HASH_ACTION:
		dest_virt = (void *) hash_action_table_virtual_addr;
		action_width = g_ha_width;
		break;
	case AAL_TABLE_HASH_OVERFLOW_ACTION:
		dest_virt = (void *) hash_overflow_hash_fib_virtual_addr;
		action_width = g_oa_width;
		break;
	case AAL_TABLE_HASH_DEFAULT_ACTION:
		dest_virt = (void *) hash_default_hash_fib_virtual_addr;
		action_width = g_da_width;
		break;
	default:
		return AAL_E_TBLNOTEXIST;
	}

	/* for HASH_ACTION OVERFLOW_ACTION and DEFAULT_ACTION */
	src = (void *) ptr;
	data_len = ha_width_array[action_width];
	offset = idx * data_len;
	//debug_Aaron
	DBG(printk("%s: idx = %u, dest_virt = 0x%p, src = 0x%p, data_len = %zu, offset = %llu\n", __func__,
	idx, dest_virt, src, data_len, offset));
	memcpy(dest_virt + offset, src, data_len);

	return AAL_E_OK;
}

static int _hash_ddr_entry_get(aal_table_t *table, unsigned int idx, unsigned int *ptr)
{
	void *dest, *src_virt;
	size_t data_len;
	ca_uint32_t *main_hash_table_ptr;
	aal_hash_value_t *hash_entry;
	u64 offset = 0;
	unsigned int action_width;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	if (table->entry[idx].data == NULL &&
		table->table_id != AAL_TABLE_CACHE_HASH_ACTION_TBL)
		return AAL_E_ENTRYNOTFOUND;

	if(idx >= table->max_entry) return AAL_E_OUTRANGE;

	//ptr = table->entry[idx].data;

	switch (table->table_id) {
	case AAL_TABLE_HASH_HASH_TBL:
		main_hash_table_ptr = (ca_uint32_t *) hash_table_virtual_addr;
		hash_entry = (aal_hash_value_t *) ptr;
		//debug_Aaron
		DBG(printk("%s: hash_entry->crc32 = 0x%x, hash_table_phy_addr = 0x%x, main_hash_table_ptr = %p\n", __func__,
		hash_entry->crc32, hash_table_phy_addr, main_hash_table_ptr));
		/* get crc32 from main hash table */
		hash_entry->crc32 = main_hash_table_ptr[idx];
		return AAL_E_OK;
	case AAL_TABLE_HASH_HASH_ACTION:
		src_virt = (void *) hash_action_table_virtual_addr;
		action_width = g_ha_width;
		break;
	case AAL_TABLE_HASH_OVERFLOW_ACTION:
		src_virt = (void *) hash_overflow_hash_fib_virtual_addr;
		action_width = g_oa_width;
		break;
	case AAL_TABLE_HASH_DEFAULT_ACTION:
		src_virt = (void *) hash_default_hash_fib_virtual_addr;
		action_width = g_da_width;
		break;
	case AAL_TABLE_CACHE_HASH_ACTION_TBL:
		src_virt = (void *) hash_cache_fib_virtual_addr;
		action_width = g_ca_width;
		break;
	default:
		return AAL_E_TBLNOTEXIST;
	}

	/* for HASH_ACTION OVERFLOW_ACTION and DEFAULT_ACTION */
	dest= (void *) ptr;
	data_len = ha_width_array[action_width];
	offset = idx * data_len;
	//debug_Aaron
	DBG(printk("%s: idx = %u, dest = 0x%p, src_virt = 0x%p, data_len = %zu, offset = %llu\n", __func__,
	idx, dest, src_virt, data_len, offset));
	memcpy(dest, src_virt + offset, data_len);

	return AAL_E_OK;
}

static int _hash_ddr_entry_clear(aal_table_t *table, unsigned int idx)
{
	unsigned int *ptr;
	void *dest_virt;
	size_t data_len;
	ca_uint32_t *main_hash_table_ptr;
	u64 offset = 0;
	unsigned int action_width;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	if(idx >= table->max_entry) return AAL_E_OUTRANGE;

	ptr = table->entry[idx].data;

	switch (table->table_id) {
	case AAL_TABLE_HASH_HASH_TBL:
		main_hash_table_ptr = (ca_uint32_t *) hash_table_virtual_addr;
		//debug_Aaron
		DBG(printk("%s: hash_idx = 0x%x, hash_table_phy_addr = 0x%x, main_hash_table_ptr = %p\n", __func__,
		idx, hash_table_phy_addr, main_hash_table_ptr));
		main_hash_table_ptr[idx] = 0;
		return AAL_E_OK;
	case AAL_TABLE_HASH_HASH_ACTION:
		dest_virt = (void *) hash_action_table_virtual_addr;
		action_width = g_ha_width;
		break;
	case AAL_TABLE_HASH_OVERFLOW_ACTION:
		dest_virt = (void *) hash_overflow_hash_fib_virtual_addr;
		action_width = g_oa_width;
		break;
	case AAL_TABLE_HASH_DEFAULT_ACTION:
		dest_virt = (void *) hash_default_hash_fib_virtual_addr;
		action_width = g_da_width;
		break;
	default:
		return AAL_E_TBLNOTEXIST;
	}

	/* for HASH_ACTION OVERFLOW_ACTION and DEFAULT_ACTION */
	data_len = ha_width_array[action_width];
	offset = idx * data_len;
	//debug_Aaron
	DBG(printk("%s: idx = %u, dest_virt = 0x%p, data_len = %zu, offset = %llu\n", __func__,
	idx, dest_virt, data_len, offset));
	memset(dest_virt + offset, 0, data_len);

	return AAL_E_OK;
}

/*static int __hash_ddr_table_flush(aal_table_t *table)
{
	int i;

	if (table == NULL || table->entry == NULL) return AAL_E_TBLNOTEXIST;

	for (i = 0; i < table->max_entry; i++)
		_hash_ddr_entry_clear(table, i);

	return AAL_E_OK;
}*/

/* For copying data into hash_hash_action_entry_t,
 * or copying hash_hash_action_entry_t into data.
 *
 * HW provide 64, 128, 192, 256, 384, 512, 768, 1024 bits space for hash action,
 * actual data content and size of hash action is unknown.
 *
 * data_len: actual size of hash action. Not size of hash_hash_action_entry_t.
 */
static void *__memcpy_hash_action(void *dest, void *src, size_t data_len)
{
	size_t final_len;

	final_len = min(ha_width_array[g_ha_width], data_len);

	return memcpy(dest, src, final_len);
}

/***************************************************************************
 * Hash misc
 ***************************************************************************/
/* if using memset in sram, alignment fault will occur. So use variable assignment to avoid this. */
int _hash_memset(void *dest, u8 c, size_t data_len){
	u8 *dest8;
	int i;

	dest8 = (u8 *) dest;
	for (i = 0; i < data_len; i++) {
		*dest8++ = c;
	}

	return AAL_E_OK;
}

/* 1. Reset all ages and valid bits in Hash Table and Overflow Table
	2. Reset all tables in DDR */
int aal_hash_mem_init(void)
{
	L3FE_FE_L3E_HS_MEM_INI_t val;
	int i;
	size_t data_len;

	memset((void *) &val, 0, sizeof(val));

	val.bf.req_sts = 0x1;
	CA_NE_REG_WRITE(val.wrd, L3FE_FE_L3E_HS_MEM_INI);

	i = 0;
	do {
		val.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_MEM_INI);
	} while ((0x1 & val.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/*2. Reset all tables in DDR and SRAM */
	data_len = hash_overflow_hash_fib_count * hash_overflow_hash_fib_length;
	printk("%s: hash_overflow_fib data_len=0x%zx, hash_overflow_fib_virtual_addr=0x%p\n", __func__,
			data_len, (void *)hash_overflow_hash_fib_virtual_addr);
	_hash_memset((void *) hash_overflow_hash_fib_virtual_addr, 0, data_len);
	overflow_tbl_table.max_entry = hash_overflow_hash_fib_count;
	overflow_action_table.max_entry = hash_overflow_hash_fib_count;

	data_len = hash_default_hash_fib_count * hash_default_hash_fib_length;
	printk("%s: hash_default_fib data_len=0x%zx, hash_default_fib_virtual_addr=0x%p\n", __func__,
			data_len, (void *)hash_default_hash_fib_virtual_addr);
	_hash_memset((void *) hash_default_hash_fib_virtual_addr, 0, data_len);

	data_len = hash_action_table_entry_count * hash_action_table_entry_length;
	printk("%s: hash_main_fib data_len=0x%zx, hash_action_table_virtual_addr=0x%p\n", __func__,
			data_len, (void *)hash_action_table_virtual_addr);
	_hash_memset((void *) hash_action_table_virtual_addr, 0, data_len);

	data_len = hash_table_entry_count * hash_table_entry_length;
	printk("%s: hash_main_hash data_len=0x%zx, hash_table_virtual_addr=0x%p\n", __func__,
			data_len, (void *)hash_table_virtual_addr);
	_hash_memset((void *) hash_table_virtual_addr, 0, data_len);

	data_len = hash_cache_fib_count * hash_cache_fib_length;
	printk("%s: hash_cache_fib data_len=0x%zx, hash_cache_fib_virtual_addr=0x%p\n", __func__,
			data_len, (void *)hash_cache_fib_virtual_addr);
	_hash_memset((void *) hash_cache_fib_virtual_addr, 0, data_len);
	cache_hash_action_table.max_entry = hash_cache_fib_count;

	return AAL_E_OK;
}

/* Register FE_L3E_HS_HASH_INI */
int aal_hash_hash_table_size_set(u32 ht_size, u8 hb_size, u8 ha_width)
{
	L3FE_FE_L3E_HS_HASH_INI_t hash_ini;

	printk("%s: ht_size = %d, hb_size = %d, ha_width = %d\n", __func__, ht_size, hb_size, ha_width);

	memset((void *) &hash_ini, 0, sizeof(hash_ini));
	if (ht_size >= HASH_HT_SIZE_512 && ht_size <= HASH_HT_SIZE_64K) {
		/*
		*	ht_size: Hash Table Size[2:0]
		*	0: 512-entry
		*	1: 1K-entry
		*	2: 2K-entry
		*	3: 4K-entry
		*	4: 8K-entry
		*	5: 16K-entry
		*	6: 32K-entry
		*	7: 64K-entry (only support in G3)
		*/
		int i = 0;

		while(ht_size >>= 1) { i++; } /* calc pow(ht_size, 2) */

		hash_ini.bf.ht_size = (i - 9) & 0x7; /* 0: 512-entry. 512 = 2^9 */

		g_hw_ht_size = hash_ini.bf.ht_size;

		printk("%s: hash_ini.bf.ht_size = %d\n", __func__, i - 9);
	} else {
		printk("%s: AAL_E_INVALID: Wrong value: ht_size = %d\n", __func__, ht_size);
		return AAL_E_INVALID;
	}

	/*
	*	HW define: Hash Bucket Size[1:0]
	*		0: 4-entry
	*		1: 8-entry
	*		2: 16-entry
	*		3: 32-entry
	*	hash_hb_size start from 2. So need to do deduct 2.
	*/
	if (hb_size >= HASH_HB_SIZE_4 && hb_size <= HASH_HB_SIZE_32) {
		hash_ini.bf.hb_size = hb_size - 2;
	} else {
		 return AAL_E_INVALID;
	}

	if (ha_width >= HASH_HA_WIDTH_64 && ha_width <= HASH_HA_WIDTH_1024) {
		hash_ini.bf.ha_width = ha_width;
	} else {
		 return AAL_E_INVALID;
	}

	CA_NE_REG_WRITE(hash_ini.wrd, L3FE_FE_L3E_HS_HASH_INI);

	g_ht_size = ht_size;
	g_hb_size = hb_size;
	g_ha_width = ha_width;

	return AAL_E_OK;
}

int aal_hash_key_selection(ca_uint32 profile_id)
{
	if(hash_key_select[profile_id] >= HASH_KEY_SEL_MAX)	return AAL_E_INVALID;
	CA_NE_REG_WRITE(hash_key_select[profile_id], L3FE_FE_L3E_HS_PF_0_KEY + profile_id*L3FE_FE_L3E_HS_PF_0_KEY_STRIDE);
	CA_NE_REG_WRITE(hash_tuple_sp_cfg[profile_id], L3FE_FE_L3E_HS_PF_0_TPL_SP + profile_id*L3FE_FE_L3E_HS_PF_0_TPL_SP_STRIDE);
	CA_NE_REG_WRITE(hash_tuple_dp_cfg[profile_id], L3FE_FE_L3E_HS_PF_0_TPL_DP + profile_id*L3FE_FE_L3E_HS_PF_0_TPL_DP_STRIDE);
	CA_NE_REG_WRITE(hash_tuple_sip_cfg[profile_id], L3FE_FE_L3E_HS_PF_0_TPL_SIP + profile_id*L3FE_FE_L3E_HS_PF_0_TPL_SIP_STRIDE);
	CA_NE_REG_WRITE(hash_tuple_dip_cfg[profile_id], L3FE_FE_L3E_HS_PF_0_TPL_DIP + profile_id*L3FE_FE_L3E_HS_PF_0_TPL_DIP_STRIDE);
	

	return AAL_E_OK;
}

/***************************************************************************
 * Hash Profile
 ***************************************************************************/

/* Validate the action group.
 * The enabled action group decides the size of hash action.
 */
static int __validate_action_group(unsigned int bitmask)
{
	int bits = 0;
	int max_bits = ha_width_array[g_ha_width] * 8;

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
	if (bitmask & ACTION_GROUP_20) bits += ACTION_GROUP_20_BITS;
	if (bitmask & ACTION_GROUP_21) bits += ACTION_GROUP_21_BITS;
	if (bitmask & ACTION_GROUP_22) bits += ACTION_GROUP_22_BITS;

	if (bits > max_bits) {
		printk("%s: max_bits = %d  bits = %d\n", __func__, max_bits, bits);
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}


/* Initialize the profile.
 * This must run after setting hash table size (aal_hash_hash_table_size_set).
 */
int aal_hash_profile_set(unsigned int profile_id, aal_hash_profile_t *profile)
{
	L3FE_FE_L3E_HS_PROFILE0_INI_t profile_ini;
	L3FE_FE_L3E_HS_MASK_MEM_MASK0_t maskptr0;
	L3FE_FE_L3E_HS_MASK_MEM_MASK1_t maskptr1;
	L3FE_FE_L3E_HS_MASK_MEM_MASK2_t maskptr2;
	L3FE_FE_L3E_HS_MASK_MEM_MASK3_t maskptr3;
	L3FE_FE_L3E_HS_PROFILE0_TUPLE0_t tuple;
	hash_default_action_entry_t default_action;
	unsigned int default_action_idx;

	unsigned int i, tpl_num = 0;
	unsigned int mask_idx;
	unsigned int val;
	int ret;

	if ((profile_id < HASH_PROFILE_0) || (profile_id >= HASH_PROFILE_MAX))
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

	memset((void *) &profile_ini, 0, sizeof(profile_ini));
	memset((void *) &maskptr0, 0, sizeof(maskptr0));
	memset((void *) &maskptr1, 0, sizeof(maskptr1));
	memset((void *) &maskptr2, 0, sizeof(maskptr2));
	memset((void *) &maskptr3, 0, sizeof(maskptr3));
	memset((void *) &tuple, 0, sizeof(tuple));
	memset((void *) &default_action, 0, sizeof(default_action));

	/* action group mask: a_mask[0:23] and fetch_sz[24:26] */
	val = 0;
	val = profile->type0_act_grp_mask_hit | (g_ha_width << 24);
	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_PROFILE0_TYPE0_ACTION
		+ profile_id * L3FE_FE_L3E_HS_PROFILE0_TYPE0_ACTION_STRIDE);

	val = profile->type1_act_grp_mask_hit | (g_ha_width << 24);
	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_PROFILE0_TYPE0_ACTION
		+ profile_id * L3FE_FE_L3E_HS_PROFILE0_TYPE0_ACTION_STRIDE + 4);

	/* default hash action when hash miss (empty miss & age miss) */

	__memcpy_hash_action(&default_action, profile->miss.type0_empty_miss, profile->miss.type0_empty_miss_len);
	ret = aal_entry_get(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
	if (ret != AAL_E_OK) {
		ret = aal_entry_add(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
		if (ret != AAL_E_OK) return ret;
		default_action_table.entry[default_action_idx].local_data = profile->miss.type0_act_grp_mask_empty_miss;
		val = 0;
		val = profile->miss.type0_act_grp_mask_empty_miss | (g_da_width << 24);
		CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_DEFAULT_ACTION_0
			+ default_action_idx * L3FE_FE_L3E_HS_DEFAULT_ACTION_0_STRIDE);
	}
	profile_ini.bf.default_sel_0e = def_action[profile_id].type0_empty_miss_idx = default_action_idx;

	__memcpy_hash_action(&default_action, profile->miss.type0_age_miss, profile->miss.type0_age_miss_len);
	ret = aal_entry_get(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
	if (ret != AAL_E_OK) {
		ret = aal_entry_add(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
		if (ret != AAL_E_OK) return ret;
		default_action_table.entry[default_action_idx].local_data = profile->miss.type0_act_grp_mask_age_miss;
		val = 0;
		val = profile->miss.type0_act_grp_mask_age_miss | (g_da_width << 24);
		CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_DEFAULT_ACTION_0
			+ default_action_idx * L3FE_FE_L3E_HS_DEFAULT_ACTION_0_STRIDE);
	}
	profile_ini.bf.default_sel_0a = def_action[profile_id].type0_age_miss_idx = default_action_idx;

	__memcpy_hash_action(&default_action, profile->miss.type1_empty_miss, profile->miss.type1_empty_miss_len);
	ret = aal_entry_get(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
	if (ret != AAL_E_OK) {
		ret = aal_entry_add(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
		if (ret != AAL_E_OK) return ret;
		default_action_table.entry[default_action_idx].local_data = profile->miss.type1_act_grp_mask_empty_miss;
		val = 0;
		val = profile->miss.type1_act_grp_mask_empty_miss | (g_da_width << 24);
		CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_DEFAULT_ACTION_0
			+ default_action_idx * L3FE_FE_L3E_HS_DEFAULT_ACTION_0_STRIDE);
	}
	profile_ini.bf.default_sel_1e = def_action[profile_id].type1_empty_miss_idx = default_action_idx;

	__memcpy_hash_action((void *)&default_action, profile->miss.type1_age_miss, profile->miss.type1_age_miss_len);
	ret = aal_entry_get(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
	if (ret != AAL_E_OK) {
		ret = aal_entry_add(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action, &default_action_idx);
		if (ret != AAL_E_OK) return ret;
		default_action_table.entry[default_action_idx].local_data = profile->miss.type1_act_grp_mask_age_miss;
		val = 0;
		val = profile->miss.type1_act_grp_mask_age_miss | (g_da_width << 24);
		CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_DEFAULT_ACTION_0
			+ default_action_idx * L3FE_FE_L3E_HS_DEFAULT_ACTION_0_STRIDE);
	}
	profile_ini.bf.default_sel_1a = def_action[profile_id].type1_age_miss_idx = default_action_idx;

	/* tuple type, hash mask index */
	for (i = 0; i < NUM_TUPLE; i++) {
		if ((profile->hit[i].tuple_type == TUPLE_TYPE_UNUSED) || (profile->hit[i].hash_mask == NULL)) {
			continue;
		} else if (profile->hit[i].tuple_type == TUPLE_TYPE_FWD) {
			tuple.bf.type = TUPLE_TYPE_FWD;
		} else if (profile->hit[i].tuple_type == TUPLE_TYPE_QOS) {
			tuple.bf.type = TUPLE_TYPE_QOS;
		}

		ret = aal_entry_get(AAL_TABLE_HASH_MASK_TBL, profile->hit[i].hash_mask, &mask_idx);
		if (ret != AAL_E_OK) {
			ret = aal_entry_add(AAL_TABLE_HASH_MASK_TBL, profile->hit[i].hash_mask, &mask_idx);
			if (ret != AAL_E_OK) return ret;
		}

		tuple.bf.maskptr = mask_idx & 0x3f;
#if defined(CONFIG_LUNA_G3_SERIES)
		tuple.bf.pri = ((L3FE_FE_L3E_HS_PROFILE0_TUPLE0_t)(profile->hit[i].tuple_wrd)).bf.pri;
		if(profile_id==2)
		{
			printk("[%d]tuple.bf.pri=%d tuple.bf.maskptr=%d !!!!!!!!!!! \n",tpl_num,tuple.bf.pri,tuple.bf.maskptr);
		}
#endif

		CA_NE_REG_WRITE(tuple.wrd, L3FE_FE_L3E_HS_PROFILE0_TUPLE0
			+ profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + tpl_num * 4);
		profile->hit[i].tuple_wrd = tuple.wrd;

		tpl_num++;
	}

	profile_ini.bf.tpl_num = tpl_num & 0xf;

	CA_NE_REG_WRITE(profile_ini.wrd, L3FE_FE_L3E_HS_PROFILE0_INI + profile_id * L3FE_FE_L3E_HS_PROFILE0_INI_STRIDE);

	aal_hash_key_selection(profile_id);

	return AAL_E_OK;
}

int aal_hash_profile_tuple_add(
	ca_uint32_t profile_id,
	aal_hash_tuple_type_e tuple_type,
	ca_uint8_t priority,
	ca_uint8_t mask_id,
	ca_uint8_t *tuple_idx) {

	L3FE_FE_L3E_HS_PROFILE0_TUPLE0_t tuple;
	L3FE_FE_L3E_HS_PROFILE0_INI_t profile_ini;
	aal_hash_mask_t hash_mask;
	aal_hash_profile_t *profile;
	unsigned int i, tpl_num = 0, tuple_num_dbchk = 0;
	int ret;
	unsigned long flags;

	if(tuple_idx == NULL)
		return AAL_E_NULLPTR;

	if(profile_id > current_profile_count) {
		printk("%s:%d Unable to add tuple to profile which does not exist.\n", __func__, __LINE__);
		return AAL_E_OUTRANGE;
	}

	memset((void *) &profile_ini, 0, sizeof(profile_ini));
	memset((void *) &hash_mask, 0, sizeof(hash_mask));
	memset((void *) &tuple, 0, sizeof(tuple));

	/* Only work for 3 profiles: LAN/WAN/MULTICAST now */
	/* Todo: use list or array to manage profile list*/
	if(profile_id == HASH_PROFILE_0)
		profile = &hash_profile_0;
	else if (profile_id == HASH_PROFILE_1)
		profile = &hash_profile_1;
	else if (profile_id == HASH_PROFILE_2)
		profile = &hash_profile_2;
#if defined(CONFIG_LUNA_G3_SERIES)
	else if (profile_id == 3)
		profile = &hash_profile_3;
	else if (profile_id == HASH_PROFILE_4)
		profile = &hash_profile_4;
#endif
	else {
		/* should not reach here */
		printk("%s:%d profile_id=[%d] does not exist.\n", __func__, __LINE__, profile_id);
		return AAL_E_OUTRANGE;
	}

	ret = aal_entry_get_by_idx(AAL_TABLE_HASH_MASK_TBL, mask_id, &hash_mask);
//	ret = __mask_tbl_entry_get_by_idx(mask_id, &hash_mask);
	if (ret != AAL_E_OK) {
		printk("%s:%d Hash mask does not exist. mask id = %d\n", __func__, __LINE__, mask_id);
		return ret;
	}

	spin_lock_irqsave(&g_hash_lock, flags);

	profile_ini.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_PROFILE0_INI + profile_id * L3FE_FE_L3E_HS_PROFILE0_INI_STRIDE);

	tpl_num = profile_ini.bf.tpl_num;
	DBG(printk("%s:%d Profile %d before adding: tpl_num = %d\n", __func__, __LINE__, profile_id, tpl_num));

	/* tuple type, hash mask index */
	for (i = 0; i < NUM_TUPLE; i++) {
		if ((profile->hit[i].tuple_type == tuple_type) && (0 == memcmp(profile->hit[i].hash_mask, &hash_mask, sizeof(hash_mask)))) {
			*tuple_idx = i;
			profile->hit[i].refcnt++;
			spin_unlock_irqrestore(&g_hash_lock, flags);
			DBG(printk("%s:%d tuple_idx %d has existed, refcnt = %d\n", __func__, __LINE__, *tuple_idx, profile->hit[i].refcnt));
			return AAL_E_OK;
		}
		if ((profile->hit[i].tuple_type == TUPLE_TYPE_UNUSED) && (profile->hit[i].hash_mask == NULL)) {
			break;
		}
	}

	if(i == NUM_TUPLE) {
		printk("%s:%d Hash tuple is FULL. profile id = %d\n", __func__, __LINE__, profile_id);
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_TBLFULL;
	}

	if( mask_tbl_table.entry[mask_id].data == NULL) {
		printk("%s:%d hash_mask is NULL. profile id = %d, mask_id = %d\n", __func__, __LINE__, profile_id, mask_id);
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_NULLPTR;
	}

	/* add a tuple */
	tuple.bf.maskptr = mask_id & 0x3f;
	tuple.bf.pri = priority & 0x7;
	tuple.bf.type = tuple_type & 0x1;
	CA_NE_REG_WRITE(tuple.wrd, L3FE_FE_L3E_HS_PROFILE0_TUPLE0 + profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + tpl_num * 4);

	profile->hit[i].tuple_type = tuple_type;
	profile->hit[i].hash_mask = (aal_hash_mask_t *) mask_tbl_table.entry[mask_id].data;
	profile->hit[i].refcnt = 1;
	profile->hit[i].tuple_wrd = tuple.wrd;

	*tuple_idx = i;
	tpl_num++;

	DBG(printk("%s:%d Profile %d after adding: tpl_num = %d\n", __func__, __LINE__, profile_id, tpl_num));
	/* Check tuple number is correct */
	for (i = 0; i < NUM_TUPLE; i++) {
		if ((profile->hit[i].tuple_type == TUPLE_TYPE_UNUSED) && (profile->hit[i].hash_mask == NULL))
			continue;
		else
			tuple_num_dbchk++;
	}

	if(tuple_num_dbchk != tpl_num) {
		printk("%s:%d Error: tpl_num = %d, tuple_num_dbchk = %d\n", __func__, __LINE__, tpl_num, tuple_num_dbchk);
		tpl_num = tuple_num_dbchk;
	}

	profile_ini.bf.tpl_num = tpl_num & 0xf;
	CA_NE_REG_WRITE(profile_ini.wrd, L3FE_FE_L3E_HS_PROFILE0_INI + profile_id * L3FE_FE_L3E_HS_PROFILE0_INI_STRIDE);

	spin_unlock_irqrestore(&g_hash_lock, flags);
	return AAL_E_OK;
}

int aal_hash_profile_tuple_del(
	ca_uint32_t profile_id,
	ca_uint8_t tuple_idx) {

	L3FE_FE_L3E_HS_PROFILE0_TUPLE0_t tuple, tuple_tmp;
	L3FE_FE_L3E_HS_PROFILE0_INI_t profile_ini;
	aal_hash_profile_t *profile;
	unsigned int i, j, tpl_num = 0, tuple_num_dbchk = 0, hw_tpl_idx = 0;
	unsigned long flags;

	if(tuple_idx > NUM_TUPLE)
		return AAL_E_OUTRANGE;

	if(profile_id > current_profile_count) {
		printk("%s:%d Unable to add tuple to profile which does not exist.\n", __func__, __LINE__);
		return AAL_E_OUTRANGE;
	}

	memset((void *) &profile_ini, 0, sizeof(profile_ini));
	memset((void *) &tuple, 0, sizeof(tuple));

	/* Only work for 3 profiles: LAN/WAN/MULTICAST now */
	/* Todo: use list or array to manage profile list*/
	if(profile_id == HASH_PROFILE_0)
		profile = &hash_profile_0;
	else if (profile_id == HASH_PROFILE_1)
		profile = &hash_profile_1;
	else if (profile_id == HASH_PROFILE_2)
		profile = &hash_profile_2;
#if defined(CONFIG_LUNA_G3_SERIES)
	else if (profile_id == 3)
		profile = &hash_profile_3;
	else if (profile_id == HASH_PROFILE_4)
		profile = &hash_profile_4;
#endif
	else {
		/* should not reach here */
		printk("%s:%d profile_id=[%d] does not exist.\n", __func__, __LINE__, profile_id);
		return AAL_E_OUTRANGE;
	}

	spin_lock_irqsave(&g_hash_lock, flags);

	profile_ini.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_PROFILE0_INI + profile_id * L3FE_FE_L3E_HS_PROFILE0_INI_STRIDE);

	tpl_num = profile_ini.bf.tpl_num;
	DBG(printk("%s:%d Profile %d before deletion: tpl_num = %d\n", __func__, __LINE__, profile_id, tpl_num));

	if ((profile->hit[tuple_idx].tuple_type == TUPLE_TYPE_UNUSED) && (profile->hit[tuple_idx].hash_mask == NULL)) {
		printk("%s:%d tuple of the profile does not exist. tuple_idx = %d\n", __func__, __LINE__, tuple_idx);
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_ENTRYNOTFOUND;
	}
	/* find hw tuple entry */
	for (j = 0; j < tpl_num; j++) {
		tuple_tmp.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_PROFILE0_TUPLE0 + profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + j * 4);
		if (tuple_tmp.wrd == profile->hit[tuple_idx].tuple_wrd) {
			/* find the hw tuple entry */
			hw_tpl_idx = j;
			break;
		}
	}
	if(j == NUM_TUPLE) {
		printk("%s:%d Hw Hash tuple doesn't be found. profile id = %d, tuple_wrd = %08x\n", __func__, __LINE__, profile_id, profile->hit[tuple_idx].tuple_wrd);
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_ENTRYNOTFOUND;
	}

	if (--profile->hit[tuple_idx].refcnt) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		DBG(printk("%s:%d tuple_idx %d is used, refcnt = %d\n", __func__, __LINE__, tuple_idx, profile->hit[tuple_idx].refcnt));
		return AAL_E_OK;
	}

	/* Todo: Need to discuss the sequence of decreasing profile tpl_num and deleting profile tuple*/
	/* hw limitation requires to allocate the active tuples in front */
	for (j = (hw_tpl_idx+1); j < tpl_num; j++, hw_tpl_idx++) {
		tuple_tmp.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_PROFILE0_TUPLE0 + profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + j * 4);
		/* copy the hw tuple entry to the front */
		CA_NE_REG_WRITE(tuple_tmp.wrd, L3FE_FE_L3E_HS_PROFILE0_TUPLE0 + profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + hw_tpl_idx * 4);
	}
	/* deleting hw tuple entry */
	CA_NE_REG_WRITE(tuple.wrd, L3FE_FE_L3E_HS_PROFILE0_TUPLE0 + profile_id * L3FE_FE_L3E_HS_PROFILE0_TUPLE0_STRIDE + hw_tpl_idx * 4);

	/* deleting profile tuple */
	profile->hit[tuple_idx].tuple_type = TUPLE_TYPE_UNUSED;
	profile->hit[tuple_idx].hash_mask = NULL; /* NOTE: remember to delete the mask_entry in the mask table */
	profile->hit[tuple_idx].tuple_wrd = 0;

	/* decreasing profile tpl_num */
	tpl_num--;

	DBG(printk("%s:%d Profile %d after deletion: tpl_num = %d\n", __func__, __LINE__, profile_id, tpl_num));

	/* Check tuple number is correct */
	for (i = 0; i < NUM_TUPLE; i++) {
		if ((profile->hit[i].tuple_type == TUPLE_TYPE_UNUSED) && (profile->hit[i].hash_mask == NULL))
			continue;
		else
			tuple_num_dbchk++;
	}

	if(tuple_num_dbchk != tpl_num) {
		printk("%s:%d Error: tpl_num = %d, tuple_num_dbchk = %d\n", __func__, __LINE__, tpl_num, tuple_num_dbchk);
		tpl_num = tuple_num_dbchk;
	}

	profile_ini.bf.tpl_num = tpl_num;
	CA_NE_REG_WRITE(profile_ini.wrd, L3FE_FE_L3E_HS_PROFILE0_INI + profile_id * L3FE_FE_L3E_HS_PROFILE0_INI_STRIDE);

	spin_unlock_irqrestore(&g_hash_lock, flags);
	return AAL_E_OK;
}

/***************************************************************************
 * Hash Aging
 ***************************************************************************/

extern ca_uint32_t l3fe_main_hash_action_cache_fib_count;

ca_status_t aal_hash_aging_timer_set_iros(
CA_IN	ca_device_id_t	device_id,
CA_IN	ca_uint32_t	time)
{
	return aal_hash_aging_timer_set(time);
}

/* Set global aging time in seconds. */
int aal_hash_aging_timer_set(unsigned int time)
{
	uint64_t quotient;
	uint32_t remainder;
	struct ca_soc_data soc_data;

	L3FE_FE_L3E_HS_CACHE_AGE32_t cache_age_32;
	L3FE_FE_L3E_HS_CACHE_AGE10_t cache_age_10;
	uint64_t cache_unit_ticks; /* each tick is 256 clocks; see description of L3FE_FE_L3E_HS_CACHE_AGE32_t */

	if (time > g_max_aging_time) return AAL_E_OUTRANGE;

	if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
		soc_data.core_clk = 250000000; /* assume core clk 250MHz */

	g_aging_time = time;

#if 0
	/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
	g_aging_granularity = (uint32_t) ((uint64_t) time * soc_data.core_clk / HASH_AGING_UNIT / HASH_AGING_START) & 0x3FFFFFFF;
#else
	ca_div((uint64_t) time * soc_data.core_clk, HASH_AGING_UNIT * HASH_AGING_START, &quotient, &remainder);
	g_aging_granularity = (uint32_t) quotient & 0x3FFFFFFF;
#endif
	DBG(printk("%s:%d time = 0x%x g_aging_granularity = 0x%x\n", __func__, __LINE__, time, g_aging_granularity));
	CA_NE_REG_WRITE(g_aging_granularity, L3FE_FE_L3E_HS_AGING_GRANULARITY);

	/* Update cache aging time if necessary because:
	 * 1. If cache aging time is much longer than hash aging time, cache hash entry may be not expired while hash entry is expired.
	 * 2. Total hash age interval is HASH_AGING_START, this may decrease 1 but cache hash age does not.
	 *    So for safe, cache_aging_time must not exceed hash_aging_time * (HASH_AGING_START - 1) / HASH_AGING_START.
	 *
	 * cache_aging_time_in_second = (1/core_clk)*CACHE_HASH_AGING_UNIT * L3FE_FE_L3E_HS_CACHE_AGE.ut * CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_COUNT * CACHE_HASH_AGING_START
	 */

	ca_div((uint64_t) time * soc_data.core_clk * (HASH_AGING_START - 1), HASH_AGING_START * CACHE_HASH_AGING_UNIT * CACHE_HASH_AGING_START * l3fe_main_hash_action_cache_fib_count, &quotient, &remainder);
	cache_unit_ticks = (quotient > 65535) ? 65535 : quotient;

	cache_age_10.bf.ut_t0 = cache_unit_ticks;
	cache_age_10.bf.ut_t1 = cache_unit_ticks;
	cache_age_32.bf.ut_t2 = cache_unit_ticks;
	cache_age_32.bf.ut_t3 = cache_unit_ticks;

	CA_NE_REG_WRITE(cache_age_10.wrd, L3FE_FE_L3E_HS_CACHE_AGE10);
	CA_NE_REG_WRITE(cache_age_32.wrd, L3FE_FE_L3E_HS_CACHE_AGE32);

	return AAL_E_OK;
}

ca_status_t aal_hash_aging_timer_get_iros(
CA_IN	ca_device_id_t	device_id,
CA_OUT	ca_uint32_t	*time)
{
	return aal_hash_aging_timer_get(time);
}

/* Get global aging time in seconds.
 * it may not same as original input due to truncated calculation.
 */
int aal_hash_aging_timer_get(unsigned int *time)
{
	*time = g_aging_time;
	//*time = (uint32_t) ((uint64_t) g_aging_granularity * HASH_AGING_UNIT * HASH_AGING_START / soc_data.core_clk);

	return AAL_E_OK;
}

ca_status_t aal_hash_current_granularity_get_iros(
CA_IN	ca_device_id_t	device_id,
CA_OUT	ca_uint32_t	*granularity)
{
	return aal_hash_current_granularity_get(granularity);
}

/* Get granularity from HW register; the unit is ticks. */
int aal_hash_current_granularity_get(unsigned int *granularity)
{
	*granularity = CA_NE_REG_READ(L3FE_FE_L3E_HS_AGING_GRANULARITY);
	return AAL_E_OK;
}

ca_status_t aal_hash_age_set_iros(
CA_IN	ca_device_id_t	device_id,
CA_IN	ca_uint32_t	table_id,
CA_IN	ca_uint32_t	hash_idx,
CA_IN	ca_uint32_t	age)
{
	printk("table id 0: main hash aging status. table id 1: overflow hash aging status.\n");
	printk("hash_idx = 0x%x, age = 0x%x\n", hash_idx, age);

	if(table_id == 0)
		aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, hash_idx, age);
	else if(table_id == 1)
		aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx, age);

	return AAL_E_OK;
}

int aal_hash_age_set(aal_table_e table_id, unsigned int idx, unsigned int age)
{
	aal_table_t *table = NULL;
	aal_hash_age_t ages;
	unsigned int bucket, slot;
	unsigned int addr = 0;
	int i;
	unsigned long flags;

	if (table_id == AAL_TABLE_HASH_HASH_TBL) {
		table = &hash_tbl_table;
		bucket = (idx >> 5) & 0x7ff;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (0 << 11); /* Hash Age Table */
	} else if (table_id == AAL_TABLE_HASH_OVERFLOW_TBL) {
		table = &overflow_tbl_table;
		bucket = (idx >> 5) & 0x7;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (1 << 11); /* Overflow Age Table */
	} else {
		return AAL_E_OPNOTSUPP;
	}

	if (age > HASH_AGING_STATIC) return AAL_E_OUTRANGE;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	spin_lock_irqsave(&g_hash_lock, flags);

	/* set addr to ACCESS reg, then read ages from DATA reg */

	addr |= 0 << 30; /* op is read */
	addr |= 1 << 31;
	DBG(printk("%s:%d addr=0x%08x, bucket=%u, slot=%u\n", __func__, __LINE__, addr, bucket, slot););
	CA_NE_REG_WRITE(addr, L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);

	i = 0;
	do {
		addr = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);
	} while (((1 << 31) & addr) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1,
		slot 16~23 is in DATA reg 3, slot 24~31 is in DATA reg 4 */
	ages.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_DATA0 - ((slot >> 3) << 2));
	DBG(printk("%s:%d ages.wrd = 0x%x \n", __func__, __LINE__, ages.wrd););
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

	/* set addr to ACCESS reg, then write ages to DATA reg */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1,
		slot 16~23 is in DATA reg 3, slot 24~31 is in DATA reg 4 */
	CA_NE_REG_WRITE(ages.wrd, L3FE_FE_L3E_HS_HASH_AGE_MEM_DATA0 - ((slot >> 3) << 2));
	DBG(printk("%s:%d ages.wrd = 0x%x \n", __func__, __LINE__, ages.wrd););
	addr |= 1 << 30; /* op is write */
	addr |= 1 << 31;

	CA_NE_REG_WRITE(addr, L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);

	i = 0;
	do {
		addr = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);
	} while (((1 << 31) & addr) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

ca_status_t aal_hash_age_get_iros(
CA_IN	ca_device_id_t	device_id,
CA_IN	ca_uint32_t	table_id,
CA_IN	ca_uint32_t	hash_idx,
CA_OUT	ca_uint32_t	*age,
CA_OUT	ca_uint32_t	*time)
{
	printk("table id 0: main hash aging status. table id 1: overflow hash aging status.\n");

	if(table_id == 0)
		aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, hash_idx, age, time);
	else if(table_id == 1)
		aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx, age, time);

	printk("hash_idx = 0x%x, age = 0x%x, time = 0x%x\n", hash_idx, *age, *time);

	return AAL_E_OK;
}

int aal_hash_age_get(aal_table_e table_id, unsigned int idx, unsigned int *age, unsigned int *time)
{
	aal_table_t *table = NULL;
	aal_hash_age_t ages;
	unsigned int granularity;
	unsigned int bucket, slot;
	unsigned int addr = 0;
	int i;
	unsigned long flags;

	uint64_t quotient;
	uint32_t remainder;
	struct ca_soc_data soc_data;

	if (table_id == AAL_TABLE_HASH_HASH_TBL) {
		table = &hash_tbl_table;
		bucket = (idx >> 5) & 0x7ff;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (0 << 11); /* Hash Age Table */
	} else if (table_id == AAL_TABLE_HASH_OVERFLOW_TBL) {
		table = &overflow_tbl_table;
		bucket = (idx >> 5) & 0x1;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (1 << 11); /* Overflow Age Table */
	} else {
		return AAL_E_OPNOTSUPP;
	}

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;

	spin_lock_irqsave(&g_hash_lock, flags);
	DBG(printk("%s:%d addr=0x%08x, bucket=%u, slot=%u\n", __func__, __LINE__, addr, bucket, slot););
	aal_hash_current_granularity_get(&granularity);


	/* set addr to ACCESS reg, then read ages from DATA reg */

	addr |= 0 << 30; // op is read
	addr |= 1 << 31;

	CA_NE_REG_WRITE(addr, L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);

	i = 0;
	do {
		addr = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);
	} while (((1 << 31) & addr) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1,
		slot 16~23 is in DATA reg 3, slot 24~31 is in DATA reg 4 */
	ages.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_DATA0 - ((slot >> 3) << 2));
	DBG(printk("%s:%d ages.wrd = 0x%x \n", __func__, __LINE__, ages.wrd););
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

	if ((*age == HASH_AGING_STATIC) || (*age == HASH_AGING_INVALID)) {
		*time = 0;
	} else {
		if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
			soc_data.core_clk = 250000000; /* assume core clk 250MHz */

#if 0
		/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
		*time = (uint32_t) ((uint64_t) ((*age - 1) * g_aging_granularity + granularity) * HASH_AGING_UNIT / soc_data.core_clk);
#else
		ca_div((uint64_t) ((*age - 1) * g_aging_granularity + granularity) * HASH_AGING_UNIT, soc_data.core_clk, &quotient, &remainder);
		*time = (uint32_t) quotient;
#endif
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)

// support read and clear; 32 entries as one group
int aal_hash_traffic_status_get(aal_table_e table_id, unsigned int idx, unsigned int *trfStatus)
{
	aal_table_t *table = NULL;
	aal_hash_age_t ages;
	unsigned int bucket, slot;
	unsigned int addr = 0;
	int i, j, ageValue;
	unsigned long flags;


	if (table_id == AAL_TABLE_HASH_HASH_TBL) {
		table = &hash_tbl_table;
		bucket = (idx >> 5) & 0x7ff;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (0 << 11); /* Hash Age Table */
	} else if (table_id == AAL_TABLE_HASH_OVERFLOW_TBL) {
		table = &overflow_tbl_table;
		bucket = (idx >> 5) & 0x1;
		slot = idx & 0x1f;
		addr |= bucket;
		addr |= (1 << 11); /* Overflow Age Table */
	} else {
		return AAL_E_OPNOTSUPP;
	}

	if(slot!=0) return AAL_E_INVALID;		// treat each 32 entries as one group, only support input with the first one index of each group.

	if (idx > table->max_entry) return AAL_E_OUTRANGE;

	*trfStatus = 0;		// init

	spin_lock_irqsave(&g_hash_lock, flags);
	DBG(printk("%s:%d addr=0x%08x, bucket=%u, slot=%u\n", __func__, __LINE__, addr, bucket, slot););

	// ============ step1: read traffic status ============ 
	/* set addr to ACCESS reg, then read ages from DATA reg */

	addr |= 0 << 30; // op is read
	addr |= 1 << 31;

	CA_NE_REG_WRITE(addr, L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);

	i = 0;
	do {
		addr = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);
	} while (((1 << 31) & addr) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1,
		slot 16~23 is in DATA reg 3, slot 24~31 is in DATA reg 4 */

	for(i = 0; i < 4; i++)		// data reg[i]
	{
		ages.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_DATA0 - (i << 2));

		for(j = 0; j < 8; j++)	// slot[j]
		{
			switch (j) {
			case 0:
				ageValue = ages.bf.slot_0;
				ages.bf.slot_0 =  HASH_AGING_1;		// clear
				break;

			case 1:
				ageValue = ages.bf.slot_1;
				ages.bf.slot_1 =  HASH_AGING_1;		// clear
				break;

			case 2:
				ageValue = ages.bf.slot_2;
				ages.bf.slot_2 =  HASH_AGING_1;		// clear
				break;

			case 3:
				ageValue = ages.bf.slot_3;
				ages.bf.slot_3 =  HASH_AGING_1;		// clear
				break;

			case 4:
				ageValue = ages.bf.slot_4;
				ages.bf.slot_4 =  HASH_AGING_1;		// clear
				break;

			case 5:
				ageValue = ages.bf.slot_5;
				ages.bf.slot_5 =  HASH_AGING_1;		// clear
				break;

			case 6:
				ageValue = ages.bf.slot_6;
				ages.bf.slot_6 =  HASH_AGING_1;		// clear
				break;

			case 7:
				ageValue = ages.bf.slot_7;
				ages.bf.slot_7 =  HASH_AGING_1;		// clear
				break;
			};

			if(ageValue>HASH_AGING_1)				// active flow!
				*trfStatus |= ((0x1 << (i<<3))<<j);
		}
	}

	// ============  step2: clear traffic status ============ 
	/* set addr to ACCESS reg, then write ages to DATA reg */

	/* slot 0~7 is in DATA reg 0, slot 8~15 is in DATA reg 1,
		slot 16~23 is in DATA reg 3, slot 24~31 is in DATA reg 4 */
	for(i = 0; i < 4; i++)		// data reg[i]
	{
		CA_NE_REG_WRITE(ages.wrd, L3FE_FE_L3E_HS_HASH_AGE_MEM_DATA0 - (i << 2));
	}
	DBG(printk("%s:%d ages.wrd = 0x%x \n", __func__, __LINE__, ages.wrd););
	addr |= 1 << 30; /* op is write */
	addr |= 1 << 31;

	CA_NE_REG_WRITE(addr, L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);

	i = 0;
	do {
		addr = CA_NE_REG_READ(L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS);
	} while (((1 << 31) & addr) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */


	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}
#endif

/***************************************************************************
 * AAL table, 	L3FE_FE_L3E_HS_MASK_MEM_MASK for Mask Table
 ***************************************************************************/

int aal_mask_tbl_entry_add(aal_hash_mask_t *entry, unsigned int *rslt_idx)
{
	return __mask_tbl_entry_add(entry, rslt_idx);
}

int aal_mask_tbl_entry_del(unsigned int rslt_idx)
{
	return __mask_tbl_entry_del(rslt_idx);
}

ca_status_t aal_mask_tbl_entry_add_iros(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	aal_hash_mask_t	*entry,
	CA_OUT	ca_uint32_t	*rslt_idx)
{
	return aal_entry_add(AAL_TABLE_HASH_MASK_TBL, entry, rslt_idx);
}

ca_status_t aal_mask_tbl_entry_del_iros(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint32_t	rslt_idx)
{
	return	aal_entry_del(AAL_TABLE_HASH_MASK_TBL, rslt_idx);
}

static int __mask_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &mask_tbl_table;
	int i, ret;
	aal_entry_t *p_entry;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	/* search if entry exists already */

	for (i = 0; i < table->max_entry; i++) {
		p_entry = sw_entry_get(table, i);
		if ( (p_entry != NULL) && (p_entry->data != NULL) && (!memcmp(p_entry->data, entry, table->entry_size)) ) {
			*rslt_idx = i; // return the existing mask index
			atomic_inc(&table->entry[*rslt_idx].refcnt);
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_EXIST;
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
			if ((ret = _hash_hw_entry_set(table, *rslt_idx)) != AAL_E_OK) {
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return ret;
			}

			break;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __mask_tbl_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &mask_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hash_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __mask_tbl_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &mask_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = _hash_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __mask_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &mask_tbl_table;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __mask_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &mask_tbl_table;

	DBG(printk("%s:%d entry pointer: 0x%p , index: %d\n", __func__, __LINE__, entry, idx));
	DBG(printk("%s:%d table->entry pointer: 0x%p \n", __func__, __LINE__, table->entry));
	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __mask_tbl_table_flush(void)
{
	int i, ret;

	for (i = 0; i < mask_tbl_table.max_entry; i++) {
		ret = aal_entry_del(AAL_TABLE_HASH_MASK_TBL, i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __mask_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&mask_tbl_table);
}

static void __mask_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &mask_tbl_table;
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

	printk(KERN_INFO "|===== Hash Mask Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_MASK_TBL, i, (void *) &entry);
		} else {
			ret = __hash_hw_entry_get(&mask_tbl_table, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);

			printk(KERN_INFO "  Hash Control:\n");
			printk(KERN_INFO "    table_id %x, hkey_id %x, ctrl_set_id %x\n", entry.table_id, entry.hkey_id, entry.ctrl_set_id);

			printk(KERN_INFO "  Source Port ID\n");
			printk(KERN_INFO "    lspid %x, orig_lspid %x, mc_idx_vld %x\n", entry.lspid, entry.orig_lspid, entry.mc_idx_vld);

			printk(KERN_INFO "  Dest Port ID\n");
			printk(KERN_INFO "    mc %x, mcgid %03x\n", entry.mc, entry.mcgid);

			printk(KERN_INFO "  COS:\n");
			printk(KERN_INFO "    cos %x\n", entry.cos);

			printk(KERN_INFO "POL ID:\n");
			printk(KERN_INFO "  pol_id %d, pol_grp_id %d, qos_premark %d\n", entry.pol_id, entry.pol_grp_id, entry.qos_premark);

			printk(KERN_INFO "  MDATA:\n");
			printk(KERN_INFO "    mdata %016llx\n", entry.mdata);

			printk(KERN_INFO "  Special Packet:\n");
			printk(KERN_INFO "    spcl_pkt_enc %x, spcl_pkt_hdr_mtch %02x\n", entry.spcl_pkt_enc, entry.spcl_pkt_hdr_mtch);

			printk(KERN_INFO "  L2:\n");
			printk(KERN_INFO "    mac_da %02x, mac_da_an_sel %x, mac_da_ip_mc %x\n",
				entry.mac_da, entry.mac_da_an_sel, entry.mac_da_ip_mc);
			printk(KERN_INFO "    mac_da_rng %x, mac_da_rsvd %x\n", entry.mac_da_rng, entry.mac_da_rsvd);
			printk(KERN_INFO "    mac_sa %02x\n", entry.mac_sa);
			printk(KERN_INFO "    ethertype %x, ethertype_enc %x\n", entry.ethertype, entry.ethertype_enc);

			printk(KERN_INFO "  L2 Format:\n");
			printk(KERN_INFO "    len_encoded %x, pktlen_rng_match_vec %x, llc_snap %x, llc_type_enc %x\n",
				entry.len_encoded, entry.pktlen_rng_match_vec, entry.llc_snap, entry.llc_type_enc);

			printk(KERN_INFO "  VLAN:\n");
			printk(KERN_INFO "    vlan_cnt %x\n", entry.vlan_cnt);
			printk(KERN_INFO "    top_tpid_enc %x, top_vid %x, top_8021p %x, top_dei %x\n",
				entry.top_tpid_enc, entry.top_vid, entry.top_8021p, entry.top_dei);
			printk(KERN_INFO "    inner_tpid_enc %x, inner_vid %x, inner_8021p %x, inner_dei%x\n",
				entry.inner_tpid_enc, entry.inner_vid, entry.inner_8021p, entry.inner_dei);

			printk(KERN_INFO "  PPP / PPPoE:\n");
			printk(KERN_INFO "    pppoe_type %x, pppoe_code_enc %x, pppoe_session_id %x, ppp_protocol_enc %x\n",
				entry.pppoe_type, entry.pppoe_code_enc, entry.pppoe_session_id, entry.ppp_protocol_enc);

			printk(KERN_INFO "  L3:\n");
			printk(KERN_INFO "    ip_vld %x, ip_ver %x, ip_dscp %02x, ip_ecn %x\n",
				entry.ip_vld, entry.ip_ver, entry.ip_dscp, entry.ip_ecn);
			printk(KERN_INFO "    ip_protocol %x, ip_l4_type %x, \n", entry.ip_protocol, entry.ip_l4_type);
			printk(KERN_INFO "    ip_sa %03x, ip_da %03x\n", entry.ip_sa, entry.ip_da);
			printk(KERN_INFO "    ipv6_flow_lbl %x, ip_ttl %x, ip_options %x, ip_fragment_flag %x\n",
				entry.ipv6_flow_lbl, entry.ip_ttl, entry.ip_options, entry.ip_fragment_flag);
			printk(KERN_INFO "    ipv6_hbh %x, ipv6_rh %x, ipv6_doh %x\n", entry.ipv6_hbh, entry.ipv6_rh, entry.ipv6_doh);
			printk(KERN_INFO "    icmp_vld %x, icmp_type %x\n", entry.icmp_vld, entry.icmp_type);
			printk(KERN_INFO "    spi_vld %x, spi %x\n", entry.spi_vld, entry.spi);
			printk(KERN_INFO "    l3_chksum_err %x\n", entry.l3_chksum_err);

			printk(KERN_INFO "  L4:\n");
			printk(KERN_INFO "    l4_sp_exact_range %05x, l4_dp_exact_range %05x\n", entry.l4_sp_exact_range, entry.l4_dp_exact_range);
			printk(KERN_INFO "    tcp_rdp_ctrl %03x, l4_chksum_zero %x\n", entry.tcp_rdp_ctrl, entry.l4_chksum_zero);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");

			cond_resched();
		}
	}
}

static aal_table_t mask_tbl_table = {
	.table_id = AAL_TABLE_HASH_MASK_TBL,
	.table_name = "HashEngine Mask Table",
	.max_entry = HASH_MASK_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hash_mask_t),
	.access_reg = L3FE_FE_L3E_HS_MASK_MEM_ACCESS,
	.data_reg_num = 4,
	.op = {
		.entry_add = __mask_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = __mask_tbl_entry_del,
		.entry_set = __mask_tbl_entry_set,
		.entry_get = __mask_tbl_entry_get,
		.entry_get_by_idx = __mask_tbl_entry_get_by_idx,
		.table_flush = __mask_tbl_table_flush,
		.get_avail_count = __mask_tbl_get_avail_count,
		.entry_print = __mask_tbl_entry_print,
	},
};

int _hash_mask_tbl_init(void)
{
	int ret;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_MASK_TBL, &mask_tbl_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void _hash_mask_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_MASK_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_IND_X for Hash Table
 ***************************************************************************/

static aal_table_t hash_tbl_table;

#if defined(CONFIG_LUNA_G3_SERIES)
// RTK patch
ca_status_t aal_hash_action_update (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	aal_hash_key_t		*hash_key,
	CA_IN	aal_hash_action_t	*hash_action,
	CA_IN	aal_hash_tuple_type_e	hash_type,
	CA_IN	aal_hash_aging_e		hash_age,
	CA_IN	ca_uint32_t		mask_id,
	CA_IN	ca_uint32_t		profile_id,
	CA_OUT	ca_uint32_t		hash_idx) {

	unsigned int hash_index;
	ca_uint32_t act_grp;
	int ret;
	ca_status_t ret2;
	aal_hash_key_t *tmp_hash_key = NULL;
	aal_hash_mask_t mask_entry;
	hash_hash_action_entry_t hash_action_entry;

	// return if invalid
	if ((ret = aal_hashtbl_hash_key_get(hash_idx, &tmp_hash_key)) != AAL_E_OK) {
		DBG(printk(KERN_INFO "Can not get hash_key, index=%u.\n", hash_idx));
		return ret;
	}

	/* get action group */
	ret = hash_action_group(profile_id, hash_type, hash_action, &act_grp);
	if (ret != AAL_E_OK) {
		DBG(printk("%s:%d No action group found, error_code = %d\n", __func__, __LINE__, ret));
		ret2 = CA_E_NOT_SUPPORT;
		goto _error_hash_add;
	}

	/* set chk_msk_ptr to avoid double check fail */
	if (act_grp & ACTION_GROUP_19) {
		hash_action->chk_msk_ptr = mask_id;
		hash_action->cache_ctrl = (hash_type == TUPLE_TYPE_QOS) ? 2 : 1;
	}

	ret = convert_to_hash_action_entry(act_grp, hash_action, &hash_action_entry);
	if (ret != AAL_E_OK) {
		DBG(printk("%s:%d Convert to hash action entry fail, error_code = %d\n", __func__, __LINE__, ret));
		ret2 = CA_E_NOT_SUPPORT;
		goto _error_hash_add;
	}
	DBG(hash_action_dump(act_grp, &hash_action_entry));

//	ret = __mask_tbl_entry_get_by_idx(mask_id, &mask_entry);
	ret = aal_entry_get_by_idx(AAL_TABLE_HASH_MASK_TBL, mask_id, &mask_entry);
	if (ret != AAL_E_OK) {
		printk("%s:%d Fail to get mask id =%d, error_code = %d\n",
			__func__, __LINE__, mask_id, ret);
		return CA_E_ERROR;
	}

	DBG(__mask_tbl_entry_print(mask_id, 1));


	if (hash_idx & HASH_HASH_IDX_OVERFLOW_FLAG) {
		/* in Overflow Table */
		hash_index = hash_idx & HASH_HASH_IDX_MASK;

		/* update hash action to Overflow Action Table */
		ret = aal_entry_add_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, &hash_action_entry, hash_index);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_TBL, hash_index);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		overflow_action_table.entry[hash_index].local_data = act_grp;

	} else {
		/* in Hash Table */
		aal_hash_value_t hash_entry;
		memset((void *) &hash_entry, 0, sizeof(hash_entry));

		hash_index = hash_idx;
		ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_TBL, hash_index, (void *) &hash_entry);
		if(ret != AAL_E_OK) {
			printk("%s:%d __hash_tbl_entry_get_by_idx is failed. ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}

		/* update hash action to Hash Action Table */
		ret = aal_entry_add_by_idx(AAL_TABLE_HASH_HASH_ACTION, &hash_action_entry, hash_index);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_HASH_TBL, hash_index);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		hash_action_table.entry[hash_index].local_data = act_grp;

		if(cache_hash_enabled)
		{
			ret = aal_hash_cache_invalidate(hash_index, hash_entry.crc16);
			if (ret != AAL_E_OK)
				DBG(printk("%s:%d aal_hash_cache_invalidate() error_code=%d\n", __func__, __LINE__, ret));
		}

	}

	return CA_E_OK;
	
_error_hash_add:
	if (tmp_hash_key != NULL)
		ca_free_trace(tmp_hash_key);

	return ret2;
}
#endif



ca_status_t aal_hash_add (
CA_IN	ca_device_id_t		device_id,
CA_IN	aal_hash_key_t		*hash_key,
CA_IN	aal_hash_action_t	*hash_action,
CA_IN	aal_hash_tuple_type_e	hash_type,
CA_IN	aal_hash_aging_e	hash_age,
CA_IN	ca_uint32_t		mask_id,
CA_IN	ca_uint32_t		profile_id,
CA_OUT	ca_uint32_t		*hash_idx) {

	aal_hash_value_t hash_value;
	aal_hash_mask_t mask_entry;
	hash_hash_action_entry_t hash_action_entry;
	ca_uint32_t act_grp;
	aal_hash_key_t *tmp_hash_key = NULL;

	int ret;
	ca_status_t ret2;
	int overflow = false;

	if (hash_key == NULL || hash_action == NULL || hash_idx == NULL)
		return CA_E_ERROR;

	memset(&hash_value, 0, sizeof(hash_value));
	memset(&mask_entry, 0, sizeof(mask_entry));
	memset(&hash_action_entry, 0, sizeof(hash_action_entry));

	ret = aal_entry_get_by_idx(AAL_TABLE_HASH_MASK_TBL, mask_id, &mask_entry);
//	ret = __mask_tbl_entry_get_by_idx(mask_id, &mask_entry);
	//ret = __hash_hw_entry_get(&mask_tbl_table, mask_id, &mask_entry);
	if (ret != AAL_E_OK) {
		printk("%s:%d Fail to get mask id =%d, error_code = %d\n",
			__func__, __LINE__, mask_id, ret);
		return CA_E_ERROR;
	}

	DBG(__mask_tbl_entry_print(mask_id, 1));

	hash_key->ctrl_set_id = profile_id;
	hash_key->table_id = L3_HASH_ENGINE;
	/* calculate hash value */
	ret = hash_value_calculate(hash_key, &mask_entry, &hash_value);
	if (ret != AAL_E_OK) {
		printk("%s:%d error_code = %d\n", __func__, __LINE__, ret);
		return CA_E_ERROR;
	}

	DBG(printk("%s:%d hash_value.crc32 = 0x%x\n", __func__, __LINE__, hash_value.crc32));
	DBG(printk("%s:%d hash_value.crc16 = 0x%x\n", __func__, __LINE__, hash_value.crc16));
	DBG(printk("%s:%d hash_value.xor32 = 0x%x\n", __func__, __LINE__, hash_value.xor32));

	/* Adjust hash_action before adding anything */
	// Requirement for L3 TE policer
	// Note: this will overwrite user's setting
	if(ip_ecn_support){
		hash_action->ip_ecn_vld = 1;
		hash_action->ip_ecn_en = 1;
	}

	/* get action group */
	ret = hash_action_group(profile_id, hash_type, hash_action, &act_grp);
	if (ret != AAL_E_OK) {
		DBG(printk("%s:%d No action group found, error_code = %d\n", __func__, __LINE__, ret));
		ret2 = CA_E_NOT_SUPPORT;
		goto _error_hash_add;
	}

	/* set chk_msk_ptr to avoid double check fail */
	if (act_grp & ACTION_GROUP_19) {
		hash_action->chk_msk_ptr = mask_id;
		hash_action->cache_ctrl = (hash_type == TUPLE_TYPE_QOS) ? 2 : 1;
	}

	if (act_grp & ACTION_GROUP_20) {
		DBG(printk("%s:%d: Group20: ip_idx_vld=%d ip_idx: %d \n", __func__, __LINE__, hash_action->ip_idx_vld, hash_action->ip_idx));
	}

	if (act_grp & ACTION_GROUP_21) {
		hash_action->chk_hash_vld = 1;
		hash_action->chk_hash_val = ((hash_value.xor32 & 0xFFFF0000)>>16) | ((hash_value.xor32 & 0x0000FFFF)<<16);
		//hash_action->chk_hash_val = hash_value.xor32; 
		printk("%s:%d hash_value.xor32 = 0x%x\n", __func__, __LINE__, hash_action->chk_hash_val);
	}

	ret = convert_to_hash_action_entry(act_grp, hash_action, &hash_action_entry);
	if (ret != AAL_E_OK) {
		DBG(printk("%s:%d Convert to hash action entry fail, error_code = %d\n", __func__, __LINE__, ret));
		ret2 = CA_E_NOT_SUPPORT;
		goto _error_hash_add;
	}
	DBG(hash_action_dump(act_grp, &hash_action_entry));

#if UNIT_TEST
	//DBG(aal_hash_crc_sw_hw_calc_check(hash_key, mask_id, hash_value.crc32, hash_value.crc16));
#endif

	if ((tmp_hash_key = ca_malloc_trace(sizeof(aal_hash_key_t), GFP_ATOMIC, true)) == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory.\n", __func__);
		return AAL_E_NOMEM;
	}
	memcpy(tmp_hash_key, hash_key, sizeof(aal_hash_key_t));

	/* add hash value to Hash Table */
	ret = aal_entry_add(AAL_TABLE_HASH_HASH_TBL, &hash_value, hash_idx);
	if (ret != AAL_E_OK) {
		if (ret == AAL_E_TBLFULL) {
			/* add to Overflow Table */
			aal_overflow_hash_t overflow_hash;
			memset((void *)&overflow_hash, 0, sizeof(aal_overflow_hash_t));
			overflow_hash.crc32 = hash_value.crc32;
			overflow_hash.crc16 = hash_value.crc16;
			overflow_hash.profile = profile_id;
			overflow = true;
			ret = aal_entry_add(AAL_TABLE_HASH_OVERFLOW_TBL, (void *)&overflow_hash, hash_idx);
			if (ret != AAL_E_OK) {
				DBG(printk("%s:%d Overflow table error_code = %d\n", __func__, __LINE__, ret));
				if (ret == AAL_E_TBLFULL) {
					DBG(hash_key_dump(hash_key));
					ret2 = CA_E_FULL;
				} else if (ret == AAL_E_EXIST) {
					ret2 = CA_E_EXISTS;
					ret = aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, *hash_idx, hash_age);
					if (ret != AAL_E_OK)
						DBG(printk("%s:%d Fail to set age for overflow hash entry index %d\n", __func__, __LINE__, *hash_idx));
					*hash_idx = *hash_idx | HASH_HASH_IDX_OVERFLOW_FLAG;
				} else {
					/* unexpected error */
					ret2 = CA_E_ERROR;
				}
				goto _error_hash_add;
			}
		} else if (ret == AAL_E_EXIST) {
			DBG(printk("%s:%d Main Hash table CA_E_EXISTS error_code = %d\n", __func__, __LINE__, ret));
			ret2 = CA_E_EXISTS;
			ret = aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, *hash_idx, hash_age);
			if (ret != AAL_E_OK)
				DBG(printk("%s:%d Fail to set age for hash entry index %d\n", __func__, __LINE__, *hash_idx));
			goto _error_hash_add;
		} else {
			printk("%s:%d Main Hash table CA_E_ERROR error_code = %d\n", __func__, __LINE__, ret);
			/* unexpected error */
			ret2 = CA_E_ERROR;
			goto _error_hash_add;
		}
	}

	if (overflow == false) {

		/* add hash action to Hash Action Table */
		ret = aal_entry_add_by_idx(AAL_TABLE_HASH_HASH_ACTION, &hash_action_entry, *hash_idx);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_HASH_TBL, *hash_idx);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		hash_action_table.entry[*hash_idx].local_data = act_grp;

		/* set age to enable the hash entry in Hash Table */
		ret = aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, *hash_idx, hash_age);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_HASH_ACTION, *hash_idx);
			aal_entry_del(AAL_TABLE_HASH_HASH_TBL, *hash_idx);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		ret = aal_hashtbl_hash_key_set(*hash_idx, tmp_hash_key);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set hash_key %p to hash index %u. (ret %d)\n", __func__, tmp_hash_key, *hash_idx, ret);
			aal_entry_del(AAL_TABLE_HASH_HASH_ACTION, *hash_idx);
			aal_entry_del(AAL_TABLE_HASH_HASH_TBL, *hash_idx);
			goto _error_hash_add;
		}

	} else { /* overflow case */

		/* add hash action to Overflow Action Table */
		ret = aal_entry_add_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, &hash_action_entry, *hash_idx);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_TBL, *hash_idx);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		overflow_action_table.entry[*hash_idx].local_data = act_grp;

		/* set age to enable the hash entry in Overflow Table */
		ret = aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, *hash_idx, hash_age);
		if (ret != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_ACTION, *hash_idx);
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_TBL, *hash_idx);
			ret2 = CA_E_INTERNAL;
			goto _error_hash_add;
		}

		*hash_idx = *hash_idx | HASH_HASH_IDX_OVERFLOW_FLAG;

		ret = aal_hashtbl_hash_key_set(*hash_idx, tmp_hash_key);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Fail to set hash_key %p to hash index %u. (ret %d)\n", __func__, tmp_hash_key, *hash_idx, ret);
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_ACTION, *hash_idx);
			aal_entry_del(AAL_TABLE_HASH_OVERFLOW_TBL, *hash_idx);
			goto _error_hash_add;
		}
	}

	if (overflow == true) {
		DBG(printk(KERN_INFO "Hash is added to Overflow Table / Overflow Action Table, index=%u.\n", *hash_idx));
	} else {
		DBG(printk(KERN_INFO "Hash is added to Hash Table / Action Table, index=%u.\n", *hash_idx));
	}

	return CA_E_OK;

_error_hash_add:
	if (tmp_hash_key != NULL)
		ca_free_trace(tmp_hash_key);

	return ret2;
}

ca_status_t aal_hash_delete (
CA_IN	ca_device_id_t		device_id,
CA_IN	ca_uint32_t		hash_idx){

	unsigned int hash_index;
	int ret;
	aal_hash_key_t *tmp_hash_key = NULL;

	if ((ret = aal_hashtbl_hash_key_get(hash_idx, &tmp_hash_key)) != AAL_E_OK) {
		DBG(printk(KERN_INFO "Can not get hash_key, index=%u.\n", hash_idx));
	}
	if (tmp_hash_key != NULL) {
		ca_free_trace(tmp_hash_key);
		aal_hashtbl_hash_key_set(hash_idx, NULL);
	}

	/* G3_hash 3.3.2 Hash Entry Removal
		(1) Write Main Hash table directly, clearing the 32-bit field to zeros
			Main Action entry is don't-care.
		(2) Invalidate the cache entry */

	if (hash_idx & HASH_HASH_IDX_OVERFLOW_FLAG) {
		/* in Overflow Table */
		hash_index = hash_idx & HASH_HASH_IDX_MASK;

		ret = aal_entry_del(AAL_TABLE_HASH_OVERFLOW_ACTION, hash_index);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_entry_del is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

		ret = aal_entry_del(AAL_TABLE_HASH_OVERFLOW_TBL, hash_index);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_entry_del is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

		ret = aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, hash_index, HASH_AGING_INVALID);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_hash_age_set is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

	} else {
		/* in Hash Table */
		aal_hash_value_t hash_entry;
		memset((void *) &hash_entry, 0, sizeof(hash_entry));

		hash_index = hash_idx;

		ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_TBL, hash_index, (void *) &hash_entry);
		if(ret == AAL_E_ENTRYNOTFOUND) {
			printk("%s:%d __hash_tbl_entry_get_by_idx is failed. ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}

		ret = aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, hash_index, HASH_AGING_INVALID);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_hash_age_set is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

		ret = aal_entry_del(AAL_TABLE_HASH_HASH_ACTION, hash_index);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_entry_del is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

		ret = aal_entry_del(AAL_TABLE_HASH_HASH_TBL, hash_index);
		if (ret != AAL_E_OK) {
			printk("%s:%d aal_entry_del is failed. ret=%d\n", __func__, __LINE__, ret);
			return CA_E_ERROR;
		}

		if( cache_hash_enabled ) {
			ret = aal_hash_cache_invalidate(hash_index, hash_entry.crc16);
			if (ret != AAL_E_OK)
				DBG(printk("%s:%d aal_hash_cache_invalidate() error_code=%d\n", __func__, __LINE__, ret));
		}

	}

	return CA_E_OK;

}

/* Set address of hash key to AAL local_data.
 * Parameter:
 *   hash_idx: It represents the hash_key is in Hash Table or Overflow Table.
 *   hash_key: Pointer of hash key.
 */
int aal_hashtbl_hash_key_set(unsigned int hash_idx, aal_hash_key_t *hash_key)
{
	aal_table_t *table = NULL;
	unsigned int idx = hash_idx & HASH_HASH_IDX_MASK;

	table = (hash_idx & HASH_HASH_IDX_OVERFLOW_FLAG) ? &overflow_tbl_table : &hash_tbl_table;

	if (table->entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_ENTRYNOTFOUND;

	table->entry[idx].local_data = (unsigned long) hash_key;

	return AAL_E_OK;
}

/* Get hash key from AAL local_data.
 * Parameter:
 *   hash_idx: It represents the hash_key is in Hash Table or Overflow Table.
 *   hash_key: Returned hash key.
 */
int aal_hashtbl_hash_key_get(unsigned int hash_idx, aal_hash_key_t **hash_key)
{
	aal_table_t *table = NULL;
	unsigned int idx = hash_idx & HASH_HASH_IDX_MASK;

	table = (hash_idx & HASH_HASH_IDX_OVERFLOW_FLAG) ? &overflow_tbl_table : &hash_tbl_table;

	if (hash_key == NULL) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (table->entry[idx].data == NULL) return AAL_E_ENTRYNOTFOUND;

	*hash_key = (aal_hash_key_t *) table->entry[idx].local_data;

	return AAL_E_OK;
}

static ca_status_t aal_hash_age_sync(
CA_IN	ca_device_id_t		device_id,
CA_IN	aal_table_e		table_id,
CA_IN	ca_uint32_t		start,
CA_IN	ca_uint32_t		end)
{
	unsigned int age, time;
	int i, ret;

	if(table_id != AAL_TABLE_HASH_HASH_TBL &&
		table_id != AAL_TABLE_HASH_OVERFLOW_TBL)
		return CA_E_ERROR;

	for (i = start; i <= end; i++) {
		ret = aal_hash_age_get(table_id, i, &age, &time);
		if(ret == AAL_E_OK) {
			if(age == HASH_AGING_INVALID) {
				if(table_id == AAL_TABLE_HASH_HASH_TBL)
					aal_hash_delete(0, i);
				else /* overflow table*/
					aal_hash_delete(0, i & HASH_HASH_IDX_OVERFLOW_FLAG);
			}
		}
	}

	return CA_E_OK;
}

static int __hash_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_tbl_table;
	unsigned int start, end;
	u16 mask = crc16_mask[g_hb_size - 2][g_hw_ht_size];
	unsigned int bucket_size = (1 << g_hb_size);
	aal_hash_value_t *orig_data;
	aal_hash_value_t *new_data = entry;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	/* If bucket size = 32 (2^5), start = crc16[15:5] */
	start = new_data->crc16 & mask;
	/* adjust start point according to hash table size */
	/*	Assume bucket size = 32 (2^5)
	*	ht_size:
	*	0: 512-entry => start = crc16[15:12]
	*	1: 1K-entry => start = crc16[15:11]
	*	2: 2K-entry => start = crc16[15:10]
	*	3: 4K-entry => start = crc16[15:9]
	*	4: 8K-entry => start = crc16[15:8]
	*	5: 16K-entry => start = crc16[15:7]
	*	6: 32K-entry => start = crc16[15:6]
	*	7: 64K-entry (only support in G3) => start = crc16[15:5]
	*/
	start = start >> (7 - g_hw_ht_size);
	end = start + bucket_size - 1;

	DBG(printk("%s:%d: start = 0x%x, end = 0x%x, bucket_size = %d, mask = 0x%x\n",
				__func__, __LINE__, start, end, bucket_size, mask));

	spin_lock_irqsave(&g_hash_lock, flags);

	/* Search the bucket to see if entry is added already.
	 * Only compare the crc32 value because HW only stores crc32.
	 */
	for (i = start; i <= end; i++) {
		orig_data = table->entry[i].data;
		if ((orig_data != NULL) && (orig_data->crc32 == new_data->crc32)) {
			*rslt_idx = i; /* return the existing hash entry index */
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_EXIST;
		}
	}

	/* search empty entry to add */
	for (i = start; i <= end; i++) {
		if (table->entry[i].data == NULL) {
			table->entry[i].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
			memcpy(table->entry[i].data, entry, table->entry_size);	// TODO: should only copy crc32?
			atomic_set(&table->entry[i].refcnt, 1);
			table->used_entry++;
			*rslt_idx = i;

			DBG(printk("%s:%d: crc32 = 0x%08x, crc16 = 0x%04x, rslt_idx = %d\n",
				__func__, __LINE__, new_data->crc32, new_data->crc16, *rslt_idx));
#if UNIT_TEST
			//printk("%u, %u, %u \n", new_data->crc32,
			//	new_data->crc16, *rslt_idx);
#endif
			/* set to HASH TABLE in DDR  */
			ret = _hash_ddr_entry_set(table, i);

			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_TBLFULL;
}

static int __hash_tbl_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear entry in DDR */
		if ((ret = _hash_ddr_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_tbl_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &hash_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	// TODO: should check crc16?
	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HASH TABLE in DDR */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_tbl_table;
	unsigned int start, end;
	u16 mask = crc16_mask[g_hb_size - 2][g_hw_ht_size];
	unsigned int bucket_size = (1 << g_hb_size);
	aal_hash_value_t *iter_data;
	aal_hash_value_t *data = entry;
	int i;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	start = data->crc16 & mask;
	start = start >> (7 - g_hw_ht_size);
	end = start + bucket_size - 1;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = start; i <= end; i++) {
		iter_data = table->entry[i].data;
		if ((iter_data != NULL) && (iter_data->crc32 == data->crc32)) {
			*rslt_idx = i;
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hash_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &hash_tbl_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_tbl_table_flush(void)
{
	int i, ret;

	for (i = 0; i < hash_tbl_table.max_entry; i++) {
		ret = __hash_tbl_entry_del(i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hash_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&hash_tbl_table);
}

static void __hash_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &hash_tbl_table;
	aal_hash_value_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;
	aal_hash_key_t *hash_key = NULL;
	unsigned int age, time_sec;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== Hash Hash Table (empty means entry is unused) ======================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		/* get aging of each entry */
		ret = aal_hash_age_get(table->table_id, i, &age, &time_sec);
		if (ret != AAL_E_OK) {
			printk(KERN_INFO "ERROR! Fail to get aging of hash_idx %d. (ret=%d)\n", i, ret);
		}

		/* get each hash entry */
		ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_TBL, i, (void *) &entry);
		if (!ca_aal_table_sw_cache && ret == AAL_E_OK) { /* Get CRC32 value in DDR */
			ret = _hash_ddr_entry_get(&hash_tbl_table, i, (unsigned int *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			/* Don't print anything */
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid %d, crc16 0x%04x, crc32 0x%08x\n", entry.valid, entry.crc16, entry.crc32);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "age %d [%s], approximate %u seconds\n",
				age,
				(age == HASH_AGING_STATIC) ? "STATIC" : ((age == HASH_AGING_INVALID) ? "EXPIRED" : "AGING"),
				time_sec);

			hash_key = (aal_hash_key_t *) table->entry[i].local_data;
			if (hash_key != NULL)
				hash_key_dump(hash_key);
			else
				printk(KERN_INFO "\nLocal data of this entry does not contain hash key.\n");

			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		}
		cond_resched();
	}
}

static aal_table_t hash_tbl_table = {
	.table_id = AAL_TABLE_HASH_HASH_TBL,
	.table_name = "HashEngine Hash Table",
	.max_entry = HASH_HASH_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_hash_value_t),
	.data_reg_num = 1,
	.op = {
		.entry_add = __hash_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = __hash_tbl_entry_del,
		.entry_set = __hash_tbl_entry_set,
		.entry_get = __hash_tbl_entry_get,
		.entry_get_by_idx = __hash_tbl_entry_get_by_idx,
		.table_flush = __hash_tbl_table_flush,
		.get_avail_count = __hash_tbl_get_avail_count,
		.entry_print = __hash_tbl_entry_print,
	},
};

int _l3fe_main_hash_info_get(void){

	memset(&l3fe_main_hash_info, 0, sizeof(aal_l3qm_l3fe_main_hash_info_t));

	aal_l3qm_get_l3fe_main_hash_action_info(&l3fe_main_hash_info);

	hash_overflow_hash_fib_virtual_addr = l3fe_main_hash_info.overflow_action_start_virt_addr;
	hash_overflow_hash_fib_phy_addr = l3fe_main_hash_info.overflow_action_start_phy_addr;
	hash_overflow_hash_fib_length = l3fe_main_hash_info.overflow_hash_fib_length;
	hash_overflow_hash_fib_count = l3fe_main_hash_info.overflow_hash_fib_count;

	hash_default_hash_fib_virtual_addr = l3fe_main_hash_info.default_action_start_virt_addr;
	hash_default_hash_fib_phy_addr = l3fe_main_hash_info.default_action_start_phy_addr;
	hash_default_hash_fib_length = l3fe_main_hash_info.default_hash_fib_length;
	hash_default_hash_fib_count = l3fe_main_hash_info.default_hash_fib_count;

	hash_cache_fib_virtual_addr = l3fe_main_hash_info.action_cache_start_virt_addr;
	hash_cache_fib_phy_addr = l3fe_main_hash_info.action_cache_start_phy_addr;
	hash_cache_fib_length = l3fe_main_hash_info.action_cache_fib_length;
	hash_cache_fib_count = l3fe_main_hash_info.action_cache_fib_count;

	hash_table_virtual_addr = l3fe_main_hash_info.hash_table_start_virt_addr;
	hash_table_phy_addr = l3fe_main_hash_info.hash_table_start_phy_addr;
	hash_table_entry_length = l3fe_main_hash_info.table_entry_length;
	hash_table_entry_count = l3fe_main_hash_info.table_entry_count;

	hash_action_table_virtual_addr = l3fe_main_hash_info.action_table_start_virt_addr;
	hash_action_table_phy_addr = l3fe_main_hash_info.action_table_start_phy_addr;
	hash_action_table_entry_length = l3fe_main_hash_info.action_table_entry_length;
	hash_action_table_entry_count = l3fe_main_hash_info.action_table_entry_count;

	return AAL_E_OK;
}

unsigned int _cache_table_size_get(unsigned int count) {
	int i, array_max = 5;

	for (i = 0; i < array_max; i++) {
		if(ct_size_array[i] == count) {
			return i;
		}
	}

	if( i == array_max) {
		printk("%s:%d Hash Action count from scfg file is WRONG value %d\n",
				__func__, __LINE__, count);
		BUG();
	}
	return (-1);
}

unsigned int _hash_action_width_get(unsigned int length) {
	int i, array_max = 8;

	for (i = 0; i < array_max; i++) {
		if(ha_width_array[i] == length) {
			return i;
		}
	}

	if( i == array_max) {
		printk("%s:%d Hash Action width from scfg file is WRONG value %d\n",
				__func__, __LINE__, length);
		BUG();
	}
	return (-1);
}

unsigned int _hash_bucket_size_get(void) {
	ca_uint32_t bucket_size = 0;
	ca_status_t ret;

	ret = aal_scfg_read(CFG_ID_MAIN_HASH_BUCKET_SIZE, 1, &bucket_size);

	if( ret == CA_E_OK) {
		/* enum hash_hb_size {
			HASH_HB_SIZE_4 = 2,
			HASH_HB_SIZE_8 = 3,
			HASH_HB_SIZE_16 = 4,
			HASH_HB_SIZE_32 = 5,
		}; */
		int i = 0;

		while(bucket_size >>= 1) { i++; } /* calc pow(bucket_size, 2) */

		return (i & 0x7);
	}
	else {
		printk("%s:%d Hash bucket size from scfg file is WRONG value %d\n",
				__func__, __LINE__, bucket_size);
		return HASH_HB_SIZE_8;
	}
}

unsigned int aal_hash_axi_read_write_outtrans_num_set(
	ca_uint8_t axi_read_outtrans_num,
	ca_uint8_t axi_write_outtrans_num)
{
	L3FE_AXIM2_CONFIG_t config;

	config.wrd = CA_NE_REG_READ(L3FE_AXIM2_CONFIG);

	config.bf.axi_write_outtrans_num = axi_write_outtrans_num & 0xf;
	config.bf.axi_read_outtrans_num = axi_read_outtrans_num & 0xf;

	CA_NE_REG_WRITE(config.wrd, L3FE_AXIM2_CONFIG);

	return AAL_E_OK;
}

aal_hash_dbl_chk_fail_ctrl_t aal_hash_dbl_chk_fail_ctrl_type0 = {
	.dpid = AAL_LPORT_CPU_0,
	.dpid_vld = 1,
	.bypass_next = 1,
};

aal_hash_dbl_chk_fail_ctrl_t aal_hash_dbl_chk_fail_ctrl_type1= {
	.dpid = AAL_LPORT_CPU_0,
	.dpid_vld = 1,
	.bypass_next = 1,
};

unsigned int aal_hash_dbl_chk_fail_ctrl_set(void)
{
	L3FE_FE_L3E_HS_CHK_FAIL_CTRL_t config;

	memset((void *) &config, 0, sizeof(L3FE_FE_L3E_HS_CHK_FAIL_CTRL_t));

	config.bf.type0 = *(ca_uint8_t *) &aal_hash_dbl_chk_fail_ctrl_type0;
	config.bf.type1 = *(ca_uint8_t *) &aal_hash_dbl_chk_fail_ctrl_type1;

	CA_NE_REG_WRITE(config.wrd, L3FE_FE_L3E_HS_CHK_FAIL_CTRL);

	return AAL_E_OK;
}

#define SET_BIT(x, n) ( (x) |= (1<< (n)) )
#define CLR_BIT(x, n) ( (x) &= (~(1<< (n))))

unsigned int aal_hash_hw_patch_set(void)
{
	L3FE_FE_L3E_HS_RSV0_t hs_rsv0;
	L3FE_FE_L3E_HS_RSV1_t hs_rsv1;
	L3FE_GLB_TE_OPTION_t te_option;

	memset((void *) &hs_rsv0, 0, sizeof(L3FE_FE_L3E_HS_RSV0_t));
	memset((void *) &hs_rsv1, 0, sizeof(L3FE_FE_L3E_HS_RSV1_t));
	memset((void *) &te_option, 0, sizeof(L3FE_GLB_TE_OPTION_t));

	hs_rsv0.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_RSV0);
	SET_BIT(hs_rsv0.bf.rsv, 31); // [31]: crc_offload patch
	SET_BIT(hs_rsv0.bf.rsv, 0); // [2:0]: he_wrapper_dmar_rreq.stg1_rreq_req (to avoid axi deadlock)
	CA_NE_REG_WRITE(hs_rsv0.wrd, L3FE_FE_L3E_HS_RSV0);

	SET_BIT(hs_rsv1.bf.rsv, 0); // [0]: extended control of he_wrapper_dmar_rreq.x_limit_id_1
	CA_NE_REG_WRITE(hs_rsv1.wrd, L3FE_FE_L3E_HS_RSV1);

#if defined(CONFIG_LUNA_G3_SERIES)
	// do nothing
#else
	te_option.wrd = CA_NE_REG_READ(L3FE_GLB_TE_OPTION);

	SET_BIT(te_option.bf.bits, 5); //for TCP checksum, if CS = 0 after incremental update, it will be changed to 16'hFFFF

	SET_BIT(te_option.bf.bits, 6); //for IPv4 checksum, if CS = 0 after incremental update, it will be changed to 16'hFFFF

	CA_NE_REG_WRITE(te_option.wrd, L3FE_GLB_TE_OPTION);
#endif

	return AAL_E_OK;
}


int _hash_tbl_init(void)
{
	int ret;
	unsigned int val = 0, ct_size;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_HASH_TBL, &hash_tbl_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	_l3fe_main_hash_info_get();

	val = hash_table_phy_addr;

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_MH0);

	//val = 0x1; /* for ACE */

	//CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_MH1);

	/* ht_size */
	switch(hash_table_entry_count) {
	case HASH_HT_SIZE_512:
	case HASH_HT_SIZE_1K:
	case HASH_HT_SIZE_2K:
	case HASH_HT_SIZE_4K:
	case HASH_HT_SIZE_8K:
	case HASH_HT_SIZE_16K:
	case HASH_HT_SIZE_32K:
	case HASH_HT_SIZE_64K:
		printk("%s:%d Hash Table size from scfg file is %d\n",
				__func__, __LINE__, hash_table_entry_count);
		break;
	default:
		printk("%s:%d Hash Table size from scfg file is WRONG value %d\n",
				__func__, __LINE__, hash_table_entry_count);
		BUG();
		break;
	}

	aal_hash_hash_table_size_set(hash_table_entry_count, _hash_bucket_size_get(),
			_hash_action_width_get(hash_action_table_entry_length));

	ct_size = l3fe_main_hash_info.main_hash_bm_test ? _cache_table_size_get(hash_cache_fib_count) : HASH_CT_SIZE_2K;
	aal_hash_cache_init(ct_size, _hash_action_width_get(hash_cache_fib_length));
	aal_hash_default_action_da_width_set(_hash_action_width_get(hash_default_hash_fib_length));
	aal_hash_overflow_action_oa_width_set(_hash_action_width_get(hash_overflow_hash_fib_length));
	aal_hash_cache_action_init();

	aal_hash_axi_read_write_outtrans_num_set(axi_read_outtrans_num, axi_write_outtrans_num);

	aal_hash_dbl_chk_fail_ctrl_set();

	aal_hash_hw_patch_set();

	aal_scfg_read(CFG_ID_IP_ECN_SUPPORT, 1, &ip_ecn_support);

	aal_scfg_read(CFG_ID_FLOW_SUPPORT, 1, &flow_support);

	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_INDEX_MODE, 1, &l3fe_hash_indexed_mode);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_KEY_SELECTION, HASH_PROFILE_MAX*sizeof(ca_uint32_t), &hash_key_select);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TPL_SP, HASH_PROFILE_MAX*sizeof(ca_uint32_t), &hash_tuple_sp_cfg);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TPL_DP, HASH_PROFILE_MAX*sizeof(ca_uint32_t), &hash_tuple_dp_cfg);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TPL_SIP, HASH_PROFILE_MAX*sizeof(ca_uint32_t), &hash_tuple_sip_cfg);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TPL_DIP, HASH_PROFILE_MAX*sizeof(ca_uint32_t), &hash_tuple_dip_cfg);
	return AAL_E_OK;
}

void _hash_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_HASH_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_IND_X for Overflow Table
 ***************************************************************************/

static aal_table_t overflow_tbl_table;

static int __overflow_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_tbl_table;
	int i, ret;
	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
			*rslt_idx = i; /* return the existing hash entry index */
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_EXIST;
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
			ret = _hash_hw_entry_set(table, i);

			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_TBLFULL;
}

static int __overflow_tbl_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear HW */
		if ((ret = __hash_hw_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_tbl_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &overflow_tbl_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = _hash_hw_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_tbl_table;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __overflow_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &overflow_tbl_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_tbl_table_flush(void)
{
	int i, ret;

	for (i = 0; i < overflow_tbl_table.max_entry; i++) {
		ret = __overflow_tbl_entry_del(i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __overflow_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&overflow_tbl_table);
}

static void __overflow_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &overflow_tbl_table;
	aal_overflow_hash_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;
	aal_hash_key_t *hash_key = NULL;
	unsigned int age, time_sec;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry)) {
		end_idx = table->max_entry;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== Hash Overflow Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		/* get aging of each entry */
		ret = aal_hash_age_get(table->table_id, i, &age, &time_sec);
		if (ret != AAL_E_OK) {
			printk(KERN_INFO "ERROR! Fail to get aging of hash_idx %d. (ret=%d)\n", i, ret);
		}

		/* get each hash entry */
		if (ca_aal_table_sw_cache) {
			ret = __overflow_tbl_entry_get_by_idx(i, (void *) &entry);
		} else {
			ret = __hash_hw_entry_get(&overflow_tbl_table, i, (void *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "profile %d, crc16 0x%04x, crc32 0x%08x\n", entry.profile, entry.crc16, entry.crc32);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "age %d [%s], approximate %u seconds\n",
				age,
				(age == HASH_AGING_STATIC) ? "STATIC" : ((age == HASH_AGING_INVALID) ? "EXPIRED" : "AGING"),
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

static aal_table_t overflow_tbl_table = {
	.table_id = AAL_TABLE_HASH_OVERFLOW_TBL,
	.table_name = "HashEngine Overflow Table",
	.max_entry = HASH_OVERFLOW_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(aal_overflow_hash_t),
	.access_reg = L3FE_FE_L3E_HS_OVERFLOW_HASH_ACCESS,
	.data_reg_num = 2,
	.op = {
		.entry_add = __overflow_tbl_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = __overflow_tbl_entry_del,
		.entry_set = __overflow_tbl_entry_set,
		.entry_get = __overflow_tbl_entry_get,
		.entry_get_by_idx = __overflow_tbl_entry_get_by_idx,
		.table_flush = __overflow_tbl_table_flush,
		.get_avail_count = __overflow_tbl_get_avail_count,
		.entry_print = __overflow_tbl_entry_print,
	},
};

int _hash_overflow_tbl_init(void)
{
	int ret;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_OVERFLOW_TBL, &overflow_tbl_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void _hash_overflow_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_OVERFLOW_TBL);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_IND_X for Hash Action
 ***************************************************************************/

static int __hash_action_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
			*rslt_idx = i; /* return the existing hash action entry index */
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_EXIST;
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
			ret = _hash_ddr_entry_set(table, i);

			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return ret;
}

static int __hash_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (table->entry[idx].data != NULL) {
#if defined(CONFIG_LUNA_G3_SERIES)	
		// support entry update
		table->used_entry--;
		goto ENTRY_UPDATE;
#else
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_ENTRY_USED;
#endif
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
#if defined(CONFIG_LUNA_G3_SERIES)	
ENTRY_UPDATE:
#endif
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_action_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;
	hash_hash_action_entry_t entry;
	int ret;
	unsigned int act_grp, ip_idx_vld = 0, mac_da_idx_vld = 0, ip_idx, mac_da_idx;
	aal_hash_value_t hash_entry;
	hash_hash_action_entry_t hash_action_entry;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	if (l3fe_hash_indexed_mode) {
		ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_TBL, idx, &hash_entry);
		if(ret != AAL_E_OK) {
			printk("%s:%d __hash_tbl_entry_get_by_idx is failed. ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
		act_grp = table->entry[idx].local_data;

		if (act_grp == ACTGRP_NAT_TO_WAN_1) {
			hash_action_to_wan_1_t *ptr = (hash_action_to_wan_1_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_NAT_TO_LAN_1) {
			hash_action_to_lan_1_t *ptr = (hash_action_to_lan_1_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_SHORT) {
			hash_action_flow_short_t *ptr = (hash_action_flow_short_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
#if defined (CONFIG_LUNA_G3_SERIES)
		} else if (act_grp == ACTGRP_FLOW_RTK_DS) {
			hash_action_flow_rtk_ds_t *ptr = (hash_action_flow_rtk_ds_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_RTK_US) {
			hash_action_flow_rtk_us_t *ptr = (hash_action_flow_rtk_us_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_RTK_MC) {
			hash_action_flow_rtk_mc_t *ptr = (hash_action_flow_rtk_mc_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_HASH_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
#endif
		} else {
			return AAL_E_ENTRYNOTFOUND;
		}

		if (ip_idx_vld) {
			if (hash_entry.ip_ver == CA_IPV4)
				aal_entry_del(AAL_TABLE_IDX_INTF_IPV4, ip_idx);
			else if (hash_entry.ip_ver == CA_IPV6)
				aal_entry_del(AAL_TABLE_IDX_INTF_IPV6, ip_idx);
		}
		if (mac_da_idx_vld) {
			aal_entry_del(AAL_TABLE_IDX_INTF_MAC, mac_da_idx);
		}
	}
	spin_lock_irqsave(&g_hash_lock, flags);
	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear entry in DDR */
		if ((ret = _hash_ddr_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}
	spin_unlock_irqrestore(&g_hash_lock, flags);
	return AAL_E_OK;
}

static int __hash_action_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __hash_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &hash_action_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __hash_action_table_flush(void)
{
	int i, ret;

	for (i = 0; i < hash_action_table.max_entry; i++) {
		ret = __hash_action_entry_del(i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __hash_action_get_avail_count(void)
{
	return generic_get_avail_count(&hash_action_table);
}

static void __hash_action_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &hash_action_table;
	hash_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash Action Table (empty means entry is unused) =======================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __hash_action_entry_get_by_idx(i, (void *) &entry);
		} else {
			ret = _hash_ddr_entry_get(&hash_action_table, i, (unsigned int *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			/* Don't print anything */
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					entry.reserved_7, entry.reserved_6, entry.reserved_5, entry.reserved_4,
					entry.reserved_3, entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hash_action_dump((unsigned int) table->entry[i].local_data, &entry);

			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		}
		cond_resched();
	}
}

static aal_table_t hash_action_table = {
	.table_id = AAL_TABLE_HASH_HASH_ACTION,
	.table_name = "HashEngine Hash Action Table",
	.max_entry = HASH_HASH_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(hash_hash_action_entry_t),
	.data_reg_num = 6,
	.op = {
		.entry_add = __hash_action_entry_add,
		.entry_add_by_idx = __hash_action_entry_add_by_idx,
		.entry_del = __hash_action_entry_del,
		.entry_set = __hash_action_entry_set,
		.entry_get = __hash_action_entry_get,
		.entry_get_by_idx = __hash_action_entry_get_by_idx,
		.table_flush = __hash_action_table_flush,
		.get_avail_count = __hash_action_get_avail_count,
		.entry_print = __hash_action_entry_print,
	},
};

int _hash_action_init(void)
{
	int ret;
	unsigned int val = 0;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_HASH_ACTION, &hash_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	val = hash_action_table_phy_addr;

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_MA0);

	//val = 0x1; /* for ACE */

	//CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_MA1);

	return AAL_E_OK;
}

void _hash_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_HASH_ACTION);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_IND_X for Overflow Action
 ***************************************************************************/

static int __overflow_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (table->entry[idx].data != NULL) {
#if defined(CONFIG_LUNA_G3_SERIES)	
		// support entry update
		table->used_entry--;
		goto ENTRY_UPDATE;
#else
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_ENTRY_USED;
#endif
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
#if defined(CONFIG_LUNA_G3_SERIES)	
ENTRY_UPDATE:
#endif
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_action_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_action_table;
	int ret;
	aal_hash_value_t hash_entry;
	hash_hash_action_entry_t hash_action_entry;
	unsigned int act_grp, ip_idx_vld = 0, mac_da_idx_vld = 0, ip_idx, mac_da_idx;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;


	if (l3fe_hash_indexed_mode) {
		ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_TBL, idx, &hash_entry);
		if(ret != AAL_E_OK) {
			printk("%s:%d __hash_tbl_entry_get_by_idx is failed. ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
		act_grp = table->entry[idx].local_data;

		if (act_grp == ACTGRP_NAT_TO_WAN_1) {
			hash_action_to_wan_1_t *ptr = (hash_action_to_wan_1_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_NAT_TO_LAN_1) {
			hash_action_to_lan_1_t *ptr = (hash_action_to_lan_1_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_SHORT) {
			hash_action_flow_short_t *ptr = (hash_action_flow_short_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
#if defined (CONFIG_LUNA_G3_SERIES)
		} else if (act_grp == ACTGRP_FLOW_RTK_DS) {
			hash_action_flow_rtk_ds_t *ptr = (hash_action_flow_rtk_ds_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_RTK_US) {
			hash_action_flow_rtk_us_t *ptr = (hash_action_flow_rtk_us_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
		} else if (act_grp == ACTGRP_FLOW_RTK_MC) {
			hash_action_flow_rtk_mc_t *ptr = (hash_action_flow_rtk_mc_t *)&hash_action_entry;
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_OVERFLOW_ACTION, idx, (void *)&hash_action_entry);
			ip_idx_vld = ptr->ip_idx_vld;
			mac_da_idx_vld = ptr->mac_da_idx_vld;
			ip_idx = ptr->ip_idx;
			mac_da_idx = ptr->mac_da_idx;
#endif
		} else {
			return AAL_E_ENTRYNOTFOUND;
		}

		if (ip_idx_vld) {
			if (hash_entry.ip_ver == CA_IPV4)
				aal_entry_del(AAL_TABLE_IDX_INTF_IPV4, ip_idx);
			else if (hash_entry.ip_ver == CA_IPV6)
				aal_entry_del(AAL_TABLE_IDX_INTF_IPV6, ip_idx);
		}
		if (mac_da_idx_vld) {
			aal_entry_del(AAL_TABLE_IDX_INTF_MAC, mac_da_idx);
		}
	}

	spin_lock_irqsave(&g_hash_lock, flags);
	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear entry in DDR */
		if ((ret = _hash_ddr_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_action_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &overflow_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &overflow_action_table;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __overflow_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &overflow_action_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __overflow_action_table_flush(void)
{
	int i, ret;

	for (i = 0; i < overflow_action_table.max_entry; i++) {
		ret = __overflow_action_entry_del(i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __overflow_action_get_avail_count(void)
{
	return generic_get_avail_count(&overflow_action_table);
}

static void __overflow_action_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &overflow_action_table;
	hash_overflow_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash Overflow Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = __overflow_action_entry_get_by_idx(i, (void *) &entry);
		} else {
			ret = _hash_ddr_entry_get(&overflow_action_table, i, (unsigned int *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					entry.reserved_7, entry.reserved_6, entry.reserved_5, entry.reserved_4,
					entry.reserved_3, entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hash_action_dump((unsigned int) table->entry[i].local_data, (hash_hash_action_entry_t *)&entry);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static aal_table_t overflow_action_table = {
	.table_id = AAL_TABLE_HASH_OVERFLOW_ACTION,
	.table_name = "HashEngine Overflow Action Table",
	.max_entry = HASH_OVERFLOW_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(hash_overflow_action_entry_t),
	.data_reg_num = 6,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = __overflow_action_entry_add_by_idx,
		.entry_del = __overflow_action_entry_del,
		.entry_set = __overflow_action_entry_set,
		.entry_get = __overflow_action_entry_get,
		.entry_get_by_idx = __overflow_action_entry_get_by_idx,
		.table_flush = __overflow_action_table_flush,
		.get_avail_count = __overflow_action_get_avail_count,
		.entry_print = __overflow_action_entry_print,
	},
};

int _hash_overflow_action_init(void)
{
	int ret;
	unsigned int val = 0;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_OVERFLOW_ACTION, &overflow_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	val = hash_overflow_hash_fib_phy_addr;

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_OA0);

	//val = 0x1; /* for ACE */

	//CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_OA1);

	return AAL_E_OK;
}

void _hash_overflow_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_OVERFLOW_ACTION);
}

/***************************************************************************
 * AAL table, FE_L3E_HS_IND_X for Default Action
 ***************************************************************************/
static int __default_action_entry_add(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;
	int i, ret;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	/* Search the bucket to see if entry is added already */
	for (i = 0; i < table->max_entry; i++) {
		if ( (table->entry[i].data != NULL) && (!memcmp(table->entry[i].data, entry, table->entry_size)) ) {
			*rslt_idx = i; /* return the existing default action entry index */
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_EXIST;
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
			ret = _hash_ddr_entry_set(table, i);

			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_OK;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return ret;
}

#if 0
static int __default_action_entry_add_by_idx(void *entry, unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (table->entry[idx].data != NULL) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return AAL_E_ENTRY_USED;
	}

	table->entry[idx].data = ca_zalloc_trace(table->entry_size, GFP_ATOMIC, true);
	memcpy(table->entry[idx].data, entry, table->entry_size);
	atomic_set(&table->entry[idx].refcnt, 1);
	table->used_entry++;

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}
#endif

static int __default_action_entry_del(unsigned int idx)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	if (atomic_dec_and_test(&table->entry[idx].refcnt)) {
		if (table->entry[idx].data != NULL)
			ca_free_trace(table->entry[idx].data);
		table->entry[idx].data = NULL;
		table->used_entry--;

		/* clear entry in DDR */
		if ((ret = _hash_ddr_entry_clear(table, idx)) != AAL_E_OK) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return ret;
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __default_action_entry_set(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;
	int ret;

	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if (entry == NULL) return AAL_E_NULLPTR;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	memcpy(table->entry[idx].data, entry, table->entry_size);

	/* set to HW */
	if ((ret = _hash_ddr_entry_set(table, idx)) != AAL_E_OK) {
		spin_unlock_irqrestore(&g_hash_lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __default_action_entry_get(void *entry, unsigned int *rslt_idx)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;
	unsigned int i, remaining_used = table->used_entry;

	if ((entry == NULL) || (rslt_idx == NULL)) return AAL_E_NULLPTR;
	if (table->entry == NULL) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);

	for (i = 0; i < table->max_entry; i++) {
		if (remaining_used == 0) {
			spin_unlock_irqrestore(&g_hash_lock, flags);
			return AAL_E_ENTRYNOTFOUND;
		}

		if (table->entry[i].data != NULL) {
			remaining_used--;

			if (!memcmp(table->entry[i].data, entry, table->entry_size)) {
				*rslt_idx = i;
				spin_unlock_irqrestore(&g_hash_lock, flags);
				return AAL_E_OK;
			}
		}
	}

	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_ENTRYNOTFOUND;
}

static int __default_action_entry_get_by_idx(unsigned int idx, void *entry)
{
	unsigned long flags;
	aal_table_t *table = &default_action_table;

	if (entry == NULL) return AAL_E_NULLPTR;
	if (idx >= table->max_entry) return AAL_E_OUTRANGE;
	if ((table->entry == NULL) || (table->entry[idx].data == NULL)) return AAL_E_ENTRYNOTFOUND;

	spin_lock_irqsave(&g_hash_lock, flags);
	memcpy(entry, table->entry[idx].data, table->entry_size);
	spin_unlock_irqrestore(&g_hash_lock, flags);

	return AAL_E_OK;
}

static int __default_action_table_flush(void)
{
	int i, ret;

	for (i = 0; i < default_action_table.max_entry; i++) {
		ret = __default_action_entry_del(i);
		if ((ret != AAL_E_OK) && (ret != AAL_E_ENTRYNOTFOUND))
			return ret;
	}

	return AAL_E_OK;
}

static int __default_action_get_avail_count(void)
{
	return generic_get_avail_count(&default_action_table);
}

static void __default_action_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &default_action_table;
	hash_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Hash Default Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = aal_entry_get_by_idx(AAL_TABLE_HASH_DEFAULT_ACTION, i, (void *) &entry);
		} else {
			ret = _hash_ddr_entry_get(&default_action_table, i, (unsigned int *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "| index %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "local_data 0x%016lx\n", table->entry[i].local_data);
			printk(KERN_INFO "raw_data %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					entry.reserved_7, entry.reserved_6, entry.reserved_5, entry.reserved_4,
					entry.reserved_3, entry.reserved_2, entry.reserved_1, entry.reserved_0);

			/* action group mask is stored in local_data of each entry */
			hash_action_dump((unsigned int) table->entry[i].local_data, (hash_hash_action_entry_t *)&entry);
		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static aal_table_t default_action_table = {
	.table_id = AAL_TABLE_HASH_DEFAULT_ACTION,
	.table_name = "HashEngine Default Action Table",
	.max_entry = HASH_DEFAULT_ACTION_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(hash_default_action_entry_t),
	.data_reg_num = 6,
	.op = {
		.entry_add = __default_action_entry_add,
		.entry_add_by_idx = NULL,
		.entry_del = __default_action_entry_del,
		.entry_set = __default_action_entry_set,
		.entry_get = __default_action_entry_get,
		.entry_get_by_idx = __default_action_entry_get_by_idx,
		.table_flush = __default_action_table_flush,
		.get_avail_count = __default_action_get_avail_count,
		.entry_print = __default_action_entry_print,
	},
};

ca_status_t aal_default_action_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint32_t profile_id,
	CA_IN	ca_uint32_t miss_to_cpu)
{
	hash_default_action_entry_t default_action;

	if(profile_id >= HASH_PROFILE_MAX)
		return CA_E_PARAM;

	memset(&default_action, 0, sizeof(hash_default_action_entry_t));
	if (miss_to_cpu == 0) {
		/* default to drop */
		__memcpy_hash_action(&default_action, &HASH_ACT_DROP, sizeof(hash_default_action_t));
	} else if (miss_to_cpu == 1) {
		/* default to cpu */
		if (profile_id == HASH_PROFILE_0)
			__memcpy_hash_action(&default_action, &WAN_NAT_HASH_ACT_DEFAULT_TO_CPU, sizeof(hash_default_action_t));
#if defined(CONFIG_LUNA_G3_SERIES)
		else if (profile_id == HASH_PROFILE_2)
			__memcpy_hash_action(&default_action, &MC_HASH_ACT_DEFAULT_TO_CPU, sizeof(hash_default_action_t));
#endif
		else
			__memcpy_hash_action(&default_action, &LAN_NAT_HASH_ACT_DEFAULT_TO_CPU, sizeof(hash_default_action_t));
	} else {
		return CA_E_PARAM;
	}

	/* only config type 0 default action */
	if (AAL_E_OK != __default_action_entry_set(def_action[profile_id].type0_empty_miss_idx, &default_action))
		return CA_E_ERROR;
	DBG(printk("%s:%d: profile %d type0_empty_miss_idx = %d miss_to_cpu = %d\n", __func__, __LINE__, profile_id, def_action[profile_id].type0_empty_miss_idx, miss_to_cpu));

	if (AAL_E_OK != __default_action_entry_set(def_action[profile_id].type0_age_miss_idx, &default_action))
		return CA_E_ERROR;
	DBG(printk("%s:%d: profile %d type0_age_miss_idx = %d miss_to_cpu = %d\n", __func__, __LINE__, profile_id, def_action[profile_id].type0_age_miss_idx, miss_to_cpu));

	return CA_E_OK;
}

int _hash_default_action_init(void)
{
	int ret;
	unsigned int val = 0;

	spin_lock_init(&g_hash_lock);

	ret = __aal_table_register(AAL_TABLE_HASH_DEFAULT_ACTION, &default_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	val = hash_default_hash_fib_phy_addr;

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_DA0);

	//val = 0x1; /* for ACE */

	//CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_DA1);

	return AAL_E_OK;
}

void _hash_default_action_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_DEFAULT_ACTION);
}

/***************************************************************************
 * Hash Engine Initialization
 ***************************************************************************/

int aal_hash_cache_action_init(void)
{
	unsigned int val = 0;

	val = hash_cache_fib_phy_addr;

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_CA0);

	//val = 0x1; /* for ACE */

	//CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_BA_CA1);

	return AAL_E_OK;
}

ca_status_t aal_hash_cache_invalidate_iros(
CA_IN	ca_device_id_t	device_id,
CA_IN	ca_uint32_t	hash_idx,
CA_IN	ca_uint32_t	hash_crc16)
{
	return aal_hash_cache_invalidate(hash_idx, hash_crc16);
}

int aal_hash_cache_invalidate(unsigned int hash_idx, unsigned int hash_crc16)
{
	int i;
	unsigned int slot;
	L3FE_FE_L3E_HS_CACHE_CTRL_t cache_ctrl;
	L3FE_FE_L3E_HS_CACHE_CTRL_REQ_t cache_ctrl_req;
	L3FE_FE_L3E_HS_CACHE_CTRL_STS_t cache_ctrl_sts;

	memset((void *) &cache_ctrl, 0, sizeof(cache_ctrl));
	memset((void *) &cache_ctrl_req, 0, sizeof(cache_ctrl_req));
	memset((void *) &cache_ctrl_sts, 0, sizeof(cache_ctrl_sts));

	slot = hash_idx & ((1 << g_hb_size) - 1);

	cache_ctrl.bf.crc16 = hash_crc16;
	cache_ctrl.bf.slot = slot & 0x1f;
	cache_ctrl.bf.cmd = 0x1; /* 1: Cache Invalidate One Entry */

	CA_NE_REG_WRITE(cache_ctrl.wrd, L3FE_FE_L3E_HS_CACHE_CTRL);

	/* check the ctrl is idle */
	i = 0;
	do {
		cache_ctrl_req.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_REQ);
	} while ((0x1 & cache_ctrl_req.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* Writing-1 to req_sts will trigger command request. */
	cache_ctrl_req.bf.req_sts = 0x1;
	CA_NE_REG_WRITE(cache_ctrl_req.wrd, L3FE_FE_L3E_HS_CACHE_CTRL_REQ);

	/* wait for it to be completed */
	i = 0;
	do {
		cache_ctrl_req.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_REQ);
	} while ((0x1 & cache_ctrl_req.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	cache_ctrl_sts.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_STS);

	if (ca_aal_debug & AAL_DBG_HASH) {
		if(cache_ctrl_sts.bf.err_nch)
			printk("Hash Cache Invalidate: the hash entry is not cached. This is a non-fatal error.\n");
		else
			printk("Hash Cache Invalidate: cache_ctrl_sts: 0x%x\n", cache_ctrl_sts.wrd);
	}
	return AAL_E_OK;

}

ca_status_t aal_hash_cache_allocation_iros(
CA_IN	ca_device_id_t	device_id,
CA_IN	ca_uint32_t	hash_idx,
CA_IN	ca_uint32_t	hash_crc16,
CA_IN	ca_uint8_t	loc,
CA_IN	ca_uint8_t	age,
CA_IN	ca_uint8_t	priority)
{
	return aal_hash_cache_allocation(hash_idx, hash_crc16, loc, age, priority);
}

int aal_hash_cache_allocation(
	unsigned int hash_idx,
	unsigned int hash_crc16,
	unsigned char loc,
	unsigned char age,
	unsigned char priority)
{
	int i;
	unsigned int slot;
	L3FE_FE_L3E_HS_CACHE_CTRL_t cache_ctrl;
	L3FE_FE_L3E_HS_CACHE_CTRL_REQ_t cache_ctrl_req;
	L3FE_FE_L3E_HS_CACHE_CTRL_STS_t cache_ctrl_sts;

	memset((void *) &cache_ctrl, 0, sizeof(cache_ctrl));
	memset((void *) &cache_ctrl_req, 0, sizeof(cache_ctrl_req));
	memset((void *) &cache_ctrl_sts, 0, sizeof(cache_ctrl_sts));

	slot = hash_idx & ((1 << g_hb_size) - 1);

	cache_ctrl.bf.slot = slot & 0x1f;		/* slot[4:0] */
	cache_ctrl.bf.crc16 = hash_crc16;	/* crc16[15:0] */
	cache_ctrl.bf.loc = loc & 0x1;		/* loc[0] */
	cache_ctrl.bf.age = age & 0x3;		/* age[1:0] */
	cache_ctrl.bf.pri = priority & 0x3;	/* pri[1:0] */

	cache_ctrl.bf.cmd = 0x0; /* 0: Cache Allocation (one entry) */

	CA_NE_REG_WRITE(cache_ctrl.wrd, L3FE_FE_L3E_HS_CACHE_CTRL);

	/* check the ctrl is idle */
	i = 0;
	do {
		cache_ctrl_req.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_REQ);
	} while ((0x1 & cache_ctrl_req.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	/* Writing-1 to req_sts will trigger command request. */
	cache_ctrl_req.bf.req_sts = 0x1;
	CA_NE_REG_WRITE(cache_ctrl_req.wrd, L3FE_FE_L3E_HS_CACHE_CTRL_REQ);

	/* wait for it to be completed */
	i = 0;
	do {
		cache_ctrl_req.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_REQ);
	} while ((0x1 & cache_ctrl_req.bf.req_sts) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	cache_ctrl_sts.wrd = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CTRL_STS);

	if(cache_ctrl_sts.bf.sts_evict)
		printk("Cache Allocation Success: One cache entry with lower priority is evicted.\n");

	//debug_Aaron
	printk("Cache Allocation Matched Cache Entry's loc: %d\n", cache_ctrl_sts.bf.sts_match_loc);

	if(cache_ctrl_sts.bf.sts_match)
		printk("Cache Allocation Success: This hash entry has been cached already.\n");

	if(cache_ctrl_sts.bf.err_free)
		printk("Cache Allocation Error: there is no available way.\n");

	if(cache_ctrl_sts.bf.err_hash)
		printk("Cache Allocation Error: It is an invalid hash entry.\n");

	return AAL_E_OK;

}

#if UNIT_TEST
void _64k_wan_to_lan_hash_add_unit_test(void) {
	int i = 0;
	unsigned int ip_sa = 0xc0a8640a;
	int mask_id = 0;
	int profile_id = 0;
	unsigned hash_idx = 0;
	aal_hash_key_t hash_key;
	aal_hash_action_t hash_action;

	memset((void *) &hash_key, 0, sizeof(hash_key));
	memset((void *) &hash_action, 0, sizeof(hash_action));
	hash_key.ip_ver = 0;
	hash_key.ip_vld = 1;
	hash_action.ip_da_l_vld = 1;
	hash_action.dpid_pri = 1;
	hash_action.dpid_vld = 1;
	hash_action.permit = 1;
	hash_action.mc = 0;
	hash_action.mcgid = 2;
	hash_action.l4_dp_vld = 1;
	for (i = 0; i < HASH_HASH_TBL_ENTRY_MAX; i++) {
		hash_key.ip_sa_0 = ip_sa;
		hash_action.ip_da_0 = ip_sa;
		hash_action.l4_dp = ip_sa & 0xffff;
		//note: you can change age value to 7 to test hash age out.
		aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, 7, mask_id, profile_id, &hash_idx);
		ip_sa++;
		cond_resched();
	}
}

void _64k_lan_to_wan_hash_add_unit_test(void) {
	int i = 0;
	unsigned int ip_da = 0xc0a8010a;
	int mask_id = 1;
	int profile_id = 1;
	unsigned hash_idx = 0;
	aal_hash_key_t hash_key;
	aal_hash_action_t hash_action;

	memset((void *) &hash_key, 0, sizeof(hash_key));
	memset((void *) &hash_action, 0, sizeof(hash_action));
	hash_key.ip_ver = 0;
	hash_key.ip_vld = 1;
	hash_action.ip_sa_l_vld = 1;
	hash_action.dpid_pri = 1;
	hash_action.dpid_vld = 1;
	hash_action.permit = 1;
	hash_action.mc = 0;
	hash_action.mcgid = 0;
	hash_action.chk_msk_ptr = mask_id;
	hash_action.l4_sp_vld = 1;
	for (i = 0; i < HASH_HASH_TBL_ENTRY_MAX; i++) {
		hash_key.ip_da_0 = ip_da;
		hash_action.ip_sa_0 = ip_da;
		hash_action.l4_sp = ip_da & 0xffff;
		//note: you can change age value to 7 to test hash age out.
		aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, 7, mask_id, profile_id, &hash_idx);
		ip_da++;
		cond_resched();
	}
}

void _64k_bidirection_hash_add_unit_test(void) {
	int i = 0;
	unsigned int ip_sa = 0xc0a8010a;
	unsigned int ip_da = 0xc0a8810a;
	int mask_id = 0;
	int profile_id = 0;
	unsigned hash_idx = 0;
	aal_hash_key_t hash_key;
	aal_hash_action_t hash_action;

	memset((void *) &hash_key, 0, sizeof(hash_key));
	memset((void *) &hash_action, 0, sizeof(hash_action));

	/* WAN to LAN */
	hash_key.ip_ver = 0;
	hash_key.ip_vld = 1;
	hash_action.ip_da_l_vld = 1;
	hash_action.dpid_pri = 1;
	hash_action.dpid_vld = 1;
	hash_action.permit = 1;
	hash_action.mc = 0;
	hash_action.mcgid = 2;
	hash_action.chk_msk_ptr = mask_id;
	hash_action.l4_dp_vld = 1;
	for (i = 0; i < HASH_HASH_TBL_ENTRY_MAX / 2; i++) {
		hash_key.ip_sa_0 = ip_sa;
		hash_action.ip_da_0 = ip_sa;
		hash_action.l4_dp = ip_sa & 0xffff;
		aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, 7, mask_id, profile_id, &hash_idx);
		ip_sa++;
		cond_resched();
	}

	/* LAN to WAN */
	mask_id = 1;
	profile_id = 1;
	hash_action.chk_msk_ptr = mask_id;
	hash_action.mcgid = 0;
	hash_action.l4_dp_vld = 0;
	hash_action.l4_sp_vld = 1;
	for (i = HASH_HASH_TBL_ENTRY_MAX / 2; i < HASH_HASH_TBL_ENTRY_MAX; i++) {
		hash_key.ip_da_0 = ip_da;
		hash_action.ip_sa_0 = ip_da;
		hash_action.l4_sp = ip_da & 0xffff;
		aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, 7, mask_id, profile_id, &hash_idx);
		ip_da++;
		cond_resched();
	}
}

void _64k_hash_delete_unit_test(void) {
	int i = 0;
	int ut_cnt;

	printk("%s: Current entry count: \n", __func__);
	printk("%s: hash_tbl_table.used_entry = %u \n", __func__, hash_tbl_table.used_entry);
	printk("%s: hash_action_table.used_entry = %u \n", __func__, hash_action_table.used_entry);
	printk("%s: overflow_tbl_table.used_entry = %u \n", __func__, overflow_tbl_table.used_entry);
	printk("%s: overflow_action_table.used_entry = %u \n", __func__, overflow_action_table.used_entry);
	printk("%s: default_action_table.used_entry = %u \n", __func__, default_action_table.used_entry);
	printk("%s: mask_tbl_table.used_entry = %u \n", __func__, mask_tbl_table.used_entry);
	ut_cnt = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CNT);
	printk("%s: Cache Utilization Counter = %u \n", __func__, ut_cnt);

	for (i = 0; i < HASH_HASH_TBL_ENTRY_MAX; i++) {
		aal_hash_delete(0, i);
		cond_resched();
	}

	i = HASH_HASH_IDX_OVERFLOW_FLAG;
	for (; i < HASH_HASH_IDX_OVERFLOW_FLAG + HASH_OVERFLOW_TBL_ENTRY_MAX; i++) {
		aal_hash_delete(0, i);
		cond_resched();
	}

	printk("\n%s: After deletion, entry count: \n", __func__);
	printk("%s: hash_tbl_table.used_entry = %u \n", __func__, hash_tbl_table.used_entry);
	printk("%s: hash_action_table.used_entry = %u \n", __func__, hash_action_table.used_entry);
	printk("%s: overflow_tbl_table.used_entry = %u \n", __func__, overflow_tbl_table.used_entry);
	printk("%s: overflow_action_table.used_entry = %u \n", __func__, overflow_action_table.used_entry);
	printk("%s: default_action_table.used_entry = %u \n", __func__, default_action_table.used_entry);
	printk("%s: mask_tbl_table.used_entry = %u \n", __func__, mask_tbl_table.used_entry);
	ut_cnt = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CNT);
	printk("%s: Cache Utilization Counter = %u \n", __func__, ut_cnt);
}

ca_status_t aal_64k_wan_to_lan_hash_add_unit_test(
CA_IN	ca_device_id_t	device_id
) {
	_64k_wan_to_lan_hash_add_unit_test();
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_64k_wan_to_lan_hash_add_unit_test);

ca_status_t aal_64k_lan_to_wan_hash_add_unit_test(
CA_IN	ca_device_id_t	device_id
) {
	_64k_lan_to_wan_hash_add_unit_test();
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_64k_lan_to_wan_hash_add_unit_test);

ca_status_t aal_64k_bidirection_hash_add_unit_test(
CA_IN	ca_device_id_t	device_id
) {
	_64k_bidirection_hash_add_unit_test();
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_64k_bidirection_hash_add_unit_test);

ca_status_t aal_64k_hash_del_unit_test(
CA_IN	ca_device_id_t	device_id
) {
	_64k_hash_delete_unit_test();
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_64k_hash_del_unit_test);
#endif

/* L3FE_FE_L3E_HS_CACHE_CNT: Cache Utilization Counter */
ca_status_t aal_hash_cache_count_get(
CA_IN	ca_device_id_t	device_id,
CA_OUT	ca_uint32_t	*ut_cnt)
{
	if(ut_cnt == NULL) return AAL_E_NULLPTR;

	*ut_cnt = CA_NE_REG_READ(L3FE_FE_L3E_HS_CACHE_CNT);

	printk("%s: hash_tbl_table.used_entry = %u \n", __func__, hash_tbl_table.used_entry);
	printk("%s: hash_action_table.used_entry = %u \n", __func__, hash_action_table.used_entry);
	printk("%s: overflow_tbl_table.used_entry = %u \n", __func__, overflow_tbl_table.used_entry);
	printk("%s: overflow_action_table.used_entry = %u \n", __func__, overflow_action_table.used_entry);
	printk("%s: default_action_table.used_entry = %u \n", __func__, default_action_table.used_entry);
	printk("%s: mask_tbl_table.used_entry = %u \n", __func__, mask_tbl_table.used_entry);
	//debug_Aaron
	printk("%s: Cache Utilization Counter = %u \n", __func__, *ut_cnt);

	return CA_E_OK;
}

int aal_hash_default_action_da_width_set(unsigned int da_width)
{
	unsigned int val = 0;

	if (da_width >= HASH_HA_WIDTH_64 && da_width <= HASH_HA_WIDTH_1024) {
		val = da_width;
	} else {
		printk(KERN_ERR "%s: wrong value of da_width = %u\n", __func__, da_width);
		return AAL_E_INVALID;
	}

	if (da_width != g_ha_width) {
		printk(KERN_ERR "%s: da_width != g_ha_width! da_width is %u, but g_ha_width is %u.\n", __func__, da_width, g_ha_width);
		//return AAL_E_INVALID;
	}

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_DEFAULT_INI);

	g_da_width= da_width;

	return AAL_E_OK;
}

int aal_hash_overflow_action_oa_width_set(unsigned int oa_width)
{
	unsigned int val = 0;

	if (oa_width >= HASH_HA_WIDTH_64 && oa_width <= HASH_HA_WIDTH_1024) {
		val = oa_width;
	} else {
		printk(KERN_ERR "%s: wrong value of oa_width = %u\n", __func__, oa_width);
		return AAL_E_INVALID;
	}

	if (oa_width != g_ha_width) {
		printk(KERN_ERR "%s: oa_width != g_ha_width! oa_width is %u, but g_ha_width is %u.\n", __func__, oa_width, g_ha_width);
		//return AAL_E_INVALID;
	}

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_OVERFLOW_INI);

	g_oa_width= oa_width;

	return AAL_E_OK;
}

int aal_hash_cache_init(unsigned int ct_size, unsigned int ca_width)
{
	L3FE_FE_L3E_HS_CACHE_INI_t cache_ini;
	L3FE_FE_L3E_HS_CACHE_MISC_t cache_misc;
	L3FE_FE_L3E_HS_CACHE_AGE32_t cache_age_32;
	L3FE_FE_L3E_HS_CACHE_AGE10_t cache_age_10;

	memset((void *) &cache_ini, 0, sizeof(cache_ini));
	memset((void *) &cache_misc, 0, sizeof(cache_misc));
	memset((void *) &cache_age_32, 0, sizeof(cache_age_32));
	memset((void *) &cache_age_10, 0, sizeof(cache_age_10));

	if (ct_size >= HASH_CT_DISABLE && ct_size <= HASH_CT_SIZE_2K) {
		cache_ini.bf.ct_size = ct_size;
	} else {
		 return AAL_E_INVALID;
	}

	if (ca_width >= HASH_HA_WIDTH_64 && ca_width <= HASH_HA_WIDTH_1024) {
		cache_ini.bf.ca_width = ca_width;
	} else {
		printk(KERN_ERR "%s: wrong value of ca_width = %u\n", __func__, ca_width);
		return AAL_E_INVALID;
	}

	//Samuel: make ca_width = g_ha_width for easy maintenance
	if (ca_width != g_ha_width) {
		printk(KERN_ERR "%s: ca_width != g_ha_width! ca_width is %u, but g_ha_width is %u.\n", __func__, ca_width, g_ha_width);
		return AAL_E_INVALID;
	}

	CA_NE_REG_WRITE(cache_ini.wrd, L3FE_FE_L3E_HS_CACHE_INI);

	g_ct_size = ct_size;
	g_ca_width= ca_width;

	/* should get it from scfg */
	cache_hash_enabled = TRUE;
	cache_hash_replacement_enabled = TRUE;

	cache_misc.bf.s3_ra_en = cache_hash_enabled;
	cache_misc.bf.s3_loc_n = cache_hash_stage3_loc;
	cache_misc.bf.crep_en = cache_hash_replacement_enabled;
	cache_misc.bf.s2_ra_pri = cache_hash_stage2_allocation_pri & 0x3;
	cache_misc.bf.s2_ra_en = cache_hash_stage2_allocation_enabled;

	CA_NE_REG_WRITE(cache_misc.wrd, L3FE_FE_L3E_HS_CACHE_MISC);

	cache_age_10.bf.ut_t0 = cache_hash_age_ut_t0;
	cache_age_10.bf.ut_t1 = cache_hash_age_ut_t1;
	cache_age_32.bf.ut_t2 = cache_hash_age_ut_t2;
	cache_age_32.bf.ut_t3 = cache_hash_age_ut_t3;

	CA_NE_REG_WRITE(cache_age_10.wrd, L3FE_FE_L3E_HS_CACHE_AGE10);
	CA_NE_REG_WRITE(cache_age_32.wrd, L3FE_FE_L3E_HS_CACHE_AGE32);

	return AAL_E_OK;
}

int aal_hash_init(void)
{
	uint64_t quotient;
	uint32_t remainder;
	int ret;
	struct ca_soc_data soc_data;

	aal_hash_key_t hash_key;
	aal_hash_action_t hash_action;

	aal_hash_mem_init();

	if ((ca_soc_data_get(&soc_data) != 0) || (soc_data.core_clk == 0))
		soc_data.core_clk = 250000000; /* assume core clk 250MHz */
#if 0
	/* ARM32 compiler calls __aeabi_uldivmod to divide 64-bit numbers, but Linux doesn't provide the function. */
	g_max_aging_time = ((uint64_t) 0x3FFFFFFF * HASH_AGING_UNIT * HASH_AGING_START / soc_data.core_clk);
#else
	ca_div((uint64_t) 0x3FFFFFFF * HASH_AGING_UNIT * HASH_AGING_START, soc_data.core_clk, &quotient, &remainder);
	g_max_aging_time = (unsigned int) quotient;
#endif

	hash_mask_unmask(&HASH_MASK_MULTICAST);
	HASH_MASK_MULTICAST.ip_da = 128;
	HASH_MASK_MULTICAST.ip_ver = 0;
	HASH_MASK_MULTICAST.ip_vld = 0;
#if defined(CONFIG_LUNA_G3_SERIES) && !defined(CONFIG_RTK_DEV_AP)
	// no possible to expect vlan id of mc data flow, so we disable vlan pattern in mc hash tuple.
	HASH_MASK_MULTICAST.top_vid = 1;	
#else
//	HASH_MASK_MULTICAST.top_vid = 0;
#endif
	aal_entry_add(AAL_TABLE_HASH_MASK_TBL, &HASH_MASK_MULTICAST, &hash_mask_multicast_idx);

#if defined(CONFIG_LUNA_G3_SERIES)&& defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH)
	hash_mask_unmask(&HASH_MASK_MULTICAST_FORWARD);
	HASH_MASK_MULTICAST_FORWARD.ip_da = 128;
	HASH_MASK_MULTICAST_FORWARD.ip_sa =128;
	HASH_MASK_MULTICAST_FORWARD.ip_ver = 0;
	HASH_MASK_MULTICAST_FORWARD.ip_vld = 0;
	HASH_MASK_MULTICAST_FORWARD.top_vid = 0;
	//HASH_MASK_MULTICAST_FORWARD.lspid=0;
	//HASH_MASK_MULTICAST_FORWARD.pol_grp_id=0;
	//aal_mask_tbl_entry_add(&HASH_MASK_MULTICAST_FORWARD, &hash_mask_multicast_forward_idx);
	aal_entry_add(AAL_TABLE_HASH_MASK_TBL,&HASH_MASK_MULTICAST_FORWARD, &hash_mask_multicast_forward_idx);
{

	//setting profile tuple priority
	L3FE_FE_L3E_HS_PROFILE0_TUPLE0_t tuple;
	memset(&tuple,0,sizeof(tuple));
	tuple.bf.pri=0;
	hash_profile_2.hit[0].tuple_wrd=tuple.wrd;
	tuple.bf.pri=1;
	hash_profile_2.hit[1].tuple_wrd=tuple.wrd;

}
#endif


	hash_mask_unmask(&HASH_MASK_IP_PROT);
	HASH_MASK_IP_PROT.ip_protocol = 0;
	HASH_MASK_IP_PROT.ip_vld = 0;
	aal_entry_add(AAL_TABLE_HASH_MASK_TBL, &HASH_MASK_IP_PROT, &hash_mask_ip_prot_idx);
	/* check flow_support before setting all NAT profiles */
	if (flow_support != 0) {
#if defined(CONFIG_LUNA_G3_SERIES)		
		hash_profile_0.type0_act_grp_mask_hit = ACTGRP_FLOW_RTK_DS;
		hash_profile_1.type0_act_grp_mask_hit = ACTGRP_FLOW_RTK_US;
#else
		hash_profile_0.type0_act_grp_mask_hit = (l3fe_hash_indexed_mode) ? ACTGRP_FLOW_SHORT : ACTGRP_FLOW;
		hash_profile_1.type0_act_grp_mask_hit = (l3fe_hash_indexed_mode) ? ACTGRP_FLOW_SHORT : ACTGRP_FLOW;
#endif
		/* remove default tuple of profile 0 and profile 1 */
		hash_profile_0.hit[0].tuple_type = TUPLE_TYPE_UNUSED;
		hash_profile_0.hit[0].hash_mask = NULL;
		hash_profile_0.hit[0].refcnt = 0;
		hash_profile_0.hit[0].tuple_wrd = 0;
		hash_profile_1.hit[0].tuple_type = TUPLE_TYPE_UNUSED;
		hash_profile_1.hit[0].hash_mask = NULL;
		hash_profile_1.hit[0].refcnt = 0;
		hash_profile_1.hit[0].tuple_wrd = 0;
	} else {
		hash_mask_unmask(&HASH_MASK_NAT);
		HASH_MASK_NAT.ctrl_set_id = 0;
		HASH_MASK_NAT.ip_sa = 128;
		HASH_MASK_NAT.ip_da = 128;
		HASH_MASK_NAT.l4_dp_exact_range = 0;
		HASH_MASK_NAT.l4_sp_exact_range = 0;
		HASH_MASK_NAT.ip_protocol = 0;
		HASH_MASK_NAT.ip_ver = 0;
		HASH_MASK_NAT.ip_vld = 0;
		aal_entry_add(AAL_TABLE_HASH_MASK_TBL, &HASH_MASK_NAT, &hash_mask_nat_idx);	
		hash_profile_0.type0_act_grp_mask_hit = (l3fe_hash_indexed_mode) ? ACTGRP_NAT_TO_LAN_1 : ACTGRP_NAT_TO_LAN;
		hash_profile_1.type0_act_grp_mask_hit = (l3fe_hash_indexed_mode) ? ACTGRP_NAT_TO_WAN_1 : ACTGRP_NAT_TO_WAN;
	}

	/* set all NAT profiles */
	ret = aal_hash_profile_set(HASH_PROFILE_0, &hash_profile_0);
	if (ret != AAL_E_OK) {
		printk("%s:%d ret = %d", __func__, __LINE__, ret);
		return ret;
	}

	ret = aal_hash_profile_set(HASH_PROFILE_1, &hash_profile_1);
	if (ret != AAL_E_OK) {
		printk("%s:%d ret = %d", __func__, __LINE__, ret);
		return ret;
	}

	ret = aal_hash_profile_set(HASH_PROFILE_2, &hash_profile_2);
	if (ret != AAL_E_OK) {
		printk("%s:%d ret = %d", __func__, __LINE__, ret);
		return ret;
	}
#if defined(CONFIG_LUNA_G3_SERIES)
	ret = aal_hash_profile_set(HASH_PROFILE_3, &hash_profile_3);
	if (ret != AAL_E_OK) {
		printk("%s:%d ret = %d", __func__, __LINE__, ret);
		return ret;
	}

	ret = aal_hash_profile_set(HASH_PROFILE_4, &hash_profile_4);
	if (ret != AAL_E_OK) {
		printk("%s:%d ret = %d", __func__, __LINE__, ret);
		return ret;
	}
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
	// RTK trap control packet by SPECIAL_PKT and CLS
	printk(KERN_INFO "%s: RTK skip adding hash for unknown Multicast ICMPv6 packet. \n", __func__);

#else
	/* add default hash entries */

	/* unknown Multicast ICMPv6 */
	memset(&hash_key, 0, sizeof(aal_hash_key_t));
	memset(&hash_action, 0, sizeof(aal_hash_action_t));
	hash_key.ip_vld = 1;
	hash_key.ip_protocol = 58; /* ICMPv6 */
	hash_action.dpid_vld = 1;
	hash_action.dpid_pri = 1;
	hash_action.permit = 1;
	hash_action.mc = 0;
	hash_action.keep_orig_pkt = 1;
	hash_action.keep_orig_pkt_vld = 1;
	hash_action.mcgid = AAL_LPORT_CPU_0;
	ret = aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, hash_mask_ip_prot_idx, l3fe_main_hash_profile_mc, &hash_idx_umc_icmpv6);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add hash for unknown Multicast ICMPv6 packet. (ret=%d)\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: Add hash for unknown Multicast ICMPv6 packet. (hash_idx=%u)\n", __func__, hash_idx_umc_icmpv6);
#endif

	return AAL_E_OK;
}

/* debugging purpose */
ca_status_t aal_hash_crc_hw_offload (
CA_IN	ca_device_id_t	device_id,
CA_IN	L3FE_HDR_I_t	*hdr_i,
CA_IN	ca_uint32_t	mask_id,
CA_OUT	ca_uint32_t	*crc32,
CA_OUT	ca_uint16_t	*crc16)
{
	unsigned int val = 0, i = 0;
	unsigned int *hdr_i_val = (unsigned int *) hdr_i;

	/*
	*	L3FE_FE_L3E_HS_SWO_IDX [IDX]
	*	0: Header-I [31:0] (RW)
	*	1: Header-I [63:32] (RW)
	*	...
	*	32: Mask-Pointer [5:0] (RW)
	*	33: CRC32 (RO)
	*	34: CRC16 (RO)
	*	35: Hash Tuple[31:0] (RO)
	*	36: Hash Tuple[63:32] (RO)
	*	...
	*/

	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_SWO_IDX);
	for(i = 0; (i * 4) < sizeof(L3FE_HDR_I_t); i++){
		DBG(printk("%s:%d hdr_i_val[%d] = 0x%x\n", __func__, __LINE__, i,  hdr_i_val[i]));
		CA_NE_REG_WRITE(hdr_i_val[i], L3FE_FE_L3E_HS_SWO_DAT);
	}
	DBG(printk("%s:%d Finish hdr_i idx = %d\n", __func__, __LINE__, i));

	/* SWO_DAT[32]: mask pointer */
	CA_NE_REG_WRITE(mask_id, L3FE_FE_L3E_HS_SWO_DAT);

	i = 0;
	do {
		val = CA_NE_REG_READ(L3FE_FE_L3E_HS_SWO_CTRL);
	} while ((0x1 & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	val = 1; /* Writing-1 to request CRC calculation. */
	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_SWO_CTRL);

	do {
		val = CA_NE_REG_READ(L3FE_FE_L3E_HS_SWO_CTRL);
	} while ((0x1 & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		return AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	val = 0; 	/* write index to 0 and read from index 0*/
	CA_NE_REG_WRITE(val, L3FE_FE_L3E_HS_SWO_IDX);

	for(i = 0; i < (1 << 6); i++) {
		val = CA_NE_REG_READ(L3FE_FE_L3E_HS_SWO_DAT);

		switch( i ) {
		case 32:
			DBG(printk("%s:%d mask pointer: 0x%x\n", __func__, __LINE__, val));
			break;
		case 33:
			DBG(printk("%s:%d CRC32: 0x%x\n", __func__, __LINE__, val));
			*crc32 = val;
			break;
		case 34:
			DBG(printk("%s:%d CRC16: 0x%x\n", __func__, __LINE__, val));
			*crc16 = (ca_uint16_t) (val & 0xffff);
			break;
		default:
			if (ca_aal_debug & AAL_DBG_HASH) {
				if (i < 33)
					printk("%s:%d Input HDR_I[%d] = 0x%x\n", __func__, __LINE__, i, val);
				else
					printk("%s:%d Output Hash Tuple[%d] = 0x%x\n", __func__, __LINE__, i - 35, val);
			}
			break;
		}
	}

	return CA_E_OK;
}


/* unit test */
ca_status_t aal_hash_crc_sw_hw_calc_check (
CA_IN	aal_hash_key_t *hash_key,
CA_IN	ca_uint32_t	mask_id,
CA_IN	ca_uint32_t	sw_crc32,
CA_IN	ca_uint16_t	sw_crc16) {
	ca_uint32_t	hw_crc32;
	ca_uint16_t	hw_crc16;
	L3FE_HDR_I_t	hdr_i;

	memset(&hdr_i, 0, sizeof(L3FE_HDR_I_t));

	hdr_i.l4_dp = hash_key->l4_dp_exact_range;
	hdr_i.l4_sp = hash_key->l4_sp_exact_range;
	hdr_i.ip_da_0 = hash_key->ip_da_0;
	hdr_i.ip_da_1 = hash_key->ip_da_1;
	hdr_i.ip_da_2 = hash_key->ip_da_2;
	hdr_i.ip_da_3 = hash_key->ip_da_3;
	hdr_i.ip_sa_0 = hash_key->ip_sa_0;
	hdr_i.ip_sa_1 = hash_key->ip_sa_1;
	hdr_i.ip_sa_2 = hash_key->ip_sa_2;
	hdr_i.ip_sa_3 = hash_key->ip_sa_3;
	hdr_i.ip_protocol= hash_key->ip_protocol;
	hdr_i.ip_vld = hash_key->ip_vld;
	hdr_i.ip_ver =  hash_key->ip_ver;
	hdr_i.t2_ctrl=  hash_key->ctrl_set_id;

	aal_hash_crc_hw_offload(0, &hdr_i, mask_id, &hw_crc32, &hw_crc16);

	if (ca_aal_debug & AAL_DBG_HASH) {
		if(hw_crc32 != sw_crc32 || hw_crc16 != sw_crc16)
			printk("%s:%d: CRC check is failed! hw_crc32=0x%08x, sw_crc32=0x%08x, hw_crc16=0x%04x, sw_crc16=0x%04x\n",
				__func__, __LINE__, hw_crc32, sw_crc32, hw_crc16, sw_crc16);
		else
			printk("%s:%d: CRC check OK! hw_crc32=0x%08x, sw_crc32=0x%08x, hw_crc16=0x%04x, sw_crc16=0x%04x\n",
				__func__, __LINE__, hw_crc32, sw_crc32, hw_crc16, sw_crc16);
	}

	return CA_E_OK;
}

/* cache_hash_table */
int _cache_hash_table_init(void)
{
	int ret;

	ret = __aal_table_register(AAL_TABLE_CACHE_HASH_TBL, &cache_hash_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void _cache_hash_table_exit(void)
{
	__aal_table_unregister(AAL_TABLE_CACHE_HASH_TBL);
}

static void __cache_hash_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &cache_hash_table;
	aal_cache_hash_t entry;
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

	printk(KERN_INFO "|===== Cache Hash Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = __hash_hw_entry_get(&cache_hash_table, i, &entry);
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index    %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "| crc32    0x%08x\n", entry.crc32);
			printk(KERN_INFO "| crc16    0x%04x\n", entry.crc16);
			printk(KERN_INFO "| slot     0x%x\n", entry.slot);
			printk(KERN_INFO "| location 0x%x\n", entry.location);
			printk(KERN_INFO "| pri      0x%x\n", entry.pri);
			printk(KERN_INFO "| age      0x%x\n", entry.age);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t cache_hash_table = {
	.table_id = AAL_TABLE_CACHE_HASH_TBL,
	.table_name = "HashEngine Cache Hash Table",
	.max_entry = HASH_HASH_CACHE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size =  sizeof(aal_cache_hash_t),
	.access_reg = L3FE_FE_L3E_HS_CACHE_HASH_ACCESS,
	.data_reg_num = 2,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = NULL,
		.entry_del = NULL,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = NULL,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = __cache_hash_tbl_entry_print,
	},
};

/* cache_hash_action_table */
int _cache_hash_action_table_init(void)
{
	int ret;
	ret = __aal_table_register(AAL_TABLE_CACHE_HASH_ACTION_TBL, &cache_hash_action_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void _cache_hash_action_table_exit(void)
{
	__aal_table_unregister(AAL_TABLE_CACHE_HASH_ACTION_TBL);
}

static void __cache_hash_action_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &cache_hash_action_table;
	hash_hash_action_entry_t entry;
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

	printk(KERN_INFO "|===== Cache Hash Action Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = _hash_ddr_entry_get(&cache_hash_action_table, i, (unsigned int *)&entry);
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index    %04d | ref count %d\n", i, refcnt);
			printk(KERN_INFO "| QWORD0     0x%llx\n", entry.reserved_0);
			printk(KERN_INFO "| QWORD1     0x%llx\n", entry.reserved_1);
			printk(KERN_INFO "| QWORD2     0x%llx\n", entry.reserved_2);
			printk(KERN_INFO "| QWORD3     0x%llx\n", entry.reserved_3);
			printk(KERN_INFO "| QWORD4     0x%llx\n", entry.reserved_4);
			printk(KERN_INFO "| QWORD5     0x%llx\n", entry.reserved_5);
			printk(KERN_INFO "| QWORD6     0x%llx\n", entry.reserved_6);
			printk(KERN_INFO "| QWORD7     0x%llx\n", entry.reserved_7);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t cache_hash_action_table = {
	.table_id = AAL_TABLE_CACHE_HASH_ACTION_TBL,
	.table_name = "HashEngine Cache Hash Action Table",
	.max_entry = HASH_HASH_CACHE_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size =  sizeof(hash_hash_action_entry_t),
	.data_reg_num = 2,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = NULL,
		.entry_del = NULL,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = NULL,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = __cache_hash_action_tbl_entry_print,
	},
};


/* hash_age_table */
int _hash_age_table_init(void)
{
	int ret;

	ret = __aal_table_register(AAL_TABLE_HASH_AGE_TBL, &hash_age_table);
	if (ret != AAL_E_OK) {
		return ret;
	}

	return AAL_E_OK;
}

void _hash_age_table_exit(void)
{
	__aal_table_unregister(AAL_TABLE_HASH_AGE_TBL);
}

static void __hash_age_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &hash_age_table;
	aal_hash_age_tbl_t entry;
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

	printk(KERN_INFO "|===== Hash Age Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		ret = __hash_hw_entry_get(&hash_age_table, i, (void *)&entry);
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "| index    %04d | ref count %d\n", i, refcnt);
			if(entry.age0.wrd != 0)
				printk(KERN_INFO "| entry.age0.wrd 0x%08x\n", entry.age0.wrd);
			if(entry.age1.wrd != 0)
				printk(KERN_INFO "| entry.age1.wrd 0x%08x\n", entry.age1.wrd);
			if(entry.age2.wrd != 0)
				printk(KERN_INFO "| entry.age2.wrd 0x%08x\n", entry.age2.wrd);
			if(entry.age3.wrd != 0)
				printk(KERN_INFO "| entry.age3.wrd 0x%08x\n", entry.age3.wrd);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t hash_age_table = {
	.table_id = AAL_TABLE_HASH_AGE_TBL,
	.table_name = "HashEngine Hash Age Table",
	.max_entry = HASH_HASH_TBL_ENTRY_MAX / 32,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size =  sizeof(aal_hash_age_tbl_t),
	.access_reg = L3FE_FE_L3E_HS_HASH_AGE_MEM_ACCESS,
	.data_reg_num = 4,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = NULL,
		.entry_del = NULL,
		.entry_set = NULL,
		.entry_get = NULL,
		.entry_get_by_idx = NULL,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = __hash_age_tbl_entry_print,
	},
};

EXPORT_SYMBOL(aal_hash_aging_timer_set_iros);
EXPORT_SYMBOL(aal_hash_aging_timer_get_iros);
EXPORT_SYMBOL(aal_hash_current_granularity_get_iros);
EXPORT_SYMBOL(aal_hash_age_set_iros);
EXPORT_SYMBOL(aal_hash_age_get_iros);
EXPORT_SYMBOL(aal_mask_tbl_entry_add_iros);
EXPORT_SYMBOL(aal_mask_tbl_entry_del_iros);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_hash_action_update);
#endif
EXPORT_SYMBOL(aal_hash_add);
EXPORT_SYMBOL(aal_hash_delete);
EXPORT_SYMBOL(aal_default_action_set);
EXPORT_SYMBOL(aal_hash_cache_invalidate_iros);
EXPORT_SYMBOL(aal_hash_cache_allocation_iros);
EXPORT_SYMBOL(aal_hash_cache_count_get);
EXPORT_SYMBOL(aal_hash_crc_hw_offload);

