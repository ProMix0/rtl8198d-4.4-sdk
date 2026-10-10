#ifndef __AAL_PLOAM_H__
#define __AAL_PLOAM_H__


extern ca_int32_t aal_ploam_debug;
extern ca_uint32_t aal_ploam_err_cnt;

#define PLOAM_DEBUG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_ploam_debug) ca_printf("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)

#define PLOAM_VENDOR_ID             0x4144544e
#define PLOAM_VSSN                  0x15430175

#define AAL_PLAOM_CHANNEL_NUM           8
#define AAL_PLAOM_POWER_CONSUMPTION_MAX 0xFFFFFF

#define  AAL_PLOAM_QUEUE_BUF_MAX_NUM    800

typedef union {
  struct {
    ca_uint32_t sw_mode_en           :  1 ; /* bits 0:0 */
    ca_uint32_t dereg_rst_en         :  1 ; /* bits 1:1 */
    ca_uint32_t resrv_bc_en          :  1 ; /* bits 2:2 */
    ca_uint32_t resrv_id_sel         :  1 ; /* bits 3:3 */
    ca_uint32_t rsrvd1               : 29 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_ctrl_mask_t;

typedef struct {
    ca_boolean_t sw_mode_en;
    ca_boolean_t dereg_rst_en;
    ca_boolean_t resrv_bc_en;
    ca_uint8_t   resrv_id_sel;
} aal_xgpn_plmp_ctrl_t;

typedef union {
  struct {
    ca_uint32_t mic_chk_dis          :  1 ;
    ca_uint32_t seq_chk_dis          :  1 ;
    ca_uint32_t onuid_flt_dis        :  1 ;
    ca_uint32_t capt_msg_en          :  1 ;
    ca_uint32_t capt_msg_type        :  1 ;
    ca_uint32_t capt_onu_id          :  1 ;
    ca_uint32_t extension_en         :  1 ;
    ca_uint32_t unkwn_msg_type_en    :  1 ;
    ca_uint32_t dozing_filter_en     :  1 ;
    ca_uint32_t o7_filter_en         :  1 ;
    ca_uint32_t pktgen_rdy_chk_en    :  1 ;
    ca_uint32_t extension_msg_type_en     :  1 ;
    ca_uint32_t capt_msg_bpressure_en         :  1 ;
    ca_uint32_t rsrvd2               : 19 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_ds_ctrl_mask_t;

typedef struct {
    ca_boolean_t mic_chk_dis      ;
    ca_boolean_t seq_chk_dis      ;
    ca_boolean_t onuid_flt_dis    ;
    ca_boolean_t capt_msg_en      ;
    ca_uint8_t   capt_msg_type    ;
    ca_uint16_t  capt_onu_id      ;
    ca_boolean_t extension_en     ;
    ca_boolean_t unkwn_msg_type_en;
    ca_boolean_t dozing_filter_en ;
    ca_boolean_t o7_filter_en     ;
    ca_boolean_t pktgen_rdy_chk_en;
    ca_boolean_t extension_msg_type_en;
    ca_boolean_t capt_msg_bpressure_en;
} aal_xgpn_plmp_ds_ctrl_t;


typedef union {
  struct {
    ca_uint32_t byte0                : 8 ;
    ca_uint32_t byte1                : 8 ;
    ca_uint32_t byte2                : 8 ;
    ca_uint32_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_ploam_rgst_id_cfg_t;

/* refer to ploam_ngpon2_us_registration_t */
#define XGPN_PLMP_RGST_ID_SIZE      36
typedef struct {
    ca_uint8_t data[XGPN_PLMP_RGST_ID_SIZE];
} aal_xgpn_plmp_rgst_id_cfg_t;

typedef union {
  struct {
    ca_uint32_t granularity          :  1 ;
    ca_uint32_t time                 :  1 ;
    ca_uint32_t rsrvd1               : 30 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_tuning_parameter_mask_t;

typedef struct{
    ca_uint8_t granularity;
    ca_uint8_t time;
} aal_xgpn_plmp_tuning_parameter_t;


typedef union {
  struct {
    ca_uint32_t level                :  1 ;
    ca_uint32_t capability           :  1 ;
    ca_uint32_t rsrvd2               : 30 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_attenuation_mask_t;

typedef struct {
    ca_uint8_t level;
    ca_uint8_t capability;
} aal_xgpn_plmp_attenuation_t;

typedef union {
  struct {
    ca_uint32_t mic0                : 8 ;
    ca_uint32_t mic1                : 8 ;
    ca_uint32_t mic2                : 8 ;
    ca_uint32_t mic3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_ploam_ack_ploam_mic_t;


#define XGPN_PLMP_MIC_SIZE          8

typedef struct {
    ca_uint8_t mic[XGPN_PLMP_MIC_SIZE];
} aal_xgpn_plmp_ack_ploam_no_msg_t;

typedef union {
  struct {
    ca_uint32_t ok                   :  1 ;
    ca_uint32_t start                :  1 ;
    ca_uint32_t seq                  :  1 ;
    ca_uint32_t mic                  :  1 ;
    ca_uint32_t rsrvd1               : 28 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_ack_ploam_busy_msg_mask_t;


typedef struct {
    ca_boolean_t ok;
    ca_boolean_t start;
    ca_uint8_t seq;
    ca_uint8_t mic[XGPN_PLMP_MIC_SIZE];
} aal_xgpn_plmp_ack_ploam_busy_msg_t;



#define XGPN_PLMP_MSG_BODY_SIZE      36
typedef struct {
    ca_boolean_t done;
    ca_uint8_t msg_type;
    ca_uint8_t data[XGPN_PLMP_MSG_BODY_SIZE];
} aal_xgpn_plmp_cfg_t;

typedef union {
  struct {
    ca_uint32_t byte0                : 8 ;
    ca_uint32_t byte1                : 8 ;
    ca_uint32_t byte2                : 8 ;
    ca_uint32_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_ploam_cfg_t;


typedef union {
  struct {
    ca_uint32_t tk5_timeout          :  1 ;
    ca_uint32_t tk4_timeout          :  1 ;
    ca_uint32_t rsrvd1               : 30 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_key_exchg_thrshd_mask_t;

typedef struct {
    ca_uint8_t  tk5_timeout;
    ca_uint16_t tk4_timeout;
} aal_xgpn_plmp_key_exchg_thrshd_t;


typedef union {
  struct {
    ca_uint32_t wrong_onuid_inti     :  1 ;
    ca_uint32_t bc_seq_mismch_inti   :  1 ;
    ca_uint32_t uc_seq_mismch_inti   :  1 ;
    ca_uint32_t mic_chk_fail_inti    :  1 ;
    ca_uint32_t wrong_ploam_inti     :  1 ;
    ca_uint32_t key_exchg_tk4_timeout_inti :  1 ;
    ca_uint32_t key_exchg_tk5_timeout_inti :  1 ;
    ca_uint32_t key_ctrl_inti        :  1 ;
    ca_uint32_t sleep_allow_inti     :  1 ;
    ca_uint32_t sleep_req_done_inti  :  1 ;
    ca_uint32_t eqd_err_inti         :  1 ;
    ca_uint32_t busy_err_inti        :  1 ;
    ca_uint32_t registration_msg_inti :  1 ;
    ca_uint32_t burst_prfl_inti      :  1 ;
    ca_uint32_t tuning_request       :  1;
    ca_uint32_t rsrvd1               : 16 ;
    ca_uint32_t rxbuf0_rdy_inti      :  1 ;
    ca_uint32_t rxbuf1_rdy_inti      :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_interrupt_mask_t;

typedef struct {
    ca_boolean_t wrong_onuid_inti  ;
    ca_boolean_t bc_seq_mismch_inti;
    ca_boolean_t uc_seq_mismch_inti;
    ca_boolean_t mic_chk_fail_inti ;
    ca_boolean_t wrong_ploam_inti  ;
    ca_boolean_t key_exchg_tk4_timeout_inti ;
    ca_boolean_t key_exchg_tk5_timeout_inti ;
    ca_boolean_t key_ctrl_inti       ;
    ca_boolean_t sleep_allow_inti    ;
    ca_boolean_t sleep_req_done_inti ;
    ca_boolean_t eqd_err_inti        ;
    ca_boolean_t busy_err_inti       ;
    ca_boolean_t registration_msg_inti;
    ca_boolean_t burst_prfl_inti      ;
    ca_boolean_t tuning_request       ;
    ca_boolean_t rxbuf0_rdy_inti      ;
    ca_boolean_t rxbuf1_rdy_inti      ;
} aal_xgpn_plmp_interrupt_t;


typedef union {
  struct {
    ca_uint32_t wrong_onuid_intie     :  1 ;
    ca_uint32_t bc_seq_mismch_intie   :  1 ;
    ca_uint32_t uc_seq_mismch_intie   :  1 ;
    ca_uint32_t mic_chk_fail_intie    :  1 ;
    ca_uint32_t wrong_ploam_intie     :  1 ;
    ca_uint32_t key_exchg_tk4_timeout_intie :  1 ;
    ca_uint32_t key_exchg_tk5_timeout_intie :  1 ;
    ca_uint32_t key_ctrl_intie        :  1 ;
    ca_uint32_t sleep_allow_intie     :  1 ;
    ca_uint32_t sleep_req_done_intie  :  1 ;
    ca_uint32_t eqd_err_intie         :  1 ;
    ca_uint32_t busy_err_intie        :  1 ;
    ca_uint32_t registration_msg_intie :  1 ;
    ca_uint32_t burst_prfl_intie      :  1 ;
    ca_uint32_t rsrvd1               : 16 ;
    ca_uint32_t rxbuf0_rdy_intie      :  1 ;
    ca_uint32_t rxbuf1_rdy_intie      :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_interrupt_enable_mask_t;

typedef struct {
    ca_boolean_t wrong_onuid_intie  ;
    ca_boolean_t bc_seq_mismch_intie;
    ca_boolean_t uc_seq_mismch_intie;
    ca_boolean_t mic_chk_fail_intie ;
    ca_boolean_t wrong_ploam_intie  ;
    ca_boolean_t key_exchg_tk4_timeout_intie ;
    ca_boolean_t key_exchg_tk5_timeout_intie ;
    ca_boolean_t key_ctrl_intie       ;
    ca_boolean_t sleep_allow_intie    ;
    ca_boolean_t sleep_req_done_intie ;
    ca_boolean_t eqd_err_intie        ;
    ca_boolean_t busy_err_intie       ;
    ca_boolean_t registration_msg_intie;
    ca_boolean_t burst_prfl_intie      ;
    ca_boolean_t rxbuf0_rdy_intie      ;
    ca_boolean_t rxbuf1_rdy_intie      ;
} aal_xgpn_plmp_interrupt_enable_t;



typedef struct {
    ca_uint8_t seq;
    ca_uint8_t type;
    ca_uint8_t index;
    ca_uint8_t length;
} aal_xgpn_plmp_key_ctrl_stat_t;

typedef union {
  struct {
    ca_uint32_t seq                  :  1 ;
    ca_uint32_t type                 :  1 ;
    ca_uint32_t index                :  1 ;
    ca_uint32_t fragment_num         :  1 ;
    ca_uint32_t rsrvd1               : 27 ;
    ca_uint32_t rdy                  :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_key_report_mask_t;


typedef struct {
    ca_uint8_t seq          ;
    ca_uint8_t type         ;
    ca_uint8_t index        ;
    ca_uint8_t fragment_num ;
    ca_boolean_t rdy        ;
} aal_xgpn_plmp_key_report_t;


#define XGPN_PLMP_KEY_REPORT_FRAG_SIZE      16
#define XGPN_PLMP_KEY_REPORT_FRAG_OFFSET    24

/* the data should offset XGPN_PLMP_KEY_REPORT_FRAG_OFFSET from whole ploam packet */
typedef struct {
    ca_uint8_t data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE];
} aal_key_report_frag_t;

typedef union {
  struct {
    ca_uint32_t byte0                : 8 ;
    ca_uint32_t byte1                : 8 ;
    ca_uint32_t byte2                : 8 ;
    ca_uint32_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_plmp_key_report_frag_t;


typedef struct {
    ca_uint8_t seq;
    ca_uint8_t type;
} aal_xgpn_plmp_sleep_allow_stat_t;

typedef union {
  struct {
    ca_uint32_t activity_level       :  1 ;
    ca_uint32_t rsrvd1               : 30 ;
    ca_uint32_t rdy                  :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_sleep_req_mask_t;

typedef struct {
    ca_uint8_t activity_level;
    ca_boolean_t rdy;
} aal_xgpn_plmp_sleep_req_t;


typedef union {
  struct {
    ca_uint32_t defkey               :  1 ;
    ca_uint32_t rsrvd1               : 30 ;
    ca_uint32_t pkt_rdy              :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_txbuf_ctrl_mask_t;

typedef struct {
    ca_boolean_t defkey;
    ca_boolean_t pkt_rdy;
} aal_xgpn_plmp_txbuf_ctrl_t;

#define XGPN_PLMP_TXBUF_SIZE        40
typedef struct {
    ca_uint8_t data[XGPN_PLMP_TXBUF_SIZE];
} __attribute__((packed)) aal_xgpn_plmp_txbuf_t;

typedef union {
  struct {
    ca_uint8_t byte0                : 8 ;
    ca_uint8_t byte1                : 8 ;
    ca_uint8_t byte2                : 8 ;
    ca_uint8_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_plmp_txbuf_data_t;


typedef union {
  struct {
    ca_uint32_t rsrvd1               : 28 ;
    ca_uint32_t mic_err              :  1 ;
    ca_uint32_t rx_sfc               :  1 ;
    ca_uint32_t turn                 :  1 ;
    ca_uint32_t pkt_rdy              :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_rxbuf_stat_mask_t;

#define XGPN_PLMP_RXBUF_BLOCK       2

typedef struct {
    ca_boolean_t mic_err;
    ca_uint8_t   rx_sfc;
    ca_boolean_t turn;
    ca_boolean_t pkt_rdy;
} aal_xgpn_plmp_rxbuf_stat_t;

#define XGPN_PLMP_RXBUF_SIZE        48
typedef union {
  struct {
    ca_uint8_t byte0                : 8 ;
    ca_uint8_t byte1                : 8 ;
    ca_uint8_t byte2                : 8 ;
    ca_uint8_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_plmp_rxbuf_t;

typedef struct {
    ca_uint8_t data[XGPN_PLMP_RXBUF_SIZE];
} __attribute__((packed)) aal_xgpn_plmp_rxbuf_t;


typedef union {
  struct {
    ca_uint32_t rsrvd1               : 30 ;
    ca_uint32_t tx_ploam_msg_sel     :  1 ; /* bits 30:30 */
    ca_uint32_t rx_ploam_gen_enable  :  1 ; /* bits 31:31 */
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_plmp_pkt_gen_mask_t;

typedef struct {
    ca_boolean_t tx_ploam_msg_sel;
    ca_boolean_t rx_ploam_gen_enable;
} aal_xgpn_plmp_pkt_gen_t;

typedef union {
  struct {
    ca_uint32_t cnt                 : 32 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_plmp_mib_t;

typedef union {
  struct {
    ca_uint32_t value                 : 32 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_debug_sts_t;



typedef struct {
    ca_uint32_t deact_onu_pkt;
    ca_uint32_t assgn_eqd_pkt;
    ca_uint32_t enable_onu_cnt;
    ca_uint32_t uc_updt_prfl_cnt;
    ca_uint32_t extension_cnt;
    ca_uint32_t regst_cnt;
    ca_uint32_t ack_ok_cnt;
    ca_uint32_t ploamd_err_cnt;
    ca_uint32_t ds_tx_cnt;
    ca_uint32_t ds_drop_bc_cnt;
    ca_uint32_t ds_drop_uc_cnt;
    ca_uint32_t rx_ff_drop_cnt;
    ca_uint32_t exten_drop_cnt;
    ca_uint32_t us_rx_cnt;
    ca_uint32_t us_sw_tx_cnt;

}aal_ploam_msg_cnt_t;


typedef union {
  struct {
    ca_uint32_t cnt                 : 32 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_reg_xgpn_plmp_cnt_t;


typedef struct {
    ca_uint32_t bc_pkt;
    ca_uint32_t uc_pkt;
}aal_xgpn_plmp_ds_drop_cnt_t;

/*
 *  ploam message
 */
#define PLOAM_NGPON2_HEADER_SIZE    4

typedef struct ploam_ngpon2_header_s {
    ca_uint16_t     onu_id;                 /* 1-2 */
    ca_uint8_t      msg_id;                 /* 3 */
    ca_uint8_t      seq_no;                 /* 4 */
}__attribute__((packed))ploam_ngpon2_header_t;

/*
 * downstream ploam
 */
typedef union {
  struct {
    ca_uint32_t     burst_profile_ctrl      : 1 ;
    ca_uint32_t     us_fec_ind              : 1 ;
    ca_uint32_t     delimiter_len           : 1 ;
    ca_uint32_t     delimiter               : 1 ;
    ca_uint32_t     preamble_len            : 1 ;
    ca_uint32_t     preamble_repeat_cnt     : 1 ;
    ca_uint32_t     preamble_pattern        : 1 ;
    ca_uint32_t     pon_tag                 : 1 ;
    ca_uint32_t     padding                 : 1 ;
    ca_uint32_t     mic                     : 1 ;
    ca_uint32_t     rsrvd1                  : 22;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_twdm_burst_profile_mask_t;

typedef struct {
    ca_uint8_t      burst_profile_ctrl;     /* 5 */
    ca_uint8_t      us_fec_ind;             /* 6 */
    ca_uint8_t      delimiter_len;          /* 7 */
    ca_uint8_t      delimiter[8];           /* 8-15 */
    ca_uint8_t      preamble_len;           /* 16 */
    ca_uint8_t      preamble_repeat_cnt;    /* 17 */
    ca_uint8_t      preamble_pattern[8];    /* 18-25 */
    ca_uint8_t      pon_tag[8];             /* 26-33 */
    ca_uint8_t      padding[7];             /* 34-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_twdm_burst_profile_t;

typedef union {
  struct {
    ca_uint32_t     assign_onu_id          : 1 ;
    ca_uint32_t     vendor_id              : 1 ;
    ca_uint32_t     vssn                   : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 27;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_assign_onuid_mask_t;


typedef struct {
    ca_uint16_t     assign_onu_id;          /* 5-6 */
    ca_uint32_t     vendor_id;              /* 7-10 */
    ca_uint32_t     vssn;                   /* 11-14 */
    ca_uint8_t      padding[26];            /* 15-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_assign_onuid_t;

typedef union {
  struct {
    ca_uint32_t     control                : 1 ;
    ca_uint32_t     equ_delay              : 1 ;
    ca_uint32_t     ds_pon_id              : 1 ;
    ca_uint32_t     us_pon_id              : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 26;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_ranging_time_mask_t;

typedef struct {
    ca_uint8_t      control;                /* 5 */
    ca_uint32_t     equ_delay;              /* 6-9  */
    ca_uint32_t     ds_pon_id;              /* 10-13 */
    ca_uint32_t     us_pon_id;              /* 14-17 */
    ca_uint8_t      padding[23];            /* 18-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_ranging_time_t;

typedef union {
  struct {
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 30;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_deact_onuid_mask_t;

typedef struct {
    ca_uint8_t      padding[36];            /* 5-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_deact_onuid_t;

typedef union {
  struct {
    ca_uint32_t     enable                 : 1 ;
    ca_uint32_t     vendor_id              : 1 ;
    ca_uint32_t     vssn                   : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 27;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_disable_sn_mask_t;

typedef struct {
    ca_uint8_t      enable;                 /* 5 */
    ca_uint32_t     vendor_id;              /* 6-9 */
    ca_uint32_t     vssn;                   /* 10-13 */
    ca_uint8_t      padding[27];            /* 14-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_disable_sn_t;

typedef union {
  struct {
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 30;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_request_reg_mask_t;

typedef struct {
    ca_uint8_t      padding[36];            /* 5-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_request_reg_t;

typedef union {
  struct {
    ca_uint32_t     alloc_id_value         : 1 ;
    ca_uint32_t     alloc_id_type          : 1 ;
    ca_uint32_t     alloc_id_scope         : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 27;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_assign_allocid_mask_t;

typedef struct {
    ca_uint16_t     alloc_id_value;         /* 5-6 */
    ca_uint8_t      alloc_id_type;          /* 7 */
    ca_uint16_t     alloc_id_scope;         /* 8-9 */
    ca_uint8_t      padding[31];            /* 10-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_assign_allocid_t;

typedef union {
  struct {
    ca_uint32_t     ctrl_flag              : 1 ;
    ca_uint32_t     key_idx                : 1 ;
    ca_uint32_t     key_len                : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 27;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_key_ctrl_mask_t;

typedef struct {
    ca_uint8_t      resv;                   /* 5 */
    ca_uint8_t      ctrl_flag;              /* 6 */
    ca_uint8_t      key_idx;                /* 7 */
    ca_uint8_t      key_len;                /* 8 */
    ca_uint8_t      padding[32];            /* 9-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_key_ctrl_t;

typedef union {
  struct {
    ca_uint32_t     ctrl_flag              : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 29;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_sleep_allow_mask_t;

typedef struct {
    ca_uint8_t      ctrl_flag;              /* 5 */
    ca_uint8_t      padding[35];            /* 6-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_sleep_allow_t;

typedef union {
  struct {
    ca_uint32_t     vendor_id              : 1 ;
    ca_uint32_t     vssn                   : 1 ;
    ca_uint32_t     current_pon_id         : 1 ;
    ca_uint32_t     correlation_tag        : 1 ;
    ca_uint32_t     target_ds_pon_id       : 1 ;
    ca_uint32_t     target_us_pon_id       : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 24;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_calibration_req_mask_t;

typedef struct {
    ca_uint32_t     vendor_id;              /* 5-8 */
    ca_uint32_t     vssn;                   /* 9-12 */
    ca_uint32_t     current_pon_id;         /* 13-16 */
    ca_uint16_t     correlation_tag;        /* 17-18 */
    ca_uint32_t     target_ds_pon_id;       /* 19-22 */
    ca_uint32_t     target_us_pon_id;       /* 23-26 */
    ca_uint8_t      padding[14];             /* 27-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_calibration_req_t;

typedef union {
  struct {
    ca_uint32_t     vendor_id              : 1 ;
    ca_uint32_t     vssn                   : 1 ;
    ca_uint32_t     current_pon_id         : 1 ;
    ca_uint32_t     correlation_tag        : 1 ;
    ca_uint32_t     freq_adj_direct        : 1 ;
    ca_uint32_t     freq_adj_size          : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 24;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_adjust_tx_wave_mask_t;

typedef struct {
    ca_uint32_t     vendor_id;              /* 5-8 */
    ca_uint32_t     vssn;                   /* 9-12 */
    ca_uint32_t     current_pon_id;         /* 13-16 */
    ca_uint16_t     correlation_tag;        /* 17-18 */
    ca_uint8_t      freq_adj_direct;        /* 19 */
    ca_uint8_t      freq_adj_size;          /*  20-21 */
    ca_uint8_t      padding[19];            /* 22-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_adjust_tx_wave_t;

typedef union {
  struct {
    ca_uint32_t     op_code                : 1 ;
    ca_uint32_t     scheduled_sfc          : 1 ;
    ca_uint32_t     rollback_flag          : 1 ;
    ca_uint32_t     target_ds_pon_id       : 1 ;
    ca_uint32_t     target_us_pon_id       : 1 ;
    ca_uint32_t     calibration_flag       : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 24;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_tuning_ctrl_mask_t;

typedef struct {
    ca_uint8_t      op_code;                /* 5 */
    ca_uint16_t     scheduled_sfc;         /* 6-7 */
    ca_uint8_t      rollback_flag;          /* 8 */
    ca_uint32_t     target_ds_pon_id;       /* 9-12 */
    ca_uint32_t     target_us_pon_id;       /* 13-16 */
    ca_uint8_t      calibration_flag;       /* 17 */
    ca_uint8_t      padding[23];            /* 18-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_tuning_ctrl_t;

typedef union {
  struct {
    ca_uint32_t     ng2sys_id                 : 1 ;
    ca_uint32_t     sys_profile_ver           : 1 ;
    ca_uint32_t     us_op_wave_len_bands      : 1 ;
    ca_uint32_t     twdm_channel_cnt          : 1 ;
    ca_uint32_t     channel_spacing           : 1 ;
    ca_uint32_t     us_mse                    : 1 ;
    ca_uint32_t     fsr                       : 1 ;
    ca_uint32_t     amcc_ctrl                 : 1 ;
    ca_uint32_t     loose_calibration         : 1 ;
    ca_uint32_t     channel_cnt               : 1 ;
    ca_uint32_t     channel_spacing_ptp       : 1 ;
    ca_uint32_t     us_mse_ptp                : 1 ;
    ca_uint32_t     fsr_ptp                   : 1 ;
    ca_uint32_t     calibration_accuracy      : 1 ;
    ca_uint32_t     loose_calibration_bound   : 1 ;
    ca_uint32_t     pading                    : 1 ;
    ca_uint32_t     mic                       : 1 ;
    ca_uint32_t     rsrvd1                    : 15;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_sys_profile_mask_t;

typedef struct {
    ca_uint8_t      ng2sys_id[3];           /* 5-7 */
    ca_uint8_t      sys_profile_ver;        /* 8 */
    ca_uint8_t      us_op_wave_len_bands;   /* 9 */
    ca_uint8_t      twdm_channel_cnt;       /* 10 */
    ca_uint8_t      channel_spacing;        /* 11 */
    ca_uint8_t      us_mse;                 /* 12 */
    ca_uint16_t     fsr;                    /* 13-14 */
    ca_uint8_t      amcc_ctrl;              /* 15 */
    ca_uint8_t      loose_calibration;      /* 16 */
    ca_uint16_t     channel_cnt;            /* 17-18 */
    ca_uint8_t      channel_spacing_ptp;    /* 19 */
    ca_uint8_t      us_mse_ptp;             /* 20 */
    ca_uint16_t     fsr_ptp;                /* 21-22 */
    ca_uint8_t      calibration_accuracy;   /* 23 */
    ca_uint8_t      loose_calibration_bound;/* 24 */
    ca_uint8_t      pading[16];             /* 25-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_sys_profile_t;

typedef union {
  struct {
    ca_uint32_t     control                : 1 ;
    ca_uint32_t     version                : 1 ;
    ca_uint32_t     pon_id                 : 1 ;
    ca_uint32_t     ds_freq_offset         : 1 ;
    ca_uint32_t     ds_rate                : 1 ;
    ca_uint32_t     channel_partition      : 1 ;
    ca_uint32_t     def_resp_channel       : 1 ;
    ca_uint32_t     sn_grant_type_ind      : 1 ;
    ca_uint32_t     amcc_window            : 1 ;
    ca_uint32_t     uwlch_id               : 1 ;
    ca_uint32_t     us_freq                : 1 ;
    ca_uint32_t     opt_link_type          : 1 ;
    ca_uint32_t     us_rate                : 1 ;
    ca_uint32_t     def_onu_attenuation    : 1 ;
    ca_uint32_t     resp_threshold         : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 15;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_channel_profile_mask_t;

/* TWDM format */
typedef struct {
    ca_uint8_t      control;                /* 5 */
    ca_uint8_t      version;                /* 6 */
    ca_uint32_t     pon_id;                 /* 7-10 */
    ca_uint8_t      ds_freq_offset;         /* 11 */
    ca_uint8_t      ds_rate;                /* 12 */
    ca_uint8_t      channel_partition;      /* 13 */
    ca_uint32_t     def_resp_channel;       /* 14-17 */
    ca_uint8_t      sn_grant_type_ind;      /* 18 */
    ca_uint32_t     amcc_window;            /* 19-22 */
    ca_uint8_t      uwlch_id;               /* 23 */
    ca_uint32_t     us_freq;                /* 24-27 */
    ca_uint8_t      opt_link_type;          /* 28 */
    ca_uint8_t      us_rate;                /* 29 */
    ca_uint8_t      def_onu_attenuation;    /* 30 */
    ca_uint8_t      resp_threshold;         /* 31 */
    ca_uint8_t      padding[9];             /* 32-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_channel_profile_t;

typedef union {
  struct {
    ca_uint32_t     protect_ds_pon_id      : 1 ;
    ca_uint32_t     protect_us_pon_id      : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 28;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_protection_ctrl_mask_t;

typedef struct {
    ca_uint32_t     protect_ds_pon_id;      /* 5-8 */
    ca_uint32_t     protect_us_pon_id;      /* 9-12 */
    ca_uint8_t      enable_flag;            /* 13 */
    ca_uint8_t      padding[27];            /* 14-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_protection_ctrl_t;

typedef union {
  struct {
    ca_uint32_t     op_type                : 1 ;
    ca_uint32_t     attenuation            : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 28;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_change_power_mask_t;

typedef struct {
    ca_uint8_t      op_type;                /* 5 */
    ca_uint8_t      attenuation;            /* 6 */
    ca_uint8_t      padding[34];            /* 7-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_change_power_t;

typedef union {
  struct {
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 30;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_power_consume_mask_t;

typedef struct {
    ca_uint8_t      padding[36];            /* 5-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_power_consume_t;

typedef union {
  struct {
    ca_uint32_t     op_code                : 1 ;
    ca_uint32_t     scheduled_sfc          : 1 ;
    ca_uint32_t     rollback_flag          : 1 ;
    ca_uint32_t     norinal_line_rate      : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 : 26;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_rate_ctrl_mask_t;

typedef struct {
    ca_uint8_t      op_code;                /* 5 */
    ca_uint16_t     scheduled_sfc;          /* 6-7 */
    ca_uint8_t      rollback_flag;          /* 8 */
    ca_uint8_t      norinal_line_rate;      /* 9 */
    ca_uint8_t      padding[31];            /* 10-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_rate_ctrl_t;


/*
 * upstream ploam
 */

typedef union {
  struct {
    ca_uint32_t     vendor_id              : 1 ;
    ca_uint32_t     vssn                   : 1 ;
    ca_uint32_t     random_delay           : 1 ;
    ca_uint32_t     correlation_tag        : 1 ;
    ca_uint32_t     current_ds_pon_id      : 1 ;
    ca_uint32_t     current_us_pon_id      : 1 ;
    ca_uint32_t     calibration_rec_status : 1 ;
    ca_uint32_t     tuning_granularity     : 1 ;
    ca_uint32_t     step_tuning_time       : 1 ;
    ca_uint32_t     us_line_rate_cap       : 1 ;
    ca_uint32_t     attenuation            : 1 ;
    ca_uint32_t     power_level_cap        : 1 ;
    ca_uint32_t     padding                : 1 ;
    ca_uint32_t     mic                    : 1 ;
    ca_uint32_t     rsrvd1                 :18 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_serial_number_mask_t;

typedef struct {
    ca_uint32_t     vendor_id;              /* 5-8 */
    ca_uint32_t     vssn;                   /* 9-12 */
    ca_uint32_t     random_delay;           /* 13-16 twdm, ptp wdm reserved */
    ca_uint16_t     correlation_tag;        /* 17-18 */
    ca_uint32_t     current_ds_pon_id;      /* 19-22 */
    ca_uint32_t     current_us_pon_id;      /* 23-26 */
    ca_uint8_t      calibration_rec_status[8]; /* 27-34 twdm, ptp wdm named sn_digest */
    ca_uint8_t      tuning_granularity;     /* 35 */
    ca_uint8_t      step_tuning_time;       /* 36 */
    ca_uint8_t      us_line_rate_cap;       /* 37 */
    ca_uint8_t      attenuation;            /* 38 */
    ca_uint8_t      power_level_cap;        /* 39 */
    ca_uint8_t      padding;                /* 40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_serial_number_t;


typedef union {
  struct {
    ca_uint32_t registration_id       :  1 ;
    ca_uint32_t rsrvd1                : 30 ;
    ca_uint32_t mic                   :  1 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_registration_mask_t;


typedef struct {
    ca_uint8_t      registration_id[36];    /* 5-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_registration_t;


typedef union {
  struct {
    ca_uint32_t report_type          : 1 ;
    ca_uint32_t key_idx              : 1 ;
    ca_uint32_t fragment_no          : 1 ;
    ca_uint32_t resv                 : 1 ;
    ca_uint32_t key_fragment         : 1 ;
    ca_uint32_t mic                  : 1 ;
    ca_uint32_t rsrvd1               : 26 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_key_report_mask_t;


typedef struct {
    ca_uint8_t      report_type;            /* 5 */
    ca_uint8_t      key_idx;                /* 6 */
    ca_uint8_t      fragment_no;            /* 7 */
    ca_uint8_t      resv;                   /* 8 */
    ca_uint8_t      key_fragment[32];       /* 9-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_key_report_t;

typedef union {
  struct {
    ca_uint8_t  complete_code        : 1 ;
    ca_uint8_t  attenuation          : 1 ;
    ca_uint8_t  power_level_cap      : 1 ;
    ca_uint8_t  padding              : 1 ;
    ca_uint8_t  mic                  : 1 ;
    ca_uint32_t rsrvd1               : 27 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_ack_mask_t;

typedef enum {
    AAL_PLOAM_ACK_COMPLETE_OK       = 0x00,    /*OK */
    AAL_PLOAM_ACK_NO_MSG_TO_SEND    = 0x01,    /*No message to send */
    AAL_PLOAM_ACK_BUSY              = 0x02,    /*Busy, preparing a response */
    AAL_PLOAM_ACK_UNKNOWN_MSG_TYPE  = 0x03,    /*Unknown message type */
    AAL_PLOAM_ACK_PARAMETER_ERROR   = 0x04,    /*Parameter error */
    AAL_PLOAM_ACK_PROCESSING_ERROR  = 0x05,    /*Processing error */
    AAL_PLOAM_ACK_COMPLETE_RESERVED
} aal_ploam_ack_complete_code_t;

typedef struct {
    ca_uint8_t      complete_code;          /* 5 */
    ca_uint8_t      attenuation;            /* 6 */
    ca_uint8_t      power_level_cap;        /* 7 */
    ca_uint8_t      padding[33];            /* 8-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_ack_t;

typedef union {
  struct {
    ca_uint32_t activity_level       :  1 ;
    ca_uint32_t padding              :  1 ;
    ca_uint32_t mic                  :  1 ;
    ca_uint32_t rsrvd1               : 29 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_sleep_req_mask_t;


typedef struct {
    ca_uint8_t      activity_level;         /* 5 */
    ca_uint8_t      padding[35];            /* 6-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_sleep_req_t;

typedef union {
  struct {
    ca_uint32_t op_code                 : 1 ;
    ca_uint32_t resp_code               : 1 ;
    ca_uint32_t vendor_id               : 1 ;
    ca_uint32_t vssn                    : 1 ;
    ca_uint32_t correlation_tag         : 1 ;
    ca_uint32_t pon_id                  : 1 ;
    ca_uint32_t uwlch_id                : 1 ;
    ca_uint32_t calibration_rec_status  : 1 ;
    ca_uint32_t tuning_granularity      : 1 ;
    ca_uint32_t one_step_tuning_time    : 1 ;
    ca_uint32_t us_line_rate_cap        : 1 ;
    ca_uint32_t attenuation             : 1 ;
    ca_uint32_t power_level_cap         : 1 ;
    ca_uint32_t padding                 : 1 ;
    ca_uint32_t mic                     : 1 ;
    ca_uint32_t rsrvd1                  : 17 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_tuning_resp_mask_t;


typedef struct {
    ca_uint8_t      op_code;                /* 5 */
    ca_uint16_t     resp_code;              /* 6-7 */
    ca_uint32_t     vendor_id;              /* 7-10 */
    ca_uint32_t     vssn;                   /* 11-14 */
    ca_uint16_t     correlation_tag;        /* 15-16 */
    ca_uint32_t     pon_id;                 /* 17-20 */
    ca_uint8_t      uwlch_id;               /* 21 */
    ca_uint8_t      calibration_rec_status; /* 22-29 sn digest for ptp wdm */
    ca_uint8_t      tuning_granularity;     /* 30 */
    ca_uint8_t      one_step_tuning_time;   /* 31 */
    ca_uint8_t      us_line_rate_cap;       /* 32 */
    ca_uint8_t      attenuation;            /* 33 */
    ca_uint8_t      power_level_cap;        /* 34 */
    ca_uint8_t      padding[6];             /* 25-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_tuning_resp_t;

typedef union {
  struct {
    ca_uint32_t power_consumption    : 1 ;
    ca_uint32_t padding              : 1 ;
    ca_uint32_t mic                  : 1 ;
    ca_uint32_t rsrvd1               : 29 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_power_consumption_report_mask_t;


typedef struct {
    ca_uint8_t      power_consumption[24];  /* 5-28 */
    ca_uint8_t      padding[12];            /* 29-40 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_power_consumption_report_t;

typedef union {
  struct {
    ca_uint8_t  op_code              : 1 ;
    ca_uint8_t  resp_code            : 1 ;
    ca_uint8_t  padding              : 1 ;
    ca_uint8_t  mic                  : 1 ;
    ca_uint32_t rsrvd1               : 28 ;
  } bf ;
  ca_uint32_t     wrd ;
}ploam_ngpon2_us_rate_resp_mask_t;

typedef struct {
    ca_uint8_t      op_code;                /* 5 */
    ca_uint8_t      resp_code;              /* 6 */
    ca_uint8_t      padding[34];            /* 34 */
    ca_uint8_t      mic[8];                 /* 41-48 */
} __attribute__((packed)) ploam_ngpon2_us_rate_resp_t;

typedef volatile union {
  struct {
    ca_uint32_t complt_code        :  1 ;
    ca_uint32_t seqno              :  1 ;
    ca_uint32_t chng_pwr_level     :  1 ;
    ca_uint32_t protect_ctrl       :  1 ;
    ca_uint32_t rsrvd1             : 28 ;
  } s ;
  ca_uint32_t     wrd ;
} aal_ploam_tuning_sw_ack_mask_t;

typedef struct {
    ca_uint8_t complt_code;
    ca_uint8_t seqno;
    ca_uint8_t chng_pwr_level;
    ca_uint8_t protect_ctrl;
} aal_ploam_tuning_sw_ack_t;

typedef volatile union {
  struct {
    ca_uint32_t tuning_request        :  1 ;
    ca_uint32_t tuning_operation_code :  1 ;
    ca_uint32_t seqno                 :  1 ;
    ca_uint32_t tuning_response_code  :  1 ;
    ca_uint32_t rsrvd1                : 28 ;
  } s ;
  ca_uint32_t     wrd ;
} aal_ploam_tuning_sw_resp_mask_t;

typedef struct {
    ca_uint8_t tuning_request;
    ca_uint8_t tuning_operation_code;
    ca_uint8_t seqno;
    ca_uint16_t tuning_response_code;
} aal_ploam_tuning_sw_resp_t;

typedef union {
  struct {
    ca_uint32_t request_schedule_sfc :  1 ;
    /*ca_uint32_t response_code        :  1 ;
    ca_uint32_t operation_code       :  1 ;*/
    ca_uint32_t skip_rx              :  1 ;
    ca_uint32_t rollback_allow       :  1 ;
    ca_uint32_t request_lead_sfc     :  1 ;
    ca_uint32_t rsrvd1               : 28 ;
  } s ;
  ca_uint32_t     wrd ;
} aal_ploam_tuning_cfg_mask_t;

typedef struct {
    ca_uint16_t request_schedule_sfc;
    /*ca_uint8_t response_code        ;
    ca_uint8_t operation_code       ;*/
    ca_uint8_t skip_rx              ;
    ca_uint8_t rollback_allow       ;
    ca_uint8_t request_lead_sfc     ;
} aal_ploam_tuning_cfg_t;


#define DEFAULT_XGPN_ONU_ID             0x3FF

typedef struct {
    ca_uint32_t  ds_rx_cnt;
    ca_uint32_t  mir_err_cnt;
    ca_uint32_t  range_time_cnt;
    ca_uint32_t  protection_ctrl_cnt;
    ca_uint32_t  adjust_tx_wavelength_cnt;
    ca_uint32_t  calibrt_request_cnt;
    ca_uint32_t  sys_profile_cnt;
    ca_uint32_t  channel_profile_cnt;
    ca_uint32_t  burst_profile_cnt;
    ca_uint32_t  assign_onu_id_cnt;
    ca_uint32_t  power_consump_inquire_cnt;
    ca_uint32_t  deactive_onu_id_cnt;
    ca_uint32_t  disable_sn_cnt;
    ca_uint32_t  request_regsitration_cnt;
    ca_uint32_t  assign_alloc_id_cnt;
    ca_uint32_t  key_ctrl_cnt;
    ca_uint32_t  sleep_allow_cnt;
}aal_ploam_rx_mib_t;

typedef struct {
    ca_uint32_t us_tx_cnt;
    ca_uint32_t sn_onu_cnt;
    ca_uint32_t registration_cnt;
    ca_uint32_t key_report_cnt;
    ca_uint32_t ack_cnt;
    ca_uint32_t sleep_request_cnt;
    ca_uint32_t tuning_resp_ack_nack_cnt;
    ca_uint32_t tuning_resp_complete_u_rollback_cnt;
    ca_uint32_t power_consumption_report_cnt;

} aal_ploam_tx_mib_t;


typedef union {
  struct {
    ca_uint32_t capture_start   :  1 ;
    ca_uint32_t capture_dump    :  1 ;
    ca_uint32_t capture_flush   :  1 ;
    ca_uint32_t print_level     :  1 ;
    ca_uint32_t rsrvd1          : 28 ;
  } s ;
  ca_uint32_t     wrd ;
} aal_ploam_dbg_mask_t;



typedef struct {
    ca_uint8_t  capture_start;
    ca_uint8_t  capture_dump;
    ca_uint8_t  capture_flush;
    ca_uint8_t  print_level;
}aal_ploam_dbg_t;

/*
 *
 * prototype
 *
 */

ca_status_t aal_ploam_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t     device_id
);

ca_status_t aal_ploam_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
);

ca_status_t aal_ploam_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
);

ca_status_t aal_ploam_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_ctrl_t       *ctrl
);

ca_status_t aal_ploam_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_ctrl_t  *ctrl
);

ca_status_t aal_ploam_ds_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ds_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_ds_ctrl_t       *ctrl
);

ca_status_t aal_ploam_ds_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_ds_ctrl_t       *ctrl
);

ca_status_t aal_ploam_rx_capture_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          msg_en
);

ca_status_t aal_ploam_rx_capture_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t      *msg_en

);

ca_status_t aal_ploam_us_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         mic_gen_dis
);

ca_status_t aal_ploam_us_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_boolean_t         *mic_gen_dis
);

ca_status_t aal_ploam_vendor_id_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          vendor_id
);

ca_status_t aal_ploam_vendor_id_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *vendor_id
);

ca_status_t aal_ploam_vssn_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          vssn
);

ca_status_t aal_ploam_vssn_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *vssn
);

ca_status_t aal_ploam_correlation_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          correlation
);

ca_status_t aal_ploam_correlation_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint16_t         *correlation
);

ca_status_t aal_ploam_us_pon_id_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
);

ca_status_t aal_ploam_us_pon_id_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
);

ca_status_t aal_ploam_calibration_even_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
);

ca_status_t aal_ploam_calibration_even_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
);

ca_status_t aal_ploam_calibration_odd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
);

ca_status_t aal_ploam_calibration_odd_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
);

ca_status_t aal_ploam_tuning_parameter_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_tuning_parameter_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_tuning_parameter_t       *ctrl
);

ca_status_t aal_ploam_tuning_parameter_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_tuning_parameter_t       *ctrl
);

ca_status_t aal_ploam_attenuation_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_attenuation_mask_t  mask,
    CA_IN aal_xgpn_plmp_attenuation_t       *data
);

ca_status_t aal_ploam_attenuation_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_attenuation_t       *data
);


ca_status_t aal_ploam_rgst_id_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_rgst_id_cfg_t  *rgst_id
);

ca_status_t aal_ploam_rgst_id_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_rgst_id_cfg_t *rgst_id
);


ca_status_t aal_ploam_eqd_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          eqd
);

ca_status_t aal_ploam_eqd_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *eqd
);

ca_status_t aal_ploam_ack_ploam_no_msg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ack_ploam_no_msg_t  *mic
);

ca_status_t aal_ploam_ack_ploam_no_msg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_ack_ploam_no_msg_t *mic
);

ca_status_t aal_ploam_ack_ploam_busy_msg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ack_ploam_busy_msg_mask_t mask,
    CA_IN aal_xgpn_plmp_ack_ploam_busy_msg_t  *busy_msg
);

ca_status_t aal_ploam_ack_ploam_busy_msg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_ack_ploam_busy_msg_t *busy_msg
);

ca_status_t aal_ploam_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_cfg_t  *plmp_cfg
);

ca_status_t aal_ploam_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_cfg_t *plmp_cfg
);


ca_status_t aal_ploam_key_exchg_thrshd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_key_exchg_thrshd_mask_t  thrshd_mask,
    CA_IN aal_xgpn_plmp_key_exchg_thrshd_t       *thrshd
);

ca_status_t aal_ploam_key_exchg_thrshd_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_exchg_thrshd_t  *thrshd
);

ca_status_t aal_ploam_interrupt_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_interrupt_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_interrupt_t       *ctrl
);

ca_status_t aal_ploam_interrupt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_interrupt_t       *ctrl
);

ca_status_t aal_ploam_interrupt_enable0_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_interrupt_enable_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_interrupt_enable_t       *ctrl
);

ca_status_t aal_ploam_interrupt_enable0_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_interrupt_enable_t       *ctrl
);

ca_status_t aal_ploam_key_ctrl_stat_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_ctrl_stat_t       *ctrl
);

ca_status_t aal_ploam_key_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_key_report_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_key_report_t       *ctrl
);

ca_status_t aal_ploam_key_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_report_t       *ctrl
);


ca_status_t aal_ploam_key_report_frag_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_key_report_frag_t  *frag
);

ca_status_t aal_ploam_key_report_frag_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_key_report_frag_t *frag
);

ca_status_t aal_ploam_key_sleep_allow_stat_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_sleep_allow_stat_t       *ctrl
);

