#ifndef __AAL_GPON_H__
#define __AAL_GPON_H__


#define AAL_GPON_PORT_PASSWORD_LEN     10
#ifdef CONFIG_LUNA_G3_SERIES
#define AAL_GPON_SYSTEM_MAX_TCONT_NUM  16
#else
#define AAL_GPON_SYSTEM_MAX_TCONT_NUM  8
#endif
#define AAL_GPON_SYSTEM_MAX_GEM_NUM    64
#define AAL_GPON_SYSTEM_MAX_ALLOCID    4096
#define AAL_GPON_SYSTEM_MAX_GEM_PORTS  4096
#define AAL_GPON_OMCI_RSV_PORT_MAX     8
#define AAL_GPON_PLOAM_MIB_READ_CLR    2
#ifdef CONFIG_LUNA_G3_SERIES
#define AAL_GPON_PLOAM_MIB_READ        0
#endif
#define AAL_GPON_AES_KEY_LEN           16
#define AAL_GPON_OMCC_GEM_IDX          6


typedef enum {
    AAL_GPON_PLOAM_RX_MIB_UPSTREAM_OVERHEAD     = 0x101,
    AAL_GPON_PLOAM_RX_MIB_ASSIGN_ONU_ID         = 0x103,
    AAL_GPON_PLOAM_RX_MIB_RANGING_TIME          = 0x104,
    AAL_GPON_PLOAM_RX_MIB_DEACTIVATE_ONU_ID     = 0x105,
    AAL_GPON_PLOAM_RX_MIB_DISABLE_SERIAL_NUMBER = 0x106,
    AAL_GPON_PLOAM_RX_MIB_ENCRYPTED_PORT_ID     = 0x108,
    AAL_GPON_PLOAM_RX_MIB_REQUEST_PASSWORD      = 0x109,
    AAL_GPON_PLOAM_RX_MIB_ASSIGN_ALLOC_ID       = 0x10A,
    AAL_GPON_PLOAM_RX_MIB_NO_MESSAGE            = 0x10B,
    AAL_GPON_PLOAM_RX_MIB_POPUP                 = 0x10C,
    AAL_GPON_PLOAM_RX_MIB_REQUEST_KEY           = 0x10D,
    AAL_GPON_PLOAM_RX_MIB_CONFIGURE_PORT_ID     = 0x10E,
    AAL_GPON_PLOAM_RX_MIB_PEE                   = 0x10F,
    AAL_GPON_PLOAM_RX_MIB_CHANGE_POWER_LEVEL    = 0x110,
    AAL_GPON_PLOAM_RX_MIB_PST_MESSAGE           = 0x111,
    AAL_GPON_PLOAM_RX_MIB_BER_INTERVAL          = 0x112,
    AAL_GPON_PLOAM_RX_MIB_KEY_SWITCHING_TIME    = 0x113,
    AAL_GPON_PLOAM_RX_MIB_EXTENDED_BURST_LENGTH = 0x114,
    AAL_GPON_PLOAM_RX_MIB_CRC_ERROR             = 0x200,
}aal_gpon_ploam_rx_mib_type_t;


typedef enum {
    AAL_GPON_PLOAM_TX_MIB_SERIAL_NUMBER_ONU = 1,
    AAL_GPON_PLOAM_TX_MIB_PASSWORD          = 2,
    AAL_GPON_PLOAM_TX_MIB_DYING_GASP        = 3,
    AAL_GPON_PLOAM_TX_MIB_NO_MESSAGE        = 4,
    AAL_GPON_PLOAM_TX_MIB_ENCRYPTION_KEY    = 5,
    AAL_GPON_PLOAM_TX_MIB_PEE               = 6,
    AAL_GPON_PLOAM_TX_MIB_PST_MESSAGE       = 7,
    AAL_GPON_PLOAM_TX_MIB_REI               = 8,
    AAL_GPON_PLOAM_TX_MIB_ACKNOWLEDGE       = 9,
}aal_gpon_ploam_tx_mib_type_t;


typedef enum {
    AAL_GPON_CFG_RESET_ALL  = 0,
    AAL_GPON_CFG_RESET_PON  = 1,
    AAL_GPON_CFG_RESET_PSDS = 2,
    AAL_GPON_CFG_MAX
}aal_gpon_cfg_type_t;
 
typedef enum {
    AAL_GPON_TO1_TIMER = 0,
    AAL_GPON_TO2_TIMER
}aal_gpon_gtc_timers_t;

typedef enum {
    AAL_GPON_STATE_INITIAL = 0,
    AAL_GPON_STATE_STANDBY,
    AAL_GPON_STATE_SERIALNO,
    AAL_GPON_STATE_RANGING,
    AAL_GPON_STATE_OPERATION,
    AAL_GPON_STATE_POPUP,
    AAL_GPON_STATE_ESTOP,
    AAL_GPON_STATE_MAX
} aal_gpon_onu_states_t;

