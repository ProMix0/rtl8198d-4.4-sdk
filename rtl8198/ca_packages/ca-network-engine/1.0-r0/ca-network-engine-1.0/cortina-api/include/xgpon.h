#ifndef __CA_NGP2_H__
#define __CA_NGP2_H__

#include "ca_types.h"


#define CA_XGPN_DEV_ID 0

#define CA_GPON_DYING_GASP_PDU_LEN  48

#define CA_XGPON_KEY_IDX_CHECK(val) /*CODEGEN_IGNORE_TAG*/  if(((val) != 0) && ((val) != 1)) { ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val);  return CA_E_PARAM; }
#define CA_NGP2_ATTENUATION_MAX   7

#define GPON_ONU_VERSION_ID_LENGTH     14
#define GPON_ONU_SERIAL_NUMBER_LENGTH  8
#define GPON_ONU_ID_STR_LENGTH         24
#define GPON_ONU_PASSWORD_LENGTH       12
#define GPON_AES_KEY_LENGTH            16
#define GPON_REGISTRATION_ID_LENGTH	   36


typedef enum {
    CA_KEY_CONTROL_NEW,
    CA_KEY_CONTROL_CONFIRM
} ca_key_control_ploam_type_t;

typedef enum {
    CA_KEY_REPORT_NEW,
    CA_KEY_REPORT_CONFIRM
} ca_key_report_ploam_type_t;

/*used to support G.988 clause 9.13.11*/
typedef enum {
    CA_MC_FLAG_MC_ENCRYPTION_KEY_SET            = 0, 
    CA_MC_FLAG_MC_ENCRYPTION_KEY_CLEAR          = 1, 
    CA_MC_FLAG_MC_ENCRYPTION_KEY_TBL_FLUSH      = 2
}ca_mc_encryption_key_flag_t;

typedef enum {
    CA_BURST_PROFILE = 0x01,
    CA_ASSIGN_ONU_ID = 0x03,
    CA_RANGING_TIME = 0x04,
    CA_DEACTIVATE_ONU_ID = 0x05,
    CA_DISABLE_SERIAL_NUMBER = 0x06,
    CA_REQUEST_REGISTRATION = 0x09,
    CA_ASSIGN_ALLOC_ID = 0x0A,
    CA_KEY_CONTROL = 0x0D,
    CA_SLEEP_ALLOW = 0x12,
    CA_CALIBRATION_REQUEST = 0x13,
    CA_ADJUST_TX_WAVELENGTH = 0x14,
    CA_TUNING_CONTROL = 0x15,
    CA_SYSTEM_PROFILE = 0x17,
    CA_CHANNEL_PROFILE = 0x18,
    CA_PROTECTION_CONTROL = 0x19,
    CA_CHANGE_POWER_LEVEL = 0x1A,
    CA_POWER_CONSUMPTION_INQUIRE = 0x1B,
    CA_RATE_CONTROL = 0x1C
} ca_xgpon_ds_ploam_msg_type_t;
    
typedef enum {
    CA_NGP2_KEY_INDEX_0                 = 0, 
    CA_NGP2_KEY_INDEX_1                 = 1
} ca_ngp2_encryption_key_id_t;

typedef enum {
    CA_SERIAL_NUMBER_ONU = 0x01,
    CA_REGISTRATION = 0x02,
    CA_KEY_REPORT = 0x05,
    CA_ACKNOWLEDGEMENT = 0x09,
    CA_SLEEP_REQUEST = 0x10,
    CA_TUNING_RESPONSE = 0x1A,
    CA_POWER_CONSUMPTION_REPORT = 0x1B,
    CA_RATE_RESPONSE = 0x1C
} ca_xgpon_us_ploam_msg_type_t;


typedef struct {
    ca_uint16_t onu_id;
    ca_uint8_t msg_type_id;
    ca_uint8_t seq_no;
    ca_uint8_t msg_content[36];
    ca_uint64_t mic;
} ca_xgpon_ploam_msg_t;

typedef struct {
    ca_uint16_t onu_id;
    ca_uint8_t msg_type_id;
    ca_uint8_t seq_no;
    ca_uint8_t rsvd;
    ca_uint8_t ctrl_flag;       /*0: Generate and send a new key. 1: Confirm the existing key.*/
    ca_uint8_t key_idx;     /*01:    First key of a key pair;10:    Second key of a key pair.*/
    ca_uint8_t key_length;
    ca_uint8_t padding[32];
    ca_uint8_t mic[8];
} ca_xgpon_key_control_ploam_msg_t;

