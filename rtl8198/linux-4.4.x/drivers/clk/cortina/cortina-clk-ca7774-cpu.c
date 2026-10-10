/*
 * Copyright (c) 2017-2018 Cortina Access Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/delay.h>
#if 0
#define DEBUG_CA7774_CLK 1
#endif

#include "cortina-clk.h"
#include <soc/cortina/cortina-clk-ca7774-soc.h>
#include "cortina-clk-ca7774-cpu.h"
#include <dt-bindings/clock/ca7774-clock.h>
#include <soc/cortina/g3_registers.h>

#define MINIMUM_DELAY_PLL_FOR_LOCK 10000
#define MINIMUM_DELAY_PLL_FOR_SWITCH 10

struct ca7774_cpll_freq_entry {
        unsigned int    cortex;
        unsigned int    prediv;
        unsigned int    divn;
        unsigned int    divsel;
};

static struct ca7774_cpll_freq_entry ca7774_cpll_freq_table[] = {
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


struct cortina_clk_ca7774_cpu {
	struct clk_hw	hw;
	struct clk **soc_clks;
	void __iomem *base;
};

static inline struct cortina_clk_ca7774_cpu *to_cortina_clk_ca7774_cpu(struct clk_hw *hw)
{
	return container_of(hw, struct cortina_clk_ca7774_cpu, hw);
}

static unsigned long cortina_clk_ca7774_cpu_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct cortina_clk_ca7774_cpu *cpu;
	struct clk **clks;
	GLOBAL_CPLLDIV_t cplldiv;
	GLOBAL_CPLL0_t cpll0;
	unsigned long current_rate;

	cpu = to_cortina_clk_ca7774_cpu(hw);
	clks = cpu->soc_clks;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
	ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);

	cplldiv.wrd = readl(cpu->base + 0x134);
	cpll0.wrd = readl(cpu->base + 0x160);

#if 0
	current_rate = ((25000000/(cpll0.bf.DIV_PREDIV_SEL + 2)) *
			 (cpll0.bf.SCPU_DIV_DIVN + 3)) /
			 ((cplldiv.bf.cortex_divsel>>1) & 0xf);
#else
	current_rate = DIV_ROUND_UP(25000000,(cpll0.bf.DIV_PREDIV_SEL + 2)); 
	current_rate = current_rate * (cpll0.bf.SCPU_DIV_DIVN + 3);
	current_rate = current_rate / ((cplldiv.bf.cortex_divsel>>1) & 0xf);
	/* round down to nearist Khz */
	current_rate = current_rate / 1000;
	current_rate = current_rate * 1000;
#endif
	
	ca7774_clk_dev_dbg("%s current rate is %d\n", __func__, current_rate);
			 
	
	return current_rate;
}

static long cortina_clk_ca7774_cpu_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	int i;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
	ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, *prate);
	ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);

	/* find a suitable rate, for now only exact rate matches are accepted */
	for(i=0; i < sizeof(ca7774_cpll_freq_table)/sizeof(struct ca7774_cpll_freq_entry); i++)
         {
           ca7774_clk_dev_dbg("%s found entry rate for %d\n", __func__, ca7774_cpll_freq_table[i].cortex);
           if(rate == ca7774_cpll_freq_table[i].cortex)
           {
                ca7774_clk_dev_dbg("%s rate match found\n", __func__);
		return(rate);
           }
	   
         }
	
        ca7774_clk_dev_dbg("%s no rate match found\n", __func__);
        ca7774_clk_dev_dbg("%s returning existing rate\n", __func__);
	return *prate;
}