typedef enum {
    AAL_GPON_ALARM_LOS  = 0x0001,
    AAL_GPON_ALARM_LOF  = 0x0002,
    AAL_GPON_ALARM_SF   = 0x0004,
    AAL_GPON_ALARM_SD   = 0x0008,
    AAL_GPON_ALARM_LCDG = 0x0010,
    AAL_GPON_ALARM_SUF  = 0x0020,
    AAL_GPON_ALARM_DACT = 0x0040,
    AAL_GPON_ALARM_DIS  = 0x0080,
    AAL_GPON_ALARM_MIS  = 0x0100,
    AAL_GPON_ALARM_PEE  = 0x0200,
    AAL_GPON_ALARM_RDI  = 0x0400,
    AAL_GPON_ALARM_TF   = 0x0800
}aal_gpon_alarms_t;

typedef enum {
    AAL_GPON_ONU_PWR_LOW = 0,
    AAL_GPON_ONU_PWR_MEDIUM,
    AAL_GPON_ONU_PWR_HIGH,
    AAL_GPON_ONU_PWR_RESERVED
} aal_gpon_onu_pwr_lvl_t;


typedef enum {
    AAL_GPON_PLOAMu_SERIAL_NUMBER = 1,
    AAL_GPON_PLOAMu_PASSWORD,
    AAL_GPON_PLOAMu_DYING_GASP,
    AAL_GPON_PLOAMu_NO_MESSAGE,
    AAL_GPON_PLOAMu_ENCRYPTION_KEY,
    AAL_GPON_PLOAMu_PEE,
    AAL_GPON_PLOAMu_PST,
    AAL_GPON_PLOAMu_REI,
    AAL_GPON_PLOAMu_ACK
} aal_gpon_onu_ploam_us_t;


typedef enum {
    AAL_GPON_PLOAMd_USOH = 1,
    AAL_GPON_PLOAMd_SNMASK,
    AAL_GPON_PLOAMd_ASSIGN_ONUID,
    AAL_GPON_PLOAMd_RANGING_TIME,
    AAL_GPON_PLOAMd_DEACT_ONUID,
    AAL_GPON_PLOAMd_DISABLE_SN,
    AAL_GPON_PLOAMd_ENCRYPTED_PORTID = 8,
    AAL_GPON_PLOAMd_REQUEST_PASSWD,
    AAL_GPON_PLOAMd_ASSIGN_ALLOCID,
    AAL_GPON_PLOAMd_NOMSG,
    AAL_GPON_PLOAMd_POPUP,
    AAL_GPON_PLOAMd_REQUEST_KEY,
    AAL_GPON_PLOAMd_CONFIGURE_PORTID,
    AAL_GPON_PLOAMd_PEE,
    AAL_GPON_PLOAMd_CPL,
    AAL_GPON_PLOAMd_PST,
    AAL_GPON_PLOAMd_BER_INTERVAL,
    AAL_GPON_PLOAMd_KEY_SWITCH_TIME,
    AAL_GPON_PLOAMd_EBL,
    AAL_GPON_PLOAMd_RSVD
} aal_gpon_onu_ploam_ds_t;

typedef enum {
    AAL_GPON_SIG_THRES_10_EXP_3,
    AAL_GPON_SIG_THRES_10_EXP_4,
    AAL_GPON_SIG_THRES_10_EXP_5,
    AAL_GPON_SIG_THRES_10_EXP_6,
    AAL_GPON_SIG_THRES_10_EXP_7,
    AAL_GPON_SIG_THRES_10_EXP_8, 
    AAL_GPON_SIG_THRES_10_EXP_9,
    AAL_GPON_SIG_THRES_10_EXP_10,
    AAL_GPON_SIG_THRES_10_EXP_11,
} aal_gpon_signal_thres_val_t;


typedef union {
    struct {
        ca_uint32_t pre_sync_threshold   :  1; 
        ca_uint32_t miss_sync_threshold  :  1; 
        ca_uint32_t los_timer_threshold  :  1; 
        ca_uint32_t fec_switch_threshold :  1;
        ca_uint32_t max_packet_size      :  1; 
        ca_uint32_t tag_exclude          :  1; 
        ca_uint32_t stop_gpn_fe          :  1; 
        ca_uint32_t rsvd                 : 25;
    }s;
    ca_uint32_t u32;
}aal_gpon_ds_frame_thrsd_cfg_msk_t;


typedef struct {
    ca_uint32_t pre_sync_threshold; 
    ca_uint32_t miss_sync_threshold; 
    ca_uint32_t los_timer_threshold; 
    ca_uint32_t fec_switch_threshold;
    ca_uint32_t max_packet_size; 
    ca_uint32_t tag_exclude; 
    ca_uint32_t stop_gpn_fe; 
}aal_gpon_ds_frame_thrsd_cfg_t;


