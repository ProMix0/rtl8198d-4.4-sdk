
#include <linux/if_vlan.h>
#include <linux/netdevice.h>
#include <linux/in.h>

#include "eth_port.h"

#include <soc/cortina/ca_kernel_hook.h>

int ca_kh_used_vid[4096]={0};

ca_status_t ca_ni_intf_get_ports(struct net_device *dev, ca_uint8_t *num_ports, int *ports, ca_uint8_t *is_wan_intf);

ca_status_t ca_kh_l2_vlan_add(ca_device_id_t device_id, struct net_device *parent_dev, int vid)
{
	int i;
	ca_uint8_t num_ports = CA_ETH_PORT_NUM;
	int ports[CA_ETH_PORT_NUM];
	ca_uint8_t existed_num_ports = 0;
	ca_uint8_t untagged_count;
	ca_status_t ret;
	ca_port_id_t untagged_ports[1];
	ca_port_id_t member_ports[CA_ETH_PORT_NUM];
	ca_flooding_port_t flood_ports[CA_ETH_PORT_NUM];
	struct net_device *real_dev;
	ca_uint8_t is_wan_intf;

	/* Arch team doesn't plan to support L2 VLAN in kernel hook */
	return CA_E_NOT_SUPPORT;

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: parent_dev->name=%s, ifindex=%d, vid=%d\n", __func__, parent_dev->name, parent_dev->ifindex, vid);
	}

	ret = ca_l2_vlan_create(device_id, vid);
	if (ret != CA_E_OK && ret != CA_E_EXISTS) {
		printk("%s: ca_l2_vlan_create(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
		return ret;
	}

	/* retrieve the existed ports */
	if (ret == CA_E_EXISTS) {
		ret = ca_l2_vlan_port_get(device_id, vid, &existed_num_ports, member_ports, &untagged_count, untagged_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_port_get() failed, ret=%d!!\n", __func__, ret);
			return ret;
		}
		if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
			printk("%s: existed_num_ports=%d\n", __func__, existed_num_ports);
			for (i = 0; i < existed_num_ports; i++) {
				printk("%s: member_ports[%d]=0x%x\n", __func__, i, member_ports[i]);
			}
		}
	}

	if (is_vlan_dev(parent_dev)) {
		real_dev = vlan_dev_real_dev(parent_dev);
	} else {
		real_dev = parent_dev;
	}
	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: real_dev->name=%s, ifindex=%d\n", __func__, real_dev->name, real_dev->ifindex);
	}

	ret = ca_ni_intf_get_ports(real_dev, &num_ports, ports, &is_wan_intf);
	if (ret != CA_E_OK) {
		printk("%s: ca_ni_intf_get_ports() failed, ret=%d!!\n", __func__, ret);
		return ret;
	}

	for (i = existed_num_ports; i < existed_num_ports+num_ports; i++) {
		member_ports[i] = (CA_PORT_TYPE_ETHERNET << CA_PORT_TYPE_SHIFT) + ports[i-existed_num_ports];

		if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
			printk("%s: member_ports[%d]=0x%x\n", __func__, i, member_ports[i]);
		}
	}

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: number of ports=%d\n", __func__, existed_num_ports+num_ports);
		for (i = 0; i < existed_num_ports+num_ports; i++) {
			printk("%s: member_ports[%d]=0x%x\n", __func__, i, member_ports[i]);
		}
	}

	ret = ca_l2_vlan_port_set(device_id, vid, num_ports + existed_num_ports, member_ports, 0, untagged_ports);
	if (ret != CA_E_OK) {
		printk("%s: ca_l2_vlan_port_set(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
		return ret;
	}

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		ret = ca_l2_vlan_flooding_domain_members_get(device_id, vid, CA_L2_ING_PORT_TYPE_LAN, &num_ports, flood_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_flooding_domain_members_get(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
		}
		else {
			printk("%s: flooding domain vid=%d, num_ports=%d\n", __func__, vid, num_ports);
			for (i = 0; i < num_ports; i++) {
				printk("%s: index=%d, egress_vid=%d, egress_port=0x%x\n", __func__, i, flood_ports[i].egress_vid, flood_ports[i].egress_port);
			}
		}
	}

	spin_lock_bh(&ca_kh_vlan_lock);
	ca_kh_used_vid[vid] = 1;
	spin_unlock_bh(&ca_kh_vlan_lock);

        return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l2_vlan_add);

