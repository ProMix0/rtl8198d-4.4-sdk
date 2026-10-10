#include <linux/delay.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include "voip_timer.h"
#include "vp_api.h"
#include "zarlinkCommonSlic.h"
#include "voip_params.h"

#undef DEBUG_API 

#if defined( CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES )
#include "ve880_api.h"
#endif

#if defined( CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES )
#include "ve886_api.h"
#include "Ve_profile.h"
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES )
#include "ve890_api.h"
#endif

#if defined(DEBUG_API)
#define DEBUG_API_PRINT() printk("%s(%d) line #%d\n",__FUNCTION__,__LINE__,pLine->ch_id);
#else
#define DEBUG_API_PRINT()
#endif

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
extern int zarlinkeventflag;
#endif

#define SUPPORT_CH_NUM  16
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
static int NTTCar1stCheckFlag[SUPPORT_CH_NUM] = {[0 ... SUPPORT_CH_NUM-1] = 0};
static unsigned long time_out_modify[SUPPORT_CH_NUM];
#endif

/*
** Global variable 
*/
int gCHId=0;

/*
**
*/
void rtkAddChannelIsInUse(int ch_id);
void rtkRemoveChannelIsInUse(int ch_id);
int rtkCheckChannelIsInUse(int ch_id);


/*
** External funcion
*/
extern RTKLineObj * rtkGetAnotherLine(RTKLineObj *pCurrentLine);


/*
** local variable 
*/
static int totalchannelID[SUPPORT_CH_NUM] = {0};

/*
** local function 
*/
static BOOL zarlinkFxsLineIsOffhook(VpLineCtxType *pLineCtx, VpDevCtxType *pDevCtx, int from_Polling_timer);
static int Conv2ZarlinkGain(int adj);

/*
*****************************************************************************
** FUNCTION:   zarlinkFxsLineIsOffhook
**
** PURPOSE:    Determine if a channel is on or off hook
**
** PARAMETERS: 
**
** RETURNS:    TRUE if offhook, FALSE if onhook
**
*****************************************************************************
*/
static BOOL zarlinkFxsLineIsOffhook(VpLineCtxType *pLineCtx, VpDevCtxType *pDevCtx, int from_polling_timer)
{
	bool lineStatus = FALSE;
	
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if ( ! zarlinkeventflag )
		return FALSE;
#endif	

	if ( VpGetLineStatus( pLineCtx, VP_INPUT_HOOK, &lineStatus ) == VP_STATUS_SUCCESS ) {
		if ( lineStatus == TRUE ) return TRUE;
		else 					  return FALSE;
		
	} else {

		PRINT_R("SLIC ERROR %s %d\n", __FUNCTION__, __LINE__);
	}

	return FALSE;
}

/* Move from ve890_api.c *****************************************************/

VpStatusType zarlinkSetFxsPcmMode(RTKLineObj *pLine, int pcm_mode)
{
	VpStatusType status;
	VpOptionCodecType codecType = getCodecType(pcm_mode);

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

	DEBUG_API_PRINT(); 

	status = VpSetOption( pLine->pLineCtx, VP_NULL, 
				  VP_OPTION_ID_CODEC, (void*)&codecType );   

	/* TODO if pcm_mode is changing from widebade to narrrowband. we should change AC profile here */
   
	if ( status == VP_STATUS_SUCCESS ) {
		pLine->codec_type = codecType;
		pLine->pcm_law_save = pcm_mode;
	}

	return status;
}

VpStatusType zarlinkSetRingVoltageFreq (
	RTKLineObj *pLine,
	int idx )
{
    VpStatusType status;
	SLICRING_VOLTAGE_FREQ index;
	VpOptionRingingParamsType ringParam = {.validMask = VP_OPTION_CFG_FREQUENCY | VP_OPTION_CFG_AMPLITUDE};
	index = (SLICRING_VOLTAGE_FREQ)idx;

	DEBUG_API_PRINT();

	switch ( index ){
		case SLIC_RING_90V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 90000;
			break;
		case SLIC_RING_80V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 80000;
			break;			
		case SLIC_RING_70V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 70000;
			break;			
		case SLIC_RING_60V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 60000;
			break;
		case SLIC_RING_50V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 50000;
			break;
		case SLIC_RING_40V_25HZ:
			ringParam.frequency = 25000;
			ringParam.amplitude = 40000;	
			break;
		case SLIC_RING_90V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 90000;
			break;
		case SLIC_RING_80V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 80000;		
			break;			
		case SLIC_RING_70V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 70000;
			break;			
		case SLIC_RING_60V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 60000;
			break;
		case SLIC_RING_50V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 50000;
			break;
		case SLIC_RING_40V_20HZ:
			ringParam.frequency = 20000;
			ringParam.amplitude = 40000;
			break;
		case SLIC_RING_90V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 90000;
			break;
		case SLIC_RING_80V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 80000;
			break;			
		case SLIC_RING_70V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 70000;
			break;			
		case SLIC_RING_60V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 60000;
			break;
		case SLIC_RING_50V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 50000;
			break;
		case SLIC_RING_40V_50HZ:
			ringParam.frequency = 50000;
			ringParam.amplitude = 40000;
			break;			
		default:
			ringParam.frequency = 25000;
			ringParam.amplitude = 70000;		
			break;
	}

	status = VpSetOption( pLine->pLineCtx, VP_NULL, 
		VP_OPTION_ID_RINGING_PARAMS, (void*)&ringParam );
		
	if ( status == VP_STATUS_SUCCESS ) {
		PRINT_MSG("[%s]set ring %dmv and %dmHz success.\n", __FUNCTION__, ringParam.amplitude, ringParam.frequency);
	}
	else{
		PRINT_MSG("[%s]set ring amplitude and frequency error\n", __FUNCTION__);
	}

	return status;	
}

