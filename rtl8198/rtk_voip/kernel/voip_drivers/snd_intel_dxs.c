#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include "rtk_voip.h"
#include "voip_init.h"
#include "voip_proc.h"
#include "con_register.h"
#include "con_defer_init.h"
#include "snd_help_ipc.h"
#include "voip_control.h"
#include <linux/kthread.h>


#include "drv_dxs_init.h"
#include "drv_dxs_api.h"
#include "drv_dxs_io_types.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_access.h"	
#include "drv_dxs_bbd.h"
#include "drv_dxs_pcm_priv.h"
#include "drv_dxs_fw_cmd_eop.h"

#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
#define NEWKTHREAD
#endif

extern IFX_TAPI_DRV_CTX_t *RtkCtx;
int init_done = 0;
int tapi_init = 0;

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101
char *intelDev = "dxs101";
#define FXSNUM 1
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
char *intelDev = "dxs102";
#define FXSNUM 2
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_INTEL_SLIC_DEBUG
int debugMode = 1;
#else
int debugMode = 0;
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

//#define DEBUGFLAG

enum initStep{
	FW_STEP,
	BBD_STEP,
	UNKNOWN_STEP
};

#if !defined(CONFIG_DEFAULTS_KERNEL_3_18) && !defined(CONFIG_LUNA)
#include "gpio.h"
#endif
#include "spi.h"

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101)
static int dxsnum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR;
DXS_DEVICE_t				dxsDevice[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
DXS_BasicDeviceInit_t		dxs_init[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
static voip_snd_t			snd_intel_dxs10X[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
#ifndef CONFIG_RTK_VOIP_LINUX_SPI
#ifndef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_SW_CS
static int swGpioNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_SW_CS_GPIO_NUM;
#endif
#endif
static int HWCSNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_PIN_CS;
#endif
#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
uint32 rtkCh[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
lineST_t rtkLf[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
uint32 rtkRing[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
rtkEn_t rtkEn[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
#endif
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
static int dxsnum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR;
DXS_DEVICE_t				dxsDevice[2*CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
DXS_BasicDeviceInit_t		dxs_init[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
static voip_snd_t			snd_intel_dxs10X[2*CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
#ifndef CONFIG_RTK_VOIP_LINUX_SPI
#ifndef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_SW_CS
static int swGpioNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_SW_CS_GPIO_NUM;
#endif
#endif
static int HWCSNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_PIN_CS;
#endif
#ifndef CONFIG_RTK_VOIP_SLIC_CB_RESPECTIVELY
uint32 rtkCh[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
lineST_t rtkLf[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
uint32 rtkRing[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
rtkEn_t rtkEn[FXSNUM * CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
#endif
#endif

extern IFX_int32_t kTAPI_DeviceStart(IFX_uint16_t dev, IFX_TAPI_DEV_START_CFG_t const *param);
extern IFX_int32_t kTAPI_Ring_Start(IFX_uint16_t dev, IFX_uint16_t chn);
extern IFX_int32_t kTAPI_Ring_Stop(IFX_uint16_t dev, IFX_uint16_t chn);
extern IFX_int32_t kTAPI_Phone_Set_Linefeed(IFX_uint16_t dev, IFX_uint16_t chn, IFX_TAPI_LINE_MODE_t mode);
extern void SLIC_Set_FXS_Line_State(uint32 chid, int state);
void disableSLIC(void);
int rtkvoip_slitinitpower = 1;

static int intelFileOp(fw_info_t fwInfo, uint8 **pt, int step){
	int ret = 0;
	uint32 fsize = 0;
	uint8 *pDataBuf = NULL;
	struct file *fp = NULL;
	char filePath[PATHLEN] = {0};
	//struct kstat stat;
	mm_segment_t oldfs;
	
	oldfs = get_fs(); 
	
	set_fs(KERNEL_DS); 
	
	switch(step){
		case FW_STEP:
			strncpy(&filePath[0], fwInfo.fwPath, PATHLEN);
			fsize = fwInfo.fwSize;
			break;
		case BBD_STEP:
			strncpy(&filePath[0], fwInfo.BBDPath, PATHLEN);
			fsize = fwInfo.BBDSize;
			break;
		default:
			printk("[%s] - [%d] unknown case.\n", __FUNCTION__, __LINE__);
			ret = -1;
			goto done;
	}
	
	//open	
	fp = filp_open(&filePath[0], O_RDONLY, 0);

	if(!fp){
		PRINT_R("[%s] - [%d] can't open %s\n", __FUNCTION__, __LINE__, &filePath[0]);
		ret = -1;
		goto done;
	}
	
	//PRINT_G("[%s] - [%d] file path:%s, file size:%u\n", __FUNCTION__, __LINE__, &filePath[0], fsize);
	
	//allocate memory for intel file
	pDataBuf = (uint8 *)kmalloc((fsize + 1), GFP_KERNEL);
	
	if(pDataBuf == NULL){
		PRINT_R("[%s] - [%d] can't alloc memory\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto done;
	}
	
	//read
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))
	if (fp->f_op && fp->f_op->read)
	{
		ret = fp->f_op->read(fp, pDataBuf, fsize, &fp->f_pos);
#else
	if(1) //for kernel 4.4.x
    {
		ret = kernel_read(fp, 0, pDataBuf, fsize);
#endif
		
		if(ret != fsize){	
			printk("[%s] - [%d] can't read file, file size = %d, read byte = %d\n", __FUNCTION__, __LINE__, fsize, ret);
			ret = -1;
			goto done;
		}
	}
	else{
		printk("[%s] - [%d] no read operation\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto done;
	}	
	
	pDataBuf[fsize] = '\0';
	*pt = pDataBuf;
	//printk("[%s] - open, read success, fsize = %u, stat.size = %lld\n", __FUNCTION__, *fsize, stat.size);
	
done:
	if(fp != NULL)
		filp_close(fp,NULL);
	
	set_fs(oldfs);
	
	return ret;
}

static int intelInit(DXS_DEVICE_t *pDev, DXS_IO_Init_t *IoInit, DXS_BBD_Download_t *bbdDwld){
	int ret = 0, ch = 0;
	IFX_TAPI_PCM_IF_CFG_t pCfg;
	IFX_TAPI_PCM_CFG_t pPCMConfig[FXSNUM];
	IFX_TAPI_DEV_START_CFG_t pDevStartCfg;

	
	ret = DXS_FW_Start(pDev, IoInit);
	
	if(ret == DXS_statusOk){
		printk("FW dl OK\n");
	}
	else{
		printk("FW dl failed\n");
		return ret;
	}
	
	memset(&pDevStartCfg, 0, sizeof(IFX_TAPI_DEV_START_CFG_t));
	pDevStartCfg.dev = pDev->nDevNr;
	pDevStartCfg.nMode = IFX_TAPI_INIT_MODE_DEFAULT;

	ret = kTAPI_DeviceStart(pDev->nDevNr, &pDevStartCfg);
	
	if(ret == DXS_statusOk){
		printk("TAPI init OK\n");
		tapi_init++;
	}
	else{
		printk("TAPI init failed, ret = %x\n", ret);
		return ret;
	}

	ret = DXS_BBD_Download((DXS_CHANNEL_t *)(IFX_void_t *)pDev, bbdDwld);
	
	if(ret == DXS_statusOk){
		printk("BBD dl OK\n");
	}
	else{
		printk("BBD dl failed\n");
		return ret;
	}


	for(ch = 0; ch < FXSNUM; ch++){
		ret = kTAPI_Phone_Set_Linefeed(pDev->nDevNr, ch, IFX_TAPI_LINE_FEED_STANDBY);
		
		if(ret == DXS_statusOk){
			printk("ch%d line feed OK\n", ch);
		}
		else{
			printk("ch%d line feed failed\n", ch);
			return ret;
		}			
	}	

	
	pCfg.nHighway = 0;
	pCfg.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE;
	pCfg.nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_2048;
	pCfg.nDoubleClk = IFX_DISABLE;
	pCfg.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
	pCfg.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	pCfg.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_7;
#else	
	pCfg.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_1;
#endif
	pCfg.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_1;
	pCfg.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
	pCfg.nShift = IFX_DISABLE;
	pCfg.nMCTS = 0x00;
	
	if(RtkCtx->PCM.ifCfg){
		ret = RtkCtx->PCM.ifCfg(pDev, &pCfg);
	
		if(ret == DXS_statusOk){
			printk("PCM IF CFG OK\n");
		}
		else{
			printk("PCM IF CFG failed\n");
			return ret;
		}
	}else{
		printk("RtkCtx->PCM.ifCfg is null\n");
		return ret;		
	}	
		
	if(RtkCtx->PCM.Cfg){
		for(ch = 0; ch < FXSNUM; ch++){
	
		pPCMConfig[ch].ch = ch;
		pPCMConfig[ch].nTimeslotRX = snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].TS1;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
		pPCMConfig[ch].nTimeslotTX = snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].TS1 - 1;
#else
		pPCMConfig[ch].nTimeslotTX = snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].TS1;
#endif
		pPCMConfig[ch].nHighway = 0;
		pPCMConfig[ch].nResolution = IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
		pPCMConfig[ch].nSampleSwap = IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED;
		pPCMConfig[ch].nBitPacking = IFX_TAPI_PCM_BITPACK_LSB;
		pPCMConfig[ch].nBitOrder = IFX_TAPI_PCM_BITORDER_NORMAL;
#if 0//defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_WIDEBAND) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_WIDEBAND)
		snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].TS2 = ( daa || TS_base + ch * 2 >= 16 ? 0 : TS_base + ( ch + 8 ) * 2 );
		snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].band_mode_sup = ( daa ? BAND_MODE_8K : BAND_MODE_8K | BAND_MODE_16K );
//#else
		snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].TS2 = 0;
		snd_intel_dxs10X[ pDev->nDevNr * FXSNUM + ch ].band_mode_sup = BAND_MODE_8K;
#endif
		//printk("%s - %d, ch%d RX = %d, TX = %d, TS_cur = %d\n", __func__, __LINE__, ch, pPCMConfig[ch].nTimeslotRX, pPCMConfig[ch].nTimeslotTX, TS_cur);
			ret = RtkCtx->PCM.Cfg(&pDev->pChannel[ch], &pPCMConfig[ch]);
		
			if(ret == DXS_statusOk){
				printk("ch%d PCM CH CFG OK\n", ch);
			}
			else{
				printk("ch%d PCM CH CFG failed, ret = %d\n", ch, ret);
				return ret;
			}		
		}
	}
	else{
		printk("RtkCtx->PCM.Cfg is null\n");
		return ret;
	}
	
	if(RtkCtx->PCM.Enable){
		for(ch = 0; ch < FXSNUM; ch++){
			ret = RtkCtx->PCM.Enable(&pDev->pChannel[ch], 1, &pPCMConfig[ch]);			
			
			if(ret == DXS_statusOk){
				printk("ch%d PCM CH enable OK\n", ch);
			}
			else{
				printk("ch%d PCM CH enable failed, ret = %04x\n", ch, ret);
				return ret;
			}				
		}
	}
	else{
		printk("RtkCtx->PCM.Enable is null\n");
		return ret;
	}

	return 0;
}

#ifdef NEWKTHREAD
static int intelEnable(DXS_DEVICE_t *data, int ch, int index){
	DXS_DEVICE_t *pDev = data;
	DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)&pDev->pChannel[ch];
	IFX_TAPI_PCM_CFG_t pPCMConfig;
	int ret = 0;
	
	//index = (pDev->nDevNr * dxsnum) + ((pCh->nChannel) - 1);
	
	pPCMConfig.ch = (pCh->nChannel) - 1;
	pPCMConfig.nTimeslotRX = rtkEn[index].TS1;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	pPCMConfig.nTimeslotTX = rtkEn[index].TS1 - 1;
#else
	pPCMConfig.nTimeslotTX = rtkEn[index].TS1;
#endif

	pPCMConfig.nHighway = 0;
	pPCMConfig.nSampleSwap = IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED;
	pPCMConfig.nBitPacking = IFX_TAPI_PCM_BITPACK_LSB;
	pPCMConfig.nBitOrder = IFX_TAPI_PCM_BITORDER_NORMAL;

	//printk("%s-%d rtkEn[%d].enable:%d, ch:%d, TS:%u\n", __func__, __LINE__, index, rtkEn[index].enable, pPCMConfig.ch, pPCMConfig.nTimeslotRX);
	
	if(rtkEn[index].enable != 0){
		ret = RtkCtx->PCM.Enable(pCh, 0, &pPCMConfig);
#ifdef DEBUGFLAG
		if(ret == 0)
			printk("[%s]-[%d] disable pcm OK\n", __func__, __LINE__);
		else
			printk("[%s]-[%d] disable pcm failed, ret = %x\n", __func__, __LINE__, ret);
#endif		
	}
	
	if (rtkEn[index].enable == 2) { /* Change to wideband mode */
#ifdef DEBUGFLAG	
		printk("Switch to Wideband mode, band = %d\n", pCh->pPCM->fw_pcm_ch.COD);
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
				printk("[%s]-[%d] Unknow codec %u\n", __func__, __LINE__, pCh->pPCM->fw_pcm_ch.WIDE);
				break;
		}
	}
	else if (rtkEn[index].enable == 1) { /* Change to narrow band mode */
#ifdef DEBUGFLAG	
		printk("Switch to Narrow band mode, band = %d\n", pCh->pPCM->fw_pcm_ch.COD);
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
		printk("[%s]-[%d] no need to change band mode\n", __func__, __LINE__);
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
	return 0;
}

static int Function_Event_Polling_INTEL_Timer(void *data)
{
	DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)data;
	uint8 pCurrentOpmode = 0;

	int ret = 0, i = 0, index = 0;		
	
	while(!init_done)
		msleep(10);
	
	while(i < FXSNUM){
		//msleep(10);
		msleep(5);
		
		/*if(!init_done){
			msleep(10);
			continue;
		}*/
		
		// get line status
		ret = DXS_ALM_OpmodeGet(&pDev->pChannel[i], &pCurrentOpmode);
		index = (pDev->nDevNr * FXSNUM) + i; //device# * total_device# + channel# e.g., dev0 * 2 + ch1 = rtkCH[1]
		rtkCh[index] = pCurrentOpmode;

		// do linefeed if needed
		if(rtkLf[index].linefeed == 1){
			rtkLf[index].linefeed = 0;
			ret = kTAPI_Phone_Set_Linefeed(rtkLf[index].dev, rtkLf[index].chn, rtkLf[index].mode);
#ifdef DEBUGFLAG			
			if(ret != 0)
				printk("[%s] - [%d] index %d set line feed fail, ret = %x\n", __func__, __LINE__, index, ret);
			else
				printk("[%s] - [%d] index %d set line feed ok\n", __func__, __LINE__, index);
#endif			
		}
	
		// do enable
		if(rtkEn[index].enable != 0){
			intelEnable(pDev, i, index);
			rtkEn[index].enable = 0;
		}
	
		// do ring
		if(rtkRing[index] == 1){ // start ring
			ret = kTAPI_Ring_Start(pDev->nDevNr, index);
			rtkRing[index] = 0;
#ifdef DEBUGFLAG			
			if(ret != 0)
				printk("[%s] - [%d] index %d start ring fail, ret = %x\n", __func__, __LINE__, index, ret);
			else
				printk("[%s] - [%d] index %d start ring ok\n", __func__, __LINE__, index);
#endif			
		}
		else if(rtkRing[index] == 2){ // stop ring, it should be TAPI_statusPhoneOffHook
			ret = kTAPI_Ring_Stop(pDev->nDevNr, index);
			rtkRing[index] = 0;
#ifdef DEBUGFLAG			
			if(ret != 0)
				printk("[%s] - [%d] index %d stop ring fail, ret = %x\n", __func__, __LINE__, index, ret);
			else
				printk("[%s] - [%d] index %d stop ring ok\n", __func__, __LINE__, index);
#endif				
		}
		
		i++;
		if(i == FXSNUM)
			i = 0;
	}
	
	return ret;

}
#endif //NEWKTHREAD

static int SLIC_init_dxs(int pcm_mode, int initonly, fw_info_t fwInfo)
{
	int ret = 0, i = 0;
	DXS_DEVICE_t *pDev = NULL;
	DXS_IO_Init_t IoInit;
	DXS_BBD_Download_t bbdDwld;
	uint8 *pFW_PramUsed = NULL, *pBBDbuf = NULL;

#ifndef CONFIG_RTK_VOIP_LINUX_SPI
	extern void slicInternalCS(int cs);
#endif
	
// setup pcm fs, slic reset
#if defined(CONFIG_RTK_VOIP_PLATFORM_8686) || defined(CONFIG_RTL8686_SLIC_RESET)
	extern unsigned int soc_set_pcm_fs;

#ifdef CONFIG_RTK_VOIP_PLATFORM_8686
	extern void aipc_module_voip_set_pcm_fs(void);
#endif

#ifdef CONFIG_RTL8686_SLIC_RESET
	extern void aipc_module_voip_slic_reset(void);
#else /* Reset signal is necessary for Intel slic */
	PRINT_R("[%s] - [%d] SLIC reset pin is not defined\n", __func__, __LINE__);
#endif

    if( soc_set_pcm_fs == 0 ){
#ifdef CONFIG_RTK_VOIP_PLATFORM_8686
		aipc_module_voip_set_pcm_fs();
#endif
#ifdef CONFIG_RTL8686_SLIC_RESET
		aipc_module_voip_slic_reset();
#endif
		soc_set_pcm_fs = 1;
	}
#endif // setup pcm fs, slic reset
	
#ifndef CONFIG_RTK_VOIP_LINUX_SPI	
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101)
	slicInternalCS(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_PIN_CS);
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
	slicInternalCS(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_PIN_CS);
#endif
#endif

	if( initonly )
		goto label_do_init_only;

	ret = intelFileOp(fwInfo, &pFW_PramUsed, FW_STEP);
	
	if(ret <= 0){
		printk("[%s] - [%d] intelFileOp %s failed, ret = %d\n", __FUNCTION__, __LINE__, fwInfo.fwPath, ret);
		return ret;
	}

	memset (&IoInit, 0, sizeof(IoInit));
	IoInit.pPRAMfw = pFW_PramUsed;
	IoInit.pram_size = fwInfo.fwSize;

	ret = intelFileOp(fwInfo, &pBBDbuf, BBD_STEP);
	
	if(ret <= 0){
		printk("[%s] - [%d] intelFileOp %s failed, ret = %d\n", __FUNCTION__, __LINE__, fwInfo.BBDPath, ret);
		return ret;
	}
	
	memset (&bbdDwld, 0, sizeof(bbdDwld));
    bbdDwld.buf  = pBBDbuf;
	bbdDwld.size = fwInfo.BBDSize;
	
	
	for ( i = 0 ; i < dxsnum; i ++ ){
		DXS_GetDevice(i, &pDev);
		memset(&dxs_init, 0, sizeof(DXS_BasicDeviceInit_t));
		dxs_init[i].nIrqNum = -1;
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
		init_spi_dev(&pDev->spi_dev[0], i);
#else
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
		_init_rtl_spi_dev_type_hw(&pDev->spi_dev[0], HWCSNum);
#else	
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_SW_CS) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_SW_CS)
		_init_rtl_spi_dev_type_hw4( &pDev->spi_dev[0], HWCSNum + i, swGpioNum );
#else
		PRINT_R("[%s] - [%d] SW Chip select is not defined\n", __func__, __LINE__);
#endif
#endif
#endif

		ret = DXS_ChipAccessInit(pDev, &dxs_init[i]);

		if(ret != 0){
			PRINT_R("[%s] DXS_ChipAccessInit slic(%d) %s failed, ret =  %x\n", __FUNCTION__, i, intelDev, ret);
			return ret;
		}
		
		ret = intelInit(pDev, &IoInit, &bbdDwld);
		
		if(ret != 0){
			PRINT_R("Initialize Intel slic(%d) %s failed, ret =  %x\n", i, intelDev, ret);
			return ret;
		}
		
		PRINT_G("Initialize Intel slic(%d) %s successfully\n", i, intelDev);
	}

label_do_init_only:
	init_done = 1;
	
	if(!pFW_PramUsed)
		kfree(pFW_PramUsed);

	if(!pBBDbuf)
		kfree(pBBDbuf);
	
	return ret;
}

static void SLIC_reset_dxs( voip_snd_t *this, int codec_law )
{
	//extern int law; // FIXME: chmap

	//SLIC_init_dxs( law, 1 /* init only */ );
}

static void SLIC_show_ID_dxs( voip_snd_t *this )
{
	//RTKLineObj * const pLine = (RTKLineObj * )this ->priv;
	//RTKDevObj * const pDev = pLine ->pDev;
}

static snd_ops_fxs_t snd_dxs_fxs_ops;
static void __init fill_dxs_register_info(
        voip_snd_t snd_intel_dxs[],
        int n_fxs, int m_daa, uint16 TS_base, DXS_DEVICE_t *dxsDevice//, RTKLineObj LineObj_dxs[]
        )
{
	// once call this function:
	//  - one control interface
	//  - n fxs
	//  - m daa
	int sch;
	int daa = 0;

	for( sch = 0; sch < n_fxs + m_daa; sch ++ ) {

		if( sch == n_fxs )
				daa = 1;

		snd_intel_dxs[ sch ].sch = sch;
		snd_intel_dxs[ sch ].name = intelDev;
		snd_intel_dxs[ sch ].snd_type = SND_TYPE_FXS;
		snd_intel_dxs[ sch ].bus_type_sup = BUS_TYPE_PCM;
		snd_intel_dxs[ sch ].TS1 = TS_base + sch * 2;
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_WIDEBAND) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_WIDEBAND)
		snd_intel_dxs[ sch ].TS2 = ( daa || TS_base + sch * 2 >= 16 ? 0 : TS_base + ( sch + 8 ) * 2 );
		snd_intel_dxs[ sch ].band_mode_sup = ( daa ? BAND_MODE_8K : BAND_MODE_8K | BAND_MODE_16K );
#else
		snd_intel_dxs[ sch ].TS2 = 0;
		snd_intel_dxs[ sch ].band_mode_sup = BAND_MODE_8K;
#endif
		snd_intel_dxs[ sch ].snd_ops = ( const snd_ops_t * )&snd_dxs_fxs_ops;
		snd_intel_dxs[ sch ].priv = &dxsDevice->pChannel[sch];
		snd_intel_dxs[ sch ].polarity = 0;
		//printk("%s-%d, ch%d addr:%p\n", __func__, __LINE__, sch, &dxsDevice->pChannel[sch]);
		// DAA port
		if( daa ) {
				snd_intel_dxs[ sch ].snd_type = SND_TYPE_DAA;
				printk( "No snd_ops for DAA!!\n" );
		}
	}
}

int __init voip_snd_intel_init_dxs( void )
{
	extern int law; // FIXME: chmap
	extern const snd_ops_fxs_t snd_intel_fxs_ops;
	int i;//, sch, daa;
	int TS_base = 0, TS_cur = 0, rtn = 0;
	DXS_DEVICE_t *pDev = NULL;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0))
	int ret = 0;
#else	
	struct task_struct *ret = NULL;
#endif

#ifdef CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT
	static defer_init_t dxs_defer;
#endif

	// dxs override proslic base ops
	snd_dxs_fxs_ops = snd_intel_fxs_ops;

	snd_dxs_fxs_ops.SLIC_reset = SLIC_reset_dxs;
	snd_dxs_fxs_ops.SLIC_show_ID = SLIC_show_ID_dxs;

	// get TS base
	TS_base = get_snd_free_timeslot();
	if( TS_base < 0 )
			TS_base = 0;

	TS_cur = TS_base;
	// common port definition
	for( i = 0; i < dxsnum; i ++ ) {
		DXS_GetDevice(i, &pDev);
		memcpy(&dxsDevice[i], pDev, sizeof(DXS_DEVICE_t));
#ifdef NEWKTHREAD
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0))	
		ret = kernel_thread(Function_Event_Polling_INTEL_Timer, ( void * )pDev, (CLONE_FS | CLONE_FILES));
		
		if(ret != -1)
			PRINT_B("Create Slic Event thread successfully.\n");
		else{
			PRINT_B("Create Slic Event thread failed.\n");
			return -1;
		}
			
	
