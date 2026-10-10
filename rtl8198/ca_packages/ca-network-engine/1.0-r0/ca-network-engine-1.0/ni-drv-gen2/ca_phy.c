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
 * ca_phy.c: Cortina PHY/MDIO registration
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <linux/phy.h>
#include <linux/mii.h>

#include "aal_port.h"
#include "eth_port.h"
#include "ca_aal.h"
#include "ca_phy.h"
#include "ca_ni.h"
#include "ca_mut.h"


/* Return to REG (reg_addr) data of PHY (phy_addr)
 * The lower 16-bit of returned value are register data.
 * The negative returned value means read error.
 */
static int __ca_phy_read(int phy_addr, int reg_addr)
{
/// These should be done in AAL
#if 0
	u16 poll_time = 0;
	PER_MDIO_CFG_t cfg;
	PER_MDIO_ADDR_t addr;
	PER_MDIO_CTRL_t ctrl;
	//PER_GPIO0_CFG_t gpio_cfg;
	//PER_GPIO0_OUT_t gpio_out;

	if (phy_addr > PER_MDIO_MAX_PHY_ADDR) {
		printk(KERN_ERR "Invalid PHY Addr < 64");
		return -1;
	}

	if (reg_addr > PER_MDIO_MAX_REG_ADDR) {
		printk(KERN_ERR "Invalid MDIO Regiser Addr < 32");
		return -1;
	}

	cfg.wrd = CA_REG_READ(PER_MDIO_CFG);
	/* Set the MDIO mode as Manual */
	cfg.bf.mdio_manual = 1;
	CA_REG_WRITE(cfg.wrd, PER_MDIO_CFG);
	addr.wrd = CA_REG_READ(PER_MDIO_ADDR);
	/* Set the Operation as Read */
	addr.bf.mdio_rd_wr = mdio_rd_wr_RD;
	/* Set the MDIO Address */
	addr.bf.mdio_offset = reg_addr;
	/* Set the PHY Address */
	addr.bf.mdio_addr = phy_addr;
	CA_REG_WRITE(addr.wrd, PER_MDIO_ADDR);

	ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);

	//ctrl.bf.mdiodone = 0;
	//printk("ctrl.bf.mdiodone = 0x%X\n",ctrl.bf.mdiodone);
	/* Start the MDIO Operation */
	ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);

	poll_time = PER_MDIO_POLL_TIME;
	do {
		ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (ctrl.bf.mdiodone)
			break;
	} while (poll_time--);
	if (!poll_time)
		return -1;

	*val = (u16) (CA_REG_READ(PER_MDIO_RDDATA) & 0xffff);

	/* Clear MDIO done */
	ctrl.bf.mdiodone = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);
#endif

	return 0;
}

/* Write val[15:0] to REG (reg_addr) of PHY (phy_addr) */
static int __ca_phy_write(int phy_addr, int reg_addr, int val)
{
/// These should be done in AAL
#if 0
	u16 poll_time = 0;
	PER_MDIO_CFG_t cfg;
	PER_MDIO_ADDR_t addr;
	PER_MDIO_CTRL_t ctrl;
	PER_MDIO_WRDATA_t wrdata;
	//PER_GPIO0_CFG_t gpio_cfg;
	//PER_GPIO0_OUT_t gpio_out;

	if (phy_addr > PER_MDIO_MAX_PHY_ADDR) {
		printk(KERN_ERR "Invalid PHY Addr < 64");
		return -1;
	}

	if (reg_addr > PER_MDIO_MAX_REG_ADDR) {
		printk(KERN_ERR "Invalid MDIO Regiser Addr < 32");
		return -1;
	}

	cfg.wrd = CA_REG_READ(PER_MDIO_CFG);
	/* Set the MDIO mode as Manual */
	cfg.bf.mdio_manual = 1;
	CA_REG_WRITE(cfg.wrd, PER_MDIO_CFG);
	addr.wrd = CA_REG_READ(PER_MDIO_ADDR);
	/* Set the Operation as Write */
	addr.bf.mdio_rd_wr = mdio_rd_wr_WR;
	/* Set the MDIO Address */
	addr.bf.mdio_offset = reg_addr;
	/* Set the PHY Address */
	addr.bf.mdio_addr = phy_addr;
	CA_REG_WRITE(addr.wrd, PER_MDIO_ADDR);

	ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
	wrdata.wrd = CA_REG_READ(PER_MDIO_WRDATA);
	wrdata.bf.mdio_wrdata = val;
	CA_REG_WRITE(wrdata.wrd, PER_MDIO_WRDATA);

	ctrl.bf.mdiodone = 0;
	/* Start the MDIO Operation */
	ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);

	poll_time = PER_MDIO_POLL_TIME;
	do {
		ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (ctrl.bf.mdiodone)
			break;
	} while (poll_time--);
	if (!poll_time)
		return -1;

	/* Clear MDIO done */
	ctrl.bf.mdiodone = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);
