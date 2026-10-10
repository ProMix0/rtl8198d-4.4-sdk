/**********************************************************************
 * maxnet_dpi_if.c
 *
 * Wang Dongquan <wdq347@163.com>
 * Description: 
 ***********************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/tty.h>
#include <net/icmp.h>
#include <net/ip.h>     /*  for local_port_range[] */
#include <net/tcp.h>        /*  struct or_callable used in sock_rcv_skb */
#include <net/netlabel.h>
#include <linux/uaccess.h>
#include <asm/ioctls.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/netlink.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ipv6.h>
#include <linux/proc_fs.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
#include <net/netfilter/nf_conntrack.h>
#else
#include <linux/netfilter_ipv4/ip_conntrack.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 33)
#include <net/netfilter/nf_conntrack_ecache.h>
#endif

#include "ctc_dpi_if.h"
#include "ctc_gw_if.h"

extern void dump_tuple_info(tupleinfo_t *tuple_info);

typedef struct netdev_info_
{
	int ifindex;
	char name[IFNAMSIZ];
}netdev_info_t;

static char *ctc_dpi_proc_name = "ctc_dpi";


// global variable
static int g_forward_func_count = 0;
static int g_prerouting_func_count = 0;
int g_fwdByPS=0;//bit position set into skb mark2.
static ctSgw_appCtxCreate   g_create_func = NULL;
static ctSgw_appCtxDestroy  g_destroy_func = NULL;
static ctSgw_appProcAppId   g_AppId_func = NULL;

static netdev_info_t g_netdev_inf[MAX_NETDEV];
static CtSgwDPIStatistic g_dpi_stat;
static int qos_pri[10] = {0,6,5,4,3,2,1,0,0,0};
static unsigned short dev_eth_ifindex = 0;
ctc_dpi_control_t g_ctc_dpi_ctrl;
static int dpi_process(struct sk_buff *skb, int iif, int oif)
{
    int ret = MAXNET_DPI_CON;
    struct iphdr *iph = NULL;
    uint8_t *layer2_data = NULL;
    uint32_t layer7_id;
    uint8_t direct = 0;
    tupleinfo_t tuple_info;
    enum ip_conntrack_info ctinfo;

    if (skb_linearize(skb)) {
        return MAXNET_DPI_CON;
    }

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
    struct nf_conn *ct;
    
    ct = nf_ct_get(skb, &ctinfo);
    layer2_data = (uint8_t *) skb_mac_header(skb);
    iph = (struct iphdr *) ip_hdr(skb);
#else
    struct ip_conntrack *ct;
    
    ct = ip_conntrack_get(skb, &ctinfo);
    layer2_data = (uint8_t *)skb->mac.raw;
    iph = (struct iphdr *)skb->nh.iph;
#endif

    if (unlikely(ct == NULL)) {
        return MAXNET_DPI_CON;
    }
    memset(&tuple_info, 0x0, sizeof(tupleinfo_t));

    direct = CTINFO2DIR(ctinfo);
    tuple_info.direct = direct;
    /* using iphdr for ip */
    tuple_info.sipv4 = ntohl(iph->saddr);
    tuple_info.dipv4 = ntohl(iph->daddr);
    tuple_info.proto = iph->protocol;
    if (direct == IP_CT_DIR_ORIGINAL) {        /* port using network order */
        tuple_info.sport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
        tuple_info.dport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);
    }
    else {
        tuple_info.sport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);
        tuple_info.dport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
    }
    
    tuple_info.in_iif = iif;
    tuple_info.out_iif = oif;
    

    dump_tuple_info(&tuple_info);

    
    if ( NULL != g_AppId_func){
        ret = (g_AppId_func)(layer2_data, &tuple_info, ct->dpi_context, &layer7_id);
        if (layer7_id != 0) {
            ct->layer7_id = layer7_id;
        }
        if (MAXNET_DPI_FIN == ret) {
            /* TODO: find app for this ct, help yourself */
    	    DPI_LOG(DPI_LOG_LEVEL_DEBUG, "%s-%d, find app for this ct: To-Do\n", __func__,__LINE__);
        }else if (MAXNET_DPI_CON == ret){
#if defined(CONFIG_RTK_SKB_MARK2)        
	    ct->mark2 |= (1<<g_fwdByPS);
	    DPI_LOG(DPI_LOG_LEVEL_DEBUG, "%s-%d, continue ret=%d mark2=%llx\n", __func__,__LINE__,ret,ct->mark2);
#endif
	}
    }

    return ret;
}


