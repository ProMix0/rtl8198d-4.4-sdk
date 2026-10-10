#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_ptp.h"  

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
#define NSECS_PER_PTP_TX_TIMESTAMP	(16)
uint64_t tx_timestamp[NUM_PORT] = {0, 0, 0, 0, 0, 0, 0, 0}; /* recorded Tx timestamp for port 0~7 */
atomic_t ptp_second; /* CA PTP timer provides lower 32-bit time (nanosecond), and this records higher 32-bit time (second). */
atomic_t tod_second; /* CA MPCP timer provides lower 32-bit time (nanosecond), and this records higher 32-bit time (second). */

/*********************************************************
 * PTP PPS config
 *********************************************************/

ca_status_t aal_ptp_pps_cfg_set(
    CA_IN  ca_device_id_t    device_id,
    CA_IN  aal_ptp_pps_cfg_mask_t   mask,
    CA_IN  aal_ptp_pps_cfg_t    config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_PTP_PPS_CFG_0_t ptp_pps_cfg0;
	NI_HV_PTP_PPS_CFG_3_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_3);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    PTP_PPS_CFG_0_t ptp_pps_cfg0 ;
    PTP_PPS_CFG_1_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_1);
#endif
    
    if(mask.cfg0_mask.s.ptp_en){
        ptp_pps_cfg0.bf.ptp_en = config.ptp_en;
    }

    if(mask.cfg0_mask.s.pps_in_en){
        ptp_pps_cfg0.bf.pps_in_en = config.pps_in_en;
    }

    if(mask.cfg0_mask.s.pps_edge_sel_sample){
        ptp_pps_cfg0.bf.pps_edge_sel_sample = config.pps_edge_sel_sample;
    }

    if(mask.cfg0_mask.s.pps_edge_sel_load){
        ptp_pps_cfg0.bf.pps_edge_sel_load = config.pps_edge_sel_load;
    }

    if(mask.cfg0_mask.s.pps_load_en){
        ptp_pps_cfg0.bf.pps_load_en = config.pps_load_en;
    }

    if(mask.cfg0_mask.s.pps_out_en){
        ptp_pps_cfg0.bf.pps_out_en = config.pps_out_en;
    }

    if(mask.cfg0_mask.s.pps_width){
        ptp_pps_cfg0.bf.pps_width = config.pps_width;
    }

    if(mask.cfg1_mask.s.s_pps){
        ptp_pps_cfg1.bf.s_pps = config.s_pps;
    }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	CA_NE_REG_WRITE(ptp_pps_cfg0.wrd, NI_HV_PTP_PPS_CFG_0);
	CA_NE_REG_WRITE(ptp_pps_cfg1.wrd, NI_HV_PTP_PPS_CFG_3);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        CA_NE_REG_WRITE(ptp_pps_cfg0.wrd,PTP_PPS_CFG_0);
        CA_NE_REG_WRITE(ptp_pps_cfg1.wrd,PTP_PPS_CFG_1);
#endif        

    return CA_E_OK;
}

ca_status_t aal_ptp_pps_cfg_get(
    CA_IN  ca_device_id_t    device_id,
    CA_OUT aal_ptp_pps_cfg_t   *config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_PTP_PPS_CFG_0_t ptp_pps_cfg0;
	NI_HV_PTP_PPS_CFG_3_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_3);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    PTP_PPS_CFG_0_t ptp_pps_cfg0 ;
    PTP_PPS_CFG_1_t ptp_pps_cfg1;

	ptp_pps_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_0);
	ptp_pps_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_PPS_CFG_1);
