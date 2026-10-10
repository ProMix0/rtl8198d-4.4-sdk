#ifndef __AAL_XGPON_ACTP_H__
#define __AAL_XGPON_ACTP_H__

/* Default TO thresholds */

#ifdef CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN
#define AAL_XGPON_ACTP_RESPONSE_THRESHOLD     15
#else
#define AAL_XGPON_ACTP_RESPONSE_THRESHOLD     10     /* 1.25ms */
#endif

#define AAL_XGPON_ACTP_SYNC_HUNT_THRESHOLD    80008 /* 1s */
#define AAL_XGPON_ACTP_TOZ_THRESHOLD          80000 /* 1000ms */
#define AAL_XGPON_ACTP_TO1_THRESHOLD          80000 /* 1000ms */
#define AAL_XGPON_ACTP_TO23_THRESHOLD         1000  /* 120ms */
#define AAL_XGPON_ACTP_TO4_THRESHOLD          800   /* 100ms */
#define AAL_XGPON_ACTP_TO5_THRESHOLD          1000  /* 120ms */

#define AAL_XGPON_ACTP_PROF_LEARNING          0 /* Profile learning */
#define AAL_XGPON_ACTP_PROF_DWLCH_OK          1 /* DWLCH OK */
#define AAL_XGPON_ACTP_PROF_DWLCH_FAIL        2 /* DWLCH not OK */

/*
 * ONU ID Values
 */
#define AAL_XGPON_INVALID_ONU_ID            0xFFFF
/* GPON */
#define AAL_GPON_ONU_ID_ASSIGNABLE_START     0
#define AAL_GPON_ONU_ID_ASSIGNABLE_END       253
#define AAL_GPON_ONU_ID_RSVD_START           254
#define AAL_GPON_ONU_ID_RSVD_END             254
#define AAL_GPON_ONU_ID_BC                   255

/* XGPON1, XGSPON */
#define AAL_XGPON_ONU_ID_ASSIGNABLE_START    0
#define AAL_XGPON_ONU_ID_ASSIGNABLE_END      1022
#define AAL_XGPON_ONU_ID_BC                  1023
/* NGPON2 */
#define AAL_NGP2_ONU_ID_ASSIGNABLE_START    0
#define AAL_NGP2_ONU_ID_ASSIGNABLE_END      1020
#define AAL_NGP2_ONU_ID_RSVD_START          1021
#define AAL_NGP2_ONU_ID_RSVD_END            1022
#define AAL_NGP2_ONU_ID_BC                  1023

/*
 * Alloc ID Values
 */
#define AAL_XGPON_INVALID_ALLOC_ID            0xFFFF

/* GPON */
#define AAL_GPON_ALLOC_ID_DEFAULT_START        0
#define AAL_GPON_ALLOC_ID_DEFAULT_END          253
#define AAL_GPON_ALLOC_ID_BC                   254
#define AAL_GPON_ALLOC_ID_RSVD_START           255
#define AAL_GPON_ALLOC_ID_RSVD_END             255
#define AAL_GPON_ALLOC_ID_ASSIGNABLE_START     256
#define AAL_GPON_ALLOC_ID_ASSIGNABLE_END       4095

/* XGPON1, XGSPON */
#define AAL_XGPON_ALLOC_ID_DEFAULT_START       0
#define AAL_XGPON_ALLOC_ID_DEFAULT_END         1022
#define AAL_XGPON_ALLOC_ID_BC                  1023
#define AAL_XGPON_ALLOC_ID_ASSIGNABLE_START    1024
#define AAL_XGPON_ALLOC_ID_ASSIGNABLE_END      16383
/* NGPON2 */
#define AAL_NGP2_ALLOC_ID_DEFAULT_START        0
#define AAL_NGP2_ALLOC_ID_DEFAULT_END          1020
#define AAL_NGP2_ALLOC_ID_BC_START             1021
#define AAL_NGP2_ALLOC_ID_BC_END               1023
#define AAL_NGP2_ALLOC_ID_ASSIGNABLE_START     1024
#define AAL_NGP2_ALLOC_ID_ASSIGNABLE_END       16383


/*
 * onu_TOP_state
 */
#define AAL_XGPON_ACTP_STATE_O1        0 /* Initial state(O1) */
#define AAL_XGPON_ACTP_STATE_O23       1 /* Serail number state(O23) */
#define AAL_XGPON_ACTP_STATE_O4        2 /* Ranging state(O4) */
#define AAL_XGPON_ACTP_STATE_O5        3 /* operation state(O5) */
#define AAL_XGPON_ACTP_STATE_O6        4 /* Intermittent LODS state(O6) */
#define AAL_XGPON_ACTP_STATE_O7        5 /* Emergency stop state(O7) */
#define AAL_XGPON_ACTP_STATE_O8        6 /* DS Tuning state(O8) */
#define AAL_XGPON_ACTP_STATE_O9        7 /* US Tuning state(O9) */

