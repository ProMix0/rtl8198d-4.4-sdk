/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_access.c
   Implementation of low level access functions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_access.h"
#include "drv_config_user.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#if (!defined(SPI_MAXBYTES_SIZE) ||\
     !defined(SPI_CS_SET) ||\
     !defined(spi_ll_read_write))
#error Set SPI support macros in drv_config_user.h and compile again!
#endif /* SPI_MAXBYTES_SIZE| SPI_CS_SET | spi_ll_read_write */

#if (!defined(DXS_USE_SPI_CS_CALLBACK) && (SPI_MAXBYTES_SIZE < 4))
#error SPI_MAXBYTES_SIZE cannot be set smaller than 4. The DUSLIC XS requires \
       at least 4 bytes for each SPI transfer cycle during which the chip \
       select is kept active.
/* Note that this limit is only for the case that the CS is handled by the SPI
   driver with each call of the spi_ll_read_write() macro. */
#endif /* !defined(DXS_USE_SPI_CS_CALLBACK) && (SPI_MAXBYTES_SIZE < 4) */

#if (SPI_MAXBYTES_SIZE < 2)
#error SPI_MAXBYTES_SIZE cannot be set smaller than 2 bytes.
/* Note that this is a general limit due to driver implementation. */
#endif /* (SPI_MAXBYTES_SIZE < 4) */

/* SPI header length in bytes */
#define DXS_SPI_HDR_LENGTH             2
/* SPI header manipulation flags and masks */
#define DXS_SPI_HDR_B0_R               0x80
#define DXS_SPI_HDR_B0_W               0x40
#define DXS_SPI_HDR_B0_RESERVED        0x3E
#define DXS_SPI_HDR_B1_I               0x01

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
static DXS_OS_mutex_t spi_dxt_mutex;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
unsigned char current_slicType = SLIC_LINK_MODE_ZSI;
#endif
/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t dxs_spi_write(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pbuf,
                        IFX_uint32_t len);

static IFX_int32_t dxs_spi_read(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pbuf,
                        IFX_uint32_t len);

static IFX_int32_t dxs_spi_setup (
                        DXS_DEVICE_t *pDev);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Write a value to one chip register.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       16-bit word value to write.

   \return
   Error code from dxs_spi_write().
*/
IFX_int32_t DXS_RegWrite(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t nValue)
{
   IFX_int32_t ret;
   IFX_uint16_t data[2] = {nValue, nValue};

#ifdef DXS_SPI_8BIT_ACCESS
   ret = dxs_spi_write(pDev, offset, data, sizeof(data));
#else
   ret = dxs_spi_write(pDev, offset, data, sizeof(IFX_uint16_t));
#endif

   LOG_WR_REG(pDev->nDevNr, pDev->nChannel, offset, data, 2);

   return ret;
}


/**
   Read a value from one chip register.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to variable where to return
                        the 16-bit word value.

   \return
   Error code from dxs_spi_read().
*/
IFX_int32_t DXS_RegRead(DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue)
{
   IFX_int32_t ret;
   IFX_uint16_t data[2];

#ifdef DXS_SPI_8BIT_ACCESS
   ret = dxs_spi_read(pDev, offset, data, sizeof(data));
#else
   ret = dxs_spi_read(pDev, offset, data, sizeof(IFX_uint16_t));
#endif
   *pValue = data[0];
   LOG_RD_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, 2);

   return ret;
}


/**
   Write multiple consecutive chip registers.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to array with 16-bit word values to write.
   \param  count        Number of 16-bit words to write.

   \return
   Error code from dxs_spi_write().
*/
IFX_int32_t DXS_RegWriteMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count)
{
   IFX_int32_t ret;

   ret = dxs_spi_write(pDev, offset, pValue, (2*count));
   LOG_WR_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, (2*count));

   return ret;
}


