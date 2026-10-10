#include <linux/module.h>
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_l3_ddos.h"
#include "aal_common.h"

ca_uint __aal_l3_dos_lock = 0;
#define __DOS_LOCK()	ca_spin_lock(__aal_l3_dos_lock)
#define __DOS_UNLOCK()	ca_spin_unlock(__aal_l3_dos_lock)

ca_status_t aal_dos_atk_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_fib_mask_t		mask,
	CA_IN	aal_dos_fib_t			*fib
)
{
	aal_dos_fib_t cur_fib;
	ca_uint32_t *ptr = &cur_fib;

	__DOS_LOCK();

	*ptr = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_0);
	*(ptr+1) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_1);
	*(ptr+2) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_2);
	*(ptr+3) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_3);
	*(ptr+4) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_4);
	*(ptr+5) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_5);
	*(ptr+6) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_6);
	*(ptr+7) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_7);
	*(ptr+8) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_8);

	if(mask.s.icmp_flood_pol_id)
		cur_fib.icmp_flood_pol_id = fib->icmp_flood_pol_id;

	if(mask.s.icmp_flood_th)
		cur_fib.icmp_flood_th = fib->icmp_flood_th;

	if(mask.s.icmp_flood)
		cur_fib.icmp_flood = fib->icmp_flood;

	if(mask.s.fin_flood_pol_id)
		cur_fib.fin_flood_pol_id = fib->fin_flood_pol_id;

	if(mask.s.fin_flood_th)
		cur_fib.fin_flood_th = fib->fin_flood_th;

	if(mask.s.fin_flood)
		cur_fib.fin_flood = fib->fin_flood;

	if(mask.s.syn_flood_pol_id)
		cur_fib.syn_flood_pol_id = fib->syn_flood_pol_id;

	if(mask.s.syn_flood_th)
		cur_fib.syn_flood_th = fib->syn_flood_th;

	if(mask.s.syn_flood)
		cur_fib.syn_flood = fib->syn_flood;

	if(mask.s.syn_with_data_pol_id)
		cur_fib.syn_with_data_pol_id = fib->syn_with_data_pol_id;

	if(mask.s.syn_with_data)
		cur_fib.syn_with_data = fib->syn_with_data;

	if(mask.s.udp_bomb_pol_id)
		cur_fib.udp_bomb_pol_id = fib->udp_bomb_pol_id;

	if(mask.s.udp_bomb)
		cur_fib.udp_bomb = fib->udp_bomb;

	if(mask.s.ping_of_death_pol_id)
		cur_fib.ping_of_death_pol_id = fib->ping_of_death_pol_id;

	if(mask.s.ping_of_death)
		cur_fib.ping_of_death = fib->ping_of_death;

	if(mask.s.icmp_fragment_pol_id)
		cur_fib.icmp_fragment_pol_id = fib->icmp_fragment_pol_id;

	if(mask.s.icmp_fragment)
		cur_fib.icmp_fragment = fib->icmp_fragment;

	if(mask.s.tcp_frag_error_pol_id)
		cur_fib.tcp_frag_error_pol_id = fib->tcp_frag_error_pol_id;

	if(mask.s.tcp_frag_error)
		cur_fib.tcp_frag_error = fib->tcp_frag_error;

	if(mask.s.tcp_short_hdr_pol_id)
		cur_fib.tcp_short_hdr_pol_id = fib->tcp_short_hdr_pol_id;

	if(mask.s.tcp_short_hdr)
		cur_fib.tcp_short_hdr = fib->tcp_short_hdr;

	if(mask.s.syn1024_pol_id)
		cur_fib.syn1024_pol_id = fib->syn1024_pol_id;

	if(mask.s.syn1024)
		cur_fib.syn1024 = fib->syn1024;

	if(mask.s.null_scan_pol_id)
		cur_fib.null_scan_pol_id = fib->null_scan_pol_id;

	if(mask.s.null_scan)
		cur_fib.null_scan = fib->null_scan;

	if(mask.s.xma_scan_pol_id)
		cur_fib.xma_scan_pol_id = fib->xma_scan_pol_id;

	if(mask.s.xma_scan)
		cur_fib.xma_scan = fib->xma_scan;

	if(mask.s.syn_fin_scan_pol_id)
		cur_fib.syn_fin_scan_pol_id = fib->syn_fin_scan_pol_id;

	if(mask.s.syn_fin_scan)
		cur_fib.syn_fin_scan = fib->syn_fin_scan;

	if(mask.s.blat_attacks_pol_id)
		cur_fib.blat_attacks_pol_id = fib->blat_attacks_pol_id;

	if(mask.s.blat_attacks)
		cur_fib.blat_attacks = fib->blat_attacks;

	if(mask.s.land_attacks_pol_id)
		cur_fib.land_attacks_pol_id = fib->land_attacks_pol_id;

	if(mask.s.land_attacks)
		cur_fib.land_attacks = fib->land_attacks;

	if(mask.s.smac_eq_dmac_pol_id)
		cur_fib.smac_eq_dmac_pol_id = fib->smac_eq_dmac_pol_id;

	if(mask.s.smac_eq_dmac)
		cur_fib.smac_eq_dmac = fib->smac_eq_dmac;

	if(mask.s.dos_type_en)
		cur_fib.dos_type_en = fib->dos_type_en;

	if(mask.s.dos_port_mask)
		cur_fib.dos_port_mask = fib->dos_port_mask;

	ptr = &cur_fib;

	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_0, *ptr);
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_1, *(ptr+1));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_2, *(ptr+2));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_3, *(ptr+3));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_4, *(ptr+4));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_5, *(ptr+5));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_6, *(ptr+6));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_7, *(ptr+7));
	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_8, *(ptr+8));

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_set);

