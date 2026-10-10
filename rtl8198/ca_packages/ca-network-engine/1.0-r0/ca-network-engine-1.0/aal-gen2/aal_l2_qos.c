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
/* aal_l2_qos.c L2FE QoS AAL driver*/

#include <generated/ca_ne_autoconf.h>
#include "osal_common.h"
#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "aal_l2_qos.h"
#include "aal_port.h"

#if defined(CONFIG_ARCH_CORTINA_G3HGU)

#define __L2FE_REG_READ    CA_DEV_NI_REG_READ
#define __L2FE_REG_WRITE   CA_DEV_NE_REG_WRITE2
#define __L2_QOS_TABLE_ACCESS(dev,rw,tbl_name,idx,acc_reg)  do { \
                ca_uint32_t  timeout = 0;                         \
                (acc_reg).wrd        = 0;                         \
                (acc_reg).bf.access  = 1;                         \
                (acc_reg).bf.rbw     = (rw);                      \
                (acc_reg).bf.addr    = (idx);                     \
                (((dev)==0)? CA_NE_REG_WRITE((acc_reg).wrd,tbl_name): CA_HGU_8279_NE_REG_WRITE((acc_reg).wrd,SATURN_##tbl_name));  \
                do {                                                 \
                    (acc_reg).wrd = (((dev)==0)? CA_NE_REG_READ(tbl_name): CA_HGU_8279_NE_REG_READ(SATURN_##tbl_name));\
                    if (timeout++ > __L2_QOS_TABLE_ACCESS_TIMEOUT)       \
                        return CA_E_TIMEOUT;                         \
                }while(0 != (acc_reg).bf.access);                    \
            }while(0)

#else
#define __L2FE_REG_READ(dev,addr)          CA_NE_REG_READ(addr)
#define __L2FE_REG_WRITE(dev,addr,data)    CA_NE_REG_WRITE(data,addr)
#define __L2_QOS_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg)   do {     \
                ca_uint32_t  timeout = 0;                            \
                do {                                                 \
                    (acc_reg).wrd = __L2FE_REG_READ(device_id,tbl_name);\
                    if (timeout++ > __L2_QOS_TABLE_ACCESS_TIMEOUT) {    \
                        __L2_QOS_HW_UNLOCK(device_id);                     \
                        return CA_E_TIMEOUT;                         \
                    }                                                \
                }while(0 != (acc_reg).bf.access);                    \
            }while(0)

#define __L2_QOS_TABLE_ACCESS(device_id,rw,tbl_name,idx,acc_reg)  do { \
                (acc_reg).wrd        = 0;                         \
                (acc_reg).bf.access  = 1;                         \
                (acc_reg).bf.rbw     = (rw);                      \
                (acc_reg).bf.addr    = (idx);                     \
                __L2FE_REG_WRITE(device_id,tbl_name,(acc_reg).wrd);  \
                __L2_QOS_TABLE_ACTION_WAIT(device_id,tbl_name,acc_reg);\
            }while(0)

#endif

/******************************************************************************/
/*                             Local MACROs                                   */
/******************************************************************************/
/* DSCP/TC mapping Table for IPv4 Packet and IPv6 Packet.
 * Indexed by {ipv6_type, wan_src, dscp[5:0]/tc[7:2]}.
 * For example: IPv6 packet from wan port, the extracted tc[7:2] is 6'd1;the mapping table search address is: {1'b1,1'b1,6'b00_0001};
 */
#define __QOS_TC_G3_DIV_SHIFT  2 /*  G3 TC limitation, only high MSB 6 bits is used */

#define __DSCP_TC_ADDR_MAKE(is_ipv6, is_wan_src, dscp_tcbit7to2) \
    (((is_ipv6)<<7) | ((is_wan_src)<<6) | ((dscp_tcbit7to2) & 0x3F))

#define __L2_QOS_DSCP_TBL_ADDR_MAKE(dir, dscp) \
            __DSCP_TC_ADDR_MAKE(0, dir, dscp)

#define __L2_QOS_TC_TBL_ADDR_MAKE(dir, tc)     \
            __DSCP_TC_ADDR_MAKE(1, dir, ((tc)>>__QOS_TC_G3_DIV_SHIFT))

#define __L2_QOS_DOT1P_TBL_ADDR_MAKE(dir, dot1p)  (((dir)<<3)|(dot1p))

#define __TX_DSCP_TC_ADDR_MAKE(is_ipv6, is_wan_src, dscp_tcbit7to2) \
    (((is_ipv6)<<7) | ((!(is_wan_src))<<6) | ((dscp_tcbit7to2) & 0x3F))

#define __L2_QOS_TX_DSCP_TBL_ADDR_MAKE(dir, dscp) \
            __TX_DSCP_TC_ADDR_MAKE(0, (dir), (dscp))

#define __L2_QOS_TX_TC_TBL_ADDR_MAKE(dir, tc)     \
            __TX_DSCP_TC_ADDR_MAKE(1, (dir), ((tc)>>__QOS_TC_G3_DIV_SHIFT))

#define __L2_QOS_TX_DOT1P_TBL_ADDR_MAKE(dir, dot1p)  (((!(dir))<<3)|(dot1p))

#define __L2_QOS_TABLE_ACCESS_TIMEOUT 10000
#define __L2_QOS_TABLE_WRITE_FLAG     1
#define __L2_QOS_TABLE_READ_FLAG      0


#define __L2_QOS_CHK_PORT(port_id)  do { \
                                     if ((port_id) >= AAL_PORT_NUM_LOGICAL)\
                                        return CA_E_PARAM;\
                                 } while(0)

