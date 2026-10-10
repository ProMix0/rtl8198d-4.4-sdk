#ifndef __PLOAM_NGPON2_H__
#define __PLOAM_NGPON2_H__

#include "ploam_main.h"
#include "aal_ploam.h"

#define PON_TAG_SIZE 8
#define PLOAM_NGPON2_MSG_SIZE       48

#define PLOAM_TX_RETRY_MAX          1500

#define __PLOAM_OP_ACK       0
#define __PLOAM_OP_NACK      1
#define __PLOAM_OP_COMPLETE  3
#define __PLOAM_OP_ROLLBACK  4

#define TUN_RESP_CODE_INT_SFC 0

#define __NACK_DS_ALBL 1
#define __NACK_DS_VOID 2
#define __NACK_DS_PART 3
#define __NACK_DS_TUNR 4
#define __NACK_DS_LNRT 5
#define __NACK_DS_LNCD 6
#define __NACK_US_ALBL 7
#define __NACK_US_VOID 8
#define __NACK_US_PART 9
#define __NACK_US_TUNR 10
#define __NACK_US_LNRT 11
#define __NACK_US_LNCD 12

#define __ROLLBACK_COM_DS  0
#define __ROLLBACK_DS_ALBL 1
#define __ROLLBACK_DS_LKTP 2
#define __ROLLBACK_US_ALBL 3
#define __ROLLBACK_US_VOID 4
#define __ROLLBACK_US_TUNR 5
#define __ROLLBACK_US_LKTP 6
#define __ROLLBACK_US_LNRT 7
#define __ROLLBACK_US_LNCD 8


typedef enum {
    PLOAM_NGPON2_US_SERIAL_NUMBER = 1,
    PLOAM_NGPON2_US_REGISTRATION = 0x2,
    PLOAM_NGPON2_US_KEY_REPORT = 0x5,
    PLOAM_NGPON2_US_ACK = 0x9,
    PLOAM_NGPON2_US_SLEEP_REQ = 0x10,
    PLOAM_NGPON2_US_TUNING_RESP = 0x1A,
    PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT = 0x1B,
    PLOAM_NGPON2_US_RATE_RESP = 0x1C,
    PLOAM_NGPON2_US_MAX
} ploam_ngpon2_us_e;

typedef enum {
    PLOAM_NGPON2_DS_BURST_PROFILE = 0x1,
    PLOAM_NGPON2_DS_ASSIGN_ONUID = 0x3,
    PLOAM_NGPON2_DS_RANGING_TIME = 0x4,
    PLOAM_NGPON2_DS_DEACT_ONUID = 0x5,
    PLOAM_NGPON2_DS_DISABLE_SN = 0x6,
    PLOAM_NGPON2_DS_REQUEST_REGISTRATION = 0x9,
    PLOAM_NGPON2_DS_ASSIGN_ALLOCID = 0xA,
    PLOAM_NGPON2_DS_KEY_CTRL = 0xD,
    PLOAM_NGPON2_DS_SLEEP_ALLOW = 0x12,
    PLOAM_NGPON2_DS_CALIBRATION_REQ = 0x13,
    PLOAM_NGPON2_DS_ADJUST_TX_WAVELEN = 0x14,
    PLOAM_NGPON2_DS_TUNING_CTRL = 0x15,
    PLOAM_NGPON2_DS_SYS_PROFILE = 0x17,
    PLOAM_NGPON2_DS_CHANNEL_PROFILE = 0x18,
    PLOAM_NGPON2_DS_PROTECTION_CTRL = 0x19,
    PLOAM_NGPON2_DS_CHANGE_POWER_LEVEL = 0x1A,
    PLOAM_NGPON2_DS_POWER_CONSUMPTION_INQUIRE = 0x1B,
    PLOAM_NGPON2_DS_RATE_CTRL = 0x1C,
    PLOAM_NGPON2_DS_MAX
}ploam_ngpon2_ds_e;



extern ca_uint8_t pon_tag[PON_TAG_SIZE];
extern ca_uint8_t omci_ik[16];
extern ca_uint8_t ploam_ik[16];
extern ca_uint8_t kek[16];

typedef ca_status_t (*ploam_handler_t)(void* data_p);

ca_status_t ploam_pkt_ngpon2_proc(
    ca_uint8 *frame_ptr, ca_uint32_t len);

ca_status_t ploam_ngpon2_us_msg_proc(
    void *msg);

ca_status_t ploam_ngpon2_us_ploam_def_val_get(
    ca_uint8_t msg_id,
    void *data);

ca_status_t ploam_ngpon2_us_ploam_def_val_set(
    ca_uint8_t msg_id,
    void *data);

ca_status_t ploam_ngpon2_ds_ploam_def_val_get(
    ca_uint8_t msg_id,
    void *data);

ca_status_t ploam_ngpon2_ds_ploam_def_val_set(
    ca_uint8_t msg_id,
    void *data);


ca_status_t ploam_ngpon2_build_ploam(
    ca_uint16_t onu_id,
    ca_uint8_t  msg_id,
    ca_uint8_t  seq_no,
    ca_uint8    ploam_buffer[]
);

ca_status_t ploam_ngpon2_send_ploam(ca_uint8_t ploam_buffer[], ca_uint8_t frame_size, ca_uint8_t use_def_key);


ca_status_t aal_ploam_ds_handler_reg(
    ca_uint8_t      msg_id,
    ploam_handler_t pfunc);

ca_status_t aal_ploam_ds_handler_unreg(
    ca_uint8_t      msg_id,
    ploam_handler_t pfunc);

#endif /* __PLOAM_NGPON2_H__ */
