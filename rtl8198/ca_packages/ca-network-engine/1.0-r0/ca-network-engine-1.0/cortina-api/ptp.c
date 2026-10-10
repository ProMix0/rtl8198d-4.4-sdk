#include <linux/module.h>
#include "port.h"
#include "util_misc.h"
#include "osal_misc.h"
#include "os_core.h"
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
#include "aal_pon.h"
#include "aal_ptp_sfu.h"
#endif
#include "ptp.h"
#include "aal_ni_l2.h"
#include "aal_ptp.h"
#include "pkt_tx.h"
#include "osal_cmn.h"
#include "osal_spinlock.h"
#include "ca-scfg/scfg.h"
#include "peri.h"
#include "osal_common.h"
#include "ca_ni.h"

#if defined(CONFIG_NE_AAL_XFI)
#include "aal_xfi.h"
#endif
#include "aal_psds.h"

static ca_uint ptp_lock;
ca_int32_t g_ptp_local_clock_offset = 0;

static ca_status_t  __ptp_tmr_int_handle(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ptp_set_trigger_t	trigger,
	CA_IN	ca_ptp_local_clock_t	local_clock)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	aal_xgpn_glb_super_frame_match_val_t sfc_val;
	ca_uint64_t current_hw_sfc = 0 ,sfc2set = 0;
	ca_uint32_t tmp_sfc0 = 0;
	struct timespec tv;
	aal_ptp_timer_cfg_mask_t ptp_timer_cfg_mask;
	aal_ptp_timer_cfg_t ptp_timer_cfg;
	cap_hardware_timer_config_t tmr_cfg;
	ca_uint32_t pon_mode = 2;

	memset(&tmr_cfg, 0, sizeof(tmr_cfg));
	memset(&tv, 0, sizeof(struct timespec));
	memset(&sfc_val, 0, sizeof(aal_xgpn_glb_super_frame_match_val_t));
	memset(&ptp_timer_cfg_mask, 0, sizeof(aal_ptp_timer_cfg_mask_t));
	memset(&ptp_timer_cfg, 0, sizeof(aal_ptp_timer_cfg_t));

	sfc_val.sfc0 = trigger.start_sfc & 0xFFFFFFFF;
	if ((pon_mode >= ONU_PON_MAC_XGPON1) & (pon_mode <= ONU_PON_MAC_NGPON2))
		aal_xgpn_dsfp_sfc_get(device_id, &tmp_sfc0, &sfc_val.sfc1);
	current_hw_sfc = sfc_val.sfc1;
	current_hw_sfc = ((current_hw_sfc & 0x7FFFF) << 32) | (tmp_sfc0 & 0xFFFFFFFF);

	if (tmp_sfc0 > sfc_val.sfc0) {
		sfc_val.sfc1 = sfc_val.sfc1 + 1;
     }
	sfc2set = sfc_val.sfc1;
	sfc2set = ((sfc2set & 0x7FFFF) << 32) | (sfc_val.sfc0);

	if ((sfc2set - current_hw_sfc) < CA_PTP_SFC_GAP_MAX) {
		tv.tv_sec = ((local_clock.sec) << 2) | ((local_clock.nsec >> 30) & 0x3);
		do_settimeofday(&tv);
		ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_load = 1;
		ptp_timer_cfg.ptp_tmr_match_load = 0;
		aal_ptp_timer_cfg_set(device_id, ptp_timer_cfg_mask, ptp_timer_cfg);

		tmr_cfg.mode = CA_TIMER_MODE_DISABLE;
		tmr_cfg.threshold = 10000;
		tmr_cfg.int_handle = __ptp_tmr_int_handle;
		tmr_cfg.int_cookie = (void *) 0xFFFF;
		cap_hardware_timer_set(0, 0, &tmr_cfg);
     }
#endif
	return CA_E_OK;
}

