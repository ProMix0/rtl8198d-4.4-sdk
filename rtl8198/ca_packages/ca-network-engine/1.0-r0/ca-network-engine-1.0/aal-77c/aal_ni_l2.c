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
 * aal_ni_l2.c: Hardware Abstraction Layer for NI -> L2 FE driver
 *
 */
/*
 NI.GLB
 NI.PT
 NI.INTPT
 NI.XRAM
 NI.MCE
 NI.PTP
 NI.XAUI.XAUI_MAC.ctrl
 NI.XAUI.XAUI_MAC.txmacl
 NI.XAUI.XAUI_MAC.rxmac
 NI.XAUI.XAUI_xgrs.tx
 NI.XAUI.XAUI_xgrs.rx
 NI.XGE.MAC
 NI.XGE.MISC
*/
#include "osal_common.h"
#include "aal_common.h"
#include "aal_table.h"
#include "aal_ni_l2.h"
#include "aal_ni_cputag.h"
#include "aal_ni.h"
#include "ca-scfg/scfg.h"
#include "aal_port.h"
#include "aal_phy.h"
#include "aal_l2_specpkt.h"

#define GPHY_REG_BASE                0xF4338000
#define GPHY_REG_CTRL                0
#define GPHY_MIDO_ADDR(port)         ((port)+1)

#ifndef CONFIG_CORTINA_BOARD_FPGA
#define GPHY_MDIO_READ(phy,reg)      \
    CA_NE_PHY_REG_READ(GPHY_REG_BASE+(((phy)&0x1F)<<7)+(((reg)&0x1F)<<2))
#define GPHY_MDIO_WRITE(phy,reg,data) \
    CA_NE_PHY_REG_WRITE(data,GPHY_REG_BASE+(((phy)&0x1F)<<7)+(((reg)&0x1F)<<2))
#else
#define GPHY_MDIO_READ(phy,reg)        0
#define GPHY_MDIO_WRITE(phy,reg,data)

#endif

ca_uint8_t dpid_mapping[] = { // index is src_port, value is dst_port
	1, // 0 -> 1
	2, // 1 -> 2
	3, // 2-> 3
	0, // 3-> 0
	0  // unused now
};

ca_uint32_t aal_ni_incr_port(ca_uint32_t port)
{
	port++;
	if ( port == AAL_NI_PORT_XGE0 )
		port++;
	return port;
}

#define __FOR_ALL_GE_DO(port)   \
                       for((port)= AAL_NI_GE_PORT_START; (port)<= AAL_NI_GE_PORT_END; port = aal_ni_incr_port(port))

#define __XE_PORT_START 6
#define __XE_PORT_END   6

#define __FOR_ALL_XE_DO(port)   \
                       for((port)=0; (port)<__NI_XGE_NUM; ++(port))

/******************************************************************************/
/*                             Import Variables and Functions                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/
/*
 * NI registers read/write MACROs
 */
#define __NI_PORT_REG_STRIDE                  (APB0_NI_HV_PT_STRIDE)
#define __NI_XGE_REG_STRIDE                   (1)
#define __NI_XSMII_REG_STRIDE                 (1)

#define __NI_TBL_READ_FLAG                    (0)
#define __NI_TBL_WRITE_FLAG                   (1)

#define __NI_GE_NUM            4
#define __NI_XAUI_NUM          1
#define __NI_XGE_NUM           1

#define __NI_RXMUX_FC_THRD_HI  130
#define __NI_RXMUX_FC_THRD_LO  10

#define __NI_PAUSE_QUANTA_DFT  0xFFFF


#define FOR_ALL_AAL_NI_PORT_DO(port) \
        for ((port) = AAL_PORT_NUM_FIRST_ETH; (port) < AAL_NI_PORT_NUM; ++(port))

/*
 * NI ASIC table access MACROs
 */
#define  __NI_TABLE_ACCESS_TIMEOUT 5000
#define  __NI_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg)   do {     \
                ca_uint32_t  timeout = 0;                           \
                do {                                                \
                    (acc_reg).wrd = CA_DEV_NI_REG_READ(device_id,tbl_name);       \
                    if (timeout++ > __NI_TABLE_ACCESS_TIMEOUT)      \
                        return CA_E_TIMEOUT;                        \
                }while(0 != (acc_reg).bf.access);                   \
            }while(0)

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __NI_SATURN_FIRST_PORT 3
#define __NI_SATURN_END_PORT 3

#define  __NI_MIB_TABLE_ACCESS(device_id,rw,tbl_name,port_id,mib_id,act,acc_reg)  do { \
                ca_uint32_t  timeout = 0;                           \
                (acc_reg).wrd        = 0;                                           \
                (acc_reg).bf.access  = 1;                                           \
                (acc_reg).bf.rbw     = (rw);                                        \
                (acc_reg).bf.op_code = (act);                                       \
                (acc_reg).bf.port_id    = (port_id);                                \
                (acc_reg).bf.counter_id = (mib_id);                                 \
                if ((device_id)==0)  \
                    CA_NI_REG_WRITE((acc_reg).wrd, tbl_name); \
                else \
                    CA_REG_WRITE((acc_reg).wrd, (uintptr_t)((SATURN_##tbl_name & 0xFFFF) + hgu_8279_ne_reg_iobase)); \
                do {                                                \
                    (acc_reg).wrd = ((device_id)==0) ? CA_NI_REG_READ(tbl_name) : (CA_REG_READ((uintptr_t)((SATURN_##tbl_name & 0xFFFF) + hgu_8279_ne_reg_iobase))); \
                    if (timeout++ > __NI_TABLE_ACCESS_TIMEOUT)      \
                        return CA_E_TIMEOUT;                        \
                }while(0 != (acc_reg).bf.access);                   \
            }while(0)

#define  __NI_MC_TABLE_ACCESS(device_id,rw,tbl_name,address,acc_reg)         do {   \
                ca_uint32_t  timeout = 0;                           \
                (acc_reg).wrd        = 0;                                     \
                (acc_reg).bf.access  = 1;                                     \
                (acc_reg).bf.rbw     = (rw);                                  \
                (acc_reg).bf.addr = (address);                                \
                if ((device_id)==0)  \
                    CA_NI_REG_WRITE((acc_reg).wrd, tbl_name); \
                else \
                    CA_REG_WRITE((acc_reg).wrd, (uintptr_t)((SATURN_##tbl_name & 0xFFFF) + hgu_8279_ne_reg_iobase)); \
                do {                                                \
                    (acc_reg).wrd = ((device_id)==0) ? CA_NI_REG_READ(tbl_name) : (CA_REG_READ((uintptr_t)((SATURN_##tbl_name & 0xFFFF) + hgu_8279_ne_reg_iobase))); \
                    if (timeout++ > __NI_TABLE_ACCESS_TIMEOUT)      \
                        return CA_E_TIMEOUT;                        \
                }while(0 != (acc_reg).bf.access);                   \
            }while(0)

#else
#define  __NI_MIB_TABLE_ACCESS(device_id,rw,tbl_name,port_id,mib_id,act,acc_reg)  do { \
                (acc_reg).wrd        = 0;                                           \
                (acc_reg).bf.access  = 1;                                           \
                (acc_reg).bf.rbw     = (rw);                                        \
                (acc_reg).bf.op_code = (act);                                       \
                (acc_reg).bf.port_id    = (port_id);                                \
                (acc_reg).bf.counter_id = (mib_id);                                 \
                CA_DEV_NI_REG_WRITE2(device_id,tbl_name,(acc_reg).wrd);                      \
                __NI_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg);                    \
            }while(0)

#define  __NI_MC_TABLE_ACCESS(device_id,rw,tbl_name,address,acc_reg)         do {\
                (acc_reg).wrd        = 0;                                     \
                (acc_reg).bf.access  = 1;                                     \
                (acc_reg).bf.rbw     = (rw);                                  \
                (acc_reg).bf.addr = (address);                                \
                CA_DEV_NI_REG_WRITE2(device_id,tbl_name,(acc_reg).wrd);                \
                __NI_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg);              \
            }while(0)
#endif
/*
 * NI resource check MACROs
 */
#define __NI_PORT_CHK(device_id, port)  do {                             \
                               if ((port) >= AAL_NI_PORT_NUM || (port) < AAL_PORT_NUM_FIRST_ETH) {\
                                   ca_printf("port %d is invalid, should be in range (%d ~ %d)\n", port, AAL_PORT_NUM_FIRST_ETH, AAL_NI_PORT_NUM-1); \
                                   return CA_E_PARAM;}\
                           } while(0)

#define __XGE_PORT_CHK(xge)  do {                           \
                               if ((xge) >= __NI_XGE_NUM)   \
                                   return CA_E_PARAM;       \
                           } while(0)


#define __NI_ASSERT(x)      AAL_ASSERT_RET(x, CA_E_PARAM)

/******************************************************************************/
/*                       Local Data-structures                                */
/******************************************************************************/

/* Ethernet MIB operation code enumeration */
typedef enum
{
	__NI_MIB_OP_CODE_READ_ONLY = 0, /* ONLY read MIB counter, not clear */
	__NI_MIB_OP_CODE_READ_CLEAR_MSB = 1, /* read MIB counter and clear MSB */
	__NI_MIB_OP_CODE_READ_CLEAR_ALL = 2  /* read MIB counter and clear all */

} __ni_mib_op_code_t;

/* Ethernet Rx MIB item enumeration */
typedef enum
{
	__NI_MIB_RX_UC_PKT_CNT = 0x0,
	__NI_MIB_RX_MC_FRM_CNT = 0x1,
	__NI_MIB_RX_BC_FRM_CNT = 0x2,
	__NI_MIB_RX_OAM_FRM_CNT = 0x3,
	__NI_MIB_RX_JUMBO_FRM_CNT = 0x4,
	__NI_MIB_RX_PAUSE_FRM_CNT = 0x5,
	__NI_MIB_RX_PFC_FRM_CNT = 0x6,
	__NI_MIB_RX_UNK_OPCODE_FRM_CNT = 0x7,
	__NI_MIB_RX_CRC_ERR_FRM_CNT = 0x8,
	__NI_MIB_RX_UNDER_SIZE_FRM_CNT = 0x9,
	__NI_MIB_RX_RUNT_FRM_CNT = 0xA,
	__NI_MIB_RX_OVSIZE_FRM_CNT = 0xB,
	__NI_MIB_RX_JABBER_FRM_CNT = 0xC,
	__NI_MIB_RX_INVALID_FRM_CNT = 0xD,
	__NI_MIB_RX_INVALID_FLOW_FRM_CNT = 0xE,
	__NI_MIB_RX_STATS_FRM_64_OCT = 0xF,
	__NI_MIB_RX_STATS_FRM_65_TO_127_OCT = 0x10,
	__NI_MIB_RX_STATS_FRM_128_TO_255_OCT = 0x11,
	__NI_MIB_RX_STATS_FRM_256_TO_511_OCT = 0x12,
	__NI_MIB_RX_STATS_FRM_512_TO_1023_OCT = 0x13,
	__NI_MIB_RX_STATS_FRM_1024_TO_1518_OCT = 0x14,
	__NI_MIB_RX_STATS_FRM_1519_TO_2100_OCT = 0x15,
	__NI_MIB_RX_STATS_FRM_2101_TO_9200_OCT = 0x16,
	__NI_MIB_RX_STATS_FRM_9201_TO_MAX_OCT = 0x17,
	__NI_MIB_RX_BYTE_COUNT_LO = 0x18,
	__NI_MIB_RX_BYTE_COUNT_HI = 0x19,

	__NI_MIB_RX_NUM
} __ni_rx_mib_id_t;

/* Ethernet Tx MIB item enumeration */
typedef enum
{
	__NI_MIB_TX_UC_PKT_CNT = 0x0,
	__NI_MIB_TX_MC_FRM_CNT = 0x1,
	__NI_MIB_TX_BC_FRM_CNT = 0x2,
	__NI_MIB_TX_OAM_FRM_CNT = 0x3,
	__NI_MIB_TX_JUMBO_FRM_CNT = 0x4,
	__NI_MIB_TX_PAUSE_FRM_CNT = 0x5,
	__NI_MIB_TX_PFC_FRM_CNT = 0x6,
	__NI_MIB_TX_CRCERR_FRM_CNT = 0x7,
	__NI_MIB_TX_OVSIZE_FRM_CNT = 0x8,
	__NI_MIB_TX_SINGLE_COL_FRM_CNT = 0x9,
	__NI_MIB_TX_MULTI_COL_FRM_CNT = 0xA,
	__NI_MIB_TX_LATE_COL_FRM_CNT = 0xB,
	__NI_MIB_TX_RSVD_12, /* 0xC is reserved */
	__NI_MIB_TX_EXESS_COL_FRM_CNT = 0xD,
	__NI_MIB_TX_STATS_FRM_64_OCT = 0xE,
	__NI_MIB_TX_STATS_FRM_65_TO_127_OCT = 0xF,
	__NI_MIB_TX_STATS_FRM_128_TO_255_OCT = 0x10,
	__NI_MIB_TX_STATS_FRM_256_TO_511_OCT = 0x11,
	__NI_MIB_TX_STATS_FRM_512_TO_1023_OCT = 0x12,
	__NI_MIB_TX_STATS_FRM_1024_TO_1518_OCT = 0x13,
	__NI_MIB_TX_STATS_FRM_1519_TO_2100_OCT = 0x14,
	__NI_MIB_TX_STATS_FRM_2101_TO_9200_OCT = 0x15,
	__NI_MIB_TX_STATS_FRM_9201_TO_MAX_OCT = 0x16,
	__NI_MIB_TX_RSVD_23 = 0x17, /* 0x17 is reserved */
	__NI_MIB_TX_BYTE_CNT_LO = 0x18,
	__NI_MIB_TX_BYTE_CNT_HI = 0x19,

	__NI_MIB_TX_NUM

} __ni_tx_mib_id_t;


/******************************************************************************/
/*                       Local Functions Implementation                       */
/******************************************************************************/

static ca_status_t __ni_mem_init(ca_device_id_t device_id)
{
	NI_HV_GLB_INIT_DONE_t init_done;
	ca_int32_t            wait_loop = 5000;

	do{
		init_done.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INIT_DONE);
		if ( init_done.bf.ni_init_done ) {
			return CA_E_OK;
		}
		udelay(100);
	} while(wait_loop-- > 0);
	ca_printf("%s: init timeout\n",__func__);
	return CA_E_TIMEOUT;

}

static ca_status_t
__ni_mib_init(
	ca_device_id_t device_id)
{
	NI_HV_GLB_STATIC_CFG_t  glb_cfg;

	NI_HV_XGE_MAC_CFG_COM_t cmn_cfg;
	ca_uint32_t             xe      = 0;

	__FOR_ALL_XE_DO(xe) {
		cmn_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_COM + (xe)*__NI_XGE_REG_STRIDE);
		cmn_cfg.bf.clronread = 1;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_CFG_COM + (xe)*__NI_XGE_REG_STRIDE,cmn_cfg.wrd);
	}

	glb_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_STATIC_CFG);
	glb_cfg.bf.cnt_op_mode = 2; /* clear on read */
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_STATIC_CFG,glb_cfg.wrd);


	return CA_E_OK;
}

static ca_status_t __ni_rx_init(ca_device_id_t device_id)
{
	NI_HV_PT_RXMAC_CFG_t    rx_cfg;
	NI_HV_XGE_MAC_CFG_COM_t xge_cfg;
	NI_HV_XGE_MAC_CFG_RX_t  xge_mac_cfg_rx;
	ca_uint32_t             port_id        = 0;


	__FOR_ALL_GE_DO(port_id) {
		rx_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_RXMAC_CFG + port_id * __NI_PORT_REG_STRIDE);
		rx_cfg.bf.rx_en = 0;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_RXMAC_CFG + port_id * __NI_PORT_REG_STRIDE,rx_cfg.wrd);
	}
	__FOR_ALL_XE_DO(port_id) {
		xge_cfg.wrd = CA_NI_REG_READ(NI_HV_XGE_MAC_CFG_COM + port_id * __NI_XGE_REG_STRIDE);
		xge_cfg.bf.rx_rst = 1;
		CA_NI_REG_WRITE(xge_cfg.wrd,NI_HV_XGE_MAC_CFG_COM + port_id * __NI_XGE_REG_STRIDE);
	}

	return CA_E_OK;

}

static ca_status_t __ni_eee_init(ca_device_id_t device_id)
{
	NI_HV_PT_EEE_LPI_CFG0_t eee_cfg0;
	ca_uint8_t              port     = 0;

	eee_cfg0.wrd = CA_NI_REG_READ(NI_HV_PT_EEE_LPI_CFG0);
	eee_cfg0.bf.cpu_lpi_deassert = 1;
	eee_cfg0.bf.tx_eee_enable = 0;

	FOR_ALL_AAL_NI_PORT_DO(port) {
		CA_NI_REG_WRITE(eee_cfg0.wrd,NI_HV_PT_EEE_LPI_CFG0 + port * __NI_PORT_REG_STRIDE);
	}

	return CA_E_OK;

}

static ca_status_t __ni_tx_init(ca_device_id_t device_id)
{
	NI_HV_GLB_NITX_MISC_CFG_t nitx_misc;

	nitx_misc.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_NITX_MISC_CFG);

/*
 * This time base signal is used in port 0~4 Energy-Efficient Ethernet function.
 * When core_clk is 333Mhz, set it to 33; 3ns x 33 = 99 ns
 * When core_clk is 300Mhz, set it to 30; 3.3ns x 30 = 100 ns
 * When core_clk is 250Mhz, set it to 25, 4ns x 25 = 100 ns
 * When core_clk is 200Mhz, set it to 20; 5ns x 20 = 100 ns
 */
#ifdef CONFIG_CORTINA_BOARD_FPGA
	nitx_misc.bf.time_base_cnt = 30;
#else /* ASIC board */
	nitx_misc.bf.time_base_cnt = 33;
#endif /* CONFIG_CORTINA_BOARD_FPGA */

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_NITX_MISC_CFG,nitx_misc.wrd);

	return CA_E_OK;

}

static void __ni_rxmux_monitor_select(
	ca_device_id_t device_id,
	ca_uint8_t aal_port)
{
	NI_HV_GLB_RXMUX_CTRL_CFG_t rxmux;

	rxmux.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_RXMUX_CTRL_CFG);
	rxmux.bf.dbg_sel = aal_port;
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_RXMUX_CTRL_CFG,rxmux.wrd);

}

static ca_status_t __ni_rxmux_init(ca_device_id_t device_id)
{
	aal_ni_port_rx_ctrl_mask_t ni_port_rx_ctrl_mask;
	aal_ni_port_rx_ctrl_t      ni_port_rx_ctrl;
	aal_ni_xge_cfg_rx_mask_t   xge_rx_mask;
	aal_ni_xge_cfg_rx_t        xge_rx;
	ca_uint32_t                port                 = 0;
	ca_status_t                ret                  = CA_E_OK;


	/* initialize GE/FE Rx MAC */
	ni_port_rx_ctrl_mask.u32 = 0;
	memset(&ni_port_rx_ctrl,0,sizeof(aal_ni_port_rx_ctrl_t));

	ni_port_rx_ctrl_mask.s.oam_drop_dis = 1;
	ni_port_rx_ctrl_mask.s.ukopcode_drop_dis = 1;

	ni_port_rx_ctrl.oam_drop_dis = 1; /* Not drop OAM by default in NI, should do in the FE */
	ni_port_rx_ctrl.ukopcode_drop_dis = 1;

	FOR_ALL_AAL_NI_PORT_DO(port) {
		ret = aal_ni_port_rx_ctrl_set(device_id,port,
			ni_port_rx_ctrl_mask,&ni_port_rx_ctrl);
		if ( CA_E_OK != ret ) {
			ca_printf("aal_ni_port_rx_ctrl_set %u return 0x%x\n",port,ret);
		}
	}

	/* initialize XGE Rx MAC */
	xge_rx_mask.u32 = 0;
	memset(&xge_rx,0,sizeof(xge_rx));
	xge_rx_mask.s.drop_short = 1;
	xge_rx_mask.s.fcs_chk_en = 1;
	xge_rx.drop_short = 1;
	xge_rx.fcs_chk_en = 1;
	__FOR_ALL_XE_DO(port) {
		ret = aal_ni_xge_rx_config_set(device_id,port,
			xge_rx_mask,&xge_rx);
		if ( CA_E_OK != ret ) {
			ca_printf("aal_ni_xge_rx_config_set %u return 0x%x\n",port,ret);
		}
	}

	/* enable the monitor WAN port incoming frames counter */
	__ni_rxmux_monitor_select(device_id,AAL_LPORT_ETH_WAN);

	return CA_E_OK;

}

static ca_status_t __ni_intr_init(ca_device_id_t device_id)
{
	aal_ni_port_intr_mask_t intr_mask;
	aal_ni_port_intr_t      intr_cfg;
	ca_uint32_t             port_id   = 0;
	ca_status_t             ret       = CA_E_OK;

	intr_mask.u32 = ~0U;

	FOR_ALL_AAL_NI_PORT_DO(port_id) {
		memset(&intr_cfg,0,sizeof(intr_cfg));
		ret = aal_ni_port_intr_enable_set(device_id,port_id,intr_mask,&intr_cfg);
		if ( CA_E_OK != ret )
			return CA_E_ERROR;

		memset(&intr_cfg,0xFF,sizeof(intr_cfg));
		(void)aal_ni_port_intr_status_clear(device_id,port_id,&intr_cfg);
	}

	return CA_E_OK;

}

static ca_status_t __ni_mc_init(ca_device_id_t device_id)
{
	ca_uint32_t tmp32 = 0;

	tmp32 = CA_DEV_NI_REG_READ(device_id,NI_HV_MCE_ERROR_STS); /* clear stats */

	for(tmp32 = 0; tmp32 < AAL_NI_MC_GROUP_NUM; ++tmp32){
		(void)aal_ni_mc_lkup_set(device_id,tmp32,0);
	}

	return CA_E_OK;
}

static ca_status_t
__ni_pkt_len_init(
	ca_device_id_t device_id)
{
	ca_status_t                  ret      = CA_E_OK;
	aal_ni_pkt_len_config_mask_t len_mask;
	aal_ni_pkt_len_config_t      len_cfg;

	len_mask.u32 = 0;
	len_mask.s.max_pkt_size = 1;
	len_mask.s.min_pkt_size = 1;
	len_mask.s.max_store_pkt_len = 1;

	memset(&len_cfg,0,sizeof(len_cfg));

	len_cfg.max_store_pkt_len = AAL_NI_PKT_LEN_MAX;
	len_cfg.max_pkt_size = AAL_NI_PKT_LEN_MAX;
	len_cfg.min_pkt_size = AAL_NI_PKT_LEN_MIN;

	ret = aal_ni_pkt_len_set(device_id,len_mask,&len_cfg);
	if ( CA_E_OK != ret ) {
		return ret;
	}

	return ret;

}


ca_status_t __ni_flow_ctrl_init(CA_IN ca_device_id_t device_id)
{
	ca_status_t                    ret                 = CA_E_OK;
	ca_uint32_t                    xge                 = 0;
	ca_uint32_t                    ge                  = 0;
	ca_uint8_t                     cos                 = 0;
	ca_uint8_t                     tx_ipg_sel          = 0;
	ca_uint32_t                    port                = 0;
	aal_ni_xaui_mac_tx_ctrl_mask_t xaui_tx_ctrl_mask;
	aal_ni_xaui_mac_tx_ctrl_t      xaui_tx_ctrl;
	aal_ni_xge_flowctrl_mask_t     xge_fc_mask;
	aal_ni_xge_flowctrl_t          xge_fc_config;
	aal_ni_xge_cfg_rx_mask_t       xge_rx_mask;
	aal_ni_xge_cfg_rx_t            xge_rx_cfg;
	aal_ni_xge_cfg_tx_mask_t       xge_tx_mask;
	aal_ni_xge_cfg_tx_t            xge_tx_cfg;
	aal_ni_eth_mac_config_mask_t   ge_mac_mask;
	aal_ni_eth_mac_config_t        ge_mac_cfg;
	L2TM_L2TE_FC_GLB_TIMER_t       fc_glb_timer;
	TE_FC_GLB_TIMER_t              te_fc_timer;
	aal_ni_rxmux_fc_thrshld_mask_t rx_mux_fc_thrd_mask;
	aal_ni_rxmux_fc_thrshld_t      rx_mux_fc_thrd;

	ret = aal_scfg_read(CFG_ID_NI_GE_TX_IPG_SEL,1,&tx_ipg_sel);
	if ( CA_E_OK != ret ) {
		tx_ipg_sel = 2;
	}
	//printk("tx_ipg_sel = %d\n",tx_ipg_sel);

	ge_mac_mask.u32 = 0;
	ge_mac_mask.s.rx_pfc_disable = 1;
	ge_mac_mask.s.tx_pfc_disable = 1;
	ge_mac_mask.s.rx_pause_en = 1;
	ge_mac_mask.s.tx_pause_en = 1;
	ge_mac_mask.s.tx_auto_xon = 1;
	ge_mac_mask.s.tx_pause_sel = 1;
	ge_mac_mask.s.tx_ipg_sel = 1;
	memset(&ge_mac_cfg,0,sizeof(ge_mac_cfg));

	ge_mac_cfg.rx_pfc_disable = 1;
	ge_mac_cfg.tx_pfc_disable = 1;
	ge_mac_cfg.rx_pause_en = 0;
	ge_mac_cfg.tx_pause_en = 0;
	ge_mac_cfg.tx_auto_xon = 1;
	ge_mac_cfg.tx_pause_sel = AAL_NI_TX_PAUSE_SEL_TE_FC_THRED;
	ge_mac_cfg.tx_ipg_sel = tx_ipg_sel;

	fc_glb_timer.wrd = CA_DEV_NI_REG_READ(device_id,L2TM_L2TE_FC_GLB_TIMER);
	fc_glb_timer.bf.enable = 1; /* Enable the TE FC timer by default */
	CA_DEV_NI_REG_WRITE2(device_id,L2TM_L2TE_FC_GLB_TIMER,fc_glb_timer.wrd);

	/* GE FlowControl init */
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if ( device_id == CA_DEV_WAN ) {
		for((ge) = 3; (ge) <= 4; ++(ge)){
			ret = aal_ni_eth_port_mac_set(device_id,ge,ge_mac_mask,&ge_mac_cfg);
			if ( CA_E_OK != ret )
				return ret;
		}

		/* XGE FlowControl init */
		xge_fc_mask.u32 = 0;
		xge_fc_mask.s.pau_tx_en = 1;
		xge_fc_mask.s.pau_rx_en = 1;

		memset(&xge_fc_config,0,sizeof(xge_fc_config));
		xge_fc_config.pau_tx_en = 0;
		xge_fc_config.pau_rx_en = 0;

		xge_rx_mask.u32 = 0;
		xge_rx_mask.s.pause_extract = 1;
		xge_rx_mask.s.pause_term = 1;

		memset(&xge_rx_cfg,0,sizeof(xge_rx_cfg));
		xge_rx_cfg.pause_extract = 1;
		xge_rx_cfg.pause_term = 1;

		xge_tx_mask.u32 = 0;
		xge_tx_mask.s.pause_req_auto_xon = 1;
		xge_tx_mask.s.pause_req_en = 1;
		xge_tx_mask.s.ifg_mode_tx = 1;

		memset(&xge_tx_cfg,0,sizeof(xge_tx_cfg));
		xge_tx_cfg.pause_req_auto_xon = 1;
		xge_tx_cfg.pause_req_en = 1;
		xge_tx_cfg.ifg_mode_tx = 1;

		for((xge) = 0; (xge) <= 1; ++(xge)){
			ret = aal_ni_xge_pause_quanta_set(device_id,xge,__NI_PAUSE_QUANTA_DFT);

			//if (CA_E_OK == ret)
			//    ret = aal_ni_xge_flowctrl_set(device_id, xge, xge_fc_mask, &xge_fc_config);

			if ( CA_E_OK == ret )
				ret = aal_ni_xge_rx_config_set(device_id,xge,xge_rx_mask,&xge_rx_cfg);

			if ( CA_E_OK == ret )
				ret = aal_ni_xge_tx_config_set(device_id,xge,xge_tx_mask,&xge_tx_cfg);

			if ( CA_E_OK != ret )
				return ret;
		}

		rx_mux_fc_thrd_mask.u32 = 0;
		rx_mux_fc_thrd_mask.s.hi_threshold = 1;
		rx_mux_fc_thrd_mask.s.low_threshold = 1;
		memset(&rx_mux_fc_thrd,0,sizeof(aal_ni_rxmux_fc_thrshld_t));
		rx_mux_fc_thrd.hi_threshold = __NI_RXMUX_FC_THRD_HI;
		rx_mux_fc_thrd.low_threshold = __NI_RXMUX_FC_THRD_LO;
		for(port = 3; port <= 4; port++){
			//ret = aal_ni_rxmux_fc_thrshld_set(device_id,port,rx_mux_fc_thrd_mask,&rx_mux_fc_thrd);
			if ( CA_E_OK != ret ) {
				return ret;
			}
		}

		return ret;
	}

#endif

	AAL_FOR_ALL_GE_DO(ge) {
		ret = aal_ni_eth_port_mac_set(device_id,ge,ge_mac_mask,&ge_mac_cfg);
		if ( CA_E_OK != ret )
			return ret;
		for(cos = 0; cos < 8; cos++){
			(void)aal_ni_eth_pfc_quanta_set(device_id,ge,cos,__NI_PAUSE_QUANTA_DFT);
		}
	}

	/* XGE FlowControl init */
	xge_fc_mask.u32 = 0;
	xge_fc_mask.s.pau_tx_en = 1;
	xge_fc_mask.s.pau_rx_en = 1;

	memset(&xge_fc_config,0,sizeof(xge_fc_config));
	xge_fc_config.pau_tx_en = 0;
	xge_fc_config.pau_rx_en = 0;

	xge_rx_mask.u32 = 0;
	xge_rx_mask.s.pause_extract = 1;
	xge_rx_mask.s.pause_term = 1;

	memset(&xge_rx_cfg,0,sizeof(xge_rx_cfg));
	xge_rx_cfg.pause_extract = 1;
	xge_rx_cfg.pause_term = 1;

	xge_tx_mask.u32 = 0;
	xge_tx_mask.s.pause_req_auto_xon = 1;
	xge_tx_mask.s.pause_req_en = 1;
	xge_tx_mask.s.ifg_mode_tx = 1;

	memset(&xge_tx_cfg,0,sizeof(xge_tx_cfg));
	xge_tx_cfg.pause_req_auto_xon = 1;
	xge_tx_cfg.pause_req_en = 1;
	xge_tx_cfg.ifg_mode_tx = 1;

	__FOR_ALL_XE_DO(xge) {
		ret = aal_ni_xge_pause_quanta_set(device_id,xge,__NI_PAUSE_QUANTA_DFT);

		if ( CA_E_OK == ret )
			ret = aal_ni_xge_flowctrl_set(device_id,xge,xge_fc_mask,&xge_fc_config);

		if ( CA_E_OK == ret )
			ret = aal_ni_xge_rx_config_set(device_id,xge,xge_rx_mask,&xge_rx_cfg);

		if ( CA_E_OK == ret )
			ret = aal_ni_xge_tx_config_set(device_id,xge,xge_tx_mask,&xge_tx_cfg);

		if ( CA_E_OK != ret )
			return ret;
	}

	xaui_tx_ctrl_mask.u32 = 0;
	memset(&xaui_tx_ctrl,0,sizeof(aal_ni_xaui_mac_tx_ctrl_t));

	xaui_tx_ctrl_mask.s.rx_pausereq_sel = 1;
	xaui_tx_ctrl.rx_pausereq_sel = 1;

	ret = aal_ni_xaui_mac_tx_ctrl_set(device_id,xaui_tx_ctrl_mask,&xaui_tx_ctrl);
	if ( CA_E_OK != ret )
		return ret;

	rx_mux_fc_thrd_mask.u32 = 0;
	rx_mux_fc_thrd_mask.s.hi_threshold = 1;
	rx_mux_fc_thrd_mask.s.low_threshold = 1;
	memset(&rx_mux_fc_thrd,0,sizeof(aal_ni_rxmux_fc_thrshld_t));
	rx_mux_fc_thrd.hi_threshold = __NI_RXMUX_FC_THRD_HI;
	rx_mux_fc_thrd.low_threshold = __NI_RXMUX_FC_THRD_LO;
	for(port = 0; port < 4; port++){
		ret = aal_ni_rxmux_fc_thrshld_set(device_id,port,rx_mux_fc_thrd_mask,&rx_mux_fc_thrd);
		if ( CA_E_OK != ret ) {
			return ret;
		}
	}
	return ret;

}

