/*
 *  Copyright (c) 2017-2018 Cortina Access Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __CORTINA_CLK_DIVN_MULTIPLIER_H
#define __CORTINA_CLK_DIVN_MULTIPLIER_H

#include <linux/io.h>
#include <linux/of.h>


/**
 * struct cortina_clk_divn_multiplier - adjustable multiplier clock
 *
 * @hw:		handle between common and hardware-specific interfaces
 * @reg:	register containing the multiplier
 * @shift:	shift to the multiplier bit field
 * @width:	width of the multiplier bit field
 * @lock:	register lock
 *
 * Clock with an adjustable multiplier affecting its output frequency.
 * Implements .recalc_rate, .set_rate and .round_rate
 *
 * Flags:
 * CORTINA_CLK_DIVN_MULTIPLIER_ZERO_BYPASS - By default, the multiplier is the value read
 *	from the register, with 0 being a valid value effectively
 *	zeroing the output clock rate. If CORTINA_CLK_DIVN_MULTIPLIER_ZERO_BYPASS is
 *	set, then a null multiplier will be considered as a bypass,
 *	leaving the parent rate unmodified.
 * CORTINA_CLK_DIVN_MULTIPLIER_ROUND_CLOSEST - Makes the best calculated divider to be
 *	rounded to the closest integer instead of the down one.
 */
struct cortina_clk_divn_multiplier {
	struct clk_hw	hw;
	void __iomem	*reg;
	u8		shift;
	u8		width;
	u8		flags;
	spinlock_t	*lock;
};

#define CORTINA_CLK_DIVN_MULTIPLIER_ZERO_BYPASS		BIT(0)
#define CORTINA_CLK_DIVN_MULTIPLIER_ROUND_CLOSEST	BIT(1)

extern const struct clk_ops cortina_clk_divn_multiplier_ops;

struct clk *cortina_clk_register_divn_multiplier(struct device *dev, const char *name,
                const char *parent_name, unsigned long flags,
                void __iomem *reg, u8 shift, u8 width,
                u8 cortina_clk_divn_multiplier_flags, spinlock_t *lock);

#endif /* __CORTINA_CLK_DIVN_MULTIPLIER_H */
