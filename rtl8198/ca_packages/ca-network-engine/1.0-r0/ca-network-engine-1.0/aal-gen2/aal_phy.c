
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
/* aal_phy.c Cortina-Access PHY driver*/
#include "ca_types.h"
#include "osal_common.h"
#include "peri.h"
#include "ca-scfg/scfg.h"
#include "aal_common.h"
#include "aal_ni.h"
#include "aal_phy.h"
#include "aal_ni_l2.h"


#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define __INTERNAL_PHY_MDIO_START 1
#define __INTERNAL_PHY_MDIO_END   1
#else
#define __INTERNAL_PHY_MDIO_START 1
#define __INTERNAL_PHY_MDIO_END   4
#endif

#if defined(CONFIG_CORTINA_BOARD_FPGA)
#define __PHY_IS_INTERNAL(addr)        0
#else
#define __PHY_IS_INTERNAL(addr) ((addr) >= __INTERNAL_PHY_MDIO_START && (addr) <= __INTERNAL_PHY_MDIO_END)
#endif
#define __MDIO_MAX_ADDR      32
#define __PHY_IS_XGPHY(addr) (g_mdio_drv.is_xgphy[addr])
/* MDC clock frequency */
#define __MDIO_CLOCK_MIN     (1)
#define __MDIO_CLOCK_MAX     (20000)
#define __MDIO_CLOCK_DEF     (4000)

#define __PER_REG_READ(dev,addr)         CA_PER_MDIO_REG_READ(addr)
#define __PER_REG_WRITE(dev,addr,data)   CA_PER_MDIO_REG_WRITE(data, addr)

#define __DELAY_US(usecs)      ca_udelay(usecs)
#define __DELAY_MS(msecs)      ca_mdelay(msecs)

ca_uint32_t ca_phy_debug = 0;

#define __PHY_DBG(fmt,...)  do {\
                                if (ca_phy_debug & 0x01)\
                                    ca_printf("%s():"fmt, __func__,##__VA_ARGS__);\
                            }while(0)

#define __MDIO_DBG(fmt,...)  do {\
                                if (ca_phy_debug & 0x02)\
                                    ca_printf("%s():"fmt, __func__,##__VA_ARGS__);\
                            }while(0)

/* Internal PHY MDIO access MCROs:
    Internally to our chip the MDIO interface of the GPHY is memory mapped into
    the chip register address space. As a result they will do a direct memory
    write for each of the writes that they put into that file.

    The mapping works like this:

    Basic GPHY offset for all memory reads/writes: 0xF433_8000

    The address for accessing a particular register inside a particular phy is:

    Address[31:12] = F4338
    Address[11:7] = PHYADDR (could be 0x1, 0x2, 0x3, 0x4 for our design)
    Address[6:2] = REGADDR
    Addresss[1:0] = 00
    ---------------------------------
*/

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define ASIC_PHY_MDIO_READ(phy,reg)      \
    (CA_NE_PHY_REG_READ((reg)<<2) & 0xFFFF)
#define ASIC_PHY_MDIO_WRITE(phy,reg,data) \
    CA_NE_PHY_REG_WRITE((data) & 0xFFFF,(reg)<<2)
#else
#define ASIC_PHY_REG_BASE                0xF4338000
#define ASIC_PHY_MDIO_READ(phy,reg)      \
    CA_NE_PHY_REG_READ(ASIC_PHY_REG_BASE+(((phy)&0x1F)<<7)+(((reg)&0x1F)<<2))
#define ASIC_PHY_MDIO_WRITE(phy,reg,data) \
    CA_NE_PHY_REG_WRITE(data,ASIC_PHY_REG_BASE+(((phy)&0x1F)<<7)+(((reg)&0x1F)<<2))
#endif

#define AAL_MDIO_LOCK(device_id)        ca_spin_lock(g_mdio_drv.aal_mdio_lock)
#define AAL_MDIO_UNLOCK(device_id)      ca_spin_unlock(g_mdio_drv.aal_mdio_lock)

/**************************************************************/
/*           MDIO Access implementation definition            */
/**************************************************************/

typedef struct {
    ca_uint aal_mdio_lock; /* spinlock for MDIO operation */
    ca_uint32_t init_flag; /* initialized flag: 1 means initialized done */
    ca_uint8_t  is_xgphy[__MDIO_MAX_ADDR]; /* XG-PHY meet the IEEE802.3 Clause 45 and later, index is MDIO address */
} aal_mdio_drv_t;

static aal_mdio_drv_t g_mdio_drv = {0};
static ca_uint8_t  g_phy_intf[CFG_ID_PHY_INTERFACE_LEN] = {0};
static ca_uint8_t  g_phy_addr[CFG_ID_PHY_BASE_ADDR_LEN] = {0};

/**aal_mdio_global_init:
 *@breif: MDIO driver initialization
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_global_init(ca_device_id_t device_id)
{
    ca_uint32_t idx = 0;

    if(g_mdio_drv.init_flag==1) {
        return CA_E_OK;
    }
    memset(&g_mdio_drv, 0, sizeof(g_mdio_drv));
    if (CA_E_OK != ca_spin_lock_init(&g_mdio_drv.aal_mdio_lock, SPINLOCK_BH)) {
        ca_printf("%s(): ca_spin_lock_init aal_mdio_lock fail!\r\n");
        return CA_E_RESOURCE;
    }


    if (CA_E_OK == scfg_read(device_id, CFG_ID_PHY_INTERFACE, CFG_ID_PHY_INTERFACE_LEN, g_phy_intf)) {
        (void)scfg_read(device_id, CFG_ID_PHY_BASE_ADDR, CFG_ID_PHY_BASE_ADDR_LEN, g_phy_addr);
        for (idx = 0; idx < CFG_ID_PHY_INTERFACE_LEN; idx++) {

            if (g_phy_intf[idx] > AAL_NI_IF_MODE_MAX)
                continue;

            if (g_phy_intf[idx] == AAL_NI_IF_MODE_XFI 
                || g_phy_intf[idx] == AAL_NI_IF_MODE_XGMII
                || g_phy_intf[idx] == AAL_NI_IF_MODE_RXAUI
                || g_phy_intf[idx] >= AAL_NI_IF_MODE_USXGMII) { /* find XG-PHY */

                if (!__PHY_IS_INTERNAL(g_phy_addr[idx]) && g_phy_addr[idx] < __MDIO_MAX_ADDR) {
                    g_mdio_drv.is_xgphy[g_phy_addr[idx]] = TRUE;
                    ca_printf("port %d PHY addr %d connect to XG-PHY.\r\n", idx, g_phy_addr[idx]);
                }
            }
        }
    }
    
    g_mdio_drv.init_flag = 1;
    ca_printf("ca-ne: init MDIO OK.\r\n");
    return CA_E_OK;
}
EXPORT_SYMBOL(aal_mdio_global_init);

/**aal_mdio_global_exit:
 *@breif: MDIO driver destroy
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_global_exit(ca_device_id_t device_id)
{
    if (g_mdio_drv.aal_mdio_lock)
        ca_spin_lock_destroy(g_mdio_drv.aal_mdio_lock);
    g_mdio_drv.aal_mdio_lock = 0;
    g_mdio_drv.init_flag = 0;
    return CA_E_OK;
}
EXPORT_SYMBOL(aal_mdio_global_exit);

/**aal_mdio_speed_set:
 *@breif: Set MDC clock frequency
 *@param [in] addr: PHY Address, 0~31
 *@param [in] clock: clock frequency, Hz
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_speed_set (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  addr,
    CA_IN  ca_uint32_t  clock)

{
    ca_uint16_t    prer = 0;
    volatile PER_MDIO_CFG_t cfg;


    if (clock < __MDIO_CLOCK_MIN ||
        clock > __MDIO_CLOCK_MAX ) {
        return CA_E_PARAM;
    }

    AAL_MDIO_LOCK(device_id);

    cfg.wrd = __PER_REG_READ(device_id, PER_MDIO_CFG);

    /* Caculate pre scale for MDIO clock adjust */
    prer = ( CA_AAL_MDIO_PER_CLK / (clock << 1)) - 1;
    if (( CA_AAL_MDIO_PER_CLK % (clock << 1)) > clock) {
        prer += 1;
    }

    cfg.bf.mdio_pre_scale = prer;
    cfg.bf.mdio_manual    = 1;

    __PER_REG_WRITE(device_id, PER_MDIO_CFG, cfg.wrd);

    AAL_MDIO_UNLOCK(device_id);

    return CA_E_OK;

}

/**aal_mdio_speed_get:
 *@breif: Get MDC clock frequency
 *@param [in] addr: PHY Address, 0~31
 *@param [out] clock: clock frequency, Hz
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_speed_get (
    CA_IN       ca_device_id_t         device_id,
    CA_IN       ca_uint32_t         addr,
    CA_OUT      ca_uint32_t        *clock)
{
    PER_MDIO_CFG_t cfg;

    CA_ASSERT_RET(clock, CA_E_PARAM);

    AAL_MDIO_LOCK(device_id);

    cfg.wrd = __PER_REG_READ(device_id, PER_MDIO_CFG);

    *clock = CA_AAL_MDIO_PER_CLK / (cfg.bf.mdio_pre_scale + 1) >> 1;

    AAL_MDIO_UNLOCK(device_id);

    return CA_E_OK;

}

static ca_status_t aal_mdio_write_direct(
   CA_IN       ca_device_id_t          device_id,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_addr,
    CA_IN      ca_uint16_t             data)
{
    PER_MDIO_ADDR_t  mdio_addr;
    PER_MDIO_CTRL_t  mdio_ctrl;
    ca_uint32_t      loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;

    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_op     = 1;  /* type of indirect access */
    mdio_addr.bf.mdio_offset = reg_addr;                  /* offset */
    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_WR_FLAG;
    mdio_addr.bf.mdio_st     = 0;  /* indirect/direct access */

    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);
    __PER_REG_WRITE(device_id, PER_MDIO_WRDATA, data);

    mdio_ctrl.wrd          = 0;
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);

    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
            /* Clear MDIO done */
             mdio_ctrl.bf.mdiodone = 1;
            __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
            return CA_E_OK;
        }
        ca_udelay(1);
    } while(--loop_wait);

    __MDIO_DBG("direct write address %#x timeout!!\n", phy_port_addr);

    return CA_E_TIMEOUT;
}

ca_status_t aal_mdio_write_indirect(
   CA_IN       ca_device_id_t             device_id,
   CA_IN       ca_uint8_t             phy_port_addr,
   CA_IN       ca_uint8_t             dev_addr,
   CA_IN       ca_uint16_t             addr,
   CA_IN       ca_uint16_t             data)
{
    PER_MDIO_ADDR_t  mdio_addr;
    PER_MDIO_CTRL_t  mdio_ctrl;
    ca_uint32_t      loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;
    ca_uint32_t      tmp_data;

    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_op     = 0;                       /* address write */
    mdio_addr.bf.mdio_offset = dev_addr;                  /* region area */
    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_WR_FLAG;
    mdio_addr.bf.mdio_st     = 1;                       /* indirect access */
    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);
    __PER_REG_WRITE(device_id, PER_MDIO_WRDATA, addr);

    tmp_data = __PER_REG_READ(device_id, PER_MDIO_ADDR);
    //printk("%s: PER_MDIO_ADDR=0x%x\n", __func__, tmp_data);

    mdio_ctrl.wrd          = 0;
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);

    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
                __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
                break;
        }
        ca_udelay(1);
    } while(--loop_wait);
    if (loop_wait == 0) {
        ca_printf("%s: indirect write address timeout!!\n", __func__);
        return CA_E_TIMEOUT;
    }

    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_offset = dev_addr;
    mdio_addr.bf.mdio_op     = 1;           /* indriect WRITE */
    mdio_addr.bf.mdio_st     = 1;	    /* indirect access */

    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_WR_FLAG;
    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);
    __PER_REG_WRITE(device_id, PER_MDIO_WRDATA, data);

    mdio_ctrl.wrd          = 0;
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);

    loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;
    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
            /* Clear MDIO done */
             mdio_ctrl.bf.mdiodone = 1;
            __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
            return CA_E_OK;
        }
        ca_udelay(1);
    } while(--loop_wait);

    __MDIO_DBG("indirect write address %#x timeout!!\n", phy_port_addr);

    return CA_E_TIMEOUT;
}
EXPORT_SYMBOL(aal_mdio_write_indirect);

 /**aal_mdio_write:
 *@breif: Write data to MDIO slaver device with 16bits width
 *@param [in] st_code : Start of Frame (01 for Clause 22;00 for Clause 45)
 *@param [in] phy_port_addr : PHY Address, 0~31
 *@param [in] reg_dev_addr  : Register Address, 0~31
 *@param [in] addr  : extended address for Clause45, normally NOT used
 *@param [in] data  : the Data writen to MDIO slaver
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_write (
    CA_IN       ca_device_id_t         device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t             addr,
    CA_IN      ca_uint16_t             data)
{
    ca_status_t ret = CA_E_OK;

    AAL_MDIO_LOCK(device_id);
#ifdef CONFIG_CORTINA_BOARD_FPGA
    ret = aal_mdio_write_direct(device_id, phy_port_addr, reg_dev_addr, data);
#else

    if (__PHY_IS_INTERNAL(phy_port_addr)) {
        ASIC_PHY_MDIO_WRITE((ca_uint32_t)phy_port_addr, (ca_uint32_t)reg_dev_addr, data);
        ca_mdelay( 1 );
    } else {
        /*1 for IEEE 802.3 clause 22.2.4.5.3 (direct access),
          0 for IEEE 802.3 clause 45.3.3  (indirect access) */
        if (st_code == 0){
            ret = aal_mdio_write_indirect(device_id, phy_port_addr, reg_dev_addr, addr, data);
        } else {
            ret = aal_mdio_write_direct(device_id, phy_port_addr, reg_dev_addr, data);
        }
    }
