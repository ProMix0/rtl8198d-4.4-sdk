#ifndef NOR_SPIF_TIMER_H
#define NOR_SPIF_TIMER_H

#ifdef CONFIG_RTK_SOC_RTL8198D
#ifndef REG32
#define REG32(reg)	(*(volatile unsigned int *)(reg))
#endif
#define TC8_DATA	0xB8003290
#define TC8_CNT		0xB8003294
#define TC8_CTRL	0xB8003298

inline void norsf_wip_start_time(u32_t *start, u32_t *now)
{
	*start = *now = 0;
	REG32(TC8_DATA) = 0xFFFFFFF;
	REG32(TC8_CTRL) = 0x110007D0;
}

inline void norsf_wip_update_time(u32_t *now)
{
	*now = REG32(TC8_CNT);
}

bool norsf_wip_is_timeout(u32_t *start, u32_t *now, u32_t timeout)
{
	u32_t wip_time_to_jiffies = *now / 1000;

	if (wip_time_to_jiffies > timeout) {
		pr_err("Flash timeout is %u, WIP last time = %u\n",
				timeout, wip_time_to_jiffies);
		return true;
	}
	return false;
}

#else

#if defined(CONFIG_UNDER_UBOOT)
	#define GET_CPU_CYCLE() read_c0_count()
#else
	#define GET_CPU_CYCLE() jiffies
#endif

inline void norsf_wip_start_time(u32_t *start, u32_t *now)
{
	*now = *start = GET_CPU_CYCLE();
}

inline void norsf_wip_update_time(u32_t *now)
{
	*now = GET_CPU_CYCLE();
}

bool norsf_wip_is_timeout(u32_t *start, u32_t *now, u32_t timeout)
{
	if ((*now - *start) > timeout) {
		pr_err("Flash timeout is %u, WIP last time = %u\n", timeout, *now - *start);
		return true;
	}

	return false;
}
#endif /* CONFIG_RTK_SOC_RTL8198D */
#endif /* NOR_SPIF_TIMER_H */
