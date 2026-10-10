/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.c
 *     bsp interrupt initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <dt-bindings/interrupt-controller/mips-gic.h>

#include <rtk/init.h>
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#include "bspchip.h"

#define GISR_IRQ_BASE 		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS)
#define GIC_EXT_TO_IRQ(x) 	(x + GISR_IRQ_BASE)
#define IRQ_TO_GIC_EXT(x) 	(x - GISR_IRQ_BASE)

#define IRQ_PERIPHERAL 12
/*9603cvd irq controller */
static const char gisr_translate_c2d[64] = { /* translate mapping from 03c->03cvd */
    43,44,45,46,47,48,60,61, /*  0- 7  */
    62,17,18,28,29,-1,-1, 8, /*  8-15 */
    24,14,32,-1,-1,42,41,49, /* 16-23 */
    50,51,52,33,34,-1,-1,-1, /* 24-31 */
    -1,39,40, 5,26,27,-1,-1, /* 32-39 */
    -1,-1,24,25,31,-1,-1,-1, /* 40-47 */
    -1,-1,16,-1,-1, 4,-1,-1, /* 48-55 */
    -1,-1,-1,-1,-1,-1,-1,-1	 /* 56-63 */
};
static const unsigned long gimr_bases[2] = { 0xb8003000, 0xb8003040 };
static const unsigned long gisr_bases[2] = { 0xb8003008, 0xb8003048 };
static const unsigned long girr_bases[2] = { 0xb8003010, 0xb8003050 };

//static char gisr_cpu_map[64];
static char gisr_translate_d2c[64];

#define GIMR(idx,cpu)  REG32((gimr_bases[cpu]+(idx<<2)))
#define GISR(idx,cpu)  REG32((gisr_bases[cpu]+(idx<<2)))
#define GIRR(idx,cpu)  REG32((girr_bases[cpu]+(idx<<2)))

static void mask_rtk_irq(struct irq_data *d)
{
    int irq = gisr_translate_c2d[IRQ_TO_GIC_EXT(d->irq)];
    u32 offset, pos, cpu;

    if (unlikely(irq<0)) {
        return;
    }

    cpu = smp_processor_id();
    offset = (irq >> 5);
    pos = irq & 0x1f;

    GIMR(offset, cpu) &= ~(1<<pos);
}

static void unmask_rtk_irq(struct irq_data *d)
{
    int irq = gisr_translate_c2d[IRQ_TO_GIC_EXT(d->irq)];
    u32 offset, pos, cpu;

    if (unlikely(irq<0)) {
        return;
    }

    cpu = smp_processor_id();

    offset = (irq >> 5);
    pos = irq & 0x1f;

    GIMR(offset, cpu) |= (1<<pos);
}

#ifdef CONFIG_SMP
static DEFINE_SPINLOCK(irq_lock);
static int set_affinity(struct irq_data *d, const struct cpumask *cpumask, bool force)
{
    int irq = gisr_translate_c2d[IRQ_TO_GIC_EXT(d->irq)];
    cpumask_t	tmp = CPU_MASK_NONE;
    unsigned long	flags;
    u32 offset, pos, cpu;

    if (irq < 0)
        return -EINVAL;

    cpumask_and(&tmp, cpumask, cpu_online_mask);
    if (cpumask_empty(&tmp))
        return -EINVAL;

    spin_lock_irqsave(&irq_lock, flags);
    offset = (irq >> 5);
    pos = irq & 0x1f;

    cpu = cpumask_first(&tmp);
    if (cpu) {
        GIMR(offset, 0) &= ~(1<<pos);
        GIMR(offset, 1) |= (1<<pos);
    } else {
        GIMR(offset, 1) &= ~(1<<pos);
        GIMR(offset, 0) |= (1<<pos);
    }

    cpumask_copy(irq_data_get_affinity_mask(d), cpumask);
    spin_unlock_irqrestore(&irq_lock, flags);

    return IRQ_SET_MASK_OK_NOCOPY;
}
#endif

static struct irq_chip rtk_irq_chip = {
    .name = "rtkgic",
    .irq_ack = mask_rtk_irq,
    .irq_mask = mask_rtk_irq,
    .irq_mask_ack = mask_rtk_irq,
    .irq_unmask = unmask_rtk_irq,
    .irq_eoi = unmask_rtk_irq,
    .irq_disable = mask_rtk_irq,
    .irq_enable = unmask_rtk_irq,
#ifdef CONFIG_SMP
    .irq_set_affinity	=	set_affinity,
#endif
};

