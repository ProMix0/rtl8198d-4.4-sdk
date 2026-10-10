/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * ca_l2cos.c: L3 Class-of-Service (cos)
 * Set cos in HashLite according to certain packet fields.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/proc_fs.h>
#include <soc/cortina/cortina-soc.h>
#include <linux/list.h>
#include "aal_port.h"
#include "ca_mut.h"
#include "route.h"
#include "route_internal.h"
#include "aal_hashlite.h"
#include "aal_l3_if.h"
#include "ca_l3cos.h"
#include "ca_adapt_proc.h"

struct proc_dir_entry *proc_l3cos = NULL;

/* rule type */
enum {
	RULE_INVALID = 0,
	RULE_EGRESS_UNTAG,
	RULE_EGRESS_VID,
	RULE_EGRESS_8021P,
	RULE_EGRESS_VID_8021P,
};

/* contains L3COS rule */
struct l3cos_node {
	/* search key */
	int32_t			egress_top_vid;		/* -1 means untag */
	int32_t			egress_top_8021p;

	/* data; result */
	ca_uint32_t		cos;

	/* linked to l3cos_node */
	struct list_head	list;

	/* linked to hash_node */
	struct list_head	hash_list;
};

/* contains hash info from L3COS rule */
struct hash_node {
	aal_hash_key_t		hash_key;
	uint32_t		rslt_idx;

	struct list_head	list;
};

static LIST_HEAD(l3cos_list);
static DEFINE_RWLOCK(l3cos_list_lock);

static ca_int32_t rslt_idx_downstream = -1;

void l3cos_list_dump(void)
{
	struct l3cos_node *iter_l3cos;
	struct hash_node *iter_hash;

	ca_printf(KERN_INFO "rslt_idx_downstream=%d\n", rslt_idx_downstream);

	ca_printf(KERN_INFO "\nDUMP l3cos_list:\n");
	read_lock(&l3cos_list_lock);
	list_for_each_entry(iter_l3cos, &l3cos_list, list) {
		ca_printf(KERN_INFO "--------------------------------------------------------------------------\n");
		ca_printf(KERN_INFO "Key:\n");
			ca_printf(KERN_INFO "  egress_top_vid(12): 0x%03x\n", iter_l3cos->egress_top_vid);
			ca_printf(KERN_INFO "  egress_top_8021p(3): %u\n", iter_l3cos->egress_top_8021p);

		ca_printf(KERN_INFO "Result:\n");
		ca_printf(KERN_INFO "  cos(3): %u\n", iter_l3cos->cos);

		ca_printf(KERN_INFO "Hashlite index:\n");
		list_for_each_entry(iter_hash, &iter_l3cos->hash_list, list) {
			ca_printf(" %u", iter_hash->rslt_idx);
		}
	}
	read_unlock(&l3cos_list_lock);
	ca_printf(KERN_INFO "==========================================================================\n");
}

extern struct ca_int_l3_intf_s *ca_l3_intf_tbl;
extern struct list_head nexthop_table;

/***************************************************************************
 * proc 'cmd'
 ***************************************************************************/

#define CA_NE_L3COS_CMD_HELP_MSG \
	"Purpose: L3COS commands\n" \
	"\n" \
	"Add rule:\n" \
	"  # echo add egress_top_vid egress_top_8021p cos > %s\n" \
	"Delete rule:\n" \
	"  # echo del egress_top_vid egress_top_8021p > %s\n" \
	"Update rule: (for the rule is added before interface is created)\n" \
	"  # echo update > %s\n" \
	"Dump rule:\n" \
	"  # echo dump > %s\n" \
	"\n" \
	"egress_top_vid: valid from 0 to 4095, or N/A if it is 0xffff or 65535.\n" \
	"egress_top_8021p: valid from 0 to 7, or N/A if it is 0xff or 255.\n" \
	"cos: valid from 0 to 7.\n" \
	"\n"

#define L3COS_CMD_INVALID			0x00000000
#define L3COS_CMD_DUMP				0x00000001
#define L3COS_CMD_ADD				0x00000002
#define L3COS_CMD_DELETE			0x00000004
#define L3COS_CMD_UPDATE_ALL			0x00000008

