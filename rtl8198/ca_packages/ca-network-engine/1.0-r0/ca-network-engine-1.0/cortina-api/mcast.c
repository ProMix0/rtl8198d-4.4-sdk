#include <linux/list.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include "ca_types.h"
#include "aal_common.h"
#include "ca_aal_proc.h"
#include "aal_table.h"
#include "aal_port.h"
#include "aal_mcast.h"
#include "api_common.h"
#include "mcast.h"
#include "ca_aal.h"
#include "port.h"
#include "ca_mut.h"
#include "ca-scfg/scfg.h"

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
#include "aal_hashlite.h"
#endif

#ifdef CONFIG_NE_L3_MCAST
#include "route.h"
#include "aal_hash.h"
#include "aal_l3fe.h"
#endif

#include "l2.h"

/* NOTE:
 * see comment before the commit 6908538de6077d15cc0edb1252b62fdc3cb29bf4
 */


#define MC_DEBUG(fmt,...)	do { \
		if (ca_api_debug & API_DBG_L3_MCAST) { \
			ca_printf(KERN_INFO "%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#define MC_ERROR(fmt,...)	do { \
		if (ca_api_debug & API_DBG_L2_MCAST) { \
			ca_printf(KERN_INFO "[ERROR!]%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)


#define VID(vlan) (vlan & 0xFFF)
#define MAX_FLOODING_PORTS CFG_ID_FLOODING_DOMAIN_1_LEN

#define INTF_VLAN_A CA_UINT16_INVALID

typedef enum {
	MCG = 0,
	SRC = 1
} node_type_t;

#define NODE_STRUCT \
	struct list_head	list; \
	node_type_t		type; \
	ca_uint16_t		mcgid; \
	ca_uint16_t		mce_indx; \
	ca_uint32_t		entry_idx;

#define L2_NODE_STRUCT \
	NODE_STRUCT \
	struct list_head	port_list; \
	struct list_head	l3_port_list;

#define L3_NODE_STRUCT \
	NODE_STRUCT \
	struct list_head	intf_list;

#define CVLAN 0
#define SVLAN 1
typedef struct node_obj_s {
	NODE_STRUCT
} node_obj_t;

typedef struct l2_node_obj_s {
	L2_NODE_STRUCT
} l2_node_obj_t;

typedef struct l3_node_obj_s {
	L3_NODE_STRUCT
} l3_node_obj_t;

typedef struct port_node_s {
	struct list_head		list;
	repl_key_t			key;
	ca_uint16_t			api_port_type;
	ca_uint16_t			api_vlan_cmd;
	ca_uint16_t			api_vlan;
	ca_int16_t			repl_bit;
	struct port_node_s		*l3_port_node;
} port_node_t;

typedef struct l2_src_node_s {
	L2_NODE_STRUCT
	ca_uint32_t			*parent;
	ca_ip_address_t			src_addr;
} l2_src_node_t;

typedef struct l2_mcg_node_s {
	L2_NODE_STRUCT
	ca_uint16_t			llid;
	ca_uint16_t			vlan;
	ca_ip_address_t			grp_addr;
	ca_mac_addr_t			mac_addr;
	ca_boolean_t			is_api_own;
#if defined(CONFIG_LUNA_G3_SERIES)	&& defined(CONFIG_RTK_G3_MC_L3FE_NEW_HWPATH)
	ca_boolean_t			is_knownMcGroup;
	ca_ip_address_t			src_addr;
	ca_uint16_t				mc_llid;
	ca_uint32_t				hash_idx;
#endif
	struct list_head		l2_src_list;
} l2_mcg_node_t;

typedef struct intf_node_s {
	struct list_head		list;
	ca_intf_id_t			intf_id;
	ca_mac_addr_t			new_mac_da;
	ca_uint16_t			port_id;
	ca_int16_t			repl_bit;
	ca_int16_t			flooding_repl_bit[MAX_FLOODING_PORTS];
	ca_uint16_t			vlan;
} intf_node_t;

typedef struct l3_src_node_s {
	L3_NODE_STRUCT
	ca_uint32_t			*parent;
	ca_ip_address_t			src_addr;
} l3_src_node_t;

typedef struct l3_mcg_node_s {
	L3_NODE_STRUCT
	ca_uint16_t			vlan;
	ca_ip_address_t			grp_addr;
	ca_boolean_t			is_api_own;
	ca_uint32_t			hash_idx;
	struct list_head		l3_src_list;
#if defined(CONFIG_LUNA_G3_SERIES)
	ca_boolean_t			is_knownMcGroup;
#endif
} l3_mcg_node_t;

spinlock_t mcast_lock;

static LIST_HEAD(l3_mcg_list);
static LIST_HEAD(l2_mcg_list);
extern ca_uint32_t igmp_snooping_on;
extern ca_uint32_t mcast_mode;
extern ca_uint32_t wan2lan_only_igmp;
extern ca_uint32_t igmp_use_mc_vlan;
extern ca_uint32_t mld_use_mc_vlan;

static ca_mac_addr_t ca_ne_mac_addr1; /* MAC address of LAN port */
#if defined(CONFIG_LUNA_G3_SERIES)
static ca_uint8_t l3fe_hash_action_indexed_mode = 0;
#endif

#ifdef CONFIG_NE_L3_MCAST
static ca_status_t l3_intfs_expand(l3_node_obj_t *node);
static ca_status_t l3_intfs_expand_all_by_rule(ca_ip_address_t grp_addr, ca_ip_address_t src_addr);
#endif
static ca_status_t l2_ports_expand(l2_node_obj_t *node);

static inline ca_boolean_t api_level_vlan_range_valid(ca_uint16_t vlan)
{
	return (vlan == CA_UINT16_INVALID || !(vlan & 0xF000));
}

static inline ca_boolean_t vlan_is_valid(ca_uint16_t vlan)
{
	return !(vlan & 0xF000);
}

static inline ca_uint16_t vlan_to_hw_vid(ca_uint16_t vlan)
{
	ca_uint16_t hw_vid;

	if (vlan_is_valid(vlan))
		hw_vid = VID(vlan);
	else
		hw_vid = 0;

	return hw_vid;
}

static inline ca_boolean_t mac_is_mcast(ca_mac_addr_t addr)
{
	return (addr[0] & 0x1) == 0x1;
}

static inline ca_boolean_t ipv4_is_mcast(ca_l3_ip_addr_t addr)
{
	return (addr.ipv4_addr & 0xF0000000) == 0xE0000000;
}

static inline ca_boolean_t ipv6_is_mcast(ca_l3_ip_addr_t addr)
{
	return (addr.ipv6_addr[0] & 0xFF000000) == 0xFF000000;
}

static ca_int16_t ip_addr_compare(ca_ip_address_t *a, ca_ip_address_t *b)
{
	if (a->afi == CA_IPV4 && b->afi == CA_IPV4) {
		if (a->ip_addr.ipv4_addr == b->ip_addr.ipv4_addr)
			return 0;
	} else if (a->afi == CA_IPV6 && b->afi == CA_IPV6){
		if (memcmp(a->ip_addr.ipv6_addr, b->ip_addr.ipv6_addr, 16) == 0)
			return 0;
	}

	return -1;
}

static port_node_t* l2_port_node_alloc(repl_key_t *real_key)
{
	port_node_t *port_node;
	ca_int16_t bit;

	MC_DEBUG("\n");
#if defined(CONFIG_LUNA_G3_SERIES)
	real_key->cos_cmd = MCFIB_COS_CMD_EN;
#else	
	real_key->cos_cmd = 1;
	real_key->mac_sa_sel = L2_PE_MAC_SA_LAN_IDX;
#endif
	bit = aal_mc_repl_bit_alloc(real_key);
	if (bit < 0)
		return NULL;

	port_node = ca_zalloc_trace(sizeof(port_node_t), GFP_KERNEL, true);
	if (port_node == NULL)
		return NULL;

	memcpy(&port_node->key, real_key, sizeof(repl_key_t));
	port_node->repl_bit = bit;

	return port_node;
}

static port_node_t* l2_port_node_get(l2_node_obj_t *node, repl_key_t *fake_key)
{
	port_node_t *port_node;

	MC_DEBUG("\n");
	if (!node)
		return NULL;

	if (wan2lan_only_igmp) {
	if (!list_empty(&node->port_list))
		list_for_each_entry(port_node, &node->port_list, list) {
			if (port_node->key.ldpid == fake_key->ldpid &&
					/*port_node->key.mac_sa_cmd == fake_key->mac_sa_cmd &&*/
				port_node->key.mac_da_cmd == fake_key->mac_da_cmd &&
				memcmp(port_node->key.mac_da, fake_key->mac_da, sizeof(ca_mac_addr_t)) == 0 &&
				port_node->api_vlan_cmd == fake_key->vlan_cmd &&
				port_node->api_vlan == fake_key->vlan) {
				return port_node;
			}
		}
	} else {

#if defined(CONFIG_LUNA_G3_SERIES)
	fake_key->cos_cmd = MCFIB_COS_CMD_EN;
	//memcmp full info
#else
	fake_key->cos_cmd = 1;
	fake_key->mac_sa_sel = L2_PE_MAC_SA_LAN_IDX;
#endif


	if (!list_empty(&node->port_list))
		list_for_each_entry(port_node, &node->port_list, list) {
			if (port_node->key.ldpid == fake_key->ldpid &&
				memcmp(&port_node->key, fake_key, sizeof(repl_key_t)) == 0)
				return port_node;
		}
	}

	return NULL;
}

static ca_status_t l2_port_node_delete(port_node_t *port_node)
{
	MC_DEBUG("\n");
	if (port_node == NULL)
		return CA_E_PARAM;

#ifdef CONFIG_NE_L3_MCAST
	if (port_node->l3_port_node) {
		list_del(&port_node->l3_port_node->list);
		aal_mce_repl_bit_free(port_node->l3_port_node->repl_bit);
		ca_free(port_node->l3_port_node);
	}
#endif
	list_del(&port_node->list);
	aal_mce_repl_bit_free(port_node->repl_bit);
	ca_free(port_node);

	return CA_E_OK;
}

static ca_status_t l2_port_node_delete_all(l2_node_obj_t *node)
{
	port_node_t *port_node, *tmp;

	MC_DEBUG("\n");
	if (!list_empty(&node->port_list))
		list_for_each_entry_safe(port_node, tmp, &node->port_list, list) {
			l2_port_node_delete(port_node);
		}

#ifdef CONFIG_NE_L3_MCAST
	if (!wan2lan_only_igmp && !list_empty(&node->l3_port_list))
		list_for_each_entry_safe(port_node, tmp, &node->l3_port_list, list) {
			l2_port_node_delete(port_node);
		}
#endif

	return CA_E_OK;
}


static ca_status_t l2_port_member_add(ca_l2_mcast_group_members_t *members)
{
	l2_node_obj_t *node;
	l2_mcg_node_t *mcg_node;
	l2_src_node_t *src_node;
	port_node_t *port_node = NULL;

	repl_key_t key;
	repl_key_t l2_key;

#ifdef CONFIG_NE_L3_MCAST
	repl_key_t l3_key;
#endif
	ca_uint32_t i, has_intf_vlan;
	ca_ip_address_t src_addr = {0};

	MC_DEBUG("\n");
		node = (l2_node_obj_t *) aal_mc_fake_mcgid_ptr_get(members->mcg_id, MC_L2);

	if (node == NULL)
		return CA_E_NOT_FOUND;

	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
		if (!mcg_node->is_api_own)
			return CA_E_NOT_FOUND;
	} else {
		src_node = (l2_src_node_t *) node;
		mcg_node = (l2_mcg_node_t *) src_node->parent;
		src_addr = src_node->src_addr;
	}
	has_intf_vlan = vlan_is_valid(mcg_node->vlan);

	if (wan2lan_only_igmp) 
	{
		for (i = 0; i < members->member_count; i++) 
		{
			memset(&key, 0, sizeof(repl_key_t));
			key.ldpid = PORT_ID(members->member[i].member_port);
			if (mcast_mode == CA_MC_MODE_IP)
				key.mac_sa_cmd = 1;
			else
				key.mac_sa_cmd = 0;
			if (members->member[i].action_mask.mac_da_overwrite) {
				key.mac_da_cmd = 1;
				memcpy(key.mac_da, members->member[i].new_mac_da, sizeof(ca_mac_addr_t));
			}
			if (members->member[i].action_mask.vlan_action) {
				key.vlan_cmd = members->member[i].vlan_action;
				key.vlan = members->member[i].vid;
			}

			if (l2_port_node_get(node, &key))
				continue;

			if (mcast_mode == CA_MC_MODE_IP) {
				switch(members->member[i].vlan_action)
				{
				case CA_L2_VLAN_TAG_ACTION_NOP:
					if (has_intf_vlan) {
						key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
						key.vlan = vlan_to_hw_vid(mcg_node->vlan);
					} else {
						key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_POP;
						key.vlan = 0;
					}
					break;
				case CA_L2_VLAN_TAG_ACTION_PUSH:
					if (has_intf_vlan) {
						return CA_E_PARAM;
					} else {
						key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
						key.vlan = vlan_to_hw_vid(members->member[i].vid);
					}
					break;
				case CA_L2_VLAN_TAG_ACTION_POP:
					key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_POP;
					key.vlan = 0;
					break;
				case CA_L2_VLAN_TAG_ACTION_SWAP:
					key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
					key.vlan = vlan_to_hw_vid(members->member[i].vid);
					break;
				default:
					return CA_E_PARAM;
				}
			} else {
				key.vlan_cmd = members->member[i].vlan_action;
				key.vlan = vlan_to_hw_vid(members->member[i].vid);
			}

			port_node = l2_port_node_alloc(&key);
			if (port_node == NULL)
				return CA_E_FULL;
			port_node->api_port_type = PORT_TYPE(members->member[i].member_port);
			port_node->api_vlan_cmd = members->member[i].vlan_action;
			port_node->api_vlan = members->member[i].vid;

			list_add(&port_node->list, &node->port_list);
		}
	} 
	else 
	{
		for (i = 0; i < members->member_count; i++) 
		{
			memset(&l2_key, 0, sizeof(repl_key_t));
			l2_key.ldpid = PORT_ID(members->member[i].member_port);

#if defined(CONFIG_LUNA_G3_SERIES)
			//printk("mcgid %d member[%d] port 0x%x wifi flow id %d\n", members->mcg_id, i, members->member[i].member_port, members->member[i].wifi_flowid);
			if(l2_key.ldpid == AAL_LPORT_CPU_7){
				l2_key.wifi_flowid = members->member[i].wifi_flowid;
			}
			l2_key.cos = members->member[i].cos;
#endif

			l2_key.mac_sa_cmd = 0;
			if (members->member[i].action_mask.mac_da_overwrite) {
				l2_key.mac_da_cmd = 1;
				memcpy(l2_key.mac_da, members->member[i].new_mac_da, sizeof(ca_mac_addr_t));
			}
			if (members->member[i].action_mask.vlan_action) {
				l2_key.vlan_cmd = members->member[i].vlan_action;
				l2_key.vlan = members->member[i].vid;
			}

			if (l2_port_node_get(node, &l2_key))
				continue;

			l2_key.vlan_cmd = members->member[i].vlan_action;
			l2_key.vlan = vlan_to_hw_vid(members->member[i].vid);

			port_node = l2_port_node_alloc(&l2_key);
			if (port_node == NULL)
				return CA_E_FULL;
			port_node->api_port_type = PORT_TYPE(members->member[i].member_port);
			port_node->api_vlan_cmd = members->member[i].vlan_action;
			port_node->api_vlan = members->member[i].vid;

			list_add(&port_node->list, &node->port_list);

#ifdef CONFIG_NE_L3_MCAST
			l3_key = l2_key;
#if defined(CONFIG_LUNA_G3_SERIES)
			l3_key.vpri = members->member[i].vpri;
			l3_key.mac_sa_cmd = members->member[i].action_mask.mac_sa_trans;
#else
			l3_key.mac_sa_cmd = 1;
#endif
			if (mcast_mode == CA_MC_MODE_IP) {
				switch(members->member[i].vlan_action)
				{
				case CA_L2_VLAN_TAG_ACTION_NOP:
					if (has_intf_vlan) {
						l3_key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
						l3_key.vlan = vlan_to_hw_vid(mcg_node->vlan);
					} else {
						l3_key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_POP;
						l3_key.vlan = 0;
					}
					break;
				case CA_L2_VLAN_TAG_ACTION_PUSH:
					if (has_intf_vlan) {
						return CA_E_PARAM;
					} else {
						l3_key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
						l3_key.vlan = vlan_to_hw_vid(members->member[i].vid);
					}
					break;
				case CA_L2_VLAN_TAG_ACTION_POP:
					l3_key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_POP;
					l3_key.vlan = 0;
					break;
				case CA_L2_VLAN_TAG_ACTION_SWAP:
					l3_key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
					l3_key.vlan = vlan_to_hw_vid(members->member[i].vid);
					break;
				default:
					return CA_E_PARAM;
				}
				port_node->l3_port_node = l2_port_node_alloc(&l3_key);
				if (port_node->l3_port_node == NULL)
					return CA_E_FULL;

				list_add(&port_node->l3_port_node->list, &node->l3_port_list);
			}
#endif
		}
	}

	if (mcast_mode == CA_MC_MODE_IP) {
#ifdef CONFIG_NE_L3_MCAST
		if (CA_E_OK != l3_intfs_expand_all_by_rule(mcg_node->grp_addr, src_addr)) {
			l2_port_node_delete(port_node);
			return CA_E_FULL;
		}
#endif
		if (!wan2lan_only_igmp && CA_E_OK != l2_ports_expand(node)) {
			l2_port_node_delete(port_node);
			return CA_E_FULL;
		}
	} else {
		if (CA_E_OK != l2_ports_expand(node)) {
			l2_port_node_delete(port_node);
			return CA_E_FULL;
		}
	}

	return CA_E_OK;
}

