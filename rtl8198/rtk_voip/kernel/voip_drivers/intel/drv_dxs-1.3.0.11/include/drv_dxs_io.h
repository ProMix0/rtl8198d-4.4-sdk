#ifndef _DRV_DXS_IO_H
#define _DRV_DXS_IO_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_io.h
   This file contains the defines specific to the DUSLIC XS driver interface
   and is used by applications.
*/

/** @defgroup DXS_DRIVER_INTERFACE DUSLIC XS Driver Interface
    Lists the entire interface to the DUSLIC XS Driver. */
/*@{*/

/** \defgroup DXS_DRIVER_INTERFACE_BASIC Basic Interface
    Basic DUSLIC XS access routines as command read/write and initilisation. */

/** \defgroup DXS_DRIVER_INTERFACE_INIT Driver Initialization Interface
    Interface needed to initialise the driver and the devices. */

/** \defgroup DXS_DRIVER_INTERFACE_GPIO GPIO Interface
    Control the device and channel specific IO pins. */

/** \defgroup DXS_KERNEL_INTERFACE Driver Kernel Interface
    Kernel Interface for GPIO/IO pin handling. */

/*@}*/ /* DXS_DRIVER_INTERFACE */


/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_io_types.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/** DUSLIC XS maximum channel number */
#define DXS_MAX_CH_NR            2
/** DUSLIC XS maximum analog channel number */
#define DXS_ANA_CH_NR            2

/** \addtogroup DXS_DRIVER_INTERFACE_INIT */
/*@{*/
/** Flag for \ref DXS_IO_Init_t to avoid firmware download */
#define DXS_NO_FW_DWLD          0x00000001
/** Flag for \ref DXS_IO_Init_t to avoid ASDSP download */
#define DXS_NO_ASDSP_DWLD       0x00000002
/*@}*/ /* TAPI_INTERFACE_CONTMEASUREMENT */

/* ============================= */
/* DUSLIC XS ioctl Defines       */
/* ============================= */

/* magic number */
#define DXS_IOC_MAGIC 'D'

/* IOCMD global defines */

/** \addtogroup DXS_DRIVER_INTERFACE_BASIC */
/*@{*/
#if 0
/**
   Write short command.
   The parameter points to a \ref DXS_IO_WRITE_SC  structure
   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation
*/
#define FIO_DXS_WSC                          _IO (DXS_IOC_MAGIC, 1)
/**
   Read short command.
   The parameter points to a \ref DXS_IO_READ_SC structure

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation.

   \code
   DXS_IO_READ_SC ioCmd;
   memset (&ioCmd, 0, sizeof (ioCmd));
   ioCmd.nCmd = 0xC100;
   ret = ioctl (fd, FIO_DXS_RSC, (INT) &ioCmd);
   if (ioCmd.pData[0] == 0x1FFF) return 1;
   \endcode
*/
#define FIO_DXS_RSC                          _IO (DXS_IOC_MAGIC,  2)

#define FIO_DXS_CRC                          _IO (DXS_IOC_MAGIC,  3)
#endif

/** Read relevant version information.
   The parameter points to a \ref DXS_IO_Version_t structure  */
#define FIO_DXS_VERS                         _IOR (DXS_IOC_MAGIC,  4, DXS_IO_Version_t)

#ifndef TAPI_ONE_DEVNODE
#define FIO_DXS_DRVVERS                      _IOR (DXS_IOC_MAGIC,  5, DXS_IO_Version_t)
#endif /* TAPI_ONE_DEVNODE */

/**
   Set the driver report levels if the driver is compiled with
   ENABLE_TRACE

   \remarks
   valid arguments are

   \arg 0: off

   \arg 1: low, high output

   \arg 2: normal, general information and warnings

   \arg 3: high, only errors are reported
*/
#define FIO_DXS_REPORT_SET                   _IOW (DXS_IOC_MAGIC, 6, IFX_uint32_t)

/**
   Write a DUSLIC XS command.
   The parameter points to a \ref DXS_IO_MB_CMD_t structure.

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation
*/
#define FIO_DXS_WCMD                         _IOW (DXS_IOC_MAGIC, 7, DXS_IO_MB_CMD_t)

