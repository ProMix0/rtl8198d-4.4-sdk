#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <dt-bindings/clock/ca8289-clock.h>
#include <soc/cortina/registers.h>

#if 1
#ifdef writel
#undef writel
#define writel(v,c)		({ printk("[%s.%d]0x%x, %p\n", "clk-venus", __LINE__, v ,c); __iowmb(); writel_relaxed((v),(c)); })
#endif
#endif

/*********************************************************
* Extern CPU CLK for soc/cortina/cortina-soc.c
* to show correct CPU Clock
********************************************/
#define RTK__CPU_SPEED_INF_PATCH

#define to_ca8289_pll_clk(_hw) container_of(_hw, struct ca8289_pll_clk, hw)
#define to_ca8289_cpu_daemon(_hw) \
		container_of(_hw, struct ca8289_cpu_daemon, hw)
#define clk_mask(width) ((1 << width) - 1)

enum {
	CA_CLK_PLL,
	CA_CLK_DIVIDER,
	CA_CLK_MUX,
	CA_CLK_GATE,
	CA_CLK_FIXED_FACTOR,
	CA_CLK_PE_DAEMON,
	CA_CLK_CPU_DAEMON,
};

enum {
	PED_REF,
	PED_FPLL,
	PED_DIV_F,
	PED_MUX_PE,
	PED_CLKS_NUM,
};

enum {
	CPUD_REF,
	CPUD_CPLL,
	CPUD_DIV_F2C,
	CPUD_MUX_CPU,
	CPUD_DIV_CORTEX,
	CPUD_CLKS_NUM,
};

#define DIVN_BASE		3
#define DIVN_SHIFT		0
#define DIVN_WIDTH		8
#define PREDIV_BASE		2
#define PREDIV_SHIFT		10
#define PREDIV_WIDTH		2

#define MIN_DELAY_PLL_FOR_LOCK		10000
#define MIN_DELAY_PLL_FOR_SWITCH	10

struct ca8289_clk_reg {
	void __iomem *regbase;
	spinlock_t regs_lock;	/* register access lock */
};

struct ca8289_clk_param {
	int type;
	u32 reg;
};

struct ca8289_pll_clk {
	void __iomem *reg;
	spinlock_t *lock;	/* register access lock */
	struct clk_hw hw;
};

struct ca8289_pll_param {
	unsigned int    pll;
	unsigned int    prediv;
	unsigned int    divn;
};

struct ca8289_cpu_strap_speed {
	unsigned int    pll;
	unsigned int    div;
};

struct ca8289_pe_speed {
	unsigned int    pll;
	unsigned int    div;
};

struct ca8289_cpu_daemon {
	void __iomem *reg;
	struct mutex mlock;	/* daemon lock */
	spinlock_t *lock;	/* register access lock */
	struct clk_hw hw;
	struct clk *clks[CPUD_CLKS_NUM];
};

static const struct ca8289_pll_param ca8289_pll_param_table[] = {
	{ .pll = 40, .prediv = 5, .divn = 200, },	/* CPU */
	{ .pll = 44, .prediv = 5, .divn = 220, },	/* CPU */
	{ .pll = 48, .prediv = 5, .divn = 240, },	/* CPU */
	{ .pll = 50, .prediv = 5, .divn = 250, },	/* CPU */
	{ .pll = 56, .prediv = 3, .divn = 168, },	/* CPU */
	{ .pll = 64, .prediv = 3, .divn = 192, },	/* FPLL/CPU */
	{ .pll = 72, .prediv = 3, .divn = 216, },	/* CPU */
	{ .pll = 80, .prediv = 2, .divn = 160, },	/* EPLL */
};

