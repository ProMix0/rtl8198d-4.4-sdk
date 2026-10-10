#ifndef __EPON_H__
#define __EPON_H__

#include "ca_types.h"

#define CA_EPON_MPCP_LLID_TO_INDEX(x) ca_epon_mpcp_llid_index_get(0, 0, x)
#define CA_MAX_EPON_PORT_LLID_NUMBER 64

#define CA_EPON_MPCP_PENDING_GRANTS_MIN 1
#define CA_EPON_MPCP_PENDING_GRANTS_MAX 8

typedef struct
{
    ca_boolean_t xgepon_linkok;
    ca_boolean_t opt_rx_los;        /* rx los of signal indication */
    ca_boolean_t opt_mod_abs;       /* module absent indication    */
    ca_boolean_t opt_tx_sd;         /* tx signal degradation indication */
    ca_boolean_t opt_tx_fault;      /* tx fault indication         */
}ca_epon_port_status_t;

typedef enum
{
    CA_EPON_PORT_LASER_POLARITY_LO = 0,   /* High level will make laser transmitter power-down. */
    CA_EPON_PORT_LASER_POLARITY_HI = 1    /* Low level will make laser transmitter power-down. */
}ca_epon_port_laser_polarity_t;

typedef enum
{
    CA_EPON_PORT_LASER_MODE_BURST = 0,    /* Transmitter works on burst mode and therefore ONU emits laser or not according to the GATE indication from OLT.*/
    CA_EPON_PORT_LASER_MODE_ON    = 1,    /* Transmitter laser on.*/
    CA_EPON_PORT_LASER_MODE_OFF   = 2     /* Transmitter is not allowed to emit laser anyway.*/
}ca_epon_port_laser_mode_t;

typedef enum
{
    CA_EPON_DIRECTION_ENC_TX = 0,       /* encrypt TX */
    CA_EPON_DIRECTION_ENC_RX = 1,       /* decryt RX */
    CA_EPON_DIRECTION_ENC_BI = 2        /* Bidirection */
}ca_epon_port_direction_encryption_t;

#define EPON_PORT_ENCRYPTION_KEY_LENGTH 16

typedef struct
{
    ca_uint8_t data[EPON_PORT_ENCRYPTION_KEY_LENGTH];   /* key value */
} ca_epon_port_encryption_key_t;

typedef enum
{
    CA_EPON_PORT_SPEED_1G     = 0,    /* u/s 1G and d/s 1G mode */
    CA_EPON_PORT_SPEED_1G_10G = 1,    /* u/s 1G and d/s 10G mode */
    CA_EPON_PORT_SPEED_XG     = 2     /* u/s 10G and d/s 10G mode */
} ca_epon_port_speed_t;

typedef struct
{
    ca_boolean_t mpcp_crc32_drop;
    ca_boolean_t mpcp_crc8_drop;
    ca_boolean_t mpcp_extention_drop;
} ca_epon_port_discard_control_t;

