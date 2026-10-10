#include <linux/list.h>
#include <linux/wait.h>
#include <net/ip.h>
#include <soc/cortina/ca_ipc_pe.h>
#include "ca_types.h"
#include "tunnel.h"
#include "tunnel_ipc.h"
#include "tunnel_ipc_internal.h"
#include "macsec.h"
//#include "route.h"
#include "route_internal.h"
#include "api_common.h"
#include "osal_common.h"
#include "aal_l3_if.h"
#include "ca-scfg/scfg.h"
#include "aal_common.h"

#ifndef CONFIG_CORTINA_IPC2RCPU
#error "ERROR! Kernel option CORTINA_IPC2RCPU is not enabled!"
#else

/* IPC that CPU sends to PE */
typedef enum {
	/*===== Generic =====*/
	CA_IPC_PE_DUMP_TBL = 0,
	CA_IPC_PE_MIB_EN = 1,
	CA_IPC_PE_ENTRY_EN = 2, /* fun_id, entry id */
	CA_IPC_PE_MTU_SET = 3,
	CA_IPC_PE_MTU_GET  = 4,

	/*===== L2TP =====*/
	CA_L2TP_IPC_ENTRY_SET = 5,
	CA_L2TP_IPC_ENTRY_DEL = 6,

	/*===== IPSec =====*/
	CA_IPSEC_IPC_SADB_SET = 7,
	CA_IPSEC_IPC_SADB_KEY_SET = 8,
	CA_IPSEC_IPC_SADB_DEL = 9,

	/*===== MACSec =====*/
	CA_MACSEC_IPC_ENTRY_SET = 10,
	CA_MACSEC_IPC_ENTRY_DEL = 11,

	/*===== MAP-E =====*/
	CA_MAP_E_IPC_ENTRY_SET = 12,
	CA_MAP_E_IPC_ENTRY_DEL = 13,

	/*===== MAP-T =====*/
	CA_MAP_T_IPC_ENTRY_SET = 14,
	CA_MAP_T_IPC_ENTRY_DEL = 15,

#if 0 /* no plan yet */
	/*===== PPTP =====*/
	CA_PPTP_IPC_ENTRY_SET,
	CA_PPTP_IPC_ENTRY_DEL,
	CA_PPTP_IPC_INFORM_LOST_PKT,
	CA_PPTP_IPC_SET_GENERIC_SA_ID_SET,
	CA_PPTP_IPC_KEY_CHANGE_EVENT,

	/*===== RTP proxy =====*/
	CA_RTP_IPC_ENTRY_SET,
	CA_RTP_IPC_ENTRY_DEL,

	/*===== IPv4/IPv6 translation =====*/
	CA_IP_TS_IPC_ENTRY_SET,
	CA_IP_TS_IPC_ENTRY_DEL,
	CA_IP_TS_IPC_PORTSET_SET,

	/*===== setup nexthop said =====*/
	CA_NEXTHOP_IPC_ENTRY_SET,
	CA_NEXTHOP_IPC_ENTRY_DEL,
#endif

} ca_tunnel_ipc_msg_cmd;

/* IPC that PE sends to CPU */
#define CA_IPC_PE_DUMP_TBL_ACK			CA_IPC_PE_DUMP_TBL
#define CA_IPC_PE_MIB_EN_ACK			CA_IPC_PE_ENTRY_EN
#define CA_IPC_PE_MTU_GET_ACK			CA_IPC_PE_MTU_GET
#define CA_L2TP_IPC_ENTRY_SET_ACK		CA_L2TP_IPC_ENTRY_SET
#define CA_L2TP_IPC_ENTRY_DEL_ACK		CA_L2TP_IPC_ENTRY_DEL
#define CA_IPSEC_IPC_SADB_SET_ACK		CA_IPSEC_IPC_SADB_SET
#define CA_IPSEC_IPC_SADB_DEL_ACK		CA_IPSEC_IPC_SADB_DEL
//#define CA_PPTP_IPC_ENTRY_DEL_ACK		CA_PPTP_IPC_ENTRY_DEL
//#define CA_PPTP_IPC_INFORM_LOST_PKT_ACK		CA_PPTP_IPC_INFORM_LOST_PKT
//#define CA_RTP_IPC_ENTRY_SET_ACK		CA_RTP_IPC_ENTRY_SET
//#define CA_RTP_IPC_ENTRY_DEL_ACK		CA_RTP_IPC_ENTRY_DEL
//#define CA_IP_TS_IPC_ENTRY_SET_ACK		CA_IP_TS_IPC_ENTRY_SET
//#define CA_IP_TS_IPC_ENTRY_DEL_ACK		CA_IP_TS_IPC_ENTRY_DEL

