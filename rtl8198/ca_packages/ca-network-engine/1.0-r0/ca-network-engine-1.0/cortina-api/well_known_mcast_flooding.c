#include <linux/inet.h>
#include <soc/cortina/ca_ipc_pe.h>
#include "well_known_mcast_flooding.h"
#include "aal_hash.h"
#include "aal_l3fe.h"
#include "aal_l2_vlan.h"
#include "aal_mcast.h"
#include "aal_common.h"
#include "api_common.h"
#include "port.h"
#include "vlan.h"
#include "aal_port.h"
#include "ca-scfg/scfg.h"

extern ca_uint32_t g_vlan_flooding_domain_1[CFG_ID_FLOODING_DOMAIN_1_LEN];
void ca_api_ip_address_htonl(ca_ip_address_t *p_dest, ca_ip_address_t *p_src);

#define CA_WMC_ADDRESS_MAX 30
#define WMC_DBG 0

typedef struct wmc_flooding_address {
        ca_ip_address_t ip_da;
        ca_ip_address_t ip_da_max;
} wmc_flooding_address_t;

static wmc_flooding_address_t g_wmc_flooding_address_v4_table[CA_WMC_ADDRESS_MAX];
static wmc_flooding_address_t g_wmc_flooding_address_v6_table[CA_WMC_ADDRESS_MAX];
static ca_uint8_t g_wmc_flooding_addr_v4_num;
static ca_uint8_t g_wmc_flooding_addr_v6_num;
static ca_uint8_t g_wmc_flooding_pe_id = 0;
static ca_uint32_t g_wmc_cpu_port_id;
static ca_uint32_t g_wmc_ipc_id;

ca_uint8_t g_wmc_flooding_default = 0;
ca_uint8_t g_wmc_flooding_enable = 0;
static ca_uint8_t g_wmc_flooding_init = 0;
static ca_uint8_t g_wmc_flooding_table_num = 0;

typedef struct wmc_flooding {
        ca_uint32_t             dst_ports[CA_WMC_GROUP_PORT_MAX];
        ca_uint8_t              dst_port_num;
        ca_int16_t              vid;
} wmc_flooding_t;

static wmc_flooding_t g_wmc_flooding_table[CA_WMC_GROUP_MAX];
static wmc_flooding_t g_default_group;

static void ca_wmc_flooding_add_with_vid(ca_int16_t vid, ca_uint32_t *member_ports, ca_uint8_t member_count);
static ca_status_t ca_wmc_flooding_add_address_ipc(wmc_flooding_address_t *ip_range);

static int wmc_flooding_ipc_debug_handle(
	ca_ipc_addr_t peer,
	ca_uint16_t msg_no,
	ca_uint16_t trans_id,
	const void * msg_data,
	ca_uint16_t* msg_size
);

static ca_ipc_msg_handle_t wmc_flooding_ipc_msg[CA_WMC_FLOODING_IPC_MAX] = {
        { .msg_no = CA_WMC_FLOODING_ENABLE_IPC,           .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_DISABLE_IPC,          .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_ADD_IPC,              .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_ADD_ADDRESS_IPC,      .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_UPDATE_IPC,           .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_DELETE_IPC,           .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_DELETE_ALL_IPC,       .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_DUMP_IPC,             .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_DEBUG_IPC,            .proc = wmc_flooding_ipc_debug_handle},
        { .msg_no = CA_WMC_FLOODING_FILTER_IPC,           .proc = wmc_flooding_ipc_debug_handle},
};

/**
 *   Parses a subnet address string on the format
 *   a.b.c.d/n into a SubnetList entry.
 */
