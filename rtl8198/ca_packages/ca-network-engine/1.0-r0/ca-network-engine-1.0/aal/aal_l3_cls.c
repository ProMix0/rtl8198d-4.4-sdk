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
 * aal_l3_cls.c: Hardware Abstraction Layer for L3 classifier to access hardware regsiters
 *
 */

#include "aal_table.h"
#include "aal_common.h"
#include "aal_l3fe.h"
#include "aal_l3_cam.h"
#include "aal_l3_cls.h"
#include "aal_port.h"
#include "ca_aal_proc.h"
#include "ca-scfg/scfg.h"

typedef enum {
	MSK_SPCL_PKT_HDR_MTCH = 0UL,
	MSK_SPCL_PKT_ENC,
	MSK_TCP_RDP_CTRL,
	MSK_L4_PORT,
	MSK_L4_VLD,
	MSK_L3_CHKSUM_ERR,
	MSK_SPI,
	MSK_ICMP_TYPE,
	MSK_ICMP_VLD,
	MSK_IPV6_DOH,
	MSK_IPV6_RH,
	MSK_IPV6_HBH,
	MSK_IPV6_NDP,
	MSK_IP_FRAGMENT,
	MSK_IP_OPTIONS,
	MSK_IPV6_FLOW_LBL,
	MSK_IP_DA_SA_EQUAL,
	MSK_IP_DA_MC,
	MSK_IP_DA_ENC,
	MSK_IPV4_DA,
	MSK_IPV6_DA,
	MSK_IP_SA_ENC,
	MSK_IPV4_SA,
	MSK_IPV6_SA,
	MSK_IP_PROTOCOL,
	MSK_IP_ECN,
	MSK_DSCP,
	MSK_IP_VER,
	MSK_IP_VLD,
	MSK_PPP_PROTOCOL,
	MSK_PPPOE_SESSION,
	MSK_INNER_DEI,
	MSK_INNER_802_1P,
	MSK_INNER_VID,
	MSK_INNER_TPID_ENC,
	MSK_TOP_DEI,
	MSK_TOP_VL_802_1P,
	MSK_TOP_VID_PTAG,
	MSK_TOP_VID,
	MSK_TOP_VID_CTL,
	MSK_TOP_TPID_ENC,
	MSK_VLAN_CNT,
	MSK_LLC_TYPE_ENC,
	MSK_PKTLEN_RNG_MATCH_VEC,
	MSK_LEN_ENCODED,
	MSK_ETHERTYPE_ENC,
	MSK_MAC_SA,
	MSK_MAC_DA_RSVD,
	MSK_MAC_DA_IP_MC,
	MSK_MAC_DA_MC,
	MSK_MAC_DA_BC,
	MSK_MAC_DA_AN_HIT,
	MSK_MAC_DA_AN_SEL,
	MSK_MAC_DA,
	MSK_DEST_ADDR,
	MSK_SRC_ADDR,
	MSK_MDATA,
	MSK_MCGID,
	MSK_O_LSPID,
	MSK_LSPID,
} l3_cls_key_mask_t;

typedef enum {
	FIB_MSK_CTRL = 0,
	FIB_MSK_DPID,
	FIB_MSK_QOS,
	FIB_MSK_POL,
	FIB_MSK_MDATA_W3,
	FIB_MSK_MDATA_W2,
	FIB_MSK_MDATA_W1,
	FIB_MSK_MDATA_W0,
	FIB_MSK_MAC_DA,
	FIB_MSK_MAC_SA,
	FIB_MSK_VLAN,
	FIB_MSK_L3_EGRESS_IF,
	FIB_MSK_PPPOE,
	FIB_MSK_IP_SA_H,
	FIB_MSK_IP_SA_L,
	FIB_MSK_IP_DA_H,
	FIB_MSK_IP_DA_L,
	FIB_MSK_L4_SP,
	FIB_MSK_L4_DP,
	FIB_MSK_HASH_CHK,
} l3_cls_fib_mask_t;

#define CLS_KEY_VLD_SET(kidx, vld) {\
	vld |= (0x1 << kidx);\
} while(0)

#define CLS_KEY_VLD_CLR(ktyp, kidx, vld) {\
	vld &= ~(0x1 << kidx);\
} while(0)

#define CLS_KEY_MSK_SPCL_PKT_HDR_MTCH (1ULL << MSK_SPCL_PKT_HDR_MTCH)
#define CLS_KEY_MSK_SPCL_PKT_ENC (1ULL << MSK_SPCL_PKT_ENC)
#define CLS_KEY_MSK_TCP_RDP_CTRL (1ULL << MSK_TCP_RDP_CTRL)
#define CLS_KEY_MSK_L4_PORT (1ULL << MSK_L4_PORT)
#define CLS_KEY_MSK_L4_VLD (1ULL << MSK_L4_VLD)
#define CLS_KEY_MSK_L3_CHKSUM_ERR (1ULL << MSK_L3_CHKSUM_ERR)
#define CLS_KEY_MSK_SPI (1ULL << MSK_SPI)
#define CLS_KEY_MSK_ICMP_TYPE (1ULL << MSK_ICMP_TYPE)
#define CLS_KEY_MSK_ICMP_VLD (1ULL << MSK_ICMP_VLD)
#define CLS_KEY_MSK_IPV6_DOH (1ULL << MSK_IPV6_DOH)
#define CLS_KEY_MSK_IPV6_RH (1ULL << MSK_IPV6_RH)
#define CLS_KEY_MSK_IPV6_HBH (1ULL << MSK_IPV6_HBH)
#define CLS_KEY_MSK_IPV6_NDP (1ULL << MSK_IPV6_NDP)
#define CLS_KEY_MSK_IP_FRAGMENT (1ULL << MSK_IP_FRAGMENT)
#define CLS_KEY_MSK_IP_OPTIONS (1ULL << MSK_IP_OPTIONS)
#define CLS_KEY_MSK_IPV6_FLOW_LBL (1ULL << MSK_IPV6_FLOW_LBL)
#define CLS_KEY_MSK_IP_DA_SA_EQUAL (1ULL << MSK_IP_DA_SA_EQUAL)
#define CLS_KEY_MSK_IP_DA_MC (1ULL << MSK_IP_DA_MC)
#define CLS_KEY_MSK_IP_DA_ENC (1ULL << MSK_IP_DA_ENC)
#define CLS_KEY_MSK_IPV4_DA (1ULL << MSK_IPV4_DA)
#define CLS_KEY_MSK_IPV6_DA (1ULL << MSK_IPV6_DA)
#define CLS_KEY_MSK_IP_SA_ENC (1ULL << MSK_IP_SA_ENC)
#define CLS_KEY_MSK_IPV4_SA (1ULL << MSK_IPV4_SA)
#define CLS_KEY_MSK_IPV6_SA (1ULL << MSK_IPV6_SA)
#define CLS_KEY_MSK_IP_PROTOCOL (1ULL << MSK_IP_PROTOCOL)
#define CLS_KEY_MSK_IP_ECN (1ULL << MSK_IP_ECN)
#define CLS_KEY_MSK_DSCP (1ULL << MSK_DSCP)
#define CLS_KEY_MSK_IP_VER (1ULL << MSK_IP_VER)
#define CLS_KEY_MSK_IP_VLD (1ULL << MSK_IP_VLD)
#define CLS_KEY_MSK_PPP_PROTOCOL (1ULL << MSK_PPP_PROTOCOL)
#define CLS_KEY_MSK_PPPOE_SESSION (1ULL << MSK_PPPOE_SESSION)
#define CLS_KEY_MSK_INNER_DEI (1ULL << MSK_INNER_DEI)
#define CLS_KEY_MSK_INNER_802_1P (1ULL << MSK_INNER_802_1P)
#define CLS_KEY_MSK_INNER_VID (1ULL << MSK_INNER_VID)
#define CLS_KEY_MSK_INNER_TPID_ENC (1ULL << MSK_INNER_TPID_ENC)
#define CLS_KEY_MSK_TOP_DEI (1ULL << MSK_TOP_DEI)
#define CLS_KEY_MSK_TOP_VL_802_1P (1ULL << MSK_TOP_VL_802_1P)
#define CLS_KEY_MSK_TOP_VID_PTAG (1ULL << MSK_TOP_VID_PTAG)
#define CLS_KEY_MSK_TOP_VID (1ULL << MSK_TOP_VID)
#define CLS_KEY_MSK_VID_CTL (1ULL << MSK_TOP_VID_CTL)
#define CLS_KEY_MSK_TOP_TPID_ENC (1ULL << MSK_TOP_TPID_ENC)
#define CLS_KEY_MSK_VLAN_CNT (1ULL << MSK_VLAN_CNT)
#define CLS_KEY_MSK_LLC_TYPE_ENC (1ULL << MSK_LLC_TYPE_ENC)
#define CLS_KEY_MSK_PKTLEN_RNG_MATCH_VEC (1ULL << MSK_PKTLEN_RNG_MATCH_VEC)
#define CLS_KEY_MSK_LEN_ENCODED (1ULL << MSK_LEN_ENCODED)
#define CLS_KEY_MSK_ETHERTYPE_ENC (1ULL << MSK_ETHERTYPE_ENC)
#define CLS_KEY_MSK_MAC_SA (1ULL << MSK_MAC_SA)
#define CLS_KEY_MSK_MAC_DA_RSVD (1ULL << MSK_MAC_DA_RSVD)
#define CLS_KEY_MSK_MAC_DA_IP_MC (1ULL << MSK_MAC_DA_IP_MC)
#define CLS_KEY_MSK_MAC_DA_MC (1ULL << MSK_MAC_DA_MC)
#define CLS_KEY_MSK_MAC_DA_BC (1ULL << MSK_MAC_DA_BC)
#define CLS_KEY_MSK_MAC_DA_AN_HIT (1ULL << MSK_MAC_DA_AN_HIT)
#define CLS_KEY_MSK_MAC_DA_AN_SEL (1ULL << MSK_MAC_DA_AN_SEL)
#define CLS_KEY_MSK_MAC_DA (1ULL << MSK_MAC_DA)
#define CLS_KEY_MSK_SRC_ADDR (1ULL << MSK_SRC_ADDR)
#define CLS_KEY_MSK_MDATA (1ULL << MSK_MDATA)
#define CLS_KEY_MSK_MCGID (1ULL << MSK_MCGID)
#define CLS_KEY_MSK_O_LSPID (1ULL << MSK_O_LSPID)
#define CLS_KEY_MSK_LSPID (1ULL << MSK_LSPID)

#define CL_IF_ID_KEY_MSK (CLS_KEY_MSK_IP_FRAGMENT|CLS_KEY_MSK_IP_DA_SA_EQUAL|CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IP_VER|CLS_KEY_MSK_IP_VLD|\
			CLS_KEY_MSK_PPPOE_SESSION|CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|\
			CLS_KEY_MSK_VLAN_CNT|CLS_KEY_MSK_ETHERTYPE_ENC|CLS_KEY_MSK_MAC_DA_IP_MC|CLS_KEY_MSK_MAC_DA_MC|\
			CLS_KEY_MSK_MAC_DA_BC|CLS_KEY_MSK_MAC_DA_AN_HIT|CLS_KEY_MSK_MAC_DA_AN_SEL|CLS_KEY_MSK_O_LSPID|\
			CLS_KEY_MSK_LSPID)
#define CL_IPV4_TUNNEL_ID_KEY_MSK (CLS_KEY_MSK_L4_PORT|CLS_KEY_MSK_L4_VLD|CLS_KEY_MSK_SPI|CLS_KEY_MSK_IP_FRAGMENT|\
			CLS_KEY_MSK_IP_DA_SA_EQUAL|CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IP_DA_ENC|CLS_KEY_MSK_IPV4_DA|CLS_KEY_MSK_IP_SA_ENC|\
			CLS_KEY_MSK_IPV4_SA|CLS_KEY_MSK_IP_PROTOCOL|CLS_KEY_MSK_IP_VER|\
			CLS_KEY_MSK_IP_VLD|CLS_KEY_MSK_PPPOE_SESSION|CLS_KEY_MSK_INNER_VID|CLS_KEY_MSK_INNER_TPID_ENC|\
			CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|CLS_KEY_MSK_VLAN_CNT|\
			CLS_KEY_MSK_ETHERTYPE_ENC|CLS_KEY_MSK_MAC_DA_IP_MC|CLS_KEY_MSK_MAC_DA_MC|CLS_KEY_MSK_MAC_DA_BC|\
			CLS_KEY_MSK_MAC_DA_AN_HIT|CLS_KEY_MSK_MAC_DA_AN_SEL|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)
#define CL_IPV4_SHORT_KEY_MSK (CLS_KEY_MSK_L4_PORT|CLS_KEY_MSK_L4_VLD|CLS_KEY_MSK_L3_CHKSUM_ERR|CLS_KEY_MSK_SPI|\
			CLS_KEY_MSK_IP_FRAGMENT|CLS_KEY_MSK_IP_DA_SA_EQUAL|CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IP_PROTOCOL|\
			CLS_KEY_MSK_IP_ECN|CLS_KEY_MSK_DSCP|CLS_KEY_MSK_TOP_DEI|CLS_KEY_MSK_TOP_VL_802_1P|CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|\
			CLS_KEY_MSK_TOP_TPID_ENC|CLS_KEY_MSK_VLAN_CNT|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)
#define CL_IPV4_SA_SHORT_KEY_MSK (CL_IPV4_SHORT_KEY_MSK | CLS_KEY_MSK_IPV4_SA)
#define CL_IPV4_DA_SHORT_KEY_MSK (CL_IPV4_SHORT_KEY_MSK | CLS_KEY_MSK_IPV4_DA)
#define CL_IPV6_SHORT_KEY_MSK (CLS_KEY_MSK_L4_PORT|CLS_KEY_MSK_L4_VLD|CLS_KEY_MSK_SPI|\
			CLS_KEY_MSK_IP_FRAGMENT|CLS_KEY_MSK_IPV6_FLOW_LBL|CLS_KEY_MSK_IP_DA_SA_EQUAL|\
			CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IP_PROTOCOL|CLS_KEY_MSK_IP_ECN|CLS_KEY_MSK_DSCP|CLS_KEY_MSK_TOP_DEI|\
			CLS_KEY_MSK_TOP_VL_802_1P|CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|\
			CLS_KEY_MSK_VLAN_CNT|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)
#define CL_IPV6_SA_SHORT_KEY_MSK (CL_IPV6_SHORT_KEY_MSK | CLS_KEY_MSK_IPV6_SA)
#define CL_IPV6_DA_SHORT_KEY_MSK (CL_IPV6_SHORT_KEY_MSK | CLS_KEY_MSK_IPV6_DA)
#define CL_SPCL_PKT_KEY_MSK (CLS_KEY_MSK_SPCL_PKT_HDR_MTCH|CLS_KEY_MSK_SPCL_PKT_ENC|CLS_KEY_MSK_ICMP_TYPE|\
			CLS_KEY_MSK_ICMP_VLD|CLS_KEY_MSK_IPV6_DOH|CLS_KEY_MSK_IPV6_RH|CLS_KEY_MSK_IPV6_HBH|CLS_KEY_MSK_IPV6_NDP|\
			CLS_KEY_MSK_IP_FRAGMENT|CLS_KEY_MSK_IP_OPTIONS|CLS_KEY_MSK_IP_PROTOCOL|CLS_KEY_MSK_TOP_DEI|\
			CLS_KEY_MSK_TOP_VL_802_1P|CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|\
			CLS_KEY_MSK_VLAN_CNT|CLS_KEY_MSK_ETHERTYPE_ENC|CLS_KEY_MSK_MAC_DA_RSVD|CLS_KEY_MSK_MAC_DA_IP_MC|\
			CLS_KEY_MSK_MAC_DA_MC|CLS_KEY_MSK_MAC_DA_BC|CLS_KEY_MSK_MAC_DA_AN_HIT|CLS_KEY_MSK_MAC_DA_AN_SEL|\
			CLS_KEY_MSK_MAC_DA|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)
#define CL_MCST_KEY_MSK (CLS_KEY_MSK_IP_DA_SA_EQUAL|CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IP_VER|CLS_KEY_MSK_IP_VLD|CLS_KEY_MSK_PPPOE_SESSION|\
			CLS_KEY_MSK_INNER_VID|CLS_KEY_MSK_INNER_TPID_ENC|CLS_KEY_MSK_TOP_DEI|CLS_KEY_MSK_TOP_VL_802_1P|\
			CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|CLS_KEY_MSK_VLAN_CNT|\
			CLS_KEY_MSK_MAC_DA_IP_MC|CLS_KEY_MSK_MAC_DA_MC|CLS_KEY_MSK_MAC_DA_BC|\
			CLS_KEY_MSK_MCGID|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)
#define CL_MCST_MAC_DA_KEY_MSK (CL_MCST_KEY_MSK | CLS_KEY_MSK_MAC_DA)
#define CL_MCST_IP_DA_KEY_MSK (CL_MCST_KEY_MSK | CLS_KEY_MSK_IPV4_DA|CLS_KEY_MSK_IPV6_DA)
#define CL_FULL_KEY_MSK (CLS_KEY_MSK_TCP_RDP_CTRL|CLS_KEY_MSK_L4_PORT|CLS_KEY_MSK_L4_VLD|CLS_KEY_MSK_L3_CHKSUM_ERR|\
			CLS_KEY_MSK_SPI|CLS_KEY_MSK_IP_FRAGMENT|CLS_KEY_MSK_IP_OPTIONS|CLS_KEY_MSK_IP_DA_SA_EQUAL|CLS_KEY_MSK_IP_DA_MC|CLS_KEY_MSK_IPV6_DA|\
			CLS_KEY_MSK_IPV6_SA|CLS_KEY_MSK_IPV4_DA|CLS_KEY_MSK_IPV4_SA|CLS_KEY_MSK_IP_PROTOCOL|CLS_KEY_MSK_IP_ECN|CLS_KEY_MSK_DSCP|CLS_KEY_MSK_IP_VER|\
			CLS_KEY_MSK_IP_VLD|CLS_KEY_MSK_PPP_PROTOCOL|CLS_KEY_MSK_PPPOE_SESSION|CLS_KEY_MSK_INNER_DEI|\
			CLS_KEY_MSK_INNER_802_1P|CLS_KEY_MSK_INNER_VID|CLS_KEY_MSK_INNER_TPID_ENC|CLS_KEY_MSK_TOP_DEI|\
			CLS_KEY_MSK_TOP_VL_802_1P|CLS_KEY_MSK_TOP_VID_PTAG|CLS_KEY_MSK_VID_CTL|CLS_KEY_MSK_TOP_VID|CLS_KEY_MSK_TOP_TPID_ENC|\
			CLS_KEY_MSK_VLAN_CNT|CLS_KEY_MSK_LLC_TYPE_ENC|CLS_KEY_MSK_PKTLEN_RNG_MATCH_VEC|CLS_KEY_MSK_LEN_ENCODED|\
			CLS_KEY_MSK_ETHERTYPE_ENC|CLS_KEY_MSK_MAC_SA|CLS_KEY_MSK_MAC_DA_RSVD|CLS_KEY_MSK_MAC_DA_IP_MC|\
			CLS_KEY_MSK_MAC_DA_MC|CLS_KEY_MSK_MAC_DA_BC|CLS_KEY_MSK_MAC_DA_AN_HIT|CLS_KEY_MSK_MAC_DA_AN_SEL|CLS_KEY_MSK_MDATA|\
			CLS_KEY_MSK_MCGID|CLS_KEY_MSK_O_LSPID|CLS_KEY_MSK_LSPID)

#define CLS_FIB_MSK_CTRL (1ULL << FIB_MSK_CTRL)
#define CLS_FIB_MSK_DPID (1ULL << FIB_MSK_DPID)
#define CLS_FIB_MSK_QOS (1ULL << FIB_MSK_QOS)
#define CLS_FIB_MSK_POL (1ULL << FIB_MSK_POL)
#define CLS_FIB_MSK_MDATA_W3 (1ULL << FIB_MSK_MDATA_W3)
#define CLS_FIB_MSK_MDATA_W2 (1ULL << FIB_MSK_MDATA_W2)
#define CLS_FIB_MSK_MDATA_W1 (1ULL << FIB_MSK_MDATA_W1)
#define CLS_FIB_MSK_MDATA_W0 (1ULL << FIB_MSK_MDATA_W0)
#define CLS_FIB_MSK_MAC_DA (1ULL << FIB_MSK_MAC_DA)
#define CLS_FIB_MSK_MAC_SA (1ULL << FIB_MSK_MAC_SA)
#define CLS_FIB_MSK_VLAN (1ULL << FIB_MSK_VLAN)
#define CLS_FIB_MSK_L3_EGRESS_IF (1ULL << FIB_MSK_L3_EGRESS_IF)
#define CLS_FIB_MSK_PPPOE (1ULL << FIB_MSK_PPPOE)
#define CLS_FIB_MSK_IP_SA_H (1ULL << FIB_MSK_IP_SA_H)
#define CLS_FIB_MSK_IP_SA_L (1ULL << FIB_MSK_IP_SA_L)
#define CLS_FIB_MSK_IP_DA_H (1ULL << FIB_MSK_IP_DA_H)
#define CLS_FIB_MSK_IP_DA_L (1ULL << FIB_MSK_IP_DA_L)
#define CLS_FIB_MSK_L4_SP (1ULL << FIB_MSK_L4_SP)
#define CLS_FIB_MSK_L4_DP (1ULL << FIB_MSK_L4_DP)
#define CLS_FIB_MSK_HASH_CHK (1ULL << FIB_MSK_HASH_CHK)

#define CL_FIB_MOD_0_MSK (CLS_FIB_MSK_CTRL|CLS_FIB_MSK_VLAN|CLS_FIB_MSK_DPID|CLS_FIB_MSK_QOS|CLS_FIB_MSK_POL|\
			CLS_FIB_MSK_L3_EGRESS_IF|CLS_FIB_MSK_MDATA_W1)
#define CL_FIB_MOD_1_MSK (CLS_FIB_MSK_CTRL|CLS_FIB_MSK_VLAN|CLS_FIB_MSK_DPID|CLS_FIB_MSK_QOS|CLS_FIB_MSK_POL|\
			CLS_FIB_MSK_MDATA_W0|CLS_FIB_MSK_MDATA_W1)
#define CL_FIB_MOD_2_MSK (CLS_FIB_MSK_CTRL|CLS_FIB_MSK_VLAN|CLS_FIB_MSK_DPID|CLS_FIB_MSK_QOS|CLS_FIB_MSK_POL|\
			CLS_FIB_MSK_L3_EGRESS_IF|CLS_FIB_MSK_PPPOE|CLS_FIB_MSK_MDATA_W0|CLS_FIB_MSK_MDATA_W1|CLS_FIB_MSK_MDATA_W2|\
			CLS_FIB_MSK_MDATA_W3|CLS_FIB_MSK_MAC_DA|CLS_FIB_MSK_MAC_SA)
#define CL_FIB_MOD_3_MSK (CLS_FIB_MSK_CTRL|CLS_FIB_MSK_VLAN|CLS_FIB_MSK_DPID|CLS_FIB_MSK_QOS|CLS_FIB_MSK_POL|\
			CLS_FIB_MSK_L3_EGRESS_IF|CLS_FIB_MSK_PPPOE|CLS_FIB_MSK_MDATA_W0|CLS_FIB_MSK_MDATA_W1|CLS_FIB_MSK_IP_SA_L|\
			CLS_FIB_MSK_IP_SA_H|CLS_FIB_MSK_L4_SP)
#define CL_FIB_MOD_4_MSK (CLS_FIB_MSK_CTRL|CLS_FIB_MSK_VLAN|CLS_FIB_MSK_DPID|CLS_FIB_MSK_QOS|CLS_FIB_MSK_POL|\
			CLS_FIB_MSK_L3_EGRESS_IF|CLS_FIB_MSK_PPPOE|CLS_FIB_MSK_MDATA_W0|CLS_FIB_MSK_MDATA_W1|CLS_FIB_MSK_IP_DA_L|\
			CLS_FIB_MSK_IP_DA_H|CLS_FIB_MSK_L4_DP)

#define CA_CLS_REG_READ(offset)		CA_REG_READ(AAL_L3FE_REG_ADDR(offset))
#define CA_CLS_REG_WRITE(data, offset)	CA_REG_WRITE(data, AAL_L3FE_REG_ADDR(offset))

static spinlock_t cls_lock;
static aal_table_t l3_cls_key_table, l3_cls_fib_table;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static aal_table_t hgu_8279_l3_cls_key_table, hgu_8279_l3_cls_fib_table;
#endif
static char datapath_mode = 0; /* 0 = router gateway, 1 = bridge mode */

/***************************************************************************
 * internal functions
 ***************************************************************************/
static ca_int32_t __l3_cls_fib_idx_get(aal_table_t *key_table, l3_cls_key_type_t key_type, ca_uint32_t key_tbl_idx, ca_uint8_t key_idx, ca_uint32_t *fib_tbl_idx);
static ca_int32_t __l3_cls_fib_mode_get(aal_table_t *fib_table, ca_uint32_t fib_tbl_idx, ca_uint16_t *short_fib, l3_cls_fib_mode_t *fib_mode);

/***************************************************************************
 * AAL table, L3 CLS FIB TBL
 ***************************************************************************/
static int __l3_cls_fib_tbl_entry_add_by_idx(void *entry, unsigned int fib_idx)
{
	return generic_entry_add_by_idx(&l3_cls_fib_table, entry, fib_idx);
}

static int __l3_cls_fib_tbl_entry_del(unsigned int fib_idx)
{
	return generic_entry_del(&l3_cls_fib_table, fib_idx);
}

static int __l3_cls_fib_tbl_entry_set(unsigned int fib_idx, void *entry)
{
	return generic_entry_set(&l3_cls_fib_table, fib_idx, entry);
}

static int __l3_cls_fib_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&l3_cls_fib_table, entry, rslt_idx);
}

static int __l3_cls_fib_tbl_entry_get_by_idx(unsigned int fib_idx, void *entry)
{
	return generic_entry_get_by_idx(&l3_cls_fib_table, fib_idx, entry);
}

static int __l3_cls_fib_table_flush(void)
{
	return generic_table_flush(&l3_cls_fib_table);
}

static int __l3_cls_fib_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&l3_cls_fib_table);
}

#define FIB_MSK_CTRL_PRINT(data) {\
	printk(KERN_INFO "| t2_ctrl = 0x%x\n", data.t2_ctrl);\
	printk(KERN_INFO "| t2_ctrl_vld = 0x%x\n", data.t2_ctrl_vld);\
	printk(KERN_INFO "| t3_ctrl = 0x%x\n", data.t3_ctrl);\
	printk(KERN_INFO "| t3_ctrl_vld = 0x%x\n", data.t3_ctrl_vld);\
	printk(KERN_INFO "| t4_ctrl = 0x%x\n", data.t4_ctrl);\
	printk(KERN_INFO "| t4_ctrl_vld = 0x%x\n", data.t4_ctrl_vld);\
	printk(KERN_INFO "| t5_ctrl = 0x%x\n", data.t5_ctrl);\
	printk(KERN_INFO "| t5_ctrl_vld = 0x%x\n", data.t5_ctrl_vld);\
	printk(KERN_INFO "| stage2_ctrl = 0x%x\n", data.stage2_ctrl);\
	printk(KERN_INFO "| stage2_ctrl_vld = 0x%x\n", data.stage2_ctrl_vld);\
	printk(KERN_INFO "| stage3_ctrl = 0x%x\n", data.stage3_ctrl);\
	printk(KERN_INFO "| stage3_ctrl_vld = 0x%x\n", data.stage3_ctrl_vld);\
	printk(KERN_INFO "| keep_orig_pkt = 0x%x\n", data.keep_orig_pkt);\
	printk(KERN_INFO "| keep_orig_pkt_vld = 0x%x\n", data.keep_orig_pkt_vld);\
	printk(KERN_INFO "| keep_ts_en = 0x%x\n", data.keep_ts_en);\
	printk(KERN_INFO "| keep_ts_vld = 0x%x\n", data.keep_ts_vld);\
	printk(KERN_INFO "| pop_l3_en = 0x%x\n", data.pop_l3_en);\
	printk(KERN_INFO "| pop_l3_chk_ecn_en = 0x%x\n", data.pop_l3_chk_ecn_en);\
	printk(KERN_INFO "| pop_l3_vld = 0x%x\n", data.pop_l3_vld);\
	printk(KERN_INFO "| cache_ctrl = 0x%x\n", data.cache_ctrl);\
	printk(KERN_INFO "| chk_msk_ptr = 0x%x\n", data.chk_msk_ptr);\
} while(0)

#define FIB_MSK_DPID_PRINT(data) {\
	printk(KERN_INFO "| mc = 0x%x\n", data.mc);\
	printk(KERN_INFO "| mcgid = 0x%x\n", data.mcgid);\
	printk(KERN_INFO "| deepq = 0x%x\n", data.deepq);\
	printk(KERN_INFO "| permit = 0x%x\n", data.permit);\
	printk(KERN_INFO "| dpid_pri = 0x%x\n", data.dpid_pri);\
	printk(KERN_INFO "| dpid_vld = 0x%x\n", data.dpid_vld);\
	printk(KERN_INFO "| no_drop = 0x%x\n", data.no_drop);\
	printk(KERN_INFO "| no_drop_vld = 0x%x\n", data.no_drop_vld);\
	printk(KERN_INFO "| mrr_en = 0x%x\n", data.mrr_en);\
	printk(KERN_INFO "| mrr_vld = 0x%x\n", data.mrr_vld);\
} while(0)

#define FIB_MSK_QOS_PRINT(data) {\
	printk(KERN_INFO "| cos = 0x%x\n", data.cos);\
	printk(KERN_INFO "| cos_sel = 0x%x\n", data.cos_sel);\
	printk(KERN_INFO "| cos_table_sel = 0x%x\n", data.cos_table_sel);\
	printk(KERN_INFO "| qos_802_1p_table_sel = 0x%x\n", data.qos_802_1p_table_sel);\
	printk(KERN_INFO "| top_802_1p = 0x%x\n", data.top_802_1p);\
	printk(KERN_INFO "| top_802_1p_sel = 0x%x\n", data.top_802_1p_sel);\
	printk(KERN_INFO "| inner_802_1p = 0x%x\n", data.inner_802_1p);\
	printk(KERN_INFO "| inner_802_1p_sel = 0x%x\n", data.inner_802_1p_sel);\
	printk(KERN_INFO "| dscp_table_sel = 0x%x\n", data.dscp_table_sel);\
	printk(KERN_INFO "| ip_dscp = 0x%x\n", data.ip_dscp);\
	printk(KERN_INFO "| ip_dscp_sel = 0x%x\n", data.ip_dscp_sel);\
	printk(KERN_INFO "| ip_dscp_update_en = 0x%x\n", data.ip_dscp_update_en);\
	printk(KERN_INFO "| ip_dscp_markdown_en = 0x%x\n", data.ip_dscp_markdown_en);\
	printk(KERN_INFO "| ip_dscp_marked_down = 0x%x\n", data.ip_dscp_marked_down);\
	printk(KERN_INFO "| ip_dscp_markdown_vld = 0x%x\n", data.ip_dscp_marked_vld);\
	printk(KERN_INFO "| ip_tos_6 = 0x%x\n", data.ip_tos_6);\
	printk(KERN_INFO "| ip_tos_vld = 0x%x\n", data.ip_tos_vld);\
	printk(KERN_INFO "| ip_ecn_en = 0x%x\n", data.ip_ecn_en);\
	printk(KERN_INFO "| ip_ecn_vld = 0x%x\n", data.ip_ecn_vld);\
} while(0)

#define FIB_MSK_POL_PRINT(data) {\
	printk(KERN_INFO "| pol_grp_id = 0x%x\n", data.pol_grp_id);\
	printk(KERN_INFO "| pol_grp_en = 0x%x\n", data.pol_grp_vld);\
	printk(KERN_INFO "| pol_table_sel = 0x%x\n", data.pol_table_sel);\
	printk(KERN_INFO "| pol_sel = 0x%x\n", data.pol_sel);\
	printk(KERN_INFO "| pol_base = 0x%x\n", data.pol_base);\
	printk(KERN_INFO "| pol_en = 0x%x\n", data.pol_en);\
	printk(KERN_INFO "| pol_all_bypass = 0x%x\n", data.pol_all_bypass);\
	printk(KERN_INFO "| qos_premark = 0x%x\n", data.qos_premark);\
	printk(KERN_INFO "| pol_vld = 0x%x\n", data.pol_vld);\
} while(0)

#define FIB_MSK_MDATA_W3_PRINT(data) {\
	printk(KERN_INFO "| mdata_w_3 = 0x%x\n", data.mdata_w_3);\
	printk(KERN_INFO "| mdata_w_vld_3 = 0x%x\n", data.mdata_w_vld_3);\
} while(0)

#define FIB_MSK_MDATA_W2_PRINT(data) {\
	printk(KERN_INFO "| mdata_w_2 = 0x%x\n", data.mdata_w_2);\
	printk(KERN_INFO "| mdata_w_vld_2 = 0x%x\n", data.mdata_w_vld_2);\
} while(0)

#define FIB_MSK_MDATA_W1_PRINT(data) {\
	printk(KERN_INFO "| mdata_w_1 = 0x%x\n", data.mdata_w_1);\
	printk(KERN_INFO "| mdata_w_vld_1 = 0x%x\n", data.mdata_w_vld_1);\
} while(0)

#define FIB_MSK_MDATA_W0_PRINT(data) {\
	printk(KERN_INFO "| mdata_w_0 = 0x%x\n", data.mdata_w_0);\
	printk(KERN_INFO "| mdata_w_vld_0 = 0x%x\n", data.mdata_w_vld_0);\
} while(0)

#define FIB_MSK_MAC_DA_PRINT(data) {\
	printk(KERN_INFO "| mac_da = %02x:%02x:%02x:%02x:%02x:%02x\n", data.mac_da_5,\
						data.mac_da_4,data.mac_da_3,\
						data.mac_da_2,data.mac_da_1,\
						data.mac_da_0);\
	printk(KERN_INFO "| mac_da_vld = 0x%x\n", data.mac_da_vld);\
} while(0)

#define FIB_MSK_MAC_SA_PRINT(data) {\
	printk(KERN_INFO "| mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\n", data.mac_sa_5,\
						data.mac_sa_4,data.mac_sa_3,\
						data.mac_sa_2,data.mac_sa_1,\
						data.mac_sa_0);\
	printk(KERN_INFO "| mac_sa_vld = 0x%x\n", data.mac_sa_vld);\
} while(0)

#define FIB_MSK_VLAN_PRINT(data) {\
	printk(KERN_INFO "| vlan_cnt = 0x%x\n", data.vlan_cnt);\
	printk(KERN_INFO "| top_tpid_enc = 0x%x\n", data.top_tpid_enc);\
	printk(KERN_INFO "| top_vid = 0x%x\n", data.top_vid);\
	printk(KERN_INFO "| top_dei = 0x%x\n", data.top_dei);\
	printk(KERN_INFO "| top_dei_sel = 0x%x\n", data.top_dei_sel);\
	printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", data.inner_tpid_enc);\
	printk(KERN_INFO "| inner_vid = 0x%x\n", data.inner_vid);\
	printk(KERN_INFO "| inner_dei = 0x%x\n", data.inner_dei);\
	printk(KERN_INFO "| inner_dei_sel = 0x%x\n", data.inner_dei_sel);\
	printk(KERN_INFO "| vlan_vld = 0x%x\n", data.vlan_vld);\
} while(0)

#define FIB_MSK_L3_EGRESS_IF_PRINT(data) {\
	printk(KERN_INFO "| l3_egress_if_vld = 0x%x\n", data.l3_egress_if_vld);\
	printk(KERN_INFO "| l2_format = 0x%x\n", data.l2_format);\
	printk(KERN_INFO "| l2_format_vld = 0x%x\n", data.l2_format_vld);\
	printk(KERN_INFO "| ip_mtu_enc = 0x%x\n", data.ip_mtu_enc);\
	printk(KERN_INFO "| ip_mtu_enc_vld = 0x%x\n", data.ip_mtu_enc_vld);\
	printk(KERN_INFO "| six_rd_ipda_from_v6 = 0x%x\n", data.six_rd_ipda_from_v6);\
	printk(KERN_INFO "| six_rd_ipda_ctrl_vld = 0x%x\n", data.six_rd_ipda_ctrl_vld);\
	printk(KERN_INFO "| ip_ttl = 0x%x\n", data.ip_ttl);\
	printk(KERN_INFO "| ip_ttl_cmd = 0x%x\n", data.ip_ttl_cmd);\
	printk(KERN_INFO "| ip_ttl_zero_discard_en = 0x%x\n", data.ip_ttl_zero_discard_en);\
	printk(KERN_INFO "| ip_ttl_vld = 0x%x\n", data.ip_ttl_vld);\
} while(0)

#define FIB_MSK_PPPOE_PRINT(data) {\
	printk(KERN_INFO "| pppoe_session_id = 0x%x\n", data.pppoe_session_id);\
	printk(KERN_INFO "| pppoe_set = 0x%x\n", data.pppoe_set);\
	printk(KERN_INFO "| pppoe_vld = 0x%x\n", data.pppoe_vld);\
} while(0)

#define FIB_MSK_IP_SA_H_PRINT(data) {\
	printk(KERN_INFO "| ip_sa_h_96 = %08x %08x %08x\n", data.ip_sa_3, data.ip_sa_2, data.ip_sa_1);\
	printk(KERN_INFO "| ip_sa_h_vld = 0x%x\n", data.ip_sa_h_vld);\
} while(0)

#define FIB_MSK_IP_SA_L_PRINT(data) {\
	printk(KERN_INFO "| ip_sa_l_32 = 0x%08x\n", data.ip_sa_0);\
	printk(KERN_INFO "| ip_sa_l_vld = 0x%x\n", data.ip_sa_l_vld);\
} while(0)

