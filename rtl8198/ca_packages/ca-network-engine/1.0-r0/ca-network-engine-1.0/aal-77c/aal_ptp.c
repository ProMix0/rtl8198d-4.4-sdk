#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_ptp.h"
#include "aal_pon.h"

/* IMPORTANT:
 *   Shoult NOT use PTP registers in G3HGU Saturn core.
 *   In G3HGU Rx, HDR_A of Saturn core can't be delivered to G3 core, so G3 core can't know tagged Rx timestamp at Saturn core.
 *   In G3HGU Tx, G3 core can't deliver PTP_CMD of HDR_A to Saturn core, so Saturn core can't tag Tx timestamp.
 *   In simple words, HDR_A of both cores can't be shared.
 */

/* PTP registers mapping:
 *   <Saturn>		<G3, G3HGU, G3Lite>
 *   PTP_PPS_CFG_0	NI_HV_PTP_PPS_CFG_0
 *   PTP_PPS_CFG_1	NI_HV_PTP_PPS_CFG_3
 *   PTP_TOD_CFG	NI_HV_PTP_PPS_CFG_4
 *   PTP_TIMER_CFG_0	NI_HV_PTP_PPS_CFG_1
 *   PTP_TIMER_CFG_1	NI_HV_PTP_PPS_CFG_2
 *   PTP_TIMER_CFG_2	NI_HV_PTP_PPS_CFG_5
 *   PTP_TIMER		NI_HV_PTP_TIMER
 *   PTP_TS_ON_PPS	NI_HV_PTP_TIMER_ON_PPS
 *   PTP_INTERRUPT	NI_HV_GLB_PTP_INTERRUPT
 *   PTP_INTENABLE	NI_HV_GLB_PTP_INTERRUPTE
 */

#define NUM_PORT	8
#define	NSECS_PER_PTP_TX_TIMESTAMP	(1)

uint64_t tx_timestamp[NUM_PORT] = {0, 0, 0, 0, 0, 0, 0, 0}; /* recorded Tx timestamp for port 0~7 */
atomic_t ptp_second; /* CA PTP timer provides lower 32-bit time (nanosecond), and this records higher 32-bit time (second). */

/*********************************************************
 * PTP PPS config
 *********************************************************/

ca_status_t aal_ptp_pps_cfg_set(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	aal_ptp_pps_cfg_mask_t	mask,
	CA_IN	aal_ptp_pps_cfg_t	config)
{
	PTP_PPS_CFG_0_t ptp_pps_cfg0;
	PTP_PPS_CFG_1_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_1);

	if (mask.cfg0_mask.s.ptp_en) {
		ptp_pps_cfg0.bf.ptp_en = config.ptp_en;
	}

	if (mask.cfg0_mask.s.pps_in_en) {
		ptp_pps_cfg0.bf.pps_in_en = config.pps_in_en;
	}

	if (mask.cfg0_mask.s.pps_edge_sel_sample) {
		ptp_pps_cfg0.bf.pps_edge_sel_sample = config.pps_edge_sel_sample;
	}

	if (mask.cfg0_mask.s.pps_edge_sel_load) {
		ptp_pps_cfg0.bf.pps_edge_sel_load = config.pps_edge_sel_load;
	}

	if (mask.cfg0_mask.s.pps_load_en) {
		ptp_pps_cfg0.bf.pps_load_en = config.pps_load_en;
	}

	if (mask.cfg0_mask.s.pps_out_en) {
		ptp_pps_cfg0.bf.pps_out_en = config.pps_out_en;
	}

	if (mask.cfg0_mask.s.pps_width) {
		ptp_pps_cfg0.bf.pps_width = config.pps_width;
	}

	if (mask.cfg1_mask.s.s_pps) {
		ptp_pps_cfg1.bf.s_pps = config.s_pps;
	}

	CA_NE_REG_WRITE(ptp_pps_cfg0.wrd, PTP_PPS_CFG_0);
	CA_NE_REG_WRITE(ptp_pps_cfg1.wrd, PTP_PPS_CFG_1);

	return CA_E_OK;
}

