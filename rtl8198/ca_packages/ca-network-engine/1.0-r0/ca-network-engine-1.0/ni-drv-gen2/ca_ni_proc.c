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
 * ca_ni_proc.c: Proc entry for Cortina NI driver
 *
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <soc/cortina/cortina-soc.h>
#include "ca_types.h"
#include "osal_common.h"
#include "ca_mut.h"
#include "ca_ni_proc.h"

#if defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#include "ca_ni.h"
#endif
#include "../ca-ne-rtk/rtk_func.h"
#endif

#if defined(CONFIG_RTL_FPGA_PHY_TEST)
#include "flow.h"
ca_ni_rx_tx_test_t ni_rx_tx_test;
#endif

#define CA_NE_NI_HELP_MSG \
	"Purpose: Enable NI driver debug message\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: NI debug message\n" \
	"  0x00000002: IRQ debug message\n" \
	"  0x00000004: DMA LSO debug message\n" \
	"  0x00000008: Dump RX packets\n" \
	"  0x00000010: Dump TX packets\n" \
	"  0x00000020: IOCTL debug message\n" \
	"  0x00000040: Virtual NI debug message\n" \
	"  0x00000080: Ethtool debug message\n" \
	"  0x00000100: PHY debug message\n" \
	"  0x00000200: NI RX NETLINK debug message\n" \
	"  0x80000000: Dump netlink list\n" \
	"  0x40000000: Dump dma_lso list\n" \
	"  0x20000000: Dump Netlink RX queue\n"

#if defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#define CA_NE_NI_DEV_PORTMAPPING_HELP_MSG \
	"Purpose: Configure NI driver multiple LAN device port mapping\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [num_of_port dev_name] > %s\n"
#endif	//CONFIG_RTL_MULTI_LAN_DEV
#endif

struct proc_dir_entry *proc_ni = NULL;

extern u32 ca_ni_debug;

static int ni_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_NE_NI_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_ni_debug);

	return 0;
}

int ni_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ni_debug_show_help, (void *) file);
}

void ca_ni_netlink_dump_pid_seq_list(void);
void ca_rx_register_pkt_dump_all(void);
void ca_ni_dma_lso_dump(void);
void ca_ni_netlink_rx_queue_dump(void);

ssize_t ni_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long mask;
	ssize_t len;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _ni_debug_invalid;

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &mask))
#else
	if (kstrtoul(tmp, 0, &mask))
#endif
		goto _ni_debug_invalid;

	ca_ni_debug = mask;

	if (ca_ni_debug == NI_DBG_NETLINK_DUMP) {
		ca_ni_netlink_dump_pid_seq_list();
		ca_rx_register_pkt_dump_all();
	} else if (ca_ni_debug == NI_DBG_DMA_LSO_DUMP){
		ca_ni_dma_lso_dump();
	}
	else if (ca_ni_debug == NI_DBG_NETLINK_RX_QUEUE_DUMP) {
		ca_ni_netlink_rx_queue_dump();
	} else {
		printk(  "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_ni_debug);
		pr_info("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_ni_debug);
	}
	return count;

_ni_debug_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_ni_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ni_debug_proc_open,
	.read		= seq_read,
	.write		= ni_debug_proc_write,
};

static u16 ca_ni_cpu_port=0xffff;
static u16 ca_ni_tx_ldpid=0xffff;
#define CA_NE_TX_LDPID_HELP_MSG \
        "Purpose: Set tx_ldpid for cpu_port\n" \
        "READ Usage: cat %s\n" \
        "WRITE Usage: echo {cpu_port tx_ldpid} > %s\n" \
	"      Example: echo \"0x2 0x19\" > tx_ldpid\n" \

static int tx_ldpid_show_help(struct seq_file *m, void *v)
{
        struct file *file = m->private;

        seq_printf(m, CA_NE_TX_LDPID_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
        seq_printf(m, "\ncpu_port = 0x%08x, %s = 0x%08x\n", ca_ni_cpu_port, file->f_path.dentry->d_iname, ca_ni_tx_ldpid);

        return 0;
}

int tx_ldpid_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, tx_ldpid_show_help, (void *) file);
}

ca_status_t ca_ni_tx_set_ldpid(u16 cpu_port, u16 tx_ldpid);
ssize_t tx_ldpid_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
        char tmp[32];
        unsigned long mask;
        ssize_t len;
	u32	cpu_port;
	u32	tx_ldpid;

        len = min(count, (sizeof(tmp) - 1));
        if (copy_from_user(tmp, buf, len))
                goto _tx_ldpid_invalid;

        tmp[len] = '\0';

	printk("%s: %s\n", __func__, tmp);
	sscanf(tmp, "0x%x 0x%x\n", &cpu_port, &tx_ldpid);
	printk("%s: cpu_port=0x%x, tx_ldpid=0x%x\n", __func__, cpu_port, tx_ldpid);

	ca_ni_cpu_port = cpu_port;
	ca_ni_tx_ldpid = tx_ldpid;

	ca_ni_tx_set_ldpid(ca_ni_cpu_port, ca_ni_tx_ldpid);
        return count;