static wmc_flooding_address_t *parseSubnetAddress(char *addrstr, ca_ip_afi_t afi)
{
        wmc_flooding_address_t *tmp_ip_range;
        char *tmpStr;
        ca_uint32_t addr, ipv4_addr_start, ipv4_addr_max;
        ca_uint8_t ipv6_addr_start[16], ipv6_addr_max[16];
        int i, ret = 0;

        printk("%s: Parsed address %s to ", __func__, addrstr);
        // Get the start address...
        tmpStr = strsep(&addrstr, " ");

        if(afi == CA_IPV4)
                ret = in4_pton(tmpStr, strlen(tmpStr), (u8 *)&ipv4_addr_start, -1, NULL);
        else
                ret = in6_pton(tmpStr, -1, ipv6_addr_start, -1, NULL);
        if (!ret) {
                printk("%s: Convert %s to binary representation failed!\n", __func__, tmpStr);
                return NULL;
        }

        // Get the max address...
        tmpStr = strsep(&addrstr, " ");

        if(afi == CA_IPV4)
                ret = in4_pton(tmpStr, strlen(tmpStr), (u8 *)&ipv4_addr_max, -1, NULL);
        else
                ret = in6_pton(tmpStr, -1, ipv6_addr_max, -1, NULL);
        if (!ret) {
                printk("%s: Convert %s to binary representation failed!\n", __func__, tmpStr);
                return NULL;
        }

        tmp_ip_range = ca_malloc(sizeof(wmc_flooding_address_t));
        if(afi == CA_IPV4) {
                tmp_ip_range->ip_da.afi = CA_IPV4;
                tmp_ip_range->ip_da_max.afi = CA_IPV4;
                tmp_ip_range->ip_da.ip_addr.ipv4_addr = ntohl(ipv4_addr_start);
                tmp_ip_range->ip_da_max.ip_addr.ipv4_addr = ntohl(ipv4_addr_max);
                tmp_ip_range->ip_da_max.addr_len = 32;
                printk("0x%x - 0x%x.\n", tmp_ip_range->ip_da.ip_addr.ipv4_addr, 
                                tmp_ip_range->ip_da_max.ip_addr.ipv4_addr);
        } else {
                tmp_ip_range->ip_da.afi = CA_IPV6;
                tmp_ip_range->ip_da_max.afi = CA_IPV6;
                for(i=0; i<4; i++) {
                        memcpy(&addr, &ipv6_addr_start[i*4], sizeof(addr));
                        tmp_ip_range->ip_da.ip_addr.ipv6_addr[i] = ntohl(addr);
                        printk("%08x", tmp_ip_range->ip_da.ip_addr.ipv6_addr[i]);
                }
                printk(" - ");
                for(i=0; i<4; i++) {
                        memcpy(&addr, &ipv6_addr_max[i*4], sizeof(addr));
                        tmp_ip_range->ip_da_max.ip_addr.ipv6_addr[i] = ntohl(addr);
                        printk("%08x", tmp_ip_range->ip_da_max.ip_addr.ipv6_addr[i]);
                }
                tmp_ip_range->ip_da_max.addr_len = 128;
                printk("\n");
        }

        return tmp_ip_range;
}

static ca_status_t parseInput(char *input, ca_int16_t *vid, ca_uint32_t *member_ports, ca_uint8_t *member_count)
{
        char *tmpStr;
        ca_uint32_t tmpPort;
        ca_uint32_t tmpPorts[CA_WMC_GROUP_PORT_MAX] = {0};
        ca_int16_t tmpVid = -1;
        ca_uint8_t tmpCount = 0;

        tmpStr = strsep(&input, " ");
        if (kstrtos16(tmpStr, 0, &tmpVid))
        {
                printk("%s: Parse vlan ID ERROR %d !!!!!!!!!!!!!\n", __func__, __LINE__);
        }

        while((tmpStr = strsep(&input, " ")) != NULL)
        {
                if (tmpCount == CA_WMC_GROUP_PORT_MAX)
                {
                        printk("%s: Port over limit(%d) ERROR !!!!!!!!!!!!!\n", __func__, CA_WMC_GROUP_PORT_MAX);
                        return -1;
                }

                if (kstrtou32(tmpStr, 0, &tmpPort))
                {
                        printk("%s: Parse port ERROR %d !!!!!!!!!!!!!\n", __func__, __LINE__);
                }

                tmpPorts[tmpCount] = tmpPort;
                tmpCount++;
        }

        *vid = tmpVid;
        memcpy(member_ports, tmpPorts, sizeof(tmpPorts));
        *member_count = tmpCount;
        return CA_E_OK;
}

static wmc_flooding_t *lookup_flooding_table_by_vid(ca_int16_t vid)
{
	wmc_flooding_t *first = g_wmc_flooding_table;
	wmc_flooding_t *last = g_wmc_flooding_table + g_wmc_flooding_table_num;

	for (; first != last; ++first) {
		if (first->vid == vid) {
			return first;
		}
	}

	return NULL;
}

