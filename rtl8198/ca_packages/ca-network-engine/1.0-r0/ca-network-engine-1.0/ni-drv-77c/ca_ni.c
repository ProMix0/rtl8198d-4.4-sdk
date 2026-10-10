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
 * ca_ni.c: Cortina Access Network Interface driver
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

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
#include <linux/workqueue.h>

#if CONFIG_OF /* DT support */
#include <linux/of.h>
#include <linux/of_device.h>

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <linux/phy/phy.h>

#include <dt-bindings/soc/cortina_irqs.h>

#endif /* DT support */

#include "ca_aal.h"
#include "ca_aal_proc.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_ni_l2.h"
#include "aal_ni.h"
#include "aal_l3qm.h"
#include "aal_l3qm_cpu256.h"
#include "aal_l3fe.h"
#include "aal_lpm.h"
#include "pon.h"
#include "aal_fbm.h"
#include "aal_phy.h"
#include "eth_port.h"
#include "port.h"
#include "sys.h"
#include "scfg.h"
#include "ca_ni.h"
#include "ca_phy.h"
#include "ca_virt_ni.h"
#include "ca_ni_rx.h"
#include "ca_ni_tx.h"
#include "ca_ethtool.h"
#include "ca_ni_proc.h"
#include "ca_mut.h"
#include "ca_api_proc.h"

#ifdef CONFIG_NE_PTP_CLOCK
#include "ca_ni_ptp.h"
#endif

#if defined(CONFIG_NE_CPU256)
#include "aal_l3qm_cpu256.h"
#include "ca_ni_cpu256.h"
#endif

#if defined(CONFIG_NE_TX_LLID_MAP)
#include "ca_ni_dump.h"
#include "pkt_tx.h"
#endif

#ifdef CONFIG_NE_L2FP
#include "ca_l2fp.h"
#endif

#ifdef CONFIG_CORTINA_CPU_TAG
#include "ca_ni_cpu_tag.h"
#endif

#if 1//defined(CONFIG_LUNA_G3_SERIES)
#include "ca-ne-rtk/rtk_func.h"
#include "ca-ne-rtk/ca_ext.h"
#endif

#include "board_fpga.h"

#ifdef CONFIG_RTK_PERF_EVENTS
#include <soc/cortina/rtk_pmu.h>
#endif

#ifdef CONFIG_NE_KERNEL_HOOK
#include "ca_kernel_hook.h"
#endif


MODULE_AUTHOR("Aaron Tseng, Raymond Tseng");
MODULE_DESCRIPTION(DRV_NAME);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

static int bypass_fe = 0;
module_param(bypass_fe, int, 0444);
MODULE_PARM_DESC(bypass_fe, "All packets bypass Forwarding Engine");

#define CA_NI_DRV_STARUP_CONFIG_MY_MAC_INDEX           0
#define CA_NI_DRV_STARUP_CONFIG_CPU_PORT_ID            1
#define CA_NI_DRV_STARUP_CONFIG_EGRESS_CPU_PORT        2

/* store the configure data from startup configuration for NI driver */
typedef struct ca_ni_drv_startup_config {
	ca_uint32_t     enable;
	ca_uint32_t     my_mac_index;
	ca_uint32_t     egress_cpu_port_id;
	ca_uint32_t     cpu_port_id[CA_NI_TOTAL_CPU_PORT];
	ca_uint8_t      num_cpu_port;
	ca_uint8_t      dev_name[IFNAMSIZ];
} ca_ni_drv_startup_config_t;
static ca_ni_drv_startup_config_t driver_startup_config[CA_NI_TOTAL_CPU_PORT];

/* debug flags */
u32 ca_ni_debug = 0;

static const u32 default_msg = NETIF_MSG_DRV | NETIF_MSG_PROBE
	| NETIF_MSG_LINK | NETIF_MSG_TIMER | NETIF_MSG_IFDOWN
	| NETIF_MSG_IFUP | NETIF_MSG_RX_ERR | NETIF_MSG_TX_ERR;

static int debug = -1;  /* defaults above */

static ca_g3_init_config_t ca_ni_init_config;

ni_info_t ni_info_data;

spinlock_t  ca_ni_rx_refill_lock;

int ca_ni_napi_budget = 16;
int ni_use_fbm = 0;
int ni_initialized=0;
int ca_ni_num_intf = 2;
ca_uint8_t ca_ni_use_qm_eq_refill_int = 0;
ca_uint8_t ca_ni_cpu64_pool0_eq_id = 0;
ca_uint8_t ca_ni_cpu64_pool1_eq_id = 0;
ca_uint8_t ca_ni_cpu64_pool0_fbm_pool_id=0, ca_ni_cpu64_pool1_fbm_pool_id=0;
ca_uint8_t ca_ni_cpu64_use_fbm=0;
ca_uint32_t ca_ni_cpu64_pool0_buf_count=0, ca_ni_cpu64_pool1_buf_count=0;

ca_uint8_t ca_ni_use_build_skb = 0;
ca_uint8_t ca_ni_tx_timer_enable = 0;

/* in CPU64 REFILL interrupt handler routine use tasklet to refill EQ pools */
ca_uint8_t ca_ni_use_qm_eq_refill_tasklet = 0;

#if defined(CONFIG_NE_TX_LLID_MAP)
ca_uint8_t ca_ni_oam_mode = 0;
#endif

void __iomem *ca_ne_sram_base = NULL;
static void __iomem *ca_ne_ddr_coherent_base = NULL;
static void __iomem *ca_ne_ddr_noncache_base = NULL;
static resource_size_t ca_ne_sram_phy_base = 0;
int ca_ne_sram_size = 0;
static resource_size_t ca_ne_ddr_noncache_phy_base = 0;
static resource_size_t ca_ne_ddr_coherent_phy_base = 0;
static ca_uint_t ca_ne_cpu_pool0_start_addr, ca_ne_cpu_pool1_start_addr;

#ifdef CONFIG_CORTINA_MUT
extern struct kmem_cache *cache_16;	/* cache for 16-byte objects */
#endif

extern void *ca_venus_pon_reg_iobase;
extern void *ca_venus_pon_glb_reg_iobase;
#if 1//def CONFIG_LUNA_G3_SERIES
extern void *ca_venus_peri_reg_iobase;
#endif

static int ca_ne_reg_init(struct platform_device *pdev);
void aal_ni_dma_lso_dump_registers(void);
void aal_l3qm_dump_registers(void);
#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
int  ca_ni_netlink_rx_init(void);
void  ca_ni_netlink_rx_exit(void);
#endif
void ca_ni_init_fbm(void);

#if defined(CONFIG_RTL_MULTI_LAN_DEV)

static int ca_ni_is_multi_lan_device(struct net_device *dev);

#define TOTAL_CA_NI_MULTI_LAN_DEV_NUM	((sizeof(ca_ni_multi_lan_dev_table))/(sizeof(struct ca_ni_multi_lan_dev_table_entry)))

enum RTL8686_IFFLAG{
	CA_NI_ELAN = 0,
	CA_NI_WAN = 1,
	CA_NI_WLAN = 2,
	CA_NI_SVAP = 3,		// Luke: slave VAP interface, should not be sent by bridge
};

struct ca_ni_multi_lan_dev_table_entry ca_ni_multi_lan_dev_table[MAX_CA_NI_MULTI_LAN_DEV_NUM] = {
	//ifname, ifflag, vid, phyPort, dev_instant
	{"eth0.2", CA_NI_ELAN, 0, LAN_PORT1, 1, NULL},
	{"eth0.3", CA_NI_ELAN, 0, LAN_PORT2, 1, NULL},
	{"eth0.4", CA_NI_ELAN, 0, LAN_PORT3, 1, NULL},
	{"eth0.5", CA_NI_ELAN, 0, LAN_PORT4, 1, NULL},
	{"eth0.6", CA_NI_ELAN, 0, LAN_PORT5, 1, NULL},
#if defined(CONFIG_RTK_PE_FC_SUPPORT)
	{"eth0.16", CA_NI_ELAN, 0, PE_PORT, 1, NULL},
#endif
	{"nas0", CA_NI_WAN, 0, WAN_PORT1, 0, NULL},
#if defined(CONFIG_RG_G3_SERIES_DEVELOPMENT)
	{"wlan0", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan0-vap0", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan0-vap1", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan0-vap2", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan0-vap3", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan0-wds0", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1-vap0", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1-vap1", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1-vap2", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1-vap3", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
	{"wlan1-wds0", CA_NI_WLAN, 0, WAN_PORT1, 0, NULL},
#endif
};

extern int aal_l3fe_keep_lspid_unchange_set(ca_device_id_t device_id, ca_uint8_t *config);
static int ca_ni_change_dev_tx_port_mask(int port_num, char* name, int index)
{
	ca_eth_private_t *cep = netdev_priv(ca_ni_multi_lan_dev_table[index].dev_instant);
	cep->port_cfg.tx_ldpid = port_num;
	printk("%s -> port %d\n", 
		ca_ni_multi_lan_dev_table[index].dev_instant->name, cep->port_cfg.tx_ldpid);
	return 0;
}

int ca_ni_change_dev_port_mapping(ca_eth_private_t *cep, int port_num, char *name)
{
	unsigned int totalDev = TOTAL_CA_NI_MULTI_LAN_DEV_NUM;
	char* dev_name;
	struct net_device *dev = NULL;
	int i;
	
	for(i=0;i<totalDev;i++){
		if(ca_ni_multi_lan_dev_table[i].dev_instant)
		{
			dev_name = ca_ni_multi_lan_dev_table[i].dev_instant->name;
			dev = ca_ni_multi_lan_dev_table[i].dev_instant;
		}
		else
		{
			printk("no dev_instant, strange.......\n");
			dev_name = ca_ni_multi_lan_dev_table[i].ifname;
		}
		if(!strcmp(dev_name, name)){				
			cep->port2dev[port_num] = ca_ni_multi_lan_dev_table[i].dev_instant;
			ca_ni_change_dev_tx_port_mask(port_num, name, i);
			break;
		}
	}
	
	if(i == totalDev){
		printk("can't find dev %s\n", name);
		return -1;
	}
	
	return 0;
}
#endif


static void __ca_ni_qm_pool0_refill_tasklet_action(unsigned long t);
DECLARE_TASKLET(ca_ni_qm_pool0_refill_tasklet, __ca_ni_qm_pool0_refill_tasklet_action, 0);

static void __ca_ni_qm_pool1_refill_tasklet_action(unsigned long t);
DECLARE_TASKLET(ca_ni_qm_pool1_refill_tasklet, __ca_ni_qm_pool1_refill_tasklet_action, 0);

static struct workqueue_struct *ca_ni_fill_eq_pool_wq;
static void ca_ni_fill_eq_pool_wq_proc(struct work_struct *wq);
static void ca_ni_enable_qm_interrupts(int enable);
static DECLARE_WORK(ca_ni_fill_eq_pool_work, ca_ni_fill_eq_pool_wq_proc);

/* export functions for kernel hook */
ca_status_t ca_ni_intf_get_ports(struct net_device *dev, ca_uint8_t *num_ports, int *ports, ca_uint8_t *is_wan_intf)
{
	int i;
	struct net_device *tmp_dev;
	ca_eth_private_t *cep = netdev_priv(dev);

	for (i = 0; i < ca_ni_num_intf; i++) {
		tmp_dev = ni_info_data.dev[i];

		if (tmp_dev == dev)
			break;;
	}
	if (i == ca_ni_num_intf) {
		printk("%s: dev->name=%s not found!!\n", __func__, dev->name);
		return CA_E_ERROR;
	}

	/* the first interface is allocated for WAN interface */
	if (ca_ni_debug & NI_DBG_NI) {
		printk("%s: dev->name=%s, i=%d\n", __func__, dev->name, i);
	}
	if (i == 0)
		*is_wan_intf = 1;
	else {
		*is_wan_intf = 0;
	}

	if (cep->port_cfg.num_phy_port > *num_ports) {
		printk("%s: num_phy_port=%d bigger than num_ports=%d\n", __func__, cep->port_cfg.num_phy_port, *num_ports);
		return CA_E_PARAM;
	}
	*num_ports = cep->port_cfg.num_phy_port;
	for (i = 0; i < cep->port_cfg.num_phy_port; i++) {
		if (ca_ni_debug & NI_DBG_NI) {
			printk("%s: port_id[%d]=%d\n", __func__, i, cep->port_cfg.port_id[i]);
		}
		ports[i] = cep->port_cfg.port_id[i];
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_ni_intf_get_ports);

/* export for external modules */
ca_uint32_t ca_ni_get_wan_port_id(void)
{
	return (ca_ni_init_config.ca_ne_port_id);
}
EXPORT_SYMBOL(ca_ni_get_wan_port_id);

void ca_ni_init_reserved_mem(void)
{
	aal_l3qm_init_startup_config();
	aal_l3qm_init_reserved_mem(ca_ne_sram_base, ca_ne_sram_size, (void *)ca_ne_sram_phy_base, (void *)ca_ne_ddr_coherent_phy_base, ca_ne_ddr_noncache_base, (void *)ca_ne_ddr_noncache_phy_base);
}


/***************************************************************************
 * netdev operations
 ***************************************************************************/
static void __ca_ni_qm_pool0_refill_tasklet_action(unsigned long t)
{
	int ret = 0;
        int bid_cnt;
        int eqid;
        int fbm_pool_id;
	struct net_device *dev = ni_info_data.dev[0];

	eqid = ca_ni_cpu64_pool0_eq_id;
	fbm_pool_id = ca_ni_cpu64_pool0_fbm_pool_id;
	if (ca_ni_cpu64_use_fbm)
		bid_cnt = ca_ni_cpu64_pool0_buf_count - aal_fbm_get_out_stnd_count(FBM_CPU_ARM0, fbm_pool_id);
	else
		bid_cnt = aal_l3qm_get_inactive_bid_cntr(eqid);

        if (bid_cnt > 0)
                ret = ca_ni_refill_eq_buf_pool(dev, 1, eqid, bid_cnt);

	if (ret == 0)
		aal_l3qm_enable_refill_threshold_interrupt(eqid, 1);
}

static void __ca_ni_qm_pool1_refill_tasklet_action(unsigned long t)
{
	int ret = 0;
	int bid_cnt;
	int eqid;
	int fbm_pool_id;
	struct net_device *dev = ni_info_data.dev[0];

	eqid = ca_ni_cpu64_pool1_eq_id;
	fbm_pool_id = ca_ni_cpu64_pool1_fbm_pool_id;
	if (ca_ni_cpu64_use_fbm)
		bid_cnt = ca_ni_cpu64_pool1_buf_count - aal_fbm_get_out_stnd_count(FBM_CPU_ARM0, fbm_pool_id);
	else
		bid_cnt = aal_l3qm_get_inactive_bid_cntr(eqid);

	if (bid_cnt > 0)
		ret = ca_ni_refill_eq_buf_pool(dev, 1, eqid, bid_cnt);

	if (ret == 0)
		aal_l3qm_enable_refill_threshold_interrupt(eqid, 1);
}

irqreturn_t ca_ni_qm_refill_interrupt(int irq, void *dev_instance)
{
	int ret = 0;
	u16 eqid = 0;
	ca_uint8_t fbm_pool_id;
	u32 bid_cnt;
	ca_uint32_t cpu64_pool_buf_count;
	struct net_device *dev = (struct net_device *)dev_instance;

	if (irq == ni_info_data.refill_req_8) {
		eqid = ca_ni_cpu64_pool0_eq_id;
		fbm_pool_id = ca_ni_cpu64_pool0_fbm_pool_id;
		cpu64_pool_buf_count = ca_ni_cpu64_pool0_buf_count;
	}
	else if (irq == ni_info_data.refill_req_9) {
		eqid = ca_ni_cpu64_pool1_eq_id;
		fbm_pool_id = ca_ni_cpu64_pool1_fbm_pool_id;
		cpu64_pool_buf_count = ca_ni_cpu64_pool1_buf_count;
	}
	else {
		printk("%s: Unknown irq=%d!!!\n", __func__, irq);
	}

	aal_l3qm_enable_refill_threshold_interrupt(eqid, 0);

	if (ca_ni_use_qm_eq_refill_tasklet) {
		if (eqid == ca_ni_cpu64_pool0_eq_id)
			tasklet_schedule(&ca_ni_qm_pool0_refill_tasklet);
		else
			tasklet_schedule(&ca_ni_qm_pool1_refill_tasklet);
	}
	else {
		if (ca_ni_cpu64_use_fbm)
			bid_cnt = cpu64_pool_buf_count - aal_fbm_get_out_stnd_count(FBM_CPU_ARM0, fbm_pool_id);
		else
			bid_cnt = aal_l3qm_get_inactive_bid_cntr(eqid);

		if (bid_cnt > 0)
			ret = ca_ni_refill_eq_buf_pool(dev, 0, eqid, bid_cnt);

		if (ret == 0) {
			aal_l3qm_enable_refill_threshold_interrupt(eqid, 1);
		}
		else {
			printk("%s: ca_ni_refill_eq_buf_pool(eqid=%d) ret != 0\n", __func__, eqid);
		}
	}

	return IRQ_HANDLED;
}

static int ca_ni_get_cpu_port_from_irq(ca_eth_private_t *cep, int irq)
{
	int i;
	ca_port_cfg_info_t *port_cfg = &(cep->port_cfg);

	for (i = 0; i < cep->num_cpu_port; i++) {
		if (port_cfg->irq[i] == irq) {
			if (ca_ni_debug & NI_DBG_IRQ) {
				printk("%s: irq=%d, cep->cpu_port[i]=%d\n", __func__, irq, cep->cpu_port[i]);
			}
			return cep->cpu_port[i];
		}
	}
	if (ca_ni_debug & NI_DBG_IRQ) {
		printk("%s: no CPU port found with irq %d!!!\n", __func__, irq);
	}
	return 0;
}

static struct napi_struct *ca_ni_get_napi_from_cpu_port(ca_eth_private_t *cep, ca_uint8_t cpu_port)
{
	int i;

	for (i = 0; i < cep->num_cpu_port; i++) {
		if (ca_ni_debug & NI_DBG_IRQ) {
			printk("%s: cep->cpu_port[%d]=%d, cpu_port=%d\n", __func__, i, cep->cpu_port[i], cpu_port);
		}
		if (cep->cpu_port[i] == cpu_port) {
			if (ca_ni_debug & NI_DBG_IRQ) {
				printk("%s: cpu_port=%d, napi=0x%p\n", __func__, cpu_port, &(cep->napi[i]));
			}
			return &(cep->napi[i]);
		}
	}
	printk("%s: no napi found with cpu_port %d!!\n", __func__, cpu_port);
	return NULL;
}

/*
 * Handles RX interrupts.
 */
irqreturn_t ca_ni_rx_interrupt(int irq, void *dev_instance)
{
	int i = 0;
	ca_eth_private_t *cep = netdev_priv((struct net_device *) dev_instance);
	unsigned long flags;
	int cpu_port;
	struct napi_struct *napi;

	spin_lock_irqsave(&cep->lock, flags);

	/* get the real cpu port number from irq number */
	cpu_port = ca_ni_get_cpu_port_from_irq(cep, irq);

	aal_ni_enable_rx_interrupt(cpu_port, 0);

	if (aal_ni_has_work(i)) {
		cep = netdev_priv((struct net_device *) dev_instance);
		napi = ca_ni_get_napi_from_cpu_port(cep, cpu_port);
		if (napi != NULL) {
			napi_schedule(napi);
		}
	}
	else {
		aal_ni_enable_rx_interrupt(cpu_port, 1);
	}

	spin_unlock_irqrestore(&cep->lock, flags);

	return IRQ_HANDLED;
}

static void ca_ni_set_flow_control(ca_port_cfg_info_t *port_cfg)
{
#if 0 //77C not using CA API
	int i;
	u8 pause_rx = 0, pause_tx = 0;
	ca_port_id_t port_id;

	if (port_cfg->flowctrl & CA_NI_RX_FLOW_CTRL)
		pause_rx = 1;

	if (port_cfg->flowctrl & CA_NI_TX_FLOW_CTRL)
		pause_tx = 1;

	for (i = 0; i < port_cfg->num_phy_port; i++) {
		port_id = ((ca_port_id_t)CA_PORT_TYPE_GPON << 8) | port_cfg->port_id[i];

		/* call Cortina port API */
		ca_eth_port_pause_set(0, port_id, 0, pause_rx, pause_tx);
	}
#endif
}

int ca_ni_init_l3qm(void)
{
	int i;

	/* make sure L3QM module is ready */
	aal_l3qm_check_init_done();

	/* disable RX of L3QM, because default it's enabled */
	aal_l3qm_enable_rx(0);

	aal_l3qm_init_empty_buffer();

	/* disable all of EPP interrupts */
	for (i = 0; i < 8; i++) {
		aal_l3qm_enable_cpu_epp_interrupt(i, 0);
	}

	aal_l3qm_init_cpu_epp();

	/* enable all of VoQs */
	aal_l3qm_init_voq();

	/* enable TX of L3QM */
	aal_l3qm_enable_tx(1);

	/* enable RX of L3QM */
	aal_l3qm_enable_rx(1);

	aal_l3qm_init_jumbo_buf();
	aal_l3qm_init_upper_ldpid_map();
	//aal_l3qm_axi_reo_rd_init(0);
	//aal_l3qm_axi_reo_wr_init(0);

	return 0;
}

static void ca_ni_enable_ports(struct net_device *dev, ca_boolean_t enable)
{
#if 0 //77C not using CA API
	int i;
	ca_eth_private_t *cep = netdev_priv(dev);

	for (i = 0; i < cep->port_cfg.num_phy_port; i++) {
		ca_port_enable_set(0, ((ca_port_id_t)CA_PORT_TYPE_ETHERNET << CA_PORT_TYPE_SHIFT) |  cep->port_cfg.port_id[i], enable);
	}
#endif
}

static int ca_ni_request_qm_refill_irq(struct net_device *dev)
{
	int retval;
	static int refill_irq = 0;

	if (refill_irq == 0 && ca_ni_use_qm_eq_refill_int) {

		retval = request_irq(ni_info_data.refill_req_8, ca_ni_qm_refill_interrupt, 0, "QM EQ8 REFILL", dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.refill_req_8, retval);
			return retval;
		}

		retval = request_irq(ni_info_data.refill_req_9, ca_ni_qm_refill_interrupt, 0, "QM EQ9 REFILL", dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ni_info_data.refill_req_9, retval);
			return retval;
		}
		refill_irq = 1;
	}
	return 0;
}

