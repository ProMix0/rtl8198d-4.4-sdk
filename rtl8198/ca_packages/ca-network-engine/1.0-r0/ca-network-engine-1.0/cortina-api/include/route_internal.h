#ifndef __ROUTE_INTERNAL_H__
#define __ROUTE_INTERNAL_H__

#include <linux/list.h>
#include "ca_types.h"
#include "route.h"

/***************************************************************************
 * Internal Use
 ***************************************************************************/

struct nexthop_node {
	ca_l3_nexthop_t		nexthop;		/* sort key is nexthop.nexthop_id */

	int32_t			hash_idx_neighbor;	/* hash_idx of neighbor host (nexthop) */
	bool			sw_only;		/* don't create hash for software-only nexthop entry */

	struct list_head	list;
};

struct route_node {
	ca_l3_route_t		route;

	unsigned int		lpm_idx;
	unsigned int		rst_idx;

	struct list_head	list;
};

#endif /* __ROUTE_INTENRAL_H__ */