typedef struct
{
    ca_uint32_t rx_crc8_sld_err;        /* Number of received frames of which SLD or CRC8 is bad*/
    ca_uint32_t rx_bc_crc8_valid_fcs_err_frames;    /* Number of received broadcast frames of which CRC8 is good but FCS is bad */
    ca_uint32_t rx_ext_frames;          /* Number of received MPCP extension frames.*/
    ca_uint64_t rx_good_octets;         /* Number of received octets without error.*/
    ca_uint32_t rx_uc_frames;           /* Number of received unicast frames.*/
    ca_uint32_t rx_mc_frames;           /* Number of received multicast frames. */
    ca_uint32_t rx_bc_frames;           /* Number of received broadcast frames. */
    ca_uint32_t rx_fcs_error_frames;
    ca_uint32_t rx_oam_frames;
    ca_uint32_t rx_pause_frames;
    ca_uint32_t rx_unknown_oframes;
    ca_uint32_t rx_runt_frames;
    ca_uint32_t rx_oversize_frames;
    ca_uint32_t rx_64_frames;
    ca_uint32_t rx_65_127_frames;
    ca_uint32_t rx_128_255_frames;
    ca_uint32_t rx_256_511_frames;
    ca_uint32_t rx_512_1023_frames;
    ca_uint32_t rx_1024_1518_frames;
    ca_uint32_t rx_1519_max_frames;     /* frame size is equal to greater than 1519 bytes */
    ca_uint32_t tx_ext_frames;
    ca_uint64_t tx_good_octets;
    ca_uint32_t tx_uc_frames;
    ca_uint32_t tx_mc_frames;
    ca_uint32_t tx_bc_frames;
    ca_uint32_t tx_oam_frames;
    ca_uint32_t tx_pause_frames;
    ca_uint32_t tx_64_frames;
    ca_uint32_t tx_65_127_frames;
    ca_uint32_t tx_128_255_frames;
    ca_uint32_t tx_256_511_frames;
    ca_uint32_t tx_512_1023_frames;
    ca_uint32_t tx_1024_1518_frames;
    ca_uint32_t tx_1519_max_frames;     /* frame size is equal to or greater than 1519 bytes */
    ca_uint32_t mpcp_rx_bc_reg;         /* Number of received broadcast MPCP registration frames */
    ca_uint32_t mpcp_rx_bc_gate;        /* Number of received broadcast MPCP discovery gate frames. */
    ca_uint32_t mpcp_tx_mac_ctrl_frames;
    ca_uint32_t mpcp_tx_bc_reg_req;
    ca_uint32_t mpcp_tx_reg_ack;
    ca_uint32_t mpcp_rx_mac_ctrl_frames;
} ca_epon_port_stats_t;

typedef struct
{
    ca_uint32_t corrected_bytes;
    ca_uint32_t corrected_codewords;
    ca_uint32_t uncorrectable_codewords;
    ca_uint32_t total_codewords;
    ca_uint32_t good_pkt;
    ca_uint32_t err_pkt;
    ca_uint32_t invalid_blocks_err;
} ca_epon_port_fec_stats_t;

typedef enum
{
    CA_EPON_MPCP_S_LLID = 0,
    CA_EPON_MPCP_M_LLID = 1
} ca_epon_mpcp_llid_mode_t;

typedef enum
{
    CA_EPON_MPCP_REPORT_MODE_2QS_8Q = 0,
    CA_EPON_MPCP_REPORT_MODE_1QS_8Q = 1,
    CA_EPON_MPCP_REPORT_MODE_1QS_1Q = 2,
    CA_EPON_MPCP_REPORT_MODE_2QS_1Q = 3,
    CA_EPON_MPCP_REPORT_MODE_4QS_1Q = 4,
    CA_EPON_MPCP_REPORT_MODE_NTT    = 5,
} ca_epon_mpcp_report_mode_t;

typedef struct
{
    ca_uint32_t mpcp_llid;
    ca_uint32_t llid_index;
}ca_epon_llid_info_t;

typedef struct
{
    ca_epon_llid_info_t llid[CA_MAX_EPON_PORT_LLID_NUMBER];
    ca_uint32_t llid_mode;
    ca_uint32_t report_mode;
    ca_uint32_t laser_on_time;
    ca_uint32_t laser_off_time;
    ca_uint32_t sync_time;
    ca_uint32_t onu_discovery_info;
    ca_uint32_t olt_discovery_info;
}ca_epon_mpcp_info_t;

typedef enum
{
    CA_EPON_MPCP_DEREG_CAUSE_NO_DEREGISTER  = 0,
    CA_EPON_MPCP_DEREG_CAUSE_TIMESTAMPDRIPT = 1,
    CA_EPON_MPCP_DEREG_CAUSE_OLT_DEREGISTER = 2,
    CA_EPON_MPCP_DEREG_CAUSE_ONU_DEREGISTER = 3,
    CA_EPON_MPCP_DEREG_CAUSE_RX_REREGISTER  = 4,
    CA_EPON_MPCP_DEREG_CAUSE_MPCP_TIMEOUT   = 5,
    CA_EPON_MPCP_DEREG_CAUSE_GATE_TIMEOUT   = 6,
    CA_EPON_MPCP_DEREG_CAUSE_SW_DEREGISTER  = 7
} ca_epon_mpcp_dereg_cause_t;

