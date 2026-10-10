/*
 * Copyright (C) 2017-2018 Cortina Access, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include "cortina-clk.h"
#include "cortina-clk-divn-multiplier.h"

#define to_cortina_clk_divn_multiplier(_hw) container_of(_hw, struct cortina_clk_divn_multiplier, hw)

static unsigned long __get_mult(struct cortina_clk_divn_multiplier *mult,
				unsigned long rate,
				unsigned long parent_rate)
{
	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
        ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
        ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);
	if (mult->flags & CORTINA_CLK_DIVN_MULTIPLIER_ROUND_CLOSEST)
		return DIV_ROUND_CLOSEST(rate, parent_rate);

	return rate / parent_rate;
}

static unsigned long cortina_clk_divn_multiplier_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	struct cortina_clk_divn_multiplier *mult = to_cortina_clk_divn_multiplier(hw);
	unsigned long val;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
        ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
        
	val = clk_readl(mult->reg) >> mult->shift;
	val &= GENMASK(mult->width - 1, 0);

	/* whatever the read back value is, add 3 for cortina HW */
	val += 3;

	if (!val && mult->flags & CORTINA_CLK_DIVN_MULTIPLIER_ZERO_BYPASS)
		val = 1;

	ca7774_clk_dev_dbg("%s recommending value of %d\n", __func__, val);
	return parent_rate * val;
}

static bool __is_best_rate(unsigned long rate, unsigned long new,
			   unsigned long best, unsigned long flags)
{
	if (flags & CORTINA_CLK_DIVN_MULTIPLIER_ROUND_CLOSEST)
		return abs(rate - new) < abs(rate - best);

	return new >= rate && new < best;
}

static unsigned long __bestmult(struct clk_hw *hw, unsigned long rate,
				unsigned long *best_parent_rate,
				u8 width, unsigned long flags)
{
	unsigned long orig_parent_rate = *best_parent_rate;
	unsigned long parent_rate, current_rate, best_rate = ~0;
	unsigned int i, bestmult = 0;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
        ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
        ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);

	if (!(clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT))
		return rate / *best_parent_rate;

	/* the range of values is 3-258 (i+3) */
	for (i = 3; i < ((1 << width) + 3 - 1); i++) {
		if (rate == orig_parent_rate * i) {
			/*
			 * This is the best case for us if we have a
			 * perfect match without changing the parent
			 * rate.
			 */
			*best_parent_rate = orig_parent_rate;
			return i;
		}

		parent_rate = clk_hw_round_rate(clk_hw_get_parent(hw),
						rate / i);
		current_rate = parent_rate * i;

		if (__is_best_rate(rate, current_rate, best_rate, flags)) {
			bestmult = i;
			best_rate = current_rate;
			*best_parent_rate = parent_rate;
		}
	}

	return bestmult;
}

static long cortina_clk_divn_multiplier_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *parent_rate)
{
	struct cortina_clk_divn_multiplier *mult = to_cortina_clk_divn_multiplier(hw);
	unsigned long factor = __bestmult(hw, rate, parent_rate,
					  mult->width, mult->flags);
	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
        ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
        ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);

        ca7774_clk_dev_dbg("%s factor returned is %d\n", __func__, factor);

	return *parent_rate * factor;
}

static int cortina_clk_divn_multiplier_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct cortina_clk_divn_multiplier *mult = to_cortina_clk_divn_multiplier(hw);
	unsigned long factor = __get_mult(mult, rate, parent_rate);
	unsigned long flags = 0;
	unsigned long val;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);
        ca7774_clk_dev_dbg("%s parent rate is %d\n", __func__, parent_rate);
        ca7774_clk_dev_dbg("%s requested rate is %d\n", __func__, rate);

	/* range is 3-258, when values is 0-255. i+3 */
	
	/* are we out of range, if so return "invalid" */
	if((factor < 3) || (factor > 258))
	 return 1;
	else
	 factor -= 3;

	if (mult->lock)
		spin_lock_irqsave(mult->lock, flags);
	else
		__acquire(mult->lock);

	val = clk_readl(mult->reg);
	val &= ~GENMASK(mult->width + mult->shift - 1, mult->shift);
	val |= factor << mult->shift;
	clk_writel(val, mult->reg);

	if (mult->lock)
		spin_unlock_irqrestore(mult->lock, flags);
	else
		__release(mult->lock);

	return 0;
}

const struct clk_ops cortina_clk_divn_multiplier_ops = {
	.recalc_rate	= cortina_clk_divn_multiplier_recalc_rate,
	.round_rate	= cortina_clk_divn_multiplier_round_rate,
	.set_rate	= cortina_clk_divn_multiplier_set_rate,
};
EXPORT_SYMBOL_GPL(cortina_clk_divn_multiplier_ops);

