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
 * ca_ni_dump.c: Common dump functions for debug
 *
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_arp.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmpv6.h>
#include <net/addrconf.h>
#include <net/ndisc.h>
#include <net/mld.h>

#include "aal_common.h"
#include "aal_l3_pe.h"
#include "ca_ni.h"
#include "ca_ni_common.h"
#include "ca_ni_proc.h"
#include "ca_ni_dump.h"

/* Parse and dump the L2 info from packet.
   This EtherType skips well-known VLAN tags, and assume there are at most two VLAN tags.

   Parameters:
   - data: packet contents. It must point to MAC_DA.
   - p_outer_tag: When function is returned, p_outer_tag points to the start address of outer VLAN tag in data.
                  Can be NULL if caller doesn't need it.
   - p_inner_tag: When function is returned, p_inner_tag points to the start address of inner VLAN tag in data.
                  Can be NULL if caller doesn't need it.
   - p_ethertype: When function is returned, p_ethertype points to the start address of EtherType in datga.
                  Can be NULL if caller doesn't need it.
   - p_payload: When function is returned, p_payload points to the L2 payload.
                Can be NULL if caller doesn't need it.
   - dump: Dump parsed part of packet or not.

   Return Value: The parsed EtherType in host order.
 */
uint16_t ca_ni_parse_l2(uint8_t *data, uint8_t **p_outer_tag, uint8_t **p_inner_tag, uint8_t **p_ethertype, uint8_t **p_payload, bool dump)
{
	uint8_t *cp;
	uint16_t ethertype = 0;

	/* default value if the field does not exist */
	if (p_outer_tag != NULL) *p_outer_tag = NULL;
	if (p_inner_tag != NULL) *p_inner_tag = NULL;
	if (p_ethertype != NULL) *p_ethertype = NULL;

	cp = data + 12;
	ethertype = (uint16_t) (*cp | ((uint16_t)(*(cp+1)) << 8));
	if ( (ethertype == ntohs(ETH_P_8021Q)) || (ethertype == ntohs(ETH_P_8021AD)) || (ethertype == ntohs(ETH_P_QINQ1)) ) {
		if (p_outer_tag != NULL) *p_outer_tag = cp;

		if (dump) {
			printk("Outer VLAN tag: %02x %02x %02x %02x [PCP: %u, DEI: %u, VID: %u]\n",
				*cp, *(cp+1), *(cp+2), *(cp+3),
				*(cp+2) >> 5, (*(cp+2) >> 4) & 0x1,
				((*(cp+2) & 0xF) << 8) + *(cp+3));
		}

		cp = cp + 4;
		ethertype = (uint16_t) (*cp | (*(cp+1) << 8));
	}
	if ( (ethertype == ntohs(ETH_P_8021Q)) || (ethertype == ntohs(ETH_P_8021AD)) || (ethertype == ntohs(ETH_P_QINQ1)) ) {
		if (p_inner_tag != NULL) *p_inner_tag = cp;

		if (dump) {
			printk("Inner VLAN tag: %02x %02x %02x %02x [PCP: %u, DEI: %u, VID: %u]\n",
				*cp, *(cp+1), *(cp+2), *(cp+3),
				*(cp+2) >> 5, (*(cp+2) >> 4) & 0x1,
				((*(cp+2) & 0xF) << 8) + *(cp+3));
		}

		cp = cp + 4;
		ethertype = (uint16_t) (*cp | (*(cp+1) << 8));
	}
	if (p_ethertype != NULL) *p_ethertype = cp;

	cp = cp + 2;
	if (p_payload != NULL) *p_payload = cp;

	if (dump)
		printk("EtherType: 0x%04x\n", htons(ethertype));

	return ethertype;
}

/* Parse and dump the PPP Discovery header.

   Parameters:
   - data: packet contents. It must point to PPP Discovery header.
   - p_payload: When function is returned, p_payload points to the payload of this PPP Discovery packet.
                Can be NULL if caller doesn't need it.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_ppp_discovery(uint8_t *data, uint8_t **p_payload, bool dump)
{
	struct pppoe_hdr *ph = (struct pppoe_hdr *) data;

	if (dump) {
		printk("PPP Discovery HDR:\n");
		printk("\t ver %u, type %u, code 0x%02x, session_id 0x%04x, length %u\n",
			ph->ver, ph->type, ph->code, ntohs(ph->sid), ntohs(ph->length));
	}

	if (p_payload != NULL) *p_payload = data + 6; /* bypass PPP Discovery header */
	return;
}

/* Parse and dump the PPP Session header.

   Parameters:
   - data: packet contents. It must point to PPP Session header.
   - p_payload: When function is returned, p_payload points to the payload of this PPP Session packet.
                Can be NULL if caller doesn't need it.
   - dump: Dump parsed part of packet or not.
 */
uint16_t ca_ni_parse_ppp_session(uint8_t *data, uint8_t **p_payload, bool dump)
{
	struct pppoe_hdr *ph = (struct pppoe_hdr *) data;

	if (dump) {
		printk("PPP Session HDR:\n");
		printk("\t ver %u, type %u, code 0x%02x, session_id 0x%04x, length %u, ppp_protocol 0x%04x\n",
			ph->ver, ph->type, ph->code, ntohs(ph->sid), ntohs(ph->length), ntohs(ph->tag[0].tag_type));
	}

	if (p_payload != NULL) *p_payload = data + PPPOE_SES_HLEN; /* bypass PPP Session header */
	return ph->tag[0].tag_type;
}

/* Parse and dump the IPv4 header.

   Parameters:
   - data: packet contents. It must point to IPv4 header.
   - p_payload: When function is returned, p_payload points to the payload of this IPv4 packet.
                Can be NULL if caller doesn't need it.
   - dump: Dump parsed part of packet or not.
   - is_frag: Return true if it is fragmented IPv4 packet.
              Can be NULL if caller doesn't need it.

   Return Value: The parsed Protocol of IPv4 header.
 */
