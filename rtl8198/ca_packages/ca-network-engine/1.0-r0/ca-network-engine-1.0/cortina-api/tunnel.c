#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/slab.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/l2tp.h>
#include "tunnel.h"
#include "tunnel_ipc.h"
#include "sw_macsec.h"
#include "route.h"
#include "ca_aal.h"
#include "aal_l3fe.h"
#include "aal_l3_pe.h"
#include "aal_l3_cls.h"
#include "aal_l3_if.h"
#include "aal_hashlite.h"
#include "aal_lpm.h"
#include "aal_port.h"
#include "aal_ni.h"
#include "aal_sa_id.h"
#include "aal_idx4pe.h"
#include "api_common.h"
#include "ca_mut.h"
#include "ca-scfg/scfg.h"

/* num is bit offset from MSB, the first bit from MSB is num 0 */
#define bitset128(target, num)		(((uint32_t *)(target))[(num) / 32] |= (1 << (32 - 1 - ((num) % 32))))
#define bitset32(target, num)		(((uint32_t *)(target))[0] |= (1 << (32 - 1 - (num))))

#define bitcheck128(target, num)	(((uint32_t *)(target))[(num) / 32] & (1 << (32 - 1 - ((num) % 32))))
#define bitcheck32(target, num)		((target) & (1 << (32 - 1 - (num))))
#define bitcheck16(target, num)		((target) & (1 << (16 - 1 - (num))))

ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];

static bool cls_rule_for_tunnel = false; /* flag of creating CLS rule for PE to L3FE (upstream & downstream) */
static ca_uint32 pe_ds_rslt_idx = 0; /* CLS rslt_idx for downstream */
static ca_uint32 pe_us_rslt_idx = 0; /* CLS rslt_idx for upstream */

extern struct ca_int_l3_intf_s *ca_l3_intf_tbl;

static void tunnel_cfg_dump(ca_tunnel_cfg_t *tcfg);
static void ipsec_sa_dump(ca_ipsec_sa_t *ipsec_sa);
static void pptp_sa_dump(ca_pptp_sa_t *pptp_sa);
static void macsec_sa_dump(ca_macsec_sa_t *macsec_sa);

/***************************************************************************
 * Common functions
 ***************************************************************************/

