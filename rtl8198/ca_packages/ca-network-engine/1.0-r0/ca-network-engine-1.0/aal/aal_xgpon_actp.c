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
 * File   : aal_xgpon_actp.c
 * Brief  : XG(S)PON Activation Processor AAL driver
 * History:
 *          2016-07-15 initial @Lidong
 *
 */

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_xgpon_actp.h"
#include "ca-scfg/scfg.h"



/**
 * @aal_xgpon_actp_ctrl_set
 * Set ONU Activation processor configuration
 */
ca_status_t aal_xgpon_actp_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_xgpon_actp_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpon_actp_ctrl_t      *ctrl)
{
    ca_status_t       ret = CA_E_OK;
    XGPN_ACTP_CTRL_t  reg_data;

    CA_ASSERT_RET(ctrl, CA_E_PARAM);

    if(!ctrl_mask.u32_data)
        return CA_E_OK;

    reg_data.wrd = AAL_PON_REG_READ(XGPN_ACTP_CTRL);

    if (ctrl_mask.s.sw_mode)
        reg_data.bf.sw_mode = ctrl->sw_mode;

    if (ctrl_mask.s.sw_discard_all)
        reg_data.bf.sw_discard_all = ctrl->sw_discard_all;

    if (ctrl_mask.s.response_threshold)
        reg_data.bf.response_threshold = ctrl->response_threshold;

    if (ctrl_mask.s.wlcp_enable)
        reg_data.bf.wlcp_enable = ctrl->wlcp_enable;

    AAL_PON_REG_WRITE(reg_data.wrd, XGPN_ACTP_CTRL);

    return ret;
}

/**
 * @aal_xgpon_actp_ctrl_get
 * Get ONU Activation processor configuration
 */
ca_status_t aal_xgpon_actp_ctrl_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_xgpon_actp_ctrl_t      *ctrl)
{
    ca_status_t       ret = CA_E_OK;
    XGPN_ACTP_CTRL_t  reg_data;

    CA_ASSERT_RET(ctrl, CA_E_PARAM);

    reg_data.wrd = AAL_PON_REG_READ(XGPN_ACTP_CTRL);

    ctrl->sw_mode            = reg_data.bf.sw_mode;
    ctrl->sw_discard_all     = reg_data.bf.sw_discard_all;
    ctrl->response_threshold = reg_data.bf.response_threshold;
    ctrl->wlcp_enable        = reg_data.bf.wlcp_enable;

    return ret;
}

#if 0
/**
 * @aal_xgpon_actp_threshold_set
 * Set ONU Activation sequencer/timeout threshold
 */
ca_status_t aal_xgpon_actp_threshold_set(
    CA_IN ca_device_id_t                      device_id,
    CA_IN aal_xgpon_actp_threshold_mask_t  threshold_mask,
    CA_IN aal_xgpon_actp_threshold_t      *threshold)
{
    ca_status_t ret = CA_E_OK;

    CA_ASSERT_RET(threshold, CA_E_PARAM);

    if (threshold_mask.s.to1_timer)
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_ACT_THRSHLD, to1_timer, threshold->to1_timer);

    if (threshold_mask.s.to2_timer)
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_ACT_THRSHLD, to2_timer, threshold->to2_timer);
    return ret;
}


/**
 * @aal_xgpon_actp_threshold_get
 * Get ONU Activation sequencer/timeout threshold configuration
 */
ca_status_t aal_xgpon_actp_threshold_get(
    CA_IN  ca_device_id_t                   device_id,
    CA_OUT aal_xgpon_actp_threshold_t   *threshold)
{
    ca_status_t              ret = CA_E_OK;
    XGPN_ACTP_ACT_THRSHLD_t  reg_data;

    CA_ASSERT_RET(threshold, CA_E_PARAM);

    reg_data.wrd = AAL_PON_REG_READ(XGPN_ACTP_ACT_THRSHLD);

    threshold->to1_timer = reg_data.bf.to1_timer;
    threshold->to2_timer = reg_data.bf.to2_timer;
    return ret;
}
#endif

/**
 * @aal_xgpon_actp_status_get
 * Get ONU Activation state and debug state: O1 ~ O7
 */
