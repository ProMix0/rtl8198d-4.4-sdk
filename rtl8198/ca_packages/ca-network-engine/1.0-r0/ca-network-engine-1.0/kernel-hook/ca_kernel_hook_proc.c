/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/

//#include <linux/module.h>
//#include <linux/moduleparam.h>
//#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 1)
# include <linux/export.h>
#endif

#include <linux/kernel.h>	/* printk() */
#include <linux/fs.h>		/* everything... */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <soc/cortina/cortina-soc.h>
#include <soc/cortina/ca_kernel_hook.h>

#include "ca_types.h"
#include "ca_kernel_hook_util.h"
#include "eth_port.h"
#include "ca_kernel_hook_vlan.h"
#include "queue.h"

static struct proc_dir_entry *proc_kernel_hook = NULL;

static int config_default_value(void)
{
	// ca_kh_hook = 0x0;
	ca_kh_debug = 0x0; // This is default value when module is loaded. Kernel also sets earlier default value.
	ca_kh_udp_offload_occasion = 0;
	printk(KERN_INFO "Kernel Hook default proc value:\n");
	printk(KERN_INFO "\tca_kh_hook=0x%x\n", ca_kh_hook);
	printk(KERN_INFO "\tca_kh_debug=0x%x\n", ca_kh_debug);
	printk(KERN_INFO "\tca_kh_udp_offload_occasion=0x%x\n", ca_kh_udp_offload_occasion);

	return 0;
}

/***************************************************************************
 * proc 'wan_port_vid_list'
 ***************************************************************************/
#define CA_KH_WAN_PORT_VID_MAX	16
wan_port_vid_t wan_port_vid_list[CA_KH_WAN_PORT_VID_MAX];
#define CA_KH_WAN_PORT_VID_LIST_HELP_MSG \
        "WAN Port VID List\n" \
        "READ Usage: cat %s\n" \
        "WRITE Usage: echo [+/-]VID > %s\n" \
        "\n"

static int ca_kh_wan_port_vid_list_show_help(struct seq_file *m, void *v)
{
        struct file *file = m->private;

        seq_printf(m, CA_KH_WAN_PORT_VID_LIST_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

        return 0;
}

int ca_kh_wan_port_vid_list_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, ca_kh_wan_port_vid_list_show_help, (void *) file);
}

static ssize_t ca_kh_wan_port_vid_list_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int i;

	/* print help message */
	printk(CA_KH_WAN_PORT_VID_LIST_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	printk("DUMP WAN port VID List:\n");
	printk("-----------------------------------------------------------------\n");
	for (i = 0; i < CA_KH_WAN_PORT_VID_MAX; i++) {
		printk("index %d: VID=%d\n", i, wan_port_vid_list[i].vid);
	}
	printk("=================================================================\n");

	return 0;
}

static ssize_t ca_kh_wan_port_vid_list_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	int i;
	char tmp[32];
	ssize_t len;

	ca_vlan_id_t vid; /* must be large enough for -65535 and 65535 */

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (len > 32)
		goto _wan_port_vid_list_invalid;
	if (copy_from_user(tmp, buffer, len))
		goto _wan_port_vid_list_invalid;
	tmp[len] = '\0';

	if (tmp[0] == '+') {
		if (kstrtou32(&(tmp[1]), 0, &vid))
			goto _wan_port_vid_list_invalid;

		for (i = 0; i < CA_KH_WAN_PORT_VID_MAX; i++) {
			if (wan_port_vid_list[i].vid == 0xffffffff) {
				break;
			}
		}
		if (i == CA_KH_WAN_PORT_VID_MAX) {
			printk(KERN_WARNING "ERROR! WAN port VID list is full.\n");
			return count;
		}
		wan_port_vid_list[i].vid = vid;
		ca_kh_add_classifier_for_wan_port_bridge_mode(vid, &wan_port_vid_list[i].cls_rule_index);
		printk("%s: i=%d, cls_rule_index=%d\n", __func__, i, wan_port_vid_list[i].cls_rule_index);
	}
	else if (tmp[0] == '-') {
		if (kstrtou32(&(tmp[1]), 0, &vid))
			goto _wan_port_vid_list_invalid;

		for (i = 0; i < CA_KH_WAN_PORT_VID_MAX; i++) {
			if (wan_port_vid_list[i].vid == vid) {
				break;
			}
		}
		if (i == CA_KH_WAN_PORT_VID_MAX) {
			printk(KERN_WARNING "ERROR! VID %d is not in WAN port VID list.\n", vid);
			return count;
		}
		wan_port_vid_list[i].vid = 0xffffffff;

		printk("%s: i=%d, cls_rule_index=%d\n", __func__, i, wan_port_vid_list[i].cls_rule_index);
		ca_kh_del_classifier_for_wan_port_bridge_mode(wan_port_vid_list[i].cls_rule_index);
	}
	else
		goto _wan_port_vid_list_invalid;

	/* if there is at least one entry then add wan port to unknown vlan flooding domain */
	for (i = 0; i < CA_KH_WAN_PORT_VID_MAX; i++) {
		if (wan_port_vid_list[i].vid != 0xffffffff) {
			break;
		}
	}
	if (i == CA_KH_WAN_PORT_VID_MAX) {
		ca_kh_add_wan_port_to_unknown_vlan_flooding_domain(FALSE);	/* no entry in list remove wan port from unknown vlan flooding domain */
	}
	else {
		ca_kh_add_wan_port_to_unknown_vlan_flooding_domain(TRUE);   	/* one or more entry in list add wan port to unknown vlan flooding domain */
	}
	return count;

_wan_port_vid_list_invalid:

        printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
        return count;
}

static const struct file_operations fops_kh_wan_port_vid_list __read_mostly = {
        .owner          = THIS_MODULE,
        .open           = ca_kh_wan_port_vid_list_proc_open,
        .read           = ca_kh_wan_port_vid_list_proc_read,
        .write          = ca_kh_wan_port_vid_list_proc_write,
};

/***************************************************************************
 * proc 'tcp_bypass_port'
 ***************************************************************************/

#define CA_KH_TCP_BYPASS_PORT_HELP_MSG \
	"TCP Bypass Port List\n" \
	"\n" \
	"Show port list:\n" \
	"  cat %s\n" \
	"\n" \
	"Add port number PORT into list:\n" \
	"  echo PORT > %s\n" \
	"\n" \
	"Delete port number PORT into list:\n" \
	"  echo -PORT > %s\n" \
	"\n"

static int ca_kh_tcp_bypass_port_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_TCP_BYPASS_PORT_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int ca_kh_tcp_bypass_port_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_tcp_bypass_port_show_help, (void *) file);
}

static ssize_t ca_kh_tcp_bypass_port_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/* print help message */
	printk(CA_KH_TCP_BYPASS_PORT_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	ca_kh_bypass_tcp_portlist_dump();

	return 0;
}

static ssize_t ca_kh_tcp_bypass_port_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char tmp[32];
	ssize_t len;
	ca_int32_t port; /* must be large enough for -65535 and 65535 */
	ca_status_t ret;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buffer, len))
		goto _tcp_bypass_port_invalid;
	tmp[len] = '\0';

	if (kstrtoint(tmp, 0, &port))
		goto _tcp_bypass_port_invalid;

	if(port > 0) {
		ret = ca_kh_bypass_tcp_portlist_add(0, port, CA_BYPASS_CNT_VALID);
		if(ret == CA_E_OK)
			printk("[%s]:Add port=%d OK\n", __func__, port);
	} else {
		ret = ca_kh_bypass_tcp_portlist_delete(0, -port, CA_BYPASS_CNT_VALID);
		if(ret == CA_E_OK)
			printk("[%s]:Delete port=%d OK\n", __func__, -port);
	}

	return count;

