#ifndef __AAL_XCVR_H__
#define __AAL_XCVR_H__

#define AAL_XCVR_VENDOR_STR_LEN     16

#define AAL_XCVR_CHANNLE_MAX_NUM  4

typedef struct {
    ca_char     name[16];
    ca_uint8_t  oui[3];
    ca_uint8_t  pn[16];
    ca_uint8_t  revesion[4];
    ca_uint8_t  sn[16];
    ca_uint8_t  date[8];
}aal_xcvr_vendor_info_t;


typedef struct {
    ca_uint8_t  tx_chan_num;
    ca_uint8_t  rx_chan_num;

} aal_xcvr_cap_t;

ca_status_t aal_xcvr_capbility_get(
    CA_IN ca_device_id_t      device_id,
    CA_OUT aal_xcvr_cap_t  *cap);

ca_status_t aal_xcvr_vendor_info_get(
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  aal_xcvr_vendor_info_t *vendor_info);

ca_status_t aal_xcvr_tx_channle_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  chan);

ca_status_t aal_xcvr_tx_channle_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *chan);

ca_status_t aal_xcvr_rx_channle_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  chan);

ca_status_t aal_xcvr_rx_channle_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *chan);

ca_status_t aal_xcvr_tx_rx_channle_mset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  tx_chan,
    CA_IN ca_uint8_t  rx_chan);

ca_status_t aal_xcvr_tx_rx_channle_mget(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_OUT ca_uint8_t  *tx_chan,
    CA_OUT ca_uint8_t  *rx_chan);

ca_status_t aal_xcvr_tx_power_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *tx_power);

ca_status_t aal_xcvr_rx_power_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *rx_power);

ca_status_t aal_xcvr_tx_wave_len_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_uint16_t  wave_len);

ca_status_t aal_xcvr_tx_wave_len_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *wave_len);

ca_status_t aal_xcvr_freq_chn_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  ca_uint16_t *freq_chn);

ca_status_t aal_xcvr_freq_chn_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_uint16_t  freq_chn);

ca_status_t aal_xcvr_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id);

#endif

