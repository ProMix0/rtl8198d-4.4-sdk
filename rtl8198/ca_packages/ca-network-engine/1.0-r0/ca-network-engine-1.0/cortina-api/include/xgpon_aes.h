#ifndef _XGPON_AES_H_
#define _XGPON_AES_H_

#include "ca_types.h"
/*#include "osal_common.h"*/
#include <linux/string.h>
#include "util_aes.h"
#include "osal_timer.h"
#include "xgpon.h"

#define RAND_KEY

#define XGPN_AES_KEY_LEN 16
#define XGPN_REG_ID_SIZE 36

#define XGPN_PLMP_INTR_MASK_REGST_MSG        (0x0001 << 12)
#define XGPN_PLMP_INTR_MASK_KEY_CTRL         (0x0001 << 7)
#define XGPN_PLMP_INTR_MASK_TK5_TIMEOUT      (0x0001 << 6)
#define XGPN_PLMP_INTR_MASK_TK4_TIMEOUT      (0x0001 << 5)
#define __KEY_REPORT_TIMEOUT 10000

extern ca_uint_t __xgpon_aes_lock;

typedef enum {
	CA_XGPON_ONU_KN0 = 0,     /* Key Inactive state */
	CA_XGPON_ONU_KN1 = 1,     /* Key Generating state */
	CA_XGPON_ONU_KN2 = 2,     /* Key Ack Waiting state */
	CA_XGPON_ONU_KN3 = 3,     /* Key Ack state */
	CA_XGPON_ONU_KN4 = 4,      /* Key Active state */
	CA_XGPON_ONU_KEY_EX_MAX
} ca_xgpon_onu_key_exchange_state_t;

extern ca_xgpon_key_report_ploam_msg_t g_key_rpt_new_msg;
extern ca_xgpon_key_report_ploam_msg_t g_key_rpt_confirm_msg;

extern ca_xgpon_onu_key_exchange_state_t current_key_exchange_state;
extern ca_xgpon_onu_key_exchange_state_t previous_key_exchange_state;
extern ca_uint32_t  g_key_ctl_new_cnt ;
extern ca_uint32_t  g_key_ctl_cfm_cnt ;
extern ca_uint32_t g_rpt_new_key_cnt ;
extern ca_uint32_t g_rpt_ext_key_cnt ;
extern ca_boolean_t key_flag; //new key or old key
extern ca_boolean_t old_key_flag;
extern ca_boolean_t timeout4_flag;
extern ca_boolean_t timeout5_flag;
//extern ca_uint8_t g_key_idx;
extern ca_uint16_t g_wrong_key_index;

extern ca_uint16_t g_onu_id;
extern ca_uint16_t g_rpt_type;
extern ca_uint8_t g_key_idx ; 
extern ca_uint8_t g_key_len ;
extern ca_uint8_t g_key_ctrl_seqNo;
extern ca_uint8_t g_key_ctrl_flag ;
extern ca_uint8_t g_key_prt_fragment_num;