#endif

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
    CA_IN  ca_device_id_t    device_id,
    CA_IN  aal_ptp_timer_cfg_mask_t mask,
    CA_IN  aal_ptp_timer_cfg_t config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_PTP_PPS_CFG_1_t ptp_timer_cfg0;
	NI_HV_PTP_PPS_CFG_2_t ptp_timer_cfg1;
	NI_HV_PTP_PPS_CFG_5_t ptp_timer_cfg2;

	ptp_timer_cfg0.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_1);
	ptp_timer_cfg1.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_2);
	ptp_timer_cfg2.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_5);

	if (mask.cf0_mask.s.ptp_clk_period) {
		ptp_timer_cfg0.bf.ptp_sum_val = config.ptp_clk_period;
	}

	if (mask.cf1_mask.s.ptp_tmr_load_val) {
		ptp_timer_cfg1.bf.pps_timestamp_load = config.ptp_tmr_load_val;
	}

	if (mask.cf2_mask.s.ptp_tmr_match_val) {
		ptp_timer_cfg2.bf.load_matching_tmr = config.ptp_tmr_match_val;
	}

	if (mask.cf2_mask.s.ptp_tmr_match_load) {
		ptp_timer_cfg2.bf.load_matching_tmr_en = config.ptp_tmr_match_load;
	}

	CA_NE_REG_WRITE(ptp_timer_cfg0.wrd, NI_HV_PTP_PPS_CFG_1);
	CA_NE_REG_WRITE(ptp_timer_cfg1.wrd, NI_HV_PTP_PPS_CFG_2);
	CA_NE_REG_WRITE(ptp_timer_cfg2.wrd, NI_HV_PTP_PPS_CFG_5);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    PTP_TIMER_CFG_0_t ptp_timer_cfg0;
    PTP_TIMER_CFG_1_t ptp_timer_cfg1;
    PTP_TIMER_CFG_2_t ptp_timer_cfg2;

	ptp_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_0);
	ptp_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_1);
	ptp_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_2);

    if(mask.cf0_mask.s.ptp_clk_period){
        ptp_timer_cfg0.bf.ptp_clk_period = config.ptp_clk_period;
    }

    if(mask.cf1_mask.s.ptp_tmr_load_val){
        ptp_timer_cfg1.bf.ptp_tmr_load_val = config.ptp_tmr_load_val;
    }

    if(mask.cf2_mask.s.ptp_tmr_match_val){
        ptp_timer_cfg2.bf.ptp_tmr_match_val = config.ptp_tmr_match_val;
    }

    if(mask.cf2_mask.s.pon_tmr_match_load){
        ptp_timer_cfg2.bf.pon_tmr_match_load = config.pon_tmr_match_load;
    }

    if(mask.cf2_mask.s.ptp_tmr_match_load){
        ptp_timer_cfg2.bf.ptp_tmr_match_load = config.ptp_tmr_match_load;
    }

        CA_NE_REG_WRITE(ptp_timer_cfg0.wrd,PTP_TIMER_CFG_0);
        CA_NE_REG_WRITE(ptp_timer_cfg1.wrd,PTP_TIMER_CFG_1);
        CA_NE_REG_WRITE(ptp_timer_cfg2.wrd,PTP_TIMER_CFG_2);
#endif        

    return CA_E_OK;
}

ca_status_t aal_ptp_timer_cfg_get(
    CA_IN  ca_device_id_t    device_id,
    CA_IN  aal_ptp_timer_cfg_t *config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_PTP_PPS_CFG_1_t ptp_timer_cfg0;
	NI_HV_PTP_PPS_CFG_2_t ptp_timer_cfg1;
	NI_HV_PTP_PPS_CFG_5_t ptp_timer_cfg2;

	ptp_timer_cfg0.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_1);
	ptp_timer_cfg1.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_2);
	ptp_timer_cfg2.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_5);

	config->ptp_clk_period = ptp_timer_cfg0.bf.ptp_sum_val;
	config->ptp_tmr_load_val = ptp_timer_cfg1.bf.pps_timestamp_load;
	config->ptp_tmr_match_load = ptp_timer_cfg2.bf.load_matching_tmr_en;
	config->ptp_tmr_match_val = ptp_timer_cfg2.bf.load_matching_tmr;
	config->pon_tmr_match_load = 0;
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
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
#endif

    return CA_E_OK;
}

/*********************************************************
 * PTP Timer Rollover
 *
 * These interrupts' hierarchy in G3/G3HGU/G3Lite:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.NI
 *    |- NI_HV_GLB_INTERRUPT.int_stat_ptp
 *        |- NI_HV_GLB_PTP_INTERRUPT.timer_rollover_intI
 *
 * These interrupts' hierarchy in Saturn SFU:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.PTP
 *    |- PTP_INTERRUPT.timer_rollover_intI
 *
 *********************************************************/

void aal_ptp_timer_rollover_intr_enable(ca_boolean_t enable)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_GLB_PTP_INTERRUPTE_t ptp_intr_enable;
	ptp_intr_enable.wrd = CA_NE_PTP_REG_READ(NI_HV_GLB_PTP_INTERRUPTE);
	ptp_intr_enable.bf.timer_rollover_intIE = (enable) ? 1 : 0;
	CA_NE_PTP_REG_WRITE(ptp_intr_enable.wrd, NI_HV_GLB_PTP_INTERRUPTE);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	PTP_INTENABLE_t	ptp_intr_enable;
	ptp_intr_enable.wrd = CA_NE_PTP_REG_READ(PTP_INTENABLE);
	ptp_intr_enable.bf.timer_rollover_IE = (enable) ? 1 : 0;
	CA_NE_PTP_REG_WRITE(ptp_intr_enable.wrd, PTP_INTENABLE);
