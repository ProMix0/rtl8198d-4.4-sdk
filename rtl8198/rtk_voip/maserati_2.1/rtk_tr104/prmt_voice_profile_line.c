#include "voip_manager.h"
#include "prmt_limit.h"
#include "prmt_voice_profile_line.h"
#include "prmt_line_codec_list.h"
#include "prmt_capabilities.h"	/* for lstCodecs */
#include "mib_def.h"
#include "mib_tr104.h"
#include "str_mib.h"
#include "cwmp_main_tr104.h"
#include "str_utility.h"
#include "cwmpevt.h"

//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.SIP. EntityLEAF*******
struct CWMP_OP tVPLineSIPEntityLeafOP = { getLineSipEntity, setLineSipEntity};

struct CWMP_PRMT tVPLineSIPEntityLeafInfo[] =
{
    /*(name,           type,           flag,                  op)*/
	{ "AuthUserName",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP},
	{ "AuthPassword",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP},
	{ "URI",           eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-AuthUserName",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-AuthPassword",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-URI",           eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineSIPEntityLeafOP}	
};

enum eVPLineSIPEntityLeaf
{
    eSIPAuthUserName,
    eSIPAuthPassword,
    eSIPURI,
    eSIP_XCT_AuthUserName,
    eSIP_XCT_AuthPassword,
    eSIP_XCT_URI    
};

struct CWMP_LEAF tVPLineSIPEntityLeaf[] =
{
    { &tVPLineSIPEntityLeafInfo[eSIPAuthUserName]  },
    { &tVPLineSIPEntityLeafInfo[eSIPAuthPassword]  },
    { &tVPLineSIPEntityLeafInfo[eSIPURI]  },
    { &tVPLineSIPEntityLeafInfo[eSIP_XCT_AuthUserName]  },
    { &tVPLineSIPEntityLeafInfo[eSIP_XCT_AuthPassword]  },
    { &tVPLineSIPEntityLeafInfo[eSIP_XCT_URI]  },    
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Stats. EntityLEAF*******
struct CWMP_OP tVPLineStatsEntityLeafOP = { getLineStatsEntity, setLineStatsEntity};

struct CWMP_PRMT tVPLineStatsEntityLeafInfo[] =
{
    /*(name,                     type,         flag,                  op)*/
    { "PacketsSent",             eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "PacketsReceived",         eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "BytesSent",               eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "BytesReceived",           eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "PacketsLost",             eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "IncomingCallsReceived",   eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "IncomingCallsAnswered",   eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "IncomingCallsConnected",  eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "IncomingCallsFailed",     eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "OutgoingCallsAttempted",  eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "OutgoingCallsAnswered",   eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "OutgoingCallsConnected",  eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "OutgoingCallsFailed",     eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "CallsDropped",     		eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "ResetStatistics",         eCWMP_tUINT,  CWMP_READ|CWMP_WRITE,  &tVPLineStatsEntityLeafOP},
    { "TotalCallTime",           eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "ReceivePacketLossRate",          eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "FarEndPacketLossRate",           eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "ReceiveInterarrivalJitter",      eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "FarEndInterarrivalJitter",       eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "RoundTripDelay",                 eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
    { "ServerDownTime",                 eCWMP_tUINT,  CWMP_READ,             &tVPLineStatsEntityLeafOP},
};

enum eVPLineStatsEntityLeaf
{
    eStatsPacketsSent,
    eStatsPacketsReceived,
    eStatsBytesSent,
    eStatsBytesReceived,
    eStatsPacketsLost,
    eStatsIncomingCallsReceived,
    eStatsIncomingCallsAnswered,
    eStatsIncomingCallsConnected,
    eStatsIncomingCallsFailed,
    eStatsOutgoingCallsAttempted,
    eStatsOutgoingCallsAnswered,
    eStatsOutgoingCallsConnected,
    eStatsOutgoingCallsFailed,
     eStatsCallsDropped,
	eResetStatistics,
	eTotalCallTime,
    eStatsReceivePacketLossRate,
    eStatsFarEndPacketLossRate,
    eStatsReceiveInterarrivalJitter,
    eStatsFarEndInterarrivalJitter,
    eStatsRoundTripDelay,
    eStatsServerDownTime,
};

struct CWMP_LEAF tVPLineStatsEntityLeaf[] =
{
    { &tVPLineStatsEntityLeafInfo[eStatsPacketsSent]  },
    { &tVPLineStatsEntityLeafInfo[eStatsPacketsReceived]  },
    { &tVPLineStatsEntityLeafInfo[eStatsBytesSent]  },
    { &tVPLineStatsEntityLeafInfo[eStatsBytesReceived]  },
    { &tVPLineStatsEntityLeafInfo[eStatsPacketsLost]  },
    { &tVPLineStatsEntityLeafInfo[eStatsIncomingCallsReceived]  },
    { &tVPLineStatsEntityLeafInfo[eStatsIncomingCallsAnswered]  },
    { &tVPLineStatsEntityLeafInfo[eStatsIncomingCallsConnected]  },
    { &tVPLineStatsEntityLeafInfo[eStatsIncomingCallsFailed]  },
    { &tVPLineStatsEntityLeafInfo[eStatsOutgoingCallsAttempted]  },
    { &tVPLineStatsEntityLeafInfo[eStatsOutgoingCallsAnswered]  },
    { &tVPLineStatsEntityLeafInfo[eStatsOutgoingCallsConnected]  },
    { &tVPLineStatsEntityLeafInfo[eStatsOutgoingCallsFailed]  },
        { &tVPLineStatsEntityLeafInfo[eStatsCallsDropped]  },
	{ &tVPLineStatsEntityLeafInfo[eResetStatistics]  },
    { &tVPLineStatsEntityLeafInfo[eTotalCallTime]  },
    { &tVPLineStatsEntityLeafInfo[eStatsReceivePacketLossRate]  },
    { &tVPLineStatsEntityLeafInfo[eStatsFarEndPacketLossRate]  },
    { &tVPLineStatsEntityLeafInfo[eStatsReceiveInterarrivalJitter]  },
    { &tVPLineStatsEntityLeafInfo[eStatsFarEndInterarrivalJitter]  },
    { &tVPLineStatsEntityLeafInfo[eStatsRoundTripDelay]  },
    { &tVPLineStatsEntityLeafInfo[eStatsServerDownTime]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec. EntityObject*******
struct CWMP_OP tVPLineCodecOP = { NULL, objLineCodecList};

struct CWMP_PRMT tVPLineCodecEntityObjectInfo[] =
{
    /*(name,    type,           flag,       op)*/
	{ "List",   eCWMP_tOBJECT,  CWMP_READ,  &tVPLineCodecOP}
};
enum eVPLineCodecEntityObject
{
	eCodecList
};
struct CWMP_NODE tVPLineCodecEntityObject[] =
{
    /*info,                                      leaf,  next)*/
    {&tVPLineCodecEntityObjectInfo[eCodecList],  NULL,  NULL},
    {NULL,                                       NULL,  NULL}
};


//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec. EntityLEAF *******

struct CWMP_OP tVPLineCodecEntityLeafOP = { getLineCodecEntity, setLineCodecEntity};

struct CWMP_PRMT tVPLineCodecEntityLeafInfo[] =
{
    /*(name,                     type,         flag,                  op)*/
    { "TransmitCodec",        eCWMP_tSTRING,  CWMP_READ,             &tVPLineCodecEntityLeafOP},
    { "ReceiveCodec",         eCWMP_tSTRING,  CWMP_READ,             &tVPLineCodecEntityLeafOP},
  
};

enum eVPLineCodecEntityLeaf
{
    eCodecTransmitCodec,
    eCodecReceiveCodec,
 
};

struct CWMP_LEAF tVPLineCodecEntityLeaf[] =
{
    { &tVPLineCodecEntityLeafInfo[eCodecTransmitCodec]  },
    { &tVPLineCodecEntityLeafInfo[eCodecReceiveCodec]  },
     { NULL }
};



//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.VoiceProcessing EntityLEAF*******
struct CWMP_OP tVPLineVoiceProcessingEntityLeafOP = 
  { getLineVoiceProcessingEntity, setLineVoiceProcessingEntity};

struct CWMP_PRMT tVPLineVoiceProcessingEntityLeafInfo[] =
{
    /*(name,                    type,           flag,                  op)*/
	{ "TransmitGain",           eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPLineVoiceProcessingEntityLeafOP},
	{ "ReceiveGain",            eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPLineVoiceProcessingEntityLeafOP},
	{ "EchoCancellationEnable", eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPLineVoiceProcessingEntityLeafOP},
	{ "EchoCancellationInUse",  eCWMP_tBOOLEAN, CWMP_READ,             &tVPLineVoiceProcessingEntityLeafOP},
    { "EchoCancellationTail",   eCWMP_tUINT,    CWMP_READ,             &tVPLineVoiceProcessingEntityLeafOP}
};

enum eVPLineVoiceProcessingEntityLeaf
{
    eVoiceProcessingTransmitGain,
    eVoiceProcessingReceiveGain,
    eVoiceProcessingEchoCancellationEnable,
    eVoiceProcessingEchoCancellationInUse,
    eVoiceProcessingEchoCancellationTail
};

struct CWMP_LEAF tVPLineVoiceProcessingEntityLeaf[] =
{
    { &tVPLineVoiceProcessingEntityLeafInfo[eVoiceProcessingTransmitGain]  },
    { &tVPLineVoiceProcessingEntityLeafInfo[eVoiceProcessingReceiveGain]  },
    { &tVPLineVoiceProcessingEntityLeafInfo[eVoiceProcessingEchoCancellationEnable]  },
    { &tVPLineVoiceProcessingEntityLeafInfo[eVoiceProcessingEchoCancellationInUse]  },
    { &tVPLineVoiceProcessingEntityLeafInfo[eVoiceProcessingEchoCancellationTail]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.X_"CTCOM_NAME_PREFIX"_IMS EntityLEAF*******
struct CWMP_OP tVPLineXCT_IMSEntityLeafOP = { getLineXCT_IMSEntity, setLineXCT_IMSEntity};

struct CWMP_PRMT tVPLineXCT_IMSEntityLeafInfo[] =
{
    /*(name,                    type,        flag,                 op)*/
	{ "dial-tone-pattern",    eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP},
	{ "mcid-service",         eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP},
	{ "no-dialing-behaviour", eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP},
	{ "hold-service",         eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP},
	{ "three-party-service",  eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP},
	{ "conference-uri",       eCWMP_tSTRING, CWMP_READ|CWMP_WRITE, &tVPLineXCT_IMSEntityLeafOP},
	{ "hotline-uri",          eCWMP_tSTRING, CWMP_READ|CWMP_WRITE, &tVPLineXCT_IMSEntityLeafOP},
	{ "cw-service",           eCWMP_tUINT,   CWMP_READ,            &tVPLineXCT_IMSEntityLeafOP}
};

enum eVPLineXCT_IMSEntityLeaf
{
    eXCT_IMSdialTonePattern,
    eXCT_IMSmcidService,
    eXCT_IMSnoDialingBehaviour,
    eXCT_IMSholdService,
    eXCT_IMSthreePartyService,
    eXCT_IMSconferenceUri,
    eXCT_IMShotlineUri,
    eXCT_IMScwService
};

struct CWMP_LEAF tVPLineXCT_IMSEntityLeaf[] =
{
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSdialTonePattern]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSmcidService]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSnoDialingBehaviour]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSholdService]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSthreePartyService]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMSconferenceUri]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMShotlineUri]  },
    { &tVPLineXCT_IMSEntityLeafInfo[eXCT_IMScwService]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.X_"CTCOM_NAME_PREFIX"_LastRegisterError EntityLEAF*******
#ifdef CONFIG_E8B
struct CWMP_OP tVPLineXCTCOM_LastRegisterErrorEntityLeafOP = { getLineXCTCOM_LastRegisterErrorEntity, setLineXCTCOM_LastRegisterErrorEntity};

struct CWMP_PRMT tVPLineXCTCOM_LastRegisterErrorEntityLeafInfo[] =
{
    /*(name,                    type,        flag,                 op)*/
	{ "Reason",          eCWMP_tSTRING, CWMP_READ|CWMP_WRITE, &tVPLineXCTCOM_LastRegisterErrorEntityLeafOP},
};

enum eVPLineXCTCOM_LastRegisterErrorEntityLeaf
{
    eXCTCOM_LastRegisterErrorReason
};

struct CWMP_LEAF tVPLineXCTCOM_LastRegisterErrorEntityLeaf[] =
{
    { &tVPLineXCTCOM_LastRegisterErrorEntityLeafInfo[eXCTCOM_LastRegisterErrorReason]  },
    { NULL }
};
#endif


//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.CallingFeatures EntityLEAF*******
 
struct CWMP_OP tVPLineCallingFeaturesEntityLeafOP = 
  { getLineCallingFeaturesEntity, setLineCallingFeaturesEntity};

struct CWMP_PRMT tVPLineCallingFeaturesEntityLeafInfo[] =
{
    /*(name,                    type,           flag,                  op)*/
	{ "CallerIDName",           eCWMP_tSTRING,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
	{ "CallWaitingEnable",      eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
	{ "X_Stanby-CallerIDName",           eCWMP_tSTRING,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardUnconditionalEnable", eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardUnconditionalNumber", eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardOnBusyEnable",        eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardOnBusyNumber",        eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardOnNoAnswerEnable",    eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardOnNoAnswerNumber",    eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallForwardOnNoAnswerRingCount", eCWMP_tUINT,        CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "CallTransferEnable",             eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "MWIEnable",                      eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_HotlineEnable",             eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_HotlineNumber",             eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_HotlineTimer",              eCWMP_tUINT,        CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_conference-uri",            eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPLineCallingFeaturesEntityLeafOP},
};

enum eVPLineCallingFeaturesEntityLeaf
{
    eCallingFeaturesCallerIDName,
    eCallingFeaturesCallWaitingEnable,
    eCallingFeatures_XCT_CallerIDName,
    eCallingFeaturesCallForwardUnconditionalEnable,
    eCallingFeaturesCallForwardUnconditionalNumber,
    eCallingFeaturesCallForwardOnBusyEnable,
    eCallingFeaturesCallForwardOnBusyNumber,
    eCallingFeaturesCallForwardOnNoAnswerEnable,
    eCallingFeaturesCallForwardOnNoAnswerNumber,
    eCallingFeaturesCallForwardOnNoAnswerRingCount,
    eCallingFeaturesCallTransferEnable,
    eCallingFeaturesMWIEnable,
    eCallingFeatures_XCT_HotlineEnable,
    eCallingFeatures_XCT_HotlineNumber,
    eCallingFeatures_XCT_HotlineTimer,
    eCallingFeatures_XCT_ConferenceUri,
};

struct CWMP_LEAF tVPLineCallingFeaturesEntityLeaf[] =
{
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallerIDName]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallWaitingEnable]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeatures_XCT_CallerIDName]  },    
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardUnconditionalEnable]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardUnconditionalNumber]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardOnBusyEnable]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardOnBusyNumber]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardOnNoAnswerEnable]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardOnNoAnswerNumber]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallForwardOnNoAnswerRingCount]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesCallTransferEnable]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeaturesMWIEnable]  },
#ifdef CONFIG_CU
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeatures_XCT_HotlineEnable]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeatures_XCT_HotlineNumber]  },
    { &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeatures_XCT_HotlineTimer]  },
	{ &tVPLineCallingFeaturesEntityLeafInfo[eCallingFeatures_XCT_ConferenceUri]  },