static int matrix_dpi_core_process(struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int af)
{
    int ret;

    if (af == AF_INET) {
        ;  
    } else {
        /* TODO: IPv6 */
        return NF_ACCEPT;
    }
    ret = dpi_process(skb, in->ifindex, out->ifindex);
    if (ret == MAXNET_DPI_DNY) {
        return NF_DROP;
    }
    else {
        return NF_ACCEPT;
    }
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
static void dpi_core_nf_event(enum ip_conntrack_events event, struct nf_conn *ct)
#else
static void dpi_core_nf_event(enum ip_conntrack_events event, struct ip_conntrack *ct)
#endif
{
    int ret = 0;
    //LOG_DEBUG("dpi_core_nf_event init\n");

    if (event == IPCT_NEW) {
        if (g_create_func) {
            ret = (g_create_func)(&(ct->dpi_context), 0);
            ct->layer7_id = 0;
            if (unlikely(ret != 0)) {
                DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s-%d, app_ctx_create failed on %d\n", __func__,__LINE__, ret);
            }
        }
    } else if (event == IPCT_RELATED) {
        /* check layer7_id is avaliable */
        ct->layer7_id = 0;
        if (ct->master != NULL && ct->master->layer7_id) {
            ct->layer7_id = ct->master->layer7_id;
        }

        if (g_create_func) {
            ret = (g_create_func)(&(ct->dpi_context), ct->layer7_id);
            if (unlikely(ret != 0)) {
                DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s-%d, app_ctx_create failed on %d\n", __func__,__LINE__, ret);
            } 
        }
    } else if (event == IPCT_DESTROY) {
        if (g_destroy_func != NULL) {
            (g_destroy_func)(&(ct->dpi_context));
        }
    } else {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s-%d, unknown event %d\n", __func__,__LINE__, event);
    }
    //LOG_DEBUG("dpi_core_nf_event init done\n");
    return;
}

static int dpi_nf_init(void)
{
    int ret = 0;
    ret = nf_conntrack_event_hook_register(dpi_core_nf_event);
    if (ret != 0) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"DPI register event hook failed.\n");
    }

    return ret;
}

