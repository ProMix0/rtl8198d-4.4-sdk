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
 *
 * -------------------------------------------------------------------------
 * @file    special_packet.c
 * @brief   Cortina Special Packet APIs implementation
 * @author  Lidong.Chen
 * @version 1.2
 * @date    2016-09-09
 *
 * #update history
 *
 * [2016-03-03] <Lidong.Chen> v1.0
 *  + finish the special packet APIs coding and push to code server
 * [2016-06-30] <Lidong.Chen> v1.1
 *  + change l2_special_pkt mask from 32 bit to 64 bit after adding my_mac1
 *  + spb table initilization for MyMAC, ARP packets for L3FE
 * [2016-09-09] <Lidong.Chen> v1.2
 *  + modify the ca_special_packet_set/get syntax, change parameter "max_rate"
 *    to "flow_id" according to latest CommonAPI manual
 * [2018-01-29] <Lidong.Chen> v1.3
 *  + add the source_port range check to support PON port too for APIs
 *****************************************************************************/
#include "ca_module.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_l2_specpkt.h"
#include "aal_l3_specpkt.h"

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#include "aal_l3fe.h"
#include "aal_l3_stg0.h"
#endif

#include "aal_l2_tm.h"
#include "aal_l3_tm.h"
#include "aal_l2_te.h"
#include "aal_l3_te.h"
#include "aal_port.h"
#include "aal_table.h"
#include "aal_l3_cam.h"

#include "api_common.h"
#include "special_packet.h"
#include "port.h"
#include "rate.h"
#include "sys.h"
#include "ca-scfg/scfg.h"
#include "log.h"


typedef struct {
    ca_boolean_t  init_done;
    ca_uint       special_packet_lock;
    ca_boolean_t  special_packet_to_cpu[CA_PKT_TYPE_MAX];
    ca_mac_addr_t my_mac;
} __special_packet_drv_t;

__special_packet_drv_t g_special_packet_drv;


/* CPU port MACRO */
#define __SPECPKT_PORT_IS_LAN(port) ((port) < AAL_LPORT_ETH_NI7)
#define __SPECPKT_PORT_IS_WAN(port) ((port) == AAL_LPORT_ETH_WAN)
#define __SPECPKT_PORT_IS_CPU(port) ((port) >= AAL_LPORT_CPU_0 \
                                     && (port) <= AAL_LPORT_CPU_7)

/* NOTE: not support CA_PKT_TYPE_CONTROL_ALL now */
#define __SPECPKT_TYPE_CHK(special_pkt) \
                    CA_ASSERT_RET(special_pkt < CA_PKT_TYPE_MAX, CA_E_PARAM)

#define __SPECPKT_PHYSICALPORT_TO_CAPORT(ptype, pport) \
                        CA_PORT_ID(ptype, pport)

#define __SPECPKT_CAPORT_TO_PHYSICALPORT(caport) \
                        PORT_ID(caport)

#define __SPECPKT_ACTION_HANDLE_FLOWID(handle)  ((handle) & 0xff)
#define __SPECPKT_ACTION_HANDLE_COS(handle)     (((handle) >> 8) & 0x7)
#define __SPECPKT_ACTION_HANDLE_INDEX(handle)   ((((handle) >> 11) & 0x1f) + 0x20)
#define __SPECPKT_IS_PON_INDEX(ldpid) ((ldpid) >= AAL_LPORT_LLID_GEM_INDEX_0 && (ldpid) <= AAL_LPORT_LLID_GEM_INDEX_31)
#define __SPECPKT_ACTION_HANDLE_PON_INDEX(ldpid, cos, flow) \
    ((((ldpid) & 0x1f) << 11)| ((cos) << 8)| (flow))

#ifdef CONFIG_CORTINA_BOARD_FPGA

#define __SPECPKT_DBG   OS_PRINTF

#else

#define __SPECPKT_DBG(fmt,...)  do {\
                                if (ca_api_debug & API_DBG_SPPKT)\
                                    ca_printf("%s()_line_%d_:"fmt, __func__,__LINE__,##__VA_ARGS__);\
                            }while(0)
#endif

#define __SPECPKT_LOCK(dev)   do { \
                                if (g_special_packet_drv.init_done) \
                                    ca_spin_lock(g_special_packet_drv.special_packet_lock);\
                              }while(0)

#define __SPECPKT_UNLOCK(dev)  do { \
                                if (g_special_packet_drv.init_done) \
                                    ca_spin_unlock(g_special_packet_drv.special_packet_lock);\
                              }while(0)

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#define __SPECPKT_SRC_PORT_CHK(source_port)      do {\
                                if (!CA_PORT_IS_ETHERNET(source_port)\
                                    && !CA_PORT_IS_PON(source_port)\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_CPU\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_OFFLOAD\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_L2RP\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_L3) {\
                                    __SPECPKT_DBG("Invalid source_port %#x.\n", source_port);\
                                    return CA_E_PARAM; \
                                } }while(0)
#else
#define __SPECPKT_SRC_PORT_CHK(source_port)      do {\
                                if (!CA_PORT_IS_ETHERNET(source_port)\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_CPU\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_OFFLOAD\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_L2RP\
                                    && PORT_TYPE(source_port) != CA_PORT_TYPE_L3) {\
                                    __SPECPKT_DBG("Invalid source_port %#x.\n", source_port);\
                                    return CA_E_PARAM; \
                                } }while(0)
#endif

/**
 * @__special_packet_type_xlat:
 * translate rx_pkt_type to hardware identified types in L2FE and L3FE
 *
 */
ca_status_t __special_packet_type_xlat(
    ca_pkt_type_t        rx_pkt_type,
    ca_port_id_t            source_port,
    aal_l2_specpkt_type_t  *l2_pkt_type,
    aal_l3_specpkt_type_t  *l3_pkt_type,
    ca_boolean_t           *is_l2_pkt,
    ca_boolean_t           *is_l3_pkt)
{
    ca_port_id_t port_id = __SPECPKT_CAPORT_TO_PHYSICALPORT(source_port);
    ca_uint32_t port_type = PORT_TYPE(source_port);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    if (CA_PORT_IS_PON(source_port)) {
        *is_l3_pkt = TRUE;
    }
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)  
    if (CA_PORT_IS_PON(source_port)) {
        *is_l2_pkt = TRUE;
    }
#endif

    if (port_type == CA_PORT_TYPE_ETHERNET) {
        if ((port_id > CA_PORT_ID_NI7) && (port_id != CA_PORT_ID_L3_WAN) && (port_id != CA_PORT_ID_L3_LAN))
            return CA_E_PORT;
        *is_l2_pkt = TRUE;
    }
    if (port_type == CA_PORT_TYPE_CPU) {
        if ((port_id < CA_PORT_ID_CPU0) || (port_id > CA_PORT_ID_CPU7))
            return CA_E_PORT;
        *is_l2_pkt = TRUE;
    }
    if (port_type == CA_PORT_TYPE_L2RP) {
        if ((port_id != CA_PORT_ID_L3_WAN) && (port_id != CA_PORT_ID_L3_LAN))
            return CA_E_PORT;
        *is_l3_pkt = TRUE;
    }
    if (port_type == CA_PORT_TYPE_L3) {
        if (port_id != CA_PORT_ID_NI7)
            return CA_E_PORT;
        *is_l3_pkt = TRUE;
    }

    switch (rx_pkt_type) {
        case CA_PKT_TYPE_BPDU:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_BPDU;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_BPDU;
            break;

        case CA_PKT_TYPE_DOT1X:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_802_DOT1X;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_802_DOT1X;
            break;

        case CA_PKT_TYPE_IGMP:
            /* share same result with CA_PKT_TYPE_ICMPV6_MLD in L2 */
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_IGMP_MLD;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_IGMP;
            break;

        case CA_PKT_TYPE_ARP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_ARP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_ARP;
            break;

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_VENUS)

        case CA_PKT_TYPE_OAM:
        case CA_PKT_TYPE_1AS:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_EFM_OAM;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_USER_DEF_0;
            break;
#else
        case CA_PKT_TYPE_OAM:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_EFM_OAM;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_EFM_OAM;

            break;