#endif
    { NULL }
};


//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}. EntityLEAF*******
struct CWMP_OP tVPLineEntityLeafOP = { getVoiceProfileLineEntity, setVoiceProfileLineEntity};	//FIXME

struct CWMP_PRMT tVPLineEntityLeafInfo[] =
{
    /*(name,              type,            flag,                  op)*/
	{ "Enable",           eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "DirectoryNumber",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "Status",           eCWMP_tSTRING,  CWMP_READ,             &tVPLineEntityLeafOP},
	{ "X_Standby-Enable", 		  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "X_Standby-DirectoryNumber",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "CallState",  eCWMP_tSTRING,  CWMP_READ,  &tVPLineEntityLeafOP},
	{ "SubsUA",      eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "SubsReg",      eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPLineEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_PBXPrefixEnable",  eCWMP_tBOOLEAN,  CWMP_READ|CWMP_WRITE, &tVPLineEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_PBXPrefix",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVPLineEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_PBXDialSecondaryEnable",  eCWMP_tBOOLEAN,  CWMP_READ|CWMP_WRITE, &tVPLineEntityLeafOP},
};

enum eVPLineEntityLeaf
{
	eLineEnable,
	eLineDirectoryNumber,
	eLineStatus,
	eX_STB_LineEnable,
	eX_STB_LineDirectoryNumber,    
	eLineCallState,
	eLineSubsUA,    
	eLineSubsReg,
	eLine_CT_PBXPrefixEnable,
	eLine_CT_PBXPrefix,
	eLine_CT_PBXDialSecondaryEnable,
};

struct CWMP_LEAF tVPLineEntityLeaf[] =
{
    { &tVPLineEntityLeafInfo[eLineEnable]  },
    { &tVPLineEntityLeafInfo[eLineDirectoryNumber]  },
    { &tVPLineEntityLeafInfo[eLineStatus]  },
    { &tVPLineEntityLeafInfo[eX_STB_LineEnable]  },
    { &tVPLineEntityLeafInfo[eX_STB_LineDirectoryNumber]  },    
    { &tVPLineEntityLeafInfo[eLineCallState]  },
    { &tVPLineEntityLeafInfo[eLineSubsUA]  },    
    { &tVPLineEntityLeafInfo[eLineSubsReg]  },    
    { &tVPLineEntityLeafInfo[eLine_CT_PBXPrefixEnable]  }, 
    { &tVPLineEntityLeafInfo[eLine_CT_PBXPrefix]  }, 
    { &tVPLineEntityLeafInfo[eLine_CT_PBXDialSecondaryEnable]  }, 
    { NULL }
};


//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}. EntityObject*******
struct CWMP_PRMT tVPLineEntityObjectInfo[] =
{
    /*(name,             type,           flag,       op)*/
	{ "SIP",             eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{ "Stats",           eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{ "Codec",           eCWMP_tOBJECT,  CWMP_READ,  NULL},
    { "VoiceProcessing", eCWMP_tOBJECT,  CWMP_READ,  NULL},
#ifdef CONFIG_E8B     
    { "X_"CTCOM_NAME_PREFIX"_IMS",           eCWMP_tOBJECT,  CWMP_READ,  NULL},  
    { "X_"CTCOM_NAME_PREFIX"_LastRegisterError", eCWMP_tOBJECT,  CWMP_READ,  NULL},    
    {"X_"CTCOM_NAME_PREFIX"_H248",           eCWMP_tOBJECT,  CWMP_READ,  NULL},
#endif
	{ "CallingFeatures", eCWMP_tOBJECT,  CWMP_READ,	 NULL}
};
enum eVPLineEntityObject
{
	eLineSIP,
	eLineStats,
	eLineCodec,
	eLineVoiceProcessing,
	eLineXCTIMS,
#ifdef CONFIG_E8B
	eLineXCTCOMLastRegisterError,
	eX_CTCOM_H248,
#endif
	eLineCallingFeatures
};
struct CWMP_NODE tVPLineEntityObject[] =
{
    /*info,                                          leaf,                             next)*/
    {&tVPLineEntityObjectInfo[eLineSIP],             tVPLineSIPEntityLeaf,             NULL},	
    {&tVPLineEntityObjectInfo[eLineStats],           tVPLineStatsEntityLeaf,           NULL},
    {&tVPLineEntityObjectInfo[eLineCodec],           tVPLineCodecEntityLeaf,     tVPLineCodecEntityObject},
    {&tVPLineEntityObjectInfo[eLineVoiceProcessing], tVPLineVoiceProcessingEntityLeaf, NULL},
#ifdef CONFIG_E8B    
    {&tVPLineEntityObjectInfo[eLineXCTIMS],          tVPLineXCT_IMSEntityLeaf,         NULL},
    {&tVPLineEntityObjectInfo[eLineXCTCOMLastRegisterError], tVPLineXCTCOM_LastRegisterErrorEntityLeaf , NULL},
    {&tVPLineEntityObjectInfo[eX_CTCOM_H248],                  tVPH248EntityLeaf,              NULL},
#endif    
    {&tVPLineEntityObjectInfo[eLineCallingFeatures], tVPLineCallingFeaturesEntityLeaf,NULL},   
    {NULL,                                           NULL,                             NULL}
};


//******* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.  *******
struct CWMP_PRMT tVPLineObjectInfo[] =
{
    /*(name,  type,           flag,		                          op)*/
    {"0",     eCWMP_tOBJECT,  CWMP_READ|CWMP_LNKLIST,  NULL}
};
enum eVPLineObject
{
    eVPLine0
};
struct CWMP_LINKNODE tVPLineObject[] =
{
    /*info,  			            leaf,               next,                 sibling,  instnum)*/
    {&tVPLineObjectInfo[eVPLine0],  tVPLineEntityLeaf,  tVPLineEntityObject,  NULL,     0}
};


int objVoiceProfileLine(char *name, struct CWMP_LEAF *e, int type, void *data)
{
    struct CWMP_NODE *entity=(struct CWMP_NODE *)e;
    
	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		int num=0,i;
		struct CWMP_LINKNODE **ptable = (struct CWMP_LINKNODE **)data;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;

		num = MAX_LINE_PER_PROFILE;
		for( i=1; i<=num;i++ )
		{			
			if( create_Object( ptable, tVPLineObject, sizeof(tVPLineObject), num, 1) < 0 )
				return -1;
		}
		add_objectNum( name, num );
		return 0;
	}
    	break;
	case eCWMP_tADDOBJ:
	case eCWMP_tDELOBJ:
         {
            CWMPDBG( 0, ( stderr, "<%s:%d> type %d unsupported now\n", __FUNCTION__, __LINE__, type) );
            return 0;
         }
    	break;  
    case eCWMP_tUPDATEOBJ:
         {
            int num=0,i;
            struct CWMP_LINKNODE *old_table;
            num = MAX_LINE_PER_PROFILE;
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
                        tVPLineObject, sizeof(tVPLineObject), (unsigned int*)&num );
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

int getVoiceProfileLineEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		enable_t Enable;
		char tmp_str[ 32 ];
		line_status_t Status;
		boolean SubsUA;
		boolean SubsReg;
		boolean PBXPrefixEnable;
		boolean PBXDialSecondaryEnable;
		char PBXPrefix[32];
		
	} s;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

