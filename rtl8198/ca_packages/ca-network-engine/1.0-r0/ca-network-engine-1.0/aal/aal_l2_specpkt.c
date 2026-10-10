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
/* aal_l2_specpkt.c L2FE special packet AAL driver*/

#include "osal_common.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_l2_specpkt.h"


/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/
#define __L2FE_PORT_MAX       (AAL_PORT_NUM_PHYSICAL)
#define __L2FE_SIP_L4PORT     (5060)
#define __L2FE_SIP_TLS_L4PORT (5061)

#define __FE_TABLE_ACCESS_TIMEOUT  10000
#define __FE_TABLE_WRITE_FLAG      1
#define __FE_TABLE_READ_FLAG       0

/*
 * L2 FE/ L3 FE hardware register access MACROs
 */
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __L2FE_REG_READ(dev,addr)                 (((dev)==0)? CA_NE_REG_READ(addr): CA_HGU_8279_NE_REG_READ(SATURN_##addr))
#define __L2FE_REG_WRITE(dev,addr,data)        (((dev)==0)? CA_NE_REG_WRITE(data,addr): CA_HGU_8279_NE_REG_WRITE(data,SATURN_##addr))
#define __L2FE_TABLE_ACCESS(dev,rw,tbl_name,idx,acc_reg)  do { \
                ca_uint32_t  timeout = 0;                         \
                (acc_reg).wrd        = 0;                         \
                (acc_reg).bf.access  = 1;                         \
                (acc_reg).bf.rbw     = (rw);                      \
                (acc_reg).bf.addr    = (idx);                     \
                (((dev)==0)? CA_NE_REG_WRITE((acc_reg).wrd,tbl_name): CA_HGU_8279_NE_REG_WRITE((acc_reg).wrd,SATURN_##tbl_name));  \
                do {                                                 \
                    (acc_reg).wrd = (((dev)==0)? CA_NE_REG_READ(tbl_name): CA_HGU_8279_NE_REG_READ(SATURN_##tbl_name));\
                    if (timeout++ > __FE_TABLE_ACCESS_TIMEOUT)       \
                        return CA_E_TIMEOUT;                         \
                }while(0 != (acc_reg).bf.access);                    \
            }while(0)

#define __L2FE_SPK_MAC_SET(a,b,c,d)           __l2_specpkt_mac_set(a,b,SATURN_##c,SATURN_##d)
#define __L2FE_SPK_MAC_GET(a,b,c,d)           __l2_specpkt_mac_get(a,b,SATURN_##c,d)

#else
#define __L2FE_REG_READ(dev,addr)             CA_NE_REG_READ(addr)
#define __L2FE_REG_WRITE(dev,addr,data)       CA_NE_REG_WRITE(data,addr)

/*
 * L2 FE/ L3 FE hardware table access MACROs
 */
#define __L2FE_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg)   do {     \
                ca_uint32_t  timeout = 0;                            \
                do {                                                 \
                    (acc_reg).wrd = __L2FE_REG_READ(device_id,tbl_name);\
                    if (timeout++ > __FE_TABLE_ACCESS_TIMEOUT)       \
                        return CA_E_TIMEOUT;                         \
                }while(0 != (acc_reg).bf.access);                    \
            }while(0)

#define __L2FE_TABLE_ACCESS(device_id,rw,tbl_name,idx,acc_reg)  do { \
                (acc_reg).wrd        = 0;                         \
                (acc_reg).bf.access  = 1;                         \
                (acc_reg).bf.rbw     = (rw);                      \
                (acc_reg).bf.addr    = (idx);                     \
                __L2FE_REG_WRITE(device_id,tbl_name,(acc_reg).wrd);  \
                __L2FE_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg);\
            }while(0)

#define __L2FE_SPK_MAC_SET(a,b,c,d)           __l2_specpkt_mac_set(a,b,c,d)
#define __L2FE_SPK_MAC_GET(a,b,c,d)           __l2_specpkt_mac_get(a,b,c,d)

#endif

/******************************************************************************/
/*                             Local Functions                                */
/******************************************************************************/

/** @__l2_specpkt_mac_set:
 *  to set MAC address related hardware registers
 */
static void __l2_specpkt_mac_set(ca_device_id_t device_id,
                 ca_mac_addr_t mac_cfg,
                 ca_uint32_t reg_addr_hi,
                 ca_uint32_t reg_addr_low)
{
    ca_uint32_t reg_val0 = 0, reg_val1 = 0;

    reg_val0 = (mac_cfg[0] << 8) | mac_cfg[1];
    reg_val1 = (mac_cfg[2] << 24) |
        (mac_cfg[3] << 16) | (mac_cfg[4] << 8) | mac_cfg[5];

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_ne_reg_write(device_id, reg_val0, reg_addr_hi);
    ca_ne_reg_write(device_id, reg_val1, reg_addr_low);
#else
    __L2FE_REG_WRITE(device_id, reg_addr_hi, reg_val0);
    __L2FE_REG_WRITE(device_id, reg_addr_low, reg_val1);
#endif


}

static void __l2_specpkt_mac_get(ca_device_id_t device_id,
                 ca_uint32_t reg_addr_hi,
                 ca_uint32_t reg_addr_low,
                 ca_mac_addr_t mac_cfg)
{
    ca_uint32_t reg_val0 = 0, reg_val1 = 0;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    reg_val0 = ca_ne_reg_read(device_id, reg_addr_hi);
    reg_val1 = ca_ne_reg_read(device_id, reg_addr_low);
#else
    reg_val0 = __L2FE_REG_READ(device_id, reg_addr_hi);
    reg_val1 = __L2FE_REG_READ(device_id, reg_addr_low);
#endif

    mac_cfg[0] = (reg_val0 >> 8) & 0xFF;
    mac_cfg[1] = (reg_val0) & 0xFF;
    mac_cfg[2] = (reg_val1 >> 24) & 0xFF;
    mac_cfg[3] = (reg_val1 >> 16) & 0xFF;
    mac_cfg[4] = (reg_val1 >> 8) & 0xFF;
    mac_cfg[5] = (reg_val1) & 0xFF;

}

/* compare the input DA MAC and the predefined HELLO packet DA MAC */
ca_status_t aal_l2_specpkt_match_hello_packet(ca_mac_addr_t da_mac_addr)
{
    int i;
    ca_mac_addr_t hello_mac_addr;
    ca_mac_addr_t hello_mac_mask;

    //printk("%s: DA MAC addr=%pM\n", __func__, da_mac_addr);
    __L2FE_SPK_MAC_GET(0, L2FE_PP_HELLO_PKT_DA_HI, L2FE_PP_HELLO_PKT_DA_LOW, hello_mac_addr);
    //printk("%s: HELLO MAC addr=%pM\n", __func__, hello_mac_addr);
    __L2FE_SPK_MAC_GET(0, L2FE_PP_HELLO_PKT_ENABLE_DA_HI, L2FE_PP_HELLO_PKT_ENABLE_DA_LOW, hello_mac_mask);
    //printk("%s: HELLO MAC mask=%pM\n", __func__, hello_mac_mask);
    for (i = 0; i < sizeof(ca_mac_addr_t); i++) {
        if ((hello_mac_addr[i] & hello_mac_mask[i]) != (da_mac_addr[i] & hello_mac_mask[i])) {
            return CA_E_ERROR;
        }
    }
    return CA_E_OK;
}

/* compare the input ethertype and DA MAC with user define packet */
ca_status_t aal_l2_specpkt_match_udf_packet(ca_mac_addr_t da_mac_addr, ca_uint16_t ethtype)
{
    int i;
    ca_mac_addr_t udf0_mac_addr, udf0_mac_mask;
    ca_uint32_t reg_val0;
    ca_uint16_t udf0_ethtype;

    /* User defined packet #0 */
    reg_val0 = __L2FE_REG_READ(0, L2FE_PP_SPECIAL_EV2PT);
    udf0_ethtype = reg_val0 & 0xFFFF;
    __L2FE_SPK_MAC_GET(0, L2FE_PP_SPECIAL_DA_0_HI, L2FE_PP_SPECIAL_DA_0_LOW, udf0_mac_addr);
    __L2FE_SPK_MAC_GET(0, L2FE_PP_SPECIAL_ENABLE_DA_0_HI, L2FE_PP_SPECIAL_ENABLE_DA_0_LOW, udf0_mac_mask);

    if (ethtype != udf0_ethtype)
	return CA_E_ERROR;
    for (i = 0; i < sizeof(ca_mac_addr_t); i++) {
        if ((udf0_mac_addr[i] & udf0_mac_mask[i]) != (da_mac_addr[i] & udf0_mac_mask[i])) {
            return CA_E_ERROR;
        }
    }
    return CA_E_OK;
}

/*********************************************************************************/
/*                Layer 2 special packet AAL implementation                       */
/*********************************************************************************/

/*
 * The EtherType encoding process use received EtherType value of the packet to
 * compare the entries of EtherType Table one by one, if there is hit, the
 * encoded value is got. If there is no hit from this table, the packet will
 * get a default 5*h1F value.
 * Normally, the ethtype encode is used for Classifer key
 */
ca_status_t  aal_l2_specpkt_ethtype_encode_set(
    CA_IN ca_device_id_t                            device_id,
    CA_IN ca_uint32_t                            entry_id,
    CA_IN aal_l2_specpkt_ethtype_encode_mask_t   mask,
    CA_IN aal_l2_specpkt_ethtype_encode_t       *config
)
{
    L2FE_PLA_EV2_PT_CNVT_ACCESS_t tbl_acc;
    L2FE_PLA_EV2_PT_CNVT_DATA_t tbl_data;


    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    AAL_ASSERT_RET(entry_id <= AAL_L2_SPECPKT_ETHTYPE_ENTRY_ID_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(config->ethertype >= AAL_L2_SPECPKT_ETHTYPE_MIN, CA_E_PARAM);
    AAL_ASSERT_RET(config->encoded_value <= AAL_L2_SPECPKT_ETHTYPE_ENCODE_MAX,
        CA_E_PARAM);

    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_READ_FLAG,
                L2FE_PLA_EV2_PT_CNVT_ACCESS, entry_id, tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_EV2_PT_CNVT_DATA);

    if (mask.s.encoded_value)
        tbl_data.bf.ev2_pt_cnvt_encoded_value = config->encoded_value;

    if (mask.s.ethertype)
        tbl_data.bf.ev2_pt_cnvt_ethertype = config->ethertype;

    __L2FE_REG_WRITE(device_id, L2FE_PLA_EV2_PT_CNVT_DATA, tbl_data.wrd);

    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_WRITE_FLAG,
                L2FE_PLA_EV2_PT_CNVT_ACCESS, entry_id, tbl_acc);

    return CA_E_OK;

}

ca_status_t  aal_l2_specpkt_ethtype_encode_get(
    CA_IN  ca_device_id_t                      device_id,
    CA_IN  ca_uint32_t                      entry_id,
    CA_OUT aal_l2_specpkt_ethtype_encode_t *config
)
{
    L2FE_PLA_EV2_PT_CNVT_ACCESS_t tbl_acc;
    L2FE_PLA_EV2_PT_CNVT_DATA_t tbl_data;

    CA_ASSERT_RET(config, CA_E_PARAM);
    CA_ASSERT_RET(entry_id <= AAL_L2_SPECPKT_ETHTYPE_ENTRY_ID_MAX, CA_E_PARAM);

    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_READ_FLAG,
                L2FE_PLA_EV2_PT_CNVT_ACCESS, entry_id, tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_EV2_PT_CNVT_DATA);

    config->encoded_value = tbl_data.bf.ev2_pt_cnvt_encoded_value;
    config->ethertype = tbl_data.bf.ev2_pt_cnvt_ethertype;

    return CA_E_OK;

}

