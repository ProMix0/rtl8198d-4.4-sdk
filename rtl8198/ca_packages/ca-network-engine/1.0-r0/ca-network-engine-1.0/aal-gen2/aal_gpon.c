/******************************************************************************/
/* This file contains unpublished documentation and software                */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure,    */
/* in whole or in part, of the information in this file without a                */
/* written consent of an officer of Cortina Systems Incorporated is          */
/* strictly prohibited.                                                                        */
/* Copyright (c) 2018 by Cortina Systems Incorporated.                        */
/******************************************************************************/
/*
  * aal_gpon.c
 *
 * $Id: aal_gpon.c  2018/11/13 11:40:14 yuanyuan 
 *
 * API's for GPON interface
 */
#ifdef CONFIG_NE_PON
#include <soc/cortina/registers.h>
#include "ca_types.h"
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_gpon.h"
#include "ca-scfg/scfg.h"
#include "aal_psds.h"
#include "aal_pdc.h"
#include "aal_puc.h"
#include "aal_gpon_intr.h"
#include "gpon.h"
#ifdef CONFIG_LUNA_G3_SERIES
#include "ca_event.h"
extern ca_uint8_t g_admin_state;
#endif

extern xgpon_basic_info_t g_xgpn_basic_info;
static ca_uint      sg_aal_gpon_lock = 0;
aal_gpon_port_cfg_t g_gpon_basic_info = {0};

#ifdef CONFIG_LUNA_G3_SERIES
aal_gpon_tcont_cfg_t gpon_tmp_alloc_id_cfg[AAL_GPON_SYSTEM_MAX_TCONT_NUM];
aal_gpon_gem_port_cfg_t tmp_usbg_gem_cfg[AAL_GPON_SYSTEM_MAX_GEM_NUM];
aal_gpon_gem_port_cfg_t tmp_dsfp_gem_cfg[AAL_GPON_SYSTEM_MAX_GEM_NUM];
extern aal_puc_queue_cfg_t tmp_puc_queue_cfg[AAL_GPON_SYSTEM_MAX_TCONT_NUM][8];
extern aal_puc_ctrl_cfg_t tmp_puc_ctrl_config;
extern aal_puc_btc_cfg_t tmp_puc_btc_config;
extern aal_puc_voq_tbc_mem_cfg_t tmp_puc_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
extern aal_puc_voq_tbc_mem_cfg_t tmp_puc_rl_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
extern ca_uint32_t tmp_puc_bp_tqmvoqid[64];//bp only has 64 entries
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel1500_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel3116_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel4732_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel6348_cfg;
extern ca_uint32_t tmp_puc_voq_buf_limit_a;
extern ca_uint32_t tmp_puc_voq_buf_limit_b;
extern ca_uint32_t tmp_puc_voq_buf_limit_c;

extern ca_uint32_t tcont_num_per_chip;
extern ca_uint32_t queue_num_per_tcont;

ca_uint32_t __gpon_onu_link_evt_handler(ca_device_id_t  device_id, ca_uint32_t  event_id, ca_void_t *event, ca_uint32_t len, ca_void_t *cookie);
#endif

#define AAL_GPON_LOCK()           ca_spin_lock(sg_aal_gpon_lock)
#define AAL_GPON_UNLOCK()         ca_spin_unlock(sg_aal_gpon_lock)
#define AAL_GPON_CFG_MASK_CK()    do {\
                                     if (!mask.u32)\
                                        return CA_E_OK;\
                                     }while(0)

#define __GPN_TCONT_FIND_INDIRCT_ADDRESS(rbw, sw_plm_en, allocid) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(sw_plm_en)<<16)|(ca_uint32_t)((allocid)&0xFFF))
#define __GPN_TCONT_DO_INDIRCT_OP(rbw, sw_plm_en, allocid, reg) AAL_PON_REG_WRITE(__GPN_TCONT_FIND_INDIRCT_ADDRESS(rbw, sw_plm_en, allocid), reg); \
                                                                                                     __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_GPN_TCONT_REG_INDIRECT_READ(sw_plm_en, allocid, reg) __GPN_TCONT_DO_INDIRCT_OP(0, sw_plm_en, allocid, reg)
#define AAL_GPN_TCONT_REG_INDIRECT_WRITE(sw_plm_en, allocid, reg) __GPN_TCONT_DO_INDIRCT_OP(1, sw_plm_en, allocid, reg)
                                     
#define __GPN_DS_GEM_PORT_FIND_INDIRCT_ADDRESS(rbw, sw_aes_en, gem_id) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(sw_aes_en)<<16)|(ca_uint32_t)((gem_id)&0xFFF))
#define __GPN_DS_GEM_PORT_DO_INDIRCT_OP(rbw, sw_aes_en, gem_id, reg) AAL_PON_REG_WRITE(__GPN_DS_GEM_PORT_FIND_INDIRCT_ADDRESS(rbw, sw_aes_en, gem_id), reg); \
                                                                                                          __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_DS_GEM_PORT_REG_INDIRECT_READ(sw_aes_en, gem_id, reg) __GPN_DS_GEM_PORT_DO_INDIRCT_OP(0, sw_aes_en, gem_id, reg)
#define AAL_DS_GEM_PORT_REG_INDIRECT_WRITE(sw_aes_en, gem_id, reg) __GPN_DS_GEM_PORT_DO_INDIRCT_OP(1, sw_aes_en, gem_id, reg)
                                     
#define __GPN_DS_PORT_MIB_FIND_INDIRCT_ADDRESS(rbw, port_index) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|(ca_uint32_t)((port_index)&0x3F))
#define __GPN_DS_PORT_MIB_DO_INDIRCT_OP(rbw, port_index, reg) AAL_PON_REG_WRITE(__GPN_DS_PORT_MIB_FIND_INDIRCT_ADDRESS(rbw, port_index), reg); \
                                                                                                   __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_GPN_DS_PORT_MIB_REG_INDIRECT_READ(port_index, reg) __GPN_DS_PORT_MIB_DO_INDIRCT_OP(0, port_index, reg)
#define AAL_GPN_DS_PORT_MIB_REG_INDIRECT_WRITE(port_index, reg) __GPN_DS_PORT_MIB_DO_INDIRCT_OP(1, port_index, reg)
                                     
#define __GPN_US_PORT_MIB_FIND_INDIRCT_ADDRESS(rbw, op_code, sel) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(op_code&0x3)<<28)|(ca_uint32_t)((sel)&0x7F))
#define __GPN_US_PORT_MIB_DO_INDIRCT_OP(rbw, op_code, sel, reg) AAL_PON_REG_WRITE(__GPN_US_PORT_MIB_FIND_INDIRCT_ADDRESS(rbw, op_code, sel), reg); \
                                                                                                     __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_GPN_US_PORT_MIB_REG_INDIRECT_READ(op_code, sel, reg) __GPN_US_PORT_MIB_DO_INDIRCT_OP(0, op_code, sel, reg)
#define AAL_GPN_US_PORT_MIB_REG_INDIRECT_WRITE(op_code, sel, reg) __GPN_US_PORT_MIB_DO_INDIRCT_OP(1, op_code, sel, reg)
                                     
#define __GPN_PLOAM_MIB_FIND_INDIRCT_ADDRESS(rbw, op_code, sel) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(op_code&0x3)<<28)|(ca_uint32_t)((sel)&0x3FF))
#define __GPN_PLOAM_MIB_DO_INDIRCT_OP(rbw, op_code, sel, reg) AAL_PON_REG_WRITE(__GPN_PLOAM_MIB_FIND_INDIRCT_ADDRESS(rbw, op_code, sel), reg); \
                                                                                                   __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_GPN_PLOAM_MIB_REG_INDIRECT_READ(op_code, sel, reg) __GPN_PLOAM_MIB_DO_INDIRCT_OP(0, op_code, sel, reg)
#define AAL_GPN_PLOAM_MIB_REG_INDIRECT_WRITE(op_code, sel, reg) __GPN_PLOAM_MIB_DO_INDIRCT_OP(1, op_code, sel, reg)
                                     
#define __GPN_US_GEM_PORT_FIND_INDIRCT_ADDRESS(rbw, gem_index) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|(ca_uint32_t)((gem_index)&0x3F))
#define __GPN_US_GEM_PORT_DO_INDIRCT_OP(rbw, gem_index, reg) AAL_PON_REG_WRITE(__GPN_US_GEM_PORT_FIND_INDIRCT_ADDRESS(rbw, gem_index), reg); \
                                                                                                  __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_US_GEM_PORT_REG_INDIRECT_READ(gem_index, reg) __GPN_US_GEM_PORT_DO_INDIRCT_OP(0, gem_index, reg)
#define AAL_US_GEM_PORT_REG_INDIRECT_WRITE(gem_index, reg) __GPN_US_GEM_PORT_DO_INDIRCT_OP(1, gem_index, reg)
                                     
#define __GPN_DBRU_BUFOCC_TBL_FIND_INDIRCT_ADDRESS(rbw, select, addr) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(select)<<16)|(ca_uint32_t)((addr)&0xF))
#define __GPN_DBRU_BUFOCC_TBL_DO_INDIRCT_OP(rbw, select, addr, reg) AAL_PON_REG_WRITE(__GPN_DBRU_BUFOCC_TBL_FIND_INDIRCT_ADDRESS(rbw, select, addr), reg); \
                                                                                                         __CHECK_INDIRCT_OPERATE_STATE(reg)
                                     
#define AAL_GPN_DBRU_BUFOCC_TBL_REG_INDIRECT_READ(select, addr, reg) __GPN_DBRU_BUFOCC_TBL_DO_INDIRCT_OP(0, select, addr, reg)
#define AAL_GPN_DBRU_BUFOCC_TBL_REG_INDIRECT_WRITE(select, addr, reg) __GPN_DBRU_BUFOCC_TBL_DO_INDIRCT_OP(1, select, addr, reg)


//This function should be remove to another file for common use
ca_status_t aal_gpon_glb_ctrl_init(
    CA_IN ca_device_id_t device_id)
{
    GLOBAL_PON_CNTL_t  ponCfg = {0};
    GLOBAL_GPON_CNTL_t gblCfg = {0};

    /* Enable PDC,PUC,PSDS */
    ponCfg.bf.puc_reset        = 1;
    ponCfg.bf.pdc_reset        = 1;
    ponCfg.bf.pon_serdes_rst_n = 1;
    ponCfg.bf.psds_reg_rst_n   = 1;
    ponCfg.bf.ptp_rst_n        = 1;
    AAL_GLB_REG_WRITE(ponCfg.wrd, GLOBAL_PON_CNTL);

    /* Enable GPON */
    gblCfg.bf.ani_rst_n  = 1;
    gblCfg.bf.gpon_rst_n = 1;
    AAL_GLB_REG_WRITE(gblCfg.wrd, GLOBAL_GPON_CNTL);

    ca_mdelay(AAL_XGPON_INIT_DELAY);
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_frame_thrsd_set(
    CA_IN ca_device_id_t                    device_id,
    CA_IN aal_gpon_ds_frame_thrsd_cfg_msk_t mask,
    CA_IN aal_gpon_ds_frame_thrsd_cfg_t     *cfg)
{
    GPON_MAC_GPON_ds_t gpon_ds_frm_thrsd = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    gpon_ds_frm_thrsd.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ds);
    if (mask.s.pre_sync_threshold) {
        gpon_ds_frm_thrsd.bf.pre_sync_threshold = cfg->pre_sync_threshold;
    }
    if (mask.s.miss_sync_threshold) {
        gpon_ds_frm_thrsd.bf.miss_sync_threshold = cfg->miss_sync_threshold;
    }
    if (mask.s.los_timer_threshold) {
        gpon_ds_frm_thrsd.bf.los_timer_threshold = cfg->los_timer_threshold;
    }
    if (mask.s.fec_switch_threshold) {
        gpon_ds_frm_thrsd.bf.fec_switch_threshold = cfg->fec_switch_threshold;
    }
    if (mask.s.max_packet_size) {
        gpon_ds_frm_thrsd.bf.max_packet_size = cfg->max_packet_size;
    }
    if (mask.s.tag_exclude) {
        gpon_ds_frm_thrsd.bf.tag_exclude = cfg->tag_exclude;
    }
    if (mask.s.stop_gpn_fe) {
        gpon_ds_frm_thrsd.bf.stop_gpn_fe = cfg->stop_gpn_fe;
    }

    AAL_PON_REG_WRITE(gpon_ds_frm_thrsd.wrd, GPON_MAC_GPON_ds);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_frame_thrsd_get(
    CA_IN ca_device_id_t                  device_id,
    CA_OUT aal_gpon_ds_frame_thrsd_cfg_t *cfg)
{
    GPON_MAC_GPON_ds_t gpon_ds_frm_thrsd = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    gpon_ds_frm_thrsd.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ds);
    cfg->pre_sync_threshold   = gpon_ds_frm_thrsd.bf.pre_sync_threshold;
    cfg->miss_sync_threshold  = gpon_ds_frm_thrsd.bf.miss_sync_threshold;
    cfg->los_timer_threshold  = gpon_ds_frm_thrsd.bf.los_timer_threshold;
    cfg->fec_switch_threshold = gpon_ds_frm_thrsd.bf.fec_switch_threshold;
    cfg->max_packet_size      = gpon_ds_frm_thrsd.bf.max_packet_size;
    cfg->tag_exclude          = gpon_ds_frm_thrsd.bf.tag_exclude;
    cfg->stop_gpn_fe          = gpon_ds_frm_thrsd.bf.stop_gpn_fe;

    return CA_E_OK;
}


ca_status_t aal_gpon_sn_req_res_ctrl_set(
    CA_IN ca_device_id_t                     device_id,
    CA_IN aal_gpon_sn_req_res_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_sn_req_res_ctrl_cfg_t     *cfg)
{
    GPON_MAC_GPON_sn_t sn_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    sn_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_sn);
    if (mask.s.req_threshold) {
        sn_ctrl.bf.req_threshold = cfg->req_threshold;
    }
    if (mask.s.rspnd_extend) {
        sn_ctrl.bf.rspnd_extend = cfg->rspnd_extend;
    }
    
    AAL_PON_REG_WRITE (sn_ctrl.wrd, GPON_MAC_GPON_sn);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_sn_req_res_ctrl_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_gpon_sn_req_res_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_sn_t sn_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    sn_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_sn);
    cfg->req_threshold = sn_ctrl.bf.req_threshold;
    cfg->rspnd_extend  = sn_ctrl.bf.rspnd_extend;

    return CA_E_OK;
}


ca_status_t aal_gpon_us_frm_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_gpon_us_frm_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_us_frm_ctrl_cfg_t     *cfg)
{
    GPON_MAC_GPON_us_t us_frm_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    us_frm_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_us);
    if (mask.s.eqd_select) {
        us_frm_ctrl.bf.eqd_select = cfg->eqd_select;
    }
    /* Masking 9 bits to avoid corruption */
    if (mask.s.frame_var) {
        us_frm_ctrl.bf.frame_var = cfg->frame_var&0x1FF;
    }
    AAL_PON_REG_WRITE (us_frm_ctrl.wrd, GPON_MAC_GPON_us);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_us_frm_ctrl_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT aal_gpon_us_frm_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_us_t us_frm_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    us_frm_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_us);
    cfg->eqd_select = us_frm_ctrl.bf.eqd_select;
    cfg->frame_var  = us_frm_ctrl.bf.frame_var;

    return CA_E_OK;
}


ca_status_t aal_gpon_sf_degrd_alrm_thrsd_set(
    CA_IN ca_device_id_t                         device_id,
    CA_IN aal_gpon_sf_degrd_alrm_thrsd_cfg_msk_t mask,
    CA_IN aal_gpon_sf_degrd_alrm_thrsd_cfg_t     *cfg)
{
    GPON_MAC_GPON_signal_t signal = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    signal.wrd = AAL_PON_REG_READ( GPON_MAC_GPON_signal);
    if (mask.s.degrade_remove_threshold) {
        signal.bf.degrade_remove_threshold = cfg->degrade_remove_threshold - 1;
    }
    if (mask.s.degrade_threshold) {
        signal.bf.degrade_threshold = cfg->degrade_threshold - 1;
    }
    if (mask.s.fail_remove_threshold) {
        signal.bf.fail_remove_threshold = cfg->fail_remove_threshold;
    }
    if (mask.s.fail_threshold) {
        signal.bf.fail_threshold = cfg->fail_threshold;
    }
    AAL_PON_REG_WRITE (signal.wrd, GPON_MAC_GPON_signal);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_sf_degrd_alrm_thrsd_get(
    CA_IN  ca_device_id_t                      device_id,
    CA_OUT aal_gpon_sf_degrd_alrm_thrsd_cfg_t  *cfg)
{
    GPON_MAC_GPON_signal_t signal = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    signal.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_signal);
    cfg->degrade_remove_threshold = (aal_gpon_signal_thres_val_t)(signal.bf.degrade_remove_threshold + 1);
    cfg->degrade_threshold        = (aal_gpon_signal_thres_val_t)(signal.bf.degrade_threshold + 1);
    cfg->fail_remove_threshold    = (aal_gpon_signal_thres_val_t)signal.bf.fail_remove_threshold;
    cfg->fail_threshold           = (aal_gpon_signal_thres_val_t)signal.bf.fail_threshold;

    return CA_E_OK;
}


ca_status_t aal_gpon_send_dying_gasp(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t   send_extra)
{
    GPON_MAC_GPON_ploamd_ctrl_t gpon_gasp_extra = {0};

    AAL_GPON_LOCK();
#ifndef CONFIG_LUNA_G3_SERIES  //extra dying-gasp should not be controlled here
    gpon_gasp_extra.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_ctrl);
    if(TRUE == send_extra) {
    /* Always try to send the maximum number support by hardware */
        gpon_gasp_extra.bf.dying_gasp_extra = 0x3;
    } else {
        gpon_gasp_extra.bf.dying_gasp_extra = 0x0;
    }
    AAL_PON_REG_WRITE(gpon_gasp_extra.wrd,GPON_MAC_GPON_ploamd_ctrl);
