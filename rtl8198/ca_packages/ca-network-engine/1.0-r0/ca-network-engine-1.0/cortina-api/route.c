#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/crc32.h>
#include <linux/crc16.h>
#include "osal_common.h"
#include "aal_l3fe.h"
#include "aal_l3_if.h"
#include "aal_l3_cam.h"
#include "aal_l3_cls.h"
#include "aal_lpm.h"
#include "aal_hashlite.h"
#include "aal_l3_pe.h"
#include "aal_l3_stg0.h"
#include "aal_port.h"
#include "aal_fdb.h"
#include "route.h"
#include "route_internal.h"
#include "tunnel.h"
#include "ca_mut.h"
#include "ca-scfg/scfg.h"
#include "api_common.h"

#ifdef CONFIG_NE_L3COS
#include "ca_l3cos.h"
#endif


/* Max parent interfaces/tunnels
 *
 * The extremely overlapped example:
 *    L2TP over IPSec over PPP over Ethernet
 * - L2TP has 3 parent interaces (IPSec, PPPoE, Ethernet)
 * - L2TP has 2 parent tunnels (IPSec, PPPoE)
 */
#define MAX_PROTOCOL_STACK	4

extern ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];

/***************************************************************************
 * common internal functions
 ***************************************************************************/

/* Find First Set
 * Return:
 *   1 ~ 64: the first position of bit 1 from LSB.
 *   0: all zero.
 */
static inline int ca_ffs(ca_uint64_t n)
{
	ca_uint64_t t = 1;
	int pos = 1;

	if (n == 0) return 0;

	while ((n & t) == 0) {
		t <<= 1;
		pos++;
	}

	return pos;
}

#define ca_ffz(x) ca_ffs(~(x))


/***************************************************************************
 * Unique number allocation/free for SW_only NextHop ID.
 *
 * When an interface is created, OS creates a neighbor route entry such as:
 *   route add -net 192.168.1.0 netnask 255.255.255.0 dev eth0
 * There is no gateway IP; the IP in NextHop entry is all zero.
 * Such kind of entry is for neighbor routing; when a packet matches this route,
 * OS looks up the ARP table and search the destination for the packet.
 *
 * HashLite engine can't create such hash_entry when there is no ARP info.
 * Therefore, a SW_only NextHop entry is created.
 * No data is set into HashLite engine unless OS receives any ARP reply.
 *
 * The minimum SW_only NextHop ID is 4096 which skips the sum of
 * HashLite Hash Table and HashLite Overflow Table.
 * The total number of unique number requirement is 64 which is same as
 * Interface Table size because such kind entry of addr zero is no more than
 * the number.
 *
 * Therefore, available range for SW_only NextHop ID is 4096 ~ 4159.
 ***************************************************************************/

ca_uint64_t bm_swonly_nhid;

static ca_uint bm_swonly_nhid_lock;

/* Alloc a unique SW_only NextHop ID.
 * It is for the NextHop entry with address zero.
 */
static ca_status_t bm_swonly_nhid_alloc(unsigned int *val)
{
	unsigned int tmp_val;
	ca_status_t ret;

	ca_spin_lock(bm_swonly_nhid_lock);

	tmp_val = ca_ffz(bm_swonly_nhid);
	if (tmp_val == 0) {
		ret = CA_E_FULL;
	} else {
		tmp_val--; /* Function returns 1~64, but here needs 0~63 for bit operation. */
		bm_swonly_nhid |= (1UL << tmp_val);
		*val = tmp_val + MIN_BM_SWONLY_NHID;
		ret = CA_E_OK;
	}

	ca_spin_unlock(bm_swonly_nhid_lock);

	return ret;
}

static void bm_swonly_nhid_free(unsigned int val)
{
	unsigned int tmp_val;

	if ((val < MIN_BM_SWONLY_NHID) || (val > MAX_BM_SWONLY_NHID)) return;

	ca_spin_lock(bm_swonly_nhid_lock);

	tmp_val = val - MIN_BM_SWONLY_NHID;
	bm_swonly_nhid &= ~(1 << tmp_val);

	ca_spin_unlock(bm_swonly_nhid_lock);
}

