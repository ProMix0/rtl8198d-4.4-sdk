#ifndef __AAL_PON_H__
#define __AAL_PON_H__

#ifdef CONFIG_NE_PON

#include "osal_cmn.h"
#include <linux/workqueue.h>
#include "aal_psds.h"
#include "aal_epon.h"
#include "aal_epon_mac.h"
#include "aal_epon_fec.h"
#include "aal_epon_intr.h"
#include "aal_mpcp.h"
#include "aal_epon_crypto.h"

#include "aal_xgpon.h"
#include "aal_xgpon_intr.h"
#include "aal_xgpon_actp.h"
#include "aal_ploam.h"
#include "ploam_api.h"
#include "xgpon_aes.h"
#include "xgpon.h"
#include "peri.h"

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
extern void *ca_g3_hgu_ne_reg_iobase;
extern void *ca_g3_hgu_glb_reg_iobase;
extern void *ca_g3_hgu_pon_reg_iobase;
extern void *ca_g3_peri_reg_iobase;
extern void *ca_g3_hgu_peri_reg_iobase;
void aal_hgu_8279_ne_reg_base_init(/*CODEGEN_IGNORE_TAG*/void **p_iobase);
#endif

extern ca_uint32_t ca_pon_debug;
extern ca_uint32_t g_pon_chip_version;

/* PON proc debug flags */
#define PON_DBG_DISABLE            0
#define PON_DBG_INTR               0x00000001
#define PON_DBG_GPON_ACT_FSM       0x00000002
#define PON_DBG_GPON_PLOAM_RX      0x00000004
#define PON_DBG_GPON_PLOAM_TX      0x00000008
#define PON_DBG_GPON_AES_MSG       0x00000010
#define PON_DBG_GPON_AES_FSM       0x00000020
#define PON_DBG_GPON_OMCI_RX       0x00000040
#define PON_DBG_GPON_OMCI_TX       0x00000080
#define PON_DBG_EPON_MPCP          0x00000100
#define PON_DBG_EPON_OAM_RX        0x00000200
#define PON_DBG_EPON_OAM_TX        0x00000400
#define PON_DBG_EVT                0x00000800
#define PON_DBG_GPON_API           0x00001000

#define TAROKO_PON_MAC_INTR_BIT    0x100
#define AAL_DEFAUL_DEV_ID          0
#define AAL_PON_MODE_DEFAULT       ONU_PON_MAC_XGSPON
#define AAL_PON_PORT_ID            7

#define AAL_CLK_SEQ_RESET_TRY_COUNT 5

#ifndef AAL_ASSERT_RET
#define AAL_ASSERT_RET(x, ret) \
    do { \
        if (!(x))  { \
            OS_PRINTF("%s() _%d_: assert %s fail\n", __FUNCTION__, __LINE__, #x); \
            return ret; \
        } \
    } while (0)
#endif

/* spinlock for PON SerDES reset protection */
extern ca_uint g_pon_spin_lock;
#define __PON_LOCK()    \
    do{ \
        ca_spin_lock(g_pon_spin_lock);\
    }while(0)

#define __PON_UNLOCK()    \
    do{ \
        ca_spin_unlock(g_pon_spin_lock);\
    }while(0)


#ifdef CONFIG_ARCH_CORTINA_G3HGU
#define AAL_PON_REG_ADDR(reg)        (volatile void *)((((ca_uint64_t)(reg)) & 0x0000FFFF) + ca_g3_hgu_pon_reg_iobase)
#define AAL_PON_GLB_REG_ADDR(reg)    (volatile void *)((((ca_uint64_t)(reg)) & 0x0000FFFF) + ca_g3_hgu_glb_reg_iobase)
#define AAL_PON_NE_REG_ADDR(reg)     (volatile void *)((((ca_uint64_t)(reg)) & 0x0000FFFF) + ca_g3_hgu_ne_reg_iobase)
/*Saturn->Peri*/
#define AAL_PON_PERI_REG_ADDR(reg)   (volatile void *)((((ca_uint32_t)(reg)) & 0x00000FFF) + ca_g3_hgu_peri_reg_iobase)
/*G3->Peri*/
#define AAL_PERI_REG_ADDR(reg)   (volatile void *)((((ca_uint32_t)(reg)) & 0x00000FFF) + ca_g3_peri_reg_iobase)
#else
#define AAL_PON_REG_ADDR(reg)        (void *)(reg)
#define AAL_PON_GLB_REG_ADDR(reg)    (void *)(reg)
#define AAL_PON_NE_REG_ADDR(reg)     (void *)(reg)
#define AAL_PON_PERI_REG_ADDR(reg)   (void *)(reg)
#define AAL_PERI_REG_ADDR(reg)       (void *)(reg)
#endif

#define AAL_PERI_REG_READ(reg)            CA_REG_READ(AAL_PERI_REG_ADDR(reg))
#define AAL_PERI_REG_WRITE(data, reg)        CA_REG_WRITE(data, AAL_PERI_REG_ADDR(reg))

#define AAL_PERI_REG_FIELD_READ(reg, field, val)       do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PERI_REG_READ(reg); \
                                                                (val) = reg_data.bf.field;\
                                                                }while(0)


