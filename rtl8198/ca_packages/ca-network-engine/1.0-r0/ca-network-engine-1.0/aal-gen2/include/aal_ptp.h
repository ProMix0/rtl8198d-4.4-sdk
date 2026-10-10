#ifndef __AAL_PTP_H__
#define __AAL_PTP_H__

#include "ca_types.h"

/* In G3 / G3Lite / G3HGU, it's default value of reg NI_HV_PTP_PPS_CFG_1.
 * In Saturn, it's default value of reg PTP_TIMER_CFG_0.
 */
#define DEFAULT_PTP_CLOCK_PERIOD	0x80000000	/* 125MHz, align to 8ns */

typedef union {
	struct {
		ca_uint32_t ptp_en               :  1;
		ca_uint32_t rsrvd1               :  1;
		ca_uint32_t pps_in_en            :  1;
		ca_uint32_t pps_edge_sel_sample  :  1;
		ca_uint32_t pps_edge_sel_load    :  1;
		ca_uint32_t pps_load_en          :  1;
		ca_uint32_t pps_out_en           :  1;
		ca_uint32_t rsrvd2               :  9;
		ca_uint32_t pps_width            : 16;
	} s;
	ca_uint32_t u32;
} aal_ptp_pps_cfg0_mask_t;

typedef union {
	struct {
		ca_uint32_t s_pps                : 30;
		ca_uint32_t rsrvd1               :  2;
	} s;
	ca_uint32_t u32;
} aal_ptp_pps_cfg1_mask_t;


typedef struct {
	ca_uint32_t ptp_en;
	ca_uint32_t pps_in_en;
	ca_uint32_t pps_edge_sel_sample;
	ca_uint32_t pps_edge_sel_load;
	ca_uint32_t pps_load_en;
	ca_uint32_t pps_out_en;
	ca_uint32_t pps_width;
	ca_uint32_t s_pps;
} aal_ptp_pps_cfg_t;

typedef union {
	aal_ptp_pps_cfg0_mask_t cfg0_mask;
	aal_ptp_pps_cfg1_mask_t cfg1_mask;
} aal_ptp_pps_cfg_mask_t;

typedef union {
	struct {
		ca_uint32_t ptp_clk_period       : 32;
	} s;
	ca_uint32_t u32;
} aal_ptp_timer_cfg0_mask_t;

typedef union {
	struct {
		ca_uint32_t ptp_tmr_load_val     : 30;
		ca_uint32_t rsrvd1               :  2;
	} s;
	ca_uint32_t u32;
} aal_ptp_timer_cfg1_mask_t;

typedef union {
	struct {
		ca_uint32_t ptp_tmr_match_val    : 30;
		ca_uint32_t pon_tmr_match_load   :  1;
		ca_uint32_t ptp_tmr_match_load   :  1;
	} s;
	ca_uint32_t u32;
} aal_ptp_timer_cfg2_mask_t;

typedef union {
	struct {
		ca_uint32_t ptp_hw_ts            : 30;
		ca_uint32_t rsrvd1               :  2;
	} s ;
	ca_uint32_t u32 ;
} aal_ptp_timer_mask_t;

typedef union {
	aal_ptp_timer_cfg0_mask_t cf0_mask;
	aal_ptp_timer_cfg1_mask_t cf1_mask;
	aal_ptp_timer_cfg2_mask_t cf2_mask;
} aal_ptp_timer_cfg_mask_t;

typedef struct {
	ca_uint32_t ptp_clk_period;
	ca_uint32_t ptp_tmr_load_val;
	ca_uint32_t ptp_tmr_match_val;
	ca_uint32_t pon_tmr_match_load;
	ca_uint32_t ptp_tmr_match_load;
} aal_ptp_timer_cfg_t;

typedef union {
	struct {
		ca_uint32_t local_tmr_match_val  : 30;
		ca_uint32_t rsrvd1               :  1;
		ca_uint32_t local_tmr_match_load :  1;
	} s;
	ca_uint32_t u32;
} aal_ptp_local_timer_cfg0_mask_t;

typedef volatile union {
	struct {
		ca_uint32_t local_tmr_load_val   : 30;
		ca_uint32_t rsrvd1               :  1;
		ca_uint32_t local_tmr_en         :  1;
	} s;
	ca_uint32_t u32;
} aal_ptp_local_timer_cfg1_mask_t;