typedef struct {
    ca_uint16_t onu_id;
    ca_uint8_t msg_type_id;
    ca_uint8_t seq_no;
    ca_uint8_t report_type; /*0: New key. 1: Report on existing key.*/
    ca_uint8_t key_idx;     /*01:    First key of a key pair;10:    Second key of a key pair.*/
    ca_uint8_t frament_no;  /*range 0..7.*/
    ca_uint8_t rsvd;
    ca_uint8_t key_fragment[32];
    ca_uint64_t mic;
} ca_xgpon_key_report_ploam_msg_t;

/*burst profile*/
#define CA_NGP2_ONU_ID_LENGTH                    2
#define CA_NGP2_BURST_PROFILE_DELMITER_LENGTH    8
#define CA_NGP2_BURST_PROFILE_PREEMBLE_LENGTH    8
#define CA_NGP2_BURST_PROFILE_PONTAG_LEN         8
#define CA_NGP2_BURST_PROFILE_PAD_LENGTH         7

typedef struct {
    ca_uint8_t onu_id[CA_NGP2_ONU_ID_LENGTH];  /* see table 11-16 of ITU 989.3 */
    ca_uint8_t msg_type;
    ca_uint8_t sequence_number;
    ca_uint8_t version_profile_index; /* up to 4 burst profiles supported */
    ca_uint8_t us_fec_indication;
    ca_uint8_t delimiter_len;
    ca_uint8_t delimiter[CA_NGP2_BURST_PROFILE_DELMITER_LENGTH];
    ca_uint8_t preamble_len;
    ca_uint8_t preamble_repeat_count;
    ca_uint8_t preamble[CA_NGP2_BURST_PROFILE_PREEMBLE_LENGTH];
    ca_uint8_t pon_tag[CA_NGP2_BURST_PROFILE_PONTAG_LEN];
} ca_ngp2_burst_profile_t;


/*
  NOTE: up to 4 burst profiles per ONU (2-bit profile index in burst profile PLOAM
  Burst profiles are not preserved across activation
 */


/*system profile*/
#define CA_NGP2_SYSTEM_PROFILE_SYSID_LENGTH   3
#define CA_NGP2_SYSTEM_PROFILE_FSR_LENGTH 2
#define CA_NGP2_SYS_PRF_PAD_LEN 2
#define CA_NGP2_PLOAM_MIC_LEN       8
#define CA_NGP2_OMCI_MIC_LEN        4


typedef struct {
    ca_uint8_t onu_id[CA_NGP2_ONU_ID_LENGTH];  /* see table 11-16 of ITU 989.3 */
    ca_uint8_t msg_type;
    ca_uint8_t sequence_number;
    ca_uint8_t ng2sysid[CA_NGP2_SYSTEM_PROFILE_SYSID_LENGTH]; /*see section 6.1.5.2*/
    ca_uint8_t version;
    ca_uint8_t us_owl_bands;
    ca_uint8_t twdm_channel_count;
    ca_uint8_t channel_spacing_twdm;
    ca_uint8_t us_mse_twdm;
    ca_uint8_t fsr_twdm[CA_NGP2_SYSTEM_PROFILE_FSR_LENGTH];
    ca_uint8_t twmd_amcc_control;
    ca_uint8_t loose_calibration_bound_for_ptwdm_channels;
    ca_uint8_t ptwdm_channel_count;
    ca_uint8_t channel_spacing_ptp;
    ca_uint8_t us_mse_ptp;
    ca_uint8_t fsr_ptp[CA_NGP2_SYSTEM_PROFILE_FSR_LENGTH];
    ca_uint8_t ptp_calibration_accuracy;
    ca_uint8_t loose_calibration_bound_ptp;
} ca_ngp2_system_profile_t;

/*channel profile*/
#define CA_NGP2_CHANNEL_PROFILE_PON_ID_LENGTH 4
#define CA_NGP2_CHANNLE_PROFILE_DEFAULT_RESPONSE_CHANNEL_LENGTH   4
#define CA_NGP2_CHAN_PRF_PHY_FC_LEN 2
#define CA_NGP2_CHANNLE_PROFLILE_UP_STREAM_FREQUENCY_LENGTH    4
#define CA_NGP2_CHANNLE_PROFILE_PAD_LENGTH        11
#define CA_NGP2_MIC_LEN             8

/*channel caliberation record  reported to OLT upon activation*/
typedef struct {
    ca_uint8_t status_0: 4; /*see section 11.2.6.4*/
    ca_uint8_t status_2: 4; /*see section 11.2.6.4*/
    ca_uint8_t status_4: 4; /*see section 11.2.6.4*/
    ca_uint8_t status_6: 4; /*see section 11.2.6.4*/
    ca_uint8_t status_8: 4; /*see section 11.2.6.4*/
    ca_uint8_t status_10: 4;    /*see section 11.2.6.4*/
    ca_uint8_t status_12: 4;    /*see section 11.2.6.4*/
    ca_uint8_t status_14: 4;    /*see section 11.2.6.4*/
    ca_uint8_t reserved[14];
    ca_uint8_t status_1;        /*see section 11.2.6.4*/
    ca_uint8_t status_3;        /*see section 11.2.6.4*/
    ca_uint8_t status_5;        /*see section 11.2.6.4*/
    ca_uint8_t status_7;        /*see section 11.2.6.4*/
    ca_uint8_t status_9;        /*see section 11.2.6.4*/
    ca_uint8_t status_11;       /*see section 11.2.6.4*/
    ca_uint8_t status_13;       /*see section 11.2.6.4*/
    ca_uint8_t status_15;       /*see section 11.2.6.4*/
} ca_ngp2_sys_calibration_status_t;

