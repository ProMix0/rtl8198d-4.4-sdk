
#include "osal_common.h"
#include "epon.h"
#include "aal_pon.h"
#include "aal_epon.h"
#include "aal_epon_mac.h"
#include "aal_epon_crypto.h"
#include "aal_epon_fec.h"
#include "aal_puc.h"
#include "aal_psds.h"
#include "aal_port.h"
#include "aal_mpcp.h"
#include <soc/cortina/registers.h>
#include "ca-scfg/scfg.h"

#define __EPON_PORT_INDEX       7
#define __EPON_PORT_CHECK(port) if((PORT_TYPE(port)!=CA_PORT_TYPE_EPON) || (PORT_ID(port) != __EPON_PORT_INDEX)) { return CA_E_PARAM;}

#define __EPON_LLID_NUM_MAX     (AAL_EPON_LLID_NUM_MAX)
#define __EPON_LLID_CHECK(llid) \
    do {                                                  \
        ca_uint32_t m_llid_flag = 0;                      \
        (void)aal_mpcp_llid_mode_get(0, 0, &m_llid_flag); \
        if (m_llid_flag)                                  \
        {                                                 \
            if ((llid) >= __EPON_LLID_NUM_MAX) return CA_E_PARAM; \
        }                                                 \
        else                                              \
        {                                                 \
            if ((llid) != 0) return CA_E_PARAM;           \
        }                                                 \
    }while(0)

ca_status_t ca_epon_port_laser_polarity_set
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_epon_port_laser_polarity_t tx_polarity,
    CA_IN ca_epon_port_laser_polarity_t rx_polarity
)
{
    ca_status_t ret = CA_E_OK;
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
    aal_psds_rg24_mask_t  rg24_mask;
    aal_psds_rg24_t      rg24;
    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    rg24_mask.u32 = 0;
    memset(&rg24, 0, sizeof(aal_psds_rg24_t));
    rg24_mask.s.reg_ben_por_sel = 1;
    rg24.reg_ben_por_sel |= (tx_polarity << 1);

    __PON_LOCK();
    ret = aal_psds_rg24_set(device_id,rg24_mask,&rg24);
    ret = aal_psds_rx_polarity_set(device_id, rx_polarity);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);
#endif
    return ret;
}

ca_status_t ca_epon_port_laser_polarity_get
(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_OUT ca_epon_port_laser_polarity_t *tx_polarity,
    CA_OUT ca_epon_port_laser_polarity_t *rx_polarity
)
{
    ca_status_t ret = CA_E_OK;
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
    aal_psds_rg24_t      rg24;
    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(tx_polarity);
    CA_NULL_PTR_CHECK(rx_polarity);

    memset(&rg24, 0, sizeof(aal_psds_rg24_t));

    __PON_LOCK();
    ret = aal_psds_rg24_get(device_id, &rg24);
    *tx_polarity = (rg24.reg_ben_por_sel >> 1);

    ret = aal_psds_rx_polarity_get(device_id, rx_polarity);
    __PON_UNLOCK();
#endif
    return ret;
}

ca_status_t ca_epon_port_laser_mode_set
(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_port_id_t              port_id,
    CA_IN ca_epon_port_laser_mode_t mode
)
{
    ca_status_t          ret = CA_E_OK;
    aal_epon_laser_msk_t msk;
    aal_epon_laser_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    if(mode > CA_EPON_PORT_LASER_MODE_OFF)
    {
        return CA_E_PARAM;
    }

    // TODO: mode(burst, on, off) is the same as aal tx mode?
    memset(&msk, 0, sizeof(msk));
    memset(&cfg, 0, sizeof(cfg));

    switch (mode)
    {
    case CA_EPON_PORT_LASER_MODE_BURST:
        cfg.tx_mode        = AAL_EPON_LASER_MODE_BURST;
        cfg.tx_burst_en    = 1;
        msk.s.tx_mode      = 1;
        msk.s.tx_burst_en  = 1;
        break;
    case CA_EPON_PORT_LASER_MODE_ON:
        cfg.tx_mode        = AAL_EPON_LASER_MODE_ON;
        cfg.tx_burst_en    = 0;
        msk.s.tx_mode      = 1;
        msk.s.tx_burst_en  = 1;
        break;
    case CA_EPON_PORT_LASER_MODE_OFF:
        cfg.tx_mode        = AAL_EPON_LASER_MODE_OFF;
        cfg.tx_burst_en    = 0;
        msk.s.tx_mode      = 1;
        msk.s.tx_burst_en  = 1;
        break;
    default:
        //CS_LOG_MIN(CS_MOD_PON, "Invalid laser mode\n");
        return CA_E_ERROR;
    }

    __PON_LOCK();
    ret = aal_epon_laser_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_port_laser_mode_get
(
    CA_IN  ca_device_id_t            device_id,
    CA_IN  ca_port_id_t              port_id,
    CA_OUT ca_epon_port_laser_mode_t *mode)
{
    ca_status_t          ret = CA_E_OK;
    aal_epon_laser_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(mode);

    memset(&cfg, 0, sizeof(cfg));

    __PON_LOCK();
    ret = aal_epon_laser_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);


    if(AAL_EPON_LASER_MODE_OFF == cfg.tx_mode)
        *mode = CA_EPON_PORT_LASER_MODE_OFF;
    else if(AAL_EPON_LASER_MODE_ON == cfg.tx_mode)
        *mode = CA_EPON_PORT_LASER_MODE_ON;
    else
        *mode = CA_EPON_PORT_LASER_MODE_BURST;

    return ret;
}

ca_status_t ca_epon_port_fec_enable_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   tx_enable,
    CA_IN ca_boolean_t   rx_enable
)
{
    ca_status_t           ret = CA_E_OK;
    ca_uint8_t onu_type = AAL_ONU_TYPE_1G_10G;
    aal_epon_mac_ctrl_t         ctrl;
    aal_epon_fec_xg_cfg_t        cfg;
    aal_epon_fec_xg_cfg_msk_t    msk;
    aal_epon_fec_cfg_t        fec_1g_cfg;
    aal_epon_fec_msk_t        fec_1g_msk;
    aal_puc_rpt_cfg_msk_t     rpt_msk;
    aal_puc_rpt_cfg_t         rpt_cfg;
    aal_puc_btc_cfg_msk_t     btc_msk;
    aal_puc_btc_cfg_t         btc_cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    memset(&ctrl, 0, sizeof(aal_epon_mac_ctrl_t));
    ret = aal_epon_mac_onu_type_get(device_id, port_id, &onu_type);
    CA_RET_VALUE_CHECK(ret);

    if (onu_type == AAL_ONU_TYPE_1G_1G)
    {
        /* TX 1G, RX 1G */
        memset(&fec_1g_msk, 0, sizeof(fec_1g_msk));
        memset((void *)&fec_1g_cfg, 0, sizeof(fec_1g_cfg));
        fec_1g_msk.s.tx_encoding_ena = 1;
        fec_1g_msk.s.rx_decoding_ena = 1;
        fec_1g_cfg.tx_encoding_ena = tx_enable;
        fec_1g_cfg.rx_decoding_ena = rx_enable;
 
        __PON_LOCK();
        ret = aal_epon_fec_cfg_set(device_id, port_id, fec_1g_msk, &fec_1g_cfg);
        __PON_UNLOCK();
    }
    else if (onu_type == AAL_ONU_TYPE_1G_10G)
    {
        /* TX 1G */
        memset(&fec_1g_msk, 0, sizeof(fec_1g_msk));
        memset((void *)&fec_1g_cfg, 0, sizeof(fec_1g_cfg));
        fec_1g_msk.s.tx_encoding_ena = 1;
        fec_1g_msk.s.rx_decoding_ena = 1;
        fec_1g_cfg.tx_encoding_ena = tx_enable;
        fec_1g_cfg.rx_decoding_ena = 0;
 
        __PON_LOCK();
        ret = aal_epon_fec_cfg_set(device_id, port_id, fec_1g_msk, &fec_1g_cfg);
        __PON_UNLOCK();

        /* RX 10G */
        memset(&msk, 0, sizeof(aal_epon_fec_xg_cfg_msk_t));
        memset(&cfg, 0, sizeof(aal_epon_fec_xg_cfg_t));
        msk.s.rx_mode = 1;
        msk.s.tx_ena  = 1;
        cfg.rx_mode   = (rx_enable == TRUE ? AAL_FEC_XG_RX_MODE_AUTO : AAL_FEC_XG_RX_MODE_OFF);
        cfg.tx_ena    = 0;

        __PON_LOCK();
        ret = aal_epon_fec_xg_cfg_set(device_id, port_id, msk, &cfg);
        __PON_UNLOCK();
    }
    else if (onu_type == AAL_ONU_TYPE_10G_10G)
    {
        /* TX 10G, RX 10G */
        memset(&msk, 0, sizeof(aal_epon_fec_xg_cfg_msk_t));
        memset(&cfg, 0, sizeof(aal_epon_fec_xg_cfg_t));
        msk.s.rx_mode = 1;
        msk.s.tx_ena  = 1;
        cfg.rx_mode   = (rx_enable == TRUE ? AAL_FEC_XG_RX_MODE_AUTO : AAL_FEC_XG_RX_MODE_OFF);
        cfg.tx_ena    = tx_enable;

        __PON_LOCK();
        ret = aal_epon_fec_xg_cfg_set(device_id, port_id, msk, &cfg);
        __PON_UNLOCK();
    }
        CA_RET_VALUE_CHECK(ret);
 
        /* PUC report configuration */
    if (onu_type == AAL_ONU_TYPE_1G_10G)
    {
        memset((void *)&rpt_cfg, 0, sizeof(rpt_cfg));
        rpt_msk.u32 = 0;
 
        rpt_msk.s.rpt1gfecon = 1;
        rpt_cfg.rpt1gfecon   = tx_enable ? 1 : 0;
 
        __PON_LOCK();
        ret = aal_puc_rpt_cfg_set(device_id, rpt_msk, &rpt_cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
 
        memset((void *)&btc_cfg, 0, sizeof(btc_cfg));
        btc_msk.u32 = 0;
 
        btc_msk.s.pyld1gfecon = 1;
        btc_cfg.pyld1gfecon   = tx_enable ? 1 : 0;
 
        __PON_LOCK();
        ret = aal_puc_btc_cfg_set(device_id, btc_msk, &btc_cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}


ca_status_t ca_epon_port_fec_enable_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_boolean_t   *tx_enable,
    CA_OUT ca_boolean_t   *rx_enable
)
{
    ca_status_t           ret = CA_E_OK;
    ca_uint8_t           onu_type = AAL_ONU_TYPE_1G_10G;
    aal_epon_mac_ctrl_t   ctrl;
    aal_epon_fec_xg_cfg_t fec_xg_cfg;
    aal_epon_fec_cfg_t    fec_1g_cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(tx_enable);
    CA_NULL_PTR_CHECK(rx_enable);

    memset(&ctrl, 0, sizeof(aal_epon_mac_ctrl_t));

    __PON_LOCK();
    ret = aal_epon_mac_onu_type_get(device_id, port_id, &onu_type);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    if (onu_type == AAL_ONU_TYPE_1G_1G)
    {
        memset(&fec_1g_cfg, 0, sizeof(fec_1g_cfg));
    __PON_LOCK();
        ret = aal_epon_fec_cfg_get(device_id, port_id, &fec_1g_cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

        *rx_enable = fec_1g_cfg.rx_decoding_ena;
        *tx_enable = fec_1g_cfg.tx_encoding_ena;
    }
    else if (onu_type == AAL_ONU_TYPE_1G_10G)
    {
        memset(&fec_1g_cfg, 0, sizeof(fec_1g_cfg));
        __PON_LOCK();
        ret = aal_epon_fec_cfg_get(device_id, port_id, &fec_1g_cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);

        //*rx_enable = fec_1g_cfg.rx_decoding_ena;
        *tx_enable = fec_1g_cfg.tx_encoding_ena;

        memset(&fec_xg_cfg, 0, sizeof(fec_xg_cfg));
        __PON_LOCK();
        ret = aal_epon_fec_xg_cfg_get(device_id, port_id, &fec_xg_cfg);
        __PON_UNLOCK();
        CA_RET_VALUE_CHECK(ret);

        *rx_enable = (fec_xg_cfg.rx_mode == AAL_FEC_XG_RX_MODE_OFF ? FALSE : TRUE);
        //*tx_enable = fec_xg_cfg.tx_ena;

    }
    else if (onu_type == AAL_ONU_TYPE_10G_10G)
    {
        memset(&fec_xg_cfg, 0, sizeof(fec_xg_cfg));
        __PON_LOCK();
        ret = aal_epon_fec_xg_cfg_get(device_id, port_id, &fec_xg_cfg);
        __PON_UNLOCK();
        CA_RET_VALUE_CHECK(ret);

        *rx_enable = (fec_xg_cfg.rx_mode == AAL_FEC_XG_RX_MODE_OFF ? FALSE : TRUE);
        *tx_enable = fec_xg_cfg.tx_ena;
    }

    return CA_E_OK;
}

ca_status_t ca_epon_llid_encryption_enable_set
(
    CA_IN ca_device_id_t                      device_id,
    CA_IN ca_port_id_t                        port_id,
    CA_IN ca_llid_t                           llid,
    CA_IN ca_epon_port_direction_encryption_t direction,
    CA_IN ca_boolean_t                        enable)
{
    ca_status_t ret = CA_E_OK;
    aal_epon_crypto_llid_t llid_cfg;
    ca_uint32_t pon_mode;
    ca_uint16_t llid_value;
    ca_boolean_t tx_flag = 0;
    ca_boolean_t rx_flag = 0;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    if (direction == CA_EPON_DIRECTION_ENC_TX)
        tx_flag = 1;
    else if (direction == CA_EPON_DIRECTION_ENC_RX)
        rx_flag = 1;
    else if(direction == CA_EPON_DIRECTION_ENC_BI)
    {
        tx_flag = 1;
        rx_flag = 1;
    }
    else
        return CA_E_PARAM;

    pon_mode = aal_pon_mac_mode_get(device_id);
    ca_printf("pon mode is %d\r\n", pon_mode);

    /* 1. set encryption mode, if encryption is enabled, mode should be the same */
    if (pon_mode == ONU_PON_MAC_EPON_1G)
    {
    }
    else if(pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        __PON_LOCK();
        ret = aal_mpcp_reg_llid_get(device_id, port_id, llid, &llid_value);
        __PON_UNLOCK();
        CA_RET_VALUE_CHECK(ret);

        memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));
        llid_cfg.ena = enable;
        llid_cfg.llid = llid_value;

        __PON_LOCK();
        if (tx_flag)
        {
            ca_printf("start to %s llid_idx %d, llid_value %d encryption\r\n", (llid_cfg.ena)?"enable":"disable", llid, llid_value);
            ret = aal_epon_crypto_xg_llid_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_TX, &llid_cfg);
        }

        if (rx_flag)
        {
            ca_printf("start to %s llid_idx %d, llid_value %d decryption\r\n", (llid_cfg.ena)?"enable":"disable", llid, llid_value);
            ret = aal_epon_crypto_xg_llid_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, &llid_cfg);
        }
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

ca_status_t ca_epon_llid_encryption_enable_get
(
    CA_IN  ca_device_id_t                      device_id,
    CA_IN  ca_port_id_t                        port_id,
    CA_IN  ca_llid_t                           llid,
    CA_IN  ca_epon_port_direction_encryption_t direction,
    CA_OUT ca_boolean_t                        *enable
)
{
    ca_status_t ret = CA_E_OK;
    aal_epon_crypto_llid_t llid_cfg;
    ca_uint32_t pon_mode;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(enable);

    pon_mode = aal_pon_mac_mode_get(device_id);

    ca_printf("pon mode is %d\r\n", pon_mode);

    if (pon_mode == ONU_PON_MAC_EPON_1G)
    {
        /* TODO : */
    }
    else if (pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));

        __PON_LOCK();
        if (direction == CA_EPON_DIRECTION_ENC_TX)
            ret = aal_epon_crypto_xg_llid_get(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_TX, &llid_cfg);
        else if (direction == CA_EPON_DIRECTION_ENC_RX)
            ret = aal_epon_crypto_xg_llid_get(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, &llid_cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
        *enable = llid_cfg.ena;
    }

    return CA_E_OK;
}

