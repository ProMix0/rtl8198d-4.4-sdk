#if defined(CONFIG_CORTINA_BOARD_FPGA)
#ifndef __AAL_IDP_H__
#define __AAL_IDP_H__

#define AAL_IDP_DPID_NUM     64
#define AAL_IDP_PKT_LEN_MAX  2048
#define AAL_IDP_HDR_A_LEN    8

typedef struct {
    ca_uint32_t  to_rmt_cpu_pkt_cnt;  /* IDP to remote CPU packet count */
    ca_uint32_t  to_rmt_cpu_byte_cnt; /* IDP to remote CPU byte count */
    ca_uint32_t  to_xge_pkt_cnt;      /* IDP to XGE CPU packet count */
    ca_uint32_t  to_xge_byte_cnt;     /* IDP to XGE CPU byte count */
}aal_idp_dns_global_stats_t;

typedef struct {
    ca_uint32_t  pkt_cnt; /* packet count */
    ca_uint32_t  byte_cnt;/* byte count */
}aal_idp_dns_dpid_stats_t;

typedef struct {
    ca_mac_addr_t my_mac; /* IDP My MAC, if matched will send to CPU */
    ca_uint16_t   my_ethtype;/* IDP My Ethertype, if matched will send to CPU */
} aal_idp_dns_cfg_t;

typedef union {
    struct {
        ca_uint32_t my_mac      : 1;
        ca_uint32_t my_ethtype  : 1;
        ca_uint32_t rsvd        : 30;
    }s;
    ca_uint32_t u32_data;
} aal_idp_dns_cfg_mask_t;


typedef struct {
    ca_uint32_t  xge_drp_pkt_cnt;   /* IDP Drop from XGE packet count */
    ca_uint32_t  rgmii_drp_pkt_cnt; /* IDP Drop from RGMII packet count */
    ca_uint32_t  xge_pkt_cnt;   /* IDP from XGE packet count */
    ca_uint32_t  rgmii_pkt_cnt; /* IDP from RGMII packet count */
    ca_uint32_t  xge_voqbp_drp_pkt_cnt;  /* IDP Drop for XGE Voq backpressure */
    ca_uint32_t  rgmii_voqbp_drp_pkt_cnt; /* IDP Drop for RGMII Voq backpressure */
}aal_idp_ups_global_stats_t;



typedef struct {
    ca_mac_addr_t my_mac_da; /* My mac address. if MAC_DA equals, set header packet type to 00 */
    ca_mac_addr_t my_mac_sa; /* mac source address. if MAC_SA match, set header no drop bit */
    ca_uint16_t   pkt_ethtype; /* Packet type for upstream comparison.  If match, set header A RX_PKT_TYPE to 3 */
    ca_uint8_t    my_llid; /* LLID for myMAC packet */
} aal_idp_ups_cfg_t;


typedef union {
    struct {
        ca_uint32_t my_mac_da   : 1;
        ca_uint32_t my_mac_sa   : 1;
        ca_uint32_t pkt_ethtype : 1;
        ca_uint32_t my_llid     : 1;
        ca_uint32_t rsvd        : 28;
    }s;
    ca_uint32_t u32_data;
} aal_idp_ups_cfg_mask_t;


typedef struct {
    ca_uint16_t  pkt_size;
    ca_uint16_t  pol_id	 ;
    ca_uint8_t   pol_grp ;
    ca_uint8_t   pkt_type;
    ca_uint8_t   ldpid	 ;
    ca_uint8_t   cos	 ;
} aal_idp_pkt_hdr_t;

typedef struct {
#ifdef CS_BIG_ENDIAN
    ca_uint32_t  cpu_flg	: 1 ;
    ca_uint32_t  deep_q	    : 1 ;
    ca_uint32_t  pol_grp_id	: 3 ;
    ca_uint32_t  pol_id	    : 9 ;
    ca_uint32_t  pol_en	    : 2 ;
    ca_uint32_t  mark	    : 1 ;
    ca_uint32_t  mirror	    : 1 ;
    ca_uint32_t  no_drop	: 1 ;
    ca_uint32_t  pkt_type   : 2 ;
    ca_uint32_t  drop_code	: 3 ;
    ca_uint32_t  mcgid	    : 8 ;
    ca_uint32_t  hdr_type	: 2 ;
    ca_uint32_t  fe_bypass	: 1 ;
    ca_uint32_t  pkt_size	: 14;
    ca_uint32_t  lspid	    : 6 ;
    ca_uint32_t  ldpid	    : 6 ;
    ca_uint32_t  cos	    : 3 ;
#else
    ca_uint32_t  cos	    : 3 ;
    ca_uint32_t  ldpid	    : 6 ;
    ca_uint32_t  lspid	    : 6 ;
    ca_uint32_t  pkt_size	: 14;
    ca_uint32_t  fe_bypass	: 1 ;
    ca_uint32_t  hdr_type	: 2 ;
    ca_uint32_t  mcgid	    : 8 ;
    ca_uint32_t  drop_code	: 3 ;
    ca_uint32_t  pkt_type   : 2 ;
    ca_uint32_t  no_drop	: 1 ;
    ca_uint32_t  mirror	    : 1 ;
    ca_uint32_t  mark	    : 1 ;
    ca_uint32_t  pol_en	    : 2 ;
    ca_uint32_t  pol_id	    : 9 ;
    ca_uint32_t  pol_grp_id	: 3 ;
    ca_uint32_t  deep_q	    : 1 ;
    ca_uint32_t  cpu_flg	: 1 ;
#endif
} aal_idp_hdr_a_t;


