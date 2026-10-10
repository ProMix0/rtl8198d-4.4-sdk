

#ifndef __AAL_EPON_CRYPTO_H__
#define __AAL_EPON_CRYPTO_H__

#include "ca_types.h"

#define AAL_EPON_CRYPTO_KEY_WORD_NUM        4
#define AAL_EPON_MACSEC_SAID_NUM_MAX               64

typedef enum{
    AAL_EPON_CRYPTO_DIRECTION_RX    =   0,
    AAL_EPON_CRYPTO_DIRECTION_TX,
    AAL_EPON_CRYPTO_DIRECTION_MAX,
}aal_epon_crypto_direction_t;

typedef enum{
    AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW    =   0,
    AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW,
    AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_MAX = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW
}aal_epon_crypto_key_exchange_mode_t;

typedef enum{
    AAL_EPON_CRYPTO_1G_MODE_CHURNING  =   0,
    AAL_EPON_CRYPTO_1G_MODE_AES_32,
    AAL_EPON_CRYPTO_1G_MODE_AES_48,
    AAL_EPON_CRYPTO_1G_MODE_AES_J,
    AAL_EPON_CRYPTO_1G_MODE_AES_DPOE,
    AAL_EPON_CRYPTO_1G_MODE_DISABLE,
    AAL_EPON_CRYPTO_1G_MODE_MAX     =   AAL_EPON_CRYPTO_1G_MODE_DISABLE
}aal_epon_crypto_1g_mode_t;

typedef enum{
    AAL_EPON_CRYPTO_XG_MODE_MACSEC  =   0,
    AAL_EPON_CRYPTO_XG_MODE_AES_DPOE,
    AAL_EPON_CRYPTO_XG_MODE_CHURNING,
    AAL_EPON_CRYPTO_XG_MODE_DISABLE,
    AAL_EPON_CRYPTO_XG_MODE_MAX     =   AAL_EPON_CRYPTO_XG_MODE_DISABLE
}aal_epon_crypto_xg_mode_t;

typedef union {
  struct {
#ifdef CA_BIG_ENDIAN
    ca_uint32_t rsrvd1                : 15 ;
    ca_uint32_t v_config              : 2 ; /* bits 16:15 */
    ca_uint32_t sc                    : 1 ; /* bits 14 */
    ca_uint32_t sec_mode              : 2 ; /* bits 13:12 */
    ca_uint32_t rlyen                 : 1 ; /* bits 11 */
    ca_uint32_t co                    : 7 ; /* bits 10:4 */
    ca_uint32_t es                    : 1 ; /* bits 3 */
    ca_uint32_t scb                   : 1 ; /* bits 2 */
    ca_uint32_t an                    : 2 ; /* bits 1:0 */
#else /* CA_LITTLE_ENDIAN */
    ca_uint32_t an                    : 2 ; /* bits 1:0 */
    ca_uint32_t scb                   : 1 ; /* bits 2 */
    ca_uint32_t es                    : 1 ; /* bits 3 */
    ca_uint32_t co                    : 7 ; /* bits 10:4 */
    ca_uint32_t rlyen                 : 1 ; /* bits 11 */
    ca_uint32_t sec_mode              : 2 ; /* bits 13:12 */
    ca_uint32_t sc                    : 1 ; /* bits 14 */
    ca_uint32_t v_config              : 2 ; /* bits 16:15 */
    ca_uint32_t rsrvd1                : 15 ;
#endif
  } bf ;
  ca_uint32_t     wrd;
} aal_epon_crypto_xg_sadb_ctrl_t;