ca_status_t aal_ni_eth_capability_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_eth_capability_t *eth_cap)
{
	__NI_ASSERT(eth_cap);

#ifdef CONFIG_CORTINA_BOARD_FPGA
	eth_cap->ge_bmp = 0x0f;
	eth_cap->xe_bmp = 0x00;
#else
	eth_cap->ge_bmp = 0x0f;
	eth_cap->xe_bmp = 0x60;
#endif/*CONFIG_CORTINA_BOARD_FPGA*/

	return CA_E_OK;
}

ca_status_t aal_ni_rxmux_monitor_stats_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *counter)
{
	__NI_ASSERT(counter);

	*counter = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_RXMUX_PKTCNT);

	return CA_E_OK;
}

ca_status_t aal_ni_pkt_len_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_pkt_len_config_mask_t mask,
	CA_IN aal_ni_pkt_len_config_t *config
	)
{
	NI_HV_GLB_PKT_LEN_CFG_t      reg_data1;
	NI_HV_GLB_PKT_STOR_LEN_CFG_t reg_data2;


	__NI_ASSERT(config);

	if ( 0 == mask.u32 ) {
		return CA_E_OK;
	}

	if ( (mask.s.min_pkt_size &&
	      (config->min_pkt_size < AAL_NI_PKT_LEN_MIN ||
	       config->min_pkt_size > AAL_NI_PKT_LEN_MAX)) ||
	     (mask.s.max_pkt_size &&
	      (config->max_pkt_size < AAL_NI_PKT_LEN_MIN ||
	       config->max_pkt_size > AAL_NI_PKT_LEN_MAX)) ) {
		return CA_E_PARAM;
	}

	/* MIN pkt size config should be less-equal than MAX pkt size */
	if ( mask.s.min_pkt_size &&
	     mask.s.max_pkt_size &&
	     config->min_pkt_size > config->max_pkt_size ) {

		return CA_E_PARAM;
	}

	if ( mask.s.max_store_pkt_len &&
	     config->max_store_pkt_len > AAL_NI_PKT_LEN_MAX ) {
		return CA_E_PARAM;
	}

	reg_data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PKT_LEN_CFG);
	reg_data2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PKT_STOR_LEN_CFG);

	if ( mask.s.min_pkt_size ) {
		reg_data1.bf.min_pkt_size = config->min_pkt_size;
	}

	if ( mask.s.max_pkt_size ) {
		reg_data1.bf.max_pkt_size = config->max_pkt_size;
	}

	if ( mask.s.min_pkt_size || mask.s.max_pkt_size ) {
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_PKT_LEN_CFG,reg_data1.wrd);
		ca_mdelay(1);
	}

	if ( mask.s.max_store_pkt_len ) {
		reg_data2.bf.max_stor_pkt_len = config->max_store_pkt_len;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_PKT_STOR_LEN_CFG,reg_data2.wrd);
		ca_mdelay(1);
	}

	return CA_E_OK;
}

ca_status_t aal_ni_pkt_len_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_pkt_len_config_t *config
	)
{
	NI_HV_GLB_PKT_LEN_CFG_t      reg_data1;
	NI_HV_GLB_PKT_STOR_LEN_CFG_t reg_data2;


	__NI_ASSERT(config);

	reg_data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PKT_LEN_CFG);
	reg_data2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PKT_STOR_LEN_CFG);

	config->min_pkt_size = reg_data1.bf.min_pkt_size;
	config->max_pkt_size = reg_data1.bf.max_pkt_size;
	config->max_store_pkt_len = reg_data2.bf.max_stor_pkt_len;

	return CA_E_OK;

}

ca_status_t aal_ni_port_rx_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_rx_ctrl_mask_t mask,
	CA_IN aal_ni_port_rx_ctrl_t *config
	)
{
	NI_HV_PT_RX_CNTRL_CFG_t reg_cfg;

	__NI_ASSERT(config);
	__NI_PORT_CHK(device_id,port_id);

	reg_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_RX_CNTRL_CFG + port_id * __NI_PORT_REG_STRIDE);

	if ( mask.s.ff_overrun_drop_dis )
		reg_cfg.bf.ff_overrun_drop_dis = config->ff_overrun_drop_dis;

	if ( mask.s.crc_drop_dis )
		reg_cfg.bf.crc_drop_dis = config->crc_drop_dis;

	if ( mask.s.ovr_drop_dis )
		reg_cfg.bf.ovr_drop_dis = config->ovr_drop_dis;

	if ( mask.s.runt_drop_dis )
		reg_cfg.bf.runt_drop_dis = config->runt_drop_dis;

	if ( mask.s.runt_drop_dis )
		reg_cfg.bf.runt_drop_dis = config->runt_drop_dis;

	if ( mask.s.ukopcode_drop_dis )
		reg_cfg.bf.ukopcode_drop_dis = config->ukopcode_drop_dis;

	if ( mask.s.oam_drop_dis )
		reg_cfg.bf.oam_drop_dis = config->oam_drop_dis;

	if ( mask.s.linkstat_drop_dis )
		reg_cfg.bf.linkstat_drop_dis = config->linkstat_drop_dis;

	if ( mask.s.pfc_drop_dis )
		reg_cfg.bf.pfc_drop_dis = config->pfc_drop_dis;

	if ( mask.s.rxctrl_byp_dpid )
		reg_cfg.bf.rxctrl_byp_dpid = config->rxctrl_byp_dpid;

	if ( mask.s.rxctrl_byp_cos )
		reg_cfg.bf.rxctrl_byp_cos = config->rxctrl_byp_cos;

	if ( mask.s.rxctrl_byp_en )
		reg_cfg.bf.rxctrl_byp_en = config->rxctrl_byp_en;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_RX_CNTRL_CFG + port_id * __NI_PORT_REG_STRIDE,reg_cfg.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_port_rx_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_port_rx_ctrl_t *config)
{
	NI_HV_PT_RX_CNTRL_CFG_t reg_cfg;

	__NI_ASSERT(config);

	reg_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_RX_CNTRL_CFG + port_id * __NI_PORT_REG_STRIDE);

	config->ff_overrun_drop_dis = reg_cfg.bf.ff_overrun_drop_dis;
	config->crc_drop_dis = reg_cfg.bf.crc_drop_dis;
	config->ovr_drop_dis = reg_cfg.bf.ovr_drop_dis;
	config->runt_drop_dis = reg_cfg.bf.runt_drop_dis;
	config->runt_drop_dis = reg_cfg.bf.runt_drop_dis;
	config->ukopcode_drop_dis = reg_cfg.bf.ukopcode_drop_dis;
	config->oam_drop_dis = reg_cfg.bf.oam_drop_dis;
	config->linkstat_drop_dis = reg_cfg.bf.linkstat_drop_dis;
	config->pfc_drop_dis = reg_cfg.bf.pfc_drop_dis;
	config->rxctrl_byp_dpid = reg_cfg.bf.rxctrl_byp_dpid;
	config->rxctrl_byp_cos = reg_cfg.bf.rxctrl_byp_cos;
	config->rxctrl_byp_en = reg_cfg.bf.rxctrl_byp_en;

	return CA_E_OK;
}

ca_status_t aal_ni_eth_if_reset_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_eth_if_reset_ctrl_mask_t mask,
	CA_IN aal_ni_eth_if_reset_ctrl_t *config
	)
{
	NI_HV_GLB_INTF_RST_CONFIG_t reg_cfg;

	__NI_ASSERT(config);

	reg_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTF_RST_CONFIG);

	if ( mask.s.intf_rst_p0 )
		reg_cfg.bf.intf_rst_p0 = config->intf_rst_p0;
	if ( mask.s.intf_rst_p1 )
		reg_cfg.bf.intf_rst_p1 = config->intf_rst_p1;
	if ( mask.s.intf_rst_p2 )
		reg_cfg.bf.intf_rst_p2 = config->intf_rst_p2;
	if ( mask.s.intf_rst_p3 )
		reg_cfg.bf.intf_rst_p3 = config->intf_rst_p3;
	if ( mask.s.intf_rst_p4 )
		reg_cfg.bf.intf_rst_p4 = config->intf_rst_p4;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_INTF_RST_CONFIG,reg_cfg.wrd);

	return CA_E_OK;

}

ca_status_t
aal_ni_eth_if_reset_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_eth_if_reset_ctrl_t *config)
{
	NI_HV_GLB_INTF_RST_CONFIG_t reg_cfg;

	__NI_ASSERT(config);

	reg_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTF_RST_CONFIG);
	config->intf_rst_p0 = reg_cfg.bf.intf_rst_p0;
	config->intf_rst_p1 = reg_cfg.bf.intf_rst_p1;
	config->intf_rst_p2 = reg_cfg.bf.intf_rst_p2;
	config->intf_rst_p3 = reg_cfg.bf.intf_rst_p3;
	config->intf_rst_p4 = reg_cfg.bf.intf_rst_p4;

	return CA_E_OK;
}

ca_status_t aal_ni_eth_if_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_eth_if_config_mask_t mask,
	CA_IN aal_ni_eth_if_config_t *config
	)
{
	NI_HV_PT_PORT_STATIC_CFG_t reg_data;

	__NI_ASSERT(config);
	__NI_PORT_CHK(device_id,port_id);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_PORT_STATIC_CFG + port_id * __NI_PORT_REG_STRIDE);

	if ( mask.s.int_cfg )
		reg_data.bf.int_cfg = config->int_cfg;
	if ( mask.s.phy_mode )
		reg_data.bf.phy_mode = config->phy_mode;
	if ( mask.s.rmii_clksrc )
		reg_data.bf.rmii_clksrc = config->rmii_clksrc;
	if ( mask.s.inv_clk_in )
		reg_data.bf.inv_clk_in = config->inv_clk_in;
	if ( mask.s.inv_clk_out )
		reg_data.bf.inv_clk_out = config->inv_clk_out;
	if ( mask.s.inv_rxclk_out )
		reg_data.bf.inv_rxclk_out = config->inv_rxclk_out;
	if ( mask.s.tx_use_gefifo )
		reg_data.bf.tx_use_gefifo = config->tx_use_gefifo;
	if ( mask.s.smii_tx_stat )
		reg_data.bf.smii_tx_stat = config->smii_tx_stat;
	if ( mask.s.crs_polarity )
		reg_data.bf.crs_polarity = config->crs_polarity;
	if ( mask.s.lpbk_mode )
		reg_data.bf.lpbk_mode = config->lpbk_mode;
	if ( mask.s.sup_tx_to_rx_lpbk_data )
		reg_data.bf.sup_tx_to_rx_lpbk_data = config->sup_tx_to_rx_lpbk_data;
	if ( mask.s.mac_addr6 )
		reg_data.bf.mac_addr6 = config->mac_addr6;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_PORT_STATIC_CFG + port_id * __NI_PORT_REG_STRIDE,reg_data.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_eth_if_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_eth_if_config_t *config
	)
{
	NI_HV_PT_PORT_STATIC_CFG_t reg_data;

	__NI_ASSERT(config);
	__NI_PORT_CHK(device_id,port_id);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_PORT_STATIC_CFG + port_id * __NI_PORT_REG_STRIDE);

	config->int_cfg = reg_data.bf.int_cfg;
	config->phy_mode = reg_data.bf.phy_mode;
	config->rmii_clksrc = reg_data.bf.rmii_clksrc;
	config->inv_clk_in = reg_data.bf.inv_clk_in;
	config->inv_clk_out = reg_data.bf.inv_clk_out;
	config->inv_rxclk_out = reg_data.bf.inv_rxclk_out;
	config->tx_use_gefifo = reg_data.bf.tx_use_gefifo;
	config->smii_tx_stat = reg_data.bf.smii_tx_stat;
	config->crs_polarity = reg_data.bf.crs_polarity;
	config->lpbk_mode = reg_data.bf.lpbk_mode;
	config->sup_tx_to_rx_lpbk_data = reg_data.bf.sup_tx_to_rx_lpbk_data;
	config->mac_addr6 = reg_data.bf.mac_addr6;

	return CA_E_OK;

}

/**aal_ni_eth_port_config_set
 *@brief: set GE port INTERFACE according to speed
 */
ca_status_t
aal_ni_eth_port_config_set(
	ca_device_id_t device_id,
	ca_uint32_t port_id,
	ca_uint32_t speed)
{
	ca_uint8_t                 phy_intfs[CFG_ID_PHY_INTERFACE_LEN]       = { 0 };
	NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg,
				   old_cfg;

	__NI_PORT_CHK(device_id,port_id);

	if ( CA_E_OK != aal_scfg_read(CFG_ID_PHY_INTERFACE,CFG_ID_PHY_INTERFACE_LEN,&(phy_intfs[0])) ) {
		ca_printf("%s(): read CFG_ID_PHY_INTERFACE fail\n",__func__);
		return CA_E_NOT_FOUND;
	}

	port_static_cfg.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_PORT_STATIC_CFG + port_id * __NI_PORT_REG_STRIDE);
	old_cfg.wrd = port_static_cfg.wrd;

	if ( phy_intfs[port_id] == AAL_NI_IF_MODE_GE_RGMII || phy_intfs[port_id] == AAL_NI_IF_MODE_FE_RGMII ) {
		if ( speed == AAL_NI_MAC_SPEED_1000M ) {
			port_static_cfg.bf.int_cfg = AAL_NI_IF_MODE_GE_RGMII;
		} else {
			port_static_cfg.bf.int_cfg = AAL_NI_IF_MODE_FE_RGMII;
		}
	} else {
		if ( speed == AAL_NI_MAC_SPEED_1000M ) {
			port_static_cfg.bf.int_cfg = AAL_NI_IF_MODE_GE_GMII;
		} else {
			port_static_cfg.bf.int_cfg = AAL_NI_IF_MODE_FE_MII;
		}
	}

	if ( port_static_cfg.wrd != old_cfg.wrd ) {
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_PORT_STATIC_CFG + port_id * __NI_PORT_REG_STRIDE,port_static_cfg.wrd);
		printk("%s: port_id=%d, phy_intfs[port_id]=%d, speed=%d, port_static_cfg.wrd=0x%x\n",__func__,port_id,phy_intfs[port_id],speed,port_static_cfg.wrd);
	}

	return CA_E_OK;
}

ca_status_t aal_ni_eth_port_mac_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_eth_mac_config_mask_t mask,
	CA_IN aal_ni_eth_mac_config_t *config
	)
{
	NI_HV_PT_RXMAC_CFG_t     reg_cfg0;
	NI_HV_PT_TXMAC_CFG_t     reg_cfg1;
	NI_HV_PT_PORT_GLB_CFG_t  reg_cfg2;
	L2TM_L2TE_FC_CTRL_t      reg_cfg3;
	L2TM_L2TE_FC_GLB_TIMER_t reg_cfg4;

	__NI_ASSERT(config);

	if ( !mask.u32 ) {
		return CA_E_OK;
	}
	reg_cfg4.wrd = CA_DEV_NI_REG_READ(device_id,L2TM_L2TE_FC_GLB_TIMER);

	if ( port_id >= AAL_NI_GE_PORT_START && port_id <= AAL_NI_GE_PORT_END ) {
		reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_RXMAC_CFG + port_id * __NI_PORT_REG_STRIDE);
		reg_cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_TXMAC_CFG + port_id * __NI_PORT_REG_STRIDE);
		reg_cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_PORT_GLB_CFG + port_id * __NI_PORT_REG_STRIDE);

		if ( mask.s.rx_en )
			reg_cfg0.bf.rx_en = config->rx_en;
		if ( mask.s.rx_flow_disable )
			reg_cfg0.bf.rx_flow_disable = config->rx_flow_disable;
		if ( mask.s.rx_flow_to_tx_en )
			reg_cfg0.bf.rx_flow_to_tx_en = config->rx_flow_to_tx_en;
		if ( mask.s.rx_pfc_disable )
			reg_cfg0.bf.rx_pfc_disable = config->rx_pfc_disable;
		if ( mask.s.send_pg_data )
			reg_cfg0.bf.send_pg_data = config->send_pg_data;
		if ( mask.s.tx_en )
			reg_cfg1.bf.tx_en = config->tx_en;
		if ( mask.s.mac_crc_calc_en )
			reg_cfg1.bf.mac_crc_calc_en = config->mac_crc_calc_en;
		if ( mask.s.tx_ipg_sel )
			reg_cfg1.bf.tx_ipg_sel = config->tx_ipg_sel;
		if ( mask.s.tx_flow_disable )
			reg_cfg1.bf.tx_flow_disable = config->tx_flow_disable;
		if ( mask.s.tx_drain )
			reg_cfg1.bf.tx_drain = config->tx_drain;
		if ( mask.s.tx_pfc_disable )
			reg_cfg1.bf.tx_pfc_disable = config->tx_pfc_disable;
		if ( mask.s.tx_auto_xon )
			reg_cfg1.bf.tx_auto_xon = config->tx_auto_xon;
		if ( mask.s.tx_pause_sel )
			reg_cfg1.bf.tx_pau_sel = config->tx_pause_sel;
		if ( mask.s.power_dwn_tx )
			reg_cfg2.bf.power_dwn_tx = config->power_dwn_tx;
		if ( mask.s.power_dwn_rx )
			reg_cfg2.bf.power_dwn_rx = config->power_dwn_rx;
		if ( mask.s.duplex )
			reg_cfg2.bf.duplex = (config->duplex == AAL_NI_MAC_DUPLEX_HALF) ? 1 : 0;
		if ( mask.s.speed )
			reg_cfg2.bf.speed = (config->speed == AAL_NI_MAC_SPEED_10M) ? 1 : 0;

		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_RXMAC_CFG + port_id * __NI_PORT_REG_STRIDE,reg_cfg0.wrd);
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_TXMAC_CFG + port_id * __NI_PORT_REG_STRIDE,reg_cfg1.wrd);
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_PT_PORT_GLB_CFG + port_id * __NI_PORT_REG_STRIDE,reg_cfg2.wrd);
		//ca_printf("%s(): write reg %#x to be %#x\r\n", __func__, NI_HV_PT_PORT_GLB_CFG, reg_cfg2.wrd);

		if ( mask.s.speed ) {
			aal_ni_eth_port_config_set(device_id,port_id,config->speed);
		}
	}

	if ( port_id < APB0_L2TM_L2TE_FC_COUNT ) {
		reg_cfg3.wrd = CA_DEV_NI_REG_READ(device_id,L2TM_L2TE_FC_CTRL + port_id * APB0_L2TM_L2TE_FC_STRIDE);
		if ( mask.s.tx_pause_en )
			reg_cfg3.bf.tx_en = config->tx_pause_en;
		if ( mask.s.rx_pause_en )
			reg_cfg3.bf.rx_en = config->rx_pause_en;
		if ( mask.s.rate_limit_pause_check_bm_ena )
			reg_cfg3.bf.src_rate_enforce = (config->rate_limit_pause_check_bm_ena == FALSE);
		CA_DEV_NI_REG_WRITE2(device_id,L2TM_L2TE_FC_CTRL + port_id * APB0_L2TM_L2TE_FC_STRIDE,reg_cfg3.wrd);
	}

	return CA_E_OK;

}

ca_status_t aal_ni_eth_port_mac_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_eth_mac_config_t *config
	)
{
	NI_HV_PT_RXMAC_CFG_t    reg_cfg0;
	NI_HV_PT_TXMAC_CFG_t    reg_cfg1;
	NI_HV_PT_PORT_GLB_CFG_t reg_cfg2;
	L2TM_L2TE_FC_CTRL_t     reg_cfg3;

	__NI_ASSERT(config);
	//__NI_PORT_CHK(device_id, port_id);

	if ( port_id < AAL_NI_PORT_NUM ) {
		reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_RXMAC_CFG + port_id * __NI_PORT_REG_STRIDE);
		reg_cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_TXMAC_CFG + port_id * __NI_PORT_REG_STRIDE);
		reg_cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_PORT_GLB_CFG + port_id * __NI_PORT_REG_STRIDE);
		config->rx_en = reg_cfg0.bf.rx_en;
		config->rx_flow_disable = reg_cfg0.bf.rx_flow_disable;
		config->rx_flow_to_tx_en = reg_cfg0.bf.rx_flow_to_tx_en;
		config->rx_pfc_disable = reg_cfg0.bf.rx_pfc_disable;
		config->send_pg_data = reg_cfg0.bf.send_pg_data;
		config->tx_en = reg_cfg1.bf.tx_en;
		config->mac_crc_calc_en = reg_cfg1.bf.mac_crc_calc_en;
		config->tx_ipg_sel = reg_cfg1.bf.tx_ipg_sel;
		config->tx_flow_disable = reg_cfg1.bf.tx_flow_disable;
		config->tx_drain = reg_cfg1.bf.tx_drain;
		config->tx_pfc_disable = reg_cfg1.bf.tx_pfc_disable;
		config->tx_auto_xon = reg_cfg1.bf.tx_auto_xon;
		config->tx_pause_sel = reg_cfg1.bf.tx_pau_sel;
		config->power_dwn_tx = reg_cfg2.bf.power_dwn_tx;
		config->power_dwn_rx = reg_cfg2.bf.power_dwn_rx;
		config->duplex = reg_cfg2.bf.duplex ? AAL_NI_MAC_DUPLEX_HALF : AAL_NI_MAC_DUPLEX_FULL;
		config->speed = reg_cfg2.bf.speed ? AAL_NI_MAC_SPEED_10M : AAL_NI_MAC_SPEED_100M;
	}

	if ( port_id < APB0_L2TM_L2TE_FC_COUNT ) {
		reg_cfg3.wrd = CA_DEV_NI_REG_READ(device_id,L2TM_L2TE_FC_CTRL + port_id * APB0_L2TM_L2TE_FC_STRIDE);
		config->tx_pause_en = reg_cfg3.bf.tx_en;
		config->rx_pause_en = reg_cfg3.bf.rx_en;
		config->rate_limit_pause_check_bm_ena = (reg_cfg3.bf.src_rate_enforce == 0);
	}

	return CA_E_OK;

}

ca_status_t aal_ni_mc_lkup_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t mc_group,
	CA_IN ca_uint64_t port_bmp
	)
{
	NI_HV_MCE_INDX_LKUP_ACCESS_t table_access;
	NI_HV_MCE_INDX_LKUP_DATA0_t  table_data;
	NI_HV_MCE_INDX_LKUP_DATA1_t  table_data1;

	if ( mc_group >= AAL_NI_MC_GROUP_NUM ) {
		return CA_E_PARAM;
	}
#if 0
	__NI_MC_TABLE_ACCESS(device_id,__NI_TBL_READ_FLAG,
			     NI_HV_MCE_INDX_LKUP_ACCESS,
			     mc_group, table_access);

	table_data.wrd = CA_DEV_NI_REG_READ(device_id, NI_HV_MCE_INDX_LKUP_DATA);
#endif
	table_data.wrd = 0;
	table_data.bf.mc_vec = port_bmp;
	table_data1.wrd = 0;
	table_data1.bf.mc_vec = port_bmp >> 32;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_MCE_INDX_LKUP_DATA0,table_data.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_MCE_INDX_LKUP_DATA1,table_data1.wrd);

	__NI_MC_TABLE_ACCESS(device_id,__NI_TBL_WRITE_FLAG,
		NI_HV_MCE_INDX_LKUP_ACCESS,
		mc_group,table_access);

	return CA_E_OK;

}

ca_status_t aal_ni_mc_lkup_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t mc_group,
	CA_IN ca_uint64_t *port_bmp
	)
{
	NI_HV_MCE_INDX_LKUP_ACCESS_t table_access;
	NI_HV_MCE_INDX_LKUP_DATA0_t  table_data;
	NI_HV_MCE_INDX_LKUP_DATA1_t  table_data1;

	AAL_ASSERT_RET(port_bmp,CA_E_PARAM);

	if ( mc_group >= AAL_NI_MC_GROUP_NUM ) {
		return CA_E_PARAM;
	}

	__NI_MC_TABLE_ACCESS(device_id,__NI_TBL_READ_FLAG,
		NI_HV_MCE_INDX_LKUP_ACCESS,
		mc_group,table_access);

	table_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_MCE_INDX_LKUP_DATA0);
	table_data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_MCE_INDX_LKUP_DATA1);

	*port_bmp = table_data1.bf.mc_vec;
	*port_bmp = (*port_bmp << 32) + table_data.bf.mc_vec;

	return CA_E_OK;

}

/**aal_ni_eth_port_status_get:
 *@brief:Retrive port ETH MAC status bits received inband
 *@note: These bits are valid only in RGMII mode
 */
ca_status_t aal_ni_eth_port_status_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_eth_port_status_t *status
	)
{
	NI_HV_PT_MAC_STATUS_t reg_data;

	__NI_ASSERT(status);
	__NI_PORT_CHK(device_id,port_id);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_MAC_STATUS + port_id * __NI_PORT_REG_STRIDE);

	status->link_status = reg_data.bf.rcv_lnk_stat;
	status->speed = reg_data.bf.rcv_speed;
	status->duplex = reg_data.bf.rcv_duplex;

	return CA_E_OK;

}

ca_status_t aal_ni_port_intr_enable_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_intr_mask_t mask,
	CA_IN aal_ni_port_intr_t *config
	)
{
	NI_HV_GLB_PORT_0_INTERRUPTE_t reg_data;

	__NI_PORT_CHK(device_id,port_id);

	if ( device_id == 0 )
		reg_data.wrd = CA_NI_REG_READ(NI_HV_GLB_PORT_0_INTERRUPTE + (port_id * NI_HV_GLB_PORT_0_INTERRUPTE_STRIDE));

	if ( mask.s.link_stat_chg )
		reg_data.bf.link_stat_chgIE = config->link_stat_chg;
	if ( mask.s.txfifo_underrun )
		reg_data.bf.txfifo_underrunIE = config->txfifo_underrun;
	if ( mask.s.txfifo_overrun )
		reg_data.bf.txfifo_overrunIE = config->txfifo_overrun;
	if ( mask.s.rxcntrl_overrun )
		reg_data.bf.rxcntrl_overrunIE = config->rxcntrl_overrun;
	if ( mask.s.rxmib_cntmsb_set )
		reg_data.bf.rxmib_cntmsb_setIE = config->rxmib_cntmsb_set;
	if ( mask.s.txmib_cntmsb_set )
		reg_data.bf.txmib_cntmsb_setIE = config->txmib_cntmsb_set;
	if ( mask.s.lpi_assert_stat_chg )
		reg_data.bf.lpi_assert_stat_chgIE = config->lpi_assert_stat_chg;
	if ( mask.s.rx_lpi_stat_chg )
		reg_data.bf.rx_lpi_stat_chgIE = config->rx_lpi_stat_chg;
	if ( mask.s.ni_wol_int )
		reg_data.bf.ni_wol_intE = config->ni_wol_int;
	if ( device_id == 0 )
		CA_NI_REG_WRITE(reg_data.wrd,NI_HV_GLB_PORT_0_INTERRUPTE + (port_id * NI_HV_GLB_PORT_0_INTERRUPTE_STRIDE));


	return CA_E_OK;

}

ca_status_t aal_ni_port_intr_enable_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_intr_t *config
	)
{
	NI_HV_GLB_PORT_0_INTERRUPTE_t reg_data;

	__NI_PORT_CHK(device_id,port_id);
	__NI_ASSERT(config);

	reg_data.wrd = CA_NI_REG_READ(NI_HV_GLB_PORT_0_INTERRUPTE + (port_id * NI_HV_GLB_PORT_0_INTERRUPTE_STRIDE));

	config->link_stat_chg = reg_data.bf.link_stat_chgIE;
	config->txfifo_underrun = reg_data.bf.txfifo_underrunIE;
	config->txfifo_overrun = reg_data.bf.txfifo_overrunIE;
	config->rxcntrl_overrun = reg_data.bf.rxcntrl_overrunIE;
	config->rxmib_cntmsb_set = reg_data.bf.rxmib_cntmsb_setIE;
	config->txmib_cntmsb_set = reg_data.bf.txmib_cntmsb_setIE;
	config->lpi_assert_stat_chg = reg_data.bf.lpi_assert_stat_chgIE;
	config->rx_lpi_stat_chg = reg_data.bf.rx_lpi_stat_chgIE;
	config->ni_wol_int = reg_data.bf.ni_wol_intE;

	return CA_E_OK;

}

ca_status_t aal_ni_port_intr_status_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_intr_t *status
	)
{
	NI_HV_GLB_PORT_0_INTERRUPT_t reg_data;

	__NI_PORT_CHK(device_id,port_id);
	__NI_ASSERT(status);

	reg_data.wrd = CA_NI_REG_READ(NI_HV_GLB_PORT_0_INTERRUPT + (port_id * NI_HV_GLB_PORT_0_INTERRUPT_STRIDE));

	status->link_stat_chg = reg_data.bf.link_stat_chgI;
	status->txfifo_underrun = reg_data.bf.txfifo_underrunI;
	status->txfifo_overrun = reg_data.bf.txfifo_overrunI;
	status->rxcntrl_overrun = reg_data.bf.rxcntrl_overrunI;
	status->rxmib_cntmsb_set = reg_data.bf.rxmib_cntmsb_setI;
	status->txmib_cntmsb_set = reg_data.bf.txmib_cntmsb_setI;
	status->lpi_assert_stat_chg = reg_data.bf.lpi_assert_stat_chgI;
	status->rx_lpi_stat_chg = reg_data.bf.rx_lpi_stat_chgI;
	status->ni_wol_int = reg_data.bf.ni_wol_int;

	return CA_E_OK;
}


