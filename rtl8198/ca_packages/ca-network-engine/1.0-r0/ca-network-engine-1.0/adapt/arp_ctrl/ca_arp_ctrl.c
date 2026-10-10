#include <linux/proc_fs.h>
#include <linux/inet.h>
#include <soc/cortina/cortina-soc.h>
#include "ca_arp_ctrl.h"
#include "ca_adapt_proc.h"
#include "ca-scfg/scfg.h"
#include "aal_ni.h"
#include "pkt_tx.h"
#include "ca_ni_dump.h"
#include "ca_ni.h"
#include "ca_ni_proc.h"
#include "special_packet.h"

#define CA_ARP_CTRL_SHOW_ALL			0
#define CA_ARP_CTRL_SHOW_INTF_IPADDR		1
#define CA_ARP_CTRL_SHOW_ACTION			2

struct proc_dir_entry *proc_arp_ctrl = NULL;
extern ca_ni_intf_ipaddr_t ca_ni_intf_ipaddr[CA_NI_INTF_IPADDR_MAX];
extern ca_ni_arp_fwd_bh_t ca_ni_arp_fwd_bh_table[CA_NI_ARP_FWD_BH_MAX];
extern ca_uint32_t arp_ctrl_enable;
extern ca_uint32_t arp_ctrl_debug;
extern ca_uint32_t arp_ctrl_wan_port_id;

typedef enum {
	TAG_ANY = -1,
	UNTAG = 0,
	TAG,
} vlan_type_t;

typedef enum {
	NOTHING = 0,
	VLAN_PUSH,
	VLAN_POP,
	VLAN_SWAP,
} vlan_cmd_t;

#define VLAN_ANY 0x0

static ca_uint8_t pon_mac_mode;
ca_uint32_t arp_ctrl_dev_ports[8];
ca_uint32_t arp_ctrl_dev_port_num;

static ca_status_t set_special_packet_to_cpu(void)
{
        int enable;
        ca_port_id_t source_port;
        ca_port_id_t cpu_port;
        ca_status_t ret;
        ca_special_packet_option_t   options = {0};
        ca_pkt_type_t pkt_type;

        pkt_type = CA_PKT_TYPE_ARP;
        enable = 1;

	printk("%s: pkt_type=%d\n", __func__, pkt_type);

        source_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI3);
        cpu_port = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU0);
        ret = ca_special_packet_set(0, source_port, pkt_type, enable, cpu_port, 7, &options);
        printk("%s: ca_special_packet_set(source_port=0x%x) ret=%d\n", __func__, source_port, ret);

        source_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI4);
        ret = ca_special_packet_set(0, source_port, pkt_type, enable, cpu_port, 7, &options);
        printk("%s: ca_special_packet_set(source_port=0x%x) ret=%d\n", __func__, source_port, ret);

        source_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI5);
        ret = ca_special_packet_set(0, source_port, pkt_type, enable, cpu_port, 7, &options);
        printk("%s: ca_special_packet_set(source_port=0x%x) ret=%d\n", __func__, source_port, ret);

        source_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, CA_PORT_ID_NI6);
        ret = ca_special_packet_set(0, source_port, pkt_type, enable, cpu_port, 7, &options);
        printk("%s: ca_special_packet_set(source_port=0x%x) ret=%d\n", __func__, source_port, ret);

        source_port = arp_ctrl_wan_port_id;
        ret = ca_special_packet_set(0, source_port, pkt_type, enable, cpu_port, 7, &options);
        printk("%s: ca_special_packet_set(source_port=0x%x) ret=%d\n", __func__, source_port, ret);

        return ret;
}

static void arp_fwd_device_port_get(void)
{
	ca_uint32_t temp_port_bit_map = 0, scfg_port_bit_map = 0;
	ca_uint32_t i = 0, port_num = 0;
	ca_uint8_t scfg_wan_port_id = 0;

	aal_scfg_read(CFG_ID_WAN_PORT_ID, 1, &scfg_wan_port_id);
	arp_ctrl_dev_ports[port_num++] = scfg_wan_port_id;

	aal_scfg_read(CFG_ID_GE_PORT_BIT_MAP, 4, &scfg_port_bit_map);
	temp_port_bit_map |= scfg_port_bit_map;
	aal_scfg_read(CFG_ID_XGE_PORT_BIT_MAP, 4, &scfg_port_bit_map);
	temp_port_bit_map |= scfg_port_bit_map;
	for (i = 0; i < 32 ; i++) {
		if (temp_port_bit_map & (1 << i))
			arp_ctrl_dev_ports[port_num++] = i;
	}

	arp_ctrl_dev_port_num = port_num;
}

