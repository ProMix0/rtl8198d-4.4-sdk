#include <generated/ca_ne_autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/ipv6.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/icmpv6.h>
#include <net/ipv6.h>

#include "rx.h"
#include "route.h"
#include "ca_ni.h"
#include "ca_ni_proc.h"
#include "ca_ni_dump.h"
#include "ca_mut.h"
#include "aal_l2_specpkt.h"
#include "aal_fdb.h"

#include "scfg.h"

#ifdef CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN
#include "ca_ni_titan.h"
#else
#include "ca_ni_rx.h"
#endif

#ifdef CA_NI_NETLINK_MIRROR_TEST
#include "aal_mir.h"
#endif

#define IPV6_IS_EXTERN_HEADER(nexthdr) (((nexthdr) == IPPROTO_HOPOPTS)||((nexthdr) == IPPROTO_ROUTING)|| ((nexthdr) == IPPROTO_FRAGMENT)|| ((nexthdr) == IPPROTO_DSTOPTS)|| ((nexthdr) == IPPROTO_MH))

#define NLMSG_PUT(skb, portid, seq, type, len) __nlmsg_put(skb, portid, seq, type, len, 0)

extern u32 ca_ni_debug;

typedef struct ca_ni_netlink_cb_list_s{
	pid_t		     pid;
	ca_int_t	     seq;
	ca_uint8_t     	     priority;
	ca_pkt_type_t        type;
	struct ca_ni_netlink_cb_list_s *p_next;
}ca_ni_netlink_cb_list_t;

ca_ni_netlink_cb_list_t *ca_ni_netlink_cb_high_head = NULL;
ca_ni_netlink_cb_list_t *ca_ni_netlink_cb_low_head = NULL;

typedef struct ca_ni_netlink_pid_seq_list_s {
	pid_t  pid;
	ca_int_t  seq;
	struct ca_ni_netlink_pid_seq_list_s *p_next;
} ca_ni_netlink_pid_seq_list_t;

ca_ni_netlink_pid_seq_list_t *ca_ni_netlink_pid_seq_head = NULL;

ca_boolean_t ca_api_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
void ca_api_ip_address_ntohl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src);
ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data);

spinlock_t ca_ni_netlink_cb_lock;
spinlock_t ca_ni_netlink_pid_seq_lock;

extern void ca_ni_rx_fill_pkt(int cpu_port, ca_pkt_t *pkt, HEADER_A_T *hdr_a, HEADER_CPU_T *hdr_cpu);

extern ca_uint8_t pon_mac_mode;

static ca_status_t __ca_rx_unregister_pkt(pid_t pid, ca_uint8_t priority, ca_pkt_type_t type);
static void __ca_rx_unregister_pkt_all(pid_t pid);

/* global variables and functions for Netlink RX queue sync function */
static DECLARE_WAIT_QUEUE_HEAD(netlink_ack_wait_queue);

#define NETLINK_RX_QUEUE_NUM    20480

typedef struct ca_ni_netlink_payload_s {
        pid_t                pid;
        ca_int_t             seq;
        ca_pkt_t             pkt;
        ca_pkt_block_t       pkt_blk;
       struct sk_buff      *skb;
} ca_ni_netlink_payload_t;

static int nextlink_rx_ack = 0;

static ca_ni_netlink_payload_t *netlink_rx_queue = NULL;
static ca_pkt_t *netlink_rx_pkt = NULL;

static int netlink_rx_queue_rd_ptr = 0, netlink_rx_queue_wr_ptr = 0;
static int netlink_rxq_put_count = 0;
static int netlink_rxq_get_count = 0;
static int netlink_sync_ack_lost_count = 0;
static int netlink_unicast_fail_count = 0;
static int netlink_unicast_send_count = 0;
static int netlink_rx_queue_full_count = 0;
static spinlock_t ca_ni_netlink_rx_queue_lock;

unsigned long netlink_rx_free_skb_count = 0;
int netlink_rx_queue_is_full = 0;
ca_uint8_t netlink_rx_ack = 0;

static char ca_ni_netlink_pkt_type_str[CA_PKT_TYPE_MAX][16+1]=
{
	"BPDU", "DOT1X", "IGMP", "ARP", "OAM", "OMCI", "PLOAM", "MPCP", "DHCP", "DHCPV6", "PPPOE_DIS", "PPPOE_SES",
	"IROS_HELLO", "ICMP", "ICMPV6_MLD", "ICMPV6_NDP", "CFM", "L2_PTP", "L4_PTP", "HTTP_GET", "HTTP_PUT",
	"HTTP_HEAD", "HTTP_POST", "HTTP_OPTIONS", "HTTP_DELETE", "HTTP_TRACE", "HTTP_CONNECT", "HTTP_UNLINK",
	"HTTP_PATCH", "DNS", "RIP", "SSDP", "CONTROL ALL", "MIRROR", "LOOPBACK", "1AS", "MYMAC", "SNAP", "LLC",
	"USER DEFINED", "OTHERS"
};

static ca_status_t ca_ni_netlink_rx_thread_create(void);
static ca_status_t ca_ni_netlink_rx_queue_put(pid_t pid, ca_int_t seq, ca_pkt_t *pkt, struct sk_buff *skb);

void ca_ni_rx_append_ptp_data(char *data, ca_uint32_t *pkt_size, ca_uint64_t nsec, ca_boolean_t dump);

ca_status_t ca_ni_netlink_icmpv6_checksum(struct sk_buff *skb, char *skb_data)
{
	int i;
	struct ipv6hdr *iph6;
	ca_uint8_t ipv6_nexthdr;
	struct in6_addr *src, *dst;
	ca_uint16_t *data;
	size_t len;
	ca_uint32_t checksum = 0;
	ca_uint32_t offset = sizeof(struct ipv6hdr);

	iph6 = ipv6_hdr(skb);
	src = &(iph6->saddr);
	dst = &(iph6->daddr);
	len = ntohs(iph6->payload_len);

	/* skip ipv6 nexthdr */
	ipv6_nexthdr = iph6->nexthdr;
	ca_ni_parse_ipv6(skb_data, (uint8_t **) &data, (ca_ni_debug & NI_DBG_NETLINK) ? true : false, NULL);
	offset = (char *)data - skb_data;
	data = (ca_uint16_t *)((char *)skb_data + offset);

	/* IPv6 Pseudo header source address, destination address, length, zeros, next header */
	for (i = 0; i < 8; i++) {
		checksum += ntohs(src->s6_addr16[i]);
	}

	for (i = 0; i < 8; i++) {
		checksum += ntohs(dst->s6_addr16[i]);
	}

	len -= (offset - sizeof(struct ipv6hdr));
	checksum += len;
	checksum += 58;   /* ICMPv6 next header*/

	/* message data from type/code/checksum */
	while (len > 1) {
		checksum += ntohs(*data);
		data++;
		len -= 2;
	}

	if (len > 0) {
		/* should shift to high byte */
		checksum += (ca_uint16_t)*((uint8_t *)data) << 8;
	}

	checksum += checksum >> 16;

	if ((uint16_t)~checksum == 0) {
		return CA_E_OK;
	}
	else {
		return CA_E_ERROR;
	}
}

