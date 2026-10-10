/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_ioctl.c
   Contains ioctl specific implementations according to io type.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_access.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_debug.h"
#include "drv_dxs_alm_lt.h"
#include "drv_dxs_init.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_version.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/**
   Dispatches ioctrl commands which doesn't need data management

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - argument for the function, cast before if needed.
*/
#define  ON_DIR_IOCTL(pContext,msg,func,arg)\
            case (msg):\
               ret = func((pContext),arg);\
               break

/**
   Dispatches ioctrl commands and manages user space data handling in both
   directions.

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - structure identifier used as argument for the function
                    call and for copying user data
\remarks
   As an alternative use a transfer structure DUS_IO_USR and
   DoDataExchange for handling data exchange.
*/
#ifdef LINUX
#define ON_IOCTL(pContext,msg,func,arg)\
           case (msg):\
              {\
              arg* p_arg = DXS_OS_MALLOC(sizeof(arg));\
              DXS_ASSERT (p_arg != IFX_NULL);\
              DXS_OS_CpyUsr2Kern(p_arg, (IFX_uint8_t*)ioarg, sizeof(arg));\
              ret = func((pContext), p_arg);\
              DXS_OS_CpyKern2Usr((IFX_uint8_t*)ioarg, p_arg, sizeof(arg));\
              DXS_OS_FREE (p_arg);\
              }\
              break
#else
#define ON_IOCTL(pContext,msg,func,arg)\
           case (msg):\
              ret = func((pContext),(arg*)ioarg);\
              break
#endif /* LINUX */

/**
   Dispatches ioctrl commands and manages user space data handling in
   driver direction.

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - structure identifier used as argument for the function
                    call and for copying user data
*/
#ifdef LINUX
#define ON_IOCTL_FRUSR(pContext,msg,func,arg)\
         case (msg):\
            {\
              arg* p_arg = DXS_OS_MALLOC(sizeof(arg));\
              DXS_ASSERT (p_arg != IFX_NULL);\
              DXS_OS_CpyUsr2Kern(p_arg, (IFX_uint8_t*)ioarg, sizeof(arg));\
              ret = func((pContext), p_arg);\
              DXS_OS_FREE (p_arg);\
            }\
            break
#else
#define ON_IOCTL_FRUSR(pContext,msg,func,arg)\
           ON_IOCTL((pContext),msg,func,arg)
#endif /* LINUX */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

#ifdef TAPI_ONE_DEVNODE

/** Generic structure for device based ioctl arguments. */
struct DXS_DEV_IO_ARG
{
   /** Device index */
   IFX_uint16_t dev;
   /** Any parameter used by ioctls */
   IFX_uint32_t param;
};
#endif /* TAPI_ONE_DEVNODE */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#ifdef DXS_FEAT_IOCTL_RW_CMD
static IFX_int32_t DXS_IOCTL_CmdWrite  (DXS_DEVICE_t *pDev,
                                        DXS_IO_MB_CMD_t *pCmd);
static IFX_int32_t DXS_IOCTL_CmdRead   (DXS_DEVICE_t *pDev,
                                        DXS_IO_MB_CMD_t *pCmd);
#endif /* DXS_FEAT_IOCTL_RW_CMD */

#ifdef DXS_FEAT_IOCTL_RW_REG
static IFX_int32_t DXS_IOCTL_RegWrite  (DXS_DEVICE_t *pDev,
                                        DXS_IO_RegAccess_t *pCmd);
static IFX_int32_t DXS_IOCTL_RegRead   (DXS_DEVICE_t *pDev,
                                        DXS_IO_RegAccess_t *pCmd);
#endif /* DXS_FEAT_IOCTL_RW_REG */
static IFX_int32_t DXS_IOCTL_ChipReset (DXS_DEVICE_t *pDev,
                                        IFX_uint16_t reset);
extern IFX_int32_t DXS_DwldAndStartFW  (DXS_DEVICE_t *pDev,
                                        DXS_FW_Download_t *pEdsp);

