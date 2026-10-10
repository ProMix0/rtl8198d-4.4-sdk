/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_bbd.c
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_fw_headers.h"
#include "drv_dxs_access.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_alm.h"
#include "drv_dxs_init.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_utd.h"
#include "drv_dxs_mbx.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define BBD_TYPE_GLOBAL_V1    0x05000001  /* type 0x0500, version 0x0001 */
#define BBD_GLOBAL_MAGIC      0x42424435  /* 'BBD5' character sequence */
#define BBD_TOC_DXS_IBB       (1<<4)
#define BBD_TOC_DXS_CIBB      (1<<5)
#define BBD_TOC_DXS_IFB       (1<<6)
#define BBD_TOC_DXS_CIFB      (1<<7)
#define BBD_TOC_DXS_IB        (1<<8)
#define BBD_TOC_DXS_BB        (1<<9)
#define BBD_TOC_DXS_CBB       (1<<10)

#define BBD_DXS_MAGIC                        0x44585321 /* "DXS!" */

/** DUSLIC XS BBD blocks tags */
/* 0x1XXX tags : DUSLIC XS DC/AC Coefficients */
#define BBD_COMPATIBILITY_BLOCK_TAG          0x000C
#define BBD_DXS_MASTER_BLOCK                 0x000A
#define BBD_DXS_CRAM_BLOCK                   0x1001
#define BBD_DXS_CRAM2_BLOCK                  0x1020
#define BBD_DXS_SLIC_BLOCK                   0x1002
#define BBD_DXS_RING_CFG_BLOCK               0x1003
#define BBD_DXS_DC_BASIC_CFG_BLOCK           0x1005
#define BBD_DXS_UTD_BLOCK                    0x1007
#define BBD_DXS_MWL_BLOCK                    0x1008
#define BBD_DXS_DCDC_BLOCK                   0x1009
#define BBD_DXS_WL_CMD_WRITE                 0x1200
#define BBD_DXS_END_BLOCK                    0x0000

/* maximum expected occurance of a single block type per BBD file,
   can be set also at compile time  */
#ifndef BBD_DXS_BLOCK_MAXDWNLD
#define BBD_DXS_BLOCK_MAXDWNLD                     10
#endif /* BBD_DXS_BLOCK_MAXDWNLD */

/* defines to extract coefficients from bbd block bytes */
#define BBD_DXS_RING_TRIP_TYPE_COEF                0x30
#define BBD_DXS_RING_TRIP_TYPE_COEF_SHIFT          4
#define BBD_DXS_RING_SIG_COEF                      0x08
#define BBD_DXS_RING_SIG_COEF_SHIFT                3
#define BBD_DXS_RING_CREST_COEF                    0x07

#define BBD_DXS_BASIC_CFG_ONHOOK_DUP_COEF          0xF0
#define BBD_DXS_BASIC_CFG_ONHOOK_DUP_COEF_SHIFT    4
#define BBD_DXS_BASIC_CFG_ACTIVE_DUP_COEF          0x0F
#define BBD_DXS_BASIC_CFG_ACTIVE_DUP_COEF_SHIFT    0
#define BBD_DXS_BASIC_CFG_GS_DUP_COEF              0xF0
#define BBD_DXS_BASIC_CFG_GS_DUP_COEF_SHIFT        4
#define BBD_DXS_BASIC_CFG_GNDK_DUP_COEF            0xF0
#define BBD_DXS_BASIC_CFG_GNDK_DUP_COEF_SHIFT      4
#define BBD_DXS_BASIC_CFG_ESD_DUP_COEF             0x0F
#define BBD_DXS_BASIC_CFG_ESD_DUP_COEF_SHIFT       0
#define BBD_DXS_BASIC_CFG_OVT_DUP_COEF             0x0F
#define BBD_DXS_BASIC_CFG_OVT_DUP_COEF_SHIFT       0
#define BBD_DXS_BASIC_CFG_GS_EN                    0x80
#define BBD_DXS_BASIC_CFG_GS_EN_SHIFT              7
#define BBD_DXS_BASIC_CFG_TTX_EN                   0x40
#define BBD_DXS_BASIC_CFG_TTX_EN_SHIFT             6
#define BBD_DXS_BASIC_CFG_AUTOBIAS_EN              0x20
#define BBD_DXS_BASIC_CFG_AUTOBIAS_EN_SHIFT        5
#define BBD_DXS_BASIC_CFG_IDLE_EXT_EN              0x08
#define BBD_DXS_BASIC_CFG_IDLE_EXT_EN_SHIFT        3
#define BBD_DXS_BASIC_CFG_LP_VOLTAGE_OFFSET        0xF0
#define BBD_DXS_BASIC_CFG_LP_VOLTAGE_OFFSET_SHIFT  4
#define BBD_DXS_BASIC_CFG_LP_BIAS_EN               0x01
#define BBD_DXS_BASIC_CFG_LP_BIAS_EN_SHIFT         0
#define BBD_DXS_BASIC_CFG_GR_FAULT_TRANS_EN        0x08
#define BBD_DXS_BASIC_CFG_GR_FAULT_TRANS_EN_SHIFT  3
#define BBD_DXS_BASIC_CFG_DCDC_OVH_COEF            0x0F
#define BBD_DXS_BASIC_CFG_DCDC_OVH_COEF_SHIFT      0
#define BBD_DXS_BASIC_CFG_T2G_DUP_COEF             0xF0
#define BBD_DXS_BASIC_CFG_T2G_DUP_COEF_SHIFT       4
#define BBD_DXS_BASIC_CFG_STBY_VOLT                0x06
#define BBD_DXS_BASIC_CFG_STBY_VOLT_SHIFT          1

#define BBD_DXS_DCDC_CFG_DCDC_OFF_C                0xF0
#define BBD_DXS_DCDC_CFG_DCDC_OFF_C_SHIFT          4
#define BBD_DXS_DCDC_CFG_DCDC_CAP                  0xF
#define BBD_DXS_DCDC_CFG_DCDC_CAP_SHIFT            0
#define BBD_DXS_DCDC_CFG_DCDC_HW                   0xF0
#define BBD_DXS_DCDC_CFG_DCDC_HW_SHIFT             4
#define BBD_DXS_DCDC_CFG_DCDC_SW_EN                0x4
#define BBD_DXS_DCDC_CFG_DCDC_SW_EN_SHIFT          2
#define BBD_DXS_DCDC_CFG_DCDC_COMB_LP              0x2
#define BBD_DXS_DCDC_CFG_DCDC_COMB_LP_SHIFT        1
#define BBD_DXS_DCDC_CFG_DCDC_SW_INV               1
#define BBD_DXS_DCDC_CFG_DCDC_SW_INV_SHIFT         0

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* Ring Configuration */
struct DXS_RingCfg
{
   /* reserved for alignment */
   IFX_uint16_t            reserved :2;
   /* ring trip type */
   IFX_uint16_t            ring_trip_type :2;
   /* ringing signal form (sinusoidal or trapezoid) */
   IFX_uint16_t            ring_sig :1;
   /* ring crest factor */
   IFX_uint16_t            ring_crest_factor :3;
   /* reserved for alignment */
   IFX_uint16_t            reserved_2 : 8;
   /* ring frequency */
   IFX_uint16_t            ring_freq;
   /* ring amplitude */
   IFX_uint16_t            ring_amp;
   /* ring hook level */
   IFX_uint16_t            ring_thres;
   /* ring dc offset */
   IFX_uint16_t            ring_dco;
   /* maximum ring current */
   IFX_uint16_t            ring_imax;
   /* ringing regulation coefficient */
   IFX_uint16_t            ring_regulation;
   /* minimum ringing voltage (peak) */
   IFX_uint16_t            ring_vmin;
   /* reserved for alignment */
   IFX_uint16_t            reserved_3;
   /* Fast Hook Threshold */
   IFX_uint16_t            ring_fast_thresh;
};

