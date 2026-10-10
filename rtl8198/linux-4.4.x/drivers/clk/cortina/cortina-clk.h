#ifndef __CORTINA_CLK_H
#define __CORTINA_CLK_H

#include <linux/spinlock.h>
#include <linux/clk-provider.h>

#ifdef DEBUG_CA7774_CLK
    #define ca7774_clk_dev_dbg(...) 	printk("ca7774_clk: " __VA_ARGS__ )
#else
    #define ca7774_clk_dev_dbg(...) 	({})
#endif /* DEBUG_CA7774_CLK */

extern spinlock_t cortina_glbclk_lock;

void cortina_check_clocks(struct clk *clks[], unsigned int count);

struct clk * cortina_obtain_fixed_clock(
			const char *name, unsigned long rate);


static inline struct clk *cortina_clk_fixed(const char *name, int rate)
{
	return clk_register_fixed_rate(NULL, name, NULL, CLK_IS_ROOT, rate);
}


#endif /* __CORTINA_CLK_H */