/* The special packet detecting configurations are used
 * to detect ASIC defined special type packet
 */
ca_status_t aal_l2_specpkt_detect_set(CA_IN ca_device_id_t device_id,
                      CA_IN aal_l2_specpkt_detect_mask_t mask,
                      CA_IN aal_l2_specpkt_detect_t * config)
{
    ca_uint32_t reg_val1 = 0;
    L2FE_PP_PARSER_CTRL_t paser_ctrl;
    L2FE_PP_HTTP_LINK_0_t l4_sppkt0;
    L2FE_PP_HTTP_LINK_1_t l4_sppkt1;
    L2FE_PP_HTTP_LINK_2_t l4_sppkt2;

    CA_ASSERT_RET(mask.u32, CA_E_PARAM);
    CA_ASSERT_RET(config, CA_E_PARAM);

    if (mask.s.hello_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->hello_mac,
                     L2FE_PP_HELLO_PKT_DA_HI,
                     L2FE_PP_HELLO_PKT_DA_LOW);
    }

    if (mask.s.hello_mac_mask) {
        __L2FE_SPK_MAC_SET(device_id, config->hello_mac_mask,
                     L2FE_PP_HELLO_PKT_ENABLE_DA_HI,
                     L2FE_PP_HELLO_PKT_ENABLE_DA_LOW);
    }

    if (mask.s.udf0_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->udf0_mac,
                     L2FE_PP_SPECIAL_DA_0_HI,
                     L2FE_PP_SPECIAL_DA_0_LOW);
    }

    if (mask.s.udf0_mac_mask) {
        __L2FE_SPK_MAC_SET(device_id, config->udf0_mac_mask,
                     L2FE_PP_SPECIAL_ENABLE_DA_0_HI,
                     L2FE_PP_SPECIAL_ENABLE_DA_0_LOW);
    }

    if (mask.s.udf0_ethtype) {
        __L2FE_REG_WRITE(device_id, L2FE_PP_SPECIAL_EV2PT,
                 config->udf0_ethtype);
    }

    if (mask.s.udf1_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->udf1_mac,
                     L2FE_PP_SPECIAL_DA_1_HI,
                     L2FE_PP_SPECIAL_DA_1_LOW);
    }

    if (mask.s.udf1_mac_mask) {
        __L2FE_SPK_MAC_SET(device_id, config->udf1_mac_mask,
                     L2FE_PP_SPECIAL_ENABLE_DA_1_HI,
                     L2FE_PP_SPECIAL_ENABLE_DA_1_LOW);
    }

    if (mask.s.udf1_vlan) {
        __L2FE_REG_WRITE(device_id, L2FE_PP_SPECIAL_VLAN,
                 config->udf1_vlan);
    }

    if (mask.s.my_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->my_mac,
                     L2FE_PP_MY_MAC_HI, L2FE_PP_MY_MAC_LOW);
    }

    if (mask.s.my_mac_mask) {
        /* Note: ASIC only mask low 24 bits */
        reg_val1 = (config->my_mac_mask[3] << 16) |
            (config->my_mac_mask[4] << 8) | (config->my_mac_mask[5]);

        __L2FE_REG_WRITE(device_id, L2FE_PP_MY_MAC_ENABLE_LOW, reg_val1);
    }

    if (mask.s.my_mac1) {
        __L2FE_SPK_MAC_SET(device_id, config->my_mac1,
                     L2FE_PP_MY_MAC_HI_1, L2FE_PP_MY_MAC_LOW_1);
    }

    if (mask.s.my_mac1_mask) {
        /* Note: ASIC only mask low 24 bits */
        reg_val1 = (config->my_mac1_mask[3] << 16) |
            (config->my_mac1_mask[4] << 8) | (config->my_mac1_mask[5]);

        __L2FE_REG_WRITE(device_id, L2FE_PP_MY_MAC_ENABLE_LOW_1, reg_val1);
    }

    if (mask.s.rsvd_mc_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->rsvd_mc_mac,
                     L2FE_PP_RSV_MAC_MC_HI,
                     L2FE_PP_RSV_MAC_MC_LOW);
    }

    if (mask.s.rsvd_mc_mac_mask) {
        __L2FE_SPK_MAC_SET(device_id, config->rsvd_mc_mac_mask,
                     L2FE_PP_RSV_ENABLE_MC_HI,
                     L2FE_PP_RSV_ENABLE_MC_LOW);
    }

    if (mask.s.rsvd_grp_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->rsvd_grp_mac,
                     L2FE_PP_RSV_GROUP_MAC_HI,
                     L2FE_PP_RSV_GROUP_MAC_LOW);
    }

    if (mask.s.rsvd_grp_mac_mask) {
        __L2FE_SPK_MAC_SET(device_id, config->rsvd_grp_mac_mask,
                     L2FE_PP_RSV_GROUP_ENABLE_HI,
                     L2FE_PP_RSV_GROUP_ENABLE_LOW);
    }

    if (mask.s.mef0_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->mef0_mac,
                     L2FE_PP_MEF_OAM_MAC_0_HI,
                     L2FE_PP_MEF_OAM_MAC_0_LOW);
    }

    if (mask.s.mef0_ethtype) {
        __L2FE_REG_WRITE(device_id, L2FE_PP_MEF_OAM_TYPE_0,
                 config->mef0_ethtype);
    }

    if (mask.s.mef1_mac) {
        __L2FE_SPK_MAC_SET(device_id, config->mef1_mac,
                     L2FE_PP_MEF_OAM_MAC_1_HI,
                     L2FE_PP_MEF_OAM_MAC_1_LOW);
    }

    if (mask.s.mef1_ethtype) {
        __L2FE_REG_WRITE(device_id, L2FE_PP_MEF_OAM_TYPE_1,
                 config->mef1_ethtype);
    }

    if (mask.s.l7_offset) {
        __L2FE_REG_WRITE(device_id, L2FE_PP_L7_OFFSET,
                 config->l7_offset & 0x7F);
    }

    /* Other control */
    if (!(mask.s.efm_oam_subtype_detect_enable ||
          mask.s.efm_oam_subtype ||
          mask.s.udf0_detect_enable ||
          mask.s.udf0_ethtype_enable ||
          mask.s.udf1_detect_enable || mask.s.udf1_vlan_enable)) {
        return CA_E_OK;
    }

    paser_ctrl.wrd = __L2FE_REG_READ(device_id, L2FE_PP_PARSER_CTRL);

    if (mask.s.efm_oam_subtype_detect_enable) {
        paser_ctrl.bf.detect_oam_subtype =
            config->efm_oam_subtype_detect_enable;
    }

    if (mask.s.efm_oam_subtype) {
        paser_ctrl.bf.oam_subtype = config->efm_oam_subtype;
    }

    if (mask.s.udf0_detect_enable) {
        paser_ctrl.bf.udf0_det_en = config->udf0_detect_enable;
    }

    if (mask.s.udf0_ethtype_enable) {
        paser_ctrl.bf.udf0_det_type_en = config->udf0_ethtype_enable;
    }

    if (mask.s.udf1_detect_enable) {
        paser_ctrl.bf.udf1_det_en = config->udf1_detect_enable;
    }

    if (mask.s.udf1_vlan_enable) {
        paser_ctrl.bf.udf1_det_vlan_en = config->udf1_vlan_enable;
    }

    if (mask.s.ipv4_checksum_check_enable) {
        paser_ctrl.bf.ipv4_checksum_check_enable = config->ipv4_checksum_check_enable;
    }

    if (mask.s.egr_ptp_pkt_no_drop_set) {
        paser_ctrl.bf.egr_ptp_pkt_no_drop_set = config->egr_ptp_pkt_no_drop_set;
    }

    if (mask.s.l3fe_to_l2fe_lrn_ctrl) {
        paser_ctrl.bf.l3fe_to_l2fe_lrn_ctrl = config->l3fe_to_l2fe_lrn_ctrl;
    }

    if (mask.s.use_ip_addr_detect_mc) {
        paser_ctrl.bf.use_ip_addr_mc = config->use_ip_addr_detect_mc;
    }

    __L2FE_REG_WRITE(device_id, L2FE_PP_PARSER_CTRL, paser_ctrl.wrd);

    if (mask.s.http_link0) {
        l4_sppkt0.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_0);
        l4_sppkt0.bf.rule_l4_pid     = config->http_link0_l4_port;
        l4_sppkt0.bf.rule_l4_pid_sel = config->http_link0_l4_port_type;
        l4_sppkt0.bf.rule_l4_type    = config->http_link0_l4_type;
        l4_sppkt0.bf.rule_lspid      = config->http_link0_lspid;
        l4_sppkt0.bf.rule_lspid_msk  = !config->http_link0_lspid_valid;
        l4_sppkt0.bf.rule_vld        = config->http_link0_detect_enable;
        __L2FE_REG_WRITE(device_id, L2FE_PP_HTTP_LINK_0, l4_sppkt0.wrd);
    }

    if (mask.s.http_link1) {
        l4_sppkt1.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_1);
        l4_sppkt1.bf.rule_l4_pid     = config->http_link1_l4_port;
        l4_sppkt1.bf.rule_l4_pid_sel = config->http_link1_l4_port_type;
        l4_sppkt1.bf.rule_l4_type    = config->http_link1_l4_type;
        l4_sppkt1.bf.rule_lspid      = config->http_link1_lspid;
        l4_sppkt1.bf.rule_lspid_msk  = !config->http_link1_lspid_valid;
        l4_sppkt1.bf.rule_vld        = config->http_link1_detect_enable;
        __L2FE_REG_WRITE(device_id, L2FE_PP_HTTP_LINK_1, l4_sppkt1.wrd);
    }

    if (mask.s.http_link2) {
        l4_sppkt2.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_2);
        l4_sppkt2.bf.rule_l4_pid     = config->http_link2_l4_port;
        l4_sppkt2.bf.rule_l4_pid_sel = config->http_link2_l4_port_type;
        l4_sppkt2.bf.rule_l4_type    = config->http_link2_l4_type;
        l4_sppkt2.bf.rule_lspid      = config->http_link2_lspid;
        l4_sppkt2.bf.rule_lspid_msk  = !config->http_link2_lspid_valid;
        l4_sppkt2.bf.rule_vld        = config->http_link2_detect_enable;
        __L2FE_REG_WRITE(device_id, L2FE_PP_HTTP_LINK_2, l4_sppkt2.wrd);
    }

    return CA_E_OK;

}

