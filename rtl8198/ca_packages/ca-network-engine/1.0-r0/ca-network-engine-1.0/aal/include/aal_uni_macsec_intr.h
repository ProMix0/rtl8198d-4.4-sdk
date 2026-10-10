#ifndef __AAL_UNI_MACSEC_INTR_H__
#define __AAL_UNI_MACSEC_INTR_H__

#include "ca_types.h"

/* NI_HV_MACSEC interrupt index  */
typedef enum {
    AAL_NI_HV_MACSEC_NANDI_INTR_RX_DECRY   = 0,
    AAL_NI_HV_MACSEC_NANDI_INTR_TX_ENCRY   = 1,
    /* add intr type here */

    AAL_NI_HV_MACSEC_INTR_NUM
}aal_uni_macsec_intr_t;


typedef enum {
    AAL_NI_HV_MACSEC_NANDI_KMSG_RX_DECRY  = 0,
    AAL_NI_HV_MACSEC_NANDI_KMSG_TX_ENCRY = 1,
    
    AAL_NI_HV_MACSEC_KMSG_CHN_NUM
} aal_uni_macsec_intr_msg_t;
    
#define AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET 0x140

/* NI_HV MACSEC NANDI interrupt mask bit */
#define AAL_NI_HV_MACSEC_INTR_NANDI_CPU_CMD_EXECUTED_EN_BIT       0
#define AAL_NI_HV_MACSEC_INTR_NANDI_PN_THLD_REACHED_EN_BIT        1
#define AAL_NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_ERR_EN_BIT    2
#define AAL_NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_CERR_EN_BIT   3
#define AAL_NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_ERR_EN_BIT        4
#define AAL_NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_CERR_EN_BIT       5
#define AAL_NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_ERR_EN_BIT          6
#define AAL_NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_CERR_EN_BIT         7

typedef ca_status_t (*aal_uni_macsec_intr_callback_t)(/*CODEGEN_IGNORE_TAG*/ca_uint32_t intr_id, ca_uint32_t intr_sts);
typedef void (*aal_uni_macsec_intr_msg_handler_t)(/*CODEGEN_IGNORE_TAG*/ca_uint32_t data0, ca_uint32_t data1, ca_uint32_t data2, void *param);

typedef struct ca_ni_macsec {
	unsigned int irq_ni_macsec_de;	/* IRQ number of rx decode interrupt */
	unsigned int irq_ni_macsec_en;	/* IRQ number of tx encode interrupt */
} ca_ni_macsec_t;

ca_status_t aal_uni_macsec_intr_enable_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t       intr_idx,
    CA_IN ca_uint32_t       enable_bmp);

ca_status_t aal_uni_macsec_intr_enable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx);

ca_status_t aal_uni_macsec_intr_disable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t       intr_idx);

ca_status_t aal_uni_macsec_intr_enable_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_OUT ca_uint32_t      *enable_bmp);

ca_status_t aal_uni_macsec_intr_status_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_OUT ca_uint32_t      *status_bmp);

ca_status_t aal_uni_macsec_intr_info_get(
    CA_IN  ca_device_id_t   device_id);

ca_status_t aal_uni_macsec_intr_name_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_IN  ca_uint32_t      name_len,
    CA_OUT char             *name_str /* [name_len:2048] */ );

ca_status_t aal_uni_macsec_intr_clr(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_IN  ca_uint32_t      clr_bmp);

ca_status_t aal_uni_macsec_intr_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t  device_id);

ca_status_t aal_uni_macsec_intr_handler_reg(/*CODEGEN_IGNORE_TAG*/
    aal_uni_macsec_intr_msg_t           intr_msg,
    aal_uni_macsec_intr_msg_handler_t   handler);

ca_status_t aal_uni_macsec_intr_handler_unreg(/*CODEGEN_IGNORE_TAG*/
    aal_uni_macsec_intr_msg_t   intr_msg,
    aal_uni_macsec_intr_msg_handler_t handler);

void aal_uni_macsec_intr_handler(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t  device_id, 
    ca_uint32_t     irq, 
    ca_uint32_t     uni_macsec_int);

#endif /* __AAL_NI_HV_MACSEC_INTR_H__ */

