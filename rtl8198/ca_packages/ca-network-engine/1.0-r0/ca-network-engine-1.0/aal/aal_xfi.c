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
 *
 * aal_xfi.c: Hardware Abstraction Layer for XFI driver
 *
 */
#include <soc/cortina/cortina-soc.h>
#include "osal_common.h"
#include "aal_common.h"
#include "aal_phy.h"
#include "aal_xfi.h"
#include "ca-scfg/scfg.h"
#include "aal_ni.h"
#include "aal_ni_l2.h"

/******************************************************************************/
/*                             Import Variables and Functions                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/
#define __SOC_REVISION_C   1 /* G3 SoC version C */
#define __SOC_REVISION_D   4 /* G3 SoC version D */

#define XFI_STRIDE            0x1000 /* APB0_XFI_STRIDE */
#define SXGMII_STRIDE         0x800  /* NI_HV_SXGMII_ register port stride */
#define XGE_MISC_STRIDE       0x100  /* NI_HV_XGE_MISC register port stride */
#define XFI_AUTONEG_TIMEOUT   20    /* 0.2 seconds, unit: 10ms */

#define XFI_REG_OP_DELAY      100    /* delay after XFI register accessing, unit: us */

#define __XFI_CFG_REG_NUM 74 /* XFI configurable register number */

#define __XFI_PCS_RESET_DELAY   1000 /* us */
#define __SXGMII_RESET_DELAY    30000 /* us */

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define XFI_NUM             APB0_XFI_COUNT
#define XFI_MAC_PORT_START  AAL_NI_PORT_XGE0
#define XFI_BITMAP_DEFAULT  0x03
#define XFI_BITMAP_ALL      0x03

#elif  defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#define XFI_NUM             APB3_XFI_COUNT
#define XFI_MAC_PORT_START  AAL_NI_PORT_XGE1
#define XFI_BITMAP_DEFAULT  0x03
#define XFI_BITMAP_ALL      0x03

#else
#define XFI_NUM             0
#define XFI_MAC_PORT_START  AAL_NI_PORT_XGE0
#define XFI_BITMAP_DEFAULT  0x00
#define XFI_BITMAP_ALL      0x00

#endif

#define FOR_ALL_XFI_DO(port)  for((port)=0; (port)< XFI_NUM; ++(port))

#define XFI_IS_VALID(xfi)  (g_xfi_drv.xfi_bmp & (1 << (xfi)))

#define XFI_PHY_ADDR(xfi)  (g_xfi_drv.xfi_phy_addr[xfi])

/*
 * NI XFI registers read/write MACROs
 */
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define XFI_INTERNAL_INIT_RETRY_TIMES     6
#define XFI_REG_READ(dev, xfi_idx, addr)  (((dev) == CA_DEV_WAN) ? (CA_REG_READ((void *)AAL_8279_NE_REG_ADDR(SATURN_##addr + (xfi_idx) * XFI_STRIDE))) : CA_NE_XFI_REG_READ(addr+ (xfi_idx) * XFI_STRIDE))
#define XFI_REG_WRITE(dev, xfi_idx, addr, data)  do { \
                                           if ((dev) == CA_DEV_WAN) \
                                               CA_REG_WRITE(data, ((void *)AAL_8279_NE_REG_ADDR(SATURN_##addr+ (xfi_idx) * XFI_STRIDE)));\
                                           else \
                                               CA_NE_XFI_REG_WRITE(data, addr+ (xfi_idx) * XFI_STRIDE); \
                                           }while(0)
#else
#define XFI_REG_READ(dev, xfi_idx, addr)        CA_NE_XFI_REG_READ((addr)+ (xfi_idx) * XFI_STRIDE)
#define XFI_REG_WRITE(dev, xfi_idx, addr, data) CA_NE_XFI_REG_WRITE(data, (addr)+ (xfi_idx) * XFI_STRIDE)
#endif

#define CA_NE_XFI_REG_WRITE_WITH_DELAY(data, addr)  do {CA_NE_XFI_REG_WRITE(data, addr); ca_udelay(XFI_REG_OP_DELAY);}while(0)

#define AAL_XFI_IDX_CHK(xfi_idx)         do { \
                                            if ((xfi_idx) >= XFI_NUM) { \
                                                ca_printf("%s(): NOT support xfi_idx %d\r\n", __func__, (xfi_idx)); \
                                                return CA_E_PARAM; \
                                            } \
                                         }while(0)

#define AAL_XFI_SPEED_CHK(speed)         do { \
                                            if ((speed) > AAL_PHY_SPEED_MAX) {\
                                                ca_printf("%s(): NOT support speed %d\r\n", __func__, (speed)); \
                                                return CA_E_PARAM;\
                                            }\
                                         }while(0)

#define AAL_XFI_PHY_ADDR_CHK(phy_addr)   do { \
                                            if ((phy_addr) > 31 || phy_addr < 1) {\
                                                ca_printf("%s(): NOT support phy_addr %d\r\n", __func__, (phy_addr)); \
                                                return CA_E_PARAM;\
                                            }\
                                         }while(0)

struct /* Cortina XFI driver control block*/
{
	ca_uint8_t  chip_version;
	ca_uint8_t  xfi_bmp;
	atomic_t    usxgmii_an_running[XFI_NUM];
	aal_usxgmii_an_tp_t usxgmii_an_tp[XFI_NUM];
	ca_uint8_t  xfi_speed[XFI_NUM];
	ca_uint8_t  xfi_intf_type[XFI_NUM];
	ca_uint8_t  xfi_phy_addr[XFI_NUM];
} g_xfi_drv = {
	.chip_version = __SOC_REVISION_D,
	.xfi_bmp = XFI_BITMAP_DEFAULT,
	.usxgmii_an_tp    = { AAL_USXGMII_AN_TP_HW },
	.xfi_speed = { 0 },
	.xfi_intf_type = { 0 },
	.xfi_phy_addr = { 0 }
};

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
/* Internal SATURN, G3  XFIs CMU, LB configurations to adjust hardware dependence */
static ca_uint32_t g_xfi_rg32_param[4] = { 0x0000827A, 0x0000FFF2, 0x0000FFF2, 0x0000FFFA };
#endif
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define  __XFI0_PCS_RST_REG(reg)  ((reg).bf.pcs_rst_p5)
#define  __XFI1_PCS_RST_REG(reg)  ((reg).bf.pcs_rst_p6)
#define  __XFI0_MAC_RST_REG(reg)  ((reg).bf.mac_rst_p5)
#define  __XFI1_MAC_RST_REG(reg)  ((reg).bf.mac_rst_p6)
#elif defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
#define  __XFI0_PCS_RST_REG(reg)  ((reg).bf.pcs_rst_p6)
#define  __XFI1_PCS_RST_REG(reg)  ((reg).bf.pcs_rst_p7)
#define  __XFI0_MAC_RST_REG(reg)  ((reg).bf.mac_rst_p6)
#define  __XFI1_MAC_RST_REG(reg)  ((reg).bf.mac_rst_p7)
#endif

/** __xfi_chk_hw_enable :
 * @brief: check XFI interface enable or not
 */
static ca_boolean_t  __xfi_chk_hw_enable(
	CA_IN  ca_device_id_t     device_id,
	CA_IN  ca_uint32_t        xfi_idx)
{
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	intf_rst_config.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_GLB_INTF_RST_CONFIG);
	if(xfi_idx == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		return (!__XFI0_PCS_RST_REG(intf_rst_config));
	}

	if(xfi_idx == AAL_SXGMII_PCS_CONTROL_INDEX_1) {
		return (!__XFI1_PCS_RST_REG(intf_rst_config));
	}
	return TRUE;
}

/** __xfi_chk_hw_clk_ready :
 * @brief: check XFI port clock
 */
static inline ca_boolean_t  __xfi_chk_hw_clk_ready(
	CA_IN  ca_device_id_t     device_id,
	CA_IN  ca_uint32_t        xfi_idx)
{
	XFI_RG4D_t  xfi_rg4d;

	if(!__xfi_chk_hw_enable(device_id,xfi_idx)) {
		ca_printf("%s: dev %d xfi %d interface is not out of reset!\n",__func__,device_id,xfi_idx);
		return FALSE;
	}

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(device_id == CA_DEV_WAN) {
		xfi_rg4d.wrd = CA_HGU_8279_NE_REG_READ(SATURN_XFI_RG4D + xfi_idx * XFI_STRIDE);
	} else
#endif
		xfi_rg4d.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + xfi_idx * XFI_STRIDE);

	ca_printf("ca-ne: device %d xfi %d (XFI_RG4D=%#x), clk_rdy = %d, rxidle = %d, ber_notify = %d!\n",
		device_id,xfi_idx,xfi_rg4d.wrd,xfi_rg4d.bf.clk_rdy,xfi_rg4d.bf.rxidle,xfi_rg4d.bf.ber_notify);

	return (xfi_rg4d.bf.clk_rdy);
}

/** __sxgmii_init :
 * @brief: XFI connected SXGMII interface initialization
 */
static  void __sxgmii_init(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t xfi,
	ca_uint32_t speed)
{
	ca_uint32_t sxgmii_offset = xfi * SXGMII_STRIDE;
	ca_uint32_t xge_offset    = xfi * XGE_MISC_STRIDE;


	CA_DEV_NI_REG_WRITE(device_id,0x00001800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);
	ca_mdelay(5);
	CA_DEV_NI_REG_WRITE(device_id,0x00003800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);
	ca_mdelay(5);

	if(speed <= AAL_PHY_SPEED_1000) {
		CA_DEV_NI_REG_WRITE(device_id,0x800000ff,NI_HV_XGE_MISC_CONFIG + xge_offset);
		CA_DEV_NI_REG_WRITE(device_id,0x80000001,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
		ca_udelay(__SXGMII_RESET_DELAY);
		CA_DEV_NI_REG_WRITE(device_id,0x00000001,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
		CA_DEV_NI_REG_WRITE(device_id,0x00,NI_HV_SXGMII_SFP_PCS_DEBUG + sxgmii_offset);
	} else if(speed == AAL_PHY_SPEED_2500 || speed == AAL_PHY_SPEED_2000) {
		CA_DEV_NI_REG_WRITE(device_id,0x800000ff,NI_HV_XGE_MISC_CONFIG + xge_offset);
		CA_DEV_NI_REG_WRITE(device_id,0x80000011,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
		ca_udelay(__SXGMII_RESET_DELAY);
		CA_DEV_NI_REG_WRITE(device_id,0x00000011,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
		CA_DEV_NI_REG_WRITE(device_id,0x14,NI_HV_SXGMII_SFP_PCS_DEBUG + sxgmii_offset);
	} else if(speed == AAL_PHY_SPEED_10000 || speed == AAL_PHY_SPEED_5000) {
		CA_DEV_NI_REG_WRITE(device_id,0x000000ff,NI_HV_XGE_MISC_CONFIG + xge_offset);
		CA_DEV_NI_REG_WRITE(device_id,0x80000000,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
		ca_udelay(__SXGMII_RESET_DELAY);
		CA_DEV_NI_REG_WRITE(device_id,0x00000000,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
	}
	ca_mdelay(10);
}

static void inline
__xfi_inte_rstr(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t xfi)
{
	aal_ni_xge_sxgra_sxg_intr_cfg_mask_t mask;
	aal_ni_xge_sxgra_sxg_intr_cfg_t      cfg;

	memset(&mask,0x1,sizeof( aal_ni_xge_sxgra_sxg_intr_cfg_mask_t ));
	memset(&cfg,0x0,sizeof( aal_ni_xge_sxgra_sxg_intr_cfg_t ));

	aal_ni_xge_sxgra_sxg_intr_cfg_get(device_id,xfi,&cfg);
	cfg.linkup = 1;
	cfg.hwancomplete = 1;
	aal_ni_xge_sxgra_sxg_intr_cfg_set(device_id,xfi,mask,&cfg);

	return;
}

/** __xfi_pcs_reset :
 * @brief: XFI PCS interface reset
 */
static void
__xfi_pcs_rst_oper(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t xfi,
	aal_phy_sxgmii_pcs_control_oper_t oper)
{

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3)
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		__XFI0_PCS_RST_REG(intf_rst_config) = oper;
	} else {
		__XFI1_PCS_RST_REG(intf_rst_config) = oper;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(device_id == CA_DEV_WAN) {
		SATURN_NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;
		intf_rst_config.wrd = CA_HGU_8279_NE_REG_READ(SATURN_NI_HV_GLB_INTF_RST_CONFIG);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p5 = oper;
		} else {
			intf_rst_config.bf.pcs_rst_p6 = oper;
		}
		CA_HGU_8279_NE_REG_WRITE(intf_rst_config.wrd,SATURN_NI_HV_GLB_INTF_RST_CONFIG);
	} else {
		NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;
		intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p6 = oper;
		} else {
			intf_rst_config.bf.pcs_rst_p7 = oper;
		}
		CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
	}
#endif

	ca_udelay(__XFI_PCS_RESET_DELAY);
	/*
	 *  Restore inte of xfi.
	 */
	__xfi_inte_rstr(device_id,xfi);

	return CA_E_OK;
}

static void __xfi_pcs_reset_with_sleep(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t xfi)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3)
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		__XFI0_PCS_RST_REG(intf_rst_config) = 1;
	} else {
		__XFI1_PCS_RST_REG(intf_rst_config) = 1;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

	msleep(2);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		__XFI0_PCS_RST_REG(intf_rst_config) = 0;
	} else {
		__XFI1_PCS_RST_REG(intf_rst_config) = 0;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)

	if(device_id == CA_DEV_WAN) {
		SATURN_NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;
		intf_rst_config.wrd = CA_HGU_8279_NE_REG_READ(SATURN_NI_HV_GLB_INTF_RST_CONFIG);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p5 = 1;
		} else {
			intf_rst_config.bf.pcs_rst_p6 = 1;
		}
		CA_HGU_8279_NE_REG_WRITE(intf_rst_config.wrd,SATURN_NI_HV_GLB_INTF_RST_CONFIG);
		msleep(2);
		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p5 = 0;
		} else {
			intf_rst_config.bf.pcs_rst_p6 = 0;
		}
		CA_HGU_8279_NE_REG_WRITE(intf_rst_config.wrd,SATURN_NI_HV_GLB_INTF_RST_CONFIG);

	} else {
		NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;
		intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p6 = 1;
		} else {
			intf_rst_config.bf.pcs_rst_p7 = 1;
		}
		CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
		msleep(2);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p6 = 0;
		} else {
			intf_rst_config.bf.pcs_rst_p7 = 0;
		}
		CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
	}
#endif

	msleep(30);

}


/** __xfi_speed_update :
 * @brief: Update the dynamic speed configuration to local speed variable
 * @note: it will impact the XFI parameters if change the speed
 */
static inline
ca_status_t
__xfi_speed_update(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_idx,
	ca_uint32_t    speed)
{
	if(unlikely(speed >= AAL_PHY_SPEED_MAX)) {
		ca_printf("%s(): error: unkown speed %d, cannot support\r\n",__func__,speed);
		return  CA_E_PARAM;
	}

	g_xfi_drv.xfi_speed[pcs_idx] = speed;

	return CA_E_OK;
}

/** __xge_mac_reset :
 * @brief: Reset one of the two XGE MAC based on pcs_index
 */
static inline void
__xge_mac_reset(
	ca_device_id_t        device_id,
	ca_uint32_t           pcs_index,
	aal_xfi_xge_mac_cfg_t oper)
{
	if(AAL_XGE_MAC_CFG_RESET == oper) {
		CA_DEV_NI_REG_WRITE(device_id,0xc014,NI_HV_XGE_MAC_CFG_COM + XGE_MISC_STRIDE * pcs_index);
		ca_mdelay(1);
	} else if(AAL_XGE_MAC_CFG_NORMAL == oper) {
		CA_DEV_NI_REG_WRITE(device_id,0x0014,NI_HV_XGE_MAC_CFG_COM + XGE_MISC_STRIDE * pcs_index);
		ca_mdelay(1);
	}
}

/** __xfi_rx_data_enable:
 * @brief: Set XFI port Rx data enable/disable
 */
static inline void
__xfi_rx_data_en_cfg(
	ca_device_id_t  device_id,
	ca_uint32_t     pcs_index,
	ca_boolean_t    enable)
{
	XFI_RG46_t   rg46;

	rg46.wrd = XFI_REG_READ(device_id,pcs_index,XFI_RG46);
	rg46.bf.RX_DATA_EN = (enable != 0);
	XFI_REG_WRITE(device_id,pcs_index,XFI_RG46,rg46.wrd);
	ca_mdelay(1);
}

static inline void
__xfi_rx_en_cfg(
	ca_device_id_t  device_id,
	ca_uint32_t     pcs_index,
	ca_boolean_t    enable)
{
	XFI_RG46_t   rg46;

	rg46.wrd = XFI_REG_READ(device_id,pcs_index,XFI_RG46);
	rg46.bf.RX_EN = (enable != 0);
	XFI_REG_WRITE(device_id,pcs_index,XFI_RG46,rg46.wrd);
	ca_mdelay(1);
}


/**aal_xfi_link_toggle_rx_en
 *@brief: workaround for XFI link NOT stable when plug out/in fiber/module.
 *       toggle the XFI Rx_EN when XFI Rx link fail can prevent link fail
 *       when plug out/in fiber/module
 *@note: can only be invoked in kernel thread but NOT ISR/BH
 */
ca_status_t
aal_xfi_link_toggle_rx_en(
	CA_IN  ca_device_id_t    device_id,
	CA_IN  ca_uint32_t       xfi_idx)
{
	aal_xfi_cfg_t      config;
	aal_xfi_cfg_mask_t mask;
	ca_uint32_t        speed;

	aal_xfi_speed_set(device_id,xfi_idx,g_xfi_drv.xfi_speed[xfi_idx]);

	memset(&config,0,sizeof(config));
	mask.data32 = 0;
	mask.s.rx_en = 1;
	config.rx_en = 0;
	aal_xfi_cfg_set(device_id,xfi_idx,mask,&config);

	config.rx_en = 1;
	aal_xfi_cfg_set(device_id,xfi_idx,mask,&config);

	return CA_E_OK;
}

/** aal_xfi_link_status_get :
 * @brief: Get XFI port link status
 */
ca_status_t
aal_xfi_link_status_get(
	CA_IN  ca_device_id_t    device_id,
	CA_IN  ca_uint32_t       xfi_idx,
	CA_OUT ca_boolean_t     *link_up)
{
	NI_HV_SXGMII_SXGRA_PCSCTL_t pcs_ctl;

	AAL_XFI_IDX_CHK(xfi_idx);
	CA_NULL_PTR_CHECK(link_up);

	if(!__xfi_chk_hw_enable(device_id,xfi_idx)) {
#ifdef DEBUG
		ca_printf("dev %d xfi %d interface is NOT ready.\n",device_id,xfi_idx);
#endif
		*link_up = FALSE;
		return CA_E_OK;
	}

	/* work-around: read twice to gurantee retriving the latest link status, clear old status */
	pcs_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_PCSCTL + (xfi_idx * SXGMII_STRIDE));

	if(pcs_ctl.bf.pcs1gen) {
		/* 1G, 2.5G link */
		NI_HV_SXGMII_SFP_PCS_STATUS_t  sfp_sts;
		sfp_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SFP_PCS_STATUS + (xfi_idx * SXGMII_STRIDE));
		sfp_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SFP_PCS_STATUS + (xfi_idx * SXGMII_STRIDE));
		*link_up = sfp_sts.bf.LinkStat;
	} else {
		/* XG link */
		NI_HV_SXGMII_PCS_PCSSTS1_t  pcs_sts;
		pcs_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTS1 + (xfi_idx * SXGMII_STRIDE));
		pcs_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTS1 + (xfi_idx * SXGMII_STRIDE));
		*link_up = pcs_sts.bf.linkup;
	}

	return CA_E_OK;

}

/** aal_xfi_cfg_get :
 * @brief: Get XFI normal configurations like mode, enable
 */
ca_status_t aal_xfi_cfg_get(
	CA_IN  ca_device_id_t     device_id,
	CA_IN  ca_uint32_t        xfi_idx,
	CA_OUT aal_xfi_cfg_t     *config)
{
	XFI_RG00_t                      rg00;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	XFI_RGNEW5_t                    rgnew5;
#endif
	XFI_RG46_t                      rg46;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t an_ctl;

	rg00.wrd   = XFI_REG_READ(device_id,xfi_idx,XFI_RG00);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	rgnew5.wrd = XFI_REG_READ(device_id,xfi_idx,XFI_RGNEW5);
#endif
	rg46.wrd   = XFI_REG_READ(device_id,xfi_idx,XFI_RG46);
	an_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + xfi_idx * SXGMII_STRIDE);

	config->txdly      = rg00.bf.REG0_TXDLY;
	config->mode_125m  = rg00.bf.REG0_125M_MODE;
	config->spdsel     = rg00.bf.REG0_SPDSEL;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	config->dfe_en     = rgnew5.bf.DFE_EN;
	config->leq_en     = rgnew5.bf.LEQ_EN;
	config->ckref_sel  = rgnew5.bf.REG_CKREF_SEL;
#endif
	config->pdown      = rg46.bf.PDOWN;
	config->rx_en      = rg46.bf.RX_EN;
	config->pow_pcix   = rg46.bf.POW_PCIX;
	config->rx_data_en = rg46.bf.RX_DATA_EN;
	config->autoneg_link_timer = an_ctl.bf.linktmr;

	return CA_E_OK;
}

/** aal_xfi_cfg_set :
 * @brief: Set XFI normal configurations like mode, enable
 */
ca_status_t aal_xfi_cfg_set(
	CA_IN ca_device_id_t     device_id,
	CA_IN ca_uint32_t        xfi_idx,
	CA_IN aal_xfi_cfg_mask_t mask,
	CA_IN aal_xfi_cfg_t *config)
{
	XFI_RG00_t                      rg00;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	ca_uint32_t                     wr_rgnew5;
	XFI_RGNEW5_t                    rgnew5;
#endif
	XFI_RG46_t                      rg46,
					rg46_old;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t an_ctl;

	AAL_XFI_IDX_CHK(xfi_idx);
	CA_NULL_PTR_CHECK(config);

	if(!mask.data32)
		return CA_E_OK;

	rg00.wrd   = XFI_REG_READ(device_id,xfi_idx,XFI_RG00);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	rgnew5.wrd = XFI_REG_READ(device_id,xfi_idx,XFI_RGNEW5);
#endif
	rg46.wrd   = XFI_REG_READ(device_id,xfi_idx,XFI_RG46);
	rg46_old.wrd = rg46.wrd;

	if(mask.s.autoneg_link_timer) {
		an_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + xfi_idx * SXGMII_STRIDE);
		an_ctl.bf.linktmr = config->autoneg_link_timer;
		CA_DEV_NI_REG_WRITE(device_id,an_ctl.wrd,NI_HV_SXGMII_SXGRA_SXGHWANCTL + xfi_idx * SXGMII_STRIDE);
		ca_printf("%s:%d: NI_HV_SXGMII_SXGRA_SXGHWANCTL[%d] = %#x\n",__func__,__LINE__,xfi_idx,
			CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + xfi_idx * SXGMII_STRIDE));
	}

	if(mask.s.txdly)
		rg00.bf.REG0_TXDLY  = config->txdly;
	if(mask.s.mode_125m)
		rg00.bf.REG0_125M_MODE = config->mode_125m;
	if(mask.s.spdsel)
		rg00.bf.REG0_SPDSEL = config->spdsel;
	if(mask.s.txdly || mask.s.mode_125m || mask.s.spdsel) {
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RG00,rg00.wrd);
		ca_udelay(1000);
	}

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	if(mask.s.dfe_en) {
		rgnew5.bf.DFE_EN = config->dfe_en;
		wr_rgnew5 = 1;
	}
	if(mask.s.leq_en) {
		rgnew5.bf.LEQ_EN = config->leq_en;
		wr_rgnew5 = 1;
	}
	if(mask.s.ckref_sel) {
		rgnew5.bf.REG_CKREF_SEL = config->ckref_sel;
		wr_rgnew5 = 1;
	}
	if(wr_rgnew5) {
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RGNEW5,rgnew5.wrd);
		ca_udelay(1000);
	}
