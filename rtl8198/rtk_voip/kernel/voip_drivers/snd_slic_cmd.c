#include "snd_slic_cmd.h"
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
#include "zarlink_int.h"
#include "zarlink_api.h"
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
#include "snd_proslic_type.h"

#define SLIC_NR RTK_VOIP_SLIC_NUM( gVoipFeature )

extern void SLIC_read_proslic_reg(voip_snd_t *this, unsigned int num, unsigned char *len, unsigned char *val);
extern void SLIC_read_proslic_ram(voip_snd_t *this, unsigned int num, unsigned int *val);
extern void SLIC_write_proslic_reg(voip_snd_t *this, unsigned int num, unsigned char *len, unsigned char *val);
extern void SLIC_write_proslic_ram(voip_snd_t *this, unsigned int num, unsigned int val);
extern void SLIC_dump_proslic_reg(voip_snd_t *this);
extern void SLIC_dump_proslic_ram(voip_snd_t *this);
#endif

extern VoipFeature_t gVoipFeature;
slic_cmd_t SLIC_CMDS[MAX_SLIC_CMD];

void voip_slic_cmd_help(){
	int cnt = 0;

	for(; cnt < MAX_SLIC_CMD; cnt ++){
		if(SLIC_CMDS[cnt].enable == true)
			printk("%s", SLIC_CMDS[cnt].illustration);
	}
	
	return;
}

void voip_dumpreg_val(char *userData, void *data, void *slic_nr){
	int cnt = 0, num = 0;
	
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
	RTKDevObj *pt = (RTKDevObj *)data;
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
	voip_snd_t *pt = (voip_snd_t *)data;
#endif
	if(slic_nr != NULL)
		num = *((int *)slic_nr);

	if(num <= 1){
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK	
		rtkDumpReg(pt);
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
		SLIC_dump_proslic_reg(pt);
#endif
	}
	else if(num > 1){
		for(; cnt < num; cnt++){
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK	
			rtkDumpReg(pt+cnt);
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
			SLIC_dump_proslic_reg(pt+cnt);
#endif
		}
	}
}

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
void zsi_get_coefficient(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	unsigned char coffdata[52] = {0x0};
	int cnt = 0, p0 = 0, p1 = 0;

	sscanf(userData, "%d %d", &p0, &p1);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support CH%d\n", __FUNCTION__, p1);
		return;
	}
	
	memset(coffdata, 0x0, sizeof(coffdata));
	printk("ch%d\n", p1);
	
	rtkGetCoeff((pt+p1), &coffdata);
	
	for ( cnt = 0 ; cnt < 52 ; cnt++ ){
		if ( cnt % 8  == 0 )
			printk("\n");

		if ( cnt == 51 )
			printk("0x%02x \n", coffdata[cnt]);
		else
			printk("0x%02x, ", coffdata[cnt]);		 
	}
}

void zsi_set_coefficient(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	unsigned char coffdata[52] = {0x0};
	int p0 = 0, p1 = 0;

	sscanf(userData, "%d %d", &p0, &p1);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support CH%d\n", __FUNCTION__, p1);
		return;
	}
	
	printk("ch%d\n", p1);
	
	rtkSetCoeff((pt+p1), &coffdata);
}

void zsi_calibration(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	int p0 = 0, p1 = 0, p2 = 0;

	sscanf(userData, "%d %d %d", &p0, &p1, &p2);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support CH%d\n", __FUNCTION__, p1);
		return;
	}

	if((p2 < 0) || (p2 > 1)){
		printk("[%s]not support mode-%d", __FUNCTION__, p2);
		return;
	}
	
	printk("ch%d\n", p1);
	rtkDoCalibration((pt+p1), (void *)p2);
}
#endif