ca_status_t ca_ptp_local_clock_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_ptp_set_trigger_t		*trigger,
	CA_IN	ca_ptp_local_clock_t		*local_clock)
{
    ca_uint64_t tmp_local_clock = 0;
    aal_ptp_pps_cfg_mask_t    ptp_pps_cfg_mask;
    aal_ptp_pps_cfg_t         ptp_pps_cfg;
    aal_ptp_timer_cfg_mask_t  ptp_timer_cfg_mask;
    aal_ptp_timer_cfg_t       ptp_timer_cfg;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint32_t               tmp_sfc0 = 0;
    aal_xgpn_glb_super_frame_match_val_t sfc_val;
    ca_uint64_t current_hw_sfc = 0 ,sfc2set = 0;
    ca_status_t ret = CA_E_OK;
	ca_uint32_t pon_mode = 2;
    struct timespec tv;
	int neg_adj = 0; 
	ca_uint32_t offset;
	ca_int32_t freq_adjust;
    memset(&sfc_val,0,sizeof(aal_xgpn_glb_super_frame_match_val_t));
    memset(&tv,0,sizeof(struct timespec));
#endif

	if ((trigger == NULL) || (local_clock == NULL))
		return CA_E_PARAM;

	if (local_clock->nsec >= 1000000000) {
		ca_printf("%s: Nanosecond value should be smaller then 1,000,000,000.\n", __func__);
		return CA_E_PARAM;
	}

    memset(&ptp_pps_cfg_mask,0,sizeof(aal_ptp_pps_cfg_mask_t));
    memset(&ptp_pps_cfg,0,sizeof(aal_ptp_pps_cfg_t));
    memset(&ptp_timer_cfg_mask,0,sizeof(aal_ptp_timer_cfg_mask_t));
    memset(&ptp_timer_cfg,0,sizeof(aal_ptp_timer_cfg_t));

//	ca_spin_lock(ptp_lock);

	if ((!trigger->mask.pps_trigger_edge) && (!trigger->mask.local_time) && (!trigger->mask.start_sfc) && (!trigger->mask.mpcp_time)) {
		tmp_local_clock = local_clock->sec * 1000000000 + local_clock->nsec;
		aal_ptp_timer_set(device_id, tmp_local_clock);
    }

	if (trigger->mask.pps_trigger_edge) {
        ptp_pps_cfg_mask.cfg0_mask.s.pps_edge_sel_load = 1;
        ptp_pps_cfg_mask.cfg0_mask.s.pps_load_en = 1;
		ptp_pps_cfg.pps_edge_sel_load = (trigger->pps_trigger_edge) ? 1 : 0;
        ptp_pps_cfg.pps_load_en = 1;
        aal_ptp_pps_cfg_set(device_id,ptp_pps_cfg_mask,ptp_pps_cfg);
    }

	if (trigger->mask.local_time) {
        /* set trigger */
        ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_load = 1;
        ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_val = 1;
        ptp_timer_cfg.ptp_tmr_match_load = 1;
		ptp_timer_cfg.ptp_tmr_match_val = trigger->local_time;
        /* set match load value */
        ptp_timer_cfg_mask.cf1_mask.s.ptp_tmr_load_val = 1;
		ptp_timer_cfg.ptp_tmr_load_val = local_clock->nsec;
        aal_ptp_timer_cfg_set(device_id,ptp_timer_cfg_mask,ptp_timer_cfg);
    }

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	pon_mode = aal_pon_mac_mode_get(device_id);

	if (trigger->mask.start_sfc) {
		if ((pon_mode < ONU_PON_MAC_XGPON1) || (pon_mode > ONU_PON_MAC_NGPON2)){
			ca_printf("PON MAC mode not correct to config SFCP\n");
//			ca_spin_unlock(ptp_lock);
			return CA_E_NOT_SUPPORT;
		}

        ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_load = 1;
		//ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_val = 1;
		ptp_timer_cfg_mask.cf1_mask.s.ptp_tmr_load_val = 1;

		sfc_val.sfc0 = trigger->start_sfc&0xFFFFFFFF;
        aal_xgpn_dsfp_sfc_get(device_id,&tmp_sfc0,&sfc_val.sfc1);
	current_hw_sfc = sfc_val.sfc1;
	current_hw_sfc = ((current_hw_sfc&0x7FFFF)<<32)|(tmp_sfc0&0xFFFFFFFF);


        if(tmp_sfc0 > sfc_val.sfc0){
            sfc_val.sfc1 = sfc_val.sfc1 + 1;
        }
	sfc2set = sfc_val.sfc1;
        sfc2set = ((sfc2set&0x7FFFF)<<32)|(sfc_val.sfc0);

		ca_printf("tmp_sfc0 is 0x%x, sfc_val.sfc0 is 0x%x, sfc_val.sfc1 is 0x%x\n",tmp_sfc0, sfc_val.sfc0, sfc_val.sfc1);

        if(sfc2set-current_hw_sfc < CA_PTP_SFC_GAP_MIN){
			ca_printf("Not enough time to handle PTP SFC GAP\n");
//			ca_spin_unlock(ptp_lock);
            return CA_E_UNAVAIL;

        }

        if(sfc2set-current_hw_sfc < CA_PTP_SFC_GAP_MAX){
            aal_xgpon_xgpn_glb_super_frame_match_val_set(device_id,sfc_val);

			ptp_timer_cfg.ptp_tmr_load_val = local_clock->nsec & 0x3FFFFFFF;
            ptp_timer_cfg.ptp_tmr_match_load = 1;
            aal_ptp_timer_cfg_set(device_id,ptp_timer_cfg_mask,ptp_timer_cfg);
			tmp_local_clock = local_clock->sec * 1000000000 + local_clock->nsec;
			aal_ptp_timer_set(device_id, tmp_local_clock);

			aal_psds_cdr_freq_offset_get(0, &offset);
			neg_adj = offset & BIT(16);
			if (neg_adj)
				offset = 0xffff - (offset & 0xffff);

			freq_adjust = offset * 100000 / 524; /* unit in ppb */
			ca_printf("freq_adj=%d (ppb), offset=%d\n", freq_adjust, offset);

			ca_ptp_local_clock_freq_adj_set(device_id, (neg_adj) ? freq_adjust : -freq_adjust);
//			do_settimeofday(&tv);
            ca_sleep(1000);

            memset(&ptp_timer_cfg_mask,0,sizeof(aal_ptp_timer_cfg_mask_t));

            ptp_timer_cfg_mask.cf2_mask.s.ptp_tmr_match_load = 1;
            ptp_timer_cfg.ptp_tmr_match_load = 0;
            aal_ptp_timer_cfg_set(device_id,ptp_timer_cfg_mask,ptp_timer_cfg);
        }else{

            cap_hardware_timer_config_t tmr_cfg;
            memset(&tmr_cfg, 0 , sizeof(tmr_cfg));

            tmr_cfg.mode = CA_TIMER_MODE_RELOAD;
            tmr_cfg.threshold = 10000;
            tmr_cfg.int_handle = __ptp_tmr_int_handle;
            tmr_cfg.int_cookie = (void *)0xFFFF;

            cap_hardware_timer_set(0, 0, &tmr_cfg);

        }
    }

	if (trigger->mask.mpcp_time) {
		if (pon_mode > ONU_PON_MAC_EPON_BI10G) {
			ca_printf("PON MAC mode not correct to config mpcp_time\n");
//			ca_spin_unlock(ptp_lock);
			return CA_E_NOT_SUPPORT;
		}
        /* set trigger */
		aal_epon_onu_mpcp_timer_match_cfg_set(0, 0, trigger->mask.mpcp_time);
        
        ptp_timer_cfg_mask.cf2_mask.s.pon_tmr_match_load = 1;
        ptp_timer_cfg.pon_tmr_match_load = 1;

        /* set match load value */
        ptp_timer_cfg_mask.cf1_mask.s.ptp_tmr_load_val = 1;
		ptp_timer_cfg.ptp_tmr_load_val = local_clock->nsec;
        aal_ptp_timer_cfg_set(device_id,ptp_timer_cfg_mask,ptp_timer_cfg);
		tmp_local_clock = local_clock->sec * 1000000000 + local_clock->nsec;

		/* Simultaneouly adjust the freq offset with OLT*/
		aal_ptp_timer_record_mpcp_tod(local_clock->sec);
		aal_psds_cdr_freq_offset_get(0, &offset);
		neg_adj = offset & BIT(16);
		if (neg_adj)
			offset = 0xffff - (offset & 0xffff);
        
		freq_adjust = offset * 100000 / 524; /* unit in ppb */
		printk("freq_adj=%d (ppb), offset=%d\n", freq_adjust, offset);
		ca_ptp_local_clock_freq_adj_set(device_id, (neg_adj) ? freq_adjust : -freq_adjust);
		
		aal_psds_prbs_intr_enable(1);
		//aal_ptp_timer_set(device_id, tmp_local_clock);
//		do_settimeofday(&tv);
    }
#endif
//	ca_spin_unlock(ptp_lock);

    return CA_E_OK;
}

