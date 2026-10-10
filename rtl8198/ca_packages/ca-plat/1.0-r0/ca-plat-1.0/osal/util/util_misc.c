/****************************************************************************
            Software License for Customer Use of Cortina Software
                          Grant Terms and Conditions

IMPORTANT NOTICE - READ CAREFULLY: This Software License for Customer Use
of Cortina Software ("LICENSE") is the agreement which governs use of
software of Cortina Systems, Inc. and its subsidiaries ("CORTINA"),
including computer software (source code and object code) and associated
printed materials ("SOFTWARE").  The SOFTWARE is protected by copyright laws
and international copyright treaties, as well as other intellectual property
laws and treaties.  The SOFTWARE is not sold, and instead is only licensed
for use, strictly in accordance with this document.  Any hardware sold by
CORTINA is protected by various patents, and is sold but this LICENSE does
not cover that sale, since it may not necessarily be sold as a package with
the SOFTWARE.  This LICENSE sets forth the terms and conditions of the
SOFTWARE LICENSE only.  By downloading, installing, copying, or otherwise
using the SOFTWARE, you agree to be bound by the terms of this LICENSE.
If you do not agree to the terms of this LICENSE, then do not download the
SOFTWARE.

DEFINITIONS:  "DEVICE" means the Cortina Systems?IROS SDK product.
"You" or "CUSTOMER" means the entity or individual that uses the SOFTWARE.
"SOFTWARE" means the Cortina Systems?SDK software.

GRANT OF LICENSE:  Subject to the restrictions below, CORTINA hereby grants
CUSTOMER a non-exclusive, non-assignable, non-transferable, royalty-free,
perpetual copyright license to (1) install and use the SOFTWARE for
reference only with the DEVICE; and (2) copy the SOFTWARE for your internal
use only for use with the DEVICE.

RESTRICTIONS:  The SOFTWARE must be used solely in conjunction with the
DEVICE and solely with Your own products that incorporate the DEVICE.  You
may not distribute the SOFTWARE to any third party.  You may not modify
the SOFTWARE or make derivatives of the SOFTWARE without assigning any and
all rights in such modifications and derivatives to CORTINA.  You shall not
through incorporation, modification or distribution of the SOFTWARE cause
it to become subject to any open source licenses.  You may not
reverse-assemble, reverse-compile, or otherwise reverse-engineer any
SOFTWARE provided in binary or machine readable form.  You may not
distribute the SOFTWARE to your customers without written permission
from CORTINA.

OWNERSHIP OF SOFTWARE AND COPYRIGHTS. All title and copyrights in and the
SOFTWARE and any accompanying printed materials, and copies of the SOFTWARE,
are owned by CORTINA. The SOFTWARE protected by the copyright laws of the
United States and other countries, and international treaty provisions.
You may not remove any copyright notices from the SOFTWARE.  Except as
otherwise expressly provided, CORTINA grants no express or implied right
under CORTINA patents, copyrights, trademarks, or other intellectual
property rights.

DISCLAIMER OF WARRANTIES. THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY
EXPRESS OR IMPLIED WARRANTY OF ANY KIND, INCLUDING ANY IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE,
TITLE, AND NON-INFRINGEMENT.  CORTINA does not warrant or assume
responsibility for the accuracy or completeness of any information, text,
graphics, links or other items contained within the SOFTWARE.  Without
limiting the foregoing, you are solely responsible for determining and
verifying that the SOFTWARE that you obtain and install is the appropriate
version for your purpose.

LIMITATION OF LIABILITY. IN NO EVENT SHALL CORTINA OR ITS SUPPLIERS BE
LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, LOST
PROFITS, BUSINESS INTERRUPTION, OR LOST INFORMATION) OR ANY LOSS ARISING OUT
OF THE USE OF OR INABILITY TO USE OF OR INABILITY TO USE THE SOFTWARE, EVEN
IF CORTINA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
TERMINATION OF THIS LICENSE. This LICENSE will automatically terminate if
You fail to comply with any of the terms and conditions hereof. Upon
termination, You will immediately cease use of the SOFTWARE and destroy all
copies of the SOFTWARE or return all copies of the SOFTWARE in your control
to CORTINA.  IF you commence or participate in any legal proceeding against
CORTINA, then CORTINA may, in its sole discretion, suspend or terminate all
license grants and any other rights provided under this LICENSE during the
pendency of such legal proceedings.
APPLICABLE LAWS. Claims arising under this LICENSE shall be governed by the
laws of the State of California, excluding its principles of conflict of
laws.  The United Nations Convention on Contracts for the International Sale
of Goods is specifically disclaimed.  You shall not export the SOFTWARE
without first obtaining any required export license or other approval from
the applicable governmental entity, if required.  This is the entire
agreement and understanding between You and CORTINA relating to this subject
matter.
GOVERNMENT RESTRICTED RIGHTS. The SOFTWARE is provided with "RESTRICTED
RIGHTS." Use, duplication, or disclosure by the Government is subject to
restrictions as set forth in FAR52.227-14 and DFAR252.227-7013 et seq. or
its successor. Use of the SOFTWARE by the Government constitutes
acknowledgment of CORTINA's proprietary rights therein. Contractor or
Manufacturer is CORTINA.

Copyright (c) 2009 by Cortina Systems Incorporated
****************************************************************************/
#include "ca_types.h"
#include "osal_cmn.h"
#include "util_misc.h"
#include "osal_mem.h"
#include "os_core.h"

