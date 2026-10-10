

#ifndef __AAL_NGPON2_CHMGMT_H__
#define __AAL_NGPON2_CHMGMT_H__

#include "ca_types.h"
#include "ca_module.h"
#include "osal_common.h"
#include "scfg.h"
#include "log.h"
#include "aal_pon.h"
#include "aal_xgpon_actp.h"
#include "aal_xgpon_intr.h"
#include "aal_xgpon.h"

#define CA_NGPON2_MAX_DWLCH     8

/*************************************below to be optimized*********************************/
#if 1
#define CA_XGPON_ONU_ID_LEN		2
#define CA_XGPON_MIC_LEN			8

//burst profile
#define CA_XGPON_BP_PRF_DELMT_LEN	8
#define CA_XGPON_BP_PRF_PREEM_LEN	8
#define CA_XGPON_BP_PRF_PONTAG_LEN	8
#define CA_XGPON_BP_PAD_LEN		7


typedef struct {
    ca_uint16_t		onu_id; /* see table 11-16 of ITU 989.3 */
    ca_uint8_t		msg_type;
    ca_uint8_t		seq_number;
    ca_uint8_t		version_profile_idx; /* up to 4 burst profiles supported */
    ca_uint8_t		up_fec_indication;
    ca_uint8_t		delimiter_len;
    ca_uint8_t		delimiter[CA_XGPON_BP_PRF_DELMT_LEN];
    ca_uint8_t		preamble_len;
    ca_uint8_t		preamble_repeat_count;
    ca_uint8_t		preamble[CA_XGPON_BP_PRF_PREEM_LEN];
    ca_uint8_t		pon_tag[CA_XGPON_BP_PRF_PONTAG_LEN];

} ca_xgpn_burst_profile_t;


/*
  NOTE: up to 4 burst profiles per ONU
  Burst profiles are not preserved across activation
 */
#define CA_XGPON_MAX_BP			4

//ca_xgpn_burst_profile_t burst_profiles[CA_XGPON_MAX_BP];
#endif


//system profile
#define CA_XGPON_SYS_PRF_SYSID_LEN	3
#define CA_XGPON_SYS_PRF_FSR_LEN	2
#define CA_XGPON_SYS_PRF_PAD_LEN	2


typedef struct {
    ca_uint32_t		learnt_flag;              /* flag to indicate whether profile is learnt  */
    //ca_uint8_t		onu_id[CA_XGPON_ONU_ID_LEN]; /* see table 11-16 of ITU 989.3 */
    //ca_uint8_t		msg_type;
    //ca_uint8_t		seq_number;
    ca_uint8_t		ng2sysid[CA_XGPON_SYS_PRF_SYSID_LEN]; //see section 6.1.5.2
    ca_uint8_t		version;
    ca_uint8_t		us_op_wl_bands;
    ca_uint8_t		twdm_channel_count;
    ca_uint8_t		channel_spacing_twdm;
    ca_uint8_t		us_mse_twdm;
    ca_uint16_t		fsr_twdm;
    ca_uint8_t		twmd_amcc_control;
    ca_uint8_t		channel_loose_calibratiob_bound_twdm;
    ca_uint8_t		ptp_wdm_channel_count;
    ca_uint8_t		channel_spacing_ptp;
    ca_uint8_t		us_mse_ptp;
    ca_uint16_t		fsr_ptp;
    ca_uint8_t		ptp_calibration_accuracy;
    ca_uint8_t		loose_calibration_bound_ptp;
    //ca_uint8_t		padding[CA_XGPON_SYS_PRF_PAD_LEN];
    //ca_uint8_t		mic[CA_XGPON_MIC_LEN];
} ca_xgpn_sys_profile_t;

//channel profile
#define CA_XGPON_CHAN_PRF_PON_ID_LEN		4
#define CA_XGPON_CHAN_PRF_DEF_RESP_LEN		4
#define CA_XGPON_CHAN_PRF_PHY_FC_LEN		2
#define CA_XGPON_CHAN_PRF_US_FREQ_LEN		4
#define CA_XGPON_CHAN_PRF_PAD_LEN		11

#define CA_NGPON2_CHAN_PRF_US_VOID_IND_BIT		    (0x1 << 0)
#define CA_NGPON2_CHAN_PRF_DS_VOID_IND_BIT		    (0x1 << 1)
#define CA_NGPON2_CHAN_PRF_THIS_CHANNEL_IND_BIT	    (0x1 << 2)

