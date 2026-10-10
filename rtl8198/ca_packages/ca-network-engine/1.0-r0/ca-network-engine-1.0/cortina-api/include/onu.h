#ifndef _ONU_H_
#define _ONU_H_

#include "ca_types.h"

#define CA_ONU_LOID_LEN                   (24)
#define CA_ONU_LOID_PW_LEN                (12)
#define CA_ONU_VENDOR_ID_LEN              (4)
#define CA_ONU_SN_LEN                     (8)
#define CA_ONU_MAC_ADDR_LEN               (6)

#define CA_XGPN_ONU_VERSION_LEN           (14)
#define CA_XGPN_ONU_EQID_LEN              (20)

typedef struct
{
    ca_uint32_t  ipv4Addr;
    ca_uint32_t  ipv4Mask;
    ca_uint32_t  ipv4Gw;
    ca_uint32_t  ponNum;
    ca_uint32_t  ethNum;
    ca_uint32_t  potsNum;
    ca_boolean_t butteryBackup;
    ca_boolean_t ipv6Flag;

    ca_uint8_t   loid[CA_ONU_LOID_LEN];
    ca_uint8_t   passwd[CA_ONU_LOID_PW_LEN];
    ca_uint8_t   vendorId[CA_ONU_VENDOR_ID_LEN];
    ca_uint8_t   sn[CA_ONU_SN_LEN];
    ca_uint8_t   mac[CA_ONU_MAC_ADDR_LEN];
}ca_onu_common_info_t;

typedef struct
{
    ca_onu_common_info_t common;
    ca_uint32_t  iphostNum;
    ca_uint32_t  veipNum;
    ca_uint32_t  ponSlot;
    ca_uint32_t  ethSlot;
    ca_uint32_t  iphostSlot;
    ca_uint32_t  potsSlot;
    ca_uint32_t  veipSlot;
    ca_boolean_t remoteDbg;
    ca_boolean_t powerCsvMode;
    ca_boolean_t veipIndepend;
    ca_uint8_t   version[CA_XGPN_ONU_VERSION_LEN];
    ca_uint8_t   eqId[CA_XGPN_ONU_EQID_LEN];
}ca_onu_xgpn_info_t;

ca_status_t ca_onu_info_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_onu_common_info_t* pData
);

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_onu_losState_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *pData
);
#endif

#endif

