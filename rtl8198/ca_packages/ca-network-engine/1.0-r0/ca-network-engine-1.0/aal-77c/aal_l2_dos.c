#include <linux/module.h>
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_l2_dos.h"
#include "aal_common.h"

ca_uint __aal_l2_dos_lock = 0;
#define __DOS_LOCK()	ca_spin_lock(__aal_l2_dos_lock)
#define __DOS_UNLOCK()	ca_spin_unlock(__aal_l2_dos_lock)

ca_status_t aal_l2_dos_atk_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_fib_mask_t		mask,
	CA_IN	aal_dos_fib_t			*fib
)
{
	aal_dos_fib_t cur_fib;
	ca_uint32_t *ptr = (ca_uint32_t*)&cur_fib;

    /* parameters check */
    if (NULL == fib) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();

	*ptr = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_0);
	*(ptr+1) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_1);
	*(ptr+2) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_2);
	*(ptr+3) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_3);
	*(ptr+4) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_4);


	if(mask.s.icmp_flood_th)
		cur_fib.icmp_flood_th = fib->icmp_flood_th;

	if(mask.s.icmp_flood)
		cur_fib.icmp_flood = fib->icmp_flood;


	if(mask.s.fin_flood_th)
		cur_fib.fin_flood_th = fib->fin_flood_th;

	if(mask.s.fin_flood)
		cur_fib.fin_flood = fib->fin_flood;


	if(mask.s.syn_flood_th)
		cur_fib.syn_flood_th = fib->syn_flood_th;

	if(mask.s.syn_flood)
		cur_fib.syn_flood = fib->syn_flood;

	if(mask.s.syn_with_data)
		cur_fib.syn_with_data = fib->syn_with_data;

	if(mask.s.udp_bomb)
		cur_fib.udp_bomb = fib->udp_bomb;

	if(mask.s.ping_of_death)
		cur_fib.ping_of_death = fib->ping_of_death;

	if(mask.s.icmp_fragment)
		cur_fib.icmp_fragment = fib->icmp_fragment;

	if(mask.s.tcp_frag_error)
		cur_fib.tcp_frag_error = fib->tcp_frag_error;

	if(mask.s.tcp_short_hdr)
		cur_fib.tcp_short_hdr = fib->tcp_short_hdr;

	if(mask.s.syn1024)
		cur_fib.syn1024 = fib->syn1024;

	if(mask.s.null_scan)
		cur_fib.null_scan = fib->null_scan;

	if(mask.s.xma_scan)
		cur_fib.xma_scan = fib->xma_scan;

	if(mask.s.syn_fin_scan)
		cur_fib.syn_fin_scan = fib->syn_fin_scan;

	if(mask.s.blat_attacks)
		cur_fib.blat_attacks = fib->blat_attacks;

	if(mask.s.land_attacks)
		cur_fib.land_attacks = fib->land_attacks;

	if(mask.s.smac_eq_dmac)
		cur_fib.smac_eq_dmac = fib->smac_eq_dmac;

	if(mask.s.dos_type_en)
		cur_fib.dos_type_en = fib->dos_type_en;

	if(mask.s.dos_port_mask)
		cur_fib.dos_port_mask = fib->dos_port_mask;

	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_0, *ptr);
	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_1, *(ptr+1));
	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_2, *(ptr+2));
	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_3, *(ptr+3));
	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_4, *(ptr+4));

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_set);

ca_status_t aal_l2_dos_atk_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_fib_t			*fib
)
{
	ca_uint32_t *ptr = (ca_uint32_t *)fib;

    /* parameters check */
    if (NULL == fib) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();

	*ptr = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_0);
	*(ptr+1) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_1);
	*(ptr+2) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_2);
	*(ptr+3) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_3);
	*(ptr+4) = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_4);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_get);

ca_status_t aal_l2_dos_atk_flood_cnt_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_flood_cnt_mask_t	mask,
	CA_IN	aal_dos_atk_flood_cnt_t		*cnt
)
{
	L2FE_PP_DOS_ATK_FLOOD_CNT_t cur_cnt;

    /* parameters check */
    if (NULL == cnt) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();

	cur_cnt.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_FLOOD_CNT);

	if(mask.s.cnt)
		cur_cnt.bf.cnt = cnt->cnt;

	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_FLOOD_CNT, cur_cnt.wrd);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_flood_cnt_set);

ca_status_t aal_l2_dos_atk_flood_cnt_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_flood_cnt_t		*cnt
)
{
	ca_uint32_t *ptr = (ca_uint32_t *)cnt;

    /* parameters check */
    if (NULL == cnt) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();
	*ptr = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_FLOOD_CNT);
	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_flood_cnt_get);

ca_status_t aal_l2_dos_atk_event_trap_set(
	CA_IN	ca_device_id_t			device_id,
	CA_IN	aal_dos_atk_event_trap_mask_t	mask,
	CA_IN	aal_dos_atk_event_trap_t	*trap_cfg
)
{
	L2FE_PP_DOS_ATK_EVENT_TRAP_t cur_trap_cfg;

    /* parameters check */
    if (NULL == trap_cfg) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();

	cur_trap_cfg.wrd = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_EVENT_TRAP);

	if(mask.s.rdir_ldpid)
		cur_trap_cfg.bf.rdir_ldpid = trap_cfg->rdir_ldpid;

	if(mask.s.rdir_cos)
		cur_trap_cfg.bf.rdir_cos = trap_cfg->rdir_cos;

	if(mask.s.keep_orig_pkt)
		cur_trap_cfg.bf.keep_orig_pkt = trap_cfg->keep_orig_pkt;

	CA_DEV_NE_REG_WRITE2(device_id, L2FE_PP_DOS_ATK_EVENT_TRAP, cur_trap_cfg.wrd);

	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_event_trap_set);

ca_status_t aal_l2_dos_atk_event_trap_get(
	CA_IN	ca_device_id_t			device_id,
	CA_OUT	aal_dos_atk_event_trap_t	*trap_cfg
)
{
	ca_uint32_t *ptr = (ca_uint32_t *)trap_cfg;

    /* parameters check */
    if (NULL == trap_cfg) {
        return CA_E_PARAM;
    }

	__DOS_LOCK();
	*ptr = CA_DEV_NE_REG_READ(device_id, L2FE_PP_DOS_ATK_EVENT_TRAP);
	__DOS_UNLOCK();

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l2_dos_atk_event_trap_get);


ca_status_t aal_l2_dos_init(CA_IN ca_device_id_t dev)
{
	ca_status_t ret = CA_E_OK;

	if (!__aal_l2_dos_lock)
        ca_spin_lock_init(&__aal_l2_dos_lock, SPINLOCK_BH);
	return ret;
}
EXPORT_SYMBOL(aal_l2_dos_init);