#define FIB_MSK_IP_DA_H_PRINT(data) {\
	printk(KERN_INFO "| ip_da_h_96 = %08x %08x %08x\n", data.ip_da_3, data.ip_da_2, data.ip_da_1);\
	printk(KERN_INFO "| ip_da_h_vld = 0x%x\n", data.ip_da_h_vld);\
} while(0)

#define FIB_MSK_IP_DA_L_PRINT(data) {\
	printk(KERN_INFO "| ip_da_l_32 = 0x%08x\n", data.ip_da_0);\
	printk(KERN_INFO "| ip_da_l_vld = 0x%x\n", data.ip_da_l_vld);\
} while(0)

#define FIB_MSK_L4_SP_PRINT(data) {\
	printk(KERN_INFO "| l4_sp = 0x%x\n", data.l4_sp);\
	printk(KERN_INFO "| l4_sp_vld = 0x%x\n", data.l4_sp_vld);\
} while(0)

#define FIB_MSK_L4_DP_PRINT(data) {\
	printk(KERN_INFO "| l4_dp = 0x%x\n", data.l4_dp);\
	printk(KERN_INFO "| l4_dp_vld = 0x%x\n", data.l4_dp_vld);\
} while(0)

static void __l3_cls_fib_tbl_entry_dump(aal_table_t *table, unsigned int start_idx, int count)
{
	cls_fib_t entry, *p_fib = NULL;
	ca_int32_t end_idx, i, j, status, sz;
	unsigned int refcnt, *ptr=NULL;
	ca_uint16_t short_fib=1;
	l3_cls_fib_mode_t fib_mode;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry))
		end_idx = table->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== L3 Classifier FIB Table ===========================================|\n");

	sz = sizeof(cls_fib_t);

	for (i = start_idx; i < end_idx; i++) {
		__l3_cls_fib_mode_get(table, i, &short_fib, &fib_mode);
		if (!short_fib)
			i = (i >>1)<<1;
		memset(&entry, 0, sz);
		/* read fib entry */
		if (ca_aal_table_sw_cache) {
			status = aal_entry_get_by_idx(table->table_id, i, entry.cls_fib_data.fib_tbl_data);
		} else {
			status = hw_entry_get(table, i, entry.cls_fib_data.fib_tbl_data);
		}

		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			printk(KERN_INFO "| index %04d | ref count %d | fib format %x\n", i, refcnt, fib_mode);

			/* print row data */
			ptr = (unsigned int *)&entry.cls_fib_data.fib_tbl_data[0];
			printk("| MSB  ");
			for (j = 6; j >= 0; j--) {
				printk("%08x ", ptr[j]);
			}
			printk("\n");

			/* read the next entry for standard fib */
			if (!short_fib) {
				i++;
				if (ca_aal_table_sw_cache) {
					status = aal_entry_get_by_idx(table->table_id, i, &entry.cls_fib_data.fib_tbl_data[1]);
				} else {
					status = hw_entry_get(table, i, &entry.cls_fib_data.fib_tbl_data[1]);
				}
				/* print row data */
				ptr = (unsigned int *)&entry.cls_fib_data.fib_tbl_data[1];
				printk("| MSB  ");
				for (j = 6; j >= 0; j--) {
					printk("%08x ", ptr[j]);
				}
				printk("\n");
			}

			/* print detail fields */
			p_fib = &entry;
			switch(fib_mode) {
				case CL_FIB_MOD_0:
					/* group 19 */
					FIB_MSK_CTRL_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 18 */
					FIB_MSK_DPID_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 17 */
					FIB_MSK_QOS_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 16 */
					FIB_MSK_POL_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 13 */
					FIB_MSK_MDATA_W1_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 09 */
					FIB_MSK_VLAN_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_PRINT(p_fib->cls_fib_data.fib_mod_0);
					break;
				case CL_FIB_MOD_1:
					/* group 19 */
					FIB_MSK_CTRL_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 18 */
					FIB_MSK_DPID_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 17 */
					FIB_MSK_QOS_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 16 */
					FIB_MSK_POL_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 13 */
					FIB_MSK_MDATA_W1_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 12 */
					FIB_MSK_MDATA_W0_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 09 */
					FIB_MSK_VLAN_PRINT(p_fib->cls_fib_data.fib_mod_1);
					break;
				case CL_FIB_MOD_2:
					/* group 19 */
					FIB_MSK_CTRL_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 18 */
					FIB_MSK_DPID_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 17 */
					FIB_MSK_QOS_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 16 */
					FIB_MSK_POL_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 15 */
					FIB_MSK_MDATA_W3_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 14 */
					FIB_MSK_MDATA_W2_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 13 */
					printk(KERN_INFO "| mdata_w_1 = 0x%x\n", (p_fib->cls_fib_data.fib_mod_2.mdata_w_1_1 << 7) | p_fib->cls_fib_data.fib_mod_2.mdata_w_1_0);
					printk(KERN_INFO "| mdata_w_vld_1 = 0x%x\n", p_fib->cls_fib_data.fib_mod_2.mdata_w_vld_1);
					/* group 12 */
					FIB_MSK_MDATA_W0_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 11 */
					FIB_MSK_MAC_DA_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 10 */
					FIB_MSK_MAC_SA_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 09 */
					FIB_MSK_VLAN_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 07 */
					FIB_MSK_PPPOE_PRINT(p_fib->cls_fib_data.fib_mod_2);
					break;
				case CL_FIB_MOD_3:
					/* group 19 */
					FIB_MSK_CTRL_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 18 */
					FIB_MSK_DPID_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 17 */
					FIB_MSK_QOS_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 16 */
					FIB_MSK_POL_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 13 */
					FIB_MSK_MDATA_W1_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 12 */
					FIB_MSK_MDATA_W0_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 09 */
					FIB_MSK_VLAN_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 07 */
					FIB_MSK_PPPOE_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 6 */
					FIB_MSK_IP_SA_H_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 5 */
					FIB_MSK_IP_SA_L_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 02 */
					FIB_MSK_L4_SP_PRINT(p_fib->cls_fib_data.fib_mod_3);
					break;
				case CL_FIB_MOD_4:
					/* group 19 */
					FIB_MSK_CTRL_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 18 */
					FIB_MSK_DPID_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 17 */
					FIB_MSK_QOS_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 16 */
					FIB_MSK_POL_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 13 */
					FIB_MSK_MDATA_W1_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 12 */
					FIB_MSK_MDATA_W0_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 09 */
					FIB_MSK_VLAN_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 07 */
					FIB_MSK_PPPOE_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 4 */
					FIB_MSK_IP_DA_H_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 3 */
					FIB_MSK_IP_DA_L_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 01 */
					FIB_MSK_L4_DP_PRINT(p_fib->cls_fib_data.fib_mod_4);
					break;
				default:
					;
			}
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			//cond_resched();
		}
	}
}

static void __l3_cls_fib_tbl_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_fib_tbl_entry_dump(&l3_cls_fib_table, start_idx, count);
}

#if defined(CONFIG_LUNA_G3_SERIES)
#define FIB_MSK_CTRL_USED_PRINT(data) {\
	if((data.t2_ctrl_vld == 1) || (data.t3_ctrl_vld == 1) || (data.t4_ctrl_vld == 1) || (data.t5_ctrl_vld == 1))\
		printk(KERN_INFO "| t2_ctrl = 0x%x, t3_ctrl = 0x%x, t4_ctrl = 0x%x, t5_ctrl = 0x%x\n", \
		data.t2_ctrl_vld?data.t2_ctrl:0xffff, data.t3_ctrl_vld?data.t3_ctrl:0xffff, data.t4_ctrl_vld?data.t4_ctrl:0xffff, data.t5_ctrl_vld?data.t5_ctrl:0xffff);\
	if((data.stage2_ctrl_vld == 1) || (data.stage3_ctrl_vld == 1)) printk(KERN_INFO "| stage2_ctrl = 0x%x, stage3_ctrl = 0x%x\n", data.stage2_ctrl_vld?data.stage2_ctrl:0xffff, data.stage3_ctrl_vld?data.stage3_ctrl:0xffff);\
	if(data.keep_orig_pkt_vld == 1) printk(KERN_INFO "| keep_orig_pkt = 0x%x\n", data.keep_orig_pkt);\
	if(data.keep_ts_vld == 1) printk(KERN_INFO "| keep_ts_en = 0x%x\n", data.keep_ts_en);\
	if(data.pop_l3_vld == 1) printk(KERN_INFO "| pop_l3_en = 0x%x, pop_l3_chk_ecn_en = 0x%x\n", data.pop_l3_en, data.pop_l3_chk_ecn_en);\
	if(data.cache_ctrl != 0) printk(KERN_INFO "| cache_ctrl = 0x%x\n", data.cache_ctrl);\
	if(data.chk_msk_ptr != 0) printk(KERN_INFO "| chk_msk_ptr = 0x%x\n", data.chk_msk_ptr);\
} while(0)

#define FIB_MSK_DPID_USED_PRINT(data) {\
	if(data.dpid_vld == 1) printk(KERN_INFO "| mc(1:mcgid=mcgid;0:mcgid=ldpid) = 0x%x, mcgid = 0x%x, permit(1:forward;0:drop) = 0x%x, deepq = 0x%x, dpid_pri = 0x%x\n", \
		data.mc, data.mcgid, data.permit, data.deepq, data.dpid_pri);\
	if((data.no_drop_vld == 1) || (data.mrr_vld == 1)) printk(KERN_INFO "| no_drop(set no_drop flag in hdr_a) = 0x%x, mrr_en = 0x%x\n", data.no_drop_vld?data.no_drop:0xffff, data.mrr_vld?data.mrr_en:0xffff);\
} while(0)

#define FIB_MSK_QOS_USED_PRINT(data) {\
	if(data.cos_sel != 0) printk(KERN_INFO "| cos = 0x%x, cos_sel = 0x%x\n", data.cos, data.cos_sel);\
	if(data.cos_table_sel != 0) printk(KERN_INFO "| cos_table_sel = 0x%x\n", data.cos_table_sel);\
	if((data.top_802_1p_sel != 0) || (data.inner_802_1p_sel != 0)) printk(KERN_INFO "| qos_802_1p_table_sel = 0x%x\n", data.qos_802_1p_table_sel);\
	if(data.top_802_1p_sel != 0) printk(KERN_INFO "| top_802_1p = 0x%x, top_802_1p_sel = 0x%x\n", data.top_802_1p, data.top_802_1p_sel);\
	if(data.inner_802_1p_sel != 0) printk(KERN_INFO "| inner_802_1p = 0x%x, inner_802_1p_sel = 0x%x\n", data.inner_802_1p, data.inner_802_1p_sel);\
	if(data.ip_dscp_update_en != 0) printk(KERN_INFO "| dscp_table_sel = 0x%x, ip_dscp = 0x%x, ip_dscp_sel = 0x%x\n", data.dscp_table_sel, data.ip_dscp, data.ip_dscp_sel);\
	if(data.ip_dscp_update_en != 0) printk(KERN_INFO "| ip_dscp_update_en(0:disable;1;enable/[0]:dscp[2:0];[1]:dscp[5:3]) = 0x%x\n", data.ip_dscp_update_en);\
	if(data.ip_dscp_marked_vld == 1) printk(KERN_INFO "| ip_dscp_markdown_en = 0x%x, ip_dscp_marked_down = 0x%x\n", data.ip_dscp_markdown_en, data.ip_dscp_marked_down);\
	if(data.ip_tos_vld == 1) printk(KERN_INFO "| ip_tos_6 = 0x%x\n", data.ip_tos_6);\
	if(data.ip_ecn_vld == 1) printk(KERN_INFO "| ip_ecn_en = 0x%x\n", data.ip_ecn_en);\
} while(0)

#define FIB_MSK_POL_USED_PRINT(data) {\
	if(data.pol_vld == 1) printk(KERN_INFO "| pol_grp_id = 0x%x, pol_table_sel = 0x%x, pol_sel = 0x%x, pol_base = 0x%x, pol_en = 0x%x, pol_all_bypass = 0x%x, qos_premark = 0x%x\n", \
		data.pol_grp_id, data.pol_table_sel, data.pol_sel, data.pol_base, data.pol_en, data.pol_all_bypass, data.qos_premark);\
} while(0)

#define FIB_MSK_MDATA_W23_USED_PRINT(data) {\
	if((data.mdata_w_vld_2 != 0) || (data.mdata_w_vld_3 != 0)) printk(KERN_INFO "| mdata_w_2 = 0x%x, mdata_w_vld_2 = 0x%x, mdata_w_3 = 0x%x, mdata_w_vld_3 = 0x%x\n", data.mdata_w_2, data.mdata_w_vld_2, data.mdata_w_3, data.mdata_w_vld_3);\
} while(0)

#define FIB_MSK_MDATA_W1_USED_PRINT(data) {\
	if(data.mdata_w_vld_1 != 0) printk(KERN_INFO "| mdata_w_1 = 0x%x, mdata_w_vld_1 = 0x%x\n", data.mdata_w_1, data.mdata_w_vld_1);\
} while(0)

#define FIB_MSK_MDATA_W0_USED_PRINT(data) {\
	if(data.mdata_w_vld_0 != 0) printk(KERN_INFO "| mdata_w_0 = 0x%x, mdata_w_vld_0 = 0x%x\n", data.mdata_w_0, data.mdata_w_vld_0);\
} while(0)

#define FIB_MSK_MAC_DA_USED_PRINT(data) {\
	if(data.mac_da_vld == 1) printk(KERN_INFO "| mac_da = %02x:%02x:%02x:%02x:%02x:%02x\n", data.mac_da_5,\
						data.mac_da_4,data.mac_da_3,\
						data.mac_da_2,data.mac_da_1,\
						data.mac_da_0);\
} while(0)
	
#define FIB_MSK_MAC_SA_USED_PRINT(data) {\
	if(data.mac_sa_vld == 1) printk(KERN_INFO "| mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\n", data.mac_sa_5,\
						data.mac_sa_4,data.mac_sa_3,\
						data.mac_sa_2,data.mac_sa_1,\
						data.mac_sa_0);\
} while(0)

#define FIB_MSK_VLAN_USED_PRINT(data) {\
	if(data.vlan_vld == 1) printk(KERN_INFO "| vlan_cnt = 0x%x, top_tpid_enc = 0x%x, top_vid = 0x%x, top_dei = 0x%x, top_dei_sel = 0x%x\n", \
		data.vlan_cnt, data.top_tpid_enc, data.top_vid, data.top_dei, data.top_dei_sel);\
	if(data.vlan_vld == 1) printk(KERN_INFO "| inner_tpid_enc = 0x%x, inner_vid = 0x%x, inner_dei = 0x%x, inner_dei_sel = 0x%x\n", \
		data.inner_tpid_enc, data.inner_vid, data.inner_dei, data.inner_dei_sel);\
} while(0)

#define FIB_MSK_L3_EGRESS_IF_USED_PRINT(data) {\
	if(data.l3_egress_if_vld != 0) printk(KERN_INFO "| l3_egress_if_vld = 0x%x\n", data.l3_egress_if_vld);\
	if(data.l2_format_vld == 1) printk(KERN_INFO "| l2_format = 0x%x\n", data.l2_format);\
	if(data.ip_mtu_enc_vld == 1) printk(KERN_INFO "| ip_mtu_enc = 0x%x\n", data.ip_mtu_enc);\
	if(data.six_rd_ipda_ctrl_vld == 1) printk(KERN_INFO "| six_rd_ipda_from_v6 = 0x%x\n", data.six_rd_ipda_from_v6);\
	if(data.ip_ttl_vld == 1) printk(KERN_INFO "| ip_ttl = 0x%x, ip_ttl_cmd = 0x%x, ip_ttl_zero_discard_en = 0x%x\n", data.ip_ttl, data.ip_ttl_cmd, data.ip_ttl_zero_discard_en);\
} while(0)

#define FIB_MSK_PPPOE_USED_PRINT(data) {\
	if(data.pppoe_vld == 1) printk(KERN_INFO "| pppoe_session_id = 0x%x, pppoe_set(0:unchange;1:set) = 0x%x\n", data.pppoe_session_id, data.pppoe_set);\
} while(0)
	
#define FIB_MSK_IP_SA_H_USED_PRINT(data) {\
	if(data.ip_sa_h_vld == 1) printk(KERN_INFO "| ip_sa_h_96 = %08x %08x %08x\n", data.ip_sa_3, data.ip_sa_2, data.ip_sa_1);\
} while(0)
	
#define FIB_MSK_IP_SA_L_USED_PRINT(data) {\
	if(data.ip_sa_l_vld == 1) printk(KERN_INFO "| ip_sa_l_32 = 0x%08x\n", data.ip_sa_0);\
} while(0)
	
#define FIB_MSK_IP_DA_H_USED_PRINT(data) {\
	if(data.ip_da_h_vld == 1) printk(KERN_INFO "| ip_da_h_96 = %08x %08x %08x\n", data.ip_da_3, data.ip_da_2, data.ip_da_1);\
} while(0)
	
#define FIB_MSK_IP_DA_L_USED_PRINT(data) {\
	if(data.ip_da_l_vld == 1) printk(KERN_INFO "| ip_da_l_32 = 0x%08x\n", data.ip_da_0);\
} while(0)
	
#define FIB_MSK_L4_SP_USED_PRINT(data) {\
	if(data.l4_sp_vld == 1) printk(KERN_INFO "| l4_sp = 0x%x\n", data.l4_sp);\
} while(0)
	
#define FIB_MSK_L4_DP_USED_PRINT(data) {\
	if(data.l4_dp_vld == 1) printk(KERN_INFO "| l4_dp = 0x%x\n", data.l4_dp);\
} while(0)


static void __l3_cls_fib_tbl_used_entry_dump(aal_table_t *table, unsigned int start_idx, int count)
{
	cls_fib_t entry, *p_fib = NULL;
	ca_int32_t end_idx, i, j, status, sz;
	unsigned int refcnt, *ptr=NULL;
	ca_uint16_t short_fib=1;
	l3_cls_fib_mode_t fib_mode;
	int keep_print = 0;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry))
		end_idx = table->max_entry;
	else
		end_idx = start_idx + count;

	if(start_idx >= 512)
		printk(KERN_INFO "|===== L3 Classifier Default FIB Table =====================================|\n");

	sz = sizeof(cls_fib_t);

	for (i = start_idx; i < end_idx; i++) {
		__l3_cls_fib_mode_get(table, i, &short_fib, &fib_mode);
		if (!short_fib)
			i = (i >>1)<<1;
		memset(&entry, 0, sz);
		/* read fib entry */
		if (ca_aal_table_sw_cache) {
			status = aal_entry_get_by_idx(table->table_id, i, entry.cls_fib_data.fib_tbl_data);
		} else {
			status = hw_entry_get(table, i, entry.cls_fib_data.fib_tbl_data);
		}

		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR)) {
			//printk(KERN_INFO "| index %04d | NOT USED\n", i);
			//printk(KERN_INFO "|------------------------------------------------------------------|\n");
			continue;
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			if((fib_mode < CL_FIB_MOD_0) || (fib_mode > CL_FIB_MOD_4))
				continue;
			keep_print = 0;
			
			/* print row data */
			ptr = (unsigned int *)&entry.cls_fib_data.fib_tbl_data[0];
			//printk("| MSB  ");
			for (j = 6; j >= 0; j--) {
				//printk("%08x ", ptr[j]);
				if((keep_print == 0) && (ptr[j] != 0))
					keep_print = 1;
			}
			//printk("\n");

			/* read the next entry for standard fib */
			if (!short_fib) {
				i++;
				if (ca_aal_table_sw_cache) {
					status = aal_entry_get_by_idx(table->table_id, i, &entry.cls_fib_data.fib_tbl_data[1]);
				} else {
					status = hw_entry_get(table, i, &entry.cls_fib_data.fib_tbl_data[1]);
				}
				/* print row data */
				ptr = (unsigned int *)&entry.cls_fib_data.fib_tbl_data[1];
				//printk("| MSB  ");
				for (j = 6; j >= 0; j--) {
					//printk("%08x ", ptr[j]);
					if((keep_print == 0) && (ptr[j] != 0))
						keep_print = 1;
				}
				//printk("\n");
			}

			if(keep_print != 1) {
				printk(KERN_INFO "|+ FIB (index %04d) +\n", (!short_fib)?(i-1):(i));
				continue;
			}
			printk(KERN_INFO "|+ FIB (index %04d | ref count %d | fib format %x) +\n", (!short_fib)?(i-1):(i), refcnt, fib_mode);

			/* print detail fields */
			p_fib = &entry;
			switch(fib_mode) {
				case CL_FIB_MOD_0:
					/* group 19 */
					FIB_MSK_CTRL_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 18 */
					FIB_MSK_DPID_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 17 */
					FIB_MSK_QOS_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 16 */
					FIB_MSK_POL_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 13 */
					FIB_MSK_MDATA_W1_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 09 */
					FIB_MSK_VLAN_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_USED_PRINT(p_fib->cls_fib_data.fib_mod_0);
					break;
				case CL_FIB_MOD_1:
					/* group 19 */
					FIB_MSK_CTRL_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 18 */
					FIB_MSK_DPID_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 17 */
					FIB_MSK_QOS_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 16 */
					FIB_MSK_POL_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 13 */
					FIB_MSK_MDATA_W1_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 12 */
					FIB_MSK_MDATA_W0_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					/* group 09 */
					FIB_MSK_VLAN_USED_PRINT(p_fib->cls_fib_data.fib_mod_1);
					break;
				case CL_FIB_MOD_2:
					/* group 19 */
					FIB_MSK_CTRL_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 18 */
					FIB_MSK_DPID_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 17 */
					FIB_MSK_QOS_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 16 */
					FIB_MSK_POL_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 14 W2 and group 15 W3 */
					FIB_MSK_MDATA_W23_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 13 */
					if(p_fib->cls_fib_data.fib_mod_2.mdata_w_vld_1 != 0) printk(KERN_INFO "| mdata_w_1 = 0x%x, mdata_w_vld_1 = 0x%x\n", ((p_fib->cls_fib_data.fib_mod_2.mdata_w_1_1 << 7) | p_fib->cls_fib_data.fib_mod_2.mdata_w_1_0), p_fib->cls_fib_data.fib_mod_2.mdata_w_vld_1);
					/* group 12 */
					FIB_MSK_MDATA_W0_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 11 */
					FIB_MSK_MAC_DA_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 10 */
					FIB_MSK_MAC_SA_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 09 */
					FIB_MSK_VLAN_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					/* group 07 */
					FIB_MSK_PPPOE_USED_PRINT(p_fib->cls_fib_data.fib_mod_2);
					break;
				case CL_FIB_MOD_3:
					/* group 19 */
					FIB_MSK_CTRL_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 18 */
					FIB_MSK_DPID_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 17 */
					FIB_MSK_QOS_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 16 */
					FIB_MSK_POL_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 13 */
					FIB_MSK_MDATA_W1_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 12 */
					FIB_MSK_MDATA_W0_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 09 */
					FIB_MSK_VLAN_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 07 */
					FIB_MSK_PPPOE_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 6 */
					FIB_MSK_IP_SA_H_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 5 */
					FIB_MSK_IP_SA_L_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					/* group 02 */
					FIB_MSK_L4_SP_USED_PRINT(p_fib->cls_fib_data.fib_mod_3);
					break;
				case CL_FIB_MOD_4:
					/* group 19 */
					FIB_MSK_CTRL_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 18 */
					FIB_MSK_DPID_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 17 */
					FIB_MSK_QOS_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 16 */
					FIB_MSK_POL_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 13 */
					FIB_MSK_MDATA_W1_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 12 */
					FIB_MSK_MDATA_W0_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 09 */
					FIB_MSK_VLAN_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 08 */
					FIB_MSK_L3_EGRESS_IF_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 07 */
					FIB_MSK_PPPOE_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 4 */
					FIB_MSK_IP_DA_H_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 3 */
					FIB_MSK_IP_DA_L_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					/* group 01 */
					FIB_MSK_L4_DP_USED_PRINT(p_fib->cls_fib_data.fib_mod_4);
					break;
				default:
					;
			}
			if(start_idx >= 512)
				printk(KERN_INFO "|------------------------------------------------------------------|\n");
			//cond_resched();
		}
	}
}

static void __l3_cls_fib_tbl_used_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_fib_tbl_used_entry_dump(&l3_cls_fib_table, start_idx, count);
}
#endif

void l3fe_debug_dump_cls_fib(cls_fib_dbg_t *entry)
{
	ca_uint32_t *ptr=NULL;
	ca_int16_t j;

	/* print row data */
	ptr = (unsigned int *)entry;
	printk("| MSB  ");
	for (j = 27; j >= 0; j--) {
		printk("%08x ", ptr[j]);
		if (j%7 == 0) {
			printk("\n");
			if (j != 0)
				printk("| MSB  ");
		}
	}

	printk(KERN_INFO "| cls_hit = %d\n", entry->cls_hit);
	printk(KERN_INFO "| cls_std_fib = %d\n", entry->cls_std_fib);
	printk(KERN_INFO "| cls_hit_addr = %d\n", entry->cls_hit_addr);

	if (!entry->cls_hit)
		return;

	/* group 19 */
	FIB_MSK_CTRL_PRINT(entry->cls_fib_data);
	/* group 18 */
	FIB_MSK_DPID_PRINT(entry->cls_fib_data);
	/* group 17 */
	FIB_MSK_QOS_PRINT(entry->cls_fib_data);
	/* group 16 */
	FIB_MSK_POL_PRINT(entry->cls_fib_data);
	/* group 15 */
	FIB_MSK_MDATA_W3_PRINT(entry->cls_fib_data);
	/* group 14 */
	FIB_MSK_MDATA_W2_PRINT(entry->cls_fib_data);
	/* group 13 */
	FIB_MSK_MDATA_W1_PRINT(entry->cls_fib_data);
	/* group 12 */
	FIB_MSK_MDATA_W0_PRINT(entry->cls_fib_data);
	/* group 11 */
	FIB_MSK_MAC_DA_PRINT(entry->cls_fib_data);
	/* group 10 */
	FIB_MSK_MAC_SA_PRINT(entry->cls_fib_data);
	/* group 09 */
	FIB_MSK_VLAN_PRINT(entry->cls_fib_data);
	/* group 08 */
	FIB_MSK_L3_EGRESS_IF_PRINT(entry->cls_fib_data);
	/* group 07 */
	FIB_MSK_PPPOE_PRINT(entry->cls_fib_data);
	/* group 6 */
	FIB_MSK_IP_SA_H_PRINT(entry->cls_fib_data);
	/* group 5 */
	FIB_MSK_IP_SA_L_PRINT(entry->cls_fib_data);
	/* group 4 */
	FIB_MSK_IP_DA_H_PRINT(entry->cls_fib_data);
	/* group 3 */
	FIB_MSK_IP_DA_L_PRINT(entry->cls_fib_data);
	/* group 02 */
	FIB_MSK_L4_SP_PRINT(entry->cls_fib_data);
	/* group 01 */
	FIB_MSK_L4_DP_PRINT(entry->cls_fib_data);
}

static aal_table_t l3_cls_fib_table = {
	.table_id = AAL_TABLE_L3_CLS_FIB,
	.table_name = "L3 CLS FIB",
	.max_entry = L3_CLS_FIB_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cls_fib_tbl_entry_t),	/* TO Do: need to define aal_l3_cls.h */
	.access_reg = L3FE_CLS_FIB_TBL_ACCESS,
	.data_reg_num = 7,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = __l3_cls_fib_tbl_entry_add_by_idx,
		.entry_del = __l3_cls_fib_tbl_entry_del,
		.entry_set = __l3_cls_fib_tbl_entry_set,
		.entry_get = __l3_cls_fib_tbl_entry_get,
		.entry_get_by_idx = __l3_cls_fib_tbl_entry_get_by_idx,
		.table_flush = __l3_cls_fib_table_flush,
		.get_avail_count = __l3_cls_fib_tbl_get_avail_count,
		.entry_print = __l3_cls_fib_tbl_entry_print,
#if defined(CONFIG_LUNA_G3_SERIES)
		.used_entry_print = __l3_cls_fib_tbl_used_entry_print,
#endif
	},
};

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static int __hgu_8279_l3_cls_fib_tbl_entry_add_by_idx(void *entry, unsigned int fib_idx)
{
	return generic_entry_add_by_idx(&hgu_8279_l3_cls_fib_table, entry, fib_idx);
}

static int __hgu_8279_l3_cls_fib_tbl_entry_del(unsigned int fib_idx)
{
	return generic_entry_del(&hgu_8279_l3_cls_fib_table, fib_idx);
}

static int __hgu_8279_l3_cls_fib_tbl_entry_set(unsigned int fib_idx, void *entry)
{
	return generic_entry_set(&hgu_8279_l3_cls_fib_table, fib_idx, entry);
}

static int __hgu_8279_l3_cls_fib_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&hgu_8279_l3_cls_fib_table, entry, rslt_idx);
}

static int __hgu_8279_l3_cls_fib_tbl_entry_get_by_idx(unsigned int fib_idx, void *entry)
{
	return generic_entry_get_by_idx(&hgu_8279_l3_cls_fib_table, fib_idx, entry);
}

static int __hgu_8279_l3_cls_fib_table_flush(void)
{
	return generic_table_flush(&hgu_8279_l3_cls_fib_table);
}

static int __hgu_8279_l3_cls_fib_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&hgu_8279_l3_cls_fib_table);
}

static void __hgu_8279_l3_cls_fib_tbl_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_fib_tbl_entry_dump(&hgu_8279_l3_cls_fib_table, start_idx, count);
}

static aal_table_t hgu_8279_l3_cls_fib_table = {
	.table_id = AAL_TABLE_8279_L3_CLS_FIB,
	.table_name = "L3 CLS FIB",
	.max_entry = HGU_L3_CLS_FIB_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cls_fib_tbl_entry_t),	/* TO Do: need to define aal_l3_cls.h */
	.access_reg = L3FE_CLS_FIB_TBL_ACCESS,
	.data_reg_num = 7,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = __hgu_8279_l3_cls_fib_tbl_entry_add_by_idx,
		.entry_del = __hgu_8279_l3_cls_fib_tbl_entry_del,
		.entry_set = __hgu_8279_l3_cls_fib_tbl_entry_set,
		.entry_get = __hgu_8279_l3_cls_fib_tbl_entry_get,
		.entry_get_by_idx = __hgu_8279_l3_cls_fib_tbl_entry_get_by_idx,
		.table_flush = __hgu_8279_l3_cls_fib_table_flush,
		.get_avail_count = __hgu_8279_l3_cls_fib_tbl_get_avail_count,
		.entry_print = __hgu_8279_l3_cls_fib_tbl_entry_print,
	},
};
#endif

/***************************************************************************
 * AAL table, L3 CLS KEY TBL
 ***************************************************************************/

static int __l3_cls_key_tbl_entry_add_by_idx(void *entry, unsigned int cls_idx)
{
	return generic_entry_add_by_idx(&l3_cls_key_table, entry, cls_idx);
}

static int __l3_cls_key_tbl_entry_del(unsigned int cls_idx)
{
	return generic_entry_del(&l3_cls_key_table, cls_idx);
}

static int __l3_cls_key_tbl_entry_set(unsigned int cls_idx, void *entry)
{
	return generic_entry_set(&l3_cls_key_table, cls_idx, entry);
}

static int __l3_cls_key_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&l3_cls_key_table, entry, rslt_idx);
}

static int __l3_cls_key_tbl_entry_get_by_idx(unsigned int cls_idx, void *entry)
{
	return generic_entry_get_by_idx(&l3_cls_key_table, cls_idx, entry);
}

static int __l3_cls_key_table_flush(void)
{
	return generic_table_flush(&l3_cls_key_table);
}

static int __l3_cls_key_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&l3_cls_key_table);
}

