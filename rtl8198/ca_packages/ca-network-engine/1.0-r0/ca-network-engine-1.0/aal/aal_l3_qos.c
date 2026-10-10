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
 * aal_l3_qos.c: ASIC Abstraction Layer driver for L3FE QoS
 */
#include "osal_common.h"
#include "aal_common.h"
#include "aal_l3_qos.h"
#include "aal_port.h"

/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/

#define __L3FE_REG_READ    CA_DEV_NI_REG_READ
#define __L3FE_REG_WRITE   CA_DEV_NE_REG_WRITE2


ca_uint __l3_qos_hw_lock = 0;      /* L3 QoS hardware lock for register access */

#define __L3_QOS_HW_LOCK(device_id)    ca_spin_lock(__l3_qos_hw_lock)
#define __L3_QOS_HW_UNLOCK(device_id)  ca_spin_unlock(__l3_qos_hw_lock)

#define __L3_QOS_POL_ID_CHK(pol_id)   AAL_ASSERT_RET((pol_id) <= 511, CA_E_PARAM)
#define __L3_QOS_POL_GRP_CHK(pol_grp) AAL_ASSERT_RET((pol_grp) <= 7, CA_E_PARAM)
#define __L3_TC_CHK(tc)               AAL_ASSERT_RET((tc) <= 0x3F, CA_E_PARAM)
#define __L3_DSCP_CHK(dscp)           AAL_ASSERT_RET((dscp) <= 0x3F, CA_E_PARAM)
#define __L3_COS_CHK(cos)             AAL_ASSERT_RET((cos) <= 0xF, CA_E_PARAM)
#define __L3_QOS_PROFILE_CHK(pro_idx) AAL_ASSERT_RET((pro_idx) < AAL_L3_QOS_PROFILE_NUM, CA_E_PARAM)


/*********************************************************************************/
/*                  Layer 3 QoS AAL implementation                               */
/*********************************************************************************/
ca_status_t aal_l3_qos_dscp_map_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t dscp,
                CA_IN ca_uint8_t profile_idx,
                CA_IN aal_l3_qos_dscp_map_cfg_mask_t mask,
                CA_IN aal_l3_qos_dscp_map_cfg_t * config)
{
    L3FE_GLB_DSCP_MAP_TBL_DATA0_t  data0;
    L3FE_GLB_DSCP_MAP_TBL_DATA1_t  data1;

    AAL_ASSERT_RET(config, CA_E_PARAM);

    __L3_DSCP_CHK(dscp);
    __L3_QOS_PROFILE_CHK(profile_idx);

    if (mask.s.dscp)
        __L3_DSCP_CHK(config->dscp);
    if (mask.s.cos)
        __L3_COS_CHK(config->cos);
    if (mask.s.pol_id)
        __L3_QOS_POL_ID_CHK(config->pol_id);
    if (mask.s.pol_grp_id)
        __L3_QOS_POL_GRP_CHK(config->pol_grp_id);
    if (mask.s.dot1p)
        CA_DOT_1P_CHECK(config->dot1p);

    __L3_QOS_HW_LOCK(device_id);

    CA_DEV_READ_INDIRCT_REG(device_id, dscp, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    data0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0);
    data1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1);

    if (0 == profile_idx) {
        if (mask.s.dot1p)
            data0.bf.dscp_2_pf0_dot1p       = config->dot1p;
        if (mask.s.dscp)
            data0.bf.dscp_2_pf0_dscp        = config->dscp;
        if (mask.s.cos)
            data0.bf.dscp_2_pf0_cos         = config->cos;
        if (mask.s.pol_id)
            data0.bf.dscp_2_pf0_pol_id      = config->pol_id ;
        if (mask.s.pol_grp_id)
            data0.bf.dscp_2_pf0_pol_grp_id  = config->pol_grp_id;
    } else {
        if (mask.s.dot1p)
            data0.bf.dscp_2_pf1_dot1p      = config->dot1p ;
        if (mask.s.dscp) {
            data0.bf.dscp_2_pf1_dscp       = config->dscp & 0xf;
            data1.bf.dscp_2_pf1_dscp       = (config->dscp >> 4) & 0x3;
        }
        if (mask.s.cos)
            data1.bf.dscp_2_pf1_cos        = config->cos        ;
        if (mask.s.pol_id)
            data1.bf.dscp_2_pf1_pol_id     = config->pol_id     ;
        if (mask.s.pol_grp_id)
            data1.bf.dscp_2_pf1_pol_grp_id = config->pol_grp_id ;
    }

    __L3FE_REG_WRITE(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0, data0.wrd);
    __L3FE_REG_WRITE(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1, data1.wrd);

    CA_DEV_WRITE_INDIRCT_REG(device_id, dscp, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    __L3_QOS_HW_UNLOCK(device_id);

    return CA_E_OK;

}

