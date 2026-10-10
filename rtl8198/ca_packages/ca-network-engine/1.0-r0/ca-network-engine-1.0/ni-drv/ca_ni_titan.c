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
 * ca_ni_titan.c: Cortina Access Network Interface driver for Titan FPGA only
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_arp.h>
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

#include <dt-bindings/soc/cortina_irqs.h>
#endif /* DT support */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <soc/cortina/cortina-soc.h>
#include <soc/cortina/registers.h>

#include "ca_types.h"
#include "osal_cmn.h"

#include "ca_ni_common.h"
#include "ca_ni_titan.h"
#include "ca_ni_proc.h"

#include "ca_mut.h"
#include "rx.h"

/* define RX_POLLING to use polling mode instead of interrupt mode */
#undef RX_POLLING

/* debug flags */
u32 ca_ni_debug = 0;

static const u32 default_msg = NETIF_MSG_DRV | NETIF_MSG_PROBE
	| NETIF_MSG_LINK | NETIF_MSG_TIMER | NETIF_MSG_IFDOWN
	| NETIF_MSG_IFUP | NETIF_MSG_RX_ERR | NETIF_MSG_TX_ERR;

ni_info_t ni_info_data;

int ca_ni_mgmt_mode = 0;
static int ni_initialized = 0;
int ca_ni_num_intf = CA_NI_MAX_INTF;

#ifdef CONFIG_CORTINA_MUT
extern struct kmem_cache *cache_16;	/* cache for 16-byte objects */
#endif

int ca_ni_netlink_rx_init(void);
void ca_ni_netlink_rx_exit(void);

#ifdef RX_POLLING
static void ca_ni_rx_poll_timer_cb(unsigned long data);
#endif

static void ni_rx_set_interrupt(struct net_device *dev, bool enable);

/***************************************************************************
 * dump functions
 ***************************************************************************/

static inline void dump_hdr_a(HEADER_A_T *hdr_a)
{
	printk("HEADER A:\n");
	printk("\t cos(3) %u, ldpid/mcidx(6) 0x%02x, lspid(6) 0x%02x, pkt_size %u, fe_bypass %u, hdr_type %u ",
			hdr_a->bits.cos, hdr_a->bits.ldpid, hdr_a->bits.lspid, hdr_a->bits.pkt_size, hdr_a->bits.fe_bypass, hdr_a->bits.hdr_type);
	switch (hdr_a->bits.hdr_type) {
	case HEADER_TYPE_UC:
		printk("[UC HDR]\n");
		break;
	case HEADER_TYPE_MC:
		printk("[MC HDR]\n");
		break;
	case HEADER_TYPE_CPU_BOUND:
		printk("[CPU bound packet]\n");
		break;
	case HEADER_TYPE_PTP:
		printk("[PTP packet]\n");
		break;
	}

	if (hdr_a->bits.hdr_type == HEADER_TYPE_PTP) {

		printk("\t cmd_asel(2) %u, cmd_op(2) %u ", hdr_a->bits.bits_32_63.ptp_cmd.cmd_asel, hdr_a->bits.bits_32_63.ptp_cmd.cmd_op);
		switch (hdr_a->bits.bits_32_63.ptp_cmd.cmd_op) {
		case 0: printk("[NO_OP]"); break;
		case 1: printk("[TS_CORRECT]"); break;
		case 2: printk("[TS_P_OFFSET]"); break;
		case 3: printk("[TS_N_OFFSET]"); break;
		}
		printk(", cmd_mode %u ", hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode);
		switch (hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode) {
		case 0: printk("[Timestamp mode]"); break;
		case 1: printk("[Correction mode]"); break;
		}
		printk("\n");

		printk("\t ptp_type(2) %u ", hdr_a->bits.bits_32_63.ptp_cmd.ptp_type);
		switch (hdr_a->bits.bits_32_63.ptp_cmd.ptp_type) {
		case 0: printk("[PTP over Ethernet]"); break;
		case 1: printk("[CFM]"); break;
		case 2: printk("[PTP over UDP over IPv4]"); break;
		case 3: printk("[PTP over UDP over IPv6]"); break;
		}
		printk(", cmd_offset(7) 0x%02x, udp_cksum_offset(7) 0x%02x, cpu_flg %u\n",
			hdr_a->bits.bits_32_63.ptp_cmd.cmd_offset, hdr_a->bits.bits_32_63.ptp_cmd.udp_cksum_offset,
			hdr_a->bits.bits_32_63.ptp_cmd.cpu_flg);

	} else {

		printk("\t mcgid(8) 0x%02x, drop_code(3) %u, no_drop %u, mirror %u, mark %u, rx_pkt_type(2) %u ",
			hdr_a->bits.bits_32_63.pkt_info.mcgid, hdr_a->bits.bits_32_63.pkt_info.drop_code,
			hdr_a->bits.bits_32_63.pkt_info.no_drop, hdr_a->bits.bits_32_63.pkt_info.mirror,
			hdr_a->bits.bits_32_63.pkt_info.mark, hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type);
		switch (hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type) {
		case 0: printk("[UC]\n"); break;
		case 1: printk("[BC]\n"); break;
		case 2: printk("[MC]\n"); break;
		case 3: printk("[Unknown-UC]\n"); break;
		}

		printk("\t deep_q %u, cpu_flg %u, pol_id(9) 0x%03x, pol_grp_id(3) %x, pol_en %u ",
			hdr_a->bits.bits_32_63.pkt_info.deep_q, hdr_a->bits.bits_32_63.pkt_info.cpu_flg,
			hdr_a->bits.bits_32_63.pkt_info.pol_id, hdr_a->bits.bits_32_63.pkt_info.pol_grp_id,
			hdr_a->bits.bits_32_63.pkt_info.pol_en);
		switch (hdr_a->bits.bits_32_63.pkt_info.pol_en) {
		case 0: printk("\n"); break;
		case 1: printk("[L2 Policer EN]\n"); break;
		case 2: printk("[L3 Policer EN]\n"); break;
		case 3: printk("[L2 & L3 Policer EN]\n"); break;
		}

	}
}