#define CL_IF_ID_KEY_PRINT(o, key_idx) {\
	printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", o.cl_if_id_key.msk_ip_fragment_##key_idx);\
	printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_if_id_key.ip_fragment_##key_idx);\
	printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", o.cl_if_id_key.msk_ip_da_sa_equal_##key_idx);\
	printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", o.cl_if_id_key.ip_da_sa_equal_##key_idx);\
	printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", o.cl_if_id_key.msk_ip_da_mc_##key_idx);\
	printk(KERN_INFO "| ip_da_mc = 0x%x\n", o.cl_if_id_key.ip_da_mc_##key_idx);\
	printk(KERN_INFO "| msk_ip_ver = 0x%x\n", o.cl_if_id_key.msk_ip_ver_##key_idx);\
	printk(KERN_INFO "| ip_ver = 0x%x\n", o.cl_if_id_key.ip_ver_##key_idx);\
	printk(KERN_INFO "| msk_ip_vld = 0x%x\n", o.cl_if_id_key.msk_ip_vld_##key_idx);\
	printk(KERN_INFO "| ip_vld = 0x%x\n", o.cl_if_id_key.ip_vld_##key_idx);\
	printk(KERN_INFO "| msk_pppoe_session = 0x%x\n", o.cl_if_id_key.msk_pppoe_session_##key_idx);\
	printk(KERN_INFO "| pppoe_session_id = 0x%x\n", o.cl_if_id_key.pppoe_session_id_##key_idx);\
	printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", o.cl_if_id_key.msk_top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_if_id_key.top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| msk_top_vid = 0x%x\n", o.cl_if_id_key.msk_top_vid_##key_idx);\
	printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_if_id_key.top_vid_##key_idx);\
	printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", o.cl_if_id_key.msk_top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_if_id_key.top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", o.cl_if_id_key.msk_vlan_cnt_##key_idx);\
	printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_if_id_key.vlan_cnt_##key_idx);\
	printk(KERN_INFO "| msk_ethertype_enc = 0x%x\n", o.cl_if_id_key.msk_ethertype_enc_##key_idx);\
	printk(KERN_INFO "| ethertype_enc = 0x%x\n", o.cl_if_id_key.ethertype_enc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_ip_mc = 0x%x\n", o.cl_if_id_key.msk_mac_da_ip_mc_##key_idx);\
	printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", o.cl_if_id_key.mac_da_ip_mc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_mc = 0x%x\n", o.cl_if_id_key.msk_mac_da_mc_##key_idx);\
	printk(KERN_INFO "| mac_da_mc = 0x%x\n", o.cl_if_id_key.mac_da_mc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_bc = 0x%x\n", o.cl_if_id_key.msk_mac_da_bc_##key_idx);\
	printk(KERN_INFO "| mac_da_bc = 0x%x\n", o.cl_if_id_key.mac_da_bc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_an_hit = 0x%x\n", o.cl_if_id_key.msk_mac_da_an_hit_##key_idx);\
	printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", o.cl_if_id_key.mac_da_an_hit_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_an_sel = 0x%x\n", o.cl_if_id_key.msk_mac_da_an_sel_##key_idx);\
	printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", o.cl_if_id_key.mac_da_an_sel_##key_idx);\
	printk(KERN_INFO "| msk_o_lspid = 0x%x\n", o.cl_if_id_key.msk_o_lspid_##key_idx);\
	printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_if_id_key.o_lspid_##key_idx);\
	printk(KERN_INFO "| msk_lspid = 0x%x\n", o.cl_if_id_key.msk_lspid_##key_idx);\
	printk(KERN_INFO "| lspid = 0x%x\n", o.cl_if_id_key.lspid_##key_idx);\
} while(0)

#define CL_IPV4_SHORT_KEY_PRINT(o, key_idx) {\
	printk(KERN_INFO "| msk_l4_port = 0x%x\n", o.cl_ipv4_short_key.msk_l4_port_##key_idx);\
	printk(KERN_INFO "| l4_dp = 0x%x\n", o.cl_ipv4_short_key.l4_dp_##key_idx);\
	printk(KERN_INFO "| l4_sp = 0x%x\n", o.cl_ipv4_short_key.l4_sp_##key_idx);\
	printk(KERN_INFO "| msk_l4_vld = 0x%x\n", o.cl_ipv4_short_key.msk_l4_vld_##key_idx);\
	printk(KERN_INFO "| l4_vld = 0x%x\n", o.cl_ipv4_short_key.l4_vld_##key_idx);\
	printk(KERN_INFO "| msk_l3_chksum_err = 0x%x\n", o.cl_ipv4_short_key.msk_l3_chksum_err_##key_idx);\
	printk(KERN_INFO "| l3_chksum_err = 0x%x\n", o.cl_ipv4_short_key.l3_chksum_err_##key_idx);\
	printk(KERN_INFO "| msk_spi = 0x%x\n", o.cl_ipv4_short_key.msk_spi_##key_idx);\
	printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", o.cl_ipv4_short_key.msk_ip_fragment_##key_idx);\
	printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_ipv4_short_key.ip_fragment_##key_idx);\
	printk(KERN_INFO "| ip_addr_type = 0x%x\n", o.cl_ipv4_short_key.ip_addr_type_##key_idx);\
	printk(KERN_INFO "| msk_ip_addr_l = 0x%x\n", o.cl_ipv4_short_key.msk_ip_addr_l_##key_idx);\
	printk(KERN_INFO "| ip_addr = 0x%08x\n", o.cl_ipv4_short_key.ip_addr_##key_idx);\
	printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", o.cl_ipv4_short_key.msk_ip_da_sa_equal_##key_idx);\
	printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", o.cl_ipv4_short_key.ip_da_sa_equal_##key_idx);\
	printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", o.cl_ipv4_short_key.msk_ip_da_mc_##key_idx);\
	printk(KERN_INFO "| ip_da_mc = 0x%x\n", o.cl_ipv4_short_key.ip_da_mc_##key_idx);\
	printk(KERN_INFO "| msk_ip_protocol = 0x%x\n", o.cl_ipv4_short_key.msk_ip_protocol_##key_idx);\
	printk(KERN_INFO "| ip_protocol = 0x%x\n", o.cl_ipv4_short_key.ip_protocol_##key_idx);\
	printk(KERN_INFO "| msk_ip_ecn = 0x%x\n", o.cl_ipv4_short_key.msk_ip_ecn_##key_idx);\
	printk(KERN_INFO "| ip_ecn = 0x%x\n", o.cl_ipv4_short_key.ip_ecn_##key_idx);\
	printk(KERN_INFO "| msk_dscp = 0x%x\n", o.cl_ipv4_short_key.msk_dscp_##key_idx);\
	printk(KERN_INFO "| ip_dscp = 0x%x\n", o.cl_ipv4_short_key.ip_dscp_##key_idx);\
	printk(KERN_INFO "| msk_top_dei = 0x%x\n", o.cl_ipv4_short_key.msk_top_dei_##key_idx);\
	printk(KERN_INFO "| top_dei = 0x%x\n", o.cl_ipv4_short_key.top_dei_##key_idx);\
	printk(KERN_INFO "| msk_top_vl_802_1p = 0x%x\n", o.cl_ipv4_short_key.msk_top_vl_802_1p_##key_idx);\
	printk(KERN_INFO "| top_802_1p = 0x%x\n", o.cl_ipv4_short_key.top_802_1p_##key_idx);\
	printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", o.cl_ipv4_short_key.msk_top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_ipv4_short_key.top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| msk_top_vid = 0x%x\n", o.cl_ipv4_short_key.msk_top_vid_##key_idx);\
	printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_ipv4_short_key.top_vid_##key_idx);\
	printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", o.cl_ipv4_short_key.msk_top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_ipv4_short_key.top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", o.cl_ipv4_short_key.msk_vlan_cnt_##key_idx);\
	printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_ipv4_short_key.vlan_cnt_##key_idx);\
	printk(KERN_INFO "| msk_o_lspid = 0x%x\n", o.cl_ipv4_short_key.msk_o_lspid_##key_idx);\
	printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_ipv4_short_key.o_lspid_##key_idx);\
	printk(KERN_INFO "| msk_lspid = 0x%x\n", o.cl_ipv4_short_key.msk_lspid_##key_idx);\
	printk(KERN_INFO "| lspid = 0x%x\n", o.cl_ipv4_short_key.lspid_##key_idx);\
} while(0)

#define CL_SPCL_PKT_KEY_PRINT(o, key_idx) {\
	printk(KERN_INFO "| spcl_pkt_hdr_mtch_msk = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_msk_##key_idx);\
	printk(KERN_INFO "| spcl_pkt_hdr_mtch = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_##key_idx);\
	printk(KERN_INFO "| spcl_pkt_enc_msk = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_enc_msk_##key_idx);\
	printk(KERN_INFO "| spcl_pkt_enc = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_enc_##key_idx);\
	printk(KERN_INFO "| msk_icmp_type = 0x%x\n", o.cl_spcl_pkt_key.msk_icmp_type_##key_idx);\
	printk(KERN_INFO "| icmp_type = 0x%x\n", o.cl_spcl_pkt_key.icmp_type_##key_idx);\
	printk(KERN_INFO "| msk_icmp_vld = 0x%x\n", o.cl_spcl_pkt_key.msk_icmp_vld_##key_idx);\
	printk(KERN_INFO "| icmp_vld = 0x%x\n", o.cl_spcl_pkt_key.icmp_vld_##key_idx);\
	printk(KERN_INFO "| msk_ipv6_doh = 0x%x\n", o.cl_spcl_pkt_key.msk_ipv6_doh_##key_idx);\
	printk(KERN_INFO "| ipv6_doh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_doh_##key_idx);\
	printk(KERN_INFO "| msk_ipv6_rh = 0x%x\n", o.cl_spcl_pkt_key.msk_ipv6_rh_##key_idx);\
	printk(KERN_INFO "| ipv6_rh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_rh_##key_idx);\
	printk(KERN_INFO "| msk_ipv6_hbh = 0x%x\n", o.cl_spcl_pkt_key.msk_ipv6_hbh_##key_idx);\
	printk(KERN_INFO "| ipv6_hbh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_hbh_##key_idx);\
	printk(KERN_INFO "| msk_ipv6_ndp = 0x%x\n", o.cl_spcl_pkt_key.msk_ipv6_ndp_##key_idx);\
	printk(KERN_INFO "| ipv6_ndp = 0x%x\n", o.cl_spcl_pkt_key.ipv6_ndp_##key_idx);\
	printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", o.cl_spcl_pkt_key.msk_ip_fragment_##key_idx);\
	printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_spcl_pkt_key.ip_fragment_##key_idx);\
	printk(KERN_INFO "| msk_ip_options = 0x%x\n", o.cl_spcl_pkt_key.msk_ip_options_##key_idx);\
	printk(KERN_INFO "| ip_options = 0x%x\n", o.cl_spcl_pkt_key.ip_options_##key_idx);\
	printk(KERN_INFO "| msk_ip_protocol = 0x%x\n", o.cl_spcl_pkt_key.msk_ip_protocol_##key_idx);\
	printk(KERN_INFO "| ip_protocol = 0x%x\n", o.cl_spcl_pkt_key.ip_protocol_##key_idx);\
	printk(KERN_INFO "| msk_top_dei = 0x%x\n", o.cl_spcl_pkt_key.msk_top_dei_##key_idx);\
	printk(KERN_INFO "| top_dei = 0x%x\n", o.cl_spcl_pkt_key.top_dei_##key_idx);\
	printk(KERN_INFO "| msk_top_vl_802_1p = 0x%x\n", o.cl_spcl_pkt_key.msk_top_vl_802_1p_##key_idx);\
	printk(KERN_INFO "| top_802_1p = 0x%x\n", o.cl_spcl_pkt_key.top_802_1p_##key_idx);\
	printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", o.cl_spcl_pkt_key.msk_top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_spcl_pkt_key.top_vid_ptag_##key_idx);\
	printk(KERN_INFO "| msk_top_vid = 0x%x\n", o.cl_spcl_pkt_key.msk_top_vid_##key_idx);\
	printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_spcl_pkt_key.top_vid_##key_idx);\
	printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", o.cl_spcl_pkt_key.msk_top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_spcl_pkt_key.top_tpid_enc_##key_idx);\
	printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", o.cl_spcl_pkt_key.msk_vlan_cnt_##key_idx);\
	printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_spcl_pkt_key.vlan_cnt_##key_idx);\
	printk(KERN_INFO "| msk_ethertype_enc = 0x%x\n", o.cl_spcl_pkt_key.msk_ethertype_enc_##key_idx);\
	printk(KERN_INFO "| ethertype_enc = 0x%x\n", o.cl_spcl_pkt_key.ethertype_enc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_rsvd = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_rsvd_##key_idx);\
	printk(KERN_INFO "| mac_da_rsvd = 0x%x\n", o.cl_spcl_pkt_key.mac_da_rsvd_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_ip_mc = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_ip_mc_##key_idx);\
	printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_ip_mc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_mc = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_mc_##key_idx);\
	printk(KERN_INFO "| mac_da_mc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_mc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_bc = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_bc_##key_idx);\
	printk(KERN_INFO "| mac_da_bc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_bc_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_an_hit = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_an_hit_##key_idx);\
	printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", o.cl_spcl_pkt_key.mac_da_an_hit_##key_idx);\
	printk(KERN_INFO "| msk_mac_da_an_sel = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_an_sel_##key_idx);\
	printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", o.cl_spcl_pkt_key.mac_da_an_sel_##key_idx);\
	printk(KERN_INFO "| msk_mac_da = 0x%x\n", o.cl_spcl_pkt_key.msk_mac_da_##key_idx);\
	printk(KERN_INFO "| mac_da = %02x:%02x:%02x:%02x:%02x:%02x\n", o.cl_spcl_pkt_key.mac_da5_##key_idx,\
					o.cl_spcl_pkt_key.mac_da4_##key_idx, o.cl_spcl_pkt_key.mac_da3_##key_idx,\
					o.cl_spcl_pkt_key.mac_da2_##key_idx, o.cl_spcl_pkt_key.mac_da1_##key_idx,\
					o.cl_spcl_pkt_key.mac_da0_##key_idx);\
	printk(KERN_INFO "| msk_o_lspid = 0x%x\n", o.cl_spcl_pkt_key.msk_o_lspid_##key_idx);\
	printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_spcl_pkt_key.o_lspid_##key_idx);\
	printk(KERN_INFO "| msk_lspid = 0x%x\n", o.cl_spcl_pkt_key.msk_lspid_##key_idx);\
	printk(KERN_INFO "| lspid = 0x%x\n", o.cl_spcl_pkt_key.lspid_##key_idx);\
} while(0)

static void __l3_cls_key_tbl_entry_dump(aal_table_t *table, unsigned int start_idx, int count)
{
	l3_cls_key_t entry;
	ca_int32_t end_idx, i, j, status, key_vld, key_idx, sz;
	unsigned int refcnt, *ptr=NULL;
	l3_cls_key_type_t key_type;
	l3_cls_key_t *p_key = NULL;
	ca_uint32_t fib_tbl_idx;
	char *key_str[] = {
		"IF_ID_KEY",
		"IPV4_TUNNEL_ID_KEY",
		"IPV4_SHORT_KEY",
		"IPV6_SHORT_KEY",
		"SPCL_PKT_KEY",
		"MCST_KEY",
		"FULL_KEY"
	} ;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry))
		end_idx = table->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== L3 Classifier Key Table ===========================================|\n");

	sz = sizeof(l3_cls_key_t);

	for (i = start_idx; i < end_idx; i++) {
		memset(&entry, 0, sz);
		/* read key entry */
		if (ca_aal_table_sw_cache) {
			status = aal_entry_get_by_idx(table->table_id, i, entry.cls_key_info.key_tbl_data);
		} else {
			status = hw_entry_get(table, i, entry.cls_key_info.key_tbl_data);
		}
		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR)) {
			printk(KERN_INFO "| index %04d | NOT USED\n", i);
			printk(KERN_INFO "|------------------------------------------------------------------|\n");
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			key_vld = entry.cls_key_info.key_tbl_data[0].cls_key_valid;
			key_type = entry.cls_key_info.key_tbl_data[0].cls_key_type;
			printk(KERN_INFO "| index %04d | ref count %d | valid %x | %s | rslt type %d | prio %d\n",
				i, refcnt, key_vld, key_str[key_type], entry.cls_key_info.key_tbl_data[0].cls_rslt_type,
				entry.cls_key_info.key_tbl_data[0].cls_pri);

			/* print row data */
			ptr = (unsigned int *)&entry.cls_key_info.key_tbl_data[0];
			printk("| MSB  ");
			for (j = 10; j >= 0; j--) {
				printk("%08x ", ptr[j]);
			}
			printk("\n");

			/* read the next entry for full key */
			if (key_type == CL_FULL_KEY) {
				i++;
				if (ca_aal_table_sw_cache) {
					status = aal_entry_get_by_idx(table->table_id, i, &entry.cls_key_info.key_tbl_data[1]);
				} else {
					status = hw_entry_get(table, i, &entry.cls_key_info.key_tbl_data[1]);
				}
				/* print row data */
				ptr = (unsigned int *)&entry.cls_key_info.key_tbl_data[1];
				printk("| MSB  ");
				for (j = 10; j >= 0; j--) {
					printk("%08x ", ptr[j]);
				}
				printk("\n");
			}

			/* print detail fields */
			p_key = (l3_cls_key_t *)&entry;
			switch(key_type) {
					case CL_IF_ID_KEY:
						for (key_idx = 0; key_idx < CL_4_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								printk(KERN_INFO "|------------------------------------------------------------------|\n");
								printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_IF_ID_KEY_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_IF_ID_KEY_PRINT(p_key->cls_key_info, 1);
							} else if (key_idx == 2) {
								CL_IF_ID_KEY_PRINT(p_key->cls_key_info, 2);
							} else if (key_idx == 3) {
								CL_IF_ID_KEY_PRINT(p_key->cls_key_info, 3);
							}
						}
						break;
					case CL_IPV4_TUNNEL_ID_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						printk(KERN_INFO "| msk_l4_port = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_l4_port);
						printk(KERN_INFO "| l4_dp = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.l4_dp);
						printk(KERN_INFO "| l4_sp = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.l4_sp);
						printk(KERN_INFO "| msk_l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_l4_vld);
						printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.l4_vld);

						printk(KERN_INFO "| msk_spi = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_spi);

						printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_fragment);
						printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_fragment);

						printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_sa_equal);
						printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_sa_equal);
						printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_mc);
						printk(KERN_INFO "| ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_mc);
						printk(KERN_INFO "| msk_ip_da_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_enc);
						printk(KERN_INFO "| ip_da_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_enc);
						printk(KERN_INFO "| msk_ip_da_l = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_l);
						printk(KERN_INFO "| ip_da = 0x%08x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da);

						printk(KERN_INFO "| msk_ip_sa_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_enc);
						printk(KERN_INFO "| ip_sa_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_sa_enc);
						printk(KERN_INFO "| msk_ip_sa_l = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_l);
						printk(KERN_INFO "| ip_sa = 0x%08x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_sa);
						printk(KERN_INFO "| msk_ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_protocol);
						printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_protocol);

						printk(KERN_INFO "| msk_ip_ver = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_ver);
						printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_ver);
						printk(KERN_INFO "| msk_ip_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_vld);
						printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_vld);

						printk(KERN_INFO "| msk_pppoe_session = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_pppoe_session);
						printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.pppoe_session_id);

						printk(KERN_INFO "| msk_inner_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_inner_vid);
						printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.inner_vid);
						printk(KERN_INFO "| msk_inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_inner_tpid_enc);
						printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.inner_tpid_enc);

						printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid_ptag);
						printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_vid_ptag);

						printk(KERN_INFO "| msk_top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid);
						printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_vid);
						printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_tpid_enc);
						printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_tpid_enc);
						printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_vlan_cnt);
						printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.vlan_cnt);

						printk(KERN_INFO "| msk_ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ethertype_enc);
						printk(KERN_INFO "| ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ethertype_enc);

						printk(KERN_INFO "| msk_mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_ip_mc);
						printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_ip_mc);
						printk(KERN_INFO "| msk_mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_mc);
						printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_mc);
						printk(KERN_INFO "| msk_mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_bc);
						printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_bc);
						printk(KERN_INFO "| msk_mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_hit);
						printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_hit);
						printk(KERN_INFO "| msk_mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_sel);
						printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_sel);

						printk(KERN_INFO "| msk_o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_o_lspid);
						printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.o_lspid);
						printk(KERN_INFO "| msk_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.msk_lspid);
						printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.lspid);
						break;
					case CL_IPV4_SHORT_KEY:
						for (key_idx = 0; key_idx < CL_2_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								printk(KERN_INFO "|------------------------------------------------------------------|\n");
								printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_IPV4_SHORT_KEY_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_IPV4_SHORT_KEY_PRINT(p_key->cls_key_info, 1);
							}
						}
						break;
					case CL_IPV6_SHORT_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						printk(KERN_INFO "| msk_l4_port = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_l4_port);
						printk(KERN_INFO "| l4_dp = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.l4_dp);
						printk(KERN_INFO "| l4_sp = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.l4_sp);
						printk(KERN_INFO "| msk_l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_l4_vld);
						printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.l4_vld);

						printk(KERN_INFO "| msk_spi = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_spi);

						printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_fragment);
						printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_fragment);

						printk(KERN_INFO "| msk_ipv6_flow_lbl = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ipv6_flow_lbl);
						printk(KERN_INFO "| ipv6_flow_lbl = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ipv6_flow_lbl);
						printk(KERN_INFO "| ip_addr_type = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_addr_type);
						printk(KERN_INFO "| msk_ip_addr_l = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_l);
						printk(KERN_INFO "| msk_ip_addr_h = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_h);
						printk(KERN_INFO "| ip_addr = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_addr_3,
										p_key->cls_key_info.cl_ipv6_short_key.ip_addr_2,
										p_key->cls_key_info.cl_ipv6_short_key.ip_addr_1,
										p_key->cls_key_info.cl_ipv6_short_key.ip_addr_0);

						printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_da_sa_equal);
						printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_da_sa_equal);
						printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_da_mc);
						printk(KERN_INFO "| ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_da_mc);

						printk(KERN_INFO "| msk_ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_protocol);
						printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_protocol);
						printk(KERN_INFO "| msk_ip_ecn = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_ip_ecn);
						printk(KERN_INFO "| ip_ecn = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_ecn );
						printk(KERN_INFO "| msk_dscp = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_dscp);
						printk(KERN_INFO "| ip_dscp = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_dscp);

						printk(KERN_INFO "| msk_top_dei = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_top_dei);
						printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_dei);
						printk(KERN_INFO "| msk_top_vl_802_1p = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_top_vl_802_1p);
						printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_802_1p);
						printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_top_vid_ptag);
						printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_vid_ptag);

						printk(KERN_INFO "| msk_top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_top_vid);
						printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_vid);
						printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_top_tpid_enc);
						printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_tpid_enc);
						printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_vlan_cnt);
						printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.vlan_cnt);

						printk(KERN_INFO "| msk_o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_o_lspid);
						printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.o_lspid);
						printk(KERN_INFO "| msk_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_lspid);
						printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.lspid);
						break;
					case CL_SPCL_PKT_KEY:
						for (key_idx = 0; key_idx < CL_2_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								printk(KERN_INFO "|------------------------------------------------------------------|\n");
								printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_SPCL_PKT_KEY_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_SPCL_PKT_KEY_PRINT(p_key->cls_key_info, 1);
							}
						}
						break;
					case CL_MCST_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_ip_da_sa_equal);
						printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_da_sa_equal);
						printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_ip_da_mc);
						printk(KERN_INFO "| ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_da_mc);

						printk(KERN_INFO "| msk_ip_ver = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_ip_ver);
						printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_ver);
						printk(KERN_INFO "| msk_ip_vld = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_ip_vld);
						printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_vld);

						printk(KERN_INFO "| msk_pppoe_session = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_pppoe_session);
						printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_mcst_key.pppoe_session_id);

						printk(KERN_INFO "| msk_inner_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_inner_vid);
						printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.inner_vid);
						printk(KERN_INFO "| msk_inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_inner_tpid_enc);
						printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.inner_tpid_enc);
						printk(KERN_INFO "| msk_top_dei = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_top_dei);
						printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_dei);
						printk(KERN_INFO "| msk_top_vl_802_1p = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_top_vl_802_1p);
						printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_802_1p);
						printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_top_vid_ptag);
						printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_vid_ptag);

						printk(KERN_INFO "| msk_top_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_top_vid);
						printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_vid);
						printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_top_tpid_enc);
						printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_tpid_enc);
						printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_vlan_cnt);
						printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_mcst_key.vlan_cnt);

						printk(KERN_INFO "| msk_mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_mac_da_ip_mc);
						printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_ip_mc);
						printk(KERN_INFO "| msk_mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_mac_da_mc);
						printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_mc);
						printk(KERN_INFO "| msk_mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_mac_da_bc);
						printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_bc);

						printk(KERN_INFO "| msk_dest_addr = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_dest_addr);
						printk(KERN_INFO "| dest_addr_type = 0x%x\n", p_key->cls_key_info.cl_mcst_key.dest_addr_type);
						if (p_key->cls_key_info.cl_mcst_key.dest_addr_type) {
							printk(KERN_INFO "| dest_addr.ip_addr = %08x %08x %08x %08x\n",
								p_key->cls_key_info.cl_mcst_key.dest_addr_3,
								p_key->cls_key_info.cl_mcst_key.dest_addr_2,
								p_key->cls_key_info.cl_mcst_key.dest_addr_1,
								p_key->cls_key_info.cl_mcst_key.dest_addr_0);
						} else {
								ca_uint8_t mac[6];
								mac[3] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 24) & 0xff;
								mac[2] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 16) & 0xff;
								mac[1] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 8) & 0xff;
								mac[0] = p_key->cls_key_info.cl_mcst_key.dest_addr_0 & 0xff;
								mac[4] = p_key->cls_key_info.cl_mcst_key.dest_addr_1 & 0xff;
								mac[5] = (p_key->cls_key_info.cl_mcst_key.dest_addr_1 >> 8) & 0xff;
							printk(KERN_INFO "| dest_addr.mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
								mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
						}
						printk(KERN_INFO "| msk_mcgid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_mcgid);
						printk(KERN_INFO "| mcgid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mcgid);
						printk(KERN_INFO "| msk_o_lspid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_o_lspid);
						printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.o_lspid);
						printk(KERN_INFO "| msk_lspid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.msk_lspid);
						break;
					case CL_FULL_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						printk(KERN_INFO "| msk_tcp_rdp_ctrl = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_tcp_rdp_ctrl);
						printk(KERN_INFO "| tcp_rdp_ctrl = 0x%x\n", p_key->cls_key_info.cl_full_key.tcp_rdp_ctrl);
						printk(KERN_INFO "| msk_l4_port = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_l4_port);
						printk(KERN_INFO "| l4_dp = 0x%x\n", p_key->cls_key_info.cl_full_key.l4_dp);
						printk(KERN_INFO "| l4_sp = 0x%x\n", p_key->cls_key_info.cl_full_key.l4_sp);
						printk(KERN_INFO "| msk_l4_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_l4_vld);
						printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.l4_vld);
						printk(KERN_INFO "| msk_l3_chksum_err = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_l3_chksum_err);
						printk(KERN_INFO "| l3_chksum_err = 0x%x\n", p_key->cls_key_info.cl_full_key.l3_chksum_err);
						printk(KERN_INFO "| msk_spi = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_spi);

						printk(KERN_INFO "| msk_ip_fragment = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_fragment);
						printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_fragment);
						printk(KERN_INFO "| msk_ip_options = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_options);
						printk(KERN_INFO "| ip_options = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_options);

						printk(KERN_INFO "| msk_ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_da_sa_equal);
						printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_da_sa_equal);
						printk(KERN_INFO "| msk_ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_da_mc);
						printk(KERN_INFO "| ip_da_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_da_mc);

						printk(KERN_INFO "| msk_ip_da_l = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_da_l);
						printk(KERN_INFO "| msk_ip_da_h = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_da_h);
						printk(KERN_INFO "| ip_da = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_full_key.ip_da_3,
											p_key->cls_key_info.cl_full_key.ip_da_2,
											p_key->cls_key_info.cl_full_key.ip_da_1,
											p_key->cls_key_info.cl_full_key.ip_da_0);

						printk(KERN_INFO "| msk_ip_sa_l = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_sa_l);
						printk(KERN_INFO "| msk_ip_sa_h = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_sa_h);
						printk(KERN_INFO "| ip_sa = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_full_key.ip_sa_3,
											p_key->cls_key_info.cl_full_key.ip_sa_2,
											p_key->cls_key_info.cl_full_key.ip_sa_1,
											p_key->cls_key_info.cl_full_key.ip_sa_0);

						printk(KERN_INFO "| msk_ip_protocol = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_protocol);
						printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_protocol);
						printk(KERN_INFO "| msk_ip_ecn = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_ecn);
						printk(KERN_INFO "| ip_ecn = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_ecn);
						printk(KERN_INFO "| msk_dscp = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_dscp);
						printk(KERN_INFO "| ip_dscp = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_dscp);
						printk(KERN_INFO "| msk_ip_ver = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_ver);
						printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_ver);
						printk(KERN_INFO "| msk_ip_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_vld);
						printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_vld);
						printk(KERN_INFO "| msk_ppp_protocol = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ppp_protocol);
						printk(KERN_INFO "| ppp_protocol_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.ppp_protocol_enc);
						printk(KERN_INFO "| msk_pppoe_session = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_pppoe_session);
						printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_full_key.pppoe_session_id);
						printk(KERN_INFO "| msk_inner_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_inner_dei);
						printk(KERN_INFO "| inner_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_dei);
						printk(KERN_INFO "| msk_inner_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_inner_802_1p);
						printk(KERN_INFO "| inner_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_802_1p);
						printk(KERN_INFO "| msk_inner_vid = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_inner_vid);
						printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_vid);
						printk(KERN_INFO "| msk_inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_inner_tpid_enc);
						printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_tpid_enc);
						printk(KERN_INFO "| msk_top_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_top_dei);
						printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.top_dei);
						printk(KERN_INFO "| msk_top_vl_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_top_vl_802_1p);
						printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.top_802_1p);
						printk(KERN_INFO "| msk_top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_top_vid_ptag);
						printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_full_key.top_vid_ptag);
						printk(KERN_INFO "| vid_ctl = 0x%x\n", p_key->cls_key_info.cl_full_key.vid_ctl);

						printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_full_key.top_vid);
						printk(KERN_INFO "| msk_top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_top_tpid_enc);
						printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.top_tpid_enc);
						printk(KERN_INFO "| msk_vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_vlan_cnt);
						printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_full_key.vlan_cnt);
						printk(KERN_INFO "| llc_type_enc_mask = 0x%x\n", p_key->cls_key_info.cl_full_key.llc_type_enc_mask);
						printk(KERN_INFO "| llc_type_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.llc_type_enc);
						printk(KERN_INFO "| pktlen_rng_match_mask = 0x%x\n", p_key->cls_key_info.cl_full_key.pktlen_rng_match_mask);
						printk(KERN_INFO "| pktlen_rng_match_vec = 0x%x\n", p_key->cls_key_info.cl_full_key.pktlen_rng_match_vec);
						printk(KERN_INFO "| msk_len_encoded = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_len_encoded);
						printk(KERN_INFO "| len_encoded = 0x%x\n", p_key->cls_key_info.cl_full_key.len_encoded);
						printk(KERN_INFO "| msk_ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ethertype_enc);
						printk(KERN_INFO "| ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.ethertype_enc);
						printk(KERN_INFO "| msk_mac_sa = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_sa);
						printk(KERN_INFO "| mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\n",
							p_key->cls_key_info.cl_full_key.mac_sa5,
							p_key->cls_key_info.cl_full_key.mac_sa4,
							p_key->cls_key_info.cl_full_key.mac_sa3,
							p_key->cls_key_info.cl_full_key.mac_sa2,
							p_key->cls_key_info.cl_full_key.mac_sa1,
							p_key->cls_key_info.cl_full_key.mac_sa0);

						printk(KERN_INFO "| msk_mac_da_rsvd = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_rsvd);
						printk(KERN_INFO "| mac_da_rsvd = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_rsvd);
						printk(KERN_INFO "| msk_mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_ip_mc);
						printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_ip_mc);
						printk(KERN_INFO "| msk_mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_mc);
						printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_mc);
						printk(KERN_INFO "| msk_mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_bc);
						printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_bc);
						printk(KERN_INFO "| msk_mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_an_hit);
						printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_an_hit);
						printk(KERN_INFO "| msk_mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mac_da_an_sel);
						printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_an_sel);

						printk(KERN_INFO "| msk_mdata = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mdata);
						printk(KERN_INFO "| mdata = %08x %08x\n", p_key->cls_key_info.cl_full_key.mdata_1,
															p_key->cls_key_info.cl_full_key.mdata_0);
						printk(KERN_INFO "| msk_mcgid = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_mcgid);
						printk(KERN_INFO "| mcgid = 0x%x\n", p_key->cls_key_info.cl_full_key.mcgid);
						printk(KERN_INFO "| msk_o_lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_o_lspid);
						printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.o_lspid);
						printk(KERN_INFO "| msk_lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_lspid);
						printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.lspid);
						break;
					default:
						;
				}

			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			//cond_resched();
		}
	}
}

static void __l3_cls_key_tbl_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_key_tbl_entry_dump(&l3_cls_key_table, start_idx, count);
}

#if defined(CONFIG_LUNA_G3_SERIES)
#define CL_IF_ID_KEY_USED_PRINT(o, key_idx) {\
	if(o.cl_if_id_key.msk_ip_fragment_##key_idx == 0) printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_if_id_key.ip_fragment_##key_idx);\
	if(o.cl_if_id_key.msk_ip_da_sa_equal_##key_idx == 0) printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", o.cl_if_id_key.ip_da_sa_equal_##key_idx);\
	if(o.cl_if_id_key.msk_ip_da_mc_##key_idx == 0) printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", o.cl_if_id_key.ip_da_mc_##key_idx);\
	if(o.cl_if_id_key.msk_ip_ver_##key_idx == 0) printk(KERN_INFO "| ip_ver = 0x%x\n", o.cl_if_id_key.ip_ver_##key_idx);\
	if(o.cl_if_id_key.msk_ip_vld_##key_idx == 0) printk(KERN_INFO "| ip_vld = 0x%x\n", o.cl_if_id_key.ip_vld_##key_idx);\
	if(o.cl_if_id_key.msk_pppoe_session_##key_idx == 0) printk(KERN_INFO "| pppoe_session_id = 0x%x\n", o.cl_if_id_key.pppoe_session_id_##key_idx);\
	if(o.cl_if_id_key.msk_top_vid_ptag_##key_idx == 0) printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_if_id_key.top_vid_ptag_##key_idx);\
	if(o.cl_if_id_key.msk_top_vid_##key_idx == 0) printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_if_id_key.top_vid_##key_idx);\
	if(o.cl_if_id_key.msk_top_tpid_enc_##key_idx == 0) printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_if_id_key.top_tpid_enc_##key_idx);\
	if(o.cl_if_id_key.msk_vlan_cnt_##key_idx == 0) printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_if_id_key.vlan_cnt_##key_idx);\
	if(o.cl_if_id_key.msk_ethertype_enc_##key_idx == 0) printk(KERN_INFO "| ethertype_enc = 0x%x\n", o.cl_if_id_key.ethertype_enc_##key_idx);\
	if(o.cl_if_id_key.msk_mac_da_ip_mc_##key_idx == 0) printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", o.cl_if_id_key.mac_da_ip_mc_##key_idx);\
	if(o.cl_if_id_key.msk_mac_da_mc_##key_idx == 0) printk(KERN_INFO "| mac_da_mc = 0x%x\n", o.cl_if_id_key.mac_da_mc_##key_idx);\
	if(o.cl_if_id_key.msk_mac_da_bc_##key_idx == 0) printk(KERN_INFO "| mac_da_bc = 0x%x\n", o.cl_if_id_key.mac_da_bc_##key_idx);\
	if(o.cl_if_id_key.msk_mac_da_an_hit_##key_idx == 0) printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", o.cl_if_id_key.mac_da_an_hit_##key_idx);\
	if(o.cl_if_id_key.msk_mac_da_an_sel_##key_idx == 0) printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", o.cl_if_id_key.mac_da_an_sel_##key_idx);\
	if(o.cl_if_id_key.msk_o_lspid_##key_idx == 0) printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_if_id_key.o_lspid_##key_idx);\
	if(o.cl_if_id_key.msk_lspid_##key_idx == 0) printk(KERN_INFO "| lspid = 0x%x\n", o.cl_if_id_key.lspid_##key_idx);\
} while(0)

#define CL_IPV4_SHORT_KEY_USED_PRINT(o, key_idx) {\
	if(o.cl_ipv4_short_key.msk_l4_port_##key_idx != 7) printk(KERN_INFO "| msk_l4_port(0:sp,dp;1:sp range;2:dp range;3:rng_match) = 0x%x, l4_dp = 0x%x, l4_sp = 0x%x\n", o.cl_ipv4_short_key.msk_l4_port_##key_idx, o.cl_ipv4_short_key.l4_dp_##key_idx, o.cl_ipv4_short_key.l4_sp_##key_idx);\
	if(o.cl_ipv4_short_key.msk_l4_vld_##key_idx == 0) printk(KERN_INFO "| l4_vld = 0x%x\n", o.cl_ipv4_short_key.l4_vld_##key_idx);\
	if(o.cl_ipv4_short_key.msk_l3_chksum_err_##key_idx == 0) printk(KERN_INFO "| l3_chksum_err = 0x%x\n", o.cl_ipv4_short_key.l3_chksum_err_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_fragment_##key_idx == 0) printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_ipv4_short_key.ip_fragment_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_addr_l_##key_idx != 0) printk(KERN_INFO "| ip_addr_type(0:da;1:sa) = 0x%x, msk_ip_addr_l = 0x%x, ip_addr = 0x%08x\n", o.cl_ipv4_short_key.ip_addr_type_##key_idx, o.cl_ipv4_short_key.msk_ip_addr_l_##key_idx, o.cl_ipv4_short_key.ip_addr_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_da_sa_equal_##key_idx == 0) printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", o.cl_ipv4_short_key.ip_da_sa_equal_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_da_mc_##key_idx == 0) printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", o.cl_ipv4_short_key.ip_da_mc_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_protocol_##key_idx == 0) printk(KERN_INFO "| ip_protocol = 0x%x\n", o.cl_ipv4_short_key.ip_protocol_##key_idx);\
	if(o.cl_ipv4_short_key.msk_ip_ecn_##key_idx == 0) printk(KERN_INFO "| ip_ecn = 0x%x\n", o.cl_ipv4_short_key.ip_ecn_##key_idx);\
	if(o.cl_ipv4_short_key.msk_dscp_##key_idx == 0) printk(KERN_INFO "| ip_dscp = 0x%x\n", o.cl_ipv4_short_key.ip_dscp_##key_idx);\
	if(o.cl_ipv4_short_key.msk_top_dei_##key_idx == 0) printk(KERN_INFO "| top_dei = 0x%x\n", o.cl_ipv4_short_key.top_dei_##key_idx);\
	if(o.cl_ipv4_short_key.msk_top_vl_802_1p_##key_idx == 0) printk(KERN_INFO "| top_802_1p = 0x%x\n", o.cl_ipv4_short_key.top_802_1p_##key_idx);\
	if(o.cl_ipv4_short_key.msk_top_vid_ptag_##key_idx == 0) printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_ipv4_short_key.top_vid_ptag_##key_idx);\
	if(o.cl_ipv4_short_key.msk_top_vid_##key_idx == 0) printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_ipv4_short_key.top_vid_##key_idx);\
	if(o.cl_ipv4_short_key.msk_top_tpid_enc_##key_idx == 0) printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_ipv4_short_key.top_tpid_enc_##key_idx);\
	if(o.cl_ipv4_short_key.msk_vlan_cnt_##key_idx == 0) printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_ipv4_short_key.vlan_cnt_##key_idx);\
	if(o.cl_ipv4_short_key.msk_o_lspid_##key_idx == 0) printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_ipv4_short_key.o_lspid_##key_idx);\
	if(o.cl_ipv4_short_key.msk_lspid_##key_idx == 0) printk(KERN_INFO "| lspid = 0x%x\n", o.cl_ipv4_short_key.lspid_##key_idx);\
} while(0)

#define CL_SPCL_PKT_KEY_USED_PRINT(o, key_idx) {\
	if(o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_msk_##key_idx == 0) printk(KERN_INFO "| spcl_pkt_hdr_mtch = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_##key_idx);\
	if(o.cl_spcl_pkt_key.spcl_pkt_enc_msk_##key_idx == 0) printk(KERN_INFO "| spcl_pkt_enc = 0x%x\n", o.cl_spcl_pkt_key.spcl_pkt_enc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_icmp_type_##key_idx == 0) printk(KERN_INFO "| icmp_type = 0x%x\n", o.cl_spcl_pkt_key.icmp_type_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_icmp_vld_##key_idx == 0) printk(KERN_INFO "| icmp_vld = 0x%x\n", o.cl_spcl_pkt_key.icmp_vld_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ipv6_doh_##key_idx == 0) printk(KERN_INFO "| ipv6_doh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_doh_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ipv6_rh_##key_idx == 0) printk(KERN_INFO "| ipv6_rh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_rh_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ipv6_hbh_##key_idx == 0) printk(KERN_INFO "| ipv6_hbh = 0x%x\n", o.cl_spcl_pkt_key.ipv6_hbh_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ipv6_ndp_##key_idx == 0) printk(KERN_INFO "| ipv6_ndp = 0x%x\n", o.cl_spcl_pkt_key.ipv6_ndp_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ip_fragment_##key_idx == 0) printk(KERN_INFO "| ip_fragment = 0x%x\n", o.cl_spcl_pkt_key.ip_fragment_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ip_options_##key_idx == 0) printk(KERN_INFO "| ip_options = 0x%x\n", o.cl_spcl_pkt_key.ip_options_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ip_protocol_##key_idx == 0) printk(KERN_INFO "| ip_protocol = 0x%x\n", o.cl_spcl_pkt_key.ip_protocol_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_top_dei_##key_idx == 0) printk(KERN_INFO "| top_dei = 0x%x\n", o.cl_spcl_pkt_key.top_dei_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_top_vl_802_1p_##key_idx == 0) printk(KERN_INFO "| top_802_1p = 0x%x\n", o.cl_spcl_pkt_key.top_802_1p_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_top_vid_ptag_##key_idx == 0) printk(KERN_INFO "| top_vid_ptag = 0x%x\n", o.cl_spcl_pkt_key.top_vid_ptag_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_top_vid_##key_idx == 0) printk(KERN_INFO "| top_vid = 0x%x\n", o.cl_spcl_pkt_key.top_vid_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_top_tpid_enc_##key_idx == 0) printk(KERN_INFO "| top_tpid_enc = 0x%x\n", o.cl_spcl_pkt_key.top_tpid_enc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_vlan_cnt_##key_idx == 0) printk(KERN_INFO "| vlan_cnt = 0x%x\n", o.cl_spcl_pkt_key.vlan_cnt_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_ethertype_enc_##key_idx == 0) printk(KERN_INFO "| ethertype_enc = 0x%x\n", o.cl_spcl_pkt_key.ethertype_enc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_rsvd_##key_idx == 0) printk(KERN_INFO "| mac_da_rsvd = 0x%x\n", o.cl_spcl_pkt_key.mac_da_rsvd_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_ip_mc_##key_idx == 0) printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_ip_mc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_mc_##key_idx == 0) printk(KERN_INFO "| mac_da_mc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_mc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_bc_##key_idx == 0) printk(KERN_INFO "| mac_da_bc = 0x%x\n", o.cl_spcl_pkt_key.mac_da_bc_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_an_hit_##key_idx == 0) printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", o.cl_spcl_pkt_key.mac_da_an_hit_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_an_sel_##key_idx == 0) printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", o.cl_spcl_pkt_key.mac_da_an_sel_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_mac_da_##key_idx == 0) printk(KERN_INFO "| mac_da = %02x:%02x:%02x:%02x:%02x:%02x\n", o.cl_spcl_pkt_key.mac_da5_##key_idx,\
					o.cl_spcl_pkt_key.mac_da4_##key_idx, o.cl_spcl_pkt_key.mac_da3_##key_idx,\
					o.cl_spcl_pkt_key.mac_da2_##key_idx, o.cl_spcl_pkt_key.mac_da1_##key_idx,\
					o.cl_spcl_pkt_key.mac_da0_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_o_lspid_##key_idx == 0) printk(KERN_INFO "| o_lspid = 0x%x\n", o.cl_spcl_pkt_key.o_lspid_##key_idx);\
	if(o.cl_spcl_pkt_key.msk_lspid_##key_idx == 0) printk(KERN_INFO "| lspid = 0x%x\n", o.cl_spcl_pkt_key.lspid_##key_idx);\
} while(0)

static void __l3_cls_key_tbl_used_entry_dump(aal_table_t *table, unsigned int start_idx, int count)
{
	l3_cls_key_t entry;
	ca_int32_t end_idx, i, j, status, key_vld, key_idx, sz;
	unsigned int refcnt, *ptr=NULL;
	l3_cls_key_type_t key_type;
	l3_cls_key_t *p_key = NULL;
	ca_uint32_t fib_tbl_idx;
	char *key_str[] = {
		"IF_ID_KEY",
		"IPV4_TUNNEL_ID_KEY",
		"IPV4_SHORT_KEY",
		"IPV6_SHORT_KEY",
		"SPCL_PKT_KEY",
		"MCST_KEY",
		"FULL_KEY"
	} ;

	if (start_idx >= table->max_entry) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((count < 0) || (start_idx + count > table->max_entry))
		end_idx = table->max_entry;
	else
		end_idx = start_idx + count;

	printk(KERN_INFO "|===== L3 Classifier Table ===========================================|\n");

	sz = sizeof(l3_cls_key_t);

	for (i = start_idx; i < end_idx; i++) {
		memset(&entry, 0, sz);
		/* read key entry */
		if (ca_aal_table_sw_cache) {
			status = aal_entry_get_by_idx(table->table_id, i, entry.cls_key_info.key_tbl_data);
		} else {
			status = hw_entry_get(table, i, entry.cls_key_info.key_tbl_data);
		}
		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR)) {
			//printk(KERN_INFO "| index %04d | NOT USED\n", i);
			//printk(KERN_INFO "|------------------------------------------------------------------|\n");
			continue;
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);
			key_vld = entry.cls_key_info.key_tbl_data[0].cls_key_valid;
			key_type = entry.cls_key_info.key_tbl_data[0].cls_key_type;
			if(key_vld == 0)
				continue;
			printk(KERN_INFO "| index %04d | ref count %d | valid %x | %s | rslt type %d | prio %d\n",
				i, refcnt, key_vld, key_str[key_type], entry.cls_key_info.key_tbl_data[0].cls_rslt_type,
				entry.cls_key_info.key_tbl_data[0].cls_pri);

			/* print row data */
#if 0
			ptr = (unsigned int *)&entry.cls_key_info.key_tbl_data[0];
			printk("| MSB  ");
			for (j = 10; j >= 0; j--) {
				printk("%08x ", ptr[j]);
			}
			printk("\n");
#endif

			/* read the next entry for full key */
			if (key_type == CL_FULL_KEY) {
				i++;
				if (ca_aal_table_sw_cache) {
					status = aal_entry_get_by_idx(table->table_id, i, &entry.cls_key_info.key_tbl_data[1]);
				} else {
					status = hw_entry_get(table, i, &entry.cls_key_info.key_tbl_data[1]);
				}
				/* print row data */
#if 0
				ptr = (unsigned int *)&entry.cls_key_info.key_tbl_data[1];
				printk("| MSB  ");
				for (j = 10; j >= 0; j--) {
					printk("%08x ", ptr[j]);
				}
				printk("\n");
#endif
			}

			/* print detail fields */
			p_key = (l3_cls_key_t *)&entry;
			switch(key_type) {
					case CL_IF_ID_KEY:
						for (key_idx = 0; key_idx < CL_4_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								//printk(KERN_INFO "|------------------------------------------------------------------|\n");
								//printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							//printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d --------------------------------------------|\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_IF_ID_KEY_USED_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_IF_ID_KEY_USED_PRINT(p_key->cls_key_info, 1);
							} else if (key_idx == 2) {
								CL_IF_ID_KEY_USED_PRINT(p_key->cls_key_info, 2);
							} else if (key_idx == 3) {
								CL_IF_ID_KEY_USED_PRINT(p_key->cls_key_info, 3);
							}

							__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						}
						break;
					case CL_IPV4_TUNNEL_ID_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						//printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_l4_port != 7) {
							printk(KERN_INFO "| msk_l4_port(0:sp,dp;1:sp range;2:dp range;3:rng_match) = 0x%x, l4_dp = 0x%x, l4_sp = 0x%x\n", 
								p_key->cls_key_info.cl_ipv4_tunnel_key.msk_l4_port, p_key->cls_key_info.cl_ipv4_tunnel_key.l4_dp, p_key->cls_key_info.cl_ipv4_tunnel_key.l4_sp);
						}
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_l4_vld == 0)
							printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.l4_vld);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_fragment == 0)
							printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_fragment);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_sa_equal == 0)
							printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_sa_equal);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_mc == 0)
							printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_mc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_enc == 0)
							printk(KERN_INFO "| ip_da_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da_enc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_l != 0)
							printk(KERN_INFO "| ip_da = 0x%08x, ip_da_l = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_da, p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_l);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_enc == 0)
							printk(KERN_INFO "| ip_sa_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_sa_enc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_l != 0)
							printk(KERN_INFO "| ip_sa = 0x%08x, ip_sa_l = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_sa, p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_l);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_protocol == 0)
							printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_protocol);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_ver == 0)
							printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_ver);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ip_vld == 0)
							printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ip_vld);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_pppoe_session == 0)
							printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.pppoe_session_id);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_inner_vid == 0)
							printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.inner_vid);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_inner_tpid_enc == 0)
							printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.inner_tpid_enc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid_ptag == 0)
							printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_vid_ptag);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid == 0)
							printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_vid);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_top_tpid_enc == 0)
							printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.top_tpid_enc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_vlan_cnt == 0)
							printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.vlan_cnt);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_ethertype_enc == 0)
							printk(KERN_INFO "| ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.ethertype_enc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_ip_mc == 0)
							printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_ip_mc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_mc == 0)
							printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_mc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_bc == 0)
							printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_bc);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_hit == 0)
							printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_hit);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_sel == 0)
							printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_sel);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_o_lspid == 0)
							printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.o_lspid);
						if(p_key->cls_key_info.cl_ipv4_tunnel_key.msk_lspid == 0)
							printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_ipv4_tunnel_key.lspid);

						__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						break;
					case CL_IPV4_SHORT_KEY:
						for (key_idx = 0; key_idx < CL_2_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								//printk(KERN_INFO "|------------------------------------------------------------------|\n");
								//printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							//printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d --------------------------------------------|\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_IPV4_SHORT_KEY_USED_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_IPV4_SHORT_KEY_USED_PRINT(p_key->cls_key_info, 1);
							}

							__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						}
						break;
					case CL_IPV6_SHORT_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						//printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						if(p_key->cls_key_info.cl_ipv6_short_key.msk_l4_port != 7) {
							printk(KERN_INFO "| msk_l4_port(0:sp,dp;1:sp range;2:dp range;3:rng_match) = 0x%x, l4_dp = 0x%x, l4_sp = 0x%x\n", 
								p_key->cls_key_info.cl_ipv6_short_key.msk_l4_port, p_key->cls_key_info.cl_ipv6_short_key.l4_dp, p_key->cls_key_info.cl_ipv6_short_key.l4_sp);
						}
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_l4_vld == 0)
							printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.l4_vld);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_spi)
							printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.msk_spi);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_fragment == 0)
							printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_fragment);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ipv6_flow_lbl == 0)
							printk(KERN_INFO "| ipv6_flow_lbl = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ipv6_flow_lbl);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_l || p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_h) {
							printk(KERN_INFO "| ip_addr_type(0:da;1:sa) = 0x%x, msk_ip_addr_l = 0x%x, msk_ip_addr_h = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_addr_type, p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_l, p_key->cls_key_info.cl_ipv6_short_key.msk_ip_addr_h);
							printk(KERN_INFO "| ip_addr = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_addr_3,
											p_key->cls_key_info.cl_ipv6_short_key.ip_addr_2,
											p_key->cls_key_info.cl_ipv6_short_key.ip_addr_1,
											p_key->cls_key_info.cl_ipv6_short_key.ip_addr_0);
						}
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_da_sa_equal == 0)
							printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_da_sa_equal);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_da_mc == 0)
							printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_da_mc);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_protocol == 0)
							printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_protocol);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_ip_ecn == 0)
							printk(KERN_INFO "| ip_ecn = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_ecn );
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_dscp == 0)
							printk(KERN_INFO "| ip_dscp = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.ip_dscp);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_top_dei == 0)
							printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_dei);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_top_vl_802_1p == 0)
							printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_802_1p);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_top_vid_ptag == 0)
							printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_vid_ptag);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_top_vid == 0)
							printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_vid);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_top_tpid_enc == 0)
							printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.top_tpid_enc);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_vlan_cnt == 0)
							printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.vlan_cnt);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_o_lspid == 0)
							printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.o_lspid);
						if(p_key->cls_key_info.cl_ipv6_short_key.msk_lspid == 0)
							printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_ipv6_short_key.lspid);

						__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						break;
					case CL_SPCL_PKT_KEY:
						for (key_idx = 0; key_idx < CL_2_KEY_NUM; key_idx++) {
							if (!(key_vld & (0x1 << key_idx))) {
								//printk(KERN_INFO "|------------------------------------------------------------------|\n");
								//printk(KERN_INFO "| KEY_IDX %d | NOT USED\n", key_idx);
								continue;
							}
							/* get fib index */
							if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, key_idx, &fib_tbl_idx))
								return;
							//printk(KERN_INFO "|------------------------------------------------------------------|\n");
							printk(KERN_INFO "| KEY IDX %d | FIB IDX %d --------------------------------------------|\n", key_idx, fib_tbl_idx);

							if (key_idx == 0) {
								CL_SPCL_PKT_KEY_USED_PRINT(p_key->cls_key_info, 0);
							} else if (key_idx == 1) {
								CL_SPCL_PKT_KEY_USED_PRINT(p_key->cls_key_info, 1);
							}

							__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						}
						break;
					case CL_MCST_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						//printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						if(p_key->cls_key_info.cl_mcst_key.msk_ip_da_sa_equal == 0)
							printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_da_sa_equal);
						if(p_key->cls_key_info.cl_mcst_key.msk_ip_da_mc == 0)
							printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_da_mc);
						if(p_key->cls_key_info.cl_mcst_key.msk_ip_ver == 0)
							printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_ver);
						if(p_key->cls_key_info.cl_mcst_key.msk_ip_vld == 0)
							printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_mcst_key.ip_vld);
						if(p_key->cls_key_info.cl_mcst_key.msk_pppoe_session == 0)
							printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_mcst_key.pppoe_session_id);
						if(p_key->cls_key_info.cl_mcst_key.msk_inner_vid == 0)
							printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.inner_vid);
						if(p_key->cls_key_info.cl_mcst_key.msk_inner_tpid_enc == 0)
							printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.inner_tpid_enc);
						if(p_key->cls_key_info.cl_mcst_key.msk_top_dei == 0)
							printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_dei);
						if(p_key->cls_key_info.cl_mcst_key.msk_top_vl_802_1p == 0)
							printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_802_1p);
						if(p_key->cls_key_info.cl_mcst_key.msk_top_vid_ptag == 0)
							printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_vid_ptag);
						if(p_key->cls_key_info.cl_mcst_key.msk_top_vid == 0)
							printk(KERN_INFO "| top_vid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_vid);
						if(p_key->cls_key_info.cl_mcst_key.msk_top_tpid_enc == 0)
							printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.top_tpid_enc);
						if(p_key->cls_key_info.cl_mcst_key.msk_vlan_cnt == 0)
							printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_mcst_key.vlan_cnt);
						if(p_key->cls_key_info.cl_mcst_key.msk_mac_da_ip_mc == 0)
							printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_ip_mc);
						if(p_key->cls_key_info.cl_mcst_key.msk_mac_da_mc == 0)
							printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_mc);
						if(p_key->cls_key_info.cl_mcst_key.msk_mac_da_bc == 0)
							printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mac_da_bc);
						if(p_key->cls_key_info.cl_mcst_key.msk_dest_addr == 0)
							printk(KERN_INFO "| dest_addr_type = 0x%x\n", p_key->cls_key_info.cl_mcst_key.dest_addr_type);
						if (p_key->cls_key_info.cl_mcst_key.dest_addr_type) {
							printk(KERN_INFO "| dest_addr.ip_addr = %08x %08x %08x %08x\n",
								p_key->cls_key_info.cl_mcst_key.dest_addr_3,
								p_key->cls_key_info.cl_mcst_key.dest_addr_2,
								p_key->cls_key_info.cl_mcst_key.dest_addr_1,
								p_key->cls_key_info.cl_mcst_key.dest_addr_0);
						} else {
								ca_uint8_t mac[6];
								mac[3] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 24) & 0xff;
								mac[2] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 16) & 0xff;
								mac[1] = (p_key->cls_key_info.cl_mcst_key.dest_addr_0 >> 8) & 0xff;
								mac[0] = p_key->cls_key_info.cl_mcst_key.dest_addr_0 & 0xff;
								mac[4] = p_key->cls_key_info.cl_mcst_key.dest_addr_1 & 0xff;
								mac[5] = (p_key->cls_key_info.cl_mcst_key.dest_addr_1 >> 8) & 0xff;
							printk(KERN_INFO "| dest_addr.mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
								mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
						}
						if(p_key->cls_key_info.cl_mcst_key.msk_mcgid == 0)
							printk(KERN_INFO "| mcgid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.mcgid);
						if(p_key->cls_key_info.cl_mcst_key.msk_o_lspid == 0)
							printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.o_lspid);
						if(p_key->cls_key_info.cl_mcst_key.msk_lspid == 0)
							printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_mcst_key.lspid);

						__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						break;
					case CL_FULL_KEY:
						/* get fib index */
						if (AAL_E_OK != __l3_cls_fib_idx_get(table, key_type, i, 0, &fib_tbl_idx))
							return;
						//printk(KERN_INFO "| FIB IDX %d\n", fib_tbl_idx);

						if(p_key->cls_key_info.cl_full_key.msk_tcp_rdp_ctrl == 0)
							printk(KERN_INFO "| tcp_rdp_ctrl = 0x%x\n", p_key->cls_key_info.cl_full_key.tcp_rdp_ctrl);
						if(p_key->cls_key_info.cl_full_key.msk_l4_port != 7) {
							printk(KERN_INFO "| msk_l4_port(0:sp,dp;1:sp range;2:dp range;3:rng_match) = 0x%x, l4_dp = 0x%x, l4_sp = 0x%x\n", 
								p_key->cls_key_info.cl_full_key.msk_l4_port, p_key->cls_key_info.cl_full_key.l4_dp, p_key->cls_key_info.cl_full_key.l4_sp);
						}
						if(p_key->cls_key_info.cl_full_key.msk_l4_vld == 0)
							printk(KERN_INFO "| l4_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.l4_vld);
						if(p_key->cls_key_info.cl_full_key.msk_l3_chksum_err == 0)
							printk(KERN_INFO "| l3_chksum_err = 0x%x\n", p_key->cls_key_info.cl_full_key.l3_chksum_err);
						if(p_key->cls_key_info.cl_full_key.msk_ip_fragment == 0)
							printk(KERN_INFO "| ip_fragment = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_fragment);
						if(p_key->cls_key_info.cl_full_key.msk_ip_options == 0)
							printk(KERN_INFO "| ip_options = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_options);
						if(p_key->cls_key_info.cl_full_key.msk_ip_da_sa_equal == 0)
							printk(KERN_INFO "| ip_da_sa_equal = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_da_sa_equal);
						if(p_key->cls_key_info.cl_full_key.msk_ip_da_mc == 0)
							printk(KERN_INFO "| ip_da_mc(1:ip_da[31:28] is 4'b1110 or ip_da[127:120] is 8'hff) = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_da_mc);
						if((p_key->cls_key_info.cl_full_key.msk_ip_da_l != 0) || (p_key->cls_key_info.cl_full_key.msk_ip_da_h != 0)) {
							printk(KERN_INFO "| msk_ip_da_l = 0x%x, msk_ip_da_h = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_da_l, p_key->cls_key_info.cl_full_key.msk_ip_da_h);
							printk(KERN_INFO "| ip_da = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_full_key.ip_da_3,
												p_key->cls_key_info.cl_full_key.ip_da_2,
												p_key->cls_key_info.cl_full_key.ip_da_1,
												p_key->cls_key_info.cl_full_key.ip_da_0);
						}
						if((p_key->cls_key_info.cl_full_key.msk_ip_sa_l != 0) || (p_key->cls_key_info.cl_full_key.msk_ip_sa_h != 0)) {
							printk(KERN_INFO "| msk_ip_sa_l = 0x%x, msk_ip_sa_h = 0x%x\n", p_key->cls_key_info.cl_full_key.msk_ip_sa_l, p_key->cls_key_info.cl_full_key.msk_ip_sa_h);
							printk(KERN_INFO "| ip_sa = %08x %08x %08x %08x\n", p_key->cls_key_info.cl_full_key.ip_sa_3,
												p_key->cls_key_info.cl_full_key.ip_sa_2,
												p_key->cls_key_info.cl_full_key.ip_sa_1,
												p_key->cls_key_info.cl_full_key.ip_sa_0);
						}
						if(p_key->cls_key_info.cl_full_key.msk_ip_protocol == 0)
							printk(KERN_INFO "| ip_protocol = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_protocol);
						if(p_key->cls_key_info.cl_full_key.msk_ip_ecn == 0)
							printk(KERN_INFO "| ip_ecn = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_ecn);
						if(p_key->cls_key_info.cl_full_key.msk_dscp == 0)
							printk(KERN_INFO "| ip_dscp = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_dscp);
						if(p_key->cls_key_info.cl_full_key.msk_ip_ver == 0)
							printk(KERN_INFO "| ip_ver = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_ver);
						if(p_key->cls_key_info.cl_full_key.msk_ip_vld == 0)
							printk(KERN_INFO "| ip_vld = 0x%x\n", p_key->cls_key_info.cl_full_key.ip_vld);
						if(p_key->cls_key_info.cl_full_key.msk_ppp_protocol == 0)
							printk(KERN_INFO "| ppp_protocol_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.ppp_protocol_enc);
						if(p_key->cls_key_info.cl_full_key.msk_pppoe_session == 0)
							printk(KERN_INFO "| pppoe_session_id = 0x%x\n", p_key->cls_key_info.cl_full_key.pppoe_session_id);
						if(p_key->cls_key_info.cl_full_key.msk_inner_dei == 0)
							printk(KERN_INFO "| inner_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_dei);
						if(p_key->cls_key_info.cl_full_key.msk_inner_802_1p == 0)
							printk(KERN_INFO "| inner_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_802_1p);
						if(p_key->cls_key_info.cl_full_key.msk_inner_vid == 0)
							printk(KERN_INFO "| inner_vid = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_vid);
						if(p_key->cls_key_info.cl_full_key.msk_inner_tpid_enc == 0)
							printk(KERN_INFO "| inner_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.inner_tpid_enc);
						if(p_key->cls_key_info.cl_full_key.msk_top_dei == 0)
							printk(KERN_INFO "| top_dei = 0x%x\n", p_key->cls_key_info.cl_full_key.top_dei);
						if(p_key->cls_key_info.cl_full_key.msk_top_vl_802_1p == 0)
							printk(KERN_INFO "| top_802_1p = 0x%x\n", p_key->cls_key_info.cl_full_key.top_802_1p);
						if(p_key->cls_key_info.cl_full_key.msk_top_vid_ptag == 0)
							printk(KERN_INFO "| top_vid_ptag = 0x%x\n", p_key->cls_key_info.cl_full_key.top_vid_ptag);
						if((p_key->cls_key_info.cl_full_key.vid_ctl != 2) && !((p_key->cls_key_info.cl_full_key.vid_ctl == 3) && (p_key->cls_key_info.cl_full_key.top_vid == 0xfff)))
							printk(KERN_INFO "| vid_ctl(0:match;1:range;2:reserve;3:mask) = 0x%x, top_vid = 0x%x\n", p_key->cls_key_info.cl_full_key.vid_ctl, p_key->cls_key_info.cl_full_key.top_vid);
						if(p_key->cls_key_info.cl_full_key.msk_top_tpid_enc == 0)
							printk(KERN_INFO "| top_tpid_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.top_tpid_enc);
						if(p_key->cls_key_info.cl_full_key.msk_vlan_cnt == 0)
							printk(KERN_INFO "| vlan_cnt = 0x%x\n", p_key->cls_key_info.cl_full_key.vlan_cnt);
						if(p_key->cls_key_info.cl_full_key.llc_type_enc_mask == 0)
							printk(KERN_INFO "| llc_type_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.llc_type_enc);
						if(p_key->cls_key_info.cl_full_key.pktlen_rng_match_mask == 0)
							printk(KERN_INFO "| pktlen_rng_match_vec = 0x%x\n", p_key->cls_key_info.cl_full_key.pktlen_rng_match_vec);
						if(p_key->cls_key_info.cl_full_key.msk_len_encoded == 0)
							printk(KERN_INFO "| len_encoded = 0x%x\n", p_key->cls_key_info.cl_full_key.len_encoded);
						if(p_key->cls_key_info.cl_full_key.msk_ethertype_enc == 0)
							printk(KERN_INFO "| ethertype_enc = 0x%x\n", p_key->cls_key_info.cl_full_key.ethertype_enc);
						if(p_key->cls_key_info.cl_full_key.msk_mac_sa == 0) {
							printk(KERN_INFO "| mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\n",
								p_key->cls_key_info.cl_full_key.mac_sa5,
								p_key->cls_key_info.cl_full_key.mac_sa4,
								p_key->cls_key_info.cl_full_key.mac_sa3,
								p_key->cls_key_info.cl_full_key.mac_sa2,
								p_key->cls_key_info.cl_full_key.mac_sa1,
								p_key->cls_key_info.cl_full_key.mac_sa0);
						}
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_rsvd == 0)
							printk(KERN_INFO "| mac_da_rsvd = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_rsvd);
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_ip_mc == 0)
							printk(KERN_INFO "| mac_da_ip_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_ip_mc);
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_mc == 0)
							printk(KERN_INFO "| mac_da_mc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_mc);
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_bc == 0)
							printk(KERN_INFO "| mac_da_bc = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_bc);
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_an_hit == 0)
							printk(KERN_INFO "| mac_da_an_hit = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_an_hit);
						if(p_key->cls_key_info.cl_full_key.msk_mac_da_an_sel == 0)
							printk(KERN_INFO "| mac_da_an_sel = 0x%x\n", p_key->cls_key_info.cl_full_key.mac_da_an_sel);
						if((p_key->cls_key_info.cl_full_key.msk_mdata != CA_UINT16_INVALID) || (p_key->cls_key_info.cl_full_key.mdata_1 != CA_UINT32_INVALID) || (p_key->cls_key_info.cl_full_key.mdata_0 != CA_UINT32_INVALID))
							printk(KERN_INFO "| msk_mdata = 0x%x, mdata = %08x %08x\n", p_key->cls_key_info.cl_full_key.msk_mdata, p_key->cls_key_info.cl_full_key.mdata_1, p_key->cls_key_info.cl_full_key.mdata_0);
						if(p_key->cls_key_info.cl_full_key.msk_mcgid == 0)
							printk(KERN_INFO "| mcgid = 0x%x\n", p_key->cls_key_info.cl_full_key.mcgid);
						if(p_key->cls_key_info.cl_full_key.msk_o_lspid == 0)
							printk(KERN_INFO "| o_lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.o_lspid);
						if(p_key->cls_key_info.cl_full_key.msk_lspid == 0)
							printk(KERN_INFO "| lspid = 0x%x\n", p_key->cls_key_info.cl_full_key.lspid);

						__l3_cls_fib_tbl_used_entry_print(fib_tbl_idx, 1);
						break;
					default:
						;
				}

			printk(KERN_INFO "|------------------------------------------------------------------|\n");
			//cond_resched();
		}
	}
}


static void __l3_cls_key_tbl_used_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_key_tbl_used_entry_dump(&l3_cls_key_table, start_idx, count);
}
#endif

static aal_table_t l3_cls_key_table = {
	.table_id = AAL_TABLE_L3_CLS_KEY,
	.table_name = "L3 CLS KEY",
	.max_entry = L3_CLS_KEY_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cls_key_tbl_entry_t),
	.access_reg = L3FE_CLS_KEY_TBL_ACCESS,
	.data_reg_num = 11,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = __l3_cls_key_tbl_entry_add_by_idx,
		.entry_del = __l3_cls_key_tbl_entry_del,
		.entry_set = __l3_cls_key_tbl_entry_set,
		.entry_get = __l3_cls_key_tbl_entry_get,
		.entry_get_by_idx = __l3_cls_key_tbl_entry_get_by_idx,
		.table_flush = __l3_cls_key_table_flush,
		.get_avail_count = __l3_cls_key_tbl_get_avail_count,
		.entry_print = __l3_cls_key_tbl_entry_print,
#if defined(CONFIG_LUNA_G3_SERIES)
		.used_entry_print = __l3_cls_key_tbl_used_entry_print,
#endif
	},
};

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static int __hgu_8279_l3_cls_key_tbl_entry_add_by_idx(void *entry, unsigned int cls_idx)
{
	return generic_entry_add_by_idx(&hgu_8279_l3_cls_key_table, entry, cls_idx);
}

static int __hgu_8279_l3_cls_key_tbl_entry_del(unsigned int cls_idx)
{
	return generic_entry_del(&hgu_8279_l3_cls_key_table, cls_idx);
}

static int __hgu_8279_l3_cls_key_tbl_entry_set(unsigned int cls_idx, void *entry)
{
	return generic_entry_set(&hgu_8279_l3_cls_key_table, cls_idx, entry);
}

static int __hgu_8279_l3_cls_key_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&hgu_8279_l3_cls_key_table, entry, rslt_idx);
}

static int __hgu_8279_l3_cls_key_tbl_entry_get_by_idx(unsigned int cls_idx, void *entry)
{
	return generic_entry_get_by_idx(&hgu_8279_l3_cls_key_table, cls_idx, entry);
}

static int __hgu_8279_l3_cls_key_table_flush(void)
{
	return generic_table_flush(&hgu_8279_l3_cls_key_table);
}

static int __hgu_8279_l3_cls_key_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&hgu_8279_l3_cls_key_table);
}

static void __hgu_8279_l3_cls_key_tbl_entry_print(unsigned int start_idx, int count)
{
	return __l3_cls_key_tbl_entry_dump(&hgu_8279_l3_cls_key_table, start_idx, count);
}

static aal_table_t hgu_8279_l3_cls_key_table = {
	.table_id = AAL_TABLE_8279_L3_CLS_KEY,
	.table_name = "HGU 8279 L3 CLS KEY",
	.max_entry = HGU_L3_CLS_KEY_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3_cls_key_tbl_entry_t),
	.access_reg = L3FE_CLS_KEY_TBL_ACCESS,
	.data_reg_num = 11,
	.op = {
		.entry_add = NULL,
		.entry_add_by_idx = __hgu_8279_l3_cls_key_tbl_entry_add_by_idx,
		.entry_del = __hgu_8279_l3_cls_key_tbl_entry_del,
		.entry_set = __hgu_8279_l3_cls_key_tbl_entry_set,
		.entry_get = __hgu_8279_l3_cls_key_tbl_entry_get,
		.entry_get_by_idx = __hgu_8279_l3_cls_key_tbl_entry_get_by_idx,
		.table_flush = __hgu_8279_l3_cls_key_table_flush,
		.get_avail_count = __hgu_8279_l3_cls_key_tbl_get_avail_count,
		.entry_print = __hgu_8279_l3_cls_key_tbl_entry_print,
	},
};
#endif

static ca_int32_t __l3_cls_fib_mode_get(
	aal_table_t *fib_table,
	ca_uint32_t fib_tbl_idx,
	ca_uint16_t *short_fib,
	l3_cls_fib_mode_t *fib_mode)
{
	ca_int32_t rc, key_tbl_idx, i, fib_def_entry;
	cls_fib_t fib_entry;
	l3_cls_key_t key_entry;
	l3_cls_key_type_t key_type;
	aal_table_t *key_table = &l3_cls_key_table;

	if (fib_tbl_idx >= fib_table->max_entry)
		return AAL_E_OUTRANGE;
	if (fib_table == NULL || short_fib == NULL || fib_mode == NULL)
		return AAL_E_NULLPTR;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (fib_table == &hgu_8279_l3_cls_fib_table)
		key_table = &hgu_8279_l3_cls_key_table;
#endif
	fib_def_entry = fib_table->max_entry - L3_CLS_FIB_TBL_DEF_ENTRY_MAX;
	/* get key type */
	if (fib_tbl_idx < fib_def_entry) {
		key_tbl_idx = fib_tbl_idx >> 2;
		if (AAL_E_OK != (rc = hw_entry_get(key_table, key_tbl_idx, key_entry.cls_key_info.key_tbl_data)))
			return rc;
		if (key_entry.cls_key_info.key_tbl_data[0].cls_key_valid)
			key_type = key_entry.cls_key_info.key_tbl_data[0].cls_key_type;
		else {
			/* it may be full key */
			key_tbl_idx--;
			if (AAL_E_OK != (rc = hw_entry_get(key_table, key_tbl_idx, key_entry.cls_key_info.key_tbl_data)))
				return rc;
			if (key_entry.cls_key_info.key_tbl_data[0].cls_key_valid && key_entry.cls_key_info.key_tbl_data[0].cls_key_type == CL_FULL_KEY)
				key_type = CL_FULL_KEY;
			else {
				*short_fib = 1;
				*fib_mode = 0xff;
				return AAL_E_INVALID;
			}
		}
	}
	/* get fib data */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
		if (AAL_E_OK != (rc = hw_entry_get(fib_table, fib_tbl_idx+i, &fib_entry.cls_fib_data.fib_tbl_data[i])))
			return rc;
	}
	/* get fib mode */
	if (fib_tbl_idx < fib_def_entry && key_type == CL_IF_ID_KEY) {
		/* get short fib mode */
		*short_fib = 1;
		*fib_mode = fib_entry.cls_fib_data.fib_tbl_data[0].fib_format_sel;
	} else {
		/* get standard fib mode */
		*short_fib = 0;
		if (fib_tbl_idx & 0x1)
			*fib_mode = fib_entry.cls_fib_data.fib_tbl_data[0].fib_format_sel;
		else
			*fib_mode = fib_entry.cls_fib_data.fib_tbl_data[1].fib_format_sel;
	}

	return AAL_E_OK;
}

static ca_int32_t __l3_cls_key_type(
	cls_key_t *key,
	l3_cls_fib_mode_t fib_mode,
	l3_cls_key_type_t *key_type)
{
	ca_uint64_t mask=0;

	if (key == NULL || key_type == NULL)
		return AAL_E_NULLPTR;

	if (!key->msk_lspid)
		mask |= CLS_KEY_MSK_LSPID;
	if (!key->msk_o_lspid)
		mask |= CLS_KEY_MSK_O_LSPID;
	if (!key->msk_mcgid)
		mask |= CLS_KEY_MSK_MCGID;
	if (key->msk_mdata != 0xffff)
		mask |= CLS_KEY_MSK_MDATA;
	if (!key->msk_dest_addr) {
		if (key->dest_addr_type) {
			if (key->dest_addr.dest_ip_addr.ip_addr_1 == 0 && key->dest_addr.dest_ip_addr.ip_addr_2 == 0 &&
				key->dest_addr.dest_ip_addr.ip_addr_3 == 0)
				mask |= CLS_KEY_MSK_IPV4_DA;
			else
				mask |= CLS_KEY_MSK_IPV6_DA;
		} else
			mask |= CLS_KEY_MSK_MAC_DA;
	}
	if (!key->msk_mac_da)
		mask |= CLS_KEY_MSK_MAC_DA;
	if (!key->msk_mac_da_an_sel)
		mask |= CLS_KEY_MSK_MAC_DA_AN_SEL;
	if (!key->msk_mac_da_an_hit)
		mask |= CLS_KEY_MSK_MAC_DA_AN_HIT;
	if (!key->msk_mac_da_bc)
		mask |= CLS_KEY_MSK_MAC_DA_BC;
	if (!key->msk_mac_da_mc)
		mask |= CLS_KEY_MSK_MAC_DA_MC;
	if (!key->msk_mac_da_ip_mc)
		mask |= CLS_KEY_MSK_MAC_DA_IP_MC;
	if (!key->msk_mac_da_rsvd)
		mask |= CLS_KEY_MSK_MAC_DA_RSVD;
	if (!key->msk_mac_sa)
		mask |= CLS_KEY_MSK_MAC_SA;
	if (!key->msk_ethertype_enc)
		mask |= CLS_KEY_MSK_ETHERTYPE_ENC;
	if (!key->msk_len_encoded)
		mask |= CLS_KEY_MSK_LEN_ENCODED;
	if (!key->pktlen_rng_match_mask)
		mask |= CLS_KEY_MSK_PKTLEN_RNG_MATCH_VEC;
	if (!key->llc_type_enc_mask)
		mask |= CLS_KEY_MSK_LLC_TYPE_ENC;
	if (!key->msk_vlan_cnt)
		mask |= CLS_KEY_MSK_VLAN_CNT;
	if (!key->msk_top_tpid_enc)
		mask |= CLS_KEY_MSK_TOP_TPID_ENC;
	if (!key->msk_top_vid)
		mask |= CLS_KEY_MSK_TOP_VID;
	if (key->vid_ctl != 3) {
		if (key->vid_ctl == 0)
			mask |= CLS_KEY_MSK_TOP_VID;
		else
			mask |= CLS_KEY_MSK_VID_CTL;
	}
	if (!key->msk_top_vid_ptag)
		mask |= CLS_KEY_MSK_TOP_VID_PTAG;
	if (!key->msk_top_vl_802_1p)
		mask |= CLS_KEY_MSK_TOP_VL_802_1P;
	if (!key->msk_top_dei)
		mask |= CLS_KEY_MSK_TOP_DEI;
	if (!key->msk_inner_tpid_enc)
		mask |= CLS_KEY_MSK_INNER_TPID_ENC;
	if (!key->msk_inner_vid)
		mask |= CLS_KEY_MSK_INNER_VID;
	if (!key->msk_inner_802_1p)
		mask |= CLS_KEY_MSK_INNER_802_1P;
	if (!key->msk_inner_dei)
		mask |= CLS_KEY_MSK_INNER_DEI;
	if (!key->msk_pppoe_session)
		mask |= CLS_KEY_MSK_PPPOE_SESSION;
	if (!key->msk_ppp_protocol)
		mask |= CLS_KEY_MSK_PPP_PROTOCOL;
	if (!key->msk_ip_vld)
		mask |= CLS_KEY_MSK_IP_VLD;
	if (!key->msk_ip_ver)
		mask |= CLS_KEY_MSK_IP_VER;
	if (!key->msk_dscp)
		mask |= CLS_KEY_MSK_DSCP;
	if (!key->msk_ip_ecn)
		mask |= CLS_KEY_MSK_IP_ECN;
	if (!key->msk_ip_protocol)
		mask |= CLS_KEY_MSK_IP_PROTOCOL;
	if (!key->msk_ip_sa_h && key->msk_ip_sa_l && key->ip_sa.ip_addr_3 == 0 && key->ip_sa.ip_addr_2 == 0 && key->ip_sa.ip_addr_1 == 0)
		mask |= CLS_KEY_MSK_IPV4_SA;
	else if (key->msk_ip_sa_h || key->msk_ip_sa_l)
		mask |= CLS_KEY_MSK_IPV6_SA;
	if (!key->msk_ip_sa_enc)
		mask |= CLS_KEY_MSK_IP_SA_ENC;
	if (!key->msk_ip_da_h && key->msk_ip_da_l && key->ip_da.ip_addr_3 == 0 && key->ip_da.ip_addr_2 == 0 && key->ip_da.ip_addr_1 == 0)
		mask |= CLS_KEY_MSK_IPV4_DA;
	else if (key->msk_ip_da_h || key->msk_ip_da_l)
		mask |= CLS_KEY_MSK_IPV6_DA;
	if (!key->msk_ip_da_enc)
		mask |= CLS_KEY_MSK_IP_DA_ENC;
	if (!key->msk_ip_da_mc)
		mask |= CLS_KEY_MSK_IP_DA_MC;
	if (!key->msk_ip_da_sa_equal)
		mask |= CLS_KEY_MSK_IP_DA_SA_EQUAL;
	if (!key->msk_ip_addr_h && key->msk_ip_addr_l && key->ip_addr.ip_addr_3 == 0 && key->ip_addr.ip_addr_2 == 0 && key->ip_addr.ip_addr_1 == 0) {
		if (key->ip_addr_type)
			mask |= CLS_KEY_MSK_IPV4_SA;
		else
			mask |= CLS_KEY_MSK_IPV4_DA;
	} else if (key->msk_ip_addr_h || key->msk_ip_addr_l) {
		if (key->ip_addr_type)
			mask |= CLS_KEY_MSK_IPV6_SA;
		else
			mask |= CLS_KEY_MSK_IPV6_DA;
	}
	if (!key->msk_ipv6_flow_lbl)
		mask |= CLS_KEY_MSK_IPV6_FLOW_LBL;
	if (!key->msk_ip_options)
		mask |= CLS_KEY_MSK_IP_OPTIONS;
	if (!key->msk_ip_fragment)
		mask |= CLS_KEY_MSK_IP_FRAGMENT;
	if (!key->msk_ipv6_ndp)
		mask |= CLS_KEY_MSK_IPV6_NDP;
	if (!key->msk_ipv6_hbh)
		mask |= CLS_KEY_MSK_IPV6_HBH;
	if (!key->msk_ipv6_rh)
		mask |= CLS_KEY_MSK_IPV6_RH;
	if (!key->msk_ipv6_doh)
		mask |= CLS_KEY_MSK_IPV6_DOH;
	if (!key->msk_icmp_vld)
		mask |= CLS_KEY_MSK_ICMP_VLD;
	if (!key->msk_icmp_type)
		mask |= CLS_KEY_MSK_ICMP_TYPE;
	if (!key->msk_spi)
		mask |= CLS_KEY_MSK_SPI;
	if (!key->msk_l3_chksum_err)
		mask |= CLS_KEY_MSK_L3_CHKSUM_ERR;
	if (!key->msk_l4_vld)
		mask |= CLS_KEY_MSK_L4_VLD;
	if (key->msk_l4_port != 7)
		mask |= CLS_KEY_MSK_L4_PORT;
	if (!key->msk_tcp_rdp_ctrl)
		mask |= CLS_KEY_MSK_TCP_RDP_CTRL;
	if (!key->spcl_pkt_enc_msk)
		mask |= CLS_KEY_MSK_SPCL_PKT_ENC;
	if (!key->spcl_pkt_hdr_mtch_msk)
		mask |= CLS_KEY_MSK_SPCL_PKT_HDR_MTCH;

	if (ca_aal_debug & AAL_DBG_L3CLS) {
		printk("%s: mask = %llx\n", __func__, mask);
		printk("%s: CL_IF_ID_KEY_MSK = %llx\n", __func__, CL_IF_ID_KEY_MSK);
		printk("%s: CL_IPV4_SA_SHORT_KEY_MSK = %llx\n", __func__, CL_IPV4_SA_SHORT_KEY_MSK);
		printk("%s: CL_IPV4_DA_SHORT_KEY_MSK = %llx\n", __func__, CL_IPV4_DA_SHORT_KEY_MSK);
		printk("%s: CL_SPCL_PKT_KEY_MSK = %llx\n", __func__, CL_SPCL_PKT_KEY_MSK);
		printk("%s: CL_IPV4_TUNNEL_ID_KEY_MSK = %llx\n", __func__, CL_IPV4_TUNNEL_ID_KEY_MSK);
		printk("%s: CL_IPV6_SA_SHORT_KEY_MSK = %llx\n", __func__, CL_IPV6_SA_SHORT_KEY_MSK);
		printk("%s: CL_IPV6_DA_SHORT_KEY_MSK = %llx\n", __func__, CL_IPV6_DA_SHORT_KEY_MSK);
		printk("%s: CL_MCST_MAC_DA_KEY_MSK = %llx\n", __func__, CL_MCST_MAC_DA_KEY_MSK);
		printk("%s: CL_MCST_IP_DA_KEY_MSK = %llx\n", __func__, CL_MCST_IP_DA_KEY_MSK);
		printk("%s: CL_FULL_KEY_MSK = %llx\n", __func__, CL_FULL_KEY_MSK);
	}

	/* determine key type */
	if ((fib_mode == CL_FIB_MOD_0 || fib_mode == CL_FIB_MOD_1) && (mask == (mask & CL_IF_ID_KEY_MSK)))
		*key_type = CL_IF_ID_KEY;
	else if (mask == (mask & CL_IPV4_SA_SHORT_KEY_MSK))
		*key_type = CL_IPV4_SHORT_KEY;
	else if (mask == (mask & CL_IPV4_DA_SHORT_KEY_MSK))
		*key_type = CL_IPV4_SHORT_KEY;
	else if (mask == (mask & CL_SPCL_PKT_KEY_MSK))
		*key_type = CL_SPCL_PKT_KEY;
	else if (mask == (mask & CL_IPV4_TUNNEL_ID_KEY_MSK))
		*key_type = CL_IPV4_TUNNEL_ID_KEY;
	else if (mask == (mask & CL_IPV6_SA_SHORT_KEY_MSK))
		*key_type = CL_IPV6_SHORT_KEY;
	else if (mask == (mask & CL_IPV6_DA_SHORT_KEY_MSK))
		*key_type = CL_IPV6_SHORT_KEY;
	else if (mask == (mask & CL_MCST_MAC_DA_KEY_MSK))
		*key_type = CL_MCST_KEY;
	else if (mask == (mask & CL_MCST_IP_DA_KEY_MSK) && !key->msk_dest_addr)
		*key_type = CL_MCST_KEY;
	else if (mask == (mask & CL_FULL_KEY_MSK)) {
		*key_type = CL_FULL_KEY;
		/* for ipv4 addr key using full key, need enable msk_ip_ver and msk_ip_da/sa + 96 bits */
		if (mask & CLS_KEY_MSK_IPV4_SA) {
			key->ip_ver = 0;
			key->msk_ip_ver = 0;
			key->msk_ip_sa_h = (key->msk_ip_sa_l + 96) >> 7;
			key->msk_ip_sa_l = (key->msk_ip_sa_l + 96) & 0x7f;
		}
		if (mask & CLS_KEY_MSK_IPV4_DA) {
			key->ip_ver = 0;
			key->msk_ip_ver = 0;
			key->msk_ip_da_h = (key->msk_ip_da_l + 96) >> 7;
			key->msk_ip_da_l = (key->msk_ip_da_l + 96) & 0x7f;
		}
	} else
		return AAL_E_OPNOTSUPP;

	if (ca_aal_debug & AAL_DBG_L3CLS)
		printk("%s: key_type = %d\n", __func__, *key_type);

	return AAL_E_OK;
}

static ca_int32_t __l3_cls_fib_mode(
	cls_action_t *action,
	l3_cls_fib_mode_t *fib_mode)
{
	ca_uint64_t mask=0;

	if (action == NULL || fib_mode == NULL)
		return AAL_E_NULLPTR;


	if (action->t2_ctrl_vld || action->t3_ctrl_vld || action->t4_ctrl_vld || action->t5_ctrl_vld ||
		action->stage2_ctrl_vld || action->stage3_ctrl_vld || action->keep_orig_pkt_vld ||
		action->keep_ts_vld || action->pop_l3_vld)
		mask |= CLS_FIB_MSK_CTRL;

	if (action->dpid_vld || action->no_drop_vld || action->mrr_vld)
		mask |= CLS_FIB_MSK_DPID;

	if (action->cos_sel || action->top_802_1p_sel || action->inner_802_1p_sel || action->ip_dscp_update_en ||
		action->ip_dscp_marked_vld || action->ip_tos_vld || action->ip_ecn_vld)
		mask |= CLS_FIB_MSK_QOS;

	if (action->pol_vld)
		mask |= CLS_FIB_MSK_POL;

	if (action->mdata_w_vld_3)
		mask |= CLS_FIB_MSK_MDATA_W3;

	if (action->mdata_w_vld_2)
		mask |= CLS_FIB_MSK_MDATA_W2;

	if (action->mdata_w_vld_1)
		mask |= CLS_FIB_MSK_MDATA_W1;

	if (action->mdata_w_vld_0)
		mask |= CLS_FIB_MSK_MDATA_W0;

	if (action->mac_da_vld)
		mask |= CLS_FIB_MSK_MAC_DA;

	if (action->mac_sa_vld)
		mask |= CLS_FIB_MSK_MAC_SA;

	if (action->vlan_vld)
		mask |= CLS_FIB_MSK_VLAN;

	if (action->l3_egress_if_vld || action->l2_format_vld || action->ip_mtu_enc_vld || action->six_rd_ipda_ctrl_vld ||
		action->ip_ttl_vld)
		mask |= CLS_FIB_MSK_L3_EGRESS_IF;

	if ((action->pppoe_vld) || (action->pppoe_set))
		mask |= CLS_FIB_MSK_PPPOE;

	if (action->ip_sa_h_vld)
		mask |= CLS_FIB_MSK_IP_SA_H;

	if (action->ip_sa_l_vld)
		mask |= CLS_FIB_MSK_IP_SA_L;

	if (action->ip_da_h_vld)
		mask |= CLS_FIB_MSK_IP_DA_H;

	if (action->ip_da_l_vld)
		mask |= CLS_FIB_MSK_IP_DA_L;

	if (action->l4_sp_vld)
		mask |= CLS_FIB_MSK_L4_SP;

	if (action->l4_dp_vld)
		mask |= CLS_FIB_MSK_L4_DP;

	/* determine fib mode */
	if (mask == (mask & CL_FIB_MOD_0_MSK))
		*fib_mode = CL_FIB_MOD_0;
	else if (mask == (mask & CL_FIB_MOD_1_MSK))
		*fib_mode = CL_FIB_MOD_1;
	else if (mask == (mask & CL_FIB_MOD_2_MSK))
		*fib_mode = CL_FIB_MOD_2;
	else if (mask == (mask & CL_FIB_MOD_3_MSK))
		*fib_mode = CL_FIB_MOD_3;
	else if (mask == (mask & CL_FIB_MOD_4_MSK))
		*fib_mode = CL_FIB_MOD_4;
	else
		return CA_E_NOT_SUPPORT;

	if (ca_aal_debug & AAL_DBG_L3CLS) {
		printk("%s: mask = 0x%llx\n", __func__, mask);
		printk("%s: fib_mode = %d\n", __func__, *fib_mode);
	}
	return AAL_E_OK;
}

static ca_int32_t __l3_cls_empty_key_entry_find(
	aal_table_t *key_table,
	ca_uint32_t profile_id,
	l3_cls_key_type_t key_type,
	cls_rslt_type_t rslt_type,
	cls_rule_prio_t pri,
	ca_uint32_t *key_tbl_idx,
	ca_uint8_t *key_idx,
	l3_cls_key_tbl_entry_t *data)
{
	ca_uint16_t i,j, key_tbl_idx_min, key_tbl_idx_max, max_entry_per_pro;
	ca_uint8_t mask, key_num, find=0;
#if defined(CONFIG_LUNA_G3_SERIES)
	l3_cls_key_tbl_entry_t key_data_later;
	ca_uint16_t key_tbl_idx_later;
	ca_uint8_t need_search_next;
#endif

	if (profile_id >=L3_CLS_PROFILE_MAX || key_type > CL_FULL_KEY || key_type < CL_IF_ID_KEY)
		return AAL_E_OUTRANGE;
	if (key_tbl_idx == NULL || key_idx == NULL || data == NULL)
		return AAL_E_NULLPTR;

	max_entry_per_pro = key_table->max_entry/L3_CLS_PROFILE_MAX;
	key_tbl_idx_min = profile_id*max_entry_per_pro;
	key_tbl_idx_max = key_tbl_idx_min+max_entry_per_pro-1;

	/* find an empty entry */
	for (i=key_tbl_idx_min; i<=key_tbl_idx_max; i++) {

#if 1
		if ((key_type == CL_FULL_KEY) && (i%2))
#else
		/* Bug 48109: hw issue to degrade the feature, only support 8 full-key rules for each profile */
		if ((key_type == CL_FULL_KEY) && (i%8))
#endif
			continue;

#if 0
		if (AAL_E_OK != aal_entry_get_by_idx(AAL_TABLE_L3_CLS_KEY, i, data))
			continue;
#else
		if (AAL_E_OK != hw_entry_get(key_table, i, data))
			continue;
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
		need_search_next = FALSE;
		if(!find && (rslt_type == CL_RSLT_TYPE_1)) {
			for (key_tbl_idx_later=i+1; key_tbl_idx_later<=key_tbl_idx_max; key_tbl_idx_later++) {
				memset(&key_data_later, 0, sizeof(l3_cls_key_tbl_entry_t));
				if (AAL_E_OK != hw_entry_get(key_table, key_tbl_idx_later, &key_data_later))
					continue;
				if (!(key_data_later.cls_key_valid))
					continue;
				if ((key_data_later.cls_rslt_type == rslt_type) && (key_data_later.cls_pri == pri)) {
					need_search_next = TRUE;
					//printk("%s:%d search empty key entry again to make sure priority work(PRI %d/TYPE %d/FIND_IDX %d/CUR_IDX %d).\n", __func__, __LINE__, pri, rslt_type, key_tbl_idx_later, i);
					i = key_tbl_idx_later-1;
					break;
				}
			}
		}
		if(need_search_next)
			continue;
#endif

		if ((data->cls_key_valid == 0) && !find) {
			*key_tbl_idx = i;
			*key_idx = 0;
			find = 1;
		}

		switch(key_type) {
			case CL_IF_ID_KEY:
			case CL_IPV4_SHORT_KEY:
			case CL_SPCL_PKT_KEY:
				/* 4 or 2 keys */
				if (data->cls_key_valid && (data->cls_key_type == key_type) && (data->cls_rslt_type == rslt_type) && (data->cls_pri == pri)) {
					if (key_type == CL_IF_ID_KEY)
						key_num = CL_4_KEY_NUM;
					else
						key_num = CL_2_KEY_NUM;
					for (j=0; j<key_num; j++) {
						mask = 0x1 << j;
						if ((data->cls_key_valid & mask) == 0) {
							*key_tbl_idx = i;
							*key_idx = j;
							return AAL_E_OK;
						}
					}
				}
				break;
			case CL_FULL_KEY:
				/* 0.5 keys */
				if (!find)
					continue;
				/* check the next entry */
#if 0
				if (AAL_E_OK != aal_entry_get_by_idx(AAL_TABLE_L3_CLS_KEY, i+1, data)) {
					find = 0;
					continue;
				}
#else
				if (AAL_E_OK !=  hw_entry_get(key_table, i+1, data)) {
					find = 0;
					continue;
				}
#endif
				if (data->cls_key_valid == 0) {
					*key_tbl_idx = i;
					*key_idx = 0;
					return AAL_E_OK;
				}else
					find = 0;
				break;
			default:
				/* 1 keys */
				/* CL_IPV4_TUNNEL_ID_KEY */
				/* CL_IPV6_SHORT_KEY */
				/* CL_MCST_KEY */
				if (find)
					return AAL_E_OK;
		}

		if (data->cls_key_valid && (data->cls_key_type ==CL_FULL_KEY))
			i++;
	}

	if (!find)
		return AAL_E_TBLFULL;

	memset(data, 0, sizeof(l3_cls_key_tbl_entry_t));

	return AAL_E_OK;
}

#define CL_IF_ID_KEY_SET(o, key_idx, n) {\
	o.cl_if_id_key.msk_ip_fragment_##key_idx = n->msk_ip_fragment;\
	o.cl_if_id_key.ip_fragment_##key_idx = n->ip_fragment;\
	o.cl_if_id_key.msk_ip_da_sa_equal_##key_idx = n->msk_ip_da_sa_equal;\
	o.cl_if_id_key.ip_da_sa_equal_##key_idx = n->ip_da_sa_equal;\
	o.cl_if_id_key.msk_ip_da_mc_##key_idx = n->msk_ip_da_mc;\
	o.cl_if_id_key.ip_da_mc_##key_idx = n->ip_da_mc;\
	o.cl_if_id_key.msk_ip_ver_##key_idx = n->msk_ip_ver;\
	o.cl_if_id_key.ip_ver_##key_idx = n->ip_ver;\
	o.cl_if_id_key.msk_ip_vld_##key_idx = n->msk_ip_vld;\
	o.cl_if_id_key.ip_vld_##key_idx = n->ip_vld;\
	o.cl_if_id_key.msk_pppoe_session_##key_idx = n->msk_pppoe_session;\
	o.cl_if_id_key.pppoe_session_id_##key_idx = n->pppoe_session_id;\
	o.cl_if_id_key.msk_top_vid_ptag_##key_idx = n->msk_top_vid_ptag;\
	o.cl_if_id_key.top_vid_ptag_##key_idx = n->top_vid_ptag;\
	o.cl_if_id_key.msk_top_vid_##key_idx = n->msk_top_vid;\
	o.cl_if_id_key.top_vid_##key_idx = n->top_vid;\
	o.cl_if_id_key.msk_top_tpid_enc_##key_idx = n->msk_top_tpid_enc;\
	o.cl_if_id_key.top_tpid_enc_##key_idx = n->top_tpid_enc;\
	o.cl_if_id_key.msk_vlan_cnt_##key_idx = n->msk_vlan_cnt;\
	o.cl_if_id_key.vlan_cnt_##key_idx = n->vlan_cnt;\
	o.cl_if_id_key.msk_ethertype_enc_##key_idx = n->msk_ethertype_enc;\
	o.cl_if_id_key.ethertype_enc_##key_idx = n->ethertype_enc;\
	o.cl_if_id_key.msk_mac_da_ip_mc_##key_idx = n->msk_mac_da_ip_mc;\
	o.cl_if_id_key.mac_da_ip_mc_##key_idx = n->mac_da_ip_mc;\
	o.cl_if_id_key.msk_mac_da_mc_##key_idx = n->msk_mac_da_mc;\
	o.cl_if_id_key.mac_da_mc_##key_idx = n->mac_da_mc;\
	o.cl_if_id_key.msk_mac_da_bc_##key_idx = n->msk_mac_da_bc;\
	o.cl_if_id_key.mac_da_bc_##key_idx = n->mac_da_bc;\
	o.cl_if_id_key.msk_mac_da_an_hit_##key_idx = n->msk_mac_da_an_hit;\
	o.cl_if_id_key.mac_da_an_hit_##key_idx = n->mac_da_an_hit;\
	o.cl_if_id_key.msk_mac_da_an_sel_##key_idx = n->msk_mac_da_an_sel;\
	o.cl_if_id_key.mac_da_an_sel_##key_idx = n->mac_da_an_sel;\
	o.cl_if_id_key.msk_o_lspid_##key_idx = n->msk_o_lspid;\
	o.cl_if_id_key.o_lspid_##key_idx = n->o_lspid;\
	o.cl_if_id_key.msk_lspid_##key_idx = n->msk_lspid;\
	o.cl_if_id_key.lspid_##key_idx = n->lspid;\
} while(0)

#define CL_IPV4_SHORT_KEY_SET(o, key_idx, n) {\
	o.cl_ipv4_short_key.msk_l4_port_##key_idx = n->msk_l4_port;\
	o.cl_ipv4_short_key.l4_dp_##key_idx = n->l4_dp;\
	o.cl_ipv4_short_key.l4_sp_##key_idx = n->l4_sp;\
	o.cl_ipv4_short_key.msk_l4_vld_##key_idx = n->msk_l4_vld;\
	o.cl_ipv4_short_key.l4_vld_##key_idx = n->l4_vld;\
	o.cl_ipv4_short_key.msk_l3_chksum_err_##key_idx = n->msk_l3_chksum_err;\
	o.cl_ipv4_short_key.l3_chksum_err_##key_idx = n->l3_chksum_err;\
	o.cl_ipv4_short_key.msk_spi_##key_idx = n->msk_spi;\
	o.cl_ipv4_short_key.msk_ip_fragment_##key_idx = n->msk_ip_fragment;\
	o.cl_ipv4_short_key.ip_fragment_##key_idx = n->ip_fragment;\
	o.cl_ipv4_short_key.ip_addr_type_##key_idx = n->ip_addr_type;\
	o.cl_ipv4_short_key.msk_ip_addr_l_##key_idx = n->msk_ip_addr_l;\
	o.cl_ipv4_short_key.ip_addr_##key_idx = n->ip_addr.ip_addr_0;\
	o.cl_ipv4_short_key.msk_ip_da_sa_equal_##key_idx = n->msk_ip_da_sa_equal;\
	o.cl_ipv4_short_key.ip_da_sa_equal_##key_idx = n->ip_da_sa_equal;\
	o.cl_ipv4_short_key.msk_ip_da_mc_##key_idx = n->msk_ip_da_mc;\
	o.cl_ipv4_short_key.ip_da_mc_##key_idx = n->ip_da_mc;\
	o.cl_ipv4_short_key.msk_ip_protocol_##key_idx = n->msk_ip_protocol;\
	o.cl_ipv4_short_key.ip_protocol_##key_idx = n->ip_protocol;\
	o.cl_ipv4_short_key.msk_ip_ecn_##key_idx = n->msk_ip_ecn;\
	o.cl_ipv4_short_key.ip_ecn_##key_idx = n->ip_ecn;\
	o.cl_ipv4_short_key.msk_dscp_##key_idx = n->msk_dscp;\
	o.cl_ipv4_short_key.ip_dscp_##key_idx = n->ip_dscp;\
	o.cl_ipv4_short_key.msk_top_dei_##key_idx = n->msk_top_dei;\
	o.cl_ipv4_short_key.top_dei_##key_idx = n->top_dei;\
	o.cl_ipv4_short_key.msk_top_vl_802_1p_##key_idx = n->msk_top_vl_802_1p;\
	o.cl_ipv4_short_key.top_802_1p_##key_idx = n->top_802_1p;\
	o.cl_ipv4_short_key.msk_top_vid_ptag_##key_idx = n->msk_top_vid_ptag;\
	o.cl_ipv4_short_key.top_vid_ptag_##key_idx = n->top_vid_ptag;\
	o.cl_ipv4_short_key.msk_top_vid_##key_idx = n->msk_top_vid;\
	o.cl_ipv4_short_key.top_vid_##key_idx = n->top_vid;\
	o.cl_ipv4_short_key.msk_top_tpid_enc_##key_idx = n->msk_top_tpid_enc;\
	o.cl_ipv4_short_key.top_tpid_enc_##key_idx = n->top_tpid_enc;\
	o.cl_ipv4_short_key.msk_vlan_cnt_##key_idx = n->msk_vlan_cnt;\
	o.cl_ipv4_short_key.vlan_cnt_##key_idx = n->vlan_cnt;\
	o.cl_ipv4_short_key.msk_o_lspid_##key_idx = n->msk_o_lspid;\
	o.cl_ipv4_short_key.o_lspid_##key_idx = n->o_lspid;\
	o.cl_ipv4_short_key.msk_lspid_##key_idx = n->msk_lspid;\
	o.cl_ipv4_short_key.lspid_##key_idx = n->lspid;\
} while(0)

#define CL_SPCL_PKT_KEY_SET(o, key_idx, n) {\
	o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_msk_##key_idx = n->spcl_pkt_hdr_mtch_msk;\
	o.cl_spcl_pkt_key.spcl_pkt_hdr_mtch_##key_idx = n->spcl_pkt_hdr_mtch;\
	o.cl_spcl_pkt_key.spcl_pkt_enc_msk_##key_idx = n->spcl_pkt_enc_msk;\
	o.cl_spcl_pkt_key.spcl_pkt_enc_##key_idx = n->spcl_pkt_enc;\
	o.cl_spcl_pkt_key.msk_icmp_type_##key_idx = n->msk_icmp_type;\
	o.cl_spcl_pkt_key.icmp_type_##key_idx = n->icmp_type;\
	o.cl_spcl_pkt_key.msk_icmp_vld_##key_idx = n->msk_icmp_vld;\
	o.cl_spcl_pkt_key.icmp_vld_##key_idx = n->icmp_vld;\
	o.cl_spcl_pkt_key.msk_ipv6_doh_##key_idx = n->msk_ipv6_doh;\
	o.cl_spcl_pkt_key.ipv6_doh_##key_idx = n->ipv6_doh;\
	o.cl_spcl_pkt_key.msk_ipv6_rh_##key_idx = n->msk_ipv6_rh;\
	o.cl_spcl_pkt_key.ipv6_rh_##key_idx = n->ipv6_rh;\
	o.cl_spcl_pkt_key.msk_ipv6_hbh_##key_idx = n->msk_ipv6_hbh;\
	o.cl_spcl_pkt_key.ipv6_hbh_##key_idx = n->ipv6_hbh;\
	o.cl_spcl_pkt_key.msk_ipv6_ndp_##key_idx = n->msk_ipv6_ndp;\
	o.cl_spcl_pkt_key.ipv6_ndp_##key_idx = n->ipv6_ndp;\
	o.cl_spcl_pkt_key.msk_ip_fragment_##key_idx = n->msk_ip_fragment;\
	o.cl_spcl_pkt_key.ip_fragment_##key_idx = n->ip_fragment;\
	o.cl_spcl_pkt_key.msk_ip_options_##key_idx = n->msk_ip_options;\
	o.cl_spcl_pkt_key.ip_options_##key_idx = n->ip_options;\
	o.cl_spcl_pkt_key.msk_ip_protocol_##key_idx = n->msk_ip_protocol;\
	o.cl_spcl_pkt_key.ip_protocol_##key_idx = n->ip_protocol;\
	o.cl_spcl_pkt_key.msk_top_dei_##key_idx = n->msk_top_dei;\
	o.cl_spcl_pkt_key.top_dei_##key_idx = n->top_dei;\
	o.cl_spcl_pkt_key.msk_top_vl_802_1p_##key_idx = n->msk_top_vl_802_1p;\
	o.cl_spcl_pkt_key.top_802_1p_##key_idx = n->top_802_1p;\
	o.cl_spcl_pkt_key.msk_top_vid_ptag_##key_idx = n->msk_top_vid_ptag;\
	o.cl_spcl_pkt_key.top_vid_ptag_##key_idx = n->top_vid_ptag;\
	o.cl_spcl_pkt_key.msk_top_vid_##key_idx = n->msk_top_vid;\
	o.cl_spcl_pkt_key.top_vid_##key_idx = n->top_vid;\
	o.cl_spcl_pkt_key.msk_top_tpid_enc_##key_idx = n->msk_top_tpid_enc;\
	o.cl_spcl_pkt_key.top_tpid_enc_##key_idx = n->top_tpid_enc;\
	o.cl_spcl_pkt_key.msk_vlan_cnt_##key_idx = n->msk_vlan_cnt;\
	o.cl_spcl_pkt_key.vlan_cnt_##key_idx = n->vlan_cnt;\
	o.cl_spcl_pkt_key.msk_ethertype_enc_##key_idx = n->msk_ethertype_enc;\
	o.cl_spcl_pkt_key.ethertype_enc_##key_idx = n->ethertype_enc;\
	o.cl_spcl_pkt_key.msk_mac_da_rsvd_##key_idx = n->msk_mac_da_rsvd;\
	o.cl_spcl_pkt_key.mac_da_rsvd_##key_idx = n->mac_da_rsvd;\
	o.cl_spcl_pkt_key.msk_mac_da_ip_mc_##key_idx = n->msk_mac_da_ip_mc;\
	o.cl_spcl_pkt_key.mac_da_ip_mc_##key_idx = n->mac_da_ip_mc;\
	o.cl_spcl_pkt_key.msk_mac_da_mc_##key_idx = n->msk_mac_da_mc;\
	o.cl_spcl_pkt_key.mac_da_mc_##key_idx = n->mac_da_mc;\
	o.cl_spcl_pkt_key.msk_mac_da_bc_##key_idx = n->msk_mac_da_bc;\
	o.cl_spcl_pkt_key.mac_da_bc_##key_idx = n->mac_da_bc;\
	o.cl_spcl_pkt_key.msk_mac_da_an_hit_##key_idx = n->msk_mac_da_an_hit;\
	o.cl_spcl_pkt_key.mac_da_an_hit_##key_idx = n->mac_da_an_hit;\
	o.cl_spcl_pkt_key.msk_mac_da_an_sel_##key_idx = n->msk_mac_da_an_sel;\
	o.cl_spcl_pkt_key.mac_da_an_sel_##key_idx = n->mac_da_an_sel;\
	o.cl_spcl_pkt_key.msk_mac_da_##key_idx = n->msk_mac_da;\
	o.cl_spcl_pkt_key.mac_da0_##key_idx = n->mac_da[0];\
	o.cl_spcl_pkt_key.mac_da1_##key_idx = n->mac_da[1];\
	o.cl_spcl_pkt_key.mac_da2_##key_idx = n->mac_da[2];\
	o.cl_spcl_pkt_key.mac_da3_##key_idx = n->mac_da[3];\
	o.cl_spcl_pkt_key.mac_da4_##key_idx = n->mac_da[4];\
	o.cl_spcl_pkt_key.mac_da5_##key_idx = n->mac_da[5];\
	o.cl_spcl_pkt_key.msk_o_lspid_##key_idx = n->msk_o_lspid;\
	o.cl_spcl_pkt_key.o_lspid_##key_idx = n->o_lspid;\
	o.cl_spcl_pkt_key.msk_lspid_##key_idx = n->msk_lspid;\
	o.cl_spcl_pkt_key.lspid_##key_idx = n->lspid;\
} while(0)

static ca_int32_t __l3_cls_key_entry_update(
	l3_cls_rule_t *rule,
	l3_cls_key_type_t key_type,
	ca_uint32_t key_tbl_idx,
	ca_uint8_t key_idx,
	l3_cls_key_t *data)
{
	l3_cls_key_t *ptr = (l3_cls_key_t *)data;
	cls_key_t *key = &rule->key;

	if (key_type > CL_FULL_KEY || key_type < CL_IF_ID_KEY)
		return AAL_E_OUTRANGE;
	if (rule == NULL || data == NULL)
		return AAL_E_NULLPTR;

	switch(key_type) {
		case CL_IF_ID_KEY:
			if (key_idx >= CL_4_KEY_NUM)
				return AAL_E_OUTRANGE;

			if (key_idx == 0) {
				CL_IF_ID_KEY_SET(ptr->cls_key_info, 0, key);
			} else if (key_idx == 1) {
				CL_IF_ID_KEY_SET(ptr->cls_key_info, 1, key);
			} else if (key_idx == 2) {
				CL_IF_ID_KEY_SET(ptr->cls_key_info, 2, key);
			} else if (key_idx == 3) {
				CL_IF_ID_KEY_SET(ptr->cls_key_info, 3, key);
			}
			break;
		case CL_IPV4_TUNNEL_ID_KEY:
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_l4_port = key->msk_l4_port;
			ptr->cls_key_info.cl_ipv4_tunnel_key.l4_dp = key->l4_dp;
			ptr->cls_key_info.cl_ipv4_tunnel_key.l4_sp = key->l4_sp;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_l4_vld = key->msk_l4_vld;
			ptr->cls_key_info.cl_ipv4_tunnel_key.l4_vld = key->l4_vld;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_spi = key->msk_spi;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_fragment = key->msk_ip_fragment;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_fragment = key->ip_fragment;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_sa_equal = key->msk_ip_da_sa_equal;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_da_sa_equal = key->ip_da_sa_equal;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_mc = key->msk_ip_da_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_da_mc = key->ip_da_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_enc = key->msk_ip_da_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_da_enc = key->ip_da_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_l = key->msk_ip_da_l;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_da = key->ip_da.ip_addr_0;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_enc = key->msk_ip_sa_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_sa_enc = key->ip_sa_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_l = key->msk_ip_sa_l;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_sa = key->ip_sa.ip_addr_0;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_protocol = key->msk_ip_protocol;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_protocol = key->ip_protocol;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_ver = key->msk_ip_ver;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_ver = key->ip_ver;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ip_vld = key->msk_ip_vld;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ip_vld = key->ip_vld;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_pppoe_session = key->msk_pppoe_session;
			ptr->cls_key_info.cl_ipv4_tunnel_key.pppoe_session_id = key->pppoe_session_id;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_inner_vid = key->msk_inner_vid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.inner_vid = key->inner_vid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_inner_tpid_enc = key->msk_inner_tpid_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.inner_tpid_enc = key->inner_tpid_enc;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid_ptag = key->msk_top_vid_ptag;
			ptr->cls_key_info.cl_ipv4_tunnel_key.top_vid_ptag = key->top_vid_ptag;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_top_vid = key->msk_top_vid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.top_vid = key->top_vid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_top_tpid_enc = key->msk_top_tpid_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.top_tpid_enc = key->top_tpid_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_vlan_cnt = key->msk_vlan_cnt;
			ptr->cls_key_info.cl_ipv4_tunnel_key.vlan_cnt = key->vlan_cnt;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_ethertype_enc = key->msk_ethertype_enc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.ethertype_enc = key->ethertype_enc;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_ip_mc = key->msk_mac_da_ip_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.mac_da_ip_mc = key->mac_da_ip_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_mc = key->msk_mac_da_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.mac_da_mc = key->mac_da_mc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_bc = key->msk_mac_da_bc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.mac_da_bc = key->mac_da_bc;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_hit = key->msk_mac_da_an_hit;
			ptr->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_hit = key->mac_da_an_hit;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_mac_da_an_sel = key->msk_mac_da_an_sel;
			ptr->cls_key_info.cl_ipv4_tunnel_key.mac_da_an_sel = key->mac_da_an_sel;

			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_o_lspid = key->msk_o_lspid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.o_lspid = key->o_lspid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.msk_lspid = key->msk_lspid;
			ptr->cls_key_info.cl_ipv4_tunnel_key.lspid = key->lspid;
			break;
		case CL_IPV4_SHORT_KEY:
			if (key_idx >= CL_2_KEY_NUM)
				return AAL_E_OUTRANGE;

			if (key_idx == 0) {
				CL_IPV4_SHORT_KEY_SET(ptr->cls_key_info, 0, key);
			} else if (key_idx == 1) {
				CL_IPV4_SHORT_KEY_SET(ptr->cls_key_info, 1, key);
			}
			break;
		case CL_IPV6_SHORT_KEY:
			ptr->cls_key_info.cl_ipv6_short_key.msk_l4_port = key->msk_l4_port;
			ptr->cls_key_info.cl_ipv6_short_key.l4_dp = key->l4_dp;
			ptr->cls_key_info.cl_ipv6_short_key.l4_sp = key->l4_sp;
			ptr->cls_key_info.cl_ipv6_short_key.msk_l4_vld = key->msk_l4_vld;
			ptr->cls_key_info.cl_ipv6_short_key.l4_vld = key->l4_vld;

			ptr->cls_key_info.cl_ipv6_short_key.msk_spi = key->msk_spi;

			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_fragment = key->msk_ip_fragment;
			ptr->cls_key_info.cl_ipv6_short_key.ip_fragment = key->ip_fragment;

			ptr->cls_key_info.cl_ipv6_short_key.msk_ipv6_flow_lbl = key->msk_ipv6_flow_lbl;
			ptr->cls_key_info.cl_ipv6_short_key.ipv6_flow_lbl = key->ipv6_flow_lbl;
			ptr->cls_key_info.cl_ipv6_short_key.ip_addr_type = key->ip_addr_type;
			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_addr_l = key->msk_ip_addr_l;
			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_addr_h = key->msk_ip_addr_h;
			ptr->cls_key_info.cl_ipv6_short_key.ip_addr_0 = key->ip_addr.ip_addr_0;
			ptr->cls_key_info.cl_ipv6_short_key.ip_addr_1 = key->ip_addr.ip_addr_1;
			ptr->cls_key_info.cl_ipv6_short_key.ip_addr_2 = key->ip_addr.ip_addr_2;
			ptr->cls_key_info.cl_ipv6_short_key.ip_addr_3 = key->ip_addr.ip_addr_3;

			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_da_sa_equal = key->msk_ip_da_sa_equal;
			ptr->cls_key_info.cl_ipv6_short_key.ip_da_sa_equal = key->ip_da_sa_equal;
			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_da_mc = key->msk_ip_da_mc;
			ptr->cls_key_info.cl_ipv6_short_key.ip_da_mc = key->ip_da_mc;

			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_protocol = key->msk_ip_protocol;
			ptr->cls_key_info.cl_ipv6_short_key.ip_protocol = key->ip_protocol;
			ptr->cls_key_info.cl_ipv6_short_key.msk_ip_ecn = key->msk_ip_ecn;
			ptr->cls_key_info.cl_ipv6_short_key.ip_ecn = key->ip_ecn ;
			ptr->cls_key_info.cl_ipv6_short_key.msk_dscp = key->msk_dscp;
			ptr->cls_key_info.cl_ipv6_short_key.ip_dscp = key->ip_dscp;

			ptr->cls_key_info.cl_ipv6_short_key.msk_top_dei = key->msk_top_dei;
			ptr->cls_key_info.cl_ipv6_short_key.top_dei = key->top_dei;
			ptr->cls_key_info.cl_ipv6_short_key.msk_top_vl_802_1p = key->msk_top_vl_802_1p;
			ptr->cls_key_info.cl_ipv6_short_key.top_802_1p = key->top_802_1p;
			ptr->cls_key_info.cl_ipv6_short_key.msk_top_vid_ptag = key->msk_top_vid_ptag;
			ptr->cls_key_info.cl_ipv6_short_key.top_vid_ptag = key->top_vid_ptag;

			ptr->cls_key_info.cl_ipv6_short_key.msk_top_vid = key->msk_top_vid;
			ptr->cls_key_info.cl_ipv6_short_key.top_vid = key->top_vid;
			ptr->cls_key_info.cl_ipv6_short_key.msk_top_tpid_enc = key->msk_top_tpid_enc;
			ptr->cls_key_info.cl_ipv6_short_key.top_tpid_enc = key->top_tpid_enc;
			ptr->cls_key_info.cl_ipv6_short_key.msk_vlan_cnt = key->msk_vlan_cnt;
			ptr->cls_key_info.cl_ipv6_short_key.vlan_cnt = key->vlan_cnt;

			ptr->cls_key_info.cl_ipv6_short_key.msk_o_lspid = key->msk_o_lspid;
			ptr->cls_key_info.cl_ipv6_short_key.o_lspid = key->o_lspid;
			ptr->cls_key_info.cl_ipv6_short_key.msk_lspid = key->msk_lspid;
			ptr->cls_key_info.cl_ipv6_short_key.lspid = key->lspid;
			break;
		case CL_SPCL_PKT_KEY:
			if (key_idx >= CL_2_KEY_NUM)
				return AAL_E_OUTRANGE;

			if (key_idx == 0) {
				CL_SPCL_PKT_KEY_SET(ptr->cls_key_info, 0, key);
			} else if (key_idx == 1) {
				CL_SPCL_PKT_KEY_SET(ptr->cls_key_info, 1, key);
			}
			break;
		case CL_MCST_KEY:
			ptr->cls_key_info.cl_mcst_key.msk_ip_da_sa_equal = key->msk_ip_da_sa_equal;
			ptr->cls_key_info.cl_mcst_key.ip_da_sa_equal = key->ip_da_sa_equal;
			ptr->cls_key_info.cl_mcst_key.msk_ip_da_mc = key->msk_ip_da_mc;
			ptr->cls_key_info.cl_mcst_key.ip_da_mc = key->ip_da_mc;

			ptr->cls_key_info.cl_mcst_key.msk_ip_ver = key->msk_ip_ver;
			ptr->cls_key_info.cl_mcst_key.ip_ver = key->ip_ver;
			ptr->cls_key_info.cl_mcst_key.msk_ip_vld = key->msk_ip_vld;
			ptr->cls_key_info.cl_mcst_key.ip_vld = key->ip_vld;

			ptr->cls_key_info.cl_mcst_key.msk_pppoe_session = key->msk_pppoe_session;
			ptr->cls_key_info.cl_mcst_key.pppoe_session_id = key->pppoe_session_id;

			ptr->cls_key_info.cl_mcst_key.msk_inner_vid = key->msk_inner_vid;
			ptr->cls_key_info.cl_mcst_key.inner_vid = key->inner_vid;
			ptr->cls_key_info.cl_mcst_key.msk_inner_tpid_enc = key->msk_inner_tpid_enc;
			ptr->cls_key_info.cl_mcst_key.inner_tpid_enc = key->inner_tpid_enc;
			ptr->cls_key_info.cl_mcst_key.msk_top_dei = key->msk_top_dei;
			ptr->cls_key_info.cl_mcst_key.top_dei = key->top_dei;
			ptr->cls_key_info.cl_mcst_key.msk_top_vl_802_1p = key->msk_top_vl_802_1p;
			ptr->cls_key_info.cl_mcst_key.top_802_1p = key->top_802_1p;
			ptr->cls_key_info.cl_mcst_key.msk_top_vid_ptag = key->msk_top_vid_ptag;
			ptr->cls_key_info.cl_mcst_key.top_vid_ptag = key->top_vid_ptag;

			ptr->cls_key_info.cl_mcst_key.msk_top_vid = key->msk_top_vid;
			ptr->cls_key_info.cl_mcst_key.top_vid = key->top_vid;
			ptr->cls_key_info.cl_mcst_key.msk_top_tpid_enc = key->msk_top_tpid_enc;
			ptr->cls_key_info.cl_mcst_key.top_tpid_enc = key->top_tpid_enc;
			ptr->cls_key_info.cl_mcst_key.msk_vlan_cnt = key->msk_vlan_cnt;
			ptr->cls_key_info.cl_mcst_key.vlan_cnt = key->vlan_cnt;

			ptr->cls_key_info.cl_mcst_key.msk_mac_da_ip_mc = key->msk_mac_da_ip_mc;
			ptr->cls_key_info.cl_mcst_key.mac_da_ip_mc = key->mac_da_ip_mc;
			ptr->cls_key_info.cl_mcst_key.msk_mac_da_mc = key->msk_mac_da_mc;
			ptr->cls_key_info.cl_mcst_key.mac_da_mc = key->mac_da_mc;
			ptr->cls_key_info.cl_mcst_key.msk_mac_da_bc = key->msk_mac_da_bc;
			ptr->cls_key_info.cl_mcst_key.mac_da_bc = key->mac_da_bc;

			ptr->cls_key_info.cl_mcst_key.msk_dest_addr = key->msk_dest_addr;
			ptr->cls_key_info.cl_mcst_key.dest_addr_type = key->dest_addr_type;
			ptr->cls_key_info.cl_mcst_key.dest_addr_0 = key->dest_addr.dest_ip_addr.ip_addr_0;
			ptr->cls_key_info.cl_mcst_key.dest_addr_1 = key->dest_addr.dest_ip_addr.ip_addr_1;
			ptr->cls_key_info.cl_mcst_key.dest_addr_2 = key->dest_addr.dest_ip_addr.ip_addr_2;
			ptr->cls_key_info.cl_mcst_key.dest_addr_3 = key->dest_addr.dest_ip_addr.ip_addr_3;

			ptr->cls_key_info.cl_mcst_key.msk_mcgid = key->msk_mcgid;
			ptr->cls_key_info.cl_mcst_key.mcgid = key->mcgid;
			ptr->cls_key_info.cl_mcst_key.msk_o_lspid = key->msk_o_lspid;
			ptr->cls_key_info.cl_mcst_key.o_lspid = key->o_lspid;
			ptr->cls_key_info.cl_mcst_key.msk_lspid = key->msk_lspid;
			ptr->cls_key_info.cl_mcst_key.lspid = key->lspid;
			break;
		case CL_FULL_KEY:
			ptr->cls_key_info.cl_full_key.msk_tcp_rdp_ctrl = key->msk_tcp_rdp_ctrl;
			ptr->cls_key_info.cl_full_key.tcp_rdp_ctrl = key->tcp_rdp_ctrl;
			ptr->cls_key_info.cl_full_key.msk_l4_port = key->msk_l4_port;
			ptr->cls_key_info.cl_full_key.l4_dp = key->l4_dp;
			ptr->cls_key_info.cl_full_key.l4_sp = key->l4_sp;
			ptr->cls_key_info.cl_full_key.msk_l4_vld = key->msk_l4_vld;
			ptr->cls_key_info.cl_full_key.l4_vld = key->l4_vld;
			ptr->cls_key_info.cl_full_key.msk_l3_chksum_err = key->msk_l3_chksum_err;
			ptr->cls_key_info.cl_full_key.l3_chksum_err = key->l3_chksum_err;
			ptr->cls_key_info.cl_full_key.msk_spi = key->msk_spi;

			ptr->cls_key_info.cl_full_key.msk_ip_fragment = key->msk_ip_fragment;
			ptr->cls_key_info.cl_full_key.ip_fragment = key->ip_fragment;
			ptr->cls_key_info.cl_full_key.msk_ip_options = key->msk_ip_options;
			ptr->cls_key_info.cl_full_key.ip_options = key->ip_options;

			ptr->cls_key_info.cl_full_key.msk_ip_da_sa_equal = key->msk_ip_da_sa_equal;
			ptr->cls_key_info.cl_full_key.ip_da_sa_equal = key->ip_da_sa_equal;
			ptr->cls_key_info.cl_full_key.msk_ip_da_mc = key->msk_ip_da_mc;
			ptr->cls_key_info.cl_full_key.ip_da_mc = key->ip_da_mc;

			ptr->cls_key_info.cl_full_key.msk_ip_da_l = key->msk_ip_da_l;
			ptr->cls_key_info.cl_full_key.msk_ip_da_h = key->msk_ip_da_h;
			ptr->cls_key_info.cl_full_key.ip_da_0 = key->ip_da.ip_addr_0;
			ptr->cls_key_info.cl_full_key.ip_da_1 = key->ip_da.ip_addr_1;
			ptr->cls_key_info.cl_full_key.ip_da_2 = key->ip_da.ip_addr_2;
			ptr->cls_key_info.cl_full_key.ip_da_3 = key->ip_da.ip_addr_3;

			ptr->cls_key_info.cl_full_key.msk_ip_sa_l = key->msk_ip_sa_l;
			ptr->cls_key_info.cl_full_key.msk_ip_sa_h = key->msk_ip_sa_h;
			ptr->cls_key_info.cl_full_key.ip_sa_0 = key->ip_sa.ip_addr_0;
			ptr->cls_key_info.cl_full_key.ip_sa_1 = key->ip_sa.ip_addr_1;
			ptr->cls_key_info.cl_full_key.ip_sa_2 = key->ip_sa.ip_addr_2;
			ptr->cls_key_info.cl_full_key.ip_sa_3 = key->ip_sa.ip_addr_3;

			ptr->cls_key_info.cl_full_key.msk_ip_protocol = key->msk_ip_protocol;
			ptr->cls_key_info.cl_full_key.ip_protocol = key->ip_protocol;
			ptr->cls_key_info.cl_full_key.msk_ip_ecn = key->msk_ip_ecn;
			ptr->cls_key_info.cl_full_key.ip_ecn = key->ip_ecn;
			ptr->cls_key_info.cl_full_key.msk_dscp = key->msk_dscp;
			ptr->cls_key_info.cl_full_key.ip_dscp = key->ip_dscp;
			ptr->cls_key_info.cl_full_key.msk_ip_ver = key->msk_ip_ver;
			ptr->cls_key_info.cl_full_key.ip_ver = key->ip_ver;
			ptr->cls_key_info.cl_full_key.msk_ip_vld = key->msk_ip_vld;
			ptr->cls_key_info.cl_full_key.ip_vld = key->ip_vld;
			ptr->cls_key_info.cl_full_key.msk_ppp_protocol = key->msk_ppp_protocol;
			ptr->cls_key_info.cl_full_key.ppp_protocol_enc = key->ppp_protocol_enc;
			ptr->cls_key_info.cl_full_key.msk_pppoe_session = key->msk_pppoe_session;
			ptr->cls_key_info.cl_full_key.pppoe_session_id = key->pppoe_session_id;
			ptr->cls_key_info.cl_full_key.msk_inner_dei = key->msk_inner_dei;
			ptr->cls_key_info.cl_full_key.inner_dei = key->inner_dei;
			ptr->cls_key_info.cl_full_key.msk_inner_802_1p = key->msk_inner_802_1p;
			ptr->cls_key_info.cl_full_key.inner_802_1p = key->inner_802_1p;
			ptr->cls_key_info.cl_full_key.msk_inner_vid = key->msk_inner_vid;
			ptr->cls_key_info.cl_full_key.inner_vid = key->inner_vid;
			ptr->cls_key_info.cl_full_key.msk_inner_tpid_enc = key->msk_inner_tpid_enc;
			ptr->cls_key_info.cl_full_key.inner_tpid_enc = key->inner_tpid_enc;
			ptr->cls_key_info.cl_full_key.msk_top_dei = key->msk_top_dei;
			ptr->cls_key_info.cl_full_key.top_dei = key->top_dei;
			ptr->cls_key_info.cl_full_key.msk_top_vl_802_1p = key->msk_top_vl_802_1p;
			ptr->cls_key_info.cl_full_key.top_802_1p = key->top_802_1p;
			ptr->cls_key_info.cl_full_key.msk_top_vid_ptag = key->msk_top_vid_ptag;
			ptr->cls_key_info.cl_full_key.top_vid_ptag = key->top_vid_ptag;
			ptr->cls_key_info.cl_full_key.vid_ctl = key->vid_ctl;

			ptr->cls_key_info.cl_full_key.top_vid = key->top_vid;
			ptr->cls_key_info.cl_full_key.msk_top_tpid_enc = key->msk_top_tpid_enc;
			ptr->cls_key_info.cl_full_key.top_tpid_enc = key->top_tpid_enc;
			ptr->cls_key_info.cl_full_key.msk_vlan_cnt = key->msk_vlan_cnt;
			ptr->cls_key_info.cl_full_key.vlan_cnt = key->vlan_cnt;
			ptr->cls_key_info.cl_full_key.llc_type_enc_mask = key->llc_type_enc_mask;
			ptr->cls_key_info.cl_full_key.llc_type_enc = key->llc_type_enc;
			ptr->cls_key_info.cl_full_key.pktlen_rng_match_mask = key->pktlen_rng_match_mask;
			ptr->cls_key_info.cl_full_key.pktlen_rng_match_vec = key->pktlen_rng_match_vec;
			ptr->cls_key_info.cl_full_key.msk_len_encoded = key->msk_len_encoded;
			ptr->cls_key_info.cl_full_key.len_encoded = key->len_encoded;
			ptr->cls_key_info.cl_full_key.msk_ethertype_enc = key->msk_ethertype_enc;
			ptr->cls_key_info.cl_full_key.ethertype_enc = key->ethertype_enc;
			ptr->cls_key_info.cl_full_key.msk_mac_sa = key->msk_mac_sa;
			ptr->cls_key_info.cl_full_key.mac_sa0 = key->mac_sa[0];
			ptr->cls_key_info.cl_full_key.mac_sa1 = key->mac_sa[1];
			ptr->cls_key_info.cl_full_key.mac_sa2 = key->mac_sa[2];
			ptr->cls_key_info.cl_full_key.mac_sa3 = key->mac_sa[3];
			ptr->cls_key_info.cl_full_key.mac_sa4 = key->mac_sa[4];
			ptr->cls_key_info.cl_full_key.mac_sa5 = key->mac_sa[5];

			ptr->cls_key_info.cl_full_key.msk_mac_da_rsvd = key->msk_mac_da_rsvd;
			ptr->cls_key_info.cl_full_key.mac_da_rsvd = key->mac_da_rsvd;
			ptr->cls_key_info.cl_full_key.msk_mac_da_ip_mc = key->msk_mac_da_ip_mc;
			ptr->cls_key_info.cl_full_key.mac_da_ip_mc = key->mac_da_ip_mc;
			ptr->cls_key_info.cl_full_key.msk_mac_da_mc = key->msk_mac_da_mc;
			ptr->cls_key_info.cl_full_key.mac_da_mc = key->mac_da_mc;
			ptr->cls_key_info.cl_full_key.msk_mac_da_bc = key->msk_mac_da_bc;
			ptr->cls_key_info.cl_full_key.mac_da_bc = key->mac_da_bc;
			ptr->cls_key_info.cl_full_key.msk_mac_da_an_hit = key->msk_mac_da_an_hit;
			ptr->cls_key_info.cl_full_key.mac_da_an_hit = key->mac_da_an_hit;
			ptr->cls_key_info.cl_full_key.msk_mac_da_an_sel = key->msk_mac_da_an_sel;
			ptr->cls_key_info.cl_full_key.mac_da_an_sel = key->mac_da_an_sel;

			ptr->cls_key_info.cl_full_key.msk_mdata = key->msk_mdata;
			ptr->cls_key_info.cl_full_key.mdata_0 = key->mdata.mdata_0;
			ptr->cls_key_info.cl_full_key.mdata_1 = key->mdata.mdata_1;
			ptr->cls_key_info.cl_full_key.msk_mcgid = key->msk_mcgid;
			ptr->cls_key_info.cl_full_key.mcgid = key->mcgid;
			ptr->cls_key_info.cl_full_key.msk_o_lspid = key->msk_o_lspid;
			ptr->cls_key_info.cl_full_key.o_lspid = key->o_lspid;
			ptr->cls_key_info.cl_full_key.msk_lspid = key->msk_lspid;
			ptr->cls_key_info.cl_full_key.lspid = key->lspid;
			break;
		default:
			return AAL_E_INVALID;
	}

	/* update key mode */
	ptr->cls_key_info.key_tbl_data[0].cls_entry_mode = 0;
	/* update priority */
	ptr->cls_key_info.key_tbl_data[0].cls_pri = rule->pri;
	/* update rsult type */
	ptr->cls_key_info.key_tbl_data[0].cls_rslt_type = rule->rslt_type;
	/* update key type */
	ptr->cls_key_info.key_tbl_data[0].cls_key_type = key_type;
	/* update key valid */
	CLS_KEY_VLD_SET(key_idx, ptr->cls_key_info.key_tbl_data[0].cls_key_valid);

	return AAL_E_OK;
}

static ca_int32_t __l3_cls_fib_entry_update(
	cls_action_t *action,
	l3_cls_fib_mode_t fib_mode,
	cls_fib_t *data)
{
	cls_fib_t *ptr = (cls_fib_t *)data;

	if (action == NULL || data == NULL)
		return CA_E_ERROR;
	if ((fib_mode > CL_FIB_MOD_4) || (fib_mode < CL_FIB_MOD_0))
		return CA_E_PARAM;

	switch(fib_mode) {
		case CL_FIB_MOD_0:
			/* group 19 */
			ptr->cls_fib_data.fib_mod_0.chk_msk_ptr = action->chk_msk_ptr;
			ptr->cls_fib_data.fib_mod_0.cache_ctrl = action->cache_ctrl;
			ptr->cls_fib_data.fib_mod_0.pop_l3_vld = action->pop_l3_vld;
			ptr->cls_fib_data.fib_mod_0.pop_l3_chk_ecn_en = action->pop_l3_chk_ecn_en;
			ptr->cls_fib_data.fib_mod_0.pop_l3_en = action->pop_l3_en;
			ptr->cls_fib_data.fib_mod_0.keep_ts_vld = action->keep_ts_vld;
			ptr->cls_fib_data.fib_mod_0.keep_ts_en = action->keep_ts_en;
			ptr->cls_fib_data.fib_mod_0.keep_orig_pkt_vld = action->keep_orig_pkt_vld;
			ptr->cls_fib_data.fib_mod_0.keep_orig_pkt = action->keep_orig_pkt;
			ptr->cls_fib_data.fib_mod_0.stage3_ctrl_vld = action->stage3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.stage3_ctrl = action->stage3_ctrl;
			ptr->cls_fib_data.fib_mod_0.stage2_ctrl_vld = action->stage2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.stage2_ctrl = action->stage2_ctrl;
			ptr->cls_fib_data.fib_mod_0.t5_ctrl_vld = action->t5_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.t5_ctrl = action->t5_ctrl;
			ptr->cls_fib_data.fib_mod_0.t4_ctrl_vld = action->t4_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.t4_ctrl = action->t4_ctrl;
			ptr->cls_fib_data.fib_mod_0.t3_ctrl_vld = action->t3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.t3_ctrl = action->t3_ctrl;
			ptr->cls_fib_data.fib_mod_0.t2_ctrl_vld = action->t2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.t2_ctrl = action->t2_ctrl;
			/* group 09 */
			ptr->cls_fib_data.fib_mod_0.vlan_vld = action->vlan_vld;
			ptr->cls_fib_data.fib_mod_0.inner_dei_sel = action->inner_dei_sel;
			ptr->cls_fib_data.fib_mod_0.inner_dei = action->inner_dei;
			ptr->cls_fib_data.fib_mod_0.inner_vid = action->inner_vid;
			ptr->cls_fib_data.fib_mod_0.inner_tpid_enc = action->inner_tpid_enc;
			ptr->cls_fib_data.fib_mod_0.top_dei_sel = action->top_dei_sel;
			ptr->cls_fib_data.fib_mod_0.top_dei = action->top_dei;
			ptr->cls_fib_data.fib_mod_0.top_vid = action->top_vid;
			ptr->cls_fib_data.fib_mod_0.top_tpid_enc = action->top_tpid_enc;
			ptr->cls_fib_data.fib_mod_0.vlan_cnt = action->vlan_cnt;
			/* group 18 */
			ptr->cls_fib_data.fib_mod_0.mrr_vld = action->mrr_vld;
			ptr->cls_fib_data.fib_mod_0.mrr_en = action->mrr_en;
			ptr->cls_fib_data.fib_mod_0.no_drop_vld = action->no_drop_vld;
			ptr->cls_fib_data.fib_mod_0.no_drop = action->no_drop;
			ptr->cls_fib_data.fib_mod_0.dpid_vld = action->dpid_vld;
			ptr->cls_fib_data.fib_mod_0.dpid_pri = action->dpid_pri;
			ptr->cls_fib_data.fib_mod_0.permit = action->permit;
			ptr->cls_fib_data.fib_mod_0.deepq = action->deepq;
			ptr->cls_fib_data.fib_mod_0.mcgid = action->mcgid;
			ptr->cls_fib_data.fib_mod_0.mc = action->mc;
			/* group 17 */
			ptr->cls_fib_data.fib_mod_0.ip_ecn_vld = action->ip_ecn_vld;
			ptr->cls_fib_data.fib_mod_0.ip_ecn_en = action->ip_ecn_en;
			ptr->cls_fib_data.fib_mod_0.ip_tos_vld = action->ip_tos_vld;
			ptr->cls_fib_data.fib_mod_0.ip_tos_6 = action->ip_tos_6;
			ptr->cls_fib_data.fib_mod_0.ip_dscp_marked_vld = action->ip_dscp_marked_vld;
			ptr->cls_fib_data.fib_mod_0.ip_dscp_marked_down = action->ip_dscp_marked_down;
			ptr->cls_fib_data.fib_mod_0.ip_dscp_markdown_en = action->ip_dscp_markdown_en;
			ptr->cls_fib_data.fib_mod_0.ip_dscp_update_en = action->ip_dscp_update_en;
			ptr->cls_fib_data.fib_mod_0.ip_dscp_sel = action->ip_dscp_sel;
			ptr->cls_fib_data.fib_mod_0.ip_dscp = action->ip_dscp;
			ptr->cls_fib_data.fib_mod_0.dscp_table_sel = action->dscp_table_sel;
			ptr->cls_fib_data.fib_mod_0.inner_802_1p_sel = action->inner_802_1p_sel;
			ptr->cls_fib_data.fib_mod_0.inner_802_1p = action->inner_802_1p;
			ptr->cls_fib_data.fib_mod_0.top_802_1p_sel = action->top_802_1p_sel;
			ptr->cls_fib_data.fib_mod_0.top_802_1p = action->top_802_1p;
			ptr->cls_fib_data.fib_mod_0.qos_802_1p_table_sel = action->qos_802_1p_table_sel;
			ptr->cls_fib_data.fib_mod_0.cos_table_sel = action->cos_table_sel;
			ptr->cls_fib_data.fib_mod_0.cos_sel = action->cos_sel;
			ptr->cls_fib_data.fib_mod_0.cos = action->cos;
			/* group 16 */
			ptr->cls_fib_data.fib_mod_0.pol_vld = action->pol_vld;
			ptr->cls_fib_data.fib_mod_0.qos_premark = action->qos_premark;
			ptr->cls_fib_data.fib_mod_0.pol_all_bypass = action->pol_all_bypass;
			ptr->cls_fib_data.fib_mod_0.pol_en	= action->pol_en;
			ptr->cls_fib_data.fib_mod_0.pol_base = action->pol_base;
			ptr->cls_fib_data.fib_mod_0.pol_sel = action->pol_sel;
			ptr->cls_fib_data.fib_mod_0.pol_table_sel = action->pol_table_sel;
			ptr->cls_fib_data.fib_mod_0.pol_grp_vld = action->pol_grp_vld;
			ptr->cls_fib_data.fib_mod_0.pol_grp_id = action->pol_grp_id;
			/* group 08 */
			ptr->cls_fib_data.fib_mod_0.ip_ttl_vld = action->ip_ttl_vld;
			ptr->cls_fib_data.fib_mod_0.ip_ttl_zero_discard_en = action->ip_ttl_zero_discard_en;
			ptr->cls_fib_data.fib_mod_0.ip_ttl_cmd = action->ip_ttl_cmd;
			ptr->cls_fib_data.fib_mod_0.ip_ttl = action->ip_ttl;
			ptr->cls_fib_data.fib_mod_0.six_rd_ipda_ctrl_vld = action->six_rd_ipda_ctrl_vld;
			ptr->cls_fib_data.fib_mod_0.six_rd_ipda_from_v6 = action->six_rd_ipda_from_v6;
			ptr->cls_fib_data.fib_mod_0.ip_mtu_enc_vld = action->ip_mtu_enc_vld;
			ptr->cls_fib_data.fib_mod_0.ip_mtu_enc = action->ip_mtu_enc;
			ptr->cls_fib_data.fib_mod_0.l2_format_vld = action->l2_format_vld;
			ptr->cls_fib_data.fib_mod_0.l2_format = action->l2_format;
			ptr->cls_fib_data.fib_mod_0.l3_egress_if_vld = action->l3_egress_if_vld;
			/* group 13 */
			ptr->cls_fib_data.fib_mod_0.mdata_w_vld_1 = action->mdata_w_vld_1;
			ptr->cls_fib_data.fib_mod_0.mdata_w_1 = action->mdata_w_1;
			break;
		case CL_FIB_MOD_1:
			/* group 19 */
			ptr->cls_fib_data.fib_mod_1.chk_msk_ptr = action->chk_msk_ptr;
			ptr->cls_fib_data.fib_mod_1.cache_ctrl = action->cache_ctrl;
			ptr->cls_fib_data.fib_mod_1.pop_l3_vld = action->pop_l3_vld;
			ptr->cls_fib_data.fib_mod_1.pop_l3_chk_ecn_en = action->pop_l3_chk_ecn_en;
			ptr->cls_fib_data.fib_mod_1.pop_l3_en = action->pop_l3_en;
			ptr->cls_fib_data.fib_mod_1.keep_ts_vld = action->keep_ts_vld;
			ptr->cls_fib_data.fib_mod_1.keep_ts_en = action->keep_ts_en;
			ptr->cls_fib_data.fib_mod_1.keep_orig_pkt_vld = action->keep_orig_pkt_vld;
			ptr->cls_fib_data.fib_mod_1.keep_orig_pkt = action->keep_orig_pkt;
			ptr->cls_fib_data.fib_mod_1.stage3_ctrl_vld = action->stage3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.stage3_ctrl = action->stage3_ctrl;
			ptr->cls_fib_data.fib_mod_1.stage2_ctrl_vld = action->stage2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.stage2_ctrl = action->stage2_ctrl;
			ptr->cls_fib_data.fib_mod_1.t5_ctrl_vld = action->t5_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.t5_ctrl = action->t5_ctrl;
			ptr->cls_fib_data.fib_mod_1.t4_ctrl_vld = action->t4_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.t4_ctrl = action->t4_ctrl;
			ptr->cls_fib_data.fib_mod_1.t3_ctrl_vld = action->t3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.t3_ctrl = action->t3_ctrl;
			ptr->cls_fib_data.fib_mod_1.t2_ctrl_vld = action->t2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_1.t2_ctrl = action->t2_ctrl;
			/* group 09 */
			ptr->cls_fib_data.fib_mod_1.vlan_vld = action->vlan_vld;
			ptr->cls_fib_data.fib_mod_1.inner_dei_sel = action->inner_dei_sel;
			ptr->cls_fib_data.fib_mod_1.inner_dei = action->inner_dei;
			ptr->cls_fib_data.fib_mod_1.inner_vid = action->inner_vid;
			ptr->cls_fib_data.fib_mod_1.inner_tpid_enc = action->inner_tpid_enc;
			ptr->cls_fib_data.fib_mod_1.top_dei_sel = action->top_dei_sel;
			ptr->cls_fib_data.fib_mod_1.top_dei = action->top_dei;
			ptr->cls_fib_data.fib_mod_1.top_vid = action->top_vid;
			ptr->cls_fib_data.fib_mod_1.top_tpid_enc = action->top_tpid_enc;
			ptr->cls_fib_data.fib_mod_1.vlan_cnt = action->vlan_cnt;
			/* group 18 */
			ptr->cls_fib_data.fib_mod_1.mrr_vld = action->mrr_vld;
			ptr->cls_fib_data.fib_mod_1.mrr_en = action->mrr_en;
			ptr->cls_fib_data.fib_mod_1.no_drop_vld = action->no_drop_vld;
			ptr->cls_fib_data.fib_mod_1.no_drop = action->no_drop;
			ptr->cls_fib_data.fib_mod_1.dpid_vld = action->dpid_vld;
			ptr->cls_fib_data.fib_mod_1.dpid_pri = action->dpid_pri;
			ptr->cls_fib_data.fib_mod_1.permit = action->permit;
			ptr->cls_fib_data.fib_mod_1.deepq = action->deepq;
			ptr->cls_fib_data.fib_mod_1.mcgid = action->mcgid;
			ptr->cls_fib_data.fib_mod_1.mc = action->mc;
			/* group 17 */
			ptr->cls_fib_data.fib_mod_1.ip_ecn_vld = action->ip_ecn_vld;
			ptr->cls_fib_data.fib_mod_1.ip_ecn_en = action->ip_ecn_en;
			ptr->cls_fib_data.fib_mod_1.ip_tos_vld = action->ip_tos_vld;
			ptr->cls_fib_data.fib_mod_1.ip_tos_6 = action->ip_tos_6;
			ptr->cls_fib_data.fib_mod_1.ip_dscp_marked_vld = action->ip_dscp_marked_vld;
			ptr->cls_fib_data.fib_mod_1.ip_dscp_marked_down = action->ip_dscp_marked_down;
			ptr->cls_fib_data.fib_mod_1.ip_dscp_markdown_en = action->ip_dscp_markdown_en;
			ptr->cls_fib_data.fib_mod_1.ip_dscp_update_en = action->ip_dscp_update_en;
			ptr->cls_fib_data.fib_mod_1.ip_dscp_sel = action->ip_dscp_sel;
			ptr->cls_fib_data.fib_mod_1.ip_dscp  = action->ip_dscp;
			ptr->cls_fib_data.fib_mod_1.dscp_table_sel = action->dscp_table_sel;
			ptr->cls_fib_data.fib_mod_1.inner_802_1p_sel = action->inner_802_1p_sel;
			ptr->cls_fib_data.fib_mod_1.inner_802_1p = action->inner_802_1p;
			ptr->cls_fib_data.fib_mod_1.top_802_1p_sel = action->top_802_1p_sel;
			ptr->cls_fib_data.fib_mod_1.top_802_1p = action->top_802_1p;
			ptr->cls_fib_data.fib_mod_1.qos_802_1p_table_sel = action->qos_802_1p_table_sel;
			ptr->cls_fib_data.fib_mod_1.cos_table_sel = action->cos_table_sel;
			ptr->cls_fib_data.fib_mod_1.cos_sel = action->cos_sel;
			ptr->cls_fib_data.fib_mod_1.cos = action->cos;
			/* group 16 */
			ptr->cls_fib_data.fib_mod_1.pol_vld = action->pol_vld;
			ptr->cls_fib_data.fib_mod_1.qos_premark = action->qos_premark;
			ptr->cls_fib_data.fib_mod_1.pol_all_bypass = action->pol_all_bypass;
			ptr->cls_fib_data.fib_mod_1.pol_en = action->pol_en;
			ptr->cls_fib_data.fib_mod_1.pol_base = action->pol_base;
			ptr->cls_fib_data.fib_mod_1.pol_sel = action->pol_sel;
			ptr->cls_fib_data.fib_mod_1.pol_table_sel = action->pol_table_sel;
			ptr->cls_fib_data.fib_mod_1.pol_grp_vld = action->pol_grp_vld;
			ptr->cls_fib_data.fib_mod_1.pol_grp_id = action->pol_grp_id;
			/* group 12 */
			ptr->cls_fib_data.fib_mod_1.mdata_w_vld_0 = action->mdata_w_vld_0;
			ptr->cls_fib_data.fib_mod_1.mdata_w_0 = action->mdata_w_0;
			/* group 13 */
			ptr->cls_fib_data.fib_mod_1.mdata_w_vld_1 = action->mdata_w_vld_1;
			ptr->cls_fib_data.fib_mod_1.mdata_w_1 = action->mdata_w_1;
			break;
		case CL_FIB_MOD_2:
			/* group 19 */
			ptr->cls_fib_data.fib_mod_2.chk_msk_ptr = action->chk_msk_ptr;
			ptr->cls_fib_data.fib_mod_2.cache_ctrl = action->cache_ctrl;
			ptr->cls_fib_data.fib_mod_2.pop_l3_vld = action->pop_l3_vld;
			ptr->cls_fib_data.fib_mod_2.pop_l3_chk_ecn_en = action->pop_l3_chk_ecn_en;
			ptr->cls_fib_data.fib_mod_2.pop_l3_en = action->pop_l3_en;
			ptr->cls_fib_data.fib_mod_2.keep_ts_vld = action->keep_ts_vld;
			ptr->cls_fib_data.fib_mod_2.keep_ts_en = action->keep_ts_en;
			ptr->cls_fib_data.fib_mod_2.keep_orig_pkt_vld = action->keep_orig_pkt_vld;
			ptr->cls_fib_data.fib_mod_2.keep_orig_pkt = action->keep_orig_pkt;
			ptr->cls_fib_data.fib_mod_2.stage3_ctrl_vld = action->stage3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.stage3_ctrl = action->stage3_ctrl;
			ptr->cls_fib_data.fib_mod_2.stage2_ctrl_vld = action->stage2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.stage2_ctrl = action->stage2_ctrl;
			ptr->cls_fib_data.fib_mod_2.t5_ctrl_vld = action->t5_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.t5_ctrl = action->t5_ctrl;
			ptr->cls_fib_data.fib_mod_2.t4_ctrl_vld = action->t4_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.t4_ctrl = action->t4_ctrl;
			ptr->cls_fib_data.fib_mod_2.t3_ctrl_vld = action->t3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.t3_ctrl = action->t3_ctrl;
			ptr->cls_fib_data.fib_mod_2.t2_ctrl_vld = action->t2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.t2_ctrl = action->t2_ctrl;
			/* group 09 */
			ptr->cls_fib_data.fib_mod_2.vlan_vld = action->vlan_vld;
			ptr->cls_fib_data.fib_mod_2.inner_dei_sel = action->inner_dei_sel;
			ptr->cls_fib_data.fib_mod_2.inner_dei = action->inner_dei;
			ptr->cls_fib_data.fib_mod_2.inner_vid = action->inner_vid;
			ptr->cls_fib_data.fib_mod_2.inner_tpid_enc = action->inner_tpid_enc;
			ptr->cls_fib_data.fib_mod_2.top_dei_sel = action->top_dei_sel;
			ptr->cls_fib_data.fib_mod_2.top_dei = action->top_dei;
			ptr->cls_fib_data.fib_mod_2.top_vid = action->top_vid;
			ptr->cls_fib_data.fib_mod_2.top_tpid_enc = action->top_tpid_enc;
			ptr->cls_fib_data.fib_mod_2.vlan_cnt = action->vlan_cnt;
			/* group 18 */
			ptr->cls_fib_data.fib_mod_2.mrr_vld = action->mrr_vld;
			ptr->cls_fib_data.fib_mod_2.mrr_en = action->mrr_en;
			ptr->cls_fib_data.fib_mod_2.no_drop_vld = action->no_drop_vld;
			ptr->cls_fib_data.fib_mod_2.no_drop = action->no_drop;
			ptr->cls_fib_data.fib_mod_2.dpid_vld = action->dpid_vld;
			ptr->cls_fib_data.fib_mod_2.dpid_pri = action->dpid_pri;
			ptr->cls_fib_data.fib_mod_2.permit = action->permit;
			ptr->cls_fib_data.fib_mod_2.deepq = action->deepq;
			ptr->cls_fib_data.fib_mod_2.mcgid = action->mcgid;
			ptr->cls_fib_data.fib_mod_2.mc = action->mc;
			/* group 17 */
			ptr->cls_fib_data.fib_mod_2.ip_ecn_vld = action->ip_ecn_vld;
			ptr->cls_fib_data.fib_mod_2.ip_ecn_en = action->ip_ecn_en;
			ptr->cls_fib_data.fib_mod_2.ip_tos_vld = action->ip_tos_vld;
			ptr->cls_fib_data.fib_mod_2.ip_tos_6 = action->ip_tos_6;
			ptr->cls_fib_data.fib_mod_2.ip_dscp_marked_vld = action->ip_dscp_marked_vld;
			ptr->cls_fib_data.fib_mod_2.ip_dscp_marked_down = action->ip_dscp_marked_down;
			ptr->cls_fib_data.fib_mod_2.ip_dscp_markdown_en = action->ip_dscp_markdown_en;
			ptr->cls_fib_data.fib_mod_2.ip_dscp_update_en = action->ip_dscp_update_en;
			ptr->cls_fib_data.fib_mod_2.ip_dscp_sel = action->ip_dscp_sel;
			ptr->cls_fib_data.fib_mod_2.ip_dscp  = action->ip_dscp;
			ptr->cls_fib_data.fib_mod_2.dscp_table_sel = action->dscp_table_sel;
			ptr->cls_fib_data.fib_mod_2.inner_802_1p_sel = action->inner_802_1p_sel;
			ptr->cls_fib_data.fib_mod_2.inner_802_1p = action->inner_802_1p;
			ptr->cls_fib_data.fib_mod_2.top_802_1p_sel = action->top_802_1p_sel;
			ptr->cls_fib_data.fib_mod_2.top_802_1p = action->top_802_1p;
			ptr->cls_fib_data.fib_mod_2.qos_802_1p_table_sel = action->qos_802_1p_table_sel;
			ptr->cls_fib_data.fib_mod_2.cos_table_sel = action->cos_table_sel;
			ptr->cls_fib_data.fib_mod_2.cos_sel = action->cos_sel;
			ptr->cls_fib_data.fib_mod_2.cos = action->cos;
			/* group 16 */
			ptr->cls_fib_data.fib_mod_2.pol_vld = action->pol_vld;
			ptr->cls_fib_data.fib_mod_2.qos_premark = action->qos_premark;
			ptr->cls_fib_data.fib_mod_2.pol_all_bypass = action->pol_all_bypass;
			ptr->cls_fib_data.fib_mod_2.pol_en = action->pol_en;
			ptr->cls_fib_data.fib_mod_2.pol_base = action->pol_base;
			ptr->cls_fib_data.fib_mod_2.pol_sel = action->pol_sel;
			ptr->cls_fib_data.fib_mod_2.pol_table_sel = action->pol_table_sel;
			ptr->cls_fib_data.fib_mod_2.pol_grp_vld = action->pol_grp_vld;
			ptr->cls_fib_data.fib_mod_2.pol_grp_id = action->pol_grp_id;
			/* group 08 */
			ptr->cls_fib_data.fib_mod_2.ip_ttl_vld = action->ip_ttl_vld;
			ptr->cls_fib_data.fib_mod_2.ip_ttl_zero_discard_en = action->ip_ttl_zero_discard_en;
			ptr->cls_fib_data.fib_mod_2.ip_ttl_cmd = action->ip_ttl_cmd;
			ptr->cls_fib_data.fib_mod_2.ip_ttl = action->ip_ttl;
			ptr->cls_fib_data.fib_mod_2.six_rd_ipda_ctrl_vld = action->six_rd_ipda_ctrl_vld;
			ptr->cls_fib_data.fib_mod_2.six_rd_ipda_from_v6 = action->six_rd_ipda_from_v6;
			ptr->cls_fib_data.fib_mod_2.ip_mtu_enc_vld = action->ip_mtu_enc_vld;
			ptr->cls_fib_data.fib_mod_2.ip_mtu_enc = action->ip_mtu_enc;
			ptr->cls_fib_data.fib_mod_2.l2_format_vld = action->l2_format_vld;
			ptr->cls_fib_data.fib_mod_2.l2_format = action->l2_format;
			ptr->cls_fib_data.fib_mod_2.l3_egress_if_vld = action->l3_egress_if_vld;
			/* group 07 */
			ptr->cls_fib_data.fib_mod_2.pppoe_vld = action->pppoe_vld;
			ptr->cls_fib_data.fib_mod_2.pppoe_set = action->pppoe_set;
			ptr->cls_fib_data.fib_mod_2.pppoe_session_id = action->pppoe_session_id;
			/* group 12 */
			ptr->cls_fib_data.fib_mod_2.mdata_w_vld_0 = action->mdata_w_vld_0;
			ptr->cls_fib_data.fib_mod_2.mdata_w_0 = action->mdata_w_0;
			/* group 13 */
			ptr->cls_fib_data.fib_mod_2.mdata_w_vld_1 = action->mdata_w_vld_1;
			ptr->cls_fib_data.fib_mod_2.mdata_w_1_0 = action->mdata_w_1;
			ptr->cls_fib_data.fib_mod_2.mdata_w_1_1 = action->mdata_w_1 >> 7;
			/* group 14 */
			ptr->cls_fib_data.fib_mod_2.mdata_w_vld_2 = action->mdata_w_vld_2;
			ptr->cls_fib_data.fib_mod_2.mdata_w_2 = action->mdata_w_2;
			/* group 15 */
			ptr->cls_fib_data.fib_mod_2.mdata_w_vld_3 = action->mdata_w_vld_3;
			ptr->cls_fib_data.fib_mod_2.mdata_w_3 = action->mdata_w_3;
			/* group 11 */
			ptr->cls_fib_data.fib_mod_2.mac_da_vld = action->mac_da_vld;
			ptr->cls_fib_data.fib_mod_2.mac_da_0 = action->mac_da_0;
			ptr->cls_fib_data.fib_mod_2.mac_da_1 = action->mac_da_1;
			ptr->cls_fib_data.fib_mod_2.mac_da_2 = action->mac_da_2;
			ptr->cls_fib_data.fib_mod_2.mac_da_3 = action->mac_da_3;
			ptr->cls_fib_data.fib_mod_2.mac_da_4 = action->mac_da_4;
			ptr->cls_fib_data.fib_mod_2.mac_da_5 = action->mac_da_5;
			/* group 10 */
			ptr->cls_fib_data.fib_mod_2.mac_sa_vld = action->mac_sa_vld;
			ptr->cls_fib_data.fib_mod_2.mac_sa_0 = action->mac_sa_0;
			ptr->cls_fib_data.fib_mod_2.mac_sa_1 = action->mac_sa_1;
			ptr->cls_fib_data.fib_mod_2.mac_sa_2 = action->mac_sa_2;
			ptr->cls_fib_data.fib_mod_2.mac_sa_3 = action->mac_sa_3;
			ptr->cls_fib_data.fib_mod_2.mac_sa_4 = action->mac_sa_4;
			ptr->cls_fib_data.fib_mod_2.mac_sa_5 = action->mac_sa_5;
			break;
		case CL_FIB_MOD_3:
			/* group 19 */
			ptr->cls_fib_data.fib_mod_3.chk_msk_ptr = action->chk_msk_ptr;
			ptr->cls_fib_data.fib_mod_3.cache_ctrl = action->cache_ctrl;
			ptr->cls_fib_data.fib_mod_3.pop_l3_vld = action->pop_l3_vld;
			ptr->cls_fib_data.fib_mod_3.pop_l3_chk_ecn_en = action->pop_l3_chk_ecn_en;
			ptr->cls_fib_data.fib_mod_3.pop_l3_en = action->pop_l3_en;
			ptr->cls_fib_data.fib_mod_3.keep_ts_vld = action->keep_ts_vld;
			ptr->cls_fib_data.fib_mod_3.keep_ts_en = action->keep_ts_en;
			ptr->cls_fib_data.fib_mod_3.keep_orig_pkt_vld = action->keep_orig_pkt_vld;
			ptr->cls_fib_data.fib_mod_3.keep_orig_pkt = action->keep_orig_pkt;
			ptr->cls_fib_data.fib_mod_3.stage3_ctrl_vld = action->stage3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.stage3_ctrl = action->stage3_ctrl;
			ptr->cls_fib_data.fib_mod_3.stage2_ctrl_vld = action->stage2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.stage2_ctrl = action->stage2_ctrl;
			ptr->cls_fib_data.fib_mod_3.t5_ctrl_vld = action->t5_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.t5_ctrl = action->t5_ctrl;
			ptr->cls_fib_data.fib_mod_3.t4_ctrl_vld = action->t4_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.t4_ctrl = action->t4_ctrl;
			ptr->cls_fib_data.fib_mod_3.t3_ctrl_vld = action->t3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.t3_ctrl = action->t3_ctrl;
			ptr->cls_fib_data.fib_mod_3.t2_ctrl_vld = action->t2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.t2_ctrl = action->t2_ctrl;
			/* group 09 */
			ptr->cls_fib_data.fib_mod_3.vlan_vld = action->vlan_vld;
			ptr->cls_fib_data.fib_mod_3.inner_dei_sel = action->inner_dei_sel;
			ptr->cls_fib_data.fib_mod_3.inner_dei = action->inner_dei;
			ptr->cls_fib_data.fib_mod_3.inner_vid = action->inner_vid;
			ptr->cls_fib_data.fib_mod_3.inner_tpid_enc = action->inner_tpid_enc;
			ptr->cls_fib_data.fib_mod_3.top_dei_sel = action->top_dei_sel;
			ptr->cls_fib_data.fib_mod_3.top_dei = action->top_dei;
			ptr->cls_fib_data.fib_mod_3.top_vid = action->top_vid;
			ptr->cls_fib_data.fib_mod_3.top_tpid_enc = action->top_tpid_enc;
			ptr->cls_fib_data.fib_mod_3.vlan_cnt = action->vlan_cnt;
			/* group 18 */
			ptr->cls_fib_data.fib_mod_3.mrr_vld = action->mrr_vld;
			ptr->cls_fib_data.fib_mod_3.mrr_en = action->mrr_en;
			ptr->cls_fib_data.fib_mod_3.no_drop_vld = action->no_drop_vld;
			ptr->cls_fib_data.fib_mod_3.no_drop = action->no_drop;
			ptr->cls_fib_data.fib_mod_3.dpid_vld = action->dpid_vld;
			ptr->cls_fib_data.fib_mod_3.dpid_pri = action->dpid_pri;
			ptr->cls_fib_data.fib_mod_3.permit = action->permit;
			ptr->cls_fib_data.fib_mod_3.deepq = action->deepq;
			ptr->cls_fib_data.fib_mod_3.mcgid = action->mcgid;
			ptr->cls_fib_data.fib_mod_3.mc = action->mc;
			/* group 17 */
			ptr->cls_fib_data.fib_mod_3.ip_ecn_vld = action->ip_ecn_vld;
			ptr->cls_fib_data.fib_mod_3.ip_ecn_en = action->ip_ecn_en;
			ptr->cls_fib_data.fib_mod_3.ip_tos_vld = action->ip_tos_vld;
			ptr->cls_fib_data.fib_mod_3.ip_tos_6 = action->ip_tos_6;
			ptr->cls_fib_data.fib_mod_3.ip_dscp_marked_vld = action->ip_dscp_marked_vld;
			ptr->cls_fib_data.fib_mod_3.ip_dscp_marked_down = action->ip_dscp_marked_down;
			ptr->cls_fib_data.fib_mod_3.ip_dscp_markdown_en = action->ip_dscp_markdown_en;
			ptr->cls_fib_data.fib_mod_3.ip_dscp_update_en = action->ip_dscp_update_en;
			ptr->cls_fib_data.fib_mod_3.ip_dscp_sel = action->ip_dscp_sel;
			ptr->cls_fib_data.fib_mod_3.ip_dscp  = action->ip_dscp;
			ptr->cls_fib_data.fib_mod_3.dscp_table_sel = action->dscp_table_sel;
			ptr->cls_fib_data.fib_mod_3.inner_802_1p_sel = action->inner_802_1p_sel;
			ptr->cls_fib_data.fib_mod_3.inner_802_1p = action->inner_802_1p;
			ptr->cls_fib_data.fib_mod_3.top_802_1p_sel = action->top_802_1p_sel;
			ptr->cls_fib_data.fib_mod_3.top_802_1p = action->top_802_1p;
			ptr->cls_fib_data.fib_mod_3.qos_802_1p_table_sel = action->qos_802_1p_table_sel;
			ptr->cls_fib_data.fib_mod_3.cos_table_sel = action->cos_table_sel;
			ptr->cls_fib_data.fib_mod_3.cos_sel = action->cos_sel;
			ptr->cls_fib_data.fib_mod_3.cos = action->cos;
			/* group 16 */
			ptr->cls_fib_data.fib_mod_3.pol_vld = action->pol_vld;
			ptr->cls_fib_data.fib_mod_3.qos_premark = action->qos_premark;
			ptr->cls_fib_data.fib_mod_3.pol_all_bypass = action->pol_all_bypass;
			ptr->cls_fib_data.fib_mod_3.pol_en = action->pol_en;
			ptr->cls_fib_data.fib_mod_3.pol_base = action->pol_base;
			ptr->cls_fib_data.fib_mod_3.pol_sel = action->pol_sel;
			ptr->cls_fib_data.fib_mod_3.pol_table_sel = action->pol_table_sel;
			ptr->cls_fib_data.fib_mod_3.pol_grp_vld = action->pol_grp_vld;
			ptr->cls_fib_data.fib_mod_3.pol_grp_id = action->pol_grp_id;
			/* group 08 */
			ptr->cls_fib_data.fib_mod_3.ip_ttl_vld = action->ip_ttl_vld;
			ptr->cls_fib_data.fib_mod_3.ip_ttl_zero_discard_en = action->ip_ttl_zero_discard_en;
			ptr->cls_fib_data.fib_mod_3.ip_ttl_cmd = action->ip_ttl_cmd;
			ptr->cls_fib_data.fib_mod_3.ip_ttl = action->ip_ttl;
			ptr->cls_fib_data.fib_mod_3.six_rd_ipda_ctrl_vld = action->six_rd_ipda_ctrl_vld;
			ptr->cls_fib_data.fib_mod_3.six_rd_ipda_from_v6 = action->six_rd_ipda_from_v6;
			ptr->cls_fib_data.fib_mod_3.ip_mtu_enc_vld = action->ip_mtu_enc_vld;
			ptr->cls_fib_data.fib_mod_3.ip_mtu_enc = action->ip_mtu_enc;
			ptr->cls_fib_data.fib_mod_3.l2_format_vld = action->l2_format_vld;
			ptr->cls_fib_data.fib_mod_3.l2_format = action->l2_format;
			ptr->cls_fib_data.fib_mod_3.l3_egress_if_vld = action->l3_egress_if_vld;
			/* group 07 */
			ptr->cls_fib_data.fib_mod_3.pppoe_vld = action->pppoe_vld;
			ptr->cls_fib_data.fib_mod_3.pppoe_set = action->pppoe_set;
			ptr->cls_fib_data.fib_mod_3.pppoe_session_id = action->pppoe_session_id;
			/* group 12 */
			ptr->cls_fib_data.fib_mod_3.mdata_w_vld_0 = action->mdata_w_vld_0;
			ptr->cls_fib_data.fib_mod_3.mdata_w_0 = action->mdata_w_0;
			/* group 13 */
			ptr->cls_fib_data.fib_mod_3.mdata_w_vld_1 = action->mdata_w_vld_1;
			ptr->cls_fib_data.fib_mod_3.mdata_w_1 = action->mdata_w_1;
			/* group 6 */
			ptr->cls_fib_data.fib_mod_3.ip_sa_h_vld = action->ip_sa_h_vld;
			ptr->cls_fib_data.fib_mod_3.ip_sa_1 = action->ip_sa_1;
			ptr->cls_fib_data.fib_mod_3.ip_sa_2 = action->ip_sa_2;
			ptr->cls_fib_data.fib_mod_3.ip_sa_3 = action->ip_sa_3;
			/* group 5 */
			ptr->cls_fib_data.fib_mod_3.ip_sa_l_vld = action->ip_sa_l_vld;
			ptr->cls_fib_data.fib_mod_3.ip_sa_0 = action->ip_sa_0;
			/* group 2 */
			ptr->cls_fib_data.fib_mod_3.l4_sp_vld = action->l4_sp_vld;
			ptr->cls_fib_data.fib_mod_3.l4_sp = action->l4_sp;
			break;
		case CL_FIB_MOD_4:
			/* group 19 */
			ptr->cls_fib_data.fib_mod_4.chk_msk_ptr = action->chk_msk_ptr;
			ptr->cls_fib_data.fib_mod_4.cache_ctrl = action->cache_ctrl;
			ptr->cls_fib_data.fib_mod_4.pop_l3_vld = action->pop_l3_vld;
			ptr->cls_fib_data.fib_mod_4.pop_l3_chk_ecn_en = action->pop_l3_chk_ecn_en;
			ptr->cls_fib_data.fib_mod_4.pop_l3_en = action->pop_l3_en;
			ptr->cls_fib_data.fib_mod_4.keep_ts_vld = action->keep_ts_vld;
			ptr->cls_fib_data.fib_mod_4.keep_ts_en = action->keep_ts_en;
			ptr->cls_fib_data.fib_mod_4.keep_orig_pkt_vld = action->keep_orig_pkt_vld;
			ptr->cls_fib_data.fib_mod_4.keep_orig_pkt = action->keep_orig_pkt;
			ptr->cls_fib_data.fib_mod_4.stage3_ctrl_vld = action->stage3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.stage3_ctrl = action->stage3_ctrl;
			ptr->cls_fib_data.fib_mod_4.stage2_ctrl_vld = action->stage2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.stage2_ctrl = action->stage2_ctrl;
			ptr->cls_fib_data.fib_mod_4.t5_ctrl_vld = action->t5_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.t5_ctrl = action->t5_ctrl;
			ptr->cls_fib_data.fib_mod_4.t4_ctrl_vld = action->t4_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.t4_ctrl = action->t4_ctrl;
			ptr->cls_fib_data.fib_mod_4.t3_ctrl_vld = action->t3_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.t3_ctrl = action->t3_ctrl;
			ptr->cls_fib_data.fib_mod_4.t2_ctrl_vld = action->t2_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.t2_ctrl = action->t2_ctrl;
			/* group 09 */
			ptr->cls_fib_data.fib_mod_4.vlan_vld = action->vlan_vld;
			ptr->cls_fib_data.fib_mod_4.inner_dei_sel = action->inner_dei_sel;
			ptr->cls_fib_data.fib_mod_4.inner_dei = action->inner_dei;
			ptr->cls_fib_data.fib_mod_4.inner_vid = action->inner_vid;
			ptr->cls_fib_data.fib_mod_4.inner_tpid_enc = action->inner_tpid_enc;
			ptr->cls_fib_data.fib_mod_4.top_dei_sel = action->top_dei_sel;
			ptr->cls_fib_data.fib_mod_4.top_dei = action->top_dei;
			ptr->cls_fib_data.fib_mod_4.top_vid = action->top_vid;
			ptr->cls_fib_data.fib_mod_4.top_tpid_enc = action->top_tpid_enc;
			ptr->cls_fib_data.fib_mod_4.vlan_cnt = action->vlan_cnt;
			/* group 18 */
			ptr->cls_fib_data.fib_mod_4.mrr_vld = action->mrr_vld;
			ptr->cls_fib_data.fib_mod_4.mrr_en = action->mrr_en;
			ptr->cls_fib_data.fib_mod_4.no_drop_vld = action->no_drop_vld;
			ptr->cls_fib_data.fib_mod_4.no_drop = action->no_drop;
			ptr->cls_fib_data.fib_mod_4.dpid_vld = action->dpid_vld;
			ptr->cls_fib_data.fib_mod_4.dpid_pri = action->dpid_pri;
			ptr->cls_fib_data.fib_mod_4.permit = action->permit;
			ptr->cls_fib_data.fib_mod_4.deepq = action->deepq;
			ptr->cls_fib_data.fib_mod_4.mcgid = action->mcgid;
			ptr->cls_fib_data.fib_mod_4.mc = action->mc;
			/* group 17 */
			ptr->cls_fib_data.fib_mod_4.ip_ecn_vld = action->ip_ecn_vld;
			ptr->cls_fib_data.fib_mod_4.ip_ecn_en = action->ip_ecn_en;
			ptr->cls_fib_data.fib_mod_4.ip_tos_vld = action->ip_tos_vld;
			ptr->cls_fib_data.fib_mod_4.ip_tos_6 = action->ip_tos_6;
			ptr->cls_fib_data.fib_mod_4.ip_dscp_marked_vld = action->ip_dscp_marked_vld;
			ptr->cls_fib_data.fib_mod_4.ip_dscp_marked_down = action->ip_dscp_marked_down;
			ptr->cls_fib_data.fib_mod_4.ip_dscp_markdown_en = action->ip_dscp_markdown_en;
			ptr->cls_fib_data.fib_mod_4.ip_dscp_update_en = action->ip_dscp_update_en;
			ptr->cls_fib_data.fib_mod_4.ip_dscp_sel = action->ip_dscp_sel;
			ptr->cls_fib_data.fib_mod_4.ip_dscp  = action->ip_dscp;
			ptr->cls_fib_data.fib_mod_4.dscp_table_sel = action->dscp_table_sel;
			ptr->cls_fib_data.fib_mod_4.inner_802_1p_sel = action->inner_802_1p_sel;
			ptr->cls_fib_data.fib_mod_4.inner_802_1p = action->inner_802_1p;
			ptr->cls_fib_data.fib_mod_4.top_802_1p_sel = action->top_802_1p_sel;
			ptr->cls_fib_data.fib_mod_4.top_802_1p = action->top_802_1p;
			ptr->cls_fib_data.fib_mod_4.qos_802_1p_table_sel = action->qos_802_1p_table_sel;
			ptr->cls_fib_data.fib_mod_4.cos_table_sel = action->cos_table_sel;
			ptr->cls_fib_data.fib_mod_4.cos_sel = action->cos_sel;
			ptr->cls_fib_data.fib_mod_4.cos = action->cos;
			/* group 16 */
			ptr->cls_fib_data.fib_mod_4.pol_vld = action->pol_vld;
			ptr->cls_fib_data.fib_mod_4.qos_premark = action->qos_premark;
			ptr->cls_fib_data.fib_mod_4.pol_all_bypass = action->pol_all_bypass;
			ptr->cls_fib_data.fib_mod_4.pol_en = action->pol_en;
			ptr->cls_fib_data.fib_mod_4.pol_base = action->pol_base;
			ptr->cls_fib_data.fib_mod_4.pol_sel = action->pol_sel;
			ptr->cls_fib_data.fib_mod_4.pol_table_sel = action->pol_table_sel;
			ptr->cls_fib_data.fib_mod_4.pol_grp_vld = action->pol_grp_vld;
			ptr->cls_fib_data.fib_mod_4.pol_grp_id = action->pol_grp_id;
			/* group 08 */
			ptr->cls_fib_data.fib_mod_4.ip_ttl_vld = action->ip_ttl_vld;
			ptr->cls_fib_data.fib_mod_4.ip_ttl_zero_discard_en = action->ip_ttl_zero_discard_en;
			ptr->cls_fib_data.fib_mod_4.ip_ttl_cmd = action->ip_ttl_cmd;
			ptr->cls_fib_data.fib_mod_4.ip_ttl = action->ip_ttl;
			ptr->cls_fib_data.fib_mod_4.six_rd_ipda_ctrl_vld = action->six_rd_ipda_ctrl_vld;
			ptr->cls_fib_data.fib_mod_4.six_rd_ipda_from_v6 = action->six_rd_ipda_from_v6;
			ptr->cls_fib_data.fib_mod_4.ip_mtu_enc_vld = action->ip_mtu_enc_vld;
			ptr->cls_fib_data.fib_mod_4.ip_mtu_enc = action->ip_mtu_enc;
			ptr->cls_fib_data.fib_mod_4.l2_format_vld = action->l2_format_vld;
			ptr->cls_fib_data.fib_mod_4.l2_format = action->l2_format;
			ptr->cls_fib_data.fib_mod_4.l3_egress_if_vld = action->l3_egress_if_vld;
			/* group 07 */
			ptr->cls_fib_data.fib_mod_4.pppoe_vld = action->pppoe_vld;
			ptr->cls_fib_data.fib_mod_4.pppoe_set = action->pppoe_set;
			ptr->cls_fib_data.fib_mod_4.pppoe_session_id = action->pppoe_session_id;
			/* group 12 */
			ptr->cls_fib_data.fib_mod_4.mdata_w_vld_0 = action->mdata_w_vld_0;
			ptr->cls_fib_data.fib_mod_4.mdata_w_0 = action->mdata_w_0;
			/* group 13 */
			ptr->cls_fib_data.fib_mod_4.mdata_w_vld_1 = action->mdata_w_vld_1;
			ptr->cls_fib_data.fib_mod_4.mdata_w_1 = action->mdata_w_1;
			/* group 4 */
			ptr->cls_fib_data.fib_mod_4.ip_da_h_vld = action->ip_da_h_vld;
			ptr->cls_fib_data.fib_mod_4.ip_da_1 = action->ip_da_1;
			ptr->cls_fib_data.fib_mod_4.ip_da_2 = action->ip_da_2;
			ptr->cls_fib_data.fib_mod_4.ip_da_3 = action->ip_da_3;
			/* group 3 */
			ptr->cls_fib_data.fib_mod_4.ip_da_l_vld = action->ip_da_l_vld;
			ptr->cls_fib_data.fib_mod_4.ip_da_0 = action->ip_da_0;
			/* group 1 */
			ptr->cls_fib_data.fib_mod_4.l4_dp_vld = action->l4_dp_vld;
			ptr->cls_fib_data.fib_mod_4.l4_dp = action->l4_dp;
			break;
		default:
			return AAL_E_INVALID;
	}

	return AAL_E_OK;
}

static ca_int32_t __l3_cls_fib_idx_get(
	aal_table_t *key_table,
	l3_cls_key_type_t key_type,
	ca_uint32_t key_tbl_idx,
	ca_uint8_t key_idx,
	ca_uint32_t *fib_tbl_idx)
{
	if (key_type > CL_FULL_KEY || key_type < CL_IF_ID_KEY || key_tbl_idx >= key_table->max_entry)
		return AAL_E_OUTRANGE;
	if (fib_tbl_idx == NULL)
		return AAL_E_NULLPTR;

	/* get fib tbl idx */
	switch(key_type) {
		case CL_IF_ID_KEY:
			/* 4 key */
			*fib_tbl_idx = (key_tbl_idx << 2) | key_idx;
			break;
		case CL_IPV4_SHORT_KEY:
		case CL_SPCL_PKT_KEY:
			/* 2 keys */
			*fib_tbl_idx = (key_tbl_idx << 2) | (key_idx << 1);
			break;
		case CL_FULL_KEY:
			/* 0.5 keys */
			*fib_tbl_idx = ((key_tbl_idx >> 1) << 3) | 0x4;
			break;
		default:
			/* 1 keys */
			/* CL_IPV4_TUNNEL_ID_KEY */
			/* CL_IPV6_SHORT_KEY */
			/* CL_MCST_KEY */
			*fib_tbl_idx = (key_tbl_idx << 2);
	}

	return AAL_E_OK;
}

int aal_l3_cls_tbl_init(void)
{
	int ret;
	cls_action_t action;
	l3_cls_rule_t rule;
	unsigned int rslt_idx;

	/* init l3 cls key */
	spin_lock_init(&l3_cls_key_table.lock);
	ret = __aal_table_register(AAL_TABLE_L3_CLS_KEY, &l3_cls_key_table);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS KEY initialization fail\n", __func__);
		return ret;
	}

	/* init l3 cls fib */
	spin_lock_init(&l3_cls_fib_table.lock);
	ret = __aal_table_register(AAL_TABLE_L3_CLS_FIB, &l3_cls_fib_table);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS FIB initialization fail\n", __func__);
		return ret;
	}

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	/* init l3 cls key */
	spin_lock_init(&hgu_8279_l3_cls_key_table.lock);
	ret = __aal_table_register(AAL_TABLE_8279_L3_CLS_KEY, &hgu_8279_l3_cls_key_table);
	if (ret != AAL_E_OK) {
		printk("%s: HGU 8279 L3 CLS KEY initialization fail\n", __func__);
		return ret;
	}

	/* init l3 cls fib */
	spin_lock_init(&hgu_8279_l3_cls_fib_table.lock);
	ret = __aal_table_register(AAL_TABLE_8279_L3_CLS_FIB, &hgu_8279_l3_cls_fib_table);
	if (ret != AAL_E_OK) {
		printk("%s: HGU 8279 L3 CLS FIB initialization fail\n", __func__);
		return ret;
	}
#endif

	spin_lock_init(&cls_lock);

	/* set WAN default fib */
	memset(&action, 0, sizeof(cls_action_t));
	/* set msk_ctrl */
#if defined(CONFIG_LUNA_G3_SERIES)	
	action.t2_ctrl_vld = 1;
	action.t2_ctrl = 0;
	action.t3_ctrl_vld = 1;
	action.t3_ctrl = T3_CTRL_BYPASS;
	action.t4_ctrl_vld = 1;
	action.t4_ctrl = T4_CTRL_BYPASS;
#else
	action.t2_ctrl_vld = 1;	
	action.t2_ctrl = T2_CTRL_BYPASS;	
	action.t3_ctrl_vld = 1;	
	action.t3_ctrl = T3_CTRL_BYPASS;	
	action.t4_ctrl_vld = 1;	
	action.t4_ctrl = T4_CTRL_BYPASS;
#endif
	action.t5_ctrl_vld = 1;
	action.t5_ctrl = T5_CTRL_BYPASS;
	/* set drop packets */
	action.dpid_vld = 1;
	action.permit = 0;
	ret = aal_l3_cls_default_set(0, 0, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS WAN DEF FIB (0,0) initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk("%s: L3 CLS WAN DEF FIB initialization success\n", __func__);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	ret = aal_l3_cls_default_set(1, 0, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: device_id = 1 L3 CLS WAN DEF FIB (0,0) initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk("%s: device_id = 1 L3 CLS WAN DEF FIB initialization success\n", __func__);
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	/* overwrite previous WAN default fib */
	action.t4_ctrl = l3fe_hash_lite_profile;
	action.t4_ctrl_vld = 1;
	action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	action.stage2_ctrl_vld = 1;
	action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	action.stage3_ctrl_vld = 1;
	action.dpid_vld = 1;
	action.permit = 1;
	action.mc = 0;
	action.mcgid = AAL_LPORT_L3_LAN;
	ret = aal_l3_cls_default_set(0, 0, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS WAN DEF FIB (0,0) for Saturn SFU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
#endif

	/* set LAN default fib */
	memset(&action, 0, sizeof(cls_action_t));
	/* set msk_ctrl */
#if defined(CONFIG_LUNA_G3_SERIES)	
	action.t2_ctrl_vld = 1;
	action.t2_ctrl = 1;
	action.t3_ctrl_vld = 1;
	action.t3_ctrl = T3_CTRL_BYPASS;
	action.t4_ctrl_vld = 1;
	action.t4_ctrl = T4_CTRL_BYPASS;
	action.t5_ctrl_vld = 1;
	action.t5_ctrl = T5_CTRL_BYPASS;
	action.dpid_vld = 1;
	action.permit = 1;
#else
	action.t2_ctrl_vld = 1;	
	action.t2_ctrl = T2_CTRL_BYPASS;	
	action.t3_ctrl_vld = 1;	
	action.t3_ctrl = T3_CTRL_BYPASS;	
	action.t4_ctrl_vld = 1;	
	action.t4_ctrl = T4_CTRL_BYPASS;
	action.t5_ctrl_vld = 1;
	action.t5_ctrl = T5_CTRL_BYPASS;
	/* set drop packets */
	action.dpid_vld = 1;
	action.permit = 0;
#endif
	ret = aal_l3_cls_default_set(0, 1, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS LAN DEF FIB (1,0) initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk("%s: L3 CLS LAN DEF FIB initialization success\n", __func__);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	ret = aal_l3_cls_default_set(1, 1, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: device_id = 1 L3 CLS LAN DEF FIB (1,0) initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk("%s: device_id = 1 L3 CLS LAN DEF FIB initialization success\n", __func__);
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	/* overwrite previous LAN default fib */
	action.t4_ctrl = l3fe_hash_lite_profile;
	action.t4_ctrl_vld = 1;
	action.stage2_ctrl = STAGE2_CTRL_BYPASS;
	action.stage2_ctrl_vld = 1;
	action.stage3_ctrl = STAGE3_CTRL_BYPASS;
	action.stage3_ctrl_vld = 1;
	action.dpid_vld = 1;
	action.permit = 1;
	action.mc = 0;
	action.mcgid = AAL_LPORT_L3_WAN;
	ret = aal_l3_cls_default_set(0, 1, CL_RSLT_TYPE_0, &action);
	if (ret != AAL_E_OK) {
		printk("%s: L3 CLS LAN DEF FIB (0,0) for Saturn SFU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
#else

	/* Add default rule for MC packet */
	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
#if !defined(CONFIG_LUNA_G3_SERIES)	/* Ignore lspid */
	rule.key.lspid = AAL_LPORT_L3_WAN;
	rule.key.msk_lspid = 0; /* match */
#endif

	rule.key.ip_vld = 1; /* IPv4 or IPv6 */
	rule.key.msk_ip_vld = 0;

	rule.key.ip_da_mc = 1; /* MC IP DA */
	rule.key.msk_ip_da_mc = 0;

#if defined(CONFIG_LUNA_G3_SERIES)
	rule.key.pppoe_session_id = 0;		// pppoetagif = 0
	rule.key.msk_pppoe_session = 0;
	rule.action.t2_ctrl = 0x2; /* HASH_PROFILE_2 */
#else

	rule.action.t2_ctrl = 0x2; /* HASH */
#endif

	rule.action.t2_ctrl_vld = 1;
	rule.action.t3_ctrl = 0xF;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t4_ctrl = 0xF;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t5_ctrl = 0xF;
	rule.action.t5_ctrl_vld = 1;
	rule.action.stage2_ctrl = 0;
	rule.action.stage2_ctrl_vld = 1;
	rule.action.stage3_ctrl = 1;
	rule.action.stage3_ctrl_vld = 1;
	rule.action.keep_orig_pkt = 0;
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.mc = 0;
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_L3FE_MC_DEEPQ)
	rule.action.mcgid = AAL_LPORT_ETH_NI6;	// enable the loopback port and deepq data path
	rule.action.deepq = 1;
#else
	rule.action.mcgid = AAL_LPORT_L3_LAN;
	rule.action.deepq = 0;
#endif
	rule.action.permit = 1;
	rule.action.dpid_pri = 1;
	rule.action.dpid_vld = 1;
	rule.action.no_drop = 1;
	rule.action.no_drop_vld = 1;

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_MCAST;

	aal_l3_cls_add(0, 0, &rule, &rslt_idx); /* Add for WAN port */
#if defined(CONFIG_LUNA_G3_SERIES)
	aal_l3_cls_add(0, 1, &rule, &rslt_idx); /* Add for LAN Profile */
#endif	//CONFIG_LUNA_G3_SERIES

#endif

	return AAL_E_OK;
}

void aal_l3_cls_tbl_exit(void)
{
	__aal_table_unregister(AAL_TABLE_L3_CLS_KEY);
	__aal_table_unregister(AAL_TABLE_L3_CLS_FIB);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	__aal_table_unregister(AAL_TABLE_8279_L3_CLS_KEY);
	__aal_table_unregister(AAL_TABLE_8279_L3_CLS_FIB);
#endif
}

ca_int32_t aal_l3_cls_key_clean(cls_key_t *key)
{
	if (key == NULL)
		return AAL_E_NULLPTR;

	/* ignore all fields */
	memset(key, 0xff, sizeof(cls_key_t));
	memset(&key->ip_sa, 0, sizeof(cls_ip_addr_t));
	memset(&key->ip_da, 0, sizeof(cls_ip_addr_t));
	memset(&key->ip_addr, 0, sizeof(cls_ip_addr_t));
	memset(&key->dest_addr.dest_ip_addr, 0, sizeof(cls_ip_addr_t));
	key->msk_ip_sa_h = 0;
	key->msk_ip_sa_l = 0;
	key->msk_ip_da_h = 0;
	key->msk_ip_da_l = 0;
	key->msk_ip_addr_h = 0;
	key->msk_ip_addr_l = 0;

	return AAL_E_OK;
}

ca_int32_t aal_l3_cls_add(ca_device_id_t device_id, ca_uint32_t profile_id, l3_cls_rule_t *rule, ca_uint32_t *rslt_idx)
{
	ca_int32_t rc=0;
	l3_cls_key_type_t key_type;
	l3_cls_fib_mode_t fib_mode;
	ca_uint32_t key_tbl_idx, fib_tbl_idx;
	ca_uint8_t key_idx, i;
	l3_cls_key_t key_data;
	cls_fib_t fib_data;
	aal_table_t *key_table = &l3_cls_key_table;
	aal_table_t *fib_table = &l3_cls_fib_table;

	if (profile_id >= L3_CLS_PROFILE_MAX)
		return AAL_E_OUTRANGE;
	if (rule == NULL || rslt_idx == NULL)
		return AAL_E_NULLPTR;
	if (rule->pri > CL_RUL_PRIO_MAX)
		return AAL_E_OUTRANGE;
	if (rule->rslt_type > CL_RSLT_TYPE_1)
		return AAL_E_OUTRANGE;

	/* determine fib mode */
	if (AAL_E_OK != __l3_cls_fib_mode(&rule->action, &fib_mode))
		return AAL_E_INVALID;

	/* determine key type */
	if (AAL_E_OK != __l3_cls_key_type(&rule->key, fib_mode, &key_type))
		return AAL_E_INVALID;

	spin_lock(&cls_lock);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1) {
		key_table = &hgu_8279_l3_cls_key_table;
		fib_table = &hgu_8279_l3_cls_fib_table;
	}
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3LITE)
	//Bug 50510 - In L3FE CLS, the MAC_DA_AN_HIT field implementation is inverted with spec definition
	if(rule->key.msk_mac_da_an_hit==0) {
		rule->key.mac_da_an_hit ^= 0x1;
	}
#endif
#endif

	/* find a empty key entry */
	memset(&key_data, 0, sizeof(l3_cls_key_t));
	if (AAL_E_OK != __l3_cls_empty_key_entry_find(key_table, profile_id, key_type, rule->rslt_type, rule->pri, &key_tbl_idx, &key_idx, key_data.cls_key_info.key_tbl_data)) {
		rc = AAL_E_TBLFULL;
		goto ERROR;
	}

	/* debug message */
	if (ca_aal_debug & AAL_DBG_L3CLS)
		printk("%s: device_id = %d, key_tbl_idx = %d, key_idx = %d\n", __func__, device_id, key_tbl_idx, key_idx);

	/* update rule.key */
	if (AAL_E_OK != __l3_cls_key_entry_update(rule, key_type, key_tbl_idx, key_idx, &key_data)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}
	/* update rule.action */
	memset(&fib_data, 0, sizeof(cls_fib_t));
	if (AAL_E_OK != __l3_cls_fib_entry_update(&rule->action, fib_mode, &fib_data)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}

	/* get fib index */
	if (AAL_E_OK != __l3_cls_fib_idx_get(key_table, key_type, key_tbl_idx, key_idx, &fib_tbl_idx)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}

	/* add fib entries */
	if (key_type == CL_IF_ID_KEY) {
		/* set fib_format_sel */
		fib_data.cls_fib_data.fib_tbl_data[0].fib_format_sel = fib_mode;

		/* short fib */
		aal_entry_del(fib_table->table_id, fib_tbl_idx);
		if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, &fib_data.cls_fib_data.fib_tbl_data[0], fib_tbl_idx)))
			goto ERROR;
	} else {
		/* set fib_format_sel */
		fib_data.cls_fib_data.fib_tbl_data[1].fib_format_sel = fib_mode;

		/* standard fib map to 2 fib entries */
		for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++)
			aal_entry_del(fib_table->table_id, fib_tbl_idx+i);
		for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, &fib_data.cls_fib_data.fib_tbl_data[i], fib_tbl_idx+i)))
				goto ERROR;
		}
	}

	/* add key entries */
	if (key_type == CL_FULL_KEY) {
		/* long key has 2 entries */
		for (i=0; i<L3_CLS_LONG_KEY_ENTRY_NUM; i++)
			aal_entry_del(key_table->table_id, key_tbl_idx+i);
		for (i=0; i<L3_CLS_LONG_KEY_ENTRY_NUM; i++) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(key_table->table_id, &key_data.cls_key_info.key_tbl_data[i], key_tbl_idx+i)))
				goto ERROR;
		}
	} else {
		aal_entry_del(key_table->table_id, key_tbl_idx);
		if (AAL_E_OK != (rc = aal_entry_add_by_idx(key_table->table_id, &key_data.cls_key_info.key_tbl_data[0], key_tbl_idx)))
			goto ERROR;
	}

	*rslt_idx = CLS_RUL_ID(device_id, key_type, fib_mode, key_tbl_idx, key_idx);

	spin_unlock(&cls_lock);
	return AAL_E_OK;

ERROR:
	spin_unlock(&cls_lock);
	return rc;
}

ca_int32_t aal_l3_cls_update(ca_device_id_t device_id, ca_uint32_t profile_id, l3_cls_rule_t *rule, ca_uint32_t rslt_idx)
{
	ca_int32_t rc=0;
	l3_cls_key_type_t key_type;
	l3_cls_fib_mode_t fib_mode;
	ca_uint32_t key_tbl_idx, fib_tbl_idx;
	ca_uint8_t key_idx, i;
	l3_cls_key_t key_data;
	cls_fib_t fib_data;
	aal_table_t *key_table = &l3_cls_key_table;
	aal_table_t *fib_table = &l3_cls_fib_table;

	if (profile_id >= L3_CLS_PROFILE_MAX)
		return AAL_E_OUTRANGE;
	if (rule == NULL)
		return AAL_E_NULLPTR;

	/* determine fib mode */
	if (AAL_E_OK != __l3_cls_fib_mode(&rule->action, &fib_mode))
		return AAL_E_INVALID;

	/* determine key type */
	if (AAL_E_OK != __l3_cls_key_type(&rule->key, fib_mode, &key_type))
		return AAL_E_INVALID;

	spin_lock(&cls_lock);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1) {
		key_table = &hgu_8279_l3_cls_key_table;
		fib_table = &hgu_8279_l3_cls_fib_table;
	}
#endif

	if ((key_type != CLS_KEY_TYPE(rslt_idx)) ||
	    (fib_mode != CLS_FIB_MODE(rslt_idx))) {
		rc = AAL_E_OPNOTSUPP;
		goto ERROR;
	}

	key_tbl_idx = CLS_TBL_ID(rslt_idx);
	key_idx = CLS_KEY_ID(rslt_idx);
	key_type = CLS_KEY_TYPE(rslt_idx);
	fib_mode = CLS_FIB_MODE(rslt_idx);

	/* debug message */
	if (ca_aal_debug & AAL_DBG_L3CLS)
		printk("%s: key_tbl_idx = %d, key_idx = %d\n", __func__, key_tbl_idx, key_idx);

	/* get key entry */
	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, key_tbl_idx, key_data.cls_key_info.key_tbl_data)) {
		rc = AAL_E_TBLFULL;
		goto ERROR;
	}

	if ((rule->pri != key_data.cls_key_info.key_tbl_data[0].cls_pri) ||
	    (rule->rslt_type != key_data.cls_key_info.key_tbl_data[0].cls_rslt_type)) {
		rc = AAL_E_OPNOTSUPP;
		goto ERROR;
	}

	/* update rule.key */
	if (AAL_E_OK != __l3_cls_key_entry_update(rule, key_type, key_tbl_idx, key_idx, &key_data)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}

	/* update rule.action */
	memset(&fib_data, 0, sizeof(cls_fib_t));
	if (AAL_E_OK != __l3_cls_fib_entry_update(&rule->action, fib_mode, &fib_data)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}

	/* get fib index */
	if (AAL_E_OK != __l3_cls_fib_idx_get(key_table, key_type, key_tbl_idx, key_idx, &fib_tbl_idx)) {
		rc = AAL_E_INVALID;
		goto ERROR;
	}

	/* update fib entries */
	if (key_type == CL_IF_ID_KEY) {
		/* set fib_format_sel */
		fib_data.cls_fib_data.fib_tbl_data[0].fib_format_sel = fib_mode;

		/* short fib */
		if (AAL_E_OK != (rc = aal_entry_set(fib_table->table_id, fib_tbl_idx, &fib_data.cls_fib_data.fib_tbl_data[0])))
			goto ERROR;
	} else {
		/* set fib_format_sel */
		fib_data.cls_fib_data.fib_tbl_data[1].fib_format_sel = fib_mode;

		/* standard fib map to 2 fib entries */
		for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
			if (AAL_E_OK != (rc = aal_entry_set(fib_table->table_id, fib_tbl_idx+i, &fib_data.cls_fib_data.fib_tbl_data[i])))
				goto ERROR;
		}
	}

	spin_unlock(&cls_lock);
	return AAL_E_OK;

ERROR:
	spin_unlock(&cls_lock);
	return rc;
}

ca_int32_t aal_l3_cls_delete(ca_device_id_t device_id, ca_uint32_t profile_id, ca_uint32_t rslt_idx)
{
	ca_int32_t rc, i;
	ca_uint32_t key_tbl_idx, key_idx, fib_tbl_idx;
	l3_cls_key_type_t key_type;
	l3_cls_fib_mode_t fib_mode;
	l3_cls_key_tbl_entry_t key_data;
	aal_table_t *key_table = &l3_cls_key_table;
	aal_table_t *fib_table = &l3_cls_fib_table;

	key_tbl_idx = CLS_TBL_ID(rslt_idx);
	key_idx = CLS_KEY_ID(rslt_idx);
	key_type = CLS_KEY_TYPE(rslt_idx);
	fib_mode = CLS_FIB_MODE(rslt_idx);

	spin_lock(&cls_lock);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1) {
		key_table = &hgu_8279_l3_cls_key_table;
		fib_table = &hgu_8279_l3_cls_fib_table;
	}
#endif

	/* get fib index */
	if (AAL_E_OK != (rc = __l3_cls_fib_idx_get(key_table, key_type, key_tbl_idx, key_idx, &fib_tbl_idx)))
		goto ERROR;

	/* get key entry */
	if (AAL_E_OK != (rc = aal_entry_get_by_idx(key_table->table_id, key_tbl_idx, &key_data)))
		goto ERROR;

	/* del key entries */
	switch(key_type) {
		case CL_IF_ID_KEY:
		case CL_IPV4_SHORT_KEY:
		case CL_SPCL_PKT_KEY:
			/* set valid bit to 0 */
			CLS_KEY_VLD_CLR(key_data.cls_key_type, key_idx, key_data.cls_key_valid);
			if (key_data.cls_key_valid) {
				/* update hw key */
				if (AAL_E_OK != (rc = aal_entry_set(key_table->table_id, key_tbl_idx, &key_data)))
					goto ERROR;
			} else {
				/* del a hw key */
				if (AAL_E_OK != (rc = aal_entry_del(key_table->table_id, key_tbl_idx)))
					goto ERROR;
			}
			break;
		case CL_FULL_KEY:
			/* long key has 2 entries */
			for (i=0; i<L3_CLS_LONG_KEY_ENTRY_NUM; i++) {
				if (AAL_E_OK != (rc = aal_entry_del(key_table->table_id, key_tbl_idx+i)))
					goto ERROR;
			}
			break;
		default:
			/* del a hw key */
			if (AAL_E_OK != (rc = aal_entry_del(key_table->table_id, key_tbl_idx)))
				goto ERROR;
	}

	/* del fib entries */
	if (key_type == CL_IF_ID_KEY) {
		/* short fib */
		if (AAL_E_OK != (rc = aal_entry_del(fib_table->table_id, fib_tbl_idx)))
			goto ERROR;
	} else {
		/* standard fib map to 2 fib entries */
		for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
			if (AAL_E_OK != (rc = aal_entry_del(fib_table->table_id, fib_tbl_idx+i)))
				goto ERROR;
		}
	}

	spin_unlock(&cls_lock);
	return AAL_E_OK;

ERROR:
	spin_unlock(&cls_lock);
	return rc;
}

ca_int32_t aal_l3_cls_default_set(ca_device_id_t device_id, ca_uint32_t profile_id, cls_rslt_type_t rslt_type, cls_action_t *action)
{
	ca_int32_t rc, i;
	l3_cls_fib_mode_t fib_mode;
	ca_uint32_t fib_tbl_idx;
	cls_fib_t fib_data;
	aal_table_t *fib_table = &l3_cls_fib_table;

	if (profile_id >= L3_CLS_PROFILE_MAX)
		return AAL_E_OUTRANGE;
	if (rslt_type > CL_RSLT_TYPE_1)
		return AAL_E_OUTRANGE;
	if (action == NULL)
		return AAL_E_NULLPTR;

	/* determine fib mode */
	if (AAL_E_OK != __l3_cls_fib_mode(action, &fib_mode))
		return AAL_E_INVALID;

	/* update action */
	memset(&fib_data, 0, sizeof(cls_fib_t));
	if (AAL_E_OK != __l3_cls_fib_entry_update(action, fib_mode, &fib_data))
		return AAL_E_INVALID;

	/* update fib format */
	fib_data.cls_fib_data.fib_tbl_data[1].fib_format_sel = fib_mode;

	spin_lock(&cls_lock);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (device_id == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	/* get fib index */
	fib_tbl_idx = (fib_table->max_entry - L3_CLS_FIB_TBL_DEF_ENTRY_MAX) | (profile_id << 2) | (rslt_type << 1);

	/* del hw fib */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++)
		aal_entry_del(fib_table->table_id, fib_tbl_idx+i);

	/* set a hw fib */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
		if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, &fib_data.cls_fib_data.fib_tbl_data[i], fib_tbl_idx+i))) {
			spin_unlock(&cls_lock);
			return rc;
		}
	}

	spin_unlock(&cls_lock);
	return AAL_E_OK;
}

ca_int32_t aal_l3_spcl_pkt_init(void)
{
	int ret = AAL_E_OK;
	l3_cls_rule_t rule;
	l3_cam_ethertype_tbl_entry_t ethertype;
	ca_uint32_t rslt_idx, cam_rslt_idx;

	ret = aal_scfg_read(CFG_ID_DATAPATH_MODE, 1, &datapath_mode);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: ERROR! Cannot decide datapath mode. (ret %d)\n", __func__, ret);
	}

	if (datapath_mode == 0) {
		/* router/gateway mode */

	/**********************************
	*    Broadcast from WAN to CPU port (0x10)
	***********************************/
	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
	rule.key.mac_da_bc = 1;
	rule.key.msk_mac_da_bc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_CPU_0;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 7;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_SCPL_PKT;
	ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: Broadcast from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for Broadcast from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	*    PPPoE Discovery packet from WAN to CPU port (0x10)
	***********************************/
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8863;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add Ethertype 0x8863 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
	rule.key.lspid = AAL_LPORT_L3_WAN;
	rule.key.msk_lspid = 0;
	rule.key.ethertype_enc = cam_rslt_idx + 1;
	rule.key.msk_ethertype_enc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_CPU_0;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 1;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
	ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: PPPoE Discovery packet from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for PPPoE Discovery from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	*    PPPoE Discovery packet from LAN to CPU port (0x11)
	***********************************/
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8863;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add Ethertype 0x8863 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
	rule.key.lspid = AAL_LPORT_L3_LAN;
	rule.key.msk_lspid = 0;
	rule.key.ethertype_enc = cam_rslt_idx + 1;
	rule.key.msk_ethertype_enc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_CPU_1;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 1;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
	ret = aal_l3_cls_add(0, 1, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: PPPoE Discovery packet from LAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for PPPoE Discovery from LAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	*    PPPoE Session packet from WAN to CPU port (0x10)
	***********************************/
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8864;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add Ethertype 0x8864 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
		rule.key.lspid = AAL_LPORT_L3_WAN;
		rule.key.msk_lspid = 0;
	rule.key.ethertype_enc = cam_rslt_idx + 1;
	rule.key.msk_ethertype_enc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_CPU_0;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 7;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
	ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: PPPoE Session packet from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for PPPoE Session from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	*    PPPoE Session packet from LAN to CPU port (0x11)
	***********************************/
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x8864;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add Ethertype 0x8864 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
		rule.key.lspid = AAL_LPORT_L3_LAN;
		rule.key.msk_lspid = 0;
	rule.key.ethertype_enc = cam_rslt_idx + 1;
	rule.key.msk_ethertype_enc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_ARCH_RTL8198F)
	rule.action.mcgid = AAL_LPORT_CPU_0;
	#else
	rule.action.mcgid = AAL_LPORT_CPU_1;
	#endif
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 7;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
	ret = aal_l3_cls_add(0, 1, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: PPPoE Session packet from LAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for PPPoE Session from LAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	 *    ARP packet from WAN to CPU port (0x10)
	 ***********************************/
	memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
	ethertype.vld = 1;
	ethertype.ethertype = 0x0806;
	ret = aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &cam_rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "%s: Fail to add Ethertype 0x0806 into CAM table, ret = %d\n", __func__, ret);
		return ret;
	}

	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
	rule.key.ethertype_enc = cam_rslt_idx + 1;
	rule.key.msk_ethertype_enc = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_CPU_0;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;
	rule.action.cos = 7;
	rule.action.cos_sel = 1; /* use FIB result */

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
	ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: ARP packet from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for ARP from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	/**********************************
	*    L2UC from WAN to L2FE (0x19)
		 * This is for hybrid mode (router + bridge).
		 * All L2UC packets are directed to L2FE and bypass other tables in L3FE.
		 ***********************************/
		memset(&rule, 0, sizeof(l3_cls_rule_t));
		aal_l3_cls_key_clean(&rule.key);
		/* set cls key */
		rule.key.mac_da_an_sel = 0; /* include msk_mac_da_bc FF-FF-FF-FF-FF-FF */
		rule.key.msk_mac_da_an_sel = 0;
		/* set cls action */
		rule.action.keep_orig_pkt_vld = 1;
		rule.action.keep_orig_pkt = 1;
		rule.action.mc = 0;
		rule.action.mcgid = AAL_LPORT_L3_LAN;
		rule.action.dpid_pri = 1;
		rule.action.permit = 1;
		rule.action.dpid_vld = 1;
		rule.action.t2_ctrl_vld = 1;
		rule.action.t2_ctrl = T2_CTRL_BYPASS;
		rule.action.t3_ctrl_vld = 1;
		rule.action.t3_ctrl = T3_CTRL_BYPASS;
		rule.action.t4_ctrl_vld = 1;
		rule.action.t4_ctrl = T4_CTRL_BYPASS;
		rule.action.t5_ctrl_vld = 1;
		rule.action.t5_ctrl = T5_CTRL_BYPASS;

		rule.rslt_type = CL_RSLT_TYPE_0;
		rule.pri = CL_RUL_PRIO_MIN;
		ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: L2UC from WAN to L2FE initialization fail, ret = %d\n", __func__, ret);
			return ret;
		}
		printk(KERN_INFO "%s: CLS rule for L2UC from WAN to L2FE is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

		/**********************************
		 * TCP SYN/FIN/RST packet from WAN to CPU port (0x10)
		 ***********************************/
		memset(&rule, 0, sizeof(l3_cls_rule_t));
		aal_l3_cls_key_clean(&rule.key);
		/* set cls key */
		rule.key.tcp_rdp_ctrl = 0x7; /* RST, SYN, FIN */
		rule.key.msk_tcp_rdp_ctrl = 0;
		/* set cls action */
		rule.action.keep_orig_pkt_vld = 1;
		rule.action.keep_orig_pkt = 1;
		rule.action.mc = 0;
		rule.action.mcgid = AAL_LPORT_CPU_0;
		rule.action.dpid_pri = 1;
		rule.action.permit = 1;
		rule.action.dpid_vld = 1;
		rule.action.t2_ctrl_vld = 1;
		rule.action.t2_ctrl = T2_CTRL_BYPASS;
		rule.action.t3_ctrl_vld = 1;
		rule.action.t3_ctrl = T3_CTRL_BYPASS;
		rule.action.t4_ctrl_vld = 1;
		rule.action.t4_ctrl = T4_CTRL_BYPASS;
		rule.action.t5_ctrl_vld = 1;
		rule.action.t5_ctrl = T5_CTRL_BYPASS;
		rule.action.cos = 6;
		rule.action.cos_sel = 1; /* use FIB result */

		rule.rslt_type = CL_RSLT_TYPE_0;
		rule.pri = CL_RUL_PRIO_L3_SCPL_PKT;
		ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: TCP SYN/FIN/RST from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
			return ret;
		}
		printk(KERN_INFO "%s: CLS rule for TCP SYN/FIN/RST from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

		/**********************************
		 * TCP SYN/FIN/RST packet from LAN to CPU port (0x11)
		 ***********************************/
		memset(&rule, 0, sizeof(l3_cls_rule_t));
		aal_l3_cls_key_clean(&rule.key);
		/* set cls key */
		rule.key.tcp_rdp_ctrl = 0x7; /* RST, SYN, FIN */
		rule.key.msk_tcp_rdp_ctrl = 0;
		/* set cls action */
		rule.action.keep_orig_pkt_vld = 1;
		rule.action.keep_orig_pkt = 1;
		rule.action.mc = 0;
		rule.action.mcgid = AAL_LPORT_CPU_1;
		rule.action.dpid_pri = 1;
		rule.action.permit = 1;
		rule.action.dpid_vld = 1;
		rule.action.t2_ctrl_vld = 1;
		rule.action.t2_ctrl = T2_CTRL_BYPASS;
		rule.action.t3_ctrl_vld = 1;
		rule.action.t3_ctrl = T3_CTRL_BYPASS;
		rule.action.t4_ctrl_vld = 1;
		rule.action.t4_ctrl = T4_CTRL_BYPASS;
		rule.action.t5_ctrl_vld = 1;
		rule.action.t5_ctrl = T5_CTRL_BYPASS;
		rule.action.cos = 6;
		rule.action.cos_sel = 1; /* use FIB result */

		rule.rslt_type = CL_RSLT_TYPE_0;
		rule.pri = CL_RUL_PRIO_L3_SCPL_PKT;
		ret = aal_l3_cls_add(0, 1, &rule, &rslt_idx);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: TCP SYN/FIN/RST from LAN to CPU initialization fail, ret = %d\n", __func__, ret);
			return ret;
		}
		printk(KERN_INFO "%s: CLS rule for TCP SYN/FIN/RST from LAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

	} else if (datapath_mode == 1) {
		/* L2 switching mode */
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		if (l3fe_hash_lite_profile == 2) { /* GPON */
			/**********************************
			 * PPPoE control packet from WAN to CPU port (0x10)
			 *
			 * IMPORTANT: User needs to handle PPPoE control packets by self.
			 ***********************************/

			l3_cam_ppp_prot_tbl_entry_t ppp_prot;

			/*** PPP protocol 0xc021 ***/
			memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
			ppp_prot.vld = 1;
			ppp_prot.ppp_prot = 0xc021;
			ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: Fail to add PPP Protocol 0xc021 into CAM table, ret = %d\n", __func__, ret);
				return ret;
			}

			memset(&rule, 0, sizeof(l3_cls_rule_t));
			aal_l3_cls_key_clean(&rule.key);
			/* set cls key */
			rule.key.ppp_protocol_enc = cam_rslt_idx + 1;
			rule.key.msk_ppp_protocol = 0;
			rule.key.mac_da_an_sel = 0; /* include msk_mac_da_bc FF-FF-FF-FF-FF-FF */
			rule.key.msk_mac_da_an_sel = 0;
			/* set cls action */
			rule.action.keep_orig_pkt_vld = 1;
			rule.action.keep_orig_pkt = 1;
			rule.action.mc = 0;
			rule.action.mcgid = AAL_LPORT_CPU_0;
			rule.action.dpid_pri = 1;
			rule.action.permit = 1;
			rule.action.dpid_vld = 1;
			rule.action.t2_ctrl_vld = 1;
			rule.action.t2_ctrl = T2_CTRL_BYPASS;
			rule.action.t3_ctrl_vld = 1;
			rule.action.t3_ctrl = T3_CTRL_BYPASS;
			rule.action.t4_ctrl_vld = 1;
			rule.action.t4_ctrl = T4_CTRL_BYPASS;
			rule.action.t5_ctrl_vld = 1;
			rule.action.t5_ctrl = T5_CTRL_BYPASS;

			rule.rslt_type = CL_RSLT_TYPE_0;
			rule.pri = CL_RUL_PRIO_L3_INTF_CPU;
			ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: PPPoE control packet (ppp_prot 0xc021) from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
				return ret;
			}
			printk(KERN_INFO "%s: CLS rule for PPPoE control packet (ppp_prot 0xc021) from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

			/*** PPP protocol 0xc023 ***/
			ppp_prot.ppp_prot = 0xc023;
			ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: Fail to add PPP Protocol 0xc023 into CAM table, ret = %d\n", __func__, ret);
				return ret;
			}

			rule.key.ppp_protocol_enc = cam_rslt_idx + 1;
			ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: PPPoE control packet (ppp_prot 0xc023) from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
				return ret;
			}
			printk(KERN_INFO "%s: CLS rule for PPPoE control packet (ppp_prot 0xc023) from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

			/*** PPP protocol 0xc223 ***/
			ppp_prot.ppp_prot = 0xc223;
			ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: Fail to add PPP Protocol 0xc223 into CAM table, ret = %d\n", __func__, ret);
				return ret;
			}

			rule.key.ppp_protocol_enc = cam_rslt_idx + 1;
			ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: PPPoE control packet (ppp_prot 0xc223) from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
				return ret;
			}
			printk(KERN_INFO "%s: CLS rule for PPPoE control packet (ppp_prot 0xc223) from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

			/*** PPP protocol 0x8021 ***/
			ppp_prot.ppp_prot = 0x8021;
			ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: Fail to add PPP Protocol 0x8021 into CAM table, ret = %d\n", __func__, ret);
				return ret;
			}

			rule.key.ppp_protocol_enc = cam_rslt_idx + 1;
			ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: PPPoE control packet (ppp_prot 0x8021) from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
				return ret;
			}
			printk(KERN_INFO "%s: CLS rule for PPPoE control packet (ppp_prot 0x8021) from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));

			/*** PPP protocol 0x8057 ***/
			ppp_prot.ppp_prot = 0x8057;
			ret = aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: Fail to add PPP Protocol 0x8057 into CAM table, ret = %d\n", __func__, ret);
				return ret;
			}

			rule.key.ppp_protocol_enc = cam_rslt_idx + 1;
			ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
			if (ret != AAL_E_OK) {
				printk(KERN_ERR "%s: PPPoE control packet (ppp_prot 0x8057) from WAN to CPU initialization fail, ret = %d\n", __func__, ret);
				return ret;
			}
			printk(KERN_INFO "%s: CLS rule for PPPoE control packet (ppp_prot 0x8057) from WAN to CPU port is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));
		} /* l3fe_hash_lite_profile == 2 */
#else /* ! CONFIG_ARCH_CORTINA_SATURN_SFU */
		/**********************************
		 * L2UC from WAN to L2FE (0x19)
		 * All L2UC packets are directed to L2FE and bypass other tables in L3FE.
		 *
		 * NOTE: Saturn SFU handles downstream L2UC in HashLite table instead of CLS table.
	***********************************/
	memset(&rule, 0, sizeof(l3_cls_rule_t));
	aal_l3_cls_key_clean(&rule.key);
	/* set cls key */
		rule.key.mac_da_an_sel = 0; /* include msk_mac_da_bc FF-FF-FF-FF-FF-FF */
	rule.key.msk_mac_da_an_sel = 0;
	/* set cls action */
	rule.action.keep_orig_pkt_vld = 1;
	rule.action.keep_orig_pkt = 1;
	rule.action.mc = 0;
	rule.action.mcgid = AAL_LPORT_L3_LAN;
	rule.action.dpid_pri = 1;
	rule.action.permit = 1;
	rule.action.dpid_vld = 1;
	rule.action.t2_ctrl_vld = 1;
	rule.action.t2_ctrl = T2_CTRL_BYPASS;
	rule.action.t3_ctrl_vld = 1;
	rule.action.t3_ctrl = T3_CTRL_BYPASS;
	rule.action.t4_ctrl_vld = 1;
	rule.action.t4_ctrl = T4_CTRL_BYPASS;
	rule.action.t5_ctrl_vld = 1;
	rule.action.t5_ctrl = T5_CTRL_BYPASS;

	rule.rslt_type = CL_RSLT_TYPE_0;
	rule.pri = CL_RUL_PRIO_MIN;
	ret = aal_l3_cls_add(0, 0, &rule, &rslt_idx);
	if (ret != AAL_E_OK) {
			printk(KERN_ERR "%s: L2UC from WAN to L2FE initialization fail, ret = %d\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: CLS rule for L2UC from WAN to L2FE is created, entry_idx %u.\n", __func__, CLS_TBL_ID(rslt_idx));
#endif /* CONFIG_ARCH_CORTINA_SATURN_SFU */
	}

	return AAL_E_OK;
}

/*******************************
 *   AAL CLI debug functions
 *******************************/
ca_status_t aal_l3_cls_if_id_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_if_id_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_if_id_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_if_id_key_t *entry)
{
	cl_if_id_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_ipv4_short_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_ipv4_short_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_ipv4_short_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_ipv4_short_key_t *entry)
{
	cl_ipv4_short_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_spcl_pkt_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_spcl_pkt_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_spcl_pkt_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_spcl_pkt_key_t *entry)
{
	cl_spcl_pkt_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_tunnel_id_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_ipv4_tunnel_id_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_tunnel_id_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_ipv4_tunnel_id_key_t *entry)
{
	cl_ipv4_tunnel_id_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_ipv6_short_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_ipv6_short_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_ipv6_short_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_ipv6_short_key_t *entry)
{
	cl_ipv6_short_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_mcst_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_mcst_key_t *entry)
{
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, entry))
		return hw_entry_get(key_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_mcst_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_mcst_key_t *entry)
{
	cl_mcst_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(key_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(key_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_full_key_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cl_full_key_t *entry)
{
	ca_status_t rc;
	ca_uint8_t i;
	l3_cls_key_t *p_key = (l3_cls_key_t *)entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	for (i=0; i<2; i++) {
		if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx+i, &p_key->cls_key_info.key_tbl_data[i]))
			if (AAL_E_OK != (rc = hw_entry_get(key_table, entry_idx+i, &p_key->cls_key_info.key_tbl_data[i])))
				return rc;
	}
	return AAL_E_OK;
}

ca_status_t aal_l3_cls_full_key_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cl_full_key_t *entry)
{
	ca_status_t rc;
	ca_uint8_t i;
	l3_cls_key_t *p_key = (l3_cls_key_t *)entry;
	cl_full_key_t tmp_entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	for (i=0; i<L3_CLS_LONG_KEY_ENTRY_NUM; i++) {
		if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx+i, &tmp_entry)) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(key_table->table_id, &p_key->cls_key_info.key_tbl_data[i], entry_idx+i)))
				return rc;
		} else {
			if (AAL_E_OK != (rc = aal_entry_set(key_table->table_id, entry_idx+i, &p_key->cls_key_info.key_tbl_data[i])))
				return rc;
		}
	}
	return AAL_E_OK;
}

ca_status_t aal_l3_cls_key_clean_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cls_key_type_t *key_type)
{
	l3_cls_key_t entry;
	aal_table_t *key_table = &l3_cls_key_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		key_table = &hgu_8279_l3_cls_key_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(key_table->table_id, entry_idx, &entry))
		return hw_entry_get(key_table, entry_idx, &entry);

	*key_type = entry.cls_key_info.key_tbl_data[0].cls_key_type;

	return AAL_E_OK;
}

#define CLS_KEY_CTRL_CLEAN(o, key_type) {\
	entry.cls_key_info.o.cls_entry_mode = 0;\
	entry.cls_key_info.o.cls_pri = 0;\
	entry.cls_key_info.o.cls_rslt_type = 0;\
	entry.cls_key_info.o.cls_key_type = key_type;\
	entry.cls_key_info.o.cls_key_valid = 0;\
} while(0)