ca_status_t aal_l3_qos_dscp_map_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t dscp,
                CA_IN ca_uint8_t profile_idx,
                CA_OUT aal_l3_qos_dscp_map_cfg_t * config)
{
    L3FE_GLB_DSCP_MAP_TBL_DATA0_t  data0;
    L3FE_GLB_DSCP_MAP_TBL_DATA1_t  data1;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    __L3_DSCP_CHK(dscp);
    __L3_QOS_PROFILE_CHK(profile_idx);

    __L3_QOS_HW_LOCK(device_id);

    CA_DEV_READ_INDIRCT_REG(device_id, dscp, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    data0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0);
    data1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1);

    __L3_QOS_HW_UNLOCK(device_id);

    if (0 == profile_idx) {
        config->dot1p      = data0.bf.dscp_2_pf0_dot1p;
        config->dscp       = data0.bf.dscp_2_pf0_dscp;
        config->cos        = data0.bf.dscp_2_pf0_cos;
        config->pol_id     = data0.bf.dscp_2_pf0_pol_id;
        config->pol_grp_id = data0.bf.dscp_2_pf0_pol_grp_id;
    } else {
        config->dot1p      = data0.bf.dscp_2_pf1_dot1p;
        config->dscp       = (data1.bf.dscp_2_pf1_dscp << 4) | data0.bf.dscp_2_pf1_dscp;
        config->cos        = data1.bf.dscp_2_pf1_cos;
        config->pol_id     = data1.bf.dscp_2_pf1_pol_id;
        config->pol_grp_id = data1.bf.dscp_2_pf1_pol_grp_id;
    }

    return CA_E_OK;

}

ca_status_t aal_l3_qos_tc_map_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t tc,
                CA_IN ca_uint8_t profile_idx,
                CA_IN aal_l3_qos_dscp_map_cfg_mask_t mask,
                CA_IN aal_l3_qos_dscp_map_cfg_t * config)
{
    L3FE_GLB_DSCP_MAP_TBL_DATA0_t  data0;
    L3FE_GLB_DSCP_MAP_TBL_DATA1_t  data1;
    ca_uint32_t  tbl_addr = ((1 << 6) | (tc & 0x3F));

    AAL_ASSERT_RET(config, CA_E_PARAM);
    __L3_TC_CHK(tc);
    __L3_QOS_PROFILE_CHK(profile_idx);

    if (mask.s.dscp)
        __L3_DSCP_CHK(config->dscp);
    if (mask.s.cos)
        __L3_COS_CHK(config->cos);
    if (mask.s.pol_id)
        __L3_QOS_POL_ID_CHK(config->pol_id);
    if (mask.s.pol_grp_id)
        __L3_QOS_POL_GRP_CHK(config->pol_grp_id);

    __L3_QOS_HW_LOCK(device_id);

    CA_DEV_READ_INDIRCT_REG(device_id, tbl_addr, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    data0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0);
    data1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1);

    if (0 == profile_idx) {
        if (mask.s.dot1p)
            data0.bf.dscp_2_pf0_dot1p      = config->dot1p;
        if (mask.s.dscp)
            data0.bf.dscp_2_pf0_dscp       = config->dscp;
        if (mask.s.cos)
            data0.bf.dscp_2_pf0_cos        = config->cos;
        if (mask.s.pol_id)
            data0.bf.dscp_2_pf0_pol_id     = config->pol_id ;
        if (mask.s.pol_grp_id)
            data0.bf.dscp_2_pf0_pol_grp_id = config->pol_grp_id;
    } else {
        if (mask.s.dot1p)
            data0.bf.dscp_2_pf1_dot1p      = config->dot1p ;
        if (mask.s.dscp) {
            data0.bf.dscp_2_pf1_dscp       = config->dscp & 0xf;
            data1.bf.dscp_2_pf1_dscp       = (config->dscp >> 4) & 0x3;
        }
        if (mask.s.cos)
            data1.bf.dscp_2_pf1_cos        = config->cos        ;
        if (mask.s.pol_id)
            data1.bf.dscp_2_pf1_pol_id     = config->pol_id     ;
        if (mask.s.pol_grp_id)
            data1.bf.dscp_2_pf1_pol_grp_id = config->pol_grp_id ;
    }

    __L3FE_REG_WRITE(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0, data0.wrd);
    __L3FE_REG_WRITE(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1, data1.wrd);

    CA_DEV_WRITE_INDIRCT_REG(device_id, tbl_addr, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    __L3_QOS_HW_UNLOCK(device_id);

    return CA_E_OK;

}

