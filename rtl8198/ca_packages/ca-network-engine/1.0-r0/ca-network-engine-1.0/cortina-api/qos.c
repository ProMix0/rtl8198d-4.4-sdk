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
 * @file    qos.c
 * @brief   Cortina QoS APIs implementation
 * @version 1.1
 * @date    2016-03-01
 *
 *****************************************************************************/
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

#if defined(CONFIG_CORTINA_BOARD_FPGA) &&  defined(CONFIG_ARCH_CORTINA_G3HGU)
#define NOT_SUPPORT_L3
#else
#define QOS_SUPPORT_L3  1
#endif

#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#define QOS_SUPPORT_PON  1
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __HGU_SATURN_INTERNAL_PORT  6
#define __HGU_G3_INTERNAL_PORT      7
#endif
#endif

#include "ca_types.h"
#include "ca_module.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_l2_qos.h"
#include "aal_port.h"
#ifdef QOS_SUPPORT_L3
#include "aal_l3_qos.h"
#include "aal_l3_if.h"
#include "aal_l3_stg0.h"
#include "aal_l3fe.h"
#endif
#include "api_common.h"
#include "qos.h"
#include "sys.h"
#include "port.h"
#include "log.h"
#include "ca-scfg/scfg.h"


/* QoS port related MACROs */
#define __FOR_ALL_QOS_PORTS_DO(port)   \
                       for((port)=0; (port)<AAL_PORT_NUM_LOGICAL; ++(port))

#define __QOS_INIT_CHK(dev)  do {\
                                if (!g_qos_init_done) {\
                                    ca_printf("QoS API is NOT initialized ready\r\n"); \
                                    return CA_E_INIT; \
                                }\
                             }while(0)
#define __QOS_COS_CHK(cos)        CA_COS_CHECK(cos)
#define __QOS_DOT1P_CHK(dot1p)    CA_DOT_1P_CHECK(dot1p)
#define __QOS_DSCP_CHK(dscp)      CA_DSCP_CHECK(dscp)
#define __QOS_TC_CHK(tc)          CA_TRAFFIC_CLASS_CHECK(tc)
#define __QOS_PORT_IS_WAN(port)  (PORT_ID(port) == AAL_LPORT_ETH_WAN)
#define __QOS_PORT_IS_L3_WAN(port)  (PORT_TYPE(port) == CA_PORT_TYPE_L3 && PORT_ID(port) == AAL_LPORT_ETH_WAN)
#if defined(QOS_SUPPORT_PON)
#define __QOS_PORT_ID_IS_SUPPORT(port)  (__QOS_PORT_IS_WAN(port) || CA_PORT_IS_PON(port) || CA_PORT_IS_ETHERNET(port) || CA_PORT_IS_CPU(port))
#define __FOR_ALL_PON_LOGICAL_PORT(port) \
        for ((port)= AAL_LPORT_ETH_WAN; (port)<=AAL_LPORT_LLID_GEM_INDEX_31; (port)++) \
            if ((port) == AAL_LPORT_L3_WAN ||(port) == AAL_LPORT_ETH_WAN || (port) >= AAL_LPORT_LLID_GEM_INDEX_0)

#else
#define __QOS_PORT_ID_IS_SUPPORT(port)  (__QOS_PORT_IS_WAN(port) || CA_PORT_IS_ETHERNET(port) || CA_PORT_IS_CPU(port))
#endif

#define __QOS_PORT_ID_CHK(port)  do { \
                                    if (!__QOS_PORT_ID_IS_SUPPORT(port)) {\
                                        __QOS_DBG("Invalid port #x.\n", port); \
                                        return CA_E_PARAM; }\
                                }while(0)

#define __PORT_TYPE_NUM  (CA_PORT_TYPE_CPU + 1)
#define __QOS_INTERNAL_PORT_ID(port) ((PORT_TYPE(port) == CA_PORT_TYPE_L3) ? PORT_ID(port) : PORT_ID(port) + (CA_MAX_PORT_NUMBER * (PORT_TYPE(port))))

#define __QOS_ASSERT_PARAM(x)    CA_ASSERT_RET((x), CA_E_PARAM)


/* lock/mutex MACROs */
#define __QOS_API_LOCK(device_id)    do { \
                                    if (g_qos_init_done)\
                                        ca_spin_lock(g_qos_drv.qos_lock); \
                                   } while(0)
#define __QOS_API_UNLOCK(device_id)  do { \
                                    if (g_qos_init_done)\
                                        ca_spin_unlock(g_qos_drv.qos_lock); \
                                   } while(0)

/* Debug MACROs */
#define __QOS_DBG(fmt,...)  do {\
                                if (ca_api_debug & API_DBG_QOS)\
                                    ca_printf("%s()_line_%d_:"fmt, __func__,__LINE__,##__VA_ARGS__);\
                            }while(0)

/* Default initialized values */
#define __QOS_DEFAULT_MODE       (CA_QOS_MAP_DOT1P_MODE)
#define __DOT1P_DEFAULT_MODE     (CA_DOT1P_MAP_DOT1P_MODE)
#define __DSCP_DEFAULT_MODE      (CA_DSCP_MAP_DSCP_TC_MODE)

/* L3FE LPB table used for WAN/LAN */

#if defined(CONFIG_ARCH_CORTINA_G3LITE)
#define __QOS_L3_LPB_TBL_FOR_WAN  2
#else
#define __QOS_L3_LPB_TBL_FOR_WAN  0
#endif

#define __QOS_L3_LPB_TBL_FOR_LAN  1

/* L3FE Qos profile table used for WAN/LAN */
#define __QOS_L3_PROFILE_FOR_WAN  1
#define __QOS_L3_PROFILE_FOR_LAN  0


/* Local QoS driver definition structures */
typedef struct {
    ca_qos_config_t qos_cfg;
    ca_uint8_t dot1p_map[AAL_L2_QOS_DOT1P_MAX+1];
    ca_uint8_t dot1p_remark[AAL_L2_QOS_DOT1P_MAX+1];
    ca_uint8_t dscp_map[AAL_L2_QOS_DSCP_MAX+1];
    ca_uint8_t dscp_remark[AAL_L2_QOS_DSCP_MAX+1];
    ca_uint8_t dscp_xlat[AAL_L2_QOS_DSCP_MAX+1];
    ca_uint8_t tc_map[AAL_L2_QOS_TC_MAX+1];
    ca_uint8_t tc_remark[AAL_L2_QOS_TC_MAX+1];
    ca_uint8_t tc_xlat[AAL_L2_QOS_TC_MAX+1];
    ca_uint8_t txdot1p_map[AAL_L2_QOS_DOT1P_MAX+1];

} __port_qos_cfg_t;

typedef struct {
    ca_uint8_t innner_8021p_sel;
    ca_uint8_t top_8021p_sel;
}__l3_port_qos_cfg_t;

/* Global variables */
/* QoS driver control block */
static ca_boolean_t      g_qos_init_done = 0; /* QoS initialized done flag */

struct {
    ca_uint           qos_lock;  /* QoS API lock */
#if defined(QOS_SUPPORT_PON)
    ca_uint8_t        pon_mode;
    ca_uint8_t        report_mode;
#endif
    __port_qos_cfg_t  port_qos[CA_PORT_MAX_NUM * __PORT_TYPE_NUM]; /* Local QoS configurations per port */
    __l3_port_qos_cfg_t l3_qos[CA_PORT_MAX_NUM]; /* Local Qos config for L3 I/F */
} g_qos_drv;

/*
 * Internal APIs for other Cortina API modules
 */

/*
 * @qos_l3_if_get
 * Get the Qos config of L3 I/F mapped destination logical port
 */
ca_status_t qos_l3_if_get(
    ca_uint32_t ldpid,
    ca_uint8_t *inner_8021p_sel,
    ca_uint8_t *top_8021p_sel)
{
    CA_NULL_PTR_CHECK(inner_8021p_sel);
    CA_NULL_PTR_CHECK(top_8021p_sel);
    CA_ASSERT_RET(ldpid < CA_PORT_MAX_NUM, CA_E_PARAM);

    *inner_8021p_sel = g_qos_drv.l3_qos[ldpid].innner_8021p_sel;
    *top_8021p_sel   = g_qos_drv.l3_qos[ldpid].top_8021p_sel;
    return CA_E_OK;
}
EXPORT_SYMBOL(qos_l3_if_get);