/*
 * onu_sub_state
 */
#define AAL_XGPON_ACTP_SUB_STATE_OX1   0 /* O1.1,O5.1,O8.1 */
#define AAL_XGPON_ACTP_SUB_STATE_OX2   1 /* O1.2,O5.2,O8.2 */


/*
 * Activation profile learning configure
 */
#define AAL_XGPON_ACTP_STS_PROFILE_LEARNING             	0  /*0: Profile learning */
#define AAL_XGPON_ACTP_STS_DWLCH_OK             	        1  /*1: DWLCH OK */
#define AAL_XGPON_ACTP_STS_DWLCH_NOT_OK                  	2  /*2: DWLCH not OK */
#define AAL_XGPON_ACTP_STS_RESERVED                      	3  /*others reserved */


typedef struct {
    ca_boolean_t sw_mode;        /* SW mode enable: ONU state is programmed by CPU, otherwise HW mode */
    ca_boolean_t sw_discard_all; /* Set to 1 and then 0 will discard profile,onuid,allocation_id,xgem_port_id,Eqd */
    ca_boolean_t wlcp_enable;     /* Wavelength protection enable OR not when ONU is in O6 state */
    ca_uint8_t   response_threshold; /* MAX number of PLOAMu messages that ONU can transfer at no-zero attenuation level in O2-3,O9 state */
} aal_xgpon_actp_ctrl_t;

typedef union {
    struct {
    ca_uint32_t sw_mode              :  1 ;
    ca_uint32_t sw_discard_all       :  1 ;
    ca_uint32_t wlcp_enable          :  1 ;
    ca_uint32_t response_threshold   :  1 ;
    ca_uint32_t rsrvd1               : 29 ;
    }s;
    ca_uint32_t u32_data;
} aal_xgpon_actp_ctrl_mask_t;


typedef struct {
    ca_uint8_t   onu_state; /*
                              0 : Initial state(O1)
                              1 : Serail number state(O23)
                              2 : Ranging state(O4)
                              3 : operation state(O5)
                              4 : Intermittent LODS state(O6)
                              5 : Emergency stop state(O7)
                              6 : DS Tuning state(O8)
                              7 : US Tuning state(O9)
                           */
    ca_uint8_t   dbg_onu_state;     /* Read-only, latch ONU state when ONU go back to O1, it is for debug */
    ca_uint8_t   onu_sub_state;     /* ONU sub state. 0: O1.1/O5.1/O8.1, 1:O1.2/O5.2/O8.2 */
    ca_uint8_t   previous_onu_state; /* Read-only, previous ONU state */
    ca_uint8_t   previous_sub_state; /* Read-only, previous ONU SUB state */
    ca_uint8_t   profile_learning ; /* profile learning state: learning, DWLCH OK, DWLCH not OK */
    ca_boolean_t rx_optical_tuning; /* Optical module RX path is tuning or not */
    ca_boolean_t tx_optical_tuning; /* Optical module TX path is tuning or not */
} aal_xgpon_actp_status_t;

typedef union {
    struct {
    ca_uint8_t   onu_state        : 1;
    ca_uint8_t   dbg_onu_state    : 1;
    ca_uint8_t   onu_sub_state    : 1;
    ca_uint8_t   profile_learning : 1;
    ca_boolean_t rx_optical_tuning: 1;
    ca_boolean_t tx_optical_tuning: 1;
    ca_uint32_t  rsrvd1           : 26 ;
    }s;
    ca_uint32_t u32_data;
} aal_xgpon_actp_status_mask_t;

