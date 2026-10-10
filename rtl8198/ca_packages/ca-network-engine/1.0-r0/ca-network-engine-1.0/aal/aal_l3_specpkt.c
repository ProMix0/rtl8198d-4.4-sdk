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
/* aal_l3_specpkt.c L3FE special packet AAL driver*/
#include "osal_common.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_l3fe.h"
#include "aal_l3_specpkt.h"

/* SIP L4 ports */
#define __L3FE_SIP_L4PORT     (5060)
#define __L3FE_SIP_TLS_L4PORT (5061)

/* L3FE register access MACROs */
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __L3FE_SPK_MAC_SET(a,b,c,d,e)           __l3_specpkt_mac_set(a,b,c,SATURN_##d,SATURN_##e)
#define __L3FE_SPK_MAC_GET(a,b,c,d,e)           __l3_specpkt_mac_get(a,b,c,SATURN_##d,e)
#define __L3FE_REG_FIELD_WRITE(dev,reg, field, val)  do{             \
                                 reg##_t data;                       \
                                 data.wrd = (((dev)==0)? CA_NE_REG_READ(reg): CA_HGU_8279_NE_REG_READ(SATURN_##reg));\
                                 data.bf.field = (val);              \
                                 (((dev)==0)? CA_NE_REG_WRITE(data.wrd,reg): CA_HGU_8279_NE_REG_WRITE(data.wrd,SATURN_##reg)); \
                                 }while(0)
#else
#define __L3FE_SPK_MAC_SET(a,b,c,d,e)           __l3_specpkt_mac_set(a,b,c,d,e)
#define __L3FE_SPK_MAC_GET(a,b,c,d,e)           __l3_specpkt_mac_get(a,b,c,d,e)
#define __L3FE_REG_FIELD_WRITE(dev,reg, field, val)  do{             \
                                 reg##_t data;                       \
                                 data.wrd = CA_DEV_NE_REG_READ(dev,reg);\
                                 data.bf.field = (val);              \
                                 CA_DEV_NE_REG_WRITE2(dev,reg,data.wrd); \
                                 }while(0)
#endif


#define __DATA_MASK_SET(data, mask, offset, value)  \
    (data) = ((data) & ~((mask)<<(offset))) | (((value)&(mask))<<(offset))

static void __l3_specpkt_mac_set(ca_device_id_t device_id,
                 ca_mac_addr_t mac_cfg,
                 ca_boolean_t use_hi_half,
                 ca_uint32_t reg_addr_hi,
                 ca_uint32_t reg_addr_low)
{
    ca_uint32_t reg_val0 = 0, reg_val1 = 0;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    reg_val0 = ca_ne_reg_read(device_id, reg_addr_hi);
#else
    reg_val0 = CA_DEV_NE_REG_READ(device_id, reg_addr_hi);
#endif

    if (use_hi_half) {
        reg_val0 &= 0xFFFF; /* mask high 16bits */
        reg_val0 |= (mac_cfg[0] << 24) | (mac_cfg[1] << 16);
    } else {
        reg_val0 &= 0xFFFF0000; /* mask low 16bits */
        reg_val0 |= (mac_cfg[0] << 8) | mac_cfg[1];
    }

    reg_val1 = (mac_cfg[2] << 24) |
        (mac_cfg[3] << 16) | (mac_cfg[4] << 8) | mac_cfg[5];

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_ne_reg_write(device_id, reg_val0, reg_addr_hi);
    ca_ne_reg_write(device_id, reg_val1, reg_addr_low);
#else
    CA_DEV_NE_REG_WRITE2(device_id, reg_addr_hi, reg_val0);
    CA_DEV_NE_REG_WRITE2(device_id, reg_addr_low, reg_val1);
#endif


}

static void __l3_specpkt_mac_get(ca_device_id_t device_id,
                 ca_uint32_t reg_addr_hi,
                 ca_boolean_t use_hi_half,
                 ca_uint32_t reg_addr_low,
                 ca_mac_addr_t mac_cfg)
{
    ca_uint32_t reg_val0 = 0, reg_val1 = 0;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    reg_val0 = ca_ne_reg_read(device_id, reg_addr_hi);
    reg_val1 = ca_ne_reg_read(device_id, reg_addr_low);
#else
    reg_val0 = CA_DEV_NE_REG_READ(device_id, reg_addr_hi);
    reg_val1 = CA_DEV_NE_REG_READ(device_id, reg_addr_low);
#endif

    if (use_hi_half) {
        mac_cfg[0] = (reg_val0 >> 24) & 0xFF;
        mac_cfg[1] = (reg_val0 >> 26) & 0xFF;
    } else {
        mac_cfg[0] = (reg_val0 >> 8) & 0xFF;
        mac_cfg[1] = (reg_val0) & 0xFF;
    }

    mac_cfg[2] = (reg_val1 >> 24) & 0xFF;
    mac_cfg[3] = (reg_val1 >> 16) & 0xFF;
    mac_cfg[4] = (reg_val1 >> 8) & 0xFF;
    mac_cfg[5] = (reg_val1) & 0xFF;

}


/*****************************************************************************/
/*                Layer 3 special packet AAL implementation                  */
/*****************************************************************************/
ca_status_t aal_l3_specpkt_ctrl_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_uint32_t specpkt_sel,
                    CA_IN aal_l3_specpkt_type_t pkt_type,
                    CA_IN aal_l3_specpkt_ctrl_mask_t mask,
                    CA_IN aal_l3_specpkt_ctrl_t * config)
{
    L3FE_STG0_SPB_TBL_DATA_t   data0;
    ca_uint32_t                tbl_addr = 0;

    AAL_ASSERT_RET(specpkt_sel <= AAL_L3_SPECPKT_SEL_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(pkt_type <= AAL_L3_SPECPKT_TYPE_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32 != 0, CA_E_OK);

    /*
     * Special Packet Behavior table. Table indexed by
     * {lpb.lpb_spcl_pkt_sel, spcl_pkt_enc[5:0]}
     */

    tbl_addr = ((specpkt_sel & 0x1) << 6) | (pkt_type & 0x3f);
    CA_DEV_READ_INDIRCT_REG(device_id, tbl_addr, L3FE_STG0_SPB_TBL_ACCESS);
    data0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_STG0_SPB_TBL_DATA);
    if (mask.s.spcl_en     )
        __DATA_MASK_SET(data0.wrd, 0x01, 27, config->spcl_en);

    if (mask.s.ldpid         )
        __DATA_MASK_SET(data0.wrd, 0x3f, 21, config->ldpid);

    if (mask.s.cos       )
        __DATA_MASK_SET(data0.wrd, 0x07, 18, config->cos);

    if (mask.s.pol_en_sel  )
        __DATA_MASK_SET(data0.wrd, 0x03, 16, config->pol_en_sel);

    if (mask.s.pol_id      )
        __DATA_MASK_SET(data0.wrd, 0x1ff, 7, config->pol_id);

    if (mask.s.grp_pol_id  )
        __DATA_MASK_SET(data0.wrd, 0x07, 4, config->grp_pol_id);

    if (mask.s.keep_ts     )
        __DATA_MASK_SET(data0.wrd, 0x01, 3, config->keep_ts);

    if (mask.s.no_drop     )
        __DATA_MASK_SET(data0.wrd, 0x01, 2, config->no_drop);

    if (mask.s.mrr_en      )
        __DATA_MASK_SET(data0.wrd, 0x01, 1, config->mrr_en);

    if (mask.s.mrr_en_valid)
        __DATA_MASK_SET(data0.wrd, 0x01, 0, config->mrr_en_valid);

    CA_DEV_NE_REG_WRITE2(device_id, L3FE_STG0_SPB_TBL_DATA, data0.wrd);
    CA_DEV_WRITE_INDIRCT_REG(device_id, tbl_addr, L3FE_STG0_SPB_TBL_ACCESS);
    //ca_printf("set dev %d L3FE_STG0_SPB_TBL_DATA = %#x\n", device_id, data0.wrd);

    return CA_E_OK;

}



ca_status_t aal_l3_specpkt_ctrl_get(CA_IN ca_device_id_t device_id,
                    CA_IN  ca_uint32_t specpkt_sel,
                    CA_IN  aal_l3_specpkt_type_t pkt_type,
                    CA_OUT aal_l3_specpkt_ctrl_t * config)
{
    L3FE_STG0_SPB_TBL_DATA_t   data0;
    ca_uint32_t                tbl_addr = 0;

    AAL_ASSERT_RET(specpkt_sel <= AAL_L3_SPECPKT_SEL_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(pkt_type <= AAL_L3_SPECPKT_TYPE_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    tbl_addr = ((specpkt_sel & 0x1) << 6) | (pkt_type & 0x3f);

    CA_DEV_READ_INDIRCT_REG(device_id, tbl_addr, L3FE_STG0_SPB_TBL_ACCESS);
    data0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_STG0_SPB_TBL_DATA);
    //ca_printf("get dev %d L3FE_STG0_SPB_TBL_DATA = %#x\n", device_id, data0.wrd);
    /*
    For each entry:
    Bit[27]       : sp_en
    Bit[26:21]    : sp_ldpid
    Bit[20:18]    : sp_cos
    Bit[17:16]    : sp_pol_en
    Bit[15:7]     : sp_pol_id
    Bit[6:4]      : sp_grp_pol_id
    Bit[3]        : sp_keep_ts
    Bit[2]        : sp_no_drop
    Bit[1]        : sp_mrr_en
    Bit[0]        : sp_mrr_vld
    */
    config->spcl_en       = (data0.wrd >> 27 ) & 0x01;
    config->ldpid         = (data0.wrd >> 21 ) & 0x3f;
    config->cos           = (data0.wrd >> 18 ) & 0x07;
    config->pol_en_sel    = (data0.wrd >> 16 ) & 0x03;
    config->pol_id        = (data0.wrd >> 7  ) & 0x1ff;
    config->grp_pol_id    = (data0.wrd >> 4  ) & 0x07;
    config->keep_ts       = (data0.wrd >> 3  ) & 0x01;
    config->no_drop       = (data0.wrd >> 2  ) & 0x01;
    config->mrr_en        = (data0.wrd >> 1  ) & 0x01;
    config->mrr_en_valid  = (data0.wrd >> 0  ) & 0x01;

    return CA_E_OK;

}

ca_status_t aal_l3_specpkt_parser_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_l3_specpkt_paser_status_t *stat
)
{
    L3FE_PP_PARSING_STTS_0_t     parsing_stts0;
    L3FE_PP_PARSING_STTS_1_t     parsing_stts1;
    L3FE_PP_PINGPONG_STS_t       pingpong_sts;
    L3FE_PP_NI_INTF_ERR_STTS_0_t ni_intf_err_stts0;
    L3FE_PP_NI_INTF_ERR_STTS_1_t ni_intf_err_stts1;
    L3FE_PP_NI_INTF_STS_t        ni_intf_sts;
    L3FE_PP_NI2FE_INTF_FF_STS_t  ni_intf_ff_sts;

    AAL_ASSERT_RET(stat != NULL, CA_E_PARAM);

    parsing_stts0.wrd     = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_STTS_0);
    parsing_stts1.wrd     = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_STTS_1);
    pingpong_sts.wrd      = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PINGPONG_STS);
    ni_intf_err_stts0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_NI_INTF_ERR_STTS_0);
    ni_intf_err_stts1.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_NI_INTF_ERR_STTS_1);
    ni_intf_sts.wrd       = CA_DEV_NE_REG_READ(device_id, L3FE_PP_NI_INTF_STS);

    stat->pingpong_fifo_overflow  =  pingpong_sts.bf.ovfl;
    stat->ni_intf_sop_eop_mismatch  =  ni_intf_err_stts0.bf.sop_eop_mismatch;
    stat->ni_intf_runt_l2_pkt  =  ni_intf_err_stts1.bf.runt_l2_pkt;
    stat->ni_intf_fifo_overflow  =  ni_intf_ff_sts.bf.ovfl;

    stat->tcp_chksum_err  =  parsing_stts0.bf.parsing_err & 0x1;
    stat->udp_chksum_err  =  (parsing_stts0.bf.parsing_err >> 1) & 0x1;
    stat->udp_lite_chksum_err  =  (parsing_stts0.bf.parsing_err >> 2) & 0x1;
    stat->rdpv1_chksum_err  =  (parsing_stts0.bf.parsing_err >> 3) & 0x1;
    stat->rdpv2_chksum_err  =  (parsing_stts0.bf.parsing_err >> 4) & 0x1;

    stat->ipv4_hdr_chksum_err  =  parsing_stts1.bf.parsing_err & 0x1;
    stat->dsl_inner_ip_hdr_chksum_err  = (parsing_stts1.bf.parsing_err >> 1) & 0x1;
    stat->pkt_l2_size_err  =  (parsing_stts1.bf.parsing_err >> 2) & 0x1;
    stat->pkt_l3_total_len_err  =  (parsing_stts1.bf.parsing_err >> 3) & 0x1;
    stat->ip_hdr_err  =  (parsing_stts1.bf.parsing_err >> 4) & 0x1;
    stat->ipv4_total_len_err  =  (parsing_stts1.bf.parsing_err >> 5) & 0x1;
    stat->tcp_data_offset_err  =  (parsing_stts1.bf.parsing_err >> 6) & 0x1;
    stat->udp_total_len_err  =  (parsing_stts1.bf.parsing_err >> 7) & 0x1;
    stat->udp_lite_cs_cvrg_err  =  (parsing_stts1.bf.parsing_err >> 8) & 0x1;
    stat->rdp_hdr_len_err  =  (parsing_stts1.bf.parsing_err >> 9) & 0x1;
    stat->pkt_inner_l3_total_len_err  =  (parsing_stts1.bf.parsing_err >> 10) & 0x1;
    stat->inner_ip_hdr_err  =  (parsing_stts1.bf.parsing_err >> 11) & 0x1;
    stat->dsl_inner_ipv4_total_len_err  =  (parsing_stts1.bf.parsing_err >> 12) & 0x1;
    stat->offset_l3_excd_255_err  =  (parsing_stts1.bf.parsing_err >> 13) & 0x1;
    stat->offset_l3_inner_excd_255_err  =  (parsing_stts1.bf.parsing_err >> 14) & 0x1;
    stat->offset_l4_excd_255_err  =  (parsing_stts1.bf.parsing_err >> 15) & 0x1;
    stat->udp_zero_chksum_err  =  (parsing_stts1.bf.parsing_err >> 16) & 0x1;
    stat->more_than_2_vlan_tags_flg  = (parsing_stts1.bf.parsing_err >> 17) & 0x1;
    stat->l2tp_header_detection_err  =  (parsing_stts1.bf.parsing_err >> 18) & 0x1;
    stat->rdp_ver_err  =  (parsing_stts1.bf.parsing_err >> 19) & 0x1;

    return CA_E_OK;

}

