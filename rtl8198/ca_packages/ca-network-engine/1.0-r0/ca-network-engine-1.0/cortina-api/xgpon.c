
#include "osal_common.h"
#include "util_aes.h"
#include "ca_module.h"
#include "log.h"
#include "ca-scfg/scfg.h"
#include "aal_pon.h"
#include "aal_xgpon.h"
#include "aal_xgpon_fec.h"
#include "aal_xgpon_actp.h"
#include "aal_ngpon2_chmgmt.h"
#include "aal_ploam.h"
#include "aal_xcvr.h"
#include "ploam_ngpon2.h"
#include "xgpon.h"
#include "gpon.h"

#define __PM_UNSUPPORT_COUNTER_VAL  0 /* default PM counter if not support */

#define CA_NGP2_KEY_LEN_CHECK(val)         if(((val) != 16)){ \
                                                    ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val); \
                                                    return CA_E_PARAM; \
                                                }

#define NGP2_PORT_ID_CHECK(port)    do { \
                                        if (PORT_TYPE(port) != CA_PORT_TYPE_GPON) { \
                                            cap_log_msg(CA_XGPN_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_ERROR, "%s::%d param %s check fail\n", __FILE__, __LINE__, #port); \
                                            return CA_E_PARAM; \
                                        } \
                                    }while(0)

#define NGP2_ATTENUATION_CHECK(attenuation)  do { \
                                         if (attenuation < CA_NGP2_ATTENUATION_MAX) { \
                                            cap_log_msg(CA_XGPN_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_ERROR, "%s::%d param %s check fail\n", __FILE__, __LINE__, #attenuation); \
                                         } \
                                    } while(0)

#define NGP2_CHAN_ID_CHECK(channel)  do { \
                                         if (channel > CA_NGP2_MAX_CHANNELS) { \
                                            cap_log_msg(CA_XGPN_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_ERROR, "%s::%d param %s check fail\n", __FILE__, __LINE__, #channel); \
                                         } \
                                    } while(0)


ca_status_t ca_ngp2_port_pon_mac_init (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_pon_mode_t pon_mode,
    CA_IN ca_uint8_t serial_number[GPON_ONU_SERIAL_NUMBER_LENGTH],
	CA_IN ca_uint8_t registration_id[GPON_REGISTRATION_ID_LENGTH]
)
{
    ca_status_t ret = CA_E_OK;
	ca_uint32_t vendor_id = 0;
	ca_uint32_t vssn = 0;
	vendor_id = ((ca_uint32_t)serial_number[0] << 24) | ((ca_uint32_t)serial_number[1] << 16) | ((ca_uint32_t)serial_number[2] << 8) | ((ca_uint32_t)serial_number[3]);
	vssn = ((ca_uint32_t)serial_number[4] << 24) | ((ca_uint32_t)serial_number[5] << 16) | ((ca_uint32_t)serial_number[6] << 8) | ((ca_uint32_t)serial_number[7]);
	ret = aal_ploam_vendor_id_cfg_set(device_id,vendor_id);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_ploam_vssn_cfg_set(device_id, vssn);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_ploam_rgst_id_cfg_set(device_id, (aal_xgpn_plmp_rgst_id_cfg_t *)registration_id);
    CA_RET_VALUE_CHECK(ret);
    return ret;
}
ca_status_t ca_ngp2_port_pon_mac_reset (
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id)
{
    ca_status_t ret = CA_E_OK;
    aal_gpon_glb_ctrl_config_msk_t mask;
    aal_gpon_glb_ctrl_config_t glb_ctrl_cfg;
    mask.u32 = 0;
    memset(&glb_ctrl_cfg, 0 , sizeof(aal_gpon_glb_ctrl_config_t));
    mask.s.gpon_rst_n = 1 ;
    glb_ctrl_cfg.gpon_rst_n = 1 ;
    ret = aal_gpon_glb_ctrl_set(device_id,mask,&glb_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);
    return ret;
}