#endif

        case CA_PKT_TYPE_OMCI:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_UDF_0;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_USER_DEF_1;
            break;

        case CA_PKT_TYPE_PLOAM:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_MEF_OAM;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_USER_DEF_2;
            break;

        case CA_PKT_TYPE_MPCP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_MPCP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_MPCP;
            break;

        case CA_PKT_TYPE_DHCP:
            /* DHCP and DHCPv6 shares same result, and there is HW bug of L2 spcl pkt DHCPv6,
             * so do not allow to set DHCP for L2 spcl pkt.
             */
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_DHCP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_DHCPV4;
            break;

        case CA_PKT_TYPE_DHCPV6:
            /* HW bug. DHCPv6 Solicit and Request messages hit
             * L2 spcl pkt rule "IEEE Reserved MAC C" which is 33-33-xx-xx-xx-xx.
             * Therefore, do not allow to set DHCPv6 for L2 spcl pkt.
             */
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_DHCPV6;
            break;

        case CA_PKT_TYPE_PPPOE_DIS:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_PPPOE_DISC;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_PPPOE_DISC;
            break;

        case CA_PKT_TYPE_PPPOE_SES:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_PPPOE_SESS;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_PPPOE_SESS;
            break;

        case CA_PKT_TYPE_IROS_HELLO:
            return CA_E_NOT_SUPPORT;

        case CA_PKT_TYPE_ICMP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_ICMP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_IPV4_ICMP;
            break;

        case CA_PKT_TYPE_ICMPV6_MLD:
            /* share same result with CA_PKT_TYPE_IGMP in L2 */
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_IGMP_MLD;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_MLD;
            break;

        case CA_PKT_TYPE_ICMPV6_NDP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_NDP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_IPV6_NDP;
            break;

        case CA_PKT_TYPE_CFM:
        case CA_PKT_TYPE_USER_DEFINED:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_UDF_0;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_USER_DEF_0;
            break;

        case CA_PKT_TYPE_L2_PTP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_L2_PTP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_L2_PTP;
            break;

        case CA_PKT_TYPE_L4_PTP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_PTP_UDP;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_L4_PTP;
            break;

        case CA_PKT_TYPE_HTTP_GET:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_GET;
            break;

        case CA_PKT_TYPE_HTTP_PUT:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_PUT;
            break;

        case CA_PKT_TYPE_HTTP_HEAD:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_HEAD;
            break;

        case CA_PKT_TYPE_HTTP_POST:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_POST;
            break;

        case CA_PKT_TYPE_HTTP_OPTIONS:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_OPTIONS;
            break;

        case CA_PKT_TYPE_HTTP_DELETE:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_DELETE;
            break;

        case CA_PKT_TYPE_HTTP_TRACE:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_TRACE;
            break;

        case CA_PKT_TYPE_HTTP_CONNECT:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_CONNECT;
            break;

        case CA_PKT_TYPE_HTTP_UNLINK:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_UNLINK;
            break;

        case CA_PKT_TYPE_HTTP_PATCH:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_HTTP_PATCH;
            break;

        case CA_PKT_TYPE_DNS:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_DNS;
            break;

        case CA_PKT_TYPE_RIP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_RIP;
            break;

        case CA_PKT_TYPE_SSDP:
            *l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
            *l3_pkt_type = AAL_L3_SPECPKT_TYPE_SSDP;
            break;

        case CA_PKT_TYPE_CONTROL_ALL:
        case CA_PKT_TYPE_MIRROR:
        case CA_PKT_TYPE_LOOPBACK:
        default:
            return CA_E_NOT_SUPPORT;

    }

    return CA_E_OK;

}

ca_status_t __special_packet_ctrl_set(
    ca_device_id_t      device_id,
    ca_uint32_t      source_port,
    ca_boolean_t     is_l3_pkt,
    ca_boolean_t     is_l2_pkt,
    ca_uint32_t      l3_pkt_type,
    ca_uint32_t      l2_pkt_type,
    ca_boolean_t     forward_to_cpu,
    ca_uint32_t      dest_port,
    ca_uint8_t       priority,
    ca_uint16_t      flow_id,
    ca_special_packet_option_t   *options)
{
    aal_l2_specpkt_ctrl_t       l2_ctrl;
    aal_l2_specpkt_ctrl_mask_t  l2_ctrl_mask;
    aal_ilpb_cfg_t              ilpb_cfg;
#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_l3_specpkt_ctrl_t       l3_ctrl;
    aal_l3_specpkt_ctrl_mask_t  l3_ctrl_mask;
    aal_l3fe_lpb_tbl_cfg_t      lpb_tbl_cfg;
    ca_uint8_t     lpb_profile = 0;
    L3FE_STG0_CTRL_t stg0_ctrl;
    l3_stg0_ldpid_map_t stg0_ldpid_map;
#endif

    ca_uint32_t    spid = __SPECPKT_CAPORT_TO_PHYSICALPORT(source_port);
    ca_uint32_t    dpid = __SPECPKT_CAPORT_TO_PHYSICALPORT(dest_port);
    ca_status_t    ret = CA_E_OK;
    ca_boolean_t   not_support = TRUE;

    CA_ASSERT_RET(options, CA_E_PARAM);

    if (is_l2_pkt && (l2_pkt_type != AAL_L2_SPECPKT_TYPE_INVALID)) {
        not_support = FALSE;

        ret = aal_port_ilpb_cfg_get(device_id, spid, &ilpb_cfg);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_port_ilpb_cfg_get return %#x.\n", ret);
            return ret;
        }

        memset(&l2_ctrl, 0, sizeof(aal_l2_specpkt_ctrl_t));

        l2_ctrl_mask.u32          = 0;
        l2_ctrl_mask.s.ldpid      = 1;
        l2_ctrl_mask.s.cos        = 1;
        l2_ctrl_mask.s.rate_limit_bypass = 1;
        l2_ctrl_mask.s.flowid     = 1;
        l2_ctrl_mask.s.spcl_fwd   = 1;
        l2_ctrl_mask.s.flow_en    = 1;

        if (options->mask.do_not_drop){
           l2_ctrl_mask.s.pkt_no_drop = 1;
           l2_ctrl.pkt_no_drop = options->do_not_drop;
        }

        if (options->mask.forward_original){
           l2_ctrl_mask.s.pkt_no_edit = 1;
           l2_ctrl.pkt_no_edit = options->forward_original;
        }

        if (options->mask.enable_sa_learning){
           l2_ctrl_mask.s.learn_dis = 1;
           l2_ctrl.learn_dis = !options->enable_sa_learning;
        }

        if (AAL_L2_SPECPKT_TYPE_L2_PTP == l2_pkt_type || AAL_L2_SPECPKT_TYPE_PTP_UDP == l2_pkt_type){
            l2_ctrl_mask.s.keep_ts = 1;
            l2_ctrl.keep_ts = 1;
        }

        l2_ctrl.ldpid = dpid;
        l2_ctrl.cos   = priority;
        if (flow_id == CA_FLOW_ID_INVALID) {
            l2_ctrl.rate_limit_bypass = 1;
            l2_ctrl.flowid  = 0;
            l2_ctrl.flow_en = 0;
        } else {
            l2_ctrl.flowid  = flow_id;
            l2_ctrl.flow_en = 1;
        }

        if (forward_to_cpu)
            l2_ctrl.spcl_fwd = 1;

        ret = aal_l2_specpkt_ctrl_set(device_id,
                ilpb_cfg.spcl_pkt_idx, l2_pkt_type, l2_ctrl_mask, &l2_ctrl);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l2_specpkt_ctrl_set return %#x.\n", ret);
            return ret;
        }
    }

#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

    if (is_l3_pkt && (l3_pkt_type != AAL_L3_SPECPKT_TYPE_INVALID)) {
        not_support = FALSE;

        /* get lpb_idx_mode for correct source of lpb_profile */
        if ((ret = aal_l3fe_stg0_ctrl_get(device_id, &stg0_ctrl)) != AAL_E_OK) {
            ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
            return ret;
        }

        if (stg0_ctrl.bf.lpb_idx_mode == 0) {
            /* get lpb_profile from L3FE_STG0_LDPID_MAP */

            if ((ret = aal_l3_stg0_ldpid_map_get(device_id, &stg0_ldpid_map)) != CA_E_OK) {
                ca_printf("%s: Fail to get STG0 LDPID map. (ret=%d)\n", __func__, ret);
                return ret;
            }

#ifdef CONFIG_CORTINA_BOARD_FPGA
            /* STG0 translates HDR_A.ldpid 0x0 to HDR_I.lspid 0x7,
               so it should search spid 0 from STG0_LDPID_MAP.
             */
            if (spid == CA_PORT_ID_NI7) spid = 0x0;
#endif

            if (spid == stg0_ldpid_map.rule_0)
                lpb_profile = 0;
            else if (spid == stg0_ldpid_map.rule_1)
                lpb_profile = 1;
            else if (spid == stg0_ldpid_map.rule_2)
                lpb_profile = 2;
            else
                lpb_profile = 3;

        } else {
            /* get lpb_profile from L3FE_STG0_LPB_IDX_TBL */

            ret = aal_l3fe_lpb_idx_tbl_get(device_id, spid, &lpb_profile);
            if (CA_E_OK != ret) {
                __SPECPKT_DBG("Invoke aal_l3fe_lpb_idx_tbl_get return %#x.\n", ret);
                return ret;
            }

        }

        l3_ctrl_mask.s.ldpid   = 1;
        l3_ctrl_mask.s.cos     = 1;
        l3_ctrl_mask.s.pol_en_sel = 1;
        l3_ctrl_mask.s.pol_id  = 1;
        l3_ctrl_mask.s.spcl_en = 1;

        memset(&l3_ctrl, 0, sizeof(aal_l3_specpkt_ctrl_t));
        l3_ctrl.ldpid = dpid;
        l3_ctrl.cos   = priority;

        if (flow_id == CA_FLOW_ID_INVALID) {
            l3_ctrl.pol_en_sel = AAL_L3_SPECPKT_POL_DISABLE;
            l3_ctrl.pol_id = 0;
        } else {
            l3_ctrl.pol_en_sel = AAL_L3_SPECPKT_POL_ENABLE_L3;
            l3_ctrl.pol_id = flow_id & (~CA_PILICER_L3_FLOW_ID_BASE);
        }

        if (forward_to_cpu)
            l3_ctrl.spcl_en = 1;

        if (AAL_L3_SPECPKT_TYPE_L2_PTP == l3_pkt_type || AAL_L3_SPECPKT_TYPE_L4_PTP == l3_pkt_type){
            l3_ctrl_mask.s.keep_ts = 1;
            l3_ctrl.keep_ts = 1;
        }

        memset(&lpb_tbl_cfg, 0, sizeof(lpb_tbl_cfg));
        ret = aal_l3_stg0_lpb_tbl_get(device_id, lpb_profile, &lpb_tbl_cfg);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l3_stg0_lpb_tbl_get return %#x.\n", ret);
            return ret;
        }

        ret = aal_l3_specpkt_ctrl_set(device_id, lpb_tbl_cfg.spcl_pkt_sel,
            l3_pkt_type, l3_ctrl_mask, &l3_ctrl);

        if (l3_pkt_type == AAL_L3_SPECPKT_TYPE_IPV4_ICMP) {
            /* specially handle CA_PKT_TYPE_ICMP which supports ICMPv4 and ICMPv6 */
            ret = aal_l3_specpkt_ctrl_set(device_id, lpb_tbl_cfg.spcl_pkt_sel,
                AAL_L3_SPECPKT_TYPE_IPV6_ICMP, l3_ctrl_mask, &l3_ctrl);
        }
    }