VpStatusType zarlinkSetImpedanceIndex(
	RTKLineObj *pLine,
	int idx )
{
    VpStatusType status;
	VpProfilePtrType AC_profile;
	SLIC_IMPEDANCE_INDEX index;
	
	index = (SLIC_IMPEDANCE_INDEX)idx;
	PRINT_MSG("SetImpedanceIndex %d\n", index);
		
	switch ( index ){
		case SLIC_IMPEDANCE_US_600R:
			AC_profile = AC_FXS_RF14_600R_DEF;
			break;			
		case SLIC_IMPEDANCE_ETSI_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_ETSI;  
			break;			
		case SLIC_IMPEDANCE_TELCORDIA_GR57_900R_2P16UF:
			AC_profile =  AC_FXS_RF14_GR57; 
			break;			
		case SLIC_IMPEDANCE_AUSTRIA_220R_820R_115NF:
			AC_profile =  AC_FXS_RF14_AT;
			break;			
		case SLIC_IMPEDANCE_AUSTALIA_220R_820R_120NF:
			AC_profile =  AC_FXS_RF14_AU;
			break;			
		case SLIC_IMPEDANCE_BELGIUM_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_BE;
			break;			
		case SLIC_IMPEDANCE_BULGARIA_220R_820R_115NF:
			AC_profile =  AC_FXS_RF14_BG; 
			break;			
		case SLIC_IMPEDANCE_BRAZIL_900R:
			AC_profile =  AC_FXS_RF14_BR; 
			break;			
		case SLIC_IMPEDANCE_SWITZERLAND_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_CH; 
			break;			
		case SLIC_IMPEDANCE_CHINA_200R_680R_100NF:
			AC_profile =  AC_FXS_RF14_CN;
			break;			
		case SLIC_IMPEDANCE_GERMANY_220R_820R_115NF:
			AC_profile =  AC_FXS_RF14_DE;
			break;			
		case SLIC_IMPEDANCE_DENMARK_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_DK; 
			break;			
		case SLIC_IMPEDANCE_SPAIN_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_ES;
			break;			
		case SLIC_IMPEDANCE_FINLAND_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_FI;
			break;			
		case SLIC_IMPEDANCE_FRANCE_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_FR; 
			break;			
		case SLIC_IMPEDANCE_UK_370R_620R_310NF:
			AC_profile =  AC_FXS_RF14_GB;
			break;			
		case SLIC_IMPEDANCE_GREECE_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_GR; 
			break;			
		case SLIC_IMPEDANCE_HUNGARY_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_HU;
			break;			
		case SLIC_IMPEDANCE_IRELAND_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_IE;
			break;			
		case SLIC_IMPEDANCE_ISRAEL_270R_750R_150NF: 
			AC_profile =  AC_FXS_RF14_IL; 
			break;			
		case SLIC_IMPEDANCE_ICELAND_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_IS;
			break;			
		case SLIC_IMPEDANCE_ITALY_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_IT;
			break;			
		case SLIC_IMPEDANCE_JAPAN_600R_1UF:
			AC_profile =  AC_FXS_RF14_JP;
			break;			
		case SLIC_IMPEDANCE_NETHERLAND_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_NL; 
			break;			
		case SLIC_IMPEDANCE_NORWAY_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_NO;
			break;			
		case SLIC_IMPEDANCE_NEWZEALAND_370R_620R_310NF:
			AC_profile =  AC_FXS_RF14_NZ;
			break;			
		case SLIC_IMPEDANCE_PORTUGAL_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_PT;
			break;			
		case SLIC_IMPEDANCE_RUSSIA_150R_510R_47NF:
			AC_profile =  AC_FXS_RF14_RU; 
			break;			
		case SLIC_IMPEDANCE_SWEDEN_270R_750R_150NF:
			AC_profile =  AC_FXS_RF14_SE;
			break;			
		//case SLIC_IMPEDANCE_TURKEY_270R_750R_150NF:
			//AC_profile =  AC_FXS_RF14_TK;
			//break;			
		case SLIC_IMPEDANCE_SOUTH_AFRICA_220R_820R_115NF:
			AC_profile =  AC_FXS_RF14_ZA;
			break;

		default:
			AC_profile = AC_FXS_RF14_600R_DEF;			
			break;
	}

	status = VpConfigLine( pLine->pLineCtx, 
						   AC_profile,
						   VP_PTABLE_NULL,
						   VP_PTABLE_NULL );

	if ( status == VP_STATUS_SUCCESS ) {
		pLine->pAC_profile = AC_profile;
	}else
		PRINT_MSG("set impedance error\n");
	
	return status;	
}

VpStatusType zarlinkSetRingCadence (   
	RTKLineObj *pLine,
    unsigned short on_msec,
    unsigned short off_msec )
{
    VpStatusType status;
	uint8 * ring_cad;
    uint8 data[4];

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

	DEBUG_API_PRINT();

	ring_cad = (uint8*)pLine->pDev->pRing_cad_usr_profile;

	/* write cadence timer coeff. */
	data[0] = 0x20 + ((on_msec/5)>>8);
	data[1] = (on_msec/5);
	data[2] = 0x20 + ((off_msec/5)>>8);
	data[3] = (off_msec/5);
	
	ring_cad[RING_PROFILE_CAD_ON_H_IDX]  = data[0];
	ring_cad[RING_PROFILE_CAD_ON_L_IDX]  = data[1];
	ring_cad[RING_PROFILE_CAD_OFF_H_IDX] = data[2];
	ring_cad[RING_PROFILE_CAD_OFF_L_IDX] = data[3];
	
	if(pLine->polarity){
		ring_cad[RING_PROFILE_CAD_REVERSE_IDX]  = 0x0F;
		ring_cad[RING_PROFILE_CAD_POLARITY_IDX] = 0x0D;
	}
	else{
		ring_cad[RING_PROFILE_CAD_REVERSE_IDX]  = 0x07;
		ring_cad[RING_PROFILE_CAD_POLARITY_IDX] = 0x05;
	}
	
	
	//TODO should handle ling base ring cadence	
	status = VpInitRing( pLine->pLineCtx,
						 ring_cad,
						 VP_PTABLE_NULL);

	if ( status == VP_STATUS_SUCCESS ) {
		pLine->pRing_cad_profile = ring_cad;
		pLine->pDev->cad_on_ms   = (unsigned int)on_msec;
		pLine->pDev->cad_off_ms  = (unsigned int)off_msec;
	}

	return status;
}