#define AAL_IDP_UPS_PKT_NUM  16

typedef struct {
    ca_uint8_t  da[6];
    ca_uint8_t  sa[6];
    ca_uint16_t tpid;
    ca_uint16_t vlan;
    ca_uint16_t ethtype;
    ca_uint16_t pad_len;
    ca_uint8_t  pad[12];
} aal_idp_ups_pkt_t;

typedef union {
    struct {
        ca_uint32_t da       : 1;
        ca_uint32_t sa       : 1;
        ca_uint32_t tpid     : 1;
        ca_uint32_t vlan     : 1;
        ca_uint32_t ethtype  : 1;
        ca_uint32_t pad_len  : 1;
        ca_uint32_t pad      : 1;
        ca_uint32_t rsvd     : 25;
    }s;
    ca_uint32_t u32_data;
} aal_idp_ups_pkt_mask_t;

ca_status_t aal_idp_dns_dpid_stats_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 dpid,
    CA_OUT aal_idp_dns_dpid_stats_t  *stats);

ca_status_t aal_idp_dns_global_stats_get(
    CA_IN  ca_device_id_t                  device_id,
    CA_OUT aal_idp_dns_global_stats_t  *stats);

ca_status_t aal_idp_dns_stats_clear(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id);

ca_status_t aal_idp_dns_cfg_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_idp_dns_cfg_mask_t     cfg_mask,
    CA_IN aal_idp_dns_cfg_t         *cfg);

ca_status_t aal_idp_dns_cfg_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_idp_dns_cfg_t         *cfg);

ca_status_t aal_idp_dns_rgmii_path_ctrl_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_boolean_t snoop_ena
);

ca_status_t aal_idp_dns_rgmii_path_ctrl_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_OUT ca_boolean_t *snoop_ena
);

ca_status_t aal_idp_dns_rx_pkt_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t         device_id,
    CA_OUT ca_uint16_t        *pkt_len,
    CA_OUT ca_uint8_t         *pkt_buf /* [*pkt_len:2048] */
    );

ca_status_t aal_idp_ups_cfg_set(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  aal_idp_ups_cfg_mask_t     mask,
    CA_IN  aal_idp_ups_cfg_t         *cfg);

ca_status_t aal_idp_ups_cfg_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_idp_ups_cfg_t         *cfg);

ca_status_t aal_idp_ups_global_stats_get(
    CA_IN  ca_device_id_t                  device_id,
    CA_OUT aal_idp_ups_global_stats_t  *stats);

ca_status_t aal_idp_ups_local_cpu_pkt_tx(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_idp_hdr_a_t    *pkt_hdr,
    CA_IN ca_uint16_t         pkt_len,
    CA_IN ca_uint8_t         *pkt_buf /* [pkt_len:2048] */
    );

ca_status_t aal_idp_ups_xge_pkt_set(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 pkt_idx,
    CA_IN  aal_idp_ups_pkt_mask_t     mask,
    CA_IN  aal_idp_ups_pkt_t         *pkt);

ca_status_t aal_idp_ups_xge_pkt_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 pkt_idx,
    CA_OUT aal_idp_ups_pkt_t         *pkt);

ca_status_t aal_idp_ups_xge_pkt_generate(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_uint16_t         pkt_vld_bmp,
    CA_IN ca_uint16_t         pkt_gen_loop,
    CA_IN ca_uint8_t          pkt_count_per_type
    );

ca_status_t aal_idp_up_heada_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *pkt_len);

ca_status_t aal_idp_up_heada_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t pkt_len);

ca_status_t aal_idp_init(/*CODEGEN_IGNORE_TAG*/
	CA_IN  ca_device_id_t device_id,
	CA_IN ca_uint32_t pon_mac_mode);

ca_status_t aal_idp_reset(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id);

#endif /* __AAL_IDP_H__ */
#endif /* defined(CONFIG_ARCH_CORTINA_G3HGU) */