/* get the interface id of incoming packet for Cortina API multicast usage */
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
static ca_status_t ca_ni_netlink_get_intf_id(ca_uint8_t *data, ca_intf_id_t *intf_id)
{
	int i, ret;
	ca_uint8_t *cp;
	ca_uint16_t ethertype;
	ca_l3_intf_t  l3_intf;
	ca_iterator_t   iterator;
	ca_ip_address_t src_ip_addr;
	struct iphdr *iph;
	struct ipv6hdr *ipv6h;

	ethertype = ca_ni_parse_l2(data, NULL, NULL, NULL, &cp, (ca_ni_debug & NI_DBG_NETLINK) ? true : false);

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("EtherType: %04x\n", htons(ethertype));
	}

	if (ethertype == ntohs(ETH_P_IP)) {
		iph = (struct iphdr *) cp;

		if (ca_ni_debug & NI_DBG_NETLINK) {

			printk("IPv4 HDR:\n");
			printk("\t saddr %pI4\n", &iph->saddr);
			printk("\t daddr %pI4\n", &iph->daddr);
		}

		src_ip_addr.afi = CA_IPV4;
		src_ip_addr.ip_addr.ipv4_addr = iph->saddr;
	}
	if (ethertype == ntohs(ETH_P_IPV6)) {
		ipv6h = (struct ipv6hdr *) cp;

		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("IPv6 HDR:\n");
			printk("\t saddr %pI6\n", &ipv6h->saddr.s6_addr);
			printk("\t daddr %pI6\n", &ipv6h->daddr.s6_addr);
		}

		src_ip_addr.afi = CA_IPV6;
		memcpy(src_ip_addr.ip_addr.ipv6_addr, ipv6h->saddr.s6_addr, sizeof(src_ip_addr.ip_addr.ipv6_addr));
	}
	/* convert to host order because Cortina API use host order */
	ca_api_ip_address_ntohl(&src_ip_addr, &src_ip_addr);

	i = 0;
	iterator.prev_handle = NULL;
	iterator.entry_count = 1;
	iterator.entry_data = &l3_intf;
	iterator.user_buffer_size = sizeof(ca_l3_intf_t);

	while (1) {

#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
		ret = ca_l3_intf_iterate(0, &iterator);
#else
		ret = CA_E_NOT_SUPPORT;
#endif

		if ((ret != CA_E_OK) || (iterator.entry_count == 0))
			break;

		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("================= i=%d, intf_id=%d =====================\n", i, l3_intf.intf_id);
			printk("type=%d, port_id=0x%x, nat_enable=%d\n", l3_intf.type, l3_intf.port_id, l3_intf.nat_enable);
			printk("mac_addr=%pM\n", l3_intf.mac_addr);
			printk("outer_vid=%d, inner_vid=%d, tunnel_id=%d, mtu=%d\n", l3_intf.outer_vid, l3_intf.inner_vid, l3_intf.tunnel_id, l3_intf.mtu);
		}

		i++;
		/* we want to find the Broadcast interface type */
		if (l3_intf.type != CA_L3_INTF_BCAST)
			continue;

		if (ca_ni_debug & NI_DBG_NETLINK) {

			//ca_kh_ip_address_htonl(&l3_intf.ip_addr, &l3_intf.ip_addr);
			if (l3_intf.ip_addr.afi == CA_IPV4) {
				printk("afi=%d, addr_len=%d, ipv4_addr=%pI4\n",
						l3_intf.ip_addr.afi, l3_intf.ip_addr.addr_len, &l3_intf.ip_addr.ip_addr.ipv4_addr);
			} else {
				printk("afi=%d, addr_len=%d, ipv6_addr=%pI6\n",
						l3_intf.ip_addr.afi, l3_intf.ip_addr.addr_len, &l3_intf.ip_addr.ip_addr.ipv6_addr);
			}
		}

		src_ip_addr.addr_len = l3_intf.ip_addr.addr_len;

		ret = ca_api_is_ip_equal(&src_ip_addr, &l3_intf.ip_addr);
		if (ret == TRUE) {
			*intf_id = l3_intf.intf_id;
			if (ca_ni_debug & NI_DBG_NETLINK) {

				printk("%s: Got the intf_id=%d\n", __func__, l3_intf.intf_id);
			}
			return CA_E_OK;
		}

	}
	return CA_E_ERROR;
}
#endif

static ca_status_t ca_ni_netlink_get_source_port(ca_uint8_t *data, ca_uint8_t *port_id)
{
	ca_status_t ret;
	ca_mac_addr_t mac;
	ca_uint16_t key_vid;
	aal_fdb_entry_cfg_t fdb_entry;

	key_vid = ca_ni_rx_get_vid(data);
	memcpy(mac, data + 6, sizeof(ca_mac_addr_t));

	ret = aal_fdb_entry_get(0, mac, key_vid, &fdb_entry);
	if (ret == CA_E_OK) {
		*port_id = fdb_entry.port_id;
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: port_id=%d\n", __func__, fdb_entry.port_id);
		}
	}
	return ret;
}

/* Netlink for ca_rx() */
#define NETLINK_CA_RX	25
static struct sock *netlink_sock_rx;

void ca_ni_netlink_dump_pid_seq_list(void)
{
	int i;
	ca_ni_netlink_pid_seq_list_t *p_node;

	i = 0;
	for (p_node = ca_ni_netlink_pid_seq_head; p_node;  p_node = p_node->p_next) {
		printk("[%d]: pid=%d, seq=%lld\n", i, p_node->pid, (ca_int64_t)p_node->seq);
		i++;
	}
}

static ca_status_t ca_ni_netlink_find_seq_by_pid(pid_t pid, ca_int_t *seq)
{
	int i;
	ca_ni_netlink_pid_seq_list_t *p_node;

	spin_lock_bh(&ca_ni_netlink_pid_seq_lock);

	/* add to end of list */
	i = 0;
	for (p_node = ca_ni_netlink_pid_seq_head; p_node;  p_node = p_node->p_next) {
		if (p_node->pid == pid) {
			*seq = p_node->seq;
			//printk("[%d]: pid=%d, seq=%d\n", i, p_node->pid, p_node->seq);
			spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
			return CA_E_OK;
		}
		i++;
	}
	spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
	return CA_E_NOT_FOUND;
}

static ca_status_t ca_ni_netlink_alloc_pid_seq_list(ca_int32_t pid, ca_int_t seq)
{
	ca_ni_netlink_pid_seq_list_t *p_node, *p_new_node, *p_prev_node;

	if (ca_ni_netlink_find_seq_by_pid(pid, &seq) == CA_E_OK) {
		printk("%s: pid=%d already exists!!\n", __func__, pid);
		return CA_E_EXISTS;
	}

	spin_lock_bh(&ca_ni_netlink_pid_seq_lock);

	p_new_node = ca_malloc_trace(sizeof(ca_ni_netlink_pid_seq_list_t), GFP_KERNEL, true);
	if (p_new_node == NULL){
		printk("%s: Out of memory!!\n", __func__);
		spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
		return CA_E_MEMORY;
	}
	p_new_node->pid = pid;
	p_new_node->seq = seq;
	p_new_node->p_next = NULL;

	if (ca_ni_netlink_pid_seq_head == NULL) {
		ca_ni_netlink_pid_seq_head = p_new_node;
	}
	else {
		/* add to end of list */
		for (p_node = ca_ni_netlink_pid_seq_head; p_node;  p_node = p_node->p_next) {
			p_prev_node = p_node;
		}
		p_prev_node->p_next = p_new_node;
	}

	if (ca_ni_debug & NI_DBG_NETLINK) {
		ca_ni_netlink_dump_pid_seq_list();
	}
	spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
	return CA_E_OK;
}

static ca_status_t ca_ni_netlink_free_pid_seq_list(ca_int32_t pid, ca_int_t seq)
{
	ca_ni_netlink_pid_seq_list_t *p_node, *p_prev_node = NULL;

	spin_lock_bh(&ca_ni_netlink_pid_seq_lock);

	for (p_node = ca_ni_netlink_pid_seq_head; p_node;  p_node = p_node->p_next) {
		/* no need to check seq, because seq should be different between alloc() and free() */
		if (p_node->pid == pid) {
			if (p_node == ca_ni_netlink_pid_seq_head) {
				ca_ni_netlink_pid_seq_head = p_node->p_next;
			}
			else {
				if (p_prev_node != NULL)
					p_prev_node->p_next = p_node->p_next;
			}
			ca_free_trace(p_node);

			if (ca_ni_debug & NI_DBG_NETLINK) {
				ca_ni_netlink_dump_pid_seq_list();
			}
			spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
			return CA_E_OK;
		}
		p_prev_node = p_node;
	}
	spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
	return CA_E_NOT_FOUND;
}