uint8_t ca_ni_parse_ipv4(uint8_t *data, uint8_t **p_payload, bool dump, bool *is_frag)
{
	struct iphdr *iph = (struct iphdr *) data;

	if (dump) {
		printk("IPv4 HDR:\n");
		printk("\t ver %u, hdr_len %u, dscp %u, ecn %u, hdr_csum 0x%04x\n",
			iph->version, iph->ihl, iph->tos >> 2, iph->tos & 0x3, ntohs(iph->check));
		printk("\t total_len %u, id 0x%04x, DF %u, MF %u, FragOff %u, TTL %u, protocol 0x%02X\n",
			ntohs(iph->tot_len), ntohs(iph->id),
			(htons(iph->frag_off) & 0x4000) ? 1 : 0,
			(htons(iph->frag_off) & 0x2000) ? 1 : 0,
			htons(iph->frag_off) & 0x1FFF,
			iph->ttl, iph->protocol);
		printk("\t saddr %pI4\n", &iph->saddr);
		printk("\t daddr %pI4\n", &iph->daddr);
	}

	if (is_frag != NULL) {
		if (((htons(iph->frag_off) & 0x2000) != 0) || ((htons(iph->frag_off) & 0x1FFF) != 0))
			*is_frag = true;
		else
			*is_frag = false;
	}

	if (p_payload != NULL) *p_payload = data + iph->ihl * 4; /* bypass IPv4 header */

	return iph->protocol;
}

/* Parse and dump the IPv6 header.

   Parameters:
   - data: packet contents. It must point to IPv6 header.
   - p_payload: When function is returned, p_payload points to the payload of this IPv6 packet.
                Can be NULL if caller doesn't need it.
   - dump: Dump parsed part of packet or not.
   - is_frag: Return true if it is fragmented IPv6 packet.
              Can be NULL if caller doesn't need it.

   Return Value: The parsed Protocol of IPv6 header.
 */
uint8_t ca_ni_parse_ipv6(uint8_t *data, uint8_t **p_payload, bool dump, bool *is_frag)
{
	struct ipv6hdr *iph6 = (struct ipv6hdr *) data;
	uint8_t ret_nexthdr = iph6->nexthdr;
	uint8_t *cp = data;
	bool continue_nexthdr = true;

	if (dump) {
		printk("IPv6 HDR:\n");
		printk("\t ver %u, priority %u, payload_len %u, next_hdr 0x%02X, hop_limit %u\n",
			iph6->version, iph6->priority, ntohs(iph6->payload_len), iph6->nexthdr, iph6->hop_limit);
		printk("\t saddr %pI6\n", &iph6->saddr);
		printk("\t daddr %pI6\n", &iph6->daddr);
	}

	if (is_frag != NULL) *is_frag = false;
	cp = data + sizeof(struct ipv6hdr); /* bypass IPv6 header */
	while (continue_nexthdr) {
		switch (ret_nexthdr) {
		case NEXTHDR_HOP:
		case NEXTHDR_ROUTING:
		case NEXTHDR_DEST:
			if (dump) printk("ExtHdr type %u: HdrExtLen %u, next_hdr %u\n", ret_nexthdr, *(cp+1), *cp);
			ret_nexthdr = *cp;
			cp = cp + 8 + *(cp+1) * 8;
			break;
		case NEXTHDR_FRAGMENT:
			if (dump) printk("ExtHdr type %u: next_hdr %u\n", ret_nexthdr, *cp);
			ret_nexthdr = *cp;
			cp = cp + 8;
			if (is_frag != NULL) *is_frag = true;
			break;
		case NEXTHDR_AUTH:
			if (dump) printk("ExtHdr type %u: HdrExtLen %u, next_hdr %u\n", ret_nexthdr, *(cp+1), *cp);
			ret_nexthdr = *cp;
			cp = cp + 8 + *(cp+1) * 4;
			break;
		case NEXTHDR_NONE:
			if (dump) printk("ExtHdr type %u: No Next Header\n", ret_nexthdr);
			continue_nexthdr = false;
			break;
		default:
			continue_nexthdr = false;
			break;
		}
	}

	if (p_payload != NULL) *p_payload = cp;

	return ret_nexthdr;
}

/* Parse and dump the ARP header.

   Parameters:
   - data: packet contents. It must point to ARP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_arp(uint8_t *data, bool dump)
{
	struct arphdr *arp = (struct arphdr *) data;
	unsigned char *cp = (unsigned char *) arp + sizeof(struct arphdr);
	int i;

	/* whole function is for dumping, and there is no more payload for parsing */
	if (!dump) return;

	printk("ARP HDR:\n");
	printk("\t hw_type %u, protocol 0x%04X, hw_addr_size %u, protocol_size %u, opcode %u ",
		ntohs(arp->ar_hrd), ntohs(arp->ar_pro), arp->ar_hln, arp->ar_pln, ntohs(arp->ar_op));
	switch (ntohs(arp->ar_op)) {
	case ARPOP_REQUEST: printk("[ARP Request]\n"); break;
	case ARPOP_REPLY: printk("[ARP Reply]\n"); break;
	case ARPOP_RREQUEST: printk("[RARP Request]\n"); break;
	case ARPOP_RREPLY: printk("[RARP Reply]\n"); break;
	default: printk("\n"); break;
	}

	if (arp->ar_pro == htons(ETH_P_IP)) {
		printk("\t Sender HW addr ");
		for (i = 0; i < arp->ar_hln - 1; i++) {
			printk("%02x-", *cp++);
		}
		printk("%02x, ", *cp++);

		printk("IP addr %pI4\n", cp);
		cp = cp + 4;

		printk("\t Target HW addr ");
		for (i = 0; i < arp->ar_hln - 1; i++) {
			printk("%02x-", *cp++);
		}
		printk("%02x, ", *cp++);

		printk("IP addr %pI4\n", cp);
		cp = cp + 4;
	}
}

/* Parse and dump the MACSec header.

   Parameters:
   - data: packet contents. It must point to MACSec header (SecTAG).
   - dump: Dump parsed part of packet or not.
 */
