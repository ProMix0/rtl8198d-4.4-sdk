#ifndef _AAL_UNI_MACSEC_H_
#define _AAL_UNI_MACSEC_H_

#define AAL_MACSEC_KEY_LEN   16

#define __AAL_UNI_MACSEC_TBL_AN_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_SC_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_SCB_MAX             0x1
#define __AAL_UNI_MACSEC_TBL_ES_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_CO_MAX              0x3F
#define __AAL_UNI_MACSEC_TBL_RLYEN_MAX           0x1
#define __AAL_UNI_MACSEC_TBL_SEC_MODE_MAX        0x3
#define __AAL_UNI_MACSEC_TBL_V_CONFIG_MAX        0x3
#define __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX    5000
#define __AAL_UNI_MACSEC_MAX_SAID_SUPPORT        32
#define __AAL_UNI_MACSEC_TX_RX_ADDR_OFFSET       0x00000140

typedef enum {
    AAL_CRYPTO_DROP_UNTAG_MODE_NO_DROP,
    AAL_CRYPTO_DROP_UNTAG_MODE_DROP_DEFINE_ETHERTYPE,
    AAL_CRYPTO_DROP_UNTAG_MODE_DROP_ALL_UNTAG,
    AAL_CRYPTO_DROP_UNTAG_MODE_RESERVED
}aal_uni_macsec_drop_untag_mode_t;

typedef enum {
    AAL_MACSEC_SECTAG_8B,
    AAL_MACSEC_SECTAG_16B
}aal_uni_macsec_sectag_type_t;
	
typedef enum {
	AAL_UNI_MACSEC_RX_DECRY_ENA_ONLY,
	AAL_UNI_MACSEC_TX_ENCRY_ENA_ONLY,
	AAL_UNI_MACSEC_RX_TX_BOTH_ENA
}aal_uni_macsec_enable_type_t;

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
} aal_uni_macsec_sadb_ctrl_t;

typedef union {
  struct {
    ca_uint32_t port_to_cpu          		:  1 ; /* bits 3:0 */
    ca_uint32_t mgmt_pt_to_fe_also   		:  1 ; /* bits 4:4 */
    ca_uint32_t txcrc_chk_en         		:  1 ; /* bits 5:5 */
    ca_uint32_t p4_rgmii_tx_clk_phase 		:  1 ; /* bits 7:6 */
    ca_uint32_t p4_rgmii_tx_data_order 		:  1 ; /* bits 8:8 */
    ca_uint32_t xgmii_rx_macsec_on   		:  1 ; /* bits 9:9 */
    ca_uint32_t xgmii_tx_macsec_on   		:  1 ; /* bits 10:10 */
    ca_uint32_t p6_txmac_in_sel      		:  1 ; /* bits 11:11 */
    ca_uint32_t p5_txmac_in_sel      		:  1 ; /* bits 12:12 */
    ca_uint32_t macsec_enc_in_sel    		:  1 ; /* bits 13:13 */
    ca_uint32_t macsec_dec_in_sel    		:  1 ; /* bits 14:14 */
    ca_uint32_t rxmib_mode           		:  1 ; /* bits 16:16 */
    ca_uint32_t txmib_mode           		:  1 ; /* bits 17:17 */
    ca_uint32_t eth_sch_rdy_pkt      		:  1 ; /* bits 18:18 */
    ca_uint32_t macsec_sectag_type   		:  1 ; /* bits 20:20 */
    ca_uint32_t p5_macsec_dec_out_sel 		:  1 ; /* bits 21:21 */
    ca_uint32_t macsec_encrypt_en    		:  1 ; /* bits 22:22 */
    ca_uint32_t p6_macsec_dec_out_sel 		:  1 ; /* bits 23:23 */
    ca_uint32_t cnt_op_mode          		:  1; /* bits 26:24 */
    ca_uint32_t saturn_p7_tx_crc32_add 		:  1 ; /* bits 27:27 */
    ca_uint32_t macsec_enc_fifo_threshold 	:  1 ; /* bits 29:28 */
    ca_uint32_t saturn_c10_enc_sel   		:  1 ; /* bits 31:31 */
	ca_uint32_t rsvd   						:  10 ; /* bits 31:31 */
  } s ;
  ca_uint32_t     u32 ;
} aal_uni_macsec_glb_staic_cfg_msk_t;

