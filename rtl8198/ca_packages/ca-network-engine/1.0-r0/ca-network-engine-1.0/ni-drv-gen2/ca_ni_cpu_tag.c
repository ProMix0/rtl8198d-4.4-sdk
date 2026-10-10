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
 * ca_ni_rx.c: Cortina NI Rx handler
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "ca_ni_cpu_tag.h"

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
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_6 = RTK_EXT_SWITCH_PORT_6;
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_5 = RTK_EXT_SWITCH_PORT_5;
		ni_hv_glb_cpu_tagx_port_id1.bf.port_id_4 = RTK_EXT_SWITCH_PORT_4;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_3 = RTK_EXT_SWITCH_PORT_3;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_2 = RTK_EXT_SWITCH_PORT_2;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_1 = RTK_EXT_SWITCH_PORT_1;
		ni_hv_glb_cpu_tagx_port_id0.bf.port_id_0 = RTK_EXT_SWITCH_PORT_0;
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_id1.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_id0.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A15 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A14 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3777;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x7;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x7;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A13 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A12 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3666;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x6;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x6;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A11 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A10 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3555;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = 0x5;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x5;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		reg_addr_h = NI_HV_GLB_CPU_TAG0_HDR_A9 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_HDR_A8 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
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
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
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
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
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
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
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
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
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
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_h, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_h));
		memset(&ni_hv_glb_cpu_tagx_hdr_ay_l, 0, sizeof(ni_hv_glb_cpu_tagx_hdr_ay_l));
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.pol_grp_id = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.drop_code = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_h.bf.mcgid = 0x4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.pkt_size = 0x3444;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.lspid = RTK_NI_PORT_4;
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.ldpid = RTK_NI_PORT_4;//currently useless, just for test reg. config
		ni_hv_glb_cpu_tagx_hdr_ay_l.bf.cos = 0x4;//currently useless, just for test reg. config
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_h.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_hdr_ay_l.wrd, reg_addr_l);

		/* config for 98F Tx direction */
		reg_addr_h = NI_HV_GLB_CPU_TAG0_PORT_MASK1 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		reg_addr_l = NI_HV_GLB_CPU_TAG0_PORT_MASK0 + RTK_TAG_APB0_NI_HV_PT_STRIDE * profile;
		memset(&ni_hv_glb_cpu_tagx_port_mask1, 0, sizeof(ni_hv_glb_cpu_tagx_port_mask1));
		memset(&ni_hv_glb_cpu_tagx_port_mask0, 0, sizeof(ni_hv_glb_cpu_tagx_port_mask0));
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_6 = BIT(RTK_EXT_SWITCH_PORT_6);
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_5 = BIT(RTK_EXT_SWITCH_PORT_5);
		ni_hv_glb_cpu_tagx_port_mask1.bf.port_mask_4 = BIT(RTK_EXT_SWITCH_PORT_4);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_3 = BIT(RTK_EXT_SWITCH_PORT_3);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_2 = BIT(RTK_EXT_SWITCH_PORT_2);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_1 = BIT(RTK_EXT_SWITCH_PORT_1);
		ni_hv_glb_cpu_tagx_port_mask0.bf.port_mask_0 = BIT(RTK_EXT_SWITCH_PORT_0);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_mask1.wrd, reg_addr_h);
		CA_NE_REG_WRITE(ni_hv_glb_cpu_tagx_port_mask0.wrd, reg_addr_l);

		tbl_acc = L2FE_ARB_PDPID_MAP_TBL_ACCESS;
		tbl_data = L2FE_ARB_PDPID_MAP_TBL_DATA;

		for(my_mac_flg = 0; my_mac_flg <= 1; my_mac_flg++){
			for(dbuf_flg = 0; dbuf_flg <= 1; dbuf_flg++){
				for(ldpid = RTK_NI_PORT_0; ldpid <= RTK_NI_PORT_4; ldpid++){
					/* prepare data */
					l2fe_arb_pdpid_map_table_data.bf.pdpid = (ca_uint32)port;
					CA_NE_REG_WRITE(l2fe_arb_pdpid_map_table_data.wrd, tbl_data);

					/* do indirect access */
					pdpid_tbl_idx = ( ((ca_uint32)my_mac_flg << 7) | ((ca_uint32)dbuf_flg << 6) | ((ca_uint32)ldpid << 0) );
					WRITE_INDIRCT_REG(0, pdpid_tbl_idx, tbl_acc);
				}
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

