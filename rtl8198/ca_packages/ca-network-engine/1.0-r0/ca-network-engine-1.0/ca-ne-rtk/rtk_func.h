#ifndef __CA_RTK_FUNC_H__
#define __CA_RTK_FUNC_H__

#if defined(CONFIG_RTK_NIC_RX_HOOK) || defined(CONFIG_RTK_NIC_TX_HOOK)
#include "ca-ne-rtk/ethctl_implement.h"
#endif

#ifdef CONFIG_RTK_HOST_SPEEDUP
#include "../../../../../linux-4.4.x/net/ipv4/ip_speedup.h"
#include <net/tcp.h>
#include "ca_ext.h"
#endif


#ifdef CONFIG_RTK_CA_NE_SKB_LIMIT
/* 10 sec, printk 1 time */
#define LUNA_G3_PRINTK_TIMED_RATELIMIT                          static unsigned long int tj;if (printk_timed_ratelimit(&tj, 10000))
	
#define LUNA_G3_NETDEV_ALLOC_SKB_LIMIT                          if (rtk_netdev_alloc_skb_limit()){      return NULL; }

extern const struct file_operations fops_ni_skb_limit;
extern int rtk_netdev_alloc_skb_limit(void);

#else
	
#define LUNA_G3_PRINTK_TIMED_RATELIMIT
#define LUNA_G3_NETDEV_ALLOC_SKB_LIMIT

#endif


#endif