ca_status_t aal_l3_cls_key_clean_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cls_key_type_t key_type)
{
	l3_cls_key_t entry;

	/* ignore all fields */
	memset(&entry, 0xff, sizeof(l3_cls_key_t));

	switch(key_type) {
		case CL_IF_ID_KEY:
			/* ignore all fields */
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_if_id_key_set(0, entry_idx, &entry.cls_key_info.cl_if_id_key);
		case CL_IPV4_TUNNEL_ID_KEY:
			/* ignore all fields */
			entry.cls_key_info.cl_ipv4_tunnel_key.msk_ip_sa_l = 0;
			entry.cls_key_info.cl_ipv4_tunnel_key.msk_ip_da_l = 0;
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_tunnel_id_key_set(0, entry_idx, &entry.cls_key_info.cl_ipv4_tunnel_key);
		case CL_IPV4_SHORT_KEY:
			/* ignore all fields */
			entry.cls_key_info.cl_ipv4_short_key.msk_ip_addr_l_0 = 0;
			entry.cls_key_info.cl_ipv4_short_key.msk_ip_addr_l_1 = 0;
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_ipv4_short_key_set(0, entry_idx, &entry.cls_key_info.cl_ipv4_short_key);
		case CL_IPV6_SHORT_KEY:
			/* ignore all fields */
			entry.cls_key_info.cl_ipv6_short_key.msk_ip_addr_h = 0;
			entry.cls_key_info.cl_ipv6_short_key.msk_ip_addr_l = 0;
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_ipv6_short_key_set(0, entry_idx, &entry.cls_key_info.cl_ipv6_short_key);
		case CL_SPCL_PKT_KEY:
			/* ignore all fields */
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_spcl_pkt_key_set(0, entry_idx, &entry.cls_key_info.cl_spcl_pkt_key);
		case CL_MCST_KEY:
			/* ignore all fields */
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_mcst_key_set(0, entry_idx, &entry.cls_key_info.cl_mcst_key);
		case CL_FULL_KEY:
			/* ignore all fields */
			entry.cls_key_info.cl_full_key.msk_ip_sa_h = 0;
			entry.cls_key_info.cl_full_key.msk_ip_sa_l = 0;
			entry.cls_key_info.cl_full_key.msk_ip_da_h = 0;
			entry.cls_key_info.cl_full_key.msk_ip_da_l = 0;
			CLS_KEY_CTRL_CLEAN(key_tbl_data[0], key_type);
			return aal_l3_cls_full_key_set(0, entry_idx, &entry.cls_key_info.cl_full_key);
		default:
			return AAL_E_INVALID;
	}

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_0_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cls_fib_mod_0_t *entry)
{
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx, entry))
		return hw_entry_get(fib_table, entry_idx, entry);

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_0_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cls_fib_mod_0_t *entry)
{
	cls_fib_mod_0_t tmp_entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(fib_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(fib_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_fib_1_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cls_fib_mod_1_t *entry)
{
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx, entry))
		return hw_entry_get(fib_table, entry_idx, entry);

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_fib_1_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cls_fib_mod_1_t *entry)
{
	cls_fib_mod_1_t tmp_entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx, &tmp_entry))
		return aal_entry_add_by_idx(fib_table->table_id, entry, entry_idx);
	else
		return aal_entry_set(fib_table->table_id, entry_idx, entry);
}