_tcp_bypass_port_invalid:
	printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_kh_tcp_bypass_port __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_tcp_bypass_port_proc_open,
	.read		= ca_kh_tcp_bypass_port_proc_read,
	.write		= ca_kh_tcp_bypass_port_proc_write,
};


/***************************************************************************
 * proc 'udp_bypass_port'
 ***************************************************************************/

#define CA_KH_UDP_BYPASS_PORT_HELP_MSG \
	"UDP Bypass Port List\n" \
	"\n" \
	"Show port list:\n" \
	"  cat %s\n" \
	"\n" \
	"Add port number PORT into list:\n" \
	"  echo PORT > %s\n" \
	"\n" \
	"Delete port number PORT into list:\n" \
	"  echo -PORT > %s\n" \
	"\n"

static int ca_kh_udp_bypass_port_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_UDP_BYPASS_PORT_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int ca_kh_udp_bypass_port_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_udp_bypass_port_show_help, (void *) file);
}

static ssize_t ca_kh_udp_bypass_port_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/* print help message */
	printk(CA_KH_UDP_BYPASS_PORT_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	ca_kh_bypass_udp_portlist_dump();
	return 0;
}

static ssize_t ca_kh_udp_bypass_port_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char tmp[32];
	ssize_t len;
	ca_int32_t port; /* must be large enough for -65535 and 65535 */
	ca_status_t ret;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buffer, len))
		goto _udp_bypass_port_invalid;
	tmp[len] = '\0';

	if (kstrtoint(tmp, 0, &port))
		goto _udp_bypass_port_invalid;

	if(port > 0) {
		ret = ca_kh_bypass_udp_portlist_add(0, port, CA_BYPASS_CNT_VALID);
		if(ret == CA_E_OK)
			printk("Add port=%d OK\n", port);

	} else {
		ret = ca_kh_bypass_udp_portlist_delete(0, -port, CA_BYPASS_CNT_VALID);
		if(ret == CA_E_OK)
			printk("Delete port=%d OK\n", -port);
	}

	return count;

_udp_bypass_port_invalid:
	printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_kh_udp_bypass_port __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_udp_bypass_port_proc_open,
	.read		= ca_kh_udp_bypass_port_proc_read,
	.write		= ca_kh_udp_bypass_port_proc_write,
};

/***************************************************************************
 * proc 'expected_master_bypass_port'
 ***************************************************************************/

#define CA_KH_EXPECTED_MASTER_BYPASS_PORT_HELP_MSG \
	"Expected Master Bypass Port List\n" \
	"\n" \
	"Show port list:\n" \
	"  cat %s\n" \
	"\n" \
	"Add port number PORT into list:\n" \
	"  echo PORT > %s\n" \
	"\n" \
	"Delete port number PORT into list:\n" \
	"  echo -PORT > %s\n" \
	"\n"

static int ca_kh_expected_master_bypass_port_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_EXPECTED_MASTER_BYPASS_PORT_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

static int ca_kh_expected_master_bypass_port_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_expected_master_bypass_port_show_help, (void *) file);
}

static ssize_t ca_kh_expected_master_bypass_port_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char tmp[32];
	ssize_t len;
	ca_int32_t port; /* must be large enough for -65535 and 65535 */

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buffer, len))
		goto _expected_master_bypass_port_invalid;
	tmp[len] = '\0';

	if (kstrtoint(tmp, 0, &port))
		goto _expected_master_bypass_port_invalid;

	// TODO

	return count;

_expected_master_bypass_port_invalid:
	printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_kh_expected_master_bypass_port __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_expected_master_bypass_port_proc_open,
	.read		= seq_read,
	.write		= ca_kh_expected_master_bypass_port_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'hook'
 ***************************************************************************/

#define CA_KH_HOOK_HELP_MSG \
	"Purpose: Enable Kernel Hook sub-modules to offload packets.\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: Hook for IPv4 Forward/NAT\n" \
	"  0x00000002: Hook for IPv6 Forward/NAT\n" \
	"  0x00000004: Hook for IPv4 Multicast\n" \
	"  0x00000008: Hook for IPv6 Multicast\n" \
	"  0x00000010: Hook for Netfilter Drop\n" \
	"  0x00000020: Hook for PPPoE (kernel mode)\n" \
	"  0x00000040: Hook for IPSec tunnel\n" \
	"  0x00000080: Hook for L2TP tunnel\n" \
	"  0x00000100: Hook for DSLite tunnel\n" \
	"  0x00000200: Hook for 6RD tunnel\n" \
	"  0x00000400: Hook for WiFi offload\n" \
	"  0x00010000: Hook for IPv4 IPSec passthrough (ESP packets)\n" \
	"  0x00020000: Hook for IPv6 IPSec passthrough (ESP packets)\n" \
	"  0x00040000: Hook for GRE passthrough\n" \
	"\n"

static int ca_kh_hook_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_HOOK_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_hook);

	return 0;
}

int ca_kh_hook_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_hook_show_help, (void *) file);
}

