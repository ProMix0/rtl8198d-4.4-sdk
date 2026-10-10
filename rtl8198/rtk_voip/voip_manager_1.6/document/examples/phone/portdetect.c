#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <voip_manager.h>
#include <voip_control.h>

int main(int argc, char* argv[]){
	int opt = 0;
	int chid = 0, isPlugged = 0, type = 0;

	TstVoipSLICLineRoh roh;
	TstVoipSLICLineVoltage voltage;
	TstVoipSLICLineResFlt flt;
	TstVoipSLICLineCap cap;
	TstVoipSLICLineLoopCurrent loopcurrent;
	TstVoipSLICLineTest_ren ren;
	TstVoipSLICLineTest_rdloop rdloop;
	TstVoipSLICLineTest_dcvoltage dcvoltage;
	TstVoipSLICLineTest_ringerst rgst;
	TstVoipSLICLineTest_rdbattery rdbattery;
	
	memset(&roh, 0, sizeof(TstVoipSLICLineRoh));
	memset(&voltage, 0, sizeof(TstVoipSLICLineVoltage));
	memset(&flt, 0, sizeof(TstVoipSLICLineResFlt));
	memset(&cap, 0, sizeof(TstVoipSLICLineCap));
	memset(&loopcurrent, 0, sizeof(TstVoipSLICLineLoopCurrent));
	memset(&ren, 0, sizeof(TstVoipSLICLineTest_ren));
	memset(&rdloop, 0, sizeof(TstVoipSLICLineTest_rdloop));	
	memset(&dcvoltage, 0, sizeof(TstVoipSLICLineTest_dcvoltage));	
	memset(&rgst, 0, sizeof(TstVoipSLICLineTest_ringerst));	
	memset(&rdbattery, 0, sizeof(TstVoipSLICLineTest_rdbattery));		
	
	if ( argc != 5 )
	{
		printf("Please input channel id\n");
		printf("\t-c : channel id = 0 or 1\n");
		printf("\t-t : \n");
		printf("\t\t 0:port detect\n");
		printf("\t\t 1:roh\n");
		printf("\t\t 2:line voltage\n");
		printf("\t\t 3:RES FLT\n");
		printf("\t\t 4:Capacity\n");
		printf("\t\t 5:Loop Current\n");
		printf("\t\t 6:rdLoop condition\n");
		printf("\t\t 7:rd battery\n");
		printf("\t\t 8:dc voltage test\n");
		printf("\t\t 9:ringer self test\n");		
		printf("\t\t 10:ren test\n");
		return 0;
	}
	
	while (( opt = getopt(argc, argv, "c:t:")) != -1 )
	{
		switch (opt)
		{
			case 'c':
				sscanf(optarg, "%d", &chid);
				break;				
			case 't':
				sscanf(optarg, "%d", &type);
				break;								
			case ':':
				printf("need to more parameters\n");
				break;
		}
	}

//	printf("chid = [%d], type = [%d]\n", chid, type);
	rtk_GetVoIPFeature();

	if ( type == 0 ){
		isPlugged = rtk_GetPortStatus(chid);
	
		if ( isPlugged )
			printf("phone is plugged\n");
		else
			printf("phone isn't been plugged\n");
	}else if ( type == 1 ){
		rtk_GetLineRoh(chid, &roh);
		printf("fltMask = 0x[%x]\n", roh.fltMask);
		printf("measStatus = 0x[%x]\n", roh.measStatus);
		printf("rLoop1 = 0x[%x]\n", roh.rLoop1);
		printf("rLoop2 = 0x[%x]\n", roh.rLoop2);
	}else if ( type == 2 ){
		//memset(voltage, 0, sizeof(int)*6);
		rtk_GetLineVoltage(chid, &voltage);
		printf("vAcDiff = %d\n", voltage.vAcDiff);
		printf("vAcRing = %d\n", voltage.vAcRing);
		printf("vAcTip = %d\n", voltage.vAcTip);
		printf("vDcDiff = %d\n", voltage.vDcDiff);
		printf("vDcRing = %d\n", voltage.vDcRing);
		printf("vDcTip = %d\n", voltage.vDcTip);		
		printf("fltMask = 0x[%x]\n", voltage.fltMask);
		printf("measStatus = 0x[%x]\n", voltage.measStatus);
	}else if ( type == 3 ){	
		rtk_GetLineResflt(chid, &flt);
		printf("rGnd = %d\n", flt.rGnd);
		printf("rrg = %d\n", flt.rrg);
		printf("rtg = %d\n", flt.rtg);
		printf("rtr = %d\n", flt.rtr);
		printf("fltMask = 0x[%x]\n", flt.fltMask);
		printf("measStatus = 0x[%x]\n", flt.measStatus);
	}else if ( type == 4 ){	 // capacitance
		rtk_GetLineCap(chid, &cap);
		printf("crg = %d\n", cap.crg);
		printf("ctg = %d\n", cap.ctg);
		printf("ctr = %d\n", cap.ctr);
		printf("fltMask = 0x[%x]\n", cap.fltMask);
		printf("measStatus = 0x[%x]\n", cap.measStatus);	
	}else if ( type == 5 ){	 // Loop Current
		rtk_GetLineLoopCurrent(chid, &loopcurrent);
		printf("iTestLoad = %d\n", loopcurrent.iTestLoad);
		printf("rTestLoad = %d\n", loopcurrent.rTestLoad);
		printf("vTestLoad = %d\n", loopcurrent.vTestLoad);
		printf("fltMask = 0x[%x]\n", loopcurrent.fltMask);
		printf("measStatus = 0x[%x]\n", loopcurrent.measStatus);	
	}else if ( type == 6 ){	 // rdLoop condition
		rtk_DoLineTest_rdloopcond(chid, &rdloop);
		printf("fltMask = 0x[%x]\n", rdloop.fltMask);
		printf("measStatus = 0x[%x]\n", rdloop.measStatus);
		printf("rloop = %d\n", rdloop.rloop);
		printf("imt = %d\n", rdloop.imt);		
		printf("ilg = %d\n", rdloop.ilg);
		printf("vab = %d\n", rdloop.vab);
		printf("vag = %d\n", rdloop.vag);
		printf("vbg = %d\n", rdloop.vbg);
		printf("vbat1 = %d\n", rdloop.vbat1);
		printf("vbat2 = %d\n", rdloop.vbat2);
		printf("vbat3 = %d\n", rdloop.vbat3);				
	}else if ( type == 7 ){	 // rd battery
		rtk_DoLineTest_rdBattery(chid, &rdbattery);
		printf("fltMask = 0x[%x]\n", rdbattery.fltMask);
		printf("measStatus = 0x[%x]\n", rdbattery.measStatus);
		printf("bat1 = %d\n", rdbattery.bat1);
		printf("bat2 = %d\n", rdbattery.bat2);
		printf("bat3 = %d\n", rdbattery.bat3);		
	}else if ( type == 8 ){	 //dc voltage test
		rtk_DoLineTest_dcvoltage(chid, &dcvoltage);
		printf("fltMask = 0x[%x]\n", dcvoltage.fltMask);
		printf("measStatus = 0x[%x]\n", dcvoltage.measStatus);
		printf("measuredVoltage1 = %d\n", dcvoltage.measuredVoltage1);		
		printf("measuredVoltage2 = %d\n", dcvoltage.measuredVoltage2);
	}else if ( type == 9 ){	 // ringer self test
		rtk_DoLineTest_ringerst(chid, &rgst);
		printf("fltMask = 0x[%x]\n", rgst.fltMask);
		printf("measStatus = 0x[%x]\n", rgst.measStatus);	
		printf("freq = 0x[%x]\n", rgst.freq);			
		printf("iRload = 0x[%x]\n", rgst.iRload);			
		printf("acRload = 0x[%x]\n", rgst.acRload);			
		printf("openCktRingVol = 0x[%x]\n", rgst.openCktRingVol);					
	}else if ( type == 10 ){	 // ren test
		rtk_DoLineTest_REN(chid, &ren);
		printf("fltMask = 0x[%x]\n", ren.fltMask);
		printf("measStatus = 0x[%x]\n", ren.measStatus);	
		printf("result = %d\n", ren.result);
		printf("ren = %d\n", ren.ren);
		printf("renrg = %d\n", ren.renrg);
		printf("rentg = %d\n", ren.rentg);		
	}
	return 0;
}

