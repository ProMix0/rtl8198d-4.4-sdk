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
#include "api_common.h"
#include "ca_event.h"
#include "port.h"
#include "vlan.h"
#include "queue.h"
#include "aal_port.h"
#include "aal_ni_l2.h"
#include "aal_ni.h"
#include "aal_phy.h"
#include "eth_port.h"
#include "aal_l2_tm.h"
#include "aal_l3_tm.h"
#include "osal_common.h"
#include "ca-scfg/scfg.h"
#if defined(CONFIG_NE_PON)
#include "aal_xfi.h"
#include "aal_pon.h"
#include "aal_epon.h"
#include "aal_uni_macsec.h"
#include "aal_epon_crypto.h"

#define CA_PORT_EPON_LLID_LPID_START    AAL_LPORT_LLID_GEM_INDEX_0
#define CA_PORT_EPON_LLID_LPID_END      AAL_LPORT_LLID_GEM_INDEX_31

#if defined(CONFIG_LUNA_G3_SERIES)
extern ca_uint8_t g_admin_state;
#endif
#endif

#define __PORT_DBG(fmt,...)  do {\
                                if (ca_api_debug & API_DBG_CMN_PORT)\
                                    ca_printf("%s():"fmt, __func__,##__VA_ARGS__);\
                            }while(0)

extern ca_status_t  ca_eth_port_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_boolean_t    enable);

extern ca_status_t  ca_eth_port_enable_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_OUT ca_boolean_t * enable);

ca_uint __port_lock = 0;

#define __PORT_LOCK(dev)           ca_spin_lock(__port_lock)
#define __PORT_UNLOCK(dev)         ca_spin_unlock(__port_lock)

typedef struct {
    ca_boolean_t valid;    /* created or not */
    ca_uint32_t active_members_num;
    ca_uint32_t active_members[CA_PORT_MAX_NUM_TRUNK_MEMBER];    /* the lpid of active members, used for trunk dest map table update */
    ca_uint64_t members_bmp;
} __port_lag_attr_t;

__port_lag_attr_t __port_lag_attr[CA_PORT_MAX_NUM_TRUNK];    /* array index 0 used for group0, and index 1 used for group1   */

ca_uint32_t __port_tpid_index[CA_PORT_MAX_NUM];     /* pvid tpid set */

ca_boolean_t __port_stp_mode[CA_PORT_MAX_NUM];       /* stp is disable or enable */

ca_uint32_t __port_ipg_value = 12; /* for NI5 XAUI port */

/* Special packet behavior table index in ILPB Table */
#define __PORT_IS_LAN(port) (((port) <= AAL_LPORT_ETH_NI7) && ((port) != AAL_LPORT_ETH_WAN))
#define __PORT_IS_WAN(port) ((port) == AAL_LPORT_ETH_WAN)

/* get ca_port_id_t (port_type + port_index) based on lpid */
ca_port_id_t ca_port_id_get(ca_uint32_t lpid)
{
    ca_uint16_t port_type = CA_PORT_TYPE_INVALID;

    if (lpid >= CA_PORT_ID_NI0 && lpid <= CA_PORT_ID_NI7) {
        port_type = CA_PORT_TYPE_ETHERNET;

    }else if (lpid == CA_PORT_ID_L3_LAN || CA_PORT_ID_L3_WAN) {
        port_type = CA_PORT_TYPE_L2RP;

    } else if (lpid >= CA_PORT_ID_CPU0 && lpid <= CA_PORT_ID_CPU7) {
        port_type = CA_PORT_TYPE_CPU;

        //} else if (lpid >= CA_PORT_ID_GROUP0 && lpid <= CA_PORT_ID_GROUP1) {
        //port_type = CA_PORT_TYPE_TRUNK;

        //} else if (lpid == CA_PORT_ID_MC) {
        //port_type = CA_PORT_TYPE_MC;

    }

    return CA_PORT_ID(port_type, lpid);
}

#if 0
/* only support ethernet ports enable/disable */
ca_status_t ca_port_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    aal_ni_eth_mac_config_mask_t ge_msk;
    aal_ni_eth_mac_config_t ge_cfg;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ni_xaui_mac_tx_cfg_mask_t xaui_tx_msk;
    aal_ni_xaui_mac_tx_cfg_t xaui_tx_cfg;
    aal_ni_xaui_mac_rx_cfg_mask_t xaui_rx_msk;
    aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;
#endif
    ca_uint8_t phy_addr[8] = {/* FPGA board has 8 Eth Ports */
        1, 2, 3, 4, 16, 17, 18, 19
    };
#ifndef CONFIG_CORTINA_BOARD_FPGA
    ca_device_config_tlv_t scfg_tlv;
#endif
    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);
    CA_BOOL_VAL_CHECK(enable);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

#ifndef CONFIG_CORTINA_BOARD_FPGA
    if (ppid <= AAL_LPORT_ETH_NI4) {
        /* Read PHY MDIO address from STARTUP config */
        memset(&scfg_tlv, 0, sizeof(scfg_tlv));
        scfg_tlv.type = CA_CFG_ID_PHY_BASE_ADDR;
        scfg_tlv.len = CFG_ID_PHY_BASE_ADDR_LEN;

        ret = ca_device_config_tlv_get(device_id, &scfg_tlv);
        if (CA_E_OK == ret) {
#ifdef _DEBUG_
            ca_printf("get phy addr from scfg \r\n");
#endif
            memcpy(phy_addr, scfg_tlv.value, CFG_ID_PHY_BASE_ADDR_LEN);
        }
    }
#endif

#if 0
    /* Leave GPHY in power on mode always */
    ca_printf("set port %d (phy addr 0x%x) power enable to %d\r\n", ppid, phy_addr[ppid], enable);
    ret = aal_eth_phy_power_set(device_id, phy_addr[ppid], enable);
    if (ret != CA_E_OK) {
        ca_printf("set port %d (phy addr 0x%x) power enable failed. ret = %d\r\n", ppid, phy_addr[ppid], ret);
        return ret;
    }
#endif

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    if(ppid >= AAL_LPORT_ETH_NI3 && ppid <= AAL_LPORT_ETH_NI4){ /* GE */
#else
    if(ppid >= AAL_LPORT_ETH_NI0 && ppid <= AAL_LPORT_ETH_NI4){ /* GE */
#endif
        memset(&ge_msk, 0, sizeof(aal_ni_eth_mac_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));

        ge_msk.s.mac_rx_rst = 1;
        ge_msk.s.rx_en = 1;
        ge_msk.s.tx_en = 1;
        ge_cfg.mac_rx_rst = !enable;
        ge_cfg.rx_en = enable;
        ge_cfg.tx_en = enable;

        ret = aal_ni_eth_port_mac_set(device_id, ppid, ge_msk, &ge_cfg);
        if (ret != CA_E_OK) {
            ca_printf("set port %d mac enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
    }
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    else if (ppid >= AAL_LPORT_ETH_NI5 && ppid <= AAL_LPORT_ETH_NI6) {

        aal_xfi_cfg_mask_t   cfg_mask;
        aal_xfi_cfg_t        cfg;

        cfg_mask.data32 = 0;
        memset(&cfg, 0, sizeof(cfg));

        cfg_mask.s.pdown = 1;
        cfg_mask.s.rx_en = 1;

        cfg.pdown = !enable;
        cfg.rx_en = enable ? 1 : 0;
        //ca_printf("%s(): %d\n", __func__, __LINE__);
        ret = aal_xfi_cfg_set(device_id, ppid - AAL_LPORT_ETH_NI5,
            cfg_mask, &cfg);
        if (ret != CA_E_OK) {
            ca_printf("aal_xfi_cfg_set %d failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
        else {
            ca_printf("XGE %d %s!\n", ppid, enable ? "enable" : "disable");
        }
    }else if(ppid == AAL_LPORT_ETH_NI7){/* PON port */
        return aal_pon_mac_enable_set(device_id, port_id, enable, enable);
    }
#elif defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    else if(ppid == AAL_LPORT_ETH_NI5){    /* XAUI */
        memset(&xaui_tx_msk, 0, sizeof(aal_ni_xaui_mac_tx_cfg_mask_t));
        memset(&xaui_tx_cfg, 0, sizeof(aal_ni_xaui_mac_tx_cfg_t));

        xaui_tx_msk.s.mac_en  = 1;
        xaui_tx_cfg.mac_en = enable;
        ret = aal_ni_xaui_mac_tx_config_set(device_id, xaui_tx_msk, &xaui_tx_cfg);
        if (ret != CA_E_OK) {
            ca_printf("set port %d mac tx enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
        memset(&xaui_rx_msk, 0, sizeof(aal_ni_xaui_mac_rx_cfg_mask_t));
        memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));

        xaui_rx_msk.s.mac_en = 1;
        xaui_rx_cfg.mac_en = enable;
        ret = aal_ni_xaui_mac_rx_config_set(device_id, xaui_rx_msk, &xaui_rx_cfg);
        if (ret != CA_E_OK) {
            ca_printf("set port %d mac rx enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    }else if(ppid == AAL_LPORT_ETH_NI6){ /* XGE */

#else
    }else if(ppid == AAL_LPORT_ETH_NI6 || ppid == AAL_LPORT_ETH_NI7){ /* XGE */
#endif
        aal_ni_xge_cmn_cfg_mask_t   cfg_mask;
        aal_ni_xge_cmn_cfg_t        cfg;

        cfg_mask.u32 = 0;
        memset(&cfg, 0, sizeof(cfg));

        cfg_mask.s.rx_rst = 1;
        cfg_mask.s.tx_rst = 1;
        cfg.rx_rst = !enable;
        cfg.tx_rst = !enable;
        ret = aal_ni_xge_cmn_config_set(device_id, ppid - AAL_LPORT_ETH_NI6,
            cfg_mask, &cfg);
        if (ret != CA_E_OK) {
            ca_printf("set port %d mac enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
    }
#endif

    eth_port_enable_cfg_sync(device_id, ppid, enable);


    return ret;
}

ca_status_t ca_port_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t * enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    aal_ni_eth_mac_config_t ge_cfg;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ni_xaui_mac_tx_cfg_t xaui_tx_cfg;
    aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;
#endif
    ca_boolean mac_en, mac_rx_en, mac_tx_en;
    ca_boolean phy_en = 1;

    ca_uint8_t phy_addr[8] = {/* FPGA board has 8 Eth Ports */
        1, 2, 3, 4, 16, 17, 18, 19
    };
#ifndef CONFIG_CORTINA_BOARD_FPGA
    ca_device_config_tlv_t scfg_tlv;
#endif
    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

#ifndef CONFIG_CORTINA_BOARD_FPGA
    /* Read PHY MDIO address from STARTUP config */
    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_PHY_BASE_ADDR;
    scfg_tlv.len = CFG_ID_PHY_BASE_ADDR_LEN;

    ret = ca_device_config_tlv_get(device_id, &scfg_tlv);
    if (CA_E_OK == ret) {
#ifdef _DEBUG_
        ca_printf("get phy addr from scfg \r\n");
#endif
        memcpy(phy_addr, scfg_tlv.value, CFG_ID_PHY_BASE_ADDR_LEN);
    }
#endif
#if 0 /* phy is always on */
    ret = aal_eth_phy_power_get(device_id, phy_addr[ppid], &phy_en);
    if (ret != CA_E_OK) {
        ca_printf("get port %d (phy addr 0x%x) power enable failed. ret = %d\r\n", ppid, phy_addr[ppid], ret);
        return ret;
    }
    //ca_printf("get port %d (phy addr 0x%x) power enable: %d\r\n", ppid, phy_addr[ppid], phy_en);
#endif
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    if(ppid >= AAL_LPORT_ETH_NI3 && ppid <= AAL_LPORT_ETH_NI4){ /* GE */
#else
    if(ppid >= AAL_LPORT_ETH_NI0 && ppid <= AAL_LPORT_ETH_NI4){ /* GE */
#endif
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));

        ret = aal_ni_eth_port_mac_get(device_id, ppid, &ge_cfg);
        if(ret != CA_E_OK){
            ca_printf("get port %d mac enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
        mac_en = ge_cfg.rx_en;
    }
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    else if (ppid >= AAL_LPORT_ETH_NI5 && ppid <= AAL_LPORT_ETH_NI6) {
        aal_xfi_cfg_t        cfg;

        memset(&cfg, 0, sizeof(cfg));
        ret = aal_xfi_cfg_get(device_id, ppid - AAL_LPORT_ETH_NI5,
            &cfg);
        if (ret != CA_E_OK) {
            ca_printf("get port %d mac enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }

        mac_en = !cfg.pdown;
    }else if(ppid == AAL_LPORT_ETH_NI7){ /* PON port */
        aal_pon_mac_enable_get(device_id, port_id, &mac_tx_en, &mac_rx_en);
        mac_en = (mac_tx_en && mac_rx_en);
    }
#elif defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    else if(ppid == AAL_LPORT_ETH_NI5){    /* XAUI */
        memset(&xaui_tx_cfg, 0, sizeof(aal_ni_xaui_mac_tx_cfg_t));
        ret = aal_ni_xaui_mac_tx_config_get(device_id, &xaui_tx_cfg);
        if(ret != CA_E_OK){
            ca_printf("get port %d mac tx enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
        memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));
        ret = aal_ni_xaui_mac_rx_config_get(device_id, &xaui_rx_cfg);
        if(ret != CA_E_OK){
            ca_printf("get port %d mac rx enable failed. ret = %d\r\n", ppid, ret);
            return ret;
        }
        mac_en = (xaui_tx_cfg.mac_en && xaui_rx_cfg.mac_en);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    }else if(ppid == AAL_LPORT_ETH_NI6) { /* XGE */
#else
    }else if(ppid == AAL_LPORT_ETH_NI6 || ppid == AAL_LPORT_ETH_NI7){ /* XGE */
#endif
        mac_en = 1;
    }
    //ca_printf("get port %d mac enable: %d\r\n", ppid, mac_en);
#endif
    *enable = mac_en & phy_en;

    return ret;
}
#endif

ca_status_t ca_port_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);
    CA_BOOL_VAL_CHECK(enable);

    ppid = PORT_ID(port_id);    /* for physical ports, the ppid is the same as lpid */

    if (CA_PORT_IS_ETHERNET(port_id)) {
        return ca_eth_port_enable_set(device_id, port_id, enable);
    }

#if defined(CONFIG_NE_PON)
    if (CA_PORT_IS_PON(port_id))
    {
        __PORT_DBG("set pon port mac enable = %d \r\n", enable);

        __PON_LOCK();
        ret = aal_pon_mac_enable_set(device_id, ppid, enable, enable);
        __PON_UNLOCK();
        if (ret != CA_E_OK)
        {
            __PORT_DBG("set pon port mac enable = %d failed. ret = %d\r\n", enable, ret);
            return ret;
        }
#ifdef CONFIG_LUNA_G3_SERIES
        g_admin_state = enable;
#endif

    }
#endif
    return ret;
}

ca_status_t ca_port_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t * enable)
{
    ca_status_t ret = CA_E_OK;
#if defined(CONFIG_NE_PON)
    ca_uint32_t ppid = PORT_ID(port_id);/* for physical ports, the ppid is the same as lpid */
    ca_boolean_t  rx_en = 0, tx_en = 0;
#endif

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);
    CA_NULL_PTR_CHECK(enable);

    if (CA_PORT_IS_ETHERNET(port_id)) {
        return ca_eth_port_enable_get(device_id, port_id, enable);
    }

#if defined(CONFIG_NE_PON)
    if (CA_PORT_IS_PON(port_id))
    {
        __PON_LOCK();
        ret = aal_pon_mac_enable_get(device_id, ppid, &tx_en, &rx_en);
        __PON_UNLOCK();
        if (ret != CA_E_OK)
        {
            ca_printf("get pon port mac failed. ret = %d\r\n", ret);
            return ret;
        }

        *enable = rx_en;
        return CA_E_OK;
    }
#endif
    return ret;
}


ca_status_t ca_port_max_frame_size_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t size,
    CA_IN ca_boolean_t include_tag)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_pkt_len_config_mask_t ge_msk;
    aal_ni_pkt_len_config_t ge_cfg;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ni_xaui_mac_rx_cfg_mask_t xaui_rx_msk;
    aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;
#endif
    aal_ni_xge_mac_frame_len_config_mask_t xge_msk;
    aal_ni_xge_mac_frame_len_config_t xge_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){
        memset(&ge_msk, 0, sizeof(aal_ni_pkt_len_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_pkt_len_config_t));

        ge_msk.s.max_pkt_size = 1;
        ge_cfg.max_pkt_size = size;
        ca_printf("aal_ni_pkt_len_set size %d \r\n", size);
        ret = aal_ni_pkt_len_set(device_id, ge_msk, &ge_cfg);
        return ret;

    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(ppid == AAL_LPORT_ETH_NI5){    /* XAUI */
            memset(&xaui_rx_msk, 0, sizeof(aal_ni_xaui_mac_rx_cfg_mask_t));
            memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));

            xaui_rx_msk.s.max_size = 1;
            xaui_rx_cfg.max_size = size;
            ca_printf("aal_ni_xaui_mac_rx_config_set size %d \r\n", size);
            ret = aal_ni_xaui_mac_rx_config_set(device_id, xaui_rx_msk, &xaui_rx_cfg);
            return ret;
        }
#endif
        xge_idx = ppid - AAL_NI_XE_START;
        memset(&xge_msk, 0, sizeof(aal_ni_xge_mac_frame_len_config_mask_t));
        memset(&xge_cfg, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));

        xge_msk.s.max_len = 1;
        xge_msk.s.maxlen_mode = 1;

        xge_cfg.max_len = size;
        xge_cfg.maxlen_mode = !include_tag;

        ca_printf("aal_ni_xge_mac_frame_len_set xge_idx %d size %d \r\n", xge_idx, size);
        ret = aal_ni_xge_mac_frame_len_set(device_id, xge_idx, xge_msk, &xge_cfg);
        return ret;
    }

