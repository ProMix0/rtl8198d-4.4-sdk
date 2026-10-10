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
/*
 * ca_ni_cpu256.c: Cortina Access Network Interface driver
 *
 */
#include <generated/ca_ne_autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <linux/slab.h>

#include <linux/io.h>
#include <linux/reset.h>

#if CONFIG_OF /* DT support */
#include <linux/of.h>
#include <linux/of_device.h>

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <linux/phy/phy.h>

#include <dt-bindings/soc/cortina_irqs.h>

#endif /* DT support */

#include "ca_mut.h"
#include "ca_ni.h"
#include "ca_ni_tx.h"
#include "ca_ni_cpu256.h"
#include "ca_ni_proc.h"
#include "aal_l3qm.h"
#include "aal_l3qm_cpu256.h"
#include "aal_fbm.h"
#include "scfg.h"

spinlock_t  ca_ni_rx_cpu256_hi_refill_lock;
spinlock_t  ca_ni_rx_cpu256_lo_refill_lock;

static ca_uint8_t ca_ni_cpu256_hi_use_fbm = 1;
static ca_uint8_t ca_ni_cpu256_lo_use_fbm = 1;
static ca_uint8_t ca_ni_cpu256_hi_pool0_eq_id = 0;
static ca_uint8_t ca_ni_cpu256_hi_pool1_eq_id = 0;
static ca_uint8_t ca_ni_cpu256_lo_pool0_eq_id = 0;
static ca_uint8_t ca_ni_cpu256_lo_pool1_eq_id = 0;
static ca_uint8_t ca_ni_cpu256_hi_pool0_fbm_pool_id = 3;
static ca_uint8_t ca_ni_cpu256_hi_pool1_fbm_pool_id = 3;
static ca_uint8_t ca_ni_cpu256_lo_pool0_fbm_pool_id = 3;
static ca_uint8_t ca_ni_cpu256_lo_pool1_fbm_pool_id = 3;
static ca_uint8_t ca_ni_support_cpu256_in_linux = 0;
static ca_uint32_t ca_ni_cpu256_hi_pool0_buf_count = 1024;
static ca_uint32_t ca_ni_cpu256_hi_pool1_buf_count = 1024;
static ca_uint32_t ca_ni_cpu256_lo_pool0_buf_count = 1024;
static ca_uint32_t ca_ni_cpu256_lo_pool1_buf_count = 1024;
static ca_uint8_t ca_ni_cpu256_use_qm_eq_refill_int = 0;
static ca_uint32_t ca_ni_cpu256_napi_budget = 160;
static ca_uint8_t ca_ni_cpu256_use_qm_eq_refill_tasklet = 0;

static const u32 default_msg = NETIF_MSG_DRV | NETIF_MSG_PROBE
        | NETIF_MSG_LINK | NETIF_MSG_TIMER | NETIF_MSG_IFDOWN
        | NETIF_MSG_IFUP | NETIF_MSG_RX_ERR | NETIF_MSG_TX_ERR;

extern int ni_initialized;

irqreturn_t ca_ni_qm_cpu256_refill_interrupt(int irq, void *dev_instance)
{
	ca_uint16_t eqid;
	ca_uint32_t bid_cnt;
	ca_uint8_t fbm_pool_id;
	ca_uint32_t buf_count;
	struct net_device *dev = (struct net_device *)dev_instance;

	/* EQ 4/5 is for CPU256 HI, EQ 6/7 is for CPU256 LO */
	if (irq == ni_info_data.cpu256_refill_req_4 || irq == ni_info_data.cpu256_refill_req_5) {
		if (irq == ni_info_data.cpu256_refill_req_4) {
			eqid = ca_ni_cpu256_hi_pool0_eq_id;
			fbm_pool_id = ca_ni_cpu256_hi_pool0_fbm_pool_id;
			buf_count = ca_ni_cpu256_hi_pool0_buf_count;
		}
		else {
			eqid = ca_ni_cpu256_hi_pool1_eq_id;
			fbm_pool_id = ca_ni_cpu256_hi_pool1_fbm_pool_id;
			buf_count = ca_ni_cpu256_hi_pool1_buf_count;
		}

		aal_l3qm_enable_refill_threshold_interrupt(eqid, 0);

		if (ca_ni_cpu256_hi_use_fbm)
			bid_cnt = buf_count - aal_fbm_get_out_stnd_count(FBM_CPU_ARM0, fbm_pool_id);
		else
			bid_cnt = aal_l3qm_get_inactive_bid_cntr(eqid);

		if (bid_cnt > 0)
			ca_ni_refill_cpu256_hi_eq_buf_pool(dev, 0, eqid, bid_cnt);

		aal_l3qm_enable_refill_threshold_interrupt(eqid, 1);
	}
	else if (irq == ni_info_data.cpu256_refill_req_6 || irq == ni_info_data.cpu256_refill_req_7) {
		if (irq == ni_info_data.cpu256_refill_req_6) {
			eqid = ca_ni_cpu256_lo_pool0_eq_id;
			fbm_pool_id = ca_ni_cpu256_lo_pool0_fbm_pool_id;
			buf_count = ca_ni_cpu256_lo_pool0_buf_count;
		}
		else {
			eqid = ca_ni_cpu256_lo_pool1_eq_id;
			fbm_pool_id = ca_ni_cpu256_lo_pool1_fbm_pool_id;
			buf_count = ca_ni_cpu256_lo_pool1_buf_count;
		}

		aal_l3qm_enable_refill_threshold_interrupt(eqid, 0);

		if (ca_ni_cpu256_lo_use_fbm)
			bid_cnt = buf_count - aal_fbm_get_out_stnd_count(FBM_CPU_ARM0, fbm_pool_id);
		else
			bid_cnt = aal_l3qm_get_inactive_bid_cntr(eqid);

		if (bid_cnt > 0)
			ca_ni_refill_cpu256_lo_eq_buf_pool(dev, 0, eqid, bid_cnt);

		aal_l3qm_enable_refill_threshold_interrupt(eqid, 1);
	}
	else {
		printk("%s: Unknown irq=%d!!!\n", __func__, irq);
		return IRQ_NONE;
	}
        return IRQ_HANDLED;
}

