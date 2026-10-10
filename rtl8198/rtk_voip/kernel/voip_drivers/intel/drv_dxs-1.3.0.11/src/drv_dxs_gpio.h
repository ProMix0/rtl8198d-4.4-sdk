#ifndef _DRV_DXS_GPIO_H
#define _DRV_DXS_GPIO_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_gpio.h
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
#ifndef _DRV_DXS_GPIO_PRIV_H
struct DXS_GPIO_PORT;
#endif /* _DRV_DXS_GPIO_PRIV_H */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_void_t DXS_GpioInit      (IFX_void_t *pDevHandle);

extern IFX_void_t DXS_GpioReset     (IFX_void_t *pDevHandle);

#endif /* DXS_FEAT_GPIO_CTRL */
#endif /* _DRV_DXS_GPIO_H */
