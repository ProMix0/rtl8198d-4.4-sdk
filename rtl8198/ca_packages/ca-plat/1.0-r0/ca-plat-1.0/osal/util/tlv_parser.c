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

DEFINITIONS:  "DEVICE" means the Cortina Systems?LynxD SDK product.
"You" or "CUSTOMER" means the entity or individual that uses the SOFTWARE.
"SOFTWARE" means the Cortina Systems?SDK software.

GRANT OF LICENSE:  Subject to the restrictions below, CORTINA hereby grants
CUSTOMER a non-exclusive, non-assignable, non-transferable, royalty-ca_free,
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
#include "os_core.h"
#include "osal_common.h"
#include "tlv_parser.h"

unsigned char tlv_get_char(char *pbuf)
{
    return (unsigned char)pbuf[0];
}

void tlv_set_char(char *pbuf , unsigned char c)
{
    pbuf[0] = c;
    return;
}

unsigned short tlv_get_short(char *pbuf)
{
    unsigned short value = 0;

    memcpy(&value , pbuf , sizeof(unsigned short));
    return value;
}

void tlv_set_short(char *pbuf , unsigned short v)
{
    memcpy(pbuf , (char *)&v , sizeof(unsigned short));
    return ;
}

unsigned int tlv_get_int(char *pbuf)
{
    unsigned int value = 0;

    memcpy(&value , pbuf , sizeof(unsigned int));
    return value;
}

void tlv_set_int(char *pbuf , unsigned int v)
{
    memcpy(pbuf , (char *)&v , sizeof(unsigned int));
    return ;
}

unsigned short tlv_crc_calc(char *pbuf , int len)
{
    unsigned int crc_num = 0;
    int i;
    unsigned short num;
    unsigned short crc = 0;

    for(i = 0 ; i < len/2 ; i++)
    {
        num = tlv_get_short(pbuf+i*2);
        num = ca_ntohs(num);
        crc_num += num;
    }

    if(len%2 == 1)
    {
        crc_num += ((pbuf[len-1]<<8)&0xFF00);
    }

    while((crc_num&0xFFFF0000))
    {
        crc_num = (crc_num>>16)+(crc_num&0xFFFF);
    }

    crc = (unsigned short)crc_num;
    crc = ~crc;

    return (crc);
}

int tlv_crc_check(tlv_header_t *pcfg)
{
    unsigned short crc = 0;
    unsigned short crc_temp = 0;
    char *pbuf = (char *)pcfg;

    crc_temp = ca_ntohs(tlv_get_short(pbuf+CFG_CRC_OFFSET));
    pcfg->cfg_crc = 0;
    crc = tlv_crc_calc(pbuf,
        ca_ntohl(tlv_get_int(pbuf+CFG_LEN_OFFSET))+sizeof(tlv_header_t));
    if(crc_temp != crc)
    {
        tlv_set_short(pbuf+CFG_CRC_OFFSET , ca_htons(crc_temp));
        return TLV_PARSER_ERROR;
    }
    tlv_set_short(pbuf+CFG_CRC_OFFSET , ca_htons(crc_temp));

    return TLV_PARSER_OK;
}

int tlv_field_check(tlv_header_t *pcfg)
{
    tlv_t *pTlv = NULL;
    unsigned int check_len = 0;
    int field_num = 0;
    unsigned short tlv_len = 0;
    unsigned int cfg_len = 0;

    cfg_len = tlv_get_int((char*)pcfg+CFG_LEN_OFFSET);
    pTlv = (tlv_t *)(pcfg+1);
    while(check_len < ca_ntohl(cfg_len))
    {
        tlv_len = tlv_get_short((char *)pTlv + TLV_LEN_OFFSET);
        if(ca_ntohs(tlv_len) == 0)
        {
            startup_cfg_printf("\r\n The max len of field(%d) is 0.",pTlv->type);
            return TLV_PARSER_ERROR;
        }
        check_len += ca_ntohs(tlv_len) + TLV_VALUE_OFFSET;
        pTlv = (tlv_t *)((char *)(pcfg+1) + check_len);
        field_num++;
    }

    if(check_len != ca_ntohl(cfg_len))
    {
        startup_cfg_printf("\r\n Cfg len doesn't mach.");
        return TLV_PARSER_ERROR;
    }

    return field_num;
}