ca_status_t aal_ploam_sleep_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_sleep_req_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_sleep_req_t       *ctrl
);

ca_status_t aal_ploam_sleep_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_sleep_req_t       *ctrl
);

ca_status_t aal_ploam_txbuf_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_txbuf_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_txbuf_ctrl_t       *ctrl
);

ca_status_t aal_ploam_txbuf_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_txbuf_ctrl_t       *ctrl
);

ca_status_t aal_ploam_txbuf_data_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_txbuf_t   *txbuf
);

ca_status_t aal_ploam_txbuf_data_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_txbuf_t       *txbuf
);

ca_status_t aal_ploam_rxbuf_stat_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          buf_idx,   /* 0..1 */
    CA_IN aal_xgpn_plmp_rxbuf_stat_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_rxbuf_stat_t       *ctrl
);

ca_status_t aal_ploam_rxbuf_stat_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t       buf_idx,   /* 0..1 */
    CA_OUT aal_xgpn_plmp_rxbuf_stat_t       *ctrl
);

ca_status_t aal_ploam_rxbuf_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          buf_idx,
    CA_IN ca_boolean_t         has_mic,
    CA_IN aal_xgpn_plmp_rxbuf_t       *rxbuf
);

ca_status_t aal_ploam_rxbuf_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t       buf_idx,
    CA_IN ca_boolean_t      has_mic,
    CA_OUT aal_xgpn_plmp_rxbuf_t       *rxbuf
);