ca_uint __l2_qos_hw_lock;      /* QoS hardware lock for register access */

#define __L2_QOS_HW_LOCK(device_id)    ca_spin_lock(__l2_qos_hw_lock)
#define __L2_QOS_HW_UNLOCK(device_id)  ca_spin_unlock(__l2_qos_hw_lock)

/*********************************************************************************/
/*                  Layer 2 QoS AAL implementation                               */
/*********************************************************************************/
ca_status_t aal_l2_qos_dscp_map_set(CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_qos_direction_t direction,
                 CA_IN ca_uint32_t dscp,
                 CA_IN aal_l2_qos_dscp_map_mask_t mask,
                 CA_IN aal_l2_qos_dscp_map_t * config)
{
#if 1
    L2FE_PLA_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_DSCP_CHECK(dscp);

    __L2_QOS_HW_LOCK(device_id);

/*
 * DSCP/TC mapping Table for IPv4 Packet and IPv6 Packet.
 * Indexed by {ipv6_type, wan_src, dscp[5:0]/tc[7:2]}.
 * For example: IPv6 packet from wan port, the extracted tc[7:2] is 6'd1;
 * the mapping table search address is: {1'b1,1'b1,6'b00_0001};
 */
    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_DSCP_TBL_ADDR_MAKE(direction,dscp), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DSCP_MAP_DATA);

    if (mask.s.mapped_premark)
        tbl_data.bf.mapped_premark = config->mapped_premark;

    if (mask.s.dscp_map_mapped_cos)
        tbl_data.bf.dscp_map_mapped_cos = config->dscp_map_mapped_cos;

    if (mask.s.dscp_map_marked_flag)
        tbl_data.bf.dscp_map_marked_flag = config->dscp_map_marked_flag;

    if (mask.s.dscp_map_mapped_dscp)
        tbl_data.bf.dscp_map_mapped_dscp = config->dscp_map_mapped_dscp;

    __L2FE_REG_WRITE(device_id, L2FE_PLA_DSCP_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_DSCP_TBL_ADDR_MAKE(direction,dscp), tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_dscp_map_get(CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_qos_direction_t direction,
                 CA_IN ca_uint32_t dscp,
                 CA_OUT aal_l2_qos_dscp_map_t * config)
{
#if 1

    L2FE_PLA_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_DSCP_CHECK(dscp);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_DSCP_TBL_ADDR_MAKE(direction,dscp), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DSCP_MAP_DATA);

    __L2_QOS_HW_UNLOCK(device_id);

    config->mapped_premark = tbl_data.bf.mapped_premark;
    config->dscp_map_mapped_cos = tbl_data.bf.dscp_map_mapped_cos;
    config->dscp_map_marked_flag = tbl_data.bf.dscp_map_marked_flag;
    config->dscp_map_mapped_dscp = tbl_data.bf.dscp_map_mapped_dscp;
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_tc_map_set(CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_qos_direction_t direction,
                 CA_IN ca_uint32_t tc, /* TC high 6 bits is used */
                 CA_IN aal_l2_qos_tc_map_mask_t mask,
                 CA_IN aal_l2_qos_tc_map_t * config)
{
#if 1
    L2FE_PLA_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_TRAFFIC_CLASS_CHECK(tc);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_TC_TBL_ADDR_MAKE(direction,tc), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DSCP_MAP_DATA);

    if (mask.s.mapped_premark)
        tbl_data.bf.mapped_premark = config->mapped_premark & 0x1;

    if (mask.s.tc_map_mapped_cos)
        tbl_data.bf.dscp_map_mapped_cos = config->tc_map_mapped_cos & 0x7;

    if (mask.s.tc_map_marked_flag)
        tbl_data.bf.dscp_map_marked_flag = config->tc_map_marked_flag & 0x1;

    if (mask.s.tc_map_mapped_dscp)
        tbl_data.bf.dscp_map_mapped_dscp = config->tc_map_mapped_dscp & 0x3F;

    __L2FE_REG_WRITE(device_id, L2FE_PLA_DSCP_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_TC_TBL_ADDR_MAKE(direction,tc), tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_tc_map_get(CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_qos_direction_t direction,
                 CA_IN ca_uint32_t tc, /* TC high 6 bits is used */
                 CA_OUT aal_l2_qos_tc_map_t * config)
{
#if 1

    L2FE_PLA_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_TRAFFIC_CLASS_CHECK(tc);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLA_DSCP_MAP_ACCESS,
                 __L2_QOS_TC_TBL_ADDR_MAKE(direction,tc), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DSCP_MAP_DATA);

    __L2_QOS_HW_UNLOCK(device_id);

    config->mapped_premark = tbl_data.bf.mapped_premark;
    config->tc_map_mapped_cos = tbl_data.bf.dscp_map_mapped_cos;
    config->tc_map_marked_flag = tbl_data.bf.dscp_map_marked_flag;
    config->tc_map_mapped_dscp = tbl_data.bf.dscp_map_mapped_dscp;
#endif
    return CA_E_OK;

}

// TODO: wait ASIC to complete TC map table, then add API aal_l2_qos_tc_map_set/get

ca_status_t aal_l2_qos_dot1p_map_set(CA_IN ca_device_id_t device_id,
                  CA_IN aal_l2_qos_direction_t direction,
                  CA_IN ca_uint32_t dot1p,
                  CA_IN aal_l2_qos_dot1p_map_mask_t mask,
                  CA_IN aal_l2_qos_dot1p_map_t * config)
{
#if 1
    L2FE_PLA_DOT1P_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DOT1P_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    CA_DOT_1P_CHECK(dot1p);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
        L2FE_PLA_DOT1P_MAP_ACCESS,
        __L2_QOS_DOT1P_TBL_ADDR_MAKE(direction, dot1p), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DOT1P_MAP_DATA);

    if (mask.s.dot1p_map_mapped_premark)
        tbl_data.bf.dot1p_map_mapped_premark =
            config->dot1p_map_mapped_premark;

    if (mask.s.dot1p_map_mapped_cos)
        tbl_data.bf.dot1p_map_mapped_cos = config->dot1p_map_mapped_cos;

    if (mask.s.dot1p_map_mapped_1p)
        tbl_data.bf.dot1p_map_mapped_1p = config->dot1p_map_mapped_1p;

    __L2FE_REG_WRITE(device_id, L2FE_PLA_DOT1P_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLA_DOT1P_MAP_ACCESS,
                 __L2_QOS_DOT1P_TBL_ADDR_MAKE(direction, dot1p), tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_dot1p_map_get(CA_IN ca_device_id_t device_id,
                  CA_IN aal_l2_qos_direction_t direction,
                  CA_IN ca_uint32_t dot1p,
                  CA_OUT aal_l2_qos_dot1p_map_t * config)
{
#if 1

    L2FE_PLA_DOT1P_MAP_ACCESS_t tbl_acc;
    L2FE_PLA_DOT1P_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_DOT_1P_CHECK(dot1p);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
        L2FE_PLA_DOT1P_MAP_ACCESS,
        __L2_QOS_DOT1P_TBL_ADDR_MAKE(direction, dot1p), tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLA_DOT1P_MAP_DATA);

    __L2_QOS_HW_UNLOCK(device_id);

    config->dot1p_map_mapped_premark = tbl_data.bf.dot1p_map_mapped_premark;
    config->dot1p_map_mapped_cos = tbl_data.bf.dot1p_map_mapped_cos;
    config->dot1p_map_mapped_1p = tbl_data.bf.dot1p_map_mapped_1p;
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_txdscp_map_set(CA_IN ca_device_id_t device_id,
                   CA_IN aal_l2_qos_direction_t direction,
                   CA_IN ca_uint32_t dscp,
                   CA_IN aal_l2_qos_txdscp_map_mask_t mask,
                   CA_IN aal_l2_qos_txdscp_map_t * config)
{
#if 1

    L2FE_PLE_TX_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    CA_DSCP_CHECK(dscp);

    __L2_QOS_HW_LOCK(device_id);
/*
TxDSCP mapping Table for IPv4/IPv6 Packet. Indexed by the resolved
{ip_type, dest_wan, tx_dscp/tc[5:0]}. Ipv4 entries are located at the
low 128 entries. Ipv6 entries are located at the high 128 entries.

*/
    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_DSCP_TBL_ADDR_MAKE(direction, dscp),
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DSCP_MAP_DATA);

    if (mask.s.txdscp_map_mapped_cos)
        tbl_data.bf.txdscp_map_mapped_cos =
            config->txdscp_map_mapped_cos;

    if (mask.s.txdscp_map_mapped_1p)
        tbl_data.bf.txdscp_map_mapped_1p =
            config->txdscp_map_mapped_1p;

    __L2FE_REG_WRITE(device_id, L2FE_PLE_TX_DSCP_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_DSCP_TBL_ADDR_MAKE(direction, dscp),
                 tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_txdscp_map_get(CA_IN ca_device_id_t device_id,
                   CA_IN aal_l2_qos_direction_t direction,
                   CA_IN ca_uint32_t dscp,
                   CA_OUT aal_l2_qos_txdscp_map_t * config)
{
#if 1
    L2FE_PLE_TX_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_DSCP_CHECK(dscp);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_DSCP_TBL_ADDR_MAKE(direction, dscp),
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DSCP_MAP_DATA);

    __L2_QOS_HW_UNLOCK(device_id);

    config->txdscp_map_mapped_cos = tbl_data.bf.txdscp_map_mapped_cos;
    config->txdscp_map_mapped_1p = tbl_data.bf.txdscp_map_mapped_1p;
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_txdot1p_map_set(CA_IN ca_device_id_t device_id,
                    CA_IN aal_l2_qos_direction_t direction,
                    CA_IN ca_uint32_t dot1p,
                    CA_IN aal_l2_qos_txdot1p_map_mask_t mask,
                    CA_IN aal_l2_qos_txdot1p_map_t * config)
{
    L2FE_PLE_TX_DOT1P_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DOT1P_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    CA_DOT_1P_CHECK(dot1p);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DOT1P_MAP_ACCESS,
                 __L2_QOS_TX_DOT1P_TBL_ADDR_MAKE(direction, dot1p),
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DOT1P_MAP_DATA);

    if (mask.s.txdot1p_map_mapped_cos)
        tbl_data.bf.txdot1p_map_mapped_cos =
            config->txdot1p_map_mapped_cos;

#ifdef CONFIG_ARCH_CORTINA_VENUS
    if (mask.s.txdot1p_map_mapped_cos_wan)
        tbl_data.bf.txdot1p_map_mapped_cos_wan = config->txdot1p_map_mapped_cos_wan;
    if (mask.s.gem_id_vld_wan)
        tbl_data.bf.gem_id_vld_wan = config->gem_id_vld_wan;
    if (mask.s.gem_id_wan)
        tbl_data.bf.gem_id_wan = config->gem_id_wan;
#endif

    __L2FE_REG_WRITE(device_id, L2FE_PLE_TX_DOT1P_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLE_TX_DOT1P_MAP_ACCESS,
                 __L2_QOS_TX_DOT1P_TBL_ADDR_MAKE(direction, dot1p),
                 tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
    return CA_E_OK;

}

ca_status_t aal_l2_qos_txdot1p_map_get(CA_IN ca_device_id_t device_id,
                    CA_IN aal_l2_qos_direction_t direction,
                    CA_IN ca_uint32_t dot1p,
                    CA_OUT aal_l2_qos_txdot1p_map_t * config)
{
    L2FE_PLE_TX_DOT1P_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DOT1P_MAP_DATA_t   tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_DOT_1P_CHECK(dot1p);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DOT1P_MAP_ACCESS,
                 __L2_QOS_TX_DOT1P_TBL_ADDR_MAKE(direction, dot1p),
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DOT1P_MAP_DATA);

    __L2_QOS_HW_UNLOCK(device_id);

    config->txdot1p_map_mapped_cos = tbl_data.bf.txdot1p_map_mapped_cos;
#ifdef CONFIG_ARCH_CORTINA_VENUS
    config->txdot1p_map_mapped_cos_wan = tbl_data.bf.txdot1p_map_mapped_cos_wan;
    config->gem_id_vld_wan = tbl_data.bf.gem_id_vld_wan;
    config->gem_id_wan = tbl_data.bf.gem_id_wan;
#endif
    return CA_E_OK;

}

ca_status_t aal_l2_qos_txtc_map_set(CA_IN ca_device_id_t device_id,
                 CA_IN aal_l2_qos_direction_t direction,
                 CA_IN ca_uint32_t tc, /* TC high 6 bits is used */
                 CA_IN aal_l2_qos_txtc_map_mask_t mask,
                 CA_IN aal_l2_qos_txtc_map_t * config)
{
#if 1

    L2FE_PLE_TX_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DSCP_MAP_DATA_t tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    CA_TRAFFIC_CLASS_CHECK(tc);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_TC_TBL_ADDR_MAKE(direction, tc), /* TC entry address */
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DSCP_MAP_DATA);

    if (mask.s.txtc_map_mapped_cos)
        tbl_data.bf.txdscp_map_mapped_cos = config->txtc_map_mapped_cos;

    if (mask.s.txtc_map_mapped_1p)
        tbl_data.bf.txdscp_map_mapped_1p = config->txtc_map_mapped_1p;

    __L2FE_REG_WRITE(device_id, L2FE_PLE_TX_DSCP_MAP_DATA, tbl_data.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_TC_TBL_ADDR_MAKE(direction, tc),
                 tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);
#endif

    return CA_E_OK;

}

ca_status_t aal_l2_qos_txtc_map_get(CA_IN ca_device_id_t device_id,
                 CA_IN  aal_l2_qos_direction_t direction,
                 CA_IN  ca_uint32_t tc, /* TC high 6 bits is used */
                 CA_OUT aal_l2_qos_txtc_map_t * config)
{
#if 1

    L2FE_PLE_TX_DSCP_MAP_ACCESS_t tbl_acc;
    L2FE_PLE_TX_DSCP_MAP_DATA_t   tbl_data;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    CA_TRAFFIC_CLASS_CHECK(tc);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PLE_TX_DSCP_MAP_ACCESS,
                 __L2_QOS_TX_TC_TBL_ADDR_MAKE(direction, tc), /* TC entry address */
                 tbl_acc);

    tbl_data.wrd = __L2FE_REG_READ(device_id, L2FE_PLE_TX_DSCP_MAP_DATA);

    config->txtc_map_mapped_cos = tbl_data.bf.txdscp_map_mapped_cos;
    config->txtc_map_mapped_1p  = tbl_data.bf.txdscp_map_mapped_1p;

    __L2_QOS_HW_UNLOCK(device_id);
#endif

    return CA_E_OK;

}

