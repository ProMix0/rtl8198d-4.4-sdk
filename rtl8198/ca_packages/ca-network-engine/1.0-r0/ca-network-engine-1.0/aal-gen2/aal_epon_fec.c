
#include "ca-scfg/scfg.h"

#include "osal_common.h"
#include "aal_pon.h"
#include <soc/cortina/registers.h>
#include "aal_epon_fec.h"

#define __TX_IPG_ADJUST_MAX                 7

static ca_uint32_t  g_xgpcs_bd_pattern_fecless[3] = {0, 0, 0};   /* do backup for fec pattern configuration */
static ca_boolean_t g_cortina_olt_flag = CA_FALSE;

#if 0
ca_status_t aal_epon_fec_llid_ena_set
(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN aal_fec_llid_ena_t       *fec_ena
)
{
    AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_encode_enable , (fec_ena->tx_ena!=0));
    AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_decode_enable , (fec_ena->rx_ena!=0));

    return CA_E_OK;
}

ca_status_t aal_epon_fec_llid_ena_get
(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_OUT aal_fec_llid_ena_t      *fec_ena
)
{
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_encode_enable, (fec_ena->tx_ena));
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_decode_enable, (fec_ena->rx_ena));
    return CA_E_OK;
}
#endif

ca_status_t aal_epon_fec_cfg_set
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_IN aal_epon_fec_msk_t mask,
    CA_IN aal_epon_fec_cfg_t *cfg
)
{
    if (mask.s.tx_encoding_mode)
    {
        if (cfg->tx_encoding_mode > AAL_FEC_PREAMBLE_MODE_FIX8B)
        {
            return CA_E_PARAM;
        }
    }

    if (mask.s.tx_tfec_pattern)
    {
        if (cfg->tx_tfec_pattern > AAL_FEC_STD_MODE_2005)
        {
            return CA_E_PARAM;
        }
    }

    if (mask.s.tx_ipg_adjust)
    {
        if (cfg->tx_ipg_adjust > __TX_IPG_ADJUST_MAX)
        {
            return CA_E_PARAM;
        }
    }

    if (mask.s.rx_ctrl)
    {
        if (cfg->rx_ctrl > 0x7ff)
            return CA_E_PARAM;
    }

    if (mask.s.tx_encoding_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_encode_enable , (cfg->tx_encoding_ena != 0));
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, txfec_1g_on , (cfg->tx_encoding_ena != 0));
        AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG, pyld1gfecon , (cfg->tx_encoding_ena != 0));
        AAL_PON_REG_FIELD_WRITE(PUC_RPTCFG, rpt1gfecon , (cfg->tx_encoding_ena != 0));
    }

    if (mask.s.tx_encoding_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CFG, tx_fec8B, cfg->tx_encoding_mode);
    }

    if (mask.s.tx_tfec_pattern)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CFG, tfec_rep, cfg->tx_tfec_pattern);
    }

    if (mask.s.tx_ipg_adjust)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CFG, tx_ipg_adj, cfg->tx_ipg_adjust);
    }

    if (mask.s.tx_1st_preamble_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CFG, tx_1st_preamble_mode, (cfg->tx_1st_preamble_mode != 0));
    }

    if (mask.s.tx_path_clear)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_tx_clr, (cfg->tx_path_clear != 0));
    }

    if (mask.s.rx_decoding_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_decode_enable , (cfg->rx_decoding_ena != 0));
    }

    if (mask.s.rx_path_clear)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_rx_clr, (cfg->rx_path_clear != 0));
    }

    if (mask.s.non_fec_mode)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, non_FEC_mode, (cfg->non_fec_mode != 0));
    }

    if (mask.s.rx_ctrl)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_CTRL, fec_rx_ctrl, cfg->rx_ctrl);
    }

    if (mask.s.cnt_clr)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_FEC_Counter_Control, FEC_CNTR_rd_clr_mode, cfg->cnt_clr);
    }

    return CA_E_OK;
}

ca_status_t aal_epon_fec_cfg_get
(
    CA_IN ca_device_id_t      device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT aal_epon_fec_cfg_t *cfg
)
{
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CFG, tx_fec8B, cfg->tx_encoding_mode);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CFG, tfec_rep, cfg->tx_tfec_pattern);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CFG, tx_ipg_adj, cfg->tx_ipg_adjust);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CFG, tx_1st_preamble_mode, cfg->tx_1st_preamble_mode);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_tx_clr, cfg->tx_path_clear);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_rx_clr, cfg->rx_path_clear);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, non_FEC_mode, cfg->non_fec_mode);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_rx_ctrl, cfg->rx_ctrl);
    
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_encode_enable ,cfg->tx_encoding_ena);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_CTRL, fec_decode_enable ,cfg->rx_decoding_ena);

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_FEC_Counter_Control, FEC_CNTR_rd_clr_mode, cfg->cnt_clr);
    return CA_E_OK;
}