static int ca_ni_request_qm_cpu256_refill_irq(struct net_device *dev)
{
        int retval;
	ca_eth_private_t *cep = netdev_priv(dev);

	if (ca_ni_cpu256_use_qm_eq_refill_int) {

		/* cpu_port == 0 for CPU256 LO, cpu_port == 1 for CPU 256 HI */
		if (cep->cpu_port[0] == 1) {
			retval = request_irq(ni_info_data.cpu256_refill_req_4, ca_ni_qm_cpu256_refill_interrupt, 0, "QM EQ4 REFILL", dev);
			if (retval != 0 && retval != -EBUSY) {
				printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_refill_req_4, retval);
				return retval;
			}

			retval = request_irq(ni_info_data.cpu256_refill_req_5, ca_ni_qm_cpu256_refill_interrupt, 0, "QM EQ5 REFILL", dev);
			if (retval != 0 && retval != -EBUSY) {
				printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_refill_req_5, retval);
				return retval;
			}
			/* enable refill interrupt for cpu256 */
			aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu256_hi_pool0_eq_id, 1);
			aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu256_hi_pool1_eq_id, 1);
		}
		else {
			retval = request_irq(ni_info_data.cpu256_refill_req_6, ca_ni_qm_cpu256_refill_interrupt, 0, "QM EQ6 REFILL", dev);
			if (retval != 0 && retval != -EBUSY) {
				printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_refill_req_6, retval);
				return retval;
			}

			retval = request_irq(ni_info_data.cpu256_refill_req_7, ca_ni_qm_cpu256_refill_interrupt, 0, "QM EQ7 REFILL", dev);
			if (retval != 0 && retval != -EBUSY) {
				printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_refill_req_7, retval);
				return retval;
			}
			 /* enable refill interrupt for cpu256 */
			aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu256_lo_pool0_eq_id, 1);
			aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu256_lo_pool1_eq_id, 1);
		}
        }
        return 0;
}

static void ca_ni_free_qm_cpu256_refill_irq(struct net_device *dev)
{
        ca_eth_private_t *cep = netdev_priv(dev);

        if (ca_ni_cpu256_use_qm_eq_refill_int) {

                /* cpu_port == 0 for CPU256 LO, cpu_port == 1 for CPU 256 HI */
                if (cep->cpu_port[0] == 1) {
                        free_irq(ni_info_data.cpu256_refill_req_4, dev);
                        free_irq(ni_info_data.cpu256_refill_req_5, dev);
                }
                else {
			free_irq(ni_info_data.cpu256_refill_req_6, dev);
			free_irq(ni_info_data.cpu256_refill_req_7, dev);
                }
        }
        return;
}

static irqreturn_t ca_ni_qm_cpu256_hi_interrupt(int irq, void *dev_instance)
{
        unsigned long flags;
        ca_uint16_t     cpu_port = 1;
        ca_eth_private_t *cep = netdev_priv((struct net_device *) dev_instance);

        spin_lock_irqsave(&cep->lock, flags);

        aal_l3qm_enable_cpu256_epp_interrupt(cpu_port, 0);

        /* schedule NAPI */
        napi_schedule(&cep->napi[0]);

        spin_unlock_irqrestore(&cep->lock, flags);
        return IRQ_HANDLED;
}

static irqreturn_t ca_ni_qm_cpu256_lo_interrupt(int irq, void *dev_instance)
{
        unsigned long flags;
        ca_uint16_t     cpu_port = 0;
        ca_eth_private_t *cep = netdev_priv((struct net_device *) dev_instance);

        spin_lock_irqsave(&cep->lock, flags);

        aal_l3qm_enable_cpu256_epp_interrupt(cpu_port, 0);

        napi_schedule(&cep->napi[0]);

        spin_unlock_irqrestore(&cep->lock, flags);
        return IRQ_HANDLED;
}