static ca_status_t bm_swonly_nhid_init(void)
{
	ca_status_t ret;

	if (CA_OSAL_OK != ca_spin_lock_init(&bm_swonly_nhid_lock, SPINLOCK_CMN)) {
		ca_printf("%s: Fail to init spinlock. (ret=%u)\n", __func__, ret);
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

static ca_status_t bm_swonly_nhid_exit(void)
{
	ca_spin_lock_destroy(bm_swonly_nhid_lock);

	return CA_E_OK;
}

/***************************************************************************
 * interface table & internal functions
 ***************************************************************************/

struct ca_int_l3_intf_s *ca_l3_intf_tbl = NULL;

static ca_uint ca_l3_intf_lock;
static int ca_l3_intf_init_done = 0;
static ca_uint32_t ip_ecn_support = 0;
static ca_mac_addr_t lb_intf_mac;

static ca_uint32_t upstream_port_id = 0x16;
static ca_uint32_t downstream_port_id = 0x17;

inline static void ca_l3_intf_dump(ca_l3_intf_t *intf)
{
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (intf == NULL) {
		ca_printf(KERN_ERR "%s: intf is NULL\n", __func__);
		return;
	}

	ca_printf(KERN_INFO "intf_id = %u\n", intf->intf_id);
	ca_printf(KERN_INFO "      intf_type = %d", intf->type);
	switch (intf->type) {
		case CA_L3_INTF_BCAST:
			ca_printf(" [BROADCAST]\n"); break;
		case CA_L3_INTF_P2P:
			ca_printf(" [P2P]\n"); break;
		case CA_L3_INTF_TUNNEL:
			ca_printf(" [TUNNEL]\n"); break;
		case CA_L3_INTF_CPU:
			ca_printf(" [CPU]\n"); break;
		case CA_L3_INTF_LB:
			ca_printf(" [LOOPBACK]\n"); break;
		default:
			ca_printf(" [UNKNOWN]\n"); break;
	}

	ca_printf(KERN_INFO "  [%c] port_id = 0x%x\n", (intf->mask.port_id) ? 'V' : ' ', intf->port_id);
	ca_printf(KERN_INFO "  [%c] mac_addr = %pM\n", (intf->mask.mac_addr) ? 'V' : ' ', intf->mac_addr);
	ca_printf(KERN_INFO "  [%c] outer_vid = %u\n", (intf->mask.outer_vid) ? 'V' : ' ', intf->outer_vid);
	ca_printf(KERN_INFO "  [%c] outer_tpid = 0x%04x\n", (intf->mask.outer_tpid) ? 'V' : ' ', intf->outer_tpid);
	ca_printf(KERN_INFO "  [%c] inner_vid = %u\n", (intf->mask.inner_vid) ? 'V' : ' ', intf->inner_vid);
	ca_printf(KERN_INFO "  [%c] inner_tpid = 0x%04x\n", (intf->mask.inner_tpid) ? 'V' : ' ', intf->inner_tpid);
	ca_printf(KERN_INFO "  [%c] mtu = %u\n", (intf->mask.mtu) ? 'V' : ' ', intf->mtu);

	if (intf->ip_addr.afi == CA_IPV4) {
		ipv4_addr.s_addr = htonl(intf->ip_addr.ip_addr.ipv4_addr);
		ca_printf(KERN_INFO "  [%c] ip_addr = %pI4 / %u\n",
				(intf->mask.ip_addr) ? 'V' : ' ',
				&ipv4_addr, intf->ip_addr.addr_len);
	} else {
		ipv6_addr.in6_u.u6_addr32[0] = htonl(intf->ip_addr.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(intf->ip_addr.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(intf->ip_addr.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(intf->ip_addr.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "  [%c] ip_addr = %pI6 / %u\n",
				(intf->mask.ip_addr) ? 'V' : ' ',
				&ipv6_addr, intf->ip_addr.addr_len);
	}

	ca_printf(KERN_INFO "  [%c] nat_enable = %u\n", (intf->mask.nat_enable) ? 'V' : ' ', intf->nat_enable);
	ca_printf(KERN_INFO "  [%c] tunnel_id = %u\n", (intf->mask.tunnel_id) ? 'V' : ' ', intf->tunnel_id);
}

ca_status_t ca_l3_intf_init(CA_IN ca_device_id_t device_id)
{
	int i, tbl_size;
	ca_status_t ret;

	if (ca_l3_intf_init_done != 0)
		return CA_E_OK;

	if (CA_OSAL_OK != ca_spin_lock_init(&ca_l3_intf_lock, SPINLOCK_BH)) {
		ca_printf("%s: init spinlock fail.\n", __func__);
		return CA_E_ERROR;
	}

	tbl_size = sizeof(struct ca_int_l3_intf_s) * FE_L3E_IF_TBL_ENTRY_MAX;
	ca_l3_intf_tbl = ca_malloc_trace(tbl_size, GFP_ATOMIC, true);
	if (ca_l3_intf_tbl == NULL) {
		ca_printf("%s: no memory for intf table(size = 0x%X)\n", __func__, tbl_size);
		ca_spin_lock_destroy(ca_l3_intf_lock);
		return CA_E_MEMORY;
	}
	memset(ca_l3_intf_tbl, 0, tbl_size);
	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
		ca_l3_intf_tbl[i].cls_idx = CA_UINT16_INVALID;
	}

	ret = aal_scfg_read(CFG_ID_IP_ECN_SUPPORT, 1, (ca_uint8_t *) &ip_ecn_support);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to read scfg CFG_ID_IP_ECN_SUPPORT! (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	ret = aal_scfg_read(CFG_ID_UPSTREAM_PORT_ID, 4, (ca_uint32_t *) &upstream_port_id);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to read scfg CFG_ID_UPSTREAM_PORT_ID! (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	ret = aal_scfg_read(CFG_ID_DOWNSTREAM_PORT_ID, 4, (ca_uint32_t *) &downstream_port_id);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to read scfg CFG_ID_DOWNSTREAM_PORT_ID! (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	/* loopback */
	lb_intf_mac[0] = 0x00;
	lb_intf_mac[1] = 0x13;
	lb_intf_mac[2] = 0x25;
	lb_intf_mac[3] = 0x00;
	lb_intf_mac[4] = 0x00;
	lb_intf_mac[5] = 0xFF;

	ca_l3_intf_init_done = 1;

	return CA_E_OK;
}

ca_status_t ca_l3_intf_exit(void)
{
	if (ca_l3_intf_init_done == 0)
		return CA_E_OK;

	ca_l3_intf_init_done = 0;

	ca_spin_lock_destroy(ca_l3_intf_lock);
	ca_free_trace(ca_l3_intf_tbl);

	return CA_E_OK;
}

static ca_status_t ca_l3_intf_alloc(ca_l3_intf_t *intf)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	int i, idx_start, idx_end, max_idx, aal_idx;

	/* normal case */
	idx_start = 0;
	idx_end = FE_L3E_IF_TBL_ENTRY_MAX - 1;

	if (intf->type == CA_L3_INTF_TUNNEL) {
		if (intf->tunnel_id >= CA_TUNNEL_MAX_LIMIT)
			return CA_E_PARAM;
		if ((g_tunnel[intf->tunnel_id] == NULL) || (g_tunnel[intf->tunnel_id]->used == CA_FALSE))
			return CA_E_PARAM;
			
		tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

		if (tcfg->type == CA_TUNNEL_DSLITE) {
			max_idx = CA_DSLITE_INTF_MAX - CA_DSLITE_INTF_MIN;
			aal_idx = ((dslite_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->aal_idx;

			if (aal_idx > max_idx)
				return CA_E_PARAM;

			idx_start = CA_DSLITE_INTF_MIN + aal_idx;
			idx_end = idx_start;
		} else if (tcfg->type == CA_TUNNEL_6RD) {
			max_idx = CA_6RD_INTF_MAX - CA_6RD_INTF_MIN;
			aal_idx = ((sixrd_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->aal_idx;

			if (aal_idx > max_idx)
				return CA_E_PARAM;

			idx_start = CA_6RD_INTF_MIN + aal_idx;
			idx_end = idx_start;
		} else if (tcfg->type == CA_TUNNEL_MAP_E) {
			max_idx = CA_DSLITE_INTF_MAX - CA_DSLITE_INTF_MIN;
			aal_idx = ((map_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->aal_idx;

			if (aal_idx > max_idx)
				return CA_E_PARAM;

			idx_start = CA_DSLITE_INTF_MIN + aal_idx;
			idx_end = idx_start;
		}
	}

	for (i = idx_start; i <= idx_end; i++) {
		if (ca_l3_intf_tbl[i].use == 0) {
			intf->intf_id = i;
			return CA_E_OK;
		}
	}

	return CA_E_FULL;
}

extern ca_status_t qos_l3_if_get(
	ca_uint32_t ldpid,
	ca_uint8_t *inner_8021p_sel,
	ca_uint8_t *top_8021p_sel);

static ca_status_t convert_intf_to_eif_broadcast(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, intf_id=%u\n", __func__, depth_protocol_stack, intf->intf_id);
	}

	eif->vlan_vld = 0; /* decide later */
	eif->vlan_cnt = 0; /* decide later */

	if (intf->mask.outer_vid != 0) {
		eif->vlan_vld = 1;

		if (intf->outer_vid >= 0 && intf->outer_vid <= 4095) {
			ca_uint8_t index;

			eif->vlan_cnt++;
			eif->top_vid = intf->outer_vid;

			if (aal_l3fe_pp_top_tpid_get(intf->outer_tpid, &index) == AAL_E_OK) {
				eif->top_tpid_enc = index;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find outer_tpid 0x%04X setting.\n", __func__, intf->outer_tpid);
				return CA_E_NOT_FOUND;
			}
		} else {
			eif->top_vid = 0;
			eif->top_tpid_enc = 0;
		}
		eif->top_dei = 0;
		eif->top_dei_sel = 0;
	} else {
		/* don't set any VLAN fields */
		eif->top_tpid_enc = 0;
		eif->top_vid = 0;
		eif->top_dei = 0;
		eif->top_dei_sel = 0;
	}

	if (intf->mask.inner_vid != 0) {
		eif->vlan_vld = 1;

		if (intf->inner_vid >= 0 && intf->inner_vid <= 4095) {
			ca_uint8_t index;

			eif->vlan_cnt++;
			eif->inner_vid = intf->inner_vid;

			if (aal_l3fe_pp_inner_tpid_get(intf->inner_tpid, &index) == AAL_E_OK) {
				eif->inner_tpid_enc = index;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find inner_tpid 0x%04X setting.\n", __func__, intf->inner_tpid);
				return CA_E_NOT_FOUND;
			}
		} else {
			eif->inner_vid = 0;
			eif->inner_tpid_enc = 0;
		}
		eif->inner_dei = 0;
		eif->inner_dei_sel = 0;
	} else {
		/* don't set any VLAN fields */
		eif->inner_tpid_enc = 0;
		eif->inner_vid = 0;
		eif->inner_dei = 0;
		eif->inner_dei_sel = 0;
	}

	return CA_E_OK;
}

static ca_status_t convert_intf_to_eif_pppoe(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	eif->pppoe_session_id = tcfg->tunnel.pppoe.pppoe_session_id;
	eif->pppoe_vld = 1;
	eif->pppoe_set = 1;

	/* Handle protocol stack. Available case:
	 * - PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPPoE doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_ipsec(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	eif->mac_sa_vld = 0;
	eif->mac_sa_an_sel = 0;
	eif->ldpid = upstream_port_id;

	eif->stage3_ctrl = STAGE3_CTRL_BYPASS; /* IPSec interface doesn't need NAT (Hash Engine result) */
	eif->stage3_ctrl_vld = 0;

	/* Handle protocol stack. Available case:
	 * - IPSec over Ethernet
	 * - IPSec over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: IPSec can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: IPSec doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_l2tp(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	eif->mac_sa_vld = 0;
	eif->mac_sa_an_sel = 0;
	eif->ldpid = upstream_port_id;

	/* Handle protocol stack. Available case:
	 * - L2TP over Ethernet
	 * - L2TP over PPP over Ethernet
	 * - L2TP over IPSec over ...
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type == CA_TUNNEL_IPSEC) {
			return convert_intf_to_eif_ipsec(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
		} else if (parent_tcfg->type == CA_TUNNEL_PPPOE) {
			return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
		} else {
			ca_printf(KERN_INFO "%s: L2TP can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: L2TP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}


	return CA_E_OK;
}

static ca_status_t convert_intf_to_eif_dslite(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! DSLite must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
		eif->ldpid = AAL_LPORT_ETH_WAN;
	else
		eif->ldpid = PORT_ID(intf->port_id);

	/* Handle protocol stack. Available case:
	 * - DSLite over Ethernet
	 * - DSLite over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: DSLite can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: DSLite doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_6rd(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! 6RD must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
		eif->ldpid = AAL_LPORT_ETH_WAN;
	else
		eif->ldpid = PORT_ID(intf->port_id);

	/* Handle protocol stack. Available case:
	 * - 6RD over Ethernet
	 * - 6RD over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: 6RD can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: 6RD doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_map_e(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-E must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN)) {
		eif->ldpid = AAL_LPORT_ETH_WAN; // send out MAP-E pkt directly
		// eif->ldpid = upstream_port_id;
	} else {
		eif->ldpid = PORT_ID(intf->port_id);
	}

	/* Handle protocol stack. Available case:
	 * - MAP-E over Ethernet
	 * - MAP-E over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-E can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-E doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_map_t(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-T must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	eif->mac_sa_vld = 0;
	eif->mac_sa_an_sel = 0;
	eif->ldpid = upstream_port_id;

	eif->stage3_ctrl = STAGE3_CTRL_BYPASS; /* MAP-T interface doesn't need NAT (Hash Engine result) */
	eif->stage3_ctrl_vld = 0;

	/* Handle protocol stack. Available case:
	 * - MAP-T over Ethernet
	 * - MAP-T over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-T can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_eif_pppoe(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-T doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif_pptp(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif,
	int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	/* action */

	eif->mac_sa_vld = 0;
	eif->mac_sa_an_sel = 0;
	eif->ldpid = upstream_port_id;

	/* Handle protocol stack. Available case:
	 * - PPTP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_eif_broadcast(&parent_intf, mtu_idx, mac_idx, eif, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPTP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_eif(
	ca_l3_intf_t *intf,
	ca_uint32 mtu_idx,
	ca_uint32 mac_idx,
	fe_l3e_if_tbl_entry_t *eif)
{
	ca_uint8_t wan_interface_deep_q = 0;
	ca_uint8_t lan_interface_deep_q = 0;
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_uint8_t inner_8021p_sel;
	ca_uint8_t top_8021p_sel;
	ca_status_t ret;

	memset(eif, 0, sizeof(fe_l3e_if_tbl_entry_t));

	eif->stage3_ctrl = (intf->mask.nat_enable) ? ((intf->nat_enable) ? STAGE3_CTRL_UPDATE : STAGE3_CTRL_BYPASS) : STAGE3_CTRL_BYPASS;
	eif->stage3_ctrl_vld = 1;

	ret = aal_scfg_read(CFG_ID_WAN_INTERFACE_DEEP_Q, 1, &wan_interface_deep_q);
	if (ret != CA_E_OK) {
		printk("%s: failed to read CFG_ID_WAN_INTERFACE_DEEP_Q, ret=%d\n", __func__, ret);
	}
	ret = aal_scfg_read(CFG_ID_LAN_INTERFACE_DEEP_Q, 1, &lan_interface_deep_q);
	if (ret != CA_E_OK) {
		printk("%s: failed to read CFG_ID_LAN_INTERFACE_DEEP_Q, ret=%d\n", __func__, ret);
	}
	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
		eif->deep_q = (wan_interface_deep_q == 0) ? 0 : 1;
	else
		eif->deep_q = (lan_interface_deep_q == 0) ? 0 : 1;

	/* get necessary info and decide ldpid */
	if ((intf->type == CA_L3_INTF_BCAST) || (intf->type == CA_L3_INTF_P2P) || (intf->type == CA_L3_INTF_TUNNEL)) {
		/* For tunnel, this is temporary decision */
		if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
			eif->ldpid = AAL_LPORT_ETH_WAN;
		else
			eif->ldpid = PORT_ID(intf->port_id);
	} else if (intf->type == CA_L3_INTF_CPU) {
		if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
			eif->ldpid = AAL_LPORT_CPU_0;
		else
			eif->ldpid = AAL_LPORT_CPU_1;
	} else {
		eif->ldpid = AAL_LPORT_L3_LAN;
	}

	eif->ldpid_vld = 1;
	eif->l2_format = 0;
	eif->l2_format_vld = 0;

	if (intf->type == CA_L3_INTF_LB) {
		eif->mac_sa_vld = 0;
		eif->mac_sa_an_sel = 0;
	} else {
		eif->mac_sa_vld = 1;
		eif->mac_sa_an_sel = mac_idx + 1;
	}
	eif->mac_da_vld = 0; /* filled by nexthop */
	eif->mac_da_an_sel = 0;

	if ((intf->type == CA_L3_INTF_BCAST) || (intf->type == CA_L3_INTF_P2P)) {
		/* if there is a PPPoE over this interface, this setting will be overwritten */
		eif->pppoe_set = 0;	/* for PPPoE Pass Through */
	}

	eif->vlan_vld = 0; /* decide later */
	eif->vlan_cnt = 0; /* decide later */

	if (intf->mask.outer_vid != 0) {
		eif->vlan_vld = 1;

		if (intf->outer_vid >= 0 && intf->outer_vid <= 4095) {
			ca_uint8_t index;

			eif->vlan_cnt++;
			eif->top_vid = intf->outer_vid;

			if (aal_l3fe_pp_top_tpid_get(intf->outer_tpid, &index) == AAL_E_OK) {
				eif->top_tpid_enc = index;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find outer_tpid 0x%04X setting.\n", __func__, intf->outer_tpid);
				return CA_E_NOT_FOUND;
			}
		} else {
			eif->top_vid = 0;
			eif->top_tpid_enc = 0;
		}
		eif->top_dei = 0;
		eif->top_dei_sel = 0;
	} else {
		/* don't set any VLAN fields */
		eif->top_tpid_enc = 0;
		eif->top_vid = 0;
		eif->top_dei = 0;
		eif->top_dei_sel = 0;
	}

	if (intf->mask.inner_vid != 0) {
		eif->vlan_vld = 1;

		if (intf->inner_vid >= 0 && intf->inner_vid <= 4095) {
			ca_uint8_t index;

			eif->vlan_cnt++;
			eif->inner_vid = intf->inner_vid;

			if (aal_l3fe_pp_inner_tpid_get(intf->inner_tpid, &index) == AAL_E_OK) {
				eif->inner_tpid_enc = index;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find inner_tpid 0x%04X setting.\n", __func__, intf->inner_tpid);
				return CA_E_NOT_FOUND;
			}
		} else {
			eif->inner_vid = 0;
			eif->inner_tpid_enc = 0;
		}
		eif->inner_dei = 0;
		eif->inner_dei_sel = 0;
	} else {
		/* don't set any VLAN fields */
		eif->inner_tpid_enc = 0;
		eif->inner_vid = 0;
		eif->inner_dei = 0;
		eif->inner_dei_sel = 0;
	}

	eif->qos_802_1p_table_sel = 0;
	eif->top_802_1p = 0;
	eif->inner_802_1p = 0;
	qos_l3_if_get(eif->ldpid ,&inner_8021p_sel, &top_8021p_sel);
	eif->top_802_1p_sel = top_8021p_sel;
	eif->inner_802_1p_sel = inner_8021p_sel;

	if (intf->mtu == 0) {
		eif->mtu_enc = 0;
		eif->mtu_vld = 0;
	} else {
		eif->mtu_enc = mtu_idx;
		eif->mtu_vld = 1;
	}

	if (intf->type == CA_L3_INTF_TUNNEL) {
		tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

		if (tcfg->type == CA_TUNNEL_PPPOE) {
			if ((ret = convert_intf_to_eif_pppoe(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_L2TP) {
			if ((ret = convert_intf_to_eif_l2tp(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_IPSEC) {
			if ((ret = convert_intf_to_eif_ipsec(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_PPTP) {
			if ((ret = convert_intf_to_eif_pptp(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_4_IN_4) {

		}

		if (tcfg->type == CA_TUNNEL_DSLITE) {
			if ((ret = convert_intf_to_eif_dslite(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6RD) {
			if ((ret = convert_intf_to_eif_6rd(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6_IN_6) {

		}

		if (tcfg->type == CA_TUNNEL_MAP_E) {
			if ((ret = convert_intf_to_eif_map_e(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_MAP_T) {
			if ((ret = convert_intf_to_eif_map_t(intf, mtu_idx, mac_idx, eif, 0)) != CA_E_OK)
				return ret;
		}
	}

	return CA_E_OK;
}

/* This function is only for the case that tunnel type interface over broadcast-type interface */
static ca_status_t convert_intf_to_cls_broadcast(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, intf_id=%u\n", __func__, depth_protocol_stack, intf->intf_id);
	}

	rule->key.vlan_cnt = 0; /* decide later; maybe don't care vlan_cnt */

	if (intf->mask.outer_vid != 0) {
		rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

		if (intf->outer_vid >= 0 && intf->outer_vid <= 4095) {
			ca_uint8_t index;

			rule->key.vlan_cnt++;

			rule->key.top_vid = intf->outer_vid;
			rule->key.msk_top_vid = 0;

			if (aal_l3fe_pp_top_tpid_get(intf->outer_tpid, &index) == AAL_E_OK) {
				rule->key.top_tpid_enc = index;
				rule->key.msk_top_tpid_enc = 0;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find outer_tpid 0x%04X setting.\n", __func__, intf->outer_tpid);
				return CA_E_NOT_FOUND;
			}
		}
	}

	if (intf->mask.inner_vid != 0) {
		rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

		if (intf->inner_vid >= 0 && intf->inner_vid <= 4095) {
			ca_uint8_t index;

			rule->key.vlan_cnt++;

			rule->key.inner_vid = intf->inner_vid;
			rule->key.msk_inner_vid = 0;

			if (aal_l3fe_pp_inner_tpid_get(intf->inner_tpid, &index) == AAL_E_OK) {
				rule->key.inner_tpid_enc = index;
				rule->key.msk_inner_tpid_enc = 0;
			} else {
				ca_printf(KERN_ERR "%s: Fail to find inner_tpid 0x%04X setting.\n", __func__, intf->inner_tpid);
				return CA_E_NOT_FOUND;
			}
		}
	}

	return CA_E_OK;
}

static ca_status_t convert_intf_to_cls_pppoe(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	l3_cam_ethertype_tbl_entry_t ethertype;
	l3_cam_ppp_prot_tbl_entry_t ppp_prot;
	ca_uint32_t cam_ethertype_rslt_idx, cam_ppp_prot_rslt_idx;
	ca_status_t ret;
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	/* CAM table, Ethertype */
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8864; /* PPPoE Session */
	ret = aal_entry_get(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_ethertype_rslt_idx);
	if (ret != AAL_E_OK) {
		printk("%s: Fail to get Ethertype 0x8864 from CAM table, ret = %d\n", __func__, ret);
		return CA_E_UNAVAIL;
	}

	/* CAM table, PPP Protocol */
	memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
	ppp_prot.vld = 1;
	ppp_prot.ppp_prot = (intf->ip_addr.afi == CA_IPV4) ? 0x0021 : 0x0057;
	ret = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
	if (ret != AAL_E_OK) {
		printk("%s: Fail to get PPP Protocol from CAM table, ret = %d\n", __func__, ret);
		return CA_E_UNAVAIL;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	rule->key.ethertype_enc = cam_ethertype_rslt_idx + 1;
	rule->key.msk_ethertype_enc = 0;
	rule->key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
	rule->key.msk_ppp_protocol = 0;
	rule->key.pppoe_session_id = tcfg->tunnel.pppoe.pppoe_session_id;
	rule->key.msk_pppoe_session = 0;

	/* action */

	// decap PPPoE fields
	rule->action.pppoe_vld = 0;
	rule->action.pppoe_set = 1;
	rule->action.pppoe_session_id = 0;

	/* Handle protocol stack. Available case:
	 * - PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPPoE doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_ipsec(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	bool is_l2tp_ipsec = false; /* true: L2TPv2/v3 over IPSec. false: IPSec only. */
	int l2tp_ver = 2;
	ca_ipsec_sa_t ipsec_sa_decrypt;
	ca_status_t ret;
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;
	tunnel_info_t tinfo;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	if ((ret = ca_ipsec_sa_get(0, tcfg->tunnel.ipsec.sa_id_decrypt, &ipsec_sa_decrypt)) != CA_E_OK) {
		return CA_E_PARAM;
	}

	/* check if this is L2TP over IPSec */
	if (   (depth_protocol_stack != 0)
	    && (rule->key.msk_ip_protocol == 0)
	    && ((rule->key.ip_protocol == IPPROTO_UDP) || (rule->key.ip_protocol == 0x73)) ) {
		is_l2tp_ipsec = true;
	}

	/* key */
	if (is_l2tp_ipsec) {
		/* overwrite L2TP key/action */
		if (ipsec_sa_decrypt.is_natt == 0) {
			rule->key.ip_protocol = IPPROTO_ESP;
			rule->key.msk_ip_protocol = 0;
			rule->key.l4_vld = 1;
			rule->key.msk_l4_vld = 1; /* disable */

			/* though spi in memory is host order, but here it is handled as a 4-byte object; needn't consider its order */
			rule->key.l4_sp = (ca_uint16_t) (ipsec_sa_decrypt.spi >> 16);			/* MSB */
			rule->key.l4_dp = (ca_uint16_t) (ipsec_sa_decrypt.spi & 0xFFFF);		/* LSB */
			rule->key.msk_spi = 0; /* Use {key.l4_sp, key_l4_dp} as SPI */
		} else {
			rule->key.ip_protocol = IPPROTO_UDP;
			rule->key.msk_ip_protocol = 0;
			rule->key.l4_vld = 1;
			rule->key.msk_l4_vld = 0;

			rule->key.l4_sp = ipsec_sa_decrypt.src_l4_port;
			rule->key.l4_dp = ipsec_sa_decrypt.dest_l4_port;
			rule->key.msk_l4_port = 0; /* L4_SP and L4_DP are exactly matched. */
			rule->key.msk_spi = 1; /* disable */
		}
		if (rule->action.mac_sa_0 == CA_FUNC_ID_L2TPV3) {
			 l2tp_ver = 3;
		} else if (rule->action.mac_sa_0 == CA_FUNC_ID_L2TP) {
			 l2tp_ver = 2;
		} else {
			ca_printf(KERN_INFO "%s: ERROR! L2TP version is unknown. (func_id %u)\n", __func__, rule->action.mac_sa_0);
			return CA_E_INTERNAL;
		}
	} else {
		/* pure IPSec */
		if (ipsec_sa_decrypt.is_natt == 0) {
			rule->key.ip_protocol = (ipsec_sa_decrypt.protocol == 0) ? IPPROTO_ESP : IPPROTO_AH;
			rule->key.msk_ip_protocol = 0;

			/* though spi in memory is host order, but here it is handled as a 4-byte object; needn't consider its order */
			rule->key.l4_sp = (ca_uint16_t) (ipsec_sa_decrypt.spi >> 16);			/* MSB */
			rule->key.l4_dp = (ca_uint16_t) (ipsec_sa_decrypt.spi & 0xFFFF);		/* LSB */
			rule->key.msk_spi = 0; /* Use {key.l4_sp, key_l4_dp} as SPI */
		} else {
			// TODO: treat spi as key
			rule->key.ip_protocol = IPPROTO_UDP;
			rule->key.msk_ip_protocol = 0;

			rule->key.l4_sp = ipsec_sa_decrypt.dest_l4_port;
			rule->key.l4_dp = ipsec_sa_decrypt.dest_l4_port;
			rule->key.msk_l4_port = 2; /* only see pkt.L4_DP which range is key.l4_sp to key.l4_dp */
		}
	}

	/* action */

	rule->action.mcgid = downstream_port_id;
	rule->action.mc = 0;
	rule->action.permit = 1;
	rule->action.dpid_pri = 1;
	rule->action.dpid_vld = 1;
	rule->action.t2_ctrl = T2_CTRL_BYPASS;
	rule->action.t2_ctrl_vld = 1;
	rule->action.t3_ctrl = T3_CTRL_BYPASS;
	rule->action.t3_ctrl_vld = 1;
	rule->action.t4_ctrl = T4_CTRL_BYPASS;
	rule->action.t4_ctrl_vld = 1;
	rule->action.t5_ctrl = T5_CTRL_BYPASS;
	rule->action.t5_ctrl_vld = 1;
	rule->action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	rule->action.stage2_ctrl_vld = 1;
	rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	rule->action.stage3_ctrl_vld = 1;

	tinfo.func_id = (is_l2tp_ipsec) ? ((l2tp_ver == 3) ? CA_FUNC_ID_L2TPV3_IPSEC : CA_FUNC_ID_L2TP_IPSEC) : CA_FUNC_ID_IPSEC;
	tinfo.pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo.direction = CA_IPSEC_SPD_DECRYPT;

	rule->action.mdata_w_2 = ((ipsec_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->idx4pe & 0xFFFF;
	rule->action.mdata_w_vld_2 = 0xF;
	rule->action.mdata_w_3 = tinfo.value;
	rule->action.mdata_w_vld_3 = 0x3;

	/* Handle protocol stack. Available case:
	 * - IPSec over Ethernet
	 * - IPSec over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: IPSec can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: IPSec doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_l2tp(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;
	tunnel_info_t tinfo;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	// assume pure L2TP
	if (tcfg->tunnel.l2tp.encap_type == 0) {
		/* UDP */
		rule->key.ip_protocol = IPPROTO_UDP;
		rule->key.msk_ip_protocol = 0;
		rule->key.l4_vld = 1; // TBD
		rule->key.msk_l4_vld = 0; // TBD

		rule->key.l4_sp = (ca_uint16_t) (htonl(tcfg->tunnel.l2tp.session_id) >> 16);		/* MSB */
		rule->key.l4_dp = (ca_uint16_t) (htonl(tcfg->tunnel.l2tp.session_id) & 0xFFFF);		/* LSB */
		rule->key.msk_spi = 0; /* Use {key.l4_sp, key_l4_dp} as SPI (L2TP session ID) */
	} else {
		/* IP */
		rule->key.ip_protocol = 0x73; /* L2TP */
		rule->key.msk_ip_protocol = 0;
		rule->key.l4_vld = 0; // TBD
		rule->key.msk_l4_vld = 0; // TBD

		rule->key.l4_sp = (ca_uint16_t) (htonl(tcfg->tunnel.l2tp.session_id) >> 16);		/* MSB */
		rule->key.l4_dp = (ca_uint16_t) (htonl(tcfg->tunnel.l2tp.session_id) & 0xFFFF);		/* LSB */
		rule->key.msk_spi = 0; /* Use {key.l4_sp, key_l4_dp} as SPI (L2TP session ID) */
	}

	/* action */

	rule->action.mcgid = downstream_port_id;
	rule->action.mc = 0;
	rule->action.permit = 1;
	rule->action.dpid_pri = 1;
	rule->action.dpid_vld = 1;
	rule->action.t2_ctrl = T2_CTRL_BYPASS;
	rule->action.t2_ctrl_vld = 1;
	rule->action.t3_ctrl = T3_CTRL_BYPASS;
	rule->action.t3_ctrl_vld = 1;
	rule->action.t4_ctrl = T4_CTRL_BYPASS;
	rule->action.t4_ctrl_vld = 1;
	rule->action.t5_ctrl = T5_CTRL_BYPASS;
	rule->action.t5_ctrl_vld = 1;
	rule->action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	rule->action.stage2_ctrl_vld = 1;
	rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	rule->action.stage3_ctrl_vld = 1;

	/* DO NOT REMOVE! This passes info to parent layer if it is L2TP over IPSec tunnel. */
	rule->action.mac_sa_0 = ((tcfg->tunnel.l2tp.version & 0x3) == 3) ? CA_FUNC_ID_L2TPV3 : CA_FUNC_ID_L2TP; /* function ID */

	tinfo.func_id = ((tcfg->tunnel.l2tp.version & 0x3) == 3) ? CA_FUNC_ID_L2TPV3 : CA_FUNC_ID_L2TP;
	tinfo.pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo.direction = CA_IPSEC_SPD_DECRYPT;

	rule->action.mdata_w_2 = ((l2tp_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->sa_id_inbound & 0xFFFF;
	rule->action.mdata_w_vld_2 = 0xF;
	rule->action.mdata_w_3 = tinfo.value;
	rule->action.mdata_w_vld_3 = 0x3;

	/* Handle protocol stack. Available case:
	 * - L2TP over Ethernet
	 * - L2TP over PPP over Ethernet
	 * - L2TP over IPSec over ...
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type == CA_TUNNEL_IPSEC) {
			return convert_intf_to_cls_ipsec(&parent_intf, rule, depth_protocol_stack + 1);
		} else if (parent_tcfg-> type == CA_TUNNEL_PPPOE) {
			return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);
		} else {
			ca_printf(KERN_INFO "%s: L2TP can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: L2TP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_dslite(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! DSLite must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	if (tcfg->tunnel.dslite.mc_flag == CA_TRUE) {
		// TODO: better solution to decide msk_ip_addr from ipv6_addr?
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
		/* In G3Lite, downstream MC packet comes from L2FE.
		 * In other SoC, downstrema MC packet comes from physical port directly.
		 */
		rule->key.lspid = wan_port_id;
#endif
		rule->key.ip_da_mc = 1;
		rule->key.msk_ip_da_mc = 0;

		/* Fill both key.ip_addr and key.ip_da, let AAL decides which key type is used */
		rule->key.ip_addr.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1] & tcfg->src_addr.ip_addr.ipv6_addr[3];
		rule->key.ip_addr.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0] & tcfg->src_addr.ip_addr.ipv6_addr[2]; // MSB
		rule->key.msk_ip_addr_l = 64; /* prefix 64 bits */
		rule->key.msk_ip_addr_h = 0;

		rule->key.ip_da.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1] & tcfg->src_addr.ip_addr.ipv6_addr[3];
		rule->key.ip_da.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0] & tcfg->src_addr.ip_addr.ipv6_addr[2]; // MSB
		rule->key.msk_ip_da_l = 64; /* prefix 64 bits */
		rule->key.msk_ip_da_h = 0;

		/* intf->mac is not Multicast MAC, so don't check MAC_DA */
		rule->key.mac_da_an_sel = 0;
		rule->key.msk_mac_da_an_sel = 1;
	} else {
		/* Must set key.ip_da instead of key.ip_addr; otherwise, there is no suitable CLS key type. */
		rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv6_addr[3];
		rule->key.ip_da.ip_addr_1 = tcfg->src_addr.ip_addr.ipv6_addr[2];
		rule->key.ip_da.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1];
		rule->key.ip_da.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0]; // MSB
		rule->key.msk_ip_da_l = intf->ip_addr.addr_len & 0x7F; /* prefix */
		rule->key.msk_ip_da_h = intf->ip_addr.addr_len >> 7; /* prefix */
	}
	rule->key.ip_protocol = IPPROTO_IPIP;
	rule->key.msk_ip_protocol = 0;

	/* action */

	rule->action.pop_l3_en = 1;
	rule->action.pop_l3_vld = 1;

	if (tcfg->tunnel.dslite.dsl_ecn_check_enable == CA_TRUE) {
		rule->action.pop_l3_chk_ecn_en = 1;
	}

	if (tcfg->tunnel.dslite.mc_flag == CA_TRUE) {
		/* Must ensure that ldpid / t#_ctrl / stage#_ctrl are same as the default MC rule in CLS. */
		/* NOTE: These settings are controllled by reg L3FE_STG1_DSL_IPMC_FWD_CTRL instead of CLS FIB, but still keep code here. */
		rule->action.t2_ctrl = 2;
		rule->action.t2_ctrl_vld = 1;
		rule->action.t3_ctrl = T3_CTRL_BYPASS;
		rule->action.t3_ctrl_vld = 1;
		rule->action.t4_ctrl = T4_CTRL_BYPASS;
		rule->action.t4_ctrl_vld = 1;
		rule->action.t5_ctrl = T5_CTRL_BYPASS;
		rule->action.t5_ctrl_vld = 1;
		rule->action.stage2_ctrl = STAGE2_CTRL_UPDATE;
		rule->action.stage2_ctrl_vld = 1;
		rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
		rule->action.stage3_ctrl_vld = 1;
		rule->action.deepq = 0;
		rule->action.no_drop = 1;
		rule->action.no_drop_vld = 1;
	}

	/* Handle protocol stack. Available case:
	 * - DSLite over Ethernet
	 * - DSLite over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: DSLite can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: DSLite doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_6rd(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! 6RD must be the top protocol stack.\n", __func__);
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	if ((tcfg->src_addr.ip_addr.ipv4_addr & 0xE0000000) == 0xE0000000) {
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
		/* In G3Lite, downstream MC packet comes from L2FE.
		 * In other SoC, downstrema MC packet comes from physical port directly.
		 */
		rule->key.lspid = wan_port_id;
#endif
		rule->key.ip_da_mc = 1;
		rule->key.msk_ip_da_mc = 0;

		/* intf->mac is not Multicast MAC, so don't check MAC_DA */
		rule->key.mac_da_an_sel = 0;
		rule->key.msk_mac_da_an_sel = 1;
	}
	/* Must set key.ip_da instead of key.ip_addr; otherwise, there is no suitable CLS key type. */
	rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv4_addr;
	rule->key.msk_ip_da_l = intf->ip_addr.addr_len;
	rule->key.ip_protocol = IPPROTO_IPV6;
	rule->key.msk_ip_protocol = 0;

	/* action */

	rule->action.pop_l3_en = 1;
	rule->action.pop_l3_vld = 1;

	if (tcfg->tunnel.six_rd.six_rd_ecn_check_enable == CA_TRUE) {
		rule->action.pop_l3_chk_ecn_en = 1;
	}

	if ((tcfg->src_addr.ip_addr.ipv4_addr & 0xE0000000) == 0xE0000000) {
		/* Must ensure that ldpid / t#_ctrl / stage#_ctrl are same as the default MC rule in CLS. */
		/* NOTE: These settings are controllled by reg L3FE_STG1_SIXRD_IPMC_FWD_CTRL instead of CLS FIB, but still keep code here. */
		rule->action.t2_ctrl = 2;
		rule->action.t2_ctrl_vld = 1;
		rule->action.t3_ctrl = T3_CTRL_BYPASS;
		rule->action.t3_ctrl_vld = 1;
		rule->action.t4_ctrl = T4_CTRL_BYPASS;
		rule->action.t4_ctrl_vld = 1;
		rule->action.t5_ctrl = T5_CTRL_BYPASS;
		rule->action.t5_ctrl_vld = 1;
		rule->action.stage2_ctrl = STAGE2_CTRL_UPDATE;
		rule->action.stage2_ctrl_vld = 1;
		rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
		rule->action.stage3_ctrl_vld = 1;
		rule->action.deepq = 0;
		rule->action.no_drop = 1;
		rule->action.no_drop_vld = 1;
	}

	/* Handle protocol stack. Available case:
	 * - 6RD over Ethernet
	 * - 6RD over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: 6RD can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: 6RD doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_map_e(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-E must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	/* Must set key.ip_da instead of key.ip_addr; otherwise, there is no suitable CLS key type. */
	rule->key.ip_ver = 1;
	rule->key.msk_ip_ver = 0;
	rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv6_addr[3];
	rule->key.ip_da.ip_addr_1 = tcfg->src_addr.ip_addr.ipv6_addr[2];
	rule->key.ip_da.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1];
	rule->key.ip_da.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0]; // MSB
	rule->key.msk_ip_da_l = 0;
	rule->key.msk_ip_da_h = 1; // ip_addr len is 128
	rule->key.ip_protocol = IPPROTO_IPIP;
	rule->key.msk_ip_protocol = 0;

	/* action */

	rule->action.pop_l3_en = 1;
	rule->action.pop_l3_vld = 1;

#if 0
	if (tcfg->tunnel.dslite.dsl_ecn_check_enable == CA_TRUE) {
		rule->action.pop_l3_chk_ecn_en = 1;
	}
#endif

	/* Handle protocol stack. Available case:
	 * - MAP-E over Ethernet
	 * - MAP-E over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-E can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-E doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_map_t(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;
	tunnel_info_t tinfo;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-T must be the top protocol stack.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* key */

	/* Must set key.ip_da instead of key.ip_addr; otherwise, there is no suitable CLS key type. */
	rule->key.ip_ver = 1;
	rule->key.msk_ip_ver = 0;
	rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv6_addr[3];
	rule->key.ip_da.ip_addr_1 = tcfg->src_addr.ip_addr.ipv6_addr[2];
	rule->key.ip_da.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1];
	rule->key.ip_da.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0]; // MSB
	rule->key.msk_ip_da_l = 0;
	rule->key.msk_ip_da_h = 1; // ip_addr len is 128

	/* action */

	rule->action.mcgid = downstream_port_id;
	rule->action.mc = 0;
	rule->action.permit = 1;
	rule->action.dpid_pri = 1;
	rule->action.dpid_vld = 1;
	rule->action.t2_ctrl = T2_CTRL_BYPASS;
	rule->action.t2_ctrl_vld = 1;
	rule->action.t3_ctrl = T3_CTRL_BYPASS;
	rule->action.t3_ctrl_vld = 1;
	rule->action.t4_ctrl = T4_CTRL_BYPASS;
	rule->action.t4_ctrl_vld = 1;
	rule->action.t5_ctrl = T5_CTRL_BYPASS;
	rule->action.t5_ctrl_vld = 1;
	rule->action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	rule->action.stage2_ctrl_vld = 1;
	rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	rule->action.stage3_ctrl_vld = 1;

	tinfo.func_id = CA_FUNC_ID_MAP_T;
	tinfo.pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo.direction = CA_TUNNEL_DIR_INBOUND;

	rule->action.mdata_w_2 = ((map_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->idx4pe & 0xFFFF;
	rule->action.mdata_w_vld_2 = 0xF;
	rule->action.mdata_w_3 = tinfo.value;
	rule->action.mdata_w_vld_3 = 0x3;

	/* Handle protocol stack. Available case:
	 * - MAP-T over Ethernet
	 * - MAP-T over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-T can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_intf_to_cls_pppoe(&parent_intf, rule, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-T doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls_pptp(ca_l3_intf_t *intf, l3_cls_rule_t *rule, int depth_protocol_stack)
{
	ca_pptp_sa_t pptp_sa_decrypt;
	ca_status_t ret;
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;
	tunnel_info_t tinfo;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	if ((ret = ca_pptp_sa_get(0, tcfg->tunnel.pptp.sa_id_inbound, &pptp_sa_decrypt)) != CA_E_OK) {
		return CA_E_PARAM;
	}

	/* key */

	// TODO: treat call_id as key; need to use L3CAM.SPCL_HDR
	rule->key.ip_protocol = IPPROTO_GRE;
	rule->key.msk_ip_protocol = 0;

	/* action */

	rule->action.mcgid = downstream_port_id;
	rule->action.mc = 0;
	rule->action.permit = 1;
	rule->action.dpid_pri = 1;
	rule->action.dpid_vld = 1;
	rule->action.t2_ctrl = T2_CTRL_BYPASS;
	rule->action.t2_ctrl_vld = 1;
	rule->action.t3_ctrl = T3_CTRL_BYPASS;
	rule->action.t3_ctrl_vld = 1;
	rule->action.t4_ctrl = T4_CTRL_BYPASS;
	rule->action.t4_ctrl_vld = 1;
	rule->action.t5_ctrl = T5_CTRL_BYPASS;
	rule->action.t5_ctrl_vld = 1;
	rule->action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	rule->action.stage2_ctrl_vld = 1;
	rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	rule->action.stage3_ctrl_vld = 1;

	tinfo.func_id = CA_FUNC_ID_PPTP;
	tinfo.pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo.direction = CA_IPSEC_SPD_DECRYPT;

	rule->action.mdata_w_2 = tcfg->tunnel.pptp.sa_id_inbound & 0xFFFF;
	rule->action.mdata_w_vld_2 = 0xF;
	rule->action.mdata_w_3 = tinfo.value;
	rule->action.mdata_w_vld_3 = 0x3;

	/* Handle protocol stack. Available case:
	 * - PPTP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_intf_to_cls_broadcast(&parent_intf, rule, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPTP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

static ca_status_t convert_intf_to_cls(
	ca_l3_intf_t *intf,
	unsigned int *profile_id,
	ca_uint32_t mac_idx,
	l3_cls_rule_t *rule)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_status_t ret;

	aal_l3_cls_key_clean(&rule->key);
	memset(&rule->action, 0, sizeof(cls_action_t));

	if (intf->type == CA_L3_INTF_TUNNEL)
		tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	/* profile */
	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
		*profile_id = l3fe_classifier_profile_wan;
	else
		*profile_id = l3fe_classifier_profile_lan;

	switch (intf->type) {
	case CA_L3_INTF_BCAST:
		rule->pri = CL_RUL_PRIO_L3_INTF_BCAST;
		break;
	case CA_L3_INTF_P2P:
		rule->pri = CL_RUL_PRIO_L3_INTF_P2P;
		break;
	case CA_L3_INTF_TUNNEL:
		if (tcfg->type == CA_TUNNEL_PPPOE)
			rule->pri = CL_RUL_PRIO_L3_INTF_TUNNEL_PPPOE;
		else
			rule->pri = CL_RUL_PRIO_L3_INTF_TUNNEL;
		break;
	case CA_L3_INTF_CPU:
		rule->pri = CL_RUL_PRIO_L3_INTF_CPU;
		break;
	case CA_L3_INTF_LB:
		rule->pri = CL_RUL_PRIO_L3_INTF_LB;
		break;
	}
	rule->rslt_type = CL_RSLT_TYPE_0;

	/* key */

	if (intf->mask.mac_addr) {
		rule->key.mac_da_an_sel = mac_idx + 1;
		rule->key.msk_mac_da_an_sel = 0;
	}

	switch (intf->type) {
	case CA_L3_INTF_BCAST:
	case CA_L3_INTF_P2P:
		if (intf->mask.mac_addr == 1) {
			/* for UC with routing mac: the lspid is 0x18
			 * for UC but not with routing mac: the lspid is 0x7
			 */
			if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
				rule->key.lspid = AAL_LPORT_L3_WAN;
			else
				rule->key.lspid = PORT_ID(intf->port_id);
			rule->key.msk_lspid = 0;
		}

		rule->key.vlan_cnt = 0; /* decide later; maybe don't care vlan_cnt */
		if (intf->mask.outer_vid != 0) {
			rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

			if (intf->outer_vid >= 0 && intf->outer_vid <= 4095) {
				ca_uint8_t index;

				rule->key.vlan_cnt++;

				rule->key.top_vid = intf->outer_vid;
				rule->key.msk_top_vid = 0;

				if (aal_l3fe_pp_top_tpid_get(intf->outer_tpid, &index) == AAL_E_OK) {
					rule->key.top_tpid_enc = index;
					rule->key.msk_top_tpid_enc = 0;
				} else {
					ca_printf(KERN_ERR "%s: Fail to find outer_tpid 0x%04X setting.\n", __func__, intf->outer_tpid);
					return CA_E_NOT_FOUND;
				}
			}
		}
		if (intf->mask.inner_vid != 0) {
			rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

			if (intf->inner_vid >= 0 && intf->inner_vid <= 4095) {
				ca_uint8_t index;

				rule->key.vlan_cnt++;

				rule->key.inner_vid = intf->inner_vid;
				rule->key.msk_inner_vid = 0;

				if (aal_l3fe_pp_inner_tpid_get(intf->inner_tpid, &index) == AAL_E_OK) {
					rule->key.inner_tpid_enc = index;
					rule->key.msk_inner_tpid_enc = 0;
				} else {
					ca_printf(KERN_ERR "%s: Fail to find inner_tpid 0x%04X setting.\n", __func__, intf->inner_tpid);
					return CA_E_NOT_FOUND;
				}
			}
		}

		if (intf->mask.ip_addr) {
			if (intf->ip_addr.afi == CA_IPV4) {
				rule->key.ip_ver = 0;
				if (intf->ip_addr.addr_len != 0) {
					rule->key.ip_addr.ip_addr_0 = intf->ip_addr.ip_addr.ipv4_addr;
					rule->key.msk_ip_addr_l = intf->ip_addr.addr_len;
					rule->key.ip_addr_type = 0; /* destination address */
				}
			} else {
				rule->key.ip_ver = 1;
				if (intf->ip_addr.addr_len != 0) {
					rule->key.ip_addr.ip_addr_0 = intf->ip_addr.ip_addr.ipv6_addr[3];
					rule->key.ip_addr.ip_addr_1 = intf->ip_addr.ip_addr.ipv6_addr[2];
					rule->key.ip_addr.ip_addr_2 = intf->ip_addr.ip_addr.ipv6_addr[1];
					rule->key.ip_addr.ip_addr_3 = intf->ip_addr.ip_addr.ipv6_addr[0]; // MSB
					rule->key.msk_ip_addr_l = intf->ip_addr.addr_len & 0x7F;
					rule->key.msk_ip_addr_h = intf->ip_addr.addr_len >> 7;
					rule->key.ip_addr_type = 0; /* destination address */
				}
			}
			rule->key.msk_ip_ver = 0;
		}

		/* Assume this is not PPPoE interface; otherwise, it will be overwritten later */
		rule->key.pppoe_session_id = 0;
		rule->key.msk_pppoe_session = 0;

		break;

	case CA_L3_INTF_CPU:
	case CA_L3_INTF_LB:
		rule->key.vlan_cnt = 0; /* decide later; maybe don't care vlan_cnt */
		if (intf->mask.outer_vid != 0) {
			rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

			if (intf->outer_vid >= 0 && intf->outer_vid <= 4095) {
				ca_uint8_t index;

				rule->key.vlan_cnt++;

				rule->key.top_vid = intf->outer_vid;
				rule->key.msk_top_vid = 0;

				if (aal_l3fe_pp_top_tpid_get(intf->outer_tpid, &index) == AAL_E_OK) {
					rule->key.top_tpid_enc = index;
					rule->key.msk_top_tpid_enc = 0;
				} else {
					ca_printf(KERN_ERR "%s: Fail to find outer_tpid 0x%04X setting.\n", __func__, intf->outer_tpid);
					return CA_E_NOT_FOUND;
				}
			}
		}
		if (intf->mask.inner_vid != 0) {
			rule->key.msk_vlan_cnt = 0; /* must match vlan_cnt exactly */

			if (intf->inner_vid >= 0 && intf->inner_vid <= 4095) {
				ca_uint8_t index;

				rule->key.vlan_cnt++;

				rule->key.inner_vid = intf->inner_vid;
				rule->key.msk_inner_vid = 0;

				if (aal_l3fe_pp_inner_tpid_get(intf->inner_tpid, &index) == AAL_E_OK) {
					rule->key.inner_tpid_enc = index;
					rule->key.msk_inner_tpid_enc = 0;
				} else {
					ca_printf(KERN_ERR "%s: Fail to find inner_tpid 0x%04X setting.\n", __func__, intf->inner_tpid);
					return CA_E_NOT_FOUND;
				}
			}
		}

		if (intf->mask.ip_addr) {
			if (intf->ip_addr.afi == CA_IPV4) {
				rule->key.ip_ver = 0;
				if (intf->ip_addr.addr_len != 0) {
					rule->key.ip_addr.ip_addr_0 = intf->ip_addr.ip_addr.ipv4_addr;
					rule->key.msk_ip_addr_l = intf->ip_addr.addr_len;
					rule->key.ip_addr_type = 0; /* destination address */
				}
			} else {
				rule->key.ip_ver = 1;
				if (intf->ip_addr.addr_len != 0) {
					rule->key.ip_addr.ip_addr_0 = intf->ip_addr.ip_addr.ipv6_addr[3];
					rule->key.ip_addr.ip_addr_1 = intf->ip_addr.ip_addr.ipv6_addr[2];
					rule->key.ip_addr.ip_addr_2 = intf->ip_addr.ip_addr.ipv6_addr[1];
					rule->key.ip_addr.ip_addr_3 = intf->ip_addr.ip_addr.ipv6_addr[0]; // MSB
					rule->key.msk_ip_addr_l = intf->ip_addr.addr_len & 0x7F;
					rule->key.msk_ip_addr_h = intf->ip_addr.addr_len >> 7;
					rule->key.ip_addr_type = 0; /* destination address */
				}
			}
			rule->key.msk_ip_ver = 0;
		}

		break;

 	case CA_L3_INTF_TUNNEL:
		/* handle this later */
		break;
	}

	/* action */

	if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) ||
	    (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN)) {
		/* WAN-to-LAN */
		rule->action.t2_ctrl = l3fe_main_hash_profile_wan;
		rule->action.t2_ctrl_vld = 1;
		rule->action.t3_ctrl = 0;
		rule->action.t3_ctrl_vld = 1;
		rule->action.t4_ctrl = l3fe_hash_lite_profile;
		rule->action.t4_ctrl_vld = 1;
		rule->action.t5_ctrl = l3fe_t5_ctrl_support ? T5_CTRL_ENABLE : T5_CTRL_BYPASS;
		rule->action.t5_ctrl_vld = 1;

		rule->action.stage2_ctrl = (intf->mask.nat_enable) ? ((intf->nat_enable) ? STAGE2_CTRL_UPDATE : STAGE2_CTRL_BYPASS) : STAGE2_CTRL_BYPASS;
		rule->action.stage2_ctrl_vld = 1;
		rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS; /* will be overwritten by egress IF */
		rule->action.stage3_ctrl_vld = 1;

	} else if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
		/* LAN-to-WAN */
		rule->action.t2_ctrl = l3fe_main_hash_profile_lan;
		rule->action.t2_ctrl_vld = 1;
		rule->action.t3_ctrl = 0;
		rule->action.t3_ctrl_vld = 1;
		rule->action.t4_ctrl = l3fe_hash_lite_profile;
		rule->action.t4_ctrl_vld = 1;
		rule->action.t5_ctrl = T5_CTRL_BYPASS;
		rule->action.t5_ctrl_vld = 1;

		rule->action.stage2_ctrl = (intf->mask.nat_enable) ? ((intf->nat_enable) ? STAGE2_CTRL_UPDATE : STAGE2_CTRL_BYPASS) : STAGE2_CTRL_BYPASS;
		rule->action.stage2_ctrl_vld = 1;
		rule->action.stage3_ctrl = STAGE3_CTRL_BYPASS; /* will be overwritten by egress IF */
		rule->action.stage3_ctrl_vld = 1;
	}

	rule->action.mdata_w_1 = (intf->intf_id & 0x003F) << 8;
	rule->action.mdata_w_vld_1 = 0xC;

	if (ip_ecn_support) {
		rule->action.ip_ecn_en = 1;
		rule->action.ip_ecn_vld = 1;
	}

	/* handle tunnel-specific keys & actions here */
	if (intf->type == CA_L3_INTF_TUNNEL) {
		if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
			rule->key.lspid = AAL_LPORT_L3_WAN;
		else
			rule->key.lspid = PORT_ID(intf->port_id);
		rule->key.msk_lspid = 0;

#if 0 // TBD: Tunnel interface shouldn't have VID directly. Its parent interface (BCAST-type) has.
		if (intf->mask.outer_vid != 0) {
			if (intf->outer_vid != CA_UINT16_INVALID) {
				rule->key.top_vid = intf->outer_vid;
				rule->key.msk_top_vid = 0;
			}
			rule->key.vlan_cnt = (intf->outer_vid >= 0 && intf->outer_vid <= 4095) ? 1 : 0;
			rule->key.msk_vlan_cnt = 0;
		}
#endif

		if (intf->ip_addr.afi == CA_IPV4)
			rule->key.ip_ver = 0;
		else
			rule->key.ip_ver = 1;
		rule->key.msk_ip_ver = 0;

		if ((tcfg->type == CA_TUNNEL_IPSEC) ||
		    (tcfg->type == CA_TUNNEL_L2TP) ||
		    (tcfg->type == CA_TUNNEL_PPTP) ||
		    (tcfg->type == CA_TUNNEL_4_IN_4) ||
		    (tcfg->type == CA_TUNNEL_6_IN_6) ) {
			if (intf->ip_addr.afi == CA_IPV4) {
				rule->key.ip_sa.ip_addr_0 = tcfg->dest_addr.ip_addr.ipv4_addr;
				rule->key.msk_ip_sa_l = 32;
				rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv4_addr;
				rule->key.msk_ip_da_l = 32;
			} else {
				rule->key.ip_sa.ip_addr_0 = tcfg->dest_addr.ip_addr.ipv6_addr[3];
				rule->key.ip_sa.ip_addr_1 = tcfg->dest_addr.ip_addr.ipv6_addr[2];
				rule->key.ip_sa.ip_addr_2 = tcfg->dest_addr.ip_addr.ipv6_addr[1];
				rule->key.ip_sa.ip_addr_3 = tcfg->dest_addr.ip_addr.ipv6_addr[0]; // MSB
				rule->key.msk_ip_sa_l = 0;
				rule->key.msk_ip_sa_h = 1; // ip_addr len is 128
				rule->key.ip_da.ip_addr_0 = tcfg->src_addr.ip_addr.ipv6_addr[3];
				rule->key.ip_da.ip_addr_1 = tcfg->src_addr.ip_addr.ipv6_addr[2];
				rule->key.ip_da.ip_addr_2 = tcfg->src_addr.ip_addr.ipv6_addr[1];
				rule->key.ip_da.ip_addr_3 = tcfg->src_addr.ip_addr.ipv6_addr[0]; // MSB
				rule->key.msk_ip_da_l = 0;
				rule->key.msk_ip_da_h = 1; // ip_addr len is 128
			}
		}
		if (tcfg->type == CA_TUNNEL_PPPOE) {
			if ((ret = convert_intf_to_cls_pppoe(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_L2TP) {
			if ((ret = convert_intf_to_cls_l2tp(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_IPSEC) {
			if ((ret = convert_intf_to_cls_ipsec(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_PPTP) {
			if ((ret = convert_intf_to_cls_pptp(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_4_IN_4) {

		}
		if (tcfg->type == CA_TUNNEL_DSLITE) {
			if ((ret = convert_intf_to_cls_dslite(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_6RD) {
			if ((ret = convert_intf_to_cls_6rd(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_6_IN_6) {

		}
		if (tcfg->type == CA_TUNNEL_MAP_E) {
			if ((ret = convert_intf_to_cls_map_e(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
		if (tcfg->type == CA_TUNNEL_MAP_T) {
			if ((ret = convert_intf_to_cls_map_t(intf, rule, 0)) != CA_E_OK)
				return ret;
		}
	}

	return CA_E_OK;
}

/* Validate intf parameters */
static ca_status_t intf_validate(ca_l3_intf_t *intf)
{
	ca_tunnel_cfg_t *tcfg = NULL;

	if (intf->mask.port_id) {
		if ((intf->type == CA_L3_INTF_CPU) &&
				((PORT_ID(intf->port_id) < AAL_LPORT_CPU_0) ||
				 (PORT_ID(intf->port_id) > AAL_LPORT_CPU_7))) {
			ca_printf("%s: invalid port id %u\n", __func__, PORT_ID(intf->port_id));
			return CA_E_INTF;
		}
	} else {
		return CA_E_PARAM;
	}

	if (intf->mask.mac_addr == 0) {
		memset(intf->mac_addr, 0, CA_ETH_ADDR_LEN);
	}

	if (intf->mask.outer_vid) {
		if ((intf->outer_vid != CA_UINT16_INVALID) && (intf->outer_vid > 4095)) {
			 ca_printf("%s: invalid outer_vid %u\n", __func__, intf->outer_vid);
			 return CA_E_INTF;
		}

		if (intf->mask.outer_tpid == 0)
			intf->outer_tpid = 0x8100;
	} else {
		intf->outer_vid = CA_UINT16_INVALID;

		if (intf->mask.outer_tpid)
			return CA_E_PARAM;
		else
			intf->outer_tpid = 0;
	}

	if (intf->mask.inner_vid) {
		if (intf->mask.outer_vid == 0)
			return CA_E_PARAM;

		if ((intf->inner_vid != CA_UINT16_INVALID) && (intf->inner_vid > 4095)) {
			 ca_printf("%s: invalid inner_vid %u\n", __func__, intf->inner_vid);
			 return CA_E_INTF;
		}

		if (intf->mask.inner_tpid == 0)
			intf->inner_tpid = 0x8100;
	} else {
		intf->inner_vid = CA_UINT16_INVALID;

		if (intf->mask.inner_tpid)
			return CA_E_PARAM;
		else
			intf->inner_tpid = 0;
	}


	if (intf->mask.tunnel_id == 0)
		intf->tunnel_id = 0;

	if (intf->mask.mtu == 0)
		intf->mtu = 0;

	if (intf->mask.ip_addr == 0) {
		memset(&intf->ip_addr, 0, sizeof(intf->ip_addr));
	} else {
		if (intf->ip_addr.addr_len == 0)
			memset(&intf->ip_addr.ip_addr, 0, sizeof(intf->ip_addr.ip_addr));
	}

	if (intf->mask.nat_enable == 0)
		intf->nat_enable = 0;

	if (intf->type == CA_L3_INTF_TUNNEL) {
		if (intf->mask.tunnel_id == 0)
			return CA_E_PARAM;

		if (intf->tunnel_id >= CA_TUNNEL_MAX_LIMIT)
			return CA_E_PARAM;

		if ((g_tunnel[intf->tunnel_id] == NULL) || (g_tunnel[intf->tunnel_id]->used == CA_FALSE))
			return CA_E_PARAM;

		tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

		/* afi is required */
		if (intf->mask.ip_addr == 0)
			return CA_E_PARAM;

		if (tcfg->type == CA_TUNNEL_PPPOE) {

		}

		if (tcfg->type == CA_TUNNEL_L2TP) {

		}

		if (tcfg->type == CA_TUNNEL_IPSEC) {

		}

		if (tcfg->type == CA_TUNNEL_PPTP) {

		}

		if (tcfg->type == CA_TUNNEL_4_IN_4) {
			if (intf->ip_addr.afi != CA_IPV4)
				return CA_E_CONFLICT;
		}

		if (tcfg->type == CA_TUNNEL_DSLITE) {
			if (intf->ip_addr.afi != CA_IPV6)
				return CA_E_CONFLICT;
		}

		if (tcfg->type == CA_TUNNEL_6RD) {
			if (intf->ip_addr.afi != CA_IPV4)
				return CA_E_CONFLICT;
		}

		if (tcfg->type == CA_TUNNEL_6_IN_6) {
			if (intf->ip_addr.afi != CA_IPV6)
				return CA_E_CONFLICT;
		}

		if (tcfg->type == CA_TUNNEL_MAP_E) {
			/* Overwrite intf->ip_addr with CE IPv4 addr (in tunnel_cfg->tunnel.map.ipv4_prefix). So caller can ignore intf->ip_addr. */
			intf->ip_addr = tcfg->tunnel.map.ipv4_prefix;
		}

		if (tcfg->type == CA_TUNNEL_MAP_T) {
			/* Overwrite intf->ip_addr with CE IPv4 addr (in tunnel_cfg->tunnel.map.ipv4_prefix). So caller can ignore intf->ip_addr. */
			intf->ip_addr = tcfg->tunnel.map.ipv4_prefix;
		}
	}

	if ((intf->type == CA_L3_INTF_BCAST) || (intf->type == CA_L3_INTF_P2P)) {
		if (intf->mask.tunnel_id != 0)
			return CA_E_PARAM;
	}

	if ((intf->type == CA_L3_INTF_CPU) || (intf->type == CA_L3_INTF_LB)) {
		if (intf->mask.tunnel_id != 0)
			return CA_E_PARAM;

		if (intf->mask.ip_addr == 0)
			return CA_E_PARAM;
	}

	return CA_E_OK;
}

/* Check if intf1 and intf2 causes same CLS key */
static ca_status_t intf_tunnel_cls_key_check(ca_l3_intf_t *intf1, ca_l3_intf_t *intf2)
{
	ca_tunnel_cfg_t *tcfg1 = NULL, *tcfg2 = NULL;

	if ((intf1->type != CA_L3_INTF_TUNNEL) || (intf2->type != CA_L3_INTF_TUNNEL))
		return CA_E_PARAM;

	if ((intf1->tunnel_id >= CA_TUNNEL_MAX_LIMIT) || (intf2->tunnel_id >= CA_TUNNEL_MAX_LIMIT))
		return CA_E_PARAM;

	tcfg1 = &g_tunnel[intf1->tunnel_id]->tcfg;
	tcfg2 = &g_tunnel[intf2->tunnel_id]->tcfg;

	if ((tcfg1 == NULL) || (tcfg2 == NULL))
		return CA_E_PARAM;

	/* This function only checks CLS keys of same tunnel type */
	if (tcfg1->type != tcfg2->type)
		return CA_E_OK;

	if (tcfg1->type == CA_TUNNEL_PPPOE) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (intf1->ip_addr.afi == intf2->ip_addr.afi) &&
		    (tcfg1->tunnel.pppoe.pppoe_session_id == tcfg2->tunnel.pppoe.pppoe_session_id))
			return CA_E_CONFLICT;
	}

	if (tcfg1->type == CA_TUNNEL_L2TP) {

	}

	if (tcfg1->type == CA_TUNNEL_IPSEC) {

	}

	if (tcfg1->type == CA_TUNNEL_PPTP) {

	}

	if (tcfg1->type == CA_TUNNEL_4_IN_4) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (memcmp(&intf1->ip_addr, &intf2->ip_addr, sizeof(ca_ip_address_t)) == 0) )
			return CA_E_CONFLICT;
	}

	if (tcfg1->type == CA_TUNNEL_DSLITE) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (memcmp(&intf1->ip_addr, &intf2->ip_addr, sizeof(ca_ip_address_t)) == 0) )
			return CA_E_CONFLICT;
	}

	if (tcfg1->type == CA_TUNNEL_6RD) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (memcmp(&intf1->ip_addr, &intf2->ip_addr, sizeof(ca_ip_address_t)) == 0) )
			return CA_E_CONFLICT;
	}

	if (tcfg1->type == CA_TUNNEL_6_IN_6) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (memcmp(&intf1->ip_addr, &intf2->ip_addr, sizeof(ca_ip_address_t)) == 0) )
			return CA_E_CONFLICT;
	}

#if 0
	if (tcfg1->type == CA_TUNNEL_MAP_E) {
		if ((intf1->port_id == intf2->port_id) &&
		    (intf1->outer_vid == intf2->outer_vid) &&
		    (intf1->inner_vid == intf2->inner_vid) &&
		    (memcmp(&intf1->ip_addr, &intf2->ip_addr, sizeof(ca_ip_address_t)) == 0) )
			return CA_E_CONFLICT;
	}
#endif

	return CA_E_OK;
}

void intf_table_dump(void)
{
	int idx;

	ca_printf(KERN_INFO "DUMP intf_table:\n");

	for (idx = 0; idx < FE_L3E_IF_TBL_ENTRY_MAX; idx++) {
		if (ca_l3_intf_tbl[idx].use == 0) continue;
		ca_printf(KERN_INFO "-----------------------------------------------------------------\n");
		ca_printf(KERN_INFO "INTF data:\n");
		ca_l3_intf_dump(&ca_l3_intf_tbl[idx].intf);

		ca_printf(KERN_INFO "Internal data:\n");
		ca_printf(KERN_INFO "  mtu_idx = %d\n", ca_l3_intf_tbl[idx].mtu_idx);
		ca_printf(KERN_INFO "  cls_profile = %u\n", ca_l3_intf_tbl[idx].cls_profile);
		ca_printf(KERN_INFO "  cls_idx = %u\n", ca_l3_intf_tbl[idx].cls_idx);
		ca_printf(KERN_INFO "    entry_idx = %u\n", CLS_TBL_ID(ca_l3_intf_tbl[idx].cls_idx));
		ca_printf(KERN_INFO "    key_idx_in_one_entry = %u\n", CLS_KEY_ID(ca_l3_intf_tbl[idx].cls_idx));
		ca_printf(KERN_INFO "    fib_type = %u\n", CLS_KEY_TYPE(ca_l3_intf_tbl[idx].cls_idx));
		ca_printf(KERN_INFO "    fib_mode = %u\n", CLS_FIB_MODE(ca_l3_intf_tbl[idx].cls_idx));
		ca_printf(KERN_INFO "  hashlite_idx = %u\n", ca_l3_intf_tbl[idx].hashlite_idx);
		ca_printf(KERN_INFO "  hashlite_idx_valid = %u\n", ca_l3_intf_tbl[idx].hashlite_idx_valid);
		ca_printf(KERN_INFO "  lpm_idx = %u\n", ca_l3_intf_tbl[idx].lpm_idx);
		ca_printf(KERN_INFO "  lpm_idx_valid = %u\n", ca_l3_intf_tbl[idx].lpm_idx_valid);
	}
	ca_printf(KERN_INFO "=================================================================\n");
}

/***************************************************************************
 * route table & internal functions
 ***************************************************************************/

static LIST_HEAD(route_table);
static DEFINE_RWLOCK(route_table_lock);

static int route_node_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct route_node *na = list_entry(a, struct route_node, list);
	struct route_node *nb = list_entry(b, struct route_node, list);

	if (na->lpm_idx > nb->lpm_idx) return 1;
	if (na->lpm_idx < nb->lpm_idx) return -1;
	return 0;
}

/*
 * Convert necessary data from ca_l3_route_t to lpm_tbl_entry_t.
 * lpm_tbl_entry_t::rst_ctrl is unknown from ca_l3_route_t.
 * entry must be an array of at least 4 elements, and contains all zero.
 */
static void convert_route_to_entry(ca_l3_route_t *route, lpm_tbl_entry_t *entry, uint32_t rst_idx)
{
	entry[0].profile = 0; /* LPM profile#0 */

	if (route->prefix.afi == CA_IPV4) {
		entry[0].valid = 1;
		entry[0].attr = ATTR_IPV4;
		entry[0].mask = route->prefix.addr_len;
		entry[0].data = route->prefix.ip_addr.ipv4_addr & (0xffffffffULL << (32 - route->prefix.addr_len));
		entry[0].rst_idx = rst_idx;
	}

	if (route->prefix.afi == CA_IPV6) {
		if (route->prefix.addr_len <= 32) {
			entry[0].valid = 1;
			entry[0].attr = ATTR_IPV6_LEN_0_32;
			entry[0].mask = route->prefix.addr_len;
			entry[0].data = route->prefix.ip_addr.ipv6_addr[0] & (0xffffffffULL << (32 - route->prefix.addr_len));
			entry[0].rst_idx = rst_idx;
		} else if ((route->prefix.addr_len > 32) && (route->prefix.addr_len <= 64)) {
			entry[1].valid = 1;
			entry[1].attr = ATTR_IPV6_LEN_33_64;
			entry[1].mask = 32;
			entry[1].data = route->prefix.ip_addr.ipv6_addr[0];

			entry[0].valid = 1;
			entry[0].attr = ATTR_IPV6_LEN_33_64;
			entry[0].mask = route->prefix.addr_len - 32;
			entry[0].data = route->prefix.ip_addr.ipv6_addr[1] & (0xffffffffULL << (64 - route->prefix.addr_len));
			entry[0].rst_idx = rst_idx;
		} else {
			entry[3].valid = 1;
			entry[3].attr = ATTR_IPV6_LEN_65_128;
			entry[3].mask = 32;
			entry[3].data = route->prefix.ip_addr.ipv6_addr[0];

			entry[2].valid = 1;
			entry[2].attr = ATTR_IPV6_LEN_65_128;
			entry[2].mask = 32;
			entry[2].data = route->prefix.ip_addr.ipv6_addr[1];

			entry[1].valid = 1;
			entry[1].attr = ATTR_IPV6_LEN_65_128;
			entry[0].valid = 1;
			entry[0].attr = ATTR_IPV6_LEN_65_128;
			if (route->prefix.addr_len < 96) {
				entry[1].mask = route->prefix.addr_len - 64;
				entry[1].data = route->prefix.ip_addr.ipv6_addr[2] & (0xffffffffULL << (96 - route->prefix.addr_len));
				entry[0].mask = 0;
				entry[0].data = 0;
			} else {
				entry[1].mask = 32;
				entry[1].data = route->prefix.ip_addr.ipv6_addr[2];
				entry[0].mask = route->prefix.addr_len - 96;
				entry[0].data = route->prefix.ip_addr.ipv6_addr[3] & (0xffffffffULL << (128 - route->prefix.addr_len));
			}
			entry[0].rst_idx = rst_idx;
		}
	}
}

#if 0
/*
 * Convert necessary data from lpm_tbl_entry_t to ca_l3_route_t.
 */
static void convert_entry_to_route(lpm_tbl_entry_t *entry, ca_l3_route_t *route)
{
	memset(route, 0, sizeof(ca_l3_route_t));

	route->nexthop_id = entry[0].rst_idx;

	if (entry[0].attr == ATTR_IPV4) {
		route->prefix.afi = CA_IPV4;
		route->prefix.addr_len = entry[0].mask;
		route->prefix.ip_addr.ipv4_addr = entry[0].data;
	} else if (entry[0].attr == ATTR_IPV6_LEN_0_32) {
		route->prefix.afi = CA_IPV6;
		route->prefix.addr_len = entry[0].mask;
		route->prefix.ip_addr.ipv6_addr[0] = entry[0].data;
	} else if (entry[0].attr == ATTR_IPV6_LEN_33_64) {
		route->prefix.afi = CA_IPV6;
		route->prefix.addr_len = entry[0].mask + entry[1].mask;
		route->prefix.ip_addr.ipv6_addr[0] = entry[1].data;
		route->prefix.ip_addr.ipv6_addr[1] = entry[0].data;
	} else {
		route->prefix.afi = CA_IPV6;
		route->prefix.addr_len = entry[0].mask + entry[1].mask + entry[2].mask + entry[3].mask;
		route->prefix.ip_addr.ipv6_addr[0] = entry[3].data;
		route->prefix.ip_addr.ipv6_addr[1] = entry[2].data;
		route->prefix.ip_addr.ipv6_addr[2] = entry[1].data;
		route->prefix.ip_addr.ipv6_addr[3] = entry[0].data;
	}
}
#endif

void route_table_dump(void)
{
	struct route_node *iter;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	ca_printf(KERN_INFO "DUMP route_table:\n");
	ca_printf(KERN_INFO "lpm_idx  prefix                                   addr_len  nexthop_id  rst_idx\n");
	ca_printf(KERN_INFO "-------------------------------------------------------------------------------\n");
	read_lock(&route_table_lock);
	list_for_each_entry(iter, &route_table, list) {
		if (iter->route.prefix.afi == CA_IPV4) {

			ipv4_addr.s_addr = htonl(iter->route.prefix.ip_addr.ipv4_addr);

			ca_printf(KERN_INFO "%4u     %pi4                          %3u       %5u          %4u\n",
				iter->lpm_idx, &ipv4_addr, iter->route.prefix.addr_len, iter->route.nexthop_id, iter->rst_idx);

		} else {

			ipv6_addr.in6_u.u6_addr32[0] = htonl(iter->route.prefix.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(iter->route.prefix.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(iter->route.prefix.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(iter->route.prefix.ip_addr.ipv6_addr[3]);

			ca_printf(KERN_INFO "%4u     %pI6  %3u       %5u          %4d\n",
				iter->lpm_idx, &ipv6_addr, iter->route.prefix.addr_len, iter->route.nexthop_id, iter->rst_idx);

		}
	}
	read_unlock(&route_table_lock);
	ca_printf(KERN_INFO "===============================================================================\n");
}

ca_status_t ca_l3_route_init(void)
{
	ca_status_t ret;

	if ((ret = bm_swonly_nhid_init()) != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to init bm_swonly_nhid. (ret=%u)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

ca_status_t ca_l3_route_exit(void)
{
	bm_swonly_nhid_exit();

	return CA_E_OK;
}

/***************************************************************************
 * nexthop table & internal functions
 ***************************************************************************/

#define NEXTHOP_TYPE_NEIGHBOR	0	/* parameter nexthop->da_mac is not 0 */
#define NEXTHOP_TYPE_GATEWAY	1	/* parameter nexthop->da_mac is 0 */

LIST_HEAD(nexthop_table);
static DEFINE_RWLOCK(nexthop_table_lock);

static int nexthop_node_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct nexthop_node *na = list_entry(a, struct nexthop_node, list);
	struct nexthop_node *nb = list_entry(b, struct nexthop_node, list);

	if (na->nexthop.nexthop_id > nb->nexthop.nexthop_id) return 1;
	if (na->nexthop.nexthop_id < nb->nexthop.nexthop_id) return -1;
	return 0;
}

/* Set the nexthop->addr.addr_len correctly */
static ca_status_t nexthop_addr_check(ca_l3_nexthop_t *nexthop)
{
	if (nexthop == NULL) return CA_E_PARAM;

	if (nexthop->addr.afi == CA_IPV4) {
		/* IPv4, addr_len must be 0 or 32 */
		if (nexthop->addr.ip_addr.ipv4_addr == 0) {
			nexthop->addr.addr_len = 0;
		} else {
			nexthop->addr.ip_addr.ipv6_addr[1] = 0;
			nexthop->addr.ip_addr.ipv6_addr[2] = 0;
			nexthop->addr.ip_addr.ipv6_addr[3] = 0;
			nexthop->addr.addr_len = 32;
		}
	} else {
		/* IPv6, addr_len must be 0 or 128 */
		if (  (nexthop->addr.ip_addr.ipv6_addr[0] == 0)
		    && (nexthop->addr.ip_addr.ipv6_addr[1] == 0)
		    && (nexthop->addr.ip_addr.ipv6_addr[2] == 0)
		    && (nexthop->addr.ip_addr.ipv6_addr[3] == 0) ) {
			nexthop->addr.addr_len = 0;
		} else {
			nexthop->addr.addr_len = 128;
		}
	}

	return CA_E_OK;
}

/* Update the nexthop->aging_time.
 *
 * If nexthop is SW-only entry, aging_time is 0.
 * If nexthop->attr_flags is CA_L3_NHOP_ATTR_ROUTER, aging_time is 0.
 * If nexthop->attr_flags is CA_L3_NHOP_ATTR_NEGH,
 */
static ca_status_t nexthop_aging_time_update(struct nexthop_node *node)
{
	aal_hashlite_aging_e age_neighbor = HASHLITE_AGING_INVALID;
	ca_uint32_t time_neighbor = 0;
	unsigned int hash_idx; /* temp hash_idx */
	int ret;

	if (node == NULL) return CA_E_PARAM;

	if ((node->nexthop.nexthop_id >= MIN_BM_SWONLY_NHID) || (node->nexthop.attr_flags == CA_L3_NHOP_ATTR_ROUTER)) {
		node->nexthop.aging_timer = 0;
		return CA_E_OK;
	}

	/* get the age / time of hash_idx_neighbor */
	if (node->hash_idx_neighbor >= 0) {
		hash_idx = node->hash_idx_neighbor & CA_L3_NEXTHOP_HASH_IDX_MASK;
		if (hash_idx & CA_L3_NEXTHOP_ID_OVERFLOW_FLAG) {
			ret = aal_hashlite_age_get(AAL_TABLE_HASHLITE_OVERFLOW_TBL, hash_idx, &age_neighbor, &time_neighbor);
		} else {
			ret = aal_hashlite_age_get(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, &age_neighbor, &time_neighbor);
		}
		if (ret != AAL_E_OK) return CA_E_ERROR;
	}

	node->nexthop.aging_timer = time_neighbor;
	return CA_E_OK;
}

void nexthop_table_dump(void)
{
	struct nexthop_node *iter;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	ca_printf(KERN_INFO "DUMP nexthop_table:\n");
	ca_printf(KERN_INFO "                                                                              aging     intf  hash_idx\n");
	ca_printf(KERN_INFO "nexthop_id  ip_addr                                  da_mac             attr  time      ID    neighbor\n");
	ca_printf(KERN_INFO "------------------------------------------------------------------------------------------------------\n");
	read_lock(&nexthop_table_lock);
	list_for_each_entry(iter, &nexthop_table, list) {
		nexthop_aging_time_update(iter);
		if (iter->nexthop.addr.afi == CA_IPV4) {

			ipv4_addr.s_addr = htonl(iter->nexthop.addr.ip_addr.ipv4_addr);

			ca_printf(KERN_INFO "%5u       %pi4                          %pM  %d     %-5u     %02u    %5d\n",
				iter->nexthop.nexthop_id, &ipv4_addr, iter->nexthop.da_mac,
				iter->nexthop.attr_flags, iter->nexthop.aging_timer, iter->nexthop.intf_id,
				iter->hash_idx_neighbor);

		} else {

			ipv6_addr.in6_u.u6_addr32[0] = htonl(iter->nexthop.addr.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(iter->nexthop.addr.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(iter->nexthop.addr.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(iter->nexthop.addr.ip_addr.ipv6_addr[3]);

			ca_printf(KERN_INFO "%5u       %pI6  %pM  %d     %-5u     %02u    %5d\n",
				iter->nexthop.nexthop_id, &ipv6_addr, iter->nexthop.da_mac,
				iter->nexthop.attr_flags, iter->nexthop.aging_timer, iter->nexthop.intf_id,
				iter->hash_idx_neighbor);

		}
	}
	read_unlock(&nexthop_table_lock);
	ca_printf(KERN_INFO "======================================================================================================\n");
}

ca_status_t convert_nexthop_to_hashlite_broadcast(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, intf_id=%u\n", __func__, depth_protocol_stack, intf->intf_id);
	}

	/* action */
	/* VLAN action is done by egress IF table (or in convert_intf_to_eif_broadcast()) */

	return CA_E_OK;
}

ca_status_t convert_nexthop_to_hashlite_pppoe(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */
	/* PPPoE action is done by egress IF table (or in convert_intf_to_eif_pppoe()) */

	/* Handle protocol stack. Available case:
	 * - PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPPoE doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_ipsec(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	bool is_l2tp_ipsec = false; /* true: L2TPv2/v3 over IPSec. false: IPSec only. */
	int l2tp_ver = 2;
	ca_ipsec_sa_t ipsec_sa_encrypt;
	ca_status_t ret;
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	if ((intf == NULL) || (hash_action == NULL) || (tinfo == NULL))
		return CA_E_PARAM;

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	if ((ret = ca_ipsec_sa_get(0, tcfg->tunnel.ipsec.sa_id_encrypt, &ipsec_sa_encrypt)) != CA_E_OK) {
		return CA_E_PARAM;
	}

	/* check if this is L2TP over IPSec */
	if (depth_protocol_stack != 0) {
		if (tinfo->func_id == CA_FUNC_ID_L2TPV3) {
			is_l2tp_ipsec = true;
			l2tp_ver = 3;
		} else if (tinfo->func_id == CA_FUNC_ID_L2TP) {
			is_l2tp_ipsec = true;
			l2tp_ver = 2;
		} else {
			ca_printf(KERN_INFO "%s: ERROR! L2TP version is unknown. (func_id %u)\n", __func__, tinfo->func_id);
			return CA_E_INTERNAL;
		}
	}

	/* action */

	tinfo->func_id = (is_l2tp_ipsec) ? ((l2tp_ver == 3) ? CA_FUNC_ID_L2TPV3_IPSEC : CA_FUNC_ID_L2TP_IPSEC) : CA_FUNC_ID_IPSEC;
	tinfo->pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo->direction = CA_IPSEC_SPD_ENCRYPT;

	hash_action->mcgid = upstream_port_id;
	hash_action->mc = 0;
	hash_action->permit = 1;
	hash_action->dpid_pri = 1;
	hash_action->dpid_vld = 1;

	hash_action->mdata_w_2 = ((ipsec_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->idx4pe & 0xFFFF;
	hash_action->mdata_w_vld_2 = 0xF;
	hash_action->mdata_w_3 = tinfo->value;
	hash_action->mdata_w_vld_3 = 0x3;

	/* 1st round in L3FE, don't modify MAC_DA; 2nd round need MyMAC to hit */
	hash_action->mac_da_vld = 0;
	hash_action->mac_da_0 = 0;		// LSB
	hash_action->mac_da_1 = 0;
	hash_action->mac_da_2 = 0;
	hash_action->mac_da_3 = 0;
	hash_action->mac_da_4 = 0;
	hash_action->mac_da_5 = 0;		// MSB

	/* Handle protocol stack. Available case:
	 * - IPSec over Ethernet
	 * - IPSec over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: IPSec can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: IPSec doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_l2tp(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	if ((intf == NULL) || (hash_action == NULL) || (tinfo == NULL))
		return CA_E_PARAM;

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	tinfo->func_id = ((tcfg->tunnel.l2tp.version & 0x3) == 3) ? CA_FUNC_ID_L2TPV3 : CA_FUNC_ID_L2TP;
	tinfo->pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo->direction = CA_IPSEC_SPD_ENCRYPT;

	hash_action->mcgid = upstream_port_id;
	hash_action->mc = 0;
	hash_action->permit = 1;
	hash_action->dpid_pri = 1;
	hash_action->dpid_vld = 1;

	hash_action->mdata_w_2 = ((l2tp_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->sa_id_inbound & 0xFFFF;
	hash_action->mdata_w_vld_2 = 0xF;
	hash_action->mdata_w_3 = tinfo->value;
	hash_action->mdata_w_vld_3 = 0x3;

	/* 1st round in L3FE, don't modify MAC_DA; 2nd round need MyMAC to hit */
	hash_action->mac_da_vld = 0;
	hash_action->mac_da_0 = 0;		// LSB
	hash_action->mac_da_1 = 0;
	hash_action->mac_da_2 = 0;
	hash_action->mac_da_3 = 0;
	hash_action->mac_da_4 = 0;
	hash_action->mac_da_5 = 0;		// MSB

	/* Handle protocol stack. Available case:
	 * - L2TP over Ethernet
	 * - L2TP over PPP over Ethernet
	 * - L2TP over IPSec over ...
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type == CA_TUNNEL_IPSEC) {
			return convert_nexthop_to_hashlite_ipsec(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
		} else if (parent_tcfg-> type == CA_TUNNEL_PPPOE) {
			return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
		} else {
			ca_printf(KERN_INFO "%s: L2TP can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: L2TP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_dslite(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! DSLite must be the top protocol stack.\n", __func__);
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	/* Handle protocol stack. Available case:
	 * - DSLite over Ethernet
	 * - DSLite over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: DSLite can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: DSLite doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_6rd(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! 6RD must be the top protocol stack.\n", __func__);
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	if (tcfg->tunnel.six_rd.six_rd_ipda_from_v6 == CA_TRUE) {
		hash_action->six_rd_ipda_ctrl_vld = 1;
		hash_action->six_rd_ipda_from_v6 = 1;
	}

	/* Handle protocol stack. Available case:
	 * - 6RD over Ethernet
	 * - 6RD over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: 6RD can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: 6RD doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_map_e(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-E must be the top protocol stack.\n", __func__);
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	// call IPC to set entry
//	ca_map_e_ipc_entry_set(0, CA_TUNNEL_DIR_OUTBOUND, intf->tunnel_id, sa_id);

	/* Handle protocol stack. Available case:
	 * - MAP-E over Ethernet
	 * - MAP-E over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-E can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-E doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_map_t(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL, *parent_tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack != 0) {
		ca_printf(KERN_INFO "%s: ERROR! MAP-T must be the top protocol stack.\n", __func__);
	}

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	tinfo->func_id = CA_FUNC_ID_MAP_T;
	tinfo->pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo->direction = CA_TUNNEL_DIR_OUTBOUND;

	hash_action->mcgid = upstream_port_id;
	hash_action->mc = 0;
	hash_action->permit = 1;
	hash_action->dpid_pri = 1;
	hash_action->dpid_vld = 1;
	hash_action->ip_ttl_cmd = 0; /* Do not decrease TTL in 1st NE round; it is decreased in 2nd NE round */

	hash_action->mdata_w_2 = ((map_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->idx4pe & 0xFFFF;
	hash_action->mdata_w_vld_2 = 0xF;
	hash_action->mdata_w_3 = tinfo->value;
	hash_action->mdata_w_vld_3 = 0x3;

#if 0
	/* 1st round in L3FE, don't modify MAC_DA; 2nd round need MyMAC to hit */
	hash_action->mac_da_vld = 0;
	hash_action->mac_da_0 = 0;              // LSB
	hash_action->mac_da_1 = 0;
	hash_action->mac_da_2 = 0;
	hash_action->mac_da_3 = 0;
	hash_action->mac_da_4 = 0;
	hash_action->mac_da_5 = 0;              // MSB
#endif

	// call IPC to set entry
//	ca_map_t_ipc_entry_set(0, CA_TUNNEL_DIR_OUTBOUND, intf->tunnel_id, sa_id);

	/* Handle protocol stack. Available case:
	 * - MAP-T over Ethernet
	 * - MAP-T over PPP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_TUNNEL) {
		parent_tcfg = &g_tunnel[parent_intf.tunnel_id]->tcfg;

		if (parent_tcfg->type != CA_TUNNEL_PPPOE) {
			ca_printf(KERN_INFO "%s: MAP-T can't over this tunnel type %d.\n", __func__, parent_tcfg->type);
			return CA_E_PARAM;
		}

		return convert_nexthop_to_hashlite_pppoe(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);

	} else if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: MAP-T doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

ca_status_t convert_nexthop_to_hashlite_pptp(ca_l3_intf_t *intf, hl_act_l3_generic_t *hash_action, tunnel_info_t *tinfo, int depth_protocol_stack)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_l3_intf_t parent_intf;

	if (depth_protocol_stack >= MAX_PROTOCOL_STACK) {
		ca_printf(KERN_INFO "%s: ERROR! Exceeds the protocol stack depth.\n", __func__);
		return CA_E_PARAM;
	}

	if ((intf == NULL) || (hash_action == NULL) || (tinfo == NULL))
		return CA_E_PARAM;

	tcfg = &g_tunnel[intf->tunnel_id]->tcfg;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf("%s: depth_protocol_stack=%d, tunnel_id=%u\n", __func__, depth_protocol_stack, intf->tunnel_id);
		ca_printf("%s: parent_l3_intf_id=%u\n", __func__, tcfg->parent_l3_intf_id);
	}

	/* action */

	tinfo->func_id = CA_FUNC_ID_PPTP;
	tinfo->pkt_type = CA_PKT_TYPE_FROM_NE;
	tinfo->direction = CA_IPSEC_SPD_ENCRYPT;

	hash_action->mcgid = upstream_port_id;
	hash_action->mc = 0;
	hash_action->permit = 1;
	hash_action->dpid_pri = 1;
	hash_action->dpid_vld = 1;

	hash_action->mdata_w_2 = ((pptp_private_data_t *) g_tunnel[intf->tunnel_id]->private_data)->sa_id_inbound & 0xFFFF;
	hash_action->mdata_w_vld_2 = 0xF;
	hash_action->mdata_w_3 = tinfo->value;
	hash_action->mdata_w_vld_3 = 0x3;

	/* 1st round in L3FE, don't modify MAC_DA; 2nd round need MyMAC to hit */
	hash_action->mac_da_vld = 0;
	hash_action->mac_da_0 = 0;		// LSB
	hash_action->mac_da_1 = 0;
	hash_action->mac_da_2 = 0;
	hash_action->mac_da_3 = 0;
	hash_action->mac_da_4 = 0;
	hash_action->mac_da_5 = 0;		// MSB

	/* Handle protocol stack. Available case:
	 * - PPTP over Ethernet
	 */

	if (ca_l3_intf_tbl[tcfg->parent_l3_intf_id].use != 0) {
		memcpy(&parent_intf, &ca_l3_intf_tbl[tcfg->parent_l3_intf_id].intf, sizeof(ca_l3_intf_t));
	} else {
		ca_printf(KERN_INFO "%s: ERROR! parent interface of ID %d doesn't exist.\n", __func__, tcfg->parent_l3_intf_id);
		return CA_E_INTERNAL;
	}

	if (parent_intf.type == CA_L3_INTF_BCAST) {
		return convert_nexthop_to_hashlite_broadcast(&parent_intf, hash_action, tinfo, depth_protocol_stack + 1);
	} else {
		ca_printf(KERN_INFO "%s: PPTP doesn't support the parent intf type %d.\n", __func__, parent_intf.type);
	}

	return CA_E_ERROR;
}

/***************************************************************************
 * Cortina API
 ***************************************************************************/

ca_status_t ca_l3_intf_add (
	CA_IN		ca_device_id_t	device_id,
	CA_OUT		ca_l3_intf_t	*intf)
{
	fe_l3e_if_tbl_entry_t eif;
	ca_status_t rc;
	unsigned int profile_id;
	l3_cls_rule_t rule;
	ca_uint32_t rslt_idx, mac_idx, mtu_idx;
	int ret, i, lpb_tbl_idx;
	ca_l3_intf_t *tmp_intf;
	l3_cam_mac_da_tbl_entry_t l3_cam_mac;
	aal_l3fe_lpb_tbl_cfg_t lpb_tbl;
	aal_l3fe_lpb_tbl_cfg_mask_t mask;
	aal_fdb_entry_cfg_t fdb_entry;

	aal_hash_key_t hash_key;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3_generic_t *hash_action = (hl_act_l3_generic_t *) &hash_action_entry;
	ca_uint32 hashlite_idx, mask_idx;

	ca_l3_route_t route_entry;
	lpm_tbl_entry_t lpm_entry[4];
	unsigned int lpm_idx;

	bool bypass_cls = false; /* bypass creating CLS rule */

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}
	if (intf == NULL) {
		ca_printf("%s: intf param is null\n", __func__);
		return CA_E_PARAM;
	}

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf(KERN_INFO "%s: Dump intf:\n", __func__);
		ca_l3_intf_dump(intf);
	}

	rc = intf_validate(intf);
	if (rc != CA_E_OK) {
		return rc;
	}

	ca_spin_lock(ca_l3_intf_lock);

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
		if (ca_l3_intf_tbl[i].use == 1) {
			tmp_intf = &ca_l3_intf_tbl[i].intf;
			if (((intf->type == CA_L3_INTF_BCAST) || (intf->type == CA_L3_INTF_P2P)) &&
			    ((tmp_intf->type == CA_L3_INTF_BCAST) || (tmp_intf->type == CA_L3_INTF_P2P)) &&
			    (intf->port_id == tmp_intf->port_id) &&
			    (intf->outer_vid == tmp_intf->outer_vid) && (intf->mask.outer_vid == tmp_intf->mask.outer_vid) &&
			    (intf->inner_vid == tmp_intf->inner_vid) && (intf->mask.inner_vid == tmp_intf->mask.inner_vid) &&
			    (intf->mask.ip_addr == tmp_intf->mask.ip_addr) && (intf->ip_addr.afi == tmp_intf->ip_addr.afi) &&
			    (intf->mask.mac_addr == tmp_intf->mask.mac_addr) && (memcmp(intf->mac_addr, tmp_intf->mac_addr, CA_ETH_ADDR_LEN) == 0)) {
				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf("%s: intf_id %d has the same key(port_id/mac/outer_vid/inner_vid/ip_addr.afi) for CLS.\n", __func__, i);
				return CA_E_INTF;
			}

			if (((intf->type == CA_L3_INTF_CPU) || (intf->type == CA_L3_INTF_LB)) && \
			    ((tmp_intf->type == CA_L3_INTF_CPU) || (tmp_intf->type == CA_L3_INTF_LB)) && \
			    (memcmp(&intf->ip_addr, &tmp_intf->ip_addr, sizeof(ca_ip_address_t)) == 0)) {
				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf("%s: intf_id %d has the same key(ip_addr) for cls\n", __func__, i);
				return CA_E_INTF;
			}

			if ((intf->type == CA_L3_INTF_TUNNEL) && (tmp_intf->type == CA_L3_INTF_TUNNEL)) {
				ret = intf_tunnel_cls_key_check(intf, tmp_intf);
				if (ret != CA_E_OK) {
					if ((g_tunnel[intf->tunnel_id]->tcfg.type == CA_TUNNEL_MAP_E) || (g_tunnel[intf->tunnel_id]->tcfg.type == CA_TUNNEL_MAP_T)) {
						/* Workaround.
						 * Ingress MAP-E packets may have same tunnel IPv6DA but different PSID (ports).
						 */
//						bypass_cls = true;
					} else {
						ca_spin_unlock(ca_l3_intf_lock);
						ca_printf("%s: intf_id %d has the same tunnel key for cls\n", __func__, i);
						return CA_E_INTF;
					}
				}
			}
		}
	}

	/* find a suitable free intf */
	rc = ca_l3_intf_alloc(intf);
	if (rc != CA_E_OK) {
		ca_spin_unlock(ca_l3_intf_lock);
		if (rc == CA_E_FULL)
			ca_printf("%s: no suitable free intf\n", __func__);
		return rc;
	}

	//mtu
	if (intf->mask.mtu) {
		rc = aal_l3pe_mtu_size_add(intf->mtu, &mtu_idx);
		if (rc != AAL_E_OK) {
			ca_spin_unlock(ca_l3_intf_lock);
			ca_printf("%s: mtu add fail\n", __func__);
			return CA_E_INTF;
		}
	}

	//mac
	if (intf->mask.mac_addr) {
		memset(&l3_cam_mac, 0x0, sizeof(l3_cam_mac_da_tbl_entry_t));
		if (intf->type == CA_L3_INTF_LB) {
			l3_cam_mac.mac_0 = lb_intf_mac[0];
			l3_cam_mac.mac_1 = lb_intf_mac[1];
			l3_cam_mac.mac_2 = lb_intf_mac[2];
			l3_cam_mac.mac_3 = lb_intf_mac[3];
			l3_cam_mac.mac_4 = lb_intf_mac[4];
			l3_cam_mac.mac_5 = lb_intf_mac[5];
			l3_cam_mac.vld   = 1;

			rc = aal_l3_cam_mac_da_lookup(lb_intf_mac, &mac_idx);
			if (rc == AAL_E_OK)
				rc = aal_entry_add_by_idx(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, mac_idx);
			else
				rc = aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, &mac_idx);
		} else {
			l3_cam_mac.mac_0 = intf->mac_addr[0];
			l3_cam_mac.mac_1 = intf->mac_addr[1];
			l3_cam_mac.mac_2 = intf->mac_addr[2];
			l3_cam_mac.mac_3 = intf->mac_addr[3];
			l3_cam_mac.mac_4 = intf->mac_addr[4];
			l3_cam_mac.mac_5 = intf->mac_addr[5];
			l3_cam_mac.vld   = 1;

			rc = aal_l3_cam_mac_da_lookup(intf->mac_addr, &mac_idx);
			if (rc == AAL_E_OK)
				rc = aal_entry_add_by_idx(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, mac_idx);
			else
				rc = aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, &mac_idx);
		}
		if (rc != AAL_E_OK) {
			if (intf->mask.mtu)
				aal_l3pe_mtu_size_del(mtu_idx);

			ca_spin_unlock(ca_l3_intf_lock);
			ca_printf("%s: mac cam add fail\n", __func__);
			return CA_E_INTF;
		}
	}

	if (intf->mask.mac_addr) {
		if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) ||
		    (((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) || \
		     (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) && \
		     (intf->type == CA_L3_INTF_LB)))		{
			memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

			if (intf->type == CA_L3_INTF_LB)
				memcpy(fdb_entry.mac, lb_intf_mac, sizeof(ca_mac_addr_t));
			else
				memcpy(fdb_entry.mac, intf->mac_addr, sizeof(ca_mac_addr_t));
			fdb_entry.key_vid = (intf->outer_vid <= 4095) ? intf->outer_vid : 0;
			fdb_entry.dot1p = 0;
			fdb_entry.static_flag = 1;
			fdb_entry.sa_permit = 1;
			fdb_entry.da_permit = 1;
			if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN)
				fdb_entry.port_id = CA_PORT_ID(PORT_TYPE(intf->port_id), AAL_LPORT_L3_LAN);
			else
				fdb_entry.port_id = CA_PORT_ID(PORT_TYPE(intf->port_id), AAL_LPORT_L3_WAN);
			fdb_entry.aging_sts = 180;

			rc = aal_fdb_entry_add(device_id, &fdb_entry);
			if (rc != CA_E_OK) {
				if (intf->mask.mac_addr)
					aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

				if (intf->mask.mtu)
					aal_l3pe_mtu_size_del(mtu_idx);

				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf("%s: mac fdb add fail\n", __func__);
				return CA_E_INTF;
			}
		}
	}

	//egress
	if ((rc = convert_intf_to_eif(intf, mtu_idx, mac_idx, &eif)) != CA_E_OK) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: fail to create intf info to IF table entry. rc=%d\n", __func__, rc);
		return rc;
	}
	rc = aal_entry_add_by_idx(AAL_TABLE_FE_L3E_IF_TBL, &eif, intf->intf_id);
	if (rc != AAL_E_OK) {
		if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
			aal_fdb_entry_del(device_id, &fdb_entry);
		}

		if (intf->mask.mac_addr)
			aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

		if (intf->mask.mtu)
			aal_l3pe_mtu_size_del(mtu_idx);

		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: eif table add fail, rc=%d\n", __func__, rc);
		return CA_E_INTF;
	}

	//ingress
	if (bypass_cls == false) {
		if ((rc = convert_intf_to_cls(intf, &profile_id, mac_idx, &rule)) != CA_E_OK) {
			ca_spin_unlock(ca_l3_intf_lock);
			ca_printf("%s: fail to create ingress cls key/action. rc=%d\n", __func__, rc);
			return rc;
		}

		rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
		if (rc != AAL_E_OK) {
			aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf->intf_id);

			if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
				aal_fdb_entry_del(device_id, &fdb_entry);
			}

			if (intf->mask.mac_addr)
				aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

			if (intf->mask.mtu)
				aal_l3pe_mtu_size_del(mtu_idx);

			ca_spin_unlock(ca_l3_intf_lock);
			ca_printf("%s: cls rule add fail, aal_l3_cls_add() returns %d.\n", __func__, rc);
			return CA_E_INTF;
		}
	}

	//to CPU port
	if ((intf->type == CA_L3_INTF_CPU) || (intf->type == CA_L3_INTF_LB))

	if ( ((intf->ip_addr.afi == CA_IPV4) && (intf->ip_addr.ip_addr.ipv4_addr != 0)) ||
	     ((intf->ip_addr.afi == CA_IPV6) &&
	     ((intf->ip_addr.ip_addr.ipv6_addr[0] != 0) || (intf->ip_addr.ip_addr.ipv6_addr[1] != 0) ||
	      (intf->ip_addr.ip_addr.ipv6_addr[2] != 0) || (intf->ip_addr.ip_addr.ipv6_addr[3] != 0))) ) {

		/*** Add HashLite entry ***/

		memset(&hash_key, 0, sizeof(hash_key));
		memset(&hash_action_entry, 0, sizeof(hash_action_entry));

		/* hash_key */
		hash_key.ip_vld = 1;
		if (intf->ip_addr.afi == CA_IPV4) {
			hash_key.ip_ver = 0;
			hash_key.ip_da_0 = intf->ip_addr.ip_addr.ipv4_addr;
			if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx)) != AAL_E_OK) {
				ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
				goto HL_FAIL;
			}

		} else {
			hash_key.ip_ver = 1;
			hash_key.ip_da_3 = intf->ip_addr.ip_addr.ipv6_addr[0];	// MSB
			hash_key.ip_da_2 = intf->ip_addr.ip_addr.ipv6_addr[1];
			hash_key.ip_da_1 = intf->ip_addr.ip_addr.ipv6_addr[2];
			hash_key.ip_da_0 = intf->ip_addr.ip_addr.ipv6_addr[3];
			if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx)) != AAL_E_OK) {
				ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
				goto HL_FAIL;
			}
		}
		hash_key.hkey_id = mask_idx;
		hash_key.mdata = intf->intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
		hash_key.ctrl_set_id = 0;		// profile#0 of HashLite

		/* hash_action */
		if (intf->type == CA_L3_INTF_CPU) {

			hash_action->permit = 1;
			hash_action->mcgid = PORT_ID(intf->port_id);
			hash_action->mc = 0;	// mcgid is LDPID
			hash_action->dpid_pri = 1;
			hash_action->dpid_vld = 1;
			hash_action->keep_orig_pkt = 1;
			hash_action->keep_orig_pkt_vld = 1;
			hash_action->t5_ctrl = 0xf;
			hash_action->t5_ctrl_vld = 1;
			hash_action->chk_msk_ptr = mask_idx;

		} else {

			hash_action->l3_egress_if_vld = 1;
			hash_action->mdata_w_vld_1 = 0x3;			// enable mdata_1[7:0]
			hash_action->mdata_w_1 = intf->intf_id & 0xff;	// fill egress intf ID into mdata_1[7:0]
			hash_action->mac_da_vld = 1;
			hash_action->mac_da_0 = lb_intf_mac[5];
			hash_action->mac_da_1 = lb_intf_mac[4];
			hash_action->mac_da_2 = lb_intf_mac[3];
			hash_action->mac_da_3 = lb_intf_mac[2];
			hash_action->mac_da_4 = lb_intf_mac[1];
			hash_action->mac_da_5 = lb_intf_mac[0];		// MSB
			hash_action->chk_msk_ptr = mask_idx;

		}

		/* add to HashLite Engine */
		ret = aal_hashlite_hash_add(0, &hash_key, &HM_L3_NEIGHBOR, &hash_action_entry, HL_ACTGRP_L3_GENERIC, HASHLITE_AGING_STATIC, &hashlite_idx);
		if (ret != AAL_E_OK) {
			ca_printf("%s: Fail to add hash into HashLite. (ret=%d)\n", __func__, ret);
			goto HL_FAIL;
		}

HL_FAIL:
		if (ret != AAL_E_OK) {
			aal_l3_cls_delete(0, profile_id, rslt_idx);

			aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf->intf_id);

			if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
				aal_fdb_entry_del(device_id, &fdb_entry);
			}

			if (intf->mask.mac_addr)
				aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

			if (intf->mask.mtu)
				aal_l3pe_mtu_size_del(mtu_idx);

			ca_spin_unlock(ca_l3_intf_lock);
			return CA_E_INTF;
		}

		/*** Add LPM entry ***/

		memset(lpm_entry, 0, sizeof(lpm_tbl_entry_t) * 4);
		memcpy(&route_entry.prefix, &intf->ip_addr, sizeof(ca_ip_address_t));
		convert_route_to_entry(&route_entry, lpm_entry, intf->intf_id & 0xfff);

		ret = aal_entry_add(AAL_TABLE_LPM, lpm_entry, &lpm_idx);
		if (ret != AAL_E_OK) {
			ca_printf("%s: Fail to add LPM entry. (ret=%d)\n", __func__, ret);
			goto LPM_FAIL;
		}

		ca_l3_intf_tbl[intf->intf_id].hashlite_idx = hashlite_idx;
		ca_l3_intf_tbl[intf->intf_id].hashlite_idx_valid = 1;
		ca_l3_intf_tbl[intf->intf_id].lpm_idx = lpm_idx;
		ca_l3_intf_tbl[intf->intf_id].lpm_idx_valid = 1;

LPM_FAIL:
		if (ret != AAL_E_OK) {
			aal_l3_cls_delete(0, profile_id, rslt_idx);

			aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf->intf_id);

			if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
				aal_fdb_entry_del(device_id, &fdb_entry);
			}

			if (intf->mask.mac_addr)
				aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

			if (intf->mask.mtu)
				aal_l3pe_mtu_size_del(mtu_idx);

			ca_spin_unlock(ca_l3_intf_lock);
			aal_hashlite_hash_delete(0, ca_l3_intf_tbl[intf->intf_id].hashlite_idx);
			return CA_E_INTF;
		}

	}

	lpb_tbl_idx = -1;
	if (((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) || \
	     (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) && \
	    (intf->type == CA_L3_INTF_LB)) {
		lpb_tbl_idx = 2;
	} else if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
		lpb_tbl_idx = 1;
	} else if ((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) ||
	    (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) {
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
		lpb_tbl_idx = 2;
#else
		lpb_tbl_idx = 0;
#endif
	}
	if (lpb_tbl_idx >= 0) {
		mask.u32 = 0;
		mask.s.mac_da_an_mask = 1;

		aal_l3_stg0_lpb_tbl_get(device_id, lpb_tbl_idx, &lpb_tbl);
		lpb_tbl.mac_da_an_mask |= 1 << (mac_idx + 1);
		aal_l3_stg0_lpb_tbl_set(device_id, lpb_tbl_idx, mask, &lpb_tbl);
	}

	memcpy(&ca_l3_intf_tbl[intf->intf_id].intf, intf, sizeof(ca_l3_intf_t));
	ca_l3_intf_tbl[intf->intf_id].cls_profile = profile_id;
	ca_l3_intf_tbl[intf->intf_id].cls_idx = rslt_idx;
	ca_l3_intf_tbl[intf->intf_id].mtu_idx = mtu_idx;
	ca_l3_intf_tbl[intf->intf_id].mac_idx = mac_idx;
	ca_l3_intf_tbl[intf->intf_id].use = 1;

	ca_spin_unlock(ca_l3_intf_lock);

#ifdef CONFIG_NE_L3COS
	/* After interface is added, try to update L3COS */
	ca_l3cos_update_all(device_id);
#endif

	return CA_E_OK;
}

ca_status_t ca_l3_intf_update (
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_l3_intf_t	*intf)
{
	fe_l3e_if_tbl_entry_t eif;
	ca_status_t rc;
	l3_cls_rule_t rule;
	unsigned int profile_id;
	ca_uint32 old_mac_idx, new_mac_idx, old_mtu_idx, new_mtu_idx;
	l3_cam_mac_da_tbl_entry_t old_l3_cam_mac, new_l3_cam_mac;
	aal_l3fe_lpb_tbl_cfg_t lpb_tbl;
	aal_l3fe_lpb_tbl_cfg_mask_t mask;
	int mtu_change = 0, mac_change = 0;
	int lpb_tbl_idx;
#if 0
	aal_hash_key_t hash_key;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3_generic_t *hash_action = (hl_act_l3_generic_t *) &hash_action_entry;
	ca_uint32 hashlite_idx, mask_idx;
#endif

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}
	if (intf == NULL) {
		ca_printf("%s: intf param is null\n", __func__);
		return CA_E_PARAM;
	}

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf(KERN_INFO "%s: Dump intf:\n", __func__);
		ca_l3_intf_dump(intf);
	}

	rc = intf_validate(intf);
	if (rc != CA_E_OK) {
		return rc;
	}

	if (intf->intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %d\n", __func__, intf->intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf->intf_id].use == 0) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: intf_id %d is not created\n", __func__, intf->intf_id);
		return CA_E_NOT_FOUND;
	}

	if ((ca_l3_intf_tbl[intf->intf_id].intf.type != intf->type) ||
	    (ca_l3_intf_tbl[intf->intf_id].intf.port_id != intf->port_id) ||
	    (ca_l3_intf_tbl[intf->intf_id].intf.outer_vid != intf->outer_vid) ||
	    (ca_l3_intf_tbl[intf->intf_id].intf.inner_vid != intf->inner_vid) ||
	    (ca_l3_intf_tbl[intf->intf_id].intf.tunnel_id != intf->tunnel_id) ||
	    (memcmp(&ca_l3_intf_tbl[intf->intf_id].intf.ip_addr, &intf->ip_addr, sizeof(ca_ip_address_t)) != 0)) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: only support mac_addr, mtu, and nat_enable field update\n", __func__);
		return CA_E_INTF;
	}

	//mtu
	old_mtu_idx = ca_l3_intf_tbl[intf->intf_id].mtu_idx;
	if (intf->mtu != ca_l3_intf_tbl[intf->intf_id].intf.mtu) {
		if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mtu)
			aal_l3pe_mtu_size_del(old_mtu_idx);
		if (intf->mask.mtu) {
			rc = aal_l3pe_mtu_size_add(intf->mtu, &new_mtu_idx);
			if (rc != AAL_E_OK) {
				if (ca_l3_intf_tbl[intf->intf_id].intf.mtu) {
					aal_l3pe_mtu_size_add(ca_l3_intf_tbl[intf->intf_id].intf.mtu, &old_mtu_idx);
					ca_l3_intf_tbl[intf->intf_id].mtu_idx = old_mtu_idx;
				}

				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf("%s: mtu update fail\n", __func__);
				return CA_E_INTF;
			}
		}
		mtu_change = 1;
	}

	new_mac_idx = ca_l3_intf_tbl[intf->intf_id].mac_idx;
	//mac
	if (intf->type != CA_L3_INTF_LB)
	if (memcmp(intf->mac_addr, ca_l3_intf_tbl[intf->intf_id].intf.mac_addr, sizeof(ca_mac_addr_t)) != 0) {
		old_mac_idx = ca_l3_intf_tbl[intf->intf_id].mac_idx;
		if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mac_addr)
			aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, old_mac_idx);

		if (intf->mask.mac_addr) {
			memset(&new_l3_cam_mac, 0, sizeof(l3_cam_mac_da_tbl_entry_t));
			new_l3_cam_mac.mac_0 = intf->mac_addr[0];
			new_l3_cam_mac.mac_1 = intf->mac_addr[1];
			new_l3_cam_mac.mac_2 = intf->mac_addr[2];
			new_l3_cam_mac.mac_3 = intf->mac_addr[3];
			new_l3_cam_mac.mac_4 = intf->mac_addr[4];
			new_l3_cam_mac.mac_5 = intf->mac_addr[5];
			new_l3_cam_mac.vld   = 1;

			rc = aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &new_l3_cam_mac, &new_mac_idx);
			if (rc != AAL_E_OK) {
				if (mtu_change) {
					if (intf->mask.mtu)
						aal_l3pe_mtu_size_del(new_mtu_idx);
					if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mtu)
						aal_l3pe_mtu_size_add(ca_l3_intf_tbl[intf->intf_id].intf.mtu, &old_mac_idx);
					ca_l3_intf_tbl[intf->intf_id].mtu_idx = old_mtu_idx;
				}

				if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mac_addr) {
					memset(&old_l3_cam_mac, 0, sizeof(l3_cam_mac_da_tbl_entry_t));
					old_l3_cam_mac.mac_0 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[0];
					old_l3_cam_mac.mac_1 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[1];
					old_l3_cam_mac.mac_2 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[2];
					old_l3_cam_mac.mac_3 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[3];
					old_l3_cam_mac.mac_4 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[4];
					old_l3_cam_mac.mac_5 = ca_l3_intf_tbl[intf->intf_id].intf.mac_addr[5];
					old_l3_cam_mac.vld   = 1;
					aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &old_l3_cam_mac, &old_mac_idx);
					ca_l3_intf_tbl[intf->intf_id].mac_idx = old_mac_idx;
				}

				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf("%s: mac cam update fail\n", __func__);
				return CA_E_INTF;
			}

			if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
				aal_fdb_entry_cfg_t fdb_entry;

				memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

				memcpy(fdb_entry.mac, &ca_l3_intf_tbl[intf->intf_id].intf.mac_addr, sizeof(ca_mac_addr_t));
				fdb_entry.key_vid = (ca_l3_intf_tbl[intf->intf_id].intf.outer_vid <= 4095) ?
						    ca_l3_intf_tbl[intf->intf_id].intf.outer_vid : 0;
				fdb_entry.dot1p = 0;
				fdb_entry.static_flag = 1;
				fdb_entry.sa_permit = 1;
				fdb_entry.da_permit = 1;
				if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN)
					fdb_entry.port_id = CA_PORT_ID(PORT_TYPE(intf->port_id), AAL_LPORT_L3_LAN);
				else
					fdb_entry.port_id = CA_PORT_ID(PORT_TYPE(intf->port_id), AAL_LPORT_L3_WAN);

				aal_fdb_entry_del(device_id, &fdb_entry);

				memcpy(fdb_entry.mac, intf->mac_addr, sizeof(ca_mac_addr_t));
				aal_fdb_entry_add(device_id, &fdb_entry);
				if (rc != CA_E_OK) {
					//memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

					memcpy(fdb_entry.mac, &ca_l3_intf_tbl[intf->intf_id].intf.mac_addr, sizeof(ca_mac_addr_t));
					aal_fdb_entry_add(device_id, &fdb_entry);

					if (intf->mask.mac_addr)
						aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, new_mac_idx);

					if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mac_addr)
						aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &old_l3_cam_mac, &old_mac_idx);
					ca_l3_intf_tbl[intf->intf_id].mac_idx = old_mac_idx;

					if (mtu_change) {
						if (intf->mask.mtu)
							aal_l3pe_mtu_size_del(new_mtu_idx);
						if (ca_l3_intf_tbl[intf->intf_id].intf.mask.mtu)
							aal_l3pe_mtu_size_add(ca_l3_intf_tbl[intf->intf_id].intf.mtu, &old_mtu_idx);
						ca_l3_intf_tbl[intf->intf_id].mtu_idx = old_mtu_idx;
					}

					ca_spin_unlock(ca_l3_intf_lock);
					ca_printf("%s: mac fdb update fail\n", __func__);
					return CA_E_INTF;
				}
			}
		}

		mac_change = 1;
	}

	//egress
	convert_intf_to_eif(intf, new_mtu_idx, new_mac_idx, &eif);
	aal_entry_set(AAL_TABLE_FE_L3E_IF_TBL, intf->intf_id, &eif);

	//ingress
	convert_intf_to_cls(intf, &profile_id, new_mac_idx, &rule);
	rc = aal_l3_cls_update(0, profile_id, &rule, ca_l3_intf_tbl[intf->intf_id].cls_idx);
#if 0
	old_profile_id = ca_l3_intf_tbl[intf->intf_id].cls_profile;
	old_rslt_idx = ca_l3_intf_tbl[intf->intf_id].cls_idx;
	convert_intf_to_cls(intf, &profile_id, &rule);
	rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
	if (rc == AAL_E_OK) {
		aal_l3_cls_delete(0, old_profile_id, old_rslt_idx);
		ca_l3_intf_tbl[intf->intf_id].cls_profile = profile_id;
		ca_l3_intf_tbl[intf->intf_id].cls_idx = rslt_idx;
	} else {
		convert_intf_to_eif(&(ca_l3_intf_tbl[intf->intf_id].intf), &eif);
		aal_entry_set(AAL_TABLE_FE_L3E_IF_TBL, intf->intf_id, &eif);
		ca_spin_unlock(ca_l3_intf_lock);
		return CA_E_INTF;
	}

	//to CPU port
	if (ca_l3_intf_tbl[intf->intf_id].hashlite_idx_valid == 1) {
		ret = aal_hashlite_hash_delete(0, ca_l3_intf_tbl[intf->intf_id].hashlite_idx);
		if (ret != AAL_E_OK) {
			ca_printf(KERN_ERR "%s: Fail to delete host route hashlite_idx=%u. (ret=%d)\n",
				__func__, ca_l3_intf_tbl[intf->intf_id].hashlite_idx, ret);
		}
		ca_l3_intf_tbl[intf->intf_id].hashlite_idx_valid = 0;
		ca_l3_intf_tbl[intf->intf_id].hashlite_idx = 0;
	}
	if ( ((intf->ip_addr.afi == CA_IPV4) && (intf->ip_addr.ip_addr.ipv4_addr != 0)) ||
	     ((intf->ip_addr.afi == CA_IPV6) &&
	     ((intf->ip_addr.ip_addr.ipv6_addr[0] != 0) || (intf->ip_addr.ip_addr.ipv6_addr[1] != 0) ||
	      (intf->ip_addr.ip_addr.ipv6_addr[2] != 0) || (intf->ip_addr.ip_addr.ipv6_addr[3] != 0))) ) {

		memset(&hash_key, 0, sizeof(hash_key));
		memset(&hash_action_entry, 0, sizeof(hash_action_entry));

		/* hash_key */
		hash_key.ip_vld = 1;
		if (intf->ip_addr.afi == CA_IPV4) {
			hash_key.ip_ver = 0;
			hash_key.ip_da_0 = intf->ip_addr.ip_addr.ipv4_addr;
			if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx)) != AAL_E_OK) {
				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
				return ret;
			}

		} else {
			hash_key.ip_ver = 1;
			hash_key.ip_da_3 = intf->ip_addr.ip_addr.ipv6_addr[0];	// MSB
			hash_key.ip_da_2 = intf->ip_addr.ip_addr.ipv6_addr[1];
			hash_key.ip_da_1 = intf->ip_addr.ip_addr.ipv6_addr[2];
			hash_key.ip_da_0 = intf->ip_addr.ip_addr.ipv6_addr[3];
			if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx)) != AAL_E_OK) {
				ca_spin_unlock(ca_l3_intf_lock);
				ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
				return CA_E_INTF;
			}
		}
		hash_key.hkey_id = mask_idx;
		hash_key.mdata = intf->intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
		hash_key.ctrl_set_id = 0;		// profile#0 of HashLite

		/* hash_action */
		hash_action->permit = 1;
		if ((PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN) || (PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN))
			hash_action->mcgid = AAL_LPORT_CPU_0;
		else
			hash_action->mcgid = AAL_LPORT_CPU_1;
		hash_action->mc = 0;	// mcgid is LDPID
		hash_action->dpid_pri = 1;
		hash_action->dpid_vld = 1;
		hash_action->keep_orig_pkt = 1;
		hash_action->keep_orig_pkt_vld = 1;
		hash_action->t5_ctrl = 0xf;
		hash_action->t5_ctrl_vld = 1;
		hash_action->chk_msk_ptr = mask_idx;

		/* add to HashLite Engine */
		ret = aal_hashlite_hash_add(0, &hash_key, &HM_L3_NEIGHBOR, &hash_action_entry, HL_ACTGRP_L3_GENERIC, HASHLITE_AGING_STATIC, &hashlite_idx);
		if (ret != AAL_E_OK) {
			ca_spin_unlock(ca_l3_intf_lock);
			return CA_E_INTF;
		}
		ca_l3_intf_tbl[intf->intf_id].hashlite_idx = hashlite_idx;
		ca_l3_intf_tbl[intf->intf_id].hashlite_idx_valid = 1;
	}
#endif
	if (mtu_change == 1)
		ca_l3_intf_tbl[intf->intf_id].mtu_idx = new_mtu_idx;

	if (mac_change == 1) {
		ca_l3_intf_tbl[intf->intf_id].mac_idx = new_mac_idx;

		if (new_mac_idx != old_mac_idx) {
			lpb_tbl_idx = -1;

			if (((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) || \
			     (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) && \
			    (intf->type == CA_L3_INTF_LB)) {
				lpb_tbl_idx = 2;
			} else if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
				lpb_tbl_idx = 1;
			} else if ((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) ||
			    (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) {
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
				lpb_tbl_idx = 2;
#else
				lpb_tbl_idx = 0;
#endif
			}
			if (lpb_tbl_idx >= 0) {
				mask.u32 = 0;
				mask.s.mac_da_an_mask = 1;

				aal_l3_stg0_lpb_tbl_get(device_id, lpb_tbl_idx, &lpb_tbl);
				lpb_tbl.mac_da_an_mask &= ~(1 << (old_mac_idx + 1));
				lpb_tbl.mac_da_an_mask |= 1 << (new_mac_idx + 1);
				aal_l3_stg0_lpb_tbl_set(device_id, lpb_tbl_idx, mask, &lpb_tbl);
			}
		}
	}

	memcpy(&ca_l3_intf_tbl[intf->intf_id].intf, intf, sizeof(ca_l3_intf_t));
	ca_spin_unlock(ca_l3_intf_lock);
	return CA_E_OK;
}

ca_status_t ca_l3_intf_delete (
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_intf_id_t	intf_id)
{
	ca_l3_intf_t *intf = NULL;
	unsigned int profile_id;
	unsigned int rslt_idx;
	aal_l3fe_lpb_tbl_cfg_t lpb_tbl;
	aal_l3fe_lpb_tbl_cfg_mask_t mask;
	l3_cam_mac_da_tbl_entry_t l3_cam_mac;
	ca_uint32_t mac_idx;
	aal_fdb_entry_cfg_t fdb_entry;
	int ret, lpb_tbl_idx;
	int keep_lpb = 0;

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}

	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %d\n", __func__, intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf_id].use == 0) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: intf_id %d is not created\n", __func__, intf_id);
		return CA_E_NOT_FOUND;
	}

	intf = &ca_l3_intf_tbl[intf_id].intf;

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf(KERN_INFO "%s: Dump intf:\n", __func__);
		ca_l3_intf_dump(intf);
	}

	//ingress
	profile_id = ca_l3_intf_tbl[intf_id].cls_profile;
	rslt_idx = ca_l3_intf_tbl[intf_id].cls_idx;
	aal_l3_cls_delete(0, profile_id, rslt_idx);

	//egress
	aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf_id);

	//mtu
	if (ca_l3_intf_tbl[intf_id].intf.mtu)
		aal_l3pe_mtu_size_del(ca_l3_intf_tbl[intf_id].mtu_idx);

	//mac
	aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, ca_l3_intf_tbl[intf_id].mac_idx);
	l3_cam_mac.mac_0 = ca_l3_intf_tbl[intf_id].intf.mac_addr[0];
	l3_cam_mac.mac_1 = ca_l3_intf_tbl[intf_id].intf.mac_addr[1];
	l3_cam_mac.mac_2 = ca_l3_intf_tbl[intf_id].intf.mac_addr[2];
	l3_cam_mac.mac_3 = ca_l3_intf_tbl[intf_id].intf.mac_addr[3];
	l3_cam_mac.mac_4 = ca_l3_intf_tbl[intf_id].intf.mac_addr[4];
	l3_cam_mac.mac_5 = ca_l3_intf_tbl[intf_id].intf.mac_addr[5];
	l3_cam_mac.vld   = 1;
	ret = aal_l3_cam_mac_da_lookup(intf->mac_addr, &mac_idx);
	if (ret == AAL_E_ENTRYNOTFOUND) {
		if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
			memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

			memcpy(fdb_entry.mac, &ca_l3_intf_tbl[intf_id].intf.mac_addr, sizeof(ca_mac_addr_t));
			fdb_entry.key_vid = (ca_l3_intf_tbl[intf_id].intf.outer_vid <= 4095) ?
					     ca_l3_intf_tbl[intf_id].intf.outer_vid : 0;
			fdb_entry.dot1p = 0;
			fdb_entry.static_flag = 1;
			fdb_entry.sa_permit = 1;
			fdb_entry.da_permit = 1;
			fdb_entry.port_id = CA_PORT_ID(PORT_TYPE(intf->port_id), AAL_LPORT_L3_LAN);

			aal_fdb_entry_del(device_id, &fdb_entry);
		}
	} else if (ret == AAL_E_OK) {
		keep_lpb = 1;
	}

	lpb_tbl_idx = -1;
	if (((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) || \
	     (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) && \
	    (intf->type == CA_L3_INTF_LB)) {
		lpb_tbl_idx = 2;
	} else if (PORT_ID(intf->port_id) == AAL_LPORT_L3_LAN) {
		lpb_tbl_idx = 1;
	} else if ((PORT_ID(intf->port_id) == AAL_LPORT_ETH_WAN) ||
	    (PORT_ID(intf->port_id) == AAL_LPORT_L3_WAN)) {
#if defined(CONFIG_ARCH_CORTINA_G3LITE)
		lpb_tbl_idx = 2;
#else
		lpb_tbl_idx = 0;
#endif
	}
	if ((lpb_tbl_idx >= 0) && (keep_lpb == 0)) {
		mask.u32 = 0;
		mask.s.mac_da_an_mask = 1;

		aal_l3_stg0_lpb_tbl_get(device_id, lpb_tbl_idx, &lpb_tbl);
		lpb_tbl.mac_da_an_mask &= ~(1 << (ca_l3_intf_tbl[intf_id].mac_idx + 1));
		aal_l3_stg0_lpb_tbl_set(device_id, lpb_tbl_idx, mask, &lpb_tbl);
	}

	//to CPU port
	if (ca_l3_intf_tbl[intf_id].hashlite_idx_valid == 1) {
		ret = aal_hashlite_hash_delete(0, ca_l3_intf_tbl[intf_id].hashlite_idx);
		if (ret != AAL_E_OK) {
			ca_printf(KERN_ERR "%s: Fail to delete host route hashlite_idx=%u. (ret=%d)\n",
				__func__, ca_l3_intf_tbl[intf_id].hashlite_idx, ret);
		}
		ca_l3_intf_tbl[intf_id].hashlite_idx_valid = 0;
		ca_l3_intf_tbl[intf_id].hashlite_idx = 0;
	}

	if (ca_l3_intf_tbl[intf_id].lpm_idx_valid == 1) {
		ret = aal_entry_del(AAL_TABLE_LPM, ca_l3_intf_tbl[intf_id].lpm_idx);
		if (ret != AAL_E_OK) {
			ca_printf(KERN_ERR "%s: Fail to delete LPM entry lpm_idx=%u. (ret=%d)\n",
				__func__, ca_l3_intf_tbl[intf_id].lpm_idx, ret);
		}
		ca_l3_intf_tbl[intf_id].lpm_idx_valid = 0;
		ca_l3_intf_tbl[intf_id].lpm_idx = 0;
	}

	ca_l3_intf_tbl[intf_id].use = 0;

	ca_spin_unlock(ca_l3_intf_lock);
	return CA_E_OK;
}

