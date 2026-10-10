#ifndef DRV_TAPI_RTK_API_H
#define DRV_TAPI__RTK_API_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file
   \remarks
 *******************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi.h"

/* ============================= */
/* Global type definition        */
/* ============================= */
#ifdef DXS_LINUX_USER_SPACE
#define TAPI_MAJOR 125
#else /* DXS_LINUX_USER_SPACE */
#define TAPI_MAJOR 0
#endif /* DXS_LINUX_USER_SPACE */

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_int_t kTAPI_ModuleCreate(void);

/* TAPI RING Service API */
#if defined(TAPI_FEAT_RINGENGINE)
extern IFX_int32_t kTAPI_Ring_SetCadenceHighRes(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_RING_CADENCE_t *param);

extern IFX_int32_t kTAPI_Ring_Start(IFX_uint16_t dev, IFX_uint16_t chn);
extern IFX_int32_t kTAPI_Ring_Stop(IFX_uint16_t dev, IFX_uint16_t chn);
#endif /* defined(TAPI_FEAT_RINGENGINE) */


/* TAPI PCM Service API */
#if defined(TAPI_FEAT_PCM)
extern IFX_int32_t kTAPI_Phone_PCM_Set_Config(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_PCM_CFG_t  *param);

extern IFX_int32_t kTAPI_Phone_PCM_Get_Config(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_PCM_CFG_t  *param);

extern IFX_int32_t kTAPI_Phone_PCM_Set_Activation(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_uint32_t nActive);
#endif /* defined(TAPI_FEAT_PCM) */


/* TAPI event Reporting API */
extern IFX_int32_t kTAPI_Event_Get(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t kTAPI_IOCTL_EventEnable(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t kTAPI_IOCTL_EventDisable(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_EVENT_t *pEvent);

/* TAPI Device API */
extern IFX_int32_t kTAPI_DeviceStart(IFX_uint16_t dev,
	IFX_TAPI_DEV_START_CFG_t const *param);

extern IFX_int32_t kTAPI_DeviceStop(IFX_uint16_t dev);

/* TAPI Line Mode API */
extern IFX_int32_t kTAPI_Phone_Set_Linefeed(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_LINE_MODE_t mode);
#endif /* DRV_TAPI_RTK_API_H */

