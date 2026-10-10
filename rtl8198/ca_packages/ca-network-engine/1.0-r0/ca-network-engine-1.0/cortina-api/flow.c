#include <linux/list.h>
#include <linux/list_sort.h>

#include "flow.h"
#include "nat.h"
#include "rate.h"
#include "route_internal.h"
#include "tunnel.h"
#include "ca-scfg/scfg.h"
#include "aal_hash.h"
#include "aal_l3_cam.h"
#include "aal_l3_pe.h"
#include "aal_l2_tm.h"
#include "aal_l3_tm.h"
#include "aal_hashlite.h"
#include "aal_l3_if.h"
#include "aal_common.h"
#include "ca_mut.h"
#include "api_common.h"

static ca_uint8_t flow_support = 0;
static ca_uint8_t l3fe_hash_action_indexed_mode = 0;

extern ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];
extern struct ca_int_l3_intf_s *ca_l3_intf_tbl;
extern struct list_head nexthop_table;

#if defined(CONFIG_LUNA_G3_SERIES)
//aal_l3fe.c
static ca_uint16_t flow_profile_map[CA_FLOW_TYPE_MAX];

#if defined(CONFIG_ARCH_CORTINA_VENUS)
static ca_uint8_t l3fe_egress_mirrored_port = AAL_LPORT_INVALID;
extern ca_uint8_t l3qm_ldpid_to_pon_port_map_offset;
#endif
#endif

/***************************************************************************
 * internal functions - Generating Hash Masks/Keys/Actions
 ***************************************************************************/

static ca_status_t get_nexthop_id_by_tunnel_id(ca_tunnel_id_t tunnel_id, ca_l3_nexthop_id_t *nexthop_id)
{
	int i;
	bool found_intf = false, found_nexthop = false;
	struct nexthop_node *iter_nh = NULL;
	ca_intf_id_t intf_id;

	if (nexthop_id == NULL)
		return CA_E_PARAM;

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
		if (ca_l3_intf_tbl[i].use == 0) continue;

		if (   (ca_l3_intf_tbl[i].intf.type == CA_L3_INTF_TUNNEL)
				&& (ca_l3_intf_tbl[i].intf.mask.tunnel_id != 0)
				&& (ca_l3_intf_tbl[i].intf.tunnel_id == tunnel_id) ) {
			found_intf = true;
			intf_id = i;
			break;
		}
	}

	if (found_intf) {
		list_for_each_entry(iter_nh, &nexthop_table, list) {
			if (iter_nh->nexthop.intf_id == intf_id) {
				found_nexthop = true;
				*nexthop_id = iter_nh->nexthop.nexthop_id;
				return CA_E_OK;
			}
		}
	}

	return CA_E_NOT_FOUND;
}

/* Generate hash_mask according to flow_key_mask.
 * Parameters:
 * - flow_key_mask: Mask of a flow key
 * - hash_mask: Return the generated hash mask
 */
static ca_status_t generate_hash_mask(ca_flow_key_mask_t *flow_key_mask, aal_hash_mask_t *hash_mask)
{
	if ((flow_key_mask == NULL) || (hash_mask == NULL))
		return CA_E_PARAM;

	hash_mask_unmask(hash_mask);

	if (flow_key_mask->l2_keys) {
		if (flow_key_mask->l2_mask.o_lspid)
			hash_mask->orig_lspid = 0;

		if (flow_key_mask->l2_mask.lspid)
			hash_mask->lspid = 0;

		if (flow_key_mask->l2_mask.mac_sa)
			hash_mask->mac_sa = 0;

		if (flow_key_mask->l2_mask.mac_da)
			hash_mask->mac_da = 0;

		if (flow_key_mask->l2_mask.outer_vlan.tpid)
			hash_mask->top_tpid_enc = 0;
		if (flow_key_mask->l2_mask.outer_vlan.pri)
			hash_mask->top_8021p = 0;
		if (flow_key_mask->l2_mask.outer_vlan.dei)
			hash_mask->top_dei = 0;
		if (flow_key_mask->l2_mask.outer_vlan.vid)
			hash_mask->top_vid = 0;

		if (flow_key_mask->l2_mask.inner_vlan.tpid)
			hash_mask->inner_tpid_enc = 0;
		if (flow_key_mask->l2_mask.inner_vlan.pri)
			hash_mask->inner_8021p = 0;
		if (flow_key_mask->l2_mask.inner_vlan.dei)
			hash_mask->inner_dei = 0;
		if (flow_key_mask->l2_mask.inner_vlan.vid)
			hash_mask->inner_vid = 0;
	}

	if (flow_key_mask->l3_keys) {
		if (flow_key_mask->l3_mask.ip_valid)
			hash_mask->ip_vld = 0;

		if (flow_key_mask->l3_mask.ip_version)
			hash_mask->ip_ver = 0;

		if (flow_key_mask->l3_mask.ip_protocol)
			hash_mask->ip_protocol = 0;

		if (flow_key_mask->l3_mask.dscp)
			hash_mask->ip_dscp = 0;

		if (flow_key_mask->l3_mask.ecn)
			hash_mask->ip_ecn = 0;

		if (flow_key_mask->l3_mask.ip_sa)
			hash_mask->ip_sa = 128;

		if (flow_key_mask->l3_mask.ip_da)
			hash_mask->ip_da = 128;

		if (flow_key_mask->l3_mask.fragment)
			hash_mask->ip_fragment_flag = 0;

		if (flow_key_mask->l3_mask.icmp_type) {
			hash_mask->icmp_vld = 0;
			hash_mask->icmp_type = 0;
		}

		if (flow_key_mask->l3_mask.igmp_type) {
			hash_mask->icmp_vld = 0;
			hash_mask->icmp_type = 0;
		}

		if (flow_key_mask->l3_mask.hbh_header){
			hash_mask->ipv6_hbh = 0;
#if defined(CONFIG_LUNA_G3_SERIES)
			hash_mask->ip_options = 0;
#endif
		}

		if (flow_key_mask->l3_mask.routing_header)
			hash_mask->ipv6_rh = 0;

		if (flow_key_mask->l3_mask.dest_opt_header)
			hash_mask->ipv6_doh = 0;

		switch (flow_key_mask->l3_mask.ip_ttl) {
		case 0xFF:
			hash_mask->ip_ttl = 2; /* exactly matched TTL */
			break;
		case 0xFE:
		case 0xFC:
		case 0xF8:
		case 0xF0:
		case 0xE0:
		case 0xC0:
		case 0x80:
			hash_mask->ip_ttl = 3; /* ranged TTL */
			break;
		case 0x00:
			break;
		default:
			return CA_E_PARAM;
		}
	}

	if (flow_key_mask->l4_keys) {
		if (flow_key_mask->l4_mask.src_l4_port)
			hash_mask->l4_sp_exact_range = 0; /* exactly match */

		if (flow_key_mask->l4_mask.dst_l4_port)
			hash_mask->l4_dp_exact_range = 0; /* exactly match */

		if (flow_key_mask->l4_mask.tcp_flags)
			hash_mask->tcp_rdp_ctrl = ~flow_key_mask->l4_mask.tcp_flags;
	}

	if (flow_key_mask->flow_id) {
		hash_mask->pol_id = 0;
	}

	if (flow_key_mask->sw_id) {
		/* mdata[55:32], 24 bits */
		hash_mask->mdata = 0xff000000ffffffff;
	}

	return CA_E_OK;
}

/* Generate hash_key according to flow_key and flow_key_mask.
 * Parameters:
 * - flow_key: Keys of a flow
 * - flow_key_mask: Key mask of the flow_key
 * - hash_key: Return the generated hash key
 */