ca_status_t aal_xgpon_actp_status_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_xgpon_actp_status_t   *sts)
{
    ca_status_t           ret = CA_E_OK;
    XGPN_ACTP_STS_t       reg_data0;
    XGPN_ACTP_DEREG_STS_t reg_data1;

    CA_ASSERT_RET(sts, CA_E_PARAM);

    reg_data0.wrd = AAL_PON_REG_READ(XGPN_ACTP_STS);
    reg_data1.wrd = AAL_PON_REG_READ(XGPN_ACTP_DEREG_STS);

    sts->onu_state          = reg_data0.bf.onu_state        ;
    sts->onu_sub_state      = reg_data0.bf.onu_sub_state    ;
    sts->profile_learning   = reg_data0.bf.profile_learning ;
    sts->rx_optical_tuning  = reg_data0.bf.rx_optical_tuning;
    sts->tx_optical_tuning  = reg_data0.bf.tx_optical_tuning;
    sts->previous_onu_state = reg_data0.bf.previous_onu_state;
    sts->previous_sub_state = reg_data0.bf.previous_sub_state;

    sts->dbg_onu_state     = reg_data1.bf.onu_state;

    return ret;
}

/**
 * @aal_xgpon_actp_status_set
 * Set ONU Activation state
 */
ca_status_t aal_xgpon_actp_status_set(
    CA_IN ca_device_id_t                  device_id,
    CA_IN aal_xgpon_actp_status_mask_t sts_mask,
    CA_IN aal_xgpon_actp_status_t     *sts)
{
    XGPN_ACTP_STS_t       reg_data0;

    CA_ASSERT_RET(sts, CA_E_PARAM);

    if(!sts_mask.u32_data)
        return CA_E_OK;

    reg_data0.wrd = AAL_PON_REG_READ(XGPN_ACTP_STS);

    if (sts_mask.s.onu_state)
        reg_data0.bf.onu_state         = sts->onu_state         ;

    if (sts_mask.s.onu_sub_state)
        reg_data0.bf.onu_sub_state     = sts->onu_sub_state     ;

    if (sts_mask.s.profile_learning)
        reg_data0.bf.profile_learning  = sts->profile_learning  ;

    if (sts_mask.s.rx_optical_tuning)
        reg_data0.bf.rx_optical_tuning = sts->rx_optical_tuning ;

    if (sts_mask.s.tx_optical_tuning)
        reg_data0.bf.tx_optical_tuning = sts->tx_optical_tuning ;

    AAL_PON_REG_WRITE(reg_data0.wrd, XGPN_ACTP_STS);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_state_chg_get
 * Get XGPON activation processing state changing reason/status
 */
ca_status_t aal_xgpon_actp_state_chg_get(
    CA_IN  ca_device_id_t                   device_id,
    CA_OUT aal_xgpon_actp_state_chg_t   *sts)
{
    ca_status_t             ret = CA_E_OK;
    XGPN_ACTP_ST_CHG_INT_t  reg_sts;

    CA_ASSERT_RET(sts, CA_E_PARAM);

    reg_sts.wrd = AAL_PON_REG_READ(XGPN_ACTP_ST_CHG_INT);

    sts->deregst              = reg_sts.bf.deregst             ;
    sts->loss_sync            = reg_sts.bf.loss_sync           ;
    sts->ds_synced            = reg_sts.bf.ds_synced           ;
    sts->DWLCH_appropriate    = reg_sts.bf.DWLCH_appropriate   ;
    sts->DWLCH_inappropriate  = reg_sts.bf.DWLCH_inappropriate ;
    sts->disable_SN           = reg_sts.bf.disable_SN          ;
    sts->deactivate_OID       = reg_sts.bf.deactivate_OID      ;
    sts->assign_OID           = reg_sts.bf.assign_OID          ;
    sts->calibr_req_mismatch  = reg_sts.bf.calibr_req_mismatch ;
    sts->TOZ_expire           = reg_sts.bf.TOZ_expire          ;
    sts->TO1_expire           = reg_sts.bf.TO1_expire          ;
    sts->assign_OID_inconsist = reg_sts.bf.assign_OID_inconsist;
    sts->assign_EqD           = reg_sts.bf.assign_EqD          ;
    sts->tuning_request_ack   = reg_sts.bf.tuning_request_ack  ;
    sts->tuning_request_tx    = reg_sts.bf.tuning_request_tx   ;
    sts->tuning_request_rx    = reg_sts.bf.tuning_request_rx   ;
    sts->enable_SN            = reg_sts.bf.enable_SN           ;
    sts->us_tuning_confirm    = reg_sts.bf.us_tuning_confirm   ;
    sts->TO2_expire           = reg_sts.bf.TO2_expire          ;
    sts->TO3_expire           = reg_sts.bf.TO3_expire          ;
    sts->TO4_expire           = reg_sts.bf.TO4_expire          ;
    sts->TO5_expire           = reg_sts.bf.TO5_expire          ;
    sts->ds_synced_O6         = reg_sts.bf.ds_synced_O6        ;

    return ret;

}

/**
 * @aal_xgpon_actp_state_chg_enable_set
 * Set ONU Activation state Changing sub interrupt enable bitmap
 */
ca_status_t aal_xgpon_actp_state_chg_enable_set(
    CA_IN  ca_device_id_t                      device_id,
    CA_IN  aal_xgpon_actp_state_chg_mask_t  mask,
    CA_IN  aal_xgpon_actp_state_chg_t      *sts)
{
    ca_status_t              ret = CA_E_OK;
    XGPN_ACTP_ST_CHG_INTE_t  reg_sts;

    CA_ASSERT_RET(sts, CA_E_PARAM);
    CA_ASSERT_RET(mask.u32_data, CA_E_OK);

    reg_sts.wrd = AAL_PON_REG_READ(XGPN_ACTP_ST_CHG_INTE);

    if (mask.s.deregst             ) reg_sts.bf.deregstE              = sts->deregst              ;
    if (mask.s.loss_sync           ) reg_sts.bf.loss_syncE            = sts->loss_sync            ;
    if (mask.s.ds_synced           ) reg_sts.bf.ds_syncedE            = sts->ds_synced            ;
    if (mask.s.DWLCH_appropriate   ) reg_sts.bf.DWLCH_appropriateE    = sts->DWLCH_appropriate    ;
    if (mask.s.DWLCH_inappropriate ) reg_sts.bf.DWLCH_inappropriateE  = sts->DWLCH_inappropriate  ;
    if (mask.s.disable_SN          ) reg_sts.bf.disable_SNE           = sts->disable_SN           ;
    if (mask.s.deactivate_OID      ) reg_sts.bf.deactivate_OIDE       = sts->deactivate_OID       ;
    if (mask.s.assign_OID          ) reg_sts.bf.assign_OIDE           = sts->assign_OID           ;
    if (mask.s.calibr_req_mismatch ) reg_sts.bf.calibr_req_mismatchE  = sts->calibr_req_mismatch  ;
    if (mask.s.TOZ_expire          ) reg_sts.bf.TOZ_expireE           = sts->TOZ_expire           ;
    if (mask.s.TO1_expire          ) reg_sts.bf.TO1_expireE           = sts->TO1_expire           ;
    if (mask.s.assign_OID_inconsist) reg_sts.bf.assign_OID_inconsistE = sts->assign_OID_inconsist ;
    if (mask.s.assign_EqD          ) reg_sts.bf.assign_EqDE           = sts->assign_EqD           ;
    if (mask.s.tuning_request_ack  ) reg_sts.bf.tuning_request_ackE   = sts->tuning_request_ack   ;
    if (mask.s.tuning_request_tx   ) reg_sts.bf.tuning_request_txE    = sts->tuning_request_tx    ;
    if (mask.s.tuning_request_rx   ) reg_sts.bf.tuning_request_rxE    = sts->tuning_request_rx    ;
    if (mask.s.enable_SN           ) reg_sts.bf.enable_SNE            = sts->enable_SN            ;
    if (mask.s.us_tuning_confirm   ) reg_sts.bf.us_tuning_confirmE    = sts->us_tuning_confirm    ;
    if (mask.s.TO2_expire          ) reg_sts.bf.TO2_expireE           = sts->TO2_expire           ;
    if (mask.s.TO3_expire          ) reg_sts.bf.TO3_expireE           = sts->TO3_expire           ;
    if (mask.s.TO4_expire          ) reg_sts.bf.TO4_expireE           = sts->TO4_expire           ;
    if (mask.s.TO5_expire          ) reg_sts.bf.TO5_expireE           = sts->TO5_expire           ;
    if (mask.s.ds_synced_O6        ) reg_sts.bf.ds_synced_O6E         = sts->ds_synced_O6         ;

    AAL_PON_REG_WRITE(reg_sts.wrd, XGPN_ACTP_ST_CHG_INTE);

    return ret;

}


/**
 * @aal_xgpon_actp_state_chg_enable_get
 * Get ONU Activation state Changing sub interrupt enable bitmap
 */
ca_status_t aal_xgpon_actp_state_chg_enable_get(
    CA_IN  ca_device_id_t                   device_id,
    CA_OUT aal_xgpon_actp_state_chg_t   *sts)
{
    ca_status_t              ret = CA_E_OK;
    XGPN_ACTP_ST_CHG_INTE_t  reg_sts;

    CA_ASSERT_RET(sts, CA_E_PARAM);

    reg_sts.wrd = AAL_PON_REG_READ(XGPN_ACTP_ST_CHG_INTE);

    sts->deregst              = reg_sts.bf.deregstE             ;
    sts->loss_sync            = reg_sts.bf.loss_syncE           ;
    sts->ds_synced            = reg_sts.bf.ds_syncedE           ;
    sts->DWLCH_appropriate    = reg_sts.bf.DWLCH_appropriateE   ;
    sts->DWLCH_inappropriate  = reg_sts.bf.DWLCH_inappropriateE ;
    sts->disable_SN           = reg_sts.bf.disable_SNE          ;
    sts->deactivate_OID       = reg_sts.bf.deactivate_OIDE      ;
    sts->assign_OID           = reg_sts.bf.assign_OIDE          ;
    sts->calibr_req_mismatch  = reg_sts.bf.calibr_req_mismatchE ;
    sts->TOZ_expire           = reg_sts.bf.TOZ_expireE          ;
    sts->TO1_expire           = reg_sts.bf.TO1_expireE          ;
    sts->assign_OID_inconsist = reg_sts.bf.assign_OID_inconsistE;
    sts->assign_EqD           = reg_sts.bf.assign_EqDE          ;
    sts->tuning_request_ack   = reg_sts.bf.tuning_request_ackE  ;
    sts->tuning_request_tx    = reg_sts.bf.tuning_request_txE   ;
    sts->tuning_request_rx    = reg_sts.bf.tuning_request_rxE   ;
    sts->enable_SN            = reg_sts.bf.enable_SNE           ;
    sts->us_tuning_confirm    = reg_sts.bf.us_tuning_confirmE   ;
    sts->TO2_expire           = reg_sts.bf.TO2_expireE          ;
    sts->TO3_expire           = reg_sts.bf.TO3_expireE          ;
    sts->TO4_expire           = reg_sts.bf.TO4_expireE          ;
    sts->TO5_expire           = reg_sts.bf.TO5_expireE          ;
    sts->ds_synced_O6         = reg_sts.bf.ds_synced_O6E        ;

    return ret;

}

/**
 * @aal_xgpon_actp_sync_hunt_set
 * Set the 125us based counter threshold when ONU is searching for
 * DS synchronization in O1.1 or O8.1 state.
 */
ca_status_t aal_xgpon_actp_sync_hunt_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    sync_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_SYNC_HUNT, threshold, sync_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_sync_hunt_get
 * Set the 125us based counter threshold when ONU is searching for
 * DS synchronization in O1.1 or O8.1 state.
 */
ca_status_t aal_xgpon_actp_sync_hunt_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *sync_threshold)
{
    CA_ASSERT_RET(sync_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_SYNC_HUNT, threshold, *sync_threshold);

    return CA_E_OK;
}