#endif

    AAL_MDIO_UNLOCK(device_id);
    return ret;
}
EXPORT_SYMBOL(aal_mdio_write);

static ca_status_t aal_mdio_read_direct (
    CA_IN       ca_device_id_t         device_id,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_addr,
    CA_OUT      ca_uint16_t           *data)
{
    PER_MDIO_ADDR_t    mdio_addr;
    PER_MDIO_CTRL_t    mdio_ctrl;
    PER_MDIO_RDDATA_t  read_data;
    ca_uint32_t        loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;


    CA_ASSERT_RET(data, CA_E_PARAM);

    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_op     = 3;
    mdio_addr.bf.mdio_offset = reg_addr;
    mdio_addr.bf.mdio_st     = 0;
    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_RD_FLAG;

    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);

    mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);

    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
            /* Read MDIO data */
            read_data.wrd = __PER_REG_READ(device_id, PER_MDIO_RDDATA);
            *data = read_data.bf.mdio_rddata;
            /* Clear MDIO done */
             mdio_ctrl.bf.mdiodone = 1;
            __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
            return CA_E_OK;
        }
        ca_udelay(1);
    } while(--loop_wait);

    __MDIO_DBG("direct read address %#x timeout!!\n", phy_port_addr);

    return CA_E_TIMEOUT;
}

ca_status_t aal_mdio_read_indirect (
    CA_IN       ca_device_id_t         device_id,
    CA_IN       ca_uint8_t             phy_port_addr,
    CA_IN       ca_uint8_t             dev_type,
    CA_IN       ca_uint16_t            addr,
    CA_OUT      ca_uint16_t           *data)
{
    PER_MDIO_ADDR_t    mdio_addr;
    PER_MDIO_CTRL_t    mdio_ctrl;
    PER_MDIO_RDDATA_t  read_data;
    ca_uint32_t        loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;
    ca_uint32_t        tmp_data;

    CA_ASSERT_RET(data, CA_E_PARAM);
    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_op     = 0;           /* address write */
    mdio_addr.bf.mdio_offset = dev_type;                  /* region area */
    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_WR_FLAG;
    mdio_addr.bf.mdio_st     = 1;           /* indirect access */
    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);
    __PER_REG_WRITE(device_id, PER_MDIO_WRDATA, addr);
    tmp_data = __PER_REG_READ(device_id, PER_MDIO_ADDR);
    /*ca_printf("%s: PER_MDIO_ADDR=0x%x\n", __func__, tmp_data);*/
    mdio_ctrl.wrd          = 0;
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
            __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
            break;
        }
        ca_udelay(1);
    } while(--loop_wait);
    if (loop_wait == 0) {
        ca_printf("%s: indirect write address timeout!!\n", __func__);
    }

    mdio_addr.wrd            = 0;
    mdio_addr.bf.mdio_addr   = phy_port_addr;
    mdio_addr.bf.mdio_op     = 3;             /* indirect read */
    mdio_addr.bf.mdio_offset = dev_type;              /* region area */
    mdio_addr.bf.mdio_st     = 1;             /* indirect access */
    mdio_addr.bf.mdio_rd_wr  = CA_AAL_MDIO_RD_FLAG;
    __PER_REG_WRITE(device_id, PER_MDIO_ADDR, mdio_addr.wrd);

    mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
    mdio_ctrl.bf.mdiostart = 1;
    __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);

    loop_wait = CA_AAL_MDIO_ACCESS_TIMEOUT;
    do {
        mdio_ctrl.wrd = __PER_REG_READ(device_id, PER_MDIO_CTRL);
        if (mdio_ctrl.bf.mdiodone) {
            /* Read MDIO data */
            read_data.wrd = __PER_REG_READ(device_id, PER_MDIO_RDDATA);
            *data = read_data.bf.mdio_rddata;
            /* Clear MDIO done */
             mdio_ctrl.bf.mdiodone = 1;
            __PER_REG_WRITE(device_id, PER_MDIO_CTRL, mdio_ctrl.wrd);
            return CA_E_OK;
        }
        ca_udelay(1);
    } while(--loop_wait);

    __MDIO_DBG("indirect read address %#x timeout!!\n", phy_port_addr);

    return CA_E_TIMEOUT;
}
EXPORT_SYMBOL(aal_mdio_read_indirect);

 /**aal_mdio_read:
 *@breif: Read MDIO slaver device data with 16bits width
 *@param [in] st_code : Start of Frame (01 for Clause 22;00 for Clause 45)
 *@param [in] phy_port_addr : PHY Address, 0~31
 *@param [in] reg_dev_addr  : Register Address, 0~31
 *@param [in] addr  : extended address for Clause45, normally NOT used
 *@param [out] data : the Data returned from MDIO slaver
 *@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
 */
ca_status_t aal_mdio_read (
    CA_IN      ca_device_id_t          device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t             addr,
    CA_OUT     ca_uint16_t            *data)
{
    ca_status_t ret = CA_E_OK;

    AAL_MDIO_LOCK(device_id);
#ifdef CONFIG_CORTINA_BOARD_FPGA
    ret = aal_mdio_read_direct(device_id, phy_port_addr, reg_dev_addr, data);
#else

    if (__PHY_IS_INTERNAL(phy_port_addr)) {
        *data = ASIC_PHY_MDIO_READ((ca_uint32_t)phy_port_addr, (ca_uint32_t)reg_dev_addr);
    } else {
        /* 1 for IEEE 802.3 clause 22.2.4.5.3 (direct access) */
        /* 0 for IEEE 802.3 clause 45.3.3  (indirect access) */
        if (st_code == 0) {
            ret = aal_mdio_read_indirect(device_id, phy_port_addr, reg_dev_addr, addr, data);
        } else {
            ret = aal_mdio_read_direct(device_id, phy_port_addr, reg_dev_addr, data);
        }
    } 
#endif /* CONFIG_CORTINA_BOARD_FPGA */

    AAL_MDIO_UNLOCK(device_id);
    return ret;
}
EXPORT_SYMBOL(aal_mdio_read);

/**aal_mdio_set
 *@brief: function to support AAL PHY CLI auto-generation format for MDIO write
 */
ca_status_t aal_mdio_set (
    CA_IN      ca_device_id_t         device_id,
    CA_IN      ca_uint8_t             st_code,
    CA_IN      ca_uint8_t             phy_port_addr,
    CA_IN      ca_uint8_t             reg_dev_addr,
    CA_IN      ca_uint16_t            addr,
    CA_IN      ca_uint16_t            data)
{
    return aal_mdio_write (device_id,st_code, phy_port_addr, reg_dev_addr, addr, data);
}

/**aal_mdio_get
 *@brief: function to support AAL PHY CLI auto-generation format for MDIO read
 */
ca_status_t aal_mdio_get (
    CA_IN       ca_device_id_t         device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t             addr,
    CA_OUT      ca_uint16_t           *data)
{
    return aal_mdio_read (device_id,st_code, phy_port_addr, reg_dev_addr, addr, data);
}

/**************************************************************/
/*       Etherent PHY MII Standard Registers definition       */
/**************************************************************/

#define PHY_REG_CTRL                             (0)
#define PHY_REG_STATUS                           (1)
#define PHY_REG_ID_1                             (2)
#define PHY_REG_ID_2                             (3)
#define PHY_REG_AUTONEG_ADV                      (4)
#define PHY_REG_LP_ABILITY_BASE                  (5)
#define PHY_REG_AN_EXT                           (6)
#define PHY_REG_AUTONEG_NEXT_PAGE                (7)
#define PHY_REG_LP_NEXT_PAGE                     (8)
#define PHY_REG_1000BASET_CTRL                   (9)
#define PHY_REG_1000BASET_STATUS                 (10)
#define PHY_REG_RSVD1                            (11)
#define PHY_REG_RSVD2                            (12)
#define PHY_REG_RSVD3                            (13)
#define PHY_REG_RSVD4                            (14)
#define PHY_REG_1000BSET_STATUS_EXT              (15)
#define PHY_REG_SPECIFIC_STATUS                  (17)

/* Control register bits */
#define PHY_REG_CTRL_BIT_SPEED_SEL_MSB           (6)
#define PHY_REG_CTRL_BIT_COLLISTION_TEST         (7)
#define PHY_REG_CTRL_BIT_DUPLEX                  (8)
#define PHY_REG_CTRL_BIT_AN_RESTART              (9)
#define PHY_REG_CTRL_BIT_ISOLATE                 (10)
#define PHY_REG_CTRL_BIT_PWR_DOWN                (11)
#define PHY_REG_CTRL_BIT_AN_CTRL                 (12)
#define PHY_REG_CTRL_BIT_SPEED_SEL_LSB           (13)
#define PHY_REG_CTRL_BIT_LPBK                    (14)
#define PHY_REG_CTRL_BIT_RESET                   (15)


/* Status register bits */
#define PHY_REG_STATUS_BIT_EXT_CAP               (0)
#define PHY_REG_STATUS_BIT_JBRD                  (1)
#define PHY_REG_STATUS_BIT_LNK                   (2)
#define PHY_REG_STATUS_BIT_AN_CAPABLE            (3)
#define PHY_REG_STATUS_BIT_RMT_FAULT             (4)
#define PHY_REG_STATUS_BIT_AN_DONE               (5)
#define PHY_REG_STATUS_BIT_EXT_STATUS            (8)
#define PHY_REG_STATUS_BIT_100BT2_HD_SUPP        (9)
#define PHY_REG_STATUS_BIT_100BT2_FD_SUPP        (10)
#define PHY_REG_STATUS_BIT_10_HD_SUPP            (11)
#define PHY_REG_STATUS_BIT_10_FD_SUPP            (12)
#define PHY_REG_STATUS_BIT_100BX_HD_SUPP         (13)
#define PHY_REG_STATUS_BIT_100BX_FD_SUPP         (14)
#define PHY_REG_STATUS_BIT_100BT4_SUPP           (15)

/* Advertisement control register bits */
#define PHY_REG_AN_ADVR_BIT_CSMA                 (0)
#define PHY_REG_AN_ADVR_BIT_10BT_HD              (5)
#define PHY_REG_AN_ADVR_BIT_10BT_FD              (6)
#define PHY_REG_AN_ADVR_BIT_100BX_HD             (7)
#define PHY_REG_AN_ADVR_BIT_100BX_FD             (8)
#define PHY_REG_AN_ADVR_BIT_100BT4_CAP           (9)
#define PHY_REG_AN_ADVR_BIT_PAUSE_CAP            (10)
#define PHY_REG_AN_ADVR_BIT_PAUSE_ASYM           (11)
#define PHY_REG_AN_ADVR_BIT_RMT_FAULT            (13)
#define PHY_REG_AN_ADVR_BIT_NPAGE                (15)

/* Link partner ability register bits */
#define PHY_REG_LP_ABLITY_BIT_10BT_HD            (5)
#define PHY_REG_LP_ABLITY_BIT_10BT_FD            (6)
#define PHY_REG_LP_ABLITY_BIT_100BX_HD           (7)
#define PHY_REG_LP_ABLITY_BIT_100BX_FD           (8)
#define PHY_REG_LP_ABLITY_BIT_100BT4_CAP         (9)
#define PHY_REG_LP_ABLITY_BIT_PAUSE_CAP          (10)
#define PHY_REG_LP_ABLITY_BIT_PAUSE_ASYM         (11)
#define PHY_REG_LP_ABLITY_BIT_RESV               (12)
#define PHY_REG_LP_ABLITY_BIT_RMT_FAULT          (13)
#define PHY_REG_LP_ABLITY_BIT_LP_ACK             (14)
#define PHY_REG_LP_ABLITY_BIT_NP                 (15)

/* Auto-negotiation Expansion register bits */
 #define PHY_REG_AN_EXT_BIT_LP_AN_CAP            (0)
 #define PHY_REG_AN_EXT_BIT_LP_NP_RX             (1)
 #define PHY_REG_AN_EXT_BIT_NP_CAP               (2)
 #define PHY_REG_AN_EXT_BIT_LP_NP_CAP            (3)
 #define PHY_REG_AN_EXT_BIT_P_FAULT              (4)

/* 1000BASE-T Control register bits */
#define PHY_REG_1000BT_CTRL_BIT_ADVR_1000_HD     (8)
#define PHY_REG_1000BT_CTRL_BIT_ADVR_1000_FD     (9)
#define PHY_REG_1000BT_CTRL_BIT_MASTER_CFG       (10)
#define PHY_REG_1000BT_CTRL_BIT_MMODE_CFG_ENBL   (11)

/* 1000BASE-T Status register bits */
#define PHY_REG_1000BT_STATUS_BIT_LP_1000_HD     (10)
#define PHY_REG_1000BT_STATUS_BIT_LP_1000_FD     (11)
#define PHY_REG_1000BT_STATUS_BIT_RMTRX_OK       (12)
#define PHY_REG_1000BT_STATUS_BIT_LCLRX_OK       (13)

/* Interrupts Definitions */
#define PHY_INTR_LINK_CHANGE           0x0001 /* Link Change Interrupt   */
#define PHY_INTR_SPEED_CHANGE          0x0002 /* Speed Change Interrupt  */
#define PHY_INTR_DUPLEX_CHANGE         0x0004 /* Duplex Change Interrupt */
#define PHY_INTR_AN_DONE               0x0008 /* AN Done Interrupt       */
#define PHY_INTR_AN_FAIL               0x0010 /* AN Fail Interrupt       */

//Register 13================================================================
//  Bits   |  R/W  | Default |   DESC     
//---------+-------+---------+----------------------------------------------
//[15:14]  |   WO  |00       | Function Code, detail is as following:
//         |       |         | 00: Address
//         |       |         | 01: Data with no post increment
//         |       |         | 10: Data with post increment on reads and writes
//         |       |         | 11: Data with post increment on writes only
//[13:05]  | RO    |000000000|  Reserved.
//[04:00]  | WO    |0        |  Device Address.
//============================================================================
#define PHY_REG_MACR          13 // for MMD access
#define PHY_REG_MAADR         14 // for MMD content W/R

