/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp interrult initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/of_fdt.h>
#include <linux/delay.h>
#include <asm/prom.h>
#include <asm/time.h>
#include <asm/reboot.h>
#include <asm/fw/fw.h>
#include <bspgpio.h>
#include "bspchip.h"


extern char __dtb_start[];

__weak void force_stop_wlan_hw(void) {
        printk("%s(%d): empty\n",__func__,__LINE__);
}

extern void PCIE_reset_pin(int *reset);
extern void PCIE1_reset_pin(int *reset);
static void plat_pcie_shutdown(void) {
	__maybe_unused int gpio_rst;

	/* Enable BTM on Lx1/2 to prevent possible system hang */
	REG32(0xb8005240) = 0xf0000000;
	REG32(0xb8005260) = 0xf0000000;

	#ifdef CONFIG_USE_PCIE_SLOT_0
	PCIE_reset_pin(&gpio_rst);
	gpioClear(gpio_rst);
	#endif
	#ifdef CONFIG_USE_PCIE_SLOT_1
	PCIE1_reset_pin(&gpio_rst);
	gpioClear(gpio_rst);
	#endif
	mdelay(1);
}

static void plat_machine_restart(char *command)
{
	printk("System restart..\n");
	force_stop_wlan_hw();
	plat_pcie_shutdown();

	if (mips_cm_base)
		REG32(0xBB000108) |= (1 << 7);
	else
		REG32(0xBB0000E0) |= (1 << 7);
	while (1) ;
}

static void plat_machine_halt(void)
{
	printk("System halted.\n");
	force_stop_wlan_hw();
	plat_pcie_shutdown();
	while(1);
}

#ifndef CONFIG_LUNA_MEMORY_AUTO_DETECTION
static int memory_dtb;

static int __init early_init_dt_find_memory(unsigned long node,
				const char *uname, int depth, void *data)
{
	if (depth == 1 && !strcmp(uname, "memory@0"))
		memory_dtb = 1;

	return 0;
}
#endif
/* callback function */
void __init plat_mem_setup(void)
{
	/* define io/mem region */
	ioport_resource.start = 0x14000000;
	ioport_resource.end = 0x1fffffff;

#if defined(CONFIG_PSTORE_RAM) || defined(CONFIG_PSTORE_RAM_MODULE)
	iomem_resource.start = 0x00000000;
#else
	iomem_resource.start = 0x14000000;
#endif
	iomem_resource.end = 0x1fffffff;

	/* set reset vectors */
	_machine_restart = plat_machine_restart;
	_machine_halt = plat_machine_halt;
	pm_power_off = plat_machine_halt;

#ifdef CONFIG_BUILTIN_DTB
	/*
	 * Load the builtin devicetree. This causes the chosen node to be
	 * parsed resulting in our memory appearing
	 */
	__dt_setup_arch(&__dtb_start);
#else
	/*
	 * U-Boot will pass $a0 and $a1 to kernel. $a0 is the magic value, -2,
	 * to check whether $a1 is the dtb address or not.
	 */
	if (fw_arg0 == -2)
		__dt_setup_arch((void *)KSEG0ADDR(fw_arg1));
#endif
#ifndef CONFIG_LUNA_MEMORY_AUTO_DETECTION 
	of_scan_flat_dt(early_init_dt_find_memory, NULL);
	if (memory_dtb)
		of_scan_flat_dt(early_init_dt_scan_memory, NULL);
	else {
		/*
		 * Memory region is shown as follows:
		 *
		 * 0x00000000 - 0x14000000 - 0x20000000 => ....
		 *      dram (320MB)      mmio        highmem
		 */
#define SZ_LOWMEM	(320 << 20)
		if (cpu_mem_size <= SZ_LOWMEM)
			add_memory_region(0, cpu_mem_size, BOOT_MEM_RAM);
		else {
			add_memory_region(0, SZ_LOWMEM, BOOT_MEM_RAM);
			if (IS_ENABLED(CONFIG_HIGHMEM)) {
				add_memory_region(HIGHMEM_START, (cpu_mem_size - SZ_LOWMEM),
						  BOOT_MEM_RAM);
			}
		}
#undef SZ_LOWMEM
	}
#endif	
	do {
		extern void rtk_core_init(void);
		rtk_core_init();
	} while (0);
}