ca_status_t aal_l3_specpkt_detect_set(CA_IN ca_device_id_t device_id,
                      CA_IN aal_l3_specpkt_detect_mask_t mask,
                      CA_IN aal_l3_specpkt_detect_t * config)
{
    ca_uint32_t reg_val0 = 0, reg_val1 = 0;
    L3FE_PP_SPCL_PKT_DETECTION_CFG_t detect;
    L3FE_PP_IPNIP_SPCL_PKT_DET_EN_t  den;
    L3FE_PP_HTTP_LINK_0_t l4_sppkt0;
    L3FE_PP_HTTP_LINK_1_t l4_sppkt1;
    L3FE_PP_HTTP_LINK_2_t l4_sppkt2;
    L3FE_PP_HTTP_LINK_3_t l4_sppkt3;
    L3FE_PP_PARSING_LMT_t parsing_lmt;

    AAL_ASSERT_RET(mask.u32[0] || mask.u32[1], CA_E_OK);
    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    if (mask.s.hello_mac) {
        __L3FE_SPK_MAC_SET(device_id, config->hello_mac, FALSE,
                     L3FE_PP_HELLO_MAC_HI,
                     L3FE_PP_HELLO_MAC_LOW);
    }

    if (mask.s.my_mac) {
        __L3FE_SPK_MAC_SET(device_id, config->my_mac, FALSE,
                     L3FE_PP_MY_MAC_HI, L3FE_PP_MY_MAC_LOW);
    }

    if (mask.s.rsvd_mc_mac_hi_range) {
        __L3FE_SPK_MAC_SET(device_id, config->rsvd_mc_mac_hi_range,
                     TRUE, L3FE_PP_MC_MAC_RNG,
                     L3FE_PP_MC_MAC_HI_RNG);
    }

    if (mask.s.rsvd_mc_mac_low_range) {
        __L3FE_SPK_MAC_SET(device_id, config->rsvd_mc_mac_low_range,
                     FALSE, L3FE_PP_MC_MAC_RNG,
                     L3FE_PP_MC_MAC_LOW_RNG);
    }

    if (mask.s.user_def_0_mac) {
        __L3FE_SPK_MAC_SET(device_id, config->user_def_0_mac, FALSE,
                     L3FE_PP_USER_MAC_0_HI,
                     L3FE_PP_USER_MAC_0_LOW);
    }

    if (mask.s.user_def_0_mac_mask) {
        __L3FE_SPK_MAC_SET(device_id, config->user_def_0_mac_mask, FALSE,
                     L3FE_PP_USER_MAC_0_MASK_HI,
                     L3FE_PP_USER_MAC_0_MASK_LOW);
    }

    if (mask.s.user_def_1_mac) {
        __L3FE_SPK_MAC_SET(device_id, config->user_def_1_mac, FALSE,
                     L3FE_PP_USER_MAC_1_HI,
                     L3FE_PP_USER_MAC_1_LOW);
    }

    if (mask.s.user_def_1_mac_mask) {
        __L3FE_SPK_MAC_SET(device_id, config->user_def_1_mac_mask, FALSE,
                     L3FE_PP_USER_MAC_1_MASK_HI,
                     L3FE_PP_USER_MAC_1_MASK_LOW);
    }

    if (mask.s.user_def_ethtype_0) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_USER_TYPE_0,
                       user_type_0, config->user_def_ethtype_0);
    }

    if (mask.s.user_def_ethtype_1) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_USER_TYPE_0,
                       user_type_1, config->user_def_ethtype_1);
    }

    if (mask.s.user_def_ethtype_2) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_USER_TYPE_1,
                       user_type_2, config->user_def_ethtype_2);
    }

    if (mask.s.user_def_ethtype_3) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_USER_TYPE_1,
                       user_type_3, config->user_def_ethtype_3);
    }

    if (mask.s.l7_offset) {
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_OFFSET_HDR,
                 config->l7_offset & 0x7F);
    }

    if (mask.s.efm_oam_subtype) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_OAM,
                       subtype, config->efm_oam_subtype);
    }

    if (mask.s.cfm_md_level) {
        __L3FE_REG_FIELD_WRITE(device_id, L3FE_PP_OAM,
                       mymdlvl, config->cfm_md_level);
    }

    if (mask.s.arp_target_ip) {
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_ARP_TARGET_IP,
                 config->arp_target_ip);
    }

    /* Other control */
    detect.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_SPCL_PKT_DETECTION_CFG);
    den.wrd    = CA_DEV_NE_REG_READ(device_id, L3FE_PP_IPNIP_SPCL_PKT_DET_EN);

    reg_val0 = detect.wrd;
    reg_val1 = den.wrd;

    if (mask.s.user_def_spcl_0_enable) {
        detect.bf.user_def_spcl_0_en = config->user_def_spcl_0_enable;
    }

    if (mask.s.user_def_spcl_1_enable) {
        detect.bf.user_def_spcl_1_en = config->user_def_spcl_1_enable;
    }

    if (mask.s.user_def_spcl_2_enable) {
        detect.bf.user_def_spcl_2_en = config->user_def_spcl_2_enable;
    }

    if (mask.s.user_def_spcl_3_enable) {
        detect.bf.user_def_spcl_3_en = config->user_def_spcl_3_enable;
    }

    if (mask.s.user_def_0_ethtype_detect_enable) {
        detect.bf.type_0_en = config->user_def_0_ethtype_detect_enable;
    }

    if (mask.s.user_def_1_ethtype_detect_enable) {
        detect.bf.type_1_en = config->user_def_1_ethtype_detect_enable;
    }

    if (mask.s.efm_oam_subtype_detect_enable) {
        detect.bf.subtype_en = config->efm_oam_subtype_detect_enable;
    }

    if (mask.s.bpdu_mac_08_enable) {
        detect.bf.bpdu_mac_08_en = config->bpdu_mac_08_enable;
    }

    if (mask.s.l4_chksum_rh_chk_enable) {
        detect.bf.l4_chksum_rh_chk_enable =
            config->l4_chksum_rh_chk_enable;
    }

    if (mask.s.l4_chksum_chk_enable) {
        detect.bf.l4_chksum_chk_enable = config->l4_chksum_chk_enable;
    }

    if (mask.s.ipv4_chksum_chk_enable) {
        detect.bf.ipv4_chksum_chk_enable =
            config->ipv4_chksum_chk_enable;
    }

    if (mask.s.rdp_v1_detection_enable) {
        detect.bf.rdp_v1_detection_en = config->rdp_v1_detection_enable;
    }

    if (mask.s.l4_spcl_hdr_detct_enable) {
        detect.bf.l4_spcl_hdr_en = config->l4_spcl_hdr_detct_enable;
    }

    if (mask.s.l3_spcl_hdr_detct_enable) {
        detect.bf.l3_spcl_hdr_en = config->l3_spcl_hdr_detct_enable;
    }

    if (mask.s.l2_spcl_hdr_detct_enable) {
        detect.bf.l2_spcl_hdr_en = config->l2_spcl_hdr_detct_enable;
    }

    if (mask.s.my_mac_detct_enable) {
        detect.bf.my_mac_enable = config->my_mac_detct_enable;
    }

    if (mask.s.mpcp_opcode_chk_enable) {
        detect.bf.mpcp_opcode_en = config->mpcp_opcode_chk_enable;
    }

    if (mask.s.check_arp_target_ip_enable) {
        detect.bf.check_arp_target_ip =
            config->check_arp_target_ip_enable;
    }

    if (mask.s.udp_lite_detct_enable) {
        detect.bf.udp_lite_enable = config->udp_lite_detct_enable;
    }

