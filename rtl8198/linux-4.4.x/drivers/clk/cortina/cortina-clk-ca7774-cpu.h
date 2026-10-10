/*
 *  Copyright (c) 2017-2018 Cortina Access Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __CORTINA_CLK_CA7774_CPU_H
#define __CORTINA_CLK_CA7774_CPU_H

#include <linux/io.h>
#include <linux/of.h>

struct clk *cortina_clk_register_ca7774_cpu(const char *name, const char *parent_name, void __iomem *base);

#endif /* __CORTINA_CLK_CA7774_CPU_H */
