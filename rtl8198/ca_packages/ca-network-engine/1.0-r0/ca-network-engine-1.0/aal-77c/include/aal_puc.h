#ifndef __AAL_PUC_H__
#define __AAL_PUC_H__

#include "aal_pon.h"
extern ca_uint32_t puc_voq_map_mode;
extern ca_uint32_t tcont_num_per_chip;
extern ca_uint32_t queue_num_per_tcont;

#define AAL_PUC_QUEUE_NUM    256
#define AAL_PUC_QUEUE_ID_MAX 255

typedef enum {
    AAL_PUC_VOQ_MODE_8Q = 0, /* Up to 8 CoS queue mode => VoQID[6:0] = {HdrA.LDPID[3:0], HdrA.CoS[2:0] */
    AAL_PUC_VOQ_MODE_4Q = 1, /* Up to 4 CoS queue mode => VoQID[6:0] = {HdrA.LDPID[4:0], HdrA.CoS[1:0] */
    AAL_PUC_VOQ_MODE_FLAT_MAP = 2, /* Flow mode or single LLID/TCONT mode => VoQID[6:0] = HdrA.FlowID[6:0] */
    AAL_PUC_VOQ_MODE_RSVD = 3 /* RSVD (unused at the moment) */

}aal_puc_voq_mode_t;

typedef union {
	struct {
		ca_uint32_t                 queue_id: 1;
		ca_uint8_t 				    queue_ena:1;
		ca_uint32_t                 queue_weight: 1;
		ca_uint32_t                 queue_pri: 1;
		ca_uint32_t                 rsvd: 28;
	} s;
	ca_uint32_t u32;
} aal_puc_queue_cfg_msk_t;


typedef struct {
	ca_uint32_t queue_id;
	ca_uint8_t  queue_ena;
	ca_uint32_t queue_weight;
	ca_uint32_t queue_pri;
} aal_puc_queue_cfg_t;

typedef struct {
	ca_uint32_t bytecnt;
	ca_uint32_t bufcnt;
} aal_puc_voq_byte_cnt_mem_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 basesel: 1;
		ca_uint32_t                 ratecomp: 1;
		ca_uint32_t                 dwrrunit: 1;
        ca_uint32_t                 pkgbschmode: 1;
		ca_uint32_t                 rsvd: 28;
	} s;
	ca_uint32_t u32;
} aal_puc_voq_sche_cfg_msk_t;


typedef struct {
	ca_uint32_t                 basesel;
	ca_uint32_t                 ratecomp;
	ca_uint32_t                 dwrrunit;
    ca_uint32_t                 pkgbschmode;
} aal_puc_voq_sche_cfg_t;


typedef union {
	struct {
		ca_uint32_t                 pkt_mode_class0_cnt: 1;
		ca_uint32_t                 pkt_mode_class1_cnt: 1;
        ca_uint32_t                 pm_mode              :  1 ; 
        ca_uint32_t                 rl_force_init_done   :  1 ; 
        ca_uint32_t                 rl_addtok_en         :  1 ; 
        ca_uint32_t                 rl_subtok_en         :  1 ; 
        ca_uint32_t                 rl_en                :  1 ; 
		ca_uint32_t                 force_init_done: 1;
		ca_uint32_t                 addtok_en: 1;
		ca_uint32_t                 subtok_en: 1;
		ca_uint32_t                 shp_en: 1;
        ca_uint32_t                 rlovhd: 1;
		ca_uint32_t                 shpovhd: 1;
        ca_uint32_t                 agrshpovhd: 1;
		ca_uint32_t                 rsvd: 18;
	} s;
	ca_uint32_t u32;
} aal_puc_ctrl_cfg_msk_t;


typedef struct {
	ca_uint32_t pkt_mode_class0_cnt ;
	ca_uint32_t pkt_mode_class1_cnt ;
    ca_uint32_t pm_mode             ; 
    ca_uint32_t rl_force_init_done  ; 
    ca_uint32_t rl_addtok_en        ; 
    ca_uint32_t rl_subtok_en        ; 
    ca_uint32_t rl_en               ; 
	ca_uint32_t force_init_done     ;
	ca_uint32_t addtok_en           ;
	ca_uint32_t subtok_en           ;
	ca_uint32_t shp_en              ;
    ca_uint32_t rlovhd              ;
    ca_uint32_t shpovhd             ;
    ca_uint32_t agrshpovhd          ;
} aal_puc_ctrl_cfg_t;