ca_status_t aal_ploam_rx_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT aal_ploam_rx_mib_t *rx_mib
);

ca_status_t aal_ploam_tx_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT aal_ploam_tx_mib_t *tx_mib
);

ca_status_t aal_ploam_debug_sts0_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
);

ca_status_t aal_ploam_debug_sts1_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
);

ca_status_t aal_ploam_debug_sts2_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
);

ca_status_t aal_ploam_msg_cnt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_ploam_msg_cnt_t *cnt
);

ca_status_t aal_ploam_bc_ploam_seq_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint8_t *seq
);

ca_status_t aal_ploam_uc_ploam_seq_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint8_t *seq
);

ca_status_t aal_ploam_ds_burst_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_twdm_burst_profile_mask_t  mask,
    CA_IN ploam_ngpon2_twdm_burst_profile_t       *data
);

ca_status_t aal_ploam_ds_burst_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_twdm_burst_profile_t       *data
);

ca_status_t aal_ploam_ds_assign_onuid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_assign_onuid_mask_t  mask,
    CA_IN ploam_ngpon2_assign_onuid_t       *data
);

ca_status_t aal_ploam_ds_assign_onuid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_assign_onuid_t       *data
);

ca_status_t aal_ploam_ds_ranging_time_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_ranging_time_mask_t  mask,
    CA_IN ploam_ngpon2_ranging_time_t       *data
);