int tlv_check(void *base_address)
{
    tlv_header_t *cfg_header = (tlv_header_t *)base_address;
    int field_num = 0;
    unsigned short crc = 0;

    /*     Magic number check    */
    if(ca_ntohl(tlv_get_int((char*)base_address+CFG_MAGIC_OFFSET)) != TLV_HEADER_MAGIC)
    {
        startup_cfg_printf("\r\n Startup-cfg magic number is error(0x%08x).",
            ca_ntohl(tlv_get_int((char*)base_address+CFG_MAGIC_OFFSET)));
        return TLV_PARSER_ERROR;
    }

    /*    CRC check    */
    if(ca_ntohs(tlv_get_short((char*)base_address+CFG_CRC_OFFSET)) != 0)
    {
        if(TLV_PARSER_ERROR == tlv_crc_check(cfg_header))
        {
            startup_cfg_printf("\r\n Startup-cfg CRC doesn't match.");
            return TLV_PARSER_ERROR;
        }
    }
    else
    {
        crc = tlv_crc_calc((char *)cfg_header,
            ca_ntohl(tlv_get_int((char*)base_address+CFG_LEN_OFFSET))+sizeof(tlv_header_t));
        tlv_set_short((char*)base_address+CFG_CRC_OFFSET , ca_htons(crc));
    }

    /*    Field number check    */
    if((field_num = tlv_field_check(cfg_header)) == TLV_PARSER_ERROR)
    {
        startup_cfg_printf("\r\n Startup-cfg field check failed.");
        return TLV_PARSER_ERROR;
    }

    return field_num;
}

int tlv_load
    (
    unsigned long instance_id
    )
{
    tlv_t *pTlv = NULL;
    unsigned int check_len = 0;
    tlv_header_t *pcfg = NULL;
    int i=0;
    int field_num = 0;
    unsigned int cfg_len = 0 , magic = 0;
    unsigned short tlv_len = 0;
    tlv_instance_t *pInst = NULL;
    unsigned short crc = 0;

    pInst = (tlv_instance_t *)instance_id;
    if(pInst == NULL)
    {
        startup_cfg_printf("\r\n The base address of startup configuration is NULL.");
        return TLV_PARSER_ERROR;
    }

    pcfg = (tlv_header_t *)pInst->base_address;
    magic = tlv_get_int((char*)pcfg+CFG_MAGIC_OFFSET);
    if(magic != ca_htonl(TLV_HEADER_MAGIC))
    {
        memset(pcfg , 0 , sizeof(tlv_header_t));
        tlv_set_int((char *)pcfg+CFG_MAGIC_OFFSET, ca_htonl(TLV_HEADER_MAGIC));
        tlv_set_short((char *)pcfg+CFG_CRC_OFFSET, ca_htons(0));
        tlv_set_int((char *)pcfg+CFG_LEN_OFFSET, ca_htonl(0));
        tlv_set_int((char *)pcfg+CFG_VERSION_OFFSET, ca_htonl(0));
        crc = tlv_crc_calc((char *)pcfg, sizeof(tlv_header_t));
        tlv_set_short((char *)pcfg+CFG_CRC_OFFSET, ca_htons(crc));
    }

    if(pInst->max_tlv_len < ca_ntohl(pcfg->cfg_len) + sizeof(tlv_header_t)){
        return TLV_PARSER_ERROR;
    }

    if((field_num = tlv_check(pInst->base_address)) == TLV_PARSER_ERROR)
        return TLV_PARSER_ERROR;

    if(pInst->tlv_ptr_list != NULL)
    {
        ca_free(pInst->tlv_ptr_list);
        pInst->tlv_ptr_list = NULL;
        pInst->tlv_num = 0;
        /*startup_cfg_printf("\r\n Reload startup configuration.");*/
    }

    if(field_num == 0)
        return TLV_PARSER_OK;

    pInst->tlv_ptr_list = ( void **)ca_malloc(sizeof(void *)*field_num);
    if(pInst->tlv_ptr_list == NULL)
        return TLV_PARSER_ERROR;
    memset(pInst->tlv_ptr_list , 0 , (sizeof(void *)*field_num));

    cfg_len = tlv_get_int((char*)pcfg+CFG_LEN_OFFSET);
    pTlv = (tlv_t *)((char *)pInst->base_address+sizeof(tlv_header_t));
    while(check_len < ca_ntohl(cfg_len))
    {
        pInst->tlv_ptr_list[i++] = (void *)pTlv;
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        check_len += ca_ntohs(tlv_len) + TLV_VALUE_OFFSET;
        pTlv = (tlv_t *)((char *)pInst->base_address+sizeof(tlv_header_t)+check_len);
    }
    pInst->tlv_num= i;

    return TLV_PARSER_OK;
}