static void wmc_flooding_show_address(wmc_flooding_address_t address)
{
	ca_ip_afi_t afi = address.ip_da.afi;
	ca_ip_address_t addr_start, addr_max;
	ca_api_ip_address_htonl(&addr_start, &(address.ip_da));
	ca_api_ip_address_htonl(&addr_max, &(address.ip_da_max));
	if (afi == CA_IPV4) {
		ca_printf("%-15pI4 - %-15pI4\n", &(addr_start.ip_addr.ipv4_addr), &(addr_max.ip_addr.ipv4_addr));
	} else {
		ca_printf("%pI6 - %pI6\n", &(addr_start.ip_addr.ipv6_addr), &(addr_max.ip_addr.ipv6_addr));
	}
}

static ca_status_t wmc_flooding_add_cls_rule_by_address(wmc_flooding_address_t ip_range, ca_ip_afi_t afi)
{
        ca_status_t ret;
        ca_classifier_key_t key;
        ca_classifier_key_mask_t key_mask;
        ca_classifier_action_t	action;
        ca_uint8_t port_index = 0, addr_num;
        ca_uint32_t rule_index;
        ca_uint32_t src_port;
        ca_ip_address_t ip_start, ip_max;

        memset(&key, 0, sizeof(ca_classifier_key_t));
        memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
        memset(&action, 0, sizeof(ca_classifier_action_t));

        memcpy(&ip_start, &(ip_range.ip_da), sizeof(ca_ip_address_t));
        memcpy(&ip_max, &(ip_range.ip_da_max), sizeof(ca_ip_address_t));
        if(afi == CA_IPV4){
                addr_num = g_wmc_flooding_addr_v4_num;
                memcpy(&(g_wmc_flooding_address_v4_table[addr_num].ip_da), &ip_start, sizeof(ca_ip_address_t));
                memcpy(&(g_wmc_flooding_address_v4_table[addr_num].ip_da_max), &ip_max, sizeof(ca_ip_address_t));
                g_wmc_flooding_addr_v4_num++;
        } else {
                addr_num = g_wmc_flooding_addr_v6_num;
                memcpy(&(g_wmc_flooding_address_v6_table[addr_num].ip_da), &ip_start, sizeof(ca_ip_address_t));
                memcpy(&(g_wmc_flooding_address_v6_table[addr_num].ip_da_max), &ip_max, sizeof(ca_ip_address_t));
                g_wmc_flooding_addr_v6_num++;
        }

        /* setup cls key, address */
        memcpy(&(key.ip.ip_da), &ip_start, sizeof(ca_ip_address_t));
        memcpy(&(key.ip.ip_da_max), &ip_max, sizeof(ca_ip_address_t));

        /* setup cls key mask */
        key_mask.src_port 	        = 1;
        key_mask.ip 		        = 1;
        key_mask.ip_mask.ip_da 	        = 1;
        key_mask.ip_mask.ip_da_max 	= 1;

        /* setup cls action */
        action.forward = CA_CLASSIFIER_FORWARD_PORT;
        action.dest.port = CA_PORT_ID(CA_PORT_TYPE_CPU, g_wmc_cpu_port_id);
        for (port_index = 0; port_index < g_default_group.dst_port_num; port_index++)
        {
                rule_index = 0;
                src_port = g_default_group.dst_ports[port_index];

                /* setup cls key, source port */
                if ( src_port <= CA_PORT_ID_NI7 )
                        key.src_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, src_port);
                else if ( src_port >= CA_PORT_ID_CPU0 && src_port <= CA_PORT_ID_CPU7 ){
                        key.src_port = CA_PORT_ID(CA_PORT_TYPE_CPU, src_port);
			ca_printf("%s: CPU port 0x%x\n", __func__, key.src_port);
		}
		else {
			ca_printf("%s: not support portid 0x%x\n", __func__, src_port);
			continue;
		}
                ca_printf("\n%s: key.src_port=%x\n", __func__, key.src_port);

                ret = ca_classifier_rule_add(0, 7, &key, &key_mask, &action, &rule_index);
                if (ret != CA_E_OK) {
                        ca_printf("%s: classifier_rule_add() failed!!, ret=%d\n", __func__, ret);
                }

                ca_printf("%s: classifier_rule_add success, rule_index=%d\n", __func__, rule_index);
                ca_printf("===========================================\n", __func__, rule_index);
        }



        return CA_E_OK;
}