/**
   Read multiple consecutive chip registers.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to array where to return the 16-bit word values.
   \param  count        Number of 16-bit words to read.

   \return
   Error code from dxs_spi_read().
*/
IFX_int32_t DXS_RegReadMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count)
{
   int32_t ret;

   ret = dxs_spi_read(pDev, offset, pValue, (2*count));
   LOG_RD_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, (2*count));

   return ret;
}


/**
   Check if the device can be accessed by writing and reading one register.

   This is used to test if the HW is connected before doing the initilization.

   \param  pDev         Pointer to device struct.

   \return
   - DXS_statusOk
   - DXS_statusSpiAccErr
   - DXS_statusTestChipAccErr

   \remarks
   The selected register can hold only values from 0 to 31. So this test is
   hardcoded to check only this range.
*/
IFX_int32_t DXS_reg_access_test(
                        DXS_DEVICE_t *pDev)
{
   IFX_int32_t  ret = DXS_statusOk;
   IFX_uint16_t offset = 0x48;
   IFX_uint16_t original_value, write_val, read_val;
   IFX_uint32_t i = 0, errors = 0;

   /* Backup the original value. */
   ret = DXS_RegRead(pDev, offset, &original_value);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   for (i = 0; i <= 31; i++)
   {
      write_val = i;

      ret = DXS_RegWrite(pDev, offset, write_val);
      if (!DXS_SUCCESS(ret))
         RETURN_DEVSTATUS (ret, IFX_NULL);

      ret = DXS_RegRead(pDev, offset, &read_val);
      if (!DXS_SUCCESS(ret))
         RETURN_DEVSTATUS (ret, IFX_NULL);

      if (write_val != read_val)
      {
         errors++;
         TRACE(DXS, DBG_LEVEL_HIGH,
               ("ERROR! DXS dev %d access mismatch wr:0x%04X rd:0x%04X\n",
                pDev->nDevNr, write_val, read_val));
      }
   }

   /* Restore the original value. */
   ret = DXS_RegWrite(pDev, offset, original_value);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   if (errors > 0)
   {
      /* errmsg: Test chip access failed. */
      ret = DXS_statusTestChipAccErr;
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/* ************************ endianess correct copy ****************************/

/**
   Copy a 16-bit-word buffer into a byte buffer respecting the endianess.

   Odd and even offsets and length are handled by this function.

   \param  pBbuf        Pointer to byte buffer. (Destination)
   \param  pWbuf        Pointer to 16-bit-word buffer. (Source)
   \param  nWoffset     Offset in bytes within the source buffer.
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpw2b (        IFX_uint8_t *pBbuf,
                        const IFX_uint16_t * const pWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i;
   IFX_uint16_t nWord;

   for (i = 0; i < nB; i++)
   {
      nWord = pWbuf[(nWoffset + i) >> 1];
      pBbuf[i] = (IFX_uint8_t)
                 ((((nWoffset+i) % 2) ? nWord : nWord >> 8) & 0xFF);
   }
}

/**
   Copy a 32-bit-word buffer into a byte buffer respecting the endianess.

   Odd and even offsets and length are handled by this function.

   \param  pBbuf        Pointer to byte buffer. (Destination)
   \param  pWbuf        Pointer to 16-bit-word buffer. (Source)
   \param  nWoffset     Offset in bytes within the source buffer.
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpdw2b (       IFX_uint8_t *pBbuf,
                        const IFX_uint32_t * const pWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i;
   IFX_uint32_t nWord;

   for (i = 0; i < nB; i++)
   {
      nWord = pWbuf[(nWoffset + i) >> 2];
      pBbuf[i] = (IFX_uint8_t)
                 ((nWord >> ((3 - ((nWoffset + i) % 4)) * 8)) & 0xFF);
   }
}

/**
   Copy a byte buffer into a 16-bit-word buffer respecting the endianess.

   In place copy is possible but otherwise the buffers must not overlap.
   When an odd number of bytes is to be copied the missing byte in the
   16-bit-word is filled with zero.

   \param  pWbuf        Pointer to 16-bit-word buffer. (Destination)
   \param  pBbuf        Pointer to byte buffer. (Source)
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpb2w (        IFX_uint16_t *pWbuf,
                        const IFX_uint8_t * const pBbuf,
                        IFX_uint32_t nB)
{
   IFX_uint32_t i;
   IFX_uint16_t nWord;

   for (i=0; i < nB; i+=2)
   {
      /* Copy each byte separately into the word buffer. Make sure that when
         an index out of range is addressed zero is copied instead. */
      nWord  = (IFX_uint16_t)pBbuf[i+0] << 8;
      if ((i+1) < nB)
      {
         /* LSB is only added when index is within range. */
         nWord |= (IFX_uint16_t)pBbuf[i+1];
      }
      pWbuf[i>>1] = nWord;
   }
}