_tx_ldpid_invalid:
        printk("ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
        /* if we return error code here, PROC fs may retry up to 3 times. */
        return count;
}

static const struct file_operations fops_tx_ldpid __read_mostly = {
        .owner          = THIS_MODULE,
        .open           = tx_ldpid_proc_open,
        .read           = seq_read,
        .write          = tx_ldpid_proc_write,
};


#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTL_MULTI_LAN_DEV)
#define TOTAL_CA_NI_MULTI_LAN_DEV_NUM	((sizeof(ca_ni_multi_lan_dev_table))/(sizeof(struct ca_ni_multi_lan_dev_table_entry)))
extern struct ca_ni_multi_lan_dev_table_entry ca_ni_multi_lan_dev_table[MAX_CA_NI_MULTI_LAN_DEV_NUM];
extern int ca_ni_num_intf;
int ni_dev_port_mapping_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}

static int ni_dev_port_mapping_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	unsigned int totalDev = TOTAL_CA_NI_MULTI_LAN_DEV_NUM;
	ca_eth_private_t *rootCep, *cep;
	int i, j;
	
	printk("DEV ability:\n");
	for(i=0;i<totalDev;i++){
		if(ca_ni_multi_lan_dev_table[i].dev_instant)			
			printk(" %s\n", ca_ni_multi_lan_dev_table[i].dev_instant->name);
	}
	printk("\n");

	printk("rx: phyPort -> dev[the packet rx from phyPort will send to kernel using dev]\n");
#if defined(CONFIG_NE_MANAGEMENT_MODE)
		rootCep = netdev_priv(ni_info_data.dev[CPU_PORT0_INST]);
		if(!(rootCep)){
			printk(KERN_INFO "Null data\n");
			return 0;
		}
		printk("Root DEV: %s\n", i, ni_info_data.dev[CPU_PORT0_INST]->name);
		if(!(rootCep)){
			printk(KERN_INFO "Null data\n");
			return 0;
		}
		for(i=0;i<CA_MAX_ETHERNET_PORT_NUMBER;i++){
			if(rootCep->port2dev[i])
				printk("port%d -> %s\n", i, rootCep->port2dev[i]->name);
		}
		printk("\n");
#else
		for (i=0;i<ca_ni_num_intf;i++) {
			rootCep = netdev_priv(ni_info_data.dev[i]);
			if(!(rootCep)){
				printk(KERN_INFO "Null data\n");
				continue;
			}
			switch (i) {
				case 0 ... 7:
					printk("Root DEV: %s\n", ni_info_data.dev[i]->name);
					for(j=0;j<CA_MAX_ETHERNET_PORT_NUMBER;j++){
						if(rootCep->port2dev[j])
							printk("port%d -> %s\n", j, rootCep->port2dev[j]->name);
					}
					printk("\n");
					continue;
				default:
					printk("%s: i=%d, should not happend\n", __func__, i);
					break;
			}
		}
#endif
	
	printk("tx:dev -> txPortMask[when tx from dev, we will use this txPortMask]\n");
	for(i=0;i<totalDev;i++){
		if(ca_ni_multi_lan_dev_table[i].dev_instant){
			cep = netdev_priv(ca_ni_multi_lan_dev_table[i].dev_instant);
			if(!(cep)){
				printk(KERN_INFO "Null data\n");
				return 0;
			}
			printk("%s -> port %d\n",
				ca_ni_multi_lan_dev_table[i].dev_instant->name, cep->port_cfg.tx_ldpid);
		}
	}
	printk("\n");

	printk("=============================================================\n");
	printk(CA_NE_NI_DEV_PORTMAPPING_HELP_MSG, filp->f_path.dentry->d_iname, filp->f_path.dentry->d_iname);
	printk("=============================================================\n");
	
	return 0;
}

extern int ca_ni_change_dev_port_mapping(ca_eth_private_t *cep, int port_num, char *name);
ssize_t ni_dev_port_mapping_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	ca_eth_private_t *rootCep;
	char		tmpbuf[64];
	char		*strptr;
	unsigned int port_num;
	char		*tokptr;
	int i;
	
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		port_num = simple_strtol(tokptr, NULL, 0);
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			printk("port %d ", port_num);
			if(tokptr[strlen(tokptr) - 1] == 0x0a)
				tokptr[strlen(tokptr) - 1] = 0x00;
			printk("assign to %s\n", tokptr);	