ca_status_t aal_ploam_ds_ranging_time_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_ranging_time_t       *data
);

ca_status_t aal_ploam_ds_deact_onuid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_deact_onuid_mask_t  mask,
    CA_IN ploam_ngpon2_deact_onuid_t       *data
);

ca_status_t aal_ploam_ds_deact_onuid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_deact_onuid_t       *data
);

ca_status_t aal_ploam_ds_disable_sn_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_disable_sn_mask_t  mask,
    CA_IN ploam_ngpon2_disable_sn_t       *data
);

ca_status_t aal_ploam_ds_disable_sn_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_disable_sn_t       *data
);

ca_status_t aal_ploam_ds_request_reg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_request_reg_mask_t  mask,
    CA_IN ploam_ngpon2_request_reg_t       *data
);

ca_status_t aal_ploam_ds_request_reg_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_request_reg_t       *data
);

ca_status_t aal_ploam_ds_assign_allocid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_assign_allocid_mask_t  mask,
    CA_IN ploam_ngpon2_assign_allocid_t       *data
);

ca_status_t aal_ploam_ds_assign_allocid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_assign_allocid_t       *data
);

ca_status_t aal_ploam_ds_key_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_key_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_key_ctrl_t       *data
);

ca_status_t aal_ploam_ds_key_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_key_ctrl_t       *data
);

