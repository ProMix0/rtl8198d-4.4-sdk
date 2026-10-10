#include "cwmp_main_tr104.h"
#include "prmt_limit.h"
#include "prmt_voice_profile.h"
#include "prmt_voice_profile_line.h"
#include "mib_def.h"
#include "mib_tr104.h"
#include "str_utility.h"
#include "str_mib.h"
#include "cwmpevt.h"
//******* VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo. EntityLEAF*******
struct CWMP_OP tVPServiceProviderEntityLeafOP = { getVPServiceProviderEntity, setVPServiceProviderEntity};

struct CWMP_PRMT tVPServiceProviderEntityLeafInfo[] =
{
    /*(name,   type,           flag,                  op)*/
	{ "Name",  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPServiceProviderEntityLeafOP},
};

enum eVPServiceProviderEntityLeaf
{
    eVPServiceProviderName
};

struct CWMP_LEAF tVPServiceProviderEntityLeaf[] =
{
    { &tVPServiceProviderEntityLeafInfo[eVPServiceProviderName]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.SIP. EntityLEAF*******
struct CWMP_OP tVPSIPEntityLeafOP = { getVPSipEntity, setVPSipEntity};

struct CWMP_PRMT tVPSIPEntityLeafInfo[] =
{
    /*(name,                                       type,           flag,                  op)*/
	{ "ProxyServer",                               eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "ProxyServerPort",                           eCWMP_tUINT,   		CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "ProxyServerTransport",                      eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegistrarServer",                           eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegistrarServerPort",                       eCWMP_tUINT,    	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegistrarServerTransport",                  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "UserAgentDomain",                           eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "UserAgentPort",                             eCWMP_tUINT,    		CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "UserAgentTransport",                        eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "OutboundProxy",                             eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "OutboundProxyPort",                         eCWMP_tUINT,    	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "Organization",                              eCWMP_tSTRING,  	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegistrationPeriod",                        eCWMP_tUINT,    	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "VLANIDMark",                                eCWMP_tINT,     		CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "DSCPMark",                                  eCWMP_tUINT,    		CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegisterRetryInterval",                     eCWMP_tUINT,    	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "RegisterExpires",                           eCWMP_tUINT,    		CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
#ifdef CONFIG_CU		
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServer", 			 eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerPort", 		 eCWMP_tUINT,	 CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerTransport",	 eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServer", 		 eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerPort", 	 eCWMP_tUINT,	 CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerTransport", eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,	&tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServer", 		   eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServerPort", 	   eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
#else
	{ "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServer",              eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerPort",          eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerTransport",     eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServer",          eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerPort",      eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerTransport", eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxy",            eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxyPort",        eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
#endif
	{ "X_Default_Proxy_Index",				eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_802-1pMark",                       eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_HeartbeatSwitch",                  eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_HeartbeatCycle",                   eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_HeartbeatCount",                   eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "InviteExpires",               eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
#ifdef CONFIG_CU
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryInviteExpires",				eCWMP_tINT, 	CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrationPeriod",		   eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterRetryInterval",					  eCWMP_tUINT,	  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterExpires", 						  eCWMP_tUINT,	  CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},		
#else
	{ "X_"CTCOM_NAME_PREFIX"_Standby-InviteExpires",               eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegistrationPeriod", 		   eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegisterRetryInterval",                     eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Standby-RegisterExpires",                           eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
#endif
	{ "X_"CTCOM_NAME_PREFIX"_SessionUpdateTimer",               eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	{ "CallIdTimePolicy",               eCWMP_tUINT,     CWMP_READ|CWMP_WRITE,  &tVPSIPEntityLeafOP},
	
};

enum eVPSIPEntityLeaf
{
    eSIPProxyServer,
    eSIPProxyServerPort,
    eSIPProxyServerTransport,
    eSIPRegistrarServer,
    eSIPRegistrarServerPort,
    eSIPRegistrarServerTransport,
    eSIPUserAgentDomain,
    eSIPUserAgentPort,
    eSIPUserAgentTransport,
    eSIPOutboundProxy,
    eSIPOutboundProxyPort,
    eSIPOrganization,
    eSIPRegistrationPeriod,
	eSIPVLANIDMark,
	eSIPDSCPMark,
	eSIPRegisterRetryInterval,
	eSIPRegisterExpires,
	eSIPXCT_STB_ProxyServer,
	eSIPXCT_STB_ProxyServerPort,
	eSIPXCT_STB_ProxyServerTransport,
	eSIPXCT_STB_RegistrarServer,
	eSIPXCT_STB_RegistrarServerPort,
	eSIPXCT_STB_RegistrarServerTransport,
	eSIPXCT_STB_OutboundProxy,
	eSIPXCT_STB_OutboundProxyPort,
	eSIPXCT_STB_Default_Proxy_Index,
	eSIPXCT_STB_802_1pMark,
	eSIPXCT_HeartbeatSwitch,
	eSIPXCT_HeartbeatCycle,
	eSIPXCT_HeartbeatCount,
	eSIPInviteExpires,
	eSIPXCT_STB_InviteExpires,
	eSIPXCT_STB_RegistrationPeriod,
	eSIPXCT_STB_RegisterRetryInterval,
	eSIPXCT_STB_RegisterExpires,
	eSIPXCT_SessionUpdateTimer,
	eSIPXCT_CallIdTimePolicy,
};

struct CWMP_LEAF tVPSIPEntityLeaf[] =
{
    { &tVPSIPEntityLeafInfo[eSIPProxyServer]  },
    { &tVPSIPEntityLeafInfo[eSIPProxyServerPort]  },
    { &tVPSIPEntityLeafInfo[eSIPProxyServerTransport]  },
    { &tVPSIPEntityLeafInfo[eSIPRegistrarServer]  },
    { &tVPSIPEntityLeafInfo[eSIPRegistrarServerPort]  },
    { &tVPSIPEntityLeafInfo[eSIPRegistrarServerTransport]  },
    { &tVPSIPEntityLeafInfo[eSIPUserAgentDomain]  },
    { &tVPSIPEntityLeafInfo[eSIPUserAgentPort]  },
    { &tVPSIPEntityLeafInfo[eSIPUserAgentTransport]  },
    { &tVPSIPEntityLeafInfo[eSIPOutboundProxy]  },
    { &tVPSIPEntityLeafInfo[eSIPOutboundProxyPort]  },
    { &tVPSIPEntityLeafInfo[eSIPOrganization]  },
    { &tVPSIPEntityLeafInfo[eSIPRegistrationPeriod]  },
    { &tVPSIPEntityLeafInfo[eSIPVLANIDMark]  },
    { &tVPSIPEntityLeafInfo[eSIPDSCPMark]  },
    { &tVPSIPEntityLeafInfo[eSIPRegisterRetryInterval]  },
    { &tVPSIPEntityLeafInfo[eSIPRegisterExpires]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_ProxyServer]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_ProxyServerPort]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_ProxyServerTransport]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegistrarServer]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegistrarServerPort]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegistrarServerTransport]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_OutboundProxy]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_OutboundProxyPort]  },   
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_Default_Proxy_Index]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_802_1pMark]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_HeartbeatSwitch]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_HeartbeatCycle]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_HeartbeatCount]  },
    { &tVPSIPEntityLeafInfo[eSIPInviteExpires]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_InviteExpires]  },    
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegistrationPeriod]  },     
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegisterRetryInterval]  },
    { &tVPSIPEntityLeafInfo[eSIPXCT_STB_RegisterExpires]  },    
    { &tVPSIPEntityLeafInfo[eSIPXCT_SessionUpdateTimer]  },    
    { &tVPSIPEntityLeafInfo[eSIPXCT_CallIdTimePolicy]  },   
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.SIP.ResponseMap.{i} EntityLEAF *******
struct CWMP_OP tVPSIPResponseMapEntityLeafOP = { getVPSIPResponseMapEntity, setVPSIPResponseMapEntity };
struct CWMP_PRMT tVPSIPResponseMapEntityLeafInfo[] =
{
    /*(name,                                        type,           flag,                   op)*/
    { "SIPResponseNumber",                          eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,   &tVPSIPResponseMapEntityLeafOP},
	{ "Tone",                                       eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,   &tVPSIPResponseMapEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_Duration",             eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,   &tVPSIPResponseMapEntityLeafOP}
};

enum eVPSIPResponseMapEntityLeaf
{
	eSIPRM_SIPResponseNumber,
	eSIPRM_Tone,
	eSIPRM_XCT_Duration
};

struct CWMP_LEAF tVPSIPResponseMapEntityLeaf[] =
{
    { &tVPSIPResponseMapEntityLeafInfo[eSIPRM_SIPResponseNumber]  },
    { &tVPSIPResponseMapEntityLeafInfo[eSIPRM_Tone]  },
#ifdef CONFIG_CU
    { &tVPSIPResponseMapEntityLeafInfo[eSIPRM_XCT_Duration]  },
#endif
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.SIP EntityObject *******
struct CWMP_PRMT tVPSIPEntityObjectInfo[] =
{
    /*(name,                type,               flag,               op)*/
    {"ResponseMap",         eCWMP_tOBJECT,      CWMP_READ,          NULL}
};

enum eVPSIPEntityObject
{
	eSIP_ResponseMap
};

struct CWMP_NODE tVSIPEntityObject[] =
{
    /*info,                                             leaf,                           next)*/
    {&tVPSIPEntityObjectInfo[eSIP_ResponseMap],         tVPSIPResponseMapEntityLeaf,    NULL},
    {NULL,                                              NULL,                           NULL}
};

//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104

