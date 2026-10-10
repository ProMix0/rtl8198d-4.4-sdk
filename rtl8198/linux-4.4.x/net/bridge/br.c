/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/llc.h>
#include <net/llc.h>
#include <net/stp.h>
#include <net/switchdev.h>

#include <linux/proc_fs.h>

#include "br_private.h"

#if defined(CONFIG_LUNA)
#include <rtk/port.h>
#include <rtk/switch.h>
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include <module/intr_bcaster/intr_bcaster.h> 
#endif
#ifdef CONFIG_RTL9607C_SERIES
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#else
#include <dal/apollomp/dal_apollomp_switch.h>
#endif
#endif
#ifdef CONFIG_BRIDGE_RTK_IGMPV3_SNOOPING
#include "rtk_br_igmpv3_hook.h"
#endif


/*
 * Handle changes in state of network devices enslaved to a bridge.
 *
 * Note: don't care about up/down if bridge itself is down, because
 *     port state is checked when bridge is brought up.
 */
static int br_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct net_bridge_port *p;
	struct net_bridge *br;
	bool changed_addr;
	int err;

	/* register of bridge completed, add sysfs entries */
	if ((dev->priv_flags & IFF_EBRIDGE) && event == NETDEV_REGISTER) {
		br_sysfs_addbr(dev);
		return NOTIFY_DONE;
	}

	/* not a port of a bridge */
	p = br_port_get_rtnl(dev);
	if (!p)
		return NOTIFY_DONE;

	br = p->br;

	switch (event) {
	case NETDEV_CHANGEMTU:
		dev_set_mtu(br->dev, br_min_mtu(br));
		break;

	case NETDEV_CHANGEADDR:
		spin_lock_bh(&br->lock);
		br_fdb_changeaddr(p, dev->dev_addr);
		changed_addr = br_stp_recalculate_bridge_id(br);
		spin_unlock_bh(&br->lock);

		if (changed_addr)
			call_netdevice_notifiers(NETDEV_CHANGEADDR, br->dev);

		break;

	case NETDEV_CHANGE:
		br_port_carrier_check(p);
		break;

	case NETDEV_FEAT_CHANGE:
		netdev_update_features(br->dev);
		break;

	case NETDEV_DOWN:
		spin_lock_bh(&br->lock);
		if (br->dev->flags & IFF_UP)
			br_stp_disable_port(p);
		spin_unlock_bh(&br->lock);
		break;

	case NETDEV_UP:
		if (netif_running(br->dev) && netif_oper_up(dev)) {
			spin_lock_bh(&br->lock);
			br_stp_enable_port(p);
			spin_unlock_bh(&br->lock);
		}
		break;

	case NETDEV_UNREGISTER:
		br_del_if(br, dev);
		break;

	case NETDEV_CHANGENAME:
		err = br_sysfs_renameif(p);
		if (err)
			return notifier_from_errno(err);
		break;

	case NETDEV_PRE_TYPE_CHANGE:
		/* Forbid underlaying device to change its type. */
		return NOTIFY_BAD;

	case NETDEV_RESEND_IGMP:
		/* Propagate to master device */
		call_netdevice_notifiers(event, br->dev);
		break;
	}

	/* Events that may cause spanning tree to refresh */
	if (event == NETDEV_CHANGEADDR || event == NETDEV_UP ||
	    event == NETDEV_CHANGE || event == NETDEV_DOWN)
		br_ifinfo_notify(RTM_NEWLINK, p);

	return NOTIFY_DONE;
}

static struct notifier_block br_device_notifier = {
	.notifier_call = br_device_event
};

/* called with RTNL */
static int br_switchdev_event(struct notifier_block *unused,
			      unsigned long event, void *ptr)
{
	struct net_device *dev = switchdev_notifier_info_to_dev(ptr);
	struct net_bridge_port *p;
	struct net_bridge *br;
	struct switchdev_notifier_fdb_info *fdb_info;
	int err = NOTIFY_DONE;

	p = br_port_get_rtnl(dev);
	if (!p)
		goto out;

	br = p->br;

	switch (event) {
	case SWITCHDEV_FDB_ADD:
		fdb_info = ptr;
		err = br_fdb_external_learn_add(br, p, fdb_info->addr,
						fdb_info->vid);
		if (err)
			err = notifier_from_errno(err);
		break;
	case SWITCHDEV_FDB_DEL:
		fdb_info = ptr;
		err = br_fdb_external_learn_del(br, p, fdb_info->addr,
						fdb_info->vid);
		if (err)
			err = notifier_from_errno(err);
		break;
	}

out:
	return err;
}

static struct notifier_block br_switchdev_notifier = {
	.notifier_call = br_switchdev_event,
};

static void __net_exit br_net_exit(struct net *net)
{
	struct net_device *dev;
	LIST_HEAD(list);

	rtnl_lock();
	for_each_netdev(net, dev)
		if (dev->priv_flags & IFF_EBRIDGE)
			br_dev_delete(dev, &list);

	unregister_netdevice_many(&list);
	rtnl_unlock();

}