/*pon id internal format for TWDM*/
typedef struct {
    ca_uint32_t admin_label: 28;
    ca_uint32_t dwlch_id: 4;    /*see section 10.1.1.3*/
} ca_ngp2_pon_id_t;



/*default WLCH (and transceiver) configurations*/
typedef struct {
    ca_uint32_t frequency;
    ca_uint32_t wavelength;
    ca_uint32_t offset;     /*+ means up, - means down*/
    ca_uint32_t attenuation;
    ca_uint32_t power_leveling;
    ca_uint8_t reserved[32];
} ca_ngp2_wlch_param_t;

typedef enum {
    CA_NGP2_CALIBRATION_UNSPEC 			= 0,
    CA_NGP2_CALIBRATION_UNCALIBRATED 	= 1,
    CA_NGP2_CALIBRATION_LOOSE 			= 2,
    CA_NGP2_CALIBRATION_SUFFICIENT 		= 3
} ca_ngp2_pon_cal_status_t;


/*transceiver parameters associated with calibration record for one of the 16 channels*/
typedef struct {
    ca_uint8_t channel_id;
    ca_uint32_t target_pon_id;  /*pon_id associated with channel*/
    ca_ngp2_wlch_param_t channel_params;
    ca_ngp2_pon_cal_status_t		calibration_status;
    ca_uint32_t TBD[32];       /*store transceiver specific data*/
} ca_ngp2_channel_calibration_record_t;

/*
typedef struct ca_xgpon_calibration_record {
    ca_uint32_t target_pon_id;
    ca_uint8_t adj_direction;
    ca_uint16_t adj_freq;       //0.1GHz unit
    ca_uint8_t granularity; //unit in 1GHz (see section 11.2.6.5)
    ca_uint8_t tuning_step_time;    //unit in PHY frame (see section
    ca_xgpon_power_level_t power_leveling;
} ca_xgpon_channel_calibration_record_t;

*/

#define CA_NGP2_MAX_CHANNELS    8
/*ca_ngp2_channel_calibration_record_t can not be pointer for iros, update by DY*/
/*channel profile (up to 8 channels)*/
typedef struct {
    ca_uint8_t onu_id[CA_NGP2_ONU_ID_LENGTH];  /* see table 11-16 of ITU 989.3 */
    ca_uint8_t msg_type;
    ca_uint8_t sequence_number;
    ca_uint8_t channel_profile_index;
    ca_uint8_t channel_profile_version;
    ca_uint8_t pon_id[CA_NGP2_CHANNEL_PROFILE_PON_ID_LENGTH]; /*see section 6.1.5.3*/
    ca_uint8_t ds_freq_offset;
    ca_uint8_t ds_rate;
    ca_uint8_t channel_partition;
    ca_uint8_t default_response_channel[CA_NGP2_CHANNLE_PROFILE_DEFAULT_RESPONSE_CHANNEL_LENGTH];
    ca_uint8_t grant_offered;
    ca_uint8_t phy_frame_count[CA_NGP2_CHAN_PRF_PHY_FC_LEN];
    ca_uint8_t uwlchannel_id;
    ca_uint8_t us_freq[CA_NGP2_CHANNLE_PROFLILE_UP_STREAM_FREQUENCY_LENGTH];
    ca_uint8_t optical_link_type;
    ca_uint8_t us_rate;
    ca_uint8_t default_onu_attenuation;
    ca_uint8_t response_threshold;
    ca_ngp2_channel_calibration_record_t channel_calibration_record;
} ca_ngp2_channel_profile_t;


/*included in serial_number_onu and tuning_response PLOAM
  see section 17.4
  transceiver parameters related to tuning/dithering*/
#define CA_NGP2_CHANNEL_TUNING_CAPABILITY_PARAMETERS_RESERVED_LENGTH	256
typedef struct {
    ca_uint8_t tuning_granularity;  /*unit 1GHz*/
    ca_uint8_t tuning_step_time;    /*unit SFC*/
    ca_uint8_t reserved[CA_NGP2_CHANNEL_TUNING_CAPABILITY_PARAMETERS_RESERVED_LENGTH];
} ca_ngp2_tuning_capability_parameters_t;

