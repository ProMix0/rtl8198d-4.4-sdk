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
 * ca_virt_ni.c: Cortina Virtual Network Interface
 *
 */

#include <linux/types.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>

#include "aal_ni.h"

#include "ca_ni.h"
#include "ca_virt_ni.h"

static ca_virt_ni_t virt_ni0[NR_VIRT_NI_ETH0];
static ca_virt_ni_t virt_ni1[NR_VIRT_NI_ETH1];
static ca_virt_ni_t virt_ni2[NR_VIRT_NI_ETH2];

/*
 * This function will return virtual device info structure.
 * vpid is the virtual device port ID, and virt_dev is the virtual device
 * struct cep->port_id of virtual device means virtual port ID now, not
 * physical device port ID.
 */
ca_virt_ni_t *ca_ni_get_virt_ni(u8 vpid, struct net_device *virt_dev)
{
	ca_eth_private_t *virt_cep = netdev_priv(virt_dev);
	ca_eth_private_t *real_cep = netdev_priv(virt_cep->dev);

	if (virt_cep->dev == virt_dev) {
		// physical device
		return NULL;
	}

	///TODO: should we use spin_lock here to protect global data???
	switch (real_cep->port_cfg.port_id) {
	case GE0_PORT:
		return (&virt_ni0[vpid]);
	case GE1_PORT:
		return (&virt_ni1[vpid]);
	case GE2_PORT:
		return (&virt_ni2[vpid]);
	default:
		return NULL;
	}
} /* ca_ni_get_virt_ni */

static ca_virt_ni_t *ca_ni_virt_ni_get_ni_by_vid(u8 port_id, u16 vlan_proto, u16 vlan_tci)
{
	int i;
	ca_virt_ni_t *virt_ni;
	ca_eth_private_t *virt_cep;

	switch (port_id) {
	case GE0_PORT:
		virt_ni = &(virt_ni0[0]);
		break;
	case GE1_PORT:
		virt_ni = &(virt_ni1[0]);
		break;
	case GE2_PORT:
		virt_ni = &(virt_ni2[0]);
		break;
	default:
		return NULL;
	}

	// TODO: Why only loop NR_VIRT_NI_ETH0  ???
	for (i = 0; i < NR_VIRT_NI_ETH0; i++, virt_ni++) {
		virt_cep = netdev_priv(virt_ni->dev);
		if (virt_cep->vni_vlan_proto == vlan_proto && virt_cep->vni_vlan_tci == vlan_tci)
			return virt_ni;
	}
	return NULL;
} /* ca_ni_virt_ni_get_ni_by_vid */

static int process_rx_skb_with_vlantag(u16 port_id, struct sk_buff *skb)
{
	ca_eth_private_t *virt_cep;
	ca_virt_ni_t *virt_ni_ptr;
	struct vlan_ethhdr *veth = veth = (struct vlan_ethhdr*)skb->data;

	/* in current Realtek switch setting, we are applying SVLAN for
	 * this virtual interface.  Therefore, if we enter here, the eth_type
	 * should always be ETH_P_8021AD (0x88A8) */
	if ((veth != NULL) && (veth->h_vlan_proto == htons(ETH_P_8021AD))) {
		unsigned char *src, *dst;
		unsigned short i;

		///TODO: VLAN protocol should be only 0x8100???
		virt_ni_ptr = ca_ni_virt_ni_get_ni_by_vid(port_id, ETH_P_8021Q, ntohs(veth->h_vlan_TCI) & VLAN_VID_MASK);
		if (virt_ni_ptr == NULL) {
			/* How could this happen? We will let kernel handle
			 * this packet whether it makes sense to Kernel
			 * network stack or not */
			return 0;
		} else {
			if (virt_ni_ptr->flag == NI_INACTIVE)
				return -1;
			skb->dev = virt_ni_ptr->dev;
		}

		/* remove the VLAN tag from the packet!! */
		if (unlikely(!pskb_may_pull(skb, VLAN_HLEN)))
			return -1;

		/* We have to do 3 4-byte copies, because the source and
		 * the target overlapped. */
		for (i = 1; i <= 3; i++) {
			src = skb->data + (ETH_ALEN * 2) - (VLAN_HLEN * i);
			dst = skb->data + VLAN_ETH_HLEN - (VLAN_HLEN * i) - 2;
			memcpy(dst, src, VLAN_HLEN);
		}
		skb_pull(skb, VLAN_HLEN);
		virt_cep = netdev_priv(skb->dev);
		virt_cep->dev->stats.rx_packets++;
		virt_cep->dev->stats.rx_bytes += skb->len;
		return 1;
	}

	return 0;
} /* process_rx_skb_with_vlantag */

