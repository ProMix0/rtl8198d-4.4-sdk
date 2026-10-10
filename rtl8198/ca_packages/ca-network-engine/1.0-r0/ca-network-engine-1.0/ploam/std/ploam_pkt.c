
#include "osal_common.h"

#include "ploam_cmn.h"
#include "ploam_adapt.h"
#include "ploam_ngpon2.h"
#include "ploam_gpon.h"
#include "ploam_pkt.h"
#include "aal_ploam.h"

/*
 * debug level set
 */
static ploam_dump_pkt_type_e g_ploam_dump_pkt_type = 0;//PLOAM_DUMP_TYPE_ALL;
static ploam_dump_pkt_type_e g_aes_ploam_dump_pkt_type = 0;//PLOAM_DUMP_TYPE_ALL;

void ploam_buf_print(ca_uint8_t *pbuf , ca_int32_t len)
{
    ca_int32_t i , j = 0 , k;
    ca_uint8_t strbuf[16];
    ca_int32_t remain_len = 0;

    ca_printf("\r\n==============================================================================");

    for (i = 0 ; i < len ; i++) {
        if (i % 16 == 0) {
            j = 0;
            ca_printf("\r\n%p: ", pbuf + i);
            memset(strbuf , 0 , sizeof(strbuf));
            remain_len = len - i;
        }

        strbuf[i % 16] =  *((ca_uint8_t *)(pbuf + i));
        ca_printf("%02x ", *((ca_uint8_t *)(pbuf + i)));

        if (++j == 8)
            ca_printf(" ");

        if (remain_len == j && (remain_len % 16) != 0) {
            if (j < 8) ca_printf(" ");

            for ( ; j < 16 ; j++) {
                ca_printf("   ");
            }
        }

        if (j == 16) {
            remain_len = 0;
            ca_printf("; ");

            for (k = 0 ; k < 16 ; k++) {
                if (strbuf[k] >= ' ' && strbuf[k] <= '~')
                    ca_printf("%c", strbuf[k]);
                else
                    ca_printf(".");
            }
        }
    }

    ca_printf("\r\n\n");
    return;
}

void ploam_dump_octet(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len)
{
    return ploam_dump_pkt(comment, buffer, len);
}

void ploam_dump_pkt(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len)
{

    PLOAM_ASSERT(comment != NULL && buffer != NULL);

    if (g_ploam_dump_pkt_type == PLOAM_DUMP_TYPE_NONE) {
        return;
    }

    ca_printf("\n\n%s:pkt len = %d",
               comment, len);
    ploam_buf_print(buffer, len);
    ca_printf("\n");
}

void ploam_set_pkt_dump(ca_uint32_t type)
{
    g_ploam_dump_pkt_type = type;
    ca_printf("ploam dump level:%d\r\n", g_ploam_dump_pkt_type);
}

void aes_ploam_dump_pkt(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len)
{

    PLOAM_ASSERT(comment != NULL && buffer != NULL);

    if (g_aes_ploam_dump_pkt_type == PLOAM_DUMP_TYPE_NONE) {
        return;
    }

    ca_printf("\n\n%s:pkt len = %d",
               comment, len);
    ploam_buf_print(buffer, len);
    ca_printf("\n");
}

void aes_ploam_set_pkt_dump(ca_uint32_t type)
{
    g_aes_ploam_dump_pkt_type = type;
    ca_printf("aes ploam dump level:%d\r\n", g_aes_ploam_dump_pkt_type);
}

EXPORT_SYMBOL(ploam_buf_print);
EXPORT_SYMBOL(ploam_dump_octet);
EXPORT_SYMBOL(ploam_dump_pkt);
EXPORT_SYMBOL(ploam_set_pkt_dump);
EXPORT_SYMBOL(aes_ploam_set_pkt_dump);