static ca_status_t generate_hash_key(ca_flow_key_t *flow_key, ca_flow_key_mask_t *flow_key_mask, aal_hash_key_t *hash_key)
{
	int ip_version = -1; /* 4 is IPv4, 6 is IPv6, others are unknown */

	if ((flow_key == NULL) || (flow_key_mask == NULL) || (hash_key == NULL))
		return CA_E_PARAM;

	/* check if ip_version is already known from flow_key */
	if (flow_key_mask->l3_keys) {
		/* decide from l3_key.ip_version */
		if (flow_key_mask->l3_mask.ip_version) {
			if (flow_key->l3_key.ip_version == 4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_version == 6)
				ip_version = 6;
		}

		/* compare l3_key.ip_version and l3_key.ip_sa.afi */
		if (flow_key_mask->l3_mask.ip_sa) {
			if (((ip_version == 4) && (flow_key->l3_key.ip_sa.afi == CA_IPV6)) ||
			    ((ip_version == 6) && (flow_key->l3_key.ip_sa.afi == CA_IPV4)))
				return CA_E_PARAM;

			if (flow_key->l3_key.ip_sa.afi == CA_IPV4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_sa.afi == CA_IPV6)
				ip_version = 6;
		}

		/* compare l3_key.ip_version and l3_key.ip_da.afi */
		if (flow_key_mask->l3_mask.ip_da) {
			if (((ip_version == 4) && (flow_key->l3_key.ip_da.afi == CA_IPV6)) ||
			    ((ip_version == 6) && (flow_key->l3_key.ip_da.afi == CA_IPV4)))
				return CA_E_PARAM;

			if (flow_key->l3_key.ip_da.afi == CA_IPV4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_da.afi == CA_IPV6)
				ip_version = 6;
		}

		/* compare l3_key.ip_sa.afi and l3_key.ip_da.afi; probably unnecessary comparison */
		if (flow_key_mask->l3_mask.ip_sa && flow_key_mask->l3_mask.ip_da) {
			if (flow_key->l3_key.ip_sa.afi != flow_key->l3_key.ip_da.afi)
				return CA_E_PARAM;
		}
	}

	if (ca_api_debug & API_DBG_FLOW) {
		printk("%s: guessed ip_version=%d\n", __func__, ip_version);
	}

	memset(hash_key, 0, sizeof(*hash_key));

	if (flow_key_mask->l2_keys) {
		if (flow_key_mask->l2_mask.o_lspid)
			hash_key->orig_lspid = flow_key->l2_key.o_lspid;

		if (flow_key_mask->l2_mask.lspid)
			hash_key->lspid = flow_key->l2_key.lspid;

		if (flow_key_mask->l2_mask.mac_sa) {
			hash_key->mac_sa_5 = flow_key->l2_key.mac_sa[0];
			hash_key->mac_sa_4 = flow_key->l2_key.mac_sa[1];
			hash_key->mac_sa_3 = flow_key->l2_key.mac_sa[2];
			hash_key->mac_sa_2 = flow_key->l2_key.mac_sa[3];
			hash_key->mac_sa_1 = flow_key->l2_key.mac_sa[4];
			hash_key->mac_sa_0 = flow_key->l2_key.mac_sa[5];
		}

		if (flow_key_mask->l2_mask.mac_da) {
			hash_key->mac_da_5 = flow_key->l2_key.mac_da[0];
			hash_key->mac_da_4 = flow_key->l2_key.mac_da[1];
			hash_key->mac_da_3 = flow_key->l2_key.mac_da[2];
			hash_key->mac_da_2 = flow_key->l2_key.mac_da[3];
			hash_key->mac_da_1 = flow_key->l2_key.mac_da[4];
			hash_key->mac_da_0 = flow_key->l2_key.mac_da[5];
		}

		if (flow_key_mask->l2_mask.outer_vlan.tpid) {
			ca_uint8_t tpid_idx;
			if (aal_l3fe_pp_top_tpid_get(flow_key->l2_key.outer_vlan.tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;
			hash_key->top_tpid_enc = tpid_idx;
		}
		if (flow_key_mask->l2_mask.outer_vlan.pri)
			hash_key->top_8021p = flow_key->l2_key.outer_vlan.pri;
		if (flow_key_mask->l2_mask.outer_vlan.dei)
			hash_key->top_dei = flow_key->l2_key.outer_vlan.dei;
		if (flow_key_mask->l2_mask.outer_vlan.vid)
			hash_key->top_vid = flow_key->l2_key.outer_vlan.vid;

		if (flow_key_mask->l2_mask.inner_vlan.tpid) {
			ca_uint8_t tpid_idx;
			if (aal_l3fe_pp_top_tpid_get(flow_key->l2_key.inner_vlan.tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;
			hash_key->inner_tpid_enc = tpid_idx;
		}
		if (flow_key_mask->l2_mask.inner_vlan.pri)
			hash_key->inner_8021p = flow_key->l2_key.inner_vlan.pri;
		if (flow_key_mask->l2_mask.inner_vlan.dei)
			hash_key->inner_dei = flow_key->l2_key.inner_vlan.dei;
		if (flow_key_mask->l2_mask.inner_vlan.vid)
			hash_key->inner_vid = flow_key->l2_key.inner_vlan.vid;
	}

	if (flow_key_mask->l3_keys) {
		if (flow_key_mask->l3_mask.ip_valid)
			hash_key->ip_vld = flow_key->l3_key.ip_valid;

		if (flow_key_mask->l3_mask.ip_version) {
			if (flow_key->l3_key.ip_version == 4)
				hash_key->ip_ver = 0;
			else if (flow_key->l3_key.ip_version == 6)
				hash_key->ip_ver = 1;
			else
				return CA_E_PARAM;
		}

		if (flow_key_mask->l3_mask.ip_protocol)
			hash_key->ip_protocol = flow_key->l3_key.ip_protocol;

		if (flow_key_mask->l3_mask.dscp)
			hash_key->ip_dscp = flow_key->l3_key.dscp;

		if (flow_key_mask->l3_mask.ecn)
			hash_key->ip_ecn = flow_key->l3_key.ecn;

		if (flow_key_mask->l3_mask.ip_sa) {
			if (ip_version == 4) {
				hash_key->ip_sa_0 = flow_key->l3_key.ip_sa.ip_addr.ipv4_addr;
			} else if (ip_version == 6) {
				hash_key->ip_sa_3 = flow_key->l3_key.ip_sa.ip_addr.ipv6_addr[0]; /* MSB */
				hash_key->ip_sa_2 = flow_key->l3_key.ip_sa.ip_addr.ipv6_addr[1];
				hash_key->ip_sa_1 = flow_key->l3_key.ip_sa.ip_addr.ipv6_addr[2];
				hash_key->ip_sa_0 = flow_key->l3_key.ip_sa.ip_addr.ipv6_addr[3];
			} else {
				return CA_E_PARAM;
			}
		}

		if (flow_key_mask->l3_mask.ip_da) {
			if (ip_version == 4) {
				hash_key->ip_da_0 = flow_key->l3_key.ip_da.ip_addr.ipv4_addr;
			} else  if (ip_version == 6) {
				hash_key->ip_da_3 = flow_key->l3_key.ip_da.ip_addr.ipv6_addr[0]; /* MSB */
				hash_key->ip_da_2 = flow_key->l3_key.ip_da.ip_addr.ipv6_addr[1];
				hash_key->ip_da_1 = flow_key->l3_key.ip_da.ip_addr.ipv6_addr[2];
				hash_key->ip_da_0 = flow_key->l3_key.ip_da.ip_addr.ipv6_addr[3];
			} else {
				return CA_E_PARAM;
			}
		}

		if (flow_key_mask->l3_mask.fragment)
			hash_key->ip_fragment_flag = !!flow_key->l3_key.fragment;

		if (flow_key_mask->l3_mask.icmp_type) {
			switch (flow_key->l3_key.icmp_type) {
			case 130: case 131: case 132:
			case 143:
				hash_key->icmp_vld = 7; /* MLD */
				break;
			case 133: case 134: case 135: case 136: case 137:
			case 146: case 147:
				hash_key->icmp_vld = 3; /* NDP */
				break;
			default:
				if (ip_version == 4)
					hash_key->icmp_vld = 4; /* ICMPv4 */
				else if (ip_version == 6)
					hash_key->icmp_vld = 5; /* ICMPv6 */
				else
					return CA_E_PARAM;
				break;
			}
			hash_key->icmp_type = flow_key->l3_key.icmp_type;
		}

		if (flow_key_mask->l3_mask.igmp_type) {
			hash_key->icmp_vld = 6; /* IGMP */
			hash_key->icmp_type = flow_key->l3_key.igmp_type;
		}

		if (flow_key_mask->l3_mask.hbh_header)
			hash_key->ipv6_hbh = flow_key->l3_key.hbh_header;

		if (flow_key_mask->l3_mask.routing_header)
			hash_key->ipv6_rh = flow_key->l3_key.routing_header;

		if (flow_key_mask->l3_mask.dest_opt_header)
			hash_key->ipv6_doh = flow_key->l3_key.dest_opt_header;

		switch (flow_key_mask->l3_mask.ip_ttl) {
		case 0xFF:
			hash_key->ip_ttl = flow_key->l3_key.ip_ttl; /* exactly matched TTL */
			break;
		case 0xFE:
			if (flow_key->l3_key.ip_ttl != 0)
				return CA_E_PARAM;
			hash_key->ip_ttl = 2; /* TTL 0~1 */
			break;
		case 0xFC:
			if (flow_key->l3_key.ip_ttl != 0)
				return CA_E_PARAM;
			hash_key->ip_ttl = 4; /* TTL 0~3 */
			break;
		case 0xF8:
			if (flow_key->l3_key.ip_ttl != 0)
				return CA_E_PARAM;
			hash_key->ip_ttl = 5; /* TTL 0~7 */
			break;
		case 0xF0:
			if (flow_key->l3_key.ip_ttl != 0)
				return CA_E_PARAM;
			hash_key->ip_ttl = 6; /* TTL 0~15 */
			break;
		case 0xE0:
			hash_key->ip_ttl = 7; /* TTL 0~31 */
			break;
		case 0xC0:
			if (flow_key->l3_key.ip_ttl != 0)
				return CA_E_PARAM;
			hash_key->ip_ttl = 8; /* TTL 0~63 */
			break;
		case 0x80:
			if (flow_key->l3_key.ip_ttl == 0)
				hash_key->ip_ttl = 9; /* TTL 0~127 */
			else if (flow_key->l3_key.ip_ttl == 0x10)
				hash_key->ip_ttl = 10; /* TTL 128~255 */
			else
				return CA_E_PARAM;
			break;
		case 0x00:
			break;
		default:
			return CA_E_PARAM;
		}
	}

	if (flow_key_mask->l4_keys) {
		if (flow_key_mask->l4_mask.src_l4_port)
			hash_key->l4_sp_exact_range = flow_key->l4_key.src_l4_port;

		if (flow_key_mask->l4_mask.dst_l4_port)
			hash_key->l4_dp_exact_range = flow_key->l4_key.dst_l4_port;

		if (flow_key_mask->l4_mask.tcp_flags)
			hash_key->tcp_rdp_ctrl = flow_key->l4_key.tcp_flags & 0x1ff;
	}

	if (flow_key_mask->flow_id) {
		hash_key->pol_id = flow_key->flow_id;
	}

	if (flow_key_mask->sw_id) {
		/* mdata[55:32], 24 bits */
		hash_key->mdata = ((uint64_t) (flow_key->sw_id[1] & 0xFF) << 48) | ((uint64_t) flow_key->sw_id[0] << 32);
	}

	return CA_E_OK;
}

/* Generate hash_action according to flow_action.
 * Parameters:
 * - flow_key: Key of a flow; some info are required by hash_action
 * - flow_key_mask: key mask of flow_key; some info are required by hash_action
 * - flow_action: Action of a flow
 * - hash_action: Return the hash action generated from flow_action
 * - action_group_mask: Return the hash action group mask that used in hash_action
 */
static ca_status_t generate_hash_action(
	ca_flow_key_t *flow_key,
	ca_flow_key_mask_t *flow_key_mask,
	ca_flow_action_t *flow_action,
	aal_hash_action_t *hash_action)
{
	int o_vid_cnt = 0, i_vid_cnt = 0, o_tpid_enc = 1, i_tpid_enc = 1, o_dei = 0, i_dei = 0, rslt_idx = 0, ret;
	int ip_version = -1; /* 4 is IPv4, 6 is IPv6, others are unknown */
	ca_uint32_t ipv6_addr[4];
	ca_l3_nexthop_id_t nexthop_id;

	if ((flow_action == NULL) || (hash_action == NULL))
		return CA_E_PARAM;

	/* check if ip_version is already known from flow_key */
	if (flow_key_mask->l3_keys) {
		/* decide from l3_key.ip_version */
		if (flow_key_mask->l3_mask.ip_version) {
			if (flow_key->l3_key.ip_version == 4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_version == 6)
				ip_version = 6;
		}

		/* compare l3_key.ip_version and l3_key.ip_sa.afi */
		if (flow_key_mask->l3_mask.ip_sa) {
			if (((ip_version == 4) && (flow_key->l3_key.ip_sa.afi == CA_IPV6)) ||
			    ((ip_version == 6) && (flow_key->l3_key.ip_sa.afi == CA_IPV4)))
				return CA_E_PARAM;

			if (flow_key->l3_key.ip_sa.afi == CA_IPV4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_sa.afi == CA_IPV6)
				ip_version = 6;
		}

		/* compare l3_key.ip_version and l3_key.ip_da.afi */
		if (flow_key_mask->l3_mask.ip_da) {
			if (((ip_version == 4) && (flow_key->l3_key.ip_da.afi == CA_IPV6)) ||
			    ((ip_version == 6) && (flow_key->l3_key.ip_da.afi == CA_IPV4)))
				return CA_E_PARAM;

			if (flow_key->l3_key.ip_da.afi == CA_IPV4)
				ip_version = 4;
			else if (flow_key->l3_key.ip_da.afi == CA_IPV6)
				ip_version = 6;
		}

		/* compare l3_key.ip_sa.afi and l3_key.ip_da.afi; probably unnecessary comparison */
		if (flow_key_mask->l3_mask.ip_sa && flow_key_mask->l3_mask.ip_da) {
			if (flow_key->l3_key.ip_sa.afi != flow_key->l3_key.ip_da.afi)
				return CA_E_PARAM;
		}
	}

	if (ca_api_debug & API_DBG_FLOW) {
		printk("%s: guessed ip_version=%d\n", __func__, ip_version);
	}

	/* check if tpid and dei are already known form flow_key */
	if (flow_key_mask->l2_keys) {
		if (flow_key_mask->l2_mask.outer_vlan.tpid) {
			ca_uint8_t tpid_idx;
			if (aal_l3fe_pp_top_tpid_get(flow_key->l2_key.outer_vlan.tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;
			o_tpid_enc = tpid_idx;
		}

		if (flow_key_mask->l2_mask.inner_vlan.tpid) {
			ca_uint8_t tpid_idx;
			if (aal_l3fe_pp_top_tpid_get(flow_key->l2_key.inner_vlan.tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;
			i_tpid_enc = tpid_idx;
		}

		if (flow_key_mask->l2_mask.outer_vlan.dei) {
			o_dei = flow_key->l2_key.outer_vlan.dei;
		}

		if (flow_key_mask->l2_mask.inner_vlan.dei) {
			i_dei = flow_key->l2_key.inner_vlan.dei;
		}
	}

	memset(hash_action, 0, sizeof(*hash_action));

	if (flow_action->options.masks.flow_id) {
#if defined(CONFIG_LUNA_G3_SERIES)
		if (flow_action->options.flow_id > CA_AAL_L3_MAX_FLOW_ID)
#else
		if (flow_action->options.flow_id > CA_AAL_MAX_FLOW_ID || flow_action->options.flow_id > CA_AAL_L3_MAX_FLOW_ID)
#endif
			return CA_E_PARAM;
		hash_action->pol_vld = 1;
		hash_action->pol_en = 2;
		hash_action->pol_base = flow_action->options.flow_id;
		hash_action->pol_sel = 2;
	}

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
	if (flow_action->options.masks.flow_id2) {
		if (flow_action->options.flow_id2 > CA_AAL_L3_MAX_FLOW2_ID)
			return CA_E_PARAM;
		hash_action->pol23_vld = 1;
		hash_action->pol2_msb_en = 1;
		hash_action->pol2_id_msb = (flow_action->options.flow_id2 >> 3) & 0x7;
		hash_action->pol_vld = 1;
		hash_action->pol_grp_en = 1;
		hash_action->pol_grp_id = flow_action->options.flow_id2 & 0x7;
	}

	if (flow_action->options.masks.flow_id3) {
		if (flow_action->options.flow_id3 > CA_AAL_L3_MAX_FLOW3_ID)
			return CA_E_PARAM;
		hash_action->pol23_vld = 1;
		hash_action->pol3_en = 1;
		hash_action->pol3_id = flow_action->options.flow_id3;
	}
	
#else
	if (flow_action->options.masks.flow_id2) {
		return CA_E_NOT_SUPPORT;
	}

	if (flow_action->options.masks.flow_id3) {
		return CA_E_NOT_SUPPORT;
	}
#endif

	if (flow_action->options.masks.cos) {
		hash_action->cos = flow_action->options.cos;
		hash_action->cos_sel = 1; /* use FIB result */
	}

	if (flow_action->options.masks.dscp) {
		hash_action->ip_dscp_sel = 1; /* use FIB result */
		hash_action->ip_dscp = flow_action->options.dscp;
		hash_action->ip_dscp_update_en = 3; /* update DSCP bit[5:0] */
	}

	if (flow_action->options.masks.decrement_ttl) {
		if (l3fe_hash_action_indexed_mode == 0) {
			hash_action->ip_ttl_cmd = 1; /* decrease TTL */
			hash_action->ip_ttl_vld = 1;
		} else {
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			hash_action->ip_ttl_dec1 = 1;
			hash_action->ip_ttl_zero_discard_en1 = 1;
#endif
		}
	}

	if (flow_action->options.masks.outer_vlan_act || flow_action->options.masks.outer_dot1p ||
	    flow_action->options.masks.outer_tpid || flow_action->options.masks.outer_dei ||
	    flow_action->options.masks.inner_vlan_act || flow_action->options.masks.inner_dot1p ||
	    flow_action->options.masks.inner_tpid || flow_action->options.masks.inner_dei) {

		hash_action->vlan_vld = 1;

		if (flow_action->options.masks.outer_vlan_act) {
			switch (flow_action->options.outer_vlan_act) {
			case CA_FLOW_VLAN_ACTION_SET:
				hash_action->top_vid = flow_action->options.outer_vid;
				o_vid_cnt = 1;
				break;
			case CA_FLOW_VLAN_ACTION_POP:
				o_vid_cnt = 0;
				break;
			default:
				return CA_E_NOT_SUPPORT;
			}
		}

		if (flow_action->options.masks.inner_vlan_act) {
			switch (flow_action->options.inner_vlan_act) {
			case CA_FLOW_VLAN_ACTION_SET:
				if ((!flow_action->options.masks.outer_vlan_act) || (flow_action->options.outer_vlan_act == CA_FLOW_VLAN_ACTION_POP))
					return CA_E_NOT_SUPPORT;
				hash_action->inner_vid = flow_action->options.inner_vid;
				i_vid_cnt = 1;
				break;
			default:
				return CA_E_NOT_SUPPORT;
			}
		}

		if ((o_vid_cnt + i_vid_cnt > 2))
			return CA_E_PARAM;
		hash_action->vlan_cnt = o_vid_cnt + i_vid_cnt;

		if (flow_action->options.masks.outer_tpid) {
			ca_uint8_t tpid_idx;

			if (o_vid_cnt == 0)
				return CA_E_NOT_SUPPORT;

			if (aal_l3fe_pp_top_tpid_get(flow_action->options.outer_tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;

			hash_action->top_tpid_enc = tpid_idx;
		} else {
			hash_action->top_tpid_enc = o_tpid_enc;
		}

		if (flow_action->options.masks.inner_tpid) {
			ca_uint8_t tpid_idx;

			if (i_vid_cnt == 0)
				return CA_E_NOT_SUPPORT;

			if (aal_l3fe_pp_top_tpid_get(flow_action->options.inner_tpid, &tpid_idx) != AAL_E_OK)
				return CA_E_NOT_SUPPORT;

			hash_action->inner_tpid_enc = tpid_idx;
		} else {
			hash_action->inner_tpid_enc = i_tpid_enc;
		}

		if (flow_action->options.masks.outer_dei) {
			if (o_vid_cnt == 0)
				return CA_E_NOT_SUPPORT;
			hash_action->top_dei_sel = 0;
			hash_action->top_dei = flow_action->options.outer_dei;
		} else {
			hash_action->top_dei_sel = 1;
		}

		if (flow_action->options.masks.inner_dei) {
			if (i_vid_cnt == 0)
				return CA_E_NOT_SUPPORT;
			hash_action->inner_dei_sel = 0;
			hash_action->inner_dei = flow_action->options.inner_dei;
		} else {
			hash_action->inner_dei_sel = 1;
		}
	}

	if (flow_action->options.masks.outer_dot1p) {
		hash_action->top_802_1p_sel = 2; /* use FIB result */
		hash_action->top_802_1p = flow_action->options.outer_dot1p;
	}

	if (flow_action->options.masks.inner_dot1p) {
		hash_action->inner_802_1p_sel = 2; /* use FIB result */
		hash_action->inner_802_1p = flow_action->options.inner_dot1p;
	}

	if (flow_action->options.masks.mac_da) {
		if (l3fe_hash_action_indexed_mode == 0) {
			hash_action->mac_da_0 = flow_action->options.mac_da[5];
			hash_action->mac_da_1 = flow_action->options.mac_da[4];
			hash_action->mac_da_2 = flow_action->options.mac_da[3];
			hash_action->mac_da_3 = flow_action->options.mac_da[2];
			hash_action->mac_da_4 = flow_action->options.mac_da[1];
			hash_action->mac_da_5 = flow_action->options.mac_da[0];
			hash_action->mac_da_vld = 1;
		} else {
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			//set mac index table at first
			idx_intf_mac_entry_t mac_entry;
			memset(&mac_entry, 0, sizeof(mac_entry));
			aal_array_reverse_copy(mac_entry.mac_da, flow_action->options.mac_da, 6);
			ret = aal_entry_add(AAL_TABLE_IDX_INTF_MAC, &mac_entry, &rslt_idx);
			if(ret != CA_E_OK) {
				return ret;
			}
			hash_action->mac_da_idx = rslt_idx;
			hash_action->mac_da_idx_vld = 1;
#endif
		}
	}
	
#if defined(CONFIG_ARCH_CORTINA_VENUS)
	if(l3fe_hash_action_indexed_mode == 0)
		hash_action->smac_trans = 0;
	else
		hash_action->smac_trans = 1;
#endif

	if (flow_action->options.masks.mac_sa) {
		if(l3fe_hash_action_indexed_mode == 0)
		{
			hash_action->mac_sa_0 = flow_action->options.mac_sa[5];
			hash_action->mac_sa_1 = flow_action->options.mac_sa[4];
			hash_action->mac_sa_2 = flow_action->options.mac_sa[3];
			hash_action->mac_sa_3 = flow_action->options.mac_sa[2];
			hash_action->mac_sa_4 = flow_action->options.mac_sa[1];
			hash_action->mac_sa_5 = flow_action->options.mac_sa[0];
			hash_action->mac_sa_vld = 1;
		}
		else
		{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			hash_action->smac_trans = 0;

			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = AAL_HL_PROFILE_SMAC_TRANS_OFFLOAD;
#endif
		}
	}

	if (flow_action->options.masks.egress_pppoe_action) {
		if (l3fe_hash_action_indexed_mode == 0) {
			hash_action->pppoe_set = 1;
			if (flow_action->options.pppoe_session_id) {
				hash_action->pppoe_vld = 1;
				hash_action->pppoe_session_id = flow_action->options.pppoe_session_id;
			} else {
				hash_action->pppoe_vld = 0; /* remove PPPoE field */
			}
		}
		else
		{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			hash_action->smac_trans = 0;
#endif
		}
	}

	if (flow_action->options.masks.ip_da) {
		if (l3fe_hash_action_indexed_mode == 0) {
			if ((ip_version == 4) && (flow_action->options.ip_da.afi == CA_IPV4)) {
				hash_action->ip_da_0 = flow_action->options.ip_da.ip_addr.ipv4_addr;
				hash_action->ip_da_l_vld = 1;
			} else if ((ip_version == 6) && (flow_action->options.ip_da.afi == CA_IPV6)) {
				/* Hash action is not long enough to support IPv6 address translation */
				return CA_E_PARAM;
#if 0
				hash_action->ip_da_3 = flow_action->options.ip_da.ip_addr.ipv6_addr[0];
				hash_action->ip_da_2 = flow_action->options.ip_da.ip_addr.ipv6_addr[1];
				hash_action->ip_da_1 = flow_action->options.ip_da.ip_addr.ipv6_addr[2];
				hash_action->ip_da_0 = flow_action->options.ip_da.ip_addr.ipv6_addr[3];
				hash_action->ip_da_h_vld = 1;
				hash_action->ip_da_l_vld = 1;
#endif
			} else {
				return CA_E_PARAM;
			}
		} else {
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			if ((ip_version == 4) && (flow_action->options.ip_da.afi == CA_IPV4)) {
				ret = aal_entry_add(AAL_TABLE_IDX_INTF_IPV4, &flow_action->options.ip_da.ip_addr.ipv4_addr, &rslt_idx);
				if(ret != CA_E_OK) {
					return ret;
				}
				hash_action->ip_idx_vld = 1;
				hash_action->ip_type = 1;
				hash_action->ip_idx = rslt_idx;
			} else if ((ip_version == 6) && (flow_action->options.ip_da.afi == CA_IPV6)) {
				ipv6_addr[3] = flow_action->options.ip_da.ip_addr.ipv6_addr[0];
				ipv6_addr[2] = flow_action->options.ip_da.ip_addr.ipv6_addr[1];
				ipv6_addr[1] = flow_action->options.ip_da.ip_addr.ipv6_addr[2];
				ipv6_addr[0] = flow_action->options.ip_da.ip_addr.ipv6_addr[3];
				ret = aal_entry_add(AAL_TABLE_IDX_INTF_IPV6, ipv6_addr, &rslt_idx);
				if(ret != CA_E_OK) {
					return ret;
				}
				hash_action->ip_idx_vld = 1;
				hash_action->ip_type = 1;
				hash_action->ip_idx = rslt_idx;
				//return CA_E_PARAM;
			} else {
				return CA_E_PARAM;
			}
#endif
		}
	}

	if (flow_action->options.masks.ip_sa) {
		if (l3fe_hash_action_indexed_mode == 0) {
			if ((ip_version == 4) && (flow_action->options.ip_sa.afi == CA_IPV4)) {
				hash_action->ip_sa_0 = flow_action->options.ip_sa.ip_addr.ipv4_addr;
				hash_action->ip_sa_l_vld = 1;
			} else if ((ip_version == 6) && (flow_action->options.ip_sa.afi == CA_IPV6)) {
				/* Hash action is not long enough to support IPv6 address translation */
				return CA_E_PARAM;
#if 0
			hash_action->ip_sa_3 = flow_action->options.ip_sa.ip_addr.ipv6_addr[0];
			hash_action->ip_sa_2 = flow_action->options.ip_sa.ip_addr.ipv6_addr[1];
			hash_action->ip_sa_1 = flow_action->options.ip_sa.ip_addr.ipv6_addr[2];
			hash_action->ip_sa_0 = flow_action->options.ip_sa.ip_addr.ipv6_addr[3];
			hash_action->ip_sa_h_vld = 1;
			hash_action->ip_sa_l_vld = 1;
#endif
			} else {
				return CA_E_PARAM;
			}
		} else {
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			if ((ip_version == 4) && (flow_action->options.ip_sa.afi == CA_IPV4)) {
				ret = aal_entry_add(AAL_TABLE_IDX_INTF_IPV4, &flow_action->options.ip_sa.ip_addr.ipv4_addr, &rslt_idx);
				if(ret != CA_E_OK) {
					return ret;
				}
				hash_action->ip_idx_vld = 1;
				hash_action->ip_type = 0;
				hash_action->ip_idx = rslt_idx;
			} else if ((ip_version == 6) && (flow_action->options.ip_sa.afi == CA_IPV6)) {
				ipv6_addr[3] = flow_action->options.ip_sa.ip_addr.ipv6_addr[0];
				ipv6_addr[2] = flow_action->options.ip_sa.ip_addr.ipv6_addr[1];
				ipv6_addr[1] = flow_action->options.ip_sa.ip_addr.ipv6_addr[2];
				ipv6_addr[0] = flow_action->options.ip_sa.ip_addr.ipv6_addr[3];
				ret = aal_entry_add(AAL_TABLE_IDX_INTF_IPV6, ipv6_addr, &rslt_idx);
				if(ret != CA_E_OK) {
					return ret;
				}
				hash_action->ip_idx_vld = 1;
				hash_action->ip_type = 0;
				hash_action->ip_idx = rslt_idx;
				//return CA_E_PARAM;
			} else {
				return CA_E_PARAM;
			}
#endif
		}
	}

	if (flow_action->options.masks.src_l4_port) {
		hash_action->l4_sp = flow_action->options.src_l4_port;
		hash_action->l4_sp_vld = 1;
	}

	if (flow_action->options.masks.dst_l4_port) {
		hash_action->l4_dp = flow_action->options.dst_l4_port;
		hash_action->l4_dp_vld = 1;
	}

	// group 12~15
	if (flow_action->options.masks.sw_id) {
#if defined(CONFIG_LUNA_G3_SERIES)
		if (l3fe_hash_action_indexed_mode == 0) {

			hash_action->l3_egress_if_vld = 1;

			hash_action->mdata_w_vld_0 = 0xF;
			hash_action->mdata_w_0 = flow_action->options.sw_id[0] & 0xFF;
		} else {
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			hash_action->mdata_byte_idx = 0;
			hash_action->mdata_byte = flow_action->options.sw_id[0];
			hash_action->mdata_byte_vld = 1;
#endif
		}

		// interface index
		hash_action->mdata_w_vld_1 = 0xF;
		hash_action->mdata_w_1 = flow_action->options.sw_id[1];
#else
		/* mdata[55:32], 24 bits */
		hash_action->mdata_w_vld_3 = 0x3;
		hash_action->mdata_w_3 = flow_action->options.sw_id[1] & 0xFF;
		hash_action->mdata_w_vld_2 = 0xF;
		hash_action->mdata_w_2 = flow_action->options.sw_id[0];
#endif
	}

	if (flow_action->options.masks.sw_shaper_id) {
		if (flow_action->forward == CA_CLASSIFIER_FORWARD_PORT)
			return CA_E_PARAM;
		/* sw_shaper_id = queue id, LDPID 0x20-0x3f (each with 8 VoQs, 256 queues total) */
		if (flow_action->options.sw_shaper_id >= 256)
			return CA_E_PARAM;
		hash_action->mc = 0;
		hash_action->mcgid = 0x20 + flow_action->options.sw_shaper_id / 8;
		hash_action->dpid_pri = 1;
		hash_action->permit = 1;
		hash_action->dpid_vld = 1;
		hash_action->cos = flow_action->options.sw_shaper_id % 8;
		hash_action->cos_sel = 1;

		hash_action->t2_ctrl_vld = 1;
		hash_action->t2_ctrl = 0xf;
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
		hash_action->t5_ctrl_vld = 1;
		hash_action->t5_ctrl = 0xf;

		hash_action->deepq = 1;
	}

	switch (flow_action->forward) {
	case CA_CLASSIFIER_FORWARD_DENY:
		hash_action->dpid_vld = 1;
		hash_action->permit = 0;
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
		hash_action->t5_ctrl_vld = 1;
		hash_action->t5_ctrl = 0xf;
		hash_action->stage3_ctrl_vld = 1;
		hash_action->stage3_ctrl = 1;
		break;
	case CA_CLASSIFIER_FORWARD_FE:
		if (flow_action->dest.fe == 1) { /* lookup route/nexthop */
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0x0;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0x0;
			hash_action->t5_ctrl_vld = 1;
			hash_action->t5_ctrl = 0xf;
			hash_action->stage3_ctrl_vld = 0; /* don't care */
			hash_action->stage3_ctrl = 1;
		} else if (flow_action->dest.fe == 2) { /* lookup mac_da */
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0xf;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xf;
			hash_action->t5_ctrl_vld = 1;
			hash_action->t5_ctrl = 0x0;
			hash_action->stage3_ctrl_vld = 0; /* don't care */
			hash_action->stage3_ctrl = 1;
		} else {
			return CA_E_PARAM;
		}
		break;
	case CA_CLASSIFIER_FORWARD_INTERFACE:
		if(l3fe_hash_action_indexed_mode == 0)
			hash_action->l3_egress_if_vld = 1;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
		else
			hash_action->l3_egress_if_vld1 = 1;
#endif
		hash_action->mdata_w_1 = flow_action->dest.intf & 0xff;
		hash_action->mdata_w_vld_1 = 0x3;
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
		hash_action->stage3_ctrl_vld = 1;
		hash_action->stage3_ctrl = 1;
		break;
	case CA_CLASSIFIER_FORWARD_PORT:
		hash_action->dpid_vld = 1;
		hash_action->permit = 1;
		hash_action->mcgid = flow_action->dest.port & 0x3f;
		hash_action->deepq = 1; /* must be 1, otherwise packets enter L2FE */
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
#if defined(CONFIG_LUNA_G3_SERIES)

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
		if((hash_action->pppoe_set == 1) && (hash_action->pppoe_vld == 0)) {
			// notify hashlite to remove pppoe tag
			hash_action->mdata_w_0 |= 1<<8;
			
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xD;	// profile 13
		}else{
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xf;
		}
#elif defined(CONFIG_ARCH_CORTINA_VENUS)
		hash_action->mrr_vld = TRUE;
		hash_action->mrr_en = (l3fe_egress_mirrored_port == (flow_action->dest.port & 0x3f)) ? TRUE : FALSE;

		hash_action->t2_ctrl_vld = ((flow_action->options.gemId_mapping_mode & 0x8) >> 3);			//FIB.t2_ctrl_vld reused as:			Keep_hdra_ldpid;
		hash_action->pop_l3_en = ((flow_action->options.gemId_mapping_mode & 0x4) >> 2);			//FIB.pop_l3_en reused as:			Keep_hdra_cos;
		hash_action->pop_l3_chk_ecn_en = ((flow_action->options.gemId_mapping_mode & 0x2) >> 1);	//FIB.pop_l3_chk_ecn_en reused as:	Keep_hdra_pol_id;
		hash_action->pop_l3_vld = ((flow_action->options.gemId_mapping_mode & 0x1) >> 0);			//FIB.pop_l3_vld reused as:			hdra_polid_ldpid_upd_sel(upd_sel);
		
		if (flow_action->options.masks.gemId_mapping_mode) {
			if(flow_action->options.gemId_mapping_mode == 0x1)
			{	// CN2 mode[1]

				hash_action->dpid_vld = 1;
				if((flow_action->options.tcont_id - l3qm_ldpid_to_pon_port_map_offset) <=7)		//  to wan & but ldpid is 0x24~0x27 => dq
					hash_action->deepq = 1;
				else																			// to wan & ldpid is 0x28~0x2F => !dq
					hash_action->deepq = 0;
				hash_action->dpid_pri = 1;
				hash_action->mc = 1;
				hash_action->mcgid = flow_action->options.gem_id; //gem id
				hash_action->t2_ctrl =  (flow_action->options.tcont_id - l3qm_ldpid_to_pon_port_map_offset) & 0xF; // tcont id. hdra.ldpid[5:0] <= CSR_ldpid_base[5:0] + hdri.t2_ctrl[3:0];
			}
		}
			
		if (l3fe_hash_action_indexed_mode == 1) 
		{
			// need l3_if FIB to apply SMAC and PPPoE action, always enabled
			hash_action->l3_egress_if_vld1 = 1;
		}

#endif

#else
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
#endif
		hash_action->t5_ctrl_vld = 1;
		hash_action->t5_ctrl = 0xf;
		hash_action->stage3_ctrl_vld = 1;
		hash_action->stage3_ctrl = 1;
		break;
	case CA_CLASSIFIER_FORWARD_TUNNEL:
		if (get_nexthop_id_by_tunnel_id(flow_action->dest.tunnel_id, &nexthop_id) != CA_E_OK) {
			ca_printf(KERN_INFO "%s: Fail to find nexthop_id by tunnel_id %d.\n", __func__, flow_action->dest.tunnel_id);
			return CA_E_PARAM;
		}

		hash_action->l3_egress_if_vld = 1;
		hash_action->mdata_w_0 = nexthop_id;
		hash_action->mdata_w_vld_0 = 0x7;
		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xe; /* directly use the HashLite entry index */
		hash_action->stage3_ctrl_vld = 1;
		hash_action->stage3_ctrl = 1;
		break;
	default:
		return CA_E_NOT_SUPPORT;
	}

	return CA_E_OK;
}

/***************************************************************************
 * internal functions - Flow Types
 ***************************************************************************/

#define NAT_PROFILE_MAX		2

struct __key_mask {
	bool valid;
	ca_uint8_t prio;
	ca_flow_key_mask_t mask;
	unsigned int hash_mask_idx;
	ca_uint8_t tuple_idx[NAT_PROFILE_MAX];
};

struct __key_mask flow_type_array[CA_FLOW_TYPE_MAX];

static void print_flow_key_mask(ca_flow_key_mask_t *mask)
{
	ca_printf(KERN_INFO "  l2_keys %u\n", mask->l2_keys);
	ca_printf(KERN_INFO "    o_lspid %u\n", mask->l2_mask.o_lspid);
	ca_printf(KERN_INFO "    lspid %u\n", mask->l2_mask.lspid);
	ca_printf(KERN_INFO "    mac_sa %u\n", mask->l2_mask.mac_sa);
	ca_printf(KERN_INFO "    mac_da %u\n", mask->l2_mask.mac_da);
	ca_printf(KERN_INFO "    outer_vlan tpid %u\n", mask->l2_mask.outer_vlan.tpid);
	ca_printf(KERN_INFO "    outer_vlan vid %u\n", mask->l2_mask.outer_vlan.vid);
	ca_printf(KERN_INFO "    outer_vlan dei %u\n", mask->l2_mask.outer_vlan.dei);
	ca_printf(KERN_INFO "    outer_vlan pri %u\n", mask->l2_mask.outer_vlan.pri);
	ca_printf(KERN_INFO "    inner_vlan tpid %u\n", mask->l2_mask.inner_vlan.tpid);
	ca_printf(KERN_INFO "    inner_vlan vid %u\n", mask->l2_mask.inner_vlan.vid);
	ca_printf(KERN_INFO "    inner_vlan dei %u\n", mask->l2_mask.inner_vlan.dei);
	ca_printf(KERN_INFO "    inner_vlan pri %u\n", mask->l2_mask.inner_vlan.pri);

	ca_printf(KERN_INFO "  l3_keys %d\n", mask->l3_keys);
	ca_printf(KERN_INFO "    ip_valid %u\n", mask->l3_mask.ip_valid);
	ca_printf(KERN_INFO "    ip_version %u\n", mask->l3_mask.ip_version);
	ca_printf(KERN_INFO "    ip_protocol %u\n", mask->l3_mask.ip_protocol);
	ca_printf(KERN_INFO "    dscp %u\n", mask->l3_mask.dscp);
	ca_printf(KERN_INFO "    ecn %u\n", mask->l3_mask.ecn);
	ca_printf(KERN_INFO "    ip_sa %u\n", mask->l3_mask.ip_sa);
	ca_printf(KERN_INFO "    ip_da %u\n", mask->l3_mask.ip_da);
	ca_printf(KERN_INFO "    fragment %u\n", mask->l3_mask.fragment);
	ca_printf(KERN_INFO "    icmp_type %u\n", mask->l3_mask.icmp_type);
	ca_printf(KERN_INFO "    igmp_type %u\n", mask->l3_mask.igmp_type);
	ca_printf(KERN_INFO "    hbh_header %u\n", mask->l3_mask.hbh_header);
	ca_printf(KERN_INFO "    routing_header %u\n", mask->l3_mask.routing_header);
	ca_printf(KERN_INFO "    dest_opt_header %u\n", mask->l3_mask.dest_opt_header);
	ca_printf(KERN_INFO "    ip_ttl %u\n", mask->l3_mask.ip_ttl);

	ca_printf(KERN_INFO "  l4_keys %d\n", mask->l4_keys);
	ca_printf(KERN_INFO "    src_l4_port %u\n", mask->l4_mask.src_l4_port);
	ca_printf(KERN_INFO "    dst_l4_port %u\n", mask->l4_mask.dst_l4_port);
	ca_printf(KERN_INFO "    tcp_flags 0x%03x\n", mask->l4_mask.tcp_flags); /* {N,C,E,U,A,P,R,S,F} */

	ca_printf(KERN_INFO "  sw_id %d\n", mask->sw_id);
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
	ca_printf(KERN_INFO "  flow_id %d\n", mask->flow_id);
#endif
}

void flow_type_dump(void)
{
	int i, j;

	ca_printf(KERN_INFO "DUMP added flow key types:\n");
	for (i = 0; i < CA_FLOW_TYPE_MAX; i++) {
		if (flow_type_array[i].valid == false) {
			ca_printf(KERN_INFO "Flow Type %d, N/A\n", i);
		} else {
			ca_printf(KERN_INFO "Flow Type %d, prio %u, hash_mask_idx %u\n",
				i, flow_type_array[i].prio, flow_type_array[i].hash_mask_idx);

			for (j = 0; j < NAT_PROFILE_MAX; j++)
				ca_printf(KERN_INFO "  tuple_idx@profile#%d %u\n", j, flow_type_array[i].tuple_idx[j]);

			print_flow_key_mask(&flow_type_array[i].mask);
		}
		ca_printf(KERN_INFO "-------------------------------------------------------------------------\n");
	}
}

/***************************************************************************
 * internal functions - Flows
 ***************************************************************************/

struct flow_node {
	ca_flow_t		flow_entry;
	bool			is_static;
	struct hlist_node	list;
};

static atomic_t num_hash_entry = ATOMIC_INIT(0);
static atomic_t num_overflow_hash_entry = ATOMIC_INIT(0);

/* flow_table is a hash table for storing flow entry.
 * No matter how L3FE Hash Table is configured, the flow_table_size is assumed to contain all.
 * The hash function is: entry_index_in_flow_table = (flow_index & CA_L3_NAT_HASH_IDX_MASK) / 10.
 * Therefore, each entry in flow_table contains at most 10 entries in Hash Table and 1 entry in Overflow Table.
 * flow_index is same as the index in L3FE Hash Table, or the masked index in L3FE Overflow Table.
 */
static struct hlist_head *flow_table = NULL;
static int flow_table_size = (HASH_HASH_TBL_ENTRY_MAX + HASH_OVERFLOW_TBL_ENTRY_MAX) / 10 + 1;
static DEFINE_RWLOCK(flow_table_lock);

static inline unsigned int get_idx_hash(ca_uint32_t flow_index)
{
	return (flow_index & CA_L3_NAT_HASH_IDX_MASK) / 10;
}

static void flow_table_hlist_add(struct flow_node *flow)
{
	if (flow == NULL) return;
	hlist_add_head(&flow->list, &flow_table[get_idx_hash(flow->flow_entry.index)]);
}

static void flow_table_hlist_del(struct flow_node *flow)
{
	if (flow == NULL) return;
	hlist_del_init(&flow->list);
}

static bool flow_table_hlist_empty(void)
{
	int i;

	for (i = 0; i < flow_table_size; i++) {
		if (hlist_empty(&flow_table[i]) == false)
			return false;
	}

	return true;
}

#if defined(CONFIG_LUNA_G3_SERIES)
void print_flow(ca_flow_t *flow, bool is_static, bool get_age_from_hw)
#else
static void print_flow(ca_flow_t *flow, bool is_static, bool get_age_from_hw)
#endif
{
	ca_flow_key_type_t key_type;
	unsigned int age = 0, time = 0;

	if (flow == NULL) return;

	key_type = flow->key_type;

	if (get_age_from_hw) {
		if (flow->index & CA_L3_NAT_ID_OVERFLOW_FLAG) {
			aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, flow->index & CA_L3_NAT_HASH_IDX_MASK, &age, &time);
		} else {
			aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, flow->index, &age, &time);
		}
	} else {
		if (is_static) {
			age = HASH_AGING_STATIC;
			time = 0;
		} else {
			time = flow->aging_time;
			age = (time == 0) ? HASH_AGING_INVALID : HASH_AGING_START;
		}
	}

	ca_printf(KERN_INFO "idx=%u (0x%x), key_type=%d, aging_timer=%u [%s]\n",
			flow->index, flow->index, key_type, time,
			(age == HASH_AGING_STATIC) ? "STATIC" : ((age == HASH_AGING_INVALID) ? "EXPIRED" : "AGING"));

	if (flow_type_array[key_type].mask.l2_keys) {
		ca_printf(KERN_INFO "  [Key: L2]\n");
		if (flow_type_array[key_type].mask.l2_mask.o_lspid)
			ca_printf(KERN_INFO "    o_lspid=0x%02x\n", flow->key.l2_key.o_lspid);
		if (flow_type_array[key_type].mask.l2_mask.lspid)
			ca_printf(KERN_INFO "    lspid=0x%02x\n", flow->key.l2_key.lspid);
		if (flow_type_array[key_type].mask.l2_mask.mac_sa)
			ca_printf(KERN_INFO "    mac_sa=%pM\n", flow->key.l2_key.mac_sa);
		if (flow_type_array[key_type].mask.l2_mask.mac_da)
			ca_printf(KERN_INFO "    mac_da=%pM\n", flow->key.l2_key.mac_da);

		if (flow_type_array[key_type].mask.l2_mask.outer_vlan.tpid)
			ca_printf(KERN_INFO "    outer tpid=0x%04x\n", flow->key.l2_key.outer_vlan.tpid);
		if (flow_type_array[key_type].mask.l2_mask.outer_vlan.pri)
			ca_printf(KERN_INFO "    outer pri=%u\n", flow->key.l2_key.outer_vlan.pri);
		if (flow_type_array[key_type].mask.l2_mask.outer_vlan.dei)
			ca_printf(KERN_INFO "    outer dei=%u\n", flow->key.l2_key.outer_vlan.dei);
		if (flow_type_array[key_type].mask.l2_mask.outer_vlan.vid)
			ca_printf(KERN_INFO "    outer vid=%u\n", flow->key.l2_key.outer_vlan.vid);

		if (flow_type_array[key_type].mask.l2_mask.inner_vlan.tpid)
			ca_printf(KERN_INFO "    inner tpid=0x%04x\n", flow->key.l2_key.inner_vlan.tpid);
		if (flow_type_array[key_type].mask.l2_mask.inner_vlan.pri)
			ca_printf(KERN_INFO "    inner pri=%u\n", flow->key.l2_key.inner_vlan.pri);
		if (flow_type_array[key_type].mask.l2_mask.inner_vlan.dei)
			ca_printf(KERN_INFO "    inner dei=%u\n", flow->key.l2_key.inner_vlan.dei);
		if (flow_type_array[key_type].mask.l2_mask.inner_vlan.vid)
			ca_printf(KERN_INFO "    inner vid=%u\n", flow->key.l2_key.inner_vlan.vid);
	}

	if (flow_type_array[key_type].mask.l3_keys) {
		ca_printf(KERN_INFO "  [Key: L3]\n");
		if (flow_type_array[key_type].mask.l3_mask.ip_valid)
			ca_printf(KERN_INFO "    ip_valid=%u\n", flow->key.l3_key.ip_valid);
		if (flow_type_array[key_type].mask.l3_mask.ip_version)
			ca_printf(KERN_INFO "    ip_version=%u\n", flow->key.l3_key.ip_version);
		if (flow_type_array[key_type].mask.l3_mask.ip_protocol)
			ca_printf(KERN_INFO "    ip_protocol=0x%02x\n", flow->key.l3_key.ip_protocol);
		if (flow_type_array[key_type].mask.l3_mask.dscp)
			ca_printf(KERN_INFO "    dscp=0x%02x\n", flow->key.l3_key.dscp);
		if (flow_type_array[key_type].mask.l3_mask.ecn)
			ca_printf(KERN_INFO "    ecn=0x%02x\n", flow->key.l3_key.ecn);
		if (flow_type_array[key_type].mask.l3_mask.ip_sa)
			ca_printf(KERN_INFO "    ip_sa=0x%08x\n", flow->key.l3_key.ip_sa.ip_addr.ipv6_addr[0]);
		if (flow_type_array[key_type].mask.l3_mask.ip_da)
			ca_printf(KERN_INFO "    ip_da=0x%08x\n", flow->key.l3_key.ip_da.ip_addr.ipv6_addr[0]);
		if (flow_type_array[key_type].mask.l3_mask.fragment)
			ca_printf(KERN_INFO "    fragment=%u\n", flow->key.l3_key.fragment);
		if (flow_type_array[key_type].mask.l3_mask.icmp_type)
			ca_printf(KERN_INFO "    icmp_type=0x%02x\n", flow->key.l3_key.icmp_type);
		if (flow_type_array[key_type].mask.l3_mask.igmp_type)
			ca_printf(KERN_INFO "    igmp_type=0x%02x\n", flow->key.l3_key.igmp_type);
		if (flow_type_array[key_type].mask.l3_mask.hbh_header)
			ca_printf(KERN_INFO "    hbh_header=0x%02x\n", flow->key.l3_key.hbh_header);
		if (flow_type_array[key_type].mask.l3_mask.routing_header)
			ca_printf(KERN_INFO "    routing_header=0x%02x\n", flow->key.l3_key.routing_header);
		if (flow_type_array[key_type].mask.l3_mask.dest_opt_header)
			ca_printf(KERN_INFO "    dest_opt_header=0x%02x\n", flow->key.l3_key.dest_opt_header);
		if (flow_type_array[key_type].mask.l3_mask.ip_ttl)
			ca_printf(KERN_INFO "    ip_ttl=%u\n", flow->key.l3_key.ip_ttl);
	}

	if (flow_type_array[key_type].mask.l4_keys) {
		ca_printf(KERN_INFO "  [Key: L4]\n");
		if (flow_type_array[key_type].mask.l4_mask.src_l4_port)
			ca_printf(KERN_INFO "    src_l4_port=%u\n", flow->key.l4_key.src_l4_port);
		if (flow_type_array[key_type].mask.l4_mask.dst_l4_port)
			ca_printf(KERN_INFO "    dst_l4_port=%u\n", flow->key.l4_key.dst_l4_port);
		if (flow_type_array[key_type].mask.l4_mask.tcp_flags)
			ca_printf(KERN_INFO "    tcp_flags=%u\n", flow->key.l4_key.tcp_flags);
	}

	if (flow_type_array[key_type].mask.sw_id) {
		ca_printf(KERN_INFO "  [Key: SW_ID]\n");
		ca_printf(KERN_INFO "    sw_id[3:0]=%04x-%04x-%04x-%04x\n",
				flow->key.sw_id[3], flow->key.sw_id[2],
				flow->key.sw_id[1], flow->key.sw_id[0]);
	}

	ca_printf(KERN_INFO "  [Action: options]\n");
	if (flow->actions.options.masks.flow_id)
		ca_printf(KERN_INFO "    flow_id=0x%04x\n", flow->actions.options.flow_id);
#if defined(CONFIG_LUNA_G3_SERIES)
	if (flow->actions.options.masks.flow_id2)
		ca_printf(KERN_INFO "    flow_id2=0x%04x\n", flow->actions.options.flow_id2);
	if (flow->actions.options.masks.flow_id3)
		ca_printf(KERN_INFO "    flow_id3=0x%04x\n", flow->actions.options.flow_id3);
#endif
	if (flow->actions.options.masks.cos)
		ca_printf(KERN_INFO "    cos=%u\n", flow->actions.options.cos);
	if (flow->actions.options.masks.dscp)
		ca_printf(KERN_INFO "    dscp=%u\n", flow->actions.options.dscp);

	if (flow->actions.options.masks.outer_vlan_act) {
		ca_printf(KERN_INFO "    outer vlan action=");
		switch (flow->actions.options.outer_vlan_act) {
		case CA_FLOW_VLAN_ACTION_PUSH: printk("PUSH\n"); break;
		case CA_FLOW_VLAN_ACTION_POP: printk("POP\n"); break;
		case CA_FLOW_VLAN_ACTION_SWAP: printk("SWAP\n"); break;
		case CA_FLOW_VLAN_ACTION_SET: printk("SET\n"); break;
		default: printk("*** UNKNOWN ***\n"); break;
		}
		ca_printf(KERN_INFO "    outer vid=%u\n", flow->actions.options.outer_vid);
	}
	if (flow->actions.options.masks.outer_dot1p)
		ca_printf(KERN_INFO "    outer dot1p=%u\n", flow->actions.options.outer_dot1p);
	if (flow->actions.options.masks.outer_tpid)
		ca_printf(KERN_INFO "    outer tpid=0x%04x\n", flow->actions.options.outer_tpid);
	if (flow->actions.options.masks.outer_dei)
		ca_printf(KERN_INFO "    outer dei=%u\n", flow->actions.options.outer_dei);

	if (flow->actions.options.masks.inner_vlan_act) {
		ca_printf(KERN_INFO "    inner vlan action=");
		switch (flow->actions.options.inner_vlan_act) {
		case CA_FLOW_VLAN_ACTION_PUSH: printk("PUSH\n"); break;
		case CA_FLOW_VLAN_ACTION_POP: printk("POP\n"); break;
		case CA_FLOW_VLAN_ACTION_SWAP: printk("SWAP\n"); break;
		case CA_FLOW_VLAN_ACTION_SET: printk("SET\n"); break;
		default: printk("*** UNKNOWN ***\n"); break;
		}
		ca_printf(KERN_INFO "    inner vid=%u\n", flow->actions.options.inner_vid);
	}
	if (flow->actions.options.masks.inner_dot1p)
		ca_printf(KERN_INFO "    inner dot1p=%u\n", flow->actions.options.inner_dot1p);
	if (flow->actions.options.masks.inner_tpid)
		ca_printf(KERN_INFO "    inner tpid=0x%04x\n", flow->actions.options.inner_tpid);
	if (flow->actions.options.masks.inner_dei)
		ca_printf(KERN_INFO "    inner dei=%u\n", flow->actions.options.inner_dei);

	if (flow->actions.options.masks.mac_da)
		ca_printf(KERN_INFO "    mac_da=%pM\n", flow->actions.options.mac_da);
	if (flow->actions.options.masks.mac_sa)
		ca_printf(KERN_INFO "    mac_sa=%pM\n", flow->actions.options.mac_sa);
	if (flow->actions.options.masks.egress_pppoe_action)
		ca_printf(KERN_INFO "    pppoe_session_id=%u\n", flow->actions.options.pppoe_session_id);

	if (flow->actions.options.masks.ip_da) {
		if (flow->actions.options.ip_da.afi == CA_IPV4) {
			ca_printf(KERN_INFO "    ip_da=0x%08x\n", flow->actions.options.ip_da.ip_addr.ipv4_addr);
		} else {
			ca_printf(KERN_INFO "    ip_da=%08x-%08x-%08x-%08x\n",
				flow->actions.options.ip_da.ip_addr.ipv6_addr[0], flow->actions.options.ip_da.ip_addr.ipv6_addr[1],
				flow->actions.options.ip_da.ip_addr.ipv6_addr[2], flow->actions.options.ip_da.ip_addr.ipv6_addr[3]);
		}
	}
	if (flow->actions.options.masks.ip_sa) {
		if (flow->actions.options.ip_sa.afi == CA_IPV4) {
			ca_printf(KERN_INFO "    ip_sa=0x%08x\n", flow->actions.options.ip_sa.ip_addr.ipv4_addr);
		} else {
			ca_printf(KERN_INFO "    ip_sa=%08x-%08x-%08x-%08x\n",
				flow->actions.options.ip_sa.ip_addr.ipv6_addr[0], flow->actions.options.ip_sa.ip_addr.ipv6_addr[1],
				flow->actions.options.ip_sa.ip_addr.ipv6_addr[2], flow->actions.options.ip_sa.ip_addr.ipv6_addr[3]);
		}
	}

	if (flow->actions.options.masks.src_l4_port)
		ca_printf(KERN_INFO "    src_l4_port=%u\n", flow->actions.options.src_l4_port);
	if (flow->actions.options.masks.dst_l4_port)
		ca_printf(KERN_INFO "    dst_l4_port=%u\n", flow->actions.options.dst_l4_port);
	if (flow->actions.options.masks.sw_id) {
		ca_printf(KERN_INFO "    sw_id[3:0]=%04x-%04x-%04x-%04x\n",
			flow->actions.options.sw_id[3], flow->actions.options.sw_id[2], flow->actions.options.sw_id[1], flow->actions.options.sw_id[0]);
	}

	ca_printf(KERN_INFO "  [Action: destination]\n");
	switch (flow->actions.forward) {
	case CA_CLASSIFIER_FORWARD_DENY:
		ca_printf(KERN_INFO "    forward=DROP\n");
		break;
	case CA_CLASSIFIER_FORWARD_FE:
		ca_printf(KERN_INFO "    forward=FE, dest.fe=%d\n", flow->actions.dest.fe);
		break;
	case CA_CLASSIFIER_FORWARD_INTERFACE:
		ca_printf(KERN_INFO "    forward=INTERFACE, dest.intf_id=%u\n", flow->actions.dest.intf);
		break;
	case CA_CLASSIFIER_FORWARD_PORT:
		ca_printf(KERN_INFO "    forward=PORT, dest.port_id=0x%x\n", flow->actions.dest.port);
		break;
	case CA_CLASSIFIER_FORWARD_TUNNEL:
		ca_printf(KERN_INFO "    forward=TUNNEL, dest.tunnel_id=0x%x\n", flow->actions.dest.tunnel_id);
		break;
	default:
		ca_printf(KERN_INFO "    forward=***UNKNOWN***\n");
		break;
	}
}

void flow_table_dump(void)
{
	struct flow_node *iter;
	int i_flow;

	ca_printf(KERN_INFO "DUMP flow_table:\n");
	read_lock(&flow_table_lock);
	for (i_flow = 0; i_flow < flow_table_size; i_flow++) {
		hlist_for_each_entry(iter, &flow_table[i_flow], list) {
			print_flow(&iter->flow_entry, iter->is_static, true);
			ca_printf(KERN_INFO "-------------------------------------------------------------------------\n");
#if defined(CONFIG_LUNA_G3_SERIES)
			//do nothing
#else
			cond_resched();
#endif

		}
	}
	read_unlock(&flow_table_lock);

	ca_printf(KERN_INFO "Number of flow_entry = %d\n", atomic_read(&num_hash_entry));
	ca_printf(KERN_INFO "Number of overflow flow_entry = %d\n", atomic_read(&num_overflow_hash_entry));
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_flow_egress_mirrored_port_set(
	CA_IN	ca_boolean_t valid,
	CA_IN	ca_uint8_t ldpid)
{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
	if(!valid)
		l3fe_egress_mirrored_port = AAL_LPORT_INVALID;
	else
		l3fe_egress_mirrored_port = ldpid;
#endif
	
	return CA_E_OK;
}
ca_status_t ca_flow_key_profile_mapping_set (
	CA_IN	ca_device_id_t 			device_id,
	CA_IN	ca_flow_key_type_t		key_type,
	CA_IN	ca_uint32_t 				profile_id)
{
	
	if ((key_type < CA_FLOW_TYPE_0) || (key_type >= CA_FLOW_TYPE_MAX))
		return CA_E_PARAM;

	if ((profile_id < HASH_PROFILE_0) || (profile_id >= HASH_PROFILE_MAX))
		return CA_E_PARAM;

	flow_profile_map[key_type] = profile_id;

	ca_printf("main hash keytype[%d] -> profile[%d]\n", key_type, flow_profile_map[key_type]);
	
	return CA_E_OK;
}
#endif

/***************************************************************************
 * Cortina API
 ***************************************************************************/

ca_status_t ca_hash_flow_key_type_add (
	CA_IN	ca_device_id_t 			device_id,
	CA_IN	ca_flow_key_type_config_t	*key_type_config)
{
	ca_status_t ret;
	aal_hash_mask_t hash_mask;
	unsigned int hash_mask_idx = 0;
	ca_uint8_t tuple_idx[NAT_PROFILE_MAX];

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (key_type_config == NULL)
		return CA_E_PARAM;

	if ((key_type_config->key_type < CA_FLOW_TYPE_0) || (key_type_config->key_type >= CA_FLOW_TYPE_MAX))
		return CA_E_PARAM;

	if (key_type_config->prio > 7)
		return CA_E_PARAM;

	if (flow_type_array[key_type_config->key_type].valid)
		return CA_E_EXISTS;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf("%s: Dump key_type_config:\n", __func__);
		ca_printf("  key_type %d\n", key_type_config->key_type);
		ca_printf("  prio %u\n", key_type_config->prio);
		print_flow_key_mask(&key_type_config->key_mask);
	}

	ret = generate_hash_mask(&key_type_config->key_mask, &hash_mask);
	if (ret != CA_E_OK)
		return ret;

	ret = aal_entry_add(AAL_TABLE_HASH_MASK_TBL, &hash_mask, &hash_mask_idx);
	if ((ret != AAL_E_OK) && (ret != AAL_E_EXIST))
		return CA_E_ERROR;

#if defined(CONFIG_LUNA_G3_SERIES)
	ret = aal_hash_profile_tuple_add(flow_profile_map[key_type_config->key_type], TUPLE_TYPE_FWD, key_type_config->prio, hash_mask_idx, &tuple_idx[0]);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	
#else
	ret = aal_hash_profile_tuple_add(0, TUPLE_TYPE_FWD, key_type_config->prio, hash_mask_idx, &tuple_idx[0]);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	ret = aal_hash_profile_tuple_add(1, TUPLE_TYPE_FWD, key_type_config->prio, hash_mask_idx, &tuple_idx[1]);
	if (ret != AAL_E_OK) {
		aal_hash_profile_tuple_del(0, flow_type_array[key_type_config->key_type].tuple_idx[0]);
		return CA_E_ERROR;
	}
#endif	

	flow_type_array[key_type_config->key_type].valid = true;
	flow_type_array[key_type_config->key_type].prio = key_type_config->prio;
	flow_type_array[key_type_config->key_type].mask = key_type_config->key_mask;
	flow_type_array[key_type_config->key_type].hash_mask_idx = hash_mask_idx;
	flow_type_array[key_type_config->key_type].tuple_idx[0] = tuple_idx[0];
	flow_type_array[key_type_config->key_type].tuple_idx[1] = tuple_idx[1];

	return CA_E_OK;
}

ca_status_t ca_hash_flow_key_type_get (
	CA_IN	ca_device_id_t 			device_id,
	CA_OUT	ca_flow_key_type_config_t	*key_type_config)
{
	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (key_type_config == NULL)
		return CA_E_PARAM;

	if (flow_type_array[key_type_config->key_type].valid == false)
		return CA_E_EMPTY;

	key_type_config->prio = flow_type_array[key_type_config->key_type].prio;
	key_type_config->key_mask = flow_type_array[key_type_config->key_type].mask;

	return CA_E_OK;
}

ca_status_t ca_hash_flow_key_type_delete (
	CA_IN	ca_device_id_t			device_id,
	CA_IN	ca_flow_key_type_t		key_type)
{
	ca_status_t ret;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if ((key_type < CA_FLOW_TYPE_0) || (key_type > CA_FLOW_TYPE_7))
		return CA_E_PARAM;

	if (flow_type_array[key_type].valid == false)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: key_type=%d\n", __func__, key_type);
	}

#if defined(CONFIG_LUNA_G3_SERIES)

	ret = aal_hash_profile_tuple_del(flow_profile_map[key_type], flow_type_array[key_type].tuple_idx[0]);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;
#else
	ret = aal_hash_profile_tuple_del(0, flow_type_array[key_type].tuple_idx[0]);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	ret = aal_hash_profile_tuple_del(1, flow_type_array[key_type].tuple_idx[1]);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;
#endif

	ret = aal_entry_del(AAL_TABLE_HASH_MASK_TBL, flow_type_array[key_type].hash_mask_idx);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	flow_type_array[key_type].valid = false;

	return CA_E_OK;
}

ca_status_t ca_hash_flow_aging_time_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_IN	ca_uint32_t		aging_time)
{
	int ret;
	struct flow_node *iter;
	struct hlist_node *p;
	bool foundNode = false;
	unsigned int max_time; /* global aging time, unit is second */
	unsigned int age;
	bool is_static; /* updated status 'is_static' after aging_time is set */

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: index=%u, aging_time=%u\n", __func__, index, aging_time);
	}

	/* search the flow by index */
	read_lock(&flow_table_lock);
	hlist_for_each(p, &flow_table[get_idx_hash(index)]) {
		iter = hlist_entry(p, typeof(*iter), list);

		if (iter->flow_entry.index == index) {
			foundNode = true;
			read_unlock(&flow_table_lock);
			goto _end_search_flow_in_time_set;
		}
	}
	read_unlock(&flow_table_lock);

_end_search_flow_in_time_set:
	if (foundNode == false)
		return CA_E_PARAM;

	/* decide age and is_static */
	if (aging_time == 0) {
		age = HASH_AGING_STATIC;
		is_static = true;
	} else {
		aal_hash_aging_timer_get(&max_time);
		if (max_time == 0)
			return CA_E_ERROR;

		if (aging_time > max_time) {
			age = HASH_AGING_START;
		} else {
			unsigned int unit_time;
			unit_time = max_time / HASH_AGING_START;
			/* equivalent to ceiling(aging_time / unit_time) */
			age = (aging_time + unit_time - 1) / unit_time;
		}
		is_static = false;
	}

	if (index & CA_L3_NAT_ID_OVERFLOW_FLAG) {
		ret = aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, index & CA_L3_NAT_HASH_IDX_MASK, age);
	} else {
		ret = aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, index, age);
	}
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	write_lock(&flow_table_lock);
	iter->is_static = is_static;
	write_unlock(&flow_table_lock);

	return CA_E_OK;
}

ca_status_t ca_hash_flow_aging_time_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_OUT	ca_uint32_t		*aging_time)
{
	int ret;
	struct flow_node *iter;
	struct hlist_node *p;
	bool foundNode = false;
	unsigned int time; /* global aging time, unit is second */
	unsigned int age;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: index=%u\n", __func__, index);
	}

	/* search the flow by index */
	read_lock(&flow_table_lock);
	hlist_for_each(p, &flow_table[get_idx_hash(index)]) {
		iter = hlist_entry(p, typeof(*iter), list);

		if (iter->flow_entry.index == index) {
			foundNode = true;
			read_unlock(&flow_table_lock);
			goto _end_search_flow_in_time_get;
		}
	}
	read_unlock(&flow_table_lock);

_end_search_flow_in_time_get:
	if (foundNode == false)
		return CA_E_PARAM;

	if (iter->is_static) {
		*aging_time = 0;
	} else {
		if (index & CA_L3_NAT_ID_OVERFLOW_FLAG) {
			ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, index & CA_L3_NAT_HASH_IDX_MASK, &age, &time);
		} else {
			ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, index, &age, &time);
		}
		if (ret != AAL_E_OK)
			return CA_E_ERROR;

		*aging_time = time;
	}

	return CA_E_OK;
}