#if 0
VpStatusType zarlinkSetRingFreqAmp(RTKLineObj *pLine, uint8 profile_id)
{
	VpProfilePtrType ring_profile;
	VpStatusType status;

    DEBUG_API_PRINT();

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)
	if (VP_DEV_880_SERIES == pLine->pDev->VpDevType)
		ring_profile = Ve880RingProfile(profile_id);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	if (VP_DEV_886_SERIES == pLine->pDev->VpDevType)
		ring_profile = Ve886RingProfile(profile_id);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	if (VP_DEV_890_SERIES == pLine->pDev->VpDevType)
		ring_profile = Ve890RingProfile(profile_id);
#endif

	status = VpConfigLine( pLine->pLineCtx,
						   VP_PTABLE_NULL,
						   VP_PTABLE_NULL,
						   ring_profile );

	pLine->pRing_profile = ring_profile;
	return status;
}
#endif

VpStatusType zarlinkFxsRing(RTKLineObj *pLine, uint8 enable)
{
	VpStatusType status;
#ifdef INTER_RING	
	RTKLineObj *pAnotherLine;
	VpLineStateType lineState;
#endif

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();
#ifdef INTER_RING
	pAnotherLine = rtkGetAnotherLine(pLine);

	if ( pAnotherLine != NULL ){
		//if another line is ringing		
		status = VpGetLineState(&pAnotherLine->pLineCtx, &lineState);
	    //if other line is in ringing state then this line needs to wait for CAD_BREAK event        
	    //so returning without setting this line to ringing state.        
	    if (lineState == VP_LINE_RINGING || lineState == VP_LINE_RINGING_POLREV) {            
			pAnotherLine->expectedEvent = VP_RING_CAD_BREAK;            
			//wait for CAD_BREAK event on another line before setting line to RINGING            
			PRINT_MSG("MARK(%d): VP_RING_CAD_BREAK\n", pAnotherLine->ch_id);            
			return;        
		}	
	}
#endif
	if (enable) 
	{
		if(pLine->polarity)
			status = VpSetLineState( pLine->pLineCtx, VP_LINE_RINGING_POLREV );
		else
			status = VpSetLineState( pLine->pLineCtx, VP_LINE_RINGING );
		//printk("VP_LINE_RINGING\n");
	}
	else
	{
		if (zarlinkFxsLineIsOffhook(pLine->pLineCtx, pLine->pDev->pDevCtx, 1))
		{
			if(pLine->polarity)
				status = VpSetLineState( pLine->pLineCtx, VP_LINE_OHT_POLREV);
			else
				status = VpSetLineState( pLine->pLineCtx, VP_LINE_OHT);
			//printk("VP_LINE_OHT\n");			
		}
		else
		{
			if(pLine->polarity)
				status = VpSetLineState( pLine->pLineCtx, VP_LINE_STANDBY_POLREV);				
			else
				status = VpSetLineState( pLine->pLineCtx, VP_LINE_STANDBY);
			//printk("VP_LINE_STANDBY\n");		
		}
	}

	return status;
}

VpStatusType zarlinkSendNTTCAR(RTKLineObj *pLine)
{
	VpStatusType status;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif


    DEBUG_API_PRINT();

	/*********** Change Setting To Create Short Ring *****************/
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)
	if (VP_DEV_880_SERIES == pLine->pDev->VpDevType)
		status = Ve880SetRingCadenceProfile(pLine, 1);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	if (VP_DEV_886_SERIES == pLine->pDev->VpDevType)
		status = Ve886SetRingCadenceProfile(pLine, 1);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	if (VP_DEV_890_SERIES == pLine->pDev->VpDevType)
		status = Ve890SetRingCadenceProfile(pLine, 1);
#endif

	/*********** Ring the FXS *************************/
	status = VpSetLineState( pLine->pLineCtx, VP_LINE_RINGING );

	return status;
}

