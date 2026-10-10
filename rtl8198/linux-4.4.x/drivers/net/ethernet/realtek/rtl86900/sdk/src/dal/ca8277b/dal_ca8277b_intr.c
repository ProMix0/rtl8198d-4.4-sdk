/*
 * Copyright (C) 2015 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision:
 * $Date: $
 *
 * Purpose : Definition those public Interrupt APIs and its data type in the SDK.
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Interrupt parameter settings
 */


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/intr.h>
#include <dal/ca8277b/dal_ca8277b.h>
#include <dal/ca8277b/dal_ca8277b_intr.h>
#include <osal/time.h>

#include <ca_event.h>
#include <event_core.h>

#include <cortina-api/include/eth_port.h>
#include <cortina-api/include/port.h>

#ifdef CONFIG_SDK_MODULES
#include <module/intr_bcaster/intr_bcaster.h>
#endif

#include <aal_phy.h>
#include <aal_l2_te.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
static uint32 intr_init = INIT_NOT_COMPLETED;

static rtk_enable_t LUNA_G3_INTR_REG[INTR_TYPE_END];

static ca_uint8_t eth_mdio_addr[CA_PORT_MAX_NUM_ETHERNET+1];

/*
 * Function Declaration
 */

ca_uint32_t ca_rtk_link_change_intr(ca_device_id_t device_id,ca_uint32_t event_id,ca_void_t *event,ca_uint32_t len,ca_void_t *cookie)
{
#ifdef CONFIG_SDK_MODULES
    ca_event_eth_port_link_t* eth_port_link_ptr = (ca_event_eth_port_link_t *)event;
    int32 portId;
    uint32 intrSubType;
    ca_boolean_t pfc_enable=0;
    ca_boolean_t pause_rx=0;
    ca_boolean_t pause_tx=0;
    aal_phy_autoneg_adv_t adv;
    aal_l2_te_policer_cfg_msk_t l2_te_policer_cfg_msk;
    aal_l2_te_policer_cfg_t l2_te_policer_cfg;
    
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    //reference to aal_fdb.c
    portId = PORT_ID(eth_port_link_ptr->port_id);
    if(eth_port_link_ptr->status == CA_PORT_LINK_UP)
    {
        intrSubType = INTR_STATUS_LINKUP;
    }
    if(eth_port_link_ptr->status == CA_PORT_LINK_DOWN)
    {
        intrSubType = INTR_STATUS_LINKDOWN;
    }
    queue_broadcast(MSG_TYPE_LINK_CHANGE, intrSubType, portId, ENABLED);

    /*For src port rate limit*/
    if(eth_port_link_ptr->status == CA_PORT_LINK_UP)
    {
        if(HAL_IS_GE_PORT(portId)||HAL_IS_FE_PORT(portId)) 
        {
            ca_eth_port_pause_get(0,eth_port_link_ptr->port_id,&pfc_enable,&pause_rx,&pause_tx);
            
            memset(&adv,0,sizeof(aal_phy_autoneg_adv_t));
            aal_eth_phy_partner_ability_adv_get(0,eth_mdio_addr[portId],&adv);

            l2_te_policer_cfg_msk.u32 = 0;
            memset(&l2_te_policer_cfg,0,sizeof(aal_l2_te_policer_cfg_t));
            l2_te_policer_cfg_msk.s.bypass_red = 1;

            if(adv.adv_pause == 1 && pause_tx == 1)
            {
                l2_te_policer_cfg.bypass_red = 1;
            }
            else
            {
                l2_te_policer_cfg.bypass_red = 0;
            }
            aal_l2_te_policer_port_cfg_set(0,portId,l2_te_policer_cfg_msk,&l2_te_policer_cfg);
        }
    }
#endif

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    return CA_EVT_OK;
}

ca_uint32_t ca_rtk_onu_state_change_intr(ca_device_id_t device_id,ca_uint32_t event_id,ca_void_t *event,ca_uint32_t len,ca_void_t *cookie)
{
#ifdef CONFIG_SDK_MODULES
    ca_event_gpon_stat_t* gpon_state_ptr = (ca_event_gpon_stat_t *)event;
    int32 portId;
    uint32 intrSubType;
    
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    portId = HAL_GET_PON_PORT();
    if(CA_PON_MODE == ONU_PON_MAC_XGPON1 || CA_PON_MODE == ONU_PON_MAC_XGSPON)
    {
        if(gpon_state_ptr->status == 0)
        {
            intrSubType = 1; //O1
        }
        else if(gpon_state_ptr->status == 1)  //(O23 in 10G GPON)
        {
            intrSubType = 3; //O3  
        }
        else if(gpon_state_ptr->status == 2)
        {
            intrSubType = 4; //O4
        }
        else if(gpon_state_ptr->status == 3)
        {
            intrSubType = 5; //O5
        }
        else if(gpon_state_ptr->status == 4)
        {
            intrSubType = 6; //O6
        }
        else if(gpon_state_ptr->status == 5)
        {
            intrSubType = 7; //O7
        }
        else 
        {
            RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s: ignored state %d",__FUNCTION__, gpon_state_ptr->status);
    
            return CA_EVT_OK;
        }
    }
    else if(CA_PON_MODE == ONU_PON_MAC_GPON)
    {
        if(gpon_state_ptr->status == 0)
        {
            intrSubType = 1; //O1
        }
        else if(gpon_state_ptr->status == 1)
        {
            intrSubType = 2; //O2  
        }
        else if(gpon_state_ptr->status == 2)
        {
            intrSubType = 3; //O3
        }
        else if(gpon_state_ptr->status == 3)
        {
            intrSubType = 4; //O4
        }
        else if(gpon_state_ptr->status == 4)
        {
            intrSubType = 5; //O5
        }
        else if(gpon_state_ptr->status == 5)
        {
            intrSubType = 6; //O6
        }
        else if(gpon_state_ptr->status == 6)
        {
            intrSubType = 7; //O7
        }
        else 
        {
            RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s: ignored state %d",__FUNCTION__, gpon_state_ptr->status);
    
            return CA_EVT_OK;
        }
    }

    queue_broadcast(MSG_TYPE_ONU_STATE, intrSubType, portId, ENABLED);
#endif

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    return CA_EVT_OK;
}

