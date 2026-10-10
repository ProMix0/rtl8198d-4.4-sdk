#include "osal_common.h"
#include "aal_table.h"
#include "aal_common.h"
#include "aal_l3_cls.h"
#include "aal_hash.h"
#include "aal_l3_cam.h"
#include "aal_l3fe.h"
#include "aal_l2_tm.h"
#include "aal_l3_tm.h"
#include "aal_l3_if.h"
#include "classifier.h"
#include "route_internal.h"
#include "tunnel.h"
#include "api_common.h"
#include "rate.h"
#include "aal_hashlite_init.h"

#define CLS_RULE_MAX (L3_CLS_KEY_TBL_ENTRY_MAX*4)
#define CLS_DEF_RULE_MAX (AAL_LPORT_L3_LAN+1)

typedef enum {
	CL_RUL_INFO_CLS = 0,
	CL_RUL_INFO_HASH,
	CL_RUL_INFO_CAM_ETHERTYPE,
	CL_RUL_INFO_CAM_PPP_PROT,
	CL_RUL_INFO_CAM_SPORT,
	CL_RUL_INFO_CAM_DPORT,
	CL_RUL_INFO_CAM_SPCL_HDR,
	CL_RUL_INFO_CAM_PKT_LEN,
	CL_RUL_INFO_L2_CLS,
	CL_RUL_INFO_HASH_LITE,
	CL_RUL_INFO_MAX = 32
} cls_rule_info_type_e;

typedef enum {
	CL_CFM_CCM = 1,
	CL_CFM_LBR = 2,
	CL_CFM_LBM = 3,
	CL_CFM_LTR = 4,
	CL_CFM_LTM = 5,
	CL_CFM_LMM = 43,
	CL_CFM_1DM = 45,
	CL_CFM_DMR = 46,
	CL_CFM_DMM = 47
} cls_cfm_opcode_e;

#define CLS_RUL_INFO_CLS (1 << CL_RUL_INFO_CLS)
#define CLS_RUL_INFO_HASH (1 << CL_RUL_INFO_HASH)
#define CLS_RUL_INFO_CAM_ETHERTYPE (1 << CL_RUL_INFO_CAM_ETHERTYPE)
#define CLS_RUL_INFO_CAM_PPP_PROT (1 << CL_RUL_INFO_CAM_PPP_PROT)
#define CLS_RUL_INFO_CAM_SPORT (1 << CL_RUL_INFO_CAM_SPORT)
#define CLS_RUL_INFO_CAM_DPORT (1 << CL_RUL_INFO_CAM_DPORT)
#define CLS_RUL_INFO_CAM_SPCL_HDR (1 << CL_RUL_INFO_CAM_SPCL_HDR)
#define CLS_RUL_INFO_CAM_PKT_LEN (1 << CL_RUL_INFO_CAM_PKT_LEN)
#define CLS_RUL_INFO_L2_CLS (1 << CL_RUL_INFO_L2_CLS)
#define CLS_RUL_INFO_HASH_LITE (1 << CL_RUL_INFO_HASH_LITE)

typedef struct cls_cls_s {
	ca_uint32_t profile_id;
	ca_uint32_t rule_idx;
} cls_cls_t;

typedef struct cls_hash_s {
	ca_uint32_t profile_id;
	ca_uint32_t mask_idx;
	ca_uint8_t tuple_idx;
	ca_uint32_t hash_idx;
} cls_hash_t;

typedef struct cls_cam_s {
	ca_uint32_t rslt_idx;
} cls_cam_t;

typedef struct cls_rule_l2fe_s {
    ca_boolean_t l2fe_cls_vld;
    ca_uint32_t l2fe_cls_index;
} cls_rule_l2fe_t;

typedef struct hw_info_s {
	cls_cls_t cls;
	cls_hash_t hash;
	cls_cam_t ethertype;
	cls_cam_t ppp_prot;
	cls_cam_t l4_sport;
	cls_cam_t l4_dport;
	cls_cam_t spcl_hdr;
	cls_cam_t pkt_len;
	cls_rule_l2fe_t l2fe_info;
} hw_info_t;

typedef struct cls_rule_s {
	ca_uint32_t priority;
	ca_classifier_key_t key;
	ca_classifier_key_mask_t key_mask;
	ca_classifier_action_t action;
} cls_rule_t;

typedef struct cls_rule_info_s {
	ca_uint32_t type;
	hw_info_t hw_info;
	ca_uint32_t rule_crc32;
	cls_rule_t rule;
	ca_uint32_t refcnt;
} cls_rule_info_t;

cls_rule_info_t *cls_rule_list[CLS_RULE_MAX];
ca_classifier_default_action_t *cls_def_rule[CLS_DEF_RULE_MAX];
static ca_uint8_t cls_init_done = 0;
static ca_uint cls_rule_lock;
static ca_uint32_t wan_hash_cnt = 0;
static ca_uint32_t lan_hash_cnt = 0;

#define CLS_TOP_TPID_ENC_SET(i_tpid, o_tpid_enc) {\
	ca_uint8_t tpid_idx;\
	if (AAL_E_OK != aal_l3fe_pp_top_tpid_get(i_tpid, &tpid_idx))\
		return CA_E_NOT_SUPPORT;\
	o_tpid_enc = tpid_idx;\
} while(0)

#define CLS_INNER_TPID_ENC_SET(i_tpid, o_tpid_enc) {\
	ca_uint8_t tpid_idx;\
	if (AAL_E_OK != aal_l3fe_pp_inner_tpid_get(i_tpid, &tpid_idx))\
		return CA_E_NOT_SUPPORT;\
	o_tpid_enc = tpid_idx;\
} while(0)

static ca_status_t cls_rule_list_def_rule_set(ca_uint32_t lspid, ca_classifier_default_action_t *def_rule);

#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
extern ca_tunnel_t *g_tunnel[CA_TUNNEL_MAX_LIMIT];
extern struct ca_int_l3_intf_s *ca_l3_intf_tbl;
extern struct list_head nexthop_table;

static ca_status_t get_nexthop_id_by_tunnel_id(ca_tunnel_id_t tunnel_id, ca_l3_nexthop_id_t *nexthop_id)
{
	int i;
	bool found_intf = false, found_nexthop = false;
	struct nexthop_node *iter_nh = NULL;
	ca_intf_id_t intf_id;

	if (nexthop_id == NULL)
		return CA_E_PARAM;

	for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
		if (ca_l3_intf_tbl[i].use == 0) continue;

		if (   (ca_l3_intf_tbl[i].intf.type == CA_L3_INTF_TUNNEL)
				&& (ca_l3_intf_tbl[i].intf.mask.tunnel_id != 0)
				&& (ca_l3_intf_tbl[i].intf.tunnel_id == tunnel_id) ) {
			found_intf = true;
			intf_id = i;
			break;
		}
	}

	if (found_intf) {
		list_for_each_entry(iter_nh, &nexthop_table, list) {
			if (iter_nh->nexthop.intf_id == intf_id) {
				found_nexthop = true;
				*nexthop_id = iter_nh->nexthop.nexthop_id;
				return CA_E_OK;
			}
		}
	}

	return CA_E_NOT_FOUND;
}
#endif

static ca_status_t cls_rule_list_init(void)
{
	ca_uint32_t lspid;
	ca_classifier_default_action_t l3_action;

	if (cls_init_done)
		return CA_E_OK;

	/* use spin_lock_bh() to protect the interfer from NI RX */
	/* use spin_lock_irqsave() to protect the interface PON isr */
	if (CA_OSAL_OK != ca_spin_lock_init(&cls_rule_lock, SPINLOCK_IRQ)) {
		ca_printf("%s: init spinlock fail\n", __func__);
		return CA_E_INIT;
	}
	memset(cls_rule_list, 0, sizeof(cls_rule_list));
	memset(cls_def_rule, 0, sizeof(cls_def_rule));

	/* init def rule list */
	memset(&l3_action, 0, sizeof(ca_classifier_default_action_t));
	l3_action.action_type = CA_CLASSIFIER_FORWARD_DENY;

	for (lspid = AAL_LPORT_ETH_NI0; lspid <= AAL_LPORT_L3_LAN; lspid++) {
		/* l3 def rule list */
		if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_L3_LAN || lspid == AAL_LPORT_ETH_NI7) {
			/* update l3 def rule list */
			cls_rule_list_def_rule_set(lspid, &l3_action);
		}
	}

	cls_init_done = 1;

	return CA_E_OK;
}

static ca_status_t cls_rule_list_rule_exist(
	cls_rule_info_t *rule,
	ca_uint32_t *index)
{
	ca_uint16_t i;

	if (rule == NULL)
		return CA_E_ERROR;

	rule->rule_crc32 = ca_crc32(0, (ca_uint8 *)&rule->rule, sizeof(cls_rule_t));
	/* find a rule entry */
	for (i = 0; i < CLS_RULE_MAX; i++) {
		if (cls_rule_list[i] == NULL)
			continue;
		if (cls_rule_list[i]->rule_crc32 != rule->rule_crc32)
			continue;
		if (memcmp(&cls_rule_list[i]->rule, &rule->rule, sizeof(cls_rule_t)))
			continue;
		/* rule found */
		cls_rule_list[i]->refcnt++;
		*index = i+1;
		return CA_E_EXISTS;
	}

	return CA_E_NOT_FOUND;
}

static ca_status_t cls_rule_list_hashlite_rule_search(
	cls_rule_info_t *rule,
	ca_uint32_t *index)
{
	ca_uint16_t i;

	if (rule == NULL)
		return CA_E_ERROR;

	/* find a rule entry */
	for (i = 0; i < CLS_RULE_MAX; i++) {
		if (cls_rule_list[i] == NULL)
			continue;
		if (!(cls_rule_list[i]->type & CLS_RUL_INFO_HASH_LITE))
			continue;
		if (cls_rule_list[i]->hw_info.hash.hash_idx != rule->hw_info.hash.hash_idx)
			continue;
		if (cls_rule_list[i]->hw_info.hash.profile_id != rule->hw_info.hash.profile_id)
			continue;
		if (memcmp(&cls_rule_list[i]->rule.key, &rule->rule.key, sizeof(ca_classifier_key_t)))
			continue;
		if (memcmp(&cls_rule_list[i]->rule.key_mask, &rule->rule.key_mask, sizeof(ca_classifier_key_mask_t)))
			continue;
		/* rule found */
		*index = i+1;
		return CA_E_OK;
	}
	return CA_E_NOT_FOUND;
}

static ca_status_t cls_rule_list_add(
	cls_rule_info_t *rule,
	ca_uint32_t *index)
{
	ca_uint16_t i;
	cls_rule_info_t *ptr;
	ca_uint32_t sz;

	if (rule == NULL || index == NULL)
		return CA_E_ERROR;
	/* find an empty rule entry */
	for (i = 0; i < CLS_RULE_MAX; i++) {
		if (cls_rule_list[i] == NULL)
			break;
	}
	if (i >= CLS_RULE_MAX)
		return CA_E_FULL;

	sz = sizeof(cls_rule_info_t);
	if (NULL == (ptr = ca_malloc(sz)))
		return CA_E_MEMORY;
	memset(ptr, 0, sz);
	rule->rule_crc32 = ca_crc32(0, (ca_uint8 *)&rule->rule, sizeof(cls_rule_t));
	rule->refcnt = 1;
	memcpy(ptr, rule, sz);
	cls_rule_list[i] = ptr;
	*index = i+1;
	return CA_E_OK;
}

static ca_status_t cls_rule_list_delete(
	ca_uint32_t index)
{
	cls_rule_info_t *ptr = NULL;

	if (index > CLS_RULE_MAX)
		return CA_E_ERROR;

	ptr = cls_rule_list[index-1];
	if (ptr == NULL)
		return CA_E_NOT_FOUND;
	if (ptr->refcnt == 1) {
		ca_free(ptr);
		cls_rule_list[index-1] = NULL;
	} else
		ptr->refcnt--;
	return CA_E_OK;
}

static ca_status_t cls_rule_list_delete_all(void)
{
	ca_uint16_t index;
	cls_rule_info_t *ptr = NULL;

	for (index = 0; index < CLS_RULE_MAX; index++) {
		ptr = cls_rule_list[index];
		if (ptr == NULL)
			continue;
		ca_free(ptr);
		cls_rule_list[index] = NULL;
	}
	return CA_E_OK;
}

static ca_status_t cls_rule_list_get(
	ca_uint32_t index,
	cls_rule_info_t *rule)
{
	cls_rule_info_t *ptr = NULL;

	if (index > CLS_RULE_MAX || index < 1 || rule == NULL)
		return CA_E_ERROR;
	ptr = cls_rule_list[index-1];
	if (ptr == NULL)
		return CA_E_NOT_FOUND;
	memcpy(rule, ptr, sizeof(cls_rule_info_t));
	return CA_E_OK;
}

static ca_status_t cls_rule_list_def_rule_set(
	ca_uint32_t                   lspid,
	ca_classifier_default_action_t *def_rule)
{
	ca_uint32_t sz;

	if (lspid >= CLS_DEF_RULE_MAX)
		return CA_E_PORT;
	if (def_rule == NULL)
		return CA_E_ERROR;

	if (cls_def_rule[lspid] == NULL) {
		sz = sizeof(ca_classifier_default_action_t);
		if (NULL == (cls_def_rule[lspid] = ca_malloc(sz)))
			return CA_E_MEMORY;
	}
	memcpy(cls_def_rule[lspid], def_rule, sz);
	return CA_E_OK;
}

static ca_status_t cls_rule_list_def_rule_get(
	ca_uint32_t                   lspid,
	ca_classifier_default_action_t *def_rule)
{
	if (lspid >= CLS_DEF_RULE_MAX)
		return CA_E_PORT;
	if (def_rule == NULL || cls_def_rule[lspid] == NULL)
		return CA_E_ERROR;

	memcpy(def_rule, cls_def_rule[lspid], sizeof(ca_classifier_default_action_t));
	return CA_E_OK;
}

static ca_status_t cls_key_entry_update(
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	cls_key_t *aal_key,
	cls_rule_info_t *rule)
{
	ca_uint32_t rslt_idx, tmp;
	l3_cam_ethertype_tbl_entry_t ethertype;
	l3_cam_ppp_prot_tbl_entry_t cam_ppp_prot;
	l3_cam_sport_tbl_entry_t cam_sport;
	l3_cam_dport_tbl_entry_t cam_dport;
	l3_cam_spcl_hdr_tbl_entry_t cam_spcl_hdr;
	l3_cam_pkt_len_tbl_entry_t cam_pkt_len;

	if (key == NULL || key_mask == NULL || aal_key == NULL)
		return CA_E_PARAM;
	if (key_mask->src_intf || key_mask->dst_intf || key_mask->dest_port ||
		key_mask->key_handle || key_mask->l2_mask.subtype)
		return CA_E_NOT_SUPPORT;

#if !defined(CONFIG_ARCH_CORTINA_VENUS)
	if (key_mask->ip_mask.ip_sa_max || key_mask->ip_mask.ip_da_max)
		return CA_E_NOT_SUPPORT;
#if !defined(CONFIG_LUNA_G3_SERIES)	//LUNA_G3 still support this pattern with hw known issue (not support mask)
	/* support by main hash due to Bug#48809. The matching logic is 'OR'.  it doesn't support logic 'AND' among these tcp_flags bits*/
	if (key_mask->l4_mask.tcp_flags)
		return CA_E_NOT_SUPPORT;
#endif	//not CONFIG_LUNA_G3_SERIES
#endif

	/* not support sa ip range + da ip range,  one sa ip + da ip range,  sa ip range + one da ip */
	if (key_mask->ip_mask.ip_sa_max && key_mask->ip_mask.ip_da_max ||
		key_mask->ip_mask.ip_sa && key_mask->ip_mask.ip_da_max ||
		key_mask->ip_mask.ip_sa_max && key_mask->ip_mask.ip_da)
		return CA_E_NOT_SUPPORT;

	/* ignore all fields */
	aal_l3_cls_key_clean(aal_key);

	/* set key value */
	if (key_mask->ingress_lan && key->ingress_lan) {
		/* ignore lspid field for all lan port */
		;
	} else if (key_mask->src_port) {
		aal_key->lspid = PORT_ID(key->src_port);
		aal_key->msk_lspid = 0;
	} else
		/* must set src_port */
		return CA_E_PARAM;

	if (key_mask->orig_src_port) {
		aal_key->o_lspid = PORT_ID(key->orig_src_port);
		aal_key->msk_o_lspid = 0;
	}

	/* l2 fields */
	if (key_mask->l2) {
		if (key_mask->l2_mask.is_length) {
			aal_key->len_encoded = key->l2.is_length;
			aal_key->msk_len_encoded = 0;
		}
		if (key_mask->l2_mask.ethertype) {
			/* config l3 cam */
			memset(&ethertype, 0, sizeof(l3_cam_ethertype_tbl_entry_t));
			ethertype.vld = 1;
			ethertype.ethertype = key->l2.ethertype;
			if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_ETHERTYPE, &ethertype, &rslt_idx))
				return CA_E_CONFIG;
			/* update rule info */
			rule->type |= CLS_RUL_INFO_CAM_ETHERTYPE;
			rule->hw_info.ethertype.rslt_idx = rslt_idx;
			/* update aal key */
			aal_key->ethertype_enc = rslt_idx+1;
			aal_key->msk_ethertype_enc = 0;
		}
#if !defined(CONFIG_ARCH_CORTINA_VENUS)
		if (key_mask->l2_mask.mac_sa) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_sa.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_sa.mac_max[0],
				CA_ETH_ADDR_LEN))
				/* not support mac range */
				return CA_E_NOT_SUPPORT;
			else {
				/* one src mac */
				aal_key->mac_sa[0] = key->l2.mac_sa.mac_min[5];
				aal_key->mac_sa[1] = key->l2.mac_sa.mac_min[4];
				aal_key->mac_sa[2] = key->l2.mac_sa.mac_min[3];
				aal_key->mac_sa[3] = key->l2.mac_sa.mac_min[2];
				aal_key->mac_sa[4] = key->l2.mac_sa.mac_min[1];
				aal_key->mac_sa[5] = key->l2.mac_sa.mac_min[0];
				aal_key->msk_mac_sa = 0;
			}
		}
		if (key_mask->l2_mask.mac_da) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_da.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_da.mac_max[0],
				CA_ETH_ADDR_LEN))
				/* not support mac range */
				return CA_E_NOT_SUPPORT;
			else {
				/* one dst mac */
				aal_key->mac_da[0] = key->l2.mac_da.mac_min[5];
				aal_key->mac_da[1] = key->l2.mac_da.mac_min[4];
				aal_key->mac_da[2] = key->l2.mac_da.mac_min[3];
				aal_key->mac_da[3] = key->l2.mac_da.mac_min[2];
				aal_key->mac_da[4] = key->l2.mac_da.mac_min[1];
				aal_key->mac_da[5] = key->l2.mac_da.mac_min[0];
				aal_key->msk_mac_da = 0;
				/* set dest_addr to select a right key type */
				aal_key->dest_addr_type = 0;
				aal_key->dest_addr.dest_mac_addr[0] = key->l2.mac_da.mac_min[5];
				aal_key->dest_addr.dest_mac_addr[1] = key->l2.mac_da.mac_min[4];
				aal_key->dest_addr.dest_mac_addr[2] = key->l2.mac_da.mac_min[3];
				aal_key->dest_addr.dest_mac_addr[3] = key->l2.mac_da.mac_min[2];
				aal_key->dest_addr.dest_mac_addr[4] = key->l2.mac_da.mac_min[1];
				aal_key->dest_addr.dest_mac_addr[5] = key->l2.mac_da.mac_min[0];
				aal_key->msk_dest_addr = 0;
			}
		}