#if defined(CONFIG_NE_PON)
    if (CA_PORT_IS_PON(port_id))
    {
        ca_printf("aal_pon_mac_max_frame_size_set size %d \r\n", size);
        __PON_LOCK();
        ret = aal_pon_mac_max_frame_size_set(device_id, port_id, size, size, !include_tag);
        __PON_UNLOCK();

        return ret;
    }
#endif
    return ret;
}

ca_status_t ca_port_max_frame_size_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *size,
    CA_OUT ca_boolean_t *include_tag)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_pkt_len_config_t ge_cfg;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ni_xaui_mac_rx_cfg_t xaui_rx_cfg;
#endif
    aal_ni_xge_mac_frame_len_config_t xge_cfg;
#if defined(CONFIG_NE_PON)
    ca_uint32_t rx_size, tx_size, mtu_tag_en = 0;
#endif

    CA_DEV_ID_CHECK(device_id);

    CA_ASSERT_RET(size, CA_E_PARAM);
    CA_ASSERT_RET(include_tag, CA_E_PARAM);

    CA_PORT_PHYSICAL_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_cfg, 0, sizeof(aal_ni_pkt_len_config_t));
        ret = aal_ni_pkt_len_get(device_id, &ge_cfg);

        if(ret == CA_E_OK){
            *size = ge_cfg.max_pkt_size;
            *include_tag = 1;
        }
        return ret;

    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(ppid == AAL_LPORT_ETH_NI5){    /* XAUI */
            memset(&xaui_rx_cfg, 0, sizeof(aal_ni_xaui_mac_rx_cfg_t));
            ret = aal_ni_xaui_mac_rx_config_get(device_id, &xaui_rx_cfg);
            if(ret == CA_E_OK){
                *size = xaui_rx_cfg.max_size;
            }
            return ret;
        }
#endif
        xge_idx = ppid - AAL_NI_XE_START;
        memset(&xge_cfg, 0, sizeof(aal_ni_xge_mac_frame_len_config_t));
        ret = aal_ni_xge_mac_frame_len_get(device_id, xge_idx, &xge_cfg);
        if(ret == CA_E_OK){
            *size = xge_cfg.max_len;
            *include_tag = !xge_cfg.maxlen_mode;
        }
        return ret;
    }

#if defined(CONFIG_NE_PON)
    if (CA_PORT_IS_PON(port_id))
    {
        __PON_LOCK();
        ret = aal_pon_mac_max_frame_size_get(device_id, port_id, &tx_size, &rx_size, &mtu_tag_en);
        __PON_UNLOCK();
        if (ret == CA_E_OK){
            *size = rx_size;
            *include_tag = !mtu_tag_en;
        }
        return ret;
    }
#endif
    return ret;
}

/* only support ethernet ports IPG config */
ca_status_t ca_port_ipg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t ipg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_mac_config_mask_t ge_msk;
    aal_ni_eth_mac_config_t ge_cfg;
    aal_ni_xge_cfg_tx_mask_t xge_tx_msk;
    aal_ni_xge_cfg_tx_t xge_tx_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){ /* GE */

        if (ipg < CA_PORT_IPG_GE_VAL_MIN || ipg > CA_PORT_IPG_GE_VAL_MAX) {
            return CA_E_PARAM;
        }
        memset(&ge_msk, 0, sizeof(aal_ni_eth_mac_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));

        ge_msk.s.tx_ipg_sel = 1;
        ge_cfg.tx_ipg_sel = ipg - 8;

        ret = aal_ni_eth_port_mac_set(device_id, ppid, ge_msk, &ge_cfg);

    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */

        if (ipg < CA_PORT_IPG_XGE_VAL_MIN || ipg > CA_PORT_IPG_XGE_VAL_MAX) {
            return CA_E_PARAM;
        }

        xge_idx = ppid - AAL_NI_XE_START;
        memset(&xge_tx_msk, 0, sizeof(aal_ni_xge_cfg_tx_mask_t));
        memset(&xge_tx_cfg, 0, sizeof(aal_ni_xge_cfg_tx_t));

        xge_tx_msk.s.ifg  = 1;
        xge_tx_cfg.ifg = ipg;
        ret = aal_ni_xge_tx_config_set(device_id, xge_idx, xge_tx_msk, &xge_tx_cfg);

    }

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_PORT_IS_PON(port_id))
    {
        ca_printf("aal_pon_mac_ipg_set size %d \r\n", ipg);
        __PON_LOCK();
        ret = aal_pon_mac_ipg_set(device_id, port_id, ipg);
        __PON_UNLOCK();
    }
#endif

    return ret;
}

ca_status_t ca_port_ipg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * ipg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_mac_config_t ge_cfg;
    aal_ni_xge_cfg_tx_t xge_tx_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_PHYSICAL_CHECK(port_id);

    CA_ASSERT_RET(ipg, CA_E_PARAM);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_mac_config_t));
        ret = aal_ni_eth_port_mac_get(device_id, ppid, &ge_cfg);
        if (ret == CA_E_OK) {
            *ipg = ge_cfg.tx_ipg_sel + 8;
        }
    }else if(CA_PORT_IS_XGE(port_id)){ /* XGE */
        xge_idx = ppid - AAL_NI_XE_START;
        memset(&xge_tx_cfg, 0, sizeof(aal_ni_xge_cfg_tx_t));
        ret = aal_ni_xge_tx_config_get(device_id, xge_idx, &xge_tx_cfg);
        if (ret == CA_E_OK) {
            *ipg = xge_tx_cfg.ifg;
        }
    }
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_PORT_IS_PON(port_id))
    {
        __PON_LOCK();
        ret = aal_pon_mac_ipg_get(device_id, port_id, ipg);
        __PON_UNLOCK();
    }
#endif
    return ret;
}

/* only support ethernet ports loopback config */
ca_status_t ca_port_loopback_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_port_loopback_mode_t loopback)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_if_config_mask_t ge_msk;
    aal_ni_eth_if_config_t ge_cfg;
    aal_ni_xge_cmn_cfg_mask_t xge_msk;
    aal_ni_xge_cmn_cfg_t xge_cfg;

    ca_uint8_t phy_addr[8] = {/* FPGA board has 8 Eth Ports */
        1, 2, 3, 4, 16, 17, 18, 19
    };