//Register 14=================================================================
//  Bits   |  R/W  | Default   |   DESC     
//---------+-------+-----------+----------------------------------------------
//[15:0]   | RW    | 0         | -If MMD Function == 0, means register address 
//         |       |           | -If MMD Function != 0, means register data
//============================================================================

#define MMD_FUNC_ADDR          0
#define MMD_FUNC_DATA_NO_INC   1
#define MMD_FUNC_DATA_INC_RW   2
#define MMD_FUNC_DATA_INC_WO   3
#define MAKE_MMD(fcode, mmddev) ((((fcode)&0x3)<<14)|((mmddev) &0x1f))

#define ETH_PHY_NUM_MAX  32

/* Ethernet PHY driver object */
typedef struct {
    ca_status_t (*init)(ca_uint8_t);
    ca_status_t (*reset)(ca_uint8_t );
    ca_status_t (*auto_neg_restart)(ca_uint8_t );
    ca_status_t (*power_set)(ca_uint8_t, ca_boolean_t );
    ca_status_t (*power_get)(ca_uint8_t, ca_boolean_t*);
    ca_status_t (*auto_neg_set)(ca_uint8_t, ca_boolean_t );
    ca_status_t (*auto_neg_get)(ca_uint8_t, ca_boolean_t*);
    ca_status_t (*ability_adv_set)(ca_uint8_t, aal_phy_autoneg_adv_t );
    ca_status_t (*ability_adv_get)(ca_uint8_t, aal_phy_autoneg_adv_t*);
    ca_status_t (*partner_ability_adv_get)(ca_uint8_t, aal_phy_autoneg_adv_t*);
    ca_status_t (*link_status_get)(ca_uint8_t, aal_phy_link_status_t* );
    ca_status_t (*speed_set)(ca_uint8_t, aal_phy_speed_mode_t );
    ca_status_t (*speed_get)(ca_uint8_t, aal_phy_speed_mode_t*);
    ca_status_t (*duplex_set)(ca_uint8_t, ca_boolean_t );
    ca_status_t (*duplex_get)(ca_uint8_t, ca_boolean_t*);
    ca_status_t (*flow_ctrl_set)(ca_uint8_t, ca_boolean_t );
    ca_status_t (*flow_ctrl_get)(ca_uint8_t, ca_boolean_t*);
    ca_status_t (*mdix_set)(ca_uint8_t, aal_phy_mdi_mode_t );
    ca_status_t (*mdix_get)(ca_uint8_t, aal_phy_mdi_mode_t*);
    ca_status_t (*loopback_set)(ca_uint8_t, aal_phy_lpbk_mode_t);
    ca_status_t (*loopback_get)(ca_uint8_t, aal_phy_lpbk_mode_t*);
    ca_status_t (*int_mask_all)(ca_uint8_t);
    ca_status_t (*int_en_set)(ca_uint8_t, ca_uint16_t);
    ca_status_t (*int_en_get)(ca_uint8_t, ca_uint16_t*);
    ca_status_t (*int_status_get)(ca_uint8_t, ca_uint16_t*);
    ca_status_t (*int_status_clr)(ca_uint8_t);
    ca_status_t (*debug_info_get)(ca_uint8_t);
	ca_status_t (*fw_upgrade_set)(ca_uint8_t, aal_phy_boot_mode_t);
	ca_status_t (*fw_upgrade_get)(ca_uint8_t, aal_phy_boot_mode_t*);

}phy_drv_t;

typedef struct {
    char        name[32];
    ca_uint32_t oui;
    ca_uint32_t model;
    phy_drv_t   drv;
} phy_dev_t;

/**************************************************************/
/*       Internal Etherent PHY register access functions      */
/**************************************************************/
static ca_status_t __eth_phy_regfield_read
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   reg,
    ca_uint8_t   offset,
    ca_uint8_t   length,
    ca_uint16_t *p_val16
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t data = 0;

    if (length == 0
        || offset >= PHY_REG_MAX_LEN
        || length > (PHY_REG_MAX_LEN - offset)
        || !p_val16) {
        return CA_E_PARAM;
    }

    ret = aal_mdio_read(0, 1, phy_dev, reg, 0, &data);
    if (ret != CA_E_OK) {
        goto end;
    }

    if ((0 == offset) && (16 == length)) {
        *p_val16 = data & 0xffff;
    }
    else {
        *p_val16 = (data >> offset) & ((0x1 << length) -1);
    }

end:
    return ret;

}

static ca_status_t __eth_phy_regfield_write
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   reg,
    ca_uint8_t   offset,
    ca_uint8_t   length,
    ca_uint16_t  val16
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t data = 0;
    ca_uint16_t data16 = 0;

    if (length == 0
        || offset >= PHY_REG_MAX_LEN
        || length > (PHY_REG_MAX_LEN - offset)){
        return CA_E_PARAM;
    }

    ret = aal_mdio_read(0, 1, phy_dev, reg, 0, &data);
    if (ret != CA_E_OK ) {
        goto end;
    }

    data16 = (data & ~(((0x1 << length) -1) << offset))
                | (val16 << offset);

    if ((data & 0xffff) == data16) {
        goto end;
    }

    ret = aal_mdio_write(0, 1, phy_dev, reg, 0, data16);
    if (ret != CA_E_OK ) {
        goto end;
    }

end:
    return ret;

}


static ca_status_t __eth_phy_regfield_mask_write
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   reg,
    ca_uint16_t  mask,
    ca_uint16_t  val16
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t data = 0;
    ca_uint16_t data16 = val16;

    if (mask != 0xffff) {
        ret = aal_mdio_read(0, 1, phy_dev, reg, 0, &data);
        if (ret != CA_E_OK ) {
            goto end;
        }

        data &= ~mask;  /*mask the bits want to be overwrited */
        data16 = (data & 0xffff) | (val16 & mask); /*overwrite the data */
    }

    ret = aal_mdio_write(0, 1, phy_dev, reg, 0, data16);
    if (ret != CA_E_OK ) {
        goto end;
    }

end:

    return ret;
}

static ca_status_t __eth_phy_mmd_read
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   mmd_dev,
    ca_uint8_t   reg,
    ca_uint16_t *p_val16
)
{
    ca_status_t ret    = CA_E_OK;

    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MACR, 0, MAKE_MMD(MMD_FUNC_ADDR, mmd_dev));
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MAADR, 0, reg);
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MACR, 0, MAKE_MMD(MMD_FUNC_DATA_NO_INC, mmd_dev));
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_read(0, 1, phy_dev, PHY_REG_MAADR, 0, p_val16);


end:
    return ret;

}

static ca_status_t __eth_phy_mmd_write
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   mmd_dev,
    ca_uint8_t   reg,
    ca_uint16_t  val16
)
{


    ca_status_t ret = CA_E_OK;

    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MACR, 0, MAKE_MMD(MMD_FUNC_ADDR, mmd_dev));
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MAADR, 0, reg);
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MACR, 0, MAKE_MMD(MMD_FUNC_DATA_NO_INC, mmd_dev));
    if (ret != CA_E_OK) {
        goto end;
    }
    ret = aal_mdio_write(0, 1, phy_dev, PHY_REG_MAADR, 0, val16);

end:
    return ret;

}

/**************************************************************/
/*       Internal Etherent PHY clause 45 register access functions      */
/**************************************************************/
static ca_status_t __eth_phy_clause45_regfield_read
(
    ca_uint8_t   phy_dev,
    ca_uint8_t   reg_dev_addr,
    ca_uint16_t   reg,
    ca_uint8_t   offset,
    ca_uint8_t   length,
    ca_uint16_t *p_val16
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t data = 0;

    if (length == 0
        || offset >= PHY_REG_MAX_LEN
        || length > (PHY_REG_MAX_LEN - offset)
        || !p_val16) {
        return CA_E_PARAM;
    }

    ret = aal_mdio_read(0, 0, phy_dev, reg_dev_addr, reg, &data);
    if (ret != CA_E_OK) {
        goto end;
    }

    if ((0 == offset) && (16 == length)) {
        *p_val16 = data & 0xffff;
    }
    else {
        *p_val16 = (data >> offset) & ((0x1 << length) -1);
    }

end:
    return ret;

}

/**************************************************************/
/*             Common Etherent PHY APIs                       */
/**************************************************************/

static ca_status_t  __eth_phy_debug_info_get(ca_uint8_t phy_dev)
{
    return CA_E_OK;
}

static ca_status_t  __eth_phy_reset(ca_uint8_t phy_dev)
{
    return __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_RESET, 1, 1);
}

static ca_status_t  __eth_phy_auto_neg_restart(ca_uint8_t phy_dev)
{
    return __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_AN_RESTART, 1, 1);
}

static ca_status_t  __eth_phy_power_set(ca_uint8_t phy_dev, ca_boolean_t power_up)
{
    return __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_PWR_DOWN, 1, power_up ? FALSE : TRUE);
}

static ca_status_t  __eth_phy_power_get(ca_uint8_t phy_dev, ca_boolean_t *power_up)
{
    ca_uint16 data16 = 0;
    ca_status_t ret = CA_E_OK;

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_PWR_DOWN, 1, &data16);
    *power_up = data16 ? FALSE : TRUE;

    return ret;
}

static ca_status_t  __eth_phy_auto_neg_set(ca_uint8_t phy_dev, ca_boolean_t enable)
{
    return __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_AN_CTRL, 1, enable ? 1 : 0);
}

static ca_status_t  __eth_phy_auto_neg_get(ca_uint8_t phy_dev, ca_boolean_t *enable)
{
    ca_uint16 data16 = 0;
    ca_status_t ret = CA_E_OK;

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_AN_CTRL, 1, &data16);
    *enable = data16 ? TRUE : FALSE;

    return ret;
}

static ca_status_t  __eth_phy_ability_adv_set(ca_uint8_t phy_dev, aal_phy_autoneg_adv_t adv)
{
    ca_uint16_t  val16 = 0;
    ca_status_t  ret = CA_E_OK;
    
    if (__PHY_IS_INTERNAL(phy_dev)) {
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a47); /* page select 0xa47 */
        ca_mdelay(1);
        /*mask nway ability
        [0]: mask my10f/my10h
        [1]: mask my100f/my100h
        [2]: mask my500
        [3]: mask my1000f/my1000h
        */
        ASIC_PHY_MDIO_WRITE(phy_dev, 17, 0x0004); /* disable masks for 10,100,1000*/
        ca_mdelay(1);
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a40);
    }

    val16 = ((ca_uint16_t)adv.adv_10base_t_half << PHY_REG_AN_ADVR_BIT_10BT_HD) |
            ((ca_uint16_t)adv.adv_10base_t_full << PHY_REG_AN_ADVR_BIT_10BT_FD) |
            ((ca_uint16_t)adv.adv_100base_tx_half << PHY_REG_AN_ADVR_BIT_100BX_HD) |
            ((ca_uint16_t)adv.adv_100base_tx_full << PHY_REG_AN_ADVR_BIT_100BX_FD) |
            ((ca_uint16_t)adv.adv_100base_t4 << PHY_REG_AN_ADVR_BIT_100BT4_CAP) |
            ((ca_uint16_t)adv.adv_pause << PHY_REG_AN_ADVR_BIT_PAUSE_CAP) |
            ((ca_uint16_t)adv.adv_asym_pause << PHY_REG_AN_ADVR_BIT_PAUSE_ASYM);
   // | ((ca_uint16_t)(adv.adv_1000base_t_full | adv.adv_1000base_t_half) << PHY_REG_AN_ADVR_BIT_NPAGE) ;

    ret = __eth_phy_regfield_mask_write(phy_dev, PHY_REG_AUTONEG_ADV, 0x0fe0, val16);
    //ca_printf("set phy %d reg %d value %#x\r\n", phy_dev, PHY_REG_AUTONEG_ADV, val16);
    //__eth_phy_regfield_read( phy_dev, PHY_REG_AUTONEG_ADV, 0, 16, &val16);
    //ca_printf("get phy %d reg %d value %#x\r\n", phy_dev, PHY_REG_AUTONEG_ADV, val16);
    if (CA_E_OK == ret) {
        /* 1G adv */
        val16 = ((ca_uint16_t)adv.adv_1000base_t_full << PHY_REG_1000BT_CTRL_BIT_ADVR_1000_FD) |
                ((ca_uint16_t)adv.adv_1000base_t_half << PHY_REG_1000BT_CTRL_BIT_ADVR_1000_HD);
        ret = __eth_phy_regfield_mask_write(phy_dev, PHY_REG_1000BASET_CTRL, 0x300, val16);
        //ca_printf("set phy %d reg %d value %#x\r\n", phy_dev, PHY_REG_1000BASET_CTRL, val16);
        //__eth_phy_regfield_read( phy_dev, PHY_REG_1000BASET_CTRL, 0, 16, &val16);
        //ca_printf("get phy %d reg %d value %#x\r\n", phy_dev, PHY_REG_1000BASET_CTRL, val16);
    }

    /* EEE (Energy Efficient Ethernet) support PHY types according to IEEE802.3az:
     * 10BASE-Te   Clause14
     * 100BASE-TX  Clause24
     * 1000BASE-T  Clause40
     * XGXS (XAUI) Clause47
     * 1000BASE-KX Clause70
     * 10GBASE-T   Clause55
     * 10GBASE-KX4 Clause71
     * 10GBASE-KR  Clause72
     */
    if (__PHY_IS_INTERNAL(phy_dev)) {
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a43); /* page A43 */
        ca_mdelay(1);
        val16 = ASIC_PHY_MDIO_READ(phy_dev, 25);
        /* bit4 for 10BASE-Te */
        val16 = (val16 & (~(0x0001<<4))) | (adv.adv_10base_te << 4);
        ASIC_PHY_MDIO_WRITE(phy_dev, 25, val16); 
        ca_mdelay(1);
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a40);/* back to page 0 */
    } else {
        __eth_phy_regfield_read(phy_dev, 0x1a, 0, 16, &val16);
        if ((val16 >> 8) & 0x1) {
        /* NOT found configure adv_10base_te for other RTL8211 PHY */
        } else {
        /* NOT support 10M EEE in current RTL PHY */
        }
    }
    
    __eth_phy_mmd_read(phy_dev, 7, 0x3c, &val16);
    /* bit2 for 1000BASE-T, bit1 for 100BASE-TX */
    val16 = (val16 & (~(0x3<<1))) | (adv.adv_100base_tx_eee << 1) | (adv.adv_1000base_t_eee << 2);
    ret = __eth_phy_mmd_write(phy_dev, 7, 0x3c, val16);

    return ret;
}

