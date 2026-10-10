/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_init.c
   This file implements functions for the Initialization of the device.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"
#include "drv_dxs_init.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_irq.h"
#include "drv_dxs_version.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_utd.h"
#include "drv_dxs_fifo.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_fw_cmd_sdd.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* maximum number of capabilities, used for allocating the array */
#define DXS_MAX_CAPS             12

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

/** device driver version string */
#define DRV_DXS_VER_STR          _MKSTR(MAJORSTEP)    "."   \
                                 _MKSTR(MINORSTEP)    "."   \
                                 _MKSTR(VERSIONSTEP)  "."   \
                                 _MKSTR(VERS_TYPE)
/** TAPI LL interface version string */
#define DRV_LL_INTERFACE_VER_STR _MKSTR(LL_IF_MAJORSTEP)    "."      \
                                 _MKSTR(LL_IF_MINORSTEP)    "."      \
                                 _MKSTR(LL_IF_VERSIONSTEP)  "."      \
                                 _MKSTR(LL_IF_VERS_TYPE)

/** what compatible driver version */
#define DRV_DXS_WHAT_STR \
        "@(#)DUSLIC XS device driver, version " DRV_DXS_VER_STR


/* ========================================================================== */
/*                                 Defines                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */


/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
/**
   Define trace output for DUSLIC XS
   OFF:    no output
   HIGH:   only important traces, as errors or some warnings
   NORMAL: including traces from high and general proceedings and
           possible problems
   LOW:    all traces and low level traces as basic chip access
           and interrupts, command data
   Traces can be completely switched off with the compiler switch
   ENABLE_TRACE to 0
*/
CREATE_TRACE_GROUP(DXS);

/* Variable that is used to set the initial debug trace level.
   In some OS implementations it can be overwritten when starting the driver.
   debug_level: LOW (1), NORMAL (2), HIGH (3), OFF (4) */
#ifdef ENABLE_TRACE
#ifdef DEBUG
   #define DXS_DBG_LEVEL_DEFAULT       DBG_LEVEL_LOW
#else /* DEBUG */
   #define DXS_DBG_LEVEL_DEFAULT       DBG_LEVEL_HIGH
#endif /* DEBUG */

/* when both drv_tapi and drv_dxs compiled as libraries,
   then there will be conflicts if debug_level name
   is used in both drivers, so change variable name for drv_dxs */
#ifdef DXS_USE_DEV_IO
IFX_uint32_t DEVIO_dxs_debug_level = DXS_DBG_LEVEL_DEFAULT;
static IFX_uint32_t * const p_dxs_debug_level = &DEVIO_dxs_debug_level;
#else /* DXS_USE_DEV_IO */
IFX_uint32_t debug_level = DXS_DBG_LEVEL_DEFAULT;
static IFX_uint32_t * const p_dxs_debug_level = &debug_level;
#endif /* DXS_USE_DEV_IO */
#endif /* ENABLE_TRACE */

/** what string support, driver version string */
const IFX_char_t DRV_DXS_WHATVERSION[] = DRV_DXS_WHAT_STR;
#ifdef HAVE_CONFIG_H
/** which configure options were set */
const IFX_char_t DRV_DXS_WHICHCONFIG[] = DXS_CONFIGURE_STR;
#endif /* HAVE_CONFIG_H */

extern IFX_uint16_t major;
extern IFX_uint16_t minorBase;
/* extern IFX_char_t  *devName; */

/* Allowed type of DC/DC converter */
enum DXS_DcDcType  nAllowedDcDcType;
/* String from commandline - overwritten by OS specific code. */
IFX_char_t *dcdc_type = "";

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
extern unsigned char current_slicType;
#endif

/* ========================================================================== */
/*                             Local  variables                               */
/* ========================================================================== */
/* static variable of the driver context struct */
static IFX_TAPI_DRV_CTX_t DrvCtx;
IFX_TAPI_DRV_CTX_t *RtkCtx = &DrvCtx;
/* static array of device structs */
static DXS_DEVICE_t DxsDevices[DXS_MAX_DEVICES];


/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t          dxs_ChipAccessExit (
                              DXS_DEVICE_t *pDev);

static IFX_TAPI_LL_DEV_t*  DXS_TAPI_LL_DevicePrepare (
                              TAPI_DEV *pTapiDev,
                              IFX_uint32_t devNum);

static IFX_int32_t         DXS_TAPI_LL_DeviceInit (
                              IFX_TAPI_LL_DEV_t* pLLDev);

static IFX_void_t          dxs_DeviceExit (
                              DXS_DEVICE_t *pDev,
                              IFX_boolean_t bChipAccess);

static IFX_void_t          DXS_TAPI_LL_DeviceExit (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_boolean_t bChipAccess);

static IFX_TAPI_LL_CH_t*   DXS_TAPI_LL_ChannelPrepare (
                              TAPI_CHANNEL *pTapiCh,
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_uint32_t chNum);

static IFX_int32_t         DXS_TAPI_LL_ChannelInit (
                              IFX_TAPI_LL_CH_t *pLLCh);

static IFX_void_t          dxs_ChannelExit (
                              DXS_CHANNEL_t *pCh,
                              IFX_boolean_t bChipAccess);

static IFX_int32_t         DXS_TAPI_LL_FW_Start (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_void_t const *pProc);

static IFX_int32_t         DXS_TAPI_LL_FW_Init (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_uint8_t nMode);

static IFX_int32_t         DXS_TAPI_LL_BBD_Dnld (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_void_t const *pProc);

static IFX_void_t          dxs_DwldPtrSet (
                              DXS_IO_Init_t *pInit,
                              DXS_FW_Download_t *pEdspDwld);

static IFX_void_t          dxs_DwldPtrUnset (
                              DXS_FW_Download_t *pEdspDwld);

