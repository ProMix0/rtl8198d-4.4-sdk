/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 68395 $
 * $Date: 2016-05-27 16:38:35 +0800 (Fri, 27 May 2016) $
 *
 * Purpose : Definition of Port Bandwidth Control and Storm Control API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Configuration of Ingress Port Bandwidth Control [Ingress Rate Limit]
 *           (2) Configuration of Egress  Port Bandwidth Control [Egress  Rate Limit]
 *           (3) Configuration of Storm Control
 *           (3) Configuration of meter
 *
 */

#ifndef __RT_RATE_H__
#define __RT_RATE_H__


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/rt/rt_port.h>


/*
 * Symbol Definition
 */
#if defined(CONFIG_SDK_CA8279)
#include <hal/chipdef/ca8279/ca8279_def.h>
#define RT_RATE_HW_IDX_L2_METER_MAX_NUM 256
#define RT_RATE_HW_IDX_L2_METER_RATE_MAX CA8279_METER_RATE_MAX
#define RT_RATE_HW_IDX_L2_METER_BURST_MAX CA8279_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_L3_METER_MAX_NUM 512
#define RT_RATE_HW_IDX_L3_METER_RATE_MAX CA8279_METER_RATE_MAX
#define RT_RATE_HW_IDX_L3_METER_BURST_MAX CA8279_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_SW_METER_MAX_NUM 32
#define RT_RATE_HW_IDX_SW_METER_RATE_MAX CA8279_METER_RATE_MAX
#define RT_RATE_HW_IDX_SW_METER_BURST_MAX 0

//224~255(32 entries) if dor ddos
#ifdef CONFIG_RG_G3_L2FE_POL_OFFLOAD
#define RT_RATE_HW_IDX_L2_RSV_FOR_DDOS 32
#define RT_RATE_HW_IDX_L2_RSV_FOR_HOST 96 //CONFIG_RG_G3_L2FE_POL_OFFLOAD 128~223(96 entries=32 rate limit+32 tx+32 rx) for host policer
#define RT_RATE_L2_ALLOC_NUM (RT_RATE_HW_IDX_L2_METER_MAX_NUM-RT_RATE_HW_IDX_L2_RSV_FOR_DDOS-RT_RATE_HW_IDX_L2_RSV_FOR_HOST)
#else
#define RT_RATE_HW_IDX_L2_RSV_FOR_DDOS 32
#define RT_RATE_L2_ALLOC_NUM (RT_RATE_HW_IDX_L2_METER_MAX_NUM-RT_RATE_HW_IDX_L2_RSV_FOR_DDOS)
#endif

#define RT_RATE_HW_IGR_RATE_MAX CA8279_INGRESS_RATE_MAX
#define RT_RATE_HW_EGR_RATE_MAX CA8279_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_RATE_MAX CA8279_PON_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_PIR_CIR_RATE_MAX CA8279_PON_PIR_CIR_RATE_MAX 

#elif defined(CONFIG_SDK_CA8277B)
#include <hal/chipdef/ca8277b/ca8277b_def.h>
#define RT_RATE_HW_IDX_L2_METER_MAX_NUM 256
#define RT_RATE_HW_IDX_L2_METER_RATE_MAX CA8277B_METER_RATE_MAX
#define RT_RATE_HW_IDX_L2_METER_BURST_MAX CA8277B_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_L3_METER_MAX_NUM 512
#define RT_RATE_HW_IDX_L3_METER_RATE_MAX CA8277B_METER_RATE_MAX
#define RT_RATE_HW_IDX_L3_METER_BURST_MAX CA8277B_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_SW_METER_MAX_NUM 32
#define RT_RATE_HW_IDX_SW_METER_RATE_MAX CA8277B_METER_RATE_MAX
#define RT_RATE_HW_IDX_SW_METER_BURST_MAX 0
//224~255(32 entries) if dor ddos
#define RT_RATE_HW_IDX_L2_RSV_FOR_DDOS 32
#define RT_RATE_L2_ALLOC_NUM (RT_RATE_HW_IDX_L2_METER_MAX_NUM-RT_RATE_HW_IDX_L2_RSV_FOR_DDOS)