typedef volatile union {
	struct {
		ca_uint32_t local_period         : 32;
	} s;
	ca_uint32_t u32;
} aal_ptp_local_timer_cfg2_mask_t;

typedef volatile union {
	struct {
		ca_uint32_t local_ts_subcnt      : 27;
		ca_uint32_t rsrvd1               :  5;
	} s;
	ca_uint32_t u32;
} aal_ptp_local_timer_cfg3_mask_t;

typedef volatile union {
	struct {
		ca_uint32_t local_hw_ts          : 30;
		ca_uint32_t rsrvd1               :  2;
	} s;
	ca_uint32_t u32;
} aal_ptp_local_timer_mask_t;

typedef struct {
	aal_ptp_local_timer_cfg0_mask_t	local_timer_cfg0_mask;
	aal_ptp_local_timer_cfg1_mask_t	local_timer_cfg1_mask;
	aal_ptp_local_timer_cfg2_mask_t	local_timer_cfg2_mask;
	aal_ptp_local_timer_cfg3_mask_t	local_timer_cfg3_mask;
	aal_ptp_local_timer_mask_t	local_timer_mask;
} aal_ptp_local_timer_cfg_mask_t;

typedef struct {
	ca_uint32_t local_tmr_match_val;
	ca_uint32_t local_tmr_match_load;
	ca_uint32_t local_tmr_load_val;
	ca_uint32_t local_tmr_en;
	ca_uint32_t local_period;
	ca_uint32_t local_ts_subcnt;
	ca_uint32_t local_hw_ts;
} aal_ptp_local_timer_cfg_t;

/*********************************************************
 * PTP PPS config
 *********************************************************/

ca_status_t aal_ptp_pps_cfg_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	aal_ptp_pps_cfg_mask_t	mask,
	CA_IN	aal_ptp_pps_cfg_t	config);

ca_status_t aal_ptp_pps_cfg_get(
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	aal_ptp_pps_cfg_t	*config);

/*********************************************************
 * PTP Timer config
 * To synchronize time at certain timing
 *********************************************************/

ca_status_t aal_ptp_timer_cfg_set(
	CA_IN  ca_device_id_t		device_id,
	CA_IN  aal_ptp_timer_cfg_mask_t	mask,
	CA_IN  aal_ptp_timer_cfg_t	config);

ca_status_t aal_ptp_timer_cfg_get(
	CA_IN  ca_device_id_t		device_id,
	CA_IN  aal_ptp_timer_cfg_t	*config);

/*********************************************************
 * PTP Timer Rollover
 *********************************************************/

void aal_ptp_timer_rollover_intr_enable(ca_boolean_t enable);
bool aal_ptp_timer_rollover_intr_status_get(void);
void aal_ptp_timer_rollover_intr_status_clear(void);

void aal_ptp_timer_rollover_triggered(void);

/*********************************************************
 * PTP Tx Timestamp
 *********************************************************/

void aal_ptp_tx_timestamp_intr_enable(ca_boolean_t enable);
uint32_t aal_ptp_tx_timestamp_intr_status_get(void);
void aal_ptp_tx_timestamp_intr_status_clear(ca_uint32_t port_bitmap);

ca_status_t aal_ptp_tx_timestamp_record(ca_port_id_t port_id);
ca_status_t aal_ptp_tx_timestamp_get(ca_port_id_t port_id, ca_uint64_t *time);

/*********************************************************
 * PTP Timer - access/adjust directly
 *********************************************************/

ca_status_t aal_ptp_timer_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint64_t	time);

ca_status_t aal_ptp_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint64_t	*time);

ca_status_t aal_ptp_clock_period_set(
	CA_IN   ca_device_id_t  device_id,
	CA_IN   ca_uint32_t     clock_period);

ca_status_t aal_ptp_clock_period_get(
	CA_IN   ca_device_id_t  device_id,
	CA_OUT  ca_uint32_t     *clock_period);

/*********************************************************
 * PTP PPS Timer
 *********************************************************/

ca_status_t aal_ptp_pps_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint32_t	*time);

/*********************************************************
 * PTP Local Timer config
 * Registers exist in Saturn SFU and G3HGU Saturon core only
 *********************************************************/

ca_status_t aal_ptp_local_timer_cfg_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_mask_t	mask,
	CA_IN	aal_ptp_local_timer_cfg_t	config);

ca_status_t aal_ptp_local_timer_cfg_get(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_t	*config);

#endif /* __AAL_PTP_H__ */
