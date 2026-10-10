#ifndef __AAL_XGPON_H__
#define __AAL_XGPON_H__

#include "ca_types.h"
#include "aal_pon.h"

/* XGPON reset flags */
#define AAL_XGPON_RESET_FLAG_MAC  0x0001
#define AAL_XGPON_RESET_FLAG_SDS  0x0002
#define AAL_XGPON_RESET_FLAG_ALL  0xffffffff

#define AAL_XGPON_BWMP_BURST_MARGIN_TIME_DEFAULT  0x40
#define AAL_XGPON_BWMP_SEQ_AHEAD_TIME_DEFAULT     0x400

#define XGPON_TUNING_DEBUG_PRINT                 0x01
#define XGPON_TUNING_DEBUG_SFC_DROP_SFC0         0x02
#define XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK     0x04
#define XGPON_TUNING_DEBUG_CALIBRATION_FLAG_CHK  0x08
#define XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN     0x100
#define XGPON_TUNING_DEBUG_RESTORE_CLR_TUNING    0x200
#define XGPON_TUNING_DEBUG_CLR_RX_TX_TUNING_FORCE 0x400
#define XGPON_TUNING_DEBUG_SYNC_HUNT_FORCE       0x10000

#define AAL_XGPON_TUNING_DEBUG_ROLLBACK_TO_TARGET_CHN  0x20000
#define AAL_XGPON_ACTFSM_DEBUG_CHN_SCAN_ENA            0x40000
#define AAL_XGPON_ACTFSM_DEBUG_ONU_ID_CHK_ENA          0x80000
#define AAL_XGPON_ACTFSM_DEBUG_ALLOC_ID_CHK_ENA        0x100000


#define AAL_XGPON_SIMULATION_FLAG_TURNING_TXRX         0x0001
#define AAL_XGPON_SIMULATION_FLAG_TURNING_TX           0x0002
#define AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK     0x0004
#define AAL_XGPON_SIMULATION_FLAG_FORCE_HW_FSM         0x0008

#define AAL_XGPON_BURST_PROF_NUM  4


#define AAL_XGPON_MIN_OMCI_PACKET_LEN  14


extern ca_uint32_t g_aal_xgpon_actfsm_debug;
extern ca_int32_t aal_xgpn_mac_debug;
extern ca_int32_t aal_xgpon_aes_debug;
extern ca_int32_t aal_xgpon_aes_sm_debug;
extern ca_int32_t  aal_xgpn_aes_rpt_hw_accelerate;
extern ca_int32_t aal_xgpon_aes_key_type;
extern ca_int32_t aal_xgpon_aes_key_hw_mode ;


#define NGPON2_TUN_DEBUG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_PRINT) ca_printf("%s()::%d: "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)

#define AAL_XGPON_ODD_CALIBRATION_DEFAULT           0xFFFF
#define AAL_XGPON_EVEN_CALIBRATION_DEFAULT          0xFFFF
#define AAL_XGPON_TUNING_PARAM_GRANULARITY_DEFAULT  5
#define AAL_XGPON_TUNING_PARAM_TIME_DEFAULT         0xC8
#define AAL_XGPON_TUNING_LEAD_SFC_DEFAULT           5
#define AAL_XGPON_PDC_LDPID_DEFAULT                 0x6
#define AAL_XGPON_PDC_COS_DEFAULT                   0
#define AAL_XGPON_PUC_FRAME_OVERHEAD                8 /* 8 Byte */
#define AAL_XGPON_PUC_VOQ_NUM_PER_TCOUNT            8
#define AAL_XGPON_PUC_Q_SCH_MODE_DEFAULT            0 /* Strict mode */
#define AAL_XGPON_PUC_VOQ_MAP_MODE_DEFAULT          0
#define AAL_XGPON_USBG_LASER_ALIGN_DEFAULT          2
#define AAL_XGPON_INIT_DELAY                        100
#define AAL_XGPON_ALLOC_ID_INVALID                  0xbaad /* ASIC defined invalid alloc ID */
#define AAL_XGPON_DS_MAP_ENA_DEFAULT                0
#define AAL_XGPON_DEF_CHANNEL_DEFAULT               0
#define AAL_XGPON_PROTECT_CHANNEL_DEFAULT           0
#define AAL_XGPON_CPI_DEFAULT                       0
#define AAL_XGPON_OLT_TYPE_DEFAULT                  0

#define AAL_XGPON_TOZ_DEFAULT                       80000 /* 1000ms */
#define AAL_XGPON_TO1_DEFAULT                       80000
#define AAL_XGPON_TO23_DEFAULT                      1000 /* 120ms */
#define AAL_XGPON_TO4_DEFAULT                       800   /* 100ms */
#define AAL_XGPON_TO5_DEFAULT                       1000
#define AAL_XGPON_BATTERY_UP_DEFAULT                0

#define AAL_XGPON_DEFAULT_XGEM_IDX                  0
#define AAL_XGPON_DEFAULT_TCONT_IDX                 0
#define AAL_XGPON_VENDOR_ID_DEFAULT   0x434f5254    /*CORT*/
#define AAL_XGPON_VSSN_DEFAULT        0x12345678
#define AAL_XGPON_BWMP_RSP_TM_DEFAULT 0x1543
#define AAL_XGPON_REG_ID_DEFAULT      0x00000000

#define AAL_XGPON_MAC_DEBUG_MISC      0x00000001
#define AAL_XGPON_MAC_DEBUG_DSFP      0x00000002
#define AAL_XGPON_MAC_DEBUG_USBG      0x00000003
#define AAL_XGPON_MAC_DEBUG_BWMP      0x00000008
#define AAL_XGPON_MAC_DEBUG_PLMP      0x00000010
#define AAL_XGPON_MAC_DEBUG_ACTP      0x00000020
#define AAL_XGPON_MAC_DEBUG_FEC       0x00000040
#define AAL_XGPON_MAC_DEBUG_AES       0x00000080
#define AAL_XGPON_MAC_DEBUG_ACTFSM    0x00000100
#define AAL_XGPON_MAC_DEBUG_ALL       0x0000ffff

#define AAL_XGPON_DBG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_xgpn_mac_debug) ca_printf("%s:%d "arg, __func__,__LINE__,##__VA_ARGS__)
#define AAL_XGPON_DBG_ACTFSM(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_xgpn_mac_debug & AAL_XGPON_MAC_DEBUG_ACTFSM) ca_printf("%s:%d "arg, __func__,__LINE__,##__VA_ARGS__)
#define AAL_XGPON_DBG_AES(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_xgpn_mac_debug & AAL_XGPON_MAC_DEBUG_AES) ca_printf("%s:%d "arg, __func__,__LINE__,##__VA_ARGS__)

