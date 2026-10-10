/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/version.h>

#define LDMA_GLB_CONTROL			0
#define LDMA_GLB_CONTROL_LPAE		(1<<31)
#define LDMA_GLB_RATE_LIMITER		4
#define LDMA_GLB_AXPATTERN_0		0x54

#define LDMA_AXI_CONFIG_ACE		0x7800
#define LDMA_QUEUE_DESCRIPTOR_ACCESS	0x00
#define LDMA_QUEUE_DESCRIPTOR_DATA2		0x04
#define LDMA_QUEUE_DESCRIPTOR_DATA1		0x08
#define	LDMA_QUEUE_DESCRIPTOR_DATA0		0x0C
#define	LDMA_QUEUE_CONTROL_REGISTER		0x10
#define	LDMA_QUEUE_DWRR_REGISTER		0x14
#define	LDMA_QUEUE_INTERRUPT_ENABLE		0x18
#define	LDMA_QUEUE_INTERRUPT_STATUS		0x1C
#define	LDMA_QUEUE_POINTER_REGISTER		0x20
#define LDMA_CHAN_REG_SIZE			0x40

#define LDMA_MAX_LEN_IN_DESC			4096

/* Desc bit definition */
#define LDMA_INT_ON_COMPLETE	(1<<15)
#define LDMA_CHAN_RESET			(1<<28)
#define LDMA_CHAN_ENABLE		(1<<31)
#define LDMA_INT_FOV_EN			(1<<1)
#define LDMA_INT_DD_EN			(1<<0)

#define MAX_TASKLET_ITERATION	10

#define SW_DESC_CNT		256
#define DMA_MAX_LEN		0xFFFF

#define CA_DMA_CHANNELS	1

#define LDMA_HW_Q_DEPTH		16

struct ca_adma_sw_desc {
	dma_addr_t dst;
	dma_addr_t src;
	size_t len;
};

struct ca_adma_desc {
	struct dma_async_tx_descriptor desc;
	struct ca_adma_sw_desc *sw_desc;
	int error;
	int desc_id;
	struct list_head node;
};

struct ca_dma_chan {
	struct dma_chan chan;
	void __iomem *map_base_v;
	void __iomem *map_base_p;
	struct list_head free;	/* Free queue, recycled from completed queue */
	struct list_head prepared;	/* stay here in prep_memcpy */
	struct list_head queued;	/* stay here in tx_submit */
	struct list_head active;	/* stay here in issue_pending */
	struct list_head completed;	/* stay here in tasklet */
	struct ca_adma_sw_desc *sw_desc_pool;
	dma_cookie_t completed_cookie;
	unsigned long prepared_cookie;
	unsigned long submit_cookie;
	/* Lock for this structure */
	spinlock_t lock;
	int channel_id;
	int irq_mux;
	bool dma_coherent;
};

struct ca_dma {
	struct dma_device dma;
	struct tasklet_struct tasklet;
	struct ca_dma_chan channels[CA_DMA_CHANNELS];
	int irq;
	uint error_status;
};

#define DRV_NAME	"ca_adma"

static void ca_dma_issue_pending(struct dma_chan *chan);
static struct of_device_id ca_adma_dt_ids[];

/* Convert struct dma_chan to struct ca_dma_chan */
static inline struct ca_dma_chan *dma_chan_to_ca_dma_chan(struct
							  dma_chan *c)
{
	return container_of(c, struct ca_dma_chan, chan);
}

/* Convert struct dma_chan to struct ca_dma */
static inline struct ca_dma *dma_chan_to_ca_dma(struct dma_chan *c)
{
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(c);
	return container_of(dma_chan, struct ca_dma, channels[c->chan_id]);
}

/* */
static void ca_adma_dma_off(struct ca_dma_chan *dma_chan)
{
	/* Reset */
	writel(LDMA_CHAN_RESET,
	       dma_chan->map_base_v + LDMA_QUEUE_CONTROL_REGISTER);
	udelay(10);
	writel(0, dma_chan->map_base_v + LDMA_QUEUE_CONTROL_REGISTER);
}

/*
 * Execute all queued DMA descriptors.
 *
 * Following requirements must be met while calling ca_dma_execute():
 * 	a) dma_chan->lock is acquired,
 * 	b) dma_chan->active list is empty,
 * 	c) dma_chan->queued list contains at least one entry.
 */
void ca_dma_execute(struct ca_dma_chan *dma_chan)
{
	/* Move all queued descriptors to active list */
	list_splice_tail_init(&dma_chan->queued, &dma_chan->active);

}