static ca_status_t ca_wmc_flooding_add_cls_rules(wmc_flooding_address_t ip_range, ca_ip_afi_t afi)
{
	ca_status_t ret;

	ca_printf("%s: addr = ", __func__);
	wmc_flooding_show_address(ip_range);
	ca_printf("\n");

	/* Add CLS rules: trap packet to PE. */
	if ((ret = wmc_flooding_add_cls_rule_by_address(ip_range, afi)) != CA_E_OK) {
		return ret;
	}

	return ret;
}

void wmc_flooding_add_def_ports(void) {
	ca_wmc_flooding_add_with_vid(g_default_group.vid,
			g_default_group.dst_ports,
			g_default_group.dst_port_num);
}

void wmc_flooding_add_def_address(void) {
	wmc_flooding_address_t default_ip_range;

	/* Default ipv4 address 224.0.0.1/24 */
	memset(&default_ip_range, 0, sizeof(default_ip_range));
	default_ip_range.ip_da.afi = CA_IPV4;
	default_ip_range.ip_da.ip_addr.ipv4_addr     = 0xE0000001;
	default_ip_range.ip_da_max.afi = CA_IPV4;
	default_ip_range.ip_da_max.ip_addr.ipv4_addr = 0xE00000FF;
	default_ip_range.ip_da_max.addr_len = 32;
	ca_wmc_flooding_add_cls_rules(default_ip_range, CA_IPV4);
	ca_wmc_flooding_add_address_ipc(&default_ip_range);


	/* Default ipv4 address 239.255.255.250, for UPnP */
	memset(&default_ip_range, 0, sizeof(default_ip_range));
	default_ip_range.ip_da.afi = CA_IPV4;
	default_ip_range.ip_da.ip_addr.ipv4_addr     = 0xEFFFFFFA;
	default_ip_range.ip_da_max.afi = CA_IPV4;
	default_ip_range.ip_da_max.ip_addr.ipv4_addr = 0xEFFFFFFA;
	default_ip_range.ip_da_max.addr_len = 32;
	ca_wmc_flooding_add_cls_rules(default_ip_range, CA_IPV4);
	ca_wmc_flooding_add_address_ipc(&default_ip_range);

	/* Default ipv6 address ff02::1/128 */
	memset(&default_ip_range, 0, sizeof(default_ip_range));
	default_ip_range.ip_da.afi = CA_IPV6;
	default_ip_range.ip_da.ip_addr.ipv6_addr[0] = 0xff020000;
	default_ip_range.ip_da.ip_addr.ipv6_addr[1] = 0x00000000;
	default_ip_range.ip_da.ip_addr.ipv6_addr[2] = 0x00000001;
	default_ip_range.ip_da.ip_addr.ipv6_addr[3] = 0xff000000;
	default_ip_range.ip_da_max.afi = CA_IPV6;
	default_ip_range.ip_da_max.ip_addr.ipv6_addr[0] = 0xff020000;
	default_ip_range.ip_da_max.ip_addr.ipv6_addr[1] = 0x00000000;
	default_ip_range.ip_da_max.ip_addr.ipv6_addr[2] = 0x00000001;
	default_ip_range.ip_da_max.ip_addr.ipv6_addr[3] = 0xffffffff;
	default_ip_range.ip_da_max.addr_len = 128;
	ca_wmc_flooding_add_cls_rules(default_ip_range, CA_IPV6);
	ca_wmc_flooding_add_address_ipc(&default_ip_range);
}