    //******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248. EntityLEAF*******
struct CWMP_OP tVPH248EntityLeafOP = { getVPH248Entity, setVPH248Entity};

struct CWMP_PRMT tVPH248EntityLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
	{ "MediaGatewayControler",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
	{ "MediaGatewayControlerPort",              eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
	{ "Standby-MediaGatewayControler",          eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "Standby-MediaGatewayControlerPort",      eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "MediaGatewayPort",                       eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "MessageEncodingType",                    eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "DeviceID",                               eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "DeviceIDType",                           eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "PhysicalTermID",                         eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "PhysicalTermIDConfigMethod",             eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "PhysicalTermIDPrefix",                   eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "PhysicalTermIDAddLen",                   eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RTPPrefix",                              eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "EphemeralTermIDAddLen",                  eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "EphemeralTermIDUniform",                 eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "EphemeralTermIDStart",                   eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "ThreeHandShake",                         eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "LongTimer",                              eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "PendingTimerInit",                       eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RetranIntervalTimer",                    eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "MaxRetranCount",                         eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RetransmissionTime",                     eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RetransmissionCycle",                    eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "HeartbeatMode",                          eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "HeartbeatCycle",                         eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "HeartbeatCount",                         eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RegisterCount",                          eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "RegisterCycle",                          eCWMP_tINT,             CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "MandatoryRegister",                      eCWMP_tBOOLEAN,         CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "AuthenticationMethID",                   eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    /* H.248 CU */
    { "Domain",                                 eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "MIDFormat",                              eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "CallAgent1",                             eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "CallAgent2",                             eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "LineName",                               eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "CallAgentPort1",                         eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "CallAgentPort2",                         eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "LocalPort",                              eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "DeviceName",                             eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "DSCPMark",                               eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "SoftswitchVendor",                       eCWMP_tUINT,            CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
    { "SoftswitchVersion",                      eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPH248EntityLeafOP},
};

enum eVPH248EntityLeaf
{
	eH248MediaGatewayControler,
	eH248MediaGatewayControlerPort,
	eH248Standby_MediaGatewayControler,
    eH248Standby_MediaGatewayControlerPort,
    eH248MediaGatewayPort,
    eH248MessageEncodingType,
    eH248DeviceID,
    eH248DeviceIDType,
    eH248PhysicalTermID,
    eH248PhysicalTermIDConfigMethod,
    eH248PhysicalTermIDPrefix,
    eH248PhysicalTermIDAddLen,
    eH248RTPPrefix,
    eH248EphemeralTermIDAddLen,
    eH248EphemeralTermIDUniform,
    eH248EphemeralTermIDStart,
    eH248ThreeHandShake,
    eH248LongTimer,
    eH248PendingTimerInit,
    eH248RetranIntervalTimer,
    eH248MaxRetranCount,
    eH248RetransmissionTime,
    eH248RetransmissionCycle,
    eH248HeartbeatMode,
    eH248HeartbeatCycle,
    eH248HeartbeatCount,
    eH248RegisterCount,
    eH248RegisterCycle,
    eH248MandatoryRegister,
    eH248AuthenticationMethID,
    /* H.248 CU */
    eH248Domain,
    eH248MIDFormat,
    eH248CallAgent1,
    eH248CallAgent2,
    eH248LineName,
    eH248CallAgentPort1,
    eH248CallAgentPort2,
    eH248LocalPort,
    eH248DeviceName,
    eH248DSCPMark,
    eH248SoftswitchVendor,
    eH248SoftswitchVersion
};

struct CWMP_LEAF tVPH248EntityLeaf[] =
{
    { &tVPH248EntityLeafInfo[eH248MediaGatewayControler]  },
    { &tVPH248EntityLeafInfo[eH248MediaGatewayControlerPort]  },
    { &tVPH248EntityLeafInfo[eH248Standby_MediaGatewayControler]  },
    { &tVPH248EntityLeafInfo[eH248Standby_MediaGatewayControlerPort]  },
    { &tVPH248EntityLeafInfo[eH248MediaGatewayPort]  },
    { &tVPH248EntityLeafInfo[eH248MessageEncodingType]  },
    { &tVPH248EntityLeafInfo[eH248DeviceID]  },
    { &tVPH248EntityLeafInfo[eH248DeviceIDType]  },
    { &tVPH248EntityLeafInfo[eH248PhysicalTermID]  },
    { &tVPH248EntityLeafInfo[eH248PhysicalTermIDConfigMethod]  },
    { &tVPH248EntityLeafInfo[eH248PhysicalTermIDPrefix]  },
    { &tVPH248EntityLeafInfo[eH248PhysicalTermIDAddLen]  },
    { &tVPH248EntityLeafInfo[eH248RTPPrefix]  },
    { &tVPH248EntityLeafInfo[eH248EphemeralTermIDAddLen]  },
    { &tVPH248EntityLeafInfo[eH248EphemeralTermIDUniform]  },
    { &tVPH248EntityLeafInfo[eH248EphemeralTermIDStart]  },
    { &tVPH248EntityLeafInfo[eH248ThreeHandShake]  },
    { &tVPH248EntityLeafInfo[eH248LongTimer]  },
    { &tVPH248EntityLeafInfo[eH248PendingTimerInit]  },
    { &tVPH248EntityLeafInfo[eH248RetranIntervalTimer]  },
    { &tVPH248EntityLeafInfo[eH248MaxRetranCount]  },
    { &tVPH248EntityLeafInfo[eH248RetransmissionTime]  },
    { &tVPH248EntityLeafInfo[eH248RetransmissionCycle]  },
    { &tVPH248EntityLeafInfo[eH248HeartbeatMode]  },
    { &tVPH248EntityLeafInfo[eH248HeartbeatCycle]  },
    { &tVPH248EntityLeafInfo[eH248HeartbeatCount]  },
    { &tVPH248EntityLeafInfo[eH248RegisterCount]  },
    { &tVPH248EntityLeafInfo[eH248RegisterCycle]  },
    { &tVPH248EntityLeafInfo[eH248MandatoryRegister]  },
    { &tVPH248EntityLeafInfo[eH248AuthenticationMethID]  },
#ifdef CONFIG_CU
    /* H.248 CU */
    { &tVPH248EntityLeafInfo[eH248Domain]  },
    { &tVPH248EntityLeafInfo[eH248MIDFormat]  },
    { &tVPH248EntityLeafInfo[eH248CallAgent1]  },
    { &tVPH248EntityLeafInfo[eH248CallAgent2]  },
    { &tVPH248EntityLeafInfo[eH248LineName]  },
    { &tVPH248EntityLeafInfo[eH248CallAgentPort1]  },
    { &tVPH248EntityLeafInfo[eH248CallAgentPort2]  },
    { &tVPH248EntityLeafInfo[eH248LocalPort]  },
    { &tVPH248EntityLeafInfo[eH248DeviceName]  },
    { &tVPH248EntityLeafInfo[eH248DSCPMark]  },
    { &tVPH248EntityLeafInfo[eH248SoftswitchVendor]  },
    { &tVPH248EntityLeafInfo[eH248SoftswitchVersion]  },
#endif
    { NULL }
};

struct CWMP_OP tVPHWProfileLeafOP = { getVPHWH248Leaf, setVPHWH248Leaf};

struct CWMP_PRMT tVPHWProfileLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
	{ "ProfileBody",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPHWProfileLeafOP},
};

enum eVPHWH248ProfileLeaf
{
	eHWH248ProfileBody,
};

struct CWMP_LEAF tVPHWProfileLeaf[] =
{
    { &tVPHWProfileLeafInfo[eHWH248ProfileBody]  },
    { NULL }
};


struct CWMP_PRMT tVPHWH248ObjectInfo[] =
{
/*(name,			type,		flag,			op)*/
{"Profile",		eCWMP_tOBJECT,	CWMP_READ,		NULL},
};

enum eVPHWH248Object
{
	eVPHWProfile,
};

struct CWMP_NODE tVPHWH248Object[] =
{
/*info,  						leaf,		next)*/
{ &tVPHWH248ObjectInfo[eVPHWProfile],		tVPHWProfileLeaf,		NULL },
{ NULL,							NULL,		NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.auth. *******
struct CWMP_OP tVPAuthLeafOP = { getVPAuthEntity, setVPAuthEntity};

struct CWMP_PRMT tVPAuthLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
	{ "auth",                  eCWMP_tBOOLEAN,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
	{ "authInitKey",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
	{ "authmgid",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
	{ "authHeader",                  eCWMP_tBOOLEAN,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
	{ "HeaderSecurityType",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
	{ "Rfc2833EncryptKey",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPAuthLeafOP},
};

enum eVPAuthLeaf
{
	eH248auth,
	eH248authInitKey,
	eH248authmgid,
	eH248authHeader,
	eH248HeaderSecurityType,
	eH248Rfc2833EncryptKey,
};

struct CWMP_LEAF tVPAuthLeaf[] =
{
    { &tVPAuthLeafInfo[eH248auth]  },
    { &tVPAuthLeafInfo[eH248authInitKey]  },
    { &tVPAuthLeafInfo[eH248authmgid]  },
    { &tVPAuthLeafInfo[eH248authHeader]  },
    { &tVPAuthLeafInfo[eH248HeaderSecurityType]  },
    { &tVPAuthLeafInfo[eH248Rfc2833EncryptKey]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.StackInfo. *******
struct CWMP_OP tVPStackInfoLeafOP = { getVPStackInfoEntity, setVPStackInfoEntity};

struct CWMP_PRMT tVPStackInfoLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
    { "MsgFormat",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "ResponseAckCtrl",                  eCWMP_tBOOLEAN,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "LongTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "RetranIntervalTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "MaxRetranCount",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "RetransmissionTime",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "RetransmissionCycle",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "RegisterCycle",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "MGProvisionalRespTime",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "MTUValue",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
	{ "MsgSegmentation",                  eCWMP_tBOOLEAN,          CWMP_READ|CWMP_WRITE,  &tVPStackInfoLeafOP},
};

enum eVPStackInfoLeaf
{
    eStackInfoMsgFormat,
	eStackInfoResponseAckCtrl,
	eStackInfoLongTimer,
	eStackInfoRetranIntervalTimer,
	eStackInfoMaxRetranCount,
	eStackInfoRetransmissionTime,
	eStackInfoRetransmissionCycle,
	eStackInfoRegisterCycle,
	eStackInfoMGProvisionalRespTime,
	eStackInfoMTUValue,
	eStackInfoMsgSegmentation,
};

struct CWMP_LEAF tVPStackInfoLeaf[] =
{
    { &tVPStackInfoLeafInfo[eStackInfoMsgFormat]  },
    { &tVPStackInfoLeafInfo[eStackInfoResponseAckCtrl]  },
    { &tVPStackInfoLeafInfo[eStackInfoLongTimer]  },
    { &tVPStackInfoLeafInfo[eStackInfoRetranIntervalTimer]  },
    { &tVPStackInfoLeafInfo[eStackInfoMaxRetranCount]  },
    { &tVPStackInfoLeafInfo[eStackInfoRetransmissionTime]  },
    { &tVPStackInfoLeafInfo[eStackInfoRetransmissionCycle]  },
    { &tVPStackInfoLeafInfo[eStackInfoRegisterCycle]  },
    { &tVPStackInfoLeafInfo[eStackInfoMGProvisionalRespTime]  },
    { &tVPStackInfoLeafInfo[eStackInfoMTUValue]  },
    { &tVPStackInfoLeafInfo[eStackInfoMsgSegmentation]  },
    { NULL }
    
};

//******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Digitmap. *******
struct CWMP_OP tVPDigitmapLeafOP = { getVPDigitmapEntity, setVPDigitmapEntity};

struct CWMP_PRMT tVPDigitmapLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
    { "DMName",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPDigitmapLeafOP},
    { "DigitMap",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPDigitmapLeafOP},
    { "DigitMapStartTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPDigitmapLeafOP},
    { "DigitMapShortTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPDigitmapLeafOP},
    { "DigitMapLongTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPDigitmapLeafOP},
};

enum eVPDigitmapLeaf
{
    eDigitmapDMName,
	eDigitmapDigitMap,
	eDigitmapDigitMapStartTimer,
	eDigitmapDigitMapShortTimer,
	eDigitmapDigitMapLongTimer,
};

struct CWMP_LEAF tVPDigitmapLeaf[] =
{
    { &tVPDigitmapLeafInfo[eDigitmapDMName]  },
    { &tVPDigitmapLeafInfo[eDigitmapDigitMap]  },
    { &tVPDigitmapLeafInfo[eDigitmapDigitMapStartTimer]  },
    { &tVPDigitmapLeafInfo[eDigitmapDigitMapShortTimer]  },
    { &tVPDigitmapLeafInfo[eDigitmapDigitMapLongTimer]  },
    { NULL }
    
};

//******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Extend. *******
struct CWMP_OP tVPExtendLeafOP = { getVPExtendEntity, setVPExtendEntity};

struct CWMP_PRMT tVPExtendLeafInfo[] =
{
    /*(name,                                    type,                   flag,                  op)*/
    { "HeartbeatMode",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "HeartBeatTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "HeartBeatRetransTimer",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "HeartBeatRetransTimes",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "MWDBaseTime",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "MWD",                  eCWMP_tUINT,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
    { "MgcSwitchMethod",                  eCWMP_tSTRING,          CWMP_READ|CWMP_WRITE,  &tVPExtendLeafOP},
};

enum eVPExtendLeaf
{
    eExtendHeartbeatMode,
    eExtendHeartBeatTimer,
    eExtendHeartBeatRetransTimer,
    eExtendHeartBeatRetransTimes,
    eExtendMWDBaseTime,
    eExtendMWD,
    eExtendMgcSwitchMethod,
};

struct CWMP_LEAF tVPExtendLeaf[] =
{
    { &tVPExtendLeafInfo[eExtendHeartbeatMode]  },
    { &tVPExtendLeafInfo[eExtendHeartBeatTimer]  },
    { &tVPExtendLeafInfo[eExtendHeartBeatRetransTimer]  },
    { &tVPExtendLeafInfo[eExtendHeartBeatRetransTimes]  },
    { &tVPExtendLeafInfo[eExtendMWDBaseTime]  },
    { &tVPExtendLeafInfo[eExtendMWD]  },
    { &tVPExtendLeafInfo[eExtendMgcSwitchMethod]  },
    { NULL }
    
};

struct CWMP_PRMT tVPH248ObjectInfo[] =
{
/*(name,			type,		flag,			op)*/
{"auth",		eCWMP_tOBJECT,	CWMP_READ,		NULL},
{"StackInfo",		eCWMP_tOBJECT,	CWMP_READ,		NULL},
{"Digitmap",		eCWMP_tOBJECT,	CWMP_READ,		NULL},
{"Extend",		eCWMP_tOBJECT,	CWMP_READ,		NULL},
};

enum eVPH248Object
{
	eVPAuth,
	eVPStackInfo,
	eVPDigitmap,
	eVPExtend,
};

struct CWMP_NODE tVPH248Object[] =
{
/*info,  						leaf,		next)*/
{ &tVPH248ObjectInfo[eVPAuth],		tVPAuthLeaf,		NULL },
{ &tVPH248ObjectInfo[eVPStackInfo],		tVPStackInfoLeaf,		NULL },
{ &tVPH248ObjectInfo[eVPDigitmap],		tVPDigitmapLeaf,		NULL },
{ &tVPH248ObjectInfo[eVPExtend],		tVPExtendLeaf,		NULL },
{ NULL,							NULL,		NULL }
};
#endif 

//******* VoiceService.{i}.VoiceProfile.{i}.RTP.RTCP EntityLEAF*******
struct CWMP_OP tVPRtpRtcpEntityLeafOP = { getVPRtpRtcpEntity, setVPRtpRtcpEntity};
struct CWMP_PRMT tVPRtpRtcpEntityLeafInfo[] =
{
    /*(name,                                type,           flag,                  op)*/
	{ "Enable",                             eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPRtpRtcpEntityLeafOP},
	{ "TxRepeatInterval",                   eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRtpRtcpEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_RtcpxrFlag",   eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPRtpRtcpEntityLeafOP}
};

enum eVPRtpRtcpEntityLeaf
{
	eVPRtpRtcpEnable,
    eVPRtpRtcpTxRepeatInterval,
    eVPRtpRtcpXCT_RtcpxrFlag
};

struct CWMP_LEAF tVPRtpRtcpEntityLeaf[] =
{
    { &tVPRtpRtcpEntityLeafInfo[eVPRtpRtcpEnable]  },
    { &tVPRtpRtcpEntityLeafInfo[eVPRtpRtcpTxRepeatInterval]  },
#ifdef CONFIG_CU
    { &tVPRtpRtcpEntityLeafInfo[eVPRtpRtcpXCT_RtcpxrFlag]  },
#endif
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.RTP.Redundancy EntityLEAF*******
struct CWMP_OP tVPRtpRedundancyEntityLeafOP = { getVPRtpRedundancyEntity, setVPRtpRedundancyEntity};
struct CWMP_PRMT tVPRtpRedundancyEntityLeafInfo[] =
{
    /*(name,                                        type,           flag,                  op)*/
	{ "Enable",                                     eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPRtpRedundancyEntityLeafOP},
	{ "PayloadType",                                eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRtpRedundancyEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_EnableFixedStart",     eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPRtpRedundancyEntityLeafOP}
};

enum eVPRtpRedundancyEntityLeaf
{
	eVPRtpRedundancyEnable,
    eVPRtpRedundancyPayloadType,
    eVPRtpRedundancyXCT_EnableFixedStart
};

struct CWMP_LEAF tVPRtpRedundancyEntityLeaf[] =
{
    { &tVPRtpRedundancyEntityLeafInfo[eVPRtpRedundancyEnable]  },
    { &tVPRtpRedundancyEntityLeafInfo[eVPRtpRedundancyPayloadType]  },
#ifdef CONFIG_CU
    { &tVPRtpRedundancyEntityLeafInfo[eVPRtpRedundancyXCT_EnableFixedStart]  },
#endif
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.RTP. EntityLEAF*******
struct CWMP_OP tVPRTPEntityLeafOP = { getVPRTPEntity, setVPRTPEntity};
struct CWMP_PRMT tVPRTPEntityLeafInfo[] =
{
    /*(name,                 type,           flag,                  op)*/
	{ "VLANIDMark",          eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
	{ "DSCPMark",            eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_802-1pMark",   eCWMP_tINT,     CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
    { "LocalPortMin",                       eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
    { "LocalPortMax",                       eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
    { "TelephoneEventPayloadType",          eCWMP_tUINT,    CWMP_READ|CWMP_WRITE,  &tVPRTPEntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_WANName",      eCWMP_tSTRING,  CWMP_READ,             &tVPRTPEntityLeafOP},
};

enum eVPRTPEntityLeaf
{
	eRTPVLANIDMark,
	eRTPDSCPMark,
	eRTPXCT_STB_802_1pMark,
    eRTPLocalPortMin,
    eRTPLocalPortMax,
    eRTPTelephoneEventPayloadType,
    eRTPXCT_WANName,
};

struct CWMP_LEAF tVPRTPEntityLeaf[] =
{
    { &tVPRTPEntityLeafInfo[eRTPVLANIDMark]  },
    { &tVPRTPEntityLeafInfo[eRTPDSCPMark]  },
    { &tVPRTPEntityLeafInfo[eRTPXCT_STB_802_1pMark]  },
    { &tVPRTPEntityLeafInfo[eRTPLocalPortMin]  },
    { &tVPRTPEntityLeafInfo[eRTPLocalPortMax]  },
    { &tVPRTPEntityLeafInfo[eRTPTelephoneEventPayloadType]  },
#ifdef CONFIG_CU
    { &tVPRTPEntityLeafInfo[eRTPXCT_WANName]  },
#endif
    { NULL }
};

//*******  VoiceService.{i}.VoiceProfile.{i}.RTP. EntityObject *******
//struct CWMP_OP tVPRTPEntityOP = { NULL, objVoiceProfileRTP };
struct CWMP_PRMT tVPRTPEntityObjectInfo[] =
{
    /*(name,            type,           flag,		op)*/
    {"RTCP",            eCWMP_tOBJECT,  CWMP_READ,  NULL},
    {"Redundancy",      eCWMP_tOBJECT,  CWMP_READ,	NULL}
};

enum eVPRTPEntityObject
{
    eVPRtpRtcp,
    eVPRtpPRedundancy
};

struct CWMP_NODE tVPRTPEntityObject[] =
{
    /*(info,                                        leaf,                           next)*/
    {&tVPRTPEntityObjectInfo[eVPRtpRtcp],           tVPRtpRtcpEntityLeaf,           NULL},
    {&tVPRTPEntityObjectInfo[eVPRtpPRedundancy],    tVPRtpRedundancyEntityLeaf,     NULL},    
    {NULL,                                          NULL,                           NULL}
};

//******* VoiceService.{i}.VoiceProfile.{i}.NumberingPlan. EntityLEAF*******
struct CWMP_OP tVPNumberingPlanEntityLeafOP = { getVPNumberingPlanEntity, setVPNumberingPlanEntity};
struct CWMP_PRMT tVPNumberingPlanEntityLeafInfo[] =
{
    /*(name,                 type,           flag,                  op)*/
	{ "X_"CTCOM_NAME_PREFIX"_NumberPlan", eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPNumberingPlanEntityLeafOP},
};

enum eVPNumberingPlanEntityLeaf
{
	eXCT_NumberPlan
};

struct CWMP_LEAF tVPNumberingPlanEntityLeaf[] =
{
    { &tVPNumberingPlanEntityLeafInfo[eXCT_NumberPlan]  },
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.FaxT38. EntityLEAF*******
struct CWMP_OP tVPFaxT38EntityLeafOP = { getVPFaxT38Entity, setVPFaxT38Entity};
struct CWMP_PRMT tVPFaxT38EntityLeafInfo[] =
{
    /*(name,    type,           flag,                  op)*/
	{ "Enable",                                 eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
    { "BitRate",                                eCWMP_tUINT,        CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
    { "TCFMethod",                              eCWMP_tSTRING,      CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
    { "HighSpeedRedundancy",                    eCWMP_tUINT,        CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
    { "LowSpeedRedundancy",                     eCWMP_tUINT,        CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
    { "X_"CTCOM_NAME_PREFIX"_PortAdd2",         eCWMP_tBOOLEAN,     CWMP_READ|CWMP_WRITE,  &tVPFaxT38EntityLeafOP},
};

enum eVPFaxT38EntityLeaf
{
	eVPT38ENABLE,
    eVPBitRate,
    eVPTCFMethod,
    eVPHighSpeedRedundancy,
    eVPLowSpeedRedundancy,
    eVPXCT_PortAdd2,
};

struct CWMP_LEAF tVPFaxT38EntityLeaf[] =
{
    { &tVPFaxT38EntityLeafInfo[eVPT38ENABLE]  },
    { &tVPFaxT38EntityLeafInfo[eVPBitRate]  },
    { &tVPFaxT38EntityLeafInfo[eVPTCFMethod]  },
    { &tVPFaxT38EntityLeafInfo[eVPHighSpeedRedundancy]  },
    { &tVPFaxT38EntityLeafInfo[eVPLowSpeedRedundancy]  },
#ifdef CONFIG_CU
    { &tVPFaxT38EntityLeafInfo[eVPXCT_PortAdd2]  },
#endif
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_G711FAX. EntityLEAF*******
struct CWMP_OP tVPXCT_G711FAXEntityLeafOP = { getVPXCT_G711FAXEntity, setVPXCT_G711FAXEntity};
struct CWMP_PRMT tVPXCT_G711FAXEntityLeafInfo[] =
{
    /*(name,         type,           flag,                  op)*/
	{ "Enable",      eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE,  &tVPXCT_G711FAXEntityLeafOP},
	{ "ControlType", eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE,  &tVPXCT_G711FAXEntityLeafOP}
};

enum eVPXCT_G711FAXEntityLeaf
{
	eXCT711FAXEnable,
    eXCT711FAXControlType
};

struct CWMP_LEAF tVPXCT_G711FAXEntityLeaf[] =
{
    { &tVPXCT_G711FAXEntityLeafInfo[eXCT711FAXEnable]  },
    { &tVPXCT_G711FAXEntityLeafInfo[eXCT711FAXControlType]  },
    { NULL }
};


//******* VoiceService.{i}.VoiceProfile.X_"CTCOM_NAME_PREFIX"_IADDiagnostics EntityLEAF*******
struct CWMP_OP tVPXCT_IADDiagEntityLeafOP = { getVPXCT_IADDiagEntity, setVPXCT_IADDiagEntity };
struct CWMP_PRMT tVPXCT_IADDiagEntityLeafInfo[] =
{
    /*(name,                 type,           flag,                 op)*/
	{ "IADDiagnosticsState", eCWMP_tSTRING, CWMP_READ|CWMP_WRITE,  &tVPXCT_IADDiagEntityLeafOP},
	{ "TestServer",          eCWMP_tUINT,   CWMP_READ|CWMP_WRITE,  &tVPXCT_IADDiagEntityLeafOP},
	{ "RegistResult",        eCWMP_tUINT,   CWMP_READ,             &tVPXCT_IADDiagEntityLeafOP},
	{ "Reason",              eCWMP_tUINT,   CWMP_READ,             &tVPXCT_IADDiagEntityLeafOP}
};

enum eVPXCT_IADDiagEntityLeaf
{
	eVPXCTIADDiagnosticsState,
    eVPXCTIADTestServer,
    eVPXCTIADRegistResult,
    eVPXCTIADReason
};

struct CWMP_LEAF tVPXCT_IADDiagEntityLeaf[] =
{
    { &tVPXCT_IADDiagEntityLeafInfo[eVPXCTIADDiagnosticsState]  },
    { &tVPXCT_IADDiagEntityLeafInfo[eVPXCTIADTestServer]  },
    { &tVPXCT_IADDiagEntityLeafInfo[eVPXCTIADRegistResult]  },
    { &tVPXCT_IADDiagEntityLeafInfo[eVPXCTIADReason]  },
    { NULL }
};

#ifdef CONFIG_CU
//******* VoiceService.{i}.VoiceProfile.X_"CTCOM_NAME_PREFIX"_FaxModem EntityLEAF*******
struct CWMP_OP tVPXCT_FaxModemEntityLeafOP = { getVPXCT_FaxModemEntity, setVPXCT_FaxModemEntity };
struct CWMP_PRMT tVPXCT_FaxModemEntityLeafInfo[] =
{
    /*(name,                    type,           flag,                   op)*/
	{ "FaxNego",                eCWMP_tINT,     CWMP_READ|CWMP_WRITE,   &tVPXCT_FaxModemEntityLeafOP}
};

enum eVPXCT_FaxModemEntityLeaf      
{
    eVPXCT_FaxModemFaxNego,
};

struct CWMP_LEAF tVPXCT_FaxModemEntityLeaf[] =
{
    { &tVPXCT_FaxModemEntityLeafInfo[eVPXCT_FaxModemFaxNego]  },
    { NULL }
};
#endif

//******* VoiceService.{i}.VoiceProfile.{i}. EntityLEAF*******
struct CWMP_OP tVoiceProfileEntityLeafOP = { getVoiceProfileEntity, setVoiceProfileEntity};	//FIXME

struct CWMP_PRMT tVoiceProfileEntityLeafInfo[] =
{
    /*(name,                               type,           flag,                 op)*/
	{ "Enable",                            eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "Reset",                             eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "NumberOfLines",                     eCWMP_tUINT,    CWMP_READ,            &tVoiceProfileEntityLeafOP},
	{ "Name",                              eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "SignalingProtocol",                 eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "MaxSessions",                       eCWMP_tUINT,    CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "DTMFMethod",                        eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "DigitMapEnable",                    eCWMP_tBOOLEAN, CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "DigitMap",                          eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
#ifdef CONFIG_CU	
	{ "X_"CTCOM_NAME_PREFIX"_ServerType",               eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
#else	
	{ "X_"CTCOM_NAME_PREFIX"_ServerType",               eCWMP_tINT,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
#endif
	{ "X_"CTCOM_NAME_PREFIX"_StartDigitTimer",          eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_InterDigitTimerShort",     eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_InterDigitTimerLong",      eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_HangingReminderToneTimer", eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_BusyToneTimer",            eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_NoAnswerTimer",            eCWMP_tINT,     CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_DigitMapMatchMode",          eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_RingVoltage",          eCWMP_tUINT,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},	
	{ "CallIDMsgType",          eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_DigitMapSpecial",		  eCWMP_tSTRING,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_DigitMapSpecialEnable",  eCWMP_tBOOLEAN,  CWMP_READ|CWMP_WRITE, &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_WANName",                  eCWMP_tSTRING,  CWMP_READ,            &tVoiceProfileEntityLeafOP},
	{ "X_"CTCOM_NAME_PREFIX"_InterfaceState",           eCWMP_tSTRING,  CWMP_READ,            &tVoiceProfileEntityLeafOP},
};

enum eVoiceProfileEntityLeaf
{
    eVPEnable,
    eVPReset,
    eVPNumberOfLines,
    eVPVoiceProfileName,  //will duplicated with service provider
    eVPSignallingProtocol,
    eVPMaxSessions,
    eVPDTMFMethod,
	eVPDigitMapEnable,
	eVPDigitMap,
	eVPXCT_ServerType,
	eVPXCT_StartDigitTimer,
	eVPXCT_InterDigitTimerShort,
	eVPXCT_InterDigitTimerLong,
	eVPXCT_HangingReminderToneTimer,
	eVPXCT_BusyToneTimer,
	eVPXCT_NoAnswerTimer,
	eVPX_HW_DigitalMapMatchMode,
	eVPX_CT_RingVoltage,
	eVPCallIDMsgType,
	eVPX_CT_DigitMapSpecial,
	eVPX_CT_DigitMapSpecialEnable,
    eVPXCT_WANName,
    eVPXCT_InterfaceState,
};

struct CWMP_LEAF tVoiceProfileEntityLeaf[] =
{
    { &tVoiceProfileEntityLeafInfo[eVPEnable]  },
    { &tVoiceProfileEntityLeafInfo[eVPReset]  },
    { &tVoiceProfileEntityLeafInfo[eVPNumberOfLines]  },
    { &tVoiceProfileEntityLeafInfo[eVPVoiceProfileName]  },
    { &tVoiceProfileEntityLeafInfo[eVPSignallingProtocol]  },
    { &tVoiceProfileEntityLeafInfo[eVPMaxSessions]  },
    { &tVoiceProfileEntityLeafInfo[eVPDTMFMethod]  },
    { &tVoiceProfileEntityLeafInfo[eVPDigitMapEnable]  },
    { &tVoiceProfileEntityLeafInfo[eVPDigitMap]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_ServerType]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_StartDigitTimer]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_InterDigitTimerShort]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_InterDigitTimerLong]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_HangingReminderToneTimer]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_BusyToneTimer]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_NoAnswerTimer]  },
    { &tVoiceProfileEntityLeafInfo[eVPX_HW_DigitalMapMatchMode]  },
    { &tVoiceProfileEntityLeafInfo[eVPX_CT_RingVoltage]  },
    { &tVoiceProfileEntityLeafInfo[eVPCallIDMsgType]  },
    { &tVoiceProfileEntityLeafInfo[eVPX_CT_DigitMapSpecial]  },
    { &tVoiceProfileEntityLeafInfo[eVPX_CT_DigitMapSpecialEnable]  },
#ifdef CONFIG_CU
    { &tVoiceProfileEntityLeafInfo[eVPXCT_WANName]  },
    { &tVoiceProfileEntityLeafInfo[eVPXCT_InterfaceState]  },
#endif 
    { NULL }
};

//******* VoiceService.{i}.VoiceProfile.{i} EntityObject*******
struct CWMP_OP tVoiceProfileLineOP = { NULL, objVoiceProfileLine };
struct CWMP_PRMT tVoiceProfileEntityObjectInfo[] =
{
    /*(name,                    type,           flag,       op)*/
    {"ServiceProviderInfo",     eCWMP_tOBJECT,  CWMP_READ,  NULL},
    {"SIP",                     eCWMP_tOBJECT,  CWMP_READ,  NULL},
//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104
    {"X_"CTCOM_NAME_PREFIX"_H248",           eCWMP_tOBJECT,  CWMP_READ,  NULL},
    {"X_HW_H248",               eCWMP_tOBJECT,  CWMP_READ,  NULL},
#endif
	{"RTP",                     eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{"NumberingPlan",           eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{"FaxT38",                  eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{"X_"CTCOM_NAME_PREFIX"_G711FAX",        eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{"X_"CTCOM_NAME_PREFIX"_IADDiagnostics", eCWMP_tOBJECT,  CWMP_READ,  NULL},
	{"X_"CTCOM_NAME_PREFIX"_FaxModem",          eCWMP_tOBJECT,  CWMP_READ,  NULL},
    {"Line",                    eCWMP_tOBJECT,  CWMP_READ,  &tVoiceProfileLineOP}
};
enum eVoiceProfileEntityObject
{
	eVPServiceProviderInfo,
	eVPSIP,
//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104
    eVH248,
    eVHW_H248,
#endif 
	eVPRTP,
//#if 0 //FXO_SUPPORT	
	eVPNumberingPlan,
//#endif
	eVPFaxT38,
	eVPXCT_G711FAX,
	eVPXCT_IADDiagnostics,
	eVPXCT_FaxModem,
	eVPLine
};

struct CWMP_NODE tVoiceProfileEntityObject[] =
{
    /*info,                                                   leaf,                          next)*/
    {&tVoiceProfileEntityObjectInfo[eVPServiceProviderInfo],  tVPServiceProviderEntityLeaf,  NULL},
    {&tVoiceProfileEntityObjectInfo[eVPSIP],                  tVPSIPEntityLeaf,              tVSIPEntityObject},
//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104
    {&tVoiceProfileEntityObjectInfo[eVH248],                  tVPH248EntityLeaf,             tVPH248Object},
    {&tVoiceProfileEntityObjectInfo[eVHW_H248],               NULL,              tVPHWH248Object},
#endif
	{&tVoiceProfileEntityObjectInfo[eVPRTP],                  tVPRTPEntityLeaf,              tVPRTPEntityObject},
//#if 0 //FXO_SUPPORT	
	{&tVoiceProfileEntityObjectInfo[eVPNumberingPlan],        tVPNumberingPlanEntityLeaf,    NULL},
//#endif	
	{&tVoiceProfileEntityObjectInfo[eVPFaxT38],               tVPFaxT38EntityLeaf,           NULL},
	{&tVoiceProfileEntityObjectInfo[eVPXCT_G711FAX],          tVPXCT_G711FAXEntityLeaf,      NULL},
	{&tVoiceProfileEntityObjectInfo[eVPXCT_IADDiagnostics],   tVPXCT_IADDiagEntityLeaf,      NULL},
#ifdef CONFIG_CU
    {&tVoiceProfileEntityObjectInfo[eVPXCT_FaxModem],         tVPXCT_FaxModemEntityLeaf,     NULL},
#endif
    {&tVoiceProfileEntityObjectInfo[eVPLine],                 NULL,                          NULL},
    {NULL,                                                    NULL,                          NULL}
};


//******* VoiceService.{i}.VoiceProfile  *******
struct CWMP_PRMT tVoiceProfileObjectInfo[] =
{
    /*(name,  type,           flag,		               op)*/
    {"0",     eCWMP_tOBJECT,  CWMP_READ|CWMP_LNKLIST,  NULL}
};
enum eVoiceProfileObject
{
    eVoiceProfile0
};
struct CWMP_LINKNODE tVoiceProfileObject[] =
{
    /*info,  			                        leaf,                     next,                       sibling,  instnum)*/
    {&tVoiceProfileObjectInfo[eVoiceProfile0],  tVoiceProfileEntityLeaf,  tVoiceProfileEntityObject,  NULL,     0}
};


int objVoiceProfile(char *name, struct CWMP_LEAF *e, int type, void *data)
{
    struct CWMP_NODE *entity=(struct CWMP_NODE *)e;

	switch( type ) {
	case eCWMP_tINITOBJ:
	{
		int num=0,i;
		struct CWMP_LINKNODE **c = (struct CWMP_LINKNODE **)data;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) 
            return -1;

		num = MAX_PROFILE_COUNT; /* Now, we has one voice profile only. */
        
		for( i=1; i<=num;i++ )
		{
			if( create_Object( c, tVoiceProfileObject, sizeof(tVoiceProfileObject), num, 1) < 0 )
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
            num = MAX_PROFILE_COUNT;
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
                        tVoiceProfileObject, sizeof(tVoiceProfileObject), (unsigned int *)&num );
                }
           }
           if( old_table )  destroy_ParameterTable( (struct CWMP_NODE *)old_table );           
           return 0;
    default:
            CWMPDBG( 0, ( stderr, "<%s:%d>Error: unknown type %d \n", __FUNCTION__, __LINE__, type) );
            break;
        }
	   	break;
	}
	return -1;
}

int getVoiceProfileEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}. */
    unsigned int nVoiceProfileInstNum;
    union {
        enable_t Enable;
        int Reset;
        unsigned int NumberOfLines;
        char Name[ 40 ];
        signaling_protocol_t SignallingProtocol;
        unsigned int MaxSessions;
        DTMF_method_t DTMFMethod;
		int DigitMapEnable;
		char DigitMap[MAX_DIALPLAN_LENGTH];
#ifdef CONFIG_CU		
        char XCT_ServerType[ 40 ];
#else
        int  XCT_ServerType;
#endif        
        int XCT_Timer;	//For Timers
        int default_index;
	char DigitMapMatchMode[8];
	unsigned int RingVoltage;
	char CallIDMsgType[8];
        char XCT_WANName[32];
        char XCT_InterfaceState[32];
	int DigitMapSpecialEnable;
	char DigitMapSpecial[256]; //size need check
    } s;
    
    const char *pszLastname = entity->info->name;

    if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
        return -1;

    if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
        return -1;

    if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )  /* convert to zero based */
        return -1;
    
    *type = entity->info->type;
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
    if( strcmp( pszLastname, "Enable" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__ENABLE, nVoiceProfileInstNum, &s.Enable );
        *data = strdup_Enable( s.Enable );
    } else if( strcmp( pszLastname, "Reset" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__RESET, nVoiceProfileInstNum, &s.Reset );
        *data = booldup( s.Reset );
    } else if( strcmp( pszLastname, "NumberOfLines" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &s.NumberOfLines );
        *data = uintdup( s.NumberOfLines );
    } else if( strcmp( pszLastname, "Name" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__NAME, nVoiceProfileInstNum, &s.Name );
        *data = strdup( s.Name );
    } else if( strcmp( pszLastname, "SignalingProtocol" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__SIGNALING_PROTOCOL, nVoiceProfileInstNum, &s.SignallingProtocol );
        *data = strdup_SignalingProtocol( s.SignallingProtocol );
    } else if( strcmp( pszLastname, "MaxSessions" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__MAX_SESSIONS, nVoiceProfileInstNum, &s.MaxSessions );
        *data = uintdup( s.MaxSessions );
    } else if( strcmp( pszLastname, "DTMFMethod" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__DTMF_METHOD, nVoiceProfileInstNum, &s.DTMFMethod );
        *data = strdup_DTMFMethod( s.DTMFMethod );  
    } else if( strcmp( pszLastname, "DigitMapEnable" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__DIGIT_MAP_ENABLE, nVoiceProfileInstNum, &s.DigitMapEnable );
        *data = booldup( s.DigitMapEnable );
    } else if( strcmp( pszLastname, "DigitMap" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__DIGIT_MAP, nVoiceProfileInstNum, &s.DigitMap );
        *data = strdup( s.DigitMap );  
#ifdef CONFIG_CU		
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_ServerType" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__XCT_SERVER_TYPE, nVoiceProfileInstNum, &s.XCT_ServerType );
        *data = strdup( s.XCT_ServerType );
#else
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_ServerType" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__XCT_SERVER_TYPE, nVoiceProfileInstNum, &s.XCT_ServerType );
        *data = intdup( s.XCT_ServerType );
#endif          
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_StartDigitTimer" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__XCT_START_DIGIT_TIMER, nVoiceProfileInstNum, &s.XCT_Timer );
        *data = intdup( s.XCT_Timer );  
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_InterDigitTimerShort" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_SHORT, nVoiceProfileInstNum, &s.XCT_Timer );
        *data = intdup( s.XCT_Timer );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_InterDigitTimerLong" ) == 0 ) {
        mib_get_type1( MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_LONG, nVoiceProfileInstNum, &s.XCT_Timer );
        *data = intdup( s.XCT_Timer );  
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HangingReminderToneTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_HANGING_REMINDER_TONE_TIMER, nVoiceProfileInstNum, &s.XCT_Timer );
		*data = intdup( s.XCT_Timer );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_BusyToneTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_BUSY_TONE_TIMER, nVoiceProfileInstNum, &s.XCT_Timer );
		*data = intdup( s.XCT_Timer );  
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_NoAnswerTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_NO_ANSWER_TIMER, nVoiceProfileInstNum, &s.XCT_Timer );
		*data = intdup( s.XCT_Timer );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_DigitMapMatchMode" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_DIGIT_MAP_MATCH_MODE, nVoiceProfileInstNum, &s.DigitMapMatchMode );
		*data = strdup(s.DigitMapMatchMode);
	} else if( strcmp( pszLastname, "X_HW_H248.Profile.ProfileBody" ) == 0 ) {
		*data = strdup( "" );	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_RingVoltage" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_RING_VOLTAGE, nVoiceProfileInstNum, &s.RingVoltage );
		*data = uintdup( s.RingVoltage );
	} else if( strcmp( pszLastname, "CallIDMsgType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_CALL_ID_MSG_TYPE, nVoiceProfileInstNum, &s.CallIDMsgType );
		*data = strdup( s.CallIDMsgType );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_WANName" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_WAN_NAME, nVoiceProfileInstNum, &s.XCT_WANName );
		*data = strdup( s.XCT_WANName );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_InterfaceState" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_INTERFACE_STATE, nVoiceProfileInstNum, &s.XCT_InterfaceState );
		*data = strdup( s.XCT_InterfaceState ); 
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_DigitMapSpecial" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL, nVoiceProfileInstNum, &s.DigitMapSpecial );
		*data = strdup(s.DigitMapSpecial);
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_DigitMapSpecialEnable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL_ENABLE, nVoiceProfileInstNum, &s.DigitMapSpecialEnable );
		*data = booldup( s.DigitMapSpecialEnable );		
    } else {
        *data = NULL;
        return ERR_9005;
    }
    
    return 0;
}

int setVoiceProfileEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}. */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	union {
		enable_t Enable;
		int Reset;
		unsigned int NumberOfLines;
		char Name[ 40 ];
		signaling_protocol_t SignallingProtocol;
		unsigned int MaxSessions;
		DTMF_method_t DTMFMethod;
		int DigitMapEnable;
		char DigitMap[MAX_DIALPLAN_LENGTH];
        int XCT_ServerType;
        int XCT_Timer;	//For Timers      
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "Enable" ) == 0 ){
		if( str2id_Enable( data, &s.Enable ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__ENABLE, nVoiceProfileInstNum, &s.Enable );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "Reset" ) == 0 ) {
		s.Reset = *( ( int * )data );
		res = mib_set_type1( MIB_VOICE_PROFILE__RESET, nVoiceProfileInstNum, &s.Reset );
	} else if( strcmp( pszLastname, "NumberOfLines" ) == 0 ) {
		return ERR_9008;	/* Read only */
	} else if( strcmp( pszLastname, "Name" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__NAME, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "SignalingProtocol" ) == 0 ) {
		if( str2id_SignalingProtocol( data, &s.SignallingProtocol ) ){
			mib_set_type1( MIB_VOICE_PROFILE__SIGNALING_PROTOCOL, nVoiceProfileInstNum, &s.SignallingProtocol );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "MaxSessions" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__MAX_SESSIONS, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "DTMFMethod" ) == 0 ) {
		if( str2id_DTMFMethod( data, &s.DTMFMethod ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__DTMF_METHOD, nVoiceProfileInstNum, &s.DTMFMethod );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "DigitMapEnable" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__DIGIT_MAP_ENABLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "DigitMap" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__DIGIT_MAP, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_ServerType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_SERVER_TYPE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_StartDigitTimer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_START_DIGIT_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_InterDigitTimerShort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_SHORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_InterDigitTimerLong" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_INTER_DIGIT_TIMER_LONG, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HangingReminderToneTimer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_HANGING_REMINDER_TONE_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_BusyToneTimer" ) == 0 ) {
		res = mib_set_type1(  MIB_VOICE_PROFILE__XCT_BUSY_TONE_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_NoAnswerTimer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_NO_ANSWER_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_DigitMapMatchMode" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_DIGIT_MAP_MATCH_MODE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_RingVoltage" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_RING_VOLTAGE, nVoiceProfileInstNum, data);
	} else if( strcmp( pszLastname, "CallIDMsgType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_CALL_ID_MSG_TYPE, nVoiceProfileInstNum,data );
	} else if( strcmp( pszLastname, "X_HW_H248.Profile.ProfileBody" ) == 0 ) {
		//eric temp.
		 res = 0;
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_DigitMapSpecial" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname,"X_"CTCOM_NAME_PREFIX"_DigitMapSpecialEnable" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_DIGITMAP_SPECIAL_ENABLE, nVoiceProfileInstNum, data );
	} else {
		return ERR_9005;
	}
	
	return res;	//1->self-reboot, 0->apply without reboot
}

int getVPSipEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP */
	unsigned int nVoiceProfileInstNum;
	union {
		char ProxyServer[ 256 ];
		unsigned int ProxyServerPort;
		transport_t ProxyServerTransport;
		char RegistrarServer[ 256 ];
		unsigned int RegistrarServerPort;
		transport_t RegistrarServerTransport;
		char UserAgentDomain[ 256 ];
		unsigned int UserAgentPort;
		transport_t UserAgentTransport;
		char OutboundProxy[ 256 ];
		unsigned int OutboundProxyPort;
		char Organization[ 256 ];
		unsigned int RegistrationPeriod;
        int VLANIDMark;
        unsigned int DSCPMark ;
        unsigned int RegisterRetryInterval;
        unsigned int RegisterExpires;
        int XCT_802_1_pMark;
        boolean XCT_HeartbeatSwitch;
        int XCT_HeartbeatCycle;
        int XCT_HeartbeatCount;
        int XCT_SessionUpdateTimer;
		unsigned int proxyindex;
		unsigned int CallIdTimePolicy;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "ProxyServer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER, nVoiceProfileInstNum, &s.ProxyServer );
		*data = strdup( s.ProxyServer );
	} else if( strcmp( pszLastname, "ProxyServerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT, nVoiceProfileInstNum, &s.ProxyServerPort );
		*data = uintdup( s.ProxyServerPort );
	} else if( strcmp( pszLastname, "ProxyServerTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
		*data = strdup_Transport( s.ProxyServerTransport );
	} else if( strcmp( pszLastname, "RegistrarServer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER, nVoiceProfileInstNum, &s.RegistrarServer );
		*data = strdup( s.RegistrarServer );
	} else if( strcmp( pszLastname, "RegistrarServerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, &s.RegistrarServerPort );
		*data = uintdup( s.RegistrarServerPort );
	} else if( strcmp( pszLastname, "RegistrarServerTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrarServerTransport );
		*data = strdup_Transport( s.RegistrarServerTransport );
	} else if( strcmp( pszLastname, "UserAgentDomain" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN, nVoiceProfileInstNum, &s.UserAgentDomain );
		*data = strdup( s.UserAgentDomain );
	} else if( strcmp( pszLastname, "UserAgentPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT, nVoiceProfileInstNum, &s.UserAgentPort );
		*data = uintdup( s.UserAgentPort );
	} else if( strcmp( pszLastname, "UserAgentTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_TRANSPORT, nVoiceProfileInstNum, &s.UserAgentTransport );
		*data = strdup_Transport( s.UserAgentTransport );
	} else if( strcmp( pszLastname, "OutboundProxy" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY, nVoiceProfileInstNum, &s.OutboundProxy );
		*data = strdup( s.OutboundProxy );
	} else if( strcmp( pszLastname, "OutboundProxyPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, &s.OutboundProxyPort );
		*data = uintdup( s.OutboundProxyPort );
	} else if( strcmp( pszLastname, "Organization" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__ORGANIZATION, nVoiceProfileInstNum, &s.Organization );
		*data = strdup( s.Organization );
	} else if( strcmp( pszLastname, "RegistrationPeriod" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD, nVoiceProfileInstNum, &s.RegistrationPeriod );
		*data = uintdup( s.RegistrationPeriod );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrationPeriod" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrationPeriod" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRATION_PERIOD, nVoiceProfileInstNum, &s.RegistrationPeriod );
		*data = uintdup( s.RegistrationPeriod );		
	} else if( strcmp( pszLastname, "VLANIDMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__VLANIDMARK, nVoiceProfileInstNum, &s.VLANIDMark );
		*data = intdup( s.VLANIDMark );
	} else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__DSCPMARK, nVoiceProfileInstNum, &s.DSCPMark );
		*data = uintdup( s.DSCPMark );
	} else if( strcmp( pszLastname, "RegisterRetryInterval" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTERRETRYINTERVAL, nVoiceProfileInstNum, &s.RegisterRetryInterval );
		*data = uintdup( s.RegisterRetryInterval );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterRetryInterval" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegisterRetryInterval" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTERRETRYINTERVAL, nVoiceProfileInstNum, &s.RegisterRetryInterval );
		*data = uintdup( s.RegisterRetryInterval );		
	} else if( strcmp( pszLastname, "RegisterExpires" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTEREXPIRES, nVoiceProfileInstNum, &s.RegisterExpires );
		*data = uintdup( s.RegisterExpires );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterExpires" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegisterExpires" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTEREXPIRES, nVoiceProfileInstNum, &s.RegisterExpires );
		*data = uintdup( s.RegisterExpires );		
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServer" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER, nVoiceProfileInstNum, &s.ProxyServer );
		*data = strdup( s.ProxyServer );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerPort" ) == 0 ) {
#endif	
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_PORT, nVoiceProfileInstNum, &s.ProxyServerPort );
		*data = uintdup( s.ProxyServerPort );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerTransport" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerTransport" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
		*data = strdup_Transport( s.ProxyServerTransport );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServer" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER, nVoiceProfileInstNum, &s.RegistrarServer );
		*data = strdup( s.RegistrarServer );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerPort" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, &s.RegistrarServerPort );
		*data = uintdup( s.RegistrarServerPort );
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerTransport" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerTransport" ) == 0 ) {
#endif	
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrarServerTransport );
		*data = strdup_Transport( s.RegistrarServerTransport );
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxy" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY, nVoiceProfileInstNum, &s.OutboundProxy );
		*data = strdup( s.OutboundProxy );
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxyPort" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, &s.OutboundProxyPort );
		*data = uintdup( s.OutboundProxyPort );		
	} else if( strcmp( pszLastname, "X_Default_Proxy_Index" ) == 0 ) {
			mib_get_type1( MIB_VOICE_PROFILE__XCT_DEFAULT_PROXY_INDEX, nVoiceProfileInstNum, &s.proxyindex );
			*data = uintdup( s.proxyindex );		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_802-1pMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_802_1PMARK, nVoiceProfileInstNum, &s.XCT_802_1_pMark );
		*data = intdup( s.XCT_802_1_pMark );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatSwitch" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATSWITCH, nVoiceProfileInstNum, &s.XCT_HeartbeatSwitch );
		*data = booldup( s.XCT_HeartbeatSwitch );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCYCLE, nVoiceProfileInstNum, &s.XCT_HeartbeatCycle );
		*data = intdup( s.XCT_HeartbeatCycle );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatCount" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCOUNT, nVoiceProfileInstNum, &s.XCT_HeartbeatCount );
		*data = intdup( s.XCT_HeartbeatCount );
	} else if( strcmp( pszLastname, "InviteExpires" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__INVITEEXPIRES, nVoiceProfileInstNum, &s.XCT_SessionUpdateTimer );
		*data = intdup( s.XCT_SessionUpdateTimer );
#ifdef CONFIG_CU
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryInviteExpires" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-InviteExpires" ) == 0 ) {
#endif
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_INVITEEXPIRES, nVoiceProfileInstNum, &s.XCT_SessionUpdateTimer );
		*data = intdup( s.XCT_SessionUpdateTimer );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SessionUpdateTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__INVITEEXPIRES, nVoiceProfileInstNum, &s.XCT_SessionUpdateTimer );
		*data = intdup( s.XCT_SessionUpdateTimer );
	} else if( strcmp( pszLastname, "CallIdTimePolicy" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__XCT_CALL_ID_TIME_POLICY, nVoiceProfileInstNum, &s.CallIdTimePolicy );
		*data = uintdup( s.CallIdTimePolicy );
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}

int setVPSipEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	union {
		transport_t ProxyServerTransport;
		transport_t RegistrarServerTransport;
		transport_t UserAgentTransport;
	} s;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "ProxyServer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "ProxyServerPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "ProxyServerTransport" ) == 0 ) {
		if( str2id_Transport( data, &s.ProxyServerTransport ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "RegistrarServer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegistrarServerPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegistrarServerTransport" ) == 0 ) {
		if( str2id_Transport( data, &s.RegistrarServerTransport ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrarServerTransport );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "UserAgentDomain" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "UserAgentPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "UserAgentTransport" ) == 0 ) {
		if( str2id_Transport( data, &s.UserAgentTransport ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_TRANSPORT, nVoiceProfileInstNum, &s.UserAgentTransport );
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "OutboundProxy" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "OutboundProxyPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "Organization" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__ORGANIZATION, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegistrationPeriod" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrationPeriod" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrationPeriod" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRATION_PERIOD, nVoiceProfileInstNum, data );

		
	} else if( strcmp( pszLastname, "VLANIDMark" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__SIP__VLANIDMARK, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__SIP__DSCPMARK, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "RegisterRetryInterval" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTERRETRYINTERVAL, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterRetryInterval" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegisterRetryInterval" ) == 0 ) {
#endif
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTERRETRYINTERVAL, nVoiceProfileInstNum, data );


    } else if( strcmp( pszLastname, "RegisterExpires" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTEREXPIRES, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU		
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegisterExpires" ) == 0 ) {
#else
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegisterExpires" ) == 0 ) {
#endif
        res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTEREXPIRES, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServer" ) == 0 ) {
#endif  
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerPort" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_PORT, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryProxyServerTransport" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-ProxyServerTransport" ) == 0 ) {
#endif	
		if( str2id_Transport( data, &s.ProxyServerTransport ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
		}else{
			return ERR_9007;
		}
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServer" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerPort" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryRegistrarServerTransport" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-RegistrarServerTransport" ) == 0 ) {
#endif	
		if( str2id_Transport( data, &s.RegistrarServerTransport ) ){
			res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrarServerTransport );
		}else{
			return ERR_9007;
		}
#ifdef CONFIG_CU			
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServer" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxy" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU	
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryOutboundProxyServerPort" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-OutboundProxyPort" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_Default_Proxy_Index" ) == 0 ) {
			res = mib_set_type1( MIB_VOICE_PROFILE__XCT_DEFAULT_PROXY_INDEX, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_802-1pMark" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_STB_802_1PMARK, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatSwitch" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATSWITCH, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatCycle" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCYCLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_HeartbeatCount" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_HEARTBEATCOUNT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "InviteExpires" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__INVITEEXPIRES, nVoiceProfileInstNum, data );
#ifdef CONFIG_CU		
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SecondaryInviteExpires" ) == 0 ) {
#else
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Standby-InviteExpires" ) == 0 ) {
#endif	
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_INVITEEXPIRES, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_SessionUpdateTimer" ) == 0 ) {
		/* for e8c set session timer, update to proxy0 and proxy1 */
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__INVITEEXPIRES, nVoiceProfileInstNum, data );
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_INVITEEXPIRES, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "CallIdTimePolicy" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__XCT_CALL_ID_TIME_POLICY, nVoiceProfileInstNum, data );	
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
}

int getVPSIPResponseMapEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.SIP.ResponseMap.{i} */
    unsigned int nVoiceProfileInstNum;
	union {
		unsigned int SIPResponseNumber;
		unsigned int Tone;
		unsigned int XCT_Duration;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "SIPResponseNumber" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__RM__SIP_RESPONSE_NUMBER, nVoiceProfileInstNum, &s.SIPResponseNumber );
		*data = uintdup( s.SIPResponseNumber );
    } else if( strcmp( pszLastname, "Tone" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__RM__TONE, nVoiceProfileInstNum, &s.Tone );
		*data = uintdup( s.Tone );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Duration" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__RM__XCT_DURATION, nVoiceProfileInstNum, &s.XCT_Duration );
		*data = uintdup( s.XCT_Duration );
    } else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

int setVPSIPResponseMapEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.SIP.ResponseMap.{i} */
    int res = 0;
	unsigned int nVoiceProfileInstNum;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "SIPResponseNumber" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__RM__SIP_RESPONSE_NUMBER, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "Tone" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__RM__TONE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_Duration" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SIP__RM__XCT_DURATION, nVoiceProfileInstNum, data );
    } else {
		return ERR_9005;
	}

	return res;	//1->self-reboot, 0->apply without reboot
}

//#ifdef CONFIG_USER_RTK_H248
#if 1 //remove 248 compiler , e8c always enable on tr104
int getVPH248Entity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248 */
	unsigned int nVoiceProfileInstNum;
	union {
		char    MediaGatewayControler[ 256 ];
		int     MediaGatewayControlerPort;
        char    Standby_MediaGatewayControler[ 256 ];
		int     Standby_MediaGatewayControlerPort;
        int     MediaGatewayPort;
        char    MessageEncodingType[ 256 ];
        char    DeviceID[ 256 ];
        unsigned int     DeviceIDType;
		char    PhysicalTermID[ 256 ];
        unsigned int     PhysicalTermIDConfigMethod;
        char    PhysicalTermIDPrefix[ 256 ];
        unsigned int     PhysicalTermIDAddLen;
        char    RTPPrefix[ 256 ];
        unsigned int     EphemeralTermIDAddLen;
        unsigned int     EphemeralTermIDUniform;
        unsigned int     EphemeralTermIDStart;
        unsigned int     ThreeHandShake;
        unsigned int     LongTimer;
        unsigned int     PendingTimerInit;
        unsigned int     RetranIntervalTimer;
        unsigned int     MaxRetranCount;
		int         RetransmissionTime;
		int         RetransmissionCycle;
        unsigned int     HeartbeatMode;
        int         HeartbeatCycle;
        int         HeartbeatCount;
        int         RegisterCount;
        int         RegisterCycle;
        boolean     MandatoryRegister;
        unsigned int     AuthenticationMethID;
        /* H.248 CU */
        char    Domain[ 256 ];
        char    MIDFormat[ 256 ];
        char    CallAgent1[ 256 ];
        char    CallAgent2[ 256 ];
        char    LineName[ 256 ];
        int     CallAgentPort1;
        int     CallAgentPort2;
        int     LocalPort;
        char    DeviceName[ 256 ];
        int     DSCPMark;
        int     SoftswitchVendor;
        char    SoftswitchVersion[ 256 ];
        /* H.248 CU end */
	} s;
fprintf(stderr,"[%s %d] \n",__FUNCTION__,__LINE__);
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "MediaGatewayControler" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER, nVoiceProfileInstNum, &s.MediaGatewayControler );
        *data = strdup( s.MediaGatewayControler );
	} else if( strcmp( pszLastname, "MediaGatewayControlerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER_PORT, nVoiceProfileInstNum, &s.MediaGatewayControlerPort );
		*data = intdup( s.MediaGatewayControlerPort );
	} else if( strcmp( pszLastname, "Standby-MediaGatewayControler" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER, nVoiceProfileInstNum, &s.Standby_MediaGatewayControler );
		*data = strdup( s.Standby_MediaGatewayControler );
	} else if( strcmp( pszLastname, "Standby-MediaGatewayControlerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER_PORT, nVoiceProfileInstNum, &s.Standby_MediaGatewayControlerPort );
		*data = intdup( s.Standby_MediaGatewayControlerPort );
	} else if( strcmp( pszLastname, "MediaGatewayPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_PORT, nVoiceProfileInstNum, &s.MediaGatewayPort );
		*data = intdup( s.MediaGatewayPort );
	} else if( strcmp( pszLastname, "MessageEncodingType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MESSAGE_ENCODING_TYPE, nVoiceProfileInstNum, &s.MessageEncodingType );
		*data = strdup( s.MessageEncodingType );
	} else if( strcmp( pszLastname, "DeviceID" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DEVICE_ID, nVoiceProfileInstNum, &s.DeviceID );
		*data = strdup( s.DeviceID );
	} else if( strcmp( pszLastname, "DeviceIDType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DEVICE_ID_TYPE, nVoiceProfileInstNum, &s.DeviceIDType );
		*data = uintdup( s.DeviceIDType );
	} else if( strcmp( pszLastname, "PhysicalTermID" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID, nVoiceProfileInstNum, &s.PhysicalTermID );
		*data = strdup( s.PhysicalTermID );	
	} else if( strcmp( pszLastname, "PhysicalTermIDConfigMethod" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_CONFIG_METHOD, nVoiceProfileInstNum, &s.PhysicalTermIDConfigMethod );
		*data = uintdup( s.PhysicalTermIDConfigMethod );
	} else if( strcmp( pszLastname, "PhysicalTermIDPrefix" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_PREFIX, nVoiceProfileInstNum, &s.PhysicalTermIDPrefix );
		*data = strdup( s.PhysicalTermIDPrefix );
	} else if( strcmp( pszLastname, "PhysicalTermIDAddLen" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_ADD_LEN, nVoiceProfileInstNum, &s.PhysicalTermIDAddLen );
		*data = uintdup( s.PhysicalTermIDAddLen );
	} else if( strcmp( pszLastname, "RTPPrefix" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RTP_PREFIX, nVoiceProfileInstNum, &s.RTPPrefix );
		*data = strdup( s.RTPPrefix );
	} else if( strcmp( pszLastname, "EphemeralTermIDAddLen" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_ADD_LEN, nVoiceProfileInstNum, &s.EphemeralTermIDAddLen );
		*data = uintdup( s.EphemeralTermIDAddLen );
	} else if( strcmp( pszLastname, "EphemeralTermIDUniform" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_UNIFORM, nVoiceProfileInstNum, &s.EphemeralTermIDUniform );
		*data = uintdup( s.EphemeralTermIDUniform );		
	} else if( strcmp( pszLastname, "EphemeralTermIDStart" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_START, nVoiceProfileInstNum, &s.EphemeralTermIDStart );
		*data = uintdup( s.EphemeralTermIDStart );
	} else if( strcmp( pszLastname, "ThreeHandShake" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__THREE_HAND_SHAKE, nVoiceProfileInstNum, &s.ThreeHandShake );
		*data = uintdup( s.ThreeHandShake );
	} else if( strcmp( pszLastname, "LongTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__LONG_TIMER, nVoiceProfileInstNum, &s.LongTimer );
		*data = uintdup( s.LongTimer );
	} else if( strcmp( pszLastname, "PendingTimerInit" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__PENDING_TIMER_INIT, nVoiceProfileInstNum, &s.PendingTimerInit );
		*data = uintdup( s.PendingTimerInit );		
	} else if( strcmp( pszLastname, "RetranIntervalTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER, nVoiceProfileInstNum, &s.RetranIntervalTimer );
		*data = uintdup( s.RetranIntervalTimer );
	} else if( strcmp( pszLastname, "MaxRetranCount" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT, nVoiceProfileInstNum, &s.MaxRetranCount );
		*data = uintdup( s.MaxRetranCount );	
	} else if( strcmp( pszLastname, "RetransmissionTime" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME, nVoiceProfileInstNum, &s.RetransmissionTime );
		*data = intdup( s.RetransmissionTime );	
	} else if( strcmp( pszLastname, "RetransmissionCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE, nVoiceProfileInstNum, &s.RetransmissionCycle );
		*data = intdup( s.RetransmissionCycle );		
	} else if( strcmp( pszLastname, "HeartbeatMode" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE, nVoiceProfileInstNum, &s.HeartbeatMode );
		*data = uintdup( s.HeartbeatMode );
	} else if( strcmp( pszLastname, "HeartbeatCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_CYCLE, nVoiceProfileInstNum, &s.HeartbeatCycle );
		*data = intdup( s.HeartbeatCycle );
	} else if( strcmp( pszLastname, "HeartbeatCount" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_COUNT, nVoiceProfileInstNum, &s.HeartbeatCount );
		*data = intdup( s.HeartbeatCount );
	} else if( strcmp( pszLastname, "RegisterCount" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__REGISTER_COUNT, nVoiceProfileInstNum, &s.RegisterCount );
		*data = intdup( s.RegisterCount );
	} else if( strcmp( pszLastname, "RegisterCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__REGISTER_CYCLE, nVoiceProfileInstNum, &s.RegisterCycle );
		*data = intdup( s.RegisterCycle );
	} else if( strcmp( pszLastname, "MandatoryRegister" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MANDATORY_REGISTER, nVoiceProfileInstNum, &s.MandatoryRegister );
		*data = booldup( s.MandatoryRegister );
	} else if( strcmp( pszLastname, "AuthenticationMethID" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__AUTHENTICATION_METH_ID, nVoiceProfileInstNum, &s.AuthenticationMethID );
		*data = uintdup( s.AuthenticationMethID );
	/* H.248 CU */
	} else if( strcmp( pszLastname, "Domain" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DOMAIN, nVoiceProfileInstNum, &s.Domain );
		*data = strdup( s.Domain );	
	} else if( strcmp( pszLastname, "MIDFormat" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MIDFORMAT, nVoiceProfileInstNum, &s.MIDFormat );
		*data = strdup( s.MIDFormat );	
	} else if( strcmp( pszLastname, "CallAgent1" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__CALLAGENT1, nVoiceProfileInstNum, &s.CallAgent1 );
		*data = strdup( s.CallAgent1 );
	} else if( strcmp( pszLastname, "CallAgent2" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__CALLAGENT2, nVoiceProfileInstNum, &s.CallAgent2 );
		*data = strdup( s.CallAgent2 );
	} else if( strcmp( pszLastname, "LineName" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__LINENAME, nVoiceProfileInstNum, &s.LineName );
		*data = strdup( s.LineName );	
	} else if( strcmp( pszLastname, "CallAgentPort1" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__CALLAGENTPORT1, nVoiceProfileInstNum, &s.CallAgentPort1 );
		*data = intdup( s.CallAgentPort1 );
	} else if( strcmp( pszLastname, "CallAgentPort2" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__CALLAGENTPORT2, nVoiceProfileInstNum, &s.CallAgentPort2 );
		*data = intdup( s.CallAgentPort2 );
	} else if( strcmp( pszLastname, "LocalPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__LOCALPORT, nVoiceProfileInstNum, &s.LocalPort );
		*data = intdup( s.LocalPort );	
	} else if( strcmp( pszLastname, "DeviceName" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DEVICENAME, nVoiceProfileInstNum, &s.DeviceName );
		*data = strdup( s.DeviceName );		
	} else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DSCPMARK, nVoiceProfileInstNum, &s.DSCPMark );
		*data = intdup( s.DSCPMark );
	} else if( strcmp( pszLastname, "SoftswitchVendor" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__SOFTSWITCHVENDOR, nVoiceProfileInstNum, &s.SoftswitchVendor );
		*data = intdup( s.SoftswitchVendor );
	} else if( strcmp( pszLastname, "SoftswitchVersion" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__SOFTSWITCHVERSION, nVoiceProfileInstNum, &s.SoftswitchVersion );
		*data = strdup( s.SoftswitchVersion );						     		
	/* H.248 CU end */		
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}

int setVPH248Entity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248 */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;
	
    fprintf(stderr,"[%s %d] \n",__FUNCTION__,__LINE__);
	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    fprintf(stderr,"[%s %d] set %s value %s \n", __FUNCTION__, __LINE__, name, (char *)data);
	if( strcmp( pszLastname, "MediaGatewayControler" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MediaGatewayControlerPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "Standby-MediaGatewayControler" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "Standby-MediaGatewayControlerPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__STANDBY_MEDIAGATEWAY_CONTROLER_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MediaGatewayPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_PORT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MessageEncodingType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MESSAGE_ENCODING_TYPE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "DeviceID" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__DEVICE_ID, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "DeviceIDType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__DEVICE_ID_TYPE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "PhysicalTermID" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "PhysicalTermIDConfigMethod" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_CONFIG_METHOD, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "PhysicalTermIDPrefix" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_PREFIX, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "PhysicalTermIDAddLen" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__PHYSICAL_TERM_ID_ADD_LEN, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RTPPrefix" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RTP_PREFIX, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "EphemeralTermIDAddLen" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_ADD_LEN, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "EphemeralTermIDUniform" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_UNIFORM, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "EphemeralTermIDStart" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__EPHERMERAL_TERM_ID_START, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "ThreeHandShake" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__THREE_HAND_SHAKE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "LongTimer" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__LONG_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "PendingTimerInit" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__PENDING_TIMER_INIT, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "RetranIntervalTimer" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "MaxRetranCount" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "RetransmissionTime" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "RetransmissionCycle" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE, nVoiceProfileInstNum, data );	
    } else if( strcmp( pszLastname, "HeartbeatMode" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "HeartbeatCycle" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_CYCLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "HeartbeatCount" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_COUNT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegisterCount" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__REGISTER_COUNT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegisterCycle" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__REGISTER_CYCLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MandatoryRegister" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MANDATORY_REGISTER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "AuthenticationMethID" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__AUTHENTICATION_METH_ID, nVoiceProfileInstNum, data );
	/* H.248 CU */
    } else if( strcmp( pszLastname, "Domain" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__DOMAIN, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "MIDFormat" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MIDFORMAT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "CallAgent1" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__CALLAGENT1, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "CallAgent2" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__CALLAGENT2, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "LineName" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__LINENAME, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "CallAgentPort1" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__CALLAGENTPORT1, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "CallAgentPort2" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__CALLAGENTPORT2, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "LocalPort" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__LOCALPORT, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "DeviceName" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__DEVICENAME, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__DSCPMARK, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "SoftswitchVendor" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__SOFTSWITCHVENDOR, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "SoftswitchVersion" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__SOFTSWITCHVERSION, nVoiceProfileInstNum, data );							
    /* H.248 CU end */	
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
}

int getVPAuthEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.auth */
	unsigned int nVoiceProfileInstNum;
	union {
		boolean     auth;
        char    authInitKey[ 256 ];
		char    authmgid[ 256 ];
        char    DeviceID[ 256 ];
        boolean     authHeader;
		char    HeaderSecurityType[ 256 ];
        char    Rfc2833EncryptKey[ 256 ];
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "auth" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__AUTH, nVoiceProfileInstNum, &s.auth );
        *data = booldup( s.auth );
	} else if( strcmp( pszLastname, "authInitKey" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__AUTHINITKEY, nVoiceProfileInstNum, &s.authInitKey );
		*data = strdup( s.authInitKey );
	} else if( strcmp( pszLastname, "authmgid" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__AUTHMGID, nVoiceProfileInstNum, &s.authmgid );
		*data = strdup( s.authmgid );
	} else if( strcmp( pszLastname, "authHeader" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__AUTHHEADER, nVoiceProfileInstNum, &s.authHeader );
		*data = booldup( s.authHeader );
	} else if( strcmp( pszLastname, "HeaderSecurityType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEADERSECURITYTYPE, nVoiceProfileInstNum, &s.HeaderSecurityType );
		*data = strdup( s.HeaderSecurityType );
	} else if( strcmp( pszLastname, "Rfc2833EncryptKey" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RFC2833ENCRYPTKEY, nVoiceProfileInstNum, &s.Rfc2833EncryptKey );
		*data = strdup( s.Rfc2833EncryptKey );
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}    

int setVPAuthEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.auth */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "auth" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__AUTH, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "authInitKey" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__AUTHINITKEY, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "authmgid" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__AUTHMGID, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "authHeader" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__AUTHHEADER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "HeaderSecurityType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__HEADERSECURITYTYPE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "Rfc2833EncryptKey" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RFC2833ENCRYPTKEY, nVoiceProfileInstNum, data );		
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
}

int getVPStackInfoEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.StackInfo */
	unsigned int nVoiceProfileInstNum;
	union {
		char    MsgFormat[ 256 ];
		boolean     ResponseAckCtrl;
		unsigned int     LongTimer;
		unsigned int     RetranIntervalTimer;
		unsigned int     MaxRetranCount;
		unsigned int     RetransmissionTime;
		unsigned int     RetransmissionCycle;
		unsigned int     RegisterCycle;
		unsigned int     MGProvisionalRespTime;
		unsigned int     MTUValue;
		boolean     MsgSegmentation;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 0, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "MsgFormat" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MSGFORMAT, nVoiceProfileInstNum, &s.MsgFormat );
        *data = strdup( s.MsgFormat );
	} else if( strcmp( pszLastname, "ResponseAckCtrl" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RESPONSEACKCTRL, nVoiceProfileInstNum, &s.ResponseAckCtrl );
		*data = booldup( s.ResponseAckCtrl );
	} else if( strcmp( pszLastname, "LongTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__LONG_TIMER, nVoiceProfileInstNum, &s.LongTimer );
		*data = intdup( s.LongTimer );
	} else if( strcmp( pszLastname, "RetranIntervalTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER, nVoiceProfileInstNum, &s.RetranIntervalTimer );
		*data = intdup( s.RetranIntervalTimer );
	} else if( strcmp( pszLastname, "MaxRetranCount" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT, nVoiceProfileInstNum, &s.MaxRetranCount );
		*data = intdup( s.MaxRetranCount );
	} else if( strcmp( pszLastname, "RetransmissionTime" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME, nVoiceProfileInstNum, &s.RetransmissionTime );
		*data = intdup( s.RetransmissionTime );
	} else if( strcmp( pszLastname, "RetransmissionCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE, nVoiceProfileInstNum, &s.RetransmissionCycle );
		*data = intdup( s.RetransmissionCycle );
	} else if( strcmp( pszLastname, "RegisterCycle" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__REGISTER_CYCLE, nVoiceProfileInstNum, &s.RegisterCycle );
		*data = intdup( s.RegisterCycle );
	} else if( strcmp( pszLastname, "MGProvisionalRespTime" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MGPROVISIONALRESPTIME, nVoiceProfileInstNum, &s.MGProvisionalRespTime );
		*data = intdup( s.MGProvisionalRespTime );		
	} else if( strcmp( pszLastname, "MTUValue" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MTUVALUE, nVoiceProfileInstNum, &s.MTUValue );
		*data = intdup( s.MTUValue );	
	} else if( strcmp( pszLastname, "MsgSegmentation" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MSGSEGMENTATION, nVoiceProfileInstNum, &s.MsgSegmentation );
		*data = booldup( s.MsgSegmentation );				
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}    

int setVPStackInfoEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.StackInfo */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "MsgFormat" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MSGFORMAT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "ResponseAckCtrl" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RESPONSEACKCTRL, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "LongTimer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__LONG_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RetranIntervalTimer" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRAN_INTERVAL_TIMER, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MaxRetranCount" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MAX_RETRAN_COUNT, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RetransmissionTime" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_TIME, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RetransmissionCycle" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__RETRANSMISSION_CYCLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "RegisterCycle" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__REGISTER_CYCLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "MGProvisionalRespTime" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MGPROVISIONALRESPTIME, nVoiceProfileInstNum, data );		
	} else if( strcmp( pszLastname, "MTUValue" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MTUVALUE, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "MsgSegmentation" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__H248__MSGSEGMENTATION, nVoiceProfileInstNum, data );				
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
} 
  
int getVPDigitmapEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Digitmap */
	unsigned int nVoiceProfileInstNum;
	union {
		char    DMName[ 256 ];
		char    DigitMap[ 256 ];
		unsigned int     DigitMapStartTimer;
		unsigned int     DigitMapShortTimer;
		unsigned int     DigitMapLongTimer;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 0, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "DMName" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DMNAME, nVoiceProfileInstNum, &s.DMName );
        *data = strdup( s.DMName );
    } else if( strcmp( pszLastname, "DigitMap" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DIGITMAP, nVoiceProfileInstNum, &s.DigitMap );
		*data = strdup( s.DigitMap ); 
	} else if( strcmp( pszLastname, "DigitMapStartTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DIGITMAPSTARTTIMER, nVoiceProfileInstNum, &s.DigitMapStartTimer );
		*data = intdup( s.DigitMapStartTimer );	   
	} else if( strcmp( pszLastname, "DigitMapShortTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DIGITMAPSHORTTIMER, nVoiceProfileInstNum, &s.DigitMapShortTimer );
		*data = intdup( s.DigitMapShortTimer );	   
	} else if( strcmp( pszLastname, "DigitMapLongTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__DIGITMAPLONGTIMER, nVoiceProfileInstNum, &s.DigitMapLongTimer );
		*data = intdup( s.DigitMapLongTimer );	   					
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}    

int setVPDigitmapEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Digitmap */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "DMName" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__DMNAME, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "DigitMap" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__DIGITMAP, nVoiceProfileInstNum, data ); 
	} else if( strcmp( pszLastname, "DigitMapStartTimer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__DIGITMAPSTARTTIMER, nVoiceProfileInstNum, data );	   
	} else if( strcmp( pszLastname, "DigitMapShortTimer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__DIGITMAPSHORTTIMER, nVoiceProfileInstNum, data );	   
	} else if( strcmp( pszLastname, "DigitMapLongTimer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__DIGITMAPLONGTIMER, nVoiceProfileInstNum, data );			
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
} 

int getVPExtendEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Extend */
	unsigned int nVoiceProfileInstNum;
	union {
		unsigned int     HeartbeatMode;
		unsigned int     HeartBeatTimer;
		unsigned int     HeartBeatRetransTimer;
		unsigned int     HeartBeatRetransTimes;
		unsigned int     MWDBaseTime;
		unsigned int     MWD;
		char    MgcSwitchMethod[ 256 ];
		
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 0, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "HeartbeatMode" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE, nVoiceProfileInstNum, &s.HeartbeatMode );
        *data = intdup( s.HeartbeatMode );
    } else if( strcmp( pszLastname, "HeartBeatTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_TIMER, nVoiceProfileInstNum, &s.HeartBeatTimer );
		*data = intdup( s.HeartBeatTimer ); 
	} else if( strcmp( pszLastname, "HeartBeatRetransTimer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMER, nVoiceProfileInstNum, &s.HeartBeatRetransTimer );
		*data = intdup( s.HeartBeatRetransTimer );	   
	} else if( strcmp( pszLastname, "HeartBeatRetransTimes" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMES, nVoiceProfileInstNum, &s.HeartBeatRetransTimes );
		*data = intdup( s.HeartBeatRetransTimes );	   
	} else if( strcmp( pszLastname, "MWDBaseTime" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MWD_BASE_TIME, nVoiceProfileInstNum, &s.MWDBaseTime );
		*data = intdup( s.MWDBaseTime );	
	} else if( strcmp( pszLastname, "MWD" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MWD, nVoiceProfileInstNum, &s.MWD );
		*data = intdup( s.MWD );	
	} else if( strcmp( pszLastname, "MgcSwitchMethod" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__H248__MGCSWITCHMETHOD, nVoiceProfileInstNum, &s.MgcSwitchMethod );
		*data = strdup( s.MgcSwitchMethod );			   					
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}    

int setVPExtendEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
    /* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_H248.Extend */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "HeartbeatMode" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_MODE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "HeartBeatTimer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_TIMER, nVoiceProfileInstNum, data ); 
	} else if( strcmp( pszLastname, "HeartBeatRetransTimer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMER, nVoiceProfileInstNum, data );	   
	} else if( strcmp( pszLastname, "HeartBeatRetransTimes" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__HEARTBEAT_RETRANS_TIMES, nVoiceProfileInstNum, data );	   
	} else if( strcmp( pszLastname, "MWDBaseTime" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__MWD_BASE_TIME, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "MWD" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__MWD, nVoiceProfileInstNum, data );	
	} else if( strcmp( pszLastname, "MgcSwitchMethod" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__H248__MGCSWITCHMETHOD, nVoiceProfileInstNum, data );					
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
} 

int getVPHWH248Leaf(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_HW_H248.Profile */
	unsigned int nVoiceProfileInstNum;
#if 0
	union {
		char    ProfileBody[ 256 ];
	} s;
#endif
	fprintf(stderr,"[%s %d] \n",__FUNCTION__,__LINE__);
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "ProfileBody" ) == 0 ) {
        *data = strdup("");
	}else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}

int setVPHWH248Leaf(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_HW_H248.Profile */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

    fprintf(stderr,"[%s %d] \n",__FUNCTION__,__LINE__);
	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

	CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    fprintf(stderr,"[%s %d] set %s value %s \n", __FUNCTION__, __LINE__, name, (char *)data);
	if( strcmp( pszLastname, "ProfileBody" ) == 0 ) {
		//res = mib_set_type1( MIB_VOICE_PROFILE__H248__MEDIAGATEWAY_CONTROLER, nVoiceProfileInstNum, data );
		//eric test
		return 0;
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
}
#endif

int getVPRtpRtcpEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP.RTCP */
	unsigned int nVoiceProfileInstNum;
	union {
        boolean Enable;
        unsigned int TxRepeatInterval;
        boolean XCT_RtcpxrFlag;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__RTCP__ENABLE, nVoiceProfileInstNum, &s.Enable );
		*data = booldup( s.Enable );
	} else if( strcmp( pszLastname, "TxRepeatInterval" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__RTCP__TX_REPEAT_INTERVAL, nVoiceProfileInstNum, &s.TxRepeatInterval );
		*data = uintdup( s.TxRepeatInterval );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_RtcpxrFlag" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__RTCP__XCT_RTCPXR_FLAG, nVoiceProfileInstNum, &s.XCT_RtcpxrFlag );
		*data = booldup( s.XCT_RtcpxrFlag );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setVPRtpRtcpEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP.RTCP */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
    if( strcmp( pszLastname, "Enable" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__RTCP__ENABLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "TxRepeatInterval" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__RTCP__TX_REPEAT_INTERVAL, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_RtcpxrFlag" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__RTCP__XCT_RTCPXR_FLAG, nVoiceProfileInstNum, data );
	} else {
		return ERR_9005;
	}

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPRtpRedundancyEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP.REDUNDANCY */
	unsigned int nVoiceProfileInstNum;
	union {
        boolean Enable;
        unsigned int PayloadType ;
        boolean XCT_EnableFixedStart;
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__ENABLE, nVoiceProfileInstNum, &s.Enable );
		*data = booldup( s.Enable );
	} else if( strcmp( pszLastname, "PayloadType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__PAYLOAD_TYPE, nVoiceProfileInstNum, &s.PayloadType );
		*data = uintdup( s.PayloadType );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_EnableFixedStart" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__XCT_ENABLE_FIXED_START, nVoiceProfileInstNum, &s.XCT_EnableFixedStart );
		*data = booldup( s.XCT_EnableFixedStart );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setVPRtpRedundancyEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP.REDUNDANCY */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
    if( strcmp( pszLastname, "Enable" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__ENABLE, nVoiceProfileInstNum, data );
	} else if( strcmp( pszLastname, "PayloadType" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__PAYLOAD_TYPE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_EnableFixedStart" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__RTP__REDUNDANCY__XCT_ENABLE_FIXED_START, nVoiceProfileInstNum, data );
	} else {
		return ERR_9005;
	}

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPRTPEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP */
	unsigned int nVoiceProfileInstNum;
	union {
        int VLANIDMark;
        unsigned int DSCPMark ;
        int XCT_802_1_pMark;
        unsigned int LocalPortMin;
        unsigned int LocalPortMax;
        unsigned int TelephoneEventPayloadType;
        char XCT_WANName[32];
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

	CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "VLANIDMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__VLANIDMARK, nVoiceProfileInstNum, &s.VLANIDMark );
		*data = intdup( s.VLANIDMark );
	} else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__DSCPMARK, nVoiceProfileInstNum, &s.DSCPMark );
		*data = uintdup( s.DSCPMark );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_802-1pMark" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__XCT_STB_802_1PMARK, nVoiceProfileInstNum, &s.XCT_802_1_pMark );
		*data = intdup( s.XCT_802_1_pMark );
    } else if( strcmp( pszLastname, "LocalPortMin" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MIN, nVoiceProfileInstNum, &s.LocalPortMin );
		*data = uintdup( s.LocalPortMin );
    } else if( strcmp( pszLastname, "LocalPortMax" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MAX, nVoiceProfileInstNum, &s.LocalPortMax );
		*data = uintdup( s.LocalPortMax );
    } else if( strcmp( pszLastname, "TelephoneEventPayloadType" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__TELEPHONE_EVENT_PAYLOAD_TYPE, nVoiceProfileInstNum, &s.TelephoneEventPayloadType );
		*data = uintdup( s.TelephoneEventPayloadType );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_WANName" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RTP__XCT_WAN_NAME, nVoiceProfileInstNum, &s.XCT_WANName );
		*data = strdup( s.XCT_WANName );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setVPRTPEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.RTP */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "VLANIDMark" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__VLANIDMARK, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "DSCPMark" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__DSCPMARK, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_802-1pMark" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__XCT_STB_802_1PMARK, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "LocalPortMin" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MIN, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "LocalPortMax" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__LOCAL_PORT_MAX, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "TelephoneEventPayloadType" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__RTP__TELEPHONE_EVENT_PAYLOAD_TYPE, nVoiceProfileInstNum, data );
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPNumberingPlanEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.NumberingPlan */
	unsigned int nVoiceProfileInstNum;
	union {
        char XCT_NumberPlan[256];
		// add more parameters here
	} s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_NumberPlan" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__NUMBERINGPLAN__XCT_NUMBERPLAN, nVoiceProfileInstNum, &s.XCT_NumberPlan );
		*data = strdup( s.XCT_NumberPlan );
	} else {
		return ERR_9005;
	}

    return 0;
}

int setVPNumberingPlanEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.NumberingPlan */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_NumberPlan" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__NUMBERINGPLAN__XCT_NUMBERPLAN, nVoiceProfileInstNum, data );
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPFaxT38Entity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.FaxT38 */
	unsigned int nVoiceProfileInstNum;
    union{
	    boolean Enable;
	    unsigned int BitRate;
	    char TCFMethod[16];
	    unsigned int HighSpeedRedundancy;
	    unsigned int LowSpeedRedundancy;
	    boolean XCT_PortAdd2;
    } s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__ENABLE, nVoiceProfileInstNum, &s.Enable );
		*data = booldup(s.Enable);
    } else if( strcmp( pszLastname, "BitRate" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__BITRATE, nVoiceProfileInstNum, &s.BitRate );
		*data = uintdup(s.BitRate);
    } else if( strcmp( pszLastname, "TCFMethod" ) == 0  ){
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__TCF_METHOD, nVoiceProfileInstNum, &s.TCFMethod );
		*data = strdup(s.TCFMethod);
    } else if( strcmp( pszLastname, "HighSpeedRedundancy" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__HIGH_SPEED_REDUNDANCY, nVoiceProfileInstNum, &s.HighSpeedRedundancy );
		*data = uintdup(s.HighSpeedRedundancy);
    } else if( strcmp( pszLastname, "LowSpeedRedundancy" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__LOW_SPEED_REDUNDANCY, nVoiceProfileInstNum, &s.LowSpeedRedundancy );
		*data = uintdup(s.LowSpeedRedundancy);
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_PortAdd2" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__FAXT38__XCT_PORT_ADD2, nVoiceProfileInstNum, &s.XCT_PortAdd2 );
		*data = booldup(s.XCT_PortAdd2);
	} else {
		return ERR_9005;
	}

    return 0;
}

int setVPFaxT38Entity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.FaxT38 */
    int res = 0;
	unsigned int nVoiceProfileInstNum;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__ENABLE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "BitRate" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__BITRATE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "TCFMethod" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__TCF_METHOD, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "HighSpeedRedundancy" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__HIGH_SPEED_REDUNDANCY, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "LowSpeedRedundancy" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__LOW_SPEED_REDUNDANCY, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "X_"CTCOM_NAME_PREFIX"_PortAdd2" ) == 0 ) {
       	res = mib_set_type1( MIB_VOICE_PROFILE__FAXT38__XCT_PORT_ADD2, nVoiceProfileInstNum, data );
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}


int getVPXCT_G711FAXEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_G711FAX */
	unsigned int nVoiceProfileInstNum;
    union{
	    int Enable;
        char ControlType[16];
    } s;

	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_G711FAX__ENABLE, nVoiceProfileInstNum, &s.Enable );
		*data = intdup(s.Enable);
	} else if( strcmp( pszLastname, "ControlType" ) == 0  ){
		mib_get_type1( MIB_VOICE_PROFILE__XCT_G711FAX__CONTROLTYPE, nVoiceProfileInstNum, &s.ControlType );
		*data = strdup(s.ControlType);
    } else {
		return ERR_9005;
	}

    return 0;
}

int setVPXCT_G711FAXEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.FaxT38 */
    int res = 0;
    unsigned int nVoiceProfileInstNum;
    /*union{
        enable_t Enable;
        char ControlType[16];
    } s;*/

    const char *pszLastname = entity->info->name;

    if( (name==NULL) || (data==NULL) || (entity==NULL)) 
        return -1;

    if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
        return -1;

    if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )  /* convert to zero based */
        return -1;
            
    if( entity->info->type != type )
        return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "Enable" ) == 0 ) {
       res = mib_set_type1( MIB_VOICE_PROFILE__XCT_G711FAX__ENABLE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "ControlType" ) == 0  ){
		res = mib_set_type1( MIB_VOICE_PROFILE__XCT_G711FAX__CONTROLTYPE, nVoiceProfileInstNum, data );
    } else {
        return ERR_9005;
    }    

    return res; //1->self-reboot, 0->apply without reboot
}


int getVPXCT_IADDiagEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_IADDiagnostics */
	unsigned int nVoiceProfileInstNum;
	union {
        char IADDiagnosticsState[32];
        unsigned int TestServer;
        unsigned int RegistResult;
        unsigned int Reason;
	} s;
	extern int gVoipReq;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;
	
	if(!gVoipReq){
		cwmpSendRequestToSolar();
		gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
	}
    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "IADDiagnosticsState" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__IADDIAGNOSTICSSTATE, nVoiceProfileInstNum, &s.IADDiagnosticsState );
		*data = strdup( s.IADDiagnosticsState );
	} else if( strcmp( pszLastname, "TestServer" ) == 0 ) { 
	    mib_get_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__TESTSERVER, nVoiceProfileInstNum, &s.TestServer );
		*data = uintdup( s.TestServer );
    } else if( strcmp( pszLastname, "RegistResult" ) == 0 ) { 
        mib_get_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__REGISTRESULT, nVoiceProfileInstNum, &s.RegistResult );
		*data = uintdup( s.RegistResult );
    } else if( strcmp( pszLastname, "Reason" ) == 0 ){ 
        mib_get_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__REASON, nVoiceProfileInstNum, &s.Reason );
		*data = uintdup( s.Reason );
    } else {
		return ERR_9005;
	}

    return 0;
}

int setVPXCT_IADDiagEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/*  VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_IADDiagnostics  */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	extern int gVoipReq;
	extern cwmpEvtMsg pEvtMsg; 	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;
	
	if(gVoipReq!=EVT_VOICEPROFILE_LINE_SET_STATUS){
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}
	
    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );
    
	if( strcmp( pszLastname, "IADDiagnosticsState" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__IADDIAGNOSTICSSTATE, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "TestServer" ) == 0 ) { 
    	/* E8C: TBD  NOT Support yet */
        res = mib_set_type1( MIB_VOICE_PROFILE__XCT_IADDIAG__TESTSERVER, nVoiceProfileInstNum, data );
    } else if( strcmp( pszLastname, "RegistResult" ) == 0 ) { 
		return ERR_9008;	/* Read only */
    } else if( strcmp( pszLastname, "Reason" ) == 0 ){ 
        return ERR_9008;	/* Read only */
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPXCT_FaxModemEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_FaxModem */
	unsigned int nVoiceProfileInstNum;
	union {
        int FaxNego;
	} s;
	extern int gVoipReq;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;	
	*data = NULL;
	
	if( !gVoipReq ) {
		cwmpSendRequestToSolar();
		gVoipReq = EVT_VOICEPROFILE_LINE_GET_STATUS;
	}

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "FaxNego" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__XCT_FAXMODEM__FAXNEGO, nVoiceProfileInstNum, &s.FaxNego );
		*data = intdup( s.FaxNego );
    } else {
		return ERR_9005;
	}

