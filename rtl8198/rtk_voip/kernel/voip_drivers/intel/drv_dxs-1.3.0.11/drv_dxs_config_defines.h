/* drv_dxs_config_defines.h.  Generated from drv_dxs_config_defines.h.in by configure.  */
/* drv_dxs_config_defines.h.in.  Generated from configure.in by autoheader.  */

/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _DRV_DXS_CONFIG_DEFINES_H_
#define _DRV_DXS_CONFIG_DEFINES_H_


/* Build in debugging symbols and enable debugging messages */
/* #undef DEBUG */

/* enable capacitance measurement services */
#define DXS_CAPACITANCE_MEASUREMENT_SUPPORT 1

/* enable CID support */
#define DXS_CID 1

/* configure options string */
#define DXS_CONFIGURE_STR ""

/* enable Analog Line Continuous Measurement */
#define DXS_CONT_MEASUREMENT 1

/* Define the DC/DC converter type of the target HW. */
#define DXS_DCDC_TYPE "IBB12"

/* enable direct chip access support */
/* #undef DXS_DIRECT_CHIP_ACCESS_SUPPORT */

/* enable GPIO control */
/* #undef DXS_GPIO_SUPPORT */

/* enable GR909 line testing services */
#define DXS_GR909_SUPPORT 1

/* enable interrupts */
#define DXS_HAVE_INTERRUPTS 1

/* enable Kernel API */
/* #undef DXS_KERNEL_API_SUPPORT */

/* Maximum DUSLIC XS devices to support */
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR)
#define DXS_MAX_DEVICES CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR)
#define DXS_MAX_DEVICES CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR
#else
#define DXS_MAX_DEVICES 1
#endif

/* enable METERING support */
#define DXS_METERING 1

/* enable NLT support */
#define DXS_NLT 1

/* DUSLIC XS SPI 8-bit access */
#define DXS_SPI_8BIT_ACCESS 1

/* enable tone generator feature */
#define DXS_TONE_GENERATOR_SUPPORT 1

/* enable use of proc filesystem entries (Linux only) */
#define DXS_USE_PROC 1

/* enable DuSLIC-xT power saving support */
#define DXT_POWER_SAVE 1

/* enable DuSLIC-xT UTD support */
#define DXT_UTD_SUPPORT 1

/* enable log (errors) outputs in general */
#define ENABLE_LOG 1

/* enable trace outputs in general */
#define ENABLE_TRACE 1

/* enable evaluation features e.g. for testing with WinEASY */
/* #undef EVALUATION */

/* enable event logger debugging */
/* #undef EVENT_LOGGER_DEBUG */

/* enable code for linux 2.6 or later */
#define LINUX_2_6 1

/* Name of package */
#define PACKAGE "drv_dxs"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "Intel Corporation"

/* Define to the full name of this package. */
#define PACKAGE_NAME "DUSLIC XS Device Driver"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "DUSLIC XS Device Driver 1.3.0.11"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "drv_dxs"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.3.0.11"

/* enable TAPI CID support */
#define TAPI_CID 1

/* enable TAPI Analog Line Continuous Measurement */
#define TAPI_CONT_MEASUREMENT 1

/* enable TAPI GR909 support */
#define TAPI_GR909 1

/* enable TAPI METERING support */
#define TAPI_METERING 1

/* enable TAPI Network Line Testing services */
#define TAPI_NLT 1

/* Version number of package */
#define VERSION "1.3.0.11"

/* environment and device specific CFLAGS passed to configure */
#define WITH_CLFAGS_CONFIGURE_OPTION -O2 -Wall


#endif /* _DRV_DXS_CONFIG_DEFINES_H_ */

