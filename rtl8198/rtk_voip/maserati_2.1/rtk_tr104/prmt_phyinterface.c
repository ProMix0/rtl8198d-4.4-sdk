#include "prmt_limit.h"
#include "prmt_phyinterface.h"
#include "mib_def.h"
#include "mib_tr104.h"
#include "str_utility.h"
#include "str_mib.h"
#include "cwmpevt.h"
#include "cwmp_main_tr104.h"

//******* VoiceService.{i}.PhyInterface.{i}.Tests.X_SimulateTest. EntityLEAF*******
struct CWMP_OP tTestsXCT_SimulateTestEntityLeafOP = { getXCTSimulateTestEntity, setXCTSimulateTestEntity};	

struct CWMP_PRMT tTestsXCT_SimulateTestEntityLeafInfo[] =
{
    /*(name,                   type,           flag,                 op)*/
    { "TestType",              eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tTestsXCT_SimulateTestEntityLeafOP},
    { "CalledNumber",          eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tTestsXCT_SimulateTestEntityLeafOP},
    { "DailDTMFConfirmEnable", eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE, &tTestsXCT_SimulateTestEntityLeafOP},
    { "DailDTMFConfirmNumber", eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tTestsXCT_SimulateTestEntityLeafOP},
    { "DailDTMFConfirmResult", eCWMP_tSTRING,  CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
    { "Status",                eCWMP_tSTRING,  CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
    { "Conclusion",            eCWMP_tSTRING,  CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
    { "CallerFailReason",      eCWMP_tSTRING,  CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
    { "CalledFailReason",      eCWMP_tSTRING,  CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
    { "FailedResponseCode",    eCWMP_tUINT,    CWMP_READ,            &tTestsXCT_SimulateTestEntityLeafOP},
};

enum eTestsXCT_SimulateTestEntityLeaf
{
    eXCT_ST_TestType,
    eXCT_ST_CalledNumber,
    eXCT_ST_DailDTMFConfirmEnable,
    eXCT_ST_DailDTMFConfirmNumber,
    eXCT_ST_DailDTMFConfirmResult,
    eXCT_ST_Status,
    eXCT_ST_Conclusion,
    eXCT_ST_CallerFailReason,
    eXCT_ST_CalledFailReason,
    eXCT_ST_FailedResponseCode
};

struct CWMP_LEAF tTestsXCT_SimulateTestEntityLeaf[] =
{
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_TestType]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_CalledNumber]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_DailDTMFConfirmEnable]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_DailDTMFConfirmNumber]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_DailDTMFConfirmResult]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_Status]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_Conclusion]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_CallerFailReason]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_CalledFailReason]  },
    { &tTestsXCT_SimulateTestEntityLeafInfo[eXCT_ST_FailedResponseCode]  },
    { NULL }
};

#ifdef CONFIG_CU
//******* VoiceService.{i}.PhyInterface.{i}.Tests.X_LineTest.EntityLEAF *******
struct CWMP_OP tTestsXCT_LineTestEntityLeafOP = { getXCTLineTestEntity, setXCTLineTestEntity};	

struct CWMP_PRMT tTestsXCT_LineTestEntityLeafInfo[] =
{
    /*(name,                    type,           flag,                   op)*/
    { "IsTestOnBusy",           eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,   &tTestsXCT_LineTestEntityLeafOP},
    { "Conclusion",             eCWMP_tSTRING,  CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "AGACVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "BGACVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "ABACVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "AGDCVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "BGDCVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "ABDCVoltage",            eCWMP_tINT,     CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "AGInsulationResistance", eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "BGInsulationResistance", eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "ABInsulationResistance", eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "AGCapacitance",          eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "BGCapacitance",          eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
    { "ABCapacitance",          eCWMP_tUINT,    CWMP_READ,              &tTestsXCT_LineTestEntityLeafOP},
};

enum eTestsXCT_LineTestEntityLeaf
{
    eXCT_LT_IsTestOnBusy,
    eXCT_LT_Conclusion,
    eXCT_LT_AGACVoltage,
    eXCT_LT_BGACVoltage,
    eXCT_LT_ABACVoltage,
    eXCT_LT_AGDCVoltage,
    eXCT_LT_BGDCVoltage,
    eXCT_LT_ABDCVoltage,
    eXCT_LT_AGInsulationResistance,
    eXCT_LT_BGInsulationResistance,
    eXCT_LT_ABInsulationResistance,
    eXCT_LT_AGCapacitance,
    eXCT_LT_BGCapacitance,
    eXCT_LT_ABCapacitance,
};

struct CWMP_LEAF tTestsXCT_LineTestEntityLeaf[] =
{
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_IsTestOnBusy]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_Conclusion]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_AGACVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_BGACVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_ABACVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_AGDCVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_BGDCVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_ABDCVoltage]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_AGInsulationResistance]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_BGInsulationResistance]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_ABInsulationResistance]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_AGCapacitance]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_BGCapacitance]  },
    { &tTestsXCT_LineTestEntityLeafInfo[eXCT_LT_ABCapacitance]  },
    { NULL }
};
#endif //CONFIG_CU

//******* VoiceService.{i}.PhyInterface.{i}.X_Stats.PoorQualityList.{i}. EntityLEAF*******
struct CWMP_OP tXCT_STPoorQualityListEntityLeafOP = { getXCT_STPoorQualityListEntity, NULL};	