ca_status_t aal_ni_port_intr_status_clear( /*AUTO-CLI gen ignore*/
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_intr_t *status
	)
{
	NI_HV_GLB_PORT_0_INTERRUPT_t reg_data;

	__NI_PORT_CHK(device_id,port_id);
	__NI_ASSERT(status);

	reg_data.wrd = 0;
	reg_data.bf.link_stat_chgI = status->link_stat_chg;
	reg_data.bf.txfifo_underrunI = status->txfifo_underrun;
	reg_data.bf.txfifo_overrunI = status->txfifo_overrun;
	reg_data.bf.rxcntrl_overrunI = status->rxcntrl_overrun;
	reg_data.bf.rxmib_cntmsb_setI = status->rxmib_cntmsb_set;
	reg_data.bf.txmib_cntmsb_setI = status->txmib_cntmsb_set;
	reg_data.bf.lpi_assert_stat_chgI = status->lpi_assert_stat_chg;
	reg_data.bf.rx_lpi_stat_chgI = status->rx_lpi_stat_chg;
	reg_data.bf.ni_wol_int = status->ni_wol_int;
	CA_NI_REG_WRITE(reg_data.wrd,NI_HV_GLB_PORT_0_INTERRUPT + (port_id * NI_HV_GLB_PORT_0_INTERRUPT_STRIDE));

	return CA_E_OK;

}

ca_status_t aal_ni_glb_intr_enable_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_glb_intr_mask_t mask,
	CA_IN aal_ni_glb_intr_t *config
	)
{
	NI_HV_GLB_INTERRUPTE_t reg_data;

	__NI_ASSERT(config);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTERRUPTE);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.int_stat_pspid_0 )
		reg_data.bf.int_stat_pspid_0E = config->int_stat_pspid_0;
	if ( mask.s.int_stat_pspid_1 )
		reg_data.bf.int_stat_pspid_1E = config->int_stat_pspid_1;
	if ( mask.s.int_stat_pspid_2 )
		reg_data.bf.int_stat_pspid_2E = config->int_stat_pspid_2;
#endif
	if ( mask.s.int_stat_pspid_3 )
		reg_data.bf.int_stat_pspid_3E = config->int_stat_pspid_3;
	if ( mask.s.int_stat_pspid_4 )
		reg_data.bf.int_stat_pspid_4E = config->int_stat_pspid_4;
	if ( mask.s.int_stat_pspid_8 )
		reg_data.bf.int_stat_pspid_8E = config->int_stat_pspid_8;
	if ( mask.s.int_stat_pspid_9 )
		reg_data.bf.int_stat_pspid_9E = config->int_stat_pspid_9;
	if ( mask.s.int_stat_pspid_10 )
		reg_data.bf.int_stat_pspid_10E = config->int_stat_pspid_10;
	if ( mask.s.int_stat_txem )
		reg_data.bf.int_stat_txemE = config->int_stat_txem;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.int_stat_pc0 )
		reg_data.bf.int_stat_pc0E = config->int_stat_pc0;
	if ( mask.s.int_stat_pc1 )
		reg_data.bf.int_stat_pc1E = config->int_stat_pc1;
	if ( mask.s.int_stat_pc2 )
		reg_data.bf.int_stat_pc2E = config->int_stat_pc2;
#endif
	if ( mask.s.int_stat_pc3 )
		reg_data.bf.int_stat_pc3E = config->int_stat_pc3;
	if ( mask.s.int_stat_pc4 )
		reg_data.bf.int_stat_pc4E = config->int_stat_pc4;
	if ( mask.s.cpuxram_int_stat_cntr )
		reg_data.bf.cpuxram_int_stat_cntrE = config->cpuxram_int_stat_cntr;
	if ( mask.s.cpuxram_int_stat_err )
		reg_data.bf.cpuxram_int_stat_errE = config->cpuxram_int_stat_err;
	if ( mask.s.int_stat_ptp )
		reg_data.bf.int_stat_ptpE = config->int_stat_ptp;
	if ( mask.s.int_stat_mceng )
		reg_data.bf.int_stat_mcengE = config->int_stat_mceng;
	if ( mask.s.cpuxram_rxpkt )
		reg_data.bf.cpuxram_rxpktE = config->cpuxram_rxpkt;
	if ( mask.s.cpuxram_txpkt )
		reg_data.bf.cpuxram_txpktE = config->cpuxram_txpkt;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_INTERRUPTE,reg_data.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_glb_intr_enable_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_glb_intr_t *config
	)
{
	NI_HV_GLB_INTERRUPTE_t reg_data;

	__NI_ASSERT(config);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTERRUPTE);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->int_stat_pspid_0 = reg_data.bf.int_stat_pspid_0E;
	config->int_stat_pspid_1 = reg_data.bf.int_stat_pspid_1E;
	config->int_stat_pspid_2 = reg_data.bf.int_stat_pspid_2E;
#endif

	config->int_stat_pspid_3 = reg_data.bf.int_stat_pspid_3E;
	config->int_stat_pspid_4 = reg_data.bf.int_stat_pspid_4E;
	config->int_stat_pspid_8 = reg_data.bf.int_stat_pspid_8E;
	config->int_stat_pspid_9 = reg_data.bf.int_stat_pspid_9E;
	config->int_stat_pspid_10 = reg_data.bf.int_stat_pspid_10E;
	config->int_stat_txem = reg_data.bf.int_stat_txemE;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->int_stat_pc0 = reg_data.bf.int_stat_pc0E;
	config->int_stat_pc1 = reg_data.bf.int_stat_pc1E;
	config->int_stat_pc2 = reg_data.bf.int_stat_pc2E;
#endif
	config->int_stat_pc3 = reg_data.bf.int_stat_pc3E;
	config->int_stat_pc4 = reg_data.bf.int_stat_pc4E;
	//config->int_stat_pc5          =   reg_data.bf.int_stat_pc5E         ;
	//config->int_stat_pc6          =   reg_data.bf.int_stat_pc6E         ;
	config->cpuxram_int_stat_cntr = reg_data.bf.cpuxram_int_stat_cntrE;
	config->cpuxram_int_stat_err = reg_data.bf.cpuxram_int_stat_errE;
	config->int_stat_ptp = reg_data.bf.int_stat_ptpE;
	config->int_stat_mceng = reg_data.bf.int_stat_mcengE;
	config->cpuxram_rxpkt = reg_data.bf.cpuxram_rxpktE;
	config->cpuxram_txpkt = reg_data.bf.cpuxram_txpktE;

	return CA_E_OK;

}

ca_status_t aal_ni_glb_intr_status_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_glb_intr_t *status
	)
{
	NI_HV_GLB_INTERRUPT_t reg_data;

	__NI_ASSERT(status);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTERRUPT);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	status->int_stat_pspid_0 = reg_data.bf.int_stat_pspid_0;
	status->int_stat_pspid_1 = reg_data.bf.int_stat_pspid_1;
	status->int_stat_pspid_2 = reg_data.bf.int_stat_pspid_2;
#endif
	status->int_stat_pspid_3 = reg_data.bf.int_stat_pspid_3;
	status->int_stat_pspid_4 = reg_data.bf.int_stat_pspid_4;
	status->int_stat_pspid_8 = reg_data.bf.int_stat_pspid_8;
	status->int_stat_pspid_9 = reg_data.bf.int_stat_pspid_9;
	status->int_stat_pspid_10 = reg_data.bf.int_stat_pspid_10;
	status->int_stat_txem = reg_data.bf.int_stat_txem;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	status->int_stat_pc0 = reg_data.bf.int_stat_pc0;
	status->int_stat_pc1 = reg_data.bf.int_stat_pc1;
	status->int_stat_pc2 = reg_data.bf.int_stat_pc2;
#endif
	status->int_stat_pc3 = reg_data.bf.int_stat_pc3;
	status->int_stat_pc4 = reg_data.bf.int_stat_pc4;
	//status->int_stat_pc5          =   reg_data.bf.int_stat_pc5           ;
	//status->int_stat_pc6          =   reg_data.bf.int_stat_pc6           ;
	status->cpuxram_int_stat_cntr = reg_data.bf.cpuxram_int_stat_cntr;
	status->cpuxram_int_stat_err = reg_data.bf.cpuxram_int_stat_err;
	status->int_stat_ptp = reg_data.bf.int_stat_ptp;
	status->int_stat_mceng = reg_data.bf.int_stat_mceng;
	status->cpuxram_rxpkt = reg_data.bf.cpuxram_rxpkt;
	status->cpuxram_txpkt = reg_data.bf.cpuxram_txpkt;

	return CA_E_OK;

}

ca_status_t aal_ni_glb_intr_status_clear( /*AUTO-CLI gen ignore*/
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_glb_intr_t *status
	)
{
	NI_HV_GLB_INTERRUPT_t reg_data;

	__NI_ASSERT(status);

	reg_data.wrd = 0;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	reg_data.bf.int_stat_pspid_0 = status->int_stat_pspid_0;
	reg_data.bf.int_stat_pspid_1 = status->int_stat_pspid_1;
	reg_data.bf.int_stat_pspid_2 = status->int_stat_pspid_2;
#endif
	reg_data.bf.int_stat_pspid_3 = status->int_stat_pspid_3;
	reg_data.bf.int_stat_pspid_4 = status->int_stat_pspid_4;
	reg_data.bf.int_stat_pspid_8 = status->int_stat_pspid_8;
	reg_data.bf.int_stat_pspid_9 = status->int_stat_pspid_9;
	reg_data.bf.int_stat_pspid_10 = status->int_stat_pspid_10;
	reg_data.bf.int_stat_txem = status->int_stat_txem;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	reg_data.bf.int_stat_pc0 = status->int_stat_pc0;
	reg_data.bf.int_stat_pc1 = status->int_stat_pc1;
	reg_data.bf.int_stat_pc2 = status->int_stat_pc2;
#endif
	reg_data.bf.int_stat_pc3 = status->int_stat_pc3;
	reg_data.bf.int_stat_pc4 = status->int_stat_pc4;
	//reg_data.bf.int_stat_pc5           = status->int_stat_pc5         ;
	//reg_data.bf.int_stat_pc6           = status->int_stat_pc6         ;
	reg_data.bf.cpuxram_int_stat_cntr = status->cpuxram_int_stat_cntr;
	reg_data.bf.cpuxram_int_stat_err = status->cpuxram_int_stat_err;
	reg_data.bf.int_stat_ptp = status->int_stat_ptp;
	reg_data.bf.int_stat_mceng = status->int_stat_mceng;
	reg_data.bf.cpuxram_rxpkt = status->cpuxram_rxpkt;
	reg_data.bf.cpuxram_txpkt = status->cpuxram_txpkt;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_INTERRUPT,reg_data.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_glb_ptp_tx_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_glb_ptp_tx_cfg_mask_t mask,
	CA_IN aal_ni_glb_ptp_tx_cfg_t *config)
{
	NI_HV_GLB_PTP_TX_CFG_t reg_data;

	__NI_ASSERT(config);

	if ( mask.u32 == 0 )
		return CA_E_OK;

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PTP_TX_CFG);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.p0_ptp_tx_en ) {
		reg_data.bf.p0_ptp_tx_en = config->p0_ptp_tx_en;
	}

	if ( mask.s.p1_ptp_tx_en ) {
		reg_data.bf.p1_ptp_tx_en = config->p1_ptp_tx_en;
	}

	if ( mask.s.p2_ptp_tx_en ) {
		reg_data.bf.p2_ptp_tx_en = config->p2_ptp_tx_en;
	}
#endif
	if ( mask.s.p3_ptp_tx_en ) {
		reg_data.bf.p3_ptp_tx_en = config->p3_ptp_tx_en;
	}

	if ( mask.s.p4_ptp_tx_en ) {
		reg_data.bf.p4_ptp_tx_en = config->p4_ptp_tx_en;
	}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if ( mask.s.p5_ptp_tx_en ) {
		reg_data.bf.p5_ptp_tx_en = config->p5_ptp_tx_en;
	}

	if ( mask.s.p6_ptp_tx_en ) {
		reg_data.bf.p6_ptp_tx_en = config->p6_ptp_tx_en;
	}

	if ( mask.s.p7_ptp_tx_en ) {
		reg_data.bf.p7_ptp_tx_en = config->p7_ptp_tx_en;
	}
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	if ( mask.s.p5_ptp_tx_en ) {
		reg_data.bf.p5_ptp_tx_en = config->p5_ptp_tx_en;
	}

	if ( mask.s.p6_ptp_tx_en ) {
		reg_data.bf.p6_ptp_tx_en = config->p6_ptp_tx_en;
	}
#endif

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_PTP_TX_CFG,reg_data.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_glb_ptp_tx_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_glb_ptp_tx_cfg_t *config)
{
	NI_HV_GLB_PTP_TX_CFG_t reg_data;

	__NI_ASSERT(config);

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PTP_TX_CFG);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->p0_ptp_tx_en = reg_data.bf.p0_ptp_tx_en;
	config->p1_ptp_tx_en = reg_data.bf.p1_ptp_tx_en;
	config->p2_ptp_tx_en = reg_data.bf.p2_ptp_tx_en;
#endif
	config->p3_ptp_tx_en = reg_data.bf.p3_ptp_tx_en;
	config->p4_ptp_tx_en = reg_data.bf.p4_ptp_tx_en;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	config->p5_ptp_tx_en = reg_data.bf.p5_ptp_tx_en;
	config->p6_ptp_tx_en = reg_data.bf.p6_ptp_tx_en;
	config->p7_ptp_tx_en = reg_data.bf.p7_ptp_tx_en;
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	config->p5_ptp_tx_en = reg_data.bf.p5_ptp_tx_en;
	config->p6_ptp_tx_en = reg_data.bf.p6_ptp_tx_en;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_glb_ptp_intr_enable_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_glb_ptp_intr_enable_cfg_mask_t mask,
	CA_IN aal_ni_glb_ptp_intr_enable_cfg_t *config
	)
{
	NI_HV_GLB_PTP_INTERRUPTE_t reg_data;

	__NI_ASSERT(config);

	reg_data.wrd = 0;

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PTP_INTERRUPTE);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.port0_tx_timestamp_rdy_intie ) {
		reg_data.bf.port0_tx_timestamp_rdy_intIE = config->port0_tx_timestamp_rdy_intie;
	}

	if ( mask.s.port1_tx_timestamp_rdy_intie ) {
		reg_data.bf.port1_tx_timestamp_rdy_intIE = config->port1_tx_timestamp_rdy_intie;
	}

	if ( mask.s.port2_tx_timestamp_rdy_intie ) {
		reg_data.bf.port2_tx_timestamp_rdy_intIE = config->port2_tx_timestamp_rdy_intie;
	}
#endif
	if ( mask.s.port3_tx_timestamp_rdy_intie ) {
		reg_data.bf.port3_tx_timestamp_rdy_intIE = config->port3_tx_timestamp_rdy_intie;
	}

	if ( mask.s.port4_tx_timestamp_rdy_intie ) {
		reg_data.bf.port4_tx_timestamp_rdy_intIE = config->port4_tx_timestamp_rdy_intie;
	}

#ifdef CONFIG_ARCH_CORTINA_G3
	if ( mask.s.port5_tx_timestamp_rdy_intie ) {
		reg_data.bf.port5_tx_timestamp_rdy_intIE = config->port5_tx_timestamp_rdy_intie;
	}

	if ( mask.s.port6_tx_timestamp_rdy_intie ) {
		reg_data.bf.port6_tx_timestamp_rdy_intIE = config->port6_tx_timestamp_rdy_intie;
	}

	if ( mask.s.port7_tx_timestamp_rdy_intie ) {
		reg_data.bf.port7_tx_timestamp_rdy_intIE = config->port7_tx_timestamp_rdy_intie;
	}
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.port0_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port0_tx_ptpfifo_overflow_intIE = config->port0_tx_ptpfifo_overflow_intie;
	}

	if ( mask.s.port1_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port1_tx_ptpfifo_overflow_intIE = config->port1_tx_ptpfifo_overflow_intie;
	}

	if ( mask.s.port2_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port2_tx_ptpfifo_overflow_intIE = config->port2_tx_ptpfifo_overflow_intie;
	}
#endif

	if ( mask.s.port3_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port3_tx_ptpfifo_overflow_intIE = config->port3_tx_ptpfifo_overflow_intie;
	}

	if ( mask.s.port4_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port4_tx_ptpfifo_overflow_intIE = config->port4_tx_ptpfifo_overflow_intie;
	}

#ifdef CONFIG_ARCH_CORTINA_G3
	if ( mask.s.port5_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port5_tx_ptpfifo_overflow_intIE = config->port5_tx_ptpfifo_overflow_intie;
	}

	if ( mask.s.port6_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port6_tx_ptpfifo_overflow_intIE = config->port6_tx_ptpfifo_overflow_intie;
	}

	if ( mask.s.port7_tx_ptpfifo_overflow_intie ) {
		reg_data.bf.port7_tx_ptpfifo_overflow_intIE = config->port7_tx_ptpfifo_overflow_intie;
	}
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	if ( mask.s.port0_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port0_tx_ptpfifo_underflow_intIE = config->port0_tx_ptpfifo_underflow_intie;
	}

	if ( mask.s.port1_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port1_tx_ptpfifo_underflow_intIE = config->port1_tx_ptpfifo_underflow_intie;
	}

	if ( mask.s.port2_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port2_tx_ptpfifo_underflow_intIE = config->port2_tx_ptpfifo_underflow_intie;
	}
#endif

	if ( mask.s.port3_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port3_tx_ptpfifo_underflow_intIE = config->port3_tx_ptpfifo_underflow_intie;
	}

	if ( mask.s.port4_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port4_tx_ptpfifo_underflow_intIE = config->port4_tx_ptpfifo_underflow_intie;
	}

#ifdef CONFIG_ARCH_CORTINA_G3
	if ( mask.s.port5_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port5_tx_ptpfifo_underflow_intIE = config->port5_tx_ptpfifo_underflow_intie;
	}

	if ( mask.s.port6_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port6_tx_ptpfifo_underflow_intIE = config->port6_tx_ptpfifo_underflow_intie;
	}

	if ( mask.s.port7_tx_ptpfifo_underflow_intie ) {
		reg_data.bf.port7_tx_ptpfifo_underflow_intIE = config->port7_tx_ptpfifo_underflow_intie;
	}
#endif

	if ( mask.s.timer_rollover_intie ) {
		reg_data.bf.timer_rollover_intIE = config->timer_rollover_intie;
	}

	if ( mask.s.pps_event_intie ) {
		reg_data.bf.pps_event_intIE = config->pps_event_intie;
	}

	if ( mask.s.pps_out_falledge_intie ) {
		reg_data.bf.pps_out_falledge_intIE = config->pps_out_falledge_intie;
	}

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_GLB_PTP_INTERRUPTE,reg_data.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_glb_ptp_intr_enable_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_glb_ptp_intr_enable_cfg_t *config
	)
{

	NI_HV_GLB_PTP_INTERRUPTE_t reg_data;

	__NI_ASSERT(config);

	reg_data.wrd = 0;

	reg_data.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_PTP_INTERRUPTE);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->port0_tx_timestamp_rdy_intie = reg_data.bf.port0_tx_timestamp_rdy_intIE;
	config->port1_tx_timestamp_rdy_intie = reg_data.bf.port1_tx_timestamp_rdy_intIE;
	config->port2_tx_timestamp_rdy_intie = reg_data.bf.port2_tx_timestamp_rdy_intIE;
#endif
	config->port3_tx_timestamp_rdy_intie = reg_data.bf.port3_tx_timestamp_rdy_intIE;
	config->port4_tx_timestamp_rdy_intie = reg_data.bf.port4_tx_timestamp_rdy_intIE;
#ifdef CONFIG_ARCH_CORTINA_G3
	config->port5_tx_timestamp_rdy_intie = reg_data.bf.port5_tx_timestamp_rdy_intIE;
	config->port6_tx_timestamp_rdy_intie = reg_data.bf.port6_tx_timestamp_rdy_intIE;
	config->port7_tx_timestamp_rdy_intie = reg_data.bf.port7_tx_timestamp_rdy_intIE;
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->port0_tx_ptpfifo_overflow_intie = reg_data.bf.port0_tx_ptpfifo_overflow_intIE;
	config->port1_tx_ptpfifo_overflow_intie = reg_data.bf.port1_tx_ptpfifo_overflow_intIE;
	config->port2_tx_ptpfifo_overflow_intie = reg_data.bf.port2_tx_ptpfifo_overflow_intIE;
#endif

	config->port3_tx_ptpfifo_overflow_intie = reg_data.bf.port3_tx_ptpfifo_overflow_intIE;
	config->port4_tx_ptpfifo_overflow_intie = reg_data.bf.port4_tx_ptpfifo_overflow_intIE;
#ifdef CONFIG_ARCH_CORTINA_G3
	config->port5_tx_ptpfifo_overflow_intie = reg_data.bf.port5_tx_ptpfifo_overflow_intIE;
	config->port6_tx_ptpfifo_overflow_intie = reg_data.bf.port6_tx_ptpfifo_overflow_intIE;
	config->port7_tx_ptpfifo_overflow_intie = reg_data.bf.port7_tx_ptpfifo_overflow_intIE;
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	config->port0_tx_ptpfifo_underflow_intie = reg_data.bf.port0_tx_ptpfifo_underflow_intIE;
	config->port1_tx_ptpfifo_underflow_intie = reg_data.bf.port1_tx_ptpfifo_underflow_intIE;
	config->port2_tx_ptpfifo_underflow_intie = reg_data.bf.port2_tx_ptpfifo_underflow_intIE;
#endif
	config->port3_tx_ptpfifo_underflow_intie = reg_data.bf.port3_tx_ptpfifo_underflow_intIE;
	config->port4_tx_ptpfifo_underflow_intie = reg_data.bf.port4_tx_ptpfifo_underflow_intIE;
#ifdef CONFIG_ARCH_CORTINA_G3
	config->port5_tx_ptpfifo_underflow_intie = reg_data.bf.port5_tx_ptpfifo_underflow_intIE;
	config->port6_tx_ptpfifo_underflow_intie = reg_data.bf.port6_tx_ptpfifo_underflow_intIE;
	config->port7_tx_ptpfifo_underflow_intie = reg_data.bf.port7_tx_ptpfifo_underflow_intIE;
#endif
	config->timer_rollover_intie = reg_data.bf.timer_rollover_intIE;
	config->pps_event_intie = reg_data.bf.pps_event_intIE;
	config->pps_out_falledge_intie = reg_data.bf.pps_out_falledge_intIE;

	return CA_E_OK;
}


static ca_status_t __ni_eth_port_rx_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_boolean_t read_clear,
	CA_OUT ca_uint64_t stats[__NI_MIB_RX_NUM]
	)
{
	NI_HV_GLB_RXMIB_ACCESS_t mib_access;
	NI_HV_GLB_RXMIB_DATA1_t  mib_data1;
	NI_HV_GLB_RXMIB_DATA0_t  mib_data0;
	__ni_rx_mib_id_t         mib_id;


	for(mib_id = __NI_MIB_RX_UC_PKT_CNT; mib_id < __NI_MIB_RX_BYTE_COUNT_LO; mib_id++){
		__NI_MIB_TABLE_ACCESS(device_id,0,NI_HV_GLB_RXMIB_ACCESS,
			port_id,mib_id,
			read_clear ? __NI_MIB_OP_CODE_READ_CLEAR_ALL : __NI_MIB_OP_CODE_READ_ONLY,
			mib_access);

		/* data0 is cnt_val_lo, Valid for all counters;
		 * data1 is cnt_val_hi, Valid for Rx Byte count only.
		 */
		mib_data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_RXMIB_DATA0);

		stats[mib_id] = mib_data0.wrd;
	}

	/* Read Rx byte counters ,
	 * note:  Access to either Lo or Hi byte counter id will update both values
	 */
	__NI_MIB_TABLE_ACCESS(device_id,0,NI_HV_GLB_RXMIB_ACCESS,
		port_id,__NI_MIB_RX_BYTE_COUNT_LO,
		read_clear ? __NI_MIB_OP_CODE_READ_CLEAR_ALL : __NI_MIB_OP_CODE_READ_ONLY,
		mib_access);

	mib_data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_RXMIB_DATA1);
	mib_data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_RXMIB_DATA0);

	stats[__NI_MIB_RX_BYTE_COUNT_LO] = mib_data0.wrd;
	stats[__NI_MIB_RX_BYTE_COUNT_HI] = mib_data1.wrd;

	return CA_E_OK;

}


static ca_status_t __ni_eth_port_tx_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_boolean_t read_clear,
	CA_OUT ca_uint64_t stats[__NI_MIB_TX_NUM]
	)
{
	NI_HV_GLB_TXMIB_ACCESS_t mib_access;
	NI_HV_GLB_TXMIB_DATA1_t  mib_data1;
	NI_HV_GLB_TXMIB_DATA0_t  mib_data0;
	__ni_tx_mib_id_t         mib_id;


	for(mib_id = __NI_MIB_TX_UC_PKT_CNT; mib_id < __NI_MIB_TX_BYTE_CNT_LO; mib_id++){
		/* Skip ASIC reserved MIB index */
		if ( mib_id == __NI_MIB_TX_RSVD_12 || mib_id == __NI_MIB_TX_RSVD_23 ) {
			stats[mib_id] = 0;
			continue;
		}

		__NI_MIB_TABLE_ACCESS(device_id,0,NI_HV_GLB_TXMIB_ACCESS,
			port_id,mib_id,
			read_clear ? __NI_MIB_OP_CODE_READ_CLEAR_ALL : __NI_MIB_OP_CODE_READ_ONLY,
			mib_access);

		/* data0 is cnt_val_lo, Valid for all counters;
		  * data1 is cnt_val_hi, Valid for Tx Byte count only.
		  */
		mib_data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_TXMIB_DATA0);

		stats[mib_id] = mib_data0.wrd;
	}

	/* Read Tx byte counters ,
	 * note:  Access to either Lo or Hi byte counter id will update both values
	 */
	__NI_MIB_TABLE_ACCESS(device_id,0,NI_HV_GLB_TXMIB_ACCESS,
		port_id,__NI_MIB_TX_BYTE_CNT_LO,
		read_clear ? __NI_MIB_OP_CODE_READ_CLEAR_ALL : __NI_MIB_OP_CODE_READ_ONLY,
		mib_access);

	mib_data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_TXMIB_DATA1);
	mib_data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_TXMIB_DATA0);

	stats[__NI_MIB_TX_BYTE_CNT_LO] = mib_data0.wrd;
	stats[__NI_MIB_TX_BYTE_CNT_HI] = mib_data1.wrd;

	return CA_E_OK;

}

/* @aal_ni_eth_port_mib_get:
 * Get Ethernet port MAC MIB for RFC standard statistics.
 */
ca_status_t aal_ni_eth_port_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_boolean_t read_clear,
	CA_OUT aal_ni_mib_stats_t *stats
	)
{
	ca_status_t ret      = CA_E_OK;
	ca_uint64_t rx_stats[__NI_MIB_RX_NUM] = { 0 };
	ca_uint64_t tx_stats[__NI_MIB_RX_NUM] = { 0 };


	__NI_ASSERT(stats);
	memset(stats,0,sizeof(aal_ni_mib_stats_t));

	ret = __ni_eth_port_rx_mib_get(device_id,port_id,read_clear,rx_stats);
	if ( CA_E_OK != ret ) {
		return ret;
	}

	stats->rx_uc_pkt_cnt = rx_stats[__NI_MIB_RX_UC_PKT_CNT];
	stats->rx_mc_frame_cnt = rx_stats[__NI_MIB_RX_MC_FRM_CNT];
	stats->rx_bc_frame_cnt = rx_stats[__NI_MIB_RX_BC_FRM_CNT];
	stats->rx_oam_frame_cnt = rx_stats[__NI_MIB_RX_OAM_FRM_CNT];
	stats->rx_jumbo_frame_cnt = rx_stats[__NI_MIB_RX_JUMBO_FRM_CNT];
	stats->rx_pause_frame_cnt = rx_stats[__NI_MIB_RX_PAUSE_FRM_CNT];
	stats->rx_pfc_frame_cnt = rx_stats[__NI_MIB_RX_PFC_FRM_CNT];
	stats->rx_unk_opcode_frame_cnt = rx_stats[__NI_MIB_RX_UNK_OPCODE_FRM_CNT];
	stats->rx_crc_error_frame_cnt = rx_stats[__NI_MIB_RX_CRC_ERR_FRM_CNT];
	stats->rx_undersize_frame_cnt = rx_stats[__NI_MIB_RX_UNDER_SIZE_FRM_CNT];
	stats->rx_runt_frame_cnt = rx_stats[__NI_MIB_RX_RUNT_FRM_CNT];
	stats->rx_ovsize_frame_cnt = rx_stats[__NI_MIB_RX_OVSIZE_FRM_CNT];
	stats->rx_jabber_frame_cnt = rx_stats[__NI_MIB_RX_JABBER_FRM_CNT];
	stats->rx_invalid_frame_cnt = rx_stats[__NI_MIB_RX_INVALID_FRM_CNT];
	stats->rx_invalid_flow_frame_cnt = rx_stats[__NI_MIB_RX_INVALID_FLOW_FRM_CNT];
	stats->rx_frame_64_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_64_OCT];
	stats->rx_frame_65to127_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_65_TO_127_OCT];
	stats->rx_frame_128to255_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_128_TO_255_OCT];
	stats->rx_frame_256to511_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_256_TO_511_OCT];
	stats->rx_frame_512to1023_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_512_TO_1023_OCT];
	stats->rx_frame_1024to1518_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_1024_TO_1518_OCT];
	stats->rx_frame_1519to2100_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_1519_TO_2100_OCT];
	stats->rx_frame_2101to9200_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_2101_TO_9200_OCT];
	stats->rx_frame_9201tomax_cnt = rx_stats[__NI_MIB_RX_STATS_FRM_9201_TO_MAX_OCT];
	stats->rx_byte_cnt_low = rx_stats[__NI_MIB_RX_BYTE_COUNT_LO];
	stats->rx_byte_cnt_high = rx_stats[__NI_MIB_RX_BYTE_COUNT_HI];

	ret = __ni_eth_port_tx_mib_get(device_id,port_id,read_clear,tx_stats);
	if ( CA_E_OK != ret ) {
		return ret;
	}

	stats->tx_uc_pkt_cnt = tx_stats[__NI_MIB_TX_UC_PKT_CNT];
	stats->tx_mc_frame_cnt = tx_stats[__NI_MIB_TX_MC_FRM_CNT];
	stats->tx_bc_frame_cnt = tx_stats[__NI_MIB_TX_BC_FRM_CNT];
	stats->tx_oam_frame_cnt = tx_stats[__NI_MIB_TX_OAM_FRM_CNT];
	stats->tx_jumbo_frame_cnt = tx_stats[__NI_MIB_TX_JUMBO_FRM_CNT];
	stats->tx_pause_frame_cnt = tx_stats[__NI_MIB_TX_PAUSE_FRM_CNT];
	stats->tx_pfc_frame_cnt = tx_stats[__NI_MIB_TX_PFC_FRM_CNT];
	stats->tx_crc_error_frame_cnt = tx_stats[__NI_MIB_TX_CRCERR_FRM_CNT];
	stats->tx_ovsize_frame_cnt = tx_stats[__NI_MIB_TX_OVSIZE_FRM_CNT];
	stats->tx_single_col_frame_cnt = tx_stats[__NI_MIB_TX_SINGLE_COL_FRM_CNT];
	stats->tx_multi_col_frame_cnt = tx_stats[__NI_MIB_TX_MULTI_COL_FRM_CNT];
	stats->tx_late_col_frame_cnt = tx_stats[__NI_MIB_TX_LATE_COL_FRM_CNT];
	stats->tx_exess_col_frame_cnt = tx_stats[__NI_MIB_TX_EXESS_COL_FRM_CNT];
	stats->tx_frame_64_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_64_OCT];
	stats->tx_frame_65to127_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_65_TO_127_OCT];
	stats->tx_frame_128to255_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_128_TO_255_OCT];
	stats->tx_frame_256to511_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_256_TO_511_OCT];
	stats->tx_frame_512to1023_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_512_TO_1023_OCT];
	stats->tx_frame_1024to1518_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_1024_TO_1518_OCT];
	stats->tx_frame_1519to2100_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_1519_TO_2100_OCT];
	stats->tx_frame_2101to9200_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_2101_TO_9200_OCT];
	stats->tx_frame_9201tomax_cnt = tx_stats[__NI_MIB_TX_STATS_FRM_9201_TO_MAX_OCT];
	stats->tx_byte_cnt_low = tx_stats[__NI_MIB_TX_BYTE_CNT_LO];
	stats->tx_byte_cnt_high = tx_stats[__NI_MIB_TX_BYTE_CNT_HI];

	return ret;

}

