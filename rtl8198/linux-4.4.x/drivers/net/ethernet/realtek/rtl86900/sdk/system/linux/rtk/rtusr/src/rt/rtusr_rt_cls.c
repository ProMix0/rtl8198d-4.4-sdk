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
 * Purpose : Definition of classify rule global API
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) classify rule add/delete
 *
 */
 

/*
 * Include Files
 */
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <osal/lib.h>
#include <rtk/rtusr/include/rtusr_util.h>
#include <rtdrv/rtdrv_netfilter.h>

/*
 * Symbol Definition                                            
 */



/*
 * Function Declaration
 */

/* Module Name    : Classify rule     */
/* Sub-module Name: Add/delete classify rule */

/* Function Name:
 *      rt_cls_rule_add
 * Description:
 *      Add  classify rule and apply to ASIC.
 * Input:
 *      pClsRule - The classify rule configuration
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pClassifyCfg point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32 rt_cls_rule_add(rt_cls_rule_t *pClsRule)
{
    rtdrv_rt_clsCfg_t rt_cls_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pClsRule), RT_ERR_NULL_POINTER);

    memset(&rt_cls_cfg, 0x0, sizeof(rtdrv_rt_clsCfg_t));

    /* function body */
    osal_memcpy(&rt_cls_cfg.clsRule, pClsRule, sizeof(rt_cls_rule_t));
    SETSOCKOPT(RTDRV_RT_CLS_RULE_ADD, &rt_cls_cfg, rtdrv_rt_clsCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_cls_rule_add */

/* Function Name:
 *      rt_cls_rule_delete
 * Description:
 *      Delete  classify rule.
 * Input:
 *      index     - index of classify rule entry.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None.
 */
int32 rt_cls_rule_delete(uint32 index)
{
    rtdrv_rt_clsCfg_t rt_cls_cfg;

    memset(&rt_cls_cfg, 0x0, sizeof(rtdrv_rt_clsCfg_t));
    /* function body */
    osal_memcpy(&rt_cls_cfg.index, &index, sizeof(uint32));
    SETSOCKOPT(RTDRV_RT_CLS_RULE_DELETE, &rt_cls_cfg, rtdrv_rt_clsCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_cls_rule_delete */