typedef union {
    struct {
        ca_uint32_t req_threshold   :  1; 
        ca_uint32_t rspnd_extend    :  1; 
        ca_uint32_t rsvd            : 30;
    } s;
    ca_uint32_t u32;
} aal_gpon_sn_req_res_ctrl_cfg_msk_t;


typedef struct {
    ca_uint32_t req_threshold; 
    ca_uint32_t rspnd_extend; 
}aal_gpon_sn_req_res_ctrl_cfg_t;


typedef union {
    struct {
        ca_uint32_t eqd_select  :  1; 
        ca_uint32_t frame_var   :  1; 
        ca_uint32_t rsvd        : 30;
    } s;
    ca_uint32_t u32;
} aal_gpon_us_frm_ctrl_cfg_msk_t;


typedef struct {
    ca_uint32_t eqd_select; 
    ca_uint32_t frame_var; 
} aal_gpon_us_frm_ctrl_cfg_t;


typedef union {
    struct {
        ca_uint32_t degrade_remove_threshold    :  1; 
        ca_uint32_t degrade_threshold           :  1; 
        ca_uint32_t fail_remove_threshold       :  1; 
        ca_uint32_t fail_threshold              :  1; 
        ca_uint32_t rsvd                        : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_sf_degrd_alrm_thrsd_cfg_msk_t;


typedef struct {
    ca_uint32_t degrade_remove_threshold; 
    ca_uint32_t degrade_threshold; 
    ca_uint32_t fail_remove_threshold; 
    ca_uint32_t fail_threshold; 
} aal_gpon_sf_degrd_alrm_thrsd_cfg_t;


typedef union {
    struct {
        ca_uint32_t key_index1          :  1; 
        ca_uint32_t key_index0          :  1; 
        ca_uint32_t dying_gasp_extra    :  1;
        ca_uint32_t hw_key_switch       :  1;
        ca_uint32_t rsvd                : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_ploamd_ctrl_cfg_msk_t;


typedef struct {
    ca_uint32_t key_index1; 
    ca_uint32_t key_index0; 
    ca_uint32_t dying_gasp_extra; 
    ca_uint32_t hw_key_switch; 
} aal_gpon_ploamd_ctrl_cfg_t;


typedef struct {
    ca_uint8  key[AAL_GPON_AES_KEY_LEN];
} aal_gpon_aes_key_t;


typedef union {
    struct {
        ca_uint32_t k2          :  1; 
        ca_uint32_t k1          :  1; 
        ca_uint32_t line_num    :  1; 
        ca_uint32_t rsvd        : 29;
    } s;
    ca_uint32_t u32;
} aal_gpon_pst_cfg_msk_t;


typedef struct {
    ca_uint32_t k2; 
    ca_uint32_t k1; 
    ca_uint32_t line_num; 
} aal_gpon_pst_cfg_t;


typedef struct {
    ca_uint32_t bip_error_accum;
    ca_uint32_t bip_error_frame_cnt;
    ca_uint32_t ds_omci_gem;
    ca_uint32_t ds_omci_pkt;
    ca_uint32_t ds_pkt_crc;
    ca_uint32_t ds_undersz_pkt;
    ca_uint32_t ds_oversz_pkt;
    ca_uint32_t superframe;

    ca_uint32_t o5_state;/* Number of time the GPON mac entered O5 state */
    ca_uint32_t gem_frag_drp;/* Number of dropped downstream gem fragments due to the design limitation of two open fragments */
    ca_uint32_t gem_err_1bit;/* Number of 1 and 2 bit error corrected by HW */
    ca_uint32_t gem_err_2bit;
    ca_uint32_t gem_uncorrect_bit_err;/* GEM frames with un-correctable bit errors */
    ca_uint32_t bwmap_drop;/* Number of dropped BWMAPs */
    ca_uint32_t omci_crc_err;/* Number of OMCI frames received with CRC errors */
    ca_uint32_t plend_err;/* Number of times when both PLEND fields were unusable */
    ca_uint32_t plend_bit_err;/* Number of times when both PLEND fields have single or double bit CRC error */
    ca_uint32_t ds_asmbl_fifo_drop_cnt;
    ca_uint32_t bwmap_uncorrected_biterr;
    ca_uint32_t bwmap_corrected_biterr;
}aal_gpon_stats_t;


typedef struct {
    ca_uint8_t t2_preamble; 
    ca_uint8_t t1_preamble; 
    ca_uint8_t guard; 
    ca_uint8_t t3_preamble; 
    ca_uint32_t delimiter;
    ca_uint32_t pre_assgin_eqd;
    ca_boolean_t pre_assgin_eqd_en;
}aal_us_ovrh_plm_msg_params_t;


typedef struct {
    ca_uint8_t             onu_id; 
    ca_uint16_t            omci_port; 
    ca_uint32_t            ber_interval;
    ca_uint32_t            key_switch_time;
    aal_gpon_onu_pwr_lvl_t pwr_lvl;
} aal_gpon_onu_params_t;


typedef struct {
    ca_uint8_t   extra; 
    ca_boolean_t en; 
    ca_boolean_t mask_en; 
}aal_gpon_sn_acquisition_status_t;


typedef struct {
    ca_uint8_t preranged_t3_preamble; 
    ca_uint8_t ranged_t3_preamble; 
} aal_gpon_type3_preamble_cfg_t;


typedef union {
    struct {
        ca_uint32_t no_up                :  1;
        ca_uint32_t en                   :  1;
        ca_uint32_t stop                 :  1;
        ca_uint32_t omcc_padding         :  1;
        ca_uint32_t dpath_rst            :  1;
        ca_uint32_t aes_en               :  1;
        ca_uint32_t ploam_crc_en         :  1;
        ca_uint32_t pm_mode              :  1;
        ca_uint32_t plen_match           :  1;
        ca_uint32_t stop_us_wr           :  1;
        ca_uint32_t stop_hw_plu          :  1;
        ca_uint32_t sleep_mode           :  1;
        ca_uint32_t sch_lead_time        :  1;
        ca_uint32_t omci_crc_dis         :  1;
        ca_uint32_t us_scramb_dis        :  1;
        ca_uint32_t ds_scramb_dis        :  1;
        ca_uint32_t dbru_blk_size        :  1;
        ca_uint32_t laser_pre_bias       :  1;
        ca_uint32_t laser_on             :  1;
        ca_uint32_t plmd_ff_wr_ext       :  1;
        ca_uint32_t laser_on_align       :  1;
        ca_uint32_t rsvd                 :  9;
    } s;
    ca_uint32_t u32;
} aal_gpon_mac_cfg_msk_t;


typedef struct {
    ca_uint32_t no_up;
    ca_uint32_t en;
    ca_uint32_t stop;
    ca_uint32_t omcc_padding;
    ca_uint32_t dpath_rst;
    ca_uint32_t aes_en;
    ca_uint32_t ploam_crc_en;
    ca_uint32_t pm_mode;
    ca_uint32_t plen_match;
    ca_uint32_t stop_us_wr;
    ca_uint32_t stop_hw_plu;
    ca_uint32_t sleep_mode;
    ca_uint32_t sch_lead_time;
    ca_uint32_t omci_crc_dis;
    ca_uint32_t us_scramb_dis;
    ca_uint32_t ds_scramb_dis;
    ca_uint32_t dbru_blk_size;
    ca_uint32_t laser_pre_bias;
    ca_uint32_t laser_on;
    ca_uint32_t plmd_ff_wr_ext;
    ca_uint32_t laser_on_align;
}aal_gpon_mac_cfg_t;


typedef union {
    struct {
        ca_uint32_t ptp_en          :  1;
        ca_uint32_t cfm_en          :  1;
        ca_uint32_t ptp_check_da    :  1;
        ca_uint32_t cfm_check_da    :  1;
        ca_uint32_t rsvd            : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_ds_pkt_process_ctrl_msk_t;


typedef struct {
    ca_uint16_t  ptp_en;
    ca_uint8_t   cfm_en;
    ca_boolean_t ptp_check_da;
    ca_boolean_t cfm_check_da;
} aal_gpon_ds_pkt_process_ctrl_cfg_t;


typedef union {
    struct {
        ca_uint32_t tcont_index : 1;
        ca_uint32_t omci_en     : 1;
        ca_uint32_t ploam_en    : 1;
        ca_uint32_t rsvd        : 29;
    } s;
    ca_uint32_t u32;
} aal_gpon_tcont_cfg_msk_t;


typedef struct {
    ca_uint16_t  tcont_index;
    ca_boolean_t omci_en;
    ca_boolean_t ploam_en;
} aal_gpon_tcont_cfg_t;


typedef union {
    struct {
        ca_uint32_t                 gem_index   : 1;
        ca_uint32_t                 tdm         : 1;
        ca_uint32_t                 aes         : 1;
        ca_uint32_t                 vld         : 1;
        ca_uint32_t                 rsvd        : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_gem_port_cfg_msk_t;


typedef struct {
    ca_uint16_t      gem_index;
    ca_boolean_t     tdm;
    ca_boolean_t     aes;
    ca_boolean_t     vld;
} aal_gpon_gem_port_cfg_t;


typedef struct {
    ca_uint32_t     pcnt;
    ca_uint32_t     fcnt;
    ca_uint64_t     bcnt;
} aal_gpon_ds_gem_port_mib_t;


typedef struct {
    ca_uint32_t     fcnt;
    ca_uint64_t     bcnt;
} aal_gpon_us_gem_port_mib_t;


typedef union {
    struct {
        ca_uint32_t sw_en     :  1;
        ca_uint32_t sw_dpid   :  1;
        ca_uint32_t sw_cos    :  1;
        ca_uint32_t ptp_dpid  :  1;
        ca_uint32_t ptp_cos   :  1;
        ca_uint32_t omci_dpid :  1;
        ca_uint32_t omci_cos  :  1;
        ca_uint32_t rsvd      : 25;
    } s;
    ca_uint32_t u32;
} aal_gpon_ds_pkt_bypass_cfg_msk_t;


typedef struct {
    ca_boolean_t sw_en;
    ca_uint8_t   sw_dpid;
    ca_uint8_t   sw_cos;
    ca_uint8_t   ptp_dpid;
    ca_uint8_t   ptp_cos;
    ca_uint8_t   omci_dpid;
    ca_uint8_t   omci_cos;
} aal_gpon_ds_pkt_bypass_cfg_t;


typedef union {
    struct {
        ca_uint32_t bufocc_mode     :  1;
        ca_uint32_t tcon_residue_en :  1;
        ca_uint32_t enble           :  1;
        ca_uint32_t data            :  1;
        ca_uint32_t rsvd            : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_sw_dbru_cfg_msk_t;


typedef struct {
    ca_boolean_t bufocc_mode;
    ca_boolean_t tcon_residue_en;
    ca_boolean_t enble;
    ca_uint16_t  data;
} aal_gpon_sw_dbru_cfg_t;


typedef union {
    struct {
        ca_uint32_t flush_en                            :  1;
        ca_uint32_t flush_id                            :  1;
        ca_uint32_t mapper_frame_no_wait_stt            :  1;
        ca_uint32_t mapper_contiguous_frame_no_wait_stt :  1;
        ca_uint32_t rsvd                                : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_mac_ctrl_cfg_msk_t;


typedef struct {
    ca_boolean_t flush_en;
    ca_boolean_t flush_id;
    ca_boolean_t mapper_frame_no_wait_stt;
    ca_boolean_t mapper_contiguous_frame_no_wait_stt;
} aal_gpon_mac_ctrl_cfg_t;


typedef union {
    struct {
        ca_uint32_t halt_cor        :  1;
        ca_uint32_t halt_unc        :  1;
        ca_uint32_t cor_ctr_clr     :  1;
        ca_uint32_t unc_ctr_clr     :  1;
        ca_uint32_t rsvd            : 28;
    } s;
    ca_uint32_t u32;
} aal_gpon_mac_fec_ctrl_cfg_msk_t;


typedef struct {
    ca_boolean_t halt_cor;
    ca_boolean_t halt_unc;
    ca_boolean_t cor_ctr_clr;
    ca_boolean_t unc_ctr_clr;
} aal_gpon_mac_fec_ctrl_cfg_t;


typedef struct {
    ca_boolean_t fec_dacor_ack_q;
    ca_boolean_t fec_uncorr;
    ca_uint8_t   fec_errors;
}aal_gpon_mac_fec_misc_status_t;


typedef struct {
    ca_uint32_t fec_rcnt;
    ca_uint32_t fec_ucnt;
    ca_uint32_t	fec_noerr;
    ca_uint32_t fec_blk_cnt;
    ca_uint32_t fec_corr_byte_cnt;
}aal_gpon_mac_fec_mibs_t;


typedef union {
    struct {
        ca_uint32_t cntr_inccfg         :  1;
        ca_uint32_t sch_to_threshold    :  1;
        ca_uint32_t single_thread       :  1;
        ca_uint32_t cntr2_event_sel     :  1;
        ca_uint32_t cntr1_event_sel     :  1;
        ca_uint32_t cntr0_event_sel     :  1;
        ca_uint32_t cntr_tconid_en      :  1;
        ca_uint32_t cntr_tconid         :  1;
        ca_uint32_t rsvd                :  24;
    } s;
    ca_uint32_t u32;
} aal_gpon_mac_pucif_ctrl_cfg_msk_t;


typedef struct {
    ca_uint8_t   cntr_inccfg;
    ca_uint16_t  sch_to_threshold;
    ca_boolean_t single_thread;
    ca_uint8_t   cntr2_event_sel;
    ca_uint8_t   cntr1_event_sel;
    ca_uint8_t   cntr0_event_sel;
    ca_boolean_t cntr_tconid_en;
    ca_uint8_t   cntr_tconid;
} aal_gpon_mac_pucif_ctrl_cfg_t;


typedef struct {
    ca_uint32_t pmac2puc_cnt;
    ca_uint32_t sob_puc2pmac_cnt;
    ca_uint32_t sof_puc2pmac_cnt;
} aal_gpon_mac_pucif_cnt_t;


typedef struct {
    ca_uint8_t data_gemid;
    ca_uint8_t data_tconid;
    ca_uint8_t sch_tconid;
} aal_gpon_mac_pucif_sts_t;


typedef struct {
    ca_uint8  passwd[AAL_GPON_PORT_PASSWORD_LEN];
} aal_gpon_passwd_t;


typedef struct {
    ca_uint16 to1_timer_threshold;//seconds
    ca_uint16 to2_timer_threshold;//Milliseconds
    ca_uint8  passwd[AAL_GPON_PORT_PASSWORD_LEN];
}aal_gpon_port_cfg_t;


ca_status_t aal_gpon_ds_frame_thrsd_set(
    CA_IN ca_device_id_t                    device_id,
    CA_IN aal_gpon_ds_frame_thrsd_cfg_msk_t mask,
    CA_IN aal_gpon_ds_frame_thrsd_cfg_t     *cfg);

ca_status_t aal_gpon_ds_frame_thrsd_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_gpon_ds_frame_thrsd_cfg_t  *cfg);

ca_status_t aal_gpon_sn_req_res_ctrl_set(
    CA_IN ca_device_id_t                     device_id,
    CA_IN aal_gpon_sn_req_res_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_sn_req_res_ctrl_cfg_t     *cfg);

ca_status_t aal_gpon_sn_req_res_ctrl_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_gpon_sn_req_res_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_us_frm_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_gpon_us_frm_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_us_frm_ctrl_cfg_t     *cfg);

ca_status_t aal_gpon_us_frm_ctrl_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT aal_gpon_us_frm_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_sf_degrd_alrm_thrsd_set(
    CA_IN ca_device_id_t                         device_id,
    CA_IN aal_gpon_sf_degrd_alrm_thrsd_cfg_msk_t mask,
    CA_IN aal_gpon_sf_degrd_alrm_thrsd_cfg_t     *cfg);

ca_status_t aal_gpon_sf_degrd_alrm_thrsd_get(
    CA_IN  ca_device_id_t                      device_id,
    CA_OUT aal_gpon_sf_degrd_alrm_thrsd_cfg_t  *cfg);

ca_status_t aal_gpon_send_dying_gasp(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t   send_extra);

ca_status_t aal_gpon_ploamd_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_gpon_ploamd_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_ploamd_ctrl_cfg_t     *cfg);

ca_status_t aal_gpon_ploamd_ctrl_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT aal_gpon_ploamd_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_aes_key_set(
    CA_IN ca_device_id_t      device_id,
    CA_IN aal_gpon_aes_key_t *key);

ca_status_t aal_gpon_aes_key_get(
    CA_IN  ca_device_id_t      device_id,
    CA_OUT aal_gpon_aes_key_t *key);

ca_status_t aal_gpon_olt_pst_get(
    CA_IN  ca_device_id_t       device_id,
    CA_OUT aal_gpon_pst_cfg_t *cfg);

ca_status_t aal_gpon_last_bip_error_get(
    CA_IN  ca_device_id_t  dev_id,
    CA_OUT ca_uint32_t    *p_bip_count,
    CA_OUT ca_uint32_t    *p_rei_sequence);

ca_status_t aal_gpon_current_bip_error_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_boolean_t   clear_pmon,
    CA_OUT ca_uint32_t    *p_bip_count_accu,
    CA_OUT ca_uint32_t    *p_bip_frame_count);

ca_status_t aal_gpon_current_alarms_get(
    CA_IN  ca_device_id_t  dev_id,
    CA_OUT ca_uint32_t     *p_bitmap);

ca_status_t aal_gpon_port_stats_get(
    CA_IN  ca_device_id_t    dev_id,
    CA_IN  ca_uint8_t        port_id,
    CA_OUT aal_gpon_stats_t  *p_gpon_stats);

ca_status_t aal_gpon_port_stats_clear(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id);

ca_status_t aal_gpon_us_ovrh_plm_msg_params_get(
    CA_IN  ca_device_id_t                dev_id,
    CA_IN  ca_uint8_t                    port_id,
    CA_OUT aal_us_ovrh_plm_msg_params_t  *us_ovhd_para);

ca_status_t aal_gpon_sn_acquisition_status_get(
    CA_IN  ca_device_id_t                    dev_id,
    CA_IN  ca_uint8_t                        port_id,
    CA_OUT aal_gpon_sn_acquisition_status_t  *sn_acquisition_status);

ca_status_t aal_gpon_onu_state_get(
    CA_IN  ca_device_id_t         dev_id,
    CA_IN  ca_uint8_t             port_id, 
    CA_OUT aal_gpon_onu_states_t  *p_state);

ca_status_t aal_gpon_eqd_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *p_eqd);

ca_status_t aal_gpon_protect_eqd_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *p_eqd);

ca_status_t aal_gpon_onu_params_get(
    CA_IN  ca_device_id_t         dev_id,
    CA_IN  ca_uint8_t             port_id,
    CA_OUT aal_gpon_onu_params_t  *onu_para);

ca_status_t aal_gpon_type3_preamble_get(
    CA_IN  ca_device_id_t                 dev_id,
    CA_IN  ca_uint8_t                     port_id,
    CA_OUT aal_gpon_type3_preamble_cfg_t  *t3_preamble_cfg);

ca_status_t aal_gpon_send_ploam_us(
    CA_IN ca_device_id_t           dev_id,
    CA_IN ca_uint8_t               port_id,
    CA_IN aal_gpon_onu_ploam_us_t  ploam_type,
    CA_IN ca_uint8_t               *p_message);

ca_status_t aal_gpon_send_ploam_ds(
    CA_IN ca_device_id_t          dev_id,
    CA_IN ca_uint8_t              port_id, 
    CA_IN aal_gpon_onu_ploam_ds_t ploam_type,
    CA_IN ca_uint8_t              onu_id,
    CA_IN ca_boolean_t            ignore_hw_state,
    CA_IN ca_uint8_t              *p_message);

/*Add [:N] for CLI code generation, N should be 8*(n+1) and n>0*/
ca_status_t aal_gpon_ds_ploam_msg_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id, 
    CA_OUT ca_uint8_t     *p_message/*[:16]*/, 
    CA_OUT ca_uint8_t     *p_num);

ca_status_t aal_gpon_mac_cfg_set(
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_gpon_mac_cfg_msk_t mask,
    CA_IN aal_gpon_mac_cfg_t     *cfg);

ca_status_t aal_gpon_mac_cfg_get(
    CA_IN  ca_device_id_t      device_id,
    CA_OUT aal_gpon_mac_cfg_t  *cfg);

ca_status_t aal_gpon_ploamd_disable_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask);

ca_status_t aal_gpon_ploamd_disable_mask_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *mask);

ca_status_t aal_gpon_ploamd_fifo_cap_ena_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask);

ca_status_t aal_gpon_ploamd_fifo_cap_ena_mask_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *mask);

ca_status_t aal_gpon_ploamu_disable_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask);

