/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_rtk_api.c
   This file contains an example of integration of the TAPI High-Level for Realtek
						API from Kernel Space 
*/
/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_dxs_rtk_api.h"
/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */


IFX_int32_t kDXS_ChipAccessInit(IFX_uint16_t dev,
	DXS_BasicDeviceInit_t *pBasicDeviceInit)
{
	DXS_DEVICE_t *pDev;
	DXS_GetDevice(dev, &pDev);

	pBasicDeviceInit->dev = dev;
	return DXS_ChipAccessInit(pDev, pBasicDeviceInit);
}


IFX_int32_t kDXS_DwldAndStartFW(IFX_uint16_t dev,
	DXS_FW_Download_t *pEdsp)
{
	DXS_DEVICE_t *pDev;
	DXS_GetDevice(dev, &pDev);

	pEdsp->dev = dev;
	return DXS_DwldAndStartFW(pDev, pEdsp);
}


IFX_int32_t kDXS_Version(IFX_uint16_t dev,
	DXS_IO_Version_t *pInd)
{
	DXS_DEVICE_t *pDev;
	DXS_GetDevice(dev, &pDev);

	pInd->dev = dev;
	return DXS_Version(pDev, pInd);
}



IFX_int32_t kDXS_BBD_Download(IFX_uint16_t dev, IFX_uint16_t chn, 
					DXS_BBD_Download_t *pBBD)
{
	DXS_DEVICE_t *pDev;
	DXS_CHANNEL_t *pCh;

	DXS_GetDevice(dev, &pDev);
	pCh = &pDev->pChannel[chn];

//#ifdef TAPI_ONE_DEVNODE	
	pBBD->dev = dev;
//#endif	
	return DXS_BBD_Download(pCh, pBBD);
}

