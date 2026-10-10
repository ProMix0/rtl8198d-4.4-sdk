/* IP tables module for matching the range of the IPv6 Flow Label field
 *
 * chloe_wang <chloe_wang@realsil.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include <linux/netfilter/xt_flowlblrange.h>
#include <linux/netfilter/x_tables.h>

MODULE_AUTHOR("chloe_wang <chloe_wang@realsil.com.cn>");
MODULE_DESCRIPTION("Xtables: Flow Label range match");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ip6t_flowlbl_range");

static bool
flowlbl_range_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_flowlbl_range_info *info = par->matchinfo;
	const struct ipv6hdr *ip6h = ipv6_hdr(skb);
	const __u32 flowlbl = (((ip6h->flow_lbl[0] & 0xF) << 16) | (ip6h->flow_lbl[1] << 8) | ip6h->flow_lbl[2]);

	return ((flowlbl >= info->flowlbl_min && flowlbl <= info->flowlbl_max) ^ !!info->invert);
}

static int flowlbl_range_mt_check(const struct xt_mtchk_param *par)
{
	const struct xt_flowlbl_range_info *info = par->matchinfo;

	if (info->flowlbl_min > XT_FLOWLABEL_MAX) {
		pr_info("flowlbl %x out of range\n", info->flowlbl_min);
		return -EDOM;
	}

	if (info->flowlbl_max > XT_FLOWLABEL_MAX) {
		pr_info("flowlbl %x out of range\n", info->flowlbl_max);
		return -EDOM;
	}

	return 0;
}

static struct xt_match flowlbl_range_mt_reg[] __read_mostly = {
	{
		.name		= "flowlblrange",
		.family		= NFPROTO_IPV6,
		.checkentry	= flowlbl_range_mt_check,
		.match		= flowlbl_range_mt,
		.matchsize	= sizeof(struct xt_flowlbl_range_info),
		.me		= THIS_MODULE,

	},
};

static int __init flowlbl_range_mt_init(void)
{
	return xt_register_matches(flowlbl_range_mt_reg, ARRAY_SIZE(flowlbl_range_mt_reg));
}

static void __exit flowlbl_range_mt_exit(void)
{
	xt_unregister_matches(flowlbl_range_mt_reg, ARRAY_SIZE(flowlbl_range_mt_reg));
}

module_init(flowlbl_range_mt_init);
module_exit(flowlbl_range_mt_exit);
