#include <net/xfrm.h>
#include <crypto/aead.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/spinlock.h>
#include <linux/if_ether.h>
#include <net/if_inet6.h>
#include <net/addrconf.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <soc/cortina/ca_kernel_hook.h>
#include <linux/list.h>

#include "aal_table.h"
#include "aal_idx4pe.h"
#include "ca_aal.h"
#include "ca_kernel_hook_tunnel.h"
#include "ca_kernel_hook_util.h"
#include "pkt_tx.h"
#include "ca_ni_common.h"
#include "ca_mut.h"

static LIST_HEAD(tunnel_list);
static DEFINE_RWLOCK(tunnel_list_lock);

struct ipsec_node {
	/* info from Cortina API */
	ca_tunnel_id_t tunnel_id;
	ca_intf_id_t intf_id;
	ca_l3_nexthop_id_t nexthop_id;
	ca_ipsec_sa_id_t sa_id_encrypt;
	ca_ipsec_sa_id_t sa_id_decrypt;

	/* info from kernel */
	uint32_t spi_encrypt;
	uint32_t spi_decrypt;
	struct xfrm_state *xfrm_state_in;
	struct xfrm_state *xfrm_state_out;
	u32 xfrm_policy_index_in;
	u32 xfrm_policy_index_out;

	struct list_head list;
};

extern ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];

static void dump_xfrm_state(struct xfrm_state *x)
{
	int i;

	if (!x) return;

	printk(KERN_INFO "id: daddr=%08x-%08x-%08x-%08x, spi=0x%08x, proto=0x%02x\n",
			ntohl(x->id.daddr.a6[0]), ntohl(x->id.daddr.a6[1]), ntohl(x->id.daddr.a6[2]), ntohl(x->id.daddr.a6[3]),
			htonl(x->id.spi), x->id.proto);
	printk(KERN_INFO "sel: daddr=%08x-%08x-%08x-%08x/%u, dport=%u, dport_mask=%u\n",
			ntohl(x->sel.daddr.a6[0]), ntohl(x->sel.daddr.a6[1]), ntohl(x->sel.daddr.a6[2]), ntohl(x->sel.daddr.a6[3]),
			x->sel.prefixlen_d, ntohs(x->sel.dport), ntohs(x->sel.dport_mask));
	printk(KERN_INFO "     saddr=%08x-%08x-%08x-%08x/%u, sport=%u, sport_mask=%u\n",
			ntohl(x->sel.saddr.a6[0]), ntohl(x->sel.saddr.a6[1]), ntohl(x->sel.saddr.a6[2]), ntohl(x->sel.saddr.a6[3]),
			x->sel.prefixlen_s, ntohs(x->sel.sport), ntohs(x->sel.sport_mask));
	printk(KERN_INFO "     family=%u, proto=%u, ifindex=%d\n",
			x->sel.family, x->sel.proto, x->sel.ifindex);
	printk(KERN_INFO "mark.v=0x%08x, mark.m=0x%08x\n", x->mark.v, x->mark.m);
	printk(KERN_INFO "tfcpad=0x%08x, genid=0x%08x\n", x->tfcpad, x->genid);
	printk(KERN_INFO "props: reqid=%u, mode=%u, replay_window=%u, aalgo=%u, ealgo=%u, calgo=%u\n",
			x->props.reqid, x->props.mode, x->props.replay_window, x->props.aalgo, x->props.ealgo, x->props.calgo);
	printk(KERN_INFO "       flags=0x%02x, family=%u, header_len=%u, trailer_len=%u, extra_flags=0x%08x\n",
			x->props.flags, x->props.family, x->props.header_len, x->props.trailer_len, x->props.flags);
	printk(KERN_INFO "       saddr=%08x-%08x-%08x-%08x\n",
			ntohl(x->props.saddr.a6[0]), ntohl(x->props.saddr.a6[1]), ntohl(x->props.saddr.a6[2]), ntohl(x->props.saddr.a6[3]));

	if (x->aalg != NULL) {
		printk(KERN_INFO "aalg: name=(%s), key_len=%u, trunc_len=%u, key=",
				x->aalg->alg_name, x->aalg->alg_key_len, x->aalg->alg_trunc_len);
		for (i = 0; i < x->aalg->alg_key_len / 8; i++) {
			printk("%02x ", x->aalg->alg_key[i]);
		}
		printk("\n");
	}

	if (x->ealg != NULL) {
		printk(KERN_INFO "ealg: name=(%s), key_len=%u, key=", x->ealg->alg_name, x->ealg->alg_key_len);
		for (i = 0; i < x->ealg->alg_key_len / 8; i++) {
			printk("%02x ", x->ealg->alg_key[i]);
		}
		printk("\n");
	}

	if (x->calg != NULL) {
		printk(KERN_INFO "calg: name=(%s), key_len=%u, key=", x->calg->alg_name, x->calg->alg_key_len);
		for (i = 0; i < x->calg->alg_key_len / 8; i++) {
			printk("%02x ", x->calg->alg_key[i]);
		}
		printk("\n");
	}

	if (x->aead != NULL) {
		printk(KERN_INFO "aead: name=(%s), key_len=%u, icv_len=%u, key=",
				x->aead->alg_name, x->aead->alg_key_len, x->aead->alg_icv_len);
		for (i = 0; i < x->aead->alg_key_len / 8; i++) {
			printk("%02x ", x->aead->alg_key[i]);
		}
		printk("\n");
	}

	if (x->encap != NULL) {
		printk(KERN_INFO "encap: encap_type=0x%04x, encap_sport=%u, encap_dport=%u, encap_oa=%08x-%08x-%08x-%08x\n",
				x->encap->encap_type, ntohs(x->encap->encap_sport), ntohs(x->encap->encap_dport),
				ntohl(x->encap->encap_oa.a6[0]), ntohl(x->encap->encap_oa.a6[1]),
				ntohl(x->encap->encap_oa.a6[2]), ntohl(x->encap->encap_oa.a6[3]));
	}

	if (x->coaddr != NULL) {
		printk(KERN_INFO "coaddr=%08x-%08x-%08x-%08x\n",
				ntohl(x->coaddr->a6[0]), ntohl(x->coaddr->a6[1]), ntohl(x->coaddr->a6[2]), ntohl(x->coaddr->a6[3]));
	}

	printk(KERN_INFO "replay: oseq=%u, seq=%u, bitmap=0x%08x\n", x->replay.oseq, x->replay.seq, x->replay.bitmap);

	if (x->replay_esn != NULL) {
		printk(KERN_INFO "replay_esn: oseq=%u, seq=%u, oseq_hi=%u, seq_hi=%u, replay_window=%u, bmp_len=%u\n",
				x->replay_esn->oseq, x->replay_esn->seq, x->replay_esn->oseq_hi, x->replay_esn->seq_hi,
				x->replay_esn->replay_window, x->replay_esn->bmp_len);
	}

	printk(KERN_INFO "xflags=0x%08x, replay_maxage=%u, replay_maxdiff=%u\n", x->xflags, x->replay_maxage, x->replay_maxdiff);
	printk(KERN_INFO "stats: replay_window=%u, replay=%u, integrity_failed=%u\n",
			x->stats.replay_window, x->stats.replay, x->stats.integrity_failed);
	printk(KERN_INFO "curlft.bytes=%llu, curlft.packets=%llu\n", x->curlft.bytes, x->curlft.packets);

	if (x->type != NULL)
		printk(KERN_INFO "type.proto=%u, type.flags=0x%02x\n", x->type->proto, x->type->flags);

	if (x->inner_mode != NULL) {
		printk(KERN_INFO "inner_mode: encap=%u, flags=0x%08x\n", x->inner_mode->encap, x->inner_mode->flags);

		if (x->inner_mode->afinfo != NULL)
			printk(KERN_INFO "inner_mode.afinfo: family=%u, proto=%u, eth_proto=0x%04x\n",
					x->inner_mode->afinfo->family, x->inner_mode->afinfo->proto, ntohs(x->inner_mode->afinfo->eth_proto));
	}

	if (x->outer_mode != NULL) {
		printk(KERN_INFO "outer_mode: encap=%u, flags=0x%08x\n", x->outer_mode->encap, x->outer_mode->flags);

		if (x->outer_mode->afinfo != NULL)
			printk(KERN_INFO "outer_mode.afinfo: family=%u, proto=%u, eth_proto=0x%04x\n",
					x->outer_mode->afinfo->family, x->outer_mode->afinfo->proto, ntohs(x->outer_mode->afinfo->eth_proto));
	}

	if (x->security != NULL) {
		printk(KERN_INFO "security: ctx_doi=%u, ctx_alg=%u, ctx_len=%u, ctx_sid=%u\n",
				x->security->ctx_doi, x->security->ctx_alg, x->security->ctx_len, x->security->ctx_sid);
	}
}