static int ca_ni_open(struct net_device *dev)
{
	int i, retval;
	ca_eth_private_t *cep = netdev_priv(dev);

	if (!ni_initialized) {
		printk("%s: dev->name=%s, probe not finished yet!!\n", __func__, dev->name);
		return 0;
	}

	/* should dirable RX interrupt first to avoid fake RX interrrupt comes in */
	for (i = 0; i < cep->num_cpu_port; i++) {
		if (ca_ni_debug & NI_DBG_NI) {
			printk("%s: cep->cpu_port[%d]=%d\n", __func__, i, cep->cpu_port[i]);
		}
		aal_ni_enable_rx_interrupt(cep->cpu_port[i], 0);
	}

#ifdef CONFIG_CA_NE_VIRTUAL_NI
	ca_eth_private_t *virt_cep = netdev_priv(dev);

	if (virt_cep->dev != dev)        /* virtual device now */
		return ca_ni_virt_ni_open(virt_cep->port_cfg.port_id, dev);
#endif

	netif_carrier_off(dev);

	for (i = 0; i < cep->num_cpu_port; i++) {
		/* request irq for this physical port */
		if (ca_ni_debug & NI_DBG_NI) {
			printk("%s: cep->port_cfg.irq[%d]=%d\n", __func__, i, cep->port_cfg.irq[i]);
		}

		retval = request_irq(cep->port_cfg.irq[i], ca_ni_rx_interrupt, 0, dev->name, dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, cep->port_cfg.irq[i], retval);
			return retval;
		}
	}

	/* reuqest QM REFILL INTERRUPTs */
	ca_ni_request_qm_refill_irq(dev);

        /* should enable EPP interrupt after request_irq() done */
        for (i = 0; i < cep->num_cpu_port; i++) {
                aal_l3qm_enable_cpu_epp_interrupt(cep->cpu_port[i], 1);
        }

	/* use callback routine to free the transmitted packet if start xmit not be called again */
	if (ca_ni_tx_timer_enable)
		setup_timer(&cep->tx_completion_timer, ca_ni_tx_completion_timer_cb, (unsigned long)dev);

	cep->status = 1;
	cep->ni_driver_state = 1;
	cep->tx_completion_queues = 0;

	/* turn on TX/RX */
	ca_ni_enable_ports(dev, 1);

	ca_ni_set_flow_control(&(cep->port_cfg));

	/* napi should be enabled before RX interrupt enabled */
	for (i = 0; i < cep->num_cpu_port; i++) {
		napi_enable(&cep->napi[i]);
	}

	/* Enable Rx interrupts at this point */
	for (i = 0; i < cep->num_cpu_port; i++) {
		aal_ni_enable_rx_interrupt(cep->cpu_port[i], 1);
	}

	/* call linux kernel routine to wake up all TX queues */
	netif_tx_wake_all_queues(dev);

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

	/* enable QM TX function for cpu port */
	for (i = 0; i < cep->num_cpu_port; i++) {
		aal_l3qm_enable_tx_cpu(cep->cpu_port[i], 1);
	}

	return 0;
}

static int ca_ni_stop(struct net_device *dev)
{
	int i;
	ca_eth_private_t *cep = netdev_priv(dev);

	/* disable QM TX function for cpu port */
	for (i = 0; i < cep->num_cpu_port; i++) {
		aal_l3qm_enable_tx_cpu(cep->cpu_port[i], 0);
	}

	/* disable all of ports connected to this interface */
	ca_ni_enable_ports(dev, 0);

	for (i = 0; i < cep->num_cpu_port; i++) {
		aal_ni_enable_rx_interrupt(cep->cpu_port[i], 0);
	}

	/* stop TX queues */
	netif_tx_disable(dev);

	/* disable NAPI */
	for (i = 0; i < cep->num_cpu_port; i++) {
		napi_disable(&cep->napi[i]);
	}

	/* release irq */
	for (i = 0; i < cep->num_cpu_port; i++) {
		free_irq(cep->port_cfg.irq[i], dev);
	}

	cep->status = 0;

	return 0;
}

void ca_ni_set_rx_to_block_mode(void)
{
	int i, j;
	struct net_device *dev;
	ca_eth_private_t *cep;

	printk("%s: ca_ni_num_intf=%d\n", __func__, ca_ni_num_intf);
	for (i = 0; i < ca_ni_num_intf; i++) {
		dev = ni_info_data.dev[i];
		cep = netdev_priv(dev);

		/* disable NAPI */
		for (i = 0; i < cep->num_cpu_port; i++) {
			napi_disable(&cep->napi[i]);
		}

		for (j = 0; j < cep->num_cpu_port; j++) {
			/* release irq */
			free_irq(cep->port_cfg.irq[j], dev);
		}
	}
}

static netdev_tx_t ca_ni_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int lso_txq_index;
	ca_eth_private_t *cep;
	ca_ni_tx_config_t tx_config;
	netdev_tx_t ret;

#if defined(CONFIG_NE_TX_LLID_MAP)
        ca_uint32_t llid;
        ca_mac_addr_t source_mac;
#endif

#ifdef CONFIG_RTK_PERF_EVENTS
	start_perf_counter(0x1); // save to idx 8
#endif

	cep = netdev_priv(dev);

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	if(ca_ni_is_multi_lan_device(dev)) {
		dev = cep->dev;
	}
#endif

	ca_skb_in(skb, true);

#ifdef CONFIG_NET_SCH_MULTIQ
	lso_txq_index = skb_get_queue_mapping(skb);
#else
	lso_txq_index = skb->priority;
#endif

	memset(&tx_config, 0, sizeof(ca_ni_tx_config_t));
	tx_config.core_config.bf.txq_index = lso_txq_index;
	tx_config.core_config.bf.ldpid = cep->port_cfg.tx_ldpid;

#if defined(CONFIG_NE_TX_LLID_MAP)
	/* if OAM mode is DPOE then should get the llid from source IP */
	if (ca_ni_oam_mode == 1) {
		memcpy(&source_mac, skb->data + sizeof(ca_mac_addr_t), sizeof(ca_mac_addr_t));
		if (ca_tx_get_llid(0, source_mac, &llid) == CA_E_OK) {
			tx_config.core_config.bf.flow_id_set = 1;
			tx_config.flow_id = llid;
		}
	}
#endif

	ret = __ca_ni_start_xmit(skb, dev, &tx_config);

#ifdef CONFIG_RTK_PERF_EVENTS
	stop_perf_counter(8, 0x1);
#endif

	return ret;
}

#if defined(CONFIG_RTK_NIC_TX_HOOK)
extern int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, struct net_device *dev);
static netdev_tx_t ca_ni_start_xmit_check(struct sk_buff *skb, struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev);
#ifdef CONFIG_RTK_HOST_SPEEDUP
	struct tcphdr *tcph;
	unsigned char *pData = skb->data;
	struct iphdr *iph;			

	if (isHostSpeedUpEnable())
	{
		pData += 12;
		if (ntohs(*(unsigned short *)pData) == 0x8100)
				pData += 4;
		if (((ntohs(*(unsigned short *)pData) == 0x8864) && (ntohs(*(unsigned short *)(pData+8)) == 0x0021)) ||
				(ntohs(*(unsigned short *)pData) == 0x0800))
		{
			if (ntohs(*(unsigned short *)pData) == 0x0800)
				iph = (struct iphdr *)(pData + 2);
			else
				iph = (struct iphdr *)(pData + 10);

			if (IPPROTO_TCP == iph->protocol)
			{
				unsigned long flags;
				void *pHostTrack;
				tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
				read_lock_irqsave(&speedup_lock,flags);
				if ((pHostTrack = shouldSpeedUp(1, iph->daddr, iph->saddr, tcph->dest, tcph->source)) != NULL)
				{
					int ret;
					//printk("[%s %d]ack_no:%x\n",__func__,__LINE__,ntohl(tcph->ack_seq));
					ret = speedtest_stream_shortcut_tx_learn(pHostTrack, iph, tcph, skb);
					read_unlock_irqrestore(&speedup_lock,flags);
					//ca_ni_start_xmit(skb, dev);
					cep->txfunc(skb, dev);
					if (ret)
					{
						start_xmit_upload_data();
					}
					return 0;
				}
				read_unlock_irqrestore(&speedup_lock,flags);
			}
		}
	}
#endif//endof CONFIG_RTK_HOST_SPEEDUP

#if 0//def	CONFIG_RTK_SMB_SPEEDUP
	int smbscflag = 0;
	smbscflag = getSMBflag();
	if ( smbscflag ){
		pData += 12;
		if ((*(unsigned short *)pData == htons(0x8100) ) || 
			(*(unsigned short *)pData == htons(0x9100) )){
			//return rtk_rg_fwdEngine_xmit (skb,dev);
			cep->txfunc(skb, dev);
			return NETDEV_TX_OK;
		}
		if (((*(unsigned short *)pData == htons(0x8864)) && (*(unsigned short *)(pData+8) == htons(0x0021))) ||
				(*(unsigned short *)pData == htons(0x0800)))
		{
			
			if (*(unsigned short *)pData == htons(0x0800))
				iph = (struct iphdr *)(pData + 2);
			else
				iph = (struct iphdr *)(pData + 10);		
			tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);				
			if (checkSMBLocalIPPort(iph->saddr, tcph->source))
			{
				cep->txfunc(skb, dev); //re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
				return NETDEV_TX_OK;
			}					
		}
	}
#endif
	if (!cep->txfunc(skb, dev))
	{
		return NETDEV_TX_OK;
	}
	else
	{
		return NETDEV_TX_BUSY;
	}
}
netdev_tx_t ca_ni_start_xmit_native(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config)
{
	netdev_tx_t ret = NETDEV_TX_OK;

	if((ret = __ca_ni_start_xmit(skb, NULL, tx_config)) != NETDEV_TX_OK)
		ca_dev_kfree_skb_any(skb);

	return ret;
}
EXPORT_SYMBOL(ca_ni_start_xmit_native);
#endif


static int ca_ni_change_mtu(struct net_device *dev, int new_mtu)
{
	ca_eth_private_t *cep = netdev_priv(dev);

	/* validate new_mtu; if it is not validated, adjust it or return -EINVAL. */
	/* support jumbo frame */
	if (new_mtu < ETH_ZLEN || new_mtu > CA_NI_MAX_FRAME_SIZE)
		return -EINVAL;

	pr_info("%s: Change MTU from %d to %d.\n", dev->name, dev->mtu, new_mtu);
	dev->mtu = new_mtu;

	if (!netif_running(dev))
		return 0;

	/* restart TX queue */
	if (cep->phydev != NULL) {
		netif_tx_disable(dev);
		phy_stop(cep->phydev);
		phy_start(cep->phydev);
		netif_carrier_off(dev);
		netif_tx_wake_all_queues(dev);
	}

	return 0;
}

static int ca_ni_set_mac_address(struct net_device *dev, void *p)
{
	int i;
	struct sockaddr *addr = p;
	ca_eth_private_t *cep = netdev_priv(dev);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	for (i = 0; i < cep->port_cfg.num_phy_port; i++) {
		printk("%s: i=%d, cep->port_cfg.port_id[i]=%d\n", __func__, i, cep->port_cfg.port_id[i]);
		aal_ni_set_mac_address(cep->port_cfg.port_id[i], dev->dev_addr);
	}
	return 0;
}

static void ca_ni_tx_timeout(struct net_device *dev)
{
	int i;
	struct net_device *tmp_dev;
	ca_eth_private_t *cep = netdev_priv(dev);

	printk("%s: dev->name=%s\n", __func__, dev->name);

	for (i = 0; i < ca_ni_num_intf; i++) {
		tmp_dev = ni_info_data.dev[i];

		if (tmp_dev == NULL)
			continue;

		cep = netdev_priv(tmp_dev);
		netif_tx_stop_all_queues(tmp_dev);
	}

	printk("%s::schedule reset task\n", __func__);
	schedule_work(&ni_info_data.reset_task);
}

static int ca_ni_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
#if defined(CONFIG_RTK_NIC_RX_HOOK) || defined(CONFIG_RTK_NIC_TX_HOOK)
	struct eth_arg arg;
#endif
	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return ca_phy_ioctl(dev, ifr, cmd);
#if defined(CONFIG_RTK_NIC_RX_HOOK) || defined(CONFIG_RTK_NIC_TX_HOOK)
	case SIOCETHTEST:
		if (copy_from_user(&arg, ifr->ifr_data, sizeof(struct eth_arg)))
			return -EFAULT;
		eth_ctl(&arg);
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if (copy_to_user(ifr->ifr_data, &arg, sizeof(struct eth_arg)))
			return -EFAULT;
#endif
		//eth_ctl((struct eth_arg *)ifr->ifr_data);
		return 0;
#endif

	default:
		return -EOPNOTSUPP;
	}
}

static void ca_ni_set_rx_mode(struct net_device *dev)
{
}

static void ca_ni_change_rx_flags(struct net_device *dev, int flags)
{
}

#define L4RxChkSum      (1 << 8)
static int ca_ni_set_features(struct net_device *dev, netdev_features_t features)
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
		aal_l3fe_l4_chksum(cep->rx_checksum);

		spin_unlock_irqrestore(&cep->lock, flags);
	}
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

	for (i = 0; i < cep->num_cpu_port; i++) {
		ca_ni_rx_interrupt(cep->port_cfg.irq[i], dev);
	}

	enable_irq(dev->irq);
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