typedef union {
    struct {
        ca_uint32_t tx_mode           			:1 ;
		ca_uint32_t tx_ivsa_sel 				:1 ;
		ca_uint32_t rx_mode           			:1 ;
		ca_uint32_t key_exchange_mode 			:1 ;
		ca_uint32_t chn_oam_flag      			:1 ;
		ca_uint32_t chn_oam_oui       			:1 ;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_cfg_msk_t;

/* Crypto configuration table */
typedef struct {
    ca_uint32_t    tx_mode;
	ca_uint32_t    tx_ivsa_sel;
    ca_uint32_t    rx_mode;
    aal_epon_crypto_key_exchange_mode_t    key_exchange_mode; /* 0-SW mode; 1-HW mode */
    ca_uint16_t    chn_oam_flag;  /* Indicate the CTC triple churning key exchange OAM's flag 16 bits */
    ca_uint32_t    chn_oam_oui;   /* Indicate the CTC triple churning key exchange OAM's OUI.Only used for the crypto mode is CTC triple churning mode and 24 bits */
}aal_epon_crypto_cfg_t;

typedef union {
    struct {
		ca_uint32_t dis_ver0_macsec_err     	:1 ;
		ca_uint32_t debug_bus_sel           	:1 ;
		ca_uint32_t drop_untag_pkt_mode			:1 ;
		ca_uint32_t en_co_error		 			:1 ;
		ca_uint32_t soft_reset      			:1 ;
		ca_uint32_t cfg_mask_dec_err  			:1 ;
		ca_uint32_t cfg_dont_modify_crc32   	:1 ;
		ca_uint32_t dis_crc32_chk      			:1 ;
		ca_uint32_t dis_preamble_chk  			:1 ;
		ca_uint32_t dis_drop_macsec_err_ptk 	:1 ;
		ca_uint32_t key_exchange_mode		 	:1 ;
		ca_uint32_t disable_crc8_chk      		:1 ;

		ca_uint32_t drop_8902_ena   			:1 ;
		ca_uint32_t drop_usrdef21_ena			:1 ;
		ca_uint32_t drop_usrdef20_ena			:1 ;
		ca_uint32_t drop_usrdef11_ena			:1 ;
		ca_uint32_t drop_usrdef10_ena			:1 ;
		ca_uint32_t drop_usrdef_ena 			:1 ;
		ca_uint32_t drop_8021x_ena  			:1 ;
		ca_uint32_t drop_mpcp_ena   			:1 ;
		ca_uint32_t drop_oam_ena    			:1 ;
		ca_uint32_t drop_usrdef   	     		:1 ;
		ca_uint32_t drop_usrdef10 		   		:1 ;
		ca_uint32_t drop_usrdef11		   		:1 ;
		ca_uint32_t drop_usrdef20		   		:1 ;
		ca_uint32_t drop_usrdef21		   		:1 ;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_coresec_glb_cfg_msk_t;

/* Crypto configuration table */
typedef struct {
	ca_uint32_t dis_ver0_macsec_err     ;
	ca_uint32_t debug_bus_sel           ;
	ca_uint32_t drop_untag_pkt_mode		;
	ca_uint32_t en_co_error		 		;
	ca_uint32_t soft_reset      		;
	ca_uint32_t cfg_mask_dec_err  		;
	ca_uint32_t cfg_dont_modify_crc32   ;
	ca_uint32_t dis_crc32_chk      		;
	ca_uint32_t dis_preamble_chk  		;
	ca_uint32_t dis_drop_macsec_err_ptk ;
	ca_uint32_t key_exchange_mode       ;
	ca_uint32_t disable_crc8_chk      	;

	ca_boolean_t drop_8902_ena			;
	ca_boolean_t drop_usrdef21_ena		;
	ca_boolean_t drop_usrdef20_ena		;
	ca_boolean_t drop_usrdef11_ena		;
	ca_boolean_t drop_usrdef10_ena		;
	ca_boolean_t drop_usrdef_ena		;
	ca_boolean_t drop_8021x_ena 		;
	ca_boolean_t drop_mpcp_ena			;
	ca_boolean_t drop_oam_ena			;
	ca_uint16_t  drop_usrdef			;
	ca_uint16_t  drop_usrdef10			;
	ca_uint16_t  drop_usrdef11			;
	ca_uint16_t  drop_usrdef20			;
	ca_uint16_t  drop_usrdef21			;
}aal_epon_coresec_glb_cfg_t;

typedef struct
{
    ca_llid_t   llid;
    ca_boolean_t  ena;
}aal_epon_crypto_llid_t;

typedef union {
    struct {
        ca_uint32_t tmr_ena     :  1 ;
        ca_uint32_t cycle_tmr   :  1 ;
        ca_uint32_t retry_tmr   :  1 ;
        ca_uint32_t base_tmr    :  1 ;
        ca_uint32_t pause_dly   :  1 ;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_key_tmr_cfg_msk_t; /* For both 1g and xg */

typedef struct {
    ca_boolean_t      tmr_ena;
    ca_uint32_t       cycle_tmr;    /* 17bits, unit is base_tmr */
    ca_uint32_t       retry_tmr;    /* 7bits, unit is base_tmr */
    ca_uint32_t       base_tmr;     /* 7bits, unit is 1ms */
    ca_uint32_t       pause_dly;    /* 12bits, unit is ms */
}aal_epon_crypto_key_tmr_cfg_t;  /* For both 1g and xg */

typedef union {
    struct {
        ca_uint32_t single_churn_ena  :  1 ;
        ca_uint32_t crc8_regen_ena    :  1 ;
        ca_uint32_t mpcp_pkt_blk      :  1 ;
        ca_uint32_t debug_index       :  1 ;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_ext_cfg_msk_t;

/* Crypto extended configuration table */
typedef struct {
    ca_boolean_t      single_churn_ena;       /* enable single churning enable for non china telecom mode. */
    ca_boolean_t      crc8_regen_ena;         /* enable/disalbe the crc8 generate, bypass NI CRC8 instead new CRC8. */
    ca_boolean_t      mpcp_pkt_blk;           /* New packet will not sent out when MPCP counter ffffff0 to 000000f */
    ca_uint32_t       debug_index;            /* 9bits. If debug mode is enabled, debug local counter shall recorded for specific LLID */
}aal_epon_crypto_ext_cfg_t;

typedef struct {
    ca_uint32_t key[AAL_EPON_CRYPTO_KEY_WORD_NUM];
}aal_epon_crypto_key_t;

typedef struct {
    ca_llid_t       llid;
    ca_boolean_t    valid;
    ca_uint32_t     sa_id;
}aal_epon_crypto_macsec_said_t;

typedef union {
    struct {
        ca_uint32_t key0              :  1 ; /* 128b */
        ca_uint32_t key1              :  1 ; /* 128b */
        ca_uint32_t hash0             :  1 ; /* 128b,MACSEC mode only */
        ca_uint32_t hash1             :  1 ; /* 128b,MACSEC mode only */
        ca_uint32_t pn                :  1 ; /* 32b,churning-{M3,M2,M1,M0}, MACSEC-Packet Number*/
        ca_uint32_t npn               :  1 ; /* 32b,MACSEC mode only */
        ca_uint32_t an                :  1 ; /* 2b,valid key index */
        ca_uint32_t sc                :  1 ; /* 1b,MACSEC mode only */
        ca_uint32_t scb               :  1 ; /* 1b,MACSEC mode only */
        ca_uint32_t es                :  1 ; /* 1b,MACSEC mode only */
        ca_uint32_t co                :  1 ; /* 7b,MACSEC mode only */
        ca_uint32_t rlyen             :  1 ; /* 1b,MACSEC mode only */
        ca_uint32_t sec_mode          :  1 ; /* 2b,MACSEC mode only */
        ca_uint32_t v_config          :  1 ; /* 2b,MACSEC mode only */
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_xg_tbl_msk_t;

typedef struct {
    ca_uint32_t key0[AAL_EPON_CRYPTO_KEY_WORD_NUM] ; /* 128b */
    ca_uint32_t key1[AAL_EPON_CRYPTO_KEY_WORD_NUM] ; /* 128b */
    ca_uint32_t hash0[AAL_EPON_CRYPTO_KEY_WORD_NUM]; /* 128b,MACSEC mode only */
    ca_uint32_t hash1[AAL_EPON_CRYPTO_KEY_WORD_NUM]; /* 128b,MACSEC mode only */
    ca_uint32_t pn      ;                       /* 32b,churning-{M3,M2,M1,M0}, MACSEC-Packet Number*/
    ca_uint32_t npn     ;                       /* 32b,MACSEC mode only */
    ca_uint8_t  an      ;                       /* 2b,valid key index */
    ca_uint8_t  sc      ;                       /* 1b,MACSEC mode only */
    ca_uint8_t  scb     ;                       /* 1b,MACSEC mode only */
    ca_uint8_t  es      ;                       /* 1b,MACSEC mode only */
    ca_uint8_t  co      ;                       /* 7b,MACSEC mode only */
    ca_uint8_t  rlyen   ;                       /* 1b,MACSEC mode only */
    ca_uint8_t  sec_mode;                       /* 2b,MACSEC mode only */
    ca_uint8_t  v_config;                       /* 2b,MACSEC mode only */
}aal_epon_crypto_xg_tbl_field_t;


typedef struct {
    ca_uint32_t rx_pkt_encrypted_crcerr_0;
    ca_uint32_t rx_pkt_encrypted_good_0;
    ca_uint32_t rx_pkt_encrypted_crcerr_1;
    ca_uint32_t rx_pkt_encrypted_good_1;
}aal_epon_crypto_1g_stats_t;

typedef struct {
    ca_uint32_t tx_pkt_unencrypted;
    ca_uint32_t tx_pkt_encrypted;
    ca_uint32_t tx_byte_unencrypted;
    ca_uint32_t tx_byte_encrypted;

    ca_uint32_t rx_pkt_unencrypted;
    ca_uint32_t rx_pkt_encrypted;
    ca_uint32_t rx_byte_unencrypted;
    ca_uint32_t rx_byte_encrypted;
}aal_epon_crypto_xg_stats_t;

typedef union {
    struct {
        ca_uint32_t            local_sa_reg0      : 1;
        ca_uint32_t            local_sa_reg1      : 1;
        ca_uint32_t            sel_local_sa_reg   : 1;
        ca_uint32_t 		   rsvd               : 29;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_macsec_sa_cfg_msk_t;

typedef struct {
	ca_uint32_t 		   local_sa_reg0    ;
	ca_uint16_t 		   local_sa_reg1    ;
	ca_uint8_t 		   	   sel_local_sa_reg ;
}aal_epon_crypto_macsec_sa_cfg_t;

typedef union {
    struct {
        ca_uint32_t	sci_llid_disable    : 1;
        ca_uint32_t	sec_tag_icv_send_en : 1;
        ca_uint32_t	sec_tag_mem_cmd   	: 1;
		ca_uint32_t sci1                : 1;
        ca_uint32_t sci0                : 1;
        ca_uint32_t pn_threshold        : 1;
        ca_uint32_t replay_window       : 1;
        ca_uint32_t rsvd              	: 25;
    } s ;
    ca_uint32_t     u32 ;
} aal_epon_crypto_nandi_cfg_msk_t;

typedef struct {
	ca_uint32_t sci_llid_disable    ;
	ca_uint16_t	sec_tag_icv_send_en    ;
	ca_uint8_t	sec_tag_mem_cmd ;
	ca_uint8_t  sci1[4]              ;
    ca_uint8_t  sci0[4]              ;
    ca_uint32_t pn_threshold         ;
    ca_uint32_t replay_window        ;
}aal_epon_crypto_nandi_cfg_t;

typedef union {
    struct {
        ca_uint32_t            pkt_type      	   :1;
        ca_uint32_t            mpcp_sec_en         :1;
        ca_uint32_t            oam_sec_en   	   :1;
		ca_uint32_t            ethernettype_sec_en :1;
        ca_uint32_t 		   rsvd                :28;
    } s ;
    ca_uint32_t     u32 ;
}aal_epon_txmac_pkt_type_ena_cfg_msk_t;

typedef struct {
	ca_uint16_t 	   pkt_type    			;
	ca_boolean_t 	   mpcp_sec_en    		;
	ca_boolean_t 	   oam_sec_en    		;
	ca_boolean_t 	   ethernettype_sec_en  ;
}aal_epon_txmac_pkt_type_ena_cfg_t;

typedef struct {
    ca_uint32_t rx_pkt_is_invalid;
    ca_uint32_t rx_pkt_number_less_than_lowest_pn;
    ca_uint64_t rx_oct_protected_unencrypted;
    ca_uint64_t rx_oct_protected_encrypted;
    ca_uint32_t rx_rplen_less_than_lowest_pn;
    ca_uint32_t rx_pkts_ok;
    ca_uint32_t rx_pkt_invalid;
    ca_uint32_t rx_pkt_not_valid;
    ca_uint32_t rx_pkt_not_using_sa;
    ca_uint32_t rx_pkt_unused_sa;
}aal_epon_crypto_macsec_rx_stats_t;

typedef struct {
    ca_uint32_t rx_pkt_unused_sa;
	ca_uint32_t rx_pkt_untagged;
    ca_uint32_t rx_pkt_no_tag;
    ca_uint32_t rx_pkt_bad_tag;
    ca_uint32_t rx_pkt_unknow_sci;
    ca_uint32_t rx_pkt_no_sci;
    ca_uint32_t rx_pkt_overrun;
    ca_uint32_t rx_pkt_dec_invalid_co_err;
    ca_uint32_t rx_pkt_dec_ver_num_err;
    ca_uint32_t rx_pkt_dec_sl_msb_err;
    ca_uint32_t rx_pkt_dec_auth_fail_err;
    ca_uint32_t rx_pkt_unknown_llid;
    ca_uint32_t rx_pkt_undersize;
}aal_epon_crypto_macsec_rx_error_stats_t;

typedef struct {
    ca_uint32_t tx_pkt_protected;
    ca_uint32_t tx_pkt_encrypted;
    ca_uint64_t tx_oct_protected;
    ca_uint64_t tx_oct_encrypted;
}aal_epon_crypto_macsec_tx_stats_t;

typedef struct {
	ca_uint32_t tx_pkt_untagged;
    ca_uint32_t tx_pkt_too_long;
}aal_epon_crypto_macsec_tx_error_stats_t;

typedef struct {
	ca_uint32_t 	   glb_xge_bypass_cfg_old   ;
	ca_uint32_t 	   glb_xge_bypass_cfg2_old	;
	ca_uint32_t 	   pvtbl_data0_old    		;
	ca_uint32_t 	   pvtbl_data1_old  ;
    ca_uint32_t 	   pvtbl_data2_old   ;
	ca_uint32_t 	   pvtbl_data3_old	;
	ca_uint32_t 	   pvtbl_data4_old    		;
	ca_uint32_t 	   pdc_mem_map_data0_old  ;
    ca_uint32_t 	   pdc_mem_map_data1_old  ;
}aal_epon_cryp_macsec_misc_cfg_t;

/* Function export_ided declarations */
ca_status_t aal_epon_crypto_macsec_reset_pn_interval_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint32_t                   interval);

ca_status_t aal_epon_crypto_macsec_reset_pn_interval_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT ca_uint32_t                  *interval);

ca_status_t aal_epon_crypto_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    level);

ca_status_t aal_epon_crypto_debug_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *level);

ca_status_t aal_epon_crypto_1g_cfg_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN aal_epon_crypto_cfg_msk_t msk,
    CA_IN aal_epon_crypto_cfg_t *cfg);

ca_status_t aal_epon_crypto_1g_cfg_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT aal_epon_crypto_cfg_t *cfg);

ca_status_t aal_epon_crypto_1g_llid_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        said,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_llid_t    *llid
);

ca_status_t aal_epon_crypto_1g_llid_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        said,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_llid_t  *llid
);

ca_status_t aal_epon_crypto_1g_llid_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32                     llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable);

ca_status_t aal_epon_crypto_1g_llid_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable);

ca_status_t aal_epon_crypto_1g_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable);

ca_status_t aal_epon_crypto_1g_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable);