    return 0;
}

int setVPXCT_FaxModemEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/*  VoiceService.{i}.VoiceProfile.{i}.X_"CTCOM_NAME_PREFIX"_FaxModem  */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	extern int gVoipReq;
	extern cwmpEvtMsg pEvtMsg; 	
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;
	
	if( gVoipReq != EVT_VOICEPROFILE_LINE_SET_STATUS ) {
		gVoipReq = EVT_VOICEPROFILE_LINE_SET_STATUS;
		cwmpEvtMsgSetEvent(&pEvtMsg, EVT_VOICEPROFILE_LINE_SET_STATUS);
	}
	
    CWMPDBG( 3, ( stderr, "<%s:%d> set %s value \n", __FUNCTION__, __LINE__, name) );

    if( strcmp( pszLastname, "FaxNego" ) == 0 ) {
        res = mib_set_type1( MIB_VOICE_PROFILE__XCT_FAXMODEM__FAXNEGO, nVoiceProfileInstNum, data );
    } else {
        return ERR_9005;
    }

    return res;	//1->self-reboot, 0->apply without reboot
}

int getVPServiceProviderEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo */
	unsigned int nVoiceProfileInstNum;
	char szName[ 256 ];
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity->info->type;
    *data = NULL;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );
		
	if( strcmp( pszLastname, "Name" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME, nVoiceProfileInstNum, &szName );
		*data = strdup( szName );
	} else {
		*data = NULL;
		return ERR_9005;
	}
	return 0;
}