/**
   Read command.
   The parameter points to a \ref DXS_IO_MB_CMD_t structure

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation

   \code
   DXS_IO_MB_CMD_t ioCmd;
   int err;

   ioCmd.cmd1 = 0x8300 | ch;
   // read OPMODE_CUR
   ioCmd.cmd2 = 0x2101;

   err = ioctl (fd, FIO_DXS_RCMD, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_RCMD                         _IOWR (DXS_IOC_MAGIC, 8, DXS_IO_MB_CMD_t)

/**
   DUSLIC XS Initialization. If the driver is compiled with TAPI support
   the interface IFX_TAPI_CH_INIT should be used.
   This interface expects a pointer to a \ref DXS_IO_Init_t structure
*/
#define FIO_DXS_INIT                         _IOW (DXS_IOC_MAGIC, 9, DXS_IO_Init_t)

#ifndef TAPI_ONE_DEVNODE
/** Driver runtime tracing of register accesses */
#define FIO_DXS_RUNTIME_TRACE_SET            _IO (DXS_IOC_MAGIC, 10)
#endif /* TAPI_ONE_DEVNODE */

/**
   Test the basic access of DUSLIC XS device

   \code
   ioctl (fd, FIO_DXS_TCA, 10);
   \endcode
    */
#define FIO_DXS_TCA                          _IOW (DXS_IOC_MAGIC, 12, DXS_TCA_t)
/*@}*/ /* DXS_DRIVER_INTERFACE_BASIC */

/** \addtogroup DXS_DRIVER_INTERFACE_GPIO GPIO Interface */
/*@{*/

/**
   Reserve GPIO pins for use. Use structure \ref DXS_IO_GPIO_Control_t

   \arg    nGpio        Each bit corresponds to one IO to be reserved.

   \code
   DXS_IO_GPIO_Control_t ioCmd;
   int err;
   memset (&ioCmd, 0, sizeof(ioCmd));
   ioCmd.nGpio = 0x00FF;
   err = ioctl (fd, FIO_DXS_GPIO_RESERVE, (INT) &ioCmd);
   myGpioHandle = ioCmd.pIoHandle;
   \endcode
*/
#define FIO_DXS_GPIO_RESERVE                 _IOWR (DXS_IOC_MAGIC, 33, DXS_IO_GPIO_Control_t)

/**
   Configure GPIO pins. Use structure \ref DXS_IO_GPIO_Control_t

   \note You have to store the pIoHandle for later use, this example expects
         that you have declared myGpioHandle somewhere in your code.

   \arg    nGpio        Each bit defines if the IO is configured (1) or left
                        unconfigured (0) in this call. The Mask given here
                        must not select IOs which were not previously reserved.
   \arg    nMask        Each bit defines the direction (0 - Input, 1 - Output).

   \code
   DXS_IO_GPIO_Control_t ioCmd;
   int err;
   memset (&ioCmd, 0, sizeof(ioCmd));
   ioCmd.pIoHandle = myGpioHandle;
   ioCmd.nGpio    = 0x00FF;
   // define pin 0..3 as input, 4..7 as output
   ioCmd.nMask    = 0x00F0;
   err = ioctl (fd, FIO_DXS_GPIO_CONFIG, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_GPIO_CONFIG                  _IOW (DXS_IOC_MAGIC, 34, DXS_IO_GPIO_Control_t)

/**
   Set GPIO pin values. Use structure \ref DXS_IO_GPIO_Control_t

   \note You have to store the pIoHandle for later use, this example expects
    that you have declared myGpioHandle somewhere in your code.

   \arg    nGpio        Each bit defines if the IO is set (1) or left unset (0)
                        in this call. The Mask given here must not select IOs
                        which were not previously reserved.

   \arg    nMask        Each bit defines the desired state of the corresponding
                        GPIO pin.

   \code
   DXS_IO_GPIO_Control_t ioCmd;
   int err;
   memset (&ioCmd, 0, sizeof(ioCmd));
   ioCmd.pIoHandle = myGpioHandle;
   ioCmd.nGpio    = 0x00FF;
   // set pin 0..3 off, 4..7 on
   ioCmd.nMask    = 0x00F0;

   err = ioctl (fd, FIO_DXS_GPIO_SET, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_GPIO_SET                     _IOW (DXS_IOC_MAGIC, 35, DXS_IO_GPIO_Control_t)

/**
   Get GPIO pin values. Use structure \ref DXS_IO_GPIO_Control_t

   \arg    nGpio        Each bit defines if the value of the IO is read (1)
                        or not read (0) in this call. The Mask given here
                        must not select IOs which were not previously reserved.

   \arg    nMask        Each bit returns the current state of a GPIO pin which
                        is selected with the nGPIO parameter.

   \code
   DXS_IO_GPIO_Control_t ioCmd;
   int err;
   memset (&ioCmd, 0, sizeof(ioCmd));
   ioCmd.pIoHandle = myGpioHandle;
   ioCmd.nGpio    = 0x00FF;
   err = ioctl (fd, FIO_DXS_GPIO_GET, (INT) &ioCmd);
   printf ("GPIO state 0x%04X\n", ioCmd.nMask);
   \endcode
*/
#define FIO_DXS_GPIO_GET                     _IOWR (DXS_IOC_MAGIC, 36, DXS_IO_GPIO_Control_t)