unsigned long tlv_inst_init
    (
    void          *base_address,
    unsigned int max_cfg_len,
    int            (*store)(void *base_addr , int len)
    )
{
    tlv_instance_t *pInst = NULL;

    if(base_address == NULL)
        return TLV_INVALID_INST_ID;

    pInst = (tlv_instance_t *)ca_malloc(sizeof(tlv_instance_t));
    if(pInst == NULL)
        return TLV_INVALID_INST_ID;

    memset(pInst , 0 , sizeof(tlv_instance_t));
    pInst->base_address = (void *)base_address;
    pInst->tlv_ptr_list = NULL;
    pInst->max_tlv_len    = max_cfg_len;
    pInst->store        = store;

    return (unsigned long)pInst;
}

unsigned long tlv_inst_duplicate
    (
    tlv_instance_t *src
    )
{
    tlv_instance_t *pInst = NULL;
    void **tlv_ptr_list = NULL;
    int i;
    int total_len = 0;
    tlv_header_t *tlv_header = NULL;
    char *base_addr = NULL;

    if(src == NULL)
        return TLV_INVALID_INST_ID;

    if(src->tlv_num == 0)
        return TLV_INVALID_INST_ID;

    tlv_header = (tlv_header_t *)src->base_address;
    total_len = sizeof(tlv_header_t) + ca_ntohl(tlv_header->cfg_len);
    tlv_ptr_list = (void **)ca_malloc(sizeof(void *)*src->tlv_num);
    if(tlv_ptr_list == NULL)
        return TLV_INVALID_INST_ID;

    base_addr = (char *)ca_malloc(total_len);
    if(base_addr == NULL)
    {
        ca_free(tlv_ptr_list);
        return TLV_INVALID_INST_ID;
    }

    pInst = (tlv_instance_t *)ca_malloc(sizeof(tlv_instance_t));
    if(pInst == NULL)
    {
        ca_free(tlv_ptr_list);
        ca_free(base_addr);
        return TLV_INVALID_INST_ID;
    }

    memset(pInst , 0 , sizeof(tlv_instance_t));
    memcpy(base_addr , src->base_address , total_len);
    pInst->base_address = (void *)base_addr;
    pInst->tlv_ptr_list = tlv_ptr_list;
    pInst->max_tlv_len  = src->max_tlv_len;
    pInst->store        = NULL;
    pInst->tlv_num      = src->tlv_num;
    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pInst->tlv_ptr_list[i] = (void *) ((unsigned long)pInst->base_address +((unsigned long)src->tlv_ptr_list[i]- (unsigned long)src->base_address));
    }

    return (unsigned long)pInst;
}


int tlv_read_char
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned char *value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    if(pInst->tlv_ptr_list == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    if(value == NULL)
    {
        startup_cfg_printf("\r\n Input an invalid parameter.");
        return TLV_PARSER_ERROR;
    }

    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) > sizeof(char))
        {
            startup_cfg_printf("\r\n Attempt to read a cfg which length is %d into a char(type = %d).",ca_ntohs(pTlv->len), type);
            return TLV_PARSER_ERROR;
        }

        *value = tlv_get_char((char *)pTlv+TLV_VALUE_OFFSET);
        return TLV_PARSER_OK;
    }

    return TLV_PARSER_ERROR;
}

int tlv_read_short
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned short *value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    if(pInst->tlv_ptr_list == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    if(value == NULL)
    {
        startup_cfg_printf("\r\n Input an invalid parameter.");
        return TLV_PARSER_ERROR;
    }

    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != sizeof(short))
        {
            startup_cfg_printf("\r\n Attempt to read a cfg which length is %d into a short(type = %d )",ca_ntohs(pTlv->len),type);
            return TLV_PARSER_ERROR;
        }

        *value = ca_ntohs(tlv_get_short((char *)pTlv+TLV_VALUE_OFFSET));
        return TLV_PARSER_OK;
    }

    return TLV_PARSER_ERROR;
}