#define AAL_PERI_REG_FIELD_WRITE(reg, field, val)      do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PERI_REG_READ(reg);\
                                                                reg_data.bf.field = (val);\
                                                                AAL_PERI_REG_WRITE(reg_data.wrd, reg); \
                                                                }while(0)

#define AAL_PON_PERI_REG_READ(reg)              CA_REG_READ(AAL_PON_PERI_REG_ADDR(reg))
#define AAL_PON_PERI_REG_WRITE(data, reg)          CA_REG_WRITE(data, AAL_PON_PERI_REG_ADDR(reg))

#define AAL_PON_PERI_REG_FIELD_READ(reg, field, val)       do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PON_PERI_REG_READ(reg); \
                                                                (val) = reg_data.bf.field;\
                                                                }while(0)


#define AAL_PON_PERI_REG_FIELD_WRITE(reg, field, val)      do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PON_PERI_REG_READ(reg);\
                                                                reg_data.bf.field = (val);\
                                                                AAL_PON_PERI_REG_WRITE(reg_data.wrd, reg); \
                                                                }while(0)


#define AAL_PON_REG_READ(reg)            CA_REG_READ(AAL_PON_REG_ADDR(reg))
#define AAL_PON_REG_WRITE(data, reg)     CA_REG_WRITE(data, AAL_PON_REG_ADDR(reg))

#define AAL_PON_REG_FIELD_READ(reg, field, val)       do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PON_REG_READ(reg); \
                                                                (val) = reg_data.bf.field;\
                                                                }while(0)


#define AAL_PON_REG_FIELD_WRITE(reg, field, val)      do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_PON_REG_READ(reg);\
                                                                reg_data.bf.field = (val);\
                                                                AAL_PON_REG_WRITE(reg_data.wrd, reg); \
                                                                }while(0)

#define AAL_GLB_REG_READ(reg)            CA_REG_READ(AAL_PON_GLB_REG_ADDR(reg))
#define AAL_GLB_REG_WRITE(data, reg)     CA_REG_WRITE(data, AAL_PON_GLB_REG_ADDR(reg))

#define AAL_GLB_REG_FIELD_READ(reg, field, val)       do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_GLB_REG_READ(reg); \
                                                                (val) = reg_data.bf.field;\
                                                                }while(0)


#define AAL_GLB_REG_FIELD_WRITE(reg, field, val)      do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_GLB_REG_READ(reg);\
                                                                reg_data.bf.field = (val);\
                                                                AAL_GLB_REG_WRITE(reg_data.wrd, reg); \
                                                                }while(0)

#define AAL_NE_REG_READ(reg)            CA_REG_READ(AAL_PON_NE_REG_ADDR(reg))
#define AAL_NE_REG_WRITE(data, reg)     CA_REG_WRITE(data, AAL_PON_NE_REG_ADDR(reg))

#define AAL_NE_REG_FIELD_READ(reg, field, val)       do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_NE_REG_READ(reg); \
                                                                (val) = reg_data.bf.field;\
                                                                }while(0)