ca_status_t aal_l2_specpkt_detect_get(CA_IN ca_device_id_t device_id,
                      CA_OUT aal_l2_specpkt_detect_t * config)
{
    ca_uint32_t reg_val0 = 0;
    ca_uint32_t reg_val1 = 0;
    L2FE_PP_PARSER_CTRL_t paser_ctrl;
    L2FE_PP_HTTP_LINK_0_t l4_sppkt0;
    L2FE_PP_HTTP_LINK_1_t l4_sppkt1;
    L2FE_PP_HTTP_LINK_2_t l4_sppkt2;

    CA_ASSERT_RET(NULL != config, CA_E_PARAM);

    /* iRos Hello Packet */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_HELLO_PKT_DA_HI,
                 L2FE_PP_HELLO_PKT_DA_LOW, config->hello_mac);

    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_HELLO_PKT_ENABLE_DA_HI,
                 L2FE_PP_HELLO_PKT_ENABLE_DA_LOW,
                 config->hello_mac_mask);

    /* User defined packet #0 */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_SPECIAL_DA_0_HI,
                 L2FE_PP_SPECIAL_DA_0_LOW, config->udf0_mac);

    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_SPECIAL_ENABLE_DA_0_HI,
                 L2FE_PP_SPECIAL_ENABLE_DA_0_LOW,
                 config->udf0_mac_mask);

    l4_sppkt0.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_0);
    l4_sppkt1.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_1);
    l4_sppkt2.wrd = __L2FE_REG_READ(device_id, L2FE_PP_HTTP_LINK_2);
    reg_val0      = __L2FE_REG_READ(device_id, L2FE_PP_SPECIAL_EV2PT);

    config->udf0_ethtype = reg_val0 & 0xFFFF;

    /* User defined packet #1 */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_SPECIAL_DA_1_HI,
                 L2FE_PP_SPECIAL_DA_1_LOW, config->udf1_mac);

    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_SPECIAL_ENABLE_DA_1_HI,
                 L2FE_PP_SPECIAL_ENABLE_DA_1_LOW,
                 config->udf1_mac_mask);

    reg_val0 = __L2FE_REG_READ(device_id, L2FE_PP_SPECIAL_VLAN);
    config->udf1_vlan = reg_val0 & 0xFFFF;

    /* My MAC0 */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_MY_MAC_HI,
                 L2FE_PP_MY_MAC_LOW, config->my_mac);

    reg_val1 = __L2FE_REG_READ(device_id, L2FE_PP_MY_MAC_ENABLE_LOW);

    config->my_mac_mask[0] = 0xFF;
    config->my_mac_mask[1] = 0xFF;
    config->my_mac_mask[2] = 0xFF;
    config->my_mac_mask[3] = (reg_val1 >> 16) & 0xFF;
    config->my_mac_mask[4] = (reg_val1 >> 8) & 0xFF;
    config->my_mac_mask[5] = (reg_val1) & 0xFF;

    /* My MAC1 */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_MY_MAC_HI_1,
                 L2FE_PP_MY_MAC_LOW_1, config->my_mac1);

    reg_val1 = __L2FE_REG_READ(device_id, L2FE_PP_MY_MAC_ENABLE_LOW_1);

    config->my_mac1_mask[0] = 0xFF;
    config->my_mac1_mask[1] = 0xFF;
    config->my_mac1_mask[2] = 0xFF;
    config->my_mac1_mask[3] = (reg_val1 >> 16) & 0xFF;
    config->my_mac1_mask[4] = (reg_val1 >> 8) & 0xFF;
    config->my_mac1_mask[5] = (reg_val1) & 0xFF;

    /* Reserved MC MAC */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_RSV_MAC_MC_HI,
                 L2FE_PP_RSV_MAC_MC_LOW, config->rsvd_mc_mac);

    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_RSV_ENABLE_MC_HI,
                 L2FE_PP_RSV_ENABLE_MC_LOW,
                 config->rsvd_mc_mac_mask);

    /* Reserved GROUP MAC */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_RSV_GROUP_MAC_HI,
                 L2FE_PP_RSV_GROUP_MAC_LOW, config->rsvd_grp_mac);

    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_RSV_GROUP_ENABLE_HI,
                 L2FE_PP_RSV_GROUP_ENABLE_LOW,
                 config->rsvd_grp_mac_mask);

    /* MEF OAM 0 MAC */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_MEF_OAM_MAC_0_HI,
                 L2FE_PP_MEF_OAM_MAC_0_LOW, config->mef0_mac);

    reg_val0 = __L2FE_REG_READ(device_id, L2FE_PP_MEF_OAM_TYPE_0);
    config->mef0_ethtype = (reg_val0) & 0xFFFF;

    /* MEF OAM 1 MAC */
    __L2FE_SPK_MAC_GET(device_id, L2FE_PP_MEF_OAM_MAC_1_HI,
                 L2FE_PP_MEF_OAM_MAC_1_LOW, config->mef1_mac);

    reg_val0 = __L2FE_REG_READ(device_id, L2FE_PP_MEF_OAM_TYPE_1);
    config->mef1_ethtype = (reg_val0) & 0xFFFF;

    /* Layer7 offset bytes */

    reg_val0 = __L2FE_REG_READ(device_id, L2FE_PP_L7_OFFSET);
    config->l7_offset = (reg_val0) & 0x7F;

    /* Other control */
    paser_ctrl.wrd = __L2FE_REG_READ(device_id, L2FE_PP_PARSER_CTRL);

    config->efm_oam_subtype_detect_enable = paser_ctrl.bf.detect_oam_subtype;
    config->efm_oam_subtype = paser_ctrl.bf.oam_subtype;
    config->udf0_detect_enable = paser_ctrl.bf.udf0_det_en;
    config->udf0_ethtype_enable = paser_ctrl.bf.udf0_det_type_en;
    config->udf1_detect_enable = paser_ctrl.bf.udf1_det_en;
    config->udf1_vlan_enable = paser_ctrl.bf.udf1_det_vlan_en;
    config->ipv4_checksum_check_enable = paser_ctrl.bf.ipv4_checksum_check_enable;
    config->egr_ptp_pkt_no_drop_set = paser_ctrl.bf.egr_ptp_pkt_no_drop_set;
    config->l3fe_to_l2fe_lrn_ctrl = paser_ctrl.bf.l3fe_to_l2fe_lrn_ctrl;
    config->use_ip_addr_detect_mc = paser_ctrl.bf.use_ip_addr_mc;

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

    return CA_E_OK;

}