#endif
	return 0;
}

/****************************************************************************
 * MDIO
 ****************************************************************************/

static int __ca_mdio_read(struct mii_bus *mii_bus, int phy_addr, int reg_addr)
{
	return __ca_phy_read(phy_addr, reg_addr);
}

static int __ca_mdio_write(struct mii_bus *mii_bus, int phy_addr, int reg_addr, u16 val)
{
	return __ca_phy_write(phy_addr, reg_addr, (int) val);
}

static int __ca_mdio_reset(struct mii_bus *mii_bus)
{
	return 0;
}

int ca_mdio_init(ca_eth_private_t *cep)
{
	int i, err;

	cep->mdio_bus = mdiobus_alloc();
	if (cep->mdio_bus == NULL) {
		err = -ENOMEM;
		goto err_out;
	}

	cep->mdio_bus->name = "ca-mdio";
	//snprintf(cep->mdio_bus->id, MII_BUS_ID_SIZE, "%d", cep->port_cfg.port_id);
	cep->mdio_bus->priv = cep;
	//cep->mdio_bus->parent = &cep->dev->dev;
	cep->mdio_bus->read = &__ca_mdio_read;
	cep->mdio_bus->write = &__ca_mdio_write;
	cep->mdio_bus->reset = &__ca_mdio_reset;
	cep->mdio_bus->phy_mask = ~0; /// TODO: make sure the phy_id assigment

	cep->mdio_bus->irq = ca_malloc_trace(sizeof(int) * PHY_MAX_ADDR, GFP_KERNEL, true);
	if (!cep->mdio_bus->irq) {
		err = -ENOMEM;
		goto err_free_mdio;
	}

	for (i = 0; i < PHY_MAX_ADDR; i++)
		cep->mdio_bus->irq[i] = PHY_POLL;

	err = mdiobus_register(cep->mdio_bus);
	if (err)
		goto err_kfree;

	return 0;

err_kfree:
	ca_free_trace(cep->mdio_bus->irq);
err_free_mdio:
	mdiobus_free(cep->mdio_bus);
err_out:
	return err;
}

int ca_mdio_exit(ca_eth_private_t *cep)
{
	mdiobus_unregister(cep->mdio_bus);
	ca_free_trace(cep->mdio_bus->irq);
	mdiobus_free(cep->mdio_bus);

	return 0;
}

/****************************************************************************
 * PHY
 ****************************************************************************/

void ca_phy_adjust_link(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	struct phy_device *phydev = cep->phydev;
	//unsigned long flags;
	int status_changed = 0;

	/// spin_lock_irqsave(&cep->hw_lock, flags);  need?

	if (phydev->link) {

#if 0
		if ( (phydev->speed != cep->port_cfg.speed) || (phydev->duplex != cep->port_cfg.duplex) ) {

			///   handle MII error

			///   duplex link change

			///   link on or off

		} /* if speed or duplex not match */
#endif

	}

	/// spin_lock_irqrestore(&cep->hw_lock, flags);  need?


	if (status_changed && netif_msg_link(cep)) {
		phy_print_status(phydev);
	}

}

int ca_phy_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	struct mii_ioctl_data *mii_data = if_mii(ifr);

	if (!netif_running(dev))
		return -EINVAL;

	if (cep->phydev == NULL)
		return -ENOTSUPP;

	switch (cmd) {
	case SIOCGMIIPHY:
		mii_data->phy_id = cep->phydev->addr;
		break;

	case SIOCGMIIREG:
		return 0;

	case SIOCSMIIREG:
		return 0;

	default:
		return -EOPNOTSUPP;

	}

	return 0;
}

