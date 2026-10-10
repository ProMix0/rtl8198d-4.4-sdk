// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#include "lzo.h"
#include "comp.h"

int ovpn_lzo_compress(struct sk_buff *skb)
{
    size_t src_len = skb->len;

    if (unlikely(src_len <= 0))
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

    return LZO_E_OK;
}

int ovpn_lzo_decompress(struct sk_buff *skb)
{
    size_t dst_len = MAX_SRC_LEN;
    size_t src_len = skb->len - COMP_PREFIX_LEN;
    unsigned char *dst = NULL;
    int err = LZO_E_OK;
    uint8_t c;      /* flag indicating whether or not our peer compressed */

    if (unlikely(src_len) <= 0)
        return -EINVAL;

    c = *(uint8_t *)(skb->data);
    __skb_pull(skb, COMP_PREFIX_LEN);

    if (c == LZO_COMPRESS_BYTE)     /* packet was compressed */
    {
    	dst = (unsigned char *)kvmalloc(MAX_SRC_LEN, GFP_KERNEL);
    	if (unlikely(!dst))
            return -ENOMEM;

        err = lzo1x_decompress_safe((const unsigned char *)skb->data, src_len, dst, &dst_len);
        if (err != LZO_E_OK)
        {
            net_err_ratelimited("%s: decompress failed: %d\n", __func__, err);
            goto error;
        }

        if (skb->end - skb->tail < dst_len - src_len + OVPN_MAX_PADDING) {
            skb->next = skb_copy_expand(skb, skb_headroom(skb), dst_len + OVPN_MAX_PADDING - skb->len, GFP_ATOMIC);
            if (unlikely(!skb->next)) {
                err = -ENOMEM;
                goto error;
            }
            memcpy(skb->next->data, dst, dst_len);
            skb->next->tail = skb->next->tail + dst_len - src_len;
            skb->next->len = dst_len;
        }
        else
        {
            memcpy(skb->data, dst, dst_len);
            skb->tail = skb->tail + dst_len - src_len;
            skb->len = dst_len;
        }
    }
    else if (c == NO_COMPRESS_BYTE)
    {
        /* nothing to do */
    }
    else
    {
        net_err_ratelimited("%s: Bad LZO decompression header byte: %d\n", __func__, c);
        return LZO_E_ERROR;
    }

error:
    if (NULL != dst)
        kvfree(dst);
    return err;
}