ca_status_t ca_kh_l2_vlan_delete(ca_device_id_t device_id, struct net_device *dev)
{
	int i, j, keep;
	int vid = vlan_dev_vlan_id(dev);
	ca_uint8_t num_ports = CA_ETH_PORT_NUM;
        int ports[CA_ETH_PORT_NUM];
        ca_uint8_t existed_num_ports = 0;
        ca_uint8_t new_num_ports;
        ca_uint8_t untagged_count;
	ca_status_t ret;
	ca_port_id_t tmp_port;
        ca_port_id_t untagged_ports[1];
        ca_port_id_t member_ports[CA_ETH_PORT_NUM];
	ca_port_id_t new_member_ports[CA_ETH_PORT_NUM];
        struct net_device *real_dev;
	ca_uint8_t is_wan_intf;

	/* Arch team doesn't plan to support L2 VLAN in kernel hook */
	return CA_E_NOT_SUPPORT;

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: dev->name=%s, ifindex=%d, vid=%d\n", __func__, dev->name, dev->ifindex, vid);
	}

	if (is_vlan_dev(dev) == 0) {
		printk("%s: dev->name=%s is not a VLAN device!!\n", __func__, dev->name);
		return CA_E_ERROR;
	}

        real_dev = vlan_dev_real_dev(dev);
	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {

		printk("%s: real_dev->name=%s\n", __func__, real_dev->name);
	}

	ret = ca_l2_vlan_port_get(device_id, vid, &existed_num_ports, member_ports, &untagged_count, untagged_ports);
	if (ret != CA_E_OK) {
		printk("%s: ca_l2_vlan_port_get(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
		return ret;
	}

        ret = ca_ni_intf_get_ports(real_dev, &num_ports, ports, &is_wan_intf);
        if (ret != CA_E_OK) {
                printk("%s: ca_ni_intf_get_ports() failed, ret=%d!!\n", __func__, ret);
                return ret;
        }

	new_num_ports = 0;
        for (i = 0; i < existed_num_ports; i++) {
		keep = 0;
		for (j = 0; j < num_ports; j++) {
			/* got the ports need to be removed */
			tmp_port = (CA_PORT_TYPE_ETHERNET << CA_PORT_TYPE_SHIFT) + ports[j];
			if (member_ports[i] == tmp_port)
				break;
		}
		if (j == num_ports)
			keep = 1;

		if (keep == 1) {
			new_member_ports[new_num_ports++] = member_ports[i];
			if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
				printk("%s: keep member_ports[%d]=0x%x\n", __func__, i, member_ports[i]);
			}
		}
        }

        if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
                printk("%s: new number of ports=%d\n", __func__, new_num_ports);
                for (i = 0; i < new_num_ports; i++) {
                        printk("%s: new_member_ports[%d]=0x%x\n", __func__, i, new_member_ports[i]);
                }
        }

        ret = ca_l2_vlan_port_set(device_id, vid, new_num_ports, new_member_ports, 0, untagged_ports);
        if (ret != CA_E_OK) {
                printk("%s: ca_l2_vlan_port_set(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
                return ret;
        }

	/* if no member ports left delete the VLAN entry */
	if (new_num_ports == 0) {
		ret = ca_l2_vlan_delete(device_id, vid);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_delete(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
			return ret;
		}

		spin_lock_bh(&ca_kh_vlan_lock);
		ca_kh_used_vid[vid] = 0;
		spin_unlock_bh(&ca_kh_vlan_lock);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l2_vlan_delete);

ca_status_t ca_kh_l2_br_add_if(ca_device_id_t device_id, __net_bridge_port_t *br_port)
{
	int i;
	int vid;
	ca_uint8_t num_ports;
	ca_uint8_t total_num_ports;
	ca_status_t ret;
        ca_flooding_port_t flood_ports[CA_ETH_PORT_NUM];
        ca_flooding_port_t total_flood_ports[CA_ETH_PORT_NUM];
	__net_bridge_port_t *p;

	///TODO: no support for hardware bridge
	return CA_E_NOT_SUPPORT;

	total_num_ports = 0;
	list_for_each_entry(p, &(br_port->list), list) {
		if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
			printk("%s: dev->name=%s\n", __func__, p->dev->name);
		}

		if (is_vlan_dev(p->dev) == 0) {
			if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
				printk("%s: %s is not a VLAN device!!\n", __func__, p->dev->name);
			}
			return CA_E_PARAM;
		}

		vid = vlan_dev_vlan_id(p->dev);
		ret = ca_l2_vlan_flooding_domain_members_get(device_id, vid, CA_L2_ING_PORT_TYPE_LAN, &num_ports, flood_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_flooding_domain_members_get(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
		}
		else {
			if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
				printk("%s: flooding domain vid=%d, num_ports=%d\n", __func__, vid, num_ports);
			}
			for (i = 0; i < num_ports; i++) {
				if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
					printk("%s: index=%d, egress_vid=%d, egress_port=0x%x\n", __func__, i, flood_ports[i].egress_vid, flood_ports[i].egress_port);
				}
				total_flood_ports[total_num_ports] = flood_ports[i];
				total_num_ports++;
			}
			if (total_num_ports > CA_ETH_PORT_NUM) {
				printk("%s: number of total ports exceed %d!!\n", __func__, CA_ETH_PORT_NUM);
				return CA_E_FULL;
			}
		}
	}
	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: total_num_ports=%d\n", __func__, total_num_ports);
		for (i = 0; i < total_num_ports; i++) {
			printk("%s: index=%d, egress_vid=%d, egress_port=0x%x\n", __func__, i, total_flood_ports[i].egress_vid, total_flood_ports[i].egress_port);
		}
	}

	list_for_each_entry(p, &(br_port->list), list) {
		vid = vlan_dev_vlan_id(p->dev);
		ret = ca_l2_vlan_flooding_domain_members_update(device_id, vid, CA_L2_ING_PORT_TYPE_LAN, total_num_ports, total_flood_ports);
		if (ret != CA_E_OK) {
			printk("%s: ca_l2_vlan_flooding_domain_members_update(vid=%d) failed, ret=%d\n", __func__, vid, ret);
		}

		if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
			ret = ca_l2_vlan_flooding_domain_members_get(device_id, vid, CA_L2_ING_PORT_TYPE_LAN, &num_ports, flood_ports);
			if (ret != CA_E_OK) {
				printk("%s: ca_l2_vlan_flooding_domain_members_get(vid=%d) failed, ret=%d!!\n", __func__, vid, ret);
			}
			for (i = 0; i < num_ports; i++) {
				printk("%s: index=%d, egress_vid=%d, egress_port=0x%x\n", __func__, i, flood_ports[i].egress_vid, flood_ports[i].egress_port);
			}
		}
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(ca_kh_l2_br_add_if);

