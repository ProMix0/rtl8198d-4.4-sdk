/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_gpio.c
   This module provides resource managment for the DUSLIC XS device GPIO pins.
   A usermode application or other kernel mode drivers may reserve GPIO pins
   for exclusive use. Every access to the DUSLIC XS GPIO resources has to be
   done via this module (direct access using DUSLIC XS messages must be
   avoided!).
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_config.h"
#include "drv_dxs_api.h"
#include "drv_dxs_gpio_priv.h"
#include "drv_dxs_mbx.h"

#ifdef DXS_FEAT_GPIO_CTRL
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define DXS_GPIO_STATUS_HIGH 1
#define DXS_GPIO_STATUS_LOW  0

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t _GpioConfig(DXS_DEVICE_t *pDev, DXS_GPIO_Config_t *pCfg);

static IFX_int32_t _GpioSet(DXS_DEVICE_t *pDev, IFX_uint16_t nSet,
                            IFX_uint16_t nMask);

static IFX_int32_t _GpioGet(DXS_DEVICE_t *pDev, IFX_uint16_t *pGet,
                            IFX_uint16_t nMask);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   \defgroup DxsGpioInternal DUSLIC XS GPIO Internal Functions

   \remarks
   These functions are only used within <b>module</b> drv_dxs_gpio.c
*/

/* @{ */
/**
  Configure the GPIO(s) according to passed settings

   \param pDev    pointer to DUSLIC XS device structure

   \param pCfg    pointer to GPIO configuration structure

   \return
   - DXS_statusOk
*/
static IFX_int32_t _GpioConfig(DXS_DEVICE_t *pDev, DXS_GPIO_Config_t *pCfg)
{
   IFX_int32_t       err         = DXS_statusOk;
   DXS_GPIO_CTRL_t   *pGpioCtrl  = &pDev->pGPIO->gpio_ctrl;
   IFX_uint16_t      nGpio       = pCfg->nGpio;

   if (pCfg->nMode & DXS_GPIO_MODE_OUTPUT)
   {
      /* Set specified bits as output, no value is written to the GPIO (this
         is done with _GpioSet */
      if (nGpio & DXS_IO_DEV_GPIO_0)
         pGpioCtrl->GPIO0_DIR = GPIO_CTRL_DIR_OUTPUT;

      if (nGpio & DXS_IO_DEV_GPIO_1)
         pGpioCtrl->GPIO1_DIR = GPIO_CTRL_DIR_OUTPUT;

      if (nGpio & DXS_IO_DEV_GPIO_2)
         pGpioCtrl->GPIO2_DIR = GPIO_CTRL_DIR_OUTPUT;

      if (nGpio & DXS_IO_DEV_GPIO_3)
         pGpioCtrl->GPIO3_DIR = GPIO_CTRL_DIR_OUTPUT;
   }
   else if (pCfg->nMode & DXS_GPIO_MODE_INPUT)
   {
      /* set specified bits as GPIO input */
      if (nGpio & DXS_IO_DEV_GPIO_0)
         pGpioCtrl->GPIO0_DIR = GPIO_CTRL_DIR_INPUT;

      if (nGpio & DXS_IO_DEV_GPIO_1)
         pGpioCtrl->GPIO1_DIR = GPIO_CTRL_DIR_INPUT;

      if (nGpio & DXS_IO_DEV_GPIO_2)
         pGpioCtrl->GPIO2_DIR = GPIO_CTRL_DIR_INPUT;

      if (nGpio & DXS_IO_DEV_GPIO_3)
         pGpioCtrl->GPIO3_DIR = GPIO_CTRL_DIR_INPUT;
   }
   err = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pGpioCtrl);
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Read the GPIO input value(s) from DUSLIC XS

   \param pDev    pointer to DUSLIC XS device structure

   \param pGet    pointer where the read value shall be stored

   \param nMask   only bits set to '1' will be valid on return

   \return
   - DXS_statusOk