/* Interrupt handler */
static irqreturn_t ca_dma_irq(int irq, void *data)
{
	struct ca_dma *ca_dma = data;
	unsigned int i;
	struct ca_dma_chan *dma_chan;

	for (i = 0; i < CA_DMA_CHANNELS; i++) {
		dma_chan = &ca_dma->channels[i];

		/* Disable IRQ */
		writel_relaxed(0,
			       dma_chan->map_base_v +
			       LDMA_QUEUE_INTERRUPT_ENABLE);
	}

	/* Schedule tasklet */
	tasklet_schedule(&ca_dma->tasklet);

	return IRQ_HANDLED;
}

/* DMA Tasklet */
static void ca_dma_tasklet(unsigned long data)
{
	struct ca_dma *ca_dma = (void *)data;
	dma_cookie_t last_cookie = 0;
	struct ca_dma_chan *dma_chan;
	struct ca_adma_desc *ca_desc;
	struct dma_async_tx_descriptor *desc;
	unsigned long flags;
	LIST_HEAD(list);
	int i, reg_v, iter = 0;

      again:

	for (i = 0; i < ca_dma->dma.chancnt; i++) {
		dma_chan = &ca_dma->channels[i];

		reg_v =
		    readl(dma_chan->map_base_v + LDMA_QUEUE_INTERRUPT_STATUS);
		if (reg_v & LDMA_INT_FOV_EN)
			printk("ADMA FIFO Overrun\n");

		/* Clear INT */
		writel(reg_v,
		       dma_chan->map_base_v + LDMA_QUEUE_INTERRUPT_STATUS);

		if (reg_v == 0)	/* No interrupt for this channel */
			continue;

		/* Get all completed descriptors */
		spin_lock_irqsave(&dma_chan->lock, flags);
		list_splice_tail_init(&dma_chan->active, &dma_chan->completed);

		if (!list_empty(&dma_chan->completed))
			list_splice_tail_init(&dma_chan->completed, &list);

		if (list_empty(&list)) {
			spin_unlock_irqrestore(&dma_chan->lock, flags);
			continue;
		}

		/* Move pending request to HW */
		if (!list_empty(&dma_chan->queued)) {
			spin_unlock_irqrestore(&dma_chan->lock, flags);
			ca_dma_issue_pending(&dma_chan->chan);
			spin_lock_irqsave(&dma_chan->lock, flags);
		}

		/* Execute callbacks and run dependencies */
		list_for_each_entry(ca_desc, &list, node) {
			desc = &ca_desc->desc;

			if (desc->callback)
				desc->callback(desc->callback_param);

			last_cookie = desc->cookie;

			dma_run_dependencies(desc);
		}

		/* Free descriptors */
		list_splice_tail_init(&list, &dma_chan->free);
		dma_chan->completed_cookie = last_cookie;
		spin_unlock_irqrestore(&dma_chan->lock, flags);
	}

	if (iter++ < MAX_TASKLET_ITERATION) {
		for (i = 0; i < ca_dma->dma.chancnt; i++) {
			reg_v =
			    readl(dma_chan->map_base_v +
				  LDMA_QUEUE_INTERRUPT_STATUS);
			if (reg_v & LDMA_INT_DD_EN)
				goto again;
		}
	}

	/* Enable IRQ */
	writel_relaxed(LDMA_INT_DD_EN | LDMA_INT_FOV_EN, dma_chan->map_base_v +
		       LDMA_QUEUE_INTERRUPT_ENABLE);

	return;
}

/* Submit descriptor to hardware */
static dma_cookie_t ca_dma_tx_submit(struct dma_async_tx_descriptor *txd)
{

	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(txd->chan);
	struct ca_adma_desc *ca_desc;
	unsigned long flags;
	dma_cookie_t cookie;

	ca_desc = container_of(txd, struct ca_adma_desc, desc);

	spin_lock_irqsave(&dma_chan->lock, flags);

	/* Move descriptor to queue */
	list_move_tail(&ca_desc->node, &dma_chan->queued);

	/* Update cookie */
	cookie = dma_chan->chan.cookie + 1;
	if (cookie <= 0)
		cookie = 1;

	dma_chan->chan.cookie = cookie;
	ca_desc->desc.cookie = cookie;

	spin_unlock_irqrestore(&dma_chan->lock, flags);

	return cookie;
}