ca_status_t aal_ni_port_tx_fifo_threshold_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_port_txfifo_threshold_mask_t mask,
	CA_IN aal_ni_port_txfifo_threshold_t *config)
{
	NI_HV_PT_TXFIFO_THLD_CFG_t reg_cfg0;

	__NI_ASSERT(config);

	reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_PT_TXFIFO_THLD_CFG + (port_id * __NI_PORT_REG_STRIDE));

	if ( mask.s.low_threshold )
		reg_cfg0.bf.low_thld = config->low_threshold;

	if ( mask.s.hi_threshold )
		reg_cfg0.bf.high_thld = config->hi_threshold;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_PT_TXFIFO_THLD_CFG + (port_id * __NI_PORT_REG_STRIDE),
		reg_cfg0.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_port_tx_fifo_threshold_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_port_txfifo_threshold_t *config)
{
	NI_HV_PT_TXFIFO_THLD_CFG_t reg_cfg0;

	__NI_ASSERT(config);

	reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_PT_TXFIFO_THLD_CFG + (port_id * __NI_PORT_REG_STRIDE));

	config->low_threshold = reg_cfg0.bf.low_thld;
	config->hi_threshold = reg_cfg0.bf.high_thld;

	return CA_E_OK;

}

ca_status_t aal_ni_eth_pfc_quanta_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint8_t cos,
	CA_IN ca_uint32_t quanta)
{
	NI_HV_PT_ETH_TXMAC_PAUSE_TIMER_CFG0_t reg_cfg0;

	__NI_PORT_CHK(device_id,port_id);
	__NI_ASSERT(cos <= 7);

	reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_PT_ETH_TXMAC_PAUSE_TIMER_CFG0 +
		(port_id * __NI_PORT_REG_STRIDE) +
		((cos >> 1) << 2));

	if ( cos & 0x1 ) {
		reg_cfg0.bf.txpfc_time_1 = quanta;
	} else {
		reg_cfg0.bf.txpfc_time_0 = quanta;
	}

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_PT_ETH_TXMAC_PAUSE_TIMER_CFG0 +
		(port_id * __NI_PORT_REG_STRIDE) +
		((cos >> 1) << 2),reg_cfg0.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_eth_pfc_quanta_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint8_t cos,
	CA_OUT ca_uint32_t *quanta)
{
	NI_HV_PT_ETH_TXMAC_PAUSE_TIMER_CFG0_t reg_cfg0;

	__NI_ASSERT(cos <= 7 && quanta != NULL);

	reg_cfg0.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_PT_ETH_TXMAC_PAUSE_TIMER_CFG0 +
		(port_id * __NI_PORT_REG_STRIDE) +
		((cos >> 1) << 2));

	if ( cos & 0x1 ) {
		*quanta = reg_cfg0.bf.txpfc_time_1;
	} else {
		*quanta = reg_cfg0.bf.txpfc_time_0;
	}

	return CA_E_OK;

}

ca_status_t aal_ni_eee_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_eee_mask_t mask,
	CA_OUT aal_ni_eee_cfg_t *eee_cfg)
{
	NI_HV_PT_EEE_LPI_CFG0_t cfg0;
	NI_HV_PT_EEE_LPI_CFG1_t cfg1;
	NI_HV_PT_EEE_LPI_CFG2_t cfg2;
	NI_HV_PT_EEE_LPI_CFG3_t cfg3;

	__NI_ASSERT(eee_cfg);
	__NI_PORT_CHK(device_id,port_id);

	if ( mask.s.tx_eee_enable || mask.s.cpu_lpi_deassert || mask.s.stop_txclk_on_lpi
	     || mask.s.lpi_data_mode || mask.s.txfifo_empty_cnt ) {

		cfg0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG0 + port_id * __NI_PORT_REG_STRIDE);
		if ( mask.s.tx_eee_enable )
			cfg0.bf.tx_eee_enable = eee_cfg->tx_eee_enable;
		if ( mask.s.cpu_lpi_deassert )
			cfg0.bf.cpu_lpi_deassert = eee_cfg->cpu_lpi_deassert;
		if ( mask.s.stop_txclk_on_lpi )
			cfg0.bf.stop_txclk_on_lpi = eee_cfg->stop_txclk_on_lpi;
		if ( mask.s.lpi_data_mode )
			cfg0.bf.lpi_data_mode = eee_cfg->lpi_data_mode;
		if ( mask.s.lpi_mode_te )
			cfg0.bf.lpi_mode_te = eee_cfg->lpi_mode_te;
		if ( mask.s.txfifo_empty_cnt )
			cfg0.bf.txfifo_empty_cnt = eee_cfg->txfifo_empty_cnt;
		CA_DEV_NI_REG_WRITE(device_id,cfg0.wrd,NI_HV_PT_EEE_LPI_CFG0 + port_id * __NI_PORT_REG_STRIDE);
	}

	if ( mask.s.lpi_min_assert_cnt ) {
		cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG1 + port_id * __NI_PORT_REG_STRIDE);
		cfg1.bf.lpi_min_assert_cnt = eee_cfg->lpi_min_assert_cnt;
		CA_DEV_NI_REG_WRITE(device_id,cfg1.wrd,NI_HV_PT_EEE_LPI_CFG1 + port_id * __NI_PORT_REG_STRIDE);
	}

	if ( mask.s.lpi_min_wakeup_cnt ) {
		cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG2 + port_id * __NI_PORT_REG_STRIDE);
		cfg2.bf.lpi_min_wakeup_cnt = eee_cfg->lpi_min_wakeup_cnt;
		CA_DEV_NI_REG_WRITE(device_id,cfg2.wrd,NI_HV_PT_EEE_LPI_CFG2 + port_id * __NI_PORT_REG_STRIDE);
	}

	if ( mask.s.lpi_mac_dis_wait_cnt || mask.s.stop_txclk_en_cnt ) {
		cfg3.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG3 + port_id * __NI_PORT_REG_STRIDE);
		if ( mask.s.lpi_mac_dis_wait_cnt )
			cfg3.bf.lpi_mac_dis_wait_cnt = eee_cfg->lpi_mac_dis_wait_cnt;
		if ( mask.s.stop_txclk_en_cnt )
			cfg3.bf.stop_txclk_en_cnt = eee_cfg->stop_txclk_en_cnt;
		CA_DEV_NI_REG_WRITE(device_id,cfg3.wrd,NI_HV_PT_EEE_LPI_CFG3 + port_id * __NI_PORT_REG_STRIDE);
	}

	return CA_E_OK;
}

ca_status_t aal_ni_eee_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_eee_cfg_t *eee_cfg)
{
	NI_HV_PT_EEE_LPI_CFG0_t cfg0;
	NI_HV_PT_EEE_LPI_CFG1_t cfg1;
	NI_HV_PT_EEE_LPI_CFG2_t cfg2;
	NI_HV_PT_EEE_LPI_CFG3_t cfg3;

	__NI_ASSERT(eee_cfg);
	__NI_PORT_CHK(device_id,port_id);

	cfg0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG0 + port_id * __NI_PORT_REG_STRIDE);
	cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG1 + port_id * __NI_PORT_REG_STRIDE);
	cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG2 + port_id * __NI_PORT_REG_STRIDE);
	cfg3.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_PT_EEE_LPI_CFG3 + port_id * __NI_PORT_REG_STRIDE);

	eee_cfg->tx_eee_enable = cfg0.bf.tx_eee_enable;
	eee_cfg->cpu_lpi_deassert = cfg0.bf.cpu_lpi_deassert;
	eee_cfg->stop_txclk_on_lpi = cfg0.bf.stop_txclk_on_lpi;
	eee_cfg->lpi_data_mode = cfg0.bf.lpi_data_mode;
	eee_cfg->lpi_mode_te = cfg0.bf.lpi_mode_te;
	eee_cfg->txfifo_empty_cnt = cfg0.bf.txfifo_empty_cnt;
	eee_cfg->lpi_min_assert_cnt = cfg1.bf.lpi_min_assert_cnt;
	eee_cfg->lpi_min_wakeup_cnt = cfg2.bf.lpi_min_wakeup_cnt;
	eee_cfg->lpi_mac_dis_wait_cnt = cfg3.bf.lpi_mac_dis_wait_cnt;
	eee_cfg->stop_txclk_en_cnt = cfg3.bf.stop_txclk_en_cnt;

	return CA_E_OK;
}


ca_status_t aal_ni_rxmux_fc_thrshld_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_rxmux_fc_thrshld_mask_t mask,
	CA_IN aal_ni_rxmux_fc_thrshld_t *cfg
	)
{
	NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_t fc_thrshld;

	__NI_ASSERT(port_id < NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_COUNT);

	__NI_ASSERT(cfg);

	fc_thrshld.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG + (port_id * NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_STRIDE));

	if ( mask.s.hi_threshold ) {
		fc_thrshld.bf.thre_hi = cfg->hi_threshold;
	}

	if ( mask.s.low_threshold ) {
		fc_thrshld.bf.thre_lo = cfg->low_threshold;
	}

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG + (port_id * NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_STRIDE),
		fc_thrshld.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_rxmux_fc_thrshld_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_rxmux_fc_thrshld_t *cfg
	)
{
	NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_t fc_thrshld;

	__NI_ASSERT(port_id < NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_COUNT);

	__NI_ASSERT(cfg);

	fc_thrshld.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG + (port_id * NI_HV_GLB_RXMUX_PORT0_FC_THRESHOLD_CFG_STRIDE));

	cfg->hi_threshold = fc_thrshld.bf.thre_hi;

	cfg->low_threshold = fc_thrshld.bf.thre_lo;

	return CA_E_OK;
}


/**********************************************************************/
/*                          XGE, XAUI                                 */
/*--------------------------------------------------------------------*/
/*              +------------------------------+                      */
/*              |          XAUI                |                      */
/*   XGE MAC <->| XAUI_MAC I/F <-> RS <-> XGXS | <-> PHY(SERDES)      */
/*              |                              |                      */
/*              +------------------------------+                      */
/**********************************************************************/

/*********************************************/
/*        10Gigabit Ethernet, XGE            */
/*********************************************/

ca_status_t aal_ni_xge_misc_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_misc_cfg_mask_t mask,
	CA_IN aal_ni_xge_misc_cfg_t *config)
{
	NI_HV_XGE_MISC_CONFIG_t cfg_data;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32,CA_E_OK);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MISC_CONFIG + port_id * __NI_XGE_REG_STRIDE);

	if ( mask.s.mode )
		cfg_data.bf.gige_mode = config->mode & 0x1;

	if ( mask.s.clk )
		cfg_data.bf.mode322 = config->clk & 0x1;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_XGE_MISC_CONFIG + port_id * __NI_XGE_REG_STRIDE,cfg_data.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_xge_misc_config_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_misc_cfg_t *config)
{
	NI_HV_XGE_MISC_CONFIG_t cfg_data;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MISC_CONFIG + port_id * __NI_XGE_REG_STRIDE);

	config->mode = cfg_data.bf.gige_mode;
	config->clk = cfg_data.bf.mode322;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_cmn_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_cmn_cfg_mask_t mask,
	CA_IN aal_ni_xge_cmn_cfg_t *config)
{
	NI_HV_XGE_MAC_CFG_COM_t cfg_com;


	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32,CA_E_OK);
	__XGE_PORT_CHK(port_id);

	cfg_com.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_CFG_COM + port_id * __NI_XGE_REG_STRIDE);

	if ( mask.s.rpr_mode )
		cfg_com.bf.rpr_mode = config->rpr_mode;
	if ( mask.s.stack_en )
		cfg_com.bf.stack_en = config->stack_en;
	if ( mask.s.clronread )
		cfg_com.bf.clronread = config->clronread;
	if ( mask.s.stats_err_order )
		cfg_com.bf.stats_err_order = config->stats_err_order;
	if ( mask.s.stats_drop_en )
		cfg_com.bf.stats_drop_en = config->stats_drop_en;
	if ( mask.s.core_tx2rx_lb )
		cfg_com.bf.core_tx2rx_lb = config->core_tx2rx_lb;
	if ( mask.s.core_rx2tx_lb )
		cfg_com.bf.core_rx2tx_lb = config->core_rx2tx_lb;
	if ( mask.s.cntr_rst )
		cfg_com.bf.cntr_rst = config->cntr_rst;
	if ( mask.s.rx_rst )
		cfg_com.bf.rx_rst = config->rx_rst;
	if ( mask.s.tx_rst )
		cfg_com.bf.tx_rst = config->tx_rst;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_XGE_MAC_CFG_COM + port_id * __NI_XGE_REG_STRIDE,cfg_com.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_xge_cmn_config_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_cmn_cfg_t *config)
{
	NI_HV_XGE_MAC_CFG_COM_t cfg_com;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg_com.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_CFG_COM + port_id * __NI_XGE_REG_STRIDE);

	config->rpr_mode = cfg_com.bf.rpr_mode;
	config->stack_en = cfg_com.bf.stack_en;
	config->clronread = cfg_com.bf.clronread;
	config->stats_err_order = cfg_com.bf.stats_err_order;
	config->stats_drop_en = cfg_com.bf.stats_drop_en;
	config->core_tx2rx_lb = cfg_com.bf.core_tx2rx_lb;
	config->core_rx2tx_lb = cfg_com.bf.core_rx2tx_lb;
	config->cntr_rst = cfg_com.bf.cntr_rst;
	config->rx_rst = cfg_com.bf.rx_rst;
	config->tx_rst = cfg_com.bf.tx_rst;

	return CA_E_OK;
}


ca_status_t aal_ni_xge_rx_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_cfg_rx_mask_t mask,
	CA_IN aal_ni_xge_cfg_rx_t *config)
{
	NI_HV_XGE_MAC_CFG_RX_t cfg_data;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32,CA_E_OK);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE);

	if ( mask.s.promis_mode )
		cfg_data.bf.promis_mode = config->promis_mode;
	if ( mask.s.drop_short )
		cfg_data.bf.drop_short = config->drop_short;
	if ( mask.s.strip )
		cfg_data.bf.strip = config->strip;
	if ( mask.s.pause_term )
		cfg_data.bf.pause_term = config->pause_term;
	if ( mask.s.pause_extract )
		cfg_data.bf.pause_extract = config->pause_extract;
	if ( mask.s.len_chk_en )
		cfg_data.bf.len_chk_en = config->len_chk_en;
	if ( mask.s.len_chk_en_pause )
		cfg_data.bf.len_chk_en_pause = config->len_chk_en_pause;
	if ( mask.s.len_chk_en_pad )
		cfg_data.bf.len_chk_en_pad = config->len_chk_en_pad;
	if ( mask.s.fcs_chk_en )
		cfg_data.bf.fcs_chk_en = config->fcs_chk_en;
	if ( mask.s.ifg_mode_rx )
		cfg_data.bf.IFG_mode_rx = config->ifg_mode_rx;
	if ( mask.s.crc_err_fatal )
		cfg_data.bf.crc_err_fatal = config->crc_err_fatal;
	if ( mask.s.maxlen_mode )
		cfg_data.bf.maxlen_mode = config->maxlen_mode;
	if ( mask.s.prmbl_sfd_gmii_err )
		cfg_data.bf.prmbl_sfd_gmii_err = config->prmbl_sfd_gmii_err;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE,cfg_data.wrd);

	return CA_E_OK;

}


ca_status_t aal_ni_xge_rx_config_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_cfg_rx_t *config)
{
	NI_HV_XGE_MAC_CFG_RX_t cfg_data;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE);

	config->promis_mode = cfg_data.bf.promis_mode;
	config->drop_short = cfg_data.bf.drop_short;
	config->strip = cfg_data.bf.strip;
	config->pause_term = cfg_data.bf.pause_term;
	config->pause_extract = cfg_data.bf.pause_extract;
	config->len_chk_en = cfg_data.bf.len_chk_en;
	config->len_chk_en_pause = cfg_data.bf.len_chk_en_pause;
	config->len_chk_en_pad = cfg_data.bf.len_chk_en_pad;
	config->fcs_chk_en = cfg_data.bf.fcs_chk_en;
	config->ifg_mode_rx = cfg_data.bf.IFG_mode_rx;
	config->crc_err_fatal = cfg_data.bf.crc_err_fatal;
	config->maxlen_mode = cfg_data.bf.maxlen_mode;
	config->prmbl_sfd_gmii_err = cfg_data.bf.prmbl_sfd_gmii_err;

	return CA_E_OK;

}


ca_status_t aal_ni_xge_tx_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_cfg_tx_mask_t mask,
	CA_IN aal_ni_xge_cfg_tx_t *config)
{
	NI_HV_XGE_MAC_CFG_TX_t     cfg1;
	NI_HV_XGE_MAC_CFG_TX_2_t   cfg2;
	NI_HV_XGE_MAC_CFG_TX_IFG_t cfg3;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32,CA_E_OK);
	__XGE_PORT_CHK(port_id);

	cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX + port_id * __NI_XGE_REG_STRIDE);
	cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX_2 + port_id * __NI_XGE_REG_STRIDE);
	cfg3.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX_IFG + port_id * __NI_XGE_REG_STRIDE);

	if ( mask.s.vlan_pad_mode )
		cfg1.bf.vlan_pad_mode = config->vlan_pad_mode;
	if ( mask.s.lf_auto_flush_en )
		cfg1.bf.lf_auto_flush_en = config->lf_auto_flush_en;
	if ( mask.s.pause_req_auto_xon )
		cfg1.bf.pause_req_auto_xon = config->pause_req_auto_xon;
	if ( mask.s.pause_req_en )
		cfg1.bf.pause_req_en = config->pause_req_en;
	if ( mask.s.insert_sa )
		cfg1.bf.insSA = config->insert_sa;
	if ( mask.s.halt )
		cfg1.bf.halt = config->halt;
	if ( mask.s.ifg_mode_tx )
		cfg1.bf.IFG_mode_tx = config->ifg_mode_tx;
	if ( mask.s.fifo1_read_thr )
		cfg2.bf.fifo1_read_thr = config->fifo1_read_thr;
	if ( mask.s.fifo1_thr_afull )
		cfg2.bf.fifo1_thr_afull = config->fifo1_thr_afull;
	if ( mask.s.ifg )
		cfg3.bf.ifg = config->ifg;

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_CFG_TX + port_id * __NI_XGE_REG_STRIDE,cfg1.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_CFG_TX_2 + port_id * __NI_XGE_REG_STRIDE,cfg2.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_CFG_TX_IFG + port_id * __NI_XGE_REG_STRIDE,cfg3.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_xge_tx_config_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_cfg_tx_t *config)
{
	NI_HV_XGE_MAC_CFG_TX_t     cfg1;
	NI_HV_XGE_MAC_CFG_TX_2_t   cfg2;
	NI_HV_XGE_MAC_CFG_TX_IFG_t cfg3;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX + port_id * __NI_XGE_REG_STRIDE);
	cfg2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX_2 + port_id * __NI_XGE_REG_STRIDE);
	cfg3.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_CFG_TX_IFG + port_id * __NI_XGE_REG_STRIDE);

	config->vlan_pad_mode = cfg1.bf.vlan_pad_mode;
	config->lf_auto_flush_en = cfg1.bf.lf_auto_flush_en;
	config->pause_req_auto_xon = cfg1.bf.pause_req_auto_xon;
	config->pause_req_en = cfg1.bf.pause_req_en;
	config->insert_sa = cfg1.bf.insSA;
	config->halt = cfg1.bf.halt;
	config->ifg_mode_tx = cfg1.bf.IFG_mode_tx;
	config->fifo1_read_thr = cfg2.bf.fifo1_read_thr;
	config->fifo1_thr_afull = cfg2.bf.fifo1_thr_afull;
	config->ifg = cfg3.bf.ifg;

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_addr_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_mac_addr_t mac)
{
	NI_HV_XGE_MAC_MAC_ADDR_A_t data_a;
	NI_HV_XGE_MAC_MAC_ADDR_B_t data_b;
	NI_HV_XGE_MAC_MAC_ADDR_C_t data_c;

	AAL_ASSERT_RET(mac,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	data_a.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_A + port_id * __NI_XGE_REG_STRIDE);
	data_b.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_B + port_id * __NI_XGE_REG_STRIDE);
	data_c.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_C + port_id * __NI_XGE_REG_STRIDE);

	data_a.bf.addr = (mac[5] << 8) | mac[4];
	data_b.bf.addr = (mac[3] << 8) | mac[2];
	data_c.bf.addr = (mac[1] << 8) | mac[0];

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_MAC_ADDR_A + port_id * __NI_XGE_REG_STRIDE,data_a.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_MAC_ADDR_B + port_id * __NI_XGE_REG_STRIDE,data_b.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_MAC_ADDR_C + port_id * __NI_XGE_REG_STRIDE,data_c.wrd);

	return CA_E_OK;

}


ca_status_t aal_ni_xge_mac_addr_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_mac_addr_t mac)
{
	NI_HV_XGE_MAC_MAC_ADDR_A_t data_a;
	NI_HV_XGE_MAC_MAC_ADDR_B_t data_b;
	NI_HV_XGE_MAC_MAC_ADDR_C_t data_c;

	AAL_ASSERT_RET(mac,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	data_a.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_A + port_id * __NI_XGE_REG_STRIDE);
	data_b.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_B + port_id * __NI_XGE_REG_STRIDE);
	data_c.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAC_ADDR_C + port_id * __NI_XGE_REG_STRIDE);

	mac[5] = (data_a.bf.addr >> 8) & 0xFF;
	mac[4] = (data_a.bf.addr >> 0) & 0xFF;
	mac[3] = (data_b.bf.addr >> 8) & 0xFF;
	mac[2] = (data_b.bf.addr >> 0) & 0xFF;
	mac[1] = (data_c.bf.addr >> 8) & 0xFF;
	mac[0] = (data_c.bf.addr >> 0) & 0xFF;

	return CA_E_OK;

}


ca_status_t aal_ni_xge_mac_vlan_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint32_t idx,
	CA_IN ca_uint16_t vlan)
{
	NI_HV_XGE_MAC_VLAN_TAG1_t vlan1;

	AAL_ASSERT_RET(idx < AAL_NI_XGE_MAC_VLAN_NUM,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	vlan1.wrd = 0;

	switch(idx){
	case 0:
		vlan1.bf.tag = vlan;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_VLAN_TAG1 + port_id * __NI_XGE_REG_STRIDE,vlan1.wrd);
		break;
	case 1:
		vlan1.bf.tag = vlan;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_VLAN_TAG2 + port_id * __NI_XGE_REG_STRIDE,vlan1.wrd);
		break;
	case 2:
		vlan1.bf.tag = vlan;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_VLAN_TAG3 + port_id * __NI_XGE_REG_STRIDE,vlan1.wrd);
		break;
	default:
		return CA_E_PARAM;
	}

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_vlan_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint32_t idx,
	CA_OUT ca_uint16_t *vlan)
{
	NI_HV_XGE_MAC_VLAN_TAG1_t vlan1;

	AAL_ASSERT_RET(idx < AAL_NI_XGE_MAC_VLAN_NUM,CA_E_PARAM);
	AAL_ASSERT_RET(vlan,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	switch(idx){
	case 0:
		vlan1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_VLAN_TAG1 + port_id * __NI_XGE_REG_STRIDE);
		break;
	case 1:
		vlan1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_VLAN_TAG2 + port_id * __NI_XGE_REG_STRIDE);
		break;
	case 2:
		vlan1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_VLAN_TAG3 + port_id * __NI_XGE_REG_STRIDE);
		break;
	default:
		return CA_E_PARAM;
	}

	*vlan = vlan1.bf.tag;

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_frame_len_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_mac_frame_len_config_mask_t mask,
	CA_IN aal_ni_xge_mac_frame_len_config_t *config)
{
	NI_HV_XGE_MAC_MINLEN_t data0;
	NI_HV_XGE_MAC_MAXLEN_t data1;
	NI_HV_XGE_MAC_CFG_RX_t  cfg_rx;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	if ( mask.s.min_len ) {
		AAL_ASSERT_RET(config->min_len < AAL_NI_XGE_MAC_MINLEN_MAX && config->min_len >= 16,CA_E_PARAM);
		data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MINLEN + port_id * __NI_XGE_REG_STRIDE);
		data0.bf.minlen = config->min_len;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_MINLEN + port_id * __NI_XGE_REG_STRIDE,data0.wrd);
	}

	if ( mask.s.max_len ) {
		AAL_ASSERT_RET(config->max_len <= AAL_NI_XGE_MAC_MAXLEN_MAX && config->max_len >= 64,CA_E_PARAM);
		data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAXLEN + port_id * __NI_XGE_REG_STRIDE);
		data1.bf.maxlen = config->max_len;
		CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_MAXLEN + port_id * __NI_XGE_REG_STRIDE,data1.wrd);
	}

	if (mask.s.maxlen_mode) {
		cfg_rx.wrd = CA_DEV_NI_REG_READ(device_id, NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE);
		cfg_rx.bf.maxlen_mode = config->maxlen_mode;
		CA_DEV_NI_REG_WRITE2(device_id, NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE, cfg_rx.wrd );
	}
	return CA_E_OK;
}

ca_status_t aal_ni_xge_mac_frame_len_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_mac_frame_len_config_t *config)
{
	NI_HV_XGE_MAC_MINLEN_t data0;
	NI_HV_XGE_MAC_MAXLEN_t data1;
	NI_HV_XGE_MAC_CFG_RX_t  cfg_rx;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MINLEN + port_id * __NI_XGE_REG_STRIDE);
	data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MAXLEN + port_id * __NI_XGE_REG_STRIDE);
	cfg_rx.wrd = CA_DEV_NI_REG_READ(device_id, NI_HV_XGE_MAC_CFG_RX + port_id * __NI_XGE_REG_STRIDE);

	config->min_len = data0.bf.minlen;
	config->max_len = data1.bf.maxlen;
	config->maxlen_mode = cfg_rx.bf.maxlen_mode;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_pause_mac_addr_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_mac_addr_t mac)
{
	NI_HV_XGE_MAC_PAUSE_ADDR_A_t data_a;
	NI_HV_XGE_MAC_PAUSE_ADDR_B_t data_b;
	NI_HV_XGE_MAC_PAUSE_ADDR_C_t data_c;

	AAL_ASSERT_RET(mac,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	data_a.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_A + port_id * __NI_XGE_REG_STRIDE);
	data_b.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_B + port_id * __NI_XGE_REG_STRIDE);
	data_c.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_C + port_id * __NI_XGE_REG_STRIDE);

	data_a.bf.addr = (mac[5] << 8) | mac[4];
	data_b.bf.addr = (mac[3] << 8) | mac[2];
	data_c.bf.addr = (mac[1] << 8) | mac[0];

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_A + port_id * __NI_XGE_REG_STRIDE,data_a.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_B + port_id * __NI_XGE_REG_STRIDE,data_b.wrd);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_C + port_id * __NI_XGE_REG_STRIDE,data_c.wrd);

	return CA_E_OK;

}

ca_status_t aal_ni_xge_pause_mac_addr_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_mac_addr_t mac)
{
	NI_HV_XGE_MAC_PAUSE_ADDR_A_t data_a;
	NI_HV_XGE_MAC_PAUSE_ADDR_B_t data_b;
	NI_HV_XGE_MAC_PAUSE_ADDR_C_t data_c;

	AAL_ASSERT_RET(mac,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	data_a.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_A + port_id * __NI_XGE_REG_STRIDE);
	data_b.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_B + port_id * __NI_XGE_REG_STRIDE);
	data_c.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_PAUSE_ADDR_C + port_id * __NI_XGE_REG_STRIDE);

	mac[5] = (data_a.bf.addr >> 8) & 0xFF;
	mac[4] = (data_a.bf.addr >> 0) & 0xFF;
	mac[3] = (data_b.bf.addr >> 8) & 0xFF;
	mac[2] = (data_b.bf.addr >> 0) & 0xFF;
	mac[1] = (data_c.bf.addr >> 8) & 0xFF;
	mac[0] = (data_c.bf.addr >> 0) & 0xFF;

	return CA_E_OK;

}
ca_status_t aal_ni_xge_pause_quanta_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint16_t quanta)
{
	NI_HV_XGE_MAC_PQUANTA_t cfg_data;

	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = 0;
	cfg_data.bf.pq = quanta;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_XGE_MAC_PQUANTA + port_id * __NI_XGE_REG_STRIDE,cfg_data.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_xge_pause_quanta_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_uint16_t *quanta)
{
	NI_HV_XGE_MAC_PQUANTA_t cfg_data;

	AAL_ASSERT_RET(quanta,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_PQUANTA + port_id * __NI_XGE_REG_STRIDE);

	*quanta = cfg_data.bf.pq;

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_pause_tx_cmd_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_boolean_t xon_xoff,
	CA_IN ca_boolean_t x_generate)
{
	NI_HV_XGE_MAC_PAUSE_TX_CMD_t cfg_data;
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = 0;
	cfg_data.bf.X_GEN = x_generate;
	cfg_data.bf.XON_XOFF = xon_xoff;

	CA_DEV_NI_REG_WRITE2(device_id,
		NI_HV_XGE_MAC_PAUSE_TX_CMD + port_id * __NI_XGE_REG_STRIDE,cfg_data.wrd);

	return CA_E_OK;
}

ca_status_t aal_ni_xge_mac_pause_tx_cmd_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_boolean_t *xon_xoff,
	CA_OUT ca_boolean_t *x_generate)
{
	NI_HV_XGE_MAC_PAUSE_TX_CMD_t cfg_data;

	AAL_ASSERT_RET(xon_xoff && x_generate,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	cfg_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_PAUSE_TX_CMD + port_id * __NI_XGE_REG_STRIDE);

	*xon_xoff = cfg_data.bf.XON_XOFF;
	*x_generate = cfg_data.bf.X_GEN;

	return CA_E_OK;
}