int setVPServiceProviderEntity(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo */
    int res = 0;
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity->info->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity->info->type != type )
		return ERR_9006;

    CWMPDBG( 3, ( stderr, "<%s:%d> get %s value \n", __FUNCTION__, __LINE__, name) );

	if( strcmp( pszLastname, "Name" ) == 0 ) {
		res = mib_set_type1( MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME, nVoiceProfileInstNum, data );
	} else {
		return ERR_9005;
	}
	return res;	//1->self-reboot, 0->apply without reboot
}


#if 0
static int getVoiceProfileSipEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setVoiceProfileSipEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

static int getServiceProviderInfoEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setServiceProviderInfoEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

static int getVoiceProfileEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data);
static int setVoiceProfileEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data);

struct sCWMP_ENTITY tVoiceProfileSipEntity[] = {
/*	{ name,								type,			flag,					accesslist,	getvalue,					setvalue,					next_table,	sibling } */
	{ "ProxyServer",					eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "ProxyServerPort",				eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "ProxyServerTransport",			eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "RegistrationServer",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "RegistrationServerPort",			eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "RegistrationServerTransport",	eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "UserAgentDomain",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "UserAgentPort",					eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "UserAgentTransport",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "OutboundProxy",					eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "OutboundProxyPort",				eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "Organization",					eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "RegistrationPeriod",				eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileSipEntity, 	setVoiceProfileSipEntity,	NULL,		NULL },
	{ "",								eCWMP_tNONE,	0,						NULL,		NULL,						NULL,						NULL,		NULL },
};