extern IFX_int32_t DXS_Version (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_Version_t *pInd);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   DUSLIC XS device specific ioctl handling

   \param pDummyCh   handle to DXS_CHANNEL_t structure (could be also a pointer
                     to a DXS_DEVICE_t structure - so we have to check)

   \param iocmd      ioctl identifier

   \param ioarg

   \return
   - DXS_statusOk
   - DXS_statusNotInitialized
*/
IFX_int32_t DXS_Dev_Spec_Ioctl (IFX_TAPI_LL_CH_t *pLLDummyCh,
                                IFX_uint32_t iocmd,
                                IFX_ulong_t  ioarg)
{
   IFX_int32_t   ret    = DXS_statusOk;
   DXS_CHANNEL_t *pCh   = (DXS_CHANNEL_t *) pLLDummyCh;
   DXS_DEVICE_t  *pDev;
#ifdef TAPI_ONE_DEVNODE
   struct DXS_DEV_IO_ARG* pDevArg = IFX_NULL;
#endif /* TAPI_ONE_DEVNODE */

#ifndef TAPI_ONE_DEVNODE
   /* validate correct channel pointer */
   if (IFX_NULL == pCh)
      return DXS_statusParam;

   /* distinguish device / channel context */
   if (pCh->nChannel == 0)
   {
      /* initial assumption is wrong - we received a device pointer */
      pDev = (DXS_DEVICE_t *) pLLDummyCh;
   }
   else
   {
      /* initial assumption was correct so we only need to get the
         corresponding device pointer */
      pDev = pCh->pParent;
   }
#else /* TAPI_ONE_DEVNODE */
   /* by default use first device and set channel to null */
   DXS_GetDevice(0, &pDev);
   pCh = IFX_NULL;

   /* getting context for specific ioctls */
   switch (iocmd)
   {
      /* Drv specific ioctls */
      case FIO_DXS_REPORT_SET:
         break;
      /* Dev specific ioctls */
      case FIO_DXS_VERS:
      case FIO_DXS_INIT:
      case FIO_DXS_BASICDEV_INIT:
      case FIO_DXS_DEV_RESET:
      case FIO_DXS_FW_DOWNLOAD:
      case FIO_DXS_CHIP_RESET:
      case FIO_DXS_WCMD:
      case FIO_DXS_RCMD:
      case FIO_DXS_RCMD_XT:
      case FIO_DXS_WRREG:
      case FIO_DXS_RDREG:
      case FIO_DXS_TCA:
      case FIO_DXS_GPIO_RESERVE:
      case FIO_DXS_GPIO_CONFIG:
      case FIO_DXS_GPIO_SET:
      case FIO_DXS_GPIO_GET:
      case FIO_DXS_GPIO_RELEASE:
         /* simple, not perfect, argument validation */
         if (ioarg <= 255)
         {
            RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
         }
         /* get data from user space */
         pDevArg = OS_MapBuffer((IFX_void_t*) ioarg, sizeof(*pDevArg));
         if (NULL == pDevArg)
         {
            RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
         }

         if (DXS_statusOk != DXS_GetDevice(pDevArg->dev, &pDev))
         {
            OS_UnmapBuffer(pDevArg);
            return DXS_statusDeviceIdErr;
         }

         if (iocmd == FIO_DXS_CHIP_RESET)
         {
            /* get reset value so handling of argument doesn't have to change
               for tapi v4 */
            ioarg = ((DXS_ChipReset_t*) pDevArg)->nReset;
         }

         if (iocmd == FIO_DXS_TCA)
         {
            /* get max_val value so handling of argument doesn't have to change
               for tapi v4 */
            ioarg = ((DXS_TCA_t*)pDevArg)->max_val;
         }
         OS_UnmapBuffer(pDevArg);

         break;
      /* channel specific ioctls - currently only one */
      case FIO_DXS_BBD_DOWNLOAD:
         {
            DXS_BBD_Download_t * pBbdArg;

            /* simple, not perfect, argument validation */
            if (ioarg <= 255)
            {
               RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
            }
            /* get data from user space */
            pBbdArg = OS_MapBuffer((IFX_void_t*) ioarg,
                                   sizeof(DXS_BBD_Download_t));
            if (NULL == pBbdArg)
            {
               RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
            }
            if (DXS_statusOk != DXS_GetDevice(pBbdArg->dev, &pDev))
            {
               OS_UnmapBuffer(pBbdArg);
               return DXS_statusDeviceIdErr;
            }
            /* if broadcast, then use this BBD for all channels */
            if (IFX_TRUE == pBbdArg->bBroadcast)
            {
               pCh = (DXS_CHANNEL_t *) pDev;
            }
            /* otherwise use only specified channel */
            else
            {
               /* get specific channels */
               if (DXS_MAX_CH_NR <= pBbdArg->ch)
               {
                  OS_UnmapBuffer(pBbdArg);
                  RETURN_DEVSTATUS(DXS_statusInvalCh, IFX_NULL);
               }
               pCh = &pDev->pChannel[pBbdArg->ch];
            }
            OS_UnmapBuffer(pBbdArg);
         }
         break;
      default:
         /* errmsg: Ioctl not supported. */
         RETURN_DEVSTATUS(DXS_statusNotSupported, IFX_NULL);
   }
#endif /* TAPI_ONE_DEVNODE */


   /* this block handles the initialization of the DUSLIC XS driver */
   switch (iocmd)
   {
      /* This must be the very first ioctl operation before hardware access. */
      ON_IOCTL_FRUSR (pDev, FIO_DXS_BASICDEV_INIT, DXS_ChipAccessInit,
                      DXS_BasicDeviceInit_t);

      /* This ioctl executes board specific DXS chip reset routine */
      ON_DIR_IOCTL   (pDev, FIO_DXS_CHIP_RESET, DXS_IOCTL_ChipReset,
                      (IFX_uint16_t) ioarg);
      /*
         This can be done to reset the device internal structure without doing
         any basic device initialization. Only the initialization state remains
         unchanged. All other states are reset.
      */
      case FIO_DXS_DEV_RESET:
         IFX_TAPI_DeviceReset (pDev->pTapiDev);
         return IFX_SUCCESS;

#ifndef TAPI_ONE_DEVNODE
      /* no basic driver initialization needed for these ioctls */
      case FIO_DXS_DRVVERS:
#ifndef LINUX
         ((DXS_IO_Version_t*)ioarg)->nDrvVers =
            (MAJORSTEP << 24 | MINORSTEP << 16 | VERSIONSTEP << 8 | VERS_TYPE);
         return IFX_SUCCESS;
#else
         return IFX_ERROR;
#endif /* LINUX */
#endif /* TAPI_ONE_DEVNODE */

      case FIO_DXS_REPORT_SET:
         return DXS_Report_Set((IFX_uint32_t)ioarg);

      case FIO_DXS_WCMD:
      case FIO_DXS_RCMD:
      case FIO_DXS_WRREG:
      case FIO_DXS_RDREG:
      case FIO_DXS_TCA:
         /* allow these ioctls even without basic device driver init */
         break;

      default:
         /* do not allow other ioctl if basic init is missing! */
         if ((pDev->nDevState & DS_BASIC_INIT) != DS_BASIC_INIT)
         {
            /* errmsg: Device not yet initialized. */
            RETURN_DEVSTATUS(DXS_statusNotInitialized, IFX_NULL);
         }
   }

   /*
      This code block handles ioctls which require previous basic device driver
      initialization.
   */
   switch (iocmd)
   {
      ON_DIR_IOCTL   (pDev, FIO_DXS_TCA, DXS_AccessTest,
                      (IFX_uint16_t) ioarg);
#ifdef DXS_FEAT_IOCTL_RW_CMD
      ON_IOCTL_FRUSR (pDev, FIO_DXS_WCMD, DXS_IOCTL_CmdWrite,
                      DXS_IO_MB_CMD_t );
      ON_IOCTL       (pDev, FIO_DXS_RCMD, DXS_IOCTL_CmdRead,
                      DXS_IO_MB_CMD_t);
#endif /* DXS_FEAT_IOCTL_RW_CMD */
      ON_IOCTL       (pDev, FIO_DXS_VERS, DXS_Version,
                     DXS_IO_Version_t);
      ON_IOCTL       (pDev, FIO_DXS_INIT, DXS_FW_Start,
                      DXS_IO_Init_t);
      /* GPIO Module */
#ifdef DXS_FEAT_GPIO_CTRL
      ON_IOCTL       (pDev, FIO_DXS_GPIO_RESERVE, DXS_GpioReserveUser,
                      DXS_IO_GPIO_Control_t);
      ON_IOCTL_FRUSR (pDev, FIO_DXS_GPIO_RELEASE, DXS_GpioReleaseUser,
                      DXS_IO_GPIO_Control_t);
      ON_IOCTL_FRUSR (pDev, FIO_DXS_GPIO_CONFIG, DXS_GpioConfigUser,
                      DXS_IO_GPIO_Control_t);
      ON_IOCTL       (pDev, FIO_DXS_GPIO_GET, DXS_GpioGetUser,
                      DXS_IO_GPIO_Control_t);
      ON_IOCTL_FRUSR (pDev, FIO_DXS_GPIO_SET, DXS_GpioSetUser,
                      DXS_IO_GPIO_Control_t);
#endif /* DXS_FEAT_GPIO_CTRL */
#ifdef DXS_FEAT_IOCTL_RW_REG
      ON_IOCTL_FRUSR (pDev, FIO_DXS_WRREG, DXS_IOCTL_RegWrite,
                      DXS_IO_RegAccess_t);
      ON_IOCTL       (pDev, FIO_DXS_RDREG, DXS_IOCTL_RegRead,
                      DXS_IO_RegAccess_t);
#endif /* DXS_FEAT_IOCTL_RW_REG */
      ON_IOCTL       (pCh, FIO_DXS_BBD_DOWNLOAD, DXS_BBD_Download,
                      DXS_BBD_Download_t);
      ON_IOCTL       (pDev, FIO_DXS_FW_DOWNLOAD, DXS_DwldAndStartFW,
                      DXS_FW_Download_t);

#ifndef TAPI_ONE_DEVNODE
#ifdef EVALUATION
      ON_DIR_IOCTL   (pDev, FIO_DXS_EVAL_EVENT, DXS_Eval_EventLog,
                      (IFX_uint16_t) ioarg);
#endif /* EVALUTAION */
#endif /* TAPI_ONE_DEVNODE */

      /* already handled in first switch, if not handled again here an unknwon
         ioctl would be traced */
      case FIO_DXS_BASICDEV_INIT:
      case FIO_DXS_CHIP_RESET:
         break;
      default:
         TRACE (DXS,DBG_LEVEL_HIGH,
            ("Unknown IOCTL command 0x%08x or feature deactivated\n",iocmd));
         break;
   }

   return ret;
}