static void tunnel_private_data_dump(ca_tunnel_t *t)
{
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if ((t == NULL) || (t->private_data == NULL)) return;

	ca_printf(KERN_INFO "\n");
	ca_printf(KERN_INFO "  Private Data:\n");

	if (t->tcfg.type == CA_TUNNEL_L2TP) {
		l2tp_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    sa_id_outbound = %u\n", priv->sa_id_outbound);
		ca_printf(KERN_INFO "    sa_id_inbound = %u\n", priv->sa_id_inbound);
		ca_printf(KERN_INFO "    idx4pe = %u\n", priv->idx4pe);
	}

	if (t->tcfg.type == CA_TUNNEL_IPSEC) {
		ipsec_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    hash_idx_upstream = %u\n", priv->hash_idx_upstream);
		ca_printf(KERN_INFO "    hash_idx_downstream = %u\n", priv->hash_idx_downstream);
		ca_printf(KERN_INFO "    idx4pe = %u\n", priv->idx4pe);
	}

	if (t->tcfg.type == CA_TUNNEL_PPTP) {
		pptp_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    sa_id_outbound = %u\n", priv->sa_id_outbound);
		ca_printf(KERN_INFO "    sa_id_inbound = %u\n", priv->sa_id_inbound);
	}

	if (t->tcfg.type == CA_TUNNEL_DSLITE) {
		dslite_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    aal_idx = %d\n", priv->aal_idx);
	}

	if (t->tcfg.type == CA_TUNNEL_6RD) {
		sixrd_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    aal_idx = %d\n", priv->aal_idx);
	}

	if ((t->tcfg.type == CA_TUNNEL_MAP_E) || (t->tcfg.type == CA_TUNNEL_MAP_T)) {
		map_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    aal_idx = %d\n", priv->aal_idx);
		ca_printf(KERN_INFO "    offload_in_ne = %u\n", priv->offload_in_ne);
		ca_printf(KERN_INFO "    idx4pe = %u\n", priv->idx4pe);

		if (priv->ce_ipv4.afi == CA_IPV4) {
			int o_r_len = t->tcfg.tunnel.map.ea_bit_length + t->tcfg.tunnel.map.ipv4_prefix.addr_len; /* o bits + r bits */
			ipv4_addr.s_addr = htonl(priv->ce_ipv4.ip_addr.ipv4_addr);
			ca_printf(KERN_INFO "    ce_ipv4 = %pI4 / %u ", &ipv4_addr, priv->ce_ipv4.addr_len);
			if (o_r_len < 32)
				ca_printf("(IPv4 prefix)\n");
			else if (o_r_len == 32)
				ca_printf("(Complete IPv4 address)\n");
			else
				ca_printf("(Shared IPv4 address)\n");
		} else {
			ipv6_addr.in6_u.u6_addr32[0] = htonl(priv->ce_ipv4.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(priv->ce_ipv4.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(priv->ce_ipv4.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(priv->ce_ipv4.ip_addr.ipv6_addr[3]);
			ca_printf(KERN_INFO "    ce_ipv4 = %pI6 / %u (ERROR)\n", &ipv6_addr, priv->ce_ipv4.addr_len);
		}
	}

	if (t->tcfg.type == CA_TUNNEL_MACSEC) {
		macsec_private_data_t *priv = t->private_data;
		ca_printf(KERN_INFO "    idx4pe = %u\n", priv->idx4pe);
	}
}

static void tunnel_cfg_dump(ca_tunnel_cfg_t *tcfg)
{
	ca_status_t ret;
	int i;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (tcfg == NULL) return;

	ca_printf(KERN_INFO "  type = %d ", tcfg->type);
	switch (tcfg->type) {
	case CA_TUNNEL_PPPOE:
		ca_printf("[PPPoE]\n"); break;
	case CA_TUNNEL_L2TP:
		ca_printf("[L2TP]\n"); break;
	case CA_TUNNEL_IPSEC:
		ca_printf("[IPSec]\n"); break;
	case CA_TUNNEL_PPTP:
		ca_printf("[PPTP]\n"); break;
	case CA_TUNNEL_4_IN_4:
		ca_printf("[IPv4 in IPv4]\n"); break;
	case CA_TUNNEL_DSLITE:
		ca_printf("[DS-Lite]\n"); break;
	case CA_TUNNEL_6RD:
		ca_printf("[6RD]\n"); break;
	case CA_TUNNEL_6_IN_6:
		ca_printf("[IPv6 in IPv6]\n"); break;
	case CA_TUNNEL_MAP_E:
		ca_printf("[MAP-E]\n"); break;
	case CA_TUNNEL_MAP_T:
		ca_printf("[MAP-T]\n"); break;
	case CA_TUNNEL_MACSEC:
		ca_printf("[MACSec]\n"); break;
	case CA_TUNNELINVALID:
	default:
		ca_printf("[INVALID]\n"); break;
	}

	if (tcfg->src_addr.afi == CA_IPV4) {
		ipv4_addr.s_addr = htonl(tcfg->src_addr.ip_addr.ipv4_addr);
		ca_printf(KERN_INFO "  src_addr = %pI4 / %u\n", &ipv4_addr, tcfg->src_addr.addr_len);
	} else {
		ipv6_addr.in6_u.u6_addr32[0] = htonl(tcfg->src_addr.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(tcfg->src_addr.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(tcfg->src_addr.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(tcfg->src_addr.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "  src_addr = %pI6 / %u\n", &ipv6_addr, tcfg->src_addr.addr_len);
	}

	if (tcfg->dest_addr.afi == CA_IPV4) {
		ipv4_addr.s_addr = htonl(tcfg->dest_addr.ip_addr.ipv4_addr);
		ca_printf(KERN_INFO "  dest_addr = %pI4 / %u\n", &ipv4_addr, tcfg->dest_addr.addr_len);
	} else {
		ipv6_addr.in6_u.u6_addr32[0] = htonl(tcfg->dest_addr.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(tcfg->dest_addr.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(tcfg->dest_addr.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(tcfg->dest_addr.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "  dest_addr = %pI6 / %u\n", &ipv6_addr, tcfg->dest_addr.addr_len);
	}

	ca_printf(KERN_INFO "  parent_l3_intf_id = %u\n", tcfg->parent_l3_intf_id);

	if (tcfg->type == CA_TUNNEL_PPPOE) {
		ca_printf(KERN_INFO "  PPPoE info:\n");
		ca_printf(KERN_INFO "    mac_da = %pM\n", tcfg->tunnel.pppoe.mac_da);
		ca_printf(KERN_INFO "    pppoe_session_id = %u\n", tcfg->tunnel.pppoe.pppoe_session_id);
	}

	if (tcfg->type == CA_TUNNEL_L2TP) {
		ca_printf(KERN_INFO "  L2TP info:\n");
		ca_printf(KERN_INFO "    version = 0x%04u\n", tcfg->tunnel.l2tp.version);
		ca_printf(KERN_INFO "    len = %u\n", tcfg->tunnel.l2tp.len);
		ca_printf(KERN_INFO "    tunnel_id = %u\n", tcfg->tunnel.l2tp.tunnel_id);
		ca_printf(KERN_INFO "    src_l4_port = %u\n", tcfg->tunnel.l2tp.src_l4_port);
		ca_printf(KERN_INFO "    dest_l4_port = %u\n", tcfg->tunnel.l2tp.dest_l4_port);
		ca_printf(KERN_INFO "    session_id = %u\n", tcfg->tunnel.l2tp.session_id);
		ca_printf(KERN_INFO "    encap_type = %u\n", tcfg->tunnel.l2tp.encap_type);
		ca_printf(KERN_INFO "    l2_specific_len = %u\n", tcfg->tunnel.l2tp.l2_specific_len);
		ca_printf(KERN_INFO "    l2_specific_type = %u\n", tcfg->tunnel.l2tp.l2_specific_type);
		ca_printf(KERN_INFO "    send_seq = %u\n", tcfg->tunnel.l2tp.send_seq);
		ca_printf(KERN_INFO "    calc_udp_csum = %u\n", tcfg->tunnel.l2tp.calc_udp_csum);
		ca_printf(KERN_INFO "    sequence_number = %u\n", tcfg->tunnel.l2tp.sequence_number);
		ca_printf(KERN_INFO "    cookie_len = %u\n", tcfg->tunnel.l2tp.cookie_len);

		ca_printf(KERN_INFO "    cookie = ");
		for (i = 0; i < tcfg->tunnel.l2tp.cookie_len; i++)
			ca_printf("%02x ", tcfg->tunnel.l2tp.cookie[i]);
		ca_printf("\n");

		ca_printf(KERN_INFO "    offset = %u\n", tcfg->tunnel.l2tp.offset);
		ca_printf(KERN_INFO "    l2tp_src_mac = %pM\n", tcfg->tunnel.l2tp.l2tp_src_mac);
		ca_printf(KERN_INFO "    peer_l2tp_src_mac = %pM\n", tcfg->tunnel.l2tp.peer_l2tp_src_mac);
	}

	if (tcfg->type == CA_TUNNEL_IPSEC) {
		ca_ipsec_sa_t ipsec_sa;

		ca_printf(KERN_INFO "  IPSec info:\n");
		ca_printf(KERN_INFO "    sa_id_encrypt = %u\n", tcfg->tunnel.ipsec.sa_id_encrypt);
		ca_printf(KERN_INFO "    sa_id_decrypt = %u\n", tcfg->tunnel.ipsec.sa_id_decrypt);

		ret = ca_ipsec_sa_get(0, tcfg->tunnel.ipsec.sa_id_encrypt, &ipsec_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Encryption IPSec SA info:\n");
			ipsec_sa_dump(&ipsec_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump encryption IPSec SA. (ret %u)\n", ret);
		}

		ret = ca_ipsec_sa_get(0, tcfg->tunnel.ipsec.sa_id_decrypt, &ipsec_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Decryption IPSec SA info:\n");
			ipsec_sa_dump(&ipsec_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump decryption IPSec SA. (ret %u)\n", ret);
		}
	}

	if (tcfg->type == CA_TUNNEL_PPTP) {
		ca_pptp_sa_t pptp_sa;

		ca_printf(KERN_INFO "  PPTP info:\n");
		ca_printf(KERN_INFO "    sa_id_outbound = %u\n", tcfg->tunnel.pptp.sa_id_outbound);
		ca_printf(KERN_INFO "    sa_id_inbound = %u\n", tcfg->tunnel.pptp.sa_id_inbound);

		ret = ca_pptp_sa_get(0, tcfg->tunnel.pptp.sa_id_outbound, &pptp_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Outbound PPTP SA info:\n");
			pptp_sa_dump(&pptp_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump outbound PPTP SA. (ret %u)\n", ret);
		}

		ret = ca_pptp_sa_get(0, tcfg->tunnel.pptp.sa_id_inbound, &pptp_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Inbound PPTP SA info:\n");
			pptp_sa_dump(&pptp_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump inbound PPTP SA. (ret %u)\n", ret);
		}
	}

	if (tcfg->type == CA_TUNNEL_DSLITE) {
		ca_printf(KERN_INFO "  DS-Lite info:\n");
		ca_printf(KERN_INFO "    dsl_ipmc_illegal_check_en = %u\n", tcfg->tunnel.dslite.dsl_ipmc_illegal_check_en);
		ca_printf(KERN_INFO "    dsl_ipmc_to_l2fe = %u\n", tcfg->tunnel.dslite.dsl_ipmc_to_l2fe);
		ca_printf(KERN_INFO "    dsl_ipmc_addr_prefix_check_en = %u\n", tcfg->tunnel.dslite.dsl_ipmc_addr_prefix_check_en);
		ca_printf(KERN_INFO "    dsl_ipmc_addr_consistency_check_en = %u\n", tcfg->tunnel.dslite.dsl_ipmc_addr_consistency_check_en);
		ca_printf(KERN_INFO "    dsl_rpf_check_en = %u\n", tcfg->tunnel.dslite.dsl_reverse_path_check_en);
		ca_printf(KERN_INFO "    dsl_ttl_keep_outer = %u\n", tcfg->tunnel.dslite.dsl_ttl_keep_outer);
		ca_printf(KERN_INFO "    dsl_dscp_keep_outer = %u\n", tcfg->tunnel.dslite.dsl_dscp_keep_outer);
		ca_printf(KERN_INFO "    dsl_ecn_keep_outer = %u\n", tcfg->tunnel.dslite.dsl_ecn_keep_outer);
		ca_printf(KERN_INFO "    dsl_ecn_check_enable = %u\n", tcfg->tunnel.dslite.dsl_ecn_check_enable);
		ca_printf(KERN_INFO "    rebuild_mc_mac = %u\n", tcfg->tunnel.dslite.rebuild_mc_mac);
		ca_printf(KERN_INFO "    mc_flag = %u\n", tcfg->tunnel.dslite.mc_flag);
		ca_printf(KERN_INFO "    mc_mac = %pM\n", tcfg->tunnel.dslite.mc_mac);
		ca_printf(KERN_INFO "    mc_drop = %u\n", tcfg->tunnel.dslite.mc_drop);
		ca_printf(KERN_INFO "    validation_check_failed_dest_port = 0x%x\n", tcfg->tunnel.dslite.validation_check_failed_dest_port);
		ca_printf(KERN_INFO "    egress_tc_value = %u\n", tcfg->tunnel.dslite.egress_tc_value);
		ca_printf(KERN_INFO "    egress_hoplimit_value = %u\n", tcfg->tunnel.dslite.egress_hoplimit_value);
		ca_printf(KERN_INFO "    egress_flow_label_value = %u\n", tcfg->tunnel.dslite.egress_flow_label_value);
	}

	if (tcfg->type == CA_TUNNEL_6RD) {
		ca_printf(KERN_INFO "  6RD info:\n");
		if (tcfg->tunnel.six_rd.ipv6_6rd_prefix.afi == CA_IPV4) {
			/* Print it anyway though it is wrong config. */
			ipv4_addr.s_addr = htonl(tcfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv4_addr);
			ca_printf(KERN_INFO "    ipv6_6rd_prefix = %pI4 / %u (ERROR)\n",
					&ipv4_addr, tcfg->tunnel.six_rd.ipv6_6rd_prefix.addr_len);
		} else {
			ipv6_addr.in6_u.u6_addr32[0] = htonl(tcfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(tcfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(tcfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(tcfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[3]);
			ca_printf(KERN_INFO "    ipv6_6rd_prefix = %pI6 / %u\n",
					&ipv6_addr, tcfg->tunnel.six_rd.ipv6_6rd_prefix.addr_len);
		}

		ca_printf(KERN_INFO "    six_rd_v4_mask_len = %u\n", tcfg->tunnel.six_rd.six_rd_v4_mask_len);
		ca_printf(KERN_INFO "    six_rd_ipda_from_v6 = %u\n", tcfg->tunnel.six_rd.six_rd_ipda_from_v6);
		ca_printf(KERN_INFO "    six_rd_v6_prefix_len = %u\n", tcfg->tunnel.six_rd.six_rd_v6_prefix_len);
		ca_printf(KERN_INFO "    six_rd_ipmc_illegal_check_en = %u\n", tcfg->tunnel.six_rd.six_rd_ipmc_illegal_check_en);
		ca_printf(KERN_INFO "    six_rd_ttl_keep_outer = %u\n", tcfg->tunnel.six_rd.six_rd_ttl_keep_outer);
		ca_printf(KERN_INFO "    six_rd_dscp_keep_outer = %u\n", tcfg->tunnel.six_rd.six_rd_dscp_keep_outer);
		ca_printf(KERN_INFO "    six_rd_ecn_keep_outer = %u\n", tcfg->tunnel.six_rd.six_rd_ecn_keep_outer);
		ca_printf(KERN_INFO "    six_rd_ecn_check_enable = %u\n", tcfg->tunnel.six_rd.six_rd_ecn_check_enable);
		ca_printf(KERN_INFO "    rebuild_mc_mac = %u\n", tcfg->tunnel.six_rd.rebuild_mc_mac);
		ca_printf(KERN_INFO "    mc_mac = %pM\n", tcfg->tunnel.six_rd.mc_mac);
		ca_printf(KERN_INFO "    mc_drop = %u\n", tcfg->tunnel.six_rd.mc_drop);
		ca_printf(KERN_INFO "    validation_check_failed_dest_port = 0x%x\n", tcfg->tunnel.six_rd.validation_check_failed_dest_port);
		ca_printf(KERN_INFO "    ce_ce_allowed = %u\n", tcfg->tunnel.six_rd.ce_ce_allowed);
		ca_printf(KERN_INFO "    egress_tos_value = %u\n", tcfg->tunnel.six_rd.egress_tos_value);
		ca_printf(KERN_INFO "    egress_ttl_value = %u\n", tcfg->tunnel.six_rd.egress_ttl_value);
		ca_printf(KERN_INFO "    egress_identification_start = %u\n", tcfg->tunnel.six_rd.egress_identification_start);
		ca_printf(KERN_INFO "    egress_identification_end = %u\n", tcfg->tunnel.six_rd.egress_identification_end);
		ca_printf(KERN_INFO "    six_rd_ipda_from_v6 = %u\n", tcfg->tunnel.six_rd.six_rd_ipda_from_v6);
	}

	if ((tcfg->type == CA_TUNNEL_MAP_E) || (tcfg->type == CA_TUNNEL_MAP_T)) {
		if (tcfg->type == CA_TUNNEL_MAP_E)
		ca_printf(KERN_INFO "  MAP-E info:\n");
		else
			ca_printf(KERN_INFO "  MAP-T info:\n");
		ca_printf(KERN_INFO "    map_type = %u\n", tcfg->tunnel.map.map_type);

		if (tcfg->tunnel.map.ipv6_prefix.afi == CA_IPV4) {
			/* Print it anyway though it is wrong config. */
			ipv4_addr.s_addr = htonl(tcfg->tunnel.map.ipv6_prefix.ip_addr.ipv4_addr);
			ca_printf(KERN_INFO "    ipv6_prefix = %pI4 / %u (ERROR)\n",
					&ipv4_addr, tcfg->tunnel.map.ipv6_prefix.addr_len);
		} else {
			ipv6_addr.in6_u.u6_addr32[0] = htonl(tcfg->tunnel.map.ipv6_prefix.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(tcfg->tunnel.map.ipv6_prefix.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(tcfg->tunnel.map.ipv6_prefix.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(tcfg->tunnel.map.ipv6_prefix.ip_addr.ipv6_addr[3]);
			ca_printf(KERN_INFO "    ipv6_prefix = %pI6 / %u\n", &ipv6_addr, tcfg->tunnel.map.ipv6_prefix.addr_len);
		}

		if (tcfg->tunnel.map.ipv4_prefix.afi == CA_IPV4) {
			ipv4_addr.s_addr = htonl(tcfg->tunnel.map.ipv4_prefix.ip_addr.ipv4_addr);
			ca_printf(KERN_INFO "    ipv4_prefix = %pI4 / %u\n",
					&ipv4_addr, tcfg->tunnel.map.ipv4_prefix.addr_len);
		} else {
			/* Print it anyway though it is wrong config. */
			ipv6_addr.in6_u.u6_addr32[0] = htonl(tcfg->tunnel.map.ipv4_prefix.ip_addr.ipv6_addr[0]); // MSB
			ipv6_addr.in6_u.u6_addr32[1] = htonl(tcfg->tunnel.map.ipv4_prefix.ip_addr.ipv6_addr[1]);
			ipv6_addr.in6_u.u6_addr32[2] = htonl(tcfg->tunnel.map.ipv4_prefix.ip_addr.ipv6_addr[2]);
			ipv6_addr.in6_u.u6_addr32[3] = htonl(tcfg->tunnel.map.ipv4_prefix.ip_addr.ipv6_addr[3]);
			ca_printf(KERN_INFO "  ipv4_prefix = %pI6 / %u (ERROR)\n", &ipv6_addr, tcfg->tunnel.map.ipv4_prefix.addr_len);
		}

		ca_printf(KERN_INFO "    ea_bit_length = %u\n", tcfg->tunnel.map.ea_bit_length);
		ca_printf(KERN_INFO "    psid_offset = %u\n", tcfg->tunnel.map.psid_offset);
		ca_printf(KERN_INFO "    psid_length = %u\n", tcfg->tunnel.map.psid_length);
		ca_printf(KERN_INFO "    psid_id = %u\n", tcfg->tunnel.map.psid_id);
		ca_printf(KERN_INFO "    validation_check_failed_dest_port = 0x%x\n", tcfg->tunnel.map.validation_check_failed_dest_port);
		ca_printf(KERN_INFO "    egress_tc_value = %u\n", tcfg->tunnel.map.egress_tc_value);
		ca_printf(KERN_INFO "    egress_hoplimit_value = %u\n", tcfg->tunnel.map.egress_hoplimit_value);
		ca_printf(KERN_INFO "    egress_flow_label_value = %u\n", tcfg->tunnel.map.egress_flow_label_value);

		// TODO: show the possible address range.
	}

	if (tcfg->type == CA_TUNNEL_MACSEC) {
		ca_macsec_sa_t macsec_sa;

		ca_printf(KERN_INFO "  MACSec info:\n");
		ca_printf(KERN_INFO "    tx_sa = %u\n", tcfg->tunnel.macsec.tx_sa);
		ca_printf(KERN_INFO "    rx_sa = %u\n", tcfg->tunnel.macsec.rx_sa);

		macsec_sa.sa_id = tcfg->tunnel.macsec.tx_sa;
		ret = sw_macsec_sa_get(0, &macsec_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Outbound MACSec SA info:\n");
			macsec_sa_dump(&macsec_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump outbound MACSec SA. (ret %u)\n", ret);
		}

		macsec_sa.sa_id = tcfg->tunnel.macsec.rx_sa;
		ret = sw_macsec_sa_get(0, &macsec_sa);
		if (ret == CA_E_OK) {
			ca_printf(KERN_INFO "  Inbound MACSec SA info:\n");
			macsec_sa_dump(&macsec_sa);
		} else {
			ca_printf(KERN_INFO "    Fail to dump inbound MACSec SA. (ret %u)\n", ret);
		}
	}
}

void tunnel_table_dump(void)
{
	int i;

	ca_printf(KERN_INFO "DUMP tunnel_table:\n");
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (g_tunnel[i] == NULL) continue;
		if (g_tunnel[i]->used == FALSE) continue;
		ca_printf(KERN_INFO "-----------------------------------------------------------------\n");
		ca_printf(KERN_INFO "tunnel_id %d, used %s\n", i, (g_tunnel[i]->used) ? "TRUE" : "FALSE");
		tunnel_cfg_dump(&g_tunnel[i]->tcfg);
		tunnel_private_data_dump(g_tunnel[i]);
	}
	ca_printf(KERN_INFO "=================================================================\n");
}

/***************************************************************************
 * Common SA ID functions
 ***************************************************************************/

extern ca_status_t ca_api_sa_id_alloc(ca_tunnel_type_t tunnel_type, ca_tunnel_direction_t dir, bool is_encrypted, uint32_t *sa_id);
extern ca_status_t ca_api_sa_id_free(uint32_t sa_id);

/***************************************************************************
 * PPPoE
 ***************************************************************************/

/* kernel_hook call flow:
 *  - ca_tunnel_add(&tunnel_id)
 *  - ca_l3_intf_add(afi=IPV4, type=TUNNEL, tunnel_id, pppoe_session_id)
 *  - ca_l3_nexthop_add(addr=PPPOE_REMOTE_IPV4)
 *  - ca_l3_route_add()
 *  - ca_l3_intf_add(afi=IPV6, type=TUNNEL, tunnel_id, pppoe_session_id)
 *  - ca_l3_nexthop_add(addr=PPPOE_REMOTE_IPV6)
 *  - ca_l3_route_add()
 *
 * 1. IPv4 intf and IPv6 intf share same tunnel_id.
 * 2. In ca_tunnel_cfg_t, src_addr / dest_addr / parent_l3_intf_id are useless.
 * 3. parent_l3_intf_id should be CA_UINT16_INVALID for PPPoE.
 */

static ca_status_t pppoe_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id )
{
	int i;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/* check if same ca_pppoe_tunnel_cfg_t exists */
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (   (g_tunnel[i] != NULL)
		    && (g_tunnel[i]->tcfg.type == CA_TUNNEL_PPPOE)
		    && (g_tunnel[i]->tcfg.tunnel.pppoe.pppoe_session_id == tunnel_cfg->tunnel.pppoe.pppoe_session_id) ) {
			*tunnel_id = i;
			return CA_E_EXISTS;
		}
	}

	return CA_E_OK;
}

static ca_status_t pppoe_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	int i;

	/* check if same ca_pppoe_tunnel_cfg_t exists */
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (   (g_tunnel[i] != NULL)
		    && (g_tunnel[i]->tcfg.type == CA_TUNNEL_PPPOE)
		    && (tunnel_id != i)
		    && (g_tunnel[i]->tcfg.tunnel.pppoe.pppoe_session_id == tunnel_cfg->tunnel.pppoe.pppoe_session_id) ) {
			return CA_E_EXISTS;
		}
	}

	return CA_E_OK;
}

/* kernel_hook call flow:
 *  - ca_l3_route_delete()
 *  - ca_l3_nexthop_delete()
 *  - ca_l3_intf_delete()
 *  - ca_tunnel_delete()
 */

static ca_status_t pppoe_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_OK;
}

/***************************************************************************
 * DS-Lite
 ***************************************************************************/

/* The API call flow for creating DS-Lite tunnel:
   1. ca_tunnel_add(type = CA_TUNNEL_DSLITE)
        search g_tunnel[] for empty tunnel and tunnel_id
        add entry into DS-Lite table
        store the entry info & index to g_tunnel[]
        return tunnel_id

   2. ca_l3_intf_add(tunnel_id)
        it also creates CLS rule for DS-Lite downstream
        return intf_id

   3. ca_l3_nexthop_add(intf_id)
        add NextHop entry for DS-Lite upstream
        return nexthop_id

   4. ca_l3_route_add(nexthop_id)
        add LPM entry for DS-Lite upstream
 */

static ca_status_t dslite_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_fwd_ctrl_t dsl_ipmc_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_new_mac_da_t dsl_ipmc_new_mac_da;

	l3pe_dsl_tbl_entry_t dsl_entry;
	fe_l3e_if_tbl_entry_t tmp_intf_entry;
	unsigned int dsl_idx; // desired index in DSLite Table
	bool found_empty_dsl_idx = false;
	int i, ret;
	dslite_private_data_t *priv;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/* validate tunnel_cfg */
	if ((tunnel_cfg->src_addr.afi != CA_IPV6) || (tunnel_cfg->dest_addr.afi != CA_IPV6))
		return CA_E_PARAM;

	/* search available if_id */
	for (i = CA_DSLITE_INTF_MIN; i <= CA_DSLITE_INTF_MAX; i++) {
		ret = aal_entry_get_by_idx(AAL_TABLE_FE_L3E_IF_TBL, i, &tmp_intf_entry);
		if (ret != AAL_E_OK) {
			dsl_idx = i - 48;
			found_empty_dsl_idx = true;
			break;
		}
	}
	if (found_empty_dsl_idx == false)
		return AAL_E_TBLFULL;

	/* validate multicast group address */
	if (tunnel_cfg->tunnel.dslite.mc_flag == CA_TRUE) {
		/* src_addr must be MC addr, dest_addr must be UC addr */
		if ((tunnel_cfg->src_addr.ip_addr.ipv6_addr[0] & 0xFF000000) != 0xFF000000)
			return CA_E_PARAM;
		if ((tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0] & 0xFF000000) == 0xFF000000)
			return CA_E_PARAM;
	} else {
		/* must be unicast addr */
		if ((tunnel_cfg->src_addr.ip_addr.ipv6_addr[0] & 0xFF000000) == 0xFF000000)
			return CA_E_PARAM;
		if ((tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0] & 0xFF000000) == 0xFF000000)
			return CA_E_PARAM;
	}

	/*** set reg L3FE_STG1_IPNIP_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_ctrl.bf.dsl_ipmc_new_mac_en = (tunnel_cfg->tunnel.dslite.rebuild_mc_mac) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_fwd_ctl = 1;
	ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = (tunnel_cfg->tunnel.dslite.dsl_ecn_check_enable == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_illegal_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_ext_det_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_to_l2fe == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_addr_prefix_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_addr_consistency_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_rpf_chk_en = (tunnel_cfg->tunnel.dslite.dsl_reverse_path_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ttl_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_ttl_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.dsl_dscp_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_dscp_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.dsl_ecn_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_ecn_keep_outer == CA_TRUE) ? 0 : 1;

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
	ecn_fail_fwd_ctrl.bf.cos = 0;
	ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
	ipnip_fail_fwd_ctrl.bf.cos = 0;
	ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_DSL_IPMC_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	dsl_ipmc_fwd_ctrl.bf.keep_orig_pkt = (tunnel_cfg->tunnel.dslite.mc_drop == CA_TRUE) ? 1 : 0;
	dsl_ipmc_fwd_ctrl.bf.ldpid = (tunnel_cfg->tunnel.dslite.mc_drop == CA_TRUE) ? AAL_LPORT_BLACKHOLE : AAL_LPORT_L3_LAN;
	dsl_ipmc_fwd_ctrl.bf.cos = 0;
	dsl_ipmc_fwd_ctrl.bf.t2_ctrl = 0x2;
	dsl_ipmc_fwd_ctrl.bf.t3_ctrl = 0xF;
	dsl_ipmc_fwd_ctrl.bf.t4_ctrl = 0xF;
	dsl_ipmc_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_set(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_DSL_IPMC_NEW_MAC_DA ***/

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_get(&dsl_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	dsl_ipmc_new_mac_da.bf.addr_prfx =
		  (tunnel_cfg->tunnel.dslite.mc_mac[0] << 17)
		| (tunnel_cfg->tunnel.dslite.mc_mac[1] << 9)
		| (tunnel_cfg->tunnel.dslite.mc_mac[2] << 1)
		| (tunnel_cfg->tunnel.dslite.mc_mac[3] >> 7);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_set(&dsl_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** add entry into DS-Lite Table ***/

	dsl_entry.valid = 1;
	dsl_entry.mc = (tunnel_cfg->tunnel.dslite.mc_flag == CA_TRUE) ? 1 : 0;
	dsl_entry.flwlabel = tunnel_cfg->tunnel.dslite.egress_flow_label_value;
	dsl_entry.hoplimit_en = (tunnel_cfg->tunnel.dslite.egress_hoplimit_value != 0) ? 1 : 0;
	dsl_entry.hoplimit = tunnel_cfg->tunnel.dslite.egress_hoplimit_value;
	dsl_entry.tc_en = 1;
	dsl_entry.tc = tunnel_cfg->tunnel.dslite.egress_tc_value;
	dsl_entry.aftr_ip0 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[3];
	dsl_entry.aftr_ip1 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[2];
	dsl_entry.aftr_ip2 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[1];
	dsl_entry.aftr_ip3 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0]; // MSB
	dsl_entry.b4_ip0 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[3];
	dsl_entry.b4_ip1 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[2];
	dsl_entry.b4_ip2 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[1];
	dsl_entry.b4_ip3 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[0]; // MSB

	ret = aal_entry_add_by_idx(AAL_TABLE_L3PE_DSL_TBL, &dsl_entry, dsl_idx);
	if (ret != AAL_E_OK) {
		switch (ret) {
		case AAL_E_TBLFULL:
			return CA_E_FULL;
		case AAL_E_ENTRYNOTFOUND:
			return CA_E_NOT_FOUND;
		case AAL_E_OPNOTSUPP:
			return CA_E_NOT_SUPPORT;
		case AAL_E_NULLPTR:
			return CA_E_PARAM;
		case AAL_E_ENTRY_USED:
			return CA_E_EXISTS;
		case AAL_E_OUTRANGE:
		case AAL_E_NOMEM:
			return CA_E_RESOURCE;
		default:
			return CA_E_ERROR;
		}
	}

	/* save private data */

	priv = ca_zalloc_trace(sizeof(dslite_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for DS-Lite private data.\n", __func__);
		return CA_E_MEMORY;
	}
	priv->aal_idx = dsl_idx;
	g_tunnel[*tunnel_id]->private_data = priv;

	return CA_E_OK;
}

static ca_status_t dslite_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_fwd_ctrl_t dsl_ipmc_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_new_mac_da_t dsl_ipmc_new_mac_da;

	l3pe_dsl_tbl_entry_t dsl_entry;
	int ret;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/* validate tunnel_cfg */
	if ((tunnel_cfg->src_addr.afi != CA_IPV6) || (tunnel_cfg->dest_addr.afi != CA_IPV6))
		return CA_E_PARAM;

	/*** set reg L3FE_STG1_IPNIP_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_ctrl.bf.dsl_ipmc_new_mac_en = (tunnel_cfg->tunnel.dslite.rebuild_mc_mac) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_fwd_ctl = 1;
	ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = (tunnel_cfg->tunnel.dslite.dsl_ecn_check_enable == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_illegal_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_ext_det_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_to_l2fe == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_addr_prefix_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en = (tunnel_cfg->tunnel.dslite.dsl_ipmc_addr_consistency_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_rpf_chk_en = (tunnel_cfg->tunnel.dslite.dsl_reverse_path_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.dsl_ttl_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_ttl_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.dsl_dscp_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_dscp_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.dsl_ecn_keep_outer = (tunnel_cfg->tunnel.dslite.dsl_ecn_keep_outer == CA_TRUE) ? 0 : 1;

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
	ecn_fail_fwd_ctrl.bf.cos = 0;
	ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
	ipnip_fail_fwd_ctrl.bf.cos = 0;
	ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_DSL_IPMC_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	dsl_ipmc_fwd_ctrl.bf.keep_orig_pkt = (tunnel_cfg->tunnel.dslite.mc_drop == CA_TRUE) ? 1 : 0;
	dsl_ipmc_fwd_ctrl.bf.ldpid = (tunnel_cfg->tunnel.dslite.mc_drop == CA_TRUE) ? AAL_LPORT_BLACKHOLE : AAL_LPORT_L3_LAN;
	dsl_ipmc_fwd_ctrl.bf.cos = 0;
	dsl_ipmc_fwd_ctrl.bf.t2_ctrl = 0x0;
	dsl_ipmc_fwd_ctrl.bf.t3_ctrl = 0xF;
	dsl_ipmc_fwd_ctrl.bf.t4_ctrl = 0xF;
	dsl_ipmc_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_set(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_DSL_IPMC_NEW_MAC_DA ***/

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_get(&dsl_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	dsl_ipmc_new_mac_da.bf.addr_prfx =
		  (tunnel_cfg->tunnel.dslite.mc_mac[0] << 17)
		| (tunnel_cfg->tunnel.dslite.mc_mac[1] << 9)
		| (tunnel_cfg->tunnel.dslite.mc_mac[2] << 1)
		| (tunnel_cfg->tunnel.dslite.mc_mac[3] >> 7);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_set(&dsl_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set entry into DS-Lite Table ***/

	dsl_entry.valid = 1;
	dsl_entry.mc = (tunnel_cfg->tunnel.dslite.mc_flag == CA_TRUE) ? 1 : 0;
	dsl_entry.flwlabel = tunnel_cfg->tunnel.dslite.egress_flow_label_value;
	dsl_entry.hoplimit_en = (tunnel_cfg->tunnel.dslite.egress_hoplimit_value != 0) ? 1 : 0;
	dsl_entry.hoplimit = tunnel_cfg->tunnel.dslite.egress_hoplimit_value;
	dsl_entry.tc_en = 1;
	dsl_entry.tc = tunnel_cfg->tunnel.dslite.egress_tc_value;
	dsl_entry.aftr_ip0 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[3];
	dsl_entry.aftr_ip1 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[2];
	dsl_entry.aftr_ip2 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[1];
	dsl_entry.aftr_ip3 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0]; // MSB
	dsl_entry.b4_ip0 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[3];
	dsl_entry.b4_ip1 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[2];
	dsl_entry.b4_ip2 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[1];
	dsl_entry.b4_ip3 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[0]; // MSB

	ret = aal_entry_set(AAL_TABLE_L3PE_DSL_TBL, ((dslite_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx, &dsl_entry);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	return CA_E_OK;
}

static ca_status_t dslite_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	if (aal_entry_del(AAL_TABLE_L3PE_DSL_TBL, ((dslite_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx) != AAL_E_OK)
		return CA_E_ERROR;

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

/***************************************************************************
 * 6RD
 ***************************************************************************/

/* The API call flow for creating 6RD tunnel:
   1. ca_tunnel_add(type = CA_TUNNEL_DSLITE)
        search g_tunnel[] for empty tunnel and tunnel_id
        add entry into 6RD table
        store the entry info & index to g_tunnel[]
        return tunnel_id

   2. ca_l3_intf_add(tunnel_id)
        it also creates CLS rule for 6RD downstream
        return intf_id

   3. ca_l3_nexthop_add(intf_id)
        add NextHop entry for 6RD upstream
        return nexthop_id

   4. ca_l3_route_add(nexthop_id)
        add LPM entry for 6RD upstream
 */

static ca_status_t sixrd_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_fwd_ctrl_t sixrd_ipmc_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_new_mac_da_t sixrd_ipmc_new_mac_da;

	l3pe_sixrd_tbl_entry_t sixrd_entry;
	fe_l3e_if_tbl_entry_t tmp_intf_entry;
	unsigned int sixrd_idx; // desired index in 6RD Table
	bool found_empty_sixrd_idx = false;
	int i, ret;
	sixrd_private_data_t *priv;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/* validate tunnel_cfg */
	if ((tunnel_cfg->src_addr.afi != CA_IPV4) || (tunnel_cfg->dest_addr.afi != CA_IPV4))
		return CA_E_PARAM;

	if (tunnel_cfg->tunnel.six_rd.egress_identification_start > tunnel_cfg->tunnel.six_rd.egress_identification_end)
		return CA_E_PARAM;

	/* search available if_id */
	for (i = CA_6RD_INTF_MIN; i <= CA_6RD_INTF_MAX; i++) {
		ret = aal_entry_get_by_idx(AAL_TABLE_FE_L3E_IF_TBL, i, &tmp_intf_entry);
		if (ret != AAL_E_OK) {
			sixrd_idx = i - 32;
			found_empty_sixrd_idx = true;
			break;
		}
	}
	if (found_empty_sixrd_idx == false)
		return AAL_E_TBLFULL;

	/*** set reg L3FE_STG1_IPNIP_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_ctrl.bf.sixrd_ipmc_new_mac_en = (tunnel_cfg->tunnel.six_rd.rebuild_mc_mac) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ipmc_fwd_ctl = 1;
	ipnip_ctrl.bf.sixrd_decap_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = (tunnel_cfg->tunnel.six_rd.six_rd_ecn_check_enable == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en = (tunnel_cfg->tunnel.six_rd.six_rd_ipmc_illegal_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ttl_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_ttl_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.sixrd_dscp_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_dscp_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.sixrd_ecn_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_ecn_keep_outer == CA_TRUE) ? 0 : 1;

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.six_rd.validation_check_failed_dest_port);
	ecn_fail_fwd_ctrl.bf.cos = 0;
	ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.six_rd.validation_check_failed_dest_port);
	ipnip_fail_fwd_ctrl.bf.cos = 0;
	ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_SIXRD_IPMC_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	sixrd_ipmc_fwd_ctrl.bf.keep_orig_pkt = (tunnel_cfg->tunnel.six_rd.mc_drop == CA_TRUE) ? 1 : 0;
	sixrd_ipmc_fwd_ctrl.bf.ldpid = (tunnel_cfg->tunnel.six_rd.mc_drop == CA_TRUE) ? AAL_LPORT_BLACKHOLE : AAL_LPORT_L3_LAN;
	sixrd_ipmc_fwd_ctrl.bf.cos = 0;
	sixrd_ipmc_fwd_ctrl.bf.t2_ctrl = 0x2;
	sixrd_ipmc_fwd_ctrl.bf.t3_ctrl = 0xF;
	sixrd_ipmc_fwd_ctrl.bf.t4_ctrl = 0xF;
	sixrd_ipmc_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_set(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_SIXRD_IPMC_NEW_MAC_DA ***/

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get(&sixrd_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	sixrd_ipmc_new_mac_da.bf.addr_prfx =
		(tunnel_cfg->tunnel.six_rd.mc_mac[0] << 8)
		| tunnel_cfg->tunnel.six_rd.mc_mac[1];

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_set(&sixrd_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** add entry into 6RD Table ***/

	sixrd_entry.valid = 1;
	sixrd_entry.ingr_chk_en = (tunnel_cfg->tunnel.six_rd.six_rd_ingress_check_en == CA_TRUE) ? 1 : 0;
	sixrd_entry.br_only = (tunnel_cfg->tunnel.six_rd.ce_ce_allowed == CA_TRUE) ? 0 : 1;
	sixrd_entry.ipsa_mtch = (tunnel_cfg->tunnel.six_rd.six_rd_ipsa_match == CA_TRUE) ? 1 : 0;
	sixrd_entry.id_end = tunnel_cfg->tunnel.six_rd.egress_identification_end;
	sixrd_entry.id_start = tunnel_cfg->tunnel.six_rd.egress_identification_start;
	sixrd_entry.ttl_en = (tunnel_cfg->tunnel.six_rd.egress_ttl_value == 0) ? 0 : 1;
	sixrd_entry.ttl = tunnel_cfg->tunnel.six_rd.egress_ttl_value;
	sixrd_entry.tos_en = 1;
	sixrd_entry.tos = tunnel_cfg->tunnel.six_rd.egress_tos_value;
	sixrd_entry.ce_v4_ip = tunnel_cfg->src_addr.ip_addr.ipv4_addr;
	sixrd_entry.br_v4_ip = tunnel_cfg->dest_addr.ip_addr.ipv4_addr;
	sixrd_entry.v6_prfx =
		((ca_uint64_t) tunnel_cfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[0] << 32)
		| tunnel_cfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[1];
	sixrd_entry.v6_prfx_len = tunnel_cfg->tunnel.six_rd.six_rd_v6_prefix_len;
	sixrd_entry.v4_msk_len = tunnel_cfg->tunnel.six_rd.six_rd_v4_mask_len;

	ret = aal_entry_add_by_idx(AAL_TABLE_L3PE_SIXRD_TBL, &sixrd_entry, sixrd_idx);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	/* save private data */

	priv = ca_zalloc_trace(sizeof(sixrd_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for 6RD private data.\n", __func__);
		return CA_E_MEMORY;
	}
	priv->aal_idx = sixrd_idx;
	g_tunnel[*tunnel_id]->private_data = priv;

	return CA_E_OK;
}

static ca_status_t sixrd_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_fwd_ctrl_t sixrd_ipmc_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_new_mac_da_t sixrd_ipmc_new_mac_da;

	l3pe_sixrd_tbl_entry_t sixrd_entry;
	int ret;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/* validate tunnel_cfg */
	if ((tunnel_cfg->src_addr.afi != CA_IPV4) || (tunnel_cfg->dest_addr.afi != CA_IPV4))
		return CA_E_PARAM;

	if (tunnel_cfg->tunnel.six_rd.egress_identification_start > tunnel_cfg->tunnel.six_rd.egress_identification_end)
		return CA_E_PARAM;

	/*** set reg L3FE_STG1_IPNIP_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_ctrl.bf.sixrd_ipmc_new_mac_en = (tunnel_cfg->tunnel.six_rd.rebuild_mc_mac) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ipmc_fwd_ctl = 1;
	ipnip_ctrl.bf.sixrd_decap_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
	ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = (tunnel_cfg->tunnel.six_rd.six_rd_ecn_check_enable == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en = (tunnel_cfg->tunnel.six_rd.six_rd_ipmc_illegal_check_en == CA_TRUE) ? 1 : 0;
	ipnip_ctrl.bf.sixrd_ttl_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_ttl_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.sixrd_dscp_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_dscp_keep_outer == CA_TRUE) ? 0 : 1;
	ipnip_ctrl.bf.sixrd_ecn_keep_outer = (tunnel_cfg->tunnel.six_rd.six_rd_ecn_keep_outer == CA_TRUE) ? 0 : 1;

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.six_rd.validation_check_failed_dest_port);
	ecn_fail_fwd_ctrl.bf.cos = 0;
	ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
	ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.six_rd.validation_check_failed_dest_port);
	ipnip_fail_fwd_ctrl.bf.cos = 0;
	ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
	ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_SIXRD_IPMC_FWD_CTRL ***/

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	sixrd_ipmc_fwd_ctrl.bf.keep_orig_pkt = (tunnel_cfg->tunnel.six_rd.mc_drop == CA_TRUE) ? 1 : 0;
	sixrd_ipmc_fwd_ctrl.bf.ldpid = (tunnel_cfg->tunnel.six_rd.mc_drop == CA_TRUE) ? AAL_LPORT_BLACKHOLE : AAL_LPORT_L3_LAN;
	sixrd_ipmc_fwd_ctrl.bf.cos = 0;
	sixrd_ipmc_fwd_ctrl.bf.t2_ctrl = 0x0;
	sixrd_ipmc_fwd_ctrl.bf.t3_ctrl = 0xF;
	sixrd_ipmc_fwd_ctrl.bf.t4_ctrl = 0xF;
	sixrd_ipmc_fwd_ctrl.bf.t5_ctrl = 0xF;

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_set(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set reg L3FE_STG1_SIXRD_IPMC_NEW_MAC_DA ***/

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get(&sixrd_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	sixrd_ipmc_new_mac_da.bf.addr_prfx =
		(tunnel_cfg->tunnel.six_rd.mc_mac[0] << 8)
		| tunnel_cfg->tunnel.six_rd.mc_mac[1];

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_set(&sixrd_ipmc_new_mac_da)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	/*** set entry into 6RD Table ***/

	sixrd_entry.valid = 1;
	sixrd_entry.ingr_chk_en = (tunnel_cfg->tunnel.six_rd.six_rd_ingress_check_en == CA_TRUE) ? 1 : 0;
	sixrd_entry.br_only = (tunnel_cfg->tunnel.six_rd.ce_ce_allowed == CA_TRUE) ? 0 : 1;
	sixrd_entry.ipsa_mtch = (tunnel_cfg->tunnel.six_rd.six_rd_ipsa_match == CA_TRUE) ? 1 : 0;
	sixrd_entry.id_end = tunnel_cfg->tunnel.six_rd.egress_identification_end;
	sixrd_entry.id_start = tunnel_cfg->tunnel.six_rd.egress_identification_start;
	sixrd_entry.ttl_en = (tunnel_cfg->tunnel.six_rd.egress_ttl_value == 0) ? 0 : 1;
	sixrd_entry.ttl = tunnel_cfg->tunnel.six_rd.egress_ttl_value;
	sixrd_entry.tos_en = 1;
	sixrd_entry.tos = tunnel_cfg->tunnel.six_rd.egress_tos_value;
	sixrd_entry.ce_v4_ip = tunnel_cfg->src_addr.ip_addr.ipv4_addr;
	sixrd_entry.br_v4_ip = tunnel_cfg->dest_addr.ip_addr.ipv4_addr;
	sixrd_entry.v6_prfx =
		((ca_uint64_t) tunnel_cfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[0] << 32)
		| tunnel_cfg->tunnel.six_rd.ipv6_6rd_prefix.ip_addr.ipv6_addr[1];
	sixrd_entry.v6_prfx_len = tunnel_cfg->tunnel.six_rd.six_rd_v6_prefix_len;
	sixrd_entry.v4_msk_len = tunnel_cfg->tunnel.six_rd.six_rd_v4_mask_len;

	ret = aal_entry_set(AAL_TABLE_L3PE_SIXRD_TBL, ((sixrd_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx, &sixrd_entry);
	if (ret != AAL_E_OK)
		return CA_E_ERROR;

	return CA_E_OK;
}

static ca_status_t sixrd_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	if (aal_entry_del(AAL_TABLE_L3PE_SIXRD_TBL, ((sixrd_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx) != AAL_E_OK)
		return CA_E_ERROR;

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

/***************************************************************************
 * MAP-E / MAP-T common
 ***************************************************************************/

/* For ca_tunnel_cfg_t:
 *   End-user IPv6 prefix length: src_addr.addr_len
 *   BR IP: dest_addr
 *
 * For ca_map_config_t:
 *   CE WAN-side IPv4 address: ipv4_prefix
 *   n bits: ipv6_prefix.addr_len
 *   r bits: ipv4_prefix.addr_len
 *   o bits: ea_bit_length
 *   p bits: ipv4 prefix length in EA bit fields
 *   a bits: psid_offset
 *   k bits: psid_length
 */

static void map_generate_ce_addr(ca_tunnel_cfg_t *tcfg, ca_ip_address_t *ce_v4_addr, ca_ip_address_t *ce_v6_addr)
{
	int offset, i;
	int v4_suffix_len_in_ea;
	int psid_len_in_ea;

	if ((ce_v4_addr == NULL) || (ce_v6_addr == NULL))
		return;

	if ((tcfg->tunnel.map.ipv4_prefix.addr_len + tcfg->tunnel.map.ea_bit_length) <= 32) {
		/* r bits + o bits <= 32, it means EA-bit contains Rule IPv4 suffix only. */
		v4_suffix_len_in_ea = tcfg->tunnel.map.ea_bit_length;
		psid_len_in_ea = 0;
	} else {
		/* r bits + o bits > 32, it means EA-bit contains Rule IPv4 suffix and PSID. */
		v4_suffix_len_in_ea = 32 - tcfg->tunnel.map.ipv4_prefix.addr_len;
		psid_len_in_ea = tcfg->tunnel.map.ea_bit_length - (32 - tcfg->tunnel.map.ipv4_prefix.addr_len); /* this should be same as tcfg->tunnel.map.psid_length */
	}

	/* CE IPv4 address
	 *
	 * 1. Copy Rule IPv4 prefix to ce_v4_addr
	 * 2. Copy IPv4 suffix from EA-bit to ce_v4_addr
	 */

	memcpy(ce_v4_addr, &tcfg->tunnel.map.ipv4_prefix, sizeof(ca_ip_address_t));

	offset = tcfg->tunnel.map.ipv4_prefix.addr_len;
	for (i = 0; i < v4_suffix_len_in_ea; i++) {
		if (bitcheck128(tcfg->src_addr.ip_addr.ipv6_addr, tcfg->tunnel.map.ipv6_prefix.addr_len + i)) {
			bitset32(&ce_v4_addr->ip_addr, offset + i);
		}
	}

	/* CE IPv6 address
	 *
	 * 1. Copy ce_v4_addr to Interface ID part of ce_v6_addr
	 * 2. Copy End-user IPv6 prefix part to ce_v6_addr
	 *    It may overwrite Interface part if prefix length is larger than 64
	 */

	memset(ce_v6_addr, 0, sizeof(*ce_v6_addr));
	ce_v6_addr->afi = CA_IPV6;
	ce_v6_addr->addr_len = tcfg->src_addr.addr_len;

	/*** fill Interface ID first because it may be overwritten by Rule IPv6 prefix and EA-bit ***/

	/* copy CE IPv4 address into Interface ID part of CE IPv6 address */
	switch (tcfg->tunnel.map.map_type) {
	case MAP_E_DRAFT_03:
		offset = 64 + 8;
		break;
	case MAP_E:
	case MAP_T:
	default:
		offset = 64 + 16;
		break;
	}
	for (i = 0; i < 32; i++) {
		if (bitcheck32(ce_v4_addr->ip_addr.ipv4_addr, i)) {
			bitset128(&ce_v6_addr->ip_addr, offset + i);
		}

	}

	/* copy PSID into Interface ID part of CE IPv6 address */
	switch (tcfg->tunnel.map.map_type) {
	case MAP_E_DRAFT_03:
		offset = 64 + 8 + 32;
		break;
	case MAP_E:
	case MAP_T:
	default:
		offset = 64 + 16 + 32;
		break;
	}
	for (i = 0; i < tcfg->tunnel.map.psid_length; i++) {
		if (bitcheck16(tcfg->tunnel.map.psid_id, 16 - tcfg->tunnel.map.psid_length + i)) {
			bitset128(&ce_v6_addr->ip_addr, offset + (16 - tcfg->tunnel.map.psid_length) + i);
		}
	}

	/* copy End-user IPv6 prefix (and only prefix) into CE IPv6 address */
	offset = 0;
	for (i = 0; i < tcfg->src_addr.addr_len; i++) {
		if (bitcheck128(&tcfg->src_addr.ip_addr, i)) {
			bitset128(&ce_v6_addr->ip_addr, offset + i);
		}
	}
}

static ca_status_t map_validate_tunnel_cfg(ca_tunnel_cfg_t *tunnel_cfg)
{
	if (tunnel_cfg == NULL) return CA_E_PARAM;

	if ((tunnel_cfg->tunnel.map.map_type < 0) && (tunnel_cfg->tunnel.map.map_type >= MAP_TUNNEL_TYPE_MAX)) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Unknown map_type %d.\n", __func__, tunnel_cfg->tunnel.map.map_type);
		}
		return CA_E_PARAM;
	}

	/* End-user IPv6 prefix */
	if ((tunnel_cfg->src_addr.afi != CA_IPV6) || (tunnel_cfg->src_addr.addr_len > 128)) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: src_addr is not IPv6 addr or prefix length exceeds 128.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* BR address */
	if (tunnel_cfg->dest_addr.afi != CA_IPV6) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: dest_addr is not IPv6 addr.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* Rule IPv6 prefix length (n bits) <= 128 */
	if ((tunnel_cfg->tunnel.map.ipv6_prefix.afi != CA_IPV6) || (tunnel_cfg->tunnel.map.ipv6_prefix.addr_len > 128)) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Rule IPv6 prefix is not IPv6 addr, or prefix length (n bits) exceeds 128.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* Rule IPv4 prefix length (r bits) <= 32 */
	if ((tunnel_cfg->tunnel.map.ipv4_prefix.afi != CA_IPV4) || (tunnel_cfg->tunnel.map.ipv4_prefix.addr_len > 32)) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Rule IPv4 prefix is not IPv4 addr, or prefix length (r bits) exceeds 32.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* EA-bit length (o bits) <= 48 */
	if (tunnel_cfg->tunnel.map.ea_bit_length > 48) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: EA-bit length (o bits) exceeds 48.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* PSID offset (a bits) + PSID length (k bits) <= 16; also imply each of them must not exceed 16. */
	if (tunnel_cfg->tunnel.map.psid_offset + tunnel_cfg->tunnel.map.psid_length > 16) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: PSID offset (a bits), PSID length (k bits), or sum of both exceed 16.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* Rule IPv4 suffix length (32 - r) + PSID length (k bits) >= EA-bit length (o bits) */
	if (32 - tunnel_cfg->tunnel.map.ipv4_prefix.addr_len + tunnel_cfg->tunnel.map.psid_length < tunnel_cfg->tunnel.map.ea_bit_length) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Rule IPv4 suffix length (32 - r bits) plus PSID length (k bits) is not enough for EA-bit length (o bits).\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* Rule IPv4 prefix length (r bits) and EA-bit length (o bits) can't be zero together. Why? Becomes DS-Lite? */
	if ((tunnel_cfg->tunnel.map.ipv4_prefix.addr_len == 0) && (tunnel_cfg->tunnel.map.ea_bit_length == 0)) {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Rule IPv4 prefix length (r bits) and EA-bit length (o bits) can't be zero together.\n", __func__);
		}
		return CA_E_PARAM;
	}

	return CA_E_OK;
}

/***************************************************************************
 * MAP-E
 ***************************************************************************/

static ca_status_t map_e_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;

	l3pe_dsl_tbl_entry_t dsl_entry;
	fe_l3e_if_tbl_entry_t tmp_intf_entry;
	unsigned int dsl_idx; // desired index in DSLite Table
	bool found_empty_dsl_idx = false; /* true: handle in NE. false: handle in PE. */
	int i, ret;
	map_private_data_t *priv;

#if 0
	aal_map_e_idx4pe_entry_t map_e_idx4pe_entry;
	ca_uint32_t rslt_idx4pe;
#endif
	ca_ip_address_t ce_ipv4;
	ca_ip_address_t ce_ipv6;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/*** validation ***/

	/* mask all IP address correctly; don't take non-zero in suffix. */
	ip_addr_mask(&tunnel_cfg->src_addr);
	ip_addr_mask(&tunnel_cfg->dest_addr);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv6_prefix);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv4_prefix);

	if ((ret = map_validate_tunnel_cfg(tunnel_cfg)) != CA_E_OK) {
		return ret;
	}

	/*** Generate WAN CE IPv6 address ***/

	map_generate_ce_addr(tunnel_cfg, &ce_ipv4, &ce_ipv6);
	tunnel_cfg->src_addr = ce_ipv6; /* Original value is End-user IPv6 prefix, new value is CE IPv6 address. */
	if (ca_api_debug & API_DBG_TUNNEL) {
		ipv4_addr.s_addr = htonl(ce_ipv4.ip_addr.ipv4_addr);
		ipv6_addr.in6_u.u6_addr32[0] = htonl(ce_ipv6.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(ce_ipv6.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(ce_ipv6.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(ce_ipv6.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "%s: Generated CE IPv4 address = %pI4 / %u\n", __func__, &ipv4_addr, ce_ipv4.addr_len);
		ca_printf(KERN_INFO "%s: Generated CE IPv6 address = %pI6 / %u\n", __func__, &ipv6_addr, ce_ipv6.addr_len);
	}

	/*** search available if_id ***/

	for (i = CA_DSLITE_INTF_MIN; i <= CA_DSLITE_INTF_MAX; i++) {
		ret = aal_entry_get_by_idx(AAL_TABLE_FE_L3E_IF_TBL, i, &tmp_intf_entry);
		if (ret != AAL_E_OK) {
			dsl_idx = i - 48;
			found_empty_dsl_idx = true;
			break;
		}
	}

	/* prepare private data */

	priv = ca_zalloc_trace(sizeof(map_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for MAP-E private data.\n", __func__);
		return CA_E_MEMORY;
	}
	g_tunnel[*tunnel_id]->private_data = priv;
	priv->ce_ipv4 = ce_ipv4;

	if (found_empty_dsl_idx == true) {
		/* handle MAP-E in NE */
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: MAP-E is handled in NE.\n", __func__);
		}

		/*** set reg L3FE_STG1_IPNIP_CTRL ***/

		if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		ipnip_ctrl.bf.dsl_ipmc_new_mac_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_fwd_ctl = 1;
		ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl = 1;
		ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
		ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = 0;
		ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_ext_det_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en = 0;
		ipnip_ctrl.bf.dsl_rpf_chk_en = 0;
		ipnip_ctrl.bf.dsl_ttl_keep_outer = 1;
		ipnip_ctrl.bf.dsl_dscp_keep_outer = 1;
		ipnip_ctrl.bf.dsl_ecn_keep_outer = 0;

		if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

		if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
		ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
		ecn_fail_fwd_ctrl.bf.cos = 0;
		ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

		if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

		if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
		ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
		ipnip_fail_fwd_ctrl.bf.cos = 0;
		ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

		if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
			ca_free(priv);
			return CA_E_ERROR;
		}

		/*** add entry into DS-Lite Table ***/

		dsl_entry.valid = 1;
		dsl_entry.mc = 0;
		dsl_entry.flwlabel = tunnel_cfg->tunnel.map.egress_flow_label_value;
		dsl_entry.hoplimit_en = (tunnel_cfg->tunnel.map.egress_hoplimit_value != 0) ? 1 : 0;
		dsl_entry.hoplimit = tunnel_cfg->tunnel.map.egress_hoplimit_value;
		dsl_entry.tc_en = 1;
		dsl_entry.tc = tunnel_cfg->tunnel.map.egress_tc_value;
		dsl_entry.aftr_ip0 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[3];
		dsl_entry.aftr_ip1 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[2];
		dsl_entry.aftr_ip2 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[1];
		dsl_entry.aftr_ip3 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0]; // MSB
		dsl_entry.b4_ip0 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[3];
		dsl_entry.b4_ip1 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[2];
		dsl_entry.b4_ip2 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[1];
		dsl_entry.b4_ip3 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[0]; // MSB

		ret = aal_entry_add_by_idx(AAL_TABLE_L3PE_DSL_TBL, &dsl_entry, dsl_idx);
		if (ret != AAL_E_OK) {
			ca_free(priv);
			switch (ret) {
			case AAL_E_TBLFULL:
				return CA_E_FULL;
			case AAL_E_ENTRYNOTFOUND:
				return CA_E_NOT_FOUND;
			case AAL_E_OPNOTSUPP:
				return CA_E_NOT_SUPPORT;
			case AAL_E_NULLPTR:
				return CA_E_PARAM;
			case AAL_E_ENTRY_USED:
				return CA_E_EXISTS;
			case AAL_E_OUTRANGE:
			case AAL_E_NOMEM:
				return CA_E_RESOURCE;
			default:
				return CA_E_ERROR;
			}
		}

		priv->offload_in_ne = 1;
		priv->aal_idx = dsl_idx;

	} else {
#if 0
		/* DS-Lite table is full, so handle MAP-E in PE */

		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: MAP-E is handled in PE.\n", __func__);
		}

		/* get a MAP-E index for PE */

		map_e_idx4pe_entry.valid = 1;
		ret = aal_entry_add(AAL_TABLE_MAP_E_IDX4PE, &map_e_idx4pe_entry, &rslt_idx4pe);
		if (ret != AAL_E_OK) {
			ca_free(priv);
			return CA_E_INTERNAL;
		}
		priv->idx4pe = rslt_idx4pe;

		/* set info to PE */

		ret = ca_map_e_ipc_entry_set(device_id, CA_TUNNEL_DIR_OUTBOUND, *tunnel_id, rslt_idx4pe);
		if (ret != CA_E_OK) {
			ca_printf("%s: Fail to set MAP-E info to PE for encapsulation. (ret=%d)\n", __func__, ret);
			ca_free(priv);
			return CA_E_INTERNAL;
		}

		ret = ca_map_e_ipc_entry_set(device_id, CA_TUNNEL_DIR_INBOUND, *tunnel_id, rslt_idx4pe);
		if (ret != CA_E_OK) {
			ca_printf("%s: Fail to set MAP-E info to PE for decapsulation. (ret=%d)\n", __func__, ret);
			ca_free(priv);
			return CA_E_INTERNAL;
		}

		priv->offload_in_ne = 0;
#else
		/* Without doing anything, MAP-E packets can be forwarded to CPU port already
		 * by Hash Engine if its tuple contains any hash mask.
		 */
		return CA_E_FULL;
#endif
	}

	return CA_E_OK;
}

static ca_status_t map_e_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;

	l3pe_dsl_tbl_entry_t dsl_entry;
	int ret;

	ca_ip_address_t ce_ipv4;
	ca_ip_address_t ce_ipv6;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/*** validation ***/

	/* mask all IP address correctly; don't take non-zero in suffix. */
	ip_addr_mask(&tunnel_cfg->src_addr);
	ip_addr_mask(&tunnel_cfg->dest_addr);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv6_prefix);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv4_prefix);

	if ((ret = map_validate_tunnel_cfg(tunnel_cfg)) != CA_E_OK) {
		return ret;
	}

	/*** Generate WAN CE IPv6 address ***/

	map_generate_ce_addr(tunnel_cfg, &ce_ipv4, &ce_ipv6);
	tunnel_cfg->src_addr = ce_ipv6; /* Original value is End-user IPv6 prefix, new value is CE IPv6 address. */
	if (ca_api_debug & API_DBG_TUNNEL) {
		ipv4_addr.s_addr = htonl(ce_ipv4.ip_addr.ipv4_addr);
		ipv6_addr.in6_u.u6_addr32[0] = htonl(ce_ipv6.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(ce_ipv6.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(ce_ipv6.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(ce_ipv6.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "%s: Generated CE IPv4 address = %pI4 / %u\n", __func__, &ipv4_addr, ce_ipv4.addr_len);
		ca_printf(KERN_INFO "%s: Generated CE IPv6 address = %pI6 / %u\n", __func__, &ipv6_addr, ce_ipv6.addr_len);
	}
	((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->ce_ipv4 = ce_ipv4;

	if (((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->offload_in_ne != 0) {
		/* handle MAP-E in NE */
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: MAP-E is handled in NE.\n", __func__);
		}

		/*** set reg L3FE_STG1_IPNIP_CTRL ***/

		if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		ipnip_ctrl.bf.dsl_ipmc_new_mac_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_fwd_ctl = 1;
		ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl = 1;
		ipnip_ctrl.bf.intf_chk_fail_fwd_ctl = 1;
		ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl = 0;
		ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_ext_det_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en = 0;
		ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en = 0;
		ipnip_ctrl.bf.dsl_rpf_chk_en = 0;
		ipnip_ctrl.bf.dsl_ttl_keep_outer = 1;
		ipnip_ctrl.bf.dsl_dscp_keep_outer = 1;
		ipnip_ctrl.bf.dsl_ecn_keep_outer = 0;

		if ((ret = aal_l3fe_stg1_ipnip_ctrl_set(&ipnip_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		/*** set reg L3FE_STG1_ECN_FAIL_FWD_CTRL ***/

		if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		ecn_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
		ecn_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
		ecn_fail_fwd_ctrl.bf.cos = 0;
		ecn_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
		ecn_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

		if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_set(&ecn_fail_fwd_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		/*** set reg L3FE_STG1_IPNIP_FAIL_FWD_CTRL ***/

		if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		ipnip_fail_fwd_ctrl.bf.keep_orig_pkt = 1;
		ipnip_fail_fwd_ctrl.bf.ldpid = PORT_ID(tunnel_cfg->tunnel.dslite.validation_check_failed_dest_port);
		ipnip_fail_fwd_ctrl.bf.cos = 0;
		ipnip_fail_fwd_ctrl.bf.t2_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t3_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t4_ctrl = 0xF;
		ipnip_fail_fwd_ctrl.bf.t5_ctrl = 0xF;

		if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_set(&ipnip_fail_fwd_ctrl)) != AAL_E_OK) {
			return CA_E_ERROR;
		}

		/*** set entry into DS-Lite Table ***/

		dsl_entry.valid = 1;
		dsl_entry.mc = 0;
		dsl_entry.flwlabel = tunnel_cfg->tunnel.map.egress_flow_label_value;
		dsl_entry.hoplimit_en = (tunnel_cfg->tunnel.map.egress_hoplimit_value != 0) ? 1 : 0;
		dsl_entry.hoplimit = tunnel_cfg->tunnel.map.egress_hoplimit_value;
		dsl_entry.tc_en = 1;
		dsl_entry.tc = tunnel_cfg->tunnel.map.egress_tc_value;
		dsl_entry.aftr_ip0 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[3];
		dsl_entry.aftr_ip1 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[2];
		dsl_entry.aftr_ip2 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[1];
		dsl_entry.aftr_ip3 = tunnel_cfg->dest_addr.ip_addr.ipv6_addr[0]; // MSB
		dsl_entry.b4_ip0 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[3];
		dsl_entry.b4_ip1 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[2];
		dsl_entry.b4_ip2 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[1];
		dsl_entry.b4_ip3 = tunnel_cfg->src_addr.ip_addr.ipv6_addr[0]; // MSB

		ret = aal_entry_set(AAL_TABLE_L3PE_DSL_TBL, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx, &dsl_entry);
		if (ret != AAL_E_OK)
			return CA_E_ERROR;

	} else {
#if 1
		/* handle MAP-E in NE */
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: MAP-E is handled in PE.\n", __func__);
		}

		/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
		ret = ca_map_e_ipc_entry_set(device_id, CA_TUNNEL_DIR_OUTBOUND, tunnel_id, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
		if (ret != CA_E_OK) {
			ca_printf("%s: Fail to set MAP-E info to PE for encapsulation. (ret=%d)\n", __func__, ret);
			return CA_E_INTERNAL;
		}

		ret = ca_map_e_ipc_entry_set(device_id, CA_TUNNEL_DIR_INBOUND, tunnel_id, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
		if (ret != CA_E_OK) {
			ca_printf("%s: Fail to set MAP-E info to PE for decapsulation. (ret=%d)\n", __func__, ret);
			return CA_E_INTERNAL;
		}
#endif
		ret = aal_entry_del(AAL_TABLE_MAP_E_IDX4PE, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
		if (ret != AAL_E_OK) {
			printk(KERN_INFO "%s: ERROR! Fail to delete MAP-E IDX4PE entry. (index %u, ret %d)\n",
					__func__, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe, ret);
			return CA_E_INTERNAL;
		}
		printk(KERN_INFO "%s: MAP-E IDX4PE entry of index %u is deleted.\n", __func__, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
#else
		/* Not support to change offloading place */
		return CA_E_CONFLICT;
#endif
	}

	return CA_E_OK;
}

static ca_status_t map_e_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	if (((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->offload_in_ne != 0) {

		if (aal_entry_del(AAL_TABLE_L3PE_DSL_TBL, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->aal_idx) != AAL_E_OK)
			return CA_E_ERROR;

	} else {
#if 1
		/* delete info from PE */

		ca_status_t ret;
#ifdef CONFIG_CORTINA_IPC2RCPU
		ret = ca_map_e_ipc_entry_del(device_id, CA_TUNNEL_DIR_OUTBOUND, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
		if (ret != CA_E_OK) {
			return CA_E_INTERNAL;
		}

		ret = ca_map_e_ipc_entry_del(device_id, CA_TUNNEL_DIR_INBOUND, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
		if (ret != CA_E_OK) {
			return CA_E_INTERNAL;
		}
#endif
#else
		/* impossible code flow */
		return CA_E_NOT_FOUND;
#endif
	}

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

/***************************************************************************
 * MAP-T
 ***************************************************************************/

static ca_status_t map_t_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	int ret;
	map_private_data_t *priv;
	aal_map_t_idx4pe_entry_t map_t_idx4pe_entry;
	ca_uint32_t rslt_idx4pe;

	ca_ip_address_t ce_ipv4;
	ca_ip_address_t ce_ipv6;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/*** validation ***/

	/* mask all IP address correctly; don't take non-zero in suffix. */
	ip_addr_mask(&tunnel_cfg->src_addr);
	ip_addr_mask(&tunnel_cfg->dest_addr);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv6_prefix);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv4_prefix);

	if ((ret = map_validate_tunnel_cfg(tunnel_cfg)) != CA_E_OK) {
		return ret;
	}

	/* MAP-T BR IPv6 prefix length must follow RFC6052 */
	switch (tunnel_cfg->dest_addr.addr_len) {
	case 32: case 40: case 48: case 56: case 64: case 96:
		break;
	default:
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_ERR "%s: Invalid BR IPv6 prefix length %u.\n", __func__, tunnel_cfg->dest_addr.addr_len);
		}
		return CA_E_PARAM;
	}

	/*** Generate WAN CE IPv6 address ***/

	map_generate_ce_addr(tunnel_cfg, &ce_ipv4, &ce_ipv6);
	tunnel_cfg->src_addr = ce_ipv6; /* Original value is End-user IPv6 prefix, new value is CE IPv6 address. */
	if (ca_api_debug & API_DBG_TUNNEL) {
		ipv4_addr.s_addr = htonl(ce_ipv4.ip_addr.ipv4_addr);
		ipv6_addr.in6_u.u6_addr32[0] = htonl(ce_ipv6.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(ce_ipv6.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(ce_ipv6.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(ce_ipv6.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "%s: Generated CE IPv4 address = %pI4 / %u\n", __func__, &ipv4_addr, ce_ipv4.addr_len);
		ca_printf(KERN_INFO "%s: Generated CE IPv6 address = %pI6 / %u\n", __func__, &ipv6_addr, ce_ipv6.addr_len);
	}

	/* prepare private data */

	priv = ca_zalloc_trace(sizeof(map_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for MAP-T private data.\n", __func__);
		return CA_E_MEMORY;
	}
	g_tunnel[*tunnel_id]->private_data = priv;
	priv->ce_ipv4 = ce_ipv4;

	/* get a MAP-T index for PE */

	map_t_idx4pe_entry.valid = 1;
	ret = aal_entry_add(AAL_TABLE_MAP_T_IDX4PE, &map_t_idx4pe_entry, &rslt_idx4pe);
	if (ret != AAL_E_OK) {
		ca_free(priv);
		return CA_E_INTERNAL;
	}
	priv->idx4pe = rslt_idx4pe;

	/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_map_t_ipc_entry_set(device_id, CA_TUNNEL_DIR_OUTBOUND, tunnel_cfg, priv, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MAP-T info to PE for encapsulation. (ret=%d)\n", __func__, ret);
		ca_free(priv);
		return CA_E_INTERNAL;
	}

	ret = ca_map_t_ipc_entry_set(device_id, CA_TUNNEL_DIR_INBOUND, tunnel_cfg, priv, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MAP-T info to PE for decapsulation. (ret=%d)\n", __func__, ret);
		ca_free(priv);
		return CA_E_INTERNAL;
	}
#endif
	return CA_E_OK;
}

static ca_status_t map_t_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	int ret;
	ca_ip_address_t ce_ipv4;
	ca_ip_address_t ce_ipv6;
	struct in_addr ipv4_addr;
	struct in6_addr ipv6_addr;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	/*** validation ***/

	/* mask all IP address correctly; don't take non-zero in suffix. */
	ip_addr_mask(&tunnel_cfg->src_addr);
	ip_addr_mask(&tunnel_cfg->dest_addr);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv6_prefix);
	ip_addr_mask(&tunnel_cfg->tunnel.map.ipv4_prefix);

	if ((ret = map_validate_tunnel_cfg(tunnel_cfg)) != CA_E_OK) {
		return ret;
	}

	/*** Generate WAN CE IPv6 address ***/

	map_generate_ce_addr(tunnel_cfg, &ce_ipv4, &ce_ipv6);
	tunnel_cfg->src_addr = ce_ipv6; /* Original value is End-user IPv6 prefix, new value is CE IPv6 address. */
	if (ca_api_debug & API_DBG_TUNNEL) {
		ipv4_addr.s_addr = htonl(ce_ipv4.ip_addr.ipv4_addr);
		ipv6_addr.in6_u.u6_addr32[0] = htonl(ce_ipv6.ip_addr.ipv6_addr[0]); // MSB
		ipv6_addr.in6_u.u6_addr32[1] = htonl(ce_ipv6.ip_addr.ipv6_addr[1]);
		ipv6_addr.in6_u.u6_addr32[2] = htonl(ce_ipv6.ip_addr.ipv6_addr[2]);
		ipv6_addr.in6_u.u6_addr32[3] = htonl(ce_ipv6.ip_addr.ipv6_addr[3]);
		ca_printf(KERN_INFO "%s: Generated CE IPv4 address = %pI4 / %u\n", __func__, &ipv4_addr, ce_ipv4.addr_len);
		ca_printf(KERN_INFO "%s: Generated CE IPv6 address = %pI6 / %u\n", __func__, &ipv6_addr, ce_ipv6.addr_len);
	}
	((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->ce_ipv4 = ce_ipv4;

	/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_map_t_ipc_entry_set(device_id, CA_TUNNEL_DIR_OUTBOUND, tunnel_cfg, g_tunnel[tunnel_id]->private_data, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MAP-T info to PE for encapsulation. (ret=%d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	ret = ca_map_t_ipc_entry_set(device_id, CA_TUNNEL_DIR_INBOUND, tunnel_cfg, g_tunnel[tunnel_id]->private_data, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MAP-T info to PE for decapsulation. (ret=%d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}
#endif
	ret = aal_entry_del(AAL_TABLE_MAP_T_IDX4PE, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete MAP-T IDX4PE entry. (index %u, ret %d)\n",
				__func__, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe, ret);
		return CA_E_INTERNAL;
	}
	printk(KERN_INFO "%s: MAP-T IDX4PE entry of index %u is deleted.\n", __func__, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);

	return CA_E_OK;
}

static ca_status_t map_t_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	ca_status_t ret;
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_map_t_ipc_entry_del(device_id, CA_TUNNEL_DIR_OUTBOUND, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}

	ret = ca_map_t_ipc_entry_del(device_id, CA_TUNNEL_DIR_INBOUND, ((map_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}
#endif
	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

/***************************************************************************
 * IPSec
 ***************************************************************************/

struct ipsec_sa_node {
	ca_ipsec_sa_t		ipsec_sa;
	ca_ipsec_sa_id_t	sa_id;

	struct list_head	list;
};

static LIST_HEAD(ipsec_sa_list);
static DEFINE_RWLOCK(ipsec_sa_list_lock);

static int ipsec_sa_node_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct ipsec_sa_node *na = list_entry(a, struct ipsec_sa_node, list);
	struct ipsec_sa_node *nb = list_entry(b, struct ipsec_sa_node, list);

	if (na->sa_id > nb->sa_id) return 1;
	if (na->sa_id < nb->sa_id) return -1;
	return 0;
}

static void ipsec_sa_dump(ca_ipsec_sa_t *ipsec_sa)
{
	int i;

	if (ipsec_sa == NULL) return;

	ca_printf(KERN_INFO "    replay_window = %u\n", ipsec_sa->replay_window);
	ca_printf(KERN_INFO "    spi = 0x%08x (network order)\n", ipsec_sa->spi);
	ca_printf(KERN_INFO "    sequence_number = %u\n", ipsec_sa->sequence_number);

	ca_printf(KERN_INFO "    encryption_keylen = %u (%u bytes)\n", ipsec_sa->encryption_keylen, ipsec_sa->encryption_keylen * 4);
	ca_printf(KERN_INFO "    ekey = ");
	for (i = 0; i < ipsec_sa->encryption_keylen * 4; i++) {
		ca_printf("%02x ", ipsec_sa->ekey[i]);
	}
	ca_printf("\n");

	ca_printf(KERN_INFO "    auth_keylen = %u (%u bytes)\n", ipsec_sa->auth_keylen, ipsec_sa->auth_keylen * 4);
	ca_printf(KERN_INFO "    akey = ");
	for (i = 0; i < ipsec_sa->auth_keylen * 4; i++) {
		ca_printf("%02x ", ipsec_sa->akey[i]);
	}
	ca_printf("\n");

	ca_printf(KERN_INFO "    ip_ver = %u [%s]\n",
		ipsec_sa->ip_version, ((ipsec_sa->ip_version == 0) ? "IPv4" : "IPv6"));

	ca_printf(KERN_INFO "    proto = %u [%s]\n",
		ipsec_sa->protocol, ((ipsec_sa->protocol == 0) ? "ESP" : "AH"));

	ca_printf(KERN_INFO "    tunnel = %u [%s]\n",
		ipsec_sa->tunnel, ((ipsec_sa->tunnel == 0) ? "Transport" : "Tunnel"));

	ca_printf(KERN_INFO "    sa_dir = %d ", ipsec_sa->sa_dir);
	switch (ipsec_sa->sa_dir) {
	case CA_TUNNEL_DIR_OUTBOUND:
		ca_printf("[Outbound, Encryption]\n"); break;
	case CA_TUNNEL_DIR_INBOUND:
		ca_printf("[Inbound, Decryption]\n"); break;
	default:
		ca_printf("[UNKNOWN]\n"); break;
	}

	/* ca_ipsec_ciph_alg_t */
	ca_printf(KERN_INFO "    ealg = %u ", ipsec_sa->ealg);
	switch (ipsec_sa->ealg) {
	case CA_IPSEC_CIPHER_NULL:
		ca_printf("[Null Cipher Algortithm]\n"); break;
	case CA_IPSEC_CIPHER_DES:
		ca_printf("[DES]\n"); break;
	case CA_IPSEC_CIPHER_3DES:
		ca_printf("[3DES]\n"); break;
	case CA_IPSEC_CIPHER_AES:
		ca_printf("[AES]\n"); break;
	default:
		ca_printf("[UNKNOWN]\n"); break;
	}

	/* ca_ipsec_ciph_mode_t */
	ca_printf(KERN_INFO "    ealg_mode = %u ", ipsec_sa->ealg_mode);
	switch (ipsec_sa->ealg_mode) {
	case CA_IPSEC_CIPHER_MODE_ECB:
		ca_printf("[ECB]\n"); break;
	case CA_IPSEC_CIPHER_MODE_CBC:
		ca_printf("[CBC]\n"); break;
	case CA_IPSEC_CIPHER_MODE_CTR:
		ca_printf("[CTR]\n"); break;
	case CA_IPSEC_CIPHER_MODE_CCM:
		ca_printf("[CCM]\n"); break;
	case CA_IPSEC_CIPHER_MODE_GCM:
		ca_printf("[GCM]\n"); break;
	case CA_IPSEC_CIPHER_MODE_OFB:
		ca_printf("[OFB]\n"); break;
	case CA_IPSEC_CIPHER_MODE_CFB:
		ca_printf("[CFB]\n"); break;
	default:
		ca_printf("[UNKNOWN]\n"); break;
	}

	/* ca_ipsec_hash_alg_t */
	ca_printf(KERN_INFO "    aalg = %u ", ipsec_sa->aalg);
	switch (ipsec_sa->aalg) {
	case CA_IPSEC_HASH_NULL:
		ca_printf("[Null Hash Algortithm]\n"); break;
	case CA_IPSEC_HASH_MD5:
		ca_printf("[MD5]\n"); break;
	case CA_IPSEC_HASH_SHA1:
		ca_printf("[SHA1]\n"); break;
	case CA_IPSEC_HASH_SHA256:
		ca_printf("[SHA256]\n"); break;
	case CA_IPSEC_HASH_SHA384:
		ca_printf("[SHA384]\n"); break;
	case CA_IPSEC_HASH_SHA512:
		ca_printf("[SHA512]\n"); break;
	default:
		ca_printf("[UNKNOWN]\n"); break;
	}

	ca_printf(KERN_INFO "    iv_len = %u (%u bytes)\n", ipsec_sa->iv_len, ipsec_sa->iv_len * 4);
	ca_printf(KERN_INFO "    icv_trunclen = %u (%u bytes)\n", ipsec_sa->icv_trunclen, ipsec_sa->icv_trunclen * 4);
	ca_printf(KERN_INFO "    etherIP = %u\n", ipsec_sa->etherIP);
	ca_printf(KERN_INFO "    is_natt = %u\n", ipsec_sa->is_natt);
	ca_printf(KERN_INFO "    src_l4_port = %u\n", ipsec_sa->src_l4_port);
	ca_printf(KERN_INFO "    dest_l4_port = %u\n", ipsec_sa->dest_l4_port);
}

static ca_status_t ipsec_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	ca_ipsec_sa_t ipsec_sa_encrypt, ipsec_sa_decrypt;
	ca_status_t ret;
	int i;
	aal_ipsec_idx4pe_entry_t ipsec_idx4pe_entry;
	unsigned int rslt_idx4pe;
	ipsec_private_data_t *priv;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	if (ca_ipsec_sa_get(0, tunnel_cfg->tunnel.ipsec.sa_id_encrypt, &ipsec_sa_encrypt) != CA_E_OK)
		return CA_E_PARAM;

	if (ca_ipsec_sa_get(0, tunnel_cfg->tunnel.ipsec.sa_id_decrypt, &ipsec_sa_decrypt) != CA_E_OK)
		return CA_E_PARAM;

	if (ipsec_sa_encrypt.sa_dir != CA_IPSEC_SPD_ENCRYPT)
		return CA_E_PARAM;

	if (ipsec_sa_decrypt.sa_dir != CA_IPSEC_SPD_DECRYPT)
		return CA_E_PARAM;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (   (g_tunnel[i] != NULL)
		    && (g_tunnel[i]->tcfg.type == CA_TUNNEL_IPSEC)
		    && (memcmp(&g_tunnel[i]->tcfg.tunnel.ipsec, &tunnel_cfg->tunnel.ipsec, sizeof(ca_ipsec_tunnel_cfg_t)) == 0) ) {
			/* Don't check tunnel src_addr & dest_addr. It may be same tunnel addr but different SPI. */
			*tunnel_id = i;
			return CA_E_EXISTS;
		}
	}

	/* get an IPSec index for PE */

	ipsec_idx4pe_entry.valid = 1;
	ipsec_idx4pe_entry.sa_id_encrypt = tunnel_cfg->tunnel.ipsec.sa_id_encrypt;
	ipsec_idx4pe_entry.sa_id_decrypt = tunnel_cfg->tunnel.ipsec.sa_id_decrypt;
	ret = aal_entry_add(AAL_TABLE_IPSEC_IDX4PE, &ipsec_idx4pe_entry, &rslt_idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to add IPSec IDX4PE entry. (ret %d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_ipsec_ipc_sadb_set(device_id, CA_IPSEC_SPD_ENCRYPT, tunnel_cfg, &ipsec_sa_encrypt, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB for encryption. (ret=%d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	ret = ca_ipsec_ipc_sadb_set(device_id, CA_IPSEC_SPD_DECRYPT, tunnel_cfg, &ipsec_sa_decrypt, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB for decryption. (ret=%d)\n", __func__, ret);
		ca_ipsec_ipc_sadb_del(device_id, CA_IPSEC_SPD_ENCRYPT, rslt_idx4pe);
		return CA_E_INTERNAL;
	}

	ret = ca_ipsec_ipc_sadb_key_set(device_id, CA_IPSEC_SPD_ENCRYPT, tunnel_cfg, &ipsec_sa_encrypt, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB key for encryption. (ret=%d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	ret = ca_ipsec_ipc_sadb_key_set(device_id, CA_IPSEC_SPD_DECRYPT, tunnel_cfg, &ipsec_sa_decrypt, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB key for decryption. (ret=%d)\n", __func__, ret);
		ca_ipsec_ipc_sadb_del(device_id, CA_IPSEC_SPD_ENCRYPT, tunnel_cfg->tunnel.ipsec.sa_id_encrypt);
		return CA_E_INTERNAL;
	}
#endif
	/* save private data */

	priv = ca_zalloc_trace(sizeof(ipsec_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for IPSec private data.\n", __func__);
		return CA_E_MEMORY;
	}
	priv->idx4pe = rslt_idx4pe;
	g_tunnel[*tunnel_id]->private_data = priv;

	return CA_E_OK;
}

static ca_status_t ipsec_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_NOT_SUPPORT;
}

static ca_status_t ipsec_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_status_t ret;

	if (g_tunnel[tunnel_id] == NULL)
		return CA_E_PARAM;

	if (g_tunnel[tunnel_id]->tcfg.type != CA_TUNNEL_IPSEC)
		return CA_E_PARAM;

	tcfg = &g_tunnel[tunnel_id]->tcfg;

	/* delete info from PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_ipsec_ipc_sadb_del(device_id, CA_IPSEC_SPD_ENCRYPT, ((ipsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ca_ipsec_ipc_sadb_del returns fail for IPSec encryption. (ret %d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	ret = ca_ipsec_ipc_sadb_del(device_id, CA_IPSEC_SPD_DECRYPT, ((ipsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ca_ipsec_ipc_sadb_del returns fail for IPSec decryption. (ret %d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}
#endif
	ret = aal_entry_del(AAL_TABLE_IPSEC_IDX4PE, ((ipsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete IPSec IDX4PE entry. (index %u, ret %d)\n",
				__func__, ((ipsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe, ret);
		return CA_E_INTERNAL;
	}

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

ca_status_t ca_ipsec_init (
	CA_IN		ca_device_id_t		device_id )
{
	return CA_E_OK;
}

ca_status_t ca_ipsec_shut (
	CA_IN		ca_device_id_t		device_id )
{
	return CA_E_OK;
}

ca_status_t ca_ipsec_sa_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_t		*ipsec_sa,
	CA_OUT		ca_ipsec_sa_id_t	*ipsec_sa_id )
{
	ca_status_t ret;
	struct ipsec_sa_node *iter, *sa_node;

	if ((ipsec_sa == NULL) || (ipsec_sa_id == NULL))
		return CA_E_PARAM;

	if (ipsec_sa->auth_keylen * 4 > MAX_AUTH_KEY_LEN)
		return CA_E_PARAM;

	if (ipsec_sa->encryption_keylen * 4 > MAX_ENCRYPTION_KEY_LEN)
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SA info:\n", __func__);
		ipsec_sa_dump(ipsec_sa);
		ca_printf(KERN_INFO "%s: ---------------------------------------\n", __func__);
	}

	/* search if entry exists already */

	read_lock(&ipsec_sa_list_lock);
	list_for_each_entry(iter, &ipsec_sa_list, list) {
		if ((iter->ipsec_sa.spi == ipsec_sa->spi) && (iter->ipsec_sa.sa_dir == ipsec_sa->sa_dir)) {
			if (ca_api_debug & API_DBG_TUNNEL) {
				ca_printf(KERN_INFO "%s: There is same ipsec_sa (sa_id %u) already.\n", __func__, iter->sa_id);
			}
			*ipsec_sa_id = iter->sa_id;
			read_unlock(&ipsec_sa_list_lock);
			return CA_E_EXISTS;
		}

	}
	read_unlock(&ipsec_sa_list_lock);

	if ((ret = ca_api_sa_id_alloc(CA_TUNNEL_IPSEC, ipsec_sa->sa_dir, true, ipsec_sa_id)) != CA_E_OK) {
		ca_printf("%s: Fail to allocate SA_ID for IPSec. (sa_dir %d, ret %d)\n", __func__, ipsec_sa->sa_dir, ret);
		return ret;
	}

	/* Add into local ipsec_sa_list */
	sa_node = ca_zalloc_trace(sizeof(struct ipsec_sa_node), GFP_KERNEL, true);
	if (sa_node == NULL) {
		return CA_E_MEMORY;
	}
	sa_node->ipsec_sa = *ipsec_sa;
	sa_node->sa_id = *ipsec_sa_id;
	write_lock(&ipsec_sa_list_lock);
	list_add(&sa_node->list, &ipsec_sa_list);
	list_sort(NULL, &ipsec_sa_list, &ipsec_sa_node_cmp);
	write_unlock(&ipsec_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SA ID is %u.\n", __func__, *ipsec_sa_id);
	}

	return CA_E_OK;
}

ca_status_t ca_ipsec_sa_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_id_t	ipsec_sa_id )
{
	struct ipsec_sa_node *iter, *tmp_iter;
	ca_status_t ret;

	if ((ret = ca_api_sa_id_free(ipsec_sa_id)) != CA_E_OK) {
		ca_printf(KERN_INFO "%s: Fail to delete SA ID. (ret %d)\n", __func__, ret);
		return ret;
	}

	write_lock(&ipsec_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &ipsec_sa_list, list) {
		if (iter->sa_id == ipsec_sa_id) {
			list_del(&iter->list);
			ca_free_trace(iter);
			break;
		}
	}
	write_unlock(&ipsec_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SA ID %u is deleted.\n", __func__, ipsec_sa_id);
	}

	return CA_E_OK;
}

ca_status_t ca_ipsec_sa_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_ipsec_sa_id_t	ipsec_sa_id,
	CA_OUT		ca_ipsec_sa_t		*ipsec_sa )
{
	struct ipsec_sa_node *iter;

	read_lock_bh(&ipsec_sa_list_lock);
	list_for_each_entry(iter, &ipsec_sa_list, list) {
		if (iter->sa_id == ipsec_sa_id) {
			*ipsec_sa = iter->ipsec_sa;
			if (ca_api_debug & API_DBG_TUNNEL) {
				ca_printf(KERN_INFO "%s: IPSec SA of ipsec_sa_id %u is found.\n", __func__, ipsec_sa_id);
			}
			read_unlock_bh(&ipsec_sa_list_lock);
			return CA_E_OK;
		}
	}
	read_unlock_bh(&ipsec_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SA of ipsec_sa_id %u is NOT found.\n", __func__, ipsec_sa_id);
	}

	return CA_E_NOT_FOUND;
}

ca_status_t ca_ipsec_sa_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry )
{
	ca_status_t status;
	ca_uint32_t count = 0;
	ca_ipsec_sa_iterate_entry_t *ptr = NULL;
	struct ipsec_sa_node *iter, *tmp_iter;
	bool found = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL) return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_ipsec_sa_iterate_entry_t) * (return_entry->entry_count) > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) found = true;

	ptr = (ca_ipsec_sa_iterate_entry_t *) return_entry->entry_data;

	read_lock(&ipsec_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &ipsec_sa_list, list) {
		/* found the node stored in prev_handle, start to get contents from next node */
		if (found == false) {
			if (iter == return_entry->prev_handle) found = true;
			continue;
		}

		ptr->ipsec_sa = iter->ipsec_sa;
		ptr->sa_id = iter->sa_id;
		ptr++;
		count++;

		if (count >= return_entry->entry_count) break;
	}
	read_unlock(&ipsec_sa_list_lock);
	return_entry->entry_count = count;

	if (count == 0) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* list_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

/***************************************************************************
 * L2TP
 ***************************************************************************/

static ca_status_t l2tp_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	ca_l3_intf_t *parent_intf = NULL;
	ca_tunnel_cfg_t *parent_tunnel_cfg = NULL;
	ca_ipsec_sa_t parent_ipsec_sa_encrypt, parent_ipsec_sa_decrypt;
	aal_l2tp_idx4pe_entry_t l2tp_idx4pe_entry;
	ca_uint32_t rslt_idx4pe;
	ca_uint8_t ip_proto;
	int i;
	ca_status_t ret;
	l2tp_private_data_t *priv;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	if (ca_l3_intf_tbl[tunnel_cfg->parent_l3_intf_id].use == 0)
		return CA_E_PARAM;

	parent_intf = &ca_l3_intf_tbl[tunnel_cfg->parent_l3_intf_id].intf;
	if (parent_intf->type == CA_L3_INTF_TUNNEL) {
		/* L2TP over IPSec */
		parent_tunnel_cfg = &g_tunnel[parent_intf->tunnel_id]->tcfg;
		if (parent_tunnel_cfg->type != CA_TUNNEL_IPSEC)
			return CA_E_PARAM;

		if (ca_ipsec_sa_get(0, parent_tunnel_cfg->tunnel.ipsec.sa_id_encrypt, &parent_ipsec_sa_encrypt) != CA_E_OK)
			return CA_E_CONFLICT;
		if (ca_ipsec_sa_get(0, parent_tunnel_cfg->tunnel.ipsec.sa_id_decrypt, &parent_ipsec_sa_decrypt) != CA_E_OK)
			return CA_E_CONFLICT;

		/* L2TP over IPSec NAT-T must be UDP */
		if ((parent_ipsec_sa_encrypt.is_natt != 0) && (tunnel_cfg->tunnel.l2tp.encap_type != 0))
			return CA_E_CONFLICT;
		if ((parent_ipsec_sa_decrypt.is_natt != 0) && (tunnel_cfg->tunnel.l2tp.encap_type != 0))
			return CA_E_CONFLICT;
	} else if (parent_intf->type == CA_L3_INTF_BCAST) {
		/* L2TP */
	} else {
		return CA_E_CONFLICT;
	}

	if (tunnel_cfg->tunnel.l2tp.version == 2) {
		/* L2TPv2 */

		if (tunnel_cfg->tunnel.l2tp.encap_type != 0) /* UDP */
			return CA_E_CONFLICT;

	} else if (tunnel_cfg->tunnel.l2tp.version == 3) {
		/* L2TPv3 */

		if ((tunnel_cfg->tunnel.l2tp.cookie_len != 0) &&
		    (tunnel_cfg->tunnel.l2tp.cookie_len != 4) &&
		    (tunnel_cfg->tunnel.l2tp.cookie_len != 8)) {
			return CA_E_CONFLICT;
		}
	}

	/* check if same ca_l2tp_tunnel_cfg_t exists */
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (   (g_tunnel[i] != NULL)
		    && (g_tunnel[i]->tcfg.type == CA_TUNNEL_L2TP)
		    && (g_tunnel[i]->tcfg.tunnel.l2tp.session_id == tunnel_cfg->tunnel.l2tp.session_id) ) {
			*tunnel_id = i;
			return CA_E_EXISTS;
		}
	}

	if (parent_tunnel_cfg->type == CA_TUNNEL_IPSEC) {
		/* L2TP over IPSec */

		if (parent_ipsec_sa_encrypt.tunnel == 1) /* Tunnel mode */
			ip_proto = IPPROTO_L2TP;
		else if (parent_ipsec_sa_encrypt.is_natt != 0)
			ip_proto = IPPROTO_UDP;
		else
			ip_proto = (parent_ipsec_sa_encrypt.protocol) ? IPPROTO_AH : IPPROTO_ESP;

	} else {
		/* pure L2TP */

		if (tunnel_cfg->tunnel.l2tp.version == 2)
			ip_proto = IPPROTO_UDP;
		else if (tunnel_cfg->tunnel.l2tp.version == 3)
			ip_proto = (tunnel_cfg->tunnel.l2tp.encap_type == 1) ? IPPROTO_L2TP : IPPROTO_UDP;

	}

	/* get an L2TP index for PE */

	l2tp_idx4pe_entry.valid = 1;
	ret = aal_entry_add(AAL_TABLE_L2TP_IDX4PE, &l2tp_idx4pe_entry, &rslt_idx4pe);
	if (ret != AAL_E_OK) {
		return CA_E_INTERNAL;
	}

	/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_l2tp_ipc_entry_set(device_id, CA_TUNNEL_DIR_OUTBOUND, *tunnel_id, ip_proto, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB for encryption. (ret=%d)\n", __func__, ret);
		aal_entry_del(AAL_TABLE_L2TP_IDX4PE, rslt_idx4pe);
		return CA_E_INTERNAL;
	}

	ret = ca_l2tp_ipc_entry_set(device_id, CA_TUNNEL_DIR_INBOUND, *tunnel_id, ip_proto, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set IPSec SADB for encryption. (ret=%d)\n", __func__, ret);
		aal_entry_del(AAL_TABLE_L2TP_IDX4PE, rslt_idx4pe);
		return CA_E_INTERNAL;
	}
#endif
	/* save private data */

	priv = ca_zalloc_trace(sizeof(l2tp_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for L2TP private data.\n", __func__);
		return CA_E_MEMORY;
	}
	priv->idx4pe = rslt_idx4pe;
	g_tunnel[*tunnel_id]->private_data = priv;

	return CA_E_OK;
}

static ca_status_t l2tp_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_NOT_SUPPORT;
}

static ca_status_t l2tp_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	ca_status_t ret;

	if (g_tunnel[tunnel_id] == NULL)
		return CA_E_PARAM;

	if (g_tunnel[tunnel_id]->tcfg.type != CA_TUNNEL_L2TP)
		return CA_E_PARAM;

	/* delete info from PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_l2tp_ipc_entry_del(device_id, CA_TUNNEL_DIR_OUTBOUND, ((l2tp_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}

	ret = ca_l2tp_ipc_entry_del(device_id, CA_TUNNEL_DIR_INBOUND, ((l2tp_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}
#endif
	ret = aal_entry_del(AAL_TABLE_L2TP_IDX4PE, ((l2tp_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete L2TP IDX4PE entry. (index %u, ret %d)\n",
				__func__, ((l2tp_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe, ret);
		return CA_E_INTERNAL;
	}
	printk(KERN_INFO "%s: L2TP IDX4PE entry of index %u is deleted.\n", __func__, ((l2tp_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

/***************************************************************************
 * PPTP
 ***************************************************************************/

struct pptp_sa_node {
	ca_pptp_sa_t		pptp_sa;
	ca_pptp_sa_id_t		sa_id;

	struct list_head	list;
};

static LIST_HEAD(pptp_sa_list);
static DEFINE_RWLOCK(pptp_sa_list_lock);

static int pptp_sa_node_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct pptp_sa_node *na = list_entry(a, struct pptp_sa_node, list);
	struct pptp_sa_node *nb = list_entry(b, struct pptp_sa_node, list);

	if (na->sa_id > nb->sa_id) return 1;
	if (na->sa_id < nb->sa_id) return -1;
	return 0;
}

static void pptp_sa_dump(ca_pptp_sa_t *pptp_sa)
{
	int i;

	if (pptp_sa == NULL) return;

	ca_printf(KERN_INFO "    direction = %d ", pptp_sa->direction);
	switch (pptp_sa->direction) {
	case CA_TUNNEL_DIR_OUTBOUND:
		ca_printf("[Upstream, Encryption]\n"); break;
	case CA_TUNNEL_DIR_INBOUND:
		ca_printf("[Downstream, Decryption]\n"); break;
	default:
		ca_printf("[UNKNOWN]\n"); break;
	}

	ca_printf(KERN_INFO "    call_id = 0x%04x\n", pptp_sa->call_id);
	ca_printf(KERN_INFO "    state_less = %u\n", pptp_sa->state_less);

	ca_printf(KERN_INFO "    crypto_type = %u ", pptp_sa->crypto_type);
	switch (pptp_sa->crypto_type) {
	case CA_CRYPTO_NONE:
		ca_printf("[None]\n"); break;
	case CA_CRYPTO_MPPE40:
		ca_printf("[MPPE 40-bit]\n"); break;
	case CA_CRYPTO_MPPE128:
		ca_printf("[MPPE 128-bit]\n"); break;
	case CA_CRYPTO_INVALID:
	default:
		ca_printf("[INVALID]\n"); break;
	}

	ca_printf(KERN_INFO "    key = ");
	for (i = 0; i < CA_PPTP_DEFAULT_KEY_LENGTH; i++) {
		ca_printf("%02x ", pptp_sa->key[i]);
	}
	ca_printf("\n");

	ca_printf(KERN_INFO "    keylen = %u\n", pptp_sa->keylen);
	ca_printf(KERN_INFO "    sequence_number = %u\n", pptp_sa->sequence_number);
}

static ca_status_t pptp_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	return CA_E_NOT_SUPPORT;
}

static ca_status_t pptp_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_NOT_SUPPORT;
}

static ca_status_t pptp_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_NOT_SUPPORT;
}

ca_status_t ca_pptp_sa_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_t		*pptp_sa,
	CA_OUT		ca_pptp_sa_id_t		*pptp_sa_id )
{
	ca_status_t ret;
	struct pptp_sa_node *iter, *sa_node;

	if ((pptp_sa == NULL) || (pptp_sa_id == NULL))
		return CA_E_PARAM;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: PPTP SA info:\n", __func__);
		pptp_sa_dump(pptp_sa);
		ca_printf(KERN_INFO "%s: ---------------------------------------\n", __func__);
	}

	/* search if entry exists already */

	read_lock(&pptp_sa_list_lock);
	list_for_each_entry(iter, &pptp_sa_list, list) {
		if ((iter->pptp_sa.call_id == pptp_sa->call_id) && (iter->pptp_sa.direction == pptp_sa->direction)) {
			if (ca_api_debug & API_DBG_TUNNEL) {
				ca_printf(KERN_INFO "%s: There is same pptp_sa (sa_id %u) already.\n", __func__, iter->sa_id);
			}
			*pptp_sa_id = iter->sa_id;
			read_unlock(&pptp_sa_list_lock);
			return CA_E_EXISTS;
		}
	}
	read_unlock(&pptp_sa_list_lock);

	if ((ret = ca_api_sa_id_alloc(CA_TUNNEL_PPTP, pptp_sa->direction, true, pptp_sa_id)) != CA_E_OK) {
		ca_printf("%s: Fail to allocate SA_ID for PPTP. (direction %d, ret %d)\n", __func__, pptp_sa->direction, ret);
		return ret;
	}

	/* Add into local pptp_sa_list */
	sa_node = ca_zalloc_trace(sizeof(struct pptp_sa_node), GFP_KERNEL, true);
	if (sa_node == NULL) {
		return CA_E_MEMORY;
	}
	sa_node->pptp_sa = *pptp_sa;
	sa_node->sa_id = *pptp_sa_id;
	write_lock(&pptp_sa_list_lock);
	list_add(&sa_node->list, &pptp_sa_list);
	list_sort(NULL, &pptp_sa_list, &pptp_sa_node_cmp);
	write_unlock(&pptp_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: PPTP SA ID is %u.\n", __func__, *pptp_sa_id);
	}

	return CA_E_OK;
}

ca_status_t ca_pptp_sa_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_id_t		pptp_sa_id )
{
	struct pptp_sa_node *iter, *tmp_iter;
	ca_status_t ret;

	if ((ret = ca_api_sa_id_free(pptp_sa_id)) != CA_E_OK) {
		ca_printf(KERN_INFO "%s: Fail to delete SA ID. (ret %d)\n", __func__, ret);
		return ret;
	}

	write_lock(&pptp_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &pptp_sa_list, list) {
		if (iter->sa_id == pptp_sa_id) {
			list_del(&iter->list);
			ca_free_trace(iter);
			break;
		}
	}
	write_unlock(&pptp_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: PPTP SA ID %u is deleted.\n", __func__, pptp_sa_id);
	}

	return CA_E_OK;
}

ca_status_t ca_pptp_sa_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_pptp_sa_id_t		pptp_sa_id,
	CA_OUT		ca_pptp_sa_t		*pptp_sa )
{
	struct pptp_sa_node *iter;

	read_lock(&pptp_sa_list_lock);
	list_for_each_entry(iter, &pptp_sa_list, list) {
		if (iter->sa_id == pptp_sa_id) {
			*pptp_sa = iter->pptp_sa;
			if (ca_api_debug & API_DBG_TUNNEL) {
				ca_printf(KERN_INFO "%s: PPTP SA of pptp_sa_id %u is found.\n", __func__, pptp_sa_id);
			}
			read_unlock(&pptp_sa_list_lock);
			return CA_E_OK;
		}
	}
	read_unlock(&pptp_sa_list_lock);

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: PPTP SA of pptp_sa_id %u is NOT found.\n", __func__, pptp_sa_id);
	}

	return CA_E_NOT_FOUND;
}

ca_status_t ca_pptp_sa_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry )
{
	ca_status_t status;
	ca_uint32_t count = 0;
	ca_pptp_sa_iterate_entry_t *ptr = NULL;
	struct pptp_sa_node *iter, *tmp_iter;
	bool found = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL) return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_pptp_sa_iterate_entry_t) * (return_entry->entry_count) > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) found = true;

	ptr = (ca_pptp_sa_iterate_entry_t *) return_entry->entry_data;

	read_lock(&pptp_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &pptp_sa_list, list) {
		/* found the node stored in prev_handle, start to get contents from next node */
		if (found == false) {
			if (iter == return_entry->prev_handle) found = true;
			continue;
		}

		ptr->pptp_sa = iter->pptp_sa;
		ptr->sa_id = iter->sa_id;
		ptr++;
		count++;

		if (count >= return_entry->entry_count) break;
	}
	read_unlock(&pptp_sa_list_lock);
	return_entry->entry_count = count;

	if (count == 0) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* list_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

ca_status_t ca_pptp_key_change (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id )
{
	return CA_E_NOT_SUPPORT;
}

/***************************************************************************
 * MACSec
 ***************************************************************************/

struct macsec_sa_node {
	ca_macsec_sa_t		sa_context;
	ca_macsec_sa_id_t	sa_id;

	struct list_head	list;
};

static LIST_HEAD(macsec_sa_list);
static DEFINE_RWLOCK(macsec_sa_list_lock);
static ca_macsec_global_config_t g_macsec_conf;

static int macsec_sa_node_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct macsec_sa_node *na = list_entry(a, struct macsec_sa_node, list);
	struct macsec_sa_node *nb = list_entry(b, struct macsec_sa_node, list);

	if (na->sa_context.sa_id > nb->sa_context.sa_id) return 1;
	if (na->sa_context.sa_id < nb->sa_context.sa_id) return -1;
	return 0;
}

static void macsec_global_config_dump(void)
{
	ca_printf(KERN_INFO "Global MACSec config:\n");
	ca_printf(KERN_INFO "  fwd_untagged_oam = %u\n", g_macsec_conf.fwd_untagged_oam);
	ca_printf(KERN_INFO "  fwd_untagged_mpcp = %u\n", g_macsec_conf.fwd_untagged_mpcp);
	ca_printf(KERN_INFO "  fwd_untagged_ethertype = %u\n", g_macsec_conf.fwd_untagged_ethertype);
	ca_printf(KERN_INFO "  skip_oam = %u\n", g_macsec_conf.skip_oam);
	ca_printf(KERN_INFO "  skip_mpcp = %u\n", g_macsec_conf.skip_mpcp);
	ca_printf(KERN_INFO "  skip_untagged_ethertype = %u\n", g_macsec_conf.skip_untagged_ethertype);
	ca_printf(KERN_INFO "  untagged_ethertype = %u\n", g_macsec_conf.untagged_ethertype);
	ca_printf(KERN_INFO "  confidential_offset_error = %u\n", g_macsec_conf.confidential_offset_error);
	ca_printf(KERN_INFO "  data_path_reset = %u\n", g_macsec_conf.data_path_reset);
	ca_printf(KERN_INFO "  KaY_reserved = %u\n", g_macsec_conf.KaY_reserved);
	ca_printf(KERN_INFO "  drop_untagged_rx_frames = = %u\n", g_macsec_conf.drop_untagged_rx_frames);
}

static void macsec_sa_dump(ca_macsec_sa_t *macsec_sa)
{
	int i;

	if (macsec_sa == NULL) return;

	ca_printf(KERN_INFO "    sa_id = %u\n", macsec_sa->sa_id);
	ca_printf(KERN_INFO "    direction = %u\n", macsec_sa->direction);
	ca_printf(KERN_INFO "    association_number = %u\n", macsec_sa->association_number);

	ca_printf(KERN_INFO "    [%c] key0[%d] = ", (macsec_sa->sa_db.mask.key0_valid) ? 'V' : ' ', MACSEC_KEY_LENGTH);
	for (i = 0; i < MACSEC_KEY_LENGTH; i++) {
		ca_printf("0x%x ", macsec_sa->sa_db.key0[i]);
	}
	ca_printf("\n");

	ca_printf(KERN_INFO "    [%c] key1[%d] = ", (macsec_sa->sa_db.mask.key1_valid) ? 'V' : ' ', MACSEC_KEY_LENGTH);
	for (i = 0; i < MACSEC_KEY_LENGTH; i++) {
		ca_printf("0x%x ", macsec_sa->sa_db.key1[i]);
	}
	ca_printf("\n");

	ca_printf(KERN_INFO "    [%c] lapn = %u\n", (macsec_sa->sa_db.mask.lapn_valid) ? 'V' : ' ', macsec_sa->sa_db.lapn);
	ca_printf(KERN_INFO "    [%c] npn = %u\n", (macsec_sa->sa_db.mask.npn_valid) ? 'V' : ' ', macsec_sa->sa_db.npn);
	ca_printf(KERN_INFO "    [%c] pn_threshold = %u\n", (macsec_sa->sa_db.mask.pn_threshold_valid) ? 'V' : ' ', macsec_sa->sa_db.pn_threshold);
	ca_printf(KERN_INFO "    [%c] replay_window = %u\n", (macsec_sa->sa_db.mask.replay_window_valid) ? 'V' : ' ', macsec_sa->sa_db.replay_window);
	ca_printf(KERN_INFO "    [%c] sci = 0x%016llx\n", (macsec_sa->sa_db.mask.sci_valid) ? 'V' : ' ', macsec_sa->sa_db.sci);
	ca_printf(KERN_INFO "    [%c] replay_window = %u\n", (macsec_sa->sa_db.mask.replay_window_valid) ? 'V' : ' ', macsec_sa->sa_db.replay_window);
	ca_printf(KERN_INFO "    [%c] mac_addr = %pM\n", (macsec_sa->sa_db.mask.mac_addr) ? 'V' : ' ', macsec_sa->sa_db.mac_addr);
	ca_printf(KERN_INFO "    [%c] SADB control flags:\n", (macsec_sa->sa_db.mask.sa_db_control_flags_valid) ? 'V' : ' ');
	ca_printf(KERN_INFO "          end_station = %u\n", macsec_sa->sa_db.sa_db_control_flags.end_station);
	ca_printf(KERN_INFO "          sci_encoded = %u\n", macsec_sa->sa_db.sa_db_control_flags.sci_encoded);
	ca_printf(KERN_INFO "          single_copy_broadcast = %u\n", macsec_sa->sa_db.sa_db_control_flags.single_copy_broadcast);

	ca_printf(KERN_INFO "          frame_validation = %u ", macsec_sa->sa_db.sa_db_control_flags.frame_validation);
	switch (macsec_sa->sa_db.sa_db_control_flags.frame_validation) {
	case 0: ca_printf("[Strict]\n"); break;
	case 1: ca_printf("[Check]\n"); break;
	case 2: ca_printf("[Disabled]\n"); break;
	default: ca_printf("[UNKNOWN]\n"); break;
	}

	ca_printf(KERN_INFO "          replay_check = %u\n", macsec_sa->sa_db.sa_db_control_flags.replay_check);
	ca_printf(KERN_INFO "          association_number = %u\n", macsec_sa->sa_db.sa_db_control_flags.association_number);
	ca_printf(KERN_INFO "          confidential_offset = %u\n", macsec_sa->sa_db.sa_db_control_flags.confidential_offset);

	ca_printf(KERN_INFO "          security_mode = %u ", macsec_sa->sa_db.sa_db_control_flags.security_mode);
	switch (macsec_sa->sa_db.sa_db_control_flags.security_mode) {
	case 0: ca_printf("[Encryption + Authentication]\n"); break;
	case 1: ca_printf("[Authentication]\n"); break;
	case 2: ca_printf("[Decryption only]\n"); break;
	default: ca_printf("[UNKNOWN]\n"); break;
	}
}

void sw_macsec_table_dump(void)
{
	struct macsec_sa_node *iter;
	int index = 0;

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		ca_printf(KERN_INFO "----- index %d ------------------------------------\n");
		ca_printf(KERN_INFO "Internal  data:\n");
		ca_printf(KERN_INFO "  sa_id = %u\n", iter->sa_id);
		ca_printf(KERN_INFO "SA context:\n");
		macsec_sa_dump(&iter->sa_context);

		index++;
	}
	read_unlock(&macsec_sa_list_lock);

	ca_printf(KERN_INFO "=================================================================\n");
}

static ca_status_t macsec_tunnel_add (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	*tunnel_id)
{
	ca_macsec_sa_t macsec_sa_outbound, macsec_sa_inbound;
	ca_status_t ret;
	int i;
	aal_macsec_idx4pe_entry_t macsec_idx4pe_entry;
	unsigned int rslt_idx4pe;
	macsec_private_data_t *priv;

	if (tunnel_cfg == NULL)
		return CA_E_PARAM;

	macsec_sa_outbound.sa_id = tunnel_cfg->tunnel.macsec.tx_sa;
	if (sw_macsec_sa_get(0, &macsec_sa_outbound) != CA_E_OK)
		return CA_E_PARAM;

	macsec_sa_inbound.sa_id = tunnel_cfg->tunnel.macsec.rx_sa;
	if (sw_macsec_sa_get(0, &macsec_sa_inbound) != CA_E_OK)
		return CA_E_PARAM;

	if (macsec_sa_outbound.direction != CA_TUNNEL_DIR_OUTBOUND)
		return CA_E_PARAM;

	if (macsec_sa_inbound.direction != CA_TUNNEL_DIR_INBOUND)
		return CA_E_PARAM;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (   (g_tunnel[i] != NULL)
		    && (g_tunnel[i]->tcfg.type == CA_TUNNEL_MACSEC)
		    && (memcmp(&g_tunnel[i]->tcfg.tunnel.macsec, &tunnel_cfg->tunnel.macsec, sizeof(ca_macsec_tunnel_cfg_t)) == 0)
		    && (memcmp(&g_tunnel[i]->tcfg.src_addr, &tunnel_cfg->src_addr, sizeof(ca_ip_address_t)) == 0)
		    && (memcmp(&g_tunnel[i]->tcfg.dest_addr, &tunnel_cfg->dest_addr, sizeof(ca_ip_address_t)) == 0) ) {
			*tunnel_id = i;
			return CA_E_EXISTS;
		}
	}

	/* get an MACSec index for PE */

	macsec_idx4pe_entry.valid = 1;
	macsec_idx4pe_entry.sa_id_outbound = tunnel_cfg->tunnel.macsec.tx_sa;
	macsec_idx4pe_entry.sa_id_inbound = tunnel_cfg->tunnel.macsec.rx_sa;
	ret = aal_entry_add(AAL_TABLE_MACSEC_IDX4PE, &macsec_idx4pe_entry, &rslt_idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to add MACSec IDX4PE entry. (ret %d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	/* set info to PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_macsec_ipc_sadb_set(device_id, CA_TUNNEL_DIR_OUTBOUND, tunnel_cfg, &macsec_sa_outbound, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MACSec SADB for encryption. (ret=%d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	ret = ca_macsec_ipc_sadb_set(device_id, CA_TUNNEL_DIR_INBOUND, tunnel_cfg, &macsec_sa_inbound, rslt_idx4pe);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to set MACSec SADB for decryption. (ret=%d)\n", __func__, ret);
		ca_macsec_ipc_sadb_del(device_id, CA_TUNNEL_DIR_OUTBOUND, rslt_idx4pe);
		return CA_E_INTERNAL;
	}
#endif
	/* save private data */

	priv = ca_zalloc_trace(sizeof(macsec_private_data_t), GFP_KERNEL, true);
	if (priv == NULL) {
		ca_printf("%s: Fail to allocate memory for MACSec private data.\n", __func__);
		return CA_E_MEMORY;
	}
	priv->idx4pe = rslt_idx4pe;
	g_tunnel[*tunnel_id]->private_data = priv;

	return CA_E_OK;
}

static ca_status_t macsec_tunnel_update (
	ca_device_id_t	device_id,
	ca_tunnel_cfg_t	*tunnel_cfg,
	ca_tunnel_id_t	tunnel_id)
{
	return CA_E_NOT_SUPPORT;
}

static ca_status_t macsec_tunnel_delete (
	ca_device_id_t	device_id,
	ca_tunnel_id_t	tunnel_id)
{
	ca_tunnel_cfg_t *tcfg = NULL;
	ca_status_t ret;

	if (g_tunnel[tunnel_id] == NULL)
		return CA_E_PARAM;

	if (g_tunnel[tunnel_id]->tcfg.type != CA_TUNNEL_MACSEC)
		return CA_E_PARAM;

	tcfg = &g_tunnel[tunnel_id]->tcfg;

	/* delete info from PE */
#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_macsec_ipc_sadb_del(device_id, CA_TUNNEL_DIR_OUTBOUND, ((macsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}

	ret = ca_macsec_ipc_sadb_del(device_id, CA_TUNNEL_DIR_INBOUND, ((macsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != CA_E_OK) {
		return CA_E_INTERNAL;
	}
#endif
	ret = aal_entry_del(AAL_TABLE_MACSEC_IDX4PE, ((macsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete MACSec IDX4PE entry. (index %u, ret %d)\n",
				__func__, ((macsec_private_data_t *) g_tunnel[tunnel_id]->private_data)->idx4pe, ret);
		return CA_E_INTERNAL;
	}

	ca_free(g_tunnel[tunnel_id]->private_data);

	return CA_E_OK;
}

ca_status_t sw_macsec_init (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_macsec_global_config_t		*macsec_global_conf)
{
	if (macsec_global_conf == NULL) return CA_E_PARAM;

	if (ca_api_debug & API_DBG_MACSEC) {
		macsec_global_config_dump();
	}

	// TODO: validation

	g_macsec_conf = *macsec_global_conf;

	return CA_E_OK;
}

ca_status_t sw_macsec_global_config_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t		*macsec_global_conf)
{
	if (macsec_global_conf == NULL) return CA_E_PARAM;

	*macsec_global_conf = g_macsec_conf;

	return CA_E_OK;
}

ca_status_t sw_macsec_sa_add (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_t				*sa_context)
{
	ca_status_t ret;
	bool is_encrypted;
	ca_tunnel_direction_t direction;
	struct macsec_sa_node *sa_node;

	if (sa_context == NULL) return CA_E_PARAM;

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: MACSec SA context:\n", __func__);
		macsec_sa_dump(sa_context);
		ca_printf(KERN_INFO "%s: ---------------------------------------\n", __func__);
	}

	/* validate API parameters */

	if (sa_context->association_number > 3)
		return CA_E_PARAM;

	if (sa_context->sa_db.mask.sa_db_control_flags_valid == 0) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! No SA_DB flags to determinate SecTAG.\n", __func__);
		}
		return CA_E_PARAM;
	}

	if (sa_context->direction == 0) { /* Tx */
		if ((sa_context->sa_db.sa_db_control_flags.security_mode != 0) && (sa_context->sa_db.sa_db_control_flags.security_mode != 1)) {
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: ERROR! SA Context is for Tx, but parameter doesn't match.\n", __func__);
			}
			return CA_E_PARAM;
		} else {
			direction = CA_TUNNEL_DIR_OUTBOUND;
		}
	} else if (sa_context->direction == 1) { /* Rx */
		direction = CA_TUNNEL_DIR_INBOUND;
	}

	if ((sa_context->sa_db.sa_db_control_flags.security_mode == 0) || (sa_context->sa_db.sa_db_control_flags.security_mode == 2))
		is_encrypted = true;
	else if (sa_context->sa_db.sa_db_control_flags.security_mode == 1)
		is_encrypted = false;
	else
		return CA_E_PARAM;

	if (   ((sa_context->sa_db.sa_db_control_flags.end_station == 1) || (sa_context->sa_db.sa_db_control_flags.single_copy_broadcast == 1))
	    && (sa_context->sa_db.sa_db_control_flags.sci_encoded == 1) ) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! In SecTAG, SC bit can't be coexist with ES bit and SCB bit.\n", __func__);
		}
		return CA_E_PARAM;
	}

	if ((sa_context->sa_db.mask.lapn_valid == 1) && (sa_context->sa_db.lapn == 0)) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! In Tx SecTAG, Packet Number can't be 0.\n", __func__);
		}
		return CA_E_PARAM;
	}

	if ((sa_context->sa_db.mask.npn_valid == 1) && (sa_context->sa_db.npn == 0)) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! In Rx SecTAG, expected Packet Number can't be 0.\n", __func__);
		}
		return CA_E_PARAM;
	}

	/* allocate resources */

	if ((ret = ca_api_sa_id_alloc(CA_TUNNEL_MACSEC, direction, is_encrypted, &sa_context->sa_id)) != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to allocate SA ID for MACSec. (ret %d)\n", __func__, ret);
		return ret;
	}

	/* Add into local macsec_sa_list */
	sa_node = ca_zalloc_trace(sizeof(struct macsec_sa_node), GFP_KERNEL, true);
	if (sa_node == NULL) {
		return CA_E_MEMORY;
	}
	sa_node->sa_context = *sa_context;
	sa_node->sa_id = sa_context->sa_id;
	write_lock(&macsec_sa_list_lock);
	list_add(&sa_node->list, &macsec_sa_list);
	list_sort(NULL, &macsec_sa_list, &macsec_sa_node_cmp);
	write_unlock(&macsec_sa_list_lock);

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: MACSec SA ID is %u.\n", __func__, sa_context->sa_id);
	}

	return ret;
}

ca_status_t sw_macsec_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_macsec_sa_t				*sa_context)
{
	struct macsec_sa_node *iter;

	if (sa_context == NULL) return CA_E_PARAM;

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: sa_id=%u\n", __func__, sa_context->sa_id);
	}

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		if (iter->sa_id == sa_context->sa_id) {
			*sa_context = iter->sa_context;
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: Found MACSec SA of sa_id %u.\n", __func__, sa_context->sa_id);
			}
			read_unlock(&macsec_sa_list_lock);
			return CA_E_OK;
		}
	}
	read_unlock(&macsec_sa_list_lock);

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: MACSec SA of sa_id %u is NOT found.\n", __func__, sa_context->sa_id);
	}

	return CA_E_NOT_FOUND;
}

ca_status_t sw_macsec_sa_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id)
{
	struct macsec_sa_node *iter, *tmp_iter;
	ca_status_t ret;

	if ((ret = ca_api_sa_id_free(sa_id)) != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to delete SA ID %u. (ret %d)\n", __func__, sa_id, ret);
		return ret;
	}

	write_lock(&macsec_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &macsec_sa_list, list) {
		if (iter->sa_id == sa_id) {
			list_del(&iter->list);
			ca_free_trace(iter);
			break;
		}
	}
	write_unlock(&macsec_sa_list_lock);

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: MACSec SA ID %u is deleted.\n", __func__, sa_id);
	}

	return ret;
}