/* Dump packet; start from data[0] to data[len - 1].
 * - data: packet contents, must start from MAC_DA.
 * - data_len: packet length to dump.
 */
static inline void __dump_pkts(unsigned char *data, int data_len)
{
	struct iphdr *iph;
	struct ipv6hdr *iph6;
	struct arphdr *arp;
	unsigned char *cp;
	uint16_t protocol;

	cp = data;
	cp = cp + 12;
	protocol = (uint16_t) (*cp | (*(cp+1) << 8));

	if ( (protocol == ntohs(ETH_P_8021Q)) || (protocol == ntohs(ETH_P_8021AD)) || (protocol == ntohs(ETH_P_QINQ1)) ) {
		printk("Outer VLAN tag: %02x %02x %02x %02x\n", *cp, *(cp+1), *(cp+2), *(cp+3));
		cp = cp + 4;
		protocol = (uint16_t) (*cp | (*(cp+1) << 8));
	}
	if ( (protocol == ntohs(ETH_P_8021Q)) || (protocol == ntohs(ETH_P_8021AD)) || (protocol == ntohs(ETH_P_QINQ1)) ) {
		printk("Inner VLAN tag: %02x %02x %02x %02x\n", *cp, *(cp+1), *(cp+2), *(cp+3));
		cp = cp + 4;
		protocol = (uint16_t) (*cp | (*(cp+1) << 8));
	}

	if (protocol == htons(ETH_P_IP)) {

		cp = cp + 2;
		iph = (struct iphdr *) cp;

		printk("IPv4 HDR: ver %d, hdr_len %d, dscp %d, ecn %d\n",
			iph->version, iph->ihl, iph->tos >> 2, iph->tos & 0x3);
		printk("\t  total_len %d, id(be) 0x%04X, df %d, mf %d, ttl %d, protocol 0x%02X\n",
			ntohs(iph->tot_len), iph->id,
			(ntohs(iph->frag_off) & 0x4000) ? 1 : 0,
			(ntohs(iph->frag_off) & 0x8000) ? 1 : 0,
			iph->ttl, iph->protocol);
		printk("\t  saddr %pI4\n", &iph->saddr);
		printk("\t  daddr %pI4\n", &iph->daddr);

	} else if (protocol == htons(ETH_P_IPV6)) {

		cp = cp + 2;
		iph6 = (struct ipv6hdr *) cp;

		printk("IPv6 HDR: ver %d, priority %d, payload_len %d, next_hdr 0x%02X, hop_limit %d\n",
			iph6->version, iph6->priority, ntohs(iph6->payload_len), iph6->nexthdr, iph6->hop_limit);
		printk("\t  saddr %pI6\n", &iph6->saddr);
		printk("\t  daddr %pI6\n", &iph6->daddr);

	} else if (protocol == htons(ETH_P_ARP)) {

		cp = cp + 2;
		arp = (struct arphdr *) cp;

		printk("ARP HDR: hw_type %d, protocol 0x%04X, hw_addr_size %d, protocol_size %d, opcode %d\n",
			ntohs(arp->ar_hrd), ntohs(arp->ar_pro), arp->ar_hln, arp->ar_pln, ntohs(arp->ar_op));

	}

	if (data_len > 1448) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, 1448, true);
	}
	else {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, data_len, true);
	}
}

static inline void dump_rx_pkts(struct sk_buff *skb)
{
	printk("====== RX data@%p, dump len %d, skb->len %d ======\n", skb->data - 14, skb->len + 14, skb->len);
	__dump_pkts(skb->data - 14, skb->len + 14);
}

static inline void dump_tx_pkts(unsigned char *data, int len)
{
	printk("====== TX data@%p, len %d ======\n", data, len);
	__dump_pkts(data, len);
}

/***************************************************************************
 * MDIO functions
 ***************************************************************************/

#define MDIO_ACCESS_TIMEOUT	1000000

static uint16_t titan_mdio_read(int reg)
{
	PER_MDIO_ADDR_t addr;
	PER_MDIO_CTRL_t ctrl;
	PER_MDIO_RDDATA_t data;
	unsigned int loop;

	addr.wrd = 0;
	addr.bf.mdio_rd_wr = 1; // read
	addr.bf.mdio_offset = reg;
	CA_REG_WRITE(addr.wrd, PER_MDIO_ADDR);

	ctrl.wrd = 0;
	ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);

	loop = MDIO_ACCESS_TIMEOUT;
	do {
		ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (ctrl.bf.mdiodone) {
			CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);
			break;
		}
	} while (--loop);

	data.wrd = CA_REG_READ(PER_MDIO_RDDATA);

	return data.bf.mdio_rddata;
}

static int titan_mdio_write(int reg, uint16_t val)
{
	PER_MDIO_ADDR_t addr;
	PER_MDIO_CTRL_t ctrl;
	PER_MDIO_WRDATA_t data;
	unsigned int loop;

	data.wrd = 0;
	data.bf.mdio_wrdata = val;
	CA_REG_WRITE(data.wrd, PER_MDIO_WRDATA);

	addr.wrd = 0;
	addr.bf.mdio_rd_wr = 0; // write
	addr.bf.mdio_offset = reg;
	CA_REG_WRITE(addr.wrd, PER_MDIO_ADDR);

	ctrl.wrd = 0;
	ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);

	loop = MDIO_ACCESS_TIMEOUT;
	do {
		ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (ctrl.bf.mdiodone) {
			CA_REG_WRITE(ctrl.wrd, PER_MDIO_CTRL);
			break;
		}
	} while (--loop);

	return 0;
}

static int titan_mdio_init(void)
{
	uint16_t val;

	/* disable 1000Base-T Full capability */
	val = titan_mdio_read(MII_CTRL1000);
	val &= ~(ADVERTISE_1000FULL);
	titan_mdio_write(MII_CTRL1000, val);

	/* reset PHY */
	val = titan_mdio_read(MII_BMCR);
	val |= (BMCR_RESET);
	titan_mdio_write(MII_BMCR, val);

	return 0;
}