#else
		ret = kthread_run(Function_Event_Polling_INTEL_Timer, (void *)pDev, "RTKSlicEvents");
	
		if (!IS_ERR(ret))
			PRINT_B("Create Slic Event thread successfully.\n");
		else{
			PRINT_B("Create Slic Event thread failed.\n");
			return -1;
		}
#endif

#endif
#ifdef 	CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101	
		fill_dxs_register_info( &snd_intel_dxs10X[ i ],
						FXSNUM /* fxs */, 0 /* daa */, (uint16)TS_cur, pDev//&dxsDevice[i]//,	&LineObj_dxs[ i ] 
						);
#else
		fill_dxs_register_info( &snd_intel_dxs10X[ i * 2 ],
						FXSNUM /* fxs */, 0 /* daa */, (uint16)TS_cur, pDev//&dxsDevice[i * 2]//,	&LineObj_dxs[ i ] 
						);	
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_OWNER_ALL_DSP
		if( fill_ipc_dsp_cpuid( &snd_intel_dxs10X[ i ],
						FXSNUM /* fxs */, 0 /* daa */,
						i,
						SND_PER_DSPCPU( dxsnum,
														CONFIG_RTK_VOIP_DSP_DEVICE_NR ) ) )
		{
			TS_cur = TS_base;
		} else
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_OWNER_ALL_DSP)
		if( fill_ipc_dsp_cpuid( &snd_intel_dxs10X[ i * 2],
						FXSNUM /* fxs */, 0 /* daa */,
						i,
						SND_PER_DSPCPU( dxsnum,
														CONFIG_RTK_VOIP_DSP_DEVICE_NR ) ) )
		{
			TS_cur = TS_base;
		} else
