/* IPID modification target for IP tables
 * This software is distributed under the terms of GNU GPL
 */


#include <linux/module.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/checksum.h>
#include <linux/random.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ipt_IPID.h>

#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
#include <net/ip6_tunnel.h>
#endif

MODULE_AUTHOR("forgotfun<forgotfun@gmail.com>");
MODULE_DESCRIPTION("IP tables IPID modification module");
MODULE_LICENSE("GPL");


/* 对网卡名进行 hash */
u_int16_t get_id(const char name []){
	u_int16_t i=0;
	u_int16_t o=0;
	while(name[i] != '\0'){
		o ^= name[i];
		i++;
	}
	return(o);
}

static unsigned int 
ipid_tg(struct sk_buff *skb, const struct xt_action_param *par)

{
	struct iphdr *iph;
	const struct ipt_IPID_info *info = par->targinfo;
	static u_int16_t new_ipid[255];
	u_int16_t id;
	u_int16_t mape_ipid;

	if (!skb_make_writable(skb, skb->len))
		return NF_DROP;

	iph = ip_hdr(skb);

	id=get_id(par->out->name);
	
	switch(info->mode){
	#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
		case IP_IPID_MAPE:
			mape_ipid = kernel_get_mape_ipid(par->out->name);
			if (mape_ipid > 0){
				new_ipid[id] = mape_ipid;
			}else{
				new_ipid[id] = ntohs(iph->id);
			}
			break;
	#endif
		case IP_IPID_PACE:
			new_ipid[id] += info->ipid;
			break;
		case IP_IPID_CHAOTIC:
		default:
			get_random_bytes(&(new_ipid[id]),sizeof(new_ipid[id]));
	}	
	iph->id = htons(new_ipid[id]);
	iph->check = 0;
	iph->check = ip_fast_csum((char *)iph,iph->ihl);

	return XT_CONTINUE;
}


static int ipid_tg_check(const struct xt_tgchk_param *par)
{
	if(strcmp(par->table, "mangle") != 0 ){
		printk(KERN_WARNING "IPID: can only be called from \'mangle\' table, not \'%s\'.\n", par->table);
		return -EINVAL;
	}
		
	return 0;

}

static struct xt_target ipid_tg_reg __read_mostly = { 
	.name 	    = "IPID",
	.revision   = 0,
	.family     = NFPROTO_UNSPEC,
	.checkentry = ipid_tg_check,
	.target     = ipid_tg,
	.targetsize = sizeof(struct ipt_IPID_info),
	.table      = "mangle",
	.me         = THIS_MODULE,
};


static int __init ipid_tg_init(void)
{
	return xt_register_target(&ipid_tg_reg);
}

static void __exit ipid_tg_exit(void)
{
	xt_unregister_target(&ipid_tg_reg);
}

module_init(ipid_tg_init);
module_exit(ipid_tg_exit);
