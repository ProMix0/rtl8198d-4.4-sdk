/*
 * FILE NAME  per_timer.c
 *
 * BRIEF MODULE DESCRIPTION
 *  Driver for Cortina Access Peripheral Timer.
 *
 * Copyright (C) 2020 Cortina Access, Inc.
 *              http://www.cortina-access.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/sched_clock.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/clocksource.h>
#include <linux/slab.h>

#define PER_TMR_LD	0x00
#define PER_TMR_CTRL	0x04
#define PER_TMR_CNT	0x08
#define PER_TMR_IE0	0x0c
#define PER_TMR_INT0	0x14
#define PER_TMR_LOADE   0x00

/*
 * struct cortina_pt - Peripheral Timer structure
 * @timer_reg_base: baseaddress of device
 * @timer_loade_base: baseaddress of PER_TMR_LOADE
 * @cept: clockevent device
 * @per_timer_irq: interrupt struct
 * @irq: irq number
 * @clk: struct clk * of a clock source
 * @clk_rate: clock rate (125MHz)
 * @cycle_per_jiffy: cycle per jiffy
 * @id: timer id
 *
 * Structure containing parameters specific to cortina peripheral timer.
 */
struct cortina_pt {
	void __iomem *timer_reg_base;
	void __iomem *timer_loade_base;
	struct clock_event_device cept;
	struct irqaction per_timer_irq;
	int irq;
	struct clk *per_clk;
	unsigned long clk_rate;
	unsigned long cycle_per_jiffy;
	int id;

};
struct cortina_pt *pt[4];	/* cortina peripheral timers */

inline void per_timer_enable(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);
	u32 temp = 0;

	temp = __raw_readl(pt->timer_reg_base + PER_TMR_CTRL);
	temp |= (0x1 << 7);
	__raw_writel(temp, pt->timer_reg_base + PER_TMR_CTRL);
}

inline void per_timer_disable(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);
	u32 temp = 0;

	temp = __raw_readl(pt->timer_reg_base + PER_TMR_CTRL);
	temp &= ~(0x1 << 7);
	__raw_writel(temp, pt->timer_reg_base + PER_TMR_CTRL);
}

/************************************** clock source ***************************************/
static u64 notrace per_timer_read_sched_clock(void)
{
	return ~__raw_readl(pt[0]->timer_reg_base + PER_TMR_CNT);
}

static u64 notrace per_timer2_read_sched_clock(void)
{
	return ~__raw_readl(pt[1]->timer_reg_base + PER_TMR_CNT);
}

static u64 notrace per_timer3_read_sched_clock(void)
{
	return ~__raw_readl(pt[2]->timer_reg_base + PER_TMR_CNT);
}

static u64 notrace per_timer4_read_sched_clock(void)
{
	return ~__raw_readl(pt[3]->timer_reg_base + PER_TMR_CNT);
}

static int __init per_clocksource_init(struct cortina_pt *pt)
{
	u64(*func_ptr[4]) (void);

	/* assign clocksource read function */
	func_ptr[0] = per_timer_read_sched_clock;
	func_ptr[1] = per_timer2_read_sched_clock;
	func_ptr[2] = per_timer3_read_sched_clock;
	func_ptr[3] = per_timer4_read_sched_clock;

	/* set the max load value and start the clock source counter */
	per_timer_disable(&pt->cept);

	__raw_writel(0xffffffff, pt->timer_reg_base + PER_TMR_LD);
	__raw_writel((0x1 << (pt->id)), pt->timer_loade_base + PER_TMR_LOADE);	/* update timer counter with PER_TMR_LD value */

	per_timer_enable(&pt->cept);

	sched_clock_register(func_ptr[(pt->id) - 1], 32, pt->clk_rate);
	return clocksource_mmio_init(pt->timer_reg_base + PER_TMR_CNT, "apb",
				     pt->clk_rate, 300, 32,
				     clocksource_mmio_readl_down);
}

/************************************** clock event ***************************************/
inline void per_timer_irq_acknowledge(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);

	__raw_writel(0x1, pt->timer_reg_base + PER_TMR_IE0);	/* enable interrupt */
}

int per_timer_set_next_event(unsigned long delta,
			     struct clock_event_device *evt)
{
	u32 temp = 0;
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);

	per_timer_irq_acknowledge(&pt->cept);

	per_timer_disable(&pt->cept);

	/* clear interrupt */
	__raw_writel(0x1, pt->timer_reg_base + PER_TMR_INT0);

	/* update timer counter with PER_TMR_LD value */
	__raw_writel(delta - 1, pt->timer_reg_base + PER_TMR_LD);
	__raw_writel((0x1 << (pt->id)), pt->timer_loade_base + PER_TMR_LOADE);

	/* enable relmode */
	temp = __raw_readl(pt->timer_reg_base + PER_TMR_CTRL);
	temp |= 0x1 << 6;
	__raw_writel(temp, pt->timer_reg_base + PER_TMR_CTRL);

	per_timer_enable(&pt->cept);

	return 0;
}