#define RT_RATE_HW_IGR_RATE_MAX CA8277B_INGRESS_RATE_MAX
#define RT_RATE_HW_EGR_RATE_MAX CA8277B_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_RATE_MAX CA8277B_PON_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_PIR_CIR_RATE_MAX CA8277B_PON_PIR_CIR_RATE_MAX

#elif defined(CONFIG_SDK_RTL8198F)
#include <hal/chipdef/rtl8198f/rtl8198f_def.h>
#define RT_RATE_HW_IDX_L2_METER_MAX_NUM 256
#define RT_RATE_HW_IDX_L2_METER_RATE_MAX RTL8198F_METER_RATE_MAX
#define RT_RATE_HW_IDX_L2_METER_BURST_MAX RTL8198F_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_L3_METER_MAX_NUM 128
#define RT_RATE_HW_IDX_L3_METER_RATE_MAX RTL8198F_METER_RATE_MAX
#define RT_RATE_HW_IDX_L3_METER_BURST_MAX RTL8198F_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_SW_METER_MAX_NUM 32
#define RT_RATE_HW_IDX_SW_METER_RATE_MAX RTL8198F_METER_RATE_MAX
#define RT_RATE_HW_IDX_SW_METER_BURST_MAX 0
//224~255(32 entries) if dor ddos
#define RT_RATE_HW_IDX_L2_RSV_FOR_DDOS 32
#define RT_RATE_L2_ALLOC_NUM (RT_RATE_HW_IDX_L2_METER_MAX_NUM-RT_RATE_HW_IDX_L2_RSV_FOR_DDOS)

#define RT_RATE_HW_IGR_RATE_MAX RTL8198F_INGRESS_RATE_MAX
#define RT_RATE_HW_EGR_RATE_MAX RTL8198F_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_RATE_MAX RTL8198F_PON_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_PIR_CIR_RATE_MAX RTL8198F_PON_PIR_CIR_RATE_MAX

#elif defined(CONFIG_SDK_RTL9607C) || defined(CONFIG_SDK_RTL9603CVD)
#include <hal/chipdef/rtl9607c/rtl9607c_def.h>
#define RT_RATE_HW_IDX_L2_METER_MAX_NUM 48
#define RT_RATE_HW_IDX_L2_METER_RATE_MAX RTL9607C_METER_RATE_MAX
#define RT_RATE_HW_IDX_L2_METER_BURST_MAX RTL9607C_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_L3_METER_MAX_NUM 32
#define RT_RATE_HW_IDX_L3_METER_RATE_MAX RTL9607C_METER_RATE_MAX
#define RT_RATE_HW_IDX_L3_METER_BURST_MAX RTL9607C_METER_BUCKETSIZE_MAX
#define RT_RATE_HW_IDX_SW_METER_MAX_NUM 32
#define RT_RATE_HW_IDX_SW_METER_RATE_MAX RTL9607C_METER_RATE_MAX
#define RT_RATE_HW_IDX_SW_METER_BURST_MAX 0

#define RT_RATE_L2_ALLOC_NUM RT_RATE_HW_IDX_L2_METER_MAX_NUM

#define RT_RATE_HW_IGR_RATE_MAX RTL9607C_INGRESS_RATE_MAX
#define RT_RATE_HW_EGR_RATE_MAX RTL9607C_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_RATE_MAX RTL9607C_PON_EGRESS_RATE_MAX
#define RT_RATE_PON_EGR_PIR_CIR_RATE_MAX RTL9607C_PON_PIR_CIR_RATE_MAX
#endif

#define RT_RATE_V_IDX_MAX_NUM RT_RATE_HW_IDX_L2_METER_MAX_NUM + RT_RATE_HW_IDX_L3_METER_MAX_NUM + RT_RATE_HW_IDX_SW_METER_MAX_NUM

#define RT_MAX_RATE 0xFFFFFFFF


/*
 * Data Declaration
 */