static ssize_t ca_kh_hook_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char tmp[32];
	ssize_t len;
	ca_uint32_t mask;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buffer, len))
		goto _hook_invalid;
	tmp[len] = '\0';

	if (kstrtou32(tmp, 0, &mask))
		goto _hook_invalid;

	ca_kh_hook = mask;
	printk(KERN_INFO "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_hook);

	return count;

_hook_invalid:
	printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_kh_hook __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_hook_proc_open,
	.read		= seq_read,
	.write		= ca_kh_hook_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'dump'
 ***************************************************************************/

#define CA_KH_DUMP_HELP_MSG \
	"Purpose: Dump table/info within Kernel Hook module\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo NUMBER > %s\n" \
	"  1: Dump Classifier Table\n" \
	"  2: Dump Interface Table\n" \
	"  3: Dump NAT Table\n" \
	"  4: Dump Nexthop Table\n" \
	"  5: Dump Route Table\n" \
	"  6: Dump Tunnel Table\n" \
	"  7: Dump Nexthop Table (Kernel Hook)\n" \
	"  8: Dump Mapping Table of intf_id and ifindex\n" \
	"  9: Dump L2 Multicast Table\n" \
	"  10: Dump L3 Multicast Table\n" \
	"  11: Dump L2 VLAN Table\n" \
	"  0xE0000000: CLEAR Multicast Max. Group Number\n" \
	"  0xF0000000: DEL HW NAT Entry\n" \
	"\n"

void ca_kh_pppoe_data_dump(void);
ca_status_t ca_kh_port_binding_dump(void);

static int g_nexthop_id = 0;
static int g_route_id = 0;
static int g_nat_id = 0;
static int ca_kh_l3_nexthop_traverse_cb(ca_device_id_t dev, ca_l3_nexthop_t *l3_nexthop, void *user_data)
{
	printk("================== nexthop_id=%d ====================\n", l3_nexthop->nexthop_id);
	ca_kh_ip_address_htonl(&l3_nexthop->addr, &l3_nexthop->addr);
	if (l3_nexthop->addr.afi == CA_IPV4) {
		printk("l3_nexthop->addr.afi=%d, l3_nexthop->addr.addr_len=%d, l3_nexthop->addr.ip_addr.ipv4_addr=%pI4\n",
				l3_nexthop->addr.afi, l3_nexthop->addr.addr_len, &l3_nexthop->addr.ip_addr.ipv4_addr);
	} else {
		printk("l3_nexthop->addr.afi=%d, l3_nexthop->addr.addr_len=%d, l3_nexthop->addr.ip_addr.ipv6_addr=%pI6\n",
				l3_nexthop->addr.afi, l3_nexthop->addr.addr_len, &l3_nexthop->addr.ip_addr.ipv6_addr);
	}
	printk("l3_nexthop->intf_id=%d, l3_nexthop->da_mac=%pM\n", l3_nexthop->intf_id, l3_nexthop->da_mac);
	printk("l3_nexthop->attr_flags=0x%x, l3_nexthop->aging_timer=%d\n", l3_nexthop->attr_flags, l3_nexthop->aging_timer);

	g_nexthop_id++;
	return CA_E_OK;
}

static int ca_kh_l3_route_traverse_cb(ca_device_id_t dev, ca_l3_route_t *l3_route, void *user_data)
{
	printk("================== g_route_id=%d ====================\n", g_route_id);
	ca_kh_ip_address_htonl(&l3_route->prefix, &l3_route->prefix);
	if (l3_route->prefix.afi == CA_IPV4) {
		printk("l3_route->prefix.afi=%d, l3_route->prefix.addr_len=%d, l3_route->prefix.ip_addr.ipv4_addr=%pI4\n",
				l3_route->prefix.afi, l3_route->prefix.addr_len, &l3_route->prefix.ip_addr.ipv4_addr);
	} else {
		printk("l3_route->prefix.afi=%d, l3_route->prefix.addr_len=%d, l3_route->prefix.ip_addr.ipv6_addr=%pI6\n",
				l3_route->prefix.afi, l3_route->prefix.addr_len, &l3_route->prefix.ip_addr.ipv6_addr);

	}
	printk("l3_route->nexthop_id=%d\n", l3_route->nexthop_id);

	g_route_id++;
	return CA_E_OK;
}

static int ca_kh_l3_interface_traverse(ca_device_id_t dev)
{
	int i, ret;
	ca_l3_intf_t l3_intf;
	ca_iterator_t iterator;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	iterator.entry_count = 1;
	iterator.entry_data = &l3_intf;
	iterator.user_buffer_size = sizeof(ca_l3_intf_t);

	i = 0;
	while (1) {
		ret = ca_l3_intf_iterate(dev, &iterator);

		if ((ret != CA_E_OK) || (iterator.entry_count == 0))
			break;

		printk("================= intf_id=%d =====================\n", l3_intf.intf_id);
		printk("type=%d, port_id=0x%x, nat_enable=%d\n", l3_intf.type, l3_intf.port_id, l3_intf.nat_enable);
		printk("mac_addr=%pM\n", l3_intf.mac_addr);
		printk("outer_vid=%d, inner_vid=%d, tunnel_id=%d, mtu=%d\n", l3_intf.outer_vid, l3_intf.inner_vid, l3_intf.tunnel_id, l3_intf.mtu);

		ca_kh_ip_address_htonl(&l3_intf.ip_addr, &l3_intf.ip_addr);
		if (l3_intf.ip_addr.afi == CA_IPV4) {
			printk("afi=%d, addr_len=%d, ipv4_addr=%pI4\n",
					l3_intf.ip_addr.afi, l3_intf.ip_addr.addr_len, &l3_intf.ip_addr.ip_addr.ipv4_addr);
		} else {
			printk("afi=%d, addr_len=%d, ipv6_addr=%pI6\n",
					l3_intf.ip_addr.afi, l3_intf.ip_addr.addr_len, &l3_intf.ip_addr.ip_addr.ipv6_addr);
		}
	}

	return CA_E_OK;
}

static int ca_kh_l3_classifier_traverse(ca_device_id_t dev)
{
	int i, ret;
	ca_classifier_rule_t	cls_rule;
	ca_iterator_t iterator;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	iterator.entry_count = 1;
	iterator.entry_data = &cls_rule;
	iterator.user_buffer_size = sizeof(ca_classifier_rule_t);

	i = 0;
	while (1) {
		ret = ca_classifier_rule_iterate(dev, &iterator);

		printk("%s: ca_classifier_rule_iterate(), ret=%d, iterator.entry_count=%d\n",
				__func__, ret, iterator.entry_count);
		if ((ret != CA_E_OK) || (iterator.entry_count == 0))
			break;

		printk("\n================= i=%d, index=%d =====================\n", i, cls_rule.index);
		printk("priority=%d\n", cls_rule.priority);
		printk("[ CLS key ]\n");
		if (cls_rule.key_mask.src_port)
			printk("src_port=0x%x ", cls_rule.key.src_port);
		if (cls_rule.key_mask.src_intf)
			printk("src_intf=0x%x ", cls_rule.key.src_intf);
		if (cls_rule.key_mask.dst_intf)
			printk("dst_intf=0x%x\n", cls_rule.key.dst_intf);

		if (cls_rule.key_mask.l2) {
			printk("\n[[ CLS key L2 ]]\n");
			if (cls_rule.key_mask.l2_mask.ethertype)
				printk("ethertype=0x%x ", cls_rule.key.l2.ethertype);
			if (cls_rule.key_mask.l2_mask.subtype)
				printk("subtype=%d ", cls_rule.key.l2.subtype);
			if (cls_rule.key_mask.l2_mask.mac_sa)
				printk("src_mac=%pM ", &cls_rule.key.l2.mac_sa);
			if (cls_rule.key_mask.l2_mask.mac_da)
				printk("dst_mac=%pM\n", &cls_rule.key.l2.mac_da);
#if 0
			if (cls_rule.key_mask.l2_mask.vlan_otag_mask.vid)
				printk("vlan_otag.vid=%d ", cls_rule.key.l2.vlan_otag.vlan_min.vid);
			if (cls_rule.key_mask.l2_mask.vlan_otag_mask.tpid)
				printk("vlan_otag.tpid=0x%x ", cls_rule.key.l2.vlan_otag.vlan_min.tpid);
			if (cls_rule.key_mask.l2_mask.vlan_otag_mask.pri)
				printk("vlan_otag.pri=%d ", cls_rule.key.l2.vlan_otag.vlan_min.pri);
			if (cls_rule.key_mask.l2_mask.vlan_otag_mask.dei)
				printk("vlan_otag.dei=%d\n", cls_rule.key.l2.vlan_otag.vlan_min.dei);

			if (cls_rule.key_mask.l2_mask.vlan_itag_mask.vid)
				printk("vlan_itag.vid=%d ", cls_rule.key.l2.vlan_itag.vlan_min.vid);
			if (cls_rule.key_mask.l2_mask.vlan_itag_mask.tpid)
				printk("vlan_itag.tpid=0x%x ", cls_rule.key.l2.vlan_itag.vlan_min.tpid);
			if (cls_rule.key_mask.l2_mask.vlan_itag_mask.pri)
				printk("vlan_itag.pri=%d ", cls_rule.key.l2.vlan_itag.vlan_min.pri);
			if (cls_rule.key_mask.l2_mask.vlan_itag_mask.dei)
				printk("vlan_itag.dei=%d\n", cls_rule.key.l2.vlan_itag.vlan_min.dei);
#endif
			if (cls_rule.key_mask.l2_mask.is_multicast)
				printk("is_multicast=%d ", cls_rule.key.l2.is_multicast);
			if (cls_rule.key_mask.l2_mask.is_length)
				printk("is_length=%d ", cls_rule.key.l2.is_length);
			if (cls_rule.key_mask.l2_mask.cfm_opcode)
				printk("cfm_opcode=%d\n", cls_rule.key.l2.cfm_opcode);
		}

		if (cls_rule.key_mask.ip) {
			printk("\n[[ CLS key IP ]]\n");
			if (cls_rule.key_mask.ip_mask.ip_valid)
				printk("ip_valid=%d ", cls_rule.key.ip.ip_valid);
			if (cls_rule.key_mask.ip_mask.ip_version)
				printk("ip_version=%d ", cls_rule.key.ip.ip_version);
			if (cls_rule.key_mask.ip_mask.ip_protocol)
				printk("ip_protocol=%d ", cls_rule.key.ip.ip_protocol);
			if (cls_rule.key_mask.ip_mask.dscp)
				printk("dscp=%d ", cls_rule.key.ip.dscp);
			if (cls_rule.key_mask.ip_mask.ecn)
				printk("ecn=%d\n", cls_rule.key.ip.ecn);

			if (cls_rule.key_mask.ip_mask.ip_sa) {
				ca_kh_ip_address_htonl(&cls_rule.key.ip.ip_sa, &cls_rule.key.ip.ip_sa);
				if (cls_rule.key.ip.ip_sa.afi == CA_IPV4) {
					printk("ip_sa(IPv4): afi=%d, addr_len=%d, ipv4_addr=%pI4\n",
							cls_rule.key.ip.ip_sa.afi, cls_rule.key.ip.ip_sa.addr_len, &cls_rule.key.ip.ip_sa.ip_addr.ipv4_addr);
				} else {
					printk("ip_sa(IPv6): afi=%d, addr_len=%d, ipv6_addr=%pI6\n",
							cls_rule.key.ip.ip_sa.afi, cls_rule.key.ip.ip_sa.addr_len, &cls_rule.key.ip.ip_sa.ip_addr.ipv6_addr);
				}
			}

			if (cls_rule.key_mask.ip_mask.ip_da) {
				ca_kh_ip_address_htonl(&cls_rule.key.ip.ip_da, &cls_rule.key.ip.ip_da);
				if (cls_rule.key.ip.ip_da.afi == CA_IPV4) {
					printk("ip_da(IPv4): afi=%d, addr_len=%d, ipv4_addr=%pI4\n",
							cls_rule.key.ip.ip_da.afi, cls_rule.key.ip.ip_da.addr_len, &cls_rule.key.ip.ip_da.ip_addr.ipv4_addr);
				} else {
					printk("ip_da(IPv6): afi=%d, addr_len=%d, ipv6_addr=%pI6\n",
							cls_rule.key.ip.ip_da.afi, cls_rule.key.ip.ip_da.addr_len, &cls_rule.key.ip.ip_da.ip_addr.ipv6_addr);
				}
			}

			if (cls_rule.key_mask.ip_mask.fragment)
				printk("fragment=%d ", cls_rule.key.ip.fragment);
			if (cls_rule.key_mask.ip_mask.have_options)
				printk("have_options=%d ", cls_rule.key.ip.have_options);
			if (cls_rule.key_mask.ip_mask.flow_label)
				printk("flow_label=%d ", cls_rule.key.ip.flow_label);
			if (cls_rule.key_mask.ip_mask.ext_header)
				printk("ext_header=%d ", cls_rule.key.ip.ext_header);
			if (cls_rule.key_mask.ip_mask.icmp_type)
				printk("icmp_type=%d ", cls_rule.key.ip.icmp_type);
			if (cls_rule.key_mask.ip_mask.igmp_type)
				printk("igmp_type=%d\n", cls_rule.key.ip.igmp_type);
		}

		if (cls_rule.key_mask.l4) {
			printk("\n[[ CLS key L4 ]]\n");
			if (cls_rule.key_mask.l4_mask.l4_valid)
				printk("l4_valid=%d ", cls_rule.key.l4.l4_valid);
			if (cls_rule.key_mask.l4_mask.src_port)
				printk("src_port.min=%d, src_port.max=%d ", cls_rule.key.l4.src_port.min, cls_rule.key.l4.src_port.max);
			if (cls_rule.key_mask.l4_mask.dst_port)
				printk("dst_port.min=%d, dst_port.max=%d ", cls_rule.key.l4.dst_port.min, cls_rule.key.l4.dst_port.max);
			if (cls_rule.key_mask.l4_mask.tcp_flags)
				printk("tcp_flags=0x%x\n", cls_rule.key.l4.tcp_flags);
		}

		if (cls_rule.key_mask.ext) {
			printk("\n[[ CLS key EXT ]]\n");
			printk("offset=%d, length=%d\n", cls_rule.key.ext.offset, cls_rule.key.ext.length);
			printk("data=0x%02x 0x%02x 0x%02x 0x%02x\n",
					cls_rule.key.ext.data[0], cls_rule.key.ext.data[1], cls_rule.key.ext.data[2], cls_rule.key.ext.data[3]);
			printk("mask=0x%02x 0x%02x 0x%02x 0x%02x\n",
					cls_rule.key.ext.mask[0], cls_rule.key.ext.mask[1], cls_rule.key.ext.mask[2], cls_rule.key.ext.mask[3]);
		}

		printk("\n[ CLS action ]\n");
		if (cls_rule.action.forward == CA_CLASSIFIER_FORWARD_PORT)
			printk("forward=CA_CLASSIFIER_FORWARD_PORT to dest_port=0x%x\n", cls_rule.action.dest.port);
		else if (cls_rule.action.forward == CA_CLASSIFIER_FORWARD_INTERFACE)
			printk("forward=CA_CLASSIFIER_FORWARD_INTERFACE to dest_intf=%d\n", cls_rule.action.dest.intf);
		else if (cls_rule.action.forward == CA_CLASSIFIER_FORWARD_DENY)
			printk("forward=CA_CLASSIFIER_FORWARD_DENY\n");
		else if (cls_rule.action.forward == CA_CLASSIFIER_FORWARD_FE)
			printk("forward=CA_CLASSIFIER_FORWARD_FE\n");

		if (cls_rule.action.options.masks.action_handle)
			printk("flow_id=%d ", cls_rule.action.options.action_handle.flow_id);
		if (cls_rule.action.options.masks.priority)
			printk("priority=%d ", cls_rule.action.options.priority);
		if (cls_rule.action.options.masks.dscp)
			printk("dscp=%d\n", cls_rule.action.options.dscp);

		if (cls_rule.action.options.masks.outer_vlan_act)
			printk("outer_vlan_act=%d outer_vid=%d ", cls_rule.action.options.outer_vlan_act, cls_rule.action.options.outer_vid);
		if (cls_rule.action.options.masks.outer_tpid)
			printk("outer_tpid=0x%x ", cls_rule.action.options.outer_tpid);
		if (cls_rule.action.options.masks.outer_dot1p)
			printk("outer_dot1p=%d ", cls_rule.action.options.outer_dot1p);
		if (cls_rule.action.options.masks.outer_dei)
			printk("outer_dot1p=%d\n", cls_rule.action.options.outer_dei);

		if (cls_rule.action.options.masks.inner_vlan_act)
			printk("inner_vlan_act=%d inner_vid=%d ", cls_rule.action.options.inner_vlan_act, cls_rule.action.options.inner_vid);
		if (cls_rule.action.options.masks.inner_tpid)
			printk("inner_tpid=0x%x ", cls_rule.action.options.inner_tpid);
		if (cls_rule.action.options.masks.inner_dot1p)
			printk("inner_dot1p=%d ", cls_rule.action.options.inner_dot1p);
		if (cls_rule.action.options.masks.inner_dei)
			printk("inner_dot1p=%d\n", cls_rule.action.options.inner_dei);

		if (cls_rule.action.options.masks.mac_da)
			printk("mac_da=%pM", &cls_rule.action.options.mac_da);
		if (cls_rule.action.options.masks.sw_id)
			printk("sw_id=0x%02x 0x%02x 0x%02x 0x%02x",
					cls_rule.action.options.sw_id[0], cls_rule.action.options.sw_id[1], cls_rule.action.options.sw_id[2], cls_rule.action.options.sw_id[3]);
		if (cls_rule.action.options.masks.sw_shaper_id)
			printk("sw_shaper_id=%d ", cls_rule.action.options.sw_shaper_id);
		i++;

		cond_resched();
	}

	return CA_E_OK;
}

static int ca_kh_tunnel_traverse(ca_device_id_t dev)
{
	int i, ret;
	ca_tunnel_cfg_t tunnel_cfg;

	for (i = 0; i < CA_TUNNEL_MAX_LIMIT; i++) {

		ret = ca_tunnel_get(dev, i, &tunnel_cfg);
		if (ret != CA_E_OK) continue;

		printk("================= tunnel_id=%d =====================\n", i);
		printk("type=%d, parent_l3_intf_id=%d\n", tunnel_cfg.type, tunnel_cfg.parent_l3_intf_id);

		if (tunnel_cfg.src_addr.afi == CA_IPV4) {
			printk("src_addr.afi=%d, src_addr.addr_len=%d, src_addr.ip_addr.ipv4_addr=%pI4\n",
					tunnel_cfg.src_addr.afi, tunnel_cfg.src_addr.addr_len, &tunnel_cfg.src_addr.ip_addr.ipv4_addr);
		} else {
			printk("src_addr.afi=%d, src_addr.addr_len=%d, src_addr.ip_addr.ipv6_addr=%pI6\n",
					tunnel_cfg.src_addr.afi, tunnel_cfg.src_addr.addr_len, &tunnel_cfg.src_addr.ip_addr.ipv6_addr);
		}

		if (tunnel_cfg.dest_addr.afi == CA_IPV4) {
			printk("dest_addr.afi=%d, dest_addr.addr_len=%d, dest_addr.ip_addr.ipv4_addr=%pI4\n",
					tunnel_cfg.dest_addr.afi, tunnel_cfg.dest_addr.addr_len, &tunnel_cfg.dest_addr.ip_addr.ipv4_addr);
		} else {
			printk("dest_addr.afi=%d, dest_addr.addr_len=%d, dest_addr.ip_addr.ipv6_addr=%pI6\n",
					tunnel_cfg.dest_addr.afi, tunnel_cfg.dest_addr.addr_len, &tunnel_cfg.dest_addr.ip_addr.ipv6_addr);
		}

		if (tunnel_cfg.type == CA_TUNNEL_PPPOE) {
			printk("pppoe_session_id=0x%04x, mac_da=%pM\n",
					tunnel_cfg.tunnel.pppoe.pppoe_session_id, tunnel_cfg.tunnel.pppoe.mac_da);
		}
	}
	return CA_E_OK;
}

static int ca_kh_nat_entry_traverse_cb(ca_device_id_t device_id, ca_nat_entry_t *session, void *user_data)
{
	printk("================= nat_id=%d =====================\n", g_nat_id);
	printk("session->xlate_flags=0x%x\n", session->xlate_flags);
	ca_kh_ip_address_htonl(&session->src_ip_addr, &session->src_ip_addr);
	printk("session->src_ip_addr.ip_addr.ipv4_addr=%pI4\n", &session->src_ip_addr.ip_addr.ipv4_addr);
	printk("session->src_l4_port=%d\n", session->src_l4_port);
	ca_kh_ip_address_htonl(&session->dst_ip_addr, &session->dst_ip_addr);
	printk("session->dst_ip_addr.ip_addr.ipv4_addr=%pI4\n", &session->dst_ip_addr.ip_addr.ipv4_addr);
	printk("session->dst_l4_port=%d\n", session->dst_l4_port);
	printk("session->ip_proto=%d\n", session->ip_proto);
	ca_kh_ip_address_htonl(&session->new_src_ip_addr, &session->new_src_ip_addr);
	printk("session->new_src_ip_addr.ip_addr.ipv4_addr=%pI4\n", &session->new_src_ip_addr.ip_addr.ipv4_addr);
	printk("session->new_src_l4_port=%d\n", session->new_src_l4_port);
	ca_kh_ip_address_htonl(&session->new_dst_ip_addr, &session->new_dst_ip_addr);
	printk("session->new_dst_ip_addr.ip_addr.ipv4_addr=%pI4\n", &session->new_dst_ip_addr.ip_addr.ipv4_addr);
	printk("session->new_dst_l4_port=%d\n", session->new_dst_l4_port);
	printk("session->aging_timer=%d\n", session->aging_timer);

	g_nat_id++;
	return CA_E_OK;
}

static int ca_kh_nexthop_traverse(ca_device_id_t device_id)
{
	int i;
	ca_kh_nexthop_entry_t *kh_nexthop;

	printk("================= IPV4 kernel hook nexthop table =====================\n");
	for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
		if (ca_kh_ipv4_nexthop_table[i].is_used == 0)
			continue;
		printk("================= index=%d =====================\n", i);
		kh_nexthop = &ca_kh_ipv4_nexthop_table[i];
		printk("kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv4_addr=%pI4, kh_nexthop->ip_addr.addr_len=%d\n",
				kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv4_addr, kh_nexthop->ip_addr.addr_len);
		printk("kh_nexthop->ifindex=%d, kh_nexthop->l3_nexthop_id=%d, kh_nexthop->attr_flags=%d\n",
				kh_nexthop->ifindex, kh_nexthop->l3_nexthop_id, kh_nexthop->attr_flags);

		cond_resched();
	}

	printk("================= IPV6 kernel hook nexthop table =====================\n");
	for (i = 0; i < CA_L3_MAX_NEXTHOP_NUM; i++) {
		if (ca_kh_ipv6_nexthop_table[i].is_used == 0)
			continue;
		printk("================= index=%d =====================\n", i);
		kh_nexthop = &ca_kh_ipv6_nexthop_table[i];
		printk("kh_nexthop->ip_addr.afi=%d, kh_nexthop->ip_addr.ip_addr.ipv6_addr=%pI6, kh_nexthop->ip_addr.addr_len=%d\n",
				kh_nexthop->ip_addr.afi, &kh_nexthop->ip_addr.ip_addr.ipv6_addr, kh_nexthop->ip_addr.addr_len);
		printk("kh_nexthop->ifindex=%d, kh_nexthop->l3_nexthop_id=%d\n", kh_nexthop->ifindex, kh_nexthop->l3_nexthop_id);

		cond_resched();
	}

	return CA_E_OK;
}