#define CA_NGPON2_CHAN_PRF_DS_RATE_10G		(0x0 << 5)  // 10G
#define CA_NGPON2_CHAN_PRF_DS_RATE_N10G		(0x1 << 5)  // 2.5G
#define CA_NGPON2_CHAN_PRF_DS_FEC_ON		(0x1 << 4)
#define CA_NGPON2_CHAN_PRF_DS_LINE_CODE		0       // NRZ

#define CA_NGPON2_CHAN_PRF_US_RATE_10G		(0x1 << 1)  // 10G supported
#define CA_NGPON2_CHAN_PRF_US_RATE_2G		(0x1 << 0)  // 2.5G supported

//channel caliberation recoird
typedef struct {
    ca_uint32_t		status_0 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_2 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_4 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_6 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_8 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_10 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_12 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_14 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_1 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_3 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_5 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_7 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_9 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_11 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_13 	: 2;	//see section 11.2.6.4
    ca_uint32_t		status_15 	: 2;	//see section 11.2.6.4
} ca_xgpn_chan_calibration_status_t;

typedef struct {
    ca_uint32_t admin_label: 28;
    ca_uint32_t dwlch_id: 4;	/*see section 10.1.1.3*/
} ca_xgpn_pon_id_t;

//PON MAC settings related to tuning
typedef struct {
    ca_uint32_t		granularity;	//unit in 1GHz (see section 11.2.6.5)
    ca_uint32_t		step;		//unit in PHY frame (see section 11.2.6.5)
} ca_xgpn_pon_mac_tuning_granularity_t;

#define CA_XGPONG_POWER_CONSUMPTION_RESP	24
//transceiver parameters related to tuning
typedef struct {
    ca_uint32_t		tuning_granularity;
    ca_uint32_t		tuning_step_time;
    ca_uint32_t		attenuation;
    ca_uint32_t		power_leveling;
    ca_uint8_t		reserved[256];
} ca_xgpn_tuning_param_t;

typedef struct {
	ca_uint8_t oper_type;
	ca_uint8_t attenuation;
	ca_uint8_t power_consumption[CA_XGPONG_POWER_CONSUMPTION_RESP];
} ca_xgpn_power_level_t;

typedef struct {
	ca_uint32_t	            target_pon_id;
	ca_uint8_t	            adj_direction;
	ca_uint16_t	            adj_freq;	//0.1GHz unit
    ca_uint8_t	            granularity;			//unit in 1GHz (see section 11.2.6.5)
    ca_uint8_t	            tuning_step_time;		//unit in PHY frame (see section
    ca_xgpn_power_level_t	power_leveling;
} ca_xgpn_calibration_record_t;


typedef struct {
	ca_uint8_t  rollback;
	ca_uint16_t schedule_sfc;
	ca_uint8_t  rate;
	ca_uint8    oper_code;
	ca_uint8_t  resp_code;
} ca_xgpn_rate_control_t;

//associated with calibration record for one of the us/ds channels
typedef struct {
    //ca_uint8_t		    channel_idx;
    //ca_uint32_t		    target_ds_pon_id; //pon_id associated with channel
    //ca_uint32_t		    target_us_pon_id;
    //ca_uint32_t		    TBD[256];
    ca_uint16_t		        calibration_record_status;
    ca_uint16_t		        frequency_adjustment_direction; /*0: adjust to lower frequency; 1: adjust to higher frequency */
    ca_uint32_t		        size_of_frequency_adjustment;   /* unit: 0.1GHz*/

    ca_uint8_t		        granularity;			//unit in 1GHz (see section 11.2.6.5)
    ca_uint8_t		        tuning_step_time;		//unit in PHY frame (see section
    ca_xgpn_power_level_t	power_leveling; //valid only for us channel
} ca_ngpon2_calibration_record_t;

