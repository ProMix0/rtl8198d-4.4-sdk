#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/crc32.h>
#include <linux/crc16.h>
#include "osal_common.h"
#include "aal_hash.h"
#include "nat.h"
#include "ca_mut.h"
#include "ca-scfg/scfg.h"

#define NAT_PROFILE_MAX			2
#define HASH_IDX_INVLD			0xFFFFFFFF
#define FE_L3E_NAT_TBL_ENTRY_MAX	65536

typedef struct {
	/* NAT table lookup missing action: 1 - to CPU, 0 - drop */
	ca_uint32_t	miss_to_cpu;
	/* 1 - Capture FIN/SYN/RST/SYN+ACK pacekts to CPU, 0 - forward as normal traffic */
	ca_boolean_t	tcp_ctrl_to_cpu;
	/* Unit in seconds */
	ca_uint32_t	aging_time;

	ca_uint8 tcp_ctrl_mask_vld;
	ca_uint32_t tcp_ctrl_mask_idx;
	ca_uint8 tcp_ctrl_tuple_vld[NAT_PROFILE_MAX];
	ca_uint8_t tcp_ctrl_tuple_idx[NAT_PROFILE_MAX];
	ca_uint32_t tcp_ctrl_fin_hash_idx[NAT_PROFILE_MAX];
	ca_uint32_t tcp_ctrl_syn_hash_idx[NAT_PROFILE_MAX];
	ca_uint32_t tcp_ctrl_rst_hash_idx[NAT_PROFILE_MAX];
	ca_uint32_t tcp_ctrl_syn_ack_hash_idx[NAT_PROFILE_MAX];
} nat_config_t;

struct nat_node {
	ca_uint32_t		crc32;		// crc32 of nat_entry
	ca_uint16_t		crc16;		// crc16 of nat_entry
	ca_nat_entry_t		nat_entry;
	ca_uint32_t		nat_id[NAT_PROFILE_MAX];
	struct hlist_node	list;
};

static atomic_t num_hash_entry = ATOMIC_INIT(0);
static atomic_t num_overflow_hash_entry = ATOMIC_INIT(0);
static ca_uint8_t nat_init_done = 0;
static ca_uint nat_lock;
static nat_config_t g_nat_config;

/* Each NAT entry is hashed a CRC32 and a CRC16 value.
 * nat_table is a hash table, it uses parts of CRC16 value to hash.
 * BITS_FOR_HASH defines the number of bits for hashing.
 * If BITS_FOR_HASH is 0, nat_table becomes just a list.
 * If BITS_FOR_HASH is 16, nat_table size is same as HW NAT table in some SoC.
 * The memory usage of nat_table is "sizeof(struct hlist_head) * nat_table_size".
 *
 * The valid range is 1 to 16.
 */
#define BITS_FOR_HASH	13
static struct hlist_head *nat_table = NULL;
static DEFINE_RWLOCK(nat_table_lock);
static int nat_table_size = 1 << BITS_FOR_HASH;
static int nat_table_mask = (1 << BITS_FOR_HASH) - 1;

static uint32_t nat_entry_crc32(uint32_t crc, unsigned char *p)
{
	/* NOTE:
	 * Only calculate the crc32 of five original tuples which are searching key.
	 * That is {ip_prot, orig_src_ip, orig_dst_ip, orig_src_port, orig_dst_port}.
	 */
	return ca_crc32(crc, p, offsetof(ca_nat_entry_t, xlate_flags));
}

static uint16_t nat_entry_crc16(uint16_t crc, unsigned char *p)
{
	/* NOTE:
	 * Only calculate the crc16 of five original tuples which are searching key.
	 * That is {ip_prot, orig_src_ip, orig_dst_ip, orig_src_port, orig_dst_port}.
	 */
	return ca_crc16(crc, p, offsetof(ca_nat_entry_t, xlate_flags));
}

/* nat->crc16 must be calculated before calling */
static void nat_table_hlist_add(struct nat_node *nat)
{
	if (nat == NULL) return;
	hlist_add_head(&nat->list, &nat_table[nat->crc16 & nat_table_mask]);
}

/* nat->crc16 must be calculated before calling */
static void nat_table_hlist_del(struct nat_node *nat)
{
	if (nat == NULL) return;
	hlist_del_init(&nat->list);
}

#if 0
static bool nat_table_hlist_empty(void)
{
	int i;

	for (i = 0; i < nat_table_size; i++) {
		if (hlist_empty(&nat_table[i]) == false)
			return false;
	}

	return true;
}
#endif

static inline void convert_ipv4addr_to_in_addr(ca_ip_address_t *ipv4_addr, struct in_addr *in_addr)
{
	if ((ipv4_addr == NULL) || (in_addr == NULL))
		return;

	in_addr->s_addr = htonl(ipv4_addr->ip_addr.ipv4_addr);
}

static inline void convert_ipv6addr_to_in6_addr(ca_ip_address_t *ipv6_addr, struct in6_addr *in6_addr)
{
	if ((ipv6_addr == NULL) || (in6_addr == NULL))
		return;

	in6_addr->s6_addr32[0] = htonl(ipv6_addr->ip_addr.ipv6_addr[0]);
	in6_addr->s6_addr32[1] = htonl(ipv6_addr->ip_addr.ipv6_addr[1]);
	in6_addr->s6_addr32[2] = htonl(ipv6_addr->ip_addr.ipv6_addr[2]);
	in6_addr->s6_addr32[3] = htonl(ipv6_addr->ip_addr.ipv6_addr[3]);
}

