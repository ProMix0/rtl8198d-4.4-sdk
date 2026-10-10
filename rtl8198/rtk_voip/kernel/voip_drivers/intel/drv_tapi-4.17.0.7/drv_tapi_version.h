#ifndef _DRV_TAPI_VERSION_H
#define _DRV_TAPI_VERSION_H
/******************************************************************************

  Copyright (c) 2001-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include "drv_tapi_ll_if_version.h"

/** driver version, major number */
#define DRV_TAPI_VER_MAJOR 4
/** driver version, minor number */
#define DRV_TAPI_VER_MINOR 17
/** driver version, build number */
#define DRV_TAPI_VER_STEP 0
/** driver version, package type */
#define DRV_TAPI_VER_TYPE 7

#define IFX_TAPI_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#endif /* _DRV_TAPI_VERSION_H */