*/
static IFX_int32_t _GpioGet(DXS_DEVICE_t *pDev, IFX_uint16_t *pGet,
                            IFX_uint16_t nMask)
{
   IFX_int32_t          err            = DXS_statusOk;
   DXS_GPIO_RW_DATA_t   *pGpioRwData   = &pDev->pGPIO->gpio_rw_data;

   err = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)pGpioRwData,
                            (IFX_uint32_t *)(IFX_void_t *)pGpioRwData);
   if (DXS_statusOk == err)
   {
      *pGet =  pGpioRwData->GPIO0_VAL |
               (pGpioRwData->GPIO1_VAL << 1) |
               (pGpioRwData->GPIO2_VAL << 2) |
               (pGpioRwData->GPIO3_VAL << 3);
      *pGet &= nMask;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Write the GPIO value(s) to the DUSLIC XS

   \param pDev    pointer to DUSLIC XS device structure

   \param nSet    new bit values

   \param nMask   only bits set to '1' will be modified

   \return
   - DXS_statusOk
*/
static IFX_int32_t _GpioSet(DXS_DEVICE_t *pDev, IFX_uint16_t nSet,
                            IFX_uint16_t nMask)
{
   IFX_int32_t          err            = DXS_statusOk;
   DXS_GPIO_RW_DATA_t   *pGpioRwData   = &pDev->pGPIO->gpio_rw_data;

   if (nMask & DXS_IO_DEV_GPIO_0)
   {
      pGpioRwData->GPIO0_WM = GPIO_RW_DATA_WM_PROGRAM;
      pGpioRwData->GPIO0_VAL = (nSet & DXS_IO_DEV_GPIO_0) ?
         GPIO_RW_DATA_GPIO_VAL_HIGH : GPIO_RW_DATA_GPIO_VAL_LOW;
   }
   if (nMask & DXS_IO_DEV_GPIO_1)
   {
      pGpioRwData->GPIO1_WM = GPIO_RW_DATA_WM_PROGRAM;
      pGpioRwData->GPIO1_VAL = (nSet & DXS_IO_DEV_GPIO_1) ?
         GPIO_RW_DATA_GPIO_VAL_HIGH : GPIO_RW_DATA_GPIO_VAL_LOW;
   }
   if (nMask & DXS_IO_DEV_GPIO_2)
   {
      pGpioRwData->GPIO2_WM = GPIO_RW_DATA_WM_PROGRAM;
      pGpioRwData->GPIO2_VAL = (nSet & DXS_IO_DEV_GPIO_2) ?
         GPIO_RW_DATA_GPIO_VAL_HIGH : GPIO_RW_DATA_GPIO_VAL_LOW;
   }
   if (nMask & DXS_IO_DEV_GPIO_3)
   {
      pGpioRwData->GPIO3_WM = GPIO_RW_DATA_WM_PROGRAM;
      pGpioRwData->GPIO3_VAL = (nSet & DXS_IO_DEV_GPIO_3) ?
         GPIO_RW_DATA_GPIO_VAL_HIGH : GPIO_RW_DATA_GPIO_VAL_LOW;
   }
   err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pGpioRwData);
   RETURN_DEVSTATUS(err, IFX_NULL);
}


/* @} */

/** \addtogroup DXS_KERNEL_INTERFACE */
/* @{ */

/** \defgroup DxsGpioKernel DUSLIC XS GPIO Kernel API
\remarks
  The kernel mode supports using the interrupt capabilities and registering a
  callback. Both - device specific GPIO and channel specific IO pins - can be
  used with this single function interface. The calling software need to pass
  either the address of the device (for the GPIOs) or channel (for the IOs)
  structure on reservation.
  From this point the calling party uses the IO handle for subsequent
  operations.
*/