void nat_table_dump(void)
{
	struct nat_node *iter;
	int i, i_nat;
	unsigned int age = 0, time = 0;
	struct in_addr ipv4_1, ipv4_2, ipv4_3, ipv4_4;
	struct in6_addr ipv6_1, ipv6_2, ipv6_3, ipv6_4;

	ca_printf(KERN_INFO "DUMP nat_table:\n");
//	read_lock(&nat_table_lock); // may cause deadlock
	for (i_nat = 0; i_nat < nat_table_size; i_nat++) {
		hlist_for_each_entry(iter, &nat_table[i_nat], list) {
			ca_printf(KERN_INFO "crc32=0x%08x, crc16=0x%04x\n", iter->crc32, iter->crc16);
			if (iter->nat_entry.src_ip_addr.afi == CA_IPV4) {
				convert_ipv4addr_to_in_addr(&iter->nat_entry.src_ip_addr, &ipv4_1);
				convert_ipv4addr_to_in_addr(&iter->nat_entry.dst_ip_addr, &ipv4_2);
				convert_ipv4addr_to_in_addr(&iter->nat_entry.new_src_ip_addr, &ipv4_3);
				convert_ipv4addr_to_in_addr(&iter->nat_entry.new_dst_ip_addr, &ipv4_4);
				ca_printf(KERN_INFO "ip_proto=%u, orig_src_ip=%pI4:%u, orig_dst_ip=%pI4:%u -> new_src_ip=%pI4:%u, new_dst_ip=%pI4:%u\n",
					iter->nat_entry.ip_proto,
					&ipv4_1, iter->nat_entry.src_l4_port, &ipv4_2, iter->nat_entry.dst_l4_port,
					&ipv4_3, iter->nat_entry.new_src_l4_port, &ipv4_4, iter->nat_entry.new_dst_l4_port);
			} else {
				convert_ipv6addr_to_in6_addr(&iter->nat_entry.src_ip_addr, &ipv6_1);
				convert_ipv6addr_to_in6_addr(&iter->nat_entry.dst_ip_addr, &ipv6_2);
				convert_ipv6addr_to_in6_addr(&iter->nat_entry.new_src_ip_addr, &ipv6_3);
				convert_ipv6addr_to_in6_addr(&iter->nat_entry.new_dst_ip_addr, &ipv6_4);
				ca_printf(KERN_INFO "ip_proto=%u, orig_src_ip=[%pI6c]:%u, orig_dst_ip=[%pI6c]:%u -> new_src_ip=[%pI6c]:%u, new_dst_ip=[%pI6c]:%u\n",
					iter->nat_entry.ip_proto,
					&ipv6_1, iter->nat_entry.src_l4_port, &ipv6_2, iter->nat_entry.dst_l4_port,
					&ipv6_3, iter->nat_entry.new_src_l4_port, &ipv6_4, iter->nat_entry.new_dst_l4_port);
			}

			for (i = 0; i < NAT_PROFILE_MAX; i++) {
				if (iter->nat_id[i] != HASH_IDX_INVLD) {
					if (iter->nat_id[i] & CA_L3_NAT_ID_OVERFLOW_FLAG) {
						aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, iter->nat_id[i] & CA_L3_NAT_HASH_IDX_MASK, &age, &time);
					} else {
						aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, iter->nat_id[i], &age, &time);
					}
					ca_printf(KERN_INFO "hash_idx@profile#%d=%u (0x%x), aging_timer=%u [%s]\n",
							i, iter->nat_id[i], iter->nat_id[i], time,
							(age == HASH_AGING_STATIC) ? "STATIC" : ((age == HASH_AGING_INVALID) ? "EXPIRED" : "AGING"));

				}
			}
			ca_printf(KERN_INFO "-------------------------------------------------------------------------\n");
			cond_resched();
		}
	}
//	read_unlock(&nat_table_lock); // may cause deadlock
	ca_printf(KERN_INFO "miss_to_cpu = %u\n", g_nat_config.miss_to_cpu);
	ca_printf(KERN_INFO "tcp_ctrl_to_cpu = %u\n", g_nat_config.tcp_ctrl_to_cpu);
	ca_printf(KERN_INFO "aging_time = %u\n", g_nat_config.aging_time);
	ca_printf(KERN_INFO "tcp_ctrl_mask_vld = %u\n", g_nat_config.tcp_ctrl_mask_vld);
	ca_printf(KERN_INFO "tcp_ctrl_mask_idx = %u\n", g_nat_config.tcp_ctrl_mask_idx);
	for (i = 0; i < NAT_PROFILE_MAX; i++) {
		ca_printf(KERN_INFO "Profile#%d:\n", i);
		ca_printf(KERN_INFO "  tcp_ctrl_tuple_vld = %u\n", g_nat_config.tcp_ctrl_tuple_vld[i]);
		ca_printf(KERN_INFO "  tcp_ctrl_tuple_idx = %u\n", g_nat_config.tcp_ctrl_tuple_idx[i]);

		if (g_nat_config.tcp_ctrl_fin_hash_idx[i] != HASH_IDX_INVLD)
			ca_printf(KERN_INFO "  tcp_ctrl_fin_hash_idx = %u\n", g_nat_config.tcp_ctrl_fin_hash_idx[i]);
		else
			ca_printf(KERN_INFO "  tcp_ctrl_fin_hash_idx = INVALID\n");

		if (g_nat_config.tcp_ctrl_syn_hash_idx[i] != HASH_IDX_INVLD)
			ca_printf(KERN_INFO "  tcp_ctrl_syn_hash_idx = %u\n", g_nat_config.tcp_ctrl_syn_hash_idx[i]);
		else
			ca_printf(KERN_INFO "  tcp_ctrl_syn_hash_idx = INVALID\n");

		if (g_nat_config.tcp_ctrl_rst_hash_idx[i] != HASH_IDX_INVLD)
			ca_printf(KERN_INFO "  tcp_ctrl_rst_hash_idx = %u\n", g_nat_config.tcp_ctrl_rst_hash_idx[i]);
		else
			ca_printf(KERN_INFO "  tcp_ctrl_rst_hash_idx = INVALID\n");

		if (g_nat_config.tcp_ctrl_syn_ack_hash_idx[i] != HASH_IDX_INVLD)
			ca_printf(KERN_INFO "  tcp_ctrl_syn_ack_hash_idx = %u\n", g_nat_config.tcp_ctrl_syn_ack_hash_idx[i]);
		else
			ca_printf(KERN_INFO "  tcp_ctrl_syn_ack_hash_idx = INVALID\n");
	}
	ca_printf(KERN_INFO "Number of hash entry = %d\n", atomic_read(&num_hash_entry));
	ca_printf(KERN_INFO "Number of overflow hash entry = %d\n", atomic_read(&num_overflow_hash_entry));
}

