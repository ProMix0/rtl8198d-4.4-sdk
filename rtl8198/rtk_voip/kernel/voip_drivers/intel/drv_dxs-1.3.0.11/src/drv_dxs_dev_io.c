/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : DXS Driver, DEV_IO part
   ========================================================================= */

/* ============================================================================
   Inlcudes
   ========================================================================= */

#include "ifx_types.h"
#include "ifxos_std_defs.h"
#include "ifxos_select.h"
#include "drv_dxs_version.h"
#include "drv_dxs_api.h"
#include "drv_dxs.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_init.h"

#ifdef DXS_USE_DEV_IO

#include "ifxos_device_access.h"
/* get at first the driver configuration */
#include "ifxos_device_io.h"

/* ============================================================================
   Local Macros & Definitions
   ========================================================================= */


/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */

static DXS_OS_mutex_t SemDrvIF;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */


/**
   Create a TAPI driver module.

\return
   IFX_SUCCESS or IFX_ERROR

*/
IFX_int_t DXS_ModuleCreate(void)
{
   IFX_int_t ret;

   DXS_OS_MutexInit (&SemDrvIF);

   ret = DXS_DeviceDriverStart();

   return ret;
}


/**
   Remove the TAPI Control Driver.

\return
   always IFX_SUCCESS
*/
IFX_int_t DXS_ModuleDelete(void)
{
   DXS_DeviceDriverStop();

   return IFX_SUCCESS;
}

/**
   Open the device from kernel mode.

   \param pLLChDev Handle to TAPI low level channel or device structure

   \return
      always DXS_statusOk
*/
IFX_int32_t DXS_OS_DeviceOpen (IFX_TAPI_LL_CH_t *pLLChDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t*)pLLChDev;
   DXS_CHANNEL_t  *pCh = IFX_NULL;

   DXS_OS_MutexGet (&SemDrvIF);

   if (pDev->nChannel != 0)
   {
      pCh = (DXS_CHANNEL_t*)pDev;
      pDev = pCh->pParent;

      /* increase in use count */
      pCh->nInUse++;
   }

   /* increase in use count */
   pDev->nInUse++;

   DXS_OS_MutexRelease (&SemDrvIF);
   return DXS_statusOk;
}

/**
   Release the device from Kernel mode.

   \param pLLChDev Handle to TAPI low level channel or device structure

   \return
      always DXS_statusOk
*/
IFX_int32_t DXS_OS_DeviceRelease (IFX_TAPI_LL_CH_t *pLLChDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t*)pLLChDev;
   DXS_CHANNEL_t  *pCh = IFX_NULL;

   DXS_OS_MutexGet (&SemDrvIF);

   if (pDev->nChannel != 0)
   {
      pCh = (DXS_CHANNEL_t*)pDev;
      pDev = pCh->pParent;

      /* decrease in use count */
      pCh->nInUse--;
   }

   /* deccrease in use count */
   pDev->nInUse--;
   DXS_OS_MutexRelease (&SemDrvIF);

   return DXS_statusOk;
}

IFX_void_t DXS_OS_ThreadKill (DXS_OS_ThreadCtrl_t *pThrCntrl,
                              DXS_OS_mutex_t *pMutex)
{
   DXS_OS_ThreadDelete (pThrCntrl, 500);
}

#endif /* DXS_USE_DEV_IO */


