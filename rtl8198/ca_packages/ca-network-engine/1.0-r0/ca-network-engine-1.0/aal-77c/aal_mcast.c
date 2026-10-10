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
 * aal_mcast.c: Hardware Abstraction Layer for Multicast to access hardware regsiters
 *
 */

#include <linux/sched.h>

#include <generated/ca_ne_autoconf.h>
#include "aal_common.h"
#include "aal_table.h"
#include "aal_mcast.h"
#include "aal_port.h"
#include "aal_arb.h"
#include "scfg.h"
#include "vlan.h"
#include "ca_aal_proc.h"
#include "aal_arb.h"

/* NOTE:
 * see comment before the commit 6908538de6077d15cc0edb1252b62fdc3cb29bf4
 */

SIZE_CHECK_ASSERT(sizeof(ni_mce_indx_tbl_entry_t) == 8);
SIZE_CHECK_ASSERT(sizeof(l2fe_arb_fib_tbl_entry_t) == 16);
SIZE_CHECK_ASSERT(sizeof(im3e_mldda_entry_t) == 24);
SIZE_CHECK_ASSERT(sizeof(im3e_mldsa_entry_t) == 20);

#define DBG(x)			{ if (ca_aal_debug & AAL_DBG_MCAST) x; }

#define IM3E_CMD_ADD	1
#define IM3E_CMD_DEL	2
#define IM3E_CMD_SRCH	3
#define IM3E_CMD_SRCH_N	4
#define TABLE_TRY_TIMEOUT	1000

struct repl_bits_s {
	ca_uint16_t ref_cnt;
	repl_key_t key;
};

struct mcgid_entry_s {
	ca_uint8_t used;
	ca_uint8_t *l2ptr;
	ca_uint8_t *l3ptr;
};

struct fake_vlan_entry_s {
	ca_uint8_t valid;
	ca_uint16_t ref_cnt;
	ca_uint16_t wan_vid;
	ca_uint16_t fake_vid;
};

#if 1//defined(CONFIG_LUNA_G3_SERIES) 
struct l2fe_pe_mymac_s {
	ca_mac_addr_t 	mac_addr;
	ca_uint16_t 	myMac_refCnt;
};
struct l2fe_pe_mymac_s free_l2fe_pe_mymac_pool[L2FE_PE_MYMAC_MAX];	
#endif

struct repl_bits_s free_repl_bits_pool[ARB_FIB_TBL_ENTRY_MAX];
struct mcgid_entry_s free_mcgid_pool[MCE_INDX_TBL_ENTRY_MAX];
ca_uint8_t im3e_cmd_mode = 1;
ca_uint32_t igmp_snooping_on;
ca_uint32_t mcast_mode;
ca_uint32_t umc_drop;
ca_uint32_t wan2lan_only_igmp;
ca_uint32_t igmp_use_mc_vlan;
ca_uint32_t mld_use_mc_vlan;


static aal_table_t ni_mce_indx_table;

int aal_mce_indx_init(void)
{
	spin_lock_init(&ni_mce_indx_table.lock);

	return __aal_table_register(AAL_TABLE_MCE_INDX_TBL, &ni_mce_indx_table);
}

void aal_mce_indx_exit(void)
{
	__aal_table_unregister(AAL_TABLE_MCE_INDX_TBL);
}

static int aal_mce_indx_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &ni_mce_indx_table;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	return generic_entry_add(&ni_mce_indx_table, entry, rslt_idx);
}

static int aal_mce_indx_entry_add_by_idx(void *entry, unsigned int idx)
{
	return generic_entry_add_by_idx(&ni_mce_indx_table, entry, idx);
}

static int aal_mce_indx_entry_del(unsigned int idx)
{
	return generic_entry_del(&ni_mce_indx_table, idx);
}

static int aal_mce_indx_entry_set(unsigned int idx, void *entry)
{
	return generic_entry_set(&ni_mce_indx_table, idx, entry);
}

static int aal_mce_indx_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&ni_mce_indx_table, entry, rslt_idx);
}

static int aal_mce_indx_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&ni_mce_indx_table, idx, entry);
}