#if defined(CONFIG_NE_MANAGEMENT_MODE)
				rootCep = netdev_priv(ni_info_data.dev[CPU_PORT0_INST]);
				if(!(rootCep)){
					printk(KERN_INFO "Null data\n");
					return count;
				}
				printk("mapping root DEV=%s to port %d\n", ni_info_data.dev[CPU_PORT0_INST]->name, port_num);
				ca_ni_change_dev_port_mapping(rootCep, port_num, tokptr);
#else
				for (i=0;i< ca_ni_num_intf;i++) {
					rootCep = netdev_priv(ni_info_data.dev[i]);
					if(!(rootCep)){
						printk(KERN_INFO "Null data\n");
						continue;
					}
					switch (i) {
						case 0 ... 7:
							printk("mapping root DEV=%s virtual DEV=%s to port %d\n", ni_info_data.dev[i]->name, tokptr, port_num);
							ca_ni_change_dev_port_mapping(rootCep, port_num, tokptr);
							break;
						default:
							printk("%s: i=%d, should not happend\n", __func__, i);
							break;
					}
				}
#endif
		}
	}
	else
	{
errout:
		printk("=============================================================\n");
		printk(CA_NE_NI_DEV_PORTMAPPING_HELP_MSG, filp->f_path.dentry->d_iname, filp->f_path.dentry->d_iname);
		printk("=============================================================\n");
	}
	return count;
}


static const struct file_operations fops_ni_dev_port_mapping __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ni_dev_port_mapping_open,
	.read		= ni_dev_port_mapping_read,
	.write		= ni_dev_port_mapping_write,
	.llseek     = seq_lseek,
    .release    = single_release,
};
#endif
#if defined(CONFIG_LUNA_G3_SERIES)
static void memDump (void *start, u32 size, char * strHeader)
{
	int row, column, index, index2, max;
//	uint32 buffer[5];
	u8 *buf, *line, ascii[17];
	char empty = '.';

	if(!start ||(size==0))
		return;
	line = (u8*)start;

	/*
	16 bytes per line
	*/
	if (strHeader)
		printk("%s", strHeader);
	column = size % 16;
	row = (size / 16) + 1;
	for (index = 0; index < row; index++, line += 16) 
	{
		buf = line;

		memset (ascii, 0, 17);

		max = (index == row - 1) ? column : 16;
		if ( max==0 ) break; /* If we need not dump this line, break it. */

		printk("\n%08x ", (u32) line);
		
		//Hex
		for (index2 = 0; index2 < max; index2++)
		{
			if (index2 == 8)
			printk("  ");
			printk("%02x ", (u8) buf[index2]);
			ascii[index2] = (((u8) buf[index2] < 32) || ((u8) buf[index2] >= 128)) ? empty : buf[index2];
		}

		if (max != 16)
		{
			if (max < 8)
				printk("  ");
			for (index2 = 16 - max; index2 > 0; index2--)
				printk("   ");
		}

		//ASCII
		printk("  %s", ascii);
	}
	printk("\n");
	return;
}

static int ni_memory_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, "echo r address len > proc/driver/cortina/ni/memory\n");
	seq_printf(m, "echo w address len value > proc/driver/cortina/ni/memory\n");

	return 0;
}

static int ni_memory_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ni_memory_show_help, (void *) file);
}

static ssize_t ni_memory_proc_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
	char			tmpbuf[64];
	unsigned long	*mem_addr, mem_data, mem_len;
	char			*strptr, *cmd_str;
	char			*tokptr;

	if (buff && !copy_from_user(tmpbuf, buff, len))
	{
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		cmd_str = strsep(&strptr," ");
		if (cmd_str==NULL)
		{
			goto errout;
		}
		printk("cmd %s\n", cmd_str);
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}

		if (!memcmp(cmd_str, "r", 1))
		{
			mem_addr=(unsigned long *)simple_strtoll(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtoll(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "");
		}
		else if (!memcmp(cmd_str, "w", 1))
		{
			mem_addr=(unsigned long *)simple_strtoll(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_data=simple_strtoll(tokptr, NULL, 0);
			*mem_addr = mem_data;
			printk("Write memory 0x%p dat 0x%x: 0x%x\n", mem_addr, mem_data, *mem_addr);
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("Memory operation only support \"r\" and \"w\" as the first parameter\n");
		printk("Read format:	\"r mem_addr length\"\n");
		printk("Write format:	\"w mem_addr mem_data\"\n");
	}

	return len;
}

static const struct file_operations fops_ni_memory __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ni_memory_proc_open,
	.read		= seq_read,
	.write		= ni_memory_proc_write,
	.release	= single_release,
};

#if defined(CONFIG_RTL_FPGA_PHY_TEST)