ca_status_t aal_l3_cls_fib_2_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cls_fib_mod_2_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t *p_fib = (l3_cls_fib_tbl_entry_t *)entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, p_fib))
			if (AAL_E_OK != (rc = hw_entry_get(fib_table, entry_idx+i, p_fib)))
				return rc;
	}

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_2_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cls_fib_mod_2_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t tmp_entry, *p_fib;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	p_fib = (l3_cls_fib_tbl_entry_t *)entry;

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, &tmp_entry)) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, p_fib, entry_idx+i)))
				return rc;
		} else {
			if (AAL_E_OK != (rc = aal_entry_set(fib_table->table_id, entry_idx+i, p_fib)))
				return rc;
		}
	}

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_3_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cls_fib_mod_3_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t *p_fib = (l3_cls_fib_tbl_entry_t *)entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, p_fib))
			if (AAL_E_OK != (rc = hw_entry_get(fib_table, entry_idx+i, p_fib)))
				return rc;
	}

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_fib_3_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cls_fib_mod_3_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t *p_fib;
	cls_fib_mod_3_t tmp_entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	p_fib = (l3_cls_fib_tbl_entry_t *)entry;

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, &tmp_entry)) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, p_fib, entry_idx+i)))
				return rc;
		} else {
			if (AAL_E_OK != (rc = aal_entry_set(fib_table->table_id, entry_idx+i, p_fib)))
				return rc;
		}
	}

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_4_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT cls_fib_mod_4_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t *p_fib = (l3_cls_fib_tbl_entry_t *)entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, p_fib))
			if (AAL_E_OK != (rc = hw_entry_get(fib_table, entry_idx+i, p_fib)))
				return rc;
	}

	return AAL_E_OK;
}