extern struct ca_int_l3_intf_s *ca_l3_intf_tbl;
extern struct list_head nexthop_table;
extern ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];

static ca_uint32_t upstream_pe_id = 0;
static ca_uint32_t downstream_pe_id = 1;

/***************************************************************************
 * intenral
 ***************************************************************************/

static ca_uint16_t ca_ipv4_checksum_calc(ca_uint32_t saddr, ca_uint32_t daddr, ca_uint8_t ip_proto)
{
	struct iphdr iph;

	iph.ihl = 5;
	iph.version = 4;
	iph.tos = 0;
	iph.tot_len = 0;
	iph.id = 0;
	iph.frag_off = 0;
	iph.ttl = 64;
	iph.protocol = ip_proto;
	iph.saddr = saddr;
	iph.daddr = daddr;
	ip_send_check(&iph);

	return iph.check;
}

/***************************************************************************
 * IPC functions - Generic
 ***************************************************************************/

ca_status_t ca_tunnel_mtu_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_tunnel_mtu_t	*p_mtu)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_ipc_pe_mtu_set_t msg;
	ca_tunnel_cfg_t tcfg;
	ca_status_t ret;

	if (p_mtu->tunnel_type >= CA_TUNNEL_MAX) {
		return CA_E_ERROR;
	}

	/* fill msg; only set MTU for outbound traffic */
	memset(&msg, 0, sizeof(ca_ipc_pe_mtu_get_t));
	ret = ca_tunnel_get(device_id, p_mtu->tunnel_id, &tcfg);
	if (ret != CA_E_OK) {
		return CA_E_PARAM;
	}
	switch (tcfg.type) {
	case CA_TUNNEL_IPSEC:
		msg.sa_id = tcfg.tunnel.ipsec.sa_id_encrypt;
		break;
	case CA_TUNNEL_PPTP:
		msg.sa_id = tcfg.tunnel.pptp.sa_id_outbound;
		break;
	default:
		return CA_E_NOT_SUPPORT;
	}

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_IPC_PE_MTU_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_async_send(&ipc_pkt);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_tunnel_mtu_set);

ca_status_t ca_tunnel_mtu_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_tunnel_mtu_t	*p_mtu)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_ipc_pe_mtu_get_t msg;
	ca_ipc_pe_mtu_get_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_tunnel_cfg_t tcfg;
	ca_status_t ret;

	if (p_mtu->tunnel_type >= CA_TUNNEL_MAX) {
		return CA_E_PARAM;
	}

	/* fill msg; only set MTU for outbound traffic */
	memset(&msg, 0, sizeof(ca_ipc_pe_mtu_get_t));
	ret = ca_tunnel_get(device_id, p_mtu->tunnel_id, &tcfg);
	if (ret != CA_E_OK) {
		return CA_E_PARAM;
	}
	switch (tcfg.type) {
	case CA_TUNNEL_IPSEC:
		msg.sa_id = tcfg.tunnel.ipsec.sa_id_encrypt;
		break;
	case CA_TUNNEL_PPTP:
		msg.sa_id = tcfg.tunnel.pptp.sa_id_outbound;
		break;
	default:
		return CA_E_NOT_SUPPORT;
	}

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_IPC_PE_MTU_GET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	p_mtu->mtu = ack.mtu;

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_tunnel_mtu_get);

static int ca_tunnel_ipc_debug_handler(ca_ipc_addr_t peer, ca_uint16_t msg_no, ca_uint16_t trans_id, const void *msg_data, ca_uint16_t *msg_size)
{
	ca_uint16_t i;
	const char *data = msg_data;

	ca_printf(KERN_INFO "msg_no %d trans_id %d msg_size %d\n", msg_no, trans_id, *msg_size);
	ca_printf(KERN_INFO "session_id %d cpu_id %d\n\t", peer.session_id, peer.cpu_id);

	for (i = 0; i < *msg_size; i++) {
		ca_printf("%02X ", data[i]);
		if ((i % 32) == 31)
			ca_printf("\n\t");
	}
	ca_printf("\n");
	return CA_E_OK;
}