extern ca_uint8_t kek[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_uc_key[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_encryped_key[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_kek_name[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_uc_key0[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_encryped_key0[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_kek_name0[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_uc_key1[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_encryped_key1[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_kek_name1[XGPN_AES_KEY_LEN];
extern ca_uint8_t g_pon_tag[8];
extern ca_uint32_t g_state_chg_cnt;
#if 1//defined(CONFIG_LUNA_G3_SERIES)
extern ca_uint8_t g_pon_tag_tmp[8];
#endif

#define XGPON_OMCI_MIC_DIR_DOWNSTREAM 1
#define XGPON_OMCI_MIC_DIR_UPSTREAM   2
typedef enum{
	NEW_KEY = 0,
	OLD_KEY = 1,
	INVALID_KEY = 2
}key_old_new_flag_t;

typedef struct{
    ca_uint8_t key_idx;
    key_old_new_flag_t old_new_flag;/*false:old; true:new*/
    ca_boolean_t active_flag;/*false:inactive; true:active*/
}key_flag_t;

typedef struct{
    ca_uint8_t key_idx;
    ca_uint8_t key[XGPN_AES_KEY_LEN];/*false:old; true:new*/
    ca_uint8_t key_type;/*1:unicast key; 2:broadcast key*/
}data_encrpt_key_t;


typedef struct {
	ca_uint8_t sn[8];
	ca_uint8_t pon_tag[8];
} ca_sk_msg_t;

typedef struct {
	ca_uint8_t pon_tag[8];
	ca_uint8_t registration_id[36];
} ca_digest_config_t;



typedef struct {
	ca_boolean_t key_control_new_flag;
	ca_boolean_t key_control_confirm_flag;
	ca_boolean_t tk4_timeout_flag;
	ca_boolean_t tk5_timeout_flag;
} ca_xgpon_key_exchange_event_t;

/*#define the macros below to 1/0 to enable/disable the mode of operation.

CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

The #ifndef-guard allows it to be configured before #include'ing or at compile time.*/
ca_status_t ca_generate_msk(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  reg_id_size,
    CA_IN ca_uint8_t *registration_id,
    CA_IN ca_uint8_t  msk_size,
    CA_OUT ca_uint8_t *msk );

ca_status_t ca_generate_sk(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *msk,
    CA_IN ca_sk_msg_t sk_msg,
    CA_IN ca_uint8_t  sk_size,
    CA_OUT ca_uint8_t *sk);

ca_status_t ca_generate_omci_ik(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  omci_ik_size);

ca_status_t ca_generate_ploam_ik(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  ploam_ik_size);

ca_status_t ca_generate_kek(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  kek_size);

ca_status_t ca_generate_kek_name(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *kek,
    CA_IN ca_uint8_t *data_encryption_key,
    CA_IN ca_uint8_t data_encrp_key_size,
    CA_IN ca_uint8_t  kek_name_size,
    CA_OUT ca_uint8_t *kek_name);

ca_status_t ca_generate_omci_mic(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
    CA_IN ca_uint8_t  *omci_content,/*[content_len:1980]*/
    CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic/*[mic_size:4]*/);

ca_status_t ca_generate_ploam_mic(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
    CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  *ploam_content,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic );

ca_status_t ca_generate_default_ploam_mic(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
	CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  *ploam_content,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic);

ca_status_t ca_generate_pon_tag_digest(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_digest_config_t digest_conf,
    CA_IN ca_uint8_t mic_size,
    CA_OUT ca_uint8_t *mic);

ca_status_t ca_generate_onu_auth_result(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  *psk,
    CA_IN ca_uint8_t  onu_selected_crypto_cap,
    CA_IN ca_uint8_t  olt_challenge_len,
    CA_IN ca_uint8_t  *olt_challenge,
    CA_IN ca_uint8_t  onu_challenge_len,
    CA_IN ca_uint8_t  *onu_challenge,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic );
ca_status_t ca_xgpon_aes_sm_state_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *pre_state,
	CA_OUT ca_uint32_t *cur_state);

ca_status_t ca_xgpon_aes_sm_state_set(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t pre_state,
	CA_OUT ca_uint32_t cur_state);

ca_status_t ca_generate_def_kek_and_kek_name(/*CODEGEN_IGNORE_TAG*/
    void);
ca_status_t ca_generate_new_kek_and_kek_name(/*CODEGEN_IGNORE_TAG*/
    ca_uint8_t key_idx);
ca_status_t xgpn_aes_key_generate(
	CA_IN ca_device_id_t device_id);

void xgpon_aes_data_key_state_init(/*CODEGEN_IGNORE_TAG*/void);

ca_status_t xgpon_ploam_key_exchange_fsm_init(
	/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id);

ca_status_t xgpon_ploam_key_exchange_handler(
    /*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id,ca_uint8_t *p_key_exchange_ploam);

ca_status_t xgpon_key_exchange_tk4_timeout_handler (
    /*CODEGEN_IGNORE_TAG*/void);
ca_status_t xgpon_key_exchange_tk5_timeout_handler (
    /*CODEGEN_IGNORE_TAG*/void);

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t xgpn_aes_key_generate_by_msk(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *new_msk);
#endif

#endif