static ca_status_t l2_port_member_delete(ca_l2_mcast_group_members_t *members)
{
	l2_node_obj_t *node;
	l2_mcg_node_t *mcg_node;
	l2_src_node_t *src_node;
	port_node_t *port_node;
	repl_key_t key;
	ca_uint32_t i;
	ca_ip_address_t src_addr = {0};

	MC_DEBUG("\n");
		node = (l2_node_obj_t *) aal_mc_fake_mcgid_ptr_get(members->mcg_id, MC_L2);

	if (node == NULL)
		return CA_E_NOT_FOUND;

	for (i = 0; i < members->member_count; i++) {
		memset(&key, 0, sizeof(repl_key_t));
		key.ldpid = PORT_ID(members->member[i].member_port);
		if (members->member[i].action_mask.mac_da_overwrite) {
			key.mac_da_cmd = 1;
			memcpy(key.mac_da, members->member[i].new_mac_da, sizeof(ca_mac_addr_t));
		}
		if (members->member[i].action_mask.vlan_action) {
			key.vlan_cmd = members->member[i].vlan_action;
			key.vlan = members->member[i].vid;
		}
#if defined(CONFIG_LUNA_G3_SERIES)	
		key.vpri = members->member[i].vpri;
		key.cos = members->member[i].cos;
		key.wifi_flowid = members->member[i].wifi_flowid;
#endif
		key.mac_sa_cmd = 0;
		port_node = l2_port_node_get(node, &key);
		if (port_node != NULL)
			l2_port_node_delete(port_node);
	}
	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
	} else {
		src_node = (l2_src_node_t *) node;
		mcg_node = (l2_mcg_node_t *) src_node->parent;
		src_addr = src_node->src_addr;
	}
	if (mcast_mode == CA_MC_MODE_IP) {
#ifdef CONFIG_NE_L3_MCAST
		if (CA_E_OK != l3_intfs_expand_all_by_rule(mcg_node->grp_addr, src_addr)) {
			l2_port_node_alloc(&key);
			return CA_E_ERROR;
		}
#endif
		if (!wan2lan_only_igmp && CA_E_OK != l2_ports_expand(node)) {
			l2_port_node_alloc(&key);
			return CA_E_ERROR;
		}
	} else {
		if (CA_E_OK != l2_ports_expand(node)) {
			l2_port_node_alloc(&key);
			return CA_E_ERROR;
		}
	}

	return CA_E_OK;
}

static l2_mcg_node_t* l2_mcg_node_get(ca_uint16_t vlan, ca_ip_address_t grp_addr, ca_mac_addr_t mac_addr, ca_uint16_t llid)
{
	l2_mcg_node_t *cur_node;
	struct list_head *tmp;

	MC_DEBUG("\n");

	list_for_each(tmp, &l2_mcg_list) {
		cur_node = list_entry(tmp, l2_mcg_node_t, list);

		if (mcast_mode == CA_MC_MODE_IP) {
			if (cur_node->vlan == vlan && ip_addr_compare(&cur_node->grp_addr, &grp_addr) == 0) {
				return (l2_mcg_node_t *)tmp;
			}
		} else if (mcast_mode == CA_MC_MODE_MAC) {
			if (cur_node->vlan == vlan && memcmp(cur_node->mac_addr, mac_addr, sizeof(ca_mac_addr_t)) == 0) {
				return (l2_mcg_node_t *)tmp;
			}
		} else { /* LLID */
			if (cur_node->llid == llid) {
				return (l2_mcg_node_t *)tmp;
			}
		}
	}

	return NULL;
}



static l2_mcg_node_t* l2_mcg_node_alloc(ca_uint16_t vlan, ca_ip_address_t grp_addr, ca_mac_addr_t mac_addr, ca_uint16_t llid)
{
	l2_mcg_node_t *mcg_node;
	ca_int16_t mcgid = 0, mce_indx;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	ca_l2_addr_entry_t fdb_entry;
	im3e_mldda_entry_t da_entry;
	ca_uint32_t rsult_idx=CA_UINT32_INVALID;

	MC_DEBUG("\n");

	if (mcast_mode != CA_MC_MODE_LLID)
		mcgid = aal_mc_fake_mcgid_alloc(MC_L2);

	if (mcgid == -1)
		return NULL;

	if (mcast_mode == CA_MC_MODE_MAC) {
		mce_indx = aal_mcgid_alloc(&repl_bits);
		if (mce_indx == -1)
			return NULL;

		memset(&fdb_entry, 0, sizeof(ca_l2_addr_entry_t));
		memcpy(fdb_entry.mac_addr, mac_addr, sizeof(ca_mac_addr_t));
		fdb_entry.vid = vlan_to_hw_vid(vlan);
		fdb_entry.static_flag = 1;
		fdb_entry.da_permit = 1;
		fdb_entry.mc_group_id = mce_indx;
		fdb_entry.port_id = CA_UINT32_INVALID;
		if (ca_l2_addr_add(0, &fdb_entry)) {
			aal_mcgid_free(mce_indx);
			aal_mc_fake_mcgid_free(mcgid, MC_L2);
			return NULL;
		}
	}
	else if (!wan2lan_only_igmp && mcast_mode == CA_MC_MODE_IP) {
		mce_indx = aal_mcgid_alloc(&repl_bits);
		if (mce_indx == -1)
			return NULL;

		memset(&da_entry, 0, sizeof(im3e_mldda_entry_t));
		da_entry.entry_valid = 1;
		da_entry.mc_group_id = mce_indx;
		da_entry.v3_ind = 0;
		da_entry.connection_id = mcgid;
		if (grp_addr.afi == CA_IPV4) {
			da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv4_addr;
			da_entry.ipv6_da_1 = 0;
			da_entry.ipv6_da_2 = 0;
			da_entry.ipv6_da_3 = 0;
			da_entry.ipv6_flg = 0;
		} else {
			da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv6_addr[3];
			da_entry.ipv6_da_1 = grp_addr.ip_addr.ipv6_addr[2];
			da_entry.ipv6_da_2 = grp_addr.ip_addr.ipv6_addr[1];
			da_entry.ipv6_da_3 = grp_addr.ip_addr.ipv6_addr[0];
			da_entry.ipv6_flg = 1;
		}
		da_entry.vid = vlan_to_hw_vid(vlan);
		da_entry.vlan_sc_ind = da_entry.vid > 0 ? SVLAN : CVLAN;
		da_entry.reserved = 0;
		if (aal_entry_add(AAL_TABLE_IM3E_MLDDA, &da_entry, &rsult_idx)) {
			aal_mcgid_free(mce_indx);
			aal_mc_fake_mcgid_free(mcgid, MC_L2);
			return NULL;
		}
	} else if (mcast_mode == CA_MC_MODE_LLID) {
		mcgid = mce_indx = aal_mcgid_free_entry_alloc();
		if (mce_indx == -1)
			return NULL;
	}

	mcg_node = ca_zalloc_trace(sizeof(l2_mcg_node_t), GFP_KERNEL, true);
		if (mcg_node == NULL) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L2);
			return NULL;
		}
		aal_mc_fake_mcgid_set_callback(mcgid, MC_L2, (ca_int8_t *) mcg_node);

	mcg_node->type = MCG;
	mcg_node->llid = llid;
	mcg_node->vlan = vlan;
	if (mcast_mode == CA_MC_MODE_IP) {
		mcg_node->grp_addr = grp_addr;
	} else {
		memcpy(mcg_node->mac_addr, mac_addr, sizeof(ca_mac_addr_t));
	}
	mcg_node->mcgid = mcgid;
	mcg_node->mce_indx = mce_indx;
	INIT_LIST_HEAD(&mcg_node->port_list);
	if (!wan2lan_only_igmp)
	INIT_LIST_HEAD(&mcg_node->l3_port_list);
	INIT_LIST_HEAD(&mcg_node->l2_src_list);

	list_add(&mcg_node->list, &l2_mcg_list);

	if (!wan2lan_only_igmp)
	mcg_node->entry_idx = rsult_idx;

	return mcg_node;
}

static ca_status_t l2_mcg_node_delete(l2_mcg_node_t *mcg_node)
{
#ifdef CONFIG_NE_L3_MCAST
	ca_ip_address_t src_addr = {0};
#endif

	MC_DEBUG("\n");
	if (mcg_node->is_api_own) {
		mcg_node->is_api_own = FALSE;
		l2_port_node_delete_all((l2_node_obj_t *) mcg_node);
#ifdef CONFIG_NE_L3_MCAST
		if (CA_E_OK != l3_intfs_expand_all_by_rule(mcg_node->grp_addr, src_addr))
			return CA_E_ERROR;
#endif
	}
	if (list_empty(&mcg_node->l2_src_list)) {
		list_del(&mcg_node->list);
		if (mcast_mode == CA_MC_MODE_IP) {
			if (wan2lan_only_igmp) {
				aal_mc_fake_mcgid_free(mcg_node->mcgid, MC_L2);
			} else {
			aal_mcgid_free(mcg_node->mce_indx);
				aal_mc_fake_mcgid_free(mcg_node->mcgid, MC_L2);
			aal_entry_del(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx);
			}
		} else if (mcast_mode == CA_MC_MODE_MAC) {
			aal_mcgid_free(mcg_node->mce_indx);
			aal_mc_fake_mcgid_free(mcg_node->mcgid, MC_L2);
			ca_l2_addr_delete_by_mac(0, mcg_node->mac_addr, CA_L2_ADDR_OPERATION_STATIC);
		} else { /* LLID */
			aal_mcgid_free(mcg_node->mce_indx);
			aal_mc_fake_mcgid_free(mcg_node->mcgid, MC_L2);
		}
		ca_free(mcg_node);
	}

	return CA_E_OK;
}