static int ni_rx_tx_test_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, "\nUsage:\n");
	seq_printf(m, "echo <mode mode_value> [option_key option_value] > proc/driver/cortina/ni/ni_rx_tx_test\n");
	seq_printf(m, "<mode> 0: disable, 1: Test mode \n");
	seq_printf(m, "<option_key> list as below for different purpose \n");
	seq_printf(m, "\t [Remarking for checksum verification] remarking_dip\n");
	seq_printf(m, "\t [Direct tx] ldpid, sw_id, aftfib_id, cos, bypass_fwd, chksum_ipv4, chksum_ipv6, chksum_udp, chksum_tcp, chksum_auto, lenfix_en\n");
	seq_printf(m, "\t [Hw lookup] lspid, ldpid, sw_id, bypass_fwd\n");
	seq_printf(m, "\t [Hash] key_ip_protocol, key_ip_sa, key_ip_da, key_l4_sport, key_l4_dport, key_o_lspid, act_ldpid, act_dscp, act_sw_id\n");
	seq_printf(m, "\t [For to-wifi verification] verify_ldpid\n");

	seq_printf(m, "\nCurrent Config:\n");
	seq_printf(m, "\t mode %d\n", ni_rx_tx_test.test_mode);
	if(ni_rx_tx_test.test_mode > 0){
		if(ni_rx_tx_test.remarking_dip > 0)
			seq_printf(m, "\t remarking_dip 0x%x\n", ni_rx_tx_test.remarking_dip);

		seq_printf(m, "\t lspid 0x%x, ldpid 0x%x, sw_id %d, cos %d, lenfix_en %d, ", 
			ni_rx_tx_test.ni_tx_config.core_config.bf.lspid, ni_rx_tx_test.ni_tx_config.core_config.bf.ldpid, ni_rx_tx_test.ni_tx_config.flow_id, ni_rx_tx_test.ni_tx_config.core_config.bf.txq_index, ni_rx_tx_test.ni_tx_lsopara0.bf.lenfix_en);
		if(ni_rx_tx_test.ni_tx_config.dma_aft_l2fib_enable)
			seq_printf(m, "aftfib_en %d, aftfib_id %d, ", ni_rx_tx_test.ni_tx_config.dma_aft_l2fib_enable, ni_rx_tx_test.ni_tx_config.dma_aft_l2fib_index);
		if(ni_rx_tx_test.ni_tx_config.core_config.bf.bypass_fwd_engine)
			seq_printf(m, "bypass_fwd %d, ", ni_rx_tx_test.ni_tx_config.core_config.bf.bypass_fwd_engine);
		if(ni_rx_tx_test.ni_tx_lsopara0.bf.ipv4_en)
			seq_printf(m, "chksum_ipv4 %d, ", ni_rx_tx_test.ni_tx_lsopara0.bf.ipv4_en);
		if(ni_rx_tx_test.ni_tx_lsopara0.bf.ipv6_en)
			seq_printf(m, "chksum_ipv6 %d, ", ni_rx_tx_test.ni_tx_lsopara0.bf.ipv6_en);
		if(ni_rx_tx_test.ni_tx_lsopara0.bf.tcp_en)
			seq_printf(m, "chksum_tcp %d, ", ni_rx_tx_test.ni_tx_lsopara0.bf.tcp_en);
		if(ni_rx_tx_test.ni_tx_lsopara0.bf.udp_en)
			seq_printf(m, "chksum_udp %d, ", ni_rx_tx_test.ni_tx_lsopara0.bf.udp_en);
		if(ni_rx_tx_test.ni_tx_lsopara0.bf.reserved)
			seq_printf(m, "chksum_auto %d, ", ni_rx_tx_test.ni_tx_lsopara0.bf.reserved);
		seq_printf(m, "\n");

		if(ni_rx_tx_test.hash_key_ip_protocol)
			seq_printf(m, "\t key_ip_protocol %d, key_ip_sa 0x%x, key_ip_da 0x%x, key_l4_sport %d, key_l4_dport %d, key_o_lspid %d, act_ldpid %d, act_dscp %d, act_sw_id %d \n", 
				ni_rx_tx_test.hash_key_ip_protocol, ni_rx_tx_test.hash_key_ip_sa, ni_rx_tx_test.hash_key_ip_da, ni_rx_tx_test.hash_key_l4_sport, ni_rx_tx_test.hash_key_l4_dport, ni_rx_tx_test.hash_key_o_lspid, ni_rx_tx_test.hash_act_ldpid, ni_rx_tx_test.hash_act_dscp, ni_rx_tx_test.hash_act_sw_id[0]);
		if(ni_rx_tx_test.hash_index != 0xFFFFFFFF)
			seq_printf(m, "\t\t ((hash_index %d))\n",ni_rx_tx_test.hash_index);
		if(ni_rx_tx_test.verify_ldpid != 0xFFFFFFFF)
			seq_printf(m, "\t verify_ldpid: %d\n",ni_rx_tx_test.verify_ldpid);
	}

	seq_printf(m, "\nExample:\n");
	seq_printf(m, "\t 1. Disable ni rx tx test mode\n");
	seq_printf(m, "\t  - echo mode 0 > ni_rx_tx_test \n");
	seq_printf(m, "\t 2. Rx packet direct tx to port 1\n");
	seq_printf(m, "\t  - echo mode 1 lspid 0x10 ldpid 1 bypass_fwd 1 > ni_rx_tx_test \n");

	seq_printf(m, "\t 3. Port 0 to Wifi fast forward (will output to port 2 for verification) \n");
	seq_printf(m, "\t  - echo mode 1 key_ip_protocol 0x11 key_ip_sa 192.168.1.11 key_ip_da 192.168.1.12 key_l4_sport 11111 key_l4_dport 22222 key_o_lspid 0x0 act_ldpid 0x12  act_dscp 2 lspid 0x10 bypass_fwd 1 verify_ldpid 0x2 > ni_rx_tx_test \n");

	seq_printf(m, "\t 4. Wifi HW look-up + fast forward (will output to port 2 for verification) \n");
	seq_printf(m, "\t  - echo mode 1 lspid 0x12 ldpid 0x19 bypass_fwd 1 verify_ldpid 0x2 key_ip_protocol 0x11 key_ip_sa 192.168.1.11 key_ip_da 192.168.1.12 key_l4_sport 11111 key_l4_dport 22222 key_o_lspid 0x12 act_ldpid 0x15  act_dscp 3 > ni_rx_tx_test \n");

	seq_printf(m, "\t 5. MC fast forward(L2CLS+L2ARB) to Wifi (will output to port 2 for verification) \n");
	seq_printf(m, "\t  - echo mode 1 lspid 0x10 bypass_fwd 1 verify_ldpid 0x2 > ni_rx_tx_test \n");
	

	return 0;
}

