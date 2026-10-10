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
 * ca_l2fp.c: Cortina Access L2 FastPath Managment driver
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/proc_fs.h>
#include <soc/cortina/cortina-soc.h>
#include "osal_pkt.h"
#include "vlan.h"
#include "aal_ni.h"
#include "ca-scfg/scfg.h"
#include "ca_aal.h"
#include "ca_mut.h"
#include "ca_ni.h"
#include "ca_ni_tx.h"
#include "ca_l2fp.h"
#include "ca_adapt_proc.h"

#define CA_NE_L2FP_HELP_MSG \
	"Purpose: Enable L2FP driver debug message\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: L2FP Dev#0 debug message\n" \
	"  ~\n" \
	"  0x00000080: L2FP Dev#7 debug message\n" \
	"  0x00000100: L2FP enable Managment debug message\n" \
	"  0x00000200: L2FP enable TX debug message\n" \
	"  0x00000400: L2FP enable RX debug message\n" \
	"  0x00000800: L2FP dump device entry\n" \
	"  0x00001000: L2FP clear statistic\n" \
	"  0xFFFF0000: L2FP set the length of RX/TX pkt dump\n" \
	"\n"

#define L2FP_DBG_MANAGMENT			0x00000100
#define L2FP_DBG_TX					0x00000200
#define L2FP_DBG_RX					0x00000400
#define L2FP_DBG_DUMP_L2FP_ENTRY	0x00000800
#define L2FP_DBG_CLEAR_STATISTIC	0x00001000
#define L2FP_DBG_L2FP_DEV0			0x00000001
#define L2FP_DBG_L2FP_DEV1			0x00000002
#define L2FP_DBG_L2FP_DEV2			0x00000004
#define L2FP_DBG_L2FP_DEV3			0x00000008
#define L2FP_DBG_L2FP_DEV4			0x00000010
#define L2FP_DBG_L2FP_DEV5			0x00000020
#define L2FP_DBG_L2FP_DEV6			0x00000040
#define L2FP_DBG_L2FP_DEV7			0x00000080
#define L2FP_DBG_L2FP_DEV_ALL		0x000000FF
#define L2FP_DBG_DUMP_LEN_MASK		0xFFFF0000
#define L2FP_DBG_DUMP_LEN_SHIFT		16

#define CPU_PORT_BASE				0x10
struct proc_dir_entry *proc_l2fp = NULL;
ca_uint32_t ca_l2fp_debug;

#define CA_L2FP_DEBUG(idx, t, x)  { \
				if ((ca_l2fp_debug & t) && ((ca_l2fp_debug & idx) || (idx == L2FP_DBG_L2FP_DEV_ALL))) x;\
			}

#define CA_L2FP_DEVICE_MAX			8
ca_uint32_t ca_l2fp_cpu_port_start 	= 0x13;
ca_uint32_t ca_l2fp_cpu_port_end 	= 0x15;
ca_uint32_t ca_l2fp_cpu_port_total 	= 3;

typedef struct {
	ca_l2fp_device_entry_t	dev_info;
	ca_uint8_t				cpu_port; /*from 0x10, 0x11..etc*/
	/*for statistic*/
	ca_uint64_t				rx_packets;
	ca_uint64_t				rx_bytes;
	ca_uint64_t				rx_fail_packets;
	ca_uint64_t				tx_packets;
	ca_uint64_t				tx_bytes;
	ca_uint64_t				tx_fail_packets;
	ca_uint64_t				tx_done_packets;
} ca_l2fp_device_t;

ca_l2fp_device_t * ca_l2fp_dev_list[CA_L2FP_DEVICE_MAX];

#if defined(CONFIG_LUNA_G3_SERIES)
		// skip l2fp rx but return OK to indicate packet belong to wifi fast path.