static const struct net_device_ops ca_ni_netdev_ops = {
	.ndo_open		= ca_ni_open,
	.ndo_stop		= ca_ni_stop,
#if defined(CONFIG_RTK_NIC_TX_HOOK)
	.ndo_start_xmit		= ca_ni_start_xmit_check,
#else
	.ndo_start_xmit		= ca_ni_start_xmit,
#endif
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

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
static int ca_ni_multi_lan_dev_open(struct net_device *dev)
{
	ca_eth_private_t *cep = netdev_priv(dev), *rootCep;
#if defined(CONFIG_NE_MANAGEMENT_MODE)
		rootCep = netdev_priv(ni_info_data.dev[CPU_PORT0_INST]);
#else
		rootCep = netdev_priv(ni_info_data.dev[0]);
#endif

	if (!(rootCep->dev->flags & IFF_UP)) {
		printk(KERN_ERR "%s: %s is not up flags 0x%x\n", __func__, rootCep->dev->name, rootCep->dev->flags);
		return CA_E_ERROR;
	}
	
#if 0//!defined(CONFIG_LUNA_G3_SERIES)	
	/* it's no need to use a timer callback routine too free transmit buffers */
	setup_timer(&cep->tx_completion_timer, ca_ni_tx_completion_timer_cb, (unsigned long)dev);
#endif
	printk("%s opened\n", dev->name);
	return CA_E_OK;
}

static int ca_ni_multi_lan_dev_stop(struct net_device *dev)
{
	printk("%s closed\n", dev->name);
	return CA_E_OK;
}
#if 1//def CONFIG_LUNA_G3_SERIES
static struct rtnl_link_stats64 *ca_ni_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *storage) {
	ca_eth_private_t *cep = netdev_priv(dev);
	aal_ni_mib_stats_t stats;
	
	if (CA_E_OK == aal_ni_eth_port_mib_get(0,cep->port_cfg.tx_ldpid,0,&stats)) {
		storage->rx_packets = stats.rx_uc_pkt_cnt+stats.rx_mc_frame_cnt+stats.rx_bc_frame_cnt;
		storage->tx_packets = stats.tx_uc_pkt_cnt+stats.tx_mc_frame_cnt+stats.tx_bc_frame_cnt;
	}

#ifdef CONFIG_FC_RTL8198F_SERIES
	{
	struct net_device_stats *stats_netdev = &dev->stats;
	storage->rx_packets = stats_netdev->rx_packets;
	storage->tx_packets = stats_netdev->tx_packets;
	storage->rx_bytes = stats_netdev->rx_bytes;
	storage->tx_bytes = stats_netdev->tx_bytes;
	storage->rx_errors = stats_netdev->rx_errors;
	storage->tx_errors = stats_netdev->tx_errors;
	storage->rx_dropped = stats_netdev->rx_dropped;
	storage->tx_dropped = stats_netdev->tx_dropped;
	storage->multicast = stats_netdev->multicast;
	storage->collisions = stats_netdev->collisions;
	storage->rx_length_errors = stats_netdev->rx_length_errors;
	storage->rx_over_errors = stats_netdev->rx_over_errors;
	storage->rx_crc_errors = stats_netdev->rx_crc_errors;
	storage->rx_frame_errors = stats_netdev->rx_frame_errors;
	storage->rx_fifo_errors = stats_netdev->rx_fifo_errors;
	storage->rx_missed_errors = stats_netdev->rx_missed_errors;
	storage->tx_aborted_errors = stats_netdev->tx_aborted_errors;
	storage->tx_carrier_errors = stats_netdev->tx_carrier_errors;
	storage->tx_fifo_errors = stats_netdev->tx_fifo_errors;
	storage->tx_heartbeat_errors = stats_netdev->tx_heartbeat_errors;
	storage->tx_window_errors = stats_netdev->tx_window_errors;
	storage->rx_compressed = stats_netdev->rx_compressed;
	storage->tx_compressed = stats_netdev->tx_compressed;
	}
#endif

	return storage;
}
#endif
static const struct net_device_ops ca_ni_multi_lan_netdev_ops = {
	.ndo_open		= ca_ni_multi_lan_dev_open,
	.ndo_stop		= ca_ni_multi_lan_dev_stop,
#ifdef CONFIG_RTK_NIC_TX_HOOK
	.ndo_start_xmit 	= ca_ni_start_xmit_check,
#else
	.ndo_start_xmit		= ca_ni_start_xmit,
#endif
	.ndo_do_ioctl		= ca_ni_ioctl,
	.ndo_tx_timeout		= ca_ni_tx_timeout,
	.ndo_set_mac_address = ca_ni_set_mac_address,
	.ndo_get_stats		= ca_ni_get_stats,
#if 1//def CONFIG_LUNA_G3_SERIES
	.ndo_get_stats64	= ca_ni_get_stats64,
#endif
};
#endif


/***************************************************************************
 * platform driver registration
 ***************************************************************************/

#define CA_NI_SCFG_READ_INTF_PORTS(idx, data) aal_scfg_read(CFG_ID_NE_INTF_PORTS##idx, CFG_ID_NE_INTF_PORTS0_LEN, (ca_uint8_t *)data)
#define CA_NI_IRQ_OF_PARSE_AND_MAP(np, idx) irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_FIFO##idx)
#define CA_NI_SCFG_READ_MAC_ADDR(idx, mac_addr) aal_scfg_read(CFG_ID_NE_MAC_ADDR##idx, CA_ETH_ADDR_LEN, mac_addr);

static void ca_ni_dump_intfs(void)
{
      int i, j, index;
        ca_eth_private_t *cep;

        for (i = 0; i < ca_ni_num_intf; i++) {
                cep = netdev_priv(ni_info_data.dev[i]);
                printk("%s: ===================== interface %d ===================\n", __func__, i);
                printk("ni_info_data.dev[i]=0x%p, cep=0x%p\n", ni_info_data.dev[i], cep);

                for (j = 0; j < cep->num_cpu_port; j++) {
                        index = driver_startup_config[i].cpu_port_id[j];
                        printk("cpu_port[%d]=%d, port_cfg_infos[%d].irq[0]=%d\n", j, cep->cpu_port[j], index, port_cfg_infos[index].irq[0]);
                }
                for (j = 0; j < port_cfg_infos[i].num_phy_port; j++) {
                        printk("port_id[%d]=%d\n", j, port_cfg_infos[i].port_id[j]);
                }
                printk("MAC ADDR=%pM\n", port_cfg_infos[i].mac_addr);
                printk("duplex=0x%x, flowctrl=0x%x, tx_ldpid=%d\n",
                        port_cfg_infos[i].duplex, port_cfg_infos[i].flowctrl,
                        port_cfg_infos[i].tx_ldpid);
        }
}

static void ca_ni_init_port_configs(struct platform_device *pdev)
{
	int i, j, cnt, ret, ret1, irq;
	ca_uint8_t ne_intf_ports[CFG_ID_NE_INTF_PORTS0_LEN];
	ca_uint8_t ne_tx_ldpids[CFG_ID_NE_TX_LDPID_LEN];
	ca_mac_addr_t mac_addr;
#ifdef CONFIG_OF
	struct device_node *np = pdev->dev.of_node;
#endif

	if (aal_scfg_read(CFG_ID_NE_TX_LDPID, CFG_ID_NE_TX_LDPID_LEN, ne_tx_ldpids) != CA_E_OK) {
		printk("%s: TX LDPID for all interface does not be specified!!\n", __func__);
		BUG();
	}

	/* get the required constants for all interfaces from startup config */
	for (i = 0; i < CA_NI_MAX_INTF; i++) {
		memset(&(port_cfg_infos[i]), 0, sizeof(ca_port_cfg_info_t));
		switch (i) {
			case 0:
				ret = CA_NI_SCFG_READ_INTF_PORTS(0, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 0);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(0, mac_addr);
				break;
			case 1:
				ret = CA_NI_SCFG_READ_INTF_PORTS(1, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 1);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(1, mac_addr);
				break;
			case 2:
				ret = CA_NI_SCFG_READ_INTF_PORTS(2, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 2);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(2, mac_addr);
				break;
			case 3:
				ret = CA_NI_SCFG_READ_INTF_PORTS(3, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 3);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(3, mac_addr);
				break;
			case 4:
				ret = CA_NI_SCFG_READ_INTF_PORTS(4, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 4);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(4, mac_addr);
				break;
			case 5:
				ret = CA_NI_SCFG_READ_INTF_PORTS(5, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 5);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(5, mac_addr);
				break;
			case 6:
				ret = CA_NI_SCFG_READ_INTF_PORTS(6, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 6);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(6, mac_addr);
				break;
			case 7:
				ret = CA_NI_SCFG_READ_INTF_PORTS(7, ne_intf_ports);
				irq = CA_NI_IRQ_OF_PARSE_AND_MAP(np, 7);
				ret1 = CA_NI_SCFG_READ_MAC_ADDR(7, mac_addr);
				break;
			default:
				printk("%s: interface ID exceed 7!!\n", __func__);
				BUG();
				break;
		}

		if (ret == CA_E_OK && ret1 == CA_E_OK) {
			for (j = 0, cnt = 0;  j < CFG_ID_NE_INTF_PORTS0_LEN && cnt < CA_NI_NUM_OF_PORT; j++) {
				if (ne_intf_ports[j] == 0xFF)
					continue;
				port_cfg_infos[i].port_id[cnt] = ne_intf_ports[j];
				cnt++;
			}
			port_cfg_infos[i].num_phy_port = cnt;
			port_cfg_infos[i].duplex = DUPLEX_FULL,
				port_cfg_infos[i].flowctrl = FLOW_CTRL_TX | FLOW_CTRL_RX,
				port_cfg_infos[i].irq[0] = irq;            /* irq will be got from device tree later */
			port_cfg_infos[i].rmii_clk_src = 0;
			memcpy(&(ca_ni_eth_mac[i][0]), mac_addr, CA_ETH_ADDR_LEN);
			port_cfg_infos[i].mac_addr = &(ca_ni_eth_mac[i][0]);
			port_cfg_infos[i].rx_ldpid = driver_startup_config[i].cpu_port_id[0];
			port_cfg_infos[i].tx_ldpid = ne_tx_ldpids[i];
			printk("%s: port_cfg_infos[%d].irq[0]=%d\n", __func__, i, port_cfg_infos[i].irq[0]);
		}
		else {
			if (ret != CA_E_OK)
				printk("%s: interface ID %d does not be defined in startup config!!\n", __func__, i);
			if (ret1 != CA_E_OK)
				printk("%s: interface ID %d does not be defined in startup config!!\n", __func__, i);
			BUG();
		}
	}
}

static int ca_ni_init_cpu_ports(struct platform_device *pdev)
{
	int i, j, index;
	struct net_device *dev;
	ca_port_cfg_info_t *port_cfg;
	ca_eth_private_t *cep;
	aal_ni_hv_glb_internal_port_id_cfg_t ni_port_id_cfg;
	int num_queue;

	ca_ni_init_port_configs(pdev);

	memset(&ni_port_id_cfg, 0, sizeof(aal_ni_hv_glb_internal_port_id_cfg_t));
	aal_ni_hv_glb_internal_port_id_cfg_get(0, &ni_port_id_cfg);

	/* by default there is only 2 network interface */
	/* LANL2RP0 lpid=16 used for LAN port */
	/* WANL2RP0 lpid=17 used for WAN port */
	printk("%s: ca_ni_num_intf=%d\n", __func__, ca_ni_num_intf);
	for (i = 0; i < ca_ni_num_intf; i++) {

		port_cfg = &port_cfg_infos[i];
#ifdef CONFIG_NET_SCH_MULTIQ
		num_queue = QUEUE_PER_INSTANCE;
#else
		num_queue = 1;
#endif
		dev = alloc_netdev_mqs(sizeof(ca_eth_private_t), driver_startup_config[i].dev_name, NET_NAME_UNKNOWN, ether_setup, num_queue, num_queue);
		if (dev == NULL) {
			pr_err("Unable to alloc new ethernet interface #%d.\n", i);
			return -ENOMEM;
		}

		/* Set the sysfs physical device reference for the network logical device */
		SET_NETDEV_DEV(dev, &pdev->dev);
		
#if defined(CONFIG_RTK_MULTI_WAN_NAME)
		if (i == 1) {
			/*
			 * 2nd interface is nas0
			 * Need to take care the name length
			 */
			snprintf(&dev->name[0], IFNAMSIZ, "%s0", CONFIG_RTK_MULTI_WAN_NAME);
		}
#endif
		ni_info_data.dev[i] = dev;

		/* fill all of the items of cep */
		cep = netdev_priv(dev);
		cep->dev = dev;
		cep->pdev = pdev;
		cep->port_cfg = *port_cfg;
		cep->rx_checksum = CA_ENABLE;
		cep->an_bng_mac_idx = 0xffff;
		cep->an_bng_mac_idx_pe0 = 0xffff;
		cep->an_bng_mac_idx_pe1 = 0xffff;

		/* get the cpu_port and mac address from startup configure */
		for (j = 0; j < driver_startup_config[i].num_cpu_port; j++) {
			cep->cpu_port[j] = driver_startup_config[i].cpu_port_id[j];
		}
		cep->num_cpu_port = driver_startup_config[i].num_cpu_port;

		/* get the DMA LSO VP from startup configure */
		cep->egress_cpu_port_id = driver_startup_config[i].egress_cpu_port_id;

		/* msg_enable can be get/set by ethtool for debug message level */
		cep->msg_enable = netif_msg_init(debug, default_msg);

		/* multiple cpu port has multiple napi */
		for (j = 0; j < cep->num_cpu_port; j++) {
			cep->napi[j].dev = dev;
		}

		cep->use_default_lso_para0 = 0;
		cep->deep_q = 0;
		cep->dma_l3fe = (ni_port_id_cfg.dma_to_l3fe & (1 << i))? 1:0;

		/* get the index from startup configure */
		index = driver_startup_config[i].my_mac_index;
		memcpy(&cep->port_cfg.mac_addr[0], &port_cfg_infos[index].mac_addr[0], sizeof(ca_mac_addr_t));

		memcpy(&dev->dev_addr[0], &(cep->port_cfg.mac_addr[0]), sizeof(ca_mac_addr_t));
		memcpy(dev->perm_addr, dev->dev_addr, sizeof(ca_mac_addr_t));

		for (j = 0; j < cep->port_cfg.num_phy_port; j++) {
			aal_ni_set_mac_address(cep->port_cfg.port_id[j], dev->dev_addr);
		}

		/* initialize TX/RX flow control */
		ca_ni_set_flow_control(&(cep->port_cfg));

		/* fill all of the items of dev */
		dev->netdev_ops = &ca_ni_netdev_ops;
		ca_ni_set_ethtool_ops(dev);

		dev->features = NETIF_F_SG | NETIF_F_HW_CSUM | NETIF_F_RXCSUM | NETIF_F_UFO;

		dev->hw_features = dev->features | NETIF_F_TSO | NETIF_F_TSO6 ;

		/* assign the irq list from cpu ports */
		for (j = 0; j < driver_startup_config[i].num_cpu_port; j++) {
			index = driver_startup_config[i].cpu_port_id[j];
			cep->port_cfg.irq[j] = port_cfg_infos[index].irq[0];
			printk("%s: port_cfg_infos[%d].irq[0]=%d\n", __func__, index, port_cfg_infos[index].irq[0]);
		}
		dev->irq = cep->port_cfg.irq[0];
		dev->watchdog_timeo = CA_NI_TX_TIMEOUT;
		dev->features |= NETIF_F_GRO;

		/* initialize spin locks */
		spin_lock_init(&cep->stats_lock);
		spin_lock_init(&cep->lock);
	} /* for (i = 0; i < ca_ni_num_intf; i++) */

	/* dump port config for all of interfaces */
	ca_ni_dump_intfs();
	return 0;
}

static void ca_ni_do_queue_work(void)
{
	int ret;

	ret = queue_work(ca_ni_fill_eq_pool_wq, &ca_ni_fill_eq_pool_work);
	if (ret == false) {
           if (ca_ni_debug & NI_DBG_NI) {
		pr_warning("add work to ca_ni_fill_eq_pool_wq fail, already exist.\n");
           }
		return;
	}
}