typedef struct
{
    ca_uint32_t   mpcp_llid;      /* Indicates the registered LLID, only valid when field reg_state is TRUE. {0x0000 ~ 0xffff} */
    ca_mac_addr_t olt_mac_addr;   /* only valid when field reg_stats is TRUE   */
    ca_uint32_t   reg_state;      /* Indicates the MPCP registration state. {FALSE, TRUE} */
    ca_uint32_t   deregistration_cause;    /* Indicates the root cause of last registration. Refer to
                                             enumeration ca_epon_mpcp_dereg_cause_t. */
    ca_uint32_t  retry_times;    /* MPCP registrations retry times before a successful registration.
                                    Hardware will clear this counter by itself after MPCP registration is completed.
                                    {0x0000 ~ 0xffff} */
}ca_epon_mpcp_registration_status_t;

#define CA_MPCGRANT_FRAME_DROP_MAX      8

typedef struct
{
    ca_uint32_t rx_mpcp_frames;         /* Indicates received MPCP packet number from 10G RxMAC*/
    ca_uint32_t rx_ext_mpcp_frames;     /* Indicates received extended MPCP packet number from
                                                 10G RxMAC. Refer to table "Deregistration Cause" */
    ca_uint32_t rx_discovery_frames_drop;/* Indicates dropped discovery packet because of
                                                 discovery information mismatch between OLT and ONU */
    ca_uint32_t rx_ext_mpcp_frames_drop;/* Indicates dropped extended MPCP packet because of the overflow of buffer */
    ca_uint32_t rx_mpcp_gate;           /* Indicates number of MPCP gates received */
    ca_uint32_t rx_mpcp_register;       /* Indicates number of MPCP register received */
    ca_uint32_t rx_grant_frames_drops[CA_MPCGRANT_FRAME_DROP_MAX];   /* Indicates the dropped MPCP grant number. System defined eight kinds of drop reasons */
    ca_uint32_t tx_mpcp_frames;         /* Indicates transmitted MPCP packet number to 10G TxMAC or 1G TxMAC */
    ca_uint32_t tx_ext_mpcp_frames;     /* Indicates transmitted extended MPCP packet number to 10G TxMAC or 1G TxMAC */
    ca_uint32_t tx_mpcp_report;         /* Indicates number of MPCP report transmimtted */
    ca_uint32_t tx_mpcp_reg_request;    /* Indicates number of MPCP registration request transmitted */
    ca_uint32_t tx_mpcp_reg_ack;        /* Indicates number of MPCP registration ACK transmitted */
}ca_epon_mpcp_stats_t;

typedef struct
{
    ca_uint64_t rx_good_octets;     /* Received total good octets       */
    ca_uint32_t rx_uc_frames;       /* Received unicast frames          */
    ca_uint32_t rx_mc_frames;       /* Received multicast frames        */
    ca_uint32_t rx_bc_frames;       /* Received broadcast frames        */
    ca_uint32_t rx_crc8_valid_crc32_bad_frames;/* The frames with bad CRC32 but correct CRC8 */

    ca_uint32_t rx_oam_frames;      /* Received OAM frames          */

    ca_uint32_t rx_total_count;
    ca_uint32_t rx_64_frames;
    ca_uint32_t rx_65_127_frames;
    ca_uint32_t rx_128_255_frames;
    ca_uint32_t rx_256_511_frames;
    ca_uint32_t rx_512_1023_frames;
    ca_uint32_t rx_1024_1518_frames;
    ca_uint32_t rx_1519_max_frames;


    ca_uint64_t tx_good_octets;     /* Transmitted total good octets    */
    ca_uint32_t tx_uc_frames;       /* Transmitted unicast frames       */
    ca_uint32_t tx_mc_frames;       /* Transmitted multicast frames     */
    ca_uint32_t tx_bc_frames;       /* Transmitted broadcast frames     */

    ca_uint32_t tx_oam_frames;      /* Transmitted OAM frames           */

    ca_uint32_t tx_total_count;
    ca_uint32_t tx_64_frames;
    ca_uint32_t tx_65_127_frames;
    ca_uint32_t tx_128_255_frames;
    ca_uint32_t tx_256_511_frames;
    ca_uint32_t tx_512_1023_frames;
    ca_uint32_t tx_1024_1518_frames;
    ca_uint32_t tx_1519_max_frames;

} ca_epon_llid_stats_t;