#define AAL_NE_REG_FIELD_WRITE(reg, field, val)      do{\
                                                                reg##_t reg_data;\
                                                                reg_data.wrd = AAL_NE_REG_READ(reg);\
                                                                reg_data.bf.field = (val);\
                                                                AAL_NE_REG_WRITE(reg_data.wrd, reg); \
                                                                }while(0)

#define __FIND_INDIRCT_ADDRESS(rbw, index)    (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|(ca_uint32_t)(index))
#define __CHECK_INDIRCT_OPERATE_STATE(reg) do{\
    ca_uint32_t i = 0; \
    ca_uint32_t val32; \
    for (i = 0; i<10000; i++){ \
       val32 = AAL_PON_REG_READ((void *)(long)reg); \
       if ((val32 & ((ca_uint32)1 <<31)) == 0) {    \
        break; \
        } \
    } \
    if (i>= 10000) \
        printk("%s(): access %s timeout.\n", __func__, #reg); \
}while(0)

#define __DO_INDIRCT_OP(rbw, index, reg) AAL_PON_REG_WRITE(__FIND_INDIRCT_ADDRESS(rbw, index), reg); \
                                  __CHECK_INDIRCT_OPERATE_STATE(reg)

#define AAL_PON_REG_INDIRECT_READ(index, reg) __DO_INDIRCT_OP(0, index, reg)
#define AAL_PON_REG_INDIRECT_WRITE(index, reg) __DO_INDIRCT_OP(1, index, reg)

/* work queue related data structure */
#define PON_WORK_INFO_DATA_BUFF_LENGTH       6
typedef struct
{
    struct work_struct work;
    ca_uint32_t data[PON_WORK_INFO_DATA_BUFF_LENGTH];
} pon_work_info_t;

typedef struct
{
    struct workqueue_struct *pon_wq;
    pon_work_info_t work_info;
} pon_wq_t;

typedef struct {
    ca_uint32_t     vendor_id;            /* vendor id, 4 bytes */
    ca_uint32_t     vssn;                 /* vssn, 4 bytes */
    ca_uint8_t      registration_id[36];  /* registration id */
    ca_uint16_t     onu_id;
    ca_uint8_t      pon_mode; /* refer to ca_scfg_pon_mac_mode_t */
    ca_uint8_t      olt_type; /* refer to ca_scfg_olt_type_t*/
    /*ca_uint8_t      ds_map_enable;*/
    ca_uint8_t      voq_mode; /* refer to aal_puc_voq_mode_t*/
    ca_uint8_t      default_channel;
    ca_uint8_t      protect_channel;
    ca_uint8_t      cpi;
    ca_uint32_t     toz;
    ca_uint32_t     to23;
    ca_uint32_t     to4;
    ca_uint32_t     to5;
    ca_uint32_t     battery_up;
	ca_uint32_t     bwmp_rsp_tm;
    ca_uint8_t      laser_on_align;
	ca_uint8_t      logical_onu_id[24];
	ca_uint8_t      password[12];
	ca_uint8_t      version[14];
} xgpon_basic_info_t;

extern xgpon_basic_info_t g_xgpn_basic_info;

#define AAL_PON_SCFG_GET_DEFAULT_CHN(/*CODEGEN_IGNORE_TAG*/dev)   (g_xgpn_basic_info.default_channel)
#define AAL_PON_SCFG_GET_CPI(/*CODEGEN_IGNORE_TAG*/dev)           (g_xgpn_basic_info.cpi)
#define AAL_PON_SCFG_GET_PROTECT_CHN(/*CODEGEN_IGNORE_TAG*/dev)   (g_xgpn_basic_info.protect_channel)