ca_status_t ca_hash_flow_add (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_flow_t		*flow_config)
{
	ca_status_t ret;
	aal_hash_aging_e hash_age = HASH_AGING_INVALID;
	aal_hash_key_t hash_key;
	aal_hash_action_t hash_action;
	ca_uint32_t rslt_idx;
	struct flow_node *flow;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (flow_config == NULL)
		return CA_E_PARAM;

	if ((flow_config->key_type < CA_FLOW_TYPE_0) || (flow_config->key_type >= CA_FLOW_TYPE_MAX))
		return CA_E_PARAM;

	if (flow_type_array[flow_config->key_type].valid == false)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: Dump flow_config:\n", __func__);
		print_flow(flow_config, (flow_config->aging_time == 0) ? true : false, false);
	}

	ret = generate_hash_key(&flow_config->key, &flow_type_array[flow_config->key_type].mask, &hash_key);
	if (ret != CA_E_OK) {
		if (ca_api_debug & API_DBG_FLOW) {
			ca_printf(KERN_INFO "%s: Fail to generate hash_key from flow. (ret=%d)\n", __func__, ret);
		}
		return ret;
	}

	ret = generate_hash_action(&flow_config->key, &flow_type_array[flow_config->key_type].mask,
		&flow_config->actions, &hash_action);
	if (ret != CA_E_OK) {
		if (ca_api_debug & API_DBG_FLOW) {
			ca_printf(KERN_INFO "%s: Fail to generate hash_action from flow. (ret=%d)\n", __func__, ret);
		}
		return ret;
	}

	if (flow_config->aging_time == 0) {
		hash_age = HASH_AGING_STATIC;
	} else {
		unsigned int aging_time, unit_time;
		aal_hash_aging_timer_get(&aging_time);
		unit_time = aging_time / HASH_AGING_START;
		if (flow_config->aging_time >= aging_time) {
			hash_age = HASH_AGING_START;
		} else {
			/* equivalent to ceiling(flow_config->aging_time / unit_time) */
			hash_age = (flow_config->aging_time + unit_time - 1) / unit_time;
		}
	}