ca_status_t aal_ni_xge_flowctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_flowctrl_mask_t mask,
	CA_IN aal_ni_xge_flowctrl_t *config)
{
	NI_HV_GLB_XGE6_FLOW_CONTROL_CFG_t fc_cfg;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(port_id < __NI_XGE_NUM,CA_E_PARAM);

	fc_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_XGE6_FLOW_CONTROL_CFG + port_id);

	if ( mask.s.qbb_tx_en )
		fc_cfg.bf.qbb_tx_en = config->qbb_tx_en;
	if ( mask.s.qbb_rx_en )
		fc_cfg.bf.qbb_rx_en = config->qbb_rx_en;
	if ( mask.s.pau_tx_en )
		fc_cfg.bf.pau_tx_en = config->pau_tx_en;
	if ( mask.s.pau_rx_en )
		fc_cfg.bf.pau_rx_en = config->pau_rx_en;
	if ( mask.s.adjust_pause_cnt )
		fc_cfg.bf.adjust_pause_cnt = config->adjust_pause_cnt;
	if ( mask.s.qbb_qen )
		fc_cfg.bf.qbb_qen = config->qbb_qen;
	if ( mask.s.rx_drp_pau_en )
		fc_cfg.bf.rx_drp_pau_en = config->rx_drp_pau_en;
	if ( mask.s.internal_txstop_sel )
		fc_cfg.bf.internal_txstop_sel = config->internal_txstop_sel;
	if ( mask.s.tx_remheader_dis )
		fc_cfg.bf.tx_remheader_dis = config->tx_remheader_dis;
	if ( mask.s.rx_pausereq_sel )
		fc_cfg.bf.rx_pausereq_sel = config->rx_pausereq_sel;
	if ( mask.s.te_pausereq_sel )
		fc_cfg.bf.te_pausereq_sel = config->te_pausereq_sel;

	CA_DEV_NI_REG_WRITE(device_id,fc_cfg.wrd,NI_HV_GLB_XGE6_FLOW_CONTROL_CFG + port_id);

	return CA_E_OK;
}

ca_status_t aal_ni_xge_flowctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_flowctrl_t *config)
{
	NI_HV_GLB_XGE6_FLOW_CONTROL_CFG_t fc_cfg;

	AAL_ASSERT_RET(config,CA_E_PARAM);
	AAL_ASSERT_RET(port_id < __NI_XGE_NUM,CA_E_PARAM);

	fc_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_XGE6_FLOW_CONTROL_CFG + port_id );

	config->qbb_tx_en = fc_cfg.bf.qbb_tx_en;
	config->qbb_rx_en = fc_cfg.bf.qbb_rx_en;
	config->pau_tx_en = fc_cfg.bf.pau_tx_en;
	config->pau_rx_en = fc_cfg.bf.pau_rx_en;
	config->adjust_pause_cnt = fc_cfg.bf.adjust_pause_cnt;
	config->qbb_qen = fc_cfg.bf.qbb_qen;
	config->rx_drp_pau_en = fc_cfg.bf.rx_drp_pau_en;
	config->internal_txstop_sel = fc_cfg.bf.internal_txstop_sel;
	config->tx_remheader_dis = fc_cfg.bf.tx_remheader_dis;
	config->rx_pausereq_sel = fc_cfg.bf.rx_pausereq_sel;
	config->te_pausereq_sel = fc_cfg.bf.te_pausereq_sel;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_mac_stats_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN aal_ni_xge_mac_stats_id_t stats_id,
	CA_OUT ca_uint64_t *counter)
{
	NI_HV_XGE_MAC_STATS_DATA1_t data1;
	NI_HV_XGE_MAC_STATS_DATA0_t data0;

	AAL_ASSERT_RET(stats_id < AAL_XGE_MAC_STATS_ID_NUM,CA_E_PARAM);
	AAL_ASSERT_RET(counter,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_STATS_DATA1 + port_id * __NI_XGE_REG_STRIDE,0);
	CA_DEV_NI_REG_WRITE2(device_id,NI_HV_XGE_MAC_STATS_DATA0 + port_id * __NI_XGE_REG_STRIDE,0);

	CA_DEV_READ_INDIRCT_REG(device_id,stats_id,NI_HV_XGE_MAC_STATS_ACCESS + port_id * __NI_XGE_REG_STRIDE);

	data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_STATS_DATA1 + port_id * __NI_XGE_REG_STRIDE);
	data0.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_STATS_DATA0 + port_id * __NI_XGE_REG_STRIDE);

	*counter = ((ca_uint64_t)data1.bf.data << 32) | data0.bf.data;

	return CA_E_OK;
}


ca_status_t
aal_ni_xge_mac_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_mib_stats_t *stats)
{
	ca_status_t               ret      = CA_E_OK;
	ca_uint64_t               counter[AAL_XGE_MAC_STATS_ID_NUM]  = { 0 };
	aal_ni_xge_mac_stats_id_t stats_id = 0;

	__XGE_PORT_CHK(port_id);
	AAL_ASSERT_RET(stats,CA_E_PARAM);

	for(stats_id = 0; stats_id < AAL_XGE_MAC_STATS_ID_NUM; stats_id++){
		ret = aal_ni_xge_mac_stats_get(device_id,port_id,stats_id,&counter[stats_id]);
		if ( ret != CA_E_OK ) {
			ca_printf("aal_ni_xge_mac_stats_get  port %d statsid %d return %d.\n",port_id,stats_id,ret);
		}
	}

	stats->rx_uc_pkt_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_UCAST_FRAMES];
	stats->rx_mc_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_MCAST_FRAMES];
	stats->rx_bc_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_BCAST_FRAMES];

	stats->rx_oam_frame_cnt = 0;   /* Rx good EFM OAM i.e.EthType=0x8809 frames counter */
	stats->rx_jumbo_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_GIANT_FRAMES]; /* Rx good MAC layer Jumbo i.e.Type=0x8870 frames counter */
	stats->rx_pause_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_PAUSE_FRAMES]; /* Rx good MAC layer PAUSE frames counter */
	if ( port_id < __NI_XGE_NUM ) {
		NI_HV_GLB_XGE6_FLOW_CONTROL_CFG_t flow_ctl;
		flow_ctl.wrd = CA_NI_REG_READ(NI_HV_GLB_XGE6_FLOW_CONTROL_CFG + port_id);
		stats->rx_pfc_frame_cnt =
			flow_ctl.bf.qbb_rx_en ? CA_NI_REG_READ(NI_HV_GLB_XGE6_RX_FLOW_CTRL_PKT_CNT + port_id) : 0;   /* Rx good MAC layer PFC frames counter */
	}
	if ( stats->rx_pfc_frame_cnt <= stats->rx_mc_frame_cnt ) {
		stats->rx_mc_frame_cnt -= stats->rx_pfc_frame_cnt; /* ASIC MC counter includes PFC counter */
	}
	stats->rx_unk_opcode_frame_cnt = 0;    /* Rx good MAC Control frame with unknown Opcode */
	stats->rx_crc_error_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_CRC_ERR_FRAMES]; /* Rx bad CRC frame counter */
	stats->rx_undersize_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_SHORT_FRAMES]; /* Rx Undersize frame counter */
	stats->rx_runt_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_RUNT_FRAMES];  /* Rx Undersize frame with CRC error counter */
	stats->rx_ovsize_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_GIANT_FRAMES];    /* Rx Oversize Frame counter, oversize is the same with XGE's giant frames */
	stats->rx_jabber_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_JABBER_FRAMES];    /* Rx Oversize Frame with CRC error counter */
	stats->rx_invalid_frame_cnt = 0;   /* Rx invalid frames due to length exceed max storable size or internal FIFO overrun condition */
	stats->rx_invalid_flow_frame_cnt = 0;  /* Rx invalid flow frames. Invalid flow frames are pause packet with matching Ethnet Type and opcode field but length not equal to 64 */
	stats->rx_frame_64_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_64B_FRAMES];    /* Rx received good frames with size 64 bytes */
	stats->rx_frame_65to127_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_65B_TO_127B_FRAMES];   /* Rx received good frames with size 65-127 bytes */
	stats->rx_frame_128to255_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_128B_TO_255B_FRAMES];  /* Rx received good frames with size 128-255 bytes */
	stats->rx_frame_256to511_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_256B_TO_511B_FRAMES];  /* Rx received good frames with size 256-511 bytes */
	stats->rx_frame_512to1023_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_512B_TO_1023B_FRAMES]; /* Rx received good frames with size 512-1023 bytes */
	stats->rx_frame_1024to1518_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_1024B_TO_1518B_FRAMES];    /* Rx received good frames with size 1024-1518 bytes */
	stats->rx_frame_1519to2100_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_1519B_TO_MAXB_FRAMES];    /* Rx received good frames with size 1519-2100 bytes */
	stats->rx_frame_2101to9200_cnt = 0;    /* Rx received good frames with size 2101-9200 bytes */
	stats->rx_frame_9201tomax_cnt = 0; /* Rx received good frames with size 9201-Max bytes */
	stats->rx_byte_cnt_low = counter[AAL_XGE_MAC_STATS_ID_RX_OCTETS];    /* Rx byte counter low word */
	stats->rx_byte_cnt_high = 0;   /* Rx byte counter high word */

	stats->tx_uc_pkt_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_UCAST_FRAMES];  /* Tx good unicast frame counter */
	stats->tx_mc_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_MCAST_FRAMES];    /* Tx good multicast frames counter */
	stats->tx_bc_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_BCAST_FRAMES];    /* Tx good broadcast frames counter */
	stats->tx_oam_frame_cnt = 0;   /* Tx EFM OAM frames counter */
	stats->tx_jumbo_frame_cnt = 0; /* Tx MAC Jumbo i.e.EthType=0x8870 frames counter */
	stats->tx_pause_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_PAUSE_FRAMES]; /* Tx MAC layer PAUSE frames counter */
	stats->tx_pfc_frame_cnt = 0;   /* Tx MAC layer PFC frames counter */
	if ( port_id < __NI_XGE_NUM ) {
		NI_HV_GLB_XGE6_FLOW_CONTROL_CFG_t flow_ctl;
		flow_ctl.wrd = CA_NI_REG_READ(NI_HV_GLB_XGE6_FLOW_CONTROL_CFG + port_id);
		stats->tx_pfc_frame_cnt =
			flow_ctl.bf.qbb_tx_en ? CA_NI_REG_READ(NI_HV_GLB_XGE6_TX_FLOW_CTRL_PKT_CNT + port_id) : 0;   /* Rx good MAC layer PFC frames counter */
	}

	if ( stats->tx_pfc_frame_cnt <= stats->tx_mc_frame_cnt ) {
		stats->tx_mc_frame_cnt -= stats->tx_pfc_frame_cnt; /* ASIC MC counter includes PFC counter */
	}

	stats->tx_crc_error_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_ERR_FRAMES]; /* Tx bad CRC frame counter */
	stats->tx_ovsize_frame_cnt = 0;    /* Tx Oversize Frame counter */
	stats->tx_single_col_frame_cnt = 0;    /* Count of transmitted frames with exactly one collision */
	stats->tx_multi_col_frame_cnt = 0; /* Count of transmitted frames with more than one collision  */
	stats->tx_late_col_frame_cnt = 0;  /* Count of frames in which late collision was detected */
	stats->tx_err_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_ERR_FRAMES];  /* Transmit Error Frames (MIB) */
	stats->tx_exess_col_frame_cnt = 0; /* Count of frames with excess collisions */
	stats->tx_frame_64_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_64B_FRAMES];    /* Tx transmitted good frames with size 64 bytes */
	stats->tx_frame_65to127_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_65B_TO_127B_FRAMES];   /* Tx transmitted good frames with size 65-127 bytes */
	stats->tx_frame_128to255_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_128B_TO_255B_FRAMES];  /* Tx transmitted good frames with size 128-255 bytes */
	stats->tx_frame_256to511_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_256B_TO_511B_FRAMES];  /* Tx transmitted good frames with size 256-511 bytes */
	stats->tx_frame_512to1023_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_512B_TO_1023B_FRAMES]; /* Tx transmitted good frames with size 512-1023 bytes */
	stats->tx_frame_1024to1518_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_1024B_TO_1518B_FRAMES];    /* Tx transmitted good frames with size 1024-1518 bytes */
	stats->tx_frame_1519to2100_cnt = counter[AAL_XGE_MAC_STATS_ID_TX_1519B_FRAMES];    /* Tx transmitted good frames with size 1519-2100 bytes */
	stats->tx_frame_2101to9200_cnt = 0;    /* Tx transmitted good frames with size 2101-9200 bytes */
	stats->tx_frame_9201tomax_cnt = 0; /* Tx transmitted good frames with size 9201-Max. */
	stats->tx_byte_cnt_low = counter[AAL_XGE_MAC_STATS_ID_TX_OCTETS];    /* Tx byte counter low word */
	stats->tx_byte_cnt_high = 0;   /* Tx byte counter high word */

	stats->rx_align_err_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_ALIGN_ERR_FRAMES];
	stats->rx_short_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_SHORT_FRAMES];
	stats->rx_giant_frame_cnt = 0;
	stats->rx_err_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_RX_ERR_FRAMES];
	stats->drop_rx_crc_err_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_DROP_RX_CRC_ERR_FRAMES];
	stats->drop_rx_align_err_frame_cnt = counter[AAL_XGE_MAC_STATS_ID_DROP_RX_ALIGN_ERR_FRAMES];

	return ret;
}

static ca_boolean_t __chk_pcs_enable(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id)
{
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	intf_rst_config.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTF_RST_CONFIG);

	if ( port_id == 0 && intf_rst_config.bf.pcs_rst_p5 ) {
		return FALSE;
	}

	return TRUE;
}

ca_status_t aal_ni_xge_sxgra_sxgsts_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_boolean_t *linkup)
{
	NI_HV_SXGMII_SXGRA_SXGSTS_t sxgsts;

	__XGE_PORT_CHK(port_id);
	AAL_ASSERT_RET(linkup,CA_E_PARAM);

	if ( !__chk_pcs_enable(device_id,port_id) ) {
		ca_printf("dev %d port %d PCS is not ready\n",device_id,port_id);
		return CA_E_UNAVAIL;
	}

	sxgsts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGSTS + port_id * __NI_XSMII_REG_STRIDE);
	*linkup = sxgsts.bf.linkup;

	return CA_E_OK;
}


ca_status_t aal_ni_xge_sxgra_sxgspeed_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_mac_speed_t *speed)
{
	NI_HV_SXGMII_SXGRA_SXGHWANSTS_t sxgsts;

	__XGE_PORT_CHK(port_id);
	AAL_ASSERT_RET(speed,CA_E_PARAM);

	sxgsts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANSTS + port_id * __NI_XSMII_REG_STRIDE);
	*speed = sxgsts.bf.hwanlinkspeed;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_prbs_test_set(ca_device_id_t device_id, ca_uint32_t port_id, ca_uint32_t enable)
{
	NI_HV_SXGMII_PCS_PCSTSTPTRNERRCNTR_t err_cnt;

	__XGE_PORT_CHK(port_id);

	if ( !enable ) {
		CA_DEV_NI_REG_WRITE(device_id,0x00,NI_HV_SXGMII_PCS_PCSTSTPTRNCTL + port_id * __NI_XSMII_REG_STRIDE);
		CA_DEV_NI_REG_WRITE(device_id,0x00000002,NI_HV_SXGMII_PCS_PCSREG32768 + port_id * __NI_XSMII_REG_STRIDE);
		CA_DEV_NI_REG_WRITE(device_id,0x00005a5a,NI_HV_SXGMII_PCS_PCSREG32783 + port_id * __NI_XSMII_REG_STRIDE);
	} else {
		err_cnt.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSTSTPTRNERRCNTR + port_id * __NI_XSMII_REG_STRIDE);
		CA_DEV_NI_REG_WRITE(device_id,0x3c,NI_HV_SXGMII_PCS_PCSTSTPTRNCTL + port_id * __NI_XSMII_REG_STRIDE);
		CA_DEV_NI_REG_WRITE(device_id,0x622,NI_HV_SXGMII_PCS_PCSREG32768 + port_id * __NI_XSMII_REG_STRIDE);
		CA_DEV_NI_REG_WRITE(device_id,0x5a2,NI_HV_SXGMII_PCS_PCSREG32783 + port_id * __NI_XSMII_REG_STRIDE);
		ca_printf("Start PRBS test, it will take a more than 5 seconds, please wait ...\n");
	}

	return CA_E_OK;
}

ca_status_t aal_ni_xge_prbs_test_get(ca_device_id_t device_id, ca_uint32_t port_id, ca_uint32_t *enable)
{
	NI_HV_SXGMII_PCS_PCSTSTPTRNCTL_t     ctl;
	NI_HV_SXGMII_PCS_PCSTSTPTRNERRCNTR_t err_cnt;

	__XGE_PORT_CHK(port_id);

	ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSTSTPTRNCTL + port_id * __NI_XSMII_REG_STRIDE);
	err_cnt.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSTSTPTRNERRCNTR + port_id * __NI_XSMII_REG_STRIDE);
	if ( ctl.wrd ) {
		ca_printf("The PRBS test result, error counter: %u\n",err_cnt.wrd);
		*enable = TRUE;
	} else {
		*enable = FALSE;
	}

	return CA_E_OK;
}

ca_status_t aal_ni_xge_pcs_status_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT aal_ni_xge_pcs_status_t *sts)
{
	NI_HV_SXGMII_PCS_PCSSTATUS1_t data1;
	NI_HV_SXGMII_PCS_PCSSTATUS2_t data2;

	__XGE_PORT_CHK(port_id);
	AAL_ASSERT_RET(sts,CA_E_PARAM);

	if ( !__chk_pcs_enable(device_id,port_id) ) {
		ca_printf("dev %d port %d PCS is not ready\n",device_id,port_id);
		return CA_E_UNAVAIL;
	}

	data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTATUS1 + port_id * __NI_XSMII_REG_STRIDE);
	data2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTATUS2 + port_id * __NI_XSMII_REG_STRIDE);

	data1.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTATUS1 + port_id * __NI_XSMII_REG_STRIDE);
	data2.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTATUS2 + port_id * __NI_XSMII_REG_STRIDE);

	sts->lopwrable = data1.bf.lopwrable;
	sts->rxlinksts = data1.bf.rxlinksts;
	sts->fault = data1.bf.fault;
	sts->rxlpiind = data1.bf.rxlpiind;
	sts->txlpiind = data1.bf.txlpiind;
	sts->rxlpircvd = data1.bf.rxlpircvd;
	sts->txlpircvd = data1.bf.txlpircvd;

	sts->rxfault = data2.bf.rxfault;
	sts->txfault = data2.bf.txfault;
	sts->devprsnt = data2.bf.devprsnt;

	return CA_E_OK;

}



ca_status_t aal_ni_xge_mac_intr_en_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_IN ca_uint32_t en_bmp)
{
	NI_HV_XGE_MAC_MISC_INTENABLE_t misc_inte;

	__XGE_PORT_CHK(port_id);

	if ( en_bmp & AAL_XGE_MAC_INTR_EN_BMP_RX_NO_SFD )
		misc_inte.bf.rx_no_sfdE = 1;
	else
		misc_inte.bf.rx_no_sfdE = 0;

	if ( en_bmp & AAL_XGE_MAC_INTR_EN_BMP_TX_PAUSE_DONE )
		misc_inte.bf.tx_pause_doneE = 1;
	else
		misc_inte.bf.tx_pause_doneE = 0;

	if ( en_bmp & AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_OVERFLOW )
		misc_inte.bf.tx_fifo1_oflwE = 1;
	else
		misc_inte.bf.tx_fifo1_oflwE = 0;

	if ( en_bmp & AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_UNDERFLOW )
		misc_inte.bf.tx_fifo1_uflwE = 1;
	else
		misc_inte.bf.tx_fifo1_uflwE = 0;

	CA_DEV_NI_REG_WRITE(device_id,misc_inte.wrd,
		NI_HV_XGE_MAC_MISC_INTENABLE + port_id * __NI_XGE_REG_STRIDE);

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_intr_en_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_uint32_t *en_bmp)
{
	NI_HV_XGE_MAC_MISC_INTENABLE_t misc_inte;

	AAL_ASSERT_RET(en_bmp,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	misc_inte.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MAC_MISC_INTENABLE + port_id * __NI_XGE_REG_STRIDE);


	if ( misc_inte.bf.rx_no_sfdE )
		*en_bmp |= AAL_XGE_MAC_INTR_EN_BMP_RX_NO_SFD;

	if ( misc_inte.bf.tx_pause_doneE )
		*en_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_PAUSE_DONE;

	if ( misc_inte.bf.tx_fifo1_oflwE )
		*en_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_OVERFLOW;

	if ( misc_inte.bf.tx_fifo1_uflwE )
		*en_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_UNDERFLOW;

	return CA_E_OK;

}

ca_status_t aal_ni_xge_mac_intr_status_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port_id,
	CA_OUT ca_uint32_t *status_bmp)
{
	NI_HV_XGE_MAC_MISC_INTERRUPT_t intr_data;

	AAL_ASSERT_RET(status_bmp,CA_E_PARAM);
	__XGE_PORT_CHK(port_id);

	intr_data.wrd = CA_DEV_NI_REG_READ(device_id,
		NI_HV_XGE_MAC_MISC_INTERRUPT + port_id * __NI_XGE_REG_STRIDE);

	*status_bmp = 0;

	if ( intr_data.bf.rx_no_sfdI )
		*status_bmp |= AAL_XGE_MAC_INTR_EN_BMP_RX_NO_SFD;

	if ( intr_data.bf.tx_pause_doneI )
		*status_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_PAUSE_DONE;

	if ( intr_data.bf.tx_fifo1_oflwI )
		*status_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_OVERFLOW;

	if ( intr_data.bf.tx_fifo1_uflwI )
		*status_bmp |= AAL_XGE_MAC_INTR_EN_BMP_TX_FIFO_UNDERFLOW;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_pfc_quanta_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port,
	CA_IN aal_ni_xge_pfc_quanta_mask_t mask,
	CA_IN aal_ni_xge_pfc_quanta_t *config)
{
	ca_uint32_t                 logic_port   = 0;
	NI_HV_GLB_XGE6_TX_QUANTA0_t que01_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA1_t que23_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA2_t que45_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA3_t que67_quanta;

	if ( port != 5 || NULL == config ) {
		return CA_E_PARAM;
	}

	logic_port = port - 5;

	que01_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA0 + logic_port);
	que23_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA1 + logic_port);
	que45_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA2 + logic_port);
	que67_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA3 + logic_port);

	if ( mask.s.xge_quanta_que0 ) {
		que01_quanta.bf.pause_quanta0 = config->xge_quanta_que0;
	}
	if ( mask.s.xge_quanta_que1 ) {
		que01_quanta.bf.pause_quanta1 = config->xge_quanta_que1;
	}
	if ( mask.s.xge_quanta_que2 ) {
		que23_quanta.bf.pause_quanta2 = config->xge_quanta_que2;
	}
	if ( mask.s.xge_quanta_que3 ) {
		que23_quanta.bf.pause_quanta3 = config->xge_quanta_que3;
	}
	if ( mask.s.xge_quanta_que4 ) {
		que45_quanta.bf.pause_quanta4 = config->xge_quanta_que4;
	}
	if ( mask.s.xge_quanta_que5 ) {
		que45_quanta.bf.pause_quanta5 = config->xge_quanta_que5;
	}
	if ( mask.s.xge_quanta_que6 ) {
		que67_quanta.bf.pause_quanta6 = config->xge_quanta_que6;
	}
	if ( mask.s.xge_quanta_que7 ) {
		que67_quanta.bf.pause_quanta7 = config->xge_quanta_que7;
	}

	CA_DEV_NI_REG_WRITE(device_id,que01_quanta.wrd,NI_HV_GLB_XGE6_TX_QUANTA0 + logic_port);
	CA_DEV_NI_REG_WRITE(device_id,que23_quanta.wrd,NI_HV_GLB_XGE6_TX_QUANTA1 + logic_port);
	CA_DEV_NI_REG_WRITE(device_id,que45_quanta.wrd,NI_HV_GLB_XGE6_TX_QUANTA2 + logic_port);
	CA_DEV_NI_REG_WRITE(device_id,que67_quanta.wrd,NI_HV_GLB_XGE6_TX_QUANTA3 + logic_port);

	return CA_E_OK;
}

ca_status_t aal_ni_xge_pfc_quanta_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t port,
	CA_OUT aal_ni_xge_pfc_quanta_t *config)
{
	ca_uint32_t                        logic_port   = 0;
	NI_HV_GLB_XGE6_TX_QUANTA0_t        que01_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA1_t        que23_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA2_t        que45_quanta;
	NI_HV_GLB_XGE6_TX_QUANTA3_t        que67_quanta;

	if ( (port < __XE_PORT_START) || (port > __XE_PORT_END) || (NULL == config) ) {
		return CA_E_PARAM;
	}

	logic_port = port - __XE_PORT_START;
	que01_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA0 + logic_port);
	que23_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA1 + logic_port);
	que45_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA2 + logic_port);
	que67_quanta.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_XGE6_TX_QUANTA3 + logic_port);
	config->xge_quanta_que0 = que01_quanta.bf.pause_quanta0;
	config->xge_quanta_que1 = que01_quanta.bf.pause_quanta1;
	config->xge_quanta_que2 = que23_quanta.bf.pause_quanta2;
	config->xge_quanta_que3 = que23_quanta.bf.pause_quanta3;
	config->xge_quanta_que4 = que45_quanta.bf.pause_quanta4;
	config->xge_quanta_que5 = que45_quanta.bf.pause_quanta5;
	config->xge_quanta_que6 = que67_quanta.bf.pause_quanta6;
	config->xge_quanta_que7 = que67_quanta.bf.pause_quanta7;

	return CA_E_OK;
}

ca_status_t aal_ni_xge_eee_cfg_set(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_uint32_t            port_id,
    CA_IN  aal_ni_xge_eee_mask_t  mask,
    CA_IN aal_ni_xge_eee_cfg_t   *eee_cfg)
{
    NI_HV_SXGMII_PCS_PCSREG32771_t  sxgmii_pcs;

    if (port_id < AAL_NI_XE_START || port_id > AAL_NI_XE_END || eee_cfg == NULL) {
        return CA_E_PARAM;
    }

    if (0 == mask.u32)
        return CA_E_OK;

    sxgmii_pcs.wrd = CA_DEV_NI_REG_READ(device_id, 
        NI_HV_SXGMII_PCS_PCSREG32771 + (port_id - AAL_NI_XE_START) * __NI_XSMII_REG_STRIDE);
    
    if (mask.s.eee_enable)
        sxgmii_pcs.bf.eee_en = eee_cfg->eee_enable;
    if (mask.s.tx_tql_val)
        sxgmii_pcs.bf.tx_tql_val = eee_cfg->tx_tql_val;

    CA_DEV_NI_REG_WRITE(device_id, sxgmii_pcs.wrd,
        NI_HV_SXGMII_PCS_PCSREG32771 + (port_id - AAL_NI_XE_START) * __NI_XSMII_REG_STRIDE);
	
    return CA_E_OK;

}

ca_status_t aal_ni_xge_eee_cfg_get(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_uint32_t            port_id,
    CA_OUT aal_ni_xge_eee_cfg_t  *eee_cfg)
{
    NI_HV_SXGMII_PCS_PCSREG32771_t  sxgmii_pcs;

    if (port_id < AAL_NI_XE_START || port_id > AAL_NI_XE_END) {
        return CA_E_PARAM;
    }

    sxgmii_pcs.wrd = CA_DEV_NI_REG_READ(device_id, 
        NI_HV_SXGMII_PCS_PCSREG32771 + (port_id - AAL_NI_XE_START) * __NI_XSMII_REG_STRIDE);

    eee_cfg->eee_enable = sxgmii_pcs.bf.eee_en;
    eee_cfg->tx_tql_val = sxgmii_pcs.bf.tx_tql_val;

    return CA_E_OK;

}

ca_status_t aal_ni_xge_sxgra_sxg_intr_cfg_set(
	CA_IN  ca_device_id_t                       device_id,
	CA_IN  ca_uint32_t                          port,
	CA_IN  aal_ni_xge_sxgra_sxg_intr_cfg_mask_t mask,
	CA_IN  aal_ni_xge_sxgra_sxg_intr_cfg_t      *config)
{
	NI_HV_SXGMII_SXGRA_SXG_INTENABLE_t sxg_inte;

	__XGE_PORT_CHK(port);
	AAL_ASSERT_RET(NULL != config, CA_E_PARAM);

	sxg_inte.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + port * __NI_XSMII_REG_STRIDE);

	if ( mask.s.xfipcs) {
		sxg_inte.bf.xfipcs_intE = config->xfipcs;
	}

	if ( mask.s.xfipcsrxerr) {
		sxg_inte.bf.xfipcsrxerr_intE = config->xfipcsrxerr;
	}

	if ( mask.s.xfipcslnkfault) {
		sxg_inte.bf.xfipcslnkfault_intE = config->xfipcslnkfault;
	}

	if ( mask.s.xfipcslnkfault) {
		sxg_inte.bf.xfipcslnkfault_intE = config->xfipcslnkfault;
	}

	if ( mask.s.hwancomplete ) {
		sxg_inte.bf.hwancomplete_intE = config->hwancomplete;
	}

	if ( mask.s.anidledet ) {
		sxg_inte.bf.anidledet_intE = config->anidledet;
	}

	if ( mask.s.ancfgwdnulldet ) {
		sxg_inte.bf.ancfgwdnulldet_intE = config->ancfgwdnulldet;
	}

	if ( mask.s.ancfgwdabdet ) {
		sxg_inte.bf.ancfgwdabdet_intE = config->ancfgwdabdet;
	}

	if ( mask.s.ancfgwddet ) {
		sxg_inte.bf.ancfgwddet_intE = config->ancfgwddet;
	}

	if ( mask.s.linkup ) {
		sxg_inte.bf.linkup_intE = config->linkup;
	}

	CA_DEV_NI_REG_WRITE(
		device_id,
		sxg_inte.wrd,
		NI_HV_SXGMII_SXGRA_SXG_INTENABLE + port * __NI_XSMII_REG_STRIDE);
	ca_mdelay(1);
	
	return CA_E_OK;
}

ca_status_t aal_ni_xge_sxgra_sxg_intr_cfg_get(
	CA_IN  ca_device_id_t                   device_id,
	CA_IN  ca_uint32_t                      port,
	CA_OUT aal_ni_xge_sxgra_sxg_intr_cfg_t  *config)
{
	NI_HV_SXGMII_SXGRA_SXG_INTENABLE_t sxg_inte;

	__XGE_PORT_CHK(port);
	AAL_ASSERT_RET(NULL != config, CA_E_PARAM);

	sxg_inte.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + port * __NI_XSMII_REG_STRIDE);

	config->xfipcs         = sxg_inte.bf.xfipcs_intE;
	config->xfipcsrxerr    = sxg_inte.bf.xfipcsrxerr_intE;
	config->xfipcslnkfault = sxg_inte.bf.xfipcslnkfault_intE;
	config->hwancomplete   = sxg_inte.bf.hwancomplete_intE;
	config->anidledet      = sxg_inte.bf.anidledet_intE;
	config->ancfgwdnulldet = sxg_inte.bf.ancfgwdnulldet_intE;
	config->ancfgwdabdet   = sxg_inte.bf.ancfgwdabdet_intE;
	config->ancfgwddet     = sxg_inte.bf.ancfgwddet_intE;
	config->linkup         = sxg_inte.bf.linkup_intE;
	config->rxpfptradjerr  = sxg_inte.bf.rxpfptradjerr_intE;
	config->rxxgmfptradjerr= sxg_inte.bf.rxxgmfptradjerr_intE;
	config->rxgmfptradjerr = sxg_inte.bf.rxgmfptradjerr_intE;
	config->txpfptradjerr  = sxg_inte.bf.txpfptradjerr_intE;
	config->txxgmfptradjerr= sxg_inte.bf.txxgmfptradjerr_intE;
	config->txgmfptradjerr = sxg_inte.bf.txgmfptradjerr_intE;
	
	return CA_E_OK;
}