ca_status_t aal_ploam_ds_sleep_allow_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_sleep_allow_mask_t  mask,
    CA_IN ploam_ngpon2_sleep_allow_t       *data
);

ca_status_t aal_ploam_ds_sleep_allow_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_sleep_allow_t       *data
);

ca_status_t aal_ploam_ds_calibration_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_calibration_req_mask_t  mask,
    CA_IN ploam_ngpon2_calibration_req_t       *data
);

ca_status_t aal_ploam_ds_calibration_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_calibration_req_t       *data
);

ca_status_t aal_ploam_ds_adjust_tx_wave_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_adjust_tx_wave_mask_t  mask,
    CA_IN ploam_ngpon2_adjust_tx_wave_t       *data
);

ca_status_t aal_ploam_ds_adjust_tx_wave_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_adjust_tx_wave_t       *data
);

ca_status_t aal_ploam_ds_tuning_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_tuning_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_tuning_ctrl_t       *data
);

ca_status_t aal_ploam_ds_tuning_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_tuning_ctrl_t       *data
);

ca_status_t aal_ploam_ds_sys_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_sys_profile_mask_t  mask,
    CA_IN ploam_ngpon2_sys_profile_t       *data
);

ca_status_t aal_ploam_ds_sys_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_sys_profile_t       *data
);

