/* SPDX-License-Identifier: GPL-2.0-only */
/* OpenVPN data channel accelerator
 *
 *  Copyright (C) 2020-2023 OpenVPN, Inc.
 *
 *  Author:	Lev Stipakov <lev@openvpn.net>
 *		Antonio Quartulli <antonio@openvpn.net>
 */

#ifndef _NET_OVPN_DCO_LINUX_COMPAT_H_
#define _NET_OVPN_DCO_LINUX_COMPAT_H_

#include <linux/kconfig.h>
#include <linux/version.h>

/* not part of any kernel yet */
#ifndef NLA_POLICY_MAX_LEN
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#define NLA_POLICY_MAX_LEN(_len) { .type = NLA_BINARY, .len = _len }
#else
#define NLA_POLICY_MAX_LEN(_len) NLA_POLICY_MAX(NLA_BINARY, _len)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)

#define genl_split_ops genl_ops

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
/**
 * commit 58caed3dacb4 renamed to netif_napi_add_tx_weight,
 * commit c3f760ef1287 removed netif_tx_napi_add
 */
#define netif_napi_add_tx_weight netif_tx_napi_add
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#define netif_napi_add_tx_weight netif_napi_add
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)

#define sock_is_readable stream_memory_read

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && !defined(EL8)

#define dev_get_tstats64 ip_tunnel_get_stats64

#include <linux/netdevice.h>

static inline void dev_sw_netstats_tx_add(struct net_device *dev,
					  unsigned int packets,
					  unsigned int len)
{
	struct pcpu_sw_netstats *tstats = this_cpu_ptr(dev->tstats);

	u64_stats_update_begin(&tstats->syncp);
	tstats->tx_bytes += len;
	tstats->tx_packets += packets;
	u64_stats_update_end(&tstats->syncp);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && !defined(EL8) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && !defined(EL8)

#define genl_small_ops genl_ops
#define small_ops ops
#define n_small_ops n_ops

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && !defined(EL8) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && !defined(EL8)

#include <linux/netdevice.h>

static inline void dev_sw_netstats_rx_add(struct net_device *dev, unsigned int len)
{
	struct pcpu_sw_netstats *tstats = this_cpu_ptr(dev->tstats);

	u64_stats_update_begin(&tstats->syncp);
	tstats->rx_bytes += len;
	tstats->rx_packets++;
	u64_stats_update_end(&tstats->syncp);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && !defined(EL8) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)

/* Iterate through singly-linked GSO fragments of an skb. */
#define skb_list_walk_safe(first, skb, next_skb)				\
	for ((skb) = (first), (next_skb) = (skb) ? (skb)->next : NULL; (skb);	\
	     (skb) = (next_skb), (next_skb) = (skb) ? (skb)->next : NULL)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
/**
 * rcu_replace_pointer() - replace an RCU pointer, returning its old value
 * @rcu_ptr: RCU pointer, whose old value is returned
 * @ptr: regular pointer
 * @c: the lockdep conditions under which the dereference will take place
 *
 * Perform a replacement, where @rcu_ptr is an RCU-annotated
 * pointer and @c is the lockdep argument that is passed to the
 * rcu_dereference_protected() call used to read that pointer.  The old
 * value of @rcu_ptr is returned, and @rcu_ptr is set to @ptr.
 */
#undef rcu_replace_pointer
#define rcu_replace_pointer(rcu_ptr, ptr, c)				\
({									\
	typeof(ptr) __tmp = rcu_dereference_protected((rcu_ptr), (c));	\
	rcu_assign_pointer((rcu_ptr), (ptr));				\
	__tmp;								\
})

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)

/* commit 895b5c9f206e renamed nf_reset to nf_reset_ct */
#undef nf_reset_ct
#define nf_reset_ct nf_reset

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)

/* commit 1550c171935d introduced rt_gw4 and rt_gw6 for IPv6 gateways */
#define rt_gw4 rt_gateway

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) */