ca_status_t aal_ni_xge_sxgra_intr_status_set (
	CA_IN  ca_device_id_t                  device_id,
	CA_IN  ca_uint32_t                     port,
	CA_IN  aal_ni_xge_sxgra_sxg_intr_status_t *status)
{
	NI_HV_SXGMII_SXGRA_SXG_INTERRUPT_t intstat;

	__XGE_PORT_CHK(port);
	AAL_ASSERT_RET(NULL != status, CA_E_PARAM);

	intstat.wrd = 0;

	intstat.bf.xfipcs_intI          = status->xfipcs;
	intstat.bf.xfipcsrxerr_IntI     = status->xfipcsrxerr;
	intstat.bf.xfipcslnkfault_IntI  = status->xfipcslnkfault;
	intstat.bf.hwancomplete_intI    = status->hwancomplete;
	intstat.bf.anidledet_intI       = status->anidledet;
	intstat.bf.ancfgwdabdet_intI    = status->ancfgwdabdet;
	intstat.bf.ancfgwdnulldet_intI  = status->ancfgwdnulldet;
	intstat.bf.ancfgwddet_intI      = status->ancfgwddet;
	intstat.bf.linkup_intI          = status->linkup;
	intstat.bf.rxpfptradjerr_intI   = status->rxpfptradjerr;
	intstat.bf.rxxgmfptradjerr_intI = status->rxxgmfptradjerr;
	intstat.bf.rxgmfptradjerr_intI  = status->rxgmfptradjerr;
	intstat.bf.txpfptradjerr_intI   = status->txpfptradjerr;
	intstat.bf.txxgmfptradjerr_intI = status->txxgmfptradjerr;
	intstat.bf.txgmfptradjerr_intI  = status->txgmfptradjerr;

	ca_printf("instat %#x, linkup %u, status %#x\n",intstat.wrd,status->linkup, *status);
	CA_DEV_NI_REG_WRITE (device_id, intstat.wrd, NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + port * __NI_XSMII_REG_STRIDE);

	return CA_E_OK;
}

ca_status_t aal_ni_xge_sxgra_intr_status_get(
	CA_IN  ca_device_id_t                     device_id,
	CA_IN  ca_uint32_t                        port,
	CA_OUT aal_ni_xge_sxgra_sxg_intr_status_t *status)
{
	NI_HV_SXGMII_SXGRA_SXG_INTERRUPT_t intstat;

	__XGE_PORT_CHK(port);
	AAL_ASSERT_RET(NULL != status,CA_E_PARAM);

	intstat.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + port * __NI_XSMII_REG_STRIDE);

	status->xfipcs      = intstat.bf.xfipcs_intI;
	status->xfipcsrxerr = intstat.bf.xfipcsrxerr_IntI;

	status->xfipcslnkfault = intstat.bf.xfipcslnkfault_IntI;
	status->hwancomplete = intstat.bf.hwancomplete_intI;
	status->anidledet    = intstat.bf.anidledet_intI;
	status->ancfgwdabdet = intstat.bf.ancfgwdabdet_intI;
	status->ancfgwdnulldet = intstat.bf.ancfgwdnulldet_intI;
	status->ancfgwddet     = intstat.bf.ancfgwddet_intI;
	status->linkup         = intstat.bf.linkup_intI;
	status->rxpfptradjerr   = intstat.bf.rxpfptradjerr_intI;
	status->rxxgmfptradjerr = intstat.bf.rxxgmfptradjerr_intI;
	status->rxgmfptradjerr  = intstat.bf.rxgmfptradjerr_intI;
	status->txpfptradjerr   = intstat.bf.txpfptradjerr_intI;
	status->txxgmfptradjerr = intstat.bf.txxgmfptradjerr_intI;
	status->txgmfptradjerr  = intstat.bf.txgmfptradjerr_intI;
	
	return CA_E_OK;

}
/*********************************************/
/* 10Gigabit Attachment Unit Interface, XAUI */
/*********************************************/

ca_status_t aal_ni_xaui_mac_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_mask_t mask,
	CA_IN aal_ni_xaui_mac_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_CFG_t reg_data;

	AAL_ASSERT_RET(mask.u32, CA_E_OK);
	AAL_ASSERT_RET(config, CA_E_PARAM);

	reg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_CFG);

	if (mask.s.rs_rmtfault_en)
	reg_data.bf.rs_rmtfault_en = config->rs_rmtfault_en;
	if (mask.s.rs_locfault_en)
	reg_data.bf.rs_locfault_en = config->rs_locfault_en;
	if (mask.s.tx2rx_lpbk_en)
	reg_data.bf.tx2rx_lpbk_en = config->tx2rx_lpbk_en;
	if (mask.s.rx2tx_lpbk_en)
	reg_data.bf.rx2tx_lpbk_en = config->rx2tx_lpbk_en;

	CA_NI_REG_WRITE( reg_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_CFG);
#endif

	return CA_E_OK;

}

ca_status_t aal_ni_xaui_mac_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_t *config)
{
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_CFG_t reg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	reg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_CFG);

	config->rs_rmtfault_en = reg_data.bf.rs_rmtfault_en;
	config->rs_locfault_en = reg_data.bf.rs_locfault_en;
	config->tx2rx_lpbk_en = reg_data.bf.tx2rx_lpbk_en;
	config->rx2tx_lpbk_en = reg_data.bf.rx2tx_lpbk_en;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_tx_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_tx_cfg_mask_t mask,
	CA_IN aal_ni_xaui_mac_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG_t reg_data;

	AAL_ASSERT_RET(mask.u32, CA_E_OK);
	AAL_ASSERT_RET(config, CA_E_PARAM);

	reg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG);

	if (mask.s.insert_cshead )
	reg_data.bf.insert_cshead = config->insert_cshead;
	if (mask.s.reset )
	reg_data.bf.reset = config->reset;
	if (mask.s.flow_en )
	reg_data.bf.flow_en = config->flow_en;
	if (mask.s.mac_en )
	reg_data.bf.mac_en = config->mac_en;
	if (mask.s.crc_recaculate_en )
	reg_data.bf.crc_recaculate_en = config->crc_recaculate_en;
	if (mask.s.swap_crc8_en )
	reg_data.bf.swap_crc8_en = config->swap_crc8_en;
	if (mask.s.swap_datain_en )
	reg_data.bf.swap_datain_en = config->swap_datain_en;
	if (mask.s.dic_mode )
	reg_data.bf.dic_mode = config->dic_mode;
	if (mask.s.link_fault_en )
	reg_data.bf.link_fault_en = config->link_fault_en;
	if (mask.s.insert_nop )
	reg_data.bf.insert_nop = config->insert_nop;

	CA_NI_REG_WRITE( reg_data.wrd, NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG);
#endif

	return CA_E_OK;

}

ca_status_t aal_ni_xaui_mac_tx_config_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG_t reg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	reg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG);

	config->insert_cshead = reg_data.bf.insert_cshead;
	config->reset = reg_data.bf.reset;
	config->flow_en = reg_data.bf.flow_en;
	config->mac_en = reg_data.bf.mac_en;
	config->crc_recaculate_en = reg_data.bf.crc_recaculate_en;
	config->swap_crc8_en = reg_data.bf.swap_crc8_en;
	config->swap_datain_en = reg_data.bf.swap_datain_en;
	config->dic_mode = reg_data.bf.dic_mode;
	config->link_fault_en = reg_data.bf.link_fault_en;
	config->insert_nop = reg_data.bf.insert_nop;
#endif

	return CA_E_OK;
}



ca_status_t aal_ni_xaui_mac_rx_config_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_rx_cfg_mask_t mask,
	CA_IN aal_ni_xaui_mac_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG_t cfg_data;
	NI_HV_XAUI_XAUI_MAC_RXMAC_RX_PKT_DET_t det_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG);
	det_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_RXMAC_RX_PKT_DET);

	if (mask.s.reset )
	cfg_data.bf.reset = config->reset;
	if (mask.s.dxaui_quanta_mode )
	cfg_data.bf.dxaui_quanta_mode = config->dxaui_quanta_mode;
	if (mask.s.swap_crc8_en )
	cfg_data.bf.swap_crc8_en = config->swap_crc8_en;
	if (mask.s.swap_datain_en )
	cfg_data.bf.swap_datain_en = config->swap_datain_en;
	if (mask.s.mac_en )
	cfg_data.bf.mac_en = config->mac_en;
	if (mask.s.pause_en )
	cfg_data.bf.pause_en = config->pause_en;
	if (mask.s.max_size )
	cfg_data.bf.max_size = config->max_size;
	if (mask.s.check_idle )
	det_data.bf.check_idle = config->check_idle;
	if (mask.s.check_start )
	det_data.bf.check_start = config->check_start;
	if (mask.s.check_sld )
	det_data.bf.check_sld = config->check_sld;
	if (mask.s.check_preamble )
	det_data.bf.check_preamble = config->check_preamble;
	if (mask.s.check_crc8 )
	det_data.bf.check_crc8 = config->check_crc8;
	if (mask.s.check_terminate )
	det_data.bf.check_terminate = config->check_terminate;
	if (mask.s.rx_revcs )
	det_data.bf.rx_revcs = config->rx_revcs;

	CA_NI_REG_WRITE( cfg_data.wrd, NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG);
	CA_NI_REG_WRITE( det_data.wrd, NI_HV_XAUI_XAUI_MAC_RXMAC_RX_PKT_DET);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_rx_config_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG_t cfg_data;
	NI_HV_XAUI_XAUI_MAC_RXMAC_RX_PKT_DET_t det_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG);
	det_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_RXMAC_RX_PKT_DET);

	config->reset = cfg_data.bf.reset;
	config->dxaui_quanta_mode = cfg_data.bf.dxaui_quanta_mode;
	config->swap_crc8_en = cfg_data.bf.swap_crc8_en;
	config->swap_datain_en = cfg_data.bf.swap_datain_en;
	config->mac_en = cfg_data.bf.mac_en;
	config->pause_en = cfg_data.bf.pause_en;
	config->max_size = cfg_data.bf.max_size;
	config->check_idle = det_data.bf.check_idle;
	config->check_start = det_data.bf.check_start;
	config->check_sld = det_data.bf.check_sld;
	config->check_preamble = det_data.bf.check_preamble;
	config->check_crc8 = det_data.bf.check_crc8;
	config->check_terminate = det_data.bf.check_terminate;
	config->rx_revcs = det_data.bf.rx_revcs;
#endif

	return CA_E_OK;
}



ca_status_t aal_ni_xaui_mac_rx_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_rx_ctrl_mask_t mask,
	CA_IN aal_ni_xaui_mac_rx_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_CTL_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_CTL);

	if (mask.s.clr_rcffoverrun )
	cfg_data.bf.clr_rcffoverrun = config->clr_rcffoverrun;
	if (mask.s.drop_ptp_pkt_type )
	cfg_data.bf.drop_ptp_pkt_type = config->drop_ptp_pkt_type;
	if (mask.s.qbb_pause_drop_en )
	cfg_data.bf.qbb_pause_drop_en = config->qbb_pause_drop_en;
	if (mask.s.frame_error_drop_en )
	cfg_data.bf.frame_error_drop_en = config->frame_error_drop_en;
	if (mask.s.frame_fcs_error_drop_en )
	cfg_data.bf.frame_fcs_error_drop_en = config->frame_fcs_error_drop_en;
	if (mask.s.frame_bytecnt_drop_en )
	cfg_data.bf.frame_bytecnt_drop_en = config->frame_bytecnt_drop_en;
	if (mask.s.frame_pause_drop_en )
	cfg_data.bf.frame_pause_drop_en = config->frame_pause_drop_en;
	if (mask.s.frame_runt_drop_en )
	cfg_data.bf.frame_runt_drop_en = config->frame_runt_drop_en;
	if (mask.s.frame_oversize_drop_en )
	cfg_data.bf.frame_oversize_drop_en = config->frame_oversize_drop_en;
	if (mask.s.frame_ffoverflow_drop_en )
	cfg_data.bf.frame_ffoverflow_drop_en = config->frame_ffoverflow_drop_en;
	if (mask.s.frame_management_drop_en )
	cfg_data.bf.frame_management_drop_en = config->frame_management_drop_en;
	if (mask.s.frame_nomanagement_drop_en)
	cfg_data.bf.frame_nomanagement_drop_en = config->frame_nomanagement_drop_en;
	if (mask.s.qbb_pause_det_en )
	cfg_data.bf.qbb_pause_det_en = config->qbb_pause_det_en;
	if (mask.s.bypass )
	cfg_data.bf.bypass = config->bypass;
	if (mask.s.ptp_ing_ts_cmp )
	cfg_data.bf.ptp_ing_ts_cmp = config->ptp_ing_ts_cmp;
	if (mask.s.rx_pkt_clr )
	cfg_data.bf.rx_pkt_clr = config->rx_pkt_clr;
	if (mask.s.ptp_da_en )
	cfg_data.bf.ptp_da_en = config->ptp_da_en;

	CA_NI_REG_WRITE( cfg_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_CTL);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_rx_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_rx_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_CTL_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_CTL);

	config->clr_rcffoverrun = cfg_data.bf.clr_rcffoverrun;
	config->drop_ptp_pkt_type = cfg_data.bf.drop_ptp_pkt_type;
	config->qbb_pause_drop_en = cfg_data.bf.qbb_pause_drop_en;
	config->frame_error_drop_en = cfg_data.bf.frame_error_drop_en;
	config->frame_fcs_error_drop_en = cfg_data.bf.frame_fcs_error_drop_en;
	config->frame_bytecnt_drop_en = cfg_data.bf.frame_bytecnt_drop_en;
	config->frame_pause_drop_en = cfg_data.bf.frame_pause_drop_en;
	config->frame_runt_drop_en = cfg_data.bf.frame_runt_drop_en;
	config->frame_oversize_drop_en = cfg_data.bf.frame_oversize_drop_en;
	config->frame_ffoverflow_drop_en = cfg_data.bf.frame_ffoverflow_drop_en;
	config->frame_management_drop_en = cfg_data.bf.frame_management_drop_en;
	config->frame_nomanagement_drop_en = cfg_data.bf.frame_nomanagement_drop_en;
	config->qbb_pause_det_en = cfg_data.bf.qbb_pause_det_en;
	config->bypass = cfg_data.bf.bypass;
	config->ptp_ing_ts_cmp = cfg_data.bf.ptp_ing_ts_cmp;
	config->rx_pkt_clr = cfg_data.bf.rx_pkt_clr;
	config->ptp_da_en = cfg_data.bf.ptp_da_en;
#endif

	return CA_E_OK;
}



ca_status_t aal_ni_xaui_mac_tx_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_tx_ctrl_mask_t mask,
	CA_IN aal_ni_xaui_mac_tx_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL);

	if (mask.s.tx_remheader_dis )
	cfg_data.bf.tx_remheader_dis = config->tx_remheader_dis;
	if (mask.s.tx_pkt_clr )
	cfg_data.bf.tx_pkt_clr = config->tx_pkt_clr;
	if (mask.s.tx_pad_en )
	cfg_data.bf.tx_pad_en = config->tx_pad_en;
	if (mask.s.loopback )
	cfg_data.bf.loopback = config->loopback;
	if (mask.s.ptp_eg_ts_comp )
	cfg_data.bf.ptp_eg_ts_comp = config->ptp_eg_ts_comp;
	if (mask.s.external_pausereq_pol)
	cfg_data.bf.external_pausereq_pol = config->external_pausereq_pol;
	if (mask.s.qbb_flow_en )
	cfg_data.bf.qbb_flow_en = config->qbb_flow_en;
	if (mask.s.qbb_qen )
	cfg_data.bf.qbb_qen = config->qbb_qen;
	if (mask.s.external_txstop_sel )
	cfg_data.bf.external_txstop_sel = config->external_txstop_sel;
	if (mask.s.internal_txstop_sel )
	cfg_data.bf.internal_txstop_sel = config->internal_txstop_sel;
	if (mask.s.external_pausereq_sel)
	cfg_data.bf.external_pausereq_sel = config->external_pausereq_sel;
	if (mask.s.rx_pausereq_sel )
	cfg_data.bf.rx_pausereq_sel = config->rx_pausereq_sel;

	CA_NI_REG_WRITE( cfg_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_tx_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_tx_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL);

	config->tx_remheader_dis = cfg_data.bf.tx_remheader_dis;
	config->tx_pkt_clr = cfg_data.bf.tx_pkt_clr;
	config->tx_pad_en = cfg_data.bf.tx_pad_en;
	config->loopback = cfg_data.bf.loopback;
	config->ptp_eg_ts_comp = cfg_data.bf.ptp_eg_ts_comp;
	config->external_pausereq_pol = cfg_data.bf.external_pausereq_pol;
	config->qbb_flow_en = cfg_data.bf.qbb_flow_en;
	config->qbb_qen = cfg_data.bf.qbb_qen;
	config->external_txstop_sel = cfg_data.bf.external_txstop_sel;
	config->internal_txstop_sel = cfg_data.bf.internal_txstop_sel;
	config->external_pausereq_sel = cfg_data.bf.external_pausereq_sel;
	config->rx_pausereq_sel = cfg_data.bf.rx_pausereq_sel;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_addr_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_mac_addr_t mac)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_H_t data_h;
	NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_L_t data_l;

	AAL_ASSERT_RET(mac, CA_E_PARAM);

	data_h.wrd = 0;
	data_l.wrd = 0;

	data_h.bf.address = (mac[5] << 8) | mac[4];
	data_l.bf.address = (mac[3] << 24) |
	(mac[2] << 16) |
	(mac[1] << 8) |
	(mac[0] << 0);

	CA_NI_REG_WRITE( data_h.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_H);
	CA_NI_REG_WRITE( data_l.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_L);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_addr_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_mac_addr_t mac)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_H_t data_h;
	NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_L_t data_l;

	AAL_ASSERT_RET(mac, CA_E_PARAM);

	data_h.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_H);
	data_l.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_ADDRESS_L);

	mac[5] = (data_h.bf.address >> 8) & 0xFF;
	mac[4] = (data_h.bf.address >> 0) & 0xFF;
	mac[3] = (data_l.bf.address >> 24) & 0xFF;
	mac[2] = (data_l.bf.address >> 16) & 0xFF;
	mac[1] = (data_l.bf.address >> 8) & 0xFF;
	mac[0] = (data_l.bf.address >> 0) & 0xFF;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_mode_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_mode_mask_t mask,
	CA_OUT aal_ni_xaui_mac_ctrl_mode_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_MODE_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_MODE);

	if (mask.s.ctrl_mode_en )
	cfg_data.bf.ctrl_mode_en = config->ctrl_mode_en;
	if (mask.s.flush_pkt_en )
	cfg_data.bf.flush_pkt_en = config->flush_pkt_en;
	if (mask.s.flush_pkt_en1 )
	cfg_data.bf.flush_pkt_en1 = config->flush_pkt_en1;
	if (mask.s.flush_pkt_sdid )
	cfg_data.bf.flush_pkt_sdid = config->flush_pkt_sdid;
	if (mask.s.pkt_tocsport_en )
	cfg_data.bf.pkt_tocsport_en = config->pkt_tocsport_en;
	if (mask.s.pkt_toupport_en )
	cfg_data.bf.pkt_toupport_en = config->pkt_toupport_en;
	if (mask.s.pkt_tocsport_toup )
	cfg_data.bf.pkt_tocsport_toup = config->pkt_tocsport_toup;
	if (mask.s.pkt_toupport_tocs )
	cfg_data.bf.pkt_toupport_tocs = config->pkt_toupport_tocs;
	if (mask.s.flush_touplink_pkt)
	cfg_data.bf.flush_touplink_pkt = config->flush_touplink_pkt;
	if (mask.s.mask_nop_en )
	cfg_data.bf.mask_nop_en = config->mask_nop_en;
	if (mask.s.mask_ddbm_en )
	cfg_data.bf.mask_ddbm_en = config->mask_ddbm_en;

	CA_NI_REG_WRITE( cfg_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_MODE);
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_mode_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_mode_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_MODE_t cfg_data;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	cfg_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_CTRL_MODE);

	config->ctrl_mode_en = cfg_data.bf.ctrl_mode_en;
	config->flush_pkt_en = cfg_data.bf.flush_pkt_en;
	config->flush_pkt_en1 = cfg_data.bf.flush_pkt_en1;
	config->flush_pkt_sdid = cfg_data.bf.flush_pkt_sdid;
	config->pkt_tocsport_en = cfg_data.bf.pkt_tocsport_en;
	config->pkt_toupport_en = cfg_data.bf.pkt_toupport_en;
	config->pkt_tocsport_toup = cfg_data.bf.pkt_tocsport_toup;
	config->pkt_toupport_tocs = cfg_data.bf.pkt_toupport_tocs;
	config->flush_touplink_pkt = cfg_data.bf.flush_touplink_pkt;
	config->mask_nop_en = cfg_data.bf.mask_nop_en;
	config->mask_ddbm_en = cfg_data.bf.mask_ddbm_en;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_flowctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_flowctrl_mask_t mask,
	CA_IN aal_ni_xaui_mac_ctrl_flowctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWOFF_CTL_t off_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWON_CTL_t on_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA_t quanta0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA1_t quanta1;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA2_t quanta2;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA3_t quanta3;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA4_t quanta4;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA5_t quanta5;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA6_t quanta6;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA7_t quanta7;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	off_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWOFF_CTL);
	on_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWON_CTL);
	quanta0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA);
	quanta1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA1);
	quanta2.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA2);
	quanta3.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA3);
	quanta4.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA4);
	quanta5.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA5);
	quanta6.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA6);
	quanta7.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA7);

	if (mask.s.xoff_threshold )
	off_data.bf.xoff_threshold = config->xoff_threshold;
	if (mask.s.xon_threshold )
	on_data.bf.xon_threshold = config->xon_threshold;
	if (mask.s.adjust_qbb_cnt )
	quanta0.bf.adjust_qbb_cnt = config->adjust_qbb_cnt;
	if (mask.s.adjust_1g_qbb_cnt)
	quanta0.bf.adjust_1g_qbb_cnt= config->adjust_1g_qbb_cnt;
	if (mask.s.qbb_pause_quanta0)
	quanta0.bf.pause_quanta = config->qbb_pause_quanta0;
	if (mask.s.qbb_pause_quanta1)
	quanta1.bf.pause_quanta = config->qbb_pause_quanta1;
	if (mask.s.qbb_pause_quanta2)
	quanta2.bf.pause_quanta = config->qbb_pause_quanta2;
	if (mask.s.qbb_pause_quanta3)
	quanta3.bf.pause_quanta = config->qbb_pause_quanta3;
	if (mask.s.qbb_pause_quanta4)
	quanta4.bf.pause_quanta = config->qbb_pause_quanta4;
	if (mask.s.qbb_pause_quanta5)
	quanta5.bf.pause_quanta = config->qbb_pause_quanta5;
	if (mask.s.qbb_pause_quanta6)
	quanta6.bf.pause_quanta = config->qbb_pause_quanta6;
	if (mask.s.qbb_pause_quanta7)
	quanta7.bf.pause_quanta = config->qbb_pause_quanta7;

	CA_NI_REG_WRITE( off_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWOFF_CTL);
	CA_NI_REG_WRITE( on_data.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWON_CTL );
	CA_NI_REG_WRITE( quanta0.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA );
	CA_NI_REG_WRITE( quanta1.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA1 );
	CA_NI_REG_WRITE( quanta2.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA2 );
	CA_NI_REG_WRITE( quanta3.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA3 );
	CA_NI_REG_WRITE( quanta4.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA4 );
	CA_NI_REG_WRITE( quanta5.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA5 );
	CA_NI_REG_WRITE( quanta6.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA6 );
	CA_NI_REG_WRITE( quanta7.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA7 );
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_flowctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_flowctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWOFF_CTL_t off_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWON_CTL_t on_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA_t quanta0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA1_t quanta1;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA2_t quanta2;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA3_t quanta3;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA4_t quanta4;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA5_t quanta5;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA6_t quanta6;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA7_t quanta7;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	off_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWOFF_CTL);
	on_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_FLOWON_CTL);
	quanta0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA);
	quanta1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA1);
	quanta2.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA2);
	quanta3.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA3);
	quanta4.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA4);
	quanta5.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA5);
	quanta6.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA6);
	quanta7.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_QUANTA7);

	config->xoff_threshold = off_data.bf.xoff_threshold;
	config->xon_threshold = on_data.bf.xon_threshold;
	config->adjust_qbb_cnt = quanta0.bf.adjust_qbb_cnt;
	config->adjust_1g_qbb_cnt = quanta0.bf.adjust_1g_qbb_cnt;
	config->qbb_pause_quanta0 = quanta0.bf.pause_quanta;
	config->qbb_pause_quanta1 = quanta1.bf.pause_quanta;
	config->qbb_pause_quanta2 = quanta2.bf.pause_quanta;
	config->qbb_pause_quanta3 = quanta3.bf.pause_quanta;
	config->qbb_pause_quanta4 = quanta4.bf.pause_quanta;
	config->qbb_pause_quanta5 = quanta5.bf.pause_quanta;
	config->qbb_pause_quanta6 = quanta6.bf.pause_quanta;
	config->qbb_pause_quanta7 = quanta7.bf.pause_quanta;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_mib_ctrl_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_mib_ctrl_mask_t mask,
	CA_IN aal_ni_xaui_mac_ctrl_mib_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_MIB_CTL_t mib_ctl;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	mib_ctl.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_MIB_CTL);

	if (mask.s.ram_clr_en)
	mib_ctl.bf.ram_clr_en = config->ram_clr_en;
	if (mask.s.ram_sw_initial)
	mib_ctl.bf.ram_sw_initial = config->ram_sw_initial;

	CA_NI_REG_WRITE( mib_ctl.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_MIB_CTL);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_mib_ctrl_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_mib_ctrl_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_MIB_CTL_t mib_ctl;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	mib_ctl.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_MIB_CTL);

	config->ram_clr_en = mib_ctl.bf.ram_clr_en;
	config->ram_sw_initial = mib_ctl.bf.ram_sw_initial;
#endif

	return CA_E_OK;
}