uint16_t ca_ni_parse_macsec(uint8_t *data, uint8_t **p_payload, bool dump)
{
	struct macsec_header *hdr = (struct macsec_header *) data;
	unsigned char *cp = (unsigned char *) hdr + sizeof(struct macsec_header);
	uint16_t inner_ethertype = 0;

	/* whole function is for dumping, and there is no more payload for parsing */
	if (dump) {
		printk("MACSec HDR (SecTAG):\n");
		printk("\t TCI 0x%02x, AN 0x%x\n", hdr->tci_an >> 2, hdr->tci_an & MACSEC_AN_MASK);
		printk("\t   [VER %u, ES %u, SC %u, SCB %u, E %u, C %u]\n",
				(hdr->tci_an & MACSEC_TCI_VERSION) >> 7,
				(hdr->tci_an & MACSEC_TCI_ES) >> 6,
				(hdr->tci_an & MACSEC_TCI_SC) >> 5,
				(hdr->tci_an & MACSEC_TCI_SCB) >> 4,
				(hdr->tci_an & MACSEC_TCI_E) >> 3,
				(hdr->tci_an & MACSEC_TCI_C)>> 2);
		printk("\t ShortLength %u, PacketNumber %u\n", hdr->short_length, ntohl(hdr->packet_number));

		if (hdr->tci_an & MACSEC_TCI_SC) {
			printk("\t SystemIdentifier %02x-%02x-%02x-%02x-%02x-%02x, PortIdentifier %02x-%02x (%u)\n",
					hdr->secure_channel_id[0], hdr->secure_channel_id[1], hdr->secure_channel_id[2],
					hdr->secure_channel_id[3], hdr->secure_channel_id[4], hdr->secure_channel_id[5],
					hdr->secure_channel_id[6], hdr->secure_channel_id[7],
					(hdr->secure_channel_id[6] << 8) + hdr->secure_channel_id[7]);
		}

	}

	if ((hdr->tci_an & MACSEC_TCI_SC) == 0) {
		/* no SCI field */
		cp = cp - 8;
	}

	if ((hdr->tci_an & MACSEC_TCI_E) == 0) {
		/* no encryption; inner EtherType can be parsed */
		inner_ethertype = (uint16_t) (*cp | ((uint16_t)(*(cp+1)) << 8));
		cp = cp + 2;

		if (dump)
			printk("Inner EtherType: 0x%04x\n", htons(inner_ethertype));
	}

	if (p_payload != NULL) *p_payload = cp;

	return inner_ethertype;
}

/* Parse and dump the ICMP header.

   Parameters:
   - data: packet contents. It must point to ICMP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_icmp(uint8_t *data, bool dump)
{
	struct icmphdr *icmph = (struct icmphdr *) data;

	if (dump) {
		printk("ICMP HDR:\n");
		printk("\t type %u, code %u, id 0x%04x, seq %u\n",
			icmph->type, icmph->code, ntohs(icmph->un.echo.id), ntohs(icmph->un.echo.sequence));
	}
}

/* Parse and dump the IGMP header.

   Parameters:
   - data: packet contents. It must point to IGMP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_igmp(uint8_t *data, bool dump)
{
	struct igmphdr *ih = (struct igmphdr *) data;

	if (dump) {
		printk("IGMP HDR:\n");
		printk("\t type %u, code %u, group %pI4\n", ih->type, ih->code, &ih->group);
	}
}

/* Parse and dump the TCP header.

   Parameters:
   - data: packet contents. It must point to TCP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_tcp(uint8_t *data, uint8_t **p_payload, uint16_t *l4_sport, uint16_t *l4_dport, bool dump)
{
	struct tcphdr *th = (struct tcphdr *) data;

	if (dump) {
		printk("TCP HDR:\n");
		printk("\t sport %u, dport %u, seq %u, ack_seq %u, csum 0x%04x\n",
				ntohs(th->source), ntohs(th->dest), ntohl(th->seq), ntohl(th->ack_seq), ntohs(th->check));
		printk("\t cwr %u, ece %u, urg %u, ack %u, psh %u, rst %u, syn %u, fin %u\n",
			th->cwr, th->ece, th->urg, th->ack, th->psh, th->rst, th->syn, th->fin);
	}

	if (l4_sport != NULL) *l4_sport = ntohs(th->source);
	if (l4_dport != NULL) *l4_dport = ntohs(th->dest);
	if (p_payload != NULL) *p_payload = data + sizeof(struct tcphdr);
}

/* Parse and dump the UDP header.

   Parameters:
   - data: packet contents. It must point to UDP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_udp(uint8_t *data, uint8_t **p_payload, uint16_t *l4_sport, uint16_t *l4_dport, bool dump)
{
	struct udphdr *uh = (struct udphdr *) data;

	if (dump) {
		printk("UDP HDR:\n");
		printk("\t sport %u, dport %u, len %u, csum 0x%04x\n",
			ntohs(uh->source), ntohs(uh->dest), ntohs(uh->len), ntohs(uh->check));
	}

	if (l4_sport != NULL) *l4_sport = ntohs(uh->source);
	if (l4_dport != NULL) *l4_dport = ntohs(uh->dest);
	if (p_payload != NULL) *p_payload = data + sizeof(struct udphdr);
}

/* Parse and dump the ESP header.

   Parameters:
   - data: packet contents. It must point to ESP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_esp(uint8_t *data, bool dump)
{
	struct ip_esp_hdr *esph = (struct ip_esp_hdr *) data;

	if (dump) {
		printk("ESP HDR:\n");
		printk("\t spi 0x%08x, seq_no %u\n", ntohl(esph->spi), ntohl(esph->seq_no));
	}
}

/* Parse and dump the Authentication Header.

   Parameters:
   - data: packet contents. It must point to Authentication Header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_ah(uint8_t *data, bool dump)
{
	struct ip_auth_hdr *ah = (struct ip_auth_hdr *) data;
	unsigned char *cp = (unsigned char *) ah + 12; /* point to ICV directly */
	int i;

	/* whole function is for dumping, and there is no more payload for parsing */
	if (!dump) return;

	printk("AH HDR:\n");
	printk("\t nexthdr 0x%02x, hdr_len %u (%u bytes), spi 0x%08x, seq_no %u\n",
		ah->nexthdr, ah->hdrlen, (ah->hdrlen + 2) * 4, ntohl(ah->spi), ntohl(ah->seq_no));

	printk("\t icv ");
	for (i = 0; i < (ah->hdrlen + 2) * 4 - 12; i++) {
		printk("%02x ", *cp++);
	}
	printk("\n");
}