extern IFX_int32_t         dxs_DefaultRegInit (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_FW_DevDataInit (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_SetDevCaps (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_AddCaps (
                              DXS_DEVICE_t *pDev);

static IFX_void_t          AddCapability (
                              IFX_TAPI_CAP_t* CapList,
                              IFX_uint32_t *pnCap,
                              IFX_char_t const * description,
                              IFX_int32_t type,
                              IFX_int32_t value);

static IFX_int32_t         DXS_TAPI_LL_Phone_Check_Capability (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_TAPI_CAP_t    *pCapList);

static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capability_List (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_TAPI_CAP_LIST_t *pCapList);

static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capabilities (
                              IFX_TAPI_LL_DEV_t *pLLDev);


/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

#ifdef DXS_TID_DCDC_AUTODETECT
/**
   Autodetection of a DC/DC variant.

   \param  pDev Pointer to a device structure.
   \param  pRet Return value (output parameter):
                DXS_statusOk if ok.
                DXS_statusDcDcTypeUnknown if unknown DC/DC type was detected.
                DXS_statusChipAccFailed if command read or command write failed.
   \return
   none
   \remarks
   This function is called only for platforms that support autodetection.
*/
static IFX_void_t dxs_dcdc_autodetect (DXS_DEVICE_t *pDev, IFX_int32_t *pRet)
{
   IFX_int32_t err = DXS_statusOk;

   if (nAllowedDcDcType == DXS_DCDC_TYPE_AUTO)
   {
      union dxs_gpio_ctrl
      {
         IFX_uint32_t     data[2];
         DXS_GPIO_CTRL_t  cmd;
      };

      union dxs_gpio_data
      {
         IFX_uint32_t        data[2];
         DXS_GPIO_RW_DATA_t  cmd;
      };

      union dxs_gpio_ctrl  gpio_ctrl;
      union dxs_gpio_data  gpio_data;
      IFX_uint32_t         bkp_gpio_ctrl;

      gpio_ctrl.data[0] = 0x86000204;
      gpio_data.data[0] = 0x86000304;
      gpio_ctrl.data[1] = gpio_data.data[1] = 0;

      /* read GPIO_CTRL */
      err = DXS_CmdRead(pDev, (IFX_uint32_t *)&gpio_ctrl,
                              (IFX_uint32_t *)&gpio_ctrl);
      if (DXS_statusOk == err)
      {
         bkp_gpio_ctrl = gpio_ctrl.data[1];

         /* enable internal PU */
         gpio_ctrl.cmd.GPIO3_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO2_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO1_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO0_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO3_DIR = 1;
         gpio_ctrl.cmd.GPIO2_DIR = 1;
         gpio_ctrl.cmd.GPIO1_DIR = 1;
         gpio_ctrl.cmd.GPIO0_DIR = 1;
         gpio_ctrl.data[0] &= ~(1 << 31);
         err = DXS_CmdWrite(pDev, (IFX_uint32_t *)&gpio_ctrl);
         if (DXS_statusOk == err)
         {
            /* read the pin status */
            err = DXS_CmdRead(pDev, (IFX_uint32_t *)&gpio_data,
                                    (IFX_uint32_t *)&gpio_data);
            if (DXS_statusOk == err)
            {
               /* restore original GPIO_CTRL */
               gpio_ctrl.data[1] = bkp_gpio_ctrl;
               err = DXS_CmdWrite(pDev, (IFX_uint32_t *)&gpio_ctrl);
            }
         }
      }

      if (DXS_statusOk == err)
      {
         IFX_uint32_t gpio_status_word = ((gpio_data.cmd.GPIO3_VAL << 3) |
                                          (gpio_data.cmd.GPIO2_VAL << 2) |
                                          (gpio_data.cmd.GPIO1_VAL << 1) |
                                          (gpio_data.cmd.GPIO0_VAL << 0)) & 0xF;

         /* GPIO3   GPIO2   GPIO1   GPIO0
            1       0       0       1        IFB12
            1       0       0       0        CIFB12
            1       1       1       0        CIBB12
            1       1       1       1        IBB12
         */

         switch (gpio_status_word)
         {
            case 0x9:
               nAllowedDcDcType = DXS_DCDC_TYPE_IFB;
               break;

            case 0x8:
               nAllowedDcDcType = DXS_DCDC_TYPE_CIFB;
               break;

            case 0xE:
               nAllowedDcDcType = DXS_DCDC_TYPE_CIBB;
               break;

            case 0xF:
               nAllowedDcDcType = DXS_DCDC_TYPE_IBB;
               break;

            default:
               nAllowedDcDcType = DXS_DCDC_TYPE_NOTSET;
               *pRet = DXS_statusDcDcTypeUnknown;
               return;
         }
      }
      else
      {
         /* chip access error */
         nAllowedDcDcType = DXS_DCDC_TYPE_NOTSET;
         *pRet = DXS_statusChipAccFailed;
         return;
      }
   }

   *pRet = DXS_statusOk;
}
#endif /* DXS_TID_DCDC_AUTODETECT */

/**
   Get a pointer to the device struct.

   \param  nr           Number of the device. Counting starts with zero.
   \param  pDev         Returns pointer to a device structure.

   \return
   DXS_statusOk         if ok.
   DXS_statusIfxError   if device number is out of supported range.
*/
IFX_int32_t DXS_GetDevice (IFX_uint16_t nr, DXS_DEVICE_t** pDev)
{
   if (nr >= DXS_MAX_DEVICES)
   {
      *pDev = IFX_NULL;
      return DXS_statusIfxError;
   }

   *pDev = &DxsDevices[nr];
   return DXS_statusOk;
}


/**
   Get a pointer to the neighbour channel struct of the given DXS channel.

   The DXS only has two channels by HW design and so if channel 0 is passed
   channel 1 is returned and vice versa.

   \param  pCh          Pointer to the channel.

   \return
   Pointer to the DXS channel struct of the neighbour channel. Also for
   1 channel devices there is a neighbouring channel although it will not
   carry any resources.
*/
DXS_CHANNEL_t *DXS_GetNeighbourChannel (DXS_CHANNEL_t *pCh)
{
   IFX_uint8_t nCh;

   /* Note that the channel number stored in the channel is index + 1. */
   nCh = (pCh->nChannel - 1) == 0 ? 1 : 0;

   return &pCh->pParent->pChannel[nCh];
}


/**
   Initialise the access to the chip.

   This function allocates all resources needed for access to the chip.

   \param  pDev         Pointer to the device structure.
   \param  pBasicDeviceInit  Pointer to the chip access init parameters.

   \return
   - IFX_SUCCESS        if successful
   - IFX_ERROR          in case of an error

   \remarks
   Must be the first action after installing the driver and
   before any access to the hardware (corresponding DUSLIC XS chip).
*/
IFX_int32_t DXS_ChipAccessInit(DXS_DEVICE_t *pDev,
                               DXS_BasicDeviceInit_t *pBasicDeviceInit)
{
   IFX_int32_t ret = IFX_SUCCESS;

   /* make sure that we really got a device structure */
   DXS_ASSERT((pDev != NULL && pDev->nChannel == 0));

   if (pDev->nDevState & DS_BASIC_INIT)
   {
      /* Chip access was already initialised. So this is a reconfiguration.
         Reset all driver internal states and do a chip access exit. */
      IFX_TAPI_DeviceReset (pDev->pTapiDev);
   }

   /* initialize firmware download protection semaphore */
   DXS_OS_MutexInit (&pDev->mtxFwDlAcc);

   /* initialize SPI access protection semaphore */
   DXS_OS_MutexInit (&pDev->mtxSpiAcc);

   /* initialize mailbox protection semaphore */
   DXS_OS_MutexInit (&pDev->mtxMbxAcc);

   /* Initialise SPI access to the device. */
   dxs_init_spi (pDev);

   /* check if device is connected and accessible */
   ret = DXS_reg_access_test(pDev);

   if (DXS_statusOk != ret)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("ERROR: SPI access test failed\n"));
      RETURN_DEVSTATUS(DXS_statusTestChipAccErr, IFX_NULL);
   }
   ret = IFX_SUCCESS;

   /* execute a controller reset */
   ret = DXS_ResetController(pDev, DXS_BOOT_ROM);
   if (DXS_statusOk != ret)
      RETURN_DEVSTATUS(DXS_statusCtrlResErr, IFX_NULL);

   /*lint -save -esym(613,pBasicDeviceInit) possible use of NULL pointer
      -> within macro ON_IOCTL_FRUSR the parameter is already checked against
      possible NULL pointers with DXS_ASSERT */
   if (pBasicDeviceInit->nIrqNum >= 0)
#ifdef DXS_HAVE_INTERRUPTS
   {
      /* Set flag that events should be retrieved after an interrupt. */
      pDev->nEventHandlingMode = DXS_EVENT_INTERRUPT;

      OS_Install_DxsIRQHandler(pDev, pBasicDeviceInit->nIrqNum);

      if (pDev->pIrq == IFX_NULL)
      {
         /* Don't start event handling for the device. */
         pDev->nEventHandlingMode = DXS_EVENT_STOP;
         ret = IFX_ERROR;
      }
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      else
      {
#ifdef IRQLINE_INIT
         /* Configure and enable the interrupt line on the Port and
            IRQ controller. */
         IRQLINE_INIT(pDev);
#ifdef VXWORKS
         pInterruptCounters[pDev->nDevNr] = 1;
#endif /* VXWORKS */
#endif /*DXS_ENABLE_EXINT*/

         DXS_IrqUnlockDevice (pDev);
      }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
   }
   else
#else /* DXS_HAVE_INTERRUPTS */
   {
      /* interrupt handling not available, but the IRQ number was set,
         print warning and use the polling mode */
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("WARNING: Interrupt mode suport not available, "
             "only polling mode supported.\n"
             "WARNING: Polling mode will be used for for handling events.\n"));
   }
#endif /* DXS_HAVE_INTERRUPTS */
   {
      /* Set flag that events should be retrieved by polling.
         It is first needed in dxs_init_spi() to check
         if resources for power save mode should be acquired. */
      pDev->nEventHandlingMode = DXS_EVENT_POLLING;
      DXS_PollingModeTimerStart();
   }
   /*lint -restore */

   if (ret == IFX_SUCCESS)
   {
      /* Device state: basic init is done */
      pDev->nDevState |= DS_BASIC_INIT;
   }
   else
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("ERROR: Reserving the resources for chip access failed\n"));
   }

   if (ret == IFX_SUCCESS /*lint --e(774)*/)
   {
      /* In IRQ as well as polling mode set the interrupt masks. */
      ret = dxs_DefaultRegInit(pDev);
   }

   if (ret == IFX_SUCCESS)
   {
      /* platform specific DC/DC type autodetection function */
      DXS_TID_DCDC_AUTODETECT_FUNC(pDev, &ret);
      if (ret != IFX_SUCCESS)
      {
         TRACE(DXS, DBG_LEVEL_HIGH,
               ("ERROR: DC/DC type autodetection failed, code 0x%08X\n", ret));
      }
   }

   return ret;
}


/**
   Release the access to the chip.

   This function releases all resources needed for access to the chip.

   \param  pDev         Pointer to the device structure.
*/
static IFX_void_t dxs_ChipAccessExit(DXS_DEVICE_t *pDev)
{
   /* make sure that we really got a device structure */
   DXS_ASSERT((pDev != NULL && pDev->nChannel == 0));

   if (pDev->nDevState & DS_BASIC_INIT)
   {
      /* Device state: basic init is not done */
      pDev->nDevState &= ~DS_BASIC_INIT;

      /* Stop event handling for the device. */
      pDev->nEventHandlingMode = DXS_EVENT_STOP;
      /* free the interrupt */
      if (pDev->pIrq != IFX_NULL)
      {
#ifdef IRQLINE_EXIT
         /* Disable the interrupt line on the Port and IRQ controller. */
         IRQLINE_EXIT(pDev)
#endif /*DXS_ENABLE_EXINT*/
#ifdef DXS_HAVE_INTERRUPTS
         OS_UnInstall_DxsIRQHandler (pDev);
#endif /* DXS_HAVE_INTERRUPTS */
      }
   }

      /* delete the mailbox protection semaphore */
      DXS_OS_MutexDelete (&pDev->mtxMbxAcc);

      /* Release the  SPI access to the device. */
      pDev->nDevState &= ~DS_SPI_ACTIVE;
      dxs_exit_spi (pDev);
      DXS_OS_MutexDelete (&pDev->mtxSpiAcc);
      DXS_OS_MutexDelete (&pDev->mtxFwDlAcc);
   }