/**
   Copy a byte buffer into a 32-bit-word buffer respecting the endianess.

   When an odd number of bytes is to be copied the missing bytes in the
   32-bit-word are filled with zeroes. Also in the first word the bytes
   which are skipped because of an offset are filled with zeroes.

   \param  pDWbuf       Pointer to 32-bit-word buffer. (Destination)
   \param  nWoffset     Offset in bytes within the destination buffer.
   \param  pBbuf        Pointer to byte buffer. (Source)
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpb2dw (       IFX_uint32_t *pDWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint8_t* const pBbuf,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i = 0,
                j = 0;
   IFX_uint32_t word32 = 0L;

   for (i = 0; i < nB; i++)
   {
      j = nWoffset + i;

      word32 |= (IFX_uint32_t)pBbuf[i] << ((3 - (j % 4)) * 8);

      if ((j % 4) == 3)
      {
         pDWbuf[j / 4] = word32;
         word32 = 0L;
      }
   }

   if ((i > 0) && ((j % 4) != 3))
   {
      pDWbuf[j / 4] = word32;
   }
}

/***************************** SPI bus adaption ******************************/

/**
   Init the SPI interface that is used for communication with the DUSLIC XS

   \param  pDev         Pointer to device structure.
*/
IFX_void_t dxs_init_spi (DXS_DEVICE_t *pDev)
{
#if !defined(SPI_INIT)
   DXS_OS_UNUSED(pDev);
#endif /* !defined(SPI_INIT) */
   /* macro defined in drv_config_user.h */
   /* This macro must be executed before the first chip access is done */
#if 0//def SPI_INIT
   SPI_INIT(pDev);
#endif /* #ifdef SPI_INIT */
#ifdef DXS_SPI_MODE_SET
   DXS_SPI_MODE_SET(pDev, 3);
#endif /* DXS_SPI_MODE_SET */
#ifdef DXS_SPI_BAUDRATE_SET
   DXS_SPI_BAUDRATE_SET(pDev, 100000);
#endif /* DXS_SPI_BAUDRATE_SET */
}


/**
   Release the SPI interface that is used for communication with the DUSLIC XS

   \param  pDev         Pointer to device structure.
*/
IFX_void_t dxs_exit_spi (DXS_DEVICE_t *pDev)
{
#if !defined(SPI_EXIT)
   DXS_OS_UNUSED(pDev);
#endif /* !defined(SPI_EXIT) */

   /* macro defined in drv_config_user.h */
#ifdef SPI_EXIT
   DXS_SPI_PROTECT(pDev);
   SPI_EXIT(pDev);
   DXS_SPI_RELEASE(pDev);
#endif /* #ifdef SPI_EXIT */
}