/**
 * @aal_xgpon_actp_toz_set
 * Set the 125us based counter threshold when ONU is in O2-3 state.
 * Timer TOZ controls the duration of an ONU discovery attempt
 * in the Serial Number state (O2-3), forcing a transition to the
 * Initial state (O1) if the discovery feedback from the OLT CT is lacking.
 */
ca_status_t aal_xgpon_actp_toz_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    toz_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TOZ, threshold, toz_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_toz_get
 * Get the 125us based counter threshold when ONU is in O2-3 state.
 */
ca_status_t aal_xgpon_actp_toz_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *toz_threshold)
{
    CA_ASSERT_RET(toz_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TOZ, threshold, *toz_threshold);

    return CA_E_OK;
}


/**
 * @aal_xgpon_actp_to1_set
 * Set the 125us based counter threshold when ONU is in O4 state.
 * Timer TO1 is used to abort an unsuccessful activation attempt by
 * limiting the overall time an ONU can remain in the Ranging state (O4).
 * The recommended initial value of TO1 is 10 seconds.
 */
ca_status_t aal_xgpon_actp_to1_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to1_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO1, threshold, to1_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_to1_get
 * Get the 125us based counter threshold when ONU is in O4 state.
 */
ca_status_t aal_xgpon_actp_to1_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to1_threshold)
{
    CA_ASSERT_RET(to1_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO1, threshold, *to1_threshold);

    return CA_E_OK;
}


