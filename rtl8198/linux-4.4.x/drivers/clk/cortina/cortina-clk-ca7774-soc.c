/*

 * Copyright 2017-2018 Cortina Access, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#if 0
#define DEBUG_CA7774_CLK 1
#endif
#include <dt-bindings/clock/ca7774-clock.h>
#include <soc/cortina/g3_registers.h>
#include <linux/kern_levels.h>

#include "cortina-clk.h"

#define MINIMUM_DELAY_PLL_FOR_LOCK 10000
#define MINIMUM_DELAY_PLL_FOR_SWITCH 10

#include "cortina-clk-divn-multiplier.h"
#include <soc/cortina/cortina-clk-ca7774-soc.h>
#include "cortina-clk-ca7774-cpu.h"


struct ca7774_cpll_global_strap_shadow_entry {
        unsigned int    cortex;
        unsigned int    prediv;
        unsigned int    divn;
        unsigned int    divsel;
};

static struct ca7774_cpll_global_strap_shadow_entry ca7774_cpll_global_strap_table[] = {
        { .cortex = 400000000, .prediv = 3, .divn = 237, .divsel = 0x26, },
        { .cortex = 625000000, .prediv = 3, .divn = 247, .divsel = 0x4, },
        { .cortex = 700000000, .prediv = 1, .divn = 165, .divsel = 0x4, },
        { .cortex = 800000000, .prediv = 1, .divn = 189, .divsel = 0x4, },
        { .cortex = 900000000, .prediv = 1, .divn = 213, .divsel = 0x4, },
        { .cortex = 1000000000, .prediv = 3, .divn = 197, .divsel = 0x2, },
        { .cortex = 1100000000, .prediv = 3, .divn = 217, .divsel = 0x2, },
        { .cortex = 1200000000, .prediv = 3, .divn = 237, .divsel = 0x2, },
        { }
};


static struct clk *clks[CA7774_CLK_END];
static struct clk_onecell_data clk_data;

struct clk **cortina_clk_get_ca7774_soc_clocks(void)
{
    return clks;
}

static const u32 clks_init_on[] __initconst = {
	CA7774_CLK_CPLL_BUS, CA7774_CLK_CPU,
};



static void __init ca7774_clocks_init(struct device_node *np)
{
	void __iomem *base;
#if 0
	int i;
#endif
	struct resource res;
	int ret;
	int global_strap_mode_idx = 0;
	GLOBAL_CPLL0_t cpll0;
	GLOBAL_FPLL0_t fpll0;
	GLOBAL_CPLLDIV_t cplldiv;
	struct ca7774_cpll_global_strap_shadow_entry globalFreqEntry;


	clks[CA7774_CLK_DUMMY] = cortina_clk_fixed("dummy", 0);

	
	of_address_to_resource(np, 0, &res);
	base = of_iomap(np, 0);

	ca7774_clk_dev_dbg("res.start 0x%lx res.end 0x%lx\n", 
		(unsigned long int) res.start,
		(unsigned long int) res.end);
	WARN_ON(!base);


	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
	ca7774_clk_dev_dbg("base 0x%lx\n", (unsigned long int) base);

	ca7774_clk_dev_dbg("Writing setup and guard timings to CPLLMUX\n");
	/* magic values received from HW team */
        ca7774_clk_dev_dbg("%s DUMP write to CPLLMUX 0x%08x\n", __func__, 0xff000000);
	writel(0xff000000, base + 0x138);

#if 1   /* dump the resulting registers */
        ca7774_clk_dev_dbg("%s DUMP CPLLDIV 0x%08x\n", __func__, readl(base + 0x134));
        ca7774_clk_dev_dbg("%s DUMP CPLLMUX 0x%08x\n", __func__, readl(base + 0x138));
        ca7774_clk_dev_dbg("%s DUMP CPLL0 0x%08x\n", __func__, readl(base + 0x160));
        ca7774_clk_dev_dbg("%s DUMP FPLL0 0x%08x\n", __func__, readl(base + 0x170));