ca_status_t ca_epon_llid_encryption_active_key_index_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint8_t     active_key_index
)
{
    ca_status_t                    ret = CA_E_OK;
    aal_epon_crypto_xg_tbl_msk_t   msk;
    aal_epon_crypto_xg_tbl_field_t cfg;
    ca_uint32_t pon_mode;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    pon_mode = aal_pon_mac_mode_get(device_id);

    if(pon_mode == ONU_PON_MAC_EPON_1G)
    {
        //
    }
    else if(pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        memset(&msk, 0, sizeof(aal_epon_crypto_xg_tbl_msk_t));
        memset(&cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));

        msk.s.an = 1;
        cfg.an = active_key_index;
        ca_printf("start to set llid_idx %d active_key_index to %d\r\n", llid, cfg.an);

        __PON_LOCK();
        ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_TX, msk, &cfg);
        ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, msk, &cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
    }

    return CA_E_OK;
}

ca_status_t ca_epon_llid_encryption_active_key_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_uint8_t     *active_key_index
)
{
    ca_status_t                    ret = CA_E_OK;
    aal_epon_crypto_xg_tbl_field_t cfg;
    ca_uint32_t pon_mode;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(active_key_index);

    pon_mode = aal_pon_mac_mode_get(device_id);

    if (pon_mode == ONU_PON_MAC_EPON_1G)
    {
        //
    }
    else if(pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        memset(&cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));

        __PON_LOCK();
        ret = aal_epon_crypto_xg_sa_tbl_get(device_id,port_id,(ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, &cfg);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);

        *active_key_index = cfg.an;
    }

    return CA_E_OK;
}

ca_status_t ca_epon_llid_encryption_key_set
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid,
    CA_IN ca_uint32_t                   key_id,
    CA_IN ca_epon_port_encryption_key_t *key
)
{
    ca_status_t                   ret = CA_E_OK;
    aal_epon_crypto_xg_tbl_msk_t     msk;
    aal_epon_crypto_xg_tbl_field_t   cfg;
    aal_epon_crypto_cfg_t config;
    ca_uint32_t key_data[4];
    ca_uint32_t pon_mode;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(key);

    pon_mode = aal_pon_mac_mode_get(device_id);

    memset(&config, 0, sizeof(aal_epon_crypto_cfg_t));

    if (pon_mode == ONU_PON_MAC_EPON_1G)
    {
        __PON_LOCK();
        ret = aal_epon_crypto_1g_cfg_get(device_id, port_id, &config);
        __PON_UNLOCK();

        if(config.rx_mode == AAL_EPON_CRYPTO_1G_MODE_AES_DPOE)
        {
            ca_printf("start to set 1G AES DPOE llid_idx %d, key %d, \r\n", llid, key_id);
            memcpy(&key_data[0], key, sizeof(ca_epon_port_encryption_key_t));
        }
        else
        {
            ca_printf("start to set 1G churning llid_idx %d, key %d, \r\n", llid, key_id);
            key_data[3] = (key->data[0]<<8) | (key->data[1]<<16) | (key->data[2]<<24) ;
            key_data[2] = 0;
            key_data[1] = 0;
            key_data[0] = 0;
            ca_printf("key_data[3] = 0x%x\r\n", key_data[3]);
    }

        __PON_LOCK();
        ret = aal_epon_crypto_1g_key_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, key_id, (aal_epon_crypto_key_t *)key_data);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);
    }
    else if (pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        __PON_LOCK();
        ret = aal_epon_crypto_xg_cfg_get(device_id, port_id, &config);
        __PON_UNLOCK();


	    CA_RET_VALUE_CHECK(ret);

	    memset(&msk, 0, sizeof(aal_epon_crypto_xg_tbl_msk_t));
	    memset(&cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));

	    if(config.rx_mode == AAL_EPON_CRYPTO_XG_MODE_AES_DPOE)
	    {
            ca_printf("start to set 10G AES DPOE llid_idx %d, key %d, \r\n", llid, key_id);
	        if(key_id == 0)
	        {
	            msk.s.key0 = 1;
	            memcpy(&cfg.key0[0], key, sizeof(ca_epon_port_encryption_key_t));
	        }
	        else
	        {
	            msk.s.key1 = 1;
	            memcpy(&cfg.key1[0], key, sizeof(ca_epon_port_encryption_key_t));
	        }

	    }
	    else
	    {
            ca_printf("start to set 10G churning llid_idx %d, key %d, \r\n", llid, key_id);
            /* CTC churning 3 * 3 bytes key -> ASIC key
            level_1 churnig key£º 0 1 2
            level_2 churnig key£º 3 4 5
            level_3 churnig key£º 6 7 8

                        byte3  byte2   byte1   byte0
            key[0] =      5       0       1       2
            key[1] =      7       8       3       4
            key[2] =      x       x       x       6
            */
	        if(key_id == 0)
	        {
	            msk.s.key0 = 1;
	            cfg.key0[3] = 0;
	            cfg.key0[2] = (key->data[6]);
	            cfg.key0[1] = (key->data[7]<<24) | (key->data[8]<<16) | (key->data[3]<<8) | (key->data[4]);
	            cfg.key0[0] = (key->data[5]<<24) | (key->data[0]<<16) | (key->data[1]<<8) | (key->data[2]);
	        }
	        else
	        {
	            msk.s.key1 = 1;
	            cfg.key1[3] = 0;
	            cfg.key1[2] = (key->data[6]);
	            cfg.key1[1] = (key->data[7]<<24) | (key->data[8]<<16) | (key->data[3]<<8) | (key->data[4]);
	            cfg.key1[0] = (key->data[5]<<24) | (key->data[0]<<16) | (key->data[1]<<8) | (key->data[2]);
	        }
	    }

        __PON_LOCK();
	    ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_TX, msk, &cfg);
	    ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port_id, (ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, msk, &cfg);
        __PON_UNLOCK();

	    CA_RET_VALUE_CHECK(ret);
	}
    return ret;

}