ca_status_t ca_ptp_local_clock_get (
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	ca_ptp_local_clock_t	*local_clock)
{
	ca_uint64_t tmp_local_clock, offset;

	if (local_clock == NULL) return CA_E_PARAM;

	aal_ptp_timer_get(device_id, &tmp_local_clock);

	if (g_ptp_local_clock_offset < 0 ) {
		offset = -g_ptp_local_clock_offset;
		tmp_local_clock -= offset;
	} else {
		offset = g_ptp_local_clock_offset;
		tmp_local_clock += offset;
	}

    ca_div(tmp_local_clock, 1000000000, &local_clock->sec, &local_clock->nsec);
    
	return CA_E_OK;
}

ca_status_t ca_ptp_local_clock_adj_offset(
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_int32_t offset)
{
	g_ptp_local_clock_offset = offset;
	//ca_printf("%s: offset=%d\n", __func__, g_ptp_local_clock_offset);
	return CA_E_OK;
}

ca_status_t ca_ptp_local_clock_freq_adj_set (
	CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_int32_t			freq_adjustment)
{
	uint64_t adj;
	uint32_t diff, clock_period;
	int neg_adj = 0;

	if (freq_adjustment < 0) {
		neg_adj = 1;
		freq_adjustment = -freq_adjustment;
	}

	clock_period = DEFAULT_PTP_CLOCK_PERIOD;
	adj = clock_period;
	adj *= freq_adjustment;
	diff = div_u64(adj, 1000000000ULL);

	clock_period = neg_adj ? clock_period - diff : clock_period + diff;

	aal_ptp_clock_period_set(device_id, clock_period);

	/* For Saturn SFU:
	 * - set PTP_TIMER_CFG_0.ptp_clk_period.
	 *   MSB is aligned to 8ns. PTP timer runs on local 125MHz.
	 *   If local PTP clock is faster than master PTP clock, set less than 0x80000000.
	 *   If local PTP clock is slower than master PTP clock, set larger than 0x80000000.
	 *
	 * For G3:
	 * - set NI_HV_PTP_PPS_CFG_1.ptp_sum_val.
	 *   MSB is aligned to 8ns. PTP timer runs on local 125MHz.
	 *   If local PTP clock is faster than master PTP clock, set less than 0x80000000.
	 *   If local PTP clock is slower than master PTP clock, set larger than 0x80000000.
	 */

    return CA_E_OK;
}

