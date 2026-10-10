#include "osal_cmn.h"
#include "aal_common.h"
#include "aal_port.h"
#include "port.h"
#include "rate.h"
#include "aal_l2_tm.h"
#include "aal_l3_tm.h"
#include "aal_l2_te.h"
#include "aal_l3_te.h"
#include "aal_arb.h"
#include "ca-scfg/scfg.h"


/*
** incoming traffic from WAN port flow is as follows,
** WAN -> NI_0 -> NI_7 -> L3FE(LSPID 18) -> NI -> L2FE(LSPID 18, PSPID 10)
** -> L2TM(LSPID 18, PSPID 10) -> NI
** therefore we need to configure entry 10 for L2 policer engine, which is from
** the definition of register NI_HV_GLB_RXMUX_CTRL_CFG.l3fe_pspid (default 10)
*/
#define NI_RXMUX_L3FE_PSPID 10


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
ca_boolean_t g_l3fe_to_wan_flag = CA_FALSE;
#endif

ca_status_t ca_policer_config_set
(
    CA_IN ca_device_id_t      device_id,
    CA_IN ca_policer_config_t *config
)
{
    aal_l2_te_policer_cfg_t     cfg;
    aal_l2_te_policer_cfg_msk_t msk;
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l2_te_glb_cfg_t     l2_te_cfg;
    aal_l2_te_glb_cfg_msk_t l2_te_msk;
    aal_l3_te_glb_cfg_t     l3_te_cfg;
    aal_l3_te_glb_cfg_msk_t l3_te_msk;
    aal_arb_ctrl_t      arb_ctrl;
    aal_arb_ctrl_mask_t arb_msk;
    ca_uint32_t i = 0;

    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&msk, 0, sizeof(msk));

    cfg.ipg   = config->overhead;
    msk.s.ipg = 1;

    for (i=0; i<=(ca_uint32_t)CA_AAL_POLICER_PKT_TYPE_UBC; i++)
    {
        if (aal_l2_te_policer_pkt_type_cfg_set(device_id, (aal_l2_te_policer_pkt_type_t)i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for packet type %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
    {
        if (aal_l2_te_policer_port_cfg_set(device_id, i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for port %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_MAX_FLOW_ID; i++)
    {
        if (aal_l2_te_policer_flow_cfg_set(device_id, i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for flow %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    /* L3 */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));

    l3_cfg.ipg   = config->overhead;
    l3_msk.s.ipg = 1;
    for (i=0; i<=(ca_uint32_t)CA_AAL_L3_POLICER_PKT_TYPE_UBC; i++)
    {
        if (aal_l3_te_policer_pkt_type_cfg_set(device_id, (aal_l3_te_policer_pkt_type_t)i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for packet type %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_L3_MAX_PORT_ID; i++)
    {
        if (aal_l3_te_policer_port_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for port %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_L3_MAX_FLOW_ID; i++)
    {
        if (aal_l3_te_policer_flow_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_policer_config_set() : error - failed to set ipg for flow %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    /* ECN drop */
    memset((void *)&l2_te_cfg, 0, sizeof(l2_te_cfg));
    memset((void *)&l3_te_cfg, 0, sizeof(l3_te_cfg));

    l2_te_msk.u32 = 0;
    l3_te_msk.u32 = 0;

    l2_te_cfg.pol_ecn_ena  = config->drop_ecn ? 0 : 1;
    l2_te_cfg.mark_ecn_ena = config->mark_ecn ? 1 : 0;
    l2_te_msk.s.pol_ecn_ena  = 1;
    l2_te_msk.s.mark_ecn_ena = 1;
    (void)aal_l2_te_glb_cfg_set(device_id, l2_te_msk, &l2_te_cfg);

    l3_te_cfg.pol_ecn_ena  = config->drop_ecn ? 0 : 1;
    l3_te_cfg.mark_ecn_ena = config->mark_ecn ? 1 : 0;
    l3_te_msk.s.pol_ecn_ena  = 1;
    l3_te_msk.s.mark_ecn_ena = 1;
    (void)aal_l3_te_glb_cfg_set(device_id, l3_te_msk, &l3_te_cfg);

    memset((void *)&arb_ctrl, 0, sizeof(arb_ctrl));
    arb_msk.u32 = 0;

    arb_msk.s.ip_ecn_en = 1;
    if (config->mark_ecn || (config->drop_ecn == 0))
        arb_ctrl.ip_ecn_en  = 1;
    else
        arb_ctrl.ip_ecn_en  = 0;

    (void)aal_arb_ctrl_set(device_id, arb_msk, &arb_ctrl);

    return CA_E_OK;
}

ca_status_t ca_policer_config_get
(
    CA_IN  ca_device_id_t      device_id,
    CA_OUT ca_policer_config_t *config
)
{
    aal_l2_te_policer_cfg_t cfg;
    aal_l2_te_glb_cfg_t     l2_te_cfg;

    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&l2_te_cfg, 0, sizeof(l2_te_cfg));

    if (!config)
        return CA_E_PARAM;

    /* as ipg configurations for ports, flows, flow groups, and pkt type are totally same, we just get any of them */
    if (aal_l2_te_policer_port_cfg_get(device_id, 0, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_policer_config_get() : error - failed to get ipg for port %d, L%d\n", 0, __LINE__);
        return CA_E_ERROR;
    }

    config->overhead = cfg.ipg;

    if (aal_l2_te_glb_cfg_get(device_id, &l2_te_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_policer_config_get() : error - failed to get TE global config for port %d, L%d\n", 0, __LINE__);
        return CA_E_ERROR;
    }

    config->drop_ecn = !l2_te_cfg.pol_ecn_ena;
    config->mark_ecn = l2_te_cfg.mark_ecn_ena;

    return CA_E_OK;
}

ca_status_t ca_port_policer_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_policer_t    *policer
)
{
    aal_l2_te_policer_cfg_t cfg;
    aal_l2_te_policer_cfg_msk_t msk;
    aal_l2_te_policer_profile_t profile;
    aal_l2_te_policer_profile_msk_t profile_msk;
    aal_l3_te_policer_cfg_t l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l3_te_policer_profile_t l3_profile;
    aal_l3_te_policer_profile_msk_t l3_profile_msk;
    ca_port_id_t port = 0;
    ca_uint32_t  port_type = CA_PORT_TYPE_INVALID;

    if (!policer)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else
        {
            return CA_E_NOT_SUPPORT;
        }
    #else
        if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
            if (port != CA_PORT_ID_NI0)
                return CA_E_PARAM;
        #else
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        #endif
        }
        else
        {
            return CA_E_NOT_SUPPORT;
        }
    #endif

    /* L3 only supports LSPID 0x0-0x7, l3 stream has LSPID 0x18 or 0x19 and therefore L3 TM cannot support WAN port */

    if (port_type == CA_PORT_TYPE_L3)   /* WAN */
    {
        /* need to configure L3 engine for WAN port */

        memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
        memset((void *)&l3_msk, 0, sizeof(l3_msk));
        memset((void *)&l3_profile, 0, sizeof(l3_profile));
        memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

        l3_msk.s.type = 1;
        l3_msk.s.pol_update_mode = 1;

        l3_cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

        switch (policer->mode)
        {
        case CA_POLICER_MODE_DISABLE:
            l3_cfg.type = CA_AAL_L3_POLICER_TYPE_NONE;
            break;
        case CA_POLICER_MODE_SRTCM:
            l3_cfg.type = CA_AAL_L3_POLICER_TYPE_RFC2697;
            break;
        case CA_POLICER_MODE_TRTCM:
            l3_cfg.type = CA_AAL_L3_POLICER_TYPE_RFC2698;
            break;
        case CA_POLICER_MODE_TRTCMDS:
            l3_cfg.type = CA_AAL_L3_POLICER_TYPE_RFC4115;
            break;
        case CA_POLICER_MODE_TRTCMDS_COUPLED:
            l3_cfg.type = CA_AAL_L3_POLICER_TYPE_RFC4115_EX;
            break;
        case CA_POLICER_MODE_RATE_ONLY:
            return CA_E_NOT_SUPPORT;
            break;
        default:
            return CA_E_NOT_SUPPORT;
            break;

        }

        /*
        ** Entry 7 is used for traffic from WAN port, which is mapped from LSPID 0x7 and 0x18.
        ** Other entires are invalid for software.
        */
        port = 7;

        if (aal_l3_te_policer_port_cfg_set(device_id, port, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to set configurations for port %d, L%d\n", port_id, __LINE__);
            return CA_E_ERROR;
        }

        l3_profile_msk.s.cir_k = 1;
        l3_profile_msk.s.cir_m = 1;
        l3_profile_msk.s.cbs   = 1;
        l3_profile_msk.s.pir_k = 1;
        l3_profile_msk.s.pir_m = 1;
        l3_profile_msk.s.pbs   = 1;

        l3_profile.cir_k = policer->cir % 1000;
        l3_profile.cir_m = policer->cir / 1000;
        l3_profile.cbs   = policer->cbs;
        l3_profile.pir_k = policer->pir % 1000;
        l3_profile.pir_m = policer->pir / 1000;
        l3_profile.pbs   = policer->pbs;

        if (aal_l3_te_policer_port_profile_set(device_id, port, l3_profile_msk, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to set policing profile for port %d, L%d\n", port_id, __LINE__);
            return CA_E_ERROR;
        }

        return CA_E_OK;
    }
    else
    {
        /* ethernet port configuration */
        memset((void *)&cfg, 0, sizeof(cfg));
        memset((void *)&msk, 0, sizeof(msk));
        memset((void *)&profile, 0, sizeof(profile));
        memset((void *)&profile_msk, 0, sizeof(profile_msk));

        msk.s.type = 1;
        msk.s.pol_update_mode = 1;

        cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

        switch (policer->mode)
        {
        case CA_POLICER_MODE_DISABLE:
            cfg.type = CA_AAL_POLICER_TYPE_NONE;
            break;
        case CA_POLICER_MODE_SRTCM:
            cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
            break;
        case CA_POLICER_MODE_TRTCM:
            cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
            break;
        case CA_POLICER_MODE_TRTCMDS:
            cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
            break;
        case CA_POLICER_MODE_TRTCMDS_COUPLED:
            cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
            break;
        case CA_POLICER_MODE_RATE_ONLY:
            return CA_E_NOT_SUPPORT;
            break;
        default:
            return CA_E_NOT_SUPPORT;
            break;

        }

// #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        /* for saturn SFH, traffic will bypass L3FE, so PSPID from WAN will keep unchanged(7). */
// #else
        if (port == CA_PORT_ID_NI7)
        {
            if (aal_l2_te_policer_port_cfg_set(device_id, port, msk, &cfg) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_policer_set() : failed to set configurations for port %d, L%d\n", port_id, __LINE__);
                return CA_E_ERROR;
            }
            port = NI_RXMUX_L3FE_PSPID;
        }
// #endif
        if (aal_l2_te_policer_port_cfg_set(device_id, port, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to set configurations for port %d, L%d\n", port_id, __LINE__);
            return CA_E_ERROR;
        }

        profile_msk.s.cir_k = 1;
        profile_msk.s.cir_m = 1;
        profile_msk.s.cbs   = 1;
        profile_msk.s.pir_k = 1;
        profile_msk.s.pir_m = 1;
        profile_msk.s.pbs   = 1;

        profile.cir_k = policer->cir % 1000;
        profile.cir_m = policer->cir / 1000;
        profile.cbs   = policer->cbs;
        profile.pir_k = policer->pir % 1000;
        profile.pir_m = policer->pir / 1000;
        profile.pbs   = policer->pbs;


        /* other L2 port */
        if (aal_l2_te_policer_port_profile_set(device_id, port, profile_msk, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to set policing profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }

        if (port == NI_RXMUX_L3FE_PSPID)
        {
            if (aal_l2_te_policer_port_profile_set(device_id, CA_PORT_ID_NI7, profile_msk, &profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_policer_set() : failed to set policing profile for port %d, L%d\n", CA_PORT_ID_NI7, __LINE__);
                return CA_E_ERROR;
            }
        }
    }

    return CA_E_OK;
}

ca_status_t ca_port_policer_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_policer_t   *policer
)
{
    aal_l2_te_policer_cfg_t     cfg;
    aal_l2_te_policer_profile_t profile;
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_profile_t l3_profile;
    ca_port_id_t port = 0;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;

    if (!policer)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);
    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else
        {
            return CA_E_NOT_SUPPORT;
        }
    #else
        if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
            if (port != CA_PORT_ID_NI0)
                return CA_E_PARAM;
        #else
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        #endif
        }
        else
        {
            return CA_E_NOT_SUPPORT;
        }
    #endif


    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));

    if (port_type == CA_PORT_TYPE_L3)   /* WAN */
    {
        port = 7;   /* entry 7 is used for traffic from WAN port */

        if (aal_l3_te_policer_port_cfg_get(device_id, port, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_get() : failed to get configurations for port %d, L%d\n", port_id, __LINE__);
            return CA_E_ERROR;
        }

        if (aal_l3_te_policer_port_profile_get(device_id, port, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to get policing profile for port %d, L%d\n", port_id, __LINE__);
            return CA_E_ERROR;
        }

        policer->cir = l3_profile.cir_m * 1000 + l3_profile.cir_k;
        policer->pir = l3_profile.pir_m * 1000 + l3_profile.pir_k;
        policer->cbs = l3_profile.cbs;
        policer->pbs = l3_profile.pbs;
        policer->pps = (l3_cfg.pol_update_mode == CA_AAL_L3_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

        switch (l3_cfg.type)
        {
        case CA_AAL_L3_POLICER_TYPE_RFC2698:
            policer->mode = CA_POLICER_MODE_TRTCM;
            break;

        case CA_AAL_L3_POLICER_TYPE_RFC2697:
            policer->mode = CA_POLICER_MODE_SRTCM;
            break;

        case CA_AAL_L3_POLICER_TYPE_RFC4115:
            policer->mode = CA_POLICER_MODE_TRTCMDS;
            break;

        case CA_AAL_L3_POLICER_TYPE_RFC4115_EX:
            policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
            break;

        case CA_AAL_L3_POLICER_TYPE_NONE:
            policer->mode = CA_POLICER_MODE_DISABLE;
            break;

        default:
            break;
        }

    }
    else
    {
// #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        /* for saturn SFH, traffic will bypass L3FE, so PSPID from WAN will keep unchanged(7). */
// #else
        if (port == CA_PORT_ID_NI7)
            port = NI_RXMUX_L3FE_PSPID;
// #endif

        if (aal_l2_te_policer_port_cfg_get(device_id, port, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_get() : failed to get configurations for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }

        if (aal_l2_te_policer_port_profile_get(device_id, port, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_policer_set() : failed to get policing profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }

        policer->cir = profile.cir_m * 1000 + profile.cir_k;
        policer->pir = profile.pir_m * 1000 + profile.pir_k;
        policer->cbs = profile.cbs;
        policer->pbs = profile.pbs;
        policer->pps = (cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

        switch (cfg.type)
        {
        case CA_AAL_POLICER_TYPE_RFC2698:
            policer->mode = CA_POLICER_MODE_TRTCM;
            break;

        case CA_AAL_POLICER_TYPE_RFC2697:
            policer->mode = CA_POLICER_MODE_SRTCM;
            break;

        case CA_AAL_POLICER_TYPE_RFC4115:
            policer->mode = CA_POLICER_MODE_TRTCMDS;
            break;

        case CA_AAL_POLICER_TYPE_RFC4115_EX:
            policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
            break;

        case CA_AAL_POLICER_TYPE_NONE:
            policer->mode = CA_POLICER_MODE_DISABLE;
            break;

        default:
            break;
        }
    }

    return CA_E_OK;
}

ca_status_t ca_port_policer_stats_get
(
    CA_IN  ca_device_id_t     device_id,
    CA_IN  ca_port_id_t       port_id,
    CA_IN  ca_boolean_t       read_clear,
    CA_OUT ca_policer_stats_t *stats
)
{
    aal_l2_te_pm_policer_t pm_data;
    aal_l3_te_pm_policer_t l3_pm_data;

    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_port_id_t port = 0;

    if (!stats)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else
        {
            return CA_E_NOT_SUPPORT;
    }
    #else
        if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
            if (port != CA_PORT_ID_NI0)
                return CA_E_PARAM;
        #else
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        #endif
        }
        else
        {
            return CA_E_NOT_SUPPORT;
        }
    #endif


    read_clear = read_clear; /* ignore read-keep flag in G3 */

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &g_l3fe_to_wan_flag) != CA_E_OK)
        ca_printf("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT.\n", __func__);
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    g_l3fe_to_wan_flag = CA_TRUE;
#endif


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (g_l3fe_to_wan_flag)
        {
            /*
            ** at this case, d/s traffic bypassed L3TM and therefore policer and related statsitics
            ** will be taken by L2TM; the SPID from L3FE is 0xa;
            */
            port = CA_AAL_L2TM_SPID_WAN_L3_L2;

            memset((void *)&pm_data, 0, sizeof(pm_data));

            if (aal_l2_te_pm_policer_port_get(device_id, port, &pm_data) != CA_E_OK)
                return CA_E_ERROR;

            stats->green_packets  = pm_data.green_pkt;
            stats->yellow_packets = pm_data.yellow_pkt;
            stats->red_packets    = pm_data.red_pkt;
            stats->green_bytes    = pm_data.green_bytes;
            stats->yellow_bytes   = pm_data.yellow_bytes;
            stats->red_bytes      = pm_data.red_bytes;
        }
        else
        {
            #if 0
            #if defined(CONFIG_ARCH_CORTINA_G3HGU)
                /*
                    for g3hgu, NI_HV_GLB_NIRX_L3FE_DEMUX_CFGxd.demux_id is slected not to L3QM,
                    so we should read l2tm data instead of l3tm data
                */

                port = CA_AAL_L2TM_SPID_WAN_L3_L2;

                memset((void *)&pm_data, 0, sizeof(pm_data));

                if (aal_l2_te_pm_policer_port_get(device_id, port, &pm_data) != CA_E_OK)
                    return CA_E_ERROR;

                stats->green_packets  = pm_data.green_pkt;
                stats->yellow_packets = pm_data.yellow_pkt;
                stats->red_packets    = pm_data.red_pkt;
            #else
                /* directly read l3 engine */
                memset((void *)&l3_pm_data, 0, sizeof(l3_pm_data));

                if (aal_l3_te_pm_policer_port_get(device_id, port, &l3_pm_data) != CA_E_OK)
                    return CA_E_ERROR;

                stats->green_packets  = l3_pm_data.green_pkt;
                stats->yellow_packets = l3_pm_data.yellow_pkt;
                stats->red_packets    = l3_pm_data.red_pkt;
            #endif
            #endif

            /* directly read l3 engine */
            memset((void *)&l3_pm_data, 0, sizeof(l3_pm_data));

            if (aal_l3_te_pm_policer_port_get(device_id, port, &l3_pm_data) != CA_E_OK)
                return CA_E_ERROR;

            stats->green_packets  = l3_pm_data.green_pkt;
            stats->yellow_packets = l3_pm_data.yellow_pkt;
            stats->red_packets    = l3_pm_data.red_pkt;
            stats->green_bytes    = l3_pm_data.green_bytes;
            stats->yellow_bytes   = l3_pm_data.yellow_bytes;
            stats->red_bytes      = l3_pm_data.red_bytes;
        }
    }
    else
    {
#endif
        /*
        ** for ethernet port type, if customer read statistics for WAN port,
        ** which means l2 traffic being transmitted, system will read l2 engine
        ** with spid 0xa.
        */
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
            if (port == CA_PORT_ID_NI0)
                port = CA_AAL_L2TM_SPID_WAN_L3_L2;
        #else
            #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                /* for saturn SFU, traffic will bypass L3FE, so PSPID from WAN will keep unchanged(7). */
            #else
                if (port == CA_PORT_ID_NI7)
                    port = CA_AAL_L2TM_SPID_WAN_L3_L2;
            #endif
        #endif


        /* ethernet port : read l2 engine */
        memset((void *)&pm_data, 0, sizeof(pm_data));

        if (aal_l2_te_pm_policer_port_get(device_id, port, &pm_data) != CA_E_OK)
            return CA_E_ERROR;

        stats->green_packets  = pm_data.green_pkt;
        stats->yellow_packets = pm_data.yellow_pkt;
        stats->red_packets    = pm_data.red_pkt;
        stats->green_bytes    = pm_data.green_bytes;
        stats->yellow_bytes   = pm_data.yellow_bytes;
        stats->red_bytes      = pm_data.red_bytes;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    }
#endif

    return CA_E_OK;
}

ca_status_t ca_port_policer_stats_clear
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id
)
{
    ca_policer_stats_t stats;
    ca_port_id_t port = 0;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_NOT_SUPPORT;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3 || port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
#ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
#else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
#endif
    }
    else
    {
        return CA_E_NOT_SUPPORT;
    }
#endif


    memset(&stats, 0x0, sizeof(stats));

    (void)ca_port_policer_stats_get(device_id, port_id, FALSE, &stats);

    return CA_E_OK;
}

ca_status_t ca_flow_policer_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    flow_id,
    CA_IN ca_policer_t   *policer
)
{
    aal_l2_te_policer_cfg_t cfg;
    aal_l2_te_policer_cfg_msk_t msk;
    aal_l2_te_policer_profile_t profile;
    aal_l2_te_policer_profile_msk_t profile_msk;
    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_l3_te_policer_cfg_t l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l3_te_policer_profile_t l3_profile;
    aal_l3_te_policer_profile_msk_t l3_profile_msk;
    #endif
    //ca_boolean_t l3_flag = FALSE;

    if (!policer)
        return CA_E_PARAM;

    flow_id &= 0xff;    /* low 8 bits are valid, bug 50323 */

    /* flow id range will be lesser than the minimum value between L2 and L3 flow ID */
    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
    #else
    if (flow_id > CA_AAL_MAX_FLOW_ID || flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;
    #endif

    /* l2 init */
    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&msk, 0, sizeof(msk));
    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&profile_msk, 0, sizeof(profile_msk));

    /* l2 flow engine */
    msk.s.type = 1;
    msk.s.pol_update_mode = 1;

    cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* HGU consists of G3 and Saturn chip, so we need to configure both NEs. Refer to bugt 50679 */

    /* configure G3 NE of which device id is 0 */
    if (aal_l2_te_policer_flow_cfg_set(0, flow_id, msk, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for G3 l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    /* configure Saturn NE of which device id is 1 */
    if (aal_l2_te_policer_flow_cfg_set(1, flow_id, msk, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for Saturn l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
#else
    if (aal_l2_te_policer_flow_cfg_set(device_id, flow_id, msk, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
#endif

    profile_msk.s.cir_k = 1;
    profile_msk.s.cir_m = 1;
    profile_msk.s.cbs   = 1;
    profile_msk.s.pir_k = 1;
    profile_msk.s.pir_m = 1;
    profile_msk.s.pbs   = 1;
    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        profile.cir_k = CA_AAL_L2_TE_POL_MAX_RATE_K;
        profile.cir_m = CA_AAL_L2_TE_POL_MAX_RATE_M;
        profile.cbs   = CA_AAL_L2_TE_POL_MAX_CBS;
        profile.pir_k = CA_AAL_L2_TE_POL_MAX_RATE_K;
        profile.pir_m = CA_AAL_L2_TE_POL_MAX_RATE_M;
        profile.pbs   = CA_AAL_L2_TE_POL_MAX_PBS;
    }
    else
    {
        profile.cir_k = policer->cir % 500;
        profile.cir_m = policer->cir / 500;
        profile.cbs   = policer->cbs;
        profile.pir_k = policer->pir % 500;
        profile.pir_m = policer->pir / 500;
        profile.pbs   = policer->pbs;
    }

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* HGU consists of G3 and Saturn chip, so we need to configure both NEs. Refer to bugt 50679 */

    /* configure G3 NE of which device id is 0 */
    if (aal_l2_te_policer_flow_profile_set(0, flow_id, profile_msk, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for G3 l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    if (aal_l2_te_policer_flow_profile_set(1, flow_id, profile_msk, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for Saturn l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
#else
    if (aal_l2_te_policer_flow_profile_set(device_id, flow_id, profile_msk, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
#endif

    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    /* l3 init */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    /* l3 flow engine */
    l3_msk.s.type = 1;
    l3_msk.s.pol_update_mode = 1;

    l3_cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        l3_cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

    if (aal_l3_te_policer_flow_cfg_set(device_id, flow_id, l3_msk, &l3_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l3 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    l3_profile_msk.s.cir_k = 1;
    l3_profile_msk.s.cir_m = 1;
    l3_profile_msk.s.cbs   = 1;
    l3_profile_msk.s.pir_k = 1;
    l3_profile_msk.s.pir_m = 1;
    l3_profile_msk.s.pbs   = 1;

    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        l3_profile.cir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.cir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.cbs   = CA_AAL_L3_TE_POL_MAX_CBS;
        l3_profile.pir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.pir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.pbs   = CA_AAL_L3_TE_POL_MAX_PBS;
    }
    else
    {
        l3_profile.cir_k = policer->cir % 500;
        l3_profile.cir_m = policer->cir / 500;
        l3_profile.cbs   = policer->cbs;
        l3_profile.pir_k = policer->pir % 500;
        l3_profile.pir_m = policer->pir / 500;
        l3_profile.pbs   = policer->pbs;
    }


    if (aal_l3_te_policer_flow_profile_set(device_id, flow_id, l3_profile_msk, &l3_profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l3 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
    #endif

    return CA_E_OK;
}

ca_status_t ca_flow_policer_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint16_t    flow_id,
    CA_OUT ca_policer_t   *policer
)
{
    aal_l2_te_policer_cfg_t     cfg;
    aal_l2_te_policer_profile_t profile;
    //aal_l3_te_policer_cfg_t     l3_cfg;
    //aal_l3_te_policer_profile_t l3_profile;
    //ca_boolean_t l3_flag = FALSE;

    if (!policer)
      return CA_E_PARAM;

    flow_id &= 0xff;    /* low 8 bits are valid, bug 50323 */

    /* flow id range will be lesser than the minimum value between L2 and L3 flow ID */
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
#else
    if (flow_id > CA_AAL_MAX_FLOW_ID || flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;
#endif

    /* as each flow will be configured into both l2 and l3 engine, reading configuration from either engine is enough */
    memset((void *)&cfg,        0, sizeof(cfg));
    memset((void *)&profile,    0, sizeof(profile));

    if (aal_l2_te_policer_flow_cfg_get(device_id, flow_id, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_get() : failed to get configurations for flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    if (aal_l2_te_policer_flow_profile_get(device_id, flow_id, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_get() : failed to get policing profile for flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    policer->cir = profile.cir_m * 500 + profile.cir_k;
    policer->pir = profile.pir_m * 500 + profile.pir_k;
    policer->cbs = profile.cbs;
    policer->pbs = profile.pbs;
    policer->pps = (cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

    switch (cfg.type)
    {
    case CA_AAL_POLICER_TYPE_NONE:
       policer->mode = CA_POLICER_MODE_DISABLE;
       break;

    case CA_AAL_POLICER_TYPE_RFC2698:
     policer->mode = CA_POLICER_MODE_TRTCM;
     break;

    case CA_AAL_POLICER_TYPE_RFC2697:
     policer->mode = CA_POLICER_MODE_SRTCM;
     break;

    case CA_AAL_POLICER_TYPE_RFC4115:
     policer->mode = CA_POLICER_MODE_TRTCMDS;
     break;

    case CA_AAL_POLICER_TYPE_RFC4115_EX:
     policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
     break;

    default:
     break;
    }

    return CA_E_OK;
}

ca_status_t ca_group_policer_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    group_id,
    CA_IN ca_policer_t   *policer
)
{
    return CA_E_OK;
}

ca_status_t ca_group_policer_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint16_t    group_id,
    CA_OUT ca_policer_t   *policer,
    CA_OUT ca_uint16_t    *members,
    CA_OUT ca_uint16_t    *member_num
)
{
    return CA_E_OK;
}

ca_status_t ca_group_member_add
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    group_id,
    CA_IN ca_uint16_t    flow_id
)
{
    return CA_E_OK;
}

ca_status_t ca_group_member_delete
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    group_id,
    CA_IN ca_uint16_t    flow_id
)
{
    return CA_E_OK;
}

ca_status_t ca_group_member_delete_all
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    group_id
)
{
    return CA_E_OK;
}

ca_status_t ca_flow_policer_stats_get
(
    CA_IN  ca_device_id_t     device_id,
    CA_IN  ca_uint16_t        flow_id,
    CA_IN  ca_boolean_t       read_clear,
    CA_OUT ca_policer_stats_t *stats
)
{
    aal_l2_te_pm_policer_t pm_data;
    aal_l3_te_pm_policer_t l3_pm_data;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_policer_stats_t tmp_stats;
#endif
    if (!stats)
        return CA_E_PARAM;

    /* flow id range will be lesser than the minimum value between L2 and L3 flow ID */
    if (flow_id > CA_AAL_MAX_FLOW_ID || flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    read_clear = read_clear;  /* ignore read-keep flag in G3 */

    memset((void *)&pm_data, 0, sizeof(pm_data));
    memset((void *)&l3_pm_data, 0, sizeof(l3_pm_data));

    if (aal_l2_te_pm_policer_flow_get(device_id, flow_id, &pm_data) != CA_E_OK)
        return CA_E_ERROR;

    if (aal_l3_te_pm_policer_flow_get(device_id, flow_id, &l3_pm_data) != CA_E_OK)
        return CA_E_ERROR;

    stats->green_packets  = (pm_data.green_pkt  > l3_pm_data.green_pkt)  ? pm_data.green_pkt  : l3_pm_data.green_pkt;
    stats->yellow_packets = (pm_data.yellow_pkt > l3_pm_data.yellow_pkt) ? pm_data.yellow_pkt : l3_pm_data.yellow_pkt;
    stats->red_packets    = (pm_data.red_pkt    > l3_pm_data.red_pkt)    ? pm_data.red_pkt    : l3_pm_data.red_pkt;

    stats->green_bytes  = (pm_data.green_bytes  > l3_pm_data.green_bytes)  ? pm_data.green_bytes  : l3_pm_data.green_bytes;
    stats->yellow_bytes = (pm_data.yellow_bytes > l3_pm_data.yellow_bytes) ? pm_data.yellow_bytes : l3_pm_data.yellow_bytes;
    stats->red_bytes    = (pm_data.red_bytes    > l3_pm_data.red_bytes)    ? pm_data.red_bytes    : l3_pm_data.red_bytes;


#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* read statistics on Saturn */
    memset((void *)&pm_data, 0, sizeof(pm_data));
    memset((void *)&l3_pm_data, 0, sizeof(l3_pm_data));
    memset((void *)&tmp_stats, 0, sizeof(tmp_stats));

    if (aal_l2_te_pm_policer_flow_get(1, flow_id, &pm_data) != CA_E_OK)
        return CA_E_ERROR;

    if (aal_l3_te_pm_policer_flow_get(1, flow_id, &l3_pm_data) != CA_E_OK)
        return CA_E_ERROR;

    tmp_stats.green_packets  = (pm_data.green_pkt  > l3_pm_data.green_pkt)  ? pm_data.green_pkt  : l3_pm_data.green_pkt;
    tmp_stats.yellow_packets = (pm_data.yellow_pkt > l3_pm_data.yellow_pkt) ? pm_data.yellow_pkt : l3_pm_data.yellow_pkt;
    tmp_stats.red_packets    = (pm_data.red_pkt    > l3_pm_data.red_pkt)    ? pm_data.red_pkt    : l3_pm_data.red_pkt;
    tmp_stats.green_bytes    = (pm_data.green_bytes  > l3_pm_data.green_bytes)  ? pm_data.green_bytes  : l3_pm_data.green_bytes;
    tmp_stats.yellow_bytes   = (pm_data.yellow_bytes > l3_pm_data.yellow_bytes) ? pm_data.yellow_bytes : l3_pm_data.yellow_bytes;
    tmp_stats.red_bytes      = (pm_data.red_bytes    > l3_pm_data.red_bytes)    ? pm_data.red_bytes    : l3_pm_data.red_bytes;

    stats->green_packets  += tmp_stats.green_packets;
    stats->yellow_packets += tmp_stats.yellow_packets;
    stats->red_packets    += tmp_stats.red_packets;
    stats->green_bytes    += tmp_stats.green_bytes;
    stats->yellow_bytes   += tmp_stats.yellow_bytes;
    stats->red_bytes      += tmp_stats.red_bytes;

#endif


#if 0
    if ((flow_id > CA_AAL_MAX_FLOW_ID && flow_id < CA_PILICER_L3_FLOW_ID_BASE) ||
        (flow_id > CA_PILICER_L3_FLOW_ID_BASE + CA_AAL_L3_MAX_FLOW_ID))
        return CA_E_PARAM;

    read_keep = read_keep;  /* ignore read-keep flag in G3 */

    if (flow_id <= CA_AAL_MAX_FLOW_ID)
    {
        memset((void *)&pm_data, 0, sizeof(pm_data));

        if (aal_l2_te_pm_policer_flow_get(device_id, flow_id, &pm_data) != CA_E_OK)
            return CA_E_ERROR;

        stats->green_packets  = pm_data.green_pkt;
        stats->yellow_packets = pm_data.yellow_pkt;
        stats->red_packets    = pm_data.red_pkt;
    }
    else
    {   /* l3 flow */
        memset((void *)&l3_pm_data, 0, sizeof(l3_pm_data));

        flow_id -= CA_PILICER_L3_FLOW_ID_BASE;
        if (aal_l3_te_pm_policer_flow_get(device_id, flow_id, &l3_pm_data) != CA_E_OK)
            return CA_E_ERROR;

        stats->green_packets  = l3_pm_data.green_pkt;
        stats->yellow_packets = l3_pm_data.yellow_pkt;
        stats->red_packets    = l3_pm_data.red_pkt;
    }
#endif

    return CA_E_OK;
}


ca_status_t ca_flow_policer_stats_clear
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t    flow_id
)
{
    ca_policer_stats_t stats;

    if (flow_id > min(CA_AAL_MAX_FLOW_ID, CA_AAL_L3_MAX_FLOW_ID))
        return CA_E_PARAM;

    memset(&stats, 0, sizeof(stats));

    (void)ca_flow_policer_stats_get(device_id, flow_id, TRUE, &stats);

    return CA_E_OK;
}

ca_status_t ca_group_policer_stats_get
(
    CA_IN  ca_device_id_t     device_id,
    CA_IN  ca_uint16_t        group_id,
    CA_OUT ca_policer_stats_t *stats
)
{
    return CA_E_OK;
}


ca_status_t ca_shaper_config_set
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_shaper_config_t *config
)
{
    ca_uint32_t i = 0;

    if (!config)
        return CA_E_OK;

    (void)aal_l2_te_shaper_ipq_set(device_id, 0, config->overhead);

    for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
        (void)aal_l2_te_shaper_port_ipq_set(device_id, i, 0);

    (void)aal_l3_te_shaper_ipq_set(device_id, 0, config->overhead);
    for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
        (void)aal_l3_te_shaper_port_ipq_set(device_id, i, 0);

    return CA_E_OK;
}

ca_status_t ca_shaper_config_get (
    CA_IN  ca_device_id_t        device_id,
    CA_OUT ca_shaper_config_t *config
)
{
    ca_uint16_t ipg = 0;

    if (!config)
        return CA_E_PARAM;

    (void)aal_l2_te_shaper_ipq_get(device_id, 0, &ipg);

    config->overhead = (ca_uint32_t)ipg;

    return CA_E_OK;
}

ca_status_t ca_system_shaper_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_msk_t profile_msk;
    aal_l2_te_shaper_tbc_cfg_t     profile;
    ca_port_id_t port = CA_AAL_L2TM_DPID_L3_WAN;

    if (!shaper)
        return CA_E_PARAM;

    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&profile_msk, 0, sizeof(profile_msk));

    /* configure L2 traffic shaper */
    profile_msk.s.bs     = 1;
    profile_msk.s.rate_k = 1;
    profile_msk.s.rate_m = 1;
    profile_msk.s.mode   = 1;
    profile_msk.s.state  = 1;

    profile.bs     = shaper->burst_size;
    profile.rate_k = shaper->rate % 1000;
    profile.rate_m = shaper->rate / 1000;
    profile.mode   = shaper->pps ? CA_AAL_SHAPER_MODE_PPS : CA_AAL_SHAPER_MODE_BPS;
    profile.state  = shaper->enable ? CA_AAL_SHAPER_ADMIN_STATE_SHAPER : CA_AAL_SHAPER_ADMIN_STATE_FORWARD;

    if (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER)
    {
        /* check burst size, minimum value is dependent on rate_m */
        if (profile.mode == CA_AAL_SHAPER_MODE_BPS)
        {
            if (profile.bs < ((profile.rate_m + 511) >> 9))
                return CA_E_PARAM;
        }
        else /* pps */
        {
            if (profile.bs < ((profile.rate_m + 1) >> 2))
                return CA_E_PARAM;
        }
    }

    if (aal_l2_te_shaper_port_tbc_set(device_id, port, profile_msk, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_system_shaper_set() : failed to set L2 shaper profile for port %d, L%d\n", port, __LINE__);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t ca_system_shaper_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_t profile;
    ca_port_id_t port = CA_AAL_L2TM_DPID_L3_WAN;

    if (!shaper)
        return CA_E_PARAM;

    memset((void *)&profile, 0, sizeof(profile));

    if (aal_l2_te_shaper_port_tbc_get(device_id, port, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_system_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
        return CA_E_ERROR;
    }

    shaper->rate       = profile.rate_m * 1000 + profile.rate_k;
    shaper->burst_size = profile.bs;
    shaper->pps        = (profile.mode == CA_AAL_SHAPER_MODE_PPS) ? TRUE : FALSE;
    shaper->enable     = (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;

    return CA_E_OK;
}


ca_status_t ca_port_shaper_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_msk_t profile_msk;
    aal_l2_te_shaper_tbc_cfg_t     profile;
    aal_l3_te_shaper_tbc_cfg_msk_t l3_profile_msk;
    aal_l3_te_shaper_tbc_cfg_t     l3_profile;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_port_id_t port = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ca_uint8_t deep_q_flag = 0;
#endif

    if (!shaper)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);
    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN && port != CA_PORT_ID_L3_LAN))
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_CPU)
        {
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else
        {
            return CA_E_PARAM;
        }
    #else
        if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN && port != CA_PORT_ID_L3_LAN))
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_CPU)
        {
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3)
        {
            #ifdef CONFIG_ARCH_CORTINA_G3LITE
			#ifndef CONFIG_RTL_G3LITE_QOS_SUPPORT
                if (port != CA_PORT_ID_NI0)
                    return CA_E_PARAM;
			#endif
            #else
                if (port != CA_PORT_ID_NI7)
                    return CA_E_PARAM;
            #endif
        }
        else
        {
            return CA_E_PARAM;
        }
    #endif


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &g_l3fe_to_wan_flag) != CA_E_OK)
    {
        ca_printf("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT.\n", __func__);
    }
#endif

    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&profile_msk, 0, sizeof(profile_msk));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    /* configure L2 traffic shaper */
    profile_msk.s.bs     = 1;
    profile_msk.s.rate_k = 1;
    profile_msk.s.rate_m = 1;
    profile_msk.s.mode   = 1;
    profile_msk.s.state  = 1;

    profile.bs     = shaper->burst_size;    /* in unit of 256 bytes */
    profile.rate_k = shaper->rate % 1000;
    profile.rate_m = shaper->rate / 1000;
    profile.mode   = shaper->pps ? CA_AAL_SHAPER_MODE_PPS : CA_AAL_SHAPER_MODE_BPS;
    profile.state  = shaper->enable ? CA_AAL_SHAPER_ADMIN_STATE_SHAPER : CA_AAL_SHAPER_ADMIN_STATE_FORWARD;

    if (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER)
    {
        /* check burst size, minimum value is dependent on rate_m */
        if (profile.mode == CA_AAL_SHAPER_MODE_BPS)
        {
            if (profile.bs < ((profile.rate_m + 511) >> 9))
                return CA_E_PARAM;
        }
        else /* pps */
        {
            if (profile.bs < ((profile.rate_m + 1) >> 2))
                return CA_E_PARAM;
        }
    }

    /* configure L3 traffic shaper */
    l3_profile_msk.s.bs     = 1;
    l3_profile_msk.s.rate_k = 1;
    l3_profile_msk.s.rate_m = 1;
    l3_profile_msk.s.mode   = 1;
    l3_profile_msk.s.state  = 1;

    l3_profile.bs     = shaper->burst_size;
    l3_profile.rate_k = shaper->rate % 1000;
    l3_profile.rate_m = shaper->rate / 1000;
    l3_profile.mode   = shaper->pps ? CA_AAL_SHAPER_MODE_PPS : CA_AAL_SHAPER_MODE_BPS;
    l3_profile.state  = shaper->enable ? CA_AAL_SHAPER_ADMIN_STATE_SHAPER : CA_AAL_SHAPER_ADMIN_STATE_FORWARD;

    switch (port_type)
    {
    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;
        if (aal_l3_te_shaper_port_tbc_set(device_id, port, l3_profile_msk, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_shaper_set() : failed to set L3 shaper profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }
        break;

    case CA_PORT_TYPE_L3:       /* WAN */
        #if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (g_l3fe_to_wan_flag)
            {
                /* bypass L3TM, using L2 engine */
                port = CA_AAL_L2TM_DPID_L3_LAN;
                if (aal_l2_te_shaper_port_tbc_set(device_id, port, profile_msk, &profile) != CA_E_OK)
                {
                    CA_TM_LOG("ca_port_shaper_set() : failed to set L2 shaper profile for port %d, L%d\n", port, __LINE__);
                    return CA_E_ERROR;
                }
            }
            else
            {
            /* pass through L3TM, using L3 engine */
            port += CA_AAL_L3_TE_NI_PORT_OFFSET; /* entey 15 for WAN */
            if (aal_l3_te_shaper_port_tbc_set(device_id, port, l3_profile_msk, &l3_profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_shaper_set() : failed to set L3 shaper profile for port %d, L%d\n", port, __LINE__);
                return CA_E_ERROR;
            }
            }
        #else
            port += CA_AAL_L3_TE_NI_PORT_OFFSET; /* entey 15 for WAN */
            if (aal_l3_te_shaper_port_tbc_set(device_id, port, l3_profile_msk, &l3_profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_shaper_set() : failed to set L3 shaper profile for port %d, L%d\n", port, __LINE__);
                return CA_E_ERROR;
            }
        #endif
        break;

    case CA_PORT_TYPE_EPON:
    case CA_PORT_TYPE_GPON:
        #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        if (aal_scfg_read(CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE, 1, &deep_q_flag) != CA_E_OK)
        {
            ca_printf("%s: failed to read CFG_ID_INTERFACE_DEEP_Q.\n", __func__);
        }

        if (deep_q_flag)
        {
            /* saturn sfu upstream passes through deep-queue, so its dpid is 0x8 intead of 0x7 */
            port = 8;
        }
        #endif

        if (aal_l2_te_shaper_port_tbc_set(device_id, port, profile_msk, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_shaper_set() : failed to set L2 shaper profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }
        break;

    case CA_PORT_TYPE_ETHERNET:
        /* convert LDPID to TM LDPID */
        if (port == CA_PORT_ID_L3_WAN)
            port = CA_AAL_L2TM_DPID_L3_WAN;
        else if (port == CA_PORT_ID_L3_LAN)
            port = CA_AAL_L2TM_DPID_L3_LAN;

        if (aal_l2_te_shaper_port_tbc_set(device_id, port, profile_msk, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_shaper_set() : failed to set L2 shaper profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }
        break;

    default:
        break;
    }

    return CA_E_OK;
}

ca_status_t ca_port_shaper_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_t     profile;
    aal_l3_te_shaper_tbc_cfg_t     l3_profile;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_port_id_t port = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ca_uint8_t deep_q_flag = 0;
#endif

    if (!shaper)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN && port != CA_PORT_ID_L3_LAN))
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_CPU)
        {
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3)
        {
            if (port != CA_PORT_ID_NI7)
                return CA_E_PARAM;
        }
    #else
        if (port_type == CA_PORT_TYPE_ETHERNET)
        {
            if (port > CA_AAL_MAX_PORT_ID && (port != CA_PORT_ID_L3_WAN && port != CA_PORT_ID_L3_LAN))
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_CPU)
        {
            if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
                return CA_E_PARAM;
        }
        else if (port_type == CA_PORT_TYPE_L3)
        {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
		#ifndef CONFIG_RTL_G3LITE_QOS_SUPPORT
                if (port != CA_PORT_ID_NI0)
                    return CA_E_PARAM;
		#endif
        #else
                if (port != CA_PORT_ID_NI7)
                    return CA_E_PARAM;
        #endif
        }
    #endif

    #if 0
    else if (port_type == CA_PORT_TYPE_L2RP)
    {
        port_id = PORT_ID(port_id);
        if (port_id != AAL_LPORT_L3_LAN)
            return CA_E_PARAM;

        /* L3_WAN(0x18) use L3 policer table entry 7 (same as WAN); L3_LAN (0x19) uses L2 TE entry 13 */

        port_id = 13;
    }
    #endif
    else
    {
        return CA_E_PARAM;
    }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &g_l3fe_to_wan_flag) != CA_E_OK)
    {
        ca_printf("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT.\n", __func__);
    }
#endif

    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));

    switch (port_type)
    {
    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;
        if (aal_l3_te_shaper_port_tbc_get(device_id, port, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
            return CA_E_ERROR;
        }

        shaper->rate       = l3_profile.rate_m * 1000 + l3_profile.rate_k;
        shaper->burst_size = l3_profile.bs;
        shaper->pps        = (l3_profile.mode == CA_AAL_L3_SHAPER_MODE_PPS) ? TRUE : FALSE;
        shaper->enable     = (l3_profile.state == CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;

        break;
    case CA_PORT_TYPE_L3:   /* WAN */
        #if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
            if (g_l3fe_to_wan_flag)
            {
                port = CA_AAL_L2TM_DPID_L3_LAN;
                if (aal_l2_te_shaper_port_tbc_get(device_id, port, &profile) != CA_E_OK)
                {
                    CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
                    return CA_E_ERROR;
                }

                shaper->rate       = profile.rate_m * 1000 + profile.rate_k;
                shaper->burst_size = profile.bs;
                shaper->pps        = (profile.mode == CA_AAL_SHAPER_MODE_PPS) ? TRUE : FALSE;
                shaper->enable     = (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
            }
            else
            {
                port += CA_AAL_L3_TE_NI_PORT_OFFSET;
                if (aal_l3_te_shaper_port_tbc_get(device_id, port, &l3_profile) != CA_E_OK)
                {
                    CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
                    return CA_E_ERROR;
                }

                shaper->rate       = l3_profile.rate_m * 1000 + l3_profile.rate_k;
                shaper->burst_size = l3_profile.bs;
                shaper->pps        = (l3_profile.mode == CA_AAL_L3_SHAPER_MODE_PPS) ? TRUE : FALSE;
                shaper->enable     = (l3_profile.state == CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
            }
        #else
            port += CA_AAL_L3_TE_NI_PORT_OFFSET;
            if (aal_l3_te_shaper_port_tbc_get(device_id, port, &l3_profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
                return CA_E_ERROR;
            }

            shaper->rate       = l3_profile.rate_m * 1000 + l3_profile.rate_k;
            shaper->burst_size = l3_profile.bs;
            shaper->pps        = (l3_profile.mode == CA_AAL_L3_SHAPER_MODE_PPS) ? TRUE : FALSE;
            shaper->enable     = (l3_profile.state == CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
        #endif
        break;

        case CA_PORT_TYPE_EPON:
        case CA_PORT_TYPE_GPON:
        #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
            if (aal_scfg_read(CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE, 1, &deep_q_flag) != CA_E_OK)
            {
                ca_printf("%s: failed to read CFG_ID_INTERFACE_DEEP_Q.\n", __func__);
            }

            if (deep_q_flag)
            {
                /* saturn sfu upstream passes through deep-queue, so its dpid is 0x8 intead of 0x7 */
                port = 8;
            }
        #endif

            if (aal_l2_te_shaper_port_tbc_get(device_id, port, &profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
                return CA_E_ERROR;
            }

            shaper->rate       = profile.rate_m * 1000 + profile.rate_k;
            shaper->burst_size = profile.bs;
            shaper->pps        = (profile.mode == CA_AAL_SHAPER_MODE_PPS) ? TRUE : FALSE;
            shaper->enable     = (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
            break;

        case CA_PORT_TYPE_ETHERNET:
            /* convert LDPID to TM LDPID */
            if (port == CA_PORT_ID_L3_WAN)
                port = CA_AAL_L2TM_DPID_L3_WAN;
            else if (port == CA_PORT_ID_L3_LAN)
                port = CA_AAL_L2TM_DPID_L3_LAN;

            if (aal_l2_te_shaper_port_tbc_get(device_id, port, &profile) != CA_E_OK)
            {
                CA_TM_LOG("ca_port_shaper_get() : failed to get shaper profile for port %d, L%d\n", port, __LINE__);
                return CA_E_ERROR;
            }

            shaper->rate       = profile.rate_m * 1000 + profile.rate_k;
            shaper->burst_size = profile.bs;
            shaper->pps        = (profile.mode == CA_AAL_SHAPER_MODE_PPS) ? TRUE : FALSE;
            shaper->enable     = (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
            break;

        default:
            break;
    }

    return CA_E_OK;
}

ca_status_t ca_queue_shaper_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue_id,
    CA_IN ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_msk_t profile_msk;
    aal_l2_te_shaper_tbc_cfg_t     profile;
    aal_l3_te_shaper_tbc_cfg_msk_t l3_profile_msk;
    aal_l3_te_shaper_tbc_cfg_t     l3_profile;
    ca_uint32_t voq_id = queue_id;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_port_id_t port = 0;

    if (!shaper)
        return CA_E_PARAM;

    if (queue_id > 7)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
        #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
        #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#endif

    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&profile_msk, 0, sizeof(profile_msk));

    /* configure L2 shaper */
    profile_msk.s.bs     = 1;
    profile_msk.s.rate_k = 1;
    profile_msk.s.rate_m = 1;
    profile_msk.s.mode   = 1;
    profile_msk.s.state  = 1;

    /* VoQ shaper is based on 500kbps */
    profile.bs     = shaper->burst_size;
    profile.rate_k = shaper->rate % 500;
    profile.rate_m = shaper->rate / 500;
    profile.mode   = shaper->pps ? CA_AAL_SHAPER_MODE_PPS : CA_AAL_SHAPER_MODE_BPS;
    profile.state  = shaper->enable ? CA_AAL_SHAPER_ADMIN_STATE_SHAPER : CA_AAL_SHAPER_ADMIN_STATE_FORWARD;

    if (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER)
    {
        /* check burst size, minimum value is dependent on rate_m */
        if (profile.mode == CA_AAL_SHAPER_MODE_BPS)
        {
            if (profile.bs < ((profile.rate_m + 511) >> 9))
                return CA_E_PARAM;
        }
        else /* pps */
        {
            if (profile.bs < ((profile.rate_m + 1) >> 2))
                return CA_E_PARAM;
        }
    }

    /* configure L3 shaper */
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    l3_profile_msk.s.bs     = 1;
    l3_profile_msk.s.rate_k = 1;
    l3_profile_msk.s.rate_m = 1;
    l3_profile_msk.s.mode   = 1;
    l3_profile_msk.s.state  = 1;

    /* VoQ shaper is based on 500kbps */
    l3_profile.bs     = shaper->burst_size;
    l3_profile.rate_k = shaper->rate % 500;
    l3_profile.rate_m = shaper->rate / 500;
    l3_profile.mode   = shaper->pps ? CA_AAL_L3_SHAPER_MODE_PPS : CA_AAL_L3_SHAPER_MODE_BPS;
    l3_profile.state  = shaper->enable ? CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER : CA_AAL_L3_SHAPER_ADMIN_STATE_FORWARD;


    switch (port_type)
    {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    case CA_PORT_TYPE_EPON:
    case CA_PORT_TYPE_GPON:
#endif
    case CA_PORT_TYPE_ETHERNET:

        voq_id = port * 8 + queue_id;
        if (aal_l2_te_shaper_voq_tbc_set(device_id, voq_id, profile_msk, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_set() : failed to set shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }
        break;

    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;
        voq_id = port * 8 + queue_id;
        if (aal_l3_te_shaper_voq_tbc_set(device_id, voq_id, l3_profile_msk, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_set() : failed to set shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }
        break;

    case CA_PORT_TYPE_L3:
        /* configure L3 shaper for WAN port */
        voq_id = (port + CA_AAL_L3_TE_NI_PORT_OFFSET) * 8 + queue_id;
        if (aal_l3_te_shaper_voq_tbc_set(device_id, voq_id, l3_profile_msk, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_set() : failed to set shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }
        break;

    default:
        break;
    }

    return CA_E_OK;
}

ca_status_t ca_queue_shaper_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    queue_id,
    CA_OUT ca_shaper_t    *shaper
)
{
    aal_l2_te_shaper_tbc_cfg_t     profile;
    aal_l3_te_shaper_tbc_cfg_t     l3_profile;
    ca_uint32_t port_type = CA_PORT_TYPE_INVALID;
    ca_port_id_t port = 0;
    ca_uint32_t voq_id = queue_id;

    if (!shaper)
        return CA_E_PARAM;

    if (queue_id > 7)
        return CA_E_PARAM;

    port_type = PORT_TYPE(port_id);
    port      = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    if (port_type == CA_PORT_TYPE_EPON || port_type == CA_PORT_TYPE_GPON)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    }
    else
    {
        return CA_E_PARAM;
    }
#else
    if (port_type == CA_PORT_TYPE_ETHERNET)
    {
        if (port > CA_AAL_MAX_PORT_ID)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_CPU)
    {
        if (port < CA_PORT_ID_CPU0 || port > CA_PORT_ID_CPU7)
            return CA_E_PARAM;
    }
    else if (port_type == CA_PORT_TYPE_L3)
    {
    #ifdef CONFIG_ARCH_CORTINA_G3LITE
        if (port != CA_PORT_ID_NI0)
            return CA_E_PARAM;
    #else
        if (port != CA_PORT_ID_NI7)
            return CA_E_PARAM;
    #endif
    }
    else
    {
        return CA_E_PARAM;
    }
#endif


    memset((void *)&profile,     0, sizeof(profile));
    memset((void *)&l3_profile,  0, sizeof(l3_profile));

    switch (port_type)
    {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    case CA_PORT_TYPE_EPON:
    case CA_PORT_TYPE_GPON:
#endif
    case CA_PORT_TYPE_ETHERNET:
        voq_id = port * 8 + queue_id;
        if (aal_l2_te_shaper_voq_tbc_get(device_id, voq_id, &profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_get() : failed to get shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }

        shaper->rate       = profile.rate_m * 500 + profile.rate_k;
        shaper->burst_size = profile.bs;
        shaper->pps        = (profile.mode == CA_AAL_SHAPER_MODE_PPS) ? TRUE : FALSE;
        shaper->enable     = (profile.state == CA_AAL_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
        break;

    case CA_PORT_TYPE_L3:
        voq_id = (port + CA_AAL_L3_TE_NI_PORT_OFFSET) * 8 + queue_id;
        if (aal_l3_te_shaper_voq_tbc_get(device_id, voq_id, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_get() : failed to get shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }

        shaper->rate       = l3_profile.rate_m * 500 + l3_profile.rate_k;
        shaper->burst_size = l3_profile.bs;
        shaper->pps        = (l3_profile.mode == CA_AAL_L3_SHAPER_MODE_PPS) ? TRUE : FALSE;
        shaper->enable     = (l3_profile.state == CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
        break;

    case CA_PORT_TYPE_CPU:
        port -= CA_PORT_ID_CPU0;
        voq_id = port * 8 + queue_id;
        if (aal_l3_te_shaper_voq_tbc_get(device_id, voq_id, &l3_profile) != CA_E_OK)
        {
            CA_TM_LOG("ca_queue_shaper_get() : failed to get shaper profile for VoQ %d, L%d\n", voq_id, __LINE__);
            return CA_E_ERROR;
        }

        shaper->rate       = l3_profile.rate_m * 500 + l3_profile.rate_k;
        shaper->burst_size = l3_profile.bs;
        shaper->pps        = (l3_profile.mode == CA_AAL_L3_SHAPER_MODE_PPS) ? TRUE : FALSE;
        shaper->enable     = (l3_profile.state == CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER) ? TRUE : FALSE;
        break;

    default:
        break;
    }

    return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_l2_policer_config_set (
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_policer_config_t *config
)
{
    aal_l2_te_policer_cfg_t     cfg;
    aal_l2_te_policer_cfg_msk_t msk;
    ca_uint32_t i = 0;

    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&msk, 0, sizeof(msk));

    cfg.ipg   = config->overhead;
    msk.s.ipg = 1;

    for (i=0; i<=(ca_uint32_t)CA_AAL_POLICER_PKT_TYPE_UBC; i++)
    {
        if (aal_l2_te_policer_pkt_type_cfg_set(device_id, (aal_l2_te_policer_pkt_type_t)i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l2_policer_config_set() : error - failed to set ipg for packet type %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
    {
        if (aal_l2_te_policer_port_cfg_set(device_id, i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l2_policer_config_set() : error - failed to set ipg for port %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_MAX_FLOW_ID; i++)
    {
        if (aal_l2_te_policer_flow_cfg_set(device_id, i, msk, &cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l2_policer_config_set() : error - failed to set ipg for flow %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    return CA_E_OK;
}

ca_status_t ca_l2_policer_config_get (
    CA_IN  ca_device_id_t         device_id,
    CA_OUT ca_policer_config_t *config
)
{
    aal_l2_te_policer_cfg_t cfg;

    memset((void *)&cfg, 0, sizeof(cfg));

    if (!config)
        return CA_E_PARAM;

    /* as ipg configurations for ports, flows, flow groups, and pkt type are totally same, we just get any of them */
    if (aal_l2_te_policer_port_cfg_get(device_id, 0, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_l2_policer_config_get() : error - failed to get ipg for port %d, L%d\n", 0, __LINE__);
        return CA_E_ERROR;
    }

    config->overhead = cfg.ipg;

    return CA_E_OK;
}

ca_status_t ca_l3_policer_config_set (
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_policer_config_t *config
)
{
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    ca_uint32_t i = 0;

    /* L3 */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));

    l3_cfg.ipg   = config->overhead;
    l3_msk.s.ipg = 1;
    for (i=0; i<=(ca_uint32_t)CA_AAL_L3_POLICER_PKT_TYPE_UBC; i++)
    {
        if (aal_l3_te_policer_pkt_type_cfg_set(device_id, (aal_l3_te_policer_pkt_type_t)i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l3_policer_config_set() : error - failed to set ipg for packet type %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_L3_MAX_PORT_ID; i++)
    {
        if (aal_l3_te_policer_port_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l3_policer_config_set() : error - failed to set ipg for port %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

    for (i=0; i<=CA_AAL_L3_MAX_FLOW_ID; i++)
    {
        if (aal_l3_te_policer_flow_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l3_policer_config_set() : error - failed to set ipg for flow %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }

#if defined(CONFIG_ARCH_CORTINA_VENUS)
	//ipg configuration is regarded as a global setting, set to policer2 and policer3 at the same time
{
	ca_uint32_t maxidx = 0;
	TE_TE_GLB_CTRL_t glb_ctrl;
	glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);
	if (glb_ctrl.bf.sel3flows == 0)
		maxidx = CA_AAL_L3_MAX_AGR_FLOW_ID;
	else if (glb_ctrl.bf.sel3flows == 1)
		maxidx = CA_AAL_L3_MAX_FLOW2_ID;
	for (i=0; i<=maxidx; i++)
    {
        if (aal_l3_te_policer_agr_flow_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l3_policer_config_set() : error - failed to set ipg for flow policer2 %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }
}
	for (i=0; i<=CA_AAL_L3_MAX_FLOW3_ID; i++)
    {
        if (aal_l3_te_policer_flow3_cfg_set(device_id, i, l3_msk, &l3_cfg) != CA_E_OK)
        {
            CA_TM_LOG("ca_l3_policer_config_set() : error - failed to set ipg for flow policer3 %d, L%d\n", (ca_uint32_t)i, __LINE__);
            return CA_E_ERROR;
        }
    }
#endif

    return CA_E_OK;
}

ca_status_t ca_l3_policer_config_get (
    CA_IN  ca_device_id_t         device_id,
    CA_OUT ca_policer_config_t *config
)
{
    aal_l3_te_policer_cfg_t     l3_cfg;

    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));

    if (!config)
        return CA_E_PARAM;

    /* as ipg configurations for ports, flows, flow groups, and pkt type are totally same, we just get any of them */
    if (aal_l3_te_policer_port_cfg_get(device_id, 0, &l3_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_l3_policer_config_get() : error - failed to get ipg for port %d, L%d\n", 0, __LINE__);
        return CA_E_ERROR;
    }

    config->overhead = l3_cfg.ipg;

    return CA_E_OK;
}


ca_status_t ca_l2_flow_policer_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint16_t  flow_id,
    CA_IN ca_policer_t *policer
)
{
    aal_l2_te_policer_cfg_t cfg;
    aal_l2_te_policer_cfg_msk_t msk;
    aal_l2_te_policer_profile_t profile;
    aal_l2_te_policer_profile_msk_t profile_msk;

    if (!policer)
        return CA_E_PARAM;

    /* flow id range will be lesser than the minimum value between L2 and L3 flow ID */
    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
    #else
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
    #endif

    /* l2 init */
    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&msk, 0, sizeof(msk));
    memset((void *)&profile, 0, sizeof(profile));
    memset((void *)&profile_msk, 0, sizeof(profile_msk));

    /* l2 flow engine */
    msk.s.type = 1;
    msk.s.pol_update_mode = 1;

    cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

    if (aal_l2_te_policer_flow_cfg_set(device_id, flow_id, msk, &cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    profile_msk.s.cir_k = 1;
    profile_msk.s.cir_m = 1;
    profile_msk.s.cbs   = 1;
    profile_msk.s.pir_k = 1;
    profile_msk.s.pir_m = 1;
    profile_msk.s.pbs   = 1;
    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        profile.cir_k = CA_AAL_L2_TE_POL_MAX_RATE_K;
        profile.cir_m = CA_AAL_L2_TE_POL_MAX_RATE_M;
        profile.cbs   = CA_AAL_L2_TE_POL_MAX_CBS;
        profile.pir_k = CA_AAL_L2_TE_POL_MAX_RATE_K;
        profile.pir_m = CA_AAL_L2_TE_POL_MAX_RATE_M;
        profile.pbs   = CA_AAL_L2_TE_POL_MAX_PBS;
    }
    else
    {
        profile.cir_k = policer->cir % 500;
        profile.cir_m = policer->cir / 500;
        profile.cbs   = policer->cbs;
        profile.pir_k = policer->pir % 500;
        profile.pir_m = policer->pir / 500;
        profile.pbs   = policer->pbs;
    }

    if (aal_l2_te_policer_flow_profile_set(device_id, flow_id, profile_msk, &profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l2 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t ca_l2_flow_policer_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint16_t  flow_id,
    CA_OUT ca_policer_t *policer
)
{
    aal_l2_te_policer_cfg_t     cfg;
    aal_l2_te_policer_profile_t profile;

    if (!policer)
      return CA_E_PARAM;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
#else
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
#endif

    /* as each flow will be configured into both l2 and l3 engine, reading configuration from either engine is enough */
    memset((void *)&cfg,        0, sizeof(cfg));
    memset((void *)&profile,    0, sizeof(profile));

    if (aal_l2_te_policer_flow_cfg_get(device_id, flow_id, &cfg) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get configurations for flow %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   if (aal_l2_te_policer_flow_profile_get(device_id, flow_id, &profile) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get policing profile for flow %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   policer->cir = profile.cir_m * 500 + profile.cir_k;
   policer->pir = profile.pir_m * 500 + profile.pir_k;
   policer->cbs = profile.cbs;
   policer->pbs = profile.pbs;
   policer->pps = (cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

   switch (cfg.type)
   {
   case CA_AAL_POLICER_TYPE_NONE:
       policer->mode = CA_POLICER_MODE_DISABLE;
       break;

   case CA_AAL_POLICER_TYPE_RFC2698:
     policer->mode = CA_POLICER_MODE_TRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC2697:
     policer->mode = CA_POLICER_MODE_SRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115:
     policer->mode = CA_POLICER_MODE_TRTCMDS;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115_EX:
     policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
     break;

   default:
     break;
   }

   return CA_E_OK;
}

ca_status_t ca_l3_flow_policer_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint16_t  flow_id,
    CA_IN ca_policer_t *policer
)
{
    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_l3_te_policer_cfg_t l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l3_te_policer_profile_t l3_profile;
    aal_l3_te_policer_profile_msk_t l3_profile_msk;
    #endif

    if (!policer)
        return CA_E_PARAM;

    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
    #else
    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;
    #endif

    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    /* l3 init */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    /* l3 flow engine */
    l3_msk.s.type = 1;
    l3_msk.s.pol_update_mode = 1;

    l3_cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        l3_cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

    if (aal_l3_te_policer_flow_cfg_set(device_id, flow_id, l3_msk, &l3_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l3 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    l3_profile_msk.s.cir_k = 1;
    l3_profile_msk.s.cir_m = 1;
    l3_profile_msk.s.cbs   = 1;
    l3_profile_msk.s.pir_k = 1;
    l3_profile_msk.s.pir_m = 1;
    l3_profile_msk.s.pbs   = 1;

    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        l3_profile.cir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.cir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.cbs   = CA_AAL_L3_TE_POL_MAX_CBS;
        l3_profile.pir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.pir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.pbs   = CA_AAL_L3_TE_POL_MAX_PBS;
    }
    else
    {
        l3_profile.cir_k = policer->cir % 500;
        l3_profile.cir_m = policer->cir / 500;
        l3_profile.cbs   = policer->cbs;
        l3_profile.pir_k = policer->pir % 500;
        l3_profile.pir_m = policer->pir / 500;
        l3_profile.pbs   = policer->pbs;
    }


    if (aal_l3_te_policer_flow_profile_set(device_id, flow_id, l3_profile_msk, &l3_profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l3 flow %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }
    #endif

    return CA_E_OK;
}

ca_status_t ca_l3_flow_policer_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint16_t  flow_id,
    CA_OUT ca_policer_t *policer
)
{
    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_profile_t l3_profile;
    #endif

    if (!policer)
      return CA_E_PARAM;

    /* flow id range will be lesser than the minimum value between L2 and L3 flow ID */
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;
#else
    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;
#endif

    #if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    memset((void *)&l3_cfg,        0, sizeof(l3_cfg));
    memset((void *)&l3_profile,    0, sizeof(l3_profile));

    if (aal_l3_te_policer_flow_cfg_get(device_id, flow_id, &l3_cfg) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get configurations for L3 flow %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   if (aal_l3_te_policer_flow_profile_get(device_id, flow_id, &l3_profile) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get policing profile for L3 flow %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   policer->cir = l3_profile.cir_m * 500 + l3_profile.cir_k;
   policer->pir = l3_profile.pir_m * 500 + l3_profile.pir_k;
   policer->cbs = l3_profile.cbs;
   policer->pbs = l3_profile.pbs;
   policer->pps = (l3_cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

   switch (l3_cfg.type)
   {
   case CA_AAL_POLICER_TYPE_NONE:
       policer->mode = CA_POLICER_MODE_DISABLE;
       break;

   case CA_AAL_POLICER_TYPE_RFC2698:
     policer->mode = CA_POLICER_MODE_TRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC2697:
     policer->mode = CA_POLICER_MODE_SRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115:
     policer->mode = CA_POLICER_MODE_TRTCMDS;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115_EX:
     policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
     break;

   default:
     break;
   }
   #endif

   return CA_E_OK;
}

#if defined(CONFIG_ARCH_CORTINA_VENUS)
ca_status_t ca_l3_flow_policer2_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint16_t  flow_id,
    CA_IN ca_policer_t *policer
)
{
    aal_l3_te_policer_cfg_t l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l3_te_policer_profile_t l3_profile;
    aal_l3_te_policer_profile_msk_t l3_profile_msk;

    if (!policer)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    /* l3 init */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    /* l3 flow engine */
    l3_msk.s.type = 1;
    l3_msk.s.pol_update_mode = 1;

    l3_cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        l3_cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

    if (aal_l3_te_policer_agr_flow_cfg_set(device_id, flow_id, l3_msk, &l3_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l3 flow policer2 %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    l3_profile_msk.s.cir_k = 1;
    l3_profile_msk.s.cir_m = 1;
    l3_profile_msk.s.cbs   = 1;
    l3_profile_msk.s.pir_k = 1;
    l3_profile_msk.s.pir_m = 1;
    l3_profile_msk.s.pbs   = 1;
    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        l3_profile.cir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.cir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.cbs   = CA_AAL_L3_TE_POL_MAX_CBS;
        l3_profile.pir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.pir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.pbs   = CA_AAL_L3_TE_POL_MAX_PBS;
    }
    else
    {
        l3_profile.cir_k = policer->cir % 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.cir_m = policer->cir / 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.cbs   = policer->cbs;
        l3_profile.pir_k = policer->pir % 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.pir_m = policer->pir / 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.pbs   = policer->pbs;
    }


    if (aal_l3_te_policer_agr_flow_profile_set(device_id, flow_id, l3_profile_msk, &l3_profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l3 flow policer2 %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t ca_l3_flow_policer2_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint16_t  flow_id,
    CA_OUT ca_policer_t *policer
)
{
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_profile_t l3_profile;

    if (!policer)
      return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    memset((void *)&l3_cfg,        0, sizeof(l3_cfg));
    memset((void *)&l3_profile,    0, sizeof(l3_profile));

    if (aal_l3_te_policer_agr_flow_cfg_get(device_id, flow_id, &l3_cfg) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get configurations for L3 flow policer2 %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   if (aal_l3_te_policer_agr_flow_profile_get(device_id, flow_id, &l3_profile) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get policing profile for L3 flow policer2 %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   policer->cir = l3_profile.cir_m * 1000 + l3_profile.cir_k;  //policer1: 500 ; policer2 & policer3: 1000
   policer->pir = l3_profile.pir_m * 1000 + l3_profile.pir_k;  //policer1: 500 ; policer2 & policer3: 1000
   policer->cbs = l3_profile.cbs;
   policer->pbs = l3_profile.pbs;
   policer->pps = (l3_cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

   switch (l3_cfg.type)
   {
   case CA_AAL_POLICER_TYPE_NONE:
       policer->mode = CA_POLICER_MODE_DISABLE;
       break;

   case CA_AAL_POLICER_TYPE_RFC2698:
     policer->mode = CA_POLICER_MODE_TRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC2697:
     policer->mode = CA_POLICER_MODE_SRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115:
     policer->mode = CA_POLICER_MODE_TRTCMDS;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115_EX:
     policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
     break;

   default:
     break;
   }

   return CA_E_OK;
}

ca_status_t ca_l3_flow_policer3_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint16_t  flow_id,
    CA_IN ca_policer_t *policer
)
{
    aal_l3_te_policer_cfg_t l3_cfg;
    aal_l3_te_policer_cfg_msk_t l3_msk;
    aal_l3_te_policer_profile_t l3_profile;
    aal_l3_te_policer_profile_msk_t l3_profile_msk;

    if (!policer)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    /* l3 init */
    memset((void *)&l3_cfg, 0, sizeof(l3_cfg));
    memset((void *)&l3_msk, 0, sizeof(l3_msk));
    memset((void *)&l3_profile, 0, sizeof(l3_profile));
    memset((void *)&l3_profile_msk, 0, sizeof(l3_profile_msk));

    /* l3 flow engine */
    l3_msk.s.type = 1;
    l3_msk.s.pol_update_mode = 1;

    l3_cfg.pol_update_mode = policer->pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE;

    switch (policer->mode)
    {
    case CA_POLICER_MODE_DISABLE:
        l3_cfg.type = CA_AAL_POLICER_TYPE_NONE;
        break;
    case CA_POLICER_MODE_SRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2697;
        break;
    case CA_POLICER_MODE_TRTCM:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC2698;
        break;
    case CA_POLICER_MODE_TRTCMDS:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115;
        break;
    case CA_POLICER_MODE_TRTCMDS_COUPLED:
        l3_cfg.type = CA_AAL_POLICER_TYPE_RFC4115_EX;
        break;
    case CA_POLICER_MODE_RATE_ONLY:
        return CA_E_NOT_SUPPORT;
        break;
    default:
        return CA_E_NOT_SUPPORT;
        break;
    }

    if (aal_l3_te_policer_flow3_cfg_set(device_id, flow_id, l3_msk, &l3_cfg) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set configurations for l3 flow policer3 %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    l3_profile_msk.s.cir_k = 1;
    l3_profile_msk.s.cir_m = 1;
    l3_profile_msk.s.cbs   = 1;
    l3_profile_msk.s.pir_k = 1;
    l3_profile_msk.s.pir_m = 1;
    l3_profile_msk.s.pbs   = 1;
    if (CA_POLICER_MODE_DISABLE == policer->mode)
    {
        l3_profile.cir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.cir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.cbs   = CA_AAL_L3_TE_POL_MAX_CBS;
        l3_profile.pir_k = CA_AAL_L3_TE_POL_MAX_RATE_K;
        l3_profile.pir_m = CA_AAL_L3_TE_POL_MAX_RATE_M;
        l3_profile.pbs   = CA_AAL_L3_TE_POL_MAX_PBS;
    }
    else
    {
        l3_profile.cir_k = policer->cir % 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.cir_m = policer->cir / 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.cbs   = policer->cbs;
        l3_profile.pir_k = policer->pir % 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.pir_m = policer->pir / 1000; //policer1: 500 ; policer2 & policer3: 1000
        l3_profile.pbs   = policer->pbs;
    }


    if (aal_l3_te_policer_flow3_profile_set(device_id, flow_id, l3_profile_msk, &l3_profile) != CA_E_OK)
    {
        CA_TM_LOG("ca_flow_policer_set() : failed to set policing profile for l3 flow policer3 %d, L%d\n", flow_id, __LINE__);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t ca_l3_flow_policer3_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint16_t  flow_id,
    CA_OUT ca_policer_t *policer
)
{
    aal_l3_te_policer_cfg_t     l3_cfg;
    aal_l3_te_policer_profile_t l3_profile;

    if (!policer)
      return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    memset((void *)&l3_cfg,        0, sizeof(l3_cfg));
    memset((void *)&l3_profile,    0, sizeof(l3_profile));

    if (aal_l3_te_policer_flow3_cfg_get(device_id, flow_id, &l3_cfg) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get configurations for L3 flow policer3 %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   if (aal_l3_te_policer_flow3_profile_get(device_id, flow_id, &l3_profile) != CA_E_OK)
   {
     CA_TM_LOG("ca_flow_policer_get() : failed to get policing profile for L3 flow policer3 %d, L%d\n", flow_id, __LINE__);
     return CA_E_ERROR;
   }

   policer->cir = l3_profile.cir_m * 1000 + l3_profile.cir_k; //policer1: 500 ; policer2 & policer3: 1000
   policer->pir = l3_profile.pir_m * 1000 + l3_profile.pir_k; //policer1: 500 ; policer2 & policer3: 1000
   policer->cbs = l3_profile.cbs;
   policer->pbs = l3_profile.pbs;
   policer->pps = (l3_cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT) ? TRUE : FALSE;

   switch (l3_cfg.type)
   {
   case CA_AAL_POLICER_TYPE_NONE:
       policer->mode = CA_POLICER_MODE_DISABLE;
       break;

   case CA_AAL_POLICER_TYPE_RFC2698:
     policer->mode = CA_POLICER_MODE_TRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC2697:
     policer->mode = CA_POLICER_MODE_SRTCM;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115:
     policer->mode = CA_POLICER_MODE_TRTCMDS;
     break;

   case CA_AAL_POLICER_TYPE_RFC4115_EX:
     policer->mode = CA_POLICER_MODE_TRTCMDS_COUPLED;
     break;

   default:
     break;
   }

   return CA_E_OK;
}


#endif
#endif


#ifdef __KERNEL__
EXPORT_SYMBOL(ca_policer_config_set);
EXPORT_SYMBOL(ca_policer_config_get);
EXPORT_SYMBOL(ca_port_policer_set);
EXPORT_SYMBOL(ca_port_policer_get);
EXPORT_SYMBOL(ca_port_policer_stats_get);
EXPORT_SYMBOL(ca_port_policer_stats_clear);
EXPORT_SYMBOL(ca_flow_policer_set);
EXPORT_SYMBOL(ca_flow_policer_get);
EXPORT_SYMBOL(ca_flow_policer_stats_get);
EXPORT_SYMBOL(ca_flow_policer_stats_clear);
EXPORT_SYMBOL(ca_group_policer_set);
EXPORT_SYMBOL(ca_group_policer_get);
EXPORT_SYMBOL(ca_group_policer_stats_get);
EXPORT_SYMBOL(ca_group_member_add);
EXPORT_SYMBOL(ca_group_member_delete);
EXPORT_SYMBOL(ca_group_member_delete_all);
EXPORT_SYMBOL(ca_shaper_config_set);
EXPORT_SYMBOL(ca_shaper_config_get);
EXPORT_SYMBOL(ca_system_shaper_set);
EXPORT_SYMBOL(ca_system_shaper_get);
EXPORT_SYMBOL(ca_port_shaper_set);
EXPORT_SYMBOL(ca_port_shaper_get);
EXPORT_SYMBOL(ca_queue_shaper_set);
EXPORT_SYMBOL(ca_queue_shaper_get);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(ca_l2_policer_config_set);
EXPORT_SYMBOL(ca_l2_policer_config_get);
EXPORT_SYMBOL(ca_l3_policer_config_set);
EXPORT_SYMBOL(ca_l3_policer_config_get);
EXPORT_SYMBOL(ca_l2_flow_policer_set);
EXPORT_SYMBOL(ca_l2_flow_policer_get);
EXPORT_SYMBOL(ca_l3_flow_policer_set);
EXPORT_SYMBOL(ca_l3_flow_policer_get);
#if defined(CONFIG_ARCH_CORTINA_VENUS)
EXPORT_SYMBOL(ca_l3_flow_policer2_set);
EXPORT_SYMBOL(ca_l3_flow_policer2_get);
EXPORT_SYMBOL(ca_l3_flow_policer3_set);
EXPORT_SYMBOL(ca_l3_flow_policer3_get);
#endif
#endif
#endif