#endif
	if(mask.s.pdown)
		rg46.bf.PDOWN = config->pdown;
	if(mask.s.rx_en)
		rg46.bf.RX_EN = config->rx_en;
	if(mask.s.rx_data_en)
		rg46.bf.RX_DATA_EN = config->rx_data_en;
	if(mask.s.pow_pcix)
		rg46.bf.POW_PCIX = config->pow_pcix;

	if(!rg46.bf.PDOWN && rg46_old.bf.PDOWN) { /* power up sequence */
		rg46_old.bf.RX_EN = 0; /* disable all Rx to prevent rx packet to XFI when reset */
		rg46_old.bf.RX_DATA_EN = 0;
		rg46_old.bf.RX_CLKWR_EN = 0;
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RG46,rg46_old.wrd);
		ca_udelay(1000);
		rg46_old.bf.PDOWN = 0;
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RG46,rg46_old.wrd); /* power up */
		ca_udelay(1000); /* delay after power up */
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RG46,rg46.wrd); /* apply new configuration */

		__xfi_pcs_rst_oper(device_id,xfi_idx,AAL_SXGMII_PCS_CONTROL_OPER_RESET);
		__xfi_pcs_rst_oper(device_id,xfi_idx,AAL_SXGMII_PCS_CONTROL_OPER_NORMAL);
	} else if(rg46.wrd != rg46_old.wrd) { /* change other configurations */
		XFI_REG_WRITE(device_id,xfi_idx,XFI_RG46,rg46.wrd);
		ca_udelay(1000);
	}

	return CA_E_OK;
}

/** aal_xfi_status_get :
 * @brief: Get XFI clock/link status
 */
ca_status_t aal_xfi_status_get(
	CA_IN  ca_device_id_t     device_id,
	CA_IN  ca_uint32_t        xfi_idx,
	CA_OUT aal_xfi_status_t *status)
{
	XFI_RG4D_t  rg4d;

	AAL_XFI_IDX_CHK(xfi_idx);
	CA_NULL_PTR_CHECK(status);

	rg4d.wrd = XFI_REG_READ(device_id,xfi_idx,XFI_RG4D);
	status->ber_notify = rg4d.bf.ber_notify;
	status->rxidle     = rg4d.bf.rxidle;
	status->clk_rdy    = rg4d.bf.clk_rdy;

	return CA_E_OK;

}

/** aal_xfi_pcs_sxgmii_reset :
 * @brief: XFI PCS & SXGMII interface reset
 */
void aal_xfi_pcs_sxgmii_reset(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t xfi)
{
	ca_uint32_t                 sxgmii_offset   = xfi * SXGMII_STRIDE;

		  #if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	CA_NI_REG_WRITE(0x00001800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);
	ca_udelay(2000);
	CA_NI_REG_WRITE(0x00003800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);

	intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		intf_rst_config.bf.pcs_rst_p5 = 1;
	} else {
		intf_rst_config.bf.pcs_rst_p6 = 1;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		intf_rst_config.bf.pcs_rst_p5 = 0;
	} else {
		intf_rst_config.bf.pcs_rst_p6 = 0;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(device_id == CA_DEV_WAN) {
		SATURN_NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;
		intf_rst_config.wrd = CA_HGU_8279_NE_REG_READ(SATURN_NI_HV_GLB_INTF_RST_CONFIG);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p5 = 1;
		} else {
			intf_rst_config.bf.pcs_rst_p6 = 1;
		}
		CA_HGU_8279_NE_REG_WRITE(intf_rst_config.wrd,SATURN_NI_HV_GLB_INTF_RST_CONFIG);

		ca_udelay(2000);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p5 = 0;
		} else {
			intf_rst_config.bf.pcs_rst_p6 = 0;
		}
		CA_HGU_8279_NE_REG_WRITE(intf_rst_config.wrd,SATURN_NI_HV_GLB_INTF_RST_CONFIG);

	} else {
		NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;

		CA_NI_REG_WRITE(0x00001800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);
		ca_udelay(2000);
		CA_NI_REG_WRITE(0x00003800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);

		intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p6 = 1;
		} else {
			intf_rst_config.bf.pcs_rst_p7 = 1;
		}
		CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

		if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
			intf_rst_config.bf.pcs_rst_p6 = 0;
		} else {
			intf_rst_config.bf.pcs_rst_p7 = 0;
		}
		CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
	}
#elif defined(CONFIG_ARCH_CORTINA_G3)
	NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;

	CA_NI_REG_WRITE(0x00001800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);

	ca_udelay(2000);

	CA_NI_REG_WRITE(0x00003800,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + sxgmii_offset);

	intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		intf_rst_config.bf.pcs_rst_p6 = 1;
	} else {
		intf_rst_config.bf.pcs_rst_p7 = 1;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);

	if(xfi == AAL_SXGMII_PCS_CONTROL_INDEX_0) {
		intf_rst_config.bf.pcs_rst_p6 = 0;
	} else {
		intf_rst_config.bf.pcs_rst_p7 = 0;
	}
	CA_NI_REG_WRITE(intf_rst_config.wrd,NI_HV_GLB_INTF_RST_CONFIG);
#endif

	ca_udelay(__XFI_PCS_RESET_DELAY);

}

/** aal_xfi_reset :
 * @brief: XFI all interfaces reset
 */
void aal_xfi_reset(ca_device_id_t device_id, aal_phy_sxgmii_pcs_control_index_t xfi)
{

	aal_xfi_pcs_sxgmii_reset(device_id,xfi);

	aal_xfi_speed_set(device_id,xfi,g_xfi_drv.xfi_speed[xfi]);

#ifdef _DEBUG_
	ca_printf("%s(): reset XFI %d done\r\n",__func__,xfi);
#endif
}

#if 0 //defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
/* the following code is for old G3 XFI rxidle work-around */
static ca_uint32_t  __old_g3_rxidle_d[XFI_NUM] = {
	0
};
static ca_uint32_t  __old_saturn_rxidle = 0;
static ca_uint32_t  __old_g3_rxidle = 0;

static void __xfi_update_rxidle(void){
	XFI_RG4D_t  xfi_rg4d;
	XFI_RG46_t  xfi_rg46;
	int i = 0;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	xfi_rg4d.wrd = CA_HGU_8279_NE_REG_READ(SATURN_XFI_RG4D + XFI_STRIDE);
	__old_saturn_rxidle = xfi_rg4d.bf.rxidle;
	xfi_rg4d.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + XFI_STRIDE);
	__old_g3_rxidle = xfi_rg4d.bf.rxidle;
#endif

	FOR_ALL_XFI_DO(i) {
		xfi_rg46.wrd = CA_NE_XFI_REG_READ(XFI_RG46 + i * XFI_STRIDE);
		__old_g3_rxidle_d[i] = xfi_rg46.bf.RXIDLE_D;
	}

#ifdef DEBUG
	ca_printf("__old_saturn_rxidle = %d, __old_g3_rxidle = %d, __old_g3_rxidle_d[0] = %d, __old_g3_rxidle_d[1] = %d\n",
		  __old_saturn_rxidle, __old_g3_rxidle, __old_g3_rxidle_d[0], __old_g3_rxidle_d[1]);
#endif
}

void aal_xfi_rxidle_poll(void){
	int         i = 0;
	XFI_RG4D_t  g3_rg4d;
	XFI_RG46_t  g3_rg46;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	XFI_RG4D_t  saturn_rg4d;
	XFI_RG46_t  saturn_rg46;

	saturn_rg4d.wrd = CA_HGU_8279_NE_REG_READ(SATURN_XFI_RG4D + XFI_STRIDE);
#endif

	g3_rg4d.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + XFI_STRIDE);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	if ((!g3_rg4d.bf.rxidle && __old_g3_rxidle) || (!saturn_rg4d.bf.rxidle && __old_saturn_rxidle)) {
		ca_printf("Internal XFI link loss, g3_rg4d.bf.rxidle = %d, saturn_rg4d.bf.rxidle = %d, so reset xfi.\n",
			  g3_rg4d.bf.rxidle, saturn_rg4d.bf.rxidle);
		saturn_rg46.wrd = CA_HGU_8279_NE_REG_READ(SATURN_XFI_RG46 + XFI_STRIDE);
		g3_rg46.wrd = CA_NE_XFI_REG_READ(XFI_RG46 + XFI_STRIDE);
		saturn_rg46.bf.RX_EN = 0;
		g3_rg46.bf.RX_EN     = 0;
		CA_HGU_8279_NE_REG_WRITE(saturn_rg46.wrd, SATURN_XFI_RG46 + XFI_STRIDE);
		CA_NE_XFI_REG_WRITE(g3_rg46.wrd, XFI_RG46 + XFI_STRIDE);
		ca_udelay(100);
		saturn_rg46.wrd = CA_DEV_XFI_REG_READ(CA_DEV_WAN, XFI_RG46 + XFI_STRIDE);
		g3_rg46.wrd = CA_DEV_XFI_REG_READ(CA_DEV_LAN, XFI_RG46 + XFI_STRIDE);
		saturn_rg46.bf.RX_EN = 1;
		g3_rg46.bf.RX_EN     = 1;
		CA_HGU_8279_NE_REG_WRITE(saturn_rg46.wrd, SATURN_XFI_RG46 + XFI_STRIDE);
		CA_NE_XFI_REG_WRITE(g3_rg46.wrd, XFI_RG46 + XFI_STRIDE);
		ca_udelay(100);
		__xfi_pcs_reset(CA_DEV_WAN, 1);
		__xfi_pcs_reset(CA_DEV_LAN, 1);
	}
#endif

	FOR_ALL_XFI_DO(i) {
		g3_rg46.wrd = CA_NE_XFI_REG_READ(XFI_RG46 + i * XFI_STRIDE);
		if (g3_rg46.bf.RXIDLE_D != __old_g3_rxidle_d[i]) {
			__old_g3_rxidle_d[i] = g3_rg46.bf.RXIDLE_D;
			g3_rg4d.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + i * XFI_STRIDE);
			g3_rg4d.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + i * XFI_STRIDE);
			g3_rg46.wrd = CA_NE_XFI_REG_READ(XFI_RG46 + i * XFI_STRIDE);
			if (g3_rg46.bf.RXIDLE_D != g3_rg4d.bf.rxidle) {
				g3_rg46.bf.RXIDLE_D = g3_rg4d.bf.rxidle;
				CA_NE_XFI_REG_WRITE(g3_rg46.wrd, XFI_RG46 + i * XFI_STRIDE);
				ca_printf("software de-glitch for internal XFI %d link done.\n", i);
			}
		}
	}
}
#endif


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
/* DFE adaption */
static void
__xfi_do_dfe_adaption(
	ca_device_id_t                     device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t                    xfi_offset,
				       sxgmii_offset;
	ca_uint32_t                    tap0,
				       vthp,
				       vthn,
				       tap1_even,
				       tap1_odd;
	XFI_RG13_t                     rg13;
	XFI_RG43_t                     rg43;
	XFI_RG0F_t                     rg0f;
	XFI_RG4A_t                     rg4a;
	XFI_RG20_t                     rg20;
	XFI_RG2E_t                     rg2e;
	XFI_RG23_t                     rg23;
	XFI_RG46_t                     rg46;
	NI_HV_SXGMII_PCS_PCSREG32770_t pcs_reg32770;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	XFI_RGNEW5_t                   rgnew5;
#endif

	xfi_offset = pcs_index * XFI_STRIDE;
	sxgmii_offset = pcs_index * SXGMII_STRIDE;

	/* enable tap0/vth/tap1 adaption */
	rg13.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG13 + xfi_offset);
	rg13.bf.reg0_load_in_init = 0x3c;
	CA_DEV_XFI_REG_WRITE(device_id,rg13.wrd,XFI_RG13 + xfi_offset);
	ca_mdelay(1);

	rg43.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG43 + xfi_offset);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	rg43.bf.reg0_dfe_adapt_eqen = 1;
#endif
	CA_DEV_XFI_REG_WRITE(device_id,rg43.wrd,XFI_RG43 + xfi_offset);
	ca_mdelay(1);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	rgnew5.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RGNEW5 + xfi_offset);
	rgnew5.bf.DFE_EN = 1;
	CA_DEV_XFI_REG_WRITE(device_id,rgnew5.wrd,XFI_RGNEW5 + xfi_offset);
	ca_mdelay(1);
#endif

	/* read initial value of tap0/vth/tap1 */
	rg0f.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG0F + xfi_offset);
	rg0f.bf.reg0_coef_sel = 0;
	CA_DEV_XFI_REG_WRITE(device_id,rg0f.wrd,XFI_RG0F + xfi_offset);
	ca_mdelay(1);

	rg4a.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG4A + xfi_offset);
	tap0 = rg4a.bf.S0_coef_data;

	rg0f.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG0F + xfi_offset);
	rg0f.bf.reg0_coef_sel = 0xc;
	CA_DEV_XFI_REG_WRITE(device_id,rg0f.wrd,XFI_RG0F + xfi_offset);
	ca_mdelay(1);

	rg4a.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG4A + xfi_offset);
	vthp = rg4a.bf.S0_coef_data & 0x7;
	vthn = (rg4a.bf.S0_coef_data & 0x38) >> 3;

	rg0f.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG0F + xfi_offset);
	rg0f.bf.reg0_coef_sel = 1;
	CA_DEV_XFI_REG_WRITE(device_id,rg0f.wrd,XFI_RG0F + xfi_offset);
	ca_mdelay(1);

	rg4a.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG4A + xfi_offset);
	tap1_even = rg4a.bf.S0_coef_data & 0x7f;

	rg0f.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG0F + xfi_offset);
	rg0f.bf.reg0_coef_sel = 6;
	CA_DEV_XFI_REG_WRITE(device_id,rg0f.wrd,XFI_RG0F + xfi_offset);
	ca_mdelay(1);

	rg4a.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG4A + xfi_offset);
	tap1_odd = rg4a.bf.S0_coef_data & 0x7f;

	ca_mdelay(1);
#if 0
	ca_printf("%s: tap0=0x%x, vthp=0x%x, vthn=0x%x, tap1_even=0x%x, tap1_odd=0x%x\n",
		  __func__, tap0, vthp, vthn, tap1_even, tap1_odd);
#endif

	/* write initial values to tap0/vth/tap1 init */
	rg20.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG20 + xfi_offset);
	rg20.bf.reg0_tap0_init = tap0;
	CA_DEV_XFI_REG_WRITE(device_id,rg20.wrd,XFI_RG20 + xfi_offset);
	ca_mdelay(1);

	rg2e.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG2E + xfi_offset);
	rg2e.bf.reg0_vthp_init = vthp;
	rg2e.bf.reg0_vthn_init = vthn;
	CA_DEV_XFI_REG_WRITE(device_id,rg2e.wrd,XFI_RG2E + xfi_offset);
	ca_mdelay(1);

	rg23.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG23 + xfi_offset);
	rg23.bf.reg0_tap1_init_even = tap1_even;
	rg23.bf.reg0_tap1_init_odd = tap1_odd;
	CA_DEV_XFI_REG_WRITE(device_id,rg23.wrd,XFI_RG23 + xfi_offset);
	ca_mdelay(1);

	/* disable tap0/vth/tap1 adaption */
	rg13.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG13 + xfi_offset);
	rg13.bf.reg0_load_in_init = 0x7f;
	CA_DEV_XFI_REG_WRITE(device_id,rg13.wrd,XFI_RG13 + xfi_offset);
	ca_mdelay(1);

	rg43.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG43 + xfi_offset);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	rg43.bf.reg0_dfe_adapt_eqen = 0;
	CA_DEV_XFI_REG_WRITE(device_id,rg43.wrd,XFI_RG43 + xfi_offset);
	ca_mdelay(1);
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	rgnew5.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RGNEW5 + xfi_offset);
	rgnew5.bf.DFE_EN = 0;
	CA_DEV_XFI_REG_WRITE(device_id,rgnew5.wrd,XFI_RGNEW5 + xfi_offset);
	ca_mdelay(1);
#endif

	/* toggle RX_EN to perform foreground & DCVS offset calibration again */
	rg46.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG46 + xfi_offset);
	rg46.bf.RX_EN = 0;
	CA_DEV_XFI_REG_WRITE(device_id,rg46.wrd,XFI_RG46 + xfi_offset);
	ca_mdelay(1);

	rg46.wrd = CA_DEV_XFI_REG_READ(device_id,XFI_RG46 + xfi_offset);
	rg46.bf.RX_EN = 1;
	CA_DEV_XFI_REG_WRITE(device_id,rg46.wrd,XFI_RG46 + xfi_offset);
	ca_mdelay(1);

	/* do PCS tx/rx reset */
	pcs_reg32770.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSREG32770 + sxgmii_offset);
	pcs_reg32770.wrd |= 0x1000;
	CA_DEV_NI_REG_WRITE(device_id,pcs_reg32770.wrd,NI_HV_SXGMII_PCS_PCSREG32770 + sxgmii_offset);
	ca_mdelay(1);

	pcs_reg32770.wrd &= 0xffffefff;
	CA_DEV_NI_REG_WRITE(device_id,pcs_reg32770.wrd,NI_HV_SXGMII_PCS_PCSREG32770 + sxgmii_offset);
	ca_mdelay(1);
}
#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __G3_XFI_CFG_REG_NUM 73
ca_uint32_t xfi_xg_init_tbl_g3_revc[__G3_XFI_CFG_REG_NUM][3] = { /* XG config for reversion C */
	{ XFI_RG00, 0x000060f0, 0x000060f0 },
	{ XFI_RG01, 0x00004806, 0x00004806 },
	{ XFI_RG02, 0x00001447, 0x00001447 },
	{ XFI_RG03, 0x0000C71D, 0x0000C71D },
	{ XFI_RG04, 0x0000AAA8, 0x0000AAA8 },
	{ XFI_RG05, 0x00005523, 0x00005523 },
	{ XFI_RG06, 0x00006003, 0x00006003 },
	{ XFI_RG07, 0x0000FFFF, 0x0000FFFF },
	{ XFI_RG08, 0x0000326A, 0x0000326A },
	{ XFI_RG09, 0x00008B40, 0x00008B40 },
	{ XFI_RG0A, 0x00000180, 0x00000180 },
	{ XFI_RG0B, 0x00004003, 0x00004003 },
	{ XFI_RG0C, 0x0000017F, 0x0000017F },
	{ XFI_RG0D, 0x00003044, 0x00003044 },
	{ XFI_RG0E, 0x000087A6, 0x000087A6 },
	{ XFI_RG0F, 0x000003CD, 0x000003CD },
	{ XFI_RG10, 0x000002FA, 0x000002FA },
	{ XFI_RG11, 0x0000BC00, 0x0000BC00 },
	{ XFI_RG12, 0x00007C48, 0x00007C48 },
	{ XFI_RG13, 0x000000FF, 0x000000FF },
	{ XFI_RG14, 0x00000000, 0x00000000 },
	{ XFI_RG15, 0x00000444, 0x00000444 },
	{ XFI_RG16, 0x00004444, 0x00004444 },
	{ XFI_RG17, 0x0000082F, 0x0000082F },
	{ XFI_RG18, 0x0000082F, 0x0000082F },
	{ XFI_RG19, 0x0000082F, 0x0000082F },
	{ XFI_RG1A, 0x00000B80, 0x00000B80 },
	{ XFI_RG1B, 0x00000000, 0x00000000 },
	{ XFI_RG1C, 0x00000000, 0x00000000 },
	{ XFI_RG1D, 0x00000430, 0x00000430 },
	{ XFI_RG1E, 0x00000401, 0x00000401 },
	{ XFI_RG1F, 0x00000000, 0x00000000 },
	{ XFI_RG20, 0x0000001F, 0x0000001F },
	{ XFI_RG21, 0x0000FF24, 0x0000FF24 },
	{ XFI_RG22, 0x0000F960, 0x0000F960 },
	{ XFI_RG23, 0x00000000, 0x00000000 },
	{ XFI_RG24, 0x00001F93, 0x00001F93 },
	{ XFI_RG25, 0x0000F400, 0x0000F400 },
	{ XFI_RG26, 0x000003FF, 0x000003FF },
	{ XFI_RG27, 0x000027D0, 0x000027D0 },
	{ XFI_RG28, 0x0000000F, 0x0000000F },
	{ XFI_RG29, 0x0000FC9F, 0x0000FC9F },
	{ XFI_RG2A, 0x00004000, 0x00004000 },
	{ XFI_RG2B, 0x00003FF2, 0x00003FF2 },
	{ XFI_RG2C, 0x00007D8E, 0x00007D8E },
	{ XFI_RG2D, 0x00004010, 0x00004010 },
	{ XFI_RG2E, 0x0000C5EC, 0x0000C5EC },
	{ XFI_RG2F, 0x0000FE00, 0x0000FE00 },
	{ XFI_RG30, 0x00008924, 0x00008924 },
	{ XFI_RG31, 0x0000C190, 0x0000C190 },
	{ XFI_RG32, 0x0000FFFA, 0x0000FFFA },
	{ XFI_RG33, 0x00001F08, 0x00001F08 },
	{ XFI_RG34, 0x000028B8, 0x000028B8 },
	{ XFI_RG35, 0x00000001, 0x00000001 },
	{ XFI_RG36, 0x00000002, 0x00000002 },
	{ XFI_RG37, 0x00001299, 0x00001299 },
	{ XFI_RG38, 0x00004210, 0x00004210 },
	{ XFI_RG39, 0x00004210, 0x00004210 },
	{ XFI_RG3A, 0x0000000D, 0x0000000D },
	{ XFI_RG40, 0x0C001000, 0x0C001000 },
	{ XFI_RG41, 0x09C00001, 0x09C00001 },
	{ XFI_RG42, 0x0000003F, 0x0000003F },
	{ XFI_RG43, 0x00000405, 0x00000405 },
	{ XFI_RG44, 0x00007E00, 0x00007E00 },
	{ XFI_RG45, 0x00000000, 0x00000000 },
	{ XFI_RG46, 0x003F0908, 0x003F0908 },
	{ XFI_RG47, 0x38AA00B2, 0x38AA00B2 },
	{ XFI_RG48, 0x00000003, 0x00000003 },
	{ XFI_RG49, 0x0000FFFF, 0x0000FFFF },
	{ XFI_RG4A, 0x000000C8, 0x000000C8 },
	{ XFI_RG4B, 0x00000000, 0x00000000 },
	{ XFI_RG4C, 0x00005007, 0x00005007 },
	{ XFI_RG4D, 0x00000005, 0x00000005 }
};
//
// release version - 20180214
//
ca_uint32_t xfi_xg_init_tbl_g3_revd[__G3_XFI_CFG_REG_NUM][3] = { /* XG config for reversion D */
	{ XFI_RG00, 0x000060F0, 0x000060F0 },
	{ XFI_RG01, 0x00004806, 0x00004806 },
	{ XFI_RG02, 0x00001447, 0x00001447 },
	{ XFI_RG03, 0x0000C71D, 0x0000C71D },
	{ XFI_RG04, 0x0000AAA8, 0x0000AAA8 },
	{ XFI_RG05, 0x00005523, 0x00005523 },
	{ XFI_RG06, 0x00006003, 0x00006003 },
	{ XFI_RG07, 0x0000FFFF, 0x0000FFFF },
	{ XFI_RG08, 0x0000326A, 0x0000326A },
	{ XFI_RG09, 0x00008A40, 0x00008A40 },
	{ XFI_RG0A, 0x00000180, 0x00000180 },
	{ XFI_RG0B, 0x00004003, 0x00004003 },
	{ XFI_RG0C, 0x0000017F, 0x0000017F },
	{ XFI_RG0D, 0x00003044, 0x00003044 },
	{ XFI_RG0E, 0x000087A6, 0x000087A6 },
	{ XFI_RG0F, 0x000003CD, 0x000003CD },
	{ XFI_RG10, 0x000002FA, 0x000002FA },
	{ XFI_RG11, 0x0000BC00, 0x0000BC00 },
	{ XFI_RG12, 0x00007C48, 0x00007C48 },
	{ XFI_RG13, 0x000000FF, 0x000000FF },
	{ XFI_RG14, 0x00000000, 0x00000000 },
	{ XFI_RG15, 0x00000444, 0x00000444 },
	{ XFI_RG16, 0x00004444, 0x00004444 },
	{ XFI_RG17, 0x0000082F, 0x0000082F },
	{ XFI_RG18, 0x0000082F, 0x0000082F },
	{ XFI_RG19, 0x0000082F, 0x0000082F },
	{ XFI_RG1A, 0x00000B80, 0x00000B80 },
	{ XFI_RG1B, 0x00000000, 0x00000000 },
	{ XFI_RG1C, 0x00000000, 0x00000000 },
	{ XFI_RG1D, 0x00000430, 0x00000430 },
	{ XFI_RG1E, 0x00000401, 0x00000401 },
	{ XFI_RG1F, 0x00000000, 0x00000000 },
	{ XFI_RG20, 0x0000001F, 0x0000001F },
	{ XFI_RG21, 0x0000FF24, 0x0000FF24 },
	{ XFI_RG22, 0x0000F960, 0x0000F960 },
	{ XFI_RG23, 0x00000000, 0x00000000 },
	{ XFI_RG24, 0x00001F93, 0x00001F93 },
	{ XFI_RG25, 0x0000F400, 0x0000F400 },
	{ XFI_RG26, 0x000003FF, 0x000003FF },
	{ XFI_RG27, 0x000027D0, 0x000027D0 },
	{ XFI_RG28, 0x0000000F, 0x0000000F },
	{ XFI_RG29, 0x0000FC9F, 0x0000FC9F },
	{ XFI_RG2A, 0x00004000, 0x00004000 },
	{ XFI_RG2B, 0x00003FF2, 0x00003FF2 },
	{ XFI_RG2C, 0x00007D8E, 0x00007D8E },
	{ XFI_RG2D, 0x00004010, 0x00004010 },
	{ XFI_RG2E, 0x0000C5EC, 0x0000C5EC },
	{ XFI_RG2F, 0x0000FE00, 0x0000FE00 },
	{ XFI_RG30, 0x00008924, 0x00008924 },
	{ XFI_RG31, 0x0000C190, 0x0000C190 },
	{ XFI_RG32, 0x0000FFF2, 0x0000FFF2 },
	{ XFI_RG33, 0x00001F08, 0x00001F08 },
	{ XFI_RG34, 0x000028B8, 0x000028B8 },
	{ XFI_RG35, 0x00000001, 0x00000001 },
	{ XFI_RG36, 0x00000002, 0x00000002 },
	{ XFI_RG37, 0x00001299, 0x00001299 },
	{ XFI_RG38, 0x00004210, 0x00004210 },
	{ XFI_RG39, 0x00004210, 0x00004210 },
	{ XFI_RG3A, 0x0000000D, 0x0000000D },
	{ XFI_RG40, 0x0C001000, 0x0C001000 },
	{ XFI_RG41, 0x09C00001, 0x09C00001 },
	{ XFI_RG42, 0x0000003F, 0x0000003F },
	{ XFI_RG43, 0x00000405, 0x00000405 },
	{ XFI_RG44, 0x00007E00, 0x00007E00 },
	{ XFI_RG45, 0x00000000, 0x00000000 },
	{ XFI_RG46, 0x003F0908, 0x003F0908 },
	{ XFI_RG47, 0xB8AA008E, 0xB8AA008E },
	{ XFI_RG48, 0x0003FC03, 0x0003FC03 },
	{ XFI_RG49, 0x0000FFFF, 0x0000FFFF },
	{ XFI_RG4A, 0x000000C8, 0x000000C8 },
	{ XFI_RG4B, 0x00000000, 0x00000000 },
	{ XFI_RG4C, 0x00005000, 0x00005000 },
	{ XFI_RG4D, 0x00000006, 0x00000006 }
};