ca_status_t ca_ptp_local_clock_freq_adj_get (
	CA_IN	ca_device_id_t		device_id,
	CA_OUT	ca_int32_t			*freq_adjustment)
{
	uint64_t adj;
	uint32_t clock_period;
	int neg_adj = 0;

	if (freq_adjustment == NULL) return CA_E_PARAM;

	aal_ptp_clock_period_get(device_id, &clock_period);
	if (clock_period >= DEFAULT_PTP_CLOCK_PERIOD) {
		adj = clock_period - DEFAULT_PTP_CLOCK_PERIOD;
	} else {
		adj = DEFAULT_PTP_CLOCK_PERIOD - clock_period;
		neg_adj = 1;
}

	adj *= 1000000000ULL;
	*freq_adjustment = div_u64(adj, (uint64_t) DEFAULT_PTP_CLOCK_PERIOD);
	if (neg_adj)
		*freq_adjustment = -(*freq_adjustment);

    return CA_E_OK;
}

ca_status_t ca_ptp_port_sync_tx_time_get (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
	CA_OUT	ca_ptp_local_clock_t		*hw_sync_tx_time)
{
	ca_status_t ret;
	ca_uint64_t hw_tx_time;

	if (hw_sync_tx_time == NULL) return CA_E_PARAM;

	ret = aal_ptp_tx_timestamp_get(port, &hw_tx_time);
	if (ret != CA_E_OK)
		return ret;

	ca_div(hw_tx_time, 1000000000, &hw_sync_tx_time->sec, &hw_sync_tx_time->nsec);

    return CA_E_OK;
}