/* Parse and dump the ICMPv6 header.

   Parameters:
   - data: packet contents. It must point to ICMPv6 header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_icmpv6(uint8_t *data, int len, bool dump)
{
	struct icmp6hdr *ih = (struct icmp6hdr *) data;
	struct nd_opt_hdr *opt = NULL;
	unsigned char *cp = (unsigned char *) data;
	int remain = 0;

	if (dump) {
		printk("ICMPv6 HDR:\n");
		printk("\t type %u, code %u, checksum 0x%04x\n", ih->icmp6_type, ih->icmp6_code, ntohs(ih->icmp6_cksum));
	}

	if ( (ih->icmp6_type == ICMPV6_ECHO_REQUEST) || (ih->icmp6_type == ICMPV6_ECHO_REPLY) ) {

		if (dump)
			printk("\t id 0x%04x, sequence 0x%04x\n", ntohs(ih->icmp6_identifier), ntohs(ih->icmp6_sequence));

		remain = len - sizeof(struct icmp6hdr);
		cp = data + sizeof(struct icmp6hdr);

	} else if ((ih->icmp6_type >= ICMPV6_MGM_QUERY) && (ih->icmp6_type <= ICMPV6_MGM_REDUCTION)) {

		struct mld_msg *msg = (struct mld_msg *) data;

		if (dump)
			printk("\t mld_maxdelay %u, mcast_addr %pI6\n", ntohs(msg->mld_maxdelay), &msg->mld_mca);

		remain = len - sizeof(struct mld_msg);
		cp = data + sizeof(struct mld_msg);

	} else if ((ih->icmp6_type == NDISC_NEIGHBOUR_SOLICITATION) || (ih->icmp6_type == NDISC_NEIGHBOUR_ADVERTISEMENT)) {

		struct nd_msg *msg = (struct nd_msg *) data;

		if (dump) {
			printk("\t router %u, solicited %u, override %u\n", ih->icmp6_router, ih->icmp6_solicited, ih->icmp6_override);
			printk("\t target %pI6\n", &msg->target);
		}

		remain = len - sizeof(struct nd_msg); /* struct nd_msg contains a byte of opt */
		cp = data + sizeof(struct nd_msg); /* struct nd_msg contains a byte of opt */

	} else if (ih->icmp6_type == NDISC_ROUTER_SOLICITATION) {

		remain = len - sizeof(struct ra_msg);
		cp = data + sizeof(struct ra_msg);

	} else if (ih->icmp6_type == NDISC_ROUTER_ADVERTISEMENT) {

		struct ra_msg *msg = (struct ra_msg *) data;

		if (dump) {
			printk("\t cur_hop_limit %u, managed %u, other %u, router_pref %u\n",
				ih->icmp6_hop_limit, ih->icmp6_addrconf_managed, ih->icmp6_addrconf_other, ih->icmp6_router_pref);
			printk("\t lifetime %u, reachable_time %u, retrans_timer %u\n",
				ntohs(ih->icmp6_rt_lifetime), ntohl(msg->reachable_time), ntohl(msg->retrans_timer));
		}

		remain = len - sizeof(struct ra_msg);
		cp = data + sizeof(struct ra_msg);

	}

	while (remain >= 8) { /* minimum size of one icmpv6 option */
		opt = (struct nd_opt_hdr *) cp;

		if ((opt->nd_opt_type == 0) || (opt->nd_opt_len == 0))
			break;
		if (dump)
			printk("\t opt_type %u, opt_len %u", opt->nd_opt_type, opt->nd_opt_len);

		switch (opt->nd_opt_type) {
		case ND_OPT_SOURCE_LL_ADDR:
		case ND_OPT_TARGET_LL_ADDR:
			if (dump) printk(", link-layer addr %pM\n", cp + 2);
			break;
		case ND_OPT_PREFIX_INFO:
			if (dump) {
				printk(", prefix_len %u, on-link %u, auto addr-conf %u\n",
					*(cp + 2),
					((*(cp + 3)) & 0x80) ? 1 : 0,
					((*(cp + 3)) & 0x40) ? 1 : 0);
				printk("\t\t valid_lifetime %u, preferred_lifetime %u, prefix %pI6\n",
					ntohl(*(unsigned int *)(cp + 4)), ntohl(*(unsigned int *)(cp + 8)), cp + 16);
			}
			break;
		case ND_OPT_REDIRECT_HDR:
			if (dump) printk("\n");
			break;
		case ND_OPT_MTU:
			if (dump) printk(", mtu %u\n", ntohl(*(unsigned int *)(cp + 4)));
			break;
		default:
			if (dump) printk("\n");
			break;
		}

		cp = cp + opt->nd_opt_len * 8;
		remain = remain - opt->nd_opt_len * 8;
	}
}