#endif

	/* shift and mask out just the speed mode field */
	global_strap_mode_idx = (readl(base + 0x14)>>1) & 0x7;

	/* dont register if global strap is set to 400 Mhz */
	if(global_strap_mode_idx == 0)
	 {
	   ca7774_clk_dev_dbg("Not setting up Clock Framework.\n");
	   ca7774_clk_dev_dbg("Dynamic CPU speed unsupported with 400Mhz strapping config\n");
	   return;
	 }
	else
	 globalFreqEntry = ca7774_cpll_global_strap_table[global_strap_mode_idx];

	ca7774_clk_dev_dbg("Setup FPLL to 1600 Mhz\n");
	fpll0.wrd = readl(base + 0x170);
	fpll0.bf.DIV_PREDIV_BPS = 0;
	/* divide by 1 + 2 = 3, 25Mhz/3 = 8.33333 Mhz */
	fpll0.bf.DIV_PREDIV_SEL = 1;
	fpll0.bf.SCPU_RSTB = 1;
	fpll0.bf.SCPU_POW = 1;
	/* multiple by 189 + 3 = 192, 8.3333 Mhz & 192 = 1600 Mhz */ 
	fpll0.bf.SCPU_DIV_DIVN = 189;
        ca7774_clk_dev_dbg("%s DUMP write to FPLL0 0x%08x\n", __func__, fpll0.wrd);
	writel(fpll0.wrd, base + 0x170);
	mdelay(100);

	ca7774_clk_dev_dbg("Setting Cortex FPLL DIVSEL to /2\n");
	cplldiv.wrd = readl(base + 0x134);
	cplldiv.bf.f2c_divsel = 2<<1;
        ca7774_clk_dev_dbg("%s DUMP write to CPLLDIV 0x%08x\n", __func__, cplldiv.wrd);
	writel(cplldiv.wrd, base + 0x134);
	mdelay(100);
	
	ca7774_clk_dev_dbg("Temporarily Switching Cortex Clk to FPLL\n");
	cplldiv.wrd = readl(base + 0x134);
	cplldiv.bf.cpll_div_override = 0;
	cplldiv.bf.cci_onetoone = 0;
	cplldiv.bf.cpll_mode_override = 0;
	cplldiv.bf.cf_sel = 1;
        ca7774_clk_dev_dbg("%s DUMP write to CPLLDIV 0x%08x\n", __func__, cplldiv.wrd);
	writel(cplldiv.wrd, base + 0x134);
	mdelay(100);

	ca7774_clk_dev_dbg("Setting up dynamic CPLL match requested Global Strap Mode Values\n");
        cpll0.wrd = readl(base + 0x160);
        cpll0.bf.DIV_PREDIV_BPS = 0;
	ca7774_clk_dev_dbg("Setting CPLL.PREDIV to %d\n", globalFreqEntry.prediv);
        cpll0.bf.DIV_PREDIV_SEL = globalFreqEntry.prediv;
        cpll0.bf.SCPU_RSTB = 1;
        cpll0.bf.SCPU_POW = 1;
	ca7774_clk_dev_dbg("Settting CPLL.DIVN to %d\n", globalFreqEntry.divn);
        cpll0.bf.SCPU_DIV_DIVN = globalFreqEntry.divn;
        ca7774_clk_dev_dbg("%s DUMP write to CPLL0 0x%08x\n", __func__, cpll0.wrd);
        writel(cpll0.wrd, base + 0x160);
        ca7774_clk_dev_dbg("%s DUMP waiting for new CPLL0 values to stabilize\n", __func__);
        udelay(MINIMUM_DELAY_PLL_FOR_LOCK);

	cplldiv.wrd = readl(base + 0x134);
	ca7774_clk_dev_dbg("Setting Cortex CPLL DIVSEL to 0x%x\n", globalFreqEntry.divsel);
	cplldiv.bf.cortex_divsel = globalFreqEntry.divsel;
	cplldiv.bf.cpll_div_override = 1;
	cplldiv.bf.cpll_mode_override = 1;
	cplldiv.bf.cf_sel = 0;
        ca7774_clk_dev_dbg("%s DUMP write to CPLLDIV 0x%08x\n", __func__, cplldiv.wrd);
	ca7774_clk_dev_dbg("Switching Cortex Clk back to CPLL/divsel\n");
        writel(cplldiv.wrd, base + 0x134);
	udelay(MINIMUM_DELAY_PLL_FOR_SWITCH);

	/* register CPU clock */
	clks[CA7774_CLK_CPU] = cortina_clk_register_ca7774_cpu("cpu", NULL, base);
#if defined(CONFIG_LUNA_G3_SERIES)
	printk("CPU Clock: %u(HZ)\n", globalFreqEntry.cortex);
#endif
#if 0
	ca7774_clk_dev_dbg("Checking Clocks\n");
	cortina_check_clocks(clks, ARRAY_SIZE(clks));



	/*
	 * Make sure those always on clocks are enabled to maintain the correct
	 * usecount and enabling/disabling of parent PLLs.
	 */
	ca7774_clk_dev_dbg("Enabling Clocks\n");
	for (i = 0; i < ARRAY_SIZE(clks_init_on); i++)
		clk_prepare_enable(clks[clks_init_on[i]]);
#endif

        ca7774_clk_dev_dbg("Setting up Min CPU freq\n");
        /* Set Min frequency for CPU output to 400 Mhz */
        ret = clk_set_min_rate(clks[CA7774_CLK_CPU], 400000000);
        if (ret)
                pr_warn("%s: failed to set minimum clock rate bounds for CPU %d!\n",
                        __func__, ret);

        ca7774_clk_dev_dbg("Setting up Max CPU freq\n");
        /* Set Max frequency for CPU output to 1.2 Ghz */
        ret = clk_set_max_rate(clks[CA7774_CLK_CPU], 1200000000);
        if (ret)
                pr_warn("%s: failed to set maximum clock rate bounds for CPU %d!\n",
                        __func__, ret);

        clk_data.clks = clks;
        clk_data.clk_num = ARRAY_SIZE(clks);
        of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);

#if 0
	ca7774_clk_dev_dbg("Set CPU to 900Mhz\n");
	ret = clk_set_rate(clks[CA7774_CLK_CPU],900000000);
        if (ret)
                pr_warn("%s: failed to set CORTEX CPU clock rate %d!\n",
                        __func__, ret);
#endif

#if 1	/* dump the resulting registers */
	ca7774_clk_dev_dbg("%s DUMP CPLLDIV 0x%08x\n", __func__, readl(base + 0x134));
	ca7774_clk_dev_dbg("%s DUMP CPLLMUX 0x%08x\n", __func__, readl(base + 0x138));
	ca7774_clk_dev_dbg("%s DUMP CPLL0 0x%08x\n", __func__, readl(base + 0x160));
	ca7774_clk_dev_dbg("%s DUMP FPLL0 0x%08x\n", __func__, readl(base + 0x170));

#endif 
}
CLK_OF_DECLARE(ca7774, "cortina,ca7774-glbclk", ca7774_clocks_init);
