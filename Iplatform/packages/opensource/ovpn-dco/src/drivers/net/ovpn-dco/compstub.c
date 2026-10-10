// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#include "compstub.h"
#include "comp.h"

int ovpn_stub_compress(struct sk_buff *skb)
{
    if (unlikely(skb->len <= 0))
        return -EINVAL;

    *(uint8_t *)(skb->data + skb->len) = *(uint8_t *)(skb->data);
    *(uint8_t *)(skb->data) = NO_COMPRESS_BYTE_SWAP;
    skb->tail += COMP_PREFIX_LEN;
    skb->len += COMP_PREFIX_LEN;

    return 0;
}

int ovpn_stub_noswap_compress(struct sk_buff *skb)
{
    if (unlikely(skb->len <= 0))
        return -EINVAL;

#if defined(QCA_IPQ50XX)
    /* to maintain skb->data alignment, but may affects the performance */
    /* for maple (e.g. AX55), must maintain skb->data alignment */
    memmove(skb->data + COMP_PREFIX_LEN, skb->data, skb->len);
    *(uint8_t *)(skb->data) = NO_COMPRESS_BYTE;
    skb->tail += COMP_PREFIX_LEN;
    skb->len += COMP_PREFIX_LEN;
#else
    __skb_push(skb, COMP_PREFIX_LEN);
    *(uint8_t *)(skb->data) = NO_COMPRESS_BYTE;
#endif

    return 0;
}

int ovpn_stubv2_compress(struct sk_buff *skb)
{
    if (unlikely(skb->len <= 0))
        return -EINVAL;

    if (*(uint8_t *)(skb->data) != COMP_ALGV2_INDICATOR_BYTE)
        return 0;

    memmove(skb->data + COMPV2_PREFIX_LEN, skb->data, skb->len);
    *(uint8_t *)(skb->data) = COMP_ALGV2_INDICATOR_BYTE;
    *(uint8_t *)(skb->data + 1) = COMP_ALGV2_UNCOMPRESSED_BYTE;
    skb->tail += COMPV2_PREFIX_LEN;
    skb->len += COMPV2_PREFIX_LEN;

    return 0;
}

int ovpn_stub_decompress(struct sk_buff *skb)
{
    uint8_t c;  /* flag */

    if (skb->len <= 1)
        return -EINVAL;

    c = *(uint8_t *)(skb->data);

    if (c == NO_COMPRESS_BYTE_SWAP)
    {
        *(uint8_t *)(skb->data) = *(uint8_t *)(skb->data + skb->len - COMP_PREFIX_LEN);
        skb->tail -= COMP_PREFIX_LEN;
        skb->len -= COMP_PREFIX_LEN;
    }
    else if (c == NO_COMPRESS_BYTE)
    {
        __skb_pull(skb, COMP_PREFIX_LEN);
    }
    else
    {
        net_err_ratelimited("%s: Bad compression stub decompression header byte: %d\n",
                            __func__, c);
        return -1;
    }

    return 0;
}

int ovpn_stubv2_decompress(struct sk_buff *skb)
{
    uint8_t c;  /* flag */

    if (skb->len <= 0)
        return -EINVAL;

    c = *(uint8_t *)(skb->data);

    /* Not compressed */
    if (c != COMP_ALGV2_INDICATOR_BYTE)
        return 0;

    if (skb->len <= 2)
        return -EINVAL;

    c = *(uint8_t *)(skb->data + 1);
    if (c == COMP_ALGV2_UNCOMPRESSED_BYTE)
    {
        __skb_pull(skb, COMPV2_PREFIX_LEN);
    }
    else
    {
        net_err_ratelimited("%s: Bad compression stubv2 decompression header byte: %d\n",
                            __func__, c);
        return -1;
    }

    return 0;
}