static void dpi_nf_fini(void)
{
    nf_conntrack_event_hook_unregister();

    return;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
static unsigned int __dpi_ipv4_process(void *priv,
    struct sk_buff *skb,
    const struct nf_hook_state *state)
{
    return matrix_dpi_core_process(skb, state->in, state->out, AF_INET);
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
static unsigned int __dpi_ipv4_process(const struct nf_hook_ops *dpi_ops,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return matrix_dpi_core_process(skb, in, out, AF_INET);    
}

#else

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
static unsigned int __dpi_ipv4_process(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return matrix_dpi_core_process(skb, in, out, AF_INET);
}
#else  /* < 2.6.24 */
static unsigned int __dpi_ipv4_process(unsigned int hooknum,
        struct sk_buff **pskb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    struct sk_buff *skb = *pskb;
    return matrix_dpi_core_process(skb, in, out, AF_INET);
}
#endif

#endif  /* end of LINUX_VERSION_CODE */


static struct nf_hook_ops dpi_ops[] __read_mostly = {
    {
        .hook =     __dpi_ipv4_process,
        //.owner =    THIS_MODULE,
        .pf =       PF_INET,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
        .hooknum =  NF_INET_FORWARD,
#else
        .hooknum =  NF_IP_FORWARD, 
#endif
        .priority = NF_IP_PRI_CONNTRACK + 10,
    },
};


int ctSgw_appRegisterFunc(ctSgw_dpiFuncs *funcs)
{
    if (funcs == NULL)
        return -1;

    g_create_func = funcs->ctSgw_appCtxCreateHook;
    g_destroy_func = funcs->ctSgw_appCtxDestroyHook; 
    g_AppId_func = funcs->ctSgw_appProcAppIdHook;
    return 0;
}

void ctSgw_appUnRegisterFunc(void)
{
    g_create_func = NULL;
    g_destroy_func = NULL;
    g_AppId_func = NULL;
}
 
EXPORT_SYMBOL(ctSgw_appRegisterFunc);
EXPORT_SYMBOL(ctSgw_appUnRegisterFunc);
#if 1
static int dpi_stat_read(struct seq_file *seq, void *v)
{
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Context\n");
	seq_printf(seq,"Create:%08lu\n",g_dpi_stat.create_context);
	seq_printf(seq,"Error:%08lu\n",g_dpi_stat.create_ctx_err);
	seq_printf(seq,"Destroy:%08lu\n",g_dpi_stat.destroy_context);
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Pre-Route\n");
	seq_printf(seq,"CNT:%08lu\n",g_dpi_stat.prerte_cnt);
	seq_printf(seq,"NOCTX:%08lu\n",g_dpi_stat.prerte_noctx);
	seq_printf(seq,"DNAT:%08lu\n",g_dpi_stat.prerte_dnat);
	seq_printf(seq,"InDir:%08lu\n",g_dpi_stat.prerte_indir);
	seq_printf(seq,"OutDir:%08lu\n",g_dpi_stat.prerte_outdir);
	seq_printf(seq,"Continue:%08lu\n",g_dpi_stat.prerte_conti);
	seq_printf(seq,"Trap:%08lu\n",g_dpi_stat.prerte_trap);
	seq_printf(seq,"Drop:%08lu\n",g_dpi_stat.prerte_drop);
	seq_printf(seq,"Scan:%08lu\n",g_dpi_stat.prerte_scan);
	seq_printf(seq,"New:%08lu\n",g_dpi_stat.prerte_new);
	seq_printf(seq,"TCP:%08lu\n",g_dpi_stat.prerte_tcp);
	seq_printf(seq,"UDP:%08lu\n",g_dpi_stat.prerte_udp);
	seq_printf(seq,"OtherPro:%08lu\n",g_dpi_stat.prerte_otherpro);
	seq_printf(seq,"ErrorRet:%08lu\n",g_dpi_stat.prerte_errret);
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Forward\n");
	seq_printf(seq,"CNT:%08lu\n",g_dpi_stat.forward_cnt);
	seq_printf(seq,"NOCTX:%08lu\n",g_dpi_stat.forward_noctx);
	seq_printf(seq,"InDir:%08lu\n",g_dpi_stat.forward_indir);
	seq_printf(seq,"OutDir:%08lu\n",g_dpi_stat.forward_outdir);
	seq_printf(seq,"Continue:%08lu\n",g_dpi_stat.forward_conti);
	seq_printf(seq,"Finish:%08lu\n",g_dpi_stat.forward_fin);
	seq_printf(seq,"Drop:%08lu\n",g_dpi_stat.forward_drop);
	seq_printf(seq,"QoS:%08lu\n",g_dpi_stat.forward_qos);
	seq_printf(seq,"Scan:%08lu\n",g_dpi_stat.forward_scan);
	seq_printf(seq,"New:%08lu\n",g_dpi_stat.forward_new);
	seq_printf(seq,"TCP:%08lu\n",g_dpi_stat.forward_tcp);
	seq_printf(seq,"UDP:%08lu\n",g_dpi_stat.forward_udp);
	seq_printf(seq,"OtherPro:%08lu\n",g_dpi_stat.forward_otherpro);
	seq_printf(seq,"ErrorRet:%08lu\n",g_dpi_stat.forward_errret);
	seq_printf(seq,"----------------------------------------\n");
/*
	if (nf_conntrack_dpi_show)
	{
		int i;
		for(i = 0 ; i < MAX_NAPT_OUT_SW_TABLE_SIZE; i++)
		{
			if(rg_db.naptOut[i].rtk_naptOut.valid)
			{
				CtSgwDpiCtx *ctx = NULL;
				ctx = (CtSgwDpiCtx *)(rg_db.naptOut[i].pContext);
				if(ctx) (*nf_conntrack_dpi_show)(seq, ctx->dpi_context);
			}
		}
    }
*/
	seq_printf(seq,"----------------------------------------\n");
	return 0;
}

static int dpi_stat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;
	if (buffer && !copy_from_user(&flag, buffer, sizeof(flag)))
	{
		switch(flag)
		{
			case '0': // reset dpi statistic
			memset(&g_dpi_stat, 0x0 , sizeof(CtSgwDPIStatistic));
			break;
			default:
			return -EFAULT;
		}
		return count;
	}
	return -EFAULT;
}

static int dpi_stat_open(struct inode *inode, struct file *file)
{
        return single_open(file, dpi_stat_read, inode->i_private);
}

static const struct file_operations dpi_stat_fops = {
        .owner          = THIS_MODULE,
        .open           = dpi_stat_open,
        .read           = seq_read,
        .write          = dpi_stat_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int dpi_hook_show_read(struct seq_file *seq, void *v)
{
	int i;

	seq_printf(seq,"appCtxCreateFunc: %p\n", g_create_func);
	seq_printf(seq,"appCtxDestroyFunc: %p\n", g_destroy_func);
	seq_printf(seq,"appProcAppIdFunc: %p\n", g_AppId_func);
#if 0
	seq_printf(seq,"PreRoute Register Hook %08lu\n", g_prerouting_func_count);
	for (i = 0; i < NETFILTER_MAX_HOOKS; i++)
	{
		seq_printf(seq,"\tPrerouteFunc[%d]: %p\n", i, g_prerouting_func[i]);
	}
	seq_printf(seq,"Forword Register Hook %08lu\n", g_forward_func_count);
	for (i = 0; i < NETFILTER_MAX_HOOKS; i++)
	{
		seq_printf(seq,"\tForwordFunc[%d]: %p\n", i, g_forward_func[i]);
	}
#endif
	return 0;
}

static int dpi_hook_show_open(struct inode *inode, struct file *file)
{
        return single_open(file, dpi_hook_show_read, inode->i_private);
}
static const struct file_operations dpi_hook_show_fops = {
        .owner          = THIS_MODULE,
        .open           = dpi_hook_show_open,
        .read           = seq_read,
        .write          = NULL,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#endif
static int proc_dpi_log_level_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "CTC DPI Log Level: %d\n\n", g_ctc_dpi_ctrl.log_level);

	seq_printf(seq, "Accept values: %d ~ %d\n", DPI_LOG_LEVEL_OFF, DPI_LOG_LEVEL_DEBUG);

	return 0;
}

static int proc_dpi_log_level_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dpi_log_level_read, inode->i_private);
}