/* @{ */
/**
   Initialize the DUSLIC XS pin resources.

   \param pDevHandle  handle to DUSLIC XS device structure

   \return
   None.
*/
IFX_void_t DXS_GpioInit(IFX_void_t *pDevHandle)
{
   IFX_int32_t             i;
   struct DXS_GPIO_PORT    *pGpioRes;
   DXS_GPIO_CTRL_t         *pGpio_ctrl;
   DXS_GPIO_RW_DATA_t      *pGpio_rw_data;
   DXS_DEVICE_t            *pDev = (DXS_DEVICE_t *)pDevHandle;

   /* Get GPIO resource structure */
   if (pDev->nChannel != 0)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error initialising: channel GPIOs not available\n",
         ((DXS_CHANNEL_t *)pDevHandle)->pParent->nDevNr));
      return;
   }

   pGpioRes = pDev->pGPIO;
   /* first free memory */
   if (pGpioRes != IFX_NULL)
      DXS_OS_FREE(pGpioRes);

   /* now allocate memory for the GPIO device structure */
   pGpioRes = DXS_OS_MALLOC(sizeof(*pGpioRes));
   if (pGpioRes == IFX_NULL)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error initialising: no memory available\n",
         pDev->nDevNr));
      /* no memory could be allocated. */
      return;
   }
   memset(pGpioRes, 0, sizeof(*pGpioRes));

   /* reset reservations */
   pGpioRes->nGlobalReserve = 0;
   /* Init GPIO resource structure */
   for (i = 0; i < DXS_GPIOS; i++)
   {
      pGpioRes->Gpio[i].pDevHandle  = pDevHandle;
      pGpioRes->Gpio[i].nReserve    = 0;
      pGpioRes->Gpio[i].pGpioRes    = IFX_NULL;
      pGpioRes->Gpio[i].err         = DXS_statusOk;
   }

   /* initialize the cached fw messages */
   pGpio_ctrl = &pGpioRes->gpio_ctrl;
   memset (pGpio_ctrl, 0, sizeof (*pGpio_ctrl));

   pGpio_ctrl->CMD         = DXS_CMD_CMD_EOP;
   pGpio_ctrl->MOD         = DXS_CMD_MOD_GPIO;
   pGpio_ctrl->ECMD        = GPIO_CTRL_ECMD_GPIO_CTRL;
   pGpio_ctrl->LENGTH      = GPIO_CTRL_LENGTH_READ;
   pGpio_ctrl->GPIO0_DIR   = GPIO_CTRL_DIR_INPUT;
   pGpio_ctrl->GPIO1_DIR   = GPIO_CTRL_DIR_INPUT;
   pGpio_ctrl->GPIO2_DIR   = GPIO_CTRL_DIR_INPUT;
   pGpio_ctrl->GPIO3_DIR   = GPIO_CTRL_DIR_INPUT;

   pGpio_rw_data = &pGpioRes->gpio_rw_data;
   memset (pGpio_rw_data, 0, sizeof (*pGpio_rw_data));

   pGpio_rw_data->CMD      = DXS_CMD_CMD_EOP;
   pGpio_rw_data->MOD      = DXS_CMD_MOD_GPIO;
   pGpio_rw_data->ECMD     = GPIO_RW_DATA_ECMD_GPIO_READ_WRITE;
   pGpio_rw_data->LENGTH   = GPIO_RW_DATA_LENGTH;

   pDev->pGPIO = pGpioRes;
}

/**
   Release the DUSLIC XS GPIO resource.

   \param pDevHandle  handle to DUSLIC XS device structure

   \return
   None.
*/
IFX_void_t DXS_GpioReset(IFX_void_t *pDevHandle)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *)pDevHandle;

   /* Get GPIO resource structure */
   if (pDev->nChannel != 0)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error releasing: channel GPIOs not available\n",
         ((DXS_CHANNEL_t *)pDevHandle)->pParent->nDevNr));
      return;
   }

   /* first free memory */
   if (pDev->pGPIO != IFX_NULL)
   {
      DXS_OS_FREE(pDev->pGPIO);
      pDev->pGPIO = IFX_NULL; /* double free protection */
   }

   return;
}