static void dump_xfrm_policy(struct xfrm_policy *xp)
{
	int i;

	if (!xp) return;

	printk(KERN_INFO "index=%u, type=%u, action=%u, flags=0x%02x, family=0x%04x, priority=%u, xfrm_nr=%u\n",
			xp->index, xp->type, xp->action, xp->flags, xp->family, xp->priority, xp->xfrm_nr);

	printk(KERN_INFO "selector: daddr=%08x-%08x-%08x-%08x/%u, dport=%u, dport_mask=%u\n",
			ntohl(xp->selector.daddr.a6[0]), ntohl(xp->selector.daddr.a6[1]),
			ntohl(xp->selector.daddr.a6[2]), ntohl(xp->selector.daddr.a6[3]),
			xp->selector.prefixlen_d, ntohs(xp->selector.dport), ntohs(xp->selector.dport_mask));
	printk(KERN_INFO "          saddr=%08x-%08x-%08x-%08x/%u, sport=%u, sport_mask=%u\n",
			ntohl(xp->selector.saddr.a6[0]), ntohl(xp->selector.saddr.a6[1]),
			ntohl(xp->selector.saddr.a6[2]), ntohl(xp->selector.saddr.a6[3]),
			xp->selector.prefixlen_s, ntohs(xp->selector.sport), ntohs(xp->selector.sport_mask));
	printk(KERN_INFO "          family=%u, proto=%u, ifindex=%d\n", xp->selector.family, xp->selector.proto, xp->selector.ifindex);

	for (i = 0; i < xp->xfrm_nr; i++) {
		printk(KERN_INFO "vec[%d]:\n", i);
		printk(KERN_INFO "  id: daddr=%08x-%08x-%08x-%08x, spi=0x%08x, proto=0x%02x\n",
				ntohl(xp->xfrm_vec[i].id.daddr.a6[0]), ntohl(xp->xfrm_vec[i].id.daddr.a6[1]),
				ntohl(xp->xfrm_vec[i].id.daddr.a6[2]), ntohl(xp->xfrm_vec[i].id.daddr.a6[3]),
				htonl(xp->xfrm_vec[i].id.spi), xp->xfrm_vec[i].id.proto);
		printk(KERN_INFO "  saddr=%08x-%08x-%08x-%08x\n",
				ntohl(xp->xfrm_vec[i].saddr.a6[0]), ntohl(xp->xfrm_vec[i].saddr.a6[1]),
				ntohl(xp->xfrm_vec[i].saddr.a6[2]), ntohl(xp->xfrm_vec[i].saddr.a6[3]));
		printk(KERN_INFO "  encap_family=%u, reqid=%u, mode=%u, share=%u\n",
				xp->xfrm_vec[i].encap_family, xp->xfrm_vec[i].reqid, xp->xfrm_vec[i].mode, xp->xfrm_vec[i].share);
	}
}

static ca_status_t get_ipsec_ealg(unsigned char *alg_name, ca_uint32_t *ealg)
{
	unsigned char alg_names[10][64] = {
		"ecb(cipher_null)", "cipher_null", "cbc(des)", "des", "cbc(des3_ede)",
		"des3_ede", "aes", "cbc(aes)", "rfc4106(gcm(aes))", "rfc4309(ccm(aes))"
	};
	int alg_nums[10] = {
		CA_IPSEC_CIPHER_NULL, CA_IPSEC_CIPHER_NULL, CA_IPSEC_CIPHER_DES, CA_IPSEC_CIPHER_DES, CA_IPSEC_CIPHER_3DES,
		CA_IPSEC_CIPHER_3DES, CA_IPSEC_CIPHER_AES, CA_IPSEC_CIPHER_AES, CA_IPSEC_CIPHER_AES, CA_IPSEC_CIPHER_AES
	};
	ca_uint8_t i;

	if (ealg == NULL)
		return CA_E_PARAM;

	for (i = 0; i < sizeof(alg_names) / sizeof(alg_names[0]); i++) {
		if (strcmp(alg_name, alg_names[i]) == 0) {
			*ealg = alg_nums[i];
			return CA_E_OK;
		}
	}

	printk(KERN_INFO "%s: ERROR: Unknown IPSec encryption algorithm: %s\n", __func__, alg_name);
	*ealg = CA_IPSEC_CIPHER_NULL;

	return CA_E_NOT_SUPPORT;
}

static ca_status_t get_dev_by_ipv4_addr(uint32_t ipv4_addr, struct net_device **dev)
{
	struct net_device *tmp_dev;
	struct in_device *in_dev;
	struct in_ifaddr *ifa;

	if (dev == NULL)
		return CA_E_PARAM;

	for_each_netdev(&init_net, tmp_dev) {
		in_dev = __in_dev_get_rtnl(tmp_dev);
		ifa = in_dev->ifa_list;

		for (; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_local == ipv4_addr) {
				*dev = tmp_dev;
				return CA_E_OK;
			}
		}
	}

	return CA_E_NOT_FOUND;
}

static ca_status_t get_dev_by_ipv6_addr(struct in6_addr *ipv6_addr, struct net_device **dev)
{
	struct net_device *tmp_dev;
	struct inet6_dev *in6_dev;
	struct inet6_ifaddr *ifa6;

	if ((ipv6_addr == NULL) || (dev == NULL))
		return CA_E_PARAM;

	for_each_netdev(&init_net, tmp_dev) {
		in6_dev = __in6_dev_get(tmp_dev);

		list_for_each_entry(ifa6, &in6_dev->addr_list, if_list) {
			if (memcmp(&ifa6->addr, ipv6_addr, sizeof(struct in6_addr)) == 0) {
				*dev = tmp_dev;
				return CA_TRUE;
			}
		}
	}

	return CA_FALSE;
}