static ca_status_t __qos_l2_config_set(
                  CA_IN ca_device_id_t     device_id,
                  CA_IN ca_port_id_t    port_id,
                  CA_IN ca_qos_config_t *config)
{
    aal_ilpb_cfg_msk_t ilpb_cfg_mask;
    aal_ilpb_cfg_t     ilpb_cfg;
    ca_status_t        ret = CA_E_OK;
    ca_port_id_t       aal_port = PORT_ID(port_id);

    memset(&ilpb_cfg_mask, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&ilpb_cfg, 0, sizeof(aal_l2_qos_port_config_t));

    ilpb_cfg_mask.s.top_802_1p_mode_sel = 1;
    ilpb_cfg_mask.s.cos_mode_sel        = 1;
    ilpb_cfg_mask.s.dscp_mode_sel       = 1;

    switch (config->qos_map_mode) {/* QoS(queue priority) mapping mode */
    case CA_QOS_MAP_DOT1P_MODE:
        ilpb_cfg.cos_mode_sel = AAL_COS_MODE_MAP_RX_TOP_DOT1P;
        break;
    case CA_QOS_MAP_DSCP_TC_MODE:
        ilpb_cfg.cos_mode_sel = AAL_COS_MODE_MAP_RX_DSCP;
        break;
    case CA_QOS_MAP_CLASSIFIER:
        ilpb_cfg.cos_mode_sel = AAL_COS_MODE_COS_SEL_BMP;
        ilpb_cfg_mask.s.cos_control_bm = 1;

        if((aal_port >= AAL_LPORT_CPU_0)&&(aal_port <= AAL_LPORT_L3_LAN)){
            ilpb_cfg.cos_control_bm = 0x7; /* all check, CLS is highest pri, if not hit, use header A cos value */
        }else{
            ilpb_cfg.cos_control_bm = 0x6;
        }
            
        break;
    case CA_QOS_MAP_TX_DOT1P_MODE:
        ilpb_cfg.cos_mode_sel = AAL_COS_MODE_MAP_TX_TOP_DOT1P;

    default:
        return CA_E_PARAM;
    }

    switch (config->dot1p_remap_mode) {/* Dot1p remapping mode */
    case CA_DOT1P_MAP_DOT1P_MODE:
        ilpb_cfg.top_802_1p_mode_sel = AAL_DOT1P_MODE_MAP_RX_DOT1P;
        break;
    case CA_DOT1P_MAP_DSCP_TC_MODE:
        ilpb_cfg.top_802_1p_mode_sel = AAL_DOT1P_MODE_MAP_TX_DSCP;
        break;
    case CA_DOT1P_MAP_CLASSIFIER:
        ilpb_cfg.top_802_1p_mode_sel = AAL_DOT1P_MODE_DOT1P_SEL_BMP;
        ilpb_cfg_mask.s.dot1p_mark_control_bm = 1;
        ilpb_cfg.dot1p_mark_control_bm = 0x7; /* all check */
        break;
    default:
        return CA_E_PARAM;
    }

    switch (config->dscp_remap_mode) {/* DSCP remapping mode */
    case CA_DSCP_MAP_DSCP_TC_MODE:
        ilpb_cfg.dscp_mode_sel = AAL_DSCP_MODE_MAP_RX_DSCP;
        break;
    case CA_DSCP_MAP_CLASSIFIER:
        ilpb_cfg.dscp_mode_sel = AAL_DSCP_MODE_DSCP_SEL_BMP;
        ilpb_cfg_mask.s.dscp_mark_control_bm = 1;
        ilpb_cfg.dscp_mark_control_bm = 0x6; /* all check */
        break;
    default:
        return CA_E_PARAM;
    }

#if defined(QOS_SUPPORT_PON) && ! defined(CONFIG_LUNA_G3_SERIES)
    if (CA_PORT_IS_PON(port_id)) {
        __FOR_ALL_PON_LOGICAL_PORT(aal_port) { /* Configure PON ports's QoS mode */
            ret |= aal_port_ilpb_cfg_set(CA_DEV_WAN, aal_port, ilpb_cfg_mask, &ilpb_cfg);
        }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        ret |= aal_port_ilpb_cfg_set(CA_DEV_LAN, __HGU_G3_INTERNAL_PORT, ilpb_cfg_mask, &ilpb_cfg);
#endif
    } else {/* Configure non-PON port's QoS mode */
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        ret = aal_port_ilpb_cfg_set(CA_DEV_WAN, __HGU_SATURN_INTERNAL_PORT, ilpb_cfg_mask, &ilpb_cfg); /* internal port */
#endif
        ret |= aal_port_ilpb_cfg_set(CA_DEV_LAN, aal_port, ilpb_cfg_mask, &ilpb_cfg);
    }

    if (CA_E_OK != ret) {
        __QOS_DBG("aal_port_ilpb_cfg_set port %#x ret %#x.\n", aal_port, ret);
        return ret;
    } 
#else
    ret = aal_port_ilpb_cfg_set(device_id, aal_port, ilpb_cfg_mask, &ilpb_cfg);
    if (CA_E_OK != ret) {
        __QOS_DBG("aal_port_ilpb_cfg_set port %#x ret %#x.\n", aal_port, ret);
        return ret;
    }
#endif /* defined(QOS_SUPPORT_PON) */

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (__QOS_PORT_IS_WAN(port_id)) {/* WAN port ingress, L3FE to L2FE, source is changed to be 24 */ 
        aal_port = AAL_LPORT_L3_WAN;
    } else {
        aal_port = AAL_LPORT_L3_LAN;/* LAN port ingress, may go to WAN, its source port will change to be 25 */
    }

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    device_id =  CA_DEV_LAN; /* HGU Gateway SoC */
#endif

    ret = aal_port_ilpb_cfg_set(device_id, aal_port,
        ilpb_cfg_mask, &ilpb_cfg);
    if (CA_E_OK != ret) {
        __QOS_DBG("aal_port_ilpb_cfg_set port %#x ret %#x.\n", aal_port, ret);
        return ret;
    }
#endif /* defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU) */

    return ret;

}

#ifdef QOS_SUPPORT_L3
static ca_status_t __qos_l3_config_set(
                  CA_IN ca_device_id_t     device_id,
                  CA_IN ca_port_id_t    port_id,
                  CA_IN ca_qos_config_t *config)
{
    ca_uint16_t                 entry_idx = 0;
    ca_uint32_t                 l3_tbl_idx = 0;
    fe_l3e_if_tbl_entry_t       entry;
    aal_l3fe_lpb_tbl_cfg_mask_t l3_lpb_mask;
    aal_l3fe_lpb_tbl_cfg_t      l3_lpb;
    ca_port_id_t                aal_port = PORT_ID(port_id);
    ca_status_t                 ret = CA_E_OK;

    l3_lpb_mask.u32 = 0;
    l3_lpb_mask.s.default_cos_sel = 1;

#ifdef CONFIG_ARCH_CORTINA_G3LITE
    l3_lpb.default_cos_sel = 0; /* always copy L2 cos, since pkt pass L2 firstly in G3Lite */

#else

    if (aal_port != AAL_LPORT_ETH_WAN){ /* L3 based Qos*/
        l3_lpb.default_cos_sel = 0; /* copy header-a, use L2-header */
    } else {
        switch (config->qos_map_mode) {
        case CA_QOS_MAP_DOT1P_MODE:
            l3_lpb.default_cos_sel = 2; /* Dot1p mapping mode */
            break;
        case CA_QOS_MAP_DSCP_TC_MODE:
            l3_lpb.default_cos_sel = 3; /* DSCP mapping mode */
            break;
        case CA_QOS_MAP_CLASSIFIER:
            l3_lpb.default_cos_sel = 1; /* default cos mode */
            break;
        default:
            return CA_E_PARAM;
        }
    }
#endif /* end of CONFIG_ARCH_CORTINA_G3LITE */

    l3_tbl_idx = (aal_port == AAL_LPORT_ETH_WAN)
        ? __QOS_L3_LPB_TBL_FOR_WAN
        : __QOS_L3_LPB_TBL_FOR_LAN;

    ret = aal_l3_stg0_lpb_tbl_set(device_id,
            l3_tbl_idx,
            l3_lpb_mask, &l3_lpb);
    if (CA_E_OK != ret) {
        __QOS_DBG("aal_l3_stg0_lpb_tbl_set %u FAIL, return %d.\n", l3_tbl_idx, ret);
        return ret;
    }

    for (entry_idx = 0; entry_idx < FE_L3E_IF_TBL_ENTRY_MAX; entry_idx++) {
        memset(&entry, 0, sizeof(fe_l3e_if_tbl_entry_t));

        ret = aal_entry_get_by_idx(AAL_TABLE_FE_L3E_IF_TBL, entry_idx, &entry);
        if (CA_E_OK != ret) {
            __QOS_DBG("aal_l3_if_get %u QoS FAIL, return %d.\n", entry_idx, ret);
            ret = CA_E_OK;
            continue;
        }

        if ((aal_port == AAL_LPORT_ETH_WAN) && entry.ldpid != AAL_LPORT_L3_LAN)
            continue;

        if (aal_port != AAL_LPORT_ETH_WAN
            && entry.ldpid != AAL_LPORT_L3_WAN
            && entry.ldpid != AAL_LPORT_ETH_WAN)
            continue;

        switch (config->dot1p_remap_mode) {
        case CA_DOT1P_MAP_DOT1P_MODE:
            entry.inner_802_1p_sel = 3;
            entry.top_802_1p_sel   = 3;
            break;
        case CA_DOT1P_MAP_DSCP_TC_MODE:
            entry.inner_802_1p_sel = 4;
            break;
        case CA_DOT1P_MAP_CLASSIFIER:
            entry.inner_802_1p_sel = 0;
            entry.top_802_1p_sel   = 0;
            break;
        default:
            return CA_E_PARAM;
        }

        /*
         * Workaround for L3 I/F qos configuration,
         * save the config here and used in L3 I/F APIs
         */
        g_qos_drv.l3_qos[entry.ldpid].innner_8021p_sel = entry.inner_802_1p_sel;
        g_qos_drv.l3_qos[entry.ldpid].top_8021p_sel = entry.top_802_1p_sel;

        ret = aal_entry_set(AAL_TABLE_FE_L3E_IF_TBL, entry_idx, &entry);
        if (CA_E_OK != ret) {
            __QOS_DBG("aal_l3_if_set %u QoS FAIL, return %d.\n", entry_idx, ret);
        }

    }

    return ret;

}
#endif

/*
 * @ca_qos_config_set
 * Configure QoS global configurations, which include IP packet QoS mapping
 * mode selection, enable or disable remark functions or translate functions.
 */
ca_status_t ca_qos_config_set(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_IN ca_qos_config_t * config)
{
    ca_uint32_t        qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t        ret = CA_E_OK;

    __QOS_DBG("device_id = %u, port_id = 0x%x, "\
        "config.qos_map_mode = %u config.dot1p_remap_mode = %u config.dscp_remap_mode = %u \n",
        device_id, port_id,
        config ? config->qos_map_mode : ~0,
        config ? config->dot1p_remap_mode : ~0,
        config ? config->dscp_remap_mode : ~0);

    __QOS_INIT_CHK(device_id);
    __QOS_ASSERT_PARAM(config != NULL);
    __QOS_PORT_ID_CHK(port_id);

    __QOS_API_LOCK(device_id);

    ret = __qos_l2_config_set(device_id, port_id, config);
    if (CA_E_OK != ret) {
        __QOS_DBG("__qos_l2_config_set port %#x ret %#x.\n", port_id, ret);
        goto __end;
    }

#ifdef QOS_SUPPORT_L3
    ret = __qos_l3_config_set(device_id, port_id, config);
    if (CA_E_OK != ret) {
        __QOS_DBG("__qos_l3_config_set port %#x ret %#x.\n", port_id, ret);
        goto __end;
    }

#endif

    /* Store Qos configuration in local memory */
    memcpy(&g_qos_drv.port_qos[qos_port].qos_cfg, config, sizeof(ca_qos_config_t));

__end:
    __QOS_API_UNLOCK(device_id);

    return ret;

}

/*
 * @ca_qos_config_get
 * Retrive QoS global configurations, which include IP packet QoS mapping
 * mode selection, enable or disable remark functions or translate functions.
 */
ca_status_t ca_qos_config_get(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_OUT ca_qos_config_t * config)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_ASSERT_PARAM(config != NULL);
    __QOS_PORT_ID_CHK(port_id);

    __QOS_API_LOCK(device_id);
    memcpy(config, &g_qos_drv.port_qos[qos_port].qos_cfg, sizeof(ca_qos_config_t));
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "config.qos_map_mode = %u config.dot1p_remap_mode = %u config.dscp_remap_mode = %u \n",
        device_id, port_id,
        config->qos_map_mode, config->dot1p_remap_mode, config->dscp_remap_mode);

    return CA_E_OK;

}