ca_status_t ca_nat_config_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_nat_config_t	*config
)
{
	ca_status_t rc;
	ca_uint32_t profile_id;
	bool chg_tcp_ctrl_to_cpu, chg_aging_err=true, chg_miss_to_cpu_err=true, chg_tcp_ctrl_to_cpu_err=true;
	aal_hash_key_t hash_key;
	aal_hash_mask_t hash_mask;
	aal_hash_action_t hash_action;

	if (config == NULL)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_NAT) {
		ca_printf("%s: miss_to_cpu=%u, tcp_ctrl_to_cpu=%d, aging_time=%u.\n",
			__func__, config->miss_to_cpu, config->tcp_ctrl_to_cpu, config->aging_time);
	}

	ca_spin_lock(nat_lock);

	chg_tcp_ctrl_to_cpu = (g_nat_config.tcp_ctrl_to_cpu != config->tcp_ctrl_to_cpu) ? true : false;

	g_nat_config.aging_time = config->aging_time;
	if (AAL_E_OK != (rc = aal_hash_aging_timer_set(config->aging_time)))
		goto ERROR;
	chg_aging_err = false;

	/* set default action for profile 0 and 1 */
	for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
		if (CA_E_OK != (rc = aal_default_action_set(0, profile_id, config->miss_to_cpu)))
			goto ERROR;
	}
	g_nat_config.miss_to_cpu = config->miss_to_cpu;
	chg_miss_to_cpu_err = false;

	if (chg_tcp_ctrl_to_cpu) {
		if (config->tcp_ctrl_to_cpu == 1) {
			/* forward fin/syn/rst/syn+ack pacekts to cpu */
			memset(&hash_key, 0, sizeof(aal_hash_key_t));

			hash_mask_unmask(&hash_mask);
			hash_mask.ctrl_set_id = 0;
			hash_mask.ip_protocol = 0;
			hash_mask.tcp_rdp_ctrl = 0;
			if (CA_E_OK != (rc = aal_mask_tbl_entry_add(&hash_mask, &g_nat_config.tcp_ctrl_mask_idx)))
				goto ERROR;
			g_nat_config.tcp_ctrl_mask_vld = 1;

			memset(&hash_action, 0, sizeof(aal_hash_action_t));
			/* set hash action */
			hash_action.t5_ctrl_vld = 1;
			hash_action.t5_ctrl = 0xf;
			hash_action.t4_ctrl_vld = 1;
			hash_action.t4_ctrl = 0xf;
			hash_action.t3_ctrl_vld = 1;
			hash_action.t3_ctrl = 0xf;
			hash_action.stage3_ctrl_vld = 1;
			hash_action.stage3_ctrl = 0;
			hash_action.permit = 1;
			hash_action.dpid_vld = 1;
			hash_action.dpid_pri = 1;
			hash_action.mc = 0;
			hash_action.keep_orig_pkt = 1;
			hash_action.keep_orig_pkt_vld = 1;

			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				if (profile_id == HASH_PROFILE_0)
					hash_action.mcgid = AAL_LPORT_CPU_0;
				else
					hash_action.mcgid = AAL_LPORT_CPU_1;

				/* add hash mask to tuple */
				if (CA_E_OK != (rc = aal_hash_profile_tuple_add(profile_id, TUPLE_TYPE_FWD, 7, g_nat_config.tcp_ctrl_mask_idx,
					&g_nat_config.tcp_ctrl_tuple_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_tuple_vld[profile_id] = 1;

				/* create hash rule for fin */
				hash_key.ctrl_set_id = profile_id;
				hash_key.ip_protocol = 6;
				hash_key.tcp_rdp_ctrl = 0x1;
				if (CA_E_OK != (rc = aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, g_nat_config.tcp_ctrl_mask_idx,
					profile_id, &g_nat_config.tcp_ctrl_fin_hash_idx[profile_id])))
					goto ERROR;

				/* create hash rule for syn */
				hash_key.ctrl_set_id = profile_id;
				hash_key.ip_protocol = 6;
				hash_key.tcp_rdp_ctrl = 0x2;
				if (CA_E_OK != (rc = aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, g_nat_config.tcp_ctrl_mask_idx,
					profile_id, &g_nat_config.tcp_ctrl_syn_hash_idx[profile_id])))
					goto ERROR;

				/* create hash rule for rst */
				hash_key.ctrl_set_id = profile_id;
				hash_key.ip_protocol = 6;
				hash_key.tcp_rdp_ctrl = 0x4;
				if (CA_E_OK != (rc = aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, g_nat_config.tcp_ctrl_mask_idx,
					profile_id, &g_nat_config.tcp_ctrl_rst_hash_idx[profile_id])))
					goto ERROR;

				/* create hash rule for syn+ack */
				hash_key.ctrl_set_id = profile_id;
				hash_key.ip_protocol = 6;
				hash_key.tcp_rdp_ctrl = 0x12;
				if (CA_E_OK != (rc = aal_hash_add(0, &hash_key, &hash_action, TUPLE_TYPE_FWD, HASH_AGING_STATIC, g_nat_config.tcp_ctrl_mask_idx,
					profile_id, &g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id])))
					goto ERROR;
			}
		} else if (config->tcp_ctrl_to_cpu == 0) {
			/* forward as normal traffic */

			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				/* delete hash rule */
				if (CA_E_OK != (rc = aal_hash_delete(0, g_nat_config.tcp_ctrl_fin_hash_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_fin_hash_idx[profile_id] = HASH_IDX_INVLD;

				if (CA_E_OK != (rc = aal_hash_delete(0, g_nat_config.tcp_ctrl_syn_hash_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_syn_hash_idx[profile_id] = HASH_IDX_INVLD;

				if (CA_E_OK != (rc = aal_hash_delete(0, g_nat_config.tcp_ctrl_rst_hash_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_rst_hash_idx[profile_id] = HASH_IDX_INVLD;

				if (CA_E_OK != (rc = aal_hash_delete(0, g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id] = HASH_IDX_INVLD;

				/* delete tuple */
				if (CA_E_OK != (rc = aal_hash_profile_tuple_del(profile_id, g_nat_config.tcp_ctrl_tuple_idx[profile_id])))
					goto ERROR;
				g_nat_config.tcp_ctrl_tuple_vld[profile_id] = 0;
			}

			/* delete hash mask */
			if (CA_E_OK != (rc = aal_mask_tbl_entry_del(g_nat_config.tcp_ctrl_mask_idx)))
				goto ERROR;
			g_nat_config.tcp_ctrl_mask_vld = 0;

		} else {
			ca_spin_unlock(nat_lock);
			return CA_E_PARAM;
		}
		g_nat_config.tcp_ctrl_to_cpu = config->tcp_ctrl_to_cpu;
		chg_tcp_ctrl_to_cpu_err = false;
	}

	ca_spin_unlock(nat_lock);
	return CA_E_OK;

ERROR:
	if (chg_aging_err) {
		if (ca_api_debug & API_DBG_NAT)
			ca_printf("%s:%d aging_time = %d failed. rc = %d", __func__, __LINE__, config->aging_time, rc);
	}
	if (chg_miss_to_cpu_err) {
		if (ca_api_debug & API_DBG_NAT)
			ca_printf("%s:%d miss_to_cpu = %d failed. rc = %d", __func__, __LINE__, config->miss_to_cpu, rc);
	}
	if (chg_tcp_ctrl_to_cpu_err) {
		if (ca_api_debug & API_DBG_NAT)
			ca_printf("%s:%d tcp_ctrl_to_cpu = %d failed. rc = %d", __func__, __LINE__, config->tcp_ctrl_to_cpu, rc);

		for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
			if (g_nat_config.tcp_ctrl_fin_hash_idx[profile_id] != HASH_IDX_INVLD) {
				aal_hash_delete(0, g_nat_config.tcp_ctrl_fin_hash_idx[profile_id]);
				g_nat_config.tcp_ctrl_fin_hash_idx[profile_id] = HASH_IDX_INVLD;
			}
			if (g_nat_config.tcp_ctrl_syn_hash_idx[profile_id] != HASH_IDX_INVLD) {
				aal_hash_delete(0, g_nat_config.tcp_ctrl_syn_hash_idx[profile_id]);
				g_nat_config.tcp_ctrl_syn_hash_idx[profile_id] = HASH_IDX_INVLD;
			}
			if (g_nat_config.tcp_ctrl_rst_hash_idx[profile_id] != HASH_IDX_INVLD) {
				aal_hash_delete(0, g_nat_config.tcp_ctrl_rst_hash_idx[profile_id]);
				g_nat_config.tcp_ctrl_rst_hash_idx[profile_id] = HASH_IDX_INVLD;
			}
			if (g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id] != HASH_IDX_INVLD) {
				aal_hash_delete(0, g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id]);
				g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id] = HASH_IDX_INVLD;
			}
			if (g_nat_config.tcp_ctrl_tuple_vld[profile_id]) {
				aal_hash_profile_tuple_del(profile_id, g_nat_config.tcp_ctrl_tuple_idx[profile_id]);
				g_nat_config.tcp_ctrl_tuple_vld[profile_id] = 0;
			}
		}
		if (g_nat_config.tcp_ctrl_mask_vld) {
			aal_mask_tbl_entry_del(g_nat_config.tcp_ctrl_mask_idx);
			g_nat_config.tcp_ctrl_mask_vld = 0;
		}
	}

	ca_spin_unlock(nat_lock);
	return CA_E_ERROR;
}


ca_status_t ca_nat_config_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_nat_config_t	*config
)
{
	if (config == NULL)
		return CA_E_PARAM;

	ca_spin_lock(nat_lock);

	config->aging_time = g_nat_config.aging_time;
	config->miss_to_cpu = g_nat_config.miss_to_cpu;
	config->tcp_ctrl_to_cpu = g_nat_config.tcp_ctrl_to_cpu;

	ca_spin_unlock(nat_lock);
	return CA_E_OK;
}


ca_status_t ca_nat_entry_add(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_nat_entry_t	*nat_entry
)
{
	aal_hash_key_t hash_key;
	aal_hash_mask_t mask_entry;
	aal_hash_value_t hash_value;
	aal_hash_action_t hash_action;
	aal_hash_aging_e hash_age = HASH_AGING_6;
	unsigned int hash_idx[NAT_PROFILE_MAX], mask_id;
	int ret = CA_E_OK, profile_id = 0;
	struct nat_node *nat = NULL;
	struct nat_node *iter = NULL;
	struct hlist_node *p, *n;
	bool foundNode = false;
	bool entryExist = false;

	if (nat_entry == NULL)
		return CA_E_PARAM;

	if((nat_entry->src_ip_addr.afi == CA_IPV6) && (nat_entry->xlate_flags & CA_NAT_TRANSLATE_SRC_IP) &&
		(nat_entry->xlate_flags & CA_NAT_TRANSLATE_DST_IP))
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_NAT) {
		if (nat_entry->src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: src_ip_addr=0x%08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv4_addr, nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		} else {
			ca_printf("%s: src_ip_addr=0x%08x %08x %08x %08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		}

		if (nat_entry->dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: dst_ip_addr=0x%08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv4_addr, nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		} else {
			ca_printf("%s: dst_ip_addr=0x%08x %08x %08x %08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		}

		if (nat_entry->new_src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_src_ip_addr=0x%08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv4_addr, nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		} else {
			ca_printf("%s: new_src_ip_addr=0x%08x %08x %08x %08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		}

		if (nat_entry->new_dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_dst_ip_addr=0x%08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv4_addr, nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		} else {
			ca_printf("%s: new_dst_ip_addr=0x%08x %08x %08x %08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		}

		ca_printf("%s: ip_proto=0x%02x, xlate_flags=0x%x, aging_timer=%u\n",
			__func__, nat_entry->ip_proto, nat_entry->xlate_flags, nat_entry->aging_timer);
	}

	memset(&hash_key, 0, sizeof(hash_key));
	memset(&mask_entry, 0, sizeof(mask_entry));
	memset(&hash_value, 0, sizeof(hash_value));
	memset(&hash_action, 0, sizeof(hash_action));

	hash_key.ip_vld = 1;

	/* fill hash key */
	if (nat_entry->src_ip_addr.afi == CA_IPV4) {
		hash_key.ip_ver = 0;
		hash_key.ip_sa_0 = nat_entry->src_ip_addr.ip_addr.ipv4_addr;
		hash_key.ip_da_0 = nat_entry->dst_ip_addr.ip_addr.ipv4_addr;
	} else {
		hash_key.ip_ver = 1;
		hash_key.ip_sa_0 = nat_entry->src_ip_addr.ip_addr.ipv6_addr[3];
		hash_key.ip_sa_1 = nat_entry->src_ip_addr.ip_addr.ipv6_addr[2];
		hash_key.ip_sa_2 = nat_entry->src_ip_addr.ip_addr.ipv6_addr[1];
		hash_key.ip_sa_3 = nat_entry->src_ip_addr.ip_addr.ipv6_addr[0];
		hash_key.ip_da_0 = nat_entry->dst_ip_addr.ip_addr.ipv6_addr[3];
		hash_key.ip_da_1 = nat_entry->dst_ip_addr.ip_addr.ipv6_addr[2];
		hash_key.ip_da_2 = nat_entry->dst_ip_addr.ip_addr.ipv6_addr[1];
		hash_key.ip_da_3 = nat_entry->dst_ip_addr.ip_addr.ipv6_addr[0];
	}
	hash_key.ip_protocol = nat_entry->ip_proto;
	hash_key.l4_sp_exact_range = nat_entry->src_l4_port;
	hash_key.l4_dp_exact_range = nat_entry->dst_l4_port;
	mask_id = hash_mask_nat_idx;

	/* fill hash action */

	/* Enable change SIP */
	if(nat_entry->xlate_flags & CA_NAT_TRANSLATE_SRC_IP) {
		profile_id = hash_key.ctrl_set_id = 1;	// profile#1, LAN to WAN
		hash_action.ip_sa_l_vld = 1;
		if (nat_entry->new_src_ip_addr.afi == CA_IPV4) {
			hash_action.ip_sa_0 = nat_entry->new_src_ip_addr.ip_addr.ipv4_addr;
		} else {
			hash_action.ip_sa_h_vld = 1;
			hash_action.ip_sa_0 = nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[3];
			hash_action.ip_sa_1 = nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[2];
			hash_action.ip_sa_2 = nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[1];
			hash_action.ip_sa_3 = nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[0];
		}
	}

	/* Enable change DIP */
	if(nat_entry->xlate_flags & CA_NAT_TRANSLATE_DST_IP) {
		profile_id = hash_key.ctrl_set_id = 0;	// profile#0, WAN to LAN
		hash_action.ip_da_l_vld = 1;
		if (nat_entry->new_dst_ip_addr.afi == CA_IPV4) {
			hash_action.ip_da_0 = nat_entry->new_dst_ip_addr.ip_addr.ipv4_addr;
		} else {
			hash_action.ip_da_h_vld = 1;
			hash_action.ip_da_0 = nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[3];
			hash_action.ip_da_1 = nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[2];
			hash_action.ip_da_2 = nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[1];
			hash_action.ip_da_3 = nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[0];
		}
	}

	if(nat_entry->xlate_flags & CA_NAT_TRANSLATE_L4_PORT) {
		hash_action.l4_dp_vld = 1;
		hash_action.l4_dp = nat_entry->new_dst_l4_port;
		hash_action.l4_sp_vld = 1;
		hash_action.l4_sp = nat_entry->new_src_l4_port;
	}

	hash_age = (nat_entry->xlate_flags & CA_NAT_TRANSLATE_STATIC) ? HASH_AGING_STATIC : HASH_AGING_START;

	hash_idx[0] = hash_idx[1] = HASH_IDX_INVLD;
	if (nat_entry->xlate_flags & CA_NAT_TRANSLATE_DST_IP &&
		nat_entry->xlate_flags & CA_NAT_TRANSLATE_SRC_IP) {
		/* add hash for profile 0 and 1 */
		for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
			hash_key.ctrl_set_id = profile_id;
			ret = aal_hash_add(device_id, &hash_key, &hash_action, TUPLE_TYPE_FWD, hash_age, mask_id, profile_id, &hash_idx[profile_id]);
			if(ret != CA_E_OK) {
				if (ret == CA_E_EXISTS) {
					if (ca_api_debug & API_DBG_NAT)
						ca_printf("%s:%d hash entry exists, hash_idx=%u\n", __func__, __LINE__, hash_idx[profile_id]);
					entryExist = true; /* There may be more for-loop, don't goto ERROR now */
					continue;
				} else {
					if (ca_api_debug & API_DBG_NAT)
						ca_printf("%s:%d aal_hash_add failed. ret = %d\n", __func__, __LINE__, ret);
					goto ERROR;
				}
			}
			if (hash_idx[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
				atomic_inc(&num_overflow_hash_entry);
			else
				atomic_inc(&num_hash_entry);
		}
	} else {
		ret = aal_hash_add(device_id, &hash_key, &hash_action, TUPLE_TYPE_FWD, hash_age, mask_id, profile_id, &hash_idx[profile_id]);
		if(ret != CA_E_OK) {
			if (ret == CA_E_EXISTS) {
				if (ca_api_debug & API_DBG_NAT)
					ca_printf("%s:%d hash entry exists, hash_idx=%u\n", __func__, __LINE__, hash_idx[profile_id]);
				entryExist = true;
				goto ERROR2;
			} else {
				if (ca_api_debug & API_DBG_NAT)
					ca_printf("%s:%d aal_hash_add failed. ret = %d\n", __func__, __LINE__, ret);
				goto ERROR;
			}
		}
		if (hash_idx[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
			atomic_inc(&num_overflow_hash_entry);
		else
			atomic_inc(&num_hash_entry);
	}

	/* entry exists already, don't create/insert nat_node again */
	if (entryExist)
		goto ERROR2;

	/* add into local nat table */
	nat = ca_zalloc_trace(sizeof(struct nat_node), GFP_ATOMIC, true);
	if (nat == NULL) {
		ret = CA_E_MEMORY;
		goto ERROR;
	}

	for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++)
		nat->nat_id[profile_id] = hash_idx[profile_id];
	nat->crc32 = nat_entry_crc32(0, (unsigned char *) nat_entry);
	nat->crc16 = nat_entry_crc16(0, (unsigned char *) nat_entry);
	nat->nat_entry = *nat_entry;
	write_lock_bh(&nat_table_lock);
	hlist_for_each_safe(p, n, &nat_table[nat->crc16 & nat_table_mask]) {
		iter = hlist_entry(p, typeof(*iter), list);
		if(iter->crc32 == nat->crc32 && iter->crc16 == nat->crc16) {
			/* found existed node with same crc32 / crc16, assume it is same NAT entry */
			foundNode = true;
			break;
		}
	}
	if (foundNode) {
		/* merge nat->nat_id[] into iter->nat_id[] */
		for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++)
			if (nat->nat_id[profile_id] != HASH_IDX_INVLD) {
				if (iter->nat_id[profile_id] == HASH_IDX_INVLD) {
					iter->nat_id[profile_id] = nat->nat_id[profile_id];
				} else {
					if (iter->nat_id[profile_id] == nat->nat_id[profile_id]) {
						/* Could this happens? */
						continue;
					}

					/* Already a hash entry created for this profile but different hash_idx, delete the new one.
					 * Usually happens in following condition:
					 * 1. Traffic keeps coming, OS queue contains several packets of same IP addr & L4 ports.
					 * 2. Thread#1: Handle 1st packet in OS queue, add hash entry to Overflow Table, add NAT entry to nat_table.
					 * 3. Thread#2: Flushing all NAT entries, and 1st packet is not yet flushed.
					 * 4. Thread#1: Handle 2nd packet in OS queue, add hash entry to Hash Table (some are flushed in Hash Table),
					 *              but fail to add NAT entry to nat_table.
					 */
					if (ca_api_debug & API_DBG_NAT) {
					ca_printf(KERN_INFO "%s: ERROR! Already created a hash entry for profile %d.\n", __func__, profile_id);
					ca_printf(KERN_INFO "%s: Created hash_idx=%u, new hash_idx=%u\n", __func__, iter->nat_id[profile_id], nat->nat_id[profile_id]);
						ca_printf(KERN_INFO "%s: Delete the new one.\n", __func__);
					}
					aal_hash_delete(device_id, nat->nat_id[profile_id]);
					if (nat->nat_id[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
						atomic_dec(&num_overflow_hash_entry);
					else
						atomic_dec(&num_hash_entry);
				}
			}

	} else {
		nat_table_hlist_add(nat);
	}
	write_unlock_bh(&nat_table_lock);

	if (ca_api_debug & API_DBG_NAT) {
		for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
			if (hash_idx[profile_id] == HASH_IDX_INVLD)
				continue;
			ca_printf(KERN_INFO "%s: Profile %d NAT entry is added, hash_idx=%u (0x%x).\n", __func__, profile_id, hash_idx[profile_id], hash_idx[profile_id]);
		}
	}

	return CA_E_OK;

ERROR:
	/* delete the existed hashs */
	for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
		if (hash_idx[profile_id] == HASH_IDX_INVLD)
			continue;
		aal_hash_delete(device_id, hash_idx[profile_id]);
		if (hash_idx[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
			atomic_dec(&num_overflow_hash_entry);
		else
			atomic_dec(&num_hash_entry);
	}
	if (ca_api_debug & API_DBG_NAT) {
		ca_printf(KERN_INFO "%s: Fail to add NAT entry. (ret=%d)\n", __func__, ret);
	}
ERROR2:
	return ret;
}

ca_status_t ca_nat_entry_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_nat_entry_t	*nat_entry
)
{
	unsigned int age = 0, time[NAT_PROFILE_MAX], e_crc32, e_crc16, profile_id, e_vld = 0, e_del = 0;
	unsigned int hash_idx;
	int ret;
	struct nat_node *iter;
	struct hlist_node *p, *n;

	if (nat_entry == NULL)
		return CA_E_PARAM;

	if((nat_entry->src_ip_addr.afi == CA_IPV6) && (nat_entry->xlate_flags & CA_NAT_TRANSLATE_SRC_IP) &&
		(nat_entry->xlate_flags & CA_NAT_TRANSLATE_DST_IP))
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_NAT) {
		if (nat_entry->src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: src_ip_addr=0x%08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv4_addr, nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		} else {
			ca_printf("%s: src_ip_addr=0x%08x %08x %08x %08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		}

		if (nat_entry->dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: dst_ip_addr=0x%08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv4_addr, nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		} else {
			ca_printf("%s: dst_ip_addr=0x%08x %08x %08x %08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		}

		if (nat_entry->new_src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_src_ip_addr=0x%08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv4_addr, nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		} else {
			ca_printf("%s: new_src_ip_addr=0x%08x %08x %08x %08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		}

		if (nat_entry->new_dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_dst_ip_addr=0x%08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv4_addr, nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		} else {
			ca_printf("%s: new_dst_ip_addr=0x%08x %08x %08x %08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		}

		ca_printf("%s: ip_proto=0x%02x, xlate_flags=0x%x, aging_timer=%u\n",
			__func__, nat_entry->ip_proto, nat_entry->xlate_flags, nat_entry->aging_timer);
	}

	e_crc32 = nat_entry_crc32(0, (unsigned char *) nat_entry);
	e_crc16 = nat_entry_crc16(0, (unsigned char *) nat_entry);

	write_lock_bh(&nat_table_lock);
	hlist_for_each_safe(p, n, &nat_table[e_crc16 & nat_table_mask]) {
		iter = hlist_entry(p, typeof(*iter), list);
		if(iter->crc32 == e_crc32 && iter->crc16 == e_crc16) {

			iter->nat_entry.aging_timer = 0;

			/* check hash age */
			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				time[profile_id] = 0;
				if (iter->nat_id[profile_id] == HASH_IDX_INVLD)
					continue;
				e_vld++;
				if (iter->nat_id[profile_id] & CA_L3_NAT_ID_OVERFLOW_FLAG) {
					hash_idx = iter->nat_id[profile_id] & CA_L3_NAT_HASH_IDX_MASK;
					ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx, &age, &time[profile_id]);
				} else {
					hash_idx = iter->nat_id[profile_id];
					ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, hash_idx, &age, &time[profile_id]);
				}

				if (ret != AAL_E_OK) continue;

				/* use larger time as its aging_timer */
				if (time[profile_id] > iter->nat_entry.aging_timer)
					iter->nat_entry.aging_timer = time[profile_id];
			}

			if (e_vld == e_del) {
				nat_table_hlist_del(iter);
				ca_free_trace(iter);
			} else {
				*nat_entry = iter->nat_entry;
				write_unlock_bh(&nat_table_lock);
				return CA_E_OK;
			}
		}

		// cond_resched(); // may cause deadlock
	}
	write_unlock_bh(&nat_table_lock);

	return CA_E_NOT_FOUND;
}

ca_status_t ca_nat_entry_iterate (
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_iterator_t	*return_entry )
{
	unsigned int age = 0, time[NAT_PROFILE_MAX], profile_id, e_vld = 0, e_del = 0;
	unsigned int hash_idx;
	int ret;
	struct nat_node *iter, *last_iter_node;
	struct hlist_node *n;
	int i_nat;
	ca_nat_entry_t *ptr = NULL;
	ca_uint32_t count = 0;
	bool foundNode = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL)
		return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (sizeof(ca_nat_entry_t) * return_entry->entry_count > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	ptr = (ca_nat_entry_t *) return_entry->entry_data;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) foundNode = true;

	write_lock_bh(&nat_table_lock);
	for (i_nat = 0; i_nat < nat_table_size; i_nat++) {
		hlist_for_each_entry_safe(iter, n, &nat_table[i_nat], list) {
			/* found the node stored in prev_handle, start to get contents */
			if ((foundNode == false) && (iter == return_entry->prev_handle)) {
				foundNode = true;
				continue;
			}
			if (foundNode == false) continue;

			iter->nat_entry.aging_timer = 0;

			/* check hash age */
			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				time[profile_id] = 0;
				if (iter->nat_id[profile_id] == HASH_IDX_INVLD)
					continue;
				e_vld++;
				if (iter->nat_id[profile_id] & CA_L3_NAT_ID_OVERFLOW_FLAG) {
					hash_idx = iter->nat_id[profile_id] & CA_L3_NAT_HASH_IDX_MASK;
					ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx, &age, &time[profile_id]);
				} else {
					hash_idx = iter->nat_id[profile_id];
					ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, hash_idx, &age, &time[profile_id]);
				}

				if (ret != AAL_E_OK) continue;

				/* use larger time as its aging_timer */
				if (time[profile_id] > iter->nat_entry.aging_timer)
					iter->nat_entry.aging_timer = time[profile_id];
			}

			if (e_vld == e_del) {
				nat_table_hlist_del(iter);
				ca_free_trace(iter);
			} else {
				*ptr = iter->nat_entry;
				last_iter_node = iter;
				ptr++;
				count++;
				if (count >= return_entry->entry_count) break;
			}
		}

		if (count >= return_entry->entry_count) break; /* leave all loops */
	}
	write_unlock_bh(&nat_table_lock);

	return_entry->entry_count = count;

	if(iter == NULL && count != 0)
		return_entry->prev_handle = last_iter_node;
	else
		return_entry->prev_handle = iter;

	if (count == 0)
		return CA_E_UNAVAIL;
	else
		return CA_E_OK;
}

ca_status_t ca_nat_entry_delete(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_nat_entry_t	*nat_entry
)
{
	int ret, e_crc32, e_crc16, profile_id;
	struct nat_node *iter;
	struct hlist_node *p, *n;
	bool found_node = false;

	if((nat_entry->src_ip_addr.afi == CA_IPV6) && (nat_entry->xlate_flags & CA_NAT_TRANSLATE_SRC_IP) &&
		(nat_entry->xlate_flags & CA_NAT_TRANSLATE_DST_IP))
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_NAT) {
		if (nat_entry->src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: src_ip_addr=0x%08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv4_addr, nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		} else {
			ca_printf("%s: src_ip_addr=0x%08x %08x %08x %08x / %u, src_l4_port=%u\n", __func__,
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->src_ip_addr.addr_len, nat_entry->src_l4_port);
		}

		if (nat_entry->dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: dst_ip_addr=0x%08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv4_addr, nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		} else {
			ca_printf("%s: dst_ip_addr=0x%08x %08x %08x %08x / %u, dst_l4_port=%u\n", __func__,
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->dst_ip_addr.addr_len, nat_entry->dst_l4_port);
		}

		if (nat_entry->new_src_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_src_ip_addr=0x%08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv4_addr, nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		} else {
			ca_printf("%s: new_src_ip_addr=0x%08x %08x %08x %08x / %u, new_src_l4_port=%u\n", __func__,
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_src_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_src_ip_addr.addr_len, nat_entry->new_src_l4_port);
		}

		if (nat_entry->new_dst_ip_addr.afi == CA_IPV4) {
			ca_printf("%s: new_dst_ip_addr=0x%08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv4_addr, nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		} else {
			ca_printf("%s: new_dst_ip_addr=0x%08x %08x %08x %08x / %u, new_dst_l4_port=%u\n", __func__,
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[0], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[1],
				nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[2], nat_entry->new_dst_ip_addr.ip_addr.ipv6_addr[3],
				nat_entry->new_dst_ip_addr.addr_len, nat_entry->new_dst_l4_port);
		}

		ca_printf("%s: ip_proto=0x%02x, xlate_flags=0x%x, aging_timer=%u\n",
			__func__, nat_entry->ip_proto, nat_entry->xlate_flags, nat_entry->aging_timer);
	}

	e_crc32 = nat_entry_crc32(0, (unsigned char *) nat_entry);
	e_crc16 = nat_entry_crc16(0, (unsigned char *) nat_entry);

	write_lock_bh(&nat_table_lock);
	hlist_for_each_safe(p, n, &nat_table[e_crc16 & nat_table_mask]) {
		iter = hlist_entry(p, typeof(*iter), list);
		if(iter->crc32 == e_crc32 && iter->crc16 == e_crc16) {
			/* find the entry */
			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				if (iter->nat_id[profile_id] == HASH_IDX_INVLD)
					continue;
				ret = aal_hash_delete(device_id, iter->nat_id[profile_id]);
				if (ret != AAL_E_OK) {
					if (ca_api_debug & API_DBG_NAT)
						ca_printf("%s:%d nat_id[%d]=%u, error_code = %d\n",
							__func__, __LINE__, profile_id, iter->nat_id[profile_id], ret);
					write_unlock_bh(&nat_table_lock);
					return CA_E_ERROR;
				}
				if (iter->nat_id[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
					atomic_dec(&num_overflow_hash_entry);
				else
					atomic_dec(&num_hash_entry);
				if (ca_api_debug & API_DBG_NAT) {
					ca_printf("%s: Delete NAT entry, hash_idx=%u (0x%x).\n",
						__func__, iter->nat_id[profile_id], iter->nat_id[profile_id]);
				}
				iter->nat_id[profile_id] = HASH_IDX_INVLD;
				found_node = true;
			}
			nat_table_hlist_del(iter);
			ca_free_trace(iter);
		}

		/* NOTE: This API may run in soft_irq, and cond_resched() may sleep, so it can't be called. */
		// cond_resched();
	}
	write_unlock_bh(&nat_table_lock);

	if(found_node)
		return CA_E_OK;
	else
		return CA_E_NOT_FOUND;
}

ca_status_t ca_nat_entry_delete_all(
	CA_IN	ca_device_id_t	device_id
)
{
	unsigned int profile_id;
	struct nat_node *iter;
	struct hlist_node *n;
	int i_nat;

	write_lock_bh(&nat_table_lock);
	for (i_nat = 0; i_nat < nat_table_size; i_nat++) {
		hlist_for_each_entry_safe(iter, n, &nat_table[i_nat], list) {
			for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
				if (iter->nat_id[profile_id] == HASH_IDX_INVLD)
					continue;
				aal_hash_delete(device_id, iter->nat_id[profile_id]);
				if (iter->nat_id[profile_id] & HASH_HASH_IDX_OVERFLOW_FLAG)
					atomic_dec(&num_overflow_hash_entry);
				else
					atomic_dec(&num_hash_entry);
				if (ca_api_debug & API_DBG_NAT) {
					ca_printf("%s: Delete NAT entry, hash_idx=%u (0x%x).\n",
							__func__, iter->nat_id[profile_id], iter->nat_id[profile_id]);
				}
			}

			nat_table_hlist_del(iter);
			ca_free_trace(iter);
		}

		cond_resched();
	}
	write_unlock_bh(&nat_table_lock);

	return CA_E_OK;
}

ca_status_t ca_nat_init(ca_device_id_t device_id)
{
	ca_uint32_t profile_id;
	ca_status_t ret;
	int i;

	if (nat_init_done)
		return CA_E_OK;

	if (CA_OSAL_OK != ca_spin_lock_init(&nat_lock, SPINLOCK_CMN)) {
		ca_printf("%s: init spinlock fail\n", __func__);
		return CA_E_INIT;
	}

	if (BITS_FOR_HASH < 0) nat_table_size = 1 << 0;
	if (BITS_FOR_HASH > 16) nat_table_size = 1 << 16;
	nat_table = ca_malloc_trace(sizeof(struct hlist_head) * nat_table_size, GFP_KERNEL, true);
	if (nat_table == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory for internal NAT table.\n", __func__);
		return CA_E_RESOURCE;
	}
	for (i = 0; i < nat_table_size; i++) {
		INIT_HLIST_HEAD(&nat_table[i]);
	}

	/* default value in g_nat_config */
	/* IMPORTANT:
	 *   Default 'miss_to_cpu' must match the type-0 default action in hash_profile_0 and hash_profile_1.
	 *   Default 'tcp_ctrl_to_cpu' must be 0 because no hash is created at this moment.
	 */
	memset(&g_nat_config, 0, sizeof(nat_config_t));
	g_nat_config.miss_to_cpu = 1;
	g_nat_config.tcp_ctrl_to_cpu = 0;
	g_nat_config.aging_time = HASH_INIT_AGE_TIME;
	for (profile_id = 0; profile_id < NAT_PROFILE_MAX; profile_id++) {
		g_nat_config.tcp_ctrl_fin_hash_idx[profile_id] = HASH_IDX_INVLD;
		g_nat_config.tcp_ctrl_syn_hash_idx[profile_id] = HASH_IDX_INVLD;
		g_nat_config.tcp_ctrl_rst_hash_idx[profile_id] = HASH_IDX_INVLD;
		g_nat_config.tcp_ctrl_syn_ack_hash_idx[profile_id] = HASH_IDX_INVLD;
	}

	ret = aal_scfg_read(CFG_ID_NAT_AGING_TIME, sizeof(g_nat_config.aging_time), (ca_uint8_t *) &g_nat_config.aging_time);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to read CFG_ID_NAT_AGING_TIME. (ret=%d)\n", __func__, ret);
		g_nat_config.aging_time = HASH_INIT_AGE_TIME; /* seconds */
	}
	aal_hash_aging_timer_set(g_nat_config.aging_time);

	nat_init_done = 1;

	return CA_E_OK;
}

EXPORT_SYMBOL(ca_nat_config_set);
EXPORT_SYMBOL(ca_nat_config_get);
EXPORT_SYMBOL(ca_nat_entry_add);
EXPORT_SYMBOL(ca_nat_entry_get);
EXPORT_SYMBOL(ca_nat_entry_iterate);
EXPORT_SYMBOL(ca_nat_entry_delete);
EXPORT_SYMBOL(ca_nat_entry_delete_all);