static void aal_mce_indx_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &ni_mce_indx_table;
	ni_mce_indx_tbl_entry_t entry;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	printk(KERN_INFO "|===== NI MCE INDX Table ==================================================|\n");

	for (i = 0; i < count; i++) {
		if (start_idx + i >= table->max_entry)
			break;
		if (ca_aal_table_sw_cache) {
			ret = aal_mce_indx_entry_get_by_idx(start_idx + i, (void *)&entry);
		} else {
			ret = hw_entry_get(&ni_mce_indx_table, start_idx + i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", start_idx + i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[start_idx + i].refcnt);

			printk(KERN_INFO "| index %04d | ref count %d\n", start_idx + i, refcnt);
			printk(KERN_INFO "  |- replication bits: 0x%016llx\n", entry.mc_vec);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t ni_mce_indx_table = {
	.table_id = AAL_TABLE_MCE_INDX_TBL,
	.table_name = "Multicast Engine (MCE)",
	.max_entry = MCE_INDX_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(ni_mce_indx_tbl_entry_t),
	.access_reg = NI_HV_MCE_INDX_LKUP_ACCESS,
	.data_reg_num = 2,
	.op = {
		.entry_add = aal_mce_indx_entry_add,
		.entry_add_by_idx = aal_mce_indx_entry_add_by_idx,
		.entry_del = aal_mce_indx_entry_del,
		.entry_set = aal_mce_indx_entry_set,
		.entry_get = aal_mce_indx_entry_get,
		.entry_get_by_idx = aal_mce_indx_entry_get_by_idx,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = aal_mce_indx_entry_print,
	},
};

static aal_table_t l2fe_arb_fib_table;

int aal_arb_fib_init(void)
{
	spin_lock_init(&l2fe_arb_fib_table.lock);
	memset(&free_l2fe_pe_mymac_pool,0,sizeof(free_l2fe_pe_mymac_pool));
	return __aal_table_register(AAL_TABLE_ARB_FIB_TBL, &l2fe_arb_fib_table);
}

void aal_arb_fib_exit(void)
{
	__aal_table_unregister(AAL_TABLE_ARB_FIB_TBL);
}

static int aal_arb_fib_entry_add(void *entry, unsigned int *rslt_idx)
{
	aal_table_t *table = &l2fe_arb_fib_table;

	if ((entry == NULL) || (rslt_idx == NULL) || (table->entry == NULL)) return AAL_E_NULLPTR;

	return generic_entry_add(&l2fe_arb_fib_table, entry, rslt_idx);
}

static int aal_arb_fib_entry_add_by_idx(void *entry, unsigned int idx)
{
	return generic_entry_add_by_idx(&l2fe_arb_fib_table, entry, idx);
}

static int aal_arb_fib_entry_del(unsigned int idx)
{
	return generic_entry_del(&l2fe_arb_fib_table, idx);
}

static int aal_arb_fib_entry_set(unsigned int idx, void *entry)
{
	return generic_entry_set(&l2fe_arb_fib_table, idx, entry);
}

static int aal_arb_fib_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&l2fe_arb_fib_table, entry, rslt_idx);
}

static int aal_arb_fib_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&l2fe_arb_fib_table, idx, entry);
}

