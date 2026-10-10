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

#if 1//defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#include "ca_ni.h"
#endif
#include "../ca-ne-rtk/rtk_func.h"
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

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#define CA_NE_NI_DEV_PORTMAPPING_HELP_MSG \
	"Purpose: Configure NI driver multiple LAN device port mapping\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [num_of_port dev_name] > %s\n"
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

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
void ca_ni_netlink_dump_pid_seq_list(void);
void ca_ni_netlink_rx_queue_dump(void);
void ca_rx_register_pkt_dump_all(void);
#endif
void ca_ni_dma_lso_dump(void);



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

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
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
#endif
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


#if defined(CONFIG_RTL_MULTI_LAN_DEV)
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
#if 1//defined(CONFIG_LUNA_G3_SERIES)
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
#endif


int ca_ni_proc_init(void)
{
	proc_ni = proc_mkdir("ni", NULL);

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
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	if (proc_create("dev_port_mapping", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_dev_port_mapping) == NULL) {
		pr_err("Fail to create proc entry dev_port_mapping.\n");
		return -ENOMEM;
	}
#endif
#if 1//defined(CONFIG_LUNA_G3_SERIES)
	/* debug: memory read/write */
	if (proc_create("memory", S_IRUGO | S_IWUGO, proc_ni, &fops_ni_memory) == NULL) {
		pr_err("Fail to create proc entry ne_memtrace.\n");
		return -ENOMEM;
	}
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
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	remove_proc_entry("dev_port_mapping", proc_ni);
#endif
#if 1//defined(CONFIG_LUNA_G3_SERIES)
	remove_proc_entry("memory", proc_ni);
#if defined(CONFIG_RTK_CA_NE_SKB_LIMIT)
        remove_proc_entry("ni_skb_limit_kb", proc_ni);
#endif
#endif
	proc_remove(proc_ni);
}