/* DC Basic Config */
struct DXS_DcBasicCfg
{
   /* dup time for hook de-bouncing in ONHOOK mode */
   IFX_uint16_t onhook_dup :4;
   /* dup time for hook de-bouncing in ACTIVE mode */
   IFX_uint16_t active_dup :4;
   /* dup time for hook de-bouncing in GS mode */
   IFX_uint16_t gs_dup :4;
   IFX_uint16_t reserved1 :4;

   /* dup time for GNDK de-bouncing */
   IFX_uint16_t gndk_dup :4;
   /* Emergency Shutdown Debounce Time */
   IFX_uint16_t esd_dup :4;
   /* ground start enable */
   IFX_uint16_t   gs_en :1;
   /* ttx enable */
   IFX_uint16_t   ttx_en :1;
   IFX_uint16_t   autobias : 1;
   IFX_uint16_t   reserved2 : 1;
   IFX_uint16_t   idle_ext : 1;
   IFX_uint16_t   reserved3: 3;

   IFX_uint16_t   lp_voltage_offset :4;
   /* dcdc overhead voltage */
   IFX_uint16_t   dcdc_ovh :4;
   /* dup time for groud key de-bouncing (T2G substate) */
   IFX_uint16_t   t2g_dup :4;
   IFX_uint16_t   gr_fault_dis_en :1;
   IFX_uint16_t   stby_volt  : 2;
   IFX_uint16_t   lp_bias  : 1;

   IFX_uint16_t   ttx_burst_length;
   /* Current Gain for PID regulator */
   IFX_uint16_t   dc_pid_gain;
   /* on-hook threshold in ACT mode */
   IFX_uint16_t   act_onhook_thresh;
   /* off-hook threshold in ACT mode */
   IFX_uint16_t   act_offhook_thresh;
   /* hook threshold in GS mode */
   IFX_uint16_t   gs_thresh;
   /* open loop voltage limit */
   IFX_uint16_t   voltage_limit;
   /* open loop current limit */
   IFX_uint16_t   current_limit;
   /* threshold for GNDK low */
   IFX_uint16_t   gklow_thresh;
   /* threshold for GNDK high */
   IFX_uint16_t   gkhigh_thresh;
};

/* DC/DC Configuration */
struct DXS_DcDcCfg
{
   IFX_uint16_t            dcdc_off_c :4;
   IFX_uint16_t            dcdc_cap :4;
   IFX_uint16_t            dcdc_hw :4;
   IFX_uint16_t            reserved_2 : 1;
   IFX_uint16_t            dcdc_sw_en : 1;
   IFX_uint16_t            dcdc_comb_lp : 1;
   IFX_uint16_t            dcdc_sw_inv : 1;
   IFX_uint16_t            dcdc_p1;
   IFX_uint16_t            dcdc_max_on_time : 8;
   IFX_uint16_t            dcdc_max_freq : 8;
   IFX_uint16_t            dcdc_on_time_stby : 8;
   IFX_uint16_t            dcdc_max_freq_stby : 8;
};

/* MWL Configuration */
struct DXS_MWL_Cfg
{
   IFX_uint16_t            mwl_voltage;
   IFX_uint16_t            mwl_thresh;
   IFX_uint16_t            mwl_slope;
   IFX_uint16_t            mwl_on_time: 8;
   IFX_uint16_t            mwl_off_time: 8;
};


struct DXS_CMD
{
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   CMD_HEAD_BE;
#else
   CMD_HEAD_LE;
#endif
};

union DXS_MSG
{
   IFX_uint32_t val[30];
   struct DXS_CMD cmd;
};

/* Header of BBD archive containing DUSLIC XS BBD files for
   multiple DC/DC converter types. (version 1, big endian) */
struct dxs_bbd_archive_header
{
   /* Type and Version identifier */
   IFX_uint32_t   nType;
   /* Length of the payload following this header */
   IFX_uint32_t   nLength;
   /* MAGIC value for endianess checking */
   IFX_uint32_t   nMagic;
   /* Table of contents indicating which BBD files are included. */
   IFX_uint32_t   nTOC;
   /* Offset of the BBD for DXS IBB within the payload section */
   IFX_uint32_t   nDxsIBBOffset;
   /* Length of the BBD for DXS IBB within the payload section */
   IFX_uint32_t   nDxsIBBLength;
   /* Offset of the BBD for DXS CIBB within the payload section */
   IFX_uint32_t   nDxsCIBBOffset;
   /* Length of the BBD for DXS CIBB within the payload section */
   IFX_uint32_t   nDxsCIBBLength;
   /* Offset of the BBD for DXS IFB within the payload section */
   IFX_uint32_t   nDxsIFBOffset;
   /* Length of the BBD for DXS IFB within the payload section */
   IFX_uint32_t   nDxsIFBLength;
   /* Offset of the BBD for DXS CIFB within the payload section */
   IFX_uint32_t   nDxsCIFBOffset;
   /* Length of the BBD for DXS CIFB within the payload section */
   IFX_uint32_t   nDxsCIFBLength;
   /* Offset of the BBD for DXS IB within the payload section */
   IFX_uint32_t   nDxsIBOffset;
   /* Length of the BBD for DXS IB within the payload section */
   IFX_uint32_t   nDxsIBLength;
   /* Offset of the BBD for DXS BB within the payload section */
   IFX_uint32_t   nDxsBBOffset;
   /* Length of the BBD for DXS BB within the payload section */
   IFX_uint32_t   nDxsBBLength;
   /* Offset of the BBD for DXS CBB within the payload section */
   IFX_uint32_t   nDxsCBBOffset;
   /* Length of the BBD for DXS CBB within the payload section */
   IFX_uint32_t   nDxsCCBLength;
};

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
/* registration of supported DXS bbd blocks
   downloadable channelwise */