int ca_ni_virt_ni_process_rx_skb(struct sk_buff *skb)
{
	ca_eth_private_t *virt_cep = netdev_priv(skb->dev);
	bool do_tag_check = false;

	switch (virt_cep->port_cfg.port_id) {
	case GE0_PORT:
	case GE1_PORT:
	case GE2_PORT:
		do_tag_check = true;
	}

	if (do_tag_check) {
		return process_rx_skb_with_vlantag(virt_cep->port_cfg.port_id, skb);
	}

	return 0;
} /* ca_ni_virt_ni_process_rx_skb */

int ca_ni_virt_ni_process_tx_skb(struct sk_buff *skb, struct net_device *dev, struct netdev_queue *txq)
{
	ca_eth_private_t *virt_cep = netdev_priv(skb->dev);
	ca_virt_ni_t *p_virt_ni;
	struct sk_buff *nskb;
	struct vlan_ethhdr *veth;

	if (virt_cep->dev == dev)
		return 0;

	p_virt_ni = ca_ni_get_virt_ni(virt_cep->port_cfg.port_id, dev);
	skb->dev = dev;

	if (p_virt_ni == NULL)
		return 0;

	if (!(p_virt_ni->flag & NI_ACTIVE))
		return -1;

	nskb = __vlan_put_tag(skb, virt_cep->vni_vlan_proto, virt_cep->vni_vlan_tci);
	///TODO: need to print out some error messages???
	if (nskb == NULL)
		return -ENOMEM;
	/* need to update the h_vlan_proto to the customized type */
	veth = (struct vlan_ethhdr *)skb->data;
	veth->h_vlan_proto = htons(ETH_P_8021AD);
	skb->protocol = htons(ETH_P_8021AD);
	skb->dev = p_virt_ni->real_dev;
	virt_cep->dev->stats.tx_packets++;
	virt_cep->dev->stats.tx_bytes += skb->len;
	txq_trans_update(txq);
	return 1;
} /* ca_ni_virt_ni_process_tx_skb */

int ca_ni_virt_ni_create_if(int port_id, struct net_device *real_dev)
{
	int j, nr_virt, vid_start, err;
	ca_eth_private_t *real_cep = netdev_priv(real_dev);
	ca_eth_private_t *virt_cep;
	ca_virt_ni_t *p_virt_ni = NULL;

	switch (port_id) {
	case GE0_PORT:
		p_virt_ni = &virt_ni0[0];
		nr_virt = NR_VIRT_NI_ETH0;
		vid_start = VID_START_ETH0;
		break;
	case GE1_PORT:
		p_virt_ni = &virt_ni1[0];
		nr_virt = NR_VIRT_NI_ETH1;
		vid_start = VID_START_ETH1;
		break;
	case GE2_PORT:
		p_virt_ni = &virt_ni2[0];
		nr_virt = NR_VIRT_NI_ETH2;
		vid_start = VID_START_ETH2;
		break;
	default:
		return -ENXIO;
	}

	for (j = 0; j < nr_virt; j++) {
#ifdef CONFIG_NET_SCH_MULTIQ
		p_virt_ni[j].dev = alloc_etherdev_mq(sizeof(ca_eth_private_t), QUEUE_PER_INSTANCE);
#else
		p_virt_ni[j].dev = alloc_etherdev(sizeof(ca_eth_private_t));
#endif
		if (p_virt_ni[j].dev == NULL) {
			printk(KERN_ERR "%s: Unable to alloc virtual Ethernet device#%d_%d\n", real_dev->name, port_id, j);
			return -ENOMEM;
		}
		snprintf(p_virt_ni[j].dev->name, IFNAMSIZ, "%s_%d", real_dev->name, j);

		virt_cep = netdev_priv(p_virt_ni[j].dev);

		/* basically virtual interface's tp is the same as
		 * its physical interface, but some of them. */
		memcpy(virt_cep, real_cep, sizeof(ca_eth_private_t));
		virt_cep->port_cfg.irq[0] = 0;

		p_virt_ni[j].real_dev = real_dev;
		// virt_cep->port_cfg.mac_addr = (u32 *) p_virt_ni[j].mac_addr; // TODO: it's better to use dev->dev_addr
		p_virt_ni[j].dev->netdev_ops = real_dev->netdev_ops;
		p_virt_ni[j].dev->irq = real_cep->port_cfg.irq[0];
		p_virt_ni[j].dev->watchdog_timeo = NI_TX_TIMEOUT;
		p_virt_ni[j].dev->features = NETIF_F_SG | NETIF_F_HW_CSUM;

		/* NETIF_F_TSO nor NETIF_F_TSO6 works, because TSO is
		 * not able to generate the tag automatically.
		 * For VLAN double tag case, if it is just a single tag,
		 * then TSO is supported; however, if user decides to
		 * create another tag, then TSO is not supported. */
		memcpy(&p_virt_ni[j].dev->dev_addr[0], &real_dev->dev_addr[0], p_virt_ni[j].dev->addr_len);
		memcpy(p_virt_ni[j].dev->perm_addr, p_virt_ni[j].dev->dev_addr, p_virt_ni[j].dev->addr_len);
		virt_cep->vni_vlan_tci = (vid_start + j) & VLAN_VID_MASK;
		p_virt_ni[j].flag = NI_INACTIVE;

		// FIXME! do we need the following 2 lines?
		spin_lock_init(&virt_cep->stats_lock);
		spin_lock_init(&virt_cep->lock);
		ca_ni_set_ethtool_ops(p_virt_ni[j].dev);

		// port id of virt_tp
		virt_cep->dev = real_dev;
		virt_cep->port_cfg.port_id = j;

		err = register_netdev(p_virt_ni[j].dev);
		if (err) {
			printk(KERN_ERR "%s: Cannot register net device, aborting.\n", p_virt_ni[j].dev->name);
			free_netdev(p_virt_ni[j].dev);
			return err;
		}
	}
	return 0;
} /* ca_ni_virt_ni_create_if */