static const struct ca8289_cpu_strap_speed ca8289_cpu_strap_speed_table[] = {
	{ .pll = 48, .div = 3, }, /* 400000000 */
	{ .pll = 50, .div = 2, }, /* 625000000 */
	{ .pll = 56, .div = 2, }, /* 700000000 */
	{ .pll = 64, .div = 2, }, /* 800000000 */
	{ .pll = 72, .div = 2, }, /* 900000000 */
	{ .pll = 40, .div = 1, }, /* 1000000000 */
	{ .pll = 44, .div = 1, }, /* 1100000000 */
	{ .pll = 48, .div = 1, }, /* 1200000000 */
	{ }
};

static const struct ca8289_pe_speed ca8289_pe_speed_table[] = {
	{ .pll = 64, .div = 2, }, /* 800000000 */
	{ }
};

struct ca8289_clk_param pll_cpll     = {CA_CLK_PLL, GLOBAL_CPLL0};
struct ca8289_clk_param pll_epll     = {CA_CLK_PLL, GLOBAL_EPLL0};
struct ca8289_clk_param pll_fpll     = {CA_CLK_PLL, GLOBAL_FPLL0};
struct ca8289_clk_param mux_cplldiv  = {CA_CLK_MUX, GLOBAL_CPLLDIV};
struct ca8289_clk_param mux_pediv    = {CA_CLK_MUX, GLOBAL_PEDIV};
struct ca8289_clk_param div_cplldiv  = {CA_CLK_DIVIDER, GLOBAL_CPLLDIV};
struct ca8289_clk_param div_fplldiv  = {CA_CLK_DIVIDER, GLOBAL_PEDIV};
struct ca8289_clk_param div_eplldiv  = {CA_CLK_DIVIDER, GLOBAL_EPLLDIV};
struct ca8289_clk_param div_eplldiv2 = {CA_CLK_DIVIDER, GLOBAL_EPLLDIV2};
struct ca8289_clk_param gate_config  = {CA_CLK_GATE, GLOBAL_GLOBAL_CONFIG};
struct ca8289_clk_param fixed_factor = {CA_CLK_FIXED_FACTOR, 0};
struct ca8289_clk_param pe_daemon    = {CA_CLK_PE_DAEMON, 0};
struct ca8289_clk_param cpu_daemon   = {CA_CLK_CPU_DAEMON, 0};
struct ca8289_clk_param div_trcdiv   = {CA_CLK_DIVIDER, GLOBAL_CPLLDIV};

static const struct of_device_id ca8289_soc_clks_ids[] __initconst = {
	{ .compatible = "cortina,ca8289-pll-cpll-clk",
	  .data = (void *)&pll_cpll, },
	{ .compatible = "cortina,ca8289-pll-fpll-clk",
	  .data = (void *)&pll_fpll, },
	{ .compatible = "cortina,ca8289-pll-epll-clk",
	  .data = (void *)&pll_epll, },
	{ .compatible = "cortina,mux-cpu-clk",
	  .data = (void *)&mux_cplldiv, },
	{ .compatible = "cortina,mux-pe-clk",
	  .data = (void *)&mux_pediv, },
	{ .compatible = "cortina,div-cpll-clk",
	  .data = (void *)&div_cplldiv, },
	{ .compatible = "cortina,div-fpll-clk",
	  .data = (void *)&div_fplldiv, },
	{ .compatible = "cortina,div-epll-clk",
	  .data = (void *)&div_eplldiv, },
	{ .compatible = "cortina,div-epll2-clk",
	  .data = (void *)&div_eplldiv2, },
	{ .compatible = "cortina,gate-config-clk",
	  .data = (void *)&gate_config, },
	{ .compatible = "cortina,fixed-factor-clk",
	  .data = (void *)&fixed_factor, },
	{ .compatible = "cortina,ca8289-pe-daemon-clk",
	  .data = (void *)&pe_daemon, },
	{ .compatible = "cortina,ca8289-cpu-daemon-clk",
	  .data = (void *)&cpu_daemon, },
	{},
};

static bool readonly;