static ca_status_t  __eth_phy_ability_adv_get(ca_uint8_t phy_dev, aal_phy_autoneg_adv_t *adv)
{
    ca_uint16 val16 = 0;
    ca_status_t ret = CA_E_OK;

    if (__PHY_IS_INTERNAL(phy_dev)) {
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a47);
        ca_mdelay(1);
        /*mask nway ability
        [0]: mask my10f/my10h
        [1]: mask my100f/my100h
        [2]: mask my500
        [3]: mask my1000f/my1000h
        [4]: mask_eee_ability
        */
        ASIC_PHY_MDIO_WRITE(phy_dev, 17, 0x0004); /* disable masks for 10,100,1000*/
        ca_mdelay(1);
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a40);
    }

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_AUTONEG_ADV, 0, 16, &val16);
    if (CA_E_OK != ret)
        return ret;

    adv->adv_asym_pause       = (val16 >> PHY_REG_AN_ADVR_BIT_PAUSE_ASYM) & 0x1;
    adv->adv_pause            = (val16 >> PHY_REG_AN_ADVR_BIT_PAUSE_CAP) & 0x1;
    adv->adv_100base_t4       = (val16 >> PHY_REG_AN_ADVR_BIT_100BT4_CAP) & 0x1;
    adv->adv_100base_tx_full  = (val16 >> PHY_REG_AN_ADVR_BIT_100BX_FD) & 0x1;
    adv->adv_100base_tx_half  = (val16 >> PHY_REG_AN_ADVR_BIT_100BX_HD) & 0x1;
    adv->adv_10base_t_full    = (val16 >> PHY_REG_AN_ADVR_BIT_10BT_FD) & 0x1;
    adv->adv_10base_t_half    = (val16 >> PHY_REG_AN_ADVR_BIT_10BT_HD) & 0x1;

    val16 = 0;
    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_1000BASET_CTRL, 0, 16, &val16);
    if (CA_E_OK != ret)
        return ret;

    adv->adv_1000base_t_full  = (val16 >> PHY_REG_1000BT_CTRL_BIT_ADVR_1000_FD) & 0x1;
    adv->adv_1000base_t_half  = (val16 >> PHY_REG_1000BT_CTRL_BIT_ADVR_1000_HD) & 0x1;

    if (__PHY_IS_INTERNAL(phy_dev)) {
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a43); /* page select 0xa47 */
        ca_mdelay(1);
        val16 = ASIC_PHY_MDIO_READ(phy_dev, 25);
        /* bit4 FOR 10M_EEE */
        adv->adv_10base_te = (val16 >> 4 ) & 0x1;
        ASIC_PHY_MDIO_WRITE(phy_dev, 25, val16);
        ca_mdelay(1);
        ASIC_PHY_MDIO_WRITE(phy_dev, 31, 0x0a40); /* page select 0xa40 */
    } else {
        __eth_phy_regfield_read(phy_dev, 0x1a, 0, 16, &val16);
        adv->adv_10base_te = (val16 >> 8) & 0x1;
    }

    ret = __eth_phy_mmd_read(phy_dev, 7, 0x3c, &val16);
    adv->adv_100base_tx_eee = (val16 >> 1) & 0x01;
    adv->adv_1000base_t_eee = (val16 >> 2) & 0x01;
    adv->adv_2d5gbase_t_eee = 0;
    adv->adv_5gbase_t_eee   = 0;
    adv->adv_10gbase_t_eee  = 0;

    return ret;
}

static ca_status_t  __eth_phy_partner_ability_adv_get(ca_uint8_t phy_dev, aal_phy_autoneg_adv_t *adv)
{
    ca_uint16 val16 = 0;

    __eth_phy_regfield_read(phy_dev, PHY_REG_LP_ABILITY_BASE, 0, 16, &val16);

    adv->adv_asym_pause       = (val16 >> PHY_REG_AN_ADVR_BIT_PAUSE_ASYM) & 0x1;
    adv->adv_pause            = (val16 >> PHY_REG_AN_ADVR_BIT_PAUSE_CAP) & 0x1;
    adv->adv_100base_t4       = (val16 >> PHY_REG_AN_ADVR_BIT_100BT4_CAP) & 0x1;
    adv->adv_100base_tx_full  = (val16 >> PHY_REG_AN_ADVR_BIT_100BX_FD) & 0x1;
    adv->adv_100base_tx_half  = (val16 >> PHY_REG_AN_ADVR_BIT_100BX_HD) & 0x1;
    adv->adv_10base_t_full    = (val16 >> PHY_REG_AN_ADVR_BIT_10BT_FD) & 0x1;
    adv->adv_10base_t_half    = (val16 >> PHY_REG_AN_ADVR_BIT_10BT_HD) & 0x1;

    return CA_E_OK;

}

static ca_status_t  __eth_phy_link_status_get(ca_uint8_t phy_dev, aal_phy_link_status_t *link_status)
{

    ca_uint16 data16 = 0;
    __eth_phy_regfield_read(phy_dev, PHY_REG_STATUS, 0, 16, &data16);
    __eth_phy_regfield_read(phy_dev, PHY_REG_STATUS, 0, 16, &data16);

    if (data16 & (0x1 << 1)) {
        link_status->link_up = TRUE;
    }
    else {
        link_status->link_up  = FALSE;
        link_status->speed    = AAL_PHY_SPEED_10;
        link_status->duplex   = FALSE;
        return CA_E_OK;
    }

    /* phy's actual speed and duplex can be got from ctrl register directily*/
    data16 = 0;
    __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, 0, 16, &data16);
    if (data16 & (0x1 << PHY_REG_CTRL_BIT_SPEED_SEL_LSB)) {
        link_status->speed = CA_SPEED_100MBPS;
    }
    else {
        link_status->speed = CA_SPEED_10MBPS;
    }

    if (data16 & (0x1 << PHY_REG_CTRL_BIT_DUPLEX)) {
        link_status->duplex = CA_DUPLEX_FULL;
    }
    else {
        link_status->duplex = CA_DUPLEX_HALF;
    }
    return CA_E_OK;
}

static ca_status_t  __eth_phy_speed_set(ca_uint8_t phy_dev, aal_phy_speed_mode_t speed)
{
    ca_status_t ret    = CA_E_OK;
    ca_uint16 data16 = 0, ge_data = 0;
/*
speed selection msb bit
{regspd1(bit6), regspd0(bit13)}:
2'b00: 10Mb/s
2'b01: 100Mb/s
2'b10: 1000Mb/s
2'b11: Reserved

*/
    switch(speed){
        case AAL_PHY_SPEED_10:
            data16 = 0;
            //ge_data = 0x0;
            break;
        case AAL_PHY_SPEED_100:
            data16 = 0x2000;
            //ge_data = 0x0;
            break;
        case AAL_PHY_SPEED_1000:
            data16 = 0x40;
            ge_data = 0x300; /* should enable 1G advertise for 1G FULL*/
            ret |=  __eth_phy_regfield_mask_write(phy_dev, PHY_REG_1000BASET_CTRL,
                0x300, ge_data);
            break;
        default:
            return CA_E_PARAM;
    }

    ret =  __eth_phy_regfield_mask_write(phy_dev, PHY_REG_CTRL,
        0x2040, data16);


    return ret;

}

static ca_status_t  __eth_phy_speed_get(ca_uint8_t phy_dev, aal_phy_speed_mode_t *speed)
{
    ca_uint16  data16 = 0;
    ca_status_t ret   = CA_E_OK;

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, 0, 16, &data16);

    if (CA_E_OK != ret)
        return ret;

    data16 &= 0x2040;
    switch(data16){
        case 0:
            *speed = AAL_PHY_SPEED_10;
            break;
        case 0x2000:
            *speed = AAL_PHY_SPEED_100;
            break;
        case 0x40:
            *speed = AAL_PHY_SPEED_1000;
            break;
        default:
            return CA_E_ERROR;
    }

    return CA_E_OK;
}


static ca_status_t  __eth_phy_duplex_set(ca_uint8_t phy_dev, ca_boolean_t is_full)
{
    return __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_DUPLEX, 1, is_full ? 1 : 0);
}

static ca_status_t  __eth_phy_duplex_get(ca_uint8_t phy_dev, ca_boolean_t *duplex)
{
    ca_uint16 data16 = 0;
    ca_status_t ret  = CA_E_OK;

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_DUPLEX, 1, &data16);
    if (CA_E_OK != ret)
        return ret;

    *duplex = (data16 != 0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_flow_ctrl_set(ca_uint8_t phy_dev, ca_boolean_t enable)
{
    return CA_E_NOT_SUPPORT;
}

static ca_status_t  __eth_phy_flow_ctrl_get(ca_uint8_t phy_dev, ca_boolean_t *enable)
{
    return CA_E_NOT_SUPPORT;
}

static ca_status_t  __eth_phy_mdix_set(ca_uint8_t phy_dev, aal_phy_mdi_mode_t mdix_mode)
{
    return CA_E_NOT_SUPPORT;
}

static ca_status_t  __eth_phy_mdix_get(ca_uint8_t phy_dev, aal_phy_mdi_mode_t *mdix_mode)
{
    return CA_E_NOT_SUPPORT;
}

static ca_status_t  __eth_phy_loopback_set(ca_uint8_t phy_dev, aal_phy_lpbk_mode_t mode)
{
    ca_status_t ret  = CA_E_OK;

    if (mode.rx2tx_enable) {
        return CA_E_PARAM;
    }

    ret = __eth_phy_regfield_write(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_LPBK, 1, mode.tx2rx_enable ? 1 : 0);
    if (CA_E_OK != ret)
        return ret;

    return CA_E_OK;
}

static ca_status_t  __eth_phy_loopback_get(ca_uint8_t phy_dev, aal_phy_lpbk_mode_t *mode)
{
    ca_uint16  data = 0;
    ca_status_t ret  = CA_E_OK;

    ret = __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, PHY_REG_CTRL_BIT_LPBK,  1, &data);
    if (CA_E_OK != ret)
        return ret;

    mode->tx2rx_enable = data;
    mode->rx2tx_enable = 0;

    return CA_E_OK;
}


/**************************************************************/
/*             Private Etherent PHY APIs                      */
/**************************************************************/
/* Fiber Mode and Loopback */
#define __RTL8021F_REG_FM_LPKT                        (28)
#define __RTL8021F_REG_FM_LPKT_BIT_FIBER_MODE         (5)
#define __RTL8021F_REG_FM_LPKT_BIT_AUTO_MDIX          (2)
#define __RTL8021F_REG_FM_LPKT_BIT_FORCE_MDI          (1)

/*Int mask Register */
#define __RTL8201F_REG_INTR_EN                        (0x13)
#define __RTL8201F_REG_INTR_EN_BIT_AN_ERR             (11)
#define __RTL8201F_REG_INTR_EN_BIT_SPEED
#define __RTL8201F_REG_INTR_EN_BIT_DUPLEX             (12)
#define __RTL8201F_REG_INTR_EN_BIT_LINK               (13)

/*MDI mode Register*/
#define __RTL8201F_REG_MDI_MODE                       (0x1c)
#define __RTL8201F_REG_MDI_MODE_BIT_MDI_AUTO          (2)
#define __RTL8201F_REG_MDI_MODE_BIT_MDI_FORCE         (1)

/*Int Status Register*/
#define __RTL8201F_REG_INTR_STATUS                    (0x1e)
#define __RTL8201F_REG_INTR_STATUS_BIT_AN_ERR         (15)
#define __RTL8201F_REG_INTR_STATUS_BIT_SPEED          (14)
#define __RTL8201F_REG_INTR_STATUS_BIT_DUPLEX         (13)
#define __RTL8201F_REG_INTR_STATUS_BIT_LINK           (11)

 /* Page select register */
#define __RTL8201F_REG_PAGE_SEL                       (0x1f)
#define __RTL8201F_REG_PAGE_INTR_EN                   (0x7)

#define  __RTL8201F_POWER_DOWN_CYCLE_TIME                 (10)
#define  __RTL8201F_POWER_UP_CYCLE_TIME                   (500)

