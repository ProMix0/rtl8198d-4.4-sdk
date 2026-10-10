#include "api_common.h"
#include "osal_common.h"
#include "aal_sa_id.h"

#include "tunnel.h"

ca_uint32_t ca_api_debug = 0x0;

/***************************************************************************
 * IP manipulation
 ***************************************************************************/

static ca_uint32_t ca_api_gen_ip_mask_v4(ca_uint8_t mask)
{
        ca_uint8_t i;
        ca_uint32_t mask_pattern=0;

        for (i = 0; i < 32; i++, mask--) {
                if (mask == 0)
                        break;
                mask_pattern |= 1 << (31 - i);
        }

        return mask_pattern;
}

ca_boolean_t ca_api_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2)
{
        ca_uint32_t ip1, ip2;
        ca_uint32_t  mask1, mask2;

        /* the input ip address should be in host order */

        if (ip_addr1->afi == CA_IPV6) {
                mask1 = ip_addr1->addr_len;
                mask2 = ip_addr2->addr_len;

                if (ca_api_debug & API_DBG_ROUTE) {
                        printk("%s: mask1=0x%x, mask2=0x%x\n", __func__, mask1, mask2);
                        printk("%s: ip_addr1=%pI6, ip_addr2=%pI6\n", __func__, &ip_addr1->ip_addr.ipv6_addr, ip_addr2->ip_addr.ipv6_addr);
                }
                if (mask1 == mask2) {

                        if (mask1 == 0)
                                return TRUE;

                        if (mask1 > 96) {        /* Just compare addr32[0~3] */
                                mask1 = ca_api_gen_ip_mask_v4(mask1 - 96);
                                if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                    (ip_addr1->ip_addr.ipv6_addr[1] == ip_addr2->ip_addr.ipv6_addr[1]) &&
                                    (ip_addr1->ip_addr.ipv6_addr[2] == ip_addr2->ip_addr.ipv6_addr[2]) &&
                                    ((ip_addr1->ip_addr.ipv6_addr[3] & mask1) == (ip_addr2->ip_addr.ipv6_addr[3] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 64) { /* Compare addr32[1~3] */
                               mask1 = ca_api_gen_ip_mask_v4(mask1 - 64);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                   (ip_addr1->ip_addr.ipv6_addr[1] == ip_addr2->ip_addr.ipv6_addr[1]) &&
                                   ((ip_addr1->ip_addr.ipv6_addr[2] & mask1) == (ip_addr2->ip_addr.ipv6_addr[2] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 32) { /* Compare addr32[2~3] */
                               mask1 = ca_api_gen_ip_mask_v4(mask1 - 32);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] == ip_addr2->ip_addr.ipv6_addr[0]) &&
                                   ((ip_addr1->ip_addr.ipv6_addr[1] & mask1) == (ip_addr2->ip_addr.ipv6_addr[1] & mask1)))
                                        return TRUE;
                        } else if (mask1 > 0) {  /* Compare addr32[3] */
                               mask1 = ca_api_gen_ip_mask_v4(mask1);
                               if ((ip_addr1->ip_addr.ipv6_addr[0] & mask1) == (ip_addr2->ip_addr.ipv6_addr[0] & mask1))
                                        return TRUE;
                        }
            }
        }
        else {
                ip1 = ip_addr1->ip_addr.ipv4_addr;
                ip2 = ip_addr2->ip_addr.ipv4_addr;
                mask1 = ca_api_gen_ip_mask_v4(ip_addr1->addr_len);
                mask2 = ca_api_gen_ip_mask_v4(ip_addr2->addr_len);

                if (ca_api_debug & API_DBG_ROUTE) {
                        printk("%s: ip1=%pI4, ip2=%pI4, mask1=0x%x, mask2=0x%x\n", __func__, &ip1, &ip2, mask1, mask2);
                }
                if ((mask1 == mask2) && ((ip1 & mask1) == (ip2 & mask2))) {
                        return TRUE;
                }
        }
        return FALSE;
}

void ca_api_ip_address_ntohl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src)
{
        p_dest->afi = p_src->afi;
        p_dest->addr_len = p_src->addr_len;

        if (p_src->afi == CA_IPV6) {
                p_dest->ip_addr.ipv6_addr[0] = ntohl(p_src->ip_addr.ipv6_addr[0]);
                p_dest->ip_addr.ipv6_addr[1] = ntohl(p_src->ip_addr.ipv6_addr[1]);
                p_dest->ip_addr.ipv6_addr[2] = ntohl(p_src->ip_addr.ipv6_addr[2]);
                p_dest->ip_addr.ipv6_addr[3] = ntohl(p_src->ip_addr.ipv6_addr[3]);
        } else {
                p_dest->ip_addr.ipv4_addr = ntohl(p_src->ip_addr.ipv4_addr);
        }
}

