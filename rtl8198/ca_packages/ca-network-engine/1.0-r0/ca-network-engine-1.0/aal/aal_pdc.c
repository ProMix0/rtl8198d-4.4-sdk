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
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "ca-scfg/scfg.h"
#include "aal_pon.h"
#include "aal_pdc.h"
#include "aal_xgpon.h"
#include "aal_mpcp.h"



/******************************************************************************/
/*                             Import Variables and Functions                 */
/******************************************************************************/


/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/
#define __PDC_LOCK()    \
    do{ \
        if (__pdc_lock) \
            ca_spin_lock(__pdc_lock);\
    }while(0)

#define __PDC_UNLOCK()    \
    do{ \
        if (__pdc_lock) \
            ca_spin_unlock(__pdc_lock); \
    }while(0)

#define PDC_DEBUG(arg,...)  \
    do { \
        if (aal_pdc_debug)\
        ca_printf("%s:%d "arg, __func__,__LINE__,##__VA_ARGS__);\
    }while(0)

#define FOR_ALL_PDC_MAP_ENTRY(i) \
    for ((i) = 0; (i) < AAL_PDC_MAP_ENTRY_NUM; (i)++)
        
/******************************************************************************/
/*                             Local global Var                               */
/******************************************************************************/
static ca_uint32_t aal_pdc_debug = 0;
static ca_uint    __pdc_lock = 0;
static ca_uint8_t __gpon_pdc_spid_gem_index_map[32] = {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/******************************************************************************/
/*                             Local function                                 */
/******************************************************************************/

static ca_status_t __pdc_gpon_family_init(ca_device_id_t        device_id)
{
    aal_pdc_pdc_map_mem_data_mask_t pdc_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_cfg;
    aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
    aal_pdc_ctrl_t                  pdc_ctrl;
    ca_uint32_t                     gem_port = 0;
    ca_status_t                     ret = CA_E_OK;


    memset(&pdc_cfg, 0, sizeof(pdc_cfg));
    pdc_mask.wrd = ~0;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    pdc_cfg.ldpid     = 5; /* since not bypass FE, so this deault dpid is not used */
    pdc_cfg.lspid     = AAL_PON_PORT_ID;
    pdc_cfg.fe_bypass = 0;
    pdc_cfg.no_drop   = 0;
    pdc_cfg.pol_en    = 1;

    FOR_ALL_PDC_MAP_ENTRY(gem_port) {
        if (gem_port < 32){
            __gpon_pdc_spid_gem_index_map[gem_port] = 0xFF;
        }

        pdc_cfg.pol_id = gem_port + 0x80; /* 128 ~ < 256 for PON downstream */
        ret = aal_pdc_map_mem_set(device_id, gem_port, pdc_mask, &pdc_cfg);
        CA_RET_VALUE_CHECK(ret);
    }

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
    pdc_cfg.ldpid     = 6;
    pdc_cfg.lspid     = AAL_PON_PORT_ID;
    pdc_cfg.fe_bypass = 0;
    pdc_cfg.no_drop   = 0;
    pdc_cfg.pol_en    = 0;

    FOR_ALL_PDC_MAP_ENTRY(gem_port) {
        if (gem_port < 32) {
            __gpon_pdc_spid_gem_index_map[gem_port] = 0xFF;
        }
        pdc_cfg.cos       = (gem_port < 8) ? 7 : 0; /* OMCI use COS 7, others use 0 */
        pdc_cfg.pol_id = gem_port + 0x80; /* 128 ~ < 256 for PON downstream */
        ret = aal_pdc_map_mem_set(device_id, gem_port, pdc_mask, &pdc_cfg);
        CA_RET_VALUE_CHECK(ret);
    }
#endif

    pdc_ctrl_mask.wrd = 0;
    pdc_ctrl_mask.bf.pdc_map_mem_en = 1;

    pdc_ctrl.pdc_map_mem_en = 1;

    ret = aal_pdc_ctrl_set(device_id, pdc_ctrl_mask, &pdc_ctrl);

    return ret;
}

static ca_status_t __pdc_epon_family_init(ca_device_id_t        device_id)
{
    aal_pdc_pdc_map_mem_data_mask_t pdc_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_cfg;
    aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
    aal_pdc_ctrl_t                  pdc_ctrl;
    ca_uint32_t                     llid_idx = 0;
    ca_uint32_t                     llid_base = 0;
    ca_status_t                     ret = CA_E_OK;

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, dis_pdc_thrtl, 0);
    
    pdc_ctrl_mask.wrd = ~0U;
    memset(&pdc_ctrl, 0, sizeof(aal_pdc_ctrl_t));

    pdc_ctrl.nocrc_insert = 1;
    pdc_ctrl.epon_insert_16byte_hdr = 1;
    pdc_ctrl.pdc_map_mem_en = 1;

    ret = aal_pdc_ctrl_set(device_id, pdc_ctrl_mask, &pdc_ctrl);
    pdc_mask.wrd = ~0;

    /* PDC_PDC_MAP_MEM_ACCESS.addr: for oam packets, bit[6] = 1 */
    llid_base = (1 << 6);
    for(llid_idx = 0; llid_idx < AAL_EPON_LLID_NUM_MAX; llid_idx++){ // for ucast llid
        memset(&pdc_cfg, 0, sizeof(pdc_cfg));
        pdc_cfg.lspid = 0x20 + llid_idx;
        pdc_cfg.pol_id = llid_idx;
        pdc_cfg.pol_en = 1;
        pdc_cfg.fe_bypass = 0;
        ret = aal_pdc_map_mem_set(device_id, llid_idx, pdc_mask, &pdc_cfg); // for data traffic
        ret |= aal_pdc_map_mem_set(device_id, llid_base + llid_idx, pdc_mask, &pdc_cfg); // for control packets
    }
    CA_RET_VALUE_CHECK(ret);

    for(llid_idx = AAL_EPON_LLID_NUM_MAX; llid_idx <= AAL_EPON_LLID_INDEX_MAX; llid_idx++){ // for mcast llid
        memset(&pdc_cfg, 0, sizeof(pdc_cfg));
        pdc_cfg.lspid = 0x20;
        pdc_cfg.pol_id = llid_idx;        
        pdc_cfg.pol_en = 1;
        pdc_cfg.fe_bypass = 0;
        ret = aal_pdc_map_mem_set(device_id, llid_idx, pdc_mask, &pdc_cfg); // for data traffic
        ret |= aal_pdc_map_mem_set(device_id, llid_base + llid_idx, pdc_mask, &pdc_cfg); // for control packets
    }

    return ret;
}

static ca_status_t __pdc_eth_init(ca_device_id_t        device_id)
{
    aal_pdc_pdc_map_mem_data_mask_t pdc_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_cfg;
    aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
    aal_pdc_ctrl_t                  pdc_ctrl;
    ca_uint32_t                     i = 0;
    ca_status_t                     ret = CA_E_OK;

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, dis_pdc_thrtl, 0);
    
    pdc_mask.wrd = ~0;
    memset(&pdc_cfg, 0, sizeof(pdc_cfg));

    pdc_cfg.lspid = AAL_PON_PORT_ID;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
    pdc_cfg.ldpid = 6;
    pdc_cfg.fe_bypass = 1;
#else
    pdc_cfg.ldpid = 0;
    pdc_cfg.fe_bypass = 0;
#endif

    /* for all downstream mapping: source port, dest port */
    FOR_ALL_PDC_MAP_ENTRY(i) {
        ret |= aal_pdc_map_mem_set(device_id, i, pdc_mask, &pdc_cfg);
    }
    CA_RET_VALUE_CHECK(ret);

    /* enable downstream mapping */
    pdc_ctrl_mask.wrd = 0;
    pdc_ctrl_mask.bf.pdc_map_mem_en = 1;

    pdc_ctrl.pdc_map_mem_en = 1;
    pdc_ctrl.epon_insert_16byte_hdr = 0;
    pdc_ctrl.nocrc_insert = 1;

    ret = aal_pdc_ctrl_set(device_id, pdc_ctrl_mask, &pdc_ctrl);

    return ret;
}