unsigned char zarlinkSendNTTCAR_Check(RTKLineObj *pLine, unsigned long time_out)
{
	VpStatusType status = VP_STATUS_SUCCESS;
	uint32 hook_status;
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	unsigned int chid;
#endif

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return 0;
#endif

	
    DEBUG_API_PRINT();
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	chid = pLine->ch_id;

	if (chid > (SUPPORT_CH_NUM-1)){
		printk("%s, line%d, chid %d is over range(%d).\n", __FUNCTION__, __LINE__, chid, SUPPORT_CH_NUM);
		return 0;
	}

	if (NTTCar1stCheckFlag[chid] == 0)
	{
		NTTCar1stCheckFlag[chid] = 1;
		//printk("=1\n");
		time_out_modify[chid] = timetick + 6000;
	}
#endif

	/*********** Check Phone Hook State ***************/
	hook_status = zarlinkFxsLineIsOffhook(pLine->pLineCtx, 
						pLine->pDev->pDevCtx, 0);

	/* if phone on-hook */
	if (hook_status == 0) {
		/* time_after(a,b) returns true if the time a is after time b. */
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
		if (timetick_after(timetick,time_out_modify[chid]) ) 		{
#else
		if (timetick_after(timetick,time_out) ) 		{
#endif
			/* don't return 0, return 1, report time out don't wait */
			PRINT_MSG("wait off-hook timeout...\n");
		} else return 0;
	}

	/******* Set Reverse On-Hook Transmission *********/		

	/* if phone off-hook, set Reverse On-Hook Transmission */
	status = VpSetLineState( pLine->pLineCtx, VP_LINE_OHT_POLREV );

	/************** restore the register ***************/
	zarlinkSetRingCadence(pLine, pLine->pDev->cad_on_ms, pLine->pDev->cad_off_ms);
	
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	NTTCar1stCheckFlag[chid] = 0;
	//PRINT_Y("=0\n");
#endif

	PRINT_MSG("Set normal ring\n");

	return 1;
}

unsigned char zarlinkGetFxsHookStatus(RTKLineObj *pLine, int from_polling_timer)
{
	unsigned char hook=0;
	BOOL bHook;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return 0;
#endif


	bHook = (unsigned char)zarlinkFxsLineIsOffhook(pLine->pLineCtx, 
				pLine->pDev->pDevCtx, from_polling_timer);

	if (TRUE == bHook)
		hook = 1;

	#ifdef DEBUG_API
	//Ve890_dump_hook_map(pLine->ch_id, hook);
	#endif

	return hook;
}

unsigned char zarlinkGetLineState(RTKLineObj *pLine)
{
	VpStatusType status;
	VpLineStateType lineState;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return 0;
#endif

	
    DEBUG_API_PRINT();

   	status = VpGetLineState( pLine->pLineCtx, &lineState );

	return (unsigned char)lineState;
}

VpStatusType zarlinkSetLineState(RTKLineObj *pLine, VpLineStateType mode)
{
	VpStatusType status;
   	VpLineStateType lineState;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();

	switch ( mode )
	{
		case VP_LINE_DISCONNECT:
			lineState = VP_LINE_DISCONNECT;
			break;

		/* Standby mode */
		case VP_LINE_STANDBY:
		 	lineState = VP_LINE_STANDBY;
			break;

		/* On-hook transmission */
		case VP_LINE_OHT:
		 	/* Active mode supports OHT */
		 	lineState = VP_LINE_OHT;
			break;

		/* On-hook transmission reverse */
		case VP_LINE_OHT_POLREV:
		 	/* Active mode supports OHT AND reverse polarity */
		 	lineState = VP_LINE_OHT_POLREV;
			break;

		/* Ringing */
		case VP_LINE_RINGING:
		 	lineState = VP_LINE_RINGING;
			break;

		/* Loop current feed */
		case VP_LINE_TALK:
		 	lineState = VP_LINE_TALK;
			break;

		/* Reverse loop current feed */
		case VP_LINE_TALK_POLREV:
		 	lineState = VP_LINE_TALK_POLREV;
			break;

		/* Tip and Ring in high impedance state. Battery is disabled. Must call VpInitDevice() to recover */
		case VP_LINE_DISABLED:
			lineState = VP_LINE_DISABLED;
			break;

		/* Ringing Reverse  */
		case VP_LINE_RINGING_POLREV:
		 	lineState = VP_LINE_RINGING_POLREV;
			break;

		/* Standby Reverse mode */
		case VP_LINE_STANDBY_POLREV:
		 	lineState = VP_LINE_STANDBY_POLREV;
			break;			

		/* Active mode */
		case VP_LINE_ACTIVE:
		 	lineState = VP_LINE_ACTIVE;
			break;	
			
		/* Active Reverse mode */
		case VP_LINE_ACTIVE_POLREV:
		 	lineState = VP_LINE_ACTIVE_POLREV;
			break;			
			
		default:
		 	PRINT_R(" ERROR: unrecognized line state (%d)\n", mode);
		 	return VP_STATUS_INVALID_ARG;
			break;
	}

   	status = VpSetLineState( pLine->pLineCtx, lineState );

	return status;
}

VpStatusType zarlinkSetOHT(RTKLineObj *pLine, uint8 reversal)
{
	VpStatusType status;

#if 0// defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return ;
#endif

    DEBUG_API_PRINT();

	if (reversal) status = VpSetLineState( pLine->pLineCtx, VP_LINE_OHT_POLREV );
	else 		  status = VpSetLineState( pLine->pLineCtx, VP_LINE_OHT );

	return status;
}

VpStatusType zarlinkSetLineOpen(RTKLineObj *pLine)
{
	VpStatusType status;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();

	/* Disconnect line */
	status = VpSetLineState( pLine->pLineCtx, VP_LINE_DISCONNECT );

	return status;
}

VpStatusType zarlinkSetPcmTxOnly(RTKLineObj *pLine, int enable)
{
	VpStatusType status;
	VpOptionPcmTxRxCntrlType pcm_ctrl;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();

	if (enable == 1) {
		pcm_ctrl = VP_OPTION_PCM_TX_ONLY;
		//PRINT_MSG("Le89xxxSetPcmTxOnly!\n");
	} else if (enable == 0) {
		pcm_ctrl = VP_OPTION_PCM_BOTH;
		//PRINT_MSG("Le89xxxSetPcmBoth!\n");
	} else {
		PRINT_R("%s :Error config!\n",__FUNCTION__);
		return VP_STATUS_INVALID_ARG;
	}
	
	status = VpSetOption( pLine->pLineCtx, 
				VP_NULL, VP_OPTION_ID_PCM_TXRX_CNTRL, &pcm_ctrl);   

	return status;
}

VpStatusType zarlinkSetPcmRxOnly(RTKLineObj *pLine, int enable)
{
	VpStatusType status;
	VpOptionPcmTxRxCntrlType pcm_ctrl;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif
	
    DEBUG_API_PRINT();

	if (enable == 1) {
		pcm_ctrl = VP_OPTION_PCM_RX_ONLY;
		//PRINT_MSG("Le89xxxSetPcmRxOnly!\n");
	} else if (enable == 0) {
		pcm_ctrl = VP_OPTION_PCM_BOTH;
		//PRINT_MSG("Le89xxxSetPcmBoth!\n");
	} else {
		PRINT_R("%s :Error config!\n",__FUNCTION__);
		return VP_STATUS_INVALID_ARG;
	}
	
	status = VpSetOption( pLine->pLineCtx, 
				VP_NULL, VP_OPTION_ID_PCM_TXRX_CNTRL, &pcm_ctrl);   

	return status;
}

#if 0
VpStatusType zarlinkSetImpedence(RTKLineObj *pLine, uint16 preset)
{
	VpStatusType status;
	VpProfilePtrType ac_profile;
	
    DEBUG_API_PRINT();

	PRINT_MSG(" <<<<<<<<< Le89xxxSetImpedence >>>>>>>>>\n");

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_880_SERIES)
	if (VP_DEV_880_SERIES == pLine->pDev->VpDevType)
		ac_profile = Ve880AcProfile(preset);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	if (VP_DEV_886_SERIES == pLine->pDev->VpDevType)
		ac_profile = Ve886AcProfile(preset);
#endif

#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	if (VP_DEV_890_SERIES == pLine->pDev->VpDevType)
		ac_profile = Ve890AcProfile(preset);
#endif
	status = VpConfigLine( pLine->pLineCtx,
						   ac_profile,
						   VP_PTABLE_NULL,
						   VP_PTABLE_NULL );

	pLine->pAC_profile = ac_profile;
	return status;
}
#endif