/**
 * @aal_xgpon_actp_to23_set
 * Set the 125us based counter threshold when ONU is in O6 state.
 * Timer TO2 is used to assert a failure to recover from an intermittent LODS
 * condition by limiting the time an ONU can remain in the Intermittent LODS
 * state (O6).
 * Timer TO3 is used to initiate wavelength channel protection handover in
 * the Intermittent LODS state.
 */
ca_status_t aal_xgpon_actp_to23_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to23_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO23, threshold, to23_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_to23_get
 * Get the 125us based counter threshold when ONU is in O6 state.
 */
ca_status_t aal_xgpon_actp_to23_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to23_threshold)
{
    CA_ASSERT_RET(to23_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO23, threshold, *to23_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_to4_set
 * Set the 125us based counter threshold when ONU is in O8 state.
 * Timer TO4 is used to abort an unsuccessful wavelength channel tuning
 * operation in state O8, when no suitable downstream wavelength channel
 * is found. For suggestions on the initial value of TO4, see clause 18.1.
 */
ca_status_t aal_xgpon_actp_to4_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to4_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO4, threshold, to4_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_to4_get
 * Get the 125us based counter threshold when ONU is in O8 state.
 */
ca_status_t aal_xgpon_actp_to4_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to4_threshold)
{
    CA_ASSERT_RET(to4_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO4, threshold, *to4_threshold);

    return CA_E_OK;
}


/**
 * @aal_xgpon_actp_to5_set
 * Set the 125us based counter threshold when ONU is in O9 state.
 * Timer TO5 controls the duration of an upstream wavelength channel tuning
 * attempt in state O9, forcing a transition to the Initial state (O1)
 * if the upstream tuning feedback from the OLT CT is lacking.
 */
ca_status_t aal_xgpon_actp_to5_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to5_threshold)
{

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO5, threshold, to5_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_to5_get
 * Get the 125us based counter threshold when ONU is in O9 state.
 */
ca_status_t aal_xgpon_actp_to5_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to5_threshold)
{
    CA_ASSERT_RET(to5_threshold != NULL, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO5, threshold, *to5_threshold);

    return CA_E_OK;
}