static ca_status_t ca_l3_nexthop_traverse(ca_device_id_t device_id, ca_ip_afi_t afi, void *user_data)
{
	ca_iterator_t   iterator;
	ca_l3_nexthop_t l3_nexthop;
	ca_uint32_t     count;
	ca_status_t     ret;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &l3_nexthop;
	iterator.user_buffer_size = sizeof(ca_l3_nexthop_t);

	while (1) {
		ret = ca_l3_nexthop_iterate(device_id, afi, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;
		ca_kh_l3_nexthop_traverse_cb(device_id, &l3_nexthop, user_data);

		cond_resched();
	}

	return ret;
}

static ca_status_t ca_l3_route_traverse(ca_device_id_t device_id, ca_ip_afi_t afi, void *user_data)
{
	ca_iterator_t   iterator;
	ca_l3_route_t l3_route;
	ca_uint32_t     count;
	ca_status_t     ret;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &l3_route;
	iterator.user_buffer_size = sizeof(ca_l3_route_t);

	while (1) {
		ret = ca_l3_route_iterate(device_id, afi, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;
		ca_kh_l3_route_traverse_cb(device_id, &l3_route, user_data);

		cond_resched();
	}

	return ret;
}

static ca_status_t ca_nat_entry_traverse(ca_device_id_t device_id, void *user_data)
{
	ca_iterator_t   iterator;
	ca_nat_entry_t	nat_entry;
	ca_uint32_t     count;
	ca_status_t     ret;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &nat_entry;
	iterator.user_buffer_size = sizeof(ca_nat_entry_t);

	while (1) {
		ret = ca_nat_entry_iterate(device_id, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;
		ca_kh_nat_entry_traverse_cb(0, &nat_entry, NULL);

		cond_resched();
	}

	return ret;
}

static ca_status_t ca_l2_multicast_entry_traverse(ca_device_id_t device_id)
{
	ca_iterator_t   iterator;
	ca_l2_mcast_entry_t		mcast_entry;
	ca_l2_mcast_group_members_t     mcast_grp_members;
	ca_l2_mcast_iterator_entry_t 	mcast_itr_entry;
	ca_uint32_t     count;
	ca_status_t     ret;
	int i, index;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &mcast_itr_entry;
	iterator.user_buffer_size = sizeof(ca_l2_mcast_iterator_entry_t);

	index = 0;
	while (1) {
		ret = ca_l2_mcast_group_iterate(device_id, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;

		printk("================= index=%d =====================\n", index);

		mcast_entry = mcast_itr_entry.mc_entry;
		mcast_grp_members = mcast_itr_entry.members;

		printk("mcast_vlan=%d, group address afi=%d\n", mcast_entry.mcast_vlan, mcast_entry.group_ip_addr.afi);
		ca_kh_ip_address_htonl(&(mcast_entry.group_ip_addr), &(mcast_entry.group_ip_addr));
		if (mcast_entry.group_ip_addr.afi == CA_IPV4) {
			printk("group address=%pI4\n", &(mcast_entry.group_ip_addr.ip_addr.ipv4_addr));
		}
		else {
			printk("group address=%pI6\n", &(mcast_entry.group_ip_addr.ip_addr.ipv6_addr[0]));
		}

		ca_kh_ip_address_htonl(&(mcast_entry.src_ip_address), &(mcast_entry.src_ip_address));
		if (mcast_entry.src_ip_address.afi == CA_IPV4)
			printk("source address=%pI4\n", &(mcast_entry.src_ip_address.ip_addr.ipv4_addr));
		else
			printk("source address=%pI6\n", &(mcast_entry.src_ip_address.ip_addr.ipv6_addr));

		printk("group member mcg_id=%d, member_count=%d\n", mcast_grp_members.mcg_id, mcast_grp_members.member_count);
		for (i= 0; i < mcast_grp_members.member_count; i++) {
			printk("[group member %d]\n", i);
			printk("    action_mask.mac_da_overwrite=%d, action_mask.vlan_action=%d\n",
					mcast_grp_members.member[i].action_mask.mac_da_overwrite, mcast_grp_members.member[i].action_mask.vlan_action);
			printk("    new_mac_da=%pM\n", &(mcast_grp_members.member[i].new_mac_da));
			printk("    vid=%d, member_port=0x%x, vlan_action=%d\n", mcast_grp_members.member[i].vid,
					mcast_grp_members.member[i].member_port, mcast_grp_members.member[i].vlan_action);
		}

		index++;
	}
	return ret;
}

extern int ca_kh_used_vid[];
static ca_status_t ca_l2_vlan_entry_traverse(ca_device_id_t device_id)
{
	int i, vid;
	ca_status_t  ret;
	ca_uint8_t   member_count;
	ca_port_id_t member_ports[CA_ETH_PORT_NUM];
	ca_uint8_t   untagged_count;
	ca_port_id_t untagged_ports[CA_ETH_PORT_NUM];
	ca_flooding_port_t flood_ports[CA_ETH_PORT_NUM];

	for (vid = 0; vid < 4096; vid++) {
		if (ca_kh_used_vid[vid] == 0)
			continue;
		printk("================= vid=%d =====================\n", vid);
		ret = ca_l2_vlan_port_get(device_id, vid, &member_count, member_ports, &untagged_count, untagged_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_port_get(vid=%d) failed, ret=%d\n", __func__, vid, ret);
			continue;
		}
		printk("[member_count=%d]\n", member_count);
		for (i = 0; i < member_count; i++) {
			printk("member_ports[%d]=0x%x\n", i, member_ports[i]);
		}
		printk("[untagged_count=%d]\n", untagged_count);
		for (i = 0; i < untagged_count; i++) {
			printk("untagged_ports[%d]=0x%x\n", i, untagged_ports[i]);
		}
		ret = ca_l2_vlan_flooding_domain_members_get(device_id, vid, CA_L2_ING_PORT_TYPE_LAN, &member_count, flood_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_flooding_domain_members_get(vid=%d) failed, ret=%d\n", __func__, vid, ret);
			continue;
		}
		printk("[flooding domain count=%d]\n", member_count);
		for (i = 0; i < member_count; i++) {
			printk("flood_ports[%d].egress_vid=0x%x, flood_ports[%d].egress_port=0x%x\n",
					i, flood_ports[i].egress_vid, i, flood_ports[i].egress_port);
		}
	}

	return CA_E_OK;
}

static ca_status_t ca_l3_multicast_entry_traverse(ca_device_id_t device_id)
{
	ca_iterator_t   iterator;
	ca_l3_mcast_entry_t             mcast_entry;
	ca_l3_mcast_group_members_t     mcast_grp_members;
	ca_l3_mcast_iterator_entry_t    mcast_itr_entry;
	ca_uint32_t     count;
	ca_status_t     ret;
	int i, index;

	/* get first nexthop entry */
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &mcast_itr_entry;
	iterator.user_buffer_size = sizeof(ca_l3_mcast_iterator_entry_t);

	index = 0;
	while (1) {
		ret = ca_l3_mcast_group_iterate(device_id, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;

		printk("================= index=%d =====================\n", index);

		mcast_entry = mcast_itr_entry.mc_entry;
		mcast_grp_members = mcast_itr_entry.members;

		printk("group address afi=%d\n", mcast_entry.group_ip_addr.afi);
		ca_kh_ip_address_htonl(&(mcast_entry.group_ip_addr), &(mcast_entry.group_ip_addr));
		if (mcast_entry.group_ip_addr.afi == CA_IPV4) {
			printk("group address=%pI4\n", &(mcast_entry.group_ip_addr.ip_addr.ipv4_addr));
		}
		else {
			printk("group address=%pI6\n", &(mcast_entry.group_ip_addr.ip_addr.ipv6_addr[0]));
		}

		ca_kh_ip_address_htonl(&(mcast_entry.src_ip_address), &(mcast_entry.src_ip_address));
		if (mcast_entry.src_ip_address.afi == CA_IPV4)
			printk("source address=%pI4\n", &(mcast_entry.src_ip_address.ip_addr.ipv4_addr));
		else
			printk("source address=%pI6\n", &(mcast_entry.src_ip_address.ip_addr.ipv6_addr));

		printk("group member mcg_id=%d, member_count=%d\n", mcast_grp_members.mcg_id, mcast_grp_members.member_count);
		for (i= 0; i < mcast_grp_members.member_count; i++) {
			printk("[group member %d]\n", i);
			printk("    action_mask.mac_da_overwrite=%d\n", mcast_grp_members.member[i].action_mask.mac_da_overwrite);
			printk("    new_mac_da=%pM\n", &(mcast_grp_members.member[i].new_mac_da));
			printk("    member_intf=%d\n", mcast_grp_members.member[i].member_intf);
		}

		index++;
	}

	return ret;
}

static int ca_kh_dump_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_DUMP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

static int ca_kh_dump_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_dump_show_help, (void *) file);
}

static ssize_t ca_kh_dump_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char buf[32];
	unsigned long mask;
	ssize_t len;
	int l3_user_data;

	len = min(count, (unsigned long)(sizeof(buf) - 1));

	if (copy_from_user(buf, buffer, len)) {
		printk("%s: copy_from_user() failed!!\n", __func__);
		goto _dump_invalid;
	}

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &mask)) {
		printk("%s: kstrtoul() failed!!\n", __func__);
		goto _dump_invalid;
	}

	if (mask == CA_KH_DUMP_IFINDEX) {
		int i, ret;
		ca_int16_t ifindex;
		ca_ip_afi_t afi;
		ca_l3_intf_t l3_intf;
		ca_tunnel_cfg_t tcfg;

		printk("================ IPv4/IPv6 ifindex table =============== \n");
		/* interface id start from 0 */
		for (i = 0; i < CA_L3_IP_MAX_INTF; i++) {
			tcfg.type = CA_TUNNELINVALID;
			ret = ca_kh_l3_get_ifindex_by_intf_id(0, &afi, i, &ifindex);
			if (ret == CA_E_OK) {
				l3_intf.intf_id = i;
				ret = ca_l3_intf_get(0, &l3_intf);
				if (ret != CA_E_OK) {
					//printk("%s: ca_l3_intf_get(intf_id=%d), return failed!!, ret=%d\n", __func__, l3_intf.intf_id, ret);
					continue;
				}
				if (l3_intf.type == CA_L3_INTF_TUNNEL) {
					ret = ca_tunnel_get(0, l3_intf.tunnel_id, &tcfg);
					if (ret != CA_E_OK) continue;
				}
				printk("intf_id=%d, afi=%d, intf_type=%d, ifindex=%d, tunnel_type=%d\n", i, afi, l3_intf.type, ifindex, tcfg.type);
			}
		}
		return count;
	}
	else if (mask == CA_KH_DUMP_INTF_TABLE) {
		printk("=================== Interface table =================== \n");
		ca_kh_l3_interface_traverse(0);
		return count;
	}
	else if (mask == CA_KH_DUMP_NEXTHOP_TABLE) {
		g_nexthop_id = 0;
		printk("=================== IPV4 nexthop table =================== \n");
		ca_l3_nexthop_traverse(0, CA_IPV4, &l3_user_data);
		printk(" =================== IPV6 nexthop table =================== \n");
		ca_l3_nexthop_traverse(0, CA_IPV6, &l3_user_data);
		return count;
	}
	else if (mask == CA_KH_DUMP_ROUTE_TABLE) {
		g_route_id = 0;
		printk("=================== IPV4 route table =================== \n");
		ca_l3_route_traverse(0, CA_IPV4, &l3_user_data);
		printk("=================== IPV6 route table =================== \n");
		ca_l3_route_traverse(0, CA_IPV6, &l3_user_data);
		return count;
	}
	else if (mask == CA_KH_DUMP_NAT_TABLE) {
		g_nat_id = 0;
		printk("=================== NAT table ====================== \n");
		ca_nat_entry_traverse(0, &l3_user_data);
		return count;
	}
	else if (mask == CA_KH_DUMP_TUNNEL_TABLE) {
		printk("=================== Tunnel table ====================== \n");
		ca_kh_tunnel_traverse(0);
		return count;
	}
	else if (mask == CA_KH_DUMP_KH_NEXTHOP_TABLE) {			//Dump kernel hook nexthop table
		printk("=================== Kernel hook nexthop table ====================== \n");
		ca_kh_nexthop_traverse(0);
		return count;
	}

	else if (mask == CA_KH_DUMP_CLASSIFIER_TABLE) {
		printk("=================== Classifier table ====================== \n");
		ca_kh_l3_classifier_traverse(0);
		return count;
	}
	else if (mask == CA_KH_DUMP_L2_MULTICAST_TABLE) {
		printk("=================== L2 multicast table ====================== \n");
		ca_l2_multicast_entry_traverse(0);
		return count;
	}
	else if (mask == CA_KH_DUMP_L2_VLAN_TABLE) {
		printk("=================== L2 VLAN table ====================== \n");
		ca_l2_vlan_entry_traverse(0);
		return count;
	}
	else if (mask == CA_KH_DUMP_L3_MULTICAST_TABLE) {
		printk("=================== L3 multicast table ====================== \n");
		ca_l3_multicast_entry_traverse(0);
		return count;
	}
#if 0
	else if (mask == CA_KH_DUMP_LIMIT_TABLE) {			//Dump limit list
		ca_kh_limit_list_dump();
		return count;
	}
	else if (mask == CA_KH_DUMP_LAN_DEV_TABLE) {			//Dump lan dev list
		ca_kh_lan_dev_dump();
		return count;
	}
	else if (mask == CA_KH_DUMP_PPPOE_DATA) {			//Dump pppoe_data[]
		ca_kh_pppoe_data_dump();
		return count;
	}
	else if (mask == CA_KH_DUMP_PORT_BINDING) {			//Dump wan_port_binding[]
		ca_kh_port_binding_dump();
		return count;
	}
#endif
	else if (mask == CA_KH_DEL_NAT_ENTRY) {
		ca_nat_entry_delete_all(0);
		return count;
	}
	else if (mask == CA_KH_CLEAR_MULTICAST_MAX_GRP) {		/* Clear Multicast Max. Group */
		ca_kh_mcast_flush_max_group_number();			/* and L2 HW entry */
		ca_kh_mcast_member_delete_all(0);
		return count;
	}

_dump_invalid:
	printk(KERN_WARNING "Invalid argument\n\n");
	printk(KERN_WARNING CA_KH_DUMP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_kh_dump __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_dump_proc_open,
	.read		= seq_read,
	.write		= ca_kh_dump_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'debug'
 ***************************************************************************/

#define CA_KH_DEBUG_HELP_MSG \
	"Purpose: Enable Kernel Hook Module Debug Message\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: Network Interface\n" \
	"  0x00000002: ARP / ICMPv6 Neighbor Discovery\n" \
	"  0x00000004: Route / NextHop\n" \
	"  0x00000008: NAT\n" \
	"  0x00000010: Tunnel (include PPPoE)\n" \
	"  0x00000020: Multicast\n" \
	"  0x00000040: VLAN\n" \
	"  0x00000080: Bypass Port List\n" \
	"\n"

static int ca_kh_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_debug);

	return 0;
}