/***************************************************************************
 * IPC functions - L2TP
 ***************************************************************************/

ca_status_t ca_l2tp_ipc_entry_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_id_t		tunnel_id,
	CA_IN	ca_uint8_t		ip_proto, /* protocol in IP header of tunnel packet */
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_l2tp_ipc_entry_set_t msg;
	ca_l2tp_ipc_entry_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_tunnel_cfg_t tcfg;
	ca_status_t ret;

	ret = ca_tunnel_get(device_id, tunnel_id, &tcfg);
	if (ret != CA_E_OK) {
		return ret;
	}
	if (tcfg.type != CA_TUNNEL_L2TP)
		return CA_E_PARAM;

	// TODO: validation

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.l2tp_idx4pe = idx4pe;
	msg.dir = dir;
	msg.l2tp = tcfg.tunnel.l2tp;
	msg.ip_ver = tcfg.src_addr.afi;
	msg.ip_checksum = 0;
	msg.tunnel_saddr.addr[0] = htonl(tcfg.src_addr.ip_addr.addr[0]);
	msg.tunnel_saddr.addr[1] = htonl(tcfg.src_addr.ip_addr.addr[1]);
	msg.tunnel_saddr.addr[2] = htonl(tcfg.src_addr.ip_addr.addr[2]);
	msg.tunnel_saddr.addr[3] = htonl(tcfg.src_addr.ip_addr.addr[3]);
	msg.tunnel_daddr.addr[0] = htonl(tcfg.dest_addr.ip_addr.addr[0]);
	msg.tunnel_daddr.addr[1] = htonl(tcfg.dest_addr.ip_addr.addr[1]);
	msg.tunnel_daddr.addr[2] = htonl(tcfg.dest_addr.ip_addr.addr[2]);
	msg.tunnel_daddr.addr[3] = htonl(tcfg.dest_addr.ip_addr.addr[3]);

	if (msg.ip_ver == CA_IPV4)
		msg.ip_checksum = ca_ipv4_checksum_calc(msg.tunnel_saddr.ipv4_addr, msg.tunnel_daddr.ipv4_addr, ip_proto);

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_L2TP_IPC_ENTRY_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: L2TP entry (tunnel_id %u, dir %u, idx4pe %u) is set. PE returns %u.\n",
				__func__, tunnel_id, dir, idx4pe, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l2tp_ipc_entry_set);