ca_status_t ca_ngp2_port_burst_profile_get(
    CA_IN       ca_device_id_t           device_id,
    CA_IN       ca_port_id_t             port_id,
    CA_IN       ca_uint8_t               burst_profile_id,
    CA_OUT      ca_ngp2_burst_profile_t *burst_profiles)
{
    ca_xgpn_burst_profile_t  burst_prof;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(burst_profiles);

    if (burst_profile_id > CA_XGPON_MAX_BURST_PROF_INDEX) {
        return CA_E_PARAM;
    }

    memset(&burst_prof, 0, sizeof(burst_prof));

    if (CA_E_OK != aal_xgpon_burst_profile_get(device_id, burst_profile_id, &burst_prof)) {
        goto __API_Exit;
    }

    /* since the structure is the same, just copy */
    memcpy(burst_profiles, &burst_prof, sizeof(ca_ngp2_burst_profile_t));

__API_Exit:

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: ca_ngp2_port_system_profile_get
**
**  Description:
**    It is used by system software for NGP2 system profile get.
**    System software should have the capability to store/retrieve ONU system
**    profile to/from persistent storage. NOTE: ca_ngp2_port_system_profile_set
**    will overwrite current system profile stored in persistent storage.
**    See ITU-T 989.3 table 12-1 for details.
**
**  Argument        Type                        IO        Description
**  --------------- --------------------------- -- ------------------------
**  port_id          ca_port_id_t               I  NGP2 port ID
**  system_profile   ca_ngp2_system_profile_t   O  system profile stored in ONT
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Junfeng       08/01/2016   Initial Writing
**  Lidong        06/12/2017   Finish according to G.989.3
**
****************************************************************************/

ca_status_t ca_ngp2_port_system_profile_get(
    CA_IN       ca_device_id_t             device_id,
    CA_IN       ca_port_id_t               port_id,
    CA_OUT      ca_ngp2_system_profile_t   *system_profile)
{
    ca_xgpn_sys_profile_t  sys_prof;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(system_profile);

    memset(&sys_prof, 0, sizeof(sys_prof));

    if (CA_E_OK != aal_xgpon_sys_profile_get(device_id, &sys_prof)) {
        goto __API_Exit;
    }

    /*
    system_profile->onu_id[CA_NGP2_ONU_ID_LEN]          = 0;
    system_profile->msg_type                            = 0;
    system_profile->seq_number                          = 0;
    */
    memcpy(system_profile->ng2sysid, sys_prof.ng2sysid, CA_NGP2_SYSTEM_PROFILE_SYSID_LENGTH);
    system_profile->version                             = sys_prof.version;
    system_profile->us_owl_bands                      = sys_prof.us_op_wl_bands;
    system_profile->twdm_channel_count                  = sys_prof.twdm_channel_count;
    system_profile->channel_spacing_twdm                = sys_prof.channel_spacing_twdm;
    system_profile->us_mse_twdm                         = sys_prof.us_mse_twdm;
    memcpy(system_profile->fsr_twdm, &sys_prof.fsr_twdm, CA_NGP2_SYSTEM_PROFILE_FSR_LENGTH);
    system_profile->twmd_amcc_control                   = sys_prof.twmd_amcc_control;
    system_profile->loose_calibration_bound_for_ptwdm_channels  = sys_prof.channel_loose_calibratiob_bound_twdm;
    system_profile->ptwdm_channel_count                 = sys_prof.ptp_wdm_channel_count;
    system_profile->channel_spacing_ptp                 = sys_prof.channel_spacing_ptp;
    system_profile->us_mse_ptp                          = sys_prof.us_mse_ptp;
    memcpy(system_profile->fsr_ptp, &sys_prof.fsr_ptp, CA_NGP2_SYSTEM_PROFILE_FSR_LENGTH);
    system_profile->ptp_calibration_accuracy            = sys_prof.ptp_calibration_accuracy;
    system_profile->loose_calibration_bound_ptp         = sys_prof.loose_calibration_bound_ptp;

__API_Exit:

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_channel_profile_get(
    CA_IN       ca_device_id_t                  device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_OUT      ca_uint8_t                      *channel_profile_count,
    CA_OUT      ca_ngp2_channel_profile_t       *channel_profile)

{
    ca_xgpn_twdm_channel_profile_t  chn_prof;
    ca_uint8_t channel_id = *channel_profile_count;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(channel_profile);

    if (channel_id >= CA_XGPON_MAX_CHAN_PROF) {
        return CA_E_PARAM;
    }

    memset(&chn_prof, 0, sizeof(chn_prof));

    if (CA_E_OK != aal_xgpon_channel_profile_get(device_id, channel_id, &chn_prof)) {
        goto __API_Exit;
    }

    // TODO: channel profile copy need copy one by one
    memcpy(&(channel_profile->channel_profile_index), &(chn_prof.channel_profile_idx), sizeof(ca_uint8_t));

__API_Exit:

    return CA_E_OK;
}

typedef struct {
    ca_xgp_power_management_parameters_t    twdm_pwm_param;
    ca_xgp_power_management_state_t         twdm_pwm_state;
    ca_boolean_t                     ca_ngp2_twdm_flag;
}ngp2_pwm_cb_t;

// TODO: should implement power management FSM
ngp2_pwm_cb_t  g_ngp2_pwm_cb = {{0}, CA_NGP2_PM_STATE_ACTIVEHOLD, TRUE };

ca_status_t ca_ngp2_port_power_mgmt_param_set(
    CA_IN       ca_device_id_t                              device_id,
    CA_IN       ca_port_id_t                                port_id,
    CA_IN       ca_xgp_power_management_sleep_mode_t        sleep_mode,
    CA_IN       ca_xgp_power_management_parameters_t        *power_mgmt_param)
{
    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(power_mgmt_param);

    memcpy(&g_ngp2_pwm_cb.twdm_pwm_param, power_mgmt_param, sizeof(ca_xgp_power_management_parameters_t));

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_power_mgmt_param_get(
    CA_IN       ca_device_id_t                          device_id,
    CA_IN       ca_port_id_t                            port_id,
    CA_OUT      ca_xgp_power_management_sleep_mode_t    *sleep_mode,
    CA_OUT      ca_xgp_power_management_parameters_t    *power_mgmt_param)
{
    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(power_mgmt_param);

    memcpy(power_mgmt_param, &g_ngp2_pwm_cb.twdm_pwm_param, sizeof(ca_xgp_power_management_parameters_t));

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_power_mgmt_state_get(
    CA_IN       ca_device_id_t                  device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_OUT      ca_xgp_power_management_state_t *power_mgmt_state)
{
    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(power_mgmt_state);


    *power_mgmt_state = g_ngp2_pwm_cb.twdm_pwm_state;
    return CA_E_OK;
}

ca_status_t ca_ngp2_port_power_local_event_set (
    CA_IN       ca_device_id_t                              device_id, 
    CA_IN       ca_port_id_t                                port_id,
    CA_OUT      ca_xgp_power_management_local_event_t       local_event)
{
    return CA_E_OK;
}


/****************************************************************************
**
**  Function Name: ca_ngp2_port_chan_attenuation_get
**
**  Description:
**    These functions are used by system software to get  NGP2 channel
**  attenuation level in PON transceiver. Channel attenuation level is stored
**  in channel profile. See UTI-T 989.3 section 11.2.6.7 for details..
**  Attenuation parameter represents a requested attenuation level as a part
**  of the power levelling instruction to an ONU, or an ONU's attenuation
**  level at the time of the message transmission as a part of the power
**  levelling report.
**
**  Argument        Type               IO        Description
**  --------------- ------------------ -- ------------------------
**  port_id          ca_port_id_t      I  NGP2 port ID
**  channel_id       ca_uint8_t        I  Channel ID
**  p_att_level      ca_uint8_t        O  Location where PON transevier attenuation level should be stored. Unit is 3db
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/12/2017   Finish according to API doc 06/08
**
****************************************************************************/
ca_status_t ca_ngp2_port_channel_attenuation_get(
    CA_IN       ca_device_id_t              device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_uint8_t                  channel_id,
    CA_OUT      ca_uint8_t                  *att_level)
{
    aal_xgpn_plmp_attenuation_t   attenuation;
    ca_status_t  ret = CA_E_OK;

    CA_NULL_PTR_CHECK(att_level);

    memset(&attenuation, 0, sizeof(aal_xgpn_plmp_attenuation_t));

    ret = aal_ploam_attenuation_get(device_id, &attenuation);
    if (CA_E_OK != ret)
        goto __API_Exit;

    *att_level = attenuation.level;

__API_Exit:

    return ret;

}

#if 0
/****************************************************************************
**
**  Function Name: ca_ngp2_port_chan_attenuation_set
**
**  Description:
**    These functions are used by system software to set NGP2 channel
**  attenuation level in PON transceiver. Channel attenuation level is stored
**  in channel profile. See UTI-T 989.3 section 11.2.6.7 for details..
**  Attenuation parameter represents a requested attenuation level as a part
**  of the power levelling instruction to an ONU, or an ONU's attenuation
**  level at the time of the message transmission as a part of the power
**  levelling report.
**
**  Argument        Type               IO        Description
**  --------------- ------------------ -- ------------------------
**  port_id          ca_port_id_t      I  NGP2 port ID
**  channel_id       ca_uint8_t        I  Channel ID
**  p_att_level      ca_uint8_t        O  Location where PON transevier attenuation level should be stored. Unit is 3db
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/12/2017   Finish according to API doc 06/08
**
****************************************************************************/
ca_status_t ca_ngp2_port_chan_attenuation_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_uint8_t                  channel_id,
    CA_IN       ca_uint8_t                  p_att_level)
{
    aal_xgpn_plmp_attenuation_mask_t mask;
    aal_xgpn_plmp_attenuation_t      attenuation;
    ca_status_t                      ret = CA_E_OK;

    NGP2_ATTENUATION_CHECK(p_att_level);

    mask.wrd = 0;
    mask.bf.level = 1;

    memset(&attenuation, 0, sizeof(aal_xgpn_plmp_attenuation_t));
    attenuation.level = p_att_level;

    ret = aal_ploam_attenuation_set(device_id, mask, &attenuation);
    if (CA_E_OK != ret)
        goto __API_Exit;

__API_Exit:

    return ret;

}
#endif


/****************************************************************************
**
**  Function Name: ca_ngp2_port_onu_power_leveling_cap_get
**
**  Description:
**    These functions are used by system software to get. NGP2 channel power
**  level capability of ONU. See UTI-T 989.3 section 11.2.6.8 for details.
**
**  Argument          Type               IO        Description
**  ---------------   ------------------ -- ------------------------
**  port_id            ca_port_id_t      I  NGP2 port ID
**  channel_id         ca_uint8_t        I  Channel ID
**  p_power_level_cap  ca_uint8_t        O  Location where PON transceiver power level capabiltiy should be stored. Unit is 3db
**
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/12/2017   Finish according to API doc 06/08
**
****************************************************************************/
ca_status_t ca_ngp2_port_onu_power_leveling_cap_get (
    CA_IN        ca_device_id_t            device_id,
    CA_IN        ca_port_id_t              port_id,
    CA_OUT       ca_uint8_t                *power_level_cap)
{
    aal_xgpn_plmp_attenuation_t   attenuation;
    ca_status_t  ret = CA_E_OK;

    CA_NULL_PTR_CHECK(power_level_cap);

    memset(&attenuation, 0, sizeof(aal_xgpn_plmp_attenuation_t));

    ret = aal_ploam_attenuation_get(device_id, &attenuation);
    if (CA_E_OK != ret)
        goto __API_Exit;

    *power_level_cap = attenuation.capability;

__API_Exit:

    return ret;

}


/*
Power levelling capability is a seven-bit bitmap of the form 0CCC CCCC, whereby a bit in the K-th least significant position indicates that the ONU supports the attenuation level of 3K dB. For example, 0000 0010 indicates support of 6dB attenuation level.
*/
ca_status_t ca_ngp2_port_channel_power_consumption_get (
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        channel_id,
    CA_OUT ca_uint32_t      *power_consumption)
{
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    NGP2_CHAN_ID_CHECK(channel_id);
    CA_NULL_PTR_CHECK(power_consumption);

    ret = aal_ploam_power_consumption_get(device_id, channel_id, power_consumption);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;

}
#if 0 /* debug code to read from XCVR */
{
    ca_status_t  ret = CA_E_OK;
    ca_uint16_t  tx_power = 0, rx_power = 0;
    ca_uint32_t  up_chn = 0, dn_chn = 0;

    CA_NULL_PTR_CHECK(p_power_consumption);

    (void)aal_xgpn_current_chan_get(device_id, &up_chn, &dn_chn);

    if (channel_id == up_chn) {
        ret = aal_xcvr_tx_power_get(device_id, &tx_power);
        if (CA_E_OK != ret)
            goto __API_Exit;
    }

    if (channel_id == dn_chn) {
        ret = aal_xcvr_rx_power_get(device_id, &rx_power);
        if (CA_E_OK != ret)
            goto __API_Exit;
    }
#if 0
    if (tx_power != 0)
    {
        tx_fuw = log10(tx_power) * 10 - 40;
    }
    if (rx_power != 0)
    {
        rx_fuw = log10(rx_power) * 10 - 40;
    }
#endif
    /*
        XXXXXXXX XXXX XXXX - Power consumption of associated TWDM channel,
        with the unit of milliwatt (maximal value is 65.535W 65535 means 65.535W).
    */
    *p_power_consumption = ((ca_uint32_t)rx_power << 16) + rx_power;
__API_Exit:

    return CA_E_OK;

}
#endif


ca_status_t ca_ngp2_port_channel_power_consumption_set (
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        channel_id,
    CA_IN ca_uint32_t       power_consumption)
{
    ca_status_t ret = CA_E_OK;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    NGP2_CHAN_ID_CHECK(channel_id);

    ret = aal_ploam_power_consumption_set(device_id, channel_id, power_consumption);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;

}



ca_status_t ca_ngp2_port_registration_id_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_OUT      ca_uint8_t              *registration_id)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpn_plmp_rgst_id_cfg_t rgst_id;

    CA_NULL_PTR_CHECK(registration_id);

    memset(&rgst_id, 0 ,sizeof(aal_xgpn_plmp_rgst_id_cfg_t));
    ret = aal_ploam_rgst_id_cfg_get(device_id,&rgst_id);
    CA_RET_VALUE_CHECK(ret);
    memcpy(registration_id,rgst_id.data,XGPN_PLMP_RGST_ID_SIZE);
    return ret;
}

ca_status_t ca_ngp2_port_registration_id_set(
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_port_id_t            port_id,
    CA_IN       ca_uint8_t              *registration_id)
{
    ca_status_t ret = CA_E_OK;
    //aal_xgpn_plmp_rgst_id_cfg_t rgst_id;

    CA_NULL_PTR_CHECK(registration_id);

    //memset(&rgst_id, 0 ,sizeof(aal_xgpn_plmp_rgst_id_cfg_t));
    //memcpy(rgst_id.data,registration_id,XGPN_PLMP_RGST_ID_SIZE);
    //ret = aal_ploam_rgst_id_cfg_set(device_id,&rgst_id);
    //CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID0, sizeof(ca_uint32_t),  registration_id);
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID1, sizeof(ca_uint32_t),  (registration_id+4));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID2, sizeof(ca_uint32_t),  (registration_id+8));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID3, sizeof(ca_uint32_t),  (registration_id+12));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID4, sizeof(ca_uint32_t),  (registration_id+16));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID5, sizeof(ca_uint32_t),  (registration_id+20));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID6, sizeof(ca_uint32_t),  (registration_id+24));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID7, sizeof(ca_uint32_t),  (registration_id+28));
    CA_RET_VALUE_CHECK(ret);
    ret = scfg_write(device_id, CFG_ID_PON_REGISTRATION_ID8, sizeof(ca_uint32_t),  (registration_id+32));
    CA_RET_VALUE_CHECK(ret);


    return ret;
}