ca_status_t sw_macsec_sa_update (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_t				*sa_context)
{
	struct macsec_sa_node *iter;
	ca_status_t ret = CA_E_OK;
	bool found = false;

	if (sa_context == NULL) return CA_E_PARAM;

	if (ca_api_debug & API_DBG_MACSEC) {
		ca_printf(KERN_INFO "%s: MACSec SA context:\n", __func__);
		macsec_sa_dump(sa_context);
		ca_printf(KERN_INFO "%s: ---------------------------------------\n", __func__);
	}

	if (sa_context->association_number > 3)
		return CA_E_PARAM;

	write_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		if (iter->sa_id == sa_context->sa_id) {
			found = true;
			break;
		}
	}

	if (!found)
		return CA_E_NOT_FOUND;

	/* validate API parameters */

	if (iter->sa_context.association_number != sa_context->association_number) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! Current association_number %u doesn't match the parameter.\n", __func__, iter->sa_context.association_number);
		}
		ret = CA_E_PARAM;
		goto leave_sw_macsec_sa_update;
	}

	if (iter->sa_context.direction != sa_context->direction) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: ERROR! Not allow to change direction.\n", __func__);
		}
		ret = CA_E_PARAM;
		goto leave_sw_macsec_sa_update;
	}

	if (sa_context->sa_db.mask.sa_db_control_flags_valid == 1) {
		if (iter->sa_context.sa_db.sa_db_control_flags.security_mode != sa_context->sa_db.sa_db_control_flags.security_mode) {
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: ERROR! Not allow to change security_mode.\n", __func__);
			}
			ret = CA_E_PARAM;
			goto leave_sw_macsec_sa_update;
		}

		if (   ((sa_context->sa_db.sa_db_control_flags.end_station == 1) || (sa_context->sa_db.sa_db_control_flags.single_copy_broadcast == 1))
				&& (sa_context->sa_db.sa_db_control_flags.sci_encoded == 1) ) {
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: ERROR! In SecTAG, SC bit can't be coexist with ES bit and SCB bit.\n", __func__);
			}
			ret = CA_E_PARAM;
			goto leave_sw_macsec_sa_update;
		}

		if ((sa_context->sa_db.mask.lapn_valid == 1) && (sa_context->sa_db.lapn == 0)) {
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: ERROR! In Tx SecTAG, Packet Number can't be 0.\n", __func__);
			}
			ret = CA_E_PARAM;
			goto leave_sw_macsec_sa_update;
		}

		if ((sa_context->sa_db.mask.npn_valid == 1) && (sa_context->sa_db.npn == 0)) {
			if (ca_api_debug & API_DBG_MACSEC) {
				ca_printf(KERN_INFO "%s: ERROR! In Rx SecTAG, expected Packet Number can't be 0.\n", __func__);
			}
			ret = CA_E_PARAM;
			goto leave_sw_macsec_sa_update;
		}
	}

	/* only allow to update inactive key */
	if (   ((iter->sa_context.association_number == 0) && (sa_context->sa_db.mask.key0_valid == 1))
	    || ((iter->sa_context.association_number == 1) && (sa_context->sa_db.mask.key1_valid == 1))
	    || ((iter->sa_context.association_number == 2) && (sa_context->sa_db.mask.key2_valid == 1))
	    || ((iter->sa_context.association_number == 3) && (sa_context->sa_db.mask.key3_valid == 1)) ) {
		ca_printf(KERN_ERR "%s: Not allow to change active key.\n", __func__);
		ret = CA_E_PARAM;
		goto leave_sw_macsec_sa_update;
	}

	/* start to update existing macsec_sa */

	if (sa_context->sa_db.mask.key0_valid == 1) {
		memcpy(iter->sa_context.sa_db.key0, sa_context->sa_db.key0, MACSEC_KEY_LENGTH);
		iter->sa_context.sa_db.mask.key0_valid = 1;
	}
	if (sa_context->sa_db.mask.key1_valid == 1) {
		memcpy(iter->sa_context.sa_db.key1, sa_context->sa_db.key1, MACSEC_KEY_LENGTH);
		iter->sa_context.sa_db.mask.key1_valid = 1;
	}
	if (sa_context->sa_db.mask.key2_valid == 1) {
		memcpy(iter->sa_context.sa_db.key2, sa_context->sa_db.key2, MACSEC_KEY_LENGTH);
		iter->sa_context.sa_db.mask.key2_valid = 1;
	}
	if (sa_context->sa_db.mask.key3_valid == 1) {
		memcpy(iter->sa_context.sa_db.key3, sa_context->sa_db.key3, MACSEC_KEY_LENGTH);
		iter->sa_context.sa_db.mask.key3_valid = 1;
	}

	if (sa_context->sa_db.mask.lapn_valid == 1) {
		iter->sa_context.sa_db.lapn = sa_context->sa_db.lapn;
		iter->sa_context.sa_db.mask.lapn_valid = 1;
	}

	if (sa_context->sa_db.mask.npn_valid == 1) {
		iter->sa_context.sa_db.npn = sa_context->sa_db.npn;
		iter->sa_context.sa_db.mask.npn_valid = 1;
	}

	if (sa_context->sa_db.mask.sci_valid == 1) {
		iter->sa_context.sa_db.sci = sa_context->sa_db.sci;
		iter->sa_context.sa_db.mask.sci_valid = 1;
	}

	if (sa_context->sa_db.mask.pn_threshold_valid == 1) {
		iter->sa_context.sa_db.pn_threshold = sa_context->sa_db.pn_threshold;
		iter->sa_context.sa_db.mask.pn_threshold_valid = 1;
	}

	if (sa_context->sa_db.mask.replay_window_valid == 1) {
		iter->sa_context.sa_db.replay_window = sa_context->sa_db.replay_window;
		iter->sa_context.sa_db.mask.replay_window_valid = 1;
	}

	if (sa_context->sa_db.mask.mac_addr == 1) {
		memcpy(iter->sa_context.sa_db.mac_addr, sa_context->sa_db.mac_addr, sizeof(ca_mac_addr_t));
		iter->sa_context.sa_db.mask.mac_addr = 1;
	}

	if (sa_context->sa_db.mask.sa_db_control_flags_valid == 1) {
		iter->sa_context.sa_db.sa_db_control_flags = sa_context->sa_db.sa_db_control_flags;
		iter->sa_context.sa_db.mask.sa_db_control_flags_valid = 1;

	}

	// TODO: update to PE here? or outside loop?