#endif
}

bool aal_ptp_timer_rollover_intr_status_get(void)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_GLB_PTP_INTERRUPT_t status;
	status.wrd = CA_NE_PTP_REG_READ(NI_HV_GLB_PTP_INTERRUPT);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	PTP_INTERRUPT_t status;
	status.wrd = CA_NE_PTP_REG_READ(PTP_INTERRUPT);
#endif
	return (status.bf.timer_rollover_intI == 1);
}

void aal_ptp_timer_rollover_intr_status_clear(void)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_GLB_PTP_INTERRUPT_t status;
	status.wrd = 0;
	status.bf.timer_rollover_intI = 1;
	CA_NE_PTP_REG_WRITE(status.wrd, NI_HV_GLB_PTP_INTERRUPT);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	PTP_INTERRUPT_t status;
	status.wrd = 0;
	status.bf.timer_rollover_intI = 1;
	CA_NE_PTP_REG_WRITE(status.wrd, PTP_INTERRUPT);
#endif        
}

void aal_ptp_timer_rollover_triggered(void)
{
	atomic_inc(&ptp_second);
}

void aal_ptp_timer_sync_mpcp_time_triggered(void)
{
	aal_ptp_timer_pon_match_load_clear();
	aal_ptp_timer_set_sec(0, atomic_read(&tod_second));
}

void aal_ptp_timer_record_mpcp_tod(int sec)
{
	atomic_set(&tod_second, sec);
}

/*********************************************************
 * PTP Tx Timestamp
 *
 * These interrupts' hierarchy in G3/G3HGU/G3Lite:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.NI
 *    |- NI_HV_GLB_INTERRUPT.int_stat_ptp
 *        |- NI_HV_GLB_PTP_INTERRUPT.port#_tx_timestamp_rdy_intI
 *
 * These interrupts' hierarchy in Saturn SFU:
 *   GLOBAL_NETWORK_ENGINE_INTERRUPT_0.NI
 *    |- NI_HV_GLB_INTERRUPT.int_stat_ptp
 *        |- NI_HV_GLB_PTP_INTERRUPT.port#_tx_timestamp_rdy_intI
 *           (timer_rollover_intI, pps_out_falledge_intI, pps_event_intI are invalid in ASIC)
 *
 *********************************************************/

void aal_ptp_tx_timestamp_intr_enable(ca_boolean_t enable)
{
	NI_HV_GLB_PTP_INTERRUPTE_t ptp_intr_enable;
	uint8_t enable_bit = (enable == CA_TRUE) ? 1 : 0;

	ptp_intr_enable.wrd = CA_NE_REG_READ(NI_HV_GLB_PTP_INTERRUPTE);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	ptp_intr_enable.bf.port0_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port1_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port2_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port3_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port4_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port5_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port6_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port7_tx_timestamp_rdy_intIE = enable_bit;
#elif defined(CONFIG_ARCH_CORTINA_G3LITE)
	ptp_intr_enable.bf.port0_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port1_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port2_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port3_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port4_tx_timestamp_rdy_intIE = enable_bit;
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	ptp_intr_enable.bf.port3_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port4_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port5_tx_timestamp_rdy_intIE = enable_bit;
	ptp_intr_enable.bf.port6_tx_timestamp_rdy_intIE = enable_bit;
#endif
	CA_NE_REG_WRITE(ptp_intr_enable.wrd, NI_HV_GLB_PTP_INTERRUPTE);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)	// TODO: move to device tree?
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
#endif
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
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	CA_NE_REG_WRITE(port_bitmap & 0xFF, NI_HV_GLB_PTP_INTERRUPT);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	CA_NE_REG_WRITE(port_bitmap & 0x78, NI_HV_GLB_PTP_INTERRUPT);
	CA_NE_REG_WRITE(BIT(27), NI_HV_GLB_INTERRUPT);
#endif
    }

ca_status_t aal_ptp_tx_timestamp_record(ca_port_id_t port_id)
{
	uint32_t nsec;
	uint32_t port = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (port > 7) return CA_E_PARAM;
	nsec = CA_NE_REG_READ(NI_HV_GLB_P0_PTP_TX_TIMESTAMP + port * NI_HV_GLB_P0_PTP_TX_TIMESTAMP_STRIDE);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	if ((port < 3) || (port > 6)) return CA_E_PARAM;
	nsec = CA_NE_REG_READ(NI_HV_GLB_P3_PTP_TX_TIMESTAMP + (port - 3) * NI_HV_GLB_P3_PTP_TX_TIMESTAMP_STRIDE);
	nsec = nsec * NSECS_PER_PTP_TX_TIMESTAMP;
#endif

	tx_timestamp[port] = ((uint64_t) atomic_read(&ptp_second)) * 1000000000 + nsec;
	return CA_E_OK;
    }