ca_status_t aal_gpon_ploamu_disable_mask_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *mask);

ca_status_t aal_gpon_ds_pkt_process_ctrl_set(
    CA_IN ca_device_id_t                     device_id,
    CA_IN aal_gpon_ds_pkt_process_ctrl_msk_t mask,
    CA_IN aal_gpon_ds_pkt_process_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_ds_pkt_process_ctrl_get(
    CA_IN  ca_device_id_t                     device_id,
    CA_OUT aal_gpon_ds_pkt_process_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_tcont_get(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_boolean_t         sw_plm_en,
    CA_IN  ca_uint32_t          allocid,
    CA_OUT aal_gpon_tcont_cfg_t *cfg);

ca_status_t aal_gpon_tcont_set(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_boolean_t              sw_plm_en,
    CA_IN ca_uint32_t               allocid,
    CA_IN aal_gpon_tcont_cfg_msk_t  mask,
    CA_IN aal_gpon_tcont_cfg_t     *cfg);

ca_status_t aal_gpon_ds_gem_port_get(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_boolean_t            sw_aes_en,
    CA_IN  ca_uint16_t             gem_port_id,
    CA_OUT aal_gpon_gem_port_cfg_t *cfg);

ca_status_t aal_gpon_ds_gem_port_set(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_boolean_t                sw_aes_en,
    CA_IN ca_uint32_t                 gem_port_id,
    CA_IN aal_gpon_gem_port_cfg_msk_t mask,
    CA_IN aal_gpon_gem_port_cfg_t     *cfg);

ca_status_t aal_gpon_ds_gem_port_mib_get(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint16_t                 gem_index,
    CA_IN ca_uint8_t                   op_code,
    CA_OUT aal_gpon_ds_gem_port_mib_t *cfg);

ca_status_t aal_gpon_us_gem_port_mib_get(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint8_t                  op_code,
    CA_IN  ca_uint8_t                  sel,
    CA_OUT aal_gpon_us_gem_port_mib_t  *cfg);

ca_status_t aal_gpon_ploam_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     op_code,
    CA_IN  ca_uint16_t    sel,
    CA_OUT ca_uint8_t     *cnt);

ca_status_t aal_gpon_us_gem_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     gem_index,
    CA_IN ca_uint16_t    gem_id);