#define MAX_HEX_STRING_LEN 10

typedef struct {
    ca_uint_t rp;
    ca_uint32_t wp;
    ca_uint32_t buf_num;
    ca_uint32_t full;
    ca_uint32_t lock;
    ca_uint_t *ptr;
} ca_ring_buf_t;

static ca_int32_t ca_char2int(ca_int8_t c)
{
    ca_int32_t digit = 0;

    if(c <= '9' && c >= '0')
        digit = c - '0';
    else if(c <= 'f' && c >= 'a')
        digit = c - 'a' + 10;
    else
        digit = c - 'A' + 10;

    return digit;
}

ca_uint64_t ca_inter_to64(ca_uint32_t high, ca_uint32_t low)
{
    ca_uint8_t i=0;
    ca_uint64_t total = high;
    for (i = 0; i < 4; i++)
    {
           total = total<<8;
    }
    total = total + low;
    return total;
}

ca_int32_t ca_atoi(ca_int8_t *str)
{
    ca_int8_t *c = str;
    ca_int32_t num = 0;

    if(str == NULL)
        return num;

    while(*c >= '0' && *c <= '9' && *c != '\0')
    {
        num = (num*10)+*c-'0';
        c++;
    }

    return num;
}

ca_int32_t ca_str2dec(ca_int8_t *str , ca_uint32_t *num)
{
    ca_int8_t *c = str;
    ca_int32_t len = 0;
    ca_int32_t parse_status = 0;
    ca_uint32_t temp_num = 0;

    /******************************************************************
        PARSE_STATUS DESC
        ------------------------------------------------------
                   0 init status
                   1 string length is 1, it must be a dec num.
                   2 a hex-string, header is '0x'.
                   3 a decimal-string,start with a digit '1-9'
        ------------------------------------------------------
    ******************************************************************/

    len = strlen((const char*)str);
    if(len == 0)
    {
        *num = 0;
        return CA_OSAL_ERROR;
    }

    if(len == 1)
        parse_status = 1;

    while(*c != '\0')
    {
        switch(*c)
        {
        case '0':
            if(parse_status == 0)
            {
                c++;
                if(*c != 'x')   /*str = 01/02/0a/0b/0h/00, but not '0x'*/
                {
                    *num = 0;
                    return CA_OSAL_ERROR;
                }
                else
                {
                    parse_status = 2;
                    if(len > MAX_HEX_STRING_LEN)/*the max len of a hex-string must less than 10. Eg. '0x12345678'*/
                    {
                        *num = 0;
                        return CA_OSAL_ERROR;
                    }
                }
            }
            else if(parse_status == 1)  /*just one byte,and it is '0'.*/
            {
                *num = 0;
                return CA_OSAL_OK;
            }
            else if(parse_status == 2)
            {
                temp_num = temp_num<<4;
            }
            else if(parse_status == 3)
            {
                temp_num = 10*temp_num;
            }
            else
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(parse_status == 0)
            {
                temp_num = *c-'0';
                parse_status = 3;
            }
            else if(parse_status == 1)
            {
                *num = ca_atoi(str);
                return CA_OSAL_OK;
            }
            else if(parse_status == 2)
            {
                temp_num = (temp_num<<4) + *c - '0';
            }
            else if(parse_status == 3)
            {
                temp_num = (temp_num*10) + *c - '0';
            }
            else
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            if(parse_status == 0 || parse_status == 1 || parse_status == 3)
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }

            if(parse_status == 2)
            {
                temp_num = (temp_num<<4) + *c - 'a' + 10;
            }
            else
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }
            break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            if(parse_status == 0 || parse_status == 1 || parse_status == 3)
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }

            if(parse_status == 2)
            {
                temp_num = (temp_num<<4) + *c - 'A' + 10;
            }
            else
            {
                *num = 0;
                return CA_OSAL_ERROR;
            }
            break;
        default:
            *num = 0;
            return CA_OSAL_ERROR;
        }
        c++;
    }

    *num = temp_num;
    return CA_OSAL_OK;
}