ca_status_t ca_l2tp_ipc_entry_del(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_l2tp_ipc_entry_del_t msg;
	ca_l2tp_ipc_entry_del_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	// TODO: validation

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.l2tp_idx4pe = idx4pe;
	msg.dir = dir;

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_L2TP_IPC_ENTRY_DEL;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: L2TP entry (idx4pe %u, dir %d) is deleted. PE returns offload_seq %u.\n", __func__, idx4pe, dir, ack.offload_seq);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l2tp_ipc_entry_del);

/***************************************************************************
 * IPC functions - IPSec
 ***************************************************************************/

ca_status_t ca_ipsec_ipc_sadb_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_cfg_t		*tunnel_cfg,
	CA_IN	ca_ipsec_sa_t		*ipsec_sa,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_ipsec_ipc_sadb_set_t msg;
	ca_ipsec_ipc_sadb_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;
	ca_uint8_t ip_proto;

	if ((tunnel_cfg == NULL) || (ipsec_sa == NULL))
		return CA_E_PARAM;

	if (tunnel_cfg->type != CA_TUNNEL_IPSEC) {
		return CA_E_PARAM;
	}

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.ipsec_idx4pe = idx4pe;
	msg.replay_window = ipsec_sa->replay_window;
	msg.spi = htonl(ipsec_sa->spi); /* though user inputs spi in network order, but actual order in memory depends on chip, so force to change order. */
	msg.seq_num = ipsec_sa->sequence_number;
	msg.ip_ver = ipsec_sa->ip_version;
	msg.proto = ipsec_sa->protocol;
	msg.tunnel = ipsec_sa->tunnel;
	msg.ealg = ipsec_sa->ealg;
	msg.ealg_mode = ipsec_sa->ealg_mode;
	msg.enc_keylen = ipsec_sa->encryption_keylen;
	msg.iv_len = ipsec_sa->iv_len;
	msg.aalg = ipsec_sa->aalg;
	msg.auth_keylen = ipsec_sa->auth_keylen;
	msg.icv_trunclen = ipsec_sa->icv_trunclen;
	msg.etherIP = ipsec_sa->etherIP;
	msg.is_natt = ipsec_sa->is_natt;
	msg.src_l4_port = ipsec_sa->src_l4_port;
	msg.dest_l4_port = ipsec_sa->dest_l4_port;
	msg.dir = ipsec_sa->sa_dir;
	msg.tunnel_saddr.addr[0] = htonl(tunnel_cfg->src_addr.ip_addr.addr[0]);
	msg.tunnel_saddr.addr[1] = htonl(tunnel_cfg->src_addr.ip_addr.addr[1]);
	msg.tunnel_saddr.addr[2] = htonl(tunnel_cfg->src_addr.ip_addr.addr[2]);
	msg.tunnel_saddr.addr[3] = htonl(tunnel_cfg->src_addr.ip_addr.addr[3]);
	msg.tunnel_daddr.addr[0] = htonl(tunnel_cfg->dest_addr.ip_addr.addr[0]);
	msg.tunnel_daddr.addr[1] = htonl(tunnel_cfg->dest_addr.ip_addr.addr[1]);
	msg.tunnel_daddr.addr[2] = htonl(tunnel_cfg->dest_addr.ip_addr.addr[2]);
	msg.tunnel_daddr.addr[3] = htonl(tunnel_cfg->dest_addr.ip_addr.addr[3]);
	// msg.copy_ip_id = ?
	// msg.copy_tos = ?
	// msg.default_tos = ?

	if (msg.ip_ver == CA_IPV4) {
		if (msg.is_natt != 0)
			ip_proto = IPPROTO_UDP;
		else
			ip_proto = msg.proto ? IPPROTO_AH : IPPROTO_ESP;

		msg.ip_checksum = ca_ipv4_checksum_calc(msg.tunnel_saddr.ipv4_addr, msg.tunnel_daddr.ipv4_addr, ip_proto);
	}

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_IPSEC_SPD_ENCRYPT) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_IPSEC_IPC_SADB_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (dst_cpu_id %u, ret %d)\n", __func__, ipc_pkt.dst_cpu_id, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SADB of (idx4pe %u, dir %d) is set. PE returns (idx4pe %u, dir %d, ret %u).\n",
				__func__, idx4pe, dir, ack.ipsec_idx4pe, ack.dir, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_ipsec_ipc_sadb_set);

ca_status_t ca_ipsec_ipc_sadb_key_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_cfg_t		*tunnel_cfg,
	CA_IN	ca_ipsec_sa_t		*ipsec_sa,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_ipsec_ipc_sadb_key_set_t msg;
	ca_ipsec_ipc_sadb_key_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	if ((tunnel_cfg == NULL) || (ipsec_sa == NULL))
		return CA_E_PARAM;

	if (tunnel_cfg->type != CA_TUNNEL_IPSEC) {
		return CA_E_PARAM;
	}

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.ipsec_idx4pe = idx4pe;
	msg.dir = dir;
	memcpy(msg.akey, ipsec_sa->akey, sizeof(msg.akey));
	memcpy(msg.ekey, ipsec_sa->ekey, sizeof(msg.ekey));

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_IPSEC_SPD_ENCRYPT) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_IPSEC_IPC_SADB_KEY_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (dst_cpu_id %u, ret %d)\n", __func__, ipc_pkt.dst_cpu_id, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SADB key of (idx4pe %u, dir %d) is set. PE returns (idx4pe %u, dir %d, ret %u).\n",
				__func__, idx4pe, dir, ack.ipsec_idx4pe, ack.dir, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_ipsec_ipc_sadb_key_set);