static int ca_kh_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_debug_show_help, (void *) file);
}

static ssize_t ca_kh_debug_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char buf[32];
	unsigned long mask;
	ssize_t len;

	len = min(count, (unsigned long)(sizeof(buf) - 1));

	if (copy_from_user(buf, buffer, len)) {
		printk("%s: copy_from_user() failed!!\n", __func__);
		goto _debug_invalid;
	}

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &mask)) {
		printk("%s: kstrtoul() failed!!\n", __func__);
		goto _debug_invalid;
	}

	ca_kh_debug = mask;
	printk(KERN_INFO "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_debug);

	return count;

_debug_invalid:
	printk(KERN_WARNING "Invalid argument\n\n");
	printk(KERN_WARNING CA_KH_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_kh_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_debug_proc_open,
	.read		= seq_read,
	.write		= ca_kh_debug_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'qos_mode'
 ***************************************************************************/

#define CA_KH_QOS_MODE_HELP_MSG \
	"Setup QOS_MODE in Linux reference design\n" \
	"READ Usage: cat %s\n" \
	"		0: tail drop mode \n"\
	"		1: WRED mode \n"\
	"WRITE Usage: echo [mode] > %s\n" \
	"\n"
int ca_kh_qos_mode = 0;

static int ca_kh_qos_mode_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_QOS_MODE_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_qos_mode);

	return 0;
}