#endif
    if (not_support)
        return CA_E_NOT_SUPPORT;
    return ret;

}

ca_status_t __special_packet_udf_set(
    ca_device_id_t device_id,
    ca_boolean_t     enable,
    ca_special_packet_option_t   *options
)
{
    ca_status_t ret = CA_E_OK;
    aal_l2_specpkt_detect_mask_t l2_detect_mask;
    aal_l2_specpkt_detect_t      l2_detect;
    ca_user_defined_special_packet_mask_t   *mask = NULL;

    CA_ASSERT_RET(options, CA_E_PARAM);

    mask = &options->user_defined_special_packet.mask;

    memset(&l2_detect_mask, 0, sizeof(aal_l2_specpkt_detect_mask_t));
    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    l2_detect_mask.s.udf0_detect_enable = 1;
    l2_detect_mask.s.udf0_mac = mask->mac_da;
    l2_detect_mask.s.udf0_mac_mask = mask->mac_da_mask;
    l2_detect_mask.s.udf0_ethtype = mask->ether_type;
    l2_detect_mask.s.udf0_ethtype_enable = 1;

    l2_detect.udf0_detect_enable = enable;
    l2_detect.udf0_ethtype_enable = enable;

    memcpy(&l2_detect.udf0_mac[0], &options->user_defined_special_packet.mac_da, sizeof(ca_mac_addr_t));
    memcpy(&l2_detect.udf0_mac_mask[0], &options->user_defined_special_packet.mac_da_mask, sizeof(ca_mac_addr_t));

    l2_detect.udf0_ethtype = options->user_defined_special_packet.ether_type;

    ret = aal_l2_specpkt_detect_set(device_id, l2_detect_mask, &l2_detect);

    return ret;
}

ca_status_t __special_packet_udf_get(
    ca_device_id_t device_id,
    ca_special_packet_option_t   *options
)
{
    ca_status_t ret = CA_E_OK;
    aal_l2_specpkt_detect_t      l2_detect;

    CA_ASSERT_RET(options, CA_E_PARAM);

    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    ret = aal_l2_specpkt_detect_get(device_id, &l2_detect);
    options->user_defined_special_packet.ether_type = l2_detect.udf0_ethtype;
    memcpy(options->user_defined_special_packet.mac_da, &l2_detect.udf0_mac[0], sizeof(ca_mac_addr_t));
    memcpy(options->user_defined_special_packet.mac_da_mask, &l2_detect.udf0_mac_mask[0], sizeof(ca_mac_addr_t));

    return ret;
}

static inline void __special_packet_global_init(ca_device_id_t device_id)
{
    ca_status_t            ret = CA_E_OK;
    ca_device_config_tlv_t scfg_tlv;
    ca_uint8_t             default_my_mac[] = {
                           0x00, 0x13, 0x25, 0x00, 0x00, 0x01};

    memset(&g_special_packet_drv, 0, sizeof(g_special_packet_drv));

    ca_spin_lock_init(&g_special_packet_drv.special_packet_lock, SPINLOCK_BH);

    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    scfg_tlv.type = CA_CFG_ID_NE_MAC_ADDR0;
    #else
    scfg_tlv.type = CA_CFG_ID_NE_MAC_ADDR1;
    #endif
    scfg_tlv.len  = 6;

    ret = ca_device_config_tlv_get(device_id, &scfg_tlv);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("read CA_CFG_ID_MAC_ADDRESS fail %#x!.\n", ret);
        memcpy(&g_special_packet_drv.my_mac[0], &default_my_mac[0], 6);
    } else {
        memcpy(&g_special_packet_drv.my_mac[0], &scfg_tlv.value[0], 6);
    }

    return ;

}

static inline void __special_packet_global_exit(ca_device_id_t device_id)
{
    if (g_special_packet_drv.special_packet_lock)
        ca_spin_lock_destroy(g_special_packet_drv.special_packet_lock);

    memset(&g_special_packet_drv, 0, sizeof(g_special_packet_drv));

    return;
}

static ca_status_t __special_packet_l2_detect_init(ca_device_id_t  device_id)
{
    ca_status_t                  ret = CA_E_OK;
    aal_l2_specpkt_detect_mask_t l2_detect_mask;
    aal_l2_specpkt_detect_t      l2_detect;
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) 
    aal_l2_specpkt_ctrl_mask_t my_mac_mask;
    aal_l2_specpkt_ctrl_t     my_mac_config;
    ca_uint8_t               * mac_id = NULL;
#endif
    memset(&l2_detect_mask, 0, sizeof(aal_l2_specpkt_detect_mask_t));
    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    l2_detect_mask.s.my_mac = 1;
    l2_detect_mask.s.my_mac_mask = 1;

#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) 
    if ( CA_LAN_TYPE_CPU_PORT_DEFAULT_ID == CA_WAN_TYPE_CPU_PORT_DEFAULT_ID) {
        mac_id = CFG_ID_NE_MAC_ADDR0;
    }else {
        mac_id = CFG_ID_NE_MAC_ADDR1;
    }
    aal_scfg_read(mac_id, 6, g_special_packet_drv.my_mac);

    memcpy(&l2_detect.my_mac, &g_special_packet_drv.my_mac, 6);
    l2_detect.my_mac_mask[0] = 0xFF;
    l2_detect.my_mac_mask[1] = 0xFF;
    l2_detect.my_mac_mask[2] = 0xFF;
    l2_detect.my_mac_mask[3] = 0xFF;
    l2_detect.my_mac_mask[4] = 0xFF;
    l2_detect.my_mac_mask[5] = 0xFF;// TODO: can be changed to 0xFE for 2 MACs
#else /* According to latest design, we do NOT use L2 MY-MAC but use STATIC FDB to forward MyMAC packets to L3FE */
    /* Set MYMAC to be 00:00:00:00:00:00 to disable L2 MY-MAC detection */
#endif

    ret = aal_l2_specpkt_detect_set(device_id, l2_detect_mask, &l2_detect);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("aal_l2_specpkt_detect_set fail, return %#x.\n", ret);
        return ret;
    }

#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    my_mac_mask.s.pkt_no_edit = 1;
    my_mac_mask.s.pkt_no_drop = 1;
    my_mac_mask.s.spcl_fwd = 1;
    my_mac_mask.s.ldpid = 1;

    my_mac_config.ldpid = CA_LAN_TYPE_CPU_PORT_DEFAULT_ID;
    my_mac_config.pkt_no_drop = 1;
    my_mac_config.pkt_no_edit = 1;
    my_mac_config.spcl_fwd = 1;
    
    ret = aal_l2_specpkt_ctrl_set( 0 , 2 , AAL_L2_SPECPKT_TYPE_MYMAC, my_mac_mask ,&my_mac_config); 
