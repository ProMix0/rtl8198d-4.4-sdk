/*

This is an implementation of the AES128 algorithm, specifically ECB and CBC mode.

The implementation is verified against the test vectors in:
  National Institute of Standards and Technology Special Publication 800-38A 2001 ED

ECB-AES128
----------

  plain-text:
    6bc1bee22e409f96e93d7e117393172a
    ae2d8a571e03ac9c9eb76fac45af8e51
    30c81c46a35ce411e5fbc1191a0a52ef
    f69f2445df4f9b17ad2b417be66c3710

  key:
    2b7e151628aed2a6abf7158809cf4f3c

  resulting cipher
    3ad77bb40d7a3660a89ecaf32466ef97
    f5d3d58503b9699de785895a96fdbaaf
    43b1cd7f598ece23881b00e3ed030688
    7b0c785e27e8ad3f8223207104725dd4


NOTE:   String length must be evenly divisible by 16byte (str_len % 16 == 0)
        You should pad the end of the string with zeros if this is not the case.

*/
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "os_core.h"
#include "util_aes.h"
#include "util_misc.h"
#include "ca-scfg/scfg.h"
#include "aal_xgpon.h"
#include "aal_xgpon_actp.h"
#include "aal_ploam.h"
#include "xgpon_aes.h"
#include "xgpon.h"
#include "gpon.h"
#include "ploam_ngpon2.h"
#include "ploam_pkt.h"

//#include "onu.h"

extern xgpon_basic_info_t g_xgpn_basic_info;
extern ca_int32_t  aal_xgpn_aes_rpt_hw_accelerate;

extern ca_uint32_t aal_xgpn_aes_ukey_chg_flag ;

#define XGPN_AES_DEBUG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_xgpon_aes_debug) ca_printf("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)
#define XGPN_AES_SM_DEBUG(arg/*CODEGEN_IGNORE_TAG*/ ,...) if (aal_xgpon_aes_sm_debug) ca_printf("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)

#define __XGPN_AES_LOCK(dev)           ca_spin_lock(__xgpon_aes_lock)
#define __XGPN_AES_UNLOCK(dev)         ca_spin_unlock(__xgpon_aes_lock)

#define CA_XGPON_AES_RET_VALUE_CHECK(ret)         do{ \
                                            if(CA_E_OK != ret) { \
                                                ca_printf("%s:[%d] return %d\n", __FUNCTION__, __LINE__, ret); \
                                                __XGPN_AES_UNLOCK(0);\
                                                return ret; \
                                            } \
                                        } while(0)
ca_uint __xgpon_aes_lock = 0;
ca_uint8_t olt_type[CFG_ID_PON_OLT_TYPE_LEN] = {0};
ca_xgpon_key_report_ploam_msg_t g_key_rpt_new_msg;
ca_xgpon_key_report_ploam_msg_t g_key_rpt_confirm_msg;

ca_uint8_t g_pon_tag[PON_TAG_SIZE] = {0x00, 0x13, 0x25, 0xFF, 0xEE, 0xBD, 0xAA, 0x55} ; /* Init as invalid PONTAG */
ca_uint8_t g_pon_tag_tmp[PON_TAG_SIZE] = {0x00, 0x13, 0x25, 0xFF, 0xEE, 0xBD, 0xAA, 0x55} ; /* Init as invalid PONTAG */

