#include <linux/kernel.h>
#include "voip_types.h"
#include "voip_debug.h"
#include "con_register.h"
#include "snd_define.h"
//#include "zarlink_api.h"
//#include "snd_intel_common.h"
#include "con_ring.h"
#include <linux/delay.h>
#include "voip_params.h"
// --------------------------------------------------------
// zarlink fxs ops 
// --------------------------------------------------------


#include "drv_dxs_init.h"
#include "drv_dxs_api.h"
#include "drv_dxs_io_types.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_access.h" 	
#include "drv_dxs_bbd.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_pcm_priv.h"
#include "drv_dxs_fw_cmd_eop.h"
#include "drv_tapi_io.h"
#include "drv_dxs_alm_lt.h"				
#include "drv_dxs_alm.h"

extern IFX_int32_t kTAPI_Ring_SetCadenceHighRes(IFX_uint16_t dev, IFX_uint16_t chn,
	IFX_TAPI_RING_CADENCE_t *param);

extern IFX_int32_t kTAPI_Ring_Start(IFX_uint16_t dev, IFX_uint16_t chn);
extern IFX_int32_t kTAPI_Ring_Stop(IFX_uint16_t dev, IFX_uint16_t chn);
extern IFX_int32_t kTAPI_Phone_Set_Linefeed(IFX_uint16_t dev, IFX_uint16_t chn, IFX_TAPI_LINE_MODE_t mode);						
//extern IFX_int32_t kTAPI_Phone_PCM_Get_Config(IFX_uint16_t dev, IFX_uint16_t chn, IFX_TAPI_PCM_CFG_t *param);
						
extern IFX_TAPI_DRV_CTX_t *RtkCtx;
#define CB_TEST
#define NEWKTHREAD
extern int init_done;

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101
#define FXSNUM 1
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
#define FXSNUM 2
#endif

#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
typedef struct lineST_s{
	IFX_uint8_t linefeed;
	IFX_uint16_t dev;
	IFX_uint16_t chn;
	IFX_TAPI_LINE_MODE_t mode;
}lineST_t;