#if 0 /* ASIC remove the below functions */
    if (mask.s.ipv4_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x1F)
            | (config->ipv4_chksum_fail_drop << 5);
    }

    if (mask.s.tcp_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x2F)
            | (config->tcp_chksum_fail_drop << 4);
    }

    if (mask.s.udp_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x37)
            | (config->udp_chksum_fail_drop << 3);
    }

    if (mask.s.udp_lite_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x3B)
            | (config->udp_lite_chksum_fail_drop << 2);
    }

    if (mask.s.rdpv1_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x3D)
            | (config->rdpv1_chksum_fail_drop << 1);
    }

    if (mask.s.rdpv2_chksum_fail_drop) {
        detect.bf.chksum_err_drop = (detect.bf.chksum_err_drop & 0x3E)
            | (config->rdpv2_chksum_fail_drop);
    }
#endif

    if (mask.s.user_define_hi_pri) {
        detect.bf.user_define_hi_pri = config->user_define_hi_pri;
    }

    if (mask.s.dhcpv4_detct_enable )
        den.bf.dhcpv4  = config->dhcpv4_detct_enable ;
    if (mask.s.dhcpv6_detct_enable )
        den.bf.dhcpv6  = config->dhcpv6_detct_enable ;
    if (mask.s.l4ptp_detct_enable  )
        den.bf.l4ptp   = config->l4ptp_detct_enable  ;
    if (mask.s.dns_detct_enable    )
        den.bf.dns     = config->dns_detct_enable    ;
    if (mask.s.rip_detct_enable    )
        den.bf.rip     = config->rip_detct_enable    ;
    if (mask.s.ssdp_detct_enable   )
        den.bf.ssdp    = config->ssdp_detct_enable   ;
    if (mask.s.l4_msgs_detct_enable)
        den.bf.l4_msgs = config->l4_msgs_detct_enable;

    if (detect.wrd != reg_val0) /* only write reg when value was changed */
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_SPCL_PKT_DETECTION_CFG,
                 detect.wrd);
    if (den.wrd != reg_val1) /* only write reg when value was changed */
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_IPNIP_SPCL_PKT_DET_EN,
                 den.wrd);

    if (mask.s.http_link0) {
        l4_sppkt0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_0);
        l4_sppkt0.bf.rule_l4_pid     = config->http_link0_l4_port;
        l4_sppkt0.bf.rule_l4_pid_sel = config->http_link0_l4_port_type;
        l4_sppkt0.bf.rule_l4_type    = config->http_link0_l4_type;
        l4_sppkt0.bf.rule_lspid      = config->http_link0_lspid;
        l4_sppkt0.bf.rule_lspid_msk  = !config->http_link0_lspid_valid;
        l4_sppkt0.bf.rule_vld        = config->http_link0_detect_enable;
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_HTTP_LINK_0, l4_sppkt0.wrd);
    }

    if (mask.s.http_link1) {
        l4_sppkt1.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_1);
        l4_sppkt1.bf.rule_l4_pid     = config->http_link1_l4_port;
        l4_sppkt1.bf.rule_l4_pid_sel = config->http_link1_l4_port_type;
        l4_sppkt1.bf.rule_l4_type    = config->http_link1_l4_type;
        l4_sppkt1.bf.rule_lspid      = config->http_link1_lspid;
        l4_sppkt1.bf.rule_lspid_msk  = !config->http_link1_lspid_valid;
        l4_sppkt1.bf.rule_vld        = config->http_link1_detect_enable;
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_HTTP_LINK_1, l4_sppkt1.wrd);
    }

    if (mask.s.http_link2) {
        l4_sppkt2.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_2);
        l4_sppkt2.bf.rule_l4_pid     = config->http_link2_l4_port;
        l4_sppkt2.bf.rule_l4_pid_sel = config->http_link2_l4_port_type;
        l4_sppkt2.bf.rule_l4_type    = config->http_link2_l4_type;
        l4_sppkt2.bf.rule_lspid      = config->http_link2_lspid;
        l4_sppkt2.bf.rule_lspid_msk  = !config->http_link2_lspid_valid;
        l4_sppkt2.bf.rule_vld        = config->http_link2_detect_enable;
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_HTTP_LINK_2, l4_sppkt2.wrd);
    }


    if (mask.s.http_link3) {
        l4_sppkt3.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_3);
        l4_sppkt3.bf.rule_l4_pid     = config->http_link3_l4_port;
        l4_sppkt3.bf.rule_l4_pid_sel = config->http_link3_l4_port_type;
        l4_sppkt3.bf.rule_l4_type    = config->http_link3_l4_type;
        l4_sppkt3.bf.rule_lspid      = config->http_link3_lspid;
        l4_sppkt3.bf.rule_lspid_msk  =!config->http_link3_lspid_valid;
        l4_sppkt3.bf.rule_vld        = config->http_link3_detect_enable;
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_HTTP_LINK_3, l4_sppkt3.wrd);
    }

    if (mask.s.detect_depth) {
        parsing_lmt.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_LMT);
        parsing_lmt.bf.offset = config->detect_depth; /* real detect depth = 128B + config->detect_depth*2B */
        CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_PARSING_LMT, parsing_lmt.wrd);
    }

    return CA_E_OK;

}