/** __xfi_cfg_init_xg_g3
 *@brief: initialize the G3 SoC's XFI with XG parameters
 *@note: it will provide different parameters for different G3 version,
 *       now support reversion C and reversion D
 */
static void
__xfi_cfg_init_xg_g3(
	ca_device_id_t                     device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx = 0;
	ca_uint32_t(*prof)[3] = NULL; /* XFI init profile pointer */

	if(device_id != CA_DEV_LAN) {
		return;
	}

	prof = (g_xfi_drv.chip_version >= __SOC_REVISION_D) ? xfi_xg_init_tbl_g3_revd : xfi_xg_init_tbl_g3_revc;

	for(idx = 0; idx < __G3_XFI_CFG_REG_NUM; idx++){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
		if(XFI_RG32 == prof[idx][0]) {
			CA_NE_XFI_REG_WRITE(g_xfi_rg32_param[2 + pcs_index],prof[idx][0] + pcs_index * XFI_STRIDE);
		} else {
			CA_NE_XFI_REG_WRITE(prof[idx][1 + pcs_index],prof[idx][0] + pcs_index * XFI_STRIDE);
		}
#else
		CA_NE_XFI_REG_WRITE(prof[idx][1 + pcs_index],prof[idx][0] + pcs_index * XFI_STRIDE);
#endif
		if(XFI_RG46 == prof[idx][0])
			ca_mdelay(30);
		else
			ca_udelay(100);
	}
#if 0
	if (g_xfi_drv.chip_version >= __SOC_REVISION_D) {
		for (idx = 0;
		     idx < __G3_XFI_CFG_REG_NUM;
		     idx++) {
			//ca_printf("CSR set %#x = %#x\n", xfi_xg_init_tbl_g3_revd[idx][0] + pcs_index * XFI_STRIDE, xfi_xg_init_tbl_g3_revd[idx][1+pcs_index]);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
			if (XFI_RG32 == xfi_xg_init_tbl_g3_revd[idx][0]) {
				CA_NE_XFI_REG_WRITE(g_xfi_rg32_param[2+pcs_index], xfi_xg_init_tbl_g3_revd[idx][0] + pcs_index * XFI_STRIDE);
			} else {
				CA_NE_XFI_REG_WRITE(xfi_xg_init_tbl_g3_revd[idx][1+pcs_index], xfi_xg_init_tbl_g3_revd[idx][0] + pcs_index * XFI_STRIDE);
			}
#else
			CA_NE_XFI_REG_WRITE(xfi_xg_init_tbl_g3_revd[idx][1+pcs_index], xfi_xg_init_tbl_g3_revd[idx][0] + pcs_index * XFI_STRIDE);
#endif
			if (XFI_RG46 == xfi_xg_init_tbl_g3_revd[idx][0])
			ca_mdelay(30);
			else
			ca_udelay(100);
		}
	} else {
		for (idx = 0;
		     idx < __G3_XFI_CFG_REG_NUM;
		     idx++) {
			//ca_printf("CSR set %#x = %#x\n", xfi_xg_init_tbl_g3_revc[idx][0] + pcs_index * XFI_STRIDE, xfi_xg_init_tbl_g3_revc[idx][1+pcs_index]);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
			if (XFI_RG32 == xfi_xg_init_tbl_g3_revc[idx][0]) {
				CA_NE_XFI_REG_WRITE(g_xfi_rg32_param[2+pcs_index], xfi_xg_init_tbl_g3_revc[idx][0] + pcs_index * XFI_STRIDE);
			} else {
				CA_NE_XFI_REG_WRITE(xfi_xg_init_tbl_g3_revc[idx][1+pcs_index], xfi_xg_init_tbl_g3_revc[idx][0] + pcs_index * XFI_STRIDE);
			}
#else
			CA_NE_XFI_REG_WRITE(xfi_xg_init_tbl_g3_revc[idx][1+pcs_index], xfi_xg_init_tbl_g3_revc[idx][0] + pcs_index * XFI_STRIDE);
#endif
			if (XFI_RG46 == xfi_xg_init_tbl_g3_revc[idx][0])
			ca_mdelay(30);
			else
			ca_udelay(100);
		}
	}
#endif
	ca_mdelay(10);

}
#if !defined(CONFIG_ARCH_CORTINA_G3)

/** __xfi_ctrl_flow_xg_g3
 *@brief: bringup G3 SoC's XFI with sequences
 *@note: the delay between each sequnces may be enlarge, but at least 1000us
 */