static void rtk_ip2_dispatch(void)
{
    printk("ip2: %x %x/%x %x\n", REG32(0xB8003000),REG32(0xB8003004), REG32(0xB8003008),REG32(0xB800300C));
};

static void rtk_ip3_dispatch(void)
{
    unsigned long gisr, gimr;
    int bit, irq;

    gisr = GISR(0, smp_processor_id());
    gimr = GIMR(0, smp_processor_id()) & ~(1<<IRQ_PERIPHERAL);
    gisr &= gimr;

    while ((bit = ffs(gisr))>0) {
        bit--;
        gisr &= ~(1<<bit);
        irq = GIC_EXT_TO_IRQ(gisr_translate_d2c[bit]);
        //printk("%s(%d): irq=%d gisr=%x bit=%d\n",__func__,__LINE__,irq,gisr,bit);
        do_IRQ(irq);
    }
};

static void rtk_ip4_dispatch(void)
{
    unsigned long gisr, gimr;
    int bit, irq;

    gisr = GISR(1, smp_processor_id());
    gimr = GIMR(1, smp_processor_id());
    gisr &= gimr;

    while ((bit = ffs(gisr))>0) {
        bit--;
        irq = GIC_EXT_TO_IRQ(gisr_translate_d2c[bit+32]);
        //printk("%s(%d): irq=%d gisr=%x bit=%d\n",__func__,__LINE__,irq,gisr,bit);
        gisr &= ~(1<<bit);

        do_IRQ(irq);
    }
};

static void rtk_ip5_dispatch(void)
{
    printk("ip5: %x %x/%x %x\n", REG32(0xB8003000),REG32(0xB8003004), REG32(0xB8003008),REG32(0xB800300C));
};

static void rtk_ip6_dispatch(void)
{
    printk("ip6: %x %x/%x %x\n", REG32(0xB8003000),REG32(0xB8003004), REG32(0xB8003008),REG32(0xB800300C));
};

static void rtk_ip7_dispatch(void)
{
    static const unsigned int cpu_timer_irq_nr[2] = { GIC_EXT_TO_IRQ(GIC_EXT_TC0), GIC_EXT_TO_IRQ(GIC_EXT_TC1) };

    //printk("ip7: %x %x/%x %x\n", REG32(0xB8003000),REG32(0xB8003004), REG32(0xB8003008),REG32(0xB800300C));
    do_IRQ(cpu_timer_irq_nr[smp_processor_id()]);
};

static void rtk_cp0_interrupt_init(void)
{
    printk("II: Initializing CPU%d CP0 STATUS/CAUSE/COMPARE...\n", smp_processor_id());

    clear_c0_status(ST0_IM);
    clear_c0_cause(CAUSEF_IP);
    write_c0_compare(0);
    change_c0_status(ST0_IM,
                     STATUSF_IP0 | STATUSF_IP1 | STATUSF_IP2 | STATUSF_IP3 |
                     STATUSF_IP4 | STATUSF_IP5 | STATUSF_IP6 | STATUSF_IP7);
    return;
}

#ifdef CONFIG_SMP

static void ipi_resched_dispatch(void)
{
    do_IRQ(0);
}

static void ipi_call_dispatch(void)
{
    do_IRQ(1);
}

static irqreturn_t ipi_resched_interrupt(int irq, void *dev_id)
{
    scheduler_ipi();
    return IRQ_HANDLED;
}

static irqreturn_t ipi_call_interrupt(int irq, void *dev_id)
{
    generic_smp_call_function_interrupt();
    return IRQ_HANDLED;
}

static struct irqaction irq_resched = {
    .handler = ipi_resched_interrupt,
    .flags = IRQF_PERCPU,
    .name = "IPI_resched"
};

static struct irqaction irq_call = {
    .handler = ipi_call_interrupt,
    .flags = IRQF_PERCPU,
    .name = "IPI_call"
};

static void __init rtl9603cvd_smp_init(void)
{

    /* mips_cpu_irq_init has been called by of_irq_init */
    //mips_cpu_irq_init();

    set_vi_handler(0, ipi_resched_dispatch);
    setup_irq(0, &irq_resched);
    irq_set_handler(0, handle_percpu_irq);

    set_vi_handler(1, ipi_call_dispatch);
    setup_irq(1, &irq_call);
    irq_set_handler(1, handle_percpu_irq);

}

static void rtl9603cvd_plat_smp_init_secondary(void)
{
    rtk_cp0_interrupt_init();
}

static void rtl9603cvd_plat_smp_exit_secondary(void)
{
    printk("%s(%d): CPU%d\n",__func__,__LINE__,smp_processor_id());
    //to-do: mask irqs
}