static ca_status_t __eth_phy_rtl8201f_init(ca_uint8_t phy_dev)
{
    /*
      * RGMII mode select
      */
    /* REG31 write 0x0007, set to extension page 7*/
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0x0007);

    /* REG16 write 0x1FFB, set an external clock input to CKXTAL2 */
    __eth_phy_regfield_write(phy_dev, 16, 0, 16, 0x7FFB);

    /* REG31 write 0X0000, back to page0 */
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);

    /*
      * LED mode select
      */
    /* REG31 write 0x0007, set to extension page 7*/
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0x0007);

    /* Write MII Phy 2 Reg 19 Data = 0xc03c */
    __eth_phy_regfield_write(phy_dev, 19, 0, 16, 0xc03c);
    __eth_phy_regfield_write(phy_dev, 17, 0, 16, 0x00bb);

    /* REG31 write 0X0000, back to page0 */
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);

    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8201f_int_mask_all(ca_uint8 phy_dev)
{
    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);
    __eth_phy_regfield_write(phy_dev, __RTL8201F_REG_INTR_EN, 0, 16, 0);
    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8201f_int_en_set(ca_uint8 phy_dev, ca_uint16_t data)
{

    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);

    if (data & PHY_INTR_LINK_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }

    if (data & PHY_INTR_DUPLEX_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_DUPLEX,
                           1,
                           1);
    }

    if (data & PHY_INTR_SPEED_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }

    if (data & PHY_INTR_AN_FAIL) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_AN_ERR,
                           1,
                           1);
    }

    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8201f_int_en_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_write(phy_dev, __RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);
    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_EN, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
        *data |= PHY_INTR_SPEED_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }

    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8201f_int_status_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_STATUS, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_SPEED)) {
        *data |= PHY_INTR_SPEED_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }

    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8201f_int_status_clr(ca_uint8 phy_dev)
{
    ca_uint16 data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_STATUS, 0, 16, &data16);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8201f_mdix_set(ca_uint8_t phy_dev, aal_phy_mdi_mode_t mdix_mode)
{
    ca_status_t ret = CA_E_OK;

    if (AAL_PHY_MDI_AUTO == mdix_mode) {
        ret = __eth_phy_regfield_write(phy_dev, __RTL8021F_REG_FM_LPKT, __RTL8021F_REG_FM_LPKT_BIT_AUTO_MDIX, 1, 0x1);
    }
    else if (AAL_PHY_MDI_MDI == mdix_mode) {
        ret = __eth_phy_regfield_mask_write(phy_dev, __RTL8021F_REG_FM_LPKT, 0x6, 0x1);
    }
    else {
        ret = __eth_phy_regfield_mask_write(phy_dev, __RTL8021F_REG_FM_LPKT, 0x6, 0x0);
    }

    return ret;
}

static ca_status_t  __eth_phy_rtl8201f_mdix_get(ca_uint8_t phy_dev, aal_phy_mdi_mode_t *mdix_mode)
{
    ca_uint16 data16 = 0;

    if (CA_E_OK != __eth_phy_regfield_read(phy_dev, __RTL8021F_REG_FM_LPKT, 0, 16, &data16)) {
        return CA_E_INTERNAL;
    }

    if (data16 & (1 << __RTL8021F_REG_FM_LPKT_BIT_AUTO_MDIX)) {
        *mdix_mode = AAL_PHY_MDI_AUTO;
    }
    else if (data16 & (1 << __RTL8021F_REG_FM_LPKT_BIT_FORCE_MDI)){
        *mdix_mode = AAL_PHY_MDI_MDI;
    }
    else {
        *mdix_mode = AAL_PHY_MDI_MDIX;

    }

    return CA_E_NOT_SUPPORT;
}

/* Specific Register */
#define __RTL8211F_REG_SPEC_PHYCR1                   (0x18)
#define __RTL8211F_REG_SPEC_PHYCR2                   (0x19)
#define __RTL8211F_REG_SPEC_PHYSR                    (0x1A)

/*Int mask Register */
#define __RTL8211_REG_INTR_EN                        (0x12)
#define __RTL8211_REG_INTR_EN_BIT_AN_ERR              (0)
#define __RTL8211_REG_INTR_EN_BIT_PAGE_RCVD           (2)
#define __RTL8211_REG_INTR_EN_BIT_AN_DONE             (3)
#define __RTL8211_REG_INTR_EN_BIT_LINK                (4)
#define __RTL8211_REG_INTR_EN_BIT_PME                 (7)
#define __RTL8211_REG_INTR_EN_BIT_ALDPS               (9)
#define __RTL8211_REG_INTR_EN_BIT_JABBER              (10)

/*MDI mode Register*/
#define __RTL8211_REG_MDI_MODE                       (0x18)
#define __RTL8211_REG_MDI_MODE_BIT_MDI_AUTO          (9)
#define __RTL8211_REG_MDI_MODE_BIT_MDI_FORCE         (8)

/*Int Status Register*/
#define __RTL8211_REG_INTR_STATUS                    (0x1d)

#define __RTL8211_REG_INTR_STATUS_BIT_AN_ERR         (0)
#define __RTL8211_REG_INTR_STATUS_BIT_PAGE_RCVD      (2)
#define __RTL8211_REG_INTR_STATUS_BIT_AN_DONE        (3)
#define __RTL8211_REG_INTR_STATUS_BIT_LINK           (4)
#define __RTL8211_REG_INTR_STATUS_BIT_PME            (7)
#define __RTL8211_REG_INTR_STATUS_BIT_ALDPS          (9)
#define __RTL8211_REG_INTR_STATUS_BIT_JABBER         (10)

 /* Page select register */
#define __RTL8211_REG_PAGE_SEL                       (0x1f)
#define __RTL8211_REG_PAGE_INTR_EN                   (0x0)
#define __RTL8211_REG_PAGE_LCR                       (0xd04)
#define __RTL8211_REG_PAGE_INTBCR                    (0xd40)

#define  __RTL8211_POWER_DOWN_CYCLE_TIME              (10)
#define  __RTL8211_POWER_UP_CYCLE_TIME                (500)

static ca_status_t  __eth_phy_rtl8211_int_mask_all(ca_uint8 phy_dev)
{
    __eth_phy_regfield_write(phy_dev,__RTL8211_REG_PAGE_SEL, 0, 16, __RTL8211_REG_PAGE_INTR_EN);
    __eth_phy_regfield_write(phy_dev, __RTL8211_REG_INTR_EN, 0, 16, 0);
    __eth_phy_regfield_write(phy_dev,__RTL8211_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8211_int_en_set(ca_uint8 phy_dev, ca_uint16_t data)
{

    __eth_phy_regfield_write(phy_dev,__RTL8211_REG_PAGE_SEL, 0, 16, __RTL8211_REG_PAGE_INTR_EN);

    if (data & PHY_INTR_LINK_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8211_REG_INTR_EN,
                           __RTL8211_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }
#if 0
    if (data & PHY_INTR_DUPLEX_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8211_REG_INTR_EN,
                           __RTL8211_REG_INTR_EN_BIT_DUPLEX,
                           1,
                           1);
    }

    if (data & PHY_INTR_SPEED_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8211_REG_INTR_EN,
                           __RTL8211_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }
#endif

    if (data & PHY_INTR_AN_FAIL) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8211_REG_INTR_EN,
                           __RTL8211_REG_INTR_EN_BIT_AN_ERR,
                           1,
                           1);
    }

    __eth_phy_regfield_write(phy_dev,__RTL8211_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8211_int_en_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_write(phy_dev, __RTL8211_REG_PAGE_SEL, 0, 16, __RTL8211_REG_PAGE_INTR_EN);
    __eth_phy_regfield_read(phy_dev, __RTL8211_REG_INTR_EN, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8211_REG_INTR_EN_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
        *data |= PHY_INTR_SPEED_CHANGE;
    }
#if 0
    if (data16 & (0x1 << __RTL8211_REG_INTR_EN_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }
#endif
    if (data16 & (0x1 << __RTL8211_REG_INTR_EN_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }

    __eth_phy_regfield_write(phy_dev,__RTL8211_REG_PAGE_SEL, 0, 16, 0);

    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8211_int_status_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8211_REG_INTR_STATUS, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8211_REG_INTR_STATUS_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
    }
#if 0
    if (data16 & (0x1 << __RTL8211_REG_INTR_STATUS_BIT_SPEED)) {
        *data |= PHY_INTR_SPEED_CHANGE;
    }

    if (data16 & (0x1 << __RTL8211_REG_INTR_STATUS_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }
#endif
    if (data16 & (0x1 << __RTL8211_REG_INTR_STATUS_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }

    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8211_int_status_clr(ca_uint8 phy_dev)
{
    ca_uint16 data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8211_REG_INTR_STATUS, 0, 16, &data16);

    return CA_E_OK;
}
static ca_status_t  __eth_phy_rtl8211_link_status_get(ca_uint8_t phy_dev, aal_phy_link_status_t *link_status)
{
        ca_uint16   data16 = 0;
        ca_status_t ret    = CA_E_OK;
        aal_phy_reg_GBCR_t      reg_GBCR_val;
        aal_phy_reg_GBSR_t      reg_GBSR_val;
        aal_phy_reg_ANAR_t      reg_ANAR_val;
        aal_phy_reg_ANLPAR_t    reg_ANLPAR_val;

        ret = __eth_phy_regfield_read(phy_dev, __RTL8211F_REG_SPEC_PHYSR, 0, 16, &data16);
        if (CA_E_OK != ret) {
            return ret;
        }

        /*
         * __RTL8211F_REG_SPEC_PHYSR_BIT_LNK             2
         * __RTL8211F_REG_SPEC_PHYSR_BIT_DUPLEX          3
         * __RTL8211F_REG_SPEC_PHYSR_BIT_SPEED           4,5
         */
                    
        link_status->link_up = (data16 >> 2) & 0x1;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        /* read the H/W status from one register directly for SATURN Lidong@20180525 */
        link_status->duplex  = (data16 >> 3) & 0x1;
        link_status->speed   = (data16 >> 4) & 0x3;
        return CA_E_OK;
#endif

        if (!link_status->link_up) {
                link_status->speed    = AAL_PHY_SPEED_10;
                link_status->duplex   = FALSE;
                return CA_E_OK;
        }

        __eth_phy_regfield_read(phy_dev, PHY_REG_1000BASET_CTRL, 0, 16, (ca_uint16_t *) &reg_GBCR_val.wrd);
        __eth_phy_regfield_read(phy_dev, PHY_REG_1000BASET_STATUS, 0, 16, (ca_uint16_t *) &reg_GBSR_val.wrd);
        //ca_printf("%s: reg_GBCR_val.wrd=0x%x, reg_GBSR_val.wrd=0x%x\n", __func__, reg_GBCR_val.wrd, reg_GBSR_val.wrd);

        /* check 1000Mbps connection */
        if (reg_GBCR_val.bf.adv_1000base_t_full && reg_GBSR_val.bf.lp_1000base_t_full) {
                //ca_printf("%s: Link at 1000Mbps Full Duplex\n", __func__);
                link_status->speed = CA_SPEED_1000MBPS;
                link_status->duplex = CA_DUPLEX_FULL;
                return CA_E_OK;
        }

        __eth_phy_regfield_read(phy_dev, PHY_REG_CTRL, 0, 16, &data16);
        if (!((data16 >> PHY_REG_CTRL_BIT_AN_CTRL) & 0x1)) {/* no autoneg */

            link_status->duplex = (data16 >> PHY_REG_CTRL_BIT_DUPLEX) & 0x1;

            if (0x2000 == (data16 & 0x2040))
                link_status->speed = AAL_PHY_SPEED_100;
            else if (0x40 == (data16 & 0x2040))
                link_status->speed = AAL_PHY_SPEED_1000;
            else
                link_status->speed = AAL_PHY_SPEED_10;

            return CA_E_OK;
        }

        /* autoneg */
        __eth_phy_regfield_read(phy_dev, PHY_REG_AUTONEG_ADV, 0, 16, (ca_uint16_t *) &reg_ANAR_val.wrd);
        __eth_phy_regfield_read(phy_dev, PHY_REG_LP_ABILITY_BASE, 0, 16, (ca_uint16_t *) &reg_ANLPAR_val.wrd);
        //ca_printf("%s: reg_ANAR_val.wrd=0x%x, reg_ANLPAR_val.wrd=0x%x\n", __func__, reg_ANAR_val.wrd, reg_ANLPAR_val.wrd);
        if (reg_ANAR_val.bf.adv_100base_tx_full && reg_ANLPAR_val.bf.adv_100base_tx_full) {
                //ca_printf("%s: Link at 100Mbps Full Duplex\n", __func__);
                link_status->speed = CA_SPEED_100MBPS;
                link_status->duplex = CA_DUPLEX_FULL;
                return CA_E_OK;
        }

        if (reg_ANAR_val.bf.adv_100base_tx_half && reg_ANLPAR_val.bf.adv_100base_tx_half) {
                //ca_printf("%s: Link at 100Mbps Half Duplex\n", __func__);
                link_status->speed = CA_SPEED_100MBPS;
                link_status->duplex = CA_DUPLEX_HALF;
                return CA_E_OK;
                                                                                                      }

        if (reg_ANAR_val.bf.adv_10base_t_full && reg_ANLPAR_val.bf.adv_10base_t_full) {
                //ca_printf("%s: Link at 10Mbps Full Duplex\n", __func__);
                link_status->speed = CA_SPEED_10MBPS;
                link_status->duplex = CA_DUPLEX_FULL;
                return CA_E_OK;
        }

        if (reg_ANAR_val.bf.adv_10base_t_half && reg_ANLPAR_val.bf.adv_10base_t_half) {
                //ca_printf("%s: Link at 10Mbps Half Duplex\n", __func__);
                link_status->speed = CA_SPEED_10MBPS;
                link_status->duplex = CA_DUPLEX_HALF;
                return CA_E_OK;
        }

        return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8211_mdix_set(ca_uint8_t phy_dev, aal_phy_mdi_mode_t mdix_mode)
{
    ca_status_t ret = CA_E_OK;

    if (AAL_PHY_MDI_AUTO == mdix_mode) {
        ret = __eth_phy_regfield_write(phy_dev, __RTL8211_REG_MDI_MODE, __RTL8211_REG_MDI_MODE_BIT_MDI_AUTO, 1, 0);
    }
    else if (AAL_PHY_MDI_MDI == mdix_mode) {
        ret = __eth_phy_regfield_mask_write(phy_dev, __RTL8211_REG_MDI_MODE, 0x300, 0x300);
    }
    else {
        ret = __eth_phy_regfield_mask_write(phy_dev, __RTL8211_REG_MDI_MODE, 0x300, 0x200);
    }

    /* Should perform reset for RTL8211 PHY after change MDIX */
    if (CA_E_OK == ret) {
        __eth_phy_reset(phy_dev);
    }

    return ret;
}

static ca_status_t  __eth_phy_rtl8211_mdix_get(ca_uint8_t phy_dev, aal_phy_mdi_mode_t *mdix_mode)
{
    ca_uint16 data16 = 0;

    if (CA_E_OK != __eth_phy_regfield_read(phy_dev, __RTL8211_REG_MDI_MODE, 0, 16, &data16)) {
        return CA_E_INTERNAL;
    }

    if (data16 & (1 << __RTL8211_REG_MDI_MODE_BIT_MDI_AUTO)) {/* Manual config */
        if (data16 & (1 << __RTL8211_REG_MDI_MODE_BIT_MDI_FORCE))
            *mdix_mode = AAL_PHY_MDI_MDI;
        else
            *mdix_mode = AAL_PHY_MDI_MDIX;
    }
    else {
        *mdix_mode = AAL_PHY_MDI_AUTO;
    }

    return CA_E_OK;
}

//#define __RTL8211E_REG_MDI_MODE                       (0x10)
//#define __RTL8211E_REG_MDI_MODE_BIT_MDI_AUTO          (6)
//#define __RTL8211E_REG_MDI_MODE_BIT_MDI_FORCE         (5)

static ca_status_t __eth_phy_rtl8211_init(ca_uint8_t phy_dev)
{
    ca_status_t ret    = CA_E_OK;

    ret =__eth_phy_rtl8211_int_mask_all(phy_dev);
    if (CA_E_OK != ret) {
        return CA_E_ERROR;
    }

    ret =__eth_phy_rtl8211_int_status_clr(phy_dev);
    if (CA_E_OK != ret) {
        return CA_E_ERROR;
    }

    ret = __eth_phy_rtl8211_int_en_set(phy_dev, PHY_INTR_LINK_CHANGE);
    if (CA_E_OK != ret) {
        return CA_E_ERROR;
    }


    /* select extension page */
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, __RTL8211_REG_PAGE_LCR);

    __eth_phy_regfield_write(phy_dev, 0x10, 0, 16, 0x6251); /* Link Act LED */
    __eth_phy_regfield_write(phy_dev, 0x11, 0, 16, 0x0); /* disable EEE LED */

    /* back to page 0 */
    __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);

    return CA_E_OK;

}


static ca_status_t  __eth_phy_rtl8214_int_mask_all(ca_uint8 phy_dev)
{
/*
    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);
    __eth_phy_regfield_write(phy_dev, __RTL8201F_REG_INTR_EN, 0, 16, 0);
    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);
*/
    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8214_int_en_set(ca_uint8 phy_dev, ca_uint16_t data)
{
/*
    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);

    if (data & PHY_INTR_LINK_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }

    if (data & PHY_INTR_DUPLEX_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_DUPLEX,
                           1,
                           1);
    }

    if (data & PHY_INTR_SPEED_CHANGE) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_LINK,
                           1,
                           1);
    }

    if (data & PHY_INTR_AN_FAIL) {
        __eth_phy_regfield_write(phy_dev,
                           __RTL8201F_REG_INTR_EN,
                           __RTL8201F_REG_INTR_EN_BIT_AN_ERR,
                           1,
                           1);
    }

    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);
*/
    return CA_E_OK;
}

static ca_status_t  __eth_phy_rtl8214_int_en_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    /*
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_write(phy_dev, __RTL8201F_REG_PAGE_SEL, 0, 16, __RTL8201F_REG_PAGE_INTR_EN);
    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_EN, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
        *data |= PHY_INTR_SPEED_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_EN_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }

    __eth_phy_regfield_write(phy_dev,__RTL8201F_REG_PAGE_SEL, 0, 16, 0);
    */
    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8214_int_status_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    /*
    ca_uint16_t data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_STATUS, 0, 16, &data16);

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_LINK)) {
        *data |= PHY_INTR_LINK_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_SPEED)) {
        *data |= PHY_INTR_SPEED_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_DUPLEX)) {
        *data |= PHY_INTR_DUPLEX_CHANGE;
    }

    if (data16 & (0x1 << __RTL8201F_REG_INTR_STATUS_BIT_AN_ERR)) {
        *data |= PHY_INTR_AN_FAIL;
    }
    */
    return CA_E_OK;

}

static ca_status_t  __eth_phy_rtl8214_int_status_clr(ca_uint8 phy_dev)
{
    /*
    ca_uint16 data16 = 0;

    __eth_phy_regfield_read(phy_dev, __RTL8201F_REG_INTR_STATUS, 0, 16, &data16);
    */
    return CA_E_OK;
}

ca_status_t __eth_phy_rtl8214_init(ca_uint8_t phy_dev)
{
    /* From TaiWan team and Realtek
    Reg 29: 0x6602
    Reg 30: 0x84D7
    Reg 29: 0x6601
    Reg 30: 0x0540
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0xF994
    Reg 29: 0x6601
    Reg 30: 0x0541
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0x2DA3
    Reg 29: 0x6601
    Reg 30: 0x0542
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0x3960
    Reg 29: 0x6601
    Reg 30: 0x0543
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0x9728
    Reg 29: 0x6601
    Reg 30: 0x0544
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0xF83F
    Reg 29: 0x6601
    Reg 30: 0x0545
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0x9D85
    Reg 29: 0x6601
    Reg 30: 0x0423
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x6602
    Reg 30: 0xD810
    Reg 29: 0x6601
    Reg 30: 0x0424
    Reg 29: 0x6600
    Reg 30: 0x00C0

    Reg 29: 0x1D11
    Reg 30: 0x1506

    Reg 29: 0x1D12
    Reg 30: 0x0800

    Reg 29: 0x6602
    Reg 30: 0xC3FA
    Reg 29: 0x6601
    Reg 30: 0x002E
    Reg 29: 0x6600
    Reg 30: 0x00C0
    */
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x84D7);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0540);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0xF994);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0541);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x2DA3);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0542);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x3960);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0543);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x9728);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0544);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0xF83F);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0545);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x9D85);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 0 , 0, 16, 0x0423);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0xD810);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0424);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x1D11);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x1506);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x1D12);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x0800);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6602);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0xC3FA);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6601);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x002E);
    __eth_phy_regfield_write(phy_dev, 29, 0, 16, 0x6600);
    __eth_phy_regfield_write(phy_dev, 30, 0, 16, 0x00C0);

    return CA_E_OK;
}
#if 0
/*******************************************************************************/
/*                 Ethernet PHY AQR109(Aquantia NBaseT PHY)                  */
/*            Supports 2.5GBASE-T /1000BASE-T /100BASE-TX                      */
/*******************************************************************************/

