#ifndef __AAL_GPON_INTR_H__
#define __AAL_GPON_INTR_H__

/*GPON MAC interrupt1 mask bit */   

#define AAL_GPON_INTR_POSEDGE_LOS     0x00000001 
#define AAL_GPON_INTR_POSEDGE_LOF     0x00000002
#define AAL_GPON_INTR_POSEDGE_SF      0x00000004
#define AAL_GPON_INTR_POSEDGE_SD      0x00000008
#define AAL_GPON_INTR_POSEDGE_LCDG    0x00000010
#define AAL_GPON_INTR_POSEDGE_TF      0x00000020
#define AAL_GPON_INTR_POSEDGE_SUF     0x00000040
#define AAL_GPON_INTR_POSEDGE_MEM     0x00000080
#define AAL_GPON_INTR_POSEDGE_DACT    0x00000100
#define AAL_GPON_INTR_POSEDGE_DIS     0x00000200
#define AAL_GPON_INTR_POSEDGE_MIS     0x00000400
#define AAL_GPON_INTR_POSEDGE_PEE     0x00000800
#define AAL_GPON_INTR_POSEDGE_RDI     0x00001000
#define AAL_GPON_INTR_POPUP           0x00002000
#define AAL_GPON_INTR_CFGV            0x00004000
#define AAL_GPON_INTR_ENC_VPI         0x00008000
#define AAL_GPON_INTR_ALLOCID         0x00010000
#define AAL_GPON_INTR_PORTID          0x00020000
#define AAL_GPON_INTR_CPL             0x00040000
#define AAL_GPON_INTR_KSW             0x00080000
#define AAL_GPON_INTR_PDROP           0x00100000
#define AAL_GPON_INTR_HPLOAMU         0x00200000
#define AAL_GPON_INTR_UNSYNC          0x00400000
#define AAL_GPON_INTR_LF_UNDER        0x00800000
#define AAL_GPON_INTR_DS_PKT          0x01000000
#define AAL_GPON_INTR_DS_FRM          0x02000000
#define AAL_GPON_INTR_ONU_ID          0x40000000
#define AAL_GPON_INTR_ONU_ST_CHG      0x80000000

#define AAL_GPON_GRP1_INTR_ALL        0xC3FFFFFF
#define AAL_GPON_GRP1_INTR_NUM        28
#define AAL_GPON_GRP1_INTR_DEF        0x00000000


/*GPON MAC interrupt2 mask bit */ 

#define AAL_GPON_INTR2_BAD_ONU_ID      0x00000001 
#define AAL_GPON_INTR2_UFRAMEREQ_SEC   0x04000000
#define AAL_GPON_INTR2_UFRAMEREQ_DED   0x08000000
#define AAL_GPON_INTR2_TCPARSE_SEC     0x10000000
#define AAL_GPON_INTR2_TCPARSE_DED     0x20000000
#define AAL_GPON_INTR2_TCREUQUE_SEC    0x40000000
#define AAL_GPON_INTR2_TCREUQUE_DED    0x80000000

#define AAL_GPON_GRP2_INTR2_ALL        0xFC000001
#define AAL_GPON_GRP2_INTR2_NUM        7
#define AAL_GPON_GRP2_INTR2_DEF        0x00000000

/*GPON MAC interrupt3 mask bit */ 

#define AAL_GPON_INTR3_NEGEDGE_LOS                0x00000001 
#define AAL_GPON_INTR3_NEGEDGE_LOF                0x00000002
#define AAL_GPON_INTR3_NEGEDGE_SF                 0x00000004
#define AAL_GPON_INTR3_NEGEDGE_SD                 0x00000008
#define AAL_GPON_INTR3_NEGEDGE_LCDG               0x00000010
#define AAL_GPON_INTR3_NEGEDGE_TF                 0x00000020
#define AAL_GPON_INTR3_NEGEDGE_SUF                0x00000040
#define AAL_GPON_INTR3_NEGEDGE_DACT               0x00000100
#define AAL_GPON_INTR3_NEGEDGE_DIS                0x00000200
#define AAL_GPON_INTR3_NEGEDGE_MIS                0x00000400
#define AAL_GPON_INTR3_NEGEDGE_PEE                0x00000800
#define AAL_GPON_INTR3_NEGEDGE_RDI                0x00001000
#define AAL_GPON_INTR3_BWMAP_HWDONE               0x00004000
#define AAL_GPON_INTR3_BIP_ERROR_ACCUM_MSB        0x00100000
#define AAL_GPON_INTR3_BIP_ERROR_FRM_CNT_MSB      0x00200000
#define AAL_GPON_INTR3_PLEND_BITERR_MSB           0x00400000
#define AAL_GPON_INTR3_PLEND_ERR_MSB              0x00800000
#define AAL_GPON_INTR3_OMCI_CRC_MSB               0x01000000
#define AAL_GPON_INTR3_O5_MSB                     0x04000000
#define AAL_GPON_INTR3_GEM_FRAG_DROP_MSB          0x08000000
#define AAL_GPON_INTR3_GEM_ONE_BITERR_MSB         0x10000000
#define AAL_GPON_INTR3_GEM_TWO_BITERR_MSB         0x20000000
#define AAL_GPON_INTR3_GEM_UNCORRECTED_BITER_MSB  0x40000000
#define AAL_GPON_INTR3_BWMAP_DROP_MSB             0x80000000