static int l3cos_cmd_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_NE_L3COS_CMD_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int l3cos_cmd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l3cos_cmd_show_help, (void *) file);
}

ssize_t l3cos_cmd_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	int cmd = L3COS_CMD_INVALID;
	ca_uint16_t egress_top_vid;
	ca_uint8_t egress_top_8021p;
	ca_uint32_t cos;
	ca_status_t ret;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _l3cos_cmd_invalid;
	tmp[len] = '\0';


	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "dump", strlen(token)) == 0) {
			cmd = L3COS_CMD_DUMP;
			goto _l3cos_cmd_apply;
		} else if (strncmp(token, "add", strlen(token)) == 0) {
			cmd = L3COS_CMD_ADD;
		} else if (strncmp(token, "del", strlen(token)) == 0) {
			cmd = L3COS_CMD_DELETE;
		} else if (strncmp(token, "update", strlen(token)) == 0) {
			cmd = L3COS_CMD_UPDATE_ALL;
			goto _l3cos_cmd_apply;
		} else {
			printk(KERN_ERR "ERROR! Invalid action '%s'.\n", token);
			goto _l3cos_cmd_invalid;
		}
	} else {
		goto _l3cos_cmd_invalid;
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtou16(token, 0, &egress_top_vid)) {
			printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
			goto _l3cos_cmd_invalid;
		}
	} else {
		goto _l3cos_cmd_invalid;
	}

	/* parse 3rd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtou8(token, 0, &egress_top_8021p)) {
			printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
			goto _l3cos_cmd_invalid;
		}
	} else {
		goto _l3cos_cmd_invalid;
	}

	if (cmd != L3COS_CMD_ADD)
		goto _l3cos_cmd_apply;

	/* parse 4th token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtoul(token, 0, (unsigned long *) &cos)) {
			printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
			goto _l3cos_cmd_invalid;
		}
	} else {
		goto _l3cos_cmd_invalid;
	}

_l3cos_cmd_apply:
	switch (cmd) {
	case L3COS_CMD_DUMP:
		l3cos_list_dump();
		break;

	case L3COS_CMD_ADD:
		printk(KERN_INFO "Add L3COS rule. vid=0x%x, 802.1p=%u, cos=%u\n", egress_top_vid, egress_top_8021p, cos);
		ret = ca_l3cos_rule_add(0, egress_top_vid, egress_top_8021p, cos);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "ERROR! Fail to add L3COS rule. (ret %d)\n", ret);
		}
		break;

	case L3COS_CMD_DELETE:
		printk(KERN_INFO "Delete L3COS rule. vid=0x%x, 802.1p=%u\n", egress_top_vid, egress_top_8021p);
		ret = ca_l3cos_rule_delete(0, egress_top_vid, egress_top_8021p);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "ERROR! Fail to delete L3COS rule. (ret %d)\n", ret);
		}
		break;

	case L3COS_CMD_UPDATE_ALL:
		ret = ca_l3cos_update_all(0);
		if (ret != CA_E_OK) {
			printk(KERN_ERR "ERROR! Fail to update all L3COS rules. (ret %d)\n", ret);
		}
		break;
	default:
		break;
	}

	return count;

_l3cos_cmd_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_l3cos_cmd __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= l3cos_cmd_proc_open,
	.read		= seq_read,
	.write		= l3cos_cmd_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'debug'
 ***************************************************************************/

#define CA_NE_L3COS_DEBUG_HELP_MSG \
	"Purpose: Debug L3COS info\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: L3COS debug message\n" \
	"\n"

ca_uint32_t ca_l3cos_debug = 0;

static int l3cos_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CA_NE_L3COS_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_l3cos_debug);

	return 0;
}

int l3cos_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l3cos_debug_show_help, (void *) file);
}

ssize_t l3cos_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long value;
	ssize_t len;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _l3cos_debug_invalid;
	tmp[len] = '\0';

	if (kstrtoul(tmp, 0, &value))
		goto _l3cos_debug_invalid;

	if (value != 0)
		ca_l3cos_debug = 1;
	else
		ca_l3cos_debug = 0;

	return count;