ca_status_t ca_syncE_clock_source_port_set (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
    CA_IN	ca_boolean_t		enable)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_xfi_cfg_mask_t xfi_cfg_mask;
	aal_xfi_cfg_t xfi_cfg;
	aal_psds_cfg_mask_t psds_cfg_mask;
	aal_psds_cfg_t psds_cfg;
	ca_uint32_t port_id = PORT_ID(port);

	if ((port_id >= AAL_NI_GE_PORT_START) && (port_id <= AAL_NI_GE_PORT_END)) {

		memset(&psds_cfg, 0, sizeof(psds_cfg));
		psds_cfg_mask.data32 = 0;

		psds_cfg.ckgphy_sel = (enable) ? 1 : 0; /* 0: local XTAL clock, 1: recovering clock */
		psds_cfg_mask.s.ckgphy_sel = 1;

		aal_psds_cfg_set(0, psds_cfg_mask, &psds_cfg);

	} else if ((port_id >= AAL_NI_XE_START) && (port_id <= AAL_NI_XE_END)) {

		memset(&xfi_cfg, 0, sizeof(xfi_cfg));
		xfi_cfg_mask.data32 = 0;

		xfi_cfg.ckref_sel = (enable) ? 0 : 1; /* 0: PON recovering clock, 1: local XTAL clock */
		xfi_cfg.clkout_en = (enable) ? 0 : 1;
		xfi_cfg.ckref_test_en = (enable) ? 0 : 1;
		xfi_cfg_mask.s.ckref_sel = 1;
		xfi_cfg_mask.s.clkout_en = 1;
		xfi_cfg_mask.s.ckref_test_en = 1;

		aal_xfi_cfg_set(0, port_id - AAL_NI_XE_START, xfi_cfg_mask, &xfi_cfg);

	} else {
		return CA_E_PARAM;
	}

	return CA_E_OK;
#else
	return CA_E_NOT_SUPPORT;
#endif
}

ca_status_t ca_syncE_clock_source_port_get (
    CA_IN	ca_device_id_t		device_id,
    CA_IN	ca_port_id_t		port,
	CA_OUT	ca_boolean_t			*enable)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_xfi_cfg_mask_t xfi_cfg_mask;
	aal_xfi_cfg_t xfi_cfg;
	aal_psds_cfg_mask_t psds_cfg_mask;
	aal_psds_cfg_t psds_cfg;
	ca_uint32_t port_id = PORT_ID(port);

	if (enable == NULL) return CA_E_PARAM;

	if ((port_id >= AAL_NI_GE_PORT_START) && (port_id <= AAL_NI_GE_PORT_END)) {

		aal_psds_cfg_get(0, &psds_cfg);

		*enable = (psds_cfg.ckgphy_sel == 1) ? CA_TRUE : CA_FALSE;

	} else if ((port_id >= AAL_NI_XE_START) && (port_id <= AAL_NI_XE_END)) {

		aal_xfi_cfg_get(0, port_id - AAL_NI_XE_START, &xfi_cfg);

		*enable = (xfi_cfg.ckref_sel == 0) ? CA_TRUE : CA_FALSE;

	} else {
		return CA_E_PARAM;
	}

	return CA_E_OK;
#else
	return CA_E_NOT_SUPPORT;
#endif
}