int per_timer_shutdown(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);

	per_timer_disable(&pt->cept);
	return 0;
}

int per_timer_set_periodic(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);

	per_timer_set_next_event(pt->cycle_per_jiffy, evt);

	return 0;
}

int per_timer_set_oneshot(struct clock_event_device *evt)
{
	struct cortina_pt *pt = container_of(evt, struct cortina_pt, cept);
	u32 temp = 0;

	per_timer_set_next_event(pt->cycle_per_jiffy, evt);

	/* disable relmode */
	temp = __raw_readl(pt->timer_reg_base + PER_TMR_CTRL);
	temp &= ~(0x1 << 6);
	__raw_writel(temp, pt->timer_reg_base + PER_TMR_CTRL);

	return 0;
}

static irqreturn_t per_timer_interrupt(int irq, void *dev_id)
{
	struct cortina_pt *pt = container_of(dev_id, struct cortina_pt, cept);
	u32 temp = 0;
	if (likely(clockevent_state_oneshot(&pt->cept))) {
		/* disable interrupt */
		__raw_writel(0x0, pt->timer_reg_base + PER_TMR_IE0);
	}

	/* clear interrupt */
	__raw_writel(0x1, pt->timer_reg_base + PER_TMR_INT0);
	return IRQ_HANDLED;
}

static int __init per_clockevent_init(struct cortina_pt *pt)
{
	/* init clockevent */
	clockevents_calc_mult_shift(&pt->cept, pt->clk_rate, 0x7fffffff);
	pt->cept.cpumask = cpumask_of(0);
	pt->cept.irq = pt->irq;
	pt->cept.max_delta_ns = clockevent_delta2ns(0x7fffffff, &pt->cept);
	pt->cept.min_delta_ns = clockevent_delta2ns(5000, &pt->cept);
	pt->cept.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	pt->cept.set_state_shutdown = per_timer_shutdown;
	pt->cept.set_state_periodic = per_timer_set_periodic;
	pt->cept.set_state_oneshot = per_timer_set_oneshot;
	pt->cept.set_next_event = per_timer_set_next_event;
	pt->cept.rating = 300;
	pt->cept.name = "per-timer-clockevent";

	/* init interrupt */
	pt->per_timer_irq.name = "per-timer-interrupt";
	pt->per_timer_irq.handler = per_timer_interrupt;
	pt->per_timer_irq.dev_id = &pt->cept;
	pt->per_timer_irq.irq = pt->irq;
	pt->per_timer_irq.flags = IRQF_TIMER | IRQF_IRQPOLL;

	per_timer_disable(&pt->cept);
	__raw_writel(0x1, pt->timer_reg_base + PER_TMR_IE0);	/* enable interrupt */

	WARN_ON(setup_irq(pt->irq, &pt->per_timer_irq));

	clockevents_config_and_register(&pt->cept, pt->clk_rate, 2, 0xffffffff);

	return 0;
}

static void __init ca_timer_init(struct device_node *np)
{
	int index;

	if (of_property_read_u32(np, "id", &index)) {
		printk("missing id property\n");
	}
	index--;

	/* allocate cortina_pt struce */
	pt[index] = kzalloc(sizeof(*(pt[index])), GFP_KERNEL);
	if (!pt[index]) {
		pr_warn("Can't allocate clock event driver struct");
		return;
	}

	/* timer id  */
	pt[index]->id = index + 1;

	/* timer base */
	pt[index]->timer_reg_base = of_iomap(np, 0);
	WARN_ON(!pt[index]->timer_reg_base);

	/* PER_TMR_LOADE base */
	pt[index]->timer_loade_base = of_iomap(np, 1);
	WARN_ON(!pt[index]->timer_loade_base);

	/* interrupt */
	pt[index]->irq = irq_of_parse_and_map(np, 0);
	WARN_ON(pt[index]->irq <= 0);

	/* clock */
	pt[index]->per_clk = of_clk_get(np, 0);
	WARN_ON(IS_ERR(pt[index]->per_clk));
	WARN_ON(clk_prepare_enable(pt[index]->per_clk));

	/* clock rate & cycle per jiffy */
	pt[index]->clk_rate = clk_get_rate(pt[index]->per_clk);
	pt[index]->cycle_per_jiffy = pt[index]->clk_rate / (HZ);

	/* enable the pit module */
	per_timer_enable(&pt[index]->cept);

	per_clocksource_init(pt[index]);

	per_clockevent_init(pt[index]);
}

CLOCKSOURCE_OF_DECLARE(per_timer, "cortina,per_timer", ca_timer_init);