leave_sw_macsec_sa_update:
	write_unlock(&macsec_sa_list_lock);
	return ret;
}

ca_status_t sw_macsec_sa_active_index_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_IN	ca_uint8_t				active_index)
{
	struct macsec_sa_node *iter;
	bool found = false;

	if (active_index > 3) {
		return CA_E_PARAM;
	}

	// TODO: key change, based on MKA

	write_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		if (iter->sa_id == sa_id) {
			found = true;
			break;
		}
	}

	if (!found)
		return CA_E_NOT_FOUND;

	switch (active_index) {
	case 0:
		if (iter->sa_context.sa_db.mask.key0_valid == 0) return CA_E_PARAM;
	case 1:
		if (iter->sa_context.sa_db.mask.key1_valid == 0) return CA_E_PARAM;
	case 2:
		if (iter->sa_context.sa_db.mask.key2_valid == 0) return CA_E_PARAM;
	case 3:
		if (iter->sa_context.sa_db.mask.key3_valid == 0) return CA_E_PARAM;
	default:
		return CA_E_PARAM;
	}

	iter->sa_context.association_number = active_index;

	write_unlock(&macsec_sa_list_lock);

	// TODO: update to PE

	return CA_E_OK;
}

ca_status_t sw_macsec_sa_active_index_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_OUT	ca_uint8_t				*active_index)
{
	struct macsec_sa_node *iter;

	if (active_index == NULL) return CA_E_PARAM;

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		if (iter->sa_id == sa_id) {
			*active_index = iter->sa_context.association_number;
			read_unlock(&macsec_sa_list_lock);
			return CA_E_OK;
		}
	}
	read_unlock(&macsec_sa_list_lock);

	return CA_E_NOT_FOUND;
}