ca_status_t ca_epon_llid_encryption_key_get
(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_IN  ca_llid_t                     llid,
    CA_IN  ca_uint32_t                   key_id,
    CA_OUT ca_epon_port_encryption_key_t *key
)
{
    ca_status_t                   ret = CA_E_OK;
    aal_epon_crypto_xg_tbl_field_t   cfg;
    aal_epon_crypto_cfg_t config;
    ca_uint32_t pon_mode;
    ca_uint32_t key_data[4];

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(key);

    pon_mode = aal_pon_mac_mode_get(device_id);

    memset(&config, 0, sizeof(aal_epon_crypto_cfg_t));

    if (pon_mode == ONU_PON_MAC_EPON_1G)
    {
        __PON_LOCK();
        ret = aal_epon_crypto_1g_cfg_get(device_id, port_id, &config);
        __PON_UNLOCK();

        CA_RET_VALUE_CHECK(ret);

        __PON_LOCK();
        ret = aal_epon_crypto_1g_key_get(device_id, port_id, llid, AAL_EPON_CRYPTO_DIRECTION_RX, key_id, (aal_epon_crypto_key_t *)key_data);
        __PON_UNLOCK();

        if (config.rx_mode == AAL_EPON_CRYPTO_1G_MODE_AES_DPOE)
        {
            memcpy(key, &key_data[0], sizeof(ca_epon_port_encryption_key_t));
        }
        else
        {
            memset(key->data, 0, sizeof(key->data));
            key->data[0] = (key_data[3] >>8)&0xff;
            key->data[1] = (key_data[3] >>16)&0xff;
            key->data[2] = (key_data[3] >>24)&0xff;
        }

    }
    else if (pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        __PON_LOCK();
        ret = aal_epon_crypto_xg_cfg_get(device_id, port_id, &config);
        __PON_UNLOCK();

		CA_RET_VALUE_CHECK(ret);

        memset(&cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));

        __PON_LOCK();
        ret = aal_epon_crypto_xg_sa_tbl_get(device_id,port_id,(ca_uint8_t)llid, AAL_EPON_CRYPTO_DIRECTION_RX, &cfg);
        __PON_UNLOCK();
        CA_RET_VALUE_CHECK(ret);

		if(config.rx_mode == AAL_EPON_CRYPTO_XG_MODE_AES_DPOE)
		{
		    if(key_id == 0)
		    {
		        memcpy(key, &cfg.key0[0], sizeof(ca_epon_port_encryption_key_t));
		    }
	        else
	        {
	            memcpy(key, &cfg.key1[0], sizeof(ca_epon_port_encryption_key_t));
	        }
	    }
	    else
	    {
	        if(key_id == 0)
	        {
	            key->data[0] = (cfg.key0[0] >>16)&0xff;
	            key->data[1] = (cfg.key0[0] >>8 )&0xff;
	            key->data[2] = (cfg.key0[0]     )&0xff;
	            key->data[3] = (cfg.key0[1] >>8 )&0xff;
	            key->data[4] = (cfg.key0[1]     )&0xff;
	            key->data[5] = (cfg.key0[0] >>24)&0xff;
	            key->data[6] = (cfg.key0[2]     )&0xff;
	            key->data[7] = (cfg.key0[1] >>24)&0xff;
	            key->data[8] = (cfg.key0[1] >>16)&0xff;
	        }
	        else
	        {
	            key->data[0] = (cfg.key1[0] >>16)&0xff;
	            key->data[1] = (cfg.key1[0] >>8 )&0xff;
	            key->data[2] = (cfg.key1[0]     )&0xff;
	            key->data[3] = (cfg.key1[1] >>8 )&0xff;
	            key->data[4] = (cfg.key1[1]     )&0xff;
	            key->data[5] = (cfg.key1[0] >>24)&0xff;
	            key->data[6] = (cfg.key1[2]     )&0xff;
	            key->data[7] = (cfg.key1[1] >>24)&0xff;
	            key->data[8] = (cfg.key1[1] >>16)&0xff;
	        }
	    }
	}
    return ret;
}

ca_status_t ca_epon_port_speed_set
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN ca_epon_port_speed_t speed
)
{
    ca_status_t       ret = CA_E_OK;
    ca_uint32_t       onu_type;
    aal_epon_mac_ctrl_msk_t msk;
    aal_epon_mac_ctrl_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    if (speed == CA_EPON_PORT_SPEED_1G)
        onu_type = AAL_ONU_TYPE_1G_1G;
    else if (speed == CA_EPON_PORT_SPEED_1G_10G)
        onu_type = AAL_ONU_TYPE_1G_10G;
    else if (speed == CA_EPON_PORT_SPEED_XG)
        onu_type = AAL_ONU_TYPE_10G_10G;
    else
        return CA_E_PARAM;

    memset(&msk, 0, sizeof(aal_epon_mac_ctrl_msk_t));
    memset(&cfg, 0, sizeof(aal_epon_mac_ctrl_t));

    msk.s.onu_type = 1;
    cfg.onu_type = onu_type;

    __PON_LOCK();
    ret = aal_epon_mac_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_port_speed_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT ca_epon_port_speed_t *speed
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t onu_type = AAL_ONU_TYPE_1G_10G;
    aal_epon_mac_ctrl_t ctrl;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(speed);

    memset(&ctrl, 0, sizeof(aal_epon_mac_ctrl_t));
    __PON_LOCK();
    ret = aal_epon_mac_onu_type_get(device_id, port_id, &onu_type);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    if(onu_type == AAL_ONU_TYPE_1G_1G)
    {
        *speed = CA_EPON_PORT_SPEED_1G;
    }else if(onu_type == AAL_ONU_TYPE_1G_10G)
    {
        *speed = CA_EPON_PORT_SPEED_1G_10G;
    }else if(onu_type == AAL_ONU_TYPE_10G_10G)
    {
        *speed = CA_EPON_PORT_SPEED_XG;
    }

    return ret;
}

ca_status_t ca_epon_port_discard_control_set
(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_port_id_t                   port_id,
    CA_IN ca_epon_port_discard_control_t *control
)
{
    ca_status_t     ret = CA_E_OK;
    aal_epon_mac_drop_msk_t        msk;
    aal_epon_mac_drop_t            cfg;
    //ca_boolean_t               en_rx_fwd;
    //ca_boolean_t               en_rx_ext;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(control);

    if (control->mpcp_extention_drop)
        return CA_E_NOT_SUPPORT;

    memset(&msk, 0, sizeof(aal_epon_mac_drop_msk_t));
    memset(&cfg, 0, sizeof(aal_epon_mac_drop_t));

    msk.s.mpcp_crc32_drop = 1;
    msk.s.mpcp_crc8_drop = 1;
    cfg.mpcp_crc32_drop = control->mpcp_crc32_drop;
    cfg.mpcp_crc8_drop = control->mpcp_crc8_drop;

    __PON_LOCK();
    ret = aal_epon_mac_drop_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

#if 0
    if (control->mpcp_extention_drop)
    {
        en_rx_fwd = 0;
        en_rx_ext = 0;
    }
    else
    {
        en_rx_fwd = 1;
        en_rx_ext = 1;
    }

    __PON_LOCK();
    ret = aal_mpcp_rx_ext_ctrl_set(device_id, port_id, en_rx_fwd, en_rx_ext);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);
#endif
    return ret;
}

ca_status_t ca_epon_port_discard_control_get
(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_OUT ca_epon_port_discard_control_t *control)
{
    ca_status_t     ret = CA_E_OK;
    aal_epon_mac_drop_t            cfg;
    ca_boolean_t               en_rx_fwd;
    ca_boolean_t               en_rx_ext;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(control);

    memset(&cfg, 0, sizeof(aal_epon_mac_drop_t));

    __PON_LOCK();
    ret = aal_epon_mac_drop_get(device_id, port_id, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    control->mpcp_crc32_drop = (cfg.mpcp_crc32_drop > 0);
    control->mpcp_crc8_drop = (cfg.mpcp_crc8_drop > 0);

    control->mpcp_extention_drop = CA_FALSE;

    #if 0
    __PON_LOCK();
    ret = aal_mpcp_rx_ext_ctrl_get(device_id, port_id, &en_rx_fwd, &en_rx_ext);
    __PON_UNLOCK();

    if (!en_rx_fwd && !en_rx_ext)
    {
        control->mpcp_extention_drop = 1;
    }
    else
    {
        control->mpcp_extention_drop = 0;
    }

    CA_RET_VALUE_CHECK(ret);
    #endif

    return ret;
}

ca_status_t ca_epon_port_stats_get
(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  ca_port_id_t        port_id,
    CA_IN  ca_boolean_t        read_clear,
    CA_OUT ca_epon_port_stats_t *data)
{
    ca_status_t                    ret = CA_E_OK;
    aal_epon_mac_stats_port_eth_t  stats_port_eth;
    aal_epon_mac_stats_port_mpcp_t stats_port_mpcp;
    aal_epon_mac_stats_port_mpcp_ctrl_t mpcp_mac_ctrl;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(data);

    __PON_LOCK();
    ret = aal_epon_mac_mib_rc_set(device_id, port_id, read_clear);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    memset(&stats_port_eth, 0, sizeof(stats_port_eth));
    memset(&stats_port_mpcp, 0, sizeof(stats_port_mpcp));
    memset(&mpcp_mac_ctrl, 0, sizeof(mpcp_mac_ctrl));

    __PON_LOCK();
    ret = aal_epon_mac_stats_port_eth_get(device_id, port_id, &stats_port_eth);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    __PON_LOCK();
    ret = aal_epon_mac_stats_port_mpcp_get(device_id, port_id, &stats_port_mpcp);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    __PON_LOCK();
    ret = aal_epon_mac_stats_port_mpcp_ctrl_get(device_id, port_id, &mpcp_mac_ctrl);
    __PON_UNLOCK();

    /* MIB MISC */
    data->rx_crc8_sld_err       = 0;
    data->rx_bc_crc8_valid_fcs_err_frames  = stats_port_mpcp.rx_epon_bccrc8vldfcserrcnt;
    data->mpcp_rx_bc_reg            = stats_port_mpcp.rx_epon_bcregstrcnt;
    data->mpcp_rx_bc_gate           = stats_port_mpcp.rx_epon_bcgatecnt;
    data->rx_ext_frames        = stats_port_mpcp.rx_epon_extencnt;
    data->mpcp_tx_bc_reg_req         = stats_port_mpcp.tx_epon_bcregreqcnt;
    data->tx_ext_frames         = stats_port_mpcp.tx_epon_extencnt;

    data->mpcp_rx_mac_ctrl_frames = mpcp_mac_ctrl.mpcp_mac_ctrl_frames_received;
    data->mpcp_tx_mac_ctrl_frames = mpcp_mac_ctrl.mpcp_mac_ctrl_frames_transmitted;
    data->mpcp_tx_reg_ack = mpcp_mac_ctrl.mpcp_tx_reg_ack;

    /* MIB RX */
    // TODO: 64bits VS 32 bits
    //data->rx_good_octets       = ((ca_uint64_t)stats_port_eth.rx_bytecnt_hi << 32) + ((ca_uint64_t)stats_port_eth.rx_bytecnt_lo);
    data->rx_good_octets       = ca_inter_to64(stats_port_eth.rx_bytecnt_hi, stats_port_eth.rx_bytecnt_lo);
    data->rx_uc_frames         = stats_port_eth.rx_ucframecnt;
    data->rx_mc_frames         = stats_port_eth.rx_mcframecnt;
    data->rx_bc_frames         = stats_port_eth.rx_bcframecnt;
    data->rx_fcs_error_frames  = stats_port_eth.rx_crcerrcnt;
    data->rx_oam_frames        = stats_port_eth.rx_oamframecnt;
    data->rx_pause_frames      = stats_port_eth.rx_pausecnt;
    data->rx_unknown_oframes   = stats_port_eth.rx_unknownopcnt;
    data->rx_runt_frames       = stats_port_eth.rx_undersizecnt;
    data->rx_oversize_frames      = stats_port_eth.rx_oversizecnt;

    data->rx_64_frames         = stats_port_eth.rx_rmon64cnt;
    data->rx_65_127_frames     = stats_port_eth.rx_rmon65_127cnt;
    data->rx_128_255_frames    = stats_port_eth.rx_rmon128_255cnt;
    data->rx_256_511_frames    = stats_port_eth.rx_rmon256_511cnt;
    data->rx_512_1023_frames   = stats_port_eth.rx_rmon512_1023cnt;
    data->rx_1024_1518_frames  = stats_port_eth.rx_rmon1024_1518cnt;
    data->rx_1519_max_frames   = stats_port_eth.rx_rmon1519_1533cnt + stats_port_eth.rx_rmon1534_2299cnt +
    stats_port_eth.rx_rmon2300_2599cnt + stats_port_eth.rx_rmon2600_maxcnt;

    /* MIB TX */
    //data->tx_good_octets        = ((ca_uint64_t)stats_port_eth.tx_bytecnt_hi << 32) + ((ca_uint64_t)stats_port_eth.tx_bytecnt_lo);
        data->tx_good_octets       = ca_inter_to64(stats_port_eth.tx_bytecnt_hi, stats_port_eth.tx_bytecnt_lo);
        data->tx_uc_frames          = stats_port_eth.tx_ucframecnt;
        data->tx_mc_frames          = stats_port_eth.tx_mcframecnt;
        data->tx_bc_frames          = stats_port_eth.tx_bcframecnt;
        data->tx_oam_frames         = stats_port_eth.tx_oamframecnt;
        data->tx_pause_frames       = stats_port_eth.tx_pausecnt;

        data->tx_64_frames          = stats_port_eth.tx_rmon64cnt;
        data->tx_65_127_frames      = stats_port_eth.tx_rmon65_127cnt;
        data->tx_128_255_frames     = stats_port_eth.tx_rmon128_255cnt;
        data->tx_256_511_frames     = stats_port_eth.tx_rmon256_511cnt;
        data->tx_512_1023_frames    = stats_port_eth.tx_rmon512_1023cnt;
        data->tx_1024_1518_frames   = stats_port_eth.tx_rmon1024_1518cnt;
    data->tx_1519_max_frames    = stats_port_eth.tx_rmon1519_1533cnt + stats_port_eth.tx_rmon1534_2299cnt +
        stats_port_eth.tx_rmon2300_2599cnt + stats_port_eth.tx_rmon2600_maxcnt;

    return ret;
}


ca_status_t ca_epon_port_fec_stats_get
(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_IN  ca_boolean_t             read_clear,
    CA_OUT ca_epon_port_fec_stats_t *data)
{
    ca_status_t   ret = CA_E_OK;
    aal_epon_fec_stats_t stats;
    aal_epon_fec_cfg_t        fec_1g_cfg;
    aal_epon_fec_msk_t        fec_1g_msk;
    ca_uint8_t onu_type = AAL_ONU_TYPE_1G_10G;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(data);

    ret = aal_epon_mac_onu_type_get(device_id, port_id, &onu_type);
    CA_RET_VALUE_CHECK(ret);

    if (onu_type == AAL_ONU_TYPE_1G_1G)
    {
        memset(&fec_1g_msk, 0, sizeof(fec_1g_msk));
        memset((void *)&fec_1g_cfg, 0, sizeof(fec_1g_cfg));
        fec_1g_msk.s.cnt_clr = 1;
        fec_1g_cfg.cnt_clr = read_clear;

        __PON_LOCK();
        ret = aal_epon_fec_cfg_set(device_id, port_id, fec_1g_msk, &fec_1g_cfg);
        ret = aal_epon_fec_stats_get(device_id, port_id, &stats);
        __PON_UNLOCK();

    }else{
    __PON_LOCK();
    ret = aal_epon_fec_xg_stats_get(device_id, port_id, &stats);
    __PON_UNLOCK();
    }

    CA_RET_VALUE_CHECK(ret);

    data->corrected_bytes = stats.rx_corr_bytes;
    data->corrected_codewords= stats.rx_corr_codeword;
    data->uncorrectable_codewords = stats.rx_uncorr_codeword;
    data->total_codewords = stats.rx_total_codeword;
    data->good_pkt = stats.rx_good_pkt;
    data->err_pkt = stats.rx_err_pkt;
    data->invalid_blocks_err = stats.rx_err_blocks;

    return ret;
}

ca_status_t ca_epon_port_status_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT ca_epon_port_status_t *status
)
{
    aal_epon_status_t epon_status;
    aal_epon_opt_status_t opt_status;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(status);

    memset((void *)&epon_status, 0, sizeof(epon_status));
    memset((void *)&opt_status,  0, sizeof(opt_status));

    __PON_LOCK();
    (void)aal_epon_status_get(0, 0, &epon_status);
    (void)aal_epon_opt_status_get(0, 0, &opt_status);
    __PON_UNLOCK();

    if ((!epon_status.pcs_sync_xg && (epon_status.speed > AAL_EPON_SPEED_MODE_DS_1G_US_1G))
       || (!epon_status.pcs_sync_1g && (epon_status.speed <= AAL_EPON_SPEED_MODE_DS_1G_US_1G)))
    {
        status->xgepon_linkok = CA_FALSE;
    }
    else
    {
        status->xgepon_linkok = CA_TRUE;
    }

    status->opt_rx_los   = opt_status.opt_rx_los;
    status->opt_mod_abs  = opt_status.opt_mod_abs ;
    status->opt_tx_sd    = opt_status.opt_tx_sd;
    status->opt_tx_fault = opt_status.opt_tx_fault;

    return CA_E_OK;
}