ca_status_t ca_l3_intf_get (
	CA_IN		ca_device_id_t	device_id,
	CA_OUT		ca_l3_intf_t	*intf)
{
	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}
	if (intf == NULL) {
		ca_printf("%s: intf param is null\n", __func__);
		return CA_E_PARAM;
	}

	if (intf->intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %d\n", __func__, intf->intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf->intf_id].use == 0) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: intf_id %d is not created\n", __func__, intf->intf_id);
		return CA_E_NOT_FOUND;
	}

	memcpy(intf, &ca_l3_intf_tbl[intf->intf_id].intf, sizeof(ca_l3_intf_t));

	ca_spin_unlock(ca_l3_intf_lock);
	return CA_E_OK;
}

ca_status_t ca_l3_intf_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry)
{
	ca_l3_intf_t *pre_intf;
	unsigned int i, start, count;
	void *intf;

	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}
	if (return_entry == NULL)
		return CA_E_PARAM;
	if (return_entry->entry_data == NULL)
		return CA_E_PARAM;
	if (return_entry->entry_count == 0)
		return CA_E_UNAVAIL;

	pre_intf = return_entry->prev_handle;
	if (pre_intf == NULL)
		start = 0;
	else
		start = pre_intf->intf_id + 1;

	ca_spin_lock(ca_l3_intf_lock);

	count = 0;
	for (i = start; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
		if (ca_l3_intf_tbl[i].use == 1)
			count++;
		if (count >= return_entry->entry_count)
			break;
	}
	if ((return_entry->user_buffer_size / sizeof(ca_l3_intf_t)) < count)
		count = return_entry->user_buffer_size;
	return_entry->entry_count = count;

	intf = return_entry->entry_data;
	for (i = start; count > 0; i++) {
		if (ca_l3_intf_tbl[i].use == 1) {
			memcpy(intf, &ca_l3_intf_tbl[i].intf, sizeof(ca_l3_intf_t));
			count--;
			intf += sizeof(ca_l3_intf_t);
			return_entry->prev_handle = &ca_l3_intf_tbl[i].intf;
		}
	}

	ca_spin_unlock(ca_l3_intf_lock);

	if (return_entry->entry_count == 0)
		return CA_E_UNAVAIL;

	return CA_E_OK;
}