static ca_status_t get_ipsec_ealg_mode(u8 ealgo, ca_uint32_t *ealg_mode)
{
	if (ealg_mode == NULL)
		return CA_E_PARAM;

	switch (ealgo) {
		case SADB_EALG_NULL:
			*ealg_mode = CA_IPSEC_CIPHER_MODE_ECB;
			return CA_E_OK;
		case SADB_EALG_DESCBC:
		case SADB_EALG_3DESCBC:
		case SADB_X_EALG_AESCBC:
			*ealg_mode = CA_IPSEC_CIPHER_MODE_CBC;
			return CA_E_OK;
		case SADB_X_EALG_AESCTR:
			*ealg_mode = CA_IPSEC_CIPHER_MODE_CTR;
			return CA_E_OK;
		case SADB_X_EALG_AES_CCM_ICV8:
		case SADB_X_EALG_AES_CCM_ICV12:
		case SADB_X_EALG_AES_CCM_ICV16:
			*ealg_mode = CA_IPSEC_CIPHER_MODE_CCM;
			return CA_E_OK;
		case SADB_X_EALG_AES_GCM_ICV8:
		case SADB_X_EALG_AES_GCM_ICV12:
		case SADB_X_EALG_AES_GCM_ICV16:
			*ealg_mode = CA_IPSEC_CIPHER_MODE_GCM;
			return CA_E_OK;
		default:
			printk("%s: ERROR: Unknown IPSec encryption algorithm mode: %d\n", __func__, ealgo);
			*ealg_mode = CA_IPSEC_CIPHER_MODE_ECB;
			return CA_E_NOT_SUPPORT;
	}
}

static ca_status_t get_ipsec_aalg(unsigned char *alg_name, ca_uint32_t *aalg)
{
	unsigned char alg_names[6][64] = {
		"md5", "sha1", "sha256", "sha384", "sha512", "digest_null"};
	unsigned char alg_hmac_names[6][64] = {
		"hmac(md5)", "hmac(sha1)", "hmac(sha256)", "hmac(sha384)", "hmac(sha512)", "hmac(digest_null)"};
	ca_uint8_t alg_nums[6] = {
		CA_IPSEC_HASH_MD5, CA_IPSEC_HASH_SHA1, CA_IPSEC_HASH_SHA256, CA_IPSEC_HASH_SHA384, CA_IPSEC_HASH_SHA512, CA_IPSEC_HASH_NULL
	};
	ca_uint8_t i;

	if (aalg == NULL)
		return CA_E_PARAM;

	for (i = 0; i < sizeof(alg_names) / sizeof(alg_names[0]); i++) {
		if (strcmp(alg_name, alg_names[i]) == 0) {
			*aalg = alg_nums[i];
			return CA_E_OK;
		}
		if (strcmp(alg_name, alg_hmac_names[i]) == 0) {
			*aalg = alg_nums[i];
			return CA_E_OK;
		}
	}

	printk("%s:ERROR! Unknown IPSec authentication algorithm: %s\n", __func__, alg_name);
	*aalg = CA_IPSEC_HASH_NULL;

	return CA_E_NOT_SUPPORT;
}

static ca_status_t construct_ipsec_sa(struct xfrm_state *x, struct xfrm_policy *xp, ca_tunnel_direction_t dir, ca_ipsec_sa_t *sa)
{
	ca_status_t ret;
	struct crypto_aead *aead;
	struct xfrm_algo_desc *aalg_desc;

	if (!x || !xp || !sa)
		return CA_E_PARAM;

	memset(sa, 0, sizeof(*sa));
	sa->replay_window = x->props.replay_window; // TBD: multiple 8?
	sa->spi = htonl(x->id.spi);
	sa->sequence_number = x->replay.oseq + 1; /* the sequence number is for the next packet */
	memcpy(sa->ekey, x->ealg->alg_key, (x->ealg->alg_key_len + 8 - 1) / 8);
	memcpy(sa->akey, x->aalg->alg_key, (x->aalg->alg_key_len + 8 - 1) / 8);
	sa->ip_version = (x->props.family == AF_INET) ? CA_IPV4 : CA_IPV6;
	sa->protocol = (x->id.proto == IPPROTO_ESP) ? 0 : 1;
	sa->tunnel = (x->props.mode == XFRM_MODE_TRANSPORT) ? 0 : 1;

	aead = x->data;
	sa->iv_len = crypto_aead_ivsize(aead) >> 2;

	if (x->aead) {
		if ((ret = get_ipsec_ealg(x->aead->alg_name, &sa->ealg)) != CA_E_OK)
			return ret;

		if ((ret = get_ipsec_ealg_mode(x->props.ealgo, &sa->ealg_mode)) != CA_E_OK)
			return ret;

		sa->encryption_keylen = (x->aead->alg_key_len + 7) >> 5;
		memcpy(sa->ekey, x->aead->alg_key, (sa->encryption_keylen << 2));

		/* in AEAD mode, there is no authentication algorithm */
		sa->aalg = CA_IPSEC_HASH_NULL;
		memset(sa->akey, 0, MAX_AUTH_KEY_LEN);
		sa->auth_keylen = 0;

		sa->icv_trunclen = crypto_aead_authsize(aead);
		// sa->icv_trunclen = x->aead->alg_icv_len; // TBD: maybe this is correct
	} else if (x->ealg) {
		if ((ret = get_ipsec_ealg(x->ealg->alg_name, &sa->ealg)) != CA_E_OK)
			return ret;

		if ((ret = get_ipsec_ealg_mode(x->props.ealgo, &sa->ealg_mode)) != CA_E_OK)
			return ret;

		sa->encryption_keylen = (x->ealg->alg_key_len + 7) >> 5;
		memcpy(sa->ekey, x->ealg->alg_key, sa->encryption_keylen << 2);

		/* get IV length */
		if (x->aalg) {
			if ((ret = get_ipsec_aalg(x->aalg->alg_name, &sa->aalg)) != CA_E_OK)
				return ret;

			sa->auth_keylen = (x->aalg->alg_key_len + 7) >> 5;
			memcpy(sa->akey, x->aalg->alg_key, sa->auth_keylen << 2);

			/* get ICV (integrity check value) truncated length */
			aalg_desc = xfrm_aalg_get_byname(x->aalg->alg_name, 0);
			if (aalg_desc == NULL) {
				printk(KERN_INFO "%s: ERROR! Can't get aalg_desc by aalg name = %s\n", __func__, x->aalg->alg_name);
				return CA_E_PARAM;
			}

			sa->icv_trunclen = x->aalg->alg_trunc_len >> 5;
		} else {
			sa->aalg = CA_IPSEC_HASH_NULL;
			memset(sa->akey, 0x00, MAX_AUTH_KEY_LEN);
			sa->auth_keylen = 0;
			sa->icv_trunclen = 0;
		}

	} else {
		printk(KERN_INFO "%s: ERROR! No encryption algorithm assigned.\n", __func__);
		return CA_E_PARAM;
	}

	if (x->encap) {
		if (x->encap->encap_type == UDP_ENCAP_ESPINUDP) {
			sa->is_natt = 1;
			sa->src_l4_port = ntohs(x->encap->encap_sport);
			sa->dest_l4_port = ntohs(x->encap->encap_dport);
		} else {
			/* other encap_types seem not IPSec NAT-T (ESP in UDP) */
			sa->is_natt = 0;
			sa->src_l4_port = 0;
			sa->dest_l4_port = 0;
		}
	} else {
		sa->is_natt = 0;
		sa->src_l4_port = 0;
		sa->dest_l4_port = 0;
	}

	sa->etherIP = 0;
	sa->sa_dir = dir;

	return CA_E_OK;
}

ca_status_t ca_kh_ipsec_add(ca_device_id_t device_id, struct xfrm_state *x_in, struct xfrm_policy *xp_in, struct xfrm_state *x_out, struct xfrm_policy *xp_out)
{
	bool rekey = false;
	ca_status_t ret;
	ca_ipsec_sa_id_t sa_id_encrypt, sa_id_decrypt;
	ca_ipsec_sa_t ipsec_sa_encrypt, ipsec_sa_decrypt;
	ca_tunnel_id_t tunnel_id;
	ca_tunnel_cfg_t tcfg;
	struct net_device *parent_dev = NULL;
	ca_l3_intf_t l3_intf, tmp_l3_intf, *parent_l3_intf = NULL;

	struct ipsec_node *iter, *tmp_iter, *ipsec_node = NULL;
	ca_kh_tunnel_entry_t kh_tunnel;

	ca_l3_nexthop_t nexthop;
	ca_l3_route_t route;

	if ((!x_in) || (!xp_in) || (!x_out) || (!xp_out))
		return CA_E_PARAM;

	if ((x_in->id.proto != x_out->id.proto) || ((x_in->id.proto != IPPROTO_ESP) && (x_in->id.proto != IPPROTO_AH)) )
		return CA_E_PARAM;

	if ( (x_in->props.family != x_out->props.family) || ((x_in->props.family != AF_INET) && (x_in->props.family != AF_INET6)) )
		return CA_E_PARAM;

	if ( (x_in->props.mode != x_out->props.mode) || ((x_in->props.mode != XFRM_MODE_TRANSPORT) && (x_in->props.mode != XFRM_MODE_TUNNEL)) )
		return CA_E_PARAM;

	if (x_in->ealg->alg_key_len > MAX_ENCRYPTION_KEY_LEN * 8) {
		printk(KERN_INFO "%s: ERROR! EALG key_len (%u bits) for decryption exceeds HW limitation (%d bits).\n",
				__func__, x_in->ealg->alg_key_len, MAX_ENCRYPTION_KEY_LEN * 8);
		return CA_E_NOT_SUPPORT;
	}

	if (x_in->aalg->alg_key_len > MAX_AUTH_KEY_LEN * 8) {
		printk(KERN_INFO "%s: ERROR! AALG key_len (%u bits) for decryption exceeds HW limitation (%d bits).\n",
				__func__, x_in->aalg->alg_key_len, MAX_AUTH_KEY_LEN * 8);
		return CA_E_NOT_SUPPORT;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Dump ingress xfrm_state:\n", __func__);
		dump_xfrm_state(x_in);
		printk(KERN_INFO "%s: Dump ingress xfrm_policy:\n", __func__);
		dump_xfrm_policy(xp_in);
		printk(KERN_INFO "%s: Dump egress xfrm_state:\n", __func__);
		dump_xfrm_state(x_out);
		printk(KERN_INFO "%s: Dump egress xfrm_policy:\n", __func__);
		dump_xfrm_policy(xp_out);
	}

	/* check if there is same xfrm_policy index recorded */
	write_lock(&tunnel_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &tunnel_list, list) {
		/* NOTE:
		 *   Application would overwrite the existing xfrm_state and xfrm_policy with same index.
		 *   If the SPI of new and old ones are different, it is re-key action.
		 *   If the SPI of new and old ones are same, no idea what condition it is because the keys are also same;
		 *   maybe it is caused by poor cable link quality. Therefore, code will return directly when this case occurs.
		 */
		if ((xp_in->index == iter->xfrm_policy_index_in) && (xp_out->index == iter->xfrm_policy_index_out)) {
			if ((iter->spi_decrypt == htonl(x_in->id.spi)) && (iter->spi_encrypt == htonl(x_out->id.spi))) {
				write_unlock(&tunnel_list_lock);
				if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
					printk(KERN_INFO "%s: Found same added SPI pairs, skip this adding.\n", __func__);
				}
				return CA_E_EXISTS;
			}

			/* delete the tunnel in iter */
			if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
				printk(KERN_INFO "%s: Found same added xfrm_policy index. Delete it first. (in->index %u, out->index %u)\n",
					__func__, xp_in->index, xp_out->index);
			}
			list_del(&iter->list);
			rekey = true;
			break;
		}
	}
	write_unlock(&tunnel_list_lock);

	ret = construct_ipsec_sa(x_in, xp_in, CA_IPSEC_SPD_DECRYPT, &ipsec_sa_decrypt);
	if (ret != CA_E_OK)
		return ret;

	ret = construct_ipsec_sa(x_out, xp_out, CA_IPSEC_SPD_ENCRYPT, &ipsec_sa_encrypt);
	if (ret != CA_E_OK)
		return ret;

	/*** Add SA ***/

	ret = ca_ipsec_sa_add(device_id, &ipsec_sa_decrypt, &sa_id_decrypt);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to add IPSec SA for decryption. (ret %d)\n", __func__, ret);
		return ret;
	}

	ret = ca_ipsec_sa_add(device_id, &ipsec_sa_encrypt, &sa_id_encrypt);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to add IPSec SA for encryption. (ret %d)\n", __func__, ret);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return ret;
	}

	/*** Add tunnel ***/

	memset(&tcfg, 0, sizeof(tcfg));
	tcfg.type = CA_TUNNEL_IPSEC;\
	if (x_in->props.family == AF_INET) {
		/* IPv4 */
		tcfg.src_addr.afi = CA_IPV4;
		tcfg.src_addr.ip_addr.ipv4_addr = ntohl(x_out->props.saddr.a4);
		tcfg.src_addr.addr_len = 32;

		tcfg.dest_addr.afi = CA_IPV4;
		tcfg.dest_addr.ip_addr.ipv4_addr = ntohl(x_out->id.daddr.a4);
		tcfg.dest_addr.addr_len = 32;

		ret = get_dev_by_ipv4_addr(x_out->props.saddr.a4, &parent_dev);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: ERROR! Fail to get parent ifindex.\n", __func__);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return CA_E_PARAM;
		}
	} else {
		/* IPv6 */
		tcfg.src_addr.afi = CA_IPV6;
		tcfg.src_addr.ip_addr.ipv6_addr[0] = ntohl(x_out->props.saddr.a6[0]);
		tcfg.src_addr.ip_addr.ipv6_addr[1] = ntohl(x_out->props.saddr.a6[1]);
		tcfg.src_addr.ip_addr.ipv6_addr[2] = ntohl(x_out->props.saddr.a6[2]);
		tcfg.src_addr.ip_addr.ipv6_addr[3] = ntohl(x_out->props.saddr.a6[3]);
		tcfg.src_addr.addr_len = 128;

		tcfg.dest_addr.afi = CA_IPV6;
		tcfg.dest_addr.ip_addr.ipv6_addr[0] = ntohl(x_out->id.daddr.a6[0]);
		tcfg.dest_addr.ip_addr.ipv6_addr[1] = ntohl(x_out->id.daddr.a6[1]);
		tcfg.dest_addr.ip_addr.ipv6_addr[2] = ntohl(x_out->id.daddr.a6[2]);
		tcfg.dest_addr.ip_addr.ipv6_addr[3] = ntohl(x_out->id.daddr.a6[3]);
		tcfg.dest_addr.addr_len = 128;

		ret = get_dev_by_ipv6_addr((struct in6_addr *) &x_out->props.saddr, &parent_dev);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: ERROR! Fail to get parent ifindex.\n", __func__);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return CA_E_PARAM;
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Found parent interface: name=%s, ifindex=%d\n", __func__, parent_dev->name, parent_dev->ifindex);
	}

	tcfg.tunnel.ipsec.sa_id_encrypt = sa_id_encrypt;
	tcfg.tunnel.ipsec.sa_id_decrypt = sa_id_decrypt;

	parent_l3_intf = kzalloc(sizeof(*parent_l3_intf), GFP_KERNEL);
	if (parent_l3_intf == NULL) {
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return CA_E_MEMORY;
	}

	if (parent_dev->flags & IFF_LOOPBACK)
		parent_l3_intf->type = CA_L3_INTF_CPU;
	else if (parent_dev->flags & IFF_POINTOPOINT)
		parent_l3_intf->type = CA_L3_INTF_TUNNEL;
	else
		parent_l3_intf->type = CA_L3_INTF_BCAST;

	ret = ca_kh_l3_get_intf_id_by_ifindex(device_id, tcfg.src_addr.afi, parent_dev->ifindex, parent_l3_intf->type, &parent_l3_intf->intf_id);
	if (ret == CA_E_NOT_FOUND) {
		printk("%s: ERROR! Can't find parent intf_id. (ret %d, ifindex %d)\n", __func__, ret, parent_dev->ifindex);
		kfree(parent_l3_intf);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return CA_E_PARAM;
	}
	tcfg.parent_l3_intf_id = parent_l3_intf->intf_id;

	ret = ca_tunnel_add(device_id, &tcfg, &tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to add IPSec tunnel. (ret %d)\n", __func__, ret);
		kfree(parent_l3_intf);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return ret;
	}

	/* add tunnel interface */

	ret = ca_l3_intf_get(device_id, parent_l3_intf);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to get parent interface of intf_id %u. (ret %d)\n", __func__, parent_l3_intf->intf_id, ret);
		kfree(parent_l3_intf);
		ca_tunnel_delete(device_id, tunnel_id);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return ret;
	}

	memset(&l3_intf, 0, sizeof(l3_intf));
	l3_intf.type = CA_L3_INTF_TUNNEL;
	l3_intf.port_id = parent_l3_intf->port_id;
	l3_intf.tunnel_id = tunnel_id;
	l3_intf.outer_vid = parent_l3_intf->outer_vid;
	l3_intf.ip_addr = tcfg.src_addr;
	memcpy(&l3_intf.mac_addr, &parent_l3_intf->mac_addr, sizeof(ca_mac_addr_t));

	l3_intf.mask.port_id = 1;
	l3_intf.mask.tunnel_id = 1;
	l3_intf.mask.outer_vid = parent_l3_intf->mask.outer_vid;
	l3_intf.mask.ip_addr = 1;
	l3_intf.mask.mac_addr = 1;

	ca_kh_decide_nat_enable(&l3_intf);

	kfree(parent_l3_intf); // no more need parent_l3_intf

	ret = ca_l3_intf_add(device_id, &l3_intf);
	if (ret != CA_E_OK) {
		printk("%s: ERROR! ca_l3_intf_add() fails! (ret %d)\n", __func__, ret);
		ca_tunnel_delete(device_id, tunnel_id);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk("%s: IPSec interface is added. (intf.intf_id %d)\n", __func__, l3_intf.intf_id);
	}

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, parent_dev->ifindex, l3_intf.intf_id, CA_TUNNEL_IPSEC);
	if (ret != CA_E_OK) {
		printk("%s: ERROR! ca_kh_l3_tunnel_set_intf_id() fails! (ret %d)\n", __func__, ret);
		ca_l3_intf_delete(device_id, l3_intf.intf_id);
		ca_tunnel_delete(device_id, tunnel_id);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return ret;
	}

	/* create nexthop for remote address (in policy) */

	if (rekey) {

		/* add new nexthop for new intf_id */

		ret = ca_l3_nexthop_get(device_id, iter->nexthop_id, &nexthop);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to get NextHop. (ret %d, nexthop_id %u)\n", __func__, ret, iter->nexthop_id);
			ca_l3_intf_delete(device_id, l3_intf.intf_id);
			ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
			ca_tunnel_delete(device_id, tunnel_id);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			kfree(iter); // TBD
			return ret;
		}

		nexthop.intf_id = l3_intf.intf_id;

		ret = ca_l3_nexthop_add(device_id, &nexthop, &nexthop.nexthop_id);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to create NextHop for IPSec peer. (ret %d)\n", __func__, ret);
			ca_l3_intf_delete(device_id, l3_intf.intf_id);
			ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
			ca_tunnel_delete(device_id, tunnel_id);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return ret;
		}

		/* create route for remote address (in policy) */

		memset(&route, 0, sizeof(route));
		route.prefix.afi = tcfg.dest_addr.afi;
		route.prefix.addr_len = xp_out->selector.prefixlen_d;
		route.prefix.ip_addr.ipv6_addr[0] = ntohl(xp_out->selector.daddr.a6[0]);
		route.prefix.ip_addr.ipv6_addr[1] = ntohl(xp_out->selector.daddr.a6[1]);
		route.prefix.ip_addr.ipv6_addr[2] = ntohl(xp_out->selector.daddr.a6[2]);
		route.prefix.ip_addr.ipv6_addr[3] = ntohl(xp_out->selector.daddr.a6[3]);

		/* try to delete old route first */
		ret = ca_l3_route_get(device_id, &route);
		if (ret == CA_E_OK) {
			ret = ca_l3_route_delete(device_id, &route);
			if (ret != CA_E_OK) {
				printk(KERN_ERR "%s: ERROR! Fail to delete existing wrong Route for IPSec peer. (ret %d)\n", __func__, ret);
				ca_l3_nexthop_delete(device_id, nexthop.nexthop_id);
				ca_l3_intf_delete(device_id, l3_intf.intf_id);
				ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
				ca_tunnel_delete(device_id, tunnel_id);
				ca_ipsec_sa_delete(device_id, sa_id_encrypt);
				ca_ipsec_sa_delete(device_id, sa_id_decrypt);
				return ret;
			}
		}

		route.nexthop_id = nexthop.nexthop_id;

		ret = ca_l3_route_add(device_id, &route);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to create Route for IPSec peer. (ret %d)\n", __func__, ret);
			ca_l3_nexthop_delete(device_id, nexthop.nexthop_id);
			ca_l3_intf_delete(device_id, l3_intf.intf_id);
			ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
			ca_tunnel_delete(device_id, tunnel_id);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return ret;
		}

		/*** new route/nexthop/intf/tunnel is set for rekey, old route is deleted, then delete other old ones ***/

		/* delete nexthop */

		ca_l3_nexthop_delete(device_id, iter->nexthop_id);

		/* delete tunnel interface */

		memset(&tmp_l3_intf, 0, sizeof(l3_intf));
		tmp_l3_intf.intf_id = iter->intf_id;
		ret = ca_l3_intf_get(device_id, &tmp_l3_intf);
		if (ret == CA_E_OK) {
			ca_l3_intf_delete(device_id, tmp_l3_intf.intf_id);
		}

		/* delete tunnel */

		ca_tunnel_delete(device_id, iter->tunnel_id);

		/* clean recorded tunnel in kernel_hook */

		ca_kh_l3_tunnel_delete(device_id, iter->tunnel_id);

		/* delete SA */

		ca_ipsec_sa_delete(device_id, iter->sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, iter->sa_id_decrypt);

		/* clean recorded tunnel intf_id in kernel_hook */

		ca_kh_l3_tunnel_set_intf_id(device_id, tmp_l3_intf.ip_addr.afi, 0, tmp_l3_intf.intf_id, CA_TUNNEL_IPSEC);

		kfree(iter);

	} else { /* !rekey */

		memset(&nexthop, 0, sizeof(nexthop));

		nexthop.attr_flags = CA_L3_NHOP_ATTR_ROUTER;
		nexthop.addr.afi = tcfg.dest_addr.afi;
		nexthop.intf_id = l3_intf.intf_id;

		ret = ca_l3_nexthop_add(device_id, &nexthop, &nexthop.nexthop_id);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to create NextHop for IPSec peer. (ret %d)\n", __func__, ret);
			ca_l3_intf_delete(device_id, l3_intf.intf_id);
			ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
			ca_tunnel_delete(device_id, tunnel_id);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return ret;
		}

		/* create route for remote address (in policy) */

		memset(&route, 0, sizeof(route));
		route.prefix.afi = tcfg.dest_addr.afi;
		route.prefix.addr_len = xp_out->selector.prefixlen_d;
		route.prefix.ip_addr.ipv6_addr[0] = ntohl(xp_out->selector.daddr.a6[0]);
		route.prefix.ip_addr.ipv6_addr[1] = ntohl(xp_out->selector.daddr.a6[1]);
		route.prefix.ip_addr.ipv6_addr[2] = ntohl(xp_out->selector.daddr.a6[2]);
		route.prefix.ip_addr.ipv6_addr[3] = ntohl(xp_out->selector.daddr.a6[3]);

		/* try to delete old route first */
		ret = ca_l3_route_get(device_id, &route);
		if (ret == CA_E_OK) {
			ret = ca_l3_route_delete(device_id, &route);
			if (ret != CA_E_OK) {
				printk(KERN_ERR "%s: ERROR! Fail to delete existing wrong Route for IPSec peer. (ret %d)\n", __func__, ret);
				ca_l3_nexthop_delete(device_id, nexthop.nexthop_id);
				ca_l3_intf_delete(device_id, l3_intf.intf_id);
				ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
				ca_tunnel_delete(device_id, tunnel_id);
				ca_ipsec_sa_delete(device_id, sa_id_encrypt);
				ca_ipsec_sa_delete(device_id, sa_id_decrypt);
				return ret;
			}
		}

		route.nexthop_id = nexthop.nexthop_id;

		ret = ca_l3_route_add(device_id, &route);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to create Route for IPSec peer. (ret %d)\n", __func__, ret);
			ca_l3_nexthop_delete(device_id, nexthop.nexthop_id);
			ca_l3_intf_delete(device_id, l3_intf.intf_id);
			ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
			ca_tunnel_delete(device_id, tunnel_id);
			ca_ipsec_sa_delete(device_id, sa_id_encrypt);
			ca_ipsec_sa_delete(device_id, sa_id_decrypt);
			return ret;
		}

	} /* rekey */

	/* record the added tunnel info into local tunnel_list */

	ipsec_node = kzalloc(sizeof(*ipsec_node), GFP_KERNEL);
	if (ipsec_node == NULL) {
		printk(KERN_INFO "%s: Out of memory.\n", __func__);
		ca_l3_route_delete(device_id, &route);
		ca_l3_nexthop_delete(device_id, nexthop.nexthop_id);
		ca_l3_intf_delete(device_id, l3_intf.intf_id);
		ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
		ca_tunnel_delete(device_id, tunnel_id);
		ca_ipsec_sa_delete(device_id, sa_id_encrypt);
		ca_ipsec_sa_delete(device_id, sa_id_decrypt);
		return CA_E_MEMORY;
	}

	ipsec_node->intf_id = l3_intf.intf_id;
	ipsec_node->tunnel_id = tunnel_id;
	ipsec_node->sa_id_encrypt = sa_id_encrypt;
	ipsec_node->sa_id_decrypt = sa_id_decrypt;
	ipsec_node->spi_encrypt = htonl(x_out->id.spi);
	ipsec_node->spi_decrypt = htonl(x_in->id.spi);
	ipsec_node->xfrm_state_in = x_in;
	ipsec_node->xfrm_state_out = x_out;
	ipsec_node->xfrm_policy_index_in = xp_in->index;
	ipsec_node->xfrm_policy_index_out = xp_out->index;
	ipsec_node->nexthop_id = nexthop.nexthop_id;
	write_lock(&tunnel_list_lock);
	list_add(&ipsec_node->list, &tunnel_list);
	write_unlock(&tunnel_list_lock);

	/* record the added tunnel info into shared tunnel table in kernel_hook */

	kh_tunnel.type = CA_TUNNEL_IPSEC;
	kh_tunnel.dest_addr.afi = tcfg.dest_addr.afi;
	kh_tunnel.dest_addr.addr_len = tcfg.dest_addr.addr_len;
	kh_tunnel.dest_addr = tcfg.dest_addr;

	kh_tunnel.src_addr.afi = tcfg.src_addr.afi;
	kh_tunnel.src_addr.addr_len = tcfg.src_addr.addr_len;
	kh_tunnel.src_addr = tcfg.src_addr;

	kh_tunnel.tunnel_id = tunnel_id;
	ret = ca_kh_l3_tunnel_add(device_id, &kh_tunnel);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! ca_kh_l3_tunnl_add() fails. (ret %d)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

/* It's impossible to get SPI from xfrm_state and xfrm_policy, so this function doesn't need xfrm_state. */
ca_status_t ca_kh_ipsec_delete(ca_device_id_t device_id, struct xfrm_policy *xp_in, struct xfrm_policy *xp_out)
{
	ca_status_t ret;
	struct ipsec_node *iter, *tmp_iter;
	bool found = false;
	ca_l3_intf_t l3_intf;
	ca_l3_route_t route;

	if ((!xp_in) || (!xp_out))
		return CA_E_PARAM;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Dump ingress xfrm_policy:\n", __func__);
		dump_xfrm_policy(xp_in);
		printk(KERN_INFO "%s: Dump egress xfrm_policy:\n", __func__);
		dump_xfrm_policy(xp_out);
	}

	/* search local tunnel_list by xfrm_policy index */

	write_lock(&tunnel_list_lock);
	list_for_each_entry_safe(iter, tmp_iter, &tunnel_list, list) {
		if ((xp_in->index == iter->xfrm_policy_index_in) && (xp_out->index == iter->xfrm_policy_index_out)) {
			found = true;
			list_del(&iter->list);
			break;
		}

		/* check the condition of reversed direction */
		if ((xp_out->index == iter->xfrm_policy_index_in) && (xp_in->index == iter->xfrm_policy_index_out)) {
			found = true;
			list_del(&iter->list);
			break;
		}
	}
	write_unlock(&tunnel_list_lock);

	/* delete route */

	memset(&route, 0, sizeof(route));
	if (xp_in->family == AF_INET)
		route.prefix.afi = CA_IPV4;
	else if (xp_in->family == AF_INET6)
		route.prefix.afi = CA_IPV6;
	else
		return CA_E_PARAM;

	route.prefix.addr_len = xp_out->selector.prefixlen_d;
	route.prefix.ip_addr.ipv6_addr[0] = ntohl(xp_out->selector.daddr.a6[0]);
	route.prefix.ip_addr.ipv6_addr[1] = ntohl(xp_out->selector.daddr.a6[1]);
	route.prefix.ip_addr.ipv6_addr[2] = ntohl(xp_out->selector.daddr.a6[2]);
	route.prefix.ip_addr.ipv6_addr[3] = ntohl(xp_out->selector.daddr.a6[3]);

	ret = ca_l3_route_get(device_id, &route);
	if (ret == CA_E_OK) {
		ret = ca_l3_route_delete(device_id, &route);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "%s: ERROR! Fail to delete Route for IPSec peer. (ret %d)\n", __func__, ret);
		}
	} else {
		printk(KERN_ERR "%s: ERROR! Fail to get Route for IPSec peer. (ret %d)\n", __func__, ret);
	}

	/* delete nexthop */

	ret = ca_l3_nexthop_delete(device_id, iter->nexthop_id);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: ERROR! Fail to delete NextHop for IPSec tunnel. (ret %d)\n", __func__, ret);
	}

	/* delete tunnel interface */

	memset(&l3_intf, 0, sizeof(l3_intf));
	l3_intf.intf_id = iter->intf_id;
	ret = ca_l3_intf_get(device_id, &l3_intf);
	if (ret == CA_E_OK) {
		ret = ca_l3_intf_delete(device_id, l3_intf.intf_id);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: ERROR! Fail to delete IPSec interface. (intf_id %u, ret %d)\n", __func__, l3_intf.intf_id, ret);
		}
	} else {
		printk(KERN_INFO "%s: ERROR! Fail to get IPSec interface. (intf_id %u, ret %d)\n", __func__, iter->intf_id, ret);
	}

	/* delete tunnel */

	ret = ca_tunnel_delete(device_id, iter->tunnel_id);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete IPSec tunnel. (tunnel_id %u, ret %d)\n", __func__, iter->tunnel_id, ret);
	}

	/* clean recorded tunnel in kernel_hook */

	ret = ca_kh_l3_tunnel_delete(device_id, iter->tunnel_id);
	if (ret != CA_E_OK) {
		printk("%s: ERROR: ca_kh_l3_tunnel_delete() fails. (ret %d)\n", __func__, ret);
	}

	/* delete SA */

	ret = ca_ipsec_sa_delete(device_id, iter->sa_id_encrypt);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete IPSec SA for encryption. (sa_id %u, ret %d)\n", __func__, iter->sa_id_encrypt, ret);
	}

	ret = ca_ipsec_sa_delete(device_id, iter->sa_id_decrypt);
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: ERROR! Fail to delete IPSec SA for decryption. (sa_id %u, ret %d)\n", __func__, iter->sa_id_decrypt, ret);
	}

	kfree(iter);

	/* clean recorded tunnel intf_id in kernel_hook */

	ret = ca_kh_l3_tunnel_set_intf_id(device_id, l3_intf.ip_addr.afi, 0, l3_intf.intf_id, CA_TUNNEL_IPSEC);
	if (ret != CA_E_OK) {
		printk("%s: ERROR! ca_kh_l3_tunnel_set_intf_id() fails! (ret %d)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

/* Redirect the packets from kernel to PE. Thus, packets can be encrypted/decrypted in PE instead of kernel.
 * skb will be freed in Linux path.
 *
 * Parameter:
 *   dir: 0 is encryption/outbound, 1 is decryption/inbound. Same as ca_tunnel_direction_t.
 *
 * Return value:
 *   0: Redirect successfully.
 *   not 0: Something wrong, so go to Linux original path.
 */
ca_status_t ca_kh_ipsec_redirect_to_pe(struct sk_buff *skb, struct xfrm_state *x, __ca_ip_afi_t afi, unsigned char dir)
{
	ca_status_t ret;
	struct ipsec_node *iter;
	ca_ipsec_sa_id_t tmp_ipsec_sa_id;
	ca_ipsec_sa_t tmp_ipsec_sa;
	bool found = false;
	uint32_t ip_hdr_len = (afi == __CA_IPV4) ? sizeof(struct iphdr) : sizeof(struct ipv6hdr);

	struct ethhdr *ethh;
	struct iphdr *iph;

	tunnel_info_t tinfo;
	ca_pkt_t *pkt;
	ca_pkt_block_t *pkt_block;

	if ((!skb) || (!x))
		return CA_E_PARAM;

	if ((afi != __CA_IPV4) && (afi != __CA_IPV6))
		return CA_E_PARAM;

	if ((dir != CA_TUNNEL_DIR_OUTBOUND) && (dir != CA_TUNNEL_DIR_INBOUND))
		return CA_E_PARAM;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: afi=%d, dir is %s.\n", __func__, afi, (dir == 0) ? "Encrypt/Outbound" : "Decrypt/Inbound");
		printk(KERN_INFO "%s: Dump xfrm_state:\n", __func__);
		dump_xfrm_state(x);
	}

	read_lock_bh(&tunnel_list_lock);
	list_for_each_entry(iter, &tunnel_list, list) {
		tmp_ipsec_sa_id = (dir == CA_TUNNEL_DIR_OUTBOUND) ? iter->sa_id_encrypt : iter->sa_id_decrypt;
		ret = ca_ipsec_sa_get(0, tmp_ipsec_sa_id, &tmp_ipsec_sa);
		if (ret != 0) {
			printk(KERN_ERR "%s: INTERNAL ERROR! Can not get SA by sa_id %u. (dir %u, ret %d)\n",
					__func__, tmp_ipsec_sa_id, dir, ret);
			read_unlock_bh(&tunnel_list_lock);
			return CA_E_INTERNAL;
		}

		if (x->id.spi == ntohl(tmp_ipsec_sa.spi)) {
			found = true;
			break;
		}

	}
	read_unlock_bh(&tunnel_list_lock);

	if (found == false) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_ERR "%s: ERROR! Can not found tunnel for the packet. (dir %u, afi %d, spi 0x%08x)\n", __func__, dir, afi, htonl(x->id.spi));
		}
		return CA_E_ERROR;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: Found tunnel. (dir %u, afi %d, tunnel_id %u)\n", __func__, dir, afi, iter->tunnel_id);
	}

	/* Current skb->data points to IP payload (ESP header) and there is not L2 header in this skb packet.
	 * For offload purpose, Packet Engine needs full packet structure, so construct a L2 header here.
	 */

	iph = (struct iphdr *) (skb->data - ip_hdr_len);
	ethh = (struct ethhdr *) (skb->data - ip_hdr_len - ETH_HLEN);

	/* simple packet validation */
	if (   ((iph->version != 4) && (iph->version != 6))
	    || ((iph->version == 4) && (afi != __CA_IPV4))
	    || ((iph->version == 6) && (afi != __CA_IPV6)) ) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_INFO "%s: Error packet. (afi %d, iph_version %u)\n", __func__, afi, iph->version);
		}
		return CA_E_ERROR;
	}

	/* setup info for PE */
	// TBD: assume this is IPSec only, not consider L2TP over IPSec.

	memset(&tinfo, 0, sizeof(tinfo));
	tinfo.func_id = CA_FUNC_ID_IPSEC;
	tinfo.pkt_type = CA_PKT_TYPE_FROM_CPU;
	tinfo.direction = (dir == 0) ? CA_IPSEC_SPD_ENCRYPT : CA_IPSEC_SPD_DECRYPT;

	pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
	if (pkt == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory for pkt.\n", __func__);
		return CA_E_MEMORY;
	}

	pkt_block = kzalloc(sizeof(*pkt_block), GFP_KERNEL);
	if (pkt == NULL) {
		printk(KERN_ERR "%s: Fail to allocate memory for pkt_block.\n", __func__);
		return CA_E_MEMORY;
	}

	pkt_block->data = skb->data - ip_hdr_len - ETH_HLEN;
	pkt_block->len = skb->len + ip_hdr_len + ETH_HLEN;

	pkt->pkt_data = pkt_block;
	pkt->block_count = 1;
	pkt->device_id = 0;
	pkt->src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU2);
	pkt->dst_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU6);
	pkt->dst_sub_port_id = 0;
	pkt->pkt_len = pkt_block->len;
	pkt->flags = CA_TX_BYPASS_FWD_ENGINE;
	pkt->cos = 0;
	pkt->sw_id[0] = ((ipsec_private_data_t *) g_tunnel[iter->tunnel_id]->private_data)->idx4pe & 0xFFFF;
	pkt->sw_id[1] = tinfo.value;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: sw_id[0]=0x%04x, sw_id[1]=0x%04x\n", __func__, pkt->sw_id[0], pkt->sw_id[1]);
	}

	ret = __ca_tx(0, pkt);
	if (ret != CA_E_OK) {
		printk(KERN_ERR "%s: Fail to call __ca_tx(). (ret %d)\n", __func__, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL)
		printk(KERN_INFO "%s: __ca_tx() is called successfully.\n", __func__);

	return CA_E_OK; /* handle successfully, kernel can skip following jobs */
}

/* Packet in skb is encrypted or decrypted by PE already, and then NE forwards packet to CPU due to hash miss.
 * In this condition, packet can't enter Linux protocol stack directly because Linux doesn't know packet is handled by PE already.
 *
 * Parameter:
 *
 * Return value:
 */
ca_status_t ca_ni_rx_ipsec(struct sk_buff *skb, HEADER_CPU_T *hdr_cpu)
{
	aal_ipsec_idx4pe_entry_t ipsec_idx4pe_entry;
	unsigned int idx4pe;
	tunnel_info_t tinfo;
	int ret;
	bool found = false;
	ca_tunnel_id_t tunnel_id;
	struct ipsec_node *iter;
	struct xfrm_state *x = NULL;

	if (skb == NULL) {
		printk(KERN_INFO "%s: skb is null.\n", __func__);
		ret = CA_E_PARAM;
		goto _error_ca_ni_rx_ipsec;
	}

	if (hdr_cpu == NULL) {
		printk(KERN_INFO "%s: hdr_cpu is null.\n", __func__);
		ret = CA_E_PARAM;
		goto _error_ca_ni_rx_ipsec;
	}

	idx4pe = hdr_cpu->mdata_raw.mdata_h & 0xffff;
	tinfo.value = (hdr_cpu->mdata_raw.mdata_h >> 16) & 0xff;

	if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
		printk(KERN_INFO "%s: mdata_h=0x%08x, idx4pe=%u, func_id=%d, pkt_type=%d, direction=%d.\n",
			__func__, hdr_cpu->mdata_raw.mdata_h, idx4pe, tinfo.func_id, tinfo.pkt_type, tinfo.direction);
	}

	ret = aal_entry_get_by_idx(AAL_TABLE_IPSEC_IDX4PE, idx4pe,  &ipsec_idx4pe_entry);
	if (ret != AAL_E_OK) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_INFO "%s: Fail to get IPSec IDX4PE. (ret %d, idx4pe %u)\n", __func__, ret, idx4pe);
		}
		ret = CA_E_INTERNAL;
		goto _error_ca_ni_rx_ipsec;
	}

	read_lock(&tunnel_list_lock);
	list_for_each_entry(iter, &tunnel_list, list) {
		if ((iter->sa_id_encrypt == ipsec_idx4pe_entry.sa_id_encrypt) && (iter->sa_id_decrypt == ipsec_idx4pe_entry.sa_id_decrypt)) {
			tunnel_id = iter->tunnel_id;
			found = true;
			break;
		}
	}
	read_unlock(&tunnel_list_lock);

	if (found == false) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_INFO "%s: Cannot find tunnel of sa_id_encrypt %u & sa_id_decrypt %u.\n",
				__func__, ipsec_idx4pe_entry.sa_id_encrypt, ipsec_idx4pe_entry.sa_id_decrypt);
		}
		ret = CA_E_PARAM;
		goto _error_ca_ni_rx_ipsec;
	}

	/* hook xfrm_state into skb */

	if (!skb->sp || atomic_read(&skb->sp->refcnt) != 1) {
		struct sec_path *sp;

		sp = secpath_dup(skb->sp);
		if (!sp) {
			if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
				printk(KERN_INFO "%s: secpath_dup() returns NULL.\n", __func__);
			}
			//XFRM_INC_STATS(net, LINUX_MIB_XFRMINERROR);
			ret = CA_E_PARAM;
			goto _error_ca_ni_rx_ipsec;
		}
		if (skb->sp)
			secpath_put(skb->sp);
		skb->sp = sp;
	}

	if (skb->sp->len == XFRM_MAX_DEPTH) {
		if (ca_kh_debug & CA_KH_DEBUG_TUNNEL) {
			printk(KERN_INFO "%s: skb->sp->len == XFRM_MAX_DEPTH (%d).\n", __func__, XFRM_MAX_DEPTH);
		}
		ret = CA_E_PARAM;
		goto _error_ca_ni_rx_ipsec;
	}

	x = (tinfo.direction == CA_IPSEC_SPD_ENCRYPT) ? iter->xfrm_state_out : iter->xfrm_state_in;

	xfrm_state_hold(x);
	skb->sp->xvec[skb->sp->len++] = x;

	ca_skb_out(skb);
	netif_rx_cortina(skb);

	return CA_E_OK;

_error_ca_ni_rx_ipsec:
	ca_skb_out(skb);
	ca_dev_kfree_skb(skb);

	return ret;
}