#define AAL_XGPON_WAVE_CHANNEL_MAX_NUM  8
#define AAL_XGPON_WAVE_CHANNEL_DEFAULT  0

#define AAL_XGPON_OMCC_TCONT_IDX 0
#define AAL_XGPON_MAX_TCONT_NUM  32
#define AAL_MAX_LLID_TCONT_IDX   31

#define AAL_XGPON_GEM_PORT_NUM         128
#define AAL_XGPON_MAX_GEM_PORT_IDX     127
#define AAL_XGPON_RX_MIN_FRAME_SIZE    64 
#define AAL_XGPON_RX_MAX_FRAME_SIZE    12000

#define AAL_BWMP_GRANT_FIFO_MAX_IDX   256
#define AAL_XGPON_MAX_GEM_NUM   128
#define AAL_PUC_MAX_QUEUE_NUM 128
#define AAL_PUC_MAX_QUEUE_PER_PORT_IDX 7

#define AAL_PUC_MAX_WEIGHT_ID 255
#define AAL_PUC_MAX_PRI_ID 7

typedef enum {
    /*
    2'b00: reserved
    2'b01: Support 2.488G line rate in NG-PON2 or XGS-PON mode
    2'b10: Support 9.953G line rate in NG-PON2 or XGS-PON mode
    2'b11: Support both 9.953G and 2.488G line rate in NG-PON2 or XGS-PON mode
    */
    AAL_XGPON_US_LINE_RATE_RSVD = 0,
    AAL_XGPON_US_LINE_RATE_2G = 1,
    AAL_XGPON_US_LINE_RATE_XG = 2,
    AAL_XGPON_US_LINE_RATE_BOTH_2G_XG = 3,

    AAL_XGPON_US_LINE_RATE_NUM
} aal_xgpon_us_line_rate_t;

typedef enum {
    CA_AAL_XGPON_KEY_IDX_0                  = 0,
    CA_AAL_XGPON_KEY_IDX_1                  = 1
} aal_xgpon_enc_key_id_t;

typedef enum {
    CA_AAL_XGPON_KEY_SW_MODE                  = 0,
    CA_AAL_XGPON_KEY_HW_MODE                  = 1
} aal_xgpon_aes_key_hw_mode_t;

typedef enum {
    CA_AAL_XGPON_REGID_FROM_REGST_PLOAM       = 0,
    CA_AAL_XGPON_REGID_IS_ZERO                = 1
}aal_xgpon_aes_msk_reg_id_src_sel_t;  

typedef enum {
    CA_AAL_XGPON_AES_SMA_NOT_EXISIT      = 0,
    CA_AAL_XGPON_AES_SMA_EXISIT          = 1
}aal_xgpon_aes_sma_mode_t;  

typedef union {
    struct {
        ca_uint32_t key_len: 1;
        ca_uint32_t msk_reg_id_src_sel: 1;
        ca_uint32_t key_hw_mode: 1;
        ca_uint32_t sma_mode: 1;
        ca_uint32_t rsvd: 28;
    } s;
    ca_uint32_t u32;
} aal_xgpon_aes_plmp_gbl_ctrl_msk_t;


typedef struct {
    ca_uint32_t key_len;
    ca_uint32_t msk_reg_id_src_sel;
    ca_uint32_t key_hw_mode;
    ca_uint32_t sma_mode;
} aal_xgpon_aes_plmp_gbl_ctrl_t;