static struct pernet_operations br_net_ops = {
	.exit	= br_net_exit,
};

static const struct stp_proto br_stp_proto = {
	.rcv	= br_stp_rcv,
};

#if defined(CONFIG_LUNA)
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
static int lan_port_remapping[4] = {
        0,
        1,
        2,
        3,
};
#endif

static rtk_port_t get_hw_port_id(int intf)
{
	uint32 id, rev, sub;
	int32 port;

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	if(rtk_switch_phyPortId_get(lan_port_remapping[intf], &port) != 0 )
	{
		printk("%s rtk_switch_phyPortId_get failed!\n", __FUNCTION__);
		return -1;
	}
#else
	rtk_switch_version_get(&id, &rev, &sub);
	if(id == RTL9602C_CHIP_ID)
	{
		intf ^= 0x1;	/* port remapping */
	}
	else if(id == APOLLOMP_CHIP_ID && (sub == APPOLOMP_CHIP_SUB_TYPE_RTL9602 ||
		    sub == APPOLOMP_CHIP_SUB_TYPE_RTL9603) )
	{
		switch(intf)
		{
		case 0:
			intf = 3;
			break;
		case 1:
			intf = 2;
			break;
		case 2:
			intf = 1;
			break;
		case 3:
			intf = 0;
			break;
		}
	}
	else if(id == RTL9607C_CHIP_ID)
	{
		switch(intf)
		{
		case 0:
			intf = 3;
			break;
		case 1:
			intf = 2;
			break;
		case 2:
			intf = 1;
			break;
		case 3:
			intf = 0;
			break;
		}
	}

	if(rtk_switch_phyPortId_get(intf, &port) != 0 )
	{
		printk("%s rtk_switch_phyPortId_get failed!\n", __FUNCTION__);
		return -1;
	}
#endif

	return port;
}

void rtk_loopback_disable_intf(int intf)
{
	rtk_port_t port = -1;
    uint32 value;

	port = get_hw_port_id(intf);
	if(port == -1)
	{
		printk("%s: get_hw_port_id failed!\n", __FUNCTION__);
		return;
	}

	rtk_port_phyReg_get(port, 0, 0, &value);
	value |= 0x0800; /* Power down bit in standard PHY standard register */
	rtk_port_phyReg_set(port, 0, 0, value);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	//Inform OMCI
	//printk("queue_broadcast is called to alarm\n");
	queue_broadcast(MSG_TYPE_RLDP_LOOP_STATE_CHNG, 1/*RLDP_STS_LOOP_DETECTED*/, port, ENABLED);
#endif
    printk("Loopback detected intf %d \n" , intf);
}

void rtk_loopback_enable_intf(int intf)
{
    uint32 value;
    rtk_port_t port = -1;

	port = (rtk_port_t)get_hw_port_id(intf);
	if(port == -1)
    {
		printk("%s: get_hw_port_id failed!\n", __FUNCTION__);
            return;
    }

    rtk_port_phyReg_get(port, 0, 0, &value);
    value &= ~(0x0800);	/* Power down bit in standard PHY standard register */
    rtk_port_phyReg_set(port, 0, 0, value);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	//Inform OMCI
	//printk("queue_broadcast is called to alarm\n");
	queue_broadcast(MSG_TYPE_RLDP_LOOP_STATE_CHNG, 0/*RLDP_STS_LOOP_REMOVED*/, port, ENABLED);
#endif
	printk("Disable loopback intf %d \n" , intf);
}

struct proc_dir_entry *procLoopdetect= NULL;
int loopdetect_start = 0;
unsigned short loop_intf = 0x0;
unsigned short back_loop_intf = 0x0;

static void reset_loop_intf(void)
{
	int i = 0, port_loop = 0;
	loop_intf |= back_loop_intf;
	if(loop_intf == 0) return ;
	for(i=0;i<(sizeof(loop_intf)*8);i++)
	{
		port_loop = (loop_intf & 1<<i);
		if(port_loop)
			rtk_loopback_enable_intf(i);
	}
	loop_intf = 0x0;
	back_loop_intf = 0x0;
}

static int loop_detect_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "%hu\n", loop_intf);
	back_loop_intf |= loop_intf;
	loop_intf = 0;
	return 0;
}

static int loop_detect_open(struct inode *inode, struct file *file)
{
	return single_open(file, loop_detect_read, inode->i_private);
}