ca_uint8_t msk[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t sk[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t kek[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t omci_ik[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t ploam_ik[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_ploam_msg[41] = {0};
ca_uint8_t g_ploam_mic[8] = {0};

ca_uint8_t rand_key[XGPN_AES_KEY_LEN] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                                 0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
ca_uint8_t g_uc_key[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_encryped_key[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_kek_name[XGPN_AES_KEY_LEN] = {0};

ca_uint8_t g_uc_key0[XGPN_AES_KEY_LEN] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
										  0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
ca_uint8_t g_uc_key1[XGPN_AES_KEY_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
										  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
ca_uint8_t g_encryped_key0[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_encryped_key1[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_kek_name0[XGPN_AES_KEY_LEN] = {0};
ca_uint8_t g_kek_name1[XGPN_AES_KEY_LEN] = {0};
ca_uint32_t g_state_chg_cnt = 0 ;
//ca_xgpon_key_report_ploam_msg_t  g_key_rpt_msg;
ca_int8_t* xgpn_key_exchg_state[CA_XGPON_ONU_KEY_EX_MAX + 1] = {"KN0", "KN1", "KN2", "KN3", "KN4"};
char g_key_exchange_trace_mem[512] = {0};

ca_xgpon_onu_key_exchange_state_t current_key_exchange_state = 0;
ca_xgpon_onu_key_exchange_state_t previous_key_exchange_state = 0;

ca_boolean_t timeout4_flag = FALSE;
ca_boolean_t timeout5_flag = FALSE;
ca_uint32_t g_rpt_new_key_cnt = 0;
ca_uint32_t g_rpt_ext_key_cnt = 0;
ca_uint32_t g_key_ctl_new_cnt = 0;
ca_uint32_t g_key_ctl_cfm_cnt = 0;

ca_uint16_t g_onu_id = 0;
ca_uint16_t g_rpt_type = 0;

ca_uint8_t g_key_idx = 255; // invalid by default
ca_uint8_t g_key_len = 0;
ca_uint8_t g_key_ctrl_seqNo = 0;
ca_uint8_t g_key_ctrl_flag = 0;
ca_uint8_t g_key_prt_fragment_num = 0;
static key_flag_t key_flag_array[2];
ca_boolean_t key_change_flag = FALSE; //new key or old key
ca_boolean_t old_key_flag = FALSE;//has old key or not

static ca_uint8_t old_key_index = 0;
ca_uint16_t g_wrong_key_index = 255;


//static data_encrpt_key_t uc_data_encrpt_key_array[2];
//static data_encrpt_key_t bc_data_encrpt_key_array[2];
//static ca_uint8_t aes_key_rpt_msg[XGPN_PLMP_TXBUF_SIZE] = {0};
extern ca_xgpon_key_report_ploam_msg_t g_key_rpt_new_msg;
extern ca_xgpon_key_report_ploam_msg_t g_key_rpt_confirm_msg;
ca_uint8_t ack_content[40] = {0};

static ca_uint32_t  g_xgpon_ploam_reset_timer = 0;

void xgpon_aes_dbg_show(void)
{
#if 0
    int i = 0;
    char *pstr = NULL;
    char tmp[514] = {0};
#endif
	ca_boolean_t active_state = 0;
	ca_uint8_t key_len = 0;
	ca_uint8_t key0[16] = {0};
	ca_uint8_t key1[16] = {0};
    ca_printf("\r\nPrint aes key exchange state!\r\n");
    ca_printf("======================================\r\n");
    ca_printf("Key FSM Previous state: %d.\r\n",previous_key_exchange_state);
    ca_printf("Key FSM Current state: %d.\r\n",current_key_exchange_state);
    ca_printf("old key flag: %d.\r\n",old_key_flag);
    ca_printf("key_change_flag: %d.\r\n",key_change_flag);
    ca_printf("timeout4_flag: %d.\r\n",timeout4_flag);
    ca_printf("timeout5_flag: %d.\r\n",timeout4_flag);
    ca_printf("======================================\r\n");
    ca_printf("Print aes key PLOAM counter!\r\n");
    ca_printf("======================================\r\n");
	ca_printf("HW rcv key_ctl_mib		 : %u\r\n",AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_MIB));
	ca_printf("HW send key_rpt_mib		 : %u\r\n",AAL_PON_REG_READ(XGPN_PLMP_KEY_REPORT_MIB));
    ca_printf("SW Rec key_ctl_new_cnt: %d.\r\n",g_key_ctl_new_cnt);
    ca_printf("SW Send key_rpt_new_cnt: %d.\r\n",g_rpt_new_key_cnt);
    ca_printf("SW Rec key_ctl_cfm_cnt: %d.\r\n",g_key_ctl_cfm_cnt);
    ca_printf("SW Send key_rpt_ext_cnt: %d.\r\n",g_rpt_ext_key_cnt);
    ca_printf("======================================\r\n");
    ca_printf("Print key control PLOAM info!\r\n");
    ca_printf("======================================\r\n");
    ca_printf("g_onu_id =  %u \r\n", ntohs(g_onu_id));
    ca_printf("g_key_idx =  %u \r\n", g_key_idx);
    ca_printf("g_key_len =  %u \r\n", g_key_len);
    ca_printf("g_key_ctrl_seqNo =  %u \r\n", g_key_ctrl_seqNo);
    ca_printf("g_key_ctrl_flag =  %u \r\n", g_key_ctrl_flag);
    ca_printf("g_key_prt_fragment_num =  %u \r\n", g_key_prt_fragment_num);
    ca_printf("======================================\r\n");
    ca_printf("Print AES state change counter!\r\n");
    ca_printf("======================================\r\n");
    ca_printf("g_state_chg_cnt =  %u \r\n", g_state_chg_cnt);
    ca_printf("======================================\r\n");
    ca_printf("Print aes key!\r\n");
    ca_printf("======================================\r\n");
    ca_buffer_dump("AES_kek", kek, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_uc_key0", g_uc_key0, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_encryped_key0", g_encryped_key0, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_kek_name0", g_kek_name0, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_uc_key1", g_uc_key1, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_encryped_key1", g_encryped_key1, XGPN_AES_KEY_LEN);
    ca_buffer_dump("AES_kek_name1", g_kek_name1, XGPN_AES_KEY_LEN);
    ca_printf("======================================\r\n");
	ca_buffer_dump("xgpn_aes_key_generate-msk", msk, 16);
    ca_buffer_dump("xgpn_aes_key_generate-sk", sk, 16);
    ca_buffer_dump("xgpn_aes_key_generate-omci_ik", omci_ik, 16);
    ca_buffer_dump("xgpn_aes_key_generate-ploam_ik", ploam_ik, 16);
	ca_buffer_dump("xgpn_aes_key_generate-ploam_msg", g_ploam_msg, 41);
	ca_buffer_dump("xgpn_aes_key_generate-ploam_mic", g_ploam_mic, 8);
	ca_buffer_dump("g_key_rpt_new_msg",(ca_uint8_t *)&g_key_rpt_new_msg, 40);
	ca_buffer_dump("g_key_rpt_confirm_msg", (ca_uint8_t *)&g_key_rpt_confirm_msg, 40);
	ca_buffer_dump("ack_content", (ca_uint8_t *)&ack_content, 40);
	ca_printf("======================================\r\n");

	ca_gpon_port_aes_key_get(0, 0, 0, &active_state,&key_len, key0);
	ca_printf("key0 is %s.\r\n", (active_state == 0)? "inactive" : "active");
	ca_buffer_dump("key0", key0, XGPN_AES_KEY_LEN);
	
	ca_gpon_port_aes_key_get(0, 0, 1, &active_state,&key_len, key1);
	ca_printf("key1 is %s.\r\n", (active_state == 0)? "inactive" : "active");
	ca_buffer_dump("key1", key1, XGPN_AES_KEY_LEN);
#if 0
    memcpy(tmp, g_key_exchange_trace_mem, 512);
    ca_printf("trace mem len = %d\r\n", strlen(tmp));
    pstr = tmp;
    for (i = 0; i < strlen(tmp); i++) {
        if (tmp[i] == '\n') {
            tmp[i] = 0;
            ca_printf("%s \n", pstr);
            pstr = &tmp[i+2];
        }
    }
#endif

}

EXPORT_SYMBOL(xgpon_aes_dbg_show);

ca_status_t ca_generate_msk(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t reg_id_size,
    CA_IN ca_uint8_t *registration_id,
    CA_IN ca_uint8_t  msk_size,
    CA_OUT ca_uint8_t *msk)
{
    ca_status_t ret = CA_E_OK;

    ca_uint8_t def_key[16] = {
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
    };

    CA_AES_PTR_CHECK(registration_id);
    if (reg_id_size != 36)
        return CA_E_PARAM;
    if (msk_size != 16)
        return CA_E_PARAM;
    if(aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_msk-rgst_id", registration_id, 36);
    }
    ret = aes128_cmac_compute(def_key, registration_id, reg_id_size, msk_size, msk);
    CA_RET_VALUE_CHECK(ret);
    
    /*program hw*/
    if (aal_xgpon_aes_key_hw_mode == CA_AAL_XGPON_KEY_SW_MODE)
    {
        ret = aal_xgpon_aes_plmp_msk_set(0,msk);
        CA_RET_VALUE_CHECK(ret);
    }
    return ret;
}

ca_status_t ca_generate_sk(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *msk,
    CA_IN ca_sk_msg_t sk_msg,
    CA_IN ca_uint8_t  sk_size,
    CA_OUT ca_uint8_t *sk)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t sk_msg_tmp[24];

    ca_uint8_t sk_string[8] = {0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x4b};/*"SessionK"*/

    CA_AES_PTR_CHECK(msk);

    if (sk_size != 16)
        return CA_E_PARAM;
    memset(sk_msg_tmp, 0, 24);
    memcpy(sk_msg_tmp, sk_msg.sn, 8);
    memcpy((sk_msg_tmp + 8), sk_msg.pon_tag, 8);
    memcpy((sk_msg_tmp + 8 + 8), sk_string, 8);
    if(aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_sk-sk_msg_tmp", sk_msg_tmp, 24);
    }
    ret = aes128_cmac_compute(msk, sk_msg_tmp, 24, sk_size, sk);
    CA_RET_VALUE_CHECK(ret);

    if (aal_xgpon_aes_key_hw_mode == CA_AAL_XGPON_KEY_SW_MODE)
    {
        ret = aal_xgpon_aes_plmp_sk_set(0,sk);
        CA_RET_VALUE_CHECK(ret);
    }  
    return ret;
}

ca_status_t ca_generate_omci_ik(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  omci_ik_size)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t omci_ik_msg[16] = {
        0x4f, 0x4d, 0x43, 0x49, 0x49, 0x6e, 0x74, 0x65,
        0x67, 0x72, 0x69, 0x74, 0x79, 0x4b, 0x65, 0x79
    };/*"OMCIIntegrityKey"*/
    CA_AES_PTR_CHECK(sk);
    if (omci_ik_size != 16)
        return CA_E_PARAM;
    ret = aes128_cmac_compute(sk, omci_ik_msg, 16, omci_ik_size, omci_ik);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_generate_ploam_ik(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  ploam_ik_size)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t ploam_ik_msg[16] = {
        0x50, 0x4c, 0x4f, 0x41, 0x4d, 0x49, 0x6e, 0x74,
        0x65, 0x67, 0x72, 0x74, 0x79, 0x4b, 0x65, 0x79
    };/*"PLOAMIntegrityKey"*/
    CA_AES_PTR_CHECK(sk);
    if (ploam_ik_size != 16)
        return CA_E_PARAM;
    ret = aes128_cmac_compute(sk, ploam_ik_msg, 16, ploam_ik_size, ploam_ik);
    CA_RET_VALUE_CHECK(ret);

    /*program hw*/
    if (aal_xgpon_aes_key_hw_mode == CA_AAL_XGPON_KEY_SW_MODE)
    {
        ret = aal_xgpon_aes_plmp_ik_set(0,ploam_ik);
        CA_RET_VALUE_CHECK(ret);
    }
    return ret;
}

ca_status_t ca_generate_kek(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *sk,
    CA_IN ca_uint8_t  kek_size)
{
    ca_status_t ret = CA_E_OK;

    ca_uint8_t kek_msg[16] = {
        0x4b, 0x65, 0x79, 0x45, 0x6e, 0x63, 0x72, 0x79,
        0x70, 0x74, 0x69, 0x6f, 0x6e, 0x4b, 0x65, 0x79
    };/*"KeyEncryptionKey"*/
    CA_AES_PTR_CHECK(sk);
    if (kek_size != 16)
        return CA_E_PARAM;
    ret = aes128_cmac_compute(sk, kek_msg, 16, kek_size, kek);
    CA_RET_VALUE_CHECK(ret);
    return ret;
}
/*data_encryption_key is generated by calling random_key_generate()*/
ca_status_t ca_generate_kek_name(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *kek,
    CA_IN ca_uint8_t *data_encryption_key,
    CA_IN ca_uint8_t data_encrp_key_size,
    CA_IN ca_uint8_t  kek_name_size,
    CA_OUT ca_uint8_t *kek_name)
{
    ca_status_t ret = CA_E_OK;

    ca_uint8_t kek_name_str [16] = {
        0x33, 0x31, 0x34, 0x31, 0x35, 0x39, 0x32, 0x36,
        0x35, 0x33, 0x35, 0x38, 0x39, 0x37, 0x39, 0x33
    };
    ca_uint8_t kek_name_msg[32];
    CA_AES_PTR_CHECK(kek);
    CA_AES_PTR_CHECK(data_encryption_key);
    if (data_encrp_key_size != 16)
        return CA_E_PARAM;
    if (kek_name_size != 16)
        return CA_E_PARAM;

    memset(kek_name_msg, 0, 32);
    memcpy(kek_name_msg, data_encryption_key, 16);
    memcpy((kek_name_msg + 16), kek_name_str, 16);
    if(aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_kek_name-kek_name_msg", kek_name_msg, 32);
    }
    ret = aes128_cmac_compute(kek, kek_name_msg, 32, kek_name_size, kek_name);
    CA_RET_VALUE_CHECK(ret);
    return ret;
}

ca_status_t ca_generate_omci_mic(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
    CA_IN ca_uint8_t  *omci_content,
    CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t *omci_msg = NULL;

    CA_AES_CHECK_DIRECTION(dir);
    CA_AES_PTR_CHECK(omci_content);
    if (mic_size != 4)
        return CA_E_PARAM;

    omci_msg = (ca_uint8_t *)OS_MALLOC_API(content_len + 1);
    if (omci_msg == NULL) {
        OS_PRINTF("omci_msg OS_MALLOC_API fail!\n");
        return CA_E_ERROR;
    }

    memset(omci_msg, 0, (content_len + 1));
    omci_msg[0] = dir;
    memcpy((omci_msg + 1), omci_content, content_len);
    if (0){
        ca_buffer_dump("ca_generate_omci_mic-omci_msg", omci_msg, (content_len+1));
    }
    ret = aes128_cmac_compute(omci_ik, omci_msg, (content_len + 1), mic_size, mic);

    if (ret != CA_E_OK) {
        goto end;
    }
    if (0){
        ca_printf("content_len:%d\r\nca_generate_omci_mic: mic[0] is:0x%x\n", content_len, mic[0]);
        ca_printf("ca_generate_omci_mic: mic[1] is:0x%x\n",mic[1]);
        ca_printf("ca_generate_omci_mic: mic[2] is:0x%x\n",mic[2]);
        ca_printf("ca_generate_omci_mic: mic[3] is:0x%x\n",mic[3]);
    }
end:
    if (omci_msg != NULL) {
        OS_FREE_API(omci_msg);
        omci_msg = NULL;
    }
    return ret;

}

ca_status_t ca_generate_ploam_mic(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
    CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  *ploam_content,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t ploam_msg[41];
    ca_uint16_t onu_id_tmp;
    ca_uint8_t def_key[16] = {
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
    };

    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)ploam_content;
    onu_id_tmp = htons(head_p->onu_id);
    CA_AES_CHECK_DIRECTION(dir);
    CA_AES_PTR_CHECK(ploam_content);
    if (content_len != 40)
    {
        OS_PRINTF("ca_generate_ploam_mic-content_len is not 40 bytes\n");
        return CA_E_PARAM;
    }
    /*for program hw, mac size should be 16 byte,so need truncate 8 byte from mac*/
    if (mic_size != 8)
    {
        OS_PRINTF("ca_generate_ploam_mic-mic_size is not 8 bytes\n");
        return CA_E_PARAM;
    }

    if(onu_id_tmp == 0x3FF)
    {
        memcpy(ploam_ik, def_key, 16);
    }

    memset(ploam_msg, 0, 41);
    memcpy(ploam_msg, &dir, 1);
    memcpy((ploam_msg + 1), ploam_content, content_len);
    if (aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_ploam_mic-ploam_msg", ploam_msg, (content_len+1));
    }
	memcpy(g_ploam_msg, ploam_msg, 41);
    ret = aes128_cmac_compute(ploam_ik, ploam_msg, 41, mic_size, mic);
    CA_RET_VALUE_CHECK(ret);
	memcpy(g_ploam_mic, mic, 8);
    /*program hw*/
    if (aal_xgpon_aes_key_hw_mode == CA_AAL_XGPON_KEY_SW_MODE)
    {
        //ret = aal_xgpon_aes_plmp_mic_key_set(0,mic);
        //CA_RET_VALUE_CHECK(ret);
    }
    
    return ret;

}

ca_status_t ca_generate_default_ploam_mic(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  dir,
    CA_IN ca_uint8_t  content_len,
    CA_IN ca_uint8_t  *ploam_content,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t def_key[16] = {
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
    };
    ca_uint8_t ploam_msg[41];

    CA_AES_CHECK_DIRECTION(dir);
    CA_AES_PTR_CHECK(ploam_content);
    if (content_len != 40)
        return CA_E_PARAM;
    if (mic_size != 8)
        return CA_E_PARAM;

    memset(ploam_msg, 0, 41);
    memcpy(ploam_msg, &dir, 1);
    memcpy((ploam_msg + 1), ploam_content, content_len);
    if (aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_default_ploam_mic-ploam_msg", ploam_msg, (content_len+1));
    }
    ret = aes128_cmac_compute(def_key, ploam_msg, 41, mic_size, mic);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}
ca_status_t ca_generate_pon_tag_digest(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_digest_config_t digest_conf,
    CA_IN ca_uint8_t mic_size,
    CA_OUT ca_uint8_t *mic)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t *digest_str = "PtoPisSimple";
    ca_uint8_t digest_key[16];
    ca_uint8_t digest_msg[48];
    ca_uint32_t rgs_id_tmp[XGPN_REG_ID_SIZE/4] = {0};
    int i =0;
    if (mic_size != 8)
        return CA_E_PARAM;

    memset(digest_key, 0, 16);
    memset(digest_msg, 0, 48);

    memcpy(digest_key, digest_conf.pon_tag, 8);
    memcpy((digest_key + 8), digest_conf.pon_tag, 8);

    memcpy(rgs_id_tmp,digest_conf.registration_id,36);

    for (i= 0 ;i<9; i++){
        rgs_id_tmp [i] = htonl(rgs_id_tmp [i]);
    }

    memcpy(digest_msg, rgs_id_tmp, 36);
    memcpy((digest_msg + 36), digest_str, 12);
    if (aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_pon_tag_digest-digest_key", digest_key, 16);
        ca_buffer_dump("ca_generate_pon_tag_digest-digest_msg", digest_msg, 48);
    }
    ret = aes128_cmac_compute(digest_key, digest_msg, 48, mic_size, mic);
    CA_RET_VALUE_CHECK(ret);
    return ret;

}

ca_status_t ca_generate_onu_auth_result(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  *psk,
    CA_IN ca_uint8_t  onu_selected_crypto_cap,
    CA_IN ca_uint8_t  olt_challenge_len,
    CA_IN ca_uint8_t  *olt_challenge,
    CA_IN ca_uint8_t  onu_challenge_len,
    CA_IN ca_uint8_t  *onu_challenge,
    CA_IN ca_uint8_t  mic_size,
    CA_OUT ca_uint8_t *mic)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t *onu_auth_msg = NULL;
    ca_uint8_t str[8] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    CA_AES_PTR_CHECK(psk);
    CA_AES_PTR_CHECK(olt_challenge);
    CA_AES_PTR_CHECK(onu_challenge);

    onu_auth_msg = (ca_uint8_t *)OS_MALLOC_API(1 + olt_challenge_len + onu_challenge_len + 8);
    if (onu_auth_msg == NULL) {
        OS_PRINTF("onu_auth_msg OS_MALLOC_API fail!\n");
        return CA_E_ERROR;
    }

    memset(onu_auth_msg, 0, (1 + olt_challenge_len + onu_challenge_len + 8));
    memcpy(onu_auth_msg, &onu_selected_crypto_cap, 1);
    memcpy((onu_auth_msg + 1), olt_challenge, olt_challenge_len);
    memcpy((onu_auth_msg + 1 + olt_challenge_len), onu_challenge, onu_challenge_len);
    memcpy((onu_auth_msg + 1 + olt_challenge_len + onu_challenge_len), str, 8);
    if (aal_xgpon_aes_debug){
        ca_buffer_dump("ca_generate_onu_auth_result-psk", psk, 16);
        ca_buffer_dump("ca_generate_onu_auth_result-onu_auth_msg", onu_auth_msg, (1 + olt_challenge_len + onu_challenge_len + 8));
    }
    ret = aes128_cmac_compute(psk, onu_auth_msg, (1 + olt_challenge_len + onu_challenge_len + 8), mic_size, mic);

    if (ret != CA_E_OK) {
        goto end;
    }

end:
    if (onu_auth_msg != NULL) {
        OS_FREE_API(onu_auth_msg);
        onu_auth_msg = NULL;
    }
    return ret;

}

ca_status_t ca_xgpon_aes_sm_state_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t pre_state,
    CA_OUT ca_uint32_t cur_state)
{
    aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t key_ctrl_mask;
    aal_xgpon_dsfp_xgem_key_ctrl_config_t key_ctrl_cfg;
	g_onu_id = 0xFFFF;
    g_key_idx = 255;
    g_key_len = 0;
    g_key_ctrl_seqNo = 0;
    g_key_ctrl_flag = 0;
    g_key_prt_fragment_num = 0;
	g_state_chg_cnt = 0;
    key_change_flag = FALSE;
    old_key_flag = FALSE;
    old_key_index = 0;
    g_wrong_key_index = 255;
    previous_key_exchange_state = pre_state;
    current_key_exchange_state = cur_state;
	memcpy(g_pon_tag, g_pon_tag_tmp, PON_TAG_SIZE);
	
    key_ctrl_mask.u32 = 0xffffffff;
    memset(&key_ctrl_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_key_ctrl_config_t));

    //key_ctrl_cfg.key0_idx_vld = 0;
    //key_ctrl_cfg.key1_idx_vld = 0;
    return  aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);

}


ca_status_t ca_xgpon_aes_sm_state_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *pre_state,
    CA_OUT ca_uint32_t *cur_state)
{
    ca_status_t ret = CA_E_OK;
    *pre_state = previous_key_exchange_state;
    *cur_state = current_key_exchange_state;
    return ret;
}
extern ca_status_t __generate_key_report_msg(ca_uint8_t rpt_type);

ca_status_t ca_generate_def_kek_and_kek_name(void)
{
    ca_status_t ret = CA_E_OK;
    aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;

    //ca_uint8_t encryped_key[XGPN_AES_KEY_LEN] = {0};
    //ca_uint8_t kek_name[XGPN_AES_KEY_LEN] = {0};

    memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));

    /*generate new key*/
	#ifdef RAND_KEY
    random_key_generate(g_uc_key0);
    random_key_generate(g_uc_key1);
	#endif
	
    uc_enc_key_msk.s.p_key_active = 1;
    uc_enc_key_msk.s.p_effective_key_length = 1;
    uc_enc_key_msk.s.uc_key16 = 1;

    uc_enc_key_cfg.p_key_active = 0;
    uc_enc_key_cfg.p_effective_key_length = XGPN_AES_KEY_LEN;

    /*program hw*/
    memcpy(uc_enc_key_cfg.uc_key,g_uc_key0,XGPN_AES_KEY_LEN);
    ret = aal_xgpon_aes_uc_enc_key_set(0,0,uc_enc_key_msk,&uc_enc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    memcpy(uc_enc_key_cfg.uc_key,g_uc_key1,XGPN_AES_KEY_LEN);
    ret = aal_xgpon_aes_uc_enc_key_set(0,1,uc_enc_key_msk,&uc_enc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    /*generate enrypted key*/
    ret = aes128_ecb_encrypt(kek,g_uc_key0, g_encryped_key0);
    CA_RET_VALUE_CHECK(ret);

    ret = aes128_ecb_encrypt(kek,g_uc_key1, g_encryped_key1);
    CA_RET_VALUE_CHECK(ret);

    /*generate kek name*/
    ret = ca_generate_kek_name(0, kek,g_uc_key0,XGPN_AES_KEY_LEN,XGPN_AES_KEY_LEN,g_kek_name0);
    CA_RET_VALUE_CHECK(ret);

    ret = ca_generate_kek_name(0, kek,g_uc_key1,XGPN_AES_KEY_LEN,XGPN_AES_KEY_LEN,g_kek_name1);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t ca_generate_new_kek_and_kek_name(ca_uint8_t key_idx)
{
    ca_status_t ret = CA_E_OK;
    //ca_uint8_t rand_key[XGPN_AES_KEY_LEN] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    //                                             0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

   	aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;

    //ca_uint8_t encryped_key[XGPN_AES_KEY_LEN] = {0};
    //ca_uint8_t kek_name[XGPN_AES_KEY_LEN] = {0};
    CA_XGPON_KEY_IDX_CHECK(key_idx);

    //memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));

    /*generate new key*/
	#ifdef RAND_KEY
    random_key_generate(rand_key);
	#endif
	
	#if 1
    uc_enc_key_msk.s.p_key_active = 1;
    uc_enc_key_msk.s.p_effective_key_length = 1;
    uc_enc_key_msk.s.uc_key16 = 1;

    uc_enc_key_cfg.p_key_active = 0;
    uc_enc_key_cfg.p_effective_key_length = XGPN_AES_KEY_LEN;

    /*program hw*/
    memcpy(uc_enc_key_cfg.uc_key,rand_key,XGPN_AES_KEY_LEN);
    ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg);
    CA_XGPON_AES_RET_VALUE_CHECK(ret);
	#endif
	if(key_idx == 0){
		#ifdef RAND_KEY
		memcpy(g_uc_key0,rand_key,XGPN_AES_KEY_LEN);//for debug
		#endif
		/*generate enrypted key*/
		ret = aes128_ecb_encrypt(kek,g_uc_key0,g_encryped_key0);
		CA_RET_VALUE_CHECK(ret);
		/*generate kek name*/
		ret = ca_generate_kek_name(0, kek,g_uc_key0,XGPN_AES_KEY_LEN,XGPN_AES_KEY_LEN,g_kek_name0);
		CA_RET_VALUE_CHECK(ret);
	}
	else{
		#ifdef RAND_KEY
		memcpy(g_uc_key1,rand_key,XGPN_AES_KEY_LEN);//for debug
		#endif
		/*generate enrypted key*/
		ret = aes128_ecb_encrypt(kek,g_uc_key1,g_encryped_key1);
		CA_RET_VALUE_CHECK(ret);
		/*generate kek name*/
		ret = ca_generate_kek_name(0, kek,g_uc_key1,XGPN_AES_KEY_LEN,XGPN_AES_KEY_LEN,g_kek_name1);
		CA_RET_VALUE_CHECK(ret);
	}
    return ret;
}

ca_status_t xgpn_aes_key_generate(
    CA_IN ca_device_id_t device_id)
{

    ca_status_t ret = CA_E_OK;
    ca_uint32_t vendor_id =0;
    ca_uint32_t vender_id_tmp =0;
    ca_uint32_t vssn = 0;
    ca_uint8_t vssn_reverse[4] = {0};
    ca_uint32_t vssn_tmp =0;
    ca_sk_msg_t sk_msg;
   
    int j = 0 ;

    aal_xgpn_plmp_rgst_id_cfg_t rgst_id = {{0x12 ,0x34 ,0x56 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78,
                                            0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78,
                                            0x78 ,0x78 ,0x78 ,0x78}};
    aal_uc_enc_key_config_t uc_enc_key_cfg;

    memset(&sk_msg,0,sizeof(ca_sk_msg_t));
    memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));

    ret = scfg_read(0, CFG_ID_PON_OLT_TYPE, CFG_ID_PON_OLT_TYPE_LEN, olt_type);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_ploam_rgst_id_cfg_get(CA_XGPN_DEV_ID,&rgst_id);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_ploam_vendor_id_cfg_get(CA_XGPN_DEV_ID,&vendor_id);
    CA_RET_VALUE_CHECK(ret);

    ret =  aal_ploam_vssn_cfg_get(CA_XGPN_DEV_ID,&vssn);
    CA_RET_VALUE_CHECK(ret);
    vender_id_tmp = htonl(vendor_id);

    //vssn_tmp = 0x21436587;
    if(0 == strcmp(olt_type, "CALIX")){
        memcpy(vssn_reverse,&vssn,4);
        for(j = 0;j<4; j++){
            vssn_reverse[j] = ((vssn_reverse[j]&0xF)<<4)|((vssn_reverse[j]&0xF0)>>4);
        }
        memcpy(&vssn_tmp,vssn_reverse,4);
    }else{
        vssn_tmp = htonl(vssn);
    }
    if (aal_xgpon_aes_debug){
		ca_buffer_dump("xgpn_aes_key_generate-olt_type", (ca_uint8_t *)&olt_type, CFG_ID_PON_OLT_TYPE_LEN);
        ca_buffer_dump("xgpn_aes_key_generate-vendor_id", (ca_uint8_t *)&vender_id_tmp, 4);
        ca_buffer_dump("xgpn_aes_key_generate-vssn", (ca_uint8_t *)&vssn_tmp, 4);
    }
    memcpy(sk_msg.sn,&vender_id_tmp,4);
    memcpy((sk_msg.sn+4),&vssn_tmp,4);
    memcpy(sk_msg.pon_tag,g_pon_tag,PON_TAG_SIZE);

    if (aal_xgpon_aes_debug){
        ca_buffer_dump("xgpn_aes_key_generate-g_pon_tag", g_pon_tag, PON_TAG_SIZE);
        ca_buffer_dump("xgpn_aes_key_generate-sk_msg", sk_msg.sn, 8);
    }

    ret = ca_generate_msk(0, XGPN_REG_ID_SIZE,(ca_uint8_t *)rgst_id.data,XGPN_AES_KEY_LEN,msk);
    CA_RET_VALUE_CHECK(ret);

    ret = ca_generate_sk(0, msk,sk_msg,XGPN_AES_KEY_LEN,sk);
    CA_RET_VALUE_CHECK(ret);

    ret = ca_generate_omci_ik(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);
    ret = ca_generate_ploam_ik(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);
    ret = ca_generate_kek(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);

    ca_generate_def_kek_and_kek_name();

    if (aal_xgpon_aes_debug){
        ca_buffer_dump("xgpn_aes_key_generate-msk", msk, 16);
        ca_buffer_dump("xgpn_aes_key_generate-sk", sk, 16);
        ca_buffer_dump("xgpn_aes_key_generate-omci_ik", omci_ik, 16);
        ca_buffer_dump("xgpn_aes_key_generate-ploam_ik", ploam_ik, 16);
        ca_buffer_dump("xgpn_aes_key_generate-kek", kek, 16);
    }
    return ret;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t xgpn_aes_key_generate_by_msk(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t *new_msk)
{

    ca_status_t ret = CA_E_OK;
    ca_sk_msg_t sk_msg;
    int j = 0 ;
    ca_uint32_t vendor_id =0;
    ca_uint32_t vender_id_tmp =0;
    ca_uint32_t vssn = 0;
    ca_uint8_t vssn_reverse[4] = {0};
    ca_uint32_t vssn_tmp =0;
    ca_uint8_t olt_type[CFG_ID_PON_OLT_TYPE_LEN] = {0};
    aal_xgpn_plmp_rgst_id_cfg_t rgst_id = {{0x12 ,0x34 ,0x56 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78,
                                            0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78 ,0x78,
                                            0x78 ,0x78 ,0x78 ,0x78}};

    memset(&sk_msg,0,sizeof(ca_sk_msg_t));
    memcpy(msk, new_msk, XGPN_AES_KEY_LEN);

    ret = scfg_read(0, CFG_ID_PON_OLT_TYPE, CFG_ID_PON_OLT_TYPE_LEN, olt_type);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_ploam_rgst_id_cfg_get(CA_XGPN_DEV_ID,&rgst_id);
    CA_RET_VALUE_CHECK(ret);

    ret = aal_ploam_vendor_id_cfg_get(CA_XGPN_DEV_ID,&vendor_id);
    CA_RET_VALUE_CHECK(ret);

    ret =  aal_ploam_vssn_cfg_get(CA_XGPN_DEV_ID,&vssn);
    CA_RET_VALUE_CHECK(ret);
    vender_id_tmp = htonl(vendor_id);

    //vssn_tmp = 0x21436587;
    if(0 == strcmp(olt_type, "CALIX")){
        memcpy(vssn_reverse,&vssn,4);
        for(j = 0;j<4; j++){
            vssn_reverse[j] = ((vssn_reverse[j]&0xF)<<4)|((vssn_reverse[j]&0xF0)>>4);
        }
        memcpy(&vssn_tmp,vssn_reverse,4);
    }else{
        vssn_tmp = htonl(vssn);
    }
	
    memcpy(sk_msg.sn,&vender_id_tmp,4);
    memcpy((sk_msg.sn+4),&vssn_tmp,4);
    memcpy(sk_msg.pon_tag,g_pon_tag,PON_TAG_SIZE);
	
    ret = ca_generate_sk(0, msk,sk_msg,XGPN_AES_KEY_LEN,sk);
    CA_RET_VALUE_CHECK(ret);

    ret = ca_generate_omci_ik(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);
    ret = ca_generate_ploam_ik(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);
    ret = ca_generate_kek(0, sk,XGPN_AES_KEY_LEN);
    CA_RET_VALUE_CHECK(ret);

    ca_generate_def_kek_and_kek_name();

    if (aal_xgpon_aes_debug){
        ca_buffer_dump("xgpn_aes_key_generate-sk_msg", &sk_msg, 16);
        ca_buffer_dump("xgpn_aes_key_generate-msk", msk, 16);
        ca_buffer_dump("xgpn_aes_key_generate-sk", sk, 16);
        ca_buffer_dump("xgpn_aes_key_generate-omci_ik", omci_ik, 16);
        ca_buffer_dump("xgpn_aes_key_generate-ploam_ik", ploam_ik, 16);
        ca_buffer_dump("xgpn_aes_key_generate-kek", kek, 16);
    }
    return ret;
}
#endif