ca_status_t __eth_phy_aqr109_init(ca_uint8_t phy_dev)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t  data0 = 0;

    ret = aal_mdio_read(0, 0, ETH_PHY_ADDR_AQR107, 4, 59410, &data0);
    if (CA_E_OK != ret)
        ca_printf("%s(): %d fail, return %d\n", __func__, __LINE__, ret);
    ca_printf(" phy_port_addr %d reg_dev_addr %d addr %d = %#x\n", 
        ETH_PHY_ADDR_AQR107, 4, 59410, data0);

    return CA_E_OK;
}

static ca_status_t  __eth_phy_aqr109_int_mask_all(ca_uint8 phy_dev)
{
    // todo:
    return CA_E_OK;
}

static ca_status_t  __eth_phy_aqr109_int_en_set(ca_uint8 phy_dev, ca_uint16_t data)
{
    // todo:
    return CA_E_OK;
}


static ca_status_t  __eth_phy_aqr109_int_en_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    // todo:
    return CA_E_OK;
}


static ca_status_t  __eth_phy_aqr109_int_status_get(ca_uint8 phy_dev, ca_uint16_t *data)
{
    // todo:
    return CA_E_OK;
}


static ca_status_t  __eth_phy_aqr109_int_status_clr(ca_uint8 phy_dev)
{
    // todo:
    return CA_E_OK;
}
#endif
/*******************************************************************************/
/*                 Ethernet PHY Driver control block                            */
/*******************************************************************************/

phy_dev_t  *p_eth_phy_dev[ETH_PHY_NUM_MAX] = {0};

phy_dev_t __phy_rtl_8201f_dev ={
    "RTL8201F",
    PHY_OUI_RTK,
    PHY_MODEL_RTL8201F,
    {
         __eth_phy_rtl8201f_init,
         __eth_phy_reset,
         __eth_phy_auto_neg_restart,
         __eth_phy_power_set,
         __eth_phy_power_get,
         __eth_phy_auto_neg_set,
         __eth_phy_auto_neg_get,
         __eth_phy_ability_adv_set,
         __eth_phy_ability_adv_get,
         __eth_phy_partner_ability_adv_get,
         __eth_phy_link_status_get,
         __eth_phy_speed_set,
         __eth_phy_speed_get,
         __eth_phy_duplex_set,
         __eth_phy_duplex_get,
         __eth_phy_flow_ctrl_set,
         __eth_phy_flow_ctrl_get,
         __eth_phy_rtl8201f_mdix_set,
         __eth_phy_rtl8201f_mdix_get,
         __eth_phy_loopback_set,
         __eth_phy_loopback_get,
         __eth_phy_rtl8201f_int_mask_all,
         __eth_phy_rtl8201f_int_en_set,
         __eth_phy_rtl8201f_int_en_get,
         __eth_phy_rtl8201f_int_status_get,
         __eth_phy_rtl8201f_int_status_clr,
         __eth_phy_debug_info_get
    }
};

phy_dev_t __phy_rtl_8211_dev ={
    "RTL8211",
    PHY_OUI_RTK,
    PHY_MODEL_RTL8211,
    {
         __eth_phy_rtl8211_init,
         __eth_phy_reset,
         __eth_phy_auto_neg_restart,
         __eth_phy_power_set,
         __eth_phy_power_get,
         __eth_phy_auto_neg_set,
         __eth_phy_auto_neg_get,
         __eth_phy_ability_adv_set,
         __eth_phy_ability_adv_get,
         __eth_phy_partner_ability_adv_get,
         __eth_phy_rtl8211_link_status_get,
         __eth_phy_speed_set,
         __eth_phy_speed_get,
         __eth_phy_duplex_set,
         __eth_phy_duplex_get,
         __eth_phy_flow_ctrl_set,
         __eth_phy_flow_ctrl_get,
         __eth_phy_rtl8211_mdix_set,
         __eth_phy_rtl8211_mdix_get,
         __eth_phy_loopback_set,
         __eth_phy_loopback_get,
         __eth_phy_rtl8211_int_mask_all,
         __eth_phy_rtl8211_int_en_set,
         __eth_phy_rtl8211_int_en_get,
         __eth_phy_rtl8211_int_status_get,
         __eth_phy_rtl8211_int_status_clr,
         __eth_phy_debug_info_get
    }
};
phy_dev_t __phy_rtl_8214_dev ={
    "RTL8214",
    PHY_OUI_RTK,
    PHY_MODEL_RTL8214,
    {
         __eth_phy_rtl8214_init,
         __eth_phy_reset,
         __eth_phy_auto_neg_restart,
         __eth_phy_power_set,
         __eth_phy_power_get,
         __eth_phy_auto_neg_set,
         __eth_phy_auto_neg_get,
         __eth_phy_ability_adv_set,
         __eth_phy_ability_adv_get,
         __eth_phy_partner_ability_adv_get,
         __eth_phy_link_status_get,
         __eth_phy_speed_set,
         __eth_phy_speed_get,
         __eth_phy_duplex_set,
         __eth_phy_duplex_get,
         __eth_phy_flow_ctrl_set,
         __eth_phy_flow_ctrl_get,
         __eth_phy_mdix_set,
         __eth_phy_mdix_get,
         __eth_phy_loopback_set,
         __eth_phy_loopback_get,
         __eth_phy_rtl8214_int_mask_all,
         __eth_phy_rtl8214_int_en_set,
         __eth_phy_rtl8214_int_en_get,
         __eth_phy_rtl8214_int_status_get,
         __eth_phy_rtl8214_int_status_clr,
         __eth_phy_debug_info_get
    }
};

#if 0
phy_dev_t __phy_aquantia_aqr109_dev ={
    "AQR109",
    0x300EE3,
    0x000000,
    {
         __eth_phy_aqr109_init,
         __eth_phy_reset,
         __eth_phy_auto_neg_restart,
         __eth_phy_power_set,
         __eth_phy_power_get,
         __eth_phy_auto_neg_set,
         __eth_phy_auto_neg_get,
         __eth_phy_ability_adv_set,
         __eth_phy_ability_adv_get,
         __eth_phy_partner_ability_adv_get,
         __eth_phy_link_status_get,
         __eth_phy_speed_set,
         __eth_phy_speed_get,
         __eth_phy_duplex_set,
         __eth_phy_duplex_get,
         __eth_phy_flow_ctrl_set,
         __eth_phy_flow_ctrl_get,
         __eth_phy_mdix_set,
         __eth_phy_mdix_get,
         __eth_phy_loopback_set,
         __eth_phy_loopback_get,
         __eth_phy_aqr109_int_mask_all,
         __eth_phy_aqr109_int_en_set,
         __eth_phy_aqr109_int_en_get,
         __eth_phy_aqr109_int_status_get,
         __eth_phy_aqr109_int_status_clr
    }
};
#endif