int tlv_read_int
    (
    unsigned long instance_id,
    unsigned short type ,
    unsigned int *value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    if(pInst->tlv_ptr_list == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    if(value == NULL)
    {
        startup_cfg_printf("\r\n Input an invalid parameter.");
        return TLV_PARSER_ERROR;
    }

    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != sizeof(int))
        {
            startup_cfg_printf("\r\n Attempt to read a cfg which length is %d into a int(type = %d ).",ca_ntohs(pTlv->len), type);
            return TLV_PARSER_ERROR;
        }

        *value = ca_ntohl(tlv_get_int((char *)pTlv+TLV_VALUE_OFFSET));
        return TLV_PARSER_OK;
    }

    return TLV_PARSER_ERROR;
}

int tlv_read_buffer
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned int len,
    unsigned char *value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return 0;

    if(pInst->tlv_ptr_list == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return 0;
    }

    if(value == NULL || len == 0)
    {
        startup_cfg_printf("\r\n Input an invalid parameter.");
        return 0;
    }

    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) > len)
        {
            startup_cfg_printf("\r\n The length of buffer is too short to store the value (type = %d).",type);
            return 0;
        }

        memcpy(value , pTlv->data , ca_ntohs(tlv_len));
        tlv_len = ca_ntohs(tlv_len);
        return (int)tlv_len;
    }

    return 0;
}

int tlv_write_char
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned char value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short crc = 0;
    tlv_header_t *p = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    unsigned int cfg_len=0,new_cfg_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = tlv_get_int((char*)p+CFG_LEN_OFFSET);
    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != sizeof(char))
        {
            startup_cfg_printf("\r\n This type tlv should be %d bytes length.",ca_ntohs(pTlv->len));
            return TLV_PARSER_ERROR;
        }

        tlv_set_char((char *)pTlv+TLV_VALUE_OFFSET , value);

        tlv_set_short((char *)p+CFG_CRC_OFFSET, 0);
        crc = tlv_crc_calc((char *)p, ca_ntohl(cfg_len)+sizeof(tlv_header_t));
        tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));
        new_cfg_len = ca_ntohl(cfg_len);
        goto write_done;
    }

    new_cfg_len = ca_ntohl(cfg_len)+TLV_VALUE_OFFSET+sizeof(char);
    if(new_cfg_len > pInst->max_tlv_len)
        return TLV_PARSER_ERROR;

    pTlv = (tlv_t *)((char *)pInst->base_address+ca_ntohl(cfg_len)+sizeof(tlv_header_t));
    tlv_len = (unsigned short)sizeof(char);
    tlv_set_short((char *)pTlv+TLV_TYPE_OFFSET , ca_htons(type));
    tlv_set_short((char *)pTlv+TLV_LEN_OFFSET , ca_htons(tlv_len));
    tlv_set_char((char *)pTlv+TLV_VALUE_OFFSET , value);
    tlv_set_int((char *)p+CFG_LEN_OFFSET, ca_htonl(new_cfg_len));
    tlv_set_short((char *)p+CFG_CRC_OFFSET, ca_htons(0));
    tlv_load(instance_id);

write_done:
    if(pInst->store)
        pInst->store(pInst->base_address , new_cfg_len + sizeof(tlv_header_t));
    return TLV_PARSER_OK;
}