/*
 * @ca_qos_dot1p_map_set
 * Configure mapping from 802.1P to internal priority
 */
ca_status_t ca_qos_dot1p_map_set(CA_IN ca_device_id_t device_id,
                 CA_IN ca_port_id_t port_id, /* Ingress port */
                 CA_IN ca_boolean_t egress,
                 CA_IN ca_uint8_t dot1p,
                 CA_IN ca_uint8_t priority)
{
    aal_l2_qos_dot1p_map_mask_t  map_mask;
    aal_l2_qos_dot1p_map_t       map;
    ca_uint32_t                  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t                  ret = CA_E_OK;
	aal_l2_qos_txdot1p_map_mask_t txdot1p_map_mask;
	aal_l2_qos_txdot1p_map_t txdot1p_map;
	

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "dot1p = %u priority = %u\n",
        device_id, port_id, dot1p, priority);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DOT1P_CHK(dot1p);
    __QOS_COS_CHK(priority);

    __QOS_API_LOCK(device_id);

    if (egress == 0){
    memset(&map, 0, sizeof(map));
    map_mask.u32                    = 0;
    map_mask.s.dot1p_map_mapped_cos = 1;
    map.dot1p_map_mapped_cos        = priority;

    ret = aal_l2_qos_dot1p_map_set(device_id,
                __QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM,
            dot1p, map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_E_OK == ret) {
        ret = aal_l2_qos_dot1p_map_set(CA_DEV_WAN,
                __QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM,
            dot1p, map_mask, &map);
    }
#endif
    }
    else{
        memset(&txdot1p_map, 0, sizeof(txdot1p_map));
        txdot1p_map_mask.u32                      = 0;
        txdot1p_map_mask.s.txdot1p_map_mapped_cos = 1;
        txdot1p_map.txdot1p_map_mapped_cos        = priority;

    	aal_l2_qos_txdot1p_map_set(device_id, 
    	    __QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM,
    	    dot1p, txdot1p_map_mask, &txdot1p_map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (CA_E_OK == ret) {
            ret = aal_l2_qos_txdot1p_map_set(CA_DEV_WAN,
                __QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM,
                dot1p, txdot1p_map_mask, &txdot1p_map);
        }
#endif

    }
#ifdef QOS_SUPPORT_L3

    if (CA_E_OK == ret) {
        aal_l3_qos_dot1p_map_mask_t  l3_qos_mask;
        aal_l3_qos_dot1p_map_cfg_t   l3_qos_cfg;

        /* L3 based QoS */
        l3_qos_mask.u32     = 0;
        l3_qos_mask.s.cos   = 1;
        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_cfg.cos      = priority;

        ret = aal_l3_qos_dot1p_map_set(device_id, dot1p,
            __QOS_PORT_IS_WAN(port_id) /* src is WAN, so dst is LAN */
            ? __QOS_L3_PROFILE_FOR_LAN
            : __QOS_L3_PROFILE_FOR_WAN,
            l3_qos_mask, &l3_qos_cfg);

    }
#endif

    if (CA_E_OK == ret) {
        if (egress == 0)
        g_qos_drv.port_qos[qos_port].dot1p_map[dot1p] = priority;
        else
            g_qos_drv.port_qos[qos_port].txdot1p_map[dot1p] = priority;
    } else {
        __QOS_DBG("call aal_l2_qos_dot1p_map_set fail %#x.\n", ret);
    }

    __QOS_API_UNLOCK(device_id);

    return ret;

}

/*
 * @ca_qos_dot1p_map_set
 * Retrive mapping from 802.1P to internal priority
 */
ca_status_t ca_qos_dot1p_map_get(CA_IN ca_device_id_t device_id,
                 CA_IN  ca_port_id_t port_id,
                 CA_IN  ca_boolean_t egress,
                 CA_IN  ca_uint8_t   dot1p,
                 CA_OUT ca_uint8_t  *priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DOT1P_CHK(dot1p);
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);

    if (egress == 0)
    *priority = g_qos_drv.port_qos[qos_port].dot1p_map[dot1p];
    else
        *priority = g_qos_drv.port_qos[qos_port].txdot1p_map[dot1p];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "dot1p = %u priority = %u\n",
        device_id, port_id, dot1p, *priority);

    return CA_E_OK;
}
/*
Configure or retrieve 802.1p mapping table on given ingress port. (enhanced version)

Parameters:
  device_id              CA device number
  port_id                Ingress, CA port ID
  egress                 0 = do 802.1P to CoS (internal queue id) mapping based on received top tag
                         1 = do 802.1P to CoS (internal queue id) mapping based on transmit top tag
  dot1p                  Ingress packet's 802.1p field values. Points to an array of 8 dot 1P values (dot1p[8])
  priority               Device internal priority values (0~7). Points to an array of 8 priority values (priority[8])
*/
ca_status_t ca_qos_dot1p_map_set_ex(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   egress,
    CA_IN ca_uint8_t     *dot1p,
    CA_IN ca_uint8_t     *priority)
{
    aal_l2_qos_dot1p_map_mask_t  map_mask;
    aal_l2_qos_dot1p_map_t       map;
    ca_uint32_t                  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t                  ret = CA_E_OK;
	aal_l2_qos_txdot1p_map_mask_t txdot1p_map_mask;
	aal_l2_qos_txdot1p_map_t txdot1p_map;
	aal_l2_qos_direction_t direction;
    ca_int32_t i;

    __QOS_DBG("device_id = %u, port_id = %u, dot1p = %u priority = %u\r\n", device_id, port_id, dot1p[0], priority[0]);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);

    for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX; i++){
        __QOS_DOT1P_CHK(dot1p[i]);
        __QOS_COS_CHK(priority[i]);
    }
    __QOS_API_LOCK(device_id);

    direction = (__QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM);

    if (egress == 0){
        memset(&map, 0, sizeof(map));
        map_mask.u32                    = 0;
        map_mask.s.dot1p_map_mapped_cos = 1;

        for(i = 0; i <= AAL_L2_QOS_DOT1P_MAX; i++){
            map.dot1p_map_mapped_cos = priority[i];

            ret = aal_l2_qos_dot1p_map_set(device_id,
                    direction,
                    dot1p[i], map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
                ret = aal_l2_qos_dot1p_map_set(CA_DEV_WAN,
                    direction,
                    dot1p[i], map_mask, &map);
#endif
        }
    }
    else{
        memset(&txdot1p_map, 0, sizeof(txdot1p_map));
        txdot1p_map_mask.u32                      = 0;
        txdot1p_map_mask.s.txdot1p_map_mapped_cos = 1;

        for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX; i++){
            txdot1p_map.txdot1p_map_mapped_cos = priority[i];

            aal_l2_qos_txdot1p_map_set(device_id,
                direction,
                dot1p[i], txdot1p_map_mask, &txdot1p_map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (CA_E_OK == ret) {
                ret = aal_l2_qos_txdot1p_map_set(CA_DEV_WAN,
                    direction,
                    dot1p[i], txdot1p_map_mask, &txdot1p_map);
            }
#endif
        }
    }

    if (CA_E_OK == ret) {
        for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX ; i++){
            if (egress == 0)
                g_qos_drv.port_qos[qos_port].dot1p_map[dot1p[i]] = priority[i];
            else
                g_qos_drv.port_qos[qos_port].txdot1p_map[dot1p[i]] = priority[i];
        }
    } else {
        __QOS_DBG("call aal_l2_qos_dot1p_map_set fail %#x.\n", ret);
    }

    __QOS_API_UNLOCK(device_id);

    return ret;

}

ca_status_t ca_qos_dot1p_map_get_ex(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_boolean_t   egress,
    CA_IN  ca_uint8_t     *dot1p,
    CA_OUT ca_uint8_t     *priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_int32_t i;

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);

    for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX ; i++){
        __QOS_DOT1P_CHK(dot1p[i]);
    }
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);

    for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX ; i++){
        if (egress == 0)
            priority[i] = g_qos_drv.port_qos[qos_port].dot1p_map[dot1p[i]];
        else
            priority[i] = g_qos_drv.port_qos[qos_port].txdot1p_map[dot1p[i]];
    }
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, dot1p = %u priority = %u\r\n",
        device_id, port_id, dot1p[0], priority[0]);

    return CA_E_OK;
}

/*
 * @ca_qos_dot1p_remark_set
 * Configure 802.1p egress remarking table setting for given port.
 */
ca_status_t ca_qos_dot1p_remark_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,/* Egress port */
                    CA_IN ca_uint8_t old_dot1p,
                    CA_IN ca_uint8_t new_dot1p)
{
    ca_status_t                  ret = CA_E_OK;
    ca_uint32_t                  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    aal_l2_qos_dot1p_map_mask_t  map_mask;
    aal_l2_qos_dot1p_map_t       map;


    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dot1p = %u new_dot1p = %u\n",
        device_id, port_id, old_dot1p, new_dot1p);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DOT1P_CHK(old_dot1p);
    __QOS_DOT1P_CHK(new_dot1p);

    __QOS_API_LOCK(device_id);

    memset(&map, 0, sizeof(map));
    map_mask.u32                   = 0;
    map_mask.s.dot1p_map_mapped_1p = 1;
    map.dot1p_map_mapped_1p        = new_dot1p;

    ret = aal_l2_qos_dot1p_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_UPSTREAM 
        : AAL_L2_QOS_DIR_DOWNSTREAM,
        old_dot1p, map_mask, &map);
    if (CA_E_OK != ret) {
        __QOS_DBG("call aal_l2_qos_dot1p_map_set fail %#x.\n", ret);
    }

#ifdef QOS_SUPPORT_L3
    if (__QOS_PORT_IS_WAN(port_id)) {
        aal_l3_qos_dot1p_map_mask_t  l3_qos_mask;
        aal_l3_qos_dot1p_map_cfg_t   l3_qos_cfg;
        ca_uint8_t                   l3_qos_profile = 0;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32     = 0;
        l3_qos_mask.s.dot1p = 1;
        l3_qos_cfg.dot1p    = new_dot1p;

        /* L3 WAN */
        l3_qos_profile = __QOS_L3_PROFILE_FOR_WAN;

        ret = aal_l3_qos_dot1p_map_set(device_id, old_dot1p,
            l3_qos_profile, l3_qos_mask, &l3_qos_cfg);
        if (CA_E_OK != ret) {
            __QOS_DBG("call aal_l3_qos_dot1p_map_set fail %#x.\n", ret);
        }
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].dot1p_remark[old_dot1p] = new_dot1p;
    }

    __QOS_API_UNLOCK(device_id);