/* Group type of storm control */
typedef enum rt_rate_storm_group_e
{
    RT_STORM_GROUP_UNKNOWN_UNICAST = 0,
    RT_STORM_GROUP_UNKNOWN_MULTICAST,
    RT_STORM_GROUP_MULTICAST,
    RT_STORM_GROUP_BROADCAST,
    RT_STORM_GROUP_DHCP,
    RT_STORM_GROUP_ARP,
    RT_STORM_GROUP_IGMP_MLD,
    RT_STORM_GROUP_END
} rt_rate_storm_group_t;

typedef struct rt_rate_storm_group_ctrl_s
{
    uint8 unknown_unicast_enable;
    uint8 unknown_multicast_enable;
    uint8 multicast_enable;
    uint8 broadcast_enable;
    uint8 dhcp_enable;
    uint8 arp_enable;
    uint8 igmp_mld_enable;
} rt_rate_storm_group_ctrl_t;

typedef enum rt_rate_meter_type_e
{
    RT_METER_TYPE_STORM,
    RT_METER_TYPE_HOST,
    RT_METER_TYPE_FLOW,
    RT_METER_TYPE_ACL,
    RT_METER_TYPE_SW,
    RT_METER_TYPE_END
} rt_rate_meter_type_t;

typedef enum rt_rate_meter_hw_type_e
{
    RT_METER_HW_TYPE_L2,
    RT_METER_HW_TYPE_L3,
    RT_METER_HW_TYPE_SW,
    RT_METER_HW_TYPE_END
} rt_rate_meter_hw_type_t;

typedef struct rt_rate_meter_mapping_s
{
    rt_rate_meter_type_t type;
    rt_rate_meter_hw_type_t hw_type;
    uint32 hw_index;
    uint32 hw_index_ext;
} rt_rate_meter_mapping_t;

typedef enum rt_rate_meter_mode_e
{
    RT_METER_MODE_BIT_RATE = 0,
    RT_METER_MODE_PACKET_RATE,
    RT_METER_MODE_END
} rt_rate_meter_mode_t;

/*
 * Function Declaration
 */

/* Function Name:
 *      rt_rate_init
 * Description:
 *      Initial the rate module.
 * Input:
 *      None.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None.
 */
extern int32
rt_rate_init(void);

/* Module Name    : Rate                                            */
/* Sub-module Name: Configuration of ingress port bandwidth control */

/* Function Name:
 *      rt_rate_portIgrBandwidthCtrlRate_get
 * Description:
 *      Get the ingress bandwidth control rate.
 * Input:
 *      port  - port id
 * Output:
 *      pRate - ingress bandwidth control rate
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in apollo is 8Kbps.
 */
extern int32
rt_rate_portIgrBandwidthCtrlRate_get(rt_port_t port, uint32 *pRate);

/* Function Name:
 *      rt_rate_portIgrBandwidthCtrlRate_set
 * Description:
 *      Set the ingress bandwidth control rate.
 * Input:
 *      port - port id
 *      rate - ingress bandwidth control rate
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in apollo is 8Kbps.
 */
extern int32
rt_rate_portIgrBandwidthCtrlRate_set(rt_port_t port, uint32 rate);


/* Module Name    : Rate                                           */
/* Sub-module Name: Configuration of egress port bandwidth control */

/* Function Name:
 *      rt_rate_portEgrBandwidthCtrlRate_get
 * Description:
 *      Get the egress bandwidth control rate.
 * Input:
 *      port  - port id
 * Output:
 *      pRate - egress bandwidth control rate
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
extern int32
rt_rate_portEgrBandwidthCtrlRate_get(rt_port_t port, uint32 *pRate);

/* Function Name:
 *      rt_rate_portEgrBandwidthCtrlRate_set
 * Description:
 *      Set the egress bandwidth control rate.
 * Input:
 *      port - port id
 *      rate - egress bandwidth control rate
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
extern int32
rt_rate_portEgrBandwidthCtrlRate_set(rt_port_t port, uint32 rate);

/* Module Name    : Rate                           */
/* Sub-module Name: Configuration of storm control */