void xgpon_aes_data_key_state_init(void)
{
    key_flag_array[0].key_idx = 0;
    key_flag_array[0].old_new_flag = INVALID_KEY;
    key_flag_array[0].active_flag= FALSE;
    key_flag_array[1].key_idx = 1;
    key_flag_array[1].old_new_flag = INVALID_KEY;
    key_flag_array[1].active_flag= FALSE;
    return;
}

ca_status_t xgpon_aes_data_key_state_set(
    CA_IN ca_uint8_t key_idx,
    CA_IN ca_uint32_t old_new_flag,
    CA_IN ca_boolean_t active_flag)
{
    ca_status_t ret = CA_E_ERROR;
    int i=0;
    for(i=0;i<2;i++){
        if(i == key_idx){
            key_flag_array[i].key_idx = key_idx;
            key_flag_array[i].old_new_flag = old_new_flag;
            key_flag_array[i].active_flag= active_flag;
            return CA_E_OK;
        }
    }
    return ret;
}

void xgpon_aes_data_key_state_get(
    CA_IN ca_uint8_t key_idx,
    CA_OUT ca_uint32_t *old_new_flag,
    CA_OUT ca_boolean_t *active_flag)
{

    int i=0;
    for(i=0;i<2;i++){
        if(i == key_idx){
            *old_new_flag = key_flag_array[i].old_new_flag;
            *active_flag = key_flag_array[i].active_flag;
            return;
        }
    }
    return;
}