ca_status_t aal_ploam_ds_channel_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_channel_profile_mask_t  mask,
    CA_IN ploam_ngpon2_channel_profile_t       *data
);

ca_status_t aal_ploam_ds_channel_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_channel_profile_t       *data
);

ca_status_t aal_ploam_ds_protection_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_protection_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_protection_ctrl_t       *data
);

ca_status_t aal_ploam_ds_protection_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_protection_ctrl_t       *data
);

ca_status_t aal_ploam_ds_change_power_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_change_power_mask_t  mask,
    CA_IN ploam_ngpon2_change_power_t       *data
);

ca_status_t aal_ploam_ds_change_power_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_change_power_t       *data
);

ca_status_t aal_ploam_ds_power_consume_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_power_consume_mask_t  mask,
    CA_IN ploam_ngpon2_power_consume_t       *data
);

ca_status_t aal_ploam_ds_power_consume_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_power_consume_t       *data
);

ca_status_t aal_ploam_ds_rate_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_rate_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_rate_ctrl_t       *data
);

ca_status_t aal_ploam_ds_rate_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_rate_ctrl_t       *data
);

ca_status_t aal_ploam_us_serial_number_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_serial_number_mask_t  mask,
    CA_IN ploam_ngpon2_us_serial_number_t       *data
);