/* Alloc channel resources */
static int ca_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct ca_dma *ca_dma = dma_chan_to_ca_dma(chan);
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(chan);
	struct ca_adma_desc *ca_desc, *tmp_desc;
	unsigned long flags;
	int i;
	LIST_HEAD(descs);

	dma_chan->sw_desc_pool =
	    kzalloc(sizeof(struct ca_adma_sw_desc) * SW_DESC_CNT, GFP_KERNEL);

	if (!dma_chan->sw_desc_pool) {
		printk("SW Desc pool alloc fail!\n");
		return ENOMEM;
	}

	/* Alloc descriptors for this channel */
	for (i = 0; i < SW_DESC_CNT; i++) {
		ca_desc = kzalloc(sizeof(struct ca_adma_desc), GFP_KERNEL);
		if (!ca_desc) {
			dev_notice(ca_dma->dma.dev,
				   "Memory allocation error. "
				   "Allocated only %u descriptors\n", i);

			break;
		}
		ca_desc->sw_desc = &dma_chan->sw_desc_pool[i];
		dma_async_tx_descriptor_init(&ca_desc->desc, chan);
		ca_desc->desc.flags = DMA_CTRL_ACK;
		ca_desc->desc.tx_submit = ca_dma_tx_submit;

		ca_desc->desc_id = i;
		list_add_tail(&ca_desc->node, &descs);
	}

	/* Return error only if no descriptors were allocated */
	/* FIXME, free allocated descript if no space */
	if (i < SW_DESC_CNT) {
		/* Free sw desc pool */
		kfree(dma_chan->sw_desc_pool);

		/* Free list */
		list_for_each_entry_safe(ca_desc, tmp_desc, &descs, node)
		    kfree(ca_desc);

		return -ENOMEM;
	}

	spin_lock_irqsave(&dma_chan->lock, flags);
	list_splice_tail_init(&descs, &dma_chan->free);
	spin_unlock_irqrestore(&dma_chan->lock, flags);

	/* FIXME: Enable Interrupt */
	writel_relaxed(LDMA_INT_DD_EN | LDMA_INT_FOV_EN, dma_chan->map_base_v +
		       LDMA_QUEUE_INTERRUPT_ENABLE);

	return 0;
}

/* Free channel resources */
static void ca_dma_free_chan_resources(struct dma_chan *chan)
{
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(chan);
	struct ca_adma_desc *ca_desc, *tmp;
	unsigned long flags;
	LIST_HEAD(descs);

	spin_lock_irqsave(&dma_chan->lock, flags);

	/* Channel must be idle */
	BUG_ON(!list_empty(&dma_chan->prepared));
	BUG_ON(!list_empty(&dma_chan->queued));
	BUG_ON(!list_empty(&dma_chan->active));
	BUG_ON(!list_empty(&dma_chan->completed));

	/* Move data */
	list_splice_tail_init(&dma_chan->free, &descs);

	spin_unlock_irqrestore(&dma_chan->lock, flags);

	/* Free descriptors */
	list_for_each_entry_safe(ca_desc, tmp, &descs, node)
	    kfree(ca_desc);

	/* Free sw desc pool */
	kfree(dma_chan->sw_desc_pool);

	/* FIXME: Disable Interrupt */
	ca_adma_dma_off(dma_chan);
}