/*encryption keys*/
/*
  NOTE: 2 UC keys each key is 128 bits long
  Keys can be pre-computed and store in startup config
  Or re-compute keys in system init
  6 types of keys
1.  MSK ?C master session key derived from PSK
2.  SK ?C section key. Derived from MSK, used to derive the following keys
3.  OMCI_IK ?C OMIC integraty key
4.  PLOAM_IK ?C PLOAM integraty key
5.  KEK ?C key encryption key, used to encrypt keys (see below)
6.  2 UC Data encryption key: generated by ONU (random number generator), encrypted with KEK, reported to OLT
7.  2 MC data encryption key: generated by OLT, encrypted with KEK, send to ONU over OMCI
8.
*/


#define CA_NGP2_UC_KEY_LEN              16  /*16 bytes effective key length*/
#define CA_NGP2_BC_KEY_LEN              16
#define CA_NGP2_UC_KEY_COUNT            2
#define CA_NGP2_BC_KEY_COUNT            2
#define CA_NGP2_REGISTRATION_ID_LEN     36  /*section 15.3.2*/
#define CA_NGP2_DEF_IK                  0x55    /*section 15.3.3*/



typedef enum {
    CA_NGP2_AUTH_TYPE_REG_ID = 0,
    CA_NGP2_AUTH_TYPE_OMCI = 1,
    CA_NGP2_AUTH_TYPE_8021X = 2
} ca_ngp2_auth_type_t;


/*
authentication performed with PSK (could be registration_id (36B),
                                   or any other pre - shared key
                                   */
typedef enum {
    CA_NGP2_OMCI_AUTH_ALGO_AES_128 = 0,
    CA_NGP2_OMCI_AUTH_ALGO_SHA_256 = 1,
    CA_NGP2_OMCI_AUTH_ALGO_SHA_512 = 2
} ca_ngp2_omci_auth_algo_t;


typedef enum {
    CA_NGP2_KEY_IDX_0 = 0,
    CA_NGP2_KEY_IDX_1 = 1
} ca_ngp2_enc_key_id_t;

/*power management timers*/
typedef struct {
    ca_uint32_t ilwpwoer;       /* see table 16-1 of ITU 989.3 */
    ca_uint32_t tlwpwoer;       /* see table 16-1 of ITU 989.3 */
    ca_uint32_t iaware;     /* see table 16-1 of ITU 989.3 */
    ca_uint32_t taware;     /* see table 16-1 of ITU 989.3 */
    ca_uint32_t itransinit; /* see table 16-1 of ITU 989.3 */
    ca_uint32_t irxoff;     /* see table 16-1 of ITU 989.3 */
    ca_uint32_t trxoff;     /* see table 16-1 of ITU 989.3 */
    ca_uint32_t talerted;       /* see table 16-1 of ITU 989.3 */
    ca_uint32_t ter;        /* see table 16-1 of ITU 989.3 */
    ca_uint32_t ihold;      /* see table 16-1 of ITU 989.3 */
    ca_uint32_t thold;      /* see table 16-1 of ITU 989.3 */
} ca_ngp2_pwr_mgmt_param_t;


/*downstream sync state*/
typedef enum {
    CA_NGP2_ONU_HUNT = 0,
    CA_NGP2_ONU_PRESYNC = 1,
    CA_NGP2_ONU_SYNCED = 3,
    CA_NGP2_ONU_RESYNC = 4
} ca_ngp2_onu_ds_sync_t;

/*only for serial number grant, upstream serial number and tuning response PLOAM CA_XGPON_ONU_PLOAM_AMCC    */
typedef enum {
    CA_NGP2_ONU_PLOAM_INBAND = 0,
    CA_NGP2_ONU_PLOAM_AMCC    = 1,
    CA_NGP2_ONU_OMCI = 2
} ca_ngp2_control_chan_type_t;


typedef struct {
    ca_uint8_t retry_count;
    ca_uint32_t chan_type;/*only CA_XGPON_ONU_PLOAM_INBAND and CA_XGPON_ONU_PLOAM_AMCC are allowed*/
} ca_ngp2_resync_param_t;

/* OMCI-based secure mutual authentication*/
typedef enum {
    CA_NGP2_KEY_ALGO_AES_CMAC_128 = 0,
    CA_NGP2_KEY_ALGO_SHA_256 = 1,
    CA_NGP2_KEY_ALGO_SHA_512 = 2
} ca_ngp2_hash_algo_t;

typedef enum {
    CA_NGP2_MSK = 0,
    CA_NGP2_SK = 1,
    CA_NGP2_OMCI_IK = 2,
    CA_NGP2_PLOAM_IK = 3,
    CA_NGP2_KEK = 4,
    CA_NGP2_UC_KEY0 = 5,
    CA_NGP2_UC_KEY1 = 6
} ca_ngp2_key_type_t;