/**
   Write a number of bytes to a given DXS register using SPI.

   If the length of the data exceeds the SPI_MAXBYTES_SIZE this function
   fragments the data into multiple blocks and calls the SPI write interface
   repeatedly until all data is transmitted.

   \param  pDev         Pointer to device structure.
   \param  offset       DXS register address.
   \param  pbuf         Pointer to buffer with data to be written.
   \param  len          Number of bytes to write.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t dxs_spi_write(DXS_DEVICE_t *pDev,
                          IFX_uint8_t offset,
                          IFX_uint16_t *pbuf,
                          IFX_uint32_t len)
{
   IFX_uint8_t   tx_buf[SPI_MAXBYTES_SIZE];
   IFX_uint32_t  tx_buf_length,
                 written,
                 fragment_length;
   IFX_int32_t   ret = DXS_statusOk;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   IFX_uint16_t  cmd[32];
#endif

   /* (len == 0) not allowed : assertion */
   DXS_ASSERT(len != 0);
   /* offset must be even, as we do 16bit access : assertion */
   DXS_ASSERT((offset & 0x1) == 0);
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   memcpy(cmd,pbuf,len);
#endif

   /* Wait until SPI becomes accessible and configure it. */
   if (dxs_spi_setup(pDev) != DXS_statusOk)
   {
      pDev->nErr = DXS_statusSpiAccErr;
      return DXS_statusSpiAccErr;
   }

   /* This variable counts the number of bytes already written. */
   written = 0;

   /* Build SPI header (16-bit word) */
   tx_buf[0] = DXS_SPI_HDR_B0_W | DXS_SPI_HDR_B0_RESERVED;
   tx_buf[1] = offset;
#ifndef DXS_SPI_8BIT_ACCESS
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
    if((SLIC_LINK_MODE_ZSI == current_slicType) && (len < 4) && (offset != DXS_HOST_DATA))
    {
        cmd[1] = cmd[0];
        len = 4;
    }
   if(SLIC_LINK_MODE_ZSI != current_slicType)
    {
#endif
   /* Set auto increment if register isn't a mailbox register. */
   if ((len > 2) && (offset != DXS_HOST_DATA))
   {
      tx_buf[1] |= DXS_SPI_HDR_B1_I;
   }
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   }
#endif
#endif
   /* The tx_buf now contains the SPI header. */
   tx_buf_length = DXS_SPI_HDR_LENGTH;
#ifdef SPI_DEVICE_RESERVE
   /* Select which device to address */
   SPI_DEVICE_RESERVE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RESERVE */
#ifndef DXS_USE_SPI_CS_CALLBACK
   /* Activate DXS by setting chip select line. (low means active) */
   SPI_CS_SET (pDev->nDevNr, IFX_LOW);
#endif /* #ifndef DXS_USE_SPI_CS_CALLBACK */

   /* This is the loop that fragments the data to be written. */
   while ((ret == DXS_statusOk) && (written < len))
   {
      /* Calculate the amount of payload bytes that can be sent during one
         write call. */
      fragment_length = len - written; /* remaining bytes to transfer */
      if (fragment_length > (SPI_MAXBYTES_SIZE - tx_buf_length))
      {
         /* Limit the transfer to the maximum the SPI can transmit. */
         fragment_length = (SPI_MAXBYTES_SIZE - tx_buf_length);
      }
      if ((offset == DXS_HOST_DATA) && (fragment_length > 4))
      {
         /* For mailboxes ensure that the fragment has a multiple of 4 bytes. */
         fragment_length = fragment_length - (fragment_length % 4);
      }
      /* Copy the payload into the write buffer with correct endianess. */
      if (offset == DXS_HOST_DATA)
      {
         /* The mailbox has 32-bit datawidth. */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
         DXS_cpdw2b(tx_buf+tx_buf_length,
                    (IFX_uint32_t *)cmd, written, fragment_length);
#else
         DXS_cpdw2b(tx_buf+tx_buf_length,
                    (IFX_uint32_t *)pbuf, written, fragment_length);
#endif
      }
      else
      {
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
         DXS_cpw2b(tx_buf+tx_buf_length, cmd, written, fragment_length);
#else
         DXS_cpw2b(tx_buf+tx_buf_length, pbuf, written, fragment_length);
#endif
      }

      /* Increment by the number of payload bytes written. */
      tx_buf_length += fragment_length;
      written += fragment_length;
	  DXS_OS_MutexGet(&spi_dxt_mutex);
#ifndef	CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI	  
	  SPI_CS_SET(pDev->spi_dev[0].hw4.gpioCS_, IFX_LOW);
#endif	  
      /* Access SPI for writing (read ptr is NULL). */
      ret = spi_ll_read_write (pDev, tx_buf, tx_buf_length, NULL, 0);
      /* ret is evaluated before the next iteration. */
#ifndef	CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI	  
	  SPI_CS_SET(pDev->spi_dev[0].hw4.gpioCS_, IFX_HIGH);
#endif	  
	DXS_OS_MutexRelease(&spi_dxt_mutex);

#ifndef DXS_USE_SPI_CS_CALLBACK
      /* For consecutive accesses we still hold the CS and do not need to
         repeat the SPI adressing that is done with the SPI header. */
      tx_buf_length = 0;
#else
      /* Preserve and reuse the SPI header for the next fragment. */
      tx_buf_length = DXS_SPI_HDR_LENGTH;
#endif /* DXS_USE_SPI_CS_CALLBACK */
   }