/***************************************************************************
 * proc entry for titan only
 ***************************************************************************/

struct proc_dir_entry *proc_titan = NULL;

#define LAN_FPS_HELP_MSG \
	"Purpose: Flow Control on LAN interface by setting FPS (frames per second).\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo FPS > %s\n" \
	"  0: Disable flow control\n" \
	"  Any positive number: Enable flow control\n"

static int lan_fps_show_help(struct seq_file *m, void *v)
{
	struct net_device *dev = ni_info_data.dev[1];
	ca_eth_private_t *cep = netdev_priv(dev);
	struct file *file = m->private;

	seq_printf(m, LAN_FPS_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\nLAN FPS = %u\n", cep->fps);

	return 0;
}

int lan_fps_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, lan_fps_show_help, (void *) file);
}

ssize_t lan_fps_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	struct net_device *dev = ni_info_data.dev[1];
	ca_eth_private_t *cep = netdev_priv(dev);
	char tmp[32];
	unsigned long fps;
	ssize_t len;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _lan_fps_invalid;

	tmp[len] = '\0';
	if (kstrtoul(tmp, 0, &fps))
		goto _lan_fps_invalid;

	if (fps == 0) {
		cep->fps = 0;
		del_timer(&cep->flow_ctrl_timer);
		ni_rx_set_interrupt(dev, true);
	} else {
		cep->fps = fps;
		if (!timer_pending(&cep->flow_ctrl_timer))
			mod_timer(&cep->flow_ctrl_timer, jiffies + msecs_to_jiffies(1000));
	}

	printk(KERN_INFO "LAN FPS = %u\n", cep->fps);

	return count;

_lan_fps_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_lan_fps __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= lan_fps_proc_open,
	.read		= seq_read,
	.write		= lan_fps_proc_write,
};

#define WAN_FPS_HELP_MSG \
	"Purpose: Flow Control on WAN interface by setting FPS (frames per second).\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo FPS > %s\n" \
	"  0: Disable flow control\n" \
	"  Any positive number: Enable flow control\n"

static int wan_fps_show_help(struct seq_file *m, void *v)
{
	struct net_device *dev = ni_info_data.dev[0];
	ca_eth_private_t *cep = netdev_priv(dev);
	struct file *file = m->private;

	seq_printf(m, WAN_FPS_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\nWAN FPS = %u\n", cep->fps);

	return 0;
}

int wan_fps_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, wan_fps_show_help, (void *) file);
}

ssize_t wan_fps_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	struct net_device *dev = ni_info_data.dev[0];
	ca_eth_private_t *cep = netdev_priv(dev);
	char tmp[32];
	unsigned long fps;
	ssize_t len;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _wan_fps_invalid;

	tmp[len] = '\0';
	if (kstrtoul(tmp, 0, &fps))
		goto _wan_fps_invalid;

	if (fps == 0) {
		cep->fps = 0;
		del_timer(&cep->flow_ctrl_timer);
		ni_rx_set_interrupt(dev, true);
	} else {
		cep->fps = fps;
		if (!timer_pending(&cep->flow_ctrl_timer))
			mod_timer(&cep->flow_ctrl_timer, jiffies + msecs_to_jiffies(1000));
	}

	printk(KERN_INFO "WAN FPS = %u\n", cep->fps);

	return count;

_wan_fps_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_wan_fps __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= wan_fps_proc_open,
	.read		= seq_read,
	.write		= wan_fps_proc_write,
};

int ca_titan_proc_init(void)
{
	proc_titan = proc_mkdir("titan", ca_proc_dir);

	if (proc_create("wan_fps", S_IRUGO | S_IWUGO, proc_titan, &fops_wan_fps) == NULL) {
		pr_err("Fail to create proc entry 'wan_fps'.\n");
		return -ENOMEM;
	}

	if (proc_create("lan_fps", S_IRUGO | S_IWUGO, proc_titan, &fops_lan_fps) == NULL) {
		pr_err("Fail to create proc entry 'lan_fps'.\n");
		return -ENOMEM;
	}

	return 0;
}

void ca_titan_proc_exit(void)
{
	remove_proc_entry("wan_fps", proc_titan);
	remove_proc_entry("lan_fps", proc_titan);
	proc_remove(proc_titan);
}

/***************************************************************************
 * internal NI functions
 ***************************************************************************/

static void flow_ctrl_timer_cb(unsigned long data)
{
	struct net_device *dev = (struct net_device *) data;
	ca_eth_private_t *cep = netdev_priv(dev);
	unsigned long next_time = jiffies + msecs_to_jiffies(1000);

	if (cep->fps == 0) return;

	if (atomic_read(&cep->in_irq_handler))
		goto _end_flow_ctrl;

	atomic_set(&cep->flow_cnt, 0);

	ni_rx_set_interrupt(cep->dev, true);

_end_flow_ctrl:
	if (time_is_after_eq_jiffies(next_time))
		mod_timer(&cep->flow_ctrl_timer, next_time);
	else
		mod_timer(&cep->flow_ctrl_timer, jiffies + 1);
}

