#ifndef __PON_EXPORT_H__
#define __PON_EXPORT_H__
#include "ca_types.h"

typedef enum {
    CA_PON_POLICER_MODE_DISABLE         = 0,    /* policer disabled                     */
    CA_PON_POLICER_MODE_SRTCM           = 1,    /* rfc2697 mode, not support in CS8160  */
    CA_PON_POLICER_MODE_TRTCM           = 2,    /* rfc2698 mode                         */
    CA_PON_POLICER_MODE_TRTCMDS         = 3,    /* rfc4115 (cflag = 0)                  */
    CA_PON_POLICER_MODE_TRTCMDS_COUPLED = 4,    /* rfc4115 with coupled flag (cflag = 1)*/
    CA_PON_POLICER_MODE_RATE_ONLY       = 5     /* rate limiter mode, no color marking  */
} ca_pon_policer_mode_t;

typedef struct {
    ca_pon_policer_mode_t        mode; /* policer mode            */
    ca_boolean_t      pps;  /* True - PPS mode, false - PBS mode */
    ca_uint32_t       cir;  /* commited rate           */
    ca_uint32_t       cbs;  /* commited burst size     */
    ca_uint32_t       pir;  /* eir for rfc4115         */
    ca_uint32_t       pbs;  /* ebs for rfc4115&rfc2697 */
} ca_pon_policer_t;

typedef ca_uint32_t (*ca_pon_tx_cb)(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id,
    void *pkt);
ca_status_t ca_pon_tx_register(/*CODEGEN_IGNORE_TAG*/
    ca_uint32_t (*handler)(ca_device_id_t device_id, void *pkt));


ca_status_t ca_pon_tx_unregister(/*CODEGEN_IGNORE_TAG*/void);


typedef ca_status_t (*ca_pon_policer_func_t) (ca_device_id_t device_id, ca_uint16_t flow_id, ca_pon_policer_t *policer_cfg);
extern ca_pon_policer_func_t ca_pon_policer_callback;

ca_status_t ca_pon_laser_tx_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t data);

#if defined(CONFIG_LUNA_G3_SERIES)&&defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)
ca_status_t ca_pon_laser_tx_disable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t data);
#endif

#endif