#endif    
    return ret;

}

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_VENUS)
static ca_status_t __special_packet_saturn_init(ca_device_id_t device_id)
{
#define __SATURN_TPID_EFM_OAM  0xFFF1
#define __SATURN_TPID_OMCI     0xFFF1
#define __SATURN_TPID_PLOAM    0xFFF0
#define __SATURN_TPID_MPCP     0xFFF0
#define __SATURN_TPID_CFM      0x8902
#define __SATURN_PLOAM_DA      {0x00, 0x13, 0x25, 0x00, 0x00, 0x00}
#define __SATURN_OMCI_DA       {0x00, 0x13, 0x25, 0x00, 0x00, 0x01}
    ca_status_t                  ret = CA_E_OK;
    ca_uint8_t                   pon_mode = 0;
    aal_l2_specpkt_detect_mask_t l2_detect_mask;
    aal_l2_specpkt_detect_t      l2_detect;
    aal_l2_specpkt_ctrl_t        l2_ctrl;
    aal_l2_specpkt_ctrl_mask_t   l2_ctrl_mask;
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    aal_l3_specpkt_detect_mask_t l3_detect_mask;
    aal_l3_specpkt_detect_t      l3_detect;
    aal_l3_specpkt_ctrl_t        l3_ctrl;
    aal_l3_specpkt_ctrl_mask_t   l3_ctrl_mask;
    aal_l3fe_lpb_tbl_cfg_t       lpb_tbl_cfg;
    ca_uint8_t     lpb_profile = 0;
#endif

    ca_uint32_t    spb_idx = 0;
    //ca_uint32_t    tmp = 0;
    ca_uint8_t     ploam_da[6] = __SATURN_PLOAM_DA;
    ca_uint8_t     omci_da[6] = __SATURN_OMCI_DA;

    /*
    For special packets ingress at WAN port, we'll configure L3 special packet as following

    Frames will arrive at G3 NE will L2 header and configured TPID:
    MAC DA (6B) + MAC SA(6B) + TPID (2B) + VID(2B) +Payload

    OAM frames from WAN port (connected to PON port):
    "    L3FE_PP_SPCL_PKT_DETECTION_CFG.user_def_spcl_0_en = 1;
    "    L3FE_PP_USER_TYPE_0.user_type_0 = 0x8809;


    OMCI frames from WAN port (connected to PON port)
    "    L3FE_PP_SPCL_PKT_DETECTION_CFG.user_def_spcl_1_en = 1;
    "    L3FE_PP_USER_TYPE_0.user_type_1 = 0xFFFE;

    PLOAM frames from WAN port (connected to PON port)
    "    L3FE_PP_SPCL_PKT_DETECTION_CFG.user_def_spcl_2_en = 1;
    "    L3FE_PP_USER_TYPE_1.user_type_2 = 0xFFFD;

    CFM frames from WAN port (connected to PON port) and/or LAN ports
    "    L3FE_PP_SPCL_PKT_DETECTION_CFG.user_def_spcl_2_en = 1;
    "    L3FE_PP_USER_TYPE_1.user_type_3 = 0x8902;

    */
    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
        ca_printf("!error : <%s, %d> read CFG_ID_PON_MAC_MODE fail\r\n", __func__, __LINE__);
        pon_mode = ONU_PON_MAC_XGSPON;
    }

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    memset(&l3_detect_mask, 0, sizeof(l3_detect_mask));
    memset(&l3_detect, 0, sizeof(l3_detect));

    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {/* EPON mode */
        l3_detect_mask.s.user_def_ethtype_0 = 1;
        l3_detect_mask.s.user_def_0_ethtype_detect_enable = 1;
        l3_detect_mask.s.user_def_spcl_0_enable = 1;
        l3_detect.user_def_ethtype_0 = __SATURN_TPID_EFM_OAM;
        l3_detect.user_def_spcl_0_enable = 1;
        l3_detect.user_def_0_ethtype_detect_enable = 1;
    } else if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) { /* GPON mode */
        l3_detect_mask.s.user_def_ethtype_1 = 1;
        l3_detect_mask.s.user_def_1_ethtype_detect_enable = 1;
        l3_detect_mask.s.user_def_spcl_1_enable = 1;
        l3_detect_mask.s.user_def_ethtype_2 = 1;
        l3_detect_mask.s.user_def_spcl_2_enable = 1;
        l3_detect.user_def_ethtype_1 = __SATURN_TPID_OMCI;
        l3_detect.user_def_ethtype_2 = __SATURN_TPID_PLOAM;
        l3_detect.user_def_spcl_1_enable = 1;
        l3_detect.user_def_spcl_2_enable = 1;
        l3_detect.user_def_1_ethtype_detect_enable = 1;
    }

    /* 802.1ag CFM support */
    l3_detect_mask.s.user_def_ethtype_3 = 1;
    l3_detect_mask.s.user_def_spcl_3_enable = 1;
    l3_detect.user_def_ethtype_3 = __SATURN_TPID_CFM;
    l3_detect.user_def_spcl_3_enable = 1;

    ret = aal_l3_specpkt_detect_set(CA_DEV_LAN, l3_detect_mask, &l3_detect);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("LAN aal_l3_specpkt_detect_set fail, return %#x.\n", ret);
        return ret;
    }

    ret = aal_l3_specpkt_detect_set(CA_DEV_WAN, l3_detect_mask, &l3_detect);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("WAN aal_l3_specpkt_detect_set fail, return %#x.\n", ret);
        return ret;
    }

    /* Special forward for OAM/OMCI/PLOAM/CFM:
     * downstream: to G3-CPU queue#7
     * upstream: to PON queue#9
     */
    l3_ctrl_mask.u32       = 0;
    l3_ctrl_mask.s.ldpid   = 1;
    l3_ctrl_mask.s.cos     = 1;
    l3_ctrl_mask.s.spcl_en = 1;
    l3_ctrl_mask.s.no_drop = 1;
    memset(&l3_ctrl, 0, sizeof(aal_l3_specpkt_ctrl_t));
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    l3_ctrl.ldpid   = AAL_LPORT_CPU_0;
    #else
    l3_ctrl.ldpid   = AAL_LPORT_CPU_1;
    #endif
    l3_ctrl.cos     = 7;
    l3_ctrl.spcl_en = 1;
    l3_ctrl.no_drop = 1;
    ret = aal_l3fe_lpb_idx_tbl_get(CA_DEV_LAN, AAL_LPORT_ETH_WAN, &lpb_profile);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Invoke aal_l3fe_lpb_idx_tbl_get return %#x.\n", ret);
        return ret;
    }

    memset(&lpb_tbl_cfg, 0, sizeof(lpb_tbl_cfg));
    ret = aal_l3_stg0_lpb_tbl_get(CA_DEV_LAN, lpb_profile, &lpb_tbl_cfg);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Invoke aal_l3fe_lpb_tbl_get return %#x.\n", ret);
        return ret;
    }

    if (!lpb_tbl_cfg.spcl_pkt_en) {
        aal_l3fe_lpb_tbl_cfg_mask_t  l3fe_lpb_mask;
        l3fe_lpb_mask.u32 = 0;
        l3fe_lpb_mask.s.spcl_pkt_en = 1;
        ca_printf("!error: l3 special packet was not enable for lpb %d\n", lpb_profile);
        lpb_tbl_cfg.spcl_pkt_en = 1;
        ret = aal_l3_stg0_lpb_tbl_set(CA_DEV_LAN, lpb_profile, l3fe_lpb_mask, &lpb_tbl_cfg);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l3_stg0_lpb_tbl_set return %#x.\n", ret);
            return ret;
        }
    }

    /* Downstream OMCI,OAM,PLOAM to CPU */
    for (spb_idx = AAL_L3_SPECPKT_TYPE_USER_DEF_0; spb_idx <= AAL_L3_SPECPKT_TYPE_USER_DEF_3; spb_idx++){
        ret = aal_l3_specpkt_ctrl_set(CA_DEV_LAN, lpb_tbl_cfg.spcl_pkt_sel,
            spb_idx, l3_ctrl_mask, &l3_ctrl);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l3_specpkt_ctrl_set OMCI return %#x.\n", ret);
            return ret;
        }
    }
#if 0
    tmp = lpb_tbl_cfg.spcl_pkt_sel;

    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {/* mutliple llid case: source port is 0x20~0x3f */
        for (spb_idx = AAL_LPORT_CPU_MQ_0; spb_idx <= AAL_LPORT_CPU_MQ_31; spb_idx++) {
            ret = aal_l3fe_lpb_idx_tbl_get(CA_DEV_LAN, spb_idx, &lpb_profile);
            if (CA_E_OK != ret) {
                __SPECPKT_DBG("Invoke aal_l3fe_lpb_idx_tbl_get return %#x.\n", ret);
                return ret;
            }

            memset(&lpb_tbl_cfg, 0, sizeof(lpb_tbl_cfg));
            ret = aal_l3_stg0_lpb_tbl_get(CA_DEV_LAN, lpb_profile, &lpb_tbl_cfg);
            if (CA_E_OK != ret) {
                __SPECPKT_DBG("Invoke aal_l3fe_lpb_tbl_get return %#x.\n", ret);
                return ret;
            }

            if (lpb_tbl_cfg.spcl_pkt_sel == tmp) {
                continue;
            }

            /* Downstream OMCI,OAM,PLOAM to CPU */
            for (spb_idx = AAL_L3_SPECPKT_TYPE_USER_DEF_0; spb_idx <= AAL_L3_SPECPKT_TYPE_USER_DEF_3; spb_idx++){
                ret = aal_l3_specpkt_ctrl_set(CA_DEV_LAN, lpb_tbl_cfg.spcl_pkt_sel,
                    spb_idx, l3_ctrl_mask, &l3_ctrl);
                if (CA_E_OK != ret) {
                    __SPECPKT_DBG("Invoke aal_l3_specpkt_ctrl_set OMCI return %#x.\n", ret);
                    return ret;
                }
            }

        }
    }