static int proc_dpi_log_level_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	char tmpbuf[64] = {0};
	int level;

	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		level = simple_strtol(tmpbuf, NULL, 10);
		if(level < DPI_LOG_LEVEL_OFF || level > DPI_LOG_LEVEL_DEBUG)
		{
			DPI_LOG(DPI_LOG_LEVEL_ERROR,
				"only accept %d ~ %d!\n", DPI_LOG_LEVEL_OFF, DPI_LOG_LEVEL_DEBUG);
			return -EFAULT;
		}

		g_ctc_dpi_ctrl.log_level = level;
	}

	return count;
}

static const struct file_operations dpi_log_level_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_dpi_log_level_open,
        .read           = seq_read,
        .write          = proc_dpi_log_level_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int proc_dpi_fwd_by_ps_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "bit position in skb mark2, g_fwdByPS: %d\n\n", g_fwdByPS);
	return 0;
}

static int proc_dpi_fwd_by_ps_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dpi_fwd_by_ps_read, inode->i_private);
}

static int proc_dpi_fwd_by_ps_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	char tmpbuf[64] = {0};
	int bit_position=0;
	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		bit_position = simple_strtol(tmpbuf, NULL, 10);
		if(bit_position < 0 || bit_position > 63)
		{
			DPI_LOG(DPI_LOG_LEVEL_ERROR,
				"g_fwdByPs only accept %d ~ %d!\n", 0, 63);
			return -EFAULT;
		}
		g_fwdByPS = bit_position;
	}

	return count;
}