ca_status_t aal_l3_cls_fib_4_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN cls_fib_mod_4_t *entry)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t i;
	l3_cls_fib_tbl_entry_t *p_fib;
	cls_fib_mod_4_t tmp_entry;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	p_fib = (l3_cls_fib_tbl_entry_t *)entry;

	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++, p_fib++) {
		if (AAL_E_OK != aal_entry_get_by_idx(fib_table->table_id, entry_idx+i, &tmp_entry)) {
			if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, p_fib, entry_idx+i)))
				return rc;
		} else {
			if (AAL_E_OK != (rc = aal_entry_set(fib_table->table_id, entry_idx+i, p_fib)))
				return rc;
		}
	}

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_clean_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_OUT l3_cls_fib_mode_t *fib_mode)
{
	ca_status_t rc;
	ca_uint16_t short_fib;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1)
		fib_table = &hgu_8279_l3_cls_fib_table;
#endif

	if (AAL_E_OK != (rc = __l3_cls_fib_mode_get(fib_table, entry_idx, &short_fib, fib_mode)))
		return rc;

	return AAL_E_OK;
}

ca_status_t aal_l3_cls_fib_clean_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t entry_idx,
	CA_IN l3_cls_fib_mode_t fib_mode)
{
	ca_status_t rc;
	ca_uint16_t i, key_tbl_idx, fib_def_entry;
	l3_cls_key_t key_entry;
	cls_fib_t fib_entry;
	l3_cls_key_type_t key_type;
	aal_table_t *key_table = &l3_cls_key_table;
	aal_table_t *fib_table = &l3_cls_fib_table;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (dev == 1) {
		key_table = &hgu_8279_l3_cls_key_table;
		fib_table = &hgu_8279_l3_cls_fib_table;
	}
#endif
	fib_def_entry = fib_table->max_entry - L3_CLS_FIB_TBL_DEF_ENTRY_MAX;

	/* get key type */
	if (entry_idx < fib_def_entry) {
		key_tbl_idx = entry_idx >> 2;
		if (AAL_E_OK != (rc = hw_entry_get(key_table, key_tbl_idx, key_entry.cls_key_info.key_tbl_data)))
			return rc;
		key_type = key_entry.cls_key_info.key_tbl_data[0].cls_key_type;
	}

	/* get fib entries */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
		if (AAL_E_OK != (rc = hw_entry_get(fib_table, entry_idx+i, &fib_entry.cls_fib_data.fib_tbl_data[i])))
			return rc;
	}

	if (entry_idx < fib_def_entry && key_type == CL_IF_ID_KEY) {
		/* short fib */
		memset(&fib_entry.cls_fib_data.fib_tbl_data[0], 0, sizeof(l3_cls_fib_tbl_entry_t));
		fib_entry.cls_fib_data.fib_tbl_data[0].fib_format_sel = fib_mode;
	} else {
		/* standard fib */
		memset(&fib_entry.cls_fib_data.fib_tbl_data[0], 0, 2*sizeof(l3_cls_fib_tbl_entry_t));
		fib_entry.cls_fib_data.fib_tbl_data[1].fib_format_sel = fib_mode;
	}

	/* del hw fib */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++)
		aal_entry_del(fib_table->table_id, entry_idx+i);

	/* set a hw fib */
	for (i=0; i<L3_CLS_STD_FIB_ENTRY_NUM; i++) {
		if (AAL_E_OK != (rc = aal_entry_add_by_idx(fib_table->table_id, &fib_entry.cls_fib_data.fib_tbl_data[i], entry_idx+i)))
			return rc;
	}

	return AAL_E_OK;
}