struct CWMP_PRMT tPXCT_STPoorQualityListEntityLeafInfo[] =
{
    /*(name,             type,            flag,      op)*/
    { "StatTime",        eCWMP_tDATETIME, CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "TxPackets",       eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "RxPackets",       eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "MeanDelay",       eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "MeanJitter",      eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "FractionLoss",    eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "LocalIPAddress",  eCWMP_tSTRING,   CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "LocalUDPPort",    eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "FarEndIPAddress", eCWMP_tSTRING,   CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "FarEndUDPPort",   eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "MosLq",           eCWMP_tUINT,     CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
    { "Codec",           eCWMP_tSTRING,   CWMP_READ, &tXCT_STPoorQualityListEntityLeafOP},
};

enum eXCT_STPoorQualityListEntityLeaf
{
    ePQL_StatTime,
	ePQL_TxPackets,
    ePQL_RxPackets,
    ePQL_MeanDelay,
    ePQL_MeanJitter,
    ePQL_FractionLoss,
    ePQL_LocalIPAddress,
    ePQL_LocalUDPPort,
    ePQL_FarEndIPAddress,
    ePQL_FarEndUDPPort,
    ePQL_MosLq,
    ePQL_Codec
};

struct CWMP_LEAF tXCT_STPoorQualityListEntityLeaf[] =
{
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_StatTime]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_TxPackets]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_RxPackets]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_MeanDelay]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_MeanJitter]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_FractionLoss]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_LocalIPAddress]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_LocalUDPPort]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_FarEndIPAddress]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_FarEndUDPPort]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_MosLq]  },
    { &tPXCT_STPoorQualityListEntityLeafInfo[ePQL_Codec]  },
    { NULL }
};

//******* VoiceService.{i}.PhyInterface.{i}.X_Stats.PoorQualityList.{i}.  *******
struct CWMP_PRMT tXCT_STPoorQualityListObjectInfo[] =
{
    /*(name,  type,           flag,		               op)*/
    {"0",     eCWMP_tOBJECT,  CWMP_READ|CWMP_LNKLIST,  NULL}
};
enum eXCT_STPoorQualityListObject
{
    eXCT_STPoorQualityList0
};
struct CWMP_LINKNODE tXCT_STPoorQualityListObject[] =
{
    /*info,  			                                         leaf,                              next, sibling, instnum)*/
    {&tXCT_STPoorQualityListObjectInfo[eXCT_STPoorQualityList0], tXCT_STPoorQualityListEntityLeaf,  NULL, NULL,    0}
};


//******* VoiceService.{i}.PhyInterface.{i}.X_Stats EntityObject*******
struct CWMP_OP tPhyPoorQualityListOP = { NULL, objPhyPoorQualityList };

struct CWMP_PRMT tXCT_StatusEntityObjectInfo[] =
{
    /*(name,            type,           flag,       op)*/
    {"PoorQualityList", eCWMP_tOBJECT,  CWMP_READ,  &tPhyPoorQualityListOP}
};
enum eXCT_StatusEntityObject
{
	eXCT_StatusPoorQualityList
};
struct CWMP_NODE tXCT_StatusEntityObject[] =
{
    /*info,                                                    leaf, next)*/
    {&tXCT_StatusEntityObjectInfo[eXCT_StatusPoorQualityList], NULL, NULL},
    {NULL,                                                     NULL, NULL}
};

//******* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Stats EntityLEAF*******
struct CWMP_OP tXCT_StatusEntityLeafOP = { getXCT_StatusEntity, setXCT_StatusEntity};	

struct CWMP_PRMT tXCT_StatusEntityLeafInfo[] =
{
    /*(name,               type,           flag,                 op)*/
	{ "ResetStatistics", eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE, &tXCT_StatusEntityLeafOP}
};

enum eXCT_StatusEntityLeaf
{
    eX_Stats_ResetStatistics
};

struct CWMP_LEAF tXCT_StatusEntityLeaf[] =
{

    { &tXCT_StatusEntityLeafInfo[eX_Stats_ResetStatistics]  },
    { NULL }
};

#ifdef CONFIG_CU
//******* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Extend EntityLEAF *******
struct CWMP_OP tXCT_ExtendEntityLeafOP = { getXCT_ExtendEntity, setXCT_ExtendEntity};	