/* the definition of crypto is the same as cortina api, port.h */
typedef enum {
    AAL_PON_CRYPTO_MODE_DISABLE     = 0, //disabled
    AAL_PON_CRYPTO_MODE_AES_CTR     = 1, // for 10G EPON, supported only on LLID
    AAL_PON_CRYPTO_MODE_AES_CFB     = 2, // for 1G EPON, supported only on LLID (DPoE)
    AAL_PON_CRYPTO_MODE_MACSEC            = 3, //supported on UNI and LLID
    AAL_PON_CRYPTO_MODE_SW_CTC_CHURNING = 4, //supported only on LLID
    AAL_PON_CRYPTO_MODE_HW_CTC_CHURNING = 5  //supported only on LLID
} aal_pon_crypto_mode_t;

ca_status_t aal_pon_scfg_load(/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id);
ca_status_t aal_pon_init(/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id, ca_uint32_t mode);
ca_uint32_t aal_pon_mac_mode_get(/*CODEGEN_IGNORE_TAG*/ca_device_id_t device_id);


//#define PON_DEBUG 1 // TODO: remove
#ifdef PON_DEBUG
    #define  PON_REG_DUMP(x) do { \
                    ca_uint32_t  reg_data = 0; \
                    reg_data = AAL_PON_REG_READ(x); \
                    printk(#x" = 0x%08x\n", reg_data); }while(0)
    #define PON_DBG(arg,...) printk("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)
#else
    #define PON_REG_DUMP(x)
    #define PON_DBG(arg,...)
#endif

typedef struct{
    ca_uint32_t sfc0;
    ca_uint32_t sfc1;
}aal_xgpn_glb_super_frame_match_val_t;

ca_status_t aal_epon_onu_mpcp_timer_match_cfg_set(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_IN ca_uint32_t                     mpcp_timer_match_cfg);

ca_status_t aal_epon_onu_mpcp_timer_match_cfg_get(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_IN ca_uint32_t                     *mpcp_timer_match_cfg);

ca_status_t aal_epon_onu_mpcp_timestamp_get(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_OUT ca_uint32_t                     *timestamp);

ca_status_t aal_xgpon_xgpn_glb_super_frame_match_val_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpn_glb_super_frame_match_val_t val);

ca_status_t aal_xgpon_xgpn_glb_super_frame_match_val_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpn_glb_super_frame_match_val_t *val);

typedef  union {
  struct {
    ca_uint32_t tod_counter          :  6 ; /* bits 5:0 */
    ca_uint32_t rsrvd1               :  1 ;
    ca_uint32_t tod_en               :  1 ; /* bits 7:7 */
    ca_uint32_t tod_uart             :  2 ; /* bits 9:8 */
    ca_uint32_t rsrvd2               : 22 ;
  } s ;
  ca_uint32_t   u32 ;
} aal_per_tod_cfg_mask_t;

typedef  union {
  struct {
    ca_uint32_t tod_bst_mem          : 32 ; /* bits 31:0 */
  } s ;
  ca_uint32_t    u32 ;
} aal_per_tod_mem_mask_t;


typedef struct{
    ca_uint32_t tod_counter;
    ca_uint32_t tod_en;
    ca_uint32_t tod_uart;
    ca_uint32_t tod_bst_mem[16];
}aal_per_tod_mem_cfg_t;

typedef union{
    aal_per_tod_cfg_mask_t per_tod_cfg_mask;
    aal_per_tod_mem_mask_t per_tod_mem_mask;
}aal_per_tod_mem_cfg_mask_t;

typedef union {
  struct {
    ca_uint32_t gen_1pps_tod_en      :  1 ; /* bits 0:0 */
    ca_uint32_t rsrvd1               : 31 ;
  } s ;
  ca_uint32_t    u32 ;
} aal_epon_onu_1pps_mask_t;

typedef union {
  struct {
    ca_uint32_t s_pps                : 32 ; /* bits 31:0 */
  } s ;
  ca_uint32_t     wrd ;
} aal_epon_onu_1pps_tod_s_pps_mask_t;


typedef union {
  struct {
    ca_uint32_t pps_width            : 16 ; /* bits 15:0 */
    ca_uint32_t rsrvd1               : 16 ;
  } s ;
  ca_uint32_t     wrd ;
} aal_epon_onu_1pps_tod_pps_wid_mask_t;

