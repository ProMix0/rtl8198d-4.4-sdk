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
 * ca_ethtool.c: Ethtool operations for Cortina NI
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/ethtool.h>
#include <linux/etherdevice.h>
#include <linux/version.h>

#include "aal_common.h"
#include "aal_port.h"
#include "ca_ethtool.h"
#include "ca_ni.h"



/****************************************************************************/

static void ca_ethtool_update_stats(ca_eth_private_t *cep)
{
	unsigned long flags;

	spin_lock_irqsave(&cep->stats_lock, flags);

	///TODO: call to interface table function to retrieve ststs of interface

	spin_unlock_irqrestore(&cep->stats_lock, flags);
}

/****************************************************************************/

static int ca_ethtool_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
	return 0;
#else
	ca_eth_private_t *cep = netdev_priv(dev);
	if (!cep->phydev)
		return -ENODEV;
	return phy_ethtool_gset(cep->phydev, cmd);
#endif
}

static int ca_ethtool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	if (!cep->phydev)
		return -ENODEV;
	return phy_ethtool_sset(cep->phydev, cmd);
}

static void ca_ethtool_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drvinfo)
{
	strlcpy(drvinfo->driver, DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, DRV_VERSION, sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, "platform", sizeof(drvinfo->bus_info));
	drvinfo->n_stats = CA_NUM_STATS;
}

static int ca_ethtool_get_regs_len(struct net_device *dev)
{
	return CA_REGDUMP_LEN;
}

static void ca_ethtool_get_regs(struct net_device *dev, struct ethtool_regs *regs, void *_p)
{
	regs->version = 1;

	memset(_p, 0, CA_REGDUMP_LEN);
///	memcpy_fromio(_p, (void *) NI_TOP_NI_INTF_RST_CONFIG, CA_REGDUMP_LEN);
}

static void ca_ethtool_get_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	pr_err("Not implemented!!!\n");
}

static int ca_ethtool_set_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	return -EOPNOTSUPP;
}

static u32 ca_ethtool_get_msglevel(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	return cep->msg_enable;
}

static void ca_ethtool_set_msglevel(struct net_device *dev, u32 data)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	cep->msg_enable = data;
}

static int ca_ethtool_nway_reset(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	return phy_start_aneg(cep->phydev);
}

static int ca_ethtool_get_coalesce(struct net_device *dev, struct ethtool_coalesce *coal)
{
	return -EOPNOTSUPP;
}

static int ca_ethtool_set_coalesce(struct net_device *dev, struct ethtool_coalesce *coal)
{
	return -EOPNOTSUPP;
}

static void ca_ethtool_get_pauseparam(struct net_device *dev, struct ethtool_pauseparam *pause)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	pause->autoneg = cep->port_cfg.autoneg;

	if (cep->port_cfg.flowctrl & FLOW_CTRL_RX)
		pause->rx_pause = 1;
	else
		pause->rx_pause = 0;

	if (cep->port_cfg.flowctrl & FLOW_CTRL_TX)
		pause->tx_pause = 1;
	else
		pause->tx_pause = 0;
}

static int ca_ethtool_set_pauseparam(struct net_device *dev, struct ethtool_pauseparam *pause)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	struct ethtool_pauseparam orig_pause;
	//int err = 0;

	ca_ethtool_get_pauseparam(dev, &orig_pause);

	cep->port_cfg.autoneg = pause->autoneg;

	if (pause->autoneg != orig_pause.autoneg) {
		cep->port_cfg.flowctrl = pause->rx_pause ? 0 : (FLOW_CTRL_TX | FLOW_CTRL_RX);
	} else {
		if (pause->rx_pause && pause->tx_pause)
			cep->port_cfg.flowctrl = FLOW_CTRL_TX | FLOW_CTRL_RX;
		else if (pause->rx_pause && !pause->tx_pause)
			cep->port_cfg.flowctrl = FLOW_CTRL_RX;
		else if (!pause->rx_pause && pause->tx_pause)
			cep->port_cfg.flowctrl = FLOW_CTRL_TX;
		else if (!pause->rx_pause && !pause->tx_pause)
			cep->port_cfg.flowctrl = 0;
	}

	ca_ni_set_flow_control(&(cep->port_cfg));

	///TODO: should we call back to open/close function ???
#if 0
	if (netif_running(dev)) {
		ca_ni_close(dev);
		err = ca_ni_open(dev);
		if (err) {
			dev_close(dev);
			return err;
		}
	}
#endif

	return 0;
}

static void ca_ethtool_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	switch(stringset) {
	case ETH_SS_STATS:
		memcpy(data, &ethtool_stats_keys, sizeof(ethtool_stats_keys));
		break;
	}
}

static void ca_ethtool_get_ethtool_stats(struct net_device *dev, struct ethtool_stats *stats, u64 *data)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	ca_ethtool_update_stats(cep);

	memcpy(data, &cep->ni_stats, sizeof(cep->ni_stats));
}

static int ca_ethtool_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return CA_NUM_STATS;
	}

	return -EOPNOTSUPP;
}

static const struct ethtool_ops ca_ni_ethtool_ops = {
	.get_settings		= ca_ethtool_get_settings,
	.set_settings		= ca_ethtool_set_settings,
	.get_drvinfo		= ca_ethtool_get_drvinfo,
	.get_regs_len		= ca_ethtool_get_regs_len,
	.get_regs		= ca_ethtool_get_regs,
	.nway_reset		= ca_ethtool_nway_reset,
	.get_wol		= ca_ethtool_get_wol,
	.set_wol		= ca_ethtool_set_wol,
	.get_msglevel		= ca_ethtool_get_msglevel,
	.set_msglevel		= ca_ethtool_set_msglevel,
	.get_link		= ethtool_op_get_link,
	.get_coalesce		= ca_ethtool_get_coalesce,
	.set_coalesce		= ca_ethtool_set_coalesce,
	.get_pauseparam		= ca_ethtool_get_pauseparam,
	.set_pauseparam		= ca_ethtool_set_pauseparam,
	.get_strings		= ca_ethtool_get_strings,
	.get_ethtool_stats	= ca_ethtool_get_ethtool_stats,
	.get_sset_count		= ca_ethtool_get_sset_count,
};

void ca_ni_set_ethtool_ops(struct net_device *dev)
{
	dev->ethtool_ops = &ca_ni_ethtool_ops;
}