static void wmc_flooding_set_vlan_def_membership(ca_device_id_t device_id, ca_uint32_t enable) {
	ca_status_t ret;
	ca_uint32_t mapped_port_id;
	aal_l2_vlan_default_cfg_mask_t vlan_def_cfg_mask;
	aal_l2_vlan_default_cfg_t vlan_def_cfg;

	ret = aal_l2_vlan_port_memship_map_get(device_id, g_wmc_cpu_port_id, &mapped_port_id);
	if (ret != CA_E_OK) {
		ca_printf("%s: aal_l2_vlan_port_memship_map_get failed, ret = %d", __func__, ret);
		return;
	}

	vlan_def_cfg_mask.s.rx_unknown_vlan_memship_check_bmp = 1;
	vlan_def_cfg_mask.s.tx_unknown_vlan_memship_check_bmp = 1;
	memset(&vlan_def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
	ret = aal_l2_vlan_default_cfg_get(device_id, &vlan_def_cfg);
	if (ret != CA_E_OK) {
		ca_printf("%s: aal_l2_vlan_default_cfg_get failed, ret = %d", __func__, ret);
		return;
	}

	if (enable) {
		vlan_def_cfg.rx_unknown_vlan_memship_check_bmp |= (1 << mapped_port_id);
		vlan_def_cfg.tx_unknown_vlan_memship_check_bmp |= (1 << mapped_port_id);
	} else {
		vlan_def_cfg.rx_unknown_vlan_memship_check_bmp &= ~(1 << mapped_port_id);
		vlan_def_cfg.tx_unknown_vlan_memship_check_bmp &= ~(1 << mapped_port_id);
	}

	ret = aal_l2_vlan_default_cfg_set(device_id, vlan_def_cfg_mask, &vlan_def_cfg);
	if (ret != CA_E_OK) {
		ca_printf("%s: aal_l2_vlan_default_cfg_set failed, ret = %d", __func__, ret);
		return;
	}

	return;
        }

static void wmc_flooding_set_pe_property(ca_device_id_t device_id, ca_uint32_t enable)
{
	ca_status_t        ret = CA_E_OK;
	aal_ilpb_cfg_msk_t igr_msk;
	aal_ilpb_cfg_t     igr_cfg;

	memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
	memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

	igr_msk.s.spcl_pkt_idx = 1;
	igr_msk.s.stp_mode = 1;
	igr_cfg.spcl_pkt_idx = enable ? AAL_PORT_SPB_IDX_NO_USED : AAL_PORT_SPB_IDX_RSVD;
	igr_cfg.stp_mode = enable ? AAL_STP_STATE_FWD_NO_LEARN : AAL_STP_STATE_FWD_AND_LEARN;
	ret = aal_port_ilpb_cfg_set(device_id, g_wmc_cpu_port_id, igr_msk, &igr_cfg);
	if (CA_E_OK != ret) {
		ca_printf("aal_port_ilpb_cfg_set port %u fail, return %#x.\n", g_wmc_cpu_port_id, ret);
	}
}

static int wmc_flooding_ipc_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
                ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
        ca_uint32_t i;
        const char *data = msg_data;
        ca_printf("msg_no %d trans_id %d msg_size %d\n", msg_no, trans_id, *msg_size);
        ca_printf("session_id %d cpu_id %d \n\t", peer.session_id, peer.cpu_id);
        for (i = 0; i < *msg_size; i++) {
                ca_printf("%02X ", data[i]);
                if ((i % 32) == 31)
                        ca_printf("\n\t");
        }
        ca_printf("\n");

        return CA_E_OK;
}

static ca_status_t wmc_flooding_ipc(ca_uint16_t msg_no,
				    const void *msg_data, ca_uint16_t msg_size,
				    void *rcv_buff, ca_uint16_t result_size)
{
        ca_status_t ret;
        ca_ipc_pkt_t ipc_pkt;

        ipc_pkt.session_id = CA_IPC_SESSION_WMC_FLOODING;
	ipc_pkt.dst_cpu_id = g_wmc_ipc_id;
        ipc_pkt.priority = 0;
	ipc_pkt.msg_no = msg_no;
	ipc_pkt.msg_data = msg_data;	/* the message data to transmit. */
	ipc_pkt.msg_size = msg_size;	/* the length of message data. */
                ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
                if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send failed, ret = %d\n", __func__, ret);
		return ret;
	}

	ca_printf("%s: ca_ipc_msg_sync_send OK\n", __func__);
                        return CA_E_OK;
                }

static ca_status_t wmc_flooding_status_ipc(ca_uint16_t status)
{
	int i;
	ca_status_t ret;
	char rcv_buff[16];

	for (i = 0; i < 5; ++i) {
		ret = wmc_flooding_ipc(status, NULL, 0,
				       rcv_buff, sizeof(rcv_buff));
		if (ret == CA_E_OK) {
			return CA_E_OK;
        }

		ca_sleep(100);
	}
        return ret;
}

static ca_status_t ca_wmc_flooding_dump_ipc(void)
{
        char rcv_buff[16];

	return wmc_flooding_ipc(CA_WMC_FLOODING_DUMP_IPC, NULL, 0,
				rcv_buff, sizeof(rcv_buff));
}

static ca_status_t ca_wmc_flooding_add_ipc(wmc_flooding_t *flooding_rule)
{
        char rcv_buff[sizeof(wmc_flooding_t)];

	return wmc_flooding_ipc(CA_WMC_FLOODING_ADD_IPC,
				flooding_rule, sizeof(wmc_flooding_t),
				rcv_buff, sizeof(rcv_buff));
}

