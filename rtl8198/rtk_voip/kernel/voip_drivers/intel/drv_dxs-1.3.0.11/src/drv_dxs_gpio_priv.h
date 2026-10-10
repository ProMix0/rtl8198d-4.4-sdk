#ifndef _DRV_DXS_GPIO_PRIV_H
#define _DRV_DXS_GPIO_PRIV_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_gpio_priv.h
   This file contains the defines, the structures declarations of gpio
   module of DUSLIC XS.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_config.h"

#ifdef DXS_FEAT_GPIO_CTRL
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

#define DXS_GPIOS    4

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/**
   GPIO structure for single GPIOs
*/
struct DXS_GPIO
{
   /** Handle to DUSLIC XS device or CHANNEL stucture */
   IFX_void_t           *pDevHandle;
   /** Pointer to enclosing reservation struct */
   struct DXS_GPIO_PORT *pGpioRes;
   /** Mask of reserved bits for this resource (0 = free, 1 = reserved) */
   IFX_uint16_t         nReserve;
   /** Error code for GPIO resource */
   IFX_int32_t          err;
};

/**
   GPIO resource structure incl. FW message
*/
struct DXS_GPIO_PORT
{
   DXS_GPIO_CTRL_t      gpio_ctrl;
   DXS_GPIO_RW_DATA_t   gpio_rw_data;
   /** Mask of reserved bits for all resources (0 = free, 1 = reserved) */
   IFX_uint16_t         nGlobalReserve;
   /** Control struct for the single GPIOs */
   struct DXS_GPIO      Gpio[DXS_GPIOS];
};


/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#endif /* DXS_FEAT_GPIO_CTRL */

#endif /* _DRV_DXS_GPIO_PRIV_H */
