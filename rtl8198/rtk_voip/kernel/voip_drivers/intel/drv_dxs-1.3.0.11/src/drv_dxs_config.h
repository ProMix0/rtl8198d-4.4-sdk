#ifndef _DRV_DXS_CONFIG_H
#define _DRV_DXS_CONFIG_H
/****************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_dxs_config.h
   This file configures the features depending on user controled defines and
   other defines.

   This is the main configuration file for the project.
*/

/* ============================= */
/* Configuration                 */
/* ============================= */
/* DUSLIC XS device name */
#define DEV_NAME "duslicxs"

/* Wait time in us for the next DUSLIC XS access in case of polling register */
#define DXS_WAIT_POLLTIME 50 /* us */

/* Time to wait until OBXML indicates data:
   DXS_OBXML_POLL_LOOP * DXS_WAIT_POLLTIME = poll time in us */
#define DXS_OBXML_POLL_LOOP 100 /* repeat count */

/* Loops until free inbox space is available
   time = DXS_FIBXMS_POLL_LOOP * DXS_WAIT_POLLTIME = poll time in us */
#define DXS_FIBXMS_POLL_LOOP 1000 /* repeat count */

/* timeout in ms for mailbox empty event in ms */
#define DXS_WAIT_CB_EMPTY 100 /* ms */


/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* includes                      */
/* ============================= */
#ifdef HAVE_CONFIG_H
   #include <drv_dxs_config_defines.h>
/*lint -save -e(19)  we need this ; to stop a runaway arguments from breaking
  out of drv_dxs_config_defines.h */
;
/*lint -restore */
#else
#ifndef VXWORKS
   #error no drv_dxs_config_defines.h
#endif /* !VXWORKS */
#endif

/* ============================= */
/* define mapping                */
/* ============================= */

/* define features depending on user controled defines and other defines */

/* support for GR909 line testing */
#if defined (DXS_GR909_SUPPORT)
   #define DXS_FEAT_GR909
   #define DXS_FEAT_CALIBRATION_STORAGE
#endif /* DXS_GR909_SUPPORT */

/* support for CID */
#if defined (DXS_CID)
   #define DXS_FEAT_CID
#endif /* DXS_CID */

/* support for CID */
#if defined (DXS_NLT)
   #define DXS_FEAT_NLT
#endif /* DXS_CID */

/* support for Metering */
#if defined (DXS_METERING)
   #define DXS_FEAT_METERING
#endif /* DXS_METERING */

/* support for analog line continuous measurement */
#if defined (DXS_CONT_MEASUREMENT)
   #define DXS_FEAT_CONT_MEASUREMENT
#endif /* DXS_CONT_MEASUREMENT */

/* support for power saving mode (sleep mode)  */
#if defined (DXT_POWER_SAVE)
   #define DXS_FEAT_POWER_SAVE
#endif /* DXS_POWER_SAVE */

#if defined (DXS_GPIO_SUPPORT)
   #define DXS_FEAT_GPIO_CTRL

   #if defined (DXS_KERNEL_API_SUPPORT)
      #define DXS_FEAT_GPIO_FUNCTIONS_EXPORT
   #endif /* DXS_KERNEL_API_SUPPORT */
#endif /* DXS_GPIO_SUPPORT */

#ifdef DXS_KERNEL_API_SUPPORT
   #define DXS_FEAT_KERNEL_FUNCTIONS_EXPORT
#endif /* DXS_KERNEL_API_SUPPORT */

#if defined (DXS_TONE_GENERATOR_SUPPORT)
   #define DXS_FEAT_TONE_GENERATOR
#endif /* DXS_TONE_GENERATOR_SUPPORT */

#if defined (DXS_UTD_SUPPORT)
   #define DXS_FEAT_UTD
#endif /* DXS_UTD_SUPPORT */

#if defined (DXS_DIRECT_CHIP_ACCESS_SUPPORT)
   #define DXS_FEAT_IOCTL_RW_CMD
   #define DXS_FEAT_IOCTL_RW_REG
#endif /* DXS_DIRECT_CHIP_ACCESS_SUPPORT */

#if defined (DXS_CAPACITANCE_MEASUREMENT_SUPPORT)
      #define DXS_FEAT_CAPACITANCE_MEASUREMENT
      #ifndef DXS_FEAT_CALIBRATION_STORAGE
         #define DXS_FEAT_CALIBRATION_STORAGE
      #endif /* DXS_FEAT_CALIBRATION_STORAGE */
#endif /* DXS_CAP_MEASUREMENT_SUPPORT */

#if (defined(IFXOS_USE_DEV_IO) && (IFXOS_USE_DEV_IO == 1))
   /* using IFXOS solution to enable usage of driver in user space */
   #define DXS_USE_DEV_IO
#endif /* (defined(IFXOS_USE_DEV_IO) && (IFXOS_USE_DEV_IO == 1)) */

#ifdef LINUX
   #include <linux/version.h>

   #ifdef __KERNEL__
      /** working as a regular driver in the kernel space */
      #define DXS_LINUX_KERNEL_SPACE
   #else /* __KERNEL__ */
      /** compiled as a library for user space */
      #define DXS_LINUX_USER_SPACE
   #endif /* __KERNEL__ */

   #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
      #define DXS_FEAT_LINUX_THREADED_IRQ
   #endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0) */
#endif /* LINUX */

/* ============================= */
/* safety check                  */
/* ============================= */

#if defined(DXS_LINUX_USER_SPACE) && defined (DXS_HAVE_INTERRUPTS)
   #error interrupt handling not supported for linux user space
#endif /* defined(DXS_LINUX_USER_SPACE) && defined (DXS_HAVE_INTERRUPTS) */

#endif /* _DRV_DXS_CONFIG_H */