ca_status_t aal_epon_crypto_1g_key_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint8_t                    llid_idx,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_uint32_t                   key_id,
    CA_IN aal_epon_crypto_key_t         *key_data);

ca_status_t aal_epon_crypto_1g_key_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint8_t                    llid_idx,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_uint32_t                   key_id,
    CA_OUT aal_epon_crypto_key_t         *key_data);

ca_status_t aal_epon_crypto_1g_stats_get(
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_uint8_t            llid_idx,
    CA_OUT aal_epon_crypto_1g_stats_t *stats);

ca_status_t aal_epon_crypto_xg_mode_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_port_id_t                port_id,
    CA_OUT aal_epon_crypto_xg_mode_t  *tx_mode,
    CA_OUT aal_epon_crypto_xg_mode_t  *rx_mode);

ca_status_t aal_epon_crypto_xg_cfg_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN aal_epon_crypto_cfg_msk_t msk,
    CA_IN aal_epon_crypto_cfg_t *cfg);

ca_status_t aal_epon_crypto_xg_cfg_get(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT aal_epon_crypto_cfg_t *cfg);

ca_status_t aal_epon_crypto_xg_llid_set
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_llid_t   *llid
);

ca_status_t aal_epon_crypto_xg_llid_get
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_llid_t  *llid
);