static void aal_arb_fib_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &l2fe_arb_fib_table;
	l2fe_arb_fib_tbl_entry_t entry;
	int i, ret;
	unsigned int refcnt;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range! Max is %u, but start_idx is %u.\n", __func__, table->max_entry, start_idx);
		return;
	}

	printk(KERN_INFO "|===== L2FE ARB MC FIB Table ==================================================|\n");

	for (i = 0; i < count; i++) {
		if (start_idx + i >= table->max_entry)
			break;
		if (ca_aal_table_sw_cache) {
			ret = aal_arb_fib_entry_get_by_idx(start_idx + i, (void *)&entry);
		} else {
			ret = hw_entry_get(&l2fe_arb_fib_table, start_idx + i, (void *)&entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", start_idx + i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[start_idx + i].refcnt);

			printk(KERN_INFO "| index %04d | ref count %d\n", start_idx + i, refcnt);
			printk(KERN_INFO "  |- wan_dst: %d (distined to %s)\n", entry.wan_dst,
					(entry.wan_dst)? "WAN": "LAN");
			printk(KERN_INFO "  |- permit_spid_en: %d (%s)\n", entry.permit_spid_en,
					(entry.permit_spid_en)? "NOP": "Drop source port loopback");
			printk(KERN_INFO "  |- mac_sa_cmd: %d (%s)\n", entry.mac_sa_cmd,
					(entry.mac_sa_cmd)? "swap":"old");
			printk(KERN_INFO "  |- mac_sa_sel: %d\n", entry.mac_sa_sel);
			printk(KERN_INFO "  |- ldpid: 0x%03x\n", entry.ldpid);
			printk(KERN_INFO "  |- cos: 0x%03x\n", entry.cos);
			printk(KERN_INFO "  |- pol_en: %d\n", entry.pol_en);
			printk(KERN_INFO "  |- pol_grp_id: 0x%03x\n", entry.pol_grp_id);
			printk(KERN_INFO "  |- pol_id: 0x%03x\n", entry.pol_id);
			printk(KERN_INFO "  |- mcgid_en: %d\n", entry.mcgid_en);
			printk(KERN_INFO "  |- mcgid: 0x%03x\n", entry.mcgid);
			printk(KERN_INFO "  |- vlan_fltr_en: %d (%s)\n", entry.vlan_fltr_en,
					(entry.vlan_fltr_en)? "drop":"forward");
			printk(KERN_INFO "  |- vlan_cmd: 0x%x (%s)\n", entry.vlan_cmd,
					(entry.vlan_cmd == 1)? "push":
					(entry.vlan_cmd == 2)? "pop all":
					(entry.vlan_cmd == 3)? "swap":
					(entry.vlan_cmd == 4)? "pop": "NOP");
			printk(KERN_INFO "  |- vid: 0x%03x\n", entry.vid);
			printk(KERN_INFO "  |- sc_ind: %d (%s)\n", entry.sc_ind,
					(entry.sc_ind)? "SVLAN":"CVLAN");
			printk(KERN_INFO "  |- dot1p_cmd: 0x%x (%s)\n", entry.dot1p_cmd,
					(entry.dot1p_cmd)? "swap":"old");
			printk(KERN_INFO "  |- dot1p: 0x%x\n", entry.dot1p);
			printk(KERN_INFO "  |- dei_cmd: 0x%x (%s)\n", entry.dei_cmd,
					(entry.dei_cmd)? "swap":"old");
			printk(KERN_INFO "  |- dei: 0x%x\n", entry.dei);
			printk(KERN_INFO "  |- mac_cmd: %d (%s)\n", entry.mac_cmd,
					(entry.mac_cmd)? "swap":"old");
			printk(KERN_INFO "  |- mac_da: %02x:%02x:%02x:%02x:%02x:%02x\n",
					entry.mac_da[5], entry.mac_da[4],
					entry.mac_da[3], entry.mac_da[2],
					entry.mac_da[1], entry.mac_da[0]);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			cond_resched();
		}
	}
}

static aal_table_t l2fe_arb_fib_table= {
	.table_id = AAL_TABLE_ARB_FIB_TBL,
	.table_name = "ARB FIB",
	.max_entry = ARB_FIB_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l2fe_arb_fib_tbl_entry_t),
	.access_reg = L2FE_ARB_MC_FIB_TBL_ACCESS,
	.data_reg_num = 4,
	.op = {
		.entry_add = aal_arb_fib_entry_add,
		.entry_add_by_idx = aal_arb_fib_entry_add_by_idx,
		.entry_del = aal_arb_fib_entry_del,
		.entry_set = aal_arb_fib_entry_set,
		.entry_get = aal_arb_fib_entry_get,
		.entry_get_by_idx = aal_arb_fib_entry_get_by_idx,
		.table_flush = NULL,
		.get_avail_count = NULL,
		.entry_print = aal_arb_fib_entry_print,
	},
};


