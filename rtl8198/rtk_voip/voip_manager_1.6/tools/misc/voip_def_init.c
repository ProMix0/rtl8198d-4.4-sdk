#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <voip_feature.h>
#include <voip_manager.h>

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
#include <sys/stat.h>
#define FW_PATH "/image/fw_dxs-2.1.3.bin"
#define BBD_PATH "/image/R600.BIN"
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
#define DELAY_TIME_CAL	1500000	
#define DELAY_TIME_INIT	500000
#define COEFFLEN	52
#define COEFFPATH	"/var/config/slicoeff"
typedef struct _Coeff{
	unsigned char coeff1[COEFFLEN];
#if CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_NR == 2
	unsigned char coeff2[COEFFLEN];
#endif	
}Coeff;
#endif

int main(void)
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
	FILE *fp;
	int len = 0;
	size_t result;
	Coeff *pCoeff;
	TstVoipCalibrationCoeff stVoipCalibrationCoeff;
	TstVoipDoCalibration stVoipDoCalibration;
#endif	

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
	TstVoipSLICFWInfo fwInfo;
	struct stat st;

	memset(&fwInfo, 0, sizeof(fwInfo));
	
	if(stat(FW_PATH, &st) < 0){
		printf("stat FW file failed\n");
		return -1;
	}
	
	fwInfo.fwSize = st.st_size;
	
	if(stat(BBD_PATH, &st) < 0){
		printf("stat BBD file failed\n");
		return -1;
	}

	fwInfo.BBDSize = st.st_size;

	strncpy(&fwInfo.fwPath[0], FW_PATH, sizeof(fwInfo.fwPath) - 1);
	strncpy(&fwInfo.BBDPath[0], BBD_PATH, sizeof(fwInfo.BBDPath) - 1);
	
	rtk_CompleteDeferInitialzation(fwInfo);
#else
	rtk_CompleteDeferInitialzation();
#endif
	fprintf(stderr , "All DSP Software Ready. VoIP Defer Init done. \n");
	return 0;

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
	fp = fopen(COEFFPATH, "rb");
	if ( fp == NULL ){
		fp = fopen(COEFFPATH, "wb");
		if ( fp == NULL ){
			printf("open file(w) %s fail\n", COEFFPATH);
			return -1;
		}
	}
	
	pCoeff  = (Coeff *)malloc(sizeof(Coeff));
	if ( pCoeff == NULL ) {
		printf("malloc pCoeff fail\n");
		return -1;		
	}
	
	memset(pCoeff, 0x0, sizeof(Coeff));
	memset(&stVoipCalibrationCoeff, 0x0, sizeof(TstVoipCalibrationCoeff));
	memset(&stVoipDoCalibration, 0x0, sizeof(TstVoipDoCalibration));

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	usleep(DELAY_TIME_INIT);
	
	if ( len == 0 ){
		printf("\nDo VpCaline\n");
		freopen(COEFFPATH, "wb", fp);
		usleep(DELAY_TIME_CAL);
		stVoipDoCalibration.mode = 0;
		stVoipDoCalibration.ch_id = 0;		
		rtk_DoCalibration(stVoipDoCalibration.ch_id, &stVoipDoCalibration);

		usleep(DELAY_TIME_CAL);
		stVoipCalibrationCoeff.ch_id = 0;		
		rtk_GetCalibrationCoeff(stVoipCalibrationCoeff.ch_id, &stVoipCalibrationCoeff);
		memcpy(pCoeff->coeff1, stVoipCalibrationCoeff.coeff, COEFFLEN);

	
#if CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_NR == 2
		usleep(DELAY_TIME_CAL);
		stVoipDoCalibration.mode = 0;
		stVoipDoCalibration.ch_id = 1;
		rtk_DoCalibration(stVoipDoCalibration.ch_id, &stVoipDoCalibration);
		
		usleep(DELAY_TIME_CAL);
		stVoipCalibrationCoeff.ch_id = 1;		
		rtk_GetCalibrationCoeff(stVoipCalibrationCoeff.ch_id, &stVoipCalibrationCoeff);
		memcpy(pCoeff->coeff2, stVoipCalibrationCoeff.coeff, COEFFLEN);

#endif	
		fwrite(pCoeff, sizeof(char), sizeof(Coeff), fp);
#if CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_NR == 2
	}else if ( len == COEFFLEN*2 ) {
#else
	}else if ( len == COEFFLEN ) {
#endif
		printf("\nDo VpCal\n");
		result = fread(pCoeff, 1, sizeof(Coeff), fp);
		if ( result != sizeof(Coeff)){
			printf("read error\n");
			return -1;
		}
		usleep(DELAY_TIME_CAL);
		memcpy(stVoipCalibrationCoeff.coeff, pCoeff->coeff1, COEFFLEN);

		stVoipCalibrationCoeff.ch_id = 0;
		rtk_SetCalibrationCoeff(stVoipCalibrationCoeff.ch_id, &stVoipCalibrationCoeff);
		
		usleep(DELAY_TIME_CAL);
		stVoipDoCalibration.ch_id = 0;
		stVoipDoCalibration.mode = 1;
		rtk_DoCalibration(stVoipDoCalibration.ch_id, &stVoipDoCalibration);		
		
#if CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_NR == 2
		usleep(DELAY_TIME_CAL);
		memcpy(stVoipCalibrationCoeff.coeff, pCoeff->coeff2, COEFFLEN);

		stVoipCalibrationCoeff.ch_id = 1;
		rtk_SetCalibrationCoeff(stVoipCalibrationCoeff.ch_id, &stVoipCalibrationCoeff);

		usleep(DELAY_TIME_CAL);
		stVoipDoCalibration.ch_id = 1;
		stVoipDoCalibration.mode = 1;
		rtk_DoCalibration(stVoipDoCalibration.ch_id, &stVoipDoCalibration);
#endif		
	}	
	
	fclose(fp);
	free(pCoeff);	
#ifndef CONFIG_RTL8686
	if ( len == 0 )
		system("/bin/flatfsd -s");
#endif		
#endif

	return 0;
}