EXPORT_SYMBOL(aal_l3_cls_if_id_key_get);
EXPORT_SYMBOL(aal_l3_cls_if_id_key_set);
EXPORT_SYMBOL(aal_l3_cls_ipv4_short_key_get);
EXPORT_SYMBOL(aal_l3_cls_ipv4_short_key_set);
EXPORT_SYMBOL(aal_l3_cls_spcl_pkt_key_get);
EXPORT_SYMBOL(aal_l3_cls_spcl_pkt_key_set);
EXPORT_SYMBOL(aal_l3_cls_tunnel_id_key_get);
EXPORT_SYMBOL(aal_l3_cls_tunnel_id_key_set);
EXPORT_SYMBOL(aal_l3_cls_ipv6_short_key_get);
EXPORT_SYMBOL(aal_l3_cls_ipv6_short_key_set);
EXPORT_SYMBOL(aal_l3_cls_mcst_key_get);
EXPORT_SYMBOL(aal_l3_cls_mcst_key_set);
EXPORT_SYMBOL(aal_l3_cls_full_key_get);
EXPORT_SYMBOL(aal_l3_cls_full_key_set);
EXPORT_SYMBOL(aal_l3_cls_key_clean_get);
EXPORT_SYMBOL(aal_l3_cls_key_clean_set);
EXPORT_SYMBOL(aal_l3_cls_fib_0_get);
EXPORT_SYMBOL(aal_l3_cls_fib_0_set);
EXPORT_SYMBOL(aal_l3_cls_fib_1_get);
EXPORT_SYMBOL(aal_l3_cls_fib_1_set);
EXPORT_SYMBOL(aal_l3_cls_fib_2_get);
EXPORT_SYMBOL(aal_l3_cls_fib_2_set);
EXPORT_SYMBOL(aal_l3_cls_fib_3_get);
EXPORT_SYMBOL(aal_l3_cls_fib_3_set);
EXPORT_SYMBOL(aal_l3_cls_fib_4_get);
EXPORT_SYMBOL(aal_l3_cls_fib_4_set);
EXPORT_SYMBOL(aal_l3_cls_fib_clean_get);
EXPORT_SYMBOL(aal_l3_cls_fib_clean_set);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_l3_cls_add);
EXPORT_SYMBOL(aal_l3_cls_delete);
EXPORT_SYMBOL(aal_l3_cls_key_clean);
EXPORT_SYMBOL(aal_l3_cls_default_set);
#endif