static int cortina_clk_ca7774_cpu_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct cortina_clk_ca7774_cpu *cpu = to_cortina_clk_ca7774_cpu(hw);
	int i;
	GLOBAL_CPLLDIV_t cplldiv;
	GLOBAL_CPLL0_t cpll0;
	unsigned int cortex;
	unsigned int prediv;
	unsigned int divn;
	unsigned int divsel;
	
	cortex = 0;
	prediv = 0;
	divn = 0;;
	divsel = 0;;

	ca7774_clk_dev_dbg("%s CPU base 0x%lx\n", __func__, cpu->base);

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);

	/* switch to FPLL clock */
	ca7774_clk_dev_dbg("%s Switch to FPLL clock\n", __func__);
	cplldiv.wrd = readl(cpu->base + 0x134);
	cplldiv.bf.cf_sel = 1;
	writel(cplldiv.wrd, cpu->base + 0x134);

 	mdelay(10);

	ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
	ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);

	for(i=0; i < sizeof(ca7774_cpll_freq_table)/sizeof(struct ca7774_cpll_freq_entry); i++)
	 {
	   ca7774_clk_dev_dbg("%s found entry rate for %d\n", __func__, ca7774_cpll_freq_table[i].cortex);
	   if(rate == ca7774_cpll_freq_table[i].cortex)
	   {
		cortex = ca7774_cpll_freq_table[i].cortex;
		prediv = ca7774_cpll_freq_table[i].prediv;
		divn = ca7774_cpll_freq_table[i].divn;
		divsel = ca7774_cpll_freq_table[i].divsel;
	        ca7774_clk_dev_dbg("%s rate match found\n", __func__);
	        ca7774_clk_dev_dbg("%s cortex %d\n", __func__, cortex);
	        ca7774_clk_dev_dbg("%s prediv %d\n", __func__, prediv);
	        ca7774_clk_dev_dbg("%s divn %d\n", __func__, divn);
	        ca7774_clk_dev_dbg("%s divsel %d\n", __func__, divsel);
	   }
	 }

	/* if we have not found a match, do a noop */
	if(cortex == 0)
	 {
	 ca7774_clk_dev_dbg("%s requested speed invalid, ignoring request\n", __func__);
	 return 0;
	 }


	cpll0.wrd = readl(cpu->base + 0x160);

	/* reprogram CPLL PREDIV */
	ca7774_clk_dev_dbg("%s reprograming CPLL PREDIV\n", __func__);
	ca7774_clk_dev_dbg("%s setting prediv to %d\n", __func__, prediv);
	cpll0.bf.DIV_PREDIV_SEL = prediv;


	/* reprogram the CPLL DIVN */
	ca7774_clk_dev_dbg("%s reprogramming CPLL DIVN\n", __func__);
	ca7774_clk_dev_dbg("%s setting divn to %d\n", __func__, divn);
	cpll0.bf.SCPU_DIV_DIVN = divn;

	ca7774_clk_dev_dbg("%s setting CPLL0 to 0x%08x\n", __func__, cpll0.wrd);
	writel(cpll0.wrd, cpu->base + 0x160);

 	mdelay(10);


	/* reprogram CPLL Cortex DIVSEL */
	cplldiv.wrd = readl(cpu->base + 0x134);
	ca7774_clk_dev_dbg("%s reprogramm CORTEX DIVSEL\n", __func__);
	ca7774_clk_dev_dbg("%s setting cortex divsel to 0x%x\n", __func__, divsel);
	cplldiv.bf.cortex_divsel = divsel;
	writel(cplldiv.wrd, cpu->base + 0x134);


	/* switch back to CPLL clock */
	ca7774_clk_dev_dbg("%s Switch back to using CPLL clock\n", __func__);

	cplldiv.wrd = readl(cpu->base + 0x134);
	cplldiv.bf.cf_sel = 0;
	cplldiv.bf.cpll_mode_override = 1;
	cplldiv.bf.cpll_div_override = 1;
	ca7774_clk_dev_dbg("%s setting CPLLDIV to 0x%08x\n", __func__, cplldiv.wrd);
	writel(cplldiv.wrd, cpu->base + 0x134);
 	udelay(MINIMUM_DELAY_PLL_FOR_LOCK);

	ca7774_clk_dev_dbg("%s CPU reprogrammed success\n", __func__);
	return rate;
}

static const struct clk_ops cortina_clk_ca7774_cpu_ops = {
	.recalc_rate	= cortina_clk_ca7774_cpu_recalc_rate,
	.round_rate	= cortina_clk_ca7774_cpu_round_rate,
	.set_rate	= cortina_clk_ca7774_cpu_set_rate,
};

struct clk *cortina_clk_register_ca7774_cpu(const char *name, const char *parent_name, void __iomem *base)
{
	struct cortina_clk_ca7774_cpu *cpu;
	struct clk *clk;
	struct clk_init_data init;

	cpu = kzalloc(sizeof(*cpu), GFP_KERNEL);
	if (!cpu)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &cortina_clk_ca7774_cpu_ops;
	init.flags = CLK_IS_BASIC | CLK_IS_ROOT;
        init.parent_names = (parent_name ? &parent_name: NULL);
        init.num_parents = (parent_name ? 1 : 0);

	cpu->soc_clks = cortina_clk_get_ca7774_soc_clocks();
	cpu->hw.init = &init;
	cpu->base = base;

	clk = clk_register(NULL, &cpu->hw);
	if (IS_ERR(clk))
		kfree(cpu);

	return clk;
}