ca_status_t aal_gpon_us_gem_port_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     gem_index,
    CA_OUT ca_uint16_t    *gem_id);

ca_status_t aal_gpon_ds_pkt_bypass_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN aal_gpon_ds_pkt_bypass_cfg_msk_t mask,
    CA_IN aal_gpon_ds_pkt_bypass_cfg_t     *cfg);

ca_status_t aal_gpon_ds_pkt_bypass_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_gpon_ds_pkt_bypass_cfg_t  *cfg);

ca_status_t aal_gpon_sw_ctrl_dba_set(
    CA_IN ca_device_id_t             device_id,
    CA_IN aal_gpon_sw_dbru_cfg_msk_t mask,
    CA_IN aal_gpon_sw_dbru_cfg_t     *cfg);

ca_status_t aal_gpon_sw_ctrl_dba_get(
    CA_IN  ca_device_id_t         device_id,
    CA_OUT aal_gpon_sw_dbru_cfg_t *cfg);

ca_status_t aal_gpon_scratch_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *scratch);

ca_status_t aal_gpon_scratch_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    scratch);

ca_status_t aal_gpon_dbru_bufocc_tbl_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     table_pointer,
    CA_IN  ca_uint8_t     index,
    CA_OUT ca_uint16_t    *buf_occ);