ca_status_t aal_ptp_tx_timestamp_get(ca_port_id_t port_id, ca_uint64_t *time)
{
	uint32_t port = PORT_ID(port_id);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (port > 7) return CA_E_PARAM;
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	if ((port < 3) || (port > 6)) return CA_E_PARAM;
#endif

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
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)        
	CA_NI_REG_WRITE(nsec, PTP_TIMER);
#endif        

	return CA_E_OK;
}

ca_status_t aal_ptp_timer_set_sec(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t sec)
{
	atomic_set(&ptp_second, (int) sec);
	return CA_E_OK;
}

ca_status_t aal_ptp_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint64_t	*time)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	NI_HV_PTP_TIMER_t ptp_timer;
	ptp_timer.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_TIMER);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	PTP_TIMER_t ptp_timer;
	ptp_timer.wrd = CA_NE_PTP_REG_READ(PTP_TIMER);
#endif

	if (time == NULL) return CA_E_PARAM;

	*time = (uint64_t) ((uint64_t) atomic_read(&ptp_second)) * NSEC_PER_SEC;
	*time += ptp_timer.bf.ptp_hw_ts;

	return CA_E_OK;
    }

ca_status_t aal_ptp_timer_get_sec(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t *sec)
{
	*sec = atomic_read(&ptp_second);
	return CA_E_OK;
}

/* PTP clock period is similar to PTP clock frequency */
ca_status_t aal_ptp_clock_period_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_uint32_t	clock_period)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	CA_NE_PTP_REG_WRITE(clock_period, NI_HV_PTP_PPS_CFG_1);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	CA_NE_PTP_REG_WRITE(clock_period, PTP_TIMER_CFG_0);
#endif
    return CA_E_OK;
}

ca_status_t aal_ptp_clock_period_get(
    CA_IN  ca_device_id_t    device_id,
	CA_OUT	ca_uint32_t	*clock_period)
{
	if (clock_period == NULL) return CA_E_PARAM;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	*clock_period = CA_NE_PTP_REG_READ(NI_HV_PTP_PPS_CFG_1);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	*clock_period = CA_NE_PTP_REG_READ(PTP_TIMER_CFG_0);
#endif
    return CA_E_OK;
}

/*********************************************************
 * PTP PPS Timer
 *********************************************************/

ca_status_t aal_ptp_pps_timer_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_uint32_t	*time)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	NI_HV_PTP_TIMER_ON_PPS_t ptp_timer_pps;
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	PTP_TS_ON_PPS_t ptp_timer_pps;
#endif

	if (time == NULL) return CA_E_PARAM;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	ptp_timer_pps.wrd = CA_NE_PTP_REG_READ(NI_HV_PTP_TIMER_ON_PPS);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	ptp_timer_pps.wrd = CA_NE_PTP_REG_READ(PTP_TS_ON_PPS);
#endif
	*time = ptp_timer_pps.bf.pps_timestamp_sample;

	return CA_E_OK;
}

/*********************************************************
 * PTP and MPCP Timer Sync
 *********************************************************/
ca_status_t aal_ptp_timer_pon_match_load_clear()
{
	aal_ptp_timer_cfg_mask_t ptp_cfg2_mask = {0};
	aal_ptp_timer_cfg_t ptp_cfg2 = {0};	
                
	ptp_cfg2_mask.cf2_mask.s.pon_tmr_match_load = 1;
	ptp_cfg2.pon_tmr_match_load = 0;

	return aal_ptp_timer_cfg_set(0, ptp_cfg2_mask, ptp_cfg2);

}

EXPORT_SYMBOL(aal_ptp_pps_cfg_set);
EXPORT_SYMBOL(aal_ptp_pps_cfg_get);
EXPORT_SYMBOL(aal_ptp_timer_cfg_set);
EXPORT_SYMBOL(aal_ptp_timer_cfg_get);
EXPORT_SYMBOL(aal_ptp_clock_period_set);
EXPORT_SYMBOL(aal_ptp_clock_period_get);
EXPORT_SYMBOL(aal_ptp_timer_get);
EXPORT_SYMBOL(aal_ptp_timer_set);
EXPORT_SYMBOL(aal_ptp_pps_timer_get);