#if defined(CONFIG_LUNA_G3_SERIES)
	ret = aal_hash_add(device_id, &hash_key, &hash_action, TUPLE_TYPE_FWD,
			hash_age, flow_type_array[flow_config->key_type].hash_mask_idx, flow_profile_map[flow_config->key_type], &rslt_idx);
	if(ret==CA_E_EXISTS) flow_config->index = rslt_idx;
#else
	ret = aal_hash_add(device_id, &hash_key, &hash_action, TUPLE_TYPE_FWD,
			hash_age, flow_type_array[flow_config->key_type].hash_mask_idx, 0, &rslt_idx);
#endif

	if (ret != CA_E_OK) {
		if (ca_api_debug & API_DBG_FLOW) {
			ca_printf(KERN_INFO "%s: aal_hash_add() is failed. (ret=%d)\n", __func__, ret);
		}
		return ret;
	}
	flow_config->index = rslt_idx;
	if (rslt_idx & HASH_HASH_IDX_OVERFLOW_FLAG)
		atomic_inc(&num_overflow_hash_entry);
	else
		atomic_inc(&num_hash_entry);

	/* add into local flow_table */
#if defined(CONFIG_ARCH_RTL8198F)
	flow = ca_zalloc_trace(sizeof(struct flow_node), GFP_ATOMIC, true);
