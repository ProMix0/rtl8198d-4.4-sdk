/*
 * Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright 2014  Tony Wu (tonywu@realtek.com)
 */

#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>

#include <linux/clkdev.h>
#include <linux/clk-provider.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/smp_twd.h>

#define WDT_CTRL_OFFSET			0x0
#define WDT_PS_OFFSET			0x4
#define WDT_LD_OFFSET			0xC
#define WDT_LOADE_OFFSET		0x10

static void __init plat_map_io(void)
{

}

static void __init plat_init_machine(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static void plat_arch_reset(char mode, const char *cmd)
{
}

void (*arch_reset)(char, const char *) = plat_arch_reset;

static void plat_restart(enum reboot_mode mode, const char *cmd)
{
	struct device_node *wdt;
	void __iomem *global_cfg, *sec_wdt_enable, *wdt_base;

	wdt = of_find_compatible_node(NULL, NULL, "rtk,rtl8198f-wdt");
	if (!wdt) {
		pr_emerg("Find 8198F WDT node failed\n");
		return;
	}

	/* Map registers for WDT in device tree */
	wdt_base = of_iomap(wdt, 0);
	if (!wdt_base) {
		pr_emerg("Map 8198F WDT base failed\n");
		return;
	}
	sec_wdt_enable = of_iomap(wdt, 1);
	if (!sec_wdt_enable) {
		pr_emerg("Map 8198F secure WDT enable register failed\n");
		return;
	}
	global_cfg = of_iomap(wdt, 2);
	if (!global_cfg) {
		pr_emerg("Map 8198F global config register failed\n");
		return;
	}

	/* Enable non-secure WDT */
	writel(0x50, global_cfg);
	writel(0x1, sec_wdt_enable);

	/* Set WDT timeout value to minimum */
	writel(0x0, wdt_base + WDT_PS_OFFSET);
	writel(0x1, wdt_base + WDT_LD_OFFSET);
	writel(0x3, wdt_base + WDT_LOADE_OFFSET);
	writel(0x7, wdt_base + WDT_CTRL_OFFSET);
}

static const char * const plat_dt_match[] __initconst = {
	"cortina,g3fpga",
	NULL
};

DT_MACHINE_START(RLXARM_DT, "Cortina ARM (Flattened Device Tree)")
	.dt_compat = plat_dt_match,
	.init_machine = plat_init_machine,
	.map_io = plat_map_io,
	.restart = plat_restart,
MACHINE_END