ca_status_t aal_ni_xaui_mac_ctrl_mgm_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_mgm_cfg_mask_t mask,
	CA_IN aal_ni_xaui_mac_ctrl_mgm_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_HI_t da_hi;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_LO_t da_lo;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE0_t type0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE1_t type1;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE2_t type2;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE3_t type3;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE4_t type4;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE5_t type5;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE6_t type6;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE7_t type7;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	if (mask.s.da) {
		da_hi.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_HI);
		da_lo.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_LO);
		da_hi.bf.management_da = (config->da[5] << 8) | config->da[4];
		da_lo.bf.management_da = (config->da[3] << 24) |
		(config->da[2] << 16) |
		(config->da[1] << 8) |
		(config->da[0] << 0);
		CA_NI_REG_WRITE( da_hi.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_HI);
		CA_NI_REG_WRITE( da_lo.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_LO);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype0) {
		type0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE0);
		type0.bf.management_type0 = config->ethtype0;
		type0.bf.det_type0_en = config->detect_en_bmp & 0x01;
		CA_NI_REG_WRITE( type0.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE0);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype1) {
		type1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE1);
		type1.bf.management_type1 = config->ethtype1;
		type1.bf.det_type1_en = (config->detect_en_bmp >> 1) & 0x01;
		CA_NI_REG_WRITE( type1.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE1);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype2) {
		type2.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE2);
		type2.bf.management_type2 = config->ethtype2;
		type2.bf.det_type2_en = (config->detect_en_bmp >> 2) & 0x01;
		CA_NI_REG_WRITE( type2.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE2);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype3) {
		type3.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE3);
		type3.bf.management_type3 = config->ethtype3;
		type3.bf.det_type3_en = (config->detect_en_bmp >> 3) & 0x01;
		CA_NI_REG_WRITE( type3.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE3);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype4) {
		type4.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE4);
		type4.bf.management_type4 = config->ethtype4;
		type4.bf.det_type4_en = (config->detect_en_bmp >> 4) & 0x01;
		CA_NI_REG_WRITE( type4.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE4);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype5) {
		type5.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE5);
		type5.bf.management_type5 = config->ethtype5;
		type5.bf.det_type5_en = (config->detect_en_bmp >> 5) & 0x01;
		CA_NI_REG_WRITE( type5.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE5);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype6) {
		type6.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE6);
		type6.bf.management_type6 = config->ethtype6;
		type6.bf.det_type6_en = (config->detect_en_bmp >> 6) & 0x01;
		CA_NI_REG_WRITE( type6.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE6);
	}

	if (mask.s.detect_en_bmp || mask.s.ethtype7) {
		type7.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE7);
		type7.bf.management_type7 = config->ethtype7;
		type7.bf.det_type7_en = (config->detect_en_bmp >> 7) & 0x01;
		CA_NI_REG_WRITE( type7.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE7);
	}
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_mgm_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_mgm_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_HI_t da_hi;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_LO_t da_lo;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE0_t type0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE1_t type1;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE2_t type2;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE3_t type3;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE4_t type4;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE5_t type5;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE6_t type6;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE7_t type7;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	da_hi.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_HI);
	da_lo.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_DA_LO);
	type0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE0);
	type1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE1);
	type2.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE2);
	type3.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE3);
	type4.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE4);
	type5.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE5);
	type6.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE6);
	type7.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_MANAGEMENT_TYPE7);

	config->da[5] = (da_hi.bf.management_da >> 8) & 0xFF;
	config->da[4] = (da_hi.bf.management_da >> 0) & 0xFF;
	config->da[3] = (da_lo.bf.management_da >> 24) & 0xFF;
	config->da[2] = (da_lo.bf.management_da >> 16) & 0xFF;
	config->da[1] = (da_lo.bf.management_da >> 8) & 0xFF;
	config->da[0] = (da_lo.bf.management_da >> 0) & 0xFF;
	config->ethtype0 = type0.bf.management_type0;
	config->ethtype1 = type1.bf.management_type1;
	config->ethtype2 = type2.bf.management_type2;
	config->ethtype3 = type3.bf.management_type3;
	config->ethtype4 = type4.bf.management_type4;
	config->ethtype5 = type5.bf.management_type5;
	config->ethtype6 = type6.bf.management_type6;
	config->ethtype7 = type7.bf.management_type7;
	config->detect_en_bmp = (type7.bf.management_type7 << 7) |
	(type6.bf.management_type6 << 6) |
	(type5.bf.management_type5 << 5) |
	(type4.bf.management_type4 << 4) |
	(type3.bf.management_type3 << 3) |
	(type2.bf.management_type2 << 2) |
	(type1.bf.management_type1 << 1) |
	(type0.bf.management_type0 << 0);
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_rx_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_rx_mib_t *counter)
{
	///TODO: debug
#if 0
	//NI_HV_XAUI_XAUI_MAC_CTRL_MIB_RX_DROPCNT
	//NI_HV_XAUI_XAUI_MAC_CTRL_MIB_TX_DROPCNT
	//NI_HV_XAUI_XAUI_MAC_CTRL_TX_PKT_MIB
	//NI_HV_XAUI_XAUI_MAC_CTRL_RX_PKT_MIB
	//NI_HV_XAUI_XAUI_MAC_CTRL_MIB_RX_BYTECNT_HI
	//NI_HV_XAUI_XAUI_MAC_CTRL_MIB_TX_BYTECNT_HI

	//NI_HV_XAUI_XAUI_MAC_CTRL_RX_MIB_ACC_ACCESS_t table_acc;
	NI_HV_XAUI_XAUI_MAC_CTRL_RX_MIB_ACC_DATA_t data0;
	aal_ni_xaui_mac_ctrl_mib_id_t mib_id;
	ca_uint32_t *pbuf = (ca_uint32_t *)counter;

	AAL_ASSERT_RET(counter, CA_E_PARAM);

	for (mib_id = AAL_NI_XAUI_MAC_CTRL_MIB_ID_BYTE_LO;
	     mib_id <= AAL_NI_XAUI_MAC_CTRL_MIB_ID_RMON1519_MAX_CNT;
	     ++mib_id) {

		READ_INDIRCT_REG (device_id, mib_id, NI_HV_XAUI_XAUI_MAC_CTRL_RX_MIB_ACC_ACCESS);
		data0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_RX_MIB_ACC_DATA);
		pbuf[mib_id] = data0.bf.cnt;
	}
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_tx_mib_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_tx_mib_t *mib)
{
	///TODO: debug
#if 0
	//NI_HV_XAUI_XAUI_MAC_CTRL_TX_MIB_ACC_ACCESS_t table_acc;
	NI_HV_XAUI_XAUI_MAC_CTRL_TX_MIB_ACC_DATA_t data0;
	aal_ni_xaui_mac_ctrl_mib_id_t mib_id;
	ca_uint32_t *pbuf = (ca_uint32_t *)mib;

	AAL_ASSERT_RET(mib, CA_E_PARAM);

	for (mib_id = AAL_NI_XAUI_MAC_CTRL_MIB_ID_BYTE_LO;
	     mib_id <= AAL_NI_XAUI_MAC_CTRL_MIB_ID_RMON1519_MAX_CNT;
	     ++mib_id) {

		READ_INDIRCT_REG (device_id, mib_id, NI_HV_XAUI_XAUI_MAC_CTRL_TX_MIB_ACC_ACCESS);
		data0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_TX_MIB_ACC_DATA);
		pbuf[mib_id] = data0.bf.cnt;
	}
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_ptp_status_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_ptp_status_t *ptp_status)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM_t rx_tm_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM_S_LO_t rx_tm_s_lo_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM_S_HI_t rx_tm_s_hi_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TYPE_t rx_type_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_SQID_t rx_sqid_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM_t tx_tm_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM_S_LO_t tx_tm_s_lo_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM_S_HI_t tx_tm_s_hi_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TYPE_t tx_type_data;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_SQID_t tx_sqid_data;

	AAL_ASSERT_RET(ptp_status, CA_E_PARAM);

	rx_tm_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM );
	rx_tm_s_lo_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM_S_LO);
	rx_tm_s_hi_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TM_S_HI);
	rx_type_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_TYPE );
	rx_sqid_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_RX_PTP_SQID );
	tx_tm_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM );
	tx_tm_s_lo_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM_S_LO);
	tx_tm_s_hi_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TM_S_HI);
	tx_type_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_TYPE );
	tx_sqid_data.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_PTP_SQID );

	ptp_status->rx_ptp_tm = rx_tm_data.bf.ptp_tm;
	ptp_status->rx_ptp_tm_s_lo = rx_tm_s_lo_data.bf.ptp_tm;
	ptp_status->rx_ptp_tm_s_hi = rx_tm_s_hi_data.bf.ptp_tm;
	ptp_status->rx_ptp_type = rx_type_data.bf.ptp_type;
	ptp_status->rx_ptp_sqid = rx_sqid_data.bf.sq_id;
	ptp_status->tx_ptp_tm = tx_tm_data.bf.ptp_tm;
	ptp_status->tx_ptp_tm_s_lo = tx_tm_s_lo_data.bf.ptp_tm;
	ptp_status->tx_ptp_tm_s_hi = tx_tm_s_hi_data.bf.ptp_tm;
	ptp_status->tx_ptp_type = tx_type_data.bf.ptp_type;
	ptp_status->tx_ptp_sqid = tx_sqid_data.bf.sq_id;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_mac_ctrl_ptp_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_mac_ctrl_ptp_cfg_mask_t mask,
	CA_OUT aal_ni_xaui_mac_ctrl_ptp_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN0_t ptp_vlan0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN1_t ptp_vlan1;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	ptp_vlan0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN0);
	ptp_vlan1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN1);

	if (mask.s.vlan0) {
		ptp_vlan0.bf.vlan0 = config->vlan0;
	}

	if (mask.s.vlan1) {
		ptp_vlan0.bf.vlan1 = config->vlan1;
	}

	if (mask.s.vlan2) {
		ptp_vlan1.bf.vlan0 = config->vlan2;
	}

	if (mask.s.vlan3) {
		ptp_vlan1.bf.vlan1 = config->vlan3;
	}

	CA_NI_REG_WRITE( ptp_vlan0.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN0);
	CA_NI_REG_WRITE( ptp_vlan1.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN1);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_mac_ctrl_ptp_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_mac_ctrl_ptp_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN0_t ptp_vlan0;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN1_t ptp_vlan1;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	ptp_vlan0.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN0);
	ptp_vlan1.wrd = CA_NI_REG_READ( NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_PTP_VLAN1);

	config->vlan0 = ptp_vlan0.bf.vlan0;
	config->vlan1 = ptp_vlan0.bf.vlan1;
	config->vlan2 = ptp_vlan1.bf.vlan0;
	config->vlan3 = ptp_vlan1.bf.vlan1;
#endif

	return CA_E_OK;
}


/*********************************************/
/*  10GigabitReconciliation Sublayer, XGRS   */
/*********************************************/

ca_status_t aal_ni_xaui_xgrs_stats_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgrs_stats_t *stats)
{

	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_XGRS_TX_TRUNCPKT_t truncpkt;
	NI_HV_XAUI_XAUI_XGRS_TX_OIDLECOUNT_t oildecnt;
	NI_HV_XAUI_XAUI_XGRS_RX_UNKNOWNORD1_t unknword1;
	NI_HV_XAUI_XAUI_XGRS_RX_UNKNOWNORD2_t unknword2;

	AAL_ASSERT_RET(stats, CA_E_PARAM);

	truncpkt.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_TX_TRUNCPKT);
	oildecnt.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_TX_OIDLECOUNT);
	unknword1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_UNKNOWNORD1);
	unknword2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_UNKNOWNORD2);

	stats->trunc_pkt_tx = truncpkt.bf.trunc_pkt_tx;
	stats->oidle_pkt_tx = oildecnt.bf.oidle_pkt_tx;
	stats->rx_unknownord_l1 = unknword1.bf.rx_unknownOrd_l1;
	stats->rx_unknownord_l2 = unknword1.bf.rx_unknownOrd_l2;
	stats->rx_unknownord_l3 = unknword2.bf.rx_unknownOrd_l3;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_xgrs_tx_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgrs_tx_cfg_mask_t mask,
	CA_OUT aal_ni_xaui_xgrs_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_XGRS_TX_TXCNTRL_t ctrl0;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_TX_TXCNTRL);

	if (mask.s.tx_localOrdEn )
	ctrl0.bf.tx_localOrdEn = config->tx_localOrdEn;
	if (mask.s.tx_remoteOrdEn )
	ctrl0.bf.tx_remoteOrdEn = config->tx_remoteOrdEn;
	if (mask.s.tx_userOrdEn )
	ctrl0.bf.tx_userOrdEn = config->tx_userOrdEn;
	if (mask.s.tx_dataTruncEn )
	ctrl0.bf.tx_dataTruncEn = config->tx_dataTruncEn;
	if (mask.s.tx_pcs6_line_txd_en)
	ctrl0.bf.tx_pcs6_line_txd_en = config->tx_pcs6_line_txd_en;
	if (mask.s.tx_logicReset )
	ctrl0.bf.tx_logicReset = config->tx_logicReset;

	CA_NI_REG_WRITE(ctrl0.wrd, NI_HV_XAUI_XAUI_XGRS_TX_TXCNTRL);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgrs_tx_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgrs_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_XGRS_TX_TXCNTRL_t ctrl0;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_TX_TXCNTRL);

	config->tx_localOrdEn = ctrl0.bf.tx_localOrdEn;
	config->tx_remoteOrdEn = ctrl0.bf.tx_remoteOrdEn;
	config->tx_userOrdEn = ctrl0.bf.tx_userOrdEn;
	config->tx_dataTruncEn = ctrl0.bf.tx_dataTruncEn;
	config->tx_pcs6_line_txd_en = ctrl0.bf.tx_pcs6_line_txd_en;
	config->tx_logicReset = ctrl0.bf.tx_logicReset;
#endif

	return CA_E_OK;
}




ca_status_t aal_ni_xaui_xgrs_rx_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgrs_rx_cfg_mask_t mask,
	CA_OUT aal_ni_xaui_xgrs_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL1_t ctrl1;
	NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL2_t ctrl2;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL2);

	if (mask.s.rx_dataLosEn )
	ctrl1.bf.rx_dataLosEn = config->rx_dataLosEn;
	if (mask.s.rx_lockLosEn )
	ctrl1.bf.rx_lockLosEn = config->rx_lockLosEn;
	if (mask.s.rx_sigLosEn )
	ctrl1.bf.rx_sigLosEn = config->rx_sigLosEn;
	if (mask.s.rx_phyRLFEn )
	ctrl1.bf.rx_phyRLFEn = config->rx_phyRLFEn;
	if (mask.s.rx_losPolarity )
	ctrl1.bf.rx_losPolarity = config->rx_losPolarity;
	if (mask.s.rx_noIpg )
	ctrl1.bf.rx_noIpg = config->rx_noIpg;
	if (mask.s.rx_ofsmUNHEn )
	ctrl1.bf.rx_ofsmUNHEn = config->rx_ofsmUNHEn;
	if (mask.s.rx_pcs6_line_rxd_en)
	ctrl1.bf.rx_pcs6_line_rxd_en = config->rx_pcs6_line_rxd_en;
	if (mask.s.rx_logicReset )
	ctrl1.bf.rx_logicReset = config->rx_logicReset;
	if (mask.s.rx_col_thr )
	ctrl2.bf.rx_col_thr = config->rx_col_thr;
	if (mask.s.rx_seq_thr )
	ctrl2.bf.rx_seq_thr = config->rx_seq_thr;

	CA_NI_REG_WRITE(ctrl1.wrd, NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL1);
	CA_NI_REG_WRITE(ctrl2.wrd, NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL2);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgrs_rx_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgrs_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL1_t ctrl1;
	NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL2_t ctrl2;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XAUI_XGRS_RX_RXCNTRL2);

	config->rx_dataLosEn = ctrl1.bf.rx_dataLosEn;
	config->rx_lockLosEn = ctrl1.bf.rx_lockLosEn;
	config->rx_sigLosEn = ctrl1.bf.rx_sigLosEn;
	config->rx_phyRLFEn = ctrl1.bf.rx_phyRLFEn;
	config->rx_losPolarity = ctrl1.bf.rx_losPolarity;
	config->rx_noIpg = ctrl1.bf.rx_noIpg;
	config->rx_ofsmUNHEn = ctrl1.bf.rx_ofsmUNHEn;
	config->rx_pcs6_line_rxd_en = ctrl1.bf.rx_pcs6_line_rxd_en;
	config->rx_logicReset = ctrl1.bf.rx_logicReset;
	config->rx_col_thr = ctrl2.bf.rx_col_thr;
	config->rx_seq_thr = ctrl2.bf.rx_seq_thr;
#endif

	return CA_E_OK;
}



/********************************************/
/*       XAUI Extender Sublayer, XGXS       */
/********************************************/


ca_status_t aal_ni_xaui_xgxs_stats_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgxs_stats_t *stats)
{

	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_GoodPkt_t rx_good;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_ErrPkt_t rx_err;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_GoodPkt_t tx_good;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_ErrPkt_t tx_err;

	AAL_ASSERT_RET(stats, CA_E_PARAM);

	rx_good.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_GoodPkt);
	rx_err.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_ErrPkt);
	tx_good.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_GoodPkt);
	tx_err.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_ErrPkt);

	stats->rx_good_cnt = rx_good.bf.good_pkt_rx;
	stats->rx_err_cnt = rx_err.bf.err_pkt_rx;
	stats->tx_good_cnt = tx_good.bf.good_pkt_tx;
	stats->tx_err_cnt = tx_err.bf.err_pkt_tx;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgxs_rx_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgxs_rx_cfg_mask_t mask,
	CA_OUT aal_ni_xaui_xgxs_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl0_t ctrl0;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl1_t ctrl1;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl2_t ctrl2;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl3_t ctrl3;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl4_t ctrl4;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl5_t ctrl5;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl0);
	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl2);
	ctrl3.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl3);
	ctrl4.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl4);
	ctrl5.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl5);

	if (mask.s.xgxs_rx_mode ) {
		ctrl0.bf.xgxs_rx_mode = config->xgxs_rx_mode;
	}

	if (mask.s.xgxs_rx_random_6) {
		ctrl0.bf.xgxs_rx_random_6 = config->xgxs_rx_random_6;
	}

	if (mask.s.xgxs_rx_err_inj ) {
		ctrl0.bf.xgxs_rx_err_inj = config->xgxs_rx_err_inj;
	}

	if (mask.s.xgxs_fpat_sel ) {
		ctrl0.bf.xgxs_fpat_sel = config->xgxs_fpat_sel;
	}

	if (mask.s.xgxs_fpat_0 ) {
		ctrl1.bf.xgxs_fpat_0 = config->xgxs_fpat_0;
	}

	if (mask.s.xgxs_fpat_1 ) {
		ctrl2.bf.xgxs_fpat_1 = config->xgxs_fpat_1;
	}

	if (mask.s.xgxs_fpat_2 ) {
		ctrl3.bf.xgxs_fpat_2 = config->xgxs_fpat_2;
	}

	if (mask.s.xgxs_fpat_3 ) {
		ctrl4.bf.xgxs_fpat_3 = config->xgxs_fpat_3;
	}

	if (mask.s.xgxs_rx_phase ) {
		ctrl5.bf.xgxs_rx_phase = config->xgxs_rx_phase;
	}

	if (mask.s.xgxs_rx_fzero ) {
		ctrl5.bf.xgxs_rx_fzero = config->xgxs_rx_fzero;
	}

	if (mask.s.xgxs_rx_fone ) {
		ctrl5.bf.xgxs_rx_fone = config->xgxs_rx_fone;
	}

	CA_NI_REG_WRITE(ctrl0.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl0);
	CA_NI_REG_WRITE(ctrl1.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl1);
	CA_NI_REG_WRITE(ctrl2.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl2);
	CA_NI_REG_WRITE(ctrl3.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl3);
	CA_NI_REG_WRITE(ctrl4.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl4);
	CA_NI_REG_WRITE(ctrl5.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl5);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgxs_rx_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgxs_rx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl0_t ctrl0;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl1_t ctrl1;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl2_t ctrl2;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl3_t ctrl3;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl4_t ctrl4;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl5_t ctrl5;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl0);
	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl2);
	ctrl3.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl3);
	ctrl4.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl4);
	ctrl5.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RxCntrl5);

	config->xgxs_rx_mode = ctrl0.bf.xgxs_rx_mode;
	config->xgxs_rx_random_6 = ctrl0.bf.xgxs_rx_random_6;
	config->xgxs_rx_err_inj = ctrl0.bf.xgxs_rx_err_inj;
	config->xgxs_fpat_sel = ctrl0.bf.xgxs_fpat_sel;
	config->xgxs_fpat_0 = ctrl1.bf.xgxs_fpat_0;
	config->xgxs_fpat_1 = ctrl2.bf.xgxs_fpat_1;
	config->xgxs_fpat_2 = ctrl3.bf.xgxs_fpat_2;
	config->xgxs_fpat_3 = ctrl4.bf.xgxs_fpat_3;
	config->xgxs_rx_phase = ctrl5.bf.xgxs_rx_phase;
	config->xgxs_rx_fzero = ctrl5.bf.xgxs_rx_fzero;
	config->xgxs_rx_fone = ctrl5.bf.xgxs_rx_fone;
#endif

	return CA_E_OK;
}



ca_status_t aal_ni_xaui_xgxs_tx_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgxs_tx_cfg_mask_t mask,
	CA_OUT aal_ni_xaui_xgxs_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl0_t ctrl0;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl1_t ctrl1;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl2_t ctrl2;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl3_t ctrl3;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl4_t ctrl4;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl5_t ctrl5;

	AAL_ASSERT_RET(config, CA_E_PARAM);
	AAL_ASSERT_RET(mask.u32, CA_E_OK);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl0);
	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl2);
	ctrl3.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl3);
	ctrl4.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl4);
	ctrl5.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl5);

	if (mask.s.xgxs_tx_mode ) {
		ctrl0.bf.xgxs_tx_mode = config->xgxs_tx_mode;
	}
	if (mask.s.xgxs_lf_enable ) {
		ctrl0.bf.xgxs_lf_enable = config->xgxs_lf_enable;
	}
	if (mask.s.xgxs_ddsa_enable ) {
		ctrl0.bf.xgxs_ddsa_enable = config->xgxs_ddsa_enable;
	}
	if (mask.s.xgxs_fpat_sel ) {
		ctrl0.bf.xgxs_fpat_sel = config->xgxs_fpat_sel;
	}
	if (mask.s.xgxs_fpat_0 ) {
		ctrl1.bf.xgxs_fpat_0 = config->xgxs_fpat_0;
	}
	if (mask.s.xgxs_fpat_1 ) {
		ctrl2.bf.xgxs_fpat_1 = config->xgxs_fpat_1;
	}
	if (mask.s.xgxs_fpat_2 ) {
		ctrl3.bf.xgxs_fpat_2 = config->xgxs_fpat_2;
	}
	if (mask.s.xgxs_fpat_3 ) {
		ctrl4.bf.xgxs_fpat_3 = config->xgxs_fpat_3;
	}
	if (mask.s.rxaui_cgalign_ena_mode ) {
		ctrl5.bf.rxaui_cgalign_ena_mode = config->rxaui_cgalign_ena_mode;
	}
	if (mask.s.rxaui_a_column_ena_mode ) {
		ctrl5.bf.rxaui_a_column_ena_mode = config->rxaui_a_column_ena_mode;
	}
	if (mask.s.rxaui_a_column_verify_mode) {
		ctrl5.bf.rxaui_a_column_verify_mode = config->rxaui_a_column_verify_mode;
	}
	if (mask.s.rxaui_lane_swap_01 ) {
		ctrl5.bf.rxaui_lane_swap_01 = config->rxaui_lane_swap_01;
	}
	if (mask.s.rxaui_lane_swap_23 ) {
		ctrl5.bf.rxaui_lane_swap_23 = config->rxaui_lane_swap_23;
	}
	if (mask.s.rxaui_comma_thresh ) {
		ctrl5.bf.rxaui_comma_thresh = config->rxaui_comma_thresh;
	}

	CA_NI_REG_WRITE(ctrl0.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl0);
	CA_NI_REG_WRITE(ctrl1.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl1);
	CA_NI_REG_WRITE(ctrl2.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl2);
	CA_NI_REG_WRITE(ctrl3.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl3);
	CA_NI_REG_WRITE(ctrl4.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl4);
	CA_NI_REG_WRITE(ctrl5.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl5);
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_xgxs_tx_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgxs_tx_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl0_t ctrl0;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl1_t ctrl1;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl2_t ctrl2;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl3_t ctrl3;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl4_t ctrl4;
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl5_t ctrl5;

	AAL_ASSERT_RET(config, CA_E_PARAM);

	ctrl0.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl0);
	ctrl1.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl1);
	ctrl2.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl2);
	ctrl3.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl3);
	ctrl4.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl4);
	ctrl5.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TxCntrl5);

	config->xgxs_tx_mode = ctrl0.bf.xgxs_tx_mode;
	config->xgxs_lf_enable = ctrl0.bf.xgxs_lf_enable;
	config->xgxs_ddsa_enable = ctrl0.bf.xgxs_ddsa_enable;
	config->xgxs_fpat_sel = ctrl0.bf.xgxs_fpat_sel;
	config->xgxs_fpat_0 = ctrl1.bf.xgxs_fpat_0;
	config->xgxs_fpat_1 = ctrl2.bf.xgxs_fpat_1;
	config->xgxs_fpat_2 = ctrl3.bf.xgxs_fpat_2;
	config->xgxs_fpat_3 = ctrl4.bf.xgxs_fpat_3;
	config->rxaui_cgalign_ena_mode = ctrl5.bf.rxaui_cgalign_ena_mode;
	config->rxaui_a_column_ena_mode = ctrl5.bf.rxaui_a_column_ena_mode;
	config->rxaui_a_column_verify_mode = ctrl5.bf.rxaui_a_column_verify_mode;
	config->rxaui_lane_swap_01 = ctrl5.bf.rxaui_lane_swap_01;
	config->rxaui_lane_swap_23 = ctrl5.bf.rxaui_lane_swap_23;
	config->rxaui_comma_thresh = ctrl5.bf.rxaui_comma_thresh;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgxs_intr_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgxs_intr_cfg_mask_t mask,
	CA_IN aal_ni_xaui_xgxs_intr_cfg_t *config)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTENABLE_t intenable;

	AAL_ASSERT_RET(NULL != config, CA_E_PARAM);

	intenable.wrd = CA_NI_REG_READ(
				       NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTENABLE);

	if (mask.s.Tx_SyncDet0 )
	intenable.bf.SyncDet0e = config->Tx_SyncDet0;
	if (mask.s.Tx_SyncDet1 )
	intenable.bf.SyncDet1e = config->Tx_SyncDet1;
	if (mask.s.Tx_SyncDet2 )
	intenable.bf.SyncDet2e = config->Tx_SyncDet2;
	if (mask.s.Tx_SyncDet3 )
	intenable.bf.SyncDet3e = config->Tx_SyncDet3;
	if (mask.s.Tx_xgxs_align_det)
	intenable.bf.AlignDete = config->Tx_xgxs_align_det;
	if (mask.s.Tx_xgxs_cpat_lock)
	intenable.bf.PatLockDete = config->Tx_xgxs_cpat_lock;

	CA_NI_REG_WRITE(intenable.wrd,
			NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTENABLE);
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgxs_intr_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgxs_intr_cfg_t *config)
{

	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTENABLE_t intenable;

	AAL_ASSERT_RET(NULL != config, CA_E_PARAM);

	intenable.wrd = CA_NI_REG_READ(
				       NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTENABLE);

	config->Tx_SyncDet0 = intenable.bf.SyncDet0e;
	config->Tx_SyncDet1 = intenable.bf.SyncDet1e;
	config->Tx_SyncDet2 = intenable.bf.SyncDet2e;
	config->Tx_SyncDet3 = intenable.bf.SyncDet3e;
	config->Tx_xgxs_align_det = intenable.bf.AlignDete;
	config->Tx_xgxs_cpat_lock = intenable.bf.PatLockDete;
#endif

	return CA_E_OK;
}

ca_status_t aal_ni_xaui_xgxs_intr_status_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_xaui_xgxs_intr_status_t *status)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTSTAT_t intstat;
	NI_HV_XAUI_XGXS_TOP_XGXS_RX_RX_INTSTAT_t rxstat;

	AAL_ASSERT_RET(NULL != status, CA_E_PARAM);

	intstat.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTSTAT);
	rxstat.wrd = CA_NI_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_RX_INTSTAT);

	status->Tx_ErrorPktSat = intstat.bf.ErrorPktSat;
	status->Tx_GoodPktSat = intstat.bf.GoodPktSat;
	status->Tx_InvCodeSat = intstat.bf.InvCodeSat;
	status->Tx_xgxs_comma_found_0 = intstat.bf.xgxs_comma_found_0;
	status->Tx_PatErrSat = intstat.bf.PatErrSat;
	status->Tx_xgxs_comma_found_1 = intstat.bf.xgxs_comma_found_1;
	status->Tx_SyncDet0 = intstat.bf.SyncDet0;
	status->Tx_SyncDet1 = intstat.bf.SyncDet1;
	status->Tx_SyncDet2 = intstat.bf.SyncDet2;
	status->Tx_SyncDet3 = intstat.bf.SyncDet3;
	status->Tx_xgxs_align_det = intstat.bf.xgxs_align_det;
	status->Tx_xgxs_cpat_lock = intstat.bf.xgxs_cpat_lock;
	status->Tx_xgxs_A_column_found_0 = intstat.bf.xgxs_A_column_found_0;
	status->Tx_xgxs_A_column_found_1 = intstat.bf.xgxs_A_column_found_1;
	status->Rx_ErrorPktSat = rxstat.bf.ErrorPktSat;
	status->Rx_GoodPktSat = rxstat.bf.GoodPktSat;
#endif

	return CA_E_OK;
}


ca_status_t aal_ni_xaui_xgxs_intr_status_clear(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_xaui_xgxs_intr_cfg_t *clear)
{
	///TODO: debug
#if 0
	NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTERRUPT_t intstat;

	AAL_ASSERT_RET(NULL != clear, CA_E_PARAM);

	intstat.wrd = 0;

	intstat.bf.SyncDet0I = clear->Tx_SyncDet0;
	intstat.bf.SyncDet1I = clear->Tx_SyncDet1;
	intstat.bf.SyncDet2I = clear->Tx_SyncDet2;
	intstat.bf.SyncDet3I = clear->Tx_SyncDet3;
	intstat.bf.AlignDetI = clear->Tx_xgxs_align_det;
	intstat.bf.PatLockDetI = clear->Tx_xgxs_cpat_lock;

	CA_NI_REG_WRITE(intstat.wrd, NI_HV_XAUI_XGXS_TOP_XGXS_TX_TX_INTERRUPT);
#endif

	return CA_E_OK;
}

/*********************************************/
/*        PTP, PPS, ToD                      */
/*********************************************/
ca_status_t aal_global_gpio_mux_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_global_gpio_mux_cfg_mask_t mask,
	CA_IN aal_global_gpio_mux_cfg_t *config)
{
	GLOBAL_GPIO_MUX_1_t mux1;

	AAL_ASSERT_RET(config,CA_E_PARAM);

	mux1.wrd = CA_GLB_REG_READ(GLOBAL_GPIO_MUX_1);

	if ( mask.s.gpio1en ) {
		mux1.bf.gpio1en = config->gpio1en;
	}

	CA_GLB_REG_WRITE(mux1.wrd,GLOBAL_GPIO_MUX_1);

	return CA_E_OK;
}

ca_status_t aal_global_gpio_mux_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_global_gpio_mux_cfg_t *config)
{
	GLOBAL_GPIO_MUX_1_t mux1;

	AAL_ASSERT_RET(config,CA_E_PARAM);

	mux1.wrd = CA_GLB_REG_READ(GLOBAL_GPIO_MUX_1);
	config->gpio1en = mux1.bf.gpio1en;

	return CA_E_OK;
}


ca_status_t aal_ni_ptp_pps_cfg_set(
	CA_IN ca_device_id_t device_id,
	CA_IN aal_ni_ptp_pps_cfg_mask_t mask,
	CA_IN aal_ni_ptp_pps_cfg_t *config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)
	NI_HV_PTP_PPS_CFG_0_t cfg0;
	NI_HV_PTP_PPS_CFG_1_t cfg1;
	NI_HV_PTP_PPS_CFG_2_t cfg2;
	NI_HV_PTP_PPS_CFG_3_t cfg3;
	NI_HV_PTP_PPS_CFG_4_t cfg4;
	NI_HV_PTP_PPS_CFG_5_t cfg5;

	AAL_ASSERT_RET(config,CA_E_PARAM);

	cfg0.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_0);
	cfg1.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_1);
	cfg2.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_2);
	cfg3.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_3);
	cfg4.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_4);
	cfg5.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_5);

	if ( mask.s.ptp_en )
		cfg0.bf.ptp_en = config->ptp_en;
	if ( mask.s.pps_in_en )
		cfg0.bf.pps_in_en = config->pps_in_en;
	if ( mask.s.pps_edge_sel_sample )
		cfg0.bf.pps_edge_sel_sample = config->pps_edge_sel_sample;
	if ( mask.s.pps_edge_sel_load )
		cfg0.bf.pps_edge_sel_load = config->pps_edge_sel_load;
	if ( mask.s.pps_load_en )
		cfg0.bf.pps_load_en = config->pps_load_en;
	if ( mask.s.pps_out_en )
		cfg0.bf.pps_out_en = config->pps_out_en;
	if ( mask.s.pps_width )
		cfg0.bf.pps_width = config->pps_width;
	if ( mask.s.ptp_sum_val )
		cfg1.bf.ptp_sum_val = config->ptp_sum_val;
	if ( mask.s.pps_timestamp_load )
		cfg2.bf.pps_timestamp_load = config->pps_timestamp_load;
	if ( mask.s.s_pps )
		cfg3.bf.s_pps = config->s_pps;
	if ( mask.s.s_tod )
		cfg4.bf.s_tod = config->s_tod;
	if ( mask.s.load_matching_tmr )
		cfg5.bf.load_matching_tmr = config->load_matching_tmr;
	if ( mask.s.load_matching_tmr_en )
		cfg5.bf.load_matching_tmr_en = config->load_matching_tmr_en;

	CA_NI_REG_WRITE(cfg0.wrd,NI_HV_PTP_PPS_CFG_0);
	CA_NI_REG_WRITE(cfg1.wrd,NI_HV_PTP_PPS_CFG_1);
	CA_NI_REG_WRITE(cfg2.wrd,NI_HV_PTP_PPS_CFG_2);
	CA_NI_REG_WRITE(cfg3.wrd,NI_HV_PTP_PPS_CFG_3);
	CA_NI_REG_WRITE(cfg4.wrd,NI_HV_PTP_PPS_CFG_4);
	CA_NI_REG_WRITE(cfg5.wrd,NI_HV_PTP_PPS_CFG_5);
#endif
	return CA_E_OK;

}