static int ca_ni_request_qm_cpu256_irq(struct net_device *dev)
{
        int retval;
        ca_eth_private_t *cep = netdev_priv(dev);

        if (cep->cpu_port[0] == 1) {
                retval = request_irq(ni_info_data.cpu256_fifo_req_hi, ca_ni_qm_cpu256_hi_interrupt, 0, dev->name, dev);
                if (retval != 0 && retval != -EBUSY) {
                        printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_fifo_req_hi, retval);
                        return retval;
                }
                printk("%s: request IRQ %d for CPU256 FIFO HI OK\n", __func__, ni_info_data.cpu256_fifo_req_hi);
        }
        else {

                retval = request_irq(ni_info_data.cpu256_fifo_req_lo, ca_ni_qm_cpu256_lo_interrupt, 0, dev->name, dev);
                if (retval != 0 && retval != -EBUSY) {
                        printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.cpu256_fifo_req_lo, retval);
                        return retval;
                }
                printk("%s: request IRQ %d for CPU256 FIFO LO OK\n", __func__, ni_info_data.cpu256_fifo_req_lo);
        }
        return retval;
}

/*
 * Handles DMA LSO VP10/11 interrupts.
 */
irqreturn_t ca_ni_tx_vp10_interrupt(int irq, void *dev_instance)
{
        aal_ni_get_dma_lso_interrupt_status(10);
        return IRQ_HANDLED;
}

irqreturn_t ca_ni_tx_vp11_interrupt(int irq, void *dev_instance)
{
        aal_ni_get_dma_lso_interrupt_status(11);
        return IRQ_HANDLED;
}

static int ca_ni_cpu256_open(struct net_device *dev)
{
	int retval;

        ca_eth_private_t *cep = netdev_priv(dev);

        if (!ni_initialized) {
                printk("%s: dev->name=%s, probe not finished yet!!\n", __func__, dev->name);
                return 0;
        }
        netif_carrier_off(dev);
        cep->status = 1;
        cep->ni_driver_state = 1;
        cep->tx_completion_queues = 0;

        ca_ni_request_qm_cpu256_irq(dev);

        /* call linux kernel routine to wake up all TX queues */
        netif_tx_wake_all_queues(dev);

         /* napi should be enabled before RX interrupt enabled */
        napi_enable(&cep->napi[0]);

        aal_l3qm_enable_cpu256_epp_interrupt(cep->cpu_port[0], 1);

        /* after wake all queues, should turn on carrier for ready to transmit */
        netif_carrier_on(dev);

#ifdef CONFIG_NET_SCH_MULTIQ
        /* multiple queues support */
        retval = netif_set_real_num_tx_queues(dev, 8);
        if (retval) {
                printk("%s: netif_set_real_num_tx_queues(), retval=%d\n", __func__, retval);
                return retval;
        }

        retval = netif_set_real_num_rx_queues(dev, 8);
        if (retval) {
                printk("%s: netif_set_real_num_rx_queues(), retval=%d\n", __func__, retval);
                return retval;
        }
#endif

	/* request irq for DMA LSO VP 10 */
	if (!strcmp(dev->name, "cpu256_eth0")) {
		retval = request_irq(dma_lso_vp10_irq, ca_ni_tx_vp10_interrupt, 0, dev->name, dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, dma_lso_vp10_irq, retval);
			return retval;
		}
	}

	/* request irq for DMA LSO VP 11 */
	if (!strcmp(dev->name, "cpu256_eth1")) {
		retval = request_irq(dma_lso_vp11_irq, ca_ni_tx_vp11_interrupt, 0, dev->name, dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, dma_lso_vp11_irq, retval);
			return retval;
		}
	}

	ca_ni_request_qm_cpu256_refill_irq(dev);

	/* enable QM TX function for cpu port */
	aal_l3qm_enable_tx_cpu256(cep->cpu_port[0], 1);

        return 0;
}

static int ca_ni_cpu256_stop(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	/* disable QM TX function for cpu port */
	aal_l3qm_enable_tx_cpu256(cep->cpu_port[0], 0);

	/* disable CPU256 EPP interrupt */	
	aal_l3qm_enable_cpu256_epp_interrupt(cep->cpu_port[0], 0);

	/* stop TX queues */
	netif_tx_disable(dev);

	/* disable NAPI */
	napi_disable(&cep->napi[0]);

	/* release irq */
	if (cep->cpu_port[0] == 1)
		free_irq(ni_info_data.cpu256_fifo_req_hi, dev);
	else
		free_irq(ni_info_data.cpu256_fifo_req_lo, dev);
	ca_ni_free_qm_cpu256_refill_irq(dev);

	cep->status = 0;

#ifdef NI_RECYCLE_SKB
	/* clean preallocate buffer */
	skb_queue_purge(&ca_ni_skb_recycle_cpu0_head);
#endif

	/* request irq for DMA LSO VP 10 */
	if (!strcmp(dev->name, "cpu256_eth0")) {
		free_irq(dma_lso_vp10_irq, dev);
	}

	/* request irq for DMA LSO VP 11 */
	if (!strcmp(dev->name, "cpu256_eth1")) {
		free_irq(dma_lso_vp11_irq, dev);
	}

	return 0;

}