unsigned char zarlinkCheckFxsRing(RTKLineObj *pLine)		
{
	unsigned char ringer = 0; //0: ring off, 1: ring on
	VpStatusType status;
	VpLineStateType lineState;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return 0;
#endif

    DEBUG_API_PRINT();

	/* Read 0xDF is also fine for ring-on/ring-off status */
   	status = VpGetLineStateINT( pLine->pLineCtx, &lineState );

	if ( lineState == VP_LINE_RINGING ||
       	 lineState == VP_LINE_RINGING_POLREV ) 
		ringer = 1;

	return ringer;

}

VpStatusType zarlinkTxGainAdjust(RTKLineObj *pLine, int gain)		
{
	VpStatusType status;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();

	//printk("%s(%d)chid %d, gain %d\n",__FUNCTION__,__LINE__,pLine->ch_id, gain);

	status = VpSetRelGain(pLine->pLineCtx,Conv2ZarlinkGain(gain),Conv2ZarlinkGain(pLine->RxGainAdj),pLine->ch_id); 

	PRINT_MSG("%s(%d)chid %d, gain %d, TX: 0x%x, RX: 0x%x\n",__FUNCTION__,__LINE__, 
		pLine->ch_id, gain, Conv2ZarlinkGain(gain), Conv2ZarlinkGain(pLine->RxGainAdj));

	if ( status == VP_STATUS_SUCCESS ) 
		pLine->TxGainAdj = gain;

	return status;
}

VpStatusType zarlinkRxGainAdjust(RTKLineObj *pLine, int gain)		
{
	VpStatusType status;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

    DEBUG_API_PRINT();

	//printk("%s(%d)chid %d, gain %d\n",__FUNCTION__,__LINE__,pLine->ch_id, gain);

	status = VpSetRelGain(pLine->pLineCtx,Conv2ZarlinkGain(pLine->TxGainAdj), Conv2ZarlinkGain(gain),pLine->ch_id); 

	PRINT_MSG("%s(%d)chid %d, gain %d, TX: 0x%x, RX: 0x%x\n",__FUNCTION__,__LINE__, 
		pLine->ch_id, gain, Conv2ZarlinkGain(pLine->TxGainAdj), Conv2ZarlinkGain(gain));

	if ( status == VP_STATUS_SUCCESS ) 
		pLine->RxGainAdj = gain;

	return status;
}

#ifdef CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES
VpStatusType zarlinkStartMeter(RTKLineObj *pLine, uint16 hz, uint16 onTime, uint16 offTime, uint16 numMeters)		
{
	VpStatusType status = VP_STATUS_SUCCESS;
	VpProfilePtrType pMeterProfile = DEF_LE886_METERING_12K_PROFILE;

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)
	if (rtkCheckChannelIsInUse(pLine->ch_id))
		return VP_STATUS_DEVICE_BUSY;
#endif

	if ( hz == 16)
		pMeterProfile = DEF_LE886_METERING_16K_PROFILE;

	status = VpInitMeter(pLine->pLineCtx, pMeterProfile);

	if ( status == VP_STATUS_SUCCESS ){
		status = VpStartMeter(pLine->pLineCtx, onTime, offTime, numMeters);
	}
	
	return status;
}
#endif

VpStatusType zarlinkSetSlicTimeSlot( RTKLineObj *pLine, unsigned int ts){
	VpOptionTimeslotType timeslot;
	VpStatusType status;

	timeslot.tx = (uint8)ts;
       timeslot.rx = (uint8)ts;

	status = VpSetOption( pLine->pLineCtx, VP_NULL,
		VP_OPTION_ID_TIMESLOT, (void*)&timeslot );

	if ( status != VP_STATUS_SUCCESS ) {
	  PRINT_R("VpSetOption for timeslot failed (%d) \n", status);
	}

	return status;
}

#if 0
#if 0
int GainTbl[]= {
    0xFEC9, 0xE314, 0xCA62, 0xB460, 0xA0C2, 0x8F47,         /*  12dB to   7dB */
  //0x7FB2, 0x71CF, 0x656E, 0x5A67, 0x5092, 0x47CF,         /*   6dB to   0dB */
	0x788D, 0x71CF, 0x656E, 0x5A67, 0x5092, 0x47CF,         /* 5.5dB to   0dB */
	0x4000,                                                 /*   0dB          */
	0x390A, 0x32D6, 0x2D4E, 0x2861, 0x23FD, 0x2013, 	    /*  -1dB to - 6dB */
	0x1C96, 0x197A, 0x16B5, 0x143D, 0x1209, 0x1013          /*  -7dB to -12dB */
	};