ca_status_t aal_l2_qos_port_config_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,
                    CA_IN aal_l2_qos_port_config_mask_t mask,
                    CA_IN aal_l2_qos_port_config_t * config)
{
    L2FE_PP_ILPB_DATA1_t reg_data1;
    L2FE_PP_ILPB_DATA0_t reg_data0;
    L2FE_PP_ILPB_ACCESS_t tbl_acc;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    AAL_ASSERT_RET(mask.u32, CA_E_OK);
    __L2_QOS_CHK_PORT(port_id);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PP_ILPB_ACCESS, port_id, tbl_acc);

    reg_data1.wrd = __L2FE_REG_READ(device_id, L2FE_PP_ILPB_DATA1);
    reg_data0.wrd = __L2FE_REG_READ(device_id, L2FE_PP_ILPB_DATA0);

    if (mask.s.cos_mode_sel)
        reg_data0.bf.ilpb_cos_mode_sel = config->cos_mode_sel;

    if (mask.s.dscp_mode_sel)
        reg_data0.bf.ilpb_dscp_mode_sel = config->dscp_mode_sel;

    if (mask.s.dot1p_mode_sel) {
        reg_data1.bf.ilpb_top_802_1p_mode_sel = config->dot1p_mode_sel;
        reg_data0.bf.ilpb_inner_802_1p_mode_sel =
            config->dot1p_mode_sel;
    }

    __L2FE_REG_WRITE(device_id, L2FE_PP_ILPB_DATA0, reg_data0.wrd);
    __L2FE_REG_WRITE(device_id, L2FE_PP_ILPB_DATA1, reg_data1.wrd);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_WRITE_FLAG,
                 L2FE_PP_ILPB_ACCESS, port_id, tbl_acc);

    __L2_QOS_HW_UNLOCK(device_id);

    return CA_E_OK;

}