#if 0
ca_status_t ca_ngp2_port_uc_enc_key_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_ngp2_enc_key_id_t        key_idx,
    CA_OUT      ca_boolean_t                *p_key_active,
    CA_OUT      ca_uint8_t                  *p_effective_key_length,
    CA_OUT      ca_uint8_t                  *uc_key)
{
    ca_status_t ret = CA_E_OK;
    aal_uc_enc_key_config_t uc_key_cfg;
    CA_XGPON_KEY_IDX_CHECK(key_idx);
    memset(&uc_key_cfg,0, sizeof(aal_uc_enc_key_config_t));

    ret = aal_xgpon_aes_uc_enc_key_get(device_id,key_idx,&uc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    *p_key_active = uc_key_cfg.p_key_active;
    *p_effective_key_length = uc_key_cfg.p_effective_key_length;
    memcpy(uc_key, uc_key_cfg.uc_key,CA_NGP2_UC_KEY_LEN);
    return ret;
}


ca_status_t ca_ngp2_port_uc_enc_key_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_ngp2_enc_key_id_t        key_idx,
    CA_IN       ca_boolean_t                key_active,
    CA_IN       ca_uint8_t                  effective_key_length,
    CA_IN       ca_uint8_t                  *uc_key)
{
    ca_status_t ret = CA_E_OK;
    aal_uc_enc_key_config_msk_t uc_key_cfg_msk;
    aal_uc_enc_key_config_t uc_key_cfg;
    uc_key_cfg_msk.u32 = 0;
    memset(&uc_key_cfg,0, sizeof(aal_uc_enc_key_config_t));
    memcpy(uc_key_cfg.uc_key, uc_key,CA_NGP2_UC_KEY_LEN);
    uc_key_cfg.p_effective_key_length = effective_key_length;
    uc_key_cfg.p_key_active = key_active;
    ret = aal_xgpon_aes_uc_enc_key_set(device_id, key_idx,uc_key_cfg_msk,&uc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}
#endif
ca_status_t ca_ngp2_port_mc_encryption_key_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_ngp2_encryption_key_id_t key_index,
    CA_OUT      ca_uint8_t                  *effective_key_length,
    CA_OUT      ca_uint8_t                  *mc_key)
{
    ca_status_t ret = CA_E_OK;
    aal_mc_enc_key_config_t mc_key_cfg;
    memset(&mc_key_cfg,0, sizeof(aal_mc_enc_key_config_t));
    if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON){
        ret = aal_xgpon_aes_mc_enc_key_get(device_id,key_index,&mc_key_cfg);
        CA_RET_VALUE_CHECK(ret);

        *effective_key_length = mc_key_cfg.p_effective_key_length;
        memcpy(mc_key, mc_key_cfg.mc_key,CA_NGP2_BC_KEY_LEN);
    }else{
    //gpon
    }
    return ret;
}