#ifndef CONFIG_CORTINA_BOARD_FPGA
    ca_device_config_tlv_t scfg_tlv;
#endif
    aal_ni_mac_lpbk_mode_t mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
    aal_phy_lpbk_mode_t phy_lpbk;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ETHERNET_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

    switch (loopback) {
    case CA_PORT_LOOPBACK_NONE:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
        phy_lpbk.rx2tx_enable = 0;
        phy_lpbk.tx2rx_enable = 0;
        break;

    case CA_PORT_LOOPBACK_MAC_LOCAL:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_TX2RX;
        phy_lpbk.rx2tx_enable = 0;
        phy_lpbk.tx2rx_enable = 0;
        break;

    case CA_PORT_LOOPBACK_MAC_REMOTE:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_RX2TX;
        phy_lpbk.rx2tx_enable = 0;
        phy_lpbk.tx2rx_enable = 0;
        break;

    case CA_PORT_LOOPBACK_PHY_LOCAL:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
        phy_lpbk.rx2tx_enable = 0;
        phy_lpbk.tx2rx_enable = 1;
        break;

    case CA_PORT_LOOPBACK_PHY_REMOTE:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
        phy_lpbk.rx2tx_enable = 1;
        phy_lpbk.tx2rx_enable = 0;
        break;

    default:
        mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
        phy_lpbk.rx2tx_enable = 0;
        phy_lpbk.tx2rx_enable = 0;
        break;

    }

#ifndef CONFIG_CORTINA_BOARD_FPGA
    /* Read PHY MDIO address from STARTUP config */
    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_PHY_BASE_ADDR;
    scfg_tlv.len = CFG_ID_PHY_BASE_ADDR_LEN;

    ret = ca_device_config_tlv_get(device_id, &scfg_tlv);
    if (CA_E_OK == ret) {
        memcpy(phy_addr, scfg_tlv.value, CFG_ID_PHY_BASE_ADDR_LEN);
    }
#endif
    ca_printf("set port %d (phy addr 0x%x) loopback tx2rx %d, rx2tx %d\r\n",
              ppid, phy_addr[ppid], phy_lpbk.tx2rx_enable, phy_lpbk.rx2tx_enable);

    ret = aal_eth_phy_loopback_set(device_id, phy_addr[ppid], phy_lpbk);
    if (ret != CA_E_OK) {
        ca_printf("phy loopback set error, ret %d\r\n", ret);
        //return ret;
    }

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_msk, 0, sizeof(aal_ni_eth_if_config_mask_t));
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_if_config_t));

        ge_msk.s.lpbk_mode = 1;
        ge_cfg.lpbk_mode = mac_lpbk;

        ret = aal_ni_eth_if_set(device_id, ppid, ge_msk, &ge_cfg);

    }
    else if(CA_PORT_IS_XGE(port_id)){ /* XGE */

        memset(&xge_msk, 0, sizeof(aal_ni_xge_cmn_cfg_mask_t));
        memset(&xge_cfg, 0, sizeof(aal_ni_xge_cmn_cfg_t));

        xge_msk.s.core_tx2rx_lb = 1;
        xge_msk.s.core_rx2tx_lb = 1;
        if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_TX2RX){
            xge_cfg.core_tx2rx_lb = 1;
        }else if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_RX2TX){
            xge_cfg.core_rx2tx_lb = 1;
        }
        xge_idx = ppid - AAL_NI_XE_START;
        ret = aal_ni_xge_cmn_config_set(device_id, xge_idx, xge_msk, &xge_cfg);
    }

    return ret;
}

ca_status_t ca_port_loopback_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_port_loopback_mode_t * loopback)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t ppid = 0;
    ca_uint32_t xge_idx = 0;
    aal_ni_eth_if_config_t ge_cfg;
    aal_ni_xge_cmn_cfg_t xge_cfg;
    ca_uint8_t phy_addr[8] = {/* FPGA board has 8 Eth Ports */
        1, 2, 3, 4, 16, 17, 18, 19
    };
#ifndef CONFIG_CORTINA_BOARD_FPGA
    ca_device_config_tlv_t scfg_tlv;
#endif
    aal_ni_mac_lpbk_mode_t mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
    aal_phy_lpbk_mode_t phy_lpbk;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ETHERNET_CHECK(port_id);

    ppid = PORT_ID(port_id);    /* for ethernet ports, the ppid is the same as lpid */

#ifndef CONFIG_CORTINA_BOARD_FPGA
    /* Read PHY MDIO address from STARTUP config */
    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_PHY_BASE_ADDR;
    scfg_tlv.len = CFG_ID_PHY_BASE_ADDR_LEN;

    ret = ca_device_config_tlv_get(device_id, &scfg_tlv);
    if (CA_E_OK == ret) {
        memcpy(phy_addr, scfg_tlv.value, CFG_ID_PHY_BASE_ADDR_LEN);
    }
#endif

    ret = aal_eth_phy_loopback_get(device_id, phy_addr[ppid], &phy_lpbk);
    if (ret != CA_E_OK) {
        ca_printf("phy loopback get error, ret %d\r\n", ret);
        //return ret;
    }
    ca_printf("get port %d (phy addr 0x%x) loopback tx2rx %d, rx2tx %d\r\n",
              ppid, phy_addr[ppid], phy_lpbk.tx2rx_enable, phy_lpbk.rx2tx_enable);

    if(CA_PORT_IS_GE(port_id)){ /* GE */
        memset(&ge_cfg, 0, sizeof(aal_ni_eth_if_config_t));
        ret = aal_ni_eth_if_get(device_id, ppid, &ge_cfg);
        if (ret != CA_E_OK) {
            ca_printf("ni %d mac loopback get error, ret %d\r\n", ppid, ret);
            return ret;
        }
        mac_lpbk = ge_cfg.lpbk_mode;
    }
    else if(CA_PORT_IS_XGE(port_id)){ /* XGE */

        memset(&xge_cfg, 0, sizeof(aal_ni_xge_cmn_cfg_t));
        xge_idx = ppid - AAL_NI_XE_START;
        ret = aal_ni_xge_cmn_config_get(device_id, xge_idx, &xge_cfg);
        if (ret != CA_E_OK) {
            ca_printf("ni %d mac loopback get error, ret %d\r\n", ppid, ret);
            return ret;
        }
        if(xge_cfg.core_tx2rx_lb == 1){
            mac_lpbk = AAL_NI_MAC_LPBK_MODE_TX2RX;

        }else if(xge_cfg.core_rx2tx_lb == 1){
            mac_lpbk = AAL_NI_MAC_LPBK_MODE_RX2TX;

        }else{
            mac_lpbk = AAL_NI_MAC_LPBK_MODE_NONE;
        }
    }


    if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_NONE && !phy_lpbk.rx2tx_enable && !phy_lpbk.tx2rx_enable){
        *loopback = CA_PORT_LOOPBACK_NONE;

    }else if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_TX2RX && !phy_lpbk.rx2tx_enable && !phy_lpbk.tx2rx_enable){
        *loopback = CA_PORT_LOOPBACK_MAC_LOCAL;

    }else if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_RX2TX && !phy_lpbk.rx2tx_enable && !phy_lpbk.tx2rx_enable){
        *loopback = CA_PORT_LOOPBACK_MAC_REMOTE;

    }else if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_NONE && !phy_lpbk.rx2tx_enable && phy_lpbk.tx2rx_enable){
        *loopback = CA_PORT_LOOPBACK_PHY_LOCAL;

    }else if(mac_lpbk == AAL_NI_MAC_LPBK_MODE_NONE && phy_lpbk.rx2tx_enable && !phy_lpbk.tx2rx_enable){
        *loopback = CA_PORT_LOOPBACK_PHY_REMOTE;

    }else{
        *loopback = CA_PORT_LOOPBACK_NONE;
    }

    return ret;
}

ca_status_t ca_port_untagged_priority_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t priority)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
 #if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ca_uint32_t pon_lpid = AAL_LPORT_INVALID;
 #endif
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    CA_COS_CHECK(priority);

    lpid = PORT_ID(port_id);
#if 0
    if (__PORT_IS_WAN(lpid))
        lpid = CA_PORT_ID_L3_WAN;
#endif
    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    msk.s.default_cos = 1;
    cfg.default_cos = priority;
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if(lpid == 7){
        for(pon_lpid = 32; pon_lpid < 64; pon_lpid ++){
            ret = aal_port_ilpb_cfg_set(device_id, pon_lpid, msk, &cfg);
            if (ret != CA_E_OK) {
                return ret;
            }
        }
    }
#endif
    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    return ret;
}

ca_status_t ca_port_untagged_priority_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * priority)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    CA_ASSERT_RET(priority, CA_E_PARAM);

    lpid = PORT_ID(port_id);
#if 0
    if (__PORT_IS_WAN(lpid))
        lpid = CA_PORT_ID_L3_WAN;
#endif
    ret = aal_port_ilpb_cfg_get(device_id, lpid, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }
    *priority = cfg.default_cos;

    return ret;
}

ca_status_t ca_port_tpid_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t tpid)
{
    return CA_E_OK;
}

ca_status_t ca_port_tpid_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * tpid)
{
    return CA_E_OK;
}

ca_status_t ca_port_stp_state_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_port_stp_state_t stp_state)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint32_t egr_status = 0;
    aal_stp_state_t igr_state = AAL_STP_STATE_FWD_AND_LEARN;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_elpb_cfg_msk_t egr_msk;
    aal_elpb_cfg_t egr_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    lpid = PORT_ID(port_id);

    __PORT_LOCK(device_id);

    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    switch (stp_state) {
    case CA_PORT_STP_DISABLED:
        igr_state = AAL_STP_STATE_DROP;
        //TODO. special packet set BPDU_PKT=drop
        egr_status = 0;
        break;

    case CA_PORT_STP_BLOCKING:
        igr_state = AAL_STP_STATE_DROP;
        egr_status = 0;
        break;

    case CA_PORT_STP_LEARNING:
        igr_state = AAL_STP_STATE_LEARN_NO_FWD;
        egr_status = 0;
        break;

    case CA_PORT_STP_FORWARDING:
        igr_state = AAL_STP_STATE_FWD_AND_LEARN;
        egr_status = 1;
        break;

    default:
        igr_state = CA_PORT_STP_FORWARDING;
        egr_status = 1;
        break;

    }

    igr_msk.s.stp_mode = 1;
    igr_cfg.stp_mode = igr_state;

    ret = aal_port_ilpb_cfg_set(device_id, lpid, igr_msk, &igr_cfg);
    if (ret != CA_E_OK) {
        goto end;
    }

    egr_msk.s.egr_port_stp_status = 1;
    egr_cfg.egr_port_stp_status = egr_status;

    ret = aal_port_elpb_cfg_set(device_id, lpid, egr_msk, &egr_cfg);
    if (ret != CA_E_OK) {
        goto end;
    }

    if(stp_state == CA_PORT_STP_DISABLED){
        __port_stp_mode[lpid] = FALSE;
    }else{
        __port_stp_mode[lpid] = TRUE;
    }
end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_stp_state_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_port_stp_state_t * stp_state)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint32_t egr_state = 0;
    aal_stp_state_t igr_state = AAL_STP_STATE_FWD_AND_LEARN;
    aal_ilpb_cfg_t igr_cfg;
    aal_elpb_cfg_t egr_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    lpid = PORT_ID(port_id);

    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    igr_cfg.stp_mode = igr_state;

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &igr_cfg);
    if (ret != CA_E_OK) {
        return ret;
    }
    igr_state = igr_cfg.stp_mode;

    ret = aal_port_elpb_cfg_get(device_id, lpid, &egr_cfg);
    if (ret != CA_E_OK) {
        return ret;
    }
    egr_state = egr_cfg.egr_port_stp_status;

    if(igr_state == AAL_STP_STATE_DROP && egr_state == 0){
        if(!__port_stp_mode[lpid]){
            *stp_state = CA_PORT_STP_DISABLED;
        }else{
            *stp_state = CA_PORT_STP_BLOCKING;
        }
    }else if(igr_state == AAL_STP_STATE_LEARN_NO_FWD && egr_state == 0){
        *stp_state = CA_PORT_STP_LEARNING;

    }else if(igr_state == AAL_STP_STATE_FWD_AND_LEARN && egr_state == 1){
        *stp_state = CA_PORT_STP_FORWARDING;

    }else{
        *stp_state = CA_PORT_STP_FORWARDING;

    }
    return ret;

}