typedef struct rtkEn_s{
	uint8 enable;
	uint16 TS1;
	uint16 TS2;
}rtkEn_t;
#endif

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101)
static int dxsnum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR;
#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
extern uint32 rtkCh[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
extern lineST_t rtkLf[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
extern uint32 rtkRing[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
extern rtkEn_t rtkEn[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
#endif
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
static int dxsnum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR;
#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
extern uint32 rtkCh[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
extern lineST_t rtkLf[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
extern uint32 rtkRing[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
extern rtkEn_t rtkEn[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
#endif
#endif

//#define DEBUGFLAG

#define RETCHK(val) \
		if(!val){	\
			return -1; \
		}	\

#define VOIDCHK(val) \
		if(!val){	\
			return; \
		}	\

/* SDD events */
/** Over temperature detected */
#define EVT_SDD_OTEMP            0
/** Line testing finished */
#define EVT_SDD_LT_FIN           1
/** Ground fault detected */
#define EVT_SDD_GF               3
/** Ground key detected */
#define EVT_SDD_GK               4
/** Opmode changed */
#define EVT_SDD_OPC              5
/** Error in command SDD_CoeffReadConfig */
#define EVT_SDD_CORCE            6
/** Error in command SDD_Coeff */
#define EVT_SDD_COEFE            7
/** On-hook */
#define EVT_SDD_ONH              9
/** Off-hook */
#define EVT_SDD_OFFH             10
/** Ground Fault Finished */
#define EVT_SDD_GF_FIN           12
/** Ground Key Finished */
#define EVT_SDD_GK_FIN           13
/** Overtemp finished which means that
   SLIC temperature is back in normal range */
#define EVT_SDD_OTEMP_FIN        14
/** Operating Mode Ignored */
#define EVT_SDD_OMI              17
/** Operating Mode discarded */
#define EVT_SDD_OPM_DIS          18
/** Line testing aborted */
#define EVT_SDD_LT_ABORT         19
	
//extern int con_ch_num;
//extern const voip_con_t *get_const_con_ptr( uint32 cch );

static void FXS_Ring_intel(voip_snd_t *this, unsigned char ringset )  //ring
{
	int ringset_int = ringset;
	int index = 0;
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	IFX_int32_t ret = 0;
#endif

	VOIDCHK(this);
	VOIDCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	if (ringset_int == 1){
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
		ret = kTAPI_Ring_Start(pDev->nDevNr, ((pCh->nChannel) - 1));
#else
		index = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);
		rtkRing[index] = 1;
#endif
	}
	else if (ringset_int == 0){
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
		ret = kTAPI_Ring_Stop(pDev->nDevNr, ((pCh->nChannel) - 1));
#else
		index = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);
		rtkRing[index] = 2;
#endif
	}	
}

static unsigned char FXS_Check_Ring_intel(voip_snd_t *this)
{
	int ch = 0;
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	IFX_int32_t ret = 0;
	uint8 pCurrentOpmode = 0;
#endif

	RETCHK(this);
	RETCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = pCh->pParent;
	
	//PTRCHK(pCh);
	//PTRCHK(pDev);
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	ch = (pCh->nChannel - 1);
	ret = DXS_ALM_OpmodeGet(&pDev->pChannel[ch], &pCurrentOpmode);
	
	if( pCurrentOpmode == DXS_SDD_Opmode_RingBurst ||
		pCurrentOpmode == DXS_SDD_Opmode_RingRevpol){
		return 1;
	}
#else
	ch = (pDev->nDevNr * FXSNUM) + (pCh->nChannel-1);//nChannel starting with 1
	
	if( rtkCh[ch] == DXS_SDD_Opmode_RingBurst || 
		rtkCh[ch] == DXS_SDD_Opmode_RingRevpol){
		//printk("[%s]-[%d] ch %d, rtkCh[ch] %d\n", __func__, __LINE__,ch, rtkCh[ch]);
		return 1;
	}
	//printk("[%s]-[%d] ch %d, rtkCh[ch] %d\n", __func__, __LINE__,ch, rtkCh[ch]);
#endif //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	return 0;
}

static void Adjust_SLIC_Tx_Gain_intel(voip_snd_t *this, int tx_gain)
{
	DXS_CHANNEL_t *pCh = NULL;
	IFX_TAPI_LINE_VOLUME_t pVol;
	
	VOIDCHK(this);
	VOIDCHK(init_done);
	VOIDCHK(RtkCtx);

	pCh = (DXS_CHANNEL_t *)this->priv;
	
	memset(&pVol, 0, sizeof(IFX_TAPI_LINE_VOLUME_t));
	
	pVol.nGainTx = tx_gain;
#ifdef DXS_FEAT_NLT	
	pVol.nGainRx = pCh->pALM->rx_gain / 10;
#else
	pVol.nGainRx = pCh->pALM->rx_gain;
#endif
	
	RtkCtx->ALM.Volume_Set((IFX_TAPI_LL_CH_t *)this->priv, &pVol);
}

static void Adjust_SLIC_Rx_Gain_intel(voip_snd_t *this, int rx_gain)
{
	DXS_CHANNEL_t *pCh = NULL;
	IFX_TAPI_LINE_VOLUME_t pVol;
	
	VOIDCHK(this);
	VOIDCHK(init_done);
	VOIDCHK(RtkCtx);

	pCh = (DXS_CHANNEL_t *)this->priv;
	
	memset(&pVol, 0, sizeof(IFX_TAPI_LINE_VOLUME_t));
	
	pVol.nGainRx = rx_gain;
#ifdef DXS_FEAT_NLT	
	pVol.nGainTx = pCh->pALM->rx_gain / 10;
#else
	pVol.nGainTx = pCh->pALM->rx_gain;
#endif
	
	RtkCtx->ALM.Volume_Set((IFX_TAPI_LL_CH_t *)this->priv, &pVol);
}

static void SLIC_Set_Ring_Cadence_intel(voip_snd_t *this, unsigned short OnMsec, unsigned short OffMsec)
{	
	IFX_int32_t ret = 0, i = 0, onBitNum = 0, offBitNum = 0;
	IFX_char_t tmp = 0;
	IFX_TAPI_RING_CADENCE_t pCadence;
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	IFX_char_t data[IFX_TAPI_RING_CADENCE_MAX_BYTES] = {0};
	//IFX_char_t data[15] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00 };

	VOIDCHK(this);
	VOIDCHK(init_done);
	
	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;
	
	onBitNum = OnMsec/50;
	offBitNum = OffMsec/50;

	for(; i < onBitNum; i++){
		if((i % 8) == 0 && i != 0){
			data[i/8-1] = tmp;
			tmp = 0;
		}
		
		tmp |= (0x80 >> (i % 8));
	}
	
	if((i % 8) != 0)
		data[i/8] = tmp;
	else 
		data[i/8 - 1] = tmp;

	//for(i = 0; i < (onBitNum+offBitNum)/8; i++)
		//printk("data[%d] = %02x\n", i, data[i]);

	//memset(&pCadence, 0, sizeof(IFX_TAPI_RING_CADENCE_t));
	memset (&pCadence, 0, sizeof (pCadence));
	memcpy(&pCadence.data, data, sizeof (data));
	//pCadence.nr = (int)(sizeof (data) * 8);
	pCadence.nr = (int)(onBitNum+offBitNum);
	
	ret = kTAPI_Ring_SetCadenceHighRes(pDev->nDevNr, ((pCh->nChannel) - 1), &pCadence);
	
#ifdef DEBUGFLAG	
	if(ret == 0)
		printk("[%s]-[%d] set Cadence ok\n", __func__, __LINE__);
	else{
		printk("[%s]-[%d] set Cadence failed, ret = %x\n", __func__, __LINE__, ret);
	}
#endif	
}

static void SLIC_Set_Multi_Ring_Cadence_intel(voip_snd_t *this, unsigned short OnMsec1, unsigned short OffMsec1, unsigned short OnMsec2, unsigned short OffMsec2, unsigned short OnMsec3, unsigned short OffMsec3, unsigned short OnMsec4, unsigned short OffMsec4)
{

}

static void SLIC_Set_Ring_Voltage_Freq_intel(voip_snd_t *this, int index)
{

}

static void SLIC_Set_ImpedanceIndex(voip_snd_t *this, int index)
{

}

static void SLIC_Set_Ring_Freq_Amp_intel(voip_snd_t *this, char preset)
{

}

static void SLIC_Set_Impendance_Country_intel(voip_snd_t *this, unsigned short country, unsigned short impd)
{

}

static void SLIC_Set_Impendance_intel(voip_snd_t *this, unsigned short preset)
{

}

static void OnHookLineReversal_intel(voip_snd_t *this, unsigned char bReversal)
{
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	int index = 0;
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	IFX_int32_t ret = 0;
#endif

	VOIDCHK(this);
	VOIDCHK(init_done);
	VOIDCHK(RtkCtx);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	if(bReversal == 0)
		ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), IFX_TAPI_LINE_FEED_ACTIVE);
	else if(bReversal == 1)
		ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), IFX_TAPI_LINE_FEED_ACTIVE_REV);
	else
		printk("[%s] -[%d] Unknow Linefeed option\n", __func__, __LINE__);
#else
	index = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);
	rtkLf[index].linefeed = 1;
	rtkLf[index].dev = pDev->nDevNr;
	rtkLf[index].chn = ((pCh->nChannel) - 1);
	if(bReversal == 0)
		rtkLf[index].mode = IFX_TAPI_LINE_FEED_ACTIVE;
	else if(bReversal == 1)
		rtkLf[index].mode = IFX_TAPI_LINE_FEED_ACTIVE_REV;
	else
		printk("[%s] -[%d] Unknow Linefeed option\n", __func__, __LINE__);

#ifdef DEBUGFLAG
	printk("[%s] - [%d], index = %d, device# = %d, channel# = %d, ready to set linefeed mode%d\n", __func__, __LINE__, index, rtkLf[index].dev, rtkLf[index].chn, rtkLf[index].mode);
#endif

#endif //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	
}

static void SLIC_Set_LineVoltageZero_intel(voip_snd_t *this)
{
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;	
	int ret = 0;

	VOIDCHK(this);
	VOIDCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), IFX_TAPI_LINE_FEED_DISABLED);