#ifndef DXS_USE_SPI_CS_CALLBACK
   /* Deactivate DXS by setting chip select line. (high means inactive) */
   SPI_CS_SET(pDev->nDevNr, IFX_HIGH);
#endif /* #ifndef DXS_USE_SPI_CS_CALLBACK */
#ifdef SPI_DEVICE_RELEASE
   /* Unselect the device */
   SPI_DEVICE_RELEASE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RELEASE */

   return ret;
}


/**
   Read a number of bytes from a given DXS register using SPI.

   If the length of the data to be read exceeds the SPI_MAXBYTES_SIZE this
   function calls the SPI read function repeatedly until all data is read
   and reassembles the data in the return buffer.

   \param  pDev         Pointer to device structure.
   \param  offset       DXS register address.
   \param  pbuf         Pointer to buffer where to return the data.
   \param  len          Number of bytes to be read.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t dxs_spi_read(DXS_DEVICE_t *pDev,
                         IFX_uint8_t offset,
                         IFX_uint16_t *pbuf,
                         IFX_uint32_t len)
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   IFX_uint8_t   tx_buf[38],
                 rx_buf[38];
#else
   IFX_uint8_t   tx_buf[SPI_MAXBYTES_SIZE],
                 rx_buf[SPI_MAXBYTES_SIZE];
#endif
   IFX_uint32_t  tx_buf_length,
                 read,
                 fragment_length;
   IFX_int32_t   ret = DXS_statusOk;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   IFX_uint32_t  read_len = 0;
#endif

   /* (len == 0) not allowed : assertion */
   DXS_ASSERT(len != 0);
   /* offset must be even, as we do 16bit access : assertion */
   DXS_ASSERT((offset & 0x1) == 0);

   /* Wait until SPI becomes accessible and configure it. */
   if (dxs_spi_setup(pDev) != DXS_statusOk)
   {
      pDev->nErr = DXS_statusSpiAccErr;
      return DXS_statusSpiAccErr;
   }

   /* tx_buf is the buffer for writing the read request. Because SPI in general
      is full duplex the complete buffer is set to zero to avoid sending random
      data after the SPI header that we set below. */
   memset (tx_buf, 0x00, sizeof(tx_buf));
   /* Build SPI header (16-bit word) */
   tx_buf[0] = DXS_SPI_HDR_B0_R | DXS_SPI_HDR_B0_RESERVED;
   tx_buf[1] = offset;
#ifndef DXS_SPI_8BIT_ACCESS
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   if(SLIC_LINK_MODE_ZSI != current_slicType)
    {
#endif
   /* Set auto increment if register isn't a mailbox register. */
   if ((len > 2) && (offset != DXS_HOST_DATA))
   {
      tx_buf[1] |= DXS_SPI_HDR_B1_I;
   }
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
   }