ca_status_t aal_epon_fec_stats_get
(
    ca_device_id_t device_id,
    ca_port_id_t port_id,
    aal_epon_fec_stats_t *stats
    )
{
    memset(stats, 0, sizeof(aal_epon_fec_stats_t));
    stats->rx_good_pkt = AAL_PON_REG_READ(EPON_ONU_EPON_FEC_Block_No_Error_Counter);
    stats->rx_err_pkt = AAL_PON_REG_READ(EPON_ONU_EPON_FEC_Block_Counter) - stats->rx_good_pkt;
    stats->rx_corr_codeword = AAL_PON_REG_READ(EPON_ONU_EPON_FEC_Block_Correctable_Error_Counter);
    stats->rx_uncorr_codeword = AAL_PON_REG_READ(EPON_ONU_EPON_FEC_Block_Uncorrectable_Error_Counter);
    stats->rx_corr_bytes = AAL_PON_REG_READ(EPON_ONU_EPON_FEC_Corr_Byte_Counter);
    stats->rx_total_codeword = stats->rx_corr_codeword + stats->rx_uncorr_codeword;  

    return CA_E_OK;
}


/* XG part */

ca_status_t aal_epon_fec_xg_cfg_set
(
    ca_device_id_t device_id,
    ca_port_id_t port_id,
    aal_epon_fec_xg_cfg_msk_t msk,
    const aal_epon_fec_xg_cfg_t *cfg
    )
{
    if(msk.s.rx_mode && cfg->rx_mode >= AAL_FEC_XG_RX_MODE_MAX)
       return CA_E_PARAM;

    if (msk.s.rx_mode)
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_XGPCS_FEC_CFG, rx_mode, (cfg->rx_mode & 0x3));

    if (msk.s.tx_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_XGPCS_FEC_CFG, tx_off, (cfg->tx_ena == 0));

        if (!g_cortina_olt_flag)    /* BD pattern needs to be changed for non-cortina OLT */
        {
            if (cfg->tx_ena)   /* recover default value when enabling FEC Tx */
            {
                AAL_PON_REG_WRITE(g_xgpcs_bd_pattern_fecless[0], EPON_EPON_GLB_XGPCS_BD_pattern_fecless0);
                AAL_PON_REG_WRITE(g_xgpcs_bd_pattern_fecless[1], EPON_EPON_GLB_XGPCS_BD_pattern_fecless1);
                AAL_PON_REG_WRITE(g_xgpcs_bd_pattern_fecless[2], EPON_EPON_GLB_XGPCS_BD_pattern_fecless2);

            }
            else    /* set special value when disabling FEC Tx*/
            {
                AAL_PON_REG_WRITE(0x1b1a27d5, EPON_EPON_GLB_XGPCS_BD_pattern_fecless0);
                AAL_PON_REG_WRITE(0xd61f1b48, EPON_EPON_GLB_XGPCS_BD_pattern_fecless1);
                AAL_PON_REG_WRITE(0x1,        EPON_EPON_GLB_XGPCS_BD_pattern_fecless2);
            }
        }
    }

    if (msk.s.rx_bypass_delay_ena)
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_FECCNTRL, rx_fec_bypass_delay_mode, (cfg->rx_bypass_delay_ena > 0));


    return CA_E_OK;
}

ca_status_t aal_epon_fec_xg_cfg_get
(
    ca_device_id_t device_id,
    ca_port_id_t port_id,
    aal_epon_fec_xg_cfg_t *cfg
    )
{
    ca_uint32_t fec_rx_mode = 0;
    ca_uint32_t fec_tx_off  = 0;

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_XGPCS_FEC_CFG, rx_mode, fec_rx_mode);
    cfg->rx_mode = (aal_epon_fec_xg_rx_mode_t)fec_rx_mode;

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_XGPCS_FEC_CFG, tx_off, fec_tx_off);
    cfg->tx_ena = (fec_tx_off == 1) ? 0 : 1;

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_RX_FECCNTRL, rx_fec_bypass_delay_mode, cfg->rx_bypass_delay_ena);

    return CA_E_OK;
}