static int rtl9603cvd_irqchip_cpu_notify(struct notifier_block *nb, unsigned long action,
        void *data)
{
    if (action == CPU_STARTING || action == CPU_STARTING_FROZEN)
        rtl9603cvd_plat_smp_init_secondary();
    else if (action == CPU_DYING)
        rtl9603cvd_plat_smp_exit_secondary();

    return NOTIFY_OK;
}


static struct notifier_block rtl9603cvd_irqchip_cpu_notifier = {
    .notifier_call = rtl9603cvd_irqchip_cpu_notify,
    .priority = 100,
};

#else
static void __init rtl9603cvd_smp_init(void) {}
#endif /* CONFIG_SMP */

#if 1
static int gisr_irq_domain_map(struct irq_domain *d, unsigned int virq,
                               irq_hw_number_t hw)
{
    /* we mapped all the irq in init_rtl9603cvd_irq */
    return 0;
}

static int gisr_irq_domain_xlate(struct irq_domain *d, struct device_node *ctrlr,
                                 const u32 *intspec, unsigned int intsize,
                                 irq_hw_number_t *out_hwirq,
                                 unsigned int *out_type)
{
    if (intsize != 3)
        return -EINVAL;

    if (intspec[0] == GIC_SHARED)
        /* irq_domain_associate (called by irq_domain_add_simple) adds the irq base,
           so we pass the GIC_EXT_IRQ here */
        *out_hwirq = intspec[1];
    else if (intspec[0] == GIC_LOCAL)
        *out_hwirq = GIC_LOCAL_TO_HWIRQ(intspec[1]);
    else
        return -EINVAL;
    *out_type = intspec[2] & IRQ_TYPE_SENSE_MASK;

    return 0;
}

static const struct irq_domain_ops gisr_irq_domain_ops = {
    .map = gisr_irq_domain_map,
    .xlate = gisr_irq_domain_xlate,
};
#endif


void __init rtl9603cvd_gisr_init(struct device_node *node)
{
    int i;

    if (mips_cm_base) {
        printk("GIC is present. GISR will not be registered\n");
        return;
    }

    for (i=0; i<ARRAY_SIZE(gisr_translate_d2c); i++)
        gisr_translate_d2c[i]=-1;

    for (i=0; i<ARRAY_SIZE(gisr_translate_c2d); i++) {
        int irq;
        irq = gisr_translate_c2d[i];

        if (irq>=0)
            gisr_translate_d2c[irq] = i;
    }

    rtl9603cvd_smp_init();

    printk("%s: gisr_translate_d2c = \n", __func__);
    for (i=0; i<ARRAY_SIZE(gisr_translate_d2c); i++) {
        printk("%d=>%d\n", i, gisr_translate_d2c[i]);
    }

    GIMR(0,0) = GIMR(0,1) = (1<<IRQ_PERIPHERAL);/* Must be set to enable peripheral irq */
    GIMR(1,0) = GIMR(1,1) = 0;

    GIRR(0,0) = GIRR(0,1) = 0x03333330;
    GIRR(1,0) = GIRR(1,1) = 0x30302222;
    GIRR(2,0) = GIRR(2,1) = 0x00020222;
    GIRR(3,0) = GIRR(3,1) = 0x22020333;
    GIRR(4,0) = 0x33333063;
    GIRR(4,1) = 0x33333603;
    GIRR(5,0) = GIRR(5,1) = 0x32322022;
    GIRR(6,0) = GIRR(6,1) = 0x00333000;

    for (i=MIPS_GIC_IRQ_BASE+MIPS_GIC_LOCAL_INTRS; i<(MIPS_GIC_IRQ_BASE+MIPS_GIC_LOCAL_INTRS+64); i++) {
        irq_set_chip_and_handler(i, &rtk_irq_chip, handle_level_irq);
    }

    set_vi_handler(2, rtk_ip2_dispatch);
    set_vi_handler(3, rtk_ip3_dispatch);
    set_vi_handler(4, rtk_ip4_dispatch);
    set_vi_handler(5, rtk_ip5_dispatch);
    set_vi_handler(6, rtk_ip6_dispatch);
    set_vi_handler(7, rtk_ip7_dispatch);

    rtk_cp0_interrupt_init();

    //use gisr irq domain to act as gic irqdomain
    irq_domain_add_simple(node, 64, MIPS_GIC_IRQ_BASE+MIPS_GIC_LOCAL_INTRS,
                          &gisr_irq_domain_ops, NULL);

#ifdef CONFIG_SMP
    register_cpu_notifier(&rtl9603cvd_irqchip_cpu_notifier);
#endif
}

