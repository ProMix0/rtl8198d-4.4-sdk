#ifndef _DRV_DXS_INIT_H
#define _DRV_DXS_INIT_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_init.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
typedef enum
{
   /* boot directly from ROM */
   DXS_BOOT_ROM = 0,
   DXS_BOOT_UART,
   /* boot from i/o address SPI command inbox (FW patch download) */
   DXS_BOOT_SPI
} DXS_BOOT_INFO_t;

/* ========================================================================== */
/*                       Global variable declaration                          */
/* ========================================================================== */
extern const IFX_char_t DRV_DXS_WHATVERSION[];
#ifdef HAVE_CONFIG_H
extern const IFX_char_t DRV_DXS_WHICHCONFIG[];
#endif /* HAVE_CONFIG_H */
extern enum DXS_DcDcType  nAllowedDcDcType;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t   DXS_GetDevice (IFX_uint16_t nr, DXS_DEVICE_t** pDev);

extern DXS_CHANNEL_t *DXS_GetNeighbourChannel (DXS_CHANNEL_t *pCh);

extern IFX_int32_t   DXS_DeviceDriverStart(void);

extern IFX_void_t    DXS_DeviceDriverStop (void);

extern IFX_int32_t   DXS_SetBootConfig (DXS_DEVICE_t *pDev,
                                        DXS_BOOT_INFO_t bootInfo);

extern IFX_int32_t   DXS_ResetController (DXS_DEVICE_t *pDev,
                                          DXS_BOOT_INFO_t bootInfo);

extern IFX_int32_t DXS_Wait4BootFinished (
                        DXS_DEVICE_t *pDev,
                        IFX_uint16_t nLoop);

extern IFX_int32_t   DXS_ReadFwVersion (
                              DXS_DEVICE_t *pDev);

extern IFX_int32_t   DXS_ChipAccessInit (DXS_DEVICE_t *pDev,
                              DXS_BasicDeviceInit_t *pBasicDeviceInit);

extern IFX_int32_t   DXS_FW_Start (DXS_DEVICE_t *pDev, DXS_IO_Init_t *pInit);

#endif /* _DRV_DXS_INIT_H */
