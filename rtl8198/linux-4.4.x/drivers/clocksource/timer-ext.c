/*
 * Realtek Semiconductor Corp.
 *
 * bsp/timer.c
 *     bsp timer initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/export.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip/mips-gic.h>
#include <asm/time.h>


#define LX_FREQ 200000000
#define DIVISOR_CLOCKSOURCE 20

enum tcid {
    vpe0 = 0,
    vpe1,
    clksrc,
};

#define DIVISOR_CLOCKEVENT 1000
#if DIVISOR_CLOCKEVENT > (1 << 16)
#error "Exceed the Maximum Value of DivFactor"
#endif

#define BSP_MHZ 200

#define TC_MAX 	8
#define TC0DATA 0x00
#define TC0CNT 	0x04
#define TC0CTL 	0x08
#define TC0INT 	0x0c

#define TCEN          (1 << 28)
#define TCMODE_TIMER  (1 << 24)
#define TCDIV_FACTOR  (0xFFFF << 0)
#define TCIE          (1 << 20)
#define TCIP          (1 << 16)

#define TCDATA(x) 	(TC0DATA+(x)*0x10)
#define TCCNT(x) 	(TC0CNT+(x)*0x10)
#define TCCTL(x) 	(TC0CTL+(x)*0x10)
#define TCINT(x) 	(TC0INT+(x)*0x10)

static void __iomem *ext_timer_base;
static unsigned int ext_timer_irq;

/* clock source */
static cycle_t ext_tick_read(struct clocksource *cs)
{
    return readl(ext_timer_base + TCCNT(clksrc));
}