/**
   Prepare the low level device struct.

   This function clears the device struct, sets the device number and links it
   with the high-level device.

   \param  pTapiDev     Pointer to the high-level device struct.
   \param  devNum       Device number.

   \return
   Pointer to DXS Device or IFX_NULL.
*/
static IFX_TAPI_LL_DEV_t* DXS_TAPI_LL_DevicePrepare(TAPI_DEV *pTapiDev,
                                                    IFX_uint32_t devNum)
{
   DXS_DEVICE_t* pDev;

   /* make sure a valid context is given */
   if (devNum >= DXS_MAX_DEVICES)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS_LL_Prepare_Dev: DXS device number out of range\n"));
      return IFX_NULL;
   }
   if (pTapiDev == IFX_NULL)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS_LL_Prepare_Dev: pTapiDev is NULL\n"));
      return IFX_NULL;
   }

   pDev = &DxsDevices[devNum];

   /* Clear the device struct (including the channels) */
   memset(pDev, 0, sizeof(DXS_DEVICE_t));

   pDev->nDevNr = devNum;

   /* Store the corresponding HL pointer */
   pDev->pTapiDev = pTapiDev;

   /* Event handling is stopped until it is configured. */
   pDev->nEventHandlingMode = DXS_EVENT_STOP;

   /* The version command is special as it can be used before the device is
      initialised. */
   memset (&pDev->fw_vers, 0, sizeof(pDev->fw_vers));
   pDev->fw_vers.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_vers.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_vers.ECMD = DXS_FW_SYS_VERS_ECMD;
   pDev->fw_vers.LENGTH = DXS_FW_SYS_VERS_LENGTH;

   dxs_AddCaps (pDev);

   /* Return the pDev pointer which is stored in the HL device */
   return pDev;
}


/**
   Initialise the low level device struct.

   Initialise the member variables of the device structure.

   \param  pLLDev       Pointer to the device structure.

   \return
   - DXS_statusOk       If successful.
   - DXS_statusIfxError In case of an error.
*/
static IFX_int32_t DXS_TAPI_LL_DeviceInit(IFX_TAPI_LL_DEV_t* pLLDev)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)pLLDev;

   /* OS and board independent initializations, resets all values */

   /* intially there is no error */
   pDev->nErr = DXS_statusOk;

   /* initialize share variables protection semaphore */
   DXS_OS_MutexInit(&pDev->mtxMemberAcc);

   /* Easy to check flag for checking dedicated / combined DC/DC converter. */
   if (nAllowedDcDcType == DXS_DCDC_TYPE_CIBB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CBB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIFB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIBGD ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIBVD)
   {
      pDev->bDcDcHwCombined = IFX_TRUE;
   }
   else
   {
      pDev->bDcDcHwCombined = IFX_FALSE;
   }

#ifdef DXS_FEAT_GPIO_CTRL
   /* Initially no GPIOs are reserved for DC/DC */
   pDev->pDcDcGpioReserve = IFX_NULL;

   /* create GPIO control struct */
   DXS_GpioInit ((IFX_void_t *)pDev);
#endif /* DXS_FEAT_GPIO_CTRL */

#ifdef EVALUATION
   /* initialize intRead procfile system semaphore in locked state */
   DXS_OS_MutexInit(&pDev->mtxProcIrq);
   DXS_OS_MutexGet(&pDev->mtxProcIrq);
   pDev->fifoProcIrq = fifoInit(32, sizeof(struct DXS_EVAL_EVT_FIFO_ELEM));
   if (!pDev->fifoProcIrq)
   {
      return DXS_statusIfxError;
   }
#endif /* EVALUATION */

   memset (&pDev->fw_caps, 0, sizeof(pDev->fw_caps));
   pDev->fw_caps.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_caps.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_caps.ECMD = DXS_FW_SYS_CAPS_ECMD;
   pDev->fw_caps.LENGTH =  DXS_FW_SYS_CAPS_LENGTH;

   memset (&pDev->fw_sys_ctrl, 0, sizeof(pDev->fw_sys_ctrl));
   pDev->fw_sys_ctrl.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_sys_ctrl.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_sys_ctrl.ECMD = DXS_FW_SYS_Control_ECMD;
   pDev->fw_sys_ctrl.LENGTH = DXS_FW_SYS_Control_LENGTH;

   return DXS_statusOk;
}


/**
   Frees all resources of the device.

   This calls also exit on all channels because they are members of the device.
   Called when the device is released.

   \param  pDev         Pointer to the device structure.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t dxs_DeviceExit(DXS_DEVICE_t *pDev,
                                 IFX_boolean_t bChipAccess)
{
   IFX_uint16_t i;

   /* call exit on all channels of the device */
   for (i = 0; i < DXS_MAX_CH_NR; i++)
   {
      dxs_ChannelExit(&pDev->pChannel[i], bChipAccess);
   }

   if (bChipAccess != IFX_FALSE)
   {
      /* The PCM interface is device global and can only be stopped after all
         PCM channels are deactivated. */
      DXS_PCM_IF_Stop(pDev);
   }

   /* reset PCM timeslot management flags */
   memset(pDev->PcmRxTs, 0, sizeof(pDev->PcmRxTs));
   memset(pDev->PcmTxTs, 0, sizeof(pDev->PcmTxTs));

#ifdef DXS_FEAT_GPIO_CTRL
   /* free GPIO control struct */
   DXS_GpioReset ((IFX_void_t *) pDev);
#endif /* DXS_FEAT_GPIO_CTRL */

   /* Fill the capabilities again with initial values.
      To have capabilities available during cleanup this is done last. */
   pDev->bCapsRead = IFX_FALSE;
   dxs_AddCaps (pDev);

#ifdef EVALUATION
   if (pDev->fifoProcIrq != IFX_NULL)
   {
      struct DXS_EVAL_EVT_FIFO_ELEM fifo_elem;

      while (fifoGet(pDev->fifoProcIrq, &fifo_elem) == IFX_SUCCESS)
      {
         DXS_OS_FREE(fifo_elem.pEvent);
      }
      fifoFree(pDev->fifoProcIrq);
      pDev->fifoProcIrq = IFX_NULL;
   }

   DXS_OS_MutexDelete (&pDev->mtxProcIrq);
#endif /* EVALUATION */

   /* delete device mutex */
   DXS_OS_MutexDelete (&pDev->mtxMemberAcc);

   /* release chip access */
   dxs_ChipAccessExit(pDev);

   /* reset the states variable */
   pDev->nDevState = 0;
}


/**
   Stop the DXS device and free all allocated resources.

   \param  pLLDev       Pointer to the device structure.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t DXS_TAPI_LL_DeviceExit (IFX_TAPI_LL_DEV_t *pLLDev,
                                          IFX_boolean_t bChipAccess)
{
   DXS_DEVICE_t  *pDev = (DXS_DEVICE_t *)pLLDev;

   /* resource cleanup */
   dxs_DeviceExit(pDev, bChipAccess);
}


/**
   Prepare the low level channel struct.

   \param  pTapiCh      High level channel pointer.
   \param  pLLDev       Pointer to the device structure.
   \param  chNum        Channel number.

   \return
   Pointer to DXS Channel or IFX_NULL.
*/
static IFX_TAPI_LL_CH_t* DXS_TAPI_LL_ChannelPrepare(TAPI_CHANNEL *pTapiCh,
                                                    IFX_TAPI_LL_DEV_t *pLLDev,
                                                    IFX_uint32_t chNum)
{
   DXS_DEVICE_t  *pDev = (DXS_DEVICE_t *)pLLDev;
   DXS_CHANNEL_t *pCh;

   DXS_ASSERT(pDev != IFX_NULL);

   /* Make sure we have a DXS channel that we can bind to the TAPI channel. */
   if (chNum >= DXS_MAX_CH_NR)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("No DXS channel available to bind to TAPI channel %d\n", chNum));
      return IFX_NULL;
   }
   if (pTapiCh == IFX_NULL)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS_TAPI_LL_ChannelPrepare: pTapiCh is NULL\n"));
      return IFX_NULL;
   }

   /* TAPI channel number is identical to the channel index in this driver. */
   pCh  = &(pDev->pChannel[chNum]);

   pCh->nChannel = (IFX_uint8_t)chNum + 1;
   pCh->pParent  = pDev;

   /* Store the corresponding HL TAPI channel pointer */
   pCh->pTapiCh = pTapiCh;

   /* Should return the pCh which should be stored by the HL */
   return pCh;
}


/**
   Initialise the low level channel struct.

   \param  pLLDev       Pointer to the channel structure.

   \return
   IFX_SUCCESS
*/
static IFX_int32_t DXS_TAPI_LL_ChannelInit (IFX_TAPI_LL_CH_t *pLLCh)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)pLLCh;

   DXS_ASSERT(pCh != IFX_NULL);

   /* init channel lock */
   DXS_OS_MutexInit (&pCh->mtxChAcc);

   return IFX_SUCCESS;
}


/**
   Frees all resources of the channel.

   Called when the channel is released.

   \param  pCh          Pointer to the channel.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t dxs_ChannelExit(DXS_CHANNEL_t *pCh,
                                  IFX_boolean_t bChipAccess)
{
   /* Deactivate all still running algorithms in the FW */
   DXS_ALM_ChStop(pCh, bChipAccess);

   if (bChipAccess != IFX_FALSE)
   {
      DXS_PCM_ChStop(pCh);
   }

   /* delete channel mutex */
   DXS_OS_MutexDelete (&pCh->mtxChAcc);

   /* free the modules */
   DXS_ALM_Free_Ch_Structures (pCh);
   DXS_PCM_Free_Ch_Structures (pCh);
   DXS_DTMF_AT_Free_Ch_Structures(pCh);
#ifdef DXS_FEAT_CID
   DXS_CID_Free_Ch_Structures(pCh);
#endif /* DXS_FEAT_CID */
#ifdef EVALUATION
   DXS_Eval_Evt_Free_Ch_Structures(pCh);
#endif /* EVALUATION */
}