typedef union {
  struct {
    ca_uint8_t byte0                : 8 ;
    ca_uint8_t byte1                : 8 ;
    ca_uint8_t byte2                : 8 ;
    ca_uint8_t byte3                : 8 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_xgpn_aes_key_cfg_t;

typedef union {
    struct {
        ca_uint32_t                 p_key_active: 1;
        ca_uint32_t                 p_effective_key_length: 1;
        ca_uint32_t                 uc_key16: 1;
        ca_uint32_t                 rsvd: 29;
    } s;
    ca_uint32_t u32;
} aal_uc_enc_key_config_msk_t;


typedef struct {
    ca_boolean_t                p_key_active;
    ca_uint8_t                  p_effective_key_length;
    ca_uint8_t                  uc_key[16];
} aal_uc_enc_key_config_t;

typedef union {
    struct {
        ca_boolean_t                p_key_active: 1;
        ca_uint32_t                 p_effective_key_length: 1;
        ca_uint32_t                 mc_key16: 1;
        ca_uint32_t                 rsvd: 29;
    } s;
    ca_uint32_t u32;
} aal_mc_enc_key_config_msk_t;

typedef struct {
    ca_boolean_t                p_key_active;
    ca_uint8_t                  p_effective_key_length;
    ca_uint8_t                  mc_key[16];
} aal_mc_enc_key_config_t;

typedef union {
    struct {
        ca_uint32_t                 ani_rst_n: 1;
        ca_uint32_t                 gpon_rst_n: 1;
        ca_uint32_t                 xgpn_rst_n: 1;
        ca_uint32_t                 rsvd1: 5;
        ca_uint32_t                 us10g_mode: 1;
        ca_uint32_t                 nxgs_mode: 1;
        ca_uint32_t                 rsvd2: 22;
    } s;
    ca_uint32_t u32;
} aal_gpon_glb_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     ani_rst_n;
    ca_uint32_t     gpon_rst_n;
    ca_uint32_t     xgpn_rst_n;
    ca_uint32_t     us10g_mode;
    ca_uint32_t     nxgs_mode;
} aal_gpon_glb_ctrl_config_t;


typedef union {
    struct {
        ca_uint32_t                 mac_en: 1;
        ca_uint32_t                 power_saving_mode: 1;
        ca_uint32_t                 pm_mode: 1;
        ca_uint32_t                 us_line_rate: 1;
        ca_uint32_t                 uwlch_id: 1;
        ca_uint32_t                 flush_id: 1;
        ca_uint32_t                 flush_en: 1;
        ca_uint32_t                 rsvd: 25;
    } s;
    ca_uint32_t u32;
} aal_xgpon_xgpn_glb_ctrl_config_msk_t;

typedef struct {
    ca_uint8_t     mac_en;
    ca_uint8_t     power_saving_mode;
    ca_uint8_t     pm_mode;
    ca_uint8_t     us_line_rate;
    ca_uint8_t     uwlch_id;
    ca_uint8_t     flush_id;
    ca_uint8_t     flush_en;
} aal_xgpon_xgpn_glb_ctrl_config_t;


typedef union {
    struct {
        ca_uint32_t                 xgem_id: 1;
        ca_uint32_t                 port_en: 1;
        ca_uint32_t                 aes_mode: 1;
        ca_uint32_t                 mc_flag: 1;
        ca_uint32_t                 rsvd: 28;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_gem_config_msk_t;

typedef struct {
    ca_uint32_t     xgem_id;
    ca_uint32_t     port_en;
    ca_uint32_t     aes_mode;
    ca_uint32_t     mc_flag;
} aal_xgpon_usbg_gem_config_t;


typedef union {
    struct {
        ca_uint32_t                 key0_idx_vld: 1;
        ca_uint32_t                 key1_idx_vld: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     key0_idx_vld;
    ca_uint32_t     key1_idx_vld;
} aal_xgpon_dsfp_xgem_key_ctrl_config_t;

typedef union {
    struct {
        ca_uint32_t                 key0_idx_vld: 1;
        ca_uint32_t                 key1_idx_vld: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_xgem_mc_key_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     key0_idx_vld;
    ca_uint32_t     key1_idx_vld;
} aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t;

typedef union {
    struct {
        ca_uint32_t                 hec0_en: 1;
        ca_uint32_t                 hec1_en: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_hec_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     hec0_en;
    ca_uint32_t     hec1_en;
} aal_xgpon_dsfp_hec_ctrl_config_t;

typedef union {
    struct {
        ca_uint32_t                 ds_descramble_en: 1;
        ca_uint32_t                 sfc_hecchk_en: 1;
        ca_uint32_t                 ponid_hecchk_en: 1;
        ca_uint32_t                 psbd_err_allow: 1;
        ca_uint32_t                 resync_loss_frame_num: 1;
        ca_uint32_t                 byte_swap_en: 1;
        ca_uint32_t                 bit_swap_en: 1;
        ca_uint32_t                 rsvd: 25;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_sync_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     ds_descramble_en;
    ca_uint32_t     sfc_hecchk_en;
    ca_uint32_t     ponid_hecchk_en;
    ca_uint32_t     psbd_err_allow;
    ca_uint32_t     resync_loss_frame_num;
    ca_uint32_t     byte_swap_en;
    ca_uint32_t     bit_swap_en;
} aal_xgpon_dsfp_sync_ctrl_config_t;

typedef struct{
    ca_uint32_t TOL1             ;
    ca_uint32_t C_flag1          ;
    ca_uint32_t link_type1       ;
    ca_uint32_t P_flag1          ;
    ca_uint32_t DS_FEC_flag1     ;
    ca_uint32_t ODN_class1       ;
    ca_uint32_t RE_flag1         ;
    ca_uint32_t ponid0           ;
}aal_xgpon_dsfp_oc_body_config_t;
typedef union {
    struct {
        ca_uint32_t                 hec_chk_en: 1;
        ca_uint32_t                 bwmp_en: 1;
        ca_uint32_t                 ploam_en: 1;
        ca_uint32_t                 ds_en: 1;
        ca_uint32_t                 drop_xgtc_frame_en: 1;
        ca_uint32_t                 rsvd: 27;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_xgtc_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     hec_chk_en;
    ca_uint32_t     bwmp_en;
    ca_uint32_t     ploam_en;
    ca_uint32_t     ds_en;
    ca_uint32_t     drop_xgtc_frame_en;
} aal_xgpon_dsfp_xgtc_ctrl_config_t;

typedef union {
    struct {
        ca_uint32_t                 hec_chk_en: 1;
        ca_uint32_t                 invld_key_xgem_drop_en: 1;
        ca_uint32_t                 old_key_xgem_drop_en: 1;
        ca_uint32_t                 los_word_cnt_clr_en: 1;
        ca_uint32_t                 short_pkt_drop_en: 1;
        ca_uint32_t                 rsvd: 27;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_xgem_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     hec_chk_en;
    ca_uint32_t     invld_key_xgem_drop_en;
    ca_uint32_t     old_key_xgem_drop_en;
    ca_uint32_t     los_word_cnt_clr_en;
    ca_uint32_t     short_pkt_drop_en;
} aal_xgpon_dsfp_xgem_ctrl_config_t;

typedef union {
    struct {
        ca_uint32_t                 crc_chk_en: 1;
        ca_uint32_t                 omci_min_pkt_size: 1;
        ca_uint32_t                 omci_max_pkt_size: 1;
        ca_uint32_t                 max_pkt_size: 1;
        ca_uint32_t                 rsvd: 29;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_rxmac_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     crc_chk_en;
    ca_uint32_t     omci_min_pkt_size;
    ca_uint32_t     omci_max_pkt_size;
    ca_uint32_t     max_pkt_size;
} aal_xgpon_dsfp_rxmac_ctrl_config_t;


typedef union {
    struct {
        ca_uint32_t                 max_pkt_size: 1;
        ca_uint32_t                 rsvd: 31;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_rxmac_ctrl1_config_msk_t;

typedef struct {
    ca_uint32_t     max_pkt_size;
} aal_xgpon_dsfp_rxmac_ctrl1_config_t;


typedef union {
    struct {
        ca_uint32_t                 ram_clr_en: 1;
        ca_uint32_t                 ram_init_value_sel: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_mib_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     ram_clr_en;
    ca_uint32_t     ram_init_value_sel;
} aal_xgpon_dsfp_mib_ctrl_config_t;

typedef struct {
    ca_uint32_t     frame_cnt;
    ca_uint64_t     byte_cnt;
} aal_xgpon_dsfp_xgem_mib_config_t;

typedef union {
    struct {
        ca_uint32_t                 xgtc_en: 1;
        ca_uint32_t                 scramble_en: 1;
        ca_uint32_t                 laser_on: 1;
        ca_uint32_t                 laser_polarity: 1;
        ca_uint32_t                 swap_tx_bus: 1;
        ca_uint32_t                 pf_align_mode: 1;
        ca_uint32_t                 aes_encrypt_en: 1;
        ca_uint32_t                 laser_on_align: 1;
        ca_uint32_t                 rsvd: 24;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_glb_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     xgtc_en;
    ca_uint32_t     scramble_en;
    ca_uint32_t     laser_on;
    ca_uint32_t     laser_polarity;
    ca_uint32_t     swap_tx_bus;
    ca_uint32_t     pf_align_mode;
    ca_uint32_t     aes_encrypt_en;
    ca_int8_t       laser_on_align;
} aal_xgpon_usbg_glb_ctrl_config_t;


typedef union {
    struct {
        ca_uint32_t                 xgem_en: 1;
        ca_uint32_t                 dbru_en: 1;
        ca_uint32_t                 ploam_en: 1;
        ca_uint32_t                 O7_plmp_off: 1;
        ca_uint32_t                 rsvd: 28;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_xgtc_ctrl_config_msk_t;

typedef struct {
    ca_uint32_t     xgem_en;
    ca_uint32_t     dbru_en;
    ca_uint32_t     ploam_en;
    ca_uint32_t     O7_plmp_off;
} aal_xgpon_usbg_xgtc_ctrl_config_t;


typedef union {
    struct {
        ca_uint32_t                 time: 1;
        ca_uint32_t                 rsvd: 31;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_guard_time_config_msk_t;

typedef struct {
    ca_uint32_t     time;
} aal_xgpon_usbg_guard_time_config_t;

typedef union {
    struct {
        ca_uint32_t                 size: 1;
        ca_uint32_t                 mode: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_idl_frm_config_msk_t;

typedef struct {
    ca_uint32_t     size;
    ca_uint32_t     mode;
} aal_xgpon_usbg_idl_frm_config_t;


typedef union {
    struct {
        ca_uint32_t                 dg: 1;
        ca_uint32_t                 rsvd: 31;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_xgtc_hd_ind_config_msk_t;

typedef struct {
    ca_uint32_t     dg;
} aal_xgpon_usbg_xgtc_hd_ind_config_t;


typedef union {
    struct {
        ca_uint32_t                 lo_threshold: 1;
        ca_uint32_t                 hi_threshold: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_xgem_buf_config_msk_t;

typedef struct {
    ca_uint32_t     lo_threshold;
    ca_uint32_t     hi_threshold;
} aal_xgpon_usbg_xgem_buf_config_t;

typedef union {
    struct {
        ca_uint32_t                 no_empty: 1;
        ca_uint32_t                 mode: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_ploamu_qu_config_msk_t;

typedef struct {
    ca_uint32_t     no_empty;
    ca_uint32_t     mode;
} aal_xgpon_usbg_ploamu_qu_config_t;

typedef union {
    struct {
        ca_uint32_t                 dump_time: 1;
        ca_uint32_t                 dump_en: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_frag_ff_dump_config_msk_t;

typedef struct {
    ca_uint32_t     dump_time;
    ca_uint32_t     dump_en;
} aal_xgpon_usbg_frag_ff_dump_config_t;

typedef union {
    struct {
        ca_uint32_t    dp_h: 1;
        ca_uint32_t    dp_l: 1;
        ca_uint32_t    pp_h: 1;
        ca_uint32_t    pp_l: 1;
        ca_uint32_t    pcnt: 1;
        ca_uint32_t    plen: 1;
        ca_uint32_t    dlen: 1;
        ca_uint32_t    oh_len: 1;
        ca_uint32_t    fecu: 1;
        ca_uint32_t    ver:  1;
        ca_uint32_t    vld:  1;
        ca_uint32_t    rsvd: 21;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_burst_prof_cfg_msk_t;


typedef struct {
    ca_uint32_t     dp_h;
    ca_uint32_t     dp_l;
    ca_uint32_t     pp_h;
    ca_uint32_t     pp_l;
    ca_uint32_t     pcnt;
    ca_uint32_t     plen;
    ca_uint32_t     dlen;
    ca_uint32_t     oh_len;
    ca_uint32_t     fecu;
    ca_uint32_t     ver;
    ca_uint32_t     vld;
} aal_xgpon_usbg_burst_prof_cfg_t;

typedef union {
    struct {
        ca_uint32_t                 ind0: 1;
        ca_uint32_t                 ind1: 1;
        ca_uint32_t                 ind2: 1;
        ca_uint32_t                 ind3: 1;
        ca_uint32_t                 rsvd: 28;
    } s;
    ca_uint32_t u32;
} aal_xgpon_usbg_bp_ver_ol_msk_t;

typedef struct {
    ca_uint32_t     ind0;
    ca_uint32_t     ind1;
    ca_uint32_t     ind2;
    ca_uint32_t     ind3;
} aal_xgpon_usbg_bp_ver_ol_t;

typedef struct {
    ca_uint32_t     frame_cnt;
    ca_uint64_t     byte_cnt;
} aal_xgpon_usbg_xgem_mib_config_t;

typedef union {
    struct {
        ca_uint32_t                 rx_fltr_en: 1;
        ca_uint32_t                 grant_src_mode: 1;
        ca_uint32_t                 st_out_chk_en: 1;
        ca_uint32_t                 pre_st_out_chk_en: 1;
        ca_uint32_t                 grt_always_drop_num: 1;
        ca_uint32_t                 grt_always_drop_en: 1;
        ca_uint32_t                 rsvd: 26;
    } s;
    ca_uint32_t u32;
} aal_xgpon_bwmp_glb_ctrl_msk_t;

typedef struct {
    ca_uint32_t     rx_fltr_en;
    ca_uint32_t     grant_src_mode;
    ca_uint32_t     st_out_chk_en;
    ca_uint32_t     pre_st_out_chk_en;
    ca_uint32_t     grt_always_drop_num;
    ca_uint32_t     grt_always_drop_en;
} aal_xgpon_bwmp_glb_ctrl_t;



typedef union {
    struct {
        ca_uint32_t                 all_onu_num_os: 1;
        ca_uint32_t                 alloc_peronu_num_os: 1;
        ca_uint32_t                 bst_series_num_os: 1;
        ca_uint32_t                 bst_sz_sum_os: 1;
        ca_uint32_t                 alloc_sz_chk_vld_os: 1;
        ca_uint32_t                 st_chk_fail: 1;
        ca_uint32_t                 rsvd: 26;
    } s;
    ca_uint32_t u32;
} aal_xgpon_bwmp_lmt_drop_src_cfg_msk_t;

typedef struct {
    ca_uint32_t     all_onu_num_os;
    ca_uint32_t     alloc_peronu_num_os;
    ca_uint32_t     bst_series_num_os;
    ca_uint32_t     bst_sz_sum_os;
    ca_uint32_t     alloc_sz_chk_vld_os;
    ca_uint32_t     st_chk_fail;
} aal_xgpon_bwmp_lmt_drop_src_cfg_t;

typedef struct {
    ca_uint8_t     wr_ptr;
    ca_uint8_t     wr_sel;
    ca_uint8_t     rd_ptr;
    ca_uint8_t     rd_sel;
} aal_xgpon_bwmp_bwmap_grt_ff_cfg_t;

typedef struct {
    ca_uint32_t     wr_ptr;
    ca_uint32_t     wr_sel;
    ca_uint32_t     rd_ptr;
    ca_uint32_t     rd_sel;
} aal_xgpon_bwmp_xgtc_grt_ff_cfg_t;


typedef struct {
    ca_uint32_t     wr_ptr;
    ca_uint32_t     rd_ptr;
} aal_xgpon_bwmp_bm_grt_ff_cfg_t;

typedef union {
    struct {
        ca_uint32_t                 grant_size: 1;
        ca_uint32_t                 st_tm: 1;
        ca_uint32_t                 bp_ind: 1;
        ca_uint32_t                 fecu: 1;
        ca_uint32_t                 ploamu: 1;
        ca_uint32_t                 dbru: 1;
        ca_uint32_t                 alloc_ind: 1;
        ca_uint32_t                 rsvd: 25;
    } s;
    ca_uint32_t u32;
} aal_xgpon_bwmp_grt_ff_cfg_msk_t;

typedef struct {
    ca_uint32_t     grant_size;
    ca_uint32_t     st_tm;
    ca_uint32_t     bp_ind;
    ca_uint32_t     fecu;
    ca_uint32_t     ploamu;
    ca_uint32_t     dbru;
    ca_uint32_t     tcont_id;
    ca_uint32_t     tcont_en;
} aal_xgpon_bwmp_grt_ff_cfg_t;


typedef union {
    struct {
        ca_uint32_t                 infor0: 1;
        ca_uint32_t                 infor1: 1;
        ca_uint32_t                 rsvd: 30;
    } s;
    ca_uint32_t u32;
} aal_xgpon_bwmp_grt_msg_ctrl_cfg_msk_t;

typedef struct {
    ca_uint32_t     infor0;
    ca_uint32_t     infor1;
} aal_xgpon_bwmp_grt_msg_ctrl_cfg_t;

typedef union {
    struct {
        ca_uint32_t                 frame_oversize_drop_en: 1;
        ca_uint32_t                 frame_runt_drop_en: 1;
        ca_uint32_t                 frame_length_mismatch_drop_en: 1;
        ca_uint32_t                 frame_fcs_error_drop_en: 1;
        ca_uint32_t                 frame_ffoverflow_drop_en: 1;
        ca_uint32_t                 fragment_drop_en: 1;
        ca_uint32_t                 omci_runt_drop_en: 1;
        ca_uint32_t                 drop_ptp_pkt_type: 1;
        ca_uint32_t                 ptp_da_en: 1;
        ca_uint32_t                 ptp_mode_sel: 1;
        ca_uint32_t                 ptp_ing_ts_cmp: 1;
        ca_uint32_t                 cripple_frame_drop_en: 1;
        ca_uint32_t                 omci_pad_en: 1;
        ca_uint32_t                 rsvd: 19;
    } s;
    ca_uint32_t u32;
} aal_xgpon_rxctl_rx_pkt_ctrl_cfg_msk_t;

typedef struct {
    ca_uint32_t     frame_oversize_drop_en;
    ca_uint32_t     frame_runt_drop_en;
    ca_uint32_t     frame_length_mismatch_drop_en;
    ca_uint32_t     frame_fcs_error_drop_en;
    ca_uint32_t     frame_ffoverflow_drop_en;
    ca_uint32_t     fragment_drop_en;
    ca_uint32_t     omci_runt_drop_en;
    ca_uint32_t     drop_ptp_pkt_type;
    ca_uint32_t     ptp_da_en;
    ca_uint32_t     ptp_mode_sel;
    ca_uint32_t     ptp_ing_ts_cmp;
    ca_uint32_t     cripple_frame_drop_en;
    ca_uint32_t     omci_pad_en;
} aal_xgpon_rxctl_rx_pkt_ctrl_cfg_t;

typedef union {
    struct {
        ca_uint32_t                 alloc_id: 1;
        ca_uint32_t                 omci_en: 1;
        ca_uint32_t                 ploam_en: 1;
        ca_uint32_t                 rsvd: 29;
    } s;
    ca_uint32_t u32;
} aal_xgpon_bwmp_alloc_id_cfg_msk_t;

typedef struct {
    ca_uint32_t     alloc_id;
    ca_uint32_t     omci_en;
    ca_uint32_t     ploam_en;
} aal_xgpon_bwmp_alloc_id_cfg_t;


typedef union {
    struct {
        ca_uint32_t                 gem_id: 1;
        ca_uint32_t                 gem_vld: 1;
        ca_uint32_t                 gem_mc: 1;
        ca_uint32_t                 rsvd: 29;
    } s;
    ca_uint32_t u32;
} aal_xgpon_dsfp_xgem_cfg_msk_t;


typedef struct {
    ca_uint32_t     gem_id;
    ca_uint32_t     gem_vld;
    ca_uint32_t     gem_mc;
} aal_xgpon_dsfp_xgem_cfg_t;

typedef struct {
    ca_uint32_t     ds_chanl;
    ca_uint32_t     us_chanl;
} aal_xgpon_chanl_cfg_t;

typedef struct {
    ca_uint32_t     xgtc_frame;
    ca_uint32_t     xgtc_err;
    ca_uint32_t     xgtc_hec_corrected;
    ca_uint32_t     xgtc_hec_uncorrected;
    ca_uint32_t     xgem_frame;
    ca_uint32_t     xgem_err0; /* hec error xgem_hd LSB  */
    ca_uint32_t     xgem_err1; /* hec error xgem_hd MSB */
    ca_uint32_t     xgem_hec_corrected;
    ca_uint32_t     xgem_hec_uncorrected;
    ca_uint32_t     xgem_nolf;
    ca_uint32_t     xgem_los_word;
    ca_uint32_t     xgem_err_key;
    ca_uint32_t     rxmac_xgem_frame;
    ca_uint32_t     rxmac_pkt_frame;
    ca_uint32_t     rxmac_pkt_crc_err;
    ca_uint32_t     bwmap_hec_corrected;
    ca_uint32_t     bwmap_hec_uncorrected;
    ca_uint32_t     psbd_hec_err;
    ca_uint32_t     psbd_hec_uncorrected;
    ca_uint32_t     ponid_hec_corrected;
    ca_uint32_t     ponid_hec_uncorrected;
    ca_uint32_t     sfc_hec_corrected;
    ca_uint32_t     sfc_hec_uncorrected;
    ca_uint32_t     bip_32_err;
    ca_uint32_t     bip_32_dw0;
    ca_uint32_t     bip_32_dw1;
} aal_xgpon_dsfp_stats_t;

typedef struct {
    ca_uint32_t  xgem_lf0_cnt;
    ca_uint32_t  xgtc_frm_cnt;
    ca_uint32_t  xgtc_fec_blk_cnt;
    ca_uint32_t  ploam_frm_cnt;
}aal_xgpon_usbg_stats_t;


typedef struct {
      ca_boolean_t gene_st      ; /*The subkey MSK,SK,IK regenerate again,HW clear it afterregenerating done */
      ca_boolean_t sk_sw_st     ; /*The subkey SK,IK start to regenerate again by SW write the MSK register value, HW clear it after regenerating done*/
      ca_boolean_t ik_sw_st     ; /*The subkey IK start to regenerate again by SW write the SK register value,HW clear it after regenerating done */
      ca_boolean_t mic_k12_sw_st; /*The PLOAM_IK K1,K2 start to regenerate again by SW write the IK register value,HW clear it after regenerating done */
} aal_xgpon_aes_plmp_subkey_ctrl_t;

ca_status_t aal_xgpon_dsfp_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_stats_t *stats
 );

ca_status_t aal_xgpon_tcont_bwmp_grant_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t grant_ff_sel,
    CA_IN ca_uint8_t tcont_id
);

ca_status_t aal_xgpon_aes_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
);

ca_status_t aal_xgpon_aes_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
);

ca_status_t aal_xgpon_aes_state_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_gpon_glb_ctrl_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_gpon_glb_ctrl_config_t *glb_ctrl_cfg);

ca_status_t aal_gpon_glb_ctrl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_gpon_glb_ctrl_config_msk_t mask,
    CA_IN aal_gpon_glb_ctrl_config_t *glb_ctrl_cfg);

ca_status_t aal_xgpon_aes_plmp_glb_ctrl_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      aal_xgpon_aes_plmp_gbl_ctrl_t *aes_plmp_gbl_ctrl);

ca_status_t aal_xgpon_aes_plmp_glb_ctrl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       aal_xgpon_aes_plmp_gbl_ctrl_msk_t   mask,
    CA_IN       aal_xgpon_aes_plmp_gbl_ctrl_t *aes_plmp_gbl_ctrl);

ca_status_t aal_xgpon_aes_plmp_msk_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      ca_uint8_t              plmp_msk[]);

ca_status_t aal_xgpon_aes_plmp_msk_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_msk[]);


ca_status_t aal_xgpon_aes_plmp_sk_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      ca_uint8_t              plmp_sk[]);

ca_status_t aal_xgpon_aes_plmp_sk_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_sk[]);

ca_status_t aal_xgpon_aes_plmp_mic_key_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_uint8_t              key_len,   
    CA_OUT      ca_uint8_t              plmp_mic_key[]);

ca_status_t aal_xgpon_aes_plmp_mic_key_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     key_len,
    CA_IN ca_uint8_t     plmp_mic_key[]);

ca_status_t aal_xgpon_aes_plmp_ik_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      ca_uint8_t              plmp_ik[]);

ca_status_t aal_xgpon_aes_plmp_ik_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_ik[]);

ca_status_t aal_xgpon_aes_uc_enc_key_get(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  aal_xgpon_enc_key_id_t   key_idx,
    CA_OUT aal_uc_enc_key_config_t *config);

ca_status_t aal_xgpon_aes_uc_enc_key_set(
    CA_IN ca_device_id_t              device_id,
    CA_IN aal_xgpon_enc_key_id_t      key_idx,
    CA_IN aal_uc_enc_key_config_msk_t mask,
    CA_IN aal_uc_enc_key_config_t    *config);

ca_status_t aal_xgpon_aes_mc_enc_key_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  aal_xgpon_enc_key_id_t       key_idx,
    CA_OUT aal_mc_enc_key_config_t *config);

ca_status_t aal_xgpon_aes_mc_enc_key_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_enc_key_id_t    key_idx,
    CA_IN aal_mc_enc_key_config_msk_t mask,
    CA_IN aal_mc_enc_key_config_t *config);

ca_status_t aal_xgpon_xgpn_glb_ver_id_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *revid);

ca_status_t aal_xgpon_xgpn_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_xgpn_glb_ctrl_config_t *glb_ctrl_cfg);

ca_status_t aal_xgpon_xgpn_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_xgpn_glb_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_xgpn_glb_ctrl_config_t *glb_ctrl_cfg);

ca_status_t aal_xgpon_xgpn_glb_dbg_ctrl_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t dbg_type,
    CA_OUT ca_uint64_t *val);

ca_status_t aal_xgpon_dsfp_hec_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_hec_ctrl_config_t *hec_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_hec_ctrl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_hec_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_hec_ctrl_config_t *hec_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_sync_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_sync_ctrl_config_t *sync_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_sync_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_sync_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_sync_ctrl_config_t *sync_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgtc_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgtc_ctrl_config_t *xgtc_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgtc_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgtc_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgtc_ctrl_config_t *xgtc_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_ctrl_config_t *xgem_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_ctrl_config_t *xgem_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_rxmac_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_rxmac_ctrl_config_t *rxmac_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_rxmac_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_rxmac_ctrl_config_msk_t mask,
    CA_IN  aal_xgpon_dsfp_rxmac_ctrl_config_t *rxmac_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_key_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_key_ctrl_config_t *key_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_key_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_key_ctrl_config_t *key_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_mc_key_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t *key_ctrl_cfg);

ca_status_t aal_xgpon_dsfp_xgem_mc_key_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_mc_key_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t *key_ctrl_cfg);


ca_status_t aal_xgpon_dsfp_xgem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_idx,
    CA_OUT aal_xgpon_dsfp_xgem_cfg_t *xgem_cfg );

ca_status_t aal_xgpon_dsfp_xgem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_idx,
    CA_IN aal_xgpon_dsfp_xgem_cfg_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_cfg_t *xgem_cfg);

ca_status_t aal_xgpon_dsfp_sync_state_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *state);

ca_status_t aal_xgpon_dsfp_super_frame_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *sfc0,
    CA_OUT ca_uint32_t *sfc1);

ca_status_t aal_xgpon_dsfp_oc_body_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_oc_body_config_t *oc_body);

ca_status_t aal_xgpon_dsfp_bip_err_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_dsfp_psbd_hec_err_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_dsfp_bip_dw0_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_dsfp_bip_dw1_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_dsfp_psbd_hec_uncorrect_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_xgtc_hec_correct_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_xgtc_hec_uncorrect_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_xgem_err0_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *hd);