static void allow_lan(bool enable)
{
	TITANGLB_FPGA_IDP_CNTL_t idp_ctrl;
	FPGA_RGMII_RXMAC_CFG_t rxmac_cfg;
	FPGA_RGMII_TXMAC_CFG_t txmac_cfg;

	idp_ctrl.wrd = CA_REG_READ(TITANGLB_FPGA_IDP_CNTL);
	rxmac_cfg.wrd = CA_REG_READ(FPGA_RGMII_RXMAC_CFG);
	txmac_cfg.wrd = CA_REG_READ(FPGA_RGMII_TXMAC_CFG);
	if (enable) {
			idp_ctrl.bf.lan2cpu_en = 1;
			idp_ctrl.bf.cpu2lan_en = 1;
			rxmac_cfg.bf.rx_en = 1;
			txmac_cfg.bf.tx_en = 1;
	} else {
			idp_ctrl.bf.lan2cpu_en = 0;
			idp_ctrl.bf.cpu2lan_en = 0;
			rxmac_cfg.bf.rx_en = 0;
			txmac_cfg.bf.tx_en = 0;
	}
	CA_REG_WRITE(idp_ctrl.wrd, TITANGLB_FPGA_IDP_CNTL);
	CA_REG_WRITE(rxmac_cfg.wrd, FPGA_RGMII_RXMAC_CFG);
	CA_REG_WRITE(txmac_cfg.wrd, FPGA_RGMII_TXMAC_CFG);
}

static void ni_rx_set_interrupt(struct net_device *dev, bool enable)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	ca_reg_interrupt_t inte;

	/* force to clear all interrupt status */
	if (enable)
		CA_REG_WRITE(0xffffffff, (ca_uint32_t) cep->reg_rx_int);

	inte.wrd = CA_REG_READ((ca_uint32_t) cep->reg_rx_int + 4);
	if (cep->index == 0) {
		inte.wan.rx_pkt_rdy = (enable) ? 1 : 0;
	} else {
		inte.lan.rx_pkt_rdy = (enable) ? 1 : 0;
	}
	CA_REG_WRITE(inte.wrd, (ca_uint32_t) cep->reg_rx_int + 4);
}

int ca_ni_rx_rcv_packet(struct net_device *dev)
{
	struct sk_buff *skb;
	FPGA_IDP_DOWN_CNTL_t idp_down_ctrl;
	ca_reg_interrupt_t intr;
	int i;
	uint32_t *src_ptr, *dst_ptr;
	ca_eth_private_t *cep = netdev_priv(dev);
	struct net_device_stats *stats = &cep->dev->stats;
	int ret;
	unsigned int pkt_len;
	HEADER_A_T hdr_a;
	uint32_t *p_uint32 = NULL;

	/* indicate that it is in IRQ handler now, avoid enabling IRQ by flow control timer */
	atomic_set(&cep->in_irq_handler, 1);

	/* SW flow control */
	if (cep->fps != 0) {
		atomic_inc(&cep->flow_cnt);
	}

	/* disable interrupt */
	ni_rx_set_interrupt(dev, false);

	intr.wrd = CA_REG_READ((ca_uint32_t) cep->reg_rx_int);
	if (cep->index == 0) {
		if (intr.wan.rx_pkt_rdy == 0)
			goto rx_done;
	} else {
		if (intr.lan.rx_pkt_rdy == 0)
			goto rx_done;
	}

	pkt_len = CA_REG_READ((ca_uint32_t) cep->reg_rx_pkt_size);
	if (pkt_len > MAX_FRAME_SIZE) {
		stats->rx_fifo_errors++;
		goto rx_done;
	}

	skb = netdev_alloc_skb(dev, AAL_IDP_PKT_SIZE + 0x100);

	if (!skb) {
		printk(KERN_ERR "%s: Not enough memory to allocate skb of size %d.\n", dev->name, AAL_IDP_PKT_SIZE + 0x100);
		goto rx_done;
	}

	skb->len = pkt_len;
	skb->data = skb->head + 0x100;
	src_ptr = (ca_uint32_t *) cep->reg_rx_data;
	dst_ptr = (ca_uint32_t *) skb->data;
	//pkt read
	for (i = 0; i < ((skb->len + 4 - 1) / 4); i++) {
		*(dst_ptr) = (ca_uint32_t) htonl(CA_REG_READ((ca_uint32_t) src_ptr));
		dst_ptr++;
		src_ptr++;
	}

	if (cep->index == 0) {
		/* HEADER_A_T is host order of quadword, but skb->data is network order of quadword */
		p_uint32 = (uint32_t *) skb->data;
		hdr_a.bits64 = ((uint64_t) ntohl(*p_uint32++)) << 32;
		hdr_a.bits64 |= (uint64_t) ntohl(*p_uint32);

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("====== %s: RX HDR_A @ %p ======\n", dev->name, &hdr_a);
			dump_hdr_a(&hdr_a);
		}

		/* skip header A */
		skb_pull(skb, sizeof(HEADER_A_T));
	}

	skb->tail = skb->data + skb->len;
	skb->data_len = 0; /* skb->data_len will be 0 or greater than 0 (jumbo packet) */

	/* end of skb list */
	skb->next = NULL;
	skb->prev = NULL;

	skb->dev = dev;
	skb->ip_summed = CHECKSUM_NONE;

	/* after eth_type_trans(), skb->data move to begine of IP header */
	skb->protocol = eth_type_trans(skb, skb->dev);

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("====== %s: reg_rx_pkt_size: %d ======\n", dev->name, pkt_len);
		dump_rx_pkts(skb);
	}

	ret = netif_rx(skb);
	if (ret == NET_RX_DROP) {
		stats->rx_dropped++;
	} else {
		stats->rx_packets++;
		stats->rx_bytes += skb->len;
	}

rx_done:
	/* 1st, clear interrupt cause; reg is "write 1 to clear" */
//	intr.wrd = 0;
	if (cep->index == 0) {
		intr.wan.rx_pkt_rdy = 1;
		intr.wan.rx_oversize = 1;
	} else {
		intr.lan.rx_pkt_rdy = 1;
		intr.lan.rx_oversize = 1;
	}
	CA_REG_WRITE(intr.wrd, (ca_uint32_t) cep->reg_rx_int);

	atomic_set(&cep->in_irq_handler, 0);

	/* 2nd, enable interrupt */
	if ((cep->fps == 0) || (atomic_read(&cep->flow_cnt) < cep->fps)) {
		/* SW flow control; if fps is lower than threshold, enable interrupt */
		ni_rx_set_interrupt(dev, true);
	}

	/* 3rd, clear cpu_read_done; reg is "write 1 to clear" */
	idp_down_ctrl.wrd = 0;
	idp_down_ctrl.bf.cpu_read_done = 1;
	CA_REG_WRITE(idp_down_ctrl.wrd, (ca_uint32_t) cep->reg_rx_ctrl);

	return 0;
}

