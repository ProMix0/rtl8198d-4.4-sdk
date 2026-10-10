#ifndef __AAL_XGPON_INTR_H__
#define __AAL_XGPON_INTR_H__

#include "ca_types.h"

#define CA_PON_AES_INVAILD_VALUE 0xFF
/* XGPON interrupt index  */
typedef enum {
    AAL_XGPON_INTR_ACTP   = 0,
    AAL_XGPON_INTR_PLOAM  = 1,
    AAL_XGPON_INTR_AES_PLOAM = 2,
    AAL_XGPON_INTR_AES    = 3,
    AAL_XGPON_INTR_RXCTL  = 4,
    AAL_XGPON_INTR_BWMP   = 5,
    AAL_XGPON_INTR_USBG   = 6,
    AAL_XGPON_INTR_DSFP   = 7,
    AAL_XGPON_INTR_FEC    = 8,
    /* add intr type here */

    AAL_XGPON_INTR_NUM
}aal_xgpon_intr_t;


typedef enum {
    AAL_PON_KMSG_CHN_ACTP  = 0,
    AAL_PON_KMSG_CHN_PLOAM = 1,
    AAL_PON_KMSG_CHN_AES_KEY_CTRL = 2,
    AAL_PON_KMSG_CHN_AES_KEY_TK4 = 3,
    AAL_PON_KMSG_CHN_AES_KEY_TK5 = 4,
    AAL_PON_KSMG_CHN_MISC  = 5,
    AAL_PON_KMSG_CHN_EPON_MAC = 6,
    AAL_PON_KMSG_CHN_MPCP = 7,
    AAL_PON_KMSG_CHN_RGST   = 8,
    AAL_PON_KMSG_CHN_NUM
} aal_xgpon_intr_msg_t;

/* XGPON ACTP interrupt mask bit */
#define AAL_XGPON_INTR_ACTP_ONU_STATE_CHANGE_BIT     0
#define AAL_XGPON_INTR_ACTP_HUNT_SYNC_EXPIRE_BIT     1
#define AAL_XGPON_INTR_ACTP_PROFILE_LEARNING_BIT     2
#define AAL_XGPON_INTR_ACTP_TX_RESPONSE_CROSS_BIT    3
#define AAL_XGPON_INTR_ACTP_REGST_BIT               20
#define AAL_XGPON_INTR_ACTP_DEREGST_BIT             21
#define AAL_XGPON_INTR_ACTP_DISABLE_SN_BIT          22
#define AAL_XGPON_INTR_ACTP_DEACT_ONU_BIT           23
#define AAL_XGPON_INTR_ACTP_LOSS_SYNC_BIT           24
#define AAL_XGPON_INTR_ACTP_TO1_EXPIRE_BIT          25
#define AAL_XGPON_INTR_ACTP_TO2_EXPIRE_BIT          26


/* xgpon ploam interrupt mask bits */
#define AAL_XGPON_INTR_PLMP_WRONG_ONUID_BIT             0
#define AAL_XGPON_INTR_PLMP_BC_SEQ_MISMCH_BIT           1
#define AAL_XGPON_INTR_PLMP_UC_SEQ_MISMCH_BIT           2
#define AAL_XGPON_INTR_PLMP_MIC_CHK_FAIL_BIT            3
#define AAL_XGPON_INTR_PLMP_WRONG_PLOAM_BIT             4
#define AAL_XGPON_INTR_PLMP_KEY_EXCHG_TK4_TIMEOUT_BIT   5
#define AAL_XGPON_INTR_PLMP_KEY_EXCHG_TK5_TIMEOUT_BIT   6
#define AAL_XGPON_INTR_PLMP_KEY_CTRL_BIT                7
#define AAL_XGPON_INTR_PLMP_SLEEP_ALLOW_BIT             8
#define AAL_XGPON_INTR_PLMP_SLEEP_REQ_DONE_BIT          9
#define AAL_XGPON_INTR_PLMP_EQD_ERR_BIT                10
#define AAL_XGPON_INTR_PLMP_BUSY_ERR_BIT               11
#define AAL_XGPON_INTR_PLMP_REGISTRATION_MSG_BIT       12
#define AAL_XGPON_INTR_PLMP_BURST_PRFL_BIT             13
#define AAL_XGPON_INTR_PLMP_TUNING_REQ_BIT             16
#define AAL_XGPON_INTR_PLMP_RXBUF0_RDY_BIT             30
#define AAL_XGPON_INTR_PLMP_RXBUF1_RDY_BIT             31