_l3cos_debug_invalid:
	printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_l3cos_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= l3cos_debug_proc_open,
	.read		= seq_read,
	.write		= l3cos_debug_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * internal functions
 ***************************************************************************/

static bool l3cos_search_hash_by_key(struct l3cos_node *l3cos_node, aal_hash_key_t *hash_key)
{
	struct hash_node *iter_hash;

	if (hash_key == NULL)
		return false;

	list_for_each_entry(iter_hash, &l3cos_node->hash_list, list) {
		if (memcmp(&iter_hash->hash_key, hash_key, sizeof(aal_hash_key_t)) == 0)
			return true;
	}

	return false;
}

static ca_status_t l3cos_add_hash(struct l3cos_node *l3cos_node, aal_hash_key_t *hash_key, aal_hash_mask_t *hash_mask)
{
	struct hash_node *hash_node;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	hl_act_l3cos_t *hash_action = (hl_act_l3cos_t *) &hash_action_entry;
	ca_uint32_t rslt_idx;
	int ret;

	if ((l3cos_node == NULL) || (hash_key == NULL) || (hash_mask == NULL))
		return CA_E_PARAM;

	if (l3cos_search_hash_by_key(l3cos_node, hash_key) == true)
		return CA_E_EXISTS;

	memset(&hash_action_entry, 0, sizeof(hash_action_entry));
	hash_action->cos = l3cos_node->cos;
	hash_action->cos_sel = 1; /* use FIB.cos as result */

	ret = aal_hashlite_hash_add(0, hash_key, hash_mask, &hash_action_entry, HL_ACTGRP_L3COS, HASHLITE_AGING_STATIC, &rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: Fail to add hash in HashLite engine. (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}

	/* add hash info to l3cos_list->hash_list */
	hash_node = ca_zalloc_trace(sizeof(struct hash_node), GFP_KERNEL, true);
	if (hash_node == NULL) {
		return CA_E_MEMORY;
	}
	memcpy(&hash_node->hash_key, hash_key, sizeof(aal_hash_key_t));
	hash_node->rslt_idx = rslt_idx;
	list_add(&hash_node->list, &l3cos_node->hash_list);

	return CA_E_OK;
}

/***************************************************************************
 * exported API
 ***************************************************************************/

ca_status_t ca_l3cos_rule_add(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p, ca_uint32_t cos)
{
	struct l3cos_node *iter_l3cos = NULL;
#if 0
	struct nexthop_node *iter_nh = NULL;
#endif
	aal_hash_key_t hash_key;
	aal_hash_mask_t *hash_mask = NULL;
	int i;
	ca_status_t ret;
	int rule_type = RULE_INVALID;
	bool found_intf = false;
	ca_intf_id_t intf_id;

	/* validation */

	if ((egress_top_vid > 0xfff) && (egress_top_vid != CA_UINT16_INVALID))
		return CA_E_PARAM;

	if ((egress_top_8021p > 7) && (egress_top_8021p != CA_UINT8_INVALID))
		return CA_E_PARAM;

	if (cos > 7)
		return CA_E_PARAM;

	if ((egress_top_vid == CA_UINT16_INVALID) && (egress_top_8021p == CA_UINT8_INVALID)) {
		/* egress untagged packet */
		rule_type = RULE_EGRESS_UNTAG;
		hash_mask = &HM_HGU_US_INTF;
	} else if ((egress_top_vid != CA_UINT16_INVALID) && (egress_top_8021p == CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_VID;
		hash_mask = &HM_HGU_US_INTF;
	} else if ((egress_top_vid == CA_UINT16_INVALID) && (egress_top_8021p != CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_8021P;
		hash_mask = &HM_HGU_US_8021P;
	} else if ((egress_top_vid != CA_UINT16_INVALID) && (egress_top_8021p != CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_VID_8021P;
		hash_mask = &HM_HGU_US_INTF_8021P;
	}

	if (ca_l3cos_debug) {
		printk(KERN_INFO "%s: egress_top_vid=0x%x, egress_top_8021p=%u, cos=%u\n", __func__, egress_top_vid, egress_top_8021p, cos);
	}

	/* search if rule exists already */

	read_lock(&l3cos_list_lock);
	list_for_each_entry(iter_l3cos, &l3cos_list, list) {
		if ((iter_l3cos->egress_top_vid == egress_top_vid) && (iter_l3cos->egress_top_8021p == egress_top_8021p)) {
			if (ca_l3cos_debug) {
				printk(KERN_INFO "%s: Same rule exists already.\n", __func__);
			}
			read_unlock(&l3cos_list_lock);
			return CA_E_EXISTS;
		}
	}
	read_unlock(&l3cos_list_lock);

	/* add rule to l3cos_list */

	iter_l3cos = ca_zalloc_trace(sizeof(struct l3cos_node), GFP_KERNEL, true);
	if (iter_l3cos == NULL) {
		return CA_E_MEMORY;
	}
	iter_l3cos->egress_top_vid = egress_top_vid;
	iter_l3cos->egress_top_8021p = egress_top_8021p;
	iter_l3cos->cos = cos;
	INIT_LIST_HEAD(&iter_l3cos->hash_list);
	write_lock(&l3cos_list_lock);
	list_add(&iter_l3cos->list, &l3cos_list);
	write_unlock(&l3cos_list_lock);

	/* add hash */

	memset(&hash_key, 0, sizeof(hash_key));

	/* NOTE for egress_top_vid:
	 * It is impossible to know egress_top_vid because it is part of routing result.
	 * So, there is the step of creating hash entry for egress_top_vid:
	 *   1. Get the intf_id of WAN interface with same egress_top_vid.
	 *      This interace must be unique because it is impossible to have multiple WAN interfaces with same egress_top_vid.
	 *   2. Get all the NextHop entries of founded intf_id.
	 *   3. Create the hash entry that key.mdata is intf_id.
	 *   4. Create the hash entry that key.mdata is rst_idx_gateway within internal NextHop entry.
	 *      (OBSOLETE! LPM uses HashLite profile#14 for gateway route, it uses hash_action directly; hash_key is bypassed.)
	 *
	 * Unlike egress_top_8021p case which may have single-tagged and double-tagged conditions,
	 * egress_top_vid actually use mey.mdata only, so API doesn't care about how many VLAN tags are for egress_top_8021p.
	 */

	/* NOTE for egress_top_8021p:
	 * In current L3FE settings, it doesn't map nor change any ingress 802.1p,
	 * so hash entry can use egress_top_8021p as key directly.
	 *
	 * If egress_top_8021p is valid, packets may be single-tagged or double-tagged.
	 * API can't use key.top_8021p only; top_8021p may be zero and untagged packets could hit this.
	 * Therefore, API needs to create two hash entries for it; one is key.vlan_cnt=1 and another is key_vlan_cnt=2.
	 */

	if ((rule_type == RULE_EGRESS_UNTAG) || (rule_type == RULE_EGRESS_VID) || (rule_type == RULE_EGRESS_VID_8021P)) {

		/* search intf_id with same egress_top_vid */
		for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
			if (ca_l3_intf_tbl[i].use == 0) continue;

			if ( ((PORT_ID(ca_l3_intf_tbl[i].intf.port_id) == AAL_LPORT_ETH_WAN) || (PORT_ID(ca_l3_intf_tbl[i].intf.port_id) == AAL_LPORT_L3_WAN))
			    && (ca_l3_intf_tbl[i].intf.outer_vid == egress_top_vid) ) {
				found_intf = true;
				intf_id = i;
				if (ca_l3cos_debug) {
					printk(KERN_INFO "%s: Found matched interface, intf_id=%u\n", __func__, intf_id);
				}
				break;
			}
		}

		if (found_intf) {
			hash_key.mdata = intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
			hash_key.top_8021p = (rule_type == RULE_EGRESS_VID_8021P) ? egress_top_8021p : 0;

			ret = l3cos_add_hash(iter_l3cos, &hash_key, hash_mask);
			if (ret != CA_E_OK) {
				printk(KERN_INFO "%s: Fail to add hash in HashLite engine. (ret %d, intf_id %u)\n", __func__, ret, intf_id);
				return ret;
			}

#if 0 /* OBSOLETE! LPM uses HashLite profile#14 for gateway route; hash_key is bypassed. */
			/* search all NextHop entry with same intf_id */
			list_for_each_entry(iter_nh, &nexthop_table, list) {
				if ((iter_nh->nexthop.intf_id == intf_id) && (iter_nh->rst_idx_gateway >= 0)) {
					hash_key.mdata = iter_nh->rst_idx_gateway & 0xfff;
					hash_key.top_8021p = (rule_type == RULE_EGRESS_VID_8021P) ? egress_top_8021p : 0;

					ret = l3cos_add_hash(iter_l3cos, &hash_key, hash_mask);
					if (ret != CA_E_OK) {
						printk(KERN_INFO "%s: Fail to add hash in HashLite engine. (ret %d, rst_idx_gateway %u)\n",
								__func__, ret, iter_nh->rst_idx_gateway);
						return ret;
					}
				}
			}
#endif

		} /* if (found_intf) */

	} else if (rule_type == RULE_EGRESS_8021P) {
		hash_key.top_8021p = egress_top_8021p;
		hash_key.vlan_cnt = 1;

		ret = l3cos_add_hash(iter_l3cos, &hash_key, hash_mask);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: Fail to add hash in HashLite engine for vlan_cnt 1. (ret %d)\n", __func__, ret);
			return ret;
		}

		/* no idea what ingress vlan_cnt is, so create another hash for vlan_cnt=2 */
		hash_key.top_8021p = egress_top_8021p;
		hash_key.vlan_cnt = 2;

		ret = l3cos_add_hash(iter_l3cos, &hash_key, hash_mask);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: Fail to add hash in HashLite engine for vlan_cnt 2. (ret %d)\n", __func__, ret);
			return ret;
		}
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l3cos_rule_add);

ca_status_t ca_l3cos_rule_delete(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p)
{
	struct l3cos_node *iter_l3cos, *tmp_iter_l3cos;
	struct hash_node *iter_hash, *tmp_iter_hash;
	int ret;
	bool found_rule = false;

	/* validation */

	if ((egress_top_vid > 0xfff) && (egress_top_vid != CA_UINT16_INVALID))
		return CA_E_PARAM;

	if ((egress_top_8021p > 7) && (egress_top_8021p != CA_UINT8_INVALID))
		return CA_E_PARAM;

	if (ca_l3cos_debug) {
		printk(KERN_INFO "%s: egress_top_vid=0x%x, egress_top_8021p=%u\n", __func__, egress_top_vid, egress_top_8021p);
	}

	/* search and delete */

	write_lock(&l3cos_list_lock);
	list_for_each_entry_safe(iter_l3cos, tmp_iter_l3cos, &l3cos_list, list) {
		if ((iter_l3cos->egress_top_vid == egress_top_vid) && (iter_l3cos->egress_top_8021p == egress_top_8021p)) {
			found_rule = true;

			/* delete hash */
			list_for_each_entry_safe(iter_hash, tmp_iter_hash, &iter_l3cos->hash_list, list) {
				ret = aal_hashlite_hash_delete(device_id, iter_hash->rslt_idx);
				if (ret != AAL_E_OK) {
					ca_printf(KERN_ERR "%s: Fail to delete hashlite index %u\n", __func__, iter_hash->rslt_idx);
				}
				list_del(&iter_hash->list);
				ca_free_trace(iter_hash);
			}

			/* delete rule */
			list_del(&iter_l3cos->list);
			ca_free_trace(iter_l3cos);
			break;
		}
	}
	write_unlock(&l3cos_list_lock);

	return (found_rule) ? CA_E_OK : CA_E_NOT_FOUND;
}
EXPORT_SYMBOL(ca_l3cos_rule_delete);

ca_status_t ca_l3cos_update(ca_device_id_t device_id, struct l3cos_node *l3cos_node)
{
#if 0
	struct nexthop_node *iter_nh = NULL;
#endif
	aal_hash_key_t hash_key;
	aal_hash_mask_t *hash_mask = NULL;
	int i, ret;
	int rule_type = RULE_INVALID;
	bool found_intf = false;
	ca_intf_id_t intf_id;

	if (l3cos_node == NULL)
		return CA_E_PARAM;

	if ((l3cos_node->egress_top_vid == CA_UINT16_INVALID) && (l3cos_node->egress_top_8021p == CA_UINT8_INVALID)) {
		/* egress untagged packet */
		rule_type = RULE_EGRESS_UNTAG;
		hash_mask = &HM_HGU_US_INTF;
	} else if ((l3cos_node->egress_top_vid != CA_UINT16_INVALID) && (l3cos_node->egress_top_8021p == CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_VID;
		hash_mask = &HM_HGU_US_INTF;
	} else if ((l3cos_node->egress_top_vid == CA_UINT16_INVALID) && (l3cos_node->egress_top_8021p != CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_8021P;
		hash_mask = &HM_HGU_US_8021P;
	} else if ((l3cos_node->egress_top_vid != CA_UINT16_INVALID) && (l3cos_node->egress_top_8021p != CA_UINT8_INVALID)) {
		/* egress single-vlan or double-vlan */
		rule_type = RULE_EGRESS_VID_8021P;
		hash_mask = &HM_HGU_US_INTF_8021P;
	}

	if (ca_l3cos_debug) {
		printk(KERN_INFO "%s: egress_top_vid=0x%x, egress_top_8021p=%u, cos=%u\n",
			__func__, l3cos_node->egress_top_vid, l3cos_node->egress_top_8021p, l3cos_node->cos);
	}

	memset(&hash_key, 0, sizeof(hash_key));

	if ((rule_type == RULE_EGRESS_UNTAG) || (rule_type == RULE_EGRESS_VID) || (rule_type == RULE_EGRESS_VID_8021P)) {

		/* search intf_id with same egress_top_vid */
		for (i = 0; i < FE_L3E_IF_TBL_ENTRY_MAX; i++) {
			if (ca_l3_intf_tbl[i].use == 0) continue;

			if ( ((PORT_ID(ca_l3_intf_tbl[i].intf.port_id) == AAL_LPORT_ETH_WAN) ||
						(PORT_ID(ca_l3_intf_tbl[i].intf.port_id) == AAL_LPORT_L3_WAN)) &&
					(ca_l3_intf_tbl[i].intf.outer_vid == l3cos_node->egress_top_vid)) {
				found_intf = true;
				intf_id = i;
				if (ca_l3cos_debug) {
					printk(KERN_INFO "%s: Found matched interface, intf_id=%u\n", __func__, intf_id);
				}
				break;
			}
		}

		if (found_intf) {
			hash_key.mdata = intf_id & 0xfff;	// MDATA[11:0] is LPM rst_idx
			hash_key.top_8021p = (rule_type == RULE_EGRESS_VID_8021P) ? l3cos_node->egress_top_8021p : 0;

			ret = l3cos_add_hash(l3cos_node, &hash_key, hash_mask);
			if (ret != CA_E_OK) {
				printk(KERN_INFO "%s: Fail to add hash in HashLite engine. (ret %d, intf_id %u)\n", __func__, ret, intf_id);
				return ret;
			}

#if 0 /* OBSOLETE! LPM uses HashLite profile#14 for gateway route; hash_key is bypassed. */
			/* search all NextHop entry with same intf_id */
			list_for_each_entry(iter_nh, &nexthop_table, list) {
				if ((iter_nh->nexthop.intf_id == intf_id) && (iter_nh->rst_idx_gateway >= 0)) {
					hash_key.mdata = iter_nh->rst_idx_gateway & 0xfff;
					hash_key.top_8021p = (rule_type == RULE_EGRESS_VID_8021P) ? l3cos_node->egress_top_8021p : 0;

					ret = l3cos_add_hash(l3cos_node, &hash_key, hash_mask);
					if (ret != CA_E_OK) {
						printk(KERN_INFO "%s: Fail to add hash in HashLite engine. (ret %d, rst_idx_gateway %u)\n",
								__func__, ret, iter_nh->rst_idx_gateway);
						return ret;
					}
				}
			}
#endif

		} /* if (found_intf) */

	} else if (rule_type == RULE_EGRESS_8021P) {
		hash_key.top_8021p = l3cos_node->egress_top_8021p;
		hash_key.vlan_cnt = 1;

		ret = l3cos_add_hash(l3cos_node, &hash_key, hash_mask);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: Fail to add hash in HashLite engine for vlan_cnt 1. (ret %d)\n", __func__, ret);
			return ret;
		}

		/* no idea what ingress vlan_cnt is, so create another hash for vlan_cnt=2 */
		hash_key.top_8021p = l3cos_node->egress_top_8021p;
		hash_key.vlan_cnt = 2;

		ret = l3cos_add_hash(l3cos_node, &hash_key, hash_mask);
		if (ret != CA_E_OK) {
			printk(KERN_INFO "%s: Fail to add hash in HashLite engine for vlan_cnt 2. (ret %d)\n", __func__, ret);
			return ret;
		}
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l3cos_update);

ca_status_t ca_l3cos_update_all(ca_device_id_t device_id)
{
	struct l3cos_node *iter_l3cos = NULL;
	ca_status_t ret;

	// TBD: append write_lock well
	list_for_each_entry(iter_l3cos, &l3cos_list, list) {
		ret = ca_l3cos_update(device_id, iter_l3cos);
		if (ret != CA_E_OK) {
			ca_printf(KERN_ERR "%s: Fail to update L3COS rule. (ret %d)\n", __func__, ret);
		}
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_l3cos_update_all);

/***************************************************************************
 * init/exit functions
 ***************************************************************************/

static ca_status_t l3cos_add_predefined_rule(void)
{
	aal_hash_key_t hash_key;
	aal_hashlite_hash_action_entry_t hash_action_entry;
	// hl_act_l3cos_t *hash_action = (hl_act_l3cos_t *) &hash_action_entry;
	ca_uint32_t rslt_idx;
	int ret;

	/* all downstream (lspid 0x18), no action to avoid default action */

	memset(&hash_key, 0, sizeof(hash_key));
	memset(&hash_action_entry, 0, sizeof(hash_action_entry));
	hash_key.lspid = AAL_LPORT_L3_WAN;

	ret = aal_hashlite_hash_add(0, &hash_key, &HM_HGU_DS, &hash_action_entry, HL_ACTGRP_L3COS, HASHLITE_AGING_STATIC, &rslt_idx);
	if (ret != AAL_E_OK) {
		printk(KERN_INFO "%s: Fail to add L3COS hash for dowmstream in HashLite engine. (ret %d)\n", __func__, ret);
		return CA_E_ERROR;
	}
	rslt_idx_downstream = rslt_idx;

	return CA_E_OK;
}

ca_status_t ca_l3cos_proc_init(void)
{
	proc_l3cos = proc_mkdir("l3cos", proc_adapt_dir);

	if (proc_create("debug", S_IRUGO | S_IWUGO, proc_l3cos, &fops_l3cos_debug) == NULL) {
		pr_err("Fail to create proc entry 'debug'.\n");
		return CA_E_ERROR;
	}

	if (proc_create("cmd", S_IRUGO | S_IWUGO, proc_l3cos, &fops_l3cos_cmd) == NULL) {
		pr_err("Fail to create proc entry 'cmd'.\n");
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

void ca_l3cos_proc_exit(void)
{
	remove_proc_entry("debug", proc_l3cos);
	remove_proc_entry("cmd", proc_l3cos);

	proc_remove(proc_l3cos);
}

ca_status_t ca_l3cos_init(void)
{
	ca_status_t ret;

	ret = l3cos_add_predefined_rule();
	if (ret != CA_E_OK) {
		printk(KERN_INFO "%s: Fail to add pre-defined rule for L3COS. (ret %d)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

ca_status_t ca_l3cos_exit(void)
{
	return CA_E_OK;
}