ca_status_t ca_port_isolation_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t port_count,
    CA_IN ca_port_id_t * dst_ports)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint64_t isolation_bmp = 0;
    ca_uint64_t mmshp_bmp = -1;
    ca_uint32 i = 0;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    //CA_ASSERT_RET(dst_ports, CA_E_PARAM); /* if port_count = 0, dst_ports can be NULL */

    if (port_count > CA_PORT_MAX_NUM) {
        return CA_E_PARAM;
    }

    for (i = 0; i < port_count; i++) {
        lpid = PORT_ID(dst_ports[i]);
        isolation_bmp |= (ca_uint64) 1 << lpid;
    }

    lpid = PORT_ID(port_id);

    __PORT_LOCK(device_id);

    ca_printf("port isolation set port 0x%x, count %d, bmp 0x%llx\r\n", port_id, port_count, isolation_bmp);
    mmshp_bmp &= ~isolation_bmp;
    ret = aal_port_mmshp_check_set(device_id, lpid, mmshp_bmp);
    if (ret != CA_E_OK) {
        goto end;
    }

end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_isolation_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint8_t * port_count,
    CA_OUT ca_port_id_t * dst_ports)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint64_t mmshp_bmp = 0;
    ca_uint32_t isolation_bmp = -1;
    ca_uint32 count = 0;
    ca_uint32 i = 0;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(port_count, CA_E_PARAM);
    CA_ASSERT_RET(dst_ports, CA_E_PARAM);

    lpid = PORT_ID(port_id);
    ret = aal_port_mmshp_check_get(device_id, lpid, &mmshp_bmp);
    isolation_bmp &= ~mmshp_bmp;

    for (i = 0; i < 64; i++) {
        if (isolation_bmp & (ca_uint64) 1 << i) {
            dst_ports[count] = ca_port_id_get(i);
            count++;
        }
    }
    *port_count = count;
    return CA_E_OK;
}

ca_status_t ca_port_discard_control_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_port_discard_control_t *control)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(control, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    msk.s.drop_untagged_packet = 1;
    msk.s.drop_pritagged_packet = 1;
    msk.s.drop_doubletagged_packet = 1;
    msk.s.drop_singletagged_packet = 1;

    if (control->drop_untag) {
        cfg.drop_untagged_packet = TRUE;
    }

    if (control->drop_priority_tag) {
        cfg.drop_pritagged_packet = TRUE;
    }

    if (control->drop_multiple_tag) {
        cfg.drop_doubletagged_packet = TRUE;
    }

    if (control->drop_single_tag) {    /* drop single tagged packets */
        cfg.drop_singletagged_packet = TRUE;
    }

    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    return ret;

}

ca_status_t ca_port_discard_control_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_port_discard_control_t *control)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(control, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    memset(control, 0, sizeof(ca_port_discard_control_t));
    if (cfg.drop_untagged_packet) {
        control->drop_untag = TRUE;
    }
    if(cfg.drop_pritagged_packet){
        control->drop_priority_tag = TRUE;
    }

    if (cfg.drop_doubletagged_packet) {
        control->drop_multiple_tag = TRUE;
    }

    if (cfg.drop_singletagged_packet) {
        control->drop_single_tag = TRUE;
    }
    return ret;
}

ca_status_t ca_port_station_move_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    CA_BOOL_VAL_CHECK(enable);

    lpid = PORT_ID(port_id);

    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    msk.s.station_move_en = 1;
    cfg.station_move_en = enable;

    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    return ret;

}

ca_status_t ca_port_station_move_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t * enable)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(enable, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    *enable = cfg.station_move_en;
    return ret;
}

ca_status_t ca_port_pvid_vlan_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t vid)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    CA_VLAN_ID_CHECK(vid);

    lpid = PORT_ID(port_id);

    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    msk.s.default_vlanid = 1;
    msk.s.default_untag_cmd = 1;
    cfg.default_vlanid = vid;

    if(vid != 0){
        cfg.default_untag_cmd = AAL_VLAN_CMD_PUSH;
    }else{
        cfg.default_untag_cmd = AAL_VLAN_CMD_NOP;
    }

    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7) == port_id ){
		/* PON port 7 will map to Lsport 0x20~0x3f in EPON SFU */
		for(lpid = CA_PORT_EPON_LLID_LPID_START; lpid < CA_PORT_EPON_LLID_LPID_END+1; lpid++){
    		ret |= aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
        }
	}
#endif
    return ret;

}

ca_status_t ca_port_pvid_vlan_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * vid)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(vid, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    *vid = cfg.default_vlanid;
    return ret;
}

ca_boolean_t __port_is_svlan_tpid(ca_port_id_t port_id, ca_uint32_t tpid_index)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    ca_uint32_t num = 0;
    ca_uint32_t tpid[4];

    memset(&tpid[0], 0, sizeof(tpid));
#ifdef RTK_MEM_CORRUPTTION_FIX
    ret = ca_l2_vlan_outer_tpid_list(0, port_id, tpid, &num);
#else
    ret = ca_l2_vlan_outer_tpid_list(0, port_id, &num, tpid);
#endif

    if((ret != CA_E_OK) || (num > 4)){
        return CA_FALSE;
    }
    for(i = 0; i < num; i++){
        if(tpid[i] == tpid_index){
            return CA_TRUE;
        }
    }
    return CA_FALSE;
}

/* index 0: 0x8100; 1: 0x9100; 2: 0x88a8; 3: user defined*/
ca_status_t ca_port_pvid_tpid_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t tpid_index)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    if (tpid_index > 3) {    /* support index 0, 1, 2, 3 */
        return CA_E_PARAM;
    }

    __PORT_LOCK(device_id);
    lpid = PORT_ID(port_id);

    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));
    if(__port_is_svlan_tpid(port_id, tpid_index)){
        msk.s.sc_ind= 1;
        cfg.sc_ind = 1;
    }else{
        msk.s.sc_ind= 1;
        cfg.sc_ind = 0;
    }

    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        goto end;
        return ret;
    }

    __port_tpid_index[lpid] = tpid_index;
end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_pvid_tpid_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * tpid_index)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(tpid_index, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    *tpid_index = __port_tpid_index[lpid];

    return ret;
}

ca_status_t ca_port_pvid_dot1p_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t priority)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_msk_t msk;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    CA_DOT_1P_CHECK(priority);

    lpid = PORT_ID(port_id);

    memset(&msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    msk.s.default_802_1p = 1;
    cfg.default_802_1p = priority;

    ret = aal_port_ilpb_cfg_set(device_id, lpid, msk, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    return ret;

}

ca_status_t ca_port_pvid_dot1p_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * priority)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    aal_ilpb_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);

    CA_ASSERT_RET(priority, CA_E_PARAM);

    lpid = PORT_ID(port_id);

    memset(&cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &cfg);
    if (ret != CA_E_OK) {
        return ret;
    }

    *priority = cfg.default_802_1p;
    return ret;
}

static ca_status_t __port_lag_hash_set(ca_device_id_t device_id, ca_uint8_t lag_group_id, ca_classifier_key_mask_t lag_hash_mask)
{
    aal_trunk_hash_key_t cfg;

    memset((void *)&cfg, 0, sizeof(aal_trunk_hash_key_t));
    //if(lag_hash_keys.l2 == 1 && lag_hash_keys.l2_mask. == 1){
    //cfg.field_sel_cos = 1;
    //}

    if (lag_hash_mask.l4 == 1 && lag_hash_mask.l4_mask.dst_port == 1) {
        cfg.field_sel_l4_dp = 1;
    }

    if (lag_hash_mask.l4 == 1 && lag_hash_mask.l4_mask.src_port == 1) {
        cfg.field_sel_l4_sp = 1;
    }

    if (lag_hash_mask.ip == 1 && lag_hash_mask.ip_mask.ip_da == 1) {
        cfg.field_sel_ip_da = 1;
    }

    if (lag_hash_mask.ip == 1 && lag_hash_mask.ip_mask.ip_sa == 1) {
        cfg.field_sel_ip_sa = 1;
    }

    if (lag_hash_mask.l2 == 1 && lag_hash_mask.l2_mask.vlan_otag == 1
            && lag_hash_mask.l2_mask.vlan_otag_mask.vid == 1) {
        cfg.field_sel_tx_top_vid = 1;
    }

    if (lag_hash_mask.l2 == 1 && lag_hash_mask.l2_mask.mac_da == 1) {
        cfg.field_sel_mac_da = 1;
    }

    if (lag_hash_mask.l2 == 1 && lag_hash_mask.l2_mask.mac_sa == 1) {
        cfg.field_sel_mac_sa = 1;
    }
    return aal_port_trunk_group_hash_set(device_id, lag_group_id, cfg);

}

static ca_status_t __port_lag_hash_get(ca_device_id_t device_id, ca_uint8_t lag_group_id, ca_classifier_key_mask_t *lag_hash_keys)
{
    ca_status_t ret = CA_E_OK;
    aal_trunk_hash_key_t cfg;

    ret = aal_port_trunk_group_hash_get(device_id, lag_group_id, &cfg);
    if(ret != CA_E_OK){
        return ret;
    }

    memset(lag_hash_keys, 0, sizeof(ca_classifier_key_mask_t));

    //if(cfg.bf.trunk_hash_field_sel_cos == 1){
    //lag_hash_keys = 1;
    //}

    if (cfg.field_sel_l4_dp == 1) {
        lag_hash_keys->l4 = 1;
        lag_hash_keys->l4_mask.dst_port = 1;
    }

    if (cfg.field_sel_l4_sp == 1) {
        lag_hash_keys->l4 = 1;
        lag_hash_keys->l4_mask.src_port = 1;
    }

    if (cfg.field_sel_ip_da == 1) {
        lag_hash_keys->ip = 1;
        lag_hash_keys->ip_mask.ip_da = 1;
    }

    if (cfg.field_sel_ip_sa == 1) {
        lag_hash_keys->ip = 1;
        lag_hash_keys->ip_mask.ip_sa = 1;
    }

    if (cfg.field_sel_tx_top_vid == 1) {
        lag_hash_keys->l2 = 1;
        lag_hash_keys->l2_mask.vlan_otag = 1;
        lag_hash_keys->l2_mask.vlan_otag_mask.vid = 1;
    }

    if (cfg.field_sel_mac_da == 1) {
        lag_hash_keys->l2 = 1;
        lag_hash_keys->l2_mask.mac_da = 1;
    }

    if (cfg.field_sel_mac_sa == 1) {
        lag_hash_keys->l2 = 1;
        lag_hash_keys->l2_mask.mac_sa = 1;
    }

    return CA_E_OK;
}

static ca_status_t __port_lag_dest_map_update(CA_IN ca_device_id_t dev,
        CA_IN ca_uint32_t group_id,
        CA_IN ca_uint32_t port_num,
        CA_IN ca_uint32_t active_ports[])
{
    ca_uint32_t i = 0;
    ca_uint32_t ldpid = 0;

    for (i = 0; i < 32; i++) {
        if (port_num == 0) {
            ldpid = 0;
        } else {
            ldpid = active_ports[i % port_num];
        }
        aal_port_trunk_dest_map_set(dev, 32 * group_id + i, ldpid);
    }

    return CA_E_OK;
}

/* return 0xff means not found */
ca_uint8_t __port_lag_member_of_group(ca_uint32_t lpid)
{
    ca_uint32_t group_id = 0;
    for(group_id = 0; group_id < CA_PORT_MAX_NUM_TRUNK; group_id++){
        if(__port_lag_attr[group_id].valid){
            if(__port_lag_attr[group_id].members_bmp & ((ca_uint64)1<<lpid)){
                return group_id;
            }
        }
    }
    return 0xff;
}