ca_status_t aal_ni_ptp_pps_cfg_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT aal_ni_ptp_pps_cfg_t *config)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)

	NI_HV_PTP_PPS_CFG_0_t cfg0;
	NI_HV_PTP_PPS_CFG_1_t cfg1;
	NI_HV_PTP_PPS_CFG_2_t cfg2;
	NI_HV_PTP_PPS_CFG_3_t cfg3;
	NI_HV_PTP_PPS_CFG_4_t cfg4;
	NI_HV_PTP_PPS_CFG_5_t cfg5;

	AAL_ASSERT_RET(config,CA_E_PARAM);

	cfg0.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_0);
	cfg1.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_1);
	cfg2.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_2);
	cfg3.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_3);
	cfg4.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_4);
	cfg5.wrd = CA_NI_REG_READ(NI_HV_PTP_PPS_CFG_5);

	config->ptp_en = cfg0.bf.ptp_en;
	config->pps_in_en = cfg0.bf.pps_in_en;
	config->pps_edge_sel_sample = cfg0.bf.pps_edge_sel_sample;
	config->pps_edge_sel_load = cfg0.bf.pps_edge_sel_load;
	config->pps_load_en = cfg0.bf.pps_load_en;
	config->pps_out_en = cfg0.bf.pps_out_en;
	config->pps_width = cfg0.bf.pps_width;
	config->ptp_sum_val = cfg1.bf.ptp_sum_val;
	config->pps_timestamp_load = cfg2.bf.pps_timestamp_load;
	config->s_pps = cfg3.bf.s_pps;
	config->s_tod = cfg4.bf.s_tod;
	config->load_matching_tmr = cfg5.bf.load_matching_tmr;
	config->load_matching_tmr_en = cfg5.bf.load_matching_tmr_en;
#endif
	return CA_E_OK;

}


ca_status_t aal_ni_ptp_pps_timer_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *time)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)

	/* The latched PTP HW timer value on selected edge of pps input */
	NI_HV_PTP_TIMER_ON_PPS_t timer_pps;

	AAL_ASSERT_RET(time,CA_E_PARAM);

	timer_pps.wrd = CA_NI_REG_READ(NI_HV_PTP_TIMER_ON_PPS);
	*time = timer_pps.bf.pps_timestamp_sample;
#endif
	return CA_E_OK;
}

ca_status_t aal_ni_ptp_hw_timer_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint64_t *time)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)

	NI_HV_PTP_TIMER_t hw_timer; /* aligned to 64 ns */

	AAL_ASSERT_RET(time != NULL,CA_E_PARAM);

	hw_timer.wrd = CA_NI_REG_READ(NI_HV_PTP_TIMER);
	*time = hw_timer.bf.ptp_hw_ts;
#endif
	return CA_E_OK;
}

ca_status_t aal_ni_ptp_hw_timer_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint64_t time)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_G3LITE)

	NI_HV_PTP_TIMER_t hw_timer;

	AAL_ASSERT_RET(time <= 0x3FFFFFFF,CA_E_PARAM);

	hw_timer.wrd = CA_NI_REG_READ(NI_HV_PTP_TIMER);
	hw_timer.bf.ptp_hw_ts = time;

	CA_NI_REG_WRITE(hw_timer.wrd,NI_HV_PTP_TIMER);
#endif

	return CA_E_OK;

}

void aal_ni_reset_intf(ca_device_id_t device_id, ca_uint32_t pbmp)
{
    ca_uint32_t ge_bmp = 0;

    CA_NI_REG_WRITE(0, NI_HV_GLB_INTF_RST_CONFIG);

    ca_printf("ca-ne: aal_ni_reset_intf init ...  OK NI_HV_GLB_INTF_RST_CONFIG = %#x\n",
        CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG));

}


/**  aal_ni_init
 * Initialize NI block to drive GE, XAUI, NI-MUX, PTP
 *
 */
ca_status_t aal_ni_init(CA_IN ca_device_id_t device_id)
{
	ca_status_t ret = CA_E_OK;
	ret = __ni_mem_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_mem_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_rx_init(device_id); /* disable MAC Rx */
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_rx_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_tx_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_tx_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_rxmux_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_rxmux_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_mc_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_mc_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_pkt_len_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_pkt_len_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_flow_ctrl_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_flow_ctrl_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_mib_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_mib_init fail, return %#x\n",ret);
		goto __end;
	}

	ret = __ni_intr_init(device_id);
	if ( CA_E_OK != ret ) {
		ca_printf("__ni_intr_init fail, return %#x\n",ret);
		goto __end;
	}


	REG_L2FE_FIELD_WRITE(NI_HV_GLB_NIRX_MISC_CFG,use_p0_as_wan_port_en,1);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_NIRX_MISC_CFG,p0_as_wan_encap_decap_en,1);

__end:

	return ret;
}

/* TODO: g3_register.h is not the latest one and appcli/iros can't suuport ifdef, so hard code the address for G3 right now. */
aal_direct_table_t ni_cpu_tag_cfg_table = {
	.max_entry = APB0_NI_HV_PT_COUNT,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_PT_PORT_STATIC_CFG,
#else
	.first_reg = 0xf4304400,
#endif
	.reg_num = 1,
	.reg_stride = APB0_NI_HV_PT_STRIDE
};

ca_status_t aal_ni_pt_cpu_tag_cfg_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t port_id,
	CA_OUT ca_ni_cpu_tag_cfg_t *entry
	)
{
	return aal_direct_table_get(&ni_cpu_tag_cfg_table,port_id,entry,AAL_TABLE_DIR_LOW_TO_HI);
}

ca_status_t aal_ni_pt_cpu_tag_cfg_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t port_id,
	CA_IN ca_ni_cpu_tag_cfg_t *entry
	)
{
	ca_ni_cpu_tag_cfg_t orig;
	aal_direct_table_get(&ni_cpu_tag_cfg_table,port_id,&orig,AAL_TABLE_DIR_LOW_TO_HI);
	orig.cpu_tag_rx_en = entry->cpu_tag_rx_en;
	orig.cpu_tag_tx_en = entry->cpu_tag_tx_en;
	orig.cpu_tag_cfg_sel = entry->cpu_tag_cfg_sel;
	return aal_direct_table_set(&ni_cpu_tag_cfg_table,port_id,&orig,AAL_TABLE_DIR_LOW_TO_HI);
}

aal_direct_table_t ni_cpu_tag_dft_hdra_table = {
	.max_entry = APB0_NI_HV_PT_COUNT,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_PT_CPU_TAG_DEFAULT_HEADER_CFG1,
#else
	.first_reg = 0xf4304464,
#endif
	.reg_num = 2,
	.reg_stride = APB0_NI_HV_PT_STRIDE
};

ca_status_t aal_ni_pt_cpu_tag_dft_hdra_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t port_id,
	CA_OUT ca_ni_cpu_tag_hdra_t *entry
	)
{
	return aal_direct_table_get(&ni_cpu_tag_dft_hdra_table,port_id,entry,AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_ni_pt_cpu_tag_dft_hdra_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t port_id,
	CA_IN ca_ni_cpu_tag_hdra_t *entry
	)
{
	return aal_direct_table_set(&ni_cpu_tag_dft_hdra_table,port_id,entry,AAL_TABLE_DIR_HI_TO_LOW);
}

aal_direct_table_t ni_cpu_tag_port_id_table = {
	.max_entry = 2,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_GLB_CPU_TAG0_PORT_ID1,
#else
	.first_reg = 0xf43042e4,
#endif
	.reg_num = 2,
	.reg_stride = 80
};

ca_status_t aal_ni_glb_cpu_tag_port_id_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_OUT ca_ni_glb_cpu_tag_port_id_t *reg
	)
{
	return aal_direct_table_get(&ni_cpu_tag_port_id_table,cpu_tag_idx,reg,AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_ni_glb_cpu_tag_port_id_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_IN ca_ni_glb_cpu_tag_port_id_t *reg
	)
{
	return aal_direct_table_set(&ni_cpu_tag_port_id_table,cpu_tag_idx,reg,AAL_TABLE_DIR_HI_TO_LOW);
}

aal_direct_table_t ni_cpu_tag_port_msk_table = {
	.max_entry = 2,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_GLB_CPU_TAG0_PORT_MASK1,
#else
	.first_reg = 0xf43042ec,
#endif
	.reg_num = 2,
	.reg_stride = 80
};

ca_status_t aal_ni_glb_cpu_tag_port_msk_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_OUT ca_ni_glb_cpu_tag_port_mk_t *reg
	)
{
	return aal_direct_table_get(&ni_cpu_tag_port_msk_table,cpu_tag_idx,reg,AAL_TABLE_DIR_HI_TO_LOW);
}

ca_status_t aal_ni_glb_cpu_tag_port_msk_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_IN ca_ni_glb_cpu_tag_port_mk_t *reg
	)
{
	return aal_direct_table_set(&ni_cpu_tag_port_msk_table,cpu_tag_idx,reg,AAL_TABLE_DIR_HI_TO_LOW);
}

aal_direct_table_t ni_cpu_tag0_hdra_table = {
	.max_entry = 8,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_GLB_CPU_TAG0_HDR_A15,
#else
	.first_reg = 0xf43042f4,
#endif
	.reg_num = 2,
	.reg_stride = 8
};

aal_direct_table_t ni_cpu_tag1_hdra_table = {
	.max_entry = 8,
#ifdef CONFIG_ARCH_CORTINA_G3LITE
	.first_reg = NI_HV_GLB_CPU_TAG1_HDR_A15,
#else
	.first_reg = 0xf4304344,
#endif
	.reg_num = 2,
	.reg_stride = 8
};

ca_status_t aal_ni_glb_cpu_tag_hdra_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_IN ca_uint16_t hdra_idx,
	CA_OUT ca_ni_cpu_tag_hdra_t *entry
	)
{
	if ( cpu_tag_idx == 0 )
		return aal_direct_table_get(&ni_cpu_tag0_hdra_table,ni_cpu_tag0_hdra_table.max_entry - hdra_idx - 1,entry,AAL_TABLE_DIR_HI_TO_LOW);
	else if ( cpu_tag_idx == 1 )
		return aal_direct_table_get(&ni_cpu_tag1_hdra_table,ni_cpu_tag1_hdra_table.max_entry - hdra_idx - 1,entry,AAL_TABLE_DIR_HI_TO_LOW);
	else
		return AAL_E_OUTRANGE;
}

ca_status_t aal_ni_glb_cpu_tag_hdra_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint16_t cpu_tag_idx,
	CA_IN ca_uint16_t hdra_idx,
	CA_OUT ca_ni_cpu_tag_hdra_t *entry
	)
{
	if ( cpu_tag_idx == 0 )
		return aal_direct_table_set(&ni_cpu_tag0_hdra_table,ni_cpu_tag0_hdra_table.max_entry - hdra_idx - 1,entry,AAL_TABLE_DIR_HI_TO_LOW);
	else if ( cpu_tag_idx == 1 )
		return aal_direct_table_set(&ni_cpu_tag1_hdra_table,ni_cpu_tag1_hdra_table.max_entry - hdra_idx - 1,entry,AAL_TABLE_DIR_HI_TO_LOW);
	else
		return AAL_E_OUTRANGE;
}


#if 1

/*
REQ:
1.    A script which sets up one GPHY port to loop back to itself through the NE. So for example GPHY 0 RX goes into the NE and the NE programming delivers the packets back to the GPHY 0 TX. This script should have the 4 GPHY ports as a parameter so that we can run it on any of the 4 GPHY ports.
2.    A script which configures the 4 GPHY ports into a snake. In this case GPHY 0 RX should be sent to GPHY 1 TX, and then using a loopback in the GPHY PCS we can loop the packets back to GPHY 1 RX, ,and then back into the chip through GPHY 2 and 3 until GPHY 3 RX sends the packets back to NE and back out GPHY 0 TX. Using this method we can sort chips.
3.    A script which configures the XFI port 6 and port 7 for loop back to itself (this will not work until XFI electrical bringup is complete).
4.    A snake which adds the XFI ports into the snake (in between the GPHY 2 and 3 ports so that 1Gb/s rate is maintained.

*/

ca_status_t aal_ni_test_loop_self_get(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint8_t start_port,
	CA_IN ca_uint8_t end_port,
	CA_OUT ca_boolean_t *enable)
{
	aal_ilpb_cfg_t ilpb_cfg;
	ca_uint8_t     p        = 0;
	ca_status_t    ret      = CA_E_OK;

	if ( start_port > end_port || start_port > 7 || end_port > 7 ) {
		ca_printf("error: invalid input start_port or end_port.\r\n");
		return CA_E_PARAM;
	}

	memset(&ilpb_cfg,0,sizeof(ilpb_cfg));

	for(p = start_port; p <= end_port; p++){
		ret = aal_port_ilpb_cfg_get(dev,p,&ilpb_cfg);
		if ( CA_E_OK == ret ) {
			*enable = ilpb_cfg.loop_enable;
			break;
		}
	}

	return CA_E_OK;

}

ca_status_t aal_ni_test_loop_self_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_uint8_t start_port,
	CA_IN ca_uint8_t end_port,
	CA_IN ca_boolean_t enable)
{
	aal_ilpb_cfg_msk_t ilpb_msk;
	aal_ilpb_cfg_t     ilpb_cfg;
	ca_uint8_t         p        = 0;
	ca_status_t        ret      = CA_E_OK;

	if ( start_port > end_port || start_port > 7 || end_port > 7 ) {
		ca_printf("error: invalid input start_port or end_port.\r\n");
		return CA_E_PARAM;
	}

	memset(&ilpb_cfg,0,sizeof(ilpb_cfg));
	memset(&ilpb_msk,0,sizeof(ilpb_msk));
	ilpb_msk.s.loop_enable = 1;
	ilpb_cfg.loop_enable = enable;

	for(p = start_port; p <= end_port; p++){
		ret = aal_port_ilpb_cfg_set(dev,p,ilpb_msk,&ilpb_cfg);
		if ( CA_E_OK != ret )
			break;
		ca_printf("config port %u done.\r\n",p);
	}

	return ret;

}

static ca_boolean_t _loop_snake = 0;
ca_status_t aal_ni_test_loop_snake_get(
	CA_IN ca_device_id_t dev,
	CA_OUT ca_boolean_t *enable)
{
	if ( NULL == enable )
		return CA_E_PARAM;

	*enable = _loop_snake;
	return CA_E_OK;
}


ca_status_t aal_ni_test_loop_snake_set(
	CA_IN ca_device_id_t dev,
	CA_IN ca_boolean_t enable)
#if 1
{
	aal_ilpb_cfg_msk_t         ilpb_msk;
	aal_ilpb_cfg_t             ilpb_cfg;
	aal_l2_specpkt_ctrl_mask_t l2_sppkt_msk;
	aal_l2_specpkt_ctrl_t      l2_sppkt_ctrl;
	ca_uint8_t                 p             = 0;
	aal_phy_lpbk_mode_t        phy_lpbk;

	ca_status_t                ret           = CA_E_OK;


	memset(&ilpb_cfg,0,sizeof(ilpb_cfg));
	memset(&l2_sppkt_ctrl,0,sizeof(l2_sppkt_ctrl));
	memset(&ilpb_msk,0,sizeof(ilpb_msk));
	memset(&phy_lpbk,0,sizeof(phy_lpbk));
	ilpb_msk.s.loop_enable = 1;
	ilpb_msk.s.spcl_pkt_idx = 1;
	ilpb_cfg.loop_enable = 0;

	l2_sppkt_msk.u32 = 0;
	l2_sppkt_msk.s.spcl_fwd = 1;
	l2_sppkt_msk.s.ldpid = 1;
	l2_sppkt_msk.s.pkt_no_drop = 1;
	if ( enable ) {
		l2_sppkt_ctrl.spcl_fwd = 1;
		l2_sppkt_ctrl.pkt_no_drop = 1;
	} else {
		l2_sppkt_ctrl.spcl_fwd = 0;
		l2_sppkt_ctrl.pkt_no_drop = 0;
	}

	AAL_FOR_ALL_GE_DO(p) {
		ilpb_cfg.spcl_pkt_idx = p;
		ret = aal_port_ilpb_cfg_set(dev,p,ilpb_msk,&ilpb_cfg);
		if ( CA_E_OK != ret ) {
			ca_printf("aal_port_ilpb_cfg_set port %u fail, return %#x.\r\n",p,ret);
			return ret;
		}
		if ( enable ) {
			l2_sppkt_ctrl.ldpid = dpid_mapping[p - AAL_NI_GE_PORT_START];
		} else {
			l2_sppkt_ctrl.ldpid = 0;
		}

		ret = aal_l2_specpkt_ctrl_set(dev,p,AAL_L2_SPECPKT_TYPE_NORMAL,
			l2_sppkt_msk,&l2_sppkt_ctrl);
		if ( CA_E_OK != ret ) {
			ca_printf("aal_l2_specpkt_ctrl_set port %u fail, return %#x.\r\n",p,ret);
			return ret;
		}

		if ( p != AAL_NI_GE_PORT_START ) {
			phy_lpbk.rx2tx_enable = 0;
			phy_lpbk.tx2rx_enable = enable;
			aal_eth_phy_loopback_set(dev,GPHY_MIDO_ADDR(p),phy_lpbk);
		}
		ca_printf("config port %u done.\r\n",p);
	}

	_loop_snake = enable;

	return ret;

}
#else
{
	aal_ilpb_cfg_msk_t ilpb_msk;
	aal_ilpb_cfg_t ilpb_cfg;
	aal_l2_specpkt_ctrl_mask_t l2_sppkt_msk;
	aal_l2_specpkt_ctrl_t l2_sppkt_ctrl;
	ca_uint8_t p = 0;
#ifndef CONFIG_CORTINA_BOARD_FPGA /* ASIC internal GPHY init */
	ca_uint32_t reg_data = 0;
#else
	aal_phy_lpbk_mode_t phy_lpbk;
#endif
	ca_status_t ret = CA_E_OK;
	ca_uint8_t dpid_mapping[] = { // index is src_port, value is dst_port
		1, // 0 -> 1
		2, // 1 -> 2
		3, // 2-> 3
		0, // 3-> 0
	};

	memset(&ilpb_cfg, 0, sizeof(ilpb_cfg));
	memset(&l2_sppkt_ctrl, 0, sizeof(l2_sppkt_ctrl));
	memset(&ilpb_msk, 0, sizeof(ilpb_msk));
	ilpb_msk.s.loop_enable = 1;
	ilpb_msk.s.spcl_pkt_idx = 1;
	ilpb_cfg.loop_enable = 0;

	l2_sppkt_msk.u32 = 0;
	l2_sppkt_msk.s.spcl_fwd = 1;
	l2_sppkt_msk.s.ldpid = 1;
	l2_sppkt_msk.s.pkt_no_drop = 1;
	if (enable) {
		l2_sppkt_ctrl.spcl_fwd = 1;
		l2_sppkt_ctrl.pkt_no_drop = 1;
	} else {
		l2_sppkt_ctrl.spcl_fwd = 0;
		l2_sppkt_ctrl.pkt_no_drop = 0;
	}

    for (p = 0; p < sizeof(dpid_mapping)/sizeof(dpid_mapping[0]); p++) {
		ilpb_cfg.spcl_pkt_idx = p;
		ret = aal_port_ilpb_cfg_set(dev, p, ilpb_msk, &ilpb_cfg);
		if (CA_E_OK != ret) {
			ca_printf("aal_port_ilpb_cfg_set port %u fail, return %#x.\r\n", p, ret);
			return ret;
		}
		if (enable) {
			l2_sppkt_ctrl.ldpid = dpid_mapping[p];
		} else {
			l2_sppkt_ctrl.ldpid = 0;
		}

		ret = aal_l2_specpkt_ctrl_set(dev, p, AAL_L2_SPECPKT_TYPE_NORMAL,
					      l2_sppkt_msk, &l2_sppkt_ctrl);
		if (CA_E_OK != ret) {
			ca_printf("aal_l2_specpkt_ctrl_set port %u fail, return %#x.\r\n", p, ret);
			return ret;
		}

		if (p != 0) {

#ifndef CONFIG_CORTINA_BOARD_FPGA /* ASIC internal GPHY init */
			reg_data = GPHY_MDIO_READ(GPHY_MIDO_ADDR(p), GPHY_REG_CTRL);
			ca_printf("read back reg 0 value = %#x.\r\n", reg_data);
			if (enable)
			reg_data |= 0x4000;
			else
			reg_data &= 0xBFFF;
			GPHY_MDIO_WRITE(GPHY_MIDO_ADDR(p), GPHY_REG_CTRL, reg_data);
#else
			phy_lpbk.rx2tx_enable = 0;
			phy_lpbk.tx2rx_enable = enable;
			aal_eth_phy_loopback_set(dev, GPHY_MIDO_ADDR(p), phy_lpbk);
#endif

		}
		ca_printf("config port %u done.\r\n", p);
	}

	_loop_snake = enable;

	return ret;

}
#endif

#endif
ca_status_t aal_ni_xge_mac_mib_get_uc_tx(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_uint32_t             port_id,
    CA_OUT ca_uint64_t		   *count
)
{
    ca_status_t  ret = CA_E_OK;

    __XGE_PORT_CHK(port_id);
   ret = aal_ni_xge_mac_stats_get(device_id, port_id, AAL_XGE_MAC_STATS_ID_TX_UCAST_FRAMES, count);
   if (ret != CA_E_OK) {
       ca_printf("aal_ni_xge_mac_stats_get  port %d statsid %d return %d.\n", port_id, AAL_XGE_MAC_STATS_ID_TX_UCAST_FRAMES, ret);
   }

   return ret;
}
ca_status_t aal_ni_xge_mac_mib_get_uc_rx(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_uint32_t             port_id,
    CA_OUT ca_uint64_t		   *count
)
{
    ca_status_t  ret = CA_E_OK;
    __XGE_PORT_CHK(port_id);
   ret = aal_ni_xge_mac_stats_get(device_id, port_id, AAL_XGE_MAC_STATS_ID_RX_UCAST_FRAMES, count);
   if (ret != CA_E_OK) {
       ca_printf("aal_ni_xge_mac_stats_get  port %d statsid %d return %d.\n", port_id, AAL_XGE_MAC_STATS_ID_RX_UCAST_FRAMES, ret);
   }

   return ret;
}

#ifdef __KERNEL__

EXPORT_SYMBOL(aal_ni_glb_cpu_tag_port_id_get);
EXPORT_SYMBOL(aal_ni_glb_cpu_tag_port_id_set);
EXPORT_SYMBOL(aal_ni_glb_cpu_tag_port_msk_get);
EXPORT_SYMBOL(aal_ni_glb_cpu_tag_port_msk_set);
EXPORT_SYMBOL(aal_ni_glb_cpu_tag_hdra_get);
EXPORT_SYMBOL(aal_ni_glb_cpu_tag_hdra_set);
EXPORT_SYMBOL(aal_ni_pt_cpu_tag_cfg_get);
EXPORT_SYMBOL(aal_ni_pt_cpu_tag_cfg_set);
EXPORT_SYMBOL(aal_ni_pt_cpu_tag_dft_hdra_get);
EXPORT_SYMBOL(aal_ni_pt_cpu_tag_dft_hdra_set);
EXPORT_SYMBOL(aal_ni_xaui_xgrs_stats_get);
EXPORT_SYMBOL(aal_ni_xge_pause_mac_addr_get);
EXPORT_SYMBOL(aal_ni_glb_intr_status_get);
EXPORT_SYMBOL(aal_ni_ptp_hw_timer_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_ptp_status_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_addr_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_ptp_cfg_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mode_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_ptp_cfg_get);
EXPORT_SYMBOL(aal_ni_eth_port_mac_set);
EXPORT_SYMBOL(aal_ni_port_intr_enable_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mib_ctrl_get);
EXPORT_SYMBOL(aal_ni_xge_mac_intr_en_get);
EXPORT_SYMBOL(aal_ni_xge_pause_quanta_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_tx_ctrl_get);
EXPORT_SYMBOL(aal_ni_xge_mac_intr_status_get);
EXPORT_SYMBOL(aal_ni_init);
EXPORT_SYMBOL(aal_ni_ptp_pps_timer_get);
EXPORT_SYMBOL(aal_ni_pkt_len_set);
EXPORT_SYMBOL(aal_ni_xge_pause_quanta_set);
EXPORT_SYMBOL(aal_ni_ptp_pps_cfg_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_flowctrl_set);
EXPORT_SYMBOL(aal_ni_xge_mac_pause_tx_cmd_set);
EXPORT_SYMBOL(aal_ni_mc_lkup_set);
EXPORT_SYMBOL(aal_ni_port_intr_status_get);
EXPORT_SYMBOL(aal_ni_xge_misc_config_set);
EXPORT_SYMBOL(aal_ni_xge_mac_frame_len_set);
EXPORT_SYMBOL(aal_ni_xge_misc_config_get);
EXPORT_SYMBOL(aal_ni_xge_mac_vlan_get);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_intr_cfg_get);
EXPORT_SYMBOL(aal_ni_eth_if_reset_ctrl_get);
EXPORT_SYMBOL(aal_ni_port_rx_ctrl_get);
EXPORT_SYMBOL(aal_ni_eth_port_status_get);
EXPORT_SYMBOL(aal_ni_glb_intr_enable_get);
EXPORT_SYMBOL(aal_ni_xge_cmn_config_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mode_set);
EXPORT_SYMBOL(aal_ni_xge_mac_vlan_set);
EXPORT_SYMBOL(aal_ni_xaui_xgrs_rx_cfg_get);
EXPORT_SYMBOL(aal_ni_port_rx_ctrl_set);
EXPORT_SYMBOL(aal_ni_xge_tx_config_get);
EXPORT_SYMBOL(aal_ni_xge_mac_addr_set);
EXPORT_SYMBOL(aal_ni_xge_mac_addr_get);
EXPORT_SYMBOL(aal_ni_xge_cmn_config_set);
EXPORT_SYMBOL(aal_ni_xge_pause_mac_addr_set);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_tx_cfg_set);
EXPORT_SYMBOL(aal_ni_eth_if_set);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_tx_cfg_get);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_intr_cfg_set);
EXPORT_SYMBOL(aal_ni_xge_rx_config_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_tx_config_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_rx_ctrl_set);
EXPORT_SYMBOL(aal_ni_pkt_len_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mgm_cfg_set);
EXPORT_SYMBOL(aal_ni_eth_if_reset_ctrl_set);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_rx_cfg_set);
EXPORT_SYMBOL(aal_ni_xge_mac_frame_len_get);
EXPORT_SYMBOL(aal_ni_port_intr_enable_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_rx_ctrl_get);
EXPORT_SYMBOL(aal_ni_xaui_xgrs_tx_cfg_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mgm_cfg_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_rx_config_get);
EXPORT_SYMBOL(aal_ni_port_intr_status_clear);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_intr_status_clear);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_rx_cfg_get);
EXPORT_SYMBOL(aal_ni_eth_port_mib_get);
EXPORT_SYMBOL(aal_ni_eth_port_mac_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_tx_ctrl_set);
EXPORT_SYMBOL(aal_ni_xaui_xgrs_tx_cfg_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_addr_get);
EXPORT_SYMBOL(aal_ni_ptp_pps_cfg_set);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_stats_get);
EXPORT_SYMBOL(aal_ni_xaui_xgrs_rx_cfg_set);
EXPORT_SYMBOL(aal_ni_xge_rx_config_set);
EXPORT_SYMBOL(aal_ni_glb_intr_status_clear);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_flowctrl_get);
EXPORT_SYMBOL(aal_ni_mc_lkup_get);
EXPORT_SYMBOL(aal_ni_eth_if_get);
EXPORT_SYMBOL(aal_ni_xge_mac_pause_tx_cmd_get);
EXPORT_SYMBOL(aal_ni_xge_tx_config_set);
EXPORT_SYMBOL(aal_ni_port_tx_fifo_threshold_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_tx_mib_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_mib_ctrl_set);
EXPORT_SYMBOL(aal_ni_port_tx_fifo_threshold_get);
EXPORT_SYMBOL(aal_ni_glb_intr_enable_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_rx_config_set);
EXPORT_SYMBOL(aal_ni_xge_mac_intr_en_set);
EXPORT_SYMBOL(aal_ni_xge_mac_stats_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_rx_mib_get);
EXPORT_SYMBOL(aal_ni_xaui_xgxs_intr_status_get);
EXPORT_SYMBOL(aal_ni_xaui_mac_tx_config_get);
EXPORT_SYMBOL(aal_ni_xge_flowctrl_get);
EXPORT_SYMBOL(aal_ni_xge_flowctrl_set);
EXPORT_SYMBOL(aal_ni_eth_pfc_quanta_set);
EXPORT_SYMBOL(aal_ni_eth_pfc_quanta_get);
EXPORT_SYMBOL(aal_ni_glb_ptp_tx_cfg_set);
EXPORT_SYMBOL(aal_ni_glb_ptp_tx_cfg_get);
EXPORT_SYMBOL(aal_ni_ptp_hw_timer_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_set);
EXPORT_SYMBOL(aal_ni_xaui_mac_ctrl_get);
EXPORT_SYMBOL(aal_ni_xge_pcs_status_get);
EXPORT_SYMBOL(aal_ni_xge_mac_mib_get);
EXPORT_SYMBOL(aal_ni_glb_ptp_intr_enable_set);
EXPORT_SYMBOL(aal_ni_glb_ptp_intr_enable_get);
EXPORT_SYMBOL(aal_global_gpio_mux_cfg_set);
EXPORT_SYMBOL(aal_global_gpio_mux_cfg_get);
EXPORT_SYMBOL(aal_ni_test_loop_self_get);
EXPORT_SYMBOL(aal_ni_test_loop_self_set);
EXPORT_SYMBOL(aal_ni_test_loop_snake_get);
EXPORT_SYMBOL(aal_ni_test_loop_snake_set);
EXPORT_SYMBOL(aal_ni_rxmux_fc_thrshld_get);
EXPORT_SYMBOL(aal_ni_rxmux_fc_thrshld_set);
EXPORT_SYMBOL(aal_ni_rxmux_monitor_stats_get);
EXPORT_SYMBOL(aal_ni_xge_pfc_quanta_set);
EXPORT_SYMBOL(aal_ni_xge_pfc_quanta_get);
EXPORT_SYMBOL(aal_ni_xge_sxgra_sxgsts_get);
EXPORT_SYMBOL(aal_ni_xge_sxgra_sxgspeed_get);
EXPORT_SYMBOL(aal_ni_xge_sxgra_sxg_intr_cfg_set);
EXPORT_SYMBOL(aal_ni_xge_sxgra_sxg_intr_cfg_get);
EXPORT_SYMBOL(aal_ni_xge_sxgra_intr_status_get);
EXPORT_SYMBOL(aal_ni_xge_sxgra_intr_status_set);
EXPORT_SYMBOL(aal_ni_eth_capability_get);
EXPORT_SYMBOL(aal_ni_xge_prbs_test_set);
EXPORT_SYMBOL(aal_ni_xge_prbs_test_get);
EXPORT_SYMBOL(aal_ni_eee_cfg_set);
EXPORT_SYMBOL(aal_ni_eee_cfg_get);
EXPORT_SYMBOL(aal_ni_xge_eee_cfg_set);
EXPORT_SYMBOL(aal_ni_xge_eee_cfg_get);
EXPORT_SYMBOL(aal_ni_eth_port_config_set);
EXPORT_SYMBOL(aal_ni_reset_intf);
EXPORT_SYMBOL(aal_ni_xge_mac_mib_get_uc_tx);
EXPORT_SYMBOL(aal_ni_xge_mac_mib_get_uc_rx);
#endif