ca_status_t ca_l3_intf_stats_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_intf_id_t		intf_id,
	CA_IN		ca_boolean_t		read_clear,
	CA_OUT		ca_l3_intf_stats_t	*stats)
{
	l3pe_cntr_tx_t tx_cntr;
	l3pe_cntr_rx_t rx_cntr;

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}

	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %d\n", __func__, intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	aal_l3pe_cntr_tx_get(device_id, intf_id, &tx_cntr);
	aal_l3pe_cntr_rx_get(device_id, intf_id, &rx_cntr);

	stats->tx_csum_err = 0;
	stats->rx_csum_err = rx_cntr.error_pkt;
	stats->tx_bytes = tx_cntr.byte;
	stats->rx_bytes = rx_cntr.byte;
	stats->tx_pkts = tx_cntr.uc_pkt + tx_cntr.nuc_pkt;
	stats->rx_pkts = rx_cntr.uc_pkt + rx_cntr.nuc_pkt;
	stats->tx_dropbytes = tx_cntr.drop_byte;
	stats->rx_dropbytes = rx_cntr.drop_byte;
	stats->tx_droppkts = tx_cntr.drop_pkt;
	stats->rx_droppkts = rx_cntr.drop_pkt;

	if (read_clear == CA_TRUE) {
		memset(&tx_cntr, 0, sizeof(tx_cntr));
		memset(&rx_cntr, 0, sizeof(rx_cntr));

		aal_l3pe_cntr_tx_set(device_id, intf_id, &tx_cntr);
		aal_l3pe_cntr_rx_set(device_id, intf_id, &rx_cntr);
	}

	ca_spin_unlock(ca_l3_intf_lock);

	return CA_E_OK;
}