static l2_src_node_t* l2_src_node_get(l2_mcg_node_t *mcg_node, ca_ip_address_t src_addr)
{
	l2_src_node_t *src_node;

	MC_DEBUG("\n");
	if (!mcg_node)
		return NULL;

	if (!list_empty(&mcg_node->l2_src_list))
		list_for_each_entry(src_node, &mcg_node->l2_src_list, list) {
			if (ip_addr_compare(&src_node->src_addr, &src_addr) == 0) {
				return src_node;
			}
		}

	return NULL;
}

static l2_src_node_t* l2_src_node_add(l2_mcg_node_t *mcg_node, ca_ip_address_t src_addr)
{
	l2_src_node_t *src_node;
	ca_int16_t mcgid, mce_indx, need_toggle = 0;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	im3e_mldda_entry_t da_entry;
	im3e_mldsa_entry_t sa_entry;
	ca_uint32_t rsult_idx = 0;

	MC_DEBUG("\n");
	if (!mcg_node)
		return NULL;

	if (mcast_mode == CA_MC_MODE_MAC)
		return NULL;

	mcgid = aal_mc_fake_mcgid_alloc(MC_L2);

	if (mcgid == -1)
		return NULL;

	if (!wan2lan_only_igmp && mcast_mode == CA_MC_MODE_IP) {
	mce_indx = aal_mcgid_alloc(&repl_bits);
	if (mce_indx == -1)
		return NULL;

	sa_entry.entry_valid = 1;
	sa_entry.mc_group_id = mce_indx;
	sa_entry.connection_id = mcg_node->mcgid;
	if (src_addr.afi == CA_IPV4) {
		sa_entry.ipv6_sa_0 = src_addr.ip_addr.ipv4_addr;
		sa_entry.ipv6_sa_1 = 0;
		sa_entry.ipv6_sa_2 = 0;
		sa_entry.ipv6_sa_3 = 0;
	} else {
		sa_entry.ipv6_sa_0 = src_addr.ip_addr.ipv6_addr[3];
		sa_entry.ipv6_sa_1 = src_addr.ip_addr.ipv6_addr[2];
		sa_entry.ipv6_sa_2 = src_addr.ip_addr.ipv6_addr[1];
		sa_entry.ipv6_sa_3 = src_addr.ip_addr.ipv6_addr[0];
	}
	sa_entry.reserved = 0;
	if (aal_entry_add(AAL_TABLE_IM3E_MLDSA, &sa_entry, &rsult_idx)) {
		aal_mcgid_free(mce_indx);
			aal_mc_fake_mcgid_free(mcgid, MC_L2);
			return NULL;
		}
	} else if (mcast_mode == CA_MC_MODE_LLID) {
		mce_indx = aal_mcgid_free_entry_alloc();
		if (mce_indx == -1)
		return NULL;
	}

	src_node = ca_zalloc_trace(sizeof(l2_src_node_t), GFP_KERNEL, true);
	if (src_node == NULL) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L2);
		return NULL;
	}
	aal_mc_fake_mcgid_set_callback(mcgid, MC_L2, (ca_int8_t *) src_node);

	src_node->type = SRC;
	src_node->parent = (ca_uint32_t *) mcg_node;
	src_node->src_addr = src_addr;
	src_node->mcgid = mcgid;
	src_node->mce_indx = mce_indx;
	INIT_LIST_HEAD(&src_node->port_list);
	if (!wan2lan_only_igmp)
	INIT_LIST_HEAD(&src_node->l3_port_list);


	if (!wan2lan_only_igmp && list_empty(&mcg_node->l2_src_list) && mcast_mode == CA_MC_MODE_IP)
		need_toggle = 1;

	list_add(&src_node->list, &mcg_node->l2_src_list);
	if (!wan2lan_only_igmp) {
	src_node->entry_idx = rsult_idx;

	if (need_toggle) {
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
		da_entry.v3_ind = 1;
		aal_entry_set(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
	}
	}

	return src_node;
}

static ca_status_t l2_src_node_delete(l2_src_node_t *src_node)
{
	l2_mcg_node_t *mcg_node;
	ca_ip_address_t src_addr;

	MC_DEBUG("\n");
	mcg_node = (l2_mcg_node_t *) src_node->parent;

	memcpy(&src_addr, &src_node->src_addr, sizeof(ca_ip_address_t));

	l2_port_node_delete_all((l2_node_obj_t *) src_node);
	list_del(&src_node->list);
	if (wan2lan_only_igmp) {
		aal_mc_fake_mcgid_free(src_node->mcgid, MC_L2);
	} else {
		if (mcast_mode == CA_MC_MODE_IP)
	aal_entry_del(AAL_TABLE_IM3E_MLDSA, src_node->entry_idx);
	aal_mcgid_free(src_node->mce_indx);
		aal_mc_fake_mcgid_free(src_node->mcgid, MC_L2);
	}
	ca_free(src_node);

#ifdef CONFIG_NE_L3_MCAST
	if (CA_E_OK != l3_intfs_expand_all_by_rule(mcg_node->grp_addr, src_addr))
		return CA_E_ERROR;
#endif

	if (list_empty(&mcg_node->l2_src_list) && !mcg_node->is_api_own) {
		l2_mcg_node_delete(mcg_node);
	}

	return CA_E_OK;
}

struct list_head * l2_port_list_search(ca_uint16_t vlan, ca_ip_address_t grp_addr, node_type_t type, ca_ip_address_t src_addr)
{
	l2_mcg_node_t *mcg_node;
	l2_src_node_t *src_node;
	ca_mac_addr_t zero_mac = {0};

	MC_DEBUG("\n");
	mcg_node = l2_mcg_node_get(vlan, grp_addr, zero_mac, 0);
	if (mcg_node == NULL)
		return NULL;
	if (type == MCG)
		return (wan2lan_only_igmp)? &mcg_node->port_list: &mcg_node->l3_port_list;

	src_node = l2_src_node_get(mcg_node, src_addr);
	if (src_node == NULL)
		return NULL;
	return (wan2lan_only_igmp)? &src_node->port_list: &src_node->l3_port_list;
}

#ifdef CONFIG_NE_L3_MCAST
static intf_node_t* l3_intf_node_alloc(ca_intf_id_t intf_id, ca_mac_addr_t new_mac_da, ca_uint32_t fvld)
{
	intf_node_t *intf_node;
	ca_l3_intf_t intf_entry;
	repl_key_t key = {0};
	l2fe_pe_my_mac_tbl_entry_t my_mac = {{0}};
	ca_uint32_t i, j;
	ca_uint32_t flooding_ports[MAX_FLOODING_PORTS];

	MC_DEBUG("\n");
	intf_node = ca_zalloc_trace(sizeof(intf_node_t), GFP_KERNEL, true);
	if (intf_node == NULL)
		return NULL;

	intf_node->intf_id = intf_id;
	memcpy(intf_node->new_mac_da, new_mac_da, sizeof(ca_mac_addr_t));
	intf_entry.intf_id = intf_id;
	if (ca_l3_intf_get(0, &intf_entry) != CA_E_OK) {
		ca_free(intf_node);
		return NULL;
	}

	intf_node->port_id = PORT_ID(intf_entry.port_id);
	intf_node->vlan = intf_entry.outer_vid;

	key.mac_sa_cmd = 1;
	key.cos_cmd = 1;
	key.mac_sa_sel = L2_PE_MAC_SA_LAN_IDX;
	if (vlan_is_valid(intf_entry.outer_vid)) {
		key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_SWAP;
		key.vlan = vlan_to_hw_vid(intf_entry.outer_vid);
	} else {
		key.vlan_cmd = CA_L2_VLAN_TAG_ACTION_POP;
		key.vlan = 0;
	}
	if (fvld) {
		key.mac_da_cmd = 1;
		memcpy(key.mac_da, new_mac_da, sizeof(ca_mac_addr_t));
	}

	if (intf_node->port_id != CA_PORT_ID_L3_LAN) {
		key.ldpid = intf_node->port_id;
		intf_node->repl_bit = aal_mc_repl_bit_alloc(&key);
		if (intf_node->repl_bit < 0) {
			ca_free(intf_node);
			return NULL;
		}
	} else if (!igmp_snooping_on) {
		if (CA_E_OK != scfg_read(0, CFG_ID_FLOODING_DOMAIN_1,
			MAX_FLOODING_PORTS * sizeof(ca_uint32_t),
			(ca_uint8_t *) flooding_ports)) {
			MC_ERROR("SCFG read %s failed!!!\n", CFG_ID_FLOODING_DOMAIN_1);
		}

		for (i = 0; i < MAX_FLOODING_PORTS; i++) {
			if (flooding_ports[i] == 0xFF) {
				intf_node->flooding_repl_bit[i] = -1;
				continue;
			}

			key.ldpid = flooding_ports[i];;
			intf_node->flooding_repl_bit[i] = aal_mc_repl_bit_alloc(&key);
			if (intf_node->flooding_repl_bit[i] < 0) {
				for (j = i; j > 0; j--)
					aal_mce_repl_bit_free(intf_node->flooding_repl_bit[j - 1]);
				ca_free(intf_node);
				return NULL;
			}
		}
	}
	aal_array_reverse_copy(my_mac.mac_addr, intf_entry.mac_addr, sizeof(ca_mac_addr_t));
	aal_l2fe_pe_my_mac_set(0, L2_PE_MAC_SA_LAN_IDX, &my_mac);

	return intf_node;
}

static intf_node_t* l3_intf_node_get(l3_node_obj_t *node, ca_intf_id_t intf_id, ca_mac_addr_t new_mac_da)
{
	intf_node_t *intf_node;

	MC_DEBUG("\n");
	if (!node)
		return NULL;

	if (!list_empty(&node->intf_list))
		list_for_each_entry(intf_node, &node->intf_list, list) {
			if (intf_node->intf_id == intf_id &&
				memcmp(intf_node->new_mac_da, new_mac_da, sizeof(ca_mac_addr_t)) == 0) {
				return intf_node;
			}
		}

	return NULL;
}

static ca_status_t l3_intf_node_delete(intf_node_t *intf_node)
{
	ca_uint32_t i;

	MC_DEBUG("\n");

	if (intf_node->port_id != CA_PORT_ID_L3_LAN) {
		aal_mce_repl_bit_free(intf_node->repl_bit);
	} else if (!igmp_snooping_on) {
		for (i = 0; i < MAX_FLOODING_PORTS; i++) {
			aal_mce_repl_bit_free(intf_node->flooding_repl_bit[i]);
		}
	}
	list_del(&intf_node->list);
	ca_free(intf_node);

	return CA_E_OK;
}

static ca_status_t l3_intf_node_delete_all(l3_node_obj_t *node)
{
	intf_node_t *intf_node, *tmp;

	MC_DEBUG("\n");
	if (!list_empty(&node->intf_list))
		list_for_each_entry_safe(intf_node, tmp, &node->intf_list, list) {
			l3_intf_node_delete(intf_node);
		}

	return CA_E_OK;
}

static ca_status_t l3_intf_member_add(ca_l3_mcast_group_members_t *members)
{
	l3_node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	intf_node_t *intf_node;
	ca_uint32_t i;

	MC_DEBUG("\n");
	node = (l3_node_obj_t *) aal_mc_fake_mcgid_ptr_get(members->mcg_id, MC_L3);

	if (node == NULL)
		return CA_E_NOT_FOUND;

	if (node->type == MCG) {
		mcg_node = (l3_mcg_node_t *) node;
		if (!mcg_node->is_api_own)
			return CA_E_NOT_FOUND;
	}

	for (i = 0; i < members->member_count; i++) {
		if (l3_intf_node_get(node, members->member[i].member_intf, members->member[i].new_mac_da))
			continue;

		intf_node = l3_intf_node_alloc(members->member[i].member_intf, members->member[i].new_mac_da, members->member[i].action_mask.mac_da_overwrite);
		if (intf_node == NULL)
			return CA_E_FULL;

		list_add(&intf_node->list, &node->intf_list);
	}

	l3_intfs_expand(node);

	return CA_E_OK;
}