typedef union {
	struct {
		ca_uint32_t                 rate_k: 1;
		ca_uint32_t                 rate_m: 1;
		ca_uint32_t                 bs: 1;
		ca_uint32_t                 enb: 1;
		ca_uint32_t                 mode: 1;
		ca_uint32_t                 tbc: 1;
		ca_uint32_t                 pkt_mode_class_sel: 1;
		ca_uint32_t                 rsvd: 25;
	} s;
	ca_uint32_t u32;
} aal_puc_voq_tbc_mem_cfg_msk_t;


typedef struct {
	ca_uint32_t rate_k               ;
	ca_uint32_t rate_m               ;
	ca_uint32_t bs                   ;
	ca_uint32_t enb                  ;
	ca_uint32_t mode                 ;
	ca_int32_t tbc                  ;
	ca_uint32_t pkt_mode_class_sel   ;
} aal_puc_voq_tbc_mem_cfg_t;

typedef struct {
	ca_uint32_t pcnt               ;
	ca_uint64_t bcnt               ;
} aal_puc_voq_dcnt_mem_cfg_t;

typedef struct {
	ca_uint32_t rate_k               ;
	ca_uint32_t rate_m               ;
    ca_uint32_t cbs                  ;
	ca_uint32_t enb                  ;
} aal_puc_agr_shp_ctrl_cfg_t;

typedef struct {
	ca_uint32_t last_upd_cyc              ;
	ca_uint32_t cyc_cnt_lmt               ;
    ca_uint32_t upd_cnt_lmt               ;
	ca_uint32_t k_cnt_lmt                 ;
} aal_puc_tb_ctrl_cfg_t;

typedef struct {
	ca_uint32_t test_cfg0              ;
	ca_uint32_t test_cfg1               ;
    ca_uint32_t test_cfg2               ;
	ca_uint32_t test_cfg3                 ;
} aal_puc_sram_test_ctrl_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 pfovrhd: 1;
		ca_uint32_t                 schmode: 1;
		ca_uint32_t 				ethmode:1;
		ca_uint32_t                 wdaligned: 1;
		ca_uint32_t                 lrgfrmfragen: 1;
		ca_uint32_t                 pyld1gfecon: 1;
		ca_uint32_t                 schen: 1;
        ca_uint32_t                 sch2en: 1;
		ca_uint32_t                 minrmnwindowsz: 1;
		ca_uint32_t                 rsvd: 23;
	} s;
	ca_uint32_t u32;
} aal_puc_btc_cfg_msk_t;

#define AAL_PUC_BTC_SCHMODE_FRAGMENT 0
#define AAL_PUC_BTC_SCHMODE_PACKET   1

typedef struct {
	ca_uint32_t pfovrhd ;
	ca_uint32_t schmode ;
	ca_uint32_t ethmode ;
	ca_uint32_t wdaligned     ;
	ca_uint32_t lrgfrmfragen;
	ca_uint32_t pyld1gfecon           ;
	ca_uint32_t schen;
    ca_uint32_t sch2en;
	ca_uint32_t minrmnwindowsz ;
} aal_puc_btc_cfg_t;


typedef union {
	struct {
		ca_uint32_t                 numqsets: 1;
		ca_uint32_t                 rptpreamblovhd: 1;
		ca_uint32_t                 rptipgovhd: 1;
		ca_uint32_t                 rpt1gfecon: 1;
		ca_uint32_t                 firstqstot: 1;
		ca_uint32_t                 lastqstot: 1;
        ca_uint32_t                 pkgarpt : 1;
        ca_uint32_t                 aggrth : 1;
		ca_uint32_t                 rsvd: 24;
	} s;
	ca_uint32_t u32;
} aal_puc_rpt_cfg_msk_t;


typedef struct {
	ca_uint32_t numqsets ;
	ca_uint32_t rptpreamblovhd;
	ca_uint32_t rptipgovhd;
	ca_uint32_t rpt1gfecon;
	ca_uint32_t firstqstot;
	ca_uint32_t lastqstot;
    ca_uint32_t pkgarpt;
    ca_uint32_t aggrth;
} aal_puc_rpt_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 qs1th: 1;
		ca_uint32_t                 qs2th: 1;
		ca_uint32_t                 rsvd: 30;
	} s;
	ca_uint32_t u32;
} aal_puc_epon_que_thrsd12_cfg_msk_t;