static const IFX_uint16_t DXS_CH_BBD_Blocks[] =
{
   BBD_DXS_CRAM_BLOCK,
   BBD_DXS_CRAM2_BLOCK,
   BBD_DXS_SLIC_BLOCK,
   BBD_DXS_RING_CFG_BLOCK,
   BBD_DXS_DC_BASIC_CFG_BLOCK,
   BBD_DXS_UTD_BLOCK,
   BBD_DXS_MWL_BLOCK,
   BBD_DXS_DCDC_BLOCK
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

static IFX_int32_t dxs_bbd_DwldCram  (DXS_CHANNEL_t *pCh, IFX_uint32_t dest,
                                      bbd_block_t *bbd_cram);

static IFX_int32_t dxs_bbd_RingCfg (
                        DXS_CHANNEL_t *pCh,
                        struct DXS_RingCfg *p_ringCfg);

static IFX_int32_t dxs_bbd_DcBasicCfg (
                        DXS_CHANNEL_t *pCh,
                        struct DXS_DcBasicCfg *p_dcBasicCfg);

#ifdef DXS_FEAT_UTD
static IFX_int32_t dxs_bbd_UtdCoeff (
                        DXS_CHANNEL_t *pCh,
                        bbd_block_t *pBBDblock);
#endif /* (DXS_FEAT_UTD) */

static IFX_int32_t dxs_bbd_BlockHandler (
                        DXS_CHANNEL_t *pCh,
                        bbd_block_t *pBBDblock);

extern DXS_CHANNEL_t *DXS_GetNeighbourChannel (
                        DXS_CHANNEL_t *pCh);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Incrementally raise the charge pump voltage from 3.95V to 5.0V
   in steps of 0.35V

   \param  pCh          Pointer to the DXS channel structure.

   \return
   - DXS_statusOk
   - error code from DXS_CmdWrite
*/
static IFX_int32_t dxs_charge_pump_enable(DXS_CHANNEL_t *pCh)
{
   IFX_int32_t  ret = DXS_statusOk;

   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_IFB ||
       pCh->pALM->nDcDcType == DXS_DCDC_TYPE_CIFB ||
       pCh->pALM->nDcDcType == DXS_DCDC_TYPE_IB ||
       pCh->pALM->nDcDcType == DXS_DCDC_TYPE_CIB)
   {
      DXS_DEVICE_t           *pDev = pCh->pParent;
      DXS_FW_SYS_Control_t   *pFW_SysControl = &pDev->fw_sys_ctrl;

      if (pFW_SysControl->CP_EN != IFX_ENABLE ||
           pFW_SysControl->CP_VOLT < DXS_FW_SYS_Control_CP_VOLT_5_0V)
      {
         IFX_uint8_t cp_volt = DXS_FW_SYS_Control_CP_VOLT_3_95V;

         pFW_SysControl->CP_EN = IFX_ENABLE;

         /* incremental raise of change pump voltage */
         for ( ; cp_volt <= DXS_FW_SYS_Control_CP_VOLT_5_0V; cp_volt++)
         {
            pFW_SysControl->CP_VOLT = cp_volt;
            ret = DXS_CmdWrite(pDev,
                                (IFX_uint32_t *)(IFX_void_t *)pFW_SysControl);
            if (ret != DXS_statusOk)
               break;
         }
      }
   }

   return ret;
}

/**
   Download CRAM Coefficients on channel.

   \param  pCh          Pointer to the DXS channel structure.
   \param  dest         Destination 0 - ACSW, 1 - ACDC
   \param  bbd_cram     Pointer to BBD CRAM block.

   \return
   - DXS_statusOk
   - DXS_statusBbdCramErr

   \remark
   - It is assumed that the given CRAM bbd block is valid and that the data
     representation in the bbd block is according to BBD specification,
     as follows:

     \verbatim
     offset_16: 0xXX, 0xXX,
     data_16[]: 0xXX, 0xXX,
                0xXX, 0xXX,
                ...
     crc_16   : 0xXX, 0xXX
     \endverbatim
*/
static IFX_int32_t dxs_bbd_DwldCram  (DXS_CHANNEL_t *pCh, IFX_uint32_t dest,
                                      bbd_block_t *bbd_cram)
{
   IFX_int32_t       ret   = DXS_statusBbdCramErr;
   IFX_uint32_t      countWords, posWords = 0;
   IFX_uint8_t       lenMsgBytes, lenCramWords, *pByte;
   IFX_uint16_t      cram_offset;
   DXS_SDD_Coeff_t   cramCoef;
   IFX_uint16_t      BlockOffset;
   IFX_uint8_t       BlockLength;
   /* instruction I1 of SDD_Coeff message */
   IFX_uint8_t       sdd_coeff_i1[4] = {0};

   memset (&cramCoef, 0, sizeof(DXS_SDD_Coeff_t));
   /* read offset */
   DXS_cpb2w (&cram_offset, &bbd_cram->pData[0], sizeof(IFX_uint16_t));
   /* set CRAM payload pointer */
   pByte = &bbd_cram->pData[2];

   cramCoef.hdr.CMD   = DXS_CMD_CMD_SDD;
   cramCoef.hdr.CHAN  = pCh->nChannel - 1;
   cramCoef.hdr.MOD   = DXS_CMD_MOD_SDD;
   cramCoef.hdr.ECMD  = DXS_SDD_Coeff_ECMD;

   /* The block size in 16-bit words without offset and crc bytes. */
   countWords = (bbd_cram->size - 4) >> 1 ;

   /* write CRAM data */
   while (countWords > 0)
   {
      if (countWords > (DXS_SDD_Coeff_MAXLENGTH >> 1) - 1)
         lenMsgBytes = DXS_SDD_Coeff_MAXLENGTH;
      else
         lenMsgBytes = (countWords + 1 /*offset&length word*/) << 1;

      /* Ensure that the CRAM msg payload length is a multiple of 4 bytes. */
      lenMsgBytes = lenMsgBytes - (lenMsgBytes % 4);

      DXS_ASSERT(((IFX_size_t)(lenMsgBytes-2) <=
         (sizeof(cramCoef.data)-sizeof(IFX_uint16_t))));

      cramCoef.hdr.LENGTH = lenMsgBytes;

      /* The amount of words of cram buffer written in this message.
         Note that the amount of data is one 16-bit word less than
         lenMsgBytes, because the message starts with one 16-bit word
         containing block offset and length of coefficient block. */
      lenCramWords = (lenMsgBytes >> 1) - 1 /*offset&length word*/;

      /* Set the MSW16 of the I1 instruction */
      BlockOffset = cram_offset + posWords;
      BlockLength = lenCramWords;
      sdd_coeff_i1[0] = (dest & 1) << 7;
      sdd_coeff_i1[0] |= (BlockOffset >> 3);
      sdd_coeff_i1[1] = (BlockOffset & 7) << 5;
      sdd_coeff_i1[1] |= BlockLength;

      /* Copy the CRAM byte buffer into the firmware message while
         taking care of the endianess. Take special care
         of the I1 instruction. */
      memcpy (&sdd_coeff_i1[2], &pByte[posWords << 1],
                        (sizeof(IFX_uint8_t) << 1));
      DXS_cpb2dw (&cramCoef.data[0], 0, sdd_coeff_i1, sizeof(sdd_coeff_i1));
      DXS_cpb2dw (&cramCoef.data[1], 0, &pByte[(posWords+1) << 1],
                        (lenCramWords-1) << 1);
      /* write Data */
      ret = DXS_CmdWrite(pCh->pParent, (IFX_uint32_t *) &cramCoef);
      if (DXS_statusOk != ret)
         break;
      /* increment position */
      posWords += lenCramWords;
      /* Decrement the number of words still to be written. */
      countWords -= lenCramWords;
   }

   if (DXS_statusOk == ret)
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
   else
   {
      /* errmsg: Download of CRAM BBD block failed. */
      RETURN_STATUS(DXS_statusBbdCramErr, IFX_NULL);
   }
}


/**
   Set Ringing configuration on the given channel.

   \param  pCh          Pointer to the DXS channel structure.
   \param  p_ringCfg    Pointer to ring configuration.

   \return
   - DXS_statusOk
   - DXS_statusBbdRingErr
*/
static IFX_int32_t  dxs_bbd_RingCfg (DXS_CHANNEL_t *pCh,
                                     struct DXS_RingCfg *p_ringCfg)
{
   IFX_int32_t             ret         = DXS_statusOk;
   DXS_DEVICE_t           *pDev        = pCh->pParent;
   DXS_SDD_RingConfig_t    ringCfg;

   memset (&ringCfg, 0, sizeof (DXS_SDD_RingConfig_t));
   ringCfg.CMD     = DXS_CMD_CMD_SDD;
   ringCfg.MOD     = DXS_CMD_MOD_SDD;
   ringCfg.ECMD    = DXS_SDD_RingConfig_ECMD;
   ringCfg.CHAN    = pCh->nChannel - 1;
   ringCfg.LENGTH  = DXS_SDD_RingConfig_LENGTH;

