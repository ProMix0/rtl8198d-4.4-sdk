#ifndef __AAL_PDC_H__
#define __AAL_PDC_H__

#include "ca_types.h"

#define  AAL_PDC_MAP_ENTRY_NUM  256

typedef volatile union {
  struct {
    ca_uint32_t saturate             :  1 ; /* bits 0 */
    ca_uint32_t clear                :  2 ; /* bits 2:1 */
    ca_uint32_t rsrvd1               : 29 ;
  } bf ;
  ca_uint32_t     wrd ;
} AAL_REG_PDC_CNTR_CFG_t;

typedef union {
  struct {
    ca_uint32_t saturate             :  1 ;
    ca_uint32_t clear                :  1 ;
    ca_uint32_t rsrvd1               : 30 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_pdc_cntr_cfg_mask_t;

typedef struct {
    ca_boolean_t saturate;
    ca_uint8_t clear;
} aal_pdc_cntr_cfg_t;


typedef struct {
    ca_uint32_t depth;
    ca_uint32_t over_flow;
} aal_pdc_fifo_status_t;


typedef struct {
    ca_uint32_t good_pkt_cnt;
    ca_uint32_t error_pkt_cnt;
} aal_pdc_stats_t;

typedef union {
    struct {
        ca_uint32_t nocrc_insert           :  1 ;
        ca_uint32_t pdc_map_mem_en         :  1 ;
        ca_uint32_t epon_insert_16byte_hdr :  1 ;
#ifdef CONFIG_LUNA_G3_SERIES
        ca_uint32_t omci_hp_cos            :  1 ;
        ca_uint32_t omci_hp_ldpid          :  1 ;
        ca_uint32_t omci_hp_en             :  1 ;
        ca_uint32_t rsrvd1                 : 26;
#else
        ca_uint32_t rsrvd1                 : 29;
#endif
    } bf ;
    ca_uint32_t     wrd ;
} aal_pdc_ctrl_mask_t;


typedef struct {
    ca_uint32_t nocrc_insert           ;
    ca_uint32_t pdc_map_mem_en         ;
    ca_uint32_t epon_insert_16byte_hdr ;
#ifdef CONFIG_LUNA_G3_SERIES
    ca_uint32_t omci_hp_cos;
    ca_uint32_t omci_hp_ldpid;
    ca_uint32_t omci_hp_en;
#endif
} aal_pdc_ctrl_t;

typedef union {
  struct {
    ca_uint32_t cos                  :  1 ;
    ca_uint32_t ldpid                :  1 ;
    ca_uint32_t lspid                :  1 ;
    ca_uint32_t fe_bypass            :  1 ;
    ca_uint32_t hdr_type             :  1 ;
    ca_uint32_t mcgid                :  1 ;
    ca_uint32_t drop_code            :  1 ;
    ca_uint32_t rx_pkt_type          :  1 ;
    ca_uint32_t no_drop              :  1 ;

    ca_uint32_t mirror               :  1 ;
    ca_uint32_t mark                 :  1 ;
    ca_uint32_t pol_en               :  1 ;
    ca_uint32_t pol_id               :  1 ;
    ca_uint32_t pol_grp_id           :  1 ;
    ca_uint32_t deepq                :  1 ;
    ca_uint32_t cpu_flag             :  1 ;
    ca_uint32_t rsrvd1               : 16 ;
  } bf ;
  ca_uint32_t     wrd ;
} aal_pdc_pdc_map_mem_data_mask_t;

typedef struct {
    ca_uint8_t   cos;
    ca_uint16_t  ldpid;
    ca_uint16_t  lspid;
    ca_boolean_t fe_bypass;
    ca_uint8_t   hdr_type;
    ca_uint8_t   mcgid;
    ca_boolean_t drop_code;
    ca_uint8_t   rx_pkt_type;
    ca_boolean_t no_drop;

    ca_uint32_t mirror             ;
    ca_uint32_t mark               ;
    ca_uint32_t pol_en             ;
    ca_uint32_t pol_id             ;
    ca_uint32_t pol_grp_id         ;
    ca_uint32_t deepq              ;
    ca_uint32_t cpu_flag           ;
} aal_pdc_pdc_map_mem_data_t;



/*
 *
 * prototype
 *
 */

ca_status_t aal_pdc_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t     device_id,
    ca_uint32_t pon_mac_mode
);

ca_status_t aal_pdc_cnt_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_pdc_cntr_cfg_mask_t  mask,
    CA_IN aal_pdc_cntr_cfg_t       *data
);

ca_status_t aal_pdc_cnt_cfg_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_cntr_cfg_t       *data
);

ca_status_t aal_pdc_stats_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_stats_t    *data
);

ca_status_t aal_pdc_fifo_status_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_fifo_status_t       *data
);

ca_status_t aal_pdc_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_pdc_ctrl_mask_t  mask,
    CA_IN aal_pdc_ctrl_t       *data
);

ca_status_t aal_pdc_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_ctrl_t       *data
);

ca_status_t aal_pdc_map_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_IN aal_pdc_pdc_map_mem_data_mask_t mask,
    CA_IN aal_pdc_pdc_map_mem_data_t *cfg
);

ca_status_t aal_pdc_map_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_OUT aal_pdc_pdc_map_mem_data_t *cfg
);

ca_status_t aal_pdc_lspid_gem_index_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    lspid,
    CA_IN ca_uint32_t    gem_index
);

ca_status_t aal_pdc_lspid_gem_index_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    lspid,
    CA_OUT ca_uint32_t   *gem_index
);

ca_uint8_t aal_pdc_free_lspid_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_uint8_t gem_index
);

#endif
