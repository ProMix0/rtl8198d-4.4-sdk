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
 * Purpose : Definition of Mirror API
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Port-based mirror
 *
 */


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_SDK_CA8277B) 
#include <rt/include/rt_ext_mapper.h>
#endif

/*
 * Symbol Definition
 */
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_SDK_CA8277B) 
static rt_ext_mapper_t *rt_fc_ext_mapper = NULL;
#endif

/*
 * Macro Declaration
 */

/*
 * Function Declaration
 */

/* Module Name : Mirror */

/* Function Name:
 *      rt_mirror_init
 * Description:
 *      Initialize the mirroring database.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      Must initialize Mirror module before calling any Mirror APIs.
 */
int32
rt_mirror_init(void)
{
    int32   ret =RT_ERR_OK;

    /* function body */
    if (NULL == RT_MAPPER->mirror_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->mirror_init();

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_SDK_CA8277B) 
    if(ret ==RT_ERR_OK)
    {
        //notify L3FE for disabling Egress Mirror
        if(NULL == rt_fc_ext_mapper)
        {
            if((rt_fc_ext_mapper = rt_ext_mapper_get()) == NULL)
            {
                ret = RT_ERR_DRIVER_NOT_FOUND;
            }
        }
    
        if((ret == RT_ERR_OK) && (rt_fc_ext_mapper->internal_mirror_port_set != NULL))
            ret = rt_fc_ext_mapper->internal_mirror_port_set(0, 0, 0);
        else
            ret =  RT_ERR_DRIVER_NOT_FOUND;
    }
#endif

    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_mirror_init */

/* Module Name    : Mirror            */
/* Sub-module Name: Port-based mirror */

/* Function Name:
 *      rt_mirror_portBased_set
 * Description:
 *      Set port mirror function.
 * Input:
 *      mirroringPort         - Monitor port.
 *      pMirroredRxPortmask   - Rx mirror port mask.
 *      pMirroredTxPortmask   - Tx mirror port mask.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_PORT_ID      - Invalid port number
 *      RT_ERR_PORT_MASK    - Invalid portmask.
 * Note:
 *      The API is to set mirror function of source port and mirror port.
 *      The mirror port can only be set to one port and the TX and RX mirror ports
 *      should be identical.
 */
int32
rt_mirror_portBased_set(rt_port_t mirroringPort, rt_portmask_t *pMirroredRxPortmask, rt_portmask_t *pMirroredTxPortmask)
{
    int32   ret;
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_SDK_CA8277B) 
    rt_port_t mirroredPort;
#endif

    /* function body */
    if (NULL == RT_MAPPER->mirror_portBased_set)
        return RT_ERR_DRIVER_NOT_FOUND;

    RTK_API_LOCK();
    ret = RT_MAPPER->mirror_portBased_set(mirroringPort, pMirroredRxPortmask, pMirroredTxPortmask);

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE) && defined(CONFIG_SDK_CA8277B) 
    //notify L3FE for enabling Egress Mirror
    if(ret == RT_ERR_OK)
    {
        if(NULL == rt_fc_ext_mapper)
            rt_fc_ext_mapper = rt_ext_mapper_get();

        if((NULL == rt_fc_ext_mapper) || (NULL == rt_fc_ext_mapper->internal_mirror_port_set))
        {
            ret = RT_ERR_DRIVER_NOT_FOUND;
        }
        else if((RTK_PORTMASK_GET_PORT_COUNT(*pMirroredTxPortmask)!=0))
        {
            mirroredPort = RTK_PORTMASK_GET_FIRST_PORT(*pMirroredTxPortmask);
            ret = rt_fc_ext_mapper->internal_mirror_port_set(1, mirroringPort, mirroredPort);
        }
        else
        {  //no Egress Mirror, clear the setting
            ret = rt_fc_ext_mapper->internal_mirror_port_set(0, 0, 0);
        }

        if((ret != RT_ERR_OK) && (NULL != RT_MAPPER->mirror_init))
        {
            RT_MAPPER->mirror_init();  //reset mirror settings
        }
    }
#endif

    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_mirror_portBased_set */

/* Function Name:
 *      rt_mirror_portBased_get
 * Description:
 *      Get port mirror function.
 * Input:
 *      None
 * Output:
 *      pMirroringPort        - Monitor port.
 *      pMirroredRxPortmask   - Rx mirror port mask.
 *      pMirroredTxPortmask   - Tx mirror port mask.
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_INPUT 		- Invalid input parameters.
 * Note:
 *      The API is to get mirror function of source port and mirror port.
 */
int32
rt_mirror_portBased_get(rt_port_t *pMirroringPort, rt_portmask_t *pMirroredRxPortmask, rt_portmask_t *pMirroredTxPortmask)
{
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->mirror_portBased_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->mirror_portBased_get(pMirroringPort, pMirroredRxPortmask, pMirroredTxPortmask);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_mirror_portBased_get */