#endif

    /* Trigger hardware to send the dying gasp. */
    /* No need to read-modify-write here. Just a toggle */
    AAL_PON_REG_WRITE(0x0,GPON_MAC_GPON_power_down);
    AAL_PON_REG_WRITE(0x1,GPON_MAC_GPON_power_down);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_gpon_ploamd_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_ploamd_ctrl_cfg_t     *cfg)
{
    GPON_MAC_GPON_ploamd_ctrl_t ploamd_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    ploamd_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_ctrl);
    if (mask.s.key_index1) {
        ploamd_ctrl.bf.key_index1 = cfg->key_index1;
    }
    if (mask.s.key_index0) {
        ploamd_ctrl.bf.key_index0 = cfg->key_index0;
    }
    if (mask.s.dying_gasp_extra) {
        ploamd_ctrl.bf.dying_gasp_extra = cfg->dying_gasp_extra;
    }
    if (mask.s.hw_key_switch) {
        ploamd_ctrl.bf.hw_key_switch = cfg->hw_key_switch;
    }
    AAL_PON_REG_WRITE(ploamd_ctrl.wrd, GPON_MAC_GPON_ploamd_ctrl);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_ctrl_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT aal_gpon_ploamd_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_ploamd_ctrl_t ploamd_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    ploamd_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_ctrl);
    cfg->key_index1       = ploamd_ctrl.bf.key_index1;
    cfg->key_index0       = ploamd_ctrl.bf.key_index0;
    cfg->dying_gasp_extra = ploamd_ctrl.bf.dying_gasp_extra;
    cfg->hw_key_switch    = ploamd_ctrl.bf.hw_key_switch;
    return CA_E_OK;
}


ca_status_t aal_gpon_aes_key_set(
    CA_IN ca_device_id_t      device_id,
    CA_IN aal_gpon_aes_key_t *key)
{
    volatile ca_uint32_t        key_frag[4]      = {0};
    GPON_MAC_GPON_ploamu_t      gpon_key_pointer = {0};
    GPON_MAC_GPON_ploamd_ctrl_t gpon_key_index   = {0};

    CA_ASSERT_RET(key, CA_E_PARAM);

    AAL_GPON_LOCK();

    gpon_key_pointer.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamu);

    key_frag[3] = key->key[0]  | (key->key[1] << 8)  | (key->key[2] << 16)  | (key->key[3] << 24);
    key_frag[2] = key->key[4]  | (key->key[5] << 8)  | (key->key[6] << 16)  | (key->key[7] << 24);
    key_frag[1] = key->key[8]  | (key->key[9] << 8)  | (key->key[10] << 16) | (key->key[11] << 24);
    key_frag[0] = key->key[12] | (key->key[13] << 8) | (key->key[14] << 16) | (key->key[15] << 24);

    /* The key index needs to be incremented */
    gpon_key_index.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_ctrl);

    if(1 == gpon_key_pointer.bf.key_pointer) {
        /* Key Set 1 is active. Program new key into key_set 0 */
        AAL_PON_REG_WRITE(key_frag[0],GPON_MAC_GPON_key0_frag0);
        AAL_PON_REG_WRITE(key_frag[1],GPON_MAC_GPON_key0_frag1);
        AAL_PON_REG_WRITE(key_frag[2],GPON_MAC_GPON_key0_frag2);
        AAL_PON_REG_WRITE(key_frag[3],GPON_MAC_GPON_key0_frag3);

        if(((ca_uint32_t) gpon_key_index.bf.key_index0 + 1) >=  255) {
        /* Wrap around */
            gpon_key_index.bf.key_index0 = 0;
        } else {
            gpon_key_index.bf.key_index0 ++;
        }
    } else {
        /* Key Set 0 is active. Program new key into key_set 1 */
        AAL_PON_REG_WRITE(key_frag[0],GPON_MAC_GPON_key1_frag0);
        AAL_PON_REG_WRITE(key_frag[1],GPON_MAC_GPON_key1_frag1);
        AAL_PON_REG_WRITE(key_frag[2],GPON_MAC_GPON_key1_frag2);
        AAL_PON_REG_WRITE(key_frag[3],GPON_MAC_GPON_key1_frag3);

        if(((ca_uint32_t) gpon_key_index.bf.key_index1 + 1) > 255) {
        /* Wrap around */
            gpon_key_index.bf.key_index1 = 0;
        } else {
            gpon_key_index.bf.key_index1 ++;
        }
    }
    /* Write back the key index */
    AAL_PON_REG_WRITE(gpon_key_index.wrd, GPON_MAC_GPON_ploamd_ctrl);
    /* Indicate to hardware that key updation is complete */
    gpon_key_pointer.bf.req_key_mask = 0;
    AAL_PON_REG_WRITE(gpon_key_pointer.wrd,GPON_MAC_GPON_ploamu);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_aes_key_get(
    CA_IN  ca_device_id_t      device_id,
    CA_OUT aal_gpon_aes_key_t *key)
{
    volatile ca_uint32_t   key_frag[4]      = {0};
    GPON_MAC_GPON_ploamu_t gpon_key_pointer = {0};

    CA_ASSERT_RET(key, CA_E_PARAM);
    gpon_key_pointer.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamu);

    if(1 == gpon_key_pointer.bf.key_pointer){
        /* Key Set 1 is active. Read key from key_set 1 */
        key_frag[0]=AAL_PON_REG_READ(GPON_MAC_GPON_key1_frag0);
        key_frag[1]=AAL_PON_REG_READ(GPON_MAC_GPON_key1_frag1);
        key_frag[2]=AAL_PON_REG_READ(GPON_MAC_GPON_key1_frag2);
        key_frag[3]=AAL_PON_REG_READ(GPON_MAC_GPON_key1_frag3);

    } else {
        /* Key Set 1 is active. Read key from key_set 0 */
        key_frag[0]=AAL_PON_REG_READ(GPON_MAC_GPON_key0_frag0);
        key_frag[1]=AAL_PON_REG_READ(GPON_MAC_GPON_key0_frag1);
        key_frag[2]=AAL_PON_REG_READ(GPON_MAC_GPON_key0_frag2);
        key_frag[3]=AAL_PON_REG_READ(GPON_MAC_GPON_key0_frag3);
    }

    key->key[0]  = key_frag[3] & 0xff;
    key->key[1]  = (key_frag[3] >> 8) & 0xff;
    key->key[2]  = (key_frag[3] >> 16) & 0xff;
    key->key[3]  = (key_frag[3] >> 24) & 0xff;
    key->key[4]  = key_frag[2] & 0xff;
    key->key[5]  = (key_frag[2] >> 8) & 0xff;
    key->key[6]  = (key_frag[2] >> 16) & 0xff;
    key->key[7]  = (key_frag[2] >> 24) & 0xff;
    key->key[8]  = key_frag[1] & 0xff;
    key->key[9]  = (key_frag[1] >> 8) & 0xff;
    key->key[10] = (key_frag[1] >> 16) & 0xff;
    key->key[11] = (key_frag[1] >> 24) & 0xff;
    key->key[12] = key_frag[0] & 0xff;
    key->key[13] = (key_frag[0] >> 8) & 0xff;
    key->key[14] = (key_frag[0] >> 16) & 0xff;
    key->key[15] = (key_frag[0] >> 24) & 0xff;
    return CA_E_OK;
}


ca_status_t aal_gpon_pst_set(
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_gpon_pst_cfg_msk_t mask,
    CA_IN aal_gpon_pst_cfg_t     *cfg)
{
    GPON_MAC_GPON_onu_pst_t gpon_onu_pst = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    gpon_onu_pst.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu_pst);
    if (mask.s.k2) {
        gpon_onu_pst.bf.k2 = cfg->k2;
    }
    if (mask.s.k1) {
        gpon_onu_pst.bf.k1 = cfg->k1;
    }
    if (mask.s.line_num) {
        gpon_onu_pst.bf.line_num = cfg->line_num;
    }
    AAL_PON_REG_WRITE (gpon_onu_pst.wrd, GPON_MAC_GPON_onu_pst);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_pst_get(
    CA_IN  ca_device_id_t     device_id,
    CA_OUT aal_gpon_pst_cfg_t *cfg)
{
    GPON_MAC_GPON_onu_pst_t gpon_onu_pst = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    gpon_onu_pst.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu_pst);
    cfg->k2       = gpon_onu_pst.bf.k2;
    cfg->k1       = gpon_onu_pst.bf.k1;
    cfg->line_num = gpon_onu_pst.bf.line_num;
    return CA_E_OK;
}


ca_status_t aal_gpon_olt_pst_get(
    CA_IN  ca_device_id_t      device_id,
    CA_OUT aal_gpon_pst_cfg_t *cfg)
{
    GPON_MAC_GPON_olt_pst_t gpon_olt_pst = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    gpon_olt_pst.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_olt_pst);
    cfg->k2       = gpon_olt_pst.bf.k2;
    cfg->k1       = gpon_olt_pst.bf.k1;
    cfg->line_num = gpon_olt_pst.bf.line_num;
    return CA_E_OK;
}


ca_status_t aal_gpon_last_bip_error_get(
    CA_IN  ca_device_id_t  dev_id,
    CA_OUT ca_uint32_t    *p_bip_count,
    CA_OUT ca_uint32_t    *p_rei_sequence)
{
    GPON_MAC_GPON_ploamu_t gpon_rei_sequence = {0};

    CA_ASSERT_RET(p_bip_count, CA_E_PARAM);
    CA_ASSERT_RET(p_rei_sequence, CA_E_PARAM);
    /* Directly read into the user supplied variable */
    *p_bip_count = AAL_PON_REG_READ(GPON_MAC_GPON_bip_error);

    /* Need a temporary storage for rei sequence */
    gpon_rei_sequence.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamu);
    *p_rei_sequence = (ca_uint32_t)gpon_rei_sequence.bf.rei_seq_num;

    return CA_E_OK;
}


ca_status_t aal_gpon_current_bip_error_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_boolean_t   clear_pmon,
    CA_OUT ca_uint32_t    *p_bip_count_accu,
    CA_OUT ca_uint32_t    *p_bip_frame_count)
{
    CA_ASSERT_RET(p_bip_count_accu, CA_E_PARAM);
    CA_ASSERT_RET(p_bip_frame_count, CA_E_PARAM);

    AAL_GPON_LOCK();

    /* Directly read into the user supplied variable */
    *p_bip_count_accu  = AAL_PON_REG_READ(GPON_MAC_GPON_bip_error_accum);
    *p_bip_frame_count = AAL_PON_REG_READ(GPON_MAC_GPON_bip_error_frame_count);

    if(clear_pmon == TRUE) {
        AAL_PON_REG_WRITE(0,GPON_MAC_GPON_bip_error_accum);
        AAL_PON_REG_WRITE(0,GPON_MAC_GPON_bip_error_frame_count);
    }

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_current_alarms_get(
    CA_IN  ca_device_id_t  dev_id,
    CA_OUT ca_uint32_t     *p_bitmap)
{
    GPON_MAC_GPON_alarm_t gpon_alarm = {0};

    CA_ASSERT_RET(p_bitmap, CA_E_PARAM);

    /* Needs a conversion to bitmap of enums */
    gpon_alarm.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_alarm);

    /* Reset the bitmap and store current alarm status */
    *p_bitmap = 0;

    /* Too much of 'if' statements. Required for working on both endianess */
    if(gpon_alarm.bf.los)
        *p_bitmap |= AAL_GPON_ALARM_LOS;
    if(gpon_alarm.bf.lof)
        *p_bitmap |= AAL_GPON_ALARM_LOF;
    if(gpon_alarm.bf.sf)
        *p_bitmap |= AAL_GPON_ALARM_SF;
    if(gpon_alarm.bf.sd)
        *p_bitmap |= AAL_GPON_ALARM_SD;
    if(gpon_alarm.bf.lcdg)
        *p_bitmap |= AAL_GPON_ALARM_LCDG;
    if(gpon_alarm.bf.suf)
        *p_bitmap |= AAL_GPON_ALARM_SUF;
    if(gpon_alarm.bf.dact)
        *p_bitmap |= AAL_GPON_ALARM_DACT;
    if(gpon_alarm.bf.dis)
        *p_bitmap |= AAL_GPON_ALARM_DIS;
    if(gpon_alarm.bf.mis)
        *p_bitmap |= AAL_GPON_ALARM_MIS;
    if(gpon_alarm.bf.pee)
        *p_bitmap |= AAL_GPON_ALARM_PEE;
    if(gpon_alarm.bf.rdi)
        *p_bitmap |= AAL_GPON_ALARM_RDI;
    if(gpon_alarm.bf.tf)
        *p_bitmap |= AAL_GPON_ALARM_TF;

    return CA_E_OK;
}

ca_status_t aal_gpon_port_stats_get(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t port_id,
    CA_OUT aal_gpon_stats_t *p_gpon_stats)
{
    CA_ASSERT_RET(p_gpon_stats, CA_E_PARAM);

    p_gpon_stats->bip_error_accum          = AAL_PON_REG_READ(GPON_MAC_GPON_bip_error_accum);
    p_gpon_stats->bip_error_frame_cnt      = AAL_PON_REG_READ(GPON_MAC_GPON_bip_error_frame_count);
    p_gpon_stats->ds_omci_gem              = AAL_PON_REG_READ(GPON_MAC_GPON_ds_omci_gem);
    p_gpon_stats->ds_omci_pkt              = AAL_PON_REG_READ(GPON_MAC_GPON_ds_omci_pkt);
    p_gpon_stats->ds_pkt_crc               = AAL_PON_REG_READ(GPON_MAC_GPON_ds_pkt_crc);
    p_gpon_stats->ds_undersz_pkt           = AAL_PON_REG_READ(GPON_MAC_GPON_ds_udrsz_pkt);
    p_gpon_stats->ds_oversz_pkt            = AAL_PON_REG_READ(GPON_MAC_GPON_ds_ovrsz_pkt);
    p_gpon_stats->superframe               = AAL_PON_REG_READ(GPON_MAC_GPON_superframe);
    p_gpon_stats->o5_state                 = AAL_PON_REG_READ(GPON_MAC_GPON_o5);
    p_gpon_stats->gem_frag_drp             = AAL_PON_REG_READ(GPON_MAC_GPON_gem_frag_drop);
    p_gpon_stats->gem_err_1bit             = AAL_PON_REG_READ(GPON_MAC_GPON_gem_one_biterr);
    p_gpon_stats->gem_err_2bit             = AAL_PON_REG_READ(GPON_MAC_GPON_gem_two_biterr);
    p_gpon_stats->gem_uncorrect_bit_err    = AAL_PON_REG_READ(GPON_MAC_GPON_gem_uncorrected_biterr);
    p_gpon_stats->bwmap_drop               = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_drop);
    p_gpon_stats->omci_crc_err             = AAL_PON_REG_READ(GPON_MAC_GPON_omci_crc);
    p_gpon_stats->plend_err                = AAL_PON_REG_READ(GPON_MAC_GPON_plend_err);
    p_gpon_stats->plend_bit_err            = AAL_PON_REG_READ(GPON_MAC_GPON_plend_biterr);
    p_gpon_stats->ds_asmbl_fifo_drop_cnt   = AAL_PON_REG_READ(GPON_MAC_GPON_ds_asmbl_fifo_drop_cnt);
    p_gpon_stats->bwmap_uncorrected_biterr = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_uncorrected_biterr);
    p_gpon_stats->bwmap_corrected_biterr   = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_corrected_biterr);

    return CA_E_OK;
}


ca_status_t aal_gpon_port_stats_clear(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id)
{
    AAL_GPON_LOCK();

    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_bip_error_accum);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_bip_error_frame_count);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_omci_gem);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_omci_pkt);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_pkt_crc);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_udrsz_pkt);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_ovrsz_pkt);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_superframe);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_o5);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_gem_frag_drop);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_gem_one_biterr);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_gem_two_biterr);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_gem_uncorrected_biterr);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_bwmap_drop);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_omci_crc);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_plend_err);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_plend_biterr);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_ds_asmbl_fifo_drop_cnt);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_bwmap_uncorrected_biterr);
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_bwmap_corrected_biterr);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_us_ovrh_plm_msg_params_get(
    CA_IN  ca_device_id_t                dev_id,
    CA_IN  ca_uint8_t                    port_id,
    CA_OUT aal_us_ovrh_plm_msg_params_t  *us_ovhd_para)
{
    GPON_MAC_GPON_bits_t       gpon_bits       = {0};
    GPON_MAC_GPON_pattern_t    gpon_pattern    = {0};
    GPON_MAC_GPON_pre_assign_t gpon_pre_assign = {0};

    CA_ASSERT_RET(us_ovhd_para, CA_E_PARAM);

    gpon_bits.wrd       = AAL_PON_REG_READ(GPON_MAC_GPON_bits);
    gpon_pattern.wrd    = AAL_PON_REG_READ(GPON_MAC_GPON_pattern);
    gpon_pre_assign.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_pre_assign);

    us_ovhd_para->guard             = gpon_bits.bf.guard;
    us_ovhd_para->t1_preamble       = gpon_bits.bf.t1_preamble;
    us_ovhd_para->t2_preamble       = gpon_bits.bf.t2_preamble;
    us_ovhd_para->t3_preamble       = gpon_pattern.bf.t3_preamble;
    us_ovhd_para->delimiter         = gpon_pattern.bf.delimiter;
    us_ovhd_para->pre_assgin_eqd_en = gpon_pre_assign.bf.eqd_en;

    /* Check whether pre-eqd is being used */
    if(gpon_pre_assign.bf.eqd_en) {
        /* Pre-Eqd is in use */
        us_ovhd_para->pre_assgin_eqd = gpon_pre_assign.bf.eqd;
    }

    return CA_E_OK;
}


ca_status_t aal_gpon_sn_acquisition_status_get(
    CA_IN  ca_device_id_t                    dev_id,
    CA_IN  ca_uint8_t                        port_id,
    CA_OUT aal_gpon_sn_acquisition_status_t  *sn_acquisition_status)
{
    GPON_MAC_GPON_sn_acquire_t gpon_sn_acquire = {0};

    CA_ASSERT_RET(sn_acquisition_status, CA_E_PARAM);

    gpon_sn_acquire.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_sn_acquire);

    sn_acquisition_status->mask_en = gpon_sn_acquire.bf.mask_en;
    sn_acquisition_status->en      = gpon_sn_acquire.bf.en;
    sn_acquisition_status->extra   = gpon_sn_acquire.bf.extra;
    return CA_E_OK;
}


ca_status_t aal_gpon_onu_state_get(
    CA_IN ca_device_id_t          dev_id,
    CA_IN ca_uint8_t              port_id, 
    CA_OUT aal_gpon_onu_states_t *p_state)
{
    GPON_MAC_GPON_onu_t gpon_onu = {0};

    CA_ASSERT_RET(p_state, CA_E_PARAM);

    gpon_onu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu);

    /* Convert to 'enum' value */
    /* Could use the register value directly */
    switch(gpon_onu.bf.state){
        case 0:
            *p_state = AAL_GPON_STATE_INITIAL;
            break;
        case 1:
            *p_state = AAL_GPON_STATE_STANDBY;
            break;
        case 2:
            *p_state = AAL_GPON_STATE_SERIALNO;
            break;
        case 3:
            *p_state = AAL_GPON_STATE_RANGING;
            break;
        case 4:
            *p_state = AAL_GPON_STATE_OPERATION;
            break;
        case 5:
            *p_state = AAL_GPON_STATE_POPUP;
            break;
        case 6:
            *p_state = AAL_GPON_STATE_ESTOP;
            break;
        default: /* Should never reach here */
            return (CA_E_ERROR);
            break;
    }

    return CA_E_OK;
}