#else
int GainTbl[]= {
    0xFEC9, 0xE314, 0xCA62, 0xB460, 0xA0C2, 0x8F47,         /*  12dB to   7dB */
	0x7FB2, 0x71CF, 0x656E, 0x5A67, 0x5092, 0x47CF,         /*   6dB to   0dB */
	//0x788D, 0x71CF, 0x656E, 0x5A67, 0x5092, 0x47CF,         /* 5.5dB to   0dB */
	0x4000,                                                 /*   0dB          */
	0x390A, 0x32D6, 0x2D4E, 0x2861, 0x23FD, 0x2013, 	    /*  -1dB to - 6dB */
	0x1C96, 0x197A, 0x16B5, 0x143D, 0x1209, 0x1013          /*  -7dB to -12dB */
	};
#endif	
#else
int GainTbl[] = {
	0x1013, 0x1209, 0x143D, 0x16B5, 0x197A, 0x1C96, /* -12 dB ~ -7dB */
	0x2013, 0x23FD, 0x2861, 0x2D4E, 0x32D6, /* -6dB ~ -2dB */

	0x390A, 0x39B3, 0x3A5E, 0x3B0B,	0x3BBA, /* -1 ~ -0.6 dB */
	0x3C6B, 0x3D1E, 0x3DD3, 0x3E8B, 0x3F44, /* -0.5 ~ -0.1 dB */
	0x4000,  /* 0 dB */
	0x40BD, 0x417D, 0x423F, 0x4304, 0x43CA, /* 0.1 dB ~ 0.5 dB */
	0x4493, 0x455F, 0x462C, 0x46FC, 0x47CF,  /* 0.6 dB ~ 1 dB */
	
	0x5092, 0x5A67, 0x656E, 0x71CF, 0x7FB2, /*   1dB to   6dB */
	0x8F47, 0xA0C2, 0xB460, 0xCA62, 0xE314, 0xFEC9  /*   7dB to  12dB */	
};
#endif
static int Conv2ZarlinkGain(int adj)
{
	int nGainVal;

	nGainVal = adj;
#if 0
	if (adj > 12)  nGainVal=12;
	if (adj < -12) nGainVal=-12;

	return (GainTbl[12-nGainVal]);
#else
	return (GainTbl[adj]);
#endif
}

/* 
** API	  : getCodecType()
** Desp	  :	Convert Realtek pcm_mode to Zarlink code type
** input  : Realtek pcm_mode
** return : Zarlink codec type
*/
VpOptionCodecType getCodecType(BUS_DATA_FORMAT pcm_data_format)
{
	VpOptionCodecType codecType;

	switch ( pcm_data_format ) {
		case BUSDATFMT_PCM_LINEAR:
			codecType = VP_OPTION_LINEAR;
			break;

		case BUSDATFMT_PCM_ALAW:
			codecType = VP_OPTION_ALAW;
			break;

		case BUSDATFMT_PCM_ULAW:
			codecType = VP_OPTION_MLAW;
			break;

		case BUSDATFMT_PCM_WIDEBAND_LINEAR:
			codecType = VP_OPTION_WIDEBAND;
			break;

		case BUSDATFMT_PCM_WIDEBAND_ALAW:
			codecType = VP_OPTION_WIDEBAND;
			break;

		case BUSDATFMT_PCM_WIDEBAND_ULAW:
			codecType = VP_OPTION_WIDEBAND;
			break;

		default:
			codecType = VP_OPTION_LINEAR;
			break;
	}
	return ( codecType );
}

//RTKDevObj
VpStatusType zarlinkDumpDevReg (
	RTKDevObj *pDev )
{
	VpStatusType status = VP_STATUS_SUCCESS;
	VpRegisterDump (pDev->pDevCtx);
	return status;
}

VpStatusType zarlinkDumpDevObj (
	RTKLineObj *pLine, RTKDevObj *pDev )
{
	//VpObjectDump(VP_NULL,pDev->pDevCtx);
	return 0;
}

VpStatusType zarlinkDumpRegToFile (
	RTKDevObj *pDev, uint8** devReg, uint8 **ch0Reg, uint8 **ch1Reg )
{
	VpStatusType status = VP_STATUS_SUCCESS;
	VpRegisterDumpToFile (pDev->pDevCtx, devReg, ch0Reg, ch1Reg);
	return status;
}

static BOOL zarlinkWaitForEvent( RTKLineObj *pLine, VpEventCategoryType category, uint16 event)
{
	int i = 0;
	VpStatusType status;
	bool vpApiEventPending = FALSE;

	//printk("1pDevCtx = %p\n", pLine->pDev->pDevCtx);
	for (i = 0; i<1200; i++)
	{		  
		status = VpApiTick( pLine->pDev->pDevCtx, &vpApiEventPending );
		if (  (status == VP_STATUS_SUCCESS) && 
			(TRUE == vpApiEventPending) )
		{
		 VpEventType pEvent;
		 while(VpGetEvent(pLine->pDev->pDevCtx, &pEvent)) 
		 {
			switch (pEvent.eventCategory) 
			{
			   /* Add more categories as needed */
			   case VP_EVCAT_RESPONSE:
				  switch(pEvent.eventId)
				  {
					 /* Add more events as needed */
					 case VP_DEV_EVID_DEV_INIT_CMP:
						PRINT_MSG("SLIC: Received VP_DEV_EVID_DEV_INIT_CMP event\n");
						//InitQuickStartSettings(&pEvent);
						break;
					 case VP_LINE_EVID_LINE_INIT_CMP:
						PRINT_MSG("SLIC: Received VP_LINE_EVID_LINE_INIT_CMP event\n");
						break;
					 case VP_EVID_CAL_CMP:
						PRINT_MSG("SLIC: Received VP_EVID_CAL_CMP event\n");
						VpGetResults(&pEvent, pLine->calCoeff);
						return TRUE;
						break;
					 case VP_LINE_EVID_RD_OPTION:
						PRINT_MSG("SLIC: Received VP_LINE_EVID_RD_OPTION event\n");
						break;
					 default: 
						/* Do nothing */
						#ifdef DEBUG_API
						PRINT_MSG("SLIC: ERROR Unexpected Event %d came from the SLIC.\n", pEvent.eventId);
						#endif
						break;
				  }
				  break;

			   default:
				  break;
			}
		 }
		}
	}

   /* Could not find any events if we reach here */
   return FALSE;
}