static void ca_ni_netlink_check_pid_was_killed(void)
{
        struct pid *p_pid;
        struct task_struct *p_task;

        ca_ni_netlink_pid_seq_list_t *p_node, *p_prev_node = NULL, *p_save_node;;

        spin_lock_bh(&ca_ni_netlink_pid_seq_lock);

        /* for loop to check whether the pid was killed */
        for (p_node = ca_ni_netlink_pid_seq_head; p_node;) {
                p_save_node = p_node;
                /* no need to check seq, because seq should be different between alloc() and free() */
                p_pid = find_get_pid(p_node->pid);
                p_task = get_pid_task(p_pid, PIDTYPE_PID);
                if (p_task == NULL) {
			/* print out error message when pid was killed */
			//if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: pid=%d has been killed!!\n", __func__, p_node->pid);
			//}
                        if (p_node == ca_ni_netlink_pid_seq_head) {
                                ca_ni_netlink_pid_seq_head = p_node->p_next;
                        }
                        else {
                                if (p_prev_node != NULL)
                                        p_prev_node->p_next = p_node->p_next;
                        }

                        /* unregister all of the packet types registered by this pid */
                        __ca_rx_unregister_pkt_all(p_node->pid);

                        p_node = p_node->p_next;
                        ca_free_trace(p_save_node);

			if (ca_ni_debug & NI_DBG_NETLINK) {
				ca_ni_netlink_dump_pid_seq_list();
			}
                        continue;
                }
                p_prev_node = p_node;
                p_node = p_node->p_next;
        }
        spin_unlock_bh(&ca_ni_netlink_pid_seq_lock);
}

/*Callback function for netlink message receive */
static void ca_ni_netlink_rx(struct sk_buff *skb)
{
	pid_t pid;
	u_int uid, seq, sid;
	void *data;
	struct nlmsghdr *nlh;
	//ca_ni_netlink_pid_seq_list_t *p_list;

	nlh = (struct nlmsghdr *)skb->data;
	pid = NETLINK_CREDS(skb)->pid;
	uid = NETLINK_CREDS(skb)->uid.val;
	//sid = NETLINK_CB(skb).sid;
	seq = nlh->nlmsg_seq;
	data = NLMSG_DATA(nlh);

	/* when netlink_uincast() is called, kernel will call this received routine */
	if (pid == 0) {
		return;
	}
	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: recv skb from user space uid:%d pid:%d seq:%d,sid:%d\n", __func__, uid,pid,seq,sid);
		printk("%s: data is :%s\n", __func__, (char *)data);
	}

	/* if the Netlink RX sync is set iros will send ACK after receive packet from netlink_unicast */
	if (netlink_rx_ack) {
		if (!strcmp(data, "ACK")) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: Got ACK\n", __func__);
			}

			nextlink_rx_ack = 1;
			wake_up_interruptible(&netlink_ack_wait_queue);
			return;
		}
	}

	if (!strcmp(data, "GoodBye")) {
		printk("%s: ca_rx is killed!!!\n", __func__);

		ca_ni_netlink_free_pid_seq_list(pid, seq);
		__ca_rx_unregister_pkt_all(pid);

		return;
	}
	else {
		if (ca_ni_netlink_alloc_pid_seq_list(pid, seq) != CA_E_OK) {
			return;
		}
	}
}

ca_status_t ca_ni_netlink_rx_reply(pid_t pid, ca_int_t seq, ca_pkt_t *payload)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int size = sizeof(ca_pkt_t) + sizeof(ca_pkt_block_t) + payload->pkt_data->len;
	int len = NLMSG_SPACE(size);
	char *data;
	int ret;
	ca_pkt_t *tmp_payload;
	//struct sock *sk;
	//struct netlink_sock *nlk;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: payload->pkt_len=%d, payload->pkt_data->len=%d\n", __func__, payload->pkt_len, payload->pkt_data->len);
		printk("%s: pid=%d, seq=%lld, payload=%p, len=%d\n", __func__, pid, (ca_uint64_t)seq, payload, len);
	}

	skb = ca_alloc_skb(len, GFP_ATOMIC, true);
	if (!skb) {
		printk("%s: ca_alloc_skb(len=%d) failed!!!\n", __func__, len);
		return CA_E_ERROR;
	}
	nlh = NLMSG_PUT(skb, pid, seq, 0, size);
	nlh->nlmsg_flags = 0;

	data = NLMSG_DATA(nlh);
	tmp_payload = (ca_pkt_t *)data;
	memcpy(data, payload, sizeof(ca_pkt_t));

	data += sizeof(ca_pkt_t);
	tmp_payload->pkt_data = (ca_pkt_block_t *)data;
	memcpy(data, payload->pkt_data, sizeof(ca_pkt_block_t));

	data += sizeof(ca_pkt_block_t);
	tmp_payload->pkt_data->data = data;
	memcpy(data, payload->pkt_data->data, payload->pkt_data->len);

	//NETLINK_CB(skb).pid = 0; /* from kernel */
	NETLINK_CB(skb).dst_group = 0; /* unicast */

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: payload->src_port_id=0x%x\n", __func__, payload->src_port_id);
	}

	netlink_unicast_send_count++;

	ret = netlink_unicast(netlink_sock_rx, skb, pid, MSG_DONTWAIT);
	if (ret < 0)
	{
		netlink_unicast_fail_count++;

		if (ca_ni_debug & NI_DBG_NETLINK) {

			printk("%s: netlink_unicast() failed, ret=%d\n", __func__, ret);
		}
		return CA_E_ERROR;
	}
	return CA_E_OK;
}

static ca_uint8_t bpdu_mac_addr[ETH_ALEN]={0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};
static ca_uint8_t oam_mac_addr[ETH_ALEN]={0x01, 0x80, 0xc2, 0x00, 0x00, 0x02};

static ca_status_t ca_ni_netlink_get_rx_http_type(ca_uint8_t *data, ca_pkt_type_t *type)
{
	struct tcphdr *tcph;

	tcph = (struct tcphdr *)(data + sizeof(struct iphdr));

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: data=%p, tcph=%p, tcph->dest=%d, tcph->doff*4=%d\n", __func__,
				data, tcph, ntohs(tcph->dest), tcph->doff*4);
	}

	if (ntohs(tcph->dest) == 80) {
		data = data + sizeof(struct iphdr) + (tcph->doff * 4);
		if (!memcmp(data, "GET", 3)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP GET\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_GET;
			return CA_E_OK;
		}
		if (!memcmp(data, "PUT", 3)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP PUT\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_PUT;
			return CA_E_OK;
		}
		if (!memcmp(data, "HEAD", 4)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP HEAD\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_HEAD;
			return CA_E_OK;
		}
		if (!memcmp(data, "POST", 4)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP POST\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_POST;
			return CA_E_OK;
		}
		if (!memcmp(data, "OPTIONS", 7)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP OPTIONS\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_OPTIONS;
			return CA_E_OK;
		}
		if (!memcmp(data, "DELETE", 6)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP DELETE\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_DELETE;
			return CA_E_OK;
		}
		if (!memcmp(data, "TRACE", 5)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP TRACE\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_TRACE;
			return CA_E_OK;
		}
		if (!memcmp(data, "CONNECT", 7)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP CONNECT\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_CONNECT;
			return CA_E_OK;
		}
		if (!memcmp(data, "UNLINK", 6)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP UNLINK\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_UNLINK;
			return CA_E_OK;
		}
		if (!memcmp(data, "PATCH", 5)) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: HTTP PATCH\n", __func__);
			}
			*type = CA_PKT_TYPE_HTTP_PATCH;
			return CA_E_OK;
		}
	}
	return CA_E_NOT_FOUND;
}