/* The special packet control configurations are used
 * to set forward/qos/learning/rate-limit.etc behaviors for special packet
 */
ca_status_t aal_l2_specpkt_ctrl_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_uint32_t                spb_idx,
                    CA_IN aal_l2_specpkt_type_t      pkt_type,
                    CA_IN aal_l2_specpkt_ctrl_mask_t mask,
                    CA_IN aal_l2_specpkt_ctrl_t     *config)
{
    L2FE_PLA_SPB_ACCESS_t tbl_acc;
    L2FE_PLA_SPB_DATA_t tbl_data;

    CA_ASSERT_RET(config, CA_E_PARAM);
    CA_ASSERT_RET(spb_idx <= AAL_L2_SPECPKT_TBL_IDX_MAX, CA_E_PARAM);
    CA_ASSERT_RET(pkt_type <= AAL_L2_SPECPKT_TYPE_MAX, CA_E_PARAM);

    tbl_acc.wrd = 0;
    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_SPB_DATA);

    /* ilpb.spb_idx[2:0], spcl_pkt_encoding[4:0] */
    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_READ_FLAG,
                L2FE_PLA_SPB_ACCESS, (spb_idx << 5) | pkt_type,
                tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_SPB_DATA);

    if (mask.s.pkt_no_drop)
        tbl_data.bf.spb_pkt_no_drop = config->pkt_no_drop;
    if (mask.s.pkt_no_edit)
        tbl_data.bf.spb_pkt_no_edit = config->pkt_no_edit;
    if (mask.s.keep_ts)
        tbl_data.bf.spb_keep_ts = config->keep_ts;
    if (mask.s.bypass_sa_lrn_chk)
        tbl_data.bf.spb_bypass_sa_lrn_chk = config->bypass_sa_lrn_chk;
    if (mask.s.limit_fwd_permit)
        tbl_data.bf.spb_limit_fwd_permit = config->limit_fwd_permit;
    if (mask.s.learn_dis)
        tbl_data.bf.spb_learn_dis = config->learn_dis;
    if (mask.s.dscp_mark_down)
        tbl_data.bf.spb_dscp_mark_down = config->dscp_mark_down;
    if (mask.s.spcl_fwd)
        tbl_data.bf.spb_spcl_fwd = config->spcl_fwd;
    if (mask.s.rate_limit_bypass)
        tbl_data.bf.spb_rate_limit_bypass = config->rate_limit_bypass;
    if (mask.s.flow_en)
        tbl_data.bf.spb_flow_en = config->flow_en;
    if (mask.s.cos)
        tbl_data.bf.spb_cos = config->cos;
    if (mask.s.ldpid)
        tbl_data.bf.spb_ldpid = config->ldpid;
    if (mask.s.flowid)
        tbl_data.bf.spb_flowid = config->flowid;

    __L2FE_REG_WRITE(device_id, L2FE_PLA_SPB_DATA, tbl_data.wrd);

    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_WRITE_FLAG,
                L2FE_PLA_SPB_ACCESS, (spb_idx << 5) | pkt_type, tbl_acc);

    return CA_E_OK;

}

