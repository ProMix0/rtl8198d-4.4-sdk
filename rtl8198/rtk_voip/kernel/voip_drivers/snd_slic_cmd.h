#ifndef SND_SLIC_CMD_H
#define SND_SLIC_CMD_H
#include "rtk_voip.h"
#include "voip_init.h"
#include "voip_types.h"
#include "voip_feature.h"
//#if  !defined(CONFIG_DEFAULTS_KERNEL_3_10)
#include <linux/delay.h>
//#endif

#define DEVOBJ(name) (&DevObj_## name)

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK //add flag for gain modification feautre here
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662) || \
    defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601) || \
	defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632) || \
	defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653) || \
	defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622)
#define ZSI_MODIFY_GAIN
#endif

#ifdef CONFIG_RTK_VOIP_SLIC_ZARLINK_886_SERIES
#define VP886_DEVICE_REGISTER_COUNT    13
#define VP886_CHANNEL_REGISTER_COUNT    57
#define DEV_REG_SIZE 6
#define CH_REG_SIZE 25
extern uint8 deviceRegs[VP886_DEVICE_REGISTER_COUNT][2];
extern uint8 channelRegs[VP886_CHANNEL_REGISTER_COUNT][2];
extern char *deviceRegsName[VP886_DEVICE_REGISTER_COUNT];
extern char *registerName[VP886_CHANNEL_REGISTER_COUNT];
#endif

#endif

#define SLIC_CMD_LEN 512
#define VDUMPREGNAME "slicdumpreg"
#define TENS_ARGC(x,y) (x/10 > 0)?(y+1):(y)

typedef enum _SlicCmd{
	DUMP_REG_VAL = 0,
	LINE_TEST,
	GET_COEFFICIENT, 
	SET_COEFFICIENT,
	DO_CALIBRATION,
	READ_CS_SLIC_ID,
	MODIFY_SLIC_GAIN,
	SHUTDOWN_DEV,
	DUMP_RAM_VAL,
	WRITE_REG,
	WRITE_RAM,
	READ_REG,
	READ_RAM,
	MODIFY_REG30,
	MODIFY_REG126,
	SLICLOOP_BACK,
	MAX_SLIC_CMD
}SlicCmd;

typedef enum _objType{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
	DEV_OBJ = 0,
	LINE_OBJ,
	VPDEVCTX_OBJ,
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB)
	SND_OBJ,
#endif
	MAX_OBJ
}objType;

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LINETEST
typedef enum _zsiLTCmd{
	LT_VOLTAGE = 1,
	LT_ROH,
	LT_PORT_DET,
	LT_RES,
	LT_END
}zsiLTCmd;
#endif

typedef struct slic_cmd_s{
	int cmd;
	int cmd_argc;
	int obj_type;
	uint8 trigger;
	uint8 enable;
	void(*callback)(char *userData, void *data, void *reserve);
	char illustration[SLIC_CMD_LEN];
}slic_cmd_t;


void voip_slic_cmd_help(void);
void voip_slic_cmd_init(int);
void voip_dumpreg_val(char *, void *, void *);


#endif