/**
   Release GPIO pins. Use structure \ref DXS_IO_GPIO_Control_t
*/
#define FIO_DXS_GPIO_RELEASE                 _IOW (DXS_IOC_MAGIC, 37, DXS_IO_GPIO_Control_t)

/**
   Read host register (DUSLIC XS).
   The parameter points to a \ref DXS_IO_RegAccess_t structure

   \note
   This interface is only for debugging and testing purposes.
   Using of this interface may disturb the driver operation

   \code
   DXS_IO_REG_ACCESS_t ioCmd;
   int err;

   ioCmd.offset = 0x0C; // register STAT_INT

   err = ioctl (fd, FIO_DXS_RDREG, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_RDREG                        _IOWR (DXS_IOC_MAGIC, 38, DXS_IO_RegAccess_t)

/**
   Write ro host register (DUSLIC XS)
   The parameter points to a \ref DXS_IO_RegAccess_t structure

   \note
   This interface is provided for debugging and testing purposes.
   Using of this interface may disturb the driver operation
   */
#define FIO_DXS_WRREG                        _IOW (DXS_IOC_MAGIC, 39, DXS_IO_RegAccess_t)

/**
   Read command extended
   The parameter points to a \ref DXS_IO_MB_CMD_t structure

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation

   \code
   DXS_IO_MB_CMD_t ioCmd;
   int err;

   ioCmd.cmd1 = 0x8100 | ch;
   -- read OPMODE_CUR
   ioCmd.cmd2 = 0x0902;
   -- additional data field for the read request
   ioCmd.pData[0] = 0x0000;
   -- expected lengh of the reply in bytes (!)
   ioCmd.pData[1] = 04;

   err = ioctl (fd, FIO_DXS_RCMD_XT, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_RCMD_XT                      _IOWR (DXS_IOC_MAGIC, 40, DXS_IO_MB_CMD_t)

/**
   Important: The use of this interface is deprecated. The chip reset is not
   a function of the DXS driver. The controller should provide means to
   manipulate chip reset line. This interface will be removed in future
   versions.
   Board specific reset of the DUSLIC XS chip
   \tapiv3
   The parameter is either 0 (deactivate reset) or 1 (activate reset)
   \endtapiv3
   \tapiv4
   The reset can be is either deactivated or activated.
   Look at parameter description for more details.
   \endtapiv4

   \note
   This interface calls the CHIP_RESET macro defined in drv_config_user.h
*/
#define FIO_DXS_CHIP_RESET                   _IOW (DXS_IOC_MAGIC, 41, DXS_ChipReset_t)

/*@}*/

#ifdef EVALUATION
/**
   Fakes the occurence of events, to evaluate the logging mechanism.

   \note
   This interface is provided for debugging and testing purposes.
   Using of this interface may disturb the driver operation.
   */
#define FIO_DXS_EVAL_EVENT                   _IOW (DXS_IOC_MAGIC, 100, IFX_uint16_t)
#endif /* #ifdef EVALUATION */

/** \addtogroup DXS_DRIVER_INTERFACE_INIT */
/*@{*/

/** Initialize DUSLIC XS Device driver information for one chip.
    Does some required settings, which must be done before any
    other chip access will work!
    Parameter is a pointer to a \ref DXS_BasicDeviceInit_t structure.
*/
#define FIO_DXS_BASICDEV_INIT                _IOW (DXS_IOC_MAGIC, 200, DXS_BasicDeviceInit_t)