#else
	flow = ca_zalloc_trace(sizeof(struct flow_node), GFP_KERNEL, true);
#endif
	if (flow == NULL) {
		ret = CA_E_MEMORY;
		goto _flow_add_error;
	}

	flow->flow_entry = *flow_config;
	flow->is_static = (flow_config->aging_time == 0) ? true : false;
	write_lock(&flow_table_lock);
	flow_table_hlist_add(flow);
	write_unlock(&flow_table_lock);

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: Flow entry is added, hash_idx=%u (0x%x).\n", __func__, rslt_idx, rslt_idx);
	}

	return CA_E_OK;

_flow_add_error:
	/* delete the created flow */
	aal_hash_delete(device_id, rslt_idx);

	if (rslt_idx & HASH_HASH_IDX_OVERFLOW_FLAG)
		atomic_dec(&num_overflow_hash_entry);
	else
		atomic_dec(&num_hash_entry);

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: Fail to add flow entry. (ret=%d)\n", __func__, ret);
	}


	return ret;
}

ca_status_t ca_hash_flow_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_OUT	ca_flow_t		*flow_config)
{
	unsigned int age = 0, time = 0;
	int ret;
	struct flow_node *iter;
	struct hlist_node *p, *n;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (flow_config == NULL)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: index=%u (0x%x)\n", __func__, index, index);
	}

	write_lock(&flow_table_lock);
	hlist_for_each_safe(p, n, &flow_table[get_idx_hash(index)]) {
		iter = hlist_entry(p, typeof(*iter), list);

		if (iter->flow_entry.index == index) {
			iter->flow_entry.aging_time = 0;
			if (iter->is_static == false) {
				if (index & CA_L3_NAT_ID_OVERFLOW_FLAG) {
					ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, index & CA_L3_NAT_HASH_IDX_MASK, &age, &time);
				} else {
					ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, index, &age, &time);
				}
				if (ret != AAL_E_OK) {
					write_unlock(&flow_table_lock);
					return CA_E_ERROR;
				}

				iter->flow_entry.aging_time = time;

			}

			*flow_config = iter->flow_entry;
			write_unlock(&flow_table_lock);
			return CA_E_OK;
		}

	}
	write_unlock(&flow_table_lock);

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: Flow of index %u is NOT found in flow_table.\n", __func__, index);
	}

	return CA_E_NOT_FOUND;
}

