#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "voip_manager.h"
#include <voip_params.h>

int main(int argc, char *argv[]){
	int opt = 0, chid = 0, index = 0, type = 0, gain = 0, dir = 0, gain2 = 0;

	if ( argc < 5 )
	{
		printf("Please input channel id\n");
		printf("\t-c : channel id = 0 or 1\n");
		printf("\t-t : \n");
		printf("\t\t 0:set \n");
		printf("\t-v : \n");
		printf("\t\t Impedance index\n");
		printf("\t-g : \n");
		printf("\t\t Gain\n");		
		printf("\t-d : \n");
		printf("\t\t direction 0:rx, 1:tx\n");				
		return 0;
	}
	
	while (( opt = getopt(argc, argv, "c:t:v:g:d:")) != -1 )
	{
		switch (opt)
		{
			case 'c':
				sscanf(optarg, "%d", &chid);
				break;				
			case 't':
				sscanf(optarg, "%d", &type);
				break;				
			case 'v':
				sscanf(optarg, "%d", &index);
				break;
			case 'g':
				sscanf(optarg, "%d", &gain);
				break;
			case 'd':
				sscanf(optarg, "%d", &dir);
				break;				
			case ':':
				printf("need to more parameters\n");
				break;
		}
	}	
	
	rtk_GetVoIPFeature();
	
	if ( type == 0 ){ // set slic impedance
		rtk_SetImpedanceIndex(index);
	}	
	
	if ( type == 1 ){ // set slic gain
	
		if ( gain == -12 )
			gain2 = SLIC_GAIN_MINUS_12_DB;
		else if ( gain == -11 )
			gain2 = SLIC_GAIN_MINUS_11_DB;
		else if ( gain == -10 )
			gain2 = SLIC_GAIN_MINUS_10_DB;
		else if ( gain == -9 )
			gain2 = SLIC_GAIN_MINUS_9_DB;
		else if ( gain == -8 )
			gain2 = SLIC_GAIN_MINUS_8_DB;
		else if ( gain == -7 )
			gain2 = SLIC_GAIN_MINUS_7_DB;
		else if ( gain == -6 )
			gain2 = SLIC_GAIN_MINUS_6_DB;		
		else if ( gain == -5 )
			gain2 = SLIC_GAIN_MINUS_5_DB;
		else if ( gain == -4 )
			gain2 = SLIC_GAIN_MINUS_4_DB;
		else if ( gain == -3 )
			gain2 = SLIC_GAIN_MINUS_3_DB;
		else if ( gain == -2 )
			gain2 = SLIC_GAIN_MINUS_2_DB;		
		else if ( gain == -1 )
			gain2 = SLIC_GAIN_MINUS_1_DB;
		else if ( gain == 0 )
			gain2 = SLIC_GAIN_0_DB;
		else if ( gain == 1 )
			gain2 = SLIC_GAIN_PLUS_1_DB;
		else if ( gain == 2 )
			gain2 = SLIC_GAIN_PLUS_2_DB;
		else if ( gain == 3 )
			gain2 = SLIC_GAIN_PLUS_3_DB;
		else if ( gain == 4 )
			gain2 = SLIC_GAIN_PLUS_4_DB;
		else if ( gain == 5 )
			gain2 = SLIC_GAIN_PLUS_5_DB;
		else if ( gain == 6 )
			gain2 = SLIC_GAIN_PLUS_6_DB;
		else if ( gain == 7 )
			gain2 = SLIC_GAIN_PLUS_7_DB;
		else if ( gain == 8 )
			gain2 = SLIC_GAIN_PLUS_8_DB;
		else if ( gain == 9 )
			gain2 = SLIC_GAIN_PLUS_9_DB;
		else if ( gain == 10 )
			gain2 = SLIC_GAIN_PLUS_10_DB;
		else if ( gain == 11 )
			gain2 = SLIC_GAIN_PLUS_11_DB;
		else if ( gain == 12 )
			gain2 = SLIC_GAIN_PLUS_12_DB;		
	
		if ( dir == 0 )
			rtk_Set_SLIC_Rx_Gain(chid, gain2);
		else if ( dir == 1 )
			rtk_Set_SLIC_Tx_Gain(chid, gain2);
		
	}
	return 0;
}