ca_status_t aal_l3_qos_tc_map_get(CA_IN ca_device_id_t device_id,
                 CA_IN ca_uint8_t tc,
                 CA_IN ca_uint8_t profile_idx,
                 CA_OUT aal_l3_qos_dscp_map_cfg_t * config)
{
    L3FE_GLB_DSCP_MAP_TBL_DATA0_t  data0;
    L3FE_GLB_DSCP_MAP_TBL_DATA1_t  data1;
    ca_uint32_t  tbl_addr = ((1 << 6) | (tc & 0x3F));

    AAL_ASSERT_RET(config, CA_E_PARAM);
    __L3_TC_CHK(tc);
    __L3_QOS_PROFILE_CHK(profile_idx);

    __L3_QOS_HW_LOCK(device_id);

    CA_DEV_READ_INDIRCT_REG(device_id, tbl_addr, L3FE_GLB_DSCP_MAP_TBL_ACCESS);

    data0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA0);
    data1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DSCP_MAP_TBL_DATA1);

    __L3_QOS_HW_UNLOCK(device_id);

    if (0 == profile_idx) {
        config->dot1p      = data0.bf.dscp_2_pf0_dot1p;
        config->dscp       = data0.bf.dscp_2_pf0_dscp;
        config->cos        = data0.bf.dscp_2_pf0_cos;
        config->pol_id     = data0.bf.dscp_2_pf0_pol_id;
        config->pol_grp_id = data0.bf.dscp_2_pf0_pol_grp_id;
    } else {
        config->dot1p      = data0.bf.dscp_2_pf1_dot1p;
        config->dscp       = (data1.bf.dscp_2_pf1_dscp << 4) | data0.bf.dscp_2_pf1_dscp;
        config->cos        = data1.bf.dscp_2_pf1_cos;
        config->pol_id     = data1.bf.dscp_2_pf1_pol_id;
        config->pol_grp_id = data1.bf.dscp_2_pf1_pol_grp_id;
    }

    return CA_E_OK;

}