ca_status_t aal_gpon_eqd_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *p_eqd)
{
    CA_ASSERT_RET(p_eqd, CA_E_PARAM);
    *p_eqd = AAL_PON_REG_READ(GPON_MAC_GPON_main);
    return CA_E_OK;
}


ca_status_t aal_gpon_protect_eqd_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id,
    CA_OUT ca_uint32_t    *p_eqd)
{
    CA_ASSERT_RET(p_eqd, CA_E_PARAM);
    *p_eqd = AAL_PON_REG_READ(GPON_MAC_GPON_protection);
    return CA_E_OK;
}


ca_status_t aal_gpon_onu_params_get(
    CA_IN  ca_device_id_t         dev_id,
    CA_IN  ca_uint8_t             port_id,
    CA_OUT aal_gpon_onu_params_t  *onu_para)
{
    GPON_MAC_GPON_onu_t       gpon_onu       = {0};
    GPON_MAC_GPON_omci_port_t gpon_omci_port = {0};

    CA_ASSERT_RET(onu_para, CA_E_PARAM);

    /* ONU-ID: Default alloc id */
    gpon_onu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu);
    onu_para->onu_id = (ca_uint8_t)gpon_onu.bf.id;

    gpon_omci_port.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_omci_port);
    if(gpon_omci_port.bf.en) {
        /* The port id is valid only if the port is active */
        onu_para->omci_port = (ca_uint16_t)gpon_omci_port.bf.id;
    }

    /* BER interval */
    /* Direct read to user variable */
    onu_para->ber_interval = AAL_PON_REG_READ(GPON_MAC_GPON_ber);
    /* Current key switching time */
    /* Direct read to user variable (30 bits)*/
    onu_para->key_switch_time = AAL_PON_REG_READ(GPON_MAC_GPON_key_switching)& 0x3FFFFFFF;

    /* Current power level */
    onu_para->pwr_lvl = (aal_gpon_onu_pwr_lvl_t)gpon_onu.bf.power_level;
    return CA_E_OK;
}


ca_status_t aal_gpon_type3_preamble_get(
    CA_IN  ca_device_id_t                 dev_id,
    CA_IN  ca_uint8_t                     port_id,
    CA_OUT aal_gpon_type3_preamble_cfg_t  *t3_preamble_cfg)
{
    GPON_MAC_GPON_t3_preamble_t gpon_t3_preamble = {0};

    CA_ASSERT_RET(t3_preamble_cfg, CA_E_PARAM);

    gpon_t3_preamble.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_t3_preamble);
    
    /* Check if EBL has been received or not */
    if(gpon_t3_preamble.bf.extend) {
        t3_preamble_cfg->preranged_t3_preamble = gpon_t3_preamble.bf.pre_range;
        t3_preamble_cfg->ranged_t3_preamble    = gpon_t3_preamble.bf.ranged;
    }
    return CA_E_OK;
}


ca_status_t aal_gpon_send_ploam_us(
    CA_IN ca_device_id_t           dev_id,
    CA_IN ca_uint8_t               port_id,
    CA_IN aal_gpon_onu_ploam_us_t  ploam_type,
    CA_IN ca_uint8_t               *p_message)
{
    register ca_int32_t  loopcount = 0;
    volatile ca_uint16_t msg_frag  = 0;
    GPON_MAC_GPON_hi_ploamu_t  gpon_ploamu  = {0};
    GPON_MAC_GPON_hi_ploamu0_t gpon_ploamu0 = {0};
    ca_int32_t max_tries = 0;

    CA_ASSERT_RET(p_message, CA_E_PARAM);

    AAL_GPON_LOCK();

    /* Parameter check for enum. No type check in C */
    if(( ploam_type < AAL_GPON_PLOAMu_SERIAL_NUMBER) 
        ||(ploam_type > AAL_GPON_PLOAMu_ACK)) {
        AAL_GPON_UNLOCK();
        ca_printf("%s: ploam_type is invalid\r\n",__func__);
        return CA_E_PARAM;
    }


/* Wait for the transmission of the previous PLOAM */
/* This case would be very rare so we can do a busy wait */
    do {
        gpon_ploamu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu);
        ca_mdelay(1);
        max_tries ++;
        if(max_tries == 10) {
            AAL_GPON_UNLOCK();
            ca_printf("%s: Ploam transmission pending\r\n",__func__);
            return CA_E_PARAM;
        }
    } while(gpon_ploamu.bf.req);

    /* Clear to send the next PLOAM */

    gpon_ploamu0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu0);

    /* Populate the data */
    /* Not going to do a read-modify-write here. Can save some memory cycles */
    /* 00-31 bits in ploamu2 --> p_message[6] - p_message[9]*/
    /* 32-63 bits in ploamu1 --> p_message[2] - p_message[5]*/
    AAL_PON_REG_WRITE((p_message[9] | (p_message[8] << 8) |
                      (p_message[7] << 16) | (p_message[6] << 24)),
                      GPON_MAC_GPON_hi_ploamu2);

    AAL_PON_REG_WRITE((p_message[5] | (p_message[4] << 8) |
                      (p_message[3] << 16) | (p_message[2] << 24)),
                      GPON_MAC_GPON_hi_ploamu1);

    /* 64-79 bits in ploamu0 --> p_message[1] - p_message[0]*/
    msg_frag = p_message[1] | (p_message[0] << 8);

    /* Modify the message */
    /* Directly using the enum */
    gpon_ploamu0.bf.message = (ca_uint32_t)ploam_type;
    /* Write the remaining data */
    gpon_ploamu0.bf.data = msg_frag;

    AAL_PON_REG_WRITE(gpon_ploamu0.wrd,GPON_MAC_GPON_hi_ploamu0);

    /* All set to initiate the transmission */
    gpon_ploamu.bf.req = 1;
    /* No need to do a compare write here. 'req' would always be '0' */
    AAL_PON_REG_WRITE(gpon_ploamu.wrd,GPON_MAC_GPON_hi_ploamu);

    /* Waiting for the processing by hardware transmission */
    do {
        ca_mdelay(1);
        gpon_ploamu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu);
        loopcount++; 
    } while( (gpon_ploamu.bf.req) && (loopcount < 5) );

    AAL_GPON_UNLOCK();
    return CA_E_OK ;
}