#else
static void __ca_l2fp_dump_rx(
	ca_uint8_t 					device_id,
	ca_uint32_t 				cpu_port,
	ca_uint32_t 				voq,
	pkt_t * 					pkt) {

	ca_uint32_t i;
	ca_uint32_t len;
	char * data = PKTDATA(pkt);
	if ((ca_l2fp_debug & L2FP_DBG_RX) && (ca_l2fp_debug & (1 << device_id))) {
		len = (ca_l2fp_debug & L2FP_DBG_DUMP_LEN_MASK) >> L2FP_DBG_DUMP_LEN_SHIFT;

		if (len == 0)
			len = PKTLEN(pkt);
		else if (len > PKTLEN(pkt))
			len = PKTLEN(pkt);

		printk("\t RX l2fp[%d] cpu 0x%x voq %d pkt %p pkt_len %d\n",
			device_id, cpu_port, voq, pkt, PKTLEN(pkt));
		printk("\t");

		for (i = 0; i < len; i++) {
			printk("%02X ", data[i]);
			if ((i % 32) == 31)
				printk("\n\t");
		}
		printk("\n\n");
	}
}
#endif

static void __ca_l2fp_dump_tx(
	ca_uint8_t 				device_id,
	ca_uint32_t 				cpu_port,
	ca_uint32_t 				voq,
	pkt_t * 					pkt) {

	ca_uint32_t i;
	ca_uint32_t len;
	char * data = PKTDATA(pkt);
	if ((ca_l2fp_debug & L2FP_DBG_TX) && (ca_l2fp_debug & (1 << device_id))) {
		len = (ca_l2fp_debug & L2FP_DBG_DUMP_LEN_MASK) >> L2FP_DBG_DUMP_LEN_SHIFT;

		if (len == 0)
			len = PKTLEN(pkt);
		else if (len > PKTLEN(pkt))
			len = PKTLEN(pkt);

		printk("\t TX l2fp[%d] cpu 0x%x voq %d pkt %p pkt_len %d\n",
			device_id, cpu_port, voq, pkt, PKTLEN(pkt));
		printk("\t");
		for (i = 0; i < len; i++) {
			printk("%02X ", data[i]);
			if ((i % 32) == 31)
				printk("\n\t");
		}
		printk("\n\n");
	}
}

ca_status_t ca_l2fp_receive_pkt(
	CA_IN		ca_uint32_t 				cpu_idx,
	CA_IN		ca_uint32_t 				voq,
	CA_IN		pkt_t * 					pkt) {

	ca_uint8_t device_id;
	ca_l2fp_device_t * l2fp_dev;
	ca_uint32_t 	cpu_port = cpu_idx + CPU_PORT_BASE;

	if ((cpu_port >= ca_l2fp_cpu_port_start) &&
		(cpu_port <= ca_l2fp_cpu_port_end)) {

#if defined(CONFIG_LUNA_G3_SERIES)
		// skip l2fp rx but return OK to indicate packet belong to wifi fast path.
#else
		device_id = cpu_port - ca_l2fp_cpu_port_start;
		l2fp_dev = ca_l2fp_dev_list[device_id];

		CA_L2FP_DEBUG(L2FP_DBG_L2FP_DEV_ALL, L2FP_DBG_RX,
			printk("%s cpu 0x%x voq %d pkt %p\n",
			__func__, cpu_port, voq, pkt));

		if (l2fp_dev == NULL){
			PKTFREE(pkt);
		} else {
			__ca_l2fp_dump_rx(device_id, cpu_port, voq, pkt);
			if (l2fp_dev->dev_info.rx_802_3_fn(l2fp_dev->dev_info.priv_data, voq, pkt) == CA_E_OK) {
				l2fp_dev->rx_packets++;
				l2fp_dev->rx_bytes += PKTLEN(pkt);
			} else {
				PKTFREE(pkt);
				l2fp_dev->rx_fail_packets++;
			}
		}
#endif		
		return CA_E_OK;
	}
	return CA_E_PORT;
}

ca_status_t ca_l2fp_xmit_done(
	CA_IN		void *					data,
	CA_IN		void * 					pkt) {

	long cast_tmp;
	ca_uint8_t device_id;
	ca_l2fp_device_t * l2fp_dev;
	ca_uint32_t 	cpu_idx;
	ca_uint32_t 	cpu_port;

	cast_tmp = (long)data;
	cpu_idx = cast_tmp & 0xffffffff;
	cpu_port = cpu_idx + CPU_PORT_BASE;

	if ((cpu_port >= ca_l2fp_cpu_port_start) &&
		(cpu_port <= ca_l2fp_cpu_port_end)) {

		device_id = cpu_port - ca_l2fp_cpu_port_start;
		l2fp_dev = ca_l2fp_dev_list[device_id];

		if (l2fp_dev == NULL){
			PKTFREE(pkt);
		} else {
			CA_L2FP_DEBUG(1 << device_id, L2FP_DBG_TX,
					printk("\t l2fp[%d] pkt %p \n",
					device_id, pkt));
			l2fp_dev->dev_info.xmit_done_fn(l2fp_dev->dev_info.priv_data, (pkt_t *) pkt);
			l2fp_dev->tx_done_packets++;
		}
		return CA_E_OK;
	}
	return CA_E_PORT;
}


