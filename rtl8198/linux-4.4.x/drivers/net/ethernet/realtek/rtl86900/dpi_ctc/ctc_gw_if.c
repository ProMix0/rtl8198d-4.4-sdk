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



//#define LOG_DEBUG   printk

enum {
    NETFILTER_PRE_ROUTING,
    NETFILTER_FORWARD,
};

typedef struct netfilter_priv_info_
{
    int                 index;
    int                 hooknum;
    struct nf_hook_ops  *ops;
} netfilter_priv_info_t;

static int  g_forward_count = 0;

static ctSgw_appGetInfoProc g_getinfo_func = NULL;
static ctSgw_appPreProc     g_prerouting_func[NETFILTER_MAX_HOOKS];
static ctSgw_appForwardProc g_forward_func[NETFILTER_MAX_HOOKS];

void dump_tuple_info(tupleinfo_t *tuple_info)
{
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"=====================================================\n");
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->direct=%d\n",tuple_info->direct);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->proto=%d\n",tuple_info->proto);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->new_conntrack=%d\n",tuple_info->new_conntrack);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->sipv4=0x%x\n",tuple_info->sipv4);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->dipv4=0x%x\n",tuple_info->dipv4);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->sipv6=%x-%x-%x-%x\n",tuple_info->sipv6[0],tuple_info->sipv6[1],tuple_info->sipv6[2],tuple_info->sipv6[3]);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->dipv6=%x-%x-%x-%x\n",tuple_info->dipv6[0],tuple_info->dipv6[1],tuple_info->dipv6[2],tuple_info->dipv6[3]);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->sport=0x%x\n",tuple_info->sport);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->dport=0x%x\n",tuple_info->dport);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->in_iif=0x%x\n",tuple_info->in_iif);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"tuple_info->out_iif=0x%x\n",tuple_info->out_iif);
	DPI_LOG(DPI_LOG_LEVEL_DEBUG,"=====================================================\n");

}

static int __netfilter_process(int hooknum, struct sk_buff *skb, int iif, int oif)
{
    int i = 0;
    int fincnt = 0;
    int ret = MAXNET_DPI_CON;
    uint8_t *layer3_data = NULL;
    uint8_t *layer2_data = NULL;
    uint8_t direct = 0;
    tupleinfo_t tuple_info;
    enum ip_conntrack_info ctinfo;
    dpi_gwinfo_t gwinfo;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
    struct nf_conn *ct;
    
    ct = nf_ct_get(skb, &ctinfo);
    layer2_data = (uint8_t *) skb_mac_header(skb);
    layer3_data = (uint8_t *) ip_hdr(skb);
#else
    struct ip_conntrack *ct;
    
    ct = ip_conntrack_get(skb, &ctinfo);
    layer2_data = (uint8_t *)skb->mac.raw;
    layer3_data = (uint8_t *)skb->nh.iph;
#endif
    memset(&tuple_info, 0x0, sizeof(tupleinfo_t));

    if (unlikely(ct == NULL)) {
        return MAXNET_DPI_CON;
    }

    if (ctinfo == IP_CT_NEW || ctinfo == IP_CT_RELATED) {
        tuple_info.new_conntrack = 1;
    }
    else {
        tuple_info.new_conntrack = 0;
    }

    direct = CTINFO2DIR(ctinfo);
    tuple_info.direct = direct;
    tuple_info.proto = ct->tuplehash[direct].tuple.dst.protonum;
    /*
      here, we assume that the inside user original the session; and so the dstip is lan_ip when direct == 1
      but, as we know when it used the nat, we should get the "original srcip" when acting statistics
    */
    if (direct == IP_CT_DIR_ORIGINAL) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
        tuple_info.sipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip);
        tuple_info.dipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip);
#else
        tuple_info.sipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip);
        tuple_info.dipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip);
#endif
        /* port using network order */
        tuple_info.sport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
        tuple_info.dport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);
    }
    else {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
        tuple_info.sipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip);
        tuple_info.dipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip);
#else
        tuple_info.sipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip);
        tuple_info.dipv4 = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip);
