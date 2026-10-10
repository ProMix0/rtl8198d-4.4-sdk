#ifndef PTOOL_H
#define PTOOL_H

/*

[1] Realtek CPU performance monitor.

	user guide:
		1. add "#include <linux/ptool.h>" in front of the trace file.
		2. add "PROFILE_START" to the function header and add "PROFILE_END" at the end of function.
		3. Start monitoring with "echo 1 > /proc/ptool/profile_enable".
		4. Stop monitoring with "echo 0 > /proc/ptool/profile_enable".
		5. Dump statistics by "cat /proc/ptool/profile_dump".
	
[2] Logical analysis by Realtek GPIO.

	user guide:
		1. add "#include <linux/ptool.h>" in front of the trace file.
		2. add "LA_GPIO_HIGH(LA_GPIO_XXX)" to the function header and add "LA_GPIO_LOW(LA_GPIO_XXX)" at the end of function.
		3. config  which GPIO pin is used by each LA_GPIO_XXX (only pin1~pin63 is supportted)
		     please see "cat /proc/ptool/la_gpio_set" for more tips.		
		
*/

#ifdef CONFIG_RTK_PTOOL_CPU_PERF

#include <linux/math64.h>
#define MAX_HASH_IDX_BIT 12
#define MAX_HASH_IDX (1<<MAX_HASH_IDX_BIT)
#define MAX_HASH_WAY_BIT 4	/* 16 way */
#define FUNC_LEN	42

int profile_start(void);
int profile_stop(void);
int profile(const char *func);
void profile_end(int idx);
int profile_dump(void);
#endif


#if defined(CONFIG_RTL9607C_SERIES) && defined(CONFIG_RTK_PTOOL_CPU_PERF)
#define PROFILE_START int profile_idx=profile(__FUNCTION__);
#define PROFILE_END profile_end(profile_idx);
#else
#define PROFILE_START do{} while(1);
#define PROFILE_END do{} while(1);
#endif



typedef enum rtk_la_gpio_signal_e
{
	LA_GPIO_SOFTIRQ,
	LA_GPIO_HW_ISR,
	LA_GPIO_SPINLOCK,
	LA_GPIO_NIC_RX,
	LA_GPIO_NIC_TX,	
	LA_GPIO_WIFI_RX_DSR,	
	LA_GPIO_WIFI_TX,	
	LA_GPIO_AUX0,
	LA_GPIO_AUX1,
	LA_GPIO_AUX2,
	LA_GPIO_AUX3,
	LA_GPIO_AUX4,
	LA_GPIO_AUX5,
	LA_GPIO_AUX6,
	LA_GPIO_AUX7,
	LA_GPIO_MAX,
} rtk_la_gpio_signal_t;


#if defined(CONFIG_RTL9607C_SERIES) && defined(CONFIG_RTK_PTOOL_LA_BY_GPIO)
extern int la_gpio_signal_cpu_mask[LA_GPIO_MAX];
extern int la_gpio_signal_gpio_id[LA_GPIO_MAX];
void la_gpio_high(int cpu_mask, int gpio_id);
void la_gpio_low(int cpu_mask, int gpio_id);

#define LA_GPIO_HIGH(signal) do { if(la_gpio_signal_gpio_id[signal]) la_gpio_high(la_gpio_signal_cpu_mask[signal], la_gpio_signal_gpio_id[signal]); } while(0);
#define LA_GPIO_LOW(signal) do { if(la_gpio_signal_gpio_id[signal]) la_gpio_low(la_gpio_signal_cpu_mask[signal], la_gpio_signal_gpio_id[signal]); } while(0);

#else	
#define LA_GPIO_HIGH(x) do{} while(0);
#define LA_GPIO_LOW(x)  do{} while(0);	
#endif

#endif