ca_status_t ca_hash_flow_iterate (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_iterator_t		*return_entry)
{
	ca_status_t status;
	unsigned int age = 0, time = 0;
	int ret;
	struct flow_node *iter;
	struct hlist_node *n;
	int i_flow;
	ca_flow_t *ptr = (ca_flow_t *) return_entry->entry_data;
	ca_uint32_t count = 0;
	bool foundNode = false;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (return_entry == NULL)
		return CA_E_PARAM;

	if (return_entry->entry_data == NULL)
		return CA_E_PARAM;

	if (return_entry->entry_count == 0)
		return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_flow_t) * return_entry->entry_count > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	ptr = (ca_flow_t *) return_entry->entry_data;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) foundNode = true;

	write_lock(&flow_table_lock);
	for (i_flow = 0; i_flow < flow_table_size; i_flow++) {
		hlist_for_each_entry_safe(iter, n, &flow_table[i_flow], list) {
			/* found the node stored in prev_handle, start to get contents */
			if (foundNode == false) {
				if (iter == return_entry->prev_handle) foundNode = true;
				continue;
			}

			iter->flow_entry.aging_time = 0;
			if (iter->is_static == false) {
				if (iter->flow_entry.index & CA_L3_NAT_ID_OVERFLOW_FLAG) {
					ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, iter->flow_entry.index & CA_L3_NAT_HASH_IDX_MASK, &age, &time);
				} else {
					ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, iter->flow_entry.index, &age, &time);
				}
				if (ret != AAL_E_OK) {
					write_unlock(&flow_table_lock);
					return CA_E_ERROR;
				}

				iter->flow_entry.aging_time = time;
			}

			memcpy(ptr, &iter->flow_entry, sizeof(iter->flow_entry));
			ptr++;
			count++;
			if (count >= return_entry->entry_count) break;
		}

		if (count >= return_entry->entry_count) break; /* leave all loops */
	}
	write_unlock(&flow_table_lock);

	return_entry->entry_count = count;

	if (flow_table_hlist_empty()) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* hlist_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