void xgpon_aes_ploam_ack_mic_update(ca_device_id_t device_id)
{
    ca_uint16_t gemportid = 0;
    ploam_ngpon2_header_t ack_header;
    ploam_ngpon2_us_ack_t us_ack;
    ca_uint8_t ack_mic[8] = {0};

    AAL_PON_REG_FIELD_READ(XGPN_DSFP_portid0_ctl_xgem, portid, gemportid);
    memset(&ack_header, 0,  sizeof(ploam_ngpon2_header_t));
    memset(&us_ack, 0,  sizeof(ploam_ngpon2_us_ack_t));
    g_onu_id = ntohs(gemportid);
    ack_header.onu_id = g_onu_id;
    ack_header.msg_id = PLOAM_NGPON2_US_ACK;
    //ack_header.seq_no = 0;
    us_ack.complete_code = AAL_PLOAM_ACK_NO_MSG_TO_SEND;
    //us_ack.attenuation = 0;
    //us_ack.power_level_cap = 0;
    memcpy(ack_content, &ack_header, sizeof(ploam_ngpon2_header_t));
    memcpy(ack_content + sizeof(ploam_ngpon2_header_t), &us_ack, 36);
    
    (void)ca_generate_ploam_mic(device_id,0x02,40,ack_content,8,ack_mic);
    (void) aal_ploam_ack_ploam_no_msg_set(device_id,(aal_xgpn_plmp_ack_ploam_no_msg_t *)ack_mic);

}


ca_status_t __generate_key_report_msg(ca_uint8_t rpt_type)
{
	ca_status_t ret = CA_E_OK;
	aal_key_report_frag_t key_rpt_frag;
	aal_xgpn_plmp_key_report_t	  key_rpt;
	aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
//	ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;
//	ca_uint32_t ploam_retry = 0;
    aal_xgpn_plmp_txbuf_ctrl_t ctrl;
   // aal_xgpn_plmp_txbuf_ctrl_mask_t  ctrl_mask;
    aal_xgpn_plmp_txbuf_t   txbuf;
   ca_xgpon_key_report_ploam_msg_t  key_rpt_msg;
   
	key_rpt_mask.wrd =	~0U;
	memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
	memset(&key_rpt,0,sizeof(key_rpt));
	memset(&key_rpt_msg, 0, sizeof(key_rpt_msg));
	key_rpt_msg.onu_id = g_onu_id;
	key_rpt_msg.msg_type_id = CA_KEY_REPORT;
	key_rpt_msg.seq_no = g_key_ctrl_seqNo;
	key_rpt_msg.report_type = rpt_type;
	if(g_wrong_key_index == 0){
		key_rpt_msg.key_idx = g_key_idx - 1;
	}else{
		key_rpt_msg.key_idx = g_key_idx;
	}
	key_rpt_msg.frament_no = g_key_prt_fragment_num;
	if (rpt_type) {
		if(g_key_idx == 1){
			memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
		}
		if(g_key_idx == 2){
			memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
		}
	}else{
		if(g_key_idx == 1){
			memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
		}
		if(g_key_idx == 2){
			memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
		}
	}
    memset(&ctrl, 0, sizeof(aal_xgpn_plmp_txbuf_ctrl_t));
	#if 0
    do { /* Check H/W ready for Tx PLOAM */
    ret =  aal_ploam_txbuf_ctrl_get(0, &ctrl);
        if (!ctrl.pkt_rdy) { /* H/W is ready for Tx */
            break;
        }
    } while (ploam_retry ++ < PLOAM_TX_RETRY_MAX);
    if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
        PLOAM_DEBUG("PLOAM TX retry timed out\n");
        return CA_E_TIMEOUT;
    }
	#endif
    memset(&txbuf, 0, sizeof(aal_xgpn_plmp_txbuf_t));
    memcpy(txbuf.data, (ca_uint8_t*)(&key_rpt_msg), XGPN_PLMP_TXBUF_SIZE);
    ret = aal_ploam_txbuf_data_set(0, &txbuf);
	if (CA_E_OK != ret) {
		ca_printf("ERROR:Set TX buffer %d fail, return %d.\r\n", rpt_type, ret);
	}
#if 0
	ret = ploam_ngpon2_send_ploam((ca_uint8_t*)(&key_rpt_msg),PLOAM_NGPON2_MSG_SIZE,FALSE);
	if (CA_E_OK != ret) {
		ca_printf("ERROR:Send report %d fail, return %d.\r\n", rpt_type, ret);
	}else{
		XGPN_AES_DEBUG("Send key report msg %d OK.\n", rpt_type);
		if(rpt_type == 0){
			g_rpt_new_key_cnt++ ;
		}
		if(rpt_type == 1){
			g_rpt_ext_key_cnt++ ;
		}
	}
	aes_ploam_dump_pkt("Send Key_rpt PLOAM:", (ca_uint8_t*)(&key_rpt_msg), PLOAM_NGPON2_MSG_SIZE);
#endif
	return CA_E_OK;
}
ca_status_t __send_rpt_ploam(ca_uint8_t rpt_type)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t ploam_retry = 0;
    aal_xgpn_plmp_txbuf_ctrl_t ctrl;
    aal_xgpn_plmp_txbuf_ctrl_mask_t  ctrl_mask;
	ctrl_mask.wrd		 = 0;
	ctrl_mask.bf.pkt_rdy = 1;
	ctrl.pkt_rdy		 = 1;
	#if 0
	if(use_def_key){
		ctrl_mask.bf.defkey = 1;
		ctrl.defkey 		= 1;
	}
	#endif
	ret = aal_ploam_txbuf_ctrl_set(0, ctrl_mask, &ctrl);
	if (CA_E_OK != ret) {
		PLOAM_DEBUG("ret:%d\r\n", ret);
		return ret;
	}
	memset(&ctrl, 0, sizeof(aal_xgpn_plmp_txbuf_ctrl_t));
	do { /* Check H/W Tx done */
		ret =  aal_ploam_txbuf_ctrl_get(0, &ctrl);
		if (!ctrl.pkt_rdy) { /* H/W Tx done */
			break;
		}
	} while (ploam_retry ++ < PLOAM_TX_RETRY_MAX);
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		PLOAM_DEBUG("PLOAM TX timed out\n");
		return CA_E_TIMEOUT;
	}
	if(rpt_type == 0){
		g_rpt_new_key_cnt++ ;
	}
	if(rpt_type == 1){
		g_rpt_ext_key_cnt++ ;
	}
	return CA_E_OK;
}

#if 0
ca_status_t __send_key_report_msg_isr(ca_xgpon_key_report_ploam_msg_t key_rpt_msg)
{
    ca_status_t ret = CA_E_OK;
    aal_key_report_frag_t key_rpt_frag;
    aal_xgpn_plmp_key_report_t    key_rpt;
    aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
    ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
	
	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));

    key_rpt_mask.wrd =  ~0U;

    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
    memset(&key_rpt,0,sizeof(key_rpt));

    if(g_wrong_key_index == 0){
        key_rpt_msg.key_idx = key_rpt_msg.key_idx - 1;
    }
    if (key_rpt_msg.report_type) {
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
        }
    }else{
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
        }
    }
    
    txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;

	//check TX buffer ready
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	//update PLOAM TX buffer (bytes 1-8)
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
                
    reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
	if(key_rpt_msg.report_type == 0){
    	g_rpt_new_key_cnt++ ;
    }
    if(key_rpt_msg.report_type == 1){
        g_rpt_ext_key_cnt++ ;
    }
    aes_ploam_dump_pkt("Send Key_rpt PLOAM:", (ca_uint8_t*)(&key_rpt_msg), PLOAM_NGPON2_MSG_SIZE);
    
    return CA_E_OK;
}
#endif
extern ca_uint32_t ptp_timer1;