ca_status_t aal_l3_qos_dot1p_map_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t dot1p,
                CA_IN ca_uint8_t profile_idx,
                CA_IN aal_l3_qos_dot1p_map_mask_t mask,
                CA_IN aal_l3_qos_dot1p_map_cfg_t * config)
{
    L3FE_GLB_DOT1P_MAPPING_0_0_t  profile0;
    ca_uint32_t                   tbl_addr = (dot1p << 3);

    CA_DOT_1P_CHECK(dot1p);
    __L3_QOS_PROFILE_CHK(profile_idx);
    AAL_ASSERT_RET(config, CA_E_PARAM);

    if (mask.s.cos)
        CA_COS_CHECK(config->cos);
    if (mask.s.dot1p)
        CA_DOT_1P_CHECK(config->dot1p);
    if (mask.s.dscp)
        CA_DSCP_CHECK(config->dscp);
    if (mask.s.tc)
        __L3_TC_CHK(config->tc);
    if (mask.s.pol_grp_id)
        __L3_QOS_POL_GRP_CHK(config->pol_grp_id);
    if (mask.s.pol_id)
        __L3_QOS_POL_ID_CHK(config->pol_id);

    __L3_QOS_HW_LOCK(device_id);

    if (0 == profile_idx)
        profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DOT1P_MAPPING_0_0 + tbl_addr);
    else
        profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DOT1P_MAPPING_1_0 + tbl_addr);

    /*
    [5:0]   dot1p_2_tc;
    [11:6]  dot1p_2_dscp;
    [14:12] dot1p_2_dot1p;
    [18:15] dot1p_2_cos;
    [27:19] dot1p_2_pol_id;
    [30:28] dot1p_2_pol_grp_id;
    */
    if (mask.s.tc)
        profile0.wrd = (profile0.wrd & ~ 0x3F) | (config->tc & 0x3F);
    if (mask.s.dscp)
        profile0.wrd = (profile0.wrd & ~(0x3F <<6 )) | ((config->dscp & 0x3F) << 6);
    if (mask.s.dot1p)
        profile0.wrd = (profile0.wrd & ~(0x7 <<12 )) | ((config->dot1p & 0x7) << 12);
    if (mask.s.cos)
        profile0.wrd = (profile0.wrd & ~(0xF <<15 )) | ((config->cos & 0xF) << 15);
    if (mask.s.pol_id)
        profile0.wrd = (profile0.wrd & ~(0x1FF <<19 )) | ((config->pol_id & 0x1FF) << 19);
    if (mask.s.pol_grp_id)
        profile0.wrd = (profile0.wrd & ~(0x7 <<28 )) | ((config->pol_grp_id & 0X7) << 28);

    if (0 == profile_idx)
        __L3FE_REG_WRITE(device_id, L3FE_GLB_DOT1P_MAPPING_0_0 + tbl_addr, profile0.wrd);
    else
        __L3FE_REG_WRITE(device_id, L3FE_GLB_DOT1P_MAPPING_1_0 + tbl_addr, profile0.wrd);

    __L3_QOS_HW_UNLOCK(device_id);

    return CA_E_OK;

}

ca_status_t aal_l3_qos_dot1p_map_get(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t dot1p,
                CA_IN ca_uint8_t profile_idx,
                CA_OUT aal_l3_qos_dot1p_map_cfg_t * config)
{
    L3FE_GLB_DOT1P_MAPPING_0_0_t  profile0;

    CA_DOT_1P_CHECK(dot1p);
    __L3_QOS_PROFILE_CHK(profile_idx);
    AAL_ASSERT_RET(config, CA_E_PARAM);

    __L3_QOS_HW_LOCK(device_id);

    if (0 == profile_idx) {
        profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DOT1P_MAPPING_0_0 + (dot1p << 3));

    } else {
        profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_DOT1P_MAPPING_1_0 + (dot1p << 3));
    }

    __L3_QOS_HW_UNLOCK(device_id);

    /*
    [5:0]   dot1p_2_tc;
    [11:6]  dot1p_2_dscp;
    [14:12] dot1p_2_dot1p;
    [18:15] dot1p_2_cos;
    [27:19] dot1p_2_pol_id;
    [30:28] dot1p_2_pol_grp_id;
    */
    config->tc         = (profile0.wrd >>  0) & 0x3F;
    config->dscp       = (profile0.wrd >>  6) & 0x3F;
    config->dot1p      = (profile0.wrd >> 12) & 0x7;
    config->cos        = (profile0.wrd >> 15) & 0xF;
    config->pol_id     = (profile0.wrd >> 19) & 0x1FF;
    config->pol_grp_id = (profile0.wrd >> 28) & 0x7;

    return CA_E_OK;

}