ca_status_t aal_l3_specpkt_detect_get(CA_IN ca_device_id_t device_id,
                      CA_OUT aal_l3_specpkt_detect_t * config)
{
    ca_uint32_t reg_val0 = 0;
    L3FE_PP_SPCL_PKT_DETECTION_CFG_t detect;
    L3FE_PP_IPNIP_SPCL_PKT_DET_EN_t  den;
    L3FE_PP_HTTP_LINK_0_t l4_sppkt0;
    L3FE_PP_HTTP_LINK_1_t l4_sppkt1;
    L3FE_PP_HTTP_LINK_2_t l4_sppkt2;
    L3FE_PP_HTTP_LINK_3_t l4_sppkt3;
    L3FE_PP_PARSING_LMT_t parsing_lmt;

    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_HELLO_MAC_HI, FALSE,
                 L3FE_PP_HELLO_MAC_LOW, config->hello_mac);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_MY_MAC_HI, FALSE,
                 L3FE_PP_MY_MAC_LOW, config->my_mac);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_MC_MAC_RNG, TRUE,
                 L3FE_PP_MC_MAC_HI_RNG,
                 config->rsvd_mc_mac_hi_range);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_MC_MAC_RNG, FALSE,
                 L3FE_PP_MC_MAC_LOW_RNG,
                 config->rsvd_mc_mac_low_range);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_USER_MAC_0_HI, FALSE,
                 L3FE_PP_USER_MAC_0_LOW, config->user_def_0_mac);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_USER_MAC_0_MASK_HI, FALSE,
                 L3FE_PP_USER_MAC_0_MASK_LOW,
                 config->user_def_0_mac_mask);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_USER_MAC_1_HI, FALSE,
                 L3FE_PP_USER_MAC_1_LOW, config->user_def_1_mac);

    __L3FE_SPK_MAC_GET(device_id, L3FE_PP_USER_MAC_1_MASK_HI, FALSE,
                 L3FE_PP_USER_MAC_1_MASK_LOW,
                 config->user_def_1_mac_mask);

    reg_val0      = CA_DEV_NE_REG_READ(device_id, L3FE_PP_USER_TYPE_0);

    config->user_def_ethtype_0 = reg_val0 & 0xFFFF;
    config->user_def_ethtype_1 = (reg_val0 >> 16) & 0xFFFF;

    reg_val0 = CA_DEV_NE_REG_READ(device_id, L3FE_PP_USER_TYPE_1);
    config->user_def_ethtype_2 = reg_val0 & 0xFFFF;
    config->user_def_ethtype_3 = (reg_val0 >> 16) & 0xFFFF;

    reg_val0 = CA_DEV_NE_REG_READ(device_id, L3FE_PP_OFFSET_HDR);
    config->l7_offset = reg_val0 & 0x7F;

    reg_val0 = CA_DEV_NE_REG_READ(device_id, L3FE_PP_OAM);
    config->efm_oam_subtype = reg_val0 & 0xFF;
    config->cfm_md_level = (reg_val0 >> 8) & 0x7;

    config->arp_target_ip = CA_DEV_NE_REG_READ(device_id, L3FE_PP_ARP_TARGET_IP);

    /* Other control */
    detect.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_SPCL_PKT_DETECTION_CFG);
    den.wrd    = CA_DEV_NE_REG_READ(device_id, L3FE_PP_IPNIP_SPCL_PKT_DET_EN);
    parsing_lmt.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_LMT);
    l4_sppkt0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_0);
    l4_sppkt1.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_1);
    l4_sppkt2.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_2);
    l4_sppkt3.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_HTTP_LINK_3);

    config->user_def_spcl_0_enable = detect.bf.user_def_spcl_0_en;
    config->user_def_spcl_1_enable = detect.bf.user_def_spcl_1_en;
    config->user_def_spcl_2_enable = detect.bf.user_def_spcl_2_en;
    config->user_def_spcl_3_enable = detect.bf.user_def_spcl_3_en;
    config->user_def_0_ethtype_detect_enable = detect.bf.type_0_en;
    config->user_def_1_ethtype_detect_enable = detect.bf.type_1_en;
    config->efm_oam_subtype_detect_enable = detect.bf.subtype_en;
    config->bpdu_mac_08_enable = detect.bf.bpdu_mac_08_en;
    config->l4_chksum_rh_chk_enable = detect.bf.l4_chksum_rh_chk_enable;
    config->l4_chksum_chk_enable = detect.bf.l4_chksum_chk_enable;
    config->ipv4_chksum_chk_enable = detect.bf.ipv4_chksum_chk_enable;
    config->rdp_v1_detection_enable = detect.bf.rdp_v1_detection_en;
    config->l4_spcl_hdr_detct_enable = detect.bf.l4_spcl_hdr_en;
    config->l3_spcl_hdr_detct_enable = detect.bf.l3_spcl_hdr_en;
    config->l2_spcl_hdr_detct_enable = detect.bf.l2_spcl_hdr_en;
    config->my_mac_detct_enable = detect.bf.my_mac_enable;
    config->mpcp_opcode_chk_enable = detect.bf.mpcp_opcode_en;
    config->check_arp_target_ip_enable = detect.bf.check_arp_target_ip;
    config->udp_lite_detct_enable = detect.bf.udp_lite_enable;
