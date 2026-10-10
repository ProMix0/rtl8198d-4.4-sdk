// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2023 Shenzhen TP-Link Technologies Co.Ltd.
 *
 *  Author:	Zhu Haijun <zhuhaijun@tp-link.com.hk>
 *
 */

#include "lz4.h"
#include "comp.h"

int ovpn_lz4_compress(struct sk_buff *skb)
{
    if (unlikely(skb->len <= 0))
        return -EINVAL;

    *(uint8_t *)(skb->data + skb->len) = *(uint8_t *)(skb->data);
    *(uint8_t *)(skb->data) = NO_COMPRESS_BYTE_SWAP;
    skb->tail += COMP_PREFIX_LEN;
    skb->len += COMP_PREFIX_LEN;

    return 0;
}

int ovpn_lz4v2_compress(struct sk_buff *skb)
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
static int do_lz4_decompress(struct sk_buff *skb)
{
    size_t dst_len = MAX_SRC_LEN;
    size_t src_len = skb->len;
    unsigned char *dest = NULL;
    int err = 0;

    dest = (unsigned char *)kvmalloc(MAX_SRC_LEN, GFP_KERNEL);
    if (unlikely(!dest))
        return -ENOMEM;

    err = lz4_decompress_unknownoutputsize((const unsigned char *)skb->data, src_len,
                                    dest, &dst_len);
    if (err != 0) {
        net_err_ratelimited("%s: decompress failed: %d\n", __func__, err);
        goto error;
    }

    if (skb->end - skb->tail < dst_len - src_len + OVPN_MAX_PADDING) {
        skb->next = skb_copy_expand(skb, skb_headroom(skb),
                    dst_len + OVPN_MAX_PADDING - skb->len, GFP_ATOMIC);
        if (unlikely(!skb->next)) {
            err = -ENOMEM;
            goto error;
        }
        memcpy(skb->next->data, dest, dst_len);
        skb->next->tail = skb->next->tail + dst_len - src_len;
        skb->next->len = dst_len;
    }
    else {
        memcpy(skb->data, dest, dst_len);
        skb->tail = skb->tail + dst_len - src_len;
        skb->len = dst_len;
    }

error:
    if (NULL != dest)
        kvfree(dest);
    return err;
}
#else
static int do_lz4_decompress(struct sk_buff *skb)
{
    int dst_len = 0, err = 0;
    int src_len = skb->len;
    int max_dstlen = MAX_SRC_LEN;
    char *dest = NULL;

    dest = (char *)kvmalloc(MAX_SRC_LEN, GFP_KERNEL);
    if (unlikely(!dest))
        return -ENOMEM;

    dst_len = LZ4_decompress_safe((const char *)skb->data, dest, src_len,
                                    max_dstlen);
    if (dst_len <= 0) {
        net_err_ratelimited("%s: decompress failed: %d\n", __func__, dst_len);
        err = -1;
        goto error;
    }

    if (skb->end - skb->tail < dst_len - src_len + OVPN_MAX_PADDING) {
        skb->next = skb_copy_expand(skb, skb_headroom(skb),
                    dst_len + OVPN_MAX_PADDING - skb->len, GFP_ATOMIC);
        if (unlikely(!skb->next)) {
            err = -ENOMEM;
            goto error;
        }
        memcpy(skb->next->data, dest, dst_len);
        skb->next->tail = skb->next->tail + dst_len - src_len;
        skb->next->len = dst_len;
    }
    else {
        memcpy(skb->data, dest, dst_len);
        skb->tail = skb->tail + dst_len - src_len;
        skb->len = dst_len;
    }

error:
    if (NULL != dest)
        kvfree(dest);
    return err;
}
#endif

int ovpn_lz4_decompress(struct sk_buff *skb)
{
    uint8_t c;  /* flag indicating whether or not our peer compressed */

    if (skb->len <= 1)
        return -EINVAL;

    /* do unframing/swap (assumes buf->len > 0) */
    {
        c = *(uint8_t *)(skb->data);
        *(uint8_t *)(skb->data) = *(uint8_t *)(skb->data + skb->len - COMP_PREFIX_LEN);
        skb->tail -= COMP_PREFIX_LEN;
        skb->len -= COMP_PREFIX_LEN;
    }

    if (c == LZ4_COMPRESS_BYTE)
    {
        return do_lz4_decompress(skb);
    }
    else if (c == NO_COMPRESS_BYTE_SWAP)
    {
        /* nothing to do */
    }
    else
    {
        net_err_ratelimited("%s: Bad LZ4 decompression header byte: %d\n", __func__, c);
        return -1;
    }

    return 0;
}

int ovpn_lz4v2_decompress(struct sk_buff *skb)
{
    uint8_t c;  /* flag indicating whether or not our peer compressed */

    if (skb->len <= 0)
        return -EINVAL;

    c = *(uint8_t *)(skb->data);

    /* Not compressed */
    if (c != COMP_ALGV2_INDICATOR_BYTE)
        return 0;

    if (skb->len <= 2)
        return -EINVAL;

    /* do unframing */
    {
        c = *(uint8_t *)(skb->data + 1);
        __skb_pull(skb, COMPV2_PREFIX_LEN);
    }

    if (c == COMP_ALGV2_LZ4_BYTE)
    {
        return do_lz4_decompress(skb);
    }
    else if (c == COMP_ALGV2_UNCOMPRESSED_BYTE)
    {
        /* nothing to do */
    }
    else
    {
        net_err_ratelimited("%s: Bad LZ4v2 decompression header byte: %d\n", __func__, c);
        return -1;
    }

    return 0;
}