ca_status_t aal_epon_crypto_coresec_glb_cfg_set(
	CA_IN ca_device_id_t	   device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_IN aal_epon_coresec_glb_cfg_msk_t msk,
	CA_IN aal_epon_coresec_glb_cfg_t *cfg);

ca_status_t aal_epon_crypto_coresec_glb_cfg_get(
	CA_IN ca_device_id_t	   device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_coresec_glb_cfg_t *cfg);

ca_status_t aal_epon_crypto_macsec_enable_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid,
    CA_IN ca_boolean_t      enable);

ca_status_t aal_epon_crypto_macsec_enable_get(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid,
    CA_OUT ca_boolean_t     *enable);

ca_status_t aal_epon_crypto_macsec_sectag_type_set(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid,
	CA_IN ca_uint8_t		type);

ca_status_t aal_epon_crypto_macsec_sectag_type_get(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid,
	CA_OUT ca_uint8_t		*type);

ca_status_t aal_epon_crypto_macsec_said_set(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid_idx,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_IN aal_epon_crypto_macsec_said_t *said);

ca_status_t aal_epon_crypto_macsec_said_get(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid_idx,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_crypto_macsec_said_t  *said);

ca_status_t aal_epon_crypto_xg_sa_tbl_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t                        said,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_xg_tbl_msk_t msk,
    CA_IN aal_epon_crypto_xg_tbl_field_t *cfg);