ca_status_t ca_tod_pps_tx_config_set(
    CA_IN	ca_device_id_t		device_id,
	CA_IN	ca_tod_pps_config_t		*config)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ptp_pps_cfg_mask_t ptp_pps_cfg_mask;
    aal_ptp_pps_cfg_t  ptp_pps_cfg;
    aal_per_tod_mem_cfg_mask_t per_tod_mem_cfg_mask;
    aal_per_tod_mem_cfg_t per_tod_mem_cfg;
    aal_epon_onu_1pps_tod_cfg_mask_t onu_1pps_tod_cfg_mask;
    aal_epon_onu_1pps_tod_cfg_t onu_1pps_tod_cfg;
    aal_xgpn_glb_super_frame_match_val_t xgpn_glb_super_frame_match_val;
    ca_uint32_t tmp_sfc0=0,tmp_sfc1=0;
	ca_uint32_t pon_mode = 2;

	if (config == NULL) return CA_E_PARAM;

    memset(&ptp_pps_cfg_mask,0,sizeof(aal_ptp_pps_cfg_mask_t));
    memset(&ptp_pps_cfg,0,sizeof(aal_ptp_pps_cfg_t));
    memset(&per_tod_mem_cfg_mask,0,sizeof(aal_per_tod_mem_cfg_mask_t));
    memset(&per_tod_mem_cfg,0,sizeof(aal_per_tod_mem_cfg_t));
    memset(&onu_1pps_tod_cfg_mask,0,sizeof(aal_epon_onu_1pps_tod_cfg_mask_t));
    memset(&onu_1pps_tod_cfg,0,sizeof(aal_epon_onu_1pps_tod_cfg_t));
    memset(&xgpn_glb_super_frame_match_val,0,sizeof(aal_xgpn_glb_super_frame_match_val_t));

	if (config->mask.uart) {
       per_tod_mem_cfg_mask.per_tod_cfg_mask.s.tod_uart = 1;
		per_tod_mem_cfg.tod_uart = config->uart;
    }

	if (config->mask.mode) {
		if (PTP_TOD_TX == config->mode) {
            per_tod_mem_cfg_mask.per_tod_cfg_mask.s.tod_en = 1;
            per_tod_mem_cfg.tod_en = 1;
        }else{
            ptp_pps_cfg_mask.cfg0_mask.s.pps_out_en = 1;
            ptp_pps_cfg.pps_out_en = 1;
        }
    }

	if (config->mask.tod_msg) {
		if (config->tod_msg_len > PTP_TOD_MSG_MAX) {
            printk("\nTOD length is not support..\n");
            return CA_E_NOT_SUPPORT;
        }
        per_tod_mem_cfg_mask.per_tod_mem_mask.s.tod_bst_mem = 1;
		memcpy(per_tod_mem_cfg.tod_bst_mem, config->tod_msg_data, config->tod_msg_len);
    }

	if (config->mask.pulse_width) {
        onu_1pps_tod_cfg_mask.onu_1pps_tod_pps_wid_mask.s.pps_width = 1;
		onu_1pps_tod_cfg.pps_width = config->pulse_width;
	}

	pon_mode = aal_pon_mac_mode_get(device_id);

	if (config->mask.start_sfc) {
		if ((pon_mode < ONU_PON_MAC_XGPON1) || (pon_mode > ONU_PON_MAC_NGPON2)) {
			ca_printf("PON MAC mode not correct to config start_sfc\n");
			return CA_E_NOT_SUPPORT;
    }

		xgpn_glb_super_frame_match_val.sfc0 = config->start_sfc & 0xFFFFFFFF;
		//xgpn_glb_super_frame_match_val.sfc1 = (config->start_sfc >> 32) & 0x7FFFF;
        aal_xgpn_dsfp_sfc_get(device_id,&tmp_sfc0,&tmp_sfc1);
        if(tmp_sfc0 < xgpn_glb_super_frame_match_val.sfc0){
            tmp_sfc1=tmp_sfc1+1;
        }
        xgpn_glb_super_frame_match_val.sfc1 = tmp_sfc1&0x7FFFF;
        aal_xgpon_xgpn_glb_super_frame_match_val_set(device_id,xgpn_glb_super_frame_match_val);
    }

	if (config->mask.start_tq) {
        onu_1pps_tod_cfg_mask.onu_1pps_tod_s_tod_mask.s.s_tod = 1;
		onu_1pps_tod_cfg.s_tod = config->start_tq;
    }
    aal_ptp_pps_cfg_set(device_id,ptp_pps_cfg_mask,ptp_pps_cfg);
    aal_per_tod_mem_cfg_set(device_id,per_tod_mem_cfg_mask,per_tod_mem_cfg);
	if (pon_mode <= ONU_PON_MAC_EPON_BI10G)
    aal_epon_onu_1pps_tod_cfg_set(device_id,onu_1pps_tod_cfg_mask, onu_1pps_tod_cfg);
