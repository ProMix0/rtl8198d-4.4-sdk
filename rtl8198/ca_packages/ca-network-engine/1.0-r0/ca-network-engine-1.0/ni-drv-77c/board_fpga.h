#ifndef __BOARD_FPGA_H__
#define __BOARD_FPGA_H__

#include "ca_ni.h"

u8 ca_ni_eth_mac[CA_NI_MAX_INTF][6] = {
	{0x00, 0x13, 0x25, 0x00, 0x00, 0x01},
	{0x00, 0x13, 0x25, 0x00, 0x00, 0x02} };

ca_port_cfg_info_t port_cfg_infos[CA_NI_MAX_INTF];

#endif /* __BOARD_FPGA_H__ */