static ca_status_t l3_intf_member_delete(ca_l3_mcast_group_members_t *members)
{
	l3_node_obj_t *node;
	intf_node_t *intf_node;
	ca_uint32_t i;

	MC_DEBUG("\n");
	node = (l3_node_obj_t *) aal_mc_fake_mcgid_ptr_get(members->mcg_id, MC_L3);

	if (node == NULL)
		return CA_E_NOT_FOUND;

	for (i = 0; i < members->member_count; i++) {
		intf_node = l3_intf_node_get(node, members->member[i].member_intf, members->member[i].new_mac_da);
		if (intf_node != NULL)
			l3_intf_node_delete(intf_node);
	}

	l3_intfs_expand(node);

	return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
static l3_mcg_node_t* l3_mcg_node_prealloc(ca_uint16_t vlan, ca_ip_address_t grp_addr)
{
	l3_mcg_node_t *mcg_node;
	ca_int16_t mcgid;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	im3e_mldda_entry_t da_entry;
	ca_uint32_t rsult_idx=CA_UINT32_INVALID;
	ca_uint16_t fake_vid;
	aal_hash_key_t key = {0}, *hash_key = &key;
	aal_hash_action_t action = {0}, *hash_action = &action;
	ca_uint32_t h_idx, *hash_idx = &h_idx, is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe;
	ca_status_t rc = CA_E_OK;

	MC_DEBUG("\n");

	fake_vid = aal_mc_fake_vid_alloc(vlan);
	if (fake_vid == CA_UINT16_INVALID) {
		return NULL;
	}

	mcgid = aal_mc_fake_mcgid_alloc(MC_L3);
	if (mcgid == -1)
		return NULL;

	//aal_entry_add_by_idx(AAL_TABLE_MCE_INDX_TBL, &repl_bits, mcgid);


	mcg_node = ca_zalloc_trace(sizeof(l3_mcg_node_t), GFP_KERNEL, true);
	if (mcg_node == NULL) {
		aal_mc_fake_mcgid_free(mcgid, MC_L3);
		return NULL;
	}
	aal_mc_fake_mcgid_set_callback(mcgid, MC_L3, (ca_int8_t *) mcg_node);

	mcg_node->type = MCG;
	mcg_node->vlan = vlan;
	mcg_node->grp_addr = grp_addr;
	mcg_node->mcgid = mcgid;
	INIT_LIST_HEAD(&mcg_node->intf_list);
	INIT_LIST_HEAD(&mcg_node->l3_src_list);

	list_add(&mcg_node->list, &l3_mcg_list);

	mcg_node->entry_idx = rsult_idx;
#ifndef CONFIG_RTK_DEV_AP
	if (vlan != CA_UINT16_INVALID)
#endif
		hash_key->top_vid = vlan_to_hw_vid(vlan);
	hash_key->ip_vld = 1;
	if (grp_addr.afi == CA_IPV4) {
		hash_key->ip_ver = 0;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv4_addr;
		hash_key->ip_da_1 = 0;
		hash_key->ip_da_2 = 0;
		hash_key->ip_da_3 = 0;
	} else {
		hash_key->ip_ver = 1;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv6_addr[3];
		hash_key->ip_da_1 = grp_addr.ip_addr.ipv6_addr[2];
		hash_key->ip_da_2 = grp_addr.ip_addr.ipv6_addr[1];
		hash_key->ip_da_3 = grp_addr.ip_addr.ipv6_addr[0];
	}

	// Note: KEEP ORIGINAL to CPU !!!
	
#if defined(CONFIG_LUNA_G3_SERIES)	
	hash_action->dpid_vld = 1;
	hash_action->mcgid = 0x10;
	hash_action->dpid_pri = 1;			// overwrite ldpid decision of previous stage
	hash_action->permit = 1;
	hash_action->no_drop = 1;
	hash_action->no_drop_vld = 1;
//	hash_action->stage3_ctrl = 0;		// no impact
//	hash_action->stage3_ctrl_vld = 1;

#endif	

	hash_action->chk_msk_ptr = hash_mask_multicast_idx;

	if (CA_E_OK != (rc = aal_hash_add(0, hash_key, hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, hash_mask_multicast_idx, l3fe_main_hash_profile_mc, hash_idx)))
		goto ERROR;
	

	mcg_node->hash_idx = *hash_idx;

	return mcg_node;

ERROR:
	MC_ERROR("Hash add failed\n");
	return mcg_node;
}

static l3_mcg_node_t* l3_mcg_node_update(ca_uint16_t vlan, ca_ip_address_t grp_addr, l3_mcg_node_t *mcg_node)
{
	ca_int16_t mcgid, mce_indx;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	im3e_mldda_entry_t da_entry;
	ca_uint32_t rsult_idx;
	ca_uint16_t fake_vid;
	aal_hash_key_t key = {0}, *hash_key = &key;
	aal_hash_mask_t mask = {0}, *hash_mask = &mask;
	aal_hash_action_t action = {0}, *hash_action = &action;
	ca_uint32_t h_idx, *hash_idx = &h_idx, is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe;
	ca_status_t rc = CA_E_OK;

	MC_DEBUG("\n");

	fake_vid = aal_mc_fake_vid_alloc(vlan);
	if (fake_vid == CA_UINT16_INVALID) {
		return NULL;
	}

	if(mcg_node == NULL)
		return NULL;

	h_idx = mcg_node->hash_idx;
	mcgid = mcg_node->mcgid;
	if (mcgid == -1)
		return NULL;

	mce_indx = aal_mcgid_alloc(&repl_bits);
	if (mce_indx == -1)
		return NULL;

	memset(&da_entry, 0, sizeof(im3e_mldda_entry_t));
	da_entry.entry_valid = 1;
	da_entry.mc_group_id = mce_indx;
	da_entry.v3_ind = 0;
	da_entry.connection_id = mcgid;
	if (grp_addr.afi == CA_IPV4) {
		da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv4_addr;
		da_entry.ipv6_da_1 = 0;
		da_entry.ipv6_da_2 = 0;
		da_entry.ipv6_da_3 = 0;
		da_entry.ipv6_flg = 0;
	} else {
		da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv6_addr[3];
		da_entry.ipv6_da_1 = grp_addr.ip_addr.ipv6_addr[2];
		da_entry.ipv6_da_2 = grp_addr.ip_addr.ipv6_addr[1];
		da_entry.ipv6_da_3 = grp_addr.ip_addr.ipv6_addr[0];
		da_entry.ipv6_flg = 1;
	}
	da_entry.vid = vlan_to_hw_vid(fake_vid);
	da_entry.vlan_sc_ind = 1;
	da_entry.reserved = 0;
	if ((rc = aal_entry_add(AAL_TABLE_IM3E_MLDDA, &da_entry, &rsult_idx)) != CA_E_OK) {
		if(rc == AAL_E_EXIST){
			// exist
			MC_DEBUG("mce entry exist, mcg_node->entry_idx = %d\n", mcg_node->entry_idx);
			return mcg_node;
		}else{
			aal_mcgid_free(mce_indx);
			aal_mc_fake_mcgid_free(mcgid, MC_L3);
			return NULL;
		}
	}
	mcg_node->mce_indx = mce_indx;
	mcg_node->entry_idx = rsult_idx;

	hash_key->top_vid = vlan_to_hw_vid(vlan);
	hash_key->ip_vld = 1;
	if (grp_addr.afi == CA_IPV4) {
		hash_key->ip_ver = 0;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv4_addr;
		hash_key->ip_da_1 = 0;
		hash_key->ip_da_2 = 0;
		hash_key->ip_da_3 = 0;
	} else {
		hash_key->ip_ver = 1;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv6_addr[3];
		hash_key->ip_da_1 = grp_addr.ip_addr.ipv6_addr[2];
		hash_key->ip_da_2 = grp_addr.ip_addr.ipv6_addr[1];
		hash_key->ip_da_3 = grp_addr.ip_addr.ipv6_addr[0];
	}

	if (CA_E_OK != scfg_read(0, CFG_ID_MC_MAC_UC_CONV_MC,
				sizeof(ca_uint32_t),
				(ca_uint8_t *) &is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe)) {
		MC_ERROR("SCFG read %s failed!!!\n", CFG_ID_MC_MAC_UC_CONV_MC);
	}
	if (is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe) {
		if(l3fe_hash_action_indexed_mode == 0)
		{
			hash_action->mac_da_vld = 1;
			if (grp_addr.afi == CA_IPV4) {
				hash_action->mac_da_5 = 0x01;
				hash_action->mac_da_4 = 0x00;
				hash_action->mac_da_3 = 0x5E;
				hash_action->mac_da_2 = (grp_addr.ip_addr.ipv4_addr & 0x7F0000) >> 16;
				hash_action->mac_da_1 = (grp_addr.ip_addr.ipv4_addr & 0xFF00) >> 8;
				hash_action->mac_da_0 = grp_addr.ip_addr.ipv4_addr & 0xFF;
			} else {
				hash_action->mac_da_5 = 0x33;
				hash_action->mac_da_4 = 0x33;
				hash_action->mac_da_3 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF000000) >> 24;
				hash_action->mac_da_2 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF0000) >> 16;
				hash_action->mac_da_1 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF00) >> 8;
				hash_action->mac_da_0 = grp_addr.ip_addr.ipv6_addr[3] & 0xFF;
			}
		}
		else
		{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			//set mac index table at first
			idx_intf_mac_entry_t mac_entry;
			ca_uint8_t mac_da[6];
			int da_rslt_idx = 0;
			int ret;
			memset(&mac_entry, 0, sizeof(mac_entry));
			if (grp_addr.afi == CA_IPV4) {
				mac_da[0] = 0x01;
				mac_da[1] = 0x00;
				mac_da[2] = 0x5E;
				mac_da[3] = (grp_addr.ip_addr.ipv4_addr & 0x7F0000) >> 16;
				mac_da[4] = (grp_addr.ip_addr.ipv4_addr & 0xFF00) >> 8;
				mac_da[5] = grp_addr.ip_addr.ipv4_addr & 0xFF;
			} else {
				mac_da[0] = 0x33;
				mac_da[1] = 0x33;
				mac_da[2] = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF000000) >> 24;
				mac_da[3] = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF0000) >> 16;
				mac_da[4] = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF00) >> 8;
				mac_da[5] = grp_addr.ip_addr.ipv6_addr[3] & 0xFF;
			}
			aal_array_reverse_copy(mac_entry.mac_da, mac_da, 6);
			ret = aal_entry_add(AAL_TABLE_IDX_INTF_MAC, &mac_entry, &da_rslt_idx);
			if(ret != CA_E_OK) {
				goto ERROR;
			}
			hash_action->mac_da_idx = da_rslt_idx;
			hash_action->mac_da_idx_vld = 1;
#endif
		}
	}
	hash_action->vlan_cnt = 1;
	hash_action->top_tpid_enc = 1;
	hash_action->top_dei_sel = 1;
	hash_action->top_vid = vlan_to_hw_vid(fake_vid);
	hash_action->vlan_vld = 1;
#if 0 // consider bridge and routing at the same flow, we disable TTL-1 action.
	hash_action->ip_ttl_cmd = 1;
	hash_action->ip_ttl_vld = 1;
#endif	
	hash_action->pppoe_set = 1;
	hash_action->pppoe_vld = 0;
	hash_action->chk_msk_ptr = hash_mask_multicast_idx;

	MC_DEBUG("update hash action to allow packet redirect to L2FE, hash_idx = %d\n", *hash_idx);
	
	if (CA_E_OK != (rc = aal_hash_action_update(0, hash_key, hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, hash_mask_multicast_idx, l3fe_main_hash_profile_mc, *hash_idx)))
		goto ERROR;
	
	return mcg_node;

ERROR:
	MC_ERROR("Hash add failed\n");
	return NULL;
}
#endif /*CONFIG_LUNA_G3_SERIES*/

static l3_mcg_node_t* l3_mcg_node_get(ca_uint16_t WAN_vlan, ca_ip_address_t grp_addr)
{
	l3_mcg_node_t *cur_node;
	struct list_head *tmp;

	MC_DEBUG("\n");

	list_for_each(tmp, &l3_mcg_list) {
		cur_node = list_entry(tmp, l3_mcg_node_t, list);

		if (cur_node->vlan == WAN_vlan && ip_addr_compare(&cur_node->grp_addr, &grp_addr) == 0) {
			return (l3_mcg_node_t *)tmp;
		}
	}

	return NULL;
}

static l3_mcg_node_t* l3_mcg_node_alloc(ca_uint16_t vlan, ca_ip_address_t grp_addr)
{
	l3_mcg_node_t *mcg_node;
	ca_int16_t mcgid, mce_indx;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	im3e_mldda_entry_t da_entry;
	ca_uint32_t rsult_idx;
	ca_uint16_t fake_vid;
	aal_hash_key_t key = {0}, *hash_key = &key;
	aal_hash_action_t action = {0}, *hash_action = &action;
	ca_uint32_t h_idx, *hash_idx = &h_idx, is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe;
	ca_status_t rc = CA_E_OK;

	MC_DEBUG("\n");

	fake_vid = aal_mc_fake_vid_alloc(vlan);
	if (fake_vid == CA_UINT16_INVALID) {
		return NULL;
	}

	mcgid = aal_mc_fake_mcgid_alloc(MC_L3);
	if (mcgid == -1)
		return NULL;

	mce_indx = aal_mcgid_alloc(&repl_bits);
	if (mce_indx == -1)
		return NULL;

	memset(&da_entry, 0, sizeof(im3e_mldda_entry_t));
	da_entry.entry_valid = 1;
	da_entry.mc_group_id = mce_indx;
	da_entry.v3_ind = 0;
	da_entry.connection_id = mcgid;
	if (grp_addr.afi == CA_IPV4) {
		da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv4_addr;
		da_entry.ipv6_da_1 = 0;
		da_entry.ipv6_da_2 = 0;
		da_entry.ipv6_da_3 = 0;
		da_entry.ipv6_flg = 0;
	} else {
		da_entry.ipv6_da_0 = grp_addr.ip_addr.ipv6_addr[3];
		da_entry.ipv6_da_1 = grp_addr.ip_addr.ipv6_addr[2];
		da_entry.ipv6_da_2 = grp_addr.ip_addr.ipv6_addr[1];
		da_entry.ipv6_da_3 = grp_addr.ip_addr.ipv6_addr[0];
		da_entry.ipv6_flg = 1;
	}
	da_entry.vid = vlan_to_hw_vid(fake_vid);
	da_entry.vlan_sc_ind = da_entry.vid > 0 ? SVLAN : CVLAN;
	da_entry.reserved = 0;
	if (aal_entry_add(AAL_TABLE_IM3E_MLDDA, &da_entry, &rsult_idx)) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L3);
		return NULL;
	}

	mcg_node = ca_zalloc_trace(sizeof(l3_mcg_node_t), GFP_KERNEL, true);
	if (mcg_node == NULL) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L3);
		return NULL;
	}
	aal_mc_fake_mcgid_set_callback(mcgid, MC_L3, (ca_int8_t *) mcg_node);

	mcg_node->type = MCG;
	mcg_node->vlan = vlan;
	mcg_node->grp_addr = grp_addr;
	mcg_node->mcgid = mcgid;
	mcg_node->mce_indx = mce_indx;
	INIT_LIST_HEAD(&mcg_node->intf_list);
	INIT_LIST_HEAD(&mcg_node->l3_src_list);

	list_add(&mcg_node->list, &l3_mcg_list);

	mcg_node->entry_idx = rsult_idx;