ca_status_t ca_l2fp_device_register (
	CA_IN		ca_l2fp_device_entry_t *	p_l2fp_device_entry,
	CA_OUT		ca_uint8_t *				p_dev_id) {
	ca_uint8 i;
	ca_l2fp_device_t * l2fp_dev;
	ca_vlan_port_control_t port_config;

	for (i = 0; (i < CA_L2FP_DEVICE_MAX) && (i < ca_l2fp_cpu_port_total); i++) {
		if (ca_l2fp_dev_list[i] == NULL) {
			l2fp_dev = (ca_l2fp_device_t *)ca_malloc_trace(sizeof(ca_l2fp_device_t), GFP_KERNEL, true);
			memset(l2fp_dev, 0, sizeof(ca_l2fp_device_t));
			memcpy(&(l2fp_dev->dev_info), p_l2fp_device_entry, sizeof(ca_l2fp_device_entry_t));
			l2fp_dev->cpu_port = ca_l2fp_cpu_port_start + i;

			if (ca_ni_virtual_instance_open(l2fp_dev->cpu_port - CPU_PORT_BASE) != CA_E_OK) {
				CA_L2FP_DEBUG(L2FP_DBG_L2FP_DEV_ALL, L2FP_DBG_MANAGMENT,
					printk("%s ca_ni_virtual_instance_open fail\n", __func__));
				ca_free_trace(l2fp_dev);
				return CA_E_UNAVAIL;
			}

#ifdef CONFIG_ARCH_REALTEK_TAURUS
			//77C not use CA API
#else
			port_config.drop_unknown_vlan =
				((p_l2fp_device_entry->capacity & CA_L2FP_DEVICE_CAP_DROP_UNKNOW_VLAN) != 0);
			port_config.ingress_membership_check_enable =
				((p_l2fp_device_entry->capacity & CA_L2FP_DEVICE_CAP_MEMBERSHIP_CHECK) != 0);
			port_config.egress_membership_check_enable = port_config.ingress_membership_check_enable;
			port_config.ingress_vlan_action_enable =
				((p_l2fp_device_entry->capacity & CA_L2FP_DEVICE_CAP_INGRESS_VLAN_ACTION) != 0);
			port_config.egress_vlan_action_enable =
				((p_l2fp_device_entry->capacity & CA_L2FP_DEVICE_CAP_EGRESS_VLAN_ACTION) != 0);
			port_config.default_tag_add = FALSE;
			port_config.default_tag = 0;
			port_config.outer_tpid = CA_L2_VLAN_TAG_TYPE_SVLAN;
			port_config.inner_tpid = CA_L2_VLAN_TAG_TYPE_CVLAN;
			ca_l2_vlan_port_control_set(0, CA_PORT_ID(CA_PORT_TYPE_CPU, l2fp_dev->cpu_port), &port_config);
#endif
			ca_l2fp_dev_list[i] = l2fp_dev;
			*p_dev_id = i;
			CA_L2FP_DEBUG(L2FP_DBG_L2FP_DEV_ALL, L2FP_DBG_MANAGMENT,
					printk("%s l2fp[%d] cpu_port 0x%X capacity 0x%X successful\n",
					__func__, i, l2fp_dev->cpu_port, p_l2fp_device_entry->capacity));
			return CA_E_OK;
		}
	}
	return CA_E_FULL;
}
EXPORT_SYMBOL(ca_l2fp_device_register);