#define AAL_GPON_GRP3_INTR3_ALL                   0xFDF05F7F
#define AAL_GPON_GRP3_INTR3_NUM                   24
#define AAL_GPON_GRP3_INTR3_DEF                   0x00000000

/*GPON MAC interrupt4 mask bit */ 

#define AAL_GPON_INTR4_VOQ0_FIFO_OVERRUN     0x00000001
#define AAL_GPON_INTR4_VOQ1_FIFO_OVERRUN     0x00000002
#define AAL_GPON_INTR4_VOQ2_FIFO_OVERRUN     0x00000004
#define AAL_GPON_INTR4_VOQ3_FIFO_OVERRUN     0x00000008
#define AAL_GPON_INTR4_VOQ4_FIFO_OVERRUN     0x00000010
#define AAL_GPON_INTR4_VOQ5_FIFO_OVERRUN     0x00000020
#define AAL_GPON_INTR4_VOQ6_FIFO_OVERRUN     0x00000040
#define AAL_GPON_INTR4_VOQ7_FIFO_OVERRUN     0x00000080
#define AAL_GPON_INTR4_VOQ8_FIFO_OVERRUN     0x00000100
                                        /* 0x00000200 */
#define AAL_GPON_INTR4_VOQ0_PTP_TX           0x00000400
#define AAL_GPON_INTR4_VOQ1_PTP_TX           0x00000800
#define AAL_GPON_INTR4_VOQ2_PTP_TX           0x00001000
#define AAL_GPON_INTR4_VOQ3_PTP_TX           0x00002000
#define AAL_GPON_INTR4_VOQ4_PTP_TX           0x00004000
#define AAL_GPON_INTR4_VOQ5_PTP_TX           0x00008000
#define AAL_GPON_INTR4_VOQ6_PTP_TX           0x00010000
#define AAL_GPON_INTR4_VOQ7_PTP_TX           0x00020000
#define AAL_GPON_INTR4_VOQ8_PTP_TX           0x00040000
                                        /* 0x00080000 */
#define AAL_GPON_INTR4_VOQ0_SYNC_ERR         0x00100000
#define AAL_GPON_INTR4_VOQ1_SYNC_ERR         0x00200000
#define AAL_GPON_INTR4_VOQ2_SYNC_ERR         0x00400000
#define AAL_GPON_INTR4_VOQ3_SYNC_ERR         0x00800000
#define AAL_GPON_INTR4_VOQ4_SYNC_ERR         0x01000000
#define AAL_GPON_INTR4_VOQ5_SYNC_ERR         0x02000000
#define AAL_GPON_INTR4_VOQ6_SYNC_ERR         0x04000000
#define AAL_GPON_INTR4_VOQ7_SYNC_ERR         0x08000000
#define AAL_GPON_INTR4_VOQ8_SYNC_ERR         0x10000000
#define AAL_GPON_INTR4_OMCI_FIFO_PKT_RDY     0x40000000
#define AAL_GPON_INTR4_OMCI_FIFO_OVRFLOW     0x80000000

#define AAL_GPON_GRP4_INTR4_ALL              0xDFF7FDFF
#define AAL_GPON_GRP4_INTR4_NUM              29
#define AAL_GPON_GRP4_INTR4_DEF              0x00000000