static ca_status_t ca_ni_netlink_get_pkt_type_by_eth_type(ca_uint8_t *data, ca_uint16 ether_type, ca_pkt_type_t *type)
{
	ca_uint8_t subtype = 0;
	ca_mac_addr_t da_mac_addr;
	ca_status_t ret = CA_E_OK;

	switch (ether_type) {
		case ETH_P_SLOW:
			if (!memcmp(data, oam_mac_addr, ETH_ALEN)) {
				/* check subtype for 1AS packets */
				subtype = data[ETH_ALEN*2+2];
				if (subtype == 0x3) {
					*type = CA_PKT_TYPE_OAM;
				}
				else if (subtype == 0xA) {
					*type = CA_PKT_TYPE_1AS;
				}
			}
			else {
				ret = CA_E_NOT_FOUND;
			}
			break;

		case ETH_P_ARP:
			*type = CA_PKT_TYPE_ARP;
			break;

		case ETH_P_PAUSE:
			*type = CA_PKT_TYPE_MPCP;
			break;

		case ETH_P_PPP_DISC:
			*type = CA_PKT_TYPE_PPPOE_DIS;
			break;

		case ETH_P_PPP_SES:
			*type = CA_PKT_TYPE_PPPOE_SES;
			break;

		case ETH_P_1588:
			*type = CA_PKT_TYPE_L2_PTP;
			break;

		case ETH_P_PAE:
			*type = CA_PKT_TYPE_DOT1X;
			break;

		case 0x8902:
			*type = CA_PKT_TYPE_CFM;
			break;

		case ETH_P_LOOPBACK:
			*type = CA_PKT_TYPE_LOOPBACK;
			break;

		case CA_PUC_GLOBAL_MAC_TYPE:
			if (CA_PON_MODE_IS_EPON_FAMILY(pon_mac_mode)) {
				*type = CA_PKT_TYPE_MPCP;
				break;
			}
			if (CA_PON_MODE_IS_GPON_FAMILY(pon_mac_mode)) {
				*type = CA_PKT_TYPE_PLOAM;
				break;
			}
			ret = CA_E_NOT_FOUND;
			break;

		case CA_PUC_GLOBAL_LNK_TYPE:
			if (CA_PON_MODE_IS_EPON_FAMILY(pon_mac_mode)
					|| CA_PON_MODE_IS_GPON_FAMILY(pon_mac_mode) ) {
				*type = CA_PON_MODE_IS_EPON_FAMILY(pon_mac_mode)? CA_PKT_TYPE_OAM : CA_PKT_TYPE_OMCI;
				break;
			}
			ret = CA_E_NOT_FOUND;
			break;

		case 0xffff:
			memcpy(&da_mac_addr, data, sizeof(ca_mac_addr_t));
			if (aal_l2_specpkt_match_hello_packet(da_mac_addr) == CA_E_OK) {
				*type = CA_PKT_TYPE_IROS_HELLO;
			}else {
				ret = CA_E_NOT_FOUND;
			}
			break;

		default:
			*type = CA_PKT_TYPE_OTHERS;
			ret = CA_E_NOT_FOUND;
			break;
	}

	/* wait for aal_l2_specpkt_match_udf_packet() is ready */
#if 0
	/* Check whether the packet is user defined packet by DA MAC and ethertype */
	memcpy(&da_mac_addr, data, sizeof(ca_mac_addr_t));

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: ether_type=0x%x, da_mac=%pM\n", __func__, ether_type, da_mac_addr);
	}
	if (aal_l2_specpkt_match_udf_packet(da_mac_addr, ether_type) == CA_E_OK) {
		*type = CA_PKT_TYPE_USER_DEFINED;
		ret = CA_E_OK;
	}
#endif

	return ret;
}

static ca_status_t ca_ni_netlink_get_pkt_type_by_ipv4_protocol(ca_uint8_t *data, ca_pkt_type_t *type)
{
	struct iphdr  *iph  = (struct iphdr *)(data);
	struct udphdr *udph = NULL;
	ca_uint16_t     dest_port;
	ca_status_t     ret = CA_E_OK;

	*type = CA_PKT_TYPE_OTHERS;

	switch (iph->protocol) {
		case IPPROTO_IGMP:
			*type = CA_PKT_TYPE_IGMP;
			break;

		case IPPROTO_ICMP:
			*type = CA_PKT_TYPE_ICMP;
			break;

		case IPPROTO_UDP:
			udph = (struct udphdr *)(data + sizeof(struct iphdr));
			dest_port = ntohs(udph->dest);

			switch (dest_port) {
				case 67:
				case 68:
					*type = CA_PKT_TYPE_DHCP;
					break;

				case 319:
				case 320:
					*type = CA_PKT_TYPE_L4_PTP;
					break;

				case 53:
					*type = CA_PKT_TYPE_DNS;
					break;

				case 520:
					*type = CA_PKT_TYPE_RIP;
					break;

				case 1900:
				case 2869:
				case 5000:
					if (*data & 0x1) {
						*type = CA_PKT_TYPE_SSDP;
					}
					break;
				default:
					ret = CA_E_NOT_FOUND;
					break;
			}
			break;

		case    IPPROTO_TCP:
			ret = ca_ni_netlink_get_rx_http_type(data, type);
			break;

		default:
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("\r\nNot find this ipv4 frame type with protocol=0x%x in %s %d\r\n", iph->protocol,  __FUNCTION__, __LINE__);
			}
			ret = CA_E_NOT_FOUND;
			break;

	}

	return ret;
}

ca_status_t ca_ni_netlink_get_pkt_type_by_ipv6_protocol(ca_uint8_t * data, ca_pkt_type_t *type)
{
	ca_uint8        nexthdr = 0;
	struct ipv6_opt_hdr  * v6_opt_h = NULL;
	struct ipv6hdr *ipv6h = NULL;
	struct udphdr  *udph = NULL;
	struct icmphdr *icmph = NULL;
	ca_uint8_t     *cur_data = data;
	ca_uint16_t      dest_port = 0;
	ca_status_t     ret = CA_E_OK;

	*type = CA_PKT_TYPE_OTHERS;
	ipv6h = (struct ipv6hdr *)(data);

	cur_data += sizeof(struct ipv6hdr);

	/*check extern header*/
	for (nexthdr = ipv6h->nexthdr; IPV6_IS_EXTERN_HEADER(nexthdr); ) {
		v6_opt_h = (struct ipv6_opt_hdr *)cur_data;
		nexthdr = v6_opt_h->nexthdr;
		cur_data += (v6_opt_h->hdrlen + 1) * 8;
	}

	switch (nexthdr) {
		case    IPPROTO_UDP:
			udph = (struct udphdr *)cur_data;
			dest_port = ntohs(udph->dest);

			switch (dest_port) {
				case 546:
				case 547:
					*type = CA_PKT_TYPE_DHCPV6;
					break;

				case 319:
				case 320:
					*type = CA_PKT_TYPE_L4_PTP;
					break;

				default:
					ret = CA_E_NOT_FOUND;
					break;
			}
			break;

		case    IPPROTO_ICMPV6:
			icmph = (struct icmphdr *)(cur_data);

			switch (icmph->type){
				case 130:
				case 131:
				case 132:
				case 143:
					*type = CA_PKT_TYPE_ICMPV6_MLD;
					break;

				case 133:
				case 134:
				case 135:
				case 136:
				case 137:
				case 146:
				case 147:
					*type = CA_PKT_TYPE_ICMPV6_NDP;
					break;

				default:
					*type = CA_PKT_TYPE_ICMP;
					break;
			}
			break;

		default:
			ret = CA_E_NOT_FOUND;
			break;
	}

	if (*type == CA_PKT_TYPE_OTHERS) {
		ret = CA_E_NOT_FOUND;
	}
	else {
		ret = CA_E_OK;
	}
	return ret;
}

ca_status_t ca_ni_netlink_get_rx_type(struct sk_buff *skb, ca_uint8_t *data, ca_pkt_type_t *type)
{
	ca_status_t ret = CA_E_OK;
	ca_uint16_t ether_type;
	ca_uint8_t *save_data = data;

	/* check BPDU */
	if (!memcmp(save_data, bpdu_mac_addr, ETH_ALEN)) {
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: got CA_PKT_TYPE_BPDU packet\n", __func__);
		}
		*type = CA_PKT_TYPE_BPDU;
		return CA_E_OK;
	}

	/* skip vlan tag and get ether type */
	ether_type = ntohs(ca_ni_parse_l2(data, NULL, NULL, NULL, &data, (ca_ni_debug & NI_DBG_NETLINK) ? true : false));

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: ether_type=0x%x\n", __func__, ether_type);
	}

	ret = ca_ni_netlink_get_pkt_type_by_eth_type(save_data, ether_type, type);
	/* if found the pkt_type by eth_type then return here */
	if (ret == CA_E_OK) {
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: pkt type=%s\n", __func__, ca_ni_netlink_pkt_type_str[*type]);
		}
		return ret;
	}

	if (ether_type == ETH_P_IP) {
		/* for IPv4 packets, get pkt_type from IPv4 protocol */
		ret = ca_ni_netlink_get_pkt_type_by_ipv4_protocol(data, type);
	}
	else if (ether_type == ETH_P_IPV6) {
		/* for IPv6 packets, get pkt_type from IPv6 protocol */
		ret = ca_ni_netlink_get_pkt_type_by_ipv6_protocol(data, type);
	}

	if (ca_ni_debug & NI_DBG_NETLINK) {
		if (ret == CA_E_OK) {
			printk("%s: pkt type=%s\n", __func__, ca_ni_netlink_pkt_type_str[*type]);
		}
		else {
			printk("%s: Not found pkt type!!\n", __func__);
		}
	}
	return ret;
}

