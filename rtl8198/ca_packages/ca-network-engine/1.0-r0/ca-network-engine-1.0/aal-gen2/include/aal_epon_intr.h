#ifndef __AAL_EPON_INTR_H__
#define __AAL_EPON_INTR_H__

#include "ca_types.h"


/* intr trace level */
typedef enum
{
    AAL_EPON_INTR_TRACE_LOG = 0x1,
    AAL_EPON_INTR_TRACE_MIN = 0x2,
    AAL_EPON_INTR_TRACE_MAJ = 0x4,
    AAL_EPON_INTR_TRACE_CRI = 0x8,
    AAL_EPON_INTR_TRACE_ALL = 0xf
}aal_epon_intr_trace_level_t;

/*
** EPON interrupt index, used for interrupt source/enable address (aal_epon_intr_tbl),
** refer to EPON_EPON_GLB_INTERRUPT.
*/
typedef enum
{
    AAL_EPON_INTR_INDEX_1GEPN  = 0, /* interrupt from 1G path       */
    AAL_EPON_INTR_INDEX_1GPCS,  /* interrupt from 1G RX PCS       */

    AAL_EPON_INTR_INDEX_XGEPN,     /* interrupt from 10G path      */
    AAL_EPON_INTR_INDEX_XGPCS,     /* interrupt from 10G RX PCS    */

    AAL_EPON_INTR_INDEX_1PPS,      /* interrupt from onu 1pps      */
    AAL_EPON_INTR_INDEX_PTP,       /* interrupt from ptp block     */
//    AAL_EPON_INTR_INDEX_RX_CORESEC10    = 4,    /* interrupt from d/s coresec10 */
//    AAL_EPON_INTR_INDEX_TX_CORESEC10    = 5,    /* interrupt from u/s coresec10 */
    AAL_EPON_INTR_INDEX_MPCP,      /* interrupt from mpcp          */
    AAL_EPON_INTR_INDEX_MAX   = AAL_EPON_INTR_INDEX_MPCP
} aal_epon_intr_index_t;

typedef enum
{
    /* 1G EPN */
    AAL_EPON_INTR_TYPE_1GEPN_LINK_CHANGE = 0,     /* 1G EPON PCS link up/down interrupt */

    /* XGEPN */
    AAL_EPON_INTR_TYPE_XGEPN_LINK_CHANGE,       /* 10G EPON PCS link up/down interrupt */
    AAL_EPON_INTR_TYPE_XGEPN_SYNC_DET,            /* the receiver detects synchronization and loss of synchronization */

    //AAL_EPON_INTR_DISCOVERY_FRAME,            /* Discovery frame received */
    //AAL_EPON_INTR_ENC_ENCRYPTED_RECEIVED,     /* Encrypted frame received interrupt */
    /* 1PPS */
    /* PTP */
    /* CORESEC10 */

    /* MPCP */
    AAL_EPON_INTR_TYPE_MPCP_REG_CHANGE,  /* ONU REG status change interrupt */
    AAL_EPON_INTR_TYPE_MPCP_RX_REG_FRAME,       /* a valid MPCP register frame received */
    AAL_EPON_INTR_TYPE_MPCP_MPCP_TIMEOUT,       /* MPCP timeout interrupt */
    AAL_EPON_INTR_TYPE_MPCP_GATE_TIMEOUT,       /* gate timeout interrupt */
    AAL_EPON_INTR_TYPE_MPCP_TIMESTAMP_DRIFT,    /* timestamp drift interrupt */
    /* TYPE END */
    AAL_EPON_INTR_TYPE_MAX = AAL_EPON_INTR_TYPE_MPCP_TIMESTAMP_DRIFT
} aal_epon_intr_type_t;

typedef void (*aal_epon_intr_handler_f)(ca_uint32_t info);

typedef struct aal_epon_intr_handler_list
{
    aal_epon_intr_handler_f func;
    struct aal_epon_intr_handler_list *next;
} aal_epon_intr_handler_list_t;


ca_status_t aal_epon_intr_handler_reg(/*CODEGEN_IGNORE_TAG*/
    CA_IN aal_epon_intr_type_t type,
    CA_IN aal_epon_intr_handler_f handler);

void aal_epon_intr_handler(/*CODEGEN_IGNORE_TAG*/
    CA_IN aal_epon_intr_type_t type,
    CA_IN ca_uint32_t info);

void aal_epon_intr_1gepn_handler(/*CODEGEN_IGNORE_TAG*/void);
void aal_epon_intr_1gepn_msg_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);

void aal_epon_intr_xgepn_handler(/*CODEGEN_IGNORE_TAG*/void);
void aal_epon_intr_xgepn_msg_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);
void aal_epon_intr_mpcp_handler(/*CODEGEN_IGNORE_TAG*/void);
void aal_epon_intr_mpcp_msg_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);

ca_status_t aal_epon_intr_glb_handler(/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id, ca_uint32_t irq, ca_uint32_t epon_intr);


ca_status_t aal_epon_intr_debug_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  level
);

ca_status_t aal_epon_intr_debug_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint32_t  *level
);


ca_status_t aal_epon_intr_ena_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t intr_idx,
    CA_IN ca_uint32_t intr_en
);

ca_status_t aal_epon_intr_ena_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint32_t intr_idx,
    CA_OUT ca_uint32_t *intr_en
);

ca_status_t aal_epon_intr_ena /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t intr_idx,
    CA_IN ca_uint32_t enable_bmp
);

ca_status_t aal_epon_intr_dis /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t intr_idx,
    CA_IN ca_uint32_t disable_bmp
);

ca_status_t aal_epon_intr_status_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint32_t intr_idx,
    CA_OUT ca_uint32_t *status_bmp
);

ca_status_t aal_epon_intr_statistics_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint32_t intr_idx,
    CA_OUT ca_uint64_t *stats
);

ca_status_t aal_epon_intr_status_clr /*CODEGEN_IGNORE_TAG*/
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint32_t intr_idx,
    CA_IN  ca_uint32_t clr_bmp
);

ca_status_t aal_epon_intr_init(/*CODEGEN_IGNORE_TAG*/
    ca_device_id_t  device_id);


#endif /* __AAL_EPON_INTR_H__ */