void zsi_slic_id(char *userData, void *data, void *slic_nr){
	RTKDevObj *pt = (RTKDevObj *)data;
	int p0 = 0, p1 = 0, cnt = 0, num = *((int *)slic_nr);;

	sscanf(userData, "%d %d", &p0, &p1);

	if((p1 < 0) || (p1 >= num)){
		printk("[%s]not support CS%d\n", __FUNCTION__, p1);
		return;
	}
	
	printk("read cs%d slic id\n", p1);

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661) || defined (CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || defined (CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643) || defined (CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601)
	mdelay(50);		
	for (; cnt < 20 ; cnt++ )
		Ve886GetRev((pt+p1));
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601)
	Ve886GetRev((pt+p1));
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116)
	for (; cnt < 10 ; cnt ++ ){
		Ve890GetRev((pt+p1));
	}
#endif
}

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST
void zsi_line_test(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	int p0 = 0, p1 = 0, p2 = 0;
	int result[8] = {0};

	sscanf(userData, "%d %d %d", &p0, &p1, &p2);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support FXS%d\n", __FUNCTION__, p1);
		return;
	}
	
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661)	
	if(p2 != LT_PORT_DET){ // only support fxs detected in 9641/9661 
		printk("[%s]not support this LT function\n", __FUNCTION__);
		return;
	}
#endif
	
	if((pt+p1) == NULL){
		printk("[%s]LineObj is empty\n", __FUNCTION__);
		return;
	}

	if(p2 == LT_VOLTAGE){//voltage		
		printk("test LINE V port %d\n", p1);
		rtkPortDetect((pt+p1), p2, &result);
		printk("vAcDiff = [%d]\n", result[0]);
		printk("vAcRing = [%d]\n", result[1]);
		printk("vAcTip = [%d]\n", result[2]);
		printk("vDcDiff = [%d]\n", result[3]);
		printk("vDcRing = [%d]\n", result[4]);
		printk("vDcTip = [%d]\n", result[5]);
	} 
	else if(p2 == LT_ROH){ //ROH
		printk("test ROH port %d\n", p1);
		rtkPortDetect((pt+p1), p2, &result);
		printk("hook = %d\n", result[0]);
	}
	else if(p2 == LT_PORT_DET){ //fxs plugged
		rtkPortDetect((pt+p1), p2, &result);
		printk("ret = [%d]\n", result[0]);
		if ( result[0] )
			printk("FXS%d is plugged.\n", p1);
		else
			printk("FXS%d is not plugged.\n", p1);
	}
	else if(p2 == LT_RES){ //res flt
		printk("test LT_TID_RES_FLT port %d\n", p1);
		rtkPortDetect((pt+p1), p2, &result);
		printk("rGnd = [%d]\n", result[0]);
		printk("rrg = [%d]\n", result[1]);
		printk("rtg = [%d]\n", result[2]);
		printk("rtr = [%d]\n", result[3]);
	}
	else{
		printk("[%s]not support linetest-%d\n", __FUNCTION__, p2);
		return;
	}
}
#endif

#ifdef ZSI_MODIFY_GAIN
void zsi_modify_gain(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	int p0 = 0, p1 = 0, p2 = 0, p3 = 0;

	sscanf(userData, "%d %d %d %d", &p0, &p1, &p2, &p3);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support CH%d\n", __FUNCTION__, p1);
		return;
	}

	if((p2 < 0) || (p2 > 1)){
		printk("[%s]not support option-%d, 0 = rx, 1 = tx\n", __FUNCTION__, p2);
		return;
	}
	
	printk("input = [%d][%d], %d\n", p1, p2, p3);
	if ( p2 == 0 ){ //rx
		ZarlinkAdjustSlicRxGain((pt+p1), p3);
	}else if ( p2 == 1 ){ //tx
		ZarlinkAdjustSlicTxGain((pt+p1), p3);
	}
}

void zsi_shutdown_dev(char *userData, void *data, void *reserve){
	VpDevCtxType *pt = (VpDevCtxType *)data;
	
	VpShutdownDevice(pt);
}
#endif

#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
void isi_dumpram_val(char *userData, void *data, void *reserve){
	voip_snd_t *pt = NULL;

	pt = (voip_snd_t *)data;
	SLIC_dump_proslic_ram(pt);
}