ca_status_t __send_key_report_msg(ca_uint8_t rpt_type){

    ca_status_t ret = CA_E_OK;
    aal_key_report_frag_t key_rpt_frag;
    aal_xgpn_plmp_key_report_t    key_rpt;
    aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
    ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;

    key_rpt_mask.wrd =  ~0U;

    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
    memset(&key_rpt,0,sizeof(key_rpt));

    if(aal_xgpn_aes_rpt_hw_accelerate){
        key_rpt.seq = g_key_ctrl_seqNo;
        key_rpt.type = rpt_type;    /*0: New key. 1: Report on existing key.*/
        key_rpt.index = g_key_idx;
        //key_rpt.index = g_key_idx-1;//for ZTE only
        key_rpt.fragment_num= g_key_prt_fragment_num ;/*Here always 0,refer to 989.3 11.3.4.3*/
        key_rpt.rdy=  TRUE;

        XGPN_AES_SM_DEBUG("key_rpt.seq == 0x%x!!\r\n",key_rpt.seq);
        XGPN_AES_SM_DEBUG("key_rpt.type == 0x%x!!\r\n",key_rpt.type);
        XGPN_AES_SM_DEBUG("key_rpt.index == 0x%x!!\r\n",key_rpt.index);
        if (rpt_type) {
            if(g_key_idx == 1){
                memcpy(key_rpt_frag.data, g_kek_name0, XGPN_AES_KEY_LEN);
            }
            if(g_key_idx == 2){
                memcpy(key_rpt_frag.data, g_kek_name1, XGPN_AES_KEY_LEN);
            }
        }else{
            if(g_key_idx == 1){
                memcpy(key_rpt_frag.data, g_encryped_key0, XGPN_AES_KEY_LEN);
            }
            if(g_key_idx == 2){
                memcpy(key_rpt_frag.data, g_encryped_key1, XGPN_AES_KEY_LEN);
            }
        }
        ret = aal_ploam_key_report_frag_set(0,&key_rpt_frag);
        CA_XGPON_AES_RET_VALUE_CHECK(ret);

        ret = aal_ploam_key_report_set(0,key_rpt_mask,&key_rpt);
        CA_XGPON_AES_RET_VALUE_CHECK(ret);

        wait_timeout = __KEY_REPORT_TIMEOUT;
        /* Wait for send finish */
        key_rpt.rdy = 1;
        do {
            aal_ploam_key_report_get(0, &key_rpt);
        }while (key_rpt.rdy && wait_timeout--);

        if (wait_timeout <= 0) {
            XGPN_AES_SM_DEBUG("%s::%d:Send key report msg %d fail, Hardware timeout.\n",
                __FUNCTION__, __LINE__, rpt_type);
            __XGPN_AES_UNLOCK(0);
            return CA_E_TIMEOUT;
        }
    }else {
    	int i = 0;
        ca_xgpon_key_report_ploam_msg_t  key_rpt_msg;
		aal_xgpn_plmp_txbuf_t *txbuf;
		XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
		aal_reg_xgpn_plmp_txbuf_data_t reg_data;
		ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
        
        reg_ctrl.wrd = 0;

        memset(&key_rpt_msg, 0, sizeof(key_rpt_msg));
        key_rpt_msg.onu_id = g_onu_id;
        key_rpt_msg.msg_type_id = CA_KEY_REPORT;
        key_rpt_msg.seq_no = g_key_ctrl_seqNo;
        key_rpt_msg.report_type = rpt_type;
        if(g_wrong_key_index == 0){
            key_rpt_msg.key_idx = g_key_idx - 1;
        }else{
            key_rpt_msg.key_idx = g_key_idx;
        }
        key_rpt_msg.frament_no = g_key_prt_fragment_num;
        if (rpt_type) {
            if(g_key_idx == 1){
                memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
            }
            if(g_key_idx == 2){
                memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
            }
        }else{
            if(g_key_idx == 1){
                memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
            }
            if(g_key_idx == 2){
                memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
            }
        }
		
		//make sure pkt_rdy = 0 for filling tx buffer 
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		while(reg_ctrl.bf.pkt_rdy) {
			reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		}
		
		//fill PLOAM TX buffer 
		reg_addr = XGPN_PLMP_TXBUF_DATA0;
		txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;
		for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
			reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
				| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
				| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
				| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
			AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
			reg_addr += 4;
		}
		
		//trigger PLOAM send
		reg_ctrl.bf.pkt_rdy = 1;
		AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
		
		//make sure key rpt went out
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		while(reg_ctrl.bf.pkt_rdy) {
			reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		}
		
		if(key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
		{
			g_rpt_new_key_cnt++ ;
			//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
			//ca_printf("\r\nfunc %s line %d:Send key report new PLOAM. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,ptp_timer1);
		}else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
		{
			g_rpt_ext_key_cnt++ ;
			//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
			//ca_printf("\r\nfunc %s line %d:Send key report confirm PLOAM. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,ptp_timer1);
		}

		#if 0
        ret = ploam_ngpon2_send_ploam((ca_uint8_t*)(&key_rpt_msg),PLOAM_NGPON2_MSG_SIZE,FALSE);
        if (CA_E_OK != ret) {
            ca_printf("ERROR:Send report %d fail, return %d.\r\n", rpt_type, ret);
        }else{
        	XGPN_AES_DEBUG("Send key report msg %d OK.\n", rpt_type);
	        if(rpt_type == 0){
	        	g_rpt_new_key_cnt++ ;
		    }
		    if(rpt_type == 1){
		        g_rpt_ext_key_cnt++ ;
		    }
		}
		#endif
        aes_ploam_dump_pkt("Send Key_rpt PLOAM:", (ca_uint8_t*)(&key_rpt_msg), PLOAM_NGPON2_MSG_SIZE);
    }
    return CA_E_OK;
}


ca_status_t __xgpon_reset_key_timer(void)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpn_plmp_key_exchg_thrshd_mask_t  thrshd_mask;
    aal_xgpn_plmp_key_exchg_thrshd_t       thrshd;
    thrshd_mask.wrd = 0;
    memset(&thrshd,0,sizeof(thrshd));

    if((timeout4_flag)&&(current_key_exchange_state == CA_XGPON_ONU_KN0)){

        thrshd_mask.bf.tk4_timeout = 1;
        thrshd.tk4_timeout = 100;/*100ms*/
        ret = aal_ploam_key_exchg_thrshd_set(0,thrshd_mask,&thrshd);
        CA_RET_VALUE_CHECK(ret);

        XGPN_AES_DEBUG("Reset tk4 timer::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
    }

    if((timeout5_flag)&&(current_key_exchange_state == CA_XGPON_ONU_KN2)){

        thrshd_mask.bf.tk5_timeout = 1;
        thrshd.tk5_timeout = 20;/*20ms*/
        ret = aal_ploam_key_exchg_thrshd_set(0,thrshd_mask,&thrshd);
        CA_RET_VALUE_CHECK(ret);

        XGPN_AES_DEBUG("Reset tk5 timer::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
    }

    return ret;

}
ca_status_t __xgpon_key_rpt_ploam_build(ca_uint8_t rpt_type)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t encryped_key[16] = {0};
    ca_uint8_t kek_name[16] = {0};
//    ca_uint8_t mic[8]={0};
//    ca_uint8_t orignal_key[16] = {0};

    ca_xgpon_key_report_ploam_msg_t key_rpt_msg;

    aal_uc_enc_key_config_t uc_enc_key_cfg;

    memset(&key_rpt_msg, 0 , sizeof(ca_xgpon_key_report_ploam_msg_t));

    memset(&uc_enc_key_cfg, 0 , sizeof(aal_uc_enc_key_config_t));

    key_rpt_msg.onu_id = g_onu_id;
    key_rpt_msg.msg_type_id = CA_KEY_REPORT;
    key_rpt_msg.seq_no = g_key_ctrl_seqNo;
    key_rpt_msg.frament_no = 0;
    key_rpt_msg.key_idx = g_key_idx;

    if (aal_xgpon_aes_sm_debug){
        ca_printf("%s::%d:onu_id is 0x%x!!\r\n",__FUNCTION__,__LINE__,key_rpt_msg.onu_id);

        ca_printf("%s::%d:msg_type_id is 0x%x!!\r\n",__FUNCTION__,__LINE__,key_rpt_msg.msg_type_id);

        ca_printf("%s::%d:seq_no is 0x%x!!\r\n",__FUNCTION__,__LINE__,key_rpt_msg.seq_no);

        ca_printf("%s::%d:key_idx is 0x%x!!\r\n",__FUNCTION__,__LINE__,key_rpt_msg.key_idx);
    }

    ret = aal_xgpon_aes_uc_enc_key_get(0,(g_key_idx-1),&uc_enc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    key_rpt_msg.report_type = rpt_type;
    if(rpt_type == CA_KEY_CONTROL_NEW){

        ret = aes128_ecb_encrypt(kek,uc_enc_key_cfg.uc_key, encryped_key);
        CA_RET_VALUE_CHECK(ret);
        memcpy(key_rpt_msg.key_fragment,encryped_key,16);
    }
    if(rpt_type == CA_KEY_CONTROL_CONFIRM){
        ret = ca_generate_kek_name(0, kek,uc_enc_key_cfg.uc_key,16,16,kek_name);
        CA_RET_VALUE_CHECK(ret);
        memcpy(key_rpt_msg.key_fragment,kek_name,16);
    }


    //memcpy(aes_key_rpt_msg,(ca_uint8_t*)(&key_rpt_msg),XGPN_PLMP_TXBUF_SIZE);

    ret = ploam_ngpon2_send_ploam((ca_uint8_t*)(&key_rpt_msg),PLOAM_NGPON2_MSG_SIZE,FALSE);
    CA_RET_VALUE_CHECK(ret);

    if (aal_xgpon_aes_sm_debug){
        ca_printf("%s::%d:!!\r\n",__FUNCTION__,__LINE__);
        ca_buffer_dump("aes_key_rpt_msg:", (ca_uint8_t*)(&key_rpt_msg), XGPN_PLMP_TXBUF_SIZE);
        ca_buffer_dump("__send_key_report_msg-kek", kek, 16);
        ca_buffer_dump("__send_key_report_uc_key", uc_enc_key_cfg.uc_key, 16);
        ca_buffer_dump("__send_key_report_encryped_key", encryped_key, 16);
        ca_buffer_dump("__send_key_report_kek_name", kek_name, 16);
    }
    #if 0
    ca_buffer_dump("__xgpon_key_rpt_ploam_build-kek:", kek, 16);
    ca_buffer_dump("__xgpon_key_rpt_ploam_build-uc_key:", uc_enc_key_cfg.uc_key, 16);
    ca_buffer_dump("__xgpon_key_rpt_ploam_build-encryped_key:", encryped_key, 16);

    ret = aes128_ecb_decrypt(kek,encryped_key,orignal_key);
    CA_RET_VALUE_CHECK(ret);
    ca_buffer_dump("__xgpon_key_rpt_ploam_build-orignal_key:", orignal_key, 16);


    ret = ca_generate_ploam_mic(0x02,&key_rpt_msg,40,8,mic);
    CA_RET_VALUE_CHECK(ret);

    ca_buffer_dump("ca_generate_ploam_mic::", mic, 8);
    #endif

    return ret;

}
ca_status_t xgpon_aes_key_flag_set(
    CA_IN ca_uint8_t key_idx,
    CA_IN ca_uint32_t key0_old_new_flag,
    CA_IN ca_uint32_t key1_old_new_flag)
{
    ca_status_t ret = CA_E_OK;
    aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;
    uc_enc_key_msk.u32 = 0;
    memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));
    uc_enc_key_msk.s.p_key_active = 1;


    if((key0_old_new_flag != INVALID_KEY)&&(key1_old_new_flag != INVALID_KEY)){
        xgpon_aes_data_key_state_set(key_idx, NEW_KEY, FALSE);
        uc_enc_key_cfg.p_key_active = 0;
        ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg); //new key invalid

        if(key_idx == 0){
            xgpon_aes_data_key_state_set(1, OLD_KEY, FALSE);
        }else if(key_idx == 1){
            xgpon_aes_data_key_state_set(0, OLD_KEY, FALSE);
        }
        if(current_key_exchange_state == CA_XGPON_ONU_KN1){
            if(key_idx == 0){
                uc_enc_key_cfg.p_key_active = 1;
                ret = aal_xgpon_aes_uc_enc_key_set(0,1,uc_enc_key_msk,&uc_enc_key_cfg); //old key valid
            }
            if(key_idx == 1){
                uc_enc_key_cfg.p_key_active = 1;
                ret = aal_xgpon_aes_uc_enc_key_set(0,0,uc_enc_key_msk,&uc_enc_key_cfg); //old key valid
            }
        }
    }else if(key0_old_new_flag != INVALID_KEY){
        xgpon_aes_data_key_state_set(key_idx, NEW_KEY, FALSE);
        uc_enc_key_cfg.p_key_active = 0;
        ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg); //new key invalid

        if(key_idx == 1){
            xgpon_aes_data_key_state_set(0, OLD_KEY, FALSE);
        }

        if(current_key_exchange_state == CA_XGPON_ONU_KN1){
            if(key_idx == 1){
                uc_enc_key_cfg.p_key_active = 1;
                ret = aal_xgpon_aes_uc_enc_key_set(0,0,uc_enc_key_msk,&uc_enc_key_cfg); //old key valid
            }
        }

    }else if(key1_old_new_flag != INVALID_KEY){

        xgpon_aes_data_key_state_set(key_idx, NEW_KEY, FALSE);
        uc_enc_key_cfg.p_key_active = 0;
        ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg); //new key invalid

        if(key_idx == 0){
            xgpon_aes_data_key_state_set(1, OLD_KEY, FALSE);
        }

        if(current_key_exchange_state == CA_XGPON_ONU_KN1){
            if(key_idx == 0){
                uc_enc_key_cfg.p_key_active = 1;
                ret = aal_xgpon_aes_uc_enc_key_set(0,1,uc_enc_key_msk,&uc_enc_key_cfg); //old key valid
            }
        }
    }else{
        ca_printf("no key valid!!\r\n");
        xgpon_aes_data_key_state_set(key_idx, NEW_KEY, FALSE);
        if(current_key_exchange_state == CA_XGPON_ONU_KN1){
                uc_enc_key_cfg.p_key_active = 0;
                ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg); //new key invalid
        }
    }

    return ret;
}