/**
 * @aal_xgpon_actp_init
 * Initialize the XGPON Activation Processing ASIC sub-module.
 */
ca_status_t aal_xgpon_actp_init(ca_device_id_t device_id)
{
    ca_status_t                ret = CA_E_OK;
    aal_xgpon_actp_ctrl_mask_t actp_ctrl_mask;
    aal_xgpon_actp_ctrl_t      actp_ctrl;
    aal_xgpon_actp_status_mask_t actp_status_mask;
    aal_xgpon_actp_status_t    actp_status;

    actp_ctrl_mask.u32_data = 0;
    memset(&actp_ctrl, 0, sizeof(actp_ctrl));

    /* HW mode for activation process, S/W only check the state and clear the status */
    actp_ctrl_mask.s.sw_mode            = 1;
    actp_ctrl_mask.s.response_threshold = 1;
    actp_ctrl.sw_mode                   = FALSE;
    actp_ctrl.response_threshold        = AAL_XGPON_ACTP_RESPONSE_THRESHOLD;
    (void)aal_xgpon_actp_ctrl_set(device_id, actp_ctrl_mask, &actp_ctrl);

    /* Init HW thresholds */
    (void)aal_xgpon_actp_toz_set(device_id, g_xgpn_basic_info.toz);
    (void)aal_xgpon_actp_to1_set(device_id, AAL_XGPON_ACTP_TO1_THRESHOLD);
    (void)aal_xgpon_actp_to23_set(device_id, g_xgpn_basic_info.to23);
    (void)aal_xgpon_actp_to4_set(device_id, g_xgpn_basic_info.to4);
    (void)aal_xgpon_actp_to5_set(device_id, g_xgpn_basic_info.to5);
    (void)aal_xgpon_actp_sync_hunt_set(device_id, AAL_XGPON_ACTP_SYNC_HUNT_THRESHOLD);

    /* Clear old interrupts and disable all the State change interrupt */
    //AAL_PON_REG_WRITE(0U, XGPN_ACTP_ST_CHG_INTE);
    //AAL_PON_REG_WRITE(~0U, XGPN_ACTP_ST_CHG_INT);
    //AAL_PON_REG_WRITE(~0U, XGPN_ACTP_ST_CHG_INTE);

    memset(&actp_status, 0, sizeof(actp_status));

    ret = aal_xgpon_actp_status_get(device_id, &actp_status);
    if (CA_E_OK != ret) {
        return ret;
    }

    if (actp_status.onu_state) {
        ca_printf("Warning: HW initial state is NOT O1.1!\n");
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_CTRL, sw_mode, 1);
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, onu_sub_state, 0);
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_CTRL, sw_mode, 0);
    }

    actp_status_mask.u32_data = ~0U;
    memset(&actp_status, 0, sizeof(actp_status));
    ret = aal_xgpon_actp_status_set(device_id, actp_status_mask, &actp_status);
    //ca_printf("INFO: SW force init HW state to be O1.1!\n");

    /* Enable sub intrs */
    if (g_xgpn_basic_info.pon_mode == ONU_PON_MAC_NGPON2)
        AAL_PON_REG_WRITE(0xffffff0, XGPN_ACTP_ST_CHG_INTE);
    else
        AAL_PON_REG_WRITE(0x7a3dfff, XGPN_ACTP_ST_CHG_INTE);
    AAL_PON_REG_WRITE(0xfffffff, XGPN_ACTP_ST_CHG_INT);

    return ret;

}