static const struct file_operations dpi_fwd_by_ps_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_dpi_fwd_by_ps_open,
        .read           = seq_read,
        .write          = proc_dpi_fwd_by_ps_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static struct proc_dir_entry *procfs = NULL;

static int __init moduledpi_init(void)
{
    int err = 0;
    struct proc_dir_entry *entry=NULL;
    
    if( (err = dpi_nf_init())) {
        goto exit;
    }
    g_ctc_dpi_ctrl.log_level = DPI_LOG_LEVEL_INFO;

    /* create a directory */
    procfs = proc_mkdir(ctc_dpi_proc_name, NULL);
    if(procfs == NULL)
    {
	    DPI_LOG(DPI_LOG_LEVEL_ERROR,
		    "Register /proc/%s failed\n", ctc_dpi_proc_name);
	    return -ENOMEM;
    }

    entry = proc_create("dpi_log_level", 0644, procfs, &dpi_log_level_fops);
    if (entry == NULL)
    {
	    DPI_LOG(DPI_LOG_LEVEL_ERROR,
		    "Register /proc/%s/dpi_log_level failed\n", ctc_dpi_proc_name);
	    return -ENOMEM;
    }
    entry = proc_create("dpi_hook_show", 0644, procfs, &dpi_hook_show_fops);
    if (entry == NULL)
    {
	    DPI_LOG(DPI_LOG_LEVEL_ERROR,
		    "Register /proc/%s/dpi_hook_show failed\n", ctc_dpi_proc_name);
	    return -ENOMEM;
    }
    entry = proc_create("dpi", 0644, procfs, &dpi_stat_fops);
    if (entry == NULL)
    {
	    DPI_LOG(DPI_LOG_LEVEL_ERROR,
		    "Register /proc/%s/dpi failed\n", ctc_dpi_proc_name);
	    return -ENOMEM;
    }

    entry = proc_create("dpi_fwdByPS", 0644, procfs, &dpi_fwd_by_ps_fops);
    if (entry == NULL)
    {
	    DPI_LOG(DPI_LOG_LEVEL_ERROR,
		    "Register /proc/%s/dpi_fwdByPS failed\n", ctc_dpi_proc_name);
	    return -ENOMEM;
    }

    err = nf_register_hooks(dpi_ops, ARRAY_SIZE(dpi_ops));
    if (err) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR, "DPI: nf_register_hooks for IPv4: error %d\n", err);
    }

    printk("Yueme DPI interface Loaded\n");

exit:
    return err;
}

static void __exit moduledpi_exit(void)
{
    nf_unregister_hooks(dpi_ops, ARRAY_SIZE(dpi_ops));

    dpi_nf_fini();
    synchronize_net();
    remove_proc_entry("dpi_log_level", procfs);
#if 1
    remove_proc_entry("dpi", procfs);
    remove_proc_entry("dpi_hook_show", procfs);
#endif
    remove_proc_entry(ctc_dpi_proc_name, NULL);
    printk("Yueme DPI interface Unloaded\n");

    return;
}

module_init(moduledpi_init);
module_exit(moduledpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("maxnet");