#if 0 /* ASIC remove the below functions */
    config->ipv4_chksum_fail_drop = (detect.bf.chksum_err_drop >> 5) & 0x1;
    config->tcp_chksum_fail_drop = (detect.bf.chksum_err_drop >> 4) & 0x1;
    config->udp_chksum_fail_drop = (detect.bf.chksum_err_drop >> 3) & 0x1;
    config->udp_lite_chksum_fail_drop =
        (detect.bf.chksum_err_drop >> 2) & 0x1;
    config->rdpv1_chksum_fail_drop = (detect.bf.chksum_err_drop >> 1) & 0x1;
    config->rdpv2_chksum_fail_drop = (detect.bf.chksum_err_drop) & 0x1;
#endif

    config->user_define_hi_pri = detect.bf.user_define_hi_pri;

    config->dhcpv4_detct_enable  = den.bf.dhcpv4 ;
    config->dhcpv6_detct_enable  = den.bf.dhcpv6 ;
    config->l4ptp_detct_enable   = den.bf.l4ptp  ;
    config->dns_detct_enable     = den.bf.dns    ;
    config->rip_detct_enable     = den.bf.rip    ;
    config->ssdp_detct_enable    = den.bf.ssdp   ;
    config->l4_msgs_detct_enable = den.bf.l4_msgs;

    config->http_link0_l4_port       = l4_sppkt0.bf.rule_l4_pid    ;
    config->http_link0_l4_port_type  = l4_sppkt0.bf.rule_l4_pid_sel;
    config->http_link0_l4_type       = l4_sppkt0.bf.rule_l4_type   ;
    config->http_link0_lspid         = l4_sppkt0.bf.rule_lspid     ;
    config->http_link0_lspid_valid   = !l4_sppkt0.bf.rule_lspid_msk ;
    config->http_link0_detect_enable = l4_sppkt0.bf.rule_vld       ;

    config->http_link1_l4_port       = l4_sppkt1.bf.rule_l4_pid    ;
    config->http_link1_l4_port_type  = l4_sppkt1.bf.rule_l4_pid_sel;
    config->http_link1_l4_type       = l4_sppkt1.bf.rule_l4_type   ;
    config->http_link1_lspid         = l4_sppkt1.bf.rule_lspid     ;
    config->http_link1_lspid_valid   = !l4_sppkt1.bf.rule_lspid_msk ;
    config->http_link1_detect_enable = l4_sppkt1.bf.rule_vld       ;

    config->http_link2_l4_port       = l4_sppkt2.bf.rule_l4_pid    ;
    config->http_link2_l4_port_type  = l4_sppkt2.bf.rule_l4_pid_sel;
    config->http_link2_l4_type       = l4_sppkt2.bf.rule_l4_type   ;
    config->http_link2_lspid         = l4_sppkt2.bf.rule_lspid     ;
    config->http_link2_lspid_valid   = !l4_sppkt2.bf.rule_lspid_msk ;
    config->http_link2_detect_enable = l4_sppkt2.bf.rule_vld       ;

    config->http_link3_l4_port       = l4_sppkt3.bf.rule_l4_pid    ;
    config->http_link3_l4_port_type  = l4_sppkt3.bf.rule_l4_pid_sel;
    config->http_link3_l4_type       = l4_sppkt3.bf.rule_l4_type   ;
    config->http_link3_lspid         = l4_sppkt3.bf.rule_lspid     ;
    config->http_link3_lspid_valid   = !l4_sppkt3.bf.rule_lspid_msk ;
    config->http_link3_detect_enable = l4_sppkt3.bf.rule_vld       ;

    config->detect_depth = parsing_lmt.bf.offset;

    return CA_E_OK;

}