typedef struct {
	ca_uint32_t qs1th ;
	ca_uint32_t qs2th ;
} aal_puc_epon_que_thrsd12_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 qs3th: 1;
		ca_uint32_t                 qs4th: 1;
		ca_uint32_t                 rsvd: 30;
	} s;
	ca_uint32_t u32;
} aal_puc_epon_que_thrsd34_cfg_msk_t;


typedef struct {
	ca_uint32_t qs3th ;
	ca_uint32_t qs4th ;
} aal_puc_epon_que_thrsd34_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 bytecnt: 1;
		ca_uint32_t                 pktcnt: 1;
		ca_uint32_t                 rsvd: 30;
	} s;
	ca_uint32_t u32;
} aal_puc_voq_total_que_len_cfg_msk_t;


typedef struct {
	ca_uint32_t bytecnt ;
	ca_uint32_t pktcnt ;
} aal_puc_voq_total_que_len_cfg_t;


typedef union {
	struct {
		ca_uint32_t                 bytecnt: 1;
		ca_uint32_t                 voqid: 1;
		ca_uint32_t                 gempid: 1;
		ca_uint32_t                 rsvd: 29;
	} s;
	ca_uint32_t u32;
} aal_puc_port_open_pkt_cfg_msk_t;


typedef struct {
	ca_uint32_t bytecnt ;
	ca_uint32_t voqid ;
	ca_uint32_t gempid ;
} aal_puc_port_open_pkt_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 crccntl: 1;
		ca_uint32_t                 sllidmode: 1;
		ca_uint32_t                 rsvd: 30;
	} s;
	ca_uint32_t u32;
} aal_puc_cfg_msk_t;


typedef struct {
	ca_uint32_t crccntl ;
	ca_uint32_t sllidmode ;
} aal_puc_cfg_t;

typedef struct {
	ca_uint32_t fbpacnt ;
	ca_uint32_t fbpbcnt ;
} aal_puc_bm_dbg_fb_cfg_t;

typedef struct {
	ca_uint32_t shp_fifo_overflow ;
	ca_uint32_t shp_fifo_empty    ;
    ca_uint32_t rl_fifo_overflow  ;
    ca_uint32_t rl_fifo_empty     ;
    ca_uint32_t rl_init_done      ;
    ca_uint32_t rl_dcnt_init_done ;
	ca_uint32_t shp_init_done     ;
} aal_puc_sts_cfg_t;

typedef struct {
	ca_uint32_t intf_fifo_depth ;
	ca_uint32_t intf_fifo_overflow ;
	ca_uint32_t intf_fifo_front_overflow ;
	ca_uint32_t intf_fifo_front_depth ;
	ca_uint32_t wqf_data_fifo_depth ;
	ca_uint32_t buf_link_fifo_depth ;
	ca_uint32_t wqf_cmd_fifo_depth ;
	ca_uint32_t rqf_cmd_fifo_depth ;
	ca_uint32_t bmc_pe ;
} aal_puc_bmc_fifo_sts_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 gemid_replacement: 1;
		ca_uint32_t                 cos_replacement: 1;
		ca_uint32_t                 tcont_replacement: 1;
		ca_uint32_t                 enable_data_pkt_replacement:1;
		ca_uint32_t                 enable_replacement: 1;
		ca_uint32_t                 rsvd: 27;
	} s;
	ca_uint32_t u32;
} aal_puc_us_omci_hdr_a_cfg_msk_t;
typedef struct {
    ca_uint32_t                 gemid_replacement;
    ca_uint32_t                 cos_replacement;
    ca_uint32_t                 tcont_replacement;
	ca_uint32_t                 enable_data_pkt_replacement;
    ca_uint32_t                 enable_replacement;
} aal_puc_us_omci_hdr_a_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 gemid_replacement: 1;
		ca_uint32_t                 cos_replacement: 1;
		ca_uint32_t                 tcont_replacement: 1;
		ca_uint32_t                 rsvd: 29;
	} s;
	ca_uint32_t u32;
} aal_puc_us_omci_hp_hdr_a_cfg_msk_t;

