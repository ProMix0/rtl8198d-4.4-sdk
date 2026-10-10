// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#ifndef _NET_OVPN_DCO_OVPNCOMPSTUB_H_
#define _NET_OVPN_DCO_OVPNCOMPSTUB_H_

#include <linux/skbuff.h>

/*
 * STUB compress/decompress function
 */
int ovpn_stub_compress(struct sk_buff *skb);

int ovpn_stub_noswap_compress(struct sk_buff *skb);

int ovpn_stub_decompress(struct sk_buff *skb);

/*
 * STUBv2 compress/decompress function
 */
int ovpn_stubv2_compress(struct sk_buff *skb);

int ovpn_stubv2_decompress(struct sk_buff *skb);

#endif  /* _NET_OVPN_DCO_OVPNCOMPSTUB_H_ */