typedef volatile union {
  struct {
    ca_uint32_t s_tod                : 32 ; /* bits 31:0 */
  } s ;
  ca_uint32_t     wrd ;
} aal_epon_onu_1pps_tod_s_tod_mask_t;

typedef union{
    aal_epon_onu_1pps_mask_t onu_1pps_mask;
    aal_epon_onu_1pps_tod_s_pps_mask_t onu_1pps_tod_s_pps_mask;
    aal_epon_onu_1pps_tod_pps_wid_mask_t onu_1pps_tod_pps_wid_mask;
    aal_epon_onu_1pps_tod_s_tod_mask_t onu_1pps_tod_s_tod_mask;
}aal_epon_onu_1pps_tod_cfg_mask_t;

typedef struct{
    ca_uint32_t gen_1pps_tod_en;
    ca_uint32_t s_pps;
    ca_uint32_t pps_width;
    ca_uint32_t s_tod;
}aal_epon_onu_1pps_tod_cfg_t;

typedef struct {
    ca_uint8_t   cos;
    ca_uint16_t  ldpid;
    ca_uint16_t  lspid;
    ca_boolean_t fe_bypass;
    ca_uint8_t   hdr_type;
    ca_uint8_t   mcgid;
    ca_boolean_t drop_code;
    ca_uint8_t   rx_pkt_type;
    ca_boolean_t no_drop;

    ca_uint32_t mirror             ;
    ca_uint32_t mark               ;
    ca_uint32_t pol_en             ;
    ca_uint32_t pol_id             ;
    ca_uint32_t pol_grp_id         ;
    ca_uint32_t deepq              ;
    ca_uint32_t cpu_flag           ;
} aal_pon_pdc_map_mem_data_t;


ca_status_t aal_per_tod_mem_cfg_set(
    ca_device_id_t device_id,
    aal_per_tod_mem_cfg_mask_t mask,
    aal_per_tod_mem_cfg_t cfg);

ca_status_t aal_per_tod_mem_cfg_get(
    ca_device_id_t device_id,
    aal_per_tod_mem_cfg_t *cfg);

ca_status_t aal_epon_onu_1pps_tod_cfg_set(
     ca_device_id_t device_id,
     aal_epon_onu_1pps_tod_cfg_mask_t mask,
     aal_epon_onu_1pps_tod_cfg_t config);

ca_status_t aal_epon_onu_1pps_tod_cfg_get(
     ca_device_id_t device_id,
     aal_epon_onu_1pps_tod_cfg_t *config);

ca_status_t aal_xgpn_dsfp_sfc_get(
    ca_device_id_t device_id,
    ca_uint32_t *sfc0,
    ca_uint32_t *sfc1);

ca_status_t aal_pon_mac_enable_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_boolean_t        tx_en,
    CA_IN ca_boolean_t        rx_en);

ca_status_t aal_pon_mac_enable_get(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_boolean_t     *tx_en,
    CA_OUT ca_boolean_t     *rx_en);

ca_status_t aal_pon_mac_max_frame_size_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_uint32_t        tx_size,
    CA_IN ca_uint32_t        rx_size,
    CA_IN ca_boolean_t       mtu_tag_en);

ca_status_t aal_pon_mac_max_frame_size_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_uint32_t         *tx_size,
    CA_OUT ca_uint32_t        *rx_size,
    CA_OUT ca_uint32_t        *mtu_tag_en);

ca_status_t aal_pon_mac_ipg_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_uint32_t         ipg);

ca_status_t aal_pon_mac_ipg_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_uint32_t         *ipg);

ca_status_t aal_pon_mac_loopback_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_boolean_t        rx2tx,
    CA_IN ca_boolean_t        tx2rx);

ca_status_t aal_pon_mac_loopback_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_boolean_t       *rx2tx,
    CA_OUT ca_boolean_t       *tx2rx);

ca_status_t aal_pon_pdc_map_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_OUT aal_pon_pdc_map_mem_data_t *cfg);
ca_status_t aal_pon_puc_flush(CA_IN ca_device_id_t device_id);

#endif /* CONFIG_NE_PON */

#endif