unsigned long ca8289_pll_recalc_rate(struct clk_hw *hw,
				     unsigned long parent_rate)
{
	struct ca8289_pll_clk *pll_clk = to_ca8289_pll_clk(hw);
	u32 val, divn, prediv;

	val = readl(pll_clk->reg) >> DIVN_SHIFT;
	val &= clk_mask(DIVN_WIDTH);
	divn = val + DIVN_BASE;

	val = readl(pll_clk->reg) >> PREDIV_SHIFT;
	val &= clk_mask(PREDIV_WIDTH);
	prediv = val + PREDIV_BASE;

	return (parent_rate * divn / prediv);
}

long ca8289_pll_round_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long *parent_rate)
{
	unsigned long mult;
	int i;

	if ((rate % (*parent_rate)) != 0)
		goto NOT_SUPPORT;
	mult = rate / (*parent_rate);

	for (i = 0; i < ARRAY_SIZE(ca8289_pll_param_table); i++) {
		if (ca8289_pll_param_table[i].pll == mult)
			return rate;
	}

NOT_SUPPORT:
	return *parent_rate;
}

int ca8289_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct ca8289_pll_clk *pll_clk = to_ca8289_pll_clk(hw);
	unsigned long mult;
	unsigned long flags = 0;
	u32 val, divn, prediv;
	int i;

	if ((rate % parent_rate) != 0)
		return 0;
	mult = rate / parent_rate;
	prediv = 0;
	divn = 0;

	for (i = 0; i < ARRAY_SIZE(ca8289_pll_param_table); i++) {
		if (ca8289_pll_param_table[i].pll == mult) {
			prediv = ca8289_pll_param_table[i].prediv;
			divn = ca8289_pll_param_table[i].divn;
		}
	}
	if (mult != (divn / prediv))
		return 0;

	spin_lock_irqsave(pll_clk->lock, flags);

	val = readl(pll_clk->reg);
	val &= ~(clk_mask(DIVN_WIDTH) << DIVN_SHIFT);
	val |= (divn - DIVN_BASE) << DIVN_SHIFT;
	val &= ~(clk_mask(PREDIV_WIDTH) << PREDIV_SHIFT);
	val |= (prediv - PREDIV_BASE) << PREDIV_SHIFT;
	writel(val, pll_clk->reg);

	spin_unlock_irqrestore(pll_clk->lock, flags);

	return rate;
}

static const struct clk_ops ca8289_pll_ops = {
	.recalc_rate	= ca8289_pll_recalc_rate,
	.round_rate	= ca8289_pll_round_rate,
	.set_rate	= ca8289_pll_set_rate,
};

int ca8289_clk_pll(struct device_node *np, struct ca8289_clk_reg *clk_reg,
		   struct ca8289_clk_param *clk_param)
{
	const char *parent_name;
	u8 init_mult;
	void __iomem *reg;
	struct clk_init_data init;
	struct ca8289_pll_clk *pll_clk;
	struct clk *soc_clk;
	unsigned long flags = 0;
	int i;

	parent_name = of_clk_get_parent_name(np, 0);
	reg = clk_reg->regbase + clk_param->reg - GLOBAL_JTAG_ID;

	if (of_property_read_u8(np, "init-mult", &init_mult))
		goto REGISTER_ONLY;

	for (i = 0; i < ARRAY_SIZE(ca8289_pll_param_table); i++) {
		GLOBAL_EPLL0_t epll0;

		if (init_mult != ca8289_pll_param_table[i].pll)
			continue;

		spin_lock_irqsave(&clk_reg->regs_lock, flags);

		epll0.wrd = readl(reg);
		epll0.bf.DIV_PREDIV_BPS = 0;
		epll0.bf.SCPU_RSTB = 1;
		epll0.bf.SCPU_POW = 1;
		epll0.bf.DIV_PREDIV_SEL = ca8289_pll_param_table[i].prediv -
					  PREDIV_BASE;
		epll0.bf.SCPU_DIV_DIVN = ca8289_pll_param_table[i].divn -
					 DIVN_BASE;
		writel(epll0.wrd, reg);

		spin_unlock_irqrestore(&clk_reg->regs_lock, flags);

		break;
	}

REGISTER_ONLY:
	pll_clk = kzalloc(sizeof(*pll_clk), GFP_KERNEL);
	if (!pll_clk)
		return -ENOMEM;