ca_status_t ca_ngp2_port_mc_encryption_key_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_ngp2_encryption_key_id_t key_index,
    CA_IN       ca_mc_encryption_key_flag_t key_flags,
    CA_IN       ca_uint8_t                  effective_key_length,
    CA_IN       ca_uint8_t                  *mc_key)
{
    ca_status_t ret = CA_E_OK;
    aal_mc_enc_key_config_msk_t mc_key_cfg_msk;
    aal_mc_enc_key_config_t mc_key_cfg;
    aal_xgpon_dsfp_xgem_mc_key_ctrl_config_msk_t key_ctrl_cfg_msk;
    aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t     key_ctrl_cfg;
    ca_uint8_t                                   decryptKey[CA_NGP2_BC_KEY_LEN] = {0};
    ca_uint8_t                                   mckey[CA_NGP2_BC_KEY_LEN] = {0};

    CA_XGPON_KEY_IDX_CHECK(key_index);

    aes128_ecb_decrypt(kek, mc_key, decryptKey);
    memset(&key_ctrl_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t));

    mc_key_cfg_msk.u32 = ~0U;
    memset(&mc_key_cfg,0, sizeof(aal_mc_enc_key_config_t));
    memcpy(mc_key_cfg.mc_key, decryptKey, CA_NGP2_BC_KEY_LEN);
    mc_key_cfg.p_effective_key_length = effective_key_length;

    ret = aal_xgpon_dsfp_xgem_mc_key_ctrl_get(device_id,&key_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);
    if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON){
        switch(key_flags){
            case CA_MC_FLAG_MC_ENCRYPTION_KEY_SET:
                if(key_index == CA_NGP2_KEY_INDEX_0){
                    if(!key_ctrl_cfg.key0_idx_vld){
                        mc_key_cfg.p_key_active = 1;
                        ret = aal_xgpon_aes_mc_enc_key_set(device_id, CA_AAL_XGPON_KEY_IDX_0, mc_key_cfg_msk,&mc_key_cfg);
                        CA_RET_VALUE_CHECK(ret);
                    }
                }
                if(key_index == CA_NGP2_KEY_INDEX_1){
                    if(!key_ctrl_cfg.key1_idx_vld){
                        mc_key_cfg.p_key_active = 1;
                        ret = aal_xgpon_aes_mc_enc_key_set(device_id, CA_AAL_XGPON_KEY_IDX_1, mc_key_cfg_msk,&mc_key_cfg);
                        CA_RET_VALUE_CHECK(ret);
                    }
                }
            break;
            case CA_MC_FLAG_MC_ENCRYPTION_KEY_CLEAR:
                if(key_index == CA_NGP2_KEY_INDEX_0){
                    if(key_ctrl_cfg.key0_idx_vld){
                        mc_key_cfg.p_key_active = 0;
                        ret = aal_xgpon_aes_mc_enc_key_set(device_id, CA_AAL_XGPON_KEY_IDX_0, mc_key_cfg_msk,&mc_key_cfg);
                        CA_RET_VALUE_CHECK(ret);
                    }
                }
                if(key_index == CA_NGP2_KEY_INDEX_1){
                    if(key_ctrl_cfg.key1_idx_vld){
                        mc_key_cfg.p_key_active = 0;
                        ret = aal_xgpon_aes_mc_enc_key_set(device_id, CA_AAL_XGPON_KEY_IDX_1, mc_key_cfg_msk,&mc_key_cfg);
                        CA_RET_VALUE_CHECK(ret);
                    }
                }
            break;
            case CA_MC_FLAG_MC_ENCRYPTION_KEY_TBL_FLUSH:
                key_ctrl_cfg_msk.u32 = ~0U;
                key_ctrl_cfg.key0_idx_vld = 0;
                key_ctrl_cfg.key1_idx_vld = 0;
                ret = aal_xgpon_dsfp_xgem_mc_key_ctrl_set(device_id, key_ctrl_cfg_msk, &key_ctrl_cfg);
                CA_RET_VALUE_CHECK(ret);
                
            break;
            default:
            break;
        }

        #if 0

        if(!key_ctrl_cfg.key0_idx_vld){
            ret = aal_xgpon_aes_mc_enc_key_set(device_id, 0,mc_key_cfg_msk,&mc_key_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        if(!key_ctrl_cfg.key1_idx_vld){
            ret = aal_xgpon_aes_mc_enc_key_set(device_id, 1,mc_key_cfg_msk,&mc_key_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        #endif
    }else{
    //gpon
    }

    return ret;
}

ca_status_t ca_ngp2_port_uc_enc_active_key_idx_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_OUT      ca_uint8_t                  *p_key_idx)
{
    ca_status_t ret = CA_E_OK;
    ret = aal_xgpon_usbg_aes_uni_key_idx_get(device_id,p_key_idx);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_ngp2_port_uc_enc_active_key_idx_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_uint8_t                  key_idx)
{
    ca_status_t ret = CA_E_OK;
    CA_XGPON_KEY_IDX_CHECK(key_idx);
    ret = aal_xgpon_usbg_aes_uni_key_idx_set(device_id,key_idx);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_ngp2_port_mc_enc_active_key_idx_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_OUT      ca_uint8_t                  *p_key_idx)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t key_ctrl_cfg;
    memset(&key_ctrl_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t));
    ret = aal_xgpon_dsfp_xgem_mc_key_ctrl_get(device_id,&key_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);
    if(key_ctrl_cfg.key0_idx_vld)
    {
        *p_key_idx = key_ctrl_cfg.key0_idx_vld;
    }else if(key_ctrl_cfg.key1_idx_vld)
    {
        *p_key_idx = key_ctrl_cfg.key1_idx_vld;
    }
    return ret;
}

ca_status_t ca_ngp2_port_mc_enc_active_key_idx_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_uint8_t                  key_idx)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_dsfp_xgem_mc_key_ctrl_config_msk_t mask;
    aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t key_ctrl_cfg;
    CA_XGPON_KEY_IDX_CHECK(key_idx);
    mask.u32 =0;
    memset(&key_ctrl_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t));
    if(key_idx == 0)
    {
       mask.s.key0_idx_vld = 1;
       key_ctrl_cfg.key0_idx_vld = 1;
    }else if(key_idx == 1)
    {
       mask.s.key1_idx_vld = 1;
       key_ctrl_cfg.key1_idx_vld = 1;
    }
    ret = aal_xgpon_dsfp_xgem_mc_key_ctrl_set(device_id,mask,&key_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);
    return CA_E_OK;
}

#if 0
ca_status_t ca_ngp2_port_onu_state_get(
    CA_IN       ca_device_id_t                     device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_OUT      ca_ngp2_onu_act_state_t     *p_onu_act_state)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_actp_status_t sts;
    memset(&sts, 0, sizeof(aal_xgpon_actp_status_t));
    ret =  aal_xgpon_actp_status_get(device_id, &sts);
    CA_RET_VALUE_CHECK(ret);
    switch(sts.onu_state)
    {
        case 0:
            *p_onu_act_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O1_1 : CA_NGP2_ONU_ACT_STATE_O1_2);
            break;
        case 1:
            *p_onu_act_state = CA_NGP2_ONU_ACT_STATE_O2_3;
            break;
        case 2:
            *p_onu_act_state = CA_NGP2_ONU_ACT_STATE_O4;
            break;
        case 3:
            *p_onu_act_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O5_1 : CA_NGP2_ONU_ACT_STATE_O5_2 );
            break;
        case 4:
            *p_onu_act_state = CA_NGP2_ONU_ACT_STATE_O6;
            break;
        case 5:
            *p_onu_act_state = CA_NGP2_ONU_ACT_STATE_O7;
            break;
        case 6:
            *p_onu_act_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O8_1 : CA_NGP2_ONU_ACT_STATE_O8_2);
            break;
        case 7:
            *p_onu_act_state = CA_NGP2_ONU_ACT_STATE_O9;
            break;
        default:
            ca_printf("ERROR:Unknown state!!/r/n");
            break;
    }
    return ret;
}
#endif