u32 ca_ni_fill_empty_buffer_by_rule(struct net_device *dev, u8 eqid, int rule)
{
	u32 phy_addr = 0;
	u8 cpu_pool0_eq_id, cpu_pool1_eq_id;
	struct sk_buff *skb;
	ca_eth_private_t *cep = netdev_priv(dev);
	struct platform_device *pdev = cep->pdev;
	int head_room = aal_l3qm_get_cpu_port_head_room_first();
	ca_uint_t cpu_pool0_start_addr, cpu_pool1_start_addr;
	ca_uint32_t  len;
	ca_uint8_t *tmp_buf;

	if (rule == 0) {

		/* if use build_skb(), we only need to allocate the packet data buffer here */
		/* will call build_skb() when packet data are received */
		if (ca_ni_use_build_skb) {
			/* only allocate data buffer for QM EQ pool, without skb struct */
			len = CA_NI_SKB_PKT_LEN + head_room + 16;
			len += NET_SKB_PAD + SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
			len = SKB_DATA_ALIGN(len);
			tmp_buf = netdev_alloc_frag(len);
			if (tmp_buf == NULL) {
				printk("%s: kmalloc failed!!!\n", __func__);

				/* wait up the work queue to wait for memory comes back */
				ca_ni_enable_qm_interrupts(0);
				ca_ni_do_queue_work();

				return 0;
			}

			if (ca_ni_debug & NI_DBG_NI) {
				printk("%s: tmp_buf=0x%p, len=%d\n", __func__, tmp_buf, len);
			}
			if ((unsigned long)(tmp_buf) % 128) {
				printk("%s: tmp_buf=0x%p not alignment!!\n", __func__, tmp_buf);
			}
			dma_wmb();

			/* should invalidate the cache data to prevent the wrong data be written to device when the cache is swapped out */
			/* if ACE on no need to do invalidate */
			if (aal_l3qm_get_ace_test() == 0) {
				dma_map_single(&(pdev->dev), tmp_buf, len, DMA_FROM_DEVICE);
			}

			dma_wmb();

			phy_addr = virt_to_phys(tmp_buf);

			if (ca_ni_debug & NI_DBG_NI) {
				printk("%s: phy_addr=0x%x, tmp_buf=%p\n", __func__, phy_addr, tmp_buf);
			}
		}
		else {

			/* allocate skb data size=2048+32(head room)+16(reset room) */
			skb = ca_netdev_alloc_skb(dev, CA_NI_SKB_PKT_LEN + head_room + 16, true);

			if (!skb) {
				
				printk_ratelimited("%s: ca_netdev_alloc_skb() failed!!!\n", dev->name);

				/* wait up the work queue to wait for memory comes back */
				ca_ni_enable_qm_interrupts(0);
				ca_ni_do_queue_work();

				return 0;
			}

			/* store the sk_buff pointer to software head room */
			*((ca_uint_t *)(skb->data)) = (ca_uint_t)skb;

			/* flush the data we write to skb->data to prevent later CPU flush cache data to overwrite the data filled by hardware */
			dma_map_single(&(pdev->dev), skb->data, 128, DMA_TO_DEVICE);

			/* should invalidate the cache data to prevent the wrong data be written to device when the cache is swapped out */
			/* if ACE on no need to do invalidate */
			if (aal_l3qm_get_ace_test() == 0) {
				dma_map_single(&(pdev->dev), skb->data, CA_NI_SKB_PKT_LEN + head_room + 16, DMA_FROM_DEVICE);
			}

			/* an private flag to indicate the skb buffer is allocated for L3QM */
			skb->is_from_l3qm = 1;

			phy_addr = virt_to_phys(skb->data);

			if (ca_ni_debug & NI_DBG_NI) {
				printk("%s: phy_addr=0x%x, skb->head=%p, skb->data=%p, skb=%p, head_stored=0x%llx\n",
						__func__, phy_addr, skb->head, skb->data, skb, *((ca_uint64_t *)(skb->data)));
			}
		}
	}
	else {
		aal_l3qm_eq_get_cpu_pool0_1_eq_id(&cpu_pool0_eq_id, &cpu_pool1_eq_id);
		if (eqid == cpu_pool0_eq_id) {
			phy_addr = virt_to_phys((void *) ca_ne_cpu_pool0_start_addr);

			ca_ne_cpu_pool0_start_addr += CA_NI_SKB_PKT_LEN;
			aal_l3qm_eq_get_cpu_pool0_1_start_addr(&cpu_pool0_start_addr, &cpu_pool1_start_addr);
			if (ca_ne_cpu_pool0_start_addr > (cpu_pool0_start_addr + CA_NI_SKB_PKT_LEN * aal_l3qm_eq_get_cpu_pool_bid_count())) {
				printk("%s: ca_ne_cpu_pool0_start_addr=0x%llx exceed boundary\n", __func__, (ca_uint64_t)ca_ne_cpu_pool0_start_addr);
				ca_ne_cpu_pool0_start_addr = cpu_pool0_start_addr;
			}
		}
		if (eqid == cpu_pool1_eq_id) {
			phy_addr = virt_to_phys((void *) ca_ne_cpu_pool1_start_addr);

			ca_ne_cpu_pool1_start_addr += CA_NI_SKB_PKT_LEN;
			aal_l3qm_eq_get_cpu_pool0_1_start_addr(&cpu_pool0_start_addr, &cpu_pool1_start_addr);
			if (ca_ne_cpu_pool1_start_addr > (cpu_pool1_start_addr + CA_NI_SKB_PKT_LEN * aal_l3qm_eq_get_cpu_pool_bid_count())) {
				printk("%s: ca_ne_cpu_pool1_start_addr=0x%llx exceed boundary\n", __func__, (ca_uint64_t)ca_ne_cpu_pool1_start_addr);
				ca_ne_cpu_pool1_start_addr = cpu_pool1_start_addr;
			}
		}
		if (ca_ni_debug & NI_DBG_NI) {
			printk("%s: eqid=%d, cpu_pool0_eq_id=%d, cpu_pool1_eq_id=%d\n", __func__, eqid, cpu_pool0_eq_id, cpu_pool1_eq_id);
			printk("%s: phy_addr=0x%x, ca_ne_cpu_pool0_start_addr=0x%llx, ca_ne_cpu_pool1_start_addr=0x%llx\n",
				__func__, phy_addr, (ca_uint64_t)ca_ne_cpu_pool0_start_addr, (ca_uint64_t)ca_ne_cpu_pool1_start_addr);
		}
	}
	return phy_addr;
}

static int ca_ni_fbm_fill_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 count, int rule)
{
	int i;
	u32 phy_addr = 0;
	ca_uint8_t fbm_pool_id;

	if (eqid == ca_ni_cpu64_pool0_eq_id) {
		fbm_pool_id = ca_ni_cpu64_pool0_fbm_pool_id;
	}
	else {
		fbm_pool_id = ca_ni_cpu64_pool1_fbm_pool_id;
	}

	for (i = 0; i < count; i++) {
		phy_addr = ca_ni_fill_empty_buffer_by_rule(dev, eqid, rule);
		/* if lack of memory just jump out */
		if (phy_addr == 0) {
			return -1;
		}
		if (phy_addr) {
			aal_fbm_buf_push(FBM_CPU_ARM0, fbm_pool_id, phy_addr);
		}
        }
	return 0;
}

int ca_ni_fill_empty_buffer(struct net_device *dev, u8 cpu_port, u8 eqid, u16 inactive_bid_cntr, int rule)
{
	int i;
	u16 count;
	u32 phy_addr = 0;
	int push_count = 0;

	if (ca_ni_debug & NI_DBG_NI) {
		printk("%s: cpu_port=%d, eqid=%d, inactive_bid_cntr=%d\n", __func__, cpu_port, eqid, inactive_bid_cntr);
	}

	while (inactive_bid_cntr) {

		if (inactive_bid_cntr > 8) {
			count = 8;
			inactive_bid_cntr -= 8;
		}
		else {
			count = inactive_bid_cntr;
			inactive_bid_cntr = 0;
		}

		/* push physical address to Empty Buffer */
		for (i = 0; i < count; i++) {

			phy_addr = ca_ni_fill_empty_buffer_by_rule(dev, eqid, rule);
			/* out of memory just jump out */
			if (phy_addr == 0) return -1;

			/* check ready bit */
			if (aal_l3qm_check_cpu_push_ready(cpu_port)) {
				aal_l3qm_set_cpu_push_paddr(cpu_port, eqid, phy_addr);
				push_count++;
			} else {
				/*Need to free the packet*/
				if (aal_l3qm_eq_get_cpu_rule() == 0) {
					if (ca_ni_use_build_skb) {
						skb_free_frag(phys_to_virt(phy_addr));
					}else {
						struct sk_buff *skb = (struct sk_buff *) (*((ca_uint_t *) phys_to_virt(phy_addr)));
						ca_dev_kfree_skb_any(skb);
					}
					if (ca_ni_debug & NI_DBG_NI) {
					printk("%s: fill buffer fail at cpu_port[%d] eqid[%d]\n",
							__func__, cpu_port, eqid);
					}
				}
			}
		} /* for loop */

		/* only check req bit, do not update inactive_brd_cntr to avoid infinite loop */
		if (aal_l3qm_get_inactive_bid_cntr(eqid) == 0) {
			if (ca_ni_debug & NI_DBG_NI) {
				printk("%s: req bit not 1 or inactive bid count=0!!\n", __func__);
			}
			break;
		}

	} /* while loop */

	if (ca_ni_debug & NI_DBG_NI) {
		printk("%s: ======== push_count=%d ======== \n", __func__, push_count);
	}
	return 0;
}

int ca_ni_refill_eq_buf_pool(struct net_device *dev, u8 cpu_port, u8 eqid, int cnt)
{
	int ret = 0;
	u16 inactive_bid_cntr;
	unsigned long flags;

	/* the EQ pools are shared by all of CPU CPU ports should be locked before changed */
	spin_lock_irqsave(&ca_ni_rx_refill_lock, flags);

	if (ca_ni_cpu64_use_fbm) {
		ret = ca_ni_fbm_fill_empty_buffer(dev, cpu_port, eqid, cnt, aal_l3qm_eq_get_cpu_rule());
	}
	else {

		/* refill the first EQ buffer pool */
		inactive_bid_cntr = aal_l3qm_get_inactive_bid_cntr(eqid);
		if (inactive_bid_cntr > cnt)
			inactive_bid_cntr = cnt;
		if (inactive_bid_cntr)
			ca_ni_fill_empty_buffer(dev, cpu_port, eqid, inactive_bid_cntr, aal_l3qm_eq_get_cpu_rule());
	}

	spin_unlock_irqrestore(&ca_ni_rx_refill_lock, flags);

	if (ca_ni_debug & NI_DBG_NI)
		printk("%s: ======== cpu_port=%d, eqid=%d, inactive_bid_cntr=%d, cnt=%d ======== \n", __func__, cpu_port, eqid, inactive_bid_cntr, cnt);
	return ret;
}

void ca_ni_fill_eq_buf_pool(struct net_device *dev, u8 cpu_port, int cnt)
{
	/* fill EQ buffer for CPU64 */
	ca_ni_refill_eq_buf_pool(dev, cpu_port, l3qm_eq_profile_cpu_pool0_eq_id, cnt);
	ca_ni_refill_eq_buf_pool(dev, cpu_port, l3qm_eq_profile_cpu_pool1_eq_id, cnt);
}

void ca_ni_init_fbm(void)
{
	ca_uint32_t phy_start_addr, entry_size, entry_count;
	ca_uint_t virt_start_addr;
	ca_uint8_t fbm_pool_id;
	struct aal_fbm_pool_config_s  fbm_pool;

	aal_fbm_init();

	if (ca_ni_cpu64_use_fbm) {

		aal_l3qm_eq_get_cpu_pool0_info(&virt_start_addr, &entry_size, &entry_count, &fbm_pool_id);

		phy_start_addr = virt_to_phys((void *) virt_start_addr);

		printk("%s: CPU pool0 virt_start_addr=0x%llx, phy_start_addr=0x%x, entry_size=%d, entry_count=%d, fbm_pool_id=%d\n", __func__,
				(ca_uint64_t)virt_start_addr, phy_start_addr, entry_size, entry_count, fbm_pool_id);

		fbm_pool.cache_invalidate = 0;
		fbm_pool.memset_mode = FBM_MSET_DISABLE;
		fbm_pool.mset_cainv_size = entry_size;
		aal_fbm_pool_init(fbm_pool_id, &fbm_pool);

		aal_l3qm_eq_get_cpu_pool1_info(&virt_start_addr, &entry_size, &entry_count, &fbm_pool_id);

		phy_start_addr = virt_to_phys((void *) virt_start_addr);

		printk("%s: CPU pool1 virt_start_addr=0x%llx, phy_start_addr=0x%x, entry_size=%d, entry_count=%d, fbm_pool_id=%d\n", __func__,
				(ca_uint64_t)virt_start_addr, phy_start_addr, entry_size, entry_count, fbm_pool_id);

		fbm_pool.cache_invalidate = 0;
		fbm_pool.memset_mode = FBM_MSET_DISABLE;
		fbm_pool.mset_cainv_size = entry_size;
		aal_fbm_pool_init(fbm_pool_id, &fbm_pool);
	}

#if defined(CONFIG_NE_CPU256)
	if (aal_l3qm_eq_get_cpu256_hi_use_fbm()) {
		aal_l3qm_eq_get_cpu256_hi_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
		fbm_pool.cache_invalidate = 0;
		fbm_pool.memset_mode = FBM_MSET_DISABLE;
		fbm_pool.mset_cainv_size = entry_size;
		aal_fbm_pool_init(fbm_pool_id, &fbm_pool);

		printk("%s: CPU256 hi pool 0 phy_start_addr=0x%x, entry_size=%d, entry_count=%d, fbm_pool_id=%d\n", __func__,
		phy_start_addr, entry_size, entry_count, fbm_pool_id);
	}

	if (aal_l3qm_eq_get_cpu256_lo_use_fbm()) {
		aal_l3qm_eq_get_cpu256_lo_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
		fbm_pool.cache_invalidate = 0;
		fbm_pool.memset_mode = FBM_MSET_DISABLE;
		fbm_pool.mset_cainv_size = entry_size;
		aal_fbm_pool_init(fbm_pool_id, &fbm_pool);

		printk("%s: CPU256 lo pool 0 phy_start_addr=0x%x, entry_size=%d, entry_count=%d, fbm_pool_id=%d\n", __func__,
			phy_start_addr, entry_size, entry_count, fbm_pool_id);

		printk("%s: inactive BID counter=%d\n", __func__, aal_l3qm_get_inactive_bid_cntr(6));
	}
#endif
}

static void ca_ni_reset_task(struct work_struct *work)
{
	int i, j;
	ca_eth_private_t *cep;
	struct net_device *dev;

	printk("%s \n" , __func__);

	rtnl_lock();

	for (i = 0; i < ca_ni_num_intf; i++) {
		dev = ni_info_data.dev[i];
		if (dev == NULL)
			continue;
		cep = netdev_priv(dev);
		for (j = 0; j < cep->num_cpu_port; j++) {
			napi_disable(&cep->napi[j]);
		}
	}

	// reset tx ring
	ca_ni_reset_tx_ring();

	for (i = 0; i < ca_ni_num_intf; i++) {
		dev = ni_info_data.dev[i];
		if (dev == NULL)
			continue;
		cep = netdev_priv(dev);
		netif_tx_wake_all_queues(dev);  // enable tx
	}

	rtnl_unlock();
} /* cs_ni_reset_task */

/* NAPI polling routine for CPU port 0 (WAN port) */
static int ca_ni_poll_0(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT0_INST, napi, budget);
}

/* NAPI polling routine for CPU port 1 (LAN port) */
static int ca_ni_poll_1(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT1_INST, napi, budget);
}

/* NAPI polling routine for CPU port 2 (LAN port) */
static int ca_ni_poll_2(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT2_INST, napi, budget);
}

/* NAPI polling routine for CPU port 3 (LAN port) */
static int ca_ni_poll_3(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT3_INST, napi, budget);
}

/* NAPI polling routine for CPU port 4 (LAN port) */
static int ca_ni_poll_4(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT4_INST, napi, budget);
}

/* NAPI polling routine for CPU port 5 (LAN port) */
static int ca_ni_poll_5(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT5_INST, napi, budget);
}

/* NAPI polling routine for CPU port 6 (LAN port) */
static int ca_ni_poll_6(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT6_INST, napi, budget);
}

/* NAPI polling routine for CPU port 7 (LAN port) */
static int ca_ni_poll_7(struct napi_struct *napi, int budget)
{
	return ca_ni_rx_poll(CPU_PORT7_INST, napi, budget);
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

#if defined(CONFIG_NE_CPU256)
	ca_ni_register_cpu256_netdev();
#endif
	return 0;
}

#ifdef CONFIG_OF
static void ca_ni_global_reset(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct reset_control *ni_reset, *l2fe_reset, *l2tm_reset, *l3fe_reset, *tqm_reset;
	ca_uint8_t phy_intfs[CFG_ID_PHY_INTERFACE_LEN];
	struct phy *sgmii_phy;
	struct reset_control *phy_sdsif_reset, *phy_sgmii_reset;
	struct reset_control *ext_reset;
	struct reset_control *ptp_timer_reset, *ptp_reset;

	ni_reset = of_reset_control_get(np, "ni_reset");
	if (IS_ERR(ni_reset)) {
		dev_warn(&pdev->dev, "OF get ni_reset failed!!");
	}
	else {
		reset_control_assert(ni_reset);
        mdelay(1);
		reset_control_deassert(ni_reset);
	}
	reset_control_put(ni_reset);

	l2fe_reset = of_reset_control_get(np, "l2fe_reset");
	if (IS_ERR(l2fe_reset)) {
		dev_warn(&pdev->dev, "OF get l2fe_reset failed!!");
	}
	else {
		reset_control_assert(l2fe_reset);
        mdelay(1);
		reset_control_deassert(l2fe_reset);
	}
	reset_control_put(l2fe_reset);

	l2tm_reset = of_reset_control_get(np, "l2tm_reset");
	if (IS_ERR(l2tm_reset)) {
		dev_warn(&pdev->dev, "OF get l2tm_reset failed!!");
	}
	else {
		reset_control_assert(l2tm_reset);
        mdelay(1);
		reset_control_deassert(l2tm_reset);
	}
	reset_control_put(l2tm_reset);

	l3fe_reset = of_reset_control_get(np, "l3fe_reset");
	if (IS_ERR(l3fe_reset)) {
		dev_warn(&pdev->dev, "OF get l3fe_reset failed!!");
	}
	else {
		reset_control_assert(l3fe_reset);
        mdelay(1);
		reset_control_deassert(l3fe_reset);
	}
	reset_control_put(l3fe_reset);

	tqm_reset = of_reset_control_get(np, "tqm_reset");
	if (IS_ERR(tqm_reset)) {
		dev_warn(&pdev->dev, "OF get tqm_reset failed!!");
	}
	else {
		reset_control_assert(tqm_reset);
        mdelay(1);
		reset_control_deassert(tqm_reset);
	}
	reset_control_put(tqm_reset);

	/* Reset HSGMII */
	aal_scfg_read(CFG_ID_PHY_INTERFACE, CFG_ID_PHY_INTERFACE_LEN, &(phy_intfs[0]));
	if (phy_intfs[AAL_NI_PORT_GE4] != AAL_NI_IF_MODE_GE_RGMII) {
		printk("%s: Port 4 connect to HSGMII\n", __func__);
	}
	else {
		printk("%s: Port 4 connect to RGMII\n", __func__);
	}
	if (phy_intfs[AAL_NI_PORT_GE4] != AAL_NI_IF_MODE_GE_RGMII) {
		sgmii_phy = devm_phy_get(&pdev->dev, "sgmii-phy");
		if (IS_ERR(sgmii_phy)) {
			dev_warn(&pdev->dev, "OF get sgmii-phy failed!!");
		}
		else {
			printk("%s: sgmii_phy=%p\n", __func__, sgmii_phy);
			phy_power_on(sgmii_phy);
			mdelay(1);
		}

		phy_sdsif_reset = of_reset_control_get(np, "phy_sdsif_reset");
		if (IS_ERR(phy_sdsif_reset)) {
			dev_warn(&pdev->dev, "OF get phy_sdsif_reset failed!!");
		}
		else {
			reset_control_assert(phy_sdsif_reset);
			mdelay(1);
			reset_control_deassert(phy_sdsif_reset);
		}
		reset_control_put(phy_sdsif_reset);

		phy_sgmii_reset = of_reset_control_get(np, "phy_sgmii_reset");
		if (IS_ERR(phy_sgmii_reset)) {
			dev_warn(&pdev->dev, "OF get phy_sgmii_reset failed!!");
		}
		else {
			reset_control_assert(phy_sgmii_reset);
			mdelay(1);
			reset_control_deassert(phy_sgmii_reset);
		}
		reset_control_put(phy_sgmii_reset);

		printk("%s: Do external RESET for RTL8367SB HSGMII mode....\n", __func__);
		ext_reset = of_reset_control_get(np, "ext_reset");
		if (IS_ERR(ext_reset)) {
			dev_warn(&pdev->dev, "OF get ext_reset failed!!");
		}
		else {
			reset_control_assert(ext_reset);
			mdelay(200);
			reset_control_deassert(ext_reset);
		}
		reset_control_put(ext_reset);
	}
	ptp_timer_reset = of_reset_control_get(np, "ptp_timer_reset");
	if (IS_ERR(ptp_timer_reset)) {
		dev_warn(&pdev->dev, "OF get ptp_timer_reset failed!!");
	}
	else {
		reset_control_assert(ptp_timer_reset);
		mdelay(1);
		reset_control_deassert(ptp_timer_reset);
	}
	reset_control_put(ptp_timer_reset);

	ptp_reset = of_reset_control_get(np, "ptp_reset");
	if (IS_ERR(ptp_reset)) {
		dev_warn(&pdev->dev, "OF get ptp_reset failed!!");
	}
	else {
		reset_control_deassert(ptp_reset);
		mdelay(1);
		reset_control_assert(ptp_reset);
	}
	reset_control_put(ptp_reset);
	mdelay(100);
}
#endif