ca_status_t aal_xgpon_dsfp_xgem_err1_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *hd);

ca_status_t aal_xgpon_dsfp_xgem_los_word_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_xgem_err_key_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_rxmac_omci_frame_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_dsfp_rxmac_xgem_frame_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *fg_xgem_err);

ca_status_t aal_xgpon_dsfp_async_ff_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *overflow_status);

ca_status_t aal_xgpon_dsfp_mib_ram_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *status);

ca_status_t aal_xgpon_dsfp_xgem_mib_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_dsfp_xgem_mib_config_t *xgem_cnt);

ca_status_t aal_xgpon_dsfp_xgem_mib_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_IN aal_xgpon_dsfp_xgem_mib_config_t *xgem_cnt);

ca_status_t aal_xgpon_usbg_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_glb_ctrl_config_t *usbg_glb_cfg);

ca_status_t aal_xgpon_usbg_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_glb_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_usbg_glb_ctrl_config_t *usbg_glb_cfg);

ca_status_t aal_xgpon_usbg_xgtc_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgtc_ctrl_config_t *xgtc_ctrl_cfg);

ca_status_t aal_xgpon_usbg_xgtc_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgtc_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_usbg_xgtc_ctrl_config_t *xgtc_ctrl_cfg);

ca_status_t aal_xgpon_usbg_guard_time_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_guard_time_config_t *guard_time_cfg);