#ifndef CONFIG_RTK_DEV_AP
	if (vlan != CA_UINT16_INVALID)
#endif
	hash_key->top_vid = vlan_to_hw_vid(vlan);
	hash_key->ip_vld = 1;
	if (grp_addr.afi == CA_IPV4) {
		hash_key->ip_ver = 0;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv4_addr;
		hash_key->ip_da_1 = 0;
		hash_key->ip_da_2 = 0;
		hash_key->ip_da_3 = 0;
	} else {
		hash_key->ip_ver = 1;
		hash_key->ip_da_0 = grp_addr.ip_addr.ipv6_addr[3];
		hash_key->ip_da_1 = grp_addr.ip_addr.ipv6_addr[2];
		hash_key->ip_da_2 = grp_addr.ip_addr.ipv6_addr[1];
		hash_key->ip_da_3 = grp_addr.ip_addr.ipv6_addr[0];
	}

	if (CA_E_OK != scfg_read(0, CFG_ID_MC_MAC_UC_CONV_MC,
				sizeof(ca_uint32_t),
				(ca_uint8_t *) &is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe)) {
		MC_ERROR("SCFG read %s failed!!!\n", CFG_ID_MC_MAC_UC_CONV_MC);
	}
	if (is_converting_incoming_wan_ucmac_to_mcmac_for_pppoe) {
		hash_action->mac_da_vld = 1;
		if (grp_addr.afi == CA_IPV4) {
			hash_action->mac_da_5 = 0x01;
			hash_action->mac_da_4 = 0x00;
			hash_action->mac_da_3 = 0x5E;
			hash_action->mac_da_2 = (grp_addr.ip_addr.ipv4_addr & 0x7F0000) >> 16;
			hash_action->mac_da_1 = (grp_addr.ip_addr.ipv4_addr & 0xFF00) >> 8;
			hash_action->mac_da_0 = grp_addr.ip_addr.ipv4_addr & 0xFF;
		} else {
			hash_action->mac_da_5 = 0x33;
			hash_action->mac_da_4 = 0x33;
			hash_action->mac_da_3 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF000000) >> 24;
			hash_action->mac_da_2 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF0000) >> 16;
			hash_action->mac_da_1 = (grp_addr.ip_addr.ipv6_addr[3] & 0xFF00) >> 8;
			hash_action->mac_da_0 = grp_addr.ip_addr.ipv6_addr[3] & 0xFF;
		}
	}
	hash_action->vlan_cnt = 1;
	hash_action->top_tpid_enc = 1;
	hash_action->top_dei_sel = 1;
	hash_action->top_vid = vlan_to_hw_vid(fake_vid);
	hash_action->vlan_vld = 1;
#if !defined(CONFIG_RTK_FC_IGMPV3_SUPPORT)
	// consider bridge and routing at the same flow, we disable TTL-1 action.
	hash_action->ip_ttl_cmd = 1;
	hash_action->ip_ttl_vld = 1;
#endif
	hash_action->pppoe_set = 1;
	hash_action->pppoe_vld = 0;
	hash_action->chk_msk_ptr = hash_mask_multicast_idx;

	/* Force to use LAN MAC as MAC_SA for saving L2FDB entry; otherwise, all MAC of Multicast sources are recorded into L2FDB. */
	hash_action->mac_sa_vld = 1;
	hash_action->mac_sa_5 = ca_ne_mac_addr1[0];
	hash_action->mac_sa_4 = ca_ne_mac_addr1[1];
	hash_action->mac_sa_3 = ca_ne_mac_addr1[2];
	hash_action->mac_sa_2 = ca_ne_mac_addr1[3];
	hash_action->mac_sa_1 = ca_ne_mac_addr1[4];
	hash_action->mac_sa_0 = ca_ne_mac_addr1[5];

	if (CA_E_OK != (rc = aal_hash_add(0, hash_key, hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, hash_mask_multicast_idx, l3fe_main_hash_profile_mc, hash_idx)))
		goto ERROR;
	mcg_node->hash_idx = *hash_idx;

	return mcg_node;

ERROR:
	MC_ERROR("Hash add failed\n");
	return mcg_node;
}

static ca_status_t l3_mcg_node_delete(l3_mcg_node_t *mcg_node)
{
	MC_DEBUG("\n");
	if (mcg_node->is_api_own) {
		mcg_node->is_api_own = FALSE;
		l3_intf_node_delete_all((l3_node_obj_t *) mcg_node);

		l3_intfs_expand((l3_node_obj_t *) mcg_node);
	}
	if (list_empty(&mcg_node->l3_src_list)) {
		aal_hash_delete(0, mcg_node->hash_idx);
		list_del(&mcg_node->list);
		aal_mc_fake_vid_free(mcg_node->vlan);
		aal_mcgid_free(mcg_node->mce_indx);
		aal_mc_fake_mcgid_free(mcg_node->mcgid, MC_L3);
		aal_entry_del(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx);
		ca_free(mcg_node);
	}

	return CA_E_OK;
}

static l3_src_node_t* l3_src_node_get(l3_mcg_node_t *mcg_node, ca_ip_address_t src_addr)
{
	l3_src_node_t *src_node;

	MC_DEBUG("\n");
	if (!mcg_node)
		return NULL;

	if (!list_empty(&mcg_node->l3_src_list))
		list_for_each_entry(src_node, &mcg_node->l3_src_list, list) {
			if (ip_addr_compare(&src_node->src_addr, &src_addr) == 0) {
				return src_node;
			}
		}

	return NULL;
}

static l3_src_node_t* l3_src_node_add(l3_mcg_node_t *mcg_node, ca_ip_address_t src_addr)
{
	l3_src_node_t *src_node;
	ca_int16_t mcgid, mce_indx, need_toggle = 0;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	im3e_mldda_entry_t da_entry;
	im3e_mldsa_entry_t sa_entry;
	ca_uint32_t rsult_idx;

	MC_DEBUG("\n");
	if (!mcg_node)
		return NULL;

	mcgid = aal_mc_fake_mcgid_alloc(MC_L3);
	if (mcgid == -1)
		return NULL;

	mce_indx = aal_mcgid_alloc(&repl_bits);
	if (mce_indx == -1)
		return NULL;

	sa_entry.entry_valid = 1;
	sa_entry.mc_group_id = mce_indx;
	sa_entry.connection_id = mcg_node->mcgid;
	if (src_addr.afi == CA_IPV4) {
		sa_entry.ipv6_sa_0 = src_addr.ip_addr.ipv4_addr;
		sa_entry.ipv6_sa_1 = 0;
		sa_entry.ipv6_sa_2 = 0;
		sa_entry.ipv6_sa_3 = 0;
	} else {
		sa_entry.ipv6_sa_0 = src_addr.ip_addr.ipv6_addr[3];
		sa_entry.ipv6_sa_1 = src_addr.ip_addr.ipv6_addr[2];
		sa_entry.ipv6_sa_2 = src_addr.ip_addr.ipv6_addr[1];
		sa_entry.ipv6_sa_3 = src_addr.ip_addr.ipv6_addr[0];
	}
	sa_entry.reserved = 0;
	if (aal_entry_add(AAL_TABLE_IM3E_MLDSA, &sa_entry, &rsult_idx)) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L3);
		return NULL;
	}

	src_node = ca_zalloc_trace(sizeof(l3_src_node_t), GFP_KERNEL, true);
	if (src_node == NULL) {
		aal_mcgid_free(mce_indx);
		aal_mc_fake_mcgid_free(mcgid, MC_L3);
		return NULL;
	}
	aal_mc_fake_mcgid_set_callback(mcgid, MC_L3, (ca_int8_t *) src_node);

	src_node->type = SRC;
	src_node->parent = (ca_uint32_t *) mcg_node;
	src_node->src_addr = src_addr;
	src_node->mcgid = mcgid;
	src_node->mce_indx = mce_indx;
	INIT_LIST_HEAD(&src_node->intf_list);

	if (list_empty(&mcg_node->l3_src_list))
		need_toggle = 1;
	list_add(&src_node->list, &mcg_node->l3_src_list);

	src_node->entry_idx = rsult_idx;

	if (need_toggle) {
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
		da_entry.v3_ind = 1;
		aal_entry_set(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
	}

	return src_node;
}

static ca_status_t l3_src_node_delete(l3_src_node_t *src_node)
{
	l3_mcg_node_t *mcg_node;

	MC_DEBUG("\n");
	mcg_node = (l3_mcg_node_t *) src_node->parent;

	l3_intf_node_delete_all((l3_node_obj_t *) src_node);
	list_del(&src_node->list);
	aal_entry_del(AAL_TABLE_IM3E_MLDSA, src_node->entry_idx);
	aal_mcgid_free(src_node->mce_indx);
	aal_mc_fake_mcgid_free(src_node->mcgid, MC_L3);
	ca_free(src_node);

	if (list_empty(&mcg_node->l3_src_list) && !mcg_node->is_api_own) {
		l3_mcg_node_delete(mcg_node);
	}

	return CA_E_OK;
}