static int ni_rx_tx_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, ni_rx_tx_test_show_help, (void *) file);
}
int ni_test_add_flow(void)
{
	ca_flow_t flow_config;
	ca_mac_addr_t sa[6] = {0x00,0xCC,0xBB,0xAA,0x01,0x05},
		da[6]= {0x00,0x00,0x00,0x00,0x01,0x00};

	int	wlan_dev_idx = 0, idx;
	int to_wifi = 0, ret=0;
	int ipv6 = 0;
	// unicast: main hash flow
	memset(&flow_config, 0, sizeof(flow_config));
	// common setting
	flow_config.aging_time = 30;
	flow_config.actions.forward = CA_CLASSIFIER_FORWARD_PORT;


	flow_config.key_type = 1;//RG_CA_FLOW_UC5TUPLE_US;	// napt or routing


	flow_config.key.l2_key.o_lspid = ni_rx_tx_test.hash_key_o_lspid;	// force enabled to support unknown sa trap by main hash

	
	
	// Key: L3
	flow_config.key.l3_key.ip_valid = TRUE;
	flow_config.key.l3_key.ip_version = 4;
	flow_config.key.l3_key.ip_protocol = ni_rx_tx_test.hash_key_ip_protocol; //ni_rx_tx_test.key_ip_protocol
	
	flow_config.key.l3_key.dscp = 0;// (pFlowPath5->in_tos >>2)&0x3f;

	if(ipv6){ // FIX ME
		flow_config.key.l3_key.ip_sa.afi = CA_IPV6;
		flow_config.key.l3_key.ip_da.afi = CA_IPV6;
		flow_config.key.l3_key.ip_sa.addr_len = 128;
		flow_config.key.l3_key.ip_da.addr_len = 128;
		flow_config.key.l3_key.ip_sa.ip_addr.ipv6_addr[0] =  (0x20010000);
		flow_config.key.l3_key.ip_sa.ip_addr.ipv6_addr[1] =  (0x00000000);
		flow_config.key.l3_key.ip_sa.ip_addr.ipv6_addr[2] =  (0x00000000);
		flow_config.key.l3_key.ip_sa.ip_addr.ipv6_addr[3] =  (0x00000001);
		flow_config.key.l3_key.ip_da.ip_addr.ipv6_addr[0] =  (0x20010000);
		flow_config.key.l3_key.ip_da.ip_addr.ipv6_addr[1] =  (0x00000001);
		flow_config.key.l3_key.ip_da.ip_addr.ipv6_addr[2] =  (0x00000001);
		flow_config.key.l3_key.ip_da.ip_addr.ipv6_addr[3] =  (0x00000001);

	}else{
		flow_config.key.l3_key.ip_sa.afi = CA_IPV4;
		flow_config.key.l3_key.ip_da.afi = CA_IPV4;
		flow_config.key.l3_key.ip_sa.ip_addr.ipv4_addr = (ni_rx_tx_test.hash_key_ip_sa);//htonl(0xc0a80102);//pFlowPath5->in_src_ipv4_addr;  htonl(ni_rx_tx_test.hash_key_ip_sa);
		flow_config.key.l3_key.ip_sa.addr_len = 32;


		flow_config.key.l3_key.ip_da.ip_addr.ipv4_addr = (ni_rx_tx_test.hash_key_ip_da); //htonl(0xc0a80101); // 192.168.1.1  htonl(ni_rx_tx_test.hash_key_ip_da);
		flow_config.key.l3_key.ip_da.addr_len = 32;
		
	}

	// Key: L4
	flow_config.key.l4_key.src_l4_port = (ni_rx_tx_test.hash_key_l4_sport); // 22222   htons(0x56CE)
	flow_config.key.l4_key.dst_l4_port = (ni_rx_tx_test.hash_key_l4_dport); // 11111   htons(0x2B67) 
/*
	// Action: L2
	{
		// routing
		flow_config.actions.options.masks.decrement_ttl = TRUE;

		flow_config.actions.options.masks.mac_sa = TRUE;
		memcpy(&flow_config.actions.options.mac_sa, &sa[0], 6);

		flow_config.actions.options.masks.mac_da = TRUE;
		memcpy(&flow_config.actions.options.mac_da, &da[0], 6);
	}

*/
	flow_config.actions.options.masks.outer_vlan_act = TRUE;
	flow_config.actions.options.outer_vlan_act = CA_FLOW_VLAN_ACTION_POP;


	
	// Action: L3 L4
	flow_config.actions.options.masks.dscp = ni_rx_tx_test.hash_act_dscp_msk;
	flow_config.actions.options.dscp =  ni_rx_tx_test.hash_act_dscp;
/*
		if(pFlowPath5->out_l4_act){
			// NAPT
			if(pFlowPath5->out_l4_direction){
				// outbound
				flow_config.actions.options.masks.ip_sa = TRUE;
				flow_config.actions.options.masks.src_l4_port = TRUE;
				flow_config.actions.options.ip_sa.afi = CA_IPV4;
				flow_config.actions.options.ip_sa.ip_addr.ipv4_addr = fc_db.netifHwTbl[pFlowPath5->out_intf_idx].intf.gateway_ipv4_addr;
				flow_config.actions.options.ip_sa.addr_len = 32;
				flow_config.actions.options.src_l4_port = pFlowPath5->out_l4_port;
			}else{
				// inbound
				flow_config.actions.options.masks.ip_da = TRUE;
				flow_config.actions.options.masks.dst_l4_port = TRUE;
				flow_config.actions.options.ip_da.afi = CA_IPV4;
				flow_config.actions.options.ip_da.ip_addr.ipv4_addr = pFlowPath5->out_dst_ipv4_addr;
				flow_config.actions.options.ip_da.addr_len = 32;
				flow_config.actions.options.dst_l4_port = pFlowPath5->out_l4_port;
			}
		}
*/
		// Action: Others
		flow_config.actions.options.masks.cos = 0;
		flow_config.actions.options.cos = 0;

		flow_config.actions.dest.port = ni_rx_tx_test.hash_act_ldpid;

		flow_config.actions.options.masks.sw_id = TRUE;
		flow_config.actions.options.sw_id[1] = 0;

		flow_config.actions.options.masks.flow_id = 0;
		flow_config.actions.options.flow_id = 0;

#if defined(CONFIG_FC_CA8277B_SERIES)
		//policer_3: flow meter
		flow_config.actions.options.masks.flow_id3 = 0;
		flow_config.actions.options.flow_id3 = 0;

		//policer_2: flow mib
		flow_config.actions.options.masks.flow_id2 = 0;
		flow_config.actions.options.flow_id2 = 0;
#endif

/* FOR WIFI*/
		if(ni_rx_tx_test.hash_act_sw_id[0]/*(1<<(flow_config.actions.dest.port))&RTK_FC_ALL_MAC_WLANCPU_PORTMASK*/){
			// shared port fast forward
			flow_config.actions.options.masks.sw_id = TRUE;
			flow_config.actions.options.sw_id[0] = ni_rx_tx_test.hash_act_sw_id[0];
		}

#if 0// defined(CONFIG_FC_CA8277B_SERIES)
#if defined(CONFIG_RG_G3_WAN_PORT_INDEX)&&(CONFIG_RG_G3_WAN_PORT_INDEX==7)
		if(pFlowPath5->out_stream_idx_act && fc_db.streamidTbl[pFlowPath5->out_stream_idx].valid
			&& (fc_db.wanPortMask.portmask & (1 << flow_config.actions.dest.port))) {
			flow_config.actions.options.masks.gemId_mapping_mode = TRUE;
			flow_config.actions.options.masks.tcont_id = TRUE;
			flow_config.actions.options.masks.gem_id = TRUE;
			flow_config.actions.options.masks.cos = TRUE;
			
			flow_config.actions.options.gemId_mapping_mode = 1;
			flow_config.actions.options.tcont_id = fc_db.streamidTbl[pFlowPath5->out_stream_idx].ldpid;
			flow_config.actions.options.gem_id = fc_db.streamidTbl[pFlowPath5->out_stream_idx].gemid;
			flow_config.actions.options.cos = fc_db.streamidTbl[pFlowPath5->out_stream_idx].cos;
		}
#endif
#endif

	ret = ca_flow_add(0, &flow_config);

	if(ret == CA_E_OK){
		idx = flow_config.index;
		//printk(">>>>> Add Main Hash flow[%d]\n", flow_config.index);
		//ca_flow_age_set(0, flow_config.index, G3_FLOW_AGE_STATE_IDLE);	// the aging time starts from flowSyncPeriod_unit1s
	}else if(ret==CA_E_FULL){
		idx = 0xFFFFFFFF;
		ret = CA_E_OK;
		printk(">>>>> Main Hash table is full\n");
	}else{
		printk(">>>>> Add Main Hash  flow type[%d] fail, ret = 0x%x\n", flow_config.key_type, ret);
	}

	return idx;

}