/**
   Reserve a DUSLIC XS GPIO pin resource.

   \param pDevHandle handle to DUSLIC XS device structure

   \param nGpio      mask for GPIOs to reserve (0 = free, 1 = reserve)

   \param ppIoHandle return comunicatin handle

   \return
   - IFX_SUCCESS  if requested GPIO is available
*/
IFX_int32_t DXS_GpioReserve(IFX_void_t *pDevHandle, IFX_uint16_t nGpio,
                            IFX_void_t **ppIoHandle)
{
   IFX_int32_t i, err = IFX_ERROR;
   struct DXS_GPIO_PORT *pGpioRes;
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *)pDevHandle;

   DXS_ASSERT(ppIoHandle);

   /* Get GPIO resource structure */
   if (pDev->nChannel != 0)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error reserving: channel GPIOs not available\n",
         ((DXS_CHANNEL_t *)pDevHandle)->pParent->nDevNr));
      return err;
   }

   pGpioRes = pDev->pGPIO;

   /* Check whether requested GPIO pins are free */
   if ((pGpioRes->nGlobalReserve & nGpio) != 0)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error reserving: requested pins not free,"
         " (request 0x%1x, reserved 0x%1x)\n",
         pDev->nDevNr, nGpio, pGpioRes->nGlobalReserve));
      return err;
   }

   /* Find free GPIO resource */
   for (i = 0; i < DXS_GPIOS; i++)
   {
      if (pGpioRes->Gpio[i].nReserve == 0)
      {
         /* bits reserved at this instance */
         pGpioRes->Gpio[i].nReserve = nGpio;
         /* back pointer to device GPIO container */
         pGpioRes->Gpio[i].pGpioRes = pGpioRes;
         /* bitmask of all GPIO pins reserved */
         pGpioRes->nGlobalReserve   |= nGpio;
         /* Return IO handle in case of success */
         *ppIoHandle = (IFX_void_t *)&pGpioRes->Gpio[i];

         TRACE(DXS, DBG_LEVEL_LOW,
            ("DXS%d: GPIO: success reserving: pin(s) 0x%x\n",
            pDev->nDevNr, nGpio));

         err = IFX_SUCCESS;
         break;
      }
   }
   return err;
}

/**
   Release a DUSLIC XS GPIO pin resource.

   \param pIoHandle  handle returned by DXS_GpioReserve

   \return
   IFX_SUCCESS if releasing targeted GPIO is possible, else IFX_ERROR
*/
IFX_int32_t DXS_GpioRelease(IFX_void_t *pIoHandle)
{
   IFX_int32_t       err = IFX_ERROR;
   IFX_uint8_t       i;
   struct DXS_GPIO_PORT *pGpioRes;
   struct DXS_GPIO      *pGpio      = (struct DXS_GPIO *)pIoHandle;
   DXS_DEVICE_t         *pDev       = (DXS_DEVICE_t *)pGpio->pDevHandle;

   pGpioRes = pGpio->pGpioRes;
   for (i = 0; i < DXS_GPIOS; i++)
   {
      /* Valid handle if pIoHandle equals pointer to GPIO structure /and/
        structure contains reserved pins */
      if (((IFX_void_t*)(&pGpioRes->Gpio[i]) == pIoHandle)
            && (pGpioRes->Gpio[i].nReserve != 0))
      {
         err = IFX_SUCCESS;
         break;
      }
   }

   if (err == IFX_SUCCESS)
   {
      TRACE(DXS, DBG_LEVEL_LOW,
         ("DXS%d: GPIO: success releasing: (pins %1x)\n",
          pDev->nDevNr, pGpio->nReserve));

      /* Mark GPIO resources as "free" */
      pGpioRes->nGlobalReserve &= ~pGpio->nReserve;
      pGpio->pGpioRes = IFX_NULL;
      pGpio->nReserve = 0;
      pGpio->err      = DXS_statusOk;
   }
   else
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error releasing: (mask 0x%04x, handle 0x%p)\n",
         pDev->nDevNr, pGpio->nReserve, pIoHandle));
   }
   return err;
}