#if 0
    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dot1p = %u new_dot1p = %u\n",
        device_id, port_id, old_dot1p, new_dot1p);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_DOT1P_CHK(old_dot1p);
    __QOS_DOT1P_CHK(new_dot1p);

    __QOS_API_LOCK(device_id);

    g_qos_drv.port_qos[qos_port].dot1p_remark[old_dot1p] = new_dot1p;

    __QOS_API_UNLOCK(device_id);
#endif

    return ret;
}

/*
 * @ca_qos_dot1p_remark_get
 * Retrieve 802.1p egress remarking table setting for given port.
 */
ca_status_t ca_qos_dot1p_remark_get(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,
                    CA_IN ca_uint8_t old_dot1p,
                    CA_OUT ca_uint8_t * new_dot1p)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_DOT1P_CHK(old_dot1p);
    __QOS_ASSERT_PARAM(new_dot1p != NULL);

    __QOS_API_LOCK(device_id);
    *new_dot1p = g_qos_drv.port_qos[qos_port].dot1p_remark[old_dot1p];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dot1p = %u new_dot1p = %u\n",
        device_id, port_id, old_dot1p, *new_dot1p);

    return CA_E_OK;

}

/*
 * @ca_qos_dscp_map_set
 * Configure DSCP to internal priority table setting.
 */
ca_status_t ca_qos_dscp_map_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,/* Ingress port */
                CA_IN ca_uint8_t dscp,
                CA_IN ca_uint8_t priority)
{
    aal_l2_qos_dscp_map_mask_t map_mask;
    aal_l2_qos_dscp_map_t      map;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t                ret = CA_E_OK;

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "dscp = %u priority = %u\n",
        device_id, port_id, dscp, priority);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(dscp);
    __QOS_COS_CHK(priority);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;
    map_mask.s.dscp_map_mapped_cos = 1;

    map.dscp_map_mapped_cos = priority;

    __QOS_API_LOCK(device_id);

    ret = aal_l2_qos_dscp_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_DOWNSTREAM
        : AAL_L2_QOS_DIR_UPSTREAM,
        dscp, map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_E_OK == ret) {
        ret = aal_l2_qos_dscp_map_set(CA_DEV_WAN,
            __QOS_PORT_IS_WAN(port_id)
            ? AAL_L2_QOS_DIR_DOWNSTREAM
            : AAL_L2_QOS_DIR_UPSTREAM,
            dscp, map_mask, &map);
    }
#endif

#ifdef QOS_SUPPORT_L3
    if (CA_E_OK == ret) {
        aal_l3_qos_dscp_map_cfg_mask_t l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t      l3_qos_cfg;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32   = 0;
        l3_qos_mask.s.cos = 1;
        l3_qos_cfg.cos    = priority;
        ret = aal_l3_qos_dscp_map_set(device_id,
                dscp,
                __QOS_PORT_IS_WAN(port_id) /* src is WAN, so dst is LAN */
                ? __QOS_L3_PROFILE_FOR_LAN
                : __QOS_L3_PROFILE_FOR_WAN,
                l3_qos_mask, &l3_qos_cfg);
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].dscp_map[dscp] = priority;
    } else {
        __QOS_DBG("call aal_l2_qos_dscp_map_set fail %#x.\n", ret);
    }

    __QOS_API_UNLOCK(device_id);

    return ret;

}

/*
 * @ca_qos_dscp_map_get
 * Retrieve DSCP to internal priority table setting.
 */
ca_status_t ca_qos_dscp_map_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_port_id_t port_id,
                CA_IN ca_uint8_t dscp,
                CA_OUT ca_uint8_t * priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(dscp);
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);
    *priority = g_qos_drv.port_qos[qos_port].dscp_map[dscp];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "dscp = %u priority = %u\n",
        device_id, port_id, dscp, *priority);

    return CA_E_OK;

}

/*
Configure or retrieve DSCP to internal priority table on given ingress port. (enhanced version)
 Parameters:
  device_id     CA device number
  port_id       Ingress CA port ID
  dscp          Ingress packet's DSCP values(0~63). Points to an array of 64 DSCP values(dscp[64])
  priority      Device internal priority values (0~7).Points to an array of 64 priority values(priority[64])
*/

ca_status_t ca_qos_dscp_map_set_ex(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_port_id_t              port_id,
    CA_IN ca_uint8_t                *dscp,
    CA_IN ca_uint8_t                *priority)
{
    aal_l2_qos_dscp_map_mask_t map_mask;
    aal_l2_qos_dscp_map_t      map;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t                ret = CA_E_OK;
    aal_l2_qos_direction_t direction;
    ca_int32_t i;

    __QOS_DBG("device_id = %u, port_id = %u, dscp = %u priority = %u\r\n", device_id, port_id, dscp[0], priority[0]);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    for (i = 0; i <= AAL_L2_QOS_DSCP_MAX; i++)
        __QOS_DSCP_CHK(dscp[i]);

    for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX; i++)
      __QOS_COS_CHK(priority[i]);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;
    map_mask.s.dscp_map_mapped_cos = 1;

    direction = (__QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM);

    __QOS_API_LOCK(device_id);

    for (i = 0; i <= AAL_L2_QOS_DSCP_MAX; i++){
        map.dscp_map_mapped_cos = priority[i % 8];
        ret = aal_l2_qos_dscp_map_set(device_id,
            direction,
            dscp[i], map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (CA_E_OK == ret) {
            ret = aal_l2_qos_dscp_map_set(CA_DEV_WAN,
                direction,
                dscp[i], map_mask, &map);
        }
#endif
    }
#if 0 //def QOS_SUPPORT_L3
    if (CA_E_OK == ret) {
        aal_l3_qos_dscp_map_cfg_mask_t l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t      l3_qos_cfg;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32   = 0;
        l3_qos_mask.s.cos = 1;
        l3_qos_cfg.cos    = priority;
        ret = aal_l3_qos_dscp_map_set(device_id,
                dscp,
                __QOS_PORT_IS_WAN(port_id) /* src is WAN, so dst is LAN */
                ? __QOS_L3_PROFILE_FOR_LAN
                : __QOS_L3_PROFILE_FOR_WAN,
                l3_qos_mask, &l3_qos_cfg);
    }
#endif

    if (CA_E_OK == ret) {
        for (i = 0; i <= AAL_L2_QOS_DSCP_MAX; i++){
            g_qos_drv.port_qos[qos_port].dscp_map[dscp[i]] = priority[i%8];
        }
    } else {
        __QOS_DBG("call aal_l2_qos_dscp_map_set fail %#x.\n", ret);
    }

    __QOS_API_UNLOCK(device_id);

    return ret;

}

ca_status_t ca_qos_dscp_map_get_ex(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_IN  ca_uint8_t               *dscp,
    CA_OUT ca_uint8_t               *priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_int32_t i;

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    for (i = 0; i <= AAL_L2_QOS_DSCP_MAX; i++)
        __QOS_DSCP_CHK(dscp[i]);
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);
    for (i = 0; i <= AAL_L2_QOS_DSCP_MAX; i++)
        priority[i%8] = g_qos_drv.port_qos[qos_port].dscp_map[dscp[i]];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, dscp = %u priority = %u\r\n",
        device_id, port_id, dscp[0], priority[0]);

    return CA_E_OK;

}

/*
 * @ca_qos_dscp_remark_set
 * Configure DSCP egress remarking table setting for given port.
 */
ca_status_t ca_qos_dscp_remark_set(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,/* Ingress port ID */
                   CA_IN ca_uint8_t old_dscp,
                   CA_IN ca_uint8_t new_dscp)
{
    ca_status_t                ret = CA_E_OK;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    aal_l2_qos_dscp_map_mask_t map_mask;
    aal_l2_qos_dscp_map_t      map;

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dscp = %u new_dscp = %u\n",
        device_id, port_id, old_dscp, new_dscp);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(old_dscp);
    __QOS_DSCP_CHK(new_dscp);

    __QOS_API_LOCK(device_id);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;
    map_mask.s.dscp_map_mapped_dscp = 1;
    map.dscp_map_mapped_dscp = new_dscp;
    ret = aal_l2_qos_dscp_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_DOWNSTREAM
        : AAL_L2_QOS_DIR_UPSTREAM,
        old_dscp, map_mask, &map);
    if (CA_E_OK != ret) {
        __QOS_DBG("call aal_l2_qos_dot1p_map_set fail %#x.\n", ret);
        goto __end;
    }
    
#ifdef QOS_SUPPORT_L3
    if (__QOS_PORT_IS_WAN(port_id)) {
        aal_l3_qos_dscp_map_cfg_mask_t  l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t       l3_qos_cfg;
        ca_uint8_t                      l3_qos_profile = 0;
        
        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32    = 0;
        l3_qos_mask.s.dscp = 1;
        l3_qos_cfg.dscp    = new_dscp;

            /* L3 WAN */
        l3_qos_profile = __QOS_L3_PROFILE_FOR_WAN;

        ret = aal_l3_qos_dscp_map_set(device_id, old_dscp,
            l3_qos_profile, l3_qos_mask, &l3_qos_cfg);
        if (CA_E_OK != ret) {
            __QOS_DBG("call aal_l3_qos_dscp_map_set fail %#x.\n", ret);
        }
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].dscp_remark[old_dscp] = new_dscp;
    }

__end:
    __QOS_API_UNLOCK(device_id);
#if 0

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dscp = %u new_dscp = %u\n",
        device_id, port_id, old_dscp, new_dscp);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(old_dscp);
    __QOS_DSCP_CHK(new_dscp);

    __QOS_API_LOCK(device_id);
    g_qos_drv.port_qos[qos_port].dscp_remark[old_dscp] = new_dscp;
    __QOS_API_UNLOCK(device_id);
#endif

    return ret;

}

/*
 * @ca_qos_dscp_remark_get
 * Retrieve DSCP egress remarking table setting for given port.
 */
ca_status_t ca_qos_dscp_remark_get(CA_IN ca_device_id_t device_id,
                   CA_IN ca_port_id_t port_id,
                   CA_IN ca_uint8_t old_dscp,
                   CA_OUT ca_uint8_t * new_dscp)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(old_dscp);
    __QOS_ASSERT_PARAM(new_dscp != NULL);

    __QOS_API_LOCK(device_id);
    *new_dscp = g_qos_drv.port_qos[qos_port].dscp_remark[old_dscp];
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, "\
        "old_dscp = %u new_dscp = %u\n",
        device_id, port_id, old_dscp, *new_dscp);

    return CA_E_OK;
}