static ca_ni_arp_fwd_bh_t *arp_ctrl_fwd_bh_table_search(
		ca_uint32_t ing_port, ca_uint32_t ing_tag, ca_uint32_t ing_vid)
{
	ca_ni_arp_fwd_bh_t *first = ca_ni_arp_fwd_bh_table;
	ca_ni_arp_fwd_bh_t *last = ca_ni_arp_fwd_bh_table + CA_NI_ARP_FWD_BH_MAX;
	ca_ni_arp_fwd_bh_t *tmp = NULL;
	for (; first != last; first++) {
		if (first->valid &&
				first->ingress_port_id == ing_port &&
				first->ingress_vtag == ing_tag) {
			if (ing_tag == TAG && first->ingress_vid == VLAN_ANY) {
				/* Find Tag ANY table */
				tmp = first;
			} else if (first->ingress_vid == ing_vid) {
				/* Find specific table */
				return first;
			}
		}
	}

	return tmp;
}

ca_uint32_t ca_arp_ctrl_check_local_fwd(ca_uint32_t lspid, ca_uint16_t vid){
	ca_ni_arp_fwd_bh_t *fwd_rule;
	fwd_rule = arp_ctrl_fwd_bh_table_search(lspid, vid == 0 ? UNTAG : TAG, vid);
	if (fwd_rule != NULL)
		return fwd_rule->local_fwd;

	return TRUE;
}

#define VLAN_HEADER_LEN         4
#define PACKET_HEADER_LEN       12
ca_status_t ca_arp_ctrl_send_packet(ca_uint16_t lspid, ca_uint16_t ldpid, ca_uint16_t vid,
		int lso_txq_index, ca_uint8_t *data, int vlan_offset, int pkt_len)
{
	ca_uint8_t      *pkt_data;
	ca_uint16_t     *vlan_ptr;
	ca_uint32_t     eg_sub_port = 0;
	ca_ni_arp_fwd_bh_t *fwd_rule;
	arp_fwd_egress_port_t *eg_port;
	ca_pkt_t        pkt;
	ca_pkt_block_t  pkt_blk;
	ca_status_t ret;
	int offset;

	if (arp_ctrl_debug) {
		printk("%s: lspid=%d, ldpid=%d, vid=%d, lso_txq_index=%d, data=0x%p, vlan_offset=%d, pkt_len=%d\n",
				__func__, lspid, ldpid, vid, lso_txq_index, data, vlan_offset, pkt_len);
	}


	pkt_data = kmalloc(pkt_len + VLAN_HEADER_LEN, GFP_KERNEL);
	if (pkt_data == NULL)
	{
		printk("%s: memory not enough for %d bytes!!\n", __func__, pkt_len + VLAN_HEADER_LEN);
		return CA_E_MEMORY;
	}

	fwd_rule = arp_ctrl_fwd_bh_table_search(lspid, vid == 0 ? UNTAG : TAG, vid);

	if (fwd_rule) {

		eg_port = &(fwd_rule->egress_port[ldpid]);

		if (eg_port->valid) {
			memcpy(pkt_data, data, PACKET_HEADER_LEN);

			if (eg_port->vlan_cmd == VLAN_PUSH) {
				/* PUSH */
				memmove(pkt_data + PACKET_HEADER_LEN + VLAN_HEADER_LEN, data + PACKET_HEADER_LEN,
						pkt_len - PACKET_HEADER_LEN);
				vlan_ptr = (ca_uint16_t *)(pkt_data + PACKET_HEADER_LEN);
				vlan_ptr[0] = htons(ETH_P_8021Q);
				vlan_ptr[1] = htons(eg_port->vid);
				pkt_len += VLAN_HEADER_LEN;
			} else if (eg_port->vlan_cmd == VLAN_POP) {
				/* POP */
				if (vlan_offset > 0) {
					offset = VLAN_HEADER_LEN;
					pkt_len -= VLAN_HEADER_LEN;
				}
				else {
					offset = 0;
				}
				memmove(pkt_data + PACKET_HEADER_LEN, data + PACKET_HEADER_LEN + offset, pkt_len - (PACKET_HEADER_LEN + offset));
			} else if (eg_port->vlan_cmd == VLAN_SWAP) {
				/* SWAP */
				memmove(pkt_data + PACKET_HEADER_LEN, data + PACKET_HEADER_LEN, pkt_len - PACKET_HEADER_LEN);
				if (vlan_offset > 0) {
					vlan_ptr = (ca_uint16_t *)(pkt_data + PACKET_HEADER_LEN);
					vlan_ptr[1] = htons(eg_port->vid);      /* swap VID keep TPID unchanged */
				}
			}
			else { /* do nothing */
				memmove(pkt_data, data, pkt_len);
			}
			eg_sub_port = eg_port->eg_sub_port;

			if (arp_ctrl_debug) {
				printk("%s: vlan_cmd=%d, eg_sub_port=%d\n", __func__, eg_port->vlan_cmd, eg_sub_port);
				printk("%s: pkt_data[]=\n", __func__);
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, (void *)pkt_data, pkt_len, true);
			}
		}
		else {
			memmove(pkt_data, data, pkt_len);
		}
	}
	else {
		memmove(pkt_data, data, pkt_len);
	}

	/* prepare ca_pkt_t and send it out through __ca_tx() */
	memset(&pkt, 0, sizeof(ca_pkt_t));
	memset(&pkt_blk, 0, sizeof(ca_pkt_block_t));
	pkt_blk.data = pkt_data;
	pkt_blk.len = pkt_len;
	pkt.pkt_data = &pkt_blk;
	pkt.block_count = 1;
	pkt.device_id = 0;
	pkt.src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU0);

	if (ldpid == CA_PORT_ID_NI7)
		pkt.dst_port_id = arp_ctrl_wan_port_id;
	else
		pkt.dst_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, ldpid);

	pkt.dst_sub_port_id = eg_sub_port;
	pkt.pkt_len = pkt_len;
	pkt.flags = CA_TX_BYPASS_FWD_ENGINE;
	pkt.cos = 6;

	if (arp_ctrl_debug) {
		printk("%s: src_port_id=0x%x, dst_port_id=0x%x, sub_port_id=%d\n", __func__,
				pkt.src_port_id, pkt.dst_port_id, pkt.dst_sub_port_id);
	}

	ret = __ca_tx(0, &pkt);
	kfree(pkt_data);

	return ret;
}