/****************************************************************************
**
**  Function Name: ca_ngp2_port_tuning_capability_get
**
**  Description:
**    These functions are used by system software for NGP2 ONU to get tuning
**  capability of ONU transceiver. See UTI-T 989.3 section 17.2.1 for details.
**
**  - The tuning granularity of the ONU: the largest applicable value over the
**    operating frequency band and the upstream rate, expressed in GHz. The
**    tuning granularity of the ONU should be equal to or finer than the tuning
**    granularity specified for the particular upstream interface in [ITU-T G.989.2], clause 11.1.4.
**  - The tuning time for a single granularity step: the largest value over the
**    operating frequency band and the tuning direction. The reported tuning
**    time corresponds to reaching at least 90% of full granularity step,
**    includes all significant transients, and is expressed in the units of PHY frames.
**
**  Argument        Type                           IO        Description
**  --------------- ------------------------------ -- ------------------------
**  port_id          ca_port_id_t                  I  NGP2 port ID
**  p_tuning_cap_params ca_ngp2_tuning_cap_param_t O  Location where channel tuning granularity parameters are stored
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/12/2017   Finish according to API doc 06/08
**
****************************************************************************/
ca_status_t ca_ngp2_port_tuning_granularity_get(
    CA_IN       ca_device_id_t                          device_id,
    CA_IN       ca_port_id_t                            port_id,
    CA_OUT      ca_ngp2_tuning_capability_parameters_t  *tuning_cap_params)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpn_plmp_tuning_parameter_t tuning_para;

    CA_NULL_PTR_CHECK(tuning_cap_params);

    /*
     * Driver will init the PLOAM_TUNING_PARAM if detect XCVR,
     * so just get the PARAM from PLOAM_TUNING_PARAM
     */
    memset(&tuning_para, 0 ,sizeof(aal_xgpn_plmp_tuning_parameter_t));
    ret = aal_ploam_tuning_parameter_get(device_id, &tuning_para);
    CA_RET_VALUE_CHECK(ret);

    tuning_cap_params->tuning_granularity = tuning_para.granularity;
    tuning_cap_params->tuning_step_time   = tuning_para.time;

    return CA_E_OK;
}



/****************************************************************************
**
**  Function Name: ca_ngp2_port_channel_tx_lamda_adjust_set
**
**  Description:
**    This function is used by system software for NGP2 ONU channel wavelength
**  adjustment. This API can be used for Adjust_Tx_Wavelength PLOAM handling.
**  See UTI-T 989.3 17.2.2, and table 11-14 for details..
**
**
**  Argument        Type                           IO        Description
**  --------------- ------------------------------ -- ------------------------
**  port_id          ca_port_id_t                  I  NGP2 port ID
**  channel_id         ca_uint8_t                    I  upstream channel id to adjust
**  up_down             ca_uint8_t                       I  0= up, 1 = down
**  p_freq_adjust     ca_uint16_t                   I  frequence to adjust (unit in 0.1GHz)
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/12/2017   Finish according to API doc 06/08
**
****************************************************************************/
ca_status_t ca_ngp2_port_channel_tx_lamda_adjust_set(
    CA_IN       ca_device_id_t           device_id,
    CA_IN       ca_port_id_t             port_id,
    CA_IN       ca_uint8_t               channel_id,
    CA_IN       ca_uint8_t               up_down,
    CA_IN       ca_uint16_t              freq_adjust)
{
    ca_uint16_t  freq_chn = 0;
    ca_status_t  ret = CA_E_OK;

/*
 * Channel number = 1 + (Desired Frequency-First Frequency)/Grid Spacing
 */
    ret = aal_xcvr_freq_chn_get(device_id, &freq_chn);
    CA_RET_VALUE_CHECK(ret);

    if (up_down) /* to down */
        freq_chn += freq_adjust;
    else if (freq_chn > freq_adjust)
        freq_chn -= freq_adjust;
    else
        freq_chn = 1;

    ret = aal_xcvr_freq_chn_set(device_id, freq_chn);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_downstream_sync_state_get(
    CA_IN        ca_device_id_t              device_id,
    CA_IN        ca_port_id_t                port_id,
    CA_OUT       ca_ngp2_onu_ds_sync_t      *sync_state)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t aal_val = 0;

    CA_NULL_PTR_CHECK(sync_state);

    ret = aal_xgpon_dsfp_sync_state_get(device_id,&aal_val);
    CA_RET_VALUE_CHECK(ret);

    *sync_state = aal_val;

    return ret;
}


/****************************************************************************
**
**  Function Name: ca_ngp2_twdm_protection_pon_id_get
**
**  Description:
**    These functions are used by system software for NGP2 protection PON ID get.
**  System software have the capability to store/retrieve ONU upstream and downstream PON ID to/from persistent storage.
**  See UTI-T 989.3 11.3.3.15  for details.
**
**
**  Argument        Type              IO        Description
**  --------------- ----------------- -- ------------------------
**  port_id          ca_port_id_t     I  NGP2 port ID
**  p_us_pon_id      ca_uint32_t      O  Location where upstream PON ID is stored
**  p_ds_pon_id      ca_uint32_t      O  Location where downstream PON ID is stored
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Lidong        06/15/2017   Finish according to API doc 06/14
**
****************************************************************************/
ca_status_t ca_ngp2_twdm_protection_pon_ids_get(
    CA_IN       ca_device_id_t           device_id,
    CA_IN       ca_port_id_t             port_id,
    CA_OUT      ca_uint32_t             *us_pon_id,
    CA_OUT      ca_uint32_t             *ds_pon_id)
{
    ca_status_t                    ret = CA_E_OK;
    ploam_ngpon2_protection_ctrl_t cfg;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    CA_NULL_PTR_CHECK(us_pon_id);
    CA_NULL_PTR_CHECK(ds_pon_id);

    memset(&cfg, 0, sizeof(ploam_ngpon2_ranging_time_t));
    ret = aal_ploam_ds_protection_ctrl_get(device_id, &cfg);
    CA_RET_VALUE_CHECK(ret);

    *us_pon_id = cfg.protect_us_pon_id;
    *ds_pon_id = cfg.protect_ds_pon_id;

    return ret;

}