	if(!gVoipReq){
		cwmpSendRequestToSolar();
		gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
   	 }
	
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Enable );
		*data = strdup_Enable( s.Enable );
	}else if( strcmp( pszLastname, "X_Standby-Enable" ) == 0 ) {
			mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_ENABLE, 
						   nVoiceProfileInstNum, nSipLineInstNum,
						   &s.Enable );
			*data = strdup_Enable( s.Enable );
		
	} else if( strcmp( pszLastname, "DirectoryNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.tmp_str );
		*data = strdup( s.tmp_str );
	} else if( strcmp( pszLastname, "X_Standby-DirectoryNumber" ) == 0 ) {
			mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_DIRECTORY_NUMBER, 
						   nVoiceProfileInstNum, nSipLineInstNum,
						   s.tmp_str );
			*data = strdup( s.tmp_str );
		
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__STATUS, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Status );
		*data = strdup_LineStatus( s.Status );
	} else if( strcmp( pszLastname, "CallState" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__CALL_STATE, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.tmp_str );
				*data = strdup( s.tmp_str );
				gVoipReq = EVT_VOICEPROFILE_LINE_NONE;
	} else if( strcmp( pszLastname, "SubsUA" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_SBUS_UA, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.SubsUA );
				*data = booldup( s.SubsUA );
	} else if( strcmp( pszLastname, "SubsReg" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_SUBSREG, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.SubsReg );
				*data = booldup( s.SubsReg );
	}else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_PBXPrefix" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.PBXPrefix );
				*data = strdup( s.PBXPrefix );
	}else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_PBXPrefixEnable" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX_ENABLE, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.PBXPrefixEnable );
				*data = booldup( s.PBXPrefixEnable );				
	}else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_PBXDialSecondaryEnable" ) == 0 ) {
				mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIXSECONDARY_ENABLE, 
							   nVoiceProfileInstNum, nSipLineInstNum,
							   &s.PBXDialSecondaryEnable );
				*data = booldup( s.PBXDialSecondaryEnable );				
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setVoiceProfileLineEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		enable_t Enable;
		char DirectoryNumber[ 32 ];
		line_status_t Status;
	} s;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "Enable" ) == 0 ) {

	/*add set value verify */
		if(strcmp(data,"Enabled")&&strcmp(data,"Disabled"))
			return ERR_9007;
			
		str2id_Enable( data, &s.Enable );
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Enable );


	}else if( strcmp( pszLastname, "X_Standby-Enable" ) == 0 ) {

	/*add set value verify */
		if(strcmp(data,"Enabled")&&strcmp(data,"Disabled"))
			return ERR_9007;
			
		str2id_Enable( data, &s.Enable );
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Enable );
		
	} else if( strcmp( pszLastname, "DirectoryNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "X_Standby-DirectoryNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_DIRECTORY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );	
	} else if( strcmp( pszLastname, "SubsUA" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_SBUS_UA, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );	
	} else if( strcmp( pszLastname, "SubsReg" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_SUBSREG, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );	
	} else if( strcmp( pszLastname,"X_"CTCOM_NAME_PREFIX"_PBXPrefix" ) == 0 ) {
			res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );		
	} else if( strcmp( pszLastname,"X_"CTCOM_NAME_PREFIX"_PBXPrefixEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIX_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );		
	} else if( strcmp( pszLastname,"X_"CTCOM_NAME_PREFIX"_PBXDialSecondaryEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_PBXPREFIXSECONDARY_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );				
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		return ERR_9008;
	} else {
		return ERR_9005;
	}

	return res;
}

int getLineStatsEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Stats. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
    //unsigned int chid;
	extern cwmpEvtMsg pEvtMsg;
#if 0
	union {
		TstRtpRtcpStatistics rtpStatistics;
	} s;
#endif
    const char *pszLastname = entity->info->name;

	if(!gVoipReq){
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }
	
	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

	/*
	 * FIXME: Now, instance number of 'Line' is seens as channel ID,  
	 *        but specification indicate thta DirectoryNumber or PhyReferenceList 
	 *        is used to identify or associate with physical interface. 
	 */
	//chid = nSipLineInstNum;
#if 1
	if( strcmp( pszLastname, "PacketsSent" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsSent);
		printf("@@@@@Line %d packetsSent:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsSent);
	} else if( strcmp( pszLastname, "PacketsReceived" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsReceived);
		printf("@@@@@Line %d PacketsReceived:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsReceived);
	} else if( strcmp( pszLastname, "BytesSent" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].bytesSent);
		printf("@@@@@Line %d bytesSent:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].bytesSent);
	} else if( strcmp( pszLastname, "BytesReceived" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].bytesReceived);
		printf("@@@@@Line %d bytesReceived:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].bytesReceived);
	} else if( strcmp( pszLastname, "PacketsLost" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsLost);
		printf("@@@@@Line %d packetsLost:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].packetsLost);
	} else if( strcmp( pszLastname, "IncomingCallsReceived" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsReceived);
		printf("@@@@@Line %d IncomingCallsReceived:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsReceived);
	} else if( strcmp( pszLastname, "IncomingCallsAnswered" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsAnswered);
		printf("@@@@@Line %d IncomingCallsAnswered:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsAnswered);
	} else if( strcmp( pszLastname, "IncomingCallsConnected" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsConnected);
		printf("@@@@@Line %d IncomingCallsConnected:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsConnected);
	} else if( strcmp( pszLastname, "IncomingCallsFailed" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsFailed);
		printf("@@@@@Line %d IncomingCallsFailed:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsFailed);
	} else if( strcmp( pszLastname, "OutgoingCallsAttempted" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAttempted);
		printf("@@@@@Line %d OutgoingCallsAttempted:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAttempted);
	} else if( strcmp( pszLastname, "OutgoingCallsAnswered" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAnswered);
		printf("@@@@@Line %d OutgoingCallsAnswered:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAnswered);
	} else if( strcmp( pszLastname, "OutgoingCallsConnected" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsConnected);
		printf("@@@@@Line %d OutgoingCallsConnected:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsConnected);
	} else if( strcmp( pszLastname, "OutgoingCallsFailed" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsFailed);
		printf("@@@@@Line %d OutgoingCallsFailed:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsFailed);
	} else if( strcmp( pszLastname, "CallsDropped" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].callsDropped);
		printf("@@@@@Line %d CallsDropped:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].callsDropped);		
	} else if( strcmp( pszLastname, "ResetStatistics" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].resetStatistics);
		printf("@@@@@Line %d ResetStatistics:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].resetStatistics);
	} else if( strcmp( pszLastname, "TotalCallTime" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].totalCallTime);
		printf("@@@@@Line %d TotalCallTime:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].totalCallTime);
    } else if( strcmp( pszLastname, "ReceivePacketLossRate" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].ReceivePacketLossRate);
    } else if( strcmp( pszLastname, "FarEndPacketLossRate" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].FarEndPacketLossRate);
    } else if( strcmp( pszLastname, "ReceiveInterarrivalJitter" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].ReceiveInterarrivalJitter);
    } else if( strcmp( pszLastname, "FarEndInterarrivalJitter" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].FarEndInterarrivalJitter);
    } else if( strcmp( pszLastname, "RoundTripDelay" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].RoundTripDelay);
    } else if( strcmp( pszLastname, "ServerDownTime" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].ServerDownTime);
	} else {
		*data = NULL;
		return ERR_9005;
	}
#endif
	return 0;
}

int setLineStatsEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	/*union {
		TstRtpRtcpStatistics rtpStatistics;
	} s;*/

	const char *pszLastname = entity->info->name;
    extern cwmpEvtMsg pEvtMsg;

    if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "ResetStatistics" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__STATS__RESET_STATISTICS, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}
            
	} else if( strcmp( pszLastname, "PacketsSent" ) == 0 || 
               strcmp( pszLastname, "BytesSent" ) == 0 ||
               strcmp( pszLastname, "BytesReceived" ) == 0 ||
               strcmp( pszLastname, "PacketsLost" ) == 0 ||
               strcmp( pszLastname, "IncomingCallsReceived" ) == 0 ||
               strcmp( pszLastname, "IncomingCallsAnswered" ) == 0 ||
               strcmp( pszLastname, "IncomingCallsConnected" ) == 0 ||
               strcmp( pszLastname, "IncomingCallsFailed" ) == 0 ||
               strcmp( pszLastname, "OutgoingCallsAttempted" ) == 0 ||
               strcmp( pszLastname, "OutgoingCallsAnswered" ) == 0 ||
               strcmp( pszLastname, "OutgoingCallsConnected" ) == 0 ||
               strcmp( pszLastname, "OutgoingCallsFailed" ) == 0 ||
               strcmp( pszLastname, "CallsDropped" ) == 0 ||
               strcmp( pszLastname, "TotalCallTime" ) == 0 || 
               strcmp( pszLastname, "ReceivePacketLossRate" ) == 0 ||
               strcmp( pszLastname, "FarEndPacketLossRate" ) == 0 ||
               strcmp( pszLastname, "ReceiveInterarrivalJitter" ) == 0 ||
               strcmp( pszLastname, "FarEndInterarrivalJitter" ) == 0 ||
               strcmp( pszLastname, "RoundTripDelay" ) == 0 ||
               strcmp( pszLastname, "ServerDownTime" ) == 0 ) {
		return ERR_9008;
	} else {
		return ERR_9005;
	}
    return res;
}


int getLineCodecEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.codec. entry */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
    //unsigned int chid;
	extern cwmpEvtMsg pEvtMsg;
#if 0
	union {
		TstRtpRtcpStatistics rtpStatistics;
	} s;
#endif
    const char *pszLastname = entity->info->name;

	if(!gVoipReq){
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }
	
	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

	/*
	 * FIXME: Now, instance number of 'Line' is seens as channel ID,  
	 *        but specification indicate thta DirectoryNumber or PhyReferenceList 
	 *        is used to identify or associate with physical interface. 
	 */
	//chid = nSipLineInstNum;

	if( strcmp( pszLastname, "TransmitCodec" ) == 0 ) {
		*data = strdup(  pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].codec_string);
			printf("@@@@@Line %d TransmitCodec:%s\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].codec_string);
	}else if( strcmp( pszLastname, "ReceiveCodec" ) == 0 ) {
		*data = strdup(  pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].codec_string);
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setLineCodecEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	/*union {
		TstRtpRtcpStatistics rtpStatistics;
	} s;*/

	const char *pszLastname = entity->info->name;
    extern cwmpEvtMsg pEvtMsg;

    if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d>Debug: set %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "TransmitCodec" ) == 0 || 
               strcmp( pszLastname, "ReceiveCodec" ) == 0 ) {
		return ERR_9008;
	} else {
		return ERR_9005;
	}
    return res;
}


int getLineSipEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		char AuthUserName[ 128 ];
		char AuthPassword[ 128 ];
		char URI[ 389 ];
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "AuthUserName" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.AuthUserName );
		*data = strdup( s.AuthUserName );

	}else if( strcmp( pszLastname, "X_Standby-AuthUserName" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.AuthUserName );
		*data = strdup( s.AuthUserName );
		
	} else if( strcmp( pszLastname, "AuthPassword" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.AuthPassword );
		*data = strdup( s.AuthPassword );
	} else if( strcmp( pszLastname, "X_Standby-AuthPassword" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_PASSWORD, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.AuthPassword );
		*data = strdup( s.AuthPassword );

		
	} else if( strcmp( pszLastname, "URI" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.URI );
		*data = strdup( s.URI );
	} else if( strcmp( pszLastname, "X_Standby-URI" ) == 0 ) {
			mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_URI, 
						   nVoiceProfileInstNum, nSipLineInstNum,
						   &s.URI );
			*data = strdup( s.URI );
		
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setLineSipEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "AuthUserName" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	}else if( strcmp( pszLastname, "X_Standby-AuthUserName" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		
	} else if( strcmp( pszLastname, "AuthPassword" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "X_Standby-AuthPassword" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_AUTH_PASSWORD, 
						   nVoiceProfileInstNum, nSipLineInstNum,
						   data );
		
	} else if( strcmp( pszLastname, "URI" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "X_Standby-URI" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__XCT_URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		
	} else {
		return ERR_9005;
	}

	return res;
}

int getLineVoiceProcessingEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.VoiceProcessing. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		int TransmitGain;
		int ReceiveGain;
		int EchoCancellationEnable;
		int  EchoCancellationInUse;
        unsigned int EchoCancellationTail;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "TransmitGain" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__TRANSMIT_GAIN, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.TransmitGain );
		*data = intdup( s.TransmitGain );
	} else if( strcmp( pszLastname, "ReceiveGain" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__RECEIVE_GAIN, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.ReceiveGain );
		*data = intdup( s.ReceiveGain );
	} else if( strcmp( pszLastname, "EchoCancellationEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.EchoCancellationEnable );
		*data = booldup( s.EchoCancellationEnable );
	} else if( strcmp( pszLastname, "EchoCancellationInUse" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_INUSE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.EchoCancellationInUse );
		*data = booldup( s.EchoCancellationInUse );
	} else if( strcmp( pszLastname, "EchoCancellationTail" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_TAIL, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.EchoCancellationTail );
		*data = uintdup( s.EchoCancellationTail );
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setLineVoiceProcessingEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.VoiceProcessing. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "TransmitGain" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__TRANSMIT_GAIN, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "ReceiveGain" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__RECEIVE_GAIN, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "EchoCancellationEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__VOIP_PROCESSING__ECHO_CANCELLATION_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} else if( strcmp( pszLastname, "EchoCancellationInUse" ) == 0 ) {
		return ERR_9008;
	} else if( strcmp( pszLastname, "EchoCancellationTail" ) == 0 ) {
		return ERR_9008;
	} else {
		return ERR_9005;
	}

	return res;
}


int getLineCallingFeaturesEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.CallingFeatures. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		char CallerIDName[ 40 ];
		int CallWaitingEnable;
        boolean CallForwardUnconditionalEnable;
		char CallForwardUnconditionalNumber[33];
        boolean CallForwardOnBusyEnable;
		char CallForwardOnBusyNumber[33];
        boolean CallForwardOnNoAnswerEnable;
        char CallForwardOnNoAnswerNumber[33];
        unsigned int CallForwardOnNoAnswerRingCount;
        boolean CallTransferEnable;
        boolean MWIEnable;
        boolean XCT_HotlineEnable;
        char XCT_HotlineNumber[33];
        unsigned int XCT_HotlineTimer;
        char XCT_ConferenceUri[33];
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
	
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;

	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;

	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;

	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	*type = entity->info->type;
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );



	if( strcmp( pszLastname, "CallerIDName" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLERIDNAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.CallerIDName );
		*data = strdup( s.CallerIDName );
	}else if( strcmp( pszLastname, "X_Stanby-CallerIDName" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_CALLING_FEATURES_CALLERIDNAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.CallerIDName );
		*data = strdup( s.CallerIDName );
	}else if( strcmp( pszLastname, "CallWaitingEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLWAITING_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallWaitingEnable );
		*data = booldup( s.CallWaitingEnable );
    }else if( strcmp( pszLastname, "CallForwardUnconditionalEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallForwardUnconditionalEnable );
		*data = booldup( s.CallForwardUnconditionalEnable );
    }else if( strcmp( pszLastname, "CallForwardUnconditionalNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.CallForwardUnconditionalNumber );
		*data = strdup( s.CallForwardUnconditionalNumber );
    }else if( strcmp( pszLastname, "CallForwardOnBusyEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallForwardOnBusyEnable );
		*data = booldup( s.CallForwardOnBusyEnable );
    }else if( strcmp( pszLastname, "CallForwardOnBusyNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.CallForwardOnBusyNumber );
		*data = strdup( s.CallForwardOnBusyNumber );
    }else if( strcmp( pszLastname, "CallForwardOnNoAnswerEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallForwardOnNoAnswerEnable );
		*data = booldup( s.CallForwardOnNoAnswerEnable );
    }else if( strcmp( pszLastname, "CallForwardOnNoAnswerNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.CallForwardOnNoAnswerNumber );
		*data = strdup( s.CallForwardOnNoAnswerNumber );
    } else if( strcmp( pszLastname, "CallForwardOnNoAnswerRingCount" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_RINGCOUNT, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallForwardOnNoAnswerRingCount );
		*data = uintdup( s.CallForwardOnNoAnswerRingCount );
    }else if( strcmp( pszLastname, "CallTransferEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLTRANSFER_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.CallTransferEnable );
		*data = booldup( s.CallTransferEnable );
    }else if( strcmp( pszLastname, "MWIEnable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_MWI_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.MWIEnable );
		*data = booldup( s.MWIEnable );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineEnable") == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.XCT_HotlineEnable );
		*data = booldup( s.XCT_HotlineEnable );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.XCT_HotlineNumber );
		*data = strdup( s.XCT_HotlineNumber );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineTimer" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_TIMER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.XCT_HotlineTimer );
		*data = uintdup( s.XCT_HotlineTimer );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_conference-uri" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_CONFERENCE_URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.XCT_ConferenceUri );
		*data = strdup( s.XCT_ConferenceUri );
	}


return 0;
}

int setLineCallingFeaturesEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity->info->name;
	

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );


	if( strcmp( pszLastname, "CallerIDName" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLERIDNAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	}else if( strcmp( pszLastname, "X_Stanby-CallerIDName" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_CALLING_FEATURES_CALLERIDNAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
			
	}else if( strcmp( pszLastname, "CallWaitingEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLWAITING_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallForwardUnconditionalEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallForwardUnconditionalNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_UNCONDITIONAL_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallForwardOnBusyEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallForwardOnBusyNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_BUSY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
                       data );
    }else if( strcmp( pszLastname, "CallForwardOnNoAnswerEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
                       data );
    }else if( strcmp( pszLastname, "CallForwardOnNoAnswerNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallForwardOnNoAnswerRingCount" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLFORWARD_ON_NOANSWER_RINGCOUNT, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "CallTransferEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_CALLTRANSFER_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "MWIEnable" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_MWI_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineEnable") == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineNumber" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HotlineTimer" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_HOTLINE_TIMER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
    }else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_conference-uri" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__CALLING_FEATURES_XCT_CONFERENCE_URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
	} 
	
	return res;

}


int getLineXCT_IMSEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.X_"CTCOM_NAME_PREFIX"_IMS. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		unsigned int dialTonePattern;
		unsigned int mcidService;
        unsigned int noDialingBehaviour;
        unsigned int holdService;
        unsigned int threePartyService;
        char conferenceUri[64];
        char hotlineUri[64];
        unsigned int cwService;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "dial-tone-pattern" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__DIALTONEPATTERN, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.dialTonePattern );
		*data = uintdup( s.dialTonePattern );
	} else if( strcmp( pszLastname, "mcid-service" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__MCIDSERVICE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.mcidService );
		*data = uintdup( s.mcidService );
	} else if( strcmp( pszLastname, "no-dialing-behaviour" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__NODIALINGBEHAVIOUR, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.noDialingBehaviour );
		*data = uintdup( s.noDialingBehaviour );
	} else if( strcmp( pszLastname, "hold-service" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__HOLDSERVICE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.holdService );
		*data = uintdup( s.holdService );
	} else if( strcmp( pszLastname, "three-party-service" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__THREEPARTYSERVICE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.threePartyService );
		*data = uintdup( s.threePartyService );
	} else if( strcmp( pszLastname, "conference-uri" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__CONFERENCEURI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.conferenceUri );
		*data = strdup( s.conferenceUri );
	} else if( strcmp( pszLastname, "hotline-uri" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__HOTLINEURI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.hotlineUri );
		*data = strdup( s.hotlineUri );
	} else if( strcmp( pszLastname, "cw-service" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__CWSERVICE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.cwService );
		*data = uintdup( s.cwService );
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setLineXCT_IMSEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.X_IMS. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
 
	if( strcmp( pszLastname, "dial-tone-pattern" ) == 0 ) {
		return ERR_9008; //read only
	} else if( strcmp( pszLastname, "mcid-service" ) == 0 ) {
		return ERR_9008; //read only
	} else if( strcmp( pszLastname, "no-dialing-behaviour" ) == 0 ) {
		return ERR_9008; //read only
	} else if( strcmp( pszLastname, "hold-service" ) == 0 ) {
		return ERR_9008; //read only
	} else if( strcmp( pszLastname, "three-party-service" ) == 0 ) {
		return ERR_9008; //read only
	} else if( strcmp( pszLastname, "conference-uri" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__CONFERENCEURI, 
					   nVoiceProfileInstNum, nSipLineInstNum, data );
	} else if( strcmp( pszLastname, "hotline-uri" ) == 0 ) {
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCT_IMS__HOTLINEURI, 
					   nVoiceProfileInstNum, nSipLineInstNum, data );
	} else if( strcmp( pszLastname, "cw-service" ) == 0 ) {
		return ERR_9008; //read only
	} else {
		return ERR_9005;
	}

	return res;
}

#ifdef CONFIG_E8B
int getLineXCTCOM_LastRegisterErrorEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.X_"CTCOM_NAME_PREFIX"_LastRegisterError. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	extern cwmpEvtMsg pEvtMsg;
	union {
        char reason[64];
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;
	
	if(!gVoipReq){
        CWMPDBG( 3, ( stderr, "<%s:%d>INFO: Send Request from cwmp to solar", __FUNCTION__, __LINE__) );
        cwmpSendRequestToSolar();
        gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
    }
	
    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "Reason" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.reason);
		*data = strdup( s.reason);
		
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setLineXCTCOM_LastRegisterErrorEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.X_"CTCOM_NAME_PREFIX"_LastRegisterError. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity->info->name;

	printf("%s(%d)\n" , __FUNCTION__ , __LINE__);

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;

	printf("%s(%d) %s\n" , __FUNCTION__ , __LINE__ , name);
	printf("%s(%d) %s\n" , __FUNCTION__ , __LINE__ , pszLastname);
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
 
	if( strcmp( pszLastname, "Reason" ) == 0 ) {
	    CWMPDBG( 0, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
		res = mib_set_type2( MIB_VOICE_PROFILE__LINE__XCTCOM_LASTREGISTERERROR__REASON, 
					   nVoiceProfileInstNum, nSipLineInstNum, data );
	} 
	else {
		return ERR_9005;
	}

	return res;
}

#endif


#if 0
static int getVoiceProfileLineCodecListEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setVoiceProfileLineCodecListEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);
static int objVoiceProfileLineCodec(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

static int getVoiceProfileLineStatusEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setVoiceProfileLineStatusEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

static int getVoiceProfileLineSipEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setLineSipEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

static int getVoiceProfileLineEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setVoiceProfileLineEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);


struct sCWMP_ENTITY tVoiceProfileLineCodecListEntity[] = {
/*	{ name,					type,			flag,					accesslist,	getvalue,								setvalue,							next_table,	sibling } */
	{ "EntryId",			eCWMP_tUINT,	CWMP_READ,				NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "Codec",				eCWMP_tSTRING,	CWMP_READ,				NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "BitRate",			eCWMP_tUINT,	CWMP_READ,				NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "PacketizationPeriod",eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "SilenceSuppression",	eCWMP_tBOOLEAN,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "Priority",			eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineCodecListEntity, 	setVoiceProfileLineCodecListEntity,	NULL,		NULL },
	{ "",					eCWMP_tNONE,	0,						NULL,		NULL,									NULL,								NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileLineCodecList[] = {
/*	{ name,		type,			flag,								accesslist,	getvalue,	setvalue,	next_table,							sibling } */
	{ "0",		eCWMP_tOBJECT,	CWMP_READ|CWMP_WRITE|CWMP_LNKLIST,	NULL,		NULL,		NULL,		tVoiceProfileLineCodecListEntity, 	NULL },
};

struct sCWMP_ENTITY tVoiceProfileLineCodecEntity[] = {
/*	{ name,					type,			flag,		accesslist,	getvalue,					setvalue,					next_table,	sibling } */
	{ "List",				eCWMP_tOBJECT,	CWMP_READ,	NULL,		NULL, 						objVoiceProfileLineCodec,	NULL,		NULL },
	{ "",					eCWMP_tNONE,	0,			NULL,		NULL,						NULL,						NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileLineStatusEntity[] = {
/*	{ name,						type,			flag,		accesslist,	getvalue,							setvalue,							next_table,	sibling } */
	{ "PacketsSent",			eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "PacketsReceived",		eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "BytesSent",				eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "BytesReceived",			eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "PacketsLost",			eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "IncomingCallsReceived",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "IncomingCallsAnswered",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "IncomingCallsConnected",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "IncomingCallsFailed",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "OutgoingCallsAttempted",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "OutgoingCallsAnswered",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "OutgoingCallsConnected",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "OutgoingCallsFailed",	eCWMP_tUINT,	CWMP_READ,	NULL,		getVoiceProfileLineStatusEntity, 	setVoiceProfileLineStatusEntity,	NULL,		NULL },
	{ "",						eCWMP_tNONE,	0,			NULL,		NULL,								NULL,								NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileLineSipEntity[] = {
/*	{ name,					type,			flag,					accesslist,	getvalue,						setvalue,						next_table,	sibling } */
	{ "AuthUserName",		eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineSipEntity, 	setLineSipEntity,	NULL,		NULL },
	{ "AuthPassword",		eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineSipEntity, 	setLineSipEntity,	NULL,		NULL },
	{ "URI",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineSipEntity, 	setLineSipEntity,	NULL,		NULL },
	{ "",					eCWMP_tNONE,	0,						NULL,		NULL,							NULL,							NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileLineEntity[] = {
/*	{ name,					type,			flag,					accesslist,	getvalue,					setvalue,					next_table,						sibling } */
	{ "Enable",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineEntity, 	setVoiceProfileLineEntity,	NULL,							NULL },
	{ "DirectoryNumber",	eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileLineEntity, 	setVoiceProfileLineEntity,	NULL,							NULL },
	{ "Status",				eCWMP_tSTRING,	CWMP_READ,				NULL,		getVoiceProfileLineEntity, 	setVoiceProfileLineEntity,	NULL,							NULL },
	{ "SIP",				eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 						NULL,						tVoiceProfileLineSipEntity,		NULL },
	{ "Stats",				eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 						NULL,						tVoiceProfileLineStatusEntity,	NULL },
	{ "Codec",				eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 						NULL,						tVoiceProfileLineCodecEntity,	NULL },
	{ "",					eCWMP_tNONE,	0,						NULL,		NULL,						NULL,						NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileLine[] = {
/*	{ name,		type,			flag,								accesslist,	getvalue,	setvalue,	next_table,				sibling } */
	{ "0",		eCWMP_tOBJECT,	CWMP_READ|CWMP_WRITE|CWMP_LNKLIST,	NULL,		NULL,		NULL,		tVoiceProfileLineEntity, 	NULL },
};

static int getVoiceProfileLineCodecListEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.Codec.List.{i}. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nCodecListInstNum;
	unsigned int nNumberOfLine;
	union {
		char PacketizationPeriod[ 64 ];
		int SilenceSuppression;
		unsigned int Priority;
	} s;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
	
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line_List( name, &nVoiceProfileInstNum, &nSipLineInstNum, &nCodecListInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( -- nCodecListInstNum >= MAX_CODEC_LIST )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;

	if( strcmp( pszLastname, "EntryId" ) == 0 ) {
		*data = uintdup( nCodecListInstNum + 1 );	/* 1 based */
	} else if( strcmp( pszLastname, "Codec" ) == 0 ) {
		*data = strdup( lstCodecs[ nCodecListInstNum ].pszCodec );
	} else if( strcmp( pszLastname, "BitRate" ) == 0 ) {
		*data = uintdup( lstCodecs[ nCodecListInstNum ].nBitRate );
	} else if( strcmp( pszLastname, "PacketizationPeriod" ) == 0 ) {
		mib_get_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PACKETIZATION_PERIOD, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   &s.PacketizationPeriod );
		*data = strdup( s.PacketizationPeriod );
	} else if( strcmp( pszLastname, "SilenceSuppression" ) == 0 ) {
		mib_get_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__SILENCE_SUPPRESSION, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   &s.SilenceSuppression );
		*data = booldup( s.SilenceSuppression );
	} else if( strcmp( pszLastname, "Priority" ) == 0 ) {
		mib_get_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PRIORITY, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   &s.Priority );
		*data = uintdup( s.Priority );
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

static int setVoiceProfileLineCodecListEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec.List.{i}. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nCodecListInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
	
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line_List( name, &nVoiceProfileInstNum, &nSipLineInstNum, &nCodecListInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( -- nCodecListInstNum >= MAX_CODEC_LIST )	/* convert to zero based */
		return -1;

	if( entity ->type != type )
		return ERR_9006;

	if( strcmp( pszLastname, "EntryId" ) == 0 ) {
		return ERR_9008;
	} else if( strcmp( pszLastname, "Codec" ) == 0 ) {
		return ERR_9008;
	} else if( strcmp( pszLastname, "BitRate" ) == 0 ) {
		return ERR_9008;
	} else if( strcmp( pszLastname, "PacketizationPeriod" ) == 0 ) {
		mib_set_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PACKETIZATION_PERIOD, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   data );
		return 1;
	} else if( strcmp( pszLastname, "SilenceSuppression" ) == 0 ) {
		mib_set_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__SILENCE_SUPPRESSION, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   data );
		return 1;
	} else if( strcmp( pszLastname, "Priority" ) == 0 ) {
		mib_set_type3( MIB_VOICE_PROFILE__LINE__SIP__CODEC__LIST__PRIORITY, 
					   nVoiceProfileInstNum, nSipLineInstNum, nCodecListInstNum,
					   data );
		return 1;
	} else {
		return ERR_9005;
	}

	return 0;
}

static int objVoiceProfileLineCodec(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		int num=0,i;
		struct sCWMP_ENTITY **c = (struct sCWMP_ENTITY **)data;
		//MIB_CE_IP_ROUTE_T *p,route_entity;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;

		num = 1;//mib_chain_total( MIB_IP_ROUTE_TBL );
		for( i=0; i<num;i++ )
		{
			//p = &route_entity;
			//if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ) )
			//	continue;
			
			//if( p->InstanceNum==0 ) //maybe createn by web or cli
			//{
			//	MaxInstNum++;
			//	p->InstanceNum = MaxInstNum;
			//	mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)p, i );
			//}else
			//	MaxInstNum = p->InstanceNum;
			
			if( create_Object( c, tVoiceProfileLineCodecList, sizeof(tVoiceProfileLineCodecList), MAX_CODEC_LIST, 1 ) < 0 )
				return -1;
			//c = & (*c)->sibling;
		}
		add_objectNum( name, 1 );
		return 0;
	}
#if 0
	// TODO: Not implement add/del/update yet.  
	case eCWMP_tADDOBJ:
	     {
	     	int ret;
	     	if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;
		ret = add_Object( name, &entity->next_table,  tForwarding, sizeof(tForwarding), data );
		if( ret >= 0 )
		{
			MIB_CE_IP_ROUTE_T fentry;
			memset( &fentry, 0, sizeof( MIB_CE_IP_ROUTE_T ) );
			fentry.InstanceNum = *(unsigned int*)data;
			fentry.FWMetric = -1;
			fentry.ifIndex = 0xff;
			mib_chain_add( MIB_IP_ROUTE_TBL, (unsigned char*)&fentry );
		}
		return ret;
	     }
	case eCWMP_tDELOBJ:
	     {
	     	int ret, id;
	     	
	     	if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;
	     	
	     	id = getChainID( entity->next_table, *(int*)data  );
	     	if(id==-1) return ERR_9005;
	     	mib_chain_delete( MIB_IP_ROUTE_TBL, id );	
		ret = del_Object( name, &entity->next_table, *(int*)data );
		if( ret == 0 ) return 1;
		return ret;
	     }
	case eCWMP_tUPDATEOBJ:	
	     {     	int num=0,i;
	     	struct sCWMP_ENTITY *old_table;
	     	num = mib_chain_total( MIB_IP_ROUTE_TBL );
	     	old_table = entity->next_table;
	     	entity->next_table = NULL;
	     	for( i=0; i<num;i++ )
	     	{
	     		struct sCWMP_ENTITY *remove_entity=NULL;
			MIB_CE_IP_ROUTE_T *p,route_entity;

			p = &route_entity;
			if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ) )
				continue;			
			remove_entity = remove_SiblingEntity( &old_table, p->InstanceNum );
			if( remove_entity!=NULL )
			{
				add_SiblingEntity( &entity->next_table, remove_entity );
			}else{ 
				unsigned int MaxInstNum=p->InstanceNum;			
				add_Object( name, &entity->next_table,  tForwarding,     							sizeof(tForwarding), &MaxInstNum );
				if(MaxInstNum!=p->InstanceNum)
				{     p->InstanceNum = MaxInstNum;
				      mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)p, i );
				}
			}	
	     	}
	     	if( old_table ) 	destroy_ParameterTable( old_table );	     	
	     	return 0;
	     }
#endif
	}
	return -1;
}

static int getVoiceProfileLineStatusEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.Stats. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	unsigned int chid;
	extern cwmpEvtMsg pEvtMsg;
	union {
		TstRtpRtcpStatistics rtpStatistics;
	} s;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;

	/*
	 * FIXME: Now, instance number of 'Line' is seens as channel ID,  
	 *        but specification indicate thta DirectoryNumber or PhyReferenceList 
	 *        is used to identify or associate with physical interface. 
	 */
	chid = nSipLineInstNum;

	if( strcmp( pszLastname, "PacketsSent" ) == 0 ) {
		if( rtk_GetRtpStatistics( chid, 0, &s.rtpStatistics ) )
			return ERR_9002;	/* Internal error */
		*data = uintdup( s.rtpStatistics.nTxRtpStatsCountPacket );
	} else if( strcmp( pszLastname, "PacketsReceived" ) == 0 ) {
		if( rtk_GetRtpStatistics( chid, 0, &s.rtpStatistics ) )
			return ERR_9002;	/* Internal error */
		*data = uintdup( s.rtpStatistics.nRxRtpStatsCountPacket );
	} else if( strcmp( pszLastname, "BytesSent" ) == 0 ) {
		if( rtk_GetRtpStatistics( chid, 0, &s.rtpStatistics ) )
			return ERR_9002;	/* Internal error */
		*data = uintdup( s.rtpStatistics.nTxRtpStatsCountByte );
	} else if( strcmp( pszLastname, "BytesReceived" ) == 0 ) {
		if( rtk_GetRtpStatistics( chid, 0, &s.rtpStatistics ) )
			return ERR_9002;	/* Internal error */
		*data = uintdup( s.rtpStatistics.nRxRtpStatsCountByte );
	} else if( strcmp( pszLastname, "PacketsLost" ) == 0 ) {
		if( rtk_GetRtpStatistics( chid, 0, &s.rtpStatistics ) )
			return ERR_9002;	/* Internal error */
		*data = uintdup( s.rtpStatistics.nRxRtpStatsLostPacket );
	} else if( strcmp( pszLastname, "IncomingCallsReceived" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsReceived);
		printf("@@@@@Line %d IncomingCallsReceived:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsReceived);
	} else if( strcmp( pszLastname, "IncomingCallsAnswered" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsAnswered);
		printf("@@@@@Line %d IncomingCallsAnswered:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsAnswered);
	} else if( strcmp( pszLastname, "IncomingCallsConnected" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsConnected);
		printf("@@@@@Line %d IncomingCallsConnected:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsConnected);
	} else if( strcmp( pszLastname, "IncomingCallsFailed" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsFailed);
		printf("@@@@@Line %d IncomingCallsFailed:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].incomingCallsFailed);
	} else if( strcmp( pszLastname, "OutgoingCallsAttempted" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAttempted);
		printf("@@@@@Line %d OutgoingCallsAttempted:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAttempted);
	} else if( strcmp( pszLastname, "OutgoingCallsAnswered" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAnswered);
		printf("@@@@@Line %d OutgoingCallsAnswered:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsAnswered);
	} else if( strcmp( pszLastname, "OutgoingCallsConnected" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsConnected);
		printf("@@@@@Line %d OutgoingCallsConnected:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsConnected);
	} else if( strcmp( pszLastname, "OutgoingCallsFailed" ) == 0 ) {
		*data = uintdup( pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsFailed);
		printf("@@@@@Line %d OutgoingCallsFailed:%d\n",nVoiceProfileInstNum, pEvtMsg.voiceProfileLineStatusMsg[nVoiceProfileInstNum].outgoingCallsFailed);
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

static int setVoiceProfileLineStatusEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	return ERR_9008;	/* Parameters are read only */
}

static int getVoiceProfileLineSipEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		char AuthUserName[ 128 ];
		char AuthPassword[ 128 ];
		char URI[ 389 ];
	} s;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;

	if( strcmp( pszLastname, "AuthUserName" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.AuthUserName );
		*data = strdup( s.AuthUserName );
	} else if( strcmp( pszLastname, "AuthPassword" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.AuthPassword );
		*data = strdup( s.AuthPassword );
	} else if( strcmp( pszLastname, "URI" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__SIP__URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.URI );
		*data = strdup( s.URI );
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

static int setLineSipEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}.SIP. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;

	if( entity ->type != type )
		return ERR_9006;

	if( strcmp( pszLastname, "AuthUserName" ) == 0 ) {
		mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_USER_NAME, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		return 1;
	} else if( strcmp( pszLastname, "AuthPassword" ) == 0 ) {
		mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__AUTH_PASSWORD, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		return 1;
	} else if( strcmp( pszLastname, "URI" ) == 0 ) {
		mib_set_type2( MIB_VOICE_PROFILE__LINE__SIP__URI, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		return 1;
	} else {
		return ERR_9005;
	}

	return 0;
}

static int getVoiceProfileLineEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		enable_t Enable;
		char DirectoryNumber[ 32 ];
		line_status_t Status;
	} s;
	
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;

	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Enable );
		*data = strdup_Enable( s.Enable );
	} else if( strcmp( pszLastname, "DirectoryNumber" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   s.DirectoryNumber );
		*data = strdup( s.DirectoryNumber );
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		mib_get_type2( MIB_VOICE_PROFILE__LINE__STATUS, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Status );
		*data = strdup_LineStatus( s.Status );
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

static int setVoiceProfileLineEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP.Line.{i}. */
	unsigned int nVoiceProfileInstNum;
	unsigned int nSipLineInstNum;
	unsigned int nNumberOfLine;
	union {
		enable_t Enable;
		char DirectoryNumber[ 32 ];
		line_status_t Status;
	} s;
	
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile_Line( name, &nVoiceProfileInstNum, &nSipLineInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	if( !mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &nNumberOfLine ) )
		return -1;
	
	if( -- nSipLineInstNum >= nNumberOfLine )	/* convert to zero based */
		return -1;
	
	if( entity ->type != type )
		return ERR_9006;

	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		str2id_Enable( data, &s.Enable );
		mib_set_type2( MIB_VOICE_PROFILE__LINE__ENABLE, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   &s.Enable );
		return 1;
	} else if( strcmp( pszLastname, "DirectoryNumber" ) == 0 ) {
		mib_set_type2( MIB_VOICE_PROFILE__LINE__DIRECTORY_NUMBER, 
					   nVoiceProfileInstNum, nSipLineInstNum,
					   data );
		return 1;
	} else if( strcmp( pszLastname, "Status" ) == 0 ) {
		return ERR_9008;
	} else {
		return ERR_9005;
	}

	return 0;
}

int objVoiceProfileLine(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		int num=0,i;
		struct sCWMP_ENTITY **c = (struct sCWMP_ENTITY **)data;
		//MIB_CE_IP_ROUTE_T *p,route_entity;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;

		num = 1;//mib_chain_total( MIB_IP_ROUTE_TBL );
		for( i=0; i<num;i++ )
		{
			//p = &route_entity;
			//if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ) )
			//	continue;
			
			//if( p->InstanceNum==0 ) //maybe createn by web or cli
			//{
			//	MaxInstNum++;
			//	p->InstanceNum = MaxInstNum;
			//	mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)p, i );
			//}else
			//	MaxInstNum = p->InstanceNum;
			
			if( create_Object( c, tVoiceProfileLine, sizeof(tVoiceProfileLine), MAX_LINE_PER_PROFILE, 1 ) < 0 )
				return -1;
			//c = & (*c)->sibling;
		}
		add_objectNum( name, 1 );
		return 0;
	}
#if 0
	// TODO: Not implement add/del/update yet.  
	case eCWMP_tADDOBJ:
	     {
	     	int ret;
	     	if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;
		ret = add_Object( name, &entity->next_table,  tForwarding, sizeof(tForwarding), data );
		if( ret >= 0 )
		{
			MIB_CE_IP_ROUTE_T fentry;
			memset( &fentry, 0, sizeof( MIB_CE_IP_ROUTE_T ) );
			fentry.InstanceNum = *(unsigned int*)data;
			fentry.FWMetric = -1;
			fentry.ifIndex = 0xff;
			mib_chain_add( MIB_IP_ROUTE_TBL, (unsigned char*)&fentry );
		}
		return ret;
	     }
	case eCWMP_tDELOBJ:
	     {
	     	int ret, id;
	     	
	     	if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;
	     	
	     	id = getChainID( entity->next_table, *(int*)data  );
	     	if(id==-1) return ERR_9005;
	     	mib_chain_delete( MIB_IP_ROUTE_TBL, id );	
		ret = del_Object( name, &entity->next_table, *(int*)data );
		if( ret == 0 ) return 1;
		return ret;
	     }
	case eCWMP_tUPDATEOBJ:	
	     {     	int num=0,i;
	     	struct sCWMP_ENTITY *old_table;
	     	num = mib_chain_total( MIB_IP_ROUTE_TBL );
	     	old_table = entity->next_table;
	     	entity->next_table = NULL;
	     	for( i=0; i<num;i++ )
	     	{
	     		struct sCWMP_ENTITY *remove_entity=NULL;
			MIB_CE_IP_ROUTE_T *p,route_entity;

			p = &route_entity;
			if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)p ) )
				continue;			
			remove_entity = remove_SiblingEntity( &old_table, p->InstanceNum );
			if( remove_entity!=NULL )
			{
				add_SiblingEntity( &entity->next_table, remove_entity );
			}else{ 
				unsigned int MaxInstNum=p->InstanceNum;			
				add_Object( name, &entity->next_table,  tForwarding,     							sizeof(tForwarding), &MaxInstNum );
				if(MaxInstNum!=p->InstanceNum)
				{     p->InstanceNum = MaxInstNum;
				      mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)p, i );
				}
			}	
	     	}
	     	if( old_table ) 	destroy_ParameterTable( old_table );	     	
	     	return 0;
	     }
#endif
	}
	return -1;
}
#endif
