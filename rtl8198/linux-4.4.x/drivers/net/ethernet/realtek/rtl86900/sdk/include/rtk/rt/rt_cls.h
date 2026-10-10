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
 
#ifndef __RT_CLS_H__
#define __RT_CLS_H__

/*
 * Include Files
 */
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>

/*
 * Symbol Definition
 */


typedef enum rt_cls_dir_e{
    RT_CLS_DIR_DS,
    RT_CLS_DIR_US
} rt_cls_dir_t;


typedef enum rt_cls_filter_mask_e
{
    RT_CLS_FILTER_FLOWID_BIT       =(1<<0),
    RT_CLS_FILTER_OUTER_TAGIf_BIT  =(1<<1),
    RT_CLS_FILTER_INNER_TAGIf_BIT  =(1<<2),
    RT_CLS_FILTER_OUTER_TPID_BIT   =(1<<3),
    RT_CLS_FILTER_OUTER_VID_BIT    =(1<<4),
    RT_CLS_FILTER_OUTER_PRI_BIT    =(1<<5),
    RT_CLS_FILTER_OUTER_DEI_BIT    =(1<<6),
    RT_CLS_FILTER_INNER_TPID_BIT   =(1<<7),
    RT_CLS_FILTER_INNER_VID_BIT    =(1<<8),
    RT_CLS_FILTER_INNER_PRI_BIT    =(1<<9),
    RT_CLS_FILTER_INNER_DEI_BIT    =(1<<10),
    RT_CLS_FILTER_ETHERTYPE        =(1<<11),
    RT_CLS_FILTER_DSCP_BIT         =(1<<12),
    RT_CLS_FILTER_EGRESS_PORT_BIT  =(1<<13)
}rt_cls_filter_mask_t;

typedef enum rt_cls_tag_action_e
{
    RT_CLS_TAG_TRANSPARENT,
    RT_CLS_TAG_TAGGING,
    RT_CLS_TAG_MODIFY,
    RT_CLS_TAG_REMOVE,
    RT_CLS_TAG_END
}rt_cls_tag_action_t;

typedef enum rt_cls_tpid_action_e{
    RT_CLS_TPID_ASSIGN,
    RT_CLS_TPID_COPY_FROM_1ST_TAG,        //outer
    RT_CLS_TPID_COPY_FROM_2ND_TAG,        //inner
    RT_CLS_TPID_END
}rt_cls_tpid_action_t;

typedef enum rt_cls_vid_action_e
{
    RT_CLS_VID_ASSIGN,
    RT_CLS_VID_COPY_FROM_1ST_TAG,        //outer
    RT_CLS_VID_COPY_FROM_2ND_TAG,        //inner
    RT_CLS_VID_TRANSPARENT,
    RT_CLS_VID_END,
}rt_cls_vid_action_t;

typedef enum rt_cls_pri_action_e{
    RT_CLS_PRI_ASSIGN,
    RT_CLS_PRI_COPY_FROM_1ST_TAG,        //outer
    RT_CLS_PRI_COPY_FROM_2ND_TAG,        //inner
    RT_CLS_PRI_COPY_FROM_DSCP_REMAP,
    RT_CLS_PRI_TRANSPARENT,
    RT_CLS_PRI_END,
}rt_cls_pri_action_t;

typedef enum rt_cls_dei_action_e{
    RT_CLS_DEI_ASSIGN,
    RT_CLS_DEI_COPY_FROM_1ST_TAG,        //outer
    RT_CLS_DEI_COPY_FROM_2ND_TAG,        //inner
    RT_CLS_DEI_END,
}rt_cls_dei_action_t;

typedef struct rt_cls_filter_rule_s
{
    /* filter mask */
    rt_cls_filter_mask_t filterMask;
    /* filter values */
    uint32               flowId;
    uint32               outerTagIf;
    uint32               innerTagIf;
    uint32               outerTagTpid;
    uint32               outerTagVid;
    uint32               outerTagPri;
    uint32               outerTagDei;
    uint32               innerTagTpid;
    uint32               innerTagVid;
    uint32               innerTagPri;
    uint32               innerTagDei;
    uint32               etherType;
    uint8                dscp[8]; /*64 bit 0~63 */
    uint32               egressPortMask;
} rt_cls_filter_rule_t;

typedef struct rt_cls_rule_action_s
{
    rt_cls_tag_action_t     tagAction;
    rt_cls_tpid_action_t    tagTpidAction;
    rt_cls_vid_action_t     tagVidAction;
    rt_cls_pri_action_t     tagPriAction;
    rt_cls_dei_action_t     tagDeiAction;
    int32 assignedTpid;
    int32 assignedVid;
    int32 assignedPri;
    int32 assignedDei;
}rt_cls_rule_action_t;

typedef enum rt_cls_ds_pri_act_e
{
     RT_CLS_DS_PRI_ACT_NOP = 0,
     RT_CLS_DS_PRI_ACT_ASSIGN = 1,
} rt_cls_ds_pri_act_t;


typedef struct rt_cls_rule_s
{
    uint32               index;
    uint32               flowId;
    rt_cls_dir_t         direction;
    int32                ingressPort;
    uint32               ingressPortMask;
    rt_cls_filter_rule_t filterRule;
    rt_cls_rule_action_t outerTagAct;
    rt_cls_rule_action_t innerTagAct;
    rt_cls_ds_pri_act_t  dsPriAct;
    uint32               dsPri;
} rt_cls_rule_t;


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
extern int32 rt_cls_rule_add(rt_cls_rule_t *pClsRule);

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
extern int32 rt_cls_rule_delete(uint32 index);



#endif