ca_status_t ca_ipsec_ipc_sadb_del(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_ipsec_ipc_sadb_del_t msg;
	ca_ipsec_ipc_sadb_del_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	/* fill msg */
	msg.ipsec_idx4pe = idx4pe;
	msg.dir = dir;

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_IPSEC_SPD_ENCRYPT) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_IPSEC_IPC_SADB_DEL;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (dst_cpu_id %u, ret %d)\n", __func__, ipc_pkt.dst_cpu_id, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: IPSec SADB of (idx4pe %u, dir %d) is deleted. PE returns (idx4pe %u, dir %d, offload_seq %u).\n",
				__func__, idx4pe, dir, ack.ipsec_idx4pe, ack.dir, ack.offload_seq);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_ipsec_ipc_sadb_del);

/***************************************************************************
 * IPC functions - PPTP
 ***************************************************************************/


/***************************************************************************
 * IPC functions - RTP proxy
 ***************************************************************************/


/***************************************************************************
 * IPC functions - IPv4/IPv6 translation
 ***************************************************************************/


/***************************************************************************
 * IPC functions - MAP-E
 ***************************************************************************/

ca_status_t ca_map_e_ipc_entry_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_id_t		tunnel_id,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_map_e_ipc_entry_set_t msg;
	ca_map_e_ipc_entry_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_tunnel_cfg_t tcfg;
	ca_status_t ret;
	int i;
	bool found_intf = false, found_nexthop = false;
	struct nexthop_node *iter_nh = NULL;
	ca_intf_id_t intf_id;
	ca_l3_nexthop_id_t nexthop_id;

	ret = ca_tunnel_get(device_id, tunnel_id, &tcfg);
	if (ret != CA_E_OK) {
		return ret;
	}
	if (tcfg.type != CA_TUNNEL_MAP_E)
		return CA_E_PARAM;

	// TODO: validation

	/* search intf that uses tunnel_id */
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

	/* search nexthop that uses intf_id */
	if (found_intf) {
		list_for_each_entry(iter_nh, &nexthop_table, list) {
			if (iter_nh->nexthop.intf_id == intf_id) {
				found_nexthop = true;
				nexthop_id = iter_nh->nexthop.nexthop_id;
				break;
			}
		}
	}

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.map_e_idx4pe = idx4pe;
	msg.dir = dir;
	msg.remote_ipv6 = tcfg.dest_addr;
	msg.local_ipv6 = tcfg.src_addr;
	msg.copy_tos = (tcfg.tunnel.map.egress_tc_copied_from_ipv4_tos) ? 1 : 0;
	msg.default_tos = tcfg.tunnel.map.egress_tc_value;
	msg.hop_limit = tcfg.tunnel.map.egress_hoplimit_value;
	msg.port_set_id = tcfg.tunnel.map.psid_id;
	msg.port_set_id_length = tcfg.tunnel.map.psid_length;
	msg.port_set_id_offset = tcfg.tunnel.map.psid_offset;
#if 0
	msg.v4id_base = 0;
	msg.v4id_mask = 0xffff;
#endif

	if (found_intf)
		memcpy(msg.local_mac, ca_l3_intf_tbl[intf_id].intf.mac_addr, 6);

	if (found_nexthop)
		memcpy(msg.remote_mac, iter_nh->nexthop.da_mac, 6);

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MAP_E_IPC_ENTRY_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MAP-E entry (tunnel_id %u, dir %u, idx4pe %u) is set. PE returns %u.\n",
				__func__, tunnel_id, dir, idx4pe, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_map_e_ipc_entry_set);