/**
   Firmware start function.

   \param  pLLDev       Pointer to the device structure.
   \param  pProc        Pointer to low-level device initialization structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusParam          no valid pointer to initialization provided
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
*/
static IFX_int32_t DXS_TAPI_LL_FW_Start(IFX_TAPI_LL_DEV_t *pLLDev,
                                        IFX_void_t const *pProc)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t*)pLLDev;
   DXS_IO_Init_t IoInit;
   IFX_int32_t ret = DXS_statusOk;

   /* blocked until the chip access is initialised */
   if (!(pDev->nDevState & DS_BASIC_INIT))
   {
      /* errmsg: Chip access is not setup. */
      RETURN_DEVSTATUS (DXS_statusChipAccNotSetup, IFX_NULL);
   }

   /* Do not start the firmware again after it was initialised. */
   if (pDev->nDevState & DS_DEV_INIT)
   {
      return DXS_statusOk;
   }

   /* check if io init ptr valid */
   if (pProc == IFX_NULL)
   {
      /* errmsg: At least one parameter is wrong. */
      RETURN_DEVSTATUS (DXS_statusParam, IFX_NULL);
   }
   else
   {
      /* The init struct is specific for the LL driver and only known here.
         Because HL does not know the struct it cannot copy it and so it is
         copied here in the LL driver. */
      DXS_OS_CpyUsr2Kern(&IoInit, pProc, sizeof (IoInit));
   }

   /* Call the chip specific part */
   ret = DXS_FW_Start(pDev, &IoInit);

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Download and start the firmware of the DUSLIC XS chip.

   \param  pDev         Pointer to the device structure.
   \param  pInit        Pointer to the initialization structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
*/
IFX_int32_t DXS_FW_Start(DXS_DEVICE_t *pDev, DXS_IO_Init_t *pInit)
{
   DXS_FW_Download_t edspDwld;
   IFX_int32_t ret = DXS_statusOk;

   /* Setting the cached mailbox length to zero will force a read from chip
      register upon the next access. */
   pDev->nMbxCachedCbiLen = 0;

   /* Check success of the download by reading the boot info register. */
   if (ret == DXS_statusOk)
   {
      if (DXS_Wait4BootFinished(pDev, 10) != DXS_statusOk)
      {
         /* errmsg: Firmware download timeout. */
         ret = DXS_statusFwDwldTimeout;
      }

#ifdef EVALUATION
      ret = DXS_Eval_InitChip(pDev, nDxsRegBootInfo);
#endif /* #ifdef EVALUATION */
   }

   if (DXS_statusOk == ret)
   {
      /* set download pointers */
      dxs_DwldPtrSet(pInit, &edspDwld);

      ret = DXS_DwldFirmwareSelect(pDev, &edspDwld);

      /* download firmware */
      if (ret != DXS_statusOk)
      {
         /* errmsg: Firmware download failed. */
         ret = DXS_statusFwDwldFail;
      }

      /* now that downloads are finished, release memory */
      dxs_DwldPtrUnset (&edspDwld);
   }

   /* Update FW version information after FW download */
   if ((ret == DXS_statusOk) && (pDev->nDevState & DS_FW_DLD))
   {
      ret = DXS_ReadFwVersion(pDev);
      if (DXS_statusOk != ret)
      {
         /* errmsg: Device read access failed. */
         ret = DXS_statusReadErr;
      }
      else
      {
         /* Warn if the FW version is older than a defined minimum version */
         if ((pDev->fw_vers.MAJ < DXS_MIN_FW_MAJORSTEP) ||
             ((pDev->fw_vers.MAJ == DXS_MIN_FW_MAJORSTEP) &&
              ((pDev->fw_vers.MIN < DXS_MIN_FW_MINORSTEP) ||
               ((pDev->fw_vers.MIN == DXS_MIN_FW_MINORSTEP) &&
                /* +1 is to fool the compiler not to output a warning
                   when DXS_MIN_FW_VERSIONSTEP==0 */
                ((pDev->fw_vers.HF+1) < (DXS_MIN_FW_VERSIONSTEP+1))))))
         {
            TRACE (DXS, DBG_LEVEL_HIGH,
                   ("\nWARNING: The current FW version %u.%u.%u is older than "
                    "the minimum required FW version %u.%u.%u\n"
                    "         Stability and featureset cannot be guaranteed!\n"
                    "         Continue at your own risk...\n",
                    pDev->fw_vers.MAJ, pDev->fw_vers.MIN, pDev->fw_vers.HF,
                    DXS_MIN_FW_MAJORSTEP,
                    DXS_MIN_FW_MINORSTEP, DXS_MIN_FW_VERSIONSTEP));
         }
      }
   }

   if (ret == DXS_statusOk)
   {
      /*
         set self-clearing mode - interrupts CBI_DATA, EBI_DATA, EVENTS in the
         HOST_INT1 register are cleared-on-read. This reduces communication
         overhead, but requires that all interrupt events have to be processed
         immediately or remembered for later processing in the controller.
      */
      IFX_uint16_t nRegWrite = 0;
      nRegWrite |= (DXS_REG_CFG_SC_MD_SCON | DXS_REG_CFG_8BIT_EN);
      ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegWrite);
   }


   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Firmware initialisation function.

   \param  pLLDev       Pointer to the device structure.
   \param  nMode        Unused - enum from IFX_TAPI_INIT_MODE_t specifying
                        the setup.

   \return
   - DXS_statusOk             if successful
   - DXS_statusParam          no valid pointer to initialization provided
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
   - DXS_statusDtmfRcvInitErr

   \remarks
   Initialization flow of DUSLIC XS :
   - SDD Rev Read, store DevId 3xyz : x=fxs ch y=fxo ch z=PCM i/f,
     store FW Revision
   - set self clearing mode via register HOST_CFG
   - FW Patch download
   - unmask default irqs (ERR, EBO_DATA, ...)
   - SDD Rev Read, check for changes
   - BBD download (SDD Basic Config, SDD Ring Config, SDD Coefs)
   - allocate and init selected FW messages, e.g. set PCM I/F configuration for
     default configuration (if PCM_CH_CFG is issued before PCM_IF_CFG
   - SDD Opmode Disabled (reset anyhow)
   - DUSLIC XS Setup init (automodes)
   - optional unmask further irqs (GndKey on FXS ch, O-Temp on FXS ch)
*/
/*lint -esym(715, nMode) */
static IFX_int32_t DXS_TAPI_LL_FW_Init(IFX_TAPI_LL_DEV_t *pLLDev,
                                       IFX_uint8_t nMode)
{
   DXS_DEVICE_t         *pDev = (DXS_DEVICE_t*)pLLDev;
   IFX_uint8_t          nMaxRes;
   IFX_TAPI_RESOURCE    nResource;
   IFX_int32_t          ret = DXS_statusOk;

   DXS_OS_UNUSED (nMode);

   /* do nothing if device initialization was already done */
   if (pDev->nDevState & DS_DEV_INIT)
   {
      return DXS_statusOk;
   }

   /* blocked until the chip access is initialised */
   if (!(pDev->nDevState & DS_BASIC_INIT))
   {
      /* errmsg: Chip access is not setup. */
      RETURN_DEVSTATUS (DXS_statusChipAccNotSetup, IFX_NULL);
   }

   /* Fill the internal capabilities struct. */
   ret = dxs_SetDevCaps (pDev);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   TRACE (DXS, DBG_LEVEL_LOW, ("\n"));
   TRACE (DXS, DBG_LEVEL_LOW,
          ("FWCAP: nPCM:%2u  nALM:%2u\n",
           pDev->caps.nPCM, pDev->caps.nALI));
   TRACE (DXS, DBG_LEVEL_LOW,
          ("FWCAP: nDTMFR:%2u nDTMFG:%2u nCIDS:%2u\n",
           pDev->caps.nDTMFR, pDev->caps.nDTMFG, pDev->caps.nCIDS));

   /* Maximum number of resources, is the maximum of: nALI, nPCM. */
   nMaxRes = (pDev->caps.nPCM > pDev->caps.nALI) ?
              pDev->caps.nPCM : pDev->caps.nALI;

   if (nMaxRes > DXS_MAX_CH_NR)
   {
      /* Warn about incorrect definition then limit the counters so that we
         do not crash when trying to initialise non existing resources. */
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("WARNING: Detected maximum resources %d but defined only %d. "
             "Some resources will not be usable\n",
             nMaxRes, DXS_MAX_CH_NR));
      if (pDev->caps.nALI > DXS_MAX_CH_NR)
         pDev->caps.nALI = DXS_MAX_CH_NR;
      if (pDev->caps.nPCM > DXS_MAX_CH_NR)
         pDev->caps.nPCM = DXS_MAX_CH_NR;
   }

   /* report the resource counts to HL TAPI */
   memset (&nResource, 0x00, sizeof(nResource));
   nResource.AlmCount   = pDev->caps.nALI;
   nResource.PcmCount   = pDev->caps.nPCM;
   nResource.DTMFGCount = pDev->caps.nDTMFG;
   nResource.DTMFRCount = pDev->caps.nDTMFR;
   nResource.FSKGCount  = pDev->caps.nCIDS;
   nResource.ToneCount  = pDev->caps.nALI;
   IFX_TAPI_ReportResources (pDev->pTapiDev, &nResource);


   /* set default firmware cache values now, required to allow storage of
      coefficients from BBD Download... */
   ret = dxs_FW_DevDataInit(pDev);

   if (ret == DXS_statusOk)
   {
      ret = dxs_DefaultRegInit(pDev);
      if (ret != DXS_statusOk)
      {
         /* errmsg: Initialzing registers with default values failed. */
         ret = DXS_statusRegInitErr;
      }
   }

   if (ret == DXS_statusOk)
   {
      pDev->nDevState |= DS_FW_DLD;
      pDev->nDevState |= DS_DEV_INIT;
      /* Update the list now that we consolidated the chip capabilities. */
      ret = dxs_AddCaps(pDev);
   }

   RETURN_DEVSTATUS (ret, IFX_NULL);
}
/*lint +esym(715, nMode) */