	init.name = np->name;
	init.ops = &ca8289_pll_ops;
	init.flags = CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = (parent_name ? 1 : 0);

	pll_clk->hw.init = &init;
	pll_clk->reg = reg;
	pll_clk->lock = &clk_reg->regs_lock;

	soc_clk = clk_register(NULL, &pll_clk->hw);
	if (!IS_ERR(soc_clk))
		of_clk_add_provider(np, of_clk_src_simple_get, soc_clk);
	else
		kfree(pll_clk);

	return IS_ERR(soc_clk) ? PTR_ERR(soc_clk) : 0;
}

int ca8289_clk_divider(struct device_node *np, struct ca8289_clk_reg *clk_reg,
		       struct ca8289_clk_param *clk_param)
{
	u8 op_shift, op_width, init_div;
	const char *parent_name;
	void __iomem *reg;
	u32 val;
	struct clk *soc_clk;
	unsigned long flags = 0;

	if (of_property_read_u8(np, "operate-shift", &op_shift))
		return -EPERM;
	if (op_shift >= 32)
		return -EINVAL;
	if (of_property_read_u8(np, "operate-width", &op_width))
		return -EPERM;
	if (op_width >= 32)
		return -EINVAL;
	if (of_property_read_u8(np, "init-div", &init_div))
		init_div = 0;

	parent_name = of_clk_get_parent_name(np, 0);
	reg = clk_reg->regbase + clk_param->reg - GLOBAL_JTAG_ID;

	if (init_div) {
		spin_lock_irqsave(&clk_reg->regs_lock, flags);

		val = readl(reg);

		val &= ~(clk_mask(op_width) << op_shift);
		val |= (init_div << 1) << op_shift;

		writel(val, reg);

		spin_unlock_irqrestore(&clk_reg->regs_lock, flags);
	}

	soc_clk = clk_register_divider(NULL, np->name, parent_name, 0,
				       reg, op_shift + 1, op_width - 1,
				       CLK_DIVIDER_ONE_BASED |
				       CLK_DIVIDER_ALLOW_ZERO,
				       &clk_reg->regs_lock);

	if (!IS_ERR(soc_clk))
		of_clk_add_provider(np, of_clk_src_simple_get, soc_clk);

	return IS_ERR(soc_clk) ? PTR_ERR(soc_clk) : 0;
}

int ca8289_clk_mux(struct device_node *np, struct ca8289_clk_reg *clk_reg,
		   struct ca8289_clk_param *clk_param)
{
	u8 op_shift, num_parents;
	void __iomem *reg;
	const char *parent_names[2];
	struct clk *soc_clk;
	int i;

	if (of_property_read_u8(np, "operate-shift", &op_shift))
		return -EPERM;
	if (op_shift >= 32)
		return -EINVAL;
	num_parents = of_clk_get_parent_count(np);
	if (num_parents != 2)
		return -EINVAL;

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(np, i);

	reg = clk_reg->regbase + clk_param->reg - GLOBAL_JTAG_ID;

	soc_clk = clk_register_mux(NULL, np->name, parent_names, num_parents,
				   0, reg, op_shift, 1, 0, &clk_reg->regs_lock);
	if (!IS_ERR(soc_clk))
		of_clk_add_provider(np, of_clk_src_simple_get, soc_clk);

	return IS_ERR(soc_clk) ? PTR_ERR(soc_clk) : 0;
}

int ca8289_clk_gate(struct device_node *np, struct ca8289_clk_reg *clk_reg,
		    struct ca8289_clk_param *clk_param)
{
	u8 op_shift, gate_flags;
	void __iomem *reg;
	struct clk *soc_clk;

	if (of_property_read_u8(np, "operate-shift", &op_shift))
		return -EPERM;
	if (op_shift >= 32)
		return -EINVAL;