static netdev_tx_t ca_ni_cpu256_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
        int lso_txq_index;
        ca_eth_private_t *cep;
        ca_ni_tx_config_t tx_config;
        netdev_tx_t ret;

#ifdef CONFIG_RTK_PERF_EVENTS
        start_perf_counter(0x1); // save to idx 8
#endif

        cep = netdev_priv(dev);

	ca_skb_in(skb, true);

#ifdef CONFIG_NET_SCH_MULTIQ
        lso_txq_index = skb_get_queue_mapping(skb);
#else
        lso_txq_index = skb->priority;
#endif

	memset(&tx_config, 0, sizeof(ca_ni_tx_config_t));
        tx_config.core_config.bf.txq_index = lso_txq_index;
        tx_config.core_config.bf.ldpid = CA_PORT_ID_L3_LAN;

        ret = __ca_ni_start_xmit(skb, dev, &tx_config);

#ifdef CONFIG_RTK_PERF_EVENTS
        stop_perf_counter(8, 0x1);
#endif

        return ret;
}

static struct net_device_stats *ca_ni_get_cpu256_stats(struct net_device *dev)
{
        return &dev->stats;
}

static int ca_ni_set_cpu256_features(struct net_device *dev, netdev_features_t features)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	unsigned long flags;

	if ((dev->features ^ features) & NETIF_F_RXCSUM) {

		spin_lock_irqsave(&cep->lock, flags);

		if (features & NETIF_F_RXCSUM) {
			cep->rx_checksum = CA_ENABLE;
		} else {
			cep->rx_checksum = CA_DISABLE;
		}
		//aal_l3fe_l4_chksum(cep->rx_checksum);

		spin_unlock_irqrestore(&cep->lock, flags);
	}
	return 0;
}


static const struct net_device_ops ca_ni_cpu256_netdev_ops = {
        .ndo_open               = ca_ni_cpu256_open,
        .ndo_stop               = ca_ni_cpu256_stop,
        .ndo_start_xmit         = ca_ni_cpu256_start_xmit,
//        .ndo_tx_timeout         = ca_ni_tx_timeout,
//        .ndo_validate_addr      = eth_validate_addr,
//        .ndo_change_mtu         = ca_ni_change_mtu,
//        .ndo_set_mac_address    = ca_ni_set_mac_address,
//        .ndo_do_ioctl           = ca_ni_ioctl,
//        .ndo_set_rx_mode        = ca_ni_set_rx_mode,
//        .ndo_change_rx_flags    = ca_ni_change_rx_flags,
	.ndo_set_features       = ca_ni_set_cpu256_features,
	.ndo_get_stats          = ca_ni_get_cpu256_stats,
};


int ca_ni_init_cpu256_intfs(struct platform_device *pdev)
{
	int i, debug=-1;
	struct net_device *dev;
	ca_eth_private_t *cep;
	char mac_addr[6] = {0x00, 0x50, 0xc2, 0x11, 0x22, 0x33};

	for (i = 0; i < 2; i++) {
#ifdef CONFIG_NET_SCH_MULTIQ
		dev = alloc_netdev_mqs(sizeof(ca_eth_private_t), "cpu256_eth%d", NET_NAME_UNKNOWN, ether_setup, QUEUE_PER_INSTANCE, QUEUE_PER_INSTANCE);
#else
		dev = alloc_netdev_mqs(sizeof(ca_eth_private_t), "cpu256_eth%d", NET_NAME_UNKNOWN, ether_setup, 1, 1);
#endif
		if (dev == NULL) {
			pr_err("Unable to alloc new cpu256 ethernet interface #%d.\n", i);
			return -ENOMEM;
		}

		/* Set the sysfs physical device reference for the network logical device */
		SET_NETDEV_DEV(dev, &pdev->dev);

		ni_info_data.cpu256_dev[i] = dev;

		/* fill all of the items of cep */
		cep = netdev_priv(dev);
		cep->dev = dev;
		cep->pdev = pdev;
		cep->rx_checksum = CA_DISABLE;

		/* add a cpu_port for recognize, cpu_port 0 for CPU256 LO, 1 for CPU256 HI */
		cep->cpu_port[0] = i;

		/* indicate which DMA LSO VP will be used */
		cep->egress_cpu_port_id = i + 10;

		/* msg_enable can be get/set by ethtool for debug message level */
		cep->msg_enable = netif_msg_init(debug, default_msg);

		mac_addr[5] += i;
		memcpy(&dev->dev_addr[0], mac_addr, 6);
		memcpy(dev->perm_addr, dev->dev_addr, 6);

		cep->napi[0].dev = dev;
		cep->use_default_lso_para0 = 0;
		cep->deep_q = 0;
		/* fill all of the items of dev */
		dev->netdev_ops = &ca_ni_cpu256_netdev_ops;
		ca_ni_set_ethtool_ops(dev);
		dev->features = NETIF_F_SG | NETIF_F_HW_CSUM | NETIF_F_RXCSUM | NETIF_F_UFO;
		dev->hw_features = dev->features | NETIF_F_TSO | NETIF_F_TSO6;

		//dev->irq = cep->port_cfg.irq;
		dev->watchdog_timeo = CA_NI_TX_TIMEOUT;
		dev->features |= NETIF_F_GRO;

	}
	return 0;
}