ca_status_t aal_gpon_send_ploam_ds(
    CA_IN ca_device_id_t          dev_id,
    CA_IN ca_uint8_t              port_id, 
    CA_IN aal_gpon_onu_ploam_ds_t ploam_type,
    CA_IN ca_uint8_t              onu_id,
    CA_IN ca_boolean_t            ignore_hw_state,
    CA_IN ca_uint8_t              *p_message)
{
    register ca_uint32_t loopcount = 0;
    volatile ca_uint16_t msg_frag  = 0;
    GPON_MAC_GPON_hi_ploamu_t  gpon_ploamu      = {0};
    GPON_MAC_GPON_hi_ploamu0_t gpon_ploamu0     = {0};
    GPON_MAC_GPON_hi_ploamu0_t gpon_ploamu0_org = {0};
    ca_uint32_t max_tries = 0;

    CA_ASSERT_RET(p_message, CA_E_PARAM);

    AAL_GPON_LOCK();

    if (ploam_type > 255 ) {
        AAL_GPON_UNLOCK();
        ca_printf("%s ploam_type should be withing 255\r\n",__func__);
        return CA_E_PARAM;
    }

    /* Wait for the transmission of the previous PLOAM */
    /* This case would be very rare so we can do a busy wait */
    do {
        gpon_ploamu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu);
        ca_mdelay(1);

        max_tries ++;
        if(max_tries == 10) {
            AAL_GPON_UNLOCK();
            ca_printf("%s Ploamd transmission pending\r\n",__func__);
            return CA_E_PARAM;
        }
    } while(gpon_ploamu.bf.reqd);

    /* Clear to send the next PLOAM */
    gpon_ploamu0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu0);


    /* Populate the data */
    /* Not going to do a read-modify-write here. Can save some memory cycles */
    /* 00-31 bits in ploamu2 --> p_message[6] - p_message[9]*/
    /* 32-63 bits in ploamu1 --> p_message[2] - p_message[5]*/
    AAL_PON_REG_WRITE((p_message[9] | (p_message[8] << 8) |
                      (p_message[7] << 16) | (p_message[6] << 24)),
                      GPON_MAC_GPON_hi_ploamu2);

    AAL_PON_REG_WRITE((p_message[5] | (p_message[4] << 8) |
                      (p_message[3] << 16) | (p_message[2] << 24)),
                      GPON_MAC_GPON_hi_ploamu1);

    /* 64-79 bits in ploamu0 --> p_message[1] - p_message[0]*/
    msg_frag = p_message[1] | (p_message[0] << 8);

    /* Modify the message */
    /* Directly using the enum */
    gpon_ploamu0.bf.message = (ca_uint32_t)ploam_type;
    /* Write the remaining data */
    gpon_ploamu0.bf.data = msg_frag;
    gpon_ploamu0.bf.onu = onu_id;

    AAL_PON_REG_WRITE(gpon_ploamu0.wrd, GPON_MAC_GPON_hi_ploamu0);

    /* All set to initiate the transmission */
    gpon_ploamu.bf.reqd = 1;
    if(ignore_hw_state == TRUE )
        gpon_ploamu.bf.reqd_ignore_state = 1;
    /* No need to do a compare write here. 'req' would always be '0' */
    AAL_PON_REG_WRITE(gpon_ploamu.wrd,GPON_MAC_GPON_hi_ploamu);

    /* Waiting for the processing by hardware transmission */
    do {
        ca_mdelay(1);
        gpon_ploamu.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_hi_ploamu);
        loopcount++; 
    } while( (gpon_ploamu.bf.reqd) && (loopcount < 5) );

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_ploam_msg_get(
    CA_IN  ca_device_id_t dev_id,
    CA_IN  ca_uint8_t     port_id, 
    CA_OUT ca_uint8_t     *p_message, 
    CA_OUT ca_uint8_t     *p_num)
{
    volatile ca_uint32_t msg_frag;
    volatile ca_uint8_t  *msg;
    volatile GPON_MAC_GPON_PLOAMD_FF_CTL_t ploamd_fifo_ctrl = {0};

    CA_ASSERT_RET(p_message, CA_E_PARAM);
    CA_ASSERT_RET(p_num, CA_E_PARAM);

    AAL_GPON_LOCK();

    *p_num = 14;

    ploamd_fifo_ctrl.wrd    = 0;
    ploamd_fifo_ctrl.bf.pop = 1;
    AAL_PON_REG_WRITE(ploamd_fifo_ctrl.wrd,GPON_MAC_GPON_PLOAMD_FF_CTL);

    msg = (volatile ca_uint8_t *)&msg_frag;
    msg_frag = AAL_PON_REG_READ(GPON_MAC_GPON_PLOAMD_FIFO0);
    p_message[13] = msg[0];
    p_message[12] = msg[1];
    p_message[11] = msg[2];
    p_message[10] = msg[3];
    /*ca_printf("\nmsg_frag is 0x%08x\n", msg_frag);*/
    msg_frag = AAL_PON_REG_READ(GPON_MAC_GPON_PLOAMD_FIFO1);
    p_message[9] = msg[0];
    p_message[8] = msg[1];
    p_message[7] = msg[2];
    p_message[6] = msg[3];
    /*ca_printf("msg_frag is 0x%08x\n", msg_frag);*/
    msg_frag = AAL_PON_REG_READ(GPON_MAC_GPON_PLOAMD_FIFO2);
    p_message[5] = msg[0];
    p_message[4] = msg[1];
    p_message[3] = msg[2];
    p_message[2] = msg[3];
    /*ca_printf("msg_frag is 0x%08x\n", msg_frag);*/
    msg_frag = AAL_PON_REG_READ(GPON_MAC_GPON_PLOAMD_FIFO3);
    p_message[1] = msg[0];
    p_message[0] = msg[1];
    /*ca_printf("msg_frag is 0x%04x\n", msg_frag);*/

    /*ca_printf("message is : \n");
    for(i=0; i<14; i++){
        ca_printf("%02x", p_message[i]);
    }
    ca_printf("\n");*/

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_mac_cfg_set(
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_gpon_mac_cfg_msk_t mask,
    CA_IN aal_gpon_mac_cfg_t     *cfg)
{
    GPON_MAC_GPON_onu_ctl_t onu_ctl = {0};
    GPON_MAC_GPON_onu_cfg_t onu_cfg = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    onu_ctl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu_ctl);
    onu_cfg.wrd = AAL_PON_REG_READ( GPON_MAC_GPON_onu_cfg);
    if (mask.s.no_up) {
        onu_ctl.bf.no_up = cfg->no_up;
    }
    if (mask.s.en) {
        onu_ctl.bf.en = cfg->en;
    }
    if (mask.s.stop) {
        onu_ctl.bf.stop = cfg->stop;
    }
    if (mask.s.omcc_padding) {
        onu_ctl.bf.omcc_padding = cfg->omcc_padding;
    }
    if (mask.s.dpath_rst) {
        onu_ctl.bf.dpath_rst = cfg->dpath_rst;
    }
    if (mask.s.aes_en) {
        onu_ctl.bf.aes_en = cfg->aes_en;
    }
    if (mask.s.ploam_crc_en) {
        onu_ctl.bf.ploam_crc_en = cfg->ploam_crc_en;
    }
    if (mask.s.pm_mode) {
        onu_ctl.bf.pm_mode = cfg->pm_mode;
    }
    if (mask.s.plen_match) {
        onu_ctl.bf.plen_match = cfg->plen_match;
    }
    if (mask.s.stop_us_wr) {
        onu_ctl.bf.stop_us_wr = cfg->stop_us_wr;
    }
    if (mask.s.stop_hw_plu) {
        onu_ctl.bf.stop_hw_plu = cfg->stop_hw_plu;
    }
    if (mask.s.sleep_mode) {
        onu_ctl.bf.sleep_mode = cfg->sleep_mode;
    }
    if (mask.s.sch_lead_time) {
        onu_ctl.bf.sch_lead_time = cfg->sch_lead_time;
    }
    if (mask.s.omci_crc_dis) {
        onu_cfg.bf.omci_crc_dis = cfg->omci_crc_dis;
    }
    if (mask.s.us_scramb_dis) {
        onu_cfg.bf.us_scramb_dis = cfg->us_scramb_dis;
    }
    if (mask.s.ds_scramb_dis) {
        onu_cfg.bf.ds_scramb_dis = cfg->ds_scramb_dis;
    }
    if (mask.s.dbru_blk_size) {
        onu_cfg.bf.dbru_blk_size = cfg->dbru_blk_size;
    }
    if (mask.s.laser_pre_bias) {
        onu_cfg.bf.laser_pre_bias = cfg->laser_pre_bias;
    }
    if (mask.s.laser_on) {
        onu_cfg.bf.laser_on = cfg->laser_on;
    }
    if (mask.s.plmd_ff_wr_ext) {
        onu_cfg.bf.plmd_ff_wr_ext = cfg->plmd_ff_wr_ext;
    }
    if (mask.s.laser_on_align) {
        onu_cfg.bf.laser_on_align = cfg->laser_on_align;
    }
    AAL_PON_REG_WRITE(onu_ctl.wrd, GPON_MAC_GPON_onu_ctl);
    AAL_PON_REG_WRITE(onu_cfg.wrd, GPON_MAC_GPON_onu_cfg);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_mac_cfg_get(
    CA_IN ca_device_id_t      device_id,
    CA_OUT aal_gpon_mac_cfg_t *cfg)
{
    GPON_MAC_GPON_onu_ctl_t onu_ctl = {0};
    GPON_MAC_GPON_onu_cfg_t onu_cfg = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    onu_ctl.wrd = AAL_PON_REG_READ( GPON_MAC_GPON_onu_ctl);
    onu_cfg.wrd = AAL_PON_REG_READ( GPON_MAC_GPON_onu_cfg);
    cfg->no_up          = onu_ctl.bf.no_up;
    cfg->en             = onu_ctl.bf.en;
    cfg->stop           = onu_ctl.bf.stop;
    cfg->omcc_padding   = onu_ctl.bf.omcc_padding;
    cfg->dpath_rst      = onu_ctl.bf.dpath_rst;
    cfg->aes_en         = onu_ctl.bf.aes_en;
    cfg->ploam_crc_en   = onu_ctl.bf.ploam_crc_en;
    cfg->pm_mode        = onu_ctl.bf.pm_mode;
    cfg->plen_match     = onu_ctl.bf.plen_match;
    cfg->stop_us_wr     = onu_ctl.bf.stop_us_wr;
    cfg->stop_hw_plu    = onu_ctl.bf.stop_hw_plu;
    cfg->sleep_mode     = onu_ctl.bf.sleep_mode;
    cfg->sch_lead_time  = onu_ctl.bf.sch_lead_time;
    cfg->omci_crc_dis   = onu_cfg.bf.omci_crc_dis;
    cfg->us_scramb_dis  = onu_cfg.bf.us_scramb_dis;
    cfg->ds_scramb_dis  = onu_cfg.bf.ds_scramb_dis;
    cfg->dbru_blk_size  = onu_cfg.bf.dbru_blk_size;
    cfg->laser_pre_bias = onu_cfg.bf.laser_pre_bias;
    cfg->laser_on       = onu_cfg.bf.laser_on;
    cfg->plmd_ff_wr_ext = onu_cfg.bf.plmd_ff_wr_ext;
    cfg->laser_on_align = onu_cfg.bf.laser_on_align;

    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_disable_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask)
{
    GPON_MAC_GPON_ploamd_disable_t ploamd_disable = {0};

    AAL_GPON_LOCK();

    ploamd_disable.wrd     = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_disable);
    ploamd_disable.bf.mask = mask;
    AAL_PON_REG_WRITE(ploamd_disable.wrd, GPON_MAC_GPON_ploamd_disable);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_disable_mask_get(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_OUT ca_uint32_t   *mask)
{
    GPON_MAC_GPON_ploamd_disable_t ploamd_disable = {0};

    ploamd_disable.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_disable);
    *mask = ploamd_disable.bf.mask;
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_fifo_cap_ena_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask)
{
    GPON_MAC_GPON_ploamd_fifo_wr_t ploamd_fifo_ena = {0};

    AAL_GPON_LOCK();

    ploamd_fifo_ena.wrd     = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_fifo_wr);
    ploamd_fifo_ena.bf.mask = mask;
    AAL_PON_REG_WRITE(ploamd_fifo_ena.wrd,GPON_MAC_GPON_ploamd_fifo_wr);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamd_fifo_cap_ena_mask_get(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_OUT ca_uint32_t   *mask)
{
    GPON_MAC_GPON_ploamd_fifo_wr_t ploamd_fifo_ena = {0};

    ploamd_fifo_ena.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamd_fifo_wr);
    *mask = ploamd_fifo_ena.bf.mask;
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamu_disable_mask_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_IN ca_uint32_t    mask)
{
    GPON_MAC_GPON_ploamu_disable_t ploamu_disable = {0};

    AAL_GPON_LOCK();

    ploamu_disable.wrd     = AAL_PON_REG_READ(GPON_MAC_GPON_ploamu_disable);
    ploamu_disable.bf.mask = mask;
    AAL_PON_REG_WRITE(ploamu_disable.wrd,GPON_MAC_GPON_ploamu_disable);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploamu_disable_mask_get(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint8_t     port_id,
    CA_OUT ca_uint32_t   *mask)
{
    GPON_MAC_GPON_ploamu_disable_t ploamu_disable;

    ploamu_disable.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ploamu_disable);
    *mask = ploamu_disable.bf.mask;
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_pkt_process_ctrl_set(
    CA_IN ca_device_id_t                     device_id,
    CA_IN aal_gpon_ds_pkt_process_ctrl_msk_t mask,
    CA_IN aal_gpon_ds_pkt_process_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_DS_PROCESS_t ds_pkt_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    ds_pkt_ctrl.wrd = AAL_PON_REG_READ( GPON_MAC_GPON_DS_PROCESS);
    if (mask.s.ptp_en) {
        ds_pkt_ctrl.bf.ptp_en = cfg->ptp_en;
    }
    if (mask.s.cfm_en) {
        ds_pkt_ctrl.bf.cfm_en = cfg->cfm_en;
    }
    if (mask.s.ptp_check_da) {
        ds_pkt_ctrl.bf.ptp_check_da = cfg->ptp_check_da;
    }
    if (mask.s.cfm_check_da) {
        ds_pkt_ctrl.bf.cfm_check_da = cfg->cfm_check_da;
    }
    AAL_PON_REG_WRITE (ds_pkt_ctrl.wrd, GPON_MAC_GPON_DS_PROCESS);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_pkt_process_ctrl_get(
    CA_IN  ca_device_id_t                     device_id,
    CA_OUT aal_gpon_ds_pkt_process_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_DS_PROCESS_t ds_pkt_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    ds_pkt_ctrl.wrd   = AAL_PON_REG_READ(GPON_MAC_GPON_DS_PROCESS);
    cfg->ptp_en       = ds_pkt_ctrl.bf.ptp_en;
    cfg->cfm_en       = ds_pkt_ctrl.bf.cfm_en;
    cfg->ptp_check_da = ds_pkt_ctrl.bf.ptp_check_da;
    cfg->cfm_check_da = ds_pkt_ctrl.bf.cfm_check_da;
    return CA_E_OK;
}


ca_status_t aal_gpon_tcont_get(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_boolean_t         sw_plm_en,
    CA_IN  ca_uint32_t          allocid,
    CA_OUT aal_gpon_tcont_cfg_t *cfg)
{
    GPON_MAC_GPON_TCONT_DATA_t tcont_data = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);
    CA_ASSERT_RET(cfg->tcont_index < AAL_GPON_SYSTEM_MAX_TCONT_NUM, CA_E_PARAM);

    if (allocid >= AAL_GPON_SYSTEM_MAX_ALLOCID) {
        ca_printf("%s Alloc id value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }
    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_GPN_TCONT_REG_INDIRECT_READ(sw_plm_en, allocid, GPON_MAC_GPON_TCONT_ACCESS);

    tcont_data.wrd   = AAL_PON_REG_READ(GPON_MAC_GPON_TCONT_DATA);
    cfg->tcont_index = tcont_data.bf.index;
    cfg->omci_en     = tcont_data.bf.omci_en;
    cfg->ploam_en    = tcont_data.bf.ploam_en;
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


/*need modify code for register access changed!*/
ca_status_t aal_gpon_tcont_set(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_boolean_t              sw_plm_en,
    CA_IN ca_uint32_t               allocid,
    CA_IN aal_gpon_tcont_cfg_msk_t  mask,
    CA_IN aal_gpon_tcont_cfg_t     *cfg)
{
    GPON_MAC_GPON_TCONT_DATA_t tcont_data = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);
    CA_ASSERT_RET(cfg->tcont_index < AAL_GPON_SYSTEM_MAX_TCONT_NUM, CA_E_PARAM);

    if (allocid >= AAL_GPON_SYSTEM_MAX_ALLOCID) {
      ca_printf("%s Alloc id value out of range \r\n",__func__);
      return CA_E_PARAM; 
    }

    AAL_GPON_LOCK();
    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_GPN_TCONT_REG_INDIRECT_READ(sw_plm_en, allocid, GPON_MAC_GPON_TCONT_ACCESS);
    tcont_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_TCONT_DATA);

    if (mask.s.tcont_index) {
        tcont_data.bf.index = cfg->tcont_index;
#ifdef CONFIG_LUNA_G3_SERIES
        gpon_tmp_alloc_id_cfg[cfg->tcont_index].tcont_index = allocid;
#endif
    }
    if (mask.s.omci_en) {
        tcont_data.bf.omci_en = cfg->omci_en;
#ifdef CONFIG_LUNA_G3_SERIES
        gpon_tmp_alloc_id_cfg[cfg->tcont_index].omci_en = cfg->omci_en;
#endif
    }
    if (mask.s.ploam_en) {
        tcont_data.bf.ploam_en = cfg->ploam_en;
#ifdef CONFIG_LUNA_G3_SERIES
        gpon_tmp_alloc_id_cfg[cfg->tcont_index].ploam_en = cfg->ploam_en;
#endif
    }

    AAL_PON_REG_WRITE(tcont_data.wrd, (GPON_MAC_GPON_TCONT_DATA));
    AAL_GPN_TCONT_REG_INDIRECT_WRITE(sw_plm_en, allocid, GPON_MAC_GPON_TCONT_ACCESS);
    AAL_GPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_gpon_ds_gem_port_get(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_boolean_t            sw_aes_en,
    CA_IN  ca_uint16_t             gem_port_id,
    CA_OUT aal_gpon_gem_port_cfg_t *cfg)
{
    GPON_MAC_GPON_DS_GEM_PORT_DATA_t gem_port_data = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    if (gem_port_id >= AAL_GPON_SYSTEM_MAX_GEM_PORTS) {
        ca_printf("%s Gem port id value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }
    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_DS_GEM_PORT_REG_INDIRECT_READ(sw_aes_en, gem_port_id, GPON_MAC_GPON_DS_GEM_PORT_ACCESS);

    gem_port_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_GEM_PORT_DATA);
    cfg->gem_index    = gem_port_data.bf.index;
    cfg->tdm          = gem_port_data.bf.tdm;
    cfg->aes          = gem_port_data.bf.aes;
    cfg->vld          = gem_port_data.bf.vld;
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


/*need modify code for register access changed!*/
ca_status_t aal_gpon_ds_gem_port_set(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_boolean_t                sw_aes_en,
    CA_IN ca_uint32_t                 gem_port_id,
    CA_IN aal_gpon_gem_port_cfg_msk_t mask,
    CA_IN aal_gpon_gem_port_cfg_t     *cfg)
{
    GPON_MAC_GPON_DS_GEM_PORT_DATA_t gem_port_data = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);
    CA_ASSERT_RET(cfg->gem_index < AAL_GPON_SYSTEM_MAX_GEM_NUM, CA_E_PARAM);

    if (gem_port_id >= AAL_GPON_SYSTEM_MAX_GEM_PORTS) {
        ca_printf("%s Gem port id value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }

    AAL_GPON_LOCK();
    AAL_DS_GEM_PORT_REG_INDIRECT_READ(sw_aes_en, gem_port_id, GPON_MAC_GPON_DS_GEM_PORT_ACCESS);
    gem_port_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_GEM_PORT_DATA);

    if (mask.s.gem_index) {
        gem_port_data.bf.index = cfg->gem_index;
    }
    if (mask.s.tdm) {
        gem_port_data.bf.tdm = cfg->tdm;
    }
    if (mask.s.aes) {
        gem_port_data.bf.aes = cfg->aes;
    }
    if (mask.s.vld) {
        gem_port_data.bf.vld = cfg->vld;
    }

    AAL_PON_REG_WRITE(gem_port_data.wrd, (GPON_MAC_GPON_DS_GEM_PORT_DATA));
    AAL_DS_GEM_PORT_REG_INDIRECT_WRITE(sw_aes_en, gem_port_id, GPON_MAC_GPON_DS_GEM_PORT_ACCESS);
    AAL_GPON_UNLOCK();

#if CONFIG_LUNA_G3_SERIES
    //keep ds gem port id
    tmp_dsfp_gem_cfg[cfg->gem_index].gem_index = ((cfg->vld)? gem_port_id : 0);
    tmp_dsfp_gem_cfg[cfg->gem_index].vld = cfg->vld;
#endif

    return CA_E_OK;
}


ca_status_t aal_gpon_ds_gem_port_mib_get(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_uint16_t                 gem_index,
    CA_IN ca_uint8_t                  op_code,
    CA_OUT aal_gpon_ds_gem_port_mib_t *cfg)
{
    GPON_MAC_GPON_DS_PORT_DATA0_t ds_port_data0 = {0};
    GPON_MAC_GPON_DS_PORT_DATA1_t ds_port_data1 = {0};
    GPON_MAC_GPON_DS_PORT_DATA2_t ds_port_data2 = {0};
    GPON_MAC_GPON_DS_PORT_DATA3_t ds_port_data3 = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    if (gem_index > 0x3FF) {
        ca_printf("%s Gem port index value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }
    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_GPN_DS_PORT_MIB_REG_INDIRECT_READ(gem_index, GPON_MAC_GPON_DS_PORT_ACCESS);

    ds_port_data0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_PORT_DATA0);
    ds_port_data1.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_PORT_DATA1);
    ds_port_data2.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_PORT_DATA2);
    ds_port_data3.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DS_PORT_DATA3);
    cfg->pcnt         = ds_port_data0.bf.pcnt;
    cfg->fcnt         = ds_port_data1.bf.fcnt;
    cfg->bcnt         = ds_port_data2.bf.bcnt|((ca_uint64_t)ds_port_data3.bf.bcnt <<32);

    //use the same value as us
    if (0x2 == op_code)
    {
        //read clear
        AAL_GPN_DS_PORT_MIB_REG_INDIRECT_WRITE(gem_index, GPON_MAC_GPON_DS_PORT_ACCESS);
        AAL_PON_REG_WRITE(0, GPON_MAC_GPON_DS_PORT_DATA0);
        AAL_PON_REG_WRITE(0, GPON_MAC_GPON_DS_PORT_DATA1);
        AAL_PON_REG_WRITE(0, GPON_MAC_GPON_DS_PORT_DATA2);
        AAL_PON_REG_WRITE(0, GPON_MAC_GPON_DS_PORT_DATA3);
    }

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_us_gem_port_mib_get(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint8_t                  op_code,
    CA_IN  ca_uint8_t                  sel,
    CA_OUT aal_gpon_us_gem_port_mib_t  *cfg)
{
    GPON_MAC_GPON_US_MIB_DATA0_t us_mib_data0 = {0};
    GPON_MAC_GPON_US_MIB_DATA1_t us_mib_data1 = {0};
    GPON_MAC_GPON_US_MIB_DATA2_t us_mib_data2 = {0};
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    if (sel > 0x52) {
        ca_printf("%s sel value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }
    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_GPN_US_PORT_MIB_REG_INDIRECT_READ(op_code, sel, GPON_MAC_GPON_US_MIB_ACCESS);

    us_mib_data0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_US_MIB_DATA0);
    us_mib_data1.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_US_MIB_DATA1);
    us_mib_data2.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_US_MIB_DATA2);
    cfg->fcnt        = us_mib_data0.bf.fcnt;
    cfg->bcnt        = us_mib_data1.bf.bcnt | ((ca_uint64_t)us_mib_data2.bf.bcnt <<32);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ploam_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     op_code,
    CA_IN  ca_uint16_t    sel,
    CA_OUT ca_uint8_t     *cnt)
{
    GPON_MAC_GPON_PLM_MIB_DATA_t plm_mib_data = {0};

    CA_ASSERT_RET(cnt, CA_E_PARAM);

    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_GPN_PLOAM_MIB_REG_INDIRECT_READ(op_code, sel, GPON_MAC_GPON_PLM_MIB_ACCESS);

    plm_mib_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_PLM_MIB_DATA);
    *cnt = plm_mib_data.bf.data;

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_us_gem_port_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     gem_index,
    CA_OUT ca_uint16_t    *gem_id)
{
    GPON_MAC_GPON_US_PORT_ID_DATA_t us_gem_port = {0};

    CA_ASSERT_RET(gem_id, CA_E_PARAM);

    if (gem_index >= AAL_GPON_SYSTEM_MAX_GEM_NUM) {
        ca_printf("%s Gem port index value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }
    AAL_GPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_US_GEM_PORT_REG_INDIRECT_READ(gem_index, GPON_MAC_GPON_US_PORT_ID_ACCESS);
    us_gem_port.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_US_PORT_ID_DATA);
    *gem_id = us_gem_port.bf.id;
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_us_gem_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     gem_index,
    CA_IN ca_uint16_t    gem_id)
{
    GPON_MAC_GPON_US_PORT_ID_DATA_t us_gem_port = {0};

    CA_ASSERT_RET(gem_index < AAL_GPON_SYSTEM_MAX_GEM_NUM, CA_E_PARAM);

    if (gem_id >= AAL_GPON_SYSTEM_MAX_GEM_PORTS) {
        ca_printf("%s Gem port index value out of range \r\n",__func__);
        return CA_E_PARAM; 
    }

    AAL_GPON_LOCK();
    AAL_US_GEM_PORT_REG_INDIRECT_READ(gem_index, GPON_MAC_GPON_US_PORT_ID_ACCESS);
    us_gem_port.wrd   = AAL_PON_REG_READ(GPON_MAC_GPON_US_PORT_ID_DATA);
    us_gem_port.bf.id = gem_id;
    AAL_PON_REG_WRITE(us_gem_port.wrd, (GPON_MAC_GPON_US_PORT_ID_DATA));
    AAL_US_GEM_PORT_REG_INDIRECT_WRITE(gem_index, GPON_MAC_GPON_US_PORT_ID_ACCESS);
    AAL_GPON_UNLOCK();

#if CONFIG_LUNA_G3_SERIES
    //keep us gem port id
    tmp_usbg_gem_cfg[gem_index].gem_index = gem_id;
#endif

    return CA_E_OK;
}


ca_status_t aal_gpon_ds_pkt_bypass_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN aal_gpon_ds_pkt_bypass_cfg_msk_t mask,
    CA_IN aal_gpon_ds_pkt_bypass_cfg_t     *cfg)
{
    GPON_MAC_GPON_bypass_t ds_pkt_bypass = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    ds_pkt_bypass.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_bypass);
    if (mask.s.sw_en) {
        ds_pkt_bypass.bf.sw_en = cfg->sw_en;
    }
    if (mask.s.sw_dpid) {
        ds_pkt_bypass.bf.sw_dpid = cfg->sw_dpid;
    }
    if (mask.s.sw_cos) {
        ds_pkt_bypass.bf.sw_cos = cfg->sw_cos;
    }
    if (mask.s.ptp_dpid) {
        ds_pkt_bypass.bf.ptp_dpid = cfg->ptp_dpid;
    }
    if (mask.s.ptp_cos) {
        ds_pkt_bypass.bf.ptp_cos = cfg->ptp_cos;
    }
    if (mask.s.omci_dpid) {
        ds_pkt_bypass.bf.omci_dpid = cfg->omci_dpid;
    }
    if (mask.s.omci_cos) {
        ds_pkt_bypass.bf.omci_cos = cfg->omci_cos;
    }

    AAL_PON_REG_WRITE (ds_pkt_bypass.wrd, GPON_MAC_GPON_bypass);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_ds_pkt_bypass_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_gpon_ds_pkt_bypass_cfg_t  *cfg)
{
    GPON_MAC_GPON_bypass_t ds_pkt_bypass = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    ds_pkt_bypass.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_ds);
    cfg->sw_en        = ds_pkt_bypass.bf.sw_en;
    cfg->sw_dpid      = ds_pkt_bypass.bf.sw_dpid;
    cfg->sw_cos       = ds_pkt_bypass.bf.sw_cos;
    cfg->ptp_dpid     = ds_pkt_bypass.bf.ptp_dpid;
    cfg->ptp_cos      = ds_pkt_bypass.bf.ptp_cos;
    cfg->omci_dpid    = ds_pkt_bypass.bf.omci_dpid;
    cfg->omci_cos     = ds_pkt_bypass.bf.omci_cos;
    return CA_E_OK;
}


ca_status_t aal_gpon_sw_ctrl_dba_set(
    CA_IN ca_device_id_t             device_id,
    CA_IN aal_gpon_sw_dbru_cfg_msk_t mask,
    CA_IN aal_gpon_sw_dbru_cfg_t     *cfg)
{
    GPON_MAC_GPON_sw_dbru_t sw_dbru = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    sw_dbru.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_sw_dbru);
    if (mask.s.bufocc_mode) {
        sw_dbru.bf.bufocc_mode = cfg->bufocc_mode;
    }
    if (mask.s.tcon_residue_en) {
        sw_dbru.bf.tcon_residue_en = cfg->tcon_residue_en;
    }
    if (mask.s.enble) {
        sw_dbru.bf.enble = cfg->enble;
    }
    if (mask.s.data) {
        sw_dbru.bf.data = cfg->data;
    }

    AAL_PON_REG_WRITE (sw_dbru.wrd, GPON_MAC_GPON_sw_dbru);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_sw_ctrl_dba_get(
    CA_IN  ca_device_id_t         device_id,
    CA_OUT aal_gpon_sw_dbru_cfg_t *cfg)
{
    GPON_MAC_GPON_sw_dbru_t sw_dbru = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    sw_dbru.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_sw_dbru);
    cfg->bufocc_mode     = sw_dbru.bf.bufocc_mode;
    cfg->tcon_residue_en = sw_dbru.bf.tcon_residue_en;
    cfg->enble           = sw_dbru.bf.enble;
    cfg->data            = sw_dbru.bf.data;
    return CA_E_OK;
}


ca_status_t aal_gpon_scratch_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *scratch)
{
    GPON_MAC_GPON_scratch_t scratch_data = {0};

    CA_ASSERT_RET(scratch, CA_E_PARAM);

    scratch_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_scratch);
    *scratch = scratch_data.bf.scratch;
    return CA_E_OK;
}