typedef enum
{
    CA_EPON_SIEPON_PACKAGE_B_SCHEDULER_THRESHOLD         = 0,
    CA_EPON_SIEPON_PACKAGE_B_SCHEDULER_PRIO_REPORT_FIRST = 1,
    CA_EPON_SIEPON_PACKAGE_B_SCHEDULER_PRIO_SP           = 2, /*strict priority*/
}ca_siepon_package_b_scheduler_mode_t;


ca_status_t ca_epon_port_laser_polarity_set
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_epon_port_laser_polarity_t tx_polarity,
    CA_IN ca_epon_port_laser_polarity_t rx_polarity
);

ca_status_t ca_epon_port_laser_polarity_get
(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_OUT ca_epon_port_laser_polarity_t *tx_polarity,
    CA_OUT ca_epon_port_laser_polarity_t *rx_polarity
);

ca_status_t ca_epon_port_laser_mode_set
(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_port_id_t              port_id,
    CA_IN ca_epon_port_laser_mode_t mode
);

ca_status_t ca_epon_port_laser_mode_get
(
    CA_IN  ca_device_id_t            device_id,
    CA_IN  ca_port_id_t              port_id,
    CA_OUT ca_epon_port_laser_mode_t *mode
);

ca_status_t ca_epon_port_fec_enable_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   tx_enable,
    CA_IN ca_boolean_t   rx_enable
);

ca_status_t ca_epon_port_fec_enable_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_boolean_t   *tx_enable,
    CA_OUT ca_boolean_t   *rx_enable
);

ca_status_t ca_epon_llid_encryption_enable_set
(
    CA_IN ca_device_id_t                      device_id,
    CA_IN ca_port_id_t                        port_id,
    CA_IN ca_llid_t                           llid,
    CA_IN ca_epon_port_direction_encryption_t direction,
    CA_IN ca_boolean_t                        enable
);

ca_status_t ca_epon_llid_encryption_enable_get
(
    CA_IN  ca_device_id_t                      device_id,
    CA_IN  ca_port_id_t                        port_id,
    CA_IN  ca_llid_t                           llid,
    CA_IN  ca_epon_port_direction_encryption_t direction,
    CA_OUT ca_boolean_t                        *enable
);

ca_status_t ca_epon_llid_encryption_active_key_index_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint8_t     active_key_index
);

ca_status_t ca_epon_llid_encryption_active_key_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid,
    CA_OUT ca_uint8_t     *active_key_index
);

ca_status_t ca_epon_llid_encryption_key_set
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid,
    CA_IN ca_uint32_t                   key_id,
    CA_IN ca_epon_port_encryption_key_t *key
);

ca_status_t ca_epon_llid_encryption_key_get
(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_IN  ca_llid_t                     llid,
    CA_IN  ca_uint32_t                   key_id,
    CA_OUT ca_epon_port_encryption_key_t *key
);

ca_status_t ca_epon_port_speed_set
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN ca_epon_port_speed_t speed
);

ca_status_t ca_epon_port_speed_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT ca_epon_port_speed_t *speed
);

ca_status_t ca_epon_port_discard_control_set
(
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
    CA_IN ca_epon_port_discard_control_t  *control
);

ca_status_t ca_epon_port_discard_control_get
(
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
    CA_OUT ca_epon_port_discard_control_t *control
);

ca_status_t ca_epon_port_stats_get
(
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_boolean_t                    read_clear,
    CA_OUT ca_epon_port_stats_t *data
);

ca_status_t ca_epon_port_fec_stats_get
(
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_boolean_t                    read_clear,
    CA_OUT ca_epon_port_fec_stats_t *data
);