ca_status_t ca_hash_flow_delete (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index)
{
	int ret;
	struct flow_node *iter;
	struct hlist_node *p, *n;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	if (ca_api_debug & API_DBG_FLOW) {
		ca_printf(KERN_INFO "%s: index=%u (0x%x)\n", __func__, index, index);
	}

	write_lock(&flow_table_lock);
	hlist_for_each_safe(p, n, &flow_table[get_idx_hash(index)]) {
		iter = hlist_entry(p, typeof(*iter), list);
		if (iter->flow_entry.index == index) {
			/* find the entry */
			ret = aal_hash_delete(device_id, index);
			if (ret != AAL_E_OK) {
				if (ca_api_debug & API_DBG_FLOW)
					ca_printf("%s: aal_hash_delete() fails. (index=%u, ret=%d)\n", __func__, index, ret);
				write_unlock(&flow_table_lock);
				return CA_E_ERROR;
			}

			if (index & HASH_HASH_IDX_OVERFLOW_FLAG)
				atomic_dec(&num_overflow_hash_entry);
			else
				atomic_dec(&num_hash_entry);

			if (ca_api_debug & API_DBG_FLOW) {
				ca_printf("%s: Delete flow entry, index=%u (0x%x).\n", __func__, index, index);
			}

			flow_table_hlist_del(iter);
			ca_free_trace(iter);
		}
	}
	write_unlock(&flow_table_lock);

	return CA_E_OK;
}