ca_status_t ca_port_link_aggregation_group_create(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t lag_group_id,
    CA_IN ca_uint8_t member_port_count,
    CA_IN ca_port_id_t *member_ports,
    CA_IN ca_classifier_key_mask_t lag_hash_mask)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint8_t group_id = 0;
    ca_uint64_t members_bmp = 0;
    ca_eth_port_link_status_t status = CA_PORT_LINK_UP;
    ca_uint32_t active_ldpid_num = 0;
    ca_uint32_t active_ldpid[CA_PORT_MAX_NUM_TRUNK_MEMBER];

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_LAG_ID_CHECK(lag_group_id);

    if (member_port_count > CA_PORT_MAX_NUM_TRUNK_MEMBER)
        return CA_E_PARAM;

    if (__port_lag_attr[lag_group_id].valid) {
        return CA_E_EXISTS;
    }

    __PORT_LOCK(device_id);

    /* note: set hash key and dest maping table before the group member set */
    ret = __port_lag_hash_set(device_id, lag_group_id, lag_hash_mask);
    if (ret != CA_E_OK) {
        goto end;
    }

    /* get the active ports */
    memset(active_ldpid, 0, sizeof(active_ldpid));
    for (i = 0; i < member_port_count; i++) {
        lpid = PORT_ID(member_ports[i]);
        group_id = __port_lag_member_of_group(lpid);
        if(group_id != lag_group_id && group_id != 0xff){
            ca_printf("port 0x%x is a member of group %d\r\n", member_ports[i], group_id);
            ret = CA_E_CONFLICT;
            goto end;
        }

        if (CA_PORT_IS_ETHERNET(member_ports[i])) {    /* ethernet port need to check link status */
            ret = ca_eth_port_link_status_get(device_id, member_ports[i], &status);
            if (ret != CA_E_OK || status != CA_PORT_LINK_UP) {
                continue;
            }
        }
        active_ldpid[active_ldpid_num] = lpid;
        active_ldpid_num++;
    }

    /* only add the linkup ports to the dest map */
    ret = __port_lag_dest_map_update(device_id, lag_group_id, active_ldpid_num, active_ldpid);
    if (ret != CA_E_OK) {
        goto end;
    }
    memset(__port_lag_attr[lag_group_id].active_members, 0,
           CA_PORT_MAX_NUM_TRUNK_MEMBER * sizeof(ca_uint32_t));
    memcpy(__port_lag_attr[lag_group_id].active_members, active_ldpid,
           active_ldpid_num * sizeof(ca_uint32_t));
    __port_lag_attr[lag_group_id].active_members_num = active_ldpid_num;

    /* set the members  for group */
    for (i = 0; i < member_port_count; i++) {
        lpid = PORT_ID(member_ports[i]);
        members_bmp |= (ca_uint64) 1 << lpid;
    }

    ret = aal_port_trunk_group_member_set(device_id, lag_group_id, members_bmp);
    if (ret != CA_E_OK) {
        goto end;
    }
    __port_lag_attr[lag_group_id].members_bmp = members_bmp;
    __port_lag_attr[lag_group_id].valid = 1;

end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_link_aggregation_group_update(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t lag_group_id,
    CA_IN ca_uint8_t member_port_count,
    CA_IN ca_port_id_t *member_ports,
    CA_IN ca_classifier_key_mask_t lag_hash_mask)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    ca_uint32_t lpid = AAL_LPORT_INVALID;
    ca_uint8_t group_id = 0;
    ca_uint64_t members_bmp = 0;
    ca_eth_port_link_status_t status = CA_PORT_LINK_UP;
    ca_uint32_t active_ldpid_num = 0;
    ca_uint32_t active_ldpid[CA_PORT_MAX_NUM_TRUNK_MEMBER];

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_LAG_ID_CHECK(lag_group_id);

    if (member_port_count > CA_PORT_MAX_NUM_TRUNK_MEMBER)
        return CA_E_PARAM;

    if (!__port_lag_attr[lag_group_id].valid) {
        return CA_E_NOT_FOUND;
    }

    __PORT_LOCK(device_id);
    ret = __port_lag_hash_set(device_id, lag_group_id, lag_hash_mask);
    if (ret != CA_E_OK) {
        goto end;
    }
    /* get the active ports */
    memset(active_ldpid, 0, sizeof(active_ldpid));
    for (i = 0; i < member_port_count; i++) {

        lpid = PORT_ID(member_ports[i]);
        group_id = __port_lag_member_of_group(lpid);
        if(group_id != lag_group_id && group_id != 0xff){
            ca_printf("port 0x%x is a member of group %d\r\n", member_ports[i], group_id);
            ret = CA_E_CONFLICT;
            goto end;
        }

        if (CA_PORT_IS_ETHERNET(member_ports[i])) {    /* only suuport ethernet port aggregation */
            ret = ca_eth_port_link_status_get(device_id, member_ports[i], &status);
            if (ret != CA_E_OK || status != CA_PORT_LINK_UP) {
                continue;
            }

            active_ldpid[active_ldpid_num] = PORT_ID(member_ports[i]);
            active_ldpid_num++;
        }

    }

    if (active_ldpid_num != __port_lag_attr[lag_group_id].active_members_num
            || memcmp(__port_lag_attr[lag_group_id].active_members,
                      active_ldpid, active_ldpid_num * sizeof(ca_port_id_t))) {
        /* only add the linkup ports to the dest map */
        ret = __port_lag_dest_map_update(device_id, lag_group_id, active_ldpid_num, active_ldpid);
        if (ret != CA_E_OK) {
            goto end;
        }
        memset(__port_lag_attr[lag_group_id].active_members, 0,
               CA_PORT_MAX_NUM_TRUNK_MEMBER * sizeof(ca_uint32_t));
        memcpy(__port_lag_attr[lag_group_id].active_members,
               active_ldpid, active_ldpid_num * sizeof(ca_uint32_t));
        __port_lag_attr[lag_group_id].active_members_num = active_ldpid_num;
    }

    /* set the members for group */
    for (i = 0; i < member_port_count; i++) {
        lpid = PORT_ID(member_ports[i]);
        members_bmp |= (ca_uint64) 1 << lpid;
    }
    ret = aal_port_trunk_group_member_set(device_id, lag_group_id, members_bmp);
    if (ret != CA_E_OK) {
        goto end;
    }
    __port_lag_attr[lag_group_id].members_bmp = members_bmp;
end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_link_aggregation_group_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t lag_group_id,
    CA_OUT ca_uint8_t *member_port_count,
    CA_OUT ca_port_id_t *member_ports,
    CA_OUT ca_classifier_key_mask_t *lag_hash_mask)
{
    ca_uint32_t i = 0;
    ca_uint32_t count = 0;
    ca_port_id_t port_id = 0;
    ca_uint64_t members_bmp = 0;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_LAG_ID_CHECK(lag_group_id);

    CA_ASSERT_RET(member_port_count, CA_E_PARAM);
    CA_ASSERT_RET(member_ports, CA_E_PARAM);
    CA_ASSERT_RET(lag_hash_mask, CA_E_PARAM);

    if (!__port_lag_attr[lag_group_id].valid) {
        return CA_E_NOT_FOUND;
    }

    __port_lag_hash_get(device_id, lag_group_id, lag_hash_mask);

    aal_port_trunk_group_member_get(device_id, lag_group_id, &members_bmp);
    for (i = 0; i < AAL_PORT_NUM_LOGICAL; i++) {
        if ((members_bmp & (ca_uint64) 1 << i) != 0) {
            port_id = ca_port_id_get(i);
            member_ports[count] = port_id;
            count++;
        }
    }
    *member_port_count = count;
    return CA_E_OK;
}

ca_status_t ca_port_link_aggregation_group_delete(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t lag_group_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t active_ports_num = 0;
    ca_classifier_key_mask_t lag_hash_mask;
    ca_uint32_t active_ldpid[CA_PORT_MAX_NUM_TRUNK_MEMBER];

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_LAG_ID_CHECK(lag_group_id);

    if (!__port_lag_attr[lag_group_id].valid) {
        return CA_E_NOT_FOUND;
    }

    __PORT_LOCK(device_id);

    /* note: clear the group member before hash key and dest maping table clear */
    ret = aal_port_trunk_group_member_set(device_id, lag_group_id, 0);
    if (ret != CA_E_OK) {
        ca_printf("aal_port_trunk_group_member_set fail, ret %d \r\n", ret);
        goto end;
    }
    memset(&lag_hash_mask, 0, sizeof(ca_classifier_key_mask_t));
    ret = __port_lag_hash_set(device_id, lag_group_id, lag_hash_mask);
    if (ret != CA_E_OK) {
        ca_printf("__port_lag_hash_set fail, ret %d \r\n", ret);
        goto end;
    }
    memset(active_ldpid, 0, sizeof(active_ldpid));
    ret = __port_lag_dest_map_update(device_id, lag_group_id, active_ports_num, active_ldpid);
    if (ret != CA_E_OK) {
        ca_printf("__port_lag_dest_map_update fail, ret %d \r\n", ret);
        goto end;
    }
    memset(&__port_lag_attr[lag_group_id], 0, sizeof(__port_lag_attr_t));

end:
    __PORT_UNLOCK(device_id);
    return ret;

}

ca_status_t ca_port_encryption_mode_set
(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_port_id_t                   port_id,
    CA_IN ca_port_direction_encryption_t direction,
    CA_IN ca_port_encryption_mode_t      mode
)
{
    ca_status_t ret = CA_E_NOT_SUPPORT;
#if 0  //obsoleted for Saturn series
    ca_uint32_t port     ;
    port = PORT_ID(port_id); 
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    aal_pon_crypto_mode_t tx_mode;
    aal_pon_crypto_mode_t rx_mode;

    aal_uni_macsec_glb_staic_cfg_msk_t xge_macsec_glb_cfg_msk;
    aal_uni_macsec_glb_staic_cfg_t     xge_macsec_glb_cfg;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_PON_CHECK(port_id);
    xge_macsec_glb_cfg_msk.u32 = 0;
    memset(&xge_macsec_glb_cfg, 0, sizeof(aal_uni_macsec_glb_staic_cfg_t));
    xge_macsec_glb_cfg_msk.s.macsec_dec_in_sel = 1;
    xge_macsec_glb_cfg_msk.s.macsec_enc_in_sel = 1;

    if (direction == CA_DIRECTION_ENC_TX)
    {
        tx_mode = mode;
        rx_mode = AAL_PON_CRYPTO_MODE_DISABLE;
    }
    else if (direction == CA_DIRECTION_ENC_RX)
    {
        tx_mode = AAL_PON_CRYPTO_MODE_DISABLE;
        rx_mode = mode;
    }
    else if (direction == CA_DIRECTION_ENC_BI)
    {
        tx_mode = mode;
        rx_mode = mode;
    }
    else
    {
        ca_printf("ca_port_encryption_mode_set(): invalid direction(%d) setting\n", direction);
        return CA_E_PARAM;
    }

    /* check if it is PON port */
    if(port == CA_PORT_ID_NI7){
    __PON_LOCK();
        ret = aal_pon_crypto_mode_set(device_id, port, tx_mode, rx_mode);
    __PON_UNLOCK();
    }else if(port == 5){
        
        xge_macsec_glb_cfg_msk.s.p5_macsec_dec_out_sel = 1;
        xge_macsec_glb_cfg_msk.s.p5_txmac_in_sel  = 1;
        
        xge_macsec_glb_cfg.macsec_dec_in_sel = 0;
        xge_macsec_glb_cfg.macsec_enc_in_sel = 0;
        if (direction == CA_DIRECTION_ENC_TX){
            xge_macsec_glb_cfg.p5_macsec_dec_out_sel = AAL_PON_CRYPTO_MODE_DISABLE;
            xge_macsec_glb_cfg.p5_txmac_in_sel       = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);

        }else if (direction == CA_DIRECTION_ENC_RX){
            xge_macsec_glb_cfg.p5_macsec_dec_out_sel = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            xge_macsec_glb_cfg.p5_txmac_in_sel       = AAL_PON_CRYPTO_MODE_DISABLE;
            
        }else if (direction == CA_DIRECTION_ENC_BI){
            xge_macsec_glb_cfg.p5_macsec_dec_out_sel = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            xge_macsec_glb_cfg.p5_txmac_in_sel       = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            
        }else
        {
            ca_printf("ca_port_encryption_mode_set(): invalid direction(%d) setting\n", direction);
            return CA_E_PARAM;
        }
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        ret = aal_uni_macsec_glb_cfg_set(device_id, xge_macsec_glb_cfg_msk, &xge_macsec_glb_cfg);
#endif
    }else if(port == 6){
        xge_macsec_glb_cfg_msk.s.p6_macsec_dec_out_sel = 1;
        xge_macsec_glb_cfg_msk.s.p6_txmac_in_sel  = 1;
        
        xge_macsec_glb_cfg.macsec_dec_in_sel = 1;
        xge_macsec_glb_cfg.macsec_enc_in_sel = 1;
        if (direction == CA_DIRECTION_ENC_TX){
            xge_macsec_glb_cfg.p6_macsec_dec_out_sel = AAL_PON_CRYPTO_MODE_DISABLE;
            xge_macsec_glb_cfg.p6_txmac_in_sel       = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);

        }else if (direction == CA_DIRECTION_ENC_RX){
            xge_macsec_glb_cfg.p6_macsec_dec_out_sel = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            xge_macsec_glb_cfg.p6_txmac_in_sel       = AAL_PON_CRYPTO_MODE_DISABLE;
            
        }else if (direction == CA_DIRECTION_ENC_BI){
            xge_macsec_glb_cfg.p6_macsec_dec_out_sel = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            xge_macsec_glb_cfg.p6_txmac_in_sel       = (mode == CA_PORT_ENCRYPTION_MODE_MACSEC ? 1:0);
            
        }else
        {
            ca_printf("ca_port_encryption_mode_set(): invalid direction(%d) setting\n", direction);
            return CA_E_PARAM;
        } 
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        ret = aal_uni_macsec_glb_cfg_set(device_id, xge_macsec_glb_cfg_msk, &xge_macsec_glb_cfg);
#endif
    }
    