#ifdef __KERNEL__

EXPORT_SYMBOL(aal_xgpon_actp_status_set);
EXPORT_SYMBOL(aal_xgpon_actp_status_get);

EXPORT_SYMBOL(aal_xgpon_actp_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_actp_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_actp_sync_hunt_set);
EXPORT_SYMBOL(aal_xgpon_actp_sync_hunt_get);
EXPORT_SYMBOL(aal_xgpon_actp_state_chg_get);

EXPORT_SYMBOL(aal_xgpon_actp_state_chg_enable_set);
EXPORT_SYMBOL(aal_xgpon_actp_state_chg_enable_get);
EXPORT_SYMBOL(aal_xgpon_actp_to5_get);
EXPORT_SYMBOL(aal_xgpon_actp_to5_set);
EXPORT_SYMBOL(aal_xgpon_actp_to4_get);
EXPORT_SYMBOL(aal_xgpon_actp_to4_set);
EXPORT_SYMBOL(aal_xgpon_actp_to23_get);
EXPORT_SYMBOL(aal_xgpon_actp_to23_set);
EXPORT_SYMBOL(aal_xgpon_actp_to1_get);
EXPORT_SYMBOL(aal_xgpon_actp_to1_set);
EXPORT_SYMBOL(aal_xgpon_actp_toz_get);
EXPORT_SYMBOL(aal_xgpon_actp_toz_set);
EXPORT_SYMBOL(aal_xgpon_actp_init);

#endif

