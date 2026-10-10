// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#ifndef _NET_OVPN_DCO_OVPNLZ4_H_
#define _NET_OVPN_DCO_OVPNLZ4_H_

#include <linux/lz4.h>
#include <linux/skbuff.h>

/*
 * LZ4 compress/decompress function
 */
int ovpn_lz4_compress(struct sk_buff *skb);

int ovpn_lz4_decompress(struct sk_buff *skb);

/*
 * LZ4v2 compress/decompress function
 */
int ovpn_lz4v2_compress(struct sk_buff *skb);

int ovpn_lz4v2_decompress(struct sk_buff *skb);

#endif  /* _NET_OVPN_DCO_OVPNLZ4_H_ */