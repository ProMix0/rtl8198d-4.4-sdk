
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
 * File   : aal_xgpon_fec.c
 * Brief  : XG(S)PON FEC AAL driver
 * History:
 *          2016-07-15 initial @Lidong
 *          2018-07-28:change aal_xgpon_fec_stats_t @Lidong
 *
 */

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_xgpon_fec.h"


/**
 * @aal_xgpon_fec_mask_mem_set
 * Set Memory stores mask Data for injecting error in the FEC blocks
 */
ca_status_t aal_xgpon_fec_mask_mem_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_uint8_t                 mem_addr,
    CA_IN ca_uint16_t                mem_data)
{

    AAL_PON_REG_WRITE(mem_data, XGPN_XGFEC_FEC_MASK_MEM_SW_DATA);

    AAL_PON_REG_INDIRECT_WRITE(mem_addr, XGPN_XGFEC_FEC_MASK_MEM_SW_ACCESS);

    return CA_E_OK;

}


/**
 * @aal_xgpon_fec_mask_mem_get
 * Get Memory stores mask Data for injecting error in the FEC blocks
 */
ca_status_t aal_xgpon_fec_mask_mem_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 mem_addr,
    CA_OUT ca_uint16_t               *mem_data)
{
    XGPN_XGFEC_FEC_MASK_MEM_SW_DATA_t reg_data;

    AAL_PON_REG_INDIRECT_READ(mem_addr, XGPN_XGFEC_FEC_MASK_MEM_SW_ACCESS);

    reg_data.wrd = AAL_PON_REG_READ(XGPN_XGFEC_FEC_MASK_MEM_SW_DATA);
    *mem_data = reg_data.bf.MASK_MEM_DATA;

    return CA_E_OK;
}

/**
 * @aal_xgpon_fec_ctrl_set
 * Set ONU FEC control configuration
 */
ca_status_t aal_xgpon_fec_ctrl_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_xgpon_fec_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpon_fec_ctrl_t      *ctrl)
{
    ca_status_t           ret = CA_E_OK;
    XGPN_XGFEC_FECCNTRL_t reg_data0;
    XGPN_XGFEC_TXDISFEC_t reg_data1;

    CA_ASSERT_RET(ctrl, CA_E_PARAM);

    reg_data0.wrd = AAL_PON_REG_READ(XGPN_XGFEC_FECCNTRL);
    reg_data1.wrd = AAL_PON_REG_READ(XGPN_XGFEC_TXDISFEC);

	if (ctrl_mask.s.rx_fec_bypass_delay_mode) reg_data0.bf.rx_fec_bypass_delay_mode  = ctrl->rx_fec_bypass_delay_mode ;
    if (ctrl_mask.s.fec_dec_corr_disable    ) reg_data0.bf.fec_dec_corr_disable      = ctrl->fec_dec_corr_disable       ;
    if (ctrl_mask.s.tx_fec_bypass_delay_mode) reg_data1.bf.tx_fec_bypass_delay_mode  = ctrl->tx_fec_bypass_delay_mode            ;
    if (ctrl_mask.s.error_inject_count      ) reg_data1.bf.error_inject_count        = ctrl->error_inject_count         ;
    if (ctrl_mask.s.swap_after_fec_encoder  ) reg_data1.bf.swap_after_fec_encoder    = ctrl->swap_after_fec_encoder           ;
    if (ctrl_mask.s.swap_before_fec_encoder ) reg_data1.bf.swap_before_fec_encoder   = ctrl->swap_before_fec_encoder     ;
    if (ctrl_mask.s.disableFEC              ) reg_data1.bf.disableFEC                = ctrl->disableFEC   ;

    if (ctrl_mask.u32_data) {
        AAL_PON_REG_WRITE(reg_data0.wrd, XGPN_XGFEC_FECCNTRL);
        AAL_PON_REG_WRITE(reg_data1.wrd, XGPN_XGFEC_TXDISFEC);
    }
    return ret;
}


/**
 * @aal_xgpon_fec_ctrl_get
 * Get ONU FEC control configuration
 */
ca_status_t aal_xgpon_fec_ctrl_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_xgpon_fec_ctrl_t      *ctrl)
{
    ca_status_t           ret = CA_E_OK;

    XGPN_XGFEC_FECCNTRL_t reg_data0;
    XGPN_XGFEC_TXDISFEC_t reg_data1;

    CA_ASSERT_RET(ctrl, CA_E_PARAM);

    reg_data0.wrd = AAL_PON_REG_READ(XGPN_XGFEC_FECCNTRL);
    reg_data1.wrd = AAL_PON_REG_READ(XGPN_XGFEC_TXDISFEC);

    ctrl->rx_fec_bypass_delay_mode = reg_data0.bf.rx_fec_bypass_delay_mode  ;
    ctrl->fec_dec_corr_disable     = reg_data0.bf.fec_dec_corr_disable      ;
    ctrl->tx_fec_bypass_delay_mode = reg_data1.bf.tx_fec_bypass_delay_mode  ;
    ctrl->error_inject_count       = reg_data1.bf.error_inject_count        ;
    ctrl->swap_after_fec_encoder   = reg_data1.bf.swap_after_fec_encoder    ;
    ctrl->swap_before_fec_encoder  = reg_data1.bf.swap_before_fec_encoder   ;
    ctrl->disableFEC               = reg_data1.bf.disableFEC                ;

    return ret;
}

/**
 * @aal_xgpon_fec_err_inject_set
 * Set ONU FEC PCS error inject enable trigger
 */
ca_status_t aal_xgpon_fec_err_inject_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_boolean_t               inject_enable)
{
    AAL_PON_REG_FIELD_WRITE(XGPN_XGFEC_FEC_ERROR_INJECT,
        enable, (inject_enable != 0));

    return CA_E_OK;
}

/**
 * @aal_xgpon_fec_err_inject_get
 * Get ONU FEC PCS error inject flag
 */
ca_status_t aal_xgpon_fec_err_inject_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_boolean_t              *inject_enable)
{
    CA_ASSERT_RET(inject_enable, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_XGFEC_FEC_ERROR_INJECT,
        enable, *inject_enable);

    return CA_E_OK;
}


/**
 * @aal_xgpon_fec_stats_get
 * Get ONU FEC statistics
 */
ca_status_t aal_xgpon_fec_stats_get(
    CA_IN  ca_device_id_t              device_id,
    CA_OUT aal_xgpon_fec_stats_t   *stats)
{
    ca_status_t  ret = CA_E_OK;

    CA_ASSERT_RET(stats, CA_E_PARAM);

    stats->corrected_bytes_rx          = AAL_PON_REG_READ(XGPN_XGFEC_FECCORRBYTE);
    stats->total_codewords_rx          = AAL_PON_REG_READ(XGPN_XGFEC_FECTOTALCODE);
    stats->corrected_codewords_rx      = AAL_PON_REG_READ(XGPN_XGFEC_FECCORRCODE);
    stats->uncorrectable_codewords_rx  = AAL_PON_REG_READ(XGPN_XGFEC_FECUNCORCODE);

    return ret;
}


#ifdef __KERNEL__

EXPORT_SYMBOL(aal_xgpon_fec_stats_get);
EXPORT_SYMBOL(aal_xgpon_fec_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_fec_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_fec_err_inject_get);
EXPORT_SYMBOL(aal_xgpon_fec_err_inject_set);
EXPORT_SYMBOL(aal_xgpon_fec_mask_mem_get);
EXPORT_SYMBOL(aal_xgpon_fec_mask_mem_set);

#endif

