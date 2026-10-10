#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_ptp_sfu.h"

/*********************************************************
 * PTP Local Timer config
 * Registers exist in Saturn SFU and G3HGU Saturon core only
 *********************************************************/

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
ca_status_t aal_ptp_local_timer_cfg_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_mask_t	mask,
	CA_IN	aal_ptp_local_timer_cfg_t	config)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
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
#elif  defined(CONFIG_ARCH_CORTINA_G3HGU)
	SATURN_PTP_LOCAL_TIMER_CFG_0_t ptp_local_timer_cfg0;
	SATURN_PTP_LOCAL_TIMER_CFG_1_t ptp_local_timer_cfg1;
	SATURN_PTP_LOCAL_TIMER_CFG_2_t ptp_local_timer_cfg2;
	SATURN_PTP_LOCAL_TIMER_CFG_3_t ptp_local_timer_cfg3;
	SATURN_PTP_LOCAL_TIMER_t ptp_local_timer;

	memset((void *)&ptp_local_timer_cfg0, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_0_t));
	memset((void *)&ptp_local_timer_cfg1, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_1_t));
	memset((void *)&ptp_local_timer_cfg2, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_2_t));
	memset((void *)&ptp_local_timer_cfg3, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_3_t));
	memset((void *)&ptp_local_timer, 0, sizeof(SATURN_PTP_LOCAL_TIMER_t));
#endif

	if (0 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		ptp_local_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_0);
		ptp_local_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_1);
		ptp_local_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_2);
		ptp_local_timer_cfg3.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_3);
		ptp_local_timer.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER);
#endif
	}else if (1 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
		ptp_local_timer_cfg0.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_0);
		ptp_local_timer_cfg1.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_1);
		ptp_local_timer_cfg2.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_2);
		ptp_local_timer_cfg3.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_3);
		ptp_local_timer.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER);
#endif
	}
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

	if (0 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		CA_NE_REG_WRITE(ptp_local_timer_cfg0.wrd, PTP_LOCAL_TIMER_CFG_0);
		CA_NE_REG_WRITE(ptp_local_timer_cfg1.wrd, PTP_LOCAL_TIMER_CFG_1);
		CA_NE_REG_WRITE(ptp_local_timer_cfg2.wrd, PTP_LOCAL_TIMER_CFG_2);
		CA_NE_REG_WRITE(ptp_local_timer_cfg3.wrd, PTP_LOCAL_TIMER_CFG_3);
		CA_NE_REG_WRITE(ptp_local_timer.wrd, PTP_LOCAL_TIMER);
#endif
	}else if (1 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
		CA_8279_NE_REG_WRITE(ptp_local_timer_cfg0.wrd, PTP_LOCAL_TIMER_CFG_0);
		CA_8279_NE_REG_WRITE(ptp_local_timer_cfg1.wrd, PTP_LOCAL_TIMER_CFG_1);
		CA_8279_NE_REG_WRITE(ptp_local_timer_cfg2.wrd, PTP_LOCAL_TIMER_CFG_2);
		CA_8279_NE_REG_WRITE(ptp_local_timer_cfg3.wrd, PTP_LOCAL_TIMER_CFG_3);
		CA_8279_NE_REG_WRITE(ptp_local_timer.wrd, PTP_LOCAL_TIMER);
#endif
	}
	return CA_E_OK;
}

ca_status_t aal_ptp_local_timer_cfg_get(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_ptp_local_timer_cfg_t	*config)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

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
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
	SATURN_PTP_LOCAL_TIMER_CFG_0_t ptp_local_timer_cfg0;
	SATURN_PTP_LOCAL_TIMER_CFG_1_t ptp_local_timer_cfg1;
	SATURN_PTP_LOCAL_TIMER_CFG_2_t ptp_local_timer_cfg2;
	SATURN_PTP_LOCAL_TIMER_CFG_3_t ptp_local_timer_cfg3;
	SATURN_PTP_LOCAL_TIMER_t ptp_local_timer;

	memset((void *)&ptp_local_timer_cfg0, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_0_t));
	memset((void *)&ptp_local_timer_cfg1, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_1_t));
	memset((void *)&ptp_local_timer_cfg2, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_2_t));
	memset((void *)&ptp_local_timer_cfg3, 0, sizeof(SATURN_PTP_LOCAL_TIMER_CFG_3_t));
	memset((void *)&ptp_local_timer, 0, sizeof(SATURN_PTP_LOCAL_TIMER_t));
#endif
	if (0 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		ptp_local_timer_cfg0.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_0);
		ptp_local_timer_cfg1.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_1);
		ptp_local_timer_cfg2.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_2);
		ptp_local_timer_cfg3.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER_CFG_3);
		ptp_local_timer.wrd = CA_NE_PTP_REG_READ(PTP_LOCAL_TIMER);
#endif
	}else if (1 == device_id) {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
		ptp_local_timer_cfg0.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_0);
		ptp_local_timer_cfg1.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_1);
		ptp_local_timer_cfg2.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_2);
		ptp_local_timer_cfg3.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER_CFG_3);
		ptp_local_timer.wrd = CA_8279_NE_REG_READ(PTP_LOCAL_TIMER);
#endif
	}
	config->local_tmr_match_val = ptp_local_timer_cfg0.bf.local_tmr_match_val;
	config->local_tmr_match_load= ptp_local_timer_cfg0.bf.local_tmr_match_load;
	config->local_tmr_load_val = ptp_local_timer_cfg1.bf.local_tmr_load_val;
	config->local_tmr_en = ptp_local_timer_cfg1.bf.local_tmr_en;
	config->local_period = ptp_local_timer_cfg2.bf.local_period;
	config->local_ts_subcnt = ptp_local_timer_cfg3.bf.local_ts_subcnt;
	config->local_hw_ts = ptp_local_timer.bf.local_hw_ts;
	return CA_E_OK;
}

#endif /* defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) */