#endif
#endif
    return ret;
}

ca_status_t ca_port_encryption_mode_get
(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_OUT ca_port_direction_encryption_t *direction,
    CA_OUT ca_port_encryption_mode_t      *mode
)
{
    ca_status_t           ret = CA_E_NOT_SUPPORT;
#if 0  //obsoleted for Saturn series
    ca_uint32_t port     ;
    port = PORT_ID(port_id); 
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    aal_pon_crypto_mode_t tx_mode;
    aal_pon_crypto_mode_t rx_mode;

    aal_uni_macsec_glb_staic_cfg_t  xge_macsec_glb_cfg;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_PON_CHECK(port_id);
    memset(&xge_macsec_glb_cfg, 0, sizeof(aal_uni_macsec_glb_staic_cfg_t));

    if(port == CA_PORT_ID_NI7){
    __PON_LOCK();
        ret = aal_pon_crypto_mode_get(device_id, port, &tx_mode, &rx_mode);
    __PON_UNLOCK();

    if (tx_mode != AAL_PON_CRYPTO_MODE_DISABLE && rx_mode == AAL_PON_CRYPTO_MODE_DISABLE)
    {
        *direction = CA_DIRECTION_ENC_TX;
        *mode = (ca_port_encryption_mode_t)tx_mode;
    }
    else if (tx_mode == AAL_PON_CRYPTO_MODE_DISABLE && rx_mode != AAL_PON_CRYPTO_MODE_DISABLE)
    {
        *direction = CA_DIRECTION_ENC_RX;
        *mode = (ca_port_encryption_mode_t)rx_mode;
    }
    else if (tx_mode == rx_mode)
    {
        *direction = CA_DIRECTION_ENC_BI;
        *mode = (ca_port_encryption_mode_t)tx_mode;
    }
        else{
            return CA_E_ERROR;
        }
    }else if(port == 5){
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        ret = aal_uni_macsec_glb_cfg_get(device_id, &xge_macsec_glb_cfg);
#endif
        if (xge_macsec_glb_cfg.p5_txmac_in_sel != AAL_PON_CRYPTO_MODE_DISABLE && xge_macsec_glb_cfg.p5_macsec_dec_out_sel == AAL_PON_CRYPTO_MODE_DISABLE)
        {
            *direction = CA_DIRECTION_ENC_TX;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p5_txmac_in_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE);
        } 
        else if (xge_macsec_glb_cfg.p5_txmac_in_sel == AAL_PON_CRYPTO_MODE_DISABLE && xge_macsec_glb_cfg.p5_macsec_dec_out_sel != AAL_PON_CRYPTO_MODE_DISABLE)
        {
            *direction = CA_DIRECTION_ENC_RX;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p5_macsec_dec_out_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE);
        }
        else if (xge_macsec_glb_cfg.p5_txmac_in_sel == xge_macsec_glb_cfg.p5_macsec_dec_out_sel)
        {
            *direction = CA_DIRECTION_ENC_BI;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p5_txmac_in_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE) ;
        }
        else{
            return CA_E_ERROR;
        }
    }else if(port == 6){
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
        ret = aal_uni_macsec_glb_cfg_get(device_id, &xge_macsec_glb_cfg);
#endif
        if (xge_macsec_glb_cfg.p6_txmac_in_sel != AAL_PON_CRYPTO_MODE_DISABLE && xge_macsec_glb_cfg.p6_macsec_dec_out_sel == AAL_PON_CRYPTO_MODE_DISABLE)
        {
            *direction = CA_DIRECTION_ENC_TX;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p6_txmac_in_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE);
        } 
        else if (xge_macsec_glb_cfg.p6_txmac_in_sel == AAL_PON_CRYPTO_MODE_DISABLE && xge_macsec_glb_cfg.p6_macsec_dec_out_sel != AAL_PON_CRYPTO_MODE_DISABLE)
        {
            *direction = CA_DIRECTION_ENC_RX;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p6_macsec_dec_out_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE);
        }
        else if (xge_macsec_glb_cfg.p6_txmac_in_sel == xge_macsec_glb_cfg.p6_macsec_dec_out_sel)
        {
            *direction = CA_DIRECTION_ENC_BI;
            *mode = (ca_port_encryption_mode_t)(xge_macsec_glb_cfg.p6_txmac_in_sel == 1 ? CA_PORT_ENCRYPTION_MODE_MACSEC : AAL_PON_CRYPTO_MODE_DISABLE) ;
        }
        else{
        return CA_E_ERROR;
        }
    }
#endif
#endif
    return ret;
}

// TODO: need to add 1G encryption
ca_status_t ca_port_encryption_enable_set (
    CA_IN       ca_device_id_t                          device_id,
    CA_IN       ca_port_id_t                            port_id,
    CA_IN       ca_port_direction_encryption_t          direction,
    CA_IN       ca_boolean_t                            enable)
{
    ca_status_t ret = CA_E_OK;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    ca_uint32_t port_type = PORT_TYPE(port_id);
    ca_uint32_t port = PORT_ID(port_id);
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(device_id);
    
    if((direction == CA_DIRECTION_ENC_RX)
        ||(direction == CA_DIRECTION_ENC_TX)){
        if(!enable){
            return CA_E_PARAM;
        }
    }
    
    if(port_type == CA_PORT_TYPE_EPON){
        /* for 1G EPON, only surport RX decryption */
        if(pon_mode == ONU_PON_MAC_EPON_1G){
            if(direction == CA_DIRECTION_ENC_RX){
                
                ret = aal_epon_crypto_1g_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
                CA_RET_VALUE_CHECK(ret);
            
            }else if(direction == CA_DIRECTION_ENC_TX){
                return CA_E_NOT_SUPPORT;
                
            }else if(direction == CA_DIRECTION_ENC_BI){
                if(enable != 0){
                    return CA_E_NOT_SUPPORT;
                }
                ret = aal_epon_crypto_1g_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
            }else {
                return CA_E_PARAM;
            }
        }
        else if(pon_mode == ONU_PON_MAC_EPON_BI10G){
            if(direction == CA_DIRECTION_ENC_RX){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, FALSE);
                CA_RET_VALUE_CHECK(ret);
            }else if(direction == CA_DIRECTION_ENC_TX){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, FALSE);
                CA_RET_VALUE_CHECK(ret);
            }else if(direction == CA_DIRECTION_ENC_BI){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);
                CA_RET_VALUE_CHECK(ret);
            }else {
                return CA_E_PARAM;
            }
        }else if(pon_mode == ONU_PON_MAC_EPON_D10G){
#if defined(CONFIG_LUNA_G3_SERIES)
            if(direction == CA_DIRECTION_ENC_RX){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, FALSE);
                CA_RET_VALUE_CHECK(ret);
            }else if(direction == CA_DIRECTION_ENC_TX){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, FALSE);
                CA_RET_VALUE_CHECK(ret);
            }else if(direction == CA_DIRECTION_ENC_BI){
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);
                CA_RET_VALUE_CHECK(ret);
            }else {
                return CA_E_PARAM;
            }
#endif
        }
    }else if(port_type == CA_PORT_TYPE_ETHERNET){
        if(port == 5){
            
        }else if(port == 6){
            
        }
    }
#endif
    return ret;
}

ca_status_t ca_port_encryption_enable_get (
    CA_IN       ca_device_id_t                          device_id,
    CA_IN       ca_port_id_t                            port_id,
    CA_OUT      ca_port_direction_encryption_t          *direction,
    CA_OUT      ca_boolean_t                            *enable)
{
    ca_status_t ret = CA_E_OK;   
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint32_t port_type = PORT_TYPE(port_id);
    ca_uint32_t port = PORT_ID(port_id);
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(device_id);
    ca_boolean_t rx_ena = 0;
    ca_boolean_t tx_ena = 0;

    if(port_type == CA_PORT_TYPE_EPON){
        if(pon_mode == ONU_PON_MAC_EPON_1G){
            ret = aal_epon_crypto_1g_enable_get(0, 0, AAL_EPON_CRYPTO_DIRECTION_RX, &rx_ena);
            ret = aal_epon_crypto_1g_enable_get(0, 0, AAL_EPON_CRYPTO_DIRECTION_TX, &tx_ena);
        }
        else if(pon_mode == ONU_PON_MAC_EPON_BI10G){

            ret = aal_epon_crypto_xg_enable_get(0, 0, AAL_EPON_CRYPTO_DIRECTION_RX, &rx_ena);
            ret = aal_epon_crypto_xg_enable_get(0, 0, AAL_EPON_CRYPTO_DIRECTION_TX, &tx_ena);

        }else if(pon_mode == ONU_PON_MAC_EPON_D10G){

        }
        if(rx_ena != 0 && tx_ena == 0){
            *direction = CA_DIRECTION_ENC_RX;
            *enable = rx_ena;
        }else if(tx_ena != 0 && tx_ena == 0){
            *direction = CA_DIRECTION_ENC_TX;
            *enable = tx_ena;
        }if(rx_ena == tx_ena){
            *direction = CA_DIRECTION_ENC_BI;
            *enable = rx_ena;
        }
    }else if(port_type == CA_PORT_TYPE_ETHERNET){
        if(port == 5){
            
        }else if(port == 6){
            
        }
    }
#endif
    return ret;
}

static ca_status_t __port_spcl_pkt_init(ca_device_id_t device_id)
{
    ca_status_t        ret = CA_E_OK;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t     igr_cfg;
    ca_uint32_t        lport = 0;


    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    igr_msk.s.spcl_pkt_idx = 1;

    for (lport = 0; lport < AAL_PORT_NUM_LOGICAL; ++lport) {

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (__PORT_IS_WAN(lport) || (lport >= AAL_LPORT_LLID_GEM_INDEX_0 && lport <= AAL_LPORT_LLID_GEM_INDEX_31))
#else
        if (__PORT_IS_WAN(lport))
#endif
            igr_cfg.spcl_pkt_idx =  AAL_PORT_SPB_IDX_WAN;
        else if (__PORT_IS_LAN(lport))
            igr_cfg.spcl_pkt_idx =  AAL_PORT_SPB_IDX_LAN;
        else
            igr_cfg.spcl_pkt_idx =  AAL_PORT_SPB_IDX_RSVD;

        ret = aal_port_ilpb_cfg_set(device_id, lport, igr_msk, &igr_cfg);
        if (CA_E_OK != ret) {
            ca_printf("aal_port_ilpb_cfg_set port %u fail, return %#x.\n", lport, ret);
            break;
        }
    }

    return ret;

}