int tlv_write_short
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned short value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short crc = 0;
    tlv_header_t *p = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    unsigned int cfg_len=0,new_cfg_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = tlv_get_int((char*)p+CFG_LEN_OFFSET);
    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != sizeof(short))
        {
            startup_cfg_printf("\r\n This type tlv should be %d bytes length.",ca_ntohs(pTlv->len));
            return TLV_PARSER_ERROR;
        }

        tlv_set_short((char *)pTlv+TLV_VALUE_OFFSET , ca_htons(value));

        tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
        crc = tlv_crc_calc((char *)p, ca_ntohl(cfg_len)+sizeof(tlv_header_t));
        tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));
        new_cfg_len = ca_ntohl(cfg_len);
        goto write_done;
    }

    new_cfg_len = ca_ntohl(cfg_len)+TLV_VALUE_OFFSET+sizeof(short);
    if(new_cfg_len > pInst->max_tlv_len)
        return TLV_PARSER_ERROR;

    pTlv = (tlv_t *)((char *)pInst->base_address+ca_ntohl(cfg_len)+sizeof(tlv_header_t));
    tlv_len = (unsigned short)sizeof(short);
    tlv_set_short((char *)pTlv+TLV_TYPE_OFFSET , ca_htons(type));
    tlv_set_short((char *)pTlv+TLV_LEN_OFFSET , ca_htons(tlv_len));
    tlv_set_short((char *)pTlv+TLV_VALUE_OFFSET , ca_htons(value));
    tlv_set_int((char *)p+CFG_LEN_OFFSET, ca_htonl(new_cfg_len));
    tlv_set_short((char *)p+CFG_CRC_OFFSET, ca_htons(0));
    tlv_load(instance_id);

write_done:
    if(pInst->store)
        pInst->store(pInst->base_address , new_cfg_len + sizeof(tlv_header_t));

    return TLV_PARSER_OK;
}

int tlv_write_int
    (
    unsigned long instance_id,
    unsigned short type,
    unsigned int value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short crc = 0;
    tlv_header_t *p = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    unsigned int cfg_len=0,new_cfg_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = tlv_get_int((char*)p+CFG_LEN_OFFSET);
    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != sizeof(int))
        {
            startup_cfg_printf("\r\n This type tlv should be %d bytes length.",ca_ntohs(pTlv->len));
            return TLV_PARSER_ERROR;
        }

        tlv_set_int((char *)pTlv+TLV_VALUE_OFFSET , ca_htonl(value));

        tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
        crc = tlv_crc_calc((char *)p, ca_ntohl(cfg_len)+sizeof(tlv_header_t));
        tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));
        new_cfg_len = ca_ntohl(cfg_len);
        goto write_done;
    }

    new_cfg_len = ca_ntohl(cfg_len)+TLV_VALUE_OFFSET+sizeof(int);
    if(new_cfg_len > pInst->max_tlv_len)
        return TLV_PARSER_ERROR;

    pTlv = (tlv_t *)((char *)pInst->base_address+ca_ntohl(cfg_len)+sizeof(tlv_header_t));
    tlv_len = (unsigned short)sizeof(int);
    tlv_set_short((char *)pTlv+TLV_TYPE_OFFSET , ca_htons(type));
    tlv_set_short((char *)pTlv+TLV_LEN_OFFSET , ca_htons(tlv_len));
    tlv_set_int((char *)pTlv+TLV_VALUE_OFFSET , ca_htonl(value));
    tlv_set_int((char *)p+CFG_LEN_OFFSET, ca_htonl(new_cfg_len));
    tlv_set_short((char *)p+CFG_CRC_OFFSET, ca_htons(0));
    tlv_load(instance_id);

write_done:
    if(pInst->store)
        pInst->store(pInst->base_address , new_cfg_len + sizeof(tlv_header_t));

    return TLV_PARSER_OK;
}