struct CWMP_PRMT tXCT_ExtendEntityLeafInfo[] =
{
    /*(name,                    type,               flag,                 op)*/
	{ "DialMode",               eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "HookFlashUpTime",        eCWMP_tUINT,        CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "HookFlashDownTime",      eCWMP_tUINT,        CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "ReversePolePulse",       eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "KcType",                 eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "ClipTransWhen",          eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "ClipReversePole",        eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "MWIRingFlag",            eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "ClipFormat",             eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "RingDCVoltageOverlapped",eCWMP_tUINT,        CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
    { "DataServiceMode",        eCWMP_tUINT,        CWMP_READ|CWMP_WRITE, &tXCT_ExtendEntityLeafOP},
};

enum eXCT_ExtendEntityLeaf
{
    eX_Extend_DialMode,
    eX_Extend_HookFlashUpTime,
    eX_Extend_HookFlashDownTime,
    eX_Extend_ReversePolePulse,
    eX_Extend_KcType,
    eX_Extend_ClipTransWhen,
    eX_Extend_ClipReversePole,
    eX_Extend_MWIRingFlag,
    eX_Extend_ClipFormat,
    eX_Extend_RingDCVoltageOverlapped,
    eX_Extend_DataServiceMode,
};

struct CWMP_LEAF tPhyInfXCT_ExtendEntityLeaf[] =                   
{
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_DialMode]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_HookFlashUpTime]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_HookFlashDownTime]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_ReversePolePulse]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_KcType]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_ClipTransWhen]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_ClipReversePole]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_MWIRingFlag]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_ClipFormat]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_RingDCVoltageOverlapped]  },
    { &tXCT_ExtendEntityLeafInfo[eX_Extend_DataServiceMode]  },
    { NULL }
};
#endif //CONFIG_CU

//******* VoiceService.{i}.PhyInterface.{i}.Tests EntityLEAF*******
struct CWMP_OP tPhyInfTestsEntityLeafOP = { getPhyInfTestsEntity, setPhyInfTestsEntity};	

struct CWMP_PRMT tPhyInfTestsEntityLeafInfo[] =
{
    /*(name,               type,           flag,                 op)*/
	{ "TestState",         eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tPhyInfTestsEntityLeafOP},
	{ "TestSelector",      eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tPhyInfTestsEntityLeafOP},
	{ "PhoneConnectivity", eCWMP_tBOOLEAN, CWMP_READ,            &tPhyInfTestsEntityLeafOP}
};

enum ePhyInfTestsEntityLeaf
{
    eTests_TestState,
    eTests_TestSelector,
    eTests_PhoneConnectivity
};

struct CWMP_LEAF tPhyInfTestsEntityLeaf[] =
{
    { &tPhyInfTestsEntityLeafInfo[eTests_TestState]  },
    { &tPhyInfTestsEntityLeafInfo[eTests_TestSelector]  },
    { &tPhyInfTestsEntityLeafInfo[eTests_PhoneConnectivity]  },
    { NULL }
};

//******* VoiceService.{i}.PhyInterface.{i}.Tests. EntityObject*******
struct CWMP_PRMT tPPhyInfTestsEntityObjectInfo[] =
{
    /*(name,                  type,           flag,       op)*/
    {"X_"CTCOM_NAME_PREFIX"_SimulateTest", eCWMP_tOBJECT,  CWMP_READ,  NULL},
    {"X_"CTCOM_NAME_PREFIX"_LineTest",     eCWMP_tOBJECT,  CWMP_READ,  NULL}
};
enum ePhyInfTestsEntityObject
{
	eTests_XCT_SimulateTest,
    eTests_XCT_LineTest
};
struct CWMP_NODE tPhyInfTestsEntityObject[] =
{
    /*info,                                                   leaf,                             next)*/
    {&tPPhyInfTestsEntityObjectInfo[eTests_XCT_SimulateTest], tTestsXCT_SimulateTestEntityLeaf, NULL},
#ifdef CONFIG_CU
    {&tPPhyInfTestsEntityObjectInfo[eTests_XCT_LineTest],     tTestsXCT_LineTestEntityLeaf,     NULL},
#endif
    {NULL,                                                    NULL,                             NULL}
};


//******* VoiceService.{i}.PhyInterface.{i}. EntityLEAF*******
struct CWMP_OP tPhyInterfaceEntityLeafOP = { getPhyInterfaceEntity, setPhyInterfaceEntity};	

struct CWMP_PRMT tPhyInterfaceEntityLeafInfo[] =
{
    /*(name,          type,           flag,      op)*/
	{ "InterfaceID",  eCWMP_tUINT,  CWMP_READ, &tPhyInterfaceEntityLeafOP},
	{ "PhyPort",  eCWMP_tSTRING,  CWMP_READ, &tPhyInterfaceEntityLeafOP},
};

enum ePhyInterfaceEntityLeaf
{
    ePhyInfInterfaceID,
	ePhyPort,
};

struct CWMP_LEAF tPhyInterfaceEntityLeaf[] =
{
    { &tPhyInterfaceEntityLeafInfo[ePhyInfInterfaceID]  },
    { &tPhyInterfaceEntityLeafInfo[ePhyPort]  },		
    { NULL }
};

//******* VoiceService.{i}.PhyInterface.{i} EntityObject*******
struct CWMP_PRMT tPhyInterfaceEntityObjectInfo[] =
{
    /*(name,           type,           flag,       op)*/
    {"Tests",          eCWMP_tOBJECT,  CWMP_READ,  NULL},
#ifdef CONFIG_E8B
    {"X_"CTCOM_NAME_PREFIX"_Stats",     eCWMP_tOBJECT,  CWMP_READ,  NULL},
#else
    {"X_Stats",                         eCWMP_tOBJECT,  CWMP_READ,  NULL},
#endif
    {"X_"CTCOM_NAME_PREFIX"_Extend",    eCWMP_tOBJECT,  CWMP_READ,  NULL}
};
enum ePhyInterfaceEntityObject
{
	ePhyInfTests,
	ePhyInfXCT_Stats,
    ePhyInfXCT_Extend
};
struct CWMP_NODE tPhyInterfaceEntityObject[] =
{
    /*info,                                            leaf,                   next)*/
    {&tPhyInterfaceEntityObjectInfo[ePhyInfTests],     tPhyInfTestsEntityLeaf, tPhyInfTestsEntityObject},
 /* E8C : dsp not support it */
#ifdef CONFIG_E8B
    {&tPhyInterfaceEntityObjectInfo[ePhyInfXCT_Stats], tXCT_StatusEntityLeaf,                   tXCT_StatusEntityObject},
#endif
#ifdef CONFIG_CU
    {&tPhyInterfaceEntityObjectInfo[ePhyInfXCT_Extend], tPhyInfXCT_ExtendEntityLeaf,    NULL},
#endif
    {NULL,                                             NULL,                   NULL}
};