static ca_status_t ca_ni_netlink_find_registered_rx_type(ca_uint8_t *data, ca_pkt_type_t *type, ca_uint32_t mirror, ca_ni_netlink_cb_list_t **p_ret_node, struct sk_buff *skb)
{
	ca_ni_netlink_cb_list_t  *p_node = NULL;

	spin_lock_bh(&ca_ni_netlink_cb_lock);

	if (ca_ni_netlink_cb_high_head == NULL && ca_ni_netlink_cb_low_head == NULL) {
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: No RX type has registered!!\n", __func__);
		}
		spin_unlock_bh(&ca_ni_netlink_cb_lock);
		return CA_E_NOT_FOUND;
	}

	if (ca_ni_netlink_get_rx_type(skb, data, type) == CA_E_OK) {
		/* if it's a PTP packet ignore mirror bit */
		if (*type != CA_PKT_TYPE_L2_PTP && *type != CA_PKT_TYPE_L4_PTP) {
			if (mirror) {
				if ((*type != CA_PKT_TYPE_IGMP) && (*type != CA_PKT_TYPE_ICMPV6_MLD)) {
					*type = CA_PKT_TYPE_MIRROR;
				}
			}
		}
	}
	else {
		if (mirror) {
			*type = CA_PKT_TYPE_MIRROR;
		}
		else {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: No RX type is matched!!\n", __func__);
			}
			spin_unlock_bh(&ca_ni_netlink_cb_lock);
			return CA_E_NOT_FOUND;
		}
	}

	if (ca_ni_netlink_cb_high_head != NULL) {
		for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: high priority, p_node->type=%d, type=%d\n", __func__, p_node->type, *type);
			}
			if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == *type) {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: Found packet type=%d\n", __func__, *type);
				}
				*p_ret_node = p_node;
				spin_unlock_bh(&ca_ni_netlink_cb_lock);
				return CA_E_OK;
			}
		}
	}
	if (ca_ni_netlink_cb_low_head != NULL) {
		for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: low priority, p_node->type=%d, type=%d\n", __func__, p_node->type, *type);
			}
			if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == *type) {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: Found packet type=%d, pid=%d, seq=%lld\n", __func__, *type, p_node->pid, (ca_int64_t)p_node->seq);
				}
				*p_ret_node = p_node;
				spin_unlock_bh(&ca_ni_netlink_cb_lock);
				return CA_E_OK;
			}
		}
	}
	spin_unlock_bh(&ca_ni_netlink_cb_lock);
	return CA_E_NOT_FOUND;
}

static ca_status_t ca_ni_netlink_set_port_id(ca_uint8_t *data, ca_pkt_t *pkt)
{
	ca_intf_id_t intf_id;
	ca_uint8_t port_id;
	ca_status_t ret;

#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
        ret = ca_ni_netlink_get_intf_id(data, &intf_id);
        if (ret != CA_E_OK) {
		if (ca_ni_debug & NI_DBG_NETLINK)
			printk("%s: ca_ni_netlink_get_intf_id(), ret=%d\n", __func__, ret);
                //return CA_E_ERROR;
	}
#endif

        ret = ca_ni_netlink_get_source_port(data, &port_id);
        if (ret != CA_E_OK) {
		if (ca_ni_debug & NI_DBG_NETLINK)
			printk("%s: ca_ni_netlink_get_source_port(), ret=%d\n", __func__, ret);
                return CA_E_ERROR;
	}

        if (ca_ni_debug & NI_DBG_NETLINK) {
                printk("%s: intf_id=%d, port_id=%d\n", __func__, intf_id, port_id);
        }

        /* packet from 0x1b */
        if (PORT_ID(pkt->src_port_id) > CA_NI_PORT7) {
                pkt->src_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port_id);
                pkt->reserved = (void *)(ca_uint_t)intf_id;
        }
	return CA_E_OK;
}

ca_status_t ca_ni_netlink_rx_reply_all(ca_pkt_t *pkt, ca_pkt_type_t type, struct sk_buff *skb)
{
	ca_status_t ret = CA_E_OK;
	ca_ni_netlink_cb_list_t  *p_node = NULL;

	spin_lock_bh(&ca_ni_netlink_cb_lock);

	if (ca_ni_netlink_cb_high_head != NULL) {
		for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: high priority, p_node->type=%d, type=%d\n", __func__, p_node->type, type);
			}
			/* if type is match or packet type is control all the send packet through netlink */
			if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == type) {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: Found packet type=%d, pid=%d, seq=%lld\n", __func__, type, p_node->pid, (ca_int64_t)p_node->seq);
				}
				/* if Netlink RX sync is set put the received packets to RX queue first */
				if (netlink_rx_ack) {
					ret = ca_ni_netlink_rx_queue_put(p_node->pid, p_node->seq, pkt, skb);
					if (ret == CA_E_FULL) {
						spin_unlock_bh(&ca_ni_netlink_cb_lock);
						return ret;
					}
				}
				else
					ca_ni_netlink_rx_reply(p_node->pid, p_node->seq, pkt);
			}
		}
	}
	if (ca_ni_netlink_cb_low_head != NULL) {
		for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
			if (ca_ni_debug & NI_DBG_NETLINK) {
				printk("%s: low priority, p_node->type=%d, type=%d\n", __func__, p_node->type, type);
			}
			/* if type is match or packet type is control all the send packet through netlink */
			if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == type) {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: Found packet type=%d, pid=%d, seq=%lld\n", __func__, type, p_node->pid, (ca_int64_t)p_node->seq);
				}
				/* if Netlink RX sync is set put the received packets to RX queue first */
				if (netlink_rx_ack) {
					ret = ca_ni_netlink_rx_queue_put(p_node->pid, p_node->seq, pkt, skb);
					if (ret == CA_E_FULL) {
						spin_unlock_bh(&ca_ni_netlink_cb_lock);
						return ret;
					}
				}
				else
					ca_ni_netlink_rx_reply(p_node->pid, p_node->seq, pkt);
			}
		}
	}
	spin_unlock_bh(&ca_ni_netlink_cb_lock);

	return ret;
}

