#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_config_user.grx750.h
   This file is intended to customize some optional settings of the driver.

   In case of optional features (like SPI interface) it may also contain
   some required definitions where no default values are making sense.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")

   \remark
   This file (drv_config_user.grx750.h) is already prepared for systems
   with the GRX750 chipset - but needs to be adapted to the individual design.

   To use this file, make a copy or link with the name "drv_config_user.h"
   in your build-directory.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#ifdef LINUX
   #include <linux/spi/spi.h>
#endif /* LINUX */
#include <drv_dxs_config_defines.h>
#include "drv_dxs_api.h"
#include "drv_dxs_init.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

#define DXS_BROKEN_LX_MIPS_BSP_DEVICETREE

#define DXS_TID_DCDC_AUTODETECT
#define DXS_TID_DCDC_AUTODETECT_FUNC(dev,ret) \
               dxs_dcdc_autodetect((dev),(ret))

/* ============================== */
/*  SPI bus driver configuration  */
/* ============================== */

/* Define the maximum number of bytes that the SPI interface can transfer in
   one one call of the spi_ll_read_write() macro below.
   The driver will fragment all data during read or write transfer into pieces
   no larger than the value given here. To avoid overhead due to fragmentation
   use the maximum value the SPI interface can support.
   The minimum possible value is 4 bytes. There is no upper limit but values
   larger than 66 bytes are never used.

   As GRX750 can utilize only 8-bit access due to SPI bus master limitation,
   the SPI_MAXBYTES_SIZE is set to 6 (2 bytes SPI header plus 4 bytes payload). */
#define SPI_MAXBYTES_SIZE                            6

/** SPI initialization
    This macro is called during chip initialisation (if SPI is used) and
    can be extended to do board specific initialisation related to the
    DuSLIC-xS.
 */
#define SPI_INIT(pDev) do {                                                    \
         DXS_SPI_drvRegister(pDev);                                            \
} while(0);

#define SPI_EXIT(pDev) do {                                                    \
         DXS_SPI_drvUnregister(pDev);                                          \
} while(0);


/* SPI CS (chip select) is controlled by the GRX750 HW.
   Set callback but with an empty callback function. */
#define DXS_USE_SPI_CS_CALLBACK

#define DXS_SPI_READ_HALF_DUPLEX 1

/** SPI chipselect set/unset */
#define SPI_CS_SET(nDevNr, high_low) do {                                      \
} while(0);

/* spi low level access function */
#define spi_ll_read_write(pDev,txptr,txsize,rxptr,rxsize)                      \
            spi_write_then_read (pDev->pSpiDev,                                \
                  (const u8 *)(const IFX_uint8_t*)(txptr),                     \
                  (unsigned)(IFX_uint32_t)(txsize),                            \
                  (u8 *)(IFX_uint8_t*)(rxptr),                                 \
                  (unsigned)(IFX_uint32_t)(rxsize))

#define CHIP_RESET(pDev, reset) do {                                           \
} while(0)

/**
   Macro to map the system function with takes care of interrupt handling
   registration.

   \param irq  irq number

   \param func interrupt handler callback function

   \param arg  argument of interrupt handler callback function

   \remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#ifdef DXS_SYS_REGISTER_INT_HANDLER
   #undef DXS_SYS_REGISTER_INT_HANDLER
#endif /* DXT_SYS_REGISTER_INT_HANDLER */

#define DXS_IRQ_LEVEL_TRIGGERED 1



#endif /* _DRV_CONFIG_USER_H */
