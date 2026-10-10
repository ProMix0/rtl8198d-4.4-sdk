/*
 * Copyright (C) 2006-2015  Realtek Semiconductor Corp.
 * Copyright (c) 2015-2017, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/time.h>
#include <asm/reboot.h>

#include <asm/taroko-mmcr.h>

#ifdef CONFIG_OF
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#endif

#include <linux/clk-provider.h>
#include <asm/prom.h>
#include "bspchip.h"

static void plat_machine_restart(char *command)
{
	GLOBAL_GLOBAL_CONFIG_t glb_cfg;
	GLOBAL_BLOCK_RESET_t blk_rst;
	PER_WDT_CTRL_t wdt_ctrl;
	PER_WDT_LOADE_t	wdt_loade;
#if defined(CONFIG_CORITNA_RESET_TYPE)
	GLOBAL_SOFTWARE_t glb_sw;
#endif
	local_irq_disable();

	/* Keep DSP at reset */
	blk_rst.wrd = __raw_readl((void *)KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET));
	blk_rst.bf.reset_rcpu1 = 1;
	__raw_writel(blk_rst.wrd, (void *)KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET));

	glb_cfg.wrd = 0;
	glb_cfg.bf.ext_reset = 1;
	glb_cfg.bf.wd_reset_ext_reset = 1;
	glb_cfg.bf.wd_reset_remap = 1;
	glb_cfg.bf.wd_reset_all_blocks = 1;
	glb_cfg.bf.wd_reset_subsys_enable = 1;
	__raw_writel(glb_cfg.wrd, (void *)KSEG1_ATU_XLAT(GLOBAL_GLOBAL_CONFIG));

#if defined(CONFIG_CORITNA_RESET_TYPE)
	glb_sw.wrd = __raw_readl((void *)KSEG1_ATU_XLAT(GLOBAL_SOFTWARE));
	glb_sw.wrd |= 0x0200;
	__raw_writel(glb_sw.wrd, (void *)KSEG1_ATU_XLAT(GLOBAL_SOFTWARE));
#endif

	/* Short delay ;) */
	__raw_writel(0x1, (void *)KSEG1_ATU_XLAT(PER_WDT_LD));

	wdt_loade.wrd = 0;
	wdt_loade.bf.update_pre = 1;
	wdt_loade.bf.update_wdt = 1;
	__raw_writel(wdt_loade.wrd, (void *)KSEG1_ATU_XLAT(PER_WDT_LOADE));

	wdt_ctrl.wrd = 0;
	wdt_ctrl.bf.wdten = 1;
	wdt_ctrl.bf.rsten = 1;
	wdt_ctrl.bf.clksel = 1;
	__raw_writel(wdt_ctrl.wrd, (void *)KSEG1_ATU_XLAT(PER_WDT_CTRL));

	while (1) ;
}

static void plat_machine_halt(void)
{
	printk("System halted.\n");
	while(1);
}

#ifdef CONFIG_CPU_HAS_L2C
static void plat_l2c_enable(void)
{
	if (IS_ENABLED(CONFIG_SMP))
		MMCR_REG32(L2C_CTRL) = MMCR_REG32(L2C_CTRL) | 0x1;
	else {
		change_c0_cctl1(CCTL_L2CON, 1);
		change_c0_cctl1(CCTL_L2COFF, 0);
	}
}
#else
static void plat_l2c_enable(void)
{
}
#endif


/* callback function */
void __init plat_mem_setup(void)
{
	void *dtb;
	/* define io/mem region */
	ioport_resource.start = 0x18000000;
	ioport_resource.end = 0x1fffffff;

	iomem_resource.start = 0xB0000000;
	iomem_resource.end = 0xBC10FFFF;

	/* set reset vectors */
	_machine_restart = plat_machine_restart;
	_machine_halt = plat_machine_halt;
	pm_power_off = plat_machine_halt;

	plat_l2c_enable();

#ifdef CONFIG_OF
    /* intended to somewhat resemble ARM; see Documentation/arm/Booting */
	if (fw_arg0 == 0 && fw_arg1 == 0xffffffff)
		dtb = phys_to_virt(fw_arg2);
	else if (fw_arg0 == -2) /* UHI interface */
		dtb = (void *)fw_arg1;
	else if (__dtb_start != __dtb_end)
		dtb = (void *)__dtb_start;
	else
		panic("no dtb found");

	__dt_setup_arch(dtb);
#endif
}

#ifdef CONFIG_OF
void __init device_tree_init(void)
{
    struct device_node *np;

	unflatten_and_copy_device_tree();

	np = of_find_node_by_name(NULL, "cpus");
    if(np != NULL){
        of_node_put(np);
    }
}


int __init plat_of_setup(void)
{
	return __dt_register_buses("rtk,sheipa", NULL);
}

arch_initcall(plat_of_setup);

static int __init plat_dev_init(void)
{
	of_clk_init(NULL);
	return 0;
}

device_initcall(plat_dev_init);

#endif


unsigned int plat_saturn_io_clk(void)
{
    /*TODO: need different of the fpga and asic*/ 
#ifdef CONFIG_CORTINA_BOARD_FPGA
 unsigned int io_clk_hz[4] = {25000000,25000000,25000000,25000000};
#else
    unsigned int io_clk_hz[4] = {125000000,125000000,125000000,125000000};
#endif

    GLOBAL_STRAP_t strap; 
    
    strap.wrd = __raw_readl((void *)BSP_GLB_STRAP_ADDR);

    return io_clk_hz[strap.bf.speed];
}


unsigned int plat_saturn_cpu_clk(void)
{
    /*TODO: need different of the fpga and asic*/ 
#ifdef CONFIG_CORTINA_BOARD_FPGA
    unsigned int cpu_clk_hz[4] = {25000000,25000000,25000000,25000000};
#else
    unsigned int cpu_clk_hz[4] = {666666666,500000000,200000000,200000000};
#endif

    GLOBAL_STRAP_t strap;
    CLKGEN_TAROKO_AP_FREQ_CONF_t cpu_freq_conf;

    strap.wrd = __raw_readl((void *)BSP_GLB_STRAP_ADDR);

    cpu_freq_conf.wrd = __raw_readl((void *)KSEG1_ATU_XLAT(CLKGEN_TAROKO_AP_FREQ_CONF));

#define MHZ	(1000000)
    if (cpu_freq_conf.bf.cpu_ap_speed_change_enable) {
        /* use AP_FREQ_CONF to get CPU clock */
        if(strap.bf.speed==0)
            return ((2000 * MHZ /(cpu_freq_conf.bf.cpu_ap_new_divsel*2)) * cpu_freq_conf.bf.cpu_ap_divsel);
        else
            return ((1000 * MHZ /(cpu_freq_conf.bf.cpu_ap_new_divsel*2)) * cpu_freq_conf.bf.cpu_ap_divsel);
    }

    return cpu_clk_hz[strap.bf.speed];
}