ca_status_t aal_l2_qos_port_config_get(CA_IN ca_device_id_t device_id,
                    CA_IN ca_port_id_t port_id,
                    CA_OUT aal_l2_qos_port_config_t * config)
{
    L2FE_PP_ILPB_DATA1_t reg_data1;
    L2FE_PP_ILPB_DATA0_t reg_data0;
    L2FE_PP_ILPB_ACCESS_t tbl_acc;

    AAL_ASSERT_RET(config, CA_E_PARAM);
    __L2_QOS_CHK_PORT(port_id);

    __L2_QOS_HW_LOCK(device_id);

    __L2_QOS_TABLE_ACCESS(device_id, __L2_QOS_TABLE_READ_FLAG,
                 L2FE_PP_ILPB_ACCESS, port_id, tbl_acc);

    reg_data1.wrd = __L2FE_REG_READ(device_id, L2FE_PP_ILPB_DATA1);
    reg_data0.wrd = __L2FE_REG_READ(device_id, L2FE_PP_ILPB_DATA0);

    __L2_QOS_HW_UNLOCK(device_id);

    config->cos_mode_sel = reg_data0.bf.ilpb_cos_mode_sel;
    config->dscp_mode_sel = reg_data0.bf.ilpb_dscp_mode_sel;
    config->dot1p_mode_sel = reg_data1.bf.ilpb_top_802_1p_mode_sel;

    return CA_E_OK;

}