ca_status_t aal_epon_crypto_xg_sa_tbl_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t                        said,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_xg_tbl_field_t *cfg);

ca_status_t aal_epon_crypto_xg_stats_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_OUT aal_epon_crypto_xg_stats_t *stats);

ca_status_t aal_epon_crypto_xg_llid_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable);

    
ca_status_t aal_epon_crypto_xg_llid_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32                     llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable);

ca_status_t aal_epon_crypto_xg_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable);

ca_status_t aal_epon_crypto_xg_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable);

ca_status_t aal_epon_crypto_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
);

ca_status_t aal_epon_crypto_macsec_local_sa_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_macsec_sa_cfg_msk_t msk,
    CA_IN aal_epon_crypto_macsec_sa_cfg_t *cfg);
ca_status_t aal_epon_crypto_macsec_local_sa_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_crypto_macsec_sa_cfg_t *cfg);

ca_status_t aal_epon_txmac_pkt_type_enable_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_epon_txmac_pkt_type_ena_cfg_msk_t msk,
	CA_IN aal_epon_txmac_pkt_type_ena_cfg_t *cfg);
ca_status_t aal_epon_txmac_pkt_type_enable_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_epon_txmac_pkt_type_ena_cfg_t *cfg);

ca_status_t aal_epon_crypto_nandi_cfg_set(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_IN aal_epon_crypto_nandi_cfg_msk_t msk,
	CA_IN aal_epon_crypto_nandi_cfg_t *cfg);

ca_status_t aal_epon_crypto_nandi_cfg_get(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_crypto_nandi_cfg_t	*cfg);

ca_status_t aal_epon_crypto_macsec_rx_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN  ca_uint8_t said,
    CA_OUT aal_epon_crypto_macsec_rx_stats_t *stats);

ca_status_t aal_epon_crypto_macsec_rx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_crypto_macsec_rx_error_stats_t *stats);

ca_status_t aal_epon_crypto_macsec_tx_stats_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_port_id_t port_id,
	CA_IN  ca_uint8_t said,
	CA_OUT aal_epon_crypto_macsec_tx_stats_t *stats);

ca_status_t aal_epon_crypto_macsec_tx_error_stats_get(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT aal_epon_crypto_macsec_tx_error_stats_t *stats);

ca_status_t aal_epon_crypto_macsec_misc_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id);
    
ca_status_t aal_epon_crypto_macsec_misc_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id);

ca_status_t aal_epon_crypto_macsec_misc_recover(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_epon_crypto_direction_t direction);

#endif