ca_int16_t aal_mc_mcgid_alloc()
{
	ca_int16_t i;

	for (i = 0; i < MCE_INDX_TBL_ENTRY_MAX; i++) {
		if (free_mcgid_pool[i].used == 0) {
			free_mcgid_pool[i].used = 1;
			free_mcgid_pool[i].l2ptr = free_mcgid_pool[i].l3ptr = NULL;
			return (ca_int16_t)(i);
		}
	}
	return -1;
}

void aal_mc_mcgid_set_callback(ca_uint16_t mcgid, mcgid_type_t type, ca_int8_t *ptr)
{
	if (free_mcgid_pool[mcgid].used) {
		if (type == MC_L2)
			free_mcgid_pool[mcgid].l2ptr = ptr;
		if (type == MC_L3)
			free_mcgid_pool[mcgid].l3ptr = ptr;
	}
}

ca_int8_t* aal_mc_mcgid_ptr_get(ca_uint16_t mcgid, mcgid_type_t type)
{
	if (mcgid >= MCE_INDX_TBL_ENTRY_MAX)
		return NULL;

	if (free_mcgid_pool[mcgid].used) {
		if (type == MC_L2)
			return free_mcgid_pool[mcgid].l2ptr;
		else if (type == MC_L3)
			return free_mcgid_pool[mcgid].l3ptr;
	}
	return NULL;
}



ca_int16_t aal_mc_repl_bit_alloc(repl_key_t *key)
{
	ca_int16_t i, free_idx = ARB_FIB_TBL_ENTRY_MAX;
	l2fe_arb_fib_tbl_entry_t arb_fib_entry = {0};

	for (i = 0; i < ARB_FIB_TBL_ENTRY_MAX; i++) {
		if (!free_repl_bits_pool[i].ref_cnt) {
			if (free_idx == ARB_FIB_TBL_ENTRY_MAX){				
				free_idx = i;
				break;
			}
		} else if (memcmp(&free_repl_bits_pool[i].key, key, sizeof(repl_key_t)) == 0) {
			free_repl_bits_pool[i].ref_cnt++;
			return i;
		}
	}

	if (free_idx == ARB_FIB_TBL_ENTRY_MAX)
		return -1;

	free_repl_bits_pool[free_idx].ref_cnt = 1;
	memcpy(&free_repl_bits_pool[free_idx].key, key, sizeof(repl_key_t));

	arb_fib_entry.mcgid = 0;
	arb_fib_entry.mcgid_en = 0;
#if 1//defined(CONFIG_LUNA_G3_SERIES)		
	arb_fib_entry.pol_id = key->wifi_flowid;	// wifi fast forward if ldpid is 0x17
	arb_fib_entry.pol_grp_id = 0;
	arb_fib_entry.pol_en = 0;
	arb_fib_entry.cos = key->cos;
	arb_fib_entry.ip_ttl_dec_cmd = key->mac_sa_cmd;	
#else
	arb_fib_entry.pol_id = 0;
	arb_fib_entry.pol_grp_id = 0;
	arb_fib_entry.pol_en = 0;
	arb_fib_entry.cos = 0;
#endif
	arb_fib_entry.ldpid = key->ldpid;


	arb_fib_entry.mac_sa_sel = key->mac_sa_sel;

	arb_fib_entry.mac_sa_cmd = key->mac_sa_cmd;
	arb_fib_entry.permit_spid_en = 0;
	arb_fib_entry.wan_dst = (arb_fib_entry.ldpid == AAL_LPORT_ETH_WAN);
#if 1//defined(CONFIG_LUNA_G3_SERIES)
	//set cos_cmd=0 ==> enable
	arb_fib_entry.cos_cmd = MCFIB_COS_CMD_EN;
#else
	arb_fib_entry.cos_cmd = 1;
#endif
	arb_fib_entry.rsvd = 0;

	arb_fib_entry.mac_cmd = key->mac_da_cmd;
	arb_fib_entry.mac_da[0] = key->mac_da[5];
	arb_fib_entry.mac_da[1] = key->mac_da[4];
	arb_fib_entry.mac_da[2] = key->mac_da[3];
	arb_fib_entry.mac_da[3] = key->mac_da[2];
	arb_fib_entry.mac_da[4] = key->mac_da[1];
	arb_fib_entry.mac_da[5] = key->mac_da[0];

	if (key->vlan_cmd) {
		arb_fib_entry.dei = (key->vlan & 0x1000) >> 12;
		arb_fib_entry.dei_cmd = 1;
#if 1//defined(CONFIG_LUNA_G3_SERIES)		
		arb_fib_entry.dot1p = key->vpri;
#endif
		arb_fib_entry.dot1p_cmd = 1;
		arb_fib_entry.sc_ind = 0;
		arb_fib_entry.vid = (key->vlan & 0xFFF);
		arb_fib_entry.vlan_cmd = key->vlan_cmd;
	} else {
		arb_fib_entry.dei = 0;
		arb_fib_entry.dei_cmd = 0;
		arb_fib_entry.dot1p = 0;
		arb_fib_entry.dot1p_cmd = 0;
		arb_fib_entry.sc_ind = 0;
		arb_fib_entry.vid = 0;
		arb_fib_entry.vlan_cmd = 0;
	}
	arb_fib_entry.vlan_fltr_en = 0;
	aal_arb_fib_entry_add_by_idx(&arb_fib_entry, free_idx);

	return free_idx;
}