   /* set the ring trip type */
   ringCfg.RingTripType    = p_ringCfg->ring_trip_type;
   /* set the ring signal form */
   ringCfg.WaveForm      = p_ringCfg->ring_sig;
   /* set the ring crest factor */
   ringCfg.CrestFact   = p_ringCfg->ring_crest_factor;
   /* set the ring frequency */
   ringCfg.Frequency   = p_ringCfg->ring_freq;
   /* set ring amplitude */
   ringCfg.Amplitude   = p_ringCfg->ring_amp;
   /* set the ring hook level */
   ringCfg.Thresh      = p_ringCfg->ring_thres;
   /* set the ring DC offset */
   ringCfg.DcOffset    = p_ringCfg->ring_dco;
   /* set the maximum ring current */
   ringCfg.Imax        = p_ringCfg->ring_imax;
   /* set the ringing regulation coefficient */
   ringCfg.RegCoeff       = p_ringCfg->ring_regulation;
   /* set the minimum ringing voltage */
   ringCfg.Vmin        = p_ringCfg->ring_vmin;
   /* set the fast hook threshold */
   ringCfg.FastThresh  = p_ringCfg->ring_fast_thresh;

   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)((IFX_void_t *)&ringCfg));

   if (DXS_statusOk == ret)
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
   else
   {
      /* errmsg: Download of RingCfg BBD block failed. */
      RETURN_STATUS(DXS_statusBbdRingErr, IFX_NULL);
   }
}


/**
   Set all the basic parameters for the Smart Device Driver.

   \param  pCh          Pointer to the DXS channel structure.
   \param  p_dcBasicCfg Pointer to DC basic configuration.

   \return
   - DXS_statusOk
   - DXS_statusBbdBasicErr
*/
static IFX_int32_t  dxs_bbd_DcBasicCfg (DXS_CHANNEL_t *pCh,
                                        struct DXS_DcBasicCfg *p_dcBasicCfg)
{
   IFX_int32_t             ret = DXS_statusOk;
   DXS_DEVICE_t           *pDev = pCh->pParent;
   DXS_SDD_BasicConfig_t   basicCfg;

   memset (&basicCfg, 0, sizeof (basicCfg));
   basicCfg.CMD     = DXS_CMD_CMD_SDD;
   basicCfg.MOD     = DXS_CMD_MOD_SDD;
   basicCfg.ECMD    = DXS_SDD_BasicConfig_ECMD;

   /* read, modify, write the configuration... */
   basicCfg.CHAN    = pCh->nChannel - 1;
   basicCfg.LENGTH  = DXS_SDD_BasicConfig_LENGTH;
   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)((IFX_void_t *)&basicCfg),
                            (IFX_uint32_t *)((IFX_void_t *)&basicCfg));
   if (DXS_statusOk != ret)
      goto error;

   basicCfg.ActiveDup         = p_dcBasicCfg->active_dup;
   basicCfg.GsDup             = p_dcBasicCfg->gs_dup;
   basicCfg.GndkDup           = p_dcBasicCfg->gndk_dup;
   basicCfg.EsdDup            = p_dcBasicCfg->esd_dup;
   basicCfg.AutoBiasEn        = p_dcBasicCfg->autobias;
   basicCfg.DcDcOvh           = p_dcBasicCfg->dcdc_ovh;
   basicCfg.StbyVolt          = p_dcBasicCfg->stby_volt;
   basicCfg.TtxBurstLength    = p_dcBasicCfg->ttx_burst_length;
   basicCfg.DcPidGain         = p_dcBasicCfg->dc_pid_gain;
   basicCfg.ActOnhookThresh   = p_dcBasicCfg->act_onhook_thresh;
   basicCfg.ActOffhookThresh  = p_dcBasicCfg->act_offhook_thresh;
   basicCfg.VoltageLimit      = p_dcBasicCfg->voltage_limit;
   basicCfg.CurrentLimit      = p_dcBasicCfg->current_limit;

   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)((IFX_void_t *)&basicCfg));

error:
   if (DXS_statusOk == ret)
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
   else
   {
      /* errmsg: Download of DC Basic BBD block failed. */
      RETURN_STATUS(DXS_statusBbdBasicErr, IFX_NULL);
   }
}


/**
   Set DC/DC converter configuration parameters.
   For a one channel device the flag for the combined DC/DC feature is cleared
   without error because there it makes no sense.
   In combined DC/DC operation mode the DCcontrol of channel 0 supplies also
   channel 1.
   The DC/DC operation mode is stored on a per channel basis. Line operations
   are only possible after configuration of the channel.
   In case of buck/boost operation mode the GPIOs 0 and 1 are reserved because
   they are used as control lines and must not be available via the GPIO API.

   \param  pCh          Pointer to the DXS channel structure.
   \param  p_dcdcCfg    Pointer to DC/DC configuration.

   \return
   - DXS_statusOk
   - DXS_statusBbdDcDcErr
*/
static IFX_int32_t  dxs_bbd_DcDcCfg (DXS_CHANNEL_t *pCh,
                                     struct DXS_DcDcCfg *p_dcdcCfg)
{
   IFX_int32_t             ret         = DXS_statusOk;
   DXS_DEVICE_t           *pDev        = pCh->pParent;
   DXS_SDD_DcDcConfig_t    dcdcCfg;
   enum DXS_DcDcType nNewDcDcType;

   memset (&dcdcCfg, 0, sizeof (DXS_SDD_DcDcConfig_t));
   dcdcCfg.CMD     = DXS_CMD_CMD_SDD;
   dcdcCfg.MOD     = DXS_CMD_MOD_SDD;
   dcdcCfg.ECMD    = DXS_SDD_DcDcConfig_ECMD;
   dcdcCfg.CHAN    = pCh->nChannel - 1;
   if (pDev->bDcDcHwCombined == IFX_TRUE)
   {
      /* For combined DC/DC operation send the command always to channel A. */
      dcdcCfg.CHAN = DXS_CMD_CHAN_A;
   }
   dcdcCfg.LENGTH  = DXS_SDD_DcDcConfig_LENGTH;
   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)((IFX_void_t *)&dcdcCfg),
                            (IFX_uint32_t *)((IFX_void_t *)&dcdcCfg));
   if (DXS_statusOk != ret)
      goto error;

   dcdcCfg.DcDcOffC = p_dcdcCfg->dcdc_off_c;
   dcdcCfg.DcDcCap = p_dcdcCfg->dcdc_cap;
   dcdcCfg.DcDcHw = p_dcdcCfg->dcdc_hw;
   dcdcCfg.DcDcSwEn = p_dcdcCfg->dcdc_sw_en;
   dcdcCfg.CombLP = p_dcdcCfg->dcdc_comb_lp;
   dcdcCfg.DcDcSwInv = p_dcdcCfg->dcdc_sw_inv;
   dcdcCfg.P1 = p_dcdcCfg->dcdc_p1;
   dcdcCfg.MaxOnTime = p_dcdcCfg->dcdc_max_on_time;
   dcdcCfg.MaxFreq = p_dcdcCfg->dcdc_max_freq;
   dcdcCfg.OnTimeStby = p_dcdcCfg->dcdc_on_time_stby;
   dcdcCfg.MaxFreqStby = p_dcdcCfg->dcdc_max_freq_stby;

   /* Block DC/DC type settings which do not match the configured HW type. */
   nNewDcDcType = (enum DXS_DcDcType)(dcdcCfg.DcDcHw + DXS_DCDC_TYPE_IBB);
   if (nNewDcDcType != nAllowedDcDcType)
   {
      /* errmsg: DC/DC converter type in the BBD download must match the
         DC/DC HW type set at driver configuration. */
      return DXS_statusBbdDcDcHwDiffers;
   }

   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)((IFX_void_t *)&dcdcCfg));
   if (DXS_statusOk != ret)
      goto error;

   /* Remember the type of DC/DC operation mode. This flag is tested
      by all functions before doing analog line operations. */
   pCh->pALM->nDcDcType = nNewDcDcType;
   /* Set flag to start an automatic calibration after BBD download. */
   pCh->pALM->bCalibrationNeeded = IFX_TRUE;

   if ((pDev->bDcDcHwCombined == IFX_TRUE) && (pDev->caps.nALI > 1))
   {
      DXS_CHANNEL_t *pOtherCh = IFX_NULL;

      /* Combined DC/DC operation mode on a multichannel device.
         Set also the DC/DC flags of the other channel. */

      pOtherCh = DXS_GetNeighbourChannel(pCh);

      pOtherCh->pALM->nDcDcType = nNewDcDcType;
      pOtherCh->pALM->bCalibrationNeeded = IFX_TRUE;
   }

   /* enable the charge pump for certain DCDC variants */
   ret = dxs_charge_pump_enable(pCh);