void isi_write_reg(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	int *len = 1, p0 = 0, p1 = 0;
	unsigned int p2 = 0;
	unsigned char p3;

	sscanf(userData, "%d %d 0x%x", &p0, &p1, &p2);
	
	p3 = (unsigned char)p2;
	printk("reg = [%d], value = 0x[%x]\n", p1, p3);
	SLIC_write_proslic_reg(pt, p1, len, &p3);
}

void isi_write_ram(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	int *len = 1, p0 = 0, p1 = 0;
	unsigned int p2 = 0;

	sscanf(userData, "%d %d 0x%x", &p0, &p1, &p2);
	
	printk("ram = [%d], value = 0x[%x]\n", p1, p2);
	SLIC_write_proslic_ram(pt, p1, p2);
}

void isi_read_reg(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	int len = 0, p0 = 0, p1 = 0;
	unsigned char p2;

	sscanf(userData, "%d %d", &p0, &p1);
	
	SLIC_read_proslic_reg(pt, p1, &len, &p2); 		
	printk("reg = [%d], value = 0x[%x]\n", p1, p2);
}

void isi_read_ram(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	int p0 = 0, p1 = 0;
	unsigned int p2;

	sscanf(userData, "%d %d", &p0, &p1);
	
	SLIC_read_proslic_ram(pt, p1, &p2);			
	printk("ram = [%d], value = 0x[%x]\n", p1, p2);
}

void isi_modify_reg30(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	unsigned char val = 0x14;
	int reg = 30, *len = 1;
	
	printk("reg = [%d], value = 0x[%x]\n", reg, val);
	SLIC_write_proslic_reg(data, reg, len, &val);
}
	
void isi_modify_reg126(char *userData, void *data, void *reserve){
	voip_snd_t *pt = (voip_snd_t *)data;
	unsigned char val = 0x3;
	int reg = 126, *len = 1;
	
	printk("reg = [%d], value = 0x[%x]\n", reg, val);
	SLIC_write_proslic_reg(data, reg, len, &val);
}
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
void set_slicloopback(char *userData, void *data, void *reserve){
	RTKLineObj *pt = (RTKLineObj *)data;
	int p0 = 0, p1 = 0, p2 = 0;

	sscanf(userData, "%d %d %d", &p0, &p1, &p2);

	if((p1 < 0) || (p1 >= CON_CH_NUM)){
		printk("[%s]not support CH%d\n", __FUNCTION__, p1);
		return;
	}
	
	printk("ch%d\n", p1);

	rtkSetSLICloopback((pt+p1), p2);
}
#endif

void voip_slic_cmd_init(int slic_nr){
	int count = 0;
		
	for(; count < MAX_SLIC_CMD; count++){
		SLIC_CMDS[count].cmd = count;
		
		switch(count){
			case DUMP_REG_VAL:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,2);
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK				
				SLIC_CMDS[count].obj_type = DEV_OBJ;
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)				
				SLIC_CMDS[count].obj_type = SND_OBJ;
#endif
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = voip_dumpreg_val;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Dump all register values:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);
				break;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661)		
			case LINE_TEST:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,6);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_line_test;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"ZARLINK LINE TEST FUNCTION:\n"
				"\tFXS#:0~%d\n"
				"\tLT:3->FXS is PLUGGED\n"
				"\techo %d FSX# LT# > /proc/%s\n"
				"\te.g., echo %d 0 3 > /proc/%s\n", 
				(CON_CH_NUM - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116)
			case LINE_TEST:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,6);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_line_test;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"ZARLINK LINE TEST FUNCTION:\n"
				"\tFXS#:0~%d\n"
				"\tLT:%d->Line Voltage Test\n"
				"\tLT:%d->Receiver Off-Hook indication\n"
				"\tLT:%d->FXS is PLUGGED\n"
				"\tLT:%d->Resistive Fault\n"
				"\techo %d FSX# LT# > /proc/%s\n"
				"\te.g., echo %d 0 2 > /proc/%s\n", 
				(CON_CH_NUM - 1), LT_VOLTAGE, LT_ROH, LT_PORT_DET, LT_RES, count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