	if (of_property_read_bool(np, "active-low"))
		gate_flags = CLK_GATE_SET_TO_DISABLE;
	else
		gate_flags = 0;

	reg = clk_reg->regbase + clk_param->reg - GLOBAL_JTAG_ID;

	soc_clk = clk_register_gate(NULL, np->name, NULL, 0, reg, op_shift,
				    gate_flags, &clk_reg->regs_lock);
	if (!IS_ERR(soc_clk))
		of_clk_add_provider(np, of_clk_src_simple_get, soc_clk);

	return IS_ERR(soc_clk) ? PTR_ERR(soc_clk) : 0;
}

int ca8289_clk_fixed_factor(struct device_node *np,
			    struct ca8289_clk_reg *clk_reg,
			    struct ca8289_clk_param *clk_param)
{
	unsigned int mult, div;
	const char *parent_name;
	struct clk *soc_clk;

	if (of_property_read_u32(np, "fixed-mult", &mult))
		mult = 1;
	if (of_property_read_u32(np, "fixed-div", &div))
		div = 1;

	parent_name = of_clk_get_parent_name(np, 0);

	soc_clk = clk_register_fixed_factor(NULL, np->name, parent_name, 0,
					    mult, div);
	if (!IS_ERR(soc_clk))
		of_clk_add_provider(np, of_clk_src_simple_get, soc_clk);

	return IS_ERR(soc_clk) ? PTR_ERR(soc_clk) : 0;
}

int ca8289_clk_pe_daemon(struct device_node *np,
			 struct ca8289_clk_reg *clk_reg,
			 struct ca8289_clk_param *clk_param)
{
	const char *parent_names[PED_CLKS_NUM];
	struct clk *ped_clks[PED_CLKS_NUM];
	GLOBAL_FPLL0_t fpll0;
	unsigned long flags = 0;
	unsigned long rate, mult, div, ref_rate;
	u32 speed;
	int i;

	for (i = 0; i < PED_CLKS_NUM; i++) {
		parent_names[i] = of_clk_get_parent_name(np, i);
		ped_clks[i] = __clk_lookup(parent_names[i]);
	}

	if (of_property_read_u32(np, "init-speed", &speed))
		return 0;

	ref_rate = clk_get_rate(ped_clks[PED_REF]);
	if (speed) {
		for (i = 0; ARRAY_SIZE(ca8289_pe_speed_table); i++) {
			mult = ca8289_pe_speed_table[i].pll;
			div = ca8289_pe_speed_table[i].div;
			rate = ref_rate * mult / div;
			if (speed == rate)
				break;
		}

		if (speed != rate)
			speed = 0;
	}
	if (!speed)
		return 0;

	spin_lock_irqsave(&clk_reg->regs_lock, flags);

	fpll0.wrd = readl(clk_reg->regbase + GLOBAL_FPLL0 - GLOBAL_JTAG_ID);
	fpll0.bf.DIV_PREDIV_BPS = 0;
	fpll0.bf.SCPU_RSTB = 1;
	fpll0.bf.SCPU_POW = 1;
	writel(fpll0.wrd, clk_reg->regbase + GLOBAL_FPLL0 - GLOBAL_JTAG_ID);

	spin_unlock_irqrestore(&clk_reg->regs_lock, flags);

	clk_set_rate(ped_clks[PED_FPLL], clk_get_rate(ped_clks[PED_REF]) *
		     mult);

	udelay(MIN_DELAY_PLL_FOR_LOCK);

	clk_set_rate(ped_clks[PED_DIV_F], clk_get_rate(ped_clks[PED_FPLL]) /
		     div);

	clk_set_parent(ped_clks[PED_MUX_PE], ped_clks[PED_DIV_F]);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	return 0;
}