ca_status_t aal_ploam_us_serial_number_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_serial_number_t       *data
);

ca_status_t aal_ploam_us_registration_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_registration_mask_t  mask,
    CA_IN ploam_ngpon2_us_registration_t       *data
);

ca_status_t aal_ploam_us_registration_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_registration_t       *data
);

ca_status_t aal_ploam_us_key_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_key_report_mask_t  mask,
    CA_IN ploam_ngpon2_us_key_report_t       *data
);

ca_status_t aal_ploam_us_key_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_key_report_t       *data
);

ca_status_t aal_ploam_us_ack_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_ack_mask_t  mask,
    CA_IN ploam_ngpon2_us_ack_t       *data
);

ca_status_t aal_ploam_us_ack_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_ack_t       *data
);

ca_status_t aal_ploam_us_sleep_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_sleep_req_mask_t  mask,
    CA_IN ploam_ngpon2_us_sleep_req_t       *data
);

ca_status_t aal_ploam_us_sleep_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_sleep_req_t       *data
);

ca_status_t aal_ploam_us_tuning_resp_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_tuning_resp_mask_t  mask,
    CA_IN ploam_ngpon2_us_tuning_resp_t       *data
);

ca_status_t aal_ploam_us_tuning_resp_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_tuning_resp_t       *data
);

