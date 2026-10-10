#ifndef __AAL_XGPON_FEC_H__
#define __AAL_XGPON_FEC_H__

typedef struct {
    ca_uint8_t rx_fec_bypass_delay_mode  ;
    ca_uint8_t fec_dec_corr_disable      ;
    ca_uint8_t tx_fec_bypass_delay_mode  ;
    ca_uint8_t error_inject_count        ;
    ca_uint8_t swap_after_fec_encoder    ;
    ca_uint8_t swap_before_fec_encoder   ;
    ca_uint8_t disableFEC                ;
} aal_xgpon_fec_ctrl_t;

typedef union {
    struct {
        ca_uint32_t rx_fec_bypass_delay_mode   : 1;
        ca_uint8_t  fec_dec_corr_disable       : 1;
        ca_uint32_t tx_fec_bypass_delay_mode   : 1;
        ca_uint32_t error_inject_count         : 1;
        ca_uint32_t swap_after_fec_encoder     : 1;
        ca_uint32_t swap_before_fec_encoder    : 1;
        ca_uint32_t disableFEC                 : 1;
        ca_uint32_t rsvd                       : 25;
    }s;
    ca_uint32_t u32_data;
} aal_xgpon_fec_ctrl_mask_t;

typedef struct {
    ca_uint32_t corrected_bytes_rx;
    ca_uint32_t corrected_codewords_rx ;
    ca_uint32_t total_codewords_rx    ;
    ca_uint32_t uncorrectable_codewords_rx  ;
} aal_xgpon_fec_stats_t;

ca_status_t aal_xgpon_fec_ctrl_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_xgpon_fec_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpon_fec_ctrl_t      *ctrl);

ca_status_t aal_xgpon_fec_ctrl_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_xgpon_fec_ctrl_t      *ctrl);

ca_status_t aal_xgpon_fec_err_inject_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_boolean_t               inject_enable);

ca_status_t aal_xgpon_fec_err_inject_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_boolean_t              *inject_enable);

ca_status_t aal_xgpon_fec_mask_mem_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_uint8_t                 mem_addr,
    CA_IN ca_uint16_t                mem_data);

ca_status_t aal_xgpon_fec_mask_mem_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 mem_addr,
    CA_OUT ca_uint16_t               *mem_data);

ca_status_t aal_xgpon_fec_stats_get(
    CA_IN  ca_device_id_t              device_id,
    CA_OUT aal_xgpon_fec_stats_t   *stats);

#endif /* __AAL_XGPON_FEC_H__ */