static void arp_ctrl_fwd_wan_port_id_get(void)
{
	ca_uint8_t   scfg_wan_port_type, wan_port_type;

#ifdef CONFIG_ARCH_CORTINA_G3
	wan_port_type = CA_PORT_TYPE_ETHERNET;
#else
	aal_scfg_read(CFG_ID_PON_MAC_MODE, 1, &scfg_wan_port_type);
	if(CA_PON_MODE_IS_EPON_FAMILY(scfg_wan_port_type))
		wan_port_type = CA_PORT_TYPE_EPON;
	else if(CA_PON_MODE_IS_GPON_FAMILY(scfg_wan_port_type))
		wan_port_type = CA_PORT_TYPE_GPON;
	else wan_port_type = CA_PORT_TYPE_ETHERNET;
#endif

	arp_ctrl_wan_port_id = CA_PORT_ID(wan_port_type, CA_PORT_ID_NI7);
}

static void __arp_ctrl_dump_bh_rule(ca_ni_arp_fwd_bh_t rule)
{
	ca_uint32_t j;
	const char *vlan_action[] = {"Nothing", "Push", "Pop", "Swap"};

	ca_printf("Ingress Port ID = %u: ", rule.ingress_port_id);
	ca_printf("%s ", rule.ingress_vtag ? "Tag" : "Untag");
	ca_printf("VID[%u], ", rule.ingress_vid);
	ca_printf("local fwd = %u\n", rule.local_fwd);

	for (j = 0; j < 8; j++) {
		if ((j != rule.ingress_port_id) && rule.egress_port[j].valid) {
			ca_printf("\t\tEgress Port ID = %u: %s, VID[%u]\n",
					j, vlan_action[rule.egress_port[j].vlan_cmd], rule.egress_port[j].vid);
			ca_printf("\t\tEgress Sub Port ID = 0x%x\n", rule.egress_port[j].eg_sub_port);
		}
	}
	ca_printf("\n");
}

/* ***********************************
 *        Interface IP address
 * ***********************************/
ca_boolean_t ca_api_is_ip_equal(ca_ip_address_t *ip_addr1, ca_ip_address_t *ip_addr2);
ca_status_t ca_rx_set_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_ip_address_t   *ip_addr)
{
	int i;

	if (arp_ctrl_debug) {
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv4_addr=%pI4\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv4_addr);
		}
		else {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv6_addr=%pI6\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv6_addr);
		}
	}

	/* check whether entry exists already */
	for (i = 0; i < CA_NI_INTF_IPADDR_MAX; i++) {
		if (ca_ni_intf_ipaddr[i].valid == 1) {
			if (ca_api_is_ip_equal(ip_addr, &(ca_ni_intf_ipaddr[i].intf_ip))) {
				if (arp_ctrl_debug)
					printk("%s: IP address exists!!\n", __func__);
				return CA_E_EXISTS;
			}
		}
	}

	/* serach a empty entry */
	for (i = 0; i < CA_NI_INTF_IPADDR_MAX; i++) {
		if (ca_ni_intf_ipaddr[i].valid == 0) {
			ca_ni_intf_ipaddr[i].valid = 1;
			memcpy(&(ca_ni_intf_ipaddr[i].intf_ip), ip_addr, sizeof(ca_ip_address_t));

			if (arp_ctrl_debug)
				printk("%s: Add to index %d!!\n", __func__, i);
			return CA_E_OK;
		}
	}
	return CA_E_RESOURCE;
}

