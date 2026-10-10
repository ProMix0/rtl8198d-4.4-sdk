/*
 * Copyright (C) 2019 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : Definition of PON MISC API
 *
 * Feature : The file includes the following modules and sub-modules
 *           
 */
#ifndef __RT_PONMISC_H__
#define __RT_PONMISC_H__

/*
 * Include Files
 */

/*
 * Symbol Definition
 */
#define TRANSCEIVER_LEN 24
#ifdef CONFIG_OE_MODULE_I2C_PORT_1
#define TRANSCEIVER_PORT 1
#else
#define TRANSCEIVER_PORT 0
#endif
#define TRANSCEIVER_A0 0x50
#define TRANSCEIVER_A2 0x51

/*
 * Data Declaration
 */
typedef struct rt_ponmisc_sidInfo_s {
    rtk_enable_t enFlag;
    rtk_enable_t dqFlag;
    uint8 dstPort;
    uint8 pri;
    uint16 flowId;
} rt_ponmisc_sidInfo_t;

typedef struct rt_transceiver_data_s{
    uint8 buf[TRANSCEIVER_LEN];
}rt_transceiver_data_t;

/*
 * Macro Declaration
 */
typedef enum{
    RT_GPON_MODE,
    RT_EPON_MODE,
    RT_PONMODE_END,
}rt_ponmisc_ponMode_t;

typedef enum{
    RT_1G25G_SPEED,
    RT_DN10G_SPEED,
    RT_BOTH10G_SPEED,
    RT_PONSPEED_END,
}rt_ponmisc_ponSpeed_t;

typedef enum{
    RT_PON_QUEUE_FIXED_MODE,
    RT_PON_QUEUE_FLOW_MODE,
    RT_PON_QUEUE_MODE_END,
}rt_ponmisc_ponQueueMode_t;

typedef enum rt_transceiver_patameter_type_e{
    RT_TRANSCEIVER_PARA_TYPE_VENDOR_NAME = 0,
    RT_TRANSCEIVER_PARA_TYPE_SN,
    RT_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM,
    RT_TRANSCEIVER_PARA_TYPE_TEMPERATURE,
    RT_TRANSCEIVER_PARA_TYPE_VOLTAGE,
    RT_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT,
    RT_TRANSCEIVER_PARA_TYPE_TX_POWER,
    RT_TRANSCEIVER_PARA_TYPE_RX_POWER,
    RT_TRANSCEIVER_PARA_TYPE_MAX
}rt_transceiver_parameter_type_t;

 /*
 * Function Declaration
 */
/* Function Name:
 *      rt_ponmisc_init
 * Description:
 *      Initialize pon misc interface.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_FEATURE_NOT_SUPPORTED
 * Note:
 *      Must initialize before calling any other APIs.
 */
extern int32
rt_ponmisc_init(void);

/* Function Name:
 *      rt_ponmisc_modeSpeed_get
 * Description:
 *      get pon misc mode speed inforamtion.
 * Input:
 *      None
 * Output:
 *      pPonMode    - current running PON mode
 *      pPonSpeed   - current running PON speed
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      
 */
extern int32
rt_ponmisc_modeSpeed_get(rt_ponmisc_ponMode_t *pPonMode,rt_ponmisc_ponSpeed_t *pPonSpeed);

/* Function Name:
 *      rt_ponmisc_sid_get
 * Description:
 *      get pon misc sid inforamtion.
 * Input:
 *      sid         - stream id
 * Output:
 *      pSidInfo    - corresponding sid infotmation
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT          - Invalid input parameters.
 *      RT_ERR_NULL_POINTER   - input parameter may be null pointer
 *      RT_ERR_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      
 */
extern int32
rt_ponmisc_sid_get(uint16 sid,rt_ponmisc_sidInfo_t *pSidInfo);

/* Function Name:
 *      rt_ponmisc_sid_set
 * Description:
 *      set pon misc sid inforamtion.
 * Input:
 *      sid         - stream id
 *      sidInfo     - corresponding sid infotmation
 * Output:
 *      none
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT        - Invalid input parameters.
 * Note:
 *
 */
extern int32
rt_ponmisc_sid_set(uint16 sid,rt_ponmisc_sidInfo_t sidInfo);

/* Function Name:
*      rt_ponmisc_transceiver_get
* Description:
*      Get transceiver value by the specific type
* Input:
*      type            - the transceiver parameter type
* Output:
*      pData           - the pointer of data for the specific transceiver parameter
* Return:
*      RT_ERR_OK
*      RT_ERR_FAILED
*      RT_ERR_INPUT        - Invalid input parameters.
*      RT_ERR_NULL_POINTER - input parameter may be null pointer
* Note:
*
*/
extern int32
rt_ponmisc_transceiver_get(rt_transceiver_parameter_type_t type, rt_transceiver_data_t *pData);
#endif