//******* VoiceService.{i}.PhyInterface.{i}  *******
struct CWMP_PRMT tPhyInterfaceObjectInfo[] =
{
    /*(name,  type,           flag,		               op)*/
    {"0",     eCWMP_tOBJECT,  CWMP_READ|CWMP_LNKLIST,  NULL}
};
enum ePhyInterfaceObject
{
    ePhyInterface0
};
struct CWMP_LINKNODE tPhyInterfaceObject[] =
{
    /*info,  			                        leaf,                     next,                       sibling,  instnum)*/
    {&tPhyInterfaceObjectInfo[ePhyInterface0],  tPhyInterfaceEntityLeaf,  tPhyInterfaceEntityObject,  NULL,     0}
};

int objPhyInterface(char *name, struct CWMP_LEAF *e, int type, void *data)
{
    struct CWMP_NODE *entity=(struct CWMP_NODE *)e;

	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		struct CWMP_LINKNODE **c = (struct CWMP_LINKNODE **)data;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) 
            return -1;

		if( create_Object( c, tPhyInterfaceObject, sizeof(tPhyInterfaceObject), MAX_PHYINTERFACE_COUNT, 1) < 0 )
				return -1;
        
        add_objectNum( name, MAX_PHYINTERFACE_COUNT );
		return 0;
	}
    	break;
	case eCWMP_tADDOBJ:
	case eCWMP_tDELOBJ:
         {
            CWMPDBG( 0, ( stderr, "<%s:%d>Warning type %d unsupport now \n", __FUNCTION__, __LINE__, type) );
            return 0;
         }
    	break;
    case eCWMP_tUPDATEOBJ:
         {
            int num=0,i;
            struct CWMP_LINKNODE *old_table;
            num = MAX_PHYINTERFACE_COUNT;
            old_table = (struct CWMP_LINKNODE*)entity->next;
            entity->next= NULL;
            for( i=1; i<=num;i++ ) {
                struct CWMP_LINKNODE *remove_entity=NULL;

                remove_entity = remove_SiblingEntity( &old_table, i );
                if( remove_entity!=NULL )
                {
                	add_SiblingEntity( (struct CWMP_LINKNODE**)&entity->next, remove_entity );
                }else{ 
    
                    add_Object( name, (struct CWMP_LINKNODE **)&entity->next,
                        tPhyInterfaceObject, sizeof(tPhyInterfaceObject), (unsigned int *)&i );
                }
           }
           if( old_table )  destroy_ParameterTable( (struct CWMP_NODE *)old_table );           
           return 0;
        }
	   	break; 
    default:
        CWMPDBG( 0, ( stderr, "<%s:%d>Error: unknown type %d \n", __FUNCTION__, __LINE__, type) );
        break;
	}
	return -1;
}

int getPhyInterfaceEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.InterfaceID */
	unsigned int nPhyInterfaceInstNum;
	union {
        unsigned int InterfaceID;
		char PhyPort[2];
        //more here
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "InterfaceID" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__INTERFACEID, nPhyInterfaceInstNum, &s.InterfaceID );
		*data = uintdup( s.InterfaceID );
	} else if ( strcmp( pszLastname, "PhyPort" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__PHYPORT, nPhyInterfaceInstNum, &s.PhyPort );
		*data = strdup( s.PhyPort );
	} else {
		return ERR_9005;
	}

    return 0;
}


int setPhyInterfaceEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.InterfaceID */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "InterfaceID" ) == 0 ) {
        res = mib_set_type1( MIB_PHYINTERFACE__INTERFACEID, nPhyInterfaceInstNum, data );
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getXCT_StatusEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Stats. */
	unsigned int nPhyInterfaceInstNum;
#if 0
	extern int gVoipReq;
	union {
        boolean ResetStatistics;
	} s;
#endif
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;
	
	
    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "ResetStatistics" ) == 0 ) {
		*data = booldup( FALSE ); //alway return FALSE
	} else {
		return ERR_9005;
	}

    return 0;
}

