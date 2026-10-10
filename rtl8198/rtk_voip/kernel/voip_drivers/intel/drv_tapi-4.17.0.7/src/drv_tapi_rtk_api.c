/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_rtk.c
   This file contains an example of integration of the TAPI High-Level foir Realtek
						API from Kernel Space 
*/
/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_tapi_rtk_api.h"


/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */


#define KTAPI_TAPI_DRVCTX()\
    IFX_TAPI_DRV_CTX_t* pDrvCtx; \
    pDrvCtx = kTAPI_get_DrvCtx(); \
if (pDrvCtx == IFX_NULL)\
{\
    return IFX_ERROR; \
}


#define KTAPI_TAPI_CHANNEL(dev, chn)\
    TAPI_CHANNEL *pChannel; \
    pChannel = kTAPI_get_TapiChannel(dev, chn); \
if (pChannel == IFX_NULL)\
{\
    return IFX_ERROR; \
}

#define KTAPI_TAPI_DEV(dev)\
    TAPI_DEV *pTapiDev; \
    pTapiDev = kTAPI_get_TapiDev(dev); \
if (pTapiDev == IFX_NULL)\
{\
    return IFX_ERROR; \
}


/* Returns the TAPI Device Driver context */
IFX_TAPI_DRV_CTX_t * kTAPI_get_DrvCtx(void)
{
	/* Return the Device context */
	return  IFX_TAPI_DeviceDriverContextGet(TAPI_MAJOR);
}

/* returns the TAPI Device Context */
TAPI_DEV * kTAPI_get_TapiDev(IFX_uint16_t dev)
{
	TAPI_DEV *pTapiDev;
	IFX_TAPI_DRV_CTX_t* pDrvCtx = IFX_NULL;

	/* Retrieve the Device context */
	pDrvCtx = kTAPI_get_DrvCtx();
	if (pDrvCtx == IFX_NULL)
	{
		return IFX_NULL;
	}

	/* return the TAPI device pointer */
	pTapiDev = &pDrvCtx->pTapiDev[dev];
	return (pTapiDev);
}

/* Retuns the TAPi channel Context */
TAPI_CHANNEL * kTAPI_get_TapiChannel(IFX_uint16_t dev, IFX_uint16_t chn)
{
	TAPI_CHANNEL *pChannel;
	IFX_TAPI_DRV_CTX_t* pDrvCtx = IFX_NULL;

	/* Retrieve the Device context */
	pDrvCtx = kTAPI_get_DrvCtx();
	if (pDrvCtx == IFX_NULL)
	{
		return IFX_NULL;
	}

	/* return the TAPI channel pointer */
	pChannel = pDrvCtx->pTapiDev[dev].pChannel + chn;
	return (pChannel);
}





/* TAPI RING Service API */
#if defined(TAPI_FEAT_RINGENGINE)
IFX_int32_t kTAPI_Ring_SetCadenceHighRes(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_RING_CADENCE_t *param)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return IFX_TAPI_Ring_SetCadenceHighRes(pChannel, param);
}

IFX_int32_t kTAPI_Ring_Start(IFX_uint16_t dev, IFX_uint16_t chn)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return IFX_TAPI_Ring_Start(pChannel);
}

IFX_int32_t kTAPI_Ring_Stop(IFX_uint16_t dev, IFX_uint16_t chn)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return IFX_TAPI_Ring_Stop(pChannel);
}
#endif /* defined(TAPI_FEAT_RINGENGINE) */


/* TAPI PCM Service API */
#if defined(TAPI_FEAT_PCM)
IFX_int32_t kTAPI_Phone_PCM_Set_Config(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_PCM_CFG_t  *param)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return TAPI_Phone_PCM_Set_Config(pChannel, param);
}

IFX_int32_t kTAPI_Phone_PCM_Get_Config(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_PCM_CFG_t  *param)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return TAPI_Phone_PCM_Get_Config(pChannel, param);
}

IFX_int32_t kTAPI_Phone_PCM_Set_Activation(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_uint32_t nActive)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return TAPI_Phone_PCM_Set_Activation(pChannel, nActive);
}
#endif /* defined(TAPI_FEAT_PCM) */


/* TAPI event Reporting API */
IFX_int32_t kTAPI_Event_Get(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent)
{
	KTAPI_TAPI_DRVCTX();
	return TAPI_Event_Get(pDrvCtx, pEvent);
}


IFX_int32_t kTAPI_IOCTL_EventEnable(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent)
{
	KTAPI_TAPI_DRVCTX();
	return IOCTL_TAPI_EventEnable(pDrvCtx, pEvent);
}


IFX_int32_t kTAPI_IOCTL_EventDisable(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent)
{
	KTAPI_TAPI_DRVCTX();
	return IOCTL_TAPI_EventDisable(pDrvCtx, pEvent);
}

/* TAPI Device API */
IFX_int32_t kTAPI_DeviceStart(IFX_uint16_t dev,
	IFX_TAPI_DEV_START_CFG_t const *param)
{
	KTAPI_TAPI_DEV(dev);
	return IFX_TAPI_DeviceStart(pTapiDev, param);
}

IFX_int32_t kTAPI_DeviceStop(IFX_uint16_t dev)
{
	KTAPI_TAPI_DEV(dev);
	return IFX_TAPI_DeviceStop(pTapiDev);
}

/* TAPI Line Mode API */
IFX_int32_t kTAPI_Phone_Set_Linefeed(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_LINE_MODE_t mode)
{
	KTAPI_TAPI_CHANNEL(dev, chn);
	return TAPI_Phone_Set_Linefeed(pChannel, mode);
}
