#ifndef __PTP_H__
#define __PTP_H__

#include "ca_types.h"

#define PTP_TOD_MSG_MAX  64

typedef enum{
    PTP_TX_PORT0 = 0,
    PTP_TX_PORT1 = 1,
    PTP_TX_PORT2 = 2,
    PTP_TX_PORT3 = 3,
    PTP_TX_PORT4 = 4,
    PTP_TX_PORT5 = 5,
    PTP_TX_PORT6 = 6,
    PTP_TX_PORT7 = 7,
}ca_ptp_tx_port_t;

/* PTP local clock is expressed in the following format
 * ss ss ss ss ss ss - 48 bits seconds
 * nn nn nn nn - 32 bits nano-seconds
 */
typedef struct ca_ptp_local_clock {
    ca_uint64_t		sec; /*48 bits ss field of PTP local clock */
    ca_uint32_t		nsec; /*32 bits nn field of PTP local clock */
} ca_ptp_local_clock_t;

typedef struct {
	ca_uint32_t		pps_trigger_edge:1;
    ca_uint32_t		local_time:1;
	ca_uint32_t		start_sfc:1;
    ca_uint32_t		mpcp_time:1;
} ca_ptp_set_trigger_mask_t;

typedef struct ca_ptp_set_trigger {
    ca_ptp_set_trigger_mask_t		mask;
	ca_uint8_t			pps_trigger_edge;	/* 0 - rising edge, 1 - falling edge */
    ca_uint32_t 			local_time;/*set HW time when local time matches the configured value */
    ca_uint64_t 			start_sfc; /*set HW time when SFC matches the configured value */
    ca_uint32_t 			mpcp_time; /*set HW time when MPCP time matches the configured value */
} ca_ptp_set_trigger_t;

typedef struct {
    ca_uint32_t		uart:1;
    ca_uint32_t		mode:1;
    ca_uint32_t		start_tq:1;
    ca_uint32_t		start_sfc:1;
    ca_uint32_t		tod_msg:1;
    ca_uint32_t		pulse_width:1;
} ca_tod_pps_mask_t;

typedef struct ca_tod_pps_config_s {
    ca_tod_pps_mask_t			mask;
    ca_uint8_t 				uart; /*select which user to send ToD*/
	ca_uint8_t		mode;		/* 0 = ToD Time of Day) TX, 1 = PPS (Pulse Per Second) TX; see ca_ptp_tx_mode_t */
    ca_uint32_t 			start_tq; /*MPCP timer (EPON) to start TX */
    ca_uint64_t 			start_sfc; /*SFC (GPON) to start TX */
	ca_uint32_t		tod_msg_len;	/* ToD (Time of Day) message length in byte */
	ca_uint8_t		*tod_msg_data;	/* ToD (Time of Day) message data */
	ca_uint32_t		pulse_width;	/* PPS (Pulse Per Second) pulse width */
} ca_tod_pps_config_t;

typedef enum{
    PTP_TOD_TX = 0,
    PTP_PPS_TX = 1
}ca_ptp_tx_mode_t;

typedef enum ca_ptp_tx_type {
	PTP_OVER_L2			= 0,	/* PTP packet over Ethernet */
	PTP_OVER_CFM			= 1,	/* PTP packet over ITU Y.1731 */
	PTP_OVER_IPV4			= 2,	/* PTP over UDP IPV4 */
	PTP_OVER_IPV6			= 3,	/* PTP over UDP IPV6 */
} ca_ptp_pkt_tx_type_t;

typedef enum ca_ptp_tx_cmd {
	PTP_TX_TS			= 0,	/* set timestamp field (only the nn nn nn nn part), 2-step uses this mode */
	PTP_TX_CORRECTION		= 1,	/* set correction field - supports 4
						 * pre-configured correction values - 1 step uses this mode
						 */
} ca_ptp_pkt_tx_cmd_t;

/* ca_ptp_port_pkt_send will append the following struct to the end of PTP frame before sending the
 * PTP packet to network driver. In case of PTP over UDP, sender must append the following struct to
 * the UDP datagrame before sending PTP packet to UDP socket
 */
typedef struct ca_ptp_cmd {
	/* fields used by HW for PTP packet modification */
	ca_ptp_pkt_tx_type_t	ptp_tx_type;			/* see ca_ptp_pkt_tx_type_t */
	ca_uint32_t 		udp_checksum_offset;		/* only valid with ptp_tx_type = PTP_OVER_IPVX */
	ca_uint32_t 		modified_field_offset;		/* offset of field to modify starting from SOP */

	/*select field to update*/
	ca_ptp_pkt_tx_cmd_t	tx_cmd;				/* ca_ptp_pkt_tx_cmd_t */
	ca_uint8_t		correction_select;		/* select one of the 4 pre-configured correction values */
} ca_ptp_cmd_t;


#define CA_PTP_SFC_GAP_MIN  4000
#define CA_PTP_SFC_GAP_MAX  8000

ca_status_t ca_ptp_local_clock_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ptp_set_trigger_t		*trigger,
	CA_IN	ca_ptp_local_clock_t		*local_clock);

ca_status_t ca_ptp_local_clock_get (
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	ca_ptp_local_clock_t	*local_clock);

ca_status_t ca_ptp_local_clock_freq_adj_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_int32_t			freq_adjustment);

ca_status_t ca_ptp_local_clock_freq_adj_get (
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	ca_int32_t			*freq_adjustment);

ca_status_t ca_ptp_port_sync_tx_time_get (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
	CA_OUT	ca_ptp_local_clock_t		*hw_sync_tx_time);

ca_status_t ca_syncE_clock_source_port_set (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
    CA_IN	ca_boolean_t		enable);

ca_status_t ca_syncE_clock_source_port_get (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
	CA_OUT	ca_boolean_t			*enable);

ca_status_t ca_tod_pps_tx_config_set (
    CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tod_pps_config_t		*config);

ca_status_t ca_tod_pps_tx_config_get (
    CA_IN	ca_device_id_t		device_id,
	CA_OUT	ca_tod_pps_config_t		*config);

ca_status_t ca_ptp_local_clock_adj_offset(
	CA_IN   ca_device_id_t          device_id,
	CA_IN   ca_int32_t offset);

ca_status_t ca_ptp_init (
    CA_IN   ca_device_id_t device_id,
	CA_IN	ca_boolean_t			enable);

#endif /* __PTP_H__ */