ca_status_t ca_rx_get_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_uint32_t 	index,
		ca_ip_address_t *ip_addr)
{
	if (ca_ni_intf_ipaddr[index].valid == 0) {
		return CA_E_NOT_FOUND;
	}

	memcpy(ip_addr, &(ca_ni_intf_ipaddr[index].intf_ip), sizeof(ca_ip_address_t));

	if (arp_ctrl_debug) {
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv4_addr=%pI4\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv4_addr);
		}
		else {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv6_addr=%pI6\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv6_addr);
		}
	}

	return CA_E_OK;
}

ca_status_t ca_rx_check_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_ip_address_t   *ip_addr)
{
	int i;

	if (arp_ctrl_debug) {
		if (ip_addr->afi == CA_IPV4) {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv4_addr=%pI4\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv4_addr);
		}
		else {
			printk("%s: ip_addr->afi=%d, ip_addr->addr_len=%d, ip_addr->ip_addr.ipv6_addr=%pI6\n", __func__,
					ip_addr->afi, ip_addr->addr_len, &ip_addr->ip_addr.ipv6_addr);
		}
	}

	/* check whether entry exists already */
	for (i = 0; i < CA_NI_INTF_IPADDR_MAX; i++) {
		if (ca_ni_intf_ipaddr[i].valid == 1) {
			if (arp_ctrl_debug) {
				printk("%s: ip_addr->ip_addr.ipv4_addr=0x%x, ca_ni_intf_ipaddr[i].intf_ip.ip_addr.ipv4_addr=0x%x\n", __func__,
						ip_addr->ip_addr.ipv4_addr, ca_ni_intf_ipaddr[i].intf_ip.ip_addr.ipv4_addr);
			}
			if (ca_api_is_ip_equal(ip_addr, &(ca_ni_intf_ipaddr[i].intf_ip))) {
				if (arp_ctrl_debug) {
					printk("%s: Got interface ip address at index %d\n", __func__, i);
				}
				return CA_E_OK;
			}
		}
	}
	if (arp_ctrl_debug) {
		printk("%s: can not find interface ip address!!\n", __func__);
	}
	return CA_E_ERROR;
}

#define CA_NE_ARP_CTRL_INTF_IP_HELP_MSG \
	"Purpose: Setting interface IP address\n" \
"READ Usage: cat %s\n" \
"WRITE Usage: echo [IP ADDR] > %s\n" \
"\n"

static int arp_ctrl_intf_ip_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;
	seq_printf(m, CA_NE_ARP_CTRL_INTF_IP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	return 0;
}

int arp_ctrl_intf_ip_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, arp_ctrl_intf_ip_show_help, (void *) file);
}

ssize_t arp_ctrl_intf_ip_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[64];
	ssize_t len;
	char *cp = tmp;
	ca_status_t ret;
	ca_ip_address_t ipaddr;
	ca_ip_afi_t afi = CA_IPV6;
	char *tmpStr, *address;
	u8 saddr4[4], saddr6[16];

	len = min(count, sizeof(tmp) - 1);
	if (copy_from_user(tmp, buf, len))
		goto _arp_ctrl_invalid;

	tmp[len] = '\0';

	// Get the start address...
	tmpStr = address = strsep(&cp, " \n");

	for (tmpStr = address; *tmpStr !='\0'; tmpStr++)
	{
		if(*tmpStr == '.') {
			afi = CA_IPV4;
			break;
		}
	}

	if(afi == CA_IPV4)
		ret = in4_pton(address, -1, &saddr4, -1, NULL);
	else
		ret = in6_pton(address, -1, &saddr6, -1, NULL);
	if (!ret) {
		ca_printf("%s: Convert %s to binary representation failed!\n", __func__, address);
		goto _arp_ctrl_invalid;
	}

	ipaddr.afi = afi;
	if(afi == CA_IPV4) {
		memcpy(&(ipaddr.ip_addr.ipv4_addr), &saddr4, sizeof(ipaddr.ip_addr.ipv4_addr));
#ifdef CA_BIG_ENDIAN
		ipaddr.ip_addr.ipv4_addr = ntohls(ipaddr.ip_addr.ipv4_addr);
#endif
		ipaddr.addr_len = 32;
	} else {
		memcpy(&(ipaddr.ip_addr.ipv6_addr), &saddr6, sizeof(ipaddr.ip_addr.ipv6_addr));
#ifdef CA_BIG_ENDIAN
		ipaddr.ip_addr.ipv6_addr = ntohls(ipaddr.ip_addr.ipv6_addr);
#endif
		ipaddr.addr_len = 128;
	}


	ca_rx_set_intf_ipaddr(0, &ipaddr);
	return count;