int ca_kh_qos_mode_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_qos_mode_show_help, (void *) file);
}

static ssize_t ca_kh_qos_mode_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char tmp[32];
	ssize_t len;
	ca_uint32_t 			new_qos_mode;
	ca_queue_wred_profile_t profile;
	ca_port_id_t            port_id;
	ca_uint32_t             queue;
	ca_status_t				ret;
	ca_uint32_t 			port;
	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buffer, len))
		goto _qos_mode_invalid;
	tmp[len] = '\0';

	if (kstrtou32(tmp, 0, &new_qos_mode))
		goto _qos_mode_invalid;

	if (new_qos_mode != ca_kh_qos_mode) {
		memset(&profile, 0, sizeof(ca_queue_wred_profile_t));
		if (new_qos_mode == 1) {
			profile.unmarked_dp[0] = 100;
			profile.marked_dp[0] = 100;
		}
		for (port = CA_PORT_ID_NI0; port <= CA_PORT_ID_NI7; port++){
			if (port == CA_PORT_ID_NI7)
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
				port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, port);
#else
				port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port);
#endif
			else
				port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port);
			for (queue = 0; queue < 8; queue++) {
				ret = ca_queue_wred_set(0, port_id, queue, &profile);

				if (ret != CA_E_OK)
					printk("%s error set port %x to qos_mode %d\n", __func__, port_id, new_qos_mode);
			}
		}
		for (port = CA_PORT_ID_CPU0; port <= CA_PORT_ID_CPU7; port++){
			port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, port);
			for (queue = 0; queue < 8; queue++) {
				ret = ca_queue_wred_set(0, port_id, queue, &profile);

				if (ret != CA_E_OK)
					printk("%s error set port %x to qos_mode %d\n", __func__, port_id, new_qos_mode);
			}
		}
		ca_kh_qos_mode = new_qos_mode;
		printk(KERN_WARNING "set %s value to %d.\n", file->f_path.dentry->d_iname, ca_kh_qos_mode);
	}

	return count;