/**
   Download a BBD file.

   \param  pLLDev       Pointer to the device structure.
   \param  pProc        Pointer to low-level device initialization structure.

   \return
   - DXS_statusOk if successful
*/
static IFX_int32_t DXS_TAPI_LL_BBD_Dnld(IFX_TAPI_LL_DEV_t *pLLDev,
                                        IFX_void_t const *pProc)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)pLLDev;
   DXS_IO_Init_t IoInit;
   DXS_BBD_Download_t bbdDwld;
   IFX_int32_t ret = DXS_statusOk;

   /* check if io init ptr valid */
   if (pProc == IFX_NULL)
   {
      /* errmsg: At least one parameter is wrong. */
      RETURN_DEVSTATUS (DXS_statusParam, IFX_NULL);
   }
   else
   {
      /* The init struct is specific for the LL driver and only known here.
         Because HL does not know the struct it cannot copy it and so it is
         copied here in the LL driver. */
      DXS_OS_CpyUsr2Kern(&IoInit, pProc, sizeof (IoInit));
   }

   /* BBD Download */
   if (IoInit.pBBDbuf != IFX_NULL)
   {
      bbdDwld.buf  = IoInit.pBBDbuf;
      bbdDwld.size = IoInit.bbd_size;
      ret = DXS_BBD_Download((DXS_CHANNEL_t *)(IFX_void_t *)pDev,
                              &bbdDwld);
      if (!DXS_SUCCESS (ret))
      {
         /* errmsg: BBD download failed. */
         ret = DXS_statusBbdErr;
      }
   }
   else
   {
      TRACE(DXS, DBG_LEVEL_HIGH, ("INFO: No BBD Buffer provided.\n"));
   }

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Maps the FW patch binary into memory usable by this driver

   This function reserves dynamic memory for download data and copies data
   from user space.

   \param  pInit        Pointer to DXS_IO_Init_t with user download data.
   \param  pEdspDwld    Pointer to struct that is filled with the pointers
                        to the mapped FW binary.
*/
static IFX_void_t dxs_DwldPtrSet (DXS_IO_Init_t *pInit,
                                  DXS_FW_Download_t *pEdspDwld)
{
   /* do some initializations ... */
   memset (pEdspDwld, 0, sizeof (*pEdspDwld));
   pEdspDwld->pPRAMfw = IFX_NULL;
   pEdspDwld->nEdspFlags = DXS_NO_FW_DWLD;

   if (pInit->pPRAMfw != IFX_NULL)
   {
      pEdspDwld->pPRAMfw = OS_MapBuffer(pInit->pPRAMfw, pInit->pram_size);
      if (pEdspDwld->pPRAMfw != NULL)
      {
         pEdspDwld->pram_size = pInit->pram_size;
         pEdspDwld->nEdspFlags = pInit->nFlags & DXS_NO_FW_DWLD;
      }
   }
   else
   {
      /* User didn't provide FW patch binary. */
      TRACE(DXS, DBG_LEVEL_LOW, ("DXS_INFO: No firmware binaries for PRAM => "
                                 "default ROM firmware will be used\n"));
   }
}


/**
   Releases the download data pointers

   This function frees dynamix memory with download data.

   \param  pEdspDwld    Pointer to struct that is filled with the pointers
                        to the mapped FW binary.
*/
static IFX_void_t dxs_DwldPtrUnset (DXS_FW_Download_t *pEdspDwld)
{
   if (pEdspDwld->pPRAMfw)
   {
      OS_UnmapBuffer(pEdspDwld->pPRAMfw);
   }
   memset (pEdspDwld, 0, sizeof(*pEdspDwld));
}


/**
   Write info for bootloader, how to boot.

   \param  pDev         Pointer to the device structure.
   \param  bootInfo     Enum describing the boot mode.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
IFX_int32_t DXS_SetBootConfig (DXS_DEVICE_t *pDev, DXS_BOOT_INFO_t bootInfo)
{
   IFX_int32_t    ret = DXS_statusOk;
   IFX_uint16_t   nHostReg = 0;

#ifdef DXS_EVAL_SET_BOOT_CONFIG
   /* for evaluation of unfused devices do some additional settings */
   DXS_EVAL_SET_BOOT_CONFIG(pDev);
#endif /* #ifdef DXS_EVAL_SET_BOOT_CONFIG */

   switch (bootInfo)
   {
      case DXS_BOOT_ROM:
         nHostReg = DXS_REG_BCFG_ASC_ROM;
         break;
      case DXS_BOOT_SPI:
         nHostReg = DXS_REG_BCFG_ASC_SPI;
         break;
      case DXS_BOOT_UART:
      default:
         /* do nothing */
         break;
   }
   /* write the boot info to boot from SPI */
   ret = DXS_RegWrite(pDev, DXS_HOST_BCFG, nHostReg);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Initiate a controller reset with the selected bootmode.

   \param  pDev         Pointer to the device structure.
   \param  bootInfo     Enum describing the boot mode.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
IFX_int32_t DXS_ResetController (DXS_DEVICE_t *pDev, DXS_BOOT_INFO_t bootInfo)
{
   IFX_int32_t      ret = DXS_statusOk;
   IFX_uint16_t     nRegHostCfg = 0x0;
   IFX_int32_t      nCh = 0;
   DXS_SDD_Opmode_t pOpmode;
   memset(&pOpmode, 0, sizeof(DXS_SDD_Opmode_t));

   /* Channels need to be disabled before software reset */
   pOpmode.CMD         = DXS_CMD_CMD_SDD;
   pOpmode.MOD         = DXS_CMD_MOD_SDD;
   pOpmode.ECMD        = DXS_SDD_Opmode_ECMD;
   pOpmode.LENGTH      = DXS_SDD_Opmode_LENGTH;
   pOpmode.OpMode      = DXS_SDD_Opmode_Disabled;
   for(nCh = 0; nCh < DXS_MAX_CH_NR; nCh++)
   {
      pOpmode.CHAN = nCh;
      ret = DXS_CmdWrite(pDev, (IFX_uint32_t *) &pOpmode);
      if (ret != DXS_statusOk)
         RETURN_DEVSTATUS(ret, IFX_NULL);
   }

   DXS_OS_MSecSleep(100);

   /* Set the boot mode. */
   ret = DXS_SetBootConfig(pDev, bootInfo);
   if (ret != DXS_statusOk)
   {
      /* errmsg: Setting of boot configuration register failed. */
      RETURN_DEVSTATUS(DXS_statusSetBootCfgErr, IFX_NULL);
   }

   /* Reset flag until alive event is reported. */
   pDev->nDevState &= ~DS_DEV_UP;

   /* Initiate the controller reset. */
   nRegHostCfg |= (DXS_REG_CFG_RST_RSTCORE | DXS_REG_CFG_8BIT_EN);
   ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegHostCfg);

   if (ret != DXS_statusOk)
      RETURN_DEVSTATUS(ret, IFX_NULL);

   /* flush command mailbox fifo */
   fifo_flush (pDev->cmd_obx_queue);

   switch (bootInfo)
   {
      case DXS_BOOT_ROM:
         if (DXS_Wait4BootFinished(pDev, 500) != DXS_statusOk)
         {
            /* errmsg: Chip boot failed. */
            ret = DXS_statusBootFailed;
         }
         break;

      case DXS_BOOT_SPI:
         /* do nothing, first the binary must be downloaded */
         break;

      default:
         /* do nothing */
         break;
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Wait until the Boot Info Register indicates that boot has finished.

   Poll the Boot Info Register until a value of at least 0x10 is indicated.

   \param  pDev         Pointer to the device structure.
   \param  loop         Repetitions of the poll loop.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t DXS_Wait4BootFinished (
                        DXS_DEVICE_t *pDev,
                        IFX_uint16_t nLoop)
{
   IFX_int32_t    ret = DXS_statusOk;
   IFX_uint16_t   nRegBootInfo = 0x0;

   while (nLoop > 0)
   {
      /* read the boot state indication */
      ret = DXS_RegRead(pDev, DXS_HOST_BINF, &nRegBootInfo);

      /*  A value equal or greater than 0x10 signals that the boot sequence
          has finished successfully and the main routine has started. */
      if ((ret == DXS_statusOk) &&
          ((nRegBootInfo & DXS_REG_BINF_BOOTSTATE_MASK) >= 0x10))
      {
         return DXS_statusOk;
      }

      /*lint -save -e(62) 'udelay' incompatible types for operator ":" */
      /* wait 2ms between register accesses */
      DXS_OS_MSecSleep(2);
      /* lint -restore */

      nLoop--;
   }

   /* errmsg: Chip boot failed. */
   return DXS_statusBootFailed;
}


/**
   Default initialization of DUSLIC XS registers.

   Enable the required interrupts in the register interface.

   \param pDev pointer to the device structure

   \return
   - DXS_statusOk
   - DXS_statusSpiAccErr
*/
extern IFX_int32_t dxs_DefaultRegInit(DXS_DEVICE_t *pDev)
{
   IFX_int32_t       ret = DXS_statusOk;
   IFX_uint16_t      nRegHostIen1, nRegHostIen2;

   /*
      Enable the interrupts OBX_RDY and ERR.
   */
   nRegHostIen1 = DXS_REG_IEN1_RESET;
   nRegHostIen1 |= DXS_REG_IEN1_OBX_RDY;
   nRegHostIen1 |= DXS_REG_IEN1_ERR;
   ret = DXS_RegWrite(pDev, DXS_HOST_IEN1, nRegHostIen1);

   if (DXS_statusOk != ret)
      RETURN_DEVSTATUS(ret, IFX_NULL);

   /*
      Enable the interrupts OBX_UFL and IBX_OFL.
   */
   nRegHostIen2 = DXS_REG_IEN2_RESET;
   nRegHostIen2 |= DXS_REG_IEN2_OBX_UFL;
   nRegHostIen2 |= DXS_REG_IEN2_IBX_OFL;
   ret = DXS_RegWrite(pDev, DXS_HOST_IEN2, nRegHostIen2);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Create and initalise the structs handling the firmware modules.

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_statusOk
   - DXS_statusInitFwMsgErr - if FW messages could not be initialized
*/
static IFX_int32_t dxs_FW_DevDataInit(DXS_DEVICE_t *pDev)
{
   IFX_uint8_t i;
   IFX_int32_t ret = DXS_statusOk;
   DXS_FW_SYS_Control_t *pFW_SysControl;

#ifdef EVALUATION
   ret = DXS_Eval_InitDevFWData (pDev);
   if (!DXS_SUCCESS (ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);
#endif /* #ifdef EVALUATION */

   /* System Control is the first message to the FW. */
   pFW_SysControl = &pDev->fw_sys_ctrl;
#ifdef DXS_FEAT_POWER_SAVE
   if (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT)
   {
      pFW_SysControl->SE   = IFX_ENABLE;
   }
   else
   {
      pFW_SysControl->SE   = IFX_DISABLE;
   }
#else /* DXS_FEAT_POWER_SAVE */
   pFW_SysControl->SE      = IFX_DISABLE;
#endif /* DXS_FEAT_POWER_SAVE */
   pFW_SysControl->CP_EN   = IFX_DISABLE;
   pFW_SysControl->CP_VOLT = DXS_FW_SYS_Control_CP_VOLT_5_0V;
#ifdef REALTEK_PATCH_FOR_INTEL
//For power loss, otherwise SLIC is too sensitive to the master pcm clock
   pFW_SysControl->PLIM    = IFX_DISABLE;
#else
   pFW_SysControl->PLIM    = IFX_ENABLE;
#endif
   pFW_SysControl->SYCLKE  = IFX_ENABLE;

   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pFW_SysControl);
   if (!DXS_SUCCESS (ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   /* allocate and initialize ALM FW messages */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nALI); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_ALM_Allocate_Ch_Structures (pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr, IFX_NULL);
      }
      else
      {
         DXS_ALM_InitCh (pCh);
      }
   }

   /* allocate and initialize DTMF/AT FW messages (for dtmf/at generator, DTMF
      dialling and dtmf receiver (if available)) */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nDTMFG); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_DTMF_AT_Allocate_Ch_Structures (pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr, IFX_NULL);
      }
      else
      {
         DXS_DTMF_AT_InitCh (pCh);
      }
   }

