#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "voip_manager.h"

int main(int argc, char *argv[])
{
	int chid = 0, enable = 0;
	
	if ( argc != 3 ){
		printf("Please input correct parameter : ./mpring chid enable \n");
		printf("Ch0 enable : ./mpring 0 1\n");
		printf("Ch0 disable: ./mpring 0 0\n");
		return 1;
	}

	enable = atoi(argv[1]);
	enable = atoi(argv[2]);
	printf("chid %d\n", chid);
	printf("enable %d\n", enable);

	rtk_GetVoIPFeature();
	
	if( !RTK_VOIP_IS_SLIC_CH( 0, g_VoIP_Feature ) )
		return 0;

	rtk_SetSlicRingCadence(chid, 10000, 100); // default value

	if ( enable )
		rtk_SetRingFxs(chid, 1);
	else
		rtk_SetRingFxs(chid, 0);

	return 0;
}