/**
   Reset Vinetic Device driver internal structure for one chip.
   This ioctl must be called after each vinetic hard reset not leading
   to a vinetic basic device initialization.
*/
#ifdef TAPI_ONE_DEVNODE
   #define FIO_DXS_DEV_RESET                    _IOW (DXS_IOC_MAGIC, 201, DXS_DevReset_t)
#else /* TAPI_ONE_DEVNODE */
   #define FIO_DXS_DEV_RESET                    _IO (DXS_IOC_MAGIC, 201)
#endif /* TAPI_ONE_DEVNODE */

/**
   Does a download according to bbd format.
   Parameter is a pointer to a \ref DXS_BBD_Download_t structure.
   \tapiv3
   This structure is a generic bbd library structure.
   \endtapiv3
*/
#define FIO_DXS_BBD_DOWNLOAD                 _IOW (DXS_IOC_MAGIC, 202, DXS_BBD_Download_t)

/**
   Does a download of firmware.
   Parameter is a pointer to a \ref DXS_FW_Download_t structure.
*/
#define FIO_DXS_FW_DOWNLOAD                  _IOW (DXS_IOC_MAGIC, 203, DXS_FW_Download_t)
/*@}*/ /* DXS_DRIVER_INTERFACE_INIT */


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
/** \addtogroup DXS_KERNEL_INTERFACE */
/*@{*/

/**
   Open the device or channel from kernel mode.

   \param pHandle  pointer to LL device structure

   \return
   IFX_SUCCESS

*/
extern IFX_int32_t DXS_OpenKernel (IFX_void_t *pHandle);

/**
   Release the device or channel.

   \param pHandle  pointer to LL device structure

   \return
   IFX_SUCCESS
*/
extern IFX_int32_t DXS_ReleaseKernel  (IFX_void_t *pHandle);

/**
   Reserve a DUSLIC XS GPIO pin resource.

   \param pDevHandle handle to DUSLIC XS device structure

   \param nGpio      mask for GPIOs to reserve (0 = free, 1 = reserve)

   \param ppIoHandle return comunication handle

   \return
   - IFX_SUCCESS  if requested GPIO is available
*/
extern IFX_int32_t DXS_GpioReserve(IFX_void_t   *pDevHandle,
                                   IFX_uint16_t nGpio,
                                   IFX_void_t   **ppIoHandle);

/**
   Release a DUSLIC XS IO or GPIO pin resource
   \param pIoHandle   handle returned by DXS_GpioReserve

   \return
   IFX_SUCCESS if releasing targeted IO/GPIO is possible, else IFX_ERROR
*/
extern IFX_int32_t DXS_GpioRelease(IFX_void_t *pIoHandle);

/**
  Configure a DUSLIC XS IO or GPIO pin

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param pCfg       handle to the GPIO configuration structure

   \return
   IFX_SUCCESS if requested IO/GPIO is available and could be configured, else IFX_ERROR
*/
extern IFX_int32_t DXS_GpioConfig  (IFX_void_t *pIoHandle,
                                    DXS_GPIO_Config_t *pCfg);

/**
   Set the value of a DUSLIC XS IO or GPIO pin

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param nSet       values to store

   \param nMask      only bits set to '1' will be stored

   \return
   IFX_SUCCESS if requested IO/GPIO is available and could be written, else IFX_ERROR
*/
extern IFX_int32_t DXS_GpioSet  (IFX_void_t *pIoHandle,
                                 IFX_uint16_t nSet,
                                 IFX_uint16_t nMask);

/**
   Read the value from a DUSLIC XS IO or GPIO pin

   \param pIoHandle  handle returned by DXS_GpioReserve

   \param nGet       pointer where the read value shall be stored

   \param nMask      only bits set to '1' will be stored
\return
   IFX_SUCCESS if requested IO/GPIO is available and could be read, else IFX_ERROR
*/
extern IFX_int32_t DXS_GpioGet  (IFX_void_t *pIoHandle,
                                 IFX_uint16_t *nGet,
                                 IFX_uint16_t nMask);

/*@}*/ /* DXS_KERNEL_INTERFACE */


/* =================================== */
/*     DUSLIC XS ioctl access          */
/* =================================== */

/* general mode settings (set or get information) */
#define  IOSET    0
#define  IOGET    1
#define  IOMODIFY 2

#endif /* _DRV_DXS_IO_H */