ca_status_t aal_xgpon_usbg_guard_time_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_guard_time_config_msk_t mask,
    CA_IN aal_xgpon_usbg_guard_time_config_t *guard_time_cfg);

ca_status_t aal_xgpon_usbg_idl_frame_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_idl_frm_config_t *idl_frm_cfg);

ca_status_t aal_xgpon_usbg_idl_frame_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_idl_frm_config_msk_t mask,
    CA_IN aal_xgpon_usbg_idl_frm_config_t *idl_frm_cfg);

ca_status_t aal_xgpon_usbg_xgtc_hd_ind_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgtc_hd_ind_config_t *hd_ind_cfg);

ca_status_t aal_xgpon_usbg_xgtc_hd_ind_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgtc_hd_ind_config_msk_t mask,
    CA_OUT aal_xgpon_usbg_xgtc_hd_ind_config_t *hd_ind_cfg);

ca_status_t aal_xgpon_usbg_aes_uni_key_idx_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *active_key_idx);

ca_status_t aal_xgpon_usbg_aes_uni_key_idx_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  active_key_idx);

ca_status_t aal_xgpon_usbg_xgem_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_usbg_gem_config_t *config);

ca_status_t aal_xgpon_usbg_xgem_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_IN aal_xgpon_usbg_gem_config_msk_t mask,
    CA_IN aal_xgpon_usbg_gem_config_t *config);