ca_status_t ca_l3_intf_stats_clear (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_intf_id_t		intf_id)
{
	l3pe_cntr_tx_t tx_cntr;
	l3pe_cntr_rx_t rx_cntr;

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}

	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %d\n", __func__, intf_id);
		return CA_E_INTF;
	}

	memset(&tx_cntr, 0, sizeof(tx_cntr));
	memset(&rx_cntr, 0, sizeof(rx_cntr));

	ca_spin_lock(ca_l3_intf_lock);

	aal_l3pe_cntr_tx_set(device_id, intf_id, &tx_cntr);
	aal_l3pe_cntr_rx_set(device_id, intf_id, &rx_cntr);

	ca_spin_unlock(ca_l3_intf_lock);

	return CA_E_OK;
}

ca_status_t ca_l3_intf_stats_clear_all (
	CA_IN	ca_device_id_t	device_id)
{
	l3pe_cntr_tx_t tx_cntr;
	l3pe_cntr_rx_t rx_cntr;
	unsigned int idx;

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}

	memset(&tx_cntr, 0, sizeof(tx_cntr));
	memset(&rx_cntr, 0, sizeof(rx_cntr));

	ca_spin_lock(ca_l3_intf_lock);

	for (idx = 0; idx < FE_L3E_IF_TBL_ENTRY_MAX; idx++) {
		aal_l3pe_cntr_tx_set(device_id, idx, &tx_cntr);
		aal_l3pe_cntr_rx_set(device_id, idx, &rx_cntr);
	}

	ca_spin_unlock(ca_l3_intf_lock);

	return CA_E_OK;
}

