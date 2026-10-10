#ifndef __WMC_FLOODING_H__
#define __WMC_FLOODING_H__
#include "ca_types.h"

#define CA_WMC_GROUP_MAX                           20
#define CA_WMC_GROUP_PORT_MAX                      10

#define CA_WMC_FLOODING_DISABLE				0
#define CA_WMC_FLOODING_VLAN				2
#define CA_WMC_FLOODING_INC_PORTS			4
#define CA_WMC_FLOODING_EXCL_PORTS			8
#define CA_MAX_PORTS					16


#define CA_WMC_FLOODING_IPC_MAX			10
#define CA_WMC_FLOODING_ENABLE_IPC                 0
#define CA_WMC_FLOODING_DISABLE_IPC                1
#define CA_WMC_FLOODING_ADD_IPC                    2
#define CA_WMC_FLOODING_ADD_ADDRESS_IPC            3
#define CA_WMC_FLOODING_UPDATE_IPC                 4
#define CA_WMC_FLOODING_DELETE_IPC                 5
#define CA_WMC_FLOODING_DELETE_ALL_IPC             6
#define CA_WMC_FLOODING_DUMP_IPC                   7
#define CA_WMC_FLOODING_DEBUG_IPC		8
#define CA_WMC_FLOODING_FILTER_IPC		9


ca_status_t ca_well_known_mc_flooding_enable_set(
	CA_IN	ca_device_id_t	device_id,
	CA_IN	ca_boolean_t	enable
);

ca_status_t ca_well_known_mc_flooding_enable_get(
	CA_IN	ca_device_id_t	device_id,
	CA_OUT	ca_boolean_t	*enable
);
#endif