ca_status_t aal_xgpon_usbg_xgem_buf_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgem_buf_config_t *xgem_buf_cfg);

ca_status_t aal_xgpon_usbg_xgem_buf_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgem_buf_config_msk_t mask,
    CA_IN aal_xgpon_usbg_xgem_buf_config_t *xgem_buf_cfg);

ca_status_t aal_xgpon_usbg_dbru_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *mode);

ca_status_t aal_xgpon_usbg_dbru_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  mode);

ca_status_t aal_xgpon_usbg_ploamu_queue_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_ploamu_qu_config_t *ploamu_qu_config);

ca_status_t aal_xgpon_usbg_ploamu_queue_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_ploamu_qu_config_msk_t mask,
    CA_IN aal_xgpon_usbg_ploamu_qu_config_t *ploamu_qu_config);

ca_status_t aal_xgpon_usbg_frag_ff_dump_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_frag_ff_dump_config_t *frag_ff_dump_config);

ca_status_t aal_xgpon_usbg_frag_ff_dump_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_frag_ff_dump_config_msk_t mask,
    CA_IN aal_xgpon_usbg_frag_ff_dump_config_t *frag_ff_dump_config);

ca_status_t aal_xgpon_usbg_xgtc_err_dump_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *dly_time);

ca_status_t aal_xgpon_usbg_xgtc_err_dump_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  dly_time);