_arp_ctrl_invalid:
	ca_printf(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_arp_ctrl_intf_ip __read_mostly = {
	.owner          = THIS_MODULE,
	.open           = arp_ctrl_intf_ip_proc_open,
	.read           = seq_read,
	.write          = arp_ctrl_intf_ip_proc_write,
	.release        = single_release,
};

/* ***********************************
 *             Vlan action
 * ***********************************/
#define CA_NE_ARP_CTRL_VLAN_BH_HELP_MSG \
	"Purpose: Setting ARP behavior\n" \
"READ Usage: cat %s\n" \
"WRITE Usage: echo [ING_PORT] [ING_TAG] [ING_VID] [LOCAL_FWD] [EG_PORT] [EG_SUB_PORT] [VLAN_CMD] [EG_VID] > %s\n" \
"\n"

#define ARP_CTRL_VLAN_BH_INGRESS_PORT		0
#define ARP_CTRL_VLAN_BH_INGRESS_TAG		1
#define ARP_CTRL_VLAN_BH_INGRESS_VID		2
#define ARP_CTRL_VLAN_BH_LOCAL_FWD		3
#define ARP_CTRL_VLAN_BH_EGRESS_PORT		4
#define ARP_CTRL_VLAN_BH_EGRESS_SUB_PORT	5
#define ARP_CTRL_VLAN_BH_EGRESS_VLAN_CMD	6
#define ARP_CTRL_VLAN_BH_EGRESS_VID		7
#define ARP_CTRL_VLAN_BH_MAX			8

#define ARP_CRTL_VLAN_BH_ANY			-1

static int arp_ctrl_vlan_bh_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;
	seq_printf(m, CA_NE_ARP_CTRL_VLAN_BH_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	return 0;
}

int arp_ctrl_vlan_bh_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, arp_ctrl_vlan_bh_show_help, (void *) file);
}

static ca_status_t __arp_ctrl_update_fwd_behavior_rule(
		ca_uint32_t ing_port, ca_uint32_t local_fwd, ca_uint32_t vlan_cmd,
		ca_uint32_t ing_vtag, ca_int32_t eg_port, ca_uint32_t eg_sub_port,
		ca_vlan_id_t ing_vid, ca_vlan_id_t eg_vid)
{
	int i;
	ca_ni_arp_fwd_bh_t *fwd_bh;

	fwd_bh = arp_ctrl_fwd_bh_table_search(ing_port, ing_vtag, ing_vid);
	if (fwd_bh == NULL) {
		for (i = 0; i < CA_NI_ARP_FWD_BH_MAX; i++) {
			if (!ca_ni_arp_fwd_bh_table[i].valid) {
				fwd_bh = &ca_ni_arp_fwd_bh_table[i];
				fwd_bh->valid = TRUE;
				fwd_bh->ingress_port_id = ing_port;
				fwd_bh->ingress_vtag = ing_vtag;
				fwd_bh->ingress_vid = ing_vid;
				break;
			}
		}
		/* table is full */
		if (i == CA_NI_ARP_FWD_BH_MAX)
			return CA_E_ERROR;
	}

	fwd_bh->local_fwd = local_fwd;
	if (eg_port == -1) {
		ca_uint32_t tmp_port;

		if (arp_ctrl_debug) {
			printk("%s: arp_ctrl_dev_port_num=%d\n", __func__, arp_ctrl_dev_port_num);
		}
		for (i = 0; i < arp_ctrl_dev_port_num; i++) {
			tmp_port = arp_ctrl_dev_ports[i];
			fwd_bh->egress_port[tmp_port].valid = TRUE;
			fwd_bh->egress_port[tmp_port].vlan_cmd = vlan_cmd;
			fwd_bh->egress_port[tmp_port].vid = eg_vid;
			fwd_bh->egress_port[tmp_port].eg_sub_port = eg_sub_port;
		}
	}
	else {
		if (arp_ctrl_debug) {
			printk("%s: eg_port=%d, eg_sub_port=0x%x\n", __func__, eg_port, eg_sub_port);
		}

		fwd_bh->egress_port[eg_port].valid = TRUE;
		fwd_bh->egress_port[eg_port].vlan_cmd = vlan_cmd;
		fwd_bh->egress_port[eg_port].vid = eg_vid;
		fwd_bh->egress_port[eg_port].eg_sub_port = eg_sub_port;
	}

	__arp_ctrl_dump_bh_rule(*fwd_bh);

	return CA_E_OK;
}