static int loop_detect_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
		char buffer[64] = {0};

        if (buf && !copy_from_user(buffer, buf, count))
        {    //call from shell
                #define REG32(reg)      (*(volatile unsigned int   *)((unsigned int)reg))
				if(memcmp(buffer , "startbyoam" , 10) == 0 )
				{
					reset_loop_intf();
					loopdetect_start = 2;
					loop_intf = 0x0;
					back_loop_intf = 0x0;
					printk("oam enable loopback detect \n");
				}
				else if(memcmp(buffer , "start" , 5) == 0 )
                {
					reset_loop_intf();
					loopdetect_start = 1;
					//loopback_detectd = 0;
					loop_intf = 0x0;
					back_loop_intf = 0x0;
					printk("enable loopback detect \n");
                }
                else if(memcmp(buffer, "enableg" , 7) == 0)
                {
                        enable_irq(26);
                }
                else if(memcmp(buffer, "disableg" , 8) == 0)
                {
                        disable_irq(26);
                }
                else if(memcmp(buffer, "stop" , 4) == 0)
                {
					reset_loop_intf();
					loopdetect_start = 0;
					printk("disable loopback detect \n");
                }
				else if(memcmp(buffer , "enableport" , 10) == 0 )
                {
					if(strlen(buffer) >= 12 && memcmp(buffer+11, "eth0.", 5) == 0 && (*(buffer+16) - '0') >= 0)
					{
						if((back_loop_intf & (1 << (*(buffer+16) - '0' - 2))))
						{
							rtk_loopback_enable_intf((*(buffer+16) - '0' - 2));
							back_loop_intf &= ~(1 << (*(buffer+16) - '0' - 2));
						}
					}
                }
                else
                {
                        printk("buffer:%s , len :%d \n" , buffer , strlen(buffer));
                        return -EFAULT;
                }
        }
        return count;
}

static const struct file_operations loopdetect_fops = {
	.owner          = THIS_MODULE,
	.open           = loop_detect_open,
	.read           = seq_read,
	.write          = loop_detect_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
static struct ctl_table loopdetect_table[] = {
	{
			.procname		= "lan_port_remapping",
			.data			= lan_port_remapping,
			.maxlen 		= 4*sizeof(int),
			.mode			= 0644,
			.proc_handler	= proc_dointvec,
	},
	{}
};

static struct ctl_table loopdetect_root[] = {
	{
        .procname       = "loopback_detect",
        .mode           = 0555,
        .child          = loopdetect_table,
	},
	{}
};

static struct ctl_table_header *loopdetect_tbl_hdr = NULL;
#endif

#endif

static int __init br_init(void)
{
	int err;

	BUILD_BUG_ON(sizeof(struct br_input_skb_cb) > FIELD_SIZEOF(struct sk_buff, cb));

	err = stp_proto_register(&br_stp_proto);
	if (err < 0) {
		pr_err("bridge: can't register sap for STP\n");
		return err;
	}

	err = br_fdb_init();
	if (err)
		goto err_out;

	err = register_pernet_subsys(&br_net_ops);
	if (err)
		goto err_out1;

	err = br_nf_core_init();
	if (err)
		goto err_out2;

	err = register_netdevice_notifier(&br_device_notifier);
	if (err)
		goto err_out3;

	err = register_switchdev_notifier(&br_switchdev_notifier);
	if (err)
		goto err_out4;

	err = br_netlink_init();
	if (err)
		goto err_out5;

	brioctl_set(br_ioctl_deviceless_stub);

#if IS_ENABLED(CONFIG_ATM_LANE)
	br_fdb_test_addr_hook = br_fdb_test_addr;
#endif

#if defined(CONFIG_LUNA)
	procLoopdetect= proc_create("loopback_detect", 0644, NULL, &loopdetect_fops);
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	loopdetect_tbl_hdr = register_sysctl_table(loopdetect_root);
#endif
#endif
	pr_info("bridge: automatic filtering via arp/ip/ip6tables has been "
		"deprecated. Update your scripts to load br_netfilter if you "
		"need this.\n");
#ifdef CONFIG_BRIDGE_RTK_IGMPV3_SNOOPING
	rtk_igmpv3_database_hook_init();
#endif
	return 0;

err_out5:
	unregister_switchdev_notifier(&br_switchdev_notifier);
err_out4:
	unregister_netdevice_notifier(&br_device_notifier);
err_out3:
	br_nf_core_fini();
err_out2:
	unregister_pernet_subsys(&br_net_ops);
err_out1:
	br_fdb_fini();
err_out:
	stp_proto_unregister(&br_stp_proto);
	return err;
}

static void __exit br_deinit(void)
{
	stp_proto_unregister(&br_stp_proto);

#if defined(CONFIG_LUNA)
    if(procLoopdetect)
    {
		remove_proc_entry("loopback_detect", NULL);
		procLoopdetect = NULL;
    }
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	if(loopdetect_tbl_hdr){
		unregister_sysctl_table(loopdetect_tbl_hdr);
		loopdetect_tbl_hdr = NULL;
	}
#endif
#endif
	br_netlink_fini();
	unregister_switchdev_notifier(&br_switchdev_notifier);
	unregister_netdevice_notifier(&br_device_notifier);
	brioctl_set(NULL);
	unregister_pernet_subsys(&br_net_ops);

	rcu_barrier(); /* Wait for completion of call_rcu()'s */

	br_nf_core_fini();
#if IS_ENABLED(CONFIG_ATM_LANE)
	br_fdb_test_addr_hook = NULL;
#endif
	br_fdb_fini();
#ifdef CONFIG_BRIDGE_RTK_IGMPV3_SNOOPING
	rtk_igmpv3_database_hook_exit();
#endif
}

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
MODULE_VERSION(BR_VERSION);
MODULE_ALIAS_RTNL_LINK("bridge");
