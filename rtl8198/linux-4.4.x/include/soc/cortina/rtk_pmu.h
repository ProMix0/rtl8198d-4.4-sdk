/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/math64.h>

enum armv7_perf_types {
	ARMV7_PERFCTR_PMNC_SW_INCR			= 0x00,
	ARMV7_PERFCTR_L1_ICACHE_REFILL		= 0x01,
	ARMV7_PERFCTR_ITLB_REFILL			= 0x02,
	ARMV7_PERFCTR_L1_DCACHE_REFILL		= 0x03,
	ARMV7_PERFCTR_L1_DCACHE_ACCESS		= 0x04,
	ARMV7_PERFCTR_DTLB_REFILL			= 0x05,
	ARMV7_PERFCTR_MEM_READ				= 0x06,
	ARMV7_PERFCTR_MEM_WRITE				= 0x07,
	ARMV7_PERFCTR_INSTR_EXECUTED		= 0x08,
	ARMV7_PERFCTR_EXC_TAKEN				= 0x09,
	ARMV7_PERFCTR_EXC_EXECUTED			= 0x0A,
	ARMV7_PERFCTR_CID_WRITE				= 0x0B,

	/*
	 * ARMV7_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
	 * It counts:
	 *  - all (taken) branch instructions,
	 *  - instructions that explicitly write the PC,
	 *  - exception generating instructions.
	 */
	ARMV7_PERFCTR_PC_WRITE				= 0x0C,
	ARMV7_PERFCTR_PC_IMM_BRANCH			= 0x0D,
	ARMV7_PERFCTR_PC_PROC_RETURN		= 0x0E,
	ARMV7_PERFCTR_MEM_UNALIGNED_ACCESS	= 0x0F,
	ARMV7_PERFCTR_PC_BRANCH_MIS_PRED	= 0x10,
	ARMV7_PERFCTR_CLOCK_CYCLES			= 0x11,
	ARMV7_PERFCTR_PC_BRANCH_PRED		= 0x12,

	/* These events are defined by the PMUv2 supplement (ARM DDI 0457A). */
	ARMV7_PERFCTR_MEM_ACCESS			= 0x13,
	ARMV7_PERFCTR_L1_ICACHE_ACCESS		= 0x14,
	ARMV7_PERFCTR_L1_DCACHE_WB			= 0x15,
	ARMV7_PERFCTR_L2_CACHE_ACCESS		= 0x16,
	ARMV7_PERFCTR_L2_CACHE_REFILL		= 0x17,
	ARMV7_PERFCTR_L2_CACHE_WB			= 0x18,
	ARMV7_PERFCTR_BUS_ACCESS			= 0x19,
	ARMV7_PERFCTR_MEM_ERROR				= 0x1A,
	ARMV7_PERFCTR_INSTR_SPEC			= 0x1B,
	ARMV7_PERFCTR_TTBR_WRITE			= 0x1C,
	ARMV7_PERFCTR_BUS_CYCLES			= 0x1D,

	ARMV7_PERFCTR_CPU_CYCLES			= 0xFF
};

#define PERF_ARRAY_MAX	20
#define PERF_EVENT_MAX 	6

struct perf_stat_s {
 	__u64 acc_count[PERF_EVENT_MAX];
	__u64 measure_times[PERF_EVENT_MAX];
 	__u64 executed_num;
};
typedef struct perf_stat_s perf_stat_t;

extern perf_stat_t perf_event_stat[PERF_ARRAY_MAX];
extern __u64 avrg_count[PERF_ARRAY_MAX];

extern void start_perf_counter(unsigned int sw_index, unsigned int event_mask);
extern int stop_perf_counter(unsigned int sw_index, unsigned int event_mask);

#if 0
	usage:
	1. enable Realtek PMU:
		make linux_menuconfig
		->  System Type 
			->  [*] light ARM PMU by Realtek 	
			
	2. insert test code to measured function, and rebuild image.
	func_a()
	{
		start_perf_counter(0, 0x3f); // count 6 events, save to idx 0
		...
		measured code
		if (...) {
			...
			stop_perf_counter(0, 0x3f);
			return;
		}
		...
		stop_perf_counter(0, 0x3f); // save to idx 0
	}

	func_b()
	{
		start_perf_counter(1, 0x13); // count 3 events, save to idx 1
		...
		measured code
		if (...) {
			...
			stop_perf_counter(1, 0x13);
			return;
		}
		...
		stop_perf_counter(1, 0x13); // save to idx 1
	}
	maximum saved array is 20, i.e. you can add 20 set of start_perf_counter() 
		and stop_perf_counter() to get perf counter.
		but they can not be nested.

	maximum perf event in each start_perf_counter() function is 6, i.e. 
			the valid value of first argument of start_perf_counter() 
			and second argument of stop_perf_counter() is from 0x1 to 0x3f.

	where the first argument of start_perf_counter() and second argument of 
			stop_perf_counter() is "perf event bit mask", it is fixed definition now.
				bit 0: ARMV7_PERFCTR_CLOCK_CYCLES
				bit 1: ARMV7_PERFCTR_L1_ICACHE_REFILL
				bit 2: ARMV7_PERFCTR_L1_DCACHE_REFILL
				bit 3: ARMV7_PERFCTR_L1_DCACHE_WB
				bit 4: ARMV7_PERFCTR_L2_CACHE_REFILL
				bit 5: ARMV7_PERFCTR_BUS_CYCLES
				
	Test code example:
	2.1 kernel static link code
		8192cd_proc.c
		=============
#ifdef CONFIG_RTK_PERF_EVENTS
#include <soc/cortina/rtk_pmu.h>
#endif

static int rtl8192cd_proc_cam_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();

#ifdef CONFIG_RTK_PERF_EVENTS
	start_perf_counter(6, 0x21); // measure ARMV7_PERFCTR_CLOCK_CYCLES and ARMV7_PERFCTR_BUS_CYCLES, save to idx 6
#endif

	.....
	
#ifdef CONFIG_RTK_PERF_EVENTS
	stop_perf_counter(6, 0x21); // save to idx 6
#endif

	return pos;
}
		
	2.2 kernel module, like NE
		ca_ni_rx.c
		==========
#ifdef CONFIG_RTK_PERF_EVENTS
#include <soc/cortina/rtk_pmu.h>
#endif

static int ca_ni_rx_napi(int cpu_port, u8 voq, struct napi_struct *napi, int budget)
{
	ca_eth_private_t *cep = netdev_priv(napi->dev);

	.....
	while ((refill_cnt < count) && (received_pkts < budget)) {

#ifdef CONFIG_RTK_PERF_EVENTS 		
		start_perf_counter(3, 0x1); // measure ARMV7_PERFCTR_CLOCK_CYCLES, save to idx 3
#endif
		.....

			if (skb == NULL) {
				skb_free_frag(rx_virt_addr);
				printk("%s: not able to allocate skb here!!\n", __func__);
#ifdef CONFIG_RTK_PERF_EVENTS 
				stop_perf_counter(3, 0x1);
#endif

				return 0;
			}

		.....
skb_handled:
		received_pkts++;
#ifdef CONFIG_RTK_PERF_EVENTS 
		stop_perf_counter(3, 0x1); // save to idx 1
#endif

	}
		
	3. after DUT is power on:
		before test: echo clear > /proc/driver/realtek/pmu
		after  test: echo dump  > /proc/driver/realtek/pmu
#endif
 