typedef struct {
    ca_uint32_t		                learnt_flag;    /*indicate whether this profile is learnt or not   */
    //ca_uint8_t		            onu_id[CA_XGPON_ONU_ID_LEN]; /* see table 11-16 of ITU 989.3 */
    //ca_uint8_t		            msg_type;
    //ca_uint8_t		            seq_number;
    ca_uint8_t		                channel_profile_idx;
    ca_uint8_t		                channel_profile_version;
    ca_uint32_t		                pon_id; //see section 6.1.5.3
    ca_uint8_t		                ds_freq_offset;
    ca_uint8_t		                ds_rate;
    ca_uint8_t		                channel_partition;
    ca_uint32_t		                default_response_channel;//[CA_XGPON_CHAN_PRF_DEF_RESP_LEN];
    ca_uint8_t		                grant_offered;
    ca_uint8_t		                phy_frame_count[CA_XGPON_CHAN_PRF_PHY_FC_LEN];
    ca_uint8_t		                uwlch_id;
    ca_uint8_t		                us_freq[CA_XGPON_CHAN_PRF_US_FREQ_LEN];
    ca_uint8_t		                optical_link_type;
    ca_uint8_t		                us_rate;
    ca_uint8_t		                default_onu_attenuationt;
    ca_uint8_t		                response_threshold;
    //ca_uint8_t		            padding[CA_XGPON_CHAN_PRF_PAD_LEN];
    //ca_uint8_t		            mic[CA_XGPON_MIC_LEN];
    //ca_uint32_t		            configured_flag;  /* flag to indicate that the parameters in this profile have been configured to transceiver level API    */
    //ca_uint32_t		            last_running_flag;  /* flag to indicate that this record was used in running state for this ONU last time, to be updated when entering O5.1    */
    //ca_uint32_t		            last_running_configured_count;  /*to count how many times for last running profile configured to transceiver  */
    ca_uint16_t		                correlation_tag;
    ca_ngpon2_calibration_record_t	calibration_record_ds;     /*to be updated in O1.1 or O8, CalibrationRequest PLOAM.*/
    ca_ngpon2_calibration_record_t	calibration_record_us;     /*to be updated in O2-3 or O9, upon receiving the OLT CT feedback for the upstream wavelength channel, and Adjust_Tx_Wavelength PLOAM.*/
    //ca_uint32_t		target_calibration_flag; /*flag to indicate whether it is for target DS/US channels to be calibrated, update when config or tuning fails */
} ca_xgpn_twdm_channel_profile_t;

typedef struct {
    ca_uint32_t		vendor_id;
    ca_uint32_t		vssn;
    ca_uint32_t		current_us_pon_id;      /*current PON ID for upstream wavelength channel*/
    ca_uint16_t		correlation_tag;
    ca_uint32_t		target_ds_pon_id;
    ca_uint32_t		target_us_pon_id;
    ca_uint16_t		calibration_request_flag;   /*indicate whether calibration request to be handled */
} ca_ngpon2_calibration_request_t;

typedef struct {
    ca_uint8_t		operation_code;
    ca_uint16_t		scheduled_sfc;  /*scheduled superframe counter value*/
    ca_uint8_t		rollback_flag;      /*indicate whether configured; if TRUE, to be cleared in O5.1*/
    ca_uint32_t		target_ds_pon_id;
    ca_uint32_t		target_us_pon_id;
    ca_uint8_t		calibration_flag;
    ca_uint16_t		tuning_ctrl_flag;   /*indicate whether tuning request to be handled, to be cleared in O5.1*/
} ca_ngpon2_tuning_control_t;

typedef struct {
    ca_uint32_t		protection_ds_pon_id;
    ca_uint32_t		protection_us_pon_id;
    ca_uint32_t		protection_ctrl_flag;   /*indicate whether protection control to be handled */
} ca_ngpon2_protection_control_t;

typedef struct {
    ca_uint32_t		ds_pon_id;
    ca_uint32_t		us_pon_id;
    ca_uint32_t		ds_channel_profile_index;
    ca_uint32_t		us_channel_profile_index;
    ca_uint32_t		pre_running_configured_count;
} ca_ngpon2_channel_config_t;

#define CA_XGPON_DEFAULT_DWLCH_CHAN_COUNT		8
#define CA_XGPON_MAX_BURST_PROFILE_PER_CHANNEL	8

//default DWLCH configurations
typedef struct {
    ca_uint8_t		chan_id;	//see ITU989.3 section 6.1.5.4
    ca_uint32_t		frequency;
    ca_uint32_t		wavelength;
    ca_uint32_t		dwlch_id;
} ca_xgpn_default_dwlch_param_t;

//ca_xgpn_default_dwlch_param_t default_dwlch_config[CA_XGPON_DEFAULT_DWLCH_CHAN_COUNT];