ca_status_t aal_ptp_pps_cfg_get(
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	aal_ptp_pps_cfg_t	*config)
{
	PTP_PPS_CFG_0_t ptp_pps_cfg0;
	PTP_PPS_CFG_1_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_1);

	config->ptp_en = ptp_pps_cfg0.bf.ptp_en;
	config->pps_in_en = ptp_pps_cfg0.bf.pps_in_en;
	config->pps_edge_sel_sample = ptp_pps_cfg0.bf.pps_edge_sel_sample;
	config->pps_edge_sel_load = ptp_pps_cfg0.bf.pps_edge_sel_load;
	config->pps_load_en = ptp_pps_cfg0.bf.pps_load_en;
	config->pps_out_en = ptp_pps_cfg0.bf.pps_out_en;
	config->pps_width = ptp_pps_cfg0.bf.pps_width;
	config->s_pps = ptp_pps_cfg1.bf.s_pps;

	return CA_E_OK;
}

/*********************************************************
 * PTP Timer config
 * To synchronize time at certain timing
 *********************************************************/

ca_status_t aal_ptp_timer_cfg_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_timer_cfg_mask_t	mask,
	CA_IN	aal_ptp_timer_cfg_t		config)
{
	PTP_TIMER_CFG_0_t ptp_timer_cfg0;
	PTP_TIMER_CFG_1_t ptp_timer_cfg1;
	PTP_TIMER_CFG_2_t ptp_timer_cfg2;

	ptp_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_0);
	ptp_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_1);
	ptp_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_2);

	if (mask.cf0_mask.s.ptp_clk_period) {
		ptp_timer_cfg0.bf.ptp_clk_period = config.ptp_clk_period;
	}

	if (mask.cf1_mask.s.ptp_tmr_load_val) {
		ptp_timer_cfg1.bf.ptp_tmr_load_val = config.ptp_tmr_load_val;
	}

	if (mask.cf2_mask.s.ptp_tmr_match_val) {
		ptp_timer_cfg2.bf.ptp_tmr_match_val = config.ptp_tmr_match_val;
	}

	if (mask.cf2_mask.s.pon_tmr_match_load) {
		ptp_timer_cfg2.bf.pon_tmr_match_load = config.pon_tmr_match_load;
	}

	if (mask.cf2_mask.s.ptp_tmr_match_load) {
		ptp_timer_cfg2.bf.ptp_tmr_match_load = config.ptp_tmr_match_load;
	}

	CA_NE_REG_WRITE(ptp_timer_cfg0.wrd, PTP_TIMER_CFG_0);
	CA_NE_REG_WRITE(ptp_timer_cfg1.wrd, PTP_TIMER_CFG_1);
	CA_NE_REG_WRITE(ptp_timer_cfg2.wrd, PTP_TIMER_CFG_2);

	return CA_E_OK;
}

ca_status_t aal_ptp_timer_cfg_get(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	aal_ptp_timer_cfg_t	*config)
{
	PTP_TIMER_CFG_0_t ptp_timer_cfg0;
	PTP_TIMER_CFG_1_t ptp_timer_cfg1;
	PTP_TIMER_CFG_2_t ptp_timer_cfg2;

	ptp_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_0);
	ptp_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_1);
	ptp_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_2);

	config->ptp_clk_period = ptp_timer_cfg0.bf.ptp_clk_period;
	config->ptp_tmr_load_val = ptp_timer_cfg1.bf.ptp_tmr_load_val;
	config->ptp_tmr_match_load = ptp_timer_cfg2.bf.ptp_tmr_match_load;
	config->ptp_tmr_match_val = ptp_timer_cfg2.bf.ptp_tmr_match_val;
	config->pon_tmr_match_load = ptp_timer_cfg2.bf.pon_tmr_match_load;

	return CA_E_OK;
}

/*********************************************************
 * PTP Timer Rollover
 *
 * These interrupts' hierarchy in Venus:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.PON
 *    |- GLOBAL_PON_INTERRUPT_0.PTP
 *        |- PTP_INTERRUPT.timer_rollover_intI
 *
 *********************************************************/