ca_status_t aal_gpon_dbru_bufocc_tbl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     table_pointer,
    CA_IN ca_uint8_t     index,
    CA_IN ca_uint16_t    buf_occ);

ca_status_t aal_gpon_mac_ctrl_set(
    CA_IN ca_device_id_t             device_id,
    CA_IN aal_gpon_mac_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_ctrl_cfg_t    *cfg);

ca_status_t aal_gpon_mac_ctrl_get(
    CA_IN  ca_device_id_t          device_id,
    CA_OUT aal_gpon_mac_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_sup_frame_value_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *value);

ca_status_t aal_gpon_sup_frame_value_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    value);

ca_status_t aal_gpon_fec_ctrl_set(
    CA_IN ca_device_id_t                  device_id,
    CA_IN aal_gpon_mac_fec_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_fec_ctrl_cfg_t     *cfg);

ca_status_t aal_gpon_fec_ctrl_get(
    CA_IN  ca_device_id_t              device_id,
    CA_OUT aal_gpon_mac_fec_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_fec_misc_status_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_gpon_mac_fec_misc_status_t *cfg);

ca_status_t aal_gpon_fec_mibs_get(
    CA_IN  ca_device_id_t          device_id,
    CA_OUT aal_gpon_mac_fec_mibs_t *cfg);