typedef struct {
    ca_uint32_t port_to_cpu            ; /* bits 3:0 */
    ca_uint32_t mgmt_pt_to_fe_also     ; /* bits 4:4 */
    ca_uint32_t txcrc_chk_en           ; /* bits 5:5 */
    ca_uint32_t p4_rgmii_tx_clk_phase  ; /* bits 7:6 */
    ca_uint32_t p4_rgmii_tx_data_order ; /* bits 8:8 */
    ca_uint32_t xgmii_rx_macsec_on     ; /* bits 9:9 */
    ca_uint32_t xgmii_tx_macsec_on     ; /* bits 10:10 */
    ca_uint32_t p6_txmac_in_sel        ; /* bits 11:11 */
    ca_uint32_t p5_txmac_in_sel        ; /* bits 12:12 */
    ca_uint32_t macsec_enc_in_sel      ; /* bits 13:13 */
    ca_uint32_t macsec_dec_in_sel      ; /* bits 14:14 */
    ca_uint32_t rxmib_mode             ; /* bits 16:16 */
    ca_uint32_t txmib_mode             ; /* bits 17:17 */
    ca_uint32_t eth_sch_rdy_pkt        ; /* bits 18:18 */
    ca_uint32_t macsec_sectag_type     ; /* bits 20:20 */
    ca_uint32_t p5_macsec_dec_out_sel  ; /* bits 21:21 */
    ca_uint32_t macsec_encrypt_en      ; /* bits 22:22 */
    ca_uint32_t p6_macsec_dec_out_sel  ; /* bits 23:23 */
    ca_uint32_t cnt_op_mode            ; /* bits 26:24 */
    ca_uint32_t saturn_p7_tx_crc32_add ; /* bits 27:27 */
    ca_uint32_t macsec_enc_fifo_threshold  ; /* bits 29:28 */
    ca_uint32_t saturn_c10_enc_sel     ; /* bits 31:31 */
}aal_uni_macsec_glb_staic_cfg_t;

typedef union {
    struct {
        ca_uint32_t            local_sa_reg0      : 1;
        ca_uint32_t            local_sa_reg1      : 1;
        ca_uint32_t            sel_local_sa_reg   : 1;
        ca_uint32_t 		   rsvd               : 29;
    } s ;
    ca_uint32_t     u32 ;
} aal_uni_macsec_sa_cfg_msk_t;

typedef struct {
	ca_uint32_t 		   local_sa_reg0    ;
	ca_uint16_t 		   local_sa_reg1    ;
	ca_uint8_t 		   	   sel_local_sa_reg ;
}aal_uni_macsec_sa_cfg_t;

typedef union {
    struct {
		ca_uint32_t  drop_8902_ena           :1;
	    ca_uint32_t  drop_usrdef21_ena       :1;
		ca_uint32_t  drop_usrdef20_ena       :1;
		ca_uint32_t  drop_usrdef11_ena       :1;
		ca_uint32_t  drop_usrdef10_ena       :1;
		ca_uint32_t  drop_usrdef_ena         :1;
		ca_uint32_t  drop_8021x_ena		     :1;
		ca_uint32_t  drop_mpcp_ena           :1;
		ca_uint32_t  drop_oam_ena            :1;

        ca_uint32_t  drop_usrdef             :1;
        ca_uint32_t  drop_usrdef10           :1;
        ca_uint32_t  drop_usrdef11           :1;
        ca_uint32_t  drop_usrdef20           :1;
        ca_uint32_t  drop_usrdef21           :1;

        ca_uint32_t  dis_ver0_macsec_err     :1;
		ca_uint32_t  debug_bus_sel           :1;
		ca_uint32_t  drop_untag_pkt_mode	 :1;
		ca_uint32_t  en_co_error		 	 :1;
		ca_uint32_t  soft_reset      		 :1;
		ca_uint32_t  cfg_mask_dec_err  		 :1;
		ca_uint32_t  cfg_dont_modify_crc32   :1;
		ca_uint32_t  dis_crc32_chk      	 :1;
		ca_uint32_t  dis_preamble_chk  		 :1;
		ca_uint32_t  dis_drop_macsec_err_ptk :1;
		ca_uint32_t  key_exchange_mode		 :1;
		ca_uint32_t  disable_crc8_chk      	 :1;
		ca_uint32_t	 validate_frames         :1;
    } s ;
    ca_uint32_t     u32 ;
} aal_uni_macsec_cfg_msk_t;