/* Function Name:
 *      rt_rate_stormControlRate_get
 * Description:
 *      Get the storm control meter index.
 * Input:
 *      port       - port id
 *      stormType  - storm group type
 * Output:
 *      pIndex     - storm control meter index.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_ENTRY_NOTFOUND    - The global strom group is not enable for this group
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *    The storm group types are as following:
 *    - STORM_GROUP_UNKNOWN_UNICAST
 *    - STORM_GROUP_UNKNOWN_MULTICAST
 *    - STORM_GROUP_MULTICAST
 *    - STORM_GROUP_BROADCAST
 *    - STORM_GROUP_DHCP
 *    - STORM_GROUP_ARP
 *    - STORM_GROUP_IGMP_MLD
 *    - Before call this API must make sure the global strom gruop for given group is enabled,
 *      otherwise this API will return RT_ERR_ENTRY_NOTFOUND
 */
extern int32
rt_rate_stormControlMeterIdx_get(
    rt_port_t              port,
    rt_rate_storm_group_t  stormType,
    uint32                 *pIndex);

/* Function Name:
 *      rt_rate_stormControlMeterIdx_set
 * Description:
 *      Set the storm control meter index.
 * Input:
 *      port       - port id
 *      storm_type - storm group type
 *      index      - storm control meter index.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_ENTRY_NOTFOUND    - The global strom group is not enable for this group
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE    - Invalid input bandwidth
 * Note:
 *    The storm group types are as following:
 *    - STORM_GROUP_UNKNOWN_UNICAST
 *    - STORM_GROUP_UNKNOWN_MULTICAST
 *    - STORM_GROUP_MULTICAST
 *    - STORM_GROUP_BROADCAST
 *    - STORM_GROUP_DHCP
 *    - STORM_GROUP_ARP
 *    - STORM_GROUP_IGMP_MLD
 *    - Before call this API must make sure the global strom gruop for given group is enabled,
 *      otherwise this API will return RT_ERR_ENTRY_NOTFOUND
 */
extern int32
rt_rate_stormControlMeterIdx_set(
    rt_port_t              port,
    rt_rate_storm_group_t  stormType,
    uint32                 index);

/* Function Name:
 *      rt_rate_stormControlEnable_get
 * Description:
 *      Get enable status of storm control on specified port.
 * Input:
 *      port       - port id
 *      stormType  - storm group type
 * Output:
 *      pEnable    - pointer to enable status of storm control
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_PORT_ID           - invalid port id
 *      RT_ERR_SFC_UNKNOWN_GROUP - Unknown storm group
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 * Note:
 *    The storm group types are as following:
 *    - STORM_GROUP_UNKNOWN_UNICAST
 *    - STORM_GROUP_UNKNOWN_MULTICAST
 *    - STORM_GROUP_MULTICAST
 *    - STORM_GROUP_BROADCAST
 *    - STORM_GROUP_DHCP
 *    - STORM_GROUP_ARP
 *    - STORM_GROUP_IGMP_MLD
 *    - When global strom gruop for given strom type is disabled,
 *      API will return DISABLED
 */
extern int32
rt_rate_stormControlPortEnable_get(
    rt_port_t              port,
    rt_rate_storm_group_t  stormType,
    rt_enable_t            *pEnable);

/* Function Name:
 *      rt_rate_stormControlPortEnable_set
 * Description:
 *      Set enable status of storm control on specified port.
 * Input:
 *      port       - port id
 *      stormType  - storm group type
 *      enable     - enable status of storm control
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_PORT_ID           - invalid port id
 *      RT_ERR_SFC_UNKNOWN_GROUP - Unknown storm group
 *      RT_ERR_ENTRY_NOTFOUND    - The global strom group is not enable for this group
 *      RT_ERR_INPUT             - invalid input parameter
 * Note:
 *    The storm group types are as following:
 *    - STORM_GROUP_UNKNOWN_UNICAST
 *    - STORM_GROUP_UNKNOWN_MULTICAST
 *    - STORM_GROUP_MULTICAST
 *    - STORM_GROUP_BROADCAST
 *    - STORM_GROUP_DHCP
 *    - STORM_GROUP_ARP
 *    - STORM_GROUP_IGMP_MLD
 *    - Before call this API must make sure the global strom gruop for given group is enabled,
 *      otherwise this API will return RT_ERR_ENTRY_NOTFOUND
 */
