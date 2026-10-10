/* IP tables module for matching the range of the IPv4/IPv6 DSCP field
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
#include <net/dsfield.h>

#include <linux/netfilter/xt_dscprange.h>
#include <linux/netfilter/x_tables.h>

MODULE_AUTHOR("chloe_wang <chloe_wang@realsil.com.cn>");
MODULE_DESCRIPTION("Xtables: DSCP/TOS range match");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_dscp_range");
MODULE_ALIAS("ip6t_dscp_range");
MODULE_ALIAS("ipt_tos");
MODULE_ALIAS("ip6t_tos");

static bool
dscp_range_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_dscp_range_info *info = par->matchinfo;
	u_int8_t dscp = ipv4_get_dsfield(ip_hdr(skb)) >> XT_DSCP_SHIFT;

	return ((dscp >= info->dscp_min && dscp <= info->dscp_max) ^ !!info->invert);
}

static bool
dscp_range_mt6(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_dscp_range_info *info = par->matchinfo;
	u_int8_t dscp = ipv6_get_dsfield(ipv6_hdr(skb)) >> XT_DSCP_SHIFT;

	return ((dscp >= info->dscp_min && dscp <= info->dscp_max) ^ !!info->invert);
}

static int dscp_range_mt_check(const struct xt_mtchk_param *par)
{
	const struct xt_dscp_range_info *info = par->matchinfo;

	if (info->dscp_min > XT_DSCP_MAX) {
		pr_info("dscp %x out of range\n", info->dscp_min);
		return -EDOM;
	}

	if (info->dscp_max > XT_DSCP_MAX) {
		pr_info("dscp %x out of range\n", info->dscp_max);
		return -EDOM;
	}

	return 0;
}

static bool
tos_range_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_tos_range_info *info = par->matchinfo;
	u_int8_t tos = ipv4_get_dsfield(ip_hdr(skb));

	return ((tos >= info->tos_min && tos <= info->tos_max) ^ !!info->invert);
}

static bool
tos_range_mt6(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_tos_range_info *info = par->matchinfo;
	u_int8_t tos = ipv6_get_dsfield(ipv6_hdr(skb));

	return ((tos >= info->tos_min && tos <= info->tos_max) ^ !!info->invert);
}

static int tos_range_mt_check(const struct xt_mtchk_param *par)
{
	const struct xt_tos_range_info *info = par->matchinfo;

	if (info->tos_min > XT_TOS_MAX) {
		pr_info("tos %x out of range\n", info->tos_min);
		return -EDOM;
	}

	if (info->tos_max > XT_TOS_MAX) {
		pr_info("tos %x out of range\n", info->tos_max);
		return -EDOM;
	}

	return 0;
}
static struct xt_match dscp_range_mt_reg[] __read_mostly = {
	{
		.name		= "dscprange",
		.family		= NFPROTO_IPV4,
		.checkentry	= dscp_range_mt_check,
		.match		= dscp_range_mt,
		.matchsize	= sizeof(struct xt_dscp_range_info),
		.me		= THIS_MODULE,

	},
	{
		.name		= "dscprange",
		.family		= NFPROTO_IPV6,
		.checkentry	= dscp_range_mt_check,
		.match		= dscp_range_mt6,
		.matchsize	= sizeof(struct xt_dscp_range_info),
		.me		= THIS_MODULE,
	},
	{
		.name		= "tosrange",
		.family		= NFPROTO_IPV4,
		.checkentry	= tos_range_mt_check,
		.match		= tos_range_mt,
		.matchsize	= sizeof(struct xt_tos_range_info),
		.me		= THIS_MODULE,
	},
	{
		.name		= "tosrange",
		.family		= NFPROTO_IPV6,
		.checkentry	= tos_range_mt_check,
		.match		= tos_range_mt6,
		.matchsize	= sizeof(struct xt_tos_range_info),
		.me		= THIS_MODULE,
	},
};

static int __init dscp_range_mt_init(void)
{
	return xt_register_matches(dscp_range_mt_reg, ARRAY_SIZE(dscp_range_mt_reg));
}

static void __exit dscp_range_mt_exit(void)
{
	xt_unregister_matches(dscp_range_mt_reg, ARRAY_SIZE(dscp_range_mt_reg));
}

module_init(dscp_range_mt_init);
module_exit(dscp_range_mt_exit);