static ca_status_t l3_intfs_expand(l3_node_obj_t *node)
{
	l3_mcg_node_t *mcg_node;
	l3_src_node_t *src_node;
	intf_node_t *intf_node;
	port_node_t *port_node;
	struct list_head *port_list;
	ni_mce_indx_tbl_entry_t repl_bits = {0};
	ca_ip_address_t src_addr = {0};
	ca_uint32_t i;
	ca_int16_t new_mce_indx;
	im3e_mldda_entry_t da_entry;
	im3e_mldsa_entry_t sa_entry;

	if (node == NULL)
		return CA_E_OK;

	MC_DEBUG("\n");

	if (node->type == MCG) {
		mcg_node = (l3_mcg_node_t *) node;
	} else {
		src_node = (l3_src_node_t *) node;
		mcg_node = (l3_mcg_node_t *) src_node->parent;
		src_addr = src_node->src_addr;
	}
	if (!list_empty(&node->intf_list))
	list_for_each_entry(intf_node, &node->intf_list, list) {
		if (intf_node->port_id == CA_PORT_ID_L3_LAN) {
			if (!igmp_snooping_on) {
				for (i = 0; i < MAX_FLOODING_PORTS; i++) {
					if (intf_node->flooding_repl_bit[i] > -1)
						repl_bits.mc_vec |= (ca_uint64_t)1 << intf_node->flooding_repl_bit[i];
				}
			} else {
				port_list = l2_port_list_search(intf_node->vlan, mcg_node->grp_addr, node->type, src_addr);
				if (port_list && !list_empty(port_list))
				list_for_each_entry(port_node, port_list, list) {
					repl_bits.mc_vec |= (ca_uint64_t)1 << port_node->repl_bit;
				}
			}
		} else if (intf_node->repl_bit > -1) {
			repl_bits.mc_vec |= (ca_uint64_t)1 << intf_node->repl_bit;
		}
	}

	new_mce_indx = aal_mcgid_alloc(&repl_bits);
	if (new_mce_indx < 0)
		return CA_E_FULL;

	/* update IM3E MLDDA/MLDSA table */
	if (node->type == MCG) {
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
		da_entry.mc_group_id = new_mce_indx;
		aal_entry_set(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
	} else {
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
		sa_entry.mc_group_id = new_mce_indx;
		aal_entry_set(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
	}

	aal_mcgid_free(node->mce_indx);
	node->mce_indx = new_mce_indx;

	return CA_E_OK;
}

static ca_status_t l3_intfs_expand_all_by_rule(ca_ip_address_t grp_addr, ca_ip_address_t src_addr)
{
	l3_node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	l3_src_node_t *src_node;
	intf_node_t *intf_node;
	port_node_t *port_node;
	struct list_head *port_list;
	ca_ip_address_t src0 = {0};
	ni_mce_indx_tbl_entry_t repl_bits;
	ca_int16_t new_mce_indx;
	im3e_mldda_entry_t da_entry;
	im3e_mldsa_entry_t sa_entry;

	MC_DEBUG("\n");
	if (mcast_mode == CA_MC_MODE_MAC)
		return CA_E_OK;

	if (l3_mcg_list.next == NULL)
		return CA_E_OK;

	if (!list_empty(&l3_mcg_list))
	list_for_each_entry(mcg_node, &l3_mcg_list, list) {
		if (ip_addr_compare(&mcg_node->grp_addr, &grp_addr) == 0) {
			node = NULL;
			if (ip_addr_compare(&src_addr, &src0) == 0) {
				node = (l3_node_obj_t *) mcg_node;
			} else {
				if (!list_empty(&mcg_node->l3_src_list))
				list_for_each_entry(src_node, &mcg_node->l3_src_list, list) {
					if (ip_addr_compare(&src_node->src_addr, &src_addr) == 0) {
						node = (l3_node_obj_t *) src_node;
						break;
					}
				}
			}

			if (node) {
				repl_bits.mc_vec = 0;
				if (!list_empty(&node->intf_list))
				list_for_each_entry(intf_node, &node->intf_list, list) {
#ifdef CONFIG_RTK_DEV_AP
					port_list = l2_port_list_search(mcg_node->vlan, grp_addr, node->type, src_addr);
#else
					port_list = l2_port_list_search(intf_node->vlan, grp_addr, node->type, src_addr);
#endif
					if (port_list && !list_empty(port_list))
					list_for_each_entry(port_node, port_list, list) {
						repl_bits.mc_vec |= (ca_uint64_t)1 << port_node->repl_bit;
					}
				}

				new_mce_indx = aal_mcgid_alloc(&repl_bits);
				if (new_mce_indx < 0)
				return CA_E_FULL;

				/* update IM3E MLDDA/MLDSA table */
				if (node->type == MCG) {
					aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
					da_entry.mc_group_id = new_mce_indx;
					aal_entry_set(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
				} else {
					aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
					sa_entry.mc_group_id = new_mce_indx;
					aal_entry_set(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
				}

				aal_mcgid_free(node->mce_indx);
				node->mce_indx = new_mce_indx;
			}
		}
	}

	return CA_E_OK;
}
#endif

static ca_status_t l2_ports_expand(l2_node_obj_t *node)
{
	ni_mce_indx_tbl_entry_t repl_bits;
	ca_int16_t new_mce_indx;
	port_node_t *port_node;
	l2_mcg_node_t *mcg_node;
	ca_l2_addr_entry_t fdb_entry;
	im3e_mldda_entry_t da_entry;
	im3e_mldsa_entry_t sa_entry;
	ca_status_t ret;

	MC_DEBUG("\n");
	if (node == NULL)
		return CA_E_OK;

	repl_bits.mc_vec = 0;

	if (!list_empty(&node->port_list))
		list_for_each_entry(port_node, &node->port_list, list) {
			repl_bits.mc_vec |= (ca_uint64_t)1 << port_node->repl_bit;
		}

	if (mcast_mode == CA_MC_MODE_LLID) {
		aal_entry_set(AAL_TABLE_MCE_INDX_TBL, node->mce_indx, &repl_bits);
	} else {
	new_mce_indx = aal_mcgid_alloc(&repl_bits);
	if (new_mce_indx < 0)
		return CA_E_FULL;

	/* update L2 fdb, IM3E MLDDA/MLDSA table */
	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
		if (mcast_mode == CA_MC_MODE_MAC) {
			ca_l2_addr_delete_by_mac(0, mcg_node->mac_addr, CA_L2_ADDR_OPERATION_STATIC);
			memset(&fdb_entry, 0, sizeof(ca_l2_addr_entry_t));
			memcpy(fdb_entry.mac_addr, mcg_node->mac_addr, sizeof(ca_mac_addr_t));
			fdb_entry.vid = vlan_to_hw_vid(mcg_node->vlan);
			fdb_entry.static_flag = 1;
			fdb_entry.da_permit = 1;
			fdb_entry.mc_group_id = new_mce_indx;
			fdb_entry.port_id = CA_UINT32_INVALID;
			if (CA_E_OK != (ret = ca_l2_addr_add(0, &fdb_entry))) {
				aal_mcgid_free(new_mce_indx);
				return ret;
			}
		} else {
			aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
			da_entry.mc_group_id = new_mce_indx;
			aal_entry_set(AAL_TABLE_IM3E_MLDDA, node->entry_idx, &da_entry);
		}
	} else {
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
		sa_entry.mc_group_id = new_mce_indx;
		aal_entry_set(AAL_TABLE_IM3E_MLDSA, node->entry_idx, &sa_entry);
	}

	aal_mcgid_free(node->mce_indx);
	node->mce_indx = new_mce_indx;
	}

	return CA_E_OK;
}

ca_status_t ca_l2_mcast_config_set (
	CA_IN ca_device_id_t          device_id,
	CA_IN ca_l2_mcast_config_t *l2_mcast_config)
{
#if defined(CONFIG_ARCH_REALTEK_TAURUS)
#else
	L2FE_IM3E_CTRL_t ctrl;
#endif
	if (l2_mcast_config == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&mcast_lock);

	if (igmp_snooping_on != !l2_mcast_config->unknown_multicast_flooding_enable ||
		mcast_mode != l2_mcast_config->mode ||
		igmp_use_mc_vlan != l2_mcast_config->igmp_use_mc_vlan ||
		mld_use_mc_vlan != l2_mcast_config->mld_use_mc_vlan) {
		if ((l3_mcg_list.next != NULL && !list_empty(&l3_mcg_list)) ||
			(l2_mcg_list.next != NULL && !list_empty(&l2_mcg_list))) {
			MC_DEBUG("MCG not empty!! You can't do that!\n");
			spin_unlock_bh(&mcast_lock);
			return CA_E_NOT_SUPPORT;
		}
	}

	if (igmp_snooping_on != !l2_mcast_config->unknown_multicast_flooding_enable) {
		if (l2_mcast_config->unknown_multicast_flooding_enable)
			aal_umc_all_ports_drop(FALSE);
		else
			aal_umc_all_ports_drop(TRUE);
	}

	igmp_snooping_on = !l2_mcast_config->unknown_multicast_flooding_enable;
	mcast_mode = l2_mcast_config->mode;
	if (igmp_use_mc_vlan != l2_mcast_config->igmp_use_mc_vlan ||
		mld_use_mc_vlan != l2_mcast_config->mld_use_mc_vlan) {
		mld_use_mc_vlan = l2_mcast_config->mld_use_mc_vlan;
		igmp_use_mc_vlan = l2_mcast_config->igmp_use_mc_vlan;

#if defined(CONFIG_ARCH_REALTEK_TAURUS)
#else
		aal_im3e_control_get(&ctrl);
		ctrl.bf.mld_no_vlan_srch = !mld_use_mc_vlan;
		ctrl.bf.igmp_no_vlan_srch = !igmp_use_mc_vlan;
		aal_im3e_control_set(&ctrl);
#endif		
	}

	spin_unlock_bh(&mcast_lock);

	return CA_E_OK;
}

ca_status_t ca_l2_mcast_config_get (
	CA_IN  ca_device_id_t          device_id,
	CA_OUT ca_l2_mcast_config_t *l2_mcast_config)
{
	if (l2_mcast_config == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&mcast_lock);

	l2_mcast_config->unknown_multicast_flooding_enable = !igmp_snooping_on;
	l2_mcast_config->mode = mcast_mode;
	l2_mcast_config->mld_use_mc_vlan = mld_use_mc_vlan;
	l2_mcast_config->igmp_use_mc_vlan = igmp_use_mc_vlan;

	spin_unlock_bh(&mcast_lock);

	return CA_E_OK;
}


ca_status_t ca_l2_mcast_group_add (
	CA_IN ca_device_id_t         device_id,
	CA_OUT ca_l2_mcast_entry_t *entry)
{
	l2_mcg_node_t *mcg_node;
	l2_src_node_t *src_node;

	MC_DEBUG("\n");

	spin_lock_bh(&mcast_lock);

	if (!igmp_snooping_on) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (!entry) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

	if (!api_level_vlan_range_valid(entry->mcast_vlan)) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

	if (mcast_mode == CA_MC_MODE_IP) {
		if (entry->group_ip_addr.afi == CA_IPV4) {
			if (entry->group_ip_addr.addr_len != 32 ||
				!ipv4_is_mcast(entry->group_ip_addr.ip_addr)) {
				spin_unlock_bh(&mcast_lock);
				return CA_E_PARAM;
			}
			if (entry->src_ip_address.addr_len > 0) {
				if (entry->src_ip_address.afi != CA_IPV4 ||
					entry->src_ip_address.addr_len != 32) {
					spin_unlock_bh(&mcast_lock);
					return CA_E_PARAM;
				}
			}
			if (!igmp_use_mc_vlan)
				entry->mcast_vlan = CA_UINT16_INVALID;
		} else {
			if (entry->group_ip_addr.addr_len != 128 ||
				!ipv6_is_mcast(entry->group_ip_addr.ip_addr)) {
				spin_unlock_bh(&mcast_lock);
				return CA_E_PARAM;
			}
			if (entry->src_ip_address.addr_len > 0) {
				if (entry->src_ip_address.afi != CA_IPV6 ||
					entry->src_ip_address.addr_len != 128) {
					spin_unlock_bh(&mcast_lock);
					return CA_E_PARAM;
				}
			}
			if (!mld_use_mc_vlan)
				entry->mcast_vlan = CA_UINT16_INVALID;
		}
	}

	mcg_node = l2_mcg_node_get(entry->mcast_vlan, entry->group_ip_addr, entry->group_mac_addr, entry->replication_only);
	if (mcg_node != NULL) {
		if (entry->src_ip_address.addr_len > 0) {
			src_node = l2_src_node_get(mcg_node, entry->src_ip_address);
			if (src_node != NULL) {
				entry->mcg_id = src_node->mcgid;
				spin_unlock_bh(&mcast_lock);
				return CA_E_EXISTS;
			}
		} else if (mcg_node->is_api_own) {
			entry->mcg_id = mcg_node->mcgid;
			spin_unlock_bh(&mcast_lock);
			return CA_E_EXISTS;
		} else {
			mcg_node->is_api_own = TRUE;
		}
	} else {
		mcg_node = l2_mcg_node_alloc(entry->mcast_vlan, entry->group_ip_addr, entry->group_mac_addr, entry->replication_only);

		if (mcg_node == NULL) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_FULL;
		}

		if (entry->src_ip_address.addr_len == 0)
			mcg_node->is_api_own = TRUE;
	}

	if ((entry->src_ip_address.addr_len > 0) && (mcast_mode == CA_MC_MODE_IP)) {
		src_node = l2_src_node_get(mcg_node, entry->src_ip_address);
		if (src_node == NULL)
			src_node = l2_src_node_add(mcg_node, entry->src_ip_address);

		if (src_node == NULL) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_FULL;
		}
		entry->mcg_id = src_node->mcgid;
	} else {
		entry->mcg_id = mcg_node->mcgid;
	}

	spin_unlock_bh(&mcast_lock);

	return CA_E_OK;
}

ca_status_t ca_l2_mcast_group_get (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32_t         mcg_id,
	CA_OUT ca_l2_mcast_iterator_entry_t *mcast_group)
{
	node_obj_t *node;
	l2_mcg_node_t *mcg_node;
	l2_src_node_t *src_node;
	port_node_t *port_node;
	ca_uint32_t i;

	if (mcast_group == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&mcast_lock);

		node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcg_id, MC_L2);
	if (node == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_FOUND;
	}

	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
		if (!mcg_node->is_api_own) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_NOT_FOUND;
		}

		mcast_group->mc_entry.replication_only = mcg_node->llid;
		mcast_group->mc_entry.mcg_id = mcg_node->mcgid;
		mcast_group->mc_entry.mcast_vlan = mcg_node->vlan;
		if (mcast_mode == CA_MC_MODE_IP)
			mcast_group->mc_entry.group_ip_addr = mcg_node->grp_addr;
		else
			memcpy(mcast_group->mc_entry.group_mac_addr, mcg_node->mac_addr, sizeof(ca_mac_addr_t));
		memset(&mcast_group->mc_entry.src_ip_address, 0, sizeof(ca_ip_address_t));
		mcast_group->members.mcg_id = mcg_node->mcgid;
		i = 0;
		if (!list_empty(&mcg_node->port_list)) {
			list_for_each_entry(port_node, &mcg_node->port_list, list) {
				mcast_group->members.member[i].action_mask.mac_da_overwrite = port_node->key.mac_da_cmd;
				memcpy(mcast_group->members.member[i].new_mac_da, port_node->key.mac_da, sizeof(ca_mac_addr_t));
				mcast_group->members.member[i].action_mask.vlan_action = port_node->key.vlan_cmd > 0;
				if (wan2lan_only_igmp) {
				mcast_group->members.member[i].vlan_action = port_node->api_vlan_cmd;
				mcast_group->members.member[i].vid = port_node->api_vlan;
				} else {
				mcast_group->members.member[i].vlan_action = port_node->key.vlan_cmd;
				mcast_group->members.member[i].vid = port_node->key.vlan;
				}
				mcast_group->members.member[i].member_port = CA_PORT_ID(port_node->api_port_type, port_node->key.ldpid);
				i++;
			}
		}

		mcast_group->members.member_count = i;
	} else {
		src_node = (l2_src_node_t *) node;
		mcg_node = (l2_mcg_node_t *) src_node->parent;
		mcast_group->mc_entry.mcg_id = src_node->mcgid;
		mcast_group->mc_entry.mcast_vlan = mcg_node->vlan;
		if (mcast_mode == CA_MC_MODE_IP)
			mcast_group->mc_entry.group_ip_addr = mcg_node->grp_addr;
		else
			memcpy(mcast_group->mc_entry.group_mac_addr, mcg_node->mac_addr, sizeof(ca_mac_addr_t));
		mcast_group->mc_entry.src_ip_address = src_node->src_addr;
		mcast_group->members.mcg_id = src_node->mcgid;
		i = 0;
		if (!list_empty(&src_node->port_list)) {
			list_for_each_entry(port_node, &src_node->port_list, list) {
				mcast_group->members.member[i].action_mask.mac_da_overwrite = port_node->key.mac_da_cmd;
				memcpy(mcast_group->members.member[i].new_mac_da, port_node->key.mac_da, sizeof(ca_mac_addr_t));
				mcast_group->members.member[i].action_mask.vlan_action = port_node->key.vlan_cmd > 0;
				if (wan2lan_only_igmp) {
				mcast_group->members.member[i].vlan_action = port_node->api_vlan_cmd;
				mcast_group->members.member[i].vid = port_node->api_vlan;
				} else {
				mcast_group->members.member[i].vlan_action = port_node->key.vlan_cmd;
				mcast_group->members.member[i].vid = port_node->key.vlan;
				}
				mcast_group->members.member[i].member_port = CA_PORT_ID(port_node->api_port_type, port_node->key.ldpid);
				i++;
			}
		}

		mcast_group->members.member_count = i;
	}

	spin_unlock_bh(&mcast_lock);

	return CA_E_OK;
}

