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
#ifndef __CA_NI_DUMP_H__
#define __CA_NI_DUMP_H__

#include "aal_common.h"
#include "ca_ni_common.h"

/*************************************************************************
 * PTP packet struct
 *************************************************************************/

#define PTP_SYNC			0x0
#define PTP_DELAY_REQ			0x1
#define PTP_PDELAY_REQ			0x2
#define PTP_PDELAY_RESP			0x3
#define PTP_FOLLOW_UP			0x8
#define PTP_DELAY_RESP			0x9
#define PTP_PDELAY_RESP_FOLLOW_UP	0xA
#define PTP_ANNOUNCE			0xB
#define PTP_SIGNALING			0xC
#define PTP_MANAGEMENT			0xD

enum ptp_control_field {
	PTP_CTL_SYNC,
	PTP_CTL_DELAY_REQ,
	PTP_CTL_FOLLOW_UP,
	PTP_CTL_DELAY_RESP,
	PTP_CTL_MANAGEMENT,
	PTP_CTL_OTHER,
};

/* PTP version 2; IEEE 1588-2008 */
struct ptp_header {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	uint8_t			msg_type : 4;
	uint8_t			transport_specific : 4;
	uint8_t			version_ptp : 4;
	uint8_t			reserved0 : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	uint8_t			transport_specific : 4;
	uint8_t			msg_type : 4;
	uint8_t			reserved0 : 4;
	uint8_t			version_ptp : 4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	uint16_t		msg_len;
	uint8_t			domain_num;
	uint8_t			reserved1;
	uint16_t		flag;
	int64_t			correction_field;
	uint32_t		reserved2;
	uint8_t			clock_id[8];
	uint16_t		source_port_id;
	uint16_t		seq_id;
	uint8_t			control_field;
	int8_t			log_msg_interval;
} __packed;

/*************************************************************************
 * MACSec frame struct
 *************************************************************************/

/* copied from struct macsec_eth_header but no struct ethhdr */
struct macsec_header {
	/* SecTAG */
	u8  tci_an;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u8  short_length:6,
		  unused:2;
#elif defined(__BIG_ENDIAN_BITFIELD)
	u8        unused:2,
	    short_length:6;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	__be32 packet_number;
	u8 secure_channel_id[8]; /* optional */
} __packed;

#define MACSEC_TCI_VERSION 0x80
#define MACSEC_TCI_ES      0x40 /* end station */
#define MACSEC_TCI_SC      0x20 /* SCI present */
#define MACSEC_TCI_SCB     0x10 /* epon */
#define MACSEC_TCI_E       0x08 /* encryption */
#define MACSEC_TCI_C       0x04 /* changed text */
#define MACSEC_AN_MASK     0x03 /* association number */
#define MACSEC_TCI_CONFID  (MACSEC_TCI_E | MACSEC_TCI_C)

/*************************************************************************
 * all functions to parse and dump packets
 *************************************************************************/

uint16_t ca_ni_parse_l2(uint8_t *data, uint8_t **p_outer_tag, uint8_t **p_inner_tag, uint8_t **p_ethertype, uint8_t **p_payload, bool dump);
void ca_ni_parse_ppp_discovery(uint8_t *data, uint8_t **p_payload, bool dump);
uint16_t ca_ni_parse_ppp_session(uint8_t *data, uint8_t **p_payload, bool dump);

uint8_t ca_ni_parse_ipv4(uint8_t *data, uint8_t **p_payload, bool dump, bool *is_frag);
uint8_t ca_ni_parse_ipv6(uint8_t *data, uint8_t **p_payload, bool dump, bool *is_frag);
void ca_ni_parse_arp(uint8_t *data, bool dump);
uint16_t ca_ni_parse_macsec(uint8_t *data, uint8_t **p_payload, bool dump);

void ca_ni_parse_icmp(uint8_t *data, bool dump);
void ca_ni_parse_igmp(uint8_t *data, bool dump);
void ca_ni_parse_tcp(uint8_t *data, uint8_t **p_payload, uint16_t *l4_sport, uint16_t *l4_dport, bool dump);
void ca_ni_parse_udp(uint8_t *data, uint8_t **p_payload, uint16_t *l4_sport, uint16_t *l4_dport, bool dump);
void ca_ni_parse_esp(uint8_t *data, bool dump);
void ca_ni_parse_ah(uint8_t *data, bool dump);
void ca_ni_parse_icmpv6(uint8_t *data, int len, bool dump);
void ca_ni_parse_udplite(uint8_t *data, bool dump);
void ca_ni_parse_ptp(uint8_t *data, bool dump);
void ca_ni_parse_rdp(uint8_t *data, bool dump);

void ca_ni_parse_tcp_udp(uint8_t *data, uint16_t l4_sport, uint16_t l4_dport, bool dump);

void ca_ni_dump_hdr_a(HEADER_A_T *hdr_a, bool is_rx);
void ca_ni_dump_rx_pkts(struct sk_buff *skb, HEADER_A_T *hdr_a, HEADER_CPU_T *hdr_cpu, HEADER_PTP_T *hdr_ptp);
void ca_ni_dump_tx_pkts(struct sk_buff *skb);

#if defined(CONFIG_NE_TX_LLID_MAP)
ca_status_t ca_ni_find_source_ip(uint8_t *data, uint8_t *data_end, ca_uint32_t *source_ip);
#endif

#endif /* __CA_NI_DUMP_H__ */