static void ca_ni_read_drv_config(int index)
{
	int i;
	int len;
	int new_entry;
	ca_uint8_t dev_name[IFNAMSIZ];
	char *drv_init_config_id, *ifname_config_id;
	ca_uint32_t tmp_config[CFG_ID_NI0_DRV_INIT_LEN];

	if (index >= CA_NI_TOTAL_CPU_PORT) {
		printk("%s: invalid index=%d!!\n", __func__, index);
		return;
	}

	switch (index) {
		case 0:
			drv_init_config_id = CFG_ID_NI0_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF0_NAME;
			len = CFG_ID_NI0_DRV_INIT_LEN;
			break;
		case 1:
			drv_init_config_id = CFG_ID_NI1_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF1_NAME;
			len = CFG_ID_NI1_DRV_INIT_LEN;
			break;
		case 2:
			drv_init_config_id = CFG_ID_NI2_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF2_NAME;
			len = CFG_ID_NI2_DRV_INIT_LEN;
			break;
		case 3:
			drv_init_config_id = CFG_ID_NI3_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF3_NAME;
			len = CFG_ID_NI3_DRV_INIT_LEN;
			break;
		case 4:
			drv_init_config_id = CFG_ID_NI4_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF4_NAME;
			len = CFG_ID_NI4_DRV_INIT_LEN;
			break;
		case 5:
			drv_init_config_id = CFG_ID_NI5_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF5_NAME;
			len = CFG_ID_NI5_DRV_INIT_LEN;
			break;
		case 6:
			drv_init_config_id = CFG_ID_NI6_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF6_NAME;
			len = CFG_ID_NI6_DRV_INIT_LEN;
			break;
		case 7:
			drv_init_config_id = CFG_ID_NI7_DRV_INIT;
			ifname_config_id = CFG_ID_ETH_INTF7_NAME;
			len = CFG_ID_NI7_DRV_INIT_LEN;
			break;
	}

	aal_scfg_read(drv_init_config_id, len * sizeof(ca_uint32_t), &tmp_config[0]);
	printk("%s: index=%d\n", __func__, index);
	printk("%s: my_mac_index = %d\n", __func__, tmp_config[CA_NI_DRV_STARUP_CONFIG_MY_MAC_INDEX]);
	printk("%s: cpu_port_id = %d\n", __func__, tmp_config[CA_NI_DRV_STARUP_CONFIG_CPU_PORT_ID]);
	printk("%s: egress_cpu_port_id = %d\n", __func__, tmp_config[CA_NI_DRV_STARUP_CONFIG_EGRESS_CPU_PORT]);

	/* no used entry */
	if (tmp_config[CA_NI_DRV_STARUP_CONFIG_MY_MAC_INDEX] == 0xFFFFFFFF) {
		return;
	}

	new_entry = 0;
	/* find an existent entry with the same my_mac_index */
	for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
		if (tmp_config[CA_NI_DRV_STARUP_CONFIG_MY_MAC_INDEX] == driver_startup_config[i].my_mac_index) {
			break;
		}
	}
	/* find an empty entry */
	if (i == CA_NI_TOTAL_CPU_PORT) {
		for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
			if (!driver_startup_config[i].enable) {
				new_entry = 1;
				break;
			}
		}
	}
	if (i == CA_NI_TOTAL_CPU_PORT) {
		printk("%s: driver startup configure is full!!\n", __func__);
		return;
	}

	driver_startup_config[i].num_cpu_port++;

	memset(dev_name, 0, IFNAMSIZ);
	aal_scfg_read(ifname_config_id, IFNAMSIZ, dev_name);


	printk("%s: i=%d, num_cpu_port=%d, dev_name=%s\n", __func__, i, driver_startup_config[i].num_cpu_port, dev_name);

	/* check whether the device name is consistent for two entry with the same my_mac_index */
	if (new_entry == 0 && strcmp(driver_startup_config[i].dev_name, dev_name) != 0) {
		printk("%s: dev_name=%s is not correct!!\n", __func__, dev_name);
	}
	else {
		strcpy(driver_startup_config[i].dev_name, dev_name);
	}

	driver_startup_config[i].enable = 1;
	driver_startup_config[i].cpu_port_id[driver_startup_config[i].num_cpu_port - 1] = tmp_config[CA_NI_DRV_STARUP_CONFIG_CPU_PORT_ID];
	driver_startup_config[i].my_mac_index = tmp_config[CA_NI_DRV_STARUP_CONFIG_MY_MAC_INDEX];
	driver_startup_config[i].egress_cpu_port_id = tmp_config[CA_NI_DRV_STARUP_CONFIG_EGRESS_CPU_PORT];

	if (new_entry)
		ca_ni_num_intf++;
}

static void ca_ni_load_driver_init(void)
{
        int i, j;

        ca_ni_num_intf = 0;
        memset(&driver_startup_config[0], 0, sizeof(ca_ni_drv_startup_config_t) * CA_NI_TOTAL_CPU_PORT);

        for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
                driver_startup_config[i].my_mac_index = -1;
        }

        for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
                ca_ni_read_drv_config(i);
                printk("%s: i=%d\n", __func__, i);
        }
        printk("%s: ca_ni_num_intf=%d\n", __func__, ca_ni_num_intf);

        for (i = 0; i < ca_ni_num_intf; i++) {
                printk("%s: index=%d\n", __func__, i);
                printk("%s: driver_startup_config.enable = %d\n", __func__, driver_startup_config[i].enable);
                printk("%s: driver_startup_config.my_mac_index = %d\n", __func__, driver_startup_config[i].my_mac_index);
                printk("%s: driver_startup_config.egress_cpu_port_id = %d\n", __func__, driver_startup_config[i].egress_cpu_port_id);

                for (j = 0; j < driver_startup_config[i].num_cpu_port; j++)
                        printk("%s: driver_startup_config.cpu_port_id[%d] = %d\n", __func__, j, driver_startup_config[i].cpu_port_id[j]);
        }

}

static ca_status_t ca_load_init(ca_g3_init_config_t *p_cfg, ca_uint32_t size)
{
	int		budget;
	ca_uint32_t     temp_data;
	ca_uint8_t 	cpu64_pool0_eq_id=0, cpu64_pool1_eq_id=0;
	ca_uint8_t	cpu64_pool0_fbm_pool_id=0, cpu64_pool1_fbm_pool_id=0;
	ca_uint32_t	cpu64_pool0_buf_count=0, cpu64_pool1_buf_count=0;
	ca_uint8_t 	cpu64_use_fbm=0;
	ca_uint8_t	use_qm_eq_refill_tasklet=0;
	ca_uint8_t	use_qm_refill_int=0;
#if defined(CONFIG_NE_TX_LLID_MAP)
	ca_uint8_t      oam_mode;
#endif

	/* load init config and call ca_init() */
	memset(p_cfg, 0, sizeof(ca_g3_init_config_t));

	if (aal_scfg_read(CFG_ID_NE_CPU_CPU_PORT_COUNT, 1, &ca_ni_cpu_cpu_port_count) == CA_E_OK) {
		if (!ca_ni_cpu_cpu_port_count || ca_ni_cpu_cpu_port_count > CA_NI_TOTAL_CPU_PORT) {
			ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;
			printk("%s: CFG_ID_NE_CPU_CPU_PORT_COUNT=%d too large!!\n", __func__, ca_ni_cpu_cpu_port_count);
		}
	}

	/* number of interfaces can not exceed ca_ni_cpu_cpu_port_count */
	if (ca_ni_num_intf > ca_ni_cpu_cpu_port_count) {
		printk("%s: ca_ni_num_intf can not bigger than ca_ni_cpu_cpu_port_count!!\n", __func__);
		ca_ni_num_intf = ca_ni_cpu_cpu_port_count;
		printk("%s: adjust ca_ni_num_intf to %d\n", __func__, ca_ni_num_intf);
	}

	/* init wan_port_id */
	aal_scfg_read(CFG_ID_WAN_PORT_ID, 1, &wan_port_id);
	p_cfg->ca_ne_port_id = wan_port_id;

	//aal_scfg_read(CFG_ID_NE_MAC_ADDR0, 6, &(p_cfg->ca_ne_mac_addr0[0]));
	//aal_scfg_read(CFG_ID_NE_MAC_ADDR1, 6, &(p_cfg->ca_ne_mac_addr1[0]));

	aal_scfg_read(CFG_ID_NE_SRAM_PHY_BASE, 4, &temp_data);
	p_cfg->ca_ne_sram_phy_base = temp_data;

	aal_scfg_read(CFG_ID_NE_DDR_COHERENT_PHY_BASE, 4, &temp_data);
	p_cfg->ca_ne_ddr_coherent_phy_base = temp_data;

	aal_scfg_read(CFG_ID_NE_DDR_NONCACHE_PHY_BASE, 4, &temp_data);
	p_cfg->ca_ne_ddr_noncache_phy_base = temp_data;

	p_cfg->ca_ne_sram_base = ca_ne_sram_base;
	p_cfg->ca_ne_ddr_noncache_base = ca_ne_ddr_noncache_base;

	/* check consistent between device tree and startup config */
	if (p_cfg->ca_ne_sram_phy_base != ca_ne_sram_phy_base) {
		printk("%s: please make sure CFG_ID_NE_SRAM_PHY_BASE=0x%x consistent with device tree!!\n", __func__, p_cfg->ca_ne_sram_phy_base);
	}

	if (p_cfg->ca_ne_ddr_noncache_phy_base != (ca_ne_ddr_noncache_phy_base & 0xffffffff)) {
		printk("%s: please make sure CFG_ID_NE_DDR_NONCACHE_PHY_BASE=0x%x consistent with device tree!!\n", __func__, p_cfg->ca_ne_ddr_noncache_phy_base);
	}

	if (p_cfg->ca_ne_ddr_coherent_phy_base != ca_ne_ddr_coherent_phy_base) {
		printk("%s: please make sure CFG_ID_NE_DDR_COHERENT_PHY_BASE=0x%x consistent with device tree!!\n", __func__, p_cfg->ca_ne_ddr_coherent_phy_base);
	}
	//memcpy(&(port_cfg_infos[0].mac_addr[0]), &(p_cfg->ca_ne_mac_addr0[0]), 6);
	//memcpy(&(port_cfg_infos[1].mac_addr[0]), &(p_cfg->ca_ne_mac_addr1[0]), 6);

	printk("=================ca_ni_init_config================\n");
	printk("%s: ca_ne_sram_base=%p, ca_ne_ddr_noncache_base=%p\n", __func__, p_cfg->ca_ne_sram_base, p_cfg->ca_ne_ddr_noncache_base);
	printk("%s: ca_ne_sram_phy_base=0x%x, ca_ne_ddr_noncache_phy_base=0x%x, ca_ne_ddr_coherent_phy_base=0x%x\n",
		__func__, p_cfg->ca_ne_sram_phy_base, p_cfg->ca_ne_ddr_noncache_phy_base, p_cfg->ca_ne_ddr_coherent_phy_base);
	//printk("%s: ca_ne_mac_addr0=%pM, ca_ne_mac_addr1=%pM\n", __func__, &(p_cfg->ca_ne_mac_addr0[0]), &(p_cfg->ca_ne_mac_addr1[0]));
	printk("==================================================\n");

	aal_scfg_read(CFG_ID_NE_USE_BUILD_SKB, 1, &ca_ni_use_build_skb);
	printk("%s: ca_ni_use_build_skb=%d\n", __func__, ca_ni_use_build_skb);

        /* read budget from startup config */
        if (aal_scfg_read(CFG_ID_NE_NAPI_BUDGET, 4, &budget) == CA_E_OK) {
                /* the budget == 0 is not allowed */
		if (budget > 0) {
			ca_ni_napi_budget = budget;
		}
	}
	printk("%s: ca_ni_napi_budget=%d\n", __func__, ca_ni_napi_budget);

	aal_scfg_read(CFG_ID_NE_TX_TIMER_ENABLE, 1, &ca_ni_tx_timer_enable);
	printk("%s: ca_ni_tx_timer_enable=%d\n", __func__, ca_ni_tx_timer_enable);

#if defined(CONFIG_NE_CPU256)
	ca_load_cpu256_init();
#endif

	/* read CFG_ID_NE_USE_QM_EQ_REFILL_INT from startup config */
	if (aal_scfg_read(CFG_ID_NE_USE_QM_EQ_REFILL_INT, 1, &use_qm_refill_int) == CA_E_OK) {
		ca_ni_use_qm_eq_refill_int = use_qm_refill_int;
		printk("%s: ca_ni_use_qm_eq_refill_int=%d\n", __func__, ca_ni_use_qm_eq_refill_int);
	}

	/* read eqid for CPU ports */
	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_EQ_ID, 1, &cpu64_pool0_eq_id) == CA_E_OK) {
		ca_ni_cpu64_pool0_eq_id = cpu64_pool0_eq_id;
		printk("%s: ca_ni_cpu64_pool0_eq_id=%d\n", __func__, ca_ni_cpu64_pool0_eq_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_EQ_ID, 1, &cpu64_pool1_eq_id) == CA_E_OK) {
		ca_ni_cpu64_pool1_eq_id = cpu64_pool1_eq_id;
		printk("%s: ca_ni_cpu64_pool1_eq_id=%d\n", __func__, ca_ni_cpu64_pool1_eq_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_USE_FBM, 1, &cpu64_use_fbm) == CA_E_OK) {
		ca_ni_cpu64_use_fbm = cpu64_use_fbm;
		printk("%s: ca_ni_cpu64_use_fbm=%d\n", __func__, ca_ni_cpu64_use_fbm);
	}

#if defined(CONFIG_CORTINA_BOARD_FPGA)
	/* if no FBM module included the always set ca_ni_cpu64_use_fbm=0 */
	if (SOC_HAS_FBM() == 0) {
		ca_ni_cpu64_use_fbm = 0;
		printk("%s: set ca_ni_cpu64_use_fbm=0 because no FBM module\n", __func__);
	}
#endif

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_FBM_POOL_ID, 1, &cpu64_pool0_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu64_pool0_fbm_pool_id = cpu64_pool0_fbm_pool_id;
		printk("%s: ca_ni_cpu64_pool0_fbm_pool_id=%d\n", __func__, ca_ni_cpu64_pool0_fbm_pool_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_FBM_POOL_ID, 1, &cpu64_pool1_fbm_pool_id) == CA_E_OK) {
		ca_ni_cpu64_pool1_fbm_pool_id = cpu64_pool1_fbm_pool_id;
		printk("%s: ca_ni_cpu64_pool1_fbm_pool_id=%d\n", __func__, ca_ni_cpu64_pool1_fbm_pool_id);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_COUNT, 4, &cpu64_pool0_buf_count) == CA_E_OK) {
		ca_ni_cpu64_pool0_buf_count = cpu64_pool0_buf_count;
		printk("%s: ca_ni_cpu64_pool0_buf_count=%d\n", __func__, ca_ni_cpu64_pool0_buf_count);
	}

	if (aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_COUNT, 4, &cpu64_pool1_buf_count) == CA_E_OK) {
		ca_ni_cpu64_pool1_buf_count = cpu64_pool1_buf_count;
		printk("%s: ca_ni_cpu64_pool1_buf_count=%d\n", __func__, ca_ni_cpu64_pool1_buf_count);
	}

	if (aal_scfg_read(CFG_ID_NE_USE_QM_EQ_REFILL_TASKLET, 1, &use_qm_eq_refill_tasklet) == CA_E_OK) {
		ca_ni_use_qm_eq_refill_tasklet = use_qm_eq_refill_tasklet;
		printk("%s: ca_ni_use_qm_eq_refill_tasklet=%d\n", __func__, ca_ni_use_qm_eq_refill_tasklet);
	}