ca_status_t aal_l3_qos_cos_map_set(CA_IN ca_device_id_t device_id,
                CA_IN ca_uint8_t cos,
                CA_IN ca_uint8_t profile_idx,
                CA_IN aal_l3_qos_cos_profile_mask_t mask,
                CA_IN aal_l3_qos_cos_profile_t * config)
{
    L3FE_GLB_COS_MAPPING_0_0_t  profile0;
    L3FE_GLB_COS_MAPPING_1_0_t  profile1;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    __L3_COS_CHK(cos);
    __L3_QOS_PROFILE_CHK(profile_idx);

    if (mask.s.dot1p)
        CA_DOT_1P_CHECK(config->dot1p);
    if (mask.s.dscp)
        __L3_DSCP_CHK(config->dscp);
    if (mask.s.tc)
        __L3_TC_CHK(config->tc);
    if (mask.s.pol_id)
        __L3_QOS_POL_ID_CHK(config->pol_id);
    if (mask.s.pol_grp_id)
        __L3_QOS_POL_GRP_CHK(config->pol_grp_id);

    __L3_QOS_HW_LOCK(device_id);

    profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_COS_MAPPING_0_0 + (cos << 3));
    profile1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_COS_MAPPING_1_0 + (cos << 3));

    if (0 == profile_idx) {
        if (mask.s.tc)
            profile1.wrd = (profile1.wrd & ~(0x3F << 12)) | (config->tc << 12);
        if (mask.s.dscp)
            profile1.wrd = (profile1.wrd & ~0x3F) | config->dscp;
        if (mask.s.dot1p)
            profile0.wrd = (profile0.wrd & ~(0x3F << 24)) | (config->dot1p << 24);
        if (mask.s.pol_grp_id)
            profile0.wrd = (profile0.wrd & ~(0x7 << 18)) | (config->pol_grp_id << 18);
        if (mask.s.pol_id)
            profile0.wrd = (profile0.wrd & ~(0x1FF)) | (config->pol_id);
    }
    else {
        if (mask.s.tc)
            profile1.wrd = (profile1.wrd & ~(0x3F << 18)) | (config->tc << 18);
        if (mask.s.dscp)
            profile1.wrd = (profile1.wrd & ~(0x3F << 6)) | (config->dscp << 6);
        if (mask.s.dot1p)
            profile0.wrd = (profile0.wrd & ~(0x7<< 27)) | (config->dot1p << 27);
        if (mask.s.pol_grp_id)
            profile0.wrd = (profile0.wrd & ~(0x7<< 21)) | (config->pol_grp_id << 21);
        if (mask.s.pol_id)
            profile0.wrd = (profile0.wrd & ~(0x1FF<< 9)) | (config->pol_id << 9);
    }

    __L3FE_REG_WRITE(device_id, L3FE_GLB_COS_MAPPING_0_0 + (cos << 3), profile0.wrd);
    __L3FE_REG_WRITE(device_id, L3FE_GLB_COS_MAPPING_1_0 + (cos << 3), profile1.wrd);

    __L3_QOS_HW_UNLOCK(device_id);

    return CA_E_OK;

}


ca_status_t aal_l3_qos_cos_map_get(
                CA_IN  ca_device_id_t device_id,
                CA_IN  ca_uint8_t cos,
                CA_IN  ca_uint8_t profile_idx,
                CA_OUT aal_l3_qos_cos_profile_t * config)
{
    L3FE_GLB_COS_MAPPING_0_0_t  profile0;
    L3FE_GLB_COS_MAPPING_1_0_t  profile1;

    __L3_COS_CHK(cos);
    __L3_QOS_PROFILE_CHK(profile_idx);
    AAL_ASSERT_RET(config, CA_E_PARAM);

    __L3_QOS_HW_LOCK(device_id);

    profile0.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_COS_MAPPING_0_0 + (cos << 3));
    profile1.wrd = __L3FE_REG_READ(device_id, L3FE_GLB_COS_MAPPING_1_0 + (cos << 3));

    __L3_QOS_HW_UNLOCK(device_id);

    if (0 == profile_idx) {
        config->tc         = (profile1.wrd >> 12) & 0x3F;
        config->dscp       = (profile1.wrd >>  0) & 0x3F;;
        config->dot1p      = (profile0.wrd >> 24) & 0x7;;
        config->pol_grp_id = (profile0.wrd >> 18) & 0x7;;
        config->pol_id     = (profile0.wrd >>  0) & 0x1FF;;

    } else {
        config->tc         = (profile1.wrd >> 18) & 0x3F;
        config->dscp       = (profile1.wrd >>  6) & 0x3F;;
        config->dot1p      = (profile0.wrd >> 27) & 0x7;;
        config->pol_grp_id = (profile0.wrd >> 21) & 0x7;;
        config->pol_id     = (profile0.wrd >>  9) & 0x1FF;;
    }


    return CA_E_OK;

}