static ssize_t ni_rx_tx_test_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
	char			tmpbuf[256];
	char			*strptr, *split_str;
	int ret = 0;
	if (!(buff && !copy_from_user(tmpbuf, buff, len)))
		goto errout;

	tmpbuf[len] = '\0';
	strptr=tmpbuf;

	split_str=strsep(&strptr," ");
	if(strptr == NULL)
		goto errout;

	if(ni_rx_tx_test.hash_index!=0xFFFFFFFF)
	{
		ret = ca_flow_delete(0, ni_rx_tx_test.hash_index);
		if(ret != CA_E_OK)
			printk(">>>>> Delete Main Hash flow[%d] fail, ret = 0x%x", ni_rx_tx_test.hash_index, ret);
	}
	//default initialize
	memset(&ni_rx_tx_test, 0, sizeof(ca_ni_rx_tx_test_t));
	ni_rx_tx_test.hash_index = 0xFFFFFFFF;
	ni_rx_tx_test.verify_ldpid = 0xFFFFFFFF;
	ni_rx_tx_test.ni_tx_config.core_config.bf.is_from_ca_tx = TRUE;

	do{
		if(strcasecmp(split_str,"mode")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.test_mode=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"remarking_dip")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.remarking_dip=simple_strtol(split_str, NULL, 16);
		}else if(strcasecmp(split_str,"lspid")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.core_config.bf.lspid=simple_strtol(split_str, NULL, 16);
		}else if(strcasecmp(split_str,"ldpid")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.core_config.bf.ldpid=simple_strtol(split_str, NULL, 16);
		}else if(strcasecmp(split_str,"sw_id")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.flow_id=simple_strtol(split_str, NULL, 16);
			if(ni_rx_tx_test.ni_tx_config.flow_id)
				ni_rx_tx_test.ni_tx_config.core_config.bf.flow_id_set=TRUE;
		}else if(strcasecmp(split_str,"chksum_ipv4")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.ipv4_en=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"chksum_ipv6")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.ipv6_en=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"chksum_tcp")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.tcp_en=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"chksum_udp")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.udp_en=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"chksum_auto")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.reserved=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"lenfix_en")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_lsopara0.bf.lenfix_en=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"aftfib_en")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.dma_aft_l2fib_enable=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"aftfib_id")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.dma_aft_l2fib_index=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"cos")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.core_config.bf.txq_index=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"bypass_fwd")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.ni_tx_config.core_config.bf.bypass_fwd_engine=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"key_ip_protocol")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_key_ip_protocol=simple_strtol(split_str, NULL, 0);
		}else if(strcasecmp(split_str,"key_l4_sport")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_key_l4_sport=simple_strtol(split_str, NULL, 0);
			//printk("hash_key_l4_sport: %x \n",ni_rx_tx_test.hash_key_l4_sport);
		}else if(strcasecmp(split_str,"key_l4_dport")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_key_l4_dport=simple_strtol(split_str, NULL, 0);
			//printk("hash_key_l4_dport: %x \n",ni_rx_tx_test.hash_key_l4_dport);
		}else if(strcasecmp(split_str,"key_ip_sa")==0){
			char *ip_token,*split_ip_token,j;
			split_str=strsep(&strptr," ");
			
			ip_token=split_str;
			ni_rx_tx_test.hash_key_ip_sa=0;
			for(j=0;j<4;j++)
			{
				split_ip_token=strsep(&ip_token,".");
				ni_rx_tx_test.hash_key_ip_sa|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
				if(ip_token==NULL) break;
			}
			
			//printk("hash_key_ip_sa: %x \n",ni_rx_tx_test.hash_key_ip_sa);
		}else if(strcasecmp(split_str,"key_ip_da")==0){
			char *ip_token,*split_ip_token,j;
			split_str=strsep(&strptr," ");
			
			ip_token=split_str;
			ni_rx_tx_test.hash_key_ip_da=0;
			for(j=0;j<4;j++)
			{
				split_ip_token=strsep(&ip_token,".");
				ni_rx_tx_test.hash_key_ip_da|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
				if(ip_token==NULL) break;
			}
			
			//printk("hash_key_ip_da: %x \n",ni_rx_tx_test.hash_key_ip_da);
		}else if(strcasecmp(split_str,"key_o_lspid")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_key_o_lspid=simple_strtol(split_str, NULL, 0);
			//printk("hash_key_o_lspid: %x \n",ni_rx_tx_test.hash_key_o_lspid);
		}else if(strcasecmp(split_str,"act_sw_id")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_act_sw_id[0]=simple_strtol(split_str, NULL, 0);
			//printk("hash_act_sw_id: %x \n",ni_rx_tx_test.hash_act_sw_id[0]);
		}else if(strcasecmp(split_str,"act_dscp")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_act_dscp=simple_strtol(split_str, NULL, 0);
			ni_rx_tx_test.hash_act_dscp_msk = 1;
			//printk("hash_act_dscp: %x \n",ni_rx_tx_test.hash_act_dscp);
		}else if(strcasecmp(split_str,"act_ldpid")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.hash_act_ldpid=simple_strtol(split_str, NULL, 0);
			//printk("act_ldpid: %x \n",ni_rx_tx_test.hash_act_ldpid);
		}else if(strcasecmp(split_str,"verify_ldpid")==0){
			split_str=strsep(&strptr," ");
			ni_rx_tx_test.verify_ldpid=simple_strtol(split_str, NULL, 0);
			//printk("verify_ldpid: %x \n",ni_rx_tx_test.verify_ldpid);
		}else{
			goto errout;
		}
		split_str=strsep(&strptr," ");
	}while(strptr != NULL);

	if(ni_rx_tx_test.hash_key_ip_protocol){
		ni_rx_tx_test.hash_index = ni_test_add_flow();
	}

	return len;