#ifdef DXS_FEAT_CID
   /* allocate and initialize CID sender FW messages */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nCIDS); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_CID_Allocate_Ch_Structures(pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr,IFX_NULL);
      }
      else
      {
         DXS_CID_InitCh (pCh);
      }
   }
#endif /* DXS_FEAT_CID */

   /* allocate and initialize PCM FW messages */
   for (i = 0;  (ret == DXS_statusOk) && (i < pDev->caps.nPCM); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_PCM_Allocate_Ch_Structures (pCh);

      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr,IFX_NULL);
      }
      else
      {
         DXS_PCM_InitCh (pCh);
      }
   }

#ifdef DXS_FEAT_UTD
   /* allocate and initialize UTD handling structs */
   for (i = 0;  (ret == DXS_statusOk) && (i < pDev->caps.nUTD); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_UTD_Allocate_Ch_Structures (pCh);

      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr,IFX_NULL);
      }
      else
      {
         DXS_UTD_InitCh (pCh);
      }
   }
#endif /* (DXS_FEAT_UTD) */

#ifdef DXS_FEAT_GPIO_CTRL
   /* The buck-or-boost DC/DC converter uses GPIO 0 & 1 as control signals
      so reserve them here so that they cannot be used via the GPIO API.
      No reconfiguration is possible so only reservation is done here.
      For one channel devices of course only one pin is reserved. */
   if ((ret == DXS_statusOk) &&
       (nAllowedDcDcType == DXS_DCDC_TYPE_BB) &&
       (pDev->pDcDcGpioReserve == IFX_NULL))
   {
      IFX_uint16_t pins = DXS_IO_DEV_GPIO_0;

      if (pDev->caps.nALI == 2)
      {
         /* On 2 channel devices reserve GPIO1 in addition to GPIO0. */
         pins |= DXS_IO_DEV_GPIO_1;
      }

      ret = DXS_GpioReserve(pDev, pins, &pDev->pDcDcGpioReserve);
      if (ret == IFX_SUCCESS)
      {
         DXS_GPIO_Config_t ioCfg;

         memset(&ioCfg, 0, sizeof(ioCfg));
         ioCfg.nMode = DXS_GPIO_MODE_OUTPUT;;
         ioCfg.nGpio = pins;
         DXS_GpioConfig(pDev->pDcDcGpioReserve, &ioCfg);
      }
   }
#endif /* DXS_FEAT_GPIO_CTRL */

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Read the firmware version.

   This function can be called as soon as the device struct is prepared and
   the chip access is set. It will store the results in the device structure.

   \param pDev handle to the device structure

   \return
   - DXS_statusOk
   - DXS_statusCmdIbNoSpace
   - DXS_statusCmdMbWrErr
*/
IFX_int32_t DXS_ReadFwVersion (DXS_DEVICE_t *pDev)
{
   IFX_int32_t ret;

   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)&pDev->fw_vers,
                            (IFX_uint32_t *)(IFX_void_t *)&pDev->fw_vers);

   if (!DXS_SUCCESS(ret))
   {
      /* errmsg: Failed to read version or capability from FW. */
      RETURN_DEVSTATUS(DXS_statusCapVersUpdateError, IFX_NULL);
   }

   /* Silicon revision are the lower 5 bits of the Device-ID */
   pDev->nChipRev    = (IFX_uint8_t) pDev->fw_vers.DEV & 0x1F;
   /* Upper 3 bits of the Device-ID extended by the number of channels will
      identify the chip type. */
   pDev->nDevId      = (IFX_uint16_t)
                       (((pDev->fw_vers.DEV >> 4) & 0xE) | pDev->fw_vers.CH);
   /* FW version number as 32-bit value - one digit per byte */
   pDev->nFwRev      = (pDev->fw_vers.MAJ << 24) |
                       (pDev->fw_vers.MIN << 16) |
                       (pDev->fw_vers.HF  <<  8);

   return DXS_statusOk;
}


/**
   Read the revision information and capabilities via firmware message.

   \param  pDev         Pointer to the device structure.

   \return
   DXS_statusOk or error code on command read error.
*/
static IFX_int32_t dxs_SetDevCaps (DXS_DEVICE_t *pDev)
{
   IFX_int32_t err = DXS_statusOk;

   err = DXS_ReadFwVersion(pDev);
   if (!DXS_SUCCESS(err))
      RETURN_DEVSTATUS(err, IFX_NULL);

   err = DXS_CmdRead(pDev,
                     (uint32_t *)&pDev->fw_caps, (uint32_t *)&pDev->fw_caps);
   if (DXS_statusOk != err)
      RETURN_DEVSTATUS(DXS_statusCapVersUpdateError, IFX_NULL);

   /* store FW capabilities */
   pDev->caps.bfw_MWI       = IFX_TRUE;
   pDev->caps.bfw_ASPwrSave = IFX_TRUE;
   pDev->caps.bfw_GR909     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_CMeas     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_ExtLT     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_combDcDc  = IFX_TRUE;
   pDev->caps.bfw_WbTsSplit = IFX_TRUE;
   pDev->caps.bfw_BuckBoost = IFX_TRUE;
   pDev->caps.bfw_UTD       = IFX_TRUE;

   /* Set capabilities which depend on the device id. */
   if ((pDev->nDevId & 0x01) == 0)
   {
      /* 2-channel device */
      pDev->caps.nALI      = 2;
      pDev->caps.nPCM      = 2;
      pDev->caps.nDTMFG    = 2;
      pDev->caps.nCIDS     = 2;
      pDev->caps.nDTMFR    = 2;
      pDev->caps.nUTD      = 2;
   }
   else
   {
      /* 1-channel device */
      pDev->caps.nALI      = 1;
      pDev->caps.nPCM      = 1;
      pDev->caps.nDTMFG    = 1;
      pDev->caps.nCIDS     = 1;
      pDev->caps.nDTMFR    = 1;
      pDev->caps.nUTD      = 1;
   }

   /* If capability for UTD is not set overwrite the previously set value. */
   if (!(pDev->caps.bfw_UTD))
      pDev->caps.nUTD         = 0;

   /* Set flag that the capabilities struct is now filled with values. */
   pDev->bCapsRead = IFX_TRUE;

   TRACE(DXS, DBG_LEVEL_LOW,
         ("FW rev %u.%u.%u Capabilities: MWI %d, AS %d, "
          "CMeas %d, ExtLT %d, CombDcDc %d, WbTsSplit %d Buck/Boost %d "
          "UTD %d GR909 %d\n",
         pDev->fw_vers.MAJ, pDev->fw_vers.MIN, pDev->fw_vers.HF,
         pDev->caps.bfw_MWI, pDev->caps.bfw_ASPwrSave,
         pDev->caps.bfw_CMeas, pDev->caps.bfw_ExtLT,
         pDev->caps.bfw_combDcDc, pDev->caps.bfw_WbTsSplit,
         pDev->caps.bfw_BuckBoost, pDev->caps.bfw_UTD,
         pDev->caps.bfw_GR909));

   return DXS_statusOk;
}