#ifdef DEBUGFLAG
	if(ret != 0)
		printk("[%s]-[%d] IFX_TAPI_LINE_FEED_DISABLED setting failed, ret = %x\n", __func__, __LINE__, ret);
#endif	

}

static uint8 SLIC_CPC_Gen_intel(voip_snd_t *this)
{
	uint8 pre_linefeed = 0;

	return pre_linefeed;
}

static void SLIC_CPC_Check_intel(voip_snd_t *this, uint8 pre_linefeed)	// check in timer
{
	
}

/*  return value:
	0: Phone dis-connect, 
	1: Phone connect, 
	2: Phone off-hook, 
	3: Check time out ( may connect too many phone set => view as connect),
	4: Can not check, Linefeed should be set to active state first.
*/
//static inline unsigned char SLIC_Get_Hook_Status( int chid );

static inline unsigned int FXS_Line_Check_intel( voip_snd_t *this )	// Note: this API may cause watch dog timeout. Should it disable WTD? 
{

	RETCHK(this);
	RETCHK(init_done);

	if ( 1 == this ->fxs_ops ->SLIC_Get_Hook_Status( this, 1 ) )
	{
		//PRINT_MSG("%s: Phone 0ff-hook\n",__FUNCTION__);
		return 2;
	}

	return 4;
}


static void SendNTTCAR_intel( voip_snd_t *this )
{

}

static unsigned int SendNTTCAR_check_intel(voip_snd_t *this, unsigned long time_out)
{

	return 0;
}

static void disableOscillators_intel(voip_snd_t *this)
{
	printk("Not implemented!\n");
}

static void SetOnHookTransmissionAndBackupRegister_intel(voip_snd_t *this) // use for DTMF caller id 
{
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	int ret = 0;

	VOIDCHK(this);
	VOIDCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;
	
	ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), IFX_TAPI_LINE_FEED_ACTIVE);

#ifdef DEBUGFLAG
	if(ret != 0)
		printk("[%s]-[%d] IFX_TAPI_LINE_FEED_ACTIVE setting failed, ret = %x\n", __func__, __LINE__, ret);
#endif		

}

static inline void RestoreBackupRegisterWhenSetOnHookTransmission_intel(voip_snd_t *this) // use for DTMF caller id
{

}

#define PCMLAW_OFFSET	3

CT_ASSERT( BUSDATFMT_PCM_WIDEBAND_LINEAR - BUSDATFMT_PCM_LINEAR == PCMLAW_OFFSET );
CT_ASSERT( BUSDATFMT_PCM_WIDEBAND_ALAW - BUSDATFMT_PCM_ALAW == PCMLAW_OFFSET );
CT_ASSERT( BUSDATFMT_PCM_WIDEBAND_ULAW - BUSDATFMT_PCM_ULAW == PCMLAW_OFFSET );