int tlv_write_buffer
    (
    unsigned long instance_id,
    unsigned short type ,
    int max_len ,
    int len ,
    unsigned char *value
    )
{
    int i;
    tlv_t *pTlv = NULL;
    unsigned short crc = 0;
    tlv_header_t *p = NULL;
    unsigned short tlv_type = 0;
    unsigned short tlv_len = 0;
    unsigned int cfg_len=0,new_cfg_len = 0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL || max_len < len){
        startup_cfg_printf("parameter wrong \n");
        return TLV_PARSER_ERROR;
    }

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    if(value == NULL || len == 0)
    {
        startup_cfg_printf("\r\n Input an invalid parameter.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = tlv_get_int((char*)p+CFG_LEN_OFFSET);
    for(i = 0 ; i < pInst->tlv_num ; i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        if(type != ca_ntohs(tlv_type))
            continue;

        if(ca_ntohs(tlv_len) != max_len)
        {
            unsigned short move_len = 0;
            char *mv_addr = NULL;
            char *dst_addr = NULL;
            char *src_addr = NULL;

            mv_addr = (char *)pTlv+(ca_ntohs(tlv_len)+TLV_VALUE_OFFSET);
            dst_addr = (char *)pTlv+(ca_ntohs(tlv_len)+TLV_VALUE_OFFSET)+max_len-ca_ntohs(tlv_len);
            move_len = ca_ntohl(cfg_len)-((unsigned long)pTlv -
                        (unsigned long)((char *)pInst->base_address + sizeof(tlv_header_t)) +
                        (ca_ntohs(tlv_len)+TLV_VALUE_OFFSET));

            new_cfg_len = ca_ntohl(cfg_len)+max_len-ca_ntohs(tlv_len);
            tlv_len = max_len;
            tlv_len = ca_htons(tlv_len);
            if(move_len == 0)
            {
                tlv_set_short((char *)pTlv+TLV_LEN_OFFSET, tlv_len);
                tlv_set_short((char *)p+CFG_LEN_OFFSET , ca_htonl(new_cfg_len));
                goto MAX_LEN_CHANGED;
            }
            src_addr = (char *)ca_malloc(move_len);
            if(src_addr == NULL)
                return TLV_PARSER_ERROR;

            memset(src_addr, 0, move_len);
            memcpy(src_addr, mv_addr, move_len);
            memcpy(dst_addr, src_addr, move_len);
            ca_free(src_addr);
            tlv_set_short((char *)pTlv+TLV_LEN_OFFSET, tlv_len);
            tlv_set_short((char *)p+CFG_LEN_OFFSET , ca_htonl(new_cfg_len));
            goto MAX_LEN_CHANGED;
        }
        else
        {
            if(ca_ntohs(tlv_len) < len)
            {
                startup_cfg_printf("\r\n The max length of this field should be less than %d.",ca_ntohs(pTlv->len));
                return TLV_PARSER_ERROR;
            }

            new_cfg_len = ca_ntohl(cfg_len);
        MAX_LEN_CHANGED:
            memset(pTlv->data , 0 , ca_ntohs(tlv_len));
            memcpy(pTlv->data , value , len);

            tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
            crc = tlv_crc_calc((char *)p, ca_ntohl(cfg_len)+sizeof(tlv_header_t));
            tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));
            goto write_done;
        }
    }

    new_cfg_len = ca_ntohl(cfg_len)+TLV_VALUE_OFFSET+max_len;
    if(new_cfg_len > pInst->max_tlv_len){
        startup_cfg_printf("new cfg len %d > max len %d \n",new_cfg_len,pInst->max_tlv_len);
        return TLV_PARSER_ERROR;
    }

    pTlv = (tlv_t *)((char *)pInst->base_address+ca_ntohl(cfg_len)+sizeof(tlv_header_t));
    tlv_len = (unsigned short)max_len;
    tlv_set_short((char *)pTlv+TLV_TYPE_OFFSET , ca_htons(type));
    tlv_set_short((char *)pTlv+TLV_LEN_OFFSET , ca_htons(tlv_len));
    memset((char *)pTlv + TLV_VALUE_OFFSET , 0 , max_len);
    memcpy((char *)pTlv + TLV_VALUE_OFFSET , value , len);
    tlv_set_int((char *)p+CFG_LEN_OFFSET, ca_htonl(new_cfg_len));
    tlv_set_short((char *)p+CFG_CRC_OFFSET, ca_htons(0));
    tlv_load(instance_id);

write_done:
    if(pInst->store)
        pInst->store(pInst->base_address , new_cfg_len + sizeof(tlv_header_t));

    return TLV_PARSER_OK;
}

int tlv_write_flag
    (
    unsigned long instance_id,
    unsigned char value
    )
{
    tlv_header_t *p = NULL;
    unsigned int cfg_len=0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;
    unsigned short crc = 0;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = ca_ntohl(tlv_get_int((char*)p+CFG_LEN_OFFSET));
    tlv_set_char((char *)p+CFG_FLAG_OFFSET, value);
    tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
    crc = tlv_crc_calc((char *)p, cfg_len+sizeof(tlv_header_t));
    tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));

    if(pInst->store)
        pInst->store(pInst->base_address , cfg_len + sizeof(tlv_header_t));

    return TLV_PARSER_OK;
}

