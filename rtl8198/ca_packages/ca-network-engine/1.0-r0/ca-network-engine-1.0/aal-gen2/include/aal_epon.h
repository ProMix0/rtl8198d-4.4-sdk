#ifndef __AAL_EPON_H
#define __AAL_EPON_H

#include "ca_types.h"

extern ca_int32_t aal_epon_debug;

#define EPON_DEBUG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_epon_debug) ca_printf("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)

#define AAL_EPON_OAM_INFO_PDU_SIZE  64
/*
#define AAL_EPON_OAM_TYPE_OFFST     ((ca_uint8_t)17)
#define AAL_EPON_OAM_TYPE_INFO      ((ca_uint8_t)0x00)
#define AAL_EPON_OAM_FLAGS_OFFST    ((ca_uint8_t)16)
#define AAL_EPON_OAM_DYNG_GASP      ((ca_uint8_t)0x02)
*/

typedef enum
{
    AAL_EPON_SPEED_MODE_DS_25G_US_1G = 0,
    AAL_EPON_SPEED_MODE_DS_1G_US_1G,
    AAL_EPON_SPEED_MODE_DS_XG_US_1G,
    AAL_EPON_SPEED_MODE_DS_XG_US_XG,
    AAL_EPON_SPEED_MODE_MAX = AAL_EPON_SPEED_MODE_DS_XG_US_XG
}aal_epon_speed_mode_t;

typedef enum
{
    AAL_EPON_ETHER_MODE_ETH = 0,
    AAL_EPON_ETHER_MODE_PON,
    AAL_EPON_ETHER_MODE_MAX = AAL_EPON_ETHER_MODE_PON
}aal_epon_ether_mode_t;

typedef struct
{
    ca_boolean_t sig_loss;                  /* 1: no laser signal detected at epon port optical module; 0: signal detected */
    ca_boolean_t pcs_sync_xg;               /* 1: xg PCS sync. up; 0: xg PCS not sync. up */
    ca_boolean_t pcs_sync_1g;               /* 1: 1g PCS sync. up; 0: 1g PCS not sync. up */
    aal_epon_ether_mode_t ether_mode;
    aal_epon_speed_mode_t speed;
}aal_epon_status_t;

typedef struct
{
    ca_boolean_t opt_rx_los;
    ca_boolean_t opt_mod_abs;
    ca_boolean_t opt_tx_sd;
    ca_boolean_t opt_tx_fault;
}aal_epon_opt_status_t;

typedef enum
{
    AAL_EPON_LASER_MODE_BURST                   = 0, /* normal burst mode                                                          */
    AAL_EPON_LASER_MODE_ON                      = 1, /* laser is always on                                                         */
    AAL_EPON_LASER_MODE_OFF                     = 2, /* laser is always off                                                        */
} aal_epon_laser_mode_t;

typedef struct {
    ca_uint8_t                  tx_mode             ; /* 0-burst mode, 1-Laser always on, 2-laser is always off                      */
    ca_boolean_t                tx_ctrl_en          ; /* TX_DIS signal output: 1-enable, 0-disable                                   */
    ca_boolean_t                tx_power            ; /* 1-optical power up, 0-optical power down                                    */
    ca_boolean_t                tx_power_polarity   ; /* 1-optical power up in high level, 0-optical power up in low level           */
    ca_boolean_t                tx_burst_en         ; /* TX_BURST signal output: 1-enable, 0-disable                                 */
    ca_boolean_t                tx_burst_polarity_1g; /* 1-laser driven in high level, 0-laser driven in low level                   */
    ca_boolean_t                tx_burst_polarity_xg; /* 1-laser driven in high level, 0-laser driven in low level                   */
    ca_uint32_t                 tx_delay_1g         ; /* adjust laser tx delay to match MAC window and laser window, unit 8B, 0~63   */
    ca_uint32_t                 tx_delay_xg         ; /* adjust laser tx delay to match MAC window and laser window, unit 8B, 0~63   */
    ca_boolean_t                tx_idle_off         ; /* 1-end sending in advance when no data in tx queue, 0-according to grant     */
    ca_boolean_t                rssi_pulse_on       ; /* 1-RSSI pulse on, 0-RSSI pulse off                                           */
    ca_boolean_t                rssi_polarity       ; /* 1-RSSI pulse on in high level, 0-RSSI pulse on in low level                 */
} aal_epon_laser_cfg_t;

typedef union {
    struct {
        ca_uint32_t             tx_mode             : 1;
        ca_uint32_t             tx_ctrl_en          : 1;
        ca_uint32_t             tx_power            : 1;
        ca_uint32_t             tx_power_polarity   : 1;
        ca_uint32_t             tx_burst_en         : 1;
        ca_uint32_t             tx_burst_polarity_1g: 1;
        ca_uint32_t             tx_burst_polarity_xg: 1;
        ca_uint32_t             tx_delay_1g         : 1;
        ca_uint32_t             tx_delay_xg         : 1;
        ca_uint32_t             tx_idle_off         : 1;
        ca_uint32_t             rssi_pulse_on       : 1;
        ca_uint32_t             rssi_polarity       : 1;
    } s;
    ca_uint32_t u32;
} aal_epon_laser_msk_t;

typedef enum {
    EPON_TX_SW_FRAME_OAM = 3,
    EPON_TX_SW_FRAME_EXT = 4
}aal_epon_tx_sw_frame_type_e;

typedef volatile union {
  struct {
    ca_uint32_t pkt_sz               : 12 ; /* bits 11:0 */
    ca_uint32_t rsrvd1               : 1  ; /* bits 12   */
    ca_uint32_t frame_type           : 3  ; /* bits 15:13 */
    ca_uint32_t llid                 : 16 ; /* bits 30:16 */
  } bf ;
  ca_uint32_t     wrd ;
} aal_epon_tx_sw_frame_header_t;


ca_status_t aal_epon_lao_set
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t flag
);

ca_status_t aal_epon_lao_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t *flag
);

ca_status_t aal_epon_laser_enable_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t flag
);

ca_status_t aal_epon_laser_enable_get
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t *flag
);

ca_status_t aal_epon_laser_cfg_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN aal_epon_laser_msk_t msk,
    CA_IN aal_epon_laser_cfg_t *cfg
);

ca_status_t aal_epon_laser_cfg_get
(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          port_id,
    CA_OUT aal_epon_laser_cfg_t *cfg
);


ca_status_t aal_epon_status_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t      port_id,
    CA_OUT aal_epon_status_t *status
);

ca_status_t aal_epon_opt_status_get
(
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT aal_epon_opt_status_t *status
);

ca_status_t aal_epon_dying_gasp_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     send_count,
    CA_IN ca_uint8       *pkt_buff,
    CA_IN ca_int32       pkt_len
);
ca_status_t aal_epon_dying_gasp_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_OUT ca_uint8_t    *send_count,
    CA_OUT ca_uint8      *pkt_buff,
    CA_OUT ca_int32      *pkt_len
);

ca_status_t aal_epon_init   /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
);

ca_status_t aal_epon_reset   /*CODEGEN_IGNORE_TAG*/
(
    ca_device_id_t device_id
);


void aal_epon_dying_gasp_handler/*CODEGEN_IGNORE_TAG*/
(
    ca_int32 gpio_pin,
    ca_uint8 status
);

ca_status_t aal_epon_init_debug_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
);

ca_status_t aal_epon_init_debug_get
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *mode
);

ca_status_t aal_epon_reset_all_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    mode
);

ca_status_t aal_epon_reset_all_get
(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t   *mode
);

#endif