static struct clk_hw *_register_cortina_divn_multiplier(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 cortina_clk_divn_multiplier_flags, const struct clk_div_table *table,
		spinlock_t *lock)
{
	struct cortina_clk_divn_multiplier *divn;
	struct clk_hw *hw;
	struct clk *clk;
	struct clk_init_data init;

	ca7774_clk_dev_dbg("%s line %d\n", __func__, __LINE__);

	if (cortina_clk_divn_multiplier_flags & CLK_DIVIDER_HIWORD_MASK) {
		if (width + shift > 16) {
			pr_warn("divn_multiplier value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the divn_multiplier */
	divn = kzalloc(sizeof(*divn), GFP_KERNEL);
	if (!divn)
		return ERR_PTR(-ENOMEM);

	init.name = name;
#if 0
	if (cortina_clk_divn_multiplier_flags & CLK_DIVIDER_READ_ONLY)
		init.ops = &cortina_clk_divn_multiplier_ro_ops;
	else
		init.ops = &cortina_clk_divn_multiplier_ops;
#else
		init.ops = &cortina_clk_divn_multiplier_ops;
#endif
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct cortina_clk_divn_multiplier assignments */
	divn->reg = reg;
	divn->shift = shift;
	divn->width = width;
	divn->flags = cortina_clk_divn_multiplier_flags;
	divn->lock = lock;
	divn->hw.init = &init;

	/* register the clock */
	hw = &divn->hw;
	clk = clk_register(dev, hw);
        if (IS_ERR(clk)) {
                pr_err("%s: could not register clk %s\n", __func__, name);
                kfree(divn);
                return NULL;
        }

	return hw;
}

/**
 * cortina_clk_register_divn_multiplier - register a divn_multiplier clock with the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divn_multiplier
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @cortina_clk_divn_multiplier_flags: divn_multiplier-specific flags for this clock
 * @lock: shared register lock for this clock
 */
struct clk *cortina_clk_register_divn_multiplier(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 cortina_clk_divn_multiplier_flags, spinlock_t *lock)
{
	struct clk_hw *hw;

	hw =  _register_cortina_divn_multiplier(dev, name, parent_name, flags, reg, shift,
			width, cortina_clk_divn_multiplier_flags, NULL, lock);
	if (IS_ERR(hw))
		return ERR_CAST(hw);
	return hw->clk;
}
EXPORT_SYMBOL_GPL(cortina_clk_register_divn_multiplier);

/**
 * cortina_clk_hw_register_divn_multiplier - register a divn_multiplier clock with the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divn_multiplier
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @cortina_clk_divn_multiplier_flags: divn_multiplier-specific flags for this clock
 * @lock: shared register lock for this clock
 */
struct clk_hw *cortina_clk_hw_register_divn_multiplier(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 cortina_clk_divn_multiplier_flags, spinlock_t *lock)
{
	return _register_cortina_divn_multiplier(dev, name, parent_name, flags, reg, shift,
			width, cortina_clk_divn_multiplier_flags, NULL, lock);
}
EXPORT_SYMBOL_GPL(cortina_clk_hw_register_divn_multiplier);

/**
 * cortina_clk_register_divn_multiplier_table - register a table based divn_multiplier clock with
 * the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divn_multiplier
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @cortina_clk_divn_multiplier_flags: divn_multiplier-specific flags for this clock
 * @table: array of divn_multiplier/value pairs ending with a div set to 0
 * @lock: shared register lock for this clock
 */
struct clk *cortina_clk_register_divn_multiplier_table(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 cortina_clk_divn_multiplier_flags, const struct clk_div_table *table,
		spinlock_t *lock)
{
	struct clk_hw *hw;

	hw =  _register_cortina_divn_multiplier(dev, name, parent_name, flags, reg, shift,
			width, cortina_clk_divn_multiplier_flags, table, lock);
	if (IS_ERR(hw))
		return ERR_CAST(hw);
	return hw->clk;
}
EXPORT_SYMBOL_GPL(cortina_clk_register_divn_multiplier_table);

/**
 * cortina_clk_hw_register_divn_multiplier_table - register a table based divn_multiplier clock with
 * the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divn_multiplier
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @cortina_clk_divn_multiplier_flags: divn_multiplier-specific flags for this clock
 * @table: array of divn_multiplier/value pairs ending with a div set to 0
 * @lock: shared register lock for this clock
 */
struct clk_hw *cortina_clk_hw_register_divn_multiplier_table(struct device *dev,
		const char *name, const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 cortina_clk_divn_multiplier_flags, const struct clk_div_table *table,
		spinlock_t *lock)
{
	return _register_cortina_divn_multiplier(dev, name, parent_name, flags, reg, shift,
			width, cortina_clk_divn_multiplier_flags, table, lock);
}
EXPORT_SYMBOL_GPL(cortina_clk_hw_register_divn_multiplier_table);

void cortina_clk_unregister_divn_multiplier(struct clk *clk)
{
	struct cortina_clk_divn_multiplier *divn;
	struct clk_hw *hw;

	hw = __clk_get_hw(clk);
	if (!hw)
		return;

	divn = to_cortina_clk_divn_multiplier(hw);

	clk_unregister(clk);
	kfree(divn);
}
EXPORT_SYMBOL_GPL(cortina_clk_unregister_divn_multiplier);

