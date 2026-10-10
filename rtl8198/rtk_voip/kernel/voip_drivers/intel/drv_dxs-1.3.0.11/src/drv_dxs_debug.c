/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_debug.c
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"
#include "drv_dxs_debug.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_init.h"
#include "drv_dxs_mbx.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#if 0
static IFX_int32_t DXS_CmdBoxReadTest(DXS_DEVICE_t *pDev, IFX_uint16_t max_val);
static IFX_int32_t DXS_CmdBoxWriteTest(DXS_DEVICE_t *pDev,IFX_uint16_t max_val);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Test the command box access of DUSLIC XS device.

   \param pDev    pointer to the device interface

   \return
   - DXS_statusNotSupported
*/
static IFX_int32_t DXS_CmdBoxReadTest (DXS_DEVICE_t *pDev, IFX_uint16_t max_val)
{
   TRACE (DXS,DBG_LEVEL_HIGH, ("DXS_ERR: DXS_CmdBoxReadTest not supported\n"));
   RETURN_DEVSTATUS(DXS_statusNotSupported, IFX_NULL);
}

/**
   Test the command box access of DUSLIC XS device on channel B by writing
   continously different values to message SDD_TestConfig and check if they can
   be read back.

   \param pDev    pointer to the device interface

   \param max_val number of repetitions

   \return
   - DXS_statusNotSupported
*/
static IFX_int32_t DXS_CmdBoxWriteTest(DXS_DEVICE_t *pDev, IFX_uint16_t max_val)
{
   TRACE (DXS,DBG_LEVEL_HIGH, ("DXS_ERR: DXS_CmdBoxWriteTest not supported\n"));
   RETURN_DEVSTATUS(DXS_statusNotSupported, IFX_NULL);
}
#endif /*0*/

/**
   DUSLIC XS Report Set for all messages.

   \param pDev          Pointer to the device interface.

   \param driver_level  New DBG_LEVEL.

   \return
*/
IFX_int32_t DXS_Report_Set(IFX_uint32_t driver_level)
{
   if ((driver_level > DBG_LEVEL_HIGH) || (driver_level < DBG_LEVEL_LOW))
   {
      TRACE_LEVEL_SET(DXS, DBG_LEVEL_OFF);
   }
   else
   {
      TRACE_LEVEL_SET(DXS, driver_level);
   }
   return DXS_statusOk;
}

/**
   Test the basic access of DUSLIC XS device.

   \param pDev    pointer to the device interface

   \param max_val maximum test value ( <= 0xFFFF )

   \return
   DXS_statusOk
*/
IFX_int32_t DXS_AccessTest (DXS_DEVICE_t *pDev, IFX_uint16_t max_val)
{
   TRACE (DXS,DBG_LEVEL_HIGH, ("DXS_ERR: DXS_AccessTest not supported\n"));
   RETURN_DEVSTATUS(DXS_statusNotSupported, IFX_NULL);
}