typedef struct {
    ca_uint32_t                 gemid_replacement;
    ca_uint32_t                 cos_replacement;
    ca_uint32_t                 tcont_replacement;
} aal_puc_us_omci_hp_hdr_a_cfg_t;

typedef union {
	struct {
		ca_uint32_t                 us_ploam_msg_gen_en: 1;
		ca_uint32_t                 ds_ploam_pkt_gen_en: 1;
		ca_uint32_t                 rsvd: 30;
	} s;
	ca_uint32_t u32;
} aal_puc_glb_plom_cfg_msk_t;
typedef struct {
	ca_uint32_t us_ploam_msg_gen_en ;
	ca_uint32_t ds_ploam_pkt_gen_en ;
} aal_puc_glb_plom_cfg_t;
typedef struct {
	ca_uint32_t link_layer ;
	ca_uint32_t mac_layer ;
} aal_puc_bmc_ctl_pkt_cnt_t;
typedef union {
	struct {
		ca_uint32_t                 pkgbrptceil: 1;
		ca_uint32_t                 pkgboptschen: 1;
		ca_uint32_t                 freebufnibplowth: 1;
		ca_uint32_t                 freebufnibpen: 1;
		ca_uint32_t                 rsvd: 28;
	} s;
	ca_uint32_t u32;
} aal_puc_pvt_cfg_msk_t;
typedef struct {
    ca_uint32_t                 pkgbrptceil     ;
    ca_uint32_t                 pkgboptschen     ;
    ca_uint32_t                 freebufnibplowth;
    ca_uint32_t                 freebufnibpen     ;
} aal_puc_pvt_cfg_t;
typedef union {
	struct {
		ca_uint32_t                 voqid: 1;
		ca_uint32_t 				tcontid:1;
    	ca_uint32_t 				openpktflushen:1;
		ca_uint32_t                 start: 1;
		ca_uint32_t                 rsvd: 28;
	} s;
	ca_uint32_t u32;
} aal_puc_voq_flush_cfg_msk_t;
typedef struct {
	ca_uint32_t voqid ;
	ca_uint32_t tcontid;
    ca_uint32_t openpktflushen;
	ca_uint32_t start ;
} aal_puc_voq_flush_cfg_t;
typedef union {
	struct {
		ca_uint32_t                 voqid: 1;
        ca_uint32_t                 invalid_voqdrop_enable: 1;
		ca_uint32_t                 enable: 1;
		ca_uint32_t                 rsvd: 29;
	} s;
	ca_uint32_t u32;
} aal_puc_voq_drop_cfg_msk_t;
typedef struct {
	ca_uint32_t voqid ;
    ca_uint32_t invalid_voqdrop_enable;
	ca_uint32_t enable ;
} aal_puc_voq_drop_cfg_t;
typedef struct {
	ca_uint32_t fbcnta ;
	ca_uint32_t fbphptra ;
} aal_puc_bm_dbg_fbpa_cnt_t;
typedef struct {
	ca_uint32_t fbcntb ;
	ca_uint32_t fbphptrb ;
} aal_puc_bm_dbg_fbpb_cnt_t;

typedef struct{
    ca_mac_addr_t da;
    ca_mac_addr_t sa;
}aal_puc_omci_mac_t;

typedef struct{
    ca_uint32_t zeroqth;
    ca_uint32_t rsrvd1;
    ca_uint32_t zeroqrpten;
}aal_puc_ntt_cfg_t;

typedef volatile union {
  struct {
    ca_uint32_t zeroqth              : 21 ;
    ca_uint32_t rsrvd1               : 10 ;
    ca_uint32_t zeroqrpten           :  1 ;
  } s ;
  ca_uint32_t     u32 ;
} aal_puc_ntt_cfg_msk_t;

typedef struct{
    ca_uint32_t rptadjpid;
    ca_uint32_t rptadjqnum;
    ca_uint32_t rptadjqsnum;
    ca_uint32_t rptgapcnten;
    ca_uint32_t rptadjbuffullen;
    ca_uint32_t rptadjunibursten;
}aal_puc_rpt_adj_cfg1_t;

typedef volatile union {
  struct {
    ca_uint32_t rptadjpid            :  5 ;
    ca_uint32_t rptadjqnum           :  3 ;
    ca_uint32_t rptadjqsnum          :  3 ;
    ca_uint32_t rptgapcnten          :  1 ;
    ca_uint32_t rptadjbuffullen      :  1 ;
    ca_uint32_t rptadjunibursten     :  1 ;
    ca_uint32_t rsvd                 :  18;
  } s ;
  ca_uint32_t     u32 ;
} aal_puc_rpt_adj_cfg1_msk_t;

