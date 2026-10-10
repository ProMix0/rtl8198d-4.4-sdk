#ifndef __API_COMMON_H__
#define __API_COMMON_H__

#include "osal_common.h"

/* ca_api_debug */

enum CA_API_DEBUG {
	API_DBG_DISABLE		= 0,
	API_DBG_CLS		= 0x00000001,
	API_DBG_NAT		= 0x00000002,
	API_DBG_ROUTE		= 0x00000004,
	API_DBG_NEXTHOP		= 0x00000008,
	API_DBG_INTF		= 0x00000010,
	API_DBG_TUNNEL		= 0x00000020,
	API_DBG_L2_MCAST	= 0x00000040,
	API_DBG_L3_MCAST	= 0x00000080,
	API_DBG_FLOW		= 0x00000100,
	API_DBG_ETH_PORT	= 0x00000200,
	API_DBG_SPPKT		= 0x00000400,
	API_DBG_QOS		= 0x00000800,
	API_DBG_CMN_PORT	= 0x00001000,
	API_DBG_L2_FDB		= 0x00002000,
	API_DBG_RATE		= 0x00004000,
	API_DBG_QUEUE		= 0x00008000,
	API_DBG_MACSEC		= 0x00010000,
	API_DBG_PTP		= 0x00020000,
	API_DBG_MIRROR		= 0x00040000,
	API_DBG_SYS		= 0x00080000,
};

extern ca_uint32_t ca_api_debug;

/* ca_api_dump */

enum CA_API_DUMP {
	API_DUMP_CLS_RULE	= 0,
	API_DUMP_NAT_TBL,
	API_DUMP_ROUTE_TBL,
	API_DUMP_NEXTHOP_TBL,
	API_DUMP_INTF_TBL,
	API_DUMP_TUNNEL_TBL,
	API_DUMP_L2_MCAST_TBL,
	API_DUMP_L3_MCAST_TBL,
	API_DUMP_FLOW_TYPE,
	API_DUMP_FLOW_TBL,
	API_DUMP_SW_SHAPER_TBL,
	API_DUMP_US_RATE_MANAGER_TBL,
	API_DUMP_MACSEC_TBL,
	API_DUMP_ETH_PORT,
    API_DUMP_QOS,
	API_DUMP_PON,
};

#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)
/* ca_api_dump */
enum CA_API_WMC_FLOODING {
        API_WMC_FLOODING_STATUS       = 0,      /*Get status of well known multicast status*/
        API_WMC_FLOODING_DISABLE,               /*Disable well known multicast flooding rule*/
        API_WMC_FLOODING_ENABLE_WITH_DEFAULT_RULES,	/*Enable well known multicast flooding rule*/
        API_WMC_FLOODING_ADD_ADDRESS,           /*Add well known multicast IPv4 address range*/
        API_WMC_FLOODING_ADD_V6_ADDRESS,        /*Add well known multicast IPv6 address range*/
        API_WMC_FLOODING_ADD,                   /*Add well known multicast flooding rule with VLAN ID*/
        API_WMC_FLOODING_UPDATE,                /*Update well known multicast flooding rule with VLAN ID*/
        API_WMC_FLOODING_DELETE,                /*Delete well known multicast flooding rule with VLAN ID*/
        API_WMC_FLOODING_DELETE_ALL,            /*Delete all well known multicast flooding rule*/
        API_WMC_FLOODING_ENABLE,			/*Enable well-known multicast without applying any rule*/
        API_WMC_FLOODING_SET_FILTER_MODE,		/*Set offload src filter mode: 0 disable, 1 enable*/
        API_WMC_FLOODING_SET_DEBUG_MODE,		/*Set offload debug mode: 0 disable, 1 enable*/
};

#endif

#endif /* __API_COMMON_H__ */