ca_status_t ca_epon_port_link_status_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t    port_id,
    CA_OUT ca_boolean_t    *link
);

ca_status_t ca_epon_port_status_get
(
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
    CA_OUT ca_epon_port_status_t *status
);


ca_status_t ca_epon_mpcp_registration_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_llid_t                       llid,
CA_IN       ca_boolean_t                    enable);

ca_status_t ca_epon_mpcp_registration_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_llid_t                       llid,
CA_OUT      ca_boolean_t                    *enable);

ca_status_t ca_epon_mpcp_pending_grants_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_uint32_t                     pending_grants);

ca_status_t ca_epon_mpcp_pending_grants_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_uint32_t                     *pending_grants);

ca_status_t ca_epon_mpcp_timer_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_uint32_t                     mpcp_timer,
CA_IN       ca_boolean_t                    deregistration);

ca_status_t ca_epon_mpcp_timer_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_uint32_t                     *mpcp_timer,
CA_OUT      ca_boolean_t                    *deregistration);

ca_status_t ca_epon_mpcp_gate_timer_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_uint32_t                     gate_timer,
CA_IN       ca_boolean_t                    deregistration);

ca_status_t ca_epon_mpcp_gate_timer_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_uint32_t                     *gate_timer,
CA_OUT      ca_boolean_t                    *deregistration);


ca_status_t ca_epon_mpcp_timestamp_drift_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_uint32_t                     threshold,
CA_IN       ca_boolean_t                    deregistration);

ca_status_t ca_epon_mpcp_timestamp_drift_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_uint32_t                     *threshold,
CA_OUT      ca_boolean_t                    *deregistration);


ca_status_t ca_epon_mpcp_keep_silence_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_llid_t                       llid,
CA_IN       ca_boolean_t                    silence_enable,
CA_IN       ca_uint32_t                     silence_time);

ca_status_t ca_epon_mpcp_keep_silence_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_llid_t                       llid,
CA_OUT      ca_boolean_t                    *silence_enable,
CA_OUT      ca_uint32_t                     *silence_time);

ca_status_t ca_epon_queue_set_thresholds_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint32_t    threshold1,
    CA_IN ca_uint32_t    threshold2,
    CA_IN ca_uint32_t    threshold3,
    CA_IN ca_uint32_t    threshold4
);

ca_status_t ca_epon_queue_set_thresholds_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_OUT ca_uint32_t   *threshold1,
    CA_OUT ca_uint32_t   *threshold2,
    CA_OUT ca_uint32_t   *threshold3,
    CA_OUT ca_uint32_t   *threshold4
);

ca_status_t ca_epon_mpcp_info_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_epon_mpcp_info_t             *info);

ca_status_t ca_epon_mpcp_report_mode_set (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_IN       ca_epon_mpcp_report_mode_t      report_mode);

ca_status_t ca_epon_mpcp_report_mode_get (
CA_IN       ca_device_id_t                  device_id,
CA_IN       ca_port_id_t                    port_id,
CA_OUT      ca_epon_mpcp_report_mode_t      *report_mode);

ca_status_t ca_epon_mpcp_registration_status_get (
CA_IN       ca_device_id_t                      evice_id,
CA_IN       ca_port_id_t                        ort_id,
CA_IN       ca_llid_t                           llid,
CA_OUT      ca_epon_mpcp_registration_status_t  *status);

ca_status_t ca_epon_mpcp_stats_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_llid_t            llid,
    CA_IN  ca_boolean_t         read_clear,
    CA_OUT ca_epon_mpcp_stats_t *data
);

ca_status_t ca_epon_llid_stats_get
(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_llid_t            llid,
    CA_IN  ca_boolean_t         read_clear,
    CA_OUT ca_epon_llid_stats_t *data
);

ca_status_t ca_epon_dying_gasp_set
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     send_count,
    CA_OUT ca_uint8_t*    buffer/*[256:256]*/
);

ca_status_t ca_epon_lods_ignore_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   enable,
    CA_IN ca_uint8_t     duration
);