ca_status_t sw_macsec_sa_iterate (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_iterator_t				*return_entry)
{
	ca_status_t status;
	ca_uint32_t count = 0;
	ca_macsec_sa_t *ptr = NULL;
	struct macsec_sa_node *iter, *tmp_iter;
	bool found = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL) return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_macsec_sa_t) * (return_entry->entry_count) > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) found = true;

	ptr = (ca_macsec_sa_t *) return_entry->entry_data;

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &macsec_sa_list, list) {
		/* found the node stored in prev_handle, start to get contents from next node */
		if (found == false) {
			if (iter == return_entry->prev_handle) found = true;
			continue;
		}

		*ptr = iter->sa_context;
		ptr++;
		count++;

		if (count >= return_entry->entry_count) break;
	}
	read_unlock(&macsec_sa_list_lock);
	return_entry->entry_count = count;

	if (count == 0) {
		return_entry->prev_handle = NULL;
		status = CA_E_UNAVAIL;
	} else {
		/* list_for_each_entry_safe() starts from head->next, so set the previous node of 'iter' to prev_handle. */
		return_entry->prev_handle = iter;
		status = (count == 0) ? CA_E_UNAVAIL : CA_E_OK;
	}

	return status;
}

ca_status_t sw_macsec_port_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			rx_sa_id,
	CA_IN	ca_macsec_sa_id_t			tx_sa_id)
{
	struct macsec_sa_node *iter_rx, *iter_tx;
	bool found_rx = false, found_tx = false;

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter_rx, &macsec_sa_list, list) {
		if (iter_rx->sa_id == rx_sa_id) {
			found_rx = true;
			break;
		}
	}
	list_for_each_entry(iter_tx, &macsec_sa_list, list) {
		if (iter_tx->sa_id == tx_sa_id) {
			found_tx = true;
			break;
		}
	}
	read_unlock(&macsec_sa_list_lock);

	if (!found_rx) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: Can not found Rx SA ID %u.\n", __func__, rx_sa_id);
		}
		return CA_E_PARAM;
	}

	if (!found_tx) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: Can not found Tx SA ID %u.\n", __func__, tx_sa_id);
		}
		return CA_E_PARAM;
	}

	// llid: when port type is not PON, ignore llid.

	// TODO: what to set?

	return CA_E_OK;
}