ca_status_t aal_gpon_scratch_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    scratch)
{
    GPON_MAC_GPON_scratch_t scratch_data = {0};

    AAL_GPON_LOCK();

    scratch_data.wrd        = AAL_PON_REG_READ(GPON_MAC_GPON_scratch);
    scratch_data.bf.scratch = scratch ;
    AAL_PON_REG_WRITE (scratch_data.wrd, GPON_MAC_GPON_scratch);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_dbru_bufocc_tbl_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     table_pointer,
    CA_IN  ca_uint8_t     index,
    CA_OUT ca_uint16_t    *buf_occ)
{
    GPON_MAC_GPON_DBRU_BUFOCC_TBL_DATA_t dbru_buf_tbl = {0};

    CA_ASSERT_RET(buf_occ, CA_E_PARAM);
    CA_ASSERT_RET(index <= 0xF, CA_E_PARAM);

    AAL_GPON_LOCK();

    AAL_GPN_DBRU_BUFOCC_TBL_REG_INDIRECT_READ(table_pointer, index, GPON_MAC_GPON_DBRU_BUFOCC_TBL_ACCESS);
    dbru_buf_tbl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_DBRU_BUFOCC_TBL_DATA);
    *buf_occ         = dbru_buf_tbl.bf.BufOcc;
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


/*need modify code for register access changed!*/
ca_status_t aal_gpon_dbru_bufocc_tbl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     table_pointer,
    CA_IN ca_uint8_t     index,
    CA_IN ca_uint16_t    buf_occ)
{
    GPON_MAC_GPON_DBRU_BUFOCC_TBL_DATA_t dbru_buf_tbl = {0};

    CA_ASSERT_RET(index <= 0xF, CA_E_PARAM);

    AAL_GPON_LOCK();
    AAL_GPN_DBRU_BUFOCC_TBL_REG_INDIRECT_READ(table_pointer, index, GPON_MAC_GPON_DBRU_BUFOCC_TBL_ACCESS);
    dbru_buf_tbl.wrd       = AAL_PON_REG_READ(GPON_MAC_GPON_DBRU_BUFOCC_TBL_DATA);
    dbru_buf_tbl.bf.BufOcc = buf_occ;
    AAL_PON_REG_WRITE(dbru_buf_tbl.wrd, (GPON_MAC_GPON_DBRU_BUFOCC_TBL_DATA));
    AAL_GPN_DBRU_BUFOCC_TBL_REG_INDIRECT_WRITE(table_pointer, index, GPON_MAC_GPON_DBRU_BUFOCC_TBL_ACCESS);
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_mac_ctrl_set(
    CA_IN ca_device_id_t             device_id,
    CA_IN aal_gpon_mac_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_ctrl_cfg_t    *cfg)
{
    GPON_MAC_GPON_CTRL_t mac_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    mac_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_CTRL);
    if (mask.s.flush_en) {
        mac_ctrl.bf.flush_en = cfg->flush_en;
    }
    if (mask.s.flush_id) {
        mac_ctrl.bf.flush_id = cfg->flush_id;
    }
    if (mask.s.mapper_frame_no_wait_stt) {
        mac_ctrl.bf.mapper_frame_no_wait_stt = cfg->mapper_frame_no_wait_stt;
    }
    if (mask.s.mapper_contiguous_frame_no_wait_stt) {
        mac_ctrl.bf.mapper_contiguous_frame_no_wait_stt = cfg->mapper_contiguous_frame_no_wait_stt;
    }

    AAL_PON_REG_WRITE (mac_ctrl.wrd, GPON_MAC_GPON_CTRL);
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_mac_ctrl_get(
    CA_IN  ca_device_id_t          device_id,
    CA_OUT aal_gpon_mac_ctrl_cfg_t *cfg)
{
    GPON_MAC_GPON_CTRL_t mac_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    mac_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_CTRL);
    cfg->flush_en                            = mac_ctrl.bf.flush_en;
    cfg->flush_id                            = mac_ctrl.bf.flush_id;
    cfg->mapper_frame_no_wait_stt            = mac_ctrl.bf.mapper_frame_no_wait_stt;
    cfg->mapper_contiguous_frame_no_wait_stt = mac_ctrl.bf.mapper_contiguous_frame_no_wait_stt;
    return CA_E_OK;
}


ca_status_t aal_gpon_sup_frame_value_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *value)
{
    GPON_MAC_GPON_SUPER_FRAME_MATCH_VAL_t sup_frame_data = {0};

    CA_ASSERT_RET(value, CA_E_PARAM);

    sup_frame_data.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_SUPER_FRAME_MATCH_VAL);
    *value = sup_frame_data.bf.sfc;
    return CA_E_OK;
}


ca_status_t aal_gpon_sup_frame_value_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    value)
{
    GPON_MAC_GPON_SUPER_FRAME_MATCH_VAL_t sup_frame_data = {0};

    AAL_GPON_LOCK();

    sup_frame_data.wrd    = AAL_PON_REG_READ(GPON_MAC_GPON_SUPER_FRAME_MATCH_VAL);
    sup_frame_data.bf.sfc = value ;
    AAL_PON_REG_WRITE (sup_frame_data.wrd, GPON_MAC_GPON_SUPER_FRAME_MATCH_VAL);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_fec_ctrl_set(
    CA_IN ca_device_id_t                  device_id,
    CA_IN aal_gpon_mac_fec_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_fec_ctrl_cfg_t     *cfg)
{
    GPON_MAC_FEC_FEC_CTRL_t fec_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    fec_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_CTRL);
    if (mask.s.halt_cor) {
        fec_ctrl.bf.halt_cor = cfg->halt_cor;
    }
    if (mask.s.halt_unc) {
        fec_ctrl.bf.halt_unc = cfg->halt_unc;
    }
    if (mask.s.cor_ctr_clr) {
        fec_ctrl.bf.cor_ctr_clr = cfg->cor_ctr_clr;
    }
    if (mask.s.unc_ctr_clr) {
        fec_ctrl.bf.unc_ctr_clr = cfg->unc_ctr_clr;
    }

    AAL_PON_REG_WRITE(fec_ctrl.wrd, GPON_MAC_FEC_FEC_CTRL);
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_fec_ctrl_get(
    CA_IN  ca_device_id_t              device_id,
    CA_OUT aal_gpon_mac_fec_ctrl_cfg_t *cfg)
{
    GPON_MAC_FEC_FEC_CTRL_t fec_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    fec_ctrl.wrd     = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_CTRL);
    cfg->halt_cor    = fec_ctrl.bf.halt_cor;
    cfg->halt_unc    = fec_ctrl.bf.halt_unc;
    cfg->cor_ctr_clr = fec_ctrl.bf.cor_ctr_clr;
    cfg->unc_ctr_clr = fec_ctrl.bf.unc_ctr_clr;
    return CA_E_OK;
}


ca_status_t aal_gpon_fec_misc_status_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_gpon_mac_fec_misc_status_t *cfg)
{
    GPON_MAC_FEC_FEC_MISC_STATUS_t fec_misc_status = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    fec_misc_status.wrd  = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_MISC_STATUS);
    cfg->fec_dacor_ack_q = fec_misc_status.bf.fec_dacor_ack_q;
    cfg->fec_uncorr      = fec_misc_status.bf.fec_uncorr;
    cfg->fec_errors      = fec_misc_status.bf.fec_errors;
    return CA_E_OK;
}


ca_status_t aal_gpon_fec_mibs_get(
    CA_IN  ca_device_id_t          device_id,
    CA_OUT aal_gpon_mac_fec_mibs_t *cfg)
{
    GPON_MAC_FEC_FEC_Block_Correctable_Error_Counter_t   correct_cnt      = {0};
    GPON_MAC_FEC_FEC_Block_Uncorrectable_Error_Counter_t uncorrect_cnt    = {0};
    GPON_MAC_FEC_FEC_Block_No_Error_Counter_t            no_err_cnt       = {0};
    GPON_MAC_FEC_FEC_Block_Counter_t                     blk_cnt          = {0};
    GPON_MAC_FEC_FEC_Corr_Byte_Counter_t                 correct_byte_cnt = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    correct_cnt.wrd        = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_Block_Correctable_Error_Counter);
    uncorrect_cnt.wrd      = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_Block_Uncorrectable_Error_Counter);
    no_err_cnt.wrd         = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_Block_No_Error_Counter);
    blk_cnt.wrd            = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_Block_Counter);
    correct_byte_cnt.wrd   = AAL_PON_REG_READ(GPON_MAC_FEC_FEC_Corr_Byte_Counter);
    cfg->fec_rcnt          = correct_cnt.bf.FEC_RCNT;
    cfg->fec_ucnt          = uncorrect_cnt.bf.FEC_UCNT;
    cfg->fec_noerr         = no_err_cnt.bf.FEC_NOERR;
    cfg->fec_blk_cnt       = blk_cnt.bf.FEC_BLK_CNT;
    cfg->fec_corr_byte_cnt = correct_byte_cnt.bf.FEC_CORR_BYTE_CNT;
    return CA_E_OK;
}


ca_status_t aal_gpon_pucif_ctrl_set(
    CA_IN ca_device_id_t                    device_id,
    CA_IN aal_gpon_mac_pucif_ctrl_cfg_msk_t mask,
    CA_IN aal_gpon_mac_pucif_ctrl_cfg_t     *cfg)
{
    GPON_MAC_PUCIF_CTRL_t pucif_ctrl = {0};

    AAL_GPON_CFG_MASK_CK();
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GPON_LOCK();

    pucif_ctrl.wrd = AAL_PON_REG_READ(GPON_MAC_PUCIF_CTRL);
    if (mask.s.cntr_inccfg) {
        pucif_ctrl.bf.cntr_inccfg = cfg->cntr_inccfg;
    }
    if (mask.s.sch_to_threshold) {
        pucif_ctrl.bf.sch_to_threshold = cfg->sch_to_threshold;
    }
    if (mask.s.single_thread) {
        pucif_ctrl.bf.single_thread = cfg->single_thread;
    }
    if (mask.s.cntr2_event_sel) {
        pucif_ctrl.bf.cntr2_event_sel = cfg->cntr2_event_sel;
    }
    if (mask.s.cntr1_event_sel) {
        pucif_ctrl.bf.cntr1_event_sel = cfg->cntr1_event_sel;
    }
    if (mask.s.cntr0_event_sel) {
        pucif_ctrl.bf.cntr0_event_sel = cfg->cntr0_event_sel;
    }
    if (mask.s.cntr_tconid_en) {
        pucif_ctrl.bf.cntr_tconid_en = cfg->cntr_tconid_en;
    }
    if (mask.s.cntr_tconid) {
        pucif_ctrl.bf.cntr_tconid = cfg->cntr_tconid;
    }
    
    AAL_PON_REG_WRITE(pucif_ctrl.wrd, GPON_MAC_PUCIF_CTRL);
    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_pucif_ctrl_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_gpon_mac_pucif_ctrl_cfg_t *cfg)
{
    GPON_MAC_PUCIF_CTRL_t pucif_ctrl = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    pucif_ctrl.wrd        = AAL_PON_REG_READ(GPON_MAC_PUCIF_CTRL);
    cfg->cntr_inccfg      = pucif_ctrl.bf.cntr_inccfg;
    cfg->sch_to_threshold = pucif_ctrl.bf.sch_to_threshold;
    cfg->single_thread    = pucif_ctrl.bf.single_thread;
    cfg->cntr2_event_sel  = pucif_ctrl.bf.cntr2_event_sel;
    cfg->cntr1_event_sel  = pucif_ctrl.bf.cntr1_event_sel;
    cfg->cntr0_event_sel  = pucif_ctrl.bf.cntr0_event_sel;
    cfg->cntr_tconid_en   = pucif_ctrl.bf.cntr_tconid_en;
    cfg->cntr_tconid      = pucif_ctrl.bf.cntr_tconid;

    return CA_E_OK;
}


ca_status_t aal_gpon_pucif_cnt_get(
    CA_IN  ca_device_id_t           device_id,
    CA_OUT aal_gpon_mac_pucif_cnt_t *cfg)
{
    GPON_MAC_PUCIF_CNTR0_t cnt0 = {0};
    GPON_MAC_PUCIF_CNTR1_t cnt1 = {0};
    GPON_MAC_PUCIF_CNTR2_t cnt2 = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    cnt0.wrd = AAL_PON_REG_READ(GPON_MAC_PUCIF_CNTR0);
    cnt1.wrd = AAL_PON_REG_READ(GPON_MAC_PUCIF_CNTR1);
    cnt2.wrd = AAL_PON_REG_READ(GPON_MAC_PUCIF_CNTR2);

    cfg->pmac2puc_cnt     = cnt0.bf.cnt;
    cfg->sob_puc2pmac_cnt = cnt1.bf.cnt;
    cfg->sof_puc2pmac_cnt = cnt2.bf.cnt;
    return CA_E_OK;
}


ca_status_t aal_gpon_pucif_sts_get(
    CA_IN  ca_device_id_t           device_id,
    CA_OUT aal_gpon_mac_pucif_sts_t *cfg)
{
    GPON_MAC_PUCIF_STS_t pucif_sts = {0};

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    pucif_sts.wrd    = AAL_PON_REG_READ(GPON_MAC_PUCIF_STS);
    cfg->data_gemid  = pucif_sts.bf.data_gemid;
    cfg->data_tconid = pucif_sts.bf.data_tconid;
    cfg->sch_tconid  = pucif_sts.bf.sch_tconid;
    return CA_E_OK;
}


ca_status_t aal_gpon_puc_protect_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint8_t     *pucif_hang_tcon_id)
{
    GPON_MAC_PUCIF_PROTECT_t pucif_protect = {0};

    CA_ASSERT_RET(pucif_hang_tcon_id, CA_E_PARAM);

    pucif_protect.wrd = AAL_PON_REG_READ(GPON_MAC_PUCIF_PROTECT);
    if(pucif_protect.bf.pucif_hang){
        *pucif_hang_tcon_id = pucif_protect.bf.pucif_hang_tcon_id;
    }
    return CA_E_OK;
}


ca_status_t aal_gpon_active_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean active)
{
    GPON_MAC_GPON_onu_ctl_t onu_ctl = {0};

    AAL_GPON_LOCK();

    onu_ctl.wrd   = AAL_PON_REG_READ(GPON_MAC_GPON_onu_ctl);
    onu_ctl.bf.en = active;
    AAL_PON_REG_WRITE(onu_ctl.wrd, GPON_MAC_GPON_onu_ctl);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_active_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_boolean     *active)
{
    GPON_MAC_GPON_onu_ctl_t onu_ctl = {0};

    AAL_GPON_LOCK();

    onu_ctl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_onu_ctl);
    *active     = onu_ctl.bf.en;

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_passwd_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN aal_gpon_passwd_t *passwd)
{
    GPON_MAC_GPON_pfrag0_t pfrag0 = {0};
    GPON_MAC_GPON_pfrag1_t pfrag1 = {0};
    GPON_MAC_GPON_pfrag2_t pfrag2 = {0};

    CA_ASSERT_RET(passwd, CA_E_PARAM);

    AAL_GPON_LOCK();

    pfrag0.wrd = passwd->passwd[1] | (passwd->passwd[0] << 8);
    pfrag1.wrd = passwd->passwd[5] | (passwd->passwd[4] << 8) | (passwd->passwd[3] << 16) | (passwd->passwd[2] << 24);
    pfrag2.wrd = passwd->passwd[9] | (passwd->passwd[8] << 8) | (passwd->passwd[7] << 16) | (passwd->passwd[6] << 24);

    AAL_PON_REG_WRITE(pfrag0.wrd, GPON_MAC_GPON_pfrag0);
    AAL_PON_REG_WRITE(pfrag1.wrd, GPON_MAC_GPON_pfrag1);
    AAL_PON_REG_WRITE(pfrag2.wrd, GPON_MAC_GPON_pfrag2);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_passwd_get(
    CA_IN  ca_device_id_t     device_id,
    CA_OUT aal_gpon_passwd_t *passwd)
{
    GPON_MAC_GPON_pfrag0_t pfrag0 = {0};
    GPON_MAC_GPON_pfrag1_t pfrag1 = {0};
    GPON_MAC_GPON_pfrag2_t pfrag2 = {0};

    CA_ASSERT_RET(passwd, CA_E_PARAM);

    AAL_GPON_LOCK();

    pfrag2.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_pfrag2);
    passwd->passwd[9] = (ca_uint8_t) (pfrag2.wrd & 0x000000FF);
    passwd->passwd[8] = (ca_uint8_t) ((pfrag2.wrd & 0x0000FF00) >> 8);
    passwd->passwd[7] = (ca_uint8_t) ((pfrag2.wrd & 0x00FF0000) >> 16);
    passwd->passwd[6] = (ca_uint8_t) ((pfrag2.wrd & 0xFF000000) >> 24);


    pfrag1.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_pfrag1);
    passwd->passwd[5] = (ca_uint8_t) (pfrag1.wrd & 0x000000FF);
    passwd->passwd[4] = (ca_uint8_t) ((pfrag1.wrd & 0x0000FF00) >> 8);
    passwd->passwd[3] = (ca_uint8_t) ((pfrag1.wrd & 0x00FF0000) >> 16);
    passwd->passwd[2] = (ca_uint8_t) ((pfrag1.wrd & 0xFF000000) >> 24);


    pfrag0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_pfrag0);
    passwd->passwd[1] = (ca_uint8_t) (pfrag0.wrd & 0x000000FF);
    passwd->passwd[0] = (ca_uint8_t) ((pfrag0.wrd & 0x0000FF00) >> 8);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_vssn_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    vssn)
{
    GPON_MAC_GPON_vendor_spec_t sn = {0};

    AAL_GPON_LOCK();

    sn.wrd = vssn;
    AAL_PON_REG_WRITE(sn.wrd, GPON_MAC_GPON_vendor_spec);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_vssn_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *vssn)
{
    GPON_MAC_GPON_vendor_spec_t sn = {0};

    AAL_GPON_LOCK();

    sn.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_vendor_spec);
    *vssn  = sn.wrd;

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_vendor_id_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    vendor_id)
{
    GPON_MAC_GPON_vendor_t vendor = {0};

    AAL_GPON_LOCK();

    vendor.wrd = vendor_id;
    AAL_PON_REG_WRITE(vendor.wrd, GPON_MAC_GPON_vendor);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_vendor_id_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *vendor_id)
{
    GPON_MAC_GPON_vendor_t vendor = {0};

    AAL_GPON_LOCK();

    vendor.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_vendor);
    *vendor_id = vendor.wrd;

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_gtc_timer_set(
    CA_IN ca_device_id_t        device_id,
    CA_IN aal_gpon_gtc_timers_t timer_id,
    CA_IN ca_uint16             timer_len)
{
    GPON_MAC_GPON_act_t act = {0};

    if (AAL_GPON_TO2_TIMER < timer_id)
    {
        return CA_E_PARAM;
    }

    AAL_GPON_LOCK();

    //get the value and set seprately
    act.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_act);

    if (AAL_GPON_TO1_TIMER == timer_id)
    {
        /* Convert 'seconds' to GTC frame count */
        /* 8000 frames per seconds */
        /* If it exceeds hardware limit. Program the maximum */
        act.bf.to1_timer_threshold = ((timer_len * 8000) >= 0x3E800) ? 0x3E800 : (timer_len * 8000);
    }
    else
    {
        /* Convert 'seconds' to GTC frame count */
        /* 8 frames per miliseconds */
        /* If it exceeds hardware limit. Program the maximum */
        act.bf.to2_timer_threshold = ((timer_len * 8) >= 0xFF8) ? 0xFF8 : timer_len * 8;
    }
    AAL_PON_REG_WRITE(act.wrd, GPON_MAC_GPON_act);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_gtc_timer_get(
    CA_IN  ca_device_id_t        device_id,
    CA_IN  aal_gpon_gtc_timers_t timer_id,
    CA_OUT ca_uint16             *timer_len)
{
    GPON_MAC_GPON_act_t act = {0};

    act.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_act);
    if (AAL_GPON_TO1_TIMER == timer_id)
    {
        *timer_len = (act.bf.to1_timer_threshold * 125)/1000000;
    }
    else
    {
        *timer_len = (act.bf.to2_timer_threshold * 125 )/ 1000;
    }

    return CA_E_OK;
}