#if 0
#define CA_NGP2_DEFAULT_KEY_MSK             0x55555555555555555555555555555555  /*0x55 x 16*/
#define CA_NGP2_DEFAULT_SK_MSG                  "SessionK"
#define CA_NGP2_DEFAULT_OMCI_MSG                "OMCIIntegrityKey"
#define CA_NGP2_DEFAULT_PLOAM_MSG               "PLOAMIntegrityKey"
#define CA_NGP2_DEFAULT_KEY_MSG             "KeyEncryptionKey"
#endif

#define CA_NGP2_DEFAULT_TK4_MSEC                100 /*in msec*/
#define CA_NGP2_DEFAULT_TK5_MSEC                20  /*in msec*/

#define CA_NGP2_DEFAULT_DATA_KEY_EFFTV_LEN      32

typedef enum {
    ca_pon_mode_gpon    = 0, /*GPON mode*/
    ca_pon_mode_xgs_pon = 1, /*XGS mode*/
    ca_pon_mode_xgpon1  = 2,/*XGPON1 mode*/
    ca_pon_mode_ngpon2  = 3 /*NGPON2 mode*/
}ca_pon_mode_t;


/*TC layer Performance Monitor Counters*/
typedef enum {
    CA_COUNTERS_GAL_ETHERNET        = 0,
    CA_COUNTERS_PHY_LOS_PM_T        = 1, /*PHY/LODS PM counters (989.3 table 14-1, VZOpenOMCI:7.3.1)*/
    CA_COUNTERS_FEC_PM_T            = 2, /*FEC PM counters (989.3 table 14-1, VZOpenOMCI:7.3.2)*/
    CA_COUNTERS_XGEM_PM_T           = 3, /*XGEN PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.3)*/
    CA_COUNTERS_PLOAM_PM1_T         = 4, /*PLOAM1 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.4)*/
    CA_COUNTERS_PLOAM_PM2_T         = 5, /*PLOAM2 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.5)*/
    CA_COUNTERS_PLOAM_PM3_T         = 6, /*PLOAM3 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.6)*/
    CA_COUNTERS_CHAN_TUNING_PM1_T   = 7, /*TUNING1 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.7)*/
    CA_COUNTERS_CHAN_TUNING_PM2_T   = 8, /*TUNING2 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.8)*/
    CA_COUNTERS_CHAN_TUNING_PM3_T   = 9, /*TUNING3 PM counters (989.3 table 14-1, VZOpenOMCI: 7.3.9)*/
    CA_COUNTERS_OMCI_PM_T           = 10 /*OMCI PM counters (VZOpenOMCI: 7.3.10)*/
}ca_ngp2_pm_counter_types_t;

/*GAL PM counters*/
typedef struct {
    ca_uint8_t interval_end_time;
    ca_uint32_t discarded;
    ca_uint32_t discarded_threshold;
}ca_gal_ethernet_counters_t;

/*PHY LODS PM counters*/
typedef struct {
    ca_uint64_t total_words_protected_by_bip;
    ca_uint32_t bip_error_count;
    ca_uint32_t psbd_hec_errors_corrected;
    ca_uint32_t psbd_hec_errors_uncorrectable;
    ca_uint32_t fs_header_errors_corrected;
    ca_uint32_t fs_header_errors_uncorrectable;
    ca_uint32_t total_lods_event_count;
    ca_uint32_t lods_restored_in_operating_twdm_ch;
    ca_uint32_t lods_restored_in_protection_twdm_ch;
    ca_uint32_t lods_restored_in_discretionary_twdm_ch;
    ca_uint32_t lods_reactivations;
    ca_uint32_t lods_handshake_failure_in_protection_ch;
    ca_uint32_t lods_handshake_failure_in_discretionary_ch;
}ca_phy_los_pm_t;

/*FEC PM counters*/
typedef struct {
    ca_uint32_t corrected_fec_bytes;
    ca_uint32_t corrected_fec_codewords;
    ca_uint32_t uncorrected_fec_codewords;
    ca_uint32_t total_fec_codewords;
}ca_fec_pm_t;

/*XGEN PM counters*/
typedef struct {
    ca_uint32_t total_transmitted_xgem_frames;
    ca_uint32_t transmitted_xgem_frames_with_lf;
    ca_uint32_t total_received_xgem_frames;
    ca_uint32_t received_xgem_header_hec_errors;
    ca_uint32_t fs_words_lost_to_xgem_header_hec_errors;
    ca_uint32_t xgem_encryption_key_errors;
    ca_uint32_t total_transmitted_bytes_in_non_idle_xgem_frames;
    ca_uint32_t total_received_bytes_in_non_idle_xgem_frames;
} ca_xgem_pm_t;