ca_status_t aal_puc_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
);

ca_status_t aal_puc_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
);


ca_status_t aal_puc_port_voq_map_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_IN aal_puc_queue_cfg_msk_t mask,
    CA_IN aal_puc_queue_cfg_t *queue_cfg);

ca_status_t aal_puc_port_voqs_map_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx);

ca_status_t aal_puc_port_voq_map_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_OUT aal_puc_queue_cfg_t *queue_cfg);

ca_status_t aal_puc_voq_pkt_len_hp_mem_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_IN ca_uint32_t hptr);

ca_status_t aal_puc_voq_pkt_len_hp_mem_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT ca_uint32_t *hptr);

ca_status_t aal_puc_voq_stats_get(
        CA_IN ca_device_id_t device_id);

ca_status_t aal_puc_bcmem_for_all_voq_get(
	    CA_IN ca_device_id_t device_id,
	    CA_IN ca_uint32_t queue_start_index,
	    CA_IN ca_uint32_t queue_end_index);

ca_status_t aal_puc_voq_len_get(
        CA_IN ca_device_id_t device_id);

ca_status_t aal_puc_voq_byte_cnt_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT aal_puc_voq_byte_cnt_mem_cfg_t *byte_cnt_cfg);

ca_status_t aal_puc_tqmem_for_all_voq_get(
	    CA_IN ca_device_id_t device_id,
	    CA_IN ca_uint32_t queue_start_index,
	    CA_IN ca_uint32_t queue_end_index);

ca_status_t aal_puc_voq_total_queue_len_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT aal_puc_voq_total_que_len_cfg_t *cfg);

ca_status_t aal_puc_port_open_pkt_mem_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN aal_puc_port_open_pkt_cfg_msk_t mask,
    CA_IN aal_puc_port_open_pkt_cfg_t *cfg);

ca_status_t aal_puc_port_open_pkt_mem_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_OUT aal_puc_port_open_pkt_cfg_t *cfg);


ca_status_t aal_puc_tcont_rem_buffers_get(
    CA_IN ca_device_id_t device_id);

ca_status_t aal_puc_port_voq_sche_info_tbl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_IN ca_uint32_t tbc);

ca_status_t aal_puc_port_voq_sche_info_tbl_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_OUT ca_uint32_t *tbc);

ca_status_t aal_puc_voq_sche_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_sche_cfg_t *voq_sche_config);

ca_status_t aal_puc_voq_sche_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_sche_cfg_msk_t mask,
    CA_IN aal_puc_voq_sche_cfg_t *voq_sche_config);

ca_status_t aal_puc_pvtbl_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_OUT ca_boolean_t *val);

ca_status_t aal_puc_pvtbl_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_boolean_t val);

ca_status_t aal_puc_enable_voq(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t ena);

ca_status_t aal_puc_voq_map_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *mode);

ca_status_t aal_puc_voq_map_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t mode);

ca_status_t aal_puc_ctrl_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_ctrl_cfg_t *puc_ctrl_config);

ca_status_t aal_puc_ctrl_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_ctrl_cfg_msk_t mask,
    CA_IN aal_puc_ctrl_cfg_t *puc_ctrl_config);

ca_status_t aal_puc_voq_tbc_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_tbc_mem_cfg_msk_t mask,
    CA_IN aal_puc_voq_tbc_mem_cfg_t *cfg);

ca_status_t aal_puc_voq_tbc_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_tbc_mem_cfg_t *cfg);


ca_status_t aal_puc_btc_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_btc_cfg_t *puc_btc_config);

ca_status_t aal_puc_btc_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_btc_cfg_msk_t mask,
    CA_IN aal_puc_btc_cfg_t *puc_btc_config);

ca_status_t aal_puc_rpt_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_rpt_cfg_t *puc_rpt_config);


ca_status_t aal_puc_rpt_cfg_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_rpt_cfg_msk_t mask,
    CA_IN aal_puc_rpt_cfg_t *puc_rpt_config);

ca_status_t aal_puc_bm_stat_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *bm_stat);

ca_status_t aal_puc_bm_stat_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t bm_stat);

