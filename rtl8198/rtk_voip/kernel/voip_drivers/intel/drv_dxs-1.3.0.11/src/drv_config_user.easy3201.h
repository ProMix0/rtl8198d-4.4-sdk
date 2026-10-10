#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_config_user.easy3201.h
   This file is intended to customize some optional settings of the driver.

   In case of optional features (like SPI interface) it may also contain
   some required definitions where no default values are making sense.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")

   \remark
   This file (drv_config_user.easy3201.h) is already prepared for the
   EASY3201 evaluation board.

   To use this file, make a copy or link with the name "drv_config_user.h"
   in your build- or source-directory.

   SPI Support:
   For the easy3201 plattform using the MPC uC, the spi driver MUST be loaded
   before and the SPI (hardware) initialization must be done by the board
   driver. Only then the SPI access can work correctly.
*/

/* EASY3201 user configuration */

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_mpc.h"
#include "drv_spi_export.h"

/* ============================= */
/* Defines                       */
/* ============================= */

/** activate the new callback mechanism to provide control of the CS handling
    to the SPI bus driver. drv_vinetic will register a callback (CB) function
    (v2cpe_spi_cs_handler()) to the SPI bus driver which allows to define
    exactly which pins to use for CS handling of DUSLIC XS.
    Currently not used for EASY3201. See sample files for AR9 or Danube
    for reference. */
#if 0
#define DXS_USE_SPI_CS_CALLBACK
#endif /* 0 */

/* EASY3201 SPI mask, dev 0 (PA1) */
#define SPI_CS0_MASK                    PADAT_D1
/* EASY3201 SPI mask, dev 1 (PC5) */
#define SPI_CS1_MASK                    PCDAT_D5

/* SPI buffer size configured to the size of the cmd inbox + SPI header
   in bytes */
#define SPI_MAXBYTES_SIZE                             66

/* SPI initialisation */
#ifndef DXS_USE_SPI_CS_CALLBACK
#define SPI_INIT(pDev) do {                                    \
   switch (pDev->nDevNr)                                       \
   {                                                           \
      case 0:                                                  \
         /* no further init required */                        \
         break;                                                \
      case 1:                                                  \
         /* configure CS for device 2 */                       \
         /* configure PC5 as general purpose gpio */           \
         *PCPAR(MPC_BASE_ADDR) &= ~PCPAR_DD5;                  \
         /* configure PC5 as CS out */                         \
         *PCDIR(MPC_BASE_ADDR) |= PCDIR_DR5;                   \
         /* set PC5 to high */                                 \
         *PCDAT(MPC_BASE_ADDR) |= SPI_CS1_MASK;                \
         break;                                                \
      default:                                                 \
         break;                                                \
   }                                                           \
} while(0)
#else
#error Currently no driver supports CS callback on this platform
#endif /* VIN_USE_SPI_CS_CALLBACK */


/* SPI chipselect set/unset */
#define SPI_CS_SET(devNo, high_low)                             \
   do                                                           \
   {                                                            \
      switch (devNo)                                            \
      {                                                         \
         case 0:                                                \
            if ((high_low) == 0)                                \
            {                                                   \
               /*IFXOS_IRQ_DISABLE(2);*/\
               *PADAT(MPC_BASE_ADDR) &= ~SPI_CS0_MASK;          \
            }                                                   \
            else                                                \
            {                                                   \
               *PADAT(MPC_BASE_ADDR) |= SPI_CS0_MASK;           \
               /*IFXOS_IRQ_ENABLE(2);*/\
            }                                                   \
            break;                                              \
         case 1:                                                \
            if ((high_low) == 0)                                \
            {                                                   \
               /*IFXOS_IRQ_DISABLE(12);*/\
               *PCDAT(MPC_BASE_ADDR) &= ~SPI_CS1_MASK;          \
            }                                                   \
            else                                                \
            {                                                   \
               *PCDAT(MPC_BASE_ADDR) |= SPI_CS1_MASK;           \
               /*IFXOS_IRQ_ENABLE(12);*/\
            }                                                   \
            break;                                              \
         default:                                               \
            break;                                              \
      }                                                         \
   } while(0)

/* spi low level access function */
#define spi_ll_read_write(pDev, txptr,txsize,rxptr,rxsize)  \
        SPI_Send ((txptr),(txsize),(rxptr),(rxsize))

#define CHIP_RESET(pDev, reset)

#endif /* _DRV_CONFIG_USER_H */