#ifdef DXS_FEAT_IOCTL_RW_CMD
/**
   DUSLIC XS command write

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with command data.

   \return
   - DXS_statusOk if no error
   - error code from DXS_CmdWrite otherwise
*/
static IFX_int32_t DXS_IOCTL_CmdWrite (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_MB_CMD_t *pCmd)
{
   IFX_int32_t  ret;
   IFX_uint32_t *pData;

   /* validate parameter */
   if (IFX_NULL == pCmd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   /* pCmd is valid (not IFX_NULL) */
   pData = (IFX_uint32_t *)&pCmd->cmd1;

   ret = DXS_CmdWrite (pDev, pData);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   DUSLIC XS command read

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with command data.

   \return
   - DXS_statusOk if no error
   - error code from DXS_CmdRead otherwise
*/
static IFX_int32_t DXS_IOCTL_CmdRead (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_MB_CMD_t *pCmd)
{
   IFX_int32_t ret;
   IFX_uint32_t *pData;

   /* validate parameter */
   if (IFX_NULL == pCmd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   pData = (IFX_uint32_t *)&pCmd->cmd1;

   ret = DXS_CmdRead (pDev, pData, pData);

   /* if read failed set the cmd length to 0 */
   if (ret != IFX_SUCCESS)
      pCmd->cmd2 = 0;

   RETURN_DEVSTATUS(ret, IFX_NULL);
}
#endif /* DXS_FEAT_IOCTL_RW_CMD */

#ifdef DXS_FEAT_IOCTL_RW_REG
/**
   IOCTL handling function used to write a single host register.

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with register details.

   \return
   - DXS_statusOk if no error
   - error code from DXS_RegWriteMulti otherwise
*/
static IFX_int32_t DXS_IOCTL_RegWrite (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_RegAccess_t *pCmd)
{
   IFX_int32_t ret = IFX_ERROR;

   /* validate parameter */
   if (IFX_NULL == pCmd || (!pCmd->count) || (pCmd->count & 1))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   if (pCmd->count == 2)
      ret = DXS_RegWrite(pDev, pCmd->offset, pCmd->pData[0]);
   else
      ret = DXS_RegWriteMulti(pDev, pCmd->offset, pCmd->pData,
                                   (pCmd->count >> 1));

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   Ioctl handling function used to read a single host register.

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with register details.

   \return
   - DXS_statusOk if no error
   - error code from DXS_RegReadMulti otherwise
*/
static IFX_int32_t DXS_IOCTL_RegRead (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_RegAccess_t *pCmd)
{
   IFX_int32_t ret = IFX_ERROR;

   /* validate parameter */
   if (IFX_NULL == pCmd || (!pCmd->count) || (pCmd->count & 1))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   if (pCmd->count == 2)
      ret = DXS_RegRead(pDev, pCmd->offset, pCmd->pData);
   else
      ret = DXS_RegReadMulti(pDev, pCmd->offset, pCmd->pData,
                                  (pCmd->count >> 1));

   RETURN_DEVSTATUS(ret, IFX_NULL);
}
#endif /* DXS_FEAT_IOCTL_RW_REG */

/**
   Ioctl handling function used to execute board specific chip reset.

   \param pDev pointer to device data

   \param reset 0=deactivate 1=activate

   \return
   returns IFX_SUCCESS, always
*/
static IFX_int32_t DXS_IOCTL_ChipReset (DXS_DEVICE_t *pDev,
                                        IFX_uint16_t reset)
{
   /* Mark as unused in case the macro is not there. */
   DXS_OS_UNUSED (pDev);
   DXS_OS_UNUSED (reset);

   /* This macro is defined in drv_config_user.h */
#ifdef CHIP_RESET
   CHIP_RESET (pDev, reset);
#endif /* #ifdef CHIP_RESET */

   return IFX_SUCCESS;
}


/**
   Get the DUSLIC XS Chip Version

   \param  pDev         Pointer to the device interface.
   \param  pInd         Pointer to the user interface.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t DXS_Version (DXS_DEVICE_t *pDev, DXS_IO_Version_t *pInd)
{
   IFX_int32_t             err = DXS_statusOk;

   /* validate parameter */
   if (IFX_NULL == pInd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* While DevStart was not done read the version every time directly
         from the firmware. */
      err = DXS_ReadFwVersion (pDev);

      if (DXS_statusOk != err)
         RETURN_DEVSTATUS(err, IFX_NULL);
   }

   pInd->nHwRev   = pDev->nChipRev;
   pInd->nDevID   = pDev->nDevId;
   pInd->nFwRev   = pDev->nFwRev;
   pInd->nAsdspRev= 0;

   /* Set number of supported FXS channels. LSB of DevId indicates the number
      of channels where 0 is a 2-channel device. */
   if ((pInd->nDevID & 0x01) == 0)
   {
      /* 2-channel device */
      pInd->nFxsCh = 0x2;
      pInd->nFxoCh = 0x0;
   }
   else
   {
      /* 1-channel device */
      pInd->nFxsCh = 0x1;
      pInd->nFxoCh = 0x0;
   }

   /* set device index */
   pInd->dev = pDev->nDevNr;

   /* set driver version */
   pInd->nDrvVers = MAJORSTEP << 24 | MINORSTEP << 16 |
                    VERSIONSTEP << 8 | VERS_TYPE;

   RETURN_DEVSTATUS(err, IFX_NULL);
}