extern int msinit;
VpStatusType zarlinkGetCoeff (
	RTKLineObj *pLine, void *data)
{
	VpStatusType status;
	msinit = 0;

	status = VpCal(pLine->pLineCtx, VP_CAL_GET_SYSTEM_COEFF, NULL);
	if ( status == VP_STATUS_SUCCESS )
   {
#if 1
      /* Check if VP_EVID_CAL_CMP occurred */
      if( TRUE != zarlinkWaitForEvent( pLine, 
			VP_EVCAT_RESPONSE, VP_EVID_CAL_CMP ) ) {
         PRINT_R("SLIC: ERROR: Line calibration was not completed\n");
         return FAILED;
      }else{
		  //printk("[%s][%d] data = %p, &pLine->calCoeff[0] = %p\n", __FUNCTION__, __LINE__, data, &pLine->calCoeff[0]);
		  memcpy(data, &pLine->calCoeff[0], 52);
	  }
#endif	  
   }else
		  printk("VpCal fail : status code 0x%x\n", status);
   msinit = 1;
   return VP_STATUS_SUCCESS;
}

VpStatusType zarlinkSetCoeff (
	RTKLineObj *pLine, void *data)
{
#if 0

#endif	
	memcpy(&pLine->calCoeff[0], data, 52);
	return VP_STATUS_SUCCESS;
}

VpStatusType zarlinkDoCalibration(RTKLineObj *pLine, unsigned char mode)
{
	VpStatusType status;
	VpLineStateType lineState;
	msinit = 0;
	
	// Get the current state and save
	status = VpGetLineState( pLine->pLineCtx, &lineState );
	if ( status != VP_STATUS_SUCCESS ) {
		PRINT_R("VpGetLineState failed (%d) \n", status);
		return status;
	}		

	/* Set the initial line state */
	status = VpSetLineState( pLine->pLineCtx, VP_LINE_STANDBY );
	if ( status != VP_STATUS_SUCCESS ) {
		PRINT_R("VpSetLineState failed (%d) \n", status);
		return status;
	}	

	if ( mode == 0 ){
		status = VpCalLine(pLine->pLineCtx);
		if ( (status != VP_STATUS_SUCCESS) && 
			(status != VP_STATUS_FUNC_NOT_SUPPORTED) ) {
				PRINT_R("SLIC ERROR: %s line%d, status = %d \n", 
				__FUNCTION__, __LINE__, status);
				return status;
		}
		else if ( status == VP_STATUS_SUCCESS )	{
			/* Check if VP_EVID_CAL_CMP occurred */
			if( TRUE != zarlinkWaitForEvent( pLine,
				VP_EVCAT_RESPONSE, VP_EVID_CAL_CMP ) ) {
				PRINT_R("SLIC: ERROR: Line calibration was not completed\n");
				return status;
			}
		}		
	}else if ( mode == 1){
		status = VpCal(pLine->pLineCtx, VP_CAL_APPLY_SYSTEM_COEFF, pLine->calCoeff);
		if ( (status != VP_STATUS_SUCCESS) && 
			(status != VP_STATUS_FUNC_NOT_SUPPORTED) ) {
				PRINT_R("SLIC ERROR: %s line%d, status = %d \n", 
				__FUNCTION__, __LINE__, status);
			return status;
		}else if ( status == VP_STATUS_SUCCESS ) {
			/* Check if VP_EVID_CAL_CMP occurred */
			if( TRUE != zarlinkWaitForEvent( pLine,
				VP_EVCAT_RESPONSE, VP_EVID_CAL_CMP ) ) {
				PRINT_R("SLIC: ERROR: Line calibration was not completed\n");
				return status;
			}
		}		
	}
	
	/* Set to original line state */
	status = VpSetLineState( pLine->pLineCtx, lineState );
	if ( status != VP_STATUS_SUCCESS ) {
		PRINT_R("VpSetLineState failed (%d) \n", status);
		return status;
	}	
	msinit = 1;
	return status;
}

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST)

int zarlinkLineTest(
	RTKLineObj *pLine, int tID, void *data)
{
	int ret = 0;
	//pLine->codec_type = 0;
	//printk("set code 0\n");
	//VpSetOption( pLine->pLineCtx, VP_NULL, VP_OPTION_ID_CODEC, (void*)&pLine->codec_type ); 
	down(&pLine->ltTestSem);
	rtkAddChannelIsInUse(pLine->ch_id);
	//mdelay(100);
	VpSetLineState( pLine->pLineCtx, VP_LINE_OHT );
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	ret = Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	ret = Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
	//mdelay(100);

	//pLine->codec_type = 2;
	//printk("set code 2\n");
	up(&pLine->ltTestSem);
	rtkRemoveChannelIsInUse(pLine->ch_id);

	//VpSetOption( pLine->pLineCtx, VP_NULL, VP_OPTION_ID_CODEC, (void*)&pLine->codec_type );   

	return ret;
}

#if 0
int zarlinkPortDetect (
	RTKLineObj *pLine, int tID, void *data)
{
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	return Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	return Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
}

int zarlinkLineROH (
	RTKLineObj *pLine, int tID, void *data)
{
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	//zarlinkSetLineState(pLine, VP_LINE_TALK);
	return Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	return Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
}

int zarlinkLineVOLTAGE (
	RTKLineObj *pLine, int tID, void *data)
{
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	return Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	return Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
}