static struct clocksource clocksource_ext = {
    .name		= "EXT",
    .read		= ext_tick_read,
    .mask		= CLOCKSOURCE_MASK(28),
    .flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int __init ext_clocksource_init(void)
{
    u32 freq = LX_FREQ/DIVISOR_CLOCKSOURCE;
    u32 ctlv;

    writel(0xFFFFFFF, ext_timer_base + TCDATA(clksrc));

    ctlv = TCEN | TCMODE_TIMER | DIVISOR_CLOCKSOURCE;
    writel(ctlv, ext_timer_base + TCCTL(clksrc));

    clocksource_ext.rating = 200 + freq / 10000000;
    clocksource_register_hz(&clocksource_ext, freq);

    pr_debug("%s: ", __func__);
    pr_debug("TC%dCTL: 0x%x, ", clksrc, readl(ext_timer_base + TCCTL(clksrc)));
    pr_debug("TC%dCNT: 0x%x, ", clksrc, readl(ext_timer_base + TCCNT(clksrc)));
    pr_debug("TC%dDATA: 0x%x\n", clksrc, readl(ext_timer_base + TCDATA(clksrc)));

    return 0;
}

/* clock event device */
DEFINE_PER_CPU(struct clock_event_device, ext_clockevent_device);

static int ext_timer_set_next_event(unsigned long delta,
                                    struct clock_event_device *evt)
{
    return -EINVAL;
}

static void ext_timer_event_handler(struct clock_event_device *dev)
{
}

static irqreturn_t ext_timer_interrupt(int irq, void *dev_id)
{
    struct clock_event_device *cd = this_cpu_ptr(&ext_clockevent_device);
    u32 val;
    u32 cpu;

    cpu = smp_processor_id();
    val = readl(ext_timer_base + TCINT(cpu));

    /* clear pending interrupt */
    writel(val | TCIP, ext_timer_base + TCINT(cpu));

    cd->event_handler(cd);

    return IRQ_HANDLED;
}

static struct irqaction ext_irqaction = {
    .handler	= ext_timer_interrupt,
    .flags = IRQF_PERCPU | IRQF_TIMER,
    .name		= "EXT_TIMER",
};

static int ext_clockevent_init(struct clock_event_device *cd)
{
    u32 cpu = smp_processor_id();
    int ret;

    cd->name		= "EXT_TIMER";
    cd->features	= CLOCK_EVT_FEAT_PERIODIC;
    cd->event_handler	= ext_timer_event_handler;
    cd->set_next_event	= ext_timer_set_next_event;
    cd->rating = 100;
    cd->irq = ext_timer_irq + cpu;

    clockevent_set_clock(cd, 32768);
    cd->max_delta_ns = clockevent_delta2ns(0x7fffffff, cd);
    cd->min_delta_ns = clockevent_delta2ns(0x300, cd);
    cd->cpumask = cpumask_of(cpu);

    clockevents_register_device(cd);

    /* setup irq */
    ret = setup_irq(cd->irq, &ext_irqaction);
    if (ret < 0) {
        pr_err("%s: setup irq error\n", __func__);
        return ret;
    }
    irq_set_handler(cd->irq, handle_percpu_irq);

    /* enalbe TC hw interrupt */
    writel(TCIE, ext_timer_base + TCINT(cpu));

    return 0;
}

static void ext_clockevent_exit(struct clock_event_device *cd)
{
    disable_irq(cd->irq);
}

static void ext_clockevent_init_notify(void)
{
    int ret;
    /* init per cpu clockevent device*/
    ret = ext_clockevent_init(this_cpu_ptr(&ext_clockevent_device));
    if (ret)
        pr_err("%s: init clock event device for cpu %d failed\n", __func__, smp_processor_id());
}

static void ext_clockevent_exit_notify(void)
{
    ext_clockevent_exit(this_cpu_ptr(&ext_clockevent_device));
}

static int ext_cpu_notifier(struct notifier_block *nb, unsigned long action,
                            void *data)
{
    switch (action & ~CPU_TASKS_FROZEN) {
    case CPU_STARTING:
        ext_clockevent_init_notify();
        break;
    case CPU_DYING:
        ext_clockevent_exit_notify();
        break;
    }

    return NOTIFY_OK;
}

static struct notifier_block ext_cpu_nb = {
    .notifier_call = ext_cpu_notifier,
};

/* TC HW configuration */
static void __init ext_timer_hw_init(void)
{
    u32 datav = 0;
    u32 ctlv = 0;
    u32 val;
    int i;

    datav = ( (BSP_MHZ * 1000000) / ((int) DIVISOR_CLOCKEVENT * HZ) );

    for (i=0; i<NR_CPUS && i<TC_MAX; i++) {

        /* clear pending interrupt */
        val = readl(ext_timer_base + TCINT(i));
        if (val & TCIP) {
            writel(val | TCIP, ext_timer_base + TCINT(i));
        }

        /* disable timer before setting CDBR */
        writel(0x0, ext_timer_base + TCCTL(i));
        writel(datav, ext_timer_base + TCDATA(i));
    }

    /* Enable timer for all VPEs at one time. Let the count of all timer to be close to each other */
    ctlv = (TCEN | TCMODE_TIMER | DIVISOR_CLOCKEVENT);
    for (i=0; i<NR_CPUS && i<TC_MAX; i++) {
        writel(ctlv,ext_timer_base + TCCTL(i));
    }

    for(i=0; i<NR_CPUS && i<TC_MAX; i++) {
        pr_debug("TC%dCTL: 0x%x\n", i, readl(ext_timer_base + TCCTL(i)));
        pr_debug("TC%dINT: 0x%x\n", i, readl(ext_timer_base + TCINT(i)));
        pr_debug("TC%dDATA: 0x%x\n", i, readl(ext_timer_base + TCDATA(i)));
    }

}

static void __init ext_timer_init(struct device_node *node)
{
    int ret;

    /* use gic timer if gic presents */
    if (gic_present)
        return;

    pr_debug("%s: init clock event timer\n", __func__);

    ext_timer_base = of_iomap(node, 0);
    if (!ext_timer_base) {
        pr_err("%s: failed to map registers\n", __func__);
        return;
    }
    pr_debug("%s: ext timer base = 0x%x\n", __func__, (u32)ext_timer_base);

    ext_timer_irq = irq_of_parse_and_map(node, 0);
    if (!ext_timer_irq) {
        pr_err("%s: ext timer IRQ not specified.\n", __func__);
        return;
    }
    pr_debug("%s: ext timer irq = %d\n", __func__, ext_timer_irq);

    /* register clock source */
    ext_clocksource_init();

    /* TC0, TC1, ... hw init */
    ext_timer_hw_init();

    /* init clock event device */
    ret = ext_clockevent_init(this_cpu_ptr(&ext_clockevent_device));
    if (ret < 0) {
        pr_err("%s: init clock event device for cpu %d failed\n", __func__, smp_processor_id());
        return;
    }

    /* register cpu notifier to creat clockevent device for secondary VPEs */
    ret = register_cpu_notifier(&ext_cpu_nb);
    if (ret < 0)
        pr_warn("%s: unable to register CPU notifier\n", __func__);

}

CLOCKSOURCE_OF_DECLARE(ext_timer, "rtk,ext", ext_timer_init);