#endif
        /* port using network order */
        tuple_info.sport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all);
        tuple_info.dport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all);
    }
    
    tuple_info.in_iif = iif;
    tuple_info.out_iif = oif;
    dump_tuple_info(&tuple_info);


    memset(&gwinfo, 0, sizeof(gwinfo));

    for (i = 0; i < NETFILTER_MAX_HOOKS; i++) {
        if (hooknum == NETFILTER_PRE_ROUTING) {
            if (g_prerouting_func[i] != NULL) {
                ret = (g_prerouting_func[i])(layer2_data, &tuple_info, ct->dpi_context, &gwinfo, NULL);
                if (ret == MAXNET_FDS_TRANS) {
                    /* dnat */
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
                    nf_nat_setup_info(ct, &(gwinfo.nat_range), NF_NAT_MANIP_DST);
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 24)
                    nf_nat_setup_info(ct, &(gwinfo.nat_range), IP_NAT_MANIP_DST);
#else
                    ip_nat_setup_info(ct, &(gwinfo.nat_range), NF_IP_PRE_ROUTING);
#endif
                }else if (MAXNET_DPI_CON == ret){
#if defined(CONFIG_RTK_SKB_MARK2)
		    ct->mark2 |= (1<<g_fwdByPS);
		    DPI_LOG(DPI_LOG_LEVEL_DEBUG,"%s-%d:g_prerouting_func[%d]= %p, ret=%d, skb->mark2=%llx\n", __func__,__LINE__,i,g_prerouting_func[i],ret,ct->mark2);
#endif
                }
            }
        }
        else if (hooknum == NETFILTER_FORWARD) {
            if (g_forward_func[i] != NULL) {
                ret = (g_forward_func[i])(layer2_data, &tuple_info, ct->dpi_context, &gwinfo, NULL);
                if (gwinfo.layer7_id != 0) {
                    ct->layer7_id = gwinfo.layer7_id;
                }

                if (ret == MAXNET_DPI_FIN) {
		    DPI_LOG(DPI_LOG_LEVEL_DEBUG,"%s-%d:g_forward_func[%d]= %p, forward finished\n", __func__,__LINE__,i,g_forward_func[i]);
                    fincnt++;
                }else if (MAXNET_DPI_CON == ret){
#if defined(CONFIG_RTK_SKB_MARK2)
			ct->mark2 |= (1<<g_fwdByPS);
			DPI_LOG(DPI_LOG_LEVEL_DEBUG,"%s-%d:g_prerouting_func[%d]= %p, ret=%d, skb->mark2=%llx\n", __func__,__LINE__,i,g_forward_func[i],ret,ct->mark2);
#endif
				}
            }
        }
    }
    
    if (fincnt == g_forward_count) {
        /* TODO: all hook finish, help yourself */
    }

    return ret;
}

static unsigned int __netfilter_core_process(struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int proto, int hooknum)
{
    int ret = 0;

    if (proto == AF_INET) {
        ;
    }
    else {
        /* TODO: ipv6 */
        return NF_ACCEPT;
    }

    if (hooknum == NETFILTER_PRE_ROUTING) {
        ret = __netfilter_process(hooknum, skb, in->ifindex, 0);
    }
    else {
        ret = __netfilter_process(hooknum, skb, in->ifindex, out->ifindex);
    }
    if (ret == MAXNET_DPI_DNY) {
        return NF_DROP;
    }

    return NF_ACCEPT;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
static unsigned int __prerouting_ipv4_process(void *priv,
    struct sk_buff *skb,
    const struct nf_hook_state *state)
{
	DPI_LOG(DPI_LOG_LEVEL_DEBUG, "%s-%d: in->name=%s, in->ifindex=0x%x,out->name=%s, out->ifindex=0x%x\n", __func__,__LINE__,state->in->name,state->in->ifindex,state->out->name,state->out->ifindex);
	return __netfilter_core_process(skb, state->in, state->out, AF_INET, NETFILTER_PRE_ROUTING);

}
static unsigned int __forward_ipv4_process(void *priv,
    struct sk_buff *skb,
    const struct nf_hook_state *state)
{
DPI_LOG(DPI_LOG_LEVEL_DEBUG, "%s-%d: in->name=%s, in->ifindex=0x%x,out->name=%s, out->ifindex=0x%x\n", __func__,__LINE__,state->in->name,state->in->ifindex,state->out->name,state->out->ifindex);

    return __netfilter_core_process(skb, state->in, state->out, AF_INET, NETFILTER_FORWARD);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
static unsigned int __prerouting_ipv4_process(const struct nf_hook_ops *ops,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_PRE_ROUTING);
}

static unsigned int __forward_ipv4_process(const struct nf_hook_ops *ops,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_FORWARD);
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
static unsigned int __prerouting_ipv4_process(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_PRE_ROUTING);
}