ca_status_t ca_map_e_ipc_entry_del(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_map_e_ipc_entry_del_t msg;
	ca_map_e_ipc_entry_del_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	// TODO: validation

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.map_e_idx4pe = idx4pe;
	msg.dir = dir;

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MAP_E_IPC_ENTRY_DEL;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MAP-E entry (idx4pe %u, dir %d) is deleted.\n", __func__, idx4pe, dir);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_map_e_ipc_entry_del);

/***************************************************************************
 * IPC functions - MAP-T
 ***************************************************************************/

ca_status_t ca_map_t_ipc_entry_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_cfg_t		*tcfg,
	CA_IN	map_private_data_t	*priv,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_map_t_ipc_entry_set_t msg;
	ca_map_t_ipc_entry_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	if (tcfg->type != CA_TUNNEL_MAP_T)
		return CA_E_PARAM;

	// TODO: validation

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.map_t_idx4pe = idx4pe;
	msg.dir = dir;

	msg.remote_ip = tcfg->dest_addr;
	msg.remote_ip.ip_addr.ipv6_addr[0] = htonl(msg.remote_ip.ip_addr.ipv6_addr[0]);
	msg.remote_ip.ip_addr.ipv6_addr[1] = htonl(msg.remote_ip.ip_addr.ipv6_addr[1]);
	msg.remote_ip.ip_addr.ipv6_addr[2] = htonl(msg.remote_ip.ip_addr.ipv6_addr[2]);
	msg.remote_ip.ip_addr.ipv6_addr[3] = htonl(msg.remote_ip.ip_addr.ipv6_addr[3]);

	if (dir == CA_TUNNEL_DIR_OUTBOUND) {
		msg.local_ip = tcfg->src_addr;
		msg.local_ip.ip_addr.ipv6_addr[0] = htonl(msg.local_ip.ip_addr.ipv6_addr[0]);
		msg.local_ip.ip_addr.ipv6_addr[1] = htonl(msg.local_ip.ip_addr.ipv6_addr[1]);
		msg.local_ip.ip_addr.ipv6_addr[2] = htonl(msg.local_ip.ip_addr.ipv6_addr[2]);
		msg.local_ip.ip_addr.ipv6_addr[3] = htonl(msg.local_ip.ip_addr.ipv6_addr[3]);
	} else {
		msg.local_ip = priv->ce_ipv4;
		msg.local_ip.ip_addr.ipv4_addr = htonl(msg.local_ip.ip_addr.ipv4_addr);
	}

	msg.copy_tos = (tcfg->tunnel.map.egress_tc_copied_from_ipv4_tos) ? 1 : 0;
	msg.default_tos = tcfg->tunnel.map.egress_tc_value;
	msg.hop_limit = tcfg->tunnel.map.egress_hoplimit_value;
	msg.port_set_id = tcfg->tunnel.map.psid_id;
	msg.port_set_id_length = tcfg->tunnel.map.psid_length;
	msg.port_set_id_offset = tcfg->tunnel.map.psid_offset;
#if 0
	msg.v4id_base = 0;
	msg.v4id_mask = 0xffff;
#endif

	/* TODO: Workaround. Should not use pre-defined MAC */
	if (dir == CA_TUNNEL_DIR_OUTBOUND) {
		msg.local_mac[0] = 0x00;
		msg.local_mac[1] = 0x00;
		msg.local_mac[2] = 0x00;
		msg.local_mac[3] = 0x00;
		msg.local_mac[4] = 0x00;
		msg.local_mac[5] = 0x01;
		msg.remote_mac[0] = 0x00;
		msg.remote_mac[1] = 0x13;
		msg.remote_mac[2] = 0x25;
		msg.remote_mac[3] = 0x00;
		msg.remote_mac[4] = 0x00;
		msg.remote_mac[5] = 0x02;
	} else {
		msg.local_mac[0] = 0x00;
		msg.local_mac[1] = 0x00;
		msg.local_mac[2] = 0x00;
		msg.local_mac[3] = 0x00;
		msg.local_mac[4] = 0x00;
		msg.local_mac[5] = 0x02;
		msg.remote_mac[0] = 0x00;
		msg.remote_mac[1] = 0x13;
		msg.remote_mac[2] = 0x25;
		msg.remote_mac[3] = 0x00;
		msg.remote_mac[4] = 0x00;
		msg.remote_mac[5] = 0x01;
	}

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MAP_T_IPC_ENTRY_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (dir %d, dst_cpu_id %u, ret %d)\n", __func__, dir, ipc_pkt.dst_cpu_id, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MAP-T entry (dir %u, idx4pe %u) is set. PE returns %u.\n",
				__func__, dir, idx4pe, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_map_t_ipc_entry_set);