/******************************************************************************/
/*                             Export function                                */
/******************************************************************************/
ca_status_t aal_pdc_init(
    ca_device_id_t  device_id,
    ca_uint32_t     pon_mac_mode
)
{
    static ca_uint32_t  pdc_init_done = 0;
    ca_status_t         ret = CA_E_OK;

    if (!__pdc_lock) {
        if (CA_E_OK != ca_spin_lock_init(&__pdc_lock, SPINLOCK_BH)) {
            __pdc_lock = 0;
            ca_printf("create pdc spin lock fail\r\n");
        }
    }

    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mac_mode)) {
        ret = __pdc_epon_family_init(device_id);

    } else if (CA_PON_MODE_IS_GPON_FAMILY(pon_mac_mode)) {
        ret =__pdc_gpon_family_init(device_id);

    } else if (pon_mac_mode == ONU_PON_MAC_ETHERNET) {
        ret = __pdc_eth_init(device_id);
    }

    if (CA_E_OK != ret) {
        ca_printf("%s(): init PDC fail, return %#x\r\n", ret);
        return ret;
    }
    
    if (!pdc_init_done)
        ca_printf("ca-pon: init PDC OK\n");

    pdc_init_done = 1;

    return CA_E_OK;
}

ca_status_t aal_pdc_cnt_cfg_set(
    CA_IN ca_device_id_t           device_id,
    CA_IN aal_pdc_cntr_cfg_mask_t  mask,
    CA_IN aal_pdc_cntr_cfg_t       *data
)
{
    AAL_REG_PDC_CNTR_CFG_t reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PDC_LOCK();

    reg_data.wrd  = AAL_PON_REG_READ(PDC_CNTR_CFG);

    if (mask.bf.saturate)
        reg_data.bf.saturate = data->saturate;
    if (mask.bf.clear)
        reg_data.bf.clear = data->clear;

    AAL_PON_REG_WRITE(reg_data.wrd, PDC_CNTR_CFG);

    __PDC_UNLOCK();

    PDC_DEBUG("write pdc reg ret:0x%x, saturate:%d, clear:%d\r\n",
        reg_data.wrd, reg_data.bf.saturate, reg_data.bf.clear);

    return CA_E_OK;

}