ca_status_t ca_epon_port_link_status_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_boolean_t   *link
)
{
    ca_status_t ret = CA_E_OK;
    aal_epon_status_t status;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(link);

    __PON_LOCK();
    //ret = aal_epon_mac_xg_link_status_get(device_id, port_id, link);
    (void)aal_epon_status_get(0, 0, &status);
    __PON_UNLOCK();

    if ((!status.pcs_sync_xg && (status.speed > AAL_EPON_SPEED_MODE_DS_1G_US_1G))
            || (!status.pcs_sync_1g && (status.speed <= AAL_EPON_SPEED_MODE_DS_1G_US_1G)))
    {
        *link = FALSE;
    }else
    {
        *link = TRUE;
    }
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_registration_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_boolean_t   enable
)
{
    ca_status_t             ret = CA_E_OK;
    aal_mpcp_llid_msk_t     msk;
    aal_mpcp_llid_cfg_t     cfg;
    aal_mpcp_llid_entry_t 	entry;
    ca_uint16_t llid_value;
    ca_uint32_t llid_bmp = 0;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    memset(&msk, 0, sizeof(aal_mpcp_llid_msk_t));
    memset(&cfg, 0, sizeof(aal_mpcp_llid_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_get(device_id, port_id, (ca_uint8_t)llid, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    msk.s.offline = 1;
    cfg.offline = (enable)?0:1;

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_set(device_id, port_id, (ca_uint8_t)llid, msk, &cfg);
    __PON_UNLOCK();

    if (enable)
    {
        //ret = aal_mpcp_reg_start_set(device_id, port_id, (ca_uint8_t)llid, (ca_uint8_t)llid);
        llid_bmp = 1<<llid;
        __PON_LOCK();
        aal_mpcp_reg_llid_add(0, llid_bmp);
        __PON_UNLOCK();
    }
    else
    {
        llid_bmp = 1<<llid;
        __PON_LOCK();
        aal_mpcp_reg_llid_del(0, llid_bmp);
        __PON_UNLOCK();

        __PON_LOCK();
        ret = aal_mpcp_reg_llid_get(device_id, port_id, llid, &llid_value);
        __PON_UNLOCK();
        CA_RET_VALUE_CHECK(ret);

        memset(&entry, 0, sizeof(aal_mpcp_llid_entry_t));
        entry.llid_idx = llid;
        entry.mac_idx = llid;
        entry.llid_value = llid_value;
        entry.mcast = 0;

        __PON_LOCK();
        ret = aal_mpcp_llid_entry_set(device_id, port_id, AAL_MPCP_LLID_ENTRY_DEL, &entry);
        __PON_UNLOCK();

    }
    CA_RET_VALUE_CHECK(ret);

    return ret;
}


ca_status_t ca_epon_mpcp_registration_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_boolean_t   *enable
)
{
    ca_status_t           ret = CA_E_OK;
    aal_mpcp_llid_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(enable);

    memset(&cfg, 0, sizeof(aal_mpcp_llid_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_get(device_id, port_id, (ca_uint8_t)llid, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    *enable = (cfg.offline) ? 0 : 1;

    return ret;
}

ca_status_t ca_epon_mpcp_pending_grants_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    pending_grants
)
{
    ca_status_t           ret = CA_E_OK;
    aal_mpcp_msk_t      msk;
    aal_mpcp_cfg_t      cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    if(pending_grants < CA_EPON_MPCP_PENDING_GRANTS_MIN ||
        pending_grants > CA_EPON_MPCP_PENDING_GRANTS_MAX)
    {
        ca_printf("the value of pending_grants (%d) is invalid. Range is 1~8.\r\n", pending_grants);
        return CA_E_PARAM;
    }

    memset(&msk, 0, sizeof(aal_mpcp_msk_t));
    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    msk.s.pending_grant = 1;
    cfg.pending_grant = pending_grants;

    __PON_LOCK();
    ret = aal_mpcp_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_pending_grants_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *pending_grants
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(pending_grants);

    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *pending_grants = cfg.pending_grant;

    return ret;
}

ca_status_t ca_epon_mpcp_timer_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    mpcp_timer,
    CA_IN ca_boolean_t   deregistration
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_msk_t msk;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_BOOL_VAL_CHECK(deregistration);

    memset(&msk, 0, sizeof(aal_mpcp_msk_t));
    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    msk.s.mpcptimer_threshold = 1;
    msk.s.mpcptimer_dereg_en = 1;
    cfg.mpcptimer_threshold = mpcp_timer;
    cfg.mpcptimer_dereg_en = deregistration;

    __PON_LOCK();
    ret = aal_mpcp_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_timer_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *mpcp_timer,
    CA_OUT ca_boolean_t   *deregistration
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(mpcp_timer);
    CA_NULL_PTR_CHECK(deregistration);

    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *mpcp_timer = cfg.mpcptimer_threshold;
    *deregistration = cfg.mpcptimer_dereg_en;

    return ret;
}

ca_status_t ca_epon_mpcp_gate_timer_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    gate_timer,
    CA_IN ca_boolean_t   deregistration
)
{
    ca_status_t           ret = CA_E_OK;
    aal_mpcp_msk_t      msk;
    aal_mpcp_cfg_t      cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_BOOL_VAL_CHECK(deregistration);

    memset(&msk, 0, sizeof(aal_mpcp_msk_t));
    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    msk.s.gatetimer_threshold = 1;
    msk.s.gatetimer_dereg_en = 1;
    cfg.gatetimer_threshold = gate_timer;
    cfg.gatetimer_dereg_en = deregistration;

    __PON_LOCK();
    ret = aal_mpcp_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_gate_timer_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *gate_timer,
    CA_OUT ca_boolean_t   *deregistration
)
{
    ca_status_t           ret = CA_E_OK;
    aal_mpcp_cfg_t      cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(gate_timer);
    CA_NULL_PTR_CHECK(deregistration);

    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *gate_timer = cfg.gatetimer_threshold;
    *deregistration = cfg.gatetimer_dereg_en;

     return ret;
}

ca_status_t ca_epon_mpcp_timestamp_drift_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    threshold,
    CA_IN ca_boolean_t   deregistration
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_msk_t msk;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    if ((threshold != AAL_MPCP_TIMEDRIFT_THRESHOLD_8TQ)
        && (threshold != AAL_MPCP_TIMEDRIFT_THRESHOLD_16TQ)
        && (threshold != AAL_MPCP_TIMEDRIFT_THRESHOLD_32TQ)
        && (threshold != AAL_MPCP_TIMEDRIFT_THRESHOLD_63TQ))
        return CA_E_PARAM;
    CA_BOOL_VAL_CHECK(deregistration);

    memset(&msk, 0, sizeof(msk));
    memset(&cfg, 0, sizeof(cfg));

    msk.s.timedrift_threshold = 1;
    msk.s.timedrift_dereg_en  = 1;
    cfg.timedrift_threshold   = threshold;
    cfg.timedrift_dereg_en    = deregistration;

    __PON_LOCK();
    ret = aal_mpcp_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_timestamp_drift_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *threshold,
    CA_OUT ca_boolean_t   *deregistration
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(threshold);
    CA_NULL_PTR_CHECK(deregistration);

    memset(&cfg, 0, sizeof(cfg));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *threshold = cfg.timedrift_threshold;
    *deregistration = cfg.timedrift_dereg_en;

    return ret;
}

ca_status_t ca_epon_mpcp_keep_silence_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_boolean_t   silence_enable,
    CA_IN ca_uint32_t    silence_time
)
{
    ca_status_t             ret = CA_E_OK;
    aal_mpcp_llid_msk_t     llid_msk;
    aal_mpcp_llid_cfg_t     llid_cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    memset(&llid_msk, 0, sizeof(llid_msk));
    memset(&llid_cfg, 0, sizeof(llid_cfg));

    llid_msk.s.keep_silence_en = 1;
    llid_msk.s.keep_silence_time = 1;
    llid_cfg.keep_silence_en = silence_enable;
    llid_cfg.keep_silence_time = silence_time;

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_set(device_id, port_id, (ca_uint8_t)llid, llid_msk, &llid_cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_keep_silence_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_boolean_t   *silence_enable,
    CA_OUT ca_uint32_t    *silence_time
)
{
    ca_status_t         ret = CA_E_OK;
    aal_mpcp_llid_cfg_t llid_cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(silence_enable);
    CA_NULL_PTR_CHECK(silence_time);

    memset(&llid_cfg, 0, sizeof(llid_cfg));

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_get(device_id, port_id, (ca_uint8_t)llid, &llid_cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *silence_enable = llid_cfg.keep_silence_en;
    *silence_time = llid_cfg.keep_silence_time;

    return ret;
}

ca_status_t ca_epon_queue_set_thresholds_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint32_t    threshold1,
    CA_IN ca_uint32_t    threshold2,
    CA_IN ca_uint32_t    threshold3,
    CA_IN ca_uint32_t    threshold4
)
{
    #if 0
    aal_mpcp_llid_msk_t llid_msk;
    aal_mpcp_llid_cfg_t llid_cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    /* queue id etc parameters validation */

    // TODO: not support per llid
    memset(&llid_msk, 0, sizeof(aal_mpcp_llid_msk_t));
    memset(&llid_cfg, 0, sizeof(aal_mpcp_llid_cfg_t));

    llid_msk.s.queue_id = 1;
    llid_cfg.queue_id = queue_id;

    __PON_LOCK();
    ret = aal_mpcp_llid_cfg_set(device_id, port_id, (ca_uint8_t)llid, llid_msk, &llid_cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    __PON_LOCK();
    ret = aal_mpcp_report_threshold_set(device_id, port_id, queue_set_id, queue_id, threshold);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);
    #endif
    ca_status_t ret = CA_E_OK;

    aal_puc_epon_que_thrsd12_cfg_t     cfg12;
    aal_puc_epon_que_thrsd12_cfg_msk_t cfg12_msk;
    aal_puc_epon_que_thrsd34_cfg_t     cfg34;
    aal_puc_epon_que_thrsd34_cfg_msk_t cfg34_msk;

    memset((void *)&cfg12, 0, sizeof(cfg12));
    memset((void *)&cfg34, 0, sizeof(cfg34));
    cfg12_msk.u32 = 0;
    cfg34_msk.u32 = 0;

    cfg12_msk.s.qs1th = 1;
    cfg12_msk.s.qs2th = 1;
    cfg12.qs1th = threshold1;
    cfg12.qs2th = threshold2;
    (void)aal_puc_epon_queue_thrshld12_set(device_id, cfg12_msk, &cfg12);

    cfg34_msk.s.qs3th = 1;
    cfg34_msk.s.qs4th = 1;
    cfg34.qs3th = threshold3;
    cfg34.qs4th = threshold4;
    (void)aal_puc_epon_queue_thrshld34_set(device_id, cfg34_msk, &cfg34);

    return ret;
}

ca_status_t ca_epon_queue_set_thresholds_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_uint32_t    *threshold1,
    CA_OUT ca_uint32_t    *threshold2,
    CA_OUT ca_uint32_t    *threshold3,
    CA_OUT ca_uint32_t    *threshold4

)
{
    aal_puc_epon_que_thrsd12_cfg_t cfg12;
    aal_puc_epon_que_thrsd34_cfg_t cfg34;
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(threshold1);
    CA_NULL_PTR_CHECK(threshold2);
    CA_NULL_PTR_CHECK(threshold3);
    CA_NULL_PTR_CHECK(threshold4);

    memset((void *)&cfg12, 0, sizeof(cfg12));
    memset((void *)&cfg34, 0, sizeof(cfg34));

    (void)aal_puc_epon_queue_thrshld12_get(device_id, &cfg12);
    (void)aal_puc_epon_queue_thrshld34_get(device_id, &cfg34);

    *threshold1 = cfg12.qs1th;
    *threshold2 = cfg12.qs2th;
    *threshold3 = cfg34.qs3th;
    *threshold4 = cfg34.qs4th;

    return ret;
}