ca_status_t ca_l2_mcast_group_delete (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32_t         mcg_id)
{
	node_obj_t *node;
	l2_mcg_node_t *mcg_node;
	ca_status_t ret;

	spin_lock_bh(&mcast_lock);

		node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcg_id, MC_L2);
	if (node == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_FOUND;
	}

	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
		if (mcg_node->is_api_own)
			ret = l2_mcg_node_delete((l2_mcg_node_t *) node);
		else
			ret = CA_E_NOT_FOUND;
	} else {
		ret = l2_src_node_delete((l2_src_node_t *) node);
	}

	spin_unlock_bh(&mcast_lock);

	return ret;
}

ca_status_t ca_l2_mcast_group_delete_all (
	CA_IN ca_device_id_t         device_id)
{
	ca_uint16_t i;

	for (i = 0; i < MCE_INDX_TBL_ENTRY_MAX; i++) {
		ca_l2_mcast_group_delete(device_id, i);
	}

	return CA_E_OK;
}

ca_status_t ca_l2_mcast_member_add (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_l2_mcast_group_members_t *member)
{
	ca_uint32_t i;
	ca_status_t ret;

	if (member == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&mcast_lock);

	for (i = 0; i < member->member_count; i++) {
		if (!member->member[i].action_mask.mac_da_overwrite)
			memset(member->member[i].new_mac_da, 0, sizeof(ca_mac_addr_t));
		if (!member->member[i].action_mask.vlan_action) {
			member->member[i].vlan_action = 0;
			member->member[i].vid = 0;
		} else if (!api_level_vlan_range_valid(member->member[i].vid)){
			spin_unlock_bh(&mcast_lock);
			return CA_E_PARAM;
		}
	}

	ret = l2_port_member_add(member);

	spin_unlock_bh(&mcast_lock);

	return ret;
}

ca_status_t ca_l2_mcast_member_delete (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_l2_mcast_group_members_t *member)
{
	ca_uint32_t i;
	ca_status_t ret;

	if (member == NULL)
		return CA_E_PARAM;

	spin_lock_bh(&mcast_lock);

	for (i = 0; i < member->member_count; i++) {
		if (!member->member[i].action_mask.mac_da_overwrite)
			memset(member->member[i].new_mac_da, 0, sizeof(ca_mac_addr_t));
		if (!member->member[i].action_mask.vlan_action) {
			member->member[i].vlan_action = 0;
			member->member[i].vid = 0;
		} else if (!api_level_vlan_range_valid(member->member[i].vid)){
			spin_unlock_bh(&mcast_lock);
			return CA_E_PARAM;
		}
	}

	ret = l2_port_member_delete(member);

	spin_unlock_bh(&mcast_lock);

	return ret;
}

ca_status_t ca_l2_mcast_group_iterate (
	CA_IN ca_device_id_t                    device_id,
	CA_OUT ca_iterator_t              *return_entry)
{
	l2_node_obj_t *node;
	ca_uint16_t mcgid, query_count, cnt, i;
	ca_l2_mcast_iterator_entry_t *entry_ptr;
	ca_status_t ret;

	if (return_entry == NULL ||
		return_entry->entry_count == 0 ||
		return_entry->entry_data == NULL) {
		return CA_E_PARAM;
	}

	query_count = return_entry->user_buffer_size / sizeof(ca_l2_mcast_iterator_entry_t);
	if (query_count > return_entry->entry_count) {
		query_count = return_entry->entry_count;
	}

	node = return_entry->prev_handle;
	if (node == NULL)
		mcgid = 0;
	else
		mcgid = node->mcgid + 1;

	entry_ptr = return_entry->entry_data;
	cnt = 0;
	for (i = mcgid; i < MCE_INDX_TBL_ENTRY_MAX; i++) {
		if (cnt == query_count)
			break;
		ret = ca_l2_mcast_group_get(0, i, entry_ptr);
		if (ret == CA_E_OK) {
			return_entry->entry_index[cnt] = entry_ptr->mc_entry.mcg_id;
				node = (l2_node_obj_t *) aal_mc_fake_mcgid_ptr_get(i, MC_L2);
			entry_ptr++;
			cnt++;
		}
	}

	return_entry->entry_count = cnt;
	return_entry->prev_handle = node;

	if (cnt == 0)
		return CA_E_UNAVAIL;
	else
		return CA_E_OK;
}

/* #ifdef CONFIG_RTK_DEV_AP */
/* CA-IROS will paser hearder file to generate api test C file. 
  If ca-network-engine doesn't provide relatived function,
  ca_sdk_cb.ko will insert fail.
*/
ca_uint16_t ca_l3_mcast_fake_vid_get(ca_uint16_t wan_vid)
{
	return aal_mc_fake_vid_get(wan_vid);
}
/* #endif */

ca_status_t ca_l3_mcast_group_add (
	CA_IN ca_device_id_t         device_id,
	CA_OUT ca_l3_mcast_entry_t *entry)
{
#ifdef CONFIG_NE_L3_MCAST
	l3_mcg_node_t *mcg_node;
	l3_src_node_t *src_node;

	spin_lock_bh(&mcast_lock);

	if (mcast_mode == CA_MC_MODE_MAC) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (!igmp_use_mc_vlan || !mld_use_mc_vlan) {
		MC_ERROR("MCAST config error!! L3 MC require fake VLAN, but igmp_use_mc_vlan %d, mld_use_mc_vlan %d\n",
				igmp_use_mc_vlan, mld_use_mc_vlan);
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (!entry) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

#ifdef CONFIG_RTK_DEV_AP 	
	if (!api_level_vlan_range_valid(entry->mcast_vlan))
		return CA_E_PARAM;
#endif
	if (entry->group_ip_addr.afi == CA_IPV4) {
		if (entry->group_ip_addr.addr_len != 32 ||
			!ipv4_is_mcast(entry->group_ip_addr.ip_addr)) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_PARAM;
		}
		if (entry->src_ip_address.addr_len > 0) {
			if (entry->src_ip_address.afi != CA_IPV4 ||
				entry->src_ip_address.addr_len != 32) {
				spin_unlock_bh(&mcast_lock);
				return CA_E_PARAM;
			}
		}
	} else {
		if (entry->group_ip_addr.addr_len != 128 ||
			!ipv6_is_mcast(entry->group_ip_addr.ip_addr)) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_PARAM;
		}
		if (entry->src_ip_address.addr_len > 0) {
			if (entry->src_ip_address.afi != CA_IPV6 ||
				entry->src_ip_address.addr_len != 128) {
				spin_unlock_bh(&mcast_lock);
				return CA_E_PARAM;
			}
		}
	}

#ifdef CONFIG_RTK_DEV_AP
	mcg_node = l3_mcg_node_get(entry->mcast_vlan, entry->group_ip_addr);
#else
	mcg_node = l3_mcg_node_get(CA_UINT16_INVALID, entry->group_ip_addr);
#endif
	if (mcg_node != NULL) {

#if defined(CONFIG_LUNA_G3_SERIES) && !defined(CONFIG_RTK_FC_IGMPV3_SUPPORT)
		if(mcg_node->is_knownMcGroup){
			// known group dip, update hash action to start hw acceleration.
#ifdef CONFIG_RTK_DEV_AP
			if (l3_mcg_node_update(entry->mcast_vlan, entry->group_ip_addr, mcg_node) == NULL)
#else			
			if (l3_mcg_node_update(CA_UINT16_INVALID, entry->group_ip_addr, mcg_node) == NULL)
#endif
				return CA_E_NOT_FOUND;
			
			if (entry->src_ip_address.addr_len == 0)
				mcg_node->is_api_own = TRUE;
		}else
#endif
		if (entry->src_ip_address.addr_len > 0) {
			src_node = l3_src_node_get(mcg_node, entry->src_ip_address);
			if (src_node != NULL) {
				spin_unlock_bh(&mcast_lock);
				return CA_E_EXISTS;
			}
		} else if (mcg_node->is_api_own) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_EXISTS;
		} else {
			mcg_node->is_api_own = TRUE;
		}
	} else {
#if defined(CONFIG_LUNA_G3_SERIES) && !defined(CONFIG_RTK_FC_IGMPV3_SUPPORT)
#ifdef CONFIG_RTK_DEV_AP 
		mcg_node = l3_mcg_node_prealloc(entry->mcast_vlan, entry->group_ip_addr);
#else
		mcg_node = l3_mcg_node_prealloc(CA_UINT16_INVALID, entry->group_ip_addr);
#endif

#else
#ifdef CONFIG_RTK_DEV_AP
		mcg_node = l3_mcg_node_alloc(entry->mcast_vlan, entry->group_ip_addr);
#else
		mcg_node = l3_mcg_node_alloc(CA_UINT16_INVALID, entry->group_ip_addr);
#endif
#endif
		if (mcg_node == NULL) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_FULL;
		}

		if (entry->src_ip_address.addr_len == 0)
			mcg_node->is_api_own = TRUE;
	}

	if (entry->src_ip_address.addr_len > 0) {
		src_node = l3_src_node_get(mcg_node, entry->src_ip_address);
		if (src_node == NULL)
			src_node = l3_src_node_add(mcg_node, entry->src_ip_address);

		if (src_node == NULL) {
			if (!mcg_node->is_api_own)
				l3_mcg_node_delete(mcg_node);
			spin_unlock_bh(&mcast_lock);
			return CA_E_FULL;
		}
		entry->mcg_id = src_node->mcgid;
	} else {
		entry->mcg_id = mcg_node->mcgid;
	}

	spin_unlock_bh(&mcast_lock);
#endif
	return CA_E_OK;
}

ca_status_t ca_l3_mcast_group_get (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32_t         mcg_id,
	CA_OUT ca_l3_mcast_iterator_entry_t *mcast_group)
{
#ifdef CONFIG_NE_L3_MCAST
	node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	l3_src_node_t *src_node;
	intf_node_t *intf_node;
	ca_mac_addr_t mac0 = {0};
	ca_uint32_t i;

	spin_lock_bh(&mcast_lock);

	if (mcast_mode == CA_MC_MODE_MAC) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (mcast_group == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

	node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcg_id, MC_L3);
	if (node == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_FOUND;
	}

	if (node->type == MCG) {
		mcg_node = (l3_mcg_node_t *) node;
		if (!mcg_node->is_api_own) {
			spin_unlock_bh(&mcast_lock);
			return CA_E_NOT_FOUND;
		}

		mcast_group->mc_entry.mcg_id = mcg_node->mcgid;
#ifdef CONFIG_RTK_DEV_AP
		mcast_group->mc_entry.mcast_vlan = mcg_node->vlan;
#endif
		mcast_group->mc_entry.group_ip_addr = mcg_node->grp_addr;
		memset(&mcast_group->mc_entry.src_ip_address, 0, sizeof(ca_ip_address_t));
		mcast_group->members.mcg_id = mcg_node->mcgid;
		i = 0;
		if (!list_empty(&mcg_node->intf_list)) {
			list_for_each_entry(intf_node, &mcg_node->intf_list, list) {
				memcpy(mcast_group->members.member[i].new_mac_da, intf_node->new_mac_da, sizeof(ca_mac_addr_t));
				if (memcmp(intf_node->new_mac_da, mac0, sizeof(ca_mac_addr_t)))
					mcast_group->members.member[i].action_mask.mac_da_overwrite = 1;
				else
					mcast_group->members.member[i].action_mask.mac_da_overwrite = 0;
				mcast_group->members.member[i].member_intf = intf_node->intf_id;
				i++;
			}
		}

		mcast_group->members.member_count = i;
	} else {
		src_node = (l3_src_node_t *) node;
		mcg_node = (l3_mcg_node_t *) src_node->parent;
		mcast_group->mc_entry.mcg_id = src_node->mcgid;
#ifdef CONFIG_RTK_DEV_AP
		mcast_group->mc_entry.mcast_vlan = mcg_node->vlan;
#endif
		mcast_group->mc_entry.group_ip_addr = mcg_node->grp_addr;
		mcast_group->mc_entry.src_ip_address = src_node->src_addr;
		mcast_group->members.mcg_id = src_node->mcgid;
		i = 0;
		if (!list_empty(&src_node->intf_list)) {
			list_for_each_entry(intf_node, &src_node->intf_list, list) {
				memcpy(mcast_group->members.member[i].new_mac_da, intf_node->new_mac_da, sizeof(ca_mac_addr_t));
				if (memcmp(intf_node->new_mac_da, mac0, sizeof(ca_mac_addr_t)))
					mcast_group->members.member[i].action_mask.mac_da_overwrite = 1;
				else
					mcast_group->members.member[i].action_mask.mac_da_overwrite = 0;
				mcast_group->members.member[i].member_intf = intf_node->intf_id;
				i++;
			}
		}

		mcast_group->members.member_count = i;
	}

	spin_unlock_bh(&mcast_lock);
#endif
	return CA_E_OK;
}

ca_status_t ca_l3_mcast_group_delete (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32_t         mcg_id)
{
#ifdef CONFIG_NE_L3_MCAST
	node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	ca_status_t ret;

	spin_lock_bh(&mcast_lock);

	if (mcast_mode == CA_MC_MODE_MAC) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcg_id, MC_L3);
	if (node == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_FOUND;
	}

	if (node->type == MCG) {
		mcg_node = (l3_mcg_node_t *) node;
		if (mcg_node->is_api_own)
			ret = l3_mcg_node_delete((l3_mcg_node_t *) node);
		else
			ret = CA_E_NOT_FOUND;
	} else {
		ret = l3_src_node_delete((l3_src_node_t *) node);
	}

	spin_unlock_bh(&mcast_lock);

	return ret;
#endif
	return CA_E_OK;
}