int zarlinkLineRESFLT (
	RTKLineObj *pLine, int tID, void *data)
{
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	return Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	return Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
}

int zarlinkLineCap (
	RTKLineObj *pLine, int tID, void *data)
{
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_890_SERIES)
	return Ve890StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#elif defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	return Ve886StartLineTest(pLine->pLineCtx, pLine->pDev->pDevCtx, tID, data);
#endif
}
#endif

#endif

#if 1//defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
VpStatusType zarlinkInitled (RTKLineObj *pLine)
{
	VpStatusType status;
#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)	
	VpOptionLineIoConfigType io_cfg_ctrl;
	VpLineIoAccessType IoCtrl;
#else
	VpOptionDeviceIoType io_cfg_ctrl;
	VpDeviceIoAccessDataType IoCtrl;
#endif	

	#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	io_cfg_ctrl.direction = 1;
	io_cfg_ctrl.outputType = 0;	
	status = VpSetOption(pLine->pLineCtx, VP_NULL, VP_OPTION_ID_LINE_IO_CFG, &io_cfg_ctrl);	
	if ( status != VP_STATUS_SUCCESS ) {
	    	printk("VpSetOption for line IO failed. status = %d\n", status);
		return FAILED;
   	}	
	#else
	io_cfg_ctrl.directionPins_31_0 = 1;
	io_cfg_ctrl.directionPins_63_32 = 0;
	io_cfg_ctrl.outputTypePins_31_0 = 0;
	io_cfg_ctrl.outputTypePins_63_32 = 0;
	status = VpSetOption(VP_NULL, pLine->pDev->pDevCtx, VP_DEVICE_OPTION_ID_DEVICE_IO, &io_cfg_ctrl);	
	if ( status != VP_STATUS_SUCCESS ) {
    	printk("VpSetOption for DeviceIO failed. status = %d\n", status);
		return FAILED;
   	}	
	#endif	

	#if defined(CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES)
	IoCtrl.direction = VP_IO_WRITE;
	IoCtrl.ioBits.mask = 0x1;
	IoCtrl.ioBits.data = 0x1;		
	status = VpLineIoAccess(pLine->pLineCtx, &IoCtrl, 0);	
	if ( status != VP_STATUS_SUCCESS ) {
    	printk("VpSetOption for VpLineIoAccess failed. status = %d\n", status);
		return FAILED;
   	}	
	#else
	IoCtrl.accessType = VP_DEVICE_IO_WRITE;
	IoCtrl.accessMask_31_0 = 0x1;
	IoCtrl.deviceIOData_31_0 = 0x1;
	status = VpDeviceIoAccess(pLine->pDev->pDevCtx, &IoCtrl);
	if ( status != VP_STATUS_SUCCESS ) {
    	printk("VpSetOption for VpDeviceIoAccess failed. status = %d\n", status);
		return FAILED;
   	}	
	#endif
	return status;
}
#if 0
VpStatusType zarlinkled (
	RTKLineObj *pLine, int onoff )
{
	VpLineIoAccessType IoCtrl;
	IoCtrl.direction = VP_IO_WRITE;
	IoCtrl.ioBits.data &= 0xFE;	
	
	if (onoff == 1){
		//IoCtrl->ioBits.mask = 2;
		IoCtrl.ioBits.data = 1;
	printk("pLine->pLineCtx[%p], onoff%d, data = 0x[%x]\n", 
		pLine->pLineCtx, onoff, IoCtrl.ioBits.data);		
		VpLineIoAccess(pLine->pLineCtx, &IoCtrl, 0);
	}else if (onoff == 0){
		//IoCtrl->ioBits.mask = 3;
		IoCtrl.ioBits.data = 0;	
	printk("pLine->pLineCtx[%p], onoff%d, data = 0x[%x]\n", 
		pLine->pLineCtx, onoff, IoCtrl.ioBits.data);		
		VpLineIoAccess(pLine->pLineCtx, &IoCtrl, 0);
	}
#if 0	
	else if (onoff == 2){
		IoCtrl.direction = VP_IO_READ;
		VpLineIoAccess(pLine->pLineCtx, &IoCtrl, 0);
		printk("pLine->pLineCtx[%p], onoff%d, data = 0x[%x], , IoCtrl.ioBits.mask=0x[%x]\n", 
			pLine->pLineCtx, onoff, IoCtrl.ioBits.data, IoCtrl.ioBits.mask);
		VpGetResults(VP_LINE_EVID_LINE_IO_RD_CMP, &IoCtrl);
		printk("pLine->pLineCtx[%p], onoff%d, data = 0x[%x], , IoCtrl.ioBits.mask=0x[%x]\n", 
			pLine->pLineCtx, onoff, IoCtrl.ioBits.data, IoCtrl.ioBits.mask);		
	}
#endif	
}
#endif
#endif
/* 
** API	  : zarlinkReadRegister()
** Desp	  :	Read Zarlink register
** input  : pDev, Reg# 
** return : print Register value
*/
VpStatusType zarlinkRWDevReg (
	RTKLineObj *pLine,
	unsigned int reg, 
	unsigned char *len, 
	char * regdata) 
{
	VpStatusType status = VP_STATUS_SUCCESS;
	VpRegisterReadWrite(pLine->pLineCtx, reg, len, regdata);
	return status;
}

/* 
** API	  : rtkGetNewChID()
** Desp	  :	Increase global ch id
** input  : Realtek NA
** return : Zarlink global ch id
*/
int rtkGetNewChID()
{
	return gCHId++;
}


void rtkAddChannelIsInUse(int ch_id)
{
	totalchannelID[ch_id] = 1;
}

void rtkRemoveChannelIsInUse(int ch_id)
{
	totalchannelID[ch_id] = 0;
}


int rtkCheckChannelIsInUse(int ch_id)
{
	if (totalchannelID[ch_id]){
		return 1;
	}
	return 0;
}