static ca_status_t ca_wmc_flooding_add_address_ipc(wmc_flooding_address_t *ip_range)
{
        char rcv_buff[sizeof(wmc_flooding_address_t)];

	return wmc_flooding_ipc(CA_WMC_FLOODING_ADD_ADDRESS_IPC,
				ip_range, sizeof(wmc_flooding_address_t),
				rcv_buff, sizeof(rcv_buff));
}

static ca_status_t ca_wmc_flooding_delete_ipc(ca_int16_t vlan_id)
{
        char rcv_buff[sizeof(ca_int16_t)];

	return wmc_flooding_ipc(CA_WMC_FLOODING_DELETE_IPC,
				&vlan_id, sizeof(ca_int16_t),
				rcv_buff, sizeof(rcv_buff));
}

static ca_status_t ca_wmc_flooding_delete_all_ipc(void)
{
        char rcv_buff[16];

	return wmc_flooding_ipc(CA_WMC_FLOODING_DELETE_ALL_IPC,
				NULL, 0, rcv_buff, sizeof(rcv_buff));
        }

static ca_status_t ca_wmc_flooding_set_mode_ipc(ca_uint16_t msg_no, ca_uint32_t mode)
{
	char rcv_buff[sizeof(ca_uint32_t)];

	return wmc_flooding_ipc(msg_no, &mode, sizeof(ca_uint32_t),
				rcv_buff, sizeof(rcv_buff));
}

void ca_wmc_flooding_show_table(void)
{
        ca_uint8_t table_index, dst_port_num;
        ca_ip_address_t addr_start, addr_max;
        printk("%s: g_wmc_flooding_enable = %d\n\n", __func__, g_wmc_flooding_enable);
        printk("--------------------Port Table--------------------\n");
        printk("VID\tDST_PORTS\n");
        printk("--------------------------------------------------\n");
        for(table_index = 0; table_index < g_wmc_flooding_table_num; table_index++)
        {
                printk("%d\t", g_wmc_flooding_table[table_index].vid );
                for(dst_port_num = 0; dst_port_num < g_wmc_flooding_table[table_index].dst_port_num; dst_port_num++)
                        printk("0x%x  ", g_wmc_flooding_table[table_index].dst_ports[dst_port_num] );
                printk("\n");
        }

        printk("\n--------------------IPv4 Table--------------------\n");
        for(table_index = 0; table_index < g_wmc_flooding_addr_v4_num; table_index++)
        {
                ca_api_ip_address_htonl(&addr_start, &(g_wmc_flooding_address_v4_table[table_index].ip_da));
                ca_api_ip_address_htonl(&addr_max, &(g_wmc_flooding_address_v4_table[table_index].ip_da_max));
                ca_printf("%pI4 - %pI4\n", &(addr_start.ip_addr.ipv4_addr),
                                &(addr_max.ip_addr.ipv4_addr));
        }

        printk("\n--------------------IPv6 Table--------------------\n");
        for(table_index = 0; table_index < g_wmc_flooding_addr_v6_num; table_index++)
        {
                ca_api_ip_address_htonl(&addr_start, &(g_wmc_flooding_address_v6_table[table_index].ip_da));
                ca_api_ip_address_htonl(&addr_max, &(g_wmc_flooding_address_v6_table[table_index].ip_da_max));
                ca_printf("%pI6 - %pI6\n", &(addr_start.ip_addr.ipv6_addr),
                                &(addr_max.ip_addr.ipv6_addr));
        }

        ca_wmc_flooding_dump_ipc();
}

static void ca_wmc_floodin_init_default_group(void)
{
        ca_uint8_t num;
        memset(g_default_group.dst_ports, 0, CA_WMC_GROUP_PORT_MAX*sizeof(ca_uint32_t));
        g_default_group.dst_port_num = 0;
        for(num = 0; num < CFG_ID_FLOODING_DOMAIN_1_LEN; num++)
        {
                if (g_vlan_flooding_domain_1[num] != 0xFF &&
				g_vlan_flooding_domain_1[num] != g_wmc_cpu_port_id){
                        g_default_group.dst_ports[g_default_group.dst_port_num] = g_vlan_flooding_domain_1[num];
                        g_default_group.dst_port_num++;
                }
        }

        for(num = 0; num < g_default_group.dst_port_num; num++)
                printk("%s: g_default_group.dst_port = %x\n", __func__, g_default_group.dst_ports[num]);
        g_default_group.vid = -1;
}