ca_status_t aal_ploam_us_power_consumption_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_power_consumption_report_mask_t  mask,
    CA_IN ploam_ngpon2_us_power_consumption_report_t       *data
);

ca_status_t aal_ploam_us_power_consumption_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_power_consumption_report_t       *data
);

ca_status_t aal_ploam_us_rate_resp_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_rate_resp_mask_t  mask,
    CA_IN ploam_ngpon2_us_rate_resp_t       *data
);

ca_status_t aal_ploam_us_rate_resp_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_rate_resp_t       *data
);

ca_status_t aal_ploam_us_pkt_send_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          onu_id,
    CA_IN ca_uint8_t           msg_id,
    CA_IN ca_uint8_t           seq_no,
    CA_IN ca_boolean_t         use_def_key
);

ca_status_t aal_ploam_us_pkt_send_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint16_t       onu_id,
    CA_IN ca_uint8_t        msg_id,
    CA_IN ca_uint8_t        seq_no,
    CA_IN ca_boolean_t      use_def_key,
    CA_OUT ca_uint32_t      *value
);

ca_status_t aal_ploam_loopback_ploam_send_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint8_t        msg_id
);

ca_status_t aal_ploam_loopback_ploam_send_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint8_t        msg_id,
    CA_OUT aal_xgpn_plmp_cfg_t      *value
);

ca_status_t aal_ploam_tuning_cfg_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_ploam_tuning_cfg_mask_t mask,
    CA_IN aal_ploam_tuning_cfg_t     *cfg
);

ca_status_t aal_ploam_tuning_cfg_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_ploam_tuning_cfg_t     *cfg
);

ca_status_t aal_ploam_tuning_sw_ack_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_ploam_tuning_sw_ack_t     *cfg
);
ca_status_t aal_ploam_tuning_sw_ack_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_ploam_tuning_sw_ack_mask_t mask,
    CA_IN aal_ploam_tuning_sw_ack_t     *cfg
);
ca_status_t aal_ploam_tuning_sw_resp_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT  aal_ploam_tuning_sw_resp_t *cfg
);

ca_status_t aal_ploam_tuning_sw_resp_set(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  aal_ploam_tuning_sw_resp_mask_t  mask,
    CA_IN  aal_ploam_tuning_sw_resp_t  *cfg
);

ca_status_t aal_ploam_power_consumption_set(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_uint32_t                 channel,
    CA_IN  ca_uint32_t                 consumption
);

ca_status_t aal_ploam_power_consumption_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_uint32_t                 channel,
    CA_OUT ca_uint32_t                *consumption
);

ca_boolean_t aal_ploam_dbg_capture_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id);

ca_status_t aal_ploam_dbg_get(
    CA_IN  ca_device_id_t   device_id,
    CA_OUT aal_ploam_dbg_t *dbg);

ca_status_t aal_ploam_dbg_set(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  aal_ploam_dbg_mask_t mask,
    CA_IN  aal_ploam_dbg_t *dbg);

ca_status_t aal_ploam_dbg_capture_add(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t      *pkt,
    CA_IN  ca_uint32_t      len);

#endif /* __AAL_PLOAM_H__ */