/*************************************above to be optimized*********************************/

#define CA_XGPON_MAX_CHANNEL_PROFILE_COUNT	16  /*4 bit field in the standard */
#define CA_XGPON_MAX_BURST_PROFILE_COUNT	64  /*up to 8 profiles per channel, i.e., 64 */

#define CA_NGPON2_CALIBRATION_RECORD_CHANNEL_NOT_ANNOUNCED	0  /*00:	   channel unspecified (not announced via Channel_Profile) */
#define CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED           1  /*01:	   uncalibrated */
#define CA_NGPON2_CALIBRATION_RECORD_LOOSE              	2  /*10:	   loose */
#define CA_NGPON2_CALIBRATION_RECORD_SUFFICIENT         	3  /*11:	   sufficient */

#define CA_NGPON2_ACTP_STS_PROFILE_LEARNING             	0  /*2'b00: Profile learning */
#define CA_NGPON2_ACTP_STS_DWLCH_OK             	        1  /*2'b01: DWLCH OK */
#define CA_NGPON2_ACTP_STS_DWLCH_NOT_OK                  	2  /*2'b10: DWLCH not OK */
#define CA_NGPON2_ACTP_STS_RESERVED                      	3  /*others reserved */


typedef struct {
    ca_uint32_t		                learnt_flag;              /* flag to indicate whether profiles are learnt at this DS wavelength channel  */
    ca_uint32_t                     profile_evaluate_request;      /* if TRUE, profiles need to be evaluated; if FALSE, no evaluation */
    ca_uint32_t                     channel_profile_count_learnt;
    //ca_uint32_t		            last_running_flag;              /* flag to indicate that this record was used in running state for this ONU last time, to be updated when entering O5.1    */
    //ca_uint32_t		            target_calibration_flag;         /*flag to indicate whether it is the target channel to be calibrated */
    ca_xgpn_sys_profile_t		    system_profile;        /*system profile for this DS wavelength channel*/
    ca_xgpn_twdm_channel_profile_t	twdm_channel_profile[CA_XGPON_MAX_CHANNEL_PROFILE_COUNT];        /*channel profiles for this DS wavelength channel*/
    ca_xgpn_burst_profile_t		    burst_profile[CA_XGPON_MAX_BURST_PROFILE_COUNT];        /*burst profiles for this DS wavelength channel*/
} ca_ngpon2_profile_record_t;

/*to be updated to include all transceiver spec  */
typedef struct {
    ca_uint8_t		channel_partition_dn[CA_NGPON2_MAX_DWLCH];              /* channel_partition downstream.      */
    ca_uint8_t		channel_partition_up[CA_NGPON2_MAX_DWLCH];              /* channel_partition upstream.      */
    ca_uint16_t		laser_first_frequency_1;    /*Unit: THZ*/
    ca_uint16_t		laser_first_frequency_2;    /*Unit: 0.1GHZ*/
    ca_uint16_t		laser_last_frequency_1;    /*Unit: THZ*/
    ca_uint16_t		laser_last_frequency_2;    /*Unit: 0.1GHZ*/
    ca_uint16_t		laser_grid;    /*maximum supported grid spacing, Unit: 0.1GHZ*/
    ca_uint8_t		upstream_optical_link_type;              /* 2 bits, Link type unspecified, A, B or Both     */
    ca_uint8_t		upstream_line_rate;              /*  10G, 2.5G     */
    ca_uint8_t		dwlch_calibration_record[CA_NGPON2_MAX_DWLCH];        /* record DS calibration result before profile learnt, defalut value: 01--uncalibrated     */
    ca_uint8_t		dwlch_config_index;        /* record DS channel configured, default 0xFF     */
    ca_uint8_t		uwlch_config_index;        /* record US channel configured, default 0xFF     */
} ca_ngpon2_transceiver_t;

typedef struct {
    ca_uint32_t		us_channel_calibration_record_even;              /*   */
    ca_uint32_t		us_channel_calibration_record_odd;              /*   */
    ca_uint32_t		ds_channel_calibration_record_even;              /*   */
    ca_uint32_t		ds_channel_calibration_record_odd;              /*   */
    ca_uint32_t		channel_partition_index;              /*  one byte */
} ca_ngpon2_channel_calibration_record_t;