ca_status_t aal_puc_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_cfg_t *cfg);

ca_status_t aal_puc_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_cfg_msk_t mask,
    CA_IN aal_puc_cfg_t *cfg);

ca_status_t aal_puc_bm_dbg_fb_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fb_cfg_t *cfg);

ca_status_t aal_puc_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sts_cfg_t *cfg);

ca_status_t aal_puc_epon_queue_thrshld12_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_epon_que_thrsd12_cfg_t *cfg);


ca_status_t aal_puc_epon_queue_thrshld12_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_epon_que_thrsd12_cfg_msk_t mask,
    CA_IN aal_puc_epon_que_thrsd12_cfg_t *cfg);

ca_status_t aal_puc_epon_queue_thrshld34_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_epon_que_thrsd34_cfg_t *cfg);


ca_status_t aal_puc_epon_queue_thrshld34_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_epon_que_thrsd34_cfg_msk_t mask,
    CA_IN aal_puc_epon_que_thrsd34_cfg_t *cfg);

ca_status_t aal_puc_bmc_length_error_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);

ca_status_t aal_puc_bmc_fifo_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bmc_fifo_sts_cfg_t *cfg);

ca_status_t aal_puc_bmc_omci_pkt_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt);
ca_status_t aal_puc_glb_lnk_type_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *lnk_type);
ca_status_t aal_puc_glb_lnk_type_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t lnk_type);
ca_status_t aal_puc_glb_mac_type_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *mac_type);
ca_status_t aal_puc_glb_mac_type_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t mac_type);

ca_status_t aal_puc_glb_da_sa_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_omci_mac_t *da_sa);

ca_status_t aal_puc_glb_da_sa_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_omci_mac_t da_sa);

ca_status_t aal_puc_us_omci_hdr_a_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_us_omci_hdr_a_cfg_t *cfg);
ca_status_t aal_puc_us_omci_hdr_a_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_us_omci_hdr_a_cfg_msk_t mask,
    CA_IN aal_puc_us_omci_hdr_a_cfg_t *cfg);

ca_status_t aal_puc_us_omci_hp_hdr_a_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_puc_us_omci_hp_hdr_a_cfg_t *cfg);

ca_status_t aal_puc_us_omci_hp_hdr_a_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_puc_us_omci_hp_hdr_a_cfg_msk_t mask,
	CA_IN aal_puc_us_omci_hp_hdr_a_cfg_t *cfg);

ca_status_t aal_puc_glb_ploam_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_glb_plom_cfg_t *cfg);
ca_status_t aal_puc_glb_ploam_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_glb_plom_cfg_msk_t mask,
    CA_IN aal_puc_glb_plom_cfg_t *cfg);
ca_status_t aal_puc_ds_ploam_hdra_cfg1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_ds_ploam_hdra_cfg1_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t cfg);
ca_status_t aal_puc_ds_ploam_hdra_cfg0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_ds_ploam_hdra_cfg0_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t cfg);
ca_status_t aal_puc_bmc_ctl_pkt_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bmc_ctl_pkt_cnt_t *cfg);
ca_status_t aal_puc_bmc_pkt_hdr1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_bmc_pkt_hdr0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_bmc_mtu_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint16_t *cfg);
ca_status_t aal_puc_bmc_mtu_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t *cfg);
ca_status_t aal_puc_voq_valid_get(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint16_t   voq_id,
    CA_OUT ca_boolean_t *valid);
ca_status_t aal_puc_voq_valid_set(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint16_t   voq_id,
    CA_IN ca_boolean_t  valid);


ca_status_t aal_puc_rl_voq_tbc_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_tbc_mem_cfg_msk_t mask,
    CA_IN aal_puc_voq_tbc_mem_cfg_t *cfg);
 
ca_status_t aal_puc_rl_voq_tbc_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_tbc_mem_cfg_t *cfg);

ca_status_t aal_puc_rl_voq_dcnt_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_dcnt_mem_cfg_t *cfg);
  
ca_status_t aal_puc_rl_voq_dcnt_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_dcnt_mem_cfg_t *cfg);

ca_status_t aal_puc_agr_shp_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_agr_shp_ctrl_cfg_t *cfg);

ca_status_t aal_puc_agr_shp_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_agr_shp_ctrl_cfg_t *cfg);
        
ca_status_t aal_puc_agr_shp_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);