typedef enum {
	CA_EVENT_GPON_ONU_STATE_CHNG = 1, /* onu_state change */
	CA_EVENT_GPON_ONU_ID,			 /* onu_id assigned */
	CA_EVENT_GPON_OMCC_GEM_PORT_ID,	 /* received OMCC GEM PortId */
	CA_EVENT_GPON_OMCC_STATE,		 /* OMCC state change - up or down */
	CA_EVENT_GPON_PLOAMU_TX_DONE,	 /* SW generated PLOAM tx done */
	CA_EVENT_GPON_PLOAM_KSW, 		 /* Key switch event */
	CA_EVENT_GPON_ALM_LOS,			 /* LOS alarm set or clear */
	CA_EVENT_GPON_ALM_LOF,			 /* LOF alarm set or clear */
	CA_EVENT_GPON_ALM_SF,			 /* SF(signal fail) alarm set or clear */
	CA_EVENT_GPON_ALM_SD,			 /* SD(signal degrade) alarm set or clear */
	CA_EVENT_GPON_ALM_LCDG,			 /* LCDG(loss of cell del) detected */
	CA_EVENT_GPON_ALM_TF,			 /* TF (tx fail) alarm set or clear */
	CA_EVENT_GPON_ALM_SUF,			 /* SUF event detected */
	CA_EVENT_GPON_ALM_MEM,			 /* MEM event detected */
	CA_EVENT_GPON_ALM_DACT,			 /* DEACT event detected */
	CA_EVENT_GPON_ALM_DIS,			 /* DIS event detected */
	CA_EVENT_GPON_ALM_MIS,			 /* MIS event detected */
	CA_EVENT_GPON_ALM_PEE,			 /* PEE event detected */
	CA_EVENT_GPON_ALM_RDI,			 /* RDI alarm set or clear */
	CA_EVENT_GPON_ERR_PLOAMU_OFLOW,	 /* GPON MAC err - PLOAMu queue overflow */
	CA_EVENT_GPON_ERR_MAC_UNSYNC,	 /* GPON MAC err - int logic unsync */
	CA_EVENT_GPON_ERR_FIFO_UNRUN,	 /* GPON MAC err - latency FIFO underrun */
	CA_EVENT_GPON_BAD_ONU_ID,		 /* Bad ONU ID (254 or 255) received */
	CA_EVENT_GPON_DS_OMCI_FIFO_OVFLOW, /* DS OMCI tail drop due to FIFO ovflow */
	CA_EVENT_GPON_MAX
}aal_gpon_act_event_t;   


/* XGPON interrupt index  */
typedef enum {
    AAL_GPON_INTR  = 0,
    AAL_GPON_INTR2 = 1,
    AAL_GPON_INTR3 = 2,
    AAL_GPON_INTR4 = 3,
    /* add intr type here */

    AAL_GPON_INTR_NUM
}aal_gpon_intr_e;


typedef enum {
    AAL_GPON_KMSG_CHN_INTR  = 0,
    AAL_GPON_KMSG_CHN_INTR2 = 1,
    AAL_GPON_KMSG_CHN_INTR3 = 2,
    AAL_GPON_KMSG_CHN_INTR4 = 3,
    AAL_GPON_KMSG_CHN_NUM
} aal_gpon_intr_msg_e;

typedef struct {
    ca_uint32_t  intr;
    ca_uint32_t  onu_state;
    ca_uint32_t  onu_id;
}aal_gpon_intr_kmsg_t;

typedef struct {
    char        *name;         /* interrupt name */
    //ca_uint32_t  hw_irq;       /* hardware irq id */
    ca_uint32_t  ena_reg_addr; /* address of INTR enable register */
    ca_uint32_t  src_reg_addr; /* address of INTR source register */
    ca_uint32_t  ena_bmp;      /* default enable sub-interrupt bitmap */
    ca_uint64_t  count;        /* count of INTR */
    ca_uint64_t  fail_count;   /* count of fail handled INTR */
    ca_uint64_t  handle_count; /* count of ok handled INTR */
}aal_gpon_intr_t;


void aal_gpon_intr_handler(/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id, ca_uint32_t gpon_int);
void aal_gpon_intr_wq_proc(/*CODEGEN_IGNORE_TAG*/struct work_struct *wq);
ca_status_t aal_gpon_intr_init(/*CODEGEN_IGNORE_TAG*/CA_IN ca_device_id_t  device_id);
#endif /* __AAL_GPON_INTR_H__ */