#endif
		{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101			
			TS_cur += 2;
#else
			if(FXSNUM == 1)
				TS_cur += 2;
			else
				TS_cur += 4;
#endif
		}
		//TS_base += 4;
	}

	// SLIC init use ops
#ifdef CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT
	dxs_defer.fn_defer_func = ( fn_defer_func_t )SLIC_init_dxs;
	dxs_defer.p0 = law;
	dxs_defer.p1 = 0;

	add_defer_initialization( &dxs_defer );
	rtn = 0; //success
#else
	rtn = SLIC_init_dxs( law, 0 /* allocate */ );
#endif

	if ( rtn == FAILED ) /* init Device fail */
		return -1;

	for( i = 0; i < dxsnum; i ++ )
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101
		register_voip_snd( &snd_intel_dxs10X[ i ], 1 );
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
		register_voip_snd( &snd_intel_dxs10X[ i * 2], 2 );
#endif

	return 0;
}

voip_initcall_snd( voip_snd_intel_init_dxs );


#define DXSBINARY	"dxsfirmware"
#if 0
static int dxsfirmwaresize = 0;

static int fxs_firmware_write(struct file *file, const char *buffer, 
                               unsigned long count, void *data)
{
	char tmp[128];
	int t1 = 0;
	dxsfirmwaresize = count;
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
		sscanf(tmp, "%d", &t1);
		printk("input = [%d]\n", t1);
	}
	return count;
}
#endif
#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int dxs_firmware_read(struct seq_file *f, void *v)
{
	return 0;
}
#else
static int dxs_firmware_read(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{	
	return 0;
}
#endif

static int dxs_firmware_open(struct inode *inode, struct file *file)
{
	return single_open(file, dxs_firmware_read, NULL);
}

static int spi_test(struct file *file, const char *buffer, 
                               unsigned long count, void *data)
{
	char tmp[128];
	int t1 = 0;
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
		sscanf(tmp, "%d", &t1);
		printk("input = [%d]\n", t1);
	}
	return count;
}


