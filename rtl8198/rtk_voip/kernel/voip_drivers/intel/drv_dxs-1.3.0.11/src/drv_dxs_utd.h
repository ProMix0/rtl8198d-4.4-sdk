#ifndef _DRV_DXS_UTD_H
#define _DRV_DXS_UTD_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_utd.c
   This file contains the declarations of the UTD (universal tone detector).
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_config.h"

#ifdef DXS_FEAT_UTD
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
struct DXS_UTD_COEF_ENTRY
{
   /** Bandwidth */
   IFX_uint16_t BW;
   /** Center Frequency */
   IFX_uint16_t CF;
   /** Noise Level */
   IFX_uint16_t NLEV;
   /** Signal Level */
   IFX_uint16_t SLEV;
   /** Delta Level */
   IFX_uint16_t DLEV;
   /** Low Pass Coefficient */
   IFX_uint16_t LPCOEF;
   /** Data Upstream Persistance Counter */
   IFX_uint8_t DUP;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_UTD_AllocateStorage (
                        IFX_void_t);

extern IFX_void_t  DXS_UTD_DeleteStorage (
                        IFX_void_t);

extern IFX_int32_t DXS_UTD_Allocate_Ch_Structures (
                        DXS_CHANNEL_t *pCh);

extern IFX_void_t  DXS_UTD_Free_Ch_Structures (
                        DXS_CHANNEL_t *pCh);

extern IFX_void_t  DXS_UTD_InitCh (
                        DXS_CHANNEL_t *pCh);

extern IFX_int32_t DXS_TAPI_LL_UTD_Start (
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TONE_SIMPLE_t const *pTone,
                        IFX_TAPI_TONE_CPTD_t const *pSignal);

extern IFX_int32_t DXS_TAPI_LL_UTD_Stop (
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TONE_CPTD_t const *pSignal);

extern IFX_uint16_t DXS_UTD_ToneIdxGet (
                        DXS_CHANNEL_t *pCh);

extern IFX_int32_t DXS_UTD_CoeffAdd (
                        IFX_uint16_t nToneIdx,
                        struct DXS_UTD_COEF_ENTRY *pUtdCoef);

#endif /* DXS_FEAT_UTD */
#endif /* _DRV_DXS_UTD_H */