/*
 * @ca_qos_dscp_dot1p_translate_set
 * Configure DSCP to 802.1p egress translation table setting for given port.
 */
ca_status_t ca_qos_dscp_dot1p_translate_set(CA_IN ca_device_id_t device_id,
                        CA_IN ca_port_id_t port_id, /* For L2: Ingress port; For L3: Egress port */
                        CA_IN ca_uint8_t dscp, /* Egress DSCP */
                        CA_IN ca_uint8_t dot1p)/* Mapped Dot1p */
{
    ca_status_t                ret = CA_E_OK;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    aal_l2_qos_txdscp_map_mask_t   map_mask;
    aal_l2_qos_txdscp_map_t        map;

    __QOS_DBG("device_id = %u, port_id = %u, dscp = %u dot1p = %u\n",
        device_id, port_id, dscp, dot1p);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(dscp);
    __QOS_DOT1P_CHK(dot1p);

    __QOS_API_LOCK(device_id);

    memset(&map, 0, sizeof(map));
    map_mask.u32                    = 0;
    map_mask.s.txdscp_map_mapped_1p = 1;
    map.txdscp_map_mapped_1p        = dot1p;

    ret = aal_l2_qos_txdscp_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_DOWNSTREAM
        : AAL_L2_QOS_DIR_UPSTREAM,
        dscp, map_mask, &map);
    if (CA_E_OK != ret) {
        __QOS_DBG("call aal_l2_qos_txdscp_map_set fail %#x.\n", ret);
        goto __end;
    }

#ifdef QOS_SUPPORT_L3
    if (__QOS_PORT_IS_WAN(port_id)) {
        aal_l3_qos_dscp_map_cfg_mask_t  l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t   l3_qos_cfg;
        ca_uint8_t                   l3_qos_profile = 0;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32     = 0;
        l3_qos_mask.s.dot1p = 1;
        l3_qos_cfg.dot1p    = dot1p;

            /* L3 WAN */
        l3_qos_profile = __QOS_L3_PROFILE_FOR_WAN;

        ret = aal_l3_qos_dscp_map_set(device_id, dscp,
            l3_qos_profile, l3_qos_mask, &l3_qos_cfg);
        if (CA_E_OK != ret) {
            __QOS_DBG("call aal_l3_qos_dscp_map_set fail %#x.\n", ret);
        }
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].dscp_xlat[dscp] = dot1p;
    }

__end:
    __QOS_API_UNLOCK(device_id);

#if 0

    __QOS_DBG("device_id = %u, port_id = %u, dscp = %u dot1p = %u\n",
        device_id, port_id, dscp, dot1p);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(dscp);
    __QOS_DOT1P_CHK(dot1p);

    __QOS_API_LOCK(device_id);
    g_qos_drv.port_qos[qos_port].dscp_xlat[dscp] = dot1p;
    __QOS_API_UNLOCK(device_id);
#endif

    return ret;

}

/*
 * @ca_qos_dscp_dot1p_translate_get
 * Retrive DSCP to 802.1p egress translation table setting for given port.
 */
ca_status_t ca_qos_dscp_dot1p_translate_get(
                CA_IN  ca_device_id_t  device_id,
                CA_IN  ca_port_id_t port_id, /* Egress port */
                CA_IN  ca_uint8_t   dscp,/* Egress DSCP */
                CA_OUT ca_uint8_t * dot1p) /* translated 802.1p priority */
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_DSCP_CHK(dscp);
    __QOS_ASSERT_PARAM(dot1p != NULL);

    __QOS_API_LOCK(device_id);
    *dot1p = g_qos_drv.port_qos[qos_port].dscp_xlat[dscp];
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, dscp = %u dot1p = %u\n",
        device_id, port_id, dscp, *dot1p);

    return CA_E_OK;

}

/*
 * @ca_qos_tc_map_set
 * Configure TC to internal priority table setting.
 */
ca_status_t ca_qos_tc_map_set(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,/*  Ingress port */
                  CA_IN ca_uint8_t tc,
                  CA_IN ca_uint8_t priority)

{
    aal_l2_qos_tc_map_mask_t map_mask;
    aal_l2_qos_tc_map_t map;
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u priority = %u\n",
        device_id, port_id, tc, priority);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(tc);
    __QOS_COS_CHK(priority);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;
    map_mask.s.tc_map_mapped_cos = 1;

    map.tc_map_mapped_cos = priority;

    __QOS_API_LOCK(device_id);

    ret = aal_l2_qos_tc_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_DOWNSTREAM
        : AAL_L2_QOS_DIR_UPSTREAM,
        tc , map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (CA_E_OK == ret) {
        ret = aal_l2_qos_tc_map_set(CA_DEV_WAN,
            __QOS_PORT_IS_WAN(port_id)
            ? AAL_L2_QOS_DIR_DOWNSTREAM
            : AAL_L2_QOS_DIR_UPSTREAM,
            tc , map_mask, &map);
    }
#endif

#ifdef QOS_SUPPORT_L3
    if (CA_E_OK == ret) {
        aal_l3_qos_dscp_map_cfg_mask_t  l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t      l3_qos_cfg;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32   = 0;
        l3_qos_mask.s.cos = 1;
        l3_qos_cfg.cos    = priority;
        ret = aal_l3_qos_tc_map_set(device_id,
                (tc >> 2) & 0x3F, /* L3FE only support 0~63 TC */
                __QOS_PORT_IS_WAN(port_id) /* src is WAN, so dst is LAN */
                ? __QOS_L3_PROFILE_FOR_LAN
                : __QOS_L3_PROFILE_FOR_WAN,
                l3_qos_mask, &l3_qos_cfg);
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].tc_map[tc] = priority;
    } else {
        __QOS_DBG("call aal_l2_qos_tc_map_set fail %#x.\n", ret);
    }
    __QOS_API_UNLOCK(device_id);

    return ret;
}

/*
 * @ca_qos_tc_map_set
 * Retrive TC to internal priority table setting.
 */
ca_status_t ca_qos_tc_map_get(CA_IN ca_device_id_t device_id,
                  CA_IN ca_port_id_t port_id,
                  CA_IN ca_uint8_t tc,
                  CA_OUT ca_uint8_t * priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(tc);
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);
    *priority = g_qos_drv.port_qos[qos_port].tc_map[tc];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u priority = %u\n",
        device_id, port_id, tc, *priority);

    return CA_E_OK;
}

/*
Configure or retrieve IPv6 traffic class to internal priority table setting on a given ingress port (enhanced version)
 Parameters
  device_id   device number
  port_id     Ingress CA port ID
  tc          Ingress packet's Traffic Class(6 bits DSCP 0~63, 2 bits ECN). Points to an array of 64 TC values (tc[64])
  priority    Device internal priority values(0~7). Points to an array of 64 priority values (priority[64])
*/

ca_status_t ca_qos_tc_map_set_ex(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_port_id_t               port_id,
    CA_IN ca_uint8_t                 *tc,
    CA_IN ca_uint8_t                 *priority)
{
    aal_l2_qos_tc_map_mask_t map_mask;
    aal_l2_qos_tc_map_t map;
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;
    ca_int32_t i;
    aal_l2_qos_direction_t direction;

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u priority = %u\r\n",
        device_id, port_id, tc[0], priority[0]);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    for (i = 0; i <= AAL_L2_QOS_TC_MAX; i++)
        __QOS_TC_CHK(tc[i]);
    for (i = 0; i <= AAL_L2_QOS_DOT1P_MAX; i++)
        __QOS_COS_CHK(priority[i]);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;
    map_mask.s.tc_map_mapped_cos = 1;

    direction = (__QOS_PORT_IS_WAN(port_id) ? AAL_L2_QOS_DIR_DOWNSTREAM : AAL_L2_QOS_DIR_UPSTREAM);
    __QOS_API_LOCK(device_id);

    for (i = 0; i <= AAL_L2_QOS_TC_MAX; i++){

        map.tc_map_mapped_cos = priority[i%32];

        ret = aal_l2_qos_tc_map_set(device_id,
            direction,
            tc[i], map_mask, &map);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (CA_E_OK == ret) {
            ret = aal_l2_qos_tc_map_set(CA_DEV_WAN,
                direction,
                tc[i], map_mask, &map);
        }
#endif
    }
#if 0 //def QOS_SUPPORT_L3
    if (CA_E_OK == ret) {
        aal_l3_qos_dscp_map_cfg_mask_t  l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t      l3_qos_cfg;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32   = 0;
        l3_qos_mask.s.cos = 1;
        l3_qos_cfg.cos    = priority;
        ret = aal_l3_qos_tc_map_set(device_id,
                (tc >> 2) & 0x3F, /* L3FE only support 0~63 TC */
                __QOS_PORT_IS_WAN(port_id) /* src is WAN, so dst is LAN */
                ? __QOS_L3_PROFILE_FOR_LAN
                : __QOS_L3_PROFILE_FOR_WAN,
                l3_qos_mask, &l3_qos_cfg);
    }
#endif

    if (CA_E_OK == ret) {
        for (i = 0; i <= AAL_L2_QOS_TC_MAX; i++)
            g_qos_drv.port_qos[qos_port].tc_map[tc[i]] = priority[i%32];
    } else {
        __QOS_DBG("call aal_l2_qos_tc_map_set fail %#x.\n", ret);
    }
    __QOS_API_UNLOCK(device_id);

    return ret;
}

ca_status_t ca_qos_tc_map_get_ex(
    CA_IN  ca_device_id_t             device_id,
    CA_IN  ca_port_id_t               port_id,
    CA_IN  ca_uint8_t                 *tc,
    CA_OUT ca_uint8_t                 *priority)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    ca_int32_t i;

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);

    for (i = 0; i <= AAL_L2_QOS_TC_MAX; i++)
        __QOS_TC_CHK(tc[i]);
    __QOS_ASSERT_PARAM(priority != NULL);

    __QOS_API_LOCK(device_id);
    priority[i%32] = g_qos_drv.port_qos[qos_port].tc_map[tc[i]];

    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u priority = %u\n",
        device_id, port_id, tc[0], priority[0]);

    return CA_E_OK;
}


/*
 * @ca_qos_tc_remark_set
 * Configure TC egress remarking table setting for given port.
 */