void ca_ni_refill_cpu256_hi_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt)
{
	u16 inactive_bid_cntr;
	unsigned long flags;
	int rule = aal_l3qm_eq_get_cpu256_hi_rule();

	/* the EQ pools are shared by all of CPU CPU ports should be locked before changed */
	spin_lock_irqsave(&ca_ni_rx_cpu256_hi_refill_lock, flags);

	if (aal_l3qm_eq_get_cpu256_hi_use_fbm()) {
		ca_ni_fbm_fill_cpu256_hi_empty_buffer(dev, cpu_port, eqid, cnt);
	}
	else {
		/* refill the first EQ buffer pool */
		inactive_bid_cntr = aal_l3qm_get_inactive_bid_cntr(eqid);
		if (inactive_bid_cntr > cnt)
			inactive_bid_cntr = cnt;
		if (inactive_bid_cntr)
			ca_ni_fill_empty_buffer(dev, cpu_port, eqid, inactive_bid_cntr, rule);
	}

	spin_unlock_irqrestore(&ca_ni_rx_cpu256_hi_refill_lock, flags);

	if (ca_ni_debug & NI_DBG_NI)
		printk("%s: ======== cpu_port=%d, eqid=%d, inactive_bid_cntr=%d, cnt=%d ======== \n", __func__, cpu_port, eqid, inactive_bid_cntr, cnt);
}

void ca_ni_refill_cpu256_lo_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt)
{
	u16 inactive_bid_cntr;
	unsigned long flags;
	int rule = aal_l3qm_eq_get_cpu256_lo_rule();

	/* the EQ pools are shared by all of CPU CPU ports should be locked before changed */
	spin_lock_irqsave(&ca_ni_rx_cpu256_lo_refill_lock, flags);

	if (aal_l3qm_eq_get_cpu256_lo_use_fbm()) {
		ca_ni_fbm_fill_cpu256_lo_empty_buffer(dev, cpu_port, eqid, cnt);
	}
	else {
		/* refill the first EQ buffer pool */
		inactive_bid_cntr = aal_l3qm_get_inactive_bid_cntr(eqid);
		if (inactive_bid_cntr > cnt)
			inactive_bid_cntr = cnt;
		if (inactive_bid_cntr)
			ca_ni_fill_empty_buffer(dev, cpu_port, eqid, inactive_bid_cntr, rule);
	}

	spin_unlock_irqrestore(&ca_ni_rx_cpu256_lo_refill_lock, flags);

	if (ca_ni_debug & NI_DBG_NI)
		printk("%s: ======== cpu_port=%d, eqid=%d, inactive_bid_cntr=%d, cnt=%d ======== \n", __func__, cpu_port, eqid, inactive_bid_cntr, cnt);
}

void ca_ni_fbm_fill_cpu256_hi_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count)
{
        int i;
        u32 phy_addr = 0;
	ca_uint32_t phy_start_addr;
	ca_uint32_t entry_size, entry_count;
	ca_uint8_t fbm_pool_id;
	int rule = aal_l3qm_eq_get_cpu256_hi_rule();

	if (eqid == ca_ni_cpu256_hi_pool0_eq_id) {
		aal_l3qm_eq_get_cpu256_hi_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	}
	else {
		aal_l3qm_eq_get_cpu256_hi_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	}

	if (ca_ni_debug & NI_DBG_NI) {
		printk("%s: eqid=%d, fbm_pool_id=%d, count=%d\n", __func__, eqid, fbm_pool_id, count);
	}

	for (i = 0; i < count; i++) {
		phy_addr = ca_ni_fill_empty_buffer_by_rule(dev, eqid, rule);
		if (phy_addr) {
			aal_fbm_buf_push(FBM_CPU_ARM0, fbm_pool_id, phy_addr);
		}
	}
}