ca_status_t ca_l3_route_add (
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_l3_route_t	*route)
{
	lpm_tbl_entry_t entry[4];
	unsigned int lpm_idx;
	int ret;
	uint32_t rst_idx = 0xffffffff; /* LPM.rst_idx, HashLite.mdata */
	struct route_node *iter_rt;
	struct nexthop_node *iter_nh;
	bool found = false;
	bool nexthop_sw_only = false;

	if (route == NULL) return CA_E_PARAM;
	if ((route->prefix.afi == CA_IPV4) && (route->prefix.addr_len > 32)) return CA_E_PARAM;
	if ((route->prefix.afi == CA_IPV6) && (route->prefix.addr_len > 128)) return CA_E_PARAM;
	if ((ret = ip_addr_mask(&route->prefix)) != CA_E_OK) return ret;

	if (ca_api_debug & API_DBG_ROUTE) {
		if (route->prefix.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: prefix=0x%08x / %u, nexthop_id=%u\n", __func__,
				route->prefix.ip_addr.ipv4_addr, route->prefix.addr_len, route->nexthop_id);
		} else {
			ca_printf(KERN_INFO "%s: prefix=%08x-%08x-%08x-%08x / %u, nexthop_id=%u\n", __func__,
				route->prefix.ip_addr.ipv6_addr[0], route->prefix.ip_addr.ipv6_addr[1],
				route->prefix.ip_addr.ipv6_addr[2], route->prefix.ip_addr.ipv6_addr[3],
				route->prefix.addr_len, route->nexthop_id);
		}
	}

	/* get rst_idx from nexthop_table */
	read_lock(&nexthop_table_lock);
	list_for_each_entry(iter_nh, &nexthop_table, list) {
		if (iter_nh->nexthop.nexthop_id == route->nexthop_id) {
			found = true;
			break;
		}
	}
	read_unlock(&nexthop_table_lock);
	if (!found) return CA_E_PARAM;

	if (ca_api_debug & API_DBG_ROUTE) {
		ca_printf(KERN_INFO "%s: Found nexthop_id %d in nexthop_table.\n", __func__, route->nexthop_id);
	}

	/* get rst_idx for LPM entry */
	if (iter_nh->sw_only) {
		/* Use intf_id as rst_idx when:
		 *  1. iter_nh->nexthop.addr and iter_nh->nexthop.da_mac are all zero; it is SW_only entry.
		 *     This route entry must route matched packets to neighbor hosts through a fixed interface, so use interface ID as rst_idx (hash_key.mdata).
		 */
		rst_idx = iter_nh->nexthop.intf_id;
		nexthop_sw_only = true;
	} else {
		/* Use a unique number as rst_idx when:
		 *  1. iter_nh->nexthop.addr or iter_nh->nexthop.da_mac are not all zero.
		 *     This route entry must route matched packets to a specific gateway or MAC_DA.
		 *     For such case, use HashLite profile#14 where only hash_action is available and be indexed by LPM.rst_idx directly.
		 */
		rst_idx = iter_nh->hash_idx_neighbor;
		nexthop_sw_only = false;
	}

	/* Add LPM entry */
	memset(&entry, 0, sizeof(entry));
	convert_route_to_entry(route, entry, rst_idx);
	entry[0].rst_ctrl = (nexthop_sw_only) ? 0 : 1; // t4_ctrl_0 & t4_ctrl_1; it contains HashLite profile number.
	ret = aal_entry_add(AAL_TABLE_LPM, entry, &lpm_idx);
	if (ret == AAL_E_NOMEM) return CA_E_FULL;
	if (ret == AAL_E_EXIST) return CA_E_EXISTS;
	if (ret == AAL_E_INVALID) return CA_E_PARAM;
	if (ret != AAL_E_OK) return CA_E_ERROR;

	/* add into local route table */
	iter_rt = ca_zalloc_trace(sizeof(struct route_node), GFP_ATOMIC, true);
	if (iter_rt == NULL) {
		return CA_E_MEMORY;
	}
	iter_rt->lpm_idx = lpm_idx;
	iter_rt->rst_idx = rst_idx;
	iter_rt->route = *route;
	write_lock(&route_table_lock);
	list_add(&iter_rt->list, &route_table);
	list_sort(NULL, &route_table, &route_node_cmp);
	write_unlock(&route_table_lock);

	if (ca_api_debug & API_DBG_ROUTE) {
		ca_printf(KERN_INFO "%s: route is added, lpm_idx=%u.\n", __func__, lpm_idx);
		route_table_dump();
	}

	return CA_E_OK;
}

ca_status_t ca_l3_route_delete (
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_l3_route_t	*route)
{
	int ret;
	struct route_node *iter_rt, *tmp_iter_rt;
	bool found = false;
	ca_l3_route_t tmp_route;
	unsigned int tmp_lpm_idx;
	uint32_t tmp_rst_idx;

	if (route == NULL) return CA_E_PARAM;
	if ((ret = ip_addr_mask(&route->prefix)) != CA_E_OK) return ret;

	if (ca_api_debug & API_DBG_ROUTE) {
		if (route->prefix.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: prefix=0x%08x / %u, nexthop_id=%u\n", __func__,
				route->prefix.ip_addr.ipv4_addr, route->prefix.addr_len, route->nexthop_id);
		} else {
			ca_printf(KERN_INFO "%s: prefix=%08x-%08x-%08x-%08x / %u, nexthop_id=%u\n", __func__,
				route->prefix.ip_addr.ipv6_addr[0], route->prefix.ip_addr.ipv6_addr[1],
				route->prefix.ip_addr.ipv6_addr[2], route->prefix.ip_addr.ipv6_addr[3],
				route->prefix.addr_len, route->nexthop_id);
		}
	}

	write_lock(&route_table_lock);

	/* Search route_table with same prefix; nexthop_id is ignored. */
	list_for_each_entry(iter_rt, &route_table, list) {
		if (memcmp(&route->prefix, &iter_rt->route.prefix, sizeof(ca_ip_address_t)) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		write_unlock(&route_table_lock);
		return CA_E_NOT_FOUND;
	}
	memcpy(&tmp_route, &iter_rt->route, sizeof(tmp_route));		// the actual route to be deleted
	tmp_lpm_idx = iter_rt->lpm_idx;
	tmp_rst_idx = iter_rt->rst_idx;

	ret = aal_entry_del(AAL_TABLE_LPM, tmp_lpm_idx);
	if (ret != AAL_E_OK) {
		write_unlock(&route_table_lock);
		return CA_E_NOT_FOUND;
	}

	/* delete from route_table */
	list_for_each_entry_safe(iter_rt, tmp_iter_rt, &route_table, list) {
		if (tmp_lpm_idx == iter_rt->lpm_idx) {
			list_del(&iter_rt->list);
			ca_free_trace(iter_rt);
			break;
		}
	}

	write_unlock(&route_table_lock);

	return CA_E_OK;
}

ca_status_t ca_l3_route_get (
	CA_IN		ca_device_id_t	device_id,
	CA_OUT		ca_l3_route_t	*route)
{
	struct route_node *iter;
	ca_ip_address_t prefix;
	ca_status_t ret;

	if (route == NULL) return CA_E_PARAM;
	memcpy(&prefix, &route->prefix, sizeof(prefix));
	if ((ret = ip_addr_mask(&prefix)) != CA_E_OK) return ret;

	if (ca_api_debug & API_DBG_ROUTE) {
		if (route->prefix.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: prefix=0x%08x / %u\n", __func__,
				route->prefix.ip_addr.ipv4_addr, route->prefix.addr_len);
		} else {
			ca_printf(KERN_INFO "%s: prefix=%08x-%08x-%08x-%08x / %u\n", __func__,
				route->prefix.ip_addr.ipv6_addr[0], route->prefix.ip_addr.ipv6_addr[1],
				route->prefix.ip_addr.ipv6_addr[2], route->prefix.ip_addr.ipv6_addr[3],
				route->prefix.addr_len);
		}
	}

	read_lock(&route_table_lock);
	list_for_each_entry(iter, &route_table, list) {
		if (memcmp(&prefix, &iter->route.prefix, sizeof(prefix)) == 0) {
			route->nexthop_id = iter->route.nexthop_id;
			if (ca_api_debug & API_DBG_ROUTE) {
				ca_printf(KERN_INFO "%s: Entry is found in route_table, nexthop_id=%u.\n", __func__, iter->route.nexthop_id);
			}
			read_unlock(&route_table_lock);
			return CA_E_OK;
		}
	} /* list_for_each_entry() */
	read_unlock(&route_table_lock);

	if (ca_api_debug & API_DBG_ROUTE) {
		ca_printf(KERN_INFO "%s: Entry is NOT found in route_table.\n", __func__);
	}

	return CA_E_NOT_FOUND;
}

ca_status_t ca_l3_route_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ip_afi_t		afi,
	CA_OUT		ca_iterator_t		*return_entry )
{
	ca_status_t status;
	ca_uint32_t count = 0;
	ca_l3_route_t *ptr = (ca_l3_route_t *) return_entry->entry_data;
	struct route_node *iter, *tmp_iter;
	bool foundNode = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL) return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_l3_route_t) * return_entry->entry_count > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) foundNode = true;

	read_lock(&route_table_lock);
	list_for_each_entry_safe(iter, tmp_iter, &route_table, list) {
		if (iter->route.prefix.afi != afi) continue;

		/* found the node stored in prev_handle, start to get contents from next node */
		if (foundNode == false) {
			if (iter == return_entry->prev_handle) foundNode = true;
			continue;
		}

		memcpy(ptr, &iter->route, sizeof(iter->route));
		return_entry->entry_index[count] = iter->lpm_idx;
		ptr++;
		count++;

		if (count >= return_entry->entry_count) break;
	}
	read_unlock(&route_table_lock);
	return_entry->entry_count = count;

	if (list_empty(&route_table)) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* list_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

