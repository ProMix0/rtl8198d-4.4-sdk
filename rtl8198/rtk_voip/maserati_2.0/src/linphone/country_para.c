#include "uglib.h"
#include "voip_manager.h"
#include "rcm_customize.h"
#include "linphonecore.h"


#if 0
void customer_country_guatemala()
{

// Guatemala tone
/*
				         Freq    on    off    on    off
dial tone                425   continue
ring tone                425      1      4 
busy tone                425    0.5    0.5
congestion tone          425    0.2    0.2
waiting call tone        425    0.2    0.2    0.2   10
call forwarding tone     425      1      4
tone message waiting     425      1      1
*/

static TstVoipToneCfg dial_tone             = {0, 0, 1, 2000,    0,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
static TstVoipToneCfg ring_tone             = {0, 0, 1, 1000, 4000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
static TstVoipToneCfg busy_tone             = {0, 0, 1,  500,  500,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
static TstVoipToneCfg congestion_tone       = {0, 0, 1,  200,  200,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
static TstVoipToneCfg call_waiting_tone     = {0, 0, 2,  200,  200, 200, 10000, 0, 0, 0, 0, 0, 1, 425, 425, 0, 0,  7,  7, 0, 0};  //2 cadances
static TstVoipToneCfg call_forwarding_tone  = {0, 0, 1, 1000, 4000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
static TstVoipToneCfg messagge_wating_tone  = {0, 0, 1, 1000, 1000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static int country_slic_impedance=-1; //use default 600 ohms


	if(country_slic_impedance >0)
		rtk_SetCountryImpedance(country_slic_impedance);

	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,			&dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING, 		&ring_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,			&busy_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,		&congestion_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,	&call_waiting_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,	&call_forwarding_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone); 


}

void customer_country_brazil(void)
{
	//BRAZIL tone
	/*
							 Freq	 on    off	  on	off
	dial tone				 425   continue
	ring tone				 425	  1 	 4 
	busy tone				 425	0.25	0.25
	congestion tone 		 425	0.75	0.25  0.25	0.25
	waiting call tone		 425	0.06s	0.25s 0.06s 9s	
	call forwarding tone	 425	  1 	 4
	tone message waiting	 425	  0.1 	 0.1
	*/
	int i;
	
	static TstVoipToneCfg dial_tone			 = {0, 0, 1, 2000,	  0,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg ring_tone			 = {0, 0, 1, 1000, 4000,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg busy_tone			 = {0, 0, 1,  250,	250,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg congestion_tone		 = {0, 0, 2,  750,	250,  250,	250, 0, 0, 0, 0, 0, 1, 425, 425, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg call_waiting_tone	 = {0, 0, 2,  60,  250,	60,	 9000, 0, 0, 0, 0, 0, 1, 425, 0, 0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg call_forwarding_tone  = {0, 0, 1, 1000, 4000,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
//	static TstVoipToneCfg messagge_wating_tone  = {0, 0, 1, 100, 10,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};


	static st_ToneCfgParam messagge_wating_tone  = {4, 1, 10, 100,100,100,100,100,100,100,100, 0, 1, 425,   425, 425, 425,	7,	0, 0, 0,425,425,425,425,0,0,0,0 , 425,425,425,425,0,0,0,0, 425,425,425,425, 0,0,0,0,
													100,100,100,100, 100,100,100,100, 425,425,425,425, 0,0,0,0,	425,425,425,425, 0,0,0,0 , 425,425,425,425, 0,0,0,0, 425,425,425,425,0,0,0,0,
													20000,0,0,0, 0,0,0,0, 425,425,425,425, 0,0,0,0,	425,425,425,425, 0,0,0,0 , 425,425,425,425, 0,0,0,0, 425,425,425,425, 0,0,0,0};

	static int country_slic_impedance=-1; //use default 600 ohms
	
	if(country_slic_impedance >0)
			rtk_SetCountryImpedance(country_slic_impedance);
		
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,			&dial_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING, 		&ring_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,			&busy_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,		&congestion_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,	&call_waiting_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,	&call_forwarding_tone);
		rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone); 

	
	for (i=0; i<g_MaxVoIPPorts; i++)
			rcm_tapi_Set_SLIC_Ring_Cadence(i, 1000, 4000);	
}
#endif  //#if 0

void customer_country_russian(void)
{
	/*
	russian
	dial tone                425   continue
	ring tone                425      1      4
	busy tone 		 415     0.35   0.35 

	other need modify

	*/
	int i;
	static TstVoipToneCfg dial_tone             = {0, 0, 1, 2000,    0,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg ring_tone             = {0, 0, 1, 1000, 4000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg busy_tone             = {0, 0, 1,  350,  350,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg congestion_tone       = {0, 0, 1,  200,  200,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg call_waiting_tone     = {0, 0, 2,  200,  200, 200, 10000, 0, 0, 0, 0, 0, 1, 425, 425, 0, 0,  7,  7, 0, 0};  //2 cadances
	static TstVoipToneCfg call_forwarding_tone  = {0, 0, 1, 1000, 4000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg messagge_wating_tone  = {0, 0, 1, 1000, 1000,   0,     0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,  7,  0, 0, 0};
	static int country_slic_impedance=-1;


	if(country_slic_impedance >0)
			rtk_SetCountryImpedance(country_slic_impedance);
		
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,			&dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING, 		&ring_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,			&busy_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,		&congestion_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,	&call_waiting_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,	&call_forwarding_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone); 

	if (g_pVoIPCfg->ring_cad==8) // default case
	{
		for (i=0; i<g_MaxVoIPPorts; i++)
			rcm_tapi_Set_SLIC_Ring_Cadence(i, 1000, 4000);
	}
}

void customer_country_india(void)
{
	/*
	customer_country_india
	Acceptance tone   -   400   1.0 on 4.0 off
	Busy tone         -   400   0.75 on 0.75 off
	Congestion tone   -   400   0.25 on 0.25 off
	Dial tone         -   400x25   continuous
	Special dial tone -   400   2.8 on 0.2 off
	Holding tone      -   400   0.25 on 0.25 off 0.25 on 3.25 off
	Intrusion tone    -   400   0.15 on 4.85 off
	Refusal tone      -   400   0.25 on 0.25 off
	Ringing tone I  (local calls)   -    400x25   0.4 on 0.2 off 0.4 on 2.0 off
	Ringing tone II (NSD/ISD calls) -    400x25   1.0 on 2.0 off
	Route tone        -   400   0.1 on 0.9 off
	Call waiting tone -   400   0.2 on 0.1 off 0.2 on 7.5 off
	*/

	/* Changed dial tone and ring tone to Modulated to fix issue of ring back tone */
	/* static TstVoipToneCfg dial_tone = {MODULATED, 0, 1, 2000, 0,   0,   0,    0, 0, 0, 0, 0, 2, 400, 25,  0, 0,  7,  0, 0, 0};*/

	static TstVoipToneCfg dial_tone            = {1, 0, 1, 500,  0,   0,   0,    0, 0, 0, 0, 0, 2, 400, 25,  0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg ring_tone            = {1, 0, 2, 400,  200, 400, 2000, 0, 0, 0, 0, 0, 2, 400, 25,  0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg busy_tone            = {0, 0, 1, 750,  750, 0,   0,    0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg congestion_tone      = {0, 0, 1, 250,  250, 0,   0,    0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg call_hold_tone       = {0, 0, 2, 250,  250, 250, 3250, 0, 0, 0, 0, 0, 1, 400, 400, 0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg call_waiting_tone    = {0, 0, 2, 200,  100, 200, 7500, 0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  7, 0, 0};
	static TstVoipToneCfg special_dial_tone    = {0, 0, 1, 2800, 200, 0,   0,    0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg call_forwarding_tone = {0, 0, 1, 400,  40,  0,   0,    0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg messagge_wating_tone = {0, 0, 1, 2800, 200, 0,   0,    0, 0, 0, 0, 0, 1, 400, 0,   0, 0,  7,  0, 0, 0};
	static int country_slic_impedance = -1;  //600om

	if(country_slic_impedance >0)
		rtk_SetCountryImpedance(country_slic_impedance);

	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,            &dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING,         &ring_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,            &busy_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,      &congestion_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,    &call_waiting_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,    &special_dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_HOLD,            &call_hold_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone);
}

void customer_country_spain(void)
{
	/*
	SPAIN
	dial tone  , 425 countinue
	ringback tone , 425 , 1.5s on , 3s 0ff
	busy tone , 425 , 0.2s on , 0.2s off
	congestion tone , 425,	0.2s on , 0.2s off, 0.2s on , 0.2s off , 0.2s on ,0.6s off
	call waiting tone , 425, 0.175s on , 0.175s off, 0.175s on ,3.5s off.
	call forward tone , 425, 1s on ,0.1 off.
	
	*/
	
	static TstVoipToneCfg dial_tone			 = {0, 0, 1, 2000,	  0,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg ring_tone			 = {0, 0, 1, 1500, 3000,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg busy_tone			 = {0, 0, 1,  200,	200,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg congestion_tone		 = {0, 0, 3,  200,	200,   200,    200, 200, 600, 0, 0, 0, 1, 425,	 0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg call_waiting_tone	 = {0, 0, 2,  175,	175, 175, 3500, 0, 0, 0, 0, 0, 1, 425, 425, 0, 0,  7,  7, 0, 0};  //2 cadances
	static TstVoipToneCfg call_forwarding_tone  = {0, 0, 1, 1000, 100,   0,	 0, 0, 0, 0, 0, 0, 1, 425,	 0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg messagge_wating_tone  = {0, 0, 1, 1000, 1000,   0,	  0, 0, 0, 0, 0, 0, 1, 425,   0, 0, 0,	7,	0, 0, 0};
	static int country_slic_impedance=9; // 270 + (750 || 150nF) 
	

	if(country_slic_impedance >0)
			rtk_SetCountryImpedance(country_slic_impedance);

	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,            &dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING,         &ring_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,            &busy_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,      &congestion_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,    &call_waiting_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,    &call_forwarding_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone);	


}

#if 0
void customer_country_NEW_Zealand(void)
{
	/*
	New Zealand
	dial tone  , 400 countinue
	ringback tone , 400+450 ,  0.4on 0.4 off,  0.4on ,2.0 off
	busy tone , 400 , 0.5s on , 0.5s off
	congestion tone , 400,	0.25s on , 0.25s off
	call waiting tone , 400, 0.2s on , 3s off, 0.2 on ,3s off. 0.2s on , 3s off

	*/
	
	static TstVoipToneCfg dial_tone			 = {0, 0, 1, 2000,	  0,   0,	  0, 0, 0, 0, 0, 0, 1, 400,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg ring_tone			 = {0, 0, 2, 400, 200,   400,	  2000, 0, 0, 0, 0, 0, 2, 400,   450, 0, 0,	7,	7, 0, 0};
	static TstVoipToneCfg busy_tone			 = {0, 0, 1,  500,	500,   0,	  0, 0, 0, 0, 0, 0, 1, 400,   0, 0, 0,	7,	0, 0, 0};
	static TstVoipToneCfg congestion_tone		 = {0, 0, 1,  250,	250,   0,    0, 0,0, 0, 0, 0, 1, 400,	 0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg call_waiting_tone	 = {0, 0, 3,  200,	3000, 200, 3000, 200, 3000, 0, 0, 0, 1, 400, 0, 0, 0,  7,  0, 0, 0}; 
	static TstVoipToneCfg call_forwarding_tone  = {0, 0, 1, 1000, 100,   0,	 0, 0, 0, 0, 0, 0, 1, 400,	 0, 0, 0,  7,  0, 0, 0};
	static TstVoipToneCfg messagge_wating_tone  = {0, 0, 1, 1000, 1000,   0,	  0, 0, 0, 0, 0, 0, 1, 400,   0, 0, 0,	7,	0, 0, 0};
	static int country_slic_impedance=20;//  370R+620R//310nF 
	
	if(country_slic_impedance >0)
			rtk_SetCountryImpedance(country_slic_impedance);

	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_DIAL,            &dial_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_RINGING,         &ring_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_BUSY,            &busy_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CONGESTION,      &congestion_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CALL_WAITING,    &call_waiting_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_CUSTOM_TONE1,    &call_forwarding_tone);
	rtk_SetUpdateTone(TONE_EXT1, DSPCODEC_TONE_MESSAGE_WAITING, &messagge_wating_tone);	


}
#endif  //#if 0


int rcm_country_para_update(voipCfgParam_t * voip_ptr)
{
	int ret =0;
	
	//always set use EXT1 country
	rtk_SetCountryTone(TONE_EXT1);
	// update tone
	g_message("rcm_country_para_update ,index is %d\n",voip_ptr->tone_of_country);
	//need also check web_voip_tone.c
	switch(voip_ptr->tone_of_country){
//		case 13: //guatemala
//			customer_country_guatemala();
//			break;
//		case 14: //brazil
//			customer_country_brazil();
//			break;
		case 13://russian
			customer_country_russian();
			break;
		case 14://spain
			customer_country_spain();
			break;
		case 15: //india
			customer_country_india();
			break;
		default:
	//		rtk_SetCountryTone(voip_ptr->tone_of_country);
			rtk_SetCountry(voip_ptr->tone_of_country);			
			g_message("rcm_country_para_update use other country ,index is %d\n",voip_ptr->tone_of_country);	
			break;
	}
		
	return ret;
}