typedef struct {
	ca_boolean_t		   drop_8902_ena			;	
	ca_boolean_t		   drop_usrdef21_ena		;	
	ca_boolean_t		   drop_usrdef20_ena		;	
	ca_boolean_t		   drop_usrdef11_ena		;	
	ca_boolean_t		   drop_usrdef10_ena		;	
	ca_boolean_t		   drop_usrdef_ena			;	
	ca_boolean_t		   drop_8021x_ena 	;	
	ca_boolean_t		   drop_mpcp_ena			;	
	ca_boolean_t		   drop_oam_ena 			;	
														
	ca_uint16_t 		   drop_usrdef				;	
	ca_uint16_t 		   drop_usrdef10			;	
	ca_uint16_t 		   drop_usrdef11			;	
	ca_uint16_t 		   drop_usrdef20			;	
	ca_uint16_t 		   drop_usrdef21			;	
														
	ca_boolean_t		   dis_ver0_macsec_err		;	
	ca_uint8_t			   debug_bus_sel			;   
	ca_uint8_t			   drop_untag_pkt_mode		;	
	ca_boolean_t		   en_co_error				;	
	ca_boolean_t		   soft_reset				;   
	ca_boolean_t		   cfg_mask_dec_err 		;   
	ca_boolean_t		   cfg_dont_modify_crc32	;	
	ca_boolean_t		   dis_crc32_chk			;	
	ca_boolean_t		   dis_preamble_chk 		;   
	ca_boolean_t		   dis_drop_macsec_err_ptk	;	
	ca_boolean_t		   key_exchange_mode		;	
	ca_boolean_t		   disable_crc8_chk 		;   
	
	ca_uint8_t			   validate_frames          ;
														
}aal_uni_macsec_cfg_t;

typedef union {
    struct {
        ca_uint32_t sec_tag_mem_cmd   	:  1 ;
        ca_uint32_t sec_tag_icv_send_en	:  1 ;
        ca_uint32_t sci_llid_disable  	:  1 ;
		ca_uint32_t sci1                :  1;
        ca_uint32_t sci0                :  1;
        ca_uint32_t pn_threshold        :  1;
        ca_uint32_t replay_window       :  1;
        ca_uint32_t rsvd              	: 25;
    } s ;
    ca_uint32_t     u32 ;
} aal_uni_macsec_nandi_cfg_msk_t;


typedef struct {
    ca_uint32_t sec_tag_mem_cmd   	 ;
    ca_uint32_t sec_tag_icv_send_en	 ;
    ca_uint32_t sci_llid_disable  	 ;
	ca_uint8_t  sci1[4]              ;
    ca_uint8_t  sci0[4]              ;
    ca_uint32_t pn_threshold         ;
    ca_uint32_t replay_window        ;
}aal_uni_macsec_nandi_cfg_t;