static ssize_t dxs_firmware_write(struct file *file, const char __user * userbuf, size_t count, loff_t * off) {
	char tmp[128];
	int t1 = 0;
	
	if (userbuf && !copy_from_user(tmp, userbuf, 128)) {
		sscanf(tmp, "%d", &t1);
		printk("input = [%d]\n", t1);	
		
		if ( t1 == 0 )
        return spi_test(file, userbuf, count, NULL);
		else if ( t1 == 1 ){
			disableSLIC();
		}else if ( t1 == 2 ){
			SLIC_Set_FXS_Line_State(0, 0);
		}
		else if (t1 == 3){
			printk("[%s] - [%d] Disable slic debug mode.\n", __func__, __LINE__);
			debugMode = 0;
		 }
		else if (t1 == 4){
			printk("[%s] - [%d] Enable slic debug mode.\n", __func__, __LINE__);
			debugMode = 1;
		}
	}
	return count;
}

struct file_operations proc_dxs_firmware_fops = {
	.owner	= THIS_MODULE,
	.open	= dxs_firmware_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
	.write  = dxs_firmware_write,
};

int __init dxs_firmware_init(void)
{
	struct proc_dir_entry *dxs_firmware_proc;
	

	dxs_firmware_proc = proc_create(DXSBINARY, 0644, NULL, &proc_dxs_firmware_fops);
	if (dxs_firmware_proc == NULL) {
		remove_proc_entry(DXSBINARY, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				DXSBINARY);
		return -ENOMEM;
	}	
	return 0;
}