static void
__xfi_ctrl_flow_xg_g3(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{

	CA_DEV_NI_REG_WRITE(device_id,0x00001d00,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + pcs_index * SXGMII_STRIDE);
	ca_udelay(100);
	CA_DEV_NI_REG_WRITE(device_id,0x00003d00,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + pcs_index * SXGMII_STRIDE);
	ca_udelay(100);
	CA_DEV_NI_REG_WRITE(device_id,0x00003f00,NI_HV_SXGMII_SXGRA_PCSSDSMISCCTL + pcs_index * SXGMII_STRIDE);
	ca_udelay(1000);

	CA_DEV_XFI_REG_WRITE(device_id,0x00270908,XFI_RG46 + pcs_index * XFI_STRIDE);
	ca_udelay(1000);
	CA_DEV_XFI_REG_WRITE(device_id,0x00370908,XFI_RG46 + pcs_index * XFI_STRIDE);
	ca_udelay(1000);
	CA_DEV_XFI_REG_WRITE(device_id,0x003f0908,XFI_RG46 + pcs_index * XFI_STRIDE);
	ca_udelay(1000);

}
#endif
#endif


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg[__XFI_CFG_REG_NUM][3]         = {
	{ XFI_RG00,   0x000061F0, 0x000061F0 },
		{ XFI_RG01,   0x00004804, 0x00004804 },
		{ XFI_RG02,   0x0000540F, 0x0000340F },
		{ XFI_RG03,   0x0000C13D, 0x0000C13D },
		{ XFI_RG04,   0x00008006, 0x00008006 },
		{ XFI_RG05,   0x0000A093, 0x0000A093 },
		{ XFI_RG06,   0x00004C4C, 0x00004C4C },
		{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
		{ XFI_RG08,   0x0000202A, 0x0000202A },
		{ XFI_RG09,   0x00008540, 0x00008540 },
		{ XFI_RG0A,   0x00000000, 0x00000000 }, //0x00008180
		{ XFI_RG0B,   0x00004002, 0x00004002 },
		{ XFI_RG0C,   0x000000FF, 0x000000FF },
		{ XFI_RG0D,   0x00000046, 0x00000046 },
		{ XFI_RG0E,   0x00008786, 0x00008786 },
		{ XFI_RG0F,   0x000003ED, 0x000003ED },
		{ XFI_RG10,   0x0000001C, 0x0000001C },
		{ XFI_RG11,   0x00000400, 0x00000400 },
		{ XFI_RG12,   0x00007D10, 0x00007D10 },
		{ XFI_RG13,   0x000000FF, 0x000000FF },
		{ XFI_RG14,   0x00000000, 0x00000000 },
		{ XFI_RG15,   0x00000111, 0x00000111 },
		{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
		{ XFI_RG17,   0x0000820F, 0x0000820F },
		{ XFI_RG18,   0x0000820F, 0x0000820F },
		{ XFI_RG19,   0x0000820F, 0x0000820F },
		{ XFI_RG1A,   0x000083D0, 0x000083D0 },
		{ XFI_RG1B,   0x000000C0, 0x000000C0 },
		{ XFI_RG1C,   0x00000062, 0x00000062 },
		{ XFI_RG1D,   0x00000430, 0x00000430 },
		{ XFI_RG1E,   0x00008000, 0x00008000 },
		{ XFI_RG1F,   0x00000000, 0x00000000 },
		{ XFI_RG20,   0x0000001F, 0x0000001F },
		{ XFI_RG21,   0x00001F04, 0x00001F04 },
		{ XFI_RG22,   0x0000F960, 0x0000F960 },
		{ XFI_RG23,   0x0000000F, 0x0000000F },
		{ XFI_RG24,   0x00001F93, 0x00001F93 },
		{ XFI_RG25,   0x0000F000, 0x0000F000 },
		{ XFI_RG26,   0x000003FF, 0x000003FF },
		{ XFI_RG27,   0x000027C0, 0x000027C0 },
		{ XFI_RG28,   0x0000000F, 0x0000000F },
		{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
		{ XFI_RG2A,   0x00000000, 0x00000000 },
		{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
		{ XFI_RG2C,   0x00007D80, 0x00007D80 },
		{ XFI_RG2D,   0x0000401E, 0x0000401E },
		{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
		{ XFI_RG2F,   0x00000000, 0x00000000 },
		{ XFI_RG30,   0x0000C924, 0x0000C924 },
		{ XFI_RG31,   0x0000C190, 0x0000C190 },
		{ XFI_RG32,   0x0000827A, 0x0000827A },
		{ XFI_RG33,   0x0000A36A, 0x0000A36A },
		{ XFI_RG34,   0x000024BA, 0x000024BA },
		{ XFI_RG35,   0x00000001, 0x00000001 },
		{ XFI_RG36,   0x00000002, 0x00000002 },
		{ XFI_RG37,   0x00003299, 0x00003299 },
		{ XFI_RG38,   0x0000A510, 0x0000A510 },
		{ XFI_RG39,   0x0000A510, 0x0000A510 },
		{ XFI_RG3A,   0x0000000D, 0x0000000D },
		{ XFI_RG40,   0x0C001180, 0x0C001180 },
		{ XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
		{ XFI_RG42,   0x00000000, 0x00000000 },
		{ XFI_RG43,   0x00000005, 0x00000005 },
		{ XFI_RG44,   0x000057DF, 0x000057DF },
		{ XFI_RG45,   0x00001000, 0x00001000 },
		{ XFI_RG46,   0x003788a7, 0x003f08a7 }, // 0x003F08A7, 0x003F08A7
		{ XFI_RG47,   0xBCA000DE, 0xBCA000DE },
		{ XFI_RGNEW0, 0x00000000, 0x00000000 },
		{ XFI_RGNEW1, 0x00000001, 0x00000001 },
		{ XFI_RGNEW2, 0x00000035, 0x00000035 },
		{ XFI_RGNEW3, 0x0AC5402C, 0x0AC5402C },
		{ XFI_RGNEW4, 0x02380000, 0x02380000 },
		{ XFI_RGNEW5, 0x03030D00, 0x03030D00 },
		{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - 20190125
//
ca_uint32_t g_xfi_reg_init_tbl_1g[__XFI_CFG_REG_NUM][3]         = {
	{ XFI_RG00,   0x00000190, 0x00000190 },
		{ XFI_RG01,   0x00004800, 0x00004800 },
		{ XFI_RG02,   0x0000040F, 0x0000040F },
		{ XFI_RG03,   0x0000C13D, 0x0000C13D },
		{ XFI_RG04,   0x00008006, 0x00008006 },
		{ XFI_RG05,   0x0000A033, 0x0000A033 },
		{ XFI_RG06,   0x00004C4C, 0x00004C4C },
		{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
		{ XFI_RG08,   0x0000202A, 0x0000202A },
		{ XFI_RG09,   0x00008540, 0x00008540 },
		{ XFI_RG0A,   0x00008180, 0x00008180 },
		{ XFI_RG0B,   0x00004002, 0x00004002 },
		{ XFI_RG0C,   0x000001FF, 0x000001FF },
		{ XFI_RG0D,   0x0000007C, 0x0000007C },
		{ XFI_RG0E,   0x00008786, 0x00008786 },
		{ XFI_RG0F,   0x000003ED, 0x000003ED },
		{ XFI_RG10,   0x0000001C, 0x0000001C },
		{ XFI_RG11,   0x00000400, 0x00000400 },
		{ XFI_RG12,   0x00007D10, 0x00007D10 },
		{ XFI_RG13,   0x000000FF, 0x000000FF },
		{ XFI_RG14,   0x00000000, 0x00000000 },
		{ XFI_RG15,   0x00000111, 0x00000111 },
		{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
		{ XFI_RG17,   0x0000820F, 0x0000820F },
		{ XFI_RG18,   0x0000820F, 0x0000820F },
		{ XFI_RG19,   0x0000820F, 0x0000820F },
		{ XFI_RG1A,   0x000083D0, 0x000083D0 },
		{ XFI_RG1B,   0x000000C0, 0x000000C0 },
		{ XFI_RG1C,   0x00000062, 0x00000062 },
		{ XFI_RG1D,   0x00000430, 0x00000430 },
		{ XFI_RG1E,   0x00008000, 0x00008000 },
		{ XFI_RG1F,   0x00000000, 0x00000000 },
		{ XFI_RG20,   0x00003F1F, 0x00003F1F },
		{ XFI_RG21,   0x00000024, 0x00000024 },
		{ XFI_RG22,   0x0000F960, 0x0000F960 },
		{ XFI_RG23,   0x0000000F, 0x0000000F },
		{ XFI_RG24,   0x00001F93, 0x00001F93 },
		{ XFI_RG25,   0x0000F000, 0x0000F000 },
		{ XFI_RG26,   0x000003FF, 0x000003FF },
		{ XFI_RG27,   0x000027C0, 0x000027C0 },
		{ XFI_RG28,   0x0000000F, 0x0000000F },
		{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
		{ XFI_RG2A,   0x00000000, 0x00000000 },
		{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
		{ XFI_RG2C,   0x00007D80, 0x00007D80 },
		{ XFI_RG2D,   0x00004010, 0x00004010 },
		{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
		{ XFI_RG2F,   0x00000000, 0x00000000 },
		{ XFI_RG30,   0x0000C924, 0x0000C924 },
		{ XFI_RG31,   0x0000C190, 0x0000C190 },
		{ XFI_RG32,   0x0000FFFA, 0x0000FFFA },
		{ XFI_RG33,   0x0000A36A, 0x0000A36A },
		{ XFI_RG34,   0x000024BA, 0x000024BA },
		{ XFI_RG35,   0x00000001, 0x00000001 },
		{ XFI_RG36,   0x00000002, 0x00000002 },
		{ XFI_RG37,   0x00003299, 0x00003299 },
		{ XFI_RG38,   0x0000A510, 0x0000A510 },
		{ XFI_RG39,   0x0000A510, 0x0000A510 },
		{ XFI_RG3A,   0x0000000D, 0x0000000D },
		{ XFI_RG40,   0x0C001180, 0x0C001180 },
		{ XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
		{ XFI_RG42,   0x00000000, 0x00000000 },
		{ XFI_RG43,   0x00000005, 0x00000005 },
		{ XFI_RG44,   0x00005F00, 0x00005F00 },
		{ XFI_RG45,   0x00001200, 0x00001200 },
		{ XFI_RG46,   0x003F08A7, 0x003F08A7 },
		{ XFI_RG47,   0x9CA001DE, 0x9CA001DE },
		{ XFI_RGNEW0, 0x00000000, 0x00000000 },
		{ XFI_RGNEW1, 0x00000000, 0x00000000 },
		{ XFI_RGNEW2, 0x00000035, 0x00000035 },
		{ XFI_RGNEW3, 0x0AC5403C, 0x0AC5403C },
		{ XFI_RGNEW4, 0x03F80000, 0x03F80000 },
		{ XFI_RGNEW5, 0x03010D00, 0x03010D00 },
		{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};

	#else
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg[__XFI_CFG_REG_NUM][3]         = {
	{ XFI_RG00,   0x000061F0, 0x000061F0 },
		{ XFI_RG01,   0x00004804, 0x00004804 },
		{ XFI_RG02,   0x0000540F, 0x0000340F },
		{ XFI_RG03,   0x0000C13D, 0x0000C13D },
		{ XFI_RG04,   0x00008006, 0x00008006 },
		{ XFI_RG05,   0x0000A093, 0x0000A093 },
		{ XFI_RG06,   0x00004C4C, 0x00004C4C },
		{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
		{ XFI_RG08,   0x0000202A, 0x0000202A },
		{ XFI_RG09,   0x00008540, 0x00008540 },
		{ XFI_RG0A,   0x00008180, 0x00008180 },
		{ XFI_RG0B,   0x00004002, 0x00004002 },
		{ XFI_RG0C,   0x000000FF, 0x000000FF },
		{ XFI_RG0D,   0x00000046, 0x00000046 },
		{ XFI_RG0E,   0x00008786, 0x00008786 },
		{ XFI_RG0F,   0x000003ED, 0x000003ED },
		{ XFI_RG10,   0x0000001C, 0x0000001C },
		{ XFI_RG11,   0x00000400, 0x00000400 },
		{ XFI_RG12,   0x00007D10, 0x00007D10 },
		{ XFI_RG13,   0x000000FF, 0x000000FF },
		{ XFI_RG14,   0x00000000, 0x00000000 },
		{ XFI_RG15,   0x00000111, 0x00000111 },
		{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
		{ XFI_RG17,   0x0000820F, 0x0000820F },
		{ XFI_RG18,   0x0000820F, 0x0000820F },
		{ XFI_RG19,   0x0000820F, 0x0000820F },
		{ XFI_RG1A,   0x000083D0, 0x000083D0 },
		{ XFI_RG1B,   0x000000C0, 0x000000C0 },
		{ XFI_RG1C,   0x00000062, 0x00000062 },
		{ XFI_RG1D,   0x00000430, 0x00000430 },
		{ XFI_RG1E,   0x00008000, 0x00008000 },
		{ XFI_RG1F,   0x00000000, 0x00000000 },
		{ XFI_RG20,   0x0000001F, 0x0000001F },
		{ XFI_RG21,   0x00001F04, 0x00001F04 },
		{ XFI_RG22,   0x0000F960, 0x0000F960 },
		{ XFI_RG23,   0x0000000F, 0x0000000F },
		{ XFI_RG24,   0x00001F93, 0x00001F93 },
		{ XFI_RG25,   0x0000F000, 0x0000F000 },
		{ XFI_RG26,   0x000003FF, 0x000003FF },
		{ XFI_RG27,   0x000027C0, 0x000027C0 },
		{ XFI_RG28,   0x0000000F, 0x0000000F },
		{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
		{ XFI_RG2A,   0x00000000, 0x00000000 },
		{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
		{ XFI_RG2C,   0x00007D80, 0x00007D80 },
		{ XFI_RG2D,   0x0000401E, 0x0000401E },
		{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
		{ XFI_RG2F,   0x00000000, 0x00000000 },
		{ XFI_RG30,   0x0000C924, 0x0000C924 },
		{ XFI_RG31,   0x0000C190, 0x0000C190 },
		{ XFI_RG32,   0x0000827A, 0x0000827A },
		{ XFI_RG33,   0x0000A36A, 0x0000A36A },
		{ XFI_RG34,   0x000024BA, 0x000024BA },
		{ XFI_RG35,   0x00000001, 0x00000001 },
		{ XFI_RG36,   0x00000002, 0x00000002 },
		{ XFI_RG37,   0x00003299, 0x00003299 },
		{ XFI_RG38,   0x0000A510, 0x0000A510 },
		{ XFI_RG39,   0x0000A510, 0x0000A510 },
		{ XFI_RG3A,   0x0000000D, 0x0000000D },
		{ XFI_RG40,   0x0C001180, 0x0C001180 },
		{ XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
		{ XFI_RG42,   0x00000000, 0x00000000 },
		{ XFI_RG43,   0x00000005, 0x00000005 },
		{ XFI_RG44,   0x000057DF, 0x000057DF },
		{ XFI_RG45,   0x00001000, 0x00001000 },
		{ XFI_RG46,   0x003708A7, 0x003708A7 }, // 0x003F08A7
		{ XFI_RG47,   0xBCA000DE, 0xBCA000DE },
		{ XFI_RGNEW0, 0x00000000, 0x00000000 },
		{ XFI_RGNEW1, 0x00000001, 0x00000001 },
		{ XFI_RGNEW2, 0x00000035, 0x00000035 },
		{ XFI_RGNEW3, 0x0AC5402C, 0x0AC5402C },
		{ XFI_RGNEW4, 0x02380000, 0x02380000 },
		{ XFI_RGNEW5, 0x03030D00, 0x03030D00 },
		{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// version - 20190125
//
ca_uint32_t g_xfi_reg_init_tbl_1g[__XFI_CFG_REG_NUM][3]         = {
	{ XFI_RG00,   0x00000190, 0x00000190 },
		{ XFI_RG01,   0x00004800, 0x00004800 },
		{ XFI_RG02,   0x0000040F, 0x0000040F },
		{ XFI_RG03,   0x0000C13D, 0x0000C13D },
		{ XFI_RG04,   0x00008006, 0x00008006 },
		{ XFI_RG05,   0x0000A033, 0x0000A033 },
		{ XFI_RG06,   0x00004C4C, 0x00004C4C },
		{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
		{ XFI_RG08,   0x0000202A, 0x0000202A },
		{ XFI_RG09,   0x00008540, 0x00008540 },
		{ XFI_RG0A,   0x00008180, 0x00008180 },
		{ XFI_RG0B,   0x00004002, 0x00004002 },
		{ XFI_RG0C,   0x000001FF, 0x000001FF },
		{ XFI_RG0D,   0x0000007C, 0x0000007C },
		{ XFI_RG0E,   0x00008786, 0x00008786 },
		{ XFI_RG0F,   0x000003ED, 0x000003ED },
		{ XFI_RG10,   0x0000001C, 0x0000001C },
		{ XFI_RG11,   0x00000400, 0x00000400 },
		{ XFI_RG12,   0x00007D10, 0x00007D10 },
		{ XFI_RG13,   0x000000FF, 0x000000FF },
		{ XFI_RG14,   0x00000000, 0x00000000 },
		{ XFI_RG15,   0x00000111, 0x00000111 },
		{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
		{ XFI_RG17,   0x0000820F, 0x0000820F },
		{ XFI_RG18,   0x0000820F, 0x0000820F },
		{ XFI_RG19,   0x0000820F, 0x0000820F },
		{ XFI_RG1A,   0x000083D0, 0x000083D0 },
		{ XFI_RG1B,   0x000000C0, 0x000000C0 },
		{ XFI_RG1C,   0x00000062, 0x00000062 },
		{ XFI_RG1D,   0x00000430, 0x00000430 },
		{ XFI_RG1E,   0x00008000, 0x00008000 },
		{ XFI_RG1F,   0x00000000, 0x00000000 },
		{ XFI_RG20,   0x00003F1F, 0x00003F1F },
		{ XFI_RG21,   0x00000024, 0x00000024 },
		{ XFI_RG22,   0x0000F960, 0x0000F960 },
		{ XFI_RG23,   0x0000000F, 0x0000000F },
		{ XFI_RG24,   0x00001F93, 0x00001F93 },
		{ XFI_RG25,   0x0000F000, 0x0000F000 },
		{ XFI_RG26,   0x000003FF, 0x000003FF },
		{ XFI_RG27,   0x000027C0, 0x000027C0 },
		{ XFI_RG28,   0x0000000F, 0x0000000F },
		{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
		{ XFI_RG2A,   0x00000000, 0x00000000 },
		{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
		{ XFI_RG2C,   0x00007D80, 0x00007D80 },
		{ XFI_RG2D,   0x00004010, 0x00004010 },
		{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
		{ XFI_RG2F,   0x00000000, 0x00000000 },
		{ XFI_RG30,   0x0000C924, 0x0000C924 },
		{ XFI_RG31,   0x0000C190, 0x0000C190 },
		{ XFI_RG32,   0x0000FFFA, 0x0000FFFA },
		{ XFI_RG33,   0x0000A36A, 0x0000A36A },
		{ XFI_RG34,   0x000024BA, 0x000024BA },
		{ XFI_RG35,   0x00000001, 0x00000001 },
		{ XFI_RG36,   0x00000002, 0x00000002 },
		{ XFI_RG37,   0x00003299, 0x00003299 },
		{ XFI_RG38,   0x0000A510, 0x0000A510 },
		{ XFI_RG39,   0x0000A510, 0x0000A510 },
		{ XFI_RG3A,   0x0000000D, 0x0000000D },
		{ XFI_RG40,   0x0C001180, 0x0C001180 },
		{ XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
		{ XFI_RG42,   0x00000000, 0x00000000 },
		{ XFI_RG43,   0x00000005, 0x00000005 },
		{ XFI_RG44,   0x00005F00, 0x00005F00 },
		{ XFI_RG45,   0x00001200, 0x00001200 },
		{ XFI_RG46,   0x003F08A7, 0x003F08A7 },
		{ XFI_RG47,   0x9CA001DE, 0x9CA001DE },
		{ XFI_RGNEW0, 0x00000000, 0x00000000 },
		{ XFI_RGNEW1, 0x00000000, 0x00000000 },
		{ XFI_RGNEW2, 0x00000035, 0x00000035 },
		{ XFI_RGNEW3, 0x0AC5403C, 0x0AC5403C },
		{ XFI_RGNEW4, 0x03F80000, 0x03F80000 },
		{ XFI_RGNEW5, 0x03010D00, 0x03010D00 },
		{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};

	#endif
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg_0off1on[__XFI_CFG_REG_NUM][3] = {
	{ XFI_RG00,   0x00006190, 0x000061F0 },
	{ XFI_RG01,   0x00004800, 0x00004804 },
	{ XFI_RG02,   0x0000044F, 0x0000340F },
	{ XFI_RG03,   0x0000C12D, 0x0000C13D },
	{ XFI_RG04,   0x00008006, 0x00008006 },
	{ XFI_RG05,   0x0000A090, 0x0000A093 },
	{ XFI_RG06,   0x00004C4C, 0x00004C4C },
	{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	{ XFI_RG08,   0x0000202A, 0x0000202A },
	{ XFI_RG09,   0x00008640, 0x00008540 },
	{ XFI_RG0A,   0x00008000, 0x00008180 },
	{ XFI_RG0B,   0x00004002, 0x00004002 },
	{ XFI_RG0C,   0x000001FF, 0x000000FF },
	{ XFI_RG0D,   0x00000046, 0x00000046 },
	{ XFI_RG0E,   0x00008786, 0x00008786 },
	{ XFI_RG0F,   0x000003ED, 0x000003ED },
	{ XFI_RG10,   0x0000001C, 0x0000001C },
	{ XFI_RG11,   0x00000400, 0x00000400 },
	{ XFI_RG12,   0x00007D10, 0x00007D10 },
	{ XFI_RG13,   0x000000FF, 0x000000FF },
	{ XFI_RG14,   0x00000000, 0x00000000 },
	{ XFI_RG15,   0x00000111, 0x00000111 },
	{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	{ XFI_RG17,   0x0000000F, 0x0000820F },
	{ XFI_RG18,   0x000028AF, 0x0000820F },
	{ XFI_RG19,   0x000028AF, 0x0000820F },
	{ XFI_RG1A,   0x00002BD0, 0x000083D0 },
	{ XFI_RG1B,   0x000000C0, 0x000000C0 },
	{ XFI_RG1C,   0x00000062, 0x00000062 },
	{ XFI_RG1D,   0x00000430, 0x00000430 },
	{ XFI_RG1E,   0x00000001, 0x00008000 },
	{ XFI_RG1F,   0x00000000, 0x00000000 },
	{ XFI_RG20,   0x0000001F, 0x0000001F },
	{ XFI_RG21,   0x00001F04, 0x00001F04 },
	{ XFI_RG22,   0x0000F900, 0x0000F960 },
	{ XFI_RG23,   0x0000000F, 0x0000000F },
	{ XFI_RG24,   0x00001F93, 0x00001F93 },
	{ XFI_RG25,   0x0000F000, 0x0000F000 },
	{ XFI_RG26,   0x000003FF, 0x000003FF },
	{ XFI_RG27,   0x000027C0, 0x000027C0 },
	{ XFI_RG28,   0x0000000F, 0x0000000F },
	{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	{ XFI_RG2A,   0x00000000, 0x00000000 },
	{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	{ XFI_RG2C,   0x00007C00, 0x00007D80 },
	{ XFI_RG2D,   0x0000401E, 0x0000401E },
	{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	{ XFI_RG2F,   0x00000000, 0x00000000 },
	{ XFI_RG30,   0x0000C924, 0x0000C924 },
	{ XFI_RG31,   0x0000C190, 0x0000C190 },
	{ XFI_RG32,   0x0000827A, 0x0000827A },
	{ XFI_RG33,   0x0000A36A, 0x0000A36A },
	{ XFI_RG34,   0x000024BA, 0x000024BA },
	{ XFI_RG35,   0x00000001, 0x00000001 },
	{ XFI_RG36,   0x00000002, 0x00000002 },
	{ XFI_RG37,   0x00003299, 0x00003299 },
	{ XFI_RG38,   0x0000A510, 0x0000A510 },
	{ XFI_RG39,   0x0000A510, 0x0000A510 },
	{ XFI_RG3A,   0x0000000D, 0x0000000D },
	{ XFI_RG40,   0x0C001180, 0x0C001180 },
	{ XFI_RG41,   0x09C00001, 0x09FFFF81 },
	{ XFI_RG42,   0x00000000, 0x00000000 },
	{ XFI_RG43,   0x00000005, 0x00000005 },
	{ XFI_RG44,   0x000057DF, 0x000057DF },
	{ XFI_RG45,   0x00001000, 0x00001000 },
	{ XFI_RG46,   0x003188A7, 0x003F08A7 },
	{ XFI_RG47,   0xBCA000DE, 0xBCA000DE },
	{ XFI_RGNEW0, 0x00000000, 0x00000000 },
	{ XFI_RGNEW1, 0x00000000, 0x00000001 },
	{ XFI_RGNEW2, 0x00000035, 0x00000035 },
	{ XFI_RGNEW3, 0x0AC5402C, 0x0AC5402C },
	{ XFI_RGNEW4, 0x00380000, 0x02380000 },
	{ XFI_RGNEW5, 0x03030D00, 0x03030D00 },
	{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg_1off0on[__XFI_CFG_REG_NUM][3] = {
	{ XFI_RG00,   0x000061F0, 0x00006190 },
	{ XFI_RG01,   0x00004804, 0x00004800 },
	{ XFI_RG02,   0x0000540F, 0x0000044F },
	{ XFI_RG03,   0x0000C13D, 0x0000C12D },
	{ XFI_RG04,   0x00008006, 0x00008006 },
	{ XFI_RG05,   0x0000A093, 0x0000A090 },
	{ XFI_RG06,   0x00004C4C, 0x00004C4C },
	{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	{ XFI_RG08,   0x0000202A, 0x0000202A },
	{ XFI_RG09,   0x00008540, 0x00008640 },
	{ XFI_RG0A,   0x00008180, 0x00008000 },
	{ XFI_RG0B,   0x00004002, 0x00004002 },
	{ XFI_RG0C,   0x000000FF, 0x000001FF },
	{ XFI_RG0D,   0x00000046, 0x00000046 },
	{ XFI_RG0E,   0x00008786, 0x00008786 },
	{ XFI_RG0F,   0x000003ED, 0x000003ED },
	{ XFI_RG10,   0x0000001C, 0x0000001C },
	{ XFI_RG11,   0x00000400, 0x00000400 },
	{ XFI_RG12,   0x00007D10, 0x00007D10 },
	{ XFI_RG13,   0x000000FF, 0x000000FF },
	{ XFI_RG14,   0x00000000, 0x00000000 },
	{ XFI_RG15,   0x00000111, 0x00000111 },
	{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	{ XFI_RG17,   0x0000820F, 0x0000000F },
	{ XFI_RG18,   0x0000820F, 0x000028AF },
	{ XFI_RG19,   0x0000820F, 0x000028AF },
	{ XFI_RG1A,   0x000083D0, 0x00002BD0 },
	{ XFI_RG1B,   0x000000C0, 0x000000C0 },
	{ XFI_RG1C,   0x00000062, 0x00000062 },
	{ XFI_RG1D,   0x00000430, 0x00000430 },
	{ XFI_RG1E,   0x00008000, 0x00000001 },
	{ XFI_RG1F,   0x00000000, 0x00000000 },
	{ XFI_RG20,   0x0000001F, 0x0000001F },
	{ XFI_RG21,   0x00001F04, 0x00001F04 },
	{ XFI_RG22,   0x0000F960, 0x0000F900 },
	{ XFI_RG23,   0x0000000F, 0x0000000F },
	{ XFI_RG24,   0x00001F93, 0x00001F93 },
	{ XFI_RG25,   0x0000F000, 0x0000F000 },
	{ XFI_RG26,   0x000003FF, 0x000003FF },
	{ XFI_RG27,   0x000027C0, 0x000027C0 },
	{ XFI_RG28,   0x0000000F, 0x0000000F },
	{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	{ XFI_RG2A,   0x00000000, 0x00000000 },
	{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	{ XFI_RG2C,   0x00007D80, 0x00007C00 },
	{ XFI_RG2D,   0x0000401E, 0x0000401E },
	{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	{ XFI_RG2F,   0x00000000, 0x00000000 },
	{ XFI_RG30,   0x0000C924, 0x0000C924 },
	{ XFI_RG31,   0x0000C190, 0x0000C190 },
	{ XFI_RG32,   0x0000827A, 0x0000827A },
	{ XFI_RG33,   0x0000A36A, 0x0000A36A },
	{ XFI_RG34,   0x000024BA, 0x000024BA },
	{ XFI_RG35,   0x00000001, 0x00000001 },
	{ XFI_RG36,   0x00000002, 0x00000002 },
	{ XFI_RG37,   0x00003299, 0x00003299 },
	{ XFI_RG38,   0x0000A510, 0x0000A510 },
	{ XFI_RG39,   0x0000A510, 0x0000A510 },
	{ XFI_RG3A,   0x0000000D, 0x0000000D },
	{ XFI_RG40,   0x0C001180, 0x0C001180 },
	{ XFI_RG41,   0x09FFFF81, 0x09C00001 },
	{ XFI_RG42,   0x00000000, 0x00000000 },
	{ XFI_RG43,   0x00000005, 0x00000005 },
	{ XFI_RG44,   0x000057DF, 0x000057DF },
	{ XFI_RG45,   0x00001000, 0x00001000 },
	{ XFI_RG46,   0x003F08A7, 0x003188A7 },
	{ XFI_RG47,   0xBCA000DE, 0xBCA000DE },
	{ XFI_RGNEW0, 0x00000000, 0x00000000 },
	{ XFI_RGNEW1, 0x00000001, 0x00000000 },
	{ XFI_RGNEW2, 0x00000035, 0x00000035 },
	{ XFI_RGNEW3, 0x0AC5402C, 0x0AC5402C },
	{ XFI_RGNEW4, 0x02380000, 0x02380000 },
	{ XFI_RGNEW5, 0x03030D00, 0x03030D00 },
	{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg0_1g1[__XFI_CFG_REG_NUM][3]    = {
	{ XFI_RG00,   0x000061F0, 0x00000190 },
	   { XFI_RG01,   0x00004804, 0x00004800 },
	   { XFI_RG02,   0x0000540F, 0x0000040F },
	   { XFI_RG03,   0x0000C13D, 0x0000C13D },
	   { XFI_RG04,   0x00008006, 0x00008006 },
	   { XFI_RG05,   0x0000A093, 0x0000A033 },
	   { XFI_RG06,   0x00004C4C, 0x00004C4C },
	   { XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	   { XFI_RG08,   0x0000202A, 0x0000202A },
	   { XFI_RG09,   0x00008540, 0x00008540 },
	   { XFI_RG0A,   0x00008180, 0x00008180 },
	   { XFI_RG0B,   0x00004002, 0x00004002 },
	   { XFI_RG0C,   0x000000FF, 0x000001FF },
	   { XFI_RG0D,   0x00000046, 0x0000007C },
	   { XFI_RG0E,   0x00008786, 0x00008786 },
	   { XFI_RG0F,   0x000003ED, 0x000003ED },
	   { XFI_RG10,   0x0000001C, 0x0000001C },
	   { XFI_RG11,   0x00000400, 0x00000400 },
	   { XFI_RG12,   0x00007D10, 0x00007D10 },
	   { XFI_RG13,   0x000000FF, 0x000000FF },
	   { XFI_RG14,   0x00000000, 0x00000000 },
	   { XFI_RG15,   0x00000111, 0x00000111 },
	   { XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	   { XFI_RG17,   0x0000820F, 0x0000820F },
	   { XFI_RG18,   0x0000820F, 0x0000820F },
	   { XFI_RG19,   0x0000820F, 0x0000820F },
	   { XFI_RG1A,   0x000083D0, 0x000083D0 },
	   { XFI_RG1B,   0x000000C0, 0x000000C0 },
	   { XFI_RG1C,   0x00000062, 0x00000062 },
	   { XFI_RG1D,   0x00000430, 0x00000430 },
	   { XFI_RG1E,   0x00008000, 0x00008000 },
	   { XFI_RG1F,   0x00000000, 0x00000000 },
	   { XFI_RG20,   0x0000001F, 0x00003F1F },
	   { XFI_RG21,   0x00001F04, 0x00000024 },
	   { XFI_RG22,   0x0000F960, 0x0000F960 },
	   { XFI_RG23,   0x0000000F, 0x0000000F },
	   { XFI_RG24,   0x00001F93, 0x00001F93 },
	   { XFI_RG25,   0x0000F000, 0x0000F000 },
	   { XFI_RG26,   0x000003FF, 0x000003FF },
	   { XFI_RG27,   0x000027C0, 0x000027C0 },
	   { XFI_RG28,   0x0000000F, 0x0000000F },
	   { XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	   { XFI_RG2A,   0x00000000, 0x00000000 },
	   { XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	   { XFI_RG2C,   0x00007D80, 0x00007D80 },
	   { XFI_RG2D,   0x0000401E, 0x00004010 },
	   { XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	   { XFI_RG2F,   0x00000000, 0x00000000 },
	   { XFI_RG30,   0x0000C924, 0x0000C924 },
	   { XFI_RG31,   0x0000C190, 0x0000C190 },
	   { XFI_RG32,   0x0000827A, 0x0000FFFA },
	   { XFI_RG33,   0x0000A36A, 0x0000A36A },
	   { XFI_RG34,   0x000024BA, 0x000024BA },
	   { XFI_RG35,   0x00000001, 0x00000001 },
	   { XFI_RG36,   0x00000002, 0x00000002 },
	   { XFI_RG37,   0x00003299, 0x00003299 },
	   { XFI_RG38,   0x0000A510, 0x0000A510 },
	   { XFI_RG39,   0x0000A510, 0x0000A510 },
	   { XFI_RG3A,   0x0000000D, 0x0000000D },
	   { XFI_RG40,   0x0C001180, 0x0C001180 },
	   { XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
	   { XFI_RG42,   0x00000000, 0x00000000 },
	   { XFI_RG43,   0x00000005, 0x00000005 },
	   { XFI_RG44,   0x000057DF, 0x00005F00 },
	   { XFI_RG45,   0x00001000, 0x00001200 },
	   { XFI_RG46,   0x003F08A7, 0x003F08A7 },
	   { XFI_RG47,   0xBCA000DE, 0x9CA001DE },
	   { XFI_RGNEW0, 0x00000000, 0x00000000 },
	   { XFI_RGNEW1, 0x00000001, 0x00000000 },
	   { XFI_RGNEW2, 0x00000035, 0x00000035 },
	   { XFI_RGNEW3, 0x0AC5402C, 0x0AC5403C },
	   { XFI_RGNEW4, 0x02380000, 0x03F80000 },
	   { XFI_RGNEW5, 0x03030D00, 0x03010D00 },
	   { XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - take 1g of xfi 0 and xg of xfi 1
//
ca_uint32_t g_xfi_reg_init_tbl_1g0_xg1[__XFI_CFG_REG_NUM][3]    = {
	{ XFI_RG00,    0x00000190, 0x000061F0 },
	   { XFI_RG01,    0x00004800, 0x00004804 },
	   { XFI_RG02,    0x0000040F, 0x0000340F },
	   { XFI_RG03,    0x0000C13D, 0x0000C13D },
	   { XFI_RG04,    0x00008006, 0x00008006 },
	   { XFI_RG05,    0x0000A033, 0x0000A093 },
	   { XFI_RG06,    0x00004C4C, 0x00004C4C },
	   { XFI_RG07,    0x0000FFFC, 0x0000FFFC },
	   { XFI_RG08,    0x0000202A, 0x0000202A },
	   { XFI_RG09,    0x00008540, 0x00008540 },
	   { XFI_RG0A,    0x00008180, 0x00008180 },
	   { XFI_RG0B,    0x00004002, 0x00004002 },
	   { XFI_RG0C,    0x000001FF, 0x000000FF },
	   { XFI_RG0D,    0x0000007C, 0x00000046 },
	   { XFI_RG0E,    0x00008786, 0x00008786 },
	   { XFI_RG0F,    0x000003ED, 0x000003ED },
	   { XFI_RG10,    0x0000001C, 0x0000001C },
	   { XFI_RG11,    0x00000400, 0x00000400 },
	   { XFI_RG12,    0x00007D10, 0x00007D10 },
	   { XFI_RG13,    0x000000FF, 0x000000FF },
	   { XFI_RG14,    0x00000000, 0x00000000 },
	   { XFI_RG15,    0x00000111, 0x00000111 },
	   { XFI_RG16,    0x0000FFFF, 0x0000FFFF },
	   { XFI_RG17,    0x0000820F, 0x0000820F },
	   { XFI_RG18,    0x0000820F, 0x0000820F },
	   { XFI_RG19,    0x0000820F, 0x0000820F },
	   { XFI_RG1A,    0x000083D0, 0x000083D0 },
	   { XFI_RG1B,    0x000000C0, 0x000000C0 },
	   { XFI_RG1C,    0x00000062, 0x00000062 },
	   { XFI_RG1D,    0x00000430, 0x00000430 },
	   { XFI_RG1E,    0x00008000, 0x00008000 },
	   { XFI_RG1F,    0x00000000, 0x00000000 },
	   { XFI_RG20,    0x00003F1F, 0x0000001F },
	   { XFI_RG21,    0x00000024, 0x00001F04 },
	   { XFI_RG22,    0x0000F960, 0x0000F960 },
	   { XFI_RG23,    0x0000000F, 0x0000000F },
	   { XFI_RG24,    0x00001F93, 0x00001F93 },
	   { XFI_RG25,    0x0000F000, 0x0000F000 },
	   { XFI_RG26,    0x000003FF, 0x000003FF },
	   { XFI_RG27,    0x000027C0, 0x000027C0 },
	   { XFI_RG28,    0x0000000F, 0x0000000F },
	   { XFI_RG29,    0x0000FC9F, 0x0000FC9F },
	   { XFI_RG2A,    0x00000000, 0x00000000 },
	   { XFI_RG2B,    0x00003FF2, 0x00003FF2 },
	   { XFI_RG2C,    0x00007D80, 0x00007D80 },
	   { XFI_RG2D,    0x00004010, 0x0000401E },
	   { XFI_RG2E,    0x0000E3FE, 0x0000E3FE },
	   { XFI_RG2F,    0x00000000, 0x00000000 },
	   { XFI_RG30,    0x0000C924, 0x0000C924 },
	   { XFI_RG31,    0x0000C190, 0x0000C190 },
	   { XFI_RG32,    0x0000FFFA, 0x0000827A },
	   { XFI_RG33,    0x0000A36A, 0x0000A36A },
	   { XFI_RG34,    0x000024BA, 0x000024BA },
	   { XFI_RG35,    0x00000001, 0x00000001 },
	   { XFI_RG36,    0x00000002, 0x00000002 },
	   { XFI_RG37,    0x00003299, 0x00003299 },
	   { XFI_RG38,    0x0000A510, 0x0000A510 },
	   { XFI_RG39,    0x0000A510, 0x0000A510 },
	   { XFI_RG3A,    0x0000000D, 0x0000000D },
	   { XFI_RG40,    0x0C001180, 0x0C001180 },
	   { XFI_RG41,    0x09FFFF81, 0x09FFFF81 },
	   { XFI_RG42,    0x00000000, 0x00000000 },
	   { XFI_RG43,    0x00000005, 0x00000005 },
	   { XFI_RG44,    0x00005F00, 0x000057DF },
	   { XFI_RG45,    0x00001200, 0x00001000 },
	   { XFI_RG46,    0x003F08A7, 0x003F08A7 },
	   { XFI_RG47,    0x9CA001DE, 0xBCA000DE },
	   { XFI_RGNEW0,  0x00000000, 0x00000000 },
	   { XFI_RGNEW1,  0x00000000, 0x00000001 },
	   { XFI_RGNEW2,  0x00000035, 0x00000035 },
	   { XFI_RGNEW3,  0x0AC5403C, 0x0AC5402C },
	   { XFI_RGNEW4,  0x03F80000, 0x02380000 },
	   { XFI_RGNEW5,  0x03010D00, 0x03030D00 },
	   { XFI_RGNEW6,  0x00000000, 0x00000000 }
};
//
// release ver - 20190221
//
ca_uint32_t g_xfi_reg_init_tbl_xg0_2d5g1[__XFI_CFG_REG_NUM][3]  = {
	{ XFI_RG00,   0x000061F0, 0x00002190 },
	 { XFI_RG01,   0x00004804, 0x00007800 },
	 { XFI_RG02,   0x0000540F, 0x0000040F },
	 { XFI_RG03,   0x0000C13D, 0x0000C13D },
	 { XFI_RG04,   0x00008006, 0x00008006 },
	 { XFI_RG05,   0x0000A093, 0x0000A093 },
	 { XFI_RG06,   0x00004C4C, 0x00004C4C },
	 { XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	 { XFI_RG08,   0x0000202A, 0x0000202A },
	 { XFI_RG09,   0x00008540, 0x00008540 },
	 { XFI_RG0A,   0x00008180, 0x00008180 },
	 { XFI_RG0B,   0x00004002, 0x00004002 },
	 { XFI_RG0C,   0x000000FF, 0x000001FF },
	 { XFI_RG0D,   0x00000046, 0x00000060 },
	 { XFI_RG0E,   0x00008786, 0x00008786 },
	 { XFI_RG0F,   0x000003ED, 0x000003ED },
	 { XFI_RG10,   0x0000001C, 0x0000001C },
	 { XFI_RG11,   0x00000400, 0x00000400 },
	 { XFI_RG12,   0x00007D10, 0x00007D10 },
	 { XFI_RG13,   0x000000FF, 0x000000FF },
	 { XFI_RG14,   0x00000000, 0x00000000 },
	 { XFI_RG15,   0x00000111, 0x00000111 },
	 { XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	 { XFI_RG17,   0x0000820F, 0x0000820F },
	 { XFI_RG18,   0x0000820F, 0x0000820F },
	 { XFI_RG19,   0x0000820F, 0x0000820F },
	 { XFI_RG1A,   0x000083D0, 0x000083D0 },
	 { XFI_RG1B,   0x000000C0, 0x000000C0 },
	 { XFI_RG1C,   0x00000062, 0x00000062 },
	 { XFI_RG1D,   0x00000430, 0x00000430 },
	 { XFI_RG1E,   0x00008000, 0x00008000 },
	 { XFI_RG1F,   0x00000000, 0x00000000 },
	 { XFI_RG20,   0x0000001F, 0x00003F1F },
	 { XFI_RG21,   0x00001F04, 0x00000024 },
	 { XFI_RG22,   0x0000F960, 0x0000F960 },
	 { XFI_RG23,   0x0000000F, 0x0000000F },
	 { XFI_RG24,   0x00001F93, 0x00001F93 },
	 { XFI_RG25,   0x0000F000, 0x0000F000 },
	 { XFI_RG26,   0x000003FF, 0x000003FF },
	 { XFI_RG27,   0x000027C0, 0x000027C0 },
	 { XFI_RG28,   0x0000000F, 0x0000000F },
	 { XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	 { XFI_RG2A,   0x00000000, 0x00000000 },
	 { XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	 { XFI_RG2C,   0x00007D80, 0x00007D80 },
	 { XFI_RG2D,   0x0000401E, 0x00004010 },
	 { XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	 { XFI_RG2F,   0x00000000, 0x00000000 },
	 { XFI_RG30,   0x0000C924, 0x0000C924 },
	 { XFI_RG31,   0x0000C190, 0x0000C190 },
	 { XFI_RG32,   0x0000827A, 0x0000FFFA },
	 { XFI_RG33,   0x0000A36A, 0x0000A36A },
	 { XFI_RG34,   0x000024BA, 0x000024BA },
	 { XFI_RG35,   0x00000001, 0x00000001 },
	 { XFI_RG36,   0x00000002, 0x00000002 },
	 { XFI_RG37,   0x00003299, 0x00003299 },
	 { XFI_RG38,   0x0000A510, 0x0000A510 },
	 { XFI_RG39,   0x0000A510, 0x0000A510 },
	 { XFI_RG3A,   0x0000000D, 0x0000000D },
	 { XFI_RG40,   0x0C001180, 0x0C001180 },
	 { XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
	 { XFI_RG42,   0x00000000, 0x00000000 },
	 { XFI_RG43,   0x00000005, 0x00000405 },
	 { XFI_RG44,   0x000057DF, 0x000057DF },
	 { XFI_RG45,   0x00001000, 0x00001000 },
	 { XFI_RG46,   0x003F08A7, 0x003F08A7 },
	 { XFI_RG47,   0xBCA000DE, 0x9CA001DE },
	 { XFI_RGNEW0, 0x00000000, 0x00000000 },
	 { XFI_RGNEW1, 0x00000001, 0x00000000 },
	 { XFI_RGNEW2, 0x00000035, 0x00000035 },
	 { XFI_RGNEW3, 0x0AC5402C, 0x0AF7403C },
	 { XFI_RGNEW4, 0x02380000, 0x07F80000 },
	 { XFI_RGNEW5, 0x03030D00, 0x03010D00 },
	 { XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//20190221 release for 2.5G for XFI0 + 10G for XFI1
ca_uint32_t g_xfi_reg_init_tbl_2d5g0_xg1[__XFI_CFG_REG_NUM][3]  = { 
	{ XFI_RG00, 0x00002190, 0x000061F0 },
	 { XFI_RG01, 0x00007800, 0x00004804 },
	 { XFI_RG02, 0x0000040F, 0x0000540F },
	 { XFI_RG03, 0x0000C13D, 0x0000C13D },
	 { XFI_RG04, 0x00008006, 0x00008006 },
	 { XFI_RG05, 0x0000A093, 0x0000A093 },
	 { XFI_RG06, 0x00004C4C, 0x00004C4C },
	 { XFI_RG07, 0x0000FFFC, 0x0000FFFC },
	 { XFI_RG08, 0x0000202A, 0x0000202A },
	 { XFI_RG09, 0x00008540, 0x00008540 },
	 { XFI_RG0A, 0x00008180, 0x00008180 },
	 { XFI_RG0B, 0x00004002, 0x00004002 },
	 { XFI_RG0C, 0x000001FF, 0x000000FF },
	 { XFI_RG0D, 0x00000060, 0x00000046 },
	 { XFI_RG0E, 0x00008786, 0x00008786 },
	 { XFI_RG0F, 0x000003ED, 0x000003ED },
	 { XFI_RG10, 0x0000001C, 0x0000001C },
	 { XFI_RG11, 0x00000400, 0x00000400 },
	 { XFI_RG12, 0x00007D10, 0x00007D10 },
	 { XFI_RG13, 0x000000FF, 0x000000FF },
	 { XFI_RG14, 0x00000000, 0x00000000 },
	 { XFI_RG15, 0x00000111, 0x00000111 },
	 { XFI_RG16, 0x0000FFFF, 0x0000FFFF },
	 { XFI_RG17, 0x0000820F, 0x0000820F },
	 { XFI_RG18, 0x0000820F, 0x0000820F },
	 { XFI_RG19, 0x0000820F, 0x0000820F },
	 { XFI_RG1A, 0x000083D0, 0x000083D0 },
	 { XFI_RG1B, 0x000000C0, 0x000000C0 },
	 { XFI_RG1C, 0x00000062, 0x00000062 },
	 { XFI_RG1D, 0x00000430, 0x00000430 },
	 { XFI_RG1E, 0x00008000, 0x00008000 },
	 { XFI_RG1F, 0x00000000, 0x00000000 },
	 { XFI_RG20, 0x00003F1F, 0x0000001F },
	 { XFI_RG21, 0x00000024, 0x00001F04 },
	 { XFI_RG22, 0x0000F960, 0x0000F960 },
	 { XFI_RG23, 0x0000000F, 0x0000000F },
	 { XFI_RG24, 0x00001F93, 0x00001F93 },
	 { XFI_RG25, 0x0000F000, 0x0000F000 },
	 { XFI_RG26, 0x000003FF, 0x000003FF },
	 { XFI_RG27, 0x000027C0, 0x000027C0 },
	 { XFI_RG28, 0x0000000F, 0x0000000F },
	 { XFI_RG29, 0x0000FC9F, 0x0000FC9F },
	 { XFI_RG2A, 0x00000000, 0x00000000 },
	 { XFI_RG2B, 0x00003FF2, 0x00003FF2 },
	 { XFI_RG2C, 0x00007D80, 0x00007D80 },
	 { XFI_RG2D, 0x00004010, 0x0000401E },
	 { XFI_RG2E, 0x0000E3FE, 0x0000E3FE },
	 { XFI_RG2F, 0x00000000, 0x00000000 },
	 { XFI_RG30, 0x0000C924, 0x0000C924 },
	 { XFI_RG31, 0x0000C190, 0x0000C190 },
	 { XFI_RG32, 0x0000FFFA, 0x0000827A },
	 { XFI_RG33, 0x0000A36A, 0x0000A36A },
	 { XFI_RG34, 0x000024BA, 0x000024BA },
	 { XFI_RG35, 0x00000001, 0x00000001 },
	 { XFI_RG36, 0x00000002, 0x00000002 },
	 { XFI_RG37, 0x00003299, 0x00003299 },
	 { XFI_RG38, 0x0000A510, 0x0000A510 },
	 { XFI_RG39, 0x0000A510, 0x0000A510 },
	 { XFI_RG3A, 0x0000000D, 0x0000000D },
	 { XFI_RG40, 0x0C001180, 0x0C001180 },
	 { XFI_RG41, 0x09FFFF81, 0x09FFFF81 },
	 { XFI_RG42, 0x00000000, 0x00000000 },
	 { XFI_RG43, 0x00000405, 0x00000005 },
	 { XFI_RG44, 0x000057DF, 0x000057DF },
	 { XFI_RG45, 0x00001000, 0x00001000 },
	 { XFI_RG46, 0x003F08A7, 0x003F08A7 },
	 { XFI_RG47, 0x9CA001DE, 0xBCA000DE },
	 { XFI_RGNEW0, 0x00000000, 0x00000000 },
	 { XFI_RGNEW1, 0x00000000, 0x00000001 },
	 { XFI_RGNEW2, 0x00000035, 0x00000035 },
	 { XFI_RGNEW3, 0x0AF7403C, 0x0AC5402C },
	 { XFI_RGNEW4, 0x07F80000, 0x02380000 },
	 { XFI_RGNEW5, 0x03030D00, 0x03030D00 },
	 { XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - 20180718
//
ca_uint32_t g_xfi_reg_init_tbl_2d5g[__XFI_CFG_REG_NUM][3]       = {
	{ XFI_RG00,   0x00002190, 0x00002190 },
	      { XFI_RG01,   0x00007800, 0x00007800 },
	      { XFI_RG02,   0x0000040F, 0x0000040F },
	      { XFI_RG03,   0x0000C13D, 0x0000C13D },
	      { XFI_RG04,   0x00008006, 0x00008006 },
	      { XFI_RG05,   0x0000A093, 0x0000A093 },
	      { XFI_RG06,   0x00004C4C, 0x00004C4C },
	      { XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	      { XFI_RG08,   0x0000202A, 0x0000202A },
	      { XFI_RG09,   0x00008540, 0x00008540 },
      #if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
	      { XFI_RG0A,   0x00000000, 0x00000000 },
      #else
	      { XFI_RG0A,   0x00008180, 0x00008180 },
      #endif
	      { XFI_RG0B,   0x00004002, 0x00004002 },
	      { XFI_RG0C,   0x000001FF, 0x000001FF },
	      { XFI_RG0D,   0x00000060, 0x00000060 },
	      { XFI_RG0E,   0x00008786, 0x00008786 },
	      { XFI_RG0F,   0x000003ED, 0x000003ED },
	      { XFI_RG10,   0x0000001C, 0x0000001C },
	      { XFI_RG11,   0x00000400, 0x00000400 },
	      { XFI_RG12,   0x00007D10, 0x00007D10 },
	      { XFI_RG13,   0x000000FF, 0x000000FF },
	      { XFI_RG14,   0x00000000, 0x00000000 },
	      { XFI_RG15,   0x00000111, 0x00000111 },
	      { XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	      { XFI_RG17,   0x0000820F, 0x0000820F },
	      { XFI_RG18,   0x0000820F, 0x0000820F },
	      { XFI_RG19,   0x0000820F, 0x0000820F },
	      { XFI_RG1A,   0x000083D0, 0x000083D0 },
	      { XFI_RG1B,   0x000000C0, 0x000000C0 },
	      { XFI_RG1C,   0x00000062, 0x00000062 },
	      { XFI_RG1D,   0x00000430, 0x00000430 },
	      { XFI_RG1E,   0x00008000, 0x00008000 },
	      { XFI_RG1F,   0x00000000, 0x00000000 },
	      { XFI_RG20,   0x00003F1F, 0x00003F1F },
	      { XFI_RG21,   0x00000024, 0x00000024 },
	      { XFI_RG22,   0x0000F960, 0x0000F960 },
	      { XFI_RG23,   0x0000000F, 0x0000000F },
	      { XFI_RG24,   0x00001F93, 0x00001F93 },
	      { XFI_RG25,   0x0000F000, 0x0000F000 },
	      { XFI_RG26,   0x000003FF, 0x000003FF },
	      { XFI_RG27,   0x000027C0, 0x000027C0 },
	      { XFI_RG28,   0x0000000F, 0x0000000F },
	      { XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	      { XFI_RG2A,   0x00000000, 0x00000000 },
	      { XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	      { XFI_RG2C,   0x00007D80, 0x00007D80 },
	      { XFI_RG2D,   0x00004010, 0x00004010 },
	      { XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	      { XFI_RG2F,   0x00000000, 0x00000000 },
	      { XFI_RG30,   0x0000C924, 0x0000C924 },
	      { XFI_RG31,   0x0000C190, 0x0000C190 },
	      { XFI_RG32,   0x0000FFFA, 0x0000FFFA },
	      { XFI_RG33,   0x0000A36A, 0x0000A36A },
	      { XFI_RG34,   0x000024BA, 0x000024BA },
	      { XFI_RG35,   0x00000001, 0x00000001 },
	      { XFI_RG36,   0x00000002, 0x00000002 },
	      { XFI_RG37,   0x00003299, 0x00003299 },
	      { XFI_RG38,   0x0000A510, 0x0000A510 },
	      { XFI_RG39,   0x0000A510, 0x0000A510 },
	      { XFI_RG3A,   0x0000000D, 0x0000000D },
	      { XFI_RG40,   0x0C001180, 0x0C001180 },
	      { XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
	      { XFI_RG42,   0x00000000, 0x00000000 },
	      { XFI_RG43,   0x00000405, 0x00000405 },
	      { XFI_RG44,   0x000057DF, 0x000057DF },
	      { XFI_RG45,   0x00001000, 0x00001000 },
	      { XFI_RG46,   0x003F08A7, 0x003F08A7 },
	      { XFI_RG47,   0x9CA001DE, 0x9CA001DE },
	      { XFI_RGNEW0, 0x00000000, 0x00000000 },
	      { XFI_RGNEW1, 0x00000000, 0x00000000 },
	      { XFI_RGNEW2, 0x00000035, 0x00000035 },
	      { XFI_RGNEW3, 0x0AF7403C, 0x0AF7403C },
	      { XFI_RGNEW4, 0x07F80000, 0x07F80000 },
	      { XFI_RGNEW5, 0x03010D00, 0x03010D00 },
	      { XFI_RGNEW6, 0x00000000, 0x00000000 }
};
//
// release ver - 20190125
//
ca_uint32_t g_xfi_reg_init_tbl_2g[__XFI_CFG_REG_NUM][3]         = {
	{ XFI_RG00,   0x00004190, 0x00004190 },
	{ XFI_RG01,   0x00004800, 0x00004800 },
	{ XFI_RG02,   0x0000040F, 0x0000040F },
	{ XFI_RG03,   0x0000C13D, 0x0000C13D },
	{ XFI_RG04,   0x00008006, 0x00008006 },
	{ XFI_RG05,   0x0000A033, 0x0000A033 },
	{ XFI_RG06,   0x00004C4C, 0x00004C4C },
	{ XFI_RG07,   0x0000FFFC, 0x0000FFFC },
	{ XFI_RG08,   0x0000202A, 0x0000202A },
	{ XFI_RG09,   0x00008540, 0x00008540 },
	{ XFI_RG0A,   0x00008180, 0x00008180 },
	{ XFI_RG0B,   0x00004002, 0x00004002 },
	{ XFI_RG0C,   0x000001FF, 0x000001FF },
	{ XFI_RG0D,   0x0000007C, 0x0000007C },
	{ XFI_RG0E,   0x00008786, 0x00008786 },
	{ XFI_RG0F,   0x000003ED, 0x000003ED },
	{ XFI_RG10,   0x0000001C, 0x0000001C },
	{ XFI_RG11,   0x00000400, 0x00000400 },
	{ XFI_RG12,   0x00007D10, 0x00007D10 },
	{ XFI_RG13,   0x000000FF, 0x000000FF },
	{ XFI_RG14,   0x00000000, 0x00000000 },
	{ XFI_RG15,   0x00000111, 0x00000111 },
	{ XFI_RG16,   0x0000FFFF, 0x0000FFFF },
	{ XFI_RG17,   0x0000820F, 0x0000820F },
	{ XFI_RG18,   0x0000820F, 0x0000820F },
	{ XFI_RG19,   0x0000820F, 0x0000820F },
	{ XFI_RG1A,   0x000083D0, 0x000083D0 },
	{ XFI_RG1B,   0x000000C0, 0x000000C0 },
	{ XFI_RG1C,   0x00000062, 0x00000062 },
	{ XFI_RG1D,   0x00000430, 0x00000430 },
	{ XFI_RG1E,   0x00008000, 0x00008000 },
	{ XFI_RG1F,   0x00000000, 0x00000000 },
	{ XFI_RG20,   0x00003F1F, 0x00003F1F },
	{ XFI_RG21,   0x00000024, 0x00000024 },
	{ XFI_RG22,   0x0000F960, 0x0000F960 },
	{ XFI_RG23,   0x0000000F, 0x0000000F },
	{ XFI_RG24,   0x00001F93, 0x00001F93 },
	{ XFI_RG25,   0x0000F000, 0x0000F000 },
	{ XFI_RG26,   0x000003FF, 0x000003FF },
	{ XFI_RG27,   0x000027C0, 0x000027C0 },
	{ XFI_RG28,   0x0000000F, 0x0000000F },
	{ XFI_RG29,   0x0000FC9F, 0x0000FC9F },
	{ XFI_RG2A,   0x00000000, 0x00000000 },
	{ XFI_RG2B,   0x00003FF2, 0x00003FF2 },
	{ XFI_RG2C,   0x00007D80, 0x00007D80 },
	{ XFI_RG2D,   0x00004010, 0x00004010 },
	{ XFI_RG2E,   0x0000E3FE, 0x0000E3FE },
	{ XFI_RG2F,   0x00000000, 0x00000000 },
	{ XFI_RG30,   0x0000C924, 0x0000C924 },
	{ XFI_RG31,   0x0000C190, 0x0000C190 },
	{ XFI_RG32,   0x0000FFFA, 0x0000FFFA },
	{ XFI_RG33,   0x0000A36A, 0x0000A36A },
	{ XFI_RG34,   0x000024BA, 0x000024BA },
	{ XFI_RG35,   0x00000001, 0x00000001 },
	{ XFI_RG36,   0x00000002, 0x00000002 },
	{ XFI_RG37,   0x00003299, 0x00003299 },
	{ XFI_RG38,   0x0000A510, 0x0000A510 },
	{ XFI_RG39,   0x0000A510, 0x0000A510 },
	{ XFI_RG3A,   0x0000000D, 0x0000000D },
	{ XFI_RG40,   0x0C001180, 0x0C001180 },
	{ XFI_RG41,   0x09FFFF81, 0x09FFFF81 },
	{ XFI_RG42,   0x00000000, 0x00000000 },
	{ XFI_RG43,   0x00000005, 0x00000005 },
	{ XFI_RG44,   0x00005F00, 0x00005F00 },
	{ XFI_RG45,   0x00001200, 0x00001200 },
	{ XFI_RG46,   0x003F08A7, 0x003F08A7 },
	{ XFI_RG47,   0x9CA001DE, 0x9CA001DE },
	{ XFI_RGNEW0, 0x00000000, 0x00000000 },
	{ XFI_RGNEW1, 0x00000000, 0x00000000 },
	{ XFI_RGNEW2, 0x00000035, 0x00000035 },
	{ XFI_RGNEW3, 0x0AC5403C, 0x0AC5403C },
	{ XFI_RGNEW4, 0x03F80000, 0x03F80000 },
	{ XFI_RGNEW5, 0x03010D00, 0x03010D00 },
	{ XFI_RGNEW6, 0x00000000, 0x00000000 }
};


/** __xfi_cfg_init_xg
 *@breif: initialize XFI & NI-MAC as 10G
 */
static void
__xfi_cfg_init_xg(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx = 0;
	ca_uint32_t(*prof)[3] = NULL; /* XFI init profile pointer */

	switch(g_xfi_drv.xfi_bmp){

	case 0x01: /* xfi0 on, xfi1 off */
		prof = g_xfi_reg_init_tbl_xg_1off0on;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 OFF\r\n");
#endif
		break;
	case 0x02: /* xfi0 off, xfi1 on */
		prof = g_xfi_reg_init_tbl_xg_0off1on;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 OFF, XFI1 ON\r\n");
#endif
		break;
	case 0x03: /* xfi0 on, xfi1 on */
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 ON\r\n");
#endif
		if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_10000)
			prof = g_xfi_reg_init_tbl_xg;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_1000)
			prof = g_xfi_reg_init_tbl_xg0_1g1;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_2500)
			prof = g_xfi_reg_init_tbl_xg0_2d5g1;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_2500 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_10000)
			prof = g_xfi_reg_init_tbl_2d5g0_xg1;
		else {
			ca_printf("ca-ne: ERROR, not support XFI scfg configurtion xfi0 speed %d, xfi1 speed %d\r\n",
				g_xfi_drv.xfi_speed[0],g_xfi_drv.xfi_speed[1]);
			return;
		}
		break;
	default  : /* both off */
		ca_printf("ca-ne: Both XFI ports are OFF in scfg, neednt init!\r\n");
		return;
	}

	for(idx = 0; idx < __XFI_CFG_REG_NUM; idx++){
		if(XFI_RG46 == prof[idx][0]) { /* power on but disable RX_DATA_EN, to prevent pkt incoming */
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1] & 0x3bFFFF);
			ca_mdelay(30);
		} else if(XFI_RGNEW5 == prof[idx][0]) {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_mdelay(20);
		} else if(XFI_RG48 <= prof[idx][0] && XFI_RG4D >= prof[idx][0]) { /* read-only registers */
			//ca_udelay(1);
		} else {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_udelay(10);
		}
	}
}

static void __xfi_cfg_init_1g(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx = 0;
	ca_uint32_t(*prof)[3] = NULL; /* XFI init profile pointer */

	switch(g_xfi_drv.xfi_bmp){

	case 0x01: /* xfi0 on, xfi1 off */
		prof = g_xfi_reg_init_tbl_1g;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 OFF\r\n");
#endif
		break;
	case 0x02: /* xfi0 off, xfi1 on */
		prof = g_xfi_reg_init_tbl_1g;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 OFF, XFI1 ON\r\n");
#endif
		break;
	case 0x03: /* xfi0 on, xfi1 on */
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 ON\r\n");
#endif
		if(g_xfi_drv.xfi_speed[0] <= AAL_PHY_SPEED_1000 && g_xfi_drv.xfi_speed[1] <= AAL_PHY_SPEED_1000) {
			prof = g_xfi_reg_init_tbl_1g;
#ifdef _DEBUG_
			ca_printf("ca-ne: XFI init use g_xfi_reg_init_tbl_1g\r\n");
#endif
		} else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_1000) {
			prof = g_xfi_reg_init_tbl_xg0_1g1;
#ifdef _DEBUG_
			ca_printf("ca-ne: XFI init use g_xfi_reg_init_tbl_xg0_1g1\r\n");
#endif
		} else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_1000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_10000)
			prof = g_xfi_reg_init_tbl_1g0_xg1;
		else if(g_xfi_drv.xfi_speed[0] <= AAL_PHY_SPEED_100 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_10000)
			prof = g_xfi_reg_init_tbl_1g0_xg1;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] <= AAL_PHY_SPEED_100)
			prof = g_xfi_reg_init_tbl_xg0_1g1;
		else {
			ca_printf("ca-ne: ERROR, not support XFI scfg configurtion xfi0 speed %d, xfi1 speed %d NOW!\r\n",
				g_xfi_drv.xfi_speed[0],g_xfi_drv.xfi_speed[1]);
			return;
		}
		break;
	default  : /* both off */
		ca_printf("ca-ne: Both XFI ports are OFF in scfg, neednt init!\r\n");
		return;
	}

	for(idx = 0; idx < __XFI_CFG_REG_NUM; idx++){
		if(XFI_RG46 == prof[idx][0]) { /* power on but disable RX_DATA_EN, to prevent pkt incoming */
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1] & 0x3bFFFF);
			ca_mdelay(30);
		} else if(XFI_RGNEW5 == prof[idx][0]) {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_mdelay(10);
		} else if(XFI_RG48 <= prof[idx][0] && XFI_RG4D >= prof[idx][0]) { /* read-only registers */
			//ca_udelay(1);
		} else {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_udelay(10);
		}
	}
}


#if !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
static void __xfi_cfg_init_2g(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx = 0;

	for(idx = 0; idx < __XFI_CFG_REG_NUM; idx++){
		XFI_REG_WRITE(device_id,pcs_index,g_xfi_reg_init_tbl_2g[idx][0],g_xfi_reg_init_tbl_2g[idx][pcs_index + 1]);
		if(XFI_RG46 == g_xfi_reg_init_tbl_2g[idx][0] || XFI_RGNEW5 == g_xfi_reg_init_tbl_2g[idx][0])
			ca_mdelay(20);
		else
			ca_udelay(10);

	}
	ca_mdelay(10);
}

#endif
static void
__xfi_cfg_init_2d5g(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx = 0;
	ca_uint32_t(*prof)[3] = NULL; /* XFI init profile pointer */

	switch(g_xfi_drv.xfi_bmp){

	case 0x01: /* xfi0 on, xfi1 off */
		prof = g_xfi_reg_init_tbl_2d5g;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 OFF\r\n");
#endif
		break;
	case 0x02: /* xfi0 off, xfi1 on */
		prof = g_xfi_reg_init_tbl_2d5g;
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 OFF, XFI1 ON\r\n");
#endif
		break;
	case 0x03: /* xfi0 on, xfi1 on */
#ifdef _DEBUG_
		ca_printf("ca-ne: XFI init profile XFI0 ON, XFI1 ON\r\n");
#endif
		if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_2500 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_2500)
			prof = g_xfi_reg_init_tbl_2d5g;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_10000 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_2500)
			prof = g_xfi_reg_init_tbl_xg0_2d5g1;
		else if(g_xfi_drv.xfi_speed[0] == AAL_PHY_SPEED_2500 && g_xfi_drv.xfi_speed[1] == AAL_PHY_SPEED_10000)
			prof = g_xfi_reg_init_tbl_2d5g0_xg1;
		else {
			ca_printf("ca-ne: ERROR, not support XFI scfg configurtion xfi0 speed %d, xfi1 speed %d\r\n",
				g_xfi_drv.xfi_speed[0],g_xfi_drv.xfi_speed[1]);
			return;
		}
		break;
	default : /* both off */
		ca_printf("ca-ne: Both XFI ports are OFF in scfg, neednt init!\r\n");
		return;
	}
	for(idx = 0; idx < __XFI_CFG_REG_NUM; idx++){
		if(XFI_RG46 == prof[idx][0]) { /* power on but disable RX_DATA_EN, to prevent pkt incoming */
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1] & 0x3bFFFF);
			ca_mdelay(30);
		} else if(XFI_RGNEW5 == prof[idx][0]) {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_mdelay(20);
		} else if(XFI_RG48 <= prof[idx][0] && XFI_RG4D >= prof[idx][0]) { /* read-only registers */
		} else {
			XFI_REG_WRITE(device_id,pcs_index,prof[idx][0],prof[idx][pcs_index + 1]);
			ca_udelay(10);
		}
	}
}


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
static ca_status_t
__xfi_leq_en_toggle(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	XFI_RGNEW5_t rgNew5;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
	if(pcs_index != 1) {
		return CA_E_OK;
	}
#endif

	XFI_REG_WRITE(device_id,pcs_index,XFI_RGNEW5,0x1030c00);
	ca_mdelay(1);
	XFI_REG_WRITE(device_id,pcs_index,XFI_RGNEW5,0x3030c00);
	ca_mdelay(1);

	return CA_E_OK;
}
#endif

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
static void __xfi_cfg_init_xg(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t idx     = 0;
	ca_uint32_t reg_tbl[__XFI_CFG_REG_NUM][3] = { /* xfi0_off_xfi1_10g_20180718 */
		{ SATURN_XFI_RG00,   0x00006190, 0x00006190 },
		{ SATURN_XFI_RG01,   0x00004800, 0x00004800 },
		{ SATURN_XFI_RG02,   0x0000044F, 0x0000040F },
		{ SATURN_XFI_RG03,   0x0000C12D, 0x0000C12D },
		{ SATURN_XFI_RG04,   0x00008006, 0x00008006 },
		{ SATURN_XFI_RG05,   0x0000A090, 0x0000A090 },
		{ SATURN_XFI_RG06,   0x0000604C, 0x0000604C },
		{ SATURN_XFI_RG07,   0x0000FFFC, 0x0000FFFC },
		{ SATURN_XFI_RG08,   0x0000202A, 0x0000202A },
		{ SATURN_XFI_RG09,   0x00008640, 0x00008640 },
		{ SATURN_XFI_RG0A,   0x00008000, 0x00008000 },
		{ SATURN_XFI_RG0B,   0x00004002, 0x00004002 },
		{ SATURN_XFI_RG0C,   0x000001FF, 0x000001FF },
		{ SATURN_XFI_RG0D,   0x00000046, 0x00000046 },
		{ SATURN_XFI_RG0E,   0x00008786, 0x00008786 },
		{ SATURN_XFI_RG0F,   0x000003ED, 0x000003ED },
		{ SATURN_XFI_RG10,   0x0000001C, 0x0000001C },
		{ SATURN_XFI_RG11,   0x00000400, 0x00000400 },
		{ SATURN_XFI_RG12,   0x00007D10, 0x00007D10 },
		{ SATURN_XFI_RG13,   0x000000FF, 0x000000FF },
		{ SATURN_XFI_RG14,   0x00000000, 0x00000000 },
		{ SATURN_XFI_RG15,   0x00000111, 0x00000111 },
		{ SATURN_XFI_RG16,   0x0000FFFF, 0x0000FFFF },
		{ SATURN_XFI_RG17,   0x0000000F, 0x0000000F },
		{ SATURN_XFI_RG18,   0x000028AF, 0x000028AF },
		{ SATURN_XFI_RG19,   0x000028AF, 0x000028AF },
		{ SATURN_XFI_RG1A,   0x00002BD0, 0x00002BD0 },
		{ SATURN_XFI_RG1B,   0x000000C0, 0x000000C0 },
		{ SATURN_XFI_RG1C,   0x00000062, 0x00000062 },
		{ SATURN_XFI_RG1D,   0x00000430, 0x00000430 },
		{ SATURN_XFI_RG1E,   0x00000001, 0x00000001 },
		{ SATURN_XFI_RG1F,   0x00000000, 0x00000000 },
		{ SATURN_XFI_RG20,   0x0000001F, 0x0000001F },
		{ SATURN_XFI_RG21,   0x00001F04, 0x00001F04 },
		{ SATURN_XFI_RG22,   0x0000F900, 0x0000F900 },
		{ SATURN_XFI_RG23,   0x00000001, 0x00000001 },
		{ SATURN_XFI_RG24,   0x00001F8D, 0x00001F8D },
		{ SATURN_XFI_RG25,   0x0000F000, 0x0000F000 },
		{ SATURN_XFI_RG26,   0x000003FF, 0x000003FF },
		{ SATURN_XFI_RG27,   0x000007C0, 0x000007C0 },
		{ SATURN_XFI_RG28,   0x0000000F, 0x0000000F },
		{ SATURN_XFI_RG29,   0x0000FC1F, 0x0000FC1F },
		{ SATURN_XFI_RG2A,   0x00000000, 0x00000000 },
		{ SATURN_XFI_RG2B,   0x00003FF0, 0x00003FF0 },
		{ SATURN_XFI_RG2C,   0x00007C01, 0x00007C01 },
		{ SATURN_XFI_RG2D,   0x0000401E, 0x0000401E },
		{ SATURN_XFI_RG2E,   0x0000C36C, 0x0000C36C },
		{ SATURN_XFI_RG2F,   0x00000000, 0x00000000 },
		{ SATURN_XFI_RG30,   0x0000C924, 0x0000C924 },
		{ SATURN_XFI_RG31,   0x0000C190, 0x0000C190 },
		{ SATURN_XFI_RG32,   0x0000827A, 0x0000827A },
		{ SATURN_XFI_RG33,   0x0000A36A, 0x0000A36A },
		{ SATURN_XFI_RG34,   0x000024BA, 0x000024BA },
		{ SATURN_XFI_RG35,   0x00000001, 0x00000001 },
		{ SATURN_XFI_RG36,   0x00000002, 0x00000002 },
		{ SATURN_XFI_RG37,   0x00001299, 0x00001299 },
		{ SATURN_XFI_RG38,   0x0000A510, 0x0000A510 },
		{ SATURN_XFI_RG39,   0x0000A510, 0x0000A510 },
		{ SATURN_XFI_RG3A,   0x0000000D, 0x0000000D },
		{ SATURN_XFI_RG40,   0x0C001180, 0x0C001180 },
		{ SATURN_XFI_RG41,   0x09C00001, 0x09C00001 },
		{ SATURN_XFI_RG42,   0x00000000, 0x00000000 },
		{ SATURN_XFI_RG43,   0x00000005, 0x00000005 },
		{ SATURN_XFI_RG44,   0x000057DF, 0x000057DF },
		{ SATURN_XFI_RG45,   0x00001000, 0x00001000 },
		{ SATURN_XFI_RG46,   0x003188A7, 0x003F08A7 },
		{ SATURN_XFI_RG47,   0xBCA000DE, 0xBCA000DE },
		{ SATURN_XFI_RGNEW0, 0x00000000, 0x00000000 },
		{ SATURN_XFI_RGNEW1, 0x00000000, 0x00000001 },
		{ SATURN_XFI_RGNEW2, 0x00000035, 0x00000035 },
		{ SATURN_XFI_RGNEW3, 0x0AC5402C, 0x0AC5402C },
		{ SATURN_XFI_RGNEW4, 0x00380000, 0x02380000 },
		{ SATURN_XFI_RGNEW5, 0x03030D00, 0x03030D00 },
		{ SATURN_XFI_RGNEW6, 0x00000000, 0x00000000 }
	};

	if(device_id == CA_DEV_WAN) {
		for(idx = 0; idx < __XFI_CFG_REG_NUM; idx++){

			if(SATURN_XFI_RG32 == reg_tbl[idx][0]) {
				CA_HGU_8279_NE_REG_WRITE(g_xfi_rg32_param[pcs_index],reg_tbl[idx][0] + XFI_STRIDE * pcs_index);
#if 0
				if (pcs_index == 1)
				CA_HGU_8279_NE_REG_WRITE(g_xfi_rg32_param[1], reg_tbl[idx][0] + XFI_STRIDE);
				else
				CA_HGU_8279_NE_REG_WRITE(g_xfi_rg32_param[0], reg_tbl[idx][0]);
#endif
			} else {
				CA_HGU_8279_NE_REG_WRITE(reg_tbl[idx][1 + pcs_index],reg_tbl[idx][0] + XFI_STRIDE * pcs_index);
#if 0
				if (pcs_index == 1)
				CA_HGU_8279_NE_REG_WRITE(reg_tbl[idx][2], reg_tbl[idx][0] + XFI_STRIDE);
				else
				CA_HGU_8279_NE_REG_WRITE(reg_tbl[idx][1], reg_tbl[idx][0]);
#endif
			}

			if(SATURN_XFI_RG46 == reg_tbl[idx][0])
				ca_mdelay(30);
			else
				ca_udelay(100);
		}
	}
}


static void __xfi_ctrl_flow_xg(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
/*
# start the XFI lane0
    devmem 0xF300C134 w 0x00030C00
    usleep 100
    devmem 0xF300C104 w 0x003788A7
    usleep 1000
    devmem 0xF300C134 w 0x02030C00
    usleep 100
    devmem 0xF300C134 w 0x03030C00
    
# start the XFI lane1
    devmem 0xF300D104 w 0x003188A7
    usleep 100
    devmem 0xF300D028 w 0x00008000
    usleep 100
    devmem 0xF300D134 w 0x00030C00
    usleep 100
    devmem 0xF300D104 w 0x003908A7
    usleep 100
    devmem 0xF300D104 w 0x003F08A7
    usleep 1000
    devmem 0xF300D134 w 0x02030C00
    usleep 100
    devmem 0xF300D134 w 0x03030C00
*/
	if(pcs_index == 0) {
		CA_HGU_8279_NE_REG_WRITE(0x00030C00,SATURN_XFI_RGNEW5);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x003788A7,SATURN_XFI_RG46);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x02030C00,SATURN_XFI_RGNEW5);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x03030C00,SATURN_XFI_RGNEW5);
		ca_udelay(1000);
	} else {
		CA_HGU_8279_NE_REG_WRITE(0x003188A7,SATURN_XFI_RG46 + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x00008000,SATURN_XFI_RG0A + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x00030C00,SATURN_XFI_RGNEW5 + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x003908A7,SATURN_XFI_RG46 + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x003F08A7,SATURN_XFI_RG46 + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x02030C00,SATURN_XFI_RGNEW5 + XFI_STRIDE);
		ca_udelay(1000);
		CA_HGU_8279_NE_REG_WRITE(0x03030C00,SATURN_XFI_RGNEW5 + XFI_STRIDE);
		ca_udelay(1000);
	}
	ca_printf("ca-ne: HGU saturn XFI %d control flow done.\r\n",pcs_index);

}

/**__hgu_wan_xfi_init
 *@brief: init HGU's SATURN XFI ports, XFI0 off, XFI1 up with 10G
 *@note: parameters from saturn-g3-intra-xfi-init-link.revB.0413.sh sent by Joe at 2018-04-13
 */
ca_status_t __hgu_wan_xfi_init(void)
{
	ca_uint32_t                       pcs_index = 0;
	ca_device_id_t                    device_id =  CA_DEV_WAN;
	SATURN_NI_HV_SXGMII_PCS_PCSSTS1_t pcs_sts;
	SATURN_XFI_RG4D_t                 rg4d_val;

	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		aal_xfi_reset(device_id,pcs_index);
	}

	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		__xfi_cfg_init_xg(device_id,pcs_index);
	}

	/* control flow to start XFI lanes */
	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		__xfi_ctrl_flow_xg(device_id,pcs_index);
	}

	/* reset interface again */
	//__xfi_pcs_reset(device_id, 1);
	CA_HGU_8279_NE_REG_WRITE(0xf6318,SATURN_NI_HV_GLB_INTF_RST_CONFIG);
	ca_mdelay(1);
	//CA_HGU_8279_NE_REG_WRITE(0xa6318, SATURN_NI_HV_GLB_INTF_RST_CONFIG); /* release port 6 MAC and PCS */
	CA_HGU_8279_NE_REG_WRITE(0x06318,SATURN_NI_HV_GLB_INTF_RST_CONFIG); /* release port 6 MAC and PCS */

	/* delay 50ms, and then check pcs#1 link status */
	ca_mdelay(50);
	rg4d_val.wrd = CA_HGU_8279_NE_REG_READ(SATURN_XFI_RG4D + XFI_STRIDE);
	pcs_sts.wrd  = CA_HGU_8279_NE_REG_READ(SATURN_NI_HV_SXGMII_PCS_PCSSTS1 + SXGMII_STRIDE);
	ca_printf("ca-ne: HGU saturn XFI 1: pcs_sts.wrd=0x%x  clk_rdy= %d, rxidle =%d,  ber_notify= %d\n",
		pcs_sts.wrd,rg4d_val.bf.clk_rdy,rg4d_val.bf.rxidle,rg4d_val.bf.ber_notify);

	return CA_E_OK;
}

ca_status_t __hgu_lan_xfi_init(void)
{
	NI_HV_SXGMII_PCS_PCSSTS1_t pcs_sts;
	XFI_RG4D_t                 rg4d_val;
	ca_device_id_t             device_id =  CA_DEV_LAN;
	ca_uint32_t                pcs_index = 0;

	/* reset interfaces */
	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		aal_xfi_reset(device_id,pcs_index);
	}

	/* config interfaces */
	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		__xfi_cfg_init_xg_g3(device_id,pcs_index);
	}

	/* control flow */
	for(pcs_index = 0; pcs_index < XFI_NUM; ++pcs_index){
		__xfi_ctrl_flow_xg_g3(device_id,pcs_index);
		__xfi_pcs_rst_oper(device_id,pcs_index,AAL_SXGMII_PCS_CONTROL_OPER_RESET);
		__xfi_pcs_rst_oper(device_id,pcs_index,AAL_SXGMII_PCS_CONTROL_OPER_NORMAL);
	}
	/* delay 100ms, and then check pcs#1 link status */
	rg4d_val.wrd = CA_NE_XFI_REG_READ(XFI_RG4D + XFI_STRIDE);
	pcs_sts.wrd  = CA_NI_REG_READ(NI_HV_SXGMII_PCS_PCSSTS1 + SXGMII_STRIDE);
	ca_printf("ca-ne: HGU G3 XFI 1: pcs_sts.wrd=0x%x  clk_rdy= %d, rxidle =%d,  ber_notify= %d\n",
		pcs_sts.wrd,rg4d_val.bf.clk_rdy,rg4d_val.bf.rxidle,rg4d_val.bf.ber_notify);

	return CA_E_OK;
}

static ca_boolean_t __hgu_xfi_internal_link_get(void)
{
	aal_ni_xge_pcs_status_t pcs_link[2];
	ca_boolean_t            xfi_link[2] = { 0 };
	ca_boolean_t            sxg_link[2] = { 0 };

	memset(&pcs_link,0,sizeof(pcs_link));
	memset(&sxg_link,0,sizeof(sxg_link));
	memset(&xfi_link,0,sizeof(xfi_link));
	aal_ni_xge_pcs_status_get(CA_DEV_LAN,1,&pcs_link[0]);
	aal_ni_xge_pcs_status_get(CA_DEV_WAN,1,&pcs_link[1]);
	aal_ni_xge_sxgra_sxgsts_get(CA_DEV_LAN,1,&sxg_link[0]);
	aal_ni_xge_sxgra_sxgsts_get(CA_DEV_WAN,1,&sxg_link[1]);
	aal_xfi_link_status_get(CA_DEV_LAN,1,&xfi_link[0]);
	aal_xfi_link_status_get(CA_DEV_WAN,1,&xfi_link[1]);

	ca_printf("ca-ne: Internal XFIs link : LAN link %d pcslink %d, sxglink %d,  WAN link %d pcslink %d, sxglink %d\n",
		xfi_link[0],pcs_link[0].rxlinksts,sxg_link[0],xfi_link[1],pcs_link[1].rxlinksts,sxg_link[1]);

	return (xfi_link[0] && xfi_link[1] && pcs_link[0].rxlinksts && pcs_link[1].rxlinksts && sxg_link[0] && sxg_link[1]);

}

static ca_status_t __hgu_xfi_internal_connect_setup(void)
{
	int          retry_cnt       = 0;
	int          link_wait_times = 3;
	ca_boolean_t link_up         = 1;

	do{
		ca_printf("ca-ne: try to setup intnerla XFI connection for %u times...\n",retry_cnt + 1);
		/* init SATURN XFI lastly */
		__hgu_wan_xfi_init();
		/* init G3 XFI lastly */
		//ca_mdelay(100);
		__hgu_lan_xfi_init();
		ca_mdelay(100); /* delay 100ms and then check link status */

		if(!__xfi_chk_hw_clk_ready(CA_DEV_WAN,1)
		   || !__xfi_chk_hw_clk_ready(CA_DEV_LAN,1)) {
			continue;
		}

		/* check link status for 3 times,
		 * should always linkup, then regard as link stable
		 */
		do{
			link_up &= __hgu_xfi_internal_link_get();
		}while(--link_wait_times > 0);

		if(link_up) { /* setup successfully, return */
			ca_printf("ca-ne: Internal XFIs linkup seccuessfully.\n");
			return CA_E_OK;
		}

	}while(++retry_cnt < XFI_INTERNAL_INIT_RETRY_TIMES);

	if(retry_cnt >= XFI_INTERNAL_INIT_RETRY_TIMES) {
		ca_printf("ca-ne: !!!error: Internal XFIs link timeout!");
		return CA_E_TIMEOUT;
	}

	return CA_E_OK;
}

/** __hgu_xfi_init :
* @brief: HGU XFI intialization
*/
ca_status_t __hgu_xfi_init(void)
{
	ca_status_t  ret = CA_E_OK;

	ca_printf("ca-ne: HGU internal XFI connection setup start...\n");
	ret = __hgu_xfi_internal_connect_setup(); /* G3 XFI#1 is connected to SATURN XFI#1 */

	/* init external XFI if XFI is not 10G mode */
	if((g_xfi_drv.xfi_bmp & (0x01 << AAL_SXGMII_PCS_CONTROL_INDEX_0)) && (g_xfi_drv.xfi_speed[AAL_SXGMII_PCS_CONTROL_INDEX_0] != AAL_PHY_SPEED_10000)) {
		ca_printf("ca-ne: HGU external XFI init start...\n");
		ret |= aal_xfi_speed_set(CA_DEV_LAN,AAL_SXGMII_PCS_CONTROL_INDEX_0,g_xfi_drv.xfi_speed[AAL_SXGMII_PCS_CONTROL_INDEX_0]); /* XFI#0, port6 */
	}

	return ret;
}

#endif /* defined(CONFIG_ARCH_CORTINA_G3HGU) */

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
/** __xfi_disable_clk_generation:
 *@brief: disable XFI clock generation output
 *@note: once XFI0/1_CLKN/P clock generation is disabled, the PINs CLKN, CLKP will output nothing
 */
static void
__xfi_disable_clk_generation(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index)
{
	XFI_RG0A_t  xfi_rg0a;

	xfi_rg0a.wrd = XFI_REG_READ(device_id,pcs_index,XFI_RG0A);
	xfi_rg0a.bf.REG0_CLK_LA_EN = 0;
	XFI_REG_WRITE(device_id,pcs_index,XFI_RG0A,xfi_rg0a.wrd);
	ca_mdelay(1);
}
#endif

static inline
ca_status_t
__xfi_link_monitor(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_status_t                        ret           = CA_E_OK;
	NI_HV_SXGMII_SXGRA_SXG_INTERRUPT_t sxg_int;
	NI_HV_SXGMII_PCS_PCSSTS1_t         pcs_sts;
	ca_uint32_t                        i             = XFI_AUTONEG_TIMEOUT,
					   sxgmii_offset = pcs_index * SXGMII_STRIDE;

	do{
		sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		if(sxg_int.bf.linkup_intI) {
			CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
			ca_printf("%s: Link up interrupt detected!!!\n",__func__);
			goto ERR_XFI_LINK_MONITOR;
		}
		ca_sleep(1);
	} while(--i != 0);

	pcs_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTS1 + sxgmii_offset);
	if(!!!pcs_sts.bf.linkup) {
		ret = CA_E_TIMEOUT;
	}

ERR_XFI_LINK_MONITOR:
	if(ret == CA_E_OK) {
		ca_printf("%s: Link up detected!!!\n",__func__);
	} else {
		ca_printf("%s: Can't detect Link!!!\n",__func__);
	}

	return ret;
}

/** aal_xfi_port2xfi:
 *@brief: translate AAL port index to XFI index
 *@return: XFI index if support, otherwise, return invalid u32
 */
ca_uint32_t aal_xfi_port2xfi(ca_port_id_t aal_port)
{
	if((aal_port < XFI_MAC_PORT_START) ||
	   aal_port >= (XFI_MAC_PORT_START + XFI_NUM)) {
		return CA_UINT32_INVALID;
	}

	return (aal_port - XFI_MAC_PORT_START);
}

/** aal_xfi_speed_get :
 * @brief: Get XFI link speed result
 */
ca_status_t
aal_xfi_speed_get(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index,
	ca_uint32_t    *speed)
{
	NI_HV_SXGMII_SXGRA_PCSCTL_t       pcs_ctl;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t   hwan_ctl;
	NI_HV_SXGMII_SXGRA_SXGHWANSTS_t   hwan_sts;
	NI_HV_SXGMII_SXGRA_LINKSPEEDCTL_t speed_ctl;
	ca_uint32_t                       sxgmii_offset = pcs_index * SXGMII_STRIDE;

	AAL_XFI_IDX_CHK(pcs_index);
	CA_NULL_PTR_CHECK(speed);

	if(!__xfi_chk_hw_enable(device_id,pcs_index)) {
		return CA_E_UNAVAIL;
	}

	pcs_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
	/*
	 * 0 => XFI PCS
	 * 1 => 1000BASE-X PCS
	 */
	if(pcs_ctl.bf.pcs1gen == 0 && pcs_ctl.bf.hisgmiien == 0) {
		hwan_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);

		if(hwan_ctl.bf.hwanenable) {
			hwan_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANSTS + sxgmii_offset);
			*speed = hwan_sts.bf.hwanlinkspeed;
		} else {
			speed_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
			*speed = speed_ctl.bf.linkspeed;
		}
	} else {
		*speed = pcs_ctl.bf.pcs1gen ? AAL_PHY_SPEED_1000 : AAL_PHY_SPEED_2500;
	}

	return CA_E_OK;
}

static void __xfi_link_dump(
	ca_device_id_t device_id,
	ca_uint32_t pcs_index,
	ca_uint32_t speed)
{
	NI_HV_SXGMII_PCS_PCSSTS1_t    pcs_sts;
	NI_HV_SXGMII_SFP_PCS_STATUS_t sfp_sts;
	NI_HV_SXGMII_SXGRA_PCSCTL_t   pcs_ctl;
	ca_uint32_t                   sxgmii_offset = pcs_index * SXGMII_STRIDE;
	char                          *speed_str    =  NULL;

	pcs_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
	speed_str = aal_eth_phy_speed_str_get(device_id,(aal_phy_speed_mode_t)speed);
	if(pcs_ctl.bf.pcs1gen) { /* 1G, 2.5G link */
		sfp_sts.wrd = CA_NI_REG_READ(NI_HV_SXGMII_SFP_PCS_STATUS + sxgmii_offset);
		ca_printf("%s: dev %d PCS %d %s Link status: sfp_sts.wrd=%#x, link %s\n",
			__func__,device_id,pcs_index,speed_str,sfp_sts.wrd,sfp_sts.bf.LinkStat ? "Up" : "Down");
	} else {
		pcs_sts.wrd = CA_NI_REG_READ(NI_HV_SXGMII_PCS_PCSSTS1 + sxgmii_offset);
		ca_printf("%s: dev %d PCS %d %s Link status: pcs_sts.wrd=%#x, link %s\n",
			__func__,device_id,pcs_index,speed_str,pcs_sts.wrd,pcs_sts.bf.linkup ? "Up" : "Down");
	}

}

ca_status_t
__xfi_pcs_linkspdctl_cfg(
	ca_device_id_t dev_id,
	ca_uint32_t    pcs_idx,
	aal_xfi_speed_mode_t speed)
{
	ca_status_t                       ret           = CA_E_OK;
	NI_HV_SXGMII_SXGRA_LINKSPEEDCTL_t sxgra_spd_cfg;
	ca_uint32_t                       sxgmii_offset = pcs_idx * SXGMII_STRIDE;

	if(speed >= AAL_PHY_SPEED_MAX) {
		ret = CA_E_PARAM;
		return ret;
	}

	/* set PCS mode: TODO: need to check whether set mode before pcs reset?? */
	if(speed == AAL_PHY_SPEED_10000 || speed == AAL_PHY_SPEED_5000) {
		CA_DEV_NI_REG_WRITE(dev_id,0x80000000,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset); /* XFI PCS */
	} else if(speed <= AAL_PHY_SPEED_1000) {
		CA_DEV_NI_REG_WRITE(dev_id,0x1,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset); /* 1000BASE-X PCS */
	} else {
		/* 2.5G, 2G */
		CA_DEV_NI_REG_WRITE(dev_id,0x11,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset); /* HISGMII  PCS */
	}

	if(speed == AAL_PHY_SPEED_2000) {
		return ret;
	}

	sxgra_spd_cfg.wrd = CA_DEV_NI_REG_READ(dev_id,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
	sxgra_spd_cfg.bf.linkspeed = speed;

	sxgra_spd_cfg.bf.autoneg_complete = 0;
	CA_DEV_NI_REG_WRITE(dev_id,sxgra_spd_cfg.wrd,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
	sxgra_spd_cfg.bf.autoneg_complete = 1;
	ca_mdelay(1);

	CA_DEV_NI_REG_WRITE(dev_id,sxgra_spd_cfg.wrd,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
	ca_mdelay(1);

	return ret;
}

/** aal_xfi_speed_set :
 * @brief: Set XFI force link speed based on pcs_index
 * @note: this function will take about 100 miliseconds delay.
 */
ca_status_t
aal_xfi_speed_set(
	ca_device_id_t       device_id,
	ca_uint32_t    	     pcs_index,
	aal_xfi_speed_mode_t speed)
{
	typedef void (*prof_func)(ca_device_id_t, aal_phy_sxgmii_pcs_control_index_t);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	prof_func                     prof_func_list[] = {
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
				    NULL,
				    NULL,
		    #else
				    __xfi_cfg_init_1g,
				    __xfi_cfg_init_1g,
		    #endif
				    __xfi_cfg_init_1g,
				    __xfi_cfg_init_xg,
		    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
				    NULL,
		    #else
				    __xfi_cfg_init_2g,
		    #endif
				    __xfi_cfg_init_2d5g,
		    #if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
				    NULL,
		    #else
				    __xfi_cfg_init_xg,
		    #endif
				    NULL
			    };
#endif
	prof_func                     run_prof_func;
	ca_uint32_t                   xfi_offset     = 0,
				      sxgmii_offset  = 0,
				      xge_offset     = 0;
	NI_HV_SXGMII_PCS_PCSSTS1_t    pcs_sts;
	NI_HV_SXGMII_SFP_PCS_STATUS_t sfp_sts;
	NI_HV_SXGMII_SXGRA_PCSCTL_t   pcs_ctl;
	int                           i              = 0;
	const char                    *speed_str[]     = { "10M", "100M", "1G", "10G", "2G", "2.5G", "5G" };

	AAL_XFI_IDX_CHK(pcs_index);
	AAL_XFI_SPEED_CHK(speed);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	run_prof_func = __xfi_cfg_init_xg_g3;
#else
	run_prof_func = prof_func_list[speed];
#endif
	/* 
	** update speed configuration to local global variable 
	*/
	if(CA_E_OK != __xfi_speed_update(device_id,pcs_index,speed)) {
		ca_printf("%s(): !error: __xfi_speed_update return fail!\r\n",__func__);
		return CA_E_PARAM;
	}

	xfi_offset    = pcs_index * XFI_STRIDE;
	sxgmii_offset = pcs_index * SXGMII_STRIDE;
	xge_offset    = pcs_index * XGE_MISC_STRIDE;
	/* make sure the XFI is released for S/W to configure */
	__xfi_pcs_rst_oper(device_id,pcs_index,AAL_SXGMII_PCS_CONTROL_OPER_NORMAL);
	__xge_mac_reset(device_id,pcs_index,AAL_XGE_MAC_CFG_RESET);
	__xfi_rx_en_cfg(device_id,pcs_index,FALSE);

	__sxgmii_init(device_id,pcs_index,speed);

	if(run_prof_func) {
		run_prof_func(device_id,pcs_index);
	}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if(speed != AAL_PHY_SPEED_10000) {
		CA_DEV_XFI_REG_WRITE(device_id,0x000020F0,XFI_RG00 + xfi_offset);
		ca_mdelay(1);
		CA_DEV_XFI_REG_WRITE(device_id,0x00000060,XFI_RG0D + xfi_offset);
		ca_mdelay(1);
		CA_DEV_XFI_REG_WRITE(device_id,0x00002682,XFI_RG33 + xfi_offset);
		ca_mdelay(1);
		CA_DEV_XFI_REG_WRITE(device_id,0x00005010,XFI_RG38 + xfi_offset);
		ca_mdelay(1);
		CA_DEV_XFI_REG_WRITE(device_id,0x00005010,XFI_RG39 + xfi_offset);
	}
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	__xfi_leq_en_toggle(device_id,pcs_index);
#endif

	__xfi_rx_en_cfg(device_id,pcs_index,TRUE);
	//
	// reset PCS
	//
	__xfi_pcs_rst_oper(device_id,pcs_index,AAL_SXGMII_PCS_CONTROL_OPER_RESET);
	__xfi_pcs_rst_oper(device_id,pcs_index,AAL_SXGMII_PCS_CONTROL_OPER_NORMAL);

	__xfi_pcs_linkspdctl_cfg(device_id,pcs_index,speed);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	__xfi_disable_clk_generation(device_id,pcs_index);
#endif
	//
	// Release reset of xge mac
	//
	__xge_mac_reset(device_id,pcs_index,AAL_XGE_MAC_CFG_NORMAL);
	//
	// check PCS link status
	//
	i = 30;
	pcs_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_PCSCTL + sxgmii_offset);
	do{
		ca_mdelay(1);
		/* check PCS link status */
		if(pcs_ctl.bf.pcs1gen) {
			/* 1G, 2.5G link */
			sfp_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SFP_PCS_STATUS + sxgmii_offset);
			if(sfp_sts.bf.LinkStat) {
				ca_printf(
					"%s: dev %d PCS %d %s Link status: sfp_sts.wrd=%#x, wait_tim = %ums\n",
					__func__,
					device_id,
					pcs_index,
					speed_str[speed],
					sfp_sts.wrd,
					(30 - i));
				break;
			}
			continue;
		}

		/* check pcs link status */
		pcs_sts.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_PCS_PCSSTS1 + sxgmii_offset);
		if(pcs_sts.bf.linkup) {
			ca_printf(
				"%s: dev %d PCS %d %s Link status: pcs_sts.wrd=%#x, wait_tim = %ums\n",
				__func__,
				device_id,
				pcs_index,
				speed_str[speed],
				pcs_sts.wrd,
				(30 - i));
			break;
		}
	} while(--i != 0);

	return CA_E_OK;
}

/** aal_xfi_do_hw_autoneg :
 * @brief: Hardware autoneg implement
 */
static ca_status_t
__usxgmii_do_hw_an(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_uint32_t                        i,
					   sxgmii_offset;
	NI_HV_XGE_MAC_CFG_COM_t            mac_cfg_com;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t    sxg_hw_an_ctl;
	NI_HV_SXGMII_SXGRA_SXGMISCCTL_t    sxg_misc_ctl;
	NI_HV_SXGMII_SXGRA_LINKSPEEDCTL_t  link_speed;
	NI_HV_SXGMII_SXGRA_SXG_INTERRUPT_t sxg_int;
	aal_ni_xge_cmn_cfg_t               xge_cmn_cfg;
	aal_ni_xge_cmn_cfg_mask_t          xge_cmn_msk;
	ca_status_t                        ret           = CA_E_OK;

	memset(&xge_cmn_msk,0x00,sizeof( aal_ni_xge_cmn_cfg_mask_t ));
	xge_cmn_msk.s.tx_rst = 1;
	xge_cmn_msk.s.rx_rst = 1;

	/* set up XFI to 10G first */
	aal_xfi_speed_set(device_id,pcs_index,AAL_PHY_SPEED_10000);

	ret = __xfi_link_monitor(device_id,pcs_index);
	if(ret != CA_E_OK) {
		return ret;
	}

	//ca_printf("%s: Check Auto nego. request..\n", __func__);
	i = XFI_AUTONEG_TIMEOUT;
	do{
		sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		if(!sxg_int.bf.ancfgwddet_intI) {
			ca_mdelay(1);
			continue;
		}

		CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		ca_printf("%s: Received Auto nego. request!!!\n",__func__);
		break;
	} while(--i != 0);

	if(i == 0) {
		ca_printf("%s: There is no Auto nego. request received!!!\n",__func__);
	}

	/* disable data path */
	xge_cmn_cfg.tx_rst   = 1;
	xge_cmn_cfg.rx_rst   = 1;
	aal_ni_xge_cmn_config_set(device_id,pcs_index,xge_cmn_msk,&xge_cmn_cfg);

	ca_printf("%s: Do HW Auto nego...\n",__func__);

	sxg_hw_an_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);
	sxg_hw_an_ctl.bf.linktmr    = 0x3D090;
	sxg_hw_an_ctl.bf.hwanstart  = 1;
	sxg_hw_an_ctl.bf.hwanenable = AAL_USXGMII_AN_ENABLE;
	CA_DEV_NI_REG_WRITE(device_id,sxg_hw_an_ctl.wrd,NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);

	i = XFI_AUTONEG_TIMEOUT;
	do{
		sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		if(!sxg_int.bf.hwancomplete_intI) {
			ca_mdelay(10);
			continue;
		}

		CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		ca_udelay(100);
		sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		//ca_printf("%s: after write sxg_int.wrd=0x%x\n", __func__, sxg_int.wrd);

		/* keep HW auto nego. enabled always */
		//sxg_hw_an_ctl.wrd = CA_DEV_NI_REG_READ(device_id, NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);
		//sxg_hw_an_ctl.bf.hwanenable = 0;
		//CA_DEV_NI_REG_WRITE(device_id, sxg_hw_an_ctl.wrd, NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);

		/* read link speed */
		link_speed.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
		ca_printf("%s: auto neg complete=%d, link speed=%d\n",__func__,
			link_speed.bf.autoneg_complete,link_speed.bf.linkspeed);

		/* recenter the USXGMII elastic buffers */
		sxg_misc_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGMISCCTL + sxgmii_offset);
		sxg_misc_ctl.bf.txgmfreinit = 1;
		sxg_misc_ctl.bf.txxgmfreinit = 1;
		sxg_misc_ctl.bf.rxgmfreinit = 1;
		sxg_misc_ctl.bf.rxxgmfreinit = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_misc_ctl.wrd,NI_HV_SXGMII_SXGRA_SXGMISCCTL + sxgmii_offset);
		ca_udelay(100);

		if(link_speed.bf.linkspeed <= AAL_PHY_SPEED_1000) {
			NI_HV_XGE_MISC_CONFIG_t  xge_misc;
			xge_misc.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MISC_CONFIG + XGE_MISC_STRIDE * pcs_index);
			xge_misc.bf.gige_mode = 1;
			CA_DEV_NI_REG_WRITE(device_id,xge_misc.wrd,NI_HV_XGE_MISC_CONFIG + XGE_MISC_STRIDE * pcs_index);
			ca_printf("%s: Sync PHY speed %d to MAC!\n",__func__,link_speed.bf.linkspeed);
		}

		goto __restore_datapath;
	} while(--i != 0);

	ca_printf("%s: HW Auto nego. no completed!!!\n",__func__);
	ret = CA_E_ERROR;

__restore_datapath:
	/* enable data path */
	xge_cmn_cfg.tx_rst   = 0;
	xge_cmn_cfg.rx_rst   = 0;
	aal_ni_xge_cmn_config_set(device_id,pcs_index,xge_cmn_msk,&xge_cmn_cfg);

	return ret;
}

/** __usxgmii_do_sw_an:
 * @brief: usxgmii software autoneg implement for mvl88x3310 10g PHY
 */
#define USXGMII_LINK_TIMER     (1600) //1.6ms = 1600us
#define CPU_NEED_INT_ENABLE    (1)
static ca_status_t
__usxgmii_do_sw_an(
	ca_device_id_t device_id,
	aal_phy_sxgmii_pcs_control_index_t pcs_index)
{
	ca_status_t                        ret                  = CA_E_OK;
	NI_HV_XGE_MAC_CFG_COM_t            mac_cfg_com;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t    sxg_hw_an_ctl;
	NI_HV_SXGMII_SXGRA_SXGRST_t        sxg_rst;
	NI_HV_SXGMII_SXGRA_SXGMISCCTL_t    sxg_misc_ctl;
	NI_HV_SXGMII_SXGRA_LINKSPEEDCTL_t  link_speed;
	NI_HV_SXGMII_SXGRA_SXG_INTENABLE_t sxg_int_en;
	NI_HV_SXGMII_SXGRA_ANTXCTL_t       sxg_an_txctl;
	NI_HV_SXGMII_SXGRA_ANRXCTL_t       sxg_an_rxctl;
	NI_HV_SXGMII_SXGRA_ANRXCFGWDAB_t   sxg_an_rxcfgwdab;
	NI_HV_SXGMII_SXGRA_ANRXCFGWD_t     sxg_an_rxcfgwd;
	NI_HV_SXGMII_SXGRA_ANRXCFGWDAB_t   sxg_an_rxcfgwdab_ack;
	NI_HV_SXGMII_SXGRA_ANMISCCTL_t     sxg_an_miscctl;
	NI_HV_SXGMII_SXGRA_SXG_INTERRUPT_t sxg_int;
	aal_ni_xge_cmn_cfg_t               xge_cmn_cfg;
	aal_ni_xge_cmn_cfg_mask_t          xge_cmn_msk;
	ca_uint32_t                        speed                = 0;
	ca_uint32_t                        i,
					   sxgmii_offset,
					   retry                = 6;

	memset(&xge_cmn_msk,0x00,sizeof( aal_ni_xge_cmn_cfg_mask_t ));
	xge_cmn_msk.s.tx_rst = 1;
	xge_cmn_msk.s.rx_rst = 1;

	sxgmii_offset = pcs_index * SXGMII_STRIDE;
	/* set up XFI to 10G first */
	aal_xfi_speed_set(device_id,pcs_index,AAL_PHY_SPEED_10000);

	ret = __xfi_link_monitor(device_id,pcs_index);
	if(ret != CA_E_OK) {
		return ret;
	}

	/* disable data path */
	xge_cmn_cfg.tx_rst   = 1;
	xge_cmn_cfg.rx_rst   = 1;
	aal_ni_xge_cmn_config_set(device_id,pcs_index,xge_cmn_msk,&xge_cmn_cfg);

	ca_printf("%s: Do SW Auto nego...\n",__func__);

	do{
		/* Restart mv88x3310 PHY USXGMII Auto-Nego */
		ret = aal_mdio_write(device_id,0,16,4,0xF0A0,0x168);
		//ca_printf("%s: Restart and Disable mvl88x3310 USXGMII Auto-Nego Mode!!!\n", __func__);
		mdelay(100);
#ifdef DEBUG
		ca_uint16_t phy_usxgmii_an_ctl = 0x00;
		ret = aal_mdio_read(device_id,0,16,4,0xF0A0,&phy_usxgmii_an_ctl);
		ca_printf("%s: mvl88x3310 4.F0A0 = 0x%x\n",__func__,phy_usxgmii_an_ctl);
		ca_uint16_t mac_speed = 0x00;
		ret = aal_mdio_read(device_id,0,16,4,0xF0A2,&mac_speed);
		ca_printf("%s: mvl88x3310 4.F0A2 = 0x%x\n",__func__,mac_speed);
#endif
		ca_uint16_t mvl_an_base_page = 0x00;
		/* Read mvl88x3310 usxgmii speed register */
		ret = aal_mdio_read(device_id,0,16,4,0xF0A6,&mvl_an_base_page);
		//ca_printf("%s: mvl88x3310 4.F0A6 = 0x%x\n", __func__, mvl_an_base_page);

		/* Ref : Figure 37-6 of IEEE Std. 802.3 - AN_ENABLE state */
		sxg_hw_an_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);
		sxg_hw_an_ctl.bf.hwanenable = 0;
		sxg_hw_an_ctl.bf.hwancopyrcvdspd = 0;
		CA_DEV_NI_REG_WRITE(device_id,sxg_hw_an_ctl.wrd,NI_HV_SXGMII_SXGRA_SXGHWANCTL + sxgmii_offset);
		sxg_an_txctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		sxg_an_txctl.bf.txcfgwden = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		//ca_printf("%s: <AN_RESTART>: CA8271 transmits breaklink (0x9c000003).\n", __func__);

		/* Ref : Figure 37-6 of IEEE Std. 802.3 - AN_RESTART state */
		/* CA8271 transmits breaklink for linker_timer = 1.6 ms */
		ca_udelay(USXGMII_LINK_TIMER);

		sxg_an_txctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		sxg_an_txctl.bf.txcfgwden = 0;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		//ca_printf("%s: <AN_RESTART>: CA8271 stop transmits breaklink.(0x9c000003).\n", __func__);
		if(mvl_an_base_page != 0 && mvl_an_base_page != 0xffff) { /* mvl88x3310 usxgmii AN path */
			speed = (mvl_an_base_page >> 9) & 0x7;
			break;
		}
	} while(--retry != 0);
// TBD
#if 0
	else {
		// TBD
		goto AN_RESTART; /* work-around for mvl88x3310 */
		/* AQR107 usxgmii AN path */
		/* Ref : Figure 37-6 of IEEE Std. 802.3 - ABILITY_DETECT state */
#if CPU_NEED_INT_ENABLE
		/* Enable ancfgwdabdet Interrupt */
		sxg_int_en.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
		sxg_int_en.bf.ancfgwdabdet_intE = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_int_en.wrd,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
#endif
		/* Need to clear "sxg_int.bf.xxxI" here before the following logic */
		CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);

		sxg_an_rxctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);
		sxg_an_rxctl.bf.rxcfgwdabdeten = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_rxctl.wrd,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);
		for(i = 0;
		    i < XFI_AUTONEG_TIMEOUT;
		    i++){
			sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
			if ( sxg_int.bf.ancfgwdabdet_intI ) {
				CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
				//ca_printf("%s: <ABLTY_DETECT>: CA8271 detected Link Partner Valid Ability_Match Word.\n", __func__);
				sxg_an_rxcfgwdab.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCFGWDAB + sxgmii_offset);
				//ca_printf("%s: <ABLTY_DETECT>: CA8271 sxg_an_rxcfgwdab.wrd = 0x%x...\n", __func__, sxg_an_rxcfgwdab.wrd);
				break;
			}
			ca_udelay(10);
		}
		if ( i == XFI_AUTONEG_TIMEOUT ) {
			//ca_printf("%s: <ABLTY_DETECT>: CA8271 detect of Link Partner Ability_Match timeout 300 us!!!\n", __func__);
			ret = CA_E_ERROR;
		}
		sxg_an_txctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		sxg_an_txctl.bf.txidlecfgwden = 0;
		sxg_an_txctl.bf.txcfgwd2en = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		//ca_printf("%s: <ABLTY_DETECT>: CA8271 transmits own abilities.(0x9c960103).\n", __func__);

		/* Ref : Figure 37-6 of IEEE Std. 802.3 - ACKNOWLEDGE_DETECT state */
#if CPU_NEED_INT_ENABLE
		/* Enable ancfgwddet Interrupt */
		sxg_int_en.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
		sxg_int_en.bf.ancfgwdabdet_intE = 0;
		sxg_int_en.bf.ancfgwddet_intE = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_int_en.wrd,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
#endif
		sxg_an_rxctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);
		sxg_an_rxctl.bf.rxcfgwdabdeten = 0;
		sxg_an_rxctl.bf.rxcfgwddeten = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_rxctl.wrd,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);
		for(i = 0;
		    i < XFI_AUTONEG_TIMEOUT;
		    i++){
			sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
			if ( sxg_int.bf.ancfgwddet_intI ) {
				CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
				//ca_printf("%s: <ACK_DETECT>: CA8271 detected Link Partner Valid Ability Acknowledge_Match Word.\n", __func__);
				sxg_an_rxcfgwd.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCFGWD + sxgmii_offset);
				//ca_printf("%s: <ACK_DETECT>: CA8271 sxg_an_rxcfgwd.wrd = 0x%x...\n", __func__, sxg_an_rxcfgwd.wrd);
				break;
			}
			ca_udelay(10);
		}
		if ( i == XFI_AUTONEG_TIMEOUT ) {
			//ca_printf("%s: <ACK_DETECT>: CA8271 detect of Link Partner Acknowledge_Match timeout 300 us!!!\n", __func__);
			ret = CA_E_ERROR;
		}
		sxg_an_txctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		sxg_an_txctl.bf.txcfgwd2en = 0;
		sxg_an_txctl.bf.txcfgwd3en = 1;
		CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
		//ca_printf("%s: <ACK_DETECT>: CA8271 transmits abilities with ACK bit.(0x9cd60103).\n", __func__);
		sxg_an_rxcfgwdab_ack.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCFGWDAB + sxgmii_offset);
		//ca_printf("%s: <ACK_DETECT>: CA8271 sxg_an_rxcfgwdab_ack.wrd = 0x%x...\n", __func__, sxg_an_rxcfgwdab_ack.wrd);

		/* Ref : Figure 37-6 of IEEE Std. 802.3 - COMPLETE_ACKNOWLEDGE state */
		if ( sxg_an_rxcfgwdab_ack.wrd == sxg_an_rxcfgwdab.wrd ) {
			ca_udelay(USXGMII_LINK_TIMER);
			speed = (sxg_an_rxcfgwdab.wrd >> 9) & 0x7;
		}
	}
#endif

	/* Ref : Figure 37-6 of IEEE Std. 802.3 - IDLE_DETECT state */
	sxg_an_txctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
	sxg_an_txctl.bf.txidlecfgwden = 1;
	sxg_an_txctl.bf.txcfgwd3en = 0;
	sxg_an_txctl.bf.txcfgwd2en = 0;
	sxg_an_txctl.bf.txcfgwden = 0;
	CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
	//ca_printf("%s: <IDLE_DETECT>: CA8271 transmits IDLEs.(0x07070707).\n", __func__);
#if CPU_NEED_INT_ENABLE
	/* Enable anidledett Interrupt */
	sxg_int_en.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
	sxg_int_en.bf.ancfgwdabdet_intE = 0;
	sxg_int_en.bf.ancfgwddet_intE = 0;
	sxg_int_en.bf.anidledet_intE = 1;
	CA_DEV_NI_REG_WRITE(device_id,sxg_int_en.wrd,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
#endif
	sxg_an_rxctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);
	sxg_an_rxctl.bf.rxcfgwdabdeten = 0;
	sxg_an_rxctl.bf.rxcfgwddeten = 0;
	sxg_an_rxctl.bf.rxidledeten = 1;
	CA_DEV_NI_REG_WRITE(device_id,sxg_an_rxctl.wrd,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);

	i = XFI_AUTONEG_TIMEOUT;
	do{
		sxg_int.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		if(!sxg_int.bf.anidledet_intI) {
			ca_udelay(10);
			continue;
		}
		CA_DEV_NI_REG_WRITE(device_id,sxg_int.wrd,NI_HV_SXGMII_SXGRA_SXG_INTERRUPT + sxgmii_offset);
		ca_printf("%s: <IDLE_DETECT>: CA8271 detected 3 consecutive IDLE words.\n",__func__);

		/* Programs the link speed and negotiation complete fields */
		link_speed.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
		link_speed.bf.linkspeed = speed;
		link_speed.bf.autoneg_complete = 0;
		CA_DEV_NI_REG_WRITE(device_id,link_speed.wrd,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
		link_speed.bf.autoneg_complete = 1;
		CA_DEV_NI_REG_WRITE(device_id,link_speed.wrd,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
		break;
	} while(--i != 0);

	if(i == 0) {
		ca_printf("%s: <IDLE_DETECT>: CA8271 detect Idle timeout 300 us!!!\n",__func__);
		ret = CA_E_ERROR;
	}

	sxg_an_txctl.bf.txidlecfgwden = 0;
	CA_DEV_NI_REG_WRITE(device_id,sxg_an_txctl.wrd,NI_HV_SXGMII_SXGRA_ANTXCTL + sxgmii_offset);
#if CPU_NEED_INT_ENABLE
	sxg_int_en.bf.anidledet_intE = 0;
	CA_DEV_NI_REG_WRITE(device_id,sxg_int_en.wrd,NI_HV_SXGMII_SXGRA_SXG_INTENABLE + sxgmii_offset);
#endif
	sxg_an_rxctl.bf.rxidledeten = 0;
	CA_DEV_NI_REG_WRITE(device_id,sxg_an_rxctl.wrd,NI_HV_SXGMII_SXGRA_ANRXCTL + sxgmii_offset);

	/* read link speed */
	link_speed.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_LINKSPEEDCTL + sxgmii_offset);
	ca_printf("%s: <LINK_OK>: CA8271 auto neg complete=%d, link speed=%d\n",__func__,
		link_speed.bf.autoneg_complete,link_speed.bf.linkspeed);

	/* Ref : Figure 37-6 of IEEE Std. 802.3 - LINK_OK state */
	/* recenter the USXGMII elastic buffers */
	sxg_misc_ctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGMISCCTL + sxgmii_offset);
	sxg_misc_ctl.bf.txgmfreinit = 1;
	sxg_misc_ctl.bf.txxgmfreinit = 1;
	sxg_misc_ctl.bf.rxgmfreinit = 1;
	sxg_misc_ctl.bf.rxxgmfreinit = 1;
	CA_DEV_NI_REG_WRITE(device_id,sxg_misc_ctl.wrd,NI_HV_SXGMII_SXGRA_SXGMISCCTL + sxgmii_offset);
	ca_udelay(100);

	if(link_speed.bf.linkspeed <= AAL_PHY_SPEED_1000) {
		NI_HV_XGE_MISC_CONFIG_t  xge_misc;
		xge_misc.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_XGE_MISC_CONFIG + XGE_MISC_STRIDE * pcs_index);
		xge_misc.bf.gige_mode = 1;
		CA_DEV_NI_REG_WRITE(device_id,xge_misc.wrd,NI_HV_XGE_MISC_CONFIG + XGE_MISC_STRIDE * pcs_index);
		ca_printf("%s: Sync PHY speed %d to MAC!\n",__func__,link_speed.bf.linkspeed);
	}

	if(ret == CA_E_OK) {
		ca_printf("%s: Do SW Auto nego has completed!!!\n",__func__);
	} else {
		ca_printf("%s: Do SW Auto nego no completed!!!\n",__func__);
		ret = CA_E_ERROR;
	}

__relese_datapath:
	/* enable data path */
	xge_cmn_cfg.tx_rst   = 0;
	xge_cmn_cfg.rx_rst   = 0;
	aal_ni_xge_cmn_config_set(device_id,pcs_index,xge_cmn_msk,&xge_cmn_cfg);

	return ret;
}

/**____usxgmii_do_an
 *@breif: run the H/W autoneg progress
 *@note: the progress may use about 100ms ~300ms
 */
static void __usxgmii_do_an(void *xfi_id)
{
	typedef ca_status_t (*usxgmii_an_func)(ca_device_id_t, aal_phy_sxgmii_pcs_control_index_t);
	ca_uint32_t     pcs_index;
	usxgmii_an_func run_an;
	usxgmii_an_func an_func_list[] = {
		__usxgmii_do_hw_an,
		__usxgmii_do_sw_an,
		NULL
	};

	if(!xfi_id) {
		goto ERROR_USXGMII_DO_AN;
	}

	pcs_index = *((ca_uint32_t*)xfi_id);
	run_an = an_func_list[g_xfi_drv.usxgmii_an_tp[pcs_index]];

	atomic_set(&g_xfi_drv.usxgmii_an_running[pcs_index],1);
	if(run_an) {
		run_an(0,pcs_index);
	}
	atomic_set(&g_xfi_drv.usxgmii_an_running[pcs_index],0);

ERROR_USXGMII_DO_AN:
	kfree(xfi_id);
	xfi_id = NULL;
}

/**aal_xfi_usxgmii_an_tp_get
*@breif: get an type of usxgmii 
*/
ca_status_t aal_xfi_usxgmii_an_tp_get(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index,
	ca_uint32_t    *an_tp)
{
	ca_status_t ret = CA_E_OK;

	CA_NULL_PTR_CHECK(an_tp);
	AAL_XFI_IDX_CHK(pcs_index);

	*an_tp = g_xfi_drv.usxgmii_an_tp[pcs_index];

ERROR_AAL_XFI_USXGMII_AN_TP_GET:
	return ret;
}

/**aal_xfi_usxgmii_an_tp_set 
*@breif: set an type of usxgmii 
*/
ca_status_t aal_xfi_usxgmii_an_tp_set(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index,
	ca_uint32_t    an_tp)
{
	ca_status_t ret = CA_E_OK;

	AAL_XFI_IDX_CHK(pcs_index);

	if(an_tp >= MAX_AAL_USXGMII_AN_TP) {
		ret = CA_E_ERROR;
		goto ERROR_AAL_XFI_USXGMII_AN_TP_SET;
	}

	g_xfi_drv.usxgmii_an_tp[pcs_index] = an_tp;

ERROR_AAL_XFI_USXGMII_AN_TP_SET:
	return ret;
}

/**aal_xfi_autoneg_get
*@breif: get the H/W autoneg state
*/
ca_status_t aal_xfi_autoneg_get(
	ca_device_id_t device_id,
	ca_uint32_t pcs_index,
	ca_boolean_t *autoneg_enable)
{
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t  sxghwanctl;

	CA_NULL_PTR_CHECK(autoneg_enable);
	AAL_XFI_IDX_CHK(pcs_index);

	sxghwanctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + pcs_index * SXGMII_STRIDE);

	*autoneg_enable = sxghwanctl.bf.hwanenable;

	return CA_E_OK;
}


/**aal_xfi_autoneg_set
 *@breif: enable/disable XFI port autoneg
 *@note: the progress will start a 10ms interval timer, and timer function will run only once
 */
ca_status_t aal_xfi_autoneg_set(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index,
	ca_boolean_t   autoneg_enable)
{
	ca_status_t                     ret        = CA_E_OK;
	NI_HV_SXGMII_SXGRA_SXGHWANCTL_t sxghwanctl;
	ca_uint32_t                     *pcs_idx   = NULL;

	AAL_XFI_IDX_CHK(pcs_index);


	if(g_xfi_drv.xfi_intf_type[pcs_index] != AAL_NI_IF_MODE_USXGMII) {
		ca_printf("Warning!!! port %u isnt USXGMII and can't do AN\n",pcs_index + XFI_MAC_PORT_START);
		goto ERROR_AAL_XFI_AUTONEG_SET;
	}

	sxghwanctl.wrd = CA_DEV_NI_REG_READ(device_id,NI_HV_SXGMII_SXGRA_SXGHWANCTL + pcs_index * SXGMII_STRIDE);

	if(AAL_USXGMII_AN_DISABLE == autoneg_enable) {
		sxghwanctl.bf.hwanenable = autoneg_enable;
		CA_DEV_NI_REG_WRITE(device_id,sxghwanctl.wrd,NI_HV_SXGMII_SXGRA_SXGHWANCTL + pcs_index * SXGMII_STRIDE);
		goto ERROR_AAL_XFI_AUTONEG_SET;
	}

	if(!!atomic_read(&g_xfi_drv.usxgmii_an_running[pcs_index])) {
		ret = CA_E_BUSY;
		goto ERROR_AAL_XFI_AUTONEG_SET;
	}

	pcs_idx = kmalloc(sizeof( ca_uint32_t ),GFP_KERNEL);
	if(!pcs_idx) {
		ret = CA_E_RESOURCE;
		goto ERROR_AAL_XFI_AUTONEG_SET;
	}

	*pcs_idx = pcs_index;
	/* start the autneg progress */
	ca_timer_add(30,__usxgmii_do_an,(void*)pcs_idx);

ERROR_AAL_XFI_AUTONEG_SET:
	return ret;
}

/**aal_xfi_autoneg_restart
 *@breif: restart XFI port autoneg
 *@note: the progress will run with an 10ms interval timer
 */
ca_status_t aal_xfi_autoneg_restart(
	ca_device_id_t device_id,
	ca_uint32_t    pcs_index)
{
	ca_status_t ret      = CA_E_ERROR;
	ca_uint32_t timer_id = 0;
	ca_uint32_t *pcs_idx = NULL;

	AAL_XFI_IDX_CHK(pcs_index);

	if(g_xfi_drv.xfi_intf_type[pcs_index] != AAL_NI_IF_MODE_USXGMII) {
		ca_printf("Warning!!! port %u isnt USXGMII and can't do AN\n",pcs_index + XFI_MAC_PORT_START);
		ret = CA_E_OK;
		goto ERROR_AAL_XFI_AUTONEG_RESTART;
	}

	if(!!atomic_read(&g_xfi_drv.usxgmii_an_running[pcs_index])) {
		ret = CA_E_OK;
		goto ERROR_AAL_XFI_AUTONEG_RESTART;
	}

	pcs_idx = kmalloc(sizeof( ca_uint32_t ),GFP_KERNEL);
	if(!pcs_idx) {
		ret = CA_E_RESOURCE;
		goto ERROR_AAL_XFI_AUTONEG_RESTART;
	}

	*pcs_idx = pcs_index;

	/* start the autneg progress */
	timer_id = ca_timer_add(10,__usxgmii_do_an,(void*)pcs_idx);
	if(timer_id == 0) {
		ca_printf("%s(): ca_timer_add fail for autneg restart",__func__);
		ret = CA_E_BUSY;
	}

ERROR_AAL_XFI_AUTONEG_RESTART:
	return CA_E_OK;
}


/** __xfi_scfg_load :
* @brief: XFI startup configuration loading
*/
static ca_status_t __xfi_scfg_load(ca_device_id_t  device_id)
{
	ca_uint8_t         intf_type[CFG_ID_PHY_INTERFACE_LEN + 1]     = { 0 };
	ca_uint8_t         intf_phy_addr[CFG_ID_PHY_INTERFACE_LEN + 1] = { 0 };
	struct ca_soc_data soc_data;
	ca_uint32_t        xfi           = 0;
	ca_uint32_t        xe_start      = XFI_MAC_PORT_START;
	ca_uint32_t        bmp           = 0;
	ca_status_t        ret           = CA_E_OK;
	char               *intf_str[]     = {
		"GMII",
				  "MII",
				  "RGMII",
				  "RGMII",
				  "TBI",
				  "RMII",
				  "NBT",
				  "SSSMII",
				  "SXGMII",
				  "SGMII",
				  "HISGMII",
				  "RXAUI",
				  "USXGMII"
			  };
#ifdef CONFIG_ARCH_CORTINA_G3HGU
	ca_uint32_t        xfi_rg32_cfg[4]  = { 0 };

	ret = scfg_read(device_id,CFG_ID_XFI_RG32_ADJUST,CFG_ID_XFI_RG32_ADJUST_LEN,(ca_uint8_t*)xfi_rg32_cfg);
	if(ret == CA_E_OK) {
		memcpy(g_xfi_rg32_param,xfi_rg32_cfg,sizeof(ca_uint32_t) * 4);
		ca_printf("ca-ne: load internal XFI RG32 scfg: %#x %#x %#x %#x ...\n",
			g_xfi_rg32_param[0],g_xfi_rg32_param[1],g_xfi_rg32_param[2],g_xfi_rg32_param[3]);
	}
#endif

	memset(&soc_data,0,sizeof(soc_data));
	ca_soc_data_get(&soc_data);
	g_xfi_drv.chip_version = soc_data.chip_revision;

	ca_printf("ca-ne: load XFI ports scfg on chip (verndor-id: %#x, version : %#x (%c)...\n",
		soc_data.vendor_id,soc_data.chip_revision,soc_data.chip_revision);

	ret = scfg_read(device_id,CFG_ID_XGE_PORT_BIT_MAP,sizeof(ca_uint32_t),(ca_uint8_t*)&g_xfi_drv.xfi_bmp);
	ret |= scfg_read(device_id,CFG_ID_GE_PORT_BIT_MAP,sizeof(ca_uint32_t),(ca_uint8_t*)&bmp);
	if(CA_E_OK != ret) {
		g_xfi_drv.xfi_bmp = XFI_BITMAP_DEFAULT; /* default support two XFIs */
		ca_printf("ca-ne: load XFI scfg %s fail, error %d\r\n",CFG_ID_XGE_PORT_BIT_MAP,ret);
	} else {
		g_xfi_drv.xfi_bmp |= bmp;
		g_xfi_drv.xfi_bmp >>= XFI_MAC_PORT_START; /* translate to XFI bitmap */
		g_xfi_drv.xfi_bmp &= XFI_BITMAP_ALL;
		ca_printf("ca-ne: load scfg %s OK, bitmap=%#x\r\n",CFG_ID_XGE_PORT_BIT_MAP,g_xfi_drv.xfi_bmp);
	}

	ret = scfg_read(device_id,CFG_ID_PHY_INTERFACE,CFG_ID_PHY_INTERFACE_LEN,intf_type);
	if(CA_E_OK != ret) {
		ca_printf("ca-ne: XFI scfg load %s fail, error %d\r\n",CFG_ID_PHY_INTERFACE,ret);
		FOR_ALL_XFI_DO(xfi) {
			g_xfi_drv.xfi_speed[xfi] = AAL_PHY_SPEED_10000;
		}
	} else {
		FOR_ALL_XFI_DO(xfi) {
			g_xfi_drv.xfi_intf_type[xfi] = intf_type[xe_start + xfi];
			if((0x01 << xfi) & g_xfi_drv.xfi_bmp) {
				ca_printf("ca-ne: load %s OK, XFI %d is connected to %s(%d)\r\n",
					CFG_ID_PHY_INTERFACE,xfi,intf_str[intf_type[xe_start + xfi]],intf_type[xe_start + xfi]);
			}
			if(intf_type[xe_start + xfi] == AAL_NI_IF_MODE_SXGMII
			   || intf_type[xe_start + xfi] == AAL_NI_IF_MODE_USXGMII) {
				g_xfi_drv.xfi_speed[xfi] = AAL_PHY_SPEED_10000;
			} else if(intf_type[xe_start + xfi] == AAL_NI_IF_MODE_XFI) {
				g_xfi_drv.xfi_speed[xfi] = AAL_PHY_SPEED_10000;
			} else if(intf_type[xe_start + xfi] == AAL_NI_IF_MODE_HISGMII) {
				g_xfi_drv.xfi_speed[xfi] = AAL_PHY_SPEED_2500;
			} else {
				g_xfi_drv.xfi_speed[xfi] = AAL_PHY_SPEED_1000;
			}
		}
	}
	ret = scfg_read(device_id,CFG_ID_PHY_BASE_ADDR,CFG_ID_PHY_INTERFACE_LEN,intf_phy_addr);
	if(CA_E_OK != ret) {
		ca_printf("ca-ne: XFI scfg load %s fail, error %d\r\n",CFG_ID_PHY_BASE_ADDR,ret);
		FOR_ALL_XFI_DO(xfi) {
			g_xfi_drv.xfi_phy_addr[xfi] = 0x10;
		}
	} else {
		FOR_ALL_XFI_DO(xfi) {
			g_xfi_drv.xfi_phy_addr[xfi] = intf_phy_addr[xe_start + xfi];
			if((0x01 << xfi) & g_xfi_drv.xfi_bmp) {
				ca_printf("ca-ne: load %s OK, XFI %d is connected to phy_addr(%d)\r\n",
					CFG_ID_PHY_BASE_ADDR,xfi,intf_phy_addr[xe_start + xfi]);
			}
		}
	}

	return CA_E_OK;
}


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
ca_status_t __sfpplus_xfi_init(void)
{
	ca_status_t                 ret       = CA_E_OK;
	ca_device_id_t              device_id = 0;
	ca_uint32_t                 xfi       = 0;
	NI_HV_GLB_INTF_RST_CONFIG_t rst_cfg;

	ca_printf("ca-ne: SFPPLUS XFI init start...\n");

	g_xfi_drv.xfi_speed[0] = g_xfi_drv.xfi_speed[1]; /* SFPPLUS speed always from XFI#1 */

	/* init XFI to 10Gbps and enable */
	FOR_ALL_XFI_DO(xfi) {
		ret = aal_xfi_speed_set(device_id,xfi,g_xfi_drv.xfi_speed[xfi]);
		if(CA_E_OK != ret) {
			ca_printf("ca-ne: Init XFI %d fail (error code = 0x%x).\n",xfi,ret);
		}
	}

	/* power down xfi#5 and interface#5 */
	rst_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
	rst_cfg.bf.mac_rst_p5 = 1;
	rst_cfg.bf.pcs_rst_p5 = 1;
	CA_NI_REG_WRITE(rst_cfg.wrd,NI_HV_GLB_INTF_RST_CONFIG);
	ca_printf("ca-ne: SFPPLUS XFI init done\n");
	return ret;

}
#endif

/** aal_xfi_init:
 *@brief: initialize XFI interface according to startup config
 *@return: CA_E_OK if successfully
 */
ca_status_t aal_xfi_init(ca_device_id_t  device_id)
{
	ca_status_t                        ret = CA_E_OK;
	ca_uint32_t                        oui = 0;
#if !(defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
	aal_phy_sxgmii_pcs_control_index_t xfi = 0;
#endif
	ret = __xfi_scfg_load(device_id);
	if(CA_E_OK != ret) {
		ca_printf("ca-ne : load XFI scfg fail (return code : %#x)",ret);
	}
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	ret = __hgu_xfi_init();
	if(CA_E_OK != ret) {
		ca_printf("ca-ne: HGU XFI init fail (return code : %#x)",ret);
	}
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
	ret = __sfpplus_xfi_init();
	if(CA_E_OK != ret) {
		ca_printf("ca-ne: SFPPLUS XFI init fail (return code : %#x)",ret);
	}
#else /* G3, SFU */

	/* init XFI to 10Gbps and enable */
	FOR_ALL_XFI_DO(xfi) {
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
		if(!XFI_IS_VALID(xfi))
			continue;
#endif
		ret = aal_xfi_speed_set(device_id,xfi,g_xfi_drv.xfi_speed[xfi]);
		if(CA_E_OK != ret) {
			ca_printf("ca-ne: init XFI %d fail (error code = 0x%x)\n",xfi,ret);
		} else {
			ca_printf(
				"ca-ne: init XFI %d OK, speed %u\n",
				xfi,
				g_xfi_drv.xfi_speed[xfi]);
		}
	}

	FOR_ALL_XFI_DO(xfi) {
		if(g_xfi_drv.xfi_intf_type[xfi] != AAL_NI_IF_MODE_USXGMII) {
			continue;
		}

		aal_eth_phy_oui_get(device_id,xfi + XFI_MAC_PORT_START,&oui);

		if(oui == PHY_OUI_MVL) {
			g_xfi_drv.usxgmii_an_tp[xfi] = AAL_USXGMII_AN_TP_SW;
		}

		aal_xfi_autoneg_set(device_id,xfi,AAL_USXGMII_AN_ENABLE);
	}
#endif

	return CA_E_OK;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(aal_xfi_usxgmii_an_tp_set);
EXPORT_SYMBOL(aal_xfi_usxgmii_an_tp_get);
EXPORT_SYMBOL(aal_xfi_autoneg_set);
EXPORT_SYMBOL(aal_xfi_autoneg_get);
EXPORT_SYMBOL(aal_xfi_autoneg_restart);
EXPORT_SYMBOL(aal_xfi_speed_set);
EXPORT_SYMBOL(aal_xfi_speed_get);
EXPORT_SYMBOL(aal_xfi_status_get);
EXPORT_SYMBOL(aal_xfi_cfg_set);
EXPORT_SYMBOL(aal_xfi_cfg_get);
EXPORT_SYMBOL(aal_xfi_link_status_get);
EXPORT_SYMBOL(aal_xfi_init);
EXPORT_SYMBOL(aal_xfi_reset);
EXPORT_SYMBOL(aal_xfi_port2xfi);
EXPORT_SYMBOL(aal_xfi_link_toggle_rx_en);

#endif /* __KERNEL__ */


