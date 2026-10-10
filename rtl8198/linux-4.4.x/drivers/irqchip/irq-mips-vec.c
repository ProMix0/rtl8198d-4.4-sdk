/*
 * Realtek Semiconductor Corp.
 *
 * This file define the irq handler for RLX CPU interrupts.
 *
 * Tony Wu (tonywu@realtek.com)
 * Feb. 28, 2008
 * Copyright (c) Cortina-Access Limited 2017.  All rights reserved.
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>

#include <asm/irq_vec.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/setup.h>

static inline void unmask_mips_vec_irq(struct irq_data *d)
{
	set_lxc0_estatus(0x10000 << (d->irq - MIPS_VEC_IRQ_BASE));
	irq_enable_hazard();
}

static inline void mask_mips_vec_irq(struct irq_data *d)
{
	clear_lxc0_estatus(0x10000 << (d->irq - MIPS_VEC_IRQ_BASE));
	irq_disable_hazard();
}

static struct irq_chip mips_vec_irq_controller = {
	.name		= "LOPI",
	.irq_ack	= mask_mips_vec_irq,
	.irq_mask	= mask_mips_vec_irq,
	.irq_mask_ack	= mask_mips_vec_irq,
	.irq_unmask	= unmask_mips_vec_irq,
	.irq_eoi	= unmask_mips_vec_irq,
	.irq_disable	= mask_mips_vec_irq,
	.irq_enable	= unmask_mips_vec_irq,
};

asmlinkage void mips_do_lopi_IRQ(int irq_offset)
{
	do_IRQ(MIPS_VEC_IRQ_BASE + irq_offset);
}

static int mips_vec_intc_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hw)
{
	static struct irq_chip *chip;

    chip = &mips_vec_irq_controller;

	irq_set_chip_and_handler(irq, chip, handle_percpu_irq);

	return 0;
}


static const struct irq_domain_ops mips_vec_intc_irq_domain_ops = {
	.map = mips_vec_intc_map,
	.xlate = irq_domain_xlate_onecell,
};


int __init mips_vec_irq_of_init(struct device_node *of_node,
				struct device_node *parent)
{
    struct irq_domain *domain;
	extern char handle_vec;

	/* Mask interrupts. */
	clear_lxc0_estatus(EST0_IM);
	clear_lxc0_ecause(ECAUSEF_IP);


	domain = irq_domain_add_legacy(of_node, 8, MIPS_VEC_IRQ_BASE, 0,
				       &mips_vec_intc_irq_domain_ops, NULL);
	if (!domain)
		panic("Failed to add irqdomain for MIPS Vec");

	write_lxc0_intvec(&handle_vec);
	return 0;
}

IRQCHIP_DECLARE(vec_intc, "cortina,vec-interrupt-controller", mips_vec_irq_of_init);