static unsigned int __forward_ipv4_process(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_FORWARD);
}
#else
static unsigned int __prerouting_ipv4_process(unsigned int hooknum,
        struct sk_buff **pskb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    struct sk_buff *skb = *pskb;
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_PRE_ROUTING);
}

static unsigned int __forward_ipv4_process(unsigned int hooknum,
        struct sk_buff **pskb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    struct sk_buff *skb = *pskb;
    return __netfilter_core_process(skb, in, out, AF_INET, NETFILTER_FORWARD);
}
#endif

static int register_prerouting_hook(ctSgw_appPreProc hook, int priority, void **priv_data)
{
    int i = 0;
    netfilter_priv_info_t **info = NULL;
    
    if (hook == NULL) {
        return -1;
    }

    for (i = 0; i < NETFILTER_MAX_HOOKS; i++) {
        if (g_prerouting_func[i] == NULL) {
            break;
        }
    }

    if (i == NETFILTER_MAX_HOOKS) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR, "%s: prerouting has max hooks\n", __func__);
        return -1;
    }

    if (!priv_data) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: priv data not init\n", __func__);
        return -1;
    }

    info = (netfilter_priv_info_t **)priv_data;
    *info = (netfilter_priv_info_t *)kmalloc(sizeof(netfilter_priv_info_t), GFP_ATOMIC);
    if (*info == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: malloc netfilter priv info failed\n", __func__);
        return -1;
    }

    memset(*info, 0, sizeof(netfilter_priv_info_t));
    (*info)->index = i + 1;
    (*info)->hooknum = NETFILTER_PRE_ROUTING;

    (*info)->ops = (struct nf_hook_ops *)kmalloc(sizeof(struct nf_hook_ops), GFP_ATOMIC);
    if ((*info)->ops == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: malloc nf_hook_ops failed\n", __func__);
        kfree(*info);
        *info = NULL;
        return -1;
    }

    memset((*info)->ops, 0, sizeof(struct nf_hook_ops));
    (*info)->ops->hook = __prerouting_ipv4_process;
    //(*info)->ops->owner = THIS_MODULE;
    (*info)->ops->pf = PF_INET;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
    (*info)->ops->hooknum = NF_INET_PRE_ROUTING;
#else
    (*info)->ops->hooknum = NF_IP_PRE_ROUTING;
#endif
    (*info)->ops->priority = priority;

    g_prerouting_func[i] = hook;
    if (nf_register_hook((*info)->ops)) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: nf_register_hook failed\n", __func__);
        kfree((*info)->ops);
        kfree(*info);
        *info = NULL;
        g_prerouting_func[i] = NULL;
        return -1;
    }

    return 0;
}

static int register_forward_hook(ctSgw_appForwardProc hook, int priority, void **priv_data)
{
    int i = 0;
    netfilter_priv_info_t **info = NULL;

    for (i = 0; i < NETFILTER_MAX_HOOKS; i++) {
        if (g_forward_func[i] == NULL) {
            break;
        }
    }

    if (i == NETFILTER_MAX_HOOKS) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: forward has max hooks\n", __func__);
        return -1;
    }

    if (!priv_data) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: priv data not init\n", __func__);
        return -1;
    }

    info = (netfilter_priv_info_t **)priv_data;
    *info = (netfilter_priv_info_t *)kmalloc(sizeof(netfilter_priv_info_t), GFP_ATOMIC);
    if (*info == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: malloc netfilter priv info failed\n", __func__);
        return -1;
    }

    memset(*info, 0, sizeof(netfilter_priv_info_t));
    (*info)->index = i + 1;
    (*info)->hooknum = NETFILTER_FORWARD;

    (*info)->ops = (struct nf_hook_ops *)kmalloc(sizeof(struct nf_hook_ops), GFP_ATOMIC);
    if ((*info)->ops == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: malloc nf_hook_ops failed\n", __func__);
        kfree(*info);
        *info = NULL;
        return -1;
    }

    memset((*info)->ops, 0, sizeof(struct nf_hook_ops));
    (*info)->ops->hook = __forward_ipv4_process;
    //(*info)->ops->owner = THIS_MODULE;
    (*info)->ops->pf = PF_INET;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
    (*info)->ops->hooknum = NF_INET_FORWARD;