#endif

    memset(&l2_detect_mask, 0, sizeof(aal_l2_specpkt_detect_mask_t));
    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    l2_detect_mask.s.udf0_detect_enable = 1;
    l2_detect_mask.s.udf0_mac = 1;
    l2_detect_mask.s.udf0_mac_mask = 1;
    l2_detect_mask.s.udf0_ethtype = 1;
    l2_detect_mask.s.udf0_ethtype_enable = 1;
    l2_detect.udf0_detect_enable = 1;
    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        l2_detect.udf0_ethtype = __SATURN_TPID_OMCI;
    } else {
        l2_detect.udf0_ethtype = __SATURN_TPID_EFM_OAM;
    }
    l2_detect.udf0_ethtype_enable = 1;

    memcpy(&l2_detect.udf0_mac[0], &omci_da[0], 6);
    l2_detect.udf0_mac_mask[0] = 0xFF;
    l2_detect.udf0_mac_mask[1] = 0xFF;
    l2_detect.udf0_mac_mask[2] = 0xFF;
    l2_detect.udf0_mac_mask[3] = 0xFF;
    l2_detect.udf0_mac_mask[4] = 0xFF;
    l2_detect.udf0_mac_mask[5] = 0x00; // for upstream da = 0013250000xx 

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        l2_detect_mask.s.mef0_mac = 1;
        l2_detect_mask.s.mef0_ethtype = 1;
        l2_detect.mef0_ethtype = __SATURN_TPID_PLOAM;
        memcpy(&l2_detect.mef0_mac[0], ploam_da, 6);
    }

    ret = aal_l2_specpkt_detect_set(CA_DEV_WAN, l2_detect_mask, &l2_detect);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("aal_l2_specpkt_detect_set fail, return %#x.\n", ret);
        return ret;
    }

    /* HGU-PON-DEV:
     * downstream OMCI/OAM to NI6 directly
     * upstream OMCI/OAM to PON port directly
     */
    l2_ctrl_mask.u32 = 0;
    l2_ctrl_mask.s.pkt_no_drop = 1;
    l2_ctrl_mask.s.pkt_no_edit = 1;
    l2_ctrl_mask.s.ldpid   = 1;
    l2_ctrl_mask.s.cos     = 1;
    l2_ctrl_mask.s.spcl_fwd = 1;
    l2_ctrl_mask.s.learn_dis = 1;
    l2_ctrl_mask.s.rate_limit_bypass = 1;
    l2_ctrl_mask.s.limit_fwd_permit = 1;
    l2_ctrl_mask.s.bypass_sa_lrn_chk = 1;

    memset(&l2_ctrl, 0, sizeof(aal_l2_specpkt_ctrl_t));
    l2_ctrl.spcl_fwd = 1;
    l2_ctrl.pkt_no_drop = 1;
    l2_ctrl.learn_dis = 1;
    l2_ctrl.rate_limit_bypass = 1;
    l2_ctrl.limit_fwd_permit  = 1;
    l2_ctrl.bypass_sa_lrn_chk = 1;

    /* Downstream to NI6 LAN port */
    for (spb_idx = AAL_L2_SPECPKT_TYPE_IEEE_RSVD_A;  spb_idx <= AAL_L2_SPECPKT_TYPE_MAX; spb_idx++) {

        if ((spb_idx == AAL_L2_SPECPKT_TYPE_UDF_0)
            || (spb_idx == AAL_L2_SPECPKT_TYPE_MEF_OAM)) { /* OMCI, OAM to NI6 queue9 for highest priority */
            l2_ctrl.ldpid       = AAL_LPORT_9QUEUE_NI6;
            l2_ctrl.cos         = 7;
            l2_ctrl.pkt_no_edit = 1;
        } else if (spb_idx == AAL_L2_SPECPKT_TYPE_IGMP_MLD) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 6;
            l2_ctrl.pkt_no_edit = 0;
        } else if (spb_idx == AAL_L2_SPECPKT_TYPE_ARP ||spb_idx == AAL_L2_SPECPKT_TYPE_DHCP) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 5;
            l2_ctrl.pkt_no_edit = 0;
        } else if (spb_idx == AAL_L2_SPECPKT_TYPE_802_DOT1X) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 4;
            l2_ctrl.pkt_no_edit = 0;
        } else if (spb_idx == AAL_L2_SPECPKT_TYPE_BPDU) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 3;
            l2_ctrl.pkt_no_edit = 0;
        } else if (spb_idx == AAL_L2_SPECPKT_TYPE_MYMAC) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 2;
            l2_ctrl.pkt_no_edit = 0;
        } else if (spb_idx >= AAL_L2_SPECPKT_TYPE_IEEE_RSVD_A && spb_idx <= AAL_L2_SPECPKT_TYPE_IEEE_RSVD_GRP) {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 1;
            l2_ctrl.pkt_no_edit = 0;
        } else {
            l2_ctrl.ldpid       = AAL_LPORT_ETH_NI6;
            l2_ctrl.cos         = 0;
            l2_ctrl.pkt_no_edit = 0;
        }

        ret = aal_l2_specpkt_ctrl_set(CA_DEV_WAN, AAL_PORT_SPB_IDX_WAN,
            spb_idx, l2_ctrl_mask, &l2_ctrl);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l2_specpkt_ctrl_set pkt_type %d return %#x.\n", spb_idx, ret);
            return ret;
        }

    }

    /* Upstream OMCI/OAM to WAN(PON) port directly */
    l2_ctrl.ldpid             = AAL_LPORT_9QUEUE_NI7;
    l2_ctrl.cos               = 7;
    l2_ctrl.pkt_no_edit       = 1;

    ret = aal_l2_specpkt_ctrl_set(CA_DEV_WAN, AAL_PORT_SPB_IDX_LAN,
        AAL_L2_SPECPKT_TYPE_UDF_0, l2_ctrl_mask, &l2_ctrl);

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        ret |= aal_l2_specpkt_ctrl_set(CA_DEV_WAN, AAL_PORT_SPB_IDX_LAN,
        AAL_L2_SPECPKT_TYPE_MEF_OAM, l2_ctrl_mask, &l2_ctrl);
    }

    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Invoke aal_l2_specpkt_ctrl_set pkt_type %d return %#x.\n", spb_idx, ret);
        return ret;
    }
#else

/*
For special packets ingress at LAN port, we'll configure L2 special packet as following:
CFM frames from WAN port (connected to PON port) and/or LAN ports
"    L2FE_PP_SPECIAL_EV2PT. ev2pt = 0x8902;
"    L2FE_PP_SPECIAL_ENABLE_DA_0_LOW = 0;
"    L2FE_PP_SPECIAL_ENABLE_DA_0_HI = 0;
*/
    memset(&l2_detect_mask, 0, sizeof(aal_l2_specpkt_detect_mask_t));
    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    l2_detect_mask.s.mef1_mac = 1;
    l2_detect_mask.s.mef1_ethtype = 1;
    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        l2_detect.mef1_ethtype = __SATURN_TPID_OMCI;
    } else {
        l2_detect.mef1_ethtype = __SATURN_TPID_EFM_OAM;
    }

    memcpy(&l2_detect.mef1_mac[0], &omci_da[0], 6);

    /*
    l2_detect_mask.s.udf0_detect_enable = 1;
    l2_detect_mask.s.udf0_mac = 1;
    l2_detect_mask.s.udf0_mac_mask = 1;
    l2_detect_mask.s.udf0_ethtype = 1;
    l2_detect_mask.s.udf0_ethtype_enable = 1;
    l2_detect.udf0_detect_enable = 1;
    l2_detect.udf0_ethtype = __SATURN_TPID_OMCI;
    l2_detect.udf0_ethtype_enable = 1;

    memcpy(&l2_detect.udf0_mac[0], &omci_da[0], 6);
    l2_detect.udf0_mac_mask[0] = 0xFF;
    l2_detect.udf0_mac_mask[1] = 0xFF;
    l2_detect.udf0_mac_mask[2] = 0xFF;
    l2_detect.udf0_mac_mask[3] = 0xFF;
    l2_detect.udf0_mac_mask[4] = 0xFF;
    l2_detect.udf0_mac_mask[5] = 0xFF;
    */
    l2_detect_mask.s.mef0_mac = 1;
    l2_detect_mask.s.mef0_ethtype = 1;
    l2_detect.mef0_ethtype = __SATURN_TPID_PLOAM;
    memcpy(&l2_detect.mef0_mac[0], ploam_da, 6);