ca_status_t ca_ni_netlink_send_packet(int cpu_port, HEADER_A_T *hdr_a, ca_uint8_t *data, ca_uint32_t pkt_size, HEADER_CPU_T *hdr_cpu, struct sk_buff *skb)
{
	ca_status_t ret;
	ca_pkt_t pkt;
	ca_pkt_block_t    pkt_blk;
	ca_pkt_type_t 	pkt_type;
	ca_ni_netlink_cb_list_t *p_node;
	ca_uint32_t mirror = hdr_a->bits.bits_32_63.pkt_info.mirror;
	ca_uint32_t lspid = hdr_a->bits.lspid;
	ca_uint16_t ethertype;
	ca_uint8_t *tmp_ptr;
	int vlan_offset;
	struct timeval tv;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: cpu_port=%d, lspid=%d, data=%p, pkt_size=%d\n", __func__, cpu_port, lspid, data, pkt_size);
	}

	if (data == NULL) {
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: data == NULL\n", __func__);
		}
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: No pkt_type=%d found!!\n", __func__, pkt_type);
		}
		return CA_E_ERROR;
	}

	/* check whether there is registered from user space */
	if (ca_ni_netlink_find_registered_rx_type(data, &pkt_type, mirror, &p_node, skb) != CA_E_OK) {
		return CA_E_ERROR;
	}
	else {
		/* if MLD packet calulate checksum */
		if (pkt_type == CA_PKT_TYPE_ICMPV6_MLD) {
			ethertype = ca_ni_parse_l2(data, NULL, NULL, &tmp_ptr, NULL, (ca_ni_debug & NI_DBG_NETLINK) ? true : false);
			vlan_offset = tmp_ptr - (data + 12); /* offset from MAC_DA to EtherType within packet */
			skb_set_network_header(skb, vlan_offset);
			skb_set_transport_header(skb, vlan_offset);

			if (ca_ni_netlink_icmpv6_checksum(skb, skb->data + vlan_offset) == CA_E_ERROR) {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: MLD checksum error!!\n", __func__);
				}
				return CA_E_OK;   /* do nothing and drop in ca_ni_rx.c */
			}
			else {
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: MLD checksum OK!!\n", __func__);
				}
			}
		}
	}

	/* if Netlink RX queue is full, then free the incoming packet */
	if (netlink_rx_queue_is_full) {
		ca_dev_kfree_skb(skb);
		netlink_rx_free_skb_count++;
		netlink_rx_queue_full_count++;
		return CA_E_FULL;
	}

	memset(&pkt, 0, sizeof(ca_pkt_t));
	memset(&pkt_blk, 0, sizeof(ca_pkt_block_t));

	pkt_blk.data = data;
	pkt_blk.len = pkt_size;
	pkt.pkt_data = &pkt_blk;
	pkt.block_count = 1;
	pkt.pkt_len = pkt_size;
	pkt.pkt_type = pkt_type;

	/* copy ptp_info of Header A to ptp_timestamp */
	if (pkt_type == CA_PKT_TYPE_L4_PTP || pkt_type == CA_PKT_TYPE_L2_PTP) {
		do_gettimeofday(&tv);
		pkt.ptp_timestamp.sec = tv.tv_sec;
		pkt.ptp_timestamp.nsec = hdr_a->bits.bits_32_63.ptp_info.ptp_info;
	}

	/* fill other fileds of ca_pkt_t */
	ca_ni_rx_fill_pkt(cpu_port, &pkt, hdr_a, hdr_cpu);

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: data=%p, pkt_size=%d, lspid=%d, pkt_type=%d\n", __func__, data, pkt_size, lspid, pkt_type);
		printk("%s: pkt.sw_id[0-3]=0x%x-0x%x-0x%x-0x%x\n", __func__, pkt.sw_id[0], pkt.sw_id[1], pkt.sw_id[2], pkt.sw_id[3]);
	}

	/* if not WAN port, get the physical dst port id if port > 7 */
	if (cpu_port != 0) {
		if (ca_ni_netlink_set_port_id(data, &pkt) != CA_E_OK) {
			return CA_E_ERROR;
		}
	}

	/* Don't clean the rx handle list */
	/*ca_ni_netlink_check_pid_was_killed();*/

	ret = ca_ni_netlink_rx_reply_all(&pkt, pkt_type, skb);

	return ret;
}

static DEFINE_MUTEX(ca_ni_nl_mutex);

struct netlink_kernel_cfg nl_cfg_rx = {
	.groups         = RTNLGRP_MAX,
	.input          = ca_ni_netlink_rx,
	.cb_mutex       = &ca_ni_nl_mutex,
	.flags          = NL_CFG_F_NONROOT_RECV,
};

void ca_rx_register_pkt_dump_all(void)
{
	int i;
	ca_ni_netlink_cb_list_t  *p_node = NULL;

	printk("%s: ===== dump high priority RX register list =====\n", __func__);
	if (ca_ni_netlink_cb_high_head != NULL) {
		i = 0;
		for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
			printk("[%d] pid=%d, seq=%lld, priority=%d, type=%d\n", i, p_node->pid, (ca_int64_t)p_node->seq, p_node->priority, p_node->type);
			i++;
		}
	}

	printk("%s: ===== dump low priority RX register list =====\n", __func__);
	if (ca_ni_netlink_cb_low_head != NULL) {
		i = 0;
		for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
				printk("[%d] pid=%d, seq=%lld, priority=%d, type=%d\n", i, p_node->pid, (ca_int64_t)p_node->seq, p_node->priority, p_node->type);
			i++;
		}
	}
}

ca_status_t ca_rx_register_pkt(
		CA_IN ca_device_id_t 	device_id,
		CA_IN pid_t		pid,
		CA_IN ca_uint8_t     	priority,
		CA_IN ca_pkt_type_t  	type)
{
	ca_ni_netlink_cb_list_t  *p_node = NULL;
	ca_ni_netlink_cb_list_t  *p_new_node = NULL;
	ca_ni_netlink_cb_list_t  *p_prev_node = NULL;
	ca_int_t	seq;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: pid=%d, priority=%d, type=%d\n", __func__, pid, priority, type);
	}

	spin_lock_bh(&ca_ni_netlink_cb_lock);
	if (priority) {
		if (ca_ni_netlink_cb_high_head != NULL){
			for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->priority == priority && p_node->pid == pid) {
					if (ca_ni_debug & NI_DBG_NETLINK) {
						printk("%s: type=%d already exists!!\n", __func__, type);
					}
					spin_unlock_bh(&ca_ni_netlink_cb_lock);
					return CA_E_EXISTS;
				}
			}
		}
	}
	else {
		if (ca_ni_netlink_cb_low_head != NULL){
			for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->priority == priority && p_node->pid == pid) {
					if (ca_ni_debug & NI_DBG_NETLINK) {
						printk("%s: type=%d already exists!!\n", __func__, type);
					}
					spin_unlock_bh(&ca_ni_netlink_cb_lock);
					return CA_E_EXISTS;
				}
			}
		}
	}

	p_new_node = ca_malloc_trace(sizeof(ca_ni_netlink_cb_list_t), GFP_KERNEL, true);

	if (p_new_node == NULL){
		printk("%s: Out of memory!!\n", __func__);
		spin_unlock_bh(&ca_ni_netlink_cb_lock);
		return CA_E_MEMORY;
	}
	memset(p_new_node, 0, sizeof(ca_ni_netlink_cb_list_t));

	if (priority) {
		if (ca_ni_netlink_cb_high_head == NULL) {
			p_new_node->p_next = ca_ni_netlink_cb_high_head;
			ca_ni_netlink_cb_high_head = p_new_node;
		}
		else {
			/* add to end of list */
			for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
				p_prev_node = p_node;
			}
			p_prev_node->p_next = p_new_node;
		}
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: p_new_node=%p, ca_ni_netlink_cb_high_head=%p\n", __func__, p_new_node, ca_ni_netlink_cb_high_head);
		}
	}
	else {
		if (ca_ni_netlink_cb_low_head == NULL) {
			p_new_node->p_next = ca_ni_netlink_cb_low_head;
			ca_ni_netlink_cb_low_head = p_new_node;
		}
		else {
			/* add to end of list */
			for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
				p_prev_node = p_node;
			}
			p_prev_node->p_next = p_new_node;
		}
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: p_new_node=%p, ca_ni_netlink_cb_low_head=%p\n", __func__, p_new_node, ca_ni_netlink_cb_low_head);
		}
	}

	if (ca_ni_netlink_find_seq_by_pid(pid, &seq) != CA_E_OK) {
		printk("%s: pid=%d not exists!!\n", __func__, pid);
	}

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: pid=%d, seq=%lld, type=%d, priority=%d\n", __func__, pid, (ca_int64_t)seq, type, priority);
	}

	p_new_node->pid		= pid;
	p_new_node->seq		= seq;
	p_new_node->priority	= priority;
	p_new_node->type    	= type;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		ca_rx_register_pkt_dump_all();
	}

	spin_unlock_bh(&ca_ni_netlink_cb_lock);
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_rx_register_pkt);

