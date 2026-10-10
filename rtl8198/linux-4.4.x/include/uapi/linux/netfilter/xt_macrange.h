#ifndef _XT_MAC_RANGE_H
#define _XT_MAC_RANGE_H

#define MAC_SRC_RANGE				0x01	/* Match source MAC address range*/
#define MAC_DST_RANGE				0x02	/* Match destination MAC address range*/
#define MAC_SRC_RANGE_INV			0x10	/* Negate the condition */
#define MAC_DST_RANGE_INV			0x20	/* Negate the condition */

struct xt_mac_range{
    unsigned char macaddr_start[ETH_ALEN];
	unsigned char macaddr_end[ETH_ALEN];
};

struct xt_mac_range_info {
   struct xt_mac_range srcaddr;
   struct xt_mac_range dstaddr;
   u_int8_t flags;
};

#endif /*_XT_MAC_RANGE_H*/