/* Following code is for SATURN FPGA TEST ONLY, use MEF to caputre OMCI, PLOAM */

    ret = aal_l2_specpkt_detect_set(device_id, l2_detect_mask, &l2_detect);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("aal_l2_specpkt_detect_set fail, return %#x.\n", ret);
        return ret;
    }

    l2_ctrl_mask.u32 = 0;
    l2_ctrl_mask.s.pkt_no_drop = 1;
    l2_ctrl_mask.s.pkt_no_edit = 1;
    l2_ctrl_mask.s.ldpid   = 1;
    l2_ctrl_mask.s.cos     = 1;
    l2_ctrl_mask.s.spcl_fwd = 1;
    l2_ctrl_mask.s.learn_dis = 1;

    memset(&l2_ctrl, 0, sizeof(aal_l2_specpkt_ctrl_t));
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    l2_ctrl.ldpid = AAL_LPORT_CPU_0; 
    #else
    l2_ctrl.ldpid = AAL_LPORT_CPU_1;
    #endif
    l2_ctrl.cos     = 7;
    l2_ctrl.spcl_fwd = 1;
    l2_ctrl.pkt_no_drop = 1;
    l2_ctrl.pkt_no_edit = 1;
    l2_ctrl.learn_dis = 1;
    for (spb_idx = AAL_PORT_SPB_IDX_RSVD; spb_idx <= AAL_PORT_SPB_IDX_LAN; spb_idx++) {
        /*
        ret = aal_l2_specpkt_ctrl_set(device_id, spb_idx,
            AAL_L2_SPECPKT_TYPE_UDF_0, l2_ctrl_mask, &l2_ctrl);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l2_specpkt_ctrl_set OMCI return %#x.\n", ret);
            return ret;
        }
        */
        ret = aal_l2_specpkt_ctrl_set(device_id, spb_idx,
            AAL_L2_SPECPKT_TYPE_MEF_OAM, l2_ctrl_mask, &l2_ctrl);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l2_specpkt_ctrl_set PLOAM return %#x.\n", ret);
            return ret;
        }
    }

#endif

    return CA_E_OK;

}

#endif


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
static ca_status_t __special_packet_l3_detect_init(ca_device_id_t device_id)
{
    ca_status_t                  ret = CA_E_OK;
    int                          aal_ret = AAL_E_OK;
    unsigned int                 rslt_idx = 0;
    aal_l3_specpkt_detect_mask_t l3_detect_mask;
    aal_l3_specpkt_detect_t      l3_detect;
    int                          entry_idx = 0;
    l3_cam_http_msgs_tbl_entry_t l3_cam_http;
    typedef struct http_msg_info_s {
        ca_uint8_t  byte_bmp;
        ca_uint64_t msg_hex;
        char       *msg_str;
    }http_msg_info_t;

    http_msg_info_t http_msgs[] = {
        {0xe0, (0x4745540000000000ULL), "GET"},
        {0xe0, (0x5055540000000000ULL), "PUT"},
        {0xf0, (0x4845414400000000ULL), "HEAD"},
        {0xf0, (0x504f535400000000ULL), "POST"},
        {0xfe, (0x4f5054494f4e5300ULL), "OPTIONS"},
        {0xfc, (0x44454c4554450000ULL), "DELETE"},
        {0xf8, (0x5452414345000000ULL), "TRACE"},
        {0xfe, (0x434f4e4e45435400ULL), "CONNECT"},
        {0xfc, (0x554e4c494e4b0000ULL), "UNLINK"},
        {0xf8, (0x5041544348000000ULL), "PATCH"}
    };

    memset(&l3_detect_mask, 0, sizeof(l3_detect_mask));
    memset(&l3_detect, 0, sizeof(aal_l3_specpkt_detect_t));

    l3_detect_mask.s.my_mac = 1;
    l3_detect_mask.s.my_mac_detct_enable = 1;
    l3_detect_mask.s.udp_lite_detct_enable = 1;

    memcpy(&l3_detect.my_mac, &g_special_packet_drv.my_mac, 6);
    l3_detect.my_mac_detct_enable = 1;
    l3_detect.udp_lite_detct_enable = 1;

    ret = aal_l3_specpkt_detect_set(device_id, l3_detect_mask, &l3_detect);
    if (CA_E_OK != ret)
        __SPECPKT_DBG("aal_l3_specpkt_detect_set fail, return %#x.\n", ret);

#if 0 /* Add by the interface creating, not here */
    /* Init MyMAC DA CAM */
    memset(&l3_cam_mac, 0, sizeof(l3_cam_mac_da_tbl_entry_t));
    l3_cam_mac.mac_0 = g_special_packet_drv.my_mac[0];
    l3_cam_mac.mac_1 = g_special_packet_drv.my_mac[1];
    l3_cam_mac.mac_2 = g_special_packet_drv.my_mac[2];
    l3_cam_mac.mac_3 = g_special_packet_drv.my_mac[3];
    l3_cam_mac.mac_4 = g_special_packet_drv.my_mac[4];
    l3_cam_mac.mac_5 = g_special_packet_drv.my_mac[5];
    l3_cam_mac.vld   = 1;

    for (entry_idx = 0; entry_idx < L3_CAM_MAC_DA_TBL_ENTRY_MAX; ++entry_idx) {

        aal_ret = aal_entry_add(AAL_TABLE_L3_CAM_MAC_DA, &l3_cam_mac, &rslt_idx);
        if (AAL_E_OK != aal_ret) {
            __SPECPKT_DBG("aal_entry_add CAM_MAC_DA,return %#x.\n", aal_ret);
            return CA_E_FULL;
        }

        if (l3_cam_mac.mac_5 < 0xFF) {
            l3_cam_mac.mac_5++;
        } else if (l3_cam_mac.mac_4 < 0xFF) {
            l3_cam_mac.mac_5 = 0;
            l3_cam_mac.mac_4++;
        } else if (l3_cam_mac.mac_3 < 0xFF) {
            l3_cam_mac.mac_5 = 0;
            l3_cam_mac.mac_4 = 0;
            l3_cam_mac.mac_3++;
        } else if (entry_idx < L3_CAM_MAC_DA_TBL_ENTRY_MAX) {
            __SPECPKT_DBG("Wrong MyMAC in STARTUP_CFG.\n");
            return CA_E_NOT_SUPPORT;
        }

    }
#endif

    /* Init HTTP Msg CAM */
    memset(&l3_cam_http, 0, sizeof(l3_cam_http_msgs_tbl_entry_t));

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    return CA_E_OK;
#endif

    //aal_table_flush(AAL_TABLE_L3_CAM_MAC_DA);
    aal_table_flush(AAL_TABLE_L3_CAM_HTTP_MSGS);

    for (entry_idx = 0;
        entry_idx < sizeof(http_msgs)/sizeof(http_msg_info_t) &&
        entry_idx < L3_CAM_HTTP_MSGS_TBL_ENTRY_MAX;
        ++entry_idx) {

        l3_cam_http.byte_en  = http_msgs[entry_idx].byte_bmp;
        l3_cam_http.http_msg = http_msgs[entry_idx].msg_hex;
        l3_cam_http.vld = 1;

        aal_ret = aal_entry_add(AAL_TABLE_L3_CAM_HTTP_MSGS, &l3_cam_http, &rslt_idx);
        if (AAL_E_OK != aal_ret) {
            __SPECPKT_DBG("aal_entry_add CAM_HTTP_MSGS,return %#x.\n", aal_ret);
            return CA_E_FULL;
        }
    }

    return ret;

}
#endif

static ca_status_t __special_packet_detect_init(ca_device_id_t  device_id)
{
    ca_status_t  ret = CA_E_OK;

    ret = aal_l2_specpkt_init(device_id);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("aal_l2_specpkt_init, return %#x.\n", ret);
        return ret;
    }

    ret = __special_packet_l2_detect_init(device_id);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("__special_packet_l2_detect_init, return %#x.\n", ret);
        return ret;
    }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    ret = aal_l3_specpkt_init(device_id);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("aal_l3_specpkt_init, return %#x.\n", ret);
        return ret;
    }

    ret = __special_packet_l3_detect_init(device_id);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("__special_packet_l3_detect_init, return %#x.\n", ret);
        return ret;
    }
#endif

    return ret;

}

static ca_status_t __special_packet_cfg_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_port_id_t source_port_id, cpu_port_id;
    ca_special_packet_option_t options = {0};

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    return CA_E_OK;
#elif defined(CONFIG_LUNA_G3_SERIES)
    return CA_E_OK;
#else
    __SPECPKT_DBG("Start do special packet cfg init.\r\n");
#endif

    /* for WAN port bridge mode support */
#ifdef CONFIG_ARCH_CORTINA_G3LITE
    /* ARP packet, WAN -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_WAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #endif
	// FIXME SD8
    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_ARP, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for ARP from WAN, return %#x.\n", ret);
        return ret;
    }
#endif

    /* ARP packet, LAN -> L2FE -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_LAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #endif 

    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_ARP, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for ARP from LAN, return %#x.\n", ret);
        return ret;
    }

    /* NDP packet, WAN -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_WAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #endif

    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_ICMPV6_NDP, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for NDP from WAN, return %#x.\n", ret);
        return ret;
    }

    /* NDP packet, LAN -> L2FE -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_LAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #endif

    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_ICMPV6_NDP, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for NDP from LAN, return %#x.\n", ret);
        return ret;
    }

#ifdef CONFIG_ARCH_CORTINA_G3LITE
    /* PPPoE Discovery packet, WAN -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_WAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #endif
	// FIXME SD8
    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_PPPOE_DIS, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for PPPoE Discovery from WAN, return %#x.\n", ret);
        return ret;
    }

    /* PPPoE Discovery packet, LAN -> L2FE -> L3FE -> CPU */
    source_port_id = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_LAN);
    #ifndef CONFIG_RTL_MULTI_ETH_WAN
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_1);
    #else
    cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, AAL_LPORT_CPU_0);
    #endif
	// FIXME SD8
    ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_PPPOE_DIS, CA_TRUE, cpu_port_id, 6, &options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Fail to init special packet for PPPoE Discovery from LAN, return %#x.\n", ret);
        return ret;
    }