typedef volatile union {
	struct {
		ca_uint32_t key				 : 32 ; /* bits 31:0 */
	} bf ;
	ca_uint32_t	  wrd ;
} aal_uni_macsec_key_cfg_t;

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
		ca_uint32_t	key0_valid        :  1 ;
		ca_uint32_t	hash0_valid       :  1 ;
		ca_uint32_t	key1_valid        :  1 ;
		ca_uint32_t	hash1_valid       :  1 ;
		ca_uint32_t	sc_valid          :  1 ;
		ca_uint32_t	pn_valid          :  1 ;
		ca_uint32_t	npn_valid         :  1 ;
		ca_uint32_t	an_valid          :  1 ;
		ca_uint32_t	scb_valid         :  1 ;
		ca_uint32_t	es_valid          :  1 ;
		ca_uint32_t	co_valid          :  1 ;
		ca_uint32_t	rlyen_valid         :  1 ;
		ca_uint32_t	sec_mode_valid      :  1 ;
		ca_uint32_t	v_config_valid      :  1 ;
		ca_uint32_t	rsvd      		  :  4 ;
    } s ;
    ca_uint32_t     u32 ;
} aal_uni_macsec_tbl_cfg_msk_t;


typedef struct {
    ca_uint8_t key0[16] ;
    ca_uint8_t key1[16] ;
    ca_uint8_t hash0[16];
    ca_uint8_t hash1[16];
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
	ca_uint8_t	key0_valid;
	ca_uint8_t	hash0_valid;
	ca_uint8_t	key1_valid;
	ca_uint8_t	hash1_valid;
	ca_uint8_t	sc_valid;
	ca_uint8_t	pn_valid;
	ca_uint8_t	npn_valid;
	ca_uint8_t	an_valid;
	ca_uint8_t	scb_valid;
	ca_uint8_t	es_valid;
	ca_uint8_t	co_valid;
	ca_uint8_t	rlyen_valid;
	ca_uint8_t	sec_mode_valid;
	ca_uint8_t	v_config_valid;
}aal_uni_macsec_tbl_cfg_t;

typedef struct {
    ca_llid_t       llid;
    ca_boolean_t    valid;
    ca_uint32_t     sa_id;
}aal_uni_macsec_said_t;

typedef struct {
    ca_uint32_t rx_pkt_is_not_valid;
    ca_uint32_t rx_pkt_number_less_than_lowest_pn;
    ca_uint64_t rx_oct_protected_unencrypted;
    ca_uint64_t rx_oct_protected_encrypted;
    ca_uint32_t rx_rplen_less_than_lowest_pn;
    ca_uint32_t rx_pkts_ok;
    ca_uint32_t rx_pkt_invalid_with_vf_check;
    ca_uint32_t rx_pkt_not_valid_with_vf_strict_c_set;
    ca_uint32_t rx_pkt_not_using_sa;
}aal_uni_macsec_rx_stats_t;

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
}aal_uni_macsec_rx_error_stats_t;

typedef struct {
    ca_uint32_t tx_pkt_protected;
    ca_uint32_t tx_pkt_encrypted;
    ca_uint64_t tx_oct_protected;
    ca_uint64_t tx_oct_encrypted;
}aal_uni_macsec_tx_stats_t;

typedef struct {
    ca_uint32_t tx_pkt_untagged;
    ca_uint32_t tx_pkt_too_long;
}aal_uni_macsec_tx_error_stats_t;

ca_status_t aal_uni_macsec_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    uni_port,
    CA_IN ca_uint32_t    sectag_type,
    CA_IN aal_uni_macsec_enable_type_t ena_type);

ca_status_t aal_uni_macsec_glb_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_glb_staic_cfg_t *cfg);

ca_status_t aal_uni_macsec_glb_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_glb_staic_cfg_msk_t msk,
	CA_IN aal_uni_macsec_glb_staic_cfg_t *cfg);