ca_status_t aal_l3_qos_init(CA_IN ca_device_id_t device_id)
{

    ca_uint8_t dot1p = 0, dscp = 0;
    ca_uint8_t profile_idx = 0;
    aal_l3_qos_dot1p_map_mask_t    dot1p_mask;
    aal_l3_qos_dot1p_map_cfg_t     dot1p_config;
    aal_l3_qos_dscp_map_cfg_mask_t dscp_mask;
    aal_l3_qos_dscp_map_cfg_t      dscp_config;
#if 0
    ca_uint8_t cos = 0;
    aal_l3_qos_cos_profile_mask_t cos_mask;
    aal_l3_qos_cos_profile_t  cos_config;
#endif
    ca_status_t ret = CA_E_OK;

    /* Init hardware lock */
    if (!__l3_qos_hw_lock) {
        if (CA_OSAL_OK != ca_spin_lock_init(&__l3_qos_hw_lock, SPINLOCK_BH)) {
            ca_printf("init __l3_qos_hw_lock fail.\n");
            return CA_E_RESOURCE;
        }
    }

    dot1p_mask.u32 = 0;
    dot1p_mask.s.dot1p = 1;
    dot1p_mask.s.cos = 1;
    memset(&dot1p_config, 0, sizeof(dot1p_config));

    for (dot1p = 0; dot1p <= AAL_L3_QOS_DOT1P_MAX; dot1p++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            dot1p_config.dot1p = dot1p;
            dot1p_config.cos = dot1p;
            ret = aal_l3_qos_dot1p_map_set(device_id, dot1p, profile_idx, dot1p_mask, &dot1p_config);
            if (CA_E_OK != ret) {
                ca_printf("aal_l3_qos_dot1p_map_set %u profile %u :set fail %x\n", dot1p, profile_idx, ret);
                continue;
            }
        }
    }


    dscp_mask.u32 = 0;
    dscp_mask.s.dscp  = 1;
    dscp_mask.s.cos   = 1;
    dscp_mask.s.dot1p = 1;
    memset(&dscp_config, 0, sizeof(dscp_config));

    for (dscp = 0; dscp <= AAL_L3_QOS_DSCP_MAX; dscp++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            dscp_config.cos   = dscp / 8;
            dscp_config.dot1p = dscp / 8;
            dscp_config.dscp  = dscp;

            ret = aal_l3_qos_dscp_map_set(device_id, dscp, profile_idx, dscp_mask, &dscp_config);
            if (CA_E_OK != ret) {
                ca_printf("aal_l3_qos_dscp_map_set%u profile %u :set fail %x\n", dscp, profile_idx, ret);
                continue;
            }
        }
    }

#if 0
    cos_mask.u32 = 0;
    cos_mask.s.dscp = 1;
    cos_mask.s.dot1p = 1;
    cos_mask.s.tc = 1;
    memset(&cos_config, 0, sizeof(cos_config));

    for (cos = 0; cos <= AAL_L3_QOS_COS_MAX; cos++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            cos_config.dot1p = cos;
            cos_config.dscp = cos * 8;
            cos_config.tc = cos * 8;

            ret = aal_l3_qos_cos_map_set(device_id, cos, profile_idx, cos_mask, &cos_config);
            if (CA_E_OK != ret) {
                ca_printf("aal_l3_qos_cos_map_set %u profile %u :set fail %x\n", cos, profile_idx, ret);
                continue;
            }
        }
    }
#endif
    return CA_E_OK;
}

#ifdef G3_L3_QOS_UT

ca_status_t aal_l3_qos_test_get(CA_IN ca_device_id_t device_id, CA_OUT ca_uint32_t *unused_param)
{
    /* fake function for CLI-auto-gen */
    return CA_E_OK;
}