ca_status_t __xgpon_key_valid_set(ca_uint8_t key_idx)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_dsfp_xgem_key_ctrl_config_t key_ctrl_cfg;
    aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t key_ctrl_mask;
    aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;

	if(0 == strcmp(olt_type, "ZTE")){
#if 0
	    key_ctrl_mask.u32 = 0;
	    memset(&key_ctrl_cfg,0,sizeof(aal_xgpon_dsfp_xgem_key_ctrl_config_t));
		memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));
	    
		uc_enc_key_msk.s.p_key_active = 1;
	    uc_enc_key_msk.s.p_effective_key_length = 1;
	    uc_enc_key_msk.s.uc_key16 = 1;

	    uc_enc_key_cfg.p_key_active = 0;
	    uc_enc_key_cfg.p_effective_key_length = XGPN_AES_KEY_LEN;

#ifdef RAND_KEY
		memcpy(uc_enc_key_cfg.uc_key,rand_key,XGPN_AES_KEY_LEN);
#else
		if(key_idx == 1)
		{//new key index is 1 ,hw new key index is 0,old key index is 1
			if(old_key_flag)
			{
				memcpy(uc_enc_key_cfg.uc_key,g_uc_key0,XGPN_AES_KEY_LEN);
			}
		}
		if(key_idx == 2)
		{//new key index is 2 ,hw new key index is 1,old key index is 0
			if(old_key_flag)
			{
				memcpy(uc_enc_key_cfg.uc_key,g_uc_key1,XGPN_AES_KEY_LEN);
			}
		}
#endif

#endif

		/*KN2:
			US old key valid;
			DS new key valid;
			DS old key valid;
		*/
	    if(current_key_exchange_state == CA_XGPON_ONU_KN2){
	        if(key_idx == 1){//new key index is 1 ,hw new key index is 0,old key index is 1
	            if(old_key_flag){
	                ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,1);//us old valid in KN2
	                CA_XGPON_AES_RET_VALUE_CHECK(ret);

					//program new key to HW
					#if 0
					ca_mdelay(AAL_XGPON_INIT_DELAY);
					ret = aal_xgpon_aes_uc_enc_key_set(0,0,uc_enc_key_msk,&uc_enc_key_cfg);
	    			CA_XGPON_AES_RET_VALUE_CHECK(ret);
					#endif
	            }
				
	        }else if(key_idx == 2){//new key index is 2 ,hw new key index is 1,old key index is 0
	            if(old_key_flag){
	                ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,0);//us old valid in KN2
	                CA_XGPON_AES_RET_VALUE_CHECK(ret);
				
					//program new key to HW
					#if 0
					ca_mdelay(AAL_XGPON_INIT_DELAY);
					ret = aal_xgpon_aes_uc_enc_key_set(0,1,uc_enc_key_msk,&uc_enc_key_cfg);
	    			CA_XGPON_AES_RET_VALUE_CHECK(ret);
					#endif
	            }
	        }
			#if 0
			key_ctrl_mask.s.key0_idx_vld = 1;
			key_ctrl_mask.s.key1_idx_vld = 1;
			key_ctrl_cfg.key1_idx_vld = 1;
            key_ctrl_cfg.key0_idx_vld = 1;
            ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);//ds new key valid in KN2
            CA_XGPON_AES_RET_VALUE_CHECK(ret);
			#endif
	    }
	    /*KN3:
	        US new key valid;
	        send rpt confirm ploam
	        DS old key invalid;
	        DS new key valid;
	    */

	    if(current_key_exchange_state == CA_XGPON_ONU_KN3){
	        if(key_idx == 1){//new key index is 1 ,hw new key index is 0,old key index is 1
	            ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,0);//us new key valid in KN3
	            CA_XGPON_AES_RET_VALUE_CHECK(ret);

	            //ret = __send_key_report_msg(1);
	            //CA_XGPON_AES_RET_VALUE_CHECK(ret);
	            #if 0
				ca_mdelay(AAL_XGPON_INIT_DELAY);
	          	if(old_key_flag){
					key_ctrl_mask.s.key0_idx_vld = 1;
	                key_ctrl_mask.s.key1_idx_vld = 1;
				 	key_ctrl_cfg.key0_idx_vld = 1;
	                key_ctrl_cfg.key1_idx_vld = 0;
	                ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);//ds old key invalid in KN3
	                CA_XGPON_AES_RET_VALUE_CHECK(ret);
	            }
				#endif
	        }
	        if(key_idx == 2){//new key index is 2 ,hw new key index is 1,old key index is 0
	            ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,1);//us new key valid in KN3
	            CA_XGPON_AES_RET_VALUE_CHECK(ret);

	            //ret = __send_key_report_msg(1);
	            //CA_XGPON_AES_RET_VALUE_CHECK(ret);

				#if 0
				ca_mdelay(AAL_XGPON_INIT_DELAY);
	            if(old_key_flag){
	                key_ctrl_mask.s.key0_idx_vld = 1;
					key_ctrl_mask.s.key1_idx_vld = 1;
				 	key_ctrl_cfg.key0_idx_vld = 0;
					key_ctrl_cfg.key1_idx_vld = 1;
	                ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);//ds old key invalid in KN3
	                CA_XGPON_AES_RET_VALUE_CHECK(ret);
	            }
				#endif
	        }
	    }
	}

    return ret;
}

ca_status_t xgpon_ploam_key_exchange_fsm_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    //aal_xgpn_plmp_key_exchg_thrshd_mask_t  thrshd_mask;
    //aal_xgpn_plmp_key_exchg_thrshd_t       thrshd;
    aal_xgpon_dsfp_xgem_ctrl_config_msk_t dsfp_xgem_ctrl_mask;
    aal_xgpon_dsfp_xgem_ctrl_config_t dsfp_xgem_ctrl_cfg;

    current_key_exchange_state = CA_XGPON_ONU_KN0 ;
    previous_key_exchange_state = CA_XGPON_ONU_KN0 ;

    #if 0
    thrshd_mask.wrd = 0;
    memset(&thrshd,0,sizeof(thrshd));

    thrshd_mask.bf.tk5_timeout = 1;
    thrshd_mask.bf.tk4_timeout = 1;

    /*iniate tk4 and tk5 timer;
    */
    thrshd.tk4_timeout = 0x320;/*100ms*/
    thrshd.tk5_timeout = 0xa0;/*20ms*/
    ret = aal_ploam_key_exchg_thrshd_set(0,thrshd_mask,&thrshd);
    CA_RET_VALUE_CHECK(ret);
    #endif

    dsfp_xgem_ctrl_mask.u32 = 0;
    memset(&dsfp_xgem_ctrl_cfg,0,sizeof(aal_xgpon_dsfp_xgem_ctrl_config_t));

    //drop older key after key switch.
    //drop frame if  key_index out of range.
    dsfp_xgem_ctrl_mask.s.invld_key_xgem_drop_en = 1;
    dsfp_xgem_ctrl_mask.s.old_key_xgem_drop_en = 1;

    dsfp_xgem_ctrl_cfg.invld_key_xgem_drop_en = 1;
    dsfp_xgem_ctrl_cfg.old_key_xgem_drop_en = 1;

    ret = aal_xgpon_dsfp_xgem_ctrl_set(0,dsfp_xgem_ctrl_mask,&dsfp_xgem_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);

    ret = ca_spin_lock_init(&__xgpon_aes_lock, SPINLOCK_BH);
	CA_RET_VALUE_CHECK(ret);
    return ret;
}