static ca_status_t arp_ctrl_update_fwd_behavior_rule(
		ca_uint32_t ing_port, ca_uint32_t local_fwd, ca_uint32_t vlan_cmd,
		ca_int32_t ing_vtag, ca_int32_t eg_port, ca_uint32_t eg_sub_port,
		ca_vlan_id_t ing_vid, ca_vlan_id_t eg_vid)
{
	ca_status_t ret;

	if (ing_vtag == -1) {
		ret = __arp_ctrl_update_fwd_behavior_rule(ing_port, local_fwd, vlan_cmd,
				UNTAG, eg_port, eg_sub_port, ing_vid, eg_vid);
		if (ret != CA_E_OK)
			return ret;
		ret = __arp_ctrl_update_fwd_behavior_rule(ing_port, local_fwd, vlan_cmd,
				TAG, eg_port, eg_sub_port, ing_vid, eg_vid);
	} else {
		ret = __arp_ctrl_update_fwd_behavior_rule(ing_port, local_fwd, vlan_cmd,
				(ca_uint32_t)ing_vtag, eg_port, eg_sub_port, ing_vid, eg_vid);
	}
	return ret;
}

static ca_uint32_t check_port_valid(ca_int32_t port_id)
{
	ca_uint32_t i;

	for (i = 0; i < arp_ctrl_dev_port_num; i++) {
		if (port_id == arp_ctrl_dev_ports[i])
			return TRUE;
	}

	return FALSE;
}

ssize_t arp_ctrl_vlan_bh_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	ca_int32_t value;
	ca_uint32_t i;
	char *token, *cp = tmp;
	const char *param[] = {"ING_PORT", "ING_TAG", "ING_VID", "LOCAL_FWD", "EG_PORT", "EG_SUB_PORT", "EG_CMD", "EG_VID"};

	ca_uint32_t             ing_port, local_fwd, vlan_cmd;
	ca_int32_t              ingress_vtag, eg_port;
	ca_uint32_t		eg_sub_port;
	ca_vlan_id_t            ing_vid, eg_vid;
	ca_status_t  ret;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _arp_ctrl_invalid;
	tmp[len] = '\0';

	for (i = 0; i < ARP_CTRL_VLAN_BH_MAX; i++) {
		token = strsep(&cp, " \n");
		if (kstrtos32(token, 0, &value))
		{
			ca_printf(KERN_ERR "ERROR! Invalid %d value '%s'.\n", i, token);
			return CA_E_PARAM;
		}

		if (arp_ctrl_debug) {
			printk("%s: i=%d, value=0x%x\n", __func__, i, value);
		}
		switch(i) {
			case ARP_CTRL_VLAN_BH_INGRESS_PORT:
				if (!check_port_valid(value)) goto _arp_ctrl_invalid;
				ing_port = (ca_uint32_t)value;
				break;
			case ARP_CTRL_VLAN_BH_INGRESS_TAG:
				if (value != TAG_ANY && value != UNTAG && value != TAG) goto _arp_ctrl_invalid;
				ingress_vtag = value;
				break;
			case ARP_CTRL_VLAN_BH_INGRESS_VID:
				if (ingress_vtag == UNTAG && value != VLAN_ANY) goto _arp_ctrl_invalid;
				if (value == -1 || ingress_vtag == TAG_ANY)
					ing_vid = VLAN_ANY;
				else ing_vid = (ca_uint32_t)value;
				break;
			case ARP_CTRL_VLAN_BH_LOCAL_FWD:
				if (value != 0 && value != 1) goto _arp_ctrl_invalid;
				local_fwd = (ca_uint32_t)value;
				break;
			case ARP_CTRL_VLAN_BH_EGRESS_PORT:
				if (value != -1 && !check_port_valid(value)) goto _arp_ctrl_invalid;
				eg_port = value;
				break;
			case ARP_CTRL_VLAN_BH_EGRESS_SUB_PORT:
				eg_sub_port = value;
				break;
			case ARP_CTRL_VLAN_BH_EGRESS_VLAN_CMD:
				if (value != VLAN_PUSH && value != VLAN_POP && value != VLAN_SWAP && value!= NOTHING) goto _arp_ctrl_invalid;
				vlan_cmd = (ca_uint32_t)value;
				break;
			case ARP_CTRL_VLAN_BH_EGRESS_VID:
				if ((vlan_cmd == VLAN_PUSH || vlan_cmd == VLAN_SWAP) && value == 0) goto _arp_ctrl_invalid;
				if (vlan_cmd == NOTHING || vlan_cmd == VLAN_POP) eg_vid = 0;
				else eg_vid = (ca_uint32_t)value;
				break;
			default: break;
		}
	}

	ret = arp_ctrl_update_fwd_behavior_rule(ing_port, local_fwd, vlan_cmd, ingress_vtag, eg_port, eg_sub_port, ing_vid, eg_vid);
	if (ret == CA_E_OK) return count;