/*PLOAM1 PM counters*/
typedef struct {
    ca_uint32_t ploam_mic_errors;
    ca_uint32_t downstream_ploam_message_count;
    ca_uint32_t ranging_time_message_count;
    ca_uint32_t protection_control_message_count;
    ca_uint32_t adjust_tx_wavelength_message_count;
    ca_uint32_t adjust_tx_wavelength_amplitude;
} ca_ploam_pm1_t;

/*PLOAM2 PM counters*/
typedef struct {
    ca_uint32_t system_profile_message_count;
    ca_uint32_t channel_profile_message_count;
    ca_uint32_t burst_profile_message_count;
    ca_uint32_t assign_onu_id_message_count;
    ca_uint32_t unsatisfied_adjust_tx_wavelength_requests;
    ca_uint32_t deactivate_onu_id_message_count;
    ca_uint32_t disable_serial_number_message_count;
    ca_uint32_t request_registration_message_count;
    ca_uint32_t assign_alloc_id_message_count;
    ca_uint32_t key_control_message_count;
    ca_uint32_t sleep_allow_message_count;
}ca_ploam_pm2_t;

/*PLOAM3 PM counters*/
typedef struct {
    ca_uint32_t upstream_ploam_message_count;
    ca_uint32_t serial_number_onu_in_band_message_count;
    ca_uint32_t serial_number_onu_amcc_message_count;
    ca_uint32_t registration_message_count;
    ca_uint32_t acknowledgement_message_count;
    ca_uint32_t sleep_request_message_count;
    ca_uint32_t tuning_response_ack_nack_msg_count;
    ca_uint32_t tuning_response_complete_u_rollback_msg_count;
    ca_uint32_t power_consumption_report_message_count;
    ca_uint32_t change_power_level_parameter_error_count;
}ca_ploam_pm3_t;

/*TUNING 1 PM counters*/
typedef struct {
    ca_uint32_t tuning_control_requests_for_rx_only_or_rx_tx;
    ca_uint32_t tuning_control_requests_for_tx_only;
    ca_uint32_t tuning_control_requests_rejected_int_spc;
    ca_uint32_t tuning_control_requests_rejected_ds_all;
    ca_uint32_t tuning_control_requests_rejected_us_all;
    ca_uint32_t tuning_control_requests_ok_target_channel;
    ca_uint32_t tuning_control_requests_failed_to4_exp;
    ca_uint32_t tuning_control_requests_failed_to5_exp;
    ca_uint32_t tuning_control_requests_resolved_discret_ch;
    ca_uint32_t tuning_control_requests_rollback_com_ds;
    ca_uint32_t tuning_control_requests_rollback_ds_all;
    ca_uint32_t tuning_control_requests_rollback_us_all;
    ca_uint32_t tuning_control_requests_failed_reactivation;
}ca_chan_tuning_pm1_t;

/*TUNING 2 PM counters*/
typedef struct {
    ca_uint32_t tuning_control_requests_rejected_ds_albl;
    ca_uint32_t tuning_control_requests_rejected_ds_void;
    ca_uint32_t tuning_control_requests_rejected_ds_part;
    ca_uint32_t tuning_control_requests_rejected_ds_tunr;
    ca_uint32_t tuning_control_requests_rejected_ds_lnrt;
    ca_uint32_t tuning_control_requests_rejected_ds_lncd;
    ca_uint32_t tuning_control_requests_rejected_us_albl;
    ca_uint32_t tuning_control_requests_rejected_us_void;
    ca_uint32_t tuning_control_requests_rejected_us_tunr;
    ca_uint32_t tuning_control_requests_rejected_us_clbr;
    ca_uint32_t tuning_control_requests_rejected_us_lktp;
    ca_uint32_t tuning_control_requests_rejected_us_lnrt;
    ca_uint32_t tuning_control_requests_rejected_us_lncd;
}ca_chan_tuning_pm2_t;

/*TUNING 3 PM counters*/
typedef struct {
    ca_uint32_t tuning_control_requests_rollback_ds_albl;
    ca_uint32_t tuning_control_requests_rollback_ds_lktp;
    ca_uint32_t tuning_control_requests_rollback_us_albl;
    ca_uint32_t tuning_control_requests_rollback_us_void;
    ca_uint32_t tuning_control_requests_rollback_us_tunr;
    ca_uint32_t tuning_control_requests_rollback_us_lktp;
    ca_uint32_t tuning_control_requests_rollback_us_lnrt;
    ca_uint32_t tuning_control_requests_rollback_us_lncd;
}ca_chan_tuning_pm3_t;

/*OMCI PM counters*/
typedef struct {
    ca_uint32_t omci_baseline_msg_count;
    ca_uint32_t omci_extended_message_count;
    ca_uint32_t omci_mic_error_count;
}ca_omci_pm_t;