int tlv_write_active
    (
    unsigned long instance_id,
    unsigned char value
    )
{
    tlv_header_t *p = NULL;
    unsigned int cfg_len=0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;
    unsigned short crc = 0;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = ca_ntohl(tlv_get_int((char*)p+CFG_LEN_OFFSET));
    tlv_set_char((char *)p+CFG_ACTIVE_OFFSET, value);
    tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
    crc = tlv_crc_calc((char *)p, cfg_len+sizeof(tlv_header_t));
    tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));

    return TLV_PARSER_OK;
}

int tlv_write_version
    (
    unsigned long instance_id,
    unsigned int value
    )
{
    tlv_header_t *p = NULL;
    unsigned int cfg_len=0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;
    unsigned short crc = 0;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    cfg_len = ca_ntohl(tlv_get_int((char*)p+CFG_LEN_OFFSET));
    tlv_set_int((char *)p+CFG_VERSION_OFFSET, ca_htonl(value));
    tlv_set_short((char *)p+CFG_CRC_OFFSET , 0);
    crc = tlv_crc_calc((char *)p, cfg_len+sizeof(tlv_header_t));
    tlv_set_short((char *)p+CFG_CRC_OFFSET , ca_htons(crc));

    if(pInst->store)
        pInst->store(pInst->base_address , cfg_len + sizeof(tlv_header_t));

    return TLV_PARSER_OK;
}


int tlv_read_flag
    (
    unsigned long instance_id,
    unsigned char *value
    )
{
    tlv_header_t *p = NULL;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return TLV_PARSER_ERROR;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
    {
        startup_cfg_printf("\r\n Maybe startup-cfg hasn't been loaded.");
        return TLV_PARSER_ERROR;
    }

    *value = tlv_get_char((char *)p+CFG_FLAG_OFFSET);

    return TLV_PARSER_OK;
}

#if 0
char *tlv_get_string(char *pbuf, int len)
{
    char str[256];
    int i;
    int total_display = 0;
    char *p = &str[0];
    int total_len=0;

    memset(str , 0 , sizeof(str));
    total_display = (len<=30)?len:30;
    for(i = 0 ; i < total_display ; i++)
    {
        sprintf(str +total_len, "%02x" , (unsigned char)pbuf[i]);
        if((i+1)%4 == 0)
        {
            sprintf(str + strlen(str) , " ");
        }
        total_len = strlen(str);
    }

    return p;
}

void tlv_dump
    (
    unsigned int instance_id
    )
{
    int i;
    tlv_header_t *p = NULL;
    tlv_t *pTlv = NULL;
    unsigned int magic=0,cfg_len=0,version=0;
    unsigned int cfg_crc=0,tlv_type=0,tlv_len=0;
    tlv_instance_t *pInst = (tlv_instance_t *)instance_id;

    if(pInst == NULL)
        return;

    p = (tlv_header_t *)pInst->base_address;
    if(p == NULL)
        return;

    cfg_len = tlv_get_int((char *)p + CFG_LEN_OFFSET);
    magic = tlv_get_int((char *)p + CFG_MAGIC_OFFSET);
    cfg_crc = tlv_get_short((char *)p + CFG_CRC_OFFSET);
    version = tlv_get_int((char *)p + CFG_VERSION_OFFSET);
    ca_printf("\r\n==============================================================================");
    ca_printf("\r\nCfg Length   : %d",ca_ntohl(cfg_len));
    ca_printf("\r\nMagic Number : 0x%08x",ca_ntohl(magic));
    ca_printf("\r\nCRC          : %d",ca_ntohs(cfg_crc));
    ca_printf("\r\nVersion      : 0x%08x",ca_ntohl(version));
    ca_printf("\r\nDetails:");
    ca_printf("\r\n------------------------------------------------------------------------------");
    ca_printf("\r\n%-4s %-6s %s","type","len","value(HEX)");
    for(i = 0 ; i < pInst->tlv_num;i++)
    {
        pTlv = (tlv_t *)pInst->tlv_ptr_list[i];
        if(pTlv == NULL)
            continue;

        tlv_len = tlv_get_short((char *)pTlv+TLV_LEN_OFFSET);
        tlv_type = tlv_get_short((char *)pTlv+TLV_TYPE_OFFSET);
        ca_printf("\r\n%-4d %-6d %s",ca_ntohs(tlv_type),
            ca_ntohs(tlv_len),tlv_get_string(pTlv->data, ca_ntohs(tlv_len)));
    }
    ca_printf("\r\n");

    return;
}
#endif