_arp_ctrl_invalid:
	ca_printf(KERN_ERR "ERROR! Invalid value of %s.\n", param[i]);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_arp_ctrl_vlan_bh __read_mostly = {
	.owner          = THIS_MODULE,
	.open           = arp_ctrl_vlan_bh_proc_open,
	.read           = seq_read,
	.write          = arp_ctrl_vlan_bh_proc_write,
	.release        = single_release,
};

/* ***********************************
 *          show information
 * ***********************************/
#define CA_NE_ARP_CTRL_DUMP_HELP_MSG \
	"Purpose: Show information.\n" \
"READ Usage: cat %s\n" \
"WRITE Usage: echo [bitwise flag] > %s\n" \
"  0: Dump all information.\n" \
"  1: Dump interface IP address information.\n" \
"  2: Dump vlan action information.\n" \
"\n"

static void arp_ctrl_dump_bh_rule(void)
{
	ca_uint32_t i;
	for (i = 0; i < CA_NI_ARP_FWD_BH_MAX; i++) {
		if (ca_ni_arp_fwd_bh_table[i].valid) {
			__arp_ctrl_dump_bh_rule(ca_ni_arp_fwd_bh_table[i]);
		}
	}
}

static void arp_ctrl_dump_intf_ip(void)
{
	ca_uint32_t i;
	ca_ip_address_t ip;
	for (i = 0; i < CA_NI_INTF_IPADDR_MAX; i++) {
		if (ca_ni_intf_ipaddr[i].valid) {
			memcpy(&ip, &(ca_ni_intf_ipaddr[i].intf_ip), sizeof(ca_ip_address_t));
			if (ip.afi == CA_IPV4) ca_printf("Addr[%d]=%pI4\n", i, &ip.ip_addr.ipv4_addr);
			else ca_printf("Addr[%d]=%pI6\n", i, &ip.ip_addr.ipv6_addr);
		}
	}
}

static int arp_ctrl_dump_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;
	seq_printf(m, CA_NE_ARP_CTRL_DUMP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	return 0;
}

int arp_ctrl_dump_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, arp_ctrl_dump_show_help, (void *) file);
}

ssize_t arp_ctrl_dump_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	ca_uint32_t value;
	char *token, *cp = tmp;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _arp_ctrl_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (kstrtou32(token, 0, &value))
	{
		ca_printf(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
		goto _arp_ctrl_invalid;
	}

	switch(value) {
		case CA_ARP_CTRL_SHOW_ALL:
			arp_ctrl_dump_intf_ip();
			arp_ctrl_dump_bh_rule();
			break;
		case CA_ARP_CTRL_SHOW_INTF_IPADDR:
			arp_ctrl_dump_intf_ip();
			break;
		case CA_ARP_CTRL_SHOW_ACTION:
			arp_ctrl_dump_bh_rule();
			break;
	}

	return count;

_arp_ctrl_invalid:
	ca_printf(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_arp_ctrl_dump __read_mostly = {
	.owner          = THIS_MODULE,
	.open           = arp_ctrl_dump_proc_open,
	.read           = seq_read,
	.write          = arp_ctrl_dump_proc_write,
	.release        = single_release,
};

/* ***********************************
 *              ARP ctrl debug
 * ***********************************/
#define CA_NE_ARP_CTRL_DEBUG_HELP_MSG \
        "Purpose: Enable/Disable ARP ctrl debug.....\n" \
"READ Usage: cat %s\n" \
"WRITE Usage: echo [enable] > %s\n" \
"  0: Disable.\n" \
"  1: Enable.\n" \
"\n"

static int arp_ctrl_debug_show_help(struct seq_file *m, void *v)
{
        struct file *file = m->private;
        seq_printf(m, CA_NE_ARP_CTRL_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
        seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, arp_ctrl_debug);
        return 0;
}

int arp_ctrl_debug_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, arp_ctrl_debug_show_help, (void *) file);
}