void aal_ptp_timer_rollover_intr_enable(ca_boolean_t enable)
{
	PTP_INTENABLE_t	ptp_intr_enable;
	GLOBAL_PON_INTENABLE_0_t pon_intr_enable;

	ptp_intr_enable.wrd = CA_NE_PTP_REG_READ(PTP_INTENABLE);
	ptp_intr_enable.bf.timer_rollover_IE = (enable) ? 1 : 0;
	CA_NE_PTP_REG_WRITE(ptp_intr_enable.wrd, PTP_INTENABLE);
#if defined(CONFIG_NE_PON)
	pon_intr_enable.wrd = CA_REG_READ(AAL_PON_GLB_REG_ADDR(GLOBAL_PON_INTENABLE_0));
	pon_intr_enable.bf.PTPe =  (enable) ? 1 : 0;
	CA_REG_WRITE(pon_intr_enable.wrd, AAL_PON_GLB_REG_ADDR(GLOBAL_PON_INTENABLE_0));
#endif
}

bool aal_ptp_timer_rollover_intr_status_get(void)
{
	PTP_INTERRUPT_t status;
	status.wrd = CA_NE_PTP_REG_READ(PTP_INTERRUPT);
	return (status.bf.timer_rollover_intI == 1);
}

void aal_ptp_timer_rollover_intr_status_clear(void)
{
	PTP_INTERRUPT_t ptp_intr_status;

	ptp_intr_status.wrd = 0;
	ptp_intr_status.bf.timer_rollover_intI = 1;
	CA_NE_PTP_REG_WRITE(ptp_intr_status.wrd, PTP_INTERRUPT);
}

void aal_ptp_timer_rollover_triggered(void)
{
	atomic_inc(&ptp_second);
}

/*********************************************************
 * PTP Tx Timestamp
 *
 * These interrupts' hierarchy in Venus:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.NI
 *    |- NI_HV_GLB_INTERRUPT.int_stat_ptp
 *        |- NI_HV_GLB_PTP_INTERRUPT.port#_tx_timestamp_rdy_intl
 *
 *********************************************************/

void aal_ptp_tx_timestamp_intr_enable(ca_boolean_t enable)
{
	NI_HV_GLB_PTP_INTERRUPTE_t ptp_intr_enable;
	uint8_t enable_bit = (enable == CA_TRUE) ? 1 : 0;

	ptp_intr_enable.wrd = CA_NE_REG_READ(NI_HV_GLB_PTP_INTERRUPTE);
	ptp_intr_enable.bf.port0_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port1_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port2_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port3_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port4_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port5_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port6_tx_timestamp_rdy_intIE = enable_bit;
	CA_NE_REG_WRITE(ptp_intr_enable.wrd, NI_HV_GLB_PTP_INTERRUPTE);

	{
		NI_HV_GLB_INTERRUPTE_t ni_glb_intr_enable;
		ni_glb_intr_enable.wrd = CA_NE_REG_READ(NI_HV_GLB_INTERRUPTE);
		if (enable) {
			ni_glb_intr_enable.bf.int_stat_ptpE = 1;
		}
		else {
			ni_glb_intr_enable.bf.int_stat_ptpE = 0;
		}
		CA_NE_REG_WRITE(ni_glb_intr_enable.wrd, NI_HV_GLB_INTERRUPTE);
	}
}

/* Get the Tx timestamp interrupt status.
 *
 * Return:
 *   The bitmap of ports that trigger Tx timestamp interrupt.
 *   Bit 0 is port 0, bit 1 is port 1, and so on.
 */
uint32_t aal_ptp_tx_timestamp_intr_status_get(void)
{
	return (CA_NE_REG_READ(NI_HV_GLB_PTP_INTERRUPT) & 0xFF);
}

/* Clear the Tx timestamp interrupt status.
 *
 * port_bitmap: Bitmap of ports. Bit 0 is port 0, bit 1 is port 1, and so on.
 *              Setting bit value to 1 means clearing its port interrupt status.
 */
void aal_ptp_tx_timestamp_intr_status_clear(ca_uint32_t port_bitmap)
{
	/* write 1 to clear */
	CA_NE_REG_WRITE(port_bitmap & 0x7F, NI_HV_GLB_PTP_INTERRUPT);
	CA_NE_REG_WRITE(BIT(27), NI_HV_GLB_INTERRUPT);
}