/* Parse and dump the UDP-Lite header.

   Parameters:
   - data: packet contents. It must point to UDP-Lite header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_udplite(uint8_t *data, bool dump)
{
	struct udphdr *uh = (struct udphdr *) data;
	bool illegal_coverage = false;

	if ((ntohs(uh->len) > 0) && (ntohs(uh->len) < 8))
		illegal_coverage = true;

	if (dump) {
		printk("UDP-Lite HDR:\n");
		printk("\t sport %u, dport %u, csum_coverage %u%s, csum 0x%04x\n",
			ntohs(uh->source), ntohs(uh->dest), ntohs(uh->len), (illegal_coverage ? " [ILLEGAL]" : ""), ntohs(uh->check));
	}
}

/* return the string of PTP message type */
static const char *ptp_msg_type_string(int type)
{
	switch (type) {
	case PTP_SYNC:
		return "SYNC";
	case PTP_DELAY_REQ:
		return "DELAY_REQ";
	case PTP_PDELAY_REQ:
		return "PDELAY_REQ";
	case PTP_PDELAY_RESP:
		return "PDELAY_RESP";
	case PTP_FOLLOW_UP:
		return "FOLLOW_UP";
	case PTP_DELAY_RESP:
		return "DELAY_RESP";
	case PTP_PDELAY_RESP_FOLLOW_UP:
		return "PDELAY_RESP_FOLLOW_UP";
	case PTP_ANNOUNCE:
		return "ANNOUNCE";
	case PTP_SIGNALING:
		return "SIGNALING";
	case PTP_MANAGEMENT:
		return "MANAGEMENT";
	}
	return "UNKNOWN";
}

/* return the string of PTP control field */
static const char *ptp_control_field_string(int control)
{
	switch (control) {
	case PTP_CTL_SYNC:
		return "Sync Message";
	case PTP_CTL_DELAY_REQ:
		return "Delay_Req Message";
	case PTP_CTL_FOLLOW_UP:
		return "Follow_Up Message";
	case PTP_CTL_DELAY_RESP:
		return "Delay_Resp Message";
	case PTP_CTL_MANAGEMENT:
		return "Management Message";
	case PTP_CTL_OTHER:
		return "Other Message";
	}
	return "Unknown Message";
}

/* Parse and dump the IEEE 1588 / PTP (Precision Time Protocol) header.

   Parameters:
   - data: packet contents. It must point to PTP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_ptp(uint8_t *data, bool dump)
{
	struct ptp_header *ph = (struct ptp_header *) data;

	if (dump) {
		printk("PTP HDR:\n");
		printk(" \t transportSpecific 0x%x, msg_type 0x%x [%s]\n",
			ph->transport_specific, ph->msg_type, ptp_msg_type_string(ph->msg_type));
		printk(" \t version_ptp %u, msg_len %u, domain_num %u\n", ph->version_ptp, ntohs(ph->msg_len), ph->domain_num);
		printk(" \t flag 0x%04x\n", ntohs(ph->flag));
		printk(" \t correctionNs %lld ns, correctionSubNs %lld ns\n",
			be64_to_cpu(ph->correction_field) >> 16, be64_to_cpu(ph->correction_field) & 0xFFFF);
		printk(" \t clock_id %02x %02x %02x %02x %02x %02x %02x %02x\n",
			ph->clock_id[0], ph->clock_id[1], ph->clock_id[2], ph->clock_id[3],
			ph->clock_id[4], ph->clock_id[5], ph->clock_id[6], ph->clock_id[7]);
		printk(" \t source_port_id %u, seq_id %u, log_msg_interval %u, control_field %u [%s]\n",
			ntohs(ph->source_port_id), ntohs(ph->seq_id), ph->log_msg_interval, ph->control_field, ptp_control_field_string(ph->control_field));

		/* TODO: Support PTPv1, and parse more header contents of different PTP message type */
	}
}

/* Parse and dump the RDP (Reliable Data Protocol) header.

   Parameters:
   - data: packet contents. It must point to RDP header.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_rdp(uint8_t *data, bool dump)
{
	if (dump) {
		printk("RDP HDR:\n");
		/* TODO: not popular protocol, dump in the future */
	}
}

void ca_ni_parse_tcp_udp(uint8_t *data, uint16_t l4_sport, uint16_t l4_dport, bool dump)
{
	if ((l4_sport == 319) || (l4_dport == 319) || (l4_sport == 320) || (l4_dport == 320)) {
		ca_ni_parse_ptp(data, dump);
	}
}

/* Dump Header_A contents.

   Parameters:
   - is_rx: Set true if hdr_a comes from Rx packet.
         If true and packet is PTP, bit[63:32] of hdr_a is treated as PTP info.
         If false and packet is PTP, bit[63:32] of hdr_a is treated as PTP cmd.
 */