ca_int32_t ca_str2decl(ca_int8_t *str , ca_uint64_t *num)
{
    ca_uint64_t number = 0;
    ca_int8_t *c = str;
    ca_uint64_t base = 10;

    if(str == NULL || num == NULL)
        return CA_OSAL_ERROR;

    if(strlen((const char*)str) >= 2 && str[0] == '0' && str[1] == 'x')
    {
        if(strlen((const char*)str) == 2)
        {
            return CA_OSAL_ERROR;
        }
        c += 2;
        base = 16;
    }

    while(*c != '\0')
    {
        if(base == 10 && !(*c >= '0' &&  *c <= '9'))
            return CA_OSAL_ERROR;

        if(*c >= '0' &&  *c <= '9')
        {
            number = number*base + (unsigned) (long long)(*c - '0');
        }
        else if(*c >= 'A' &&  *c <= 'F')
        {
            number = number*base + (unsigned) (long long)(*c - 'A') + 10;
        }
        else if(*c >= 'a' &&  *c <= 'f')
        {
            number = number*base + (unsigned) (long long)(*c - 'a') + 10;
        }
        else
            return CA_OSAL_ERROR;

        c++;
    }

    *num = number;
    return CA_OSAL_OK;
}

ca_uint32_t ca_strtol(ca_int8_t *str)
{
    ca_uint32_t num = 0;
    ca_str2dec(str, &num);

    return num;
}

ca_uint32 ca_strtoul(const char *nptr, char **endptr, int base){
    return OS_STRTOUL(nptr, endptr, base);
}

ca_uint64_t ca_strtoll(ca_int8_t *str)
{
    ca_uint64_t num = 0;
    if(ca_str2decl(str, &num) == CA_OSAL_ERROR)
        return 0;

    return num;
}