ca_status_t aal_pdc_cnt_cfg_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_cntr_cfg_t  *data
)
{
    AAL_REG_PDC_CNTR_CFG_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd   = AAL_PON_REG_READ(PDC_CNTR_CFG);
    data->saturate = reg_data.bf.saturate;
    data->clear    = reg_data.bf.clear;

    PDC_DEBUG("read pdc reg:0x%x, saturate:%d, clear:%d\r\n",
        reg_data.wrd, data->saturate, data->clear);

    return CA_E_OK;
}

#if 0
ca_status_t aal_pdc_pkt_cnt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint16_t       *data
)
{
    PDC_PKT_CNTR_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(PDC_PKT_CNTR);

    *data = reg_data.bf.cntr;

    PDC_DEBUG("read pdc reg:0x%x, cnt:%d\r\n", reg_data.wrd, *data);

    return CA_E_OK;
}


ca_status_t aal_pdc_err_pkt_cnt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint16_t       *data
)
{
    PDC_ERR_PKT_CNTR_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(PDC_ERR_PKT_CNTR);

    *data = reg_data.bf.cntr;

    PDC_DEBUG("read pdc reg:0x%x, cnt:%d\r\n", reg_data.wrd, *data);

    return CA_E_OK;
}
#endif

ca_status_t aal_pdc_stats_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_stats_t    *data
)
{
    PDC_PKT_CNTR_t  good_cnt;
    PDC_ERR_PKT_CNTR_t  error_cnt;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    good_cnt.wrd  = AAL_PON_REG_READ(PDC_PKT_CNTR);
    error_cnt.wrd  = AAL_PON_REG_READ(PDC_ERR_PKT_CNTR);

    data->good_pkt_cnt  = good_cnt.bf.cntr;
    data->error_pkt_cnt = error_cnt.bf.cntr;

    return CA_E_OK;
}



ca_status_t aal_pdc_fifo_status_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_fifo_status_t       *data
)
{
    PDC_FIFO_STATUS_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(PDC_FIFO_STATUS);

    data->depth = reg_data.bf.depth;
    data->over_flow = reg_data.bf.over_flow;

    PDC_DEBUG("read pdc reg:0x%x, depth:%d, over_flow:%d\r\n",
        reg_data.wrd, data->depth, data->over_flow);

    return CA_E_OK;
}

ca_status_t aal_pdc_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_pdc_ctrl_mask_t  mask,
    CA_IN aal_pdc_ctrl_t       *data
)
{
    PDC_CTRL_t reg_data, data_old;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(PDC_CTRL);
    data_old.wrd = reg_data.wrd;

    if (mask.bf.nocrc_insert)
        reg_data.bf.nocrc_insert = data->nocrc_insert;

    if (mask.bf.pdc_map_mem_en)
        reg_data.bf.pdc_map_mem_en = data->pdc_map_mem_en;

    if (mask.bf.epon_insert_16byte_hdr)
        reg_data.bf.epon_insert_16byte_hdr = data->epon_insert_16byte_hdr;

    if (reg_data.wrd != data_old.wrd) {
        PDC_DEBUG("write pdc reg ret:0x%x\r\n", reg_data.wrd);
        __PDC_LOCK();
        AAL_PON_REG_WRITE(reg_data.wrd, PDC_CTRL);
        __PDC_UNLOCK();
    } else {
        PDC_DEBUG("write data 0x%08x is the same as old\r\n", reg_data.wrd);
    }

    return CA_E_OK;

}