//#define __CHECK_KEY_MSG_PROCESS_TIME 1
ca_status_t xgpon_ploam_key_exchange_handler(ca_device_id_t device_id,ca_uint8_t *p_key_exchange_ploam)
{//XGPN_PLMP_KEY_CTRL_STAT
    ca_status_t ret = CA_E_OK;
    ca_xgpon_key_control_ploam_msg_t *ds_ploam_msg = NULL ;

    aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t key_ctrl_mask;
    aal_xgpon_dsfp_xgem_key_ctrl_config_t key_ctrl_cfg;
    aal_key_report_frag_t key_rpt_frag;

    char trace_str[128] = {0};

#ifdef __CHECK_KEY_MSG_PROCESS_TIME
    ca_uint32_t  sfc_lsb = 0, sfc_msb = 0, sfc_new_lsb = 0, sfc_new_msb = 0;
    aal_xgpon_dsfp_super_frame_get(device_id, &sfc_lsb, &sfc_msb);
#endif

    if (!p_key_exchange_ploam) {
        ca_printf("p_key_exchange_ploam is NULL!\r\n");
        return CA_E_ERROR;
    }

    key_ctrl_mask.u32 = ~0U;

    memset(&key_ctrl_cfg,0,sizeof(aal_xgpon_dsfp_xgem_key_ctrl_config_t));
    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));

    ds_ploam_msg = (ca_xgpon_key_control_ploam_msg_t *)p_key_exchange_ploam;
	ds_ploam_msg->msg_type_id = CA_KEY_CONTROL;
	ds_ploam_msg->onu_id = g_onu_id;
	aes_ploam_dump_pkt("Recv Key_ctrl PLOAM:", p_key_exchange_ploam, 48);

	if((ds_ploam_msg->key_idx != 1)&&(ds_ploam_msg->key_idx != 2)) {
        ca_printf("ERROR:ds_ploam_msg->key_idx is %d! it should be 1 or 2.\r\n",ds_ploam_msg->key_idx);
        return CA_E_ERROR;
    }
	
    if(ds_ploam_msg->key_idx == 0){
        g_wrong_key_index = 0;
        ds_ploam_msg->key_idx = 1;
    }else{
        g_wrong_key_index = 1;
    }
    //g_onu_id = htons(g_xgpn_basic_info.onu_id);
    //g_msg_type_id = ds_ploam_msg->msg_type_id;
    
    __XGPN_AES_LOCK(0);
    g_key_len = ds_ploam_msg->key_length;
    g_key_ctrl_seqNo = ds_ploam_msg->seq_no;
    g_key_ctrl_flag = ds_ploam_msg->ctrl_flag;
    if(g_key_idx != ds_ploam_msg->key_idx){
        if((current_key_exchange_state == CA_XGPON_ONU_KN4)
           &&(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW)){
            key_change_flag = TRUE;//new key flag
            g_state_chg_cnt++;
            ca_printf("\r\nStart key exchange!new key index :%d.\r\n",ds_ploam_msg->key_idx);

        }
        if(g_key_idx == 255){
            old_key_flag = FALSE;//has no old key
            ca_printf("Has no old key.\r\n");
        }else{
            ca_printf("Has old key.\r\n");
            old_key_flag = TRUE;//has old key
            old_key_index = g_key_idx;
        }
        g_key_idx = ds_ploam_msg->key_idx;
    }else{
		if((current_key_exchange_state == CA_XGPON_ONU_KN4)
           &&(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW)){
           	ca_printf("Warning:Duplicated key index %d.\r\n",ds_ploam_msg->key_idx);
			key_change_flag = FALSE;//new key flag
        }
        if(current_key_exchange_state != CA_XGPON_ONU_KN4){
            key_change_flag = FALSE;//new key flag
        }
    }

    if (ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW){//generate new key
        g_rpt_type = 0;
		//ret = __send_key_report_msg(g_rpt_type);
		//CA_XGPON_AES_RET_VALUE_CHECK(ret);
    }else if(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_CONFIRM){
        g_rpt_type = 1;
		//ca_printf("g_state_chg_cnt %d.\r\n",g_state_chg_cnt);
		//ret = __send_key_report_msg(g_rpt_type);
		//CA_XGPON_AES_RET_VALUE_CHECK(ret);
    }

    switch (current_key_exchange_state) {
        case CA_XGPON_ONU_KN0:
            previous_key_exchange_state = CA_XGPON_ONU_KN0;
            current_key_exchange_state = CA_XGPON_ONU_KN0;
            #if 0
            key_ctrl_cfg.key0_idx_vld = 1;
            key_ctrl_cfg.key1_idx_vld = 1;
            ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
            CA_XGPON_AES_RET_VALUE_CHECK(ret);
            #endif
            if (ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW){
                previous_key_exchange_state = CA_XGPON_ONU_KN0;
                current_key_exchange_state = CA_XGPON_ONU_KN1;

                /*In KN1,If there is an old key, the old key is valid to receive and valid to transmit at the ONU.
                The new key is invalid to receive and invalid to transmit at the ONU.*/
                #if 0
                if(old_key_flag){
                    if(g_key_idx == 1){//for hw, new key idx is 0,old key idx is 1 if exsits.
                        key_ctrl_mask.s.key1_idx_vld = 1;
                        //key_ctrl_cfg.key0_idx_vld = 0;
                        key_ctrl_cfg.key1_idx_vld = 1;
                        ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
                        CA_XGPON_AES_RET_VALUE_CHECK(ret);
                        ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,1);//new key invalid,old key valid
                        CA_XGPON_AES_RET_VALUE_CHECK(ret);

                    }else if(g_key_idx == 2){//for hw, new key idx is 1,old key idx is 0 if exsits.
                        key_ctrl_mask.s.key0_idx_vld = 1;
                        key_ctrl_cfg.key0_idx_vld = 1;
                        //key_ctrl_cfg.key1_idx_vld = 0;
                        ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
                        CA_XGPON_AES_RET_VALUE_CHECK(ret);
                        ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,0);//new key invalid
                        CA_XGPON_AES_RET_VALUE_CHECK(ret);
                    }
                }
				#endif

                previous_key_exchange_state = CA_XGPON_ONU_KN1;
                current_key_exchange_state = CA_XGPON_ONU_KN2;
                __xgpon_key_valid_set(g_key_idx);
                //ret = __send_key_report_msg(0);
                //CA_XGPON_AES_RET_VALUE_CHECK(ret);


                XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            }
            break;
        case CA_XGPON_ONU_KN1:
            previous_key_exchange_state = CA_XGPON_ONU_KN1;
            current_key_exchange_state = CA_XGPON_ONU_KN1;
            if (!timeout4_flag){
                current_key_exchange_state = CA_XGPON_ONU_KN2;
            }

            break;
        case CA_XGPON_ONU_KN2:
            previous_key_exchange_state = CA_XGPON_ONU_KN2;
            current_key_exchange_state = CA_XGPON_ONU_KN2;

            /*send Key_Report(NewKey) PLOAM message ;*/
            if(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_CONFIRM){
                previous_key_exchange_state = CA_XGPON_ONU_KN2;
                current_key_exchange_state = CA_XGPON_ONU_KN3;

                __xgpon_key_valid_set(g_key_idx);
				#if 0
				if(0 == strcmp(olt_type, "ZTE")){
	                if(g_key_idx == 1){
	                    ca_generate_new_kek_and_kek_name(1);
						memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
						memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
					}
	                if(g_key_idx == 2){
	                    ca_generate_new_kek_and_kek_name(0);
						memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
						memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
	                }
				}
				#endif
                XGPN_AES_SM_DEBUG("Recv key conirm msg:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
                previous_key_exchange_state = CA_XGPON_ONU_KN3;
                if (!timeout4_flag){
                    current_key_exchange_state = CA_XGPON_ONU_KN4;
                }

                XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            }else if(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW){
                previous_key_exchange_state = CA_XGPON_ONU_KN2;
                current_key_exchange_state = CA_XGPON_ONU_KN2;

                //ret = __send_key_report_msg(0);
                //CA_XGPON_AES_RET_VALUE_CHECK(ret);

                XGPN_AES_SM_DEBUG("Recv key new msg:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            }else{
                previous_key_exchange_state = CA_XGPON_ONU_KN2;
                current_key_exchange_state = CA_XGPON_ONU_KN2;
                //ret = __send_key_report_msg(0);
                //CA_XGPON_AES_RET_VALUE_CHECK(ret);
            }

            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);

            break;
        case CA_XGPON_ONU_KN3:
            previous_key_exchange_state = CA_XGPON_ONU_KN3;
            current_key_exchange_state = CA_XGPON_ONU_KN3;

            /*send existing key report*/

            if (!timeout4_flag){
                current_key_exchange_state = CA_XGPON_ONU_KN4;
            }

            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            break;
        case CA_XGPON_ONU_KN4:

            previous_key_exchange_state = CA_XGPON_ONU_KN4;
            current_key_exchange_state = CA_XGPON_ONU_KN4;
            /*receive new key, change to state kn1*/
            if(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_NEW){
                previous_key_exchange_state = CA_XGPON_ONU_KN4;
                if(key_change_flag){
                    /*set current new key to old key */
                    current_key_exchange_state = CA_XGPON_ONU_KN1;
                    XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
                    previous_key_exchange_state = CA_XGPON_ONU_KN1;
                    current_key_exchange_state = CA_XGPON_ONU_KN2;
                    __xgpon_key_valid_set(g_key_idx);
                }
                //ret = __send_key_report_msg(0);
                //CA_XGPON_AES_RET_VALUE_CHECK(ret);

                XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            }else if(ds_ploam_msg->ctrl_flag == CA_KEY_CONTROL_CONFIRM){
                /*send existing key report*/
                //ret = __send_key_report_msg(1);
                //CA_XGPON_AES_RET_VALUE_CHECK(ret);
                XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            }

            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            break;

        default:
            XGPN_AES_SM_DEBUG("Invalid state!!\r\n");

            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            break;
    }

    __XGPN_AES_UNLOCK(0);

#ifdef __CHECK_KEY_MSG_PROCESS_TIME
    aal_xgpon_dsfp_super_frame_get(device_id, &sfc_new_lsb, &sfc_new_msb);

    if (sfc_new_lsb >= sfc_lsb) {
        ca_printf("From %s => %s Use about 0x%08x %08x XGPON cycles( %u us).\r\n",
            xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state],
            sfc_new_msb - sfc_msb, sfc_new_lsb - sfc_lsb, (sfc_new_lsb - sfc_lsb) * 125);
    } else {
        ca_printf("From %s => %s use about 0x%08x %08x XGPON cycles(%u us).\r\n",
            xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state],
            sfc_new_msb - sfc_msb - 1,  0xffffffff - (sfc_lsb - sfc_new_lsb), (0xffffffff - (sfc_lsb - sfc_new_lsb)) * 125);
    }
#endif

    // add debug trace memory
    snprintf(trace_str, sizeof(trace_str)-1, "Rx key_ctrl_flag %u,seq_no %u,key_idx %u,%s => %s\r\n",
        ds_ploam_msg->ctrl_flag,
        ds_ploam_msg->seq_no,
        ds_ploam_msg->key_idx,
        xgpn_key_exchg_state[previous_key_exchange_state],
        xgpn_key_exchg_state[current_key_exchange_state]);

    if ((sizeof(g_key_exchange_trace_mem) - 1) > strlen(g_key_exchange_trace_mem)) {
        strncat(g_key_exchange_trace_mem, trace_str, sizeof(g_key_exchange_trace_mem) - strlen(g_key_exchange_trace_mem) - 1);
    }
#if 0
    else {
        memset(g_key_exchange_trace_mem, 0, sizeof(g_key_exchange_trace_mem));
        strncat(g_key_exchange_trace_mem, trace_str, strlen(trace_str));
    }
#endif

    if ((current_key_exchange_state == CA_XGPON_ONU_KN4)
        && (current_key_exchange_state != previous_key_exchange_state)) {
        XGPN_AES_SM_DEBUG("AES Key %d exchange successfully go to KN4.\r\n", g_key_idx);
    }

    return CA_E_OK;
}


ca_status_t xgpon_key_exchange_tk4_timeout_handler()
{
    ca_status_t ret = CA_E_OK;

    __XGPN_AES_LOCK(0);
    timeout4_flag = TRUE;
    switch (current_key_exchange_state) {
        case CA_XGPON_ONU_KN0:
        case CA_XGPON_ONU_KN4:

            XGPN_AES_SM_DEBUG("ERROR::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            ret = CA_E_ERROR;
            break;
        case CA_XGPON_ONU_KN1:
        case CA_XGPON_ONU_KN2:
        case CA_XGPON_ONU_KN3:
            /*back to CA_XGPON_ONU_KN0*/
            current_key_exchange_state = CA_XGPON_ONU_KN0;
            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            break;
        default:

            XGPN_AES_SM_DEBUG("ERROR::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            ret = CA_E_ERROR;
            break;
    }

    timeout4_flag = FALSE;
    __XGPN_AES_UNLOCK(0);

    return ret;
}

ca_status_t xgpon_key_exchange_tk5_timeout_handler()
{
    ca_status_t ret = CA_E_OK;

    __XGPN_AES_LOCK(0);
    timeout5_flag = TRUE;
    switch (current_key_exchange_state) {
        case CA_XGPON_ONU_KN0:
        case CA_XGPON_ONU_KN1:
        case CA_XGPON_ONU_KN3:
        case CA_XGPON_ONU_KN4:
            XGPN_AES_SM_DEBUG("ERROR::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            ret = CA_E_ERROR;
            break;
        case CA_XGPON_ONU_KN2:
            /*back to CA_XGPON_ONU_KN2*/
            current_key_exchange_state = CA_XGPON_ONU_KN2;
            /*ret = __xgpon_key_rpt_ploam_build(0);*/
            //ret = __send_key_report_msg(0);
            //CA_XGPON_AES_RET_VALUE_CHECK(ret);
            XGPN_AES_SM_DEBUG("END:previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            break;
        default:
            XGPN_AES_SM_DEBUG("ERROR::previous state is :%s and current state is :%s\r\n",xgpn_key_exchg_state[previous_key_exchange_state],xgpn_key_exchg_state[current_key_exchange_state]);
            ret = CA_E_ERROR;
            break;
    }

    timeout5_flag = FALSE;
    __XGPN_AES_UNLOCK(0);

    return ret;
}

#define __XGPON_PLOAM_RESET_TIMER_INTERVAL_MS          100
#define __XGPON_PLOAM_TRY_TIMES          20

static void __xgpon_ploam_reset_timer_callback(void *param)
{
    XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
    int i = 0;
    reg_ctrl.wrd      = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);

	while(reg_ctrl.bf.pkt_rdy && i++ < __XGPON_PLOAM_TRY_TIMES) {
        reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	}
    
    if(i >= __XGPON_PLOAM_TRY_TIMES) {
		aal_xgpon_reset(0, AAL_XGPON_RESET_FLAG_MAC);
    }
}

void xgpon_aes_key_ctrl_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie)
{
	ca_xgpon_key_control_ploam_msg_t key_ctl_msg;

    memset(&key_ctl_msg, 0, sizeof(ca_xgpon_key_control_ploam_msg_t));
    if (xgpn_ploam_int & XGPN_PLMP_INTR_MASK_KEY_CTRL) {
#ifdef CONFIG_PROC_FS
        if (PON_DBG_GPON_AES_MSG & ca_pon_debug) {
            ca_printf("XGPON AES Key Control Msg Rx: seq_no %u, flag %u, key_idx %u, key_len %u.\n",
                key_ctl_msg.seq_no, key_ctl_msg.ctrl_flag, key_ctl_msg.key_idx, key_ctl_msg.key_length);
        }
#endif
		if(key_ctrl_stat != 0)
		{
		    key_ctl_msg.seq_no = key_ctrl_stat&0xFF;
		    key_ctl_msg.ctrl_flag = (key_ctrl_stat>>8)&0xFF;
		    key_ctl_msg.key_idx = (key_ctrl_stat>>16)&0xFF;
		    key_ctl_msg.key_length = (key_ctrl_stat>>24)&0xFF;
		}else{
			printk("ERROR:func %s :key_ctrl_stat :0x%x\r\n",__FUNCTION__,key_ctrl_stat);
		}
        xgpon_ploam_key_exchange_handler(AAL_DEFAUL_DEV_ID,(ca_uint8_t *)&key_ctl_msg);

         /*
         * Start polling timer to poll XGPN_USBG_PLOAMU_QU_CFG'no_empty and XGPN_PLMP_TXBUF_CTRL'pkt_rdy,
         * if they can't  change to be 0 , then re-init PON MAC, to recover the PON H/W logic
         */
        if (!g_xgpon_ploam_reset_timer) {
            g_xgpon_ploam_reset_timer = ca_circle_timer_add(__XGPON_PLOAM_RESET_TIMER_INTERVAL_MS,
                __xgpon_ploam_reset_timer_callback, NULL);

            if (g_xgpon_ploam_reset_timer == 0) {
                ca_printf("%s(): error! fail to add xgpon ploam reset timer!\r\n", __func__);
            }
        }
    }

}
EXPORT_SYMBOL(xgpon_aes_key_ctrl_handler);

void xgpon_aes_key_tk4_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie)
{
	
	//printk("func %s :Receive xgpn_ploam_int :0x%x from workqueue\r\n",__FUNCTION__,xgpn_ploam_int);
	if (xgpn_ploam_int & XGPN_PLMP_INTR_MASK_TK4_TIMEOUT) {
#ifdef CONFIG_PROC_FS
        if (PON_DBG_GPON_AES_MSG & ca_pon_debug) {
            ca_printf("XGPON AES TK4 timeout.\n");
        }
#endif
        xgpon_key_exchange_tk4_timeout_handler();
    }
}
EXPORT_SYMBOL(xgpon_aes_key_tk4_handler);

void xgpon_aes_key_tk5_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie)
{
	//printk("func %s :Receive xgpn_ploam_int :0x%x from workqueue\r\n",__FUNCTION__,xgpn_ploam_int);
    if (xgpn_ploam_int & XGPN_PLMP_INTR_MASK_TK5_TIMEOUT) {
#ifdef CONFIG_PROC_FS
        if (PON_DBG_GPON_AES_MSG & ca_pon_debug) {
            ca_printf("XGPON AES TK5 timeout.\n");
        }
#endif
        xgpon_key_exchange_tk5_timeout_handler();
    }
}
EXPORT_SYMBOL(xgpon_aes_key_tk5_handler);