ca_status_t aal_xgpon_usbg_dbru_rpt_max_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *val);

ca_status_t aal_xgpon_usbg_dbru_rpt_max_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  val);

ca_status_t aal_xgpon_usbg_dbru_bufocc_tbl_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t alloc_idx,
    CA_IN ca_uint32_t select,
    CA_OUT ca_uint32_t *buf_occ);

ca_status_t aal_xgpon_usbg_dbru_bufocc_tbl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t alloc_idx,
    CA_IN ca_uint32_t select,
    CA_IN ca_uint32_t buf_occ);

ca_status_t aal_xgpon_usbg_burst_prof_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    prof_idx,
    CA_IN aal_xgpon_usbg_burst_prof_cfg_msk_t mask,
    CA_IN aal_xgpon_usbg_burst_prof_cfg_t *prof_cfg);

ca_status_t aal_xgpon_usbg_burst_prof_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    prof_idx,
    CA_OUT aal_xgpon_usbg_burst_prof_cfg_t *prof_cfg);

ca_status_t aal_xgpon_usbg_bp_ver_ol_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_bp_ver_ol_t *ind);

ca_status_t aal_xgpon_usbg_mib_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *ram_clr_en);

ca_status_t aal_xgpon_usbg_mib_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  ram_clr_en);

ca_status_t aal_xgpon_usbg_xgem_mib_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_usbg_xgem_mib_config_t *xgem_cnt);

ca_status_t aal_xgpon_usbg_xgem_mib_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_IN aal_xgpon_usbg_xgem_mib_config_t *xgem_cnt);

ca_status_t aal_xgpon_usbg_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_stats_t *stats);

ca_status_t aal_xgpon_bwmp_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_glb_ctrl_t *bwmp_glb_cfg);

ca_status_t aal_xgpon_bwmp_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_bwmp_glb_ctrl_msk_t mask,
    CA_IN aal_xgpon_bwmp_glb_ctrl_t *bwmp_glb_cfg);

ca_status_t aal_xgpon_bwmp_tcont_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel,
    CA_IN ca_uint32_t tcont_idx,
    CA_OUT aal_xgpon_bwmp_alloc_id_cfg_t *alloc_id_cfg);

ca_status_t aal_xgpon_tconts_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel);

ca_status_t aal_xgpon_xgems_get(
    CA_IN ca_device_id_t device_id);

ca_status_t aal_xgpon_bwmp_tcont_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel,
    CA_IN ca_uint32_t tcont_idx,
    CA_IN aal_xgpon_bwmp_alloc_id_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_alloc_id_cfg_t *alloc_id_cfg);

ca_status_t aal_xgpon_bwmp_rsp_tm_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *time);

ca_status_t aal_xgpon_bwmp_rsp_tm_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  time);

ca_status_t aal_xgpon_bwmp_bst_mg_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *time);

