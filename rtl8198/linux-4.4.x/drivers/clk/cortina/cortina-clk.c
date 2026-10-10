#include <linux/clk.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "cortina-clk.h"

DEFINE_SPINLOCK(cortina_glbclk_lock);

void __init cortina_check_clocks(struct clk *clks[], unsigned int count)
{
	unsigned i;

	for (i = 0; i < count; i++)
		if (IS_ERR(clks[i]))
			pr_err("cortina clk %u: register failed with %ld\n",
			       i, PTR_ERR(clks[i]));
}

#if 0
static struct clk * __init cortina_obtain_fixed_clock_from_dt(const char *name)
{
	struct of_phandle_args phandle;
	struct clk *clk = ERR_PTR(-ENODEV);
	char *path;

	path = kasprintf(GFP_KERNEL, "/clocks/%s", name);
	if (!path)
		return ERR_PTR(-ENOMEM);

	phandle.np = of_find_node_by_path(path);
	kfree(path);

	if (phandle.np) {
		clk = of_clk_get_from_provider(&phandle);
		of_node_put(phandle.np);
	}
	return clk;
}
#endif

struct clk * __init cortina_obtain_fixed_clock(
			const char *name, unsigned long rate)
{
	struct clk *clk;

	clk = cortina_obtain_fixed_clock_from_dt(name);
	if (IS_ERR(clk))
		clk = cortina_clk_fixed(name, rate);
	return clk;
}