struct sCWMP_ENTITY tServiceProviderInfoEntity[] = {
/*	{ name,					type,			flag,					accesslist,	getvalue,						setvalue,						next_table,	sibling } */
	{ "Name",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getServiceProviderInfoEntity, 	setServiceProviderInfoEntity,	NULL,		NULL },
	{ "",					eCWMP_tNONE,	0,						NULL,		NULL,							NULL,							NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfileEntity[] = {
/*	{ name,					type,			flag,					accesslist,	getvalue,				setvalue,				next_table,					sibling } */
	{ "Enable",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "Reset",				eCWMP_tBOOLEAN,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "NumberOfLines",		eCWMP_tUINT,	CWMP_READ,				NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "Name",				eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "SignalingProtocol",	eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "MaxSessions",		eCWMP_tUINT,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "DTMFMethod",			eCWMP_tSTRING,	CWMP_READ | CWMP_WRITE,	NULL,		getVoiceProfileEntity, 	setVoiceProfileEntity,	NULL,						NULL },
	{ "ServiceProviderInfo",eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 					NULL,					tServiceProviderInfoEntity,	NULL },
	{ "SIP",				eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 					NULL,					tVoiceProfileSipEntity,		NULL },
	{ "Line",				eCWMP_tOBJECT,	CWMP_READ,				NULL,		NULL, 					objVoiceProfileLine,	NULL,						NULL },
	{ "",					eCWMP_tNONE,	0,						NULL,		NULL,					NULL,					NULL,		NULL },
};

struct sCWMP_ENTITY tVoiceProfile[] = {
/*	{ name,		type,			flag,								accesslist,	getvalue,	setvalue,	next_table,				sibling } */
	{ "0",		eCWMP_tOBJECT,	CWMP_READ|CWMP_WRITE|CWMP_LNKLIST,	NULL,		NULL,		NULL,		tVoiceProfileEntity, 	NULL },
};

static int getVoiceProfileSipEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP */
	unsigned int nVoiceProfileInstNum;
	union {
		char ProxyServer[ 256 ];
		unsigned int ProxyServerPort;
		transport_t ProxyServerTransport;
		char RegistrationServer[ 256 ];
		unsigned int RegistrationServerPort;
		transport_t RegistrationServerTransport;
		char UserAgentDomain[ 256 ];
		unsigned int UserAgentPort;
		transport_t UserAgentTransport;
		char OutboundProxy[ 256 ];
		unsigned int OutboundProxyPort;
		char Organization[ 256 ];
		unsigned int RegistrationPeriod;
	} s;
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;	
		
	if( strcmp( pszLastname, "ProxyServer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER, nVoiceProfileInstNum, &s.ProxyServer );
		*data = strdup( s.ProxyServer );
	} else if( strcmp( pszLastname, "ProxyServerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT, nVoiceProfileInstNum, &s.ProxyServerPort );
		*data = uintdup( s.ProxyServerPort );
	} else if( strcmp( pszLastname, "ProxyServerTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
		*data = strdup_Transport( s.ProxyServerTransport );
	} else if( strcmp( pszLastname, "RegistrationServer" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER, nVoiceProfileInstNum, &s.RegistrationServer );
		*data = strdup( s.RegistrationServer );
	} else if( strcmp( pszLastname, "RegistrationServerPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, &s.RegistrationServerPort );
		*data = uintdup( s.RegistrationServerPort );
	} else if( strcmp( pszLastname, "RegistrationServerTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrationServerTransport );
		*data = strdup_Transport( s.RegistrationServerTransport );
	} else if( strcmp( pszLastname, "UserAgentDomain" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN, nVoiceProfileInstNum, &s.UserAgentDomain );
		*data = strdup( s.UserAgentDomain );
	} else if( strcmp( pszLastname, "UserAgentPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT, nVoiceProfileInstNum, &s.UserAgentPort );
		*data = uintdup( s.UserAgentPort );
	} else if( strcmp( pszLastname, "UserAgentTransport" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_TRANSPORT, nVoiceProfileInstNum, &s.UserAgentTransport );
		*data = strdup_Transport( s.UserAgentTransport );
	} else if( strcmp( pszLastname, "OutboundProxy" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY, nVoiceProfileInstNum, &s.OutboundProxy );
		*data = strdup( s.OutboundProxy );
	} else if( strcmp( pszLastname, "OutboundProxyPort" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, &s.OutboundProxyPort );
		*data = uintdup( s.OutboundProxyPort );
	} else if( strcmp( pszLastname, "Organization" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__ORGANIZATION, nVoiceProfileInstNum, &s.Organization );
		*data = strdup( s.Organization );
	} else if( strcmp( pszLastname, "RegistrationPeriod" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD, nVoiceProfileInstNum, &s.RegistrationPeriod );
		*data = uintdup( s.RegistrationPeriod );
	} else {
		*data = NULL;
		return ERR_9005;
	}
	
	return 0;
}

static int setVoiceProfileSipEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.SIP */
	unsigned int nVoiceProfileInstNum;
	union {
		char ProxyServer[ 256 ];
		unsigned int ProxyServerPort;
		transport_t ProxyServerTransport;
		char RegistrationServer[ 256 ];
		unsigned int RegistrationServerPort;
		transport_t RegistrationServerTransport;
		char UserAgentDomain[ 256 ];
		unsigned int UserAgentPort;
		transport_t UserAgentTransport;
		char OutboundProxy[ 256 ];
		unsigned int OutboundProxyPort;
		char Organization[ 256 ];
		unsigned int RegistrationPeriod;
	} s;
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity ->type != type )
		return ERR_9006;
	
	if( strcmp( pszLastname, "ProxyServer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "ProxyServerPort" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_PORT, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "ProxyServerTransport" ) == 0 ) {
		if( str2id_SignalingProtocol( data, &s.ProxyServerTransport ) ){
			mib_set_type1( MIB_VOICE_PROFILE__SIP__PROXY_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.ProxyServerTransport );
			return 1;
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "RegistrationServer" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "RegistrationServerPort" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_PORT, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "RegistrationServerTransport" ) == 0 ) {
		if( str2id_SignalingProtocol( data, &s.RegistrationServerTransport ) ){
			mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRAR_SERVER_TRANSPORT, nVoiceProfileInstNum, &s.RegistrationServerTransport );
			return 1;
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "UserAgentDomain" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_DOMAIN, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "UserAgentPort" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_PORT, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "UserAgentTransport" ) == 0 ) {
		if( str2id_SignalingProtocol( data, &s.UserAgentTransport ) ){
			mib_set_type1( MIB_VOICE_PROFILE__SIP__USER_AGENT_TRANSPORT, nVoiceProfileInstNum, &s.UserAgentTransport );
			return 1;
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "OutboundProxy" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "OutboundProxyPort" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__OUTBOUND_PROXY_PORT, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "Organization" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__ORGANIZATION, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "RegistrationPeriod" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SIP__REGISTRATION_PERIOD, nVoiceProfileInstNum, data );
		return 1;
	} else {
		return ERR_9005;
	}	
	
	return 0;
}

static int getServiceProviderInfoEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo */
	unsigned int nVoiceProfileInstNum;
	char szName[ 256 ];
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
	
	*type = entity ->type;	
		
	if( strcmp( pszLastname, "Name" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME, nVoiceProfileInstNum, &szName );
		*data = strdup( szName );
	} else {
		*data = NULL;
		return ERR_9005;
	}
	
	return 0;
}

static int setServiceProviderInfoEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo */
	unsigned int nVoiceProfileInstNum;
	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity ->type != type )
		return ERR_9006;
	
	if( strcmp( pszLastname, "Name" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__SERVICE_PROVIDE_INFO__NAME, nVoiceProfileInstNum, data );
	} else {
		return ERR_9005;
	}	
	
	return 0;
}

static int getVoiceProfileEntity(char *name, struct sCWMP_ENTITY *entity, int *type, void **data)
{
	/* VoiceService.{i}.VoiceProfile.{i}. */
	unsigned int nVoiceProfileInstNum;
	union {
		enable_t Enable;
		int Reset;
		unsigned int NumberOfLines;
		char Name[ 40 ];
		signaling_protocol_t SignallingProtocol;
		unsigned int MaxSessions;
		DTMF_method_t DTMFMethod;
	} s;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;

	*type = entity ->type;

	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__ENABLE, nVoiceProfileInstNum, &s.Enable );
		*data = strdup_Enable( s.Enable );
	} else if( strcmp( pszLastname, "Reset" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__RESET, nVoiceProfileInstNum, &s.Reset );
		*data = booldup( s.Reset );
	} else if( strcmp( pszLastname, "NumberOfLines" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__NUMBER_OF_LINES, nVoiceProfileInstNum, &s.NumberOfLines );
		*data = uintdup( s.NumberOfLines );
	} else if( strcmp( pszLastname, "Name" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__NAME, nVoiceProfileInstNum, &s.Name );
		*data = strdup( s.Name );
	} else if( strcmp( pszLastname, "SignalingProtocol" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__SIGNALING_PROTOCOL, nVoiceProfileInstNum, &s.SignallingProtocol );
		*data = strdup_SignalingProtocol( s.SignallingProtocol );
	} else if( strcmp( pszLastname, "MaxSessions" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__MAX_SESSIONS, nVoiceProfileInstNum, &s.MaxSessions );
		*data = uintdup( s.MaxSessions );
	} else if( strcmp( pszLastname, "DTMFMethod" ) == 0 ) {
		mib_get_type1( MIB_VOICE_PROFILE__DTMF_METHOD, nVoiceProfileInstNum, &s.DTMFMethod );
		*data = strdup_DTMFMethod( s.DTMFMethod );	
	} else {
		*data = NULL;
		return ERR_9005;
	}