ca_status_t aal_xgpon_bwmp_bst_mg_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  time);

ca_status_t aal_xgpon_bwmp_seq_tm_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *map_lead_tm);

ca_status_t aal_xgpon_bwmp_seq_tm_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  map_lead_tm);

ca_status_t aal_xgpon_bwmp_msg_ctl_cfg_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t idx,
    CA_OUT aal_xgpon_bwmp_grt_msg_ctrl_cfg_t *config);

ca_status_t aal_xgpon_bwmp_msg_ctl_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t idx,
    CA_IN aal_xgpon_bwmp_grt_msg_ctrl_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_grt_msg_ctrl_cfg_t *config);

ca_status_t aal_xgpon_bwmp_grt_ff_ctrl_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t ff_idx,
    CA_IN ca_uint32_t grant_ff_sel,
    CA_OUT aal_xgpon_bwmp_grt_ff_cfg_t *grf_ff_cfg);

ca_status_t aal_xgpon_bwmp_bm_buf_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *wr_threshold);

ca_status_t aal_xgpon_bwmp_bm_buf_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  wr_threshold);

ca_status_t aal_xgpon_bwmp_eqd_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_bwmp_eqd_sts_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  data);

ca_status_t aal_xgpon_bwmp_random_dly_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data);

ca_status_t aal_xgpon_bwmp_lmt_drop_src_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_lmt_drop_src_cfg_t *lmt_drp_src_config);

ca_status_t aal_xgpon_bwmp_lmt_drop_src_sts_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_bwmp_lmt_drop_src_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_lmt_drop_src_cfg_t *lmt_drp_src_config);

ca_status_t aal_xgpon_bwmp_bwmap_grant_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_bwmap_grt_ff_cfg_t *grt_ff_config);

ca_status_t aal_xgpon_bwmp_xgtc_grt_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_xgtc_grt_ff_cfg_t *xgtc_grt_ff_config);

ca_status_t aal_xgpon_bwmp_bm_grant_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_bm_grt_ff_cfg_t *bm_grt_ff_config);

ca_status_t aal_xgpon_bwmp_unk_bf_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_as_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_as_hec_err_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_cnt_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *cnt_ctl);

ca_status_t aal_xgpon_bwmp_cnt_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  cnt_ctl);

ca_status_t aal_xgpon_bwmp_ds_fm_sync_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_us_map_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_us_xgem_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_bwmp_us_xgtc_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_rxctl_rx_port_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *spid);

ca_status_t aal_xgpon_rxctl_rx_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t spid);

ca_status_t aal_xgpon_rxctl_rx_pkt_ctl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_rxctl_rx_pkt_ctrl_cfg_t  *rx_pkt_ctrl_cfg);

ca_status_t aal_xgpon_rxctl_rx_pkt_ctl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_rxctl_rx_pkt_ctrl_cfg_msk_t  mask,
    CA_IN aal_xgpon_rxctl_rx_pkt_ctrl_cfg_t  *rx_pkt_ctrl_cfg);

ca_status_t aal_xgpon_rxctl_ds_gem_drop_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *cnt);

ca_status_t aal_xgpon_rxctl_ds_gem_drop_cnt_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cnt);

ca_status_t aal_xgpon_rxctl_rc1_ff_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_rxctl_rc2_ff_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_xgpon_rxctl_usbg_dbg_sts_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *state);

ca_status_t aal_xgpon_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
);
ca_status_t aal_xgpon_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
);

ca_status_t aal_xgpon_cb_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index);

ca_status_t aal_xgpon_cb_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_partition_index);

ca_status_t aal_xgpon_sysprof_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t   device_id,
    CA_OUT ca_uint8_t  *channel_count);

ca_status_t aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  timeout_ms );

ca_status_t aal_xgpon_clk_seq_reset(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_xgpon_xfp_chanl_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t    device_id,
    CA_OUT ca_uint32_t  *ds_pon_id,
    CA_OUT ca_uint32_t  *us_pon_id);

ca_status_t aal_xgpon_xfp_chanl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  ds_pon_id,
    CA_IN ca_uint32_t  us_pon_id);

ca_status_t aal_xgpn_activation_fsm_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t onu_pon_mac_mode);

ca_status_t aal_xgpon_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pon_mac_mode);

ca_status_t aal_xgpon_xfp_cap2_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint8_t    index,
    CA_OUT ca_uint8_t  *dn_support,
    CA_OUT ca_uint8_t  *up_support,
    CA_OUT ca_uint8_t  *dwlch_calibration_record);


ca_status_t aal_xgpon_serdes_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t delay
);

ca_status_t aal_xgpon_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t reset_flag);

void aal_xgpon_datapath_reset(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_xgpon_tcont_flush(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint8_t   uwlch_id,
    CA_IN ca_uint8_t   tcont_id);

ca_status_t aal_xgpon_ups_mib_get(
    CA_IN  ca_device_id_t  device_id);

ca_status_t aal_xgpon_dns_mib_get(
    CA_IN  ca_device_id_t device_id);

ca_status_t aal_xgpon_omci_mib_get(
    CA_IN  ca_device_id_t device_id);

ca_status_t aal_xgpon_xgem_mibs_get(
    CA_IN ca_device_id_t device_id);

ca_status_t aal_gpon_dying_gasp_get
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_OUT ca_uint8          *pkt_buff,
    CA_OUT ca_int32          *pkt_len
);
ca_status_t aal_gpon_dying_gasp_set
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8          *pkt_buff,
    CA_IN ca_int32          pkt_len
);

void aal_gpon_dying_gasp_handler/*CODEGEN_IGNORE_TAG*/
(
    ca_int32 gpio_pin,
    ca_uint8 status
);

ca_status_t aal_xgpon_bwmp_tcont_clr(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id,
    ca_uint32_t tcont_start,
    ca_uint32_t tcont_end);

ca_status_t aal_xgpon_aes_plmp_subkey_ctrl_get(
    CA_IN       ca_device_id_t                   device_id,
    CA_OUT      aal_xgpon_aes_plmp_subkey_ctrl_t *ctrl
);

ca_status_t aal_xgpon_aes_plmp_subkey_ctrl_set(
    CA_IN      ca_device_id_t                   device_id,
    CA_IN      aal_xgpon_aes_plmp_subkey_ctrl_t *ctrl
);

ca_status_t  aal_xgpon_activation_state_get(
    CA_IN  ca_device_id_t device_id);

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_xgpon_backup_ponmac(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_xgpon_restore_ponmac(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

ca_status_t aal_xgpon_restore_tcont(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);
#endif


#endif
