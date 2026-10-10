// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#ifndef _NET_OVPN_DCO_OVPNLZO_H_
#define _NET_OVPN_DCO_OVPNLZO_H_

#include <linux/lzo.h>
#include <linux/skbuff.h>

/*
 * LZO compress/decompress function
 */
int ovpn_lzo_compress(struct sk_buff *skb);

int ovpn_lzo_decompress(struct sk_buff *skb);

#endif  /* _NET_OVPN_DCO_OVPNLZO_H_ */