/* Send all pending descriptor to hardware */
static void ca_dma_issue_pending(struct dma_chan *chan)
{
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(chan);
	struct ca_adma_desc *ca_desc;
	struct ca_adma_sw_desc *sw_desc;
	int free_hw_desc, len, loop=0;
	unsigned int data0, data1, data2, new_desc_in_hw = 0;
	unsigned long flags;

	/* Return if queue empty */
	if (list_empty(&dma_chan->queued))
		return;

	spin_lock_irqsave(&dma_chan->lock, flags);
	/* Get free HW desc from LDMA register */
	free_hw_desc =
	    readl(dma_chan->map_base_v + LDMA_QUEUE_POINTER_REGISTER);
	free_hw_desc = (free_hw_desc & 0x1F000000) >> 24;
	free_hw_desc = LDMA_HW_Q_DEPTH - free_hw_desc;
	if (free_hw_desc == 0)
		return;

	/* Programming sequence on LDMA: 
	 * Data2 -> Data1 -> Data0
	 * HW wrt_prt increase once Data0 be write!
	 */
	ca_desc = list_first_entry(&dma_chan->queued, struct ca_adma_desc,
				   node);

	/* Enable LDMA */
	writel(LDMA_CHAN_ENABLE,
	       dma_chan->map_base_v + LDMA_QUEUE_CONTROL_REGISTER);

	do {
		sw_desc = ca_desc->sw_desc;
		len = sw_desc->len;

		/* HW queue is not able to cover this request, wait next time */
		if ((len / LDMA_MAX_LEN_IN_DESC +
		     (len % LDMA_MAX_LEN_IN_DESC) ? 1 : 0) > free_hw_desc){
		     printk("LDMA Descriptor too low! wait\n");
			break;
		}

		while (len > 0) {
			new_desc_in_hw = 1;
			data0 = sw_desc->src + loop*LDMA_MAX_LEN_IN_DESC;
			data1 = sw_desc->dst + loop*LDMA_MAX_LEN_IN_DESC ;
			writel_relaxed(data1, dma_chan->map_base_v +
				       LDMA_QUEUE_DESCRIPTOR_DATA1);

			data2 =
			    (len >= LDMA_MAX_LEN_IN_DESC) ? 0x0FFF : (len - 1);
			len -= LDMA_MAX_LEN_IN_DESC;

			/* Fill address bit 32~39 */
			data2 |= (sw_desc->dst >> 32) << 16;
			data2 |= (sw_desc->src >> 32) << 24;
			if(dma_chan->dma_coherent == 1){
				data2 |= 2<<16;
				data2 |= 2<<24;
			}
			if (len <= 0) {	/* The last Desc of this request */
				/* Do not update data0 or HW increase wrt_ptr */
			} else {
				writel_relaxed(data2, dma_chan->map_base_v +
					       LDMA_QUEUE_DESCRIPTOR_DATA2);

				writel_relaxed(data0, dma_chan->map_base_v +
					       LDMA_QUEUE_DESCRIPTOR_DATA0);
			}
			free_hw_desc--;
			loop++;
		}

		dma_chan->submit_cookie++;
		list_del(&ca_desc->node);
		list_add_tail(&ca_desc->node, &dma_chan->active);
		if (list_empty(&dma_chan->queued)){
			break;
		}
		else {
			ca_desc =
			    list_first_entry(&dma_chan->queued,
					     struct ca_adma_desc, node);
		}
	} while (free_hw_desc);

	if (new_desc_in_hw) {	/* Increse HW write pointer and break */
		/* Enable interrupt only for the last desc! */
		writel_relaxed(data2 | LDMA_INT_ON_COMPLETE,
			       dma_chan->map_base_v +
			       LDMA_QUEUE_DESCRIPTOR_DATA2);
		writel_relaxed(data0,
			       dma_chan->map_base_v +
			       LDMA_QUEUE_DESCRIPTOR_DATA0);
	}

	spin_unlock_irqrestore(&dma_chan->lock, flags);
}

/* Check request completion status */
static enum dma_status
ca_dma_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
		 struct dma_tx_state *txstate)
{
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(chan);
	unsigned long flags;
	dma_cookie_t last_used;
	dma_cookie_t last_complete;

	spin_lock_irqsave(&dma_chan->lock, flags);
	last_used = dma_chan->chan.cookie;
	last_complete = dma_chan->completed_cookie;
	spin_unlock_irqrestore(&dma_chan->lock, flags);

	dma_set_tx_state(txstate, last_complete, last_used, 0);
	return dma_async_is_complete(cookie, last_complete, last_used);
}

/* Prepare descriptor for memory to memory copy */
static struct dma_async_tx_descriptor *ca_dma_prep_memcpy(struct dma_chan
							  *chan,
							  dma_addr_t dst,
							  dma_addr_t src,
							  size_t len,
							  unsigned long flags)
{
	struct ca_dma_chan *dma_chan = dma_chan_to_ca_dma_chan(chan);
	struct ca_adma_desc *ca_desc = NULL;
	unsigned long iflags;

	/* Reject if length over 64KB */
	if (len > DMA_MAX_LEN) {
		printk("ADMA REQUEST SIZE OVER[ %d > %d]\n", (int)len,
		       DMA_MAX_LEN);
		return NULL;
	}

	/* Get free descriptor */
	spin_lock_irqsave(&dma_chan->lock, iflags);
	if (!list_empty(&dma_chan->free)) {
		ca_desc =
		    list_first_entry(&dma_chan->free, struct ca_adma_desc,
				     node);
		list_del(&ca_desc->node);
	}
	spin_unlock_irqrestore(&dma_chan->lock, iflags);

	if (!ca_desc) {
		printk(" Allocate desc fail for memcpy\n");
		return NULL;
	}

	/* Copy SRC/DST/Len to sw_desc */
	ca_desc->error = 0;
	ca_desc->sw_desc->dst = dst;
	ca_desc->sw_desc->src = src;
	ca_desc->sw_desc->len = len;

	/* Place descriptor in prepared list */
	spin_lock_irqsave(&dma_chan->lock, iflags);
	list_add_tail(&ca_desc->node, &dma_chan->prepared);
	spin_unlock_irqrestore(&dma_chan->lock, iflags);

	return &ca_desc->desc;
}

