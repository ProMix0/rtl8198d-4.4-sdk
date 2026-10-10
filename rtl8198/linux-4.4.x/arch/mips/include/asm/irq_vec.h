/*
 *  Realtek Semiconductor Corp.
 *
 *  Tony Wu (tonywu@realtek.com)
 *  Feb. 28, 2008
 */
#ifndef _ASM_IRQ_VEC_H_
#define _ASM_IRQ_VEC_H_

#ifdef CONFIG_IRQ_DOMAIN
struct device_node;
extern int mips_vec_irq_of_init(struct device_node *of_node,
				struct device_node *parent);
#endif

#endif /* _ASM_IRQ_VEC_H_ */