ca_status_t aal_gpon_mac_onu_cfg_set(
    CA_IN ca_device_id_t device_id)
{
    GPON_MAC_GPON_onu_cfg_t onuCfg = {0};

    AAL_GPON_LOCK();

    onuCfg.wrd = 0x12000780; //18bits later than trasmit_data
    AAL_PON_REG_WRITE(onuCfg.wrd, GPON_MAC_GPON_onu_cfg);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_gpon_cfg_set(
    CA_IN ca_device_id_t      device_id,
    CA_IN aal_gpon_cfg_type_t cfg_type,
    CA_IN ca_uint32_t         value)
{
    if (AAL_GPON_CFG_RESET_ALL == cfg_type)
    {
        aal_gpon_init(device_id);
        return CA_E_OK;
    }

    if (AAL_GPON_CFG_RESET_PON == cfg_type)
    {
        aal_gpon_reinit(device_id);
        return CA_E_OK;
    }

    if (AAL_GPON_CFG_RESET_PSDS == cfg_type)
    {
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
        aal_psds_reset(device_id, ONU_PON_MAC_GPON);
#endif
        return CA_E_OK;
    }

    return CA_E_PARAM;
}


ca_status_t aal_gpon_cfg_get(
    CA_IN  ca_device_id_t      device_id,
    CA_IN  aal_gpon_cfg_type_t cfg_type,
    CA_OUT ca_uint32_t         value)
{
    if (AAL_GPON_CFG_RESET_ALL == cfg_type)
    {
        ca_printf("usage:\r\n ");
        ca_printf("    cfg_type: 0 for gpon mac reset all\r\n ");
        return CA_E_OK;
    }

    return CA_E_OK;
}


ca_status_t aal_gpon_omcc_tcont_enable(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    alloc_id)
{
    aal_gpon_tcont_cfg_msk_t  mask = {0};
    aal_gpon_tcont_cfg_t      cfg  = {0};

    ca_printf("Set Tcont 0 allocid %d\r\n", alloc_id);

    mask.s.tcont_index = 1;
    mask.s.omci_en     = 1;
    mask.s.ploam_en    = 1;
    cfg.omci_en        = 1;
    cfg.ploam_en       = 1;
    cfg.tcont_index    = 0;
    aal_gpon_tcont_set(device_id, FALSE, alloc_id, mask, &cfg);
    return CA_E_OK;
}


ca_status_t aal_gpon_omcc_gem_enable(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    port_id)
{
    ca_uint8_t idx = 0;

    ca_printf("Set US Gemport 0-7 gemId %d\r\n", port_id);
    for (idx = 0; idx < AAL_GPON_OMCI_RSV_PORT_MAX; idx++)
    {
        aal_gpon_us_gem_port_set(device_id, idx, port_id);
    }
    return CA_E_OK;
}


ca_status_t aal_gpon_ploam_rx_mib_get(
    CA_IN ca_device_id_t device_id)
{
    ca_uint8_t op_code               = AAL_GPON_PLOAM_MIB_READ_CLR;
    ca_uint8_t upstream_overhead     = 0;
    ca_uint8_t assign_onu_id         = 0;
    ca_uint8_t ranging_time          = 0;
    ca_uint8_t deactivate_onu_id     = 0;
    ca_uint8_t disable_serial_number = 0;
    ca_uint8_t encrypted_port_id     = 0;
    ca_uint8_t request_password      = 0;
    ca_uint8_t assign_alloc_id       = 0;
    ca_uint8_t no_message            = 0;
    ca_uint8_t popup                 = 0;
    ca_uint8_t request_key           = 0;
    ca_uint8_t configure_port_id     = 0;
    ca_uint8_t pee                   = 0;
    ca_uint8_t change_power_level    = 0;
    ca_uint8_t pst_message           = 0;
    ca_uint8_t ber_interval          = 0;
    ca_uint8_t key_switching_time    = 0;
    ca_uint8_t extended_burst_length = 0;
#ifdef CONFIG_LUNA_G3_SERIES
    ca_uint8_t ploam_crc_error       = 0;
#endif
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_UPSTREAM_OVERHEAD,     &upstream_overhead);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_ASSIGN_ONU_ID,         &assign_onu_id);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_RANGING_TIME,          &ranging_time);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_DEACTIVATE_ONU_ID,     &deactivate_onu_id);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_DISABLE_SERIAL_NUMBER, &disable_serial_number);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_ENCRYPTED_PORT_ID,     &encrypted_port_id);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_REQUEST_PASSWORD,      &request_password);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_ASSIGN_ALLOC_ID,       &assign_alloc_id);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_NO_MESSAGE,            &no_message);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_POPUP,                 &popup);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_REQUEST_KEY,           &request_key);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_CONFIGURE_PORT_ID,     &configure_port_id);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_PEE,                   &pee);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_CHANGE_POWER_LEVEL,    &change_power_level);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_PST_MESSAGE,           &pst_message);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_BER_INTERVAL,          &ber_interval);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_KEY_SWITCHING_TIME,    &key_switching_time);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_EXTENDED_BURST_LENGTH, &extended_burst_length);
#ifdef CONFIG_LUNA_G3_SERIES
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_RX_MIB_CRC_ERROR,             &ploam_crc_error);
#endif

    ca_printf("\r\n");    
    ca_printf("upstream_overhead-----%d\r\n", upstream_overhead);        
    ca_printf("assign_onu_id---------%d\r\n", assign_onu_id);
    ca_printf("ranging_time----------%d\r\n", ranging_time);
    ca_printf("deactivate_onu_id-----%d\r\n", deactivate_onu_id);
    ca_printf("disable_serial_number-%d\r\n", disable_serial_number);
    ca_printf("encrypted_port_id-----%d\r\n", encrypted_port_id);
    ca_printf("request_password------%d\r\n", request_password);
    ca_printf("assign_alloc_id-------%d\r\n", assign_alloc_id);
    ca_printf("no_message------------%d\r\n", no_message);
    ca_printf("popup-----------------%d\r\n", popup);
    ca_printf("request_key-----------%d\r\n", request_key);
    ca_printf("configure_port_id-----%d\r\n", configure_port_id);
    ca_printf("pee-------------------%d\r\n", pee);
    ca_printf("change_power_level----%d\r\n", change_power_level);
    ca_printf("pst_message-----------%d\r\n", pst_message);
    ca_printf("ber_interval----------%d\r\n", ber_interval);
    ca_printf("key_switching_time----%d\r\n", key_switching_time);
    ca_printf("extended_burst_length-%d\r\n", extended_burst_length);
#ifdef CONFIG_LUNA_G3_SERIES
    ca_printf("ploam_crc_error-------%d\r\n", ploam_crc_error);
#endif
    return CA_E_OK;
}


ca_status_t aal_gpon_ploam_tx_mib_get(
    CA_IN ca_device_id_t device_id)
{
    ca_uint8_t op_code           = AAL_GPON_PLOAM_MIB_READ_CLR;
    ca_uint8_t serial_number_onu = 0;
    ca_uint8_t password          = 0;
    ca_uint8_t dying_gasp        = 0;
    ca_uint8_t no_message        = 0;
    ca_uint8_t encryption_key    = 0;
    ca_uint8_t pee               = 0;
    ca_uint8_t pst_message       = 0;
    ca_uint8_t rei               = 0;
    ca_uint8_t acknowledge       = 0;

    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_SERIAL_NUMBER_ONU, &serial_number_onu);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_PASSWORD,          &password);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_DYING_GASP,        &dying_gasp);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_NO_MESSAGE,        &no_message);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_ENCRYPTION_KEY,    &encryption_key);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_PEE,               &pee);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_PST_MESSAGE,       &pst_message);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_REI,               &rei);
    aal_gpon_ploam_mib_get(device_id, op_code, AAL_GPON_PLOAM_TX_MIB_ACKNOWLEDGE,       &acknowledge);

    ca_printf("\r\n");
    ca_printf("serial_number_onu-%d\r\n", serial_number_onu);
    ca_printf("password----------%d\r\n", password);
    ca_printf("dying_gasp--------%d\r\n", dying_gasp);
    ca_printf("no_message--------%d\r\n", no_message);
    ca_printf("encryption_key----%d\r\n", encryption_key);
    ca_printf("pee---------------%d\r\n", pee);
    ca_printf("pst_message-------%d\r\n", pst_message);
    ca_printf("rei---------------%d\r\n", rei);
    ca_printf("acknowledge-------%d\r\n", acknowledge);
    return CA_E_OK;
}


ca_status_t aal_gpon_gems_get(CA_IN ca_device_id_t device_id)
{
    ca_uint8_t              idx       = 0;
    ca_uint16_t             gem_id    = 0;
    ca_uint16_t             ds_gem    = 0;
    ca_boolean_t            sw_aes_en = TRUE;
    ca_boolean_t            ds_vld    = FALSE;
    aal_gpon_gem_port_cfg_t cfg       = {0};

#ifdef CONFIG_LUNA_G3_SERIES
    ca_printf("\r\n");
    ca_printf("GEM-Idx   GEM-ID         Valid    AES-Mode\r\n");
    ca_printf("(UP/DN)   (UP/DN)       (UP/DN)      DN   \r\n");
    ca_printf("-----------------------------------------------------\r\n");
#else
    ca_printf("\r\n");
    ca_printf("Internal  GEM-ID      Valid     AES-Mode\r\n");
    ca_printf("GEM-Idx   (UP/DN)    (UP/DN)       DN   \r\n");
    ca_printf("---------------------------------------------------\r\n");
#endif
    for (idx = 0; idx < AAL_GPON_SYSTEM_MAX_GEM_NUM; idx++)
    {
        gem_id = 0;
        ds_gem = 0;
        ds_vld = FALSE;
        memset(&cfg, 0, sizeof(cfg));

#ifdef CONFIG_LUNA_G3_SERIES
        if(idx < AAL_GPON_OMCI_RSV_PORT_MAX)
        {
            aal_gpon_us_gem_port_get(device_id, idx, &gem_id);
            if (0 != gem_id)
            {
                //gpon ds gem use gemport id to get the gemport index
                aal_gpon_ds_gem_port_get(device_id, sw_aes_en, gem_id, &cfg);
                if (idx == cfg.gem_index)
                {
                    ds_gem = gem_id;
                    ds_vld = TRUE;
                }
                ca_printf("(%-2u/%-2u)   (%-4u/%-4u)   (%-2u/%-2u)      %-2u\r\n",
                       idx, cfg.gem_index, gem_id, ds_gem, TRUE, ds_vld, cfg.aes);
            }
        }
        else if ((idx >= AAL_GPON_OMCI_RSV_PORT_MAX) &&((tmp_usbg_gem_cfg[idx].gem_index != 0)||(tmp_dsfp_gem_cfg[idx].vld !=0 )))
        {
            aal_gpon_us_gem_port_get(device_id, idx, &gem_id);

            if(tmp_dsfp_gem_cfg[idx].vld)
                aal_gpon_ds_gem_port_get(device_id, sw_aes_en, tmp_dsfp_gem_cfg[idx].gem_index, &cfg);

            if((gem_id != 0)||(cfg.vld != 0))
            {
                ca_printf("(%-2u/%-2u)   (%-4u/%-4u)   (%-2u/%-2u)      %-2u\r\n",
                       (tmp_usbg_gem_cfg[idx].gem_index != 0)? idx : 0,
                       (tmp_dsfp_gem_cfg[idx].vld)? idx : 0,
                       tmp_usbg_gem_cfg[idx].gem_index,
                       (tmp_dsfp_gem_cfg[idx].vld)? tmp_dsfp_gem_cfg[idx].gem_index : 0,
                       (tmp_usbg_gem_cfg[idx].gem_index != 0)? 1 : 0,
                       tmp_dsfp_gem_cfg[idx].vld,
                       cfg.aes);
            }
        }
#else
        aal_gpon_us_gem_port_get(device_id, idx, &gem_id);
        if (0 != gem_id)
        {
            //gpon ds gem use gemport id to get the gemport index
            aal_gpon_ds_gem_port_get(device_id, sw_aes_en, gem_id, &cfg);
            if (idx == cfg.gem_index)
            {
                ds_gem = gem_id;
                ds_vld = TRUE;
            }
            ca_printf("%-9u (%-4u/%-4u)   (%-2u/%-2u)    %-2u\r\n",
                       idx, gem_id, ds_gem, TRUE, ds_vld, cfg.aes);
        }
#endif
    }

    return CA_E_OK;
}



ca_status_t aal_gpon_gems_mib_get(CA_IN ca_device_id_t device_id)
{
    ca_uint8_t                 idx       = 0;
    ca_uint16_t                gem_id    = 0;
    ca_uint16_t                ds_gem    = 0;
    ca_boolean_t               sw_aes_en = TRUE;
    ca_boolean_t               ds_vld    = FALSE;
    aal_gpon_gem_port_cfg_t    cfg       = {0};
    aal_gpon_ds_gem_port_mib_t dsMib     = {0};
    aal_gpon_us_gem_port_mib_t usMib     = {0};
    GPON_MAC_GPON_MIB_INIT_t   mibCfg    = {0};

    ca_printf("\r\nGEM-Idx  US_XGEM-ID  DN_XGEM-ID   UP-pkt-cnt    UP-byte-cnt    DN-pkt-cnt    DN-byte-cnt\r\n");
    ca_printf("-----------------------------------------------------------------------------------------\r\n");

    for (idx = 0; idx < AAL_GPON_SYSTEM_MAX_GEM_NUM; idx++)
    {
        gem_id = 0;
        ds_gem = 0;
        ds_vld = FALSE;
        memset(&cfg,   0, sizeof(cfg));
        memset(&dsMib, 0, sizeof(dsMib));
        memset(&usMib, 0, sizeof(usMib));

#ifdef CONFIG_LUNA_G3_SERIES
        if ((tmp_usbg_gem_cfg[idx].gem_index != 0)||(tmp_dsfp_gem_cfg[idx].vld != 0))
        {
            //get usMib
            aal_gpon_us_gem_port_mib_get(device_id, 0, idx, &usMib);

            //gpon ds gem use gemport id to get the gem index
            aal_gpon_ds_gem_port_mib_get(device_id, idx, 0, &dsMib);

            ca_printf("%-6d    %-6d    %-6d    %-12d  %-13lld  %-12d  %-13lld  \r\n",
                      idx, tmp_usbg_gem_cfg[idx].gem_index, tmp_dsfp_gem_cfg[idx].gem_index, usMib.fcnt, usMib.bcnt, dsMib.fcnt, dsMib.bcnt);
        }
#else
        aal_gpon_us_gem_port_get(device_id, idx, &gem_id);
        if (0 != gem_id)
        {
            //get usMib
            aal_gpon_us_gem_port_mib_get(device_id, 0, idx, &usMib);

            //gpon ds gem use gemport id to get the gem index
            aal_gpon_ds_gem_port_get(device_id, sw_aes_en, gem_id, &cfg);
            if (idx == cfg.gem_index)
            {
                ds_gem = gem_id;
                ds_vld = TRUE;
                aal_gpon_ds_gem_port_mib_get(device_id, idx, &dsMib);
            }

            ca_printf("%-6d    %-6d    %-6d    %-12d  %-13lld  %-12d  %-13lld  \r\n",
                      idx, gem_id, ds_gem, usMib.fcnt, usMib.bcnt, dsMib.fcnt, dsMib.bcnt);
        }
#endif
    }

    //clear after read
    AAL_GPON_LOCK();

    mibCfg.bf.us = 1;
    mibCfg.bf.ds = 1;
    AAL_PON_REG_WRITE(mibCfg.wrd, GPON_MAC_GPON_MIB_INIT);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_gpon_mib_init(
    CA_IN  ca_device_id_t     device_id,
    CA_IN  ca_uint32_t        us_clear, 
    CA_IN  ca_uint32_t        ds_clear, 
    CA_IN  ca_uint32_t        plm_clear)
{
    GPON_MAC_GPON_MIB_INIT_t   mibCfg    = {0};

    //clear 
    AAL_GPON_LOCK();

    mibCfg.bf.us = us_clear;
    mibCfg.bf.ds = ds_clear;
    mibCfg.bf.plm = plm_clear;
    AAL_PON_REG_WRITE(mibCfg.wrd, GPON_MAC_GPON_MIB_INIT);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}
#endif

ca_status_t aal_gpon_tcont_bwmaps_get(CA_IN ca_device_id_t device_id)
{
    GPON_MAC_GPON_bwmap_ctl_t   ctl     = {0};
    GPON_MAC_GPON_bwmap_data0_t data0   = {0};
    GPON_MAC_GPON_bwmap_data1_t data1   = {0};
    ca_uint32_t                 idx     = 0;
    ca_uint16_t                 allocId = 0;
    ca_uint8_t                  plsu    = 0;
    ca_uint8_t                  ploamu  = 0;
    ca_uint8_t                  fec     = 0;
    ca_uint8_t                  dbru    = 0;
    ca_uint8_t                  res     = 0;
    ca_uint32_t                 Sstart  = 0;
    ca_uint32_t                 Sstop   = 0;

    AAL_GPON_LOCK();

    ctl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_ctl);
    if (0 == ctl.bf.hwdone)
    {
        AAL_GPON_UNLOCK();
        ca_printf("Can't enable capture when hwdone bit is 0\r\n");
        return CA_E_OK;
    }

    ctl.bf.hwdone = 0;
    ctl.bf.enable = 1;
    AAL_PON_REG_WRITE(ctl.wrd, GPON_MAC_GPON_bwmap_ctl);

    ca_mdelay(100);

    //get counter now
    memset(&ctl, 0, sizeof(ctl));
    ctl.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_ctl);
    if (0 == ctl.bf.count)
    {
        AAL_PON_REG_FIELD_WRITE(GPON_MAC_GPON_bwmap_ctl, enable, 0);
        AAL_GPON_UNLOCK();
        ca_printf("Capture NULL\r\n");
        return CA_E_OK;
    }

    //scan all the bwmap data
    ca_printf("\r\n");