/* Function Name:
 *      dal_ca8277b_intr_init
 * Description:
 *      Initialize interrupt module.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize interrupt module before calling any interrupt APIs.
 */
int32
dal_ca8277b_intr_init(void)
{
    int32 ret = RT_ERR_OK;
    int i;
    ca_device_config_tlv_t scfg_tlv;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    intr_init = INIT_COMPLETED;

    for(i=0;i<INTR_TYPE_END;i++)
    {
        LUNA_G3_INTR_REG[i] = DISABLED;
    }

    if((ret = dal_ca8277b_intr_imr_set(INTR_TYPE_LINK_CHANGE,ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_INTR | MOD_DAL), "");
        intr_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_ca8277b_intr_imr_set(INTR_TYPE_GPON,ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_INTR | MOD_DAL), "");
        intr_init = INIT_NOT_COMPLETED;
        return ret;
    }

    scfg_tlv.type = CA_CFG_ID_PHY_BASE_ADDR;
    scfg_tlv.len = CFG_ID_PHY_BASE_ADDR_LEN;

    if((ret = ca_device_config_tlv_get(0, &scfg_tlv)) != CA_E_OK)
    {
        RT_ERR(ret, (MOD_INTR | MOD_DAL), "");
        intr_init = INIT_NOT_COMPLETED;
        return RT_ERR_FAILED;
    }

    memcpy(eth_mdio_addr,scfg_tlv.value, CFG_ID_PHY_BASE_ADDR_LEN);
    
    return RT_ERR_OK;
} /* end of dal_ca8277b_intr_init */

/* Function Name:
 *      dal_ca8277b_intr_imr_get
 * Description:
 *      Get interrupt mask.
 * Input:
 *      intr            - interrupt type
 *      pEnable      - pointer of return status
 *
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_INPUT
 *      RT_ERR_OUT_OF_RANGE
 * Note:
 *      None.
 */
int32
dal_ca8277b_intr_imr_get(rtk_intr_type_t intr, rtk_enable_t *pEnable)
{
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(intr_init);

    /* parameter check */
    RT_PARAM_CHK((INTR_TYPE_END <= intr), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(intr>INTR_TYPE_END, RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    switch(intr)
    {
        case INTR_TYPE_LINK_CHANGE:
            break;
        case INTR_TYPE_GPON:
            break;
        default:
            RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
            return RT_ERR_INPUT;

    }
    *pEnable = LUNA_G3_INTR_REG[intr];

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_ca8277b_intr_imr_set
 * Description:
 *      Set interrupt mask.
 * Input:
 *      intr          - interrupt type
 *      enable      - interrupt state
 *
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT
 *      RT_ERR_OUT_OF_RANGE
 * Note:
 *     None.
 */
int32
dal_ca8277b_intr_imr_set(rtk_intr_type_t intr, rtk_enable_t enable)
{
    ca_status_t ret=CA_E_OK;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_INTR),"%s",__FUNCTION__);
    RT_INIT_CHK(intr_init);

    /* parameter check */
    RT_PARAM_CHK((INTR_TYPE_END <= intr), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(intr>INTR_TYPE_END, RT_ERR_INPUT);
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_OUT_OF_RANGE);

    switch(intr)
    {
        case INTR_TYPE_LINK_CHANGE:
            if(LUNA_G3_INTR_REG[intr] == DISABLED && enable == ENABLED)
            {
                if((ret = ca_event_register(0,CA_EVENT_ETH_PORT_LINK,ca_rtk_link_change_intr,NULL))!=CA_E_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
                    return RT_ERR_FAILED;
                }
            }
            if(LUNA_G3_INTR_REG[intr] == ENABLED && enable == DISABLED)
            {
                if((ret = ca_event_deregister(0,CA_EVENT_ETH_PORT_LINK,ca_rtk_link_change_intr))!=CA_E_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
                    return RT_ERR_FAILED;
                }
            }
            break;
        case INTR_TYPE_GPON:
            if(LUNA_G3_INTR_REG[intr] == DISABLED && enable == ENABLED)
            {
                if((ret = ca_event_register(0,CA_EVENT_GPON_REG_STAT_CHNG,ca_rtk_onu_state_change_intr,NULL))!=CA_E_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
                    return RT_ERR_FAILED;
                }
            }
            if(LUNA_G3_INTR_REG[intr] == ENABLED && enable == DISABLED)
            {
                if((ret = ca_event_deregister(0,CA_EVENT_GPON_REG_STAT_CHNG, ca_rtk_onu_state_change_intr))!=CA_E_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
                    return RT_ERR_FAILED;
                }
            }
            break;
        default:
            RT_ERR(ret, (MOD_DAL|MOD_INTR), "");
            return RT_ERR_INPUT;

    }
    LUNA_G3_INTR_REG[intr] = enable;

    return RT_ERR_OK;
} /* end of dal_ca8277b_intr_imr_set */