ca_status_t ca_kh_add_wan_port_to_unknown_vlan_flooding_domain(ca_uint8_t enable)
{
	ca_status_t ret;
	ca_vlan_port_control_t port_cntl;
	ca_port_id_t port_id;

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: enable=%d\n", __func__, enable);
	}

	/* add wan port to unknown vlan flooding domain */
	port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI7);
	ret = ca_l2_vlan_port_control_get(0, port_id, &port_cntl);
	if (ret != CA_E_OK) {
		printk("%s: ca_l2_vlan_port_control_get(port_id=0x%x) failed, ret=%d\n", __func__, port_id, ret);
		return ret;
	}

	port_cntl.ingress_membership_check_enable = TRUE;
	port_cntl.egress_membership_check_enable = TRUE;
	port_cntl.drop_unknown_vlan = enable ? FALSE : TRUE;

	ret = ca_l2_vlan_port_control_set(0, port_id, &port_cntl);
	if (ret != CA_E_OK) {
		printk("%s: ca_l2_vlan_port_control_set(port_id=0x%x) failed, ret=%d\n", __func__, port_id, ret);
		return ret;
	}
	return ret;
}

ca_status_t ca_kh_add_classifier_for_wan_port_bridge_mode(ca_vlan_id_t vid, ca_uint32_t *cls_rule_index)
{
	ca_classifier_key_t      key;
	ca_classifier_key_mask_t key_mask;
	ca_classifier_action_t   action;
	ca_uint32_t              index;
	ca_status_t 		 ret;

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: vid=%d\n", __func__, vid);
	}
	memset(&key, 0, sizeof(ca_classifier_key_t));
	key.src_port = CA_PORT_ID(CA_PORT_TYPE_L2RP, CA_PORT_ID_NI7);
	key.l2.vlan_otag.vlan_min.tpid = 0x8100;
	key.l2.vlan_otag.vlan_min.pri = 0;
	key.l2.vlan_otag.vlan_min.dei = 0;
	key.l2.vlan_otag.vlan_min.vid = vid;
	key.l2.vlan_otag.vlan_max.tpid = 0x8100;
	key.l2.vlan_otag.vlan_max.pri = 0;
	key.l2.vlan_otag.vlan_max.dei = 0;
	key.l2.vlan_otag.vlan_max.vid = vid;

	memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
	key_mask.src_port = 1;
	key_mask.l2 = 1;
	key_mask.l2_mask.vlan_otag = 1;
	key_mask.l2_mask.vlan_otag_mask.vid = 1;

	memset(&action, 0, sizeof(ca_classifier_action_t));
	action.forward = CA_CLASSIFIER_FORWARD_PORT;
	action.dest.port = CA_PORT_ID_L3_LAN;

	ret = ca_classifier_rule_add(0, 12, &key, &key_mask, &action, &index);
	if (ret != CA_E_OK) {
		printk("%s: ca_classifier_rule_add() failed, ret=%d\n", __func__, ret);
		return ret;
	}
	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: rule index=%d\n", __func__, index);
	}
	*cls_rule_index = index;
	return ret;
}

ca_status_t ca_kh_del_classifier_for_wan_port_bridge_mode(ca_uint32_t cls_rule_index)
{
	ca_status_t ret;

	if (ca_kh_debug & CA_KH_DEBUG_VLAN) {
		printk("%s: cls_rule_index=%d\n", __func__, cls_rule_index);
	}

	ret = ca_classifier_rule_delete(0, cls_rule_index);
	if (ret != CA_E_OK) {
		printk("%s: ca_classifier_rule_delete(%d) failed, ret=%d\n", __func__, cls_rule_index, ret);
	}
	return ret;
}