ca_status_t ca_epon_mpcp_info_get
(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  ca_port_id_t        port_id,
    CA_OUT ca_epon_mpcp_info_t *info
)
{
    ca_status_t ret = CA_E_OK;
    aal_mpcp_cfg_t mpcp_cfg;
    ca_uint8_t i = 0;
    ca_uint16_t llid = 0;
    ca_uint8_t  llid_idx = 0;
    ca_uint16_t sync_time = 0;
    ca_uint16_t onu_disc_info = 0;
    ca_uint16_t olt_disc_info = 0;
    ca_boolean_t reg_flag = CA_FALSE;
    ca_uint8_t report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid_idx);
    CA_NULL_PTR_CHECK(info);

    memset(info, 0, sizeof(*info));
    memset(&mpcp_cfg, 0, sizeof(mpcp_cfg));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &mpcp_cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    info->llid_mode   = mpcp_cfg.llid_mode;
    info->laser_on_time  = mpcp_cfg.laser_on_time;
    info->laser_off_time = mpcp_cfg.laser_off_time;

    __PON_LOCK();
    AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_SYNCTIME_CFG, sync_time_dis, sync_time);
    __PON_UNLOCK();
    info->sync_time = sync_time & 0x7ff;

    __PON_LOCK();
    (void)aal_mpcp_local_discovery_info_get(device_id, port_id, &onu_disc_info);
    __PON_UNLOCK();

    info->onu_discovery_info = onu_disc_info;

    __PON_LOCK();
    (void)aal_mpcp_local_remote_info_get(device_id, port_id, &olt_disc_info);
    __PON_UNLOCK();
    info->olt_discovery_info = olt_disc_info;

    ret = scfg_read(0, CFG_ID_PON_REPORT_MODE, sizeof(report_mode), &report_mode);
    if (ret != CA_E_OK)
        report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;

    info->report_mode = report_mode;

    if (mpcp_cfg.llid_mode)
    {
        for (i=0; i<AAL_EPON_LLID_INDEX_MAX; i++)
        {
            llid = 0;

            __PON_LOCK();
            (void)aal_mpcp_reg_status_get(device_id, port_id, i, &reg_flag);
            __PON_UNLOCK();

            if (reg_flag)
            {
                __PON_LOCK();
                ret = aal_mpcp_reg_llid_get(device_id, port_id, i, &llid);
                __PON_UNLOCK();
                if (ret == CA_E_OK)
            {
                    info->llid[i].llid_index = i;
                    info->llid[i].mpcp_llid  = llid;
                }
                else
                {
                    info->llid[i].llid_index = i;
                    info->llid[i].mpcp_llid  = AAL_MPCP_INVALID_LLID;
                }
            }
            else
            {
                info->llid[i].llid_index = i;
                info->llid[i].mpcp_llid  = AAL_MPCP_INVALID_LLID;  /* invalid */
            }
        }
    }
    else
    {
        /* single LLID, entry 0 is used always */
        llid_idx = 0;
        __PON_LOCK();
        (void)aal_mpcp_reg_status_get(device_id, port_id, llid_idx, &reg_flag);
        __PON_UNLOCK();
        if (!reg_flag)
        {
            info->llid[0].llid_index = 0;
            info->llid[0].mpcp_llid  = AAL_MPCP_INVALID_LLID;
        }
        else
        {
        __PON_LOCK();
        ret = aal_mpcp_reg_llid_get(device_id, port_id, llid_idx, &llid);
        __PON_UNLOCK();
            if (ret == CA_E_OK)
            {
                info->llid[0].llid_index = 0;
                info->llid[0].mpcp_llid  = llid;
            }
            else
            {
                info->llid[0].llid_index = 0;
                info->llid[0].mpcp_llid  = AAL_MPCP_INVALID_LLID;
            }

            for (i=1; i<AAL_EPON_LLID_INDEX_MAX; i++)
            {
                info->llid[i].llid_index = i;
                info->llid[i].mpcp_llid  = AAL_MPCP_INVALID_LLID;
            }
        }
    }

    return CA_E_OK;
}

