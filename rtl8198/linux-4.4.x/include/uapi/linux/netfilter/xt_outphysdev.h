#ifndef _XT_OUTPHYSDEV_H
#define _XT_OUTPHYSDEV_H

struct xt_outphysdev_info {
    char out_physdev[IFNAMSIZ];
	char mask[IFNAMSIZ];
	__u8 bitmask;
};

#endif /*_XT_OUTPHYSDEV_H*/