ca_status_t ca_l2fp_device_unregister (
	CA_IN		ca_uint8_t 				device_id){

	ca_l2fp_device_t * l2fp_dev;

	if (device_id > ca_l2fp_cpu_port_total)
		return CA_E_PARAM;

	if (ca_l2fp_dev_list[device_id] != NULL) {
		l2fp_dev = ca_l2fp_dev_list[device_id];
		ca_l2fp_dev_list[device_id] = NULL;
		ca_ni_virtual_instance_close(l2fp_dev->cpu_port - CPU_PORT_BASE);
		CA_L2FP_DEBUG(L2FP_DBG_L2FP_DEV_ALL, L2FP_DBG_MANAGMENT,
					printk("%s l2fp[%d] cpu_port 0x%X successful\n",
					__func__, device_id, l2fp_dev->cpu_port));
		ca_free_trace(l2fp_dev);
		return CA_E_OK;
	}

	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_l2fp_device_unregister);

ca_status_t ca_l2fp_device_dump (void){
	ca_uint8 i;
	ca_l2fp_device_t * l2fp_dev;

	for (i = 0; (i < CA_L2FP_DEVICE_MAX) && (i < ca_l2fp_cpu_port_total); i++) {
		if (ca_l2fp_dev_list[i] != NULL) {
			l2fp_dev = ca_l2fp_dev_list[i];
			printk("%s l2fp[%d] cpu_port 0x%X capacity 0x%X\n",
					__func__, i, l2fp_dev->cpu_port, l2fp_dev->dev_info.capacity);
			printk("\t rx_packets %llu rx_bytes %llu rx_fail_packets %llu\n",
				l2fp_dev->rx_packets, l2fp_dev->rx_bytes, l2fp_dev->rx_fail_packets);
			printk("\t tx_packets %llu tx_bytes %llu tx_fail_packets %llu tx_done_packets %llu\n",
				l2fp_dev->tx_packets, l2fp_dev->tx_bytes, l2fp_dev->tx_fail_packets, l2fp_dev->tx_done_packets);
		}
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l2fp_device_dump);

ca_status_t ca_l2fp_device_clear_statistic(ca_uint16 dev_bitmap) {
	ca_uint8 i;
	ca_l2fp_device_t * l2fp_dev;

	for (i = 0; (i < CA_L2FP_DEVICE_MAX) && (dev_bitmap & (1<<i)); i++) {
		if (ca_l2fp_dev_list[i] != NULL) {
			l2fp_dev = ca_l2fp_dev_list[i];
			l2fp_dev->rx_packets = 0;
			l2fp_dev->rx_bytes = 0;
			l2fp_dev->rx_fail_packets = 0;
			l2fp_dev->tx_packets = 0;
			l2fp_dev->tx_bytes = 0;
			l2fp_dev->tx_fail_packets = 0;
			l2fp_dev->tx_done_packets = 0;
		}
	}
	return CA_E_OK;
}

ca_status_t ca_l2fp_device_pkt_xmit (
	CA_IN		ca_uint8_t 				device_id,
	CA_IN 		ca_uint8_t 					priority_queue,
	CA_IN 		ca_uint8_t 					dest_port,
	CA_IN		pkt_t *						pkt){

	ca_l2fp_device_t * l2fp_dev;
	ca_status_t ret;
	ca_ni_tx_config_t tx_config;

	if (device_id > ca_l2fp_cpu_port_total)
		return CA_E_PARAM;

	if (ca_l2fp_dev_list[device_id] != NULL) {
		l2fp_dev = ca_l2fp_dev_list[device_id];

		__ca_l2fp_dump_tx(device_id, l2fp_dev->cpu_port, priority_queue, pkt);

		memset(&tx_config, 0, sizeof(ca_ni_tx_config_t));
		tx_config.core_config.bf.txq_index = priority_queue;
		tx_config.core_config.bf.ldpid = dest_port;
		tx_config.core_config.bf.cb_fn_mask = 1;
		tx_config.xmit_done_cb_fn = ca_l2fp_xmit_done;
		tx_config.cb_cookie = (void *)(long)(l2fp_dev->cpu_port - CPU_PORT_BASE);

		ret = ca_ni_virtual_instance_xmit(l2fp_dev->cpu_port - CPU_PORT_BASE,
			pkt, &tx_config);
		if (ret == CA_E_OK) {
			l2fp_dev->tx_packets++;
			l2fp_dev->tx_bytes += PKTLEN(pkt);
		} else {
			l2fp_dev->tx_fail_packets++;
		}
		return ret;
	}

	return CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_l2fp_device_pkt_xmit);

ca_status_t ca_l2fp_cpu_port_id_get (
	CA_IN		ca_uint8_t 				device_id,
	CA_OUT		ca_uint32_t *				p_cpu_id){

	if (p_cpu_id == NULL)
		return CA_E_PARAM;

	if (device_id > ca_l2fp_cpu_port_total)
		return CA_E_PARAM;

	if (ca_l2fp_dev_list[device_id] != NULL) {
		*p_cpu_id = ca_l2fp_dev_list[device_id]->cpu_port;
		return CA_E_OK;
	}

	return CA_E_NOT_FOUND;
}

static int l2fp_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_NE_L2FP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_l2fp_debug);

	return 0;
}