void __exit dxs_firmware_exit( void )
{
	remove_proc_entry( DXSBINARY, NULL );
}

module_init( dxs_firmware_init );
module_exit( dxs_firmware_exit );

extern IFX_void_t    DXS_DeviceDriverStop (void);
extern IFX_int32_t kTAPI_DeviceStop(IFX_uint16_t dev);
void disableSLIC(void){
	DXS_DeviceDriverStop();
	kTAPI_DeviceStop(0);
#ifdef CONFIG_RTL8686_SLIC_RESET
	_rtl_generic_setGpioDataBit(CONFIG_RTL8686_SLIC_RESET_GPIO_NUM, 0);
#else
	PRINT_R("[%s] - [%d] SLIC reset pin is not defined\n", __func__, __LINE__);
#endif	
}
#if 0
#define VDUMPREGNAME2 "slicdumpreg3"

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int voip_dumpreg2_read(struct seq_file *f, void *v)
{
	return 0;
}
#else
static int voip_dumpreg2_read(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{	
	return 0;
}
#endif

static int voip_dumpreg2_write(struct file *file, const char *buffer, 
                               unsigned long count, void *data)
{
	extern int spi_debug_flag;
	char tmp[128];
	int i = 0;
	unsigned char sendbuf[1] = {0};
	unsigned char recvbuf[16] = {0};
	int t1 = 0;
	DXS_DEVICE_t *pDev = NULL;
	
	DXS_GetDevice(0, &pDev);
	
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
		sscanf(tmp, "%d", &t1);
		printk("input = [%d]\n", t1);

		//_init_rtl_spi_dev_type_hw(&spi_dev_test[ 0 ], CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_PIN_CS);
		printk("%s-%d &pDev->spi_dev[ 0 ].hw.spiCS_ = %d, spi_dev addr = %p\n", __func__, __LINE__, pDev->spi_dev[ 0 ].hw.spiCS_, &pDev->spi_dev[ 0 ]);
		
		spi_debug_flag = 1;
		sendbuf[0] = 0x7e;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x24;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0xaa;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x55;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0xaa;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x55;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		

//		sleep(3);
		sendbuf[0] = 0xbe;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		//sendbuf[0] = 0x18;
		sendbuf[0] = 0x24;
		_rtl_spi_rawWrite(&pDev->spi_dev[ 0 ], &sendbuf[0], 8);
		
		printk("\n");
		for ( i = 0 ; i < 10 ; i ++ ){
		//printk("0x%02x\n", recvbuf[0]);
		_rtl_spi_rawRead(&pDev->spi_dev[ 0 ], &recvbuf[i], 8);
		printk("%d : 0x%02x\n", i, recvbuf[i]);
		}
		printk("\n");
//		for ( i = 0 ; i < 10 ; i ++ )printk("0x%02x ", recvbuf[i]);
		
		printk("\n");
		spi_debug_flag = 0;
	}
	return count;
}

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int spi_dumpreg2_open(struct inode *inode, struct file *file)
{
	return single_open(file, voip_dumpreg2_read, NULL);
}