/**
   Configure requested DUSLIC XS GPIO pins

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param pCfg       handle to the GPIO configuration structure

   \return
   - IFX_SUCCESS if requested GPIO is available and could be configured
   - IFX_ERROR
*/
IFX_int32_t DXS_GpioConfig(IFX_void_t *pIoHandle, DXS_GPIO_Config_t *pCfg)
{
   IFX_int32_t       err      = IFX_SUCCESS;
   struct DXS_GPIO   *pGpio   = (struct DXS_GPIO *)pIoHandle;
   DXS_DEVICE_t      *pDev    = (DXS_DEVICE_t *)pGpio->pDevHandle;

   /* Check whether GPIO resource is owned by module */
   if ((pGpio->nReserve & pCfg->nGpio) != pCfg->nGpio)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error configuring, not owned by module,"
          " (requested 0x%04x, reserved 0x%04x)\n",
         pDev->nDevNr, pCfg->nGpio,  pGpio->nReserve));

      /* errmsg: GPIO resource is not available.*/
      pGpio->err = DXS_statusGpioNoResource;
      return IFX_ERROR;
   }

   err = _GpioConfig(pDev, pCfg);

   TRACE(DXS, DBG_LEVEL_LOW,
      ("DXS%d: GPIO: %s configuring, (mask 0x%04x, mode 0x%08x, "
       "handle 0x%p)\n", pDev->nDevNr, !err ? "success" : "error",
       pCfg->nGpio, pCfg->nMode, pIoHandle));

   return err;
}


/**
   Set values of DUSLIC XS GPIO pins

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param nSet       values to write for bits masked by 'nMask'

   \param nMask      only bits masked with '1' will be modified

   \return
   - IFX_SUCCESS if requested GPIO is available and could be written
   - IFX_ERROR
*/
IFX_int32_t DXS_GpioSet(IFX_void_t *pIoHandle, IFX_uint16_t nSet,
                        IFX_uint16_t nMask)
{
   IFX_int32_t       err      = IFX_SUCCESS;
   struct DXS_GPIO   *pGpio   = (struct DXS_GPIO *)pIoHandle;
   DXS_DEVICE_t      *pDev    = (DXS_DEVICE_t *)pGpio->pDevHandle;

   /* Check whether GPIO resource is owned by module */
   if ((pGpio->nReserve & nMask) != nMask)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error setting bits, not owned by module, "
          "(requsted %04x, reserved %04x)\n",
         pDev->nDevNr, nMask,  pGpio->nReserve));

      /* errmsg: GPIO resource is not available.*/
      pGpio->err = DXS_statusGpioNoResource;
      return IFX_ERROR;
   }

   err = _GpioSet(pDev, nSet, nMask);

   TRACE(DXS, DBG_LEVEL_LOW,
      ("DXS%d: GPIO: %s setting bits, "
      "(set 0x%04x, mask 0x%04x, handle 0x%p)\n",
      pDev->nDevNr, !err ? "success" : "error", nSet,
      nMask, pIoHandle));
   return err;
}

/**
   Read the value from a DUSLIC XS GPIO pin

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param nGet       pointer where the read value shall be stored

   \param nMask      only bits set to '1' will be stored

   \return
   - IFX_SUCCESS if requested GPIO could successfully be read
   - IFX_ERROR
*/
IFX_int32_t DXS_GpioGet(IFX_void_t *pIoHandle, IFX_uint16_t *nGet,
                        IFX_uint16_t nMask)
{
   IFX_int32_t       err      = IFX_SUCCESS;
   struct DXS_GPIO   *pGpio   = (struct DXS_GPIO *)pIoHandle;
   DXS_DEVICE_t      *pDev    = (DXS_DEVICE_t *)pGpio->pDevHandle;

   /* Check whether GPIO resource is owned by module */
   if ((pGpio->nReserve & nMask) != nMask)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
         ("DXS%d: GPIO: error getting bits,"
         " not owned by module (requsted %04x, reserved %04x)\n",
         pDev->nDevNr, nMask,  pGpio->nReserve));

      /* errmsg: GPIO resource is not available.*/
      pGpio->err = DXS_statusGpioNoResource;
      return IFX_ERROR;
   }

   err = _GpioGet(pDev, nGet, nMask);

   TRACE(DXS, DBG_LEVEL_LOW,
      ("DXS%d: GPIO: %s getting bits, "
      "(get 0x%04x, mask 0x%04x, handle 0x%p)\n",
      pDev->nDevNr, !err ? "success" : "error", *nGet, nMask, pIoHandle));

   return err;
}

/* @} */
/* @} */

/** \defgroup DXSGpioUser DUSLIC XS GPIO User API
\remarks
  The usermode interface provides simplified acces to the GPIO resources. It
  only supports reserving, reading and setting an GPIO. Using interrupts and
  callbacks is not permitted.
*/
/* @{ */