ca_int32_t ca_str2mac(ca_int8_t *mac_str , ca_int8_t *mac_addr)
{
    ca_int8_t *c = NULL;
    ca_int32_t idx = 0;
    ca_int32_t parse_state = 0;
    ca_int32_t char_type = 0; /*0: decimal number   1: Hex Number,lower case 2: hex number, upper case  3:colon type 4:error type*/
    ca_int32_t high_bits = 0, low_bits = 0;

    if(mac_str == NULL || mac_addr == NULL)
        return CA_OSAL_ERROR;

    c = mac_str;
    while(*c != '\0')
    {
        if(*c <= '9' && *c >= '0')
            char_type = 0;
        else if(*c <= 'f' && *c >= 'a')
            char_type = 1;
        else if(*c <= 'F' && *c >= 'A')
            char_type = 2;
        else if(*c == ':'||*c == '.')
            char_type = 3;
        else
            char_type = 4;

        switch(char_type)
        {
            case 0:
            case 1:
            case 2:
                if(parse_state == 0 || parse_state == 3)
                {
                    high_bits = ca_char2int(*c);
                    parse_state = 1;
                }
                else if(parse_state == 1)
                {
                    low_bits = ca_char2int(*c);
                    parse_state = 2;
                }
                else
                    return CA_OSAL_ERROR;

                if(idx >= 6)
                    return CA_OSAL_ERROR;

                c++;
                if(*c == '\0')
                {
                    if(parse_state == 1)
                    {
                        mac_addr[idx++] = high_bits;
                    }

                    if(parse_state == 2)
                    {
                        mac_addr[idx++] = (high_bits<<4)+low_bits;
                    }
                }
                break;
            case 3:
                if(parse_state != 1 && parse_state != 2)
                    return CA_OSAL_ERROR;

                if(parse_state == 1)
                {
                    mac_addr[idx++] = high_bits;
                }

                if(parse_state == 2)
                {
                    mac_addr[idx++] = (high_bits<<4)+low_bits;
                }

                parse_state = 3;
                c++;
                break;
            default:
                return CA_OSAL_ERROR;
        }
    }

    if(idx != 6 || parse_state == 3)
        return CA_OSAL_ERROR;

    return CA_OSAL_OK;
}

ca_int32_t ca_str2ip(ca_int8_t *ip_str , ca_uint32_t *ipaddr)
{
    ca_int8_t *c = NULL;
    ca_int32_t parse_state = 0;
    ca_int32_t idx = 0;
    ca_int32_t ip[4]={0,0,0,0};

    if(ip_str == NULL || ipaddr == NULL)
        return CA_OSAL_ERROR;

    /******************************************************************
      parse_state DESC
      ------------------------------------------------------
      0 init status
      1 convert one byte, ignore the following digit
      2 previous char is '.'
      ------------------------------------------------------
     ******************************************************************/

    c = ip_str;
    while(*c != '\0')
    {/*The first char can not be 0*/
        if((*c == '0')&&(parse_state == 0))
            return CA_OSAL_ERROR;

        switch(*c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if(parse_state == 0 || parse_state == 2)
                {
                    if(idx >= 4)
                        return CA_OSAL_ERROR;
                    ip[idx++] = ca_atoi(c);
                    if(ip[idx-1] > 255 || ip[idx-1] < 0)
                    return CA_OSAL_ERROR;
                }
                c++;
                parse_state = 1;
                break;
            case '.':
                if(parse_state != 1)
                    return CA_OSAL_ERROR;

                c++;
                parse_state = 2;
                break;
            default:
                return CA_OSAL_ERROR;
        }
    }

    if(parse_state != 1)
        return CA_OSAL_ERROR;

    if(idx != 4)
        return CA_OSAL_ERROR;
    *ipaddr = (unsigned int)(((ip[0]<<24)&0xFF000000)|((ip[1]<<16)&0xFF0000)|((ip[2]<<8)&0xFF00)|((ip[3]<<0)&0xFF));

    return CA_OSAL_OK;
}

ca_int32_t ca_ip2str(ca_int8_t *ip_str , ca_uint32_t ipaddr)
{
    if(ip_str == NULL)
        return CA_OSAL_ERROR;

    sprintf((char *)ip_str , "%d.%d.%d.%d",(int)((ipaddr>>24)&0xFF),
            (int)((ipaddr>>16)&0xFF),
            (int)((ipaddr>>8)&0xFF),
            (int)((ipaddr>>0)&0xFF));
    return CA_OSAL_OK;
}