void ca_api_ip_address_htonl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src)
{
        p_dest->afi = p_src->afi;
        p_dest->addr_len = p_src->addr_len;

        if (p_src->afi == CA_IPV6) {
                p_dest->ip_addr.ipv6_addr[0] = htonl(p_src->ip_addr.ipv6_addr[0]);
                p_dest->ip_addr.ipv6_addr[1] = htonl(p_src->ip_addr.ipv6_addr[1]);
                p_dest->ip_addr.ipv6_addr[2] = htonl(p_src->ip_addr.ipv6_addr[2]);
                p_dest->ip_addr.ipv6_addr[3] = htonl(p_src->ip_addr.ipv6_addr[3]);
        } else {
                p_dest->ip_addr.ipv4_addr = htonl(p_src->ip_addr.ipv4_addr);
        }
}

/***************************************************************************
 * SA ID management
 ***************************************************************************/

/* NOTE:
 * Crypto Engine contains 3 cores. Each core contains 8 contexts for encrpytion / decryption.
 *   Core#0, SA ID #0 ~ #7, reserved for Taroko (Packet Engine), outbound (encryption).
 *   Core#1, SA ID #8 ~ #15, reserved for Taroko (Packet Engine), inbound (decryption).
 *   Core#2: SA ID #16 ~ #23, reserved for Cortex A53 (CPUs that run Linux).
 *
 *   SA ID #24 ~ SA_ID_ENTRY_MAX, plaintext.
 */
#define CA_SA_ID_OUTBOUND_MIN	0
#define CA_SA_ID_OUTBOUND_MAX	7
#define CA_SA_ID_INBOUND_MIN	8
#define CA_SA_ID_INBOUND_MAX	15
#define CA_SA_ID_PLAINTEXT_MIN	SA_ID_PLAINTEXT_MIN
#define CA_SA_ID_PLAINTEXT_MAX	SA_ID_PLAINTEXT_MAX
#define CA_SA_ID_INVALID	SA_ID_INVALID

ca_status_t ca_api_sa_id_alloc(ca_tunnel_type_t tunnel_type, ca_tunnel_direction_t dir, bool is_encrypted, uint32_t *sa_id)
{
	aal_sa_id_entry_t sa_id_entry;
	uint32_t rslt_idx = CA_SA_ID_INVALID;
	int ret;
	int i, start, end;

	if (sa_id == NULL)
		return CA_E_PARAM;

	memset(&sa_id_entry, 0, sizeof(sa_id_entry));
	sa_id_entry.valid = 1;
	sa_id_entry.direction = (dir == CA_TUNNEL_DIR_OUTBOUND) ? CA_TUNNEL_DIR_OUTBOUND : CA_TUNNEL_DIR_INBOUND;
	sa_id_entry.is_encrypted = is_encrypted ? 1 : 0;
	sa_id_entry.tunnel_type = tunnel_type;

	if (is_encrypted) {
		if (dir == CA_TUNNEL_DIR_OUTBOUND) {
			start = CA_SA_ID_OUTBOUND_MIN;
			end = CA_SA_ID_OUTBOUND_MAX;
		} else {
			start = CA_SA_ID_INBOUND_MIN;
			end = CA_SA_ID_INBOUND_MAX;
		}
	} else {
		start = CA_SA_ID_PLAINTEXT_MIN;
		end = CA_SA_ID_PLAINTEXT_MAX;
	}

	/* search available SA ID to use */
	for (i = start; i <= end; i++) {
		ret = aal_entry_add_by_idx(AAL_TABLE_SA_ID, &sa_id_entry, i);
		if (ret == AAL_E_OK) {
			rslt_idx = i;
			break;
		} else if (ret == AAL_E_ENTRY_USED) {
			continue;
		} else {
			/* what's wrong? */
			break;
		}
	}

	if (rslt_idx == CA_SA_ID_INVALID) {
		return CA_E_RESOURCE;
	} else {
		*sa_id = rslt_idx;
		return CA_E_OK;
	}
}

ca_status_t ca_api_sa_id_free(uint32_t sa_id)
{
	int ret;

	if ((ret = aal_entry_del(AAL_TABLE_SA_ID, sa_id)) != AAL_E_OK) {
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