ca_status_t aal_l2_qos_keep_original_cos_set(CA_IN ca_device_id_t device_id,
                    CA_IN ca_boolean_t keep)
{
    L2FE_GLB_L2FE_CFG_SPARE_t  l2fe_glb_cfg;

    l2fe_glb_cfg.wrd = __L2FE_REG_READ(device_id, L2FE_GLB_L2FE_CFG_SPARE);
    l2fe_glb_cfg.wrd = (l2fe_glb_cfg.wrd & ~0x2) | ((keep & 0x1) << 1);
    __L2FE_REG_WRITE(device_id, L2FE_GLB_L2FE_CFG_SPARE, l2fe_glb_cfg.wrd);

    return CA_E_OK;
}


ca_status_t aal_l2_qos_keep_original_cos_get(CA_IN ca_device_id_t device_id,
                    CA_OUT ca_boolean_t * keep)
{
    L2FE_GLB_L2FE_CFG_SPARE_t  l2fe_glb_cfg;

    CA_NULL_PTR_CHECK(keep);

    l2fe_glb_cfg.wrd = __L2FE_REG_READ(device_id, L2FE_GLB_L2FE_CFG_SPARE);
    *keep = (l2fe_glb_cfg.wrd >>1 ) & 0x1;

    return CA_E_OK;
}