typedef ca_status_t (*aal_intr_callback_t)(/*CODEGEN_IGNORE_TAG*/ca_uint32_t intr_id, ca_uint32_t intr_sts);
typedef void (*aal_intr_msg_handler_t)(/*CODEGEN_IGNORE_TAG*/ca_uint32_t data0, ca_uint32_t data1, ca_uint32_t data2, void *param);


typedef struct {
    ca_uint32_t  intr;
    ca_uint32_t  st_chg;
    ca_uint32_t  sts;
}aal_xgpon_actp_kmsg_t;

typedef struct {
    ca_uint32_t           intr;
    ca_uint32_t           len;
    ca_uint8_t            ploam_pkt[48];
}aal_xgpon_ploam_kmsg_t;

typedef struct {
    ca_uint32_t  intr;
    ca_uint32_t  sts;
    ca_uint32_t  key_ctrl_stat;
}aal_xgpon_key_exchg_kmsg_t;

typedef struct {
    ca_uint32_t  intr;
}aal_xgpon_rgst_kmsg_t;

typedef struct {
    ca_uint32_t  intr_id;
    ca_uint32_t  sts;
}aal_xgpon_misc_kmsg_t;

ca_status_t aal_xgpon_intr_enable_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx,
    CA_IN ca_uint32_t  enable_bmp);

ca_status_t aal_xgpon_intr_enable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx);

ca_status_t aal_xgpon_intr_disable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx);

ca_status_t aal_xgpon_intr_enable_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_OUT ca_uint32_t *enable_bmp);

ca_status_t aal_xgpon_intr_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_OUT ca_uint32_t *status_bmp);

ca_status_t aal_xgpon_intr_info_get(
    CA_IN  ca_device_id_t  device_id);

ca_status_t aal_xgpon_intr_name_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_IN  ca_uint32_t  name_len,
    CA_OUT char        *name_str /* [name_len:2048] */ );

ca_status_t aal_xgpon_intr_clr(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_IN  ca_uint32_t  clr_bmp);

ca_status_t aal_xgpon_intr_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t  device_id);

ca_status_t aal_xgpon_intr_handler_reg(/*CODEGEN_IGNORE_TAG*/
    aal_xgpon_intr_msg_t   intr_msg,
    aal_intr_msg_handler_t handler);

ca_status_t aal_xgpon_intr_handler_unreg(/*CODEGEN_IGNORE_TAG*/
    aal_xgpon_intr_msg_t   intr_msg,
    aal_intr_msg_handler_t handler);

 ca_status_t aal_pon_kmsg_recv(/*CODEGEN_IGNORE_TAG*/
    ca_uint32_t msg_chn,
    void *msg,
    ca_uint8_t msg_len);

ca_status_t aal_pon_kmsg_send(/*CODEGEN_IGNORE_TAG*/
    ca_uint32_t msg_chn,
    void *msg,
    ca_uint32_t msg_len);

void aal_xgpon_intr_handler(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t device_id, ca_uint32_t irq, ca_uint32_t xgpon_int);

void aal_act_wq_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);
void aal_misc_wq_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);
void aal_ploam_key_exchg_wq_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);
void aal_ploam_wq_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);


#endif /* __AAL_XGPON_INTR_H__ */