ca_status_t aal_gpon_pucif_ctrl_set(
    CA_IN ca_device_id_t                    device_id,
    CA_IN aal_gpon_mac_pucif_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_pucif_ctrl_cfg_t     *cfg);

ca_status_t aal_gpon_pucif_ctrl_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_gpon_mac_pucif_ctrl_cfg_t *cfg);

ca_status_t aal_gpon_pucif_cnt_get(
    CA_IN  ca_device_id_t           device_id,
    CA_OUT aal_gpon_mac_pucif_cnt_t *cfg);

ca_status_t aal_gpon_pucif_sts_get(
    CA_IN  ca_device_id_t           device_id,
    CA_OUT aal_gpon_mac_pucif_sts_t *cfg);

ca_status_t aal_gpon_puc_protect_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint8_t     *pucif_hang_tcon_id);

ca_status_t aal_gpon_active_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean     active);

ca_status_t aal_gpon_active_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_boolean     *active);

ca_status_t aal_gpon_passwd_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN aal_gpon_passwd_t *passwd);

ca_status_t aal_gpon_passwd_get(
    CA_IN  ca_device_id_t     device_id,
    CA_OUT aal_gpon_passwd_t *passwd);

ca_status_t aal_gpon_vssn_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    vssn);

ca_status_t aal_gpon_vssn_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *vssn);