ca_status_t aal_pdc_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_pdc_ctrl_t       *data
)
{
    PDC_CTRL_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(PDC_CTRL);

    PDC_DEBUG("read pdc reg:0x%x\r\n", reg_data.wrd);

    data->nocrc_insert           = reg_data.bf.nocrc_insert;
    data->pdc_map_mem_en         = reg_data.bf.pdc_map_mem_en;
    data->epon_insert_16byte_hdr = reg_data.bf.epon_insert_16byte_hdr;


    return CA_E_OK;
}

ca_status_t aal_pdc_map_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_IN aal_pdc_pdc_map_mem_data_mask_t mask,
    CA_IN aal_pdc_pdc_map_mem_data_t *cfg)
{
    ca_status_t ret = CA_E_OK;

    PDC_PDC_MAP_MEM_DATA0_t  reg_data0;
    PDC_PDC_MAP_MEM_DATA1_t  reg_data1;

    AAL_ASSERT_RET((cfg != NULL) && (gem_port_id < AAL_PDC_MAP_ENTRY_NUM), CA_E_PARAM);

    if (!mask.wrd) {
        PDC_DEBUG("nothing to set\r\n");
        return CA_E_OK;
    }

    reg_data0.wrd = 0;
    reg_data1.wrd = 0;

    __PDC_LOCK();

    AAL_PON_REG_INDIRECT_READ(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    reg_data0.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA0);
    reg_data1.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA1);

    if (mask.bf.cos)
        reg_data0.bf.cos = cfg->cos;
    if (mask.bf.ldpid)
        reg_data0.bf.ldpid = cfg->ldpid;
    if (mask.bf.lspid)
        reg_data0.bf.lspid = cfg->lspid;
    if (mask.bf.fe_bypass)
        reg_data0.bf.fe_bypass = cfg->fe_bypass;
    if (mask.bf.hdr_type)
        reg_data0.bf.hdr_type = cfg->hdr_type;
    if (mask.bf.mcgid)
        reg_data0.bf.mcgid = cfg->mcgid;
    if (mask.bf.drop_code)
        reg_data0.bf.drop_code = cfg->drop_code;
    if (mask.bf.rx_pkt_type)
        reg_data0.bf.rx_pkt_type = cfg->rx_pkt_type;
    if (mask.bf.no_drop)
        reg_data0.bf.no_drop = cfg->no_drop;

    if (mask.bf.mirror)
        reg_data1.bf.mirror = cfg->mirror;
    if (mask.bf.mark)
        reg_data1.bf.mark = cfg->mark;
    if (mask.bf.pol_en)
        reg_data1.bf.pol_en = cfg->pol_en;
    if (mask.bf.pol_id)
        reg_data1.bf.pol_id = cfg->pol_id;
    if (mask.bf.pol_grp_id)
        reg_data1.bf.pol_grp_id = cfg->pol_grp_id;
    if (mask.bf.deepq)
        reg_data1.bf.deepq = cfg->deepq;
    if (mask.bf.cpu_flag)
        reg_data1.bf.cpu_flag = cfg->cpu_flag;

    AAL_PON_REG_WRITE (reg_data0.wrd, PDC_PDC_MAP_MEM_DATA0);
    AAL_PON_REG_WRITE (reg_data1.wrd, PDC_PDC_MAP_MEM_DATA1);
    AAL_PON_REG_INDIRECT_WRITE(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    __PDC_UNLOCK();

    PDC_DEBUG("cos:%d, ldpid:%d, lspid:%d, fe_bypass:%d, hdr_type:%d, mcgid:%d, drop_code:%d,rx_pkt_type:%d,no_drop:%d \r\n",
            reg_data0.bf.cos, reg_data0.bf.ldpid, reg_data0.bf.lspid, reg_data0.bf.fe_bypass,
            reg_data0.bf.hdr_type, reg_data0.bf.mcgid, reg_data0.bf.drop_code,reg_data0.bf.rx_pkt_type,reg_data0.bf.no_drop);
    PDC_DEBUG("mirror:%d, mark:%d, pol_en:%d, pol_id:%d, pol_grp_id:%d, deepq:%d, cpu_flag:%d\r\n",
            reg_data1.bf.mirror, reg_data1.bf.mark, reg_data1.bf.pol_en, reg_data1.bf.pol_id,
            reg_data1.bf.pol_grp_id, reg_data1.bf.deepq, reg_data1.bf.cpu_flag);

    return ret;
}