/**
 * @aal_l3_specpkt_err_fwd_ctrl_set
 * configure the parsing error forward control:
 * drop or trap or forward as normal
 */
ca_status_t  aal_l3_specpkt_err_fwd_ctrl_set(
    CA_IN ca_device_id_t                             device_id,
    CA_IN aal_l3_specpkt_err_fwd_ctrl_mask_t      mask,
    CA_IN aal_l3_specpkt_err_fwd_ctrl_t          *config
)
{
    L3FE_PP_PARSING_ERR_FWD_0_t  reg_err_fwd0;
    L3FE_PP_PARSING_ERR_FWD_1_t  reg_err_fwd1;

    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    reg_err_fwd0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_FWD_0);
    reg_err_fwd1.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_FWD_1);

    if (mask.s.ipv4_hdr_cs_err             )
        reg_err_fwd0.bf.ipv4_hdr_cs_err              = config->ipv4_hdr_cs_err               ;
    if (mask.s.pkt_l2_size_err             )
        reg_err_fwd0.bf.pkt_l2_size_err              = config->pkt_l2_size_err               ;
    if (mask.s.pkt_l3_total_len_err        )
        reg_err_fwd0.bf.pkt_l3_total_len_err         = config->pkt_l3_total_len_err          ;
    if (mask.s.ip_hdr_err                  )
        reg_err_fwd0.bf.ip_hdr_err                   = config->ip_hdr_err                    ;
    if (mask.s.ipv4_total_len_err          )
        reg_err_fwd0.bf.ipv4_total_len_err           = config->ipv4_total_len_err            ;
    if (mask.s.tcp_data_offset_err         )
        reg_err_fwd0.bf.tcp_data_offset_err          = config->tcp_data_offset_err           ;
    if (mask.s.udp_total_len_err           )
        reg_err_fwd0.bf.udp_total_len_err            = config->udp_total_len_err             ;
    if (mask.s.udp_lite_cs_cvrg_err        )
        reg_err_fwd0.bf.udp_lite_cs_cvrg_err         = config->udp_lite_cs_cvrg_err          ;
    if (mask.s.rdp_hdr_len_err             )
        reg_err_fwd0.bf.rdp_hdr_len_err              = config->rdp_hdr_len_err               ;
    if (mask.s.offset_l3_excd_255_err      )
        reg_err_fwd0.bf.offset_l3_excd_255_err       = config->offset_l3_excd_255_err        ;
    if (mask.s.offset_l4_excd_255_err      )
        reg_err_fwd0.bf.offset_l4_excd_255_err       = config->offset_l4_excd_255_err        ;

    if (mask.s.dsl_inner_ip_hdr_cs_err     )
        reg_err_fwd1.bf.dsl_inner_ip_hdr_cs_err      = config->dsl_inner_ip_hdr_cs_err       ;
    if (mask.s.pkt_inner_l3_total_len_err  )
        reg_err_fwd1.bf.pkt_inner_l3_total_len_err   = config->pkt_inner_l3_total_len_err    ;
    if (mask.s.inner_ip_hdr_err            )
        reg_err_fwd1.bf.inner_ip_hdr_err             = config->inner_ip_hdr_err              ;
    if (mask.s.dsl_inner_ipv4_total_len_err)
        reg_err_fwd1.bf.dsl_inner_ipv4_total_len_err = config->dsl_inner_ipv4_total_len_err  ;
    if (mask.s.offset_l3_inner_excd_255_err)
        reg_err_fwd1.bf.offset_l3_inner_excd_255_err = config->offset_l3_inner_excd_255_err  ;
    if (mask.s.udp_zero_chksum_err         )
        reg_err_fwd1.bf.udp_zero_chksum_err          = config->udp_zero_chksum_err           ;
    if (mask.s.more_than_2_vlan_tags_flg   )
        reg_err_fwd1.bf.more_than_2_vlan_tags_flg    = config->more_than_2_vlan_tags_flg     ;
    if (mask.s.l2tp_hdr_err                )
        reg_err_fwd1.bf.l2tp_hdr_err                 = config->l2tp_hdr_err                  ;
    if (mask.s.rdp_ver_err                 )
        reg_err_fwd1.bf.rdp_ver_err                  = config->rdp_ver_err                   ;

    CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_PARSING_ERR_FWD_0, reg_err_fwd0.wrd);
    CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_PARSING_ERR_FWD_1, reg_err_fwd1.wrd);

    return CA_E_OK;

}