int setXCT_StatusEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Stats. */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	const char *pszLastname = entity->info->name;
    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;    

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 0, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );

	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}
    
	if( strcmp( pszLastname, "ResetStatistics" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__X_STATS__RESETSTATISTICS, nPhyInterfaceInstNum, data );
	} else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getXCT_ExtendEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Extend. */
	unsigned int nPhyInterfaceInstNum;
	extern int gVoipReq;
	union {
        char DialMode[32];
        unsigned int HookFlashUpTime;
        unsigned int HookFlashDownTime;
        boolean ReversePolePulse;
        char KcType[32];
        char ClipTransWhen[32];
        boolean ClipReversePole;
        boolean MWIRingFlag;
        char ClipFormat[32];
        unsigned int RingDCVoltageOverlapped;
        unsigned int DataServiceMode;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;
		
    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );

    if( !gVoipReq ) {
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }
    	
	fprintf(stderr, "EVT_VOICEPROFILE_LINE_GET_STATUS %s\n", pszLastname);
    
    if( strcmp( pszLastname, "DialMode" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__DIALMODE, nPhyInterfaceInstNum, &s.DialMode );
		*data = strdup( s.DialMode );
    } else if( strcmp( pszLastname, "HookFlashUpTime" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHUPTIME, nPhyInterfaceInstNum, &s.HookFlashUpTime );
		*data = uintdup( s.HookFlashUpTime );
    } else if( strcmp( pszLastname, "HookFlashDownTime" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHDOWNTIME, nPhyInterfaceInstNum, &s.HookFlashDownTime );
		*data = uintdup( s.HookFlashDownTime );
    } else if( strcmp( pszLastname, "ReversePolePulse" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__REVERSEPOLEPULSE, nPhyInterfaceInstNum, &s.ReversePolePulse );
		*data = booldup( s.ReversePolePulse );
    } else if( strcmp( pszLastname, "KcType" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__KCTYPE, nPhyInterfaceInstNum, &s.KcType );
		*data = strdup( s.KcType );
    } else if( strcmp( pszLastname, "ClipTransWhen" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPTRANSWHEN, nPhyInterfaceInstNum, &s.ClipTransWhen );
		*data = strdup( s.ClipTransWhen );
    } else if( strcmp( pszLastname, "ClipReversePole" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPREVERSEPOLE, nPhyInterfaceInstNum, &s.ClipReversePole );
		*data = booldup( s.ClipReversePole );
    } else if( strcmp( pszLastname, "MWIRingFlag" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__MWIRINGFLAG, nPhyInterfaceInstNum, &s.MWIRingFlag );
		*data = booldup( s.MWIRingFlag );
    } else if( strcmp( pszLastname, "ClipFormat" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPFORMAT, nPhyInterfaceInstNum, &s.ClipFormat );
		*data = strdup( s.ClipFormat );
    } else if( strcmp( pszLastname, "RingDCVoltageOverlapped" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__RINGDCVOLTAGEOVERLAPPED, nPhyInterfaceInstNum, &s.RingDCVoltageOverlapped );
		*data = uintdup( s.RingDCVoltageOverlapped );
    } else if( strcmp( pszLastname, "DataServiceMode" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__XCT_EXTEND__DATASERVICEMODE, nPhyInterfaceInstNum, &s.DataServiceMode );
		*data = uintdup( s.DataServiceMode );
    } else {
		return ERR_9005;
	}

    return 0;
}

int setXCT_ExtendEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.X_"CTCOM_NAME_PREFIX"_Extend. */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	const char *pszLastname = entity->info->name;
    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;    

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 0, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );

	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}

    if( strcmp( pszLastname, "DialMode" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__DIALMODE, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "HookFlashUpTime" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHUPTIME, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "HookFlashDownTime" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__HOOKFLASHDOWNTIME, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "ReversePolePulse" ) == 0 ) {
        mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__REVERSEPOLEPULSE, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "KcType" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__KCTYPE, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "ClipTransWhen" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPTRANSWHEN, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "ClipReversePole" ) == 0 ) {
        mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPREVERSEPOLE, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "MWIRingFlag" ) == 0 ) {
        mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__MWIRINGFLAG, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "ClipFormat" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__CLIPFORMAT, nPhyInterfaceInstNum, data );
     } else if( strcmp( pszLastname, "RingDCVoltageOverlapped" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__RINGDCVOLTAGEOVERLAPPED, nPhyInterfaceInstNum, data );
    } else if( strcmp( pszLastname, "DataServiceMode" ) == 0 ) {
		mib_set_type1( MIB_PHYINTERFACE__XCT_EXTEND__DATASERVICEMODE, nPhyInterfaceInstNum, data );
	} else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getPhyInfTestsEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests. */
	unsigned int nPhyInterfaceInstNum;
	extern int gVoipReq;
	union {
        char TestState[16];
        char TestSelector[25];
        boolean PhoneConnectivity;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;
	
	if(!gVoipReq){
		cwmpSendRequestToSolar();
		gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
   	 }
	
    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "TestState" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__TESTSTATE, nPhyInterfaceInstNum, &s.TestState );
		*data = strdup( s.TestState );
	} else if( strcmp( pszLastname, "TestSelector" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__TESTSELECTOR, nPhyInterfaceInstNum, &s.TestSelector );
		*data = strdup( s.TestSelector );
	} else if( strcmp( pszLastname, "PhoneConnectivity" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__PHONECONNECTIVITY, nPhyInterfaceInstNum, &s.PhoneConnectivity );
		*data = booldup( s.PhoneConnectivity );
		gVoipReq = EVT_VOICEPROFILE_LINE_NONE;
	} else {
		return ERR_9005;
	}

    return 0;
}

int setPhyInfTestsEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests. */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	const char *pszLastname = entity->info->name;
    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;    

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 0, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );

	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}
    
	if( strcmp( pszLastname, "TestState" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__TESTSTATE, nPhyInterfaceInstNum, data );
	} else if( strcmp( pszLastname, "TestSelector" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__TESTSELECTOR, nPhyInterfaceInstNum, data );
	} else if( strcmp( pszLastname, "PhoneConnectivity" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getXCTSimulateTestEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests.X_SimulateTest. */
	unsigned int nPhyInterfaceInstNum;
	union {
        char TestType[16];
        char CalledNumber[40];
        unsigned int DailDTMFConfirmEnable;
        char DailDTMFConfirmNumber[40];
        char DailDTMFConfirmResult[16];
        char Status[16];
        char Conclusion[16];
        char CallerFailReason[16];
        char CalledFailReason[16];
        unsigned int FailedResponseCode;
	} s;
    extern int gVoipReq;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
	


	if(!gVoipReq){
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }	
	fprintf(stderr,"EVT_VOICEPROFILE_LINE_GET_STATUS %s\n",pszLastname);
	if( strcmp( pszLastname, "TestType" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__TESTTYPE,
            nPhyInterfaceInstNum, &s.TestType);
		*data = strdup( s.TestType );
	} else if( strcmp( pszLastname, "CalledNumber" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDNUMBER,
            nPhyInterfaceInstNum, &s.CalledNumber );
		*data = strdup( s.CalledNumber );
	} else if( strcmp( pszLastname, "DailDTMFConfirmEnable" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMENABLE,
            nPhyInterfaceInstNum, &s.DailDTMFConfirmEnable );
		*data = uintdup( s.DailDTMFConfirmEnable );
	} else if( strcmp( pszLastname, "DailDTMFConfirmNumber" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMNUMBER,
            nPhyInterfaceInstNum, &s.DailDTMFConfirmNumber );
		*data = strdup( s.DailDTMFConfirmNumber );
	} else if( strcmp( pszLastname, "DailDTMFConfirmResult" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMRESULT,
            nPhyInterfaceInstNum, &s.DailDTMFConfirmResult );
		*data = strdup( s.DailDTMFConfirmResult );
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__STATUS,
            nPhyInterfaceInstNum, &s.Status );
		*data = strdup( s.Status );
	} else if( strcmp( pszLastname, "Conclusion" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CONCLUSION,
            nPhyInterfaceInstNum, &s.Conclusion );
		*data = strdup( s.Conclusion );
	} else if( strcmp( pszLastname, "CallerFailReason" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLERFAILREASON,
            nPhyInterfaceInstNum, &s.CallerFailReason );
		*data = strdup( s.CallerFailReason );
	} else if( strcmp( pszLastname, "CalledFailReason" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDFAILREASON, 
            nPhyInterfaceInstNum, &s.CalledFailReason );
		*data = strdup( s.CalledFailReason );
	} else if( strcmp( pszLastname, "FailedResponseCode" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__FAILEDRESPONSECODE, 
            nPhyInterfaceInstNum, &s.FailedResponseCode );
		*data = uintdup( s.FailedResponseCode );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setXCTSimulateTestEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests.X_SimulateTest. */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	const char *pszLastname = entity->info->name;
    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
    }

    CWMPDBG( 0, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );
    
    if( strcmp( pszLastname, "TestType" ) == 0 ) {
        res = mib_set_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__TESTTYPE,
            nPhyInterfaceInstNum, data );
	} else if( strcmp( pszLastname, "CalledNumber" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__CALLEDNUMBER,
            nPhyInterfaceInstNum, data );
	} else if( strcmp( pszLastname, "DailDTMFConfirmEnable" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMENABLE,
            nPhyInterfaceInstNum, data );
	} else if( strcmp( pszLastname, "DailDTMFConfirmNumber" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__XCT_SIMULATETEST__DAILDTMFCONFIRMNUMBER,
                nPhyInterfaceInstNum, data );
	
	} else if( strcmp( pszLastname, "DailDTMFConfirmResult" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else if( strcmp( pszLastname, "Conclusion" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else if( strcmp( pszLastname, "CallerFailReason" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else if( strcmp( pszLastname, "CalledFailReason" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else if( strcmp( pszLastname, "FailedResponseCode" ) == 0 ) {
		return ERR_9008;	/* Read Only*/
	} else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getXCTLineTestEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests.X_LineTest. */
	unsigned int nPhyInterfaceInstNum;
	union {
        boolean IsTestOnBusy;
        char Conclusion[16];
        int AGACVoltage;
        int BGACVoltage;
        int ABACVoltage;
        int AGDCVoltage;
        int BGDCVoltage;
        int ABDCVoltage;
        unsigned int AGInsulationResistance;
        unsigned int BGInsulationResistance;
        unsigned int ABInsulationResistance;
        unsigned int AGCapacitance;
        unsigned int BGCapacitance;
        unsigned int ABCapacitance;
	} s;
    extern int gVoipReq;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
	
	if( !gVoipReq ) {
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }
    	
	fprintf(stderr,"EVT_VOICEPROFILE_LINE_GET_STATUS %s\n",pszLastname);

	if( strcmp( pszLastname, "IsTestOnBusy" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ISTESTONBUSY, nPhyInterfaceInstNum, &s.IsTestOnBusy);
		*data = booldup( s.IsTestOnBusy );
	} else if( strcmp( pszLastname, "Conclusion" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__CONCLUSION, nPhyInterfaceInstNum, &s.Conclusion );
		*data = strdup( s.Conclusion );
	} else if( strcmp( pszLastname, "AGACVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGACVOLTAGE, nPhyInterfaceInstNum, &s.AGACVoltage );
		*data = intdup( s.AGACVoltage ); 
    } else if( strcmp( pszLastname, "BGACVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGACVOLTAGE, nPhyInterfaceInstNum, &s.BGACVoltage );
		*data = intdup( s.BGACVoltage );
    } else if( strcmp( pszLastname, "ABACVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABACVOLTAGE, nPhyInterfaceInstNum, &s.ABACVoltage );
		*data = intdup( s.ABACVoltage );
    } else if( strcmp( pszLastname, "AGDCVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGDCVOLTAGE, nPhyInterfaceInstNum, &s.AGDCVoltage );
		*data = intdup( s.AGDCVoltage );
    } else if( strcmp( pszLastname, "BGDCVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGDCVOLTAGE, nPhyInterfaceInstNum, &s.BGDCVoltage );
		*data = intdup( s.BGDCVoltage );
    } else if( strcmp( pszLastname, "ABDCVoltage" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABDCVOLTAGE, nPhyInterfaceInstNum, &s.ABDCVoltage );
		*data = intdup( s.ABDCVoltage );
    } else if( strcmp( pszLastname, "AGInsulationResistance" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGINSULATIONRESISTANCE, nPhyInterfaceInstNum, &s.AGInsulationResistance );
		*data = uintdup( s.AGInsulationResistance );
    } else if( strcmp( pszLastname, "BGInsulationResistance" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGINSULATIONRESISTANCE, nPhyInterfaceInstNum, &s.BGInsulationResistance );
		*data = uintdup( s.BGInsulationResistance );
    } else if( strcmp( pszLastname, "ABInsulationResistance" ) == 0 ) {
		mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABINSULATIONRESISTANCE, nPhyInterfaceInstNum, &s.ABInsulationResistance );
		*data = uintdup( s.ABInsulationResistance );
    } else if( strcmp( pszLastname, "AGCapacitance" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__AGCAPACITANCE, nPhyInterfaceInstNum, &s.AGCapacitance );
        *data = uintdup( s.AGCapacitance );
    } else if( strcmp( pszLastname, "BGCapacitance" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__BGCAPACITANCE, nPhyInterfaceInstNum, &s.BGCapacitance );
        *data = uintdup( s.BGCapacitance );
    } else if( strcmp( pszLastname, "ABCapacitance" ) == 0 ) {
        mib_get_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ABCAPACITANCE, nPhyInterfaceInstNum, &s.ABCapacitance );
        *data = uintdup( s.ABCapacitance );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setXCTLineTestEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.PhyInterface.{i}.Tests.X_LineTest. */
    int res = 0;
	unsigned int nPhyInterfaceInstNum;
	const char *pszLastname = entity->info->name;
    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_PhyInterface( name, &nPhyInterfaceInstNum ) )
		return -1;
	
	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT)	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

	if( gVoipReq != EVT_VOICEPROFILE_LINE_SET_STATUS ) {
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
    }

    CWMPDBG( 0, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );
    
    if( strcmp( pszLastname, "IsTestOnBusy" ) == 0 ) {
		res = mib_set_type1( MIB_PHYINTERFACE__TESTS__XCT_LINETEST__ISTESTONBUSY, nPhyInterfaceInstNum, data );
	} else if( ( strcmp( pszLastname, "Conclusion" ) == 0 ) ||  
               ( strcmp( pszLastname, "AGACVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "BGACVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "ABACVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "AGDCVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "BGDCVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "ABDCVoltage" ) == 0 ) ||
               ( strcmp( pszLastname, "AGInsulationResistance" ) == 0 ) ||
               ( strcmp( pszLastname, "BGInsulationResistance" ) == 0 ) ||
               ( strcmp( pszLastname, "ABInsulationResistance" ) == 0 ) ||
               ( strcmp( pszLastname, "AGCapacitance" ) == 0 ) ||
               ( strcmp( pszLastname, "BGCapacitance" ) == 0 ) ||
               ( strcmp( pszLastname, "ABCapacitance" ) == 0 ) 
             ) {
		return ERR_9008;	/* Read Only*/
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int objPhyPoorQualityList(char *name, struct CWMP_LEAF *e, int type, void *data)
{
    struct CWMP_NODE *entity=(struct CWMP_NODE *)e;

	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		int num=0,i;
		struct CWMP_LINKNODE **c = (struct CWMP_LINKNODE **)data;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) 
            return -1;

		num = MAX_POORQUALITYLIST_COUNT; /* Now, we has one voice profile only. */
        
		for( i=1; i<=num;i++ )
		{
			if( create_Object( c, tXCT_STPoorQualityListObject, sizeof(tXCT_STPoorQualityListObject), 1, MAX_PROFILE_COUNT) < 0 )
				return -1;
		}
        add_objectNum( name, num );
		return 0;
	}
    	break;
	case eCWMP_tADDOBJ:
	case eCWMP_tDELOBJ:
         {
            CWMPDBG( 0, ( stderr, "<%s:%d>Warning: type %d unsupport now \n", __FUNCTION__, __LINE__, type) );
            return 0;
         }
    	break;
    case eCWMP_tUPDATEOBJ:
         {
            int num=0,i;
            struct CWMP_LINKNODE *old_table;
            num = MAX_POORQUALITYLIST_COUNT;
            old_table = (struct CWMP_LINKNODE*)entity->next;
            entity->next= NULL;
            for( i=1; i<=num;i++ ) {
                struct CWMP_LINKNODE *remove_entity=NULL;
		  unsigned int MaxInstNum=i;
                remove_entity = remove_SiblingEntity( &old_table, i );
                if( remove_entity!=NULL )
                {
                	add_SiblingEntity( (struct CWMP_LINKNODE**)&entity->next, remove_entity );
                }else{ 
    
                    add_Object( name, (struct CWMP_LINKNODE **)&entity->next,
                        tXCT_STPoorQualityListObject, sizeof(tXCT_STPoorQualityListObject), (unsigned int *)&MaxInstNum );
                }
           }
           if( old_table )  destroy_ParameterTable( (struct CWMP_NODE *)old_table );           
           return 0;
        }
	   	break;
    default:
	    CWMPDBG( 0, ( stderr, "<%s:%d>Error: unknown type %d \n", __FUNCTION__, __LINE__, type) );
        break;       
	}
	return -1;
}

int getXCT_STPoorQualityListEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.PhyInterface.{i}.X_Stats.PoorQualityList.{i}. */
	unsigned int nPhyInterfaceInstNum;
    unsigned int nPoorQualityListInstNum;
    unsigned int nNumberOfList;
	union {
        time_t StatTime;
        unsigned int TxPackets;
        unsigned int RxPackets;
        unsigned int MeanDelay;
        unsigned int MeanJitter;
        unsigned int FractionLoss;
        char LocalIPAddress[16];
        unsigned int LocalUDPPort;
        char FarEndIPAddress[16];
        unsigned int FarEndUDPPort;
        unsigned int MosLq;
        char Codec[16];
	} s;

	const char *pszLastname = entity->info->name;

    extern int gVoipReq;
    extern cwmpEvtMsg pEvtMsg;

	if(!gVoipReq){
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;

	if( !GetOneBasedInstanceNumber_PhyInterface_PoorQualityList( name, 
        &nPhyInterfaceInstNum, &nPoorQualityListInstNum ) )
		return -1;

	if( -- nPhyInterfaceInstNum >= MAX_PHYINTERFACE_COUNT )	/* convert to zero based */
		return -1;

	if( !mib_get_type1( MIB_PHYINTERFACE__INTERFACEID, nPhyInterfaceInstNum, &nNumberOfList ) )
    	return -1;

	if( -- nPoorQualityListInstNum >= MAX_POORQUALITYLIST_COUNT )   /* convert to zero based */
   		return -1;

	*type = entity->info->type;	
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "StatTime" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__STATTIME,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.StatTime );
		*data = timedup( s.StatTime );
	} else if( strcmp( pszLastname, "TxPackets" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__TXPACKETS,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.TxPackets );
		*data = uintdup( s.TxPackets );
	} else if( strcmp( pszLastname, "RxPackets" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__RXPACKETS,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.RxPackets );
		*data = uintdup( s.RxPackets );
	} else if( strcmp( pszLastname, "MeanDelay" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANDELAY,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.MeanDelay );
		*data = uintdup( s.MeanDelay );
	} else if( strcmp( pszLastname, "MeanJitter" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MEANJITTER,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.MeanJitter );
		*data = uintdup( s.MeanJitter );
	} else if( strcmp( pszLastname, "FractionLoss" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FRACTIONLOSS,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.FractionLoss );
		*data = uintdup( s.FractionLoss );
	} else if( strcmp( pszLastname, "LocalIPAddress" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALIPADDRESS,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.LocalIPAddress );
		*data = strdup( s.LocalIPAddress );
	} else if( strcmp( pszLastname, "LocalUDPPort" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__LOCALUDPPORT,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.LocalUDPPort );
		*data = uintdup( s.LocalUDPPort );
	} else if( strcmp( pszLastname, "FarEndIPAddress" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDIPADDRESS,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.FarEndIPAddress );
		*data = strdup( s.FarEndIPAddress );
	} else if( strcmp( pszLastname, "FarEndUDPPort" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__FARENDUDPPORT,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.FarEndUDPPort );
		*data = uintdup( s.FarEndUDPPort );
	} else if( strcmp( pszLastname, "MosLq" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__MOSLQ,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.MosLq );
		*data = uintdup( s.MosLq );
	} else if( strcmp( pszLastname, "Codec" ) == 0 ) {
		mib_get_type2( MIB_PHYINTERFACE__XCT_STATUS__POORQUALITYLIST__CODEC,
            nPhyInterfaceInstNum, nPoorQualityListInstNum, &s.Codec );
		*data = strdup( s.Codec );
	} else {
		return ERR_9005;
	}
    
    return 0;
}