ca_status_t aal_pdc_map_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_OUT aal_pdc_pdc_map_mem_data_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PDC_PDC_MAP_MEM_DATA0_t  reg_data0;
    PDC_PDC_MAP_MEM_DATA1_t  reg_data1;

    AAL_ASSERT_RET(cfg != NULL && gem_port_id < AAL_PDC_MAP_ENTRY_NUM, CA_E_PARAM);


    reg_data0.wrd = 0;
    reg_data1.wrd = 0;

    __PDC_LOCK();

    AAL_PON_REG_INDIRECT_READ(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    reg_data0.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA0);
    reg_data1.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA1);

    __PDC_UNLOCK();

    cfg->cos = reg_data0.bf.cos;
    cfg->ldpid = reg_data0.bf.ldpid;
    cfg->lspid = reg_data0.bf.lspid;
    cfg->fe_bypass = reg_data0.bf.fe_bypass;
    cfg->hdr_type = reg_data0.bf.hdr_type;
    cfg->mcgid = reg_data0.bf.mcgid;
    cfg->drop_code = reg_data0.bf.drop_code;
    cfg->rx_pkt_type = reg_data0.bf.rx_pkt_type;
    cfg->no_drop = reg_data0.bf.no_drop;

    cfg->mirror = reg_data1.bf.mirror;
    cfg->mark = reg_data1.bf.mark;
    cfg->pol_en = reg_data1.bf.pol_en;
    cfg->pol_id = reg_data1.bf.pol_id;
    cfg->pol_grp_id = reg_data1.bf.pol_grp_id;
    cfg->deepq = reg_data1.bf.deepq;
    cfg->cpu_flag = reg_data1.bf.cpu_flag;

    PDC_DEBUG("cos:%d, ldpid:%d, lspid:%d, fe_bypass:%d, hdr_type:%d, mcgid:%d, drop_code:%d, rx_pkt_type:%d, no_drop:%d, mirror:%d, mark:%d, pol_en:%d, pol_id:%d, pol_grp_id:%d, deepq:%d, cpu_flag:%d\r\n",
            cfg->cos, cfg->ldpid, cfg->lspid, cfg->fe_bypass, cfg->hdr_type, cfg->mcgid, cfg->drop_code, cfg->rx_pkt_type, cfg->no_drop,
            cfg->mirror, cfg->mark, cfg->pol_en, cfg->pol_id, cfg->pol_grp_id, cfg->deepq, cfg->cpu_flag);

    return ret;
}

ca_uint8_t aal_pdc_free_lspid_get(CA_IN ca_device_id_t     device_id,
                                          CA_IN ca_uint8_t gem_index)
{
    ca_uint8_t indx = 0;
    for(indx = 0; indx<32; indx++){
        if(__gpon_pdc_spid_gem_index_map[indx] == 0xff){
            __gpon_pdc_spid_gem_index_map[indx] = gem_index;
            return 0x20+indx;
        }
    }
    return 7;
}

ca_status_t aal_pdc_lspid_gem_index_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    lspid,
    CA_IN ca_uint32_t    gem_index
)
{
    if((lspid<0x20) || (lspid>0x3f)){
        return CA_E_PARAM;
    }
    __gpon_pdc_spid_gem_index_map[lspid-0x20] = gem_index;
    return CA_E_OK;
}

ca_status_t aal_pdc_lspid_gem_index_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    lspid,
    CA_OUT ca_uint32_t   *gem_index
)
{
    if((gem_index == NULL)|| (lspid<0x20) || (lspid>0x3f)){
        return CA_E_PARAM;
    }
    *gem_index = __gpon_pdc_spid_gem_index_map[lspid-0x20];
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_pdc_cnt_cfg_set);
EXPORT_SYMBOL(aal_pdc_cnt_cfg_get);
EXPORT_SYMBOL(aal_pdc_stats_get);
EXPORT_SYMBOL(aal_pdc_fifo_status_get);
EXPORT_SYMBOL(aal_pdc_ctrl_set);
EXPORT_SYMBOL(aal_pdc_ctrl_get);
EXPORT_SYMBOL(aal_pdc_map_mem_set);
EXPORT_SYMBOL(aal_pdc_map_mem_get);
EXPORT_SYMBOL(aal_pdc_free_lspid_get);
EXPORT_SYMBOL(aal_pdc_lspid_gem_index_set);
EXPORT_SYMBOL(aal_pdc_lspid_gem_index_get);