int ca_ni_virt_ni_open(u8 port_id, struct net_device *dev)
{
	ca_eth_private_t *cep, *virt_cep = netdev_priv(dev);
	ca_virt_ni_t *virt_ni = ca_ni_get_virt_ni(port_id, dev);

	if (virt_ni->flag == NI_ACTIVE)
		return 0;

	cep = netdev_priv(virt_cep->dev);

	///TODO: should set status=1 after dev_open???
	if (cep->status == 0) {
		/* enable phydev now. */
		dev_open(virt_cep->dev);
		cep->status = 1;
	}

	netif_tx_start_all_queues(dev);
	netif_carrier_on(dev);
	virt_ni->flag = NI_ACTIVE;
	return 0;
} /* ca_ni_virt_ni_open */

/* return 0 when the caller function doesn't have to turn off the physical
 * interface.  return 1 otherwise */
int ca_ni_virt_ni_close(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	ca_virt_ni_t *virt_ni = ca_ni_get_virt_ni(cep->port_cfg.port_id, dev);
	int j, nr_virt_ni;

	if (virt_ni == NULL) {
		switch (cep->port_cfg.port_id) {
		case GE0_PORT:
			virt_ni = &virt_ni0[0];
			nr_virt_ni = NR_VIRT_NI_ETH0;
			break;
		case GE1_PORT:
			virt_ni = &virt_ni1[0];
			nr_virt_ni = NR_VIRT_NI_ETH1;
			break;
		case GE2_PORT:
			virt_ni = &virt_ni2[0];
			nr_virt_ni = NR_VIRT_NI_ETH2;
			break;
		default:
			return 1;
		}
		for (j = 0; j < nr_virt_ni; j++) {
			if (virt_ni->flag == NI_ACTIVE)
				dev_close(virt_ni->dev);
		}
		return 1;
	}
	else {
		netif_tx_disable(dev);
		virt_ni->flag = NI_INACTIVE;
		cep->status = 0;
		return 0;
	}
} /* ca_ni_virt_ni_close */

void ca_ni_virt_ni_remove_if(u8 port_id)
{
	int j, nr_virt_ni;
	ca_virt_ni_t *virt_ni;

	switch (port_id) {
	case GE0_PORT:
		virt_ni = &virt_ni0[0];
		nr_virt_ni = NR_VIRT_NI_ETH0;
		break;
	case GE1_PORT:
		virt_ni = &virt_ni1[0];
		nr_virt_ni = NR_VIRT_NI_ETH1;
		break;
	case GE2_PORT:
		virt_ni = &virt_ni2[0];
		nr_virt_ni = NR_VIRT_NI_ETH2;
		break;
	default:
		return;
	}

	for (j = 0; j < nr_virt_ni; j++) {
		if (virt_ni->dev) {
			unregister_netdev(virt_ni->dev);
		}
	}
} /* ca_ni_virt_ni_remove_if */

