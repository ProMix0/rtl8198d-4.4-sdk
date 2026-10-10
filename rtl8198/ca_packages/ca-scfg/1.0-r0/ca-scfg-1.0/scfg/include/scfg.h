#ifndef __SCFG_H__
#define __SCFG_H__

#include <ca_types.h>
#include "scfg_defs.h"
#include "not_scfg.h"

#define CFG_ID_VALUE_LEN        (512)   /*KEY_VALUE_LEN_MAX */
#define CA_DEVICE_CONFIG_TLV_MAX_LEN        128

typedef enum ca_scfg_ecode_enum {
    SCFG_OK,
    SCFG_ERR,
    SCFG_PARAM_ERR,
    SCFG_MALLOC_ERR,
    SCFG_FOPEN_ERR,
    SCFG_PARSE_ERR,
    SCFG_FILEOP_ERR,
    SCFG_ERR_MAX
} ca_scfg_ecode_en;

typedef struct {
    ca_uint32_t type;
    ca_uint32_t len;
    ca_uint8_t value[CA_DEVICE_CONFIG_TLV_MAX_LEN];
} ca_device_config_tlv_t;

ca_uint32 scfg_read(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN const ca_int8_t *scfg_id/*[32:32]*/,
    CA_IN ca_uint32_t len,
    CA_OUT void *pbuf/*[len:130]*/);

ca_uint32 scfg_write(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN const ca_int8_t *scfg_id/*[32:32]*/,
    CA_IN ca_uint32_t len,
    CA_IN void *pbuf/*[len:130]*/);

ca_status_t ca_scfg_read(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int8_t *scfg_id/*[64:64]*/,
    CA_IN ca_uint32_t len,
    CA_OUT ca_uint8_t *pbuf/*[256:256]*/);

ca_status_t ca_scfg_write(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int8_t *scfg_id/*[64:64]*/,
    CA_IN ca_uint32_t len,
    CA_IN ca_uint8_t *pbuf/*[256:256]*/);

ca_status_t ca_device_config_tlv_get(
    CA_IN       ca_device_id_t      device_id,
    CA_IN_OUT   ca_device_config_tlv_t  *p_tlv);

ca_status_t ca_device_config_tlv_set(
    CA_IN       ca_device_id_t      device_id,
    CA_IN_OUT   ca_device_config_tlv_t  *p_tlv);

#endif