/* NOTE: buf_ptr must reserve the memory space for CRC and minimal frame padding */
static ca_uint32_t __idp_send_pkt(struct net_device *dev, ca_uint8_t * buf_ptr, ca_uint32_t pkt_len)
{
	ca_eth_private_t *cep = netdev_priv(dev);
	struct net_device_stats *stats = &cep->dev->stats;
	unsigned long flags;
	HEADER_A_T hdr_a;
	FPGA_IDP_UP_CPU_CNTL_t idp_up_cpu_ctrl; /* share bitfields with FPGA_IDP_DOWN_CPU2LAN_CNTL */
	int i;
	ca_uint32_t *src_ptr;
	ca_uint32_t *dst_ptr = cep->reg_tx_data;
	ca_uint32_t write_len; /* not include hdr_a */

	if (pkt_len > MAX_FRAME_SIZE) {
		stats->tx_fifo_errors++;
		return NETDEV_TX_OK;
	}

	/* for minimal frame padding and CRC */
	write_len = (pkt_len <= ETH_ZLEN) ? (ETH_ZLEN + ETH_FCS_LEN) : (pkt_len + ETH_FCS_LEN);

	spin_lock_irqsave(&cep->lock, flags);

	idp_up_cpu_ctrl.wrd = CA_REG_READ((ca_uint32_t) cep->reg_tx_ctrl);
	if (idp_up_cpu_ctrl.bf.cpu_write_done == 1) {
		spin_unlock_irqrestore(&cep->lock, flags);
		return NETDEV_TX_BUSY;
	}

	if (cep->index == 0) {
		/* set Header_A */
		hdr_a.bits64 = 0;
		hdr_a.bits.cos = 0;
		hdr_a.bits.pkt_size = write_len;
		hdr_a.bits.ldpid = (cep->port_id != CA_UINT32_INVALID) ? cep->port_id : 0;
		hdr_a.bits.lspid = 0x10;
		hdr_a.bits.fe_bypass = 1;
		hdr_a.bits.bits_32_63.pkt_info.no_drop = 1;

		if (ca_ni_debug & NI_DBG_DUMP_TX) {
			printk("====== %s: TX HDR_A @ %p ======\n", dev->name, &hdr_a);
			dump_hdr_a(&hdr_a);
		}

		/* write hdr_a to reg */
		src_ptr = (ca_uint32_t *) &hdr_a;
		for (i = 0; i < sizeof(HEADER_A_T) / 4; i++) {
			/* hdr_a is host order of quadword */
			if (i % 2 == 0)
				CA_REG_WRITE(*src_ptr, (ca_uint32_t) (dst_ptr + 1));
			else
				CA_REG_WRITE(*src_ptr, (ca_uint32_t) (dst_ptr - 1));

			dst_ptr++;
			src_ptr++;
		}

		CA_REG_WRITE(write_len + sizeof(HEADER_A_T), (ca_uint32_t) cep->reg_tx_pkt_size);
	} else {
		CA_REG_WRITE(write_len, (ca_uint32_t) cep->reg_tx_pkt_size);
	}

	if (ca_ni_debug & NI_DBG_DUMP_TX) {
		dump_tx_pkts(buf_ptr, pkt_len);
	}

	/* write skb->data to reg */
	src_ptr = (ca_uint32_t *) buf_ptr;
	for (i = 0; i < ((write_len + 4 - 1) / 4); i++) {
		CA_REG_WRITE(ntohl(*src_ptr), (ca_uint32_t) dst_ptr);
		dst_ptr++;
		src_ptr++;
	}

	idp_up_cpu_ctrl.bf.cpu_write_done = 1;
	CA_REG_WRITE(idp_up_cpu_ctrl.wrd, (ca_uint32_t) cep->reg_tx_ctrl);

	stats->tx_packets++;
	stats->tx_bytes += write_len;
	spin_unlock_irqrestore(&cep->lock, flags);

	return NETDEV_TX_OK;
}

int ca_ni_send_packet(u8 *pkt_data, int length, ca_uint16_t ldpid, int txq_index)
{
	ca_eth_private_t *cep = NULL;
	struct net_device *dev;
	ca_uint8_t * buf;
	size_t alloc_len;
	int i;

	dev = ni_info_data.dev[1];	/* default Tx dev */
	for (i = 0; i < ca_ni_num_intf; i++) {
		cep = netdev_priv(ni_info_data.dev[i]);
		if (ldpid == cep->port_id) {
			dev = ni_info_data.dev[i];
			break;
		}
	}

	if (dev == NULL) {
		printk("%s: dev is null, ldpid=%u, txq_index=%d.\n", __func__, ldpid, txq_index);
		return -1;
	}
	if (length > MAX_FRAME_SIZE) {
		printk("%s: packet length %d exceeds maximum %d.\n", __func__, length, MAX_FRAME_SIZE);
		return -1;
	}

	/* reserved for CRC and minimal frame size */
	if (length <= ETH_ZLEN) {
		alloc_len = ETH_ZLEN + ETH_FCS_LEN + 0x20;
	} else {
		alloc_len = length + ETH_FCS_LEN + 0x20;
	}

	buf = kmalloc(alloc_len, GFP_ATOMIC);
	if (buf == NULL) {
		printk("%s: Out of memory, length=%d.\n", __func__, length);
		return -1;
	}

	copy_from_user(buf, pkt_data, length);
	__idp_send_pkt(dev, buf, length);

	kfree(buf);
	return 0;
}

irqreturn_t ca_ni_rx_interrupt(int irq, void *dev_instance)
{
	struct net_device *dev = dev_instance;
	ca_eth_private_t *cep = netdev_priv(dev);

	/* receive packet from buffer */
	ca_ni_rx_rcv_packet(dev);

	return IRQ_HANDLED;
}