ca_status_t aal_l2_specpkt_ctrl_get(CA_IN ca_device_id_t device_id,
                    CA_IN  ca_uint32_t              spb_idx,
                    CA_IN  aal_l2_specpkt_type_t    pkt_type,
                    CA_OUT aal_l2_specpkt_ctrl_t  *config)
{
    L2FE_PLA_SPB_ACCESS_t tbl_acc;
    L2FE_PLA_SPB_DATA_t tbl_data;

    CA_ASSERT_RET(config, CA_E_PARAM);
    CA_ASSERT_RET(spb_idx <= AAL_L2_SPECPKT_TBL_IDX_MAX, CA_E_PARAM);
    CA_ASSERT_RET(pkt_type <= AAL_L2_SPECPKT_TYPE_MAX, CA_E_PARAM);

    __L2FE_TABLE_ACCESS(device_id, __FE_TABLE_READ_FLAG,
                L2FE_PLA_SPB_ACCESS, (spb_idx << 5) | pkt_type,
                tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_SPB_DATA);
    config->pkt_no_drop = tbl_data.bf.spb_pkt_no_drop;
    config->pkt_no_edit = tbl_data.bf.spb_pkt_no_edit;
    config->keep_ts = tbl_data.bf.spb_keep_ts;
    config->bypass_sa_lrn_chk = tbl_data.bf.spb_bypass_sa_lrn_chk;
    config->limit_fwd_permit = tbl_data.bf.spb_limit_fwd_permit;
    config->learn_dis = tbl_data.bf.spb_learn_dis;
    config->dscp_mark_down = tbl_data.bf.spb_dscp_mark_down;
    config->spcl_fwd = tbl_data.bf.spb_spcl_fwd;
    config->rate_limit_bypass = tbl_data.bf.spb_rate_limit_bypass;
    config->flow_en = tbl_data.bf.spb_flow_en;
    config->cos = tbl_data.bf.spb_cos;
    config->ldpid = tbl_data.bf.spb_ldpid;
    config->flowid = tbl_data.bf.spb_flowid;

    return CA_E_OK;

}