#if defined(CONFIG_NE_TX_LLID_MAP)
	if (aal_scfg_read(CFG_ID_OAM_MODE, 1, &oam_mode) == CA_E_OK) {
		ca_ni_oam_mode = oam_mode;
		printk("%s: ca_ni_oam_mode=%d\n", __func__, ca_ni_oam_mode);
	}
#endif

	ca_ni_load_driver_init();

	return CA_E_OK;
}

#if defined(CONFIG_RTK_NIC_RX_HOOK)
int nic_register_rxfunc_by_port(unsigned int port, p2rfunc_t pfunc)
{
	ca_eth_private_t *cep;

	if(port >= CA_NI_MAX_INTF || !pfunc)
		return -EINVAL;
	printk(KERN_INFO "[%s-%d] port =%d, pfunc=%p\n", __func__,__LINE__,port,pfunc);
	cep = netdev_priv(ni_info_data.dev[port]);
	cep->port2rxfunc[port] = pfunc;
	return 0;
}

int nic_register_rxfunc_all_port(p2rfunc_t pfunc)
{
	unsigned int i;
	int ret;
	
	if(!pfunc)
		return -EINVAL;
	
	printk(KERN_INFO "[%s-%d] pfunc=%p ca_ni_num_intf=%d\n", __func__,__LINE__, pfunc, ca_ni_num_intf);
	for(i=0;i<ca_ni_num_intf;i++){
		ret = nic_register_rxfunc_by_port(i, pfunc);
		if(ret){
			return ret;
		}
	}

	return 0;
}

void nic_reset_rxfunc_to_default(void)
{
	unsigned int i;
	
	printk(KERN_INFO "[%s-%d] ca_ni_num_intf=%d\n", __func__,__LINE__, ca_ni_num_intf);
	for(i=0;i<ca_ni_num_intf;i++){
		nic_register_rxfunc_by_port(i, netif_receive_skb);
	}
}
#endif

#if defined(CONFIG_RTK_NIC_TX_HOOK)
int nic_register_txfunc(p2tfunc_t pfunc)
{
	ca_eth_private_t *cep;
	int i;

	if(pfunc == NULL)
	{
		printk(KERN_ERR "%s: Cannot register NULL function pointer, aborting.\n", __func__);
		return -1;
	}
	
	printk(KERN_INFO "[%s-%d] txfunc=%p\n", __func__, __LINE__, pfunc);
	for(i=0 ; i<CA_NI_MAX_INTF ; i++)
	{
		if(ni_info_data.dev[i] == NULL)
			continue;
		
		cep = netdev_priv(ni_info_data.dev[i]);
		cep->txfunc = pfunc;
	}
	
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	for(i=0 ; i<MAX_CA_NI_MULTI_LAN_DEV_NUM ; i++)
	{
		if(ca_ni_multi_lan_dev_table[i].dev_instant == NULL)
			continue;
		
		cep = netdev_priv(ca_ni_multi_lan_dev_table[i].dev_instant);
		cep->txfunc = pfunc;
	}
#endif
	
	return 0;
}

int nic_register_txfunc_default(void)
{
	ca_eth_private_t *cep;
	int i;
	
	printk(KERN_INFO "[%s-%d] txfunc=%p\n", __func__, __LINE__, ca_ni_start_xmit);
	for(i=0 ; i<CA_NI_MAX_INTF ; i++)
	{
		if(ni_info_data.dev[i] == NULL)
			continue;
		
		cep = netdev_priv(ni_info_data.dev[i]);
		cep->txfunc = ca_ni_start_xmit;
	}

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	for(i=0 ; i<MAX_CA_NI_MULTI_LAN_DEV_NUM ; i++)
	{
		if(ca_ni_multi_lan_dev_table[i].dev_instant == NULL)
			continue;
		
		cep = netdev_priv(ca_ni_multi_lan_dev_table[i].dev_instant);
		cep->txfunc = ca_ni_start_xmit;
	}
#endif

	return 0;
}
#endif

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
static int ca_ni_is_multi_lan_device(struct net_device *dev)
{
	unsigned int totalDev = TOTAL_CA_NI_MULTI_LAN_DEV_NUM;
	char* dev_name;
	int i;

	for (i = 0; i < totalDev; i++) {
		if (ca_ni_multi_lan_dev_table[i].dev_instant) {
			dev_name = ca_ni_multi_lan_dev_table[i].dev_instant->name;
		}
		else {
			printk("no dev_instant, strange.......\n");
			dev_name = ca_ni_multi_lan_dev_table[i].ifname;
		}
		if (!strcmp(dev_name, dev->name)) {
			return 1;
		}
	}
	return 0;
}

static int ca_ni_register_multi_lan_device(ca_eth_private_t *cep)
{
	unsigned int totalDev = TOTAL_CA_NI_MULTI_LAN_DEV_NUM;
	ca_port_cfg_info_t *port_cfg;
	struct net_device *dev, *dev2;
	ca_eth_private_t *new_cep;
	int rc;
	int i, j;

	// allocate and register other device
	for (i = 0; i < totalDev; i++) {
#ifdef CONFIG_NET_SCH_MULTIQ
		dev = alloc_etherdev_mq(sizeof(ca_eth_private_t), QUEUE_PER_INSTANCE);
#else
		dev = alloc_etherdev(sizeof(ca_eth_private_t));
#endif
		if (!dev) {
			printk("%s %d alloc_etherdev fail!\n", __func__, __LINE__);
			return -1;
		}

		/* read MAC address from EEPROM */
		for (j = 0; j < 3; j++)
			((u16 *) (dev->dev_addr))[j] = j;

		port_cfg = &port_cfg_infos[1];
		/* fill all of the items of cep */
		new_cep = netdev_priv(dev);
		new_cep->dev = cep->dev;
		new_cep->pdev = cep->pdev;
		new_cep->port_cfg = *port_cfg;
		new_cep->rx_checksum = CA_ENABLE;
		new_cep->an_bng_mac_idx = 0xffff;
		new_cep->an_bng_mac_idx_pe0 = 0xffff;
		new_cep->an_bng_mac_idx_pe1 = 0xffff;

		/* add a cpu_port for recognize */
#if 1 //RTK R0.7.29.0 workaround
		new_cep->cpu_port[0] = i;
#else
		new_cep->cpu_port = i;
#endif
		/* msg_enable can be get/set by ethtool for debug message level */
		new_cep->msg_enable = netif_msg_init(debug, default_msg);
#if 1 //RTK R0.7.29.0 workaround
		new_cep->napi[0].dev = dev;
#else
		new_cep->napi.dev = dev;
#endif
		new_cep->use_default_lso_para0 = 0;
		new_cep->deep_q = 0;
		new_cep->ni_driver_state = 1;

		/* fill all of the items of dev */
		dev->netdev_ops = &ca_ni_multi_lan_netdev_ops;
		ca_ni_set_ethtool_ops(dev);

		/* disable features for fragment that boa can login*/

#if defined(CONFIG_NE_MANAGEMENT_MODE)
			dev->features = /*NETIF_F_SG | */NETIF_F_RXCSUM/* | NETIF_F_UFO*/;
#else
			dev->features = /*NETIF_F_SG | */NETIF_F_HW_CSUM | NETIF_F_RXCSUM/* | NETIF_F_UFO*/;
#endif
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		dev->features |= NETIF_F_SG; // for FC enable device feature
#endif

		dev->hw_features = dev->features /*| NETIF_F_TSO | NETIF_F_TSO6*/ ;
		///   dev->vlan_features
		dev->irq = cep->port_cfg.irq;
		dev->watchdog_timeo = CA_NI_TX_TIMEOUT;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		dev->features |= NETIF_F_GRO; // for FC enable device feature
#endif
		/*dev->features |= NETIF_F_GRO;*/
		
		ca_ni_multi_lan_dev_table[i].dev_instant = dev;
		memset(dev->name, 0, IFNAMSIZ);
		memcpy(dev->name, ca_ni_multi_lan_dev_table[i].ifname, strlen(ca_ni_multi_lan_dev_table[i].ifname));

		switch(ca_ni_multi_lan_dev_table[i].ifflag)
		{
			case CA_NI_ELAN:
					dev->priv_flags = IFF_DOMAIN_ELAN;
					break;
			case CA_NI_WAN:
					dev->priv_flags = IFF_DOMAIN_WAN;
					break;
			case CA_NI_WLAN:
					dev->priv_flags = IFF_DOMAIN_WLAN;
					break;
			case CA_NI_SVAP:		   //for slave VAP interfaces, bridge should not send query for each, just need one for slave's br0
					dev->priv_flags = IFF_DOMAIN_WLAN | IFF_DOMAIN_WAN;
					break;
			default:
					printk("Error! Should not go here!\n");
		}

#ifndef CONFIG_ARCH_RTL8198F
		if(strstr(dev->name, "eth0"))
			netif_carrier_off(dev);
#endif

		ca_ni_set_ethtool_ops(dev);
		if (strcmp(dev->name, "nas0")) {
			rc = register_netdev(dev);
			if (rc) {
				printk("%s %d rc = %d\n", __func__, __LINE__, rc);
				return -1;
			}
			printk (KERN_INFO "%s: %s, "
				"%02x:%02x:%02x:%02x:%02x:%02x, "
				"IRQ %d\n",
				dev->name,
				"CA NI",
				dev->dev_addr[0], dev->dev_addr[1],
				dev->dev_addr[2], dev->dev_addr[3],
				dev->dev_addr[4], dev->dev_addr[5],
				dev->irq);
		} else if ((dev2 = dev_get_by_name(&init_net,"nas0"))) {
			ca_ni_multi_lan_dev_table[i].dev_instant = dev2;
			dev2->priv_flags = IFF_DOMAIN_WAN;
			free_netdev(dev);
		}
	}
	return 0;
}
#endif

#if (defined(CONFIG_RTL_MULTI_LAN_DEV) || defined(CONFIG_GPON_FEATURE))
#include "ca_event.h"
#endif
#if defined(CONFIG_RTL_MULTI_LAN_DEV) //register function to update carrier state
ca_uint32_t rtk_g3_ethlink_change_set_carrier(ca_device_id_t device_id, ca_uint32_t event_id, ca_void_t *event, ca_uint32_t len, ca_void_t *cookie)
{
	ca_event_port_link_t *msg = (ca_event_port_link_t *)event;
	ca_eth_private_t *rootCep, *cep;
	int i, j;
	ca_uint32_t logic_port_id=0;
	CA_PORT_ID_CHECK(msg->port_id);
	logic_port_id = PORT_ID(msg->port_id);


#if !defined(CONFIG_NE_MANAGEMENT_MODE)
	{
		rootCep = netdev_priv(ni_info_data.dev[device_id]);
		if(!(rootCep)){
			printk(KERN_INFO "Null data\n");
			return CA_EVT_ERROR;
		}
	}
#endif

	if (!netif_carrier_ok(rootCep->port2dev[logic_port_id]) && msg->status == CA_PORT_LINK_UP)
	{
//		printk("[%s:%d] set device %s : %u carrier on\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name, netif_carrier_ok(rootCep->port2dev[logic_port_id]));
		netif_carrier_on(rootCep->port2dev[logic_port_id]);
	}
	else if (netif_carrier_ok(rootCep->port2dev[logic_port_id]) && msg->status == CA_PORT_LINK_DOWN)
	{
//		printk("[%s:%d] set device %s : %u carrier off\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name, netif_carrier_ok(rootCep->port2dev[logic_port_id]));
		netif_carrier_off(rootCep->port2dev[logic_port_id]);
	}
//	else
//		printk("[%s:%d] set device %s carrier error\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name);

//	ca_printf("[%s:%d]INFO: Device %d Ethernet Port %u, %u status change %d.\n", __func__, __LINE__, device_id, msg->port_id, logic_port_id, msg->status);
}
#endif

#if defined(CONFIG_GPON_FEATURE) //register function to update GPON wan carrier state
ca_uint32_t rtk_g3_gmacintr_notifier_onu_state_change(ca_device_id_t device_id, ca_uint32_t event_id, ca_void_t *event, ca_uint32_t len, ca_void_t *cookie)
{
	ca_event_omcc_link_t *msg = (ca_event_omcc_link_t *)event;
    ca_eth_private_t *rootCep, *cep;
	int i, j;
	ca_uint32_t logic_port_id=0;
	CA_PORT_ID_CHECK(msg->port_id);
	logic_port_id = PORT_ID(msg->port_id);

#if !defined(CONFIG_NE_MANAGEMENT_MODE)
	{
		rootCep = netdev_priv(ni_info_data.dev[device_id]);
		if(!(rootCep)){
			printk(KERN_INFO "Null data\n");
			return CA_EVT_ERROR;
		}
	}
#endif
	if (!netif_carrier_ok(rootCep->port2dev[logic_port_id]) && msg->status == CA_OMCC_LINK_UP)
	{
//		printk("[%s:%d] set device %s : %u carrier on\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name, netif_carrier_ok(rootCep->port2dev[logic_port_id]));
		netif_carrier_on(rootCep->port2dev[logic_port_id]);
	}
	else if (netif_carrier_ok(rootCep->port2dev[logic_port_id]) && msg->status == CA_OMCC_LINK_DOWN)
	{
//		printk("[%s:%d] set device %s : %u carrier off\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name, netif_carrier_ok(rootCep->port2dev[logic_port_id]));
		netif_carrier_off(rootCep->port2dev[logic_port_id]);
	}
//	else
//		printk("[%s:%d] set device %s carrier error\n", __func__, __LINE__, rootCep->port2dev[logic_port_id]->name);
	
//	ca_printf("[%s:%d]INFO: Device %d PON Port %u, %u status change %d.\n", __func__, __LINE__, device_id, msg->port_id, logic_port_id, msg->status);
}
#endif


#ifdef CONFIG_ARCH_CORTINA_G3HGU
extern void ca_pon_reset(ca_device_id_t device_id);
#endif

#if defined(CONFIG_NE_PON)
extern int ca77xx_pon_probe(struct platform_device *pdev);
extern int ca77xx_pon_remove(struct platform_device *pdev);
#endif

#if 0
/*
 * Handles PTP timer rollover interrupts.
 */
static irqreturn_t ca_ni_ptp_timer_rollover_interrupt(int irq, void *dev_instance)
{
#if 0
        PTP_INTERRUPT_t status;

        status.wrd = aal_ptp_get_timer_rollover_int_status();

        if (ca_ni_debug & NI_DBG_IRQ) {
                printk("%s: status=0x%x\n", __func__, status.wrd);
        }

        aal_ptp_timer_rollover_int_enable(0);

        if (status.bf.timer_rollover_intI) {
                ca_ni_ptp_local_clock.sec++;
                ca_ni_ptp_local_clock.nsec = aal_ptp_get_timer();

                if (ca_ni_debug & NI_DBG_IRQ) {
                        printk("%s: clock.sec=%lld, clock.nsec=%ld\n", __func__, ca_ni_ptp_local_clock.sec, ca_ni_ptp_local_clock.nsec);
                }
        }


        /* release interrupt */
        aal_ptp_set_timer_rollover_int_status(status.wrd);

        aal_ptp_timer_rollover_int_enable(1);

        if (ca_ni_debug & NI_DBG_IRQ) {
                printk("%s: irq handled\n", __func__);
        }
#endif

        return IRQ_HANDLED;
}

static void ca_ni_ptp_init(struct platform_device *pdev)
{
        int irq, retval;
        static int ptp_init_done = 0;
        struct device_node *parent_np = pdev->dev.of_node;
        struct device_node *np = of_get_child_by_name(parent_np, "pon_ictl");

        if (ptp_init_done)
                return;

        printk("%s: np=0x%p\n", __func__, np);
        if (np == NULL) {
                return;
        }

        /* get ptp timer rollover irq number */
        irq = irq_of_parse_and_map(np, 0);
        printk("%s: timer rollover irq=%d\n", __func__, irq);

        retval = request_irq(irq, ca_ni_ptp_timer_rollover_interrupt, IRQF_SHARED, "ptp_rollover", &(pdev->dev));
        if (retval != 0 && retval != -EBUSY) {
                printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, irq, retval);
        }

#if 0
        do_gettimeofday(&now);
        printk("%s: now.tv_sec=%ld, sizeof(now.tv_sec)=%d\n", __func__, now.tv_sec, sizeof(now.tv_sec));
        ca_ni_ptp_local_clock.sec = now.tv_sec;

        aal_ptp_timer_rollover_int_enable(1);
        aal_ptp_tx_timestamp_int_enable(1);
#endif

        ptp_init_done = 1;
}
#endif

