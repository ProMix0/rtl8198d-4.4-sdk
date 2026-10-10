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
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/reset.h>
#include <linux/workqueue.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#ifdef CONFIG_RTK_SMB_SPEEDUP
int smbdbgcount = 0;
int smbscflag = 0x0;
unsigned int smbserverip = 0xc0a80101;

int checkSMBLocalIPPort(u32 srcip, u16 dport){
	if ( !smbscflag ){
		printk("NO SMB SHORTCUT\n");
		return 0;	
	}
	
	if ( smbdbgcount > 0 ) {
		printk("src 0x%x:0x%x, dport %d:%d, smbserverip 0x%x\n", htonl(srcip), ntohl(srcip), htons(dport), ntohs(dport), smbserverip);
	}
	if ( ntohl(srcip) == smbserverip && 
		( ntohs(dport) == 20 || ntohs(dport) == 21 || 
		  ntohs(dport) == 137 || ntohs(dport) == 138 || 
		  ntohs(dport) == 139 || ntohs(dport) == 445))
	{
		if ( smbdbgcount > 0 ) {
			printk("src 0x%x, dport %d\n", srcip, dport);
		}		
		return 1;
	}
	else
		return 0;
	
	smbdbgcount --;
}
EXPORT_SYMBOL(checkSMBLocalIPPort);

int checkSMBIPPort(struct sk_buff *skb, int hook){
	struct iphdr *iph = ip_hdr(skb);
	//const struct iphdr *iph = ((struct sk_buff *)skb)->nh.iph;
	struct tcphdr *th;

	if ( unlikely(!smbscflag) )
		return 0;

	if (unlikely( ntohs(iph->frag_off) & 0x3fff )) /* Ignore fragment */
	{
		printk(KERN_ERR "[%s][%d] iph->frag_off %x\n", __FUNCTION__, __LINE__, iph->frag_off);
		return 0;
	}

	th=(struct tcphdr *) ((void *) iph + iph->ihl*4);
	if (th->syn)
		return 0;

	switch (hook) {
		case NF_INET_LOCAL_IN:
		case NF_INET_PRE_ROUTING:
			if ( checkSMBLocalIPPort(iph->daddr, th->dest) ){
				//printk("Incoming serverip = %d.%d.%d.%d\n", (ntohl(iph->daddr) & 0xff000000) >> 24, (ntohl(iph->daddr) & 0x00ff0000) >> 16,(ntohl(iph->daddr) & 0x0000ff00) >> 8, (ntohl(iph->daddr) & 0xff));
				return 1;
			}
			break;
		case NF_INET_LOCAL_OUT:
		case NF_INET_POST_ROUTING:
			if ( checkSMBLocalIPPort(iph->saddr, th->source) ){
				//printk("Outgoing serverip = %d.%d.%d.%d\n", (ntohl(iph->saddr) & 0xff000000) >> 24,  (ntohl(iph->saddr) & 0x00ff0000) >> 16, (ntohl(iph->saddr) & 0x0000ff00) >> 8, (ntohl(iph->saddr) & 0xff));
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 0;
}
EXPORT_SYMBOL(checkSMBIPPort);

static void smbSetServerIP(u32 sip){
	smbserverip = sip;
}
static void smbSetflag(int flag){
	smbscflag = flag;
}

int getSMBflag(void){
	return smbscflag;
}
EXPORT_SYMBOL(getSMBflag);
static int smbsc_read_proc(struct seq_file *seq, void *v)
{
	seq_printf(seq, "SMB serverip = %d.%d.%d.%d\n", 
		(smbserverip & 0xff000000) >> 24, 
		(smbserverip & 0x00ff0000) >> 16, 
		(smbserverip & 0x0000ff00) >> 8, 
		(smbserverip & 0xff));
	if ( smbscflag )
		seq_printf(seq, "SMB shortcut enable\n");
	else
		seq_printf(seq, "SMB shortcut disable\n");
	return 0;
}

static int proc_smbsc_open(struct inode *inode, struct file *file)
{
	return single_open(file, smbsc_read_proc, inode->i_private);
}

//# echo "sip 192.168.1.2" > /proc/smbshortcut
//# echo "set 1" > /proc/smbshortcut
static ssize_t smbsc_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmpbuf[512]={0};
	u32 sip=0;
	int flag = 0;

	if (buffer && !copy_from_user(tmpbuf, buffer, count))
	{
		char *strptr,*split_str;

		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		split_str=strsep(&strptr," ");
		/*parse command*/

		//printk("split_str [%s]\n", split_str);
		while(1)
		{
			//split_str=strsep(&strptr," ");
			//printk("split_str [%s]\n", split_str);
			if(strcasecmp(split_str, "sip") == 0)
			{
				char *ip_token,*split_ip_token,j;
				split_str=strsep(&strptr," ");
				ip_token=split_str;
				//printk("%s-%d sip=%s\n",__func__,__LINE__,ip_token);
				for(j=0;j<4;j++)
				{
					split_ip_token=strsep(&ip_token,".");
					//printk("ip_token [%s], split_ip_token [%s]\n", ip_token, split_ip_token);
					sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
					if(ip_token==NULL) {
						smbSetServerIP(sip);
						break;
					}
				}
			}else if(strcasecmp(split_str, "set") == 0)
			{
				split_str=strsep(&strptr," ");
				flag=simple_strtol(split_str, NULL, 0);
				smbSetflag(flag);
				//printk("%s-%d sport=%d\n",__func__,__LINE__,sport);
			}else if(strcasecmp(split_str, "dbg") == 0)
			{
				split_str=strsep(&strptr," ");
				smbdbgcount=simple_strtol(split_str, NULL, 0);
				printk("%s-%d smbdbgcount=%d\n",__func__,__LINE__,smbdbgcount);
			}else
				split_str=strsep(&strptr," ");
			if (strptr==NULL) break;
		}
		//printk("sip 0x%x, flag 0x%x\n", sip, flag);

	}
	
	return count;
}

static const struct file_operations smbsc_fops = {
	.owner          = THIS_MODULE,
	.open           = proc_smbsc_open,
	.read           = seq_read,
	.write          = smbsc_write_proc,
	.llseek         = seq_lseek,
	.release        = single_release,
};

int __init SMBshort_proc_init(void)
{
	struct proc_dir_entry *entry=NULL;

	entry = proc_create_data("smbshortcut", 0644, NULL, &smbsc_fops,NULL);
	if (entry == NULL)
	{
		printk("create proc smbsc fail!\n");		
		remove_proc_entry("smbsc", NULL);
		return -ENOMEM;
	}

	return 0;
}

int SMBshort_proc_remove(void){
	remove_proc_entry( "smbshortcut", NULL );
	return 0;
}
module_init(SMBshort_proc_init);
#endif