void ca_ni_dump_hdr_a(HEADER_A_T *hdr_a, bool is_rx)
{
	uint8_t mcgid_lrn_encoding;

	printk("HEADER A:\n");
	printk("\t cos(3) %u, ldpid/mcidx(6) 0x%02x, lspid(6) 0x%02x, pkt_size %u, fe_bypass %u, hdr_type %u ",
		hdr_a->bits.cos, hdr_a->bits.ldpid, hdr_a->bits.lspid, hdr_a->bits.pkt_size, hdr_a->bits.fe_bypass, hdr_a->bits.hdr_type);
	switch (hdr_a->bits.hdr_type) {
	case HEADER_TYPE_UC:
		printk("[Generic UC HDR]\n");
		break;
	case HEADER_TYPE_MC:
		printk("[MC HDR, MCGID is valid]\n");
		break;
	case HEADER_TYPE_CPU_BOUND:
		printk("[CPU bound packet, MCGID contains L2 learning info]\n");
		break;
	case HEADER_TYPE_PTP:
		printk("[PTP packet]\n");
		break;
	}

	if (hdr_a->bits.hdr_type == HEADER_TYPE_PTP) {

		if (is_rx) {
			printk("\t ptp_info 0x%08x, cpu_flg %u\n", hdr_a->bits.bits_32_63.ptp_info.ptp_info, hdr_a->bits.bits_32_63.ptp_info.cpu_flg);
		} else {
			printk("\t cmd_asel(2) %u, cmd_op(2) %u ", hdr_a->bits.bits_32_63.ptp_cmd.cmd_asel, hdr_a->bits.bits_32_63.ptp_cmd.cmd_op);
			switch (hdr_a->bits.bits_32_63.ptp_cmd.cmd_op) {
			case 0: printk("[NO_OP]"); break;
			case 1: printk("[TS_CORRECT]"); break;
			case 2: printk("[TS_P_OFFSET]"); break;
			case 3: printk("[TS_N_OFFSET]"); break;
			}
			printk(", cmd_mode %u ", hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode);
			switch (hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode) {
			case 0: printk("[Timestamp mode]"); break;
			case 1: printk("[Correction mode]"); break;
			}
			printk("\n");

			printk("\t ptp_type(2) %u ", hdr_a->bits.bits_32_63.ptp_cmd.ptp_type);
			switch (hdr_a->bits.bits_32_63.ptp_cmd.ptp_type) {
			case 0: printk("[PTP over Ethernet]"); break;
			case 1: printk("[CFM]"); break;
			case 2: printk("[PTP over UDP over IPv4]"); break;
			case 3: printk("[PTP over UDP over IPv6]"); break;
			}
			printk(", cmd_offset(7) 0x%02x, udp_cksum_offset(7) 0x%02x, cpu_flg %u\n",
					hdr_a->bits.bits_32_63.ptp_cmd.cmd_offset, hdr_a->bits.bits_32_63.ptp_cmd.udp_cksum_offset,
					hdr_a->bits.bits_32_63.ptp_cmd.cpu_flg);
		}

	} else {

			printk("\t rx_pkt_type(2) %u ", hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type);
			switch (hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type) {
			case 0: printk("[UC or Disable L2 Learning]\n"); break;
			case 1: printk("[BC]\n"); break;
			case 2: printk("[MC]\n"); break;
			case 3: printk("[Unknown-UC or Enable L2 Learning]\n"); break;
			}

			printk("\t mcgid(8) 0x%02x ", hdr_a->bits.bits_32_63.pkt_info.mcgid);
			if (hdr_a->bits.hdr_type == HEADER_TYPE_CPU_BOUND) {
				/* mcgid[7:5] is debug encoding that interpret mcgid[4:0] contents */
				mcgid_lrn_encoding = (hdr_a->bits.bits_32_63.pkt_info.mcgid >> 5) & 0x7;
				if (mcgid_lrn_encoding == 1) {
					printk("[Unknown Redirection]");
					switch (hdr_a->bits.bits_32_63.pkt_info.mcgid & 0xF) {
					case 0: printk("[Known UC]"); break;
					case 1: printk("[Known MC]"); break;
					case 2: printk("[Unknown UC]"); break;
					case 3: printk("[Unknown MC]"); break;
					case 4: printk("[BC]"); break;
					default: printk("[Unknown pkt_fwd_type 0x%x]", hdr_a->bits.bits_32_63.pkt_info.mcgid & 0xF); break;
					}
				} else if (mcgid_lrn_encoding == 2) {
					printk("[Drop Redirection][Drop Source 0x%02x]", hdr_a->bits.bits_32_63.pkt_info.mcgid & 0x1F);
				} else if (mcgid_lrn_encoding == 3) {
					printk("[Learning Info]");
					if (hdr_a->bits.bits_32_63.pkt_info.mcgid & 0x1) printk("[sw_new_lrn_flg]");
					if (hdr_a->bits.bits_32_63.pkt_info.mcgid & 0x2) printk("[lrn_clsn_flg]");
					if (hdr_a->bits.bits_32_63.pkt_info.mcgid & 0x4) printk("[lrn_lmt_excd_flag]");
				} else {
					printk("[No L2 Learning Info]");
				}
			}
			printk("\n");

	}

	printk("\t drop_code(3) %u, no_drop %u, mirror %u, mark %u\n",
			hdr_a->bits.bits_32_63.pkt_info.drop_code, hdr_a->bits.bits_32_63.pkt_info.no_drop,
			hdr_a->bits.bits_32_63.pkt_info.mirror, hdr_a->bits.bits_32_63.pkt_info.mark);

	printk("\t deep_q %u, cpu_flg %u, pol_id(9) 0x%03x, pol_grp_id(3) %x, pol_en %u ",
			hdr_a->bits.bits_32_63.pkt_info.deep_q, hdr_a->bits.bits_32_63.pkt_info.cpu_flg,
			hdr_a->bits.bits_32_63.pkt_info.pol_id, hdr_a->bits.bits_32_63.pkt_info.pol_grp_id,
			hdr_a->bits.bits_32_63.pkt_info.pol_en);
	switch (hdr_a->bits.bits_32_63.pkt_info.pol_en) {
		case 0: printk("[Disabled]\n"); break;
		case 1: printk("[L2 Enabled]\n"); break;
		case 2: printk("[L3 Enabled]\n"); break;
		case 3: printk("[L2 & L3 Enabled]\n"); break;
	}
}

/* Parse and dump the packets from L2 to L4.

   Parameters:
   - data: packet contents. It must point to MAC_DA.
   - dat_end: Point to end of packet in 'skb->data'. Not include skb in frag_list.
   - dump: Dump parsed part of packet or not.
 */