int ca_ni_init_l3qm(void)
{
	return 0;
}

void ca_ni_init_fbm(void)
{

}

void ca_ni_init_reserved_mem(void)
{

}

ca_status_t ca_rx_unregister_pkt(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_uint8_t	priority,
	CA_IN ca_pkt_type_t	type)
{
	printk("%s not support in titan!!\n", __func__);
	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_rx_unregister_pkt);

ca_status_t ca_rx_register_pkt(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_uint8_t	priority,
	CA_IN ca_pkt_type_t	type)
{
	printk("%s not support in titan!!\n", __func__);
	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_rx_register_pkt);

static void ca_ni_reg_reset(void)
{
	TITANGLB_FPGA_IDP_CNTL_t idp_cntl;
	TITANGLB_FPGA_RGMII_CNTL_t rgmii_cntl;
	FPGA_RGMII_PORT_STATIC_CFG_t rgmii_port_static_cfg;

	/* hold the IDP reset */
	idp_cntl.wrd = CA_REG_READ(TITANGLB_FPGA_IDP_CNTL);
	idp_cntl.bf.idp_rst_n = 0;
	idp_cntl.bf.en_local_cpu_path = 1;
	CA_REG_WRITE(idp_cntl.wrd, TITANGLB_FPGA_IDP_CNTL);

	/* release the RGMII reset */
	rgmii_cntl.wrd = CA_REG_READ(TITANGLB_FPGA_RGMII_CNTL);
	rgmii_cntl.bf.rgmii_rst_n = 1;
	CA_REG_WRITE(rgmii_cntl.wrd, TITANGLB_FPGA_RGMII_CNTL);

	/* release the RxMAC reset */
	rgmii_port_static_cfg.wrd = CA_REG_READ(FPGA_RGMII_PORT_STATIC_CFG);
	rgmii_port_static_cfg.bf.mac_rst = 0;
	CA_REG_WRITE(rgmii_port_static_cfg.wrd, FPGA_RGMII_PORT_STATIC_CFG);

	/* release the IDP reset */
	idp_cntl.wrd = CA_REG_READ(TITANGLB_FPGA_IDP_CNTL);
	idp_cntl.bf.idp_rst_n = 1;
	idp_cntl.bf.en_local_cpu_path = 1;
	CA_REG_WRITE(idp_cntl.wrd, TITANGLB_FPGA_IDP_CNTL);
}

/***************************************************************************
 * netdev operations
 ***************************************************************************/

ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data)
{
	u16 *tmp_ptr;
	u16 vid;
	u16 tpid;
	struct ethhdr *p_ethhdr;

	p_ethhdr = (struct ethhdr *)skb_data;
	tpid = ntohs(p_ethhdr->h_proto);

	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		printk("%s: TPID of skb = 0x%x\n", __func__, tpid);
		printk("%s: ETH_P_8021Q=0x%x, ETH_P_8021AD=0x%x, ETH_P_QINQ1=0x%x, ETH_P_QINQ2=0x%x\n", __func__,
			ETH_P_8021Q, ETH_P_8021AD, ETH_P_QINQ1, ETH_P_QINQ2);
	}

	/* the vid to pass to AAL level should be 0 as untag */
	vid = 0;
	if (tpid == ETH_P_8021Q || tpid == ETH_P_8021AD || tpid == ETH_P_QINQ1 || tpid == ETH_P_QINQ2) {
		tmp_ptr = (u16 *)(skb_data + sizeof(struct ethhdr));
		vid = (ntohs(*tmp_ptr) & 0x0fff);

		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("%s: vid of skb = 0x%x\n", __func__, vid);
		}
	}
	return vid;
}

ca_uint16_t ca_ni_rx_get_protocol(ca_uint8_t *data, ca_uint8_t **p_protocol)
{
	ca_uint8_t *cp;
	ca_uint16_t protocol;

	cp = data + 12;
	protocol = (uint16_t) (*cp | ((ca_uint16_t)(*(cp+1)) << 8));
	if ( (protocol == ntohs(ETH_P_8021Q)) || (protocol == ntohs(ETH_P_8021AD)) || (protocol == ntohs(ETH_P_QINQ1)) ) {
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("Outer VLAN tag: %02x %02x %02x %02x\n", *cp, *(cp+1), *(cp+2), *(cp+3));
		}

		cp = cp + 4;
		protocol = (uint16_t) (*cp | (*(cp+1) << 8));
	}
	if ( (protocol == ntohs(ETH_P_8021Q)) || (protocol == ntohs(ETH_P_8021AD)) || (protocol == ntohs(ETH_P_QINQ1)) ) {
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			printk("Inner VLAN tag: %02x %02x %02x %02x\n", *cp, *(cp+1), *(cp+2), *(cp+3));
		}

		cp = cp + 4;
		protocol = (uint16_t) (*cp | (*(cp+1) << 8));
	}
	*p_protocol = cp;
	return protocol;
}

static int ca_ni_open(struct net_device *dev)
{
	int retval = 0;
	ca_eth_private_t *cep = netdev_priv(dev);

	if (!ni_initialized) {
		printk("%s: dev->name=%s, probe not finished yet!!\n", __func__, dev->name);
		return 0;
	}

#ifdef RX_POLLING
	setup_timer(&cep->rx_poll_timer, ca_ni_rx_poll_timer_cb, (unsigned long)dev);
	ca_ni_rx_poll_timer_cb((unsigned long) dev);
#else
	netif_carrier_off(dev);

	printk("%s: %s is up, irq %d.\n", __func__, dev->name, dev->irq);

	if (dev->irq >= 0) {
		/* disable interrupt */
		ni_rx_set_interrupt(dev, false);

		retval = request_irq(dev->irq, ca_ni_rx_interrupt, 0, dev->name, dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, dev->irq, retval);
			return retval;
		}

		/* enable interrupt */
		ni_rx_set_interrupt(dev, true);
	}

	/* after wake all queues, should turn on carrier for ready to transmit */
	netif_carrier_on(dev);