ca_status_t ca_map_t_ipc_entry_del(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_map_t_ipc_entry_del_t msg;
	ca_map_t_ipc_entry_del_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	// TODO: validation

	/* fill msg */
	memset(&msg, 0, sizeof(msg));
	msg.map_t_idx4pe = idx4pe;
	msg.dir = dir;

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MAP_T_IPC_ENTRY_DEL;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MAP-T entry (idx4pe %u, dir %d) is deleted.\n", __func__, idx4pe, dir);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_map_t_ipc_entry_del);

/***************************************************************************
 * IPC functions - MACSec
 ***************************************************************************/

ca_status_t ca_macsec_ipc_sadb_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_tunnel_cfg_t		*tunnel_cfg,
	CA_IN	ca_macsec_sa_t		*macsec_sa,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_macsec_ipc_sadb_set_t msg;
	ca_macsec_ipc_sadb_set_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;
	ca_uint8_t *cp;
	int i;

	if ((tunnel_cfg == NULL) || (macsec_sa == NULL))
		return CA_E_PARAM;

	if (tunnel_cfg->type != CA_TUNNEL_MACSEC) {
		return CA_E_PARAM;
	}

	/* fill msg */

	msg.macsec_idx4pe = idx4pe;
	msg.dir = macsec_sa->direction;

	///TODO: debug
	/* how to decide key length??? */
	if (macsec_sa->sa_db.sa_db_control_flags.association_number == 0) {
		if (macsec_sa->sa_db.mask.key0_valid) {
			memcpy(msg.ekey, macsec_sa->sa_db.key0, MACSEC_KEY_LENGTH);
			msg.enc_keylen = MACSEC_KEY_LENGTH / 4;
		}
	}

	if (macsec_sa->sa_db.sa_db_control_flags.association_number == 1) {
		if (macsec_sa->sa_db.mask.key1_valid) {
			memcpy(msg.ekey, macsec_sa->sa_db.key1, MACSEC_KEY_LENGTH);
			msg.enc_keylen = MACSEC_KEY_LENGTH / 4;
		}
	}

	if (macsec_sa->sa_db.mask.lapn_valid) {
		msg.packet_number = macsec_sa->sa_db.lapn;

	}

	if (macsec_sa->sa_db.mask.sa_db_control_flags_valid) {
		msg.tci_an |= macsec_sa->sa_db.sa_db_control_flags.end_station << 6;
		msg.tci_an |= macsec_sa->sa_db.sa_db_control_flags.sci_encoded << 5;
		msg.tci_an |= macsec_sa->sa_db.sa_db_control_flags.single_copy_broadcast << 4;
		msg.tci_an |= 1 << 3; /* Encryption bit */
		msg.tci_an |= macsec_sa->sa_db.sa_db_control_flags.association_number;
	}

	cp = (ca_uint8_t *) &macsec_sa->sa_db.sci;
	for (i = 0; i < 8; i++) {
		/* macsec_sa->sa_db.sci is host order, change it into network order */
		msg.secure_channel_id[i] = *(cp + 7 - i);
	}

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MACSEC_IPC_ENTRY_SET;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MACSec SADB of (idx4pe %u, dir %d) is set. PE returns (idx4pe %u, dir %d, ret %u).\n",
				__func__, idx4pe, dir, ack.macsec_idx4pe, ack.dir, ack.ret_value);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_macsec_ipc_sadb_set);