typedef struct {
    ca_ngpon2_profile_record_t		profile_record[1/*CA_NGPON2_MAX_DWLCH*/];      /* Profile record for each DS wavelength channel  */
    ca_ngpon2_calibration_request_t	calibration_request;
    ca_ngpon2_tuning_control_t		tuning_control;
    ca_ngpon2_protection_control_t	protection_control;
    ca_ngpon2_channel_config_t		channel_configured;    //
    ca_ngpon2_channel_config_t		pre_working_channel;	/*the previous working channel*/
    ca_ngpon2_transceiver_t			transceiver_cap;                /* Transceiver capability on this ONU. */
    ca_ngpon2_channel_calibration_record_t calibration_record;
} ca_ngpon2_channel_mgmt_t;

#if 0
ca_status_t aal_xgpon_cb_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_cb_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_partition_index);
ca_status_t aal_xgpon_sysprof_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_sysprof_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_count,
    CA_IN ca_uint8_t  channel_spacing,
    CA_IN ca_uint8_t  us_mse,
    CA_IN ca_uint16_t  fsr_twdm,
    CA_IN ca_uint16_t  loose_bound);
ca_status_t aal_xgpon_channelprof_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_channelprof_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  index,
    CA_IN ca_uint32_t  learnt_flag);
ca_status_t aal_xgpon_xfp_cap1_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_xfp_cap1_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint16_t  laser_first_frequency_1,
    CA_IN ca_uint16_t  laser_first_frequency_2,
    CA_IN ca_uint16_t  laser_last_frequency_1,
    CA_IN ca_uint16_t  laser_last_frequency_2,
    CA_IN ca_uint16_t  laser_grid,
    CA_IN ca_uint8_t  upstream_optical_link_type,
    CA_IN ca_uint8_t  upstream_line_rate,
    CA_IN ca_uint8_t  dwlch_config_index);
ca_status_t aal_xgpon_xfp_cap2_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_xfp_cap2_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  index,
    CA_IN ca_uint8_t  dn_support,
    CA_IN ca_uint8_t  up_support,
    CA_IN ca_uint8_t  dwlch_calibration_record);
ca_status_t aal_xgpon_xfp_chanl_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);
ca_status_t aal_xgpon_xfp_chanl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint32_t  ds_pon_id,
    CA_IN ca_uint32_t  us_pon_id);
#endif

#define AAL_NGPON2_TWDM_INVALID_CHN_ID 0xFF
typedef struct {
    ca_uint32_t  valid : 1; /* this channel exist or not in transcevier */
    ca_uint32_t  oper_state : 1; /* operation state : on or off */
    ca_uint32_t  active_status : 1; /* actived status : link up or down */
    ca_uint32_t  map_989dot2_ds_chn : 8; /* index of G.989.2 defined downstream channel */
    ca_uint32_t  map_989dot2_us_chn : 8; /* index of G.989.2 defined downstream channel */
    ca_uint32_t  rsvd : 13; /* for feature usaage */
}aal_xgpon_twdm_chn_info_t;


ca_status_t aal_xgpon_sys_profile_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT ca_xgpn_sys_profile_t  *sys_prof);

ca_status_t aal_xgpon_burst_profile_get(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_uint8_t                burst_prof_idx,
    CA_OUT ca_xgpn_burst_profile_t  *burst_prof);

ca_status_t aal_xgpon_channel_profile_get(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_uint8_t                chn_prof_idx,
    CA_OUT ca_xgpn_twdm_channel_profile_t  *chn_prof);

ca_status_t aal_xgpn_cpi_timeout_set(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_uint32_t  time_out);

ca_status_t aal_xgpn_cpi_timeout_get(
    CA_IN  ca_device_id_t               device_id,
    CA_OUT ca_uint32_t  *time_out);

ca_status_t aal_xgpn_current_chan_get(
    ca_device_id_t   device_id,
    ca_uint32_t  *up_chn,
    ca_uint32_t  *dn_chn);

ca_status_t aal_xgpn_chn_info_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t    chn_id,
    CA_OUT aal_xgpon_twdm_chn_info_t *chn_info);

ca_status_t aal_xgpn_chn_info_set(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t    chn_id,
    CA_IN  aal_xgpon_twdm_chn_info_t *chn_info);

/* AAL public APIs */




#endif /* End of __AAL_NGPON2_H__ */