/**
 * @aal_l3_specpkt_err_fwd_ctrl_get
 * get the configuration of the parsing error forward control
 */
ca_status_t  aal_l3_specpkt_err_fwd_ctrl_get(
    CA_IN  ca_device_id_t                       device_id,
    CA_OUT aal_l3_specpkt_err_fwd_ctrl_t    *config
)
{
    L3FE_PP_PARSING_ERR_FWD_0_t  reg_err_fwd0;
    L3FE_PP_PARSING_ERR_FWD_1_t  reg_err_fwd1;

    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    reg_err_fwd0.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_FWD_0);
    reg_err_fwd1.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_FWD_1);

    config->ipv4_hdr_cs_err              = reg_err_fwd0.bf.ipv4_hdr_cs_err              ;
    config->pkt_l2_size_err              = reg_err_fwd0.bf.pkt_l2_size_err              ;
    config->pkt_l3_total_len_err         = reg_err_fwd0.bf.pkt_l3_total_len_err         ;
    config->ip_hdr_err                   = reg_err_fwd0.bf.ip_hdr_err                   ;
    config->ipv4_total_len_err           = reg_err_fwd0.bf.ipv4_total_len_err           ;
    config->tcp_data_offset_err          = reg_err_fwd0.bf.tcp_data_offset_err          ;
    config->udp_total_len_err            = reg_err_fwd0.bf.udp_total_len_err            ;
    config->udp_lite_cs_cvrg_err         = reg_err_fwd0.bf.udp_lite_cs_cvrg_err         ;
    config->rdp_hdr_len_err              = reg_err_fwd0.bf.rdp_hdr_len_err              ;
    config->offset_l3_excd_255_err       = reg_err_fwd0.bf.offset_l3_excd_255_err       ;
    config->offset_l4_excd_255_err       = reg_err_fwd0.bf.offset_l4_excd_255_err       ;

    config->dsl_inner_ip_hdr_cs_err      = reg_err_fwd1.bf.dsl_inner_ip_hdr_cs_err      ;
    config->pkt_inner_l3_total_len_err   = reg_err_fwd1.bf.pkt_inner_l3_total_len_err   ;
    config->inner_ip_hdr_err             = reg_err_fwd1.bf.inner_ip_hdr_err             ;
    config->dsl_inner_ipv4_total_len_err = reg_err_fwd1.bf.dsl_inner_ipv4_total_len_err ;
    config->offset_l3_inner_excd_255_err = reg_err_fwd1.bf.offset_l3_inner_excd_255_err ;
    config->udp_zero_chksum_err          = reg_err_fwd1.bf.udp_zero_chksum_err          ;
    config->more_than_2_vlan_tags_flg    = reg_err_fwd1.bf.more_than_2_vlan_tags_flg    ;
    config->l2tp_hdr_err                 = reg_err_fwd1.bf.l2tp_hdr_err                 ;
    config->rdp_ver_err                  = reg_err_fwd1.bf.rdp_ver_err                  ;

    return CA_E_OK;

}


ca_status_t  aal_l3_specpkt_err_trap_cfg_set(
    CA_IN ca_device_id_t                             device_id,
    CA_IN aal_l3_specpkt_err_trap_cfg_mask_t      mask,
    CA_IN aal_l3_specpkt_err_trap_cfg_t          *config
)
{
    L3FE_PP_PARSING_ERR_TRAP_t  reg_err_trap;

    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    reg_err_trap.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_TRAP);

    if (mask.s.keep_orig_pkt)
        reg_err_trap.bf.keep_orig_pkt = config->keep_orig_pkt;
    if (mask.s.rdir_ldpid   )
        reg_err_trap.bf.rdir_ldpid    = config->rdir_ldpid   ;
    if (mask.s.rdir_cos     )
        reg_err_trap.bf.rdir_cos      = config->rdir_cos     ;
    if (mask.s.t0_ctrl      )
        reg_err_trap.bf.t0_ctrl       = config->t0_ctrl      ;
    if (mask.s.t1_ctrl      )
        reg_err_trap.bf.t1_ctrl       = config->t1_ctrl      ;
    if (mask.s.t2_ctrl      )
        reg_err_trap.bf.t2_ctrl       = config->t2_ctrl      ;
    if (mask.s.t3_ctrl      )
        reg_err_trap.bf.t3_ctrl       = config->t3_ctrl      ;
    if (mask.s.t4_ctrl      )
        reg_err_trap.bf.t4_ctrl       = config->t4_ctrl      ;
    if (mask.s.t5_ctrl      )
        reg_err_trap.bf.t5_ctrl       = config->t5_ctrl      ;

    CA_DEV_NE_REG_WRITE2(device_id, L3FE_PP_PARSING_ERR_TRAP, reg_err_trap.wrd);

    return CA_E_OK;

}