ca_status_t __test_cos_map(ca_device_id_t device_id)
{
    ca_uint8_t cos = 0;
    ca_uint8_t profile_idx = 0;
    aal_l3_qos_cos_profile_mask_t mask;
    aal_l3_qos_cos_profile_t  config, config_get;
    ca_status_t ret = CA_E_OK, test_ret = CA_E_OK;

    mask.u32 = ~0;
    memset(&config, 0, sizeof(aal_l3_qos_cos_profile_t));

    for (cos = 0; cos <= AAL_L3_QOS_COS_MAX; cos++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            memset(&config_get, 0, sizeof(aal_l3_qos_cos_profile_t));
            config.dot1p = (ca_uint8_t)ca_current_time() % 8;
            config.dscp = (ca_uint8_t)ca_current_time() % 64;
            config.tc = (ca_uint8_t)ca_current_time() % 64;
            config.pol_grp_id = (ca_uint16_t)ca_current_time() % 4;
            config.pol_id  = (ca_uint16_t)ca_current_time() % 512;

            ret = aal_l3_qos_cos_map_set(device_id, cos, profile_idx, mask, &config);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test cos %u profile %u :set fail %x\n", cos, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            ret = aal_l3_qos_cos_map_get(device_id, cos, profile_idx, &config_get);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test cos %u profile %u :get fail %x\n", cos, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            if (memcmp(&config, &config_get, sizeof(aal_l3_qos_cos_profile_t))){
                ca_printf("Test cos %u profile %u :compare fail\n", cos, profile_idx);
                test_ret = CA_E_ERROR;
                continue;
            }
        }
    }

    return test_ret;

}

ca_status_t __test_dot1p_map(ca_device_id_t device_id)
{
    ca_uint8_t dot1p = 0;
    ca_uint8_t profile_idx = 0;
    aal_l3_qos_dot1p_map_mask_t mask;
    aal_l3_qos_dot1p_map_cfg_t  config, config_get;
    ca_status_t ret = CA_E_OK, test_ret = CA_E_OK;

    mask.u32 = ~0;
    memset(&config, 0, sizeof(aal_l3_qos_dot1p_map_cfg_t));

    for (dot1p = 0; dot1p <= AAL_L3_QOS_DOT1P_MAX; dot1p++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            memset(&config_get, 0, sizeof(aal_l3_qos_dot1p_map_cfg_t));
            config.cos = (ca_uint8_t)ca_current_time() % 8;
            config.dot1p = (ca_uint8_t)ca_current_time() % 8;
            config.dscp = (ca_uint8_t)ca_current_time() % 64;
            config.tc = (ca_uint8_t)ca_current_time() % 64;
            config.pol_grp_id = (ca_uint16_t)ca_current_time() % 4;
            config.pol_id  = (ca_uint16_t)ca_current_time() % 512;

            ret = aal_l3_qos_dot1p_map_set(device_id, dot1p, profile_idx, mask, &config);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test dot1p %u profile %u :set fail %x\n", dot1p, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            ret = aal_l3_qos_dot1p_map_get(device_id, dot1p, profile_idx, &config_get);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test dot1p %u profile %u :get fail %x\n", dot1p, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            if (memcmp(&config, &config_get, sizeof(aal_l3_qos_dot1p_map_cfg_t))){
                ca_printf("Test dot1p %u profile %u :compare fail\n", dot1p, profile_idx);
                test_ret = CA_E_ERROR;
                continue;
            }
        }
    }

    return test_ret;

}

ca_status_t __test_dscp_map(ca_device_id_t device_id)
{
    ca_uint8_t dscp = 0;
    ca_uint8_t profile_idx = 0;
    aal_l3_qos_dscp_map_cfg_mask_t mask;
    aal_l3_qos_dscp_map_cfg_t  config, config_get;
    ca_status_t ret = CA_E_OK, test_ret = CA_E_OK;

    mask.u32 = ~0;
    memset(&config, 0, sizeof(aal_l3_qos_dscp_map_cfg_t));

    for (dscp = 0; dscp <= AAL_L3_QOS_DSCP_MAX; dscp++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            memset(&config_get, 0, sizeof(aal_l3_qos_dscp_map_cfg_t));
            config.cos = (ca_uint8_t)ca_current_time() % 8;
            config.dot1p = (ca_uint8_t)ca_current_time() % 8;
            config.dscp = (ca_uint8_t)ca_current_time() % 64;
            config.pol_grp_id = (ca_uint16_t)ca_current_time() % 4;
            config.pol_id  = (ca_uint16_t)ca_current_time() % 512;

            ret = aal_l3_qos_dscp_map_set(device_id, dscp, profile_idx, mask, &config);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test dscp %u profile %u :set fail %x\n", dscp, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            ret = aal_l3_qos_dscp_map_get(device_id, dscp, profile_idx, &config_get);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test dscp %u profile %u :get fail %x\n", dscp, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            if (memcmp(&config, &config_get, sizeof(aal_l3_qos_dscp_map_cfg_t))){
                ca_printf("Test dscp %u profile %u :compare fail\n", dscp, profile_idx);
                test_ret = CA_E_ERROR;
                continue;
            }
        }
    }

    return test_ret;

}