static int ipv6_include_ipv4(const char *src, unsigned char *dst)
{
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    unsigned char tmp[4], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            unsigned int new = *tp * 10 + (pch - digits);

            if (saw_digit && *tp == 0)
                return (0);
            if (new > 255)
                return (0);
            *tp = new;
            if (!saw_digit) {
                if (++octets > 4)
                    return (0);
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return (0);
            *++tp = 0;
            saw_digit = 0;
        } else
            return (0);
    }
    if (octets < 4)
        return (0);
    memcpy(dst, tmp, 4);
    return (1);
}

ca_int32 ca_str2ipv6(ca_int8 *ip_str, ca_uint8 * ipv6_addr)
{
    static const char xdigits_l[] = "0123456789abcdef",
                      xdigits_u[] = "0123456789ABCDEF";
    unsigned char tmp[16], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, seen_xdigits;
    unsigned int val;

    memset((tp = tmp), '\0', 16);
    endp = tp + 16;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*ip_str == ':' && *++ip_str != ':')
        return CA_OSAL_ERROR;

    curtok = ip_str;
    seen_xdigits = 0;
    val = 0;
    while ((ch = *ip_str++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);

        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (++seen_xdigits > 4)
                return CA_OSAL_ERROR;
            continue;
        }
        if (ch == ':') {
            curtok = ip_str;
            if (!seen_xdigits) {
                if (colonp)
                    return CA_OSAL_ERROR;
                colonp = tp;
                continue;
            } else if (*ip_str == '\0') {
                return CA_OSAL_ERROR;
            }
            if (tp + 2 > endp)
                return CA_OSAL_ERROR;
            *tp++ = (unsigned char) (val >> 8) & 0xff;
            *tp++ = (unsigned char) val & 0xff;
            seen_xdigits = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + 4) <= endp) &&
            ipv6_include_ipv4(curtok, tp) > 0) {
                tp += 4;
                seen_xdigits = 0;
                break;  /*%< '\\' was seen by inet_pton4(). */
        }
        return CA_OSAL_ERROR;
    }
    if (seen_xdigits) {
        if (tp + 2 > endp)
            return CA_OSAL_ERROR;
        *tp++ = (unsigned char) (val >> 8) & 0xff;
        *tp++ = (unsigned char) val & 0xff;
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        if (tp == endp)
            return CA_OSAL_ERROR;
        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return CA_OSAL_ERROR;
    memcpy(ipv6_addr, tmp, 16);
    return CA_OSAL_OK;
}

ca_int32 ca_ipv6tostr(ca_int8 *ip_str,ca_uint8 * ipv6_addr)
{
    int i;
    unsigned short var;
    char *addr_str_end_ptr;

    if(ip_str == NULL)
        return CA_OSAL_ERROR;

    ip_str[0] = '\0';
    addr_str_end_ptr = ip_str;
    for (i = 0; i < 16; i=i+2)
    {
        var = (ipv6_addr[i] << 8) + ipv6_addr[i+1];
        addr_str_end_ptr += sprintf(addr_str_end_ptr, "%X:", var);
    }
    *(addr_str_end_ptr - 1) = '\0';
    return CA_OSAL_OK;
}


ca_int32_t ca_str2hex(char *oui_str , char *oui_value , int maxbytes)
{
    char *c = NULL;
    int index = 0;
    int parse_state = 0;
    int char_type = 0; /*0: decimal number   1: Hex Number,lower case 2: hex number, upper case  3:colon type 4:error type*/
    int high_bits = 0, low_bits = 0;

    if(oui_str == NULL || oui_value == NULL)
        return CA_OSAL_ERROR;

    c = oui_str;
    while(*c != '\0')
    {
        if(*c <= '9' && *c >= '0')
            char_type = 0;
        else if(*c <= 'f' && *c >= 'a')
            char_type = 1;
        else if(*c <= 'F' && *c >= 'A')
            char_type = 2;
        else if(*c == ':'||*c == '.')
            char_type = 3;
        else
            char_type = 4;

        switch(char_type)
        {
        case 0:
        case 1:
        case 2:
            if(parse_state == 0 || parse_state == 3)
            {
                high_bits = ca_char2int(*c);
                parse_state = 1;
            }
            else if(parse_state == 1)
            {
                low_bits = ca_char2int(*c);
                parse_state = 2;
            }
            else
                return CA_OSAL_ERROR;

            if(index >= maxbytes)
                return CA_OSAL_ERROR;

            c++;
            if(*c == '\0')
            {
                if(parse_state == 1)
                {
                    oui_value[index++] = high_bits;
                }

                if(parse_state == 2)
                {
                    oui_value[index++] = (high_bits<<4)+low_bits;
                }
            }
            break;
        case 3:
            if(parse_state != 1 && parse_state != 2)
                return CA_OSAL_ERROR;

            if(parse_state == 1)
            {
                oui_value[index++] = high_bits;
            }

            if(parse_state == 2)
            {
                oui_value[index++] = (high_bits<<4)+low_bits;
            }

            parse_state = 3;
            c++;
            break;
        default:
            return CA_OSAL_ERROR;
        }
    }

    if(index != maxbytes)
        return CA_OSAL_ERROR;

    return CA_OSAL_OK;
}