#endif
#endif
   /* The tx_buf contains now the SPI header. */
   tx_buf_length = DXS_SPI_HDR_LENGTH;

#ifdef SPI_DEVICE_RESERVE
   /* Select which device to address */
   SPI_DEVICE_RESERVE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RESERVE */
#ifndef DXS_USE_SPI_CS_CALLBACK
   /* Activate DXS by setting chip select line. (low means active) */
   SPI_CS_SET(pDev->nDevNr, IFX_LOW);
#endif /* #ifndef DXS_USE_SPI_CS_CALLBACK */

   /* Loop while more data needs to be read. */
   for (read = 0; (ret == DXS_statusOk) && (read < len); read += fragment_length)
   {
      /* Calculate the amount of payload bytes that can be read during one
         read call. */
      fragment_length = len - read; /* remaining bytes to read */
      if (fragment_length > (SPI_MAXBYTES_SIZE - tx_buf_length))
      {
         /* Limit the transfer to the maximum the SPI can transmit. */
         fragment_length = (SPI_MAXBYTES_SIZE - tx_buf_length);
      }
      if ((offset == DXS_HOST_DATA) && (fragment_length > 4))
      {
         /* For mailboxes ensure that the fragment has a multiple of 4 bytes. */
         fragment_length = fragment_length - (fragment_length % 4);
      }
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
      read_len = fragment_length;

      if((SLIC_LINK_MODE_ZSI == current_slicType) && (fragment_length < 10))
      {
           read_len = 10;
      }
#endif
		DXS_OS_MutexGet(&spi_dxt_mutex);
#ifndef	CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI	
	  SPI_CS_SET(pDev->spi_dev[0].hw4.gpioCS_, IFX_LOW);
#endif	  
      /* Access SPI for writing and reading. */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
      ret = spi_ll_read_write (pDev, tx_buf, tx_buf_length,
                                     rx_buf, read_len);
#else
      ret = spi_ll_read_write (pDev, tx_buf, tx_buf_length,
                                     rx_buf, fragment_length);
#endif
#ifndef	CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	  SPI_CS_SET(pDev->spi_dev[0].hw4.gpioCS_, IFX_HIGH);
#endif
		DXS_OS_MutexRelease(&spi_dxt_mutex);

      if (ret == DXS_statusOk)
      {
         /* Copy the byte buffer into the 16-bit-word buffer respecting
            the endianess. */

#ifdef DXS_SPI_READ_HALF_DUPLEX
         /* Received data is aligned at offset 0 in the buffer. */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
        if(SLIC_LINK_MODE_ZSI == current_slicType)
        {
            DXS_cpb2w (pbuf + (read/2),
                    rx_buf+1, fragment_length);
        }
        else
        {
#endif
         DXS_cpb2w (pbuf + (read/2),
                    rx_buf, fragment_length);
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
        }
#endif
#else
         /* Received data is at offset where the write ends. */
         DXS_cpb2w (pbuf + (read/2),
                    rx_buf + DXS_SPI_HDR_LENGTH, fragment_length);
#endif
      }

#ifndef DXS_USE_SPI_CS_CALLBACK
      /* for consecutive accesses we still hold the CS and do not need to
         repeat the SPI adressing that is done with the SPI header */
      tx_buf_length = 0;
#endif /* DXS_USE_SPI_CS_CALLBACK */

      /* ret is evaluated before the next iteration. */
   }

#ifndef DXS_USE_SPI_CS_CALLBACK
   /* Deactivate DXS by setting chip select line. (high means inactive) */
   SPI_CS_SET(pDev->nDevNr, IFX_HIGH);