/**
   Set all capabilities.

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    no memory could be allocated for capability list

   \remarks
   Macro DXS_MAX_CAPS must match with the capabilities number. So adapt this
   macro accordingly if new capabilities are added.
*/
static IFX_int32_t dxs_AddCaps (DXS_DEVICE_t *pDev)
{
   /* capability list */
   IFX_TAPI_CAP_t *CapList;

   /* count the number of entries */
   IFX_uint32_t nCap = 0;

   if (pDev->CapList == NULL)
   {
      pDev->CapList = (IFX_TAPI_CAP_t*)
                      DXS_OS_MALLOC(DXS_MAX_CAPS * sizeof(IFX_TAPI_CAP_t));
      if (pDev->CapList == NULL)
      {
         /* errmsg: No memory could be allocated */
         RETURN_DEVSTATUS (DXS_statusNoMem, IFX_NULL);
      }
   }

   CapList = pDev->CapList;

   AddCapability (CapList, &nCap, "INTEL",
                  IFX_TAPI_CAP_TYPE_VENDOR,0);
   AddCapability (CapList, &nCap, "DUSLIC XS",
                  IFX_TAPI_CAP_TYPE_DEVICE, 0);
   AddCapability (CapList, &nCap, "POTS",
                  IFX_TAPI_CAP_TYPE_PORT, IFX_TAPI_CAP_PORT_POTS);
   AddCapability (CapList, &nCap, "PSTN",
                  IFX_TAPI_CAP_TYPE_PORT, IFX_TAPI_CAP_PORT_PSTN);
   AddCapability (CapList, &nCap, "DEVICE TYPE",
                  IFX_TAPI_CAP_TYPE_DEVTYPE, DXS_DEV_TYPE);
   AddCapability (CapList, &nCap, "u-LAW",
                  IFX_TAPI_CAP_TYPE_CODEC, IFX_TAPI_ENC_TYPE_MLAW);
   AddCapability (CapList, &nCap, "A-LAW",
                  IFX_TAPI_CAP_TYPE_CODEC, IFX_TAPI_ENC_TYPE_ALAW);

   if (pDev->nDevState & DS_DEV_INIT)
   {
      switch (pDev->nChipRev)
      {
         case DXS_V11:
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0101);
            break;

         case DXS1_V12:
            /*lint -fallthrough */
         case DXS2_V12:
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0102);
            break;

         default:
            /* unknown */
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0000);
            break;
      }
   }

   if (pDev->bCapsRead)
   {
      AddCapability (CapList, &nCap, "PCM",
                     IFX_TAPI_CAP_TYPE_PCM, pDev->caps.nPCM);
      AddCapability (CapList, &nCap, "Phones",
                     IFX_TAPI_CAP_TYPE_PHONES, pDev->caps.nALI);
   }
   else
   {
      AddCapability (CapList, &nCap, "PCM",
                     IFX_TAPI_CAP_TYPE_PCM, 2);
      AddCapability (CapList, &nCap, "Phones",
                     IFX_TAPI_CAP_TYPE_PHONES, 2);
   }

   /* check if the array is not out of its limit */
   DXS_ASSERT (nCap <= DXS_MAX_CAPS);
   pDev->nMaxCaps = (IFX_uint8_t)nCap;

   return DXS_statusOk;
}


/**
   Add capability to given list.

   Writes a capability to the capability list entry that is adressed with
   nCap and increments nCap afterwards. Previous data in this slot is
   overwritten.

   \param  CapList      Pointer to the capability list.
   \param  pnCap        Pointer to an integer used as index to the list.
                        The integer is automatically postincremented by one.
   \param  description  C-string with a text describing the capability.
   \param  type         Type ID of the capability
   \param  value        Value (amount) of the capability.
*/
static IFX_void_t AddCapability (IFX_TAPI_CAP_t* CapList,
                                 IFX_uint32_t *pnCap,
                                 IFX_char_t const * description,
                                 IFX_int32_t type,
                                 IFX_int32_t value)
{
   IFX_uint32_t capnr;

   if (pnCap  == IFX_NULL)
      return;

   if (*pnCap >= DXS_MAX_CAPS)
      return;

   capnr = (*pnCap);
   /* Note: strncpy will terminate the target string with \0 if the source
      string is longer than length parameter */
   strncpy (CapList[capnr].desc, description, sizeof (CapList[0].desc));
   CapList[capnr].captype = (IFX_TAPI_CAP_TYPE_t)type;
   CapList[capnr].cap = value;
   CapList[capnr].handle = (IFX_int32_t)capnr;
   (*pnCap) = capnr + 1;
   TRACE (DXS, DBG_LEVEL_LOW, ("Cap: %s , type %d = %d size=%u\n",
          description, type, value, (IFX_uint32_t) strlen(description)));
}


/**
   Returns the number of entries in the capability list.

   \param pLLDev  pointer to LL device structure

   \return The number of capability entries.
*/
static IFX_int32_t  DXS_TAPI_LL_Phone_Get_Capabilities (
                                 IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t *pDev = pLLDev;
   return (pDev->nMaxCaps);
}


/**
   Returns DUSLIC XS's capability lists.

   \param  pLLDev       Pointer to DXS_DEVICE_t structure.
   \param  pCapList     Pointer to IFX_TAPI_CAP_LIST_t structure with details
                        where to copy the data to. No more capabilities than
                        specified in the element nCap will be copied into the
                        memory given in this structure.

   \return
   - DXS_statusOk       Always successful.
*/
static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capability_List (
                        IFX_TAPI_LL_DEV_t *pLLDev,
                        IFX_TAPI_CAP_LIST_t *pCapList)
{
   DXS_DEVICE_t *pDev = pLLDev;

   /* The count of entries given in the parameter is the limit. Should the
      internal list have less entries than this reduce the number of entries
      to be copied to this smaller number. */
   if (pDev->nMaxCaps < pCapList->nCap)
   {
      pCapList->nCap = pDev->nMaxCaps;
   }

   DXS_OS_CpyKern2Usr (
      pCapList->pList,
      pDev->CapList, /*lint !e64 */
      pCapList->nCap * sizeof (*pDev->CapList));

   return DXS_statusOk;
}


/**
   Checks in the capability list if a specific capability is supported.

   \param  pLLDev       Pointer to DXS_DEVICE_t structure
   \param  pCapList     Pointer to IFX_TAPI_CAP_t structure

   \return Support status of the capability
   - 0 if not supported
   - 1 if supported

   \remarks
   This function compares only the captype and the cap members of the given
   IFX_TAPI_CAP_t structure with the ones of DUSLIC XS.
*/
static IFX_int32_t DXS_TAPI_LL_Phone_Check_Capability (
                                 IFX_TAPI_LL_DEV_t *pLLDev,
                                 IFX_TAPI_CAP_t    *pCapList)
{
   DXS_DEVICE_t *pDev = pLLDev;
   IFX_int32_t cnt;
   IFX_int32_t ret = 0;

   /* do checks */
   for (cnt = 0; cnt < pDev->nMaxCaps; cnt++)
   {
      if (pCapList->captype == pDev->CapList[cnt].captype)
      {
         switch (pCapList->captype)
         {
         /* Handle number counters, cap is returned */
         case IFX_TAPI_CAP_TYPE_PCM:
         case IFX_TAPI_CAP_TYPE_CODECS:
         case IFX_TAPI_CAP_TYPE_PHONES:
         case IFX_TAPI_CAP_TYPE_T38:
         case IFX_TAPI_CAP_TYPE_DEVVERS:
         case IFX_TAPI_CAP_TYPE_DEVTYPE:
            pCapList->cap = pDev->CapList[cnt].cap;
            ret = 1;
            break;
         case IFX_TAPI_CAP_TYPE_DEVICE:
         case IFX_TAPI_CAP_TYPE_VENDOR:
            strncpy (pCapList->desc, pDev->CapList[cnt].desc, sizeof (pCapList->desc));
            ret = 1;
            break;
         default:
            /* default IFX_TRUE or IFX_FALSE capabilities */
            if (pCapList->cap == pDev->CapList[cnt].cap)
               ret = 1;
            break;
         }
      }
   }
   return ret;
}


