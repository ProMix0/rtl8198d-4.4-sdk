// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#include "comp.h"

int ovpn_compress_state_reset(struct ovpn_compress_state *compress,
                                enum ovpn_compress_alg comp_alg)
{
    switch (comp_alg) {
    case OVPN_COMPRESS_ALG_LZO:
    case OVPN_COMPRESS_ALG_LZ4:
    case OVPN_COMPRESS_ALG_LZ4V2:
    case OVPN_COMPRESS_ALG_STUB:
    case OVPN_COMPRESS_ALG_STUB_NOSWAP:
    case OVPN_COMPRESS_ALG_STUBV2:
        compress->comp_enabled = true;
        compress->comp_alg = comp_alg;
        break;
    case OVPN_COMPRESS_ALG_NONE:
        compress->comp_enabled = false;
        compress->comp_alg = comp_alg;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

int ovpn_compress(struct sk_buff *skb, enum ovpn_compress_alg comp_alg)
{
    int ret = 0;

    switch (comp_alg) {
    case OVPN_COMPRESS_ALG_LZO:
        ret = ovpn_lzo_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_LZ4:
        ret = ovpn_lz4_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_LZ4V2:
        ret = ovpn_lz4v2_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_STUB:
        ret = ovpn_stub_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_STUB_NOSWAP:
        ret = ovpn_stub_noswap_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_STUBV2:
        ret = ovpn_stubv2_compress(skb);
        break;
    case OVPN_COMPRESS_ALG_NONE:
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

int ovpn_decompress(struct sk_buff *skb, enum ovpn_compress_alg comp_alg)
{
    int ret = 0;

    switch (comp_alg) {
    case OVPN_COMPRESS_ALG_LZO:
        ret = ovpn_lzo_decompress(skb);
        break;
    case OVPN_COMPRESS_ALG_LZ4:
        ret = ovpn_lz4_decompress(skb);
        break;
    case OVPN_COMPRESS_ALG_LZ4V2:
        ret = ovpn_lz4v2_decompress(skb);
        break;
    case OVPN_COMPRESS_ALG_STUB:
    case OVPN_COMPRESS_ALG_STUB_NOSWAP:
        ret = ovpn_stub_decompress(skb);
        break;
    case OVPN_COMPRESS_ALG_STUBV2:
        ret = ovpn_stubv2_decompress(skb);
        break;
    case OVPN_COMPRESS_ALG_NONE:
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}