ca_status_t ca_epon_mpcp_report_mode_set
(
    CA_IN ca_device_id_t			 device_id,
    CA_IN ca_port_id_t				 port_id,
    CA_IN ca_epon_mpcp_report_mode_t report_mode
)
{
    ca_status_t         ret = CA_E_OK;
    aal_mpcp_msk_t      msk;
    aal_mpcp_cfg_t      cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);

    if(report_mode > CA_EPON_MPCP_REPORT_MODE_4QS_1Q)
    {
        return CA_E_PARAM;
    }

    memset(&msk, 0, sizeof(aal_mpcp_msk_t));
    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    msk.s.mpcp_report_mode = 1;
    cfg.mpcp_report_mode = report_mode;

    __PON_LOCK();
    ret = aal_mpcp_cfg_set(device_id, port_id, msk, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_report_mode_get
(
    CA_IN  ca_device_id_t             device_id,
    CA_IN  ca_port_id_t               port_id,
    CA_OUT ca_epon_mpcp_report_mode_t *report_mode
)
{
    ca_status_t    ret = CA_E_OK;
    aal_mpcp_cfg_t cfg;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(report_mode);

    memset(&cfg, 0, sizeof(aal_mpcp_cfg_t));

    __PON_LOCK();
    ret = aal_mpcp_cfg_get(device_id, port_id, &cfg);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    *report_mode = cfg.mpcp_report_mode;

    return ret;
}

ca_status_t ca_epon_mpcp_registration_status_get
(
    CA_IN  ca_device_id_t                     device_id,
    CA_IN  ca_port_id_t                       port_id,
    CA_IN  ca_llid_t                          llid,
    CA_OUT ca_epon_mpcp_registration_status_t *status)
{
    ca_mac_addr_t olt_mac;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t cause;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(status);

    memset(status, 0, sizeof(*status));

    /* get OLT MAC address */
    memset((void *)&olt_mac, 0, sizeof(olt_mac));
    __PON_LOCK();
    aal_mpcp_olt_mac_addr_get(0, 0, olt_mac);
    __PON_UNLOCK();
    CA_RET_VALUE_CHECK(ret);

    memcpy((void *)&status->olt_mac_addr, (void *)&olt_mac, sizeof(olt_mac));

    /* get status etc. */
    __PON_LOCK();
    ret = aal_mpcp_reg_llid_get(device_id, port_id, (ca_uint8_t)llid, (ca_uint16_t*)&(status->mpcp_llid));
    __PON_UNLOCK();

    __PON_LOCK();
    ret = aal_mpcp_reg_status_get(device_id, port_id, (ca_uint8_t)llid, (ca_boolean_t*)&(status->reg_state));
    __PON_UNLOCK();

    // TODO: dereg_cause
    /*cause: the register was read clear by __dereg_cause_show when ONU deregistered. bug 40751*/
    __PON_LOCK();
    ret = aal_mpcp_dereg_cause_get(device_id, port_id, (ca_uint8_t)llid, &cause);
    __PON_UNLOCK();

    switch (cause)
    {
    case AAL_MPCP_DEREG_UNKNOWN:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_NO_DEREGISTER;
        break;
    case AAL_MPCP_DEREG_TIMEDRFT:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_TIMESTAMPDRIPT;
        break;
    case AAL_MPCP_DEREG_OLT:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_OLT_DEREGISTER;
        break;
    case AAL_MPCP_DEREG_LOCAL:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_ONU_DEREGISTER;
        break;
    case AAL_MPCP_DEREG_REREG:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_RX_REREGISTER;
        break;
    case AAL_MPCP_DEREG_TIMEOUT:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_MPCP_TIMEOUT;
        break;
    case AAL_MPCP_DEREG_GATE:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_GATE_TIMEOUT;
        break;
    case AAL_MPCP_DEREG_SW:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_SW_DEREGISTER;
        break;
    default:
        status->deregistration_cause = CA_EPON_MPCP_DEREG_CAUSE_NO_DEREGISTER;
        break;
    }

    __PON_LOCK();
    ret = aal_mpcp_reg_retry_times_get(device_id, port_id, (ca_uint8_t)llid, (ca_uint16_t *)&(status->retry_times));
    __PON_UNLOCK();

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_mpcp_stats_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_llid_t            llid,
    CA_IN  ca_boolean_t         read_clear,
    CA_OUT ca_epon_mpcp_stats_t *data
)
{
    ca_status_t              ret = CA_E_OK;
    aal_mpcp_mib_stats_t   mpcp_mib;
    aal_epon_mac_stats_llid_mpcp_t stats_llid_mpcp;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    CA_NULL_PTR_CHECK(data);

    __PON_LOCK();
    ret = aal_epon_mac_mib_rc_set(device_id, port_id, read_clear);
    __PON_UNLOCK();

    memset(&mpcp_mib, 0, sizeof(aal_mpcp_mib_stats_t));
    memset(&stats_llid_mpcp, 0, sizeof(stats_llid_mpcp));

    __PON_LOCK();
    ret = aal_mpcp_mib_stats_get(device_id, port_id, &mpcp_mib);
    __PON_UNLOCK();

    __PON_LOCK();
    ret = aal_epon_mac_stats_llid_mpcp_get(device_id, port_id, llid, &stats_llid_mpcp);
    __PON_UNLOCK();

    data->rx_mpcp_frames       = mpcp_mib.mpcp_pkt_rx;
    data->rx_ext_mpcp_frames = 0;
    data->rx_discovery_frames_drop = 0;
    data->rx_ext_mpcp_frames_drop = 0;
    data->rx_mpcp_gate = stats_llid_mpcp.rx_epon_ucgatecnt;
    data->rx_mpcp_register = stats_llid_mpcp.rx_epon_regstrcnt;
    memcpy(data->rx_grant_frames_drops, mpcp_mib.grant_rx_drop, CA_MPCGRANT_FRAME_DROP_MAX * sizeof(data->rx_grant_frames_drops[0]));

    data->tx_mpcp_frames       = mpcp_mib.mpcp_pkt_tx;
    data->tx_ext_mpcp_frames = 0;
    data->tx_mpcp_report = stats_llid_mpcp.tx_epon_reportcnt;
    data->tx_mpcp_reg_request = stats_llid_mpcp.tx_epon_regreqcnt;
    data->tx_mpcp_reg_ack = stats_llid_mpcp.tx_epon_regackcnt;

    return ret;
}

ca_status_t ca_epon_llid_stats_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_llid_t            llid,
    CA_IN  ca_boolean_t         read_clear,
    CA_OUT ca_epon_llid_stats_t *data
)
{
    ca_status_t                    ret = CA_E_OK;
    aal_epon_mac_stats_llid_mpcp_t stats_llid_mpcp;
    aal_epon_mac_stats_llid_eth_t  stats_llid_eth;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(data);

    __PON_LOCK();
    ret = aal_epon_mac_mib_rc_set(device_id, port_id, read_clear);
    __PON_UNLOCK();

    memset(&stats_llid_mpcp, 0, sizeof(stats_llid_mpcp));
    memset(&stats_llid_eth, 0, sizeof(stats_llid_eth));

    __PON_LOCK();
    ret = aal_epon_mac_stats_llid_mpcp_get(device_id, port_id, llid, &stats_llid_mpcp);
    __PON_UNLOCK();

    __PON_LOCK();
    ret = aal_epon_mac_stats_llid_eth_get(device_id, port_id, llid, &stats_llid_eth);
    __PON_UNLOCK();

    /* MIB RX */
    //data->rx_good_octets   = ((ca_uint64_t)(stats_llid_eth.rx_epon_bytecnt_hi)<<32) + ((ca_uint64_t)(stats_llid_eth.rx_epon_bytecnt_lo));
    data->rx_good_octets   = ca_inter_to64(stats_llid_eth.rx_epon_bytecnt_hi, stats_llid_eth.rx_epon_bytecnt_lo);
    /* ramecnt is total counter which includes OAM frames, so unicast should be ramcnt - mc - bc - oam */
    data->rx_uc_frames  = stats_llid_eth.rx_epon_ramecnt - stats_llid_eth.rx_epon_mcframecnt - stats_llid_eth.rx_epon_bcframecnt - stats_llid_mpcp.rx_epon_oamfrmcnt;
    data->rx_mc_frames  = stats_llid_eth.rx_epon_mcframecnt;
    data->rx_bc_frames  = stats_llid_eth.rx_epon_bcframecnt;
    data->rx_crc8_valid_crc32_bad_frames = stats_llid_eth.rx_epon_fcscnt;
    data->rx_oam_frames = stats_llid_mpcp.rx_epon_oamfrmcnt;
    //data->rx_mpcp_gate     = stats_llid_mpcp.rx_epon_ucgatecnt;
    //data->rx_mpcp_register = stats_llid_mpcp.rx_epon_regstrcnt;

    data->rx_64_frames        = stats_llid_eth.rx_epon_rmon64cnt;
    data->rx_65_127_frames    = stats_llid_eth.rx_epon_rmon65_127cnt;
    data->rx_128_255_frames   = stats_llid_eth.rx_epon_rmon128_255cnt;
    data->rx_256_511_frames   = stats_llid_eth.rx_epon_rmon256_511cnt;
    data->rx_512_1023_frames  = stats_llid_eth.rx_epon_rmon512_1023cnt;
    data->rx_1024_1518_frames = stats_llid_eth.rx_epon_rmon1024_1518cnt;
    data->rx_1519_max_frames  = stats_llid_eth.rx_epon_rmon1519_1533cnt + stats_llid_eth.rx_epon_rmon1534_2299cnt + stats_llid_eth.rx_epon_rmon2300_2599cnt + stats_llid_eth.rx_epon_rmon2600_maxcnt;
    data->rx_total_count = data->rx_64_frames
                           + data->rx_65_127_frames
                           + data->rx_128_255_frames
                           + data->rx_256_511_frames
                           + data->rx_512_1023_frames
                           + data->rx_1024_1518_frames
                           + data->rx_1519_max_frames;

    /* MIB TX */
    //data->tx_good_octets   = ((ca_uint64_t)(stats_llid_eth.tx_epon_bytecnt_hi)<<32) + ((ca_uint64_t)(stats_llid_eth.tx_epon_bytecnt_lo));
    data->tx_good_octets   = ca_inter_to64(stats_llid_eth.tx_epon_bytecnt_hi, stats_llid_eth.tx_epon_bytecnt_lo);
    /* ramecnt is total counter which includes OAM frames, so unicast should be ramcnt - mc - bc - oam */
    data->tx_uc_frames  = stats_llid_eth.tx_epon_ramecnt - stats_llid_eth.tx_epon_mcframecnt - stats_llid_eth.tx_epon_bcframecnt - stats_llid_mpcp.tx_epon_oamfrmcnt;
    data->tx_mc_frames  = stats_llid_eth.tx_epon_mcframecnt;
    data->tx_bc_frames  = stats_llid_eth.tx_epon_bcframecnt;
    data->tx_oam_frames    = stats_llid_mpcp.tx_epon_oamfrmcnt;
    //data->tx_mpcp_report      = stats_llid_mpcp.tx_epon_reportcnt;
    //data->tx_mpcp_reg_request = stats_llid_mpcp.tx_epon_regreqcnt;
    //data->tx_mpcp_reg_ack     = stats_llid_mpcp.tx_epon_regackcnt;

    data->tx_64_frames        = stats_llid_eth.tx_epon_rmon64cnt;
    data->tx_65_127_frames    = stats_llid_eth.tx_epon_rmon65_127cnt;
    data->tx_128_255_frames   = stats_llid_eth.tx_epon_rmon128_255cnt;
    data->tx_256_511_frames   = stats_llid_eth.tx_epon_rmon256_511cnt;
    data->tx_512_1023_frames  = stats_llid_eth.tx_epon_rmon512_1023cnt;
    data->tx_1024_1518_frames = stats_llid_eth.tx_epon_rmon1024_1518cnt;
    data->tx_1519_max_frames  = stats_llid_eth.tx_epon_rmon1519_1533cnt + stats_llid_eth.tx_epon_rmon1534_2299cnt + stats_llid_eth.tx_epon_rmon2300_2599cnt + stats_llid_eth.tx_epon_rmon2600_maxcnt;
    data->tx_total_count = data->tx_64_frames
                           + data->tx_65_127_frames
                           + data->tx_128_255_frames
                           + data->tx_256_511_frames
                           + data->tx_512_1023_frames
                           + data->tx_1024_1518_frames
                           + data->tx_1519_max_frames;


    return ret;
}


/* mpcp llid to llid instance mapping, return value 0xff means invalid entry id */
ca_uint8_t ca_epon_mpcp_llid_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid
)
{
    return aal_mpcp_llid_index_get(device_id, port_id, llid);
}

