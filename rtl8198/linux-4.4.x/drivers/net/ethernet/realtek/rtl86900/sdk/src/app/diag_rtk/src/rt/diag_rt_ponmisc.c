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
 * Purpose : Definition of PON misc API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) PON misc
 *
 */


#include <stdio.h>
#include <string.h>
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <diag_util.h>
#include <diag_str.h>
#include <parser/cparser_priv.h>

#include <rtk/rt/rt_ponmisc.h>

/*
 * rt_ponmisc init
 */
cparser_result_t
cparser_cmd_rt_ponmisc_init(
    cparser_context_t *context)
{
    int32 ret = RT_ERR_FAILED;

    DIAG_UTIL_PARAM_CHK();

    DIAG_UTIL_ERR_CHK(rt_ponmisc_init(), ret);

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_ponmisc_init */

/*
 * rt_ponmisc get mode
 */
cparser_result_t
cparser_cmd_rt_ponmisc_get_mode(
    cparser_context_t *context)
{
    int32 ret = RT_ERR_FAILED;
    rt_ponmisc_ponMode_t ponMode;
    rt_ponmisc_ponSpeed_t ponSpeed;

    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_OUTPUT_INIT();

    DIAG_UTIL_ERR_CHK(rt_ponmisc_modeSpeed_get(&ponMode,&ponSpeed), ret);

    diag_util_mprintf("============================================================\r\n");
    diag_util_mprintf(" PON MODE : ");
    if(ponMode == RT_GPON_MODE)
        diag_util_mprintf("GPON");
    else if(ponMode == RT_EPON_MODE)
        diag_util_mprintf("EPON");
    else
        diag_util_mprintf("Not support");
    diag_util_mprintf("\r\n");
    diag_util_mprintf(" PON SPEED : ");
    if(ponSpeed == RT_1G25G_SPEED)
        diag_util_mprintf("1G or 2.5G");
    else if(ponSpeed == RT_DN10G_SPEED)
        diag_util_mprintf("10G1G");
    else if(ponSpeed == RT_BOTH10G_SPEED)
        diag_util_mprintf("10G10G");
    diag_util_mprintf("\r\n");
    diag_util_mprintf("============================================================\r\n");

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_ponmisc_get_mode */

/*
 * rt_ponmisc get sid <INT:sid>
 */
cparser_result_t
cparser_cmd_rt_ponmisc_get_sid_sid(
    cparser_context_t *context,
    int32_t  *sid_ptr)
{
    int32 ret = RT_ERR_FAILED;
    rt_ponmisc_sidInfo_t sidInfo;

    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_OUTPUT_INIT();

    DIAG_UTIL_ERR_CHK(rt_ponmisc_sid_get(*sid_ptr,&sidInfo), ret);

    if(sidInfo.enFlag >= RTK_ENABLE_END || sidInfo.dqFlag >= RTK_ENABLE_END)
        return CPARSER_OK;

    diag_util_mprintf("sid = %d\n",*sid_ptr);
    diag_util_mprintf("state = %s\n",diagStr_enable[sidInfo.enFlag]);
    diag_util_mprintf("dstPort = %d\n",sidInfo.dstPort);
    diag_util_mprintf("priority = %d\n",sidInfo.pri);
    diag_util_mprintf("flowId = %d\n",sidInfo.flowId);
    diag_util_mprintf("deepqueue = %s\n",diagStr_enable[sidInfo.dqFlag]);

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_ponmisc_get_sid_sid */

/*
 * rt_ponmisc set sid <INT:sid> state ( disable | enable ) dpn <INT:dpn> pri <INT:pri> flowid <INT:flowid> dq ( disable | enable ) */
cparser_result_t
cparser_cmd_rt_ponmisc_set_sid_sid_state_disable_enable_dpn_dpn_pri_pri_flowid_flowid_dq_disable_enable(
    cparser_context_t *context,
    int32_t  *sid_ptr,
    int32_t  *dpn_ptr,
    int32_t  *pri_ptr,
    int32_t  *flowid_ptr)
{
    int32 ret = RT_ERR_FAILED;
    rt_ponmisc_sidInfo_t sidInfo;

    DIAG_UTIL_PARAM_CHK();

    if('e' == TOKEN_CHAR(5, 0))
        sidInfo.enFlag = ENABLED;
    else
        sidInfo.enFlag = DISABLED;

    sidInfo.dstPort = *dpn_ptr;
    sidInfo.pri = *pri_ptr;
    sidInfo.flowId = *flowid_ptr;

    if('e' == TOKEN_CHAR(13, 0))
        sidInfo.dqFlag = ENABLED;
    else
        sidInfo.dqFlag = DISABLED;

    DIAG_UTIL_ERR_CHK(rt_ponmisc_sid_set(*sid_ptr,sidInfo), ret);

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_ponmisc_set_sid_sid_state_disable_enable_dpn_dpn_pri_pri_flowid_flowid_dq_disable_enable */

/*
 * rt_ponmisc get transceiver ( vendor-name | sn | part-number | temperature | voltage | bias-current | tx-power | rx-power )
 */
cparser_result_t
cparser_cmd_rt_ponmisc_get_transceiver_vendor_name_sn_part_number_temperature_voltage_bias_current_tx_power_rx_power(
    cparser_context_t *context)
{
    rt_transceiver_data_t dataCfg, readableCfg;
    int32 ret = RT_ERR_FAILED;
    DIAG_UTIL_PARAM_CHK();
    DIAG_UTIL_OUTPUT_INIT();

    if(0 == osal_strcmp(TOKEN_STR(3), "vendor-name"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_VENDOR_NAME, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_VENDOR_NAME, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Vendor Name: ");
    }else if(0 == osal_strcmp(TOKEN_STR(3), "sn"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_SN, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_SN, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("SN: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "part-number"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Part Number: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "temperature"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_TEMPERATURE, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_TEMPERATURE, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Temperature: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "voltage"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_VOLTAGE, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_VOLTAGE, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Voltage: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "bias-current"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Bias Current: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "tx-power"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_TX_POWER, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_TX_POWER, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Tx Power: ");
    }else if (0 == osal_strcmp(TOKEN_STR(3), "rx-power"))
    {
        DIAG_UTIL_ERR_CHK(rt_ponmisc_transceiver_get(RT_TRANSCEIVER_PARA_TYPE_RX_POWER, &dataCfg), ret);
        _get_data_by_type(RT_TRANSCEIVER_PARA_TYPE_RX_POWER, (rtk_transceiver_data_t *)&dataCfg, (rtk_transceiver_data_t *)&readableCfg);
        diag_util_mprintf("Rx Power: ");
    }

    diag_util_mprintf("%s\n", readableCfg.buf);
    return CPARSER_OK;
}    /* end of cparser_cmd_rt_ponmisc_get_transceiver_vendor_name_sn_part_number_temperature_voltage_bias_current_tx_power_rx_power */