ssize_t arp_ctrl_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
        char tmp[32];
        ssize_t len;
        ca_uint32_t value;
        char *token, *cp = tmp;

        len = min(count, (sizeof(tmp) - 1));
        if (copy_from_user(tmp, buf, len))
                goto _arp_ctrl_debug_invalid;
        tmp[len] = '\0';

        /* parse 1st token */
        token = strsep(&cp, " \n");
        if (kstrtou32(token, 0, &value))
        {
                ca_printf(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
                goto _arp_ctrl_debug_invalid;
        }

        if (value != 0 && value != 1) goto _arp_ctrl_debug_invalid;
        arp_ctrl_debug = value;

        printk("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, arp_ctrl_debug);

        return count;

_arp_ctrl_debug_invalid:
        ca_printf(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
        /* if we return error code here, PROC fs may retry up to 3 times. */
        return count;
}

static const struct file_operations fops_arp_ctrl_debug __read_mostly = {
        .owner          = THIS_MODULE,
        .open           = arp_ctrl_debug_proc_open,
        .read           = seq_read,
        .write          = arp_ctrl_debug_proc_write,
        .release        = single_release,
};

/* ***********************************
 *              ARP ctrl
 * ***********************************/
#define CA_NE_ARP_CTRL_HELP_MSG \
	"Purpose: Enable/Disable ARP ctrl.....\n" \
"READ Usage: cat %s\n" \
"WRITE Usage: echo [enable] > %s\n" \
"  0: Disable.\n" \
"  1: Enable.\n" \
"\n"

static int arp_ctrl_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;
	seq_printf(m, CA_NE_ARP_CTRL_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, arp_ctrl_enable);
	return 0;
}

int arp_ctrl_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, arp_ctrl_show_help, (void *) file);
}

ssize_t arp_ctrl_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	ca_uint32_t value;
	char *token, *cp = tmp;

	len = min(count, (sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _arp_ctrl_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (kstrtou32(token, 0, &value))
	{
		ca_printf(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
		goto _arp_ctrl_invalid;
	}

	if (value != 0 && value != 1) goto _arp_ctrl_invalid;
	arp_ctrl_enable = value;

	printk("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, arp_ctrl_enable);

	return count;

_arp_ctrl_invalid:
	ca_printf(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_arp_ctrl __read_mostly = {
	.owner          = THIS_MODULE,
	.open           = arp_ctrl_proc_open,
	.read           = seq_read,
	.write          = arp_ctrl_proc_write,
	.release        = single_release,
};

int ca_arp_ctrl_proc_init(void)
{
	proc_arp_ctrl = proc_mkdir("arp_ctrl", proc_adapt_dir);

	if (proc_create("intf_ip", S_IRUGO | S_IWUGO, proc_arp_ctrl, &fops_arp_ctrl_intf_ip) == NULL) {
		pr_err("Fail to create proc entry intf_ip.\n");
		return -ENOMEM;
	}

	if (proc_create("vlan_behavior", S_IRUGO | S_IWUGO, proc_arp_ctrl, &fops_arp_ctrl_vlan_bh) == NULL) {
		pr_err("Fail to create proc entry vlan_behavior.\n");
		return -ENOMEM;
	}

	if (proc_create("dump", S_IRUGO | S_IWUGO, proc_arp_ctrl, &fops_arp_ctrl_dump) == NULL) {
		pr_err("Fail to create proc entry dump.\n");
		return -ENOMEM;
	}

	if (proc_create("debug", S_IRUGO | S_IWUGO, proc_arp_ctrl, &fops_arp_ctrl_debug) == NULL) {
		pr_err("Fail to create proc entry debug.\n");
		return -ENOMEM;
	}

	if (proc_create("ctrl", S_IRUGO | S_IWUGO, proc_arp_ctrl, &fops_arp_ctrl) == NULL) {
		pr_err("Fail to create proc entry ctrl.\n");
		return -ENOMEM;
	}

	aal_scfg_read(CFG_ID_PON_MAC_MODE, 1, &pon_mac_mode);

	return 0;
}

ca_status_t ca_arp_ctrl_init(void)
{
	ca_ip_address_t ip_addr;
	arp_ctrl_fwd_wan_port_id_get();

	/* init fwd table */
	memset(ca_ni_arp_fwd_bh_table, 0, sizeof(ca_ni_arp_fwd_bh_table));

	/* init interface IP table */
	memset(ca_ni_intf_ipaddr, 0, sizeof(ca_ni_intf_ipaddr));
	memset(&ip_addr, 0, sizeof(ip_addr));
	ip_addr.afi = CA_IPV4;
	ip_addr.ip_addr.ipv4_addr = htonl(0xC0A80001);
	ip_addr.addr_len = 32;
	ca_rx_set_intf_ipaddr(0, &ip_addr);

	/* get device port */
	memset(arp_ctrl_dev_ports, 0, sizeof(arp_ctrl_dev_ports));
	arp_ctrl_dev_port_num = 0;
	arp_fwd_device_port_get();
	arp_ctrl_enable = TRUE;

	set_special_packet_to_cpu();

	return CA_E_OK;
}

