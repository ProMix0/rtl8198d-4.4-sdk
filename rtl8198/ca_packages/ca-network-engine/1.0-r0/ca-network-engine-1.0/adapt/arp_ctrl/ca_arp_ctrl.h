#ifndef __ARP_CTRL_H__
#define __ARP_CTRL_H__
#include <soc/cortina/ca_types.h>
#include <linux/netdevice.h>
int ca_arp_ctrl_proc_init(void);
ca_status_t ca_arp_ctrl_init(void);

ca_status_t ca_rx_set_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_ip_address_t *ip_addr);

ca_status_t ca_rx_get_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_uint32_t     index,
		ca_ip_address_t *ip_addr);

ca_status_t ca_rx_check_intf_ipaddr(
		ca_device_id_t  device_id,
		ca_ip_address_t *ip_addr);

ca_uint32_t ca_arp_ctrl_check_local_fwd(
		ca_uint32_t port_id,
		ca_uint16_t vid);

void ca_arp_ctrl_check_vlan_action(
		ca_uint32_t lspid,
		ca_uint16_t vid,
		ca_uint32_t ldpid,
		ca_uint8_t *pkt_data,
		ca_uint32_t *pkt_len,
		ca_uint32_t *eg_sub_port);
#endif
