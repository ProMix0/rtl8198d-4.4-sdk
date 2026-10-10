#ifndef _UTIL_BITMAP_H_
#define _UTIL_BITMAP_H_
#include "ca_types.h"

void ca_bit_set(ca_uint8_t *pbuf , ca_int32_t bit_pos);
void ca_bit_clr(ca_uint8_t *pbuf , ca_int32_t bit_pos);
ca_int32_t ca_bit_tst(ca_uint8_t *pbuf , ca_int32_t bit_pos);
ca_int32_t ca_bit_get_0(ca_uint8_t *pbuf , ca_int32_t total_bit);
ca_int32_t ca_bit_get_1(ca_uint8_t *pbuf , ca_int32_t total_bit);
ca_int32_t ca_bit_tst_all(ca_uint8_t *pbuf , ca_int32_t total_bit);

#endif