void xgpon_key_exchange_fsm_reset(void)
{
    current_key_exchange_state = CA_XGPON_ONU_KN0;
    previous_key_exchange_state = CA_XGPON_ONU_KN0;
#if 0
    g_key_idx = 0;
    g_key_len = 0;
    g_key_ctrl_seqNo = 0;
    g_key_ctrl_flag = 0;
#endif
}
EXPORT_SYMBOL(xgpon_key_exchange_fsm_reset);

#if 0

int main(void)
{
    unsigned char L[16], K1[16], K2[16], T[16], T1[8], T2[4];

    unsigned char MSK[16] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };

    /*unsigned char ONU_SN[16] = {
        0x56, 0x4e, 0x44, 0x52, 0x00, 0x11, 0x22, 0x33
    };

    unsigned char PON_TAG[16] = {
        0x4f, 0x4c, 0x54, 0x23, 0x44, 0x55, 0x66, 0x77
    };*/

    //SK        = 0x79, 0x5f, 0xcf, 0x6c, 0xb2, 0x15, 0x22, 0x40, 0x87, 0x43, 0x06, 0x00, 0xdd, 0x17, 0x0f, 0x07
    unsigned char SK[16] = {
        0x79, 0x5f, 0xcf, 0x6c, 0xb2, 0x15, 0x22, 0x40,
        0x87, 0x43, 0x06, 0x00, 0xdd, 0x17, 0x0f, 0x07

    };

    //0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x4b
    unsigned char sk_msg[24] = {
        0x56, 0x4e, 0x44, 0x52, 0x00, 0x11, 0x22, 0x33,
        0x4f, 0x4c, 0x54, 0x23, 0x44, 0x55, 0x66, 0x77,
        0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x4b
    };

    unsigned char omci_ik_msg[16] = {
        0x4f, 0x4d, 0x43, 0x49, 0x49, 0x6e, 0x74, 0x65,
        0x67, 0x72, 0x69, 0x74, 0x79, 0x4b, 0x65, 0x79
    };

    unsigned char ploam_ik_msg[16] = {
        0x50, 0x4c, 0x4f, 0x41, 0x4d, 0x49, 0x6e, 0x74,
        0x65, 0x67, 0x72, 0x74, 0x79, 0x4b, 0x65, 0x79
    };

    unsigned char kek_msg[16] = {
        0x4b, 0x65, 0x79, 0x45, 0x6e, 0x63, 0x72, 0x79,
        0x70, 0x74, 0x69, 0x6f, 0x6e, 0x4b, 0x65, 0x79
    };


    /*unsigned char alloc_id_ploam_msg[41] ={
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
        0xff, 0x01, 0x04, 0x45, 0x03, 0x0a, 0x00, 0x13,0x01
    };*/
    /*unsigned char sleep_re_ploam_msg[41] ={
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x13,0x02
    };*/

    unsigned char PLOAM_IK[16] = {
        0xe2, 0x56, 0xce, 0x76, 0x78, 0x5c, 0x78, 0x71,
        0x7c, 0x7b, 0x30, 0x44, 0xab, 0x28, 0xe2, 0xcd

    };

    unsigned char alloc_id_ploam_msg[41] = {
        0x01, 0x00, 0x13, 0x0a, 0x03, 0x04, 0x45, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char sleep_re_ploam_msg[41] = {
        0x02, 0x00, 0x13, 0x10, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };




    //KEK       = 0x6f, 0x9c, 0x99, 0xb8, 0x36, 0x17, 0x68, 0x93, 0x7e, 0x45, 0x3b, 0x16, 0x5f, 0x60, 0x97, 0x10

    unsigned char KEK[16] = {
        0x6f, 0x9c, 0x99, 0xb8, 0x36, 0x17, 0x68, 0x93,
        0x7e, 0x45, 0x3b, 0x16, 0x5f, 0x60, 0x97, 0x10
    };
    unsigned char data_enc_key[16] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };

    unsigned char us_key_report_msg[32] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
        0x33, 0x31, 0x34, 0x31, 0x35, 0x39, 0x32, 0x36,
        0x35, 0x33, 0x35, 0x38, 0x39, 0x37, 0x39, 0x33
    };

    //OMCI_IK   = 0x18, 0x4b, 0x8a, 0xd4, 0xd1, 0xac, 0x4a, 0xf4, 0xdd, 0x4b, 0x33, 0x9e, 0xcc, 0x0d, 0x33, 0x70
    unsigned char OMCI_IK[16] = {
        0x18, 0x4b, 0x8a, 0xd4, 0xd1, 0xac, 0x4a, 0xf4,
        0xdd, 0x4b, 0x33, 0x9e, 0xcc, 0x0d, 0x33, 0x70
    };

    unsigned char omci_msg[45] = {
        0x01, 0x80, 0x00, 0x49, 0x0a, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x28
    };
    memset(L, 0, 16);
    memset(K1, 0, 16);
    memset(K2, 0, 16);
    memset(T, 0, 16);
    memset(T1, 0, 8);
    memset(T2, 0, 4);


    OS_PRINTF("--------------------------------------------------\n");
    OS_PRINTF("msk_Key: \n");
    print128(MSK);
    OS_PRINTF("\n");
    OS_PRINTF("\nSubkey Generation\n");

    aes128_ecb_encrypt(MSK, const_Zero, L);
    OS_PRINTF(" aes128_ecb_encrypt(MSK,0) ");
    print128(L);
    OS_PRINTF("\n");
    generate_subkey(MSK, K1, K2);
    OS_PRINTF("K1:\n");
    print128(K1);
    OS_PRINTF("\n");
    OS_PRINTF("K2:\n");
    print128(K2);
    OS_PRINTF("\n");


    OS_PRINTF("\nExample 1: len = 24\n");
    OS_PRINTF("SK Message:\n ");
    print_hex(" ", sk_msg, 24);
    aes128_cmac_compute(MSK, sk_msg, 24, 16, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print128(T);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 2: len = 16\n");
    OS_PRINTF("omci_ik_msg Message:\n ");
    print_hex(" ", omci_ik_msg, 16);
    aes128_cmac_compute(SK, omci_ik_msg, 16, 16, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print128(T);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 3: len = 16\n");
    OS_PRINTF("ploam_ik_msg Message:\n ");
    print_hex(" ", ploam_ik_msg, 16);
    aes128_cmac_compute(SK, ploam_ik_msg, 16, 16, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print128(T);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 4: len = 16\n");
    OS_PRINTF("kek_msg Message:\n ");
    print_hex(" ", kek_msg, 16);
    aes128_cmac_compute(SK, kek_msg, 16, 16, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print128(T);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 5: len = 41\n");
    OS_PRINTF("alloc_id_ploam_msg :\n ");
    print_hex(" ", alloc_id_ploam_msg, 41);
    aes128_cmac_compute(PLOAM_IK, alloc_id_ploam_msg, 41, 8, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print64(T1);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 6: len = 41\n");
    OS_PRINTF("sleep_re_ploam_msg :\n ");
    print_hex(" ", sleep_re_ploam_msg, 41);
    aes128_cmac_compute(PLOAM_IK, sleep_re_ploam_msg, 41, 8, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print64(T1);
    OS_PRINTF("\n");

    aes128_ecb_encrypt(KEK, data_enc_key, L);

    OS_PRINTF("\n");
    OS_PRINTF("encrypted data encryption key: ");
    print128(L);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 7: len = 32\n");
    OS_PRINTF("us_key_report_msg :\n ");
    print_hex(" ", us_key_report_msg, 32);
    aes128_cmac_compute(KEK, us_key_report_msg, 32, 16, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print128(T);
    OS_PRINTF("\n");

    OS_PRINTF("\nExample 8: len = 45\n");
    OS_PRINTF("omci_msg :\n ");
    print_hex(" ", omci_msg, 45);
    aes128_cmac_compute(OMCI_IK, omci_msg, 45, 4, T);
    OS_PRINTF("aes128_cmac_compute: ");
    print32(T2);
    OS_PRINTF("\n");


    OS_PRINTF("--------------------------------------------------\n");
    return 0;
}

#endif
EXPORT_SYMBOL(ca_generate_ploam_ik);
EXPORT_SYMBOL(ca_generate_omci_ik);
EXPORT_SYMBOL(xgpon_key_exchange_tk5_timeout_handler);
EXPORT_SYMBOL(ca_generate_omci_mic);
EXPORT_SYMBOL(ca_generate_default_ploam_mic);
EXPORT_SYMBOL(ca_generate_sk);
EXPORT_SYMBOL(ca_generate_pon_tag_digest);
EXPORT_SYMBOL(xgpon_key_exchange_tk4_timeout_handler);
EXPORT_SYMBOL(ca_generate_msk);
EXPORT_SYMBOL(ca_generate_onu_auth_result);
EXPORT_SYMBOL(ca_generate_kek_name);
EXPORT_SYMBOL(ca_generate_kek);
EXPORT_SYMBOL(ca_generate_ploam_mic);
EXPORT_SYMBOL(xgpon_ploam_key_exchange_handler);
EXPORT_SYMBOL(ca_generate_def_kek_and_kek_name);
EXPORT_SYMBOL(ca_generate_new_kek_and_kek_name);
EXPORT_SYMBOL(xgpn_aes_key_generate);
EXPORT_SYMBOL(xgpon_ploam_key_exchange_fsm_init);
EXPORT_SYMBOL(ca_xgpon_aes_sm_state_get);
EXPORT_SYMBOL(ca_xgpon_aes_sm_state_set);
EXPORT_SYMBOL(xgpon_aes_ploam_ack_mic_update);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(xgpn_aes_key_generate_by_msk);
#endif