void ca_ni_fbm_fill_cpu256_lo_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count)
{
	int i;
	u32 phy_addr = 0;
	ca_uint32_t phy_start_addr;
	ca_uint32_t entry_size, entry_count;
	ca_uint8_t fbm_pool_id;
	int rule = aal_l3qm_eq_get_cpu256_lo_rule();

	if (eqid == ca_ni_cpu256_lo_pool0_eq_id) {
		aal_l3qm_eq_get_cpu256_lo_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	}
	else {
		aal_l3qm_eq_get_cpu256_lo_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	}

	if (ca_ni_debug & NI_DBG_NI) {
		printk("%s: eqid=%d, fbm_pool_id=%d, count=%d\n", __func__, eqid, fbm_pool_id, count);
	}

	for (i = 0; i < count; i++) {
		phy_addr = ca_ni_fill_empty_buffer_by_rule(dev, eqid, rule);
		if (phy_addr) {
			aal_fbm_buf_push(FBM_CPU_ARM0, fbm_pool_id, phy_addr);
		}
	}
}

void ca_ni_fill_cpu256_eq_buf_pool(struct net_device *dev)
{
	int cpu_port = 0;
	ca_uint32_t phy_start_addr, entry_size, entry_count;
	ca_uint8_t fbm_pool_id;

	/* fill EQ buffer for CPU256 */
	aal_l3qm_eq_get_cpu256_hi_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	ca_ni_refill_cpu256_hi_eq_buf_pool(dev, cpu_port, ca_ni_cpu256_hi_pool0_eq_id, entry_count);

	aal_l3qm_eq_get_cpu256_hi_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	ca_ni_refill_cpu256_hi_eq_buf_pool(dev, cpu_port, ca_ni_cpu256_hi_pool1_eq_id, entry_count);

	aal_l3qm_eq_get_cpu256_lo_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	ca_ni_refill_cpu256_lo_eq_buf_pool(dev, cpu_port, ca_ni_cpu256_lo_pool0_eq_id, entry_count);

	aal_l3qm_eq_get_cpu256_lo_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	ca_ni_refill_cpu256_lo_eq_buf_pool(dev, cpu_port, ca_ni_cpu256_lo_pool1_eq_id, entry_count);
}

/* NAPI polling routine for CPU256 lo port */
extern int ca_ni_cpu256_rx_poll(int cpu_port, struct napi_struct *napi, int budget);
static int ca_ni_cpu256_lo_poll(struct napi_struct *napi, int budget)
{
        return ca_ni_cpu256_rx_poll(0, napi, budget);
}

/* NAPI polling routine for CPU256 lo port */
static int ca_ni_cpu256_hi_poll(struct napi_struct *napi, int budget)
{
        return ca_ni_cpu256_rx_poll(1, napi, budget);
}

int ca_ni_register_cpu256_netdev(void)
{
	int i, err;

        if (!ca_ni_support_cpu256_in_linux) {
                printk("%s: CPU256 is not supportted in Linux!!\n", __func__);
                return -1;
        }
	for (i = 0; i < 2; i++) {
		err = register_netdev(ni_info_data.cpu256_dev[i]);
		if (err) {
			printk(KERN_ERR "%s: Cannot register net device, aborting.\n", ni_info_data.cpu256_dev[i]->name);
			free_netdev(ni_info_data.cpu256_dev[i]);
			return err;
		}
	}
	return 0;
}