_qos_mode_invalid:
	printk(KERN_WARNING "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_kh_qos_mode_proc __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_qos_mode_proc_open,
	.read		= seq_read,
	.write		= ca_kh_qos_mode_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'udp_offload_occasion'
 ***************************************************************************/

#define CA_KH_UDP_OFFLOAD_OCCASION_HELP_MSG \
	"Purpose: Decide when to create hash for UDP stream\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo NUMBER > %s\n" \
	"  0: Create 1st NAT hash (initial pkt direction) when two packets are received from initial pkt direction.\n" \
	"     Create 2nd NAT hash (reversed pkt direction) when one packet is received from reversed pkt direction.\n" \
	"\n" \
	"  1: Create two hashes when nf conntrack state becomes ASSURED.\n" \
	"     This requires one more packets of any direciton after bi-direcitonal packets are routed succesfully.\n" \
	"\n"

static int ca_kh_udp_offload_occasion_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_KH_UDP_OFFLOAD_OCCASION_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_udp_offload_occasion);

	return 0;
}

static int ca_kh_udp_offload_occasion_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ca_kh_udp_offload_occasion_show_help, (void *) file);
}

static ssize_t ca_kh_udp_offload_occasion_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char buf[32];
	unsigned long flag;
	ssize_t len;

	len = min(count, (unsigned long)(sizeof(buf) - 1));

	if (copy_from_user(buf, buffer, len)) {
		printk("%s: copy_from_user() failed!!\n", __func__);
		goto _debug_invalid;
	}

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &flag)) {
		printk("%s: kstrtoul() failed!!\n", __func__);
		goto _debug_invalid;
	}

	ca_kh_udp_offload_occasion = (flag == 0) ? 0 : 1;
	printk(KERN_INFO "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_kh_udp_offload_occasion);

	return count;

_debug_invalid:
	printk(KERN_WARNING "Invalid argument\n\n");
	printk(KERN_WARNING CA_KH_UDP_OFFLOAD_OCCASION_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_kh_udp_offload_occasion __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ca_kh_udp_offload_occasion_proc_open,
	.read		= seq_read,
	.write		= ca_kh_udp_offload_occasion_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * init/exit functions; create and remove the proc entries.
 ***************************************************************************/

int ca_kernel_hook_proc_init_module(void)
{
	int i;

	config_default_value();

	proc_kernel_hook = proc_mkdir("kernel_hook", ca_proc_dir);

	for (i = 0; i < CA_KH_WAN_PORT_VID_MAX; i++) {
		wan_port_vid_list[i].vid = 0xffffffff;
	}
	if (proc_create_data("wan_port_vid_list", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_wan_port_vid_list, NULL) == NULL) {
		pr_err("Fail to create proc entry 'wan_port_vid_list'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("tcp_bypass_port", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_tcp_bypass_port, NULL) == NULL) {
		pr_err("Fail to create proc entry 'tcp_bypass_port'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("udp_bypass_port", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_udp_bypass_port, NULL) == NULL) {
		pr_err("Fail to create proc entry 'udp_bypass_port'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("expected_master_bypass_port", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_expected_master_bypass_port, NULL) == NULL) {
		pr_err("Fail to create proc entry 'expected_master_bypass_port'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("hook", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_hook, NULL) == NULL) {
		pr_err("Fail to create proc entry 'hook'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("dump", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_dump, NULL) == NULL) {
		pr_err("Fail to create proc entry 'dump'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("debug", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_debug, NULL) == NULL) {
		pr_err("Fail to create proc entry 'debug'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("qos_mode", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_qos_mode_proc, NULL) == NULL) {
		pr_err("Fail to create proc entry 'qos_mode'.\n");
		return -ENOMEM;
	}

	if (proc_create_data("udp_offload_occasion", S_IRUGO | S_IWUGO, proc_kernel_hook, &fops_kh_udp_offload_occasion, NULL) == NULL) {
		pr_err("Fail to create proc entry 'udp_offload_occasion'.\n");
		return -ENOMEM;
	}

	return 0;
}

void ca_kernel_hook_proc_cleanup_module(void)
{
	remove_proc_entry("debug", proc_kernel_hook);
	proc_remove(proc_kernel_hook);
}