ca_status_t aal_l2_qos_init(CA_IN ca_device_id_t device_id)
{
    ca_uint32_t temp_idx = 0;
    aal_l2_qos_dscp_map_mask_t mask_dscp_map;
    aal_l2_qos_dscp_map_t dscp_map;
    aal_l2_qos_tc_map_mask_t mask_tc_map;
    aal_l2_qos_tc_map_t tc_map;
    aal_l2_qos_dot1p_map_mask_t mask_dot1p_map;
    aal_l2_qos_dot1p_map_t dot1p_map;
    aal_l2_qos_txdscp_map_mask_t txdscp_map_mask;
    aal_l2_qos_txdscp_map_t txdscp_map;
    aal_l2_qos_txdot1p_map_mask_t txdot1p_map_mask;
    aal_l2_qos_txdot1p_map_t txdot1p_map;
    aal_l2_qos_txtc_map_mask_t txtc_map_mask;
    aal_l2_qos_txtc_map_t txtc_map;
    aal_l2_qos_port_config_mask_t port_config_mask;
    aal_l2_qos_port_config_t port_config;

    if (!__l2_qos_hw_lock) {
        /* Init hardware lock */
        if (CA_OSAL_OK != ca_spin_lock_init(&__l2_qos_hw_lock, SPINLOCK_BH)) {
            ca_printf("init __l2_qos_hw_lock fail.\n");
        }
    }
    /* Init all the QoS tables to be zeros */
    mask_dscp_map.u32 = ~0U;
    memset(&dscp_map, 0, sizeof(dscp_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DSCP_MAX; ++temp_idx) {
        dscp_map.dscp_map_mapped_cos  = temp_idx / 8;
        dscp_map.dscp_map_mapped_dscp = temp_idx;

        aal_l2_qos_dscp_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_dscp_map, &dscp_map);
        aal_l2_qos_dscp_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, mask_dscp_map, &dscp_map);
    }

    mask_tc_map.u32 = ~0U;
    memset(&tc_map, 0, sizeof(tc_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_TC_MAX; ++temp_idx) {
        tc_map.tc_map_mapped_cos  = temp_idx / 32;
        tc_map.tc_map_mapped_dscp = temp_idx >> __QOS_TC_G3_DIV_SHIFT;

        aal_l2_qos_tc_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_tc_map, &tc_map);
        aal_l2_qos_tc_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, mask_tc_map, &tc_map);
    }

    mask_dot1p_map.u32 = ~0U;
    memset(&dot1p_map, 0, sizeof(dot1p_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DOT1P_MAX; ++temp_idx) {

        dot1p_map.dot1p_map_mapped_cos = temp_idx;
        dot1p_map.dot1p_map_mapped_1p  = temp_idx;

        aal_l2_qos_dot1p_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, mask_dot1p_map, &dot1p_map);
        aal_l2_qos_dot1p_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, mask_dot1p_map, &dot1p_map);
    }

    txdscp_map_mask.u32 = ~0U;
    memset(&txdscp_map, 0, sizeof(txdscp_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DSCP_MAX; ++temp_idx) {

        txdscp_map.txdscp_map_mapped_cos = temp_idx / 8;
        txdscp_map.txdscp_map_mapped_1p  = temp_idx / 8;

        aal_l2_qos_txdscp_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txdscp_map_mask, &txdscp_map);
        aal_l2_qos_txdscp_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, txdscp_map_mask, &txdscp_map);

    }

    txdot1p_map_mask.u32 = ~0U;
    memset(&txdot1p_map, 0, sizeof(txdot1p_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_DOT1P_MAX; ++temp_idx) {

        txdot1p_map.txdot1p_map_mapped_cos = temp_idx;

        aal_l2_qos_txdot1p_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txdot1p_map_mask, &txdot1p_map);
        aal_l2_qos_txdot1p_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, txdot1p_map_mask, &txdot1p_map);
    }

    txtc_map_mask.u32 = ~0U;
    memset(&txtc_map, 0, sizeof(txtc_map));
    for (temp_idx = 0; temp_idx <= AAL_L2_QOS_TC_MAX; ++temp_idx) {

        txtc_map.txtc_map_mapped_cos = temp_idx / 32;
        txtc_map.txtc_map_mapped_1p  = temp_idx / 32;

        aal_l2_qos_txtc_map_set(device_id, AAL_L2_QOS_DIR_UPSTREAM,
            temp_idx, txtc_map_mask, &txtc_map);
        aal_l2_qos_txtc_map_set(device_id, AAL_L2_QOS_DIR_DOWNSTREAM,
            temp_idx, txtc_map_mask, &txtc_map);
    }

    port_config_mask.u32 = ~0U;

    for (temp_idx = 0; temp_idx < AAL_PORT_NUM_LOGICAL; ++temp_idx) {

        port_config.cos_mode_sel   = AAL_COS_MODE_MAP_RX_TOP_DOT1P;
        port_config.dot1p_mode_sel = AAL_DOT1P_MODE_MAP_RX_DOT1P;
        port_config.dscp_mode_sel  = AAL_DSCP_MODE_MAP_RX_DSCP;

        aal_l2_qos_port_config_set(device_id, temp_idx,
            port_config_mask, &port_config);

    }

    return CA_E_OK;

}