#if 0
ca_status_t ca_epon_powersave_protocol_set(
CA_IN       ca_device_id_t                     device_id,
CA_IN       ca_epon_powersave_protocol_t    protocol)
{
    ca_status_t           ret = CA_E_OK;
    ca_port_id_t        port_id = 0;
    aal_pwsv_cfg_msk_t  msk;
    aal_pwsv_cfg_t      cfg;

    CS_DEV_ID_VALIDATION(device_id);

    if(protocol > CA_EPON_POWERSAVE_PROTOCOL_EMPCP)
    {
        return CA_E_PARAM;
    }
    memset(&msk, 0, sizeof(aal_pwsv_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_pwsv_cfg_t));

    msk.s.eoam_detect_ena  = 1;
    msk.s.empcp_detect_ena = 1;

    if(protocol == CA_EPON_POWERSAVE_PROTOCOL_EOAM)
    {
        cfg.eoam_detect_ena  = 1;
        cfg.empcp_detect_ena = 0;
    }
    else
    {
        cfg.eoam_detect_ena  = 0;
        cfg.empcp_detect_ena = 1;
    }

    ret = aal_pwsv_cfg_set(device_id, port_id, msk, &cfg);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_powersave_protocol_get(
CA_IN       ca_device_id_t                     device_id,
CA_OUT      ca_epon_powersave_protocol_t    *protocol)
{
    ca_status_t           ret = CA_E_OK;
    ca_port_id_t        port_id = 0;
    aal_pwsv_cfg_t      cfg;

    CS_DEV_ID_VALIDATION(device_id);
    CA_NULL_PTR_CHECK(protocol);

    memset(&cfg, 0, sizeof(aal_pwsv_cfg_t));
    ret = aal_pwsv_cfg_get(device_id, port_id, &cfg);

    if(cfg.eoam_detect_ena)
    {
        *protocol = CA_EPON_POWERSAVE_PROTOCOL_EOAM;
    }
    else
    {
        *protocol = CA_EPON_POWERSAVE_PROTOCOL_EMPCP;
    }

    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_epon_powersave_monitor_set(
CA_IN       ca_device_id_t                     device_id,
CA_IN       ca_epon_powersave_monitor_t     *config)
{
    return CA_E_OK;
}

ca_status_t ca_epon_powersave_monitor_get(
CA_IN       ca_device_id_t                     device_id,
CA_OUT      ca_epon_powersave_monitor_t     *config)
{
    return CA_E_OK;
}


ca_status_t ca_epon_powersave_monitor_start(
CA_IN       ca_device_id_t                     device_id)
{
    ca_status_t               ret = CA_E_OK;
    ca_port_id_t            port_id = 0;
    ca_llid_t               llid = 0;
    aal_port_ingress_dir_t  dir;

    CS_DEV_ID_VALIDATION(device_id);

    for(port_id = AAL_PHY_PORT_ID_PON_10G; port_id < AAL_PHY_PORT_ID_MAX; port_id++)
    {
        ret = aal_port_ingress_dir_get(device_id, port_id, port_id, &dir);
        CA_RET_VALUE_CHECK(ret);

        if(dir != AAL_PORT_INGRESS_DIR_UPSTREAM)
        {
            continue;
        }

        ret = aal_port_def_llid_get(device_id, port_id, port_id, &llid);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_pwsv_onu_ena_set(device_id, port_id, llid, 1);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

ca_status_t ca_epon_powersave_monitor_stop(
CA_IN       ca_device_id_t                     device_id)
{
    ca_status_t               ret = CA_E_OK;
    ca_port_id_t            port_id = 0;
    ca_llid_t               llid = 0;
    aal_port_ingress_dir_t  dir;

    CS_DEV_ID_VALIDATION(device_id);

    for(port_id = AAL_PHY_PORT_ID_PON_10G; port_id < AAL_PHY_PORT_ID_MAX; port_id++)
    {
        ret = aal_port_ingress_dir_get(device_id, port_id, port_id, &dir);
        CA_RET_VALUE_CHECK(ret);

        if(dir != AAL_PORT_INGRESS_DIR_UPSTREAM)
        CA_RET_VALUE_CHECK(ret);

        ret = aal_port_def_llid_get(device_id, port_id, port_id, &llid);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_pwsv_onu_ena_set(device_id, port_id, llid, 0);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

ca_status_t ca_epon_powersave_monitor_status_get (
CA_IN       ca_device_id_t                         device_id,
CA_OUT      ca_epon_powersave_monitor_status_t  *status)
{
    return CA_E_OK;
}
#endif

ca_status_t ca_epon_dying_gasp_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     send_count,
    CA_IN ca_uint8_t     buffer[]
)
{
    // need to zhenbo fix
    //aal_epon_dying_gasp_set(device_id, port_id, send_count, buffer, 64);
    return CA_E_OK;
}


ca_status_t ca_epon_lods_ignore_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t	 port_id,
    CA_IN ca_boolean_t	 enable,
    CA_IN ca_uint8_t     duration
)
{

    CA_DEV_ID_CHECK(device_id);

    return CA_E_OK;

}

ca_status_t ca_epon_lods_ignore_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_boolean_t   *enable,
    CA_OUT ca_uint8_t     *duration
)
{
    CA_ASSERT_RET(enable, CA_E_PARAM);
    CA_ASSERT_RET(duration, CA_E_PARAM);


    return CA_E_OK;
}


ca_status_t ca_siepon_mc_llid_add
(
    CA_IN ca_device_id_t device_id,
    CA_IN  ca_mpcp_llid_t mc_llid,
    CA_OUT ca_llid_t      *llid_instance
)
{
    if (!llid_instance)
        return CA_E_PARAM;

    mc_llid &= 0xffff;  /* only use low 16 bits */

    #if 0
    if (mc_llid == 0xffff || mc_llid == 0xfffe)
        return CA_E_PARAM;
    #endif

    return aal_mpcp_mc_llid_add(device_id, 0, mc_llid, llid_instance);
}

ca_status_t ca_siepon_mc_llid_delete
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_mpcp_llid_t mc_llid

)
{
    mc_llid &= 0xffff;  /* only use low 16 bits */

    #if 0
    if (mc_llid == 0xffff || mc_llid == 0xfffe)
        return CA_E_PARAM;
    #endif

    return aal_mpcp_mc_llid_del(device_id, 0, mc_llid);
}

ca_status_t ca_siepon_mc_llid_list
(
    CA_IN     ca_device_id_t device_id,
    CA_IN_OUT ca_uint32_t    *mc_llid_nums,
    CA_IN_OUT ca_mpcp_llid_t *mc_llids
)
{
    if (!mc_llids || !mc_llid_nums)
        return CA_E_PARAM;

    if (*mc_llid_nums == 0)
        return CA_E_PARAM;

    return aal_mpcp_mc_llid_list(device_id, 0, mc_llid_nums, mc_llids);
}

ca_status_t ca_siepon_package_b_report_config_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     queue_id,
    CA_IN ca_boolean     zeroed_queue,
    CA_IN ca_uint32_t    zero_queue_threshold_value
)
{
    aal_puc_ntt_cfg_t puc_ntt_cfg;
    aal_puc_ntt_cfg_msk_t puc_ntt_cfg_msk;
    ca_status_t           ret = CA_E_OK;

    memset(&puc_ntt_cfg,0,sizeof(aal_puc_ntt_cfg_t));
    memset((void*)&puc_ntt_cfg_msk,0,sizeof(aal_puc_ntt_cfg_msk_t));

    puc_ntt_cfg_msk.s.zeroqth = 1;
    puc_ntt_cfg_msk.s.zeroqrpten = 1;

    puc_ntt_cfg.zeroqrpten = zeroed_queue;
    puc_ntt_cfg.zeroqth = zero_queue_threshold_value;

    ret = aal_puc_ntt_cfg_set(device_id,puc_ntt_cfg_msk,puc_ntt_cfg);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;

}

ca_status_t ca_siepon_package_b_report_config_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     queue_id,
    CA_OUT ca_boolean     *zeroed_queue,
    CA_OUT ca_uint32_t    *zero_queue_threshold_value
)
{
    aal_puc_ntt_cfg_t puc_ntt_cfg;
    ca_status_t       ret = CA_E_OK;

    CA_ASSERT_RET(zeroed_queue ,CA_E_PARAM);
    CA_ASSERT_RET(zero_queue_threshold_value,CA_E_PARAM);

    if(queue_id > 7)
        return CA_E_PARAM;

    memset(&puc_ntt_cfg,0,sizeof(aal_puc_ntt_cfg_t));

    ret = aal_puc_ntt_cfg_get(device_id,&puc_ntt_cfg);
    CA_RET_VALUE_CHECK(ret);

    *zeroed_queue = puc_ntt_cfg.zeroqrpten;
    *zero_queue_threshold_value = puc_ntt_cfg.zeroqth;

    return CA_E_OK;

}

ca_status_t ca_siepon_package_b_mpcp_queue_threshold_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint32_t    threshold
)
{
    aal_puc_epon_que_thrsd12_cfg_msk_t mask;
    aal_puc_epon_que_thrsd12_cfg_t cfg;
    aal_puc_rpt_adj_cfg1_msk_t  adj_cfg_msk;
    aal_puc_rpt_adj_cfg1_t  adj_cfg;

    memset(&mask,0,sizeof(aal_puc_epon_que_thrsd12_cfg_msk_t));
    memset(&cfg,0,sizeof(aal_puc_epon_que_thrsd12_cfg_t));
    memset((void*)&adj_cfg_msk,0,sizeof(aal_puc_rpt_adj_cfg1_msk_t));
    memset(&adj_cfg,0,sizeof(aal_puc_rpt_adj_cfg1_t));

    mask.s.qs1th = 1;

    cfg.qs1th = threshold;
    aal_puc_epon_queue_thrshld12_set(device_id,mask,&cfg);

    adj_cfg_msk.s.rptadjpid = 1;

    adj_cfg.rptadjpid = llid;
    aal_puc_rpt_adj_cfg1_set(device_id,adj_cfg_msk,adj_cfg);

    return CA_E_OK;

}

ca_status_t ca_siepon_package_b_mpcp_queue_threshold_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_llid_t      *llid,
    CA_OUT ca_uint32_t    *threshold
)
{
    aal_puc_epon_que_thrsd12_cfg_t cfg;
    aal_puc_rpt_adj_cfg1_t  adj_cfg;

    CA_ASSERT_RET(llid ,CA_E_PARAM);
    CA_ASSERT_RET(threshold,CA_E_PARAM);

    aal_puc_epon_queue_thrshld12_get(device_id,&cfg);
    *threshold = cfg.qs1th;

    aal_puc_rpt_adj_cfg1_get(device_id,&adj_cfg);
    *llid = adj_cfg.rptadjpid;

    return CA_E_OK;
}

ca_status_t ca_siepon_package_b_upstream_scheduler_mode_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_siepon_package_b_scheduler_mode_t scheduler_mode
)
{
    aal_puc_voq_sche_cfg_msk_t mask;
    aal_puc_voq_sche_cfg_t voq_sche_cfg;

    memset(&mask,0,sizeof(aal_puc_voq_sche_cfg_msk_t));
    memset(&voq_sche_cfg,0,sizeof(aal_puc_voq_sche_cfg_t));

    mask.s.pkgbschmode = 1;
    voq_sche_cfg.pkgbschmode = scheduler_mode;

    aal_puc_voq_sche_cfg_set(device_id,mask,&voq_sche_cfg);

    return CA_E_OK;
}

ca_status_t ca_siepon_package_b_upstream_scheduler_mode_get
(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  ca_port_id_t        port_id,
    CA_OUT ca_siepon_package_b_scheduler_mode_t	*scheduler_mode
)
{
    aal_puc_voq_sche_cfg_t voq_sche_cfg;

    CA_ASSERT_RET(scheduler_mode,CA_E_PARAM);

    memset(&voq_sche_cfg,0,sizeof(aal_puc_voq_sche_cfg_t));

    aal_puc_voq_sche_cfg_get(device_id,&voq_sche_cfg);
    *scheduler_mode = voq_sche_cfg.pkgbschmode;

    return CA_E_OK;
}

ca_status_t ca_siepon_mc_register_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   mc_llid,
    CA_IN ca_port_id_t   uc_llid,
    CA_IN ca_boolean_t   f_bind
)
{
    ca_status_t ret = CA_E_OK ;
    CA_ASSERT_RET((0==f_bind)||(1==f_bind),CA_E_PARAM);

    ret = aal_mpcp_cmd_llid_entry_set(0,0x30002,f_bind,uc_llid,mc_llid, 1);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;
}
ca_status_t ca_siepon_mc_register_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   mc_llid,
    CA_OUT ca_port_id_t   *uc_llid
)
{
    ca_status_t ret = CA_E_OK ;
    ca_uint8_t llid_idx = 0;

    CA_ASSERT_RET(uc_llid,CA_E_PARAM);

    ret = aal_mpcp_reg_llid_index_get(device_id,0x30002,mc_llid,&llid_idx);
    CA_RET_VALUE_CHECK(ret);

    *uc_llid = (ca_port_id_t)llid_idx;

    return CA_E_OK;
}

ca_status_t ca_epon_reset_pon_mac
(
    CA_IN   ca_device_id_t     device_id
)
{
    ca_uint32_t pon_mode;
    pon_mode = aal_pon_mac_mode_get(device_id);
    aal_puc_reset(device_id);
    //aal_pon_init(0,pon_mode);
    aal_epon_init(0, 0, pon_mode);
    return CA_E_OK;
}

