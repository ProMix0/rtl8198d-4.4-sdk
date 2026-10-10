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
 * ca_ni_cpu_tag.c: handle ports' cpu tag setting
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "ca_ni_cpu_tag.h"

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
#include "ca-scfg/scfg.h"

/* cpu tag port mapping:
	V3xx board
	8367RB			8198F lspid
	port 0			0
	port 1			1
	port 2			2
	port 3			3
	port 4			4
 
	V5xx board, 8226 (98f port 1) is presented
	8367S			8198F lspid		net i/f
	port 0			5				X (do not used)
	port 1			0				eth0.2
	port 2			2				eth0.3
	port 3			3				eth0.4
	port 4			4				eth0.5
	
	V6xx board 
	8367RB			8198F lspid		net i/f
					0				nas0
					1				eth0.2
	port 0			2				eth0.3
	port 1			3				eth0.4
	port 2			4				eth0.5
 */

#define PORT_MAPPING_ARRAY_SIZE		8
static ca_uint8_t external_giga_switch_port_to_lspid[PORT_MAPPING_ARRAY_SIZE]= {0,1,2,3,4,5,6,7};
const static ca_uint8_t external_switch_port_to_lspid_V5xx[PORT_MAPPING_ARRAY_SIZE]= {5,0,2,3,4,5,6,7}; //{0,2,3,4,5,5,6,7};
const static ca_uint8_t external_switch_port_to_lspid_V6xx[PORT_MAPPING_ARRAY_SIZE]= {2,3,4,5,5,5,6,7};

static ca_uint8_t csr_port_id_to_switch_port[PORT_MAPPING_ARRAY_SIZE]= {0,1,2,3,4,5,6,0};
/* register 0xf43042e8 ~
	V5xx:
	csr_port_id			external_switch_port
	6					unused
	5					0
	4					4
	3					3
	2					2
	1					7 (8198F port 1 is 8226, just assign an unused switch port)
	0					1
	--------------------------------------------
	V6xx:
	csr_port_id			external_switch_port
	6					unused
	5					unused
	4					2
	3					1
	2					0
	1					7 (8198F port 1 is 8226, just assign an unused switch port)
	0					7 (8198F port 0 is 8226, just assign an unused switch port)
 */
const static ca_uint8_t csr_port_id_to_switch_port_V5xx[PORT_MAPPING_ARRAY_SIZE]= {1,7,2,3,4,0,6,0}; //{0,7,1,2,3,4,6,0};
const static ca_uint8_t csr_port_id_to_switch_port_V6xx[PORT_MAPPING_ARRAY_SIZE]= {7,7,0,1,2,5,6,0};

#if 0
/* cpu tag port mapping example:
	8367RB/8367S	8198F lspid
	port 0			0
	port 1			4
	port 2			3
	port 3			2
	port 4			1
 */
// method 1 
static ca_uint8_t external_giga_switch_port_to_lspid[8]=
	{0,4,3,2,1,5,6,7};
// method 2
static ca_uint8_t external_giga_switch_port_to_lspid[8]=
	{0,1,2,3,4,5,6,7};

		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_4 = RTK_EXT_SWITCH_PORT_1;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_3 = RTK_EXT_SWITCH_PORT_2;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_2 = RTK_EXT_SWITCH_PORT_3;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_1 = RTK_EXT_SWITCH_PORT_4;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_0 = RTK_EXT_SWITCH_PORT_0;

		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_4 = BIT(RTK_EXT_SWITCH_PORT_1);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_3 = BIT(RTK_EXT_SWITCH_PORT_2);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_2 = BIT(RTK_EXT_SWITCH_PORT_3);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_1 = BIT(RTK_EXT_SWITCH_PORT_4);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_0 = BIT(RTK_EXT_SWITCH_PORT_0);
#endif
#if 0
/* for example:
	8367RB/8367S	8198F lspid
	port 0			0
	port 1			1
	port 2			2
	port 3			3
	port 7 (GMAC2)	4
 */
	ni_hv_glb_cpu_tagx_port_id1.bf.port_id_4 = RTK_EXT_SWITCH_PORT_7;
	...
	reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A9 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
	reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A8 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
	ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x4;
	...
	ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_4 = BIT(RTK_EXT_SWITCH_PORT_7);			
#endif		

ca_uint32 _8198f_board_info;
static ca_uint32 skip_mapping_port_mask=0;
static ca_uint32 cpu_tag_max_ni_port=RTK_NI_PORT_4;