typedef  union {
  struct {
    ca_uint32_t rsrvd1               :  1 ;
    ca_uint32_t deregst              :  1 ; /* bits 1:1 */
    ca_uint32_t rsrvd2               :  4 ;
    ca_uint32_t loss_sync            :  1 ; /* bits 6:6 */
    ca_uint32_t ds_synced            :  1 ; /* bits 7:7 */
    ca_uint32_t DWLCH_appropriate    :  1 ; /* bits 8:8 */
    ca_uint32_t DWLCH_inappropriate  :  1 ; /* bits 9:9 */
    ca_uint32_t disable_SN           :  1 ; /* bits 10:10 */
    ca_uint32_t deactivate_OID       :  1 ; /* bits 11:11 */
    ca_uint32_t assign_OID           :  1 ; /* bits 12:12 */
    ca_uint32_t calibr_req_mismatch  :  1 ; /* bits 13:13 */
    ca_uint32_t TOZ_expire           :  1 ; /* bits 14:14 */
    ca_uint32_t TO1_expire           :  1 ; /* bits 15:15 */
    ca_uint32_t assign_OID_inconsist :  1 ; /* bits 16:16 */
    ca_uint32_t assign_EqD           :  1 ; /* bits 17:17 */
    ca_uint32_t tuning_request_ack   :  1 ; /* bits 18:18 */
    ca_uint32_t tuning_request_tx    :  1 ; /* bits 19:19 */
    ca_uint32_t tuning_request_rx    :  1 ; /* bits 20:20 */
    ca_uint32_t enable_SN            :  1 ; /* bits 21:21 */
    ca_uint32_t us_tuning_confirm    :  1 ; /* bits 22:22 */
    ca_uint32_t TO2_expire           :  1 ; /* bits 23:23 */
    ca_uint32_t TO3_expire           :  1 ; /* bits 24:24 */
    ca_uint32_t TO4_expire           :  1 ; /* bits 25:25 */
    ca_uint32_t TO5_expire           :  1 ; /* bits 26:26 */
    ca_uint32_t ds_synced_O6         :  1 ; /* bits 27:27 */
    ca_uint32_t rsrvd3               :  4 ;
  } s ;
  ca_uint32_t     u32_data;
} aal_xgpon_actp_state_chg_mask_t;

typedef struct {
    ca_uint8_t deregst             ;
    ca_uint8_t loss_sync           ;
    ca_uint8_t ds_synced           ;
    ca_uint8_t DWLCH_appropriate   ;
    ca_uint8_t DWLCH_inappropriate ;
    ca_uint8_t disable_SN          ;
    ca_uint8_t deactivate_OID      ;
    ca_uint8_t assign_OID          ;
    ca_uint8_t calibr_req_mismatch ;
    ca_uint8_t TOZ_expire          ;
    ca_uint8_t TO1_expire          ;
    ca_uint8_t assign_OID_inconsist;
    ca_uint8_t assign_EqD          ;
    ca_uint8_t tuning_request_ack  ;
    ca_uint8_t tuning_request_tx   ;
    ca_uint8_t tuning_request_rx   ;
    ca_uint8_t enable_SN           ;
    ca_uint8_t us_tuning_confirm   ;
    ca_uint8_t TO2_expire          ;
    ca_uint8_t TO3_expire          ;
    ca_uint8_t TO4_expire          ;
    ca_uint8_t TO5_expire          ;
    ca_uint8_t ds_synced_O6        ;

}aal_xgpon_actp_state_chg_t;

ca_status_t aal_xgpon_actp_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_xgpon_actp_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpon_actp_ctrl_t      *ctrl);

ca_status_t aal_xgpon_actp_ctrl_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_xgpon_actp_ctrl_t      *ctrl);

ca_status_t aal_xgpon_actp_status_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN aal_xgpon_actp_status_mask_t  sts_mask,
    CA_IN aal_xgpon_actp_status_t      *sts);

ca_status_t aal_xgpon_actp_status_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_xgpon_actp_status_t   *sts);

ca_status_t aal_xgpon_actp_state_chg_get(
    CA_IN  ca_device_id_t                   device_id,
    CA_OUT aal_xgpon_actp_state_chg_t   *sts);

ca_status_t aal_xgpon_actp_state_chg_enable_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t                     device_id,
    CA_IN  aal_xgpon_actp_state_chg_mask_t mask,
    CA_IN  aal_xgpon_actp_state_chg_t      *sts);

ca_status_t aal_xgpon_actp_state_chg_enable_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t                   device_id,
    CA_OUT aal_xgpon_actp_state_chg_t   *sts);

ca_status_t aal_xgpon_actp_sync_hunt_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    sync_threshold);

ca_status_t aal_xgpon_actp_sync_hunt_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *sync_threshold);

ca_status_t aal_xgpon_actp_to5_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to5_threshold);

ca_status_t aal_xgpon_actp_to5_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to5_threshold);

ca_status_t aal_xgpon_actp_to4_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to4_threshold);

ca_status_t aal_xgpon_actp_to4_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to4_threshold);

ca_status_t aal_xgpon_actp_to23_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to23_threshold);

ca_status_t aal_xgpon_actp_to23_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to23_threshold);

ca_status_t aal_xgpon_actp_to1_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *to1_threshold);

ca_status_t aal_xgpon_actp_to1_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    to1_threshold);

ca_status_t aal_xgpon_actp_toz_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT ca_uint32_t   *toz_threshold);

ca_status_t aal_xgpon_actp_toz_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t    toz_threshold);


#define aal_xgpon_actp_profile_learning_set(/*CODEGEN_IGNORE_TAG*/device_id, pf)  AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, profile_learning, (pf) & 0x3)


ca_status_t aal_xgpon_actp_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

#endif /* __AAL_XGPON_ACTP_H__ */