error:
   if (DXS_statusOk == ret)
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
   else
   {
      /* errmsg: Download of DC/DC BBD block failed. */
      RETURN_STATUS(DXS_statusBbdDcDcErr, IFX_NULL);
   }
}


/**
   Set Message Waiting lamp Indication (MWI) configuration on the given channel.

   \param  pCh          Pointer to the DXS channel structure.
   \param  p_mwlCfg    Pointer to MWI configuration.

   \return
   - DXS_statusOk
   - DXS_statusBbdRingErr
*/
static IFX_int32_t  dxs_bbd_MwlCfg (DXS_CHANNEL_t *pCh,
                                     struct DXS_MWL_Cfg *p_mwlCfg)
{
   IFX_int32_t             ret         = DXS_statusOk;
   DXS_DEVICE_t           *pDev        = pCh->pParent;
   DXS_SDD_MwlConfig_t     mwlCfg;

   memset (&mwlCfg, 0, sizeof (mwlCfg));
   mwlCfg.CMD     = DXS_CMD_CMD_SDD;
   mwlCfg.MOD     = DXS_CMD_MOD_SDD;
   mwlCfg.ECMD    = DXS_SDD_MwlConfig_ECMD;
   mwlCfg.CHAN    = pCh->nChannel - 1;
   mwlCfg.LENGTH  = DXS_SDD_MwlConfig_LENGTH;
   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)((IFX_void_t *)&mwlCfg),
                            (IFX_uint32_t *)((IFX_void_t *)&mwlCfg));

   if (DXS_statusOk != ret)
      goto error;

   mwlCfg.Voltage = p_mwlCfg->mwl_voltage;
   mwlCfg.Thresh = p_mwlCfg->mwl_thresh;
   mwlCfg.Slope = p_mwlCfg->mwl_slope;
   mwlCfg.OnTime = p_mwlCfg->mwl_on_time;
   mwlCfg.OffTime = p_mwlCfg->mwl_off_time;
   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)((IFX_void_t *)&mwlCfg));

error:
   if (DXS_statusOk == ret)
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
   else
   {
      /* errmsg: Download of MWL BBD block failed. */
      RETURN_STATUS(DXS_statusBbdMwlErr, IFX_NULL);
   }
}

#ifdef DXS_FEAT_UTD
/**
   Store UTD coefficients into the internal table.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pBBDblock    Pointer to BBD coefficient block.

   \return
   - DXS_statusOk
   - DXS_status
*/
static IFX_int32_t dxs_bbd_UtdCoeff (
                        DXS_CHANNEL_t *pCh,
                        bbd_block_t *pBBDblock)
{
   IFX_uint16_t nToneIdx;
   struct DXS_UTD_COEF_ENTRY UtdCoef;
   IFX_int32_t ret = DXS_statusOk;

   memset (&UtdCoef, 0, sizeof(UtdCoef));

   /* Tone index */
   DXS_cpb2w (&nToneIdx,
              &pBBDblock->pData[0], sizeof (IFX_uint16_t));
   /* UTD coefficients */
   DXS_cpb2w (&UtdCoef.BW,
              &pBBDblock->pData[2], sizeof (IFX_uint16_t));
   DXS_cpb2w (&UtdCoef.CF,
              &pBBDblock->pData[4], sizeof (IFX_uint16_t));
   DXS_cpb2w (&UtdCoef.NLEV,
              &pBBDblock->pData[6], sizeof (IFX_uint16_t));
   DXS_cpb2w (&UtdCoef.SLEV,
              &pBBDblock->pData[8], sizeof (IFX_uint16_t));
   DXS_cpb2w (&UtdCoef.DLEV,
              &pBBDblock->pData[10], sizeof (IFX_uint16_t));
   DXS_cpb2w (&UtdCoef.LPCOEF,
              &pBBDblock->pData[12], sizeof (IFX_uint16_t));
   UtdCoef.DUP = pBBDblock->pData[14]; /* single byte only */


   ret = DXS_UTD_CoeffAdd (nToneIdx, &UtdCoef);

   RETURN_STATUS (ret, IFX_NULL);
}
#endif /* (DXS_FEAT_UTD) */