static ssize_t spi_dumpreg2_write(struct file *file, const char __user * userbuf, size_t count, loff_t * off) {
        return voip_dumpreg2_write(file, userbuf, count, NULL);
}

struct file_operations proc_spi_dumpreg3_fops = {
	.owner	= THIS_MODULE,
	.open	= spi_dumpreg2_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
	.write  = spi_dumpreg2_write,
};
#endif

int __init voip_proc_dumpreg3_init(void)
{
	struct proc_dir_entry *voip_add_test_proc;
	
#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
		voip_add_test_proc = proc_create(VDUMPREGNAME2, 0644, NULL, &proc_spi_dumpreg3_fops);
		if (voip_add_test_proc == NULL) {
			remove_proc_entry(VDUMPREGNAME2, NULL);
			printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
					VDUMPREGNAME2);
			return -ENOMEM;
		}
#else
	voip_add_test_proc = create_proc_entry( VDUMPREGNAME2, 0644, NULL);
	if (voip_add_test_proc == NULL) {
		remove_proc_entry(VDUMPREGNAME2, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				VDUMPREGNAME2);
		return -ENOMEM;
	}
	voip_add_test_proc->read_proc  = (read_proc_t *)voip_dumpreg2_read;
	voip_add_test_proc->write_proc  = (write_proc_t *)voip_dumpreg2_write;
#endif
	
	return 0;
}

void __exit voip_proc_dumpreg3_exit( void )
{
	remove_proc_entry( VDUMPREGNAME2, NULL );
}

module_init( voip_proc_dumpreg3_init );
module_exit( voip_proc_dumpreg3_exit );
#endif