#endif /* #ifndef DXS_USE_SPI_CS_CALLBACK */
#ifdef SPI_DEVICE_RELEASE
   /* Unselect the device */
   SPI_DEVICE_RELEASE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RELEASE */
   return ret;
}


/**
   Return the number of payload bytes that can be transferred over SPI
   without being fragmented into multiple SPI transfers.

   \return
   Maximum number of bytes for a single SPI transfer.
*/
IFX_uint32_t DXS_spi_blocksize_get(void)
{
   return SPI_MAXBYTES_SIZE - DXS_SPI_HDR_LENGTH;
}


#ifdef DXS_USE_SPI_CS_CALLBACK
/**
   callback function for SPI bus driver to all during it's sequence to
   provide access to DUSLIC XS device driver
   \param on           CS status high or low
   \param cs_data      data stored in the SPI bus driver, in our case the devNr
   \return
   always DXS_statusOk as the macro doesn't have a return value
*/
IFX_int32_t dxs_spi_cs_handler (IFX_uint32_t on, IFX_int32_t cs_data)
{
    SPI_CS_SET(cs_data, on);
    return DXS_statusOk;
}
#endif /* DXS_USE_SPI_CS_CALLBACK */


/**
   Wait for the SPI interface of the DXS to become accessible and set the
   DXS IRQ behaviour in the configuration register.

   After a reset via the RESET_N pin (hard reset), the host controller must
   first wait for the SPI interface to become accessible again and then
   configure the basic behavior of the DXS (the polarity of the interrupt
   line and behavior of the interrupt acknowledge) before it can access
   the DUSLIC XS.

   \param  pDev         Pointer to the device structure.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
static IFX_int32_t dxs_spi_setup(DXS_DEVICE_t *pDev)
{
   IFX_uint16_t nRegWrite  = 0,
                nRegRead   = 0;
   IFX_uint16_t nLoop      = 0,
                nRetry     = 0;
   IFX_int32_t  ret        = DXS_statusOk;

   if (pDev->nDevState & (DS_SPI_ACTIVE | DS_SPI_SETUP))
   {
      /* No need to configure more than once. */
      return DXS_statusOk;
   }
#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
   /* SPI access is not configured */
   if(pDev->pSpiDev == IFX_NULL)
   {
      pDev->nDevState &= ~DS_SPI_SETUP;
      RETURN_DEVSTATUS(DXS_statusSpiAccErr, IFX_NULL);
   }
#endif
#endif
	DXS_OS_MutexInit(&spi_dxt_mutex);

   /* Set flag that is tested above to get out of the recursion that happens
      when DXS_RegWrite() is called below. */
   pDev->nDevState |= DS_SPI_SETUP;

   nRegWrite |= (DXS_REG_CFG_SC_MD_SCON | DXS_REG_CFG_8BIT_EN);

   while (nLoop <= 400)
   {
      /* Write SPI configuration. */
      ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegWrite);
      if (ret != DXS_statusOk)
         break;
      /* Read configuration back. */
      ret = DXS_RegRead(pDev, DXS_HOST_CFG, &nRegRead);
      if (ret != DXS_statusOk)
         break;

      /* While the SPI is not accessible it returns 0xFFFF. During this
         we loop until we read a value which is different. */

      if (nRegWrite == nRegRead)
      {
         pDev->nDevState &= ~DS_SPI_SETUP;
         pDev->nDevState |= DS_SPI_ACTIVE;
         return ret;
      }

      if (nRegRead != 0xFFFF)
      {
         /* Retry up to 3 times when read and write do not match. */
         if (nRetry >= 3)
         {
            break;
         }
         nRetry++;
      }

      /*lint -save -e(62) 'udelay' incompatible types for operator ":" */
      /* Wait 5ms between register accesses. */
      DXS_OS_MSecSleep(5);
      nLoop++;
   };

   pDev->nDevState &= ~DS_SPI_SETUP;
   RETURN_DEVSTATUS(DXS_statusSpiAccErr, IFX_NULL);
}