#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#include <linux/atomic.h>
#ifdef OVPN_MIPS_ARCH
static inline s64 atomic64_fetch_add_unless(atomic64_t *v, s64 a, s64 u)
{
	s64 c, old;
	c = atomic64_read(v);
	for (;;)
	{
		if (unlikely(c == u))
			break;
		old = atomic64_cmpxchg((v), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}

	return c;
}
#else
static inline s64 atomic64_fetch_add_unless(atomic64_t *v, s64 a, s64 u)
{
	s64 oldval, newval;
	unsigned long tmp;

	smp_mb();
	prefetchw(&v->counter);

	__asm__ __volatile__("@ atomic64_add_unless\n"
"1:	ldrexd	%0, %H0, [%4]\n"
"	teq	%0, %5\n"
"	teqeq	%H0, %H5\n"
"	beq	2f\n"
"	adds	%Q1, %Q0, %Q6\n"
"	adc	%R1, %R0, %R6\n"
"	strexd	%2, %1, %H1, [%4]\n"
"	teq	%2, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (oldval), "=&r" (newval), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (u), "r" (a)
	: "cc");

	if (oldval != u)
		smp_mb();

	return oldval;
}
#endif
#define atomic64_fetch_add_unless atomic64_fetch_add_unless
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>
static inline void *__compat_kvmalloc(size_t size, gfp_t flags)
{
	gfp_t kmalloc_flags = flags;
	void *ret;
	if (size > PAGE_SIZE) {
		kmalloc_flags |= __GFP_NOWARN;
		if (!(kmalloc_flags & __GFP_REPEAT) || (size <= PAGE_SIZE << PAGE_ALLOC_COSTLY_ORDER))
			kmalloc_flags |= __GFP_NORETRY;
	}
	ret = kmalloc(size, kmalloc_flags);
	if (ret || size <= PAGE_SIZE)
		return ret;
	return __vmalloc(size, flags, PAGE_KERNEL);
}
static inline void *__compat_kvzalloc(size_t size, gfp_t flags)
{
	return __compat_kvmalloc(size, flags | __GFP_ZERO);
}
#define kvmalloc __compat_kvmalloc
#define kvzalloc __compat_kvzalloc
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#define NLA_POLICY_RANGE(tp, _min, _max) { .type = tp }
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 9)
#include <linux/netdevice.h>
#define priv_destructor destructor
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
#include <linux/skbuff.h>
#define skb_probe_transport_header(a) skb_probe_transport_header(a, 0)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 10) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)) || LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 217)
static inline void skb_mark_not_on_list(struct sk_buff *skb)
{
	skb->next = NULL;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#include <linux/timer.h>
#define timer_setup(a, b, c) setup_timer(a, ((void (*)(unsigned long))b), ((unsigned long)a))
#define from_timer(var, callback_timer, timer_fieldname) container_of((struct timer_list *)callback_timer, typeof(*var), timer_fieldname)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 83)
#define ipv6_dst_lookup_flow(a, b, c, d) ipv6_dst_lookup_flow(b, c, d)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 5) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) || (LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 18) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)) || ( LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 119) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)) || (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 181) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) || (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 224) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)) || (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 224))
#define ipv6_dst_lookup_flow(a, b, c, d) ipv6_dst_lookup(a, b, &dst, c) + (void *)0 ?: dst
#define COMPAT_IPV6_NEED_SPECIAL_HANDLING
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#include <net/netlink.h>
#include <net/genetlink.h>
#define nlmsg_parse(a, b, c, d, e, f) nlmsg_parse(a, b, c, d, e)
#define nla_parse_nested(a, b, c, d, e) nla_parse_nested(a, b, c, d)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#include <net/netlink.h>
#ifndef NLA_POLICY_EXACT_LEN
#define NLA_POLICY_EXACT_LEN(_len) { .type = NLA_UNSPEC, .len = _len }
#endif
#ifndef NLA_POLICY_NESTED
#define NLA_POLICY_NESTED(policy) { .type = NLA_NESTED, .len = ARRAY_SIZE(policy) - 1 }
#endif
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#include <net/netlink.h>
#ifndef NLA_POLICY_MIN_LEN
#define NLA_POLICY_MIN_LEN(_len) { .type = NLA_UNSPEC, .len = _len }
#endif
#define COMPAT_CANNOT_INDIVIDUAL_NETLINK_OPS_POLICY
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
#include <net/netlink.h>
#define nla_put_u64_64bit(a, b, c, d) nla_put_u64(a, b, c)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#include <net/netlink.h>
#define nlmsg_parse_deprecated(a, b, c, d, e, f) nlmsg_parse(a, b, c, d, e, f)
#endif

#include <linux/cache.h>
#include <linux/init.h>
#ifndef __ro_after_init
#define __ro_after_init __read_mostly
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#include <linux/crypto.h>
struct crypto_wait {
	struct completion completion;
	int err;
};

#define DECLARE_CRYPTO_WAIT(_wait) \
	struct crypto_wait _wait = { \
		COMPLETION_INITIALIZER_ONSTACK((_wait).completion), 0 }

static inline void crypto_req_done(struct crypto_async_request *req, int err)
{
	struct crypto_wait *wait = req->data;

	if (err == -EINPROGRESS)
		return;

	wait->err = err;
	complete(&wait->completion);
}

static inline int crypto_wait_req(int err, struct crypto_wait *wait)
{
	switch (err) {
	case -EINPROGRESS:
	case -EBUSY:
		wait_for_completion(&wait->completion);
		reinit_completion(&wait->completion);
		err = wait->err;
		break;
	};

	return err;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#include <net/udp_tunnel_partial_compat.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
#include <linux/skbuff.h>
#ifdef OVPN_NO_KERNEL_COMPAT
static inline void * skb_put_data(struct sk_buff *skb, const void *data, unsigned int len)
{
	void *tmp = skb_put(skb, len);
	memcpy(tmp, data, len);
	return tmp;
}
#endif
#endif

#endif /* _NET_OVPN_DCO_LINUX_COMPAT_H_ */