#endif
    return CA_E_OK;
}

ca_status_t ca_tod_pps_tx_config_get(
    CA_IN	ca_device_id_t		device_id,
		CA_OUT	ca_tod_pps_config_t		*config)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_ptp_pps_cfg_t  ptp_pps_cfg;
    aal_per_tod_mem_cfg_t per_tod_mem_cfg;
    aal_epon_onu_1pps_tod_cfg_t onu_1pps_tod_cfg;
    aal_xgpn_glb_super_frame_match_val_t xgpn_glb_super_frame_match_val;
	ca_uint32_t pon_mode = 2;

	CA_ASSERT_RET(config, CA_E_PARAM);
    memset(&ptp_pps_cfg,0,sizeof(aal_ptp_pps_cfg_t));
    memset(&per_tod_mem_cfg,0,sizeof(aal_per_tod_mem_cfg_t));
    memset(&onu_1pps_tod_cfg,0,sizeof(aal_epon_onu_1pps_tod_cfg_t));
    memset(&xgpn_glb_super_frame_match_val,0,sizeof(aal_xgpn_glb_super_frame_match_val_t));

	pon_mode = aal_pon_mac_mode_get(device_id);

    aal_per_tod_mem_cfg_get(device_id,&per_tod_mem_cfg);
	if (pon_mode <= ONU_PON_MAC_EPON_BI10G)
    aal_epon_onu_1pps_tod_cfg_get(device_id,&onu_1pps_tod_cfg);
    aal_ptp_pps_cfg_get(device_id,&ptp_pps_cfg);
	if ((pon_mode >= ONU_PON_MAC_XGPON1) & (pon_mode <= ONU_PON_MAC_NGPON2))
    aal_xgpon_xgpn_glb_super_frame_match_val_get(device_id,&xgpn_glb_super_frame_match_val);

	if (config->mask.uart) {
		config->uart = per_tod_mem_cfg.tod_uart;
    }

	if (config->mask.mode) {
        if(per_tod_mem_cfg.tod_en){
			config->mode = PTP_TOD_TX;
        }else{
			config->mode = PTP_PPS_TX;
        }
    }

	if (config->mask.start_sfc) {
		config->start_sfc = xgpn_glb_super_frame_match_val.sfc1;
		config->start_sfc = ((config->start_sfc & 0x7FFFF) << 32) | (xgpn_glb_super_frame_match_val.sfc0 & 0xFFFFFFFF);
    }

	if (config->mask.start_tq) {
		config->start_tq = onu_1pps_tod_cfg.s_tod;
    }

	if (config->mask.tod_msg) {
		memcpy(config->tod_msg_data, per_tod_mem_cfg.tod_bst_mem, config->tod_msg_len);
    }

	if (config->mask.pulse_width) {
		config->pulse_width = onu_1pps_tod_cfg.pps_width;
    }
#endif
    return CA_E_OK;
}

ca_status_t ca_ptp_init (
    CA_IN   ca_device_id_t device_id,
	CA_IN	ca_boolean_t			enable)
{
	aal_ptp_timer_rollover_intr_enable(enable);
	aal_ptp_tx_timestamp_intr_enable(enable);

    return CA_E_OK;
}

EXPORT_SYMBOL(ca_ptp_local_clock_set);
EXPORT_SYMBOL(ca_ptp_local_clock_get);
EXPORT_SYMBOL(ca_ptp_local_clock_freq_adj_set);
EXPORT_SYMBOL(ca_ptp_local_clock_freq_adj_get);
EXPORT_SYMBOL(ca_ptp_port_sync_tx_time_get);
EXPORT_SYMBOL(ca_syncE_clock_source_port_set);
EXPORT_SYMBOL(ca_syncE_clock_source_port_get);
EXPORT_SYMBOL(ca_tod_pps_tx_config_set);
EXPORT_SYMBOL(ca_tod_pps_tx_config_get);
EXPORT_SYMBOL(ca_ptp_local_clock_adj_offset);
EXPORT_SYMBOL(ca_ptp_init);