static void SLIC_Set_PCM_state_intel(voip_snd_t *this, int enable) //Intel doesn't have this feature
{
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY

	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	int ch = 0, ret = 0;

	RETCHK(this);
	RETCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;
	IFX_TAPI_PCM_CFG_t pPCMConfig;

	pPCMConfig.ch = (pCh->nChannel) - 1;
	pPCMConfig.nTimeslotRX = this->TS1;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	pPCMConfig.nTimeslotTX = this->TS1 - 1;
#else
	pPCMConfig.nTimeslotTX = this->TS1;
#endif

	pPCMConfig.nHighway = 0;
	pPCMConfig.nSampleSwap = IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED;
	pPCMConfig.nBitPacking = IFX_TAPI_PCM_BITPACK_LSB;
	pPCMConfig.nBitOrder = IFX_TAPI_PCM_BITORDER_NORMAL;

#ifdef DEBUGFLAG
	printk("[%s]-[%d] enable:%d, ch:%d, RX_TS:%u, TX_TS:%u\n", __func__, __LINE__, enable, pPCMConfig.ch, pPCMConfig.nTimeslotRX, pPCMConfig.nTimeslotTX);
#endif

	ret = RtkCtx->PCM.Enable(pCh, 0, &pPCMConfig);

#ifdef DEBUGFLAG
	if(ret == 0)
		printk("[%s]-[%d] disable pcm OK\n", __func__, __LINE__);
	else
		printk("[%s]-[%d] disable pcm failed, ret = %x\n", __func__, __LINE__, ret);
#endif

	if(!enable)
		return 0;
	
	if (enable == 2) { /* Change to wideband mode */
#ifdef DEBUGFLAG
		printk("[%s]-[%d] Switch to Wideband mode, band = %d\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.COD);
#endif
		switch(pCh->pPCM->fw_pcm_ch.COD){
			case PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_ULAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_ALAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
				break;
			default:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
				printk("[%s]-[%d] Unknown codec %u\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.WIDE);
				break;
		}
	}
	else if (enable == 1) { /* Change to narrow band mode */
#ifdef DEBUGFLAG
		printk("[%s]-[%d] Switch to Narrow band mode, band = %d\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.COD);
#endif
		switch(pCh->pPCM->fw_pcm_ch.COD){
			case PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_ULAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_ALAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
				break;
			default:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
				printk("[%s]-[%d] Unknow codec %u\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.WIDE);
				break;
		}
	}
	else{
#ifdef DEBUGFLAG
		printk("[%s]-[%d] no need to change band mode\n", __func__, __LINE__);
#endif
		//return 0;
	}

	ret = RtkCtx->PCM.Cfg(pCh, &pPCMConfig);
#ifdef DEBUGFLAG
	if(ret == 0)
		printk("[%s]-[%d] set pcm codec %u OK\n", __func__, __LINE__, pPCMConfig.nResolution);
	else
		printk("[%s]-[%d] set pcm codec %u failed, ret = %x\n", __func__, __LINE__, pPCMConfig.nResolution, ret);
#endif
	ret = RtkCtx->PCM.Enable(pCh, 1, &pPCMConfig);
#endif
}

static unsigned char SLIC_Get_Hook_Status_intel(voip_snd_t *this, int directly)
{
	unsigned char status = 0;
	DXS_CHANNEL_t *pCh = NULL;

	RETCHK(this);
	RETCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;

	if(pCh->sddEvent & (1 << EVT_SDD_OFFH)){
		return 1;
	}
	else if(pCh->sddEvent & (1 << EVT_SDD_ONH)){
		return 0;
	}
	
	return status;
}

static void SLIC_Set_Power_Save_Mode_intel(voip_snd_t *this)
{
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	IFX_int32_t ret = 0;

	VOIDCHK(this);
	VOIDCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), IFX_TAPI_LINE_FEED_STANDBY);

#ifdef DEBUGFLAG
	if(ret != 0)
		printk("[%s]-[%d] IFX_TAPI_LINE_FEED_STANDBY setting failed, ret = %x\n", __func__, __LINE__, ret);
#endif
#else //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	int index = 0;

	VOIDCHK(this);
	VOIDCHK(init_done);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	index = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);
	rtkLf[index].linefeed = 1;
	rtkLf[index].dev = pDev->nDevNr;
	rtkLf[index].chn = ((pCh->nChannel) - 1);
	rtkLf[index].mode = IFX_TAPI_LINE_FEED_STANDBY;

#ifdef DEBUGFLAG
	printk("[%s] - [%d], index = %d, device# = %d, channel# = %d, ready to set linefeed mode%d\n", __func__, __LINE__, index, rtkLf[index].dev, rtkLf[index].chn, rtkLf[index].mode);
#endif

#endif //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
}

/* state: 
	0: line in-active state
	1: line active state
	2: power save state
	3: OHT
	4: OHT polrev
	5: Ring
*/
static void SLIC_Set_FXS_Line_State_intel(voip_snd_t *this, int state)
{
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	int index = 0;
#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	IFX_int32_t ret = 0;
#endif

	VOIDCHK(this);
	VOIDCHK(init_done);	

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	switch (state)
	{
		case 0:
			state = IFX_TAPI_LINE_FEED_DISABLED;
			break;
		case 1:
			state = IFX_TAPI_LINE_FEED_ACTIVE;
			break;
		case 2:
			state = IFX_TAPI_LINE_FEED_STANDBY;
			break;
		case 3:
			state = IFX_TAPI_LINE_FEED_ACTIVE;
			break;
		case 4:
			state = IFX_TAPI_LINE_FEED_ACTIVE_REV;//OHT_polrev
			break;
		case 5:
			state = IFX_TAPI_LINE_FEED_RING_BURST;
			break;
		case 6:
			state = IFX_TAPI_LINE_FEED_ACTIVE_REV;
			break;
		case 7:
			state = IFX_TAPI_LINE_FEED_STANDBY;
			break;
		case 8:
			state = IFX_TAPI_LINE_FEED_DISABLED; 			
			break;
		case 9:
			state = IFX_TAPI_LINE_FEED_DISABLED;//shut down dcdc
			break;			
		default:
			printk("[%s]-[%d] unknown case\n", __func__, __LINE__);
			state = IFX_TAPI_LINE_FEED_ACTIVE;
			break;
	}

#ifdef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ((pCh->nChannel) - 1), state);

#ifdef DEBUGFLAG
	if(ret != 0)
		printk("[%s]-[%d] %d setting failed, ret = %x\n", __func__, __LINE__, state, ret);
#endif

#else //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	index = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);
	rtkLf[index].linefeed = 1;
	rtkLf[index].dev = pDev->nDevNr;
	rtkLf[index].chn = ((pCh->nChannel) - 1);
	rtkLf[index].mode = state;
	
#ifdef DEBUGFLAG
	printk("[%s] - [%d], index = %d, device# = %d, channel# = %d, ready to set linefeed mode%d\n", __func__, __LINE__, index, rtkLf[index].dev, rtkLf[index].chn, rtkLf[index].mode);
#endif

#endif //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
}

static void SLIC_read_reg_intel(voip_snd_t *this, unsigned int num, unsigned char *len, unsigned char *val)
{
	return;
}

static void SLIC_write_reg_intel(voip_snd_t *this, unsigned int num, unsigned char *len, unsigned char *val)
{

	return;
}

static void SLIC_read_ram_intel(voip_snd_t *this, unsigned short num, unsigned int *val)
{
	printk("%s(%d)Not support yet!\n",__FUNCTION__,__LINE__);
}

static void SLIC_write_ram_intel(voip_snd_t *this, unsigned short num, unsigned int val)
{
	printk("%s(%d)Not support yet!\n",__FUNCTION__,__LINE__);
}

static void SLIC_dump_reg_intel(voip_snd_t *this)
{
	
}

static void SLIC_dump_ram_intel(voip_snd_t *this)
{
	printk("%s(%d)Not support yet!\n",__FUNCTION__,__LINE__);
}

static void FXS_FXO_DTx_DRx_Loopback_intel(voip_snd_t *this, voip_snd_t *daa_snd, unsigned int enable)
{
	printk( "Not implement FXS_FXO loopback\n" );
}

static void SLIC_OnHookTrans_PCM_start_intel(voip_snd_t *this)
{
	//this ->fxs_ops ->SLIC_Set_PCM_state(this, SLIC_PCM_ON);
	VOIDCHK(this);
	VOIDCHK(init_done);	
	
	this ->fxs_ops ->OnHookLineReversal(this, 0);		//Forward On-Hook Transmission
	PRINT_MSG("SLIC_OnHookTrans_PCM_start, ch = %d\n", this ->sch);
}

static int enable_intel( voip_snd_t *this, int enable )
{
#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	int ch = 0;

	RETCHK(this);
	RETCHK(init_done);
#if 1 //avoid scheduling while atomic
	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;
	ch = (pDev->nDevNr * FXSNUM) + ((pCh->nChannel) - 1);

	rtkEn[ch].enable = enable;
	rtkEn[ch].TS1 = this->TS1;
	rtkEn[ch].TS2 = this->TS2;
#else	
	pPCMConfig.ch = (pCh->nChannel) - 1;
	pPCMConfig.nTimeslotRX = this->TS1;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	pPCMConfig.nTimeslotTX = this->TS1 - 1;
#else
	pPCMConfig.nTimeslotTX = this->TS1;
#endif

	pPCMConfig.nHighway = 0;
	pPCMConfig.nSampleSwap = IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED;
	pPCMConfig.nBitPacking = IFX_TAPI_PCM_BITPACK_LSB;
	pPCMConfig.nBitOrder = IFX_TAPI_PCM_BITORDER_NORMAL;

#ifdef DEBUGFLAG
	printk("[%s]-[%d] enable:%d, ch:%d, RX_TS:%u, TX_TS:%u\n", __func__, __LINE__, enable, pPCMConfig.ch, pPCMConfig.nTimeslotRX, pPCMConfig.nTimeslotTX);
#endif

	if(enable != 0){
		ret = RtkCtx->PCM.Enable(pCh, 0, &pPCMConfig);
#ifdef DEBUGFLAG
		if(ret == 0)
			printk("[%s]-[%d] disable pcm OK\n", __func__, __LINE__);
		else
			printk("[%s]-[%d] disable pcm failed, ret = %x\n", __func__, __LINE__, ret);
#endif		
	}
	
	if (enable == 2) { /* Change to wideband mode */
#ifdef DEBUGFLAG	
		printk("[%s]-[%d] Switch to Wideband mode, band = %d\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.COD);
#endif
		switch(pCh->pPCM->fw_pcm_ch.COD){
			case PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_ULAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_ALAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
				break;				
			default:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
				printk("[%s]-[%d] Unknown codec %u\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.WIDE);
				break;
		}
	}
	else if (enable == 1) { /* Change to narrow band mode */
#ifdef DEBUGFLAG		
		printk("[%s]-[%d] Switch to Narrow band mode, band = %d\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.COD);
#endif
		switch(pCh->pPCM->fw_pcm_ch.COD){
			case PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_ULAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_ALAW_8BIT;
				break;
			case PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
				break;				
			default:
				pPCMConfig.nResolution =  IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
				printk("[%s]-[%d] Unknow codec %u\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.WIDE);
				break;
		}
	}
	else{
#ifdef DEBUGFLAG		
		printk("[%s]-[%d] no need to change band mode\n", __func__, __LINE__);
#endif
		return 0;
	}
	
	ret = RtkCtx->PCM.Cfg(pCh, &pPCMConfig);
#ifdef DEBUGFLAG	
	if(ret == 0)
		printk("[%s]-[%d] set pcm codec %u OK\n", __func__, __LINE__, pPCMConfig.nResolution);
	else
		printk("[%s]-[%d] set pcm codec %u failed, ret = %x\n", __func__, __LINE__, pPCMConfig.nResolution, ret);
#endif	
	ret = RtkCtx->PCM.Enable(pCh, 1, &pPCMConfig);

#ifdef DEBUGFLAG
	if(ret == 0)
		printk("[%s]-[%d] enable pcm codec %u OK\n", __func__, __LINE__, pPCMConfig.nResolution);
	else
		printk("[%s]-[%d] enable pcm codec %u failed, ret = %x\n", __func__, __LINE__, pPCMConfig.nResolution, ret);
#endif		
	this ->fxs_ops ->SLIC_Set_PCM_state( this, enable );
#endif //avoid scheduling while atomic	

#endif //CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
	return 0;
}

static void SLIC_set_param_intel(voip_snd_t *this, unsigned int slic_type, unsigned int param_type, unsigned char* pParam, unsigned int param_size)
{
	printk("%s(%d)Not support yet!\n",__FUNCTION__,__LINE__);
}

#if defined(CONFIG_RTK_VOIP_DRIVERS_INTEL_SLIC_LINETEST)
#if 0
static int SLIC_PortDetect_intel(voip_snd_t *this, unsigned int *val)
{
	int res = 0;

	return res;
}

static int SLIC_LineROH_intel(voip_snd_t *this, unsigned int *val)
{
	int res = 0;

	return res;
}

static int SLIC_LineVOLTAGE_intel(voip_snd_t *this, unsigned int *val)
{
	int res = 0;

	return res;
}

static int SLIC_LineRESFLT_intel(voip_snd_t *this, unsigned int *val)
{
	int res = 0;

	return res;
}
#endif


static int SLIC_LineTest_intel(voip_snd_t *this, uint16 tID, unsigned char *val)
{
	DXS_CHANNEL_t *pCh = NULL;
	IFX_TAPI_GR909_START_t p_start;
	IFX_TAPI_GR909_RESULT_t pResults;
	IFX_TAPI_NLT_CAPACITANCE_RESULT_t capResults;
	IFX_TAPI_CONTMEASUREMENT_GET_t pContMeas;
	int item = 0, ret = 0, count = 0;
	int *result = (int *)val;

	RETCHK(this);
	RETCHK(init_done);
	RETCHK(RtkCtx);	

	pCh = (DXS_CHANNEL_t *)this->priv;

	switch(tID){
		case LT_INTEL_TID_VOLTAGE:
		case LT_INTEL_TID_RES_FLT:
		case LT_INTEL_TID_ROH:
		case LT_INTEL_TID_REN:
			item = 0;
			break;
		case LT_INTEL_TID_CAPACITANCE:
			item = 1;
			break;
		case LT_INTEL_TID_DC_FEED_ST:
			item = 2;
			break;
		default:
			printk("[%s]-[%d] Unknown line test tID%d, ready to run GR909.\n", __func__, __LINE__, tID);
			item = 0;
			break;
	}

	if(item == 0){//GR909
		memset(&pResults, 0, sizeof(IFX_TAPI_GR909_RESULT_t));
		
		p_start.test_mask = IFX_TAPI_GR909_HPT|IFX_TAPI_GR909_FEMF|IFX_TAPI_GR909_RFT|IFX_TAPI_GR909_ROH|IFX_TAPI_GR909_RIT;
		
		ret = DXS_TAPI_LL_ALM_LT_GR909_Start ((IFX_TAPI_LL_CH_t *)this->priv, &p_start);
		
		if(ret != 0){
			printk("[%s]-[%d] GR909 start failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}

		while(count < 1000){
			if(pCh->sddEvent & (1 << EVT_SDD_LT_FIN))
				break;
			
			msleep(2);
			count++;
			
			if(count == 1000){
				printk("[%s]-[%d] GR909 timeout\n", __func__, __LINE__);
				goto done;
				//return -1;
			}
		}
		
		pCh->sddEvent &= ~(1 << EVT_SDD_LT_FIN);//reset flag to 0
		
		ret = DXS_TAPI_LL_ALM_LT_GR909_Result ((IFX_TAPI_LL_CH_t *)this->priv, &pResults);

		if(ret != 0){
			printk("[%s]-[%d] GR909 get result failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}

		switch(tID){
			case LT_INTEL_TID_VOLTAGE:
				result[0] = pResults.HPT_AC_T2R;
				result[1] = pResults.HPT_AC_R2G;
				result[2] = pResults.HPT_AC_T2G;
				result[3] = pResults.HPT_DC_T2R;
				result[4] = pResults.HPT_DC_R2G;
				result[5] = pResults.HPT_DC_T2G;
				result[6] = 0;
				result[7] = 0; 
				break;
			case LT_INTEL_TID_RES_FLT:
				result[0] = 0;
				result[1] = pResults.RFT_R2G;
				result[2] = pResults.RFT_T2G;
				result[3] = pResults.RFT_T2R;
				result[4] = 0;
				result[5] = 0;
				break;
			case LT_INTEL_TID_ROH:
				result[0] = 0;
				result[1] = 0;
				result[2] = pResults.ROH_T2R_L;
				result[3] = pResults.ROH_T2R_H;
				break;
			case LT_INTEL_TID_REN:
				result[0] = 0;
				result[1] = 0;
				result[2] = 0;
				result[3] = pResults.RIT_RES;
				result[4] = 0;
				result[5] = 0;			
				break;
			case LT_INTEL_TID_CAPACITANCE:
				
				break;
			default:
				printk("[%s]-[%d] GR909 unknown test tID%d, ready to get VOLTAGE\n", __func__, __LINE__, tID);
				result[0] = pResults.HPT_AC_T2R;
				result[1] = pResults.HPT_AC_R2G;
				result[2] = pResults.HPT_AC_T2G;
				result[3] = pResults.HPT_DC_T2R;
				result[4] = pResults.HPT_DC_R2G;
				result[5] = pResults.HPT_DC_T2G;
				result[6] = 0;
				result[7] = 0; 
				break;
		}
	}
	else if(item == 1){
		memset(&capResults, 0, sizeof(IFX_TAPI_NLT_CAPACITANCE_RESULT_t));
		
		ret = DXS_TAPI_LL_ALM_LT_CapMeasStart((IFX_TAPI_LL_CH_t *)this->priv, IFX_FALSE);
		
		if(ret != 0){
			printk("[%s]-[%d] CapMeas start failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}
		
		while(count < 1000){
			if(pCh->sddEvent & (1 << EVT_SDD_LT_FIN))
				break;
			
			msleep(2);
			count++;
			
			if(count == 1000){
				printk("[%s]-[%d] CapMeas timeout\n", __func__, __LINE__);
				goto done;
				//return -1;
			}
		}
		
		pCh->sddEvent &= ~(1 << EVT_SDD_LT_FIN);//reset flag to 0
		
		ret = DXS_TAPI_LL_ALM_LT_CapMeasResult((IFX_TAPI_LL_CH_t *)this->priv);

		if(ret != 0){
			printk("[%s]-[%d] Get capacitance measurement result failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}
		
		ret = DXS_TAPI_LL_ALM_NLT_Cap_Result((IFX_TAPI_LL_CH_t *)this->priv, &capResults);
		
		if(ret != 0){
			printk("[%s]-[%d] Reads the results of the capacitance measurement failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}
		
		result[0] = capResults.nCapRing2Gnd;
		result[1] = capResults.nCapTip2Gnd;
		result[2] = capResults.nCapTip2Ring;
		result[3] = 0;
		result[4] = 0;
	}
	else if(item == 2){
		memset(&pContMeas, 0, sizeof(IFX_TAPI_CONTMEASUREMENT_GET_t));
		
		ret = RtkCtx->ALM.ContMeasGet((IFX_TAPI_LL_CH_t *)this->priv, &pContMeas);
		
		if(ret != 0){
			printk("[%s]-[%d] Get measure results failed, ret = %x\n", __func__, __LINE__, ret);
			return ret;
		}

		result[0] = pContMeas.nILine;
		result[1] = 0;
		result[2] = pContMeas.nVLineDesired;
		result[3] = 0;
		result[4] = 0;
	}
	
	
done:
	if(item == 0){
		ret = DXS_TAPI_LL_ALM_LT_GR909_Stop((IFX_TAPI_LL_CH_t *)this->priv);
	}
	else if(item == 1){
		ret = DXS_TAPI_LL_ALM_LT_CapMeasStop((IFX_TAPI_LL_CH_t *)this->priv);
	}
#ifdef DEBUGFLAG	
	if(ret == 0)
		printk("[%s]-[%d] Test item%d Stop OK\n", __func__, __LINE__, item);
	else
		printk("[%s]-[%d] Test item%d Stop failed\n", __func__, __LINE__, item);
#endif

	if(item != 2)
		ret = RtkCtx->ALM.Line_Mode_Set((DXS_CHANNEL_t *)this->priv, IFX_TAPI_LINE_FEED_STANDBY, IFX_TAPI_LINE_FEED_STANDBY);

#ifdef DEBUGFLAG
	if(ret == 0)
		printk("[%s]-[%d] Test item%d  standby OK\n", __func__, __LINE__, item);
	else
		printk("[%s]-[%d] Test item%d  standby failed\n", __func__, __LINE__, item);
#endif
	
	return ret;
}

#endif

static int SLIC_GetCalibrationCoeff_intel(voip_snd_t *this, unsigned char *coeff)
{
	unsigned char status = 0;

	return status;
}

static int SLIC_SetCalibrationCoeff_intel(voip_snd_t *this, unsigned char *coeff)
{
	unsigned char status = 0;

	
	return status;
}

static int SLIC_DoCalibration_intel(voip_snd_t *this, unsigned char mode)
{
	unsigned char status = 0;

	return status;
}

#ifdef CONFIG_RTK_VOIP_SLIC_intel_886_SERIES
static int SLIC_MerteringPulse_intel(voip_snd_t *this, uint16 hz, uint16 onTime, uint16 offTime, uint16 numMeters)
{
	int res = 0;

	return res;
}
#endif

static void SLIC_Set_Time_Slot_intel( voip_snd_t *this, unsigned int ts)
{
#if 0
	DXS_CHANNEL_t *pCh = NULL;
	DXS_DEVICE_t *pDev = NULL;
	IFX_TAPI_PCM_CFG_t pPCMConfig;

	int ret = 0;

	INITCHK(init_done, NULL);

	PTRCHK(RtkCtx, NULL);

	pCh = (DXS_CHANNEL_t *)this->priv;
	pDev = (DXS_DEVICE_t *)pCh->pParent;

	ret = kTAPI_Phone_PCM_Get_Config(pDev->nDevNr, ((pCh->nChannel) - 1), &pPCMConfig);

	if(ret == TAPI_statusOk){
		pPCMConfig.nTimeslotRX = ts;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
		pPCMConfig.nTimeslotTX = ts - 1;
#else
		pPCMConfig.nTimeslotTX = ts;
#endif
		ret = RtkCtx->PCM.Enable(pCh, 0, &pPCMConfig);

		if(ret == TAPI_statusOk){
			ret = RtkCtx->PCM.Cfg(pCh, &pPCMConfig);

			if(ret == TAPI_statusOk){
				RtkCtx->PCM.Enable(pCh, 1, &pPCMConfig);
			}
		}
	}

#endif
}


// --------------------------------------------------------
// channel mapping architecture 
// --------------------------------------------------------

const snd_ops_fxs_t snd_intel_fxs_ops = {
	// common operation 
	.enable = enable_intel,
	
	// for each snd_type 
	//.SLIC_reset = SLIC_reset_intel,
	.FXS_Ring = FXS_Ring_intel,
	.FXS_Check_Ring = FXS_Check_Ring_intel,
	.FXS_Line_Check = FXS_Line_Check_intel,	// Note: this API may cause watch dog timeout. Should it disable WTD?
	.SLIC_Set_PCM_state = SLIC_Set_PCM_state_intel,
	.SLIC_Get_Hook_Status = SLIC_Get_Hook_Status_intel,
	.SLIC_Set_Power_Save_Mode = SLIC_Set_Power_Save_Mode_intel,
	.SLIC_Set_FXS_Line_State = SLIC_Set_FXS_Line_State_intel,
	
	.Set_SLIC_Tx_Gain = Adjust_SLIC_Tx_Gain_intel,
	.Set_SLIC_Rx_Gain = Adjust_SLIC_Rx_Gain_intel,
	.SLIC_Set_Ring_Cadence = SLIC_Set_Ring_Cadence_intel,
	.SLIC_Set_Multi_Ring_Cadence = SLIC_Set_Multi_Ring_Cadence_intel,
	.SLIC_Set_Ring_Freq_Amp = SLIC_Set_Ring_Freq_Amp_intel,
	.SLIC_Set_Impendance_Country = SLIC_Set_Impendance_Country_intel, 
	.SLIC_Set_Impendance = SLIC_Set_Impendance_intel,
	.OnHookLineReversal = OnHookLineReversal_intel,	//0: Forward On-Hook Transmission, 1: Reverse On-Hook Transmission
	.SLIC_Set_LineVoltageZero = SLIC_Set_LineVoltageZero_intel,
	
	.SLIC_CPC_Gen = SLIC_CPC_Gen_intel,
	.SLIC_CPC_Check = SLIC_CPC_Check_intel,	// check in timer
	
	.SendNTTCAR = SendNTTCAR_intel,
	.SendNTTCAR_check = SendNTTCAR_check_intel,
	
	.disableOscillators = disableOscillators_intel,
	
	.SetOnHookTransmissionAndBackupRegister = SetOnHookTransmissionAndBackupRegister_intel,	// use for DTMF caller id
	.RestoreBackupRegisterWhenSetOnHookTransmission = RestoreBackupRegisterWhenSetOnHookTransmission_intel,	// use for DTMF caller id
	
	.FXS_FXO_DTx_DRx_Loopback = FXS_FXO_DTx_DRx_Loopback_intel,
	.SLIC_OnHookTrans_PCM_start = SLIC_OnHookTrans_PCM_start_intel,
	.SLIC_set_param = SLIC_set_param_intel,
#if defined(CONFIG_RTK_VOIP_DRIVERS_INTEL_SLIC_LINETEST)
	.LineTest = SLIC_LineTest_intel,
#endif
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL)
	.MeteringPulse = NULL, //SLIC_MerteringPulse_intel,
#endif
	.SLIC_Set_Time_Slot     = SLIC_Set_Time_Slot_intel,
	.GetCalibrationCoeff	= SLIC_GetCalibrationCoeff_intel,
	.SetCalibrationCoeff	= SLIC_SetCalibrationCoeff_intel,
	.DoCalibration			= SLIC_DoCalibration_intel,
	
	// read/write register/ram
	.SLIC_read_reg = SLIC_read_reg_intel,
	.SLIC_write_reg = SLIC_write_reg_intel,
	.SLIC_read_ram = SLIC_read_ram_intel,
	.SLIC_write_ram = SLIC_write_ram_intel,
	.SLIC_dump_reg = SLIC_dump_reg_intel,
	.SLIC_dump_ram = SLIC_dump_ram_intel,
	
	//.SLIC_show_ID = ??
	.SLIC_Set_Ring_Voltage_Freq	= SLIC_Set_Ring_Voltage_Freq_intel,	
	.SLIC_Set_ImpedanceIndex	= SLIC_Set_ImpedanceIndex,	
};