errout:
	printk("invalid parameter... \n\n");

	return len;
}

static const struct file_operations fops_ni_rx_tx_test __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ni_rx_tx_test_open,
	.read		= seq_read,
	.write		= ni_rx_tx_test_write,
	.release	= single_release,
};
#endif //(CONFIG_RTL_FPGA_PHY_TEST)

#endif


int ca_ni_proc_init(void)
{
	proc_ni = proc_mkdir("ni", ca_proc_dir);

	if (proc_create("ni_debug", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_debug) == NULL) {
		pr_err("Fail to create proc entry ni_debug.\n");
		return -ENOMEM;
	}

	if (proc_create("ne_memtrace", S_IRUGO | S_IWUGO, proc_ni, &fops_ne_memtrace) == NULL) {
		pr_err("Fail to create proc entry ne_memtrace.\n");
		return -ENOMEM;
	}

        if (proc_create("tx_ldpid", S_IRUGO | S_IWUGO, proc_ni, &fops_tx_ldpid) == NULL) {
                pr_err("Fail to create proc entry tx_ldpid.\n");
                return -ENOMEM;
        }
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	if (proc_create("dev_port_mapping", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_dev_port_mapping) == NULL) {
		pr_err("Fail to create proc entry dev_port_mapping.\n");
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_LUNA_G3_SERIES)
	/* debug: memory read/write */
	if (proc_create("memory", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_memory) == NULL) {
		pr_err("Fail to create proc entry memory.\n");
		return -ENOMEM;
	}
#if defined(CONFIG_RTL_FPGA_PHY_TEST)

	/* debug: ni tx test */
	if (proc_create("ni_rx_tx_test", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_rx_tx_test) == NULL) {
		pr_err("Fail to create proc entry ni_rx_tx_test.\n");
		return -ENOMEM;
	}
#endif
#if defined(CONFIG_RTK_CA_NE_SKB_LIMIT)
        if (proc_create("ni_skb_limit_kb", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_skb_limit) == NULL) {
                pr_err("Fail to create proc entry ni_skb_limit_kb.\n");
                return -ENOMEM;
        }
#endif
#endif
	return 0;
}

void ca_ni_proc_exit(void)
{
	remove_proc_entry("ni_debug", proc_ni);
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTL_MULTI_LAN_DEV)
	remove_proc_entry("dev_port_mapping", proc_ni);
#endif
#if defined(CONFIG_LUNA_G3_SERIES)
	remove_proc_entry("memory", proc_ni);
#if defined(CONFIG_RTK_CA_NE_SKB_LIMIT)
        remove_proc_entry("ni_skb_limit_kb", proc_ni);
#endif
#endif
	proc_remove(proc_ni);
}

