#ifndef __AAL_PTP_SFU_H__
#define __AAL_PTP_SFU_H__

#include "ca_types.h"

/* In G3 / G3Lite / G3HGU, it's default value of reg NI_HV_PTP_PPS_CFG_1.
 * In Saturn, it's default value of reg PTP_TIMER_CFG_0.
 */
#define DEFAULT_PTP_CLOCK_PERIOD	0x80000000	/* 125MHz, align to 8ns */

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

typedef union {
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

#endif /* __AAL_PTP_SFU_H__ */
