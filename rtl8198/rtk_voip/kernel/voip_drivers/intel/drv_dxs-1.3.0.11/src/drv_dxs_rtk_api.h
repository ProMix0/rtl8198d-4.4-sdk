#ifndef DRV_DXS_RTK_API_H
#define DRV_DXS_RTK_API_H
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
#include "drv_dxs_api.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_init.h"
/* ============================= */
/* Global type definition        */
/* ============================= */


/* ============================= */
/* Global function declaration   */
/* ============================= */
extern IFX_int32_t DXS_DwldAndStartFW (DXS_DEVICE_t *pDev, DXS_FW_Download_t *pEdsp);
extern IFX_int32_t DXS_Version (DXS_DEVICE_t *pDev, DXS_IO_Version_t *pInd);


extern IFX_int32_t DXS_DeviceDriverStart(void);

IFX_int32_t kDXS_ChipAccessInit(IFX_uint16_t dev,
	DXS_BasicDeviceInit_t *pBasicDeviceInit);

IFX_int32_t kDXS_DwldAndStartFW(IFX_uint16_t dev,
	DXS_FW_Download_t *pEdsp);


IFX_int32_t kDXS_Version(IFX_uint16_t dev,
	DXS_IO_Version_t *pInd);



IFX_int32_t kDXS_BBD_Download(IFX_uint16_t dev, IFX_uint16_t chn,
	DXS_BBD_Download_t *pBBD);

#endif /* DRV_DXS_RTK_API_H */