#ifdef CONFIG_LUNA_G3_SERIES
    ca_printf("%-3s|%-7s|%-4s|%-6s|%-3s|%-4s|%-3s|%-6s|%-6s|%-5s\r\n",
              "No","allocId","plsu","ploamu","fec","dbru","res", "Sstart", "Sstop", "len");
    ca_printf("----------------------------------------------------------\r\n");
#else
    ca_printf(" %-3s | %-7s | %-4s | %-6s | %-3s | %-4s | %-3s | %-7s | %-7s\r\n",
              "No","allocId","plsu","ploamu","fec","dbru","res", "Sstart", "Sstop");
    ca_printf("----------------------------------------------------------------------\r\n");
#endif
    for (idx = 0; idx < ctl.bf.count; idx++)
    {
        allocId = 0;
        plsu    = 0;
        ploamu  = 0;
        fec     = 0;
        dbru    = 0;
        res     = 0;
        Sstart  = 0;
        Sstop   = 0;
        memset(&data0, 0, sizeof(data0));
        memset(&data1, 0, sizeof(data1));

        data0.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_data0);
        data1.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_bwmap_data1);

        allocId = (0xfff000 & data1.bf.data) >> 12; //bit12-23
        plsu    = (0x800 & data1.bf.data) >> 11; //bit 11
        ploamu  = (0x400 & data1.bf.data) >> 10; //bit 10
        fec     = (0x200 & data1.bf.data) >> 9; //bit 9
        dbru    = (0x180 & data1.bf.data) >> 7; //bit 7-8
        res     = 0x7F & data1.bf.data; //bit 0-6
        Sstart  = (0xffff0000 & data0.bf.data) >> 16; //bit 16-32
        Sstop   = 0xffff & data0.bf.data; //bit 0-15
#ifdef CONFIG_LUNA_G3_SERIES
        ca_printf("%-3d|%-7d|%-4d|%-6d|%-3d|%-4d|%-3d|%-6d|%-6d|%-5d\r\n", 
                    idx, allocId, plsu, ploamu, fec, dbru, res, Sstart, Sstop, (Sstop-Sstart+1));
#else
        ca_printf(" %-3d", idx);
        ca_printf(" | %-7d", allocId);
        ca_printf(" | %-4d", plsu);
        ca_printf(" | %-6d", ploamu);
        ca_printf(" | %-3d", fec);
        ca_printf(" | %-4d", dbru);
        ca_printf(" | %-3d", res);
        ca_printf(" | %-7d", Sstart);
        ca_printf(" | %-7d", Sstop);
        ca_printf("\r\n");
#endif
    }

    //clear the value
    AAL_PON_REG_FIELD_WRITE(GPON_MAC_GPON_bwmap_ctl, enable, 0);

    AAL_GPON_UNLOCK();
    return CA_E_OK;
}


static ca_status_t __gpon_glb_reset(ca_device_id_t device_id)
{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    AAL_GLB_REG_WRITE(0x1, GLOBAL_PSDS_INIT_CNTL);
#else
    AAL_GLB_REG_WRITE(0x1, CLKGEN_PSDS_INIT_CNTL);
#endif
    AAL_GLB_REG_WRITE(0x30000, GLOBAL_EPON_CNTL);
    AAL_GLB_REG_WRITE(0x0, GLOBAL_PON_CNTL);
    AAL_GLB_REG_WRITE(0x0, GLOBAL_GPON_CNTL);
    AAL_GLB_REG_WRITE(0x4, GLOBAL_PON_CNTL);

    return CA_E_OK;
}


static ca_status_t __gpon_datapath_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    /* PDC Init */
    ret = aal_pdc_init(device_id, ONU_PON_MAC_GPON);
    CA_RET_VALUE_CHECK(ret);

    /* PUC Init */
    ret = aal_puc_init(device_id, ONU_PON_MAC_GPON);
    CA_RET_VALUE_CHECK(ret);
    return CA_E_OK;
}


static ca_status_t __gpon_gemport_init(ca_device_id_t device_id)
{
    ca_uint8_t idx = 0;

    for (idx = 0; idx < AAL_GPON_SYSTEM_MAX_GEM_NUM; idx++)
    {
#ifdef CONFIG_LUNA_G3_SERIES
        if((idx >= AAL_GPON_OMCI_RSV_PORT_MAX)&&(tmp_usbg_gem_cfg[idx].gem_index!=0))
            continue;
#endif
        aal_gpon_us_gem_port_set(device_id, idx, 0);
    }
    return CA_E_OK;
}


static ca_status_t __gpon_aes_key_init(ca_device_id_t device_id)
{
    volatile ca_uint32_t key_frag0[4]      = {0};
    volatile ca_uint32_t key_frag1[4]      = {0};
    ca_uint8_t  key0[AAL_GPON_AES_KEY_LEN] = {0};
    ca_uint8_t  key1[AAL_GPON_AES_KEY_LEN] = {0};

    random_key_generate(key0);
    random_key_generate(key1);

    key_frag0[3] = key0[0]  | (key0[1] << 8)  | (key0[2] << 16)  | (key0[3] << 24);
    key_frag0[2] = key0[4]  | (key0[5] << 8)  | (key0[6] << 16)  | (key0[7] << 24);
    key_frag0[1] = key0[8]  | (key0[9] << 8)  | (key0[10] << 16) | (key0[11] << 24);
    key_frag0[0] = key0[12] | (key0[13] << 8) | (key0[14] << 16) | (key0[15] << 24);
    key_frag1[3] = key1[0]  | (key1[1] << 8)  | (key1[2] << 16)  | (key1[3] << 24);
    key_frag1[2] = key1[4]  | (key1[5] << 8)  | (key1[6] << 16)  | (key1[7] << 24);
    key_frag1[1] = key1[8]  | (key1[9] << 8)  | (key1[10] << 16) | (key1[11] << 24);
    key_frag1[0] = key1[12] | (key1[13] << 8) | (key1[14] << 16) | (key1[15] << 24);


    /* Key Set 1 is active. Program new key into key_set 0 */
    AAL_PON_REG_WRITE(key_frag0[0],GPON_MAC_GPON_key0_frag0);
    AAL_PON_REG_WRITE(key_frag0[1],GPON_MAC_GPON_key0_frag1);
    AAL_PON_REG_WRITE(key_frag0[2],GPON_MAC_GPON_key0_frag2);
    AAL_PON_REG_WRITE(key_frag0[3],GPON_MAC_GPON_key0_frag3);
    AAL_PON_REG_WRITE(key_frag1[0],GPON_MAC_GPON_key1_frag0);
    AAL_PON_REG_WRITE(key_frag1[1],GPON_MAC_GPON_key1_frag1);
    AAL_PON_REG_WRITE(key_frag1[2],GPON_MAC_GPON_key1_frag2);
    AAL_PON_REG_WRITE(key_frag1[3],GPON_MAC_GPON_key1_frag3);

    return CA_E_OK;
}


static ca_status_t __gpon_common_init(ca_device_id_t device_id)
{
    ca_status_t       ret    = CA_E_OK;
    aal_gpon_passwd_t passwd = {0};

    //ret = aal_gpon_gtc_timer_set(0, AAL_GPON_TO1_TIMER,g_gpon_basic_info.to1_timer_threshold);
    //ca_printf("ca-pon: init gpon GTC timers %s\n", (CA_E_OK != ret) ? "FAIL" : "OK");

    ret = aal_gpon_mac_onu_cfg_set(device_id);

    //no need to check the return value
    ret = aal_gpon_vendor_id_set(device_id, g_xgpn_basic_info.vendor_id);
    ret = aal_gpon_vssn_set(device_id, g_xgpn_basic_info.vssn);

    memcpy(passwd.passwd, g_gpon_basic_info.passwd, AAL_GPON_PORT_PASSWORD_LEN);
    ret = aal_gpon_passwd_set(device_id, &passwd);
    ca_printf("ca-pon: init gpon password %s\n", (CA_E_OK != ret) ? "FAIL" : "OK");

    //init gemport
    __gpon_gemport_init(device_id);

    //init key fragment all
    __gpon_aes_key_init(device_id);
    return ret;
}


static ca_status_t __gpon_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    if (0 == sg_aal_gpon_lock)
        ret = ca_spin_lock_init(&sg_aal_gpon_lock, SPINLOCK_BH);

    //copy from lynxe, need to get from scfg later
    g_gpon_basic_info.to1_timer_threshold = 10;
    g_gpon_basic_info.to2_timer_threshold = 100;

    //use the 10bytes of regitration id for gpon password
    memcpy(g_gpon_basic_info.passwd, g_xgpn_basic_info.registration_id, AAL_GPON_PORT_PASSWORD_LEN);
    return ret;
}


ca_status_t aal_gpon_datapath_reset(CA_IN ca_device_id_t device_id)
{
    ca_printf("ca-pon: reset gpon datapath\n");

    AAL_PON_REG_FIELD_WRITE(GPON_MAC_GPON_onu_ctl, dpath_rst, 1);
    ca_mdelay(10);
    AAL_PON_REG_FIELD_WRITE(GPON_MAC_GPON_onu_ctl, dpath_rst, 0);
    return CA_E_OK;
}


ca_status_t aal_gpon_reset(CA_IN ca_device_id_t device_id)
{
    ca_printf("ca-pon: reset gpon\n");

#ifdef CONFIG_LUNA_G3_SERIES
        aal_gpon_backup_ponmac(AAL_DEFAUL_DEV_ID);
#endif

    //maybe reset serdes should be ok for fiber flap
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
    aal_psds_reset(AAL_DEFAUL_DEV_ID, ONU_PON_MAC_GPON);
#endif

    //need to re init pon!!!
    aal_gpon_reinit(AAL_DEFAUL_DEV_ID);

    ca_gpon_cb_init(device_id);

#ifdef CONFIG_LUNA_G3_SERIES
        aal_gpon_restore_ponmac(AAL_DEFAUL_DEV_ID);
#endif

    return CA_E_OK;
}


ca_status_t aal_gpon_reinit(ca_device_id_t device_id)
{
    GLOBAL_GPON_CNTL_t gblCfg = {0};
    GLOBAL_PON_CNTL_t  ponCfg = {0};

    ca_printf("ca-pon: re init pon \n");

    //first of all, reset data path
    aal_gpon_datapath_reset(device_id);
   
    /* Reset GPON */
    AAL_GLB_REG_WRITE(0x0, GLOBAL_GPON_CNTL);

    ca_mdelay(AAL_XGPON_INIT_DELAY);

    /* reset puc pdc*/
    ponCfg.wrd          = AAL_GLB_REG_READ(GLOBAL_PON_CNTL);
    ponCfg.bf.puc_reset = 1;
    ponCfg.bf.pdc_reset = 1;
    AAL_GLB_REG_WRITE(ponCfg.wrd, GLOBAL_PON_CNTL);

    gblCfg.bf.ani_rst_n  = 1;
    gblCfg.bf.gpon_rst_n = 1;
    AAL_GLB_REG_WRITE(gblCfg.wrd, GLOBAL_GPON_CNTL);

    ca_mdelay(AAL_XGPON_INIT_DELAY);

    __gpon_common_init(device_id);
    __gpon_datapath_init(device_id);
    aal_gpon_intr_init(device_id);

#if defined(CONFIG_ARCH_CORTINA_VENUS)
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL, ben_oen, TRUE);
#else
    AAL_GLB_REG_FIELD_WRITE(CLKGEN_PSDS_INIT_CNTL, ben_oen, TRUE);
#endif

#ifdef CONFIG_LUNA_G3_SERIES //reset might be triggered by deactivation on purpose, donot enable automatically
    if(g_admin_state)
        aal_gpon_active_set(device_id, TRUE);
#else
    aal_gpon_active_set(device_id, TRUE);
#endif

    return CA_E_OK;
}


//only gpon mode will call this function
ca_status_t aal_gpon_init(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    //init paramters for aal gpon layer
    __gpon_init(device_id);

    __gpon_glb_reset(device_id);
    
#if defined(CONFIG_ARCH_CORTINA_VENUS) && !defined(CONFIG_CORTINA_BOARD_FPGA)
    ret = aal_psds_init(device_id, ONU_PON_MAC_GPON);
    ca_printf("ca-pon: init GPON SERDES %s \n", (CA_E_OK != ret) ? "FAIL" : "OK");
#endif
    /* Before init, should disable PON MAC to prevent Rx any signal as XGPON */
    ret = aal_gpon_glb_ctrl_init(device_id);
    ca_printf("ca-pon: init GPON global control %s \n", (CA_E_OK != ret) ? "FAIL" : "OK");

    //init gpon steps follow lynxe
    ret = __gpon_common_init(device_id);
    ca_printf("ca-pon: init GPON common %s \n", (CA_E_OK != ret) ? "FAIL" : "OK");

    //need to add gpon in this fsm
    //ret = aal_xgpn_activation_fsm_init(device_id, ONU_PON_MAC_GPON);
    ca_printf("ca-pon: init GPON activation FSM %s\n", (CA_E_OK != ret) ? "FAIL" : "OK");

    ret = __gpon_datapath_init(device_id);
    ca_printf("ca-pon: init GPON DATAPATH %s\n", (CA_E_OK != ret) ? "FAIL" : "OK");
    
    ret = aal_gpon_intr_init(device_id);
    ca_printf("ca-pon: init GPON interrupt %s\n", (CA_E_OK != ret) ? "FAIL" : "OK");

#ifdef CONFIG_LUNA_G3_SERIES
    memset(gpon_tmp_alloc_id_cfg, 0, sizeof(aal_gpon_tcont_cfg_t)*AAL_GPON_SYSTEM_MAX_TCONT_NUM);
    memset(tmp_usbg_gem_cfg, 0, sizeof(aal_gpon_gem_port_cfg_t)*AAL_GPON_SYSTEM_MAX_GEM_NUM);
    memset(tmp_dsfp_gem_cfg, 0, sizeof(aal_gpon_gem_port_cfg_t)*AAL_GPON_SYSTEM_MAX_GEM_NUM);
    ret = ca_event_register(device_id, CA_EVENT_GPON_PORT_LINK,
            __gpon_onu_link_evt_handler,  NULL);
#endif

    return ret;
}

#ifdef CONFIG_LUNA_G3_SERIES
ca_uint32_t __gpon_onu_link_evt_handler(
    ca_device_id_t  device_id,
    ca_uint32_t  event_id,
    ca_void_t    *event,
    ca_uint32_t  len,
    ca_void_t    *cookie
)
{
    ca_event_gpon_port_link_t *link_evt = (ca_event_gpon_port_link_t *)event;

    if (!link_evt) {
        return CA_E_PARAM;
    }

    if (link_evt->status) {
        ca_printf("!EVENT: ONU Active(onuid = %u)!\n", link_evt->port_id);
        aal_gpon_restore_tcont(device_id);
    } else {
        ca_printf("!EVENT: ONU De-Active(reason = %d)!\n", link_evt->reason);
    }

    return CA_E_OK;
}

ca_status_t aal_gpon_backup_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;
    ca_uint32_t priId;
    ca_uint32_t gem_idx;
    ca_uint32_t queue_idx;
    ca_uint32_t br_tcont_num = AAL_GPON_SYSTEM_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;
    aal_gpon_gem_port_cfg_t gem_cfg;

    if(tcont_num_per_chip < AAL_GPON_SYSTEM_MAX_TCONT_NUM)
        br_tcont_num = tcont_num_per_chip;

    if(queue_num_per_tcont < 8)
        br_queue_num = queue_num_per_tcont;

    for(tcontId=0;tcontId<br_tcont_num;tcontId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
            aal_puc_port_voq_map_get(device_id, tcontId, priId,&tmp_puc_queue_cfg[tcontId][priId]);
#if 0
            ca_printf("tcontId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",tcontId,priId
                ,tmp_puc_queue_cfg[tcontId][priId].queue_id
                ,tmp_puc_queue_cfg[tcontId][priId].queue_weight
                ,tmp_puc_queue_cfg[tcontId][priId].queue_pri
                ,tmp_puc_queue_cfg[tcontId][priId].queue_ena);
#endif
        }
    }

    aal_puc_ctrl_cfg_get(device_id,&tmp_puc_ctrl_config);
#if 0
    ca_printf("puc_ctrl_cfg pkt_mode_class0_cnt=%d pkt_mode_class1_cnt=%d force_init_done=%d addtok_en=%d subtok_en=%d shp_en=%d\n"
            ,tmp_puc_ctrl_config.pkt_mode_class0_cnt
            ,tmp_puc_ctrl_config.pkt_mode_class1_cnt
            ,tmp_puc_ctrl_config.force_init_done
            ,tmp_puc_ctrl_config.addtok_en
            ,tmp_puc_ctrl_config.subtok_en
            ,tmp_puc_ctrl_config.shp_en);
#endif

    aal_puc_btc_cfg_get(device_id,&tmp_puc_btc_config);
#if 0
    ca_printf("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n"
            ,tmp_puc_btc_config.pfovrhd
            ,tmp_puc_btc_config.schmode
            ,tmp_puc_btc_config.ethmode
            ,tmp_puc_btc_config.wdaligned
            ,tmp_puc_btc_config.lrgfrmfragen
            ,tmp_puc_btc_config.pyld1gfecon
            ,tmp_puc_btc_config.schen
            ,tmp_puc_btc_config.minrmnwindowsz);