ca_status_t  aal_l3_specpkt_err_trap_cfg_get(
    CA_IN  ca_device_id_t                       device_id,
    CA_OUT aal_l3_specpkt_err_trap_cfg_t    *config
)
{
    L3FE_PP_PARSING_ERR_TRAP_t  reg_err_trap;

    AAL_ASSERT_RET(config != NULL, CA_E_PARAM);

    reg_err_trap.wrd = CA_DEV_NE_REG_READ(device_id, L3FE_PP_PARSING_ERR_TRAP);

    config->keep_orig_pkt = reg_err_trap.bf.keep_orig_pkt ;
    config->rdir_ldpid    = reg_err_trap.bf.rdir_ldpid    ;
    config->rdir_cos      = reg_err_trap.bf.rdir_cos      ;
    config->t0_ctrl       = reg_err_trap.bf.t0_ctrl       ;
    config->t1_ctrl       = reg_err_trap.bf.t1_ctrl       ;
    config->t2_ctrl       = reg_err_trap.bf.t2_ctrl       ;
    config->t3_ctrl       = reg_err_trap.bf.t3_ctrl       ;
    config->t4_ctrl       = reg_err_trap.bf.t4_ctrl       ;
    config->t5_ctrl       = reg_err_trap.bf.t5_ctrl       ;

    return CA_E_OK;

}

#define __OMCI_ETHTYPE 0xFFF1 //0x8902

ca_status_t  aal_l3_specpkt_init(ca_device_id_t device_id)
{
    aal_l3_specpkt_detect_mask_t l3_detect_mask;
    aal_l3_specpkt_detect_t      l3_detect;
    ca_status_t                  ret = CA_E_OK;

    memset(&l3_detect_mask, 0, sizeof(aal_l3_specpkt_detect_mask_t));
    memset(&l3_detect, 0, sizeof(aal_l3_specpkt_detect_t));

    l3_detect_mask.s.l4_chksum_rh_chk_enable = 1;
    l3_detect_mask.s.l4_chksum_chk_enable = 1;
    l3_detect_mask.s.ipv4_chksum_chk_enable = 1;
    l3_detect_mask.s.rdp_v1_detection_enable = 1;
    l3_detect_mask.s.l4_spcl_hdr_detct_enable = 1;
    l3_detect_mask.s.l3_spcl_hdr_detct_enable = 1;
    l3_detect_mask.s.l2_spcl_hdr_detct_enable = 1;
    l3_detect_mask.s.my_mac_detct_enable = 1;
    l3_detect_mask.s.mpcp_opcode_chk_enable = 1;
    l3_detect_mask.s.check_arp_target_ip_enable = 1;
    l3_detect_mask.s.udp_lite_detct_enable = 1;
    l3_detect_mask.s.l4ptp_detct_enable = 1;
    l3_detect_mask.s.l4_msgs_detct_enable = 1;
    l3_detect_mask.s.l4_spcl_hdr_detct_enable = 1;
    l3_detect_mask.s.dns_detct_enable = 1;
    l3_detect_mask.s.rip_detct_enable = 1;
    l3_detect_mask.s.ssdp_detct_enable = 1;
    l3_detect_mask.s.dhcpv4_detct_enable = 1;
    l3_detect_mask.s.dhcpv6_detct_enable = 1;

    l3_detect_mask.s.user_def_ethtype_2 = 1;
    l3_detect_mask.s.user_def_spcl_2_enable = 1;

    l3_detect.user_def_ethtype_2 = __OMCI_ETHTYPE;
    l3_detect.user_def_spcl_2_enable = 1;

    l3_detect.l4_chksum_rh_chk_enable = 0;
    l3_detect.l4_chksum_chk_enable = 1;
    l3_detect.ipv4_chksum_chk_enable = 1;
    l3_detect.rdp_v1_detection_enable = 0;
    l3_detect.l4_spcl_hdr_detct_enable = 1;
    l3_detect.l3_spcl_hdr_detct_enable = 1;
    l3_detect.l2_spcl_hdr_detct_enable = 1;
    l3_detect.my_mac_detct_enable = 1;
    l3_detect.mpcp_opcode_chk_enable = 1;
    l3_detect.check_arp_target_ip_enable = 0;
    l3_detect.udp_lite_detct_enable = 1;
    l3_detect.l4ptp_detct_enable = 1;
    l3_detect.l4_msgs_detct_enable = 1;
    l3_detect.l4_spcl_hdr_detct_enable = 1;
    l3_detect.dns_detct_enable = 1;
    l3_detect.rip_detct_enable = 1;
    l3_detect.ssdp_detct_enable = 1;
    l3_detect.dhcpv4_detct_enable = 1;
    l3_detect.dhcpv6_detct_enable = 1;

    /* Init SIP special packet detect */
    l3_detect_mask.s.http_link0 = 1;
    l3_detect_mask.s.http_link1 = 1;

    l3_detect.http_link0_detect_enable = 1;
    l3_detect.http_link0_lspid_valid   = 0;
    l3_detect.http_link0_lspid         = 0;
    l3_detect.http_link0_l4_type       = AAL_L3_SPECPKT_HTTPLINK_TYPE_BOTH_TCP_UDP;
    l3_detect.http_link0_l4_port_type  = AAL_L3_SPECPKT_HTTPLINK_L4_PORT_TYPE_SRC;
    l3_detect.http_link0_l4_port       = __L3FE_SIP_L4PORT;

    l3_detect.http_link1_detect_enable = 1;
    l3_detect.http_link1_lspid_valid   = 0;
    l3_detect.http_link1_lspid         = 0;
    l3_detect.http_link1_l4_type       = AAL_L3_SPECPKT_HTTPLINK_TYPE_BOTH_TCP_UDP;
    l3_detect.http_link1_l4_port_type  = AAL_L3_SPECPKT_HTTPLINK_L4_PORT_TYPE_DST;
    l3_detect.http_link1_l4_port       = __L3FE_SIP_L4PORT;

    ret = aal_l3_specpkt_detect_set(device_id, l3_detect_mask, &l3_detect);

    return ret;

}

#ifdef __KERNEL__

EXPORT_SYMBOL(aal_l3_specpkt_init);
EXPORT_SYMBOL(aal_l3_specpkt_ctrl_set);
EXPORT_SYMBOL(aal_l3_specpkt_ctrl_get);
EXPORT_SYMBOL(aal_l3_specpkt_detect_get);
EXPORT_SYMBOL(aal_l3_specpkt_detect_set);
EXPORT_SYMBOL(aal_l3_specpkt_parser_status_get);
EXPORT_SYMBOL(aal_l3_specpkt_err_fwd_ctrl_set);
EXPORT_SYMBOL(aal_l3_specpkt_err_fwd_ctrl_get);
EXPORT_SYMBOL(aal_l3_specpkt_err_trap_cfg_set);
EXPORT_SYMBOL(aal_l3_specpkt_err_trap_cfg_get);

#endif