/// parameter:
/// 1. internal data structure
/// 2. phy addr, usually defined in compiler time, depend on CONFIG_ca_BOARDNAME
int ca_phy_init(ca_eth_private_t *cep, int phy_addr)
{
	int data;
	uint32_t phy_id;
	char mii_name[MII_BUS_ID_SIZE + 3];
	char *phy_name = NULL;

	/* reset PHY */
	if ((data = __ca_phy_read(phy_addr, MII_BMCR)) < 0)
		return -ENODEV;

	if (__ca_phy_write(phy_addr, MII_BMCR, data | BMCR_RESET) < 0)
		return -ENODEV;

	do {
		data = __ca_phy_read(phy_addr, MII_BMCR);
	} while (data >= 0 && (data & BMCR_RESET));

	/* read PHY ID */
	if ((data = __ca_phy_read(phy_addr, MII_PHYSID1)) < 0)
		return -ENODEV;
	phy_id = ((data & 0xffff) << 16);

	if ((data = __ca_phy_read(phy_addr, MII_PHYSID2)) < 0)
		return -ENODEV;
	phy_id |= (data & 0xffff);

	snprintf(mii_name, sizeof(mii_name), PHY_ID_FMT, "g3-phy", phy_addr);
	//cep->phydev = phy_connect(cep->dev, mii_name, &ca_phy_adjust_link, cep->port_cfg.phy_mode);

	switch (cep->phydev->interface) {
	case PHY_INTERFACE_MODE_GMII:
	case PHY_INTERFACE_MODE_RGMII:
		cep->phydev->supported |= (PHY_GBIT_FEATURES |
				SUPPORTED_Pause |
				SUPPORTED_Asym_Pause);
		break;
	case PHY_INTERFACE_MODE_RMII:
	case PHY_INTERFACE_MODE_MII:
		cep->phydev->supported |= (PHY_BASIC_FEATURES |
				SUPPORTED_Pause |
				SUPPORTED_Asym_Pause);
		break;
	default:
		phy_disconnect(cep->phydev);
		return -EINVAL;
	}

	cep->phydev->advertising = cep->phydev->supported;

	/// TODO: set cep->port_cfg??? This should be pre-defined


	/// TODO: Write cfg to AAL
	/// ca_ni_set_eth_cfg(cep, NI_CONFIG_1);
	/// ca_ni_set_mac_speed_duplex(cep, cep->port_cfg.phy_mode);

	switch (phy_id) {
	case PHY_ID_RTL8201E:
		phy_name = "Realtek RTL8201E";
		break;

	case PHY_ID_RTL8201F:
		phy_name = "Realtek RTL8201F";
		break;

	case PHY_ID_RTL8211C:
		phy_name = "Realtek RTL8211C";
		break;

	case PHY_ID_RTL8211E:
		phy_name = "Realtek RTL8211E";
		break;

	case PHY_ID_RTL8214C:
		phy_name = "Realtek RTL8214C";
		break;

	case PHY_ID_ATH8021:
		phy_name = "Atheros AR8021";
		break;

	case PHY_ID_ATH8032:
		phy_name = "Atheros AR8032";
		break;

	case PHY_ID_QCA8337:
		phy_name = "Qualcomm Atheros QCA8337(N)";
		break;

	default:
		phy_name = "Unknown";
		break;
	}

	printk(KERN_INFO "%s: Found an attached PHY\n", cep->dev->name);
	printk(KERN_INFO "\t* Driver name: %s\n", cep->phydev->drv->name);
	printk(KERN_INFO "\t* PHY name: %s\n", phy_name);
	printk(KERN_INFO "\t* PHY ID: 0x%x\n", phy_id);
	printk(KERN_INFO "\t* PHY addr: %d\n", phy_addr);

	return 0;
}

int ca_phy_exit(ca_eth_private_t *cep)
{
	if (cep->phydev != NULL)
		phy_detach(cep->phydev);

	return 0;
}

