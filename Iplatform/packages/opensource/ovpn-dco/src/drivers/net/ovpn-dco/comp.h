// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#ifndef _NET_OVPN_DCO_OVPNCOMP_H_
#define _NET_OVPN_DCO_OVPNCOMP_H_

#include "lzo.h"
#include "lz4.h"
#include "compstub.h"
#include "main.h"

#include <uapi/linux/ovpn_dco.h>
#include <linux/skbuff.h>

/*
 * Length of prepended prefix on compressed packets
 */
#define COMP_PREFIX_LEN 1
#define COMPV2_PREFIX_LEN 2

/*
 * Prefix bytes
 */
/* V1 on wire codes */
/* Initial command byte to tell our peer if we compressed */
#define LZO_COMPRESS_BYTE       0x66
#define LZ4_COMPRESS_BYTE       0x69
#define NO_COMPRESS_BYTE        0xFA
#define NO_COMPRESS_BYTE_SWAP   0xFB  /* to maintain payload alignment */

/* V2 on wire codes */
#define COMP_ALGV2_INDICATOR_BYTE       0x50
#define COMP_ALGV2_UNCOMPRESSED_BYTE    0
#define COMP_ALGV2_LZ4_BYTE             1

/*
 * Maximum skb->len before compress
 */
#define MAX_SRC_LEN 1600

struct ovpn_compress_state {
    enum ovpn_compress_alg comp_alg;

    bool comp_enabled;
};

static inline void ovpn_compress_state_init(struct ovpn_compress_state *compress)
{
    compress->comp_alg = OVPN_COMPRESS_ALG_NONE;
    compress->comp_enabled = false;
}

int ovpn_compress_state_reset(struct ovpn_compress_state *compress,
                                enum ovpn_compress_alg comp_alg);

int ovpn_compress(struct sk_buff *skb, enum ovpn_compress_alg comp_alg);

int ovpn_decompress(struct sk_buff *skb, enum ovpn_compress_alg comp_alg);

#endif  /* _NET_OVPN_DCO_OVPNCOMP_H_ */