#else
		if (key_mask->l2_mask.mac_sa) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_sa.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_sa.mac_max[0],
				CA_ETH_ADDR_LEN)) {
				/*  src mac range */
				aal_key->mac_da[0] = key->l2.mac_sa.mac_min[5];
				aal_key->mac_da[1] = key->l2.mac_sa.mac_min[4];
				aal_key->mac_da[2] = key->l2.mac_sa.mac_min[3];
				aal_key->mac_da[3] = key->l2.mac_sa.mac_min[2];
				aal_key->mac_da[4] = key->l2.mac_sa.mac_min[1];
				aal_key->mac_da[5] = key->l2.mac_sa.mac_min[0];
				aal_key->mac_sa[0] = key->l2.mac_sa.mac_max[5];
				aal_key->mac_sa[1] = key->l2.mac_sa.mac_max[4];
				aal_key->mac_sa[2] = key->l2.mac_sa.mac_max[3];
				aal_key->mac_sa[3] = key->l2.mac_sa.mac_max[2];
				aal_key->mac_sa[4] = key->l2.mac_sa.mac_max[1];
				aal_key->mac_sa[5] = key->l2.mac_sa.mac_max[0];
				aal_key->mac_addr_ctrl = 1;
			} else {
				/* one src mac */
				aal_key->mac_sa[0] = key->l2.mac_sa.mac_min[5];
				aal_key->mac_sa[1] = key->l2.mac_sa.mac_min[4];
				aal_key->mac_sa[2] = key->l2.mac_sa.mac_min[3];
				aal_key->mac_sa[3] = key->l2.mac_sa.mac_min[2];
				aal_key->mac_sa[4] = key->l2.mac_sa.mac_min[1];
				aal_key->mac_sa[5] = key->l2.mac_sa.mac_min[0];
				aal_key->msk_mac_sa = 0;
				/* set mac_addr_ctrl to select a right key type */
				aal_key->mac_addr_ctrl = 5;
			}
		}
		if (key_mask->l2_mask.mac_da) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_da.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_da.mac_max[0],
				CA_ETH_ADDR_LEN)) {
				/* dst mac range */
				if (aal_key->mac_addr_ctrl == 1 || aal_key->mac_addr_ctrl == 5)
					/* not support  src mac range + dst mac range or one src mac + dst mac range */
					return CA_E_NOT_SUPPORT;
				aal_key->mac_da[0] = key->l2.mac_da.mac_min[5];
				aal_key->mac_da[1] = key->l2.mac_da.mac_min[4];
				aal_key->mac_da[2] = key->l2.mac_da.mac_min[3];
				aal_key->mac_da[3] = key->l2.mac_da.mac_min[2];
				aal_key->mac_da[4] = key->l2.mac_da.mac_min[1];
				aal_key->mac_da[5] = key->l2.mac_da.mac_min[0];
				aal_key->mac_sa[0] = key->l2.mac_da.mac_max[5];
				aal_key->mac_sa[1] = key->l2.mac_da.mac_max[4];
				aal_key->mac_sa[2] = key->l2.mac_da.mac_max[3];
				aal_key->mac_sa[3] = key->l2.mac_da.mac_max[2];
				aal_key->mac_sa[4] = key->l2.mac_da.mac_max[1];
				aal_key->mac_sa[5] = key->l2.mac_da.mac_max[0];
				aal_key->mac_addr_ctrl = 2;
			} else {
				/* one dst mac */
				if (aal_key->mac_addr_ctrl == 1)
					/* not support  src mac range + one dst mac */
					return CA_E_NOT_SUPPORT;
				aal_key->mac_da[0] = key->l2.mac_da.mac_min[5];
				aal_key->mac_da[1] = key->l2.mac_da.mac_min[4];
				aal_key->mac_da[2] = key->l2.mac_da.mac_min[3];
				aal_key->mac_da[3] = key->l2.mac_da.mac_min[2];
				aal_key->mac_da[4] = key->l2.mac_da.mac_min[1];
				aal_key->mac_da[5] = key->l2.mac_da.mac_min[0];
				aal_key->msk_mac_da = 0;
				/* set mac_addr_ctrl to select a right key type */
				/* 4:  one src mac + one dst mac, 6:  one dst mac */
				aal_key->mac_addr_ctrl = (aal_key->mac_addr_ctrl == 5) ? 4 : 6;
				/* set dest_addr to select a right key type */
				aal_key->dest_addr_type = 0;
				aal_key->dest_addr.dest_mac_addr[0] = key->l2.mac_da.mac_min[5];
				aal_key->dest_addr.dest_mac_addr[1] = key->l2.mac_da.mac_min[4];
				aal_key->dest_addr.dest_mac_addr[2] = key->l2.mac_da.mac_min[3];
				aal_key->dest_addr.dest_mac_addr[3] = key->l2.mac_da.mac_min[2];
				aal_key->dest_addr.dest_mac_addr[4] = key->l2.mac_da.mac_min[1];
				aal_key->dest_addr.dest_mac_addr[5] = key->l2.mac_da.mac_min[0];
				aal_key->msk_dest_addr = 0;
			}
		}
#endif
		if (key_mask->l2_mask.is_multicast) {
			aal_key->mac_da_mc = key->l2.is_multicast;
			aal_key->msk_mac_da_mc = 0;
		}
		if (key_mask->l2_mask.cfm_opcode) {
			switch(key->l2.cfm_opcode) {
				case CL_CFM_CCM:
					aal_key->spcl_pkt_enc = 0x22;
					break;
				case CL_CFM_LBR:
					aal_key->spcl_pkt_enc = 0x23;
					break;
				case CL_CFM_LBM:
					aal_key->spcl_pkt_enc = 0x13;
					break;
				case CL_CFM_LTR:
					aal_key->spcl_pkt_enc = 0x24;
					break;
				case CL_CFM_LTM:
					aal_key->spcl_pkt_enc = 0x25;
					break;
				case CL_CFM_LMM:
					aal_key->spcl_pkt_enc = 0x14;
					break;
				case CL_CFM_1DM:
					aal_key->spcl_pkt_enc = 0x15;
					break;
				case CL_CFM_DMR:
					aal_key->spcl_pkt_enc = 0x16;
					break;
				case CL_CFM_DMM:
					aal_key->spcl_pkt_enc = 0x15;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
			aal_key->spcl_pkt_enc_msk = 0;
		}
		/* ppp protocol */
		if (key_mask->l2_mask.ppp_proto) {
			/* config l3 cam */
			memset(&cam_ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
			cam_ppp_prot.vld = 1;
			cam_ppp_prot.ppp_prot = key->l2.ppp_proto;
			if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_PPP_PROT, &cam_ppp_prot, &rslt_idx))
				return CA_E_CONFIG;
			/* update rule info */
			rule->type |= CLS_RUL_INFO_CAM_PPP_PROT;
			rule->hw_info.ppp_prot.rslt_idx = rslt_idx;
			/* update aal key */
			aal_key->ppp_protocol_enc = rslt_idx+1;
			aal_key->msk_ppp_protocol = 0;
		}
		/* vlan count */
		if (key_mask->l2_mask.vlan_count) {
			aal_key->vlan_cnt = key->l2.vlan_count;
			aal_key->msk_vlan_cnt = 0;
		}
		/* outer vlan fields */
		if (key_mask->l2_mask.vlan_otag) {
			if (key_mask->l2_mask.vlan_otag_mask.tpid) {
				if(key->l2.vlan_otag.vlan_min.tpid !=
					key->l2.vlan_otag.vlan_max.tpid)
					return CA_E_NOT_SUPPORT;
				CLS_TOP_TPID_ENC_SET(key->l2.vlan_otag.vlan_min.tpid, aal_key->top_tpid_enc);
				aal_key->msk_top_tpid_enc = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.vid) {
				if(key->l2.vlan_otag.vlan_min.vid !=
					key->l2.vlan_otag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				aal_key->top_vid = key->l2.vlan_otag.vlan_min.vid;
				if (key_mask->l2_mask.vlan_itag && (key_mask->l2_mask.vlan_itag_mask.dei || key_mask->l2_mask.vlan_itag_mask.pri))
					aal_key->vid_ctl = 0;
				else
					aal_key->msk_top_vid = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.dei) {
				if(key->l2.vlan_otag.vlan_min.dei !=
					key->l2.vlan_otag.vlan_max.dei)
					return CA_E_NOT_SUPPORT;
				aal_key->top_dei= key->l2.vlan_otag.vlan_min.dei;
				aal_key->msk_top_dei = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.pri) {
				if(key->l2.vlan_otag.vlan_min.pri !=
					key->l2.vlan_otag.vlan_max.pri)
					return CA_E_NOT_SUPPORT;
				aal_key->top_802_1p = key->l2.vlan_otag.vlan_min.pri;
				aal_key->msk_top_vl_802_1p = 0;
			}
		}
		/* inner vlan fields */
		if (key_mask->l2_mask.vlan_itag) {
			if (key_mask->l2_mask.vlan_itag_mask.tpid) {
				if(key->l2.vlan_itag.vlan_min.tpid !=
					key->l2.vlan_itag.vlan_max.tpid)
					return CA_E_NOT_SUPPORT;
				CLS_INNER_TPID_ENC_SET(key->l2.vlan_itag.vlan_min.tpid, aal_key->inner_tpid_enc);
				aal_key->msk_inner_tpid_enc = 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.vid) {
				if(key->l2.vlan_itag.vlan_min.vid !=
					key->l2.vlan_itag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				aal_key->inner_vid = key->l2.vlan_itag.vlan_min.vid;
				aal_key->msk_inner_vid = 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.dei) {
				if(key->l2.vlan_itag.vlan_min.dei !=
					key->l2.vlan_itag.vlan_max.dei)
					return CA_E_NOT_SUPPORT;
				aal_key->inner_dei= key->l2.vlan_itag.vlan_min.dei;
				aal_key->msk_inner_dei = 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.pri) {
				if(key->l2.vlan_itag.vlan_min.pri !=
					key->l2.vlan_itag.vlan_max.pri)
					return CA_E_NOT_SUPPORT;
				aal_key->inner_802_1p = key->l2.vlan_itag.vlan_min.pri;
				aal_key->msk_inner_802_1p = 0;
			}
		}
	}
	/* ip fields */
	if (key_mask->ip) {
		if (key_mask->ip_mask.ip_valid) {
			aal_key->ip_vld = key->ip.ip_valid;
			aal_key->msk_ip_vld = 0;
		}
		if (key_mask->ip_mask.icmp_type == 0 && key_mask->ip_mask.igmp_type == 0 && key_mask->ip_mask.ip_version) {
			if (key->ip.ip_version == 4) /* ipv4 */
				aal_key->ip_ver = 0;
			else	/* ipv6 */
				aal_key->ip_ver = 1;
			aal_key->msk_ip_ver= 0;
		}
		if (key_mask->ip_mask.ip_protocol) {
			aal_key->ip_protocol = key->ip.ip_protocol;
			aal_key->msk_ip_protocol = 0;
		}
		if (key_mask->ip_mask.dscp) {
			aal_key->ip_dscp = key->ip.dscp;
			aal_key->msk_dscp = 0;
		}
		if (key_mask->ip_mask.ecn) {
			aal_key->ip_ecn = key->ip.ecn;
			aal_key->msk_ip_ecn = 0;
		}
#if !defined(CONFIG_ARCH_CORTINA_VENUS)
		if (key_mask->ip_mask.ip_sa) {
			/* set a different hw key field */
			if (key->ip.ip_sa.afi == CA_IPV4) {
				if (key->ip.ip_sa.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				aal_key->ip_addr.ip_addr_1 = aal_key->ip_addr.ip_addr_2 = aal_key->ip_addr.ip_addr_3 = 0;
				aal_key->msk_ip_addr_l = key->ip.ip_sa.addr_len;
				aal_key->ip_addr_type = 1;
				/* set ip_sa to select a right key type */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				aal_key->ip_sa.ip_addr_1 = aal_key->ip_sa.ip_addr_2 = aal_key->ip_sa.ip_addr_3 = 0;
				aal_key->msk_ip_sa_l = key->ip.ip_sa.addr_len;
			} else if (key->ip.ip_sa.afi == CA_IPV6) {
				if (key->ip.ip_sa.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				aal_key->ip_addr.ip_addr_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				aal_key->ip_addr.ip_addr_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				aal_key->ip_addr.ip_addr_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_addr_l = key->ip.ip_sa.addr_len & 0x7f;
				aal_key->msk_ip_addr_h = key->ip.ip_sa.addr_len >> 7;
				aal_key->ip_addr_type = 1;
				/* set ip_sa to select a right key type */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				aal_key->ip_sa.ip_addr_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				aal_key->ip_sa.ip_addr_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				aal_key->ip_sa.ip_addr_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_sa_l = key->ip.ip_sa.addr_len & 0x7f;
				aal_key->msk_ip_sa_h = key->ip.ip_sa.addr_len >> 7;
			} else
				return CA_E_PARAM;
		}
		if (key_mask->ip_mask.ip_da) {
			/* set a different hw key field */
			if (key->ip.ip_da.afi == CA_IPV4) {
				if (key->ip.ip_da.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				aal_key->ip_addr.ip_addr_1 = aal_key->ip_addr.ip_addr_2 = aal_key->ip_addr.ip_addr_3 = 0;
				aal_key->msk_ip_addr_l = key->ip.ip_da.addr_len;
				aal_key->ip_addr_type = 0;
				/* set ip_da to select a right key type */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				aal_key->ip_da.ip_addr_1 = aal_key->ip_da.ip_addr_2 = aal_key->ip_da.ip_addr_3 = 0;
				aal_key->msk_ip_da_l = key->ip.ip_da.addr_len;
				/* set dest_addr to select a right key type */
				if (key->ip.ip_da.addr_len == 32) {
					aal_key->dest_addr.dest_ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
					aal_key->dest_addr.dest_ip_addr.ip_addr_1 = 0;
					aal_key->dest_addr.dest_ip_addr.ip_addr_2 = 0;
					aal_key->dest_addr.dest_ip_addr.ip_addr_3 = 0;
					aal_key->msk_dest_addr = 0;
					aal_key->dest_addr_type = 1;
				}
			} else if (key->ip.ip_da.afi == CA_IPV6) {
				if (key->ip.ip_da.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				aal_key->ip_addr.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				aal_key->ip_addr.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				aal_key->ip_addr.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_addr_l = key->ip.ip_da.addr_len & 0x7f;
				aal_key->msk_ip_addr_h = key->ip.ip_da.addr_len >> 7;
				aal_key->ip_addr_type = 0;
				/* set ip_da to select a right key type */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				aal_key->ip_da.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				aal_key->ip_da.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				aal_key->ip_da.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_da_l = key->ip.ip_da.addr_len & 0x7f;
				aal_key->msk_ip_da_h = key->ip.ip_da.addr_len >> 7;
				/* set dest_addr to select a right key type */
				if (key->ip.ip_da.addr_len == 128) {
					aal_key->dest_addr.dest_ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
					aal_key->dest_addr.dest_ip_addr.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
					aal_key->dest_addr.dest_ip_addr.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
					aal_key->dest_addr.dest_ip_addr.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
					aal_key->msk_dest_addr = 0;
					aal_key->dest_addr_type = 1;
				}
			} else
				return CA_E_PARAM;
		}
#else
		if (key_mask->ip_mask.ip_sa) {
			/* set a different hw key field */
			if (key->ip.ip_sa.afi == CA_IPV4) {
				if (key->ip.ip_sa.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				aal_key->ip_addr.ip_addr_1 = aal_key->ip_addr.ip_addr_2 = aal_key->ip_addr.ip_addr_3 = 0;
				aal_key->msk_ip_addr_l = key->ip.ip_sa.addr_len;
				aal_key->ip_addr_type = 1;
				/* set ip_sa to select a right key type */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				aal_key->ip_sa.ip_addr_1 = aal_key->ip_sa.ip_addr_2 = aal_key->ip_sa.ip_addr_3 = 0;
				aal_key->msk_ip_sa_l = key->ip.ip_sa.addr_len;
				aal_key->ip_addr_ctrl = 0;
			} else if (key->ip.ip_sa.afi == CA_IPV6) {
				if (key->ip.ip_sa.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				aal_key->ip_addr.ip_addr_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				aal_key->ip_addr.ip_addr_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				aal_key->ip_addr.ip_addr_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_addr_l = key->ip.ip_sa.addr_len & 0x7f;
				aal_key->msk_ip_addr_h = key->ip.ip_sa.addr_len >> 7;
				aal_key->ip_addr_type = 1;
				/* set ip_sa to select a right key type */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				aal_key->ip_sa.ip_addr_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				aal_key->ip_sa.ip_addr_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				aal_key->ip_sa.ip_addr_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_sa_l = key->ip.ip_sa.addr_len & 0x7f;
				aal_key->msk_ip_sa_h = key->ip.ip_sa.addr_len >> 7;
				aal_key->ip_addr_ctrl = 0;
			} else
				return CA_E_PARAM;
		}
		if (key_mask->ip_mask.ip_sa_max) {
			/* set a different hw key field */
			if (key->ip.ip_sa_max.afi == CA_IPV4) {
				if (key->ip.ip_sa_max.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_sa to select a right key type */
				/* min sa ip */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				aal_key->ip_da.ip_addr_1 = aal_key->ip_da.ip_addr_2 = aal_key->ip_da.ip_addr_3 = 0;
				/* max sa ip */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa_max.ip_addr.ipv4_addr;
				aal_key->ip_sa.ip_addr_1 = aal_key->ip_sa.ip_addr_2 = aal_key->ip_sa.ip_addr_3 = 0;
				aal_key->ip_addr_ctrl = 2;
			} else if (key->ip.ip_sa_max.afi == CA_IPV6) {
				if (key->ip.ip_sa_max.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_sa to select a right key type */
				/* min sa ip */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				aal_key->ip_da.ip_addr_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				aal_key->ip_da.ip_addr_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				aal_key->ip_da.ip_addr_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
				/* max sa ip */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_sa_max.ip_addr.ipv6_addr[3];
				aal_key->ip_sa.ip_addr_1 = key->ip.ip_sa_max.ip_addr.ipv6_addr[2];
				aal_key->ip_sa.ip_addr_2 = key->ip.ip_sa_max.ip_addr.ipv6_addr[1];
				aal_key->ip_sa.ip_addr_3 = key->ip.ip_sa_max.ip_addr.ipv6_addr[0];
				aal_key->ip_addr_ctrl = 2;
			} else
				return CA_E_PARAM;
		}
		if (key_mask->ip_mask.ip_da) {
			/* set a different hw key field */
			if (key->ip.ip_da.afi == CA_IPV4) {
				if (key->ip.ip_da.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				aal_key->ip_addr.ip_addr_1 = aal_key->ip_addr.ip_addr_2 = aal_key->ip_addr.ip_addr_3 = 0;
				aal_key->msk_ip_addr_l = key->ip.ip_da.addr_len;
				aal_key->ip_addr_type = 0;
				/* set ip_da to select a right key type */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				aal_key->ip_da.ip_addr_1 = aal_key->ip_da.ip_addr_2 = aal_key->ip_da.ip_addr_3 = 0;
				aal_key->msk_ip_da_l = key->ip.ip_da.addr_len;
				aal_key->ip_addr_ctrl = 0;
				/* set dest_addr to select a right key type */
				if (key->ip.ip_da.addr_len == 32) {
					aal_key->dest_addr.dest_ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
					aal_key->dest_addr.dest_ip_addr.ip_addr_1 = 0;
					aal_key->dest_addr.dest_ip_addr.ip_addr_2 = 0;
					aal_key->dest_addr.dest_ip_addr.ip_addr_3 = 0;
					aal_key->msk_dest_addr = 0;
					aal_key->dest_addr_type = 1;
				}
			} else if (key->ip.ip_da.afi == CA_IPV6) {
				if (key->ip.ip_da.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_addr to select a right key type */
				aal_key->ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				aal_key->ip_addr.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				aal_key->ip_addr.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				aal_key->ip_addr.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_addr_l = key->ip.ip_da.addr_len & 0x7f;
				aal_key->msk_ip_addr_h = key->ip.ip_da.addr_len >> 7;
				aal_key->ip_addr_type = 0;
				/* set ip_da to select a right key type */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				aal_key->ip_da.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				aal_key->ip_da.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				aal_key->ip_da.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
				aal_key->msk_ip_da_l = key->ip.ip_da.addr_len & 0x7f;
				aal_key->msk_ip_da_h = key->ip.ip_da.addr_len >> 7;
				/* set dest_addr to select a right key type */
				if (key->ip.ip_da.addr_len == 128) {
					aal_key->dest_addr.dest_ip_addr.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
					aal_key->dest_addr.dest_ip_addr.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
					aal_key->dest_addr.dest_ip_addr.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
					aal_key->dest_addr.dest_ip_addr.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
					aal_key->msk_dest_addr = 0;
					aal_key->dest_addr_type = 1;
				}
			} else
				return CA_E_PARAM;
		}
		if (key_mask->ip_mask.ip_da_max) {
			/* set a different hw key field */
			if (key->ip.ip_da_max.afi == CA_IPV4) {
				if (key->ip.ip_da_max.addr_len > 32)
					return CA_E_PARAM;
				/* set ip_da to select a right key type */
				/* min da ip */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				aal_key->ip_da.ip_addr_1 = aal_key->ip_da.ip_addr_2 = aal_key->ip_da.ip_addr_3 = 0;
				/* max da ip */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_da_max.ip_addr.ipv4_addr;
				aal_key->ip_sa.ip_addr_1 = aal_key->ip_sa.ip_addr_2 = aal_key->ip_sa.ip_addr_3 = 0;
				aal_key->ip_addr_ctrl = 1;
			} else if (key->ip.ip_da_max.afi == CA_IPV6) {
				if (key->ip.ip_da_max.addr_len > 128)
					return CA_E_PARAM;
				/* set ip_da to select a right key type */
				/* min da ip */
				aal_key->ip_da.ip_addr_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				aal_key->ip_da.ip_addr_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				aal_key->ip_da.ip_addr_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				aal_key->ip_da.ip_addr_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
				/* max da ip */
				aal_key->ip_sa.ip_addr_0 = key->ip.ip_da_max.ip_addr.ipv6_addr[3];
				aal_key->ip_sa.ip_addr_1 = key->ip.ip_da_max.ip_addr.ipv6_addr[2];
				aal_key->ip_sa.ip_addr_2 = key->ip.ip_da_max.ip_addr.ipv6_addr[1];
				aal_key->ip_sa.ip_addr_3 = key->ip.ip_da_max.ip_addr.ipv6_addr[0];
				aal_key->ip_addr_ctrl = 1;
			} else
				return CA_E_PARAM;
		}
#endif
		if (key_mask->ip_mask.fragment) {
			if (key->ip.fragment == 0)
				aal_key->ip_fragment = 0;
			else
				aal_key->ip_fragment = 1;
			aal_key->msk_ip_fragment = 0;
		}
		if (key_mask->ip_mask.have_options) {
			aal_key->ip_options = key->ip.have_options;
			aal_key->msk_ip_options = 0;
		}
		if (key_mask->ip_mask.flow_label) {
			aal_key->ipv6_flow_lbl = key->ip.flow_label;
			aal_key->msk_ipv6_flow_lbl = 0;
		}
		if (key_mask->ip_mask.ext_header) {
			switch (key->ip.ext_header) {
				case CA_IPV6_EXT_HEADER_HOP_BY_HOP:
					aal_key->ipv6_hbh = 1;
					aal_key->msk_ipv6_hbh = 0;
					break;
				case CA_IPV6_EXT_HEADER_ROUTING:
					aal_key->ipv6_rh = 1;
					aal_key->msk_ipv6_rh = 0;
					break;
				case CA_IPV6_EXT_HEADER_DESTINATION:
					aal_key->ipv6_doh = 1;
					aal_key->msk_ipv6_doh = 0;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		if (key_mask->ip_mask.icmp_type) {
			if ((key->ip.icmp_type >= 130 &&  key->ip.icmp_type <= 132) || key->ip.icmp_type == 143) {
				/* mld */
				aal_key->icmp_vld = 7;
			} else if ((key->ip.icmp_type >= 133 &&  key->ip.icmp_type <= 137) || key->ip.icmp_type == 146 || key->ip.icmp_type == 147) {
				/* ndp */
				aal_key->icmp_vld = 3;
			} else {
				if (key_mask->ip_mask.ip_version && key->ip.ip_version ==  4)
					aal_key->icmp_vld = 4;
				else if (key_mask->ip_mask.ip_version && key->ip.ip_version == 6)
					aal_key->icmp_vld = 5;
				else
					return CA_E_PARAM;
			}
			aal_key->msk_icmp_vld = 0;
			aal_key->icmp_type = key->ip.icmp_type;
			aal_key->msk_icmp_type = 0;
		}
		if (key_mask->ip_mask.igmp_type) {
			if (key_mask->ip_mask.ip_version && key->ip.ip_version == 6)
				return CA_E_PARAM;
			/* ipv4 igmp */
			aal_key->icmp_vld = 6;
			aal_key->msk_icmp_vld = 0;
			aal_key->icmp_type = key->ip.igmp_type;
			aal_key->msk_icmp_type = 0;
		}
		if (key_mask->ip_mask.is_multicast) {
			aal_key->ip_da_mc = 1;
			aal_key->msk_ip_da_mc = 0;
		}
	}
	/* l4 fields */
	if (key_mask->l4) {
		if (key_mask->l4_mask.l4_valid) {
			aal_key->l4_vld = key->l4.l4_valid;
			aal_key->msk_l4_vld = 0;
		}
		if (key_mask->l4_mask.tcp_flags) {
			aal_key->tcp_rdp_ctrl = key->l4.tcp_flags & key_mask->l4_mask.tcp_flags;
#if defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_ARCH_CORTINA_VENUS)
			/*ATTENTATION: Even we configure tcp_falgs as ACK bit only, CLS will hit as long as it include ack(ex: syn+ack).*/
			aal_key->msk_tcp_rdp_ctrl = 0;
#else
			if((key_mask->l4_mask.tcp_flags == 0xfff) || (key_mask->l4_mask.tcp_flags != key->l4.tcp_flags))
				aal_key->msk_tcp_rdp_ctrl = 2;	//match the tcp_rdp_ctrl bits use AND Match
			else
				aal_key->msk_tcp_rdp_ctrl = 1;	//match the tcp_rdp_ctrl bits use OR Match
#endif
#else	//not CONFIG_LUNA_G3_SERIES
			aal_key->msk_tcp_rdp_ctrl = 2;
#endif	//CONFIG_LUNA_G3_SERIES
		}
		if (key_mask->l4_mask.src_port && key_mask->l4_mask.dst_port &&
				(key->l4.src_port.min == key->l4.src_port.max) &&
				(key->l4.dst_port.min == key->l4.dst_port.max)) {
			/* one l4 src port & one l4 dst port */
			aal_key->l4_sp = key->l4.src_port.min;
			aal_key->l4_dp = key->l4.dst_port.min;
			aal_key->msk_l4_port = 0;
			aal_key->l4_vld = 1;
			aal_key->msk_l4_vld = 0;
		} else if (key_mask->l4_mask.src_port && !key_mask->l4_mask.dst_port) {
			/* one l4 src port or range */
			aal_key->l4_sp = key->l4.src_port.min;
			aal_key->l4_dp = key->l4.src_port.max;
			aal_key->msk_l4_port = 1;
			aal_key->l4_vld = 1;
			aal_key->msk_l4_vld = 0;
		} else if (!key_mask->l4_mask.src_port && key_mask->l4_mask.dst_port) {
			/* one l4 dst port or range */
			aal_key->l4_sp = key->l4.dst_port.min;
			aal_key->l4_dp = key->l4.dst_port.max;
			aal_key->msk_l4_port = 2;
			aal_key->l4_vld = 1;
			aal_key->msk_l4_vld = 0;
		} else if (key_mask->l4_mask.src_port && key_mask->l4_mask.dst_port) {
			/* l4 src port range */
			/* config l3 cam */
			memset(&cam_sport, 0, sizeof(l3_cam_sport_tbl_entry_t));
			cam_sport.vld = 1;
			cam_sport.sport_low = key->l4.src_port.min;
			cam_sport.sport_hi = key->l4.src_port.max;
			if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_SPORT, &cam_sport, &rslt_idx))
				return CA_E_CONFIG;
			/* update rule info */
			rule->type |= CLS_RUL_INFO_CAM_SPORT;
			rule->hw_info.l4_sport.rslt_idx = rslt_idx;
			aal_key->l4_sp = 0x1 << rslt_idx;

			/* l4 dst port range */
			/* config l3 cam */
			memset(&cam_dport, 0, sizeof(l3_cam_dport_tbl_entry_t));
			cam_dport.vld = 1;
			cam_dport.dport_low = key->l4.dst_port.min;
			cam_dport.dport_hi = key->l4.dst_port.max;
			if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_DPORT, &cam_dport, &rslt_idx))
				return CA_E_CONFIG;
			/* update rule info */
			rule->type |= CLS_RUL_INFO_CAM_DPORT;
			rule->hw_info.l4_dport.rslt_idx = rslt_idx;
			aal_key->l4_dp = 0x1 << rslt_idx;
			aal_key->msk_l4_port = 3;
			aal_key->l4_vld = 1;
			aal_key->msk_l4_vld = 0;
		}
	}
#if !defined(CONFIG_ARCH_CORTINA_VENUS)
	/* ext fields */
	if (key_mask->ext) {
		ca_int16_t i, avail_cnt, len;

		if (key->ext.length > 4 || key->ext.length <= 0)
			return CA_E_PARAM;

		avail_cnt = aal_table_get_avail_count(AAL_TABLE_L3_CAM_SPCL_HDR);
		if (avail_cnt < (key->ext.length + 1)/2)
			return CA_E_UNAVAIL;
		/* config l3 cam */
		memset(&cam_spcl_hdr, 0, sizeof(l3_cam_spcl_hdr_tbl_entry_t));
		cam_spcl_hdr.vld = 1;
		cam_spcl_hdr.type = L3_CAM_SPCL_HDR_PROG;
		for (i = 0, len = key->ext.length; i < key->ext.length; i++) {
			if (i%2) {
				cam_spcl_hdr.mask |= key->ext.mask[i];
				cam_spcl_hdr.key |= key->ext.data[i];
			} else {
				cam_spcl_hdr.mask |= key->ext.mask[i] << 8;
				cam_spcl_hdr.key |= key->ext.data[i] << 8;
			}
			if (i/2)
				cam_spcl_hdr.ms_vld = 0;
			else
				cam_spcl_hdr.ms_vld = 1;
			len--;
			if (len <= 0 || i%2) {
				if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_SPCL_HDR, &cam_spcl_hdr, &rslt_idx))
					return CA_E_CONFIG;
				cam_spcl_hdr.mask = 0;
				cam_spcl_hdr.key = 0;
				rule->hw_info.spcl_hdr.rslt_idx |= 0x1 << rslt_idx;
			}
		}
		CA_NE_REG_WRITE(key->ext.offset, L3FE_PP_OFFSET_HDR);
		/* update rule info */
		rule->type |= CLS_RUL_INFO_CAM_SPCL_HDR;

		aal_key->spcl_pkt_hdr_mtch = rule->hw_info.spcl_hdr.rslt_idx;
		aal_key->spcl_pkt_hdr_mtch_msk = 0;
	}
#else
	/* ext fields */
	if (key_mask->ext) {
		ca_int16_t i, avail_cnt, len;

		if (key->ext.length > 4 || key->ext.length <= 0)
			return CA_E_PARAM;
		if (key->ext.offset > 0x1ff)
			return CA_E_PARAM;

		avail_cnt = aal_table_get_avail_count(AAL_TABLE_L3_CAM_SPCL_HDR);
		if (avail_cnt < (key->ext.length + 1)/2)
			return CA_E_UNAVAIL;
		/* config l3 cam */
		memset(&cam_spcl_hdr, 0, sizeof(l3_cam_spcl_hdr_tbl_entry_t));
		cam_spcl_hdr.vld = 1;
		switch(key->ext.starting_location) {
			case CA_CLASSIFIER_OFFSET_START_L2:
				cam_spcl_hdr.type = L3_CAM_SPCL_HDR_PROG;
				tmp = CA_NE_REG_READ(L3FE_PP_OFFSET_HDR);
				tmp = tmp & 0xfffffe00 | key->ext.offset;
				CA_NE_REG_WRITE(tmp, L3FE_PP_OFFSET_HDR);
				break;
			case CA_CLASSIFIER_OFFSET_START_L3:
				cam_spcl_hdr.type = L3_CAM_SPCL_HDR_L3_PLUS;
				cam_spcl_hdr.ext_type = 1;
				tmp = CA_NE_REG_READ(L3FE_PP_OFFSET_HDR);
				tmp = tmp & 0x1ff | (key->ext.offset-4) << 16;
				CA_NE_REG_WRITE(tmp, L3FE_PP_OFFSET_HDR);
				break;
			case CA_CLASSIFIER_OFFSET_START_L4:
				cam_spcl_hdr.type = L3_CAM_SPCL_HDR_L4_PLUS_A;
				cam_spcl_hdr.ext_type = 4;
				CA_NE_REG_WRITE(key->ext.offset-4, L3FE_PP_L7_PROG_OFFSET_HDR_0);
				break;
			default:
				return CA_E_NOT_SUPPORT;
		}
		for (i = 0, len = key->ext.length; i < key->ext.length; i++) {
			if (i%2) {
				cam_spcl_hdr.mask |= key->ext.mask[i];
				cam_spcl_hdr.key |= key->ext.data[i];
			} else {
				cam_spcl_hdr.mask |= key->ext.mask[i] << 8;
				cam_spcl_hdr.key |= key->ext.data[i] << 8;
			}
			if (i/2)
				cam_spcl_hdr.ms_vld = 0;
			else
				cam_spcl_hdr.ms_vld = 1;
			len--;
			if (len <= 0 || i%2) {
				if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_SPCL_HDR, &cam_spcl_hdr, &rslt_idx))
					return CA_E_CONFIG;
				cam_spcl_hdr.mask = 0;
				cam_spcl_hdr.key = 0;
				rule->hw_info.spcl_hdr.rslt_idx |= 0x1 << rslt_idx;
			}
		}
		/* update rule info */
		rule->type |= CLS_RUL_INFO_CAM_SPCL_HDR;

		aal_key->spcl_pkt_hdr_mtch = rule->hw_info.spcl_hdr.rslt_idx;
		aal_key->spcl_pkt_hdr_mtch_msk = 0;
	}
#endif
	/* packet_length */
	if (key_mask->packet_length) {
		/* config l3 cam */
		if (key->packet_length_low > 0x3fff || key->packet_length_high > 0x3fff)
			return CA_E_PARAM;
		memset(&cam_pkt_len, 0, sizeof(l3_cam_pkt_len_tbl_entry_t));
		cam_pkt_len.vld = 1;
		cam_pkt_len.pkt_len_low = key->packet_length_low;
		cam_pkt_len.pkt_len_hi = key->packet_length_high;
		if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_PKT_LEN, &cam_pkt_len, &rslt_idx))
			return CA_E_CONFIG;
		/* update rule info */
		rule->type |= CLS_RUL_INFO_CAM_PKT_LEN;
		rule->hw_info.pkt_len.rslt_idx = rslt_idx;
		/* update aal key */
		aal_key->pktlen_rng_match_vec = 0x1 << rslt_idx;
		aal_key->pktlen_rng_match_mask = 0;
	}
	return CA_E_OK;
}

static ca_status_t cls_fib_entry_update(
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	ca_classifier_action_t	 *action,
	cls_action_t *aal_action)
{
	ca_uint8_t o_vid_cnt, i_vid_cnt, o_tpid_enc, i_tpid_enc, o_dei, i_dei, vlan_mode=0;
	ca_uint8_t keep_original = 1;
	ca_l3_nexthop_id_t nexthop_id;

	if (action == NULL || aal_action == NULL)
		return CA_E_ERROR;

	/* ignore all fields */
	memset(aal_action, 0, sizeof(cls_action_t));

	switch (action->forward) {
		case CA_CLASSIFIER_FORWARD_DENY:
			aal_action->permit = 0;
			aal_action->dpid_vld = 1;
			aal_action->t2_ctrl_vld = 1;
			aal_action->t2_ctrl = 0xf;
			aal_action->t3_ctrl_vld = 1;
			aal_action->t3_ctrl = 0xf;
			aal_action->t4_ctrl_vld = 1;
			aal_action->t4_ctrl = 0xf;
			aal_action->t5_ctrl_vld = 1;
			aal_action->t5_ctrl = 0xf;
			break;
		case CA_CLASSIFIER_FORWARD_PORT:
			aal_action->mc = 0;
			aal_action->mcgid = PORT_ID(action->dest.port);
			aal_action->dpid_pri = 1;
			aal_action->permit = 1;
			aal_action->dpid_vld = 1;
			aal_action->t2_ctrl_vld = 1;
			aal_action->t2_ctrl = 0xf;
			aal_action->t3_ctrl_vld = 1;
			aal_action->t3_ctrl = 0xf;
			aal_action->t4_ctrl_vld = 1;
			aal_action->t4_ctrl = 0xf;
			aal_action->t5_ctrl_vld = 1;
			aal_action->t5_ctrl = 0xf;
			if (!(aal_action->mcgid >= AAL_LPORT_CPU_0 && aal_action->mcgid <= AAL_LPORT_CPU_7)) {
				aal_action->deepq = 1;
			}
#if defined(CONFIG_LUNA_G3_SERIES)
			if (aal_action->mcgid >= AAL_LPORT_CPU_0 && aal_action->mcgid <= AAL_LPORT_CPU_1) {
				aal_action->mdata_w_1 = 0;
				aal_action->mdata_w_vld_1 = 0xf;
			}
#endif
			break;
		case CA_CLASSIFIER_FORWARD_INTERFACE:
			aal_action->mdata_w_1 = action->dest.intf & 0xff;
			aal_action->mdata_w_vld_1 = 0x3;
			aal_action->l3_egress_if_vld = 1;
			aal_action->t3_ctrl_vld = 1;
			aal_action->t3_ctrl = 0xf;
			aal_action->t4_ctrl_vld = 1;
			aal_action->t4_ctrl = 0xf;
			keep_original = 0;
			break;
		case CA_CLASSIFIER_FORWARD_FE:
			keep_original = 0;
			if (key_mask != NULL && key_mask->ip && key_mask->ip_mask.is_multicast) {
				aal_action->t2_ctrl_vld = 1;
				aal_action->t2_ctrl = l3fe_main_hash_profile_mc;
			}
			break;
		case CA_CLASSIFIER_FORWARD_TUNNEL:
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
			return CA_E_NOT_SUPPORT;
#else
			if (get_nexthop_id_by_tunnel_id(action->dest.tunnel_id, &nexthop_id) != CA_E_OK) {
				ca_printf(KERN_INFO "%s: Fail to find nexthop_id by tunnel_id %d.\n", __func__, action->dest.tunnel_id);
				return CA_E_PARAM;
			}
			aal_action->mdata_w_0 = nexthop_id;
			aal_action->mdata_w_vld_0 = 0x7;
			aal_action->l3_egress_if_vld = 1;
			aal_action->t3_ctrl_vld = 1;
			aal_action->t3_ctrl = 0xf;
			aal_action->t4_ctrl_vld = 1;
			aal_action->t4_ctrl = 0xe; /* directly use the HashLite entry index */
			keep_original = 0;
			break;
#endif
		default:
			return CA_E_NOT_SUPPORT;
	}

	/* set flow_id */
	if (action->options.masks.action_handle) {
		if (action->options.action_handle.flow_id > CA_AAL_MAX_FLOW_ID || action->options.action_handle.flow_id > CA_AAL_L3_MAX_FLOW_ID)
			return CA_E_PARAM;
		aal_action->pol_vld = 1;
		/* both L2, L3 Policer enable */
		aal_action->pol_en = 3;
		aal_action->pol_base = action->options.action_handle.flow_id;
		aal_action->pol_sel = 2;
	}
	/* sw_shaper_id */
	if (action->options.masks.sw_shaper_id) {
		if (action->forward == CA_CLASSIFIER_FORWARD_PORT || action->options.masks.priority)
			return CA_E_PARAM;
		/* sw_shaper_id = queue id, LDPID 0x20-0x3f (each with 8 VoQs, 256 queues total) */
		if (action->options.sw_shaper_id >= 256)
			return CA_E_PARAM;
		aal_action->mc = 0;
		aal_action->mcgid = 0x20 + action->options.sw_shaper_id/8;
		aal_action->dpid_pri = 1;
		aal_action->permit = 1;
		aal_action->dpid_vld = 1;
		aal_action->cos = action->options.sw_shaper_id%8;
		aal_action->cos_sel = 1;

		aal_action->t2_ctrl_vld = 1;
		aal_action->t2_ctrl = 0xf;
		aal_action->t3_ctrl_vld = 1;
		aal_action->t3_ctrl = 0xf;
		aal_action->t4_ctrl_vld = 1;
		aal_action->t4_ctrl = 0xf;
		aal_action->t5_ctrl_vld = 1;
		aal_action->t5_ctrl = 0xf;

		aal_action->deepq = 1;
	}
	/* l2 fields */
	if (action->options.masks.mac_da) {
		aal_action->mac_da_0 = action->options.mac_da[5];
		aal_action->mac_da_1 = action->options.mac_da[4];
		aal_action->mac_da_2 = action->options.mac_da[3];
		aal_action->mac_da_3 = action->options.mac_da[2];
		aal_action->mac_da_4 = action->options.mac_da[1];
		aal_action->mac_da_5 = action->options.mac_da[0];
		aal_action->mac_da_vld = 1;
		keep_original = 0;
	}
	/* priority */
	if (action->options.masks.priority) {
		aal_action->cos = action->options.priority;
		aal_action->cos_sel = 1;
	}
	/* vlan fields */
	if (action->options.masks.outer_vlan_act || action->options.masks.outer_dei || action->options.masks.outer_tpid ||
		action->options.masks.inner_vlan_act || action->options.masks.inner_dei || action->options.masks.inner_tpid) {

		if ((action->options.masks.outer_vlan_act && action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_SET &&
			action->options.masks.inner_vlan_act && action->options.inner_vlan_act != CA_CLASSIFIER_VLAN_ACTION_SET) ||
			(action->options.masks.outer_vlan_act && action->options.outer_vlan_act != CA_CLASSIFIER_VLAN_ACTION_SET &&
			action->options.masks.inner_vlan_act && action->options.inner_vlan_act == CA_CLASSIFIER_VLAN_ACTION_SET) )
			return CA_E_PARAM;

		if (key == NULL || key_mask == NULL)
			return CA_E_ERROR;
		keep_original = 0;
		/* get vlan info for the incomming packet from key */
		o_vid_cnt = i_vid_cnt = 0;
		o_tpid_enc = i_tpid_enc = 1;
		o_dei = i_dei = 0;
		if (key_mask->l2) {
			if (key_mask->l2_mask.vlan_otag) {
				/* get outter tpid */
				if (key_mask->l2_mask.vlan_otag_mask.tpid) {
					CLS_TOP_TPID_ENC_SET(key->l2.vlan_otag.vlan_min.tpid, o_tpid_enc);
				}
				/* get outter dei */
				if (key_mask->l2_mask.vlan_otag_mask.dei) {
					o_dei = key->l2.vlan_otag.vlan_min.dei;
				}
			}

			if (key_mask->l2_mask.vlan_itag) {
				/* get inner tpid */
				if (key_mask->l2_mask.vlan_itag_mask.tpid) {
					CLS_INNER_TPID_ENC_SET(key->l2.vlan_itag.vlan_min.tpid, i_tpid_enc);
				}
				/* get inner dei */
				if (key_mask->l2_mask.vlan_itag_mask.dei) {
					i_dei = key->l2.vlan_itag.vlan_min.dei;
				}
			}
		}

#if !defined(CONFIG_ARCH_CORTINA_VENUS)
		/* set outer vlan action for outgoing packet */
		if (action->options.masks.outer_vlan_act) {
			switch(action->options.outer_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					aal_action->top_vid = action->options.outer_vid;
					o_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_POP:
					o_vid_cnt = 0;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* set inner vlan action for outgoing packet */
		if (action->options.masks.inner_vlan_act) {
			switch(action->options.inner_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					if (!action->options.masks.outer_vlan_act || (action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_POP))
						return CA_E_NOT_SUPPORT;
					aal_action->inner_vid = action->options.inner_vid;
					i_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_POP:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* cls fib supports 2 vlan tags at most */
		if ((o_vid_cnt + i_vid_cnt) > 2)
			return CA_E_PARAM;
		aal_action->vlan_cnt = o_vid_cnt + i_vid_cnt;
		aal_action->vlan_vld = 1;

		/* set outer tpid */
		if (action->options.masks.outer_tpid) {
			if (!o_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_TOP_TPID_ENC_SET(action->options.outer_tpid, aal_action->top_tpid_enc);
		} else
			aal_action->top_tpid_enc = o_tpid_enc;
		/* set inner tpid */
		if (action->options.masks.inner_tpid) {
			if (!i_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_INNER_TPID_ENC_SET(action->options.inner_tpid, aal_action->inner_tpid_enc);
		} else
			aal_action->inner_tpid_enc = i_tpid_enc;
#else
		/* set outer vlan action for outgoing packet */
		if (action->options.masks.outer_vlan_act) {
			switch(action->options.outer_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					aal_action->top_vid = action->options.outer_vid;
					o_vid_cnt = 1;
					vlan_mode = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_POP:
					/*  vlan stacking operation mode */
					aal_action->vlan_cnt = 2;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
					/*  vlan stacking operation mode */
					aal_action->top_vid = action->options.outer_vid;
					aal_action->vlan_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
					/*  vlan stacking operation mode */
					aal_action->top_vid = action->options.outer_vid;
					aal_action->vlan_cnt = 3;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
					/*  vlan stacking operation mode*/
					aal_action->vlan_cnt = 0;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* set inner vlan action for outgoing packet */
		if (action->options.masks.inner_vlan_act) {
			switch(action->options.inner_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					if (!action->options.masks.outer_vlan_act)
						return CA_E_NOT_SUPPORT;
					aal_action->inner_vid = action->options.inner_vid;
					i_vid_cnt = 1;
					vlan_mode = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
					/*  vlan stacking operation mode */
					aal_action->inner_vid = action->options.inner_vid;
					aal_action->inner_vlan_cmd = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_POP:
					/*  vlan stacking operation mode */
					aal_action->inner_vlan_cmd = 2;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
					/*  vlan stacking operation mode */
					aal_action->inner_vid = action->options.inner_vid;
					aal_action->inner_vlan_cmd = 3;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
					/*  vlan stacking operation mode*/
					aal_action->inner_vlan_cmd = 0;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* cls fib supports 2 vlan tags at most */
		if ((o_vid_cnt + i_vid_cnt) > 2)
			return CA_E_PARAM;
		/* vlan_mod == vlan set mode */
		if (vlan_mode)
			aal_action->vlan_cnt = o_vid_cnt + i_vid_cnt;
		/* set vlan mode */
		aal_action->vlan_vld = vlan_mode;

		/* set outer tpid */
		if (action->options.masks.outer_tpid) {
			aal_action->top_tpid_sel = 3;
			CLS_TOP_TPID_ENC_SET(action->options.outer_tpid, aal_action->top_tpid_enc);
		} else
			aal_action->top_tpid_sel = 1;
		/* set inner tpid */
		if (action->options.masks.inner_tpid) {
			aal_action->inner_tpid_sel = 3;
			CLS_INNER_TPID_ENC_SET(action->options.inner_tpid, aal_action->inner_tpid_enc);
		} else
			aal_action->inner_tpid_sel = 1;
#endif
		/* set outer dei */
		if (action->options.masks.outer_dei) {
			aal_action->top_dei_sel = 3;
			aal_action->top_dei = action->options.outer_dei;
		} else
			aal_action->top_dei_sel = 1;
		/* set inner dei */
		if (action->options.masks.inner_dei) {
			aal_action->inner_dei_sel = 3;
			aal_action->inner_dei = action->options.inner_dei;
		} else
			aal_action->inner_dei_sel = 1;
	}
	if (action->options.masks.inner_dot1p) {
		/* set hdr_i_out.inner_802_1p = fib.inner_802_1p */
		aal_action->inner_802_1p_sel = 2;
		aal_action->inner_802_1p = action->options.inner_dot1p;
		keep_original = 0;
	}
	if (action->options.masks.outer_dot1p) {
		/* set hdr_i_out.top_802_1p = fib.top_802_1p */
		aal_action->top_802_1p_sel = 2;
		aal_action->top_802_1p = action->options.outer_dot1p;
		keep_original = 0;
	}
	/* l3 fields */
	if (action->options.masks.dscp) {
		/* set RSLT_DSCP */
		aal_action->ip_dscp_sel = 1;
		aal_action->ip_dscp = action->options.dscp;
		aal_action->ip_dscp_update_en = 3;
		keep_original = 0;
	}
	/* sw_id */
	if (action->options.masks.sw_id) {
		/* mdata[55:32], 24 bits */
		aal_action->mdata_w_vld_3 = 0x3;
		aal_action->mdata_w_3 = action->options.sw_id[1] & 0xff;
		aal_action->mdata_w_vld_2 = 0xf;
		aal_action->mdata_w_2 = action->options.sw_id[0];
	}

	if (keep_original == 1)
		aal_action->keep_orig_pkt = 1;
	else
		aal_action->keep_orig_pkt = 0;

	aal_action->keep_orig_pkt_vld = 1;

	return CA_E_OK;
}

#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
static ca_status_t cls_type_1_default_set (
	ca_uint32_t hash_profile_id,
	ca_uint8_t valid)
{
	ca_status_t rc = CA_E_OK;
	cls_action_t aal_action;

	if ((hash_profile_id != l3fe_main_hash_profile_wan && hash_profile_id != l3fe_main_hash_profile_lan) || valid > 1)
		return CA_E_PARAM;

	if (hash_profile_id == l3fe_main_hash_profile_wan) {
		if (valid && wan_hash_cnt >= 1) {
			wan_hash_cnt++;
			return CA_E_OK;
		}
		if (!valid && wan_hash_cnt > 1) {
			wan_hash_cnt--;
			return CA_E_OK;
		}
	}
	if (hash_profile_id == l3fe_main_hash_profile_lan) {
		if (valid && lan_hash_cnt >= 1) {
			lan_hash_cnt++;
			return CA_E_OK;
		}
		if (!valid && lan_hash_cnt > 1) {
			lan_hash_cnt--;
			return CA_E_OK;
		}
	}

	/* ignore all fields */
	memset(&aal_action, 0, sizeof(cls_action_t));

	if (valid ) {
		aal_action.t2_ctrl_vld = 1;
		aal_action.t2_ctrl = hash_profile_id;
	}

	if (hash_profile_id == l3fe_main_hash_profile_wan) {
		if (CA_E_OK != (rc = aal_l3_cls_default_set(0, l3fe_classifier_profile_wan, CL_RSLT_TYPE_1, &aal_action)))
			return CA_E_ERROR;
		(valid) ? wan_hash_cnt++ : wan_hash_cnt--;
	} else {
		if (CA_E_OK != (rc = aal_l3_cls_default_set(0, l3fe_classifier_profile_lan, CL_RSLT_TYPE_1, &aal_action)))
			return CA_E_ERROR;
		(valid) ? lan_hash_cnt++ : lan_hash_cnt--;
	}

	return CA_E_OK;
}

static ca_status_t cls_hash_key_update(
    ca_classifier_key_t      *key,
	ca_classifier_key_mask_t *key_mask,
	aal_hash_key_t           *hash_key,
	aal_hash_mask_t          *hash_mask,
	cls_rule_info_t          *rule)
{
	l3_cam_sport_tbl_entry_t cam_sport;
	l3_cam_dport_tbl_entry_t cam_dport;
	l3_cam_spcl_hdr_tbl_entry_t cam_spcl_hdr;
	ca_uint32_t rslt_idx;

	if (key == NULL || key_mask == NULL || hash_key == NULL || hash_mask == NULL || rule == NULL)
		return CA_E_ERROR;

	if (key_mask->dst_intf || key_mask->l2_mask.subtype || key_mask->l2_mask.is_multicast || key_mask->dest_port
		|| key_mask->key_handle || key_mask->l2_mask.is_length
		|| key_mask->ip_mask.is_multicast|| key_mask->ip_mask.ip_sa_max || key_mask->ip_mask.ip_da_max
		|| key_mask->l4_mask.l4_valid)
		return CA_E_NOT_SUPPORT;

	memset(hash_key, 0, sizeof(aal_hash_key_t));
	hash_mask_unmask(hash_mask);

	if (key_mask->src_port) {
		hash_mask->lspid = 0;
		hash_key->lspid = PORT_ID(key->src_port);
	} else
		/* must set src_port */
		return CA_E_PARAM;

	if (key_mask->orig_src_port) {
		hash_mask->orig_lspid = 0;
		hash_key->orig_lspid = PORT_ID(key->orig_src_port);
	}

	if (key_mask->src_intf) {
		hash_mask->mdata &= ~(0xff << 24);
		hash_key->mdata |= key->src_intf << 24;
	}
	/* l2 fields */
	if (key_mask->l2) {
		if (key_mask->l2_mask.ethertype) {
			hash_mask->ethertype = 0;
			hash_key->ethertype = key->l2.ethertype;
		}
		if (key_mask->l2_mask.mac_sa) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_sa.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_sa.mac_max[0],
				CA_ETH_ADDR_LEN))
				/* not support mac range */
				return CA_E_NOT_SUPPORT;
			else {
				/* one src mac */
				hash_mask->mac_sa = 0;
				hash_key->mac_sa_0 = key->l2.mac_sa.mac_min[5];
				hash_key->mac_sa_1 = key->l2.mac_sa.mac_min[4];
				hash_key->mac_sa_2 = key->l2.mac_sa.mac_min[3];
				hash_key->mac_sa_3 = key->l2.mac_sa.mac_min[2];
				hash_key->mac_sa_4 = key->l2.mac_sa.mac_min[1];
				hash_key->mac_sa_5 = key->l2.mac_sa.mac_min[0];
			}
		}
		if (key_mask->l2_mask.mac_da) {
			if(memcmp((ca_uint8_t *) &key->l2.mac_da.mac_min[0],
				(ca_uint8_t *) &key->l2.mac_da.mac_max[0],
				CA_ETH_ADDR_LEN))
				/* not support mac range */
				return CA_E_NOT_SUPPORT;
			else {
				/* one dst mac */
				hash_mask->mac_da = 0;
				hash_key->mac_da_0 = key->l2.mac_da.mac_min[5];
				hash_key->mac_da_1 = key->l2.mac_da.mac_min[4];
				hash_key->mac_da_2 = key->l2.mac_da.mac_min[3];
				hash_key->mac_da_3 = key->l2.mac_da.mac_min[2];
				hash_key->mac_da_4 = key->l2.mac_da.mac_min[1];
				hash_key->mac_da_5 = key->l2.mac_da.mac_min[0];
			}
		}
		if (key_mask->l2_mask.cfm_opcode) {
			switch(key->l2.cfm_opcode) {
				case CL_CFM_CCM:
					hash_key->spcl_pkt_enc = 0x22;
					break;
				case CL_CFM_LBR:
					hash_key->spcl_pkt_enc = 0x23;
					break;
				case CL_CFM_LBM:
					hash_key->spcl_pkt_enc = 0x13;
					break;
				case CL_CFM_LTR:
					hash_key->spcl_pkt_enc = 0x24;
					break;
				case CL_CFM_LTM:
					hash_key->spcl_pkt_enc = 0x25;
					break;
				case CL_CFM_LMM:
					hash_key->spcl_pkt_enc = 0x14;
					break;
				case CL_CFM_1DM:
					hash_key->spcl_pkt_enc = 0x15;
					break;
				case CL_CFM_DMR:
					hash_key->spcl_pkt_enc = 0x16;
					break;
				case CL_CFM_DMM:
					hash_key->spcl_pkt_enc = 0x15;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
			hash_mask->spcl_pkt_enc = 0;
		}
		/* vlan count */
		if (key_mask->l2_mask.vlan_count) {
			hash_key->vlan_cnt = key->l2.vlan_count;
			hash_mask->vlan_cnt = 0;
		}
		/* vlan fields */
		if (key_mask->l2_mask.vlan_otag) {
			if (key_mask->l2_mask.vlan_otag_mask.tpid) {
				if(key->l2.vlan_otag.vlan_min.tpid !=
					key->l2.vlan_otag.vlan_max.tpid)
					return CA_E_NOT_SUPPORT;
				CLS_TOP_TPID_ENC_SET(key->l2.vlan_otag.vlan_min.tpid, hash_key->top_tpid_enc);
				hash_mask->top_tpid_enc = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.vid) {
				if(key->l2.vlan_otag.vlan_min.vid !=
					key->l2.vlan_otag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				hash_key->top_vid = key->l2.vlan_otag.vlan_min.vid;
				hash_mask->top_vid = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.dei) {
				if(key->l2.vlan_otag.vlan_min.dei !=
					key->l2.vlan_otag.vlan_max.dei)
					return CA_E_NOT_SUPPORT;
				hash_key->top_dei= key->l2.vlan_otag.vlan_min.dei;
				hash_mask->top_dei= 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.pri) {
				if(key->l2.vlan_otag.vlan_min.pri !=
					key->l2.vlan_otag.vlan_max.pri)
					return CA_E_NOT_SUPPORT;
				hash_key->top_8021p = key->l2.vlan_otag.vlan_min.pri;
				hash_mask->top_8021p = 0;
			}
		}
		/* inner vlan fields */
		if (key_mask->l2_mask.vlan_itag) {
			if (key_mask->l2_mask.vlan_itag_mask.tpid) {
				if(key->l2.vlan_itag.vlan_min.tpid !=
					key->l2.vlan_itag.vlan_max.tpid)
					return CA_E_NOT_SUPPORT;
				CLS_INNER_TPID_ENC_SET(key->l2.vlan_itag.vlan_min.tpid, hash_key->inner_tpid_enc);
				hash_mask->inner_tpid_enc = 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.vid) {
				if(key->l2.vlan_itag.vlan_min.vid !=
					key->l2.vlan_itag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				hash_key->inner_vid = key->l2.vlan_itag.vlan_min.vid;
				hash_mask->inner_vid = 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.dei) {
				if(key->l2.vlan_itag.vlan_min.dei !=
					key->l2.vlan_itag.vlan_max.dei)
					return CA_E_NOT_SUPPORT;
				hash_key->inner_dei= key->l2.vlan_itag.vlan_min.dei;
				hash_mask->inner_dei= 0;
			}
			if (key_mask->l2_mask.vlan_itag_mask.pri) {
				if(key->l2.vlan_itag.vlan_min.pri !=
					key->l2.vlan_itag.vlan_max.pri)
					return CA_E_NOT_SUPPORT;
				hash_key->inner_8021p = key->l2.vlan_itag.vlan_min.pri;
				hash_mask->inner_8021p = 0;
			}
		}
	}
	/* ip fields */
	if (key_mask->ip) {
		if (key_mask->ip_mask.ip_valid) {
			hash_key->ip_vld = key->ip.ip_valid;
			hash_mask->ip_vld = 0;
		}
		if (key_mask->ip_mask.ip_version) {
			if (key->ip.ip_version == 4) /* ipv4 */
				hash_key->ip_ver = 0;
			else	/* ipv6 */
				hash_key->ip_ver = 1;
			hash_mask->ip_ver= 0;
		}
		if (key_mask->ip_mask.ip_protocol) {
			hash_key->ip_protocol = key->ip.ip_protocol;
			hash_mask->ip_protocol = 0;
		}
		if (key_mask->ip_mask.dscp) {
			hash_key->ip_dscp = key->ip.dscp;
			hash_mask->ip_dscp = 0;
		}
		if (key_mask->ip_mask.ecn) {
			hash_key->ip_ecn = key->ip.ecn;
			hash_mask->ip_ecn = 0;
		}
		if (key_mask->ip_mask.ip_sa) {
			if (key->ip.ip_sa.afi == CA_IPV4) {
				if (key->ip.ip_sa.addr_len > 32)
					return CA_E_PARAM;
				hash_key->ip_ver = 0;
				hash_key->ip_sa_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				hash_key->ip_sa_1 = 0;
				hash_key->ip_sa_2 = 0;
				hash_key->ip_sa_3 = 0;
			} else if (key->ip.ip_sa.afi == CA_IPV6) {
				if (key->ip.ip_sa.addr_len > 128)
					return CA_E_PARAM;
				hash_key->ip_ver = 1;
				hash_key->ip_sa_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				hash_key->ip_sa_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				hash_key->ip_sa_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				hash_key->ip_sa_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
			} else
				return CA_E_PARAM;
			hash_mask->ip_ver= 0;
			hash_mask->ip_sa = key->ip.ip_sa.addr_len;
		}
		if (key_mask->ip_mask.ip_da) {
			if (key->ip.ip_da.afi == CA_IPV4) {
				if (key->ip.ip_da.addr_len > 32)
					return CA_E_PARAM;
				hash_key->ip_ver = 0;
				hash_key->ip_da_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				hash_key->ip_da_1 = 0;
				hash_key->ip_da_2 = 0;
				hash_key->ip_da_3 = 0;
			} else if (key->ip.ip_da.afi == CA_IPV6) {
				if (key->ip.ip_da.addr_len > 128)
					return CA_E_PARAM;
				hash_key->ip_ver = 1;
				hash_key->ip_da_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				hash_key->ip_da_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				hash_key->ip_da_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				hash_key->ip_da_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
			} else
				return CA_E_PARAM;
			hash_mask->ip_ver= 0;
			hash_mask->ip_da = key->ip.ip_da.addr_len;
		}
		if (key_mask->ip_mask.fragment) {
			if (key->ip.fragment == 0)
				hash_key->ip_fragment_flag= 0;
			else
				hash_key->ip_fragment_flag = 1;
			hash_mask->ip_fragment_flag = 0;
		}
		if (key_mask->ip_mask.have_options) {
			hash_key->ip_options = key->ip.have_options;
			hash_mask->ip_options = 0;
		}
		if (key_mask->ip_mask.flow_label) {
			hash_key->ipv6_flow_lbl = key->ip.flow_label;
			hash_mask->ipv6_flow_lbl = 0;
		}
		if (key_mask->ip_mask.ext_header) {
			switch (key->ip.ext_header) {
				case CA_IPV6_EXT_HEADER_HOP_BY_HOP:
					hash_key->ipv6_hbh = 1;
					hash_mask->ipv6_hbh = 0;
					break;
				case CA_IPV6_EXT_HEADER_ROUTING:
					hash_key->ipv6_rh = 1;
					hash_mask->ipv6_rh = 0;
					break;
				case CA_IPV6_EXT_HEADER_DESTINATION:
					hash_key->ipv6_doh = 1;
					hash_mask->ipv6_doh = 0;
					break;
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		if (key_mask->ip_mask.icmp_type) {
			if ((key->ip.icmp_type >= 130 &&  key->ip.icmp_type <= 132) || key->ip.icmp_type == 143) {
				/* mld */
				hash_key->icmp_vld = 7;
			} else if ((key->ip.icmp_type >= 133 &&  key->ip.icmp_type <= 137) || key->ip.icmp_type == 146 || key->ip.icmp_type == 147) {
				/* ndp */
				hash_key->icmp_vld = 3;
			} else {
				if (key_mask->ip_mask.ip_version && key->ip.ip_version ==  4)
					hash_key->icmp_vld = 4;
				else if (key_mask->ip_mask.ip_version && key->ip.ip_version == 6)
					hash_key->icmp_vld = 5;
				else
					return CA_E_PARAM;
			}
			hash_key->icmp_type = key->ip.icmp_type;
			hash_mask->icmp_vld = 0;
			hash_mask->icmp_type = 0;
		}
		if (key_mask->ip_mask.igmp_type) {
			if (key_mask->ip_mask.ip_version && key->ip.ip_version == 6)
				return CA_E_PARAM;
			/* ipv4 igmp */
			hash_key->icmp_vld = 6;
			hash_key->icmp_type = key->ip.igmp_type;
			hash_mask->icmp_vld = 0;
			hash_mask->icmp_type = 0;
		}
	}
	/* l4 fields */
	if (key_mask->l4) {
		if (key_mask->l4_mask.tcp_flags) {
			hash_key->tcp_rdp_ctrl = key->l4.tcp_flags;
			hash_mask->tcp_rdp_ctrl = ~key_mask->l4_mask.tcp_flags;
		}
		/* src port */
		if (key_mask->l4_mask.src_port) {
			if (key->l4.src_port.min == key->l4.src_port.max) {
				/* one l4 src port */
				hash_key->l4_sp_exact_range = key->l4.src_port.min;
				hash_mask->l4_sp_exact_range = 0;
			} else {
				/* l4 src port range */
				/* config l3 cam */
				memset(&cam_sport, 0, sizeof(l3_cam_sport_tbl_entry_t));
				cam_sport.vld = 1;
				cam_sport.sport_low = key->l4.src_port.min;
				cam_sport.sport_hi = key->l4.src_port.max;
				if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_SPORT, &cam_sport, &rslt_idx))
					return CA_E_CONFIG;
				/* update rule info */
				rule->type |= CLS_RUL_INFO_CAM_SPORT;
				rule->hw_info.l4_sport.rslt_idx = rslt_idx;
				hash_key->l4_sp_exact_range = 0x1 << rslt_idx;
				hash_mask->l4_sp_exact_range = 0x1 << 16 | ~hash_key->l4_sp_exact_range;
			}
		}
		/* dst port */
		if (key_mask->l4_mask.dst_port) {
			if (key->l4.dst_port.min == key->l4.dst_port.max) {
				/* one l4 dst port */
				hash_key->l4_dp_exact_range = key->l4.dst_port.min;
				hash_mask->l4_dp_exact_range = 0;
			} else {
				/* l4 dst port range */
				/* config l3 cam */
				memset(&cam_dport, 0, sizeof(l3_cam_dport_tbl_entry_t));
				cam_dport.vld = 1;
				cam_dport.dport_low = key->l4.dst_port.min;
				cam_dport.dport_hi = key->l4.dst_port.max;
				if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_DPORT, &cam_dport, &rslt_idx))
					return CA_E_CONFIG;
				/* update rule info */
				rule->type |= CLS_RUL_INFO_CAM_DPORT;
				rule->hw_info.l4_dport.rslt_idx = rslt_idx;
				hash_key->l4_dp_exact_range = 0x1 << rslt_idx;
				hash_mask->l4_dp_exact_range = 0x1 << 16 | ~hash_key->l4_dp_exact_range;
			}
		}
	}
	/* ext fields */
	if (key_mask->ext) {
		ca_int16_t i, avail_cnt, len;

		if (key->ext.length > 4 || key->ext.length <= 0)
			return CA_E_PARAM;

		avail_cnt = aal_table_get_avail_count(AAL_TABLE_L3_CAM_SPCL_HDR);
		if (avail_cnt < (key->ext.length + 1)/2)
			return CA_E_UNAVAIL;
		/* config l3 cam */
		memset(&cam_spcl_hdr, 0, sizeof(l3_cam_spcl_hdr_tbl_entry_t));
		cam_spcl_hdr.vld = 1;
		cam_spcl_hdr.type = 5;
		for (i = 0, len = key->ext.length; i < key->ext.length; i++) {
			if (i%2) {
				cam_spcl_hdr.mask |= key->ext.mask[i];
				cam_spcl_hdr.key |= key->ext.data[i];
			} else {
				cam_spcl_hdr.mask |= key->ext.mask[i] << 8;
				cam_spcl_hdr.key |= key->ext.data[i] << 8;
			}
			if (i/2)
				cam_spcl_hdr.ms_vld = 0;
			else
				cam_spcl_hdr.ms_vld = 1;
			len--;
			if (len <= 0 || i%2) {
				if (AAL_E_OK != aal_entry_add(AAL_TABLE_L3_CAM_SPCL_HDR, &cam_spcl_hdr, &rslt_idx))
					return CA_E_CONFIG;
				cam_spcl_hdr.mask = 0;
				cam_spcl_hdr.key = 0;
				rule->hw_info.spcl_hdr.rslt_idx |= 0x1 << rslt_idx;
			}
		}
		CA_NE_REG_WRITE(key->ext.offset, L3FE_PP_OFFSET_HDR);
		/* update rule info */
		rule->type |= CLS_RUL_INFO_CAM_SPCL_HDR;

		hash_key->spcl_pkt_hdr_mtch = rule->hw_info.spcl_hdr.rslt_idx;
		hash_mask->spcl_pkt_hdr_mtch = 0;
	}
	return CA_E_OK;
}

static ca_status_t cls_hash_action_update(
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	ca_classifier_action_t	 *action,
	aal_hash_action_t *hash_action)
{
	ca_uint8_t o_vid_cnt, i_vid_cnt, o_tpid_enc, i_tpid_enc, o_dei, i_dei;
	ca_uint8_t keep_original = 1;
	ca_l3_nexthop_id_t nexthop_id;

	if (action == NULL || hash_action == NULL)
		return CA_E_ERROR;

	/* ignore all fields */
	memset(hash_action, 0, sizeof(aal_hash_action_t));

	switch (action->forward) {
		case CA_CLASSIFIER_FORWARD_DENY:
			hash_action->permit = 0;
			hash_action->dpid_vld = 1;
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0xf;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xf;
			hash_action->t5_ctrl_vld = 1;
			hash_action->t5_ctrl = 0xf;
			hash_action->stage2_ctrl_vld = 1;
			hash_action->stage2_ctrl = 1;
			hash_action->stage3_ctrl_vld = 1;
			hash_action->stage3_ctrl = 1;
			break;
		case CA_CLASSIFIER_FORWARD_PORT:
			hash_action->mc = 0;
			hash_action->mcgid = PORT_ID(action->dest.port);
			hash_action->dpid_pri = 1;
			hash_action->permit = 1;
			hash_action->dpid_vld = 1;
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0xf;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xf;
			hash_action->t5_ctrl_vld = 1;
			hash_action->t5_ctrl = 0xf;
			hash_action->stage2_ctrl_vld = 1;
			hash_action->stage2_ctrl = 1;
			hash_action->stage3_ctrl_vld = 1;
			hash_action->stage3_ctrl = 1;
			if (!(hash_action->mcgid >= AAL_LPORT_CPU_0 && hash_action->mcgid <= AAL_LPORT_CPU_7)) {
				hash_action->deepq = 1;
				hash_action->keep_orig_pkt_vld = 1;
				hash_action->keep_orig_pkt = 0;
			}
			break;
		case CA_CLASSIFIER_FORWARD_INTERFACE:
			hash_action->mdata_w_1 = action->dest.intf & 0xff;
			hash_action->mdata_w_vld_1 = 0x3;
			hash_action->l3_egress_if_vld = 1;
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0xf;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xf;
			keep_original = 0;
			break;
		case CA_CLASSIFIER_FORWARD_FE:
			keep_original = 0;
			break;
		case CA_CLASSIFIER_FORWARD_TUNNEL:
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
			return CA_E_NOT_SUPPORT;
#else
			if (get_nexthop_id_by_tunnel_id(action->dest.tunnel_id, &nexthop_id) != CA_E_OK) {
				ca_printf(KERN_INFO "%s: Fail to find nexthop_id by tunnel_id %d.\n", __func__, action->dest.tunnel_id);
				return CA_E_PARAM;
			}
			hash_action->mdata_w_0 = nexthop_id;
			hash_action->mdata_w_vld_0 = 0x7;
			hash_action->l3_egress_if_vld = 1;
			hash_action->t3_ctrl_vld = 1;
			hash_action->t3_ctrl = 0xf;
			hash_action->t4_ctrl_vld = 1;
			hash_action->t4_ctrl = 0xe; /* directly use the HashLite entry index */
			keep_original = 0;
			break;
#endif
		default:
			return CA_E_NOT_SUPPORT;
	}
	/* set flow_id */
	if (action->options.masks.action_handle) {
		if (action->options.action_handle.flow_id > CA_AAL_MAX_FLOW_ID || action->options.action_handle.flow_id > CA_AAL_L3_MAX_FLOW_ID)
			return CA_E_PARAM;
		hash_action->pol_vld = 1;
		/* both L2, L3 Policer enable */
		hash_action->pol_en = 3;
		hash_action->pol_base = action->options.action_handle.flow_id;
		hash_action->pol_sel = 2;
	}
	/* sw_shaper_id */
	if (action->options.masks.sw_shaper_id) {
		if (action->forward == CA_CLASSIFIER_FORWARD_PORT || action->options.masks.priority)
			return CA_E_PARAM;
		/* flow id = queue id, LDPID 0x20-0x3f (each with 8 VoQs, 256 queues total) */
		if (action->options.sw_shaper_id >= 256)
			return CA_E_PARAM;
		hash_action->mc = 0;
		hash_action->mcgid = 0x20 + action->options.sw_shaper_id/8;
		hash_action->dpid_pri = 1;
		hash_action->permit = 1;
		hash_action->dpid_vld = 1;
		hash_action->cos = action->options.sw_shaper_id%8;
		hash_action->cos_sel = 1;

		hash_action->t3_ctrl_vld = 1;
		hash_action->t3_ctrl = 0xf;
		hash_action->t4_ctrl_vld = 1;
		hash_action->t4_ctrl = 0xf;
		hash_action->t5_ctrl_vld = 1;
		hash_action->t5_ctrl = 0xf;

		hash_action->deepq = 1;
	}
	/* l2 fields */
	if (action->options.masks.mac_da) {
		hash_action->mac_da_0 = action->options.mac_da[5];
		hash_action->mac_da_1 = action->options.mac_da[4];
		hash_action->mac_da_2 = action->options.mac_da[3];
		hash_action->mac_da_3 = action->options.mac_da[2];
		hash_action->mac_da_4 = action->options.mac_da[1];
		hash_action->mac_da_5 = action->options.mac_da[0];
		hash_action->mac_da_vld = 1;
		keep_original = 0;
	}
	/* priority */
	if (action->options.masks.priority) {
		hash_action->cos = action->options.priority;
		hash_action->cos_sel = 1;
	}
	/* vlan fields */
	if (action->options.masks.outer_vlan_act || action->options.masks.outer_dei || action->options.masks.outer_tpid ||
		action->options.masks.inner_vlan_act || action->options.masks.inner_dei || action->options.masks.inner_tpid) {
		if (key == NULL || key_mask == NULL)
			return CA_E_ERROR;
		keep_original = 0;
		/* get vlan info for the incomming packet from key */
		o_vid_cnt = i_vid_cnt = 0;
		o_tpid_enc = i_tpid_enc = 1;
		o_dei = i_dei = 0;
		if (key_mask->l2) {
			if (key_mask->l2_mask.vlan_otag) {
				/* get outter tpid */
				if (key_mask->l2_mask.vlan_otag_mask.tpid) {
					CLS_TOP_TPID_ENC_SET(key->l2.vlan_otag.vlan_min.tpid, o_tpid_enc);
				}
				/* get outter dei */
				if (key_mask->l2_mask.vlan_otag_mask.dei) {
					o_dei = key->l2.vlan_otag.vlan_min.dei;
				}
			}

			if (key_mask->l2_mask.vlan_itag) {
				/* get inner tpid */
				if (key_mask->l2_mask.vlan_itag_mask.tpid) {
					CLS_INNER_TPID_ENC_SET(key->l2.vlan_itag.vlan_min.tpid, i_tpid_enc);
				}
				/* get inner dei */
				if (key_mask->l2_mask.vlan_itag_mask.dei) {
					i_dei = key->l2.vlan_itag.vlan_min.dei;
				}
			}
		}
		/* set outer vlan action for outgoing packet */
		if (action->options.masks.outer_vlan_act) {
			switch(action->options.outer_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					hash_action->top_vid = action->options.outer_vid;
					o_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_POP:
					o_vid_cnt = 0;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* set inner vlan action for outgoing packet */
		if (action->options.masks.inner_vlan_act) {
			switch(action->options.inner_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					if (!action->options.masks.outer_vlan_act || (action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_POP))
						return CA_E_NOT_SUPPORT;
					hash_action->inner_vid = action->options.inner_vid;
					i_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_POP:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* cls fib supports 2 vlan tags at most */
		if ((o_vid_cnt + i_vid_cnt) > 2)
			return CA_E_PARAM;
		hash_action->vlan_cnt = o_vid_cnt + i_vid_cnt;

		/* set outer tpid */
		if (action->options.masks.outer_tpid) {
			if (!o_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_TOP_TPID_ENC_SET(action->options.outer_tpid, hash_action->top_tpid_enc);
		} else
			hash_action->top_tpid_enc = o_tpid_enc;
		/* set outer dei */
		if (action->options.masks.outer_dei) {
			if (!o_vid_cnt)
				return CA_E_NOT_SUPPORT;
			hash_action->top_dei_sel = 0;
			hash_action->top_dei = action->options.outer_dei;
		} else
			hash_action->top_dei_sel = 1;

		/* set inner tpid */
		if (action->options.masks.inner_tpid) {
			if (!i_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_INNER_TPID_ENC_SET(action->options.inner_tpid, hash_action->inner_tpid_enc);
		} else
			hash_action->inner_tpid_enc = i_tpid_enc;
		/* set inner dei */
		if (action->options.masks.inner_dei) {
			if (!i_vid_cnt)
				return CA_E_NOT_SUPPORT;
			hash_action->inner_dei_sel = 0;
			hash_action->inner_dei = action->options.inner_dei;
		} else
			hash_action->inner_dei_sel = 1;

		hash_action->vlan_vld = 1;
	}
	if (action->options.masks.inner_dot1p) {
		/* set hdr_i_out.inner_802_1p = fib.inner_802_1p */
		hash_action->inner_802_1p_sel = 2;
		hash_action->inner_802_1p = action->options.inner_dot1p;
		keep_original = 0;
	}
	if (action->options.masks.outer_dot1p) {
		/* set hdr_i_out.top_802_1p = fib.top_802_1p */
		hash_action->top_802_1p_sel = 2;
		hash_action->top_802_1p = action->options.outer_dot1p;
		keep_original = 0;
	}
	/* l3 fields */
	if (action->options.masks.dscp) {
		/* set RSLT_DSCP */
		hash_action->ip_dscp_sel = 1;
		hash_action->ip_dscp = action->options.dscp;
		hash_action->ip_dscp_update_en = 3;
		keep_original = 0;
	}
	/* sw_id */
	if (action->options.masks.sw_id) {
		/* mdata[55:32], 24 bits */
		hash_action->mdata_w_vld_3 = 0x3;
		hash_action->mdata_w_3 = action->options.sw_id[1] & 0xff;
		hash_action->mdata_w_vld_2 = 0xf;
		hash_action->mdata_w_2 = action->options.sw_id[0];
	}

	if (keep_original == 1)
		hash_action->keep_orig_pkt = 1;
	else
		hash_action->keep_orig_pkt = 0;

	hash_action->keep_orig_pkt_vld = 1;

	return CA_E_OK;
}

static ca_status_t cls_hash_rule_add (
	ca_uint32_t              profile_id,
	ca_uint32_t              priority,
	aal_hash_key_t           *hash_key,
	aal_hash_mask_t          *hash_mask,
	aal_hash_action_t        *hash_action,
	ca_uint32_t              *mask_idx,
	ca_uint8_t               *tuple_idx,
	ca_uint32_t              *hash_idx)
{
	ca_status_t rc = CA_E_OK;
	ca_uint8_t msk_flag = 0, tuple_flag = 0;

	if (hash_key == NULL || hash_mask == NULL || hash_action == NULL || mask_idx == NULL || tuple_idx == NULL || hash_idx == NULL)
		return CA_E_ERROR;
	if (priority > CL_RUL_PRIO_MAX)
		return  CA_E_PARAM;

	/* create hash mask */
	if (AAL_E_OK != (rc = aal_mask_tbl_entry_add(hash_mask, mask_idx))) {
		if (rc != AAL_E_EXIST)
			goto ERROR;
	}
	msk_flag = 1;
	/* add hash mask to tuple */
	if (AAL_E_OK != (rc = aal_hash_profile_tuple_add(profile_id, TUPLE_TYPE_QOS, priority/2, *mask_idx, tuple_idx))) {
		if (rc != AAL_E_EXIST)
			goto ERROR;
	}
	tuple_flag = 1;
	/* create hash rule */
	if (CA_E_OK != (rc = aal_hash_add(0, hash_key, hash_action, TUPLE_TYPE_QOS, HASH_AGING_STATIC, *mask_idx, profile_id, hash_idx)))
		goto ERROR;

	return CA_E_OK;

ERROR:
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);

	/* delete tuple entry */
	if (tuple_flag)
		aal_hash_profile_tuple_del(profile_id, *tuple_idx);
	/* delete mask entry */
	if (msk_flag)
		aal_mask_tbl_entry_del(*mask_idx);

	if (rc == AAL_E_TBLFULL)
		return CA_E_FULL;
	else if (rc == AAL_E_ENTRYNOTFOUND)
		return CA_E_NOT_FOUND;
	else
		return CA_E_ERROR;
}

static ca_status_t cls_hash_rule_delete (
	ca_uint32_t             profile_id,
	ca_uint32_t             mask_idx,
	ca_uint8_t              tuple_idx,
	ca_uint32_t             hash_idx)
{
	ca_status_t rc = CA_E_OK;

	/* delete hash rule */
	if (CA_E_OK != (rc = aal_hash_delete(0, hash_idx)))
		goto ERROR;
	/* delete tuple */
	if (CA_E_OK != (rc = aal_hash_profile_tuple_del(profile_id, tuple_idx)))
		goto ERROR;
	/* delete hash mask */
	if (CA_E_OK != (rc = aal_mask_tbl_entry_del(mask_idx)))
		goto ERROR;

	return CA_E_OK;

ERROR:
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}
#endif

static ca_status_t cls_hashlite_key_update(
    ca_classifier_key_t      *key,
	ca_classifier_key_mask_t *key_mask,
	aal_hash_key_t           *hash_key,
	aal_hash_mask_t          *hash_mask,
	cls_rule_info_t          *rule)
{
	ca_uint8_t mac_msk=0, i;

	if (key == NULL || key_mask == NULL || hash_key == NULL || hash_mask == NULL)
		return CA_E_ERROR;

	if (key_mask->orig_src_port || key_mask->dest_port || key_mask->src_intf || key_mask->dst_intf ||
		key_mask->l2_mask.subtype || key_mask->l2_mask.mac_sa || key_mask->l2_mask.is_multicast ||
		key_mask->l2_mask.is_length || key_mask->l2_mask.cfm_opcode ||
		key_mask->ip_mask.ip_version || key_mask->ip_mask.dscp ||
		key_mask->ip_mask.ecn || key_mask->ip_mask.ip_sa_max || key_mask->ip_mask.ip_da_max || key_mask->ip_mask.fragment ||
		key_mask->ip_mask.have_options || key_mask->ip_mask.flow_label || key_mask->ip_mask.ext_header || key_mask->ip_mask.icmp_type ||
		key_mask->ip_mask.igmp_type || key_mask->ip_mask.is_multicast ||
		key_mask->l4 || key_mask->ext)
		return CA_E_NOT_SUPPORT;

	memset(hash_key, 0, sizeof(aal_hash_key_t));
	hash_mask_unmask(hash_mask);

	if (key_mask->key_handle) {
		if (key->handle_type == CA_KEY_HANDLE_TYPE_GEM_INDEX) {
			/* gem_index 7 bits map to pol_id */
			hash_key->pol_id = key->key_handle.gem_index & 0x7f;
		}else if (key->handle_type == CA_KEY_HANDLE_TYPE_LLID_COS) {
			/* llid_cos_index 6 bits map to pol_id */
			hash_key->pol_id = key->key_handle.llid_cos_index & 0x3f;
		} else
			return CA_E_NOT_SUPPORT;
		hash_mask->pol_id = 0;
	}
	/* l2 fields */
	if (key_mask->l2) {
		if (key_mask->l2_mask.ethertype) {
			hash_mask->ethertype = 0;
			hash_key->ethertype = key->l2.ethertype;
		}
		if (key_mask->l2_mask.mac_da) {
			for (i=0; i < CA_ETH_ADDR_LEN; i++) {
				if (key->l2.mac_da.mac_min[i] == key->l2.mac_da.mac_max[i])
					mac_msk |= (0x1 << (5-i));
				else if (key->l2.mac_da.mac_max[i]-key->l2.mac_da.mac_min[i] != 0xff)
					return CA_E_PARAM;
			}
			hash_mask->mac_da = ~mac_msk;
			hash_key->mac_da_0 = key->l2.mac_da.mac_min[5];
			hash_key->mac_da_1 = key->l2.mac_da.mac_min[4];
			hash_key->mac_da_2 = key->l2.mac_da.mac_min[3];
			hash_key->mac_da_3 = key->l2.mac_da.mac_min[2];
			hash_key->mac_da_4 = key->l2.mac_da.mac_min[1];
			hash_key->mac_da_5 = key->l2.mac_da.mac_min[0];
		}
		/* vlan count */
		if (key_mask->l2_mask.vlan_count) {
			hash_key->vlan_cnt = key->l2.vlan_count;
			hash_mask->vlan_cnt = 0;
			if (!hash_key->vlan_cnt)
				hash_mask->top_vid = 0;
		}
		/* vlan fields */
		if (key_mask->l2_mask.vlan_otag) {
			if (key_mask->l2_mask.vlan_otag_mask.vid) {
				if(key->l2.vlan_otag.vlan_min.vid !=
					key->l2.vlan_otag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				hash_key->top_vid = key->l2.vlan_otag.vlan_min.vid;
				hash_mask->top_vid = 0;
			}
			if (key_mask->l2_mask.vlan_otag_mask.pri) {
				if(key->l2.vlan_otag.vlan_min.pri !=
					key->l2.vlan_otag.vlan_max.pri)
					return CA_E_NOT_SUPPORT;
				hash_key->top_8021p = key->l2.vlan_otag.vlan_min.pri;
				hash_mask->top_8021p = 0;
			}
		}
		/* inner vlan fields */
		if (key_mask->l2_mask.vlan_itag) {
			if (key_mask->l2_mask.vlan_itag_mask.vid) {
				if(key->l2.vlan_itag.vlan_min.vid !=
					key->l2.vlan_itag.vlan_max.vid)
					return CA_E_NOT_SUPPORT;
				hash_key->inner_vid = key->l2.vlan_itag.vlan_min.vid;
				hash_mask->inner_vid = 0;
			}
		}
	}
	/* ip fields */
	if (key_mask->ip) {
		if (key_mask->ip_mask.ip_valid) {
			hash_key->ip_vld = key->ip.ip_valid;
			hash_mask->ip_vld = 0;
		}
		if (key_mask->ip_mask.ip_protocol) {
			hash_key->ip_protocol = key->ip.ip_protocol;
			hash_mask->ip_protocol = 0;
		}
		if (key_mask->ip_mask.ip_sa) {
			if (key->ip.ip_sa.afi == CA_IPV4) {
				if (key->ip.ip_sa.addr_len > 32)
					return CA_E_PARAM;
				if (key->ip.ip_sa.addr_len != 32)
					return CA_E_NOT_SUPPORT;
				hash_key->ip_sa_0 = key->ip.ip_sa.ip_addr.ipv4_addr;
				hash_key->ip_sa_1 = 0;
				hash_key->ip_sa_2 = 0;
				hash_key->ip_sa_3 = 0;
			} else if (key->ip.ip_sa.afi == CA_IPV6) {
				if (key->ip.ip_sa.addr_len > 128)
					return CA_E_PARAM;
				if (key->ip.ip_sa.addr_len != 128)
					return CA_E_NOT_SUPPORT;
				hash_key->ip_sa_0 = key->ip.ip_sa.ip_addr.ipv6_addr[3];
				hash_key->ip_sa_1 = key->ip.ip_sa.ip_addr.ipv6_addr[2];
				hash_key->ip_sa_2 = key->ip.ip_sa.ip_addr.ipv6_addr[1];
				hash_key->ip_sa_3 = key->ip.ip_sa.ip_addr.ipv6_addr[0];
			} else
				return CA_E_PARAM;
			/* only support mask, and no prefix */
			hash_mask->ip_sa = 128;
		}
		if (key_mask->ip_mask.ip_da) {
			if (key->ip.ip_da.afi == CA_IPV4) {
				if (key->ip.ip_da.addr_len > 32)
					return CA_E_PARAM;
				if (key->ip.ip_da.addr_len != 32)
					return CA_E_NOT_SUPPORT;
				hash_key->ip_da_0 = key->ip.ip_da.ip_addr.ipv4_addr;
				hash_key->ip_da_1 = 0;
				hash_key->ip_da_2 = 0;
				hash_key->ip_da_3 = 0;
			} else if (key->ip.ip_da.afi == CA_IPV6) {
				if (key->ip.ip_da.addr_len > 128)
					return CA_E_PARAM;
				if (key->ip.ip_da.addr_len != 128)
					return CA_E_NOT_SUPPORT;
				hash_key->ip_da_0 = key->ip.ip_da.ip_addr.ipv6_addr[3];
				hash_key->ip_da_1 = key->ip.ip_da.ip_addr.ipv6_addr[2];
				hash_key->ip_da_2 = key->ip.ip_da.ip_addr.ipv6_addr[1];
				hash_key->ip_da_3 = key->ip.ip_da.ip_addr.ipv6_addr[0];
			} else
				return CA_E_PARAM;
			/* only support mask, and no prefix */
			hash_mask->ip_da = 128;
		}
	}

	/* check if the hash mask is supported by hashlite */
	switch (l3fe_hash_lite_profile) {
		case HASHLITE_PROFILE_1:
			hash_mask->hkey_id = 0;
			if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_MC, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_MC\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_MC_ETHTYPE, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_MC_ETHTYPE\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_MC_VLAN, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_MC_VLAN\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_MC_IGMPV3, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_MC_IGMPV3\n", __func__);
#if 0 //rtk workaround for R0.7.29.0
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_MC_MACDA, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_MC_MACDA\n", __func__);
#endif
			} else {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: no match any hashlite mask\n", __func__);
				return CA_E_NOT_SUPPORT;
			}
			break;
		case HASHLITE_PROFILE_2:
			hash_mask->hkey_id = 0;
			if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_VLAN_0_TAG, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_VLAN_0_TAG\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_OUTVID_8021P, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_OUTVID_8021P\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_VLAN_1_TAG, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_VLAN_1_TAG\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_OUT8021P, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_OUT8021P\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_ETHTYPE, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_ETHTYPE\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_MACDARNG, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_MACDARNG\n", __func__);
			} else if (!memcmp((ca_uint8_t *)hash_mask, &HM_SFU_DS_LLID_VLAN_2_TAG, sizeof(aal_hash_mask_t))) {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: match the hashlite mask of HM_SFU_DS_LLID_VLAN_2_TAG\n", __func__);
			} else {
				if (ca_api_debug & API_DBG_CLS)
					ca_printf("%s: no match any hashlite mask\n", __func__);
				return CA_E_NOT_SUPPORT;
			}
			break;
		default:
			if (ca_api_debug & API_DBG_CLS)
				ca_printf("%s: HASHLITE_PROFILE_%d doesn't support this hashlite mask\n", __func__, l3fe_hash_lite_profile);
			return CA_E_NOT_SUPPORT;
	}
	return CA_E_OK;
}

static ca_status_t cls_hashlite_action_update(
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	ca_classifier_action_t	 *action,
	aal_hashlite_hash_action_entry_t *hashlt_action)
{
	ca_uint8_t o_vid_cnt, i_vid_cnt, o_tpid_enc, i_tpid_enc;
	hl_act_sfu_ds_llid_t *hash_action = (hl_act_sfu_ds_llid_t *)hashlt_action;

	if (action == NULL || hash_action == NULL)
		return CA_E_ERROR;

	if (action->options.masks.inner_dot1p ||
		action->options.masks.mac_da || action->options.masks.sw_id || action->options.masks.sw_shaper_id)
		return CA_E_NOT_SUPPORT;

	/* ignore all fields */
	memset(hash_action, 0, sizeof(hl_act_sfu_ds_llid_t));

	switch (action->forward) {
		case CA_CLASSIFIER_FORWARD_DENY:
			hash_action->permit = 0;
			hash_action->dpid_vld = 1;
			break;
		case CA_CLASSIFIER_FORWARD_PORT:
			hash_action->mc = 0;
			hash_action->mcgid = PORT_ID(action->dest.port);
			hash_action->dpid_pri = 1;
			hash_action->permit = 1;
			hash_action->dpid_vld = 1;
			if (hash_action->mcgid != AAL_LPORT_L3_WAN && hash_action->mcgid != AAL_LPORT_L3_LAN) {
				/* enable deep-Q if forward the pkt to dest-port directly, for support QoS & Shaping */
				hash_action->deepq = 1;
			}
			break;
		case CA_CLASSIFIER_FORWARD_FE:
			break;
		default:
			return CA_E_NOT_SUPPORT;
	}

	if (action->options.masks.mcg_id) {
		hash_action->mc = 1;
		hash_action->mcgid = action->options.mcg_id;
	}

	/* set flow_id */
	if (action->options.masks.action_handle) {
		if (action->options.action_handle.flow_id > CA_AAL_MAX_FLOW_ID || action->options.action_handle.flow_id > CA_AAL_L3_MAX_FLOW_ID)
			return CA_E_PARAM;
		hash_action->pol_vld = 1;
		/* both L2, L3 Policer enable */
		hash_action->pol_en = 3;
		hash_action->pol_base = action->options.action_handle.flow_id;
		hash_action->pol_sel = 2;
	}
	/* priority */
	if (action->options.masks.priority) {
		hash_action->cos = action->options.priority;
		hash_action->cos_sel = 1;
	}
	/* vlan fields */
	if (action->options.masks.outer_vlan_act || action->options.masks.outer_dei || action->options.masks.outer_tpid ||
		action->options.masks.inner_vlan_act || action->options.masks.inner_dei || action->options.masks.inner_tpid) {
		if (key == NULL || key_mask == NULL)
			return CA_E_ERROR;
		/* get vlan info for the incomming packet from key */
		o_vid_cnt = i_vid_cnt = 0;
		o_tpid_enc = i_tpid_enc = 1;
		/* set outer vlan action for outgoing packet */
		if (action->options.masks.outer_vlan_act) {
			switch(action->options.outer_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					hash_action->top_vid = action->options.outer_vid;
					o_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_POP:
					o_vid_cnt = 0;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* set inner vlan action for outgoing packet */
		if (action->options.masks.inner_vlan_act) {
			switch(action->options.inner_vlan_act) {
				case CA_CLASSIFIER_VLAN_ACTION_SET:
					if (!action->options.masks.outer_vlan_act || (action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_POP))
						return CA_E_NOT_SUPPORT;
					hash_action->inner_vid = action->options.inner_vid;
					i_vid_cnt = 1;
					break;
				case CA_CLASSIFIER_VLAN_ACTION_PUSH:
				case CA_CLASSIFIER_VLAN_ACTION_POP:
				case CA_CLASSIFIER_VLAN_ACTION_SWAP:
				case CA_CLASSIFIER_VLAN_ACTION_NOP:
				default:
					return CA_E_NOT_SUPPORT;
			}
		}
		/* cls fib supports 2 vlan tags at most */
		if ((o_vid_cnt + i_vid_cnt) > 2)
			return CA_E_PARAM;
		hash_action->vlan_cnt = o_vid_cnt + i_vid_cnt;

		/* set outer tpid */
		if (action->options.masks.outer_tpid) {
			if (!o_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_TOP_TPID_ENC_SET(action->options.outer_tpid, hash_action->top_tpid_enc);
		} else
			hash_action->top_tpid_enc = o_tpid_enc;
		/* set outer dei */
		if (action->options.masks.outer_dei) {
			if (!o_vid_cnt)
				return CA_E_NOT_SUPPORT;
			hash_action->top_dei_sel = 0;
			hash_action->top_dei = action->options.outer_dei;
		} else
			hash_action->top_dei_sel = 1;

		/* set inner tpid */
		if (action->options.masks.inner_tpid) {
			if (!i_vid_cnt)
				return CA_E_NOT_SUPPORT;
			CLS_INNER_TPID_ENC_SET(action->options.inner_tpid, hash_action->inner_tpid_enc);
		} else
			hash_action->inner_tpid_enc = i_tpid_enc;
		/* set inner dei */
		if (action->options.masks.inner_dei) {
			if (!i_vid_cnt)
				return CA_E_NOT_SUPPORT;
			hash_action->inner_dei_sel = 0;
			hash_action->inner_dei = action->options.inner_dei;
		} else
			hash_action->inner_dei_sel = 1;

		hash_action->vlan_vld = 1;
	}
	if (action->options.masks.outer_dot1p) {
		/* set hdr_i_out.top_802_1p = fib.top_802_1p */
		hash_action->top_802_1p_sel = 2;
		hash_action->top_802_1p = action->options.outer_dot1p;
	}
	/* l3 fields */
	if (action->options.masks.dscp) {
		/* set RSLT_DSCP */
		hash_action->ip_dscp_sel = 1;
		hash_action->ip_dscp = action->options.dscp;
		hash_action->ip_dscp_update_en = 3;
	}

	return CA_E_OK;
}

extern ca_status_t l2_classifier_rule_del (
CA_IN      ca_device_id_t                           device_id,
CA_IN      ca_uint32_t                            index);

static ca_status_t cls_hw_rule_delete (
	cls_rule_info_t *rule)
{
	ca_status_t rc;
	ca_uint8 i;

	if (rule == NULL)
		return CA_E_ERROR;

	if (rule->type & CLS_RUL_INFO_CLS) {
		if (CA_E_OK != (rc = aal_l3_cls_delete(0, rule->hw_info.cls.profile_id, rule->hw_info.cls.rule_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CLS;
	}
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
	if (rule->type & CLS_RUL_INFO_HASH) {
		if (CA_E_OK != (rc = cls_hash_rule_delete(rule->hw_info.hash.profile_id, rule->hw_info.hash.mask_idx,
						rule->hw_info.hash.tuple_idx, rule->hw_info.hash.hash_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_HASH;
	}
#endif
	if (rule->type & CLS_RUL_INFO_CAM_ETHERTYPE) {
		if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_ETHERTYPE, rule->hw_info.ethertype.rslt_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CAM_ETHERTYPE;
	}
	if (rule->type & CLS_RUL_INFO_CAM_PPP_PROT) {
		if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_PPP_PROT, rule->hw_info.ppp_prot.rslt_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CAM_PPP_PROT;
	}
	if (rule->type & CLS_RUL_INFO_CAM_SPORT) {
		if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_SPORT, rule->hw_info.l4_sport.rslt_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CAM_SPORT;
	}
	if (rule->type & CLS_RUL_INFO_CAM_DPORT) {
		if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_DPORT, rule->hw_info.l4_dport.rslt_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CAM_DPORT;
	}
	if (rule->type & CLS_RUL_INFO_CAM_SPCL_HDR) {
		i = 0;
		while (rule->hw_info.spcl_hdr.rslt_idx) {
			if (rule->hw_info.spcl_hdr.rslt_idx & 0x1) {
				if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_SPCL_HDR, i)))
					goto ERROR;
			}
			rule->hw_info.spcl_hdr.rslt_idx >>= 1;
			i++;
		}
		rule->type &= ~CLS_RUL_INFO_CAM_SPCL_HDR;
	}
	if (rule->type & CLS_RUL_INFO_CAM_PKT_LEN) {
		if (CA_E_OK != (rc = aal_entry_del(AAL_TABLE_L3_CAM_PKT_LEN, rule->hw_info.pkt_len.rslt_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_CAM_PKT_LEN;
	}
	if (rule->type & CLS_RUL_INFO_L2_CLS) {
		if ( rule->hw_info.l2fe_info.l2fe_cls_vld) {
			if (CA_E_OK != (rc = l2_classifier_rule_del(0, rule->hw_info.l2fe_info.l2fe_cls_index)))
				goto ERROR;
		}
		rule->hw_info.l2fe_info.l2fe_cls_vld = 0;
		rule->type &= ~CLS_RUL_INFO_L2_CLS;
	}
	if (rule->type & CLS_RUL_INFO_HASH_LITE) {
		if (CA_E_OK != (rc = aal_hashlite_hash_delete(0, rule->hw_info.hash.hash_idx)))
			goto ERROR;
		rule->type &= ~CLS_RUL_INFO_HASH_LITE;
	}

	return CA_E_OK;

ERROR:
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rule->type = %x, rc = %x\n", __func__, rule->type, rc);
	return rc;
}

static ca_status_t l3_cls_rule_add(
	ca_uint32_t profile_id,
	ca_uint32_t priority,
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	ca_classifier_action_t *action,
	cls_rule_info_t *rule)
{
	ca_status_t rc = CA_E_OK;
	l3_cls_rule_t aal_rule;
	ca_uint32_t hw_idx;

	if (key_mask->merge_prio && key->merge_prio >= CL_RSLT_TYPE_MAX)
		return CA_E_PARAM;

	/* update ca_classifier_key_t to cls_key_t */
	if (CA_E_OK != (rc = cls_key_entry_update(key, key_mask, &aal_rule.key, rule)))
		goto ERROR;

	/* update ca_classifier_action_t to cls_action_t */
	if (CA_E_OK != (rc = cls_fib_entry_update(key, key_mask, action, &aal_rule.action)))
		goto ERROR;

	/* msk_ctrl will be merged from l3 intf rule(CL_RSLT_TYPE_0) after rule hit */
	aal_rule.pri = priority;
	aal_rule.rslt_type = key->merge_prio;

	/* ignore key.src_port field for wan profile */
	if (profile_id == l3fe_classifier_profile_wan) {
		aal_rule.key.msk_lspid = 1;
		aal_rule.key.lspid = 0x3f;
	}

	if (CA_E_OK != (rc = aal_l3_cls_add(0, profile_id, &aal_rule, &hw_idx)))
		goto ERROR;

	rule->type |= CLS_RUL_INFO_CLS;
	rule->hw_info.cls.profile_id = profile_id;
	rule->hw_info.cls.rule_idx = hw_idx;
	return CA_E_OK;

ERROR:
	/* delete hw rule */
	cls_hw_rule_delete(rule);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);

	if (rc == AAL_E_TBLFULL)
		return CA_E_FULL;
	else if (rc == AAL_E_ENTRYNOTFOUND)
		return CA_E_NOT_FOUND;
	else
		return CA_E_ERROR;
}

static ca_status_t cls_hashlite_rule_add(
	ca_uint32_t profile_id,
	ca_classifier_key_t *key,
	ca_classifier_key_mask_t *key_mask,
	ca_classifier_action_t *action,
	cls_rule_info_t *rule)
{
	ca_status_t rc = CA_E_OK;
	aal_hash_key_t hashlt_key;
	aal_hash_mask_t hashlt_mask;
	aal_hashlite_hash_action_entry_t hashlt_action;
	ca_uint32_t hashlt_idx = 0;

	if (CA_E_OK != (rc = cls_hashlite_key_update(key, key_mask, &hashlt_key, &hashlt_mask, NULL)))
		goto ERROR;
	if (CA_E_OK != (rc = cls_hashlite_action_update(key, key_mask, action, &hashlt_action)))
		goto ERROR;

	if (CA_E_OK != (rc = aal_hashlite_hash_add(0, &hashlt_key, &hashlt_mask, &hashlt_action, HL_ACTGRP_SFU_DS_LLID, HASHLITE_AGING_STATIC, &hashlt_idx))) {
		if (ca_api_debug & API_DBG_CLS)
			ca_printf("%s: add hashlite rule fail, rc = %x\n", __func__, rc);
		goto ERROR;
	}
	rule->type |= CLS_RUL_INFO_HASH_LITE;
	rule->hw_info.hash.profile_id = profile_id;
	rule->hw_info.hash.mask_idx = 0xFFFF;
	rule->hw_info.hash.tuple_idx = 0xFF;
	rule->hw_info.hash.hash_idx = hashlt_idx;
	return CA_E_OK;

ERROR:
	/* delete hw rule */
	cls_hw_rule_delete(rule);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: create hashlite fail, rc = %x\n", __func__, rc);

	if (rc == AAL_E_TBLFULL)
		return CA_E_FULL;
	else if (rc == AAL_E_EXIST) {
		if (ca_api_debug & API_DBG_CLS)
			ca_printf("%s: hashlite entry exists, profile_id = %d, hashlt_idx = %d, rc = %x\n", __func__, profile_id, hashlt_idx, rc);
		rule->hw_info.hash.profile_id = profile_id;
		rule->hw_info.hash.hash_idx = hashlt_idx;
		return CA_E_EXISTS;
	} else
		return CA_E_ERROR;
}

extern ca_status_t l2_classifier_rule_add (
CA_IN      ca_device_id_t                           device_id,
CA_IN      ca_uint32_t                          priority,
CA_IN      ca_classifier_key_t                  *key,
CA_IN      ca_classifier_key_mask_t             *key_mask,
CA_IN      ca_classifier_action_t               *action,
CA_OUT     ca_uint32_t                          *index);

ca_status_t ca_classifier_rule_add (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              priority,
    CA_IN  ca_classifier_key_t      *key,
    CA_IN  ca_classifier_key_mask_t *key_mask,
    CA_IN  ca_classifier_action_t   *action,
    CA_OUT ca_uint32_t              *index)
{
	ca_status_t rc = CA_E_OK;
	cls_rule_info_t rule;
	ca_uint32_t profile_id=0,  hash_profile_id=0, lspid,ldpid, port_type;
#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	ca_uint32_t mask_idx;
	ca_uint8_t tuple_idx;
	ca_uint32_t hash_idx;
	aal_hash_key_t hash_key;
	aal_hash_mask_t hash_mask;
	aal_hash_action_t hash_action;
#endif
	ca_uint32_t       l2fe_index;

	if (key == NULL || key_mask == NULL || action == NULL || index == NULL)
		return CA_E_ERROR;

    if(key_mask->src_port && key_mask->dest_port)
        return CA_E_NOT_SUPPORT;
	/* must set src_port */
	if (key_mask->src_port) {
		lspid = PORT_ID(key->src_port);
		port_type = PORT_TYPE(key->src_port);
	} else if(key_mask->dest_port){
        ldpid = PORT_ID(key->dest_port);
		port_type = PORT_TYPE(key->dest_port);
    }else{
		port_type = CA_PORT_TYPE_ETHERNET;
    }

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	memset(&rule, 0, sizeof(cls_rule_info_t));
	rule.rule.priority = priority;
	memcpy(&rule.rule.key, key, sizeof(ca_classifier_key_t));
	memcpy(&rule.rule.key_mask, key_mask, sizeof(ca_classifier_key_mask_t));
	memcpy(&rule.rule.action, action, sizeof(ca_classifier_action_t));
	/* check if rule exist */
	if (CA_E_EXISTS == (rc = cls_rule_list_rule_exist(&rule, index))) {
		/* rule found */
		ca_spin_unlock(cls_rule_lock);
		return CA_E_OK;
	}

	switch (port_type) {
		case  CA_PORT_TYPE_GPON:
		case CA_PORT_TYPE_EPON:
		case CA_PORT_TYPE_ETHERNET:
		case CA_PORT_TYPE_CPU:
			/* create hashlite rules */
			if (lspid == AAL_LPORT_ETH_NI7 && key_mask->key_handle &&
				(key->handle_type == CA_KEY_HANDLE_TYPE_LLID_COS || key->handle_type == CA_KEY_HANDLE_TYPE_GEM_INDEX)){
				if (CA_E_OK == (rc = cls_hashlite_rule_add(l3fe_hash_lite_profile, key, key_mask, action, &rule)))
					break;
				else if (rc == CA_E_EXISTS) {
					if (CA_E_OK == cls_rule_list_hashlite_rule_search(&rule,  index)) {
						if (ca_api_debug & API_DBG_CLS)
							ca_printf("%s: hashlite entry exists, index = %d, rc = %x\n", __func__, *index, rc);
					} else {
						if (ca_api_debug & API_DBG_CLS)
							ca_printf("%s: no hashlite entry exists\n", __func__);
						rc = CA_E_ERROR;
					}
					goto ERROR;
				}
			}

			/* config l2 cls rule */
			rc = l2_classifier_rule_add(device_id, priority, key, key_mask, action, &l2fe_index);
			if(rc != CA_E_OK) {
				goto ERROR;
			}
			rule.type |= CLS_RUL_INFO_L2_CLS;
			rule.hw_info.l2fe_info.l2fe_cls_vld = TRUE;
			rule.hw_info.l2fe_info.l2fe_cls_index = l2fe_index;
			break;
		case CA_PORT_TYPE_L2RP:
		case CA_PORT_TYPE_L3:
			/* config l3 cls rule */
			/* get cls profile_id from lspid */
			if (lspid == AAL_LPORT_ETH_NI7 || (key_mask->ingress_lan && key->ingress_lan == 0)) {
				profile_id = l3fe_classifier_profile_wan;
				hash_profile_id = l3fe_main_hash_profile_wan;
			} else if (lspid == AAL_LPORT_L3_WAN) {
				ca_spin_unlock(cls_rule_lock);
				return CA_E_NOT_SUPPORT;
			} else {
				profile_id = l3fe_classifier_profile_lan;
				hash_profile_id = l3fe_main_hash_profile_lan;
			}

			/* add the rule to l3 cls */
			if (CA_E_OK != (rc =l3_cls_rule_add(profile_id, priority, key, key_mask, action, &rule))) {
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
				goto ERROR;
#else
#ifdef CONFIG_LUNA_G3_SERIES
				// Translate CLS to MainHash is disabled!!
				goto ERROR;
#endif
				/* l3 cls fail, add to main hash */
				if (CA_E_OK != (rc = cls_hash_key_update(key, key_mask, &hash_key, &hash_mask, &rule)))
					goto ERROR;
				if (CA_E_OK != (rc = cls_hash_action_update(key, key_mask, action, &hash_action)))
					goto ERROR;
				/* ignore key.src_port field for wan profile */
				if (hash_profile_id == l3fe_main_hash_profile_wan) {
					hash_mask.lspid = 1;
					hash_key.lspid = 0;
				}
				/* add the rule to main hash */
				if (CA_E_OK != (rc = cls_hash_rule_add(hash_profile_id, priority, &hash_key, &hash_mask, &hash_action, &mask_idx, &tuple_idx, &hash_idx))) {
					if (ca_api_debug & API_DBG_CLS)
						ca_printf("%s: add hash rule fail, rc = %x\n", __func__, rc);
					goto ERROR;
				}
				rule.type |= CLS_RUL_INFO_HASH;
				rule.hw_info.hash.profile_id = hash_profile_id;
				rule.hw_info.hash.mask_idx = mask_idx;
				rule.hw_info.hash.tuple_idx = tuple_idx;
				rule.hw_info.hash.hash_idx = hash_idx;
#endif
			}
			break;
		default:
			rc = CA_E_PORT;
			goto ERROR;
	}

	/* add to rule list */
	if (CA_E_OK != (rc = cls_rule_list_add(&rule, index)))
		goto ERROR;

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	/* delete hw rule */
	cls_hw_rule_delete(&rule);
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}

ca_status_t ca_classifier_rule_get (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              index,
    CA_OUT ca_uint32_t              *priority,
    CA_OUT ca_classifier_key_t      *key,
    CA_OUT ca_classifier_key_mask_t *key_mask,
    CA_OUT ca_classifier_action_t   *action)
{
	cls_rule_info_t rule;
	ca_status_t rc;

	if (priority == NULL || key == NULL || key_mask == NULL || action == NULL)
		return CA_E_ERROR;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	if (CA_E_OK != (rc = cls_rule_list_get(index, &rule)))
		goto ERROR;

	*priority = rule.rule.priority;
	memcpy(key, &rule.rule.key, sizeof(ca_classifier_key_t));
	memcpy(key_mask, &rule.rule.key_mask, sizeof(ca_classifier_key_mask_t));
	memcpy(action, &rule.rule.action, sizeof(ca_classifier_action_t));

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}

ca_status_t ca_classifier_rule_iterate (
    CA_IN     ca_device_id_t 					 device_id,
    CA_OUT ca_iterator_t					*return_entry)
{
	ca_uint32_t i, start, e_sz, cnt;
	cls_rule_info_t rule;
	ca_classifier_rule_t *p_rule = return_entry->entry_data;
	ca_int32_t b_sz = return_entry->user_buffer_size;

	if (return_entry == NULL || return_entry->entry_data == NULL)
		return CA_E_PARAM;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	e_sz = sizeof(ca_classifier_rule_t);
	if (return_entry->prev_handle == NULL) {
		start = 0;
	} else {
		start = ((ca_uint32_t)(uintptr_t)return_entry->prev_handle - (ca_uint32_t)(uintptr_t)cls_rule_list)/sizeof(ca_uint32_t *);
	}

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	ca_spin_lock(cls_rule_lock);
	for (i = start, cnt = 0; i < CLS_RULE_MAX && b_sz >= e_sz && cnt < return_entry->entry_count; i++) {
		if (CA_E_OK != cls_rule_list_get(i+1, &rule))
			continue;
		/* copy data */
		p_rule->index = return_entry->entry_index[cnt] = i+1;
		p_rule->priority = rule.rule.priority;
		memcpy(&p_rule->key, &rule.rule.key, sizeof(ca_classifier_key_t));
		memcpy(&p_rule->key_mask, &rule.rule.key_mask, sizeof(ca_classifier_key_mask_t));
		memcpy(&p_rule->action, &rule.rule.action, sizeof(ca_classifier_action_t));
		p_rule++;
		b_sz -= e_sz;
		cnt++;
	}
	ca_spin_unlock(cls_rule_lock);

	return_entry->entry_count = cnt;
	return_entry->prev_handle = &cls_rule_list[i];

	if (i >= CLS_RULE_MAX && cnt == 0)
		return CA_E_UNAVAIL;

	return CA_E_OK;
}

ca_status_t ca_classifier_rule_delete (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t index)
{
	ca_status_t rc;
	cls_rule_info_t rule;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	/* get hw_info */
	if (CA_E_OK != (rc = cls_rule_list_get(index, &rule)))
		goto ERROR;

	/* delete hw rules */
	if (rule.refcnt == 1) {
		if (CA_E_OK != (rc = cls_hw_rule_delete(&rule)))
			goto ERROR;
	}

	/* delete rule list */
	cls_rule_list_delete(index);

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}

ca_status_t ca_classifier_rule_delete_all (
    CA_IN ca_device_id_t device_id)
{
	ca_uint32_t index;
	ca_status_t rc;
	cls_rule_info_t rule;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	for (index = 1; index <= CLS_RULE_MAX; index++) {
		/* get hw_info */
		if (CA_E_OK != (rc = cls_rule_list_get(index, &rule)))
			continue;

		/* delete hw rules */
		cls_hw_rule_delete(&rule);
	}

	/* delete rule list */
	cls_rule_list_delete_all();

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;
}

extern ca_status_t l2_classifier_port_default_action_get (
	CA_IN ca_device_id_t                    device_id,
	CA_IN ca_port_id_t                   port_id,
	CA_OUT ca_classifier_default_action_t *p_action
);

ca_status_t ca_classifier_port_default_action_get (
    CA_IN     ca_device_id_t                    device_id,
    CA_IN     ca_port_id_t                   port_id,
    CA_OUT ca_classifier_default_action_t *p_action
)
{
	ca_status_t rc = CA_E_OK;
	ca_uint32_t lspid, port_type;

	if (p_action == NULL)
		return CA_E_ERROR;

	lspid = PORT_ID(port_id);
	port_type = PORT_TYPE(port_id);

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	switch (port_type) {
		case  CA_PORT_TYPE_GPON:
		case CA_PORT_TYPE_EPON:
		case CA_PORT_TYPE_ETHERNET:
		case CA_PORT_TYPE_CPU:
			/* get l2 cls rule */
			if (CA_E_OK != (rc = l2_classifier_port_default_action_get(device_id, port_id, p_action)))
				goto ERROR;
			break;
		case CA_PORT_TYPE_L2RP:
		case CA_PORT_TYPE_L3:
			if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_L3_LAN || lspid == AAL_LPORT_ETH_NI7) {
				if (CA_E_OK != (rc = cls_rule_list_def_rule_get(lspid, p_action)))
					goto ERROR;
			} else {
				rc = CA_E_PORT;
				goto ERROR;
			}
			break;
		default:
			rc = CA_E_PORT;
			goto ERROR;
	}

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}
extern ca_status_t l2_classifier_port_default_action_set (
        CA_IN ca_device_id_t                    device_id,
        CA_IN ca_port_id_t                   port_id,
        CA_IN ca_classifier_default_action_t *p_action
);


ca_status_t ca_classifier_port_default_action_set (
    CA_IN ca_device_id_t                    device_id,
    CA_IN ca_port_id_t                   port_id,
    CA_IN ca_classifier_default_action_t *p_action
)
{
	ca_status_t rc = CA_E_OK;
	ca_classifier_action_t action;
	cls_action_t aal_action;
	ca_uint32_t profile_id=0, lspid, port_type;

	if (p_action == NULL)
		return CA_E_ERROR;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	lspid = PORT_ID(port_id);
	port_type = PORT_TYPE(port_id);

	switch (port_type) {
		case  CA_PORT_TYPE_GPON:
		case CA_PORT_TYPE_EPON:
		case CA_PORT_TYPE_ETHERNET:
		case CA_PORT_TYPE_CPU:
			/* config l2 cls rule */
			if (CA_E_OK != (rc = l2_classifier_port_default_action_set(device_id,port_id,p_action)))
				goto ERROR;
			break;
		case CA_PORT_TYPE_L2RP:
		case CA_PORT_TYPE_L3:
			/* config l3 cls rule */
			if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_L3_LAN || lspid == AAL_LPORT_ETH_NI7) {
				/* update ca_classifier_action_t to cls_action_t */
				memset(&action, 0, sizeof(ca_classifier_action_t));
				action.forward = p_action->action_type;
				action.dest.intf = p_action->dest.intf;
				action.dest.port = p_action->dest.port;
				if (CA_E_OK != (rc = cls_fib_entry_update(NULL, NULL, &action, &aal_action)))
					goto ERROR;
				if (p_action->action_type == CA_CLASSIFIER_FORWARD_INTERFACE) {
					aal_action.t2_ctrl_vld = 1;
					aal_action.t2_ctrl = 0xf;
					aal_action.t5_ctrl_vld = 1;
					if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_ETH_NI7)
						aal_action.t5_ctrl = 0x0;
					else
						aal_action.t5_ctrl = 0xf;
				}
				if (p_action->action_type == CA_CLASSIFIER_FORWARD_FE) {
					aal_action.t2_ctrl_vld = 1;
					aal_action.t3_ctrl_vld = 1;
					aal_action.t3_ctrl = 0x1;
					aal_action.t4_ctrl_vld = 1;
					aal_action.t4_ctrl = l3fe_hash_lite_profile;
					aal_action.t5_ctrl_vld = 1;
					if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_ETH_NI7) {
						aal_action.t2_ctrl = l3fe_main_hash_profile_wan;
						aal_action.t5_ctrl = 0x0;
					} else {
						aal_action.t2_ctrl = l3fe_main_hash_profile_lan;
						aal_action.t5_ctrl = 0xf;
					}
				}

				/* get cls profile_id from lspid */
				if (lspid == AAL_LPORT_L3_WAN || lspid == AAL_LPORT_ETH_NI7) {
					profile_id = l3fe_classifier_profile_wan;
				} else {
					profile_id = l3fe_classifier_profile_lan;
				}

				if (CA_E_OK != (rc = aal_l3_cls_default_set(0, profile_id, CL_RSLT_TYPE_0, &aal_action)))
					goto ERROR;
			} else {
				rc = CA_E_PORT;
				goto ERROR;
			}
			/* update def rule list */
			if (CA_E_OK != (rc = cls_rule_list_def_rule_set(lspid, p_action)))
				goto ERROR;
			break;
		default:
			rc = CA_E_PORT;
			goto ERROR;
	}

	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}

ca_status_t ca_cls_rule_dump(void)
{
	ca_uint32_t i, cnt = 0;
	cls_rule_info_t rule;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init()) {
		ca_printf("CLS rule list is not initialized.\n");
		return CA_E_INIT;
	}

	ca_spin_lock(cls_rule_lock);
	for (i=1; i<=CLS_RULE_MAX; i++) {
		if (CA_E_OK != cls_rule_list_get(i, &rule))
			continue;
		/* print data */
		ca_printf("Index %04d | Type %08x | Refcnt %04d | CRC %08x\n", i, rule.type, rule.refcnt, rule.rule_crc32);
		if (rule.type & CLS_RUL_INFO_CLS) {
			ca_printf("L3 CLS | profile %d | rule idx %08x | dev id %d | key idx %d/%d\n", rule.hw_info.cls.profile_id, rule.hw_info.cls.rule_idx,
				CLS_DEV_ID(rule.hw_info.cls.rule_idx), CLS_TBL_ID(rule.hw_info.cls.rule_idx), CLS_KEY_ID(rule.hw_info.cls.rule_idx));
		}
		if (rule.type & CLS_RUL_INFO_HASH) {
			ca_printf("L3 HASH | profile %d | mask idx %d | tuple idx %d | hash idx %d\n", rule.hw_info.hash.profile_id,
					rule.hw_info.hash.mask_idx, rule.hw_info.hash.tuple_idx, rule.hw_info.hash.hash_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_ETHERTYPE) {
			ca_printf("CAM ETHERTYPE | entry idx %d \n", rule.hw_info.ethertype.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_PPP_PROT) {
			ca_printf("CAM PPP PROT | entry idx %d \n", rule.hw_info.ppp_prot.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_SPORT) {
			ca_printf("CAM L4 SPORT | entry idx %d \n", rule.hw_info.l4_sport.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_DPORT) {
			ca_printf("CAM L4 DPORT | entry idx %d \n", rule.hw_info.l4_dport.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_SPCL_HDR) {
			ca_printf("CAM SPCL HDR | entry idx %08x \n", rule.hw_info.spcl_hdr.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_CAM_PKT_LEN) {
			ca_printf("CAM PKT LEN | entry idx %d \n", rule.hw_info.pkt_len.rslt_idx);
		}
		if (rule.type & CLS_RUL_INFO_L2_CLS) {
			ca_printf("L2 CLS | vld %d |entry idx %08x \n", rule.hw_info.l2fe_info.l2fe_cls_vld, rule.hw_info.l2fe_info.l2fe_cls_index);
		}
		if (rule.type & CLS_RUL_INFO_HASH_LITE) {
			ca_printf("L3 HASHLITE | profile %d | mask idx %d | tuple idx %d | hash idx %d\n", rule.hw_info.hash.profile_id,
					rule.hw_info.hash.mask_idx, rule.hw_info.hash.tuple_idx, rule.hw_info.hash.hash_idx);
		}
		ca_printf("|------------------------------------------------------------------|\n");
		cnt++;
	}
	ca_printf("TOTAL ENTRIES %d \n", cnt);
	ca_spin_unlock(cls_rule_lock);

	return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_classifier_l3_cls_rule_add(
	CA_IN	ca_device_id_t device_id,
	CA_IN	ca_uint32_t priority,
	CA_IN	ca_classifier_key_t *key,
	CA_IN	ca_classifier_key_mask_t *key_mask,
	CA_IN	ca_classifier_action_t *action,
	CA_IN	ca_uint32_t *index,
	CA_IN	unsigned int aal_customize)
{
	ca_status_t rc = CA_E_OK;
	ca_uint32_t lspid, ldpid, port_type, profile_id, hw_idx;
	cls_rule_info_t rule;
	l3_cls_rule_t aal_rule;
	l3_cam_ppp_prot_tbl_entry_t ppp_prot;
	ca_uint32_t cam_ppp_prot_rslt_idx, cam_rslt_idx;
	l3_cam_pkt_len_tbl_entry_t pkt_len;
	l3_cam_spcl_hdr_tbl_entry_t spcl_hdr;

	if (key == NULL || key_mask == NULL || action == NULL || index == NULL)
		return CA_E_ERROR;

    if(key_mask->src_port && key_mask->dest_port)
        return CA_E_NOT_SUPPORT;
	/* must set src_port */
	if (key_mask->src_port) {
		lspid = PORT_ID(key->src_port);
		port_type = PORT_TYPE(key->src_port);
	} else if(key_mask->dest_port){
        ldpid = PORT_ID(key->dest_port);
		port_type = PORT_TYPE(key->dest_port);
    }else{
		port_type = CA_PORT_TYPE_ETHERNET;
    }

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	memset(&rule, 0, sizeof(cls_rule_info_t));
	rule.rule.priority = priority;
	memcpy(&rule.rule.key, key, sizeof(ca_classifier_key_t));
	memcpy(&rule.rule.key_mask, key_mask, sizeof(ca_classifier_key_mask_t));
	memcpy(&rule.rule.action, action, sizeof(ca_classifier_action_t));
	/* check if rule exist */
	if(aal_customize > 0) {
		//not check rule exist or not, due to it may use same cls pattern with different aal field
	}
	else if (CA_E_EXISTS == (rc = cls_rule_list_rule_exist(&rule, index))) {
		/* rule found */
		ca_spin_unlock(cls_rule_lock);
		return CA_E_OK;
	}	

	if (lspid == AAL_LPORT_ETH_NI7) {
		profile_id = l3fe_classifier_profile_wan;
	} else if (lspid == AAL_LPORT_L3_WAN) {
		ca_spin_unlock(cls_rule_lock);
		return CA_E_NOT_SUPPORT;
	} else {
		profile_id = l3fe_classifier_profile_lan;
	}

	/* update ca_classifier_key_t to cls_key_t */
	if (CA_E_OK != (rc = cls_key_entry_update(key, key_mask, &aal_rule.key, &rule)))
		goto ERROR;

	/* update ca_classifier_action_t to cls_action_t */
	if (CA_E_OK != (rc = cls_fib_entry_update(key, key_mask, action, &aal_rule.action)))
		goto ERROR;

	/* msk_ctrl will be merged from l3 intf rule(CL_RSLT_TYPE_0) after rule hit */
	aal_rule.pri = priority;
	if(key_mask->merge_prio)
		aal_rule.rslt_type = key->merge_prio;
	else
		aal_rule.rslt_type = CL_RSLT_TYPE_1;

	/* ignore key.src_port field for wan profile */
	if (profile_id == l3fe_classifier_profile_wan) {
		aal_rule.key.msk_lspid = 1;
		aal_rule.key.lspid = 0x3f;
	}

	/* Customize for aal */
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPP_LCP) {
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = 0xC021;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PPP Protocol from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		aal_rule.key.msk_ppp_protocol = 0;
		aal_rule.key.ip_vld = 0;
		aal_rule.key.msk_ip_vld = 0;

		aal_rule.action.keep_orig_pkt_vld = 1;
		aal_rule.action.keep_orig_pkt = 1;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPP_IPCP) {
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = 0x8021;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PPP Protocol from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		aal_rule.key.msk_ppp_protocol = 0;
		aal_rule.key.ip_vld = 0;
		aal_rule.key.msk_ip_vld = 0;
		
		aal_rule.action.keep_orig_pkt_vld = 1;
		aal_rule.action.keep_orig_pkt = 1;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPP_IP6CP) {
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = 0x8057;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PPP Protocol from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		aal_rule.key.msk_ppp_protocol = 0;
		aal_rule.key.ip_vld = 0;
		aal_rule.key.msk_ip_vld = 0;
		
		aal_rule.action.keep_orig_pkt_vld = 1;
		aal_rule.action.keep_orig_pkt = 1;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_IGNORE_SRC_PORT) {
		aal_rule.key.msk_lspid = 1;
		aal_rule.key.lspid = 0x3f;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_NOT_BROADCAST) {
		aal_rule.key.mac_da_bc = 0;	//Set if the MAC DA is broadcast address (48hffff_ffff_ffff)
		aal_rule.key.msk_mac_da_bc = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX0) {
		/* CAM table, PKT LEN */
		memset(&pkt_len, 0, sizeof(l3_cam_pkt_len_tbl_entry_t));
		pkt_len.vld = 1;
		pkt_len.pkt_len_low = 0;
		pkt_len.pkt_len_hi = 72;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PKT_LEN, &pkt_len, &cam_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PKT LEN idx 0 from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.pktlen_rng_match_vec = 1 << cam_rslt_idx;
		aal_rule.key.pktlen_rng_match_mask = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPP_IPV4) {
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = 0x0021;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PPP Protocol from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		aal_rule.key.msk_ppp_protocol = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPP_IPV6) {
		/* CAM table, PPP Protocol */
		memset(&ppp_prot, 0, sizeof(l3_cam_ppp_prot_tbl_entry_t));
		ppp_prot.vld = 1;
		ppp_prot.ppp_prot = 0x0057;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PPP_PROT, &ppp_prot, &cam_ppp_prot_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PPP Protocol from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.ppp_protocol_enc = cam_ppp_prot_rslt_idx + 1;
		aal_rule.key.msk_ppp_protocol = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_KEEP_ORIG) {
		aal_rule.action.keep_orig_pkt_vld = 1;
		aal_rule.action.keep_orig_pkt = 1;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX1) {
		/* CAM table, PKT LEN */
		memset(&pkt_len, 0, sizeof(l3_cam_pkt_len_tbl_entry_t));
		pkt_len.vld = 1;
		pkt_len.pkt_len_low = 129;
		pkt_len.pkt_len_hi = 9216;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PKT_LEN, &pkt_len, &cam_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PKT LEN idx 1 from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.pktlen_rng_match_vec = 1 << cam_rslt_idx;
		aal_rule.key.pktlen_rng_match_mask = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_SPEC_PKT_L4_L2TP_PORT) {
		/* CAM table, SPEC PKT */
		memset(&spcl_hdr, 0, sizeof(l3_cam_spcl_hdr_tbl_entry_t));
		spcl_hdr.vld = 1;
		spcl_hdr.type = L3_CAM_SPCL_HDR_L3_PLUS;
		spcl_hdr.ms_vld = 1;	//sport
		spcl_hdr.mask = 0xffff;
		spcl_hdr.key = 0x06A5;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_SPCL_HDR, &spcl_hdr, &cam_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get SPEC PKT (l4 sport) from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.spcl_pkt_hdr_mtch = 1 << cam_rslt_idx;

		memset(&spcl_hdr, 0, sizeof(l3_cam_spcl_hdr_tbl_entry_t));
		spcl_hdr.vld = 1;
		spcl_hdr.type = L3_CAM_SPCL_HDR_L3_PLUS;
		spcl_hdr.ms_vld = 0;	//dport
		spcl_hdr.mask = 0xffff;
		spcl_hdr.key = 0x06A5;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_SPCL_HDR, &spcl_hdr, &cam_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get SPEC PKT (l4 dport) from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.spcl_pkt_hdr_mtch |= 1 << cam_rslt_idx;
		aal_rule.key.spcl_pkt_hdr_mtch_msk = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_MCAST_FORWARD) {
		aal_rule.action.vlan_cnt = 1;
		aal_rule.action.top_tpid_enc = 1;
		aal_rule.action.top_dei_sel = 1;
		aal_rule.action.top_vid = 0xFF0;	/* ref: l3_mcg_node_update, fake_vid */
		aal_rule.action.vlan_vld = 1;

		aal_rule.action.t2_ctrl = 0xF;	/* ref: aal_l3_cls_tbl_init, HASH_PROFILE_2 */
		aal_rule.action.t2_ctrl_vld = 1;
		aal_rule.action.t3_ctrl = 0xF;
		aal_rule.action.t3_ctrl_vld = 1;
		aal_rule.action.t4_ctrl = 0xF;
		aal_rule.action.t4_ctrl_vld = 1;
		aal_rule.action.t5_ctrl = 0xF;
		aal_rule.action.t5_ctrl_vld = 1;
		aal_rule.action.stage2_ctrl = 0;
		aal_rule.action.stage2_ctrl_vld = 1;
		aal_rule.action.stage3_ctrl = 1;
		aal_rule.action.stage3_ctrl_vld = 1;
		aal_rule.action.keep_orig_pkt = 0;
		aal_rule.action.keep_orig_pkt_vld = 1;
		aal_rule.action.mc = 0;
		aal_rule.action.mcgid = AAL_LPORT_L3_LAN;
		aal_rule.action.deepq = 0;
		aal_rule.action.permit = 1;
		aal_rule.action.dpid_pri = 1;
		aal_rule.action.dpid_vld = 1;
		aal_rule.action.no_drop = 1;
		aal_rule.action.no_drop_vld = 1;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PKT_LEN_RANGE_IDX2) {
		/* CAM table, PKT LEN */
		memset(&pkt_len, 0, sizeof(l3_cam_pkt_len_tbl_entry_t));
		pkt_len.vld = 1;
		pkt_len.pkt_len_low = 124;
		pkt_len.pkt_len_hi = 128;
		rc = aal_entry_get(AAL_TABLE_L3_CAM_PKT_LEN, &pkt_len, &cam_rslt_idx);
		if (rc != AAL_E_OK) {
			ca_printf("%s: Fail to get PKT LEN idx 2 from CAM table, rc = %x\n", __func__, rc);
			goto ERROR;
		}
		aal_rule.key.pktlen_rng_match_vec = 1 << cam_rslt_idx;
		aal_rule.key.pktlen_rng_match_mask = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_RSLT_TYPE_0) {
		aal_rule.rslt_type = CL_RSLT_TYPE_0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_MAC_DA_IP_MC) {
		aal_rule.key.mac_da_ip_mc = 1;	//Set if the MAC DA is ((MAC DA[47:24] == h01-00-5E) && (etherType == IPV4)) |((MAC DA[47:32] == h33-33) && (etherType == IPV6))
		aal_rule.key.msk_mac_da_ip_mc = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_MAC_DA_RSVD) {
		aal_rule.key.mac_da_rsvd = 1;	//Set if the MAC DA[47:8] = 40h01_80_C2_00_00
		aal_rule.key.msk_mac_da_rsvd = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_PPPOE_SESID_0) {
		aal_rule.key.pppoe_session_id = 0;
		aal_rule.key.msk_pppoe_session = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_MAC_DA_AN_HIT) {
		aal_rule.key.mac_da_an_hit = 1;
		aal_rule.key.msk_mac_da_an_hit = 0;
	}
	if(aal_customize&CA_CLASSIFIER_AAL_L3_POL_REMAP_GRP) {
		aal_rule.action.t2_ctrl = 0x4;	//hash profile 4
		aal_rule.action.t2_ctrl_vld = 1;
		aal_rule.action.pol_vld = 1;
		aal_rule.action.pol_grp_vld = 1;
		aal_rule.action.pol_grp_id = aal_rule.action.pol_base;
		//hash profile 4 will use pol_grp_id, so reset pol_id
		aal_rule.action.pol_en = 0;
		aal_rule.action.pol_sel = 0;
		aal_rule.action.pol_base = 0;
	}

	if (CA_E_OK != (rc = aal_l3_cls_add(0, profile_id, &aal_rule, &hw_idx)))
		goto ERROR;

	rule.type |= CLS_RUL_INFO_CLS;
	rule.hw_info.cls.profile_id = profile_id;
	rule.hw_info.cls.rule_idx = hw_idx;

	/* add to rule list */
	if (CA_E_OK != (rc = cls_rule_list_add(&rule, index)))
		goto ERROR;

	ca_spin_unlock(cls_rule_lock);

	return CA_E_OK;

ERROR:
	/* delete hw rule */
	cls_hw_rule_delete(&rule);

	ca_spin_unlock(cls_rule_lock);
	
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);

	if (rc == AAL_E_TBLFULL)
		return CA_E_FULL;
	else if (rc == AAL_E_ENTRYNOTFOUND)
		return CA_E_NOT_FOUND;
	else
		return CA_E_ERROR;
}

/* get aal hw table index by ca rule idx */
ca_status_t ca_classifier_l3_cls_rule_idx_get (
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              index,
    CA_OUT  ca_uint32_t 	*rule_idx)
{
	cls_rule_info_t rule;
	ca_status_t rc;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init())
		return CA_E_INIT;

	ca_spin_lock(cls_rule_lock);

	if (CA_E_OK != (rc = cls_rule_list_get(index, &rule)))
		goto ERROR;

	if(rule.type&CLS_RUL_INFO_CLS)
		*rule_idx = rule.hw_info.cls.rule_idx;
	else{
		rc = CA_E_PARAM;
		goto ERROR;
	}
	
	ca_spin_unlock(cls_rule_lock);
	return CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);
	return rc;
}

ca_status_t ca_cls_rule_get(
	CA_IN  ca_device_id_t			   device_id,
	CA_IN  ca_uint32_t				index,
	CA_OUT	cls_rule_info_t 	*rule_info)
{
	ca_uint32_t i, cnt = 0;
	ca_status_t rc;
	cls_rule_info_t rule;

	if (rule_info == NULL)
		return CA_E_ERROR;

	/* check if init rule list */
	if (CA_E_OK != cls_rule_list_init()) {
		ca_printf("CLS rule list is not initialized.\n");
		return CA_E_INIT;
	}

	ca_spin_lock(cls_rule_lock);

	if (CA_E_OK != (rc = cls_rule_list_get(index, &rule)))
		goto ERROR;

	memcpy(rule_info, &rule, sizeof(cls_rule_info_t));
	rc = CA_E_OK;

ERROR:
	ca_spin_unlock(cls_rule_lock);
	if (ca_api_debug & API_DBG_CLS)
		ca_printf("%s: rc = %x\n", __func__, rc);

	return rc;
}

extern ca_status_t aal_l2_cls_used_entry_print (void);
extern ca_status_t l2_classifier_rule_info_get (
CA_IN		ca_device_id_t				   device_id,
CA_IN		ca_uint32_t 				index,
CA_OUT		ca_uint32_t 				*key_type,
CA_OUT      ca_uint32_t                 *priority);

ca_status_t ca_classifier_l3_cls_rule_dump(
CA_IN		ca_uint32_t 				hw_index,
CA_IN		ca_uint32_t 				priority,
CA_IN		ca_int32_t 					index)
{
	char string_buf[8];
	char* string_print;

	if(index > 0)
		snprintf(string_buf, sizeof(string_buf), " SW[%2d]", index);
	else
		snprintf(string_buf, sizeof(string_buf), "AAL[%2d]", ~index+1);	//hwnat rsv acl

	switch(CLS_KEY_TYPE(hw_index)) {
		case CL_IF_ID_KEY: string_print = "IF_ID_KEY(1/4)"; break;
		case CL_IPV4_TUNNEL_ID_KEY: string_print = "IPV4_TUNNEL_ID_KEY(1)"; break;
		case CL_IPV4_SHORT_KEY: string_print = "IPV4_SHORT_KEY(1/2)"; break;
		case CL_IPV6_SHORT_KEY: string_print = "IPV6_SHORT_KEY(1)"; break;
		case CL_SPCL_PKT_KEY: string_print = "SPCL_PKT_KEY(1/2)"; break;
		case CL_MCST_KEY: string_print = "MCST_KEY(1)"; break;
		case CL_FULL_KEY: string_print = "FULL_KEY(2)"; break;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
		case CL_FLOW_KEY: string_print = "FLOW_KEY(2)"; break;
#endif
		default: string_print = "UNKNOWN"; break;
	}
	ca_printf("|%s-L3: DEV_IDX %d | %s | PRI %2d | TBL/KEY_IDX: %04d / %d | %s\n", 
						string_buf, CLS_DEV_ID(hw_index), CLS_TBL_ID(hw_index)<(L3_CLS_KEY_TBL_ENTRY_MAX/2)?"WAN":"LAN", priority, CLS_TBL_ID(hw_index), CLS_KEY_ID(hw_index), string_print);

	return CA_E_OK;
}

ca_status_t ca_used_aal_entry_print(
	CA_IN		ca_device_id_t				   device_id)
{
	ca_status_t ret;
	ca_uint8_t port_idx = 0, table_id;
	ca_uint8_t l2_port_index[AAL_LPORT_CPU_7+1][2];
	ca_uint16_t index, i, start_l3_key = 0, count_l3_key = 128, start_l3_fib = 512, count_l3_fib = 8;
	ca_uint32_t hw_index, key_type, l2_priority;
	cls_rule_info_t cls_rule;
	char* string_print;
	aal_ilpb_cfg_t cfg_port;

	for(index=AAL_LPORT_ETH_NI0; index<=AAL_LPORT_CPU_7; index++) {
		ret = aal_port_ilpb_cfg_get(device_id,index,&cfg_port);
		if(ret == CA_E_OK) {
			l2_port_index[index][0] = cfg_port.cls_start;
			l2_port_index[index][1] = cfg_port.cls_length;
		}
	}
	
#if !defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	ca_printf("|===== CA Classifier SW & HW Mapping Table ==============================|\n");
#endif
	for(index=0; index<CLS_RULE_MAX; index++) {
		ret = ca_cls_rule_get(device_id, index, &cls_rule);
		if(!((ret == CA_E_OK) && ((cls_rule.type&CLS_RUL_INFO_CLS) || (cls_rule.type&CLS_RUL_INFO_L2_CLS))))
			continue;
		if(cls_rule.type&CLS_RUL_INFO_L2_CLS) {
			hw_index = cls_rule.hw_info.l2fe_info.l2fe_cls_index;
			ret = l2_classifier_rule_info_get(device_id, hw_index, &key_type, &l2_priority);
			if(ret == CA_E_OK) {
				switch(key_type) {
					case 0: string_print = "L2R(1/2)"; break;
					case 1: string_print = "L3R(1/2)"; break;
					case 2: string_print = "CMR(1/2)"; break;
					case 3: string_print = "IPV4R(1)"; break;
					case 4: string_print = "IPV6(1)"; break;
					case 5: string_print = "IPV6_LONG(2)"; break;
					case 6: string_print = "VLAN_FIB"; break;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
					case 7: string_print = "SATURN_VLAN_FIB"; break;
#endif
					default: string_print = "UNKNOWN"; break;
				}
			}
			for(i=AAL_LPORT_ETH_NI0; i<=AAL_LPORT_CPU_7; i++) {
				if(l2_port_index[i][1] == 0)
					continue;
				if(((hw_index/2) >= l2_port_index[i][0]) && ((hw_index/2) < (l2_port_index[i][0]+l2_port_index[i][1]))) {
					port_idx = i;
					break;
				}
			}
			if(ret == CA_E_OK)
				ca_printf("| SW[%2d]-L2: PORT_IDX %2x | PRI %d | HW/SUB_IDX %04d / %d | %s\n", index, port_idx, l2_priority, hw_index/2, hw_index%2, string_print);
			else
				ca_printf("| SW[%2d]-L2: PORT_IDX %2x | HW/SUB_IDX %04d / %d | ERROR!\n", index, port_idx, hw_index/2, hw_index%2);
		} else {
			ca_classifier_l3_cls_rule_dump(cls_rule.hw_info.cls.rule_idx, cls_rule.rule.priority, index);
		}
	}

	ret = aal_l2_cls_used_entry_print();
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to dump l2 cls. (ret=%d)\n", ret);
	}

	table_id = AAL_TABLE_L3_CLS_KEY;
	ret = aal_used_entry_print(table_id, start_l3_key, count_l3_key);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to dump for table id %lu. (ret=%d)\n", table_id, ret);
	}

	table_id = AAL_TABLE_L3_CLS_FIB;
	ret = aal_used_entry_print(table_id, start_l3_fib, count_l3_fib);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to dump for table id %lu. (ret=%d)\n", table_id, ret);
	}

	return CA_E_OK;
}

ca_status_t ca_used_aal_entry_print_by_fib_idx(
	CA_IN		ca_device_id_t				   device_id,
	CA_IN 		unsigned int 					fib_idx)
{
	ca_uint8_t table = AAL_TABLE_L3_CLS_KEY;
	ca_int32_t status, key_vld, key_idx, fib_tbl_idx, key_tbl_idx, index;
	l3_cls_key_t entry;
	l3_cls_key_type_t key_type;
	cls_rule_info_t cls_rule;

	if(fib_idx < 0)
		return CA_E_OK;

	for (key_tbl_idx = 0; key_tbl_idx < L3_CLS_KEY_TBL_ENTRY_MAX; key_tbl_idx++) {
		memset(&entry, 0, sizeof(l3_cls_key_t));
		status = aal_entry_get_by_idx(table, key_tbl_idx, entry.cls_key_info.key_tbl_data);
		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR))
			continue;

		key_vld = entry.cls_key_info.key_tbl_data[0].cls_key_valid;
		key_type = entry.cls_key_info.key_tbl_data[0].cls_key_type;
		if(key_vld == 0)
			continue;
		if (key_type == CL_FULL_KEY) {
			key_tbl_idx++;
			status = aal_entry_get_by_idx(table, key_tbl_idx, &entry.cls_key_info.key_tbl_data[1]);
		}
#if defined(CONFIG_ARCH_CORTINA_VENUS)
		else if (key_type == CL_FLOW_KEY) {
			key_tbl_idx++;
			status = aal_entry_get_by_idx(table, key_tbl_idx, &entry.cls_key_info.key_tbl_data[1]);
		}
#endif
		if ((status == AAL_E_ENTRYNOTRSVD) || (status == AAL_E_ENTRYNOTFOUND) || (status == AAL_E_NULLPTR))
			continue;

		switch(key_type) {	/*ref: __l3_cls_fib_idx_get()*/
			case CL_IF_ID_KEY:
				for (key_idx = 0; key_idx < CL_4_KEY_NUM; key_idx++) {
					if (!(key_vld & (0x1 << key_idx)))
						continue;
					fib_tbl_idx = (key_tbl_idx << 2) | key_idx;
					if(fib_idx == fib_tbl_idx)
						break;
				}
				break;
			case CL_IPV4_SHORT_KEY:
			case CL_SPCL_PKT_KEY:
				for (key_idx = 0; key_idx < CL_2_KEY_NUM; key_idx++) {
					if (!(key_vld & (0x1 << key_idx)))
						continue;
					fib_tbl_idx = (key_tbl_idx << 2) | (key_idx << 1);
					if(fib_idx == fib_tbl_idx)
						break;
				}
				break;
			case CL_FULL_KEY:
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			case CL_FLOW_KEY:
#endif
				fib_tbl_idx = ((key_tbl_idx >> 1) << 3) | 0x4;
				break;
			default:
				/* 1 keys: CL_IPV4_TUNNEL_ID_KEY, CL_IPV6_SHORT_KEY, CL_MCST_KEY */
				fib_tbl_idx = (key_tbl_idx << 2);
				break;
		}

		//printk(KERN_INFO "Debug: KEY TYPE %d | KEY TBL %d | KEY IDX %d | FIB IDX %d\n", key_type, key_tbl_idx, key_idx, fib_tbl_idx);
		if(fib_idx == fib_tbl_idx) {
			if(key_type == CL_FULL_KEY)
				key_tbl_idx--;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			else if(key_type == CL_FLOW_KEY)
				key_tbl_idx--;
#endif
			break;
		}
		key_idx = 0;
	}

	if(fib_idx == fib_tbl_idx) {
		status = aal_used_entry_print(table, key_tbl_idx, 1);
		if (status != AAL_E_OK) {
			printk(KERN_ERR "Fail to dump L3 CLS for FIB %d/KeyTBL %d/Key %d. (ret=%d)\n", fib_tbl_idx, key_tbl_idx, key_idx, status);
		}
	}
	
	for(index=0; index<CLS_RULE_MAX; index++) {
		status = ca_cls_rule_get(0, index, &cls_rule);
		if(!((status == CA_E_OK) && (cls_rule.type&CLS_RUL_INFO_CLS)))
			continue;
		if((CLS_TBL_ID(cls_rule.hw_info.cls.rule_idx) == key_tbl_idx) && (CLS_KEY_ID(cls_rule.hw_info.cls.rule_idx) == key_idx)) {
			ca_classifier_l3_cls_rule_dump(cls_rule.hw_info.cls.rule_idx, cls_rule.rule.priority, index);
			break;
		}
	}

	return CA_E_OK;
}

EXPORT_SYMBOL(ca_classifier_l3_cls_rule_add);
EXPORT_SYMBOL(ca_classifier_l3_cls_rule_idx_get);
EXPORT_SYMBOL(ca_classifier_l3_cls_rule_dump);
EXPORT_SYMBOL(ca_used_aal_entry_print);
EXPORT_SYMBOL(ca_used_aal_entry_print_by_fib_idx);
#endif

EXPORT_SYMBOL(ca_classifier_rule_add);
EXPORT_SYMBOL(ca_classifier_rule_get);
EXPORT_SYMBOL(ca_classifier_rule_iterate);
EXPORT_SYMBOL(ca_classifier_rule_delete);
EXPORT_SYMBOL(ca_classifier_rule_delete_all);
EXPORT_SYMBOL(ca_classifier_port_default_action_get);
EXPORT_SYMBOL(ca_classifier_port_default_action_set);
EXPORT_SYMBOL(ca_cls_rule_dump);