/*****************************************************************************
 * GPIO/IO user interface
 *****************************************************************************/

/**
  Release a DUSLIC XS GPIO pin resource from user mode

   \param pDev    pointer to either DUSLIC XS device or channel structure

   \param pCtrl   pointer to user mode control structure

   \return
   - DXS_statusOk                if requested GPIO is available
   - DXS_statusParam
   - DXS_statusGpioNoResource
*/
IFX_int32_t DXS_GpioReserveUser(DXS_DEVICE_t *pDev,
                                DXS_IO_GPIO_Control_t *pCtrl)
{
   IFX_int32_t err         = DXS_statusOk;
   IFX_void_t  *pDevHandle = (IFX_void_t *)pDev;

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* errmsg: Device not yet initialized. */
      RETURN_DEVSTATUS(DXS_statusNotInitialized,IFX_NULL);
   }

   /* validate parameter */
   if (IFX_NULL == pCtrl)
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   err = DXS_GpioReserve(pDevHandle, pCtrl->nGpio, &pCtrl->pIoHandle);
   if (err == IFX_SUCCESS)
   {
      err = DXS_statusOk;
   }
   else
   {
      /* errmsg: GPIO resource is not available. */
      err = DXS_statusGpioNoResource;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Release a DUSLIC XS IO or GPIO pin resource from user mode

   \param pDev pointer to DUSLIC XS device or channel structure

   \param pCtrl   pointer to user mode control structure

   \return
   - DXS_statusOk
   - DXS_statusParam
   - DXS_statusGpioRelResErr
*/
IFX_int32_t DXS_GpioReleaseUser(DXS_DEVICE_t *pDev,
                                DXS_IO_GPIO_Control_t *pCtrl)
{
   IFX_int32_t err;

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* errmsg: Device not yet initialized. */
      RETURN_DEVSTATUS(DXS_statusNotInitialized,IFX_NULL);
   }

   /* validate parameter */
   if ((pCtrl == IFX_NULL) || (pCtrl->pIoHandle == IFX_NULL))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   err = DXS_GpioRelease(pCtrl->pIoHandle);
   if (err != IFX_SUCCESS)
   {
      struct DXS_GPIO *pGpio = (struct DXS_GPIO *)pCtrl->pIoHandle;

      /* errmsg: Releasing a GPIO resource failed.*/
      err = DXS_statusGpioRelResErr;
      pGpio->err = DXS_statusOk;    /* reset the GPIO error */
   }
   else
   {
      err = DXS_statusOk;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Configure a DUSLIC XS GPIO pin from user mode

   \param pDev    pointer to DUSLIC XS device or channel structure

   \param pCtrl   pointer to user mode control structure

   \return
   - DXS_statusOk                   if requested GPIO is available and could
                                    be configured
   - DXS_statusParam
   - DXS_statusGpioConfigResErr

   \remarks
   A GPIO pin can be configured using this function either as input, or output.
   Configuring a GPIO pin as interrupt triggering input, can only be done by
   calling function DXS_GpioConfig() from kernel space.
*/
IFX_int32_t DXS_GpioConfigUser(DXS_DEVICE_t *pDev,
                               DXS_IO_GPIO_Control_t *pCtrl)
{
   IFX_int32_t          err = DXS_statusIfxError;

   IFX_int32_t          i;
   DXS_GPIO_Config_t    gpioCfg;
   IFX_uint16_t         nInput  = 0;
   IFX_uint16_t         nOutput = 0;

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* errmsg: Device not yet initialized. */
      RETURN_DEVSTATUS(DXS_statusNotInitialized,IFX_NULL);
   }

   /* validate parameter */
   if ((pCtrl == IFX_NULL) || (pCtrl->pIoHandle == IFX_NULL))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   memset (&gpioCfg, 0, sizeof (DXS_GPIO_Config_t));
   /* Parse which GPIOs to configure as input or output */
   for (i = 0; i < DXS_GPIOS; i++)
   {
      if (pCtrl->nGpio & (1 << i))
      {
         if (pCtrl->nMask & (1 << i))
         {
            /* Pin active and configured as output */
            nOutput |= (1 << i);
         }
         else
         {
            /* Pin active and configured as input */
            nInput |= (1 << i);
         }
      }
   }
   if (nInput != 0)
   {
      /* Configure input pins */
      gpioCfg.nGpio = nInput;
      gpioCfg.nMode = DXS_GPIO_MODE_INPUT;
      err = DXS_GpioConfig(pCtrl->pIoHandle, &gpioCfg);
   }
   if (nOutput != 0)
   {
      /* Configure output pins */
      gpioCfg.nGpio = nOutput;
      gpioCfg.nMode = DXS_GPIO_MODE_OUTPUT;

      err = DXS_GpioConfig(pCtrl->pIoHandle, &gpioCfg);
   }
   if ((err != IFX_SUCCESS) &&
      (((struct DXS_GPIO*)pCtrl->pIoHandle)->err != DXS_statusOk))
   {
      struct DXS_GPIO *pGpio = (struct DXS_GPIO *)pCtrl->pIoHandle;

      /* errmsg: Configuring a GPIO resource failed.*/
      err = DXS_statusGpioConfigResErr;
      pGpio->err = DXS_statusOk;    /* reset the GPIO error */
   }
   else
   {
      err = DXS_statusOk;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Set DUSLIC XS GPIO pin resource from user mode

   \param pDev    pointer to DUSLIC XS device or channel structure

   \param pCtrl   pointer to user mode control structure

   \return
   - DXS_statusOk if requested GPIO is available and could be written
   - DXS_statusParam
   - DXS_statusGpioSetResErr
*/
IFX_int32_t DXS_GpioSetUser(DXS_DEVICE_t *pDev, DXS_IO_GPIO_Control_t *pCtrl)
{
   IFX_int32_t err = DXS_statusOk;

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* errmsg: Device not yet initialized. */
      RETURN_DEVSTATUS(DXS_statusNotInitialized,IFX_NULL);
   }

   /* validate parameter */
   if ((pCtrl == IFX_NULL) || (pCtrl->pIoHandle == IFX_NULL))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   err = DXS_GpioSet(pCtrl->pIoHandle, pCtrl->nGpio, pCtrl->nMask);
   if (err != IFX_SUCCESS)
   {
      struct DXS_GPIO *pGpio = (struct DXS_GPIO *)pCtrl->pIoHandle;

      /* errmsg: Setting the value of a GPIO resource failed.*/
      err = DXS_statusGpioSetResErr;
      pGpio->err = DXS_statusOk;    /* reset the GPIO error */
   }
   else
   {
      err = DXS_statusOk;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}

/**
   Read DUSLIC XS GPIO pin resource from user mode

   \param pDev pointer to DUSLIC XS device or channel structure

   \param pCtrl   pointer to user mode control structure

   \return
   - DXS_statusOk if requested GPIO is available and could be read
   - DXS_statusParam
   - DXS_statusGpioGetResErr
*/
IFX_int32_t DXS_GpioGetUser(DXS_DEVICE_t* pDev, DXS_IO_GPIO_Control_t *pCtrl)
{
   IFX_int32_t err = DXS_statusOk;

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* errmsg: Device not yet initialized. */
      RETURN_DEVSTATUS(DXS_statusNotInitialized,IFX_NULL);
   }

   /* validate parameter */
   if ((pCtrl == IFX_NULL) || (pCtrl->pIoHandle == IFX_NULL))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   err = DXS_GpioGet(pCtrl->pIoHandle, &pCtrl->nGpio, pCtrl->nMask);
   if (err != IFX_SUCCESS)
   {
      struct DXS_GPIO *pGpio = (struct DXS_GPIO *)pCtrl->pIoHandle;

      /* errmsg: Getting the value of a GPIO resource failed.*/
      err = DXS_statusGpioGetResErr;
      pGpio->err = DXS_statusOk;    /* reset the GPIO error */
   }
   else
   {
      err = DXS_statusOk;
   }
   RETURN_DEVSTATUS(err, IFX_NULL);
}
/* @} */

/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
#endif /* DXS_FEAT_GPIO_CTRL */

