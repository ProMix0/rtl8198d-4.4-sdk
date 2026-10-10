#ifndef _DRV_DXS_DWLD_H
#define _DRV_DXS_DWLD_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dwld.h
   This file contains the declaration of the download and CRC structures,
   macros and functions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_config.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_DwldFirmwareSelect (
                        DXS_DEVICE_t *pDev,
                        DXS_FW_Download_t *pFwDwld);

#endif /* _DRV_DXS_DWLD_H */