static ca_status_t __ca_rx_unregister_pkt(
	pid_t             pid,
	ca_uint8_t        priority,
	ca_pkt_type_t     type)
{
	ca_ni_netlink_cb_list_t  *p_node = NULL;
	ca_ni_netlink_cb_list_t  *p_prev_node = NULL;

	spin_lock_bh(&ca_ni_netlink_cb_lock);

	if (priority) {
		if (ca_ni_netlink_cb_high_head != NULL){
			for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->priority == priority && p_node->pid == pid) {
					if (p_node == ca_ni_netlink_cb_high_head) {
						ca_ni_netlink_cb_high_head = p_node->p_next;
					}
					else {
						if (p_prev_node != NULL)
							p_prev_node->p_next = p_node->p_next;
					}
					ca_free_trace(p_node);

					if (ca_ni_debug & NI_DBG_NETLINK) {
						ca_rx_register_pkt_dump_all();
					}
					spin_unlock_bh(&ca_ni_netlink_cb_lock);
					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
			spin_unlock_bh(&ca_ni_netlink_cb_lock);
			return CA_E_NOT_FOUND;
		}
	}
	else {
		if (ca_ni_netlink_cb_low_head != NULL){
			for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->priority == priority && p_node->pid == pid) {
					if (p_node == ca_ni_netlink_cb_low_head) {
						ca_ni_netlink_cb_low_head = p_node->p_next;
					}
					else {
						if (p_prev_node != NULL)
							p_prev_node->p_next = p_node->p_next;
					}
					ca_free_trace(p_node);

					if (ca_ni_debug & NI_DBG_NETLINK) {
						ca_rx_register_pkt_dump_all();
					}
					spin_unlock_bh(&ca_ni_netlink_cb_lock);
					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
			spin_unlock_bh(&ca_ni_netlink_cb_lock);
			return CA_E_NOT_FOUND;
		}
	}
	spin_unlock_bh(&ca_ni_netlink_cb_lock);
	return CA_E_NOT_FOUND;
}

/* free all of nodes which belong to the specified pid */
static void __ca_rx_unregister_pkt_all(pid_t pid)
{
        ca_ni_netlink_cb_list_t  *p_node = NULL;
        ca_ni_netlink_cb_list_t  *p_prev_node = NULL;

	spin_lock_bh(&ca_ni_netlink_cb_lock);
	if (ca_ni_netlink_cb_high_head != NULL) {
		for (p_node = ca_ni_netlink_cb_high_head; p_node;  p_node = p_node->p_next) {
			if (p_node->pid == pid) {
				if (p_node == ca_ni_netlink_cb_high_head) {
					ca_ni_netlink_cb_high_head = p_node->p_next;
				}
				else {
					if (p_prev_node != NULL)
						p_prev_node->p_next = p_node->p_next;
				}
				ca_free_trace(p_node);

				if (ca_ni_debug & NI_DBG_NETLINK) {
					ca_rx_register_pkt_dump_all();
				}
			}
			else {
				p_prev_node = p_node;
			}
		}
	}

	if (ca_ni_netlink_cb_low_head != NULL) {
		for (p_node = ca_ni_netlink_cb_low_head; p_node;  p_node = p_node->p_next) {
			if (p_node->pid == pid) {
				if (p_node == ca_ni_netlink_cb_low_head) {
					ca_ni_netlink_cb_low_head = p_node->p_next;
				}
				else {
					if (p_prev_node != NULL)
						p_prev_node->p_next = p_node->p_next;
				}
				ca_free_trace(p_node);

				if (ca_ni_debug & NI_DBG_NETLINK) {
					ca_rx_register_pkt_dump_all();
				}
			}
			else {
				p_prev_node = p_node;
			}
		}
	}
	spin_unlock_bh(&ca_ni_netlink_cb_lock);
}

ca_status_t ca_rx_unregister_pkt(
		CA_IN ca_device_id_t	device_id,
		CA_IN pid_t		pid,
		CA_IN ca_uint8_t     	priority,
		CA_IN ca_pkt_type_t  	type)
{
	ca_status_t ret;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: pid=%d, priority=%d, type=%d\n", __func__, pid, priority, type);
	}

	ret = __ca_rx_unregister_pkt(pid, priority, type);
	return ret;
}
EXPORT_SYMBOL(ca_rx_unregister_pkt);

#ifdef CA_NI_NETLINK_MIRROR_TEST
static void ca_ni_netlink_set_mirror(void)
{
	ca_status_t ret;
	aal_mire_mir_rule_mask_t   rule_mask;
	aal_mire_mir_rule_t	   rule;
	aal_mire_mir_act_mask_t	   act_mask;
	aal_mire_mir_act_t	   act;

	rule_mask.wrd = 0;
	rule_mask.bf.lspid = 1;
	rule_mask.bf.mode = 1;

	memset(&rule, 0, sizeof(aal_mire_mir_rule_t));
	rule.lspid = 2;
	rule.mode = MIR_INGRESS_PORT;

	ret = aal_mire_rule_set(0, 0, rule_mask, &rule);
	printk("%s: aal_mire_rule_set(), ret=%d\n", __func__, ret);

	act_mask.wrd = 0;
	act_mask.bf.ldpid = 1;

	memset(&act, 0, sizeof(aal_mire_mir_act_t));
	act.ldpid = 0x11;

	ret = aal_mire_act_set(0, 0, act_mask, &act);
	printk("%s: al_mire_act_set(), ret=%d\n", __func__, ret);
}
#endif

int ca_ni_netlink_rx_init(void)
{
	netlink_sock_rx = netlink_kernel_create(&init_net, NETLINK_CA_RX, &nl_cfg_rx);

	if (!netlink_sock_rx) {
		printk("%s: ca_rx Netlink socket creation failed!!\n", __func__);
		return -1;
	}

#ifdef CA_NI_NETLINK_MIRROR_TEST
	ca_ni_netlink_set_mirror();
#endif
	spin_lock_init(&ca_ni_netlink_cb_lock);
	spin_lock_init(&ca_ni_netlink_pid_seq_lock);

	aal_scfg_read(CFG_ID_NE_NETLINK_RX_ACK, 1, &netlink_rx_ack);
	printk("%s: read from starup config, netlink_rx_ack=%d\n", __func__, netlink_rx_ack);

	if (netlink_rx_ack) {
		spin_lock_init(&ca_ni_netlink_rx_queue_lock);
		ca_ni_netlink_rx_thread_create();

		netlink_rx_queue = ca_malloc_trace(NETLINK_RX_QUEUE_NUM * sizeof(ca_ni_netlink_payload_t), GFP_KERNEL, true);
		if (netlink_rx_queue == NULL) {
			sock_release(netlink_sock_rx->sk_socket);
			printk("%s: allocate netlink_rx_queue failed!!\n", __func__);
			return -1;
		}
		memset(netlink_rx_queue, 0, NETLINK_RX_QUEUE_NUM * sizeof(ca_ni_netlink_payload_t));
		netlink_rx_pkt = ca_malloc_trace(NETLINK_RX_QUEUE_NUM * sizeof(ca_pkt_t),  GFP_KERNEL, true);
		if (netlink_rx_pkt == NULL) {
			printk("%s: allocate netlink_rx_pkt failed!!\n", __func__);
			sock_release(netlink_sock_rx->sk_socket);
			ca_free_trace(netlink_rx_queue);
			return -1;
		}
		memset(netlink_rx_pkt, 0, NETLINK_RX_QUEUE_NUM * sizeof(ca_pkt_t));
	}

	return 0;
}

void ca_ni_netlink_rx_exit(void)
{
	sock_release(netlink_sock_rx->sk_socket);
	ca_free_trace(netlink_rx_queue);
	ca_free_trace(netlink_rx_pkt);
	printk("netlink socket for ca_rx remove successfully\n");
}

void ca_ni_netlink_rx_queue_dump(void)
{
	printk("netlink_rx_ack=%d, netlink_rx_queue_is_full=%d\n", netlink_rx_ack, netlink_rx_queue_is_full);
	printk("netlink_rxq_put_count=%d, netlink_rxq_get_count=%d\n",
			netlink_rxq_put_count, netlink_rxq_get_count);
	printk("rd_ptr=%d, wr_ptr=%d\n", netlink_rx_queue_rd_ptr, netlink_rx_queue_wr_ptr);
	printk("netlink_sync_ack_lost_count=%d, netlink_rx_queue_full_count=%d, netlink_unicast_fail_count=%d\n",
			netlink_sync_ack_lost_count, netlink_rx_queue_full_count, netlink_unicast_fail_count);
	printk("netlink_rx_free_skb_count=%ld\n", netlink_rx_free_skb_count);
	printk("netlink_unicast_send_count=%d\n", netlink_unicast_send_count);
}

