#include "ca_types.h"

static ca_uint8_t bitMask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

void ca_bit_set(ca_uint8_t *pbuf , ca_int32_t bit_pos)
{
    pbuf[bit_pos/8] |= bitMask[bit_pos%8];
}

void ca_bit_clr(ca_uint8_t *pbuf , ca_int32_t bit_pos)
{
    pbuf[bit_pos/8] &= ~(bitMask[bit_pos%8]);
}

ca_int32_t ca_bit_tst(ca_uint8_t *pbuf , ca_int32_t bit_pos)
{
    if (pbuf[bit_pos/8] & bitMask[bit_pos%8])
        return 1;
    else
        return 0;
}

ca_int32_t ca_bit_get_0(ca_uint8_t *pbuf , ca_int32_t total_bit)
{
    ca_int32_t intNum = 0;
    ca_int32_t left_bits = 0;
    ca_uint32_t *pInt = (ca_uint32_t *)pbuf;
    int i , j;

    intNum = total_bit / 32;
    left_bits = total_bit % 32;
    for (i = 0 ; i < intNum ; i++) {
        if (pInt[i] == (ca_uint32_t)0xFFFFFFFF)
            continue;

        for (j = 0 ; j < 32 ; j++) {
            if (!ca_bit_tst((ca_uint8_t *)&pInt[i] , j))
                return (i*32 + j);
        }
    }

    for (j = 0 ; j < left_bits ; j++) {
        if (!ca_bit_tst((ca_uint8_t *)&pInt[intNum] , j))
            return (intNum*32 + j);
    }

    return -1;
}

ca_int32_t ca_bit_get_1(ca_uint8_t *pbuf , ca_int32_t total_bit)
{
    ca_int32_t intNum = 0;
    ca_int32_t left_bits = 0;
    ca_uint32_t *pInt = (ca_uint32_t *)pbuf;
    int i , j;

    intNum = total_bit / 32;
    left_bits = total_bit % 32;
    for (i = 0 ; i < intNum ; i++) {
        if (pInt[i] == 0)
            continue;

        for (j = 0 ; j < 32 ; j++) {
            if (ca_bit_tst((ca_uint8_t *)&pInt[i] , j))
                return (i*32 + j);
        }
    }

    for (j = 0 ; j < left_bits ; j++) {
        if (ca_bit_tst((ca_uint8_t *)&pInt[intNum] , j))
            return (intNum*32 + j);
    }

    return -1;
}

ca_int32_t ca_bit_tst_all(ca_uint8_t *pbuf , ca_int32_t total_bit)
{
    if (ca_bit_get_0(pbuf , total_bit) == -1)
        return 1;
    else
        return 0;
}