unsigned long ca8289_cpu_recalc_rate(struct clk_hw *hw,
				     unsigned long parent_rate)
{
	struct ca8289_cpu_daemon *cpud = to_ca8289_cpu_daemon(hw);

	return clk_get_rate(cpud->clks[CPUD_DIV_CORTEX]);
}

long ca8289_cpu_round_rate(struct clk_hw *hw, unsigned long rate,
			   unsigned long *parent_rate)
{
	struct ca8289_cpu_daemon *cpud = to_ca8289_cpu_daemon(hw);
	unsigned long ref_rate, target;
	u32 mult, div;
	int i;

	ref_rate = clk_get_rate(cpud->clks[CPUD_REF]);
	for (i = 0; ARRAY_SIZE(ca8289_cpu_strap_speed_table); i++) {
		mult = ca8289_cpu_strap_speed_table[i].pll;
		div = ca8289_cpu_strap_speed_table[i].div;
		target = ref_rate * mult / div;
		if (rate == target)
			return target;
	}

	return *parent_rate;
}

int ca8289_cpu_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct ca8289_cpu_daemon *cpud = to_ca8289_cpu_daemon(hw);
	GLOBAL_CPLLDIV_t cplldiv;
	GLOBAL_CPLL0_t cpll0;
	void __iomem *reg;
	unsigned long ref_rate, target;
	u32 mult, div;
	unsigned long flags = 0;
	int i;

	if (readonly)
		return 0;

	ref_rate = clk_get_rate(cpud->clks[CPUD_REF]);
	for (i = 0; ARRAY_SIZE(ca8289_cpu_strap_speed_table); i++) {
		mult = ca8289_cpu_strap_speed_table[i].pll;
		div = ca8289_cpu_strap_speed_table[i].div;
		target = ref_rate * mult / div;
		if (rate == target)
			goto MATCH;
	}

	return 0;

MATCH:
	mutex_lock(&cpud->mlock);

	/* unbind daemon <--> cortex temporarily to avoid loop */
	clk_set_parent(hw->clk, NULL);

	clk_set_parent(cpud->clks[CPUD_MUX_CPU], cpud->clks[CPUD_DIV_F2C]);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	clk_set_rate(cpud->clks[CPUD_CPLL], ref_rate * mult);
	udelay(MIN_DELAY_PLL_FOR_LOCK);

	/* +1 for div is 3 */
	target = clk_get_rate(cpud->clks[CPUD_MUX_CPU]) / div + 1;
	clk_set_rate(cpud->clks[CPUD_DIV_CORTEX], target);

	/* Reset CPLL */
	reg = cpud->reg + GLOBAL_CPLL0 - GLOBAL_JTAG_ID;
	cpll0.wrd = readl(reg);
	cpll0.bf.SCPU_RSTB = 0;
	cpll0.bf.SCPU_POW = 1;
	writel(cpll0.wrd, reg);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);
	cpll0.bf.SCPU_RSTB = 1;
	writel(cpll0.wrd, reg);
	isb();
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	spin_lock_irqsave(cpud->lock, flags);

	reg = cpud->reg + GLOBAL_CPLLDIV - GLOBAL_JTAG_ID;
	cplldiv.wrd = readl(reg);
	cplldiv.bf.cpll_mode_override = 1;
	cplldiv.bf.cpll_div_override = 1;
	writel(cplldiv.wrd, reg);

	spin_unlock_irqrestore(cpud->lock, flags);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);
	isb();

	clk_set_parent(cpud->clks[CPUD_MUX_CPU], cpud->clks[CPUD_CPLL]);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	clk_set_parent(hw->clk, cpud->clks[CPUD_DIV_CORTEX]);

	mutex_unlock(&cpud->mlock);

	pr_info("Change CPU frequency to %ld MHz\n", rate / 1000000);

	return rate;
}

static const struct clk_ops ca8289_cpu_ops = {
	.recalc_rate	= ca8289_cpu_recalc_rate,
	.round_rate	= ca8289_cpu_round_rate,
	.set_rate	= ca8289_cpu_set_rate,
};


#ifdef RTK__CPU_SPEED_INF_PATCH
struct clk *cpu_daemon_clk = NULL;
#endif

int ca8289_clk_cpu_daemon(struct device_node *np,
			  struct ca8289_clk_reg *clk_reg,
			  struct ca8289_clk_param *clk_param)
{
	const char *parent_names[5];
	struct clk *ref_clk;
	unsigned long rate, mult, div, ref_rate, min_rate, max_rate;
	void __iomem *reg;
	int strap;
	u32 speed;
	int i;
	struct clk *cpu_clk;
	GLOBAL_FPLL0_t cpll0;
	GLOBAL_CPLLDIV_t cplldiv;
	struct clk_init_data init;
	struct ca8289_cpu_daemon *cpud;
	unsigned long flags = 0;

	readonly = of_property_read_bool(np, "read-only");

	if (of_property_read_u32(np, "init-speed", &speed))
		speed = 0;

	cpud = kzalloc(sizeof(*cpud), GFP_KERNEL);
	if (!cpud)
		return -ENOMEM;

	for (i = 0; i < CPUD_CLKS_NUM; i++) {
		parent_names[i] = of_clk_get_parent_name(np, i);
		cpud->clks[i] = __clk_lookup(parent_names[i]);
	}

	ref_clk = __clk_lookup(parent_names[CPUD_REF]);
	ref_rate = clk_get_rate(ref_clk);
	min_rate = ref_rate * ca8289_cpu_strap_speed_table[0].pll /
			      ca8289_cpu_strap_speed_table[0].div;
	max_rate = ref_rate * ca8289_cpu_strap_speed_table[7].pll /
			      ca8289_cpu_strap_speed_table[7].div;
	if (speed) {
		for (i = 0; ARRAY_SIZE(ca8289_cpu_strap_speed_table); i++) {
			mult = ca8289_cpu_strap_speed_table[i].pll;
			div = ca8289_cpu_strap_speed_table[i].div;
			rate = ref_rate * mult / div;
			if (speed == rate)
				break;
		}
		if (speed != rate)
			speed = 0;
	}

	if (!speed) {
		reg = clk_reg->regbase + GLOBAL_STRAP - GLOBAL_JTAG_ID;

		strap = (readl(reg) >> 1) & 0x7;
		mult = ca8289_cpu_strap_speed_table[strap].pll;
		div = ca8289_cpu_strap_speed_table[strap].div;
		rate = ref_rate * mult / div;
	}

	clk_set_parent(cpud->clks[CPUD_MUX_CPU], cpud->clks[CPUD_DIV_F2C]);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	clk_set_rate(cpud->clks[CPUD_CPLL], ref_rate * mult);
	udelay(MIN_DELAY_PLL_FOR_LOCK);

	spin_lock_irqsave(&clk_reg->regs_lock, flags);
	reg = clk_reg->regbase + GLOBAL_CPLL0 - GLOBAL_JTAG_ID;
	cpll0.wrd = readl(reg);
	cpll0.bf.DIV_PREDIV_BPS = 0;
	cpll0.bf.SCPU_POW = 1;
	writel(cpll0.wrd, reg);
	spin_unlock_irqrestore(&clk_reg->regs_lock, flags);

	udelay(MIN_DELAY_PLL_FOR_LOCK);

	spin_lock_irqsave(&clk_reg->regs_lock, flags);
	cpll0.wrd = readl(reg);
	cpll0.bf.SCPU_RSTB = 0;
	writel(cpll0.wrd, reg);
	isb();
	spin_unlock_irqrestore(&clk_reg->regs_lock, flags);
	udelay(MIN_DELAY_PLL_FOR_LOCK);

	spin_lock_irqsave(&clk_reg->regs_lock, flags);
	cpll0.wrd = readl(reg);
	cpll0.bf.SCPU_RSTB = 1;
	writel(cpll0.wrd, reg);
	isb();
	spin_unlock_irqrestore(&clk_reg->regs_lock, flags);
	udelay(MIN_DELAY_PLL_FOR_LOCK);