#endif

    for(queue_idx=0;queue_idx<AAL_PUC_MAX_QUEUE_NUM;queue_idx++)
    {
        aal_puc_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
#if 0
        ca_printf("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
#endif

        aal_puc_rl_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
#if 0
        ca_printf("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
#endif
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
        aal_puc_voq_bp_remap_data_get(device_id, queue_idx, &(tmp_puc_bp_tqmvoqid[queue_idx]));
        //ca_printf("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
    }

    aal_puc_voq_buf_lmt_sel1500_get(device_id, &tmp_puc_voq_buf_lmt_sel1500_cfg);
    //ca_printf("puc_voq_buf_lmt 1500=%d\n",tmp_puc_voq_buf_lmt_sel1500_cfg);

    aal_puc_voq_buf_lmt_sel3116_get(device_id, &tmp_puc_voq_buf_lmt_sel3116_cfg);
    //ca_printf("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel3116_cfg);

    aal_puc_voq_buf_lmt_sel4732_get(device_id, &tmp_puc_voq_buf_lmt_sel4732_cfg);
    //ca_printf("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel4732_cfg);

    aal_puc_voq_buf_lmt_sel6348_get(device_id, &tmp_puc_voq_buf_lmt_sel6348_cfg);
    //ca_printf("puc_voq_buf_lmt 6348=%d\n",tmp_puc_voq_buf_lmt_sel6348_cfg);

    aal_puc_voq_buf_lmt_a_get(device_id, &tmp_puc_voq_buf_limit_a);
    //ca_printf("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);

    aal_puc_voq_buf_lmt_b_get(device_id, &tmp_puc_voq_buf_limit_b);
    //ca_printf("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);

    aal_puc_voq_buf_lmt_c_get(device_id, &tmp_puc_voq_buf_limit_c);
    //ca_printf("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);

    return CA_E_OK;
}

ca_status_t aal_gpon_restore_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;

    ca_uint32_t priId;
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp;
    aal_puc_queue_cfg_msk_t queue_cfg_mask;
    aal_puc_queue_cfg_t queue_cfg;
    aal_puc_voq_sche_cfg_t voq_sche_config;
    ca_uint32_t tbc = 0;

    ca_uint32_t gem_idx;
    aal_gpon_gem_port_cfg_msk_t gem_cfg_mask;
    aal_gpon_gem_port_cfg_t gem_cfg;

    aal_puc_ctrl_cfg_msk_t puc_ctrl_config_mask;
    aal_puc_btc_cfg_msk_t puc_btc_mask;

    ca_uint32_t queue_idx;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_rl_voq_tbc_cfg_msk;

    ca_uint32_t br_tcont_num = AAL_GPON_SYSTEM_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;

    if(tcont_num_per_chip < AAL_GPON_SYSTEM_MAX_TCONT_NUM)
        br_tcont_num = tcont_num_per_chip;

    if(queue_num_per_tcont < 8)
        br_queue_num = queue_num_per_tcont;


    for(tcontId=0;tcontId<br_tcont_num;tcontId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
#if 0
            ca_printf("tcontId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",tcontId,priId
                ,tmp_puc_queue_cfg[tcontId][priId].queue_id
                ,tmp_puc_queue_cfg[tcontId][priId].queue_weight
                ,tmp_puc_queue_cfg[tcontId][priId].queue_pri
                ,tmp_puc_queue_cfg[tcontId][priId].queue_ena);
#endif

            voq_drp_mask.u32      = 0;
            voq_drp_mask.s.voqid  = 1;
            voq_drp_mask.s.enable = 1;

            memset(&voq_drp, 0, sizeof(voq_drp));
            voq_drp.enable = 0;
            voq_drp.voqid = tmp_puc_queue_cfg[tcontId][priId].queue_id;
            aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);

            queue_cfg_mask.u32 = ~0U;
            queue_cfg.queue_id = tmp_puc_queue_cfg[tcontId][priId].queue_id;
            queue_cfg.queue_weight = tmp_puc_queue_cfg[tcontId][priId].queue_weight;
            queue_cfg.queue_pri = tmp_puc_queue_cfg[tcontId][priId].queue_pri;
            queue_cfg.queue_ena = tmp_puc_queue_cfg[tcontId][priId].queue_ena;

            aal_puc_port_voq_map_set(device_id, tcontId, priId, queue_cfg_mask, &queue_cfg);
#ifdef RTK_VOQ_VALID_SUPPORT
            aal_puc_voq_valid_set(device_id, queue_cfg.queue_id, queue_cfg.queue_ena);
#endif
            memset(&voq_sche_config, 0, sizeof(aal_puc_voq_sche_cfg_t));
            aal_puc_voq_sche_cfg_get(device_id,&voq_sche_config);
            if(voq_sche_config.basesel == 0)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 128;
            if(voq_sche_config.basesel == 1)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 256;
            if(voq_sche_config.basesel == 2)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 512;
            if(voq_sche_config.basesel == 3)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 1024;
            aal_puc_port_voq_sche_info_tbl_set(device_id,tcontId,priId,tbc);
        }
    }
#ifdef RTK_VOQ_VALID_SUPPORT
    aal_puc_voq_valid_set(device_id, 127, TRUE);

    voq_drp_mask.u32 = 0;
    memset(&voq_drp, 0, sizeof(aal_puc_voq_drop_cfg_t));
    voq_drp_mask.s.invalid_voqdrop_enable = 1;
    voq_drp_mask.s.enable = 1;
    voq_drp.invalid_voqdrop_enable = 1;
    voq_drp.enable = 0;
    aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);
#endif

    puc_ctrl_config_mask.u32 = ~0;
    aal_puc_ctrl_cfg_set(device_id, puc_ctrl_config_mask, &tmp_puc_ctrl_config);

#if 0
    ca_printf("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n" 
            ,tmp_puc_btc_config.pfovrhd
            ,tmp_puc_btc_config.schmode
            ,tmp_puc_btc_config.ethmode
            ,tmp_puc_btc_config.wdaligned
            ,tmp_puc_btc_config.lrgfrmfragen
            ,tmp_puc_btc_config.pyld1gfecon
            ,tmp_puc_btc_config.schen
            ,tmp_puc_btc_config.minrmnwindowsz);
#endif
    puc_btc_mask.u32 = ~0;
    aal_puc_btc_cfg_set(device_id, puc_btc_mask, &tmp_puc_btc_config);

    puc_voq_tbc_cfg_msk.u32=~0;
    puc_rl_voq_tbc_cfg_msk.u32=~0;
    for(queue_idx=0;queue_idx<AAL_PUC_MAX_QUEUE_NUM;queue_idx++)
    {
#ifdef RTK_VOQ_VALID_SUPPORT
        if(tmp_puc_voq_tbc_mem_cfg[queue_idx].enb == 1) //workaround
            tmp_puc_voq_tbc_mem_cfg[queue_idx].enb = 0;
#endif
#if 0
        ca_printf("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
#endif
        aal_puc_voq_tbc_mem_set(device_id,queue_idx,puc_voq_tbc_cfg_msk,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
#ifdef RTK_VOQ_VALID_SUPPORT
        if(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb == 1) //workaround
            tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb = 0;
#endif
#if 0
        ca_printf("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
#endif
        aal_puc_rl_voq_tbc_mem_set(device_id,queue_idx,puc_rl_voq_tbc_cfg_msk,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
        //ca_printf("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
        aal_puc_voq_bp_remap_data_set(device_id, queue_idx, tmp_puc_bp_tqmvoqid[queue_idx]);
    }

    //ca_printf("puc_voq_buf_lmt 1500=%d\n",tmp_puc_voq_buf_lmt_sel1500_cfg);
    aal_puc_voq_buf_lmt_sel1500_set(device_id, tmp_puc_voq_buf_lmt_sel1500_cfg);

    //ca_printf("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel3116_cfg);
    aal_puc_voq_buf_lmt_sel3116_set(device_id, tmp_puc_voq_buf_lmt_sel3116_cfg);

    //ca_printf("puc_voq_buf_lmt 4732=%d\n",tmp_puc_voq_buf_lmt_sel4732_cfg);
    aal_puc_voq_buf_lmt_sel4732_set(device_id, tmp_puc_voq_buf_lmt_sel4732_cfg);

    //ca_printf("puc_voq_buf_lmt 6348=%d\n",tmp_puc_voq_buf_lmt_sel6348_cfg);
    aal_puc_voq_buf_lmt_sel6348_set(device_id, tmp_puc_voq_buf_lmt_sel6348_cfg);

    //ca_printf("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);
    aal_puc_voq_buf_lmt_a_set(device_id, tmp_puc_voq_buf_limit_a);

    //ca_printf("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);
    aal_puc_voq_buf_lmt_b_set(device_id, tmp_puc_voq_buf_limit_b);

    //ca_printf("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);
    aal_puc_voq_buf_lmt_c_set(device_id, tmp_puc_voq_buf_limit_c);

    return CA_E_OK;
}

ca_status_t aal_gpon_restore_gems(ca_device_id_t device_id)
{
    ca_uint32_t gem_idx;
    aal_gpon_gem_port_cfg_msk_t gem_cfg_mask;
    aal_gpon_gem_port_cfg_t gem_cfg;

    gem_cfg_mask.u32 = ~0;
    for(gem_idx=AAL_GPON_OMCI_RSV_PORT_MAX;gem_idx<AAL_GPON_SYSTEM_MAX_GEM_NUM;gem_idx++)
    {
        if(tmp_usbg_gem_cfg[gem_idx].gem_index != 0)
        {
            if(CA_E_OK != aal_gpon_us_gem_port_set(device_id, gem_idx, tmp_usbg_gem_cfg[gem_idx].gem_index))
                ca_printf("aal_gpon_us_gem_port_set failed when restore gem (idx=%d, id=%d) \n", gem_idx ,tmp_usbg_gem_cfg[gem_idx].gem_index);

            //ca_printf("usbg gem_idx=%d gem_id=%d \n", gem_idx ,tmp_usbg_gem_cfg[gem_idx].gem_index);
        }

        if(tmp_dsfp_gem_cfg[gem_idx].vld ==1)
        {
            memset(&gem_cfg, 0, sizeof(aal_gpon_gem_port_cfg_t));
            gem_cfg.vld = tmp_dsfp_gem_cfg[gem_idx].vld;
            gem_cfg.gem_index = gem_idx;
            if(CA_E_OK != aal_gpon_ds_gem_port_set(device_id, 0, tmp_dsfp_gem_cfg[gem_idx].gem_index, gem_cfg_mask, &gem_cfg))
                ca_printf("aal_gpon_ds_gem_port_set failed when restore gem (idx=%d, id=%d) \n", gem_idx ,tmp_dsfp_gem_cfg[gem_idx].gem_index);

            //ca_printf("dsfp gem_idx=%d gem_id=%d \n", gem_idx ,tmp_dsfp_gem_cfg[gem_idx].gem_index);
        }
    }

    return CA_E_OK;
}
ca_status_t aal_gpon_restore_tcont(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;
    aal_gpon_tcont_cfg_msk_t mask;
    aal_gpon_tcont_cfg_t alloc_id_cfg;

    for(tcontId=1;tcontId<AAL_GPON_SYSTEM_MAX_TCONT_NUM;tcontId++)
    {
        if(gpon_tmp_alloc_id_cfg[tcontId].omci_en != 1)
            continue;

#if 0
        ca_printf("tcontId=%d omci_en=%d alloc_id=%d\n",tcontId
            ,gpon_tmp_alloc_id_cfg[tcontId].omci_en
            ,gpon_tmp_alloc_id_cfg[tcontId].tcont_index);
#endif

        mask.u32 = 0;
        mask.s.tcont_index =1;
        mask.s.omci_en =1;
        mask.s.ploam_en =1;
        alloc_id_cfg.tcont_index = tcontId;
        alloc_id_cfg.omci_en = 1;
        alloc_id_cfg.ploam_en = 0;

        aal_gpon_tcont_set(device_id, TRUE, gpon_tmp_alloc_id_cfg[tcontId].tcont_index, mask, &alloc_id_cfg);

        //workaround
        aal_puc_voq_flush_by_idx(device_id, tcontId, 0, 7);
    }

    return CA_E_OK;
}

EXPORT_SYMBOL(aal_gpon_backup_ponmac);
EXPORT_SYMBOL(aal_gpon_restore_ponmac);
EXPORT_SYMBOL(aal_gpon_restore_gems);
EXPORT_SYMBOL(aal_gpon_restore_tcont);
#endif

EXPORT_SYMBOL(aal_gpon_ds_frame_thrsd_set);
EXPORT_SYMBOL(aal_gpon_ds_frame_thrsd_get);
EXPORT_SYMBOL(aal_gpon_sn_req_res_ctrl_set);
EXPORT_SYMBOL(aal_gpon_sn_req_res_ctrl_get);
EXPORT_SYMBOL(aal_gpon_us_frm_ctrl_set);
EXPORT_SYMBOL(aal_gpon_us_frm_ctrl_get);
EXPORT_SYMBOL(aal_gpon_sf_degrd_alrm_thrsd_set);
EXPORT_SYMBOL(aal_gpon_sf_degrd_alrm_thrsd_get);
EXPORT_SYMBOL(aal_gpon_send_dying_gasp);
EXPORT_SYMBOL(aal_gpon_ploamd_ctrl_set);
EXPORT_SYMBOL(aal_gpon_ploamd_ctrl_get);
EXPORT_SYMBOL(aal_gpon_aes_key_set);
EXPORT_SYMBOL(aal_gpon_aes_key_get);
EXPORT_SYMBOL(aal_gpon_pst_set);
EXPORT_SYMBOL(aal_gpon_pst_get);
EXPORT_SYMBOL(aal_gpon_olt_pst_get);
EXPORT_SYMBOL(aal_gpon_last_bip_error_get);
EXPORT_SYMBOL(aal_gpon_current_bip_error_get);
EXPORT_SYMBOL(aal_gpon_current_alarms_get);
EXPORT_SYMBOL(aal_gpon_port_stats_get);
EXPORT_SYMBOL(aal_gpon_port_stats_clear);
EXPORT_SYMBOL(aal_gpon_us_ovrh_plm_msg_params_get);
EXPORT_SYMBOL(aal_gpon_sn_acquisition_status_get);
EXPORT_SYMBOL(aal_gpon_onu_state_get);
EXPORT_SYMBOL(aal_gpon_eqd_get);
EXPORT_SYMBOL(aal_gpon_protect_eqd_get);
EXPORT_SYMBOL(aal_gpon_onu_params_get);
EXPORT_SYMBOL(aal_gpon_type3_preamble_get);
EXPORT_SYMBOL(aal_gpon_send_ploam_us);
EXPORT_SYMBOL(aal_gpon_send_ploam_ds);
EXPORT_SYMBOL(aal_gpon_ds_ploam_msg_get);
EXPORT_SYMBOL(aal_gpon_mac_cfg_set);
EXPORT_SYMBOL(aal_gpon_mac_cfg_get);
EXPORT_SYMBOL(aal_gpon_ploamd_disable_mask_set);
EXPORT_SYMBOL(aal_gpon_ploamd_disable_mask_get);
EXPORT_SYMBOL(aal_gpon_ploamd_fifo_cap_ena_mask_set);
EXPORT_SYMBOL(aal_gpon_ploamd_fifo_cap_ena_mask_get);
EXPORT_SYMBOL(aal_gpon_ploamu_disable_mask_set);
EXPORT_SYMBOL(aal_gpon_ploamu_disable_mask_get);
EXPORT_SYMBOL(aal_gpon_ds_pkt_process_ctrl_set);
EXPORT_SYMBOL(aal_gpon_ds_pkt_process_ctrl_get);
EXPORT_SYMBOL(aal_gpon_tcont_get);
EXPORT_SYMBOL(aal_gpon_tcont_set);
EXPORT_SYMBOL(aal_gpon_ds_gem_port_get);
EXPORT_SYMBOL(aal_gpon_ds_gem_port_set);
EXPORT_SYMBOL(aal_gpon_ds_gem_port_mib_get);
EXPORT_SYMBOL(aal_gpon_us_gem_port_mib_get);
EXPORT_SYMBOL(aal_gpon_ploam_mib_get);
EXPORT_SYMBOL(aal_gpon_us_gem_port_get);
EXPORT_SYMBOL(aal_gpon_us_gem_port_set);
EXPORT_SYMBOL(aal_gpon_ds_pkt_bypass_set);
EXPORT_SYMBOL(aal_gpon_ds_pkt_bypass_get);
EXPORT_SYMBOL(aal_gpon_sw_ctrl_dba_set);
EXPORT_SYMBOL(aal_gpon_sw_ctrl_dba_get);
EXPORT_SYMBOL(aal_gpon_scratch_get);
EXPORT_SYMBOL(aal_gpon_scratch_set);
EXPORT_SYMBOL(aal_gpon_dbru_bufocc_tbl_get);
EXPORT_SYMBOL(aal_gpon_dbru_bufocc_tbl_set);
EXPORT_SYMBOL(aal_gpon_mac_ctrl_set);
EXPORT_SYMBOL(aal_gpon_mac_ctrl_get);
EXPORT_SYMBOL(aal_gpon_sup_frame_value_get);
EXPORT_SYMBOL(aal_gpon_sup_frame_value_set);
EXPORT_SYMBOL(aal_gpon_fec_ctrl_set);
EXPORT_SYMBOL(aal_gpon_fec_ctrl_get);
EXPORT_SYMBOL(aal_gpon_fec_misc_status_get);
EXPORT_SYMBOL(aal_gpon_fec_mibs_get);
EXPORT_SYMBOL(aal_gpon_pucif_ctrl_set);
EXPORT_SYMBOL(aal_gpon_pucif_ctrl_get);
EXPORT_SYMBOL(aal_gpon_pucif_cnt_get);
EXPORT_SYMBOL(aal_gpon_pucif_sts_get);
EXPORT_SYMBOL(aal_gpon_puc_protect_get);
EXPORT_SYMBOL(aal_gpon_active_set);
EXPORT_SYMBOL(aal_gpon_active_get);
EXPORT_SYMBOL(aal_gpon_passwd_set);
EXPORT_SYMBOL(aal_gpon_passwd_get);
EXPORT_SYMBOL(aal_gpon_vssn_set);
EXPORT_SYMBOL(aal_gpon_vssn_get);
EXPORT_SYMBOL(aal_gpon_vendor_id_set);
EXPORT_SYMBOL(aal_gpon_vendor_id_get);
EXPORT_SYMBOL(aal_gpon_gtc_timer_set);
EXPORT_SYMBOL(aal_gpon_gtc_timer_get);
EXPORT_SYMBOL(aal_gpon_cfg_set);
EXPORT_SYMBOL(aal_gpon_cfg_get);
EXPORT_SYMBOL(aal_gpon_ploam_rx_mib_get);
EXPORT_SYMBOL(aal_gpon_ploam_tx_mib_get);
EXPORT_SYMBOL(aal_gpon_gems_get);
EXPORT_SYMBOL(aal_gpon_gems_mib_get);
EXPORT_SYMBOL(aal_gpon_tcont_bwmaps_get);
#ifdef CONFIG_LUNA_G3_SERIES
EXPORT_SYMBOL(aal_gpon_mib_init);
#endif
#endif /* CONFIG_NE_PON */