/**
   DUSLIC XS device driver initialization.
   This is the device driver initialization function to call at the system
   startup prior any access to the DUSLIC XS device driver.
   After the initialization the device driver is ready to be accessed by
   the application. The global structure "DXS_dev_ctx" contains all the data
   handling the interface (open, close, ioctl,...).

   \return
   - IFX_SUCCESS        If successful.
   - IFX_ERROR          if registration with HL failed.
*/
IFX_int32_t DXS_DeviceDriverStart(void)
{
   IFX_int32_t          result   = 0;
   IFX_TAPI_DRV_CTX_t   *pDrvCtx = &DrvCtx;

   /* Set the default trace level */
   TRACE_LEVEL_SET(DXS, (*p_dxs_debug_level));

   /* Get the coding of the allowed DC/DC converter type. */
   nAllowedDcDcType = DXS_BBD_DcDcStringTranslate(DXS_DCDC_TYPE);
   if ((dcdc_type[0] != '\0') &&
       (nAllowedDcDcType != DXS_DCDC_TYPE_EVAL) &&
       (nAllowedDcDcType != DXS_DCDC_TYPE_AUTO)
      )
   {
      /* Setting the type on cmdline is not allowed if not compiled for EVAL. */
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS driver start: Setting DC/DC HW type on the cmdline not"
             "allowed when driver is already configured.\n"));
      return DXS_statusDrvInitFail;
   }
   if (nAllowedDcDcType == DXS_DCDC_TYPE_EVAL)
   {
      nAllowedDcDcType = DXS_BBD_DcDcStringTranslate(dcdc_type);
   }
   if (nAllowedDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS driver start: Configuration of DC/DC HW type invalid.\n"));
      return DXS_statusDrvInitFail;
   }

#ifdef DXS_FEAT_UTD
   /* Allocate driver global storage for UTD coefficients. */
   DXS_UTD_AllocateStorage();
#endif /* DXS_FEAT_UTD */

   memset(pDrvCtx, 0, sizeof(*pDrvCtx));

   /* Initialize the function pointers structure and register
      with the High Level TAPI */
#ifdef DXS_LINUX_USER_SPACE
   pDrvCtx->majorNumber       = DXS_MAJOR;
   pDrvCtx->minorBase         = DXS_MINOR_BASE;
#else /* DXS_LINUX_USER_SPACE */
   pDrvCtx->majorNumber       = major;
   pDrvCtx->minorBase         = minorBase;
#endif /* DXS_LINUX_USER_SPACE */

   pDrvCtx->devNodeName       = "dxs"; /* devName; */
   pDrvCtx->maxDevs           = DXS_MAX_DEVICES;
   pDrvCtx->maxChannels       = DXS_MAX_CH_NR;

   /* procfs info */
   pDrvCtx->drvName           = DEV_NAME;
   pDrvCtx->drvVersion        = DRV_DXS_VER_STR;
   pDrvCtx->hlLLInterfaceVersion = DRV_LL_INTERFACE_VER_STR;

   /* Generic functions  */
   pDrvCtx->Prepare_Dev       = DXS_TAPI_LL_DevicePrepare;
   pDrvCtx->Init_Dev          = DXS_TAPI_LL_DeviceInit;
   pDrvCtx->Exit_Dev          = DXS_TAPI_LL_DeviceExit;

   pDrvCtx->Prepare_Ch        = DXS_TAPI_LL_ChannelPrepare;
   pDrvCtx->Init_Ch           = DXS_TAPI_LL_ChannelInit;

   pDrvCtx->FW_Start          = DXS_TAPI_LL_FW_Start;
   pDrvCtx->FW_Init           = DXS_TAPI_LL_FW_Init;
   pDrvCtx->BBD_Dnld          = DXS_TAPI_LL_BBD_Dnld;

   pDrvCtx->Ioctl             = DXS_Dev_Spec_Ioctl;

#if 0
   pDrvCtx->GetCmdMbxSize     = DXS_TAPI_LL_GetCmdMbxSize;
#endif

   pDrvCtx->CAP_Number_Get    = DXS_TAPI_LL_Phone_Get_Capabilities;
   pDrvCtx->CAP_List_Get      = DXS_TAPI_LL_Phone_Get_Capability_List;
   pDrvCtx->CAP_Check         = DXS_TAPI_LL_Phone_Check_Capability;

#ifdef DXS_HAVE_INTERRUPTS
   /* IRQ information */
   pDrvCtx->IRQ.LockDevice    = DXS_IrqLockDevice;
   pDrvCtx->IRQ.UnlockDevice  = DXS_IrqUnlockDevice;
   pDrvCtx->IRQ.IrqEnable     = DXS_IrqEnable;
   pDrvCtx->IRQ.IrqDisable    = DXS_IrqDisable;
#else /* DXS_HAVE_INTERRUPTS */
   pDrvCtx->IRQ.LockDevice    = IFX_NULL;
   pDrvCtx->IRQ.UnlockDevice  = IFX_NULL;
   pDrvCtx->IRQ.IrqEnable     = IFX_NULL;
   pDrvCtx->IRQ.IrqDisable    = IFX_NULL;
#endif /* DXS_HAVE_INTERRUPTS */
   /* PCM related */
   DXS_PCM_Func_Register (&pDrvCtx->PCM);

   /* ALM specific */
   DXS_ALM_Func_Register (&pDrvCtx->ALM);
#ifdef DXS_FEAT_NLT
   /* NLT specific */
   DXS_NLT_Func_Register (&pDrvCtx->NLT);
#endif
#ifdef DXS_FEAT_CID
   pDrvCtx->SIG.CID_TX_Start  = DXS_TAPI_LL_CID_TX_Start;
   pDrvCtx->SIG.CID_TX_Stop   = DXS_TAPI_LL_CID_TX_Stop;
#endif /* DXS_FEAT_CID */

   /* DTMFG for FXO dialling support */
   pDrvCtx->SIG.DTMFG_Cfg     = DXS_TAPI_LL_DTMFG_Cfg;
   pDrvCtx->SIG.DTMFG_Start   = DXS_TAPI_LL_DTMFG_Start;
   pDrvCtx->SIG.DTMFG_Stop    = DXS_TAPI_LL_DTMFG_Stop;

   /* DTMF receiver, always running if supported */
   pDrvCtx->SIG.DTMF_RxCoeff  = DXS_TAPI_LL_DTMF_RX_CFG;

#ifdef DXS_FEAT_UTD
   /* UTD detector is mapped to the CPTD interface */
   pDrvCtx->SIG.CPTD_Start    = DXS_TAPI_LL_UTD_Start;
   pDrvCtx->SIG.CPTD_Stop     = DXS_TAPI_LL_UTD_Stop;
#endif /* DXS_FEAT_UTD */

#if defined(DXS_LINUX_USER_SPACE) || defined(VXWORKS)
   pDrvCtx->Open              = IFX_NULL;
   pDrvCtx->Release           = IFX_NULL;
#else /* DXS_LINUX_USER_SPACE */
   pDrvCtx->Open              = DXS_OpenKernel;
   pDrvCtx->Release           = DXS_ReleaseKernel;
#endif /* DXS_LINUX_USER_SPACE */
   /* Register this driver with the HL-TAPI driver
      this also registers the driver context for use by the HL-TAPI */
   result = IFX_TAPI_Register_LL_Drv (pDrvCtx);
   if (result != IFX_SUCCESS)
   {
      TRACE(DXS, DBG_LEVEL_HIGH, ("DXS driver start: registration failed\n"));
      return result;
   }

   /* Start and initialize the realtime kernel thread and it's resources
      to handle the interrupt routine for all devices handled by this driver.
      Note: not to be done before the register where the device structs are
      prepared. */
   DXS_interrupt_init();


#ifdef DXS_LINUX_KERNEL_SPACE
   printk("%s, (c) 2014-2016 Lantiq Beteiligungs-GmbH & Co.KG\n",
          &DRV_DXS_WHATVERSION [4]);
#else /* DXS_LINUX_KERNEL_SPACE */
   printf("%s, (c) 2014-2016 Lantiq Beteiligungs-GmbH & Co.KG\n\r",
          &DRV_DXS_WHATVERSION [4]);
#endif /* DXS_LINUX_KERNEL_SPACE */

#ifdef EVENT_LOGGER_DEBUG
   {
      IFX_int32_t i = 0;

      /* Register driver with device name, type and number to the eventlogger */
      for (i = 0; i < pDrvCtx->maxDevs; i++)
      {
         EL_REG_Register(DEV_NAME, DEV_TYPE_DUSLIC_XS,
                         i /* dev num */, IFX_NULL /* cb func */);
      }
   }
#endif /* EVENT_LOGGER_DEBUG */

   return IFX_SUCCESS;
}


/**
   DUSLIC XS device driver shutdown.
*/
IFX_void_t DXS_DeviceDriverStop (void)
{
   IFX_uint8_t    i;

   /* Unregister this driver from the HL-TAPI driver
      this also unregisters the device driver context */
   IFX_TAPI_Unregister_LL_Drv ((&DrvCtx)->majorNumber);

   for (i=0; i < DXS_MAX_DEVICES; i++)
   {
      /* resource cleanup */
      dxs_DeviceExit(&DxsDevices[i], IFX_TRUE);

      /* Free the capabilities list */
      if (DxsDevices[i].CapList != IFX_NULL)
      {
         DXS_OS_FREE (DxsDevices[i].CapList);
      }
   }

   /* terminate interrupt processing kernel thread */
   DXS_interrupt_exit();

   /* Note: the device struct array is a static variable so no free is needed.*/

#ifdef DXS_FEAT_UTD
   /* Delete the driver global storage for UTD coefficients. */
   DXS_UTD_DeleteStorage();
#endif /* DXS_FEAT_UTD */

#ifdef EVENT_LOGGER_DEBUG
   /* Unregister the driver with device name, type and number
      from the eventlogger */
   EL_REG_Unregister(DEV_NAME, DEV_TYPE_DUSLIC_XS, -1 /* dev num (-1 all) */);
#endif /* EVENT_LOGGER_DEBUG */
}