/* This driver does not implement any of the optional DMA operations. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0) 
static int
ca_dma_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd, unsigned long arg)
{
	return -ENOSYS;
}
#endif

static int ca_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dma_device *dma;
	struct ca_dma *ca_dma;
	struct ca_dma_chan *dma_chan;
	int retval, i;
	void __iomem *io_global, *io, *io_irq_mux, *io_axi;
	int err, ret;
	struct resource mem_resource, mem_resource_irqmux, mem_resource_global, mem_resource_axi;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	unsigned int tmp;

	/* search DT for a match */
	match = of_match_device(ca_adma_dt_ids, &pdev->dev);
	if (!match)
		return -EINVAL;

	dev_notice(&pdev->dev, "Cortina-Access Async. DMA driver\n");

	ca_dma = devm_kzalloc(dev, sizeof(struct ca_dma), GFP_KERNEL);
	if (!ca_dma) {
		dev_err(dev, "Memory exhausted!\n");
		return -ENOMEM;
	}

	/* Get Control register */
	ret = of_address_to_resource(np, 0, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	io = devm_ioremap(&pdev->dev, mem_resource.start,
			  resource_size(&mem_resource));

	ca_dma->irq = irq_of_parse_and_map(np, 0);
	printk("Async. DMA IRQ: %d\n",ca_dma->irq);
	retval =
	    devm_request_irq(dev, ca_dma->irq, &ca_dma_irq, 0, DRV_NAME,
			     ca_dma);
	if (retval) {
		dev_err(dev, "Error requesting IRQ!\n");
		err = -EINVAL;
		goto err_kfree;
	}

	/* Get register for interrupt mux */
	ret = of_address_to_resource(np, 1, &mem_resource_irqmux);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	io_irq_mux = devm_ioremap(&pdev->dev, mem_resource_irqmux.start,
			  resource_size(&mem_resource_irqmux));

	/* Get LDMA_Global register */
	ret = of_address_to_resource(np, 2, &mem_resource_global);
	if (ret) {
		dev_warn(&pdev->dev, "Assume no LPAE %d\n", ret);
		goto NO_LPAE;
	}

	io_global = devm_ioremap(&pdev->dev, mem_resource_global.start,
			  resource_size(&mem_resource_global));

	/* AXI Master control register */
	ret = of_address_to_resource(np, 3, &mem_resource_axi);
	if (ret) {
		dev_warn(&pdev->dev, "No  no LPAE %d\n", ret);
		goto NO_LPAE;
	}

	io_axi = devm_ioremap(&pdev->dev, mem_resource_axi.start,
			  resource_size(&mem_resource_axi));
	tmp = readl(io_axi);
	tmp |= 0xFF;	/* 16 Read/Write outstanding */
	writel(tmp, io_axi);

	/* LPAE */
	tmp = readl(io_global + LDMA_GLB_CONTROL);
	tmp |= LDMA_GLB_CONTROL_LPAE;
	writel(tmp, io_global + LDMA_GLB_CONTROL);


	/* Rate limit */
	tmp = readl(io_global + LDMA_GLB_RATE_LIMITER);
	tmp &= ~0xffff;
	writel(tmp, io_global + LDMA_GLB_RATE_LIMITER);
	devm_iounmap(&pdev->dev, io_global);

NO_LPAE:

	if (!of_property_read_u32(np, "ldma-channel-id", &tmp))
		ca_dma->channels[0].channel_id = tmp;
	if (!of_property_read_u32(np, "ldma-irq-mux", &tmp))
		ca_dma->channels[0].irq_mux = tmp;

	/* One mux include Enable and status register  */
	tmp = readl(io_irq_mux + 4 * 2 * ca_dma->channels[0].irq_mux );
	tmp |= (1<< ca_dma->channels[0].channel_id);
	writel(tmp, io_irq_mux + 4 * 2 * ca_dma->channels[0].irq_mux );

	devm_iounmap(&pdev->dev, io_irq_mux);

	dma = &ca_dma->dma;
	dma->dev = dev;
	dma->chancnt = CA_DMA_CHANNELS;
	dma->device_alloc_chan_resources = ca_dma_alloc_chan_resources;
	dma->device_free_chan_resources = ca_dma_free_chan_resources;
	dma->device_issue_pending = ca_dma_issue_pending;
	dma->device_tx_status = ca_dma_tx_status;
	dma->device_prep_dma_memcpy = ca_dma_prep_memcpy;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0) 
	dma->device_control = ca_dma_control;