extern int32
rt_rate_stormControlPortEnable_set(
    rt_port_t              port,
    rt_rate_storm_group_t  stormType,
    rt_enable_t            enable);

/* Function Name:
 *      rt_rate_shareMeterType_add
 * Description:
 *      Add share meter type configuration
 * Input:
 *      type        - shared meter type

 * Output:
 *      index       - shared meter index
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      
 */
extern int32
rt_rate_shareMeterType_add (
    rt_rate_meter_type_t type,
    uint32* index);

/* Function Name:
 *      rt_rate_shareMeterType_del
 * Description:
 *      Delete share meter type configuration
 * Input:
 *      index       - shared meter index
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      
 */
extern int32
rt_rate_shareMeterType_del (
    uint32 index);

/* Function Name:
 *      rt_rate_shareMeterType_get
 * Description:
 *      Get share meter type configuration
 * Input:
 *      index       - shared meter index

 * Output:
 *      pType       - shared meter type
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      
 */
extern int32
rt_rate_shareMeterType_get (
    uint32 index,
    rt_rate_meter_type_t *pType);

/* Function Name:
 *      rt_rate_shareMeterRate_set
 * Description:
 *      Set meter rate configuration
 * Input:
 *      index       - shared meter index
 *      rate        - rate of share meter
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      The API can set shared meter rate and ifg include for each meter.
 *      The rate unit is 1 kbps and the range is from 8k to 1048568k.
 *      The granularity of rate is 8 kbps. The ifg_include parameter is used
 *      for rate calculation with/without inter-frame-gap and preamble.
 */
extern int32
rt_rate_shareMeterRate_set (
    uint32 index,
    uint32 rate);

/* Function Name:
 *      rt_rate_shareMeterRate_get
 * Description:
 *      Get meter configuration
 * Input:
 *      index        - shared meter index
 * Output:
 *      pRate        - pointer of rate of share meter
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 * Note:
 *      The API can get shared meter rate and ifg include for each meter.
 *      The rate unit is 1 kbps and the granularity of rate is 8 kbps.
 *      The ifg_include parameter is used for rate calculation with/without inter-frame-gap and preamble
 */
extern int32
rt_rate_shareMeterRate_get (
    uint32  index,
    uint32* pRate);

/* Function Name:
 *      rt_rate_shareMeterMappingHw_get
 * Description:
 *      Get share meter hw index mapping configuration
 * Input:
 *      v_index     - shared meter index

 * Output:
 *      pMeterMap   - shared meter mapping
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      
 */
extern int32
rt_rate_shareMeterMappingHw_get (
    uint32 v_index,
    rt_rate_meter_mapping_t* pMeterMap);

/* Function Name:
 *      rt_rate_shareMeterMappingVirtual_get
 * Description:
 *      Get share meter virtual index mapping configuration
 * Input:
 *      pMeterMap   - shared meter mapping

 * Output:
 *      pSw_index   - shared meter index
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 *      RT_ERR_RATE             - Invalid rate
 *      RT_ERR_INPUT            - Invalid input parameters
 * Note:
 *      
 */
extern int32
rt_rate_shareMeterMappingVirtual_get (
    rt_rate_meter_mapping_t meterMap,
    uint32* pV_index);

/* Function Name:
 *      rt_rate_shareMeterBucket_set
 * Description:
 *      Set meter Bucket Size
 * Input:
 *      index        - shared meter index
 *      bucketSize   - Bucket Size
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_INPUT            - Error Input
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 * Note:
 *      The API can set shared meter bucket size.
 */
extern int32
rt_rate_shareMeterBucket_set(uint32 index, uint32 bucketSize);


/* Function Name:
 *      rt_rate_shareMeterBucket_get
 * Description:
 *      Get meter Bucket Size
 * Input:
 *      index        - shared meter index
 * Output:
 *      pBucketSize - Bucket Size
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_FILTER_METER_ID  - Invalid meter
 * Note:
 *      The API can get shared meter bucket size.
 */
extern int32
rt_rate_shareMeterBucket_get(uint32 index, uint32 *pBucketSize);

#endif /* __RT_RATE_H__ */