ca_status_t ca_ngp2_port_pm_counters_get(
    CA_IN       ca_device_id_t                  device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_IN       ca_ngp2_pm_counter_types_t      counter_type,
    CA_IN       ca_uint32_t                     counter_buffer_size,
    CA_OUT      ca_uint8_t                      *counter_buffer)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  xgem_idx = 0;
    //ca_gal_ethernet_counters_t gal_eth_cnt;
    //ca_phy_los_pm_t phy_los_pm;
    ca_uint32_t bip32_data0 = 0;
    ca_uint32_t bip32_data1 = 0;
    ca_uint32_t bip32_err = 0;
    ca_uint32_t psbd_hec_err = 0;
    ca_uint32_t psbd_hec_uncorrect_cnt = 0;
    ca_uint32_t xgtc_hec_correct_cnt = 0;
    ca_uint32_t xgtc_hec_uncorrect_cnt = 0;
    aal_xgpon_fec_stats_t fec_stats;
    aal_xgpon_usbg_stats_t usbg_stats;
    //ca_uint32_t usbg_xgem_lf = 0;
    aal_xgpon_usbg_xgem_mib_config_t usbg_xgem_mib;
    aal_xgpon_dsfp_xgem_mib_config_t dsfp_xgem_mib;
    aal_ploam_rx_mib_t ploam_rx_mib;
    aal_ploam_tx_mib_t ploam_tx_mib;
    ca_uint32_t dsfp_xgem_hec_err1 = 0;
    ca_uint32_t dsfp_xgem_hec_err0 = 0;
    ca_uint32_t dsfp_xgem_los_word_cnt = 0;
    ca_uint32_t dsfp_xgem_err_key_cnt = 0;
    //ca_chan_tuning_pm1_t chan_tuning_pm1;
    //ca_chan_tuning_pm2_t chan_tuning_pm2;
    //ca_chan_tuning_pm3_t chan_tuning_pm3;
    //ca_omci_pm_t omci_pm;

    CA_NULL_PTR_CHECK(counter_buffer);
    CA_ASSERT_RET(counter_buffer_size > 0, CA_E_PARAM);

    //memset(&gal_eth_cnt , 0 ,sizeof(gal_eth_cnt));
    //memset(&phy_los_pm , 0, sizeof(phy_los_pm));
    //memset(&chan_tuning_pm1 , 0, sizeof(ca_chan_tuning_pm1_t));
   // memset(&chan_tuning_pm2 , 0, sizeof(ca_chan_tuning_pm2_t));
    //memset(&chan_tuning_pm3 , 0, sizeof(ca_chan_tuning_pm3_t));
    //memset(&omci_pm , 0, sizeof(omci_pm));

    memset(&fec_stats , 0, sizeof(aal_xgpon_fec_stats_t));
    memset(&usbg_xgem_mib , 0, sizeof(aal_xgpon_usbg_xgem_mib_config_t));
    memset(&dsfp_xgem_mib , 0, sizeof(aal_xgpon_dsfp_xgem_mib_config_t));

    switch(counter_type){
        case CA_COUNTERS_GAL_ETHERNET:
            if (counter_buffer_size < sizeof(ca_gal_ethernet_counters_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_gal_ethernet_counters_t *gal_eth_cnt = (ca_gal_ethernet_counters_t *)counter_buffer;
                gal_eth_cnt->interval_end_time   = __PM_UNSUPPORT_COUNTER_VAL;
                gal_eth_cnt->discarded           = __PM_UNSUPPORT_COUNTER_VAL;
                gal_eth_cnt->discarded_threshold = __PM_UNSUPPORT_COUNTER_VAL;
            }
            break;

        case CA_COUNTERS_PHY_LOS_PM_T:
            if (counter_buffer_size < sizeof(ca_phy_los_pm_t)) {
                return CA_E_PARAM;
            } else {
                ca_phy_los_pm_t *phy_los_pm = (ca_phy_los_pm_t *)counter_buffer;

                ret = aal_xgpon_dsfp_bip_dw0_get(device_id,&bip32_data0);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_bip_dw1_get(device_id,&bip32_data1);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_bip_err_get(device_id,&bip32_err);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_psbd_hec_err_get(device_id,&psbd_hec_err);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_psbd_hec_uncorrect_cnt_get(device_id,&psbd_hec_uncorrect_cnt);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_xgtc_hec_correct_cnt_get(device_id, &xgtc_hec_correct_cnt);
                CA_RET_VALUE_CHECK(ret);
                ret = aal_xgpon_dsfp_xgtc_hec_uncorrect_cnt_get(device_id,&xgtc_hec_uncorrect_cnt);
                CA_RET_VALUE_CHECK(ret);

                phy_los_pm->total_words_protected_by_bip =  ((ca_uint64_t)bip32_data1<<32)|bip32_data0;
                phy_los_pm->bip_error_count = bip32_err;
                phy_los_pm->psbd_hec_errors_corrected = psbd_hec_err;
                phy_los_pm->psbd_hec_errors_uncorrectable = psbd_hec_uncorrect_cnt;
                phy_los_pm->fs_header_errors_corrected = xgtc_hec_correct_cnt;
                phy_los_pm->fs_header_errors_uncorrectable = xgtc_hec_uncorrect_cnt;

            }

            break;

        case CA_COUNTERS_FEC_PM_T:

            if (counter_buffer_size < sizeof(ca_fec_pm_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_fec_pm_t *fec_pm = (ca_fec_pm_t *)counter_buffer;

                ret = aal_xgpon_fec_stats_get(device_id, &fec_stats);
                CA_RET_VALUE_CHECK(ret);
                fec_pm->corrected_fec_bytes = fec_stats.corrected_bytes_rx;
                fec_pm->corrected_fec_codewords = fec_stats.corrected_codewords_rx;
                fec_pm->uncorrected_fec_codewords = fec_stats. uncorrectable_codewords_rx;
                fec_pm->total_fec_codewords = fec_stats.total_codewords_rx;
            }
            break;

        case CA_COUNTERS_XGEM_PM_T:
            if (counter_buffer_size < sizeof(ca_xgem_pm_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_xgem_pm_t *xgem_pm = (ca_xgem_pm_t *)counter_buffer;
                for(xgem_idx = 1; xgem_idx <128; xgem_idx++){
                    ret = aal_xgpon_usbg_xgem_mib_get( device_id, xgem_idx,&usbg_xgem_mib);
                    CA_RET_VALUE_CHECK(ret);
                    ret = aal_xgpon_dsfp_xgem_mib_get( device_id, xgem_idx,&dsfp_xgem_mib);
                    CA_RET_VALUE_CHECK(ret);

                    xgem_pm->total_transmitted_xgem_frames += usbg_xgem_mib.frame_cnt;
                    xgem_pm->total_transmitted_bytes_in_non_idle_xgem_frames += usbg_xgem_mib.byte_cnt;
                    xgem_pm->total_received_xgem_frames += dsfp_xgem_mib.frame_cnt;;
                    xgem_pm->total_received_bytes_in_non_idle_xgem_frames += dsfp_xgem_mib.byte_cnt;
                }

                (void)aal_xgpon_usbg_stats_get(device_id,&usbg_stats);
                xgem_pm->transmitted_xgem_frames_with_lf = usbg_stats.xgem_lf0_cnt;
                (void)aal_xgpon_dsfp_xgem_err1_get(device_id,&dsfp_xgem_hec_err1);
                (void)aal_xgpon_dsfp_xgem_err0_get(device_id,&dsfp_xgem_hec_err0);
                xgem_pm->received_xgem_header_hec_errors =  ((ca_uint64_t)dsfp_xgem_hec_err1<<32)|dsfp_xgem_hec_err0;
                (void)aal_xgpon_dsfp_xgem_los_word_cnt_get(device_id,&dsfp_xgem_los_word_cnt);
                xgem_pm->fs_words_lost_to_xgem_header_hec_errors = dsfp_xgem_los_word_cnt;
                (void)aal_xgpon_dsfp_xgem_err_key_cnt_get(device_id,&dsfp_xgem_err_key_cnt);
                xgem_pm->xgem_encryption_key_errors = dsfp_xgem_err_key_cnt;
            }

            break;

        case CA_COUNTERS_PLOAM_PM1_T:
            if (counter_buffer_size < sizeof(ca_ploam_pm1_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_ploam_pm1_t *ploam_pm1 = (ca_ploam_pm1_t *)counter_buffer;

                memset(&ploam_rx_mib, 0, sizeof(ploam_rx_mib));
                ret = aal_ploam_rx_mib_get(device_id, &ploam_rx_mib);
                CA_RET_VALUE_CHECK(ret);

                ploam_pm1->ploam_mic_errors = ploam_rx_mib.mir_err_cnt;
                ploam_pm1->downstream_ploam_message_count = ploam_rx_mib.ds_rx_cnt;
                ploam_pm1->ranging_time_message_count = ploam_rx_mib.range_time_cnt;
                ploam_pm1->protection_control_message_count = ploam_rx_mib.protection_ctrl_cnt;
                ploam_pm1->adjust_tx_wavelength_message_count = ploam_rx_mib.adjust_tx_wavelength_cnt;
            }

            break;

        case CA_COUNTERS_PLOAM_PM2_T:
            if (counter_buffer_size < sizeof(ca_ploam_pm2_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_ploam_pm2_t *ploam_pm2 = (ca_ploam_pm2_t *)counter_buffer;

                memset(&ploam_rx_mib, 0, sizeof(ploam_rx_mib));
                ret = aal_ploam_rx_mib_get(device_id, &ploam_rx_mib);
                CA_RET_VALUE_CHECK(ret);

                ploam_pm2->system_profile_message_count = ploam_rx_mib.sys_profile_cnt;
                ploam_pm2->channel_profile_message_count = ploam_rx_mib.channel_profile_cnt;
                ploam_pm2->burst_profile_message_count = ploam_rx_mib.burst_profile_cnt;
                ploam_pm2->assign_onu_id_message_count = ploam_rx_mib.assign_onu_id_cnt;
                ploam_pm2->unsatisfied_adjust_tx_wavelength_requests = __PM_UNSUPPORT_COUNTER_VAL;
                ploam_pm2->deactivate_onu_id_message_count = ploam_rx_mib.deactive_onu_id_cnt;
                ploam_pm2->disable_serial_number_message_count = ploam_rx_mib.disable_sn_cnt;
                ploam_pm2->request_registration_message_count = ploam_rx_mib.request_regsitration_cnt;
                ploam_pm2->assign_alloc_id_message_count = ploam_rx_mib.assign_alloc_id_cnt;
                ploam_pm2->key_control_message_count = ploam_rx_mib.key_ctrl_cnt;
                ploam_pm2->sleep_allow_message_count = ploam_rx_mib.sleep_allow_cnt;

            }

            break;

        case CA_COUNTERS_PLOAM_PM3_T:
            if (counter_buffer_size < sizeof(ca_ploam_pm3_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_ploam_pm3_t *ploam_pm3 = (ca_ploam_pm3_t *)counter_buffer;

                memset(&ploam_tx_mib, 0, sizeof(ploam_tx_mib));
                ret = aal_ploam_tx_mib_get(device_id, &ploam_tx_mib);
                CA_RET_VALUE_CHECK(ret);

                ploam_pm3->upstream_ploam_message_count = ploam_tx_mib.us_tx_cnt;
                ploam_pm3->serial_number_onu_in_band_message_count = ploam_tx_mib.sn_onu_cnt;
                ploam_pm3->serial_number_onu_amcc_message_count = __PM_UNSUPPORT_COUNTER_VAL;
                ploam_pm3->registration_message_count = ploam_tx_mib.registration_cnt;
                ploam_pm3->acknowledgement_message_count = ploam_tx_mib.ack_cnt;
                ploam_pm3->sleep_request_message_count = ploam_tx_mib.sleep_request_cnt;
                ploam_pm3->tuning_response_ack_nack_msg_count = ploam_tx_mib.tuning_resp_ack_nack_cnt;
                ploam_pm3->tuning_response_complete_u_rollback_msg_count= ploam_tx_mib.tuning_resp_complete_u_rollback_cnt;
                ploam_pm3->power_consumption_report_message_count = ploam_tx_mib.power_consumption_report_cnt;
                ploam_pm3->change_power_level_parameter_error_count = __PM_UNSUPPORT_COUNTER_VAL;
            }

            break;

        case CA_COUNTERS_CHAN_TUNING_PM1_T:
            if (counter_buffer_size < sizeof(ca_chan_tuning_pm1_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_chan_tuning_pm1_t *chan_tuning_pm1 = (ca_chan_tuning_pm1_t *)counter_buffer;

                // TODO: get conts from s/w stats
                chan_tuning_pm1->tuning_control_requests_for_rx_only_or_rx_tx = 0;
                chan_tuning_pm1->tuning_control_requests_for_tx_only = 0;
                chan_tuning_pm1->tuning_control_requests_rejected_int_spc = 0;
                chan_tuning_pm1->tuning_control_requests_rejected_ds_all = 0;
                chan_tuning_pm1->tuning_control_requests_rejected_us_all = 0;
                chan_tuning_pm1->tuning_control_requests_ok_target_channel = 0;
                chan_tuning_pm1->tuning_control_requests_failed_to4_exp = 0;
                chan_tuning_pm1->tuning_control_requests_failed_to5_exp = 0;
                chan_tuning_pm1->tuning_control_requests_resolved_discret_ch = 0;
                chan_tuning_pm1->tuning_control_requests_rollback_com_ds = 0;
                chan_tuning_pm1->tuning_control_requests_rollback_ds_all = 0;
                chan_tuning_pm1->tuning_control_requests_rollback_us_all = 0;
                chan_tuning_pm1->tuning_control_requests_failed_reactivation = 0;
            }

            break;

        case CA_COUNTERS_CHAN_TUNING_PM2_T:
            if (counter_buffer_size < sizeof(ca_chan_tuning_pm2_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_chan_tuning_pm2_t *chan_tuning_pm2 = (ca_chan_tuning_pm2_t *)counter_buffer;

                // TODO: get conts from s/w stats
                chan_tuning_pm2->tuning_control_requests_rejected_ds_albl = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_ds_void = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_ds_part = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_ds_tunr = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_ds_lnrt = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_ds_lncd = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_albl = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_void = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_tunr = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_clbr = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_lktp = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_lnrt = 0;
                chan_tuning_pm2->tuning_control_requests_rejected_us_lncd = 0;
            }
            break;
        case CA_COUNTERS_CHAN_TUNING_PM3_T:
            if (counter_buffer_size < sizeof(ca_chan_tuning_pm3_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_chan_tuning_pm3_t *chan_tuning_pm3 = (ca_chan_tuning_pm3_t *)counter_buffer;

                // TODO: get conts from s/w stats
                chan_tuning_pm3->tuning_control_requests_rollback_ds_albl = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_ds_lktp = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_albl = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_void = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_tunr = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_lktp = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_lnrt = 0;
                chan_tuning_pm3->tuning_control_requests_rollback_us_lncd = 0;
            }

            break;

        case CA_COUNTERS_OMCI_PM_T:
            if (counter_buffer_size < sizeof(ca_omci_pm_t)) {
                ret = CA_E_PARAM;
            } else {
                ca_omci_pm_t *omci_pm = (ca_omci_pm_t *)counter_buffer;
                // TODO: should get in OMCI S/W, but not here
                ret = aal_xgpon_dsfp_rxmac_omci_frame_cnt_get(device_id, &bip32_data0);
                CA_RET_VALUE_CHECK(ret);
                omci_pm->omci_mic_error_count = bip32_data0;
            }

            break;

        default:
            break;
    }

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_pm_counter_clear(
    CA_IN       ca_device_id_t                  device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_IN       ca_ngp2_pm_counter_types_t      counter_type)
{
#if 0
    //ca_status_t ret = CA_E_OK;
    ca_gal_ethernet_counters_t gal_eth_cnt;
    ca_phy_los_pm_t phy_los_pm;
    ca_fec_pm_t fec_pm;
    ca_xgem_pm_t xgem_pm;
    ca_ploam_pm1_t ploam_pm1;
    ca_ploam_pm2_t ploam_pm2;
    ca_ploam_pm3_t ploam_pm3;
    ca_chan_tuning_pm1_t chan_tuning_pm1;
    ca_chan_tuning_pm2_t chan_tuning_pm2;
    ca_chan_tuning_pm3_t chan_tuning_pm3;
    ca_omci_pm_t omci_pm;

    memset(&gal_eth_cnt , 0, sizeof(ca_gal_ethernet_counters_t));
    memset(&phy_los_pm , 0, sizeof(ca_phy_los_pm_t));
    memset(&fec_pm , 0, sizeof(ca_fec_pm_t));
    memset(&xgem_pm , 0, sizeof(ca_xgem_pm_t));
    memset(&ploam_pm1 , 0, sizeof(ca_ploam_pm1_t));
    memset(&ploam_pm2 , 0, sizeof(ca_ploam_pm2_t));
    memset(&ploam_pm3 , 0, sizeof(ca_ploam_pm3_t));
    memset(&chan_tuning_pm1 , 0, sizeof(ca_chan_tuning_pm1_t));
    memset(&chan_tuning_pm2 , 0, sizeof(ca_chan_tuning_pm2_t));
    memset(&chan_tuning_pm3 , 0, sizeof(ca_chan_tuning_pm3_t));
    memset(&omci_pm , 0, sizeof(ca_omci_pm_t));
#endif

    switch(counter_type){
        case CA_COUNTERS_GAL_ETHERNET:
            break;
        case CA_COUNTERS_PHY_LOS_PM_T:
            break;
        case CA_COUNTERS_FEC_PM_T:
            break;
        case CA_COUNTERS_XGEM_PM_T:
            break;
        case CA_COUNTERS_PLOAM_PM1_T:
            break;
        case CA_COUNTERS_PLOAM_PM2_T:
            break;
        case CA_COUNTERS_PLOAM_PM3_T:
            break;
        case CA_COUNTERS_CHAN_TUNING_PM1_T:
            break;
        case CA_COUNTERS_CHAN_TUNING_PM2_T:
            break;
        case CA_COUNTERS_CHAN_TUNING_PM3_T:
            break;
        case CA_COUNTERS_OMCI_PM_T:
            break;

        default:
            break;
    }

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: ca_ngp2_port_channel_info_get
**
**  Description:
**    These functions are used by system software to get TWDM channel
**  information.
**
**  Argument        Type                        IO        Description
**  --------------- --------------------------- -- ------------------------
**  port_id          ca_port_id_t               I  NGP2 port ID
**  channel_id       ca_uint8_t                    I  TWDM channel id
**  p_channel_info   ca_channel_info_t          O  Pointer to location where channel information is stored
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Junfeng       08/01/2016   Initial Writing
**  Lidong        06/16/2017   Finish according to 06/14 API doc
**
****************************************************************************/
ca_status_t ca_ngp2_port_channel_info_get (
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                   port_id,
    CA_IN       ca_uint8_t                     channel_id,
    CA_OUT      ca_channel_info_t              *channel_info)
{
    ca_status_t  ret    = CA_E_OK;
    aal_xgpon_twdm_chn_info_t  twdm_chn_info;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    NGP2_CHAN_ID_CHECK(channel_id);
    CA_NULL_PTR_CHECK(channel_info);

    /*
     * For an active TWDM channel, this attribute identifies the downstream
     * wavelength channel in reference to Table 11-2/G.989.2.
     * For an inactive channel it has value 0xFF. (R) (mandatory) (1 byte)
    */

    memset(&twdm_chn_info, 0, sizeof(twdm_chn_info));
    ret = aal_xgpn_chn_info_get(device_id, channel_id, &twdm_chn_info);
    CA_RET_VALUE_CHECK(ret);

    channel_info->active_indication = twdm_chn_info.active_status;
    channel_info->operational_status = twdm_chn_info.oper_state;
    channel_info->ds_twdm_chid = twdm_chn_info.map_989dot2_ds_chn;
    channel_info->us_twdm_chid = twdm_chn_info.map_989dot2_us_chn;

#if 0
    ret = aal_xgpn_current_chan_get(device_id, &up_chn, &dn_chn);
    CA_RET_VALUE_CHECK(ret);

    if (up_chn != channel_id) {
        p_channel_info->active_indication  = FALSE;
        p_channel_info->operational_status = FALSE;
        p_channel_info->us_twdm_chid = 0xFF;
    } else {
        p_channel_info->active_indication  = FALSE;
        p_channel_info->operational_status = FALSE;
        p_channel_info->us_twdm_chid = (up_chn & 0x7) + 1; /* 0 based to 1 based */
    }

    if (dn_chn != channel_id) {
        p_channel_info->ds_twdm_chid = 0xFF;
    } else {
        p_channel_info->ds_twdm_chid = (dn_chn & 0x7) + 1; ;
    }

    p_channel_info->twdm_channel_id = channel_id + 1;
#endif
    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: ca_ngp2_port_channel_info_set
**
**  Description:
**    These functions are used by system software to set TWDM channel
**  information.
**
**  Argument        Type                        IO        Description
**  --------------- --------------------------- -- ------------------------
**  port_id          ca_port_id_t               I  NGP2 port ID
**  channel_id       ca_uint8_t                    I  TWDM channel id
**  p_channel_info   ca_channel_info_t          O  Pointer to location where channel information is stored
**
**  Return Value:
**    CA_E_OK               Operation complete successfully
**    CA_E_XXX              Operation Failed
**
**  HISTORY (Recommended but Optional):
**
**  Programmer    Date       Description of Revision
**  ------------- ---------- ------------------------------
**  Junfeng       08/01/2016   Initial Writing
**  Lidong        06/16/2017   Finish according to 06/14 API doc
**
****************************************************************************/
ca_status_t ca_ngp2_port_channel_info_set (
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                   port_id,
    CA_IN       ca_uint8_t                     channel_id,
    CA_IN       ca_channel_info_t              *channel_info)
{
    ca_status_t  ret    = CA_E_OK;
    aal_xgpon_twdm_chn_info_t  twdm_chn_info;

    CA_DEV_ID_CHECK(device_id);
    NGP2_PORT_ID_CHECK(port_id);
    NGP2_CHAN_ID_CHECK(channel_id);
    CA_NULL_PTR_CHECK(channel_info);

    /*
     * For an active TWDM channel, this attribute identifies the downstream
     * wavelength channel in reference to Table 11-2/G.989.2.
     * For an inactive channel it has value 0xFF. (R) (mandatory) (1 byte)
    */

    memset(&twdm_chn_info, 0, sizeof(twdm_chn_info));
    ret = aal_xgpn_chn_info_get(device_id, channel_id, &twdm_chn_info);
    CA_RET_VALUE_CHECK(ret);

    twdm_chn_info.active_status = channel_info->active_indication;
    twdm_chn_info.oper_state = channel_info->operational_status;
    twdm_chn_info.map_989dot2_ds_chn = channel_info->ds_twdm_chid;
    twdm_chn_info.map_989dot2_us_chn = channel_info->us_twdm_chid;

    ret = aal_xgpn_chn_info_set(device_id, channel_id, &twdm_chn_info);
    CA_RET_VALUE_CHECK(ret);

    return CA_E_OK;
}

ca_status_t ca_ngp2_port_channel_partition_index_get(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT ca_uint32_t      *cpi)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t chan_partition_index = 0;

    CA_NULL_PTR_CHECK(cpi);

    ret = scfg_read(device_id, CFG_ID_PON_NGPON2_CHAN_PARTITION_INDEX, sizeof(ca_uint8_t), (ca_uint8_t *) &chan_partition_index);
    CA_RET_VALUE_CHECK(ret);
    *cpi = chan_partition_index;
    return CA_E_OK;
}

ca_status_t ca_ngp2_port_channel_partition_index_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    cpi)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t chan_partition_index = 0;

    chan_partition_index = (ca_uint8_t)cpi;
    ret = scfg_write(device_id, CFG_ID_PON_NGPON2_CHAN_PARTITION_INDEX, sizeof(ca_uint8_t), (ca_uint8_t *) &chan_partition_index);
    CA_RET_VALUE_CHECK(ret);
    return CA_E_OK;
}

EXPORT_SYMBOL(ca_ngp2_port_pon_mac_reset);
EXPORT_SYMBOL(ca_ngp2_port_pon_mac_init);
EXPORT_SYMBOL(ca_ngp2_port_burst_profile_get);
EXPORT_SYMBOL(ca_ngp2_port_system_profile_get);
EXPORT_SYMBOL(ca_ngp2_port_channel_profile_get);
EXPORT_SYMBOL(ca_ngp2_port_power_mgmt_param_get);
EXPORT_SYMBOL(ca_ngp2_port_power_mgmt_param_set);
EXPORT_SYMBOL(ca_ngp2_port_power_mgmt_state_get);
EXPORT_SYMBOL(ca_ngp2_port_power_local_event_set);
EXPORT_SYMBOL(ca_ngp2_port_channel_attenuation_get);
//EXPORT_SYMBOL(ca_ngp2_port_chan_attenuation_set);
EXPORT_SYMBOL(ca_ngp2_port_channel_power_consumption_get);
EXPORT_SYMBOL(ca_ngp2_port_channel_power_consumption_set);
EXPORT_SYMBOL(ca_ngp2_port_onu_power_leveling_cap_get);
EXPORT_SYMBOL(ca_ngp2_port_registration_id_get);
EXPORT_SYMBOL(ca_ngp2_port_registration_id_set);
EXPORT_SYMBOL(ca_ngp2_port_mc_encryption_key_get);
EXPORT_SYMBOL(ca_ngp2_port_mc_encryption_key_set);
EXPORT_SYMBOL(ca_ngp2_port_tuning_granularity_get);
EXPORT_SYMBOL(ca_ngp2_port_channel_tx_lamda_adjust_set);
EXPORT_SYMBOL(ca_ngp2_port_downstream_sync_state_get);
EXPORT_SYMBOL(ca_ngp2_twdm_protection_pon_ids_get);
EXPORT_SYMBOL(ca_ngp2_port_pm_counters_get);
EXPORT_SYMBOL(ca_ngp2_port_pm_counter_clear);
EXPORT_SYMBOL(ca_ngp2_port_channel_info_get);
EXPORT_SYMBOL(ca_ngp2_port_channel_info_set);
EXPORT_SYMBOL(ca_ngp2_port_channel_partition_index_set);
EXPORT_SYMBOL(ca_ngp2_port_channel_partition_index_get);