ca_status_t aal_gpon_vendor_id_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    vendor_id);

ca_status_t aal_gpon_vendor_id_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *vendor_id);

ca_status_t aal_gpon_gtc_timer_set(
    CA_IN ca_device_id_t        device_id,
    CA_IN aal_gpon_gtc_timers_t timer_id,
    CA_IN ca_uint16             timer_len);

ca_status_t aal_gpon_gtc_timer_get(
    CA_IN  ca_device_id_t        device_id,
    CA_IN  aal_gpon_gtc_timers_t timer_id,
    CA_OUT ca_uint16             *timer_len);

ca_status_t aal_gpon_cfg_set(
    CA_IN ca_device_id_t      device_id,
    CA_IN aal_gpon_cfg_type_t cfg_type,
    CA_IN ca_uint32_t         value);

ca_status_t aal_gpon_cfg_get(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  aal_gpon_cfg_type_t cfg_type,
    CA_OUT ca_uint32_t         value);

ca_status_t aal_gpon_ploam_rx_mib_get(
    CA_IN  ca_device_id_t device_id);

ca_status_t aal_gpon_ploam_tx_mib_get(
    CA_IN  ca_device_id_t device_id);

ca_status_t aal_gpon_gems_get(
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_gems_mib_get(
    CA_IN ca_device_id_t device_id);

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_gpon_mib_init(
    CA_IN  ca_device_id_t     device_id,
    CA_IN  ca_uint32_t        us_clear, 
    CA_IN  ca_uint32_t        ds_clear, 
    CA_IN  ca_uint32_t        plm_clear);
#endif

ca_status_t aal_gpon_tcont_bwmaps_get(
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_omcc_tcont_enable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    alloc_id);

ca_status_t aal_gpon_omcc_gem_enable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    port_id);

ca_status_t aal_gpon_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_datapath_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_reinit(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_gpon_backup_ponmac(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_gpon_restore_ponmac(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_gpon_restore_gems(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_gpon_restore_tcont(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);
#endif
#endif /*__AAL_GPON_H__*/