	/* +1 for div is 3 */
	rate = clk_get_rate(cpud->clks[CPUD_MUX_CPU]) / div + 1;
	clk_set_rate(cpud->clks[CPUD_DIV_CORTEX], rate);

	if (speed) {
		spin_lock_irqsave(&clk_reg->regs_lock, flags);

		reg = clk_reg->regbase + GLOBAL_CPLLDIV - GLOBAL_JTAG_ID;
		cplldiv.wrd = readl(reg);
		cplldiv.bf.cpll_mode_override = 1;
		cplldiv.bf.cpll_div_override = 1;
		writel(cplldiv.wrd, reg);
		isb();

		spin_unlock_irqrestore(&clk_reg->regs_lock, flags);
		udelay(MIN_DELAY_PLL_FOR_LOCK);
	}

	clk_set_parent(cpud->clks[CPUD_MUX_CPU], cpud->clks[CPUD_CPLL]);
	udelay(MIN_DELAY_PLL_FOR_SWITCH);

	init.name = np->name;
	init.ops = &ca8289_cpu_ops;
	init.flags = CLK_IS_BASIC;
	init.parent_names = &parent_names[CPUD_DIV_CORTEX];
	init.num_parents = 1;

	mutex_init(&cpud->mlock);
	cpud->reg = clk_reg->regbase;
	cpud->lock = &clk_reg->regs_lock;
	cpud->hw.init = &init;

	cpu_clk = clk_register(NULL, &cpud->hw);
	if (!IS_ERR(cpu_clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, cpu_clk);

		clk_set_min_rate(cpu_clk, min_rate);
		clk_set_max_rate(cpu_clk, max_rate);
#ifdef RTK__CPU_SPEED_INF_PATCH
		cpu_daemon_clk = cpu_clk;
#endif
	} else {
		kfree(cpud);
	}

	return IS_ERR(cpu_clk) ? PTR_ERR(cpu_clk) : 0;
}

int (*ca8289_clk_setup_fns[])(struct device_node *np,
			      struct ca8289_clk_reg *clk_reg,
			      struct ca8289_clk_param *clk_param) = {
	[CA_CLK_PLL]          = ca8289_clk_pll,
	[CA_CLK_DIVIDER]      = ca8289_clk_divider,
	[CA_CLK_MUX]          = ca8289_clk_mux,
	[CA_CLK_GATE]         = ca8289_clk_gate,
	[CA_CLK_FIXED_FACTOR] = ca8289_clk_fixed_factor,
	[CA_CLK_PE_DAEMON]    = ca8289_clk_pe_daemon,
	[CA_CLK_CPU_DAEMON]   = ca8289_clk_cpu_daemon,
};

static void __init ca8289_soc_clks_setup(struct device_node *np)
{
	struct ca8289_clk_reg *clk_reg;
	struct device_node *childnp;
	const struct of_device_id *clk_id;
	struct ca8289_clk_param *clk_param;

	clk_reg = kzalloc(sizeof(*clk_reg), GFP_KERNEL);
	if (!clk_reg)
		return;

	clk_reg->regbase = of_iomap(np, 0);
	if (!clk_reg->regbase)
		return;

	spin_lock_init(&clk_reg->regs_lock);

	for_each_child_of_node(np, childnp) {
		clk_id = of_match_node(ca8289_soc_clks_ids, childnp);
		if (!clk_id)
			continue;

		clk_param = (struct ca8289_clk_param *)clk_id->data;
		if (ca8289_clk_setup_fns[clk_param->type](childnp, clk_reg,
							  clk_param))
			pr_err("ca8289_clk_setup_fns(%s) fail!\n", np->name);
	}
}

module_param(readonly, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(readonly, "CPU daemon is readonly");

CLK_OF_DECLARE(ca8289_soc_clks, "cortina,ca8289-soc-clks",
	       ca8289_soc_clks_setup);