#endif

    return ret;
}


/**
 * @brief  Set a Special Packet's configurations for given Special Packet index.
 *
 * @param[in] device_id  CA device number
 * @param[in] source_port  Source Port Identifier
 * @param[in] special_packet  Special Packet type
 * @param[in] special_forward  0 = forward as normal packet, 1 = forward as special packet
 * @param[in] dest_port  Destination port.
 * @param[in] priority  Priority queue index, only valid for forward_to_cpu is 1
 * @param[in] action_handle_valid   1 = action_handle parameter is valid, 0 = ignore action_handle parameter
 * @param[in] action_handle   Used to set maximum rate (in packets per second) allowed for this special packet type. 
 *                            Assign an un-used flow id here and setup policing rate with ca_flow_policer_set.
 *                            To set aggregated special packet rate limit (rate limit enforced on total rate of all special 
 *                            packets), set special_packet parameter to CA_RX_PKT_TYPE_CONTROL_ALL.
 *                            Use action_handle to forward special packets to GEM port (when dest_port is of type GPON) or LLID (when dest_port is of type EPON). 
 * 
 * @return   opreation result, refer to the following:
 *           CA_E_OK    Operation complete successfully
 *           CA_E_XXX Operation Failed
 */

// #define 
ca_status_t ca_special_packet_set (
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_port_id_t     source_port,
    CA_IN ca_pkt_type_t    special_packet_type,
    CA_IN ca_boolean_t     enable,
    CA_IN ca_port_id_t     dest_port,
    CA_IN ca_uint8_t       priority,
    CA_IN ca_special_packet_option_t   *options
)
{
    ca_status_t            ret         = CA_E_OK;
    ca_boolean_t           is_l2_pkt   = 0;
    ca_boolean_t           is_l3_pkt   = 0;
    aal_l2_specpkt_type_t  l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
    aal_l3_specpkt_type_t  l3_pkt_type = AAL_L3_SPECPKT_TYPE_INVALID;
    ca_uint16_t            flow_id = CA_FLOW_ID_INVALID;
    ca_uint32_t            port_type = CA_PORT_TYPE_INVALID;
    ca_uint32_t            port_id = 0;
    ca_port_id_t           d_port = dest_port;
    ca_uint8_t             cos = priority;
    ca_pkt_type_t    special_packet = special_packet_type;
    ca_classifier_handle_t  *action_handle;
    ca_boolean_t special_forward = enable;
    ca_boolean_t     action_handle_valid;

    port_id   = PORT_ID(dest_port);
    port_type = PORT_TYPE(dest_port);

    CA_DEV_ID_CHECK(device_id);
    CA_COS_CHECK(priority);
    CA_ASSERT_RET(special_packet < CA_PKT_TYPE_MAX, CA_E_PARAM);
    CA_ASSERT_RET(special_forward <= 1, CA_E_PARAM);
    __SPECPKT_SRC_PORT_CHK(source_port);
    CA_ASSERT_RET(options, CA_E_PARAM);

    action_handle_valid = options->mask.action_handle;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) 
    if (CA_PORT_IS_PON(dest_port) && action_handle_valid == FALSE){
        __SPECPKT_DBG("action_handle_valid should be true when pon\r\n");
        return CA_E_PARAM;
    }
#endif

    action_handle = &options->action_handle;

    if (special_forward)
        CA_PORT_ID_CHECK(dest_port);
    

    if (action_handle_valid == FALSE)
        flow_id = CA_FLOW_ID_INVALID;

    if (action_handle_valid){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (port_type == CA_PORT_TYPE_EPON){
            flow_id = __SPECPKT_ACTION_HANDLE_FLOWID(action_handle->llid_cos_index);
            d_port = __SPECPKT_ACTION_HANDLE_INDEX(action_handle->llid_cos_index);
            cos = __SPECPKT_ACTION_HANDLE_COS(action_handle->llid_cos_index);
        }else if (port_type == CA_PORT_TYPE_GPON){
            flow_id = __SPECPKT_ACTION_HANDLE_FLOWID(action_handle->gem_index);    
            d_port = __SPECPKT_ACTION_HANDLE_INDEX(action_handle->gem_index);
            cos = __SPECPKT_ACTION_HANDLE_COS(action_handle->gem_index);

        }else{
            flow_id = action_handle->flow_id;

        }
#else
        flow_id = action_handle->flow_id;
#endif
    }

    ret = __special_packet_type_xlat(special_packet, source_port, &l2_pkt_type, &l3_pkt_type, &is_l2_pkt, &is_l3_pkt);

    __SPECPKT_DBG("device_id %u, source_port %#x, special_packet %u,"\
        "special_forward %u d_port %#x cos %u flow_id %u. l2_pkt_type:%d/0x%x, l3_pkt_type:%d/0x%x\n",
        device_id, source_port, special_packet, special_forward, d_port,
        cos, flow_id, is_l2_pkt, l2_pkt_type, is_l3_pkt, l3_pkt_type);

    if (CA_E_OK != ret) {
        __SPECPKT_DBG("invoke __special_packet_type_xlat return %#x.\n", ret);
        return ret;
    }

    if (flow_id != CA_FLOW_ID_INVALID) {
        if ((is_l2_pkt) && (flow_id > CA_AAL_MAX_FLOW_ID)) {
            __SPECPKT_DBG("is_l2_pkt invalid flow_id:%d\r\n", flow_id);
            return CA_E_PARAM;
        }
        if ( (is_l3_pkt) &&
             ((flow_id < CA_PILICER_L3_FLOW_ID_BASE) ||
              (flow_id > CA_PILICER_L3_FLOW_ID_BASE + CA_AAL_L3_MAX_FLOW_ID)) ) {
            __SPECPKT_DBG("is_l3_pkt invalid flow_id:%d\r\n", flow_id);
            return CA_E_PARAM;
        }
    }

    __SPECPKT_LOCK(device_id);

    if (special_packet_type == CA_PKT_TYPE_USER_DEFINED){
        __special_packet_udf_set(device_id, enable, options);
        //goto __end;
    }

    ret = __special_packet_ctrl_set(device_id, source_port, is_l3_pkt, is_l2_pkt,
        l3_pkt_type, l2_pkt_type, special_forward, d_port, cos, flow_id, options);
    if (CA_E_OK != ret) {
        __SPECPKT_DBG("Invoke __special_packet_ctrl_set return %#x.\n", ret);
        goto __end;
    }

    g_special_packet_drv.special_packet_to_cpu[special_packet] = special_forward;

__end:
    __SPECPKT_UNLOCK(device_id);

    return ret;

}


/**
 * @brief  Get a Special Packet's configurations for given Special Packet index.
 *
 * @param[in]  device_id  CS device ID
 * @param[in]  source_port  Source Port Identifier
 * @param[in]  special_packet  Special Packet type
 * @param[out] forward_to_cpu  Enable forwarding to CPU
 * @param[out] cpu_port  Destination CPU port
 * @param[out] priority  Priority queue index
 * @param[out] flow_id  Used to set maximum rate (in packets per second) allowed for this
 *                      special packet type. Assign an un-used flow id here and setup
 *                      policing rate with ca_flow_policer_set.
 *                      To set aggregated special packet rate limit (rate limit enforced
 *                      on total rate of all special packets), set special_packet
 *                      parameter to CA_PKT_TYPE_CONTROL_ALL.
 *                      To disable rate limit, set its value to CA_FLOW_ID_INVALID.
 *
 * @return   opreation result, refer to the following:
 *           CA_E_OK    Operation complete successfully
 *           CA_E_XXX Operation Failed
 */