static int ca_ni_init_module_probe(struct platform_device *pdev)
{
	int i, ret;
	ca_eth_private_t *cep = platform_get_drvdata(pdev);
#ifdef CONFIG_OF
	struct device_node *np = pdev->dev.of_node;
#endif
	ca_init_err_t err_code;

	pr_notice("%s %s\n", DRV_NAME, DRV_VERSION);
#if defined(CONFIG_RTK_NIC_RX_HOOK)
    extern int nic_rxhook_init(void);
#endif
#if defined(CONFIG_RTK_NIC_TX_HOOK)
	extern int nic_txhook_init(void);
#endif
	if (ni_initialized)
		return 0;

	if ((ret = ca_ne_reg_init(pdev)) != 0) {
		printk(KERN_ERR "%s: Fail to do ca_ne_reg_init(). (ret=%d)\n", __func__, ret);
		return -ENXIO;
	}

#ifdef CONFIG_NE_PON
	ca77xx_pon_probe(pdev);
#endif

	if (!SOC_HAS_NI()) {
		printk(KERN_ERR "%s: No NI hardware module, fail to load driver.\n", __func__);
		return -ENODEV;
	}

	ca_mut_init();

#ifdef CONFIG_OF
	ca_ni_global_reset(pdev);
#endif

#ifdef CONFIG_ARCH_CORTINA_G3HGU
	ca_pon_reset(CA_DEV_WAN);
#endif

	memset(&ni_info_data, 0, sizeof(ni_info_data));

	/* record pdev for later use */
	ni_info_data.pdev = pdev;

	/* load init config and call ca_init() */
	ca_load_init(&ca_ni_init_config, sizeof(ca_g3_init_config_t));

	ret = ca_init(0, (ca_init_config_t *)&ca_ni_init_config, &err_code);
	if (ret == CA_E_OK) {
		printk("%s: ca_init() is done successfully!\n", __func__);
	} else {
		printk("%s: ERROR! ca_init() fails. (ret=%d, err_code=%d)\n", __func__, ret, err_code);
		return -ENODEV;
	}

	aal_l3qm_eq_get_cpu_pool0_1_start_addr(&ca_ne_cpu_pool0_start_addr, &ca_ne_cpu_pool1_start_addr);

	/* enable refill threshold interrupts for CPU 64 */
	aal_l3qm_setup_refill_threshold_interrup();

#if defined(CONFIG_NE_PHY_CALIBRATION)
	printk("%s: call phy_cal_load()......\n", __func__);
	phy_cal_load(&(pdev->dev));
#endif

	ca_ni_init_cpu_ports(pdev);

	/// init a reset task
	INIT_WORK(&ni_info_data.reset_task, ca_ni_reset_task);


	/* should after ca_ni_init_cpu_ports(), to wait the net_device be created */
	/* initialize CPU EPP FIFO and Empty Buffer Pools */
	if (ca_ni_cpu64_use_fbm)
	{
		ca_uint_t virt_start_addr;
		ca_uint32_t entry_size;
		ca_uint32_t entry_count;
		ca_uint8_t fbm_pool_id;

		aal_l3qm_eq_get_cpu_pool0_info(&virt_start_addr, &entry_size, &entry_count, &fbm_pool_id);
		printk("%s: entry_count=%d\n", __func__, entry_count);
		ca_ni_fill_eq_buf_pool(ni_info_data.dev[0], 0, entry_count);
	}
	else
		ca_ni_fill_eq_buf_pool(ni_info_data.dev[0], 0, aal_l3qm_eq_get_cpu_pool_bid_count());
	wmb();
	printk("%s: ======= after CPU pool bid count=%d =======\n", __func__, aal_l3qm_eq_get_cpu_pool_bid_count());

	ni_info_data.refill_req_8 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_8);        /* QM EQ refill threshold interrupt */
	ni_info_data.refill_req_9 = irq_of_parse_and_map(np, CA_NI_IRQ_QM_CPU_REFILL_REQ_9);        /* QM EQ refill threshold interrupt */
	printk("%s: ni_info_data.refill_req_8=%d, ni_info_data.refill_req_9=%d\n", __func__,
			ni_info_data.refill_req_8, ni_info_data.refill_req_9);

#if defined(CONFIG_NE_CPU256)
	ca_ni_init_cpu256_module_probe(pdev);
#endif

	ca_ni_init_tx_dma_lso(pdev);

	/* add NAPI for cpu ports */
	for ( i = 0; i < ca_ni_num_intf; i++) {
		int j, cpu_port;

		cep = netdev_priv(ni_info_data.dev[i]);
		for (j = 0; j < cep->num_cpu_port; j++) {
			cpu_port = cep->cpu_port[j];
			switch (cpu_port) {
				case 0:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_0, ca_ni_napi_budget);
					break;
				case 1:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_1, ca_ni_napi_budget);
					break;
				case 2:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_2, ca_ni_napi_budget);
					break;
				case 3:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_3, ca_ni_napi_budget);
					break;
				case 4:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_4, ca_ni_napi_budget);
					break;
				case 5:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_5, ca_ni_napi_budget);
					break;
				case 6:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_6, ca_ni_napi_budget);
					break;
				case 7:
					netif_napi_add(ni_info_data.dev[i], &cep->napi[j], ca_ni_poll_7, ca_ni_napi_budget);
					break;
			}
		}
	}

#ifdef CONFIG_NE_PTP_CLOCK
        ca_ni_ptp_init(pdev);
#endif

#ifdef CONFIG_PROC_FS
	ca_ni_proc_init();
	ca_aal_proc_init();
	ca_api_proc_init();
#endif

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
	/* create NETLINK */
	ca_ni_netlink_rx_init();
#endif

	/* register_netdev() should be the last one */
	ca_ni_register_netdev();

	spin_lock_init(&ca_ni_rx_refill_lock);

#ifdef CONFIG_NE_L2FP
	ca_l2fp_init();
#endif

#ifdef CONFIG_CORTINA_CPU_TAG
	ca_ni_cpu_tag_init(CA_ENABLE, RTK_NI_PORT_4, RTK_TAG_PROFILE_0);
#endif


#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#if 1
#if defined(CONFIG_NE_MANAGEMENT_MODE)
		cep = netdev_priv(ni_info_data.dev[CPU_PORT0_INST]);
		ca_ni_register_multi_lan_device(cep);
		ca_ni_change_dev_port_mapping(cep, LAN_PORT1, "eth0.2");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT2, "eth0.3");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT3, "eth0.4");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT4, "eth0.5");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT5, "eth0.6");
		ca_ni_change_dev_port_mapping(cep, WAN_PORT1, "nas0");
#else
		for ( i = 0; i < ca_ni_num_intf; i++) {
			cep = netdev_priv(ni_info_data.dev[i]);
			switch (i) {
				case 0:
					ca_ni_register_multi_lan_device(cep);
				case 1 ... 7:
					ca_ni_change_dev_port_mapping(cep, LAN_PORT1, "eth0.2");
					ca_ni_change_dev_port_mapping(cep, LAN_PORT2, "eth0.3");
					ca_ni_change_dev_port_mapping(cep, LAN_PORT3, "eth0.4");
					ca_ni_change_dev_port_mapping(cep, LAN_PORT4, "eth0.5");
					ca_ni_change_dev_port_mapping(cep, LAN_PORT5, "eth0.6");
					ca_ni_change_dev_port_mapping(cep, WAN_PORT1, "nas0");
#if defined(CONFIG_RTK_PE_FC_SUPPORT)
					ca_ni_change_dev_port_mapping(cep, PE_PORT, "eth0.16");
#endif
					break;
			}
		}
#endif //CONFIG_NE_MANAGEMENT_MODE
#else
	//keep lspid unchange	
	ca_uint8_t keep=1;
	if(aal_l3fe_keep_lspid_unchange_set(0, &keep) == CA_E_OK)
	{
		printk(KERN_INFO "%s: #####Set keep_lspid_unchange to %u#####\n", __func__,keep);
		if (ca_ni_mgmt_mode) {
		cep = netdev_priv(ni_info_data.dev[CPU_PORT0_INST]);
		ca_ni_register_multi_lan_device(cep);
		ca_ni_change_dev_port_mapping(cep, LAN_PORT1, "eth0.2");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT2, "eth0.3");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT3, "eth0.4");
		ca_ni_change_dev_port_mapping(cep, LAN_PORT4, "eth0.5");
		ca_ni_change_dev_port_mapping(cep, WAN_PORT1, "nas");
		} else {
			for ( i = 0; i < ca_ni_num_intf; i++) {
				cep = netdev_priv(ni_info_data.dev[i]);
				switch (i) {
					case 0:
						ca_ni_register_multi_lan_device(cep);
					case 1 ... 7:
						ca_ni_change_dev_port_mapping(cep, LAN_PORT1, "eth0.2");
						ca_ni_change_dev_port_mapping(cep, LAN_PORT2, "eth0.3");
						ca_ni_change_dev_port_mapping(cep, LAN_PORT3, "eth0.4");
						ca_ni_change_dev_port_mapping(cep, LAN_PORT4, "eth0.5");
						ca_ni_change_dev_port_mapping(cep, WAN_PORT1, "nas");
						break;
				}
			}
		}
	}
	else
	{
		printk(KERN_INFO "%s: @@@@@Set keep_lspid_unchange to %u FAIL@@@@@\n", __func__,keep);
	}
#endif
#endif


#if defined(CONFIG_RTL_MULTI_LAN_DEV) //register function to update carrier state
	if((ret = ca_event_register(0, CA_EVENT_ETH_PORT_LINK, rtk_g3_ethlink_change_set_carrier, NULL))!=CA_E_OK)
	{
		printk(KERN_INFO "[%s:%d]: @@@@@Set ca_event_register CA_EVENT_ETH_PORT_LINK FAIL@@@@@\n", __func__, __LINE__);
	}
#endif

#if defined(CONFIG_GPON_FEATURE) //register function to update GPON WAN carrier state
	if((ret = ca_event_register(0, CA_EVENT_OMCC_LINK, rtk_g3_gmacintr_notifier_onu_state_change, NULL))!=CA_E_OK)
	{
		printk(KERN_INFO "[%s:%d]: @@@@@Set ca_event_register CA_EVENT_OMCC_LINK FAIL@@@@@\n", __func__, __LINE__);
	}
#endif

#ifdef CONFIG_RTK_NIC_RX_HOOK
	/*init nic rx hook*/
	nic_rxhook_init();
#endif
#ifdef CONFIG_RTK_NIC_TX_HOOK
	/*init nic tx hook*/
	nic_txhook_init();
#endif

	rtk_nic_cpu_us_mib_clear();


#ifdef CONFIG_NE_KERNEL_HOOK
	ca_kernel_hook_init();
#endif

	ni_initialized = 1;

	return 0;
}

static int ca_ni_cleanup_module_remove(struct platform_device *pdev)
{
	int i;
	ni_info_t *ni_info;
	ca_eth_private_t *cep = platform_get_drvdata(pdev);

	ni_info = &ni_info_data;

	/* free the memory allocated for DMA LSO */
	ca_ni_cleanup_tx_dma_lso(pdev);

	for (i = 0; i < ca_ni_num_intf; i++) {

#ifdef CONFIG_CA_NE_VIRTUAL_NI
		ca_ni_virt_ni_remove_if(i);
#endif
		cep = netdev_priv(ni_info_data.dev[i]);
		unregister_netdev(cep->dev);
		free_netdev(ni_info_data.dev[i]);
	}

#ifdef CONFIG_PROC_FS
	ca_ni_proc_exit();
	ca_aal_proc_exit();
	ca_api_proc_exit();
#endif

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
	/* unregister NETLINK */
	ca_ni_netlink_rx_exit();
#endif

#ifdef CONFIG_NE_L2FP
	ca_l2fp_cleanup();
#endif

#if defined(CONFIG_NE_PON)
	ca77xx_pon_remove(pdev);
#endif

#ifdef CONFIG_NE_KERNEL_HOOK
	ca_kernel_hook_exit();
#endif

	return 0;
}

#ifdef CONFIG_PM
static int ca_ni_resume(struct platform_device *pdev)
{
	return 0;
}

static int ca_ni_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}
#else /* CONFIG_PM */
#define ca_ni_resume	NULL
#define ca_ni_suspend	NULL
#endif /* CONFIG_PM */

#include <linux/init.h>
/* ca_ne_reg_xxx() should move to cs_init() */
static void __iomem *ca_ne_reg_base = NULL;
static void __iomem *ca_ne_glb_ne_intr_reg_base = NULL;
static void __iomem *ca_xram_reg_base = NULL;
static void __iomem *ca_dma_lso_reg_base = NULL;
static void __iomem *ca_per_mdio_reg_base = NULL;

#ifndef CONFIG_CORTINA_BOARD_FPGA
static void __iomem *ca_ne_phy_reg_base = NULL;
#endif

#if defined(CONFIG_CA_NE_AAL_XFI)
static void __iomem *ca_ne_xfi_reg_base = NULL;
#endif

#define CA_NI_BASE_ADDR (L2FE_GLB_VER_CTRL & 0xFFFF0000)
#define CA_GLB_BASE_ADDR GLOBAL_JTAG_ID
#define CA_XRAM_BASE_ADDR CA_NI_XRAM_BASE

#ifdef CONFIG_OF
#define CA_NI_MEM_RESOURCE_NI_REGS      0
#define CA_NI_MEM_RESOURCE_GLOBAL_NETWORK_ENGINE_INTERRUPT_REGS  1
#define CA_NI_MEM_RESOURCE_MDIO_REGS    2
#define CA_NI_MEM_RESOURCE_XRAM         3
#define CA_NI_MEM_RESOURCE_DMA_LSO      4
#define CA_NI_MEM_RESOURCE_SRAM         5
#define CA_NI_MEM_RESOURCE_DDR_COHERENT          6
#define CA_NI_MEM_RESOURCE_DDR_NONCACHE          7

/*Need update map the value in device tree DingYi*/
#define CA_NI_MEM_RESOURCE_PON                   8
#define CA_NI_MEM_RESOURCE_PON_GLB               9
#define CA_NI_MEM_RESOURCE_PHY_REGS		 11	

#define CA_VENUS_PERI_BASE_ADDR PER_SOFT_RESET&0xFFFFF000


#if defined(CONFIG_CA_NE_AAL_XFI)
#define CA_NI_MEM_RESOURCE_XFI_REGS     10
#endif

#define	CA_NI_MEM_RESOURCE_AXI_REO_REGS	(10)
static void __iomem *ca_ne_axi_reo_reg_base = NULL;

static const struct of_device_id cortina_ni_of_match[];
#endif

static int ca_ne_reg_init(struct platform_device *pdev)
{
	int ret;
	ca_uint_t ddr_coherent_base_addr;

	/* get the resource map from device tree for NI/L2FE/L3FE/TQM module registers */
#if CONFIG_OF /* DT support */
	/* assign DT node pointer */
	struct device_node *np = pdev->dev.of_node;
	struct resource mem_resource;
	const struct of_device_id *match;

	/* search DT for a match */
	match = of_match_device(cortina_ni_of_match, &pdev->dev);
	if (!match)
		return -EINVAL;

	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_NI_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_NI_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for NI registers=%pR\n", __func__, &mem_resource);

	ca_ne_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_reg_base) {
		printk("%s: ioremap(CA_NI_BASE_ADDR=0x%x) 1M failed!!!\n", __func__, CA_NI_BASE_ADDR);
		return -ENOMEM;
	}

	printk("%s: ca_ne_reg_base=0x%p\n", __func__, ca_ne_reg_base);

	/* search DT for a match */
	match = of_match_device(cortina_ni_of_match, &pdev->dev);
	if (!match)
		return -EINVAL;

	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_GLOBAL_NETWORK_ENGINE_INTERRUPT_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_GLOBAL_NETWORK_ENGINE_INTERRUPT_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for GLOBAL NETWORK ENGINE INTRRUPT registers=%pR\n", __func__, &mem_resource);

	ca_ne_glb_ne_intr_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_glb_ne_intr_reg_base) {
		printk("%s: GLOBAL NETWORK ENGINE INTRRUPT registers ioremap failed!!!\n", __func__);
		return -ENOMEM;
	}

	printk("%s: ca_ne_glb_ne_intr_reg_base=0x%p\n", __func__, ca_ne_glb_ne_intr_reg_base);

	aal_ne_reg_base_init(ca_ne_reg_base);
	aal_glb_ne_intr_reg_base_init(ca_ne_glb_ne_intr_reg_base);

	/* get the resource map from device tree for PER_MDIO module registers */
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_MDIO_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_MDIO_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for MDIO registers=%pR\n", __func__, &mem_resource);

	ca_per_mdio_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_per_mdio_reg_base) {
		printk("%s: MDIO registers ioremap() failed!!!\n", __func__);
		return -ENOMEM;
	}

	printk("%s: ca_per_mdio_reg_base=0x%p\n", __func__, ca_per_mdio_reg_base);
	aal_per_mdio_reg_base_init(ca_per_mdio_reg_base);

	/* get the resource map from device tree for XRAM */
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_XRAM, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_XRAM) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for XRAM=%pR\n", __func__, &mem_resource);
	ca_xram_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_xram_reg_base) {
		printk("%s: ioremap(CA_XRAM_BASE_ADDR=0x%x) SZ_8K failed!!!\n", __func__, CA_XRAM_BASE_ADDR);
		return -ENOMEM;
	}
	printk("%s: ca_xram_reg_base=0x%p\n", __func__, ca_xram_reg_base);
	aal_xram_reg_base_init(ca_xram_reg_base);

	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_DMA_LSO, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_DMA_LSO) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for DMA LSO registers=%pR\n", __func__, &mem_resource);

	ca_dma_lso_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_dma_lso_reg_base) {
		printk("%s: DMA LSO ioremap() failed!!!\n", __func__);
		return -ENOMEM;
	}

	printk("%s: ca_dma_lso_reg_base=0x%p\n", __func__, ca_dma_lso_reg_base);
	aal_dma_reg_base_init(ca_dma_lso_reg_base);


	/* get the resource map from device tree for SRAM */
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_SRAM, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_SRAM) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for SRAM=%pR\n", __func__, &mem_resource);
	ca_ne_sram_phy_base = mem_resource.start;
	ca_ne_sram_size = resource_size(&mem_resource);
	if (mem_resource.end > (CA_NI_SRAM_BASE_ADDR + CA_NI_SRAM_SIZE)) {
		printk("%s: out of SRAM range 0x%p NE driver initialized failed!!!\n", __func__, (void *)mem_resource.end);
		BUG_ON(0);
	}
	printk("%s: ca_ne_sram_phy_base=0x%llx end 0x%p size 0x%llx\n", __func__, (ca_uint64_t)ca_ne_sram_phy_base, (void *)mem_resource.end, resource_size(&mem_resource));
	ca_ne_sram_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_sram_base) {
		printk("%s: SRAM memory ioremap() failed!!!\n", __func__);
		return -ENOMEM;
	}

	printk("%s: ca_ne_sram_base=0x%p\n", __func__, ca_ne_sram_base);

	/* get the resource map from device tree for DDR coherent */
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_DDR_COHERENT, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_DDR_COHERENT) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for DDR=%pR\n", __func__, &mem_resource);

	ddr_coherent_base_addr = mem_resource.start;
	ca_ne_ddr_coherent_base = (void __iomem *)ddr_coherent_base_addr;
	ca_ne_ddr_coherent_phy_base = ddr_coherent_base_addr;
	ca_ne_ddr_coherent_reserved_size = mem_resource.end - mem_resource.start + 1;
	printk("%s: ca_ne_ddr_coherent_reserved_size=0x%x\n", __func__, ca_ne_ddr_coherent_reserved_size);
	printk("%s: ca_ne_ddr_coherent_base=0x%p, ca_ne_ddr_coherent_phy_base=0x%llx\n",
		__func__, ca_ne_ddr_coherent_base, (ca_uint64_t)ca_ne_ddr_coherent_phy_base);

	/* get the resource map from device tree for DDR non-cache  */
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_DDR_NONCACHE, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_DDR_NONCACHE) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for DDR non-cache=%pR\n", __func__, &mem_resource);

	ca_ne_ddr_noncache_phy_base = mem_resource.start;

	ca_ne_ddr_noncache_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_ddr_noncache_base) {
		printk("%s: DDR noncache memory ioremap() failed!!!\n", __func__);
		return -ENOMEM;
	}

	ca_ne_ddr_noncache_reserved_size = mem_resource.end - mem_resource.start + 1;
	printk("%s: ca_ne_ddr_noncache_reserved_size=0x%x\n", __func__, ca_ne_ddr_noncache_reserved_size);
	printk("%s: ca_ne_ddr_noncache_base=%p, ca_ne_ddr_noncache_phy_base=0x%llx\n", __func__,
			ca_ne_ddr_noncache_base, (ca_uint64_t)ca_ne_ddr_noncache_phy_base);