void ca_load_cpu256_init(void)
{
	ca_uint8_t      cpu256_hi_pool0_eq_id, cpu256_hi_pool1_eq_id, cpu256_lo_pool0_eq_id, cpu256_lo_pool1_eq_id;
	ca_uint8_t	cpu256_hi_use_fbm, cpu256_lo_use_fbm;
	ca_uint8_t	cpu256_hi_pool0_fbm_pool_id, cpu256_hi_pool1_fbm_pool_id, cpu256_lo_pool0_fbm_pool_id, cpu256_lo_pool1_fbm_pool_id;
	ca_uint32_t	cpu256_hi_pool0_buf_count, cpu256_hi_pool1_buf_count, cpu256_lo_pool0_buf_count, cpu256_lo_pool1_buf_count;
	ca_uint8_t	support_cpu256_in_linux;
	ca_uint8_t	cpu256_use_qm_eq_refill_int;
	ca_uint32_t	cpu256_napi_budget;
	ca_uint8_t	cpu256_use_qm_eq_refill_tasklet;

	if (aal_scfg_read(CFG_ID_NE_CPU256_NAPI_BUDGET, 4, &cpu256_napi_budget) == CA_E_OK) {
		ca_ni_cpu256_napi_budget = cpu256_napi_budget;
		printk("%s: ca_ni_cpu256_napi_budget=%d\n", __func__, ca_ni_cpu256_napi_budget);
	}

	if (aal_scfg_read(CFG_ID_NE_USE_QM_EQ_EPP256_REFILL_INT, 1, &cpu256_use_qm_eq_refill_int) == CA_E_OK) {
		ca_ni_cpu256_use_qm_eq_refill_int = cpu256_use_qm_eq_refill_int;
		printk("%s: ca_ni_cpu256_use_qm_eq_refill_int=%d\n", __func__, ca_ni_cpu256_use_qm_eq_refill_int);
	}

        if (aal_scfg_read(CFG_ID_NE_USE_QM_EQ_EPP256_REFILL_TASKLET, 1, &cpu256_use_qm_eq_refill_int) == CA_E_OK) {
                ca_ni_cpu256_use_qm_eq_refill_tasklet = cpu256_use_qm_eq_refill_tasklet;
                printk("%s: ca_ni_cpu256_use_qm_eq_refill_tasklet=%d\n", __func__, ca_ni_cpu256_use_qm_eq_refill_tasklet);
        }

	if (aal_scfg_read(CFG_ID_SUPPORT_CPU256_IN_LINUX, 1, &support_cpu256_in_linux) == CA_E_OK) {
		ca_ni_support_cpu256_in_linux = support_cpu256_in_linux;
		printk("%s: ca_ni_support_cpu256_in_linux=%d\n", __func__, ca_ni_support_cpu256_in_linux);
	}
	if (!ca_ni_support_cpu256_in_linux) {
		printk("%s: CPU256 is not supportted in Linux!!\n", __func__);
		return;
	}

	/* read eqid for CPU256 ports */
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID, 1, &cpu256_hi_pool0_eq_id) == CA_E_OK) {
		ca_ni_cpu256_hi_pool0_eq_id = cpu256_hi_pool0_eq_id;
		printk("%s: ca_ni_cpu256_hi_pool0_eq_id=%d\n", __func__, ca_ni_cpu256_hi_pool0_eq_id);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID, 1, &cpu256_hi_pool1_eq_id) == CA_E_OK) {
		ca_ni_cpu256_hi_pool1_eq_id = cpu256_hi_pool1_eq_id;
		printk("%s: ca_ni_cpu256_hi_pool1_eq_id=%d\n", __func__, ca_ni_cpu256_hi_pool1_eq_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID, 1, &cpu256_lo_pool0_eq_id) == CA_E_OK) {
		ca_ni_cpu256_lo_pool0_eq_id = cpu256_lo_pool0_eq_id;
		printk("%s: ca_ni_cpu256_lo_pool0_eq_id=%d\n", __func__, ca_ni_cpu256_lo_pool0_eq_id);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID, 1, &cpu256_lo_pool1_eq_id) == CA_E_OK) {
		ca_ni_cpu256_lo_pool1_eq_id = cpu256_lo_pool1_eq_id;
		printk("%s: ca_ni_cpu256_lo_pool1_eq_id=%d\n", __func__, ca_ni_cpu256_lo_pool1_eq_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_USE_FBM, 1, &cpu256_hi_use_fbm) == CA_E_OK) {
		ca_ni_cpu256_hi_use_fbm = cpu256_hi_use_fbm;
		printk("%s: ca_ni_cpu256_hi_use_fbm=%d\n", __func__, ca_ni_cpu256_hi_use_fbm);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_USE_FBM, 1, &cpu256_lo_use_fbm) == CA_E_OK) {
		ca_ni_cpu256_lo_use_fbm = cpu256_lo_use_fbm;
		printk("%s: ca_ni_cpu256_lo_use_fbm=%d\n", __func__, ca_ni_cpu256_lo_use_fbm);
	}
#if defined(CONFIG_CORTINA_BOARD_FPGA)
        /* if no FBM module included the always set ca_ni_cpu256_hi_use_fbm=0 and ca_ni_cpu256_lo_use_fbm=0 */
        if (SOC_HAS_FBM() == 0) {
                ca_ni_cpu256_hi_use_fbm = 0;
                ca_ni_cpu256_lo_use_fbm = 0;
                printk("%s: set ca_ni_cpu256_hi_use_fbm=0 and ca_ni_cpu256_lo_use_fbm=0 because no FBM module\n", __func__);
        }