#if defined(CONFIG_RTK_DEV_AP)
/* port_mask ---- 98f physical port mask
 * return value ---- external switch port mask
*/
unsigned int rtl_get_external_switch_portmask(unsigned int port_mask)
{
	int i = 0;
	unsigned int ext_portmsk = port_mask;
	
	for (i = 0; i <= cpu_tag_max_ni_port; i++){
		if ((1<<external_giga_switch_port_to_lspid[i]) == port_mask)
		{
			ext_portmsk = 1<<i;
			printk("%s %d external_giga_switch_port_to_lspid[%d]=0x%x port_mask=0x%x \n", __func__, __LINE__, i, external_giga_switch_port_to_lspid[i], port_mask);
			break;
		}
		
	}
	printk("%s %d ext_portmsk=0x%x port_mask=0x%x \n", __func__, __LINE__, ext_portmsk, port_mask);
	return ext_portmsk;
}
EXPORT_SYMBOL(rtl_get_external_switch_portmask);

unsigned int rtl_get_board_type(void)
{
	return _8198f_board_info;
}
EXPORT_SYMBOL(rtl_get_board_type);
#endif
static ca_status_t aal_l2_tm_dq_to_tm_port_mapping_modify(ca_uint32_t enable)
{
	L2TM_L2TM_BM_DQ_TO_TM_PORT_MAP_t map;
	ca_uint32_t i;

	map.wrd = CA_NI_REG_READ(L2TM_L2TM_BM_DQ_TO_TM_PORT_MAP);

	for (i=0; i<=cpu_tag_max_ni_port; i++) {
		if(enable) {
			if ((1 << i) & skip_mapping_port_mask)
				map.wrd = (map.wrd & ~(0xF << (i*4))) | (i << (i*4));
			else
				map.wrd = (map.wrd & ~(0xF << (i*4))) | (HOST_GMII_PORT << (i*4));
		}
		else {
			map.wrd = (map.wrd & ~(0xF << (i*4))) | (i << (i*4));
		}
	}

#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)&&defined(CONFIG_RTK_8367_RELAY)
	/* for 8198F hardware multicast forwarding issue
	   map L2TM ldpid 0x6 to RGMII pdpid HOST_GMII_PORT
	   		Set L2TM_L2TM_BM_DQ_TO_TM_PORT_MAP(0xf4302120).tm_dpid_sel_6[27:24] = HOST_GMII_PORT	   		
	 */
	/* for tm_dpid_sel_6 */
	if(enable)
		map.wrd = (map.wrd & ~(0xF << (RTK_NI_PORT_6*4))) | (HOST_GMII_PORT << (RTK_NI_PORT_6*4));
	else
		map.wrd = (map.wrd & ~(0xF << (RTK_NI_PORT_6*4))) | (RTK_NI_PORT_6 << (RTK_NI_PORT_6*4));
#endif

	CA_NI_REG_WRITE(map.wrd, L2TM_L2TM_BM_DQ_TO_TM_PORT_MAP);

	return CA_E_OK;
}
#endif
ca_status_t ca_ni_cpu_tag_init(ca_uint32_t enable, ca_uint32_t port, ca_uint32_t profile)
{
	ca_uint32_t reg_addr, reg_addr_h, reg_addr_l, tbl_acc, tbl_data;
	NI_HV_PT_PORT_STATIC_CFG_t ni_hv_pt_port_static_cfg;
	NI_HV_GLB_CPU_TAG_CFG_t ni_hv_glb_cpu_tag_cfg;
	NI_HV_GLB_CPU_TAGx_PORT_ID1_t ni_hv_glb_cpu_tagx_port_id1;
	NI_HV_GLB_CPU_TAGx_PORT_ID0_t ni_hv_glb_cpu_tagx_port_id0;
	NI_HV_GLB_CPU_TAGx_HDR_Ay_H_t ni_hv_glb_cpu_tagx_hdr_ay_h;
	NI_HV_GLB_CPU_TAGx_HDR_Ay_L_t ni_hv_glb_cpu_tagx_hdr_ay_l;
	NI_HV_GLB_CPU_TAGx_PORT_MASK1_t ni_hv_glb_cpu_tagx_port_mask1;
	NI_HV_GLB_CPU_TAGx_PORT_MASK0_t ni_hv_glb_cpu_tagx_port_mask0;
	L2FE_ARB_PDPID_MAP_TBL_DATA_t l2fe_arb_pdpid_map_table_data;
	int my_mac_flg, dbuf_flg, ldpid, pdpid_tbl_idx;

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
	if (aal_scfg_read(CFG_ID_BOARD_INFO_BIT, 4, &_8198f_board_info) == CA_E_OK) {
		if ((_8198f_board_info & 0xF00) == 0x600) {
			memcpy(external_giga_switch_port_to_lspid, external_switch_port_to_lspid_V6xx, PORT_MAPPING_ARRAY_SIZE);
			memcpy(csr_port_id_to_switch_port, csr_port_id_to_switch_port_V6xx, PORT_MAPPING_ARRAY_SIZE);

			skip_mapping_port_mask = ((1<<RTK_NI_PORT_0) | (1<<RTK_NI_PORT_1));
		}
		else if ((_8198f_board_info & 0xF00) == 0x500) {
			memcpy(external_giga_switch_port_to_lspid, external_switch_port_to_lspid_V5xx, PORT_MAPPING_ARRAY_SIZE);
			memcpy(csr_port_id_to_switch_port, csr_port_id_to_switch_port_V5xx, PORT_MAPPING_ARRAY_SIZE);

			skip_mapping_port_mask = (1<<RTK_NI_PORT_1);
			cpu_tag_max_ni_port = RTK_NI_PORT_5;
		}
	}
	
	/* all packets forwarding to 8198F HOST_GMII_PORT to external Giga switch */
	aal_l2_tm_dq_to_tm_port_mapping_modify(enable);
#endif
	if(enable)
	{	
		/* config g3-lite rtk-tag processing func. */
		reg_addr = NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port;
		ni_hv_pt_port_static_cfg.wrd = CA_NE_REG_READ(reg_addr);
		ni_hv_pt_port_static_cfg.bf.cpu_tag_rx_en = enable;
		ni_hv_pt_port_static_cfg.bf.cpu_tag_tx_en = enable;
		ni_hv_pt_port_static_cfg.bf.cpu_tag_cfg_sel = profile;
		CA_NE_REG_WRITE(ni_hv_pt_port_static_cfg.wrd, reg_addr);

		reg_addr = NI_HV_GLB_CPU_TAG_CFG;
		ni_hv_glb_cpu_tag_cfg.wrd = CA_NE_REG_READ(reg_addr);
		ni_hv_glb_cpu_tag_cfg.bf.rtk_tag = RTK_TAG_ETH_TYPE;
		ni_hv_glb_cpu_tag_cfg.bf.protocol = RTK_TAG_PROTO;
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tag_cfg.wrd, reg_addr);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_PORT_ID1 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_PORT_ID0 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_port_id1, 0, sizeof(ni_hv_glb_cpu_tagx_port_id1));
		memset(&ni_hv_glb_cpu_tagx_port_id0, 0, sizeof(ni_hv_glb_cpu_tagx_port_id0));

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_6 = csr_port_id_to_switch_port[6];
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_5 = csr_port_id_to_switch_port[5];
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_4 = csr_port_id_to_switch_port[4];
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_3 = csr_port_id_to_switch_port[3];
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_2 = csr_port_id_to_switch_port[2];
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_1 = csr_port_id_to_switch_port[1];
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_0 = csr_port_id_to_switch_port[0];
#else
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_6 = RTK_EXT_SWITCH_PORT_6;
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_5 = RTK_EXT_SWITCH_PORT_5;
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_4 = RTK_EXT_SWITCH_PORT_4;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_3 = RTK_EXT_SWITCH_PORT_3;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_2 = RTK_EXT_SWITCH_PORT_2;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_1 = RTK_EXT_SWITCH_PORT_1;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_0 = RTK_EXT_SWITCH_PORT_0;
#endif
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_id1.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_id0.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A15 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A14 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3777;//currently useless, just for test reg. config
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_7;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_7;
#else
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x7;//currently useless, just for test reg. config
#endif
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x7;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A13 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A12 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3666;//currently useless, just for test reg. config
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_6;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_6;
#else
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x6;//currently useless, just for test reg. config
#endif
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x6;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A11 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A10 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3555;//currently useless, just for test reg. config
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_5;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_5;
#else
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x5;//currently useless, just for test reg. config
#endif
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x5;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A9 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A8 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3444;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_4;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x4;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A7 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A6 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x3;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x3;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x3;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3333;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_3;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_3;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x3;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A5 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A4 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x2;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x2;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x2;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3222;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_2;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_2;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x2;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A3 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A2 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x1;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x1;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x1;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3111;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_1;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_1;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x1;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A1 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A0 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x0;
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x0;
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x0;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3000;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_0;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_0;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x0;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		/* for all pkt w/o rtk-tag, we config their lspid to be Port 4 */
		reg_addr_h = NI_HV_PT_CPU_TAG_DEFAULT_HEADER_CFG1 + APB0_NI_HV_PT_STRIDE * port;
		reg_addr_l = NI_HV_PT_CPU_TAG_DEFAULT_HEADER_CFG0 + APB0_NI_HV_PT_STRIDE * port;
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset((void *)&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3444;//currently useless, just for test reg. config
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = port;
#else
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_4;
#endif
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x4;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		/* config for 98F Tx direction */
		reg_addr_h = NI_HV_GLB_CPU_TAG0_PORT_MASK1 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_PORT_MASK0 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset((void *)&ni_hv_glb_cpu_tagx_port_mask1, 0, sizeof(ni_hv_glb_cpu_tagx_port_mask1));
		memset((void *)&ni_hv_glb_cpu_tagx_port_mask0, 0, sizeof(ni_hv_glb_cpu_tagx_port_mask0));
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)&&defined(CONFIG_RTK_8367_RELAY)
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_6 = 0;
#else
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_6 = BIT(csr_port_id_to_switch_port[6]);
#endif
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_5 = BIT(csr_port_id_to_switch_port[5]);
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_4 = BIT(csr_port_id_to_switch_port[4]);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_3 = BIT(csr_port_id_to_switch_port[3]);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_2 = BIT(csr_port_id_to_switch_port[2]);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_1 = BIT(csr_port_id_to_switch_port[1]);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_0 = BIT(csr_port_id_to_switch_port[0]);
#else
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_6 = BIT(RTK_EXT_SWITCH_PORT_6);
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_5 = BIT(RTK_EXT_SWITCH_PORT_5);
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_4 = BIT(RTK_EXT_SWITCH_PORT_4);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_3 = BIT(RTK_EXT_SWITCH_PORT_3);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_2 = BIT(RTK_EXT_SWITCH_PORT_2);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_1 = BIT(RTK_EXT_SWITCH_PORT_1);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_0 = BIT(RTK_EXT_SWITCH_PORT_0);
#endif
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_mask1.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_mask0.wrd, reg_addr_l);

		tbl_acc = L2FE_ARB_PDPID_MAP_TBL_ACCESS;
		tbl_data = L2FE_ARB_PDPID_MAP_TBL_DATA;

		for(my_mac_flg = 0; my_mac_flg <= 1; my_mac_flg++){
			for(dbuf_flg = 0; dbuf_flg <= 1; dbuf_flg++){
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
				for(ldpid = RTK_NI_PORT_0; ldpid <= cpu_tag_max_ni_port; ldpid++){
					/* prepare data */
					if ((1 << ldpid) & skip_mapping_port_mask)
						l2fe_arb_pdpid_map_table_data.bf.pdpid = (ca_uint32)ldpid;
					else
#else
				for(ldpid = RTK_NI_PORT_0; ldpid <= RTK_NI_PORT_4; ldpid++){
					/* prepare data */
#endif
						l2fe_arb_pdpid_map_table_data.bf.pdpid = (ca_uint32)port;
					CA_NE_REG_WRITE(l2fe_arb_pdpid_map_table_data.wrd, tbl_data);

					/* do indirect access */
					pdpid_tbl_idx = ( ((ca_uint32)my_mac_flg << 7) | ((ca_uint32)dbuf_flg << 6) | ((ca_uint32)ldpid << 0) );
					WRITE_INDIRCT_REG(0, pdpid_tbl_idx, tbl_acc);
				}

#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)&&defined(CONFIG_RTK_8367_RELAY)

				l2fe_arb_pdpid_map_table_data.bf.pdpid = (ca_uint32)HOST_GMII_PORT;
				CA_NE_REG_WRITE(l2fe_arb_pdpid_map_table_data.wrd, tbl_data);

				/* do indirect access */
				pdpid_tbl_idx = ( ((ca_uint32)my_mac_flg << 7) | ((ca_uint32)dbuf_flg << 6) | ((ca_uint32)RTK_NI_PORT_6 << 0) );
				WRITE_INDIRCT_REG(0, pdpid_tbl_idx, tbl_acc);
#endif
			}
		}
	} else {
		//todo: disable rtk-tag processing
		/* config g3-lite rtk-tag processing func. */
		reg_addr = NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port;
		ni_hv_pt_port_static_cfg.wrd = CA_NE_REG_READ(reg_addr);
		ni_hv_pt_port_static_cfg.bf.cpu_tag_rx_en = enable;
		ni_hv_pt_port_static_cfg.bf.cpu_tag_tx_en = enable;
		ni_hv_pt_port_static_cfg.bf.cpu_tag_cfg_sel = profile;
		CA_NE_REG_WRITE(ni_hv_pt_port_static_cfg.wrd, reg_addr);
	}

	return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
ca_status_t ca_ni_short_ipg_init(ca_uint32_t port, ca_uint32_t ipg_bytes)
{
	NI_HV_PT_TXMAC_CFG_t txmac_cfg;

	if ((ipg_bytes < 8) || (ipg_bytes > 15))
		return CA_E_PARAM;
	
	txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port));
	txmac_cfg.bf.tx_ipg_sel = ipg_bytes - 8;
	CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port));

	return CA_E_OK;
}

#endif