ca_status_t aal_ptp_tx_timestamp_record(ca_port_id_t port_id)
{
	uint32_t nsec;
	uint32_t port = PORT_ID(port_id);

	if (port > 6) return CA_E_PARAM;
	nsec = CA_NE_REG_READ(NI_HV_GLB_P0_PTP_TX_TIMESTAMP + port * NI_HV_GLB_P0_PTP_TX_TIMESTAMP_STRIDE);
	nsec = nsec * NSECS_PER_PTP_TX_TIMESTAMP;

	tx_timestamp[port] = ((uint64_t) atomic_read(&ptp_second)) * 1000000000 + nsec;
	return CA_E_OK;
}

ca_status_t aal_ptp_tx_timestamp_get(ca_port_id_t port_id, ca_uint64_t *time)
{
	uint32_t port = PORT_ID(port_id);

	if (port > 6) return CA_E_PARAM;

	if (time == NULL) return CA_E_PARAM;
	*time = tx_timestamp[port];

	return CA_E_OK;
}

/*********************************************************
 * PTP Timer - access/adjust directly
 *********************************************************/

ca_status_t aal_ptp_timer_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint64_t	time)
{
	uint32_t sec, nsec;

	sec = div_u64_rem(time, 1000000000, &nsec);

	atomic_set(&ptp_second, (int) sec);
	CA_NI_REG_WRITE(nsec, PTP_TIMER);

	return CA_E_OK;
}

ca_status_t aal_ptp_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint64_t	*time)
{
	PTP_TIMER_t ptp_timer;
	ptp_timer.wrd = CA_NE_PTP_REG_READ(PTP_TIMER);

	if (time == NULL) return CA_E_PARAM;

	*time = (uint64_t) ((uint64_t) atomic_read(&ptp_second)) * NSEC_PER_SEC;
	*time += ptp_timer.bf.ptp_hw_ts;

	return CA_E_OK;
}

/* PTP clock period is similar to PTP clock frequency */
ca_status_t aal_ptp_clock_period_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint32_t	clock_period)
{
	CA_NE_PTP_REG_WRITE(clock_period, PTP_TIMER_CFG_0);
	return CA_E_OK;
}

ca_status_t aal_ptp_clock_period_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint32_t	*clock_period)
{
	if (clock_period == NULL) return CA_E_PARAM;

	*clock_period = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_0);
	return CA_E_OK;
}

/*********************************************************
 * PTP PPS Timer
 *********************************************************/

ca_status_t aal_ptp_pps_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint32_t	*time)
{
	PTP_TS_ON_PPS_t ptp_timer_pps;

	if (time == NULL) return CA_E_PARAM;

	ptp_timer_pps.wrd = CA_NE_PTP_REG_READ(PTP_TS_ON_PPS);
	*time = ptp_timer_pps.bf.pps_timestamp_sample;

	return CA_E_OK;
}

/*********************************************************
 * PTP Local Timer config
 * Registers exist in Saturn SFU and G3HGU Saturon core only
 *********************************************************/

ca_status_t aal_ptp_local_timer_cfg_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_mask_t	mask,
	CA_IN	aal_ptp_local_timer_cfg_t	config)
{
	PTP_LOCAL_TIMER_CFG_0_t ptp_local_timer_cfg0;
	PTP_LOCAL_TIMER_CFG_1_t ptp_local_timer_cfg1;
	PTP_LOCAL_TIMER_CFG_2_t ptp_local_timer_cfg2;
	PTP_LOCAL_TIMER_CFG_3_t ptp_local_timer_cfg3;
	PTP_LOCAL_TIMER_t ptp_local_timer;

	memset((void *)&ptp_local_timer_cfg0, 0, sizeof(PTP_LOCAL_TIMER_CFG_0_t));
	memset((void *)&ptp_local_timer_cfg1, 0, sizeof(PTP_LOCAL_TIMER_CFG_1_t));
	memset((void *)&ptp_local_timer_cfg2, 0, sizeof(PTP_LOCAL_TIMER_CFG_2_t));
	memset((void *)&ptp_local_timer_cfg3, 0, sizeof(PTP_LOCAL_TIMER_CFG_3_t));
	memset((void *)&ptp_local_timer, 0, sizeof(PTP_LOCAL_TIMER_t));

	ptp_local_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_0);
	ptp_local_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_1);
	ptp_local_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_2);
	ptp_local_timer_cfg3.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_3);
	ptp_local_timer.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER);

	if (mask.local_timer_cfg0_mask.s.local_tmr_match_val) {
		ptp_local_timer_cfg0.bf.local_tmr_match_val = config.local_tmr_match_val;
	}

	if (mask.local_timer_cfg0_mask.s.local_tmr_match_load) {
		ptp_local_timer_cfg0.bf.local_tmr_match_load = config.local_tmr_match_load;
	}

	if (mask.local_timer_cfg1_mask.s.local_tmr_load_val) {
		ptp_local_timer_cfg1.bf.local_tmr_load_val = config.local_tmr_load_val;
	}

	if (mask.local_timer_cfg1_mask.s.local_tmr_en) {
		ptp_local_timer_cfg1.bf.local_tmr_en = config.local_tmr_en;
	}

	if (mask.local_timer_cfg2_mask.s.local_period) {
		ptp_local_timer_cfg2.bf.local_period = config.local_period;
	}

	if (mask.local_timer_cfg3_mask.s.local_ts_subcnt) {
		ptp_local_timer_cfg3.bf.local_ts_subcnt = config.local_ts_subcnt;
	}

	if (mask.local_timer_mask.s.local_hw_ts) {
		ptp_local_timer.bf.local_hw_ts = config.local_hw_ts;
	}

	CA_NE_REG_WRITE(ptp_local_timer_cfg0.wrd, PTP_LOCAL_TIMER_CFG_0);
	CA_NE_REG_WRITE(ptp_local_timer_cfg1.wrd, PTP_LOCAL_TIMER_CFG_1);
	CA_NE_REG_WRITE(ptp_local_timer_cfg2.wrd, PTP_LOCAL_TIMER_CFG_2);
	CA_NE_REG_WRITE(ptp_local_timer_cfg3.wrd, PTP_LOCAL_TIMER_CFG_3);
	CA_NE_REG_WRITE(ptp_local_timer.wrd, PTP_LOCAL_TIMER);

	return CA_E_OK;
}

ca_status_t aal_ptp_local_timer_cfg_get(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_t	*config)
{
	PTP_LOCAL_TIMER_CFG_0_t ptp_local_timer_cfg0;
	PTP_LOCAL_TIMER_CFG_1_t ptp_local_timer_cfg1;
	PTP_LOCAL_TIMER_CFG_2_t ptp_local_timer_cfg2;
	PTP_LOCAL_TIMER_CFG_3_t ptp_local_timer_cfg3;
	PTP_LOCAL_TIMER_t ptp_local_timer;

	memset((void *)&ptp_local_timer_cfg0, 0, sizeof(PTP_LOCAL_TIMER_CFG_0_t));
	memset((void *)&ptp_local_timer_cfg1, 0, sizeof(PTP_LOCAL_TIMER_CFG_1_t));
	memset((void *)&ptp_local_timer_cfg2, 0, sizeof(PTP_LOCAL_TIMER_CFG_2_t));
	memset((void *)&ptp_local_timer_cfg3, 0, sizeof(PTP_LOCAL_TIMER_CFG_3_t));
	memset((void *)&ptp_local_timer, 0, sizeof(PTP_LOCAL_TIMER_t));

	ptp_local_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_0);
	ptp_local_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_1);
	ptp_local_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_2);
	ptp_local_timer_cfg3.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_3);
	ptp_local_timer.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER);

	config->local_tmr_match_val = ptp_local_timer_cfg0.bf.local_tmr_match_val;
	config->local_tmr_match_load= ptp_local_timer_cfg0.bf.local_tmr_match_load;
	config->local_tmr_load_val = ptp_local_timer_cfg1.bf.local_tmr_load_val;
	config->local_tmr_en = ptp_local_timer_cfg1.bf.local_tmr_en;
	config->local_period = ptp_local_timer_cfg2.bf.local_period;
	config->local_ts_subcnt = ptp_local_timer_cfg3.bf.local_ts_subcnt;
	config->local_hw_ts = ptp_local_timer.bf.local_hw_ts;
	return CA_E_OK;
}