ca_status_t sw_macsec_port_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*rx_sa_id,
	CA_OUT	ca_macsec_sa_id_t			*tx_sa_id)
{
	return CA_E_OK;
}

ca_status_t sw_macsec_port_mc_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			mc_sa_id)
{
	struct macsec_sa_node *iter;
	bool found = false;

	read_lock(&macsec_sa_list_lock);
	list_for_each_entry(iter, &macsec_sa_list, list) {
		if (iter->sa_id == mc_sa_id) {
			found = true;
			break;
		}
	}
	read_unlock(&macsec_sa_list_lock);

	if (found == false) {
		if (ca_api_debug & API_DBG_MACSEC) {
			ca_printf(KERN_INFO "%s: Can not found SA ID %u.\n", __func__, mc_sa_id);
		}
		return CA_E_PARAM;
	}

	// TODO: validation

	// llid: when port type is not PON, ignore llid.

	// TODO: what to set?

	return CA_E_OK;
}

ca_status_t sw_macsec_port_mc_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*mc_sa_id)
{
	return CA_E_OK;
}

ca_status_t sw_macsec_port_stats_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_stats_t			*counter)
{
	// TODO: get from PE

	return CA_E_OK;
}

ca_status_t sw_macsec_port_stats_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid)
{
	// TODO: clear PE stats through IPC

	return CA_E_OK;
}

