#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file drv_config_user.usr.h
   Configuration for EASY3201 EVS (Evaluation system)
   for driver running in user space.
   This file is intended to customize system specific settings of the driver.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")
*/
//#include "drv_dxs_api.h"
#include "spi/spi.h"
#include "gpio/gpio.h"
//#ifdef __KERNEL__
//   #error This file can be used only for user space
//#endif /* __KERNEL__ */
#if 1
//#define DXS_BROKEN_LX_MIPS_BSP_DEVICETREE

#define DXS_TID_DCDC_AUTODETECT
#define DXS_TID_DCDC_AUTODETECT_FUNC(dev,ret) \
               dxs_dcdc_autodetect((dev),(ret))
#endif
/* ========================================================================== */
/* Includes                      */
/* ========================================================================== */


/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ============================== */
/*  SPI bus driver configuration  */
/* ============================== */

/* The SVIP SPI interface buffer has a size of 32 bytes. This is also the
   maximum number of bytes that the SVIP SPI driver accepts in each call of
   spi_write_then_read() below. */
#define SPI_MAXBYTES_SIZE   6 

#define DXS_POLL_CYCLE_MS   20 /* ms */

/* SPI initialisation */
//extern IFX_int32_t USR_EXT_Init (DXS_DEVICE_t *pDev);
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI)
#define SPI_INIT(pDev)	do {	\
	_init_rtl_spi_dev_type_hw(pDev->spi_dev, pDev->spiCS_);	\
	}	while(0)
#else // SPI + PCM,  SPI_CS is gpio-simulated
#if 1 
#define SPI_INIT(pDev)	do {	\
	_init_rtl_spi_dev_type_hw4(pDev->spi_dev, pDev->spiCS_);	\
	}	while(0)
#else
#define SPI_INIT(pDev)	do {	\
	_init_rtl_spi_dev_type_hw2(pDev->spi_dev, pDev->spiCS_);	\
	}	while(0)
#endif 
#endif

/* SPI uninitialisation */
#define SPI_EXIT(pDev) /* DXS_SPI_drvUnregister(pDev) */
#define IFX_SSC_HANDLE int
#if 1//ndef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
/* Set callback mode because HW will drive the CS line ifself. */
#define DXS_USE_SPI_CS_CALLBACK
#endif
//extern IFX_void_t USR_EXT_SPI_DevSet (IFX_uint16_t nDev, IFX_highLow_t high_low);
#if 0
IFX_void_t USR_EXT_SPI_DevSet (rtl_spi_dev_t *spi_dev, IFX_uint16_t nDev, IFX_highLow_t high_low);
#else
IFX_void_t USR_EXT_SPI_DevSet (IFX_uint16_t nDev, IFX_highLow_t high_low);
#endif

/* SPI chipselect set/unset */
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
#define SPI_CS_SET(devNo, high_low)
#else
#define SPI_CS_SET(devNo, high_low) \
   USR_EXT_SPI_DevSet (devNo, high_low)  
#endif

int USR_EXT_SPI_xfer (rtl_spi_dev_t *spi_dev, IFX_uint8_t* txptr, IFX_uint32_t txsize,  IFX_uint8_t* rxptr, IFX_uint32_t rxsize);

/* spi low level access function */
#if 1
#define spi_ll_read_write_1(pDev, txptr,txsize,rxptr,rxsize)  \
                  USR_EXT_SPI_xfer(                    \
				  (rtl_spi_dev_t*)(pDev),			\
                  (IFX_uint8_t*)(txptr),              \
                  (IFX_uint32_t)(txsize),             \
                  (IFX_uint8_t*)(rxptr),              \
                  (IFX_uint32_t)(rxsize))
//#else
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
#define spi_ll_read_write(pDev, txptr,txsize,rxptr,rxsize)  \
                  USR_EXT_SPI_xfer(                    \
				  (rtl_spi_dev_t*)(pDev->spi_dev[0]),	\
                  (IFX_uint8_t*)(txptr),              \
                  (IFX_uint32_t)(txsize),             \
                  (IFX_uint8_t*)(rxptr),              \
                  (IFX_uint32_t)(rxsize))
#else
#define spi_ll_read_write(pDev, txptr,txsize,rxptr,rxsize)  \
                  USR_EXT_SPI_xfer(                    \
				  (rtl_spi_dev_t*)(&pDev->spi_dev[0]),	\
                  (IFX_uint8_t*)(txptr),              \
                  (IFX_uint32_t)(txsize),             \
                  (IFX_uint8_t*)(rxptr),              \
                  (IFX_uint32_t)(rxsize))
#endif
#endif



#if 1
#if !defined(CONFIG_RTK_VOIP_LINUX_SPI) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI)
#define DXS_SPI_READ_HALF_DUPLEX 1
#endif

#ifdef DXS_SYS_REGISTER_INT_HANDLER
   #undef DXS_SYS_REGISTER_INT_HANDLER
#endif /* DXT_SYS_REGISTER_INT_HANDLER */

#define DXS_IRQ_LEVEL_TRIGGERED 1
#endif
#endif /* _DRV_CONFIG_USER_H */