ca_status_t aal_puc_tb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_tb_ctrl_cfg_t *cfg);

ca_status_t aal_puc_tb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_tb_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_2_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_2_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_2p_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc1_2p_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2b_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2b_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2b_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc2b_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc3_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg);

ca_status_t aal_puc_sram_test_ctrl_puc3_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg);



ca_status_t aal_puc_pvt_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_pvt_cfg_t *cfg);
ca_status_t aal_puc_pvt_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_pvt_cfg_msk_t mask,
    CA_IN aal_puc_pvt_cfg_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_sel1500_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_sel1500_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_sel3116_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_sel3116_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_sel4732_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_sel4732_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_sel6348_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_sel6348_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_a_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_a_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_b_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_b_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_buf_lmt_c_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg);
ca_status_t aal_puc_voq_buf_lmt_c_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg);
ca_status_t aal_puc_voq_flush_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_flush_cfg_t *cfg);
ca_status_t aal_puc_voq_flush_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_flush_cfg_msk_t mask,
    CA_IN aal_puc_voq_flush_cfg_t *cfg);
ca_status_t aal_puc_voq_drop_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_drop_cfg_t *cfg);
ca_status_t aal_puc_voq_drop_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_drop_cfg_msk_t mask,
    CA_IN aal_puc_voq_drop_cfg_t *cfg);

ca_status_t aal_puc_voq_bpremap_get(
	CA_IN ca_device_id_t device_id);

ca_status_t aal_puc_voq_bp_remap_data_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t voq_id,
	CA_OUT ca_uint32_t *tqmvoqid);

ca_status_t aal_puc_voq_bp_remap_data_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t voq_id,
	CA_IN ca_uint32_t tqmvoqid);


ca_status_t aal_puc_bm_dbg_fbpa_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fbpa_cnt_t *cfg);
ca_status_t aal_puc_bm_dbg_fbpb_cnt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fbpb_cnt_t *cfg);

ca_status_t aal_puc_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint32_t pon_mac_mode);

ca_status_t aal_puc_voq_flush(/*CODEGEN_IGNORE_TAG*/void);

ca_status_t aal_puc_voq_flush_by_idx(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t puc_port,
    CA_IN ca_uint32_t voq_start,
    CA_IN ca_uint32_t voq_end);

ca_status_t aal_puc_ntt_cfg_set(
    CA_IN   ca_device_id_t device_id,
    CA_IN   aal_puc_ntt_cfg_msk_t puc_ntt_cfg_msk,
    CA_IN   aal_puc_ntt_cfg_t puc_ntt_cfg);

ca_status_t aal_puc_ntt_cfg_get(
    CA_IN   ca_device_id_t device_id,
    CA_OUT  aal_puc_ntt_cfg_t *puc_ntt_cfg);

ca_status_t aal_puc_rpt_adj_cfg1_set(
    CA_IN   ca_device_id_t device_id,
    CA_IN   aal_puc_rpt_adj_cfg1_msk_t  adj_cfg_msk,
    CA_IN   aal_puc_rpt_adj_cfg1_t  adj_cfg);

ca_status_t aal_puc_rpt_adj_cfg1_get(
    CA_IN   ca_device_id_t device_id,
    CA_OUT   aal_puc_rpt_adj_cfg1_t  *adj_cfg);

ca_status_t aal_puc_reset(ca_device_id_t  device_id);

typedef struct{
    ca_uint32_t pkt_hdr1;
    ca_uint32_t pkt_hdr0;
    ca_uint8_t  cpu_flag;
    ca_uint8_t  deep_q;
    ca_uint8_t  policy_group_id;
    ca_uint16_t policy_id;
    ca_uint8_t  policy_ena;
    ca_uint8_t  marked;
    ca_uint8_t  mirror;
    ca_uint8_t  no_drop;
    ca_uint8_t  rx_packet_type;
    ca_uint8_t  drop_code;
    ca_uint16_t mc_group_id;
    ca_uint8_t  header_type;
    ca_uint8_t  fe_bypass;
    ca_uint16_t packet_size;
    ca_uint8_t  logic_spid;
    ca_uint8_t  logic_dpid;
    ca_uint8_t  cos;
}aal_puc_heada_t;

ca_status_t aal_puc_bmc_pkt_hdr_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_heada_t *heada);

#endif