ca_status_t ca_qos_tc_remark_set(CA_IN ca_device_id_t device_id,
                 CA_IN ca_port_id_t port_id,/* For L2: Ingress port; For L3: Egress port */
                 CA_IN ca_uint8_t old_tc,
                 CA_IN ca_uint8_t new_tc)
{
    ca_status_t                ret = CA_E_OK;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    aal_l2_qos_tc_map_mask_t   map_mask;
    aal_l2_qos_tc_map_t        map;

    __QOS_DBG("device_id = %u, port_id = %u, old_tc = %u new_tc = %u\n",
        device_id, port_id, old_tc, new_tc);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(old_tc);
    __QOS_TC_CHK(new_tc);

    __QOS_API_LOCK(device_id);

    memset(&map, 0, sizeof(map));
    map_mask.u32                  = 0;
    map_mask.s.tc_map_mapped_dscp = 1;
    map.tc_map_mapped_dscp        = new_tc;

    ret = aal_l2_qos_tc_map_set(device_id,
        __QOS_PORT_IS_WAN(port_id)
        ? AAL_L2_QOS_DIR_DOWNSTREAM
        : AAL_L2_QOS_DIR_UPSTREAM,
        old_tc, map_mask, &map);
    if (CA_E_OK != ret) {
        __QOS_DBG("call aal_l2_qos_tc_map_set fail %#x.\n", ret);
        goto __end;
    }

  
#ifdef QOS_SUPPORT_L3
    if (__QOS_PORT_IS_WAN(port_id)) {
        aal_l3_qos_dscp_map_cfg_mask_t  l3_qos_mask;
        aal_l3_qos_dscp_map_cfg_t       l3_qos_cfg;
        ca_uint8_t                      l3_qos_profile = 0;

        memset(&l3_qos_cfg, 0, sizeof(l3_qos_cfg));
        l3_qos_mask.u32    = 0;
        l3_qos_mask.s.dscp = 1;
        l3_qos_cfg.dscp    = new_tc;
        /* L3 WAN */
        l3_qos_profile = __QOS_L3_PROFILE_FOR_WAN;

        ret = aal_l3_qos_dscp_map_set(device_id, old_tc,
            l3_qos_profile, l3_qos_mask, &l3_qos_cfg);
        if (CA_E_OK != ret) {
            __QOS_DBG("call aal_l3_qos_dscp_map_set fail %#x.\n", ret);
        }
    }
#endif

    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].tc_remark[old_tc] = new_tc;
    }

__end:
    __QOS_API_UNLOCK(device_id);

#if 0
    __QOS_DBG("device_id = %u, port_id = %u, old_tc = %u new_tc = %u\n",
        device_id, port_id, old_tc, new_tc);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(old_tc);
    __QOS_TC_CHK(new_tc);

    __QOS_API_LOCK(device_id);
    g_qos_drv.port_qos[qos_port].tc_remark[old_tc] = new_tc;
    __QOS_API_UNLOCK(device_id);
#endif

    return ret;

}
/*
 * @ca_qos_tc_remark_get
 * Retrive TC egress remarking table setting for given port.
 */
ca_status_t ca_qos_tc_remark_get(CA_IN ca_device_id_t device_id,
                 CA_IN ca_port_id_t port_id,
                 CA_IN ca_uint8_t old_tc,
                 CA_OUT ca_uint8_t * new_tc)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(old_tc);
    __QOS_ASSERT_PARAM(new_tc != NULL);

    __QOS_API_LOCK(device_id);
    *new_tc = g_qos_drv.port_qos[qos_port].tc_remark[old_tc];
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, old_tc = %u new_tc = %u\n",
        device_id, port_id, old_tc, *new_tc);

    return CA_E_OK;

}

/*
 * @ca_qos_tc_dot1p_translate_set
 * Configure TC to 802.1p egress translation table setting for given port.
 */
ca_status_t ca_qos_tc_dot1p_translate_set(CA_IN ca_device_id_t device_id,
                      CA_IN ca_port_id_t port_id, /* Egress port */
                      CA_IN ca_uint8_t tc,
                      CA_IN ca_uint8_t dot1p)
{
    ca_status_t                ret = CA_E_OK;
    ca_uint32_t                qos_port = __QOS_INTERNAL_PORT_ID(port_id);
    aal_l2_qos_txtc_map_mask_t map_mask;
    aal_l2_qos_txtc_map_t map;
    aal_l2_qos_direction_t  direction;

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u dot1p = %u\n",
        device_id, port_id, tc, dot1p);

    __QOS_INIT_CHK(device_id);
    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(tc);
    __QOS_DOT1P_CHK(dot1p);

    memset(&map, 0, sizeof(map));
    map_mask.u32 = 0;

    map_mask.s.txtc_map_mapped_1p = 1;
    map.txtc_map_mapped_1p = dot1p;

    if (__QOS_PORT_IS_WAN(port_id)) {
        direction = AAL_L2_QOS_DIR_UPSTREAM;
    } else {
        direction = AAL_L2_QOS_DIR_DOWNSTREAM;
    }

    __QOS_API_LOCK(device_id);

    ret = aal_l2_qos_txtc_map_set(device_id, direction, tc , map_mask, &map);
    if (CA_E_OK == ret) {
        g_qos_drv.port_qos[qos_port].tc_xlat[tc] = dot1p;
    } else {
        __QOS_DBG("call aal_l2_qos_txtc_map_set fail %#x.\n", ret);
    }

    __QOS_API_UNLOCK(device_id);

#if 0
    __QOS_DBG("device_id = %u, port_id = %u, tc = %u dot1p = %u\n",
        device_id, port_id, tc, dot1p);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(tc);
    __QOS_DOT1P_CHK(dot1p);


    __QOS_API_LOCK(device_id);
    g_qos_drv.port_qos[qos_port].tc_xlat[tc] = dot1p;
    __QOS_API_UNLOCK(device_id);
#endif

    return ret;

}

/*
 * @ca_qos_tc_dot1p_translate_get
 * Retrive TC to 802.1p egress translation table setting for given port.
 */
ca_status_t ca_qos_tc_dot1p_translate_get(CA_IN ca_device_id_t device_id,
                      CA_IN ca_port_id_t port_id,
                      CA_IN ca_uint8_t tc,
                      CA_OUT ca_uint8_t * dot1p)
{
    ca_uint32_t  qos_port = __QOS_INTERNAL_PORT_ID(port_id);

    __QOS_PORT_ID_CHK(port_id);
    __QOS_TC_CHK(tc);
    __QOS_ASSERT_PARAM(dot1p != NULL);

    __QOS_API_LOCK(device_id);
    *dot1p = g_qos_drv.port_qos[qos_port].tc_xlat[tc];
    __QOS_API_UNLOCK(device_id);

    __QOS_DBG("device_id = %u, port_id = %u, tc = %u dot1p = %u\n",
        device_id, port_id, tc, *dot1p);

    return CA_E_OK;

}

/* @__qos_mode_init
 * Init Port Qos mode configuration
 * NOTE: only for ca_qos_init invoking
 */
static ca_status_t __qos_mode_init(CA_IN ca_device_id_t device_id)
{
    aal_ilpb_cfg_msk_t    ilpb_cfg_mask;
    aal_ilpb_cfg_t        ilpb_cfg;
    ca_uint32_t           qos_port = 0;
    ca_status_t           ret = CA_E_OK;
    ca_uint8_t            qos_map_use_dft = 0;

    memset(&ilpb_cfg_mask, 0, sizeof(ilpb_cfg_mask));
    memset(&ilpb_cfg, 0, sizeof(aal_l2_qos_port_config_t));

    ilpb_cfg.cos_mode_sel          = AAL_COS_MODE_MAP_RX_TOP_DOT1P;
    ilpb_cfg_mask.s.cos_control_bm = 1;
    ilpb_cfg.cos_control_bm        = 0x6;
    ilpb_cfg.top_802_1p_mode_sel   = AAL_DOT1P_MODE_MAP_RX_DOT1P;
    ilpb_cfg.dscp_mode_sel         = AAL_DSCP_MODE_MAP_RX_DSCP;

    ilpb_cfg_mask.s.top_802_1p_mode_sel = 1;
    ilpb_cfg_mask.s.dscp_mode_sel       = 1;
    ilpb_cfg_mask.s.cos_mode_sel        = 1;

    /* Global port qos mode */
    __FOR_ALL_QOS_PORTS_DO(qos_port) {
        ret = aal_port_ilpb_cfg_set(device_id, qos_port, ilpb_cfg_mask, &ilpb_cfg);
        if (CA_E_OK != ret) {
            ca_printf("Init ILPB %u QoS mode FAIL, return %d.\n", qos_port, ret);
        }
    }

#ifdef QOS_SUPPORT_L3
    if (CA_E_OK == ret) {
        /* following is L3FE QoS */
        ca_uint16_t           entry_idx = 0;
        //fe_l3e_if_tbl_entry_t entry;
        aal_l3fe_lpb_tbl_cfg_mask_t l3_lpb_mask;
        aal_l3fe_lpb_tbl_cfg_t  l3_lpb;

        /* L3 Qos profile */
        l3_lpb_mask.u32 = 0;
        l3_lpb_mask.s.default_cos_sel     = 1;
        l3_lpb_mask.s.default_cos_tbl_sel = 1;

        for (entry_idx = 0; entry_idx < AAL_L3FE_LPB_TBL_NUM; entry_idx++) {

            ret = aal_l3_stg0_lpb_tbl_get(device_id, entry_idx, &l3_lpb);
            if (CA_E_OK != ret) {
                __QOS_DBG("aal_l3_stg0_lpb_tbl_get %d FAIL, return %d.\n", entry_idx, ret);
                continue;
            }
#ifdef CONFIG_ARCH_CORTINA_G3LITE
            l3_lpb.default_cos_sel = 0; /* from L2 cos mode */

#else
            l3_lpb.default_cos_sel = 2; /* Dot1p mapping mode */
#endif
            if (l3_lpb.default_ldpid == AAL_LPORT_L3_WAN
                || l3_lpb.default_ldpid == AAL_LPORT_ETH_WAN) {
                l3_lpb.default_cos_tbl_sel = __QOS_L3_PROFILE_FOR_WAN;
            } else if (l3_lpb.default_ldpid == AAL_LPORT_L3_LAN) {
                l3_lpb.default_cos_tbl_sel = __QOS_L3_PROFILE_FOR_LAN;
            }

            ret = aal_l3_stg0_lpb_tbl_set(device_id, entry_idx, l3_lpb_mask, &l3_lpb);
            if (CA_E_OK != ret) {
                __QOS_DBG("Init L3 IF %u QoS FAIL, return %d.\n", entry_idx, ret);
            }

        }
    }

    if (CA_E_OK != scfg_read(0, CFG_ID_PORT_INGRESS_COS_MAPPING_USE_DEFAULT, sizeof(qos_map_use_dft), (ca_uint8_t *) &qos_map_use_dft)) {
        printk("SCFG read %s failed!!!\n", CFG_ID_PORT_INGRESS_COS_MAPPING_USE_DEFAULT);
    }

    (void)aal_l2_qos_keep_original_cos_set(device_id, qos_map_use_dft); /* keep original COS from L3FE to L2FE */

#endif

    return ret;

}