/*channel info*/
typedef struct {
    ca_uint8_t twdm_channel_id;
    ca_uint8_t active_indication;
    ca_uint8_t operational_status; /*1-operational, 0-down*/
    ca_uint8_t ds_twdm_chid;
    ca_uint8_t us_twdm_chid;
}ca_channel_info_t;

/*onu power management local events ??union of 989.3 and 987.3 ONU power management local events*/
typedef enum {
	CA_XGPWR_MGMT_LSI 				= 1, 
	CA_XGPWR_MGMT_LWI 				= 2, 
	CA_XGPWR_MGMT_LDI 				= 3, 	/*987.3 only*/
	CA_XGPWR_MGMT_LPI 				= 4	 	/*987.3 only*/
} ca_xgp_power_management_local_event_t;

/*onu power management sleep modes ??see 988 clause 9.1.14*/
typedef struct {
    ca_uint8_t    ca_xgp_power_mgmt_sleep_mode_enable: 1;     
    ca_uint8_t    ca_xgp_power_mgmt_sleep_mode_doze: 1; 
    ca_uint8_t    ca_xgp_power_mgmt_sleep_mode_cyclic_sleep: 1;                    
    ca_uint8_t    ca_xgp_power_mgmt_sleep_mode_watchful_sleep: 1;                    
}ca_xgp_power_management_sleep_mode_t;

/*power management timers ??union of 989.3 and 987.3 ONU power management parameters timers in 125 usec unit*/
typedef struct {
    ca_uint32_t					ilowpower; 	/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					tlowpower; 	/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					iaware; 		/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					taware; 		/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					itransinit; 	/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					irxoff; 		/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					trxoff; 		/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					itxinit; 	/* see table 16-1 of ITU 987.3 */
    ca_uint32_t					ihold; 		/* see table 16-1 of ITU 989.3 */
    ca_uint32_t					thold; 		/* see table 16-1 of ITU 989.3 */
} ca_xgp_power_management_parameters_t;

/*onu power management states ??union of 989.3 and 987.3 ONU power management states*/
typedef enum {
    CA_NGP2_PM_STATE_ACTIVEHOLD   	= 1, 
    CA_NGP2_PM_STATE_ACTIVEFREE   	= 2, 
    CA_NGP2_PM_STATE_AWARE   		= 3, 
    CA_NGP2_PM_STATE_LOWPOWER   	= 4, 
    CA_NGP2_PM_STATE_ASLEEP   		= 5, 
    CA_NGP2_PM_STATE_LISTEN   		= 6, 
    CA_NGP2_PM_STATE_WATCH   		= 7, 
    CA_NGP2_PM_STATE_DOZEAWARE   	= 8, 
    CA_NGP2_PM_STATE_SLEEPAWARE   	= 9, 
    CA_NGP2_PM_STATE_WATCHAWARE   	= 10 
} ca_xgp_power_management_state_t;

/*TC layer timeout values configured via OpenOMCI (clause 6.1.2) unit is 125usec*/
#define CA_TOZ_DEEFAULT_VALUE		8 * 1000 * 10		//10sec
#define CA_TO1_DEEFAULT_VALUE		8 * 1000 * 10	     //10sec
#define CA_TO2_DEEFAULT_VALUE		1000				//125msec
#define CA_TO3_DEEFAULT_VALUE		200				//25msec
#define CA_TO4_DEEFAULT_VALUE		800				//100msec
#define CA_TO5_DEEFAULT_VALUE		1000				//125msec
#define CA_TOCPI_DEEFAULT_VALUE	    8 * 1000 * 60 * 5	//5 minutes


#define CA_XGPON_MAX_CHAN_PROF     8
#define CA_XGPON_MAX_BURST_PROF_INDEX    3
/*
#define CA_XGPON_KEY_LEN       16
#define CA_XGPON_REG_ID_LEN         36
#define CA_XGPON_MIN_PLOAM_LEN      4
#define CA_XGPON_MAX_PLOAM_LEN     40
#define CA_XGPON_MAX_OMCI_LEN      40
#define CA_XGPON_VENDOR_ID_LEN     4
#define CA_XGPON_SSN_LEN           4
#define CA_XGPON_ONUID_LEN         4
*/
ca_status_t ca_ngp2_port_pon_mac_init (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_pon_mode_t pon_mode,
    CA_IN ca_uint8_t serial_number[GPON_ONU_SERIAL_NUMBER_LENGTH],
	CA_IN ca_uint8_t registration_id[GPON_REGISTRATION_ID_LENGTH]
);

ca_status_t ca_ngp2_port_pon_mac_reset (
    CA_IN   ca_device_id_t                 device_id,
    CA_IN   ca_port_id_t                port_id);

ca_status_t ca_ngp2_port_burst_profile_get(
    CA_IN       ca_device_id_t              device_id,
    CA_IN       ca_port_id_t             port_id,
    CA_IN       ca_uint8_t               burst_profile_id,
    CA_OUT      ca_ngp2_burst_profile_t *burst_profiles);