	return 0;
}

static int setVoiceProfileEntity(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
{
	/* VoiceService.{i}.VoiceProfile.{i}. */
	unsigned int nVoiceProfileInstNum;
	union {
		enable_t Enable;
		int Reset;
		unsigned int NumberOfLines;
		char Name[ 40 ];
		signaling_protocol_t SignallingProtocol;
		unsigned int MaxSessions;
		DTMF_method_t DTMFMethod;
	} s;

	const char *pszLastname = entity ->name;

	if( (name==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;
		
	if( !GetOneBasedInstanceNumber_VoiceProfile( name, &nVoiceProfileInstNum ) )
		return -1;
	
	if( -- nVoiceProfileInstNum >= MAX_PROFILE_COUNT )	/* convert to zero based */
		return -1;
		
	if( entity ->type != type )
		return ERR_9006;

	if( strcmp( pszLastname, "Enable" ) == 0 ) {
		if( str2id_Enable( data, &s.Enable ) ){
			mib_set_type1( MIB_VOICE_PROFILE__ENABLE, nVoiceProfileInstNum, &s.Enable );
			return 1;
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "Reset" ) == 0 ) {
		s.Reset = *( ( int * )data );
		mib_set_type1( MIB_VOICE_PROFILE__RESET, nVoiceProfileInstNum, &s.Reset );
		return 1;
	} else if( strcmp( pszLastname, "NumberOfLines" ) == 0 ) {
		return ERR_9008;	/* Read only */
	} else if( strcmp( pszLastname, "Name" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__NAME, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "SignalingProtocol" ) == 0 ) {
		if( str2id_SignalingProtocol( data, &s.SignallingProtocol ) ){
			mib_set_type1( MIB_VOICE_PROFILE__SIGNALING_PROTOCOL, nVoiceProfileInstNum, &s.SignallingProtocol );
			return 1;
		}else{
			return ERR_9007;
		}
	} else if( strcmp( pszLastname, "MaxSessions" ) == 0 ) {
		mib_set_type1( MIB_VOICE_PROFILE__MAX_SESSIONS, nVoiceProfileInstNum, data );
		return 1;
	} else if( strcmp( pszLastname, "DTMFMethod" ) == 0 ) {
		if( str2id_DTMFMethod( data, &s.DTMFMethod ) ){
			mib_set_type1( MIB_VOICE_PROFILE__DTMF_METHOD, nVoiceProfileInstNum, &s.DTMFMethod );
			return 1;
		}else{
			return ERR_9007;
		}
	} else {
		return ERR_9005;
	}
	
	return 1;
}

int objVoiceProfile(char *name, struct sCWMP_ENTITY *entity, int type, void *data)
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
			
			if( create_Object( c, tVoiceProfile, sizeof(tVoiceProfile), MAX_PROFILE_COUNT, 1 ) < 0 )
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