ca_status_t sw_macsec_port_error_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_error_t			*counter)
{
	// TODO: get from PE

	return CA_E_OK;
}

ca_status_t sw_macsec_port_error_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid)
{
	// TODO: clear PE stats through IPC

	return CA_E_OK;
}

/***************************************************************************
 * Cortina API - tunnel generic
 ***************************************************************************/

static ca_status_t create_cls_rule_for_tunnel(void)
{
	l3_cls_rule_t rule;
	int ret;
	ca_uint32_t pe_pid = -1;

	/*
	 * CLS rule for downstream packet flow: PE -> WAN port -> L3FE -> LAN port
	 */

	ret = aal_scfg_read(CFG_ID_DOWNSTREAM_PORT_ID, 4, &pe_pid);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to read scfg CFG_ID_DOWNSTREAM_PORT_ID!!!(ret = %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	aal_l3_cls_key_clean(&rule.key);
	memset(&rule.action, 0, sizeof(cls_action_t));

	rule.pri = CL_RUL_PRIO_MAX;
	rule.rslt_type = CL_RSLT_TYPE_0;

	rule.key.msk_o_lspid = 0;
	rule.key.o_lspid = pe_pid;
	rule.key.msk_mac_sa = 0;
	rule.key.mac_sa[5] = 0x00; /* MAC_SA 00:00:00:00:00:02 represents downstream from PE */
	rule.key.mac_sa[4] = 0x00;
	rule.key.mac_sa[3] = 0x00;
	rule.key.mac_sa[2] = 0x00;
	rule.key.mac_sa[1] = 0x00;
	rule.key.mac_sa[0] = 0x02;

#if 0
	/* ASIC bug.
	 * DMALSO can't send HDR_CPU (contains mdata, sw_id).
	 * Even ASIC is fixed through ECO, DMALSO can't send HDR_CPU together with any checksum offload.
	 */
	rule.key.msk_mdata = 0xcfff; /* mdata[55:48] is available */
	rule.key.mdata.mdata_1 = 0xff02ffff; /* mdata[55:48] is 0x02 */
#endif

	rule.action.mcgid = AAL_LPORT_L3_LAN;
	rule.action.mc = 0;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.dpid_pri = 0;
	rule.action.t2_ctrl = l3fe_main_hash_profile_wan;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t3_ctrl = 0;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t4_ctrl = l3fe_hash_lite_profile;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.stage2_ctrl = STAGE2_CTRL_UPDATE;
	rule.action.stage2_ctrl_vld = 1;
	rule.action.stage3_ctrl = STAGE3_CTRL_BYPASS; /* will be overwritten by egress IF */
	rule.action.stage3_ctrl_vld = 1;

	ret = aal_l3_cls_add(0, l3fe_classifier_profile_lan, &rule, &pe_ds_rslt_idx);
	if (ret != AAL_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to create CLS rule for PE downstream. (ret %d)\n", __func__, ret);
		return CA_E_INTERNAL;
	}

	/*
	 * CLS rule for upstream packet flow: PE -> LAN port -> L3FE -> WAN port
	 */

	ret = aal_scfg_read(CFG_ID_UPSTREAM_PORT_ID, 4, &pe_pid);
	if (ret != CA_E_OK) {
		ca_printf("%s: Fail to read scfg CFG_ID_UPSTREAM_PORT_ID!!!(ret = %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	aal_l3_cls_key_clean(&rule.key);
	memset(&rule.action, 0, sizeof(cls_action_t));

	rule.key.msk_o_lspid = 0;
	rule.key.o_lspid = pe_pid;
	rule.key.msk_mac_sa = 0;
	rule.key.mac_sa[5] = 0x00; /* MAC_SA 00:00:00:00:00:01 represents upstream from PE */
	rule.key.mac_sa[4] = 0x00;
	rule.key.mac_sa[3] = 0x00;
	rule.key.mac_sa[2] = 0x00;
	rule.key.mac_sa[1] = 0x00;
	rule.key.mac_sa[0] = 0x01;

#if 0
	/* ASIC bug.
	 * DMALSO can't send HDR_CPU (contains mdata, sw_id).
	 * Even ASIC is fixed through ECO, DMALSO can't send HDR_CPU together with any checksum offload.
	 */
	rule.key.msk_mdata = 0xcfff; /* mdata[55:48] is available */
	rule.key.mdata.mdata_1 = 0xff01ffff; /* mdata[55:48] is 0x01 */
#endif

	rule.action.mcgid = AAL_LPORT_L3_WAN;
	rule.action.mc = 0;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.dpid_pri = 0;
	rule.action.t2_ctrl = T2_CTRL_BYPASS; /* encapsulated tunnel packet doesn't need NAT */
	rule.action.t2_ctrl_vld = 1;
	rule.action.t3_ctrl = 0;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t4_ctrl = l3fe_hash_lite_profile;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	rule.action.stage2_ctrl_vld = 1;
	rule.action.stage3_ctrl = STAGE3_CTRL_BYPASS; /* will be overwritten by egress IF */
	rule.action.stage3_ctrl_vld = 1;

	ret = aal_l3_cls_add(0, l3fe_classifier_profile_lan, &rule, &pe_us_rslt_idx);
	if (ret != AAL_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to create CLS rule for PE upstream. (ret %d)\n", __func__, ret);

		/* delete the created CLS rule */
		aal_l3_cls_delete(0, l3fe_classifier_profile_lan, pe_ds_rslt_idx);

		return CA_E_INTERNAL;
	}

	ca_printf(KERN_INFO "%s: CLS rule for PE downstream is created. (entry_idx %d)\n", __func__, CLS_TBL_ID(pe_ds_rslt_idx));
	ca_printf(KERN_INFO "%s: CLS rule for PE upstream is created. (entry_idx %d)\n", __func__, CLS_TBL_ID(pe_us_rslt_idx));

	return CA_E_OK;
}

ca_status_t ca_tunnel_add (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_cfg_t		*tunnel_cfg,
	CA_OUT		ca_tunnel_id_t		*tunnel_id )
{
	ca_uint32_t i;
	ca_tunnel_id_t tmp_tunnel_id; /* tunnel_id candidate */
	ca_tunnel_id_t tmp2_tunnel_id; /* internal use; for returning existing tunnnel_id */
	ca_status_t ret = CA_E_ERROR;
	bool found = false;

	if ((tunnel_cfg == NULL) || (tunnel_id == NULL))
		return CA_E_PARAM;

	if (!cls_rule_for_tunnel) {
		ret = create_cls_rule_for_tunnel();
		if (ret != CA_E_OK) {
			return ret;
		}

		cls_rule_for_tunnel = true;
	}

	/* search empty slot in g_tunnel[] */
	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (g_tunnel[i] == NULL) {
			/* occupy this index */
			g_tunnel[i] = ca_zalloc_trace(sizeof(ca_tunnel_t), GFP_ATOMIC, true);
			g_tunnel[i]->private_data = NULL;
			g_tunnel[i]->used = FALSE;
			tmp_tunnel_id = i;
			tmp2_tunnel_id = i;
			found = true;
			break;
		}
	}
	if (!found) return CA_E_FULL;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: Dump tunnel_cfg:\n", __func__);
		tunnel_cfg_dump(tunnel_cfg);
	}

	/* do validation and set to HW */
	switch (tunnel_cfg->type) {
	case CA_TUNNEL_PPPOE:
		ret = pppoe_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_L2TP:
		ret = l2tp_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_IPSEC:
		ret = ipsec_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_PPTP:
		ret = pptp_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_4_IN_4:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_DSLITE:
		/* workaround: Pass wanted intf_id to AAL by tunnel_id */
		ret = dslite_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_6RD:
		/* workaround: Pass wanted intf_id to AAL by tunnel_id */
		ret = sixrd_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_6_IN_6:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_MAP_E:
		/* workaround: Pass wanted intf_id to AAL by tunnel_id */
		ret = map_e_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_MAP_T:
		/* workaround: Pass wanted intf_id to AAL by tunnel_id */
		ret = map_t_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	case CA_TUNNEL_MACSEC:
		ret = macsec_tunnel_add(device_id, tunnel_cfg, &tmp2_tunnel_id);
		break;

	default:
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Unknown tunnel type %d.\n", __func__, tunnel_cfg->type);
		}
		return CA_E_PARAM;
	}

	if (ret == CA_E_OK) {
		memcpy(&g_tunnel[tmp_tunnel_id]->tcfg, tunnel_cfg, sizeof(ca_tunnel_cfg_t));
		*tunnel_id = (ca_tunnel_id_t) tmp_tunnel_id;
		g_tunnel[tmp_tunnel_id]->used = TRUE;

		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Tunnel is added, tunnel_id is %u.\n", __func__, *tunnel_id);
		}
	} else {
		/* tunnel already exists, return the existed tunnel_id */
		if (ret == CA_E_EXISTS)
			*tunnel_id = (ca_tunnel_id_t) tmp2_tunnel_id;

		ca_free(g_tunnel[tmp_tunnel_id]);
		g_tunnel[tmp_tunnel_id] = NULL;

		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Tunnel is NOT added. (ret %u)\n", __func__, ret);
		}
	}

	return ret;
}