void ca_wmc_flooding_add_address(char *address, ca_ip_afi_t afi)
{
        wmc_flooding_address_t *ip_range = parseSubnetAddress(address, afi);
        ca_wmc_flooding_add_cls_rules(*ip_range, afi);
        ca_wmc_flooding_add_address_ipc(ip_range);
		ca_free(ip_range);
}

static void ca_wmc_flooding_add_with_vid(ca_int16_t vid, ca_uint32_t *member_ports, ca_uint8_t member_count)
{
        wmc_flooding_t flooding_rule, *founded;

	founded = lookup_flooding_table_by_vid(vid);
	if (founded != NULL) {
		ca_printf("%s: There is already vid(%d), please use update API.\n", __func__, vid);
		return;
	}

        /*IPC, update taroko side*/
        memset(flooding_rule.dst_ports, 0, sizeof(flooding_rule.dst_ports));
        memcpy(flooding_rule.dst_ports, member_ports, sizeof(*member_ports)*member_count);
        flooding_rule.dst_port_num = member_count;
        flooding_rule.vid = vid;
        ca_wmc_flooding_add_ipc(&flooding_rule);

        memcpy(&(g_wmc_flooding_table[g_wmc_flooding_table_num]), &flooding_rule, sizeof(wmc_flooding_t));
        g_wmc_flooding_table_num++;
}

void ca_wmc_flooding_add(char *input)
{
        ca_int16_t vid;
        ca_uint32_t member_ports[CA_WMC_GROUP_PORT_MAX];
        ca_uint8_t member_count = 0;
        ca_status_t ret;

        ret = parseInput(input, &vid, member_ports, &member_count);
        if (ret == CA_E_OK)
                ca_wmc_flooding_add_with_vid(vid, member_ports, member_count);
}

static void ca_wmc_flooding_update_with_vid(ca_int16_t vid,
					    ca_uint32_t * member_ports,
					    ca_uint8_t member_count)
{
	wmc_flooding_t *founded;

	founded = lookup_flooding_table_by_vid(vid);
	if (founded == NULL) {
		return;
	}

	memcpy(founded->dst_ports + founded->dst_port_num, member_ports,
	       sizeof(*member_ports) * member_count);
	founded->dst_port_num += member_count;

                        /*IPC, update taroko side*/
	ca_wmc_flooding_add_ipc(founded);

                        return;
                }

void ca_wmc_flooding_update(char *input)
{
        ca_int16_t vid;
        ca_uint32_t member_ports[CA_WMC_GROUP_PORT_MAX];
        ca_uint8_t member_count = 0;
        ca_status_t ret;

        ret = parseInput(input, &vid, member_ports, &member_count);
        if (ret == CA_E_OK)
        {
                ca_wmc_flooding_update_with_vid(vid, member_ports, member_count);
        }
}

void ca_wmc_flooding_delete(char *input)
{
        ca_int16_t vid;
	wmc_flooding_t *founded;

	if (kstrtos16(input, 0, &vid)) {
                goto input_invalid;
	}

	founded = lookup_flooding_table_by_vid(vid);
	if (founded == NULL) {
		goto input_invalid;
	}

	memcpy(founded, g_wmc_flooding_table + (g_wmc_flooding_table_num - 1),
                                        sizeof(wmc_flooding_t));
                        g_wmc_flooding_table_num--;

                        /*IPC, update taroko side*/
                        ca_wmc_flooding_delete_ipc(vid);
                        return;

input_invalid:
	printk("%s: Wrong Entry ID(%d)!!!!!!!!!!!!!!!!!!!\n", __func__, vid);
}

void ca_wmc_flooding_delete_all(void)
{
        g_wmc_flooding_table_num = 0;

        /*IPC, update taroko side*/
        ca_wmc_flooding_delete_all_ipc();
}

void ca_wmc_flooding_set_mode(char *input, unsigned long msg_no)
{
        ca_status_t ret;
	char *tmpStr;
	ca_uint32_t mode;

	tmpStr = strsep(&input, " ");
	if (kstrtou32(tmpStr, 0, &mode)) {
		ca_printf("%s: Parse mode ERROR!!!!!!!!!!!!!\n", __func__);
		return;
	}

	if (mode > 1) {
		ca_printf("%s: Don't have mode(%d)!!!!!!!!!!!!!\n", __func__, mode);
		return;
	}

        /*IPC, update taroko side*/
	ret = ca_wmc_flooding_set_mode_ipc(msg_no, mode);
	if (ret != CA_E_OK) {
		ca_printf("%s: IPC failed\n", __func__);
	}

                return;
        }