#endif
	INIT_LIST_HEAD(&dma->channels);
	dma_cap_set(DMA_MEMCPY, dma->cap_mask);

	for (i = 0; i < dma->chancnt; i++) {
		dma_chan = &ca_dma->channels[i];
		dma_chan->map_base_p =
		    (void __iomem *)mem_resource.start +
		    (i * LDMA_CHAN_REG_SIZE);
		dma_chan->map_base_v = io + (i * LDMA_CHAN_REG_SIZE);

		if (of_property_read_bool(np, "dma-coherent")){
			printk("LDMA HW coherent enabled!\n");
			writel(0x0083c080, io_global + LDMA_GLB_AXPATTERN_0);
			tmp = readl(io_global + LDMA_GLB_CONTROL);
			tmp |= (1<<3);			/* ace_cmd */
			writel(tmp, io_global + LDMA_GLB_CONTROL);
			/* AXI master */
			tmp = readl(io_axi);
			tmp |= LDMA_AXI_CONFIG_ACE;
			writel(tmp, io_axi);
			dma_chan->dma_coherent = 1;
		}

		dma_chan->chan.device = dma;
		dma_chan->chan.chan_id = i;
		dma_chan->chan.cookie = 1;
		dma_chan->completed_cookie = dma_chan->chan.cookie;

		INIT_LIST_HEAD(&dma_chan->free);
		INIT_LIST_HEAD(&dma_chan->prepared);
		INIT_LIST_HEAD(&dma_chan->queued);
		INIT_LIST_HEAD(&dma_chan->active);
		INIT_LIST_HEAD(&dma_chan->completed);

		spin_lock_init(&dma_chan->lock);
		list_add_tail(&dma_chan->chan.device_node, &dma->channels);

		/* Disable DMA */
		ca_adma_dma_off(dma_chan);
	}

	tasklet_init(&ca_dma->tasklet, ca_dma_tasklet, (unsigned long)ca_dma);

	/*
	 * Configure DMA Engine:
	 * - Burst length 64 * 64 Bits
	 */

	/* Register DMA engine */
	dev_set_drvdata(dev, ca_dma);
	retval = dma_async_device_register(dma);
	if (retval)
		devm_free_irq(dev, ca_dma->irq, ca_dma);

	return retval;

      err_kfree:
	kfree(ca_dma);
	return err;
}

static int ca_dma_remove(struct platform_device *op)
{

	struct device *dev = &op->dev;
	struct ca_dma *ca_dma = dev_get_drvdata(dev);

	dma_async_device_unregister(&ca_dma->dma);
	devm_free_irq(dev, ca_dma->irq, ca_dma);

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ca_adma_dt_ids[] = {
	{.compatible = "cortina,adma",},
	{},
};

MODULE_DEVICE_TABLE(of, ca_adma_dt_ids);
#endif

static struct platform_driver ca_adma_driver = {
	.probe = ca_dma_probe,
	.remove = ca_dma_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "ca_adma",
		   .of_match_table = of_match_ptr(ca_adma_dt_ids),
		   },
};

static int __init ca_dma_init(void)
{
	//return platform_driver_probe(&ca_dma_driver, ca_dma_probe);
	return platform_driver_register(&ca_adma_driver);
}

module_init(ca_dma_init);

static void __exit ca_dma_exit(void)
{
	platform_driver_unregister(&ca_adma_driver);
}

module_exit(ca_dma_exit);

/* How to test Adma
cd /sys/module/dmatest/parameters/
echo 2768 > test_buf_size
echo 1 > threads_per_chan
echo dma0chan0 > channel
echo 100 > timeout
echo 1 > iterations
echo 1 > run
*/

MODULE_AUTHOR("Jason Li <jason.li@cortina-access.com>");
MODULE_DESCRIPTION("Async. DMA driver ");
MODULE_LICENSE("GPL");