#else
    (*info)->ops->hooknum = NF_IP_FORWARD;
#endif
    (*info)->ops->priority = priority;

    g_forward_func[i] = hook;
    if (nf_register_hook((*info)->ops)) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: nf_register_hook failed\n", __func__);
        kfree((*info)->ops);
        kfree(*info);
        *info = NULL;
        return -1;
    }

    g_forward_count++;
    return 0;
}

static void unregister_netfilter_hook(void **priv_data)
{
    netfilter_priv_info_t *info = NULL;
    
    if (!priv_data) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: priv data not init\n", __func__);
        return;
    }

    info = *((netfilter_priv_info_t **)priv_data);
    if (info == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: info is null\n", __func__);
        return;
    }

    if (info->ops == NULL) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: info ops is null\n", __func__);
        kfree(info);
        *priv_data = NULL;
        return;
    }

    if (info->index < 1 || info->index > NETFILTER_MAX_HOOKS) {
        DPI_LOG(DPI_LOG_LEVEL_ERROR,"%s: info index %d\n", __func__, info->index);
        nf_unregister_hook(info->ops);
        kfree(info->ops);
        kfree(info);
        *priv_data = NULL;
        return;
    }
    
    if (info->hooknum == NETFILTER_PRE_ROUTING) {
        g_prerouting_func[info->index - 1] = NULL;
    }
    else if (info->hooknum == NETFILTER_FORWARD) {
        g_forward_count--;
        g_forward_func[info->index - 1] = NULL;
    }

    nf_unregister_hook(info->ops);
    kfree(info->ops);
    kfree(info);

    *priv_data = NULL;
    return;
}

int ctSgw_appRegisterGetInfo(ctSgw_appGetInfoProc funcs)
{
    if (funcs == NULL) {
        return -1;
    }

    g_getinfo_func = funcs;
    return 0;
}

void ctSgw_appUnRegisterGetInfo(void)
{
    g_getinfo_func = NULL;
    return;
}

int ctSgw_appGetInfo(void *app_ctx, void *app_info)
{
    if (g_getinfo_func == NULL) {
        return -1;
    }

    return g_getinfo_func(app_ctx, app_info);
}

int ctSgw_appRegisterPreProc(CtSgwPreProcInfo *info)
{
    if (info == NULL) {
        return -1;
    }

    return register_prerouting_hook(info->hook, info->priority, &(info->priv_data));
}

void ctSgw_appUnRegisterPreProc(CtSgwPreProcInfo *info)
{
    if (info != NULL) {
        unregister_netfilter_hook(&(info->priv_data));
    }
    return;
}

int ctSgw_appRegisterForwardProc(CtSgwForwardProcInfo *info)
{
    if (info == NULL) {
        return -1;
    }

    return register_forward_hook(info->hook, info->priority, &(info->priv_data));
}

void ctSgw_appUnRegisterForwardProc(CtSgwForwardProcInfo *info)
{
    if (info != NULL) {
        unregister_netfilter_hook(&(info->priv_data));
    }
    return;
}

EXPORT_SYMBOL(ctSgw_appRegisterGetInfo);
EXPORT_SYMBOL(ctSgw_appUnRegisterGetInfo);
EXPORT_SYMBOL(ctSgw_appGetInfo);

EXPORT_SYMBOL(ctSgw_appRegisterPreProc);
EXPORT_SYMBOL(ctSgw_appUnRegisterPreProc);

EXPORT_SYMBOL(ctSgw_appRegisterForwardProc);
EXPORT_SYMBOL(ctSgw_appUnRegisterForwardProc);


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("maxnet");


