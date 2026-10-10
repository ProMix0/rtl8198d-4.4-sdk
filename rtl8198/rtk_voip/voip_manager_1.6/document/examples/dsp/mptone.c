#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "voip_manager.h"

//the ToneTable vlaue can be filled in Web page 
TstVoipToneCfg ToneTable[] =
// China tone example
{
	{0, 0, 1, 5000,	   0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},
	{0, 0, 1, 5000,    0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},
	{0, 0, 1, 5000,    0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},		 
	{0, 0, 1, 5000,    0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},			 
	{0, 0, 1, 5000,    0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},			 
	{0, 0, 1, 5000,	   0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},			
	{0, 0, 1, 5000,    0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0},			
	{0, 0, 1, 5000,	   0, 0, 0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0}		 
};

void SetCustomTone(void);

int main(int argc, char *argv[])
{
	int chid = 0, enable = 0;
	//st_ToneCfgParam MP_tone             = {0, 0, 1, 2000,    0,   0,     0, 0, 0, 0, 0, 0, 1, 1000,   0, 0, 0,  7,  0, 0, 0};	
	
	if ( argc != 3 ){
		printf("Please input correct parameter : ./mptone chid enable\n");
		printf("Ch0 enable : ./mptone 0 1\n");
		printf("Ch0 disable: ./mptone 0 0\n");
	}	

	chid = atoi(argv[1]);
	enable = atoi(argv[2]);
	printf("chid %d enable %d\n", chid, enable);	
	
	rtk_GetVoIPFeature();
	
	SetCustomTone();
	//rtk_SetUpdateTone(TONE_CHINA, DSPCODEC_TONE_EXTEND_1,	&MP_tone);
	
	if( !RTK_VOIP_IS_SLIC_CH( chid, g_VoIP_Feature ) )
		return 0;
	
	rtk_SetVoiceGain(chid, 0, 0);
	rtk_SetFlashHookTime(chid, 0, 300,800);
	rtk_SetFlushFifo(chid);					// flush kernel fifo before app run

	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);	
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_STUTTERDIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);	
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_MESSAGE_WAITING, 0, DSPCODEC_TONEDIRECTION_LOCAL);	
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CONFIRMATION, 0, DSPCODEC_TONEDIRECTION_LOCAL);	
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_CONGESTION, 0, DSPCODEC_TONEDIRECTION_LOCAL);	
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_BUSY, 0, DSPCODEC_TONEDIRECTION_LOCAL);		
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_OFFHOOKWARNING, 0, DSPCODEC_TONEDIRECTION_LOCAL);
	rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_HOLD, 0, DSPCODEC_TONEDIRECTION_LOCAL);
	if ( enable == 1 )
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 1, DSPCODEC_TONEDIRECTION_LOCAL);
	else
		rtk_SetPlayTone(chid, 0, DSPCODEC_TONE_DIAL, 0, DSPCODEC_TONEDIRECTION_LOCAL);

	return 0;
}
void SetCustomTone()
{
	int cust_idx;
	
	TstVoipToneCfg custom_tone[TONE_CUSTOMER_MAX];
	
//	voipCfgParam_t VoIPCfg;
	
	
	memcpy(custom_tone, ToneTable, TONE_CUSTOMER_MAX*sizeof(TstVoipToneCfg));
	
	for (cust_idx=0; cust_idx < TONE_CUSTOMER_MAX; cust_idx++)
	{
		rtk_SetCustomTone(cust_idx, &custom_tone[cust_idx]);
	}	
	
//	VoIPCfg.tone_of_country = TONE_CUSTOMER; //Custom
//	VoIPCfg.tone_of_custdial = 0;
//	VoIPCfg.tone_of_custring = 1;
//	VoIPCfg.tone_of_custbusy = 2;
//	VoIPCfg.tone_of_custwaiting = 3;
	
	rtk_SetCountry(TONE_CUSTOMER);
	/* set congestion tone */
	rtk_SetUpdateTone(TONE_CUSTOMER, DSPCODEC_TONE_CONGESTION, &custom_tone[4]);
}