ca_status_t ca_special_packet_get (
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t     source_port,
    CA_IN  ca_pkt_type_t    special_packet_type,
    CA_OUT ca_boolean_t     *enable,
    CA_OUT ca_port_id_t     *dest_port,
    CA_OUT ca_uint8_t       *priority,
    CA_OUT ca_special_packet_option_t  *options
)
{
    ca_boolean_t           is_l2_pkt = 0;
    aal_l2_specpkt_type_t  l2_pkt_type = AAL_L2_SPECPKT_TYPE_INVALID;
    aal_ilpb_cfg_t         ilpb_cfg;
    aal_l2_specpkt_ctrl_t  l2_pkt_cfg;
    ca_pkt_type_t    special_packet = special_packet_type;
    ca_port_type_t   port_type;
    ca_classifier_handle_t  *action_handle;

    ca_boolean_t           is_l3_pkt = 0;
    aal_l3_specpkt_type_t  l3_pkt_type = AAL_L3_SPECPKT_TYPE_INVALID;
#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

    aal_l3_specpkt_ctrl_t  l3_pkt_cfg;
    ca_uint8_t     lpb_profile = 0;
    L3FE_STG0_CTRL_t stg0_ctrl;
    l3_stg0_ldpid_map_t stg0_ldpid_map;
    aal_l3fe_lpb_tbl_cfg_t  lpb_tbl_cfg;
#endif
    ca_status_t            ret = CA_E_OK;
    ca_uint32_t            spid = __SPECPKT_CAPORT_TO_PHYSICALPORT(source_port);
    ca_boolean_t           not_support = TRUE;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint8_t             pon_mode = 0xFF;
#endif

    __SPECPKT_DBG("device_id %u, source_port %#x, special_packet %u,"\
        "enable %p dest_port %p priority %p options %p.\n",
        device_id, source_port, special_packet, enable, dest_port,
        priority, options);

    CA_DEV_ID_CHECK(device_id);
    CA_ASSERT_RET(enable && dest_port && priority && options, CA_E_PARAM);

    __SPECPKT_SRC_PORT_CHK(source_port);

    __SPECPKT_TYPE_CHK(special_packet);

    action_handle = &options->action_handle;

    ret = __special_packet_type_xlat(special_packet, source_port, &l2_pkt_type, &l3_pkt_type, &is_l2_pkt, &is_l3_pkt);

    if (CA_E_OK != ret) {
        __SPECPKT_DBG("invoke __special_packet_type_xlat return %#x.\n", ret);
        return ret;
    }

    __SPECPKT_LOCK(device_id);

    if (is_l3_pkt && (l3_pkt_type != AAL_L3_SPECPKT_TYPE_INVALID)) {
        not_support = FALSE;
#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

        /* get lpb_idx_mode for correct source of lpb_profile */
        if ((ret = aal_l3fe_stg0_ctrl_get(device_id, &stg0_ctrl)) != AAL_E_OK) {
            ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
            return ret;
        }

        if (stg0_ctrl.bf.lpb_idx_mode == 0) {
            /* get lpb_profile from L3FE_STG0_LDPID_MAP */

            if ((ret = aal_l3_stg0_ldpid_map_get(device_id, &stg0_ldpid_map)) != CA_E_OK) {
                ca_printf("%s: Fail to get STG0 LDPID map. (ret=%d)\n", __func__, ret);
                return ret;
            }

            if (spid == stg0_ldpid_map.rule_0)
                lpb_profile = 0;
            else if (spid == stg0_ldpid_map.rule_1)
                lpb_profile = 1;
            else if (spid == stg0_ldpid_map.rule_2)
                lpb_profile = 2;
            else
                lpb_profile = 3;

        } else {
            /* get lpb_profile from L3FE_STG0_LPB_IDX_TBL */

            ret = aal_l3fe_lpb_idx_tbl_get(device_id, spid, &lpb_profile);
            if (CA_E_OK != ret) {
                __SPECPKT_DBG("Invoke aal_l3fe_lpb_idx_tbl_get return %#x.\n", ret);
                return ret;
            }

        }

        memset(&lpb_tbl_cfg, 0, sizeof(lpb_tbl_cfg));
        ret = aal_l3_stg0_lpb_tbl_get(device_id, lpb_profile, &lpb_tbl_cfg);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("Invoke aal_l3_stg0_lpb_tbl_get return %#x.\n", ret);
            return ret;
        }

        memset(&l3_pkt_cfg, 0, sizeof(l3_pkt_cfg));
        ret = aal_l3_specpkt_ctrl_get(device_id,
            lpb_tbl_cfg.spcl_pkt_sel, l3_pkt_type, &l3_pkt_cfg);
#endif
    }

    if (is_l2_pkt && (l2_pkt_type != AAL_L2_SPECPKT_TYPE_INVALID)) {
        not_support = FALSE;

        if (special_packet_type == CA_PKT_TYPE_USER_DEFINED){
            __special_packet_udf_get(device_id, options);
            //goto __end;
        }
        memset(&ilpb_cfg, 0, sizeof(ilpb_cfg));
        ret = aal_port_ilpb_cfg_get(device_id, spid, &ilpb_cfg);
        if (CA_E_OK != ret) {
            __SPECPKT_DBG("invoke aal_port_ilpb_cfg_get return %#x.\n", ret);
            return ret;
        }

        memset(&l2_pkt_cfg, 0, sizeof(l2_pkt_cfg));
        ret = aal_l2_specpkt_ctrl_get(device_id,
            ilpb_cfg.spcl_pkt_idx, l2_pkt_type, &l2_pkt_cfg);
    }

    if (not_support)
        ret = CA_E_NOT_SUPPORT;

    if (CA_E_OK != ret)
        goto __end;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
        ca_printf("<%s, %d> read CFG_ID_PON_MAC_MODE fail\r\n", __func__, __LINE__);
    }
#endif

    if (is_l3_pkt) {
#if defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)

        *enable = __SPECPKT_PORT_IS_CPU(l3_pkt_cfg.ldpid);
        port_type = aal_port_type_get(l3_pkt_cfg.ldpid);
        *dest_port = __SPECPKT_PHYSICALPORT_TO_CAPORT(port_type, l3_pkt_cfg.ldpid);
        *priority = l3_pkt_cfg.cos;

        if (l3_pkt_cfg.pol_en_sel == AAL_L3_SPECPKT_POL_DISABLE) {
            action_handle->flow_id = CA_FLOW_ID_INVALID;
            goto __end;
        }

        action_handle->flow_id = l3_pkt_cfg.pol_id | CA_PILICER_L3_FLOW_ID_BASE;

#endif
    }  else {

        *enable = l2_pkt_cfg.spcl_fwd;
        *priority = l2_pkt_cfg.cos;
        options->mask.action_handle = TRUE;
        options->do_not_drop = l2_pkt_cfg.pkt_no_drop;
        options->enable_sa_learning = !l2_pkt_cfg.learn_dis;
        options->forward_original = l2_pkt_cfg.pkt_no_edit;
        action_handle->flow_id = l2_pkt_cfg.flowid;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (__SPECPKT_IS_PON_INDEX(l2_pkt_cfg.ldpid)){
            if(CA_PON_MODE_IS_EPON_FAMILY(pon_mode)){
                *dest_port = CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7);
                action_handle->llid_cos_index = __SPECPKT_ACTION_HANDLE_PON_INDEX(l2_pkt_cfg.ldpid - 0x20, l2_pkt_cfg.cos, l2_pkt_cfg.flowid);
            }else if(CA_PON_MODE_IS_GPON_FAMILY(pon_mode)){
                *dest_port = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
                action_handle->gem_index = __SPECPKT_ACTION_HANDLE_PON_INDEX(l2_pkt_cfg.ldpid - 0x20, l2_pkt_cfg.cos, l2_pkt_cfg.flowid);
            }else if(CA_PON_MODE_IS_ETHERNET(pon_mode)){
                *dest_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI7);
                action_handle->flow_id = l2_pkt_cfg.flowid;
            }
        }else
#endif
        {
            port_type = aal_port_type_get(l2_pkt_cfg.ldpid);

            *dest_port = __SPECPKT_PHYSICALPORT_TO_CAPORT(port_type, l2_pkt_cfg.ldpid);
            action_handle->flow_id = l2_pkt_cfg.flowid;
        }



        if (l2_pkt_cfg.rate_limit_bypass || !l2_pkt_cfg.flow_en) {
            options->mask.action_handle = FALSE;
            action_handle->flow_id = CA_FLOW_ID_INVALID;
            goto __end;
        }

    }

__end:
    __SPECPKT_UNLOCK(device_id);

    __SPECPKT_DBG("return: device_id %u, source_port %#x, special_packet %u,"\
        "forward_to_cpu %u cpu_port %#x priority %u flow_id %u.\n",
        device_id, source_port, special_packet, *enable, *dest_port,
        *priority, action_handle->flow_id);
    return ret;
}

/**
 * @ca_special_packet_init:
 * Initialize Special packets functions,
 * NOTE: ONLY used for ca_init invoking!!!
 */
ca_status_t ca_special_packet_init(
            CA_IN ca_device_id_t device_id,
            CA_IN ca_init_config_t * p_cfg)
{
    ca_status_t ret = CA_E_OK;

    if (g_special_packet_drv.init_done)
        return CA_E_OK;

    /* Global data init */
    __special_packet_global_init(device_id);


    /* Special packet parser/detect init */
    ret = __special_packet_detect_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("__special_packet_detect_init fail, return %#x.\n", ret);
        goto __err_end;
    }


    /* Configuration init */
    ret = __special_packet_cfg_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("__special_packet_cfg_init fail, return %#x.\n", ret);
        goto __err_end;
    }

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_ARCH_CORTINA_VENUS)
	// no need saturn special packet rules
#else
    ret = __special_packet_saturn_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("__special_packet_saturn_init fail, return %#x.\n", ret);
        goto __err_end;
    }
#endif
#endif

    g_special_packet_drv.init_done = TRUE;
    ca_printf("ca-ne: init special packet OK\n");
    return CA_E_OK; 

__err_end:
    __special_packet_global_exit(device_id);
    return ret;

}

#ifdef __KERNEL__

EXPORT_SYMBOL(ca_special_packet_set);
EXPORT_SYMBOL(ca_special_packet_get);

#endif