ca_int16_t aal_mc_repl_bit_get(repl_key_t *key)
{
	ca_int16_t i;

	for (i = 0; i < ARB_FIB_TBL_ENTRY_MAX; i++) {
		if (free_repl_bits_pool[i].ref_cnt &&
			memcmp(&free_repl_bits_pool[i].key, key, sizeof(repl_key_t)) == 0) {
			return i;
		}
	}

	return -1;
}

ca_status_t aal_mc_repl_bit_free(ca_int16_t port_bit)
{
	if (port_bit < 0 || port_bit >= ARB_FIB_TBL_ENTRY_MAX)
		return CA_E_ERROR;

	aal_mce_repl_bit_free(port_bit);

	return CA_E_OK;
}

ca_int16_t aal_mce_repl_bit_alloc(l2fe_arb_fib_tbl_entry_t *key)
{
	unsigned int idx;
	int ret;

	ret = aal_arb_fib_entry_add(key, &idx);

	if (ret == AAL_E_OK) {
	} else {
		return -1;
	}

	return idx;
}

ca_int16_t aal_mce_repl_bit_get(l2fe_arb_fib_tbl_entry_t *key)
{
	unsigned int idx;
	int ret;

	ret = aal_arb_fib_entry_get(key, &idx);

	if (ret == AAL_E_OK)
		return idx;
	else
		return -1;
}

ca_status_t aal_mce_repl_bit_free(ca_int16_t port_bit)
{

//hmmmmmmmm... Where is pool ref_cnt decrease ? why do not using generic_entry_del entry->refcnt to counting ?
#if 1//defined(CONFIG_LUNA_G3_SERIES)	
	if (free_repl_bits_pool[port_bit].ref_cnt>=1)
	{
		free_repl_bits_pool[port_bit].ref_cnt--;
		if(free_repl_bits_pool[port_bit].ref_cnt!=0)
			return CA_E_OK;
	}


#endif	
	if (aal_arb_fib_entry_del(port_bit)) {
		return CA_E_ERROR;
	} else {
		return CA_E_OK;
	}
}

ca_int16_t aal_mcgid_alloc(ni_mce_indx_tbl_entry_t *key)
{
	unsigned int idx;
	int ret;

	ret = aal_mce_indx_entry_add(key, &idx);

	if (ret == AAL_E_OK) {
	} else {
		return -1;
	}

	return idx;
}


ca_int16_t aal_mcgid_get(ni_mce_indx_tbl_entry_t *key)
{
	unsigned int idx;
	int ret;

	ret = aal_mce_indx_entry_get(key, &idx);

	if (ret == AAL_E_OK)
		return idx;
	else
		return -1;
}

ca_status_t aal_mcgid_free(ca_int16_t mcgid)
{
	if (aal_mce_indx_entry_del(mcgid)) {
		return CA_E_ERROR;
	} else {
		return CA_E_OK;
	}
}