/** @aal_l2_specpkt_current_capture_get
 * this is ASIC debug function to display the special type of
 * current packet incoming to ForwardEngine
 */
ca_status_t  aal_l2_specpkt_current_capture_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_l2_specpkt_type_t      *pkt_type
)
{
    L2FE_PP_SPECIAL_PKT_t reg_data;

    CA_ASSERT_RET(NULL != pkt_type, CA_E_PARAM);

    reg_data.wrd = __L2FE_REG_READ(device_id, L2FE_PP_SPECIAL_PKT);
    *pkt_type = (aal_l2_specpkt_type_t) reg_data.bf.type;

    return CA_E_OK;
}

#if 0
#define __OMCI_ETHTYPE 0x8902
#endif

ca_status_t  aal_l2_specpkt_init(ca_device_id_t device_id)
{
    ca_uint8_t  default_hello_da[] = {0x01, 0x13, 0x25, 0x00, 0x00, 0x01};
    aal_l2_specpkt_detect_mask_t l2_detect_mask;
    aal_l2_specpkt_detect_t      l2_detect;
    ca_status_t                  ret = CA_E_OK;

    memset(&l2_detect_mask, 0, sizeof(aal_l2_specpkt_detect_mask_t));
    memset(&l2_detect, 0, sizeof(aal_l2_specpkt_detect_t));

    l2_detect_mask.s.efm_oam_subtype_detect_enable = 1;
    l2_detect.efm_oam_subtype_detect_enable = 0; /* suport other slow protocol */
    /*
    l2_detect_mask.s.efm_oam_subtype = 1;
    l2_detect.efm_oam_subtype = 0x3;
    */
    l2_detect_mask.s.hello_mac = 1;
#if 0
    l2_detect_mask.s.udf0_detect_enable = 1;
    l2_detect_mask.s.udf0_mac = 1;
    l2_detect_mask.s.udf0_mac_mask = 1;
    l2_detect_mask.s.udf0_ethtype = 1;
    l2_detect_mask.s.udf0_ethtype_enable = 1;
    l2_detect.udf0_detect_enable = 1;
    l2_detect.udf0_ethtype = __OMCI_ETHTYPE;
    l2_detect.udf0_ethtype_enable = 1;
#endif
    memcpy(&l2_detect.hello_mac[0], &default_hello_da[0], 6);

    /* Init SIP special packet detect */
    l2_detect_mask.s.http_link0 = 1;
    l2_detect_mask.s.http_link1 = 1;

    l2_detect.http_link0_detect_enable = 1;
    l2_detect.http_link0_lspid_valid   = 0;
    l2_detect.http_link0_lspid         = 0;
    l2_detect.http_link0_l4_type       = AAL_L2_SPECPKT_HTTPLINK_TYPE_BOTH_TCP_UDP;
    l2_detect.http_link0_l4_port_type  = AAL_L2_SPECPKT_HTTPLINK_L4_PORT_TYPE_SRC;
    l2_detect.http_link0_l4_port       = __L2FE_SIP_L4PORT;

    l2_detect.http_link1_detect_enable = 1;
    l2_detect.http_link1_lspid_valid   = 0;
    l2_detect.http_link1_lspid         = 0;
    l2_detect.http_link1_l4_type       = AAL_L2_SPECPKT_HTTPLINK_TYPE_BOTH_TCP_UDP;
    l2_detect.http_link1_l4_port_type  = AAL_L2_SPECPKT_HTTPLINK_L4_PORT_TYPE_DST;
    l2_detect.http_link1_l4_port       = __L2FE_SIP_L4PORT;

    ret = aal_l2_specpkt_detect_set(device_id, l2_detect_mask, &l2_detect);

    return ret;

}

#ifdef __KERNEL__

EXPORT_SYMBOL(aal_l2_specpkt_ctrl_get);
EXPORT_SYMBOL(aal_l2_specpkt_detect_set);
EXPORT_SYMBOL(aal_l2_specpkt_detect_get);
EXPORT_SYMBOL(aal_l2_specpkt_ethtype_encode_set);
EXPORT_SYMBOL(aal_l2_specpkt_current_capture_get);
EXPORT_SYMBOL(aal_l2_specpkt_ethtype_encode_get);
EXPORT_SYMBOL(aal_l2_specpkt_ctrl_set);
EXPORT_SYMBOL(aal_l2_specpkt_init);

#endif