ca_status_t ca_port_tx_traffic_block_set(CA_IN ca_device_id_t device_id,
                                                 CA_IN ca_port_id_t port_id,
                                                 CA_IN ca_boolean_t  enable
                                         )
{
    ca_uint32_t logic_port_id = 0;
    ca_uint32_t new_phy_port_id = 0;
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_ID_CHECK(port_id);

    logic_port_id = PORT_ID(port_id);
    if(enable){
        new_phy_port_id = AAL_PPORT_BLACKHOLE;
    }else{
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if(CA_PORT_IS_PON(port_id)){
            new_phy_port_id = AAL_PPORT_NI7;
        }
        else
#endif
        if(logic_port_id <= AAL_LPORT_ETH_NI7 ){
            new_phy_port_id = logic_port_id;
        }else if(logic_port_id <= AAL_LPORT_9QUEUE_NI7){
            new_phy_port_id = AAL_PPORT_OAM;
        }else if(logic_port_id <= AAL_LPORT_CPU_7){
            new_phy_port_id = AAL_PPORT_CPU;
        }else if(logic_port_id == AAL_LPORT_L3_WAN){
            new_phy_port_id = AAL_PPORT_L3_WAN;
        }else if(logic_port_id == AAL_LPORT_L3_LAN){
            new_phy_port_id = AAL_PPORT_L3_LAN;
        }else if(logic_port_id == AAL_LPORT_MC){
            new_phy_port_id = AAL_PPORT_MC;
        }else if(logic_port_id == AAL_LPORT_OAM){
            new_phy_port_id = AAL_PPORT_OAM;
        }
    }

#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if(CA_PORT_IS_PON(port_id)){
        for(logic_port_id = AAL_LPORT_CPU_MQ_0; logic_port_id <= AAL_LPORT_CPU_MQ_31;logic_port_id ++){
            ret = aal_port_arb_ldpid_pdpid_map_set(device_id,0,0,logic_port_id,new_phy_port_id);
            if(ret != CA_E_OK){
                goto end;
            }
        }
        ret = aal_port_arb_ldpid_pdpid_map_set(device_id,0,0,AAL_LPORT_ETH_NI7,AAL_PPORT_BLACKHOLE);
        if(ret != CA_E_OK){
            goto end;
        }
    }else
#endif
    {
        ret = aal_port_arb_ldpid_pdpid_map_set(device_id,0,0,logic_port_id,new_phy_port_id);
   }
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
end:
#endif
    return ret;

}


ca_status_t ca_port_tx_traffic_block_get(CA_IN ca_device_id_t device_id,
                                         CA_IN ca_port_id_t port_id,
                                         CA_OUT ca_boolean_t  *enable
                                         )
{
    ca_uint32_t  logic_port_id = 0;
    ca_uint32_t  phy_port_id = 0;
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
    CA_PORT_ID_CHECK(port_id);
    if(enable == NULL){
        return CA_E_PARAM;
    }

    logic_port_id = PORT_ID(port_id);
    ret = aal_port_arb_ldpid_pdpid_map_get(device_id,0,0,logic_port_id,&phy_port_id);
    if(CA_E_OK != ret){
        return ret;
    }
    if(phy_port_id == AAL_PPORT_BLACKHOLE){
        *enable = TRUE;
    }else{
        *enable = FALSE;
    }
    return CA_E_OK;
}

ca_status_t ca_port_egr_qm_block_set(CA_IN ca_device_id_t device_id,
                                                 CA_IN ca_port_id_t port_id,
                                                 CA_IN ca_boolean_t  enable
                                            )
{
    ca_uint32_t logic_port_id = 0;
    ca_uint32_t phy_port_id = 0;
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
   // CA_PORT_ID_CHECK(port_id);

    logic_port_id = PORT_ID(port_id);
    if(logic_port_id <= AAL_LPORT_ETH_NI7){
        phy_port_id = logic_port_id;
    }else if(logic_port_id <= AAL_LPORT_9QUEUE_NI7){
        phy_port_id = AAL_PPORT_OAM;
    }else if(logic_port_id <= AAL_LPORT_CPU_7){
        phy_port_id = AAL_PPORT_CPU;
    }else if(logic_port_id == AAL_LPORT_L3_WAN){
        phy_port_id = AAL_PPORT_L3_WAN;
    }else if(logic_port_id == AAL_LPORT_L3_LAN){
        phy_port_id = AAL_PPORT_L3_LAN;
    }else if(logic_port_id == AAL_LPORT_MC){
        phy_port_id = AAL_PPORT_MC;
    }else if(logic_port_id == AAL_LPORT_OAM){
        phy_port_id = AAL_PPORT_OAM;
    }

    ret = aal_l2_tm_es_port_ena_set(device_id,phy_port_id,enable == 0);
    if(ret != CA_E_OK){
        return ret;
    }
    if(phy_port_id == 7){
        return aal_l3_tm_es_ni_port_ena_set(0,7,enable == 0);
    }
    return CA_E_OK;
}

ca_status_t ca_port_egr_qm_block_get(CA_IN ca_device_id_t device_id,
                                         CA_IN ca_port_id_t port_id,
                                         CA_OUT ca_boolean_t  *enable
                                         )
{
    ca_uint32_t  logic_port_id = 0;
    ca_uint32_t  phy_port_id = 0;
    ca_status_t ret = CA_E_OK;
    ca_boolean_t es_port_ena = FALSE;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_ID_CHECK(port_id);
    if(enable == NULL){
        return CA_E_PARAM;
    }

    logic_port_id = PORT_ID(port_id);
    ret = aal_port_arb_ldpid_pdpid_map_get(device_id,0,0,logic_port_id,&phy_port_id);
    if(CA_E_OK != ret){
        return ret;
    }
    ret = aal_l2_tm_es_port_ena_get(device_id,phy_port_id, &es_port_ena);
    *enable = (es_port_ena == 0);

    return CA_E_OK;
}

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
/** __pon_port_init:
 *@brief: init PON port as ETHERNET mode
 *@return: CA_E_OK if successfully, return others if fail
 */
static ca_status_t __pon_port_init(ca_device_id_t device_id)
{
    ca_uint8_t  pon_mode  = 0;
    ca_status_t ret = CA_E_OK;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_elpb_cfg_msk_t egr_msk;
    aal_elpb_cfg_t egr_cfg;

    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    memset(&egr_msk, 0, sizeof(aal_elpb_cfg_msk_t));
    memset(&egr_cfg, 0, sizeof(aal_elpb_cfg_t));

    igr_msk.s.wan_ind = 1;
    //igr_msk.s.stp_mode = 1;
    igr_cfg.wan_ind = 1;
    igr_cfg.stp_mode = AAL_STP_STATE_FWD_NO_LEARN;
    egr_msk.s.egr_dest_wan = 1;
    egr_cfg.egr_dest_wan = 1;

    ret = scfg_read(device_id, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
        ca_printf("%s(): SCFG read %s failed, use default XGSPON mode!\n", __func__, CFG_ID_PON_MAC_MODE);
        pon_mode = ONU_PON_MAC_XGSPON;
    }

    if (pon_mode == ONU_PON_MAC_ETHERNET) { /* ETHERNET mode */
#if 0//defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        igr_cfg.wan_ind = 0; /* Under L2 switch ethernet mode, no WAN port */
        egr_cfg.egr_dest_wan = 0;
#endif
        igr_cfg.stp_mode = AAL_STP_STATE_FWD_AND_LEARN;
    } else { /* PON mode */
        igr_msk.s.ilpb_gemid_dynamic_mode = 1;
        igr_msk.s.ilpb_flowid_as_gemid = 1;
        igr_cfg.ilpb_gemid_dynamic_mode = 1;
        igr_cfg.ilpb_flowid_as_gemid = 1;
    }

    ret = aal_port_ilpb_cfg_set(device_id, AAL_LPORT_ETH_WAN, igr_msk, &igr_cfg);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_port_elpb_cfg_set(device_id, AAL_LPORT_ETH_WAN, egr_msk, &egr_cfg);
    CA_RET_VALUE_CHECK(ret);

    return ret;

}
#endif



ca_status_t ca_port_init(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t wan_lpid = AAL_LPORT_ETH_WAN;
    ca_uint32_t i = 0;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_elpb_cfg_msk_t egr_msk;
    aal_elpb_cfg_t egr_cfg;

    if (!__port_lock) {
        ret = ca_spin_lock_init(&__port_lock, SPINLOCK_BH);
        CA_RET_VALUE_CHECK(ret);
    }

    memset(&__port_lag_attr, 0, sizeof(__port_lag_attr));
    memset(&__port_tpid_index, 0, sizeof(__port_tpid_index));
    memset(&__port_stp_mode, 0, sizeof(__port_stp_mode));

    ret = aal_port_init(device_id);
    CA_RET_VALUE_CHECK(ret);

    for (i = 0; i < CA_PORT_MAX_NUM; i++) {
        __port_stp_mode[i] = TRUE;     /* stp is enbled by default */
    }

    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));
    memset(&egr_msk, 0, sizeof(aal_elpb_cfg_msk_t));
    memset(&egr_cfg, 0, sizeof(aal_elpb_cfg_t));


    igr_msk.s.wan_ind = 1;
    igr_cfg.wan_ind = 1;
    egr_msk.s.egr_dest_wan = 1;
    egr_cfg.egr_dest_wan = 1;

    ret = aal_port_ilpb_cfg_set(device_id, wan_lpid, igr_msk, &igr_cfg);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_port_elpb_cfg_set(device_id, wan_lpid, egr_msk, &egr_cfg);
    CA_RET_VALUE_CHECK(ret);

    ret = __port_spcl_pkt_init(device_id);
    CA_RET_VALUE_CHECK(ret);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ret = __pon_port_init(device_id);
    CA_RET_VALUE_CHECK(ret);
#endif