/**************************************************************/
/*             Export Etherent PHY APIs                       */
/**************************************************************/
ca_status_t aal_eth_phy_init(ca_device_id_t  device_id, ca_uint8_t phy_addr)
{
    ca_uint16_t  phy_id1   = 0;
    ca_uint16_t  phy_id2   = 0;
    ca_uint16_t  phy_oui   = 0;
    ca_uint16_t  phy_model = 0;
    ca_status_t  ret       = CA_E_OK;
    static ca_boolean_t mdio_init = 0;

    if (phy_addr >= ETH_PHY_NUM_MAX) {
        return CA_E_PARAM;
    }

    if (mdio_init == 0) {
        (void)aal_mdio_speed_set(device_id, phy_addr, __MDIO_CLOCK_DEF);
        mdio_init = 1;
        ca_mdelay(1);
    }

    if (__PHY_IS_XGPHY(phy_addr)) { /* Clause45 MDIO access */
        ret = __eth_phy_clause45_regfield_read(phy_addr, 1, PHY_REG_ID_1, 0, 16, &phy_id1);
        if (ret != CA_E_OK)
            return CA_E_ERROR;

        ret = __eth_phy_clause45_regfield_read(phy_addr, 1, PHY_REG_ID_2, 0, 16, &phy_id2);
        if (ret != CA_E_OK)
            return CA_E_ERROR;
    } else {/* Clause22 MDIO access */
        ret = __eth_phy_regfield_read(phy_addr, PHY_REG_ID_1, 0, 16, &phy_id1);
        if (ret != CA_E_OK)
            return CA_E_ERROR;

        ret = __eth_phy_regfield_read(phy_addr, PHY_REG_ID_2, 0, 16, &phy_id2);
        if (ret != CA_E_OK)
            return CA_E_ERROR;
    }

    if ((phy_id1 == 0xFFFF) && (phy_id2 == 0xFFFF))
        return CA_E_NOT_FOUND;

    phy_oui   = (((phy_id1 & 0x3ff) << 6)|((phy_id2 & 0xfc00) >> 10));
    phy_model = ((phy_id2 & 0x03f0) >> 4);

    ca_printf("ca-ne: read Ethernet PHY %u IDs(%#x %#x): phy_oui %#x, phy_model %#x .\n",
        phy_addr, phy_id1, phy_id2, phy_oui, phy_model);

    if(phy_oui == PHY_OUI_RTK) { /* RealTek Ethernet PHY */
        switch (phy_model) {
            case PHY_MODEL_RTL8201F:
                p_eth_phy_dev[phy_addr] = &__phy_rtl_8201f_dev;
                ca_printf("ca-ne: detect Eth PHY RTL8201F at PHY addr %u\n", phy_addr);
                break;

            case PHY_MODEL_RTL8211:
            case PHY_MODEL_RTL8211E:
            case PHY_MODEL_RTL8211_ASIC:
                p_eth_phy_dev[phy_addr] = &__phy_rtl_8211_dev;
                ca_printf("ca-ne: detect Eth PHY RTL8211 at PHY addr %u\n", phy_addr);
                break;
            case PHY_MODEL_RTL8198F:
                p_eth_phy_dev[phy_addr] = &__phy_rtl_8211_dev;
                ca_printf("ca-ne: detect Embedded FE PHY RTL8198F at PHY addr %u\n", phy_addr);
                break;
            case PHY_MODEL_RTL8214:
            case PHY_MODEL_RTL8214C:
                p_eth_phy_dev[phy_addr] = &__phy_rtl_8214_dev;
                ca_printf("ca-ne: detect Eth PHY RTL8214(C) at PHY addr %u\n", phy_addr);
                break;

            default:
                p_eth_phy_dev[phy_addr] = NULL;
                ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
                return CA_E_NOT_SUPPORT;
        }
	} else if ( phy_oui == PHY_OUI_MVL ) { /* Marvel 10G Ethernet PHY */
        switch (phy_model) {
            case PHY_MODEL_MVL88X3310:
			if ( cap_phy_drv_get(device_id,CA_PHY_TYPE_0,&p_eth_phy_dev[phy_addr]) ) {
                    p_eth_phy_dev[phy_addr] = NULL;
                    ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
                    return CA_E_NOT_SUPPORT;
                }
				ca_printf("ca-ne: detect Eth PHY MVL88x3310 at PHY addr %u\n", phy_addr);
                break;
            default:
                p_eth_phy_dev[phy_addr] = NULL;
                ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
                return CA_E_NOT_SUPPORT;
        }
	} else if ( phy_oui == PHY_OUI_AQR ) { /* Aquantia 10G Ethernet PHY */
		switch (phy_model) {
			case PHY_MODEL_AQR107:
			if ( cap_phy_drv_get(device_id,CA_PHY_TYPE_1,(void**)&p_eth_phy_dev[phy_addr]) ) {
                    p_eth_phy_dev[phy_addr] = NULL;
                    ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
                    return CA_E_NOT_SUPPORT;
                }
				ca_printf("ca-ne: detect Eth PHY AQR107 at PHY addr %u\n", phy_addr);
                break;
			default:
				p_eth_phy_dev[phy_addr] = NULL;
				ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
				return CA_E_NOT_SUPPORT;
		}
	} else {
        ca_printf("ca-ne: !!!No driver support for PHY at addr %u\n", phy_addr);
        return CA_E_NOT_SUPPORT;
    }

    if (p_eth_phy_dev[phy_addr] && p_eth_phy_dev[phy_addr]->drv.init)
        p_eth_phy_dev[phy_addr]->drv.init(phy_addr);

    return CA_E_OK;

}

//#ifndef CONFIG_CORTINA_BOARD_FPGA

#if defined(CONFIG_ARCH_CORTINA_VENUS)
#define __FOR_ALL_INTERNAL_PHY_DO(phy_addr) \
    for (phy_addr = 4; phy_addr > 0; phy_addr--)
#else
#define __FOR_ALL_INTERNAL_PHY_DO(phy_addr) \
    for (phy_addr = 1; phy_addr < 1; phy_addr++)
#endif

ca_status_t aal_internal_phy_init(ca_device_id_t  device_id)
{
    ca_uint8_t  phy_addr = 0;
    ca_uint16_t data     = 0;

    ca_printf("ca-ne: init intergrated PHY start\r\n");

    __FOR_ALL_INTERNAL_PHY_DO(phy_addr) {
        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0x0c40);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 17, 0x0480);
        data = ASIC_PHY_MDIO_READ(phy_addr, 17);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 17 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0x0BC6);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 16, 0x0053);
        data = ASIC_PHY_MDIO_READ(phy_addr, 16);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 16 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 18, 0x4003);
        data = ASIC_PHY_MDIO_READ(phy_addr, 18);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 18 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 22, 0x7e01);
        data = ASIC_PHY_MDIO_READ(phy_addr, 22);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 22 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 23, 0x0000);
        data = ASIC_PHY_MDIO_READ(phy_addr, 23);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 23 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0x0bc0);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 19, 0x01c0);
        data = ASIC_PHY_MDIO_READ(phy_addr, 19);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 19 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 20, 0x3c61);
        data = ASIC_PHY_MDIO_READ(phy_addr, 20);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 20= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 21, 0x8022);
        data = ASIC_PHY_MDIO_READ(phy_addr, 21);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 21= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 23, 0x0441);
        data = ASIC_PHY_MDIO_READ(phy_addr, 23);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 23= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0x0bc1);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 16, 0x1328);
        data = ASIC_PHY_MDIO_READ(phy_addr, 16);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 16 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 17, 0xf0b0);
        data = ASIC_PHY_MDIO_READ(phy_addr, 17);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 17= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 18, 0x0e00);
        data = ASIC_PHY_MDIO_READ(phy_addr, 18);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 18= %#x\r\n", phy_addr, data);
		
        ASIC_PHY_MDIO_WRITE(phy_addr,31,0x0c40);
        data = ASIC_PHY_MDIO_READ(phy_addr,31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n",phy_addr,data);
        data = ASIC_PHY_MDIO_READ(phy_addr,18);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 18 = %#x\r\n",phy_addr,data);

        ASIC_PHY_MDIO_WRITE(phy_addr,31,0x0b90);
        data = ASIC_PHY_MDIO_READ(phy_addr,31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n",phy_addr,data);
        data = ASIC_PHY_MDIO_READ(phy_addr,19);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 19 = %#x\r\n",phy_addr,data);

        ASIC_PHY_MDIO_WRITE(phy_addr,31,0x0b90);
        data = ASIC_PHY_MDIO_READ(phy_addr,31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n",phy_addr,data);
        data = ASIC_PHY_MDIO_READ(phy_addr,19);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 19 = %#x\r\n",phy_addr,data);

        ASIC_PHY_MDIO_WRITE(phy_addr,31,0x0c40);
        data = ASIC_PHY_MDIO_READ(phy_addr,31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n",phy_addr,data);
        data = ASIC_PHY_MDIO_READ(phy_addr,17);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 17 = %#x\r\n",phy_addr,data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0x0A42);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 31 = %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 0, 0x9140);
        data = ASIC_PHY_MDIO_READ(phy_addr, 0);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

#ifdef CONFIG_LUNA_G3_SERIES
        /* Temporary PHY calibration data, should be removed after OPT being writen */
        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xbcd);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 23, 0xbbbb);
        data = ASIC_PHY_MDIO_READ(phy_addr, 23);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 22, 0xbbbb);
        data = ASIC_PHY_MDIO_READ(phy_addr, 22);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xbce);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 16, 0x7777);
        data = ASIC_PHY_MDIO_READ(phy_addr, 16);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 17, 0x7777);
        data = ASIC_PHY_MDIO_READ(phy_addr, 17);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xbca);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 22, 0xbbbb);
        data = ASIC_PHY_MDIO_READ(phy_addr, 22);
        ca_printf("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xbce);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        ca_printf("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr, 16, 0x7777);
        data = ASIC_PHY_MDIO_READ(phy_addr, 16);
        ca_printf("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr, 17, 0x7777);
        data = ASIC_PHY_MDIO_READ(phy_addr, 17);
        ca_printf("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xbca);
        data = ASIC_PHY_MDIO_READ(phy_addr, 31);
        ca_printf("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr, 22, 0xeeee);
        data = ASIC_PHY_MDIO_READ(phy_addr, 22);
        __MDIO_DBG("ca-ne: intergrated phy %d reg 0= %#x\r\n", phy_addr, data);
#endif

#if 0
        ca_printf("ca-ne: intergrated org phy 24(0xa43) reg 24 = %#x\r\n", data);
        data &= ~( 1 << 2 );
        ASIC_PHY_MDIO_WRITE(phy_addr, 24, data);/* page 0a40 */
        data = ASIC_PHY_MDIO_READ(phy_addr, 24);
        ca_printf("ca-ne: intergrated phy 24(0xa43) reg 24 = %#x\r\n", data);
        ca_mdelay(1);

        ASIC_PHY_MDIO_WRITE(phy_addr,  0, 0x1140);
        data = ASIC_PHY_MDIO_READ(phy_addr, 0);
        ca_printf("ca-ne: intergrated phy %d reg 0 = %#x\r\n", phy_addr, data);
        ca_mdelay(1);
#endif
    }

    __DELAY_MS(100);

#ifdef CONFIG_LUNA_G3_SERIES
    /*  For the GPHY work around, please check the following GPHY register (apply to all 4 ports), 
        page 0xb90, rg19, you should see 0x10, this bit will be automatically cleared upon the read,
    */
    __FOR_ALL_INTERNAL_PHY_DO(phy_addr) {
        /* Check clk_fail_sys */
        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xC40);
        data = ASIC_PHY_MDIO_READ(phy_addr, 18);
        ca_printf("%s: phy_addr=%d, read page 0xC40 register 18, value=0x%x\n", __func__, phy_addr,data);

        ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xB90);
        data = ASIC_PHY_MDIO_READ(phy_addr, 19);
        ca_printf("%s: phy_addr=%d, read page 0xB90 register 19, value=0x%x\n", __func__, phy_addr,data);
    }
#endif
    /* Recover to Page0 */
    ASIC_PHY_MDIO_WRITE(phy_addr, 31, 0xa40);
    ca_printf("ca-ne: init intergrated PHY OK\r\n");
    return CA_E_OK;
}

//#endif /* CONFIG_CORTINA_BOARD_FPGA */

ca_status_t  aal_eth_phy_reset(
		ca_device_id_t  device_id,
    ca_uint8_t   phy_addr)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.reset(phy_addr);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_reset_get(
        ca_device_id_t  device_id,
        ca_uint8_t   phy_addr)
{
    return CA_E_OK;
}

ca_status_t  aal_eth_phy_reset_set(
        ca_device_id_t  device_id,
        ca_uint8_t   phy_addr)
{
    return aal_eth_phy_reset(device_id, phy_addr);
}

ca_status_t  aal_eth_phy_auto_neg_restart(
    ca_device_id_t  device_id,
    ca_uint8_t   phy_addr)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.auto_neg_restart(phy_addr);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_power_set(
    ca_device_id_t  device_id,
    ca_uint8_t   phy_addr,
    ca_boolean_t power_up)
{
    ca_status_t ret = CA_E_OK;

    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        ret = p_eth_phy_dev[phy_addr]->drv.power_set(phy_addr, power_up);
    else
        return CA_E_NOT_SUPPORT;


    if (power_up) {
        __DELAY_MS(100);
    } else {
        __DELAY_MS(50);
    }

    return ret;
}