ca_status_t __test_tc_map(ca_device_id_t device_id)
{
    ca_uint8_t tc = 0;
    ca_uint8_t profile_idx = 0;
    aal_l3_qos_dscp_map_cfg_mask_t mask;
    aal_l3_qos_dscp_map_cfg_t  config, config_get;
    ca_status_t ret = CA_E_OK, test_ret = CA_E_OK;

    mask.u32 = ~0;
    memset(&config, 0, sizeof(aal_l3_qos_dscp_map_cfg_t));

    for (tc = 0; tc <= AAL_L3_QOS_TC_MAX; tc++) {
        for (profile_idx = 0; profile_idx < AAL_L3_QOS_PROFILE_NUM; profile_idx++) {

            memset(&config_get, 0, sizeof(aal_l3_qos_dscp_map_cfg_t));
            config.cos = (ca_uint8_t)ca_current_time() % 8;
            config.dot1p = (ca_uint8_t)ca_current_time() % 8;
            config.dscp = (ca_uint8_t)ca_current_time() % 64;
            config.pol_grp_id = (ca_uint16_t)ca_current_time() % 4;
            config.pol_id  = (ca_uint16_t)ca_current_time() % 512;

            ret = aal_l3_qos_tc_map_set(device_id, tc, profile_idx, mask, &config);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test tc %u profile %u :set fail %x\n", tc, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            ret = aal_l3_qos_tc_map_get(device_id, tc, profile_idx, &config_get);
            if (CA_E_OK != ret && CA_E_PARAM != ret) {
                ca_printf("Test tc %u profile %u :get fail %x\n", tc, profile_idx, ret);
                test_ret = CA_E_ERROR;
                continue;
            }

            if (CA_E_OK != ret)
                continue;

            if (memcmp(&config, &config_get, sizeof(aal_l3_qos_dscp_map_cfg_t))){
                ca_printf("Test tc %u profile %u :compare fail\n", tc, profile_idx);
                test_ret = CA_E_ERROR;
                continue;
            }
        }
    }

    return test_ret;

}


ca_status_t aal_l3_qos_test_set(CA_IN ca_device_id_t device_id, CA_IN ca_uint32_t unused_param)
{
    ca_status_t  ret = CA_E_OK;

    ca_printf("\r================================\n");

    ret = __test_cos_map(device_id);
    ca_printf("\rTest COS Map %s\n", CA_E_OK == ret ? "PASS" : "FAIL");

    ret = __test_dot1p_map(device_id);
    ca_printf("\rTest DOT1P Map %s\n", CA_E_OK == ret ? "PASS" : "FAIL");

    ret = __test_dscp_map(device_id);
    ca_printf("\rTest DSCP Map %s\n", CA_E_OK == ret ? "PASS" : "FAIL");

    ret = __test_tc_map(device_id);
    ca_printf("\rTest TC Map %s\n", CA_E_OK == ret ? "PASS" : "FAIL");
    ca_printf("\r================================\n\n");

    return ret;
}



#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(aal_l3_qos_tc_map_set);
EXPORT_SYMBOL(aal_l3_qos_tc_map_get);
EXPORT_SYMBOL(aal_l3_qos_cos_map_set);
EXPORT_SYMBOL(aal_l3_qos_cos_map_get);
EXPORT_SYMBOL(aal_l3_qos_dot1p_map_set);
EXPORT_SYMBOL(aal_l3_qos_dot1p_map_get);
EXPORT_SYMBOL(aal_l3_qos_dscp_map_get);
EXPORT_SYMBOL(aal_l3_qos_dscp_map_set);
EXPORT_SYMBOL(aal_l3_qos_init);
#ifdef G3_L3_QOS_UT
EXPORT_SYMBOL(aal_l3_qos_test_set);
EXPORT_SYMBOL(aal_l3_qos_test_get);
#endif

#endif