#if 0
    for (i = 0; i < CA_PORT_MAX_NUM_ETHERNET; i++) {
        port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
        ret = ca_port_enable_set(0, port_id, 1);
        if(ret != CA_E_OK){
            ca_printf("port enable failed! port_id 0x%x\r\n", port_id);
        }
    }
    /* cross connection setup: init ILPB and DPB table, refer to 7.20.1 in G3 Arch doc */
    /* 1. WAN NI6 (LPORT 0x06) -> L2RP WAN (LPORT 0x09) */
    igr_msk.s.wan_ind = 1;
    igr_msk.s.igr_port_l2uc_redir_en = 1;
    igr_msk.s.igr_port_l2uc_ldpid = 1;
    igr_msk.s.igr_port_l2mc_redir_en = 1;
    igr_msk.s.igr_port_l2mc_ldpid = 1;
    igr_msk.s.igr_port_l2bc_redir_en = 1;
    igr_msk.s.igr_port_l2bc_ldpid = 1;

    igr_cfg.wan_ind = 1;
    igr_cfg.igr_port_l2uc_redir_en = 1;
    igr_cfg.igr_port_l2uc_ldpid = AAL_LPORT_L2RP_WAN;    /* 0x09 */
    igr_cfg.igr_port_l2mc_redir_en = 1;
    igr_cfg.igr_port_l2mc_ldpid = AAL_LPORT_L2RP_WAN;
    igr_cfg.igr_port_l2bc_redir_en = 1;
    igr_cfg.igr_port_l2bc_ldpid = AAL_LPORT_L2RP_WAN;

    ret = aal_port_ilpb_cfg_set(device_id, wan_ni_index, igr_msk, &igr_cfg);    /* by default, wan_ni_index is 0x06 */
    if (ret != CA_E_OK) {
        return ret;
    }

    /* 2. L2RP WAN (LPORT 0x09) -> L3 WAN (LPORT 0x19): enable redirect and set ldpid via ILPB table */
    igr_msk.s.igr_port_l2uc_redir_en = 1;
    igr_msk.s.igr_port_l2uc_ldpid = 1;
    igr_msk.s.igr_port_l2mc_redir_en = 1;
    igr_msk.s.igr_port_l2mc_ldpid = 1;
    igr_msk.s.igr_port_l2bc_redir_en = 1;
    igr_msk.s.igr_port_l2bc_ldpid = 1;

    igr_cfg.igr_port_l2uc_redir_en = 1;
    igr_cfg.igr_port_l2uc_ldpid = AAL_LPORT_L3_WAN;    /* 0x19 */
    igr_cfg.igr_port_l2mc_redir_en = 1;
    igr_cfg.igr_port_l2mc_ldpid = AAL_LPORT_L3_WAN;
    igr_cfg.igr_port_l2bc_redir_en = 1;
    igr_cfg.igr_port_l2bc_ldpid = AAL_LPORT_L3_WAN;

    ret = aal_port_ilpb_cfg_set(device_id, AAL_LPORT_L2RP_WAN, igr_msk, &igr_cfg);    /* 0x09 */
    if (ret != CA_E_OK) {
        return ret;
    }

    /* 3. L2RP LAN (LPORT 0x08) -> L3 LAN (LPORT 0x18): enable redirect and set ldpid via ILPB table */
    igr_msk.s.igr_port_l2uc_redir_en = 1;
    igr_msk.s.igr_port_l2uc_ldpid = 1;
    igr_msk.s.igr_port_l2mc_redir_en = 1;
    igr_msk.s.igr_port_l2mc_ldpid = 1;
    igr_msk.s.igr_port_l2bc_redir_en = 1;
    igr_msk.s.igr_port_l2bc_ldpid = 1;

    igr_cfg.igr_port_l2uc_redir_en = 1;
    igr_cfg.igr_port_l2uc_ldpid = AAL_LPORT_L3_LAN;    /* 0x18 */
    igr_cfg.igr_port_l2mc_redir_en = 1;
    igr_cfg.igr_port_l2mc_ldpid = AAL_LPORT_L3_LAN;
    igr_cfg.igr_port_l2bc_redir_en = 1;
    igr_cfg.igr_port_l2bc_ldpid = AAL_LPORT_L3_LAN;

    ret = aal_port_ilpb_cfg_set(device_id, AAL_LPORT_L2RP_LAN, igr_msk, &igr_cfg);    /* 0x08 */
    if (ret != CA_E_OK) {
        return ret;
    }

    /* set DPB table for the traffic from L3 WAN (0x18), L3 LAN (0x19), OAM, MC, SPECL */
    ret = aal_port_dpb_ctrl_set(device_id, 3);    /* top 3 bits of ldpid 0x18~0x1f (0x011xxx)  = 3 */
    if (ret != CA_E_OK) {
        return ret;
    }

    /* 4. L3 WAN (LPORT 0x18) -> WAN NI (LPORT 0x07): enable L2FE bypass and set ldpid via DPB table */
    memset(&dpb_msk, 0, sizeof(aal_dpb_cfg_msk_t));
    memset(&dpb_cfg, 0, sizeof(aal_dpb_cfg_t));

    dpb_msk.s.bypass = 1;
    dpb_msk.s.ldpid = 1;
    dpb_msk.s.cos = 1;

    dpb_cfg.bypass = 1;
    dpb_cfg.ldpid = AAL_LPORT_ETH_WAN;    /* 0x07 */

    lpid_cos = ((0x011000) << 3) & 0x1f;        /* low 3  bits of lpid 0x18 (0x011000) << 3 | cos (0~7) = 0~7 */
    for (i = 0; i < 7; i++) {
        dpb_cfg.cos = i;
        aal_port_dpb_cfg_set(device_id, lpid_cos + i, dpb_msk, &dpb_cfg);
    }

    /* 5. L3 LAN (LPORT 0x19) -> L2FE: disable L2FE bypass via DPB table */
    memset(&dpb_msk, 0, sizeof(aal_dpb_cfg_msk_t));
    memset(&dpb_cfg, 0, sizeof(aal_dpb_cfg_t));

    dpb_msk.s.bypass = 1;
    dpb_cfg.bypass = 0;

    lpid_cos = ((0x011001) << 3) & 0x1f;        /* low 3  bits of lpid 0x19 (0x011001) << 3 | cos (0~7) = 8~15 */
    for (i = 0; i < 7; i++) {
        aal_port_dpb_cfg_set(device_id, lpid_cos + i, dpb_msk, &dpb_cfg);
    }
 #endif
    return CA_E_OK;

}

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_uint32_t __pon_link_evt_dp_handler(ca_device_id_t  device_id,
    ca_uint32_t  event_id,
    ca_void_t    *event,
    ca_uint32_t  len,
    ca_void_t    *cookie
)
{
    ca_boolean_t  link_status = 0;
    ca_uint32_t   queue = 0;
    ca_uint8_t    pon_mode = 0;
    ca_port_id_t  pon_port_id = CA_PORT_GPON_PORT;
    ca_port_id_t  oam_omci_port = 0x3000F;

    if (NULL == event) {
        return CA_E_PARAM;
    }

    if (CA_E_OK != scfg_read(device_id, CFG_ID_PON_MAC_MODE, sizeof(ca_uint8_t), &pon_mode)) {
        return CA_E_OK;
    }

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        link_status = ((ca_event_gpon_port_link_t *)event)->status;
        pon_port_id = CA_PORT_GPON_PORT;
    } else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        link_status = ((ca_event_epon_port_link_t *)event)->status;
        pon_port_id = CA_PORT_EPON_PORT;
    } else {
        return CA_E_OK;
    }
    if (link_status) {
        /* let all upstream pass */
        ca_port_tx_traffic_block_set(device_id,pon_port_id,FALSE);
        ca_port_tx_traffic_block_set(device_id,oam_omci_port,FALSE);
        ca_port_egr_qm_block_set(device_id,pon_port_id,FALSE);
        ca_port_egr_qm_block_set(device_id,oam_omci_port,FALSE);
        __PORT_DBG("re-cover all upstream when ONU active\r\n");
    } else {
        /* block all upstream pass */
        ca_port_tx_traffic_block_set(device_id,pon_port_id,TRUE);
        ca_port_tx_traffic_block_set(device_id,oam_omci_port,TRUE);

        for (queue = 0; queue < 8; queue++) {
            ca_queue_flush(device_id,0x3000C,queue);
            ca_queue_flush(device_id,0x30007,queue);
            ca_queue_flush(device_id,0xC0007,queue);
        }
        ca_port_egr_qm_block_set(device_id,pon_port_id,TRUE);
        ca_port_egr_qm_block_set(device_id,oam_omci_port,TRUE);

        aal_pon_puc_flush(device_id);
        ca_port_egr_qm_block_set(device_id,pon_port_id,FALSE);
        ca_port_egr_qm_block_set(device_id,pon_port_id,FALSE);
        ca_udelay(500);
        ca_port_egr_qm_block_set(device_id,pon_port_id,TRUE);
        ca_port_egr_qm_block_set(device_id,oam_omci_port,TRUE);
        aal_pon_puc_flush(device_id);


        __PORT_DBG("block all upstream when ONU de-active\r\n");
    }
    return CA_E_OK;
}
#endif

ca_status_t  ca_dp_init(ca_device_id_t device_id)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint8_t  pon_mode = 0;

    if (CA_E_OK == scfg_read(device_id, CFG_ID_PON_MAC_MODE, sizeof(ca_uint8_t), &pon_mode)) {
        if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
            ca_event_register(device_id, CA_EVENT_GPON_PORT_LINK,__pon_link_evt_dp_handler,  NULL);
            ca_printf("ca-ne: GPON LINK event handler __pon_link_evt_dp_handler registered\r\n");
            /* drop all traffic to PON by default, only pass them after ONU active */
            ca_port_tx_traffic_block_set(device_id,CA_PORT_GPON_PORT,TRUE); /* block user data */
            ca_port_tx_traffic_block_set(device_id,0x3000F,TRUE); /* block OMCI */
            ca_port_egr_qm_block_set(0,CA_PORT_GPON_PORT,TRUE);
            ca_port_egr_qm_block_set(0,0x3000F,TRUE);

        } else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
#if 0 // todo: need check with Jianguang
            ca_event_register(device_id, CA_EVENT_EPON_PORT_LINK,__pon_link_evt_dp_handler,  NULL);
            ca_printf("ca-ne: EPON LINK event handler __pon_link_evt_dp_handler registered\r\n");
            /* drop all traffic to PON by default, only pass them after ONU active */
            ca_port_tx_traffic_block_set(device_id,CA_PORT_EPON_PORT,TRUE); /* block user data */
            ca_port_tx_traffic_block_set(device_id,0x3000F,TRUE); /* block OMCI */
            ca_port_egr_qm_block_set(0,CA_PORT_EPON_PORT,TRUE);
            ca_port_egr_qm_block_set(0,0x3000F,TRUE);
#endif
        }
    }

#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_port_arb_ldpid_pdpid_map_set(device_id, 0, 0, AAL_LPORT_9QUEUE_NI7, AAL_PPORT_OAM);
    aal_port_arb_ldpid_pdpid_map_set(device_id, 1, 0, AAL_LPORT_9QUEUE_NI7, AAL_PPORT_OAM);
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_port_arb_ldpid_pdpid_map_set(1, 0, 0, AAL_LPORT_9QUEUE_NI7, AAL_PPORT_OAM);
    aal_port_arb_ldpid_pdpid_map_set(1, 1, 0, AAL_LPORT_9QUEUE_NI7, AAL_PPORT_OAM);
#endif

    return CA_E_OK;
}

EXPORT_SYMBOL(ca_port_link_aggregation_group_delete);
EXPORT_SYMBOL(ca_port_max_frame_size_set);
EXPORT_SYMBOL(ca_port_discard_control_set);
EXPORT_SYMBOL(ca_port_stp_state_set);
EXPORT_SYMBOL(ca_port_pvid_dot1p_get);
EXPORT_SYMBOL(ca_port_isolation_set);
EXPORT_SYMBOL(ca_port_station_move_set);
EXPORT_SYMBOL(ca_port_ipg_get);
EXPORT_SYMBOL(ca_port_link_aggregation_group_create);
EXPORT_SYMBOL(ca_port_tpid_set);
EXPORT_SYMBOL(ca_port_pvid_vlan_set);
EXPORT_SYMBOL(ca_port_max_frame_size_get);
EXPORT_SYMBOL(ca_port_discard_control_get);
EXPORT_SYMBOL(ca_port_untagged_priority_get);
EXPORT_SYMBOL(ca_port_loopback_get);
EXPORT_SYMBOL(ca_port_stp_state_get);
EXPORT_SYMBOL(ca_port_pvid_dot1p_set);
EXPORT_SYMBOL(ca_port_pvid_tpid_get);
EXPORT_SYMBOL(ca_port_ipg_set);
EXPORT_SYMBOL(ca_port_station_move_get);
EXPORT_SYMBOL(ca_port_enable_get);
EXPORT_SYMBOL(ca_port_link_aggregation_group_update);
EXPORT_SYMBOL(ca_port_tpid_get);
EXPORT_SYMBOL(ca_port_loopback_set);
EXPORT_SYMBOL(ca_port_untagged_priority_set);
EXPORT_SYMBOL(ca_port_enable_set);
EXPORT_SYMBOL(ca_port_pvid_tpid_set);
EXPORT_SYMBOL(ca_port_pvid_vlan_get);
EXPORT_SYMBOL(ca_port_isolation_get);
EXPORT_SYMBOL(ca_port_link_aggregation_group_get);
EXPORT_SYMBOL(ca_port_encryption_enable_set);
EXPORT_SYMBOL(ca_port_encryption_enable_get);
EXPORT_SYMBOL(ca_port_encryption_mode_set);
EXPORT_SYMBOL(ca_port_encryption_mode_get);
EXPORT_SYMBOL(ca_port_tx_traffic_block_set);
EXPORT_SYMBOL(ca_port_tx_traffic_block_get);
EXPORT_SYMBOL(ca_port_egr_qm_block_set);
EXPORT_SYMBOL(ca_port_egr_qm_block_get);