aal_direct_table_t l2pe_my_mac_table = {
	.max_entry = 8,
	.first_reg = L2FE_PE_MY_MAC_LOW_0,
	.reg_num = 2,
	.reg_stride = 8
};


#if 1//defined(CONFIG_LUNA_G3_SERIES)

ca_status_t aal_l2fe_pe_my_mac_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t *entry_idx,
	CA_IN l2fe_pe_my_mac_tbl_entry_t *entry
)
{
	int i,firstInvalid=-1;
	for(i=0 ;  i<L2FE_PE_MYMAC_MAX ; i++)
	{
		if(free_l2fe_pe_mymac_pool[i].myMac_refCnt)
		{
			if(memcmp(entry->mac_addr,free_l2fe_pe_mymac_pool[i].mac_addr,sizeof(entry->mac_addr))==0)
			{
				//we have same entry here add refCnt and return
				free_l2fe_pe_mymac_pool[i].myMac_refCnt++;
				*entry_idx = i;
				return CA_E_OK;
			}
		}
	}

	return CA_E_NOT_FOUND;
	
}


ca_status_t aal_l2fe_pe_my_mac_add(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t *entry_idx,
	CA_IN l2fe_pe_my_mac_tbl_entry_t *entry
)
{
	int i,firstInvalid=-1;
	for(i=0 ; i<L2FE_PE_MYMAC_MAX ;i++)
	{
		if(free_l2fe_pe_mymac_pool[i].myMac_refCnt)
		{
			if(memcmp(entry->mac_addr,free_l2fe_pe_mymac_pool[i].mac_addr,sizeof(entry->mac_addr))==0)
			{
				//we have same entry here add refCnt and return
				free_l2fe_pe_mymac_pool[i].myMac_refCnt++;
				*entry_idx = i;
				return CA_E_OK;
			}
		}
		else
		{
			if(firstInvalid==-1)
				firstInvalid=i;
		}
	}
	if(firstInvalid==-1 )
		return CA_E_FULL;

	memcpy(free_l2fe_pe_mymac_pool[firstInvalid].mac_addr,entry->mac_addr,sizeof(entry->mac_addr));
	free_l2fe_pe_mymac_pool[firstInvalid].myMac_refCnt=1;
	aal_direct_table_set(&l2pe_my_mac_table, firstInvalid, entry, AAL_TABLE_DIR_LOW_TO_HI);
	*entry_idx=firstInvalid;
	
	return CA_E_OK;
	
}

ca_status_t aal_l2fe_pe_my_mac_del(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx
)
{

	if(free_l2fe_pe_mymac_pool[entry_idx].myMac_refCnt)
	{
		free_l2fe_pe_mymac_pool[entry_idx].myMac_refCnt--;
		return CA_E_OK;
	}
	else
		return CA_E_NOT_FOUND;	
}

#endif


ca_status_t aal_l2fe_pe_my_mac_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l2fe_pe_my_mac_tbl_entry_t *entry
)
{
	return aal_direct_table_set(&l2pe_my_mac_table, entry_idx, entry, AAL_TABLE_DIR_LOW_TO_HI);
}

ca_status_t aal_mc_ni_mce_indx_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT ni_mce_indx_tbl_entry_t *entry
)
{
	return hw_entry_get(&ni_mce_indx_table, entry_idx, entry);
};

ca_status_t aal_mc_ni_mce_indx_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN ni_mce_indx_tbl_entry_t *entry
)
{
	ni_mce_indx_tbl_entry_t tmp_entry;

	if (AAL_E_OK != aal_mce_indx_entry_get_by_idx(entry_idx, &tmp_entry))
		return aal_mce_indx_entry_add_by_idx(entry, entry_idx);
	else
		return aal_mce_indx_entry_set(entry_idx, entry);
};

ca_status_t aal_mc_l2fe_arb_fib_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l2fe_arb_fib_tbl_entry_t *entry
)
{
	return hw_entry_get(&l2fe_arb_fib_table, entry_idx, entry);
};