void ca_ni_parse_pkts(uint8_t *data, uint8_t *data_end, bool dump)
{
	unsigned char *cp = data;
	uint16_t ethertype = 0;
	uint8_t ip_prot = 0xff;
	uint16_t l4_sport = 0, l4_dport = 0;

	ethertype = ca_ni_parse_l2(cp, NULL, NULL, NULL, &cp, dump);

	switch (ntohs(ethertype)) {
	case ETH_P_PPP_DISC:
		if (cp + sizeof(struct pppoe_hdr) <= data_end)
			ca_ni_parse_ppp_discovery(cp, &cp, dump);
		break;
	case ETH_P_PPP_SES:
		if (cp + sizeof(struct pppoe_hdr) <= data_end)
			ethertype = ca_ni_parse_ppp_session(cp, &cp, dump);
		break;
	case ETH_P_MACSEC:
		if (cp + sizeof(struct macsec_header) <= data_end)
			ethertype = ca_ni_parse_macsec(cp, &cp, dump);
		break;
	case ETH_P_1588:
		ca_ni_parse_ptp(cp, dump);
		break;
	}

	switch (ntohs(ethertype)) {
	case ETH_P_IP:
	case PPP_IP:
		if (cp + sizeof(struct iphdr) <= data_end)
			ip_prot = ca_ni_parse_ipv4(cp, &cp,  dump, NULL);
		break;
	case ETH_P_IPV6:
	case PPP_IPV6:
		if (cp + sizeof(struct ipv6hdr) <= data_end)
			ip_prot = ca_ni_parse_ipv6(cp, &cp, dump, NULL);
		break;
	case ETH_P_ARP:
		if (cp + sizeof(struct arphdr) <= data_end)
			ca_ni_parse_arp(cp, dump);
		break;
	}

	switch (ip_prot) {
	case IPPROTO_ICMP:
		if (cp + sizeof(struct icmphdr) <= data_end)
			ca_ni_parse_icmp(cp, dump);
		break;
	case IPPROTO_IGMP:
		if (cp + sizeof(struct igmphdr) <= data_end)
			ca_ni_parse_igmp(cp, dump);
		break;
	case IPPROTO_TCP:
		if (cp + sizeof(struct tcphdr) <= data_end)
			ca_ni_parse_tcp(cp, &cp, &l4_sport, &l4_dport, dump);
		break;
	case IPPROTO_UDP:
		if (cp + sizeof(struct udphdr) <= data_end)
			ca_ni_parse_udp(cp, &cp, &l4_sport, &l4_dport, dump);
		break;
	case IPPROTO_ESP:
		if (cp + sizeof(struct ip_esp_hdr) <= data_end)
			ca_ni_parse_esp(cp, dump);
		break;
	case IPPROTO_AH:
		if (cp + sizeof(struct ip_auth_hdr) <= data_end)
			ca_ni_parse_ah(cp, dump);
		break;
	case IPPROTO_ICMPV6:
		if (cp + sizeof(struct icmp6hdr) <= data_end)
			ca_ni_parse_icmpv6(cp, data_end - cp, dump);
		break;
	case IPPROTO_UDPLITE:
		if (cp + sizeof(struct udphdr) <= data_end)
			ca_ni_parse_udplite(cp, dump);
		break;
	case 27: /* RDPv1, RDPv2 */
		ca_ni_parse_rdp(cp, dump);
		break;
	}

	if ((l4_sport != 0) && (l4_dport != 0)) {
		ca_ni_parse_tcp_udp(cp, l4_sport, l4_dport, dump);
	}
}

#if defined(CONFIG_NE_TX_LLID_MAP)
/* Parse find the source IP address of the packet data

   Parameters:
   - data: packet contents. It must point to MAC_DA.
   - dat_end: Point to end of packet in 'skb->data'. Not include skb in frag_list.
   - source_ip: source ip of incoming packet.
 */
ca_status_t ca_ni_find_source_ip(uint8_t *data, uint8_t *data_end, ca_uint32_t *source_ip)
{
	int i;
	unsigned char *cp = data;
	uint16_t ethertype = 0;

	ethertype = ca_ni_parse_l2(cp, NULL, NULL, NULL, &cp, 0);

	if (ntohs(ethertype) == ETH_P_ARP) {
		if (cp + sizeof(struct arphdr) <= data_end) {
			struct arphdr *arp = (struct arphdr *) cp;
			unsigned char *cp = (unsigned char *) arp + sizeof(struct arphdr);
			if (arp->ar_pro == htons(ETH_P_IP)) {
				//printk("%s: arp->ar_hln=%d, cp=0x%p\n", __func__, arp->ar_hln, cp);
				for (i = 0; i < arp->ar_hln - 1; i++) {
					cp++;
				}
				cp++;

				*source_ip = *(ca_uint32_t *)cp;
				//printk("%s: ARP packet, source_ip=0x%x\n", __func__, *source_ip);
				return CA_E_OK;
			}
			return CA_E_ERROR;
		}
		return CA_E_ERROR;
	}

	if (ntohs(ethertype) == ETH_P_IP) {
		if (cp + sizeof(struct iphdr) <= data_end) {
			struct iphdr *iph = (struct iphdr *) cp;
			*source_ip = iph->saddr;
			//printk("%s: IPv4 packet, source_ip=0x%x\n", __func__, *source_ip);
		}
		return CA_E_ERROR;
}

	return CA_E_ERROR;
}
#endif

