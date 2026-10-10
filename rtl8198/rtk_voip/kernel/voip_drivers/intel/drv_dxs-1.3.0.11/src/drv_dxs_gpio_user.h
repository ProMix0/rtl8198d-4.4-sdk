#ifndef _DRV_DXS_GPIO_USER_H
#define _DRV_DXS_GPIO_USER_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_gpio_user.h
   DuSLIC I/O resource managment module.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_config.h"

#ifdef DXS_FEAT_GPIO_CTRL
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_GpioReserveUser(DXS_DEVICE_t *pDev,
                                       DXS_IO_GPIO_Control_t *pCtrl);
extern IFX_int32_t DXS_GpioReleaseUser(DXS_DEVICE_t *pDev,
                                       DXS_IO_GPIO_Control_t *pCtrl);
extern IFX_int32_t DXS_GpioConfigUser(DXS_DEVICE_t *pDev,
                                      DXS_IO_GPIO_Control_t *pCtrl);
extern IFX_int32_t DXS_GpioGetUser    (DXS_DEVICE_t* pDev,
                                       DXS_IO_GPIO_Control_t *pCtrl);
extern IFX_int32_t DXS_GpioSetUser    (DXS_DEVICE_t *pDev,
                                       DXS_IO_GPIO_Control_t *pCtrl);
#endif /* DXS_FEAT_GPIO_CTRL */
#endif /* _DRV_DXS_GPIO_USER_H */