ca_status_t aal_dos_atk_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_fib_t			*fib
)
{
	ca_uint32_t *ptr = fib;

	__DOS_LOCK();

	*ptr = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_0);
	*(ptr+1) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_1);
	*(ptr+2) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_2);
	*(ptr+3) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_3);
	*(ptr+4) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_4);
	*(ptr+5) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_5);
	*(ptr+6) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_6);
	*(ptr+7) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_7);
	*(ptr+8) = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_8);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_get);

ca_status_t aal_dos_atk_flood_cnt_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_flood_cnt_mask_t	mask,
	CA_IN	aal_dos_atk_flood_cnt_t		*cnt
)
{
	L3FE_PP_DOS_ATK_FLOOD_CNT_t cur_cnt;

	__DOS_LOCK();

	cur_cnt.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_FLOOD_CNT);

	if(mask.s.cnt)
		cur_cnt.bf.cnt = cnt->cnt;

	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_FLOOD_CNT, cur_cnt.wrd);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_flood_cnt_set);

ca_status_t aal_dos_atk_flood_cnt_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_flood_cnt_t		*cnt
)
{
	ca_uint32_t *ptr = cnt;

	__DOS_LOCK();
	*ptr = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_FLOOD_CNT);
	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_flood_cnt_get);

ca_status_t aal_dos_atk_event_trap_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_event_trap_mask_t	mask,
	CA_IN	aal_dos_atk_event_trap_t	*trap_cfg
)
{
	L3FE_PP_DOS_ATK_EVENT_TRAP_t cur_trap_cfg;

	__DOS_LOCK();

	cur_trap_cfg.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_EVENT_TRAP);

	if(mask.s.rdir_ldpid)
		cur_trap_cfg.bf.rdir_ldpid = trap_cfg->rdir_ldpid;

	if(mask.s.rdir_cos)
		cur_trap_cfg.bf.rdir_cos = trap_cfg->rdir_cos;

	if(mask.s.keep_orig_pkt)
		cur_trap_cfg.bf.keep_orig_pkt = trap_cfg->keep_orig_pkt;

	if(mask.s.t0_ctrl)
		cur_trap_cfg.bf.t0_ctrl = trap_cfg->t0_ctrl;

	if(mask.s.t1_ctrl)
		cur_trap_cfg.bf.t1_ctrl = trap_cfg->t1_ctrl;

	if(mask.s.t2_ctrl)
		cur_trap_cfg.bf.t2_ctrl = trap_cfg->t2_ctrl;

	if(mask.s.t3_ctrl)
		cur_trap_cfg.bf.t3_ctrl = trap_cfg->t3_ctrl;

	if(mask.s.t4_ctrl)
		cur_trap_cfg.bf.t4_ctrl = trap_cfg->t4_ctrl;

	if(mask.s.t5_ctrl)
		cur_trap_cfg.bf.t5_ctrl = trap_cfg->t5_ctrl;

	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_EVENT_TRAP, cur_trap_cfg.wrd);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_event_trap_set);

ca_status_t aal_dos_atk_event_trap_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_event_trap_t	*trap_cfg
)
{
	ca_uint32_t *ptr = trap_cfg;

	__DOS_LOCK();
	*ptr = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_EVENT_TRAP);
	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_event_trap_get);

ca_status_t aal_dos_atk_event_drop_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_event_drop_mask_t	mask,
	CA_IN	aal_dos_atk_event_drop_t	*drop_cfg
)
{

	L3FE_PP_DOS_ATK_EVENT_DROP_t cur_drop_cfg;

	__DOS_LOCK();
	cur_drop_cfg.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_EVENT_DROP);

	if(mask.s.rdir_ldpid)
		cur_drop_cfg.bf.rdir_ldpid = drop_cfg->rdir_ldpid;

	if(mask.s.rdir_cos)
		cur_drop_cfg.bf.rdir_cos = drop_cfg->rdir_cos;

	if(mask.s.keep_orig_pkt)
		cur_drop_cfg.bf.keep_orig_pkt = drop_cfg->keep_orig_pkt;

	if(mask.s.t0_ctrl)
		cur_drop_cfg.bf.t0_ctrl = drop_cfg->t0_ctrl;

	if(mask.s.t1_ctrl)
		cur_drop_cfg.bf.t1_ctrl = drop_cfg->t1_ctrl;

	if(mask.s.t2_ctrl)
		cur_drop_cfg.bf.t2_ctrl = drop_cfg->t2_ctrl;

	if(mask.s.t3_ctrl)
		cur_drop_cfg.bf.t3_ctrl = drop_cfg->t3_ctrl;

	if(mask.s.t4_ctrl)
		cur_drop_cfg.bf.t4_ctrl = drop_cfg->t4_ctrl;

	if(mask.s.t5_ctrl)
		cur_drop_cfg.bf.t5_ctrl = drop_cfg->t5_ctrl;

	CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_DOS_ATK_EVENT_DROP, cur_drop_cfg.wrd);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_event_drop_set);

ca_status_t aal_dos_atk_event_drop_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_event_drop_t	*drop_cfg
)
{
	ca_uint32_t *ptr = drop_cfg;

	__DOS_LOCK();
	*ptr = CA_DEV_NE_REG_READ(device_id, L3FE_PP_DOS_ATK_EVENT_DROP);
	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_dos_atk_event_drop_get);

ca_status_t aal_dos_init(CA_IN ca_device_id_t dev)
{
	ca_status_t ret = CA_E_OK;

	if (!__aal_l3_dos_lock)
        ca_spin_lock_init(&__aal_l3_dos_lock, SPINLOCK_BH);
	return ret;
}
EXPORT_SYMBOL(aal_dos_init);