/**
   Handle a bdd block, internally we'll dispatch depending on block type.
   This function is used within the context of a single channel.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pBBDblock    Pointer to a kernel level copy of the bbd block.

   \return
   - DXS_statusOk
   - DXS_statusBbdDwldErr
*/
static IFX_int32_t dxs_bbd_BlockHandler (DXS_CHANNEL_t *pCh,
                                         bbd_block_t *pBBDblock)
{
   struct DXS_RingCfg      ringCfg;
   struct DXS_DcBasicCfg   dcBasicCfg;
   struct DXS_DcDcCfg      dcdcCfg;
   struct DXS_MWL_Cfg      mwlCfg;
   IFX_int32_t ret = DXS_statusOk;

   /*lint -save -e40 */
   TRACE(DXS, DBG_LEVEL_LOW,
         ("%s - bbd block with tag 0x%04X passed\n",
         __FUNCTION__, pBBDblock->tag));
   /*lint -restore */
   switch (pBBDblock->tag)
   {
      case  BBD_DXS_CRAM_BLOCK:
         ret = dxs_bbd_DwldCram (pCh, DXS_SDD_Coeff_Dest_ACSW, pBBDblock);
         break;
      case BBD_DXS_CRAM2_BLOCK:
         ret = dxs_bbd_DwldCram (pCh, DXS_SDD_Coeff_Dest_ACDC, pBBDblock);
         break;
      case BBD_DXS_SLIC_BLOCK:
         /* do nothing */
         break;
      case BBD_DXS_RING_CFG_BLOCK:
         memset(&ringCfg, 0, sizeof (ringCfg));
         ringCfg.ring_trip_type =
            ((pBBDblock->pData[0] & BBD_DXS_RING_TRIP_TYPE_COEF) >>
             BBD_DXS_RING_TRIP_TYPE_COEF_SHIFT);
         ringCfg.ring_sig =
            ((pBBDblock->pData[0] & BBD_DXS_RING_SIG_COEF) >>
             BBD_DXS_RING_SIG_COEF_SHIFT);
         ringCfg.ring_crest_factor =
            (pBBDblock->pData[0] & BBD_DXS_RING_CREST_COEF);
         DXS_cpb2w (&ringCfg.ring_freq, &pBBDblock->pData[2],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_amp, &pBBDblock->pData[4],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_thres, &pBBDblock->pData[6],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_dco, &pBBDblock->pData[8],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_imax, &pBBDblock->pData[0xA],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_regulation, &pBBDblock->pData[0xC],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_vmin, &pBBDblock->pData[0xE],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&ringCfg.ring_fast_thresh, &pBBDblock->pData[0x12],
                    sizeof (IFX_uint16_t));
         ret = dxs_bbd_RingCfg (pCh, &ringCfg);
         break;

      case BBD_DXS_DC_BASIC_CFG_BLOCK:
         memset (&dcBasicCfg, 0, sizeof (dcBasicCfg));

         dcBasicCfg.onhook_dup =
         ((pBBDblock->pData[0] & BBD_DXS_BASIC_CFG_ONHOOK_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_ONHOOK_DUP_COEF_SHIFT);
         dcBasicCfg.active_dup =
         ((pBBDblock->pData[0] & BBD_DXS_BASIC_CFG_ACTIVE_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_ACTIVE_DUP_COEF_SHIFT);
         dcBasicCfg.gs_dup =
         ((pBBDblock->pData[1] & BBD_DXS_BASIC_CFG_GS_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_GS_DUP_COEF_SHIFT);
         dcBasicCfg.gndk_dup =
         ((pBBDblock->pData[2] & BBD_DXS_BASIC_CFG_GNDK_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_GNDK_DUP_COEF_SHIFT);
         dcBasicCfg.esd_dup =
         ((pBBDblock->pData[2] & BBD_DXS_BASIC_CFG_ESD_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_ESD_DUP_COEF_SHIFT);
         dcBasicCfg.gs_en =
         ((pBBDblock->pData[3] & BBD_DXS_BASIC_CFG_GS_EN) >>
          BBD_DXS_BASIC_CFG_GS_EN_SHIFT);
         dcBasicCfg.ttx_en =
         ((pBBDblock->pData[3] & BBD_DXS_BASIC_CFG_TTX_EN) >>
          BBD_DXS_BASIC_CFG_TTX_EN_SHIFT);
         dcBasicCfg.dcdc_ovh =
         ((pBBDblock->pData[4] & BBD_DXS_BASIC_CFG_DCDC_OVH_COEF) >>
          BBD_DXS_BASIC_CFG_DCDC_OVH_COEF_SHIFT);
         dcBasicCfg.t2g_dup =
         ((pBBDblock->pData[5] & BBD_DXS_BASIC_CFG_T2G_DUP_COEF) >>
          BBD_DXS_BASIC_CFG_T2G_DUP_COEF_SHIFT);

         switch (pBBDblock->version)
         {
            case 1:
               dcBasicCfg.idle_ext           = 1;
               dcBasicCfg.autobias           = 0;
               dcBasicCfg.lp_bias            = 0;
               dcBasicCfg.lp_voltage_offset  = 0;
               dcBasicCfg.gr_fault_dis_en    = 0;
               TRACE(DXS, DBG_LEVEL_HIGH,
                    ("DXS BBD Dwld: Basic Cfg block v1, "
                     "setting defaults for new parameters\n"));
               break;

            case 3:
               dcBasicCfg.autobias =
               ((pBBDblock->pData[3] & BBD_DXS_BASIC_CFG_AUTOBIAS_EN) >>
                BBD_DXS_BASIC_CFG_AUTOBIAS_EN_SHIFT);

               dcBasicCfg.idle_ext =
               ((pBBDblock->pData[3] & BBD_DXS_BASIC_CFG_IDLE_EXT_EN) >>
                BBD_DXS_BASIC_CFG_IDLE_EXT_EN_SHIFT);

               dcBasicCfg.lp_voltage_offset =
               ((pBBDblock->pData[4] & BBD_DXS_BASIC_CFG_LP_VOLTAGE_OFFSET) >>
                BBD_DXS_BASIC_CFG_LP_VOLTAGE_OFFSET_SHIFT);

               dcBasicCfg.lp_bias =
               ((pBBDblock->pData[5] & BBD_DXS_BASIC_CFG_LP_BIAS_EN) >>
                BBD_DXS_BASIC_CFG_LP_BIAS_EN_SHIFT);

               dcBasicCfg.gr_fault_dis_en =
               ((pBBDblock->pData[5] & BBD_DXS_BASIC_CFG_GR_FAULT_TRANS_EN) >>
                BBD_DXS_BASIC_CFG_GR_FAULT_TRANS_EN_SHIFT);

               TRACE(DXS, DBG_LEVEL_NORMAL,
                    ("DXS BBD Dwld ch%d: "
                     "Idle Ext %d, AutoBias %d, LpBias %d, LpVoltOff %d "
                     "GrFaultDisEn %d\n",
                     pCh->nChannel-1,
                     dcBasicCfg.idle_ext,
                     dcBasicCfg.autobias,
                     dcBasicCfg.lp_bias,
                     dcBasicCfg.lp_voltage_offset,
                     dcBasicCfg.gr_fault_dis_en));
               break;

            case 4:
               dcBasicCfg.autobias =
               ((pBBDblock->pData[3] & BBD_DXS_BASIC_CFG_AUTOBIAS_EN) >>
                BBD_DXS_BASIC_CFG_AUTOBIAS_EN_SHIFT);

               dcBasicCfg.stby_volt =
               ((pBBDblock->pData[5] & BBD_DXS_BASIC_CFG_STBY_VOLT) >>
                  BBD_DXS_BASIC_CFG_STBY_VOLT_SHIFT);

               TRACE(DXS, DBG_LEVEL_NORMAL,
                    ("DXS BBD Dwld ch%d: "
                     "AutoBias %d, StbyVolt %d\n",
                     pCh->nChannel-1,
                     dcBasicCfg.autobias,
                     dcBasicCfg.stby_volt));
               break;

            default:
               TRACE(DXS, DBG_LEVEL_HIGH,
                    ("DXS BBD Dwld: unknown Basic Cfg block version\n"));
               break;
         }

         DXS_cpb2w (&dcBasicCfg.ttx_burst_length, &pBBDblock->pData[6],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.dc_pid_gain, &pBBDblock->pData[8],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.act_onhook_thresh, &pBBDblock->pData[0xA],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.act_offhook_thresh, &pBBDblock->pData[0xC],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.gs_thresh, &pBBDblock->pData[0xE],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.voltage_limit, &pBBDblock->pData[0x10],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.current_limit, &pBBDblock->pData[0x12],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.gklow_thresh, &pBBDblock->pData[0x14],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&dcBasicCfg.gkhigh_thresh, &pBBDblock->pData[0x16],
                    sizeof (IFX_uint16_t));
         ret =  dxs_bbd_DcBasicCfg (pCh, &dcBasicCfg);
         break;
      case BBD_DXS_DCDC_BLOCK:
         memset(&dcdcCfg, 0, sizeof(dcdcCfg));
         dcdcCfg.dcdc_off_c =
            ((pBBDblock->pData[0] & BBD_DXS_DCDC_CFG_DCDC_OFF_C) >>
               BBD_DXS_DCDC_CFG_DCDC_OFF_C_SHIFT);
         dcdcCfg.dcdc_cap =
            ((pBBDblock->pData[0] & BBD_DXS_DCDC_CFG_DCDC_CAP) >>
               BBD_DXS_DCDC_CFG_DCDC_CAP_SHIFT);
         dcdcCfg.dcdc_hw =
            ((pBBDblock->pData[1] & BBD_DXS_DCDC_CFG_DCDC_HW) >>
               BBD_DXS_DCDC_CFG_DCDC_HW_SHIFT);
         dcdcCfg.dcdc_sw_en =
            ((pBBDblock->pData[1] & BBD_DXS_DCDC_CFG_DCDC_SW_EN) >>
               BBD_DXS_DCDC_CFG_DCDC_SW_EN_SHIFT);
         dcdcCfg.dcdc_comb_lp =
            ((pBBDblock->pData[1] & BBD_DXS_DCDC_CFG_DCDC_COMB_LP) >>
               BBD_DXS_DCDC_CFG_DCDC_COMB_LP_SHIFT);
         dcdcCfg.dcdc_sw_inv =
            ((pBBDblock->pData[1] & BBD_DXS_DCDC_CFG_DCDC_SW_INV) >>
               BBD_DXS_DCDC_CFG_DCDC_SW_INV_SHIFT);
         DXS_cpb2w (&dcdcCfg.dcdc_p1, &pBBDblock->pData[2],
                    sizeof (IFX_uint16_t));
         dcdcCfg.dcdc_max_on_time = pBBDblock->pData[4];
         dcdcCfg.dcdc_max_freq = pBBDblock->pData[5];
         dcdcCfg.dcdc_on_time_stby = pBBDblock->pData[6];
         dcdcCfg.dcdc_max_freq_stby = pBBDblock->pData[7];
         ret = dxs_bbd_DcDcCfg (pCh, &dcdcCfg);
         break;
      case BBD_DXS_MWL_BLOCK:
         memset(&mwlCfg, 0, sizeof(mwlCfg));
         DXS_cpb2w (&mwlCfg.mwl_voltage, &pBBDblock->pData[0],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&mwlCfg.mwl_thresh, &pBBDblock->pData[2],
                    sizeof (IFX_uint16_t));
         DXS_cpb2w (&mwlCfg.mwl_slope, &pBBDblock->pData[4],
                    sizeof (IFX_uint16_t));
         mwlCfg.mwl_on_time = pBBDblock->pData[6];
         mwlCfg.mwl_off_time = pBBDblock->pData[7];
         ret = dxs_bbd_MwlCfg (pCh, &mwlCfg);
         break;
#ifdef DXS_FEAT_UTD
      case BBD_DXS_UTD_BLOCK:
         ret = dxs_bbd_UtdCoeff(pCh, pBBDblock);
         break;
#endif /* DXS_FEAT_UTD */

      default:
         TRACE (DXS, DBG_LEVEL_HIGH, ("DXS device driver WARNING:"
                " unsupported block tag 0x%04X", pBBDblock->tag));
         /* parsing continues */
         break;
   }

   if (DXS_statusOk != ret)
   {
      /* errmsg: Downloading a BBD block failed. */
      RETURN_STATUS(DXS_statusBbdBlockErr, IFX_NULL);
   }
   else
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
}


/**
   Does a BBD download on a given channel.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pBBD         Pointer to user BBD buffer.

   \return
   - DXS_statusOk
   - DXS_statusParam
   - DXS_statusInvalCh
   - DXS_statusNoMem
   - DXS_statusBbdCorrupt
   - DXS_statusBbdBlockErr
*/
static IFX_int32_t dxs_bbd_download (DXS_CHANNEL_t *pCh, DXS_BBD_Download_t *pBBD)
{
   IFX_int32_t       ret = DXS_statusOk;
   DXS_DEVICE_t      *pDev = IFX_NULL;
   IFX_boolean_t     bBroadcast = IFX_FALSE;
   IFX_uint32_t      i, j, block_num;
   bbd_block_t       bbd_dxs_block;
   bbd_error_t       bbd_err;
   bbd_format_t      bbd;
   IFX_uint32_t      ch;

   /* in case DXS_BBD_Download is called via device ioctl, parameters
      should be validated */
   if ((IFX_NULL == pCh) || (IFX_NULL == pBBD))
      return DXS_statusParam;

   /* detect channel or device context */
   if (pCh->nChannel == 0)
   {
      /* device context - broadcast on all channels */

      bBroadcast = IFX_TRUE;
      pDev = (DXS_DEVICE_t *)(IFX_void_t *)pCh;

      /* check if there is any channel available for download */
      for (ch=0; ch < DXS_MAX_CH_NR; ch++)
      {
         if (pDev->pChannel[ch].pALM != IFX_NULL)
            break;
      }
      if (ch >= DXS_MAX_CH_NR)
      {
         /* errmsg: Resource not valid. Channel number out of range */
         RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);
      }
   }
   else
   {
      /* channel context - download on one channel only */

      /* currently the download only covers ALM channels - if the
         application tries to download on a channel which doesn't
         have an ALM we return with error. */
      if (pCh->pALM == IFX_NULL)
      {
         /* errmsg: Resource not valid. Channel number out of range */
         RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);
      }

      pDev = pCh->pParent;
   }

   /* initializations */
   memset (&bbd, 0, sizeof(bbd));
   memset (&bbd_dxs_block, 0, sizeof (bbd_dxs_block));

   if (pBBD->buf != IFX_NULL)
   {
      /* get local memory for bbd buffer */
      bbd.buf  = OS_MapBuffer(pBBD->buf, pBBD->size);
      if (bbd.buf == NULL)
      {
         /* errmsg: No memory could be allocated. */
         RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
      }
      /* set size */
      bbd.size = pBBD->size;
      /* check BBD Buffer integrity */
      bbd_err = bbd_check_integrity (&bbd, BBD_DXS_MAGIC);
      if (bbd_err != BBD_INTG_OK)
      {
         OS_UnmapBuffer(bbd.buf);
         /* errmsg: The BBD download buffer is corrupt. */
         RETURN_DEVSTATUS(DXS_statusBbdCorrupt, IFX_NULL);
      }
   }

   /* go through bbd buffer and download any channel block of relevance found,
      DXS_CH_BBD_Blocks is a constant array of 16 bit entries defining the
      different supported block types */
   block_num  = (sizeof (DXS_CH_BBD_Blocks) / sizeof (IFX_uint16_t));
   bbd_dxs_block.identifier = BBD_DXS_MAGIC;

   for (i = 0; i < block_num; i++)
   {
      /* the order of block search and handling is defined in
         const DXS_CH_BBD_Blocks array. */
      bbd_dxs_block.tag = DXS_CH_BBD_Blocks [i];

      /* sniff blocks of this tag upto maximum allowed */
      for (j = 0; j < BBD_DXS_BLOCK_MAXDWNLD ; j++)
      {
         /* look at block of this index and download it if available */
         bbd_dxs_block.index = j;
         bbd_get_block (&bbd, &bbd_dxs_block);
         if ((bbd_dxs_block.pData == IFX_NULL) || (bbd_dxs_block.size == 0))
         {
            break;
         }

         if (!bBroadcast)
         {
            /* do the download on a single channel */
            ret = dxs_bbd_BlockHandler(pCh, &bbd_dxs_block);
         }
         else
         {
            /* do the downloads on all ALM channels */

            /* ATTENTION, if we need to extend the download to other channel
               types, checks must be added per block if the selected channel
               has the required channel type! */
            for (ch=0; ch < DXS_MAX_CH_NR; ch++)
            {
               pCh = &pDev->pChannel[ch];

               if (pCh->pALM != IFX_NULL)
               {
                  ret = dxs_bbd_BlockHandler(pCh, &bbd_dxs_block);
               }
               if (DXS_statusOk != ret)
                  break;
            }
         }
         /* stop everything if the previous download went wrong */
         if (DXS_statusOk != ret)
            break;
      }
      /* stop everything if the previous download went wrong */
      if (DXS_statusOk != ret)
      {
         OS_UnmapBuffer(bbd.buf);
         /* errmsg: Downloading a BBD block failed. */
         RETURN_STATUS(DXS_statusBbdBlockErr, IFX_NULL);
      }
   }
   OS_UnmapBuffer(bbd.buf);

   if (DXS_SUCCESS(ret))
   {
      /* Do automatically a calibration on the channel on which the BBD was
         downloaded to adapt to new coefficients. */
      if (!bBroadcast)
      {
         if (pCh->pALM->bCalibrationNeeded == IFX_TRUE)
         {
            ret = DXS_ALM_Calibration (pCh);
         }
      }
      else
      {
         for (i=0; (i < pDev->caps.nALI) && DXS_SUCCESS(ret); i++)
         {
            pCh = &pDev->pChannel[i];
            if (pCh->pALM->bCalibrationNeeded == IFX_TRUE)
            {
               ret = DXS_ALM_Calibration (pCh);
            }

            if (!DXS_SUCCESS(ret))
            {
               break;
            }
         }
      }

      if (!DXS_SUCCESS(ret))
      {
         /* errmsg: Automatic calibration after BBD download failed. */
         ret = DXS_statusAutomaticCalibrationFailed;
      }
   }

   /* errmsg: Success, no error occurred. */
   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Translate the DC/DC converter type string into an enum type.

   \param  string       Pointer to C-style character string.

   \return Enum value corresponding to the string.
*/
enum DXS_DcDcType DXS_BBD_DcDcStringTranslate (const char *string)
{
   struct DcDcTranslation
   {
      const char * const name;
      enum DXS_DcDcType type;
   };

   struct DcDcTranslation DcDcTypes[] =
      {
         {"AUTO",   DXS_DCDC_TYPE_AUTO},
         {"EVAL",   DXS_DCDC_TYPE_EVAL},
         {"IBB12",  DXS_DCDC_TYPE_IBB},
         {"CIBB12", DXS_DCDC_TYPE_CIBB},
         {"IB12",   DXS_DCDC_TYPE_IB},
         {"CIB12",  DXS_DCDC_TYPE_CIB},
         {"BB48",   DXS_DCDC_TYPE_BB},
         {"CBB48",  DXS_DCDC_TYPE_CBB},
         {"IFB3",   DXS_DCDC_TYPE_IFB},
         {"IFB12",  DXS_DCDC_TYPE_IFB},
         {"CIFB12", DXS_DCDC_TYPE_CIFB},
         {"IBGD12", DXS_DCDC_TYPE_IBGD},
         {"CIBGD12",DXS_DCDC_TYPE_CIBGD},
         {"IBVD3",  DXS_DCDC_TYPE_IBVD},
         {"CIBVD3", DXS_DCDC_TYPE_CIBVD},
         {"IBVD12", DXS_DCDC_TYPE_IBVD},
         /* Allow also names without voltage detail. */
         {"IBB",    DXS_DCDC_TYPE_IBB},
         {"CIBB",   DXS_DCDC_TYPE_CIBB},
         {"IB",     DXS_DCDC_TYPE_IB},
         {"CIB",    DXS_DCDC_TYPE_CIB},
         {"BB",     DXS_DCDC_TYPE_BB},
         {"CBB",    DXS_DCDC_TYPE_CBB},
         {"IFB",    DXS_DCDC_TYPE_IFB},
         {"CIFB",   DXS_DCDC_TYPE_CIFB},
         {"IBGD",   DXS_DCDC_TYPE_IBGD},
         {"CIBGD",  DXS_DCDC_TYPE_CIBGD},
         {"IBVD",   DXS_DCDC_TYPE_IBVD},
         {"CIBVD",  DXS_DCDC_TYPE_CIBVD}
      };

   IFX_uint8_t i;

   for (i=0; i < (sizeof(DcDcTypes)/sizeof(struct DcDcTranslation)); i++)
   {
      if (strcmp(string, DcDcTypes[i].name) == 0)
      {
         return DcDcTypes[i].type;
      }
   }

   /* Nothing found */
   return DXS_DCDC_TYPE_NOTSET;
}

/**
   Does a BBD download on a given channel.

   This handles the ioctl FIO_DXS_BBD_DOWNLOAD to download a BBD buffer on
   one specified channel. The function first checks for unified BBD archive
   that contains BBD files for all supported DC/DC variants, then, depending
   on allowed DC/DC variant, selects the appropriate BBD file from archive
   and invokes the BBD download function.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pBBD         Pointer to user BBD buffer.

   \return
   - DXS_statusOk
   - DXS_statusParam
   - DXS_statusInvalCh
   - DXS_statusNoMem
   - DXS_statusBbdCorrupt
   - DXS_statusBbdBlockErr
   - DXS_statusBbdMissingInArchive
*/
IFX_int32_t DXS_BBD_Download (DXS_CHANNEL_t *pCh, DXS_BBD_Download_t *pBBD)
{
   if (pBBD != IFX_NULL && pBBD->buf != IFX_NULL)
   {
      struct dxs_bbd_archive_header  header;
      IFX_uint8_t  *pReadBuf = DXS_OS_MALLOC(sizeof(header));

      if (IFX_NULL == pReadBuf)
      {
         TRACE (DXS, DBG_LEVEL_HIGH, ("Cannot allocate buffer for BBD header.\n"));
         /* errmsg: No memory could be allocated. */
         RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
      }

      /* copy archive header from user space */
      DXS_OS_CpyUsr2Kern(pReadBuf, pBBD->buf, sizeof(header));
      DXS_cpb2dw((IFX_uint32_t *)&header, 0, pReadBuf, sizeof(header));
      DXS_OS_FREE(pReadBuf);

      /* Process the archive after verifying the header. */
      if (header.nType == BBD_TYPE_GLOBAL_V1 &&
          header.nMagic == BBD_GLOBAL_MAGIC &&
          header.nLength == header.nDxsIBBLength +
                            header.nDxsCIBBLength +
                            header.nDxsIFBLength +
                            header.nDxsCIFBLength +
                            header.nDxsIBLength +
                            header.nDxsBBLength +
                            header.nDxsCCBLength)
      {
         switch (nAllowedDcDcType)
         {
            case DXS_DCDC_TYPE_IBB:
               if ((header.nTOC & BBD_TOC_DXS_IBB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsIBBOffset;
               pBBD->size = header.nDxsIBBLength;
               break;

            case DXS_DCDC_TYPE_CIBB:
               if ((header.nTOC & BBD_TOC_DXS_CIBB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsCIBBOffset;
               pBBD->size = header.nDxsCIBBLength;
               break;

            case DXS_DCDC_TYPE_IFB:
               if ((header.nTOC & BBD_TOC_DXS_IFB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsIFBOffset;
               pBBD->size = header.nDxsIFBLength;
               break;

            case DXS_DCDC_TYPE_CIFB:
               if ((header.nTOC & BBD_TOC_DXS_CIFB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsCIFBOffset;
               pBBD->size = header.nDxsCIFBLength;
               break;

            case DXS_DCDC_TYPE_IB:
               if ((header.nTOC & BBD_TOC_DXS_IB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsIBOffset;
               pBBD->size = header.nDxsIBLength;
               break;

            case DXS_DCDC_TYPE_BB:
               if ((header.nTOC & BBD_TOC_DXS_BB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsBBOffset;
               pBBD->size = header.nDxsBBLength;
               break;

            case DXS_DCDC_TYPE_CBB:
               if ((header.nTOC & BBD_TOC_DXS_CBB) == 0)
               {
                  /* errmsg: BBD archive does not contain the needed BBD. */
                  RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
               }
               pBBD->buf += sizeof(header) + header.nDxsCBBOffset;
               pBBD->size = header.nDxsCCBLength;
               break;

            default:
               /* errmsg: BBD archive does not contain the needed BBD. */
               RETURN_STATUS(DXS_statusBbdMissingInArchive, IFX_NULL);
         }
      }
   }
   return dxs_bbd_download(pCh, pBBD);
}

/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