static ca_status_t ca_ni_netlink_rx_queue_put(pid_t pid, ca_int_t seq, ca_pkt_t *pkt, struct sk_buff *skb)
{
	ca_pkt_t *tmp_pkt;
	char *data;
	int count;

	spin_lock_bh(&ca_ni_netlink_rx_queue_lock);

	if (netlink_rx_queue_wr_ptr >= netlink_rx_queue_rd_ptr) {
		count = netlink_rx_queue_wr_ptr - netlink_rx_queue_rd_ptr;
	}
	else {
		count = NETLINK_RX_QUEUE_NUM - netlink_rx_queue_rd_ptr + netlink_rx_queue_wr_ptr;
	}

	/* buffer is full drop the incoming packets */
	if (count >= (NETLINK_RX_QUEUE_NUM - 1)) {
		ca_dev_kfree_skb(skb);
		netlink_rx_queue_is_full = 1;
		netlink_rx_queue_full_count++;
		netlink_rx_free_skb_count++;
		spin_unlock_bh(&ca_ni_netlink_rx_queue_lock);
		return CA_E_FULL;
	}
	netlink_rx_queue_is_full = 0;

	netlink_rx_queue[netlink_rx_queue_wr_ptr].pid = pid;
	netlink_rx_queue[netlink_rx_queue_wr_ptr].seq = seq;
	netlink_rx_queue[netlink_rx_queue_wr_ptr].skb = skb;

	tmp_pkt = &(netlink_rx_queue[netlink_rx_queue_wr_ptr].pkt);
	memcpy(tmp_pkt, pkt, sizeof(ca_pkt_t));

	tmp_pkt->pkt_data = &(netlink_rx_queue[netlink_rx_queue_wr_ptr].pkt_blk);
	memcpy(tmp_pkt->pkt_data, pkt->pkt_data, sizeof(ca_pkt_block_t));

	/* use the data in skb struct and not free skb until skb->data has been netlink_unicast() */
	//tmp_pkt->pkt_data->data = &(netlink_rx_queue[netlink_rx_queue_wr_ptr].data);
	//memcpy(tmp_pkt->pkt_data->data, pkt->pkt_data->data, pkt->pkt_data->len);
	tmp_pkt->pkt_data->data = pkt->pkt_data->data;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		data = tmp_pkt->pkt_data->data;
		printk("%s: tmp_pkt=0x%p, data=0x%p\n", __func__, tmp_pkt, data);
		printk("%s: DA_MAC[%2x-%2x-%2x-%2x-%2x-%2x]\n", __func__, data[0], data[1], data[2], data[3], data[4], data[5]);
	}

	netlink_rx_queue_wr_ptr = CA_NI_RWPTR_ADVANCE_ONE(netlink_rx_queue_wr_ptr, NETLINK_RX_QUEUE_NUM);

	if (ca_ni_debug & NI_DBG_NETLINK) {
		printk("%s: rd_ptr=%d, wr_ptr=%d\n", __func__, netlink_rx_queue_rd_ptr, netlink_rx_queue_wr_ptr);
	}

	netlink_rxq_put_count++;

	spin_unlock_bh(&ca_ni_netlink_rx_queue_lock);
	return CA_E_OK;
}

static ca_pkt_t *ca_ni_netlink_rx_queue_get(pid_t *pid, ca_int_t *seq,  struct sk_buff **skb)
{
	ca_pkt_t *pkt;
	int count;

	if (ca_ni_debug & NI_DBG_NETLINK) {
		//printk("%s: rd_ptr=%d, wr_ptr=%d\n", __func__, netlink_rx_queue_rd_ptr, netlink_rx_queue_wr_ptr);
	}

	/* use spin_lock_bh() to avoid software interrupt here */
	spin_lock_bh(&ca_ni_netlink_rx_queue_lock);

	if (netlink_rx_queue_wr_ptr >= netlink_rx_queue_rd_ptr) {
		count = netlink_rx_queue_wr_ptr - netlink_rx_queue_rd_ptr;
	}
	else {
		count = NETLINK_RX_QUEUE_NUM - netlink_rx_queue_rd_ptr + netlink_rx_queue_wr_ptr;
	}
	if (count == NETLINK_RX_QUEUE_NUM) {
		netlink_rx_queue_is_full = 1;
	}
	else {
		netlink_rx_queue_is_full = 0;
	}
	if (count > 0) {
		if (ca_ni_debug & NI_DBG_NETLINK) {
			printk("%s: rd_ptr=%d, wr_ptr=%d\n", __func__, netlink_rx_queue_rd_ptr, netlink_rx_queue_wr_ptr);

		}

		*pid = netlink_rx_queue[netlink_rx_queue_rd_ptr].pid;
		*seq = netlink_rx_queue[netlink_rx_queue_rd_ptr].seq;
		*skb = netlink_rx_queue[netlink_rx_queue_rd_ptr].skb;
		pkt = &(netlink_rx_queue[netlink_rx_queue_rd_ptr].pkt);
		netlink_rx_queue_rd_ptr = CA_NI_RWPTR_ADVANCE_ONE(netlink_rx_queue_rd_ptr, NETLINK_RX_QUEUE_NUM);

		netlink_rxq_get_count++;

		spin_unlock_bh(&ca_ni_netlink_rx_queue_lock);
		return pkt;
	}
	spin_unlock_bh(&ca_ni_netlink_rx_queue_lock);
	return NULL;
}

static int ca_ni_netlink_rx_thread_entry(void *cookie)
{
	ca_status_t ret;
	pid_t pid;
	ca_int_t seq;
	ca_pkt_t *pkt;
	char *data;
	struct sk_buff *skb;

	printk("[NL_RX_THREAD]\n");

	while (1) {
		if (kthread_should_stop())
			break;
		set_current_state(TASK_INTERRUPTIBLE);
		cond_resched();
		pkt = ca_ni_netlink_rx_queue_get(&pid, &seq, &skb);
		if (pkt == NULL) {
			msleep_interruptible(1);        /* release CPU usage */
			continue;
		}

		if (ca_ni_debug & NI_DBG_NETLINK) {
			data = pkt->pkt_data->data;
			printk("%s: pkt=0x%p\n", __func__, pkt);
			printk("%s: pid=%d, seq=%ld, pkt=0x%p, data=0x%p\n", __func__, pid, seq, pkt, data);
			printk("%s: DA_MAC[%x-%x-%x-%x-%x-%x]\n", __func__, data[0], data[1], data[2], data[3], data[4], data[5]);
		}

		nextlink_rx_ack = 0;
		ret = ca_ni_netlink_rx_reply(pid, seq, pkt);

		/* only wait for reply when netlink_unicast send out the packet successfully */
		if (ret == CA_E_OK) {

			/* use a wait queue to wait the ACK event */
			{
				int wait_ret;

				//printk("%s: HZ=%d\n", __func__, HZ);
				/* timeout is 1s */
				if (ca_ni_debug & NI_DBG_NETLINK) {
					printk("%s: before add_wait_queue()..\n", __func__);
				}
				wait_ret = wait_event_interruptible_timeout(netlink_ack_wait_queue, nextlink_rx_ack == 1, 1 * HZ);
				if (wait_ret <= 1) {
					printk("%s: wait_ret=%d, ACK is lost...\n", __func__, wait_ret);
					netlink_sync_ack_lost_count++;
				}
			}
		}

		/* check whether there are other process use the same skb */
		/* like as ca-igmpsnooing and igmpproxy both use the same skb */
		if (netlink_rx_queue_wr_ptr != netlink_rx_queue_rd_ptr) {
			if (netlink_rx_queue[netlink_rx_queue_rd_ptr].skb != skb) {
				ca_dev_kfree_skb(skb);
				netlink_rx_free_skb_count++;
			}
		}
		else {
			/* free the skb here after packet has been netlink unicast to application layer */
			ca_dev_kfree_skb(skb);
			netlink_rx_free_skb_count++;
		}


	}
	printk("[NL_RX_THREAD-2]\n");
	return 0;
}

static struct task_struct *rx_thread;
ca_status_t ca_ni_netlink_rx_thread_create(void)
{
	ca_status_t ret = CA_E_OK;

	if (netlink_rx_ack) {
		rx_thread = kthread_create(ca_ni_netlink_rx_thread_entry, NULL, "NETLINK RX");
		if (IS_ERR(rx_thread)) {
			printk("%s: kthread_create() failed, ret=%d\n", __func__, ret);
			return CA_E_ERROR;
		}

		wake_up_process(rx_thread);
		printk("%s: =========kthread_create() OK, rx_thread=0x%p=======================\n", __func__, rx_thread);
	}
	return ret;
}