ca_status_t aal_uni_macsec_rx_sa_cfg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_uni_macsec_sa_cfg_msk_t msk,
    CA_IN aal_uni_macsec_sa_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_sa_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_sa_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_sa_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_sa_cfg_msk_t msk,
	CA_IN aal_uni_macsec_sa_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_sa_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_sa_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_cfg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_uni_macsec_cfg_msk_t msk,
    CA_IN aal_uni_macsec_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_cfg_get(
    CA_IN ca_device_id_t  device_id,
    CA_OUT aal_uni_macsec_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_cfg_msk_t msk,
	CA_IN aal_uni_macsec_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_nandi_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_nandi_cfg_msk_t msk,
	CA_IN aal_uni_macsec_nandi_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_nandi_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_nandi_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_nandi_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_nandi_cfg_msk_t msk,
	CA_IN aal_uni_macsec_nandi_cfg_t *cfg
);

ca_status_t aal_uni_macsec_tx_nandi_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_nandi_cfg_t *cfg
);

ca_status_t aal_uni_macsec_rx_nandi_cmdctrl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t rwn,
	CA_IN ca_uint8_t start_write);

ca_status_t aal_uni_macsec_rx_nandi_cmdctrl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t  rwn,
	CA_OUT ca_uint8_t *start_write);

ca_status_t aal_uni_macsec_tx_nandi_cmdctrl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t rwn,
	CA_IN ca_uint8_t start_write);

ca_status_t aal_uni_macsec_tx_nandi_cmdctrl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t  rwn,
	CA_OUT ca_uint8_t *start_write);

ca_status_t aal_uni_macsec_rx_sadb_tbl_set(
	CA_IN ca_device_id_t  				device_id,
	CA_IN ca_uint32_t 					said,
	CA_IN aal_uni_macsec_tbl_cfg_msk_t 	msk,
	CA_IN aal_uni_macsec_tbl_cfg_t 		*cfg);

ca_status_t aal_uni_macsec_rx_sadb_tbl_get(
	CA_IN ca_device_id_t  			device_id,
	CA_IN ca_uint32_t 				said,
	CA_OUT aal_uni_macsec_tbl_cfg_t *cfg);

ca_status_t aal_uni_macsec_tx_sadb_tbl_set(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_uint32_t 					said,
	CA_IN aal_uni_macsec_tbl_cfg_msk_t 	msk,
	CA_IN aal_uni_macsec_tbl_cfg_t 		*cfg);

ca_status_t aal_uni_macsec_tx_sadb_tbl_get(
	CA_IN ca_device_id_t  			device_id,
	CA_IN ca_uint32_t 				said,
	CA_OUT aal_uni_macsec_tbl_cfg_t *cfg);

ca_status_t aal_uni_macsec_rx_said_set(
    CA_IN ca_device_id_t			device_id,
    CA_IN ca_uint8_t                llid_idx,
    CA_IN aal_uni_macsec_said_t     *said
);

ca_status_t aal_uni_macsec_rx_said_get(
    CA_IN ca_device_id_t			device_id,
    CA_IN ca_uint8_t				llid_idx,
    CA_OUT aal_uni_macsec_said_t	*said
);

ca_status_t aal_uni_macsec_tx_said_set(
	CA_IN ca_device_id_t			device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_IN aal_uni_macsec_said_t 	*said
);

ca_status_t aal_uni_macsec_tx_said_get(
	CA_IN ca_device_id_t			device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_OUT aal_uni_macsec_said_t	*said
);

ca_status_t aal_uni_macsec_rx_stats_get(
    CA_IN ca_device_id_t				device_id,
    CA_IN ca_port_id_t					port_id,
    CA_IN  ca_uint8_t 					llid_idx,
    CA_OUT aal_uni_macsec_rx_stats_t	*stats
);

ca_status_t aal_uni_macsec_rx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t  port_id,
    CA_OUT aal_uni_macsec_rx_error_stats_t *stats
);

ca_status_t aal_uni_macsec_tx_stats_get(
    CA_IN ca_device_id_t  				device_id,
    CA_IN ca_port_id_t 					port_id,
    CA_IN  ca_uint8_t 					llid_idx,
    CA_OUT aal_uni_macsec_tx_stats_t 	*stats
);

ca_status_t aal_uni_macsec_tx_error_stats_get(
    CA_IN ca_device_id_t  					device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_uni_macsec_tx_error_stats_t *stats
);

#endif