#endif
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
			case GET_COEFFICIENT:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,4);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_get_coefficient;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Get CH# coefficient:\n"
				"\tCH#:0~%d\n"
				"\techo %d CH# > /proc/%s\n"
				"\te.g., echo %d 0 > /proc/%s\n", 
				(CON_CH_NUM - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);			
				break;
			case SET_COEFFICIENT:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,4);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_set_coefficient;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Set CH# coefficient:\n"
				"\tCH#:0~%d\n"
				"\techo %d CH# > /proc/%s\n"
				"\te.g., echo %d 0 > /proc/%s\n", 
				(CON_CH_NUM - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);			
				break;
			case DO_CALIBRATION:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,6);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_calibration;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Calibration CH# with MODE#:\n"
				"\tCH#:0~%d\n"
				"\tMODE#:0->This mode initiates a calibration operation for analog circuits associated with a given line.\n"
				"\tMODE#:1->This mode runs a selected calibration option.\n"
				"\techo %d CH# MODE# > /proc/%s\n"
				"\te.g., echo %d 0 1 > /proc/%s\n", 
				(CON_CH_NUM - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
#endif
			case READ_CS_SLIC_ID:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,4);
				SLIC_CMDS[count].obj_type = DEV_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_slic_id;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Read CS# slic id:\n"
				"\tCS#:0~%u\n"
				"\techo %d CS# > /proc/%s\n"
				"\te.g., echo %d 0 > /proc/%s\n", 
				(slic_nr - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
#ifdef ZSI_MODIFY_GAIN
			case MODIFY_SLIC_GAIN:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,8);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_modify_gain;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Modify slic gain:\n"
				"\tCH#:0~%d\n"
				"\tDIR:rx->0, tx->1\n"
				"\tGAIN:gain value\n"
				"\techo %d CH# DIR GAIN > /proc/%s\n"
				"\te.g., echo %d 0 0 2 > /proc/%s\n", 
				(CON_CH_NUM - 1), count, VDUMPREGNAME, count, VDUMPREGNAME);			
				break;
			case SHUTDOWN_DEV:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,2);
				SLIC_CMDS[count].obj_type = VPDEVCTX_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = zsi_shutdown_dev;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Shut down device:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);			
				break;				
#endif
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
			case DUMP_RAM_VAL:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,2);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_dumpram_val;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Dump all RAM vlaues:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);			
				break;
			case WRITE_REG:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,8);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_write_reg;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Write value to REG:\n"
				"\techo %d REG value > /proc/%s\n"
				"\te.g., echo %d 0 0x2 > /proc/%s\n", 
				count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
			case WRITE_RAM:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,8);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_write_ram;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Write value to RAM:\n"
				"\techo %d RAM value > /proc/%s\n"
				"\te.g., echo %d 0 0x2 > /proc/%s\n", 
				count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
			case READ_REG:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,4);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_read_reg;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Read value from REG:\n"
				"\techo %d REG > /proc/%s\n"
				"\te.g., echo %d 2 > /proc/%s\n", 
				count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
			case READ_RAM:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,4);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_read_ram;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Read value from RAM:\n"
				"\techo %d RAM > /proc/%s\n"
				"\te.g., echo %d 2 > /proc/%s\n", 
				count, VDUMPREGNAME, count, VDUMPREGNAME);				
				break;
			case MODIFY_REG30:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,2);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_modify_reg30;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Modify value of REG30 to 0x14:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);				
				break;
			case MODIFY_REG126:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,2);
				SLIC_CMDS[count].obj_type = SND_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = isi_modify_reg126;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Modify value of REG126 to 0x3:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);					
				break;
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
			case SLICLOOP_BACK:
				SLIC_CMDS[count].cmd_argc = TENS_ARGC(count,6);
				SLIC_CMDS[count].obj_type = LINE_OBJ;
				SLIC_CMDS[count].enable = true;
				SLIC_CMDS[count].callback = set_slicloopback;
				sprintf(&SLIC_CMDS[count].illustration[0], 
				"Set up slic into loopback mode:\n"
				"\techo %d > /proc/%s\n", 
				count, VDUMPREGNAME);	

				break;
#endif
			default:
				break;
		} 
	}
}