ca_status_t ca_macsec_ipc_sadb_del(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tunnel_direction_t	dir,
	CA_IN	ca_uint32_t		idx4pe)
{
	ca_ipc_pkt_t ipc_pkt;
	ca_macsec_ipc_sadb_del_t msg;
	ca_macsec_ipc_sadb_del_ack_t ack;
	ca_uint16 ack_len = sizeof(ack);
	ca_status_t ret;

	/* fill msg */
	msg.macsec_idx4pe = idx4pe;
	msg.dir = dir;

	/* fill ipc_pkt */
	ipc_pkt.session_id = CA_IPC_SESSION_VPN;
	ipc_pkt.dst_cpu_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? upstream_pe_id : downstream_pe_id;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_MACSEC_IPC_ENTRY_DEL;
	ipc_pkt.msg_data = &msg;
	ipc_pkt.msg_size = sizeof(msg);

	ret = ca_ipc_msg_sync_send(&ipc_pkt, &ack, &ack_len);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to send IPC message. (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_api_debug & API_DBG_TUNNEL) {
		ca_printf(KERN_INFO "%s: MACSec SADB of (idx4pe %u, dir %d) is deleted. PE returns (idx4pe %u, dir %d).\n",
				__func__, idx4pe, dir, ack.macsec_idx4pe, ack.dir);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_macsec_ipc_sadb_del);

/***************************************************************************
 * IPC initialization
 ***************************************************************************/

static ca_ipc_msg_handle_t ca_pe_ipc_procs[] = {
	/* Generic */
	{ .msg_no = CA_IPC_PE_MTU_GET_ACK, .proc = ca_tunnel_ipc_debug_handler},
#if 0
	/* L2TP */
	{ .msg_no = CA_L2TP_IPC_PE_SET_ENTRY_ACK, .proc = ca_l2tp_ipc_rcv_set_entry_ack},
	{ .msg_no = CA_L2TP_IPC_PE_DEL_ENTRY_ACK, .proc = ca_l2tp_ipc_rcv_del_entry_ack},
	/* IPSec */
	{ .msg_no = CA_IPSEC_IPC_PE_SET_SADB_ACK, .proc = ca_ipsec_ipc_rcv_set_sadb_ack},
	{ .msg_no = CA_IPSEC_IPC_PE_DEL_SADB_ACK, .proc = ca_ipsec_ipc_rcv_del_sadb_ack},
	{ .msg_no = CA_IPSEC_IPC_PE_SET_SADB_KEY_ACK, .proc = ca_ipsec_ipc_rcv_set_sadb_key_ack},
	/* PPTP */
	{ .msg_no = CA_PPTP_IPC_PE_DEL_ENTRY_ACK, .proc = ca_pptp_ipc_del_entry_ack},
	{ .msg_no = CA_PPTP_IPC_PE_INFORM_LOST_PKT_ACK, .proc = ca_pptp_ipc_inform_lost_pkt_ack},
	/* RTP proxy */
	{ .msg_no = CA_RTP_IPC_PE_SET_ENTRY_ACK, .proc = ca_rtp_ipc_rcv_set_entry_ack},
	{ .msg_no = CA_RTP_IPC_PE_DEL_ENTRY_ACK, .proc = ca_rtp_ipc_rcv_del_entry_ack},
	/* IPv4/IPv6 translation */
	{ .msg_no = CA_IP_TS_IPC_PE_SET_ENTRY_ACK, .proc = ca_ip_translate_ipc_rcv_set_entry_ack},
	{ .msg_no = CA_IP_TS_IPC_PE_DEL_ENTRY_ACK, .proc = ca_ip_translate_ipc_rcv_del_entry_ack},
#endif
};

ca_status_t ca_tunnel_ipc_init(ca_device_id_t device_id)
{
	int ret;
	ca_uint16_t msg_count;

	ret = aal_scfg_read(CFG_ID_UPSTREAM_PE_ID, 4, (ca_uint32_t *) &upstream_pe_id);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to read scfg CFG_ID_UPSTREAM_PE_ID! (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	ret = aal_scfg_read(CFG_ID_DOWNSTREAM_PE_ID, 4, (ca_uint32_t *) &downstream_pe_id);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to read scfg CFG_ID_DOWNSTREAM_PE_ID! (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	msg_count = sizeof(ca_pe_ipc_procs) / sizeof(ca_pe_ipc_procs[0]);
	ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_VPN, ca_pe_ipc_procs, msg_count);
	if (ret != CA_E_OK) {
		ca_printf(KERN_ERR "%s: Fail to register IPC for PE.\n", __func__);
		return ret;
	}

	ca_printf(KERN_INFO "%s: Register IPC for PE.\n", __func__);

	return CA_E_OK;
}

ca_status_t ca_tunnel_ipc_exit(ca_device_id_t device_id)
{
	ca_ipc_msg_handle_unregister(CA_IPC_SESSION_VPN);
	ca_printf(KERN_INFO "%s: Unregister IPC for PE.\n", __func__);

	return CA_E_OK;
}

#endif /* CONFIG_CORTINA_IPC2RCPU */