ca_status_t aal_mc_l2fe_arb_fib_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l2fe_arb_fib_tbl_entry_t *entry
)
{
	l2fe_arb_fib_tbl_entry_t tmp_entry;

	if (AAL_E_OK != aal_arb_fib_entry_get_by_idx(entry_idx, &tmp_entry))
		return aal_arb_fib_entry_add_by_idx(entry, entry_idx);
	else
		return aal_arb_fib_entry_set(entry_idx, entry);
};

#if 0 /*TBD for RTL8277C AAL*/


ca_status_t aal_mc_im3e_mldda_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT im3e_mldda_entry_t *entry
)
{
	int ret;

	ret = aal_im3e_mldda_entry_get_by_idx(entry_idx, entry);
	if (ret != AAL_E_OK)
		memset(entry, 0, sizeof(im3e_mldda_entry_t));

	return AAL_E_OK;
};

ca_status_t aal_mc_im3e_mldda_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN im3e_mldda_entry_t *entry
)
{
	im3e_mldda_entry_t tmp_entry;

	if (AAL_E_OK != aal_im3e_mldda_entry_get_by_idx(entry_idx, &tmp_entry)) {
		return aal_im3e_mldda_entry_add_by_idx(entry, entry_idx);
	} else {
		if (entry->entry_valid)
			return aal_im3e_mldda_entry_set(entry_idx, entry);
		else
			return aal_im3e_mldda_entry_del(entry_idx);
	}
};

ca_status_t aal_mc_im3e_mldsa_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT im3e_mldsa_entry_t *entry
)
{
	int ret;

	ret = aal_im3e_mldsa_entry_get_by_idx(entry_idx, entry);
	if (ret != AAL_E_OK)
		memset(entry, 0, sizeof(im3e_mldsa_entry_t));

	return AAL_E_OK;
};

ca_status_t aal_mc_im3e_mldsa_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN im3e_mldsa_entry_t *entry
)
{
	im3e_mldsa_entry_t tmp_entry;

	if (AAL_E_OK != aal_im3e_mldsa_entry_get_by_idx(entry_idx, &tmp_entry)) {
		return aal_im3e_mldsa_entry_add_by_idx(entry, entry_idx);
	} else {
		if (entry->entry_valid)
			return aal_im3e_mldsa_entry_set(entry_idx, entry);
		else
			return aal_im3e_mldsa_entry_del(entry_idx);
	}
};

#endif /*TBD for RTL8277C AAL*/

ca_status_t aal_mc_ni_mce_indx_dump(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx
)
{
	ni_mce_indx_tbl_entry_t entry;
	ca_status_t ret = 0;

	memset(&entry, 0, sizeof(entry));

	ret =  hw_entry_get(&ni_mce_indx_table, entry_idx, &entry);
	if(AAL_E_OK != ret)
		return CA_E_PARAM;

	ca_printf("-mcgid: %d vector: 0x%016llx\r\n", entry_idx, entry.mc_vec);
	return CA_E_OK;
};


EXPORT_SYMBOL(aal_mc_ni_mce_indx_get);
EXPORT_SYMBOL(aal_mc_ni_mce_indx_set);
EXPORT_SYMBOL(aal_mc_l2fe_arb_fib_get);
EXPORT_SYMBOL(aal_mc_l2fe_arb_fib_set);
#if 0 /*TBD for RTL8277C AAL*/
EXPORT_SYMBOL(aal_mc_im3e_mldda_get);
EXPORT_SYMBOL(aal_mc_im3e_mldda_set);
EXPORT_SYMBOL(aal_mc_im3e_mldsa_get);
EXPORT_SYMBOL(aal_mc_im3e_mldsa_set);
#endif

EXPORT_SYMBOL(aal_mc_ni_mce_indx_dump);
#if 1//defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_l2fe_pe_my_mac_get);
EXPORT_SYMBOL(aal_l2fe_pe_my_mac_add);
EXPORT_SYMBOL(aal_l2fe_pe_my_mac_del);
EXPORT_SYMBOL(aal_mcgid_alloc);
EXPORT_SYMBOL(aal_mcgid_free);
EXPORT_SYMBOL(aal_mce_repl_bit_free);
EXPORT_SYMBOL(aal_mc_repl_bit_alloc);

#endif