/*
ca_status_t ca_epon_llid_traffic_enable_set (
    CA_IN   ca_device_id_t      device_id,
    CA_IN   ca_port_id_t        port_id,
    CA_IN   ca_llid_t           llid,
    CA_IN   ca_uint8_t          upstream,
    CA_IN   ca_uint8_t          downstream);
ca_status_t ca_epon_llid_traffic_enable_get (
    CA_IN   ca_device_id_t      device_id,
    CA_IN   ca_port_id_t        port_id,
    CA_IN   ca_llid_t           llid,
    CA_OUT  ca_uint8_t          *upstream,
    CA_OUT  ca_uint8_t          *downstream);

Parameters
    device_id   CA device number
    port_id     EPON port ID
    llid        LLID instance ID
    upstream    0=disable upstream frame sending on the specified LLID.
                1=enable upstream frame sending on the specified LLID.
    downstream  0=disable downstream frame sending on the specified LLID.
                1=enable downstream frame sending on the specified LLID.

Description
These API routines are used to enable/disable frame forwarding for the specified LLID instance.

*/
ca_status_t ca_epon_llid_traffic_enable_set
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_IN  ca_boolean_t   upstream,
    CA_IN  ca_boolean_t   downstream
)
{
    ca_status_t ret = CA_E_OK;
    ca_device_id_t dev = device_id;
    ca_uint32_t ldpid;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    if ((upstream > 1) || (1 != downstream))//Saturn will not set DS control now.
        return CA_E_PARAM;

#ifdef CONFIG_ARCH_CORTINA_G3HGU
    dev = device_id + 1;
#endif

    ldpid = llid + AAL_LPORT_LLID_GEM_INDEX_0;

    //__PON_LOCK();
    if (0 == upstream){
        ret = aal_port_arb_ldpid_pdpid_map_set(dev, 0, 0, ldpid, AAL_PPORT_BLACKHOLE);
        CA_RET_VALUE_CHECK(ret);
        ret = aal_port_arb_ldpid_pdpid_map_set(dev, 0, 1, ldpid, AAL_PPORT_BLACKHOLE);
        CA_RET_VALUE_CHECK(ret);
    }
    else{
        ret = aal_port_arb_ldpid_pdpid_map_set(dev, 0, 0, ldpid, AAL_PPORT_NI7);
        CA_RET_VALUE_CHECK(ret);
        ret = aal_port_arb_ldpid_pdpid_map_set(dev, 0, 1, ldpid, AAL_PPORT_QM);
        CA_RET_VALUE_CHECK(ret);
    }
    //__PON_UNLOCK();

    return ret;
}

ca_status_t ca_epon_llid_traffic_enable_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_boolean_t   *upstream,
    CA_OUT ca_boolean_t   *downstream
)
{
    ca_status_t ret = CA_E_OK;
    ca_device_id_t dev = device_id;
    ca_uint32_t ldpid, pdpid;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);

    CA_ASSERT_RET(upstream, CA_E_PARAM);
    CA_ASSERT_RET(downstream, CA_E_PARAM);

#ifdef CONFIG_ARCH_CORTINA_G3HGU
    dev = device_id + 1;
#endif

    ldpid = llid + AAL_LPORT_LLID_GEM_INDEX_0;

    //__PON_LOCK();
    *downstream= 1; //Saturn don't control DS always

    ret = aal_port_arb_ldpid_pdpid_map_get(dev, 0, 0, ldpid, &pdpid);
    CA_RET_VALUE_CHECK(ret);
    if (AAL_PPORT_BLACKHOLE == pdpid)
        *upstream = 0;
    else if (AAL_PPORT_NI7 == pdpid)
        *upstream = 1;
    else
        ret = CA_E_ERROR;

    //__PON_UNLOCK();

    return ret;
}

ca_status_t ca_epon_llid_loopback_enable_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_mpcp_llid_t llid,
    CA_IN ca_boolean_t   enable
)
{
    aal_ilpb_cfg_msk_t  ilpb_mask;
    aal_ilpb_cfg_t      ilpb_cfg;
    ca_uint32_t         llid_index;
    static ca_uint8_t   s_tag_match = 0;
    static ca_uint8_t   c_tag_match = 0;
    static ca_boolean_t original_enable = FALSE;
    ca_status_t ret = CA_E_OK;

    CA_BOOL_VAL_CHECK(enable);

    if(enable == original_enable){
        return CA_E_OK;
    }

    ilpb_mask.u32[0] = 0;
    ilpb_mask.u32[1] = 0;
    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));
    llid_index = ca_epon_mpcp_llid_index_get(device_id,port_id,llid);
    if(llid_index>= CA_MAX_EPON_PORT_LLID_NUMBER){
        ret= CA_E_ERROR;
        goto end;
    }

    ret = aal_port_ilpb_cfg_get(device_id,0x20+llid_index,&ilpb_cfg);
    if(ret != CA_E_OK){
        goto end;
    }
    if(enable){
        s_tag_match = ilpb_cfg.s_tpid_match;
        c_tag_match = ilpb_cfg.c_tpid_match;
        ilpb_cfg.default_cos = 7;
        ilpb_cfg.s_tpid_match = 0;
        ilpb_cfg.c_tpid_match = 0;
    }else{
        ilpb_cfg.s_tpid_match = s_tag_match;
        ilpb_cfg.c_tpid_match = c_tag_match;
        ilpb_cfg.default_cos = 0;
    }
    ilpb_cfg.loop_enable = (enable > 0);
    ilpb_cfg.da_sa_swap_en = (enable > 0);


    ilpb_mask.s.loop_enable = 1;
    ilpb_mask.s.da_sa_swap_en = 1;
    ilpb_mask.s.default_cos = 1;
    ilpb_mask.s.s_tpid_match = 1;
    ilpb_mask.s.c_tpid_match = 1;

    ret = aal_port_ilpb_cfg_set(device_id,0x20 + llid_index,ilpb_mask,&ilpb_cfg);
    if(ret != CA_E_OK){
        goto end;
    }

    original_enable = (enable>0);
end:

    return ret;
}

ca_status_t ca_epon_llid_loopback_enable_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_mpcp_llid_t llid,
    CA_OUT ca_boolean_t   *enable
)
{
    ca_status_t ret = CA_E_OK;
    aal_ilpb_cfg_t      ilpb_cfg;
    ca_uint32_t         llid_index;
    if(NULL == enable){
        return CA_E_PARAM;
    }

    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));
    llid_index = ca_epon_mpcp_llid_index_get(device_id,port_id,llid);
    if(llid_index>= CA_MAX_EPON_PORT_LLID_NUMBER){
        ret= CA_E_ERROR;
        goto end;
    }
    ret = aal_port_ilpb_cfg_get(device_id,0x20+llid_index,&ilpb_cfg);
    if(ret != CA_E_OK){
        goto end;
    }

    *enable = ilpb_cfg.loop_enable;
end:
    return ret;
}


EXPORT_SYMBOL(ca_epon_port_laser_polarity_set);
EXPORT_SYMBOL(ca_epon_port_laser_polarity_get);
EXPORT_SYMBOL(ca_epon_port_laser_mode_set);
EXPORT_SYMBOL(ca_epon_port_laser_mode_get);
EXPORT_SYMBOL(ca_epon_port_fec_enable_set);
EXPORT_SYMBOL(ca_epon_port_fec_enable_get);
EXPORT_SYMBOL(ca_epon_llid_encryption_enable_set);
EXPORT_SYMBOL(ca_epon_llid_encryption_enable_get);
EXPORT_SYMBOL(ca_epon_llid_encryption_key_set);
EXPORT_SYMBOL(ca_epon_llid_encryption_key_get);
EXPORT_SYMBOL(ca_epon_llid_encryption_active_key_index_set);
EXPORT_SYMBOL(ca_epon_llid_encryption_active_key_index_get);
EXPORT_SYMBOL(ca_epon_port_speed_set);
EXPORT_SYMBOL(ca_epon_port_speed_get);
EXPORT_SYMBOL(ca_epon_port_discard_control_set);
EXPORT_SYMBOL(ca_epon_port_discard_control_get);
EXPORT_SYMBOL(ca_epon_port_stats_get);
EXPORT_SYMBOL(ca_epon_port_fec_stats_get);
EXPORT_SYMBOL(ca_epon_port_status_get);
EXPORT_SYMBOL(ca_epon_port_link_status_get);
EXPORT_SYMBOL(ca_epon_mpcp_registration_set);
EXPORT_SYMBOL(ca_epon_mpcp_registration_get);
EXPORT_SYMBOL(ca_epon_mpcp_pending_grants_set);
EXPORT_SYMBOL(ca_epon_mpcp_pending_grants_get);
EXPORT_SYMBOL(ca_epon_mpcp_timer_set);
EXPORT_SYMBOL(ca_epon_mpcp_timer_get);
EXPORT_SYMBOL(ca_epon_mpcp_gate_timer_set);
EXPORT_SYMBOL(ca_epon_mpcp_gate_timer_get);
EXPORT_SYMBOL(ca_epon_mpcp_timestamp_drift_set);
EXPORT_SYMBOL(ca_epon_mpcp_timestamp_drift_get);
EXPORT_SYMBOL(ca_epon_mpcp_keep_silence_set);
EXPORT_SYMBOL(ca_epon_mpcp_keep_silence_get);
EXPORT_SYMBOL(ca_epon_queue_set_thresholds_set);
EXPORT_SYMBOL(ca_epon_queue_set_thresholds_get);
EXPORT_SYMBOL(ca_epon_mpcp_info_get);
EXPORT_SYMBOL(ca_epon_mpcp_report_mode_set);
EXPORT_SYMBOL(ca_epon_mpcp_report_mode_get);
EXPORT_SYMBOL(ca_epon_mpcp_registration_status_get);
EXPORT_SYMBOL(ca_epon_mpcp_stats_get);
EXPORT_SYMBOL(ca_epon_mpcp_llid_index_get);
EXPORT_SYMBOL(ca_epon_llid_stats_get);
EXPORT_SYMBOL(ca_epon_dying_gasp_set);
EXPORT_SYMBOL(ca_epon_lods_ignore_set);
EXPORT_SYMBOL(ca_epon_lods_ignore_get);
EXPORT_SYMBOL(ca_siepon_mc_llid_add);
EXPORT_SYMBOL(ca_siepon_mc_llid_delete);
EXPORT_SYMBOL(ca_siepon_mc_llid_list);
EXPORT_SYMBOL(ca_siepon_package_b_report_config_set);
EXPORT_SYMBOL(ca_siepon_package_b_report_config_get);
EXPORT_SYMBOL(ca_siepon_package_b_mpcp_queue_threshold_set);
EXPORT_SYMBOL(ca_siepon_package_b_mpcp_queue_threshold_get);
EXPORT_SYMBOL(ca_siepon_package_b_upstream_scheduler_mode_set);
EXPORT_SYMBOL(ca_siepon_package_b_upstream_scheduler_mode_get);
EXPORT_SYMBOL(ca_siepon_mc_register_set);
EXPORT_SYMBOL(ca_siepon_mc_register_get);
EXPORT_SYMBOL(ca_epon_reset_pon_mac);
EXPORT_SYMBOL(ca_epon_llid_traffic_enable_set);
EXPORT_SYMBOL(ca_epon_llid_traffic_enable_get);
EXPORT_SYMBOL(ca_epon_llid_loopback_enable_set);
EXPORT_SYMBOL(ca_epon_llid_loopback_enable_get);

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_epon_mpcp_silence_status_get (
CA_IN       ca_device_id_t         device_id,
CA_IN       ca_port_id_t           port_id,
CA_IN       ca_llid_t              llid,
CA_OUT      ca_boolean_t           *status)
{
    ca_status_t ret = CA_E_OK;
    ca_boolean_t silence_status;

    CA_DEV_ID_CHECK(device_id);
    __EPON_PORT_CHECK(port_id);
    __EPON_LLID_CHECK(llid);
    CA_NULL_PTR_CHECK(status);

    ret = aal_mpcp_llid_silence_status_get(device_id, (ca_uint8_t)llid, &silence_status);
    CA_RET_VALUE_CHECK(ret);

    *status = silence_status;

    return ret;
}
EXPORT_SYMBOL(ca_epon_mpcp_silence_status_get);
#endif