#ifdef __KERNEL__
EXPORT_SYMBOL(aal_l2_qos_port_config_get);
EXPORT_SYMBOL(aal_l2_qos_tc_map_set);
EXPORT_SYMBOL(aal_l2_qos_port_config_set);
EXPORT_SYMBOL(aal_l2_qos_txdscp_map_set);
EXPORT_SYMBOL(aal_l2_qos_txdot1p_map_set);
EXPORT_SYMBOL(aal_l2_qos_dscp_map_set);
EXPORT_SYMBOL(aal_l2_qos_txdscp_map_get);
EXPORT_SYMBOL(aal_l2_qos_txdot1p_map_get);
EXPORT_SYMBOL(aal_l2_qos_dscp_map_get);
EXPORT_SYMBOL(aal_l2_qos_dot1p_map_get);
EXPORT_SYMBOL(aal_l2_qos_txtc_map_set);
EXPORT_SYMBOL(aal_l2_qos_tc_map_get);
EXPORT_SYMBOL(aal_l2_qos_dot1p_map_set);
EXPORT_SYMBOL(aal_l2_qos_txtc_map_get);
EXPORT_SYMBOL(aal_l2_qos_keep_original_cos_set);
EXPORT_SYMBOL(aal_l2_qos_keep_original_cos_get);

EXPORT_SYMBOL(aal_l2_qos_init);
#endif