ca_uint_t ca_rbuf_create
    (
    ca_int32_t rbuf_num
    )
{
    ca_ring_buf_t *pRing = NULL;

    if(rbuf_num <= 0)
    {
        return CA_INVALID_RING_BUF;
    }

    pRing = (ca_ring_buf_t *)ca_malloc(sizeof(ca_ring_buf_t));
    if(pRing == NULL)
    {
        return CA_INVALID_RING_BUF;
    }
    memset(pRing, 0, sizeof(ca_ring_buf_t));

    pRing->buf_num = rbuf_num;
    pRing->ptr = (ca_uint_t *)ca_malloc(sizeof(ca_uint_t)*pRing->buf_num);
    if(pRing->ptr == NULL)
    {
        ca_free(pRing);
        return CA_INVALID_RING_BUF;
    }
    memset(pRing->ptr, 0, sizeof(ca_uint32_t)*pRing->buf_num);
    return (ca_uint_t)pRing;
}

ca_int32_t ca_rbuf_read(ca_uint_t rbuf_id, void **buf)
{
    ca_ring_buf_t *pRing = NULL;

    if(rbuf_id == CA_INVALID_RING_BUF)
    {
        *buf = NULL;
        return CA_OSAL_ERROR;
    }

    pRing = (ca_ring_buf_t *)rbuf_id;
    if(pRing->rp == pRing->wp && !pRing->full)
    {
        *buf = NULL;
        return CA_OSAL_ERROR;
    }

    *buf = (void *)pRing->ptr[pRing->rp];
    pRing->ptr[pRing->rp] = 0;
    pRing->rp = (pRing->rp+1)%pRing->buf_num;
    if(pRing->rp == pRing->wp)
        pRing->full = 0;
    return CA_OSAL_OK;
}

ca_int32_t ca_rbuf_write(ca_uint_t rbuf_id, void *buf)
{
    ca_ring_buf_t *pRing = NULL;

    if(rbuf_id == CA_INVALID_RING_BUF)
    {
        return CA_OSAL_ERROR;
    }

    pRing = (ca_ring_buf_t *)rbuf_id;
    if(pRing->rp == pRing->wp && pRing->full)
    {
        return CA_OSAL_ERROR;
    }

    pRing->ptr[pRing->wp] = (ca_uint_t)buf;
    pRing->wp = (pRing->wp+1)%pRing->buf_num;
    if(pRing->wp == pRing->rp)
        pRing->full = 1;

    return CA_OSAL_OK;
}


void  ca_buffer_dump( char *tp, ca_uint8_t *dp, ca_uint16_t len)
{
    int  ii = 0;

    ca_printf("\r\n%s\n", tp);

    for (ii = 0; ii < len; ii++) {
        ca_printf("%02x ", dp[ii]);
        if (ii % 16 == 15) {
            ca_printf("\n");
        }
    }

    ca_printf("\n");
}

#ifdef __KERNEL__
EXPORT_SYMBOL(ca_buffer_dump);

#endif

