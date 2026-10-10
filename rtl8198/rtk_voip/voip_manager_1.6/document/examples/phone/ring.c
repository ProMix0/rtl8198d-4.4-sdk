#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <voip_manager.h>
#include <voip_params.h>

int main(int argc, char* argv[])
{
	int opt = 0, chid = 0, index = 0, type = 0;

	if ( argc < 5 )
	{
		printf("Please input channel id\n");
		printf("\t-c : channel id = 0 or 1\n");
		printf("\t-t : \n");
		printf("\t\t 0:make the phone ring\n");
		printf("\t\t 1:set ring voltage\n");
		printf("\t-v : \n");
		printf("\t\t Ring Voltage and Frequency index\n");
		return 0;
	}
	
	while (( opt = getopt(argc, argv, "c:t:v:")) != -1 )
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
			case ':':
				printf("need to more parameters\n");
				break;
		}
	}	
	
	rtk_GetVoIPFeature();
	
	if ( type == 0 ){ // make the slic ring
		rtk_SetRingFxs(chid, 1);
		sleep(3);
		rtk_SetRingFxs(chid, 0);
	}else if ( type == 1 ){
		rtk_SetSlicRingVoltageFreq(chid, index);
	}
	
	return 0;
}