ca_status_t ca_l3_mcast_group_delete_all (
	CA_IN ca_device_id_t         device_id)
{
#ifdef CONFIG_NE_L3_MCAST
	ca_uint16_t i;

	if (mcast_mode == CA_MC_MODE_MAC)
		return CA_E_NOT_SUPPORT;

	for (i = 0; i < MCE_INDX_TBL_ENTRY_MAX; i++) {
		ca_l3_mcast_group_delete(device_id, i);
	}
#endif
	return CA_E_OK;
}

ca_status_t ca_l3_mcast_member_add (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_l3_mcast_group_members_t *members)
{
#ifdef CONFIG_NE_L3_MCAST
	ca_uint32_t i;
	ca_status_t ret;

	spin_lock_bh(&mcast_lock);

	if (mcast_mode == CA_MC_MODE_MAC) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (members == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

	for (i = 0; i < members->member_count; i++) {
		if (!members->member[i].action_mask.mac_da_overwrite)
			memset(members->member[i].new_mac_da, 0, sizeof(ca_mac_addr_t));
	}

	ret = l3_intf_member_add(members);

	spin_unlock_bh(&mcast_lock);

	return ret;
#else
	return CA_E_OK;
#endif
}

ca_status_t ca_l3_mcast_member_delete (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_l3_mcast_group_members_t *members)
{
#ifdef CONFIG_NE_L3_MCAST
	ca_uint32_t i;
	ca_status_t ret;

	spin_lock_bh(&mcast_lock);

	if (mcast_mode == CA_MC_MODE_MAC) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_NOT_SUPPORT;
	}

	if (members == NULL) {
		spin_unlock_bh(&mcast_lock);
		return CA_E_PARAM;
	}

	for (i = 0; i < members->member_count; i++) {
		if (!members->member[i].action_mask.mac_da_overwrite)
			memset(members->member[i].new_mac_da, 0, sizeof(ca_mac_addr_t));
	}

	ret = l3_intf_member_delete(members);

	spin_unlock_bh(&mcast_lock);

	return ret;
#else
	return CA_E_OK;
#endif
}

ca_status_t ca_l3_mcast_group_iterate (
	CA_IN ca_device_id_t                    device_id,
	CA_OUT ca_iterator_t              *return_entry)
{
#ifdef CONFIG_NE_L3_MCAST
	l3_node_obj_t *node;
	ca_uint16_t mcgid, query_count, cnt, i;
	ca_l3_mcast_iterator_entry_t *entry_ptr;
	ca_status_t ret;

	if (mcast_mode == CA_MC_MODE_MAC) {
		return CA_E_NOT_SUPPORT;
	}

	if (return_entry == NULL ||
		return_entry->entry_count == 0 ||
		return_entry->entry_data == NULL) {
		return CA_E_PARAM;
	}

	query_count = return_entry->user_buffer_size / sizeof(ca_l3_mcast_iterator_entry_t);
	if (query_count > return_entry->entry_count) {
		query_count = return_entry->entry_count;
	}

	node = return_entry->prev_handle;
	if (node == NULL)
		mcgid = 0;
	else
		mcgid = node->mcgid + 1;

	entry_ptr = return_entry->entry_data;
	cnt = 0;
	for (i = mcgid; i < MCE_INDX_TBL_ENTRY_MAX; i++) {
		if (cnt == query_count)
			break;
		ret = ca_l3_mcast_group_get(0, i, entry_ptr);
		if (ret == CA_E_OK) {
			return_entry->entry_index[cnt] = entry_ptr->mc_entry.mcg_id;
			node = (l3_node_obj_t *) aal_mc_fake_mcgid_ptr_get(i, MC_L3);
			entry_ptr++;
			cnt++;
		}
	}

	return_entry->entry_count = cnt;
	return_entry->prev_handle = node;

	if (cnt == 0)
		return CA_E_UNAVAIL;
	else
		return CA_E_OK;
#else
	return CA_E_UNAVAIL;
#endif
}

void ca_api_ip_address_htonl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src);
void l2_mcast_table_dump(void)
{
	ca_iterator_t   iterator;
	ca_l2_mcast_entry_t             mcast_entry;
	ca_l2_mcast_group_members_t     *mcast_grp_members;
	ca_l2_mcast_iterator_entry_t    *mcast_itr_entry;
	ca_uint32_t     count;
	ca_status_t     ret;
	int i, index, mce_indx;
	node_obj_t *node = NULL;

	mcast_itr_entry = ca_zalloc_trace(sizeof(ca_l2_mcast_iterator_entry_t), GFP_KERNEL, true);

	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = mcast_itr_entry;
	iterator.user_buffer_size = sizeof(ca_l2_mcast_iterator_entry_t);

	index = 0;
	while (1) {
		ret = ca_l2_mcast_group_iterate(0, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;

		printk("================= index=%d  =====================\n", index);

		mcast_entry = mcast_itr_entry->mc_entry;
		mcast_grp_members = &mcast_itr_entry->members;

		printk("replication_only = 0x%04x\n", mcast_entry.replication_only);
		printk("mcast_vlan=%d, group address afi=%d\n", mcast_entry.mcast_vlan, mcast_entry.group_ip_addr.afi);
		printk("group mac address=%pM\n", &(mcast_entry.group_mac_addr));
		ca_api_ip_address_htonl(&(mcast_entry.group_ip_addr), &(mcast_entry.group_ip_addr));
		if (mcast_entry.group_ip_addr.afi == CA_IPV4) {
			printk("group address=%pI4\n", &(mcast_entry.group_ip_addr.ip_addr.ipv4_addr));
		}
		else {
			printk("group address=%pI6\n", &(mcast_entry.group_ip_addr.ip_addr.ipv6_addr[0]));
		}

		ca_api_ip_address_htonl(&(mcast_entry.src_ip_address), &(mcast_entry.src_ip_address));
		if (mcast_entry.src_ip_address.afi == CA_IPV4)
			printk("source address=%pI4\n", &(mcast_entry.src_ip_address.ip_addr.ipv4_addr));
		else
			printk("source address=%pI6\n", &(mcast_entry.src_ip_address.ip_addr.ipv6_addr));

		node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcast_grp_members->mcg_id, MC_L2);
		mce_indx = node ? node->mce_indx : (-1);
		printk("group member mcg_id=%d, mce_indx=%d, member_count=%d\n", mcast_grp_members->mcg_id, mce_indx, mcast_grp_members->member_count);
		for (i= 0; i < mcast_grp_members->member_count; i++) {
			printk("[group member %d]\n", i);
			printk("    action_mask.mac_da_overwrite=%d, action_mask.vlan_action=%d\n",
					mcast_grp_members->member[i].action_mask.mac_da_overwrite, mcast_grp_members->member[i].action_mask.vlan_action);
			printk("    new_mac_da=%pM\n", &(mcast_grp_members->member[i].new_mac_da));
			printk("    vid=%d, member_port=0x%x, vlan_action=%d\n", mcast_grp_members->member[i].vid,
					mcast_grp_members->member[i].member_port, mcast_grp_members->member[i].vlan_action);
		}

		index++;
	}
}

#ifdef CONFIG_NE_L3_MCAST
void l3_mcast_table_dump(void)
{
        ca_iterator_t   iterator;
        ca_l3_mcast_entry_t             mcast_entry;
        ca_l3_mcast_group_members_t     *mcast_grp_members;
        ca_l3_mcast_iterator_entry_t    *mcast_itr_entry;
        ca_uint32_t     count;
        ca_status_t     ret;
        int i, index, mce_indx;
        node_obj_t *node = NULL;

	mcast_itr_entry = ca_zalloc_trace(sizeof(ca_l3_mcast_iterator_entry_t), GFP_KERNEL, true);

        iterator.prev_handle = NULL;
        count = 1;
        iterator.entry_count = count;
        iterator.entry_data = mcast_itr_entry;
        iterator.user_buffer_size = sizeof(ca_l3_mcast_iterator_entry_t);

        index = 0;
        while (1) {
                ret = ca_l3_mcast_group_iterate(0, &iterator);
                if (ret != CA_E_OK || iterator.entry_count == 0)
                        break;

                printk("================= index=%d  =====================\n", index);

                mcast_entry = mcast_itr_entry->mc_entry;
                mcast_grp_members = &mcast_itr_entry->members;
#ifdef CONFIG_RTK_DEV_AP
				printk("mcast_vlan=%d, group address afi=%d\n", mcast_entry.mcast_vlan, mcast_entry.group_ip_addr.afi);
#else
                printk("group address afi=%d\n", mcast_entry.group_ip_addr.afi);
#endif
	            ca_api_ip_address_htonl(&(mcast_entry.group_ip_addr), &(mcast_entry.group_ip_addr));
                if (mcast_entry.group_ip_addr.afi == CA_IPV4) {
                        printk("group address=%pI4\n", &(mcast_entry.group_ip_addr.ip_addr.ipv4_addr));
                }
                else {
                        printk("group address=%pI6\n", &(mcast_entry.group_ip_addr.ip_addr.ipv6_addr[0]));
                }

                ca_api_ip_address_htonl(&(mcast_entry.src_ip_address), &(mcast_entry.src_ip_address));
                if (mcast_entry.src_ip_address.afi == CA_IPV4)
                        printk("source address=%pI4\n", &(mcast_entry.src_ip_address.ip_addr.ipv4_addr));
                else
                        printk("source address=%pI6\n", &(mcast_entry.src_ip_address.ip_addr.ipv6_addr));

                node = (node_obj_t *) aal_mc_fake_mcgid_ptr_get(mcast_grp_members->mcg_id, MC_L3);
				mce_indx = node ? node->mce_indx : (-1);
                printk("group member mcg_id=%d, mce_indx=%d, member_count=%d\n", mcast_grp_members->mcg_id, mce_indx, mcast_grp_members->member_count);
                for (i= 0; i < mcast_grp_members->member_count; i++) {
                        printk("[group member %d]\n", i);
                        printk("    action_mask.mac_da_overwrite=%d\n", mcast_grp_members->member[i].action_mask.mac_da_overwrite);
                        printk("    new_mac_da=%pM\n", &(mcast_grp_members->member[i].new_mac_da));
                        printk("    member_intf=%d\n", mcast_grp_members->member[i].member_intf);
                }

                index++;
        }
}
#endif

ca_status_t ca_mcast_init(void)
{
	ca_status_t ret;

	ret = aal_scfg_read(CFG_ID_NE_MAC_ADDR1, 6, ca_ne_mac_addr1);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to init Mcast API. (ret %d)\n", __func__, ret);
		return ret;
	}

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_INDEX_MODE, 1, &l3fe_hash_action_indexed_mode);
#endif

	spin_lock_init(&mcast_lock);
	return CA_E_OK;
}

#if 0
ca_status_t ca_l3_mcast_update_IPDA_entry (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32 mcg_id,
	CA_IN ca_uint32 v3_ind)
{
	node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	im3e_mldda_entry_t da_entry;

	if (mcast_mode == CA_MC_MODE_MAC)
		return CA_E_NOT_SUPPORT;

	node = (node_obj_t *) aal_mc_mcgid_ptr_get(mcg_id, MC_L3);
	if (node == NULL)
		return CA_E_NOT_FOUND;

	if (node->type == MCG) {
		mcg_node = (l3_mcg_node_t *) node;
		
		// update 
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
		da_entry.v3_ind = v3_ind;
		aal_entry_set(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
	}else
		return CA_E_NOT_FOUND;
	
	return CA_E_OK;
}

ca_status_t ca_l2_mcast_update_IPDA_entry (
	CA_IN ca_device_id_t         device_id,
	CA_IN ca_uint32 mcg_id,
	CA_IN ca_uint32 v3_ind)
{
	node_obj_t *node;
	l3_mcg_node_t *mcg_node;
	im3e_mldda_entry_t da_entry;

	node = (node_obj_t *) aal_mc_mcgid_ptr_get(mcg_id, MC_L2);
	if (node == NULL)
		return CA_E_NOT_FOUND;

	if (node->type == MCG) {
		mcg_node = (l2_mcg_node_t *) node;
		
		// update 
		aal_entry_get_by_idx(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
		da_entry.v3_ind = v3_ind;
		aal_entry_set(AAL_TABLE_IM3E_MLDDA, mcg_node->entry_idx, &da_entry);
	}else
		return CA_E_NOT_FOUND;
	
	return CA_E_OK;
}
#endif /*#if 0*/

EXPORT_SYMBOL(ca_l2_mcast_config_set);
EXPORT_SYMBOL(ca_l2_mcast_config_get);
EXPORT_SYMBOL(ca_l2_mcast_group_add);
EXPORT_SYMBOL(ca_l2_mcast_group_get);
EXPORT_SYMBOL(ca_l2_mcast_group_delete);
EXPORT_SYMBOL(ca_l2_mcast_group_delete_all);
EXPORT_SYMBOL(ca_l2_mcast_member_add);
EXPORT_SYMBOL(ca_l2_mcast_member_delete);
EXPORT_SYMBOL(ca_l2_mcast_group_iterate);
EXPORT_SYMBOL(ca_l3_mcast_group_add);
EXPORT_SYMBOL(ca_l3_mcast_group_get);
EXPORT_SYMBOL(ca_l3_mcast_group_delete);
EXPORT_SYMBOL(ca_l3_mcast_group_delete_all);
EXPORT_SYMBOL(ca_l3_mcast_member_add);
EXPORT_SYMBOL(ca_l3_mcast_member_delete);
EXPORT_SYMBOL(ca_l3_mcast_group_iterate);
/* #ifdef CONFIG_RTK_DEV_AP*/
EXPORT_SYMBOL(ca_l3_mcast_fake_vid_get);
//EXPORT_SYMBOL(ca_l3_mcast_update_IPDA_entry);
//EXPORT_SYMBOL(ca_l2_mcast_update_IPDA_entry);