ca_status_t aal_epon_fec_xg_stats_get
(
    ca_device_id_t device_id,
    ca_port_id_t port_id,
    aal_epon_fec_stats_t *stats
    )
{
    stats->rx_corr_bytes      = 0; /* meaningless for xg */
    stats->rx_good_pkt = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_GOODPKT);
    stats->rx_err_pkt = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_ERRPKT);
    stats->rx_corr_codeword = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_FECCORCODE);
    stats->rx_uncorr_codeword = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_FECUNCORCODE);
    stats->rx_err_pattern = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_ERRPAT);
    stats->rx_err_blocks = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_ERRDEC);
    stats->rx_err_ber = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_FECCORRBYTE);
    stats->rx_total_codeword = AAL_PON_REG_READ(EPON_XGEPN_XGPCS_RX_FECTOTALCODE);

    return CA_E_OK;
}

ca_status_t aal_epon_fec_init
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    mode)
{
    aal_epon_fec_msk_t        mask;
    aal_epon_fec_cfg_t        cfg;
    aal_epon_fec_xg_cfg_msk_t xg_mask;
    aal_epon_fec_xg_cfg_t     xg_cfg;
    ca_uint8_t str_olt_type[128] = {0};
    ca_status_t rtn = CA_E_ERROR;

    if (mode == ONU_PON_MAC_EPON_1G)
    {
        memset(&mask, 0, sizeof(aal_epon_fec_msk_t));
        memset(&cfg, 0, sizeof(aal_epon_fec_cfg_t));

        mask.s.tx_encoding_ena = 1;
        mask.s.rx_decoding_ena = 1;
        //mask.s.non_fec_mode = 1;
        cfg.tx_encoding_ena = FALSE;
        cfg.rx_decoding_ena = FALSE;
        //cfg.non_fec_mode = 0;

        return aal_epon_fec_cfg_set(0, 0, mask, &cfg);
    }
    else if (mode == ONU_PON_MAC_EPON_D10G)
    {
        memset(&mask, 0, sizeof(aal_epon_fec_msk_t));
        memset(&cfg, 0, sizeof(aal_epon_fec_cfg_t));

        mask.s.tx_encoding_ena = 1;
        //mask.s.non_fec_mode = 1;
        cfg.tx_encoding_ena = FALSE;
        //cfg.non_fec_mode = 0;

        aal_epon_fec_cfg_set(0, 0, mask, &cfg);

        memset(&xg_mask, 0 , sizeof(aal_epon_fec_xg_cfg_msk_t));
        memset(&xg_cfg, 0 , sizeof(aal_epon_fec_xg_cfg_t));

        xg_mask.s.rx_mode = 1;
        xg_cfg.rx_mode = AAL_FEC_XG_RX_MODE_AUTO;

        return aal_epon_fec_xg_cfg_set(0, 0, xg_mask, &xg_cfg);
    }
    else
    {
        /* save xgpcs fec pattern */
        g_xgpcs_bd_pattern_fecless[0] = AAL_PON_REG_READ(EPON_EPON_GLB_XGPCS_BD_pattern_fecless0);
        g_xgpcs_bd_pattern_fecless[1] = AAL_PON_REG_READ(EPON_EPON_GLB_XGPCS_BD_pattern_fecless1);
        g_xgpcs_bd_pattern_fecless[2] = AAL_PON_REG_READ(EPON_EPON_GLB_XGPCS_BD_pattern_fecless2);

        rtn = scfg_read(0, CFG_ID_PON_OLT_TYPE, sizeof(str_olt_type), &str_olt_type[0]);
        if (rtn != CA_E_OK)
            strcpy(str_olt_type, "CORTINA");

        ca_printf("ca-epon: OLT vendor(%s)\n", str_olt_type);

        if (strcmp(str_olt_type, "CORTINA") == 0)
            g_cortina_olt_flag = CA_TRUE;
        else
            g_cortina_olt_flag = CA_FALSE;

        memset(&xg_mask, 0, sizeof(aal_epon_fec_xg_cfg_msk_t));
        memset(&xg_cfg,  0, sizeof(aal_epon_fec_xg_cfg_t));

        xg_mask.s.tx_ena  = 1;
        xg_mask.s.rx_mode = 1;
        xg_cfg.tx_ena     = TRUE;
        xg_cfg.rx_mode    = AAL_FEC_XG_RX_MODE_AUTO;

        return aal_epon_fec_xg_cfg_set(0, 0, xg_mask, &xg_cfg);
    }

    return CA_E_OK;
}

EXPORT_SYMBOL(aal_epon_fec_cfg_set);
EXPORT_SYMBOL(aal_epon_fec_cfg_get);
EXPORT_SYMBOL(aal_epon_fec_stats_get);
EXPORT_SYMBOL(aal_epon_fec_xg_cfg_set);
EXPORT_SYMBOL(aal_epon_fec_xg_cfg_get);
EXPORT_SYMBOL(aal_epon_fec_xg_stats_get);