ca_status_t ca_l3_nexthop_add (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_l3_nexthop_t		*nexthop,
	CA_OUT	ca_l3_nexthop_id_t	*nexthop_id)
{
	aal_hash_key_t hash_key;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3_generic_t *hash_action = (hl_act_l3_generic_t *) &hash_action_entry;
	int hash_idx_neighbor = -1;
	unsigned int mask_idx_neighbor;
	int ret;
	struct nexthop_node *nh, *iter;
	unsigned int tmp_nexthop_id = -1; /* candidate nexthop_id for nexthop entry */
	aal_hashlite_aging_e hash_aging = HASHLITE_AGING_INVALID;
	ca_tunnel_cfg_t *tcfg = NULL;
	tunnel_info_t tinfo;
	bool sw_only = false;

	if ((nexthop == NULL) || (nexthop_id == NULL))
		return CA_E_PARAM;

	if (nexthop->intf_id >= FE_L3E_IF_TBL_ENTRY_MAX)
		return CA_E_PARAM;

	if (ca_l3_intf_tbl[nexthop->intf_id].use == 0)
		return CA_E_PARAM;

	if ((ret = nexthop_addr_check(nexthop)) != CA_E_OK)
		return ret;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		if (nexthop->addr.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: ipv4_addr=0x%08x / %u, attr_flags=%u, intf_id=0x%x, da_mac=%02x-%02x-%02x-%02x-%02x-%02x\n", __func__,
				nexthop->addr.ip_addr.ipv4_addr, nexthop->addr.addr_len, nexthop->attr_flags, nexthop->intf_id,
				nexthop->da_mac[0], nexthop->da_mac[1], nexthop->da_mac[2],
				nexthop->da_mac[3], nexthop->da_mac[4], nexthop->da_mac[5]);
			ca_printf(KERN_INFO "%s: aging_timer=%u\n", __func__, nexthop->aging_timer);
		} else {
			ca_printf(KERN_INFO "%s: ipv6_addr=0x%08x 0x%08x 0x%08x 0x%08x / %u, attr_flags=%u, intf_id=0x%x, da_mac=%02x-%02x-%02x-%02x-%02x-%02x\n",
				__func__,
				nexthop->addr.ip_addr.ipv6_addr[0], nexthop->addr.ip_addr.ipv6_addr[1],
				nexthop->addr.ip_addr.ipv6_addr[2], nexthop->addr.ip_addr.ipv6_addr[3],
				nexthop->addr.addr_len, nexthop->attr_flags, nexthop->intf_id,
				nexthop->da_mac[0], nexthop->da_mac[1], nexthop->da_mac[2],
				nexthop->da_mac[3], nexthop->da_mac[4], nexthop->da_mac[5]);
			ca_printf(KERN_INFO "%s: aging_timer=%u\n", __func__, nexthop->aging_timer);
		}
	}

	/* search if SW nexthop_table already contains same data */
	read_lock(&nexthop_table_lock);
	list_for_each_entry(iter, &nexthop_table, list) {

		if (   (iter->nexthop.intf_id == nexthop->intf_id)
		    && (memcmp(&iter->nexthop.addr, &nexthop->addr, sizeof(ca_ip_address_t)) == 0) ) {
			/* return nexthop_id of existed nexthop entry */
			*nexthop_id = iter->nexthop.nexthop_id;
			read_unlock(&nexthop_table_lock);
			return CA_E_EXISTS;
		}

	}
	read_unlock(&nexthop_table_lock);

	if (ca_l3_intf_tbl[nexthop->intf_id].intf.type == CA_L3_INTF_TUNNEL) {
		if (   (g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id] == NULL)
		    || (g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id]->used == CA_FALSE) )
			return CA_E_ERROR; /* should not happen */
		tcfg = &g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id]->tcfg;
	}

	/* If (nexthop.addr.adr_len, nexthop.da_mac) contains
	 * (0, 0):
	 *   - Neighbor host route. Created when IP is assigned to a broadcast-type interface.
	 *   - Example cmd is 'route add 192.168.1.0/24 dev eth0'
	 *   - SW only.
	 *   - This case doesn't create any hash; this is for neighbor route.
	 *     If neighbor route matches this nexthop entry, it uses intf_id as key.mdata (LPM.rst_idx).
	 * (0, not 0):
	 *    - PPP route. It is also gateway route but peer MAC addr is known.
	 *    - Example cmd is 'route add default dev ppp0' or 'route add gw PEER_IP dev ppp0'
	 *    - static entry.
	 *    - Must use an unique LPM.rst_idx, here uses HashLite profile#14 and hash_idx as LPM.rst_idx.
	 *      Only hash_action of the hash is required.
	 * (not 0, 0):
	 *    - Gateway route.
	 *    - Example cmd is 'route add gw GW_IP dev eth0'
	 *    - static entry.
	 *    - nexthop.da_mac is 0, so set age is 0 (expired, invalid).
	 *    - Must use an unique LPM.rst_idx, here uses HashLite profile#14 and hash_idx as LPM.rst_idx.
	 *      Only hash_action of the hash is required.
	 * (not 0, not 0):
	 *    - Neighbor route. Created when ARP or NDP response is received.
	 *    - dynamic entry.
	 *    - Key.mdata (LPM.rst_idx) is intf_id for neighbor-type nexhtop.
	 */
	if ((memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) != 0) && (nexthop->addr.addr_len != 0)) {

		/* NextHop entry created from ARP or NDP */
		sw_only = false;
		hash_aging = (nexthop->attr_flags == CA_L3_NHOP_ATTR_NEGH) ? HASHLITE_AGING_START : HASHLITE_AGING_STATIC;

	} else if ((memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) == 0) && (nexthop->addr.addr_len == 0)) {

		if (ca_l3_intf_tbl[nexthop->intf_id].intf.type == CA_L3_INTF_TUNNEL) {
			/* Created by command "ifconfig TUNNEL_INTF up".
			 * Such tunnel interface often has no ARP, so no neighbor.
			 * Make it as a static NextHop entry.
			 */
			sw_only = false;
			hash_aging = HASHLITE_AGING_INVALID;
		} else {
			/* Neighbor host route.
			 * Just add it into SW table; don't add it into HW table.
			 * Otherwise, unknown packet can't go to CPU for triggering an ARP/NDP.
			 */
			sw_only = true;
			hash_aging = HASHLITE_AGING_INVALID;
			if ((ret = bm_swonly_nhid_alloc(&tmp_nexthop_id)) != CA_E_OK) {
				ca_printf(KERN_ERR "%s: No available SW-only NextHop ID. (ret=%u)\n", __func__, ret);
				return ret;
			}
			hash_idx_neighbor = -1;
			goto _add_sw_nexthop;
		}

	} else {

		/* Gateway route */
		sw_only = false;

		if (memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) == 0) {
			hash_aging = HASHLITE_AGING_INVALID;
		} else {
			hash_aging = HASHLITE_AGING_STATIC;
		}

	}

	if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx_neighbor)) != AAL_E_OK) {
		ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
		return ret;
	}

	memset(&hash_key, 0, sizeof(hash_key));
	memset(&hash_action_entry, 0, sizeof(hash_action_entry));

	/* fill hash key */
	hash_key.ip_vld = 1;
	if (nexthop->addr.afi == CA_IPV4) {
		hash_key.ip_ver = 0;
		hash_key.ip_da_0 = nexthop->addr.ip_addr.ipv4_addr;
	} else {
		hash_key.ip_ver = 1;
		hash_key.ip_da_3 = nexthop->addr.ip_addr.ipv6_addr[0];	// MSB
		hash_key.ip_da_2 = nexthop->addr.ip_addr.ipv6_addr[1];
		hash_key.ip_da_1 = nexthop->addr.ip_addr.ipv6_addr[2];
		hash_key.ip_da_0 = nexthop->addr.ip_addr.ipv6_addr[3];
	}
	hash_key.hkey_id = mask_idx_neighbor;
	hash_key.mdata = nexthop->intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
	hash_key.ctrl_set_id = 0;			// profile#0 of HashLite

	/* fill hash action */
	hash_action->ip_ttl_vld = 1;
	hash_action->ip_ttl_zero_discard_en = 1;
	hash_action->ip_ttl_cmd = 1;
	hash_action->l3_egress_if_vld = 1;
	hash_action->mdata_w_vld_1 = 0x3;			// enable mdata_1[7:0]
	hash_action->mdata_w_1 = nexthop->intf_id & 0xff;	// fill egress intf ID into mdata_1[7:0]
	hash_action->mac_da_vld = 1;
	hash_action->mac_da_0 = nexthop->da_mac[5];		// LSB
	hash_action->mac_da_1 = nexthop->da_mac[4];
	hash_action->mac_da_2 = nexthop->da_mac[3];
	hash_action->mac_da_3 = nexthop->da_mac[2];
	hash_action->mac_da_4 = nexthop->da_mac[1];
	hash_action->mac_da_5 = nexthop->da_mac[0];		// MSB
	hash_action->chk_msk_ptr = mask_idx_neighbor;
	if (hash_aging == HASHLITE_AGING_INVALID) {
		hash_action->l3_egress_if_vld = 0;
		hash_action->mdata_w_vld_1 = 0x0;			// disable mdata_1[7:0]
		hash_action->mdata_w_1 = 0x00;				// clear the value
		hash_action->keep_orig_pkt = 1;
		hash_action->keep_orig_pkt_vld = 1;
		hash_action->dpid_vld = 1;
		hash_action->permit = 1;
		hash_action->mcgid = 0x10;
		hash_action->mc = 0;
	}

	if (tcfg != NULL) {
		memset(&tinfo, 0, sizeof(tinfo));
		tinfo.func_id = CA_FUNC_ID_INVALID;

		if (tcfg->type == CA_TUNNEL_PPPOE) {
			if ((ret = convert_nexthop_to_hashlite_pppoe(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_L2TP) {
			if ((ret = convert_nexthop_to_hashlite_l2tp(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_IPSEC) {
			/* overwrite previous actions */
			hash_action->l3_egress_if_vld = 1;
			hash_action->mdata_w_vld_1 = 0x3;			// enable mdata_1[7:0]
			hash_action->mdata_w_1 = nexthop->intf_id & 0xff;	// fill egress intf ID into mdata_1[7:0]
			hash_action->keep_orig_pkt = 0;
			hash_action->keep_orig_pkt_vld = 0;

			if ((ret = convert_nexthop_to_hashlite_ipsec(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_PPTP) {
			if ((ret = convert_nexthop_to_hashlite_pptp(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_4_IN_4) {

		}

		if (tcfg->type == CA_TUNNEL_DSLITE) {
			if ((ret = convert_nexthop_to_hashlite_dslite(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6RD) {
			if ((ret = convert_nexthop_to_hashlite_6rd(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6_IN_6) {

		}

		if (tcfg->type == CA_TUNNEL_MAP_E) {
			if ((ret = convert_nexthop_to_hashlite_map_e(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_MAP_T) {
			if ((ret = convert_nexthop_to_hashlite_map_t(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}
	}

	ret = aal_hashlite_hash_add(0,
		&hash_key,
		&HM_L3_NEIGHBOR,
		&hash_action_entry,
		HL_ACTGRP_L3_GENERIC,
		hash_aging,
		&hash_idx_neighbor);
	if (ret != AAL_E_OK) {
		if (ca_api_debug & API_DBG_NEXTHOP) {
			ca_printf(KERN_ERR "%s: Fail to add hash. (ret=%d)\n", __func__, ret);
		}
		goto _end_nexthop_add;
	}

	tmp_nexthop_id = hash_idx_neighbor;

_add_sw_nexthop:
	/* add nexthop into local nexthop_table */
	nh = ca_zalloc_trace(sizeof(struct nexthop_node), GFP_ATOMIC, true);
	if (nh == NULL) {
		return CA_E_MEMORY;
	}
	nh->nexthop = *nexthop;
	nh->nexthop.nexthop_id = tmp_nexthop_id;
	nh->hash_idx_neighbor = hash_idx_neighbor;
	nh->sw_only = sw_only;
	write_lock(&nexthop_table_lock);
	list_add(&nh->list, &nexthop_table);
	list_sort(NULL, &nexthop_table, &nexthop_node_cmp);
	write_unlock(&nexthop_table_lock);

	*nexthop_id = nh->nexthop.nexthop_id;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf(KERN_INFO "%s: nexthop is added, nexthop_id=%u (0x%x).\n", __func__, *nexthop_id, *nexthop_id);
		// nexthop_table_dump();
	}

	return CA_E_OK;

_end_nexthop_add:
	if (ret == AAL_E_OK)
		return CA_E_OK;
	if (ret == AAL_E_TBLFULL)
		return CA_E_FULL;
	else if ((ret == AAL_E_NULLPTR) || (ret == AAL_E_OUTRANGE))
		return CA_E_PARAM;
	else if (ret == AAL_E_NOMEM)
		return CA_E_RESOURCE;
	else if (ret == AAL_E_ENTRY_USED)
		return CA_E_PARAM;
	else if (ret == AAL_E_EXIST)
		return CA_E_EXISTS;
	else
		return CA_E_NEXTHOP;

}

/* NextHop table is HashLite Engine, only allow to update hash action. */
ca_status_t ca_l3_nexthop_update (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_l3_nexthop_t		*nexthop,
	CA_IN	ca_l3_nexthop_id_t	nexthop_id)
{
	aal_hash_key_t hash_key;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3_generic_t *hash_action = (hl_act_l3_generic_t *) &hash_action_entry;
	int hash_idx_neighbor = -1;
	unsigned int mask_idx_neighbor;
	unsigned int hash_idx; /* temp hash_idx */
	int ret;
	struct nexthop_node *iter;
	aal_hashlite_aging_e hash_aging = HASHLITE_AGING_INVALID;
	ca_tunnel_cfg_t *tcfg = NULL;
	tunnel_info_t tinfo;

	if (nexthop == NULL)
		return CA_E_PARAM;

	if ((ret = nexthop_addr_check(nexthop)) != CA_E_OK)
		return ret;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		if (nexthop->addr.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: ipv4_addr=0x%08x / %u, attr_flags=%u, intf_id=0x%x, da_mac=%02x-%02x-%02x-%02x-%02x-%02x\n", __func__,
				nexthop->addr.ip_addr.ipv4_addr, nexthop->addr.addr_len, nexthop->attr_flags, nexthop->intf_id,
				nexthop->da_mac[0], nexthop->da_mac[1], nexthop->da_mac[2],
				nexthop->da_mac[3], nexthop->da_mac[4], nexthop->da_mac[5]);
			ca_printf(KERN_INFO "%s: aging_timer=%u, nexthop_id=%u\n", __func__, nexthop->aging_timer, nexthop_id);
		} else {
			ca_printf(KERN_INFO "%s: ipv6_addr=0x%08x 0x%08x 0x%08x 0x%08x / %u, attr_flags=%u, intf_id=0x%x, da_mac=%02x-%02x-%02x-%02x-%02x-%02x\n",
				__func__,
				nexthop->addr.ip_addr.ipv6_addr[0], nexthop->addr.ip_addr.ipv6_addr[1],
				nexthop->addr.ip_addr.ipv6_addr[2], nexthop->addr.ip_addr.ipv6_addr[3],
				nexthop->addr.addr_len, nexthop->attr_flags, nexthop->intf_id,
				nexthop->da_mac[0], nexthop->da_mac[1], nexthop->da_mac[2],
				nexthop->da_mac[3], nexthop->da_mac[4], nexthop->da_mac[5]);
			ca_printf(KERN_INFO "%s: aging_timer=%u, nexthop_id=%u\n", __func__, nexthop->aging_timer, nexthop_id);
		}
	}

	read_lock(&nexthop_table_lock);
	list_for_each_entry(iter, &nexthop_table, list) {
		if (iter->nexthop.nexthop_id == nexthop_id) {
			if (   (iter->nexthop.intf_id != nexthop->intf_id)
			    || (memcmp(&iter->nexthop.addr, &nexthop->addr, sizeof(ca_ip_address_t)) != 0) ) {
				 /* hash_key is different, this should be a different nexthop entry; upper layer must delete old then add new. */
				read_unlock(&nexthop_table_lock);
				return CA_E_NOT_SUPPORT;
			} else {
				/* found */
				break;
			}
		}
	}
	read_unlock(&nexthop_table_lock);
	if (iter->nexthop.nexthop_id != nexthop_id) return CA_E_NOT_FOUND;
	hash_idx_neighbor = iter->hash_idx_neighbor;

	if (ca_l3_intf_tbl[nexthop->intf_id].intf.type == CA_L3_INTF_TUNNEL) {
		if (   (g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id] == NULL)
		    || (g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id]->used == CA_FALSE) )
			return CA_E_ERROR; /* should not happen */
		tcfg = &g_tunnel[ca_l3_intf_tbl[nexthop->intf_id].intf.tunnel_id]->tcfg;
	}

	if ((memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) != 0) && (nexthop->addr.addr_len != 0)) {

		/* NextHop entry created from ARP or NDP */
		hash_aging = (nexthop->attr_flags == CA_L3_NHOP_ATTR_NEGH) ? HASHLITE_AGING_START : HASHLITE_AGING_STATIC;

	} else if ((memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) == 0) && (nexthop->addr.addr_len == 0)) {

		/* Two kinds of nexthop entries have zero da_mac & addr_len:
		 *   1. software-only
		 *   2. tunnel's nexthop
		 * But, only tunnel's nexthop could be updated to zero da_mac & addr_len.
		 * This is usually caused by the call flow in kernel hook.
		 * In this condition, invalid the hash entry; forward the packet to CPU.
		 */
		hash_aging = HASHLITE_AGING_INVALID;

	} else {

		/* Gateway route */
		if (memcmp(nexthop->da_mac, "\0\0\0\0\0\0", 6) == 0) {
			hash_aging = HASHLITE_AGING_INVALID;
		} else {
			hash_aging = HASHLITE_AGING_STATIC;
		}

	}

	/* If nexthop->addr.addr_len is 0 (host route usually), just add it into SW table; don't add it into HW table.
	 * Otherwise, unknown packet can't go to CPU for triggering an ARP.
	 */
	if (iter->sw_only) {
		goto _update_sw_nexthop;
	}

	if ((ret = aal_entry_get(AAL_TABLE_HASHLITE_MASK_TBL, &HM_L3_NEIGHBOR, &mask_idx_neighbor)) != AAL_E_OK) {
		ca_printf(KERN_ERR "%s: ERROR! Hashmask is not in HashLite Mask Table. (ret=%d)\n", __func__, ret);
		return ret;
	}

	/* fill new hash action */
	memset(&hash_action_entry, 0, sizeof(hash_action_entry));
	hash_action->ip_ttl_vld = 1;
	hash_action->ip_ttl_zero_discard_en = 1;
	hash_action->ip_ttl_cmd = 1;
	hash_action->l3_egress_if_vld = 1;
	hash_action->mdata_w_vld_1 = 0x3;			// enable mdata_1[7:0]
	hash_action->mdata_w_1 = nexthop->intf_id & 0xff;	// fill egress intf ID into mdata_1[7:0]
	hash_action->mac_da_vld = 1;
	hash_action->mac_da_0 = nexthop->da_mac[5];
	hash_action->mac_da_1 = nexthop->da_mac[4];
	hash_action->mac_da_2 = nexthop->da_mac[3];
	hash_action->mac_da_3 = nexthop->da_mac[2];
	hash_action->mac_da_4 = nexthop->da_mac[1];
	hash_action->mac_da_5 = nexthop->da_mac[0];		// MSB
	hash_action->chk_msk_ptr = mask_idx_neighbor;
	if (hash_aging == HASHLITE_AGING_INVALID) {
		hash_action->l3_egress_if_vld = 0;
		hash_action->mdata_w_vld_1 = 0x0;			// disable mdata_1[7:0]
		hash_action->mdata_w_1 = 0x00;				// clear the value
		hash_action->keep_orig_pkt = 1;
		hash_action->keep_orig_pkt_vld = 1;
		hash_action->dpid_vld = 1;
		hash_action->permit = 1;
		hash_action->mcgid = 0x10;
		hash_action->mc = 0;
	}

	if (tcfg != NULL) {
		memset(&tinfo, 0, sizeof(tinfo));
		tinfo.func_id = CA_FUNC_ID_INVALID;

		if (tcfg->type == CA_TUNNEL_PPPOE) {
			if ((ret = convert_nexthop_to_hashlite_pppoe(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_L2TP) {
			if ((ret = convert_nexthop_to_hashlite_l2tp(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_IPSEC) {
			if ((ret = convert_nexthop_to_hashlite_ipsec(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_PPTP) {
			if ((ret = convert_nexthop_to_hashlite_pptp(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_4_IN_4) {

		}

		if (tcfg->type == CA_TUNNEL_DSLITE) {
			if ((ret = convert_nexthop_to_hashlite_dslite(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6RD) {
			if ((ret = convert_nexthop_to_hashlite_6rd(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_6_IN_6) {

		}

		if (tcfg->type == CA_TUNNEL_MAP_E) {
			if ((ret = convert_nexthop_to_hashlite_map_e(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}

		if (tcfg->type == CA_TUNNEL_MAP_T) {
			if ((ret = convert_nexthop_to_hashlite_map_t(&ca_l3_intf_tbl[nexthop->intf_id].intf, hash_action, &tinfo, 0)) != CA_E_OK)
				return ret;
		}
	}

	if (hash_idx_neighbor >= 0) {
		/* Update for hash_idx_neighbor */

		hash_idx = hash_idx_neighbor & CA_L3_NEXTHOP_HASH_IDX_MASK;

		if (hash_idx_neighbor & CA_L3_NEXTHOP_ID_OVERFLOW_FLAG) {

			ret = aal_entry_set(AAL_TABLE_HASHLITE_OVERFLOW_ACTION, hash_idx, &hash_action_entry);
			if (ret != AAL_E_OK) {
				aal_entry_del(AAL_TABLE_HASHLITE_OVERFLOW_TBL, hash_idx);
				return CA_E_NEXTHOP;
			}

			ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_OVERFLOW_TBL, hash_idx, hash_aging);
			if (ret != AAL_E_OK) return CA_E_NEXTHOP;

		} else {

			ret = aal_entry_set(AAL_TABLE_HASHLITE_HASH_ACTION, hash_idx, &hash_action_entry);
			if (ret != AAL_E_OK) {
				aal_entry_del(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx);
				return CA_E_NEXTHOP;
			}

			ret = aal_hashlite_age_set(AAL_TABLE_HASHLITE_HASH_TBL, hash_idx, hash_aging);
			if (ret != AAL_E_OK) return CA_E_NEXTHOP;

		}

	} else {
		/* Add for hash_idx_neighbor */

		memset(&hash_key, 0, sizeof(hash_key));
		hash_key.ip_vld = 1;
		if (nexthop->addr.afi == CA_IPV4) {
			hash_key.ip_ver = 0;
			hash_key.ip_da_0 = nexthop->addr.ip_addr.ipv4_addr;
		} else {
			hash_key.ip_ver = 1;
			hash_key.ip_da_3 = nexthop->addr.ip_addr.ipv6_addr[0];	// MSB
			hash_key.ip_da_2 = nexthop->addr.ip_addr.ipv6_addr[1];
			hash_key.ip_da_1 = nexthop->addr.ip_addr.ipv6_addr[2];
			hash_key.ip_da_0 = nexthop->addr.ip_addr.ipv6_addr[3];
		}
		hash_key.hkey_id = mask_idx_neighbor;
		hash_key.mdata = nexthop->intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
		hash_key.ctrl_set_id = 0;			// profile#0 of HashLite

		ret = aal_hashlite_hash_add(0,
				&hash_key,
				&HM_L3_NEIGHBOR,
				&hash_action_entry,
				HL_ACTGRP_L3_GENERIC,
				hash_aging,
				&hash_idx_neighbor);
		if (ret != AAL_E_OK) {
			if (ca_api_debug & API_DBG_NEXTHOP) {
				ca_printf(KERN_ERR "%s: Fail to add hash for HM_L3_NEIGHBOR. (ret=%d)\n", __func__, ret);
			}
			return CA_E_NEXTHOP;
		}

	}

_update_sw_nexthop:
	/* update local nexthop table */
	write_lock(&nexthop_table_lock);
	list_for_each_entry(iter, &nexthop_table, list) {
		if (iter->nexthop.nexthop_id == nexthop_id) {
			iter->nexthop = *nexthop;
			iter->nexthop.nexthop_id = nexthop_id;
			iter->hash_idx_neighbor = hash_idx_neighbor;
		}
	}
	write_unlock(&nexthop_table_lock);

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf(KERN_INFO "%s: nexthop is updated, nexthop_id=%u (0x%x).\n", __func__, nexthop_id, nexthop_id);
		// nexthop_table_dump();
	}

	return CA_E_OK;
}

ca_status_t ca_l3_nexthop_delete (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_l3_nexthop_id_t	nexthop_id)
{
	int ret = CA_E_NOT_FOUND;
	struct nexthop_node *iter;
	bool found = false;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf(KERN_INFO "%s: nexthop_id=%u (0x%x)\n", __func__, nexthop_id, nexthop_id);
	}

	write_lock(&nexthop_table_lock);

	list_for_each_entry(iter, &nexthop_table, list) {
		if (iter->nexthop.nexthop_id == nexthop_id) {

			found = true;
			break;
		}
	}

	if (found) {
		if (nexthop_id < MIN_BM_SWONLY_NHID) {
			if (iter->hash_idx_neighbor >= 0) {
				ret = aal_hashlite_hash_delete(0, iter->hash_idx_neighbor);
				if (ret != AAL_E_OK) {
					write_unlock(&nexthop_table_lock);
					return CA_E_NEXTHOP;
				}
			}
		} else if ((nexthop_id >= MIN_BM_SWONLY_NHID) && (nexthop_id <= MAX_BM_SWONLY_NHID)) {
			bm_swonly_nhid_free(nexthop_id);
		} else {
			/* Bug! This is impossible! */
		}

		list_del(&iter->list);
		ca_free_trace(iter);
		ret = CA_E_OK;

		if (ca_api_debug & API_DBG_NEXTHOP) {
			ca_printf(KERN_INFO "%s: nexthop is deleted, nexthop_id=%u (0x%x).\n", __func__, nexthop_id, nexthop_id);
		}
	}

	write_unlock(&nexthop_table_lock);

	return ret;
}

ca_status_t ca_l3_nexthop_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_l3_nexthop_id_t	nexthop_id,
	CA_OUT		ca_l3_nexthop_t		*nexthop)
{
	int ret = CA_E_NOT_FOUND;
	struct nexthop_node *iter;
	bool found = false;

	if (nexthop == NULL)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf(KERN_INFO "%s: nexthop_id=%u (0x%x)\n", __func__, nexthop_id, nexthop_id);
	}

	read_lock(&nexthop_table_lock);

	list_for_each_entry(iter, &nexthop_table, list) {
		if (iter->nexthop.nexthop_id == nexthop_id) {
			found = true;
			break;
		}
	}

	if (!found) {
		if (ca_api_debug & API_DBG_NEXTHOP) {
			ca_printf(KERN_INFO "%s: nexthop_id=%u (0x%x) is not found.\n", __func__, nexthop_id, nexthop_id);
		}
		ret = CA_E_NOT_FOUND;
		goto _end_nexthop_get;
	}

	ret = nexthop_aging_time_update(iter);
	if (ret != CA_E_OK) goto _end_nexthop_get;

	*nexthop = iter->nexthop;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		ca_printf(KERN_INFO "%s: nexthop_id=%u (0x%x) is found.\n", __func__, nexthop_id, nexthop_id);
	}

_end_nexthop_get:
	read_unlock(&nexthop_table_lock);
	return ret;
}

ca_status_t ca_l3_nexthop_get_by_ip (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ip_address_t		ip_addr,
	CA_OUT		ca_l3_nexthop_t		*nexthop)
{
	int ret = CA_E_NOT_FOUND;
	struct nexthop_node *iter;
	bool found = false;

	if (nexthop == NULL)
		return CA_E_PARAM;

	if (ip_addr.afi == CA_IPV4) {
		/* IPv4, addr_len must be 0 or 32 */
		if (ip_addr.ip_addr.ipv4_addr == 0) {
			ip_addr.addr_len = 0;
		} else {
			ip_addr.ip_addr.ipv6_addr[1] = 0;
			ip_addr.ip_addr.ipv6_addr[2] = 0;
			ip_addr.ip_addr.ipv6_addr[3] = 0;
			ip_addr.addr_len = 32;
		}

		if (ca_api_debug & API_DBG_NEXTHOP) {
			ca_printf(KERN_INFO "%s: ip_addr=0x%08x/%d\n", __func__, ip_addr.ip_addr.ipv4_addr, ip_addr.addr_len);
		}
	} else {
		/* IPv6, addr_len must be 0 or 128 */
		if (  (ip_addr.ip_addr.ipv6_addr[0] == 0)
				&& (ip_addr.ip_addr.ipv6_addr[1] == 0)
				&& (ip_addr.ip_addr.ipv6_addr[2] == 0)
				&& (ip_addr.ip_addr.ipv6_addr[3] == 0) ) {
			ip_addr.addr_len = 0;
		} else {
			ip_addr.addr_len = 128;
		}

		if (ca_api_debug & API_DBG_NEXTHOP) {
			ca_printf(KERN_INFO "%s: ip_addr=%08x-%08x-%08x-%08x/%d\n", __func__,
				ip_addr.ip_addr.ipv6_addr[0], ip_addr.ip_addr.ipv6_addr[1],
				ip_addr.ip_addr.ipv6_addr[2], ip_addr.ip_addr.ipv6_addr[3], ip_addr.addr_len);
		}
	}

	read_lock(&nexthop_table_lock);

	list_for_each_entry(iter, &nexthop_table, list) {
		if (memcmp(&iter->nexthop.addr, &ip_addr, sizeof(ca_ip_address_t)) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		if (ca_api_debug & API_DBG_NEXTHOP) {
			if (ip_addr.afi == CA_IPV4) {
				ca_printf(KERN_INFO "%s: ip_addr=0x%08x/%d is NOT found.\n", __func__, ip_addr.ip_addr.ipv4_addr, ip_addr.addr_len);
			} else {
				ca_printf(KERN_INFO "%s: ip_addr=%08x-%08x-%08x-%08x/%d is NOT found.\n", __func__,
					ip_addr.ip_addr.ipv6_addr[0], ip_addr.ip_addr.ipv6_addr[1],
					ip_addr.ip_addr.ipv6_addr[2], ip_addr.ip_addr.ipv6_addr[3], ip_addr.addr_len);
			}
		}
		ret = CA_E_NOT_FOUND;
		goto _end_nexthop_get_by_ip;
	}

	ret = nexthop_aging_time_update(iter);
	if (ret != CA_E_OK) goto _end_nexthop_get_by_ip;

	*nexthop = iter->nexthop;

	if (ca_api_debug & API_DBG_NEXTHOP) {
		if (ip_addr.afi == CA_IPV4) {
			ca_printf(KERN_INFO "%s: ip_addr=0x%08x/%d is found.\n", __func__, ip_addr.ip_addr.ipv4_addr, ip_addr.addr_len);
		} else {
			ca_printf(KERN_INFO "%s: ip_addr=%08x-%08x-%08x-%08x/%d is found.\n", __func__,
				ip_addr.ip_addr.ipv6_addr[0], ip_addr.ip_addr.ipv6_addr[1],
				ip_addr.ip_addr.ipv6_addr[2], ip_addr.ip_addr.ipv6_addr[3], ip_addr.addr_len);
		}
	}

_end_nexthop_get_by_ip:
	read_unlock(&nexthop_table_lock);
	return ret;
}

ca_status_t ca_l3_nexthop_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ip_afi_t		afi,
	CA_OUT		ca_iterator_t		*return_entry )
{
	int ret;
	ca_status_t status;
	struct nexthop_node *iter, *tmp_iter;
	ca_l3_nexthop_t *ptr = NULL;
	ca_uint32_t count = 0;
	bool foundNode = false;

	if (return_entry == NULL)
		return CA_E_PARAM;

	if (return_entry->entry_data == NULL)
		return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_l3_nexthop_t) * return_entry->entry_count > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	ptr = (ca_l3_nexthop_t *) return_entry->entry_data;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) foundNode = true;

	read_lock(&nexthop_table_lock);
	list_for_each_entry_safe(iter, tmp_iter, &nexthop_table, list) {

		if (iter->nexthop.addr.afi != afi) continue;

		/* found the node stored in prev_handle, start to get contents from next node */
		if (foundNode == false) {
			if (iter == return_entry->prev_handle) foundNode = true;
			continue;
		}

		ret = nexthop_aging_time_update(iter);
		if (ret != CA_E_OK) {
			ca_printf(KERN_INFO "ERROR! why fail to update aging_time? nexthop_id=%d\n", iter->nexthop.nexthop_id);
			continue;
		}

		memcpy(ptr, &iter->nexthop, sizeof(iter->nexthop));
		return_entry->entry_index[count] = iter->nexthop.nexthop_id;
		ptr++;
		count++;
		if (count >= return_entry->entry_count) break;
	}
	read_unlock(&nexthop_table_lock);
	return_entry->entry_count = count;

	if (list_empty(&nexthop_table)) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* list_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

ca_status_t ca_l3_nexthop_aging_timer_set (
	CA_IN		ca_device_id_t	device_id,
	CA_IN		ca_uint32_t	time)
{
	int ret;

	ret = aal_hashlite_aging_timer_set(time);
	if (ret != AAL_E_OK)
		return CA_E_PARAM;

	return CA_E_OK;
}

ca_status_t ca_l3_nexthop_aging_timer_get (
	CA_IN		ca_device_id_t	device_id,
	CA_OUT		ca_uint32_t	*time)
{
	if (time == NULL)
		return CA_E_PARAM;

	aal_hashlite_aging_timer_get(time);

	return CA_E_OK;
}

/*
 * INPUT:
 *	  pppoe_session_id, mtu, priority
 *	  key->ethernet_type, dst_mac, tpid, vid, ip_da
 * CLS_KEY:
 *    pppoe_session_id
 *	  ethernet_type, key->dst_mac, tpid, vid, key->ip_da
 * EGR_ACTION:
 *    mtu check
 *    pppoe_session_id, tpid, vid, key->dst_mac
 */
ca_status_t ca_generic_intf_add(
	CA_IN  ca_device_id_t 			device_id,
	CA_IN  ca_gen_intf_attrib_t		*gen_intf_attrib)
{
	fe_l3e_if_tbl_entry_t eif;
	ca_status_t rc;
	unsigned int profile_id;
	l3_cls_rule_t rule;
	ca_uint32_t rslt_idx, mac_idx, mtu_idx;
	int ret;
	l3_cam_mac_da_tbl_entry_t l3_cam_mac;
	ca_uint8_t index;
	ca_l3_intf_t * intf;
	l3_cam_ethertype_tbl_entry_t ethertype;
	l3_cam_ppp_prot_tbl_entry_t ppp_prot;
	ca_uint32_t cam_ethertype_rslt_idx, cam_ppp_prot_rslt_idx;
	ca_intf_id_t		    intf_id;
	ca_uint16_t		        mtu = 0;
	ca_classifier_key_t      *key;
	ca_classifier_key_mask_t *key_mask;

	//check
	if (ca_l3_intf_init_done == 0) {
		ca_printf("%s: intf resource not init.\n", __func__);
		return CA_E_INIT;
	}

	if (gen_intf_attrib == NULL) {
		ca_printf("%s: intf param is null\n", __func__);
		return CA_E_PARAM;
	}

	intf_id = gen_intf_attrib->intf_id;
	key = &gen_intf_attrib->key;
	key_mask = &gen_intf_attrib->key_mask;
	if (gen_intf_attrib->action_mask.mtu == 1)
		mtu = gen_intf_attrib->mtu;

	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %u\n", __func__, intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf_id].use == 1) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: intf_id %d is used\n", __func__, intf_id);
		return CA_E_EXISTS;
	}

	memset(&ca_l3_intf_tbl[intf_id], 0x0, sizeof(struct ca_int_l3_intf_s));
	intf = &ca_l3_intf_tbl[intf_id].intf;
	intf->type = CA_L3_INTF_BCAST;				/* L3 Intface type */
	intf->intf_id = intf_id;					/* IP Interface ID */
	intf->port_id = key->src_port;				/* PHY port intf is bound to */
	intf->outer_tpid = key->l2.vlan_otag.vlan_min.tpid;		/* Outer TPID */
	intf->outer_vid = key->l2.vlan_otag.vlan_min.vid;		/* Outer VLAN */
	intf->tunnel_id = 0;						/* Tunnel Id */
	intf->mtu = mtu;							/* L3 MTU */
	intf->mask.nat_enable = gen_intf_attrib->action_mask.nat_enable;
	intf->nat_enable = gen_intf_attrib->nat_enable;		/* NAT enabled/disable */
	memcpy(&intf->mac_addr, &key->l2.mac_da.mac_min,
		sizeof(ca_mac_addr_t));					/* dst MAC address of packet*/
#if defined(CONFIG_LUNA_G3_SERIES)
	intf->mask.mac_addr = 1;
#endif	
	memcpy(&intf->ip_addr, &key->ip.ip_da,
		sizeof(ca_ip_address_t)); 				/* dst IP address of packet*/

	if (ca_api_debug & API_DBG_INTF) {
		ca_printf(KERN_INFO "=======\n %s: intf_id=%u, port_id=0x%x, type=%d, mac_addr=%02x-%02x-%02x-%02x-%02x-%02x\n",
			__func__, intf_id, intf->port_id, intf->type,
			intf->mac_addr[0], intf->mac_addr[1], intf->mac_addr[2],
			intf->mac_addr[3], intf->mac_addr[4], intf->mac_addr[5]);
		ca_printf(KERN_INFO "\t outer_vid=%u, mtu=%u, ip_addr=%08x-%08x-%08x-%08x, nat_enable=%u\n",
			intf->outer_vid, intf->mtu, intf->ip_addr.ip_addr.addr[3], intf->ip_addr.ip_addr.addr[2],
			intf->ip_addr.ip_addr.addr[1], intf->ip_addr.ip_addr.addr[0], intf->nat_enable);
	}

	mac_idx = CA_UINT32_INVALID;
	mtu_idx = CA_UINT32_INVALID;

	//mtu
	if (mtu) {
		rc = aal_l3pe_mtu_size_add(mtu, &mtu_idx);
		if (rc != AAL_E_OK) {
			ca_spin_unlock(ca_l3_intf_lock);
			ca_printf("%s: mtu add fail\n", __func__);
			return CA_E_INTF;
		}
	}

	//mac
	if ((key_mask->l2) && (key_mask->l2_mask.mac_da)) {
		memset(&l3_cam_mac, 0x0, sizeof(l3_cam_mac_da_tbl_entry_t));

		l3_cam_mac.mac_0 = key->l2.mac_da.mac_min[0];
		l3_cam_mac.mac_1 = key->l2.mac_da.mac_min[1];
		l3_cam_mac.mac_2 = key->l2.mac_da.mac_min[2];
		l3_cam_mac.mac_3 = key->l2.mac_da.mac_min[3];
		l3_cam_mac.mac_4 = key->l2.mac_da.mac_min[4];
		l3_cam_mac.mac_5 = key->l2.mac_da.mac_min[5];
		l3_cam_mac.vld   = 1;

		rc = aal_l3_cam_mac_da_lookup(key->l2.mac_da.mac_min, &mac_idx);
		if (rc == AAL_E_OK)
			rc = aal_entry_add_by_idx(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, mac_idx);
		else
			rc = aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, &mac_idx);
	}

	if (rc != AAL_E_OK) {
		if (mtu_idx != CA_UINT32_INVALID)
			aal_l3pe_mtu_size_del(mtu_idx);

		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: mac cam add fail\n", __func__);
		return CA_E_INTF;
	}

	/*
	 * egress intf action setup
	 */
	if ((rc = convert_intf_to_eif(intf, mtu_idx, 0 /*mac_idx*/, &eif)) != CA_E_OK) {
		ca_printf("%s: fail to create intf info to IF table entry. rc=%d\n", __func__, rc);
		goto CLASS_INTF_FAIL;
	}

	eif.ldpid = PORT_ID(key->src_port);

	if (gen_intf_attrib->action_mask.src_mac) {
		eif.mac_sa_vld = 0;
		eif.mac_sa_an_sel = 0;
	} else {
		eif.mac_sa_vld = 1;
		eif.mac_sa_an_sel = mac_idx + 1;
	}

	if (gen_intf_attrib->action_mask.egress_pppoe_action == 0) {/* for PPPoE Pass Through */
		eif.pppoe_set = 0;
	} else if ((gen_intf_attrib->key_pppoe_session_id == 0) ||
				(gen_intf_attrib->key_pppoe_session_id == CA_UINT16_INVALID)){ /* for PPPoE not eccapsulated */
		eif.pppoe_session_id = 0;
		eif.pppoe_vld = 1;
		eif.pppoe_set = 1;
	} else { /* for PPPoE eccapsulated */
		eif.pppoe_session_id = gen_intf_attrib->key_pppoe_session_id;
		eif.pppoe_vld = 1;
		eif.pppoe_set = 1;
	}
	if (gen_intf_attrib->action_mask.outer_vid == 0) {
		eif.vlan_vld = 0;
	} else if (gen_intf_attrib->action_mask.inner_tpid) {
		eif.vlan_cnt = key->l2.vlan_count;
		if (eif.vlan_cnt == 2) {
			if (aal_l3fe_pp_inner_tpid_get(key->l2.vlan_itag.vlan_min.tpid, &index) == AAL_E_OK) {
				eif.inner_tpid_enc = index;
				eif.inner_vid = key->l2.vlan_itag.vlan_min.vid;
				eif.inner_dei = 0;
				eif.inner_dei_sel = 0;
			} else {
				printk("why tpid %x cannot find??\n", key->l2.vlan_itag.vlan_min.tpid);
				eif.inner_tpid_enc = 0;
			}
		} else {
			eif.inner_tpid_enc = 0;
		}
	}
	/*
	 * deep_q == 0, refer to NI_HV_GLB_NIRX_L3FE_DEMUX_CFG, by default it will enter L2FE
	 * deep_q == 1, refer to NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG, by default it will enter L3QM
	 */
	eif.deep_q = 1; /*for not go back to L2FE*/

	rc = aal_entry_add_by_idx(AAL_TABLE_FE_L3E_IF_TBL, &eif, intf_id);
	if (rc != AAL_E_OK) {
		ca_printf("%s: eif table add fail, rc=%d\n", __func__, rc);
		goto CLASS_INTF_FAIL;
	}

	/*
	 * ingress classifier setup
	 */ 
	if ((rc = convert_intf_to_cls(intf, &profile_id, mac_idx, &rule)) != CA_E_OK) {
		ca_printf("%s: fail to create ingress cls key/action. rc=%d\n", __func__, rc);
		goto CLASS_INTF_FAIL;
	}
	intf->port_id = key->src_port;
#if defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)
	if((G3_LOOPBACK_P_START <= PORT_ID(intf->port_id)) && (PORT_ID(intf->port_id) <= G3_LOOPBACK_P_END))
	{
		rule.key.lspid = G3_LOOPBACK_P_NEWSPA;
		rule.key.msk_lspid = 0;
		rule.key.top_vid = G3_LOOPBACK_UPSTREAM_VID(PORT_ID(intf->port_id));
		rule.key.msk_top_vid = 0;
	}
	else
#endif
#endif
	rule.key.lspid = PORT_ID(intf->port_id);

	rule.key.vlan_cnt = key->l2.vlan_count;
	if (key->l2.vlan_count > 0) {
		if (rule.key.vlan_cnt == 2) {
			if (aal_l3fe_pp_inner_tpid_get(key->l2.vlan_itag.vlan_min.tpid, &index) == AAL_E_OK) {
				rule.key.msk_inner_tpid_enc = 0;
				rule.key.inner_tpid_enc = index;
				rule.key.msk_inner_vid = 0;
				rule.key.inner_vid = key->l2.vlan_itag.vlan_min.vid;
			} else {
				ca_printf("%s: fail to get ingress cls inner tpid idx. tpid=%x\n",
					__func__, key->l2.vlan_itag.vlan_min.tpid);
				goto CLASS_INTF_FAIL;
			}
		}
		rule.key.vid_ctl = 0;
	}

	if (mac_idx != CA_UINT32_INVALID) {
		rule.key.mac_da_an_sel = mac_idx + 1;
		rule.key.msk_mac_da_an_sel = 0;
	} else
		rule.key.msk_mac_da_an_sel = 1;

#if !defined(CONFIG_LUNA_G3_SERIES)	// DO NOT compare ip_da in NAT mode since we didn't seperate v4 and v6 rule.
	if ((key_mask->ip) && (key_mask->ip_mask.ip_da)) {
		if (key->ip.ip_da.afi == CA_IPV4) {
			rule.key.ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
			rule.key.msk_ip_da_l = 32;
		} else {
			rule.key.ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[0];
			rule.key.ip_da.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[1];
			rule.key.ip_da.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[2];
			rule.key.ip_da.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[3]; // MSB
			rule.key.msk_ip_da_l = 0;
			rule.key.msk_ip_da_h = 1;
		}
	}
#endif	

	if ((key_mask->ip) && (key_mask->ip_mask.ip_da)) {
		if (key->ip.ip_da.afi == CA_IPV4) {
			rule.key.ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
			rule.key.msk_ip_da_l = 32;
		} else {
			rule.key.ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[0];
			rule.key.ip_da.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[1];
			rule.key.ip_da.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[2];
			rule.key.ip_da.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[3]; // MSB
			rule.key.msk_ip_da_l = 0;
			rule.key.msk_ip_da_h = 1;
		}
	}
	if (gen_intf_attrib->key_pppoe_session_id == 0) {
		rule.key.pppoe_session_id = gen_intf_attrib->key_pppoe_session_id;
		rule.key.msk_pppoe_session = 0;
	} else if (gen_intf_attrib->key_pppoe_session_id != CA_UINT16_INVALID) {
		/* CAM table, Ethertype */
		memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
		ethertype.vld = 1;
		ethertype.ethertype = 0x8864; /* PPPoE Session */
		ret = aal_entry_get(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_ethertype_rslt_idx);
		if (ret != AAL_E_OK) {
			printk("%s: Fail to get Ethertype 0x8864 from CAM table, ret = %d\n", __func__, ret);
			goto CLASS_INTF_FAIL;
		}
#if !defined(CONFIG_LUNA_G3_SERIES)
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = (intf->ip_addr.afi == CA_IPV4) ? 0x0021 : 0x0057;
		ret = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (ret != AAL_E_OK) {
			printk("%s: Fail to get PPP Protocol from CAM table, ret = %d\n", __func__, ret);
			goto CLASS_INTF_FAIL;
		}
#endif
		rule.key.ethertype_enc = cam_ethertype_rslt_idx + 1;
		rule.key.msk_ethertype_enc = 0;
#if !defined(CONFIG_LUNA_G3_SERIES)		
		rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		rule.key.msk_ppp_protocol = 0;
#endif		
		rule.key.pppoe_session_id = gen_intf_attrib->key_pppoe_session_id;
		rule.key.msk_pppoe_session = 0;
#if defined(CONFIG_LUNA_G3_SERIES)
	}else if (gen_intf_attrib->key_pppoe_session_id == CA_UINT16_INVALID) {
		/* Disable this pattern for non PPPoE interface due to it always enable in convert_intf_to_cls() */
	 	rule.key.msk_pppoe_session = 1;
#endif
	}else {
		rule.key.pppoe_session_id = 0xffff;
		rule.key.msk_pppoe_session = 1;
	}

	if (gen_intf_attrib->key_sw_id != CA_UINT32_INVALID) {
		rule.key.msk_mdata = 0x3f;
		rule.key.mdata.mdata_0 = 0;
		rule.key.mdata.mdata_1 = gen_intf_attrib->key_sw_id & 0xffffff;
	}
	if (gen_intf_attrib->action_mask.sw_id == 1) {
		rule.action.mdata_w_2 = gen_intf_attrib->sw_id;
		rule.action.mdata_w_3 = ((gen_intf_attrib->sw_id & 0xffffff) >> 16);
		rule.action.mdata_w_vld_2 = 0xf;
		rule.action.mdata_w_vld_3 = 0x3;
	}

	rule.pri = gen_intf_attrib->prio;

#if defined(CONFIG_LUNA_G3_SERIES)
	{
		enum{
			CL_RUL_GENERICINTF_PRIO_LOW=0,
			CL_RUL_GENERICINTF_PRIO_MIDDLE=1,
			CL_RUL_GENERICINTF_PRIO_HIGH=2,
		};
		int hashProfile5tuple = T2_CTRL_BYPASS;
		int hashProfile2tuple = T2_CTRL_BYPASS;
		bool add5tuple = FALSE;
		bool add2tuple = FALSE;

		if((rule.key.lspid == AAL_LPORT_ETH_NI7) || (PORT_TYPE(key->src_port) == CA_PORT_TYPE_GPON) || (PORT_TYPE(key->src_port) == CA_PORT_TYPE_EPON)){
			// for physical (WAN) ports(0x7 or 0x6 ...) 
			hashProfile5tuple = l3fe_main_hash_profile_wan;
			hashProfile2tuple = l3fe_main_hash_profile_2tuple_bridge;
		}else{
			// for all physical (LAN) ports(0x0~0x6) and 11ac root+vap(0x12~0x16)
			hashProfile5tuple = l3fe_main_hash_profile_lan;
			hashProfile2tuple = l3fe_main_hash_profile_2tuple_bridge;
		}

		if(rule.key.msk_mac_da_an_sel==0){
			// routing/napt only support 5 tuple main hash.
			add5tuple = TRUE;
			add2tuple = FALSE;
		}else{
			// bridge support 5 tuple or 2 tuple main hash.
			add5tuple = TRUE;
			add2tuple = TRUE;
		}
		
		
		/* Per interface, seperate uc to two different mainhash profiles */
		ca_l3_intf_tbl[intf_id].cls_idx_uc5tuple = CA_UINT16_INVALID;
		ca_l3_intf_tbl[intf_id].cls_idx_uc2tuple = CA_UINT16_INVALID;
		ca_l3_intf_tbl[intf_id].cls_idx_mc = CA_UINT16_INVALID;
		
		/** with L4 header **/
		if(add5tuple){
			rule.pri = CL_RUL_GENERICINTF_PRIO_MIDDLE;
			rule.key.msk_ip_vld = 1;
			rule.key.ip_vld = 1;
			rule.key.msk_ip_ver = 1;	// don't care ip version
			rule.key.ip_ver = 1;
			rule.key.msk_l4_vld = 0;	// tcp udp rdp
			rule.key.l4_vld = 1;		// tcp udp rdp
			rule.action.t2_ctrl = hashProfile5tuple;
			rule.action.t2_ctrl_vld = 1;	
			//default controls
			rule.action.t3_ctrl_vld = 1;
			rule.action.t3_ctrl = 0xf;
			rule.action.t4_ctrl_vld = 1;
			if(PORT_ID(intf->port_id) <= AAL_LPORT_ETH_NI7)
				rule.action.t4_ctrl = PORT_ID(intf->port_id);
			else
				rule.action.t4_ctrl = 0xf;
			rule.action.stage2_ctrl_vld = 1;
			rule.action.stage2_ctrl = 0;		// Merge type-0 and type-1 result of Hash Engine
			rule.action.stage3_ctrl_vld = 1;
			rule.action.stage3_ctrl = 1;
			rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
			if (rc != AAL_E_OK) {
				ca_printf("%s: cls rule add fail, aal_l3_cls_add() returns %d.\n", __func__, rc);
				goto CLASS_INTF_FAIL;
			}
			ca_l3_intf_tbl[intf_id].cls_idx_uc5tuple = rslt_idx;
		}else
			ca_l3_intf_tbl[intf_id].cls_idx_uc5tuple = CA_UINT16_INVALID;

		/** without L4 header **/
		if(add2tuple){
			rule.pri = CL_RUL_GENERICINTF_PRIO_LOW;
			rule.key.msk_ip_vld = 1;	
			rule.key.ip_vld = 1;
			rule.key.msk_ip_ver = 1;	// don't care ip version
			rule.key.ip_ver = 1;	
			rule.key.msk_l4_vld = 1;	// don't care L4 (tcp udp rdp)	// care spa only - default cls rule per interface
			rule.key.l4_vld = 1;	
			rule.action.t2_ctrl = hashProfile2tuple;
			rule.action.t2_ctrl_vld = 1;
			//default controls
			rule.action.t3_ctrl_vld = 1;
			rule.action.t3_ctrl = 0xf;
			rule.action.t4_ctrl_vld = 1;
			if(PORT_ID(intf->port_id) <= AAL_LPORT_ETH_NI7)
				rule.action.t4_ctrl = PORT_ID(intf->port_id);
			else
				rule.action.t4_ctrl = 0xf;
			rule.action.stage2_ctrl_vld = 1;
			rule.action.stage2_ctrl = 0;		// Merge type-0 and type-1 result of Hash Engine
			rule.action.stage3_ctrl_vld = 1;
			rule.action.stage3_ctrl = 1;
			rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
			if (rc != AAL_E_OK) {
				ca_printf("%s: cls rule add fail, aal_l3_cls_add() returns %d.\n", __func__, rc);
				goto CLASS_INTF_FAIL;
			}
			ca_l3_intf_tbl[intf_id].cls_idx_uc2tuple = rslt_idx;
		}else
			ca_l3_intf_tbl[intf_id].cls_idx_uc2tuple = CA_UINT16_INVALID;
			

		/** add mc cls if pppoe interface **/
		if( gen_intf_attrib->key_pppoe_session_id != CA_UINT16_INVALID){
			rule.pri = CL_RUL_GENERICINTF_PRIO_HIGH;
			rule.key.msk_ip_vld = 0;
			rule.key.ip_vld = 1;
			rule.key.msk_ip_ver = 1;	// don't care ip version
			
			rule.key.msk_ip_sa_h = 0;
			rule.key.msk_ip_sa_l = 0;
			rule.key.msk_ip_da_h = 0;
			rule.key.msk_ip_da_l = 0;
			rule.key.msk_ip_addr_h = 0;
			rule.key.msk_ip_addr_l = 0;
			
			rule.key.ip_da_mc = 1;
			rule.key.msk_ip_da_mc = 0;
			rule.action.t2_ctrl = l3fe_main_hash_profile_mc;
			rule.action.t2_ctrl_vld = 1;
			rule.action.t4_ctrl_vld = 1;
			rule.action.t4_ctrl = 0xf;	//mc profile not need check uuc

			rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
			if (rc != AAL_E_OK) {
				ca_printf("%s: cls rule add fail, aal_l3_cls_add() returns %d.\n", __func__, rc);
				goto CLASS_INTF_FAIL;
			}
			ca_l3_intf_tbl[intf_id].cls_idx_mc = rslt_idx;
		}
	}
#else
	rc = aal_l3_cls_add(0, profile_id, &rule, &rslt_idx);
	if (rc != AAL_E_OK) {
		ca_printf("%s: cls rule add fail, aal_l3_cls_add() returns %d.\n", __func__, rc);
		goto CLASS_INTF_FAIL;
	}
#endif	

	//memcpy(&ca_l3_intf_tbl[intf_id].intf, intf, sizeof(ca_l3_intf_t));
	intf->type = CA_CLASS_INTF_CUSOMIZE;
	ca_l3_intf_tbl[intf_id].cls_profile = profile_id;
#if defined(CONFIG_LUNA_G3_SERIES)
	ca_l3_intf_tbl[intf_id].cls_idx = CA_UINT16_INVALID;
#else
	ca_l3_intf_tbl[intf_id].cls_idx = rslt_idx;
#endif
	ca_l3_intf_tbl[intf_id].mtu_idx = mtu_idx;
	ca_l3_intf_tbl[intf_id].mac_idx = mac_idx;
	ca_l3_intf_tbl[intf_id].use = 1;

	ca_spin_unlock(ca_l3_intf_lock);
	return CA_E_OK;

CLASS_INTF_FAIL:
	if (mac_idx != CA_UINT32_INVALID)
		aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, mac_idx);

	if (mtu_idx != CA_UINT32_INVALID)
		aal_l3pe_mtu_size_del(mtu_idx);

	aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf_id);

	ca_spin_unlock(ca_l3_intf_lock);

	ca_printf("%s: add failrc %d\n", __func__, rc);
	return rc;
}

ca_status_t ca_generic_intf_del(
	CA_IN  ca_device_id_t 			device_id,
	CA_IN  ca_intf_id_t		        intf_id)
{

	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %u\n", __func__, intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf_id].use == 0) {
		ca_spin_unlock(ca_l3_intf_lock);
		ca_printf("%s: intf_id %d is not used\n", __func__, intf_id);
		return CA_E_EXISTS;
	}

#if defined(CONFIG_LUNA_G3_SERIES)
	if(ca_l3_intf_tbl[intf_id].cls_idx_uc5tuple!=CA_UINT16_INVALID)
		aal_l3_cls_delete(0, ca_l3_intf_tbl[intf_id].cls_profile, ca_l3_intf_tbl[intf_id].cls_idx_uc5tuple);
	if(ca_l3_intf_tbl[intf_id].cls_idx_uc2tuple!=CA_UINT16_INVALID)
		aal_l3_cls_delete(0, ca_l3_intf_tbl[intf_id].cls_profile, ca_l3_intf_tbl[intf_id].cls_idx_uc2tuple);
	if(ca_l3_intf_tbl[intf_id].cls_idx_mc!=CA_UINT16_INVALID)
		aal_l3_cls_delete(0, ca_l3_intf_tbl[intf_id].cls_profile, ca_l3_intf_tbl[intf_id].cls_idx_mc);
	
#else
	aal_l3_cls_delete(0, ca_l3_intf_tbl[intf_id].cls_profile,
		ca_l3_intf_tbl[intf_id].cls_idx);

#endif

	aal_entry_del(AAL_TABLE_FE_L3E_IF_TBL, intf_id);

	if (ca_l3_intf_tbl[intf_id].mac_idx != CA_UINT8_INVALID)
		aal_entry_del(AAL_TABLE_L3_CAM_MAC_DA, ca_l3_intf_tbl[intf_id].mac_idx);

	if (ca_l3_intf_tbl[intf_id].intf.mtu)
		aal_l3pe_mtu_size_del(ca_l3_intf_tbl[intf_id].mtu_idx);

	ca_l3_intf_tbl[intf_id].use = 0;

	ca_spin_unlock(ca_l3_intf_lock);

#ifdef CONFIG_NE_L3COS
	/* After interface is added, try to update L3COS */
	ca_l3cos_update_all(device_id);
#endif

	return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_generic_intf_get (
	CA_IN		ca_device_id_t	device_id,
	CA_IN		ca_intf_id_t		intf_id,
	CA_OUT		struct ca_int_l3_intf_s	*intf)
	
{
	//check
	
	if (intf_id >= FE_L3E_IF_TBL_ENTRY_MAX) {
		ca_printf("%s: invalid intf_id %u\n", __func__, intf_id);
		return CA_E_INTF;
	}

	ca_spin_lock(ca_l3_intf_lock);

	if (ca_l3_intf_tbl[intf_id].use == 0) {
		ca_spin_unlock(ca_l3_intf_lock);
		return CA_E_NOT_FOUND;
	}

	memcpy(intf, &ca_l3_intf_tbl[intf_id], sizeof(struct ca_int_l3_intf_s));

	ca_spin_unlock(ca_l3_intf_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_generic_intf_get);

#endif

EXPORT_SYMBOL(ca_l3_intf_add);
EXPORT_SYMBOL(ca_l3_intf_delete);
EXPORT_SYMBOL(ca_l3_intf_get);
EXPORT_SYMBOL(ca_l3_intf_update);
EXPORT_SYMBOL(ca_l3_intf_iterate);
EXPORT_SYMBOL(ca_l3_intf_stats_clear);
EXPORT_SYMBOL(ca_l3_intf_stats_clear_all);
EXPORT_SYMBOL(ca_l3_intf_stats_get);
EXPORT_SYMBOL(ca_l3_route_add);
EXPORT_SYMBOL(ca_l3_route_delete);
EXPORT_SYMBOL(ca_l3_route_get);
EXPORT_SYMBOL(ca_l3_route_iterate);
EXPORT_SYMBOL(ca_l3_nexthop_add);
EXPORT_SYMBOL(ca_l3_nexthop_delete);
EXPORT_SYMBOL(ca_l3_nexthop_get);
EXPORT_SYMBOL(ca_l3_nexthop_get_by_ip);
EXPORT_SYMBOL(ca_l3_nexthop_update);
EXPORT_SYMBOL(ca_l3_nexthop_iterate);
EXPORT_SYMBOL(ca_l3_nexthop_aging_timer_set);
EXPORT_SYMBOL(ca_l3_nexthop_aging_timer_get);
EXPORT_SYMBOL(ca_generic_intf_add);
EXPORT_SYMBOL(ca_generic_intf_del);

