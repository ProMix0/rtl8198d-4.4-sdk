#ifndef _XT_FLOWLABEL_RANGE_H
#define _XT_FLOWLABEL_RANGE_H

#define XT_FLOWLABEL_MAX		0xFFFFF	

/* match info */
struct xt_flowlbl_range_info {
	__u32 flowlbl_min;
	__u32 flowlbl_max;
	__u8 invert;
};

#endif /* _XT_FLOWLABEL_RANGE_H */