#endif

	mod_timer(&cep->flow_ctrl_timer, jiffies + msecs_to_jiffies(1000));

	if (cep->index == 1)
		allow_lan(true);

	return retval;
}

static int ca_ni_stop(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	if (cep->index == 1)
		allow_lan(false);

#ifdef RX_POLLING
	del_timer_sync(&cep->rx_poll_timer);
#else
	del_timer(&cep->flow_ctrl_timer);

	if (dev->irq >= 0) {
		ni_rx_set_interrupt(dev, false);
		free_irq(dev->irq, dev);
	}
#endif

	return 0;
}

static netdev_tx_t ca_ni_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int ret;

	/* padding for CRC and minimal frame size */
	if (skb->len <= ETH_ZLEN) {
		if (skb_padto(skb, ETH_ZLEN + ETH_FCS_LEN))
			return NETDEV_TX_OK;
	} else {
		if (skb_padto(skb, skb->len + ETH_FCS_LEN))
			return NETDEV_TX_OK;
	}

	ret = __idp_send_pkt(dev, skb->data, skb->len);

	if (ret == NETDEV_TX_OK)
		dev_kfree_skb_any(skb);

	return ret;
}

#ifdef RX_POLLING
static void ca_ni_rx_poll_timer_cb(unsigned long data)
{
	struct net_device *dev = (struct net_device *) data;
	ca_eth_private_t *cep = netdev_priv(dev);
	unsigned long flags;

	//printk("%s cep %p jiffies %ld\n", __func__, cep, jiffies);
	if (cep == NULL) {
		return;
	}
	spin_lock_irqsave(&cep->lock, flags);
	while (ca_ni_rx_rcv_packet(dev) == 0) {};
	spin_unlock_irqrestore(&cep->lock, flags);

	mod_timer(&cep->rx_poll_timer, jiffies + msecs_to_jiffies(1000));
}
#endif

static int ca_ni_change_mtu(struct net_device *dev, int new_mtu)
{
	/* validate new_mtu; if it is not validated, adjust it or return -EINVAL. */
	/* support jumbo frame */
	if ((new_mtu < ETH_ZLEN) || (new_mtu > MAX_FRAME_SIZE))
		return -EINVAL;

	pr_info("%s: Change MTU from %d to %d.\n", dev->name, dev->mtu, new_mtu);
	dev->mtu = new_mtu;

	return 0;
}

static int ca_ni_set_mac_address(struct net_device *dev, void *p)
{
	return 0;
}

static void ca_ni_tx_timeout(struct net_device *dev)
{

}

static int ca_ni_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return -EOPNOTSUPP;

}

static void ca_ni_set_rx_mode(struct net_device *dev)
{

}

static void ca_ni_change_rx_flags(struct net_device *dev, int flags)
{

}

static int ca_ni_set_features(struct net_device *dev, netdev_features_t features)
{
	return 0;
}