int l2fp_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l2fp_debug_show_help, (void *) file);
}

ssize_t l2fp_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long mask;
	ssize_t len;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _l2fp_debug_invalid;

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &mask))
#else
	if (kstrtoul(tmp, 0, &mask))
#endif
		goto _l2fp_debug_invalid;

	if (mask & L2FP_DBG_DUMP_L2FP_ENTRY) {
		ca_l2fp_device_dump();

	} else if (mask & L2FP_DBG_CLEAR_STATISTIC) {
		ca_l2fp_device_clear_statistic(mask & L2FP_DBG_L2FP_DEV_ALL);
	} else {
		ca_l2fp_debug = mask;
		printk(KERN_INFO "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_l2fp_debug);
		pr_info("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_l2fp_debug);
	}

	return count;

_l2fp_debug_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_l2fp_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= l2fp_debug_proc_open,
	.read		= seq_read,
	.write		= l2fp_debug_proc_write,
	.release	= single_release,
};

int ca_l2fp_proc_init(void)
{
	proc_l2fp = proc_mkdir("l2fp", proc_adapt_dir);

	if (proc_create("l2fp_debug", S_IRUGO | S_IWUGO, proc_l2fp, &fops_l2fp_debug) == NULL) {
		pr_err("Fail to create proc entry l2fp_debug.\n");
		return -ENOMEM;
	}

	return 0;
}

void ca_l2fp_proc_exit(void)
{
	remove_proc_entry("l2fp_debug", proc_l2fp);
	proc_remove(proc_l2fp);
}


ca_status_t ca_l2fp_init(void)
{
	ca_uint8 i;
	ca_char8_t dev_name[256];
	ca_status_t ret;
	for (i = 0; i < CA_L2FP_DEVICE_MAX; i++) {
		ca_l2fp_dev_list[i] = NULL;
	}
	ret = aal_scfg_read(CFG_ID_L2FP_CPU_PORT_START, 4, &(ca_l2fp_cpu_port_start));
	if(ret != CA_E_OK) {
		printk("%s: scfg read CFG_ID_L2FP_CPU_PORT_START fail!\n", __func__);
		return CA_E_ERROR;
	}
	ret = aal_scfg_read(CFG_ID_L2FP_CPU_PORT_END, 4, &(ca_l2fp_cpu_port_end));
	if(ret != CA_E_OK) {
		printk("%s: scfg read CFG_ID_L2FP_CPU_PORT_END fail!\n", __func__);
		return CA_E_ERROR;
	}
	ca_l2fp_cpu_port_total = (ca_l2fp_cpu_port_end - ca_l2fp_cpu_port_start + 1);

	for (i = ca_l2fp_cpu_port_start; i <= ca_l2fp_cpu_port_end ; i++) {
		snprintf(dev_name, 256, "%s%02X", "l2tp_cpu#0x", i);
		ca_ni_virtual_instance_alloc(i - CPU_PORT_BASE, dev_name);
	}

	printk("%s for cpu 0x%x ~ 0x%x done!\n", __func__, ca_l2fp_cpu_port_start, ca_l2fp_cpu_port_end);
	return CA_E_OK;
}

ca_status_t ca_l2fp_cleanup(void)
{
	ca_uint8 i;

	for (i = 0; i < CA_L2FP_DEVICE_MAX; i++) {
		ca_l2fp_device_unregister(i);
	}

	for (i = ca_l2fp_cpu_port_start; i<= ca_l2fp_cpu_port_end ; i++) {
		ca_ni_virtual_instance_free(i - CPU_PORT_BASE);
	}

	printk("%s for cpu 0x%x ~ 0x%x!\n", __func__, ca_l2fp_cpu_port_start, ca_l2fp_cpu_port_end);
	return CA_E_OK;
}