ca_status_t ca_tunnel_update (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_cfg_t		*tunnel_cfg,
	CA_IN		ca_tunnel_id_t		tunnel_id )
{
	ca_status_t ret = CA_E_ERROR;

	if (tunnel_cfg == NULL) return CA_E_PARAM;
	if (tunnel_id >= CA_TUNNEL_MAX_LIMIT) return CA_E_PARAM;

	if (g_tunnel[tunnel_id] == NULL)
		return CA_E_NOT_FOUND;

	if (g_tunnel[tunnel_id]->tcfg.type != tunnel_cfg->type)
		return CA_E_CONFLICT;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: tunnel_id = %u\n", __func__, tunnel_id);
		ca_printf(KERN_INFO "%s: Dump tunnel_cfg:\n", __func__);
		tunnel_cfg_dump(tunnel_cfg);
	}

	switch (tunnel_cfg->type) {
	case CA_TUNNEL_PPPOE:
		ret = pppoe_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_L2TP:
		ret = l2tp_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_IPSEC:
		ret = ipsec_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_PPTP:
		ret = pptp_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_4_IN_4:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_DSLITE:
		ret = dslite_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_6RD:
		ret = sixrd_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_6_IN_6:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_MAP_E:
		ret = map_e_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_MAP_T:
		ret = map_t_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	case CA_TUNNEL_MACSEC:
		ret = macsec_tunnel_update(device_id, tunnel_cfg, tunnel_id);
		break;

	default:
		return CA_E_PARAM;
	}

	if (ret == CA_E_OK) {
		memcpy(&g_tunnel[tunnel_id]->tcfg, tunnel_cfg, sizeof(ca_tunnel_cfg_t));

		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Tunnel is updated, tunnel_id is %u.\n", __func__, tunnel_id);
		}
	} else {
		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Tunnel is NOT updated. (ret %u)\n", __func__, ret);
		}
	}

	return ret;
}

ca_status_t ca_tunnel_delete (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id )
{
	ca_status_t ret = CA_E_ERROR;

	if (tunnel_id >= CA_TUNNEL_MAX_LIMIT) return CA_E_PARAM;
	if (g_tunnel[tunnel_id] == NULL) return CA_E_NOT_FOUND;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: tunnel_id = %u\n", __func__, tunnel_id);
	}

	g_tunnel[tunnel_id]->used = FALSE;

	switch (g_tunnel[tunnel_id]->tcfg.type) {
	case CA_TUNNEL_PPPOE:
		ret = pppoe_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_L2TP:
		ret = l2tp_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_IPSEC:
		ret = ipsec_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_PPTP:
		ret = pptp_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_4_IN_4:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_DSLITE:
		ret = dslite_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_6RD:
		ret = sixrd_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_6_IN_6:
		return CA_E_NOT_SUPPORT;

	case CA_TUNNEL_MAP_E:
		ret = map_e_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_MAP_T:
		ret = map_t_tunnel_delete(device_id, tunnel_id);
		break;

	case CA_TUNNEL_MACSEC:
		ret = macsec_tunnel_delete(device_id, tunnel_id);
		break;

	default:
		return CA_E_PARAM;
	}

	if (ret == CA_E_OK) {
		ca_free(g_tunnel[tunnel_id]);
		g_tunnel[tunnel_id] = NULL;

		if (ca_api_debug & API_DBG_TUNNEL) {
			ca_printf(KERN_INFO "%s: Tunnel of tunnel_id %u is deleted.\n", __func__, tunnel_id);
		}
	}

	return ret;
}

ca_status_t ca_tunnel_delete_all (
	CA_IN		ca_device_id_t		device_id )
{
	int i;
	ca_status_t ret;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		if (g_tunnel[i] == NULL) continue;

		if ((ret = ca_tunnel_delete(device_id, i)) != CA_E_OK)
			return ret;
	}

	return CA_E_OK;
}

ca_status_t ca_tunnel_get (
	CA_IN		ca_device_id_t		device_id,
	CA_IN		ca_tunnel_id_t		tunnel_id,
	CA_OUT		ca_tunnel_cfg_t		*tunnel_cfg )
{
	if (tunnel_cfg == NULL) return CA_E_PARAM;
	if (tunnel_id >= CA_TUNNEL_MAX_LIMIT) return CA_E_PARAM;
	if (g_tunnel[tunnel_id] == NULL) return CA_E_EMPTY;
	if (g_tunnel[tunnel_id]->used == FALSE) return CA_E_EMPTY;

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: tunnel_id = %u\n", __func__, tunnel_id);
	}

	memcpy(tunnel_cfg, &g_tunnel[tunnel_id]->tcfg, sizeof(ca_tunnel_cfg_t));

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: Dump tunnel_cfg of tunnel_id %u:\n", __func__, tunnel_id);
		tunnel_cfg_dump(tunnel_cfg);
	}

	return CA_E_OK;
}

/* Iterate through all valid tunnel entries.
 * Entries of type ca_tunnel_iterate_entry_t will be returned in return_entry->entry_data.
 * Before returning, API will store the iterator context in return_entry->prev_handle.
 * Caller is not allowed to modify return_entry->prev_handle.
 * When there is no more entries to return, API returns CA_E_UNAVAIL.
 */
ca_status_t ca_tunnel_iterate (
	CA_IN		ca_device_id_t		device_id,
	CA_OUT		ca_iterator_t		*return_entry )
{
	ca_uint32_t idx;
	ca_uint32_t count = 0;
	ca_tunnel_iterate_entry_t *ptr = NULL;
	bool found = false;

	if (return_entry == NULL) return CA_E_PARAM;

	if (return_entry->entry_data == NULL) return CA_E_PARAM;

	if (return_entry->entry_count == 0) return CA_E_OK;

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	if (sizeof(ca_tunnel_iterate_entry_t) * (return_entry->entry_count) > return_entry->user_buffer_size)
		return CA_E_MEMORY;

	/* NULL means starting from first node */
	if (return_entry->prev_handle == NULL) found = true;

	ptr = (ca_tunnel_iterate_entry_t *) return_entry->entry_data;

	for (idx = 0; idx < CA_TUNNEL_MAX_LIMIT; idx++) {
		if (g_tunnel[idx] == NULL) continue;
		if (g_tunnel[idx]->used == FALSE) continue;

		if (found == false) {
			if (g_tunnel[idx] == return_entry->prev_handle) found = true;
			continue;
		}

		memcpy(&ptr->tunnel, &g_tunnel[idx]->tcfg, sizeof(ca_tunnel_cfg_t));
		ptr->tunnel_id = idx;
		ptr++;
		count++;

		if (count >= return_entry->entry_count) break;
	}
	return_entry->entry_count = count;

	if (count == 0) {
		return_entry->prev_handle = NULL;
		return CA_E_UNAVAIL;
	} else {
#ifdef RTK_MEM_CORRUPTTION_FIX
		if (idx == CA_TUNNEL_MAX_LIMIT)
			idx = idx - 1;
#endif
		return_entry->prev_handle = g_tunnel[idx];
		return CA_E_OK;
	}
}

ca_status_t ca_tunnel_init(ca_device_id_t device_id)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
	int i;
	ca_status_t ca_ret;
	aal_ni_hv_glb_internal_port_id_cfg_mask_t port_id_cfg_mask;
	aal_ni_hv_glb_internal_port_id_cfg_t port_id_cfg;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		g_tunnel[i] = NULL;
	}

#ifdef CONFIG_CORTINA_IPC2RCPU
	if ((ca_ret = ca_tunnel_ipc_init(device_id)) != CA_E_OK)
		return ca_ret;
#endif

	/* Send packet to PE through CPU port 0x12.
	 * When sending packet to PE, necessary tunnel info is embedded in CPU header.
	 *
	 * In reg NI_HV_GLB_INTERNAL_PORT_ID_CFG,
	 *   if dma_to_l3fe is set, packet goes to L3FE, and CPU header is kept;
	 *   if dma_to_l3fe is unset, packet goes to L2FE, and CPU header is removed.
	 */
	ca_ret = aal_ni_hv_glb_internal_port_id_cfg_get(device_id, &port_id_cfg);
	if (ca_ret != CA_E_OK) {
		ca_printf(KERN_INFO "%s: ERROR! Fail to get port ID config. (ret %d)\n", __func__, ca_ret);
		return ca_ret;
	}

	port_id_cfg_mask.wrd = 0;
	port_id_cfg_mask.bf.dma_to_l3fe = 1;
	port_id_cfg.dma_to_l3fe |= 1 << (CA_PORT_ID_CPU2 & 0x7);


	ca_ret = aal_ni_hv_glb_internal_port_id_cfg_set(device_id, port_id_cfg_mask, &port_id_cfg);
	if (ca_ret != CA_E_OK) {
		ca_printf(KERN_INFO "%s: ERROR! Fail to set port ID config. (ret %d)\n", __func__, ca_ret);
		return ca_ret;
	}

	ca_printf(KERN_INFO "%s: Tunnel API is initialized.\n", __func__);
#endif
	return CA_E_OK;
}

ca_status_t ca_tunnel_exit(ca_device_id_t device_id)
{
	int i;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {
		/* Should call ca_tunnel_delete_all()? */
		if (g_tunnel[i] != NULL) ca_free(g_tunnel[i]);
		g_tunnel[i] = NULL;
	}

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_tunnel_ipc_exit(device_id);
#endif

	if (cls_rule_for_tunnel) {
		/* CLS rule for downstream packet flow: PE -> WAN port -> L3FE -> LAN port */
		aal_l3_cls_delete(0, l3fe_classifier_profile_lan, pe_ds_rslt_idx);
		/* CLS rule for upstream packet flow: PE -> LAN port -> L3FE -> WAN port */
		aal_l3_cls_delete(0, l3fe_classifier_profile_lan, pe_us_rslt_idx);

		cls_rule_for_tunnel = false;
	}

	return CA_E_OK;
}

EXPORT_SYMBOL(ca_ipsec_init);
EXPORT_SYMBOL(ca_ipsec_shut);
EXPORT_SYMBOL(ca_ipsec_sa_add);
EXPORT_SYMBOL(ca_ipsec_sa_delete);
EXPORT_SYMBOL(ca_ipsec_sa_get);
EXPORT_SYMBOL(ca_ipsec_sa_iterate);
EXPORT_SYMBOL(ca_pptp_sa_add);
EXPORT_SYMBOL(ca_pptp_sa_delete);
EXPORT_SYMBOL(ca_pptp_sa_get);
EXPORT_SYMBOL(ca_pptp_sa_iterate);
EXPORT_SYMBOL(ca_pptp_key_change);
EXPORT_SYMBOL(ca_tunnel_add);
EXPORT_SYMBOL(ca_tunnel_delete);
EXPORT_SYMBOL(ca_tunnel_delete_all);
EXPORT_SYMBOL(ca_tunnel_get);
EXPORT_SYMBOL(ca_tunnel_update);
EXPORT_SYMBOL(ca_tunnel_iterate);
EXPORT_SYMBOL(g_tunnel);