#endif

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_COUNT, 4, &cpu256_hi_pool0_buf_count) == CA_E_OK) {
		ca_ni_cpu256_hi_pool0_buf_count = cpu256_hi_pool0_buf_count;
		printk("%s: ca_ni_cpu256_hi_pool0_buf_count=%d\n", __func__, ca_ni_cpu256_hi_pool0_buf_count);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_COUNT, 4, &cpu256_hi_pool1_buf_count) == CA_E_OK) {
		ca_ni_cpu256_hi_pool1_buf_count = cpu256_hi_pool1_buf_count;
		printk("%s: ca_ni_cpu256_hi_pool1_buf_count=%d\n", __func__, ca_ni_cpu256_hi_pool1_buf_count);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_COUNT, 4, &cpu256_lo_pool0_buf_count) == CA_E_OK) {
		ca_ni_cpu256_lo_pool0_buf_count = cpu256_lo_pool0_buf_count;
		printk("%s: ca_ni_cpu256_lo_pool0_buf_count=%d\n", __func__, ca_ni_cpu256_lo_pool0_buf_count);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_COUNT, 4, &cpu256_lo_pool1_buf_count) == CA_E_OK) {
		ca_ni_cpu256_lo_pool1_buf_count = cpu256_lo_pool1_buf_count;
		printk("%s: ca_ni_cpu256_lo_pool1_buf_count=%d\n", __func__, ca_ni_cpu256_lo_pool1_buf_count);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_FBM_POOL_ID, 1, &cpu256_hi_pool0_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu256_hi_pool0_fbm_pool_id = cpu256_hi_pool0_fbm_pool_id;
		printk("%s: ca_ni_cpu256_hi_pool0_fbm_pool_id=%d\n", __func__, ca_ni_cpu256_hi_pool0_fbm_pool_id);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_FBM_POOL_ID, 1, &cpu256_hi_pool1_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu256_hi_pool1_fbm_pool_id = cpu256_hi_pool1_fbm_pool_id;
		printk("%s: ca_ni_cpu256_hi_pool1_fbm_pool_id=%d\n", __func__, ca_ni_cpu256_hi_pool1_fbm_pool_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_FBM_POOL_ID, 1, &cpu256_lo_pool0_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu256_lo_pool0_fbm_pool_id = cpu256_lo_pool0_fbm_pool_id;
		printk("%s: ca_ni_cpu256_lo_pool0_fbm_pool_id=%d\n", __func__, ca_ni_cpu256_lo_pool0_fbm_pool_id);
	}
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_FBM_POOL_ID, 1, &cpu256_lo_pool1_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu256_lo_pool1_fbm_pool_id = cpu256_lo_pool1_fbm_pool_id;
		printk("%s: ca_ni_cpu256_lo_pool1_fbm_pool_id=%d\n", __func__, ca_ni_cpu256_lo_pool1_fbm_pool_id);
	}
}

void ca_ni_init_cpu256_module_probe(struct platform_device *pdev)
{
	ca_eth_private_t *cep = platform_get_drvdata(pdev);
#ifdef CONFIG_OF
	struct device_node *np = pdev->dev.of_node;
#endif

	if (!ca_ni_support_cpu256_in_linux) {
		printk("%s: CPU256 is not supportted in Linux!!\n", __func__);
		return;
	}
	ca_ni_init_cpu256_intfs(pdev);
	ca_ni_fill_cpu256_eq_buf_pool(ni_info_data.dev[0]);
	ni_info_data.cpu256_fifo_req_hi = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU256_FIFO_REQ_HI);
	ni_info_data.cpu256_fifo_req_lo = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU256_FIFO_REQ_LO);
	printk("%s: ni_info_data.cpu256_fifo_req_hi=%d, ni_info_data.cpu256_fifo_req_lo=%d\n", __func__,
			ni_info_data.cpu256_fifo_req_hi, ni_info_data.cpu256_fifo_req_lo);

	ni_info_data.cpu256_refill_req_4 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_4);        /* QM EQ4 refill threshold interrupt */
	ni_info_data.cpu256_refill_req_5 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_5);        /* QM EQ5 refill threshold interrupt */
	ni_info_data.cpu256_refill_req_6 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_6);        /* QM EQ6 refill threshold interrupt */
        ni_info_data.cpu256_refill_req_7 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_7);        /* QM EQ7 refill threshold interrupt */
	printk("%s: ni_info_data.cpu256_refill_req_4=%d, ni_info_data.cpu256_refill_req_5=%d,\n", __func__,
			ni_info_data.cpu256_refill_req_4, ni_info_data.cpu256_refill_req_5);
	printk("%s: ni_info_data.cpu256_refill_req_6=%d, ni_info_data.cpu256_refill_req_7=%d,\n", __func__,
			ni_info_data.cpu256_refill_req_6, ni_info_data.cpu256_refill_req_7);

	cep = netdev_priv(ni_info_data.cpu256_dev[0]);
	netif_napi_add(ni_info_data.cpu256_dev[0], &cep->napi[0], ca_ni_cpu256_lo_poll, ca_ni_cpu256_napi_budget);
	printk("%s: ni_info_data.cpu256_dev[0]=%p, cep->napi[0]=%p, budget=%d\n",
			__func__, ni_info_data.cpu256_dev[0], &(cep->napi[0]), ca_ni_cpu256_napi_budget);
	cep->egress_cpu_port_id = 10;

	cep = netdev_priv(ni_info_data.cpu256_dev[1]);
	netif_napi_add(ni_info_data.cpu256_dev[1], &cep->napi[0], ca_ni_cpu256_hi_poll, ca_ni_cpu256_napi_budget);
	printk("%s: ni_info_data.cpu256_dev[1]=%p, cep->napi[0]=%p, budget=%d\n",
			__func__, ni_info_data.cpu256_dev[1], &(cep->napi[0]), ca_ni_cpu256_napi_budget);
	cep->egress_cpu_port_id = 11;

	/* Stop egress packet scheduling/transmission to this CPU256 virtual port */
	aal_l3qm_enable_tx_cpu256(0, 0);
	aal_l3qm_enable_tx_cpu256(1, 0);

	aal_l3qm_enable_cpu256_epp_interrupt(0, 0);
	aal_l3qm_enable_cpu256_epp_interrupt(1, 0);
}