/* @__qos_global_init
 * Init QoS driver resource
 * NOTE: only for ca_qos_init invoking
 */
static ca_status_t __qos_global_init(CA_IN ca_device_id_t device_id)
{
    ca_uint32_t  port = 0, dot1p = 0, dscp = 0, tc = 0;

    memset(&g_qos_drv, 0, sizeof(g_qos_drv));

    for (port = 0; port < CA_PORT_MAX_NUM * __PORT_TYPE_NUM; ++port) {

        g_qos_drv.port_qos[port].qos_cfg.qos_map_mode     = __QOS_DEFAULT_MODE;
        g_qos_drv.port_qos[port].qos_cfg.dot1p_remap_mode = __DOT1P_DEFAULT_MODE;
        g_qos_drv.port_qos[port].qos_cfg.dscp_remap_mode  = __DSCP_DEFAULT_MODE;

        /* Map dot1p 0 to 0, 1 to 1 ..., i.e. 1 to one */
        for (dot1p = 0; dot1p <= AAL_L2_QOS_DOT1P_MAX; ++dot1p) {
            g_qos_drv.port_qos[port].dot1p_map[dot1p]    = dot1p;
            g_qos_drv.port_qos[port].dot1p_remark[dot1p] = dot1p;
        }

        /* Map dscp 0~7 to 0, 8~15 to 1 ...,i.e. 7 to one */
        for (dscp = 0; dscp <= AAL_L2_QOS_DSCP_MAX; ++dscp) {
            g_qos_drv.port_qos[port].dscp_map[dscp]    = dscp / 8;
            g_qos_drv.port_qos[port].dscp_remark[dscp] = dscp;
            g_qos_drv.port_qos[port].dscp_xlat[dscp]   = dscp / 8;
        }

        /* Map tc 0~31 to 0, 32~63 to 1 ..., i.e. 32 to one */
        for (tc = 0; tc <= AAL_L2_QOS_TC_MAX; ++tc) {
            g_qos_drv.port_qos[port].tc_map[tc]    = tc / 32;
            g_qos_drv.port_qos[port].tc_remark[tc] = tc;
            g_qos_drv.port_qos[port].tc_xlat[tc]   = tc / 32;
        }

    }

    if (CA_OSAL_OK != ca_spin_lock_init(&g_qos_drv.qos_lock, SPINLOCK_BH)) {
        ca_printf("init spinlock fail.\n");
        return CA_E_RESOURCE;
    }

    return CA_E_OK;

}

/* @__qos_exit
 * release qos initialized resources
 * NOTE: only for ca_qos_init invoking
 */
static void __qos_exit(CA_IN ca_device_id_t device_id)
{
    if (g_qos_drv.qos_lock)
        ca_spin_lock_destroy(g_qos_drv.qos_lock);

    memset(&g_qos_drv, 0, sizeof(g_qos_drv));

}

#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
/**__pon_mode_qos_init
 *@brief: init the QoS configurations for PON products, normally for PON ports
 *@note:  for G3HGU, init the SATURN-CORE QoS here
 */
static ca_status_t  __pon_mode_qos_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t temp_idx = 0;
    ca_port_id_t  port = 0;
    aal_l2_qos_dscp_map_mask_t mask_dscp_map;
    aal_l2_qos_dscp_map_t dscp_map;
    aal_l2_qos_tc_map_mask_t mask_tc_map;
    aal_l2_qos_tc_map_t tc_map;
    aal_l2_qos_dot1p_map_mask_t mask_dot1p_map;
    aal_l2_qos_dot1p_map_t dot1p_map;
    aal_l2_qos_txdscp_map_mask_t txdscp_map_mask;
    aal_l2_qos_txdscp_map_t txdscp_map;
    aal_l2_qos_txdot1p_map_mask_t txdot1p_map_mask;
    aal_l2_qos_txdot1p_map_t txdot1p_map;
    aal_l2_qos_txtc_map_mask_t txtc_map_mask;
    aal_l2_qos_txtc_map_t txtc_map;
    aal_ilpb_cfg_msk_t ipb_msk;
    aal_ilpb_cfg_t ipb_cfg;
    ca_uint8_t report_mode = 0, pon_mode = 0;

    device_id = CA_DEV_WAN;

    ret = scfg_read(0, CFG_ID_PON_REPORT_MODE, sizeof(ca_uint8_t), &report_mode);
    if (ret != CA_E_OK) {
        ca_printf("%s(): CFG_ID_PON_REPORT_MODE get failed. ret = %d\r\n", __func__, ret);
        g_qos_drv.report_mode = 0;
    } else {
        g_qos_drv.report_mode = report_mode;
        //ca_printf("ca-ne: QOS read CFG_ID_PON_REPORT_MODE = %d\r\n", g_qos_drv.report_mode);
    }
    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
        ca_printf("%s(): CFG_ID_PON_MAC_MODE get failed. ret = %d\r\n", __func__, ret);
        g_qos_drv.report_mode = ONU_PON_MAC_EPON_BI10G;
    } else {
        g_qos_drv.pon_mode = pon_mode;
        //ca_printf("ca-ne: QOS read CFG_ID_PON_MAC_MODE = %d\r\n", g_qos_drv.pon_mode);
    }

    if (!CA_PON_MODE_IS_EPON_FAMILY(g_qos_drv.pon_mode) || g_qos_drv.report_mode < 2) {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* Set WAN Core's Qos tables to be the same as LAN Core's */
        memset(&ipb_msk, 0, sizeof(ipb_msk));
        memset(&ipb_cfg, 0, sizeof(aal_l2_qos_port_config_t));
        ipb_msk.s.default_cos = 1;
        ipb_msk.s.cos_control_bm = 1;
        ipb_msk.s.cos_mode_sel = 1;

        ipb_cfg.cos_mode_sel        = AAL_COS_MODE_MAP_RX_TOP_DOT1P;
        ipb_cfg.top_802_1p_mode_sel = AAL_DOT1P_MODE_MAP_RX_DOT1P;
        ipb_cfg.dscp_mode_sel       = AAL_DSCP_MODE_MAP_RX_DSCP;

        ipb_msk.s.cos_mode_sel        = 1;
        ipb_msk.s.top_802_1p_mode_sel = 1;
        ipb_msk.s.dscp_mode_sel       = 1;

        ret = aal_l2_qos_init(CA_DEV_WAN);
        if (CA_E_OK != ret) {
            ca_printf("error: aal_l2_qos_init for DEV_WAN FAIL, return %d.\n", ret);
        }

        /* Global port qos mode */
        __FOR_ALL_QOS_PORTS_DO(port) {
            ret = aal_port_ilpb_cfg_set(CA_DEV_WAN, port, ipb_msk, &ipb_cfg);
            if (CA_E_OK != ret) {
                ca_printf("error: Init ILPB %u QoS mode FAIL, return %d.\n", port, ret);
            }
        }
#endif
        return CA_E_OK;
    }

    /* Init special QoS configuration for EPON 1Q mode, only support on COS:7 
     * So all the COS should be initialized as 7,
     * and only Classifier based Qos support to be configured
     */
    ca_printf("ca-ne: QOS init for 1Q report mode\r\n");

    for (port = CA_PORT_TYPE_ETHERNET * CA_PORT_MAX_NUM; port < CA_PORT_MAX_NUM * __PORT_TYPE_NUM; ++port) {

        for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DOT1P_MAX; ++temp_idx) {
            g_qos_drv.port_qos[port].dot1p_map[temp_idx]    = AAL_L2_QOS_COS_MAX;
        }

        for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DSCP_MAX; ++temp_idx) {
            g_qos_drv.port_qos[port].dscp_map[temp_idx]    = AAL_L2_QOS_COS_MAX;
        }

        for (temp_idx = 0; temp_idx <= AAL_L2_QOS_TC_MAX; ++temp_idx) {
            g_qos_drv.port_qos[port].tc_map[temp_idx]    = AAL_L2_QOS_COS_MAX;
        }
        g_qos_drv.port_qos[port].qos_cfg.qos_map_mode = CA_QOS_MAP_CLASSIFIER;
    }
    
    for (port = AAL_PORT_NUM_FIRST_ETH; port <= AAL_LPORT_CPU_7; ++port) {/* all UNI ports and DSP, CPU ports */
        memset(&ipb_msk, 0, sizeof(ipb_msk));
        ipb_msk.s.default_cos = 1;
        ipb_msk.s.cos_control_bm = 1;
        ipb_msk.s.cos_mode_sel = 1;
        aal_port_ilpb_cfg_get(device_id, port, &ipb_cfg);
        ipb_cfg.default_cos = 7; /* ASIC only Queue7 can work under 1Q mode */
        ipb_cfg.cos_mode_sel = AAL_COS_MODE_COS_SEL_BMP;
        ipb_cfg.cos_control_bm = 0x6; /* all check, CLS is highest pri */
        ret = aal_port_ilpb_cfg_set(device_id, port, ipb_msk, &ipb_cfg);
    }

    /* Init all the QoS tables to be zeros */
    mask_dscp_map.u32 = 0;
    mask_dscp_map.s.dscp_map_mapped_cos = 1;
    memset(&dscp_map, 0, sizeof(dscp_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DSCP_MAX; ++temp_idx) {
        dscp_map.dscp_map_mapped_cos  = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_dscp_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_dscp_map, &dscp_map);
    }

    mask_tc_map.u32 = 0;
    mask_tc_map.s.tc_map_mapped_cos = 1;
    memset(&tc_map, 0, sizeof(tc_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_TC_MAX; ++temp_idx) {
        tc_map.tc_map_mapped_cos  = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_tc_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_tc_map, &tc_map);
    }

    mask_dot1p_map.u32 = 0;
    mask_dot1p_map.s.dot1p_map_mapped_cos = 1;
    memset(&dot1p_map, 0, sizeof(dot1p_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DOT1P_MAX; ++temp_idx) {

        dot1p_map.dot1p_map_mapped_cos = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_dot1p_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_dot1p_map, &dot1p_map);
    }

    txdscp_map_mask.u32 = 0;
    txdscp_map_mask.s.txdscp_map_mapped_cos = 1;
    memset(&txdscp_map, 0, sizeof(txdscp_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DSCP_MAX; ++temp_idx) {

        txdscp_map.txdscp_map_mapped_cos = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_txdscp_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txdscp_map_mask, &txdscp_map);

    }

    txdot1p_map_mask.u32 = 0;
    txdot1p_map_mask.s.txdot1p_map_mapped_cos = 1;
    memset(&txdot1p_map, 0, sizeof(txdot1p_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DOT1P_MAX; ++temp_idx) {

        txdot1p_map.txdot1p_map_mapped_cos = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_txdot1p_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txdot1p_map_mask, &txdot1p_map);
    }

    txtc_map_mask.u32 = 0;
    txtc_map_mask.s.txtc_map_mapped_cos = 1;
    memset(&txtc_map, 0, sizeof(txtc_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_TC_MAX; ++temp_idx) {

        txtc_map.txtc_map_mapped_cos = AAL_L2_QOS_COS_MAX;
        aal_l2_qos_txtc_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txtc_map_mask, &txtc_map);
    }

