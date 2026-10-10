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

#ifdef __KERNEL__
   #error This file can be used only for user space
#endif /* __KERNEL__ */

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
#define SPI_MAXBYTES_SIZE              32

#define DXS_POLL_CYCLE_MS   20 /* ms */

/* SPI initialisation */
extern IFX_int32_t USR_EXT_Init ();
#define SPI_INIT(pDev)    USR_EXT_Init()

/* SPI uninitialisation */
#define SPI_EXIT(pDev) /* DXS_SPI_drvUnregister(pDev) */
#define IFX_SSC_HANDLE int
/* Set callback mode because HW will drive the CS line ifself. */
#define DXS_USE_SPI_CS_CALLBACK

extern IFX_void_t USR_EXT_SPI_DevSet (IFX_uint16_t nDev, IFX_highLow_t high_low);

/* SPI chipselect set/unset */
#define SPI_CS_SET(devNo, high_low) \
   USR_EXT_SPI_DevSet (devNo, high_low) 

extern int USR_EXT_SPI_xfer (IFX_uint8_t* txptr, IFX_uint32_t txsize,
                             IFX_uint8_t* rxptr, IFX_uint32_t rxsize);

/* spi low level access function */
#define spi_ll_read_write(pDev, txptr,txsize,rxptr,rxsize)  \
                  USR_EXT_SPI_xfer(                    \
                  (IFX_uint8_t*)(txptr),              \
                  (IFX_uint32_t)(txsize),             \
                  (IFX_uint8_t*)(rxptr),              \
                  (IFX_uint32_t)(rxsize))

#endif /* _DRV_CONFIG_USER_H */