#if defined(CONFIG_NE_PON)
	//get venus pon reg database
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_PON, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_PON) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for PON=%pR\n", __func__, &mem_resource);
	ca_venus_pon_reg_iobase = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_venus_pon_reg_iobase) {
		printk("%s: ioremap(CA_NI_MEM_RESOURCE_PON=0x%pap) failed!!!\n", __func__, &mem_resource.start);
		return -ENOMEM;
	}
	printk("ca_venus_pon_reg_iobase = 0x%p\n", ca_venus_pon_reg_iobase);

	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_PON_GLB, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_PON_GLB) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: resource for PONGLB=%pR\n", __func__, &mem_resource);
	ca_venus_pon_glb_reg_iobase = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_venus_pon_glb_reg_iobase) {
		printk("%s: ioremap(CA_NI_MEM_RESOURCE_PON_GLB=0x%pap) failed!!!\n", __func__, &mem_resource.start);
		return -ENOMEM;
	}
	printk("ca_venus_pon_glb_reg_iobase = 0x%p\n", ca_venus_pon_glb_reg_iobase);

	printk("%s: resource for PERI=0x%x\n", __func__, CA_VENUS_PERI_BASE_ADDR);
	ca_venus_peri_reg_iobase = ioremap(CA_VENUS_PERI_BASE_ADDR, 0xfff);
	if (!ca_venus_peri_reg_iobase) {
		printk("%s: ioremap(CA_VENUS_PERI_BASE_ADDR=0x%x) failed!!!\n", __func__, CA_VENUS_PERI_BASE_ADDR);
		return -ENOMEM;
	}
	printk("ca_venus_peri_reg_iobase = 0x%p\n", ca_venus_peri_reg_iobase);

#endif
	/* get "reg" property from DT and convert to axi reoder mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_AXI_REO_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_AXI_REO_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for AXI REO registers=%pR\n", __func__, &mem_resource);

	ca_ne_axi_reo_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_axi_reo_reg_base) {
		printk("%s: ioremap(CA_NI_MEM_RESOURCE_AXI_REO_REGS=0x%pap) failed!!!\n", __func__, &mem_resource.start);
		return -ENOMEM;
	}
	printk("%s: ca_ne_axi_reo_reg_base=0x%p\n", __func__, ca_ne_axi_reo_reg_base);
	aal_ne_axi_reo_reg_base_init(ca_ne_axi_reo_reg_base);

#ifndef CONFIG_CORTINA_BOARD_FPGA

	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_PHY_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_PHY_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for ASIC GPHY registers=%pR\n", __func__, &mem_resource);

	ca_ne_phy_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_phy_reg_base) {
		printk("%s: ioremap(CA_NI_MEM_RESOURCE_PHY_REGS=0x%pap) failed!!!\n", __func__, &mem_resource.start);
		return -ENOMEM;
	}

	printk("%s: ca_ne_phy_reg_base=0x%p\n", __func__, ca_ne_phy_reg_base);

	aal_ne_phy_reg_base_init(ca_ne_phy_reg_base);

#if defined(CONFIG_CA_NE_AAL_XFI)
	/* get "reg" property from DT and convert to platform mem address resource */
	ret = of_address_to_resource(np, CA_NI_MEM_RESOURCE_XFI_REGS, &mem_resource);
	if (ret) {
		printk("%s: of_address_to_resource(CA_NI_MEM_RESOURCE_XFI_REGS) return %d\n", __func__, ret);
		return ret;
	}

	printk("%s: mem_resource for XFI registers=%pR\n", __func__, &mem_resource);

	ca_ne_xfi_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
	if (!ca_ne_xfi_reg_base) {
		printk("%s: ioremap(CA_NI_MEM_RESOURCE_XFI_REGS=0x%pap) failed!!!\n", __func__, &mem_resource.start);
		return -ENOMEM;
	}

	printk("%s: ca_ne_xfi_reg_base=0x%p\n", __func__, ca_ne_xfi_reg_base);

	aal_ne_xfi_reg_base_init(ca_ne_xfi_reg_base);
#endif
#endif

#endif /* end of CONFIG_OF */

	return 0;
}

void ca_ne_reg_exit(void)
{
	iounmap(ca_ne_reg_base);
	iounmap(ca_ne_sram_base);
	iounmap(ca_ne_ddr_coherent_base);
	iounmap(ca_ne_ddr_noncache_base);
}

struct net_device * ca_ni_virtual_instance_alloc(int cpu_idx, char *name)
{
	struct net_device *dev;
	ca_eth_private_t *cep;
	int dev_idx = cpu_idx;
	aal_ni_hv_glb_internal_port_id_cfg_t ni_port_id_cfg;

#ifdef CONFIG_NET_SCH_MULTIQ
	dev = alloc_etherdev_mq(sizeof(ca_eth_private_t), QUEUE_PER_INSTANCE);
#else
	dev = alloc_etherdev(sizeof(ca_eth_private_t));
#endif

	if (dev == NULL)
		return NULL;

	snprintf(dev->name, IFNAMSIZ, name);
	dev->mtu = 1500;

	ni_info_data.dev[dev_idx] = dev;
	cep = netdev_priv(dev);
	cep->dev = dev;
	/* add a cpu_port for recognize */
	cep->cpu_port[0] = cpu_idx;
	cep->num_cpu_port = 1;
	cep->egress_cpu_port_id = cpu_idx;

	cep->port_cfg = port_cfg_infos[dev_idx];
	cep->use_default_lso_para0 = 1;
	cep->default_lso_para0 = 0;
	cep->deep_q = 1;

	aal_ni_hv_glb_internal_port_id_cfg_get(0, &ni_port_id_cfg);
	if (ni_port_id_cfg.dma_to_l3fe & (1 << dev_idx))
		cep->dma_l3fe = 1;
	else
		cep->dma_l3fe = 0;

	cep->napi[0].dev = dev;
	cep->pdev = ni_info_data.pdev;

	switch (cpu_idx) {
		case 0:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_0, ca_ni_napi_budget);
			break;
		case 1:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_1, ca_ni_napi_budget);
			break;
		case 2:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_2, ca_ni_napi_budget);
			break;
		case 3:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_3, ca_ni_napi_budget);
			break;
		case 4:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_4, ca_ni_napi_budget);
			break;
		case 5:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_5, ca_ni_napi_budget);
			break;
		case 6:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_6, ca_ni_napi_budget);
			break;
		case 7:
			netif_napi_add(dev, &cep->napi[0], ca_ni_poll_7, ca_ni_napi_budget);
			break;
	}

	/* initialize spin locks */
	spin_lock_init(&cep->stats_lock);
	spin_lock_init(&cep->lock);
	return dev;
}

ca_status_t ca_ni_virtual_instance_free(int cpu_idx)
{
	struct net_device *dev;
	//ca_eth_private_t *cep;
	int dev_idx = cpu_idx;

	dev = ni_info_data.dev[dev_idx];
	if (dev) {
		ca_ni_virtual_instance_close(cpu_idx);
		free_netdev(dev);
	}
	return CA_E_OK;
}

ca_status_t ca_ni_virtual_instance_open(int cpu_idx)
{
	struct net_device *dev;
	ca_eth_private_t *cep;
	int dev_idx = cpu_idx;
	int retval;

	dev = ni_info_data.dev[dev_idx];
	if (dev) {
		cep = netdev_priv(dev);

		if (dev->flags & IFF_UP) {
			printk(KERN_ERR "%s: already up flags 0x%x\n", __func__, dev->flags);
			return CA_E_ERROR;
		}

		dev->flags |= IFF_UP;

		/* should dirable RX interrupt first to avoid fake RX interrrupt comes in */
		aal_ni_enable_rx_interrupt(cpu_idx, 0);

		netif_carrier_off(dev);

		/* request irq for this physical port */
		printk("%s: cep->port_cfg.irq=%d\n", __func__, cep->port_cfg.irq[0]);

		retval = request_irq(cep->port_cfg.irq[0], ca_ni_rx_interrupt, 0, dev->name, dev);
		if (retval != 0 && retval != -EBUSY) {
			printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, cep->port_cfg.irq[0], retval);
			return CA_E_ERROR;
		}

		/* should enable EPP interrupt after request_irq() done */
		aal_l3qm_enable_cpu_epp_interrupt(cep->cpu_port[0], 1);

		/* use callback routine to free the transmitted packet if start xmit not be called again */
		if (ca_ni_tx_timer_enable)
			setup_timer(&cep->tx_completion_timer, ca_ni_tx_completion_timer_cb, (unsigned long)dev);

		cep->status = 1;
		cep->ni_driver_state = 1;
		cep->tx_completion_queues = 0;

		/* napi should be enabled before RX interrupt enabled */
		napi_enable(&cep->napi[0]);

		/* Enable Rx interrupts at this point */
		aal_ni_enable_rx_interrupt(cep->cpu_port[0], 1);

		/* enable QM TX function for cpu port */
		aal_l3qm_enable_tx_cpu(cep->cpu_port[0], 1);

		return CA_E_OK;
	}

	return CA_E_ERROR;
}

ca_status_t ca_ni_virtual_instance_close(int cpu_idx)
{
	int i;
	struct net_device *dev;
	ca_eth_private_t *cep;
	int dev_idx = cpu_idx;

	dev = ni_info_data.dev[dev_idx];
	if (dev) {
		cep = netdev_priv(dev);
		dev->flags &= ~IFF_UP;

		/* disable QM TX function for cpu port */
		aal_l3qm_enable_tx_cpu(cep->cpu_port[0], 0);

		aal_ni_enable_rx_interrupt(cep->cpu_port[0], 0);

		/* stop TX queues */
		netif_tx_disable(dev);

		/* disable NAPI */
		for (i = 0; i < cep->num_cpu_port; i++) {
			napi_disable(&cep->napi[i]);
		}

		/* release irq */
		free_irq(cep->port_cfg.irq[0], dev);

		cep->status = 0;
	}
	return CA_E_OK;
}

/*
 * if dest_port == 0xFF, then not bypass L2FE
 * else, bypass L2FE and directly send to physical port
 */
ca_status_t ca_ni_virtual_instance_xmit(int cpu_idx,
		struct sk_buff *skb, ca_ni_tx_config_t *p_tx_config)
{
	struct net_device *dev;
	ca_eth_private_t *cep;
	int dev_idx = cpu_idx;


	dev = ni_info_data.dev[dev_idx];
	if (dev) {
		cep = netdev_priv(dev);
		p_tx_config->core_config.bf.lspid = cpu_idx + AAL_LPORT_CPU_0;
		if (p_tx_config->core_config.bf.ldpid == 0xFF)
			p_tx_config->core_config.bf.ldpid = cep->port_cfg.tx_ldpid; /*0x18 or 0x19*/

		if (__ca_ni_start_xmit(skb, dev, p_tx_config) == NETDEV_TX_OK) {
			ca_skb_in(skb, true);
			return CA_E_OK;
		} else
			return CA_E_RESOURCE;
	}
	return CA_E_NOT_FOUND;
}

/* Match table for of_platform binding */
static const struct of_device_id cortina_ni_of_match[] = {
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

static void ca_ni_recover_from_no_memory(void)
{
        int bid_cnt;
        ca_uint16_t cpu_port;
        ca_uint16_t count;
        ca_uint32_t rd_ptr, wr_ptr;
        struct net_device *dev;

        dev = ni_info_data.dev[0];

        /* dump the read/write pointer for cpu port 0 and 1 */
        wr_ptr = aal_l3qm_get_rx_write_ptr(0, 0, &count);
        rd_ptr = aal_l3qm_get_rx_read_ptr(0, 0);

        if (ca_ni_debug & NI_DBG_NI) {
        printk("%s: cpu port 0, qid 0, rd_ptr=%d, wr_ptr=%d\n", __func__, rd_ptr, wr_ptr);
        }
        wr_ptr = aal_l3qm_get_rx_write_ptr(1, 0, &count);
        rd_ptr = aal_l3qm_get_rx_read_ptr(1, 0);

        if (ca_ni_debug & NI_DBG_NI) {
        printk("%s: cpu port 1, qid 0, rd_ptr=%d, wr_ptr=%d\n", __func__, rd_ptr, wr_ptr);
        }
        for (cpu_port = 0; cpu_port < 2; cpu_port++)
        {
                /* refill EQ buffer pools */
                bid_cnt = aal_l3qm_get_inactive_bid_cntr(l3qm_eq_profile_cpu_pool0_eq_id);
                if (bid_cnt > 0)
                        ca_ni_refill_eq_buf_pool(dev, cpu_port, l3qm_eq_profile_cpu_pool0_eq_id, bid_cnt);

                bid_cnt = aal_l3qm_get_inactive_bid_cntr(l3qm_eq_profile_cpu_pool1_eq_id);
                if (bid_cnt > 0)
                        ca_ni_refill_eq_buf_pool(dev, cpu_port, l3qm_eq_profile_cpu_pool1_eq_id, count);
        }
}

static void ca_ni_enable_qm_interrupts(int enable)
{
        /* disable QM TX and CPU EPP and QM REFILL interrupts */
        aal_l3qm_enable_tx_cpu(0, enable);
        aal_l3qm_enable_tx_cpu(1, enable);
        aal_l3qm_enable_cpu_epp_interrupt(0, enable);
        aal_l3qm_enable_cpu_epp_interrupt(1, enable);
        if (ca_ni_use_qm_eq_refill_int) {
                aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu64_pool0_eq_id, enable);
                aal_l3qm_enable_refill_threshold_interrupt(ca_ni_cpu64_pool1_eq_id, enable);
        }
}

/* allocate skb failed, wait for memory comes back */
static void ca_ni_fill_eq_pool_wq_proc(struct work_struct *wq)
{
        struct sk_buff *skb;
        struct net_device *dev;
        int head_room = aal_l3qm_get_cpu_port_head_room_first();

        dev = ni_info_data.dev[0];
        if (ca_ni_debug & NI_DBG_NI) {
        printk("%s: ca_ni_cpu64_pool0_eq_id=%d, ca_ni_cpu64_pool1_eq_id=%d\n", __func__,
                ca_ni_cpu64_pool0_eq_id, ca_ni_cpu64_pool1_eq_id);
        printk("%s: wait for memory comes back...\n", __func__);
        }
        while (1) {
                skb = ca_netdev_alloc_skb(dev, CA_NI_SKB_PKT_LEN + head_room + 0x100, true);
                /* if memory comes back then back to normal */
                if (skb != NULL) {
                        ca_dev_kfree_skb_any(skb);

                        /* reset read/write pointer and fill empty buffer pool */
                        ca_ni_recover_from_no_memory();

                        /* check again here */
                        skb = ca_netdev_alloc_skb(dev, CA_NI_SKB_PKT_LEN + head_room + 0x100, true);
                        if (skb == NULL) {
                                /* reduce CPU loading */
                                schedule_timeout_interruptible(1);
                                continue;
                        }
                        ca_dev_kfree_skb_any(skb);
                       if (ca_ni_debug & NI_DBG_NI) {
                        printk("%s: Memory comes back, skb=0x%p\n", __func__, skb);
                        printk("%s: ca_ni_use_qm_eq_refill_int=%d\n", __func__, ca_ni_use_qm_eq_refill_int);
                       }
                        /* enable QM TX and CPU EPP and QM REFILL interrupts */
                        ca_ni_enable_qm_interrupts(1);

                        return;
                }
                /* reduce CPU loading */
                schedule_timeout_interruptible(1);
        }
}

static int __init ca_ni_init_module(void)
{
	int ret;

	ret = platform_driver_register(&ca_ni_platform_driver);

	printk("%s: platform_driver_register() return ret=%d\n", __func__, ret);

	/* create a work queue for skb allocate failed */
	ca_ni_fill_eq_pool_wq = create_workqueue("CA_NI_FILL_EQ_POOL_WQ");
	if (!ca_ni_fill_eq_pool_wq) {
		pr_err("could not create workqueue\n");
	}

	return ret;
}

static void __exit ca_ni_cleanup_module(void)
{
	/* need put before platform_driver_unregister() */
	aal_exit();

	platform_driver_unregister(&ca_ni_platform_driver);

	/* destroy work queue for skb allocate failed */
	destroy_workqueue(ca_ni_fill_eq_pool_wq);
}

module_init(ca_ni_init_module);
module_exit(ca_ni_cleanup_module);