#ifdef QOS_SUPPORT_L3
    do {
        ca_uint8_t dot1p = 0, dscp = 0;
        ca_uint8_t profile_idx = 0;
        aal_l3_qos_dot1p_map_mask_t    l3_dot1p_mask;
        aal_l3_qos_dot1p_map_cfg_t     l3_dot1p_config;
        aal_l3_qos_dscp_map_cfg_mask_t l3_dscp_mask;
        aal_l3_qos_dscp_map_cfg_t      l3_dscp_config;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    device_id = CA_DEV_WAN;
#endif
        l3_dot1p_mask.u32 = 0;
        l3_dot1p_mask.s.cos = 1;
        memset(&l3_dot1p_config, 0, sizeof(l3_dot1p_config));

        for (dot1p = 0; dot1p <= AAL_L3_QOS_DOT1P_MAX; dot1p++) {
            for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

                l3_dot1p_config.cos = AAL_L3_QOS_COS_MAX;
                ret = aal_l3_qos_dot1p_map_set(device_id, dot1p, profile_idx, l3_dot1p_mask, &l3_dot1p_config);
                if (CA_E_OK != ret) {
                    ca_printf("aal_l3_qos_dot1p_map_set %u profile %u :set fail %x\n", dot1p, profile_idx, ret);
                    continue;
                }
            }
        }

        l3_dscp_mask.u32 = 0;
        l3_dscp_mask.s.cos   = 1;
        memset(&l3_dscp_config, 0, sizeof(l3_dscp_config));

        for (dscp = 0; dscp <= AAL_L3_QOS_DSCP_MAX; dscp++) {
            for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

                l3_dscp_config.cos   = AAL_L3_QOS_COS_MAX;
                ret = aal_l3_qos_dscp_map_set(device_id, dscp, profile_idx, l3_dscp_mask, &l3_dscp_config);
                if (CA_E_OK != ret) {
                    ca_printf("aal_l3_qos_dscp_map_set%u profile %u :set fail %x\n", dscp, profile_idx, ret);
                    continue;
                }
            }
        }

    }while(0);
#endif

    return CA_E_OK;
}

#endif

/* @ca_qos_init
 * Init QoS driver and related API resource
 * NOTE: only for ca_init invoking
 */
ca_status_t ca_qos_init(CA_IN ca_device_id_t device_id,
            CA_IN ca_init_config_t * p_cfg)
{
    if (g_qos_init_done) {
        ca_printf("QoS was already init done.\n");
        return CA_E_OK;
    }

    if (CA_E_OK != __qos_global_init(device_id)) {
        ca_printf("__qos_global_init fail.\n");
        return CA_E_ERROR;
    }

    if (CA_E_OK != aal_l2_qos_init(device_id)) {
        ca_printf("aal_l2_qos_init fail.\n");
        __qos_exit(device_id);
        return CA_E_ERROR;
    }

#ifdef QOS_SUPPORT_L3
    if (CA_E_OK != aal_l3_qos_init(device_id)) {
        ca_printf("aal_l3_qos_init fail.\n");
        __qos_exit(device_id);
        return CA_E_ERROR;
    }
#endif

    if (CA_E_OK != __qos_mode_init(device_id)) {
        ca_printf("__qos_mode_init fail.\n");
        __qos_exit(device_id);
        return CA_E_ERROR;
    }

#ifdef QOS_SUPPORT_PON
    if (CA_E_OK !=  __pon_mode_qos_init(device_id)) {
        ca_printf("__pon_mode_qos_init fail.\n");
        __qos_exit(device_id);
        return CA_E_ERROR;
    }
#endif

    g_qos_init_done = TRUE;
    ca_printf("ca-ne: init QoS OK\r\n");

    return CA_E_OK;
}

/* @ca_qos_exit
 * Unload QoS driver and free related API resource
 */
ca_status_t ca_qos_exit(CA_IN ca_device_id_t device_id)
{
    __qos_exit(device_id);
    g_qos_init_done = FALSE;
    ca_printf("ca-ne: exit QoS OK\r\n");

    return CA_E_OK;
}

/* Debug dump for QoS configurations */
void ca_qos_dump(void)
{
    ca_status_t    ret = CA_E_OK;
    ca_device_id_t dev  = 0;
    ca_port_id_t   port = 0;
    ca_qos_config_t qos_config;
    const char *qos_str[] = {"Dot1P", "DscpTc", "ClS"};
    const char *dot1p_str[] = {"Dot1P-Map", "DscpTc-Map", "CLS-Map"};
    const char *dscp_str[] = {"Default", "DscpTc-Map", "CLS-Map"};


    memset(&qos_config, 0, sizeof(qos_config));

    ca_printf("\r\n");
    ca_printf("Port   CosMode     Dot1pMode    DscpMode\r\n");
    ca_printf("--------------------------------------------\r\n");

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    for (port = CA_PORT_ID_NI3; port <= CA_PORT_ID_NI6; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }
    for (port = CA_PORT_ID_NI7; port <= AAL_LPORT_ETH_WAN; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_GPON, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }

    for (port = CA_PORT_ID_CPU0; port <= CA_PORT_ID_CPU3; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_CPU, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    for (port = CA_PORT_ID_NI0; port <= CA_PORT_ID_NI6; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }
    for (port = CA_PORT_ID_NI7; port <= AAL_LPORT_ETH_WAN; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_GPON, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }

    for (port = CA_PORT_ID_CPU0; port <= CA_PORT_ID_CPU7; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_CPU, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }
#else
    for (port = CA_PORT_ID_NI0; port <= CA_PORT_ID_NI7; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }

    for (port = CA_PORT_ID_CPU0; port <= CA_PORT_ID_CPU7; port++) {
        ret = ca_qos_config_get(dev, CA_PORT_ID(CA_PORT_TYPE_CPU, port), &qos_config);
        if (CA_E_OK != ret)
            continue;
        ca_printf("%-4u   %-9s   %-10s   %-9s \r\n", port, qos_str[qos_config.qos_map_mode], dot1p_str[qos_config.dot1p_remap_mode], dscp_str[qos_config.dscp_remap_mode]);
    }
#endif

    ca_printf("\r\n");
}

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_ARCH_RTL8198F)
unsigned int gQosEnabled = 0;
extern int rtk_backup_l3fe_flow_control(void);
extern int rtk_restore_l3fe_flow_control(void);
extern int rtk_set_l3fe_flow_control(unsigned int lan_en, unsigned int wan_en);

ca_status_t rtk_qos_enable(unsigned int enable)
{
	ca_status_t ret;
		
	if(enable){
		if((ret = rtk_backup_l3fe_flow_control()) != 0){
			ca_printf("%s, 0x%x\n", "rtk_backup_l3fe_flow_control error", ret);
			return ret;
		}

		// disable l3fe flow control to avoid buff full drop (expect tail drop) to make qos schedule well
		if((ret = rtk_set_l3fe_flow_control(0, 0)) != 0){
			ca_printf("%s, 0x%x\n", "rtk_set_l3fe_flow_control error", ret);
			return ret;
		}
	}
	else{
		if((ret = rtk_restore_l3fe_flow_control()) != 0){
			ca_printf("%s, 0x%x\n", "rtk_restore_l3fe_flow_control error", ret);
			return ret;
		}
	}

	gQosEnabled = enable>0 ? 1 : 0;	
	return ret;
}
#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(ca_qos_init);
EXPORT_SYMBOL(ca_qos_exit);
EXPORT_SYMBOL(ca_qos_tc_remark_get);
EXPORT_SYMBOL(ca_qos_dscp_dot1p_translate_get);
EXPORT_SYMBOL(ca_qos_tc_dot1p_translate_set);
EXPORT_SYMBOL(ca_qos_config_get);
EXPORT_SYMBOL(ca_qos_tc_remark_set);
EXPORT_SYMBOL(ca_qos_tc_dot1p_translate_get);
EXPORT_SYMBOL(ca_qos_tc_map_get);
EXPORT_SYMBOL(ca_qos_tc_map_set);
EXPORT_SYMBOL(ca_qos_tc_map_get_ex);
EXPORT_SYMBOL(ca_qos_tc_map_set_ex);
EXPORT_SYMBOL(ca_qos_dot1p_remark_set);
EXPORT_SYMBOL(ca_qos_dscp_dot1p_translate_set);
EXPORT_SYMBOL(ca_qos_config_set);
EXPORT_SYMBOL(ca_qos_dscp_remark_set);
EXPORT_SYMBOL(ca_qos_dscp_remark_get);
EXPORT_SYMBOL(ca_qos_dot1p_remark_get);
EXPORT_SYMBOL(ca_qos_dscp_map_set);
EXPORT_SYMBOL(ca_qos_dscp_map_get);
EXPORT_SYMBOL(ca_qos_dscp_map_set_ex);
EXPORT_SYMBOL(ca_qos_dscp_map_get_ex);
EXPORT_SYMBOL(ca_qos_dot1p_map_get);
EXPORT_SYMBOL(ca_qos_dot1p_map_set);
EXPORT_SYMBOL(ca_qos_dot1p_map_get_ex);
EXPORT_SYMBOL(ca_qos_dot1p_map_set_ex);
EXPORT_SYMBOL(ca_qos_dump);
#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_ARCH_RTL8198F)
EXPORT_SYMBOL(gQosEnabled);
EXPORT_SYMBOL(rtk_qos_enable);
#endif
#endif