ca_status_t ca_hash_flow_delete_all (
	CA_IN	ca_device_id_t		device_id)
{
	struct flow_node *iter;
	struct hlist_node *n;
	int i_flow;

	if (flow_support == 0)
		return CA_E_NOT_SUPPORT;

	write_lock(&flow_table_lock);
	for (i_flow = 0; i_flow < flow_table_size; i_flow++) {
		hlist_for_each_entry_safe(iter, n, &flow_table[i_flow], list) {
			aal_hash_delete(device_id, iter->flow_entry.index);

			if (iter->flow_entry.index & HASH_HASH_IDX_OVERFLOW_FLAG)
				atomic_dec(&num_overflow_hash_entry);
			else
				atomic_dec(&num_hash_entry);

			if (ca_api_debug & API_DBG_FLOW) {
				ca_printf("%s: Delete flow entry, index=%u (0x%x).\n", __func__, iter->flow_entry.index, iter->flow_entry.index);
			}

			flow_table_hlist_del(iter);
			ca_free_trace(iter);
		}
	}
	write_unlock(&flow_table_lock);

	return CA_E_OK;
}

ca_status_t ca_hash_flow_init(ca_device_id_t device_id)
{
	int i;

	flow_table = ca_malloc_trace(sizeof(struct hlist_head) * flow_table_size, GFP_KERNEL, true);
	if (flow_table == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory for internal Flow Table.\n", __func__);
		return CA_E_RESOURCE;
	}
	for (i = 0; i < flow_table_size; i++) {
		INIT_HLIST_HEAD(&flow_table[i]);
	}

	aal_scfg_read(CFG_ID_FLOW_SUPPORT, 1, &flow_support);

#if defined(CONFIG_ARCH_CORTINA_VENUS)
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_INDEX_MODE, 1, &l3fe_hash_action_indexed_mode);
#endif

#if defined(CONFIG_LUNA_G3_SERIES)	
	memset(&flow_profile_map, 0, sizeof(flow_profile_map));
#endif

	return CA_E_OK;
}

EXPORT_SYMBOL(ca_hash_flow_key_type_add);
EXPORT_SYMBOL(ca_hash_flow_key_type_get);
EXPORT_SYMBOL(ca_hash_flow_key_type_delete);
EXPORT_SYMBOL(ca_hash_flow_aging_time_set);
EXPORT_SYMBOL(ca_hash_flow_aging_time_get);
EXPORT_SYMBOL(ca_hash_flow_add);
EXPORT_SYMBOL(ca_hash_flow_get);
EXPORT_SYMBOL(ca_hash_flow_iterate);
EXPORT_SYMBOL(ca_hash_flow_delete);
EXPORT_SYMBOL(ca_hash_flow_delete_all);

/***************************************************************************
 * Cortina API - will be obsolete
 ***************************************************************************/

ca_status_t ca_flow_key_type_add (
	CA_IN	ca_device_id_t 			device_id,
	CA_IN	ca_flow_key_type_config_t	*key_type_config)
{
	return ca_hash_flow_key_type_add(device_id, key_type_config);
}

ca_status_t ca_flow_key_type_get (
	CA_IN	ca_device_id_t 			device_id,
	CA_OUT	ca_flow_key_type_config_t	*key_type_config)
{
	return ca_hash_flow_key_type_get(device_id, key_type_config);
}

ca_status_t ca_flow_key_type_delete (
	CA_IN	ca_device_id_t			device_id,
	CA_IN	ca_flow_key_type_t		key_type)
{
	return ca_hash_flow_key_type_delete(device_id, key_type);
}

ca_status_t ca_flow_aging_time_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_IN	ca_uint32_t		aging_time)
{
	return ca_hash_flow_aging_time_set(device_id, index, aging_time);
}

ca_status_t ca_flow_aging_time_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_OUT	ca_uint32_t		*aging_time)
{
	return ca_hash_flow_aging_time_get(device_id, index, aging_time);
}

ca_status_t ca_flow_add (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_flow_t		*flow_config)
{
	return ca_hash_flow_add(device_id, flow_config);
}

ca_status_t ca_flow_get (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index,
	CA_OUT	ca_flow_t		*flow_config)
{
	return ca_hash_flow_get(device_id, index, flow_config);
}

ca_status_t ca_flow_iterate (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_iterator_t		*return_entry)
{
	return ca_hash_flow_iterate(device_id, return_entry);
}

ca_status_t ca_flow_delete (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_uint32_t		index)
{
	return ca_hash_flow_delete(device_id, index);
}

ca_status_t ca_flow_delete_all (
	CA_IN	ca_device_id_t		device_id)
{
	return ca_hash_flow_delete_all(device_id);
}

#if defined(CONFIG_LUNA_G3_SERIES)	
EXPORT_SYMBOL(ca_flow_egress_mirrored_port_set);
EXPORT_SYMBOL(ca_flow_key_profile_mapping_set);
#endif
EXPORT_SYMBOL(ca_flow_key_type_add);
EXPORT_SYMBOL(ca_flow_key_type_get);
EXPORT_SYMBOL(ca_flow_key_type_delete);
EXPORT_SYMBOL(ca_flow_aging_time_set);
EXPORT_SYMBOL(ca_flow_aging_time_get);
EXPORT_SYMBOL(ca_flow_add);
EXPORT_SYMBOL(ca_flow_get);
EXPORT_SYMBOL(ca_flow_iterate);
EXPORT_SYMBOL(ca_flow_delete);
EXPORT_SYMBOL(ca_flow_delete_all);
