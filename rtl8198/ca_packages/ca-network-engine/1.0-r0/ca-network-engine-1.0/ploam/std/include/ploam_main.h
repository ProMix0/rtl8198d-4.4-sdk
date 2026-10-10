#ifndef __PLOAM_MAIN_H__
#define __PLOAM_MAIN_H__

#ifdef CONFIG_NE_PON

#include "ploam_api.h"

#define PLOAM_DEV_ID        0
#define PLOAM_PORT_ID       0

#define CS_SN_LEN               8
#define CS_PLOAM_VENDOR_ID_LEN  4
#define CS_PLOAM_PKT_LEN        14  /* first byte means entry number */

#define PLOAM_THREAD_NAME         "ca_ploam"


#define PLOAM_QUEUE_NAME          "ploam_queue"
#define PLOAM_QUEUE_PRIORITY      30

#define PLOAM_MQ_MSG_HDR_LEN      32
#define PLOAM_MQ_MSG_MAX_LEN      128
#define PLOAM_MQ_MSG_MAX_NUM      10
#define PLOAM_MQ_MSG_LEN          (PLOAM_MQ_MSG_HDR_LEN + PLOAM_MQ_MSG_MAX_LEN)

#define PLOAM_PACKET              1


typedef ca_uint8_t    ca_sn_t[CS_SN_LEN];

typedef struct
{
    ca_uint8_t        vendor_id[CS_PLOAM_VENDOR_ID_LEN];
    ca_uint32_t       serial_num;
} ca_ploam_sn_t;

typedef struct {
    struct work_struct work;
    void*  data;
} ca_ploam_work_data_t;

typedef struct {
    struct workqueue_struct *wq;
    ca_ploam_work_data_t work_data;
} ca_ploam_wq_t;

/* ploam control block */
typedef struct
{
    ca_uint           lock;
    ca_uint           wq_id;

    ca_boolean_t      es_state;   /* ONU in O7 state or not */
    ca_ploam_sn_t     sn;

    ca_uint32_t       onu_state_handle;
    ca_uint32_t       alarm_mem_handle;
    ca_ploam_wq_t     ploam_wq;
} ca_ploam_t;

/* ploam pkt struct */
typedef struct
{
    ca_uint8_t        onu_id;
    ca_uint8_t        msg_id;
    ca_uint8_t        data[10];
    ca_uint8_t        crc;
} __attribute__((packed)) ca_ploam_pkt_t;

typedef struct
{
    ca_uint8_t        left_entry_num; /* left entry after this time reading */
    ca_ploam_pkt_t  pkt;
} __attribute__((packed)) ca_ploam_msg_t;

typedef struct
{
    ca_uint8_t        onu_id;
    ca_uint8_t        msg_id;
    ca_uint8_t        data[10];
    ca_uint8_t        crc;
} __attribute__((packed)) ploam_pkt_gpon_t;

#define PLOAM_OVERHEAD_SIZE     14    /* SA, DA, ETHERNET-TYPE, total 14 bytes */
#define PLOAM_NGPON2_PKT_SIZE   48

typedef struct
{
    ca_uint16_t       onu_id;
    ca_uint8_t        msg_id;
    ca_uint8_t        seq_no;
    ca_uint8_t        data[36];
    ca_uint8_t        mic[8];
} __attribute__((packed)) ploam_pkt_ngpon2_t;


typedef struct {
    ca_uint16_t     type;   /* message type */
    ca_uint16_t     len;    /* data length */
} ploam_msg_hdr_t;

typedef struct {
    ploam_msg_hdr_t      hdr;
    ca_uint8_t           data[PLOAM_MQ_MSG_MAX_LEN];
} ploam_msg_t;


#ifdef ZTE_SPECIFIC

/* PLOAM message ID */
#define PLOAM_MSG_ID_TRCVR_TX_CTRL      0x81
#define PLOAM_MSG_ID_ONU_REBOOT         0x82

/* Transceiver TX control action */
#define PLOAM_LASER_ON_BY_SN            0xFF
#define PLOAM_LASER_ON                  0x1F
#define PLOAM_LASER_OFF_BY_SN           0x11
#define PLOAM_LASER_BRST_LOCKED_BY_SN   0x10
#define PLOAM_LASER_BRST_NORMAL_BY_SN   0x0F
#define PLOAM_LASER_BRST_LOCKED         0x01
#define PLOAM_LASER_BRST_NORMAL         0x00

/* GPIO */
#define PLOAM_TX_POWER_GPIO             6

typedef struct
{
    ca_uint8_t        action;
    cs_sn_t         sn;
    ca_uint8_t        rscv;
} __attribute__((packed)) cs_ploam_trcvr_tx_ctrl_t;

/* ONU Reboot action */
#define PLOAM_ONU_REBOOT                0xFF
#define PLOAM_ONU_REBOOT_BY_SN          0x01

typedef struct
{
    ca_uint8_t        action;
    cs_sn_t         sn;
    ca_uint8_t        rscv;
} __attribute__((packed)) cs_ploam_onu_reboot_t;

#endif/*END_ZTE_SPECIFIC*/

/*
 * external API
 */
/*
void ca_ploam_alarm_mem_proc();
void ca_ploam_onu_state_chg_proc(void *data, cs_uint16 data_len);
*/
ca_status_t ca_ploam_tx(ca_int8_t *buffer, ca_uint32_t len, ca_uint16_t port_id);


#endif/*END_CONFIG_NE_PON*/

#endif