ca_status_t  aal_eth_phy_power_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t *power_up)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.power_get(phy_addr, power_up);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_auto_neg_set(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t  enable)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.auto_neg_set(phy_addr, enable);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_auto_neg_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t *enable)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.auto_neg_get(phy_addr, enable);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_ability_adv_set(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_autoneg_adv_t adv)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.ability_adv_set(phy_addr, adv);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_ability_adv_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_autoneg_adv_t *adv)
{

    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.ability_adv_get(phy_addr, adv);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_partner_ability_adv_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_autoneg_adv_t *adv)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.partner_ability_adv_get(phy_addr, adv);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_link_status_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_link_status_t *link_status)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.link_status_get(phy_addr, link_status);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_speed_set(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_speed_mode_t speed)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.speed_set(phy_addr, speed);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_speed_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    aal_phy_speed_mode_t *speed)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.speed_get(phy_addr, speed);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_duplex_set(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t  is_full)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.duplex_set(phy_addr, is_full);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_duplex_get(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t *is_full)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.duplex_get(phy_addr, is_full);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_flow_ctrl_set(
    ca_device_id_t   device_id,
    ca_uint8_t    phy_addr,
    ca_boolean_t  enable)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.flow_ctrl_set(phy_addr, enable);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_flow_ctrl_get(
    ca_device_id_t    device_id,
    ca_uint8_t     phy_addr,
    ca_boolean_t  *enable)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.flow_ctrl_get(phy_addr, enable);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_mdix_set(
    ca_device_id_t    device_id,
    ca_uint8_t     phy_addr,
    aal_phy_mdi_mode_t mdix_mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.mdix_set(phy_addr, mdix_mode);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_mdix_get(
    ca_device_id_t    device_id,
    ca_uint8_t     phy_addr,
    aal_phy_mdi_mode_t *mdix_mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.mdix_get(phy_addr, mdix_mode);

    return CA_E_NOT_SUPPORT;
}


ca_status_t  aal_eth_phy_loopback_set(
    ca_device_id_t    device_id,
    ca_uint8_t     phy_addr,
    aal_phy_lpbk_mode_t mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.loopback_set(phy_addr, mode);

    return CA_E_NOT_SUPPORT;
}

ca_status_t  aal_eth_phy_loopback_get(
    ca_device_id_t       device_id,
    ca_uint8_t           phy_addr,
    aal_phy_lpbk_mode_t *mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr])
        return p_eth_phy_dev[phy_addr]->drv.loopback_get(phy_addr, mode);

    return CA_E_NOT_SUPPORT;
}

ca_status_t aal_eth_phy_name_get(    
    ca_device_id_t    device_id,
    ca_uint8_t        phy_addr,
    char             *name)
{
    if (name != NULL && phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr]) {
        strncpy(name, p_eth_phy_dev[phy_addr]->name, CA_AAL_PHY_NAME_LEN_MAX - 10);
        if (__PHY_IS_INTERNAL(phy_addr)) {
            strncat(name, "-Internal", 9);
        }
        return CA_E_OK;
    }
    
    return CA_E_NOT_SUPPORT;
}

ca_status_t 
aal_eth_phy_oui_get ( 
     CA_IN ca_device_id_t    device_id,
     CA_IN ca_port_id_t      aal_port,
     CA_OUT ca_uint32_t       *oui )
{
     ca_uint32_t port_id = PORT_ID(aal_port);
     phy_dev_t*  phy_dev = p_eth_phy_dev[g_phy_addr[ port_id ]];

     if ( oui && port_id < CA_NI_TOTAL_NI_PORT && phy_dev ) {
             *oui = phy_dev->oui;
             return CA_E_OK;
     }

     return CA_E_NOT_SUPPORT;
}

char *aal_eth_phy_speed_str_get(
    ca_device_id_t       device_id,
    aal_phy_speed_mode_t  phy_speed)
{
    switch(phy_speed) {
    case AAL_PHY_SPEED_10   :
        return "10M";
    case AAL_PHY_SPEED_100  :
        return "100M";
    case AAL_PHY_SPEED_1000 :
        return "1G";
    case AAL_PHY_SPEED_10000:
        return "10G";
    case AAL_PHY_SPEED_2000 :
        return "2G";
    case AAL_PHY_SPEED_2500 :
        return "2.5G";
    case AAL_PHY_SPEED_5000 :
        return "5G";
    default :
        return "Unkown";
    }
}

ca_status_t aal_eth_phy_debug_info_get(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr] && p_eth_phy_dev[phy_addr]->drv.debug_info_get)
        return p_eth_phy_dev[phy_addr]->drv.debug_info_get(phy_addr);
    return CA_E_OK;
}

ca_status_t aal_eth_phy_fw_upgrade_set(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr,
    CA_IN   aal_phy_boot_mode_t  boot_mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr] && p_eth_phy_dev[phy_addr]->drv.fw_upgrade_set)
        return p_eth_phy_dev[phy_addr]->drv.fw_upgrade_set(phy_addr, boot_mode);
    return CA_E_OK;
}

ca_status_t aal_eth_phy_fw_upgrade_get(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr,
    CA_OUT  aal_phy_boot_mode_t  *boot_mode)
{
    if (phy_addr < ETH_PHY_NUM_MAX && p_eth_phy_dev[phy_addr] && p_eth_phy_dev[phy_addr]->drv.fw_upgrade_get)
        return p_eth_phy_dev[phy_addr]->drv.fw_upgrade_get(phy_addr, boot_mode);
    return CA_E_OK;
}


#if defined(CONFIG_NE_PHY_CALIBRATION)
#include <linux/nvmem-consumer.h>
#include <soc/cortina/ca_efuse_cfg.h>

#define PHY_K_PORT_NUM 4

int phy_cal_load(struct device *dev)
{
    struct nvmem_cell *cell = NULL;
    ca_uint32_t *buf = NULL;
    size_t len = 0;
    int i = 0, ret = 0;
    ca_uint8_t  phy_dev = 0;
    ca_phy_param_t phy_param;
    ca_phy_k_t phy_k_data[PHY_K_PORT_NUM];
    ca_uint16_t data;

    cell = nvmem_cell_get(dev, "param");
    ca_printf("%s: param dev=%p cell=%p\n", __func__, dev, cell);
    if (IS_ERR(cell)) {
        cell = nvmem_cell_get(dev, "phy_param");
        ca_printf("%s: phy_param dev=%p cell=%p\n", __func__, dev, cell);
    }
    if (IS_ERR(cell)) {
        if (PTR_ERR(cell) == -EPROBE_DEFER) {
            ca_printf("%s: line %d invalid cell=%p\n", __func__, __LINE__, cell);
            return PTR_ERR(cell);
        }
        return 0;
    }

    buf = (ca_uint32_t *)nvmem_cell_read(cell, &len);
    ca_printf("%s: buf=%p, len=%d, sizeof(phy_k_data)=%d\n", __func__, buf, len, sizeof(phy_k_data));
    if (IS_ERR(buf)) {
        ca_printf("%s: invalid buf=%p\n", __func__, buf);
        return PTR_ERR(buf);
    }

    if (len < sizeof(phy_param)) {
        dev_warn(dev, "invalid calibration data for phy_param\n");
        ret = -EINVAL;
        goto __out;
    }

    nvmem_cell_put(cell);
    ca_printf("%s: read phy_param buf=%p len=%d\n", __func__, buf, len);
    memcpy(&phy_param, buf, len);
    if(buf)
        kfree(buf);

    ///TODO: debug
    if ((phy_param.sw_patch != 0) || (phy_param.gphy_cal_cmplt != 1)) {
        ca_printf("%s: !!!error: phy_param.sw_patch=%d, phy_param.gphy_cal_cmplt=%d\n", __func__, phy_param.sw_patch, phy_param.gphy_cal_cmplt);
        return 0;
    }

    cell = nvmem_cell_get(dev, "calibration");
    ca_printf("%s: calibration dev=%p cell=%p\n", __func__, dev, cell);
    if (IS_ERR(cell)) {
        cell = nvmem_cell_get(dev, "calib");
        ca_printf("%s: calib dev=%p cell=%p\n", __func__, dev, cell);
    }
    if (IS_ERR(cell)) {
        if (PTR_ERR(cell) == -EPROBE_DEFER) {
            ca_printf("%s: line %d invalid cell=%p\n", __func__, __LINE__, cell);
            return PTR_ERR(cell);
        }
        return 0;
    }

    len = 0;
    buf = (ca_uint32_t *)nvmem_cell_read(cell, &len);
    if (IS_ERR(buf)) {
        ca_printf("%s: line %d invalid buf=%p\n", __func__, __LINE__, buf);
        return PTR_ERR(buf);
    }

    if (len < sizeof(phy_k_data)) {
        dev_warn(dev, "invalid calibration data for phy_k_data\n");
        ret = -EINVAL;
        goto __out;
    }

    nvmem_cell_put(cell);
    ca_printf("%s: read phy_k_data buf=%p len=%d\n", __func__, buf, len);

    memcpy(phy_k_data, buf, len > sizeof(phy_k_data) ? sizeof(phy_k_data) : len);
    if (buf)
        kfree(buf);

    for (i = 0; i < PHY_K_PORT_NUM; i++) {
        ca_printf("rc_cal_len(%d): 0x%04x\n", i, phy_k_data[i].rc_cal_len);
        ca_printf("amp_cal(%d):    0x%04x\n", i, phy_k_data[i].amp_cal);
        ca_printf("adc0_cal(%d):   0x%04x\n", i, phy_k_data[i].adc0_cal);
        ca_printf("r_cal(%d):      0x%04x\n", i, phy_k_data[i].r_cal);
        ca_printf("reserved(%d):   0x%04x\n", i, phy_k_data[i].reserved);
    }

    for (i = 0; i < PHY_K_PORT_NUM; i++) {
        ca_uint16_t adoadj, rlen_val, tapbinrx_pm, ibadj;
        
        phy_dev = __INTERNAL_PHY_MDIO_START + i;
	/* disable AUTO-R-CODE fine tune */
	__eth_phy_regfield_write(phy_dev, 27, 0, 16, 0x8011);
	__eth_phy_regfield_read(phy_dev, 28, 0, 16, &data);
	data &= 0xf7ff;
	__eth_phy_regfield_write(phy_dev, 27, 0, 16, 0x8011);
	__eth_phy_regfield_write(phy_dev, 28, 0, 16, data);
        if (phy_param.sw_patch)
             continue;

        ca_printf("start to write eFuse data to port %d phy_addr %d\n", i, phy_dev);

        if (phy_param.en_ado_cal) {
             adoadj = phy_k_data[i].adc0_cal;
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0xbcf);
             __eth_phy_regfield_write(phy_dev, 22, 0, 16, adoadj);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        }

        if (phy_param.en_rc_cal) {
             rlen_val = phy_k_data[i].rc_cal_len;
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0xbcd);
             __eth_phy_regfield_write(phy_dev, 22, 0, 16, rlen_val);
             __eth_phy_regfield_write(phy_dev, 23, 0, 16, rlen_val);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        }

        if (phy_param.en_r_cal) {
             tapbinrx_pm = phy_k_data[i].r_cal;
             tapbinrx_pm |= (tapbinrx_pm << 4) | (tapbinrx_pm << 8) | (tapbinrx_pm << 12);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0xbce);
             __eth_phy_regfield_write(phy_dev, 16, 0, 16, tapbinrx_pm);
             __eth_phy_regfield_write(phy_dev, 17, 0, 16, tapbinrx_pm);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        }

        if (phy_param.en_amp_cal) {
             ibadj = phy_k_data[i].amp_cal;
             //ibadj = ibadj | (ibadj << 4) | (ibadj << 8) | (ibadj << 12);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0x0bca);
             __eth_phy_regfield_write(phy_dev, 22, 0, 16, ibadj);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        }

        if (phy_param.disable_500m) {
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0xa4a);
             __eth_phy_regfield_write(phy_dev, 17, 8, 1, 0);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        } else {
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0xa4a);
             __eth_phy_regfield_write(phy_dev, 17, 8, 1, 1);
             __eth_phy_regfield_write(phy_dev, 31, 0, 16, 0);
        }
    }
__out:
    ca_printf("ca-ne: %s return %d\n", __func__, ret);

    return ret;
}
#endif


#ifdef __KERNEL__

EXPORT_SYMBOL(aal_mdio_speed_set);
EXPORT_SYMBOL(aal_mdio_speed_get);
EXPORT_SYMBOL(aal_mdio_set);
EXPORT_SYMBOL(aal_mdio_get);
EXPORT_SYMBOL(aal_eth_phy_reset);
EXPORT_SYMBOL(aal_eth_phy_auto_neg_restart);
EXPORT_SYMBOL(aal_eth_phy_power_set);
EXPORT_SYMBOL(aal_eth_phy_power_get);
EXPORT_SYMBOL(aal_eth_phy_auto_neg_set);
EXPORT_SYMBOL(aal_eth_phy_auto_neg_get);
EXPORT_SYMBOL(aal_eth_phy_ability_adv_set);
EXPORT_SYMBOL(aal_eth_phy_ability_adv_get);
EXPORT_SYMBOL(aal_eth_phy_partner_ability_adv_get);
EXPORT_SYMBOL(aal_eth_phy_link_status_get);
EXPORT_SYMBOL(aal_eth_phy_speed_set);
EXPORT_SYMBOL(aal_eth_phy_speed_get);
EXPORT_SYMBOL(aal_eth_phy_duplex_set);
EXPORT_SYMBOL(aal_eth_phy_duplex_get);
EXPORT_SYMBOL(aal_eth_phy_loopback_set);
EXPORT_SYMBOL(aal_eth_phy_loopback_get);
EXPORT_SYMBOL(aal_eth_phy_mdix_get);
EXPORT_SYMBOL(aal_eth_phy_mdix_set);
EXPORT_SYMBOL(aal_eth_phy_name_get);
EXPORT_SYMBOL(aal_eth_phy_speed_str_get);
EXPORT_SYMBOL(aal_internal_phy_init);
EXPORT_SYMBOL(aal_eth_phy_debug_info_get);
EXPORT_SYMBOL(aal_eth_phy_fw_upgrade_set);
EXPORT_SYMBOL(aal_eth_phy_fw_upgrade_get);
EXPORT_SYMBOL(aal_eth_phy_reset_get);
EXPORT_SYMBOL(aal_eth_phy_reset_set);

#endif