static struct net_device_stats *ca_ni_get_stats(struct net_device *dev)
{
	return &dev->stats;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void ca_ni_poll_controller(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	disable_irq(dev->irq);

	/// call interrupt handler to handle packets; ex. ni_rx_irq_handler(dev->irq, dev);

	// ca_ni_rx_interrupt(cep->port_cfg.irq, dev);

	enable_irq(dev->irq);
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

static const struct net_device_ops ca_ni_netdev_ops = {
	.ndo_open		= ca_ni_open,
	.ndo_stop		= ca_ni_stop,
	.ndo_start_xmit		= ca_ni_start_xmit,
	.ndo_tx_timeout		= ca_ni_tx_timeout,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= ca_ni_change_mtu,
	.ndo_set_mac_address	= ca_ni_set_mac_address,
	.ndo_do_ioctl		= ca_ni_ioctl,
	.ndo_set_rx_mode	= ca_ni_set_rx_mode,
	.ndo_change_rx_flags	= ca_ni_change_rx_flags,
	.ndo_set_features	= ca_ni_set_features,
	.ndo_get_stats		= ca_ni_get_stats,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ca_ni_poll_controller,
#endif
};

/***************************************************************************
 * platform driver registration
 ***************************************************************************/

#define CA_NI_SCFG_READ_INTF_PORTS(idx, data) aal_scfg_read(CFG_ID_NE_INTF_PORTS##idx, CFG_ID_NE_INTF_PORTS_LEN, (ca_uint8_t *)data)
#define CA_NI_IRQ_OF_PARSE_AND_MAP(np, idx) irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_FIFO##idx)
#define CA_NI_SCFG_READ_MAC_ADDR(idx, mac_addr) aal_scfg_read(CFG_ID_NE_MAC_ADDR##idx, CA_ETH_ADDR_LEN, mac_addr);

static int ca_ni_init_etherdev(struct platform_device *pdev)
{
	int i;
	struct net_device *dev;
	char mac_addr[CA_NI_MAX_INTF][6] = {{0,0x13,0x25,0,0,1}, {0,0x13,0x25,0,0,2}};
	ca_eth_private_t *cep;

	/* by default there is only 2 network interface */
	for (i = 0; i < ca_ni_num_intf; i++) {

		dev = alloc_etherdev(sizeof(ca_eth_private_t));
		if (dev == NULL) {
			pr_err("Unable to alloc new ethernet interface #%d.\n", i);
			return -ENOMEM;
		}

		/* Set the sysfs physical device reference for the network logical device */
		SET_NETDEV_DEV(dev, &pdev->dev);

		ni_info_data.dev[i] = dev;

		memcpy(&dev->dev_addr[0], mac_addr[i], 6);
		memcpy(dev->perm_addr, dev->dev_addr, 6);

		/* fill all of the items of dev */
		dev->netdev_ops = &ca_ni_netdev_ops;
		//ca_ni_set_ethtool_ops(dev);

		cep = netdev_priv(dev);
		cep->dev = dev;
		cep->pdev = pdev;
		cep->index = i;
		if (i == 0) {
			cep->port_id = 4;

			cep->reg_rx_data = (void __iomem *) FPGA_IDP_DOWN_CPU_RX_PKT_DATA1;
			cep->reg_rx_ctrl = (void __iomem *) FPGA_IDP_DOWN_CNTL;
			cep->reg_rx_pkt_size = (void __iomem *) FPGA_IDP_DOWN_CPURX_PKT;
			cep->reg_rx_int = (void __iomem *) FPGA_IDP_DOWN_INT; // interrupt at bit[3:0]
			cep->reg_tx_data = (void __iomem *) FPGA_IDP_UP_CPU_TX_PKT_DATA1;
			cep->reg_tx_ctrl = (void __iomem *) FPGA_IDP_UP_CPU_CNTL;
			cep->reg_tx_pkt_size = (void __iomem *) FPGA_IDP_UP_CPUTX_PKT;
		} else {
			cep->port_id = CA_UINT32_INVALID;

			cep->reg_rx_data = (void __iomem *) FPGA_IDP_MEM_LAN2CPU_PKT_DATA1;
			cep->reg_rx_ctrl = (void __iomem *) FPGA_IDP_UP_LAN2CPU_CNTL;
			cep->reg_rx_pkt_size = (void __iomem *) FPGA_IDP_UP_LAN2CPURX_PKT;
			cep->reg_rx_int = (void __iomem *) FPGA_IDP_UP_INT; // interrupt at bit[11:8]

			cep->reg_tx_data = (void __iomem *) FPGA_IDP_MEM_CPU2LAN_PKT_DATA1;
			cep->reg_tx_ctrl = (void __iomem *) FPGA_IDP_DOWN_CPU2LAN_CNTL;
			cep->reg_tx_pkt_size = (void __iomem *) FPGA_IDP_DOWN_CPU2LAN_PKT;
		}
		spin_lock_init(&cep->lock);
		//dev->features = NETIF_F_SG | NETIF_F_RXCSUM | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_UFO;
		dev->hw_features = dev->features;
#ifndef RX_POLLING
		dev->irq = platform_get_irq(pdev, i);
		if (dev->irq < 0)
			printk(KERN_ERR "%s: Fail to get IRQ from device tree index %d.\n", __func__, i);
		ni_info_data.dev[i]->irq = dev->irq;
#endif

		/* pure software flow control */
		cep->fps = 0;
		atomic_set(&cep->flow_cnt, 0);
		atomic_set(&cep->in_irq_handler, 0);
		setup_timer(&cep->flow_ctrl_timer, flow_ctrl_timer_cb, (unsigned long) dev);
	}

	platform_set_drvdata(pdev, &ni_info_data);

	return 0;
}

static int ca_ni_register_netdev(void)
{
	int i, err;

	for (i = 0; i < ca_ni_num_intf; i++) {
		err = register_netdev(ni_info_data.dev[i]);
		if (err) {
			printk(KERN_ERR "%s: Cannot register net device, aborting.\n", ni_info_data.dev[i]->name);
			free_netdev(ni_info_data.dev[i]);
			return err;
		}
	}
	return 0;
}

static int ca_ni_init_module_probe(struct platform_device *pdev)
{
	pr_notice("%s %s\n", DRV_NAME, DRV_VERSION);

	if (ni_initialized)
		return 0;

	ca_mut_init();

	memset(&ni_info_data, 0, sizeof(ni_info_data));

	titan_mdio_init();

	ca_ni_init_etherdev(pdev);

	ca_ni_reg_reset();

#ifdef CONFIG_PROC_FS
	ca_titan_proc_init();
	ca_ni_proc_init();
#endif

	/* register_netdev() should be the last one */
	ca_ni_register_netdev();

	ni_initialized = 1;

	return 0;
}

static int ca_ni_cleanup_module_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ca_ni_num_intf; i++) {
		unregister_netdev(ni_info_data.dev[i]);
		free_netdev(ni_info_data.dev[i]);
	}

#ifdef CONFIG_PROC_FS
	ca_titan_proc_exit();
	ca_ni_proc_exit();
#endif

	return 0;
}

#ifdef CONFIG_PM
static int ca_ni_resume(struct platform_device *pdev)
{
	return -ENOSYS;
}

static int ca_ni_suspend(struct platform_device *pdev, pm_message_t state)
{
	return -ENOSYS;
}
#else /* CONFIG_PM */
#define ca_ni_resume	NULL
#define ca_ni_suspend	NULL
#endif /* CONFIG_PM */

/* Match table for of_platform binding */
static const struct of_device_id cortina_ni_of_match[] __initconst = {
	{ .compatible = "cortina,ni-interface", },
	{}
};
MODULE_DEVICE_TABLE(of, cortina_ni_of_match);

static struct platform_driver ca_ni_platform_driver = {
	.probe		= ca_ni_init_module_probe,
	.remove		= ca_ni_cleanup_module_remove,
	.resume		= ca_ni_resume,
	.suspend	= ca_ni_suspend,
	.driver = {
		.name		= "ca-ni",
		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
		.of_match_table = cortina_ni_of_match,
	},
};

static int __init ca_ni_init_module(void)
{
	int ret;

	if (ca_ni_mgmt_mode)
		ca_ni_num_intf = 1;

	if ((ret = platform_driver_register(&ca_ni_platform_driver)) != 0)
		printk(KERN_ERR "%s: Fail to register device. (ret %d)\n", __func__, ret);

	return ret;
}

static void __exit ca_ni_cleanup_module(void)
{
	platform_driver_unregister(&ca_ni_platform_driver);
}

module_init(ca_ni_init_module);
module_exit(ca_ni_cleanup_module);

MODULE_AUTHOR("Raymond Tseng");
MODULE_DESCRIPTION(DRV_NAME);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