void wmc_flooding_disable(void)
{
	ca_well_known_mc_flooding_enable_set(0, FALSE);
}

void wmc_flooding_enable(ca_uint8_t add_default)
{
        ca_status_t ret;

	ret = ca_well_known_mc_flooding_enable_set(0, TRUE);
        if(ret != CA_E_OK)
                return;

	if (add_default) {
		if (!g_wmc_flooding_init) {
			wmc_flooding_add_def_ports();
			wmc_flooding_add_def_address();
			g_wmc_flooding_init = TRUE;
		} else {
			ca_printf("%s: Already add default address.\n", __func__);
        }
	}
}

ca_status_t ca_well_known_mc_flooding_enable_set(
	CA_IN   ca_device_id_t  device_id,
	CA_IN   ca_boolean_t    enable)
{
	ca_status_t ret;
	ca_uint16_t status = enable ? CA_WMC_FLOODING_ENABLE_IPC : CA_WMC_FLOODING_DISABLE_IPC;

	if (g_wmc_flooding_enable == enable) return CA_E_OK;

	/*IPC, update taroko side*/
	ret = wmc_flooding_status_ipc(status);
	if(ret != CA_E_OK)
        {
		ca_printf("%s: IPC failed, ret = %d\n", __func__, ret);
		return ret;
	}

	wmc_flooding_set_pe_property(device_id, enable);
	wmc_flooding_set_vlan_def_membership(device_id, enable);
	g_wmc_flooding_enable = enable;

	return CA_E_OK;
}

ca_status_t ca_well_known_mc_flooding_enable_get(
	CA_IN   ca_device_id_t  device_id,
	CA_OUT  ca_boolean_t    *enable)
{
	if (enable == NULL) return CA_E_PARAM;

	*enable = g_wmc_flooding_enable && g_wmc_flooding_default;
	return CA_E_OK;
}

ca_status_t well_known_mcast_flooding_init(void)
{
        /* Init IPC start */
        ca_status_t ret;

        ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_WMC_FLOODING, wmc_flooding_ipc_msg, CA_WMC_FLOODING_IPC_MAX);

        if (ret != CA_E_OK) {
                ca_printf("%s: ca-ipc_msg_handle_register() fail ret %d\n", __func__, ret);
                return CA_E_INIT;
        }
        else ca_printf("%s: ca-ipc_msg_handle_register() Success!!!!!!\n", __func__);

        ret = aal_scfg_read(CFG_ID_RFC4541_FLOODING_ENABLE, 1, &g_wmc_flooding_default);
	if (ret != CA_E_OK) {
		ca_printf("%s: failed to read CFG_ID_RFC4541_FLOODING_ENABLE, ret=%d\n", __func__, ret);
	}

        ret = aal_scfg_read(CFG_ID_WELL_KNOWN_MC_OFFLOAD_PE_ID, 1, &g_wmc_flooding_pe_id);
	if (ret != CA_E_OK) {
		ca_printf("%s: failed to read CFG_ID_WELL_KNOWN_MC_OFFLOAD_PE_ID, ret=%d\n", __func__, ret);
	}

	g_wmc_cpu_port_id = g_wmc_flooding_pe_id ? CA_PORT_ID_CPU7 : CA_PORT_ID_CPU6;
	g_wmc_ipc_id = g_wmc_flooding_pe_id ? CA_IPC_CPU_PE1 : CA_IPC_CPU_PE0;

	ca_printf("g_wmc_cpu_port_id = 0x%x, g_wmc_ipc_id = 0x%x\n", g_wmc_cpu_port_id, g_wmc_ipc_id);
        g_wmc_flooding_addr_v4_num = 0;
        g_wmc_flooding_addr_v6_num = 0;
        g_wmc_flooding_table_num = 0;
        ca_wmc_floodin_init_default_group();

	return CA_E_OK;
}


EXPORT_SYMBOL(ca_well_known_mc_flooding_enable_set);
EXPORT_SYMBOL(ca_well_known_mc_flooding_enable_get);
