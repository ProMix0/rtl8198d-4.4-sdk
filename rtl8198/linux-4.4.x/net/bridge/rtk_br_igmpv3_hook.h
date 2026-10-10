#ifndef __RTK_IGMPV3_DATABASE_HOOK_BR_H__
#define __RTK_IGMPV3_DATABASE_HOOK_BR_H__
#include <net/rtl/rtk_igmpv3_database_hook.h>

void rtk_br_igmpv3_hook_get_fwdPortMask(struct net_bridge *br,struct sk_buff *skb,unsigned int *fwdPortMask);

#endif