void ca_ni_dump_rx_pkts(struct sk_buff *skb, HEADER_A_T *hdr_a, HEADER_CPU_T *hdr_cpu, HEADER_PTP_T *hdr_ptp)
{
	struct sk_buff *skbinlist;
	l3pe_port_egress_behavior_t behavior[4];
	uint8_t cpu_cmd;
	uint8_t * payload_ptr;
	uint32_t total_len;
	/*
	 * because skb has been called eth_type_trans() before dump
	 * so the skb->len will be decrease ETH_HLEN and skb->data has been pull ETH_HLEN too.
	 */
#ifdef CONFIG_RTK_NIC_RX_HOOK
	payload_ptr = skb->data;
	total_len = skb->len;
#else
	payload_ptr = skb->data - ETH_HLEN;
	total_len = skb->len + ETH_HLEN;
#endif

	printk("====== RX skb data@%p, total_len %u ======\n", payload_ptr, total_len);

	/*** header A ***/

	ca_ni_dump_hdr_a(hdr_a, true);

	/*** header CPU ***/

	if (hdr_a->bits.bits_32_63.pkt_info.cpu_flg != 0) {

		if ((hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type == 1) && (hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type == 2)) {
			/* Multicast or Broadcast */
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
			aal_l3pe_port_egress_behavior_get(0, 0, &(behavior[0]));	// dedicated behavior for MC
#endif
			cpu_cmd = behavior[0].cpu_cmd;
		} else {
			/* Unicast */
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
			aal_l3pe_port_egress_behavior_get(0, 1, &(behavior[1]));
			aal_l3pe_port_egress_behavior_get(0, 2, &(behavior[2]));
			aal_l3pe_port_egress_behavior_get(0, 3, &(behavior[3]));	// default behavior
#endif
			if (hdr_a->bits.ldpid == behavior[1].ldpid)
				cpu_cmd = behavior[1].cpu_cmd;
			else if (hdr_a->bits.ldpid == behavior[2].ldpid)
				cpu_cmd = behavior[2].cpu_cmd;
			else
				cpu_cmd = behavior[3].cpu_cmd;
		}

		printk("HEADER_CPU:\n");
		printk("\t raw_data(64) 0x%016llx\n", hdr_cpu->bits64);

		if (cpu_cmd == 1) {
			printk("\t mdata_h(32) 0x%08x, mdata_l(32) 0x%08x\n", hdr_cpu->mdata_raw.mdata_h, hdr_cpu->mdata_raw.mdata_l);
			printk("\t   [l3_ingress_if_id(8) 0x%02x, l3_egress_if_id(8) 0x%02x, lpm_result(12) 0x%03x]\n",
				hdr_cpu->mdata_uc.l3_ingress_if_id, hdr_cpu->mdata_uc.l3_egress_if_id, hdr_cpu->mdata_uc.lpm_result);

		} else if (cpu_cmd == 2) {

			printk("\t l3_offset(7) 0x%02x, l4_offset(7) 0x%02x, l2l3_flag(5) 0x%02x ",
				hdr_cpu->hdr_i_info.l3_offset, hdr_cpu->hdr_i_info.l4_offset, hdr_cpu->hdr_i_info.l2l3_flag);
			if (hdr_cpu->hdr_i_info.l2l3_flag & 0x1)
				printk("[Outer Tag]");
			if (hdr_cpu->hdr_i_info.l2l3_flag & 0x2)
				printk("[Innter Tag]");
			if (hdr_cpu->hdr_i_info.l2l3_flag & 0x4)
				printk("[IPv4]");
			if (hdr_cpu->hdr_i_info.l2l3_flag & 0x8)
				printk("[IPv6]");
			if (hdr_cpu->hdr_i_info.l2l3_flag & 0x10)
				printk("[PPPoE]");
			printk("\n");

			printk("\t cls_action(12) 0x%03x, hash_idx(21) 0x%05x, hash_dbl_chk_fail(3) %x ",
				hdr_cpu->hdr_i_info.cls_action, hdr_cpu->hdr_i_info.hash_idx, hdr_cpu->hdr_i_info.hash_dbl_chk_fail);
			if (hdr_cpu->hdr_i_info.hash_dbl_chk_fail & 0x1) {
				printk(KERN_INFO "[Hash Engine double check fail (type-0)]");
			}
			if (hdr_cpu->hdr_i_info.hash_dbl_chk_fail & 0x2) {
				printk(KERN_INFO "[Hash Engine double check fail (type-1)]");
			}
			if (hdr_cpu->hdr_i_info.hash_dbl_chk_fail & 0x4) {
				printk(KERN_INFO "[HashLite Engine double check fail]");
			}
			printk("\n");

			printk("\t fwd_vld(9) 0x%03x\n", hdr_cpu->hdr_i_info.fwd_vld);
			printk("\t   [CLS: %s]\n", (hdr_cpu->hdr_i_info.fwd_vld & 0x1) ? "Hit" : "NO Hit");
			printk("\t   [Hash(type-0): ");
			switch ((hdr_cpu->hdr_i_info.fwd_vld >> 1) & 0x3) {
			case 0: printk("NO Hit"); break;
			case 1: printk("Hit Overflow Entry"); break;
			case 2: printk("Hit Cache Hash Entry"); break;
			case 3: printk("Hit Main Hash Entry"); break;
			default: printk("UNKNOWN"); break;
			}
			printk(", Hash(type-1): ");
			switch ((hdr_cpu->hdr_i_info.fwd_vld >> 3) & 0x3) {
			case 0: printk("NO Hit"); break;
			case 1: printk("Hit Overflow Entry"); break;
			case 2: printk("Hit Cache Hash Entry"); break;
			case 3: printk("Hit Main Hash Entry"); break;
			default: printk("UNKNOWN"); break;
			}
			printk("]\n");
			printk("\t   [LPM: %s]\n", (hdr_cpu->hdr_i_info.fwd_vld & 0x20) ? "Hit" : "NO Hit");
			printk("\t   [HashLite(type-0): ");
			switch ((hdr_cpu->hdr_i_info.fwd_vld >> 6) & 0x3) {
			case 0:
				printk("NO Hit"); break;
			case 1:
				printk("Hit Overflow Entry"); break;
			case 2:
			case 3:
				printk("Hit Hash Entry"); break;
			default:
				printk("UNKNOWN"); break;
			}
			printk(", HashLite(type-1): %s]\n", (hdr_cpu->hdr_i_info.fwd_vld & 0x100) ? "Hit" : "NO Hit");

		}
	}

	/*** parse skb->data ***/

	/* skb->len: the data length in skb->data.
	 * skb->data_len: the data lengh in skb fragment list.
	 */

	ca_ni_parse_pkts(payload_ptr, payload_ptr + total_len - skb->data_len, true);

	/*** dump skb->data ***/

	printk(KERN_INFO "==> Dump SKB (after pull 14 bytes), skb->len %u, skb->data_len %u, dump_len %u\n", skb->len, skb->data_len, total_len - skb->data_len);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, payload_ptr, total_len - skb->data_len, true);

	if (skb_shinfo(skb)->frag_list) {
		for (skbinlist = skb_shinfo(skb)->frag_list; skbinlist; skbinlist = skbinlist->next) {
			printk(KERN_INFO "==> Dump SKB in frag_list, skb->len (dump_len) %u\n", skbinlist->len);
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, skbinlist->data, skbinlist->len, true);
		}
	}
}

void ca_ni_dump_tx_pkts(struct sk_buff *skb)
{
	printk("====== TX skb data@%p, skb->len %d ======\n", skb->data, skb->len);

	/*** parse skb->data ***/

	ca_ni_parse_pkts(skb->data, skb->data + (skb->len - skb->data_len), true);

	/*** dump skb->data ***/

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, skb->data, skb->len, true);
}

