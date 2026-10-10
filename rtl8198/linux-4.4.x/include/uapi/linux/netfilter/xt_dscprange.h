#ifndef _XT_DSCP_RANGE_H
#define _XT_DSCP_RANGE_H

#define XT_DSCP_MASK	0xfc	/* 11111100 */
#define XT_DSCP_SHIFT	2
#define XT_DSCP_MAX		0x3f	/* 00111111 */
#define XT_TOS_MAX		0xff

/* match info */
struct xt_dscp_range_info {
	__u8 dscp_min;
	__u8 dscp_max;
	__u8 invert;
};

struct xt_tos_range_info {
	__u8 tos_min;
	__u8 tos_max;
	__u8 invert;
};

#endif /* _XT_DSCP_RANGE_H */