ca_status_t ca_epon_lods_ignore_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_boolean_t   *enable,
    CA_OUT ca_uint8_t     *duration
);

ca_status_t ca_siepon_mc_llid_add
(
    CA_IN ca_device_id_t device_id,
    CA_IN  ca_mpcp_llid_t mc_llid,
    CA_OUT ca_llid_t      *llid_instance
);

ca_status_t ca_siepon_mc_llid_delete
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_mpcp_llid_t mc_llid
);

ca_status_t ca_siepon_mc_llid_list
(
    CA_IN     ca_device_id_t device_id,
    CA_IN_OUT ca_uint32_t    *mc_llid_nums,
    CA_IN_OUT ca_mpcp_llid_t *mc_llids/*[32:32]*/
);

ca_status_t ca_siepon_package_b_report_config_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     queue_id,
    CA_IN ca_boolean     zeroed_queue,
    CA_IN ca_uint32_t    zero_queue_threshold_value
);

ca_status_t ca_siepon_package_b_report_config_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     queue_id,
    CA_OUT ca_boolean     *zeroed_queue,
    CA_OUT ca_uint32_t    *zero_queue_threshold_value
);

ca_status_t ca_siepon_package_b_mpcp_queue_threshold_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_llid_t      llid,
    CA_IN ca_uint32_t    threshold
);

ca_status_t ca_siepon_package_b_mpcp_queue_threshold_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_llid_t      *llid,
    CA_OUT ca_uint32_t    *threshold
);

ca_status_t ca_siepon_package_b_upstream_scheduler_mode_set
(
    CA_IN ca_device_id_t                       device_id,
    CA_IN ca_port_id_t                         port_id,
    CA_IN ca_siepon_package_b_scheduler_mode_t scheduler_mode
);

ca_status_t ca_siepon_package_b_upstream_scheduler_mode_get
(
    CA_IN  ca_device_id_t                       device_id,
    CA_IN  ca_port_id_t                         port_id,
    CA_OUT ca_siepon_package_b_scheduler_mode_t *scheduler_mode
);

ca_status_t ca_siepon_mc_register_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   mc_llid,
    CA_IN ca_port_id_t   uc_llid,
    CA_IN ca_boolean_t   f_bind
);

ca_status_t ca_siepon_mc_register_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   mc_llid,
    CA_OUT ca_port_id_t   *uc_llid
);

ca_status_t ca_epon_reset_pon_mac
(
    CA_IN ca_device_id_t device_id
);

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_epon_mpcp_silence_status_get (
CA_IN       ca_device_id_t         device_id,
CA_IN       ca_port_id_t           port_id,
CA_IN       ca_llid_t              llid,
CA_OUT      ca_boolean_t           *status);
#endif

ca_status_t ca_epon_llid_traffic_enable_set
(
    CA_IN   ca_device_id_t      device_id,
    CA_IN   ca_port_id_t        port_id,
    CA_IN   ca_llid_t           llid,
    CA_IN   ca_boolean_t        upstream,
    CA_IN   ca_boolean_t        downstream
);

ca_status_t ca_epon_llid_traffic_enable_get
(
    CA_IN   ca_device_id_t      device_id,
    CA_IN   ca_port_id_t        port_id,
    CA_IN   ca_llid_t           llid,
    CA_OUT  ca_boolean_t        *upstream,
    CA_OUT  ca_boolean_t        *downstream
);

ca_uint8_t ca_epon_mpcp_llid_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid
);

ca_status_t ca_epon_llid_loopback_enable_set (
CA_IN         ca_device_id_t                    device_id,
CA_IN         ca_port_id_t                      port_id,
CA_IN         ca_mpcp_llid_t                    llid,
CA_IN         ca_boolean_t                      enable);

ca_status_t ca_epon_llid_loopback_enable_get (
CA_IN         ca_device_id_t                    device_id,
CA_IN         ca_port_id_t                      port_id,
CA_IN         ca_mpcp_llid_t                    llid,
CA_OUT        ca_boolean_t                      *enable);

#endif