ca_status_t ca_ngp2_port_system_profile_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_OUT      ca_ngp2_system_profile_t   *system_profile);

ca_status_t ca_ngp2_port_channel_profile_get(
    CA_IN       ca_device_id_t                     device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_OUT      ca_uint8_t                      *channel_profile_count,
    CA_OUT      ca_ngp2_channel_profile_t       *channel_profile);

ca_status_t ca_ngp2_port_power_mgmt_param_get(
    CA_IN       ca_device_id_t                          device_id,
    CA_IN       ca_port_id_t                            port_id,
    CA_OUT      ca_xgp_power_management_sleep_mode_t    *sleep_mode,
    CA_OUT      ca_xgp_power_management_parameters_t    *power_mgmt_param);

ca_status_t ca_ngp2_port_power_mgmt_param_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN       ca_xgp_power_management_sleep_mode_t        sleep_mode,
    CA_IN       ca_xgp_power_management_parameters_t        *power_mgmt_param);

ca_status_t ca_ngp2_port_power_mgmt_state_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT      ca_xgp_power_management_state_t *power_mgmt_state);

ca_status_t ca_ngp2_port_power_local_event_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT      ca_xgp_power_management_local_event_t       local_event);

ca_status_t ca_ngp2_port_channel_attenuation_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t   channel_id,
    CA_OUT      ca_uint8_t                  *att_level);

ca_status_t ca_ngp2_port_channel_power_consumption_get (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t   channel_id,
    CA_OUT ca_uint32_t      *power_consumption);

ca_status_t ca_ngp2_port_channel_power_consumption_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t  channel_id,
    CA_IN ca_uint32_t  power_consumption);

ca_status_t ca_ngp2_port_onu_power_leveling_cap_get (
    CA_IN        ca_device_id_t            device_id,
    CA_IN        ca_port_id_t              port_id,
    CA_OUT       ca_uint8_t                *power_level_cap);

ca_status_t ca_ngp2_port_registration_id_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint8_t       *registration_id /*[36:36]*/);

ca_status_t ca_ngp2_port_registration_id_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t        *registration_id/*[36:36]*/);

ca_status_t ca_ngp2_port_mc_encryption_key_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN       ca_ngp2_encryption_key_id_t key_index,
    CA_OUT      ca_uint8_t                  *effective_key_length,
    CA_OUT      ca_uint8_t                  *mc_key/*[16: 16]*/);

ca_status_t ca_ngp2_port_mc_encryption_key_set(
    CA_IN       ca_device_id_t              device_id,
    CA_IN       ca_port_id_t                port_id,
    CA_IN       ca_ngp2_encryption_key_id_t key_index,
    CA_IN       ca_mc_encryption_key_flag_t key_flags,
    CA_IN       ca_uint8_t                  effective_key_length,
    CA_IN       ca_uint8_t                  *mc_key/*[16: 16]*/);

ca_status_t ca_ngp2_port_tuning_granularity_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT      ca_ngp2_tuning_capability_parameters_t  *tuning_cap_params);

ca_status_t ca_ngp2_port_channel_tx_lamda_adjust_set(
    CA_IN       ca_device_id_t              device_id,
    CA_IN       ca_port_id_t             port_id,
    CA_IN       ca_uint8_t               channel_id,
    CA_IN       ca_uint8_t               up_down,
    CA_IN       ca_uint16_t              freq_adjust);

ca_status_t ca_ngp2_port_downstream_sync_state_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT       ca_ngp2_onu_ds_sync_t      *sync_state);

ca_status_t ca_ngp2_twdm_protection_pon_ids_get(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_port_id_t  port_id,
    CA_OUT      ca_uint32_t             *us_pon_id,
    CA_OUT      ca_uint32_t             *ds_pon_id);

ca_status_t ca_ngp2_port_pm_counters_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_ngp2_pm_counter_types_t counter_type,
    CA_IN ca_uint32_t counter_buffer_size,
    CA_OUT      ca_uint8_t                      *counter_buffer/*[counter_buffer_size:60]*/);

ca_status_t ca_ngp2_port_pm_counter_clear(
    CA_IN       ca_device_id_t                         device_id,
    CA_IN       ca_port_id_t                        port_id,
    CA_IN       ca_ngp2_pm_counter_types_t      counter_type);


ca_status_t ca_ngp2_port_channel_info_get (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t channel_id,
    CA_OUT      ca_channel_info_t              *channel_info);

ca_status_t ca_ngp2_port_channel_info_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t channel_id,
    CA_IN       ca_channel_info_t              *channel_info);

ca_status_t ca_ngp2_port_channel_partition_index_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t      *cpi);

ca_status_t ca_ngp2_port_channel_partition_index_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t cpi);

#endif

