/* include/drv_tapi_autoconf.h.  Generated from drv_tapi_autoconf.h.in by configure.  */
/* include/drv_tapi_autoconf.h.in.  Generated from configure.ac by autoheader.  */

/******************************************************************************

  Copyright (c) 2001-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _DRV_CONFIG_H_
#define _DRV_CONFIG_H_


/* Build in debugging symbols and enable debugging messages */
/* #undef DEBUG */

/* enable DECT channel support */
/* #undef DECT_SUPPORT */

/* configure options string */
#define DRV_TAPI_CONFIGURE_STR ""

/* enable hotplug events in general */
/* #undef ENABLE_HOTPLUG */

/* enable log (errors) outputs in general */
#define ENABLE_LOG 1

/* enable trace outputs in general */
#define ENABLE_TRACE 1

/* Enable counting of events */
/* #undef EVENT_COUNTER */

/* enable event logger debugging */
/* #undef EVENT_LOGGER_DEBUG */

/* Define to 1 if ifxos should be enabled. */
/* #undef HAVE_LIBIFXOS */

/* enable KPI support */
/* #undef KPI_SUPPORT */

/* enable KPI tasklet mode */
#define KPI_TASKLET 1

/* enable code for linux 2.6 or later */
#define LINUX_2_6 1

/* enable Linux SMP support */
/* #undef LINUX_SMP_SUPPORT */

/* Name of package */
#define PACKAGE "drv_tapi"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "Lantiq Beteiligungs-GmbH & Co.KG"

/* Define to the full name of this package. */
#define PACKAGE_NAME "TAPI v4 - The Common Voice API"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "TAPI v4 - The Common Voice API 4.17.0.7"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "drv_tapi"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.17.0.7"

/* enable PCM channel support */
#define PCM_SUPPORT 1

/* enable QOS support */
/* #undef QOS_SUPPORT */

/* enable runtime traces as register access */
#define RUNTIME_TRACE 1

/* enable TAPI Announcements support */
#define TAPI_ANNOUNCEMENTS 1

/* enable TAPI Calibration support */
#define TAPI_CALIBRATION 1

/* enable capacitance measurement services */
#define TAPI_CAPACITANCE_MEASUREMENT_SUPPORT 1

/* enable TAPI CID support */
#define TAPI_CID 1

/* enable TAPI Analog Line Continuous Measurement */
#define TAPI_CONT_MEASUREMENT 1

/* enable TAPI CPTD support */
#define TAPI_CPTD 1

/* enable TAPI pulse dial/hook support */
#define TAPI_DIAL 1

/* enable TAPI DTMF support */
#define TAPI_DTMF 1

/* enable TAPI legacy FAX Data Pump support */
/* #undef TAPI_FAX_T38 */

/* enable TAPI FAX T.38 stack support */
#define TAPI_FAX_T38_FW 1

/* enable TAPI FXO support */
#define TAPI_FXO_SUPPORT 1

/* enable TAPI GR909 support */
#define TAPI_GR909 1

/* enabled */
/* #undef TAPI_HDLC */

/* enable TAPI Hook state machine */
#define TAPI_HOOKSTATE 1

/* enable Kernel API */
#define TAPI_KERNEL_API_SUPPORT 1

/* enable TAPI LEC support */
#define TAPI_LEC_SUPPORT 1

/* enable TAPI METERING support */
#define TAPI_METERING 1

/* enable TAPI MFTD support */
#define TAPI_MFTD_SUPPORT 1

/* enable TAPI MF R2 tone detector support */
/* #undef TAPI_MF_R2 */

/* enable TAPI MOS-LQE calculation support */
/* #undef TAPI_MOS_LQE */

/* enable TAPI MWL support */
#define TAPI_MWL 1

/* enable TAPI Network Line Testing services */
#define TAPI_NLT 1

/* enabled read/write packet interface */
#define TAPI_PACKET 1

/* enabled owner ID for packet interface */
/* #undef TAPI_PACKET_OWNID */

/* enable TAPI Tone Level Peak Detector support */
#define TAPI_PEAKD 1

/* enable FXS Phone Detection support */
/* #undef TAPI_PHONE_DETECTION */

/* enable Power Management Control support */
/* #undef TAPI_PMC */

/* Use Linux CPU frequency interface */
/* #undef TAPI_PMC_IF_CPUFREQ */

/* Use LANTIQ PMCU interface */
/* #undef TAPI_PMC_IF_PMCU */

/* enable TAPI ring engine support */
#define TAPI_RING_ENGINE 1

/* enable TAPI SRTP support */
/* #undef TAPI_SRTP */

/* enable TAPI SmartSLIC support */
#define TAPI_SSLIC_SUPPORT 1

/* enable packet path statistics support */
/* #undef TAPI_STATISTICS */

/* enable tone generator feature */
#define TAPI_TONE_GENERATOR_SUPPORT 1

/* enable use of proc filesystem entries (Linux only) */
#define TAPI_USE_PROC 1

/* enable Voice support */
#define TAPI_VOICE 1

/* use linux bufferpool allocation */
/* #undef USE_LINUX_ALLOC */

/* Version number of package */
#define VERSION "4.17.0.7"

/* environment and device specific CFLAGS passed to configure */
#define WITH_CLFAGS_CONFIGURE_OPTION -O2 -Wall


#endif /* _DRV_CONFIG_H_ */

