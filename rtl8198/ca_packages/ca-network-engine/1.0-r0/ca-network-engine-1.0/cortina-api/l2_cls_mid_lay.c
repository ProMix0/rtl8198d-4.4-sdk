#include "aal_port.h"
#include "aal_l2_cls.h"
#include "classifier.h"
#include "osal_common.h"
#include "port.h"
#include "aal_l2_specpkt.h"
#include "aal_l2_te.h"
#include "vlan.h"
#include "aal_common.h"
#include "rate.h"
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
#include "aal_pon.h"
#endif
#include "ca-scfg/scfg.h"

extern void cls_printf(const ca_int8 *fmt, ...);
extern ca_uint32_t   mirror_vlan;
static ca_boolean_t  __g_rule_is_use_for_mirror = FALSE;
extern ca_uint32_t l2_customer_info;

#define __CLS_ENTRY_START               0
#define __CLS_KEY_HANDLE_NUM            32
#define __CLS_KEY_HANDLE_ID             127

#ifndef __CLS_PORT_NUM
#define __CLS_PORT_NUM                  18
#endif

#define __CLS_IPV4_ETH_TYPE     0x0800
#define __CLS_IPV6_ETH_TYPE     0x86DD
#define __CLS_OAM_ETH_TYPE      0x8809
#define __CLS_MPCP_ETH_TYPE     0x8808
#define __CLS_CFM_ETH_TYPE      0x8902

#define __CLS_IPV4_BYTE_MASK    0xFF

#define __CLS_TCP_FLAGS_BIT_NS                  (1<<8)
#define __CLS_TCP_FLAGS_BIT_CWR                 (1<<7)
#define __CLS_TCP_FLAGS_BIT_ECE                 (1<<6)
#define __CLS_TCP_FLAGS_BIT_URG                 (1<<5)
#define __CLS_TCP_FLAGS_BIT_ACK                 (1<<4)
#define __CLS_TCP_FLAGS_BIT_PSH                 (1<<3)
#define __CLS_TCP_FLAGS_BIT_RST                 (1<<2)
#define __CLS_TCP_FLAGS_BIT_SYN                 (1<<1)
#define __CLS_TCP_FLAGS_BIT_FIN                 (1<<0)

ca_boolean_t __g_l2_cls_debug = FALSE;
static ca_boolean_t __g_l2_cls_is_saturn = FALSE;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_VENUS)
ca_uint8_t  __g_puc_voq_mode = 0;
static ca_uint8_t  __g_pon_mac_mode = 0;
static ca_uint8_t __g_temp_ldpid = 0x30;
#if defined (CONFIG_ARCH_CORTINA_VENUS)
#define __CLS_PON_MAX_FLOW_ID           255
#else
#define __CLS_PON_MAX_FLOW_ID           127
#endif
#endif

#define  __CLS_CPU256_PORT_START  0x20
#define  __CLS_CPU256_PORT_END    0x3F
#define  __CLS_CPU256_PORT_QUE_NUM  8

typedef enum {
    __CLS_KEY_MASK_ETYPE_IPVER                     = 0,
    __CLS_KEY_L2_MASK_SRC_MAC                      = 1,
    __CLS_KEY_L2_MASK_DST_MAC                      = 2,
    __CLS_KEY_L2_MASK_PRI                          = 3,
    __CLS_KEY_L2_MASK_SUB_TYPE                      = 4,
    __CLS_KEY_L3_MASK_IP_DSCP_MASK                 = 5,
    __CLS_KEY_L2_MASK_VTAG_CNT                     = 6,
    __CLS_KEY_L2_MASK_VID                          = 7,
    __CLS_KEY_L2_MASK_VID_RANGE                    = 8,
    __CLS_KEY_L2_MASK_MAC                          = 9,
    __CLS_KEY_L3_MASK_IP_PROTOCOL                  = 10,
    __CLS_KEY_L3_MASK_IP_DSCP                      = 11,
    __CLS_KEY_L3_MASK_IP_FRAGMENT                  = 12,
    __CLS_KEY_L3_MASK_IP_OPTION                    = 13,
    __CLS_KEY_L3_MASK_FLOW_LABLE                   = 14,
    __CLS_KEY_L3_MASK_ICMP_TYPE                    = 15,
    __CLS_KEY_L3_MASK_IGMP_TYPE                    = 16,
    __CLS_KEY_L3_MASK_SRC_IP                       = 17,
    __CLS_KEY_L3_MASK_SRC_IP_PREFIX                = 18,
    __CLS_KEY_L3_MASK_DST_IP                       = 19,
    __CLS_KEY_L3_MASK_DST_IP_PREFIX                = 20,
    __CLS_KEY_L4_MASK_SRC_PORT                     = 21,
    __CLS_KEY_L4_MASK_SRC_PORT_BMP                 = 22,
    __CLS_KEY_L4_MASK_DST_PORT                     = 23,
    __CLS_KEY_L4_MASK_DST_PORT_BMP                 = 24,
    __CLS_KEY_L4_MASK_TCP_FLAGS                    = 25,
    __CLS_KEY_EXT_MASK_SPCL_PKT                    = 26,
    __CLS_KEY_MASK_IPV4                            = 27,
    __CLS_KEY_MASK_IPV6                            = 28,
    __CLS_KEY_MASK_SRC_PORT                        = 29,
    __CLS_KEY_L3_MASK_IP                           = 30,
    __CLS_KEY_L3_MASK_IP_LENGTH                    = 31,

}__cls_key_mask_t;


#define __L2R_KEY_FIELDS_MASK               ((0x1UL<<__CLS_KEY_MASK_ETYPE_IPVER)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_SRC_MAC)                     \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_DST_MAC)                     \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_PRI)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VTAG_CNT)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID_RANGE)                   \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_MASK_IPV4)                           \
                                            |(0x1UL<<__CLS_KEY_MASK_SRC_PORT))

#define __L3R_KEY_FIELDS_MASK               ((0x1UL<<__CLS_KEY_MASK_ETYPE_IPVER)                    \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_SRC_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_SRC_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_OPTION)                   \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_PROTOCOL)                 \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP)                     \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_FRAGMENT)                 \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_PRI)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VTAG_CNT)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID_RANGE)                   \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID)                         \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_TCP_FLAGS)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_IPV4)                           \
                                            |(0x1UL<<__CLS_KEY_MASK_SRC_PORT))

#define __CMR_KEY_FIELDS_MASK               ((0x1UL<<__CLS_KEY_MASK_ETYPE_IPVER)                    \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP)                          \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_OPTION)                   \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_PROTOCOL)                 \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP)                     \
                                             |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP_MASK)                     \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_FRAGMENT)                 \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_MAC)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_PRI)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VTAG_CNT)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID)                         \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_TCP_FLAGS)                   \
                                            |(0x1UL<<__CLS_KEY_EXT_MASK_SPCL_PKT)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_IPV4)                           \
                                            |(0x1UL<<__CLS_KEY_MASK_SRC_PORT))

#define __IPV4_KEY_FIELDS_MASK              ((0x1UL<<__CLS_KEY_MASK_ETYPE_IPVER)                    \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP)                          \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_SRC_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_SRC_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_OPTION)                   \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_PROTOCOL)                 \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP)                     \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP_MASK)             \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_SUB_TYPE)                     \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_ICMP_TYPE)                   \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IGMP_TYPE)                   \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_FRAGMENT)                 \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_MAC)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_SRC_MAC)                     \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_DST_MAC)                     \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_PRI)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VTAG_CNT)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID_RANGE)                   \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID)                         \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT_BMP)                \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT_BMP)                \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_TCP_FLAGS)                   \
                                            |(0x1UL<<__CLS_KEY_EXT_MASK_SPCL_PKT)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_IPV4)                           \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_LENGTH)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_SRC_PORT))

#define __IPV6_SHORT_KEY_FIELDS_MASK        ((0x1UL<<__CLS_KEY_L3_MASK_SRC_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_SRC_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP)                      \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_DST_IP_PREFIX)               \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_FLOW_LABLE)                  \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_PROTOCOL)                 \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP)                     \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_DSCP_MASK)             \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_FRAGMENT)                 \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_PRI)                         \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VTAG_CNT)                    \
                                            |(0x1UL<<__CLS_KEY_L2_MASK_VID)                         \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_SRC_PORT_BMP)                \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT)                    \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_DST_PORT_BMP)                \
                                            |(0x1UL<<__CLS_KEY_L4_MASK_TCP_FLAGS)                   \
                                            |(0x1UL<<__CLS_KEY_EXT_MASK_SPCL_PKT)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_IPV6)                           \
                                            |(0x1UL<<__CLS_KEY_L3_MASK_IP_LENGTH)                   \
                                            |(0x1UL<<__CLS_KEY_MASK_SRC_PORT))

#define __CLS_SETBIT(value, bit_index)     (value |= ((ca_uint64_t)1<<(bit_index)))
#define __CLS_CLRBIT(value, bit_index)     (value &= ~((ca_uint64_t)1<<(bit_index)))
#define __CLS_COMPBIT(value, bit_index)    (value & ((ca_uint64_t)1<< (bit_index)))


#define __CLS_DOT_1P_CHECK(dot1p)  do{\
                                    if(dot1p>7){\
                                        cls_printf("err dot1p = %d\n", dot1p); \
                                        return CA_E_PARAM;}\
                                    }while(0)

#define __CLS_VLAN_ID_CHECK(vid)  do{\
                                    if(vid>4095){\
                                        cls_printf("err vlan id = %d\n", vid); \
                                        return CA_E_PARAM;}\
                                    }while(0)

#define __CLS_DSCP_CHECK(dscp)  do{\
                                    if(dscp>63){\
                                        cls_printf("err dscp = %d\n", dscp); \
                                        return CA_E_PARAM;}\
                                    }while(0)

#define __CLS_DSCP_MASK_CHECK(dscp_mask)  do{\
                                        if(dscp_mask>7){\
                                            printk("err dscp_mask = %d\n", dscp_mask); \
                                            return CA_E_PARAM;}\
                                        }while(0)
#define __CLS_FLOW_ID_CHECK(flow_id)  do{\
                                    if(flow_id>511){\
                                        cls_printf("err flow id = %d\n", flow_id); \
                                        return CA_E_PARAM;}\
                                    }while(0)

#define __CLS_COS_CHECK(cos)  do{\
                                    if(cos>7){\
                                        cls_printf("err cos = %d\n", cos); \
                                        return CA_E_PARAM;}\
                                    }while(0)

#define __CLS_PORT_CHECK_FOR_CPU256(port)  ((CA_PORT_TYPE_CPU == PORT_TYPE(port)) && (PORT_ID(port) >= __CLS_CPU256_PORT_START) && (PORT_ID(port) <= __CLS_CPU256_PORT_END)) 
                                    
ca_uint64_t __g_cls_entry_occupy[__CLS_PORT_NUM] = {0,0,0,0,0,0,0,0,
                                                            0,0,0,0,0,0,0,0,0,0};

ca_uint64_t __g_cls_egr_entry_occupy[__CLS_PORT_NUM] = {0,0,0,0,0,0,0,0,
                                                            0,0,0,0,0,0,0,0,0,0};


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
 ca_uint64_t __g_cls_entry_occupy_pon[__CLS_KEY_HANDLE_NUM] = {0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0};
static ca_uint64_t __g_cls_egr_entry_occupy_pon[__CLS_KEY_HANDLE_NUM] = {0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0,
                                                    0,0,0,0,0,0,0,0};
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_uint64_t __g_cls_saturn_entry_occupy[1] = {0};
//static ca_uint32_t __g_cls_saturn_egr_entry_occupy[1] = {0};
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define __HGU_SATURN_CLS_INDEX_START 0x20000
#if 0//defined(CONFIG_NE_L3COS)
extern ca_status_t ca_l3cos_rule_add(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p, ca_uint32_t cos);
extern ca_status_t ca_l3cos_rule_delete(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p);
#endif
#endif

#define __CLS_RULE_MAX          32
#define __CLS_INVALID_INDEX     0xFFFFFFFF




#define __CLS_IP_V4                       4
#define __CLS_IP_V6                       6

typedef struct __cls_rule{
    struct __cls_rule            *next;
    ca_uint32_t                  sw_idx;
    __cls_hw_key_type_t          key_type;
    ca_uint32_t                  pri;
    ca_uint32_t                  key_handle;
    ca_uint32_t                  port;
    ca_uint32_t                  dest_port;
    ca_uint32_t                  src_port_valid;
    ca_uint32_t                  key_handle_valid;
    ca_uint32_t                  dest_port_valid; 
#if 0//defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint16_t                  outer_vid;
    ca_uint16_t                  outer_8021p;
#endif
}__cls_rule_t;


typedef struct {
    ca_uint32_t          counter;
    __cls_rule_t        *next;
}__l2_cls_rule_header_t;

typedef struct {
    ca_uint8_t   cls_entry_start;
    ca_uint8_t   cls_entry_length;
}__l2_cls_entry_t;

static  __l2_cls_rule_header_t     __l2_cls_rule_head;


#if defined(CONFIG_ARCH_CORTINA_G3HGU)
#define CLS_IDX_CNT_G3HGU  48
#endif
#define __CLS_PORT_TYPE_FOR_SATURN  ((TRUE == __g_l2_cls_is_saturn))


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
extern ca_boolean_t  __eth_port3_active_evb_check ( void);
#endif




static ca_status_t __l2_cls_entry_get(ca_uint32_t port, ca_boolean_t is_egr, __l2_cls_entry_t *entry)
{
    aal_ilpb_cfg_t ilpb_cfg;
    aal_elpb_cfg_t elpb_cfg;
    ca_status_t ret = CA_E_OK;
    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));
    memset(&elpb_cfg, 0, sizeof(aal_elpb_cfg_t));
    
    if(port >= AAL_LPORT_LLID_GEM_INDEX_0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(is_egr == FALSE){ 
            ret = aal_port_ilpb_cfg_get(1,port,&ilpb_cfg);
        }else{
            ret = aal_port_elpb_cfg_get(1,port,&elpb_cfg);
        }
#else
        if(is_egr == FALSE){ 
            ret = aal_port_ilpb_cfg_get(0,port,&ilpb_cfg);
        }else{
            ret = aal_port_elpb_cfg_get(0,port,&elpb_cfg);
        }
#endif
    }else{
        if(is_egr == FALSE){ 
            ret = aal_port_ilpb_cfg_get(0,port,&ilpb_cfg);
        }else{
            ret = aal_port_elpb_cfg_get(0,port,&elpb_cfg);
        }
    }
    if(CA_E_OK != ret){
        return ret;
    }
    if(is_egr == FALSE){
        entry->cls_entry_start = ilpb_cfg.cls_start;
        entry->cls_entry_length = ilpb_cfg.cls_length;
    }else{    
        entry->cls_entry_start = elpb_cfg.egr_cls_start;
        entry->cls_entry_length = elpb_cfg.egr_cls_length;
    }
    return ret;
}

static ca_boolean_t __l2_cls_is_use_vlan_fib(ca_classifier_key_mask_t *key_mask, ca_classifier_key_t *key, ca_classifier_action_t *action)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if((action->forward == CA_CLASSIFIER_FORWARD_PORT)&&(CA_PORT_IS_PON(action->dest.port)&&((action->options.action_handle.flow_id != 0xFFFF)&&(action->options.masks.action_handle == 1))))
        /*||((key_mask->src_port)&&CA_PORT_IS_PON(key->src_port) &&(key_mask->key_handle)&&(key->key_handle.flow_id != 0xFFFF))*/{
        __g_l2_cls_is_saturn = TRUE;
        key_mask->src_port = 0;
        key_mask->dest_port= 0;
    }else if((key_mask->src_port)&&CA_PORT_IS_PON(key->src_port) &&(key_mask->key_handle)&&(key->key_handle.flow_id != 0xFFFF)){
        __g_l2_cls_is_saturn = TRUE;
    }else{
        __g_l2_cls_is_saturn = FALSE;
    }
#endif

   if((key_mask->src_port == 0)&&(key_mask->dest_port == 0)&&(key_mask->l2 == 1)&&(key_mask->ip == 0)&&
             (key_mask->l4 == 0)&&(key_mask->ext == 0) &&(key_mask->key_handle == 0)&&
             (key_mask->l2_mask.vlan_otag == 1)&&(key_mask->l2_mask.ethertype == 0)
             &&(key_mask->l2_mask.subtype == 0) &&(key_mask->l2_mask.mac_sa == 0)
             &&(key_mask->l2_mask.mac_da== 0)&&(key_mask->l2_mask.vlan_itag == 0)
             &&(key_mask->l2_mask.vlan_count == 0) &&(key_mask->l2_mask.is_length== 0)
             &&(key_mask->l2_mask.is_multicast== 0) &&(key_mask->l2_mask.cfm_opcode == 0)
             &&(key_mask->l2_mask.vlan_otag_mask.vid == 1) &&(key_mask->l2_mask.vlan_otag_mask.tpid == 0)
             &&(key_mask->l2_mask.vlan_otag_mask.dei == 0)&&(key_mask->l2_mask.vlan_otag_mask.pri == 0)           
             /*&&(memcmp(&action->options.masks,&act_ption_mask,sizeof(ca_classifier_action_option_mask_t)) == 0)
             */
             ){
             return TRUE;
    }
    return FALSE;
}

static ca_status_t __l2_cls_vlan_fib_get(ca_device_id_t device_id, 
    ca_classifier_action_t *action,
    aal_l2_vlan_fib_mask_t *vlan_fib_mask,
    aal_l2_vlan_fib_t      *vlan_fib
    )
{
    CA_ASSERT_RET(action, CA_E_PARAM);
    CA_ASSERT_RET(vlan_fib_mask, CA_E_PARAM);
    CA_ASSERT_RET(vlan_fib, CA_E_PARAM);
    if(action->forward == CA_CLASSIFIER_FORWARD_DENY){
        vlan_fib_mask->s.permit = 1;
        vlan_fib->permit        = 0;
    }else{
        vlan_fib_mask->s.permit = 1;
        vlan_fib->permit        = 1;
    }
    if(action->forward == CA_CLASSIFIER_FORWARD_PORT){
        vlan_fib_mask->s.dpid_field_vld = 1;
        vlan_fib->dpid_field_vld        = 1;
        vlan_fib_mask->s.dpid           = 1;
        vlan_fib->dpid                  = action->dest.port;
    }
    if(action->options.masks.priority){
        vlan_fib_mask->s.cos_field_vld   =   1;      
        vlan_fib_mask->s.cos             =   1;
        vlan_fib->cos_field_vld             =    1;
        vlan_fib->cos                       =    action->options.priority;
    }
    if(action->options.masks.action_handle){
        vlan_fib_mask->s.flow_field_vld = 1;
        vlan_fib->flow_field_vld        = 1;
        vlan_fib_mask->s.flow_id        = 1;
        vlan_fib->flow_id               = (action->options.action_handle.flow_id&0xFF);    
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined (CONFIG_ARCH_CORTINA_G3HGU)
        if(CA_PORT_IS_PON(action->dest.port) && (action->forward == CA_CLASSIFIER_FORWARD_PORT)){                    
            vlan_fib_mask->s.dpid_field_vld = 1;
            vlan_fib->dpid_field_vld        = 1;
            vlan_fib_mask->s.dpid           = 1;
            vlan_fib->dpid                  = AAL_LPORT_LLID_GEM_INDEX_0 +((action->options.action_handle.flow_id>>11)&0x1F);
            vlan_fib_mask->s.cos_field_vld  =   1;      
            vlan_fib_mask->s.cos            =   1;
            vlan_fib->cos_field_vld         =   1;
            vlan_fib->cos                   = (action->options.action_handle.flow_id>>8)&7;
        }
#endif
    }
    if(action->options.masks.dscp){
        vlan_fib_mask->s.dscp_field_vld = 1;
        vlan_fib->dscp_field_vld        = 1;
        vlan_fib_mask->s.dscp           = 1;
        vlan_fib->dscp                  = action->options.dscp;
    }
    if(action->options.masks.outer_vlan_act){
        vlan_fib_mask->s.top_vlan_cmd_field_vld = 1;
        vlan_fib->top_vlan_cmd_field_vld        = 1;
        vlan_fib_mask->s.top_vlan_cmd           = 1;
        vlan_fib->top_vlan_cmd                  = action->options.outer_vlan_act;
        vlan_fib_mask->s.vlan_id                = 1;
        vlan_fib->vlan_id                       = action->options.outer_vid;
    }    
    if(action->options.masks.outer_dot1p){
        vlan_fib_mask->s.dot1p_field_vld =   1;      
        vlan_fib_mask->s.dot1p           =   1;
        vlan_fib->dot1p_field_vld        =   1;
        vlan_fib->dot1p                  =   action->options.outer_dot1p;
    }

    if(action->options.masks.sw_shaper_id){
        vlan_fib_mask->s.cos_field_vld      =   1;      
        vlan_fib_mask->s.cos                =   1;
        vlan_fib_mask->s.dpid               = 1;        
        vlan_fib_mask->s.dpid_field_vld     = 1;
        vlan_fib->cos_field_vld             =   1;
        vlan_fib->cos                       =   (action->options.sw_shaper_id & 0x07);
        vlan_fib->dpid_field_vld            = 1;
        vlan_fib->dpid                      = (action->options.sw_shaper_id >> 3)&0x3f;
    }

    return CA_E_OK;
}
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
ca_uint32_t __hgu_saturn_ne_wan_svlan_fib_occupied[4] = {0,0,0,0};
ca_uint32_t __hgu_saturn_ne_wan_cvlan_fib_occupied[4] = {0,0,0,0};
ca_uint32_t __hgu_saturn_ne_lan_vlan_fib_occupied[4] = {0,0,0,0};

static ca_boolean_t __l2_cls_vlan_fib_id_find(ca_device_id_t device_id, ca_boolean_t is_wan, ca_boolean_t is_svlan, ca_uint16_t *fib_id)
{
    ca_uint16_t tmp_group_id = 0;
    ca_uint16_t tmp_index = 0;
    
    if(NULL == fib_id){
        return FALSE;
    }
    for(tmp_group_id = 0; tmp_group_id<4; tmp_group_id++){
        for(tmp_index = 0; tmp_index<32; tmp_index++){
            if(is_wan == TRUE){
                if(is_svlan){
                    if(((__hgu_saturn_ne_wan_svlan_fib_occupied[tmp_group_id] >> tmp_index)&1) == 0){
                        *fib_id = (tmp_group_id << 5)|tmp_index;
                        __hgu_saturn_ne_wan_svlan_fib_occupied[tmp_group_id] |= (1<<tmp_index);
                        return TRUE;
                    }
                }else{
                    if(((__hgu_saturn_ne_wan_cvlan_fib_occupied[tmp_group_id] >> tmp_index)&1) == 0){
                        *fib_id = (tmp_group_id << 5)|tmp_index;
                        __hgu_saturn_ne_wan_cvlan_fib_occupied[tmp_group_id] |= (1<<tmp_index);
                        return TRUE;
                    }
                }
            }else{
                if(((__hgu_saturn_ne_lan_vlan_fib_occupied[tmp_group_id] >> tmp_index)&1) == 0){
                    *fib_id = (tmp_group_id << 5)|tmp_index;
                    __hgu_saturn_ne_lan_vlan_fib_occupied[tmp_group_id] |= (1<<tmp_index);
                    cls_printf("*fib_id = %d\n",*fib_id);
                    return TRUE;
                }
            }
        }
    }
    cls_printf("vlan fib id not found\n");
    return FALSE;
}

static ca_boolean_t __l2_cls_vlan_fib_id_remove(ca_device_id_t device_id, ca_boolean_t is_wan, ca_boolean_t is_svlan, ca_uint16_t fib_id)
{
    ca_uint16_t tmp_group_id = ((fib_id>>5)&3);
    ca_uint16_t tmp_index = (fib_id&0x1F);
    
  
    if(is_wan == TRUE){
        if(is_svlan){           
                __hgu_saturn_ne_wan_svlan_fib_occupied[tmp_group_id] &= ~(1<<tmp_index);
        }else{
                __hgu_saturn_ne_wan_cvlan_fib_occupied[tmp_group_id] &= ~(1<<tmp_index);
        }
    }else{

            __hgu_saturn_ne_lan_vlan_fib_occupied[tmp_group_id] &= ~(1<<tmp_index);
        }
    return TRUE;
}


#endif
static ca_status_t __l2_cls_vlan_fib_add(ca_device_id_t device_id, ca_vlan_id_t vlan_id, ca_classifier_action_t *action)
{
    aal_l2_vlan_fib_t               vlan_fib;
    aal_l2_vlan_fib_map_t           vlan_fib_map;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_l2_vlan_fib_map_mask_t      vlan_fib_map_mask;
#endif
    aal_l2_vlan_fib_mask_t          vlan_fib_mask;
    ca_status_t                     ret = CA_E_OK;
    aal_ilpb_cfg_t                  ilpb_cfg;
    ca_uint32_t                     logic_port = 0;
    ca_uint16_t                     vlan_fib_id;

    __CLS_VLAN_ID_CHECK(vlan_id);    
    CA_ASSERT_RET(action, CA_E_PARAM);

    memset(&vlan_fib_map, 0, sizeof(aal_l2_vlan_fib_map_t));
    memset(&vlan_fib, 0, sizeof(aal_l2_vlan_fib_t));
    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));
    
    vlan_fib_mask.u32 = 0;
    ret = __l2_cls_vlan_fib_get(device_id,action,&vlan_fib_mask,&vlan_fib);    
    if(ret != CA_E_OK){
        goto end;
    }    
    cls_printf("vlan_fib_mask       = 0x%x\n",vlan_fib_mask.u32);
    cls_printf("vlan_fib.dpid       = 0x%x\n",vlan_fib.dpid);
    cls_printf("vlan_fib.flow_id    = 0x%x\n",vlan_fib.flow_id);
    cls_printf("vlan_fib.cos        = 0x%x\n",vlan_fib.cos);
    cls_printf("vlan_fib.dot1p      = 0x%x\n",vlan_fib.dot1p);
    cls_printf("vlan_fib.dscp       = 0x%x\n",vlan_fib.dscp);
    cls_printf("vlan_fib.vlan_id    = 0x%x\n",vlan_fib.vlan_id);
    cls_printf("vlan_fib.top_vlan_cmd= 0x%x\n",vlan_fib.top_vlan_cmd);
    
    if(action->forward == CA_CLASSIFIER_FORWARD_PORT){
        logic_port = PORT_ID(action->dest.port);
        ret = aal_port_ilpb_cfg_get(device_id,logic_port,&ilpb_cfg);
        if(ret != CA_E_OK){
            goto end;
        }
        if(ilpb_cfg.wan_ind == FALSE){
            if(ilpb_cfg.top_s_tpid_enable){
                ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_id,&vlan_fib_map);
                if(ret != CA_E_OK){
                    goto end;
                }
                
                if(vlan_fib_map.valid == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                    
                    if(__l2_cls_vlan_fib_id_find(device_id,TRUE,TRUE,&vlan_fib_id) == FALSE){
                        ret = CA_E_RESOURCE;
                        goto end;
                    }
                    vlan_fib_map_mask.u32 = 0;
                    vlan_fib_map_mask.s.valid = 1;
                    vlan_fib_map_mask.s.fib_id = 1;
                    vlan_fib_map.valid = TRUE;
                    vlan_fib_map.fib_id = vlan_fib_id;
                    ret = aal_l2_vlan_fib_map_set(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_id,vlan_fib_map_mask,&vlan_fib_map);
                    if(ret != CA_E_OK){
                        goto end;
                    }
                    vlan_fib.memship_bm = 0xC0;
                    vlan_fib_mask.s.memship_bm = 1;
                    vlan_fib.permit = TRUE;
                    vlan_fib_mask.s.permit = 1;
                    vlan_fib.stp_stat = 3;
                    vlan_fib_mask.s.stp_stat =1;
#endif                   
                } else{
                    vlan_fib_id = vlan_fib_map.fib_id;
                }

                
                ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_fib_id, vlan_fib_mask, &vlan_fib);
                if(ret != CA_E_OK){
                    goto end;
                }
            }else{
                ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_id,&vlan_fib_map);
                if(ret != CA_E_OK){
                    goto end;
                }
                if(vlan_fib_map.valid == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                    
                    if(__l2_cls_vlan_fib_id_find(device_id,TRUE,FALSE,&vlan_fib_id) == FALSE){
                        ret = CA_E_RESOURCE;
                        goto end;
                    }
                    vlan_fib_map_mask.u32 = 0;
                    vlan_fib_map_mask.s.valid = 1;
                    vlan_fib_map_mask.s.fib_id = 1;
                    vlan_fib_map.valid = TRUE;
                    vlan_fib_map.fib_id = vlan_fib_id;
                    ret = aal_l2_vlan_fib_map_set(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_id,vlan_fib_map_mask,&vlan_fib_map);
                    if(ret != CA_E_OK){
                        goto end;
                    }
                    vlan_fib.memship_bm = 0xC0;
                    vlan_fib_mask.s.memship_bm = 1;
                    vlan_fib.permit = TRUE;
                    vlan_fib_mask.s.permit = 1;
                    vlan_fib.stp_stat = 3;
                    vlan_fib_mask.s.stp_stat =1;
#endif                   
                } else{
                    vlan_fib_id = vlan_fib_map.fib_id;
                }              
                ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_fib_id, vlan_fib_mask, &vlan_fib);
                if(ret != CA_E_OK){
                    goto end;
                }
            }
        }else{
            ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_id,&vlan_fib_map);
            if(ret != CA_E_OK){
                goto end;
            } 
            if(vlan_fib_map.valid == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                    
                if(__l2_cls_vlan_fib_id_find(device_id,FALSE,FALSE,&vlan_fib_id) == FALSE){
                    ret = CA_E_RESOURCE;
                    goto end;
                }
                vlan_fib_map_mask.u32 = 0;
                vlan_fib_map_mask.s.valid = 1;
                vlan_fib_map_mask.s.fib_id = 1;
                vlan_fib_map.valid = TRUE;
                vlan_fib_map.fib_id = vlan_fib_id;
                ret = aal_l2_vlan_fib_map_set(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_id,vlan_fib_map_mask,&vlan_fib_map);
                if(ret != CA_E_OK){
                    goto end;
                }
                vlan_fib.memship_bm = 0xC0;
                vlan_fib_mask.s.memship_bm = 1;
                vlan_fib.permit = TRUE;
                vlan_fib_mask.s.permit = 1;
                vlan_fib.stp_stat = 3;
                vlan_fib_mask.s.stp_stat =1;
#endif                   
            } else{
                vlan_fib_id = vlan_fib_map.fib_id;
            }  

            ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_fib_id, vlan_fib_mask, &vlan_fib);
            if(ret != CA_E_OK){
                goto end;
            }
        }
    }else{    
        ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_id,&vlan_fib_map);
        if(ret != CA_E_OK){
            goto end;
        }
        if(vlan_fib_map.valid == 0){
            ret =  CA_E_NOT_FOUND;/*vlan not create*/
            goto end;
        }         
        ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
        if(ret != CA_E_OK){
            goto end;
        }
        ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_id,&vlan_fib_map);
        if(ret != CA_E_OK){
            goto end;
        }
        if(vlan_fib_map.valid == 0){
            ret =  CA_E_NOT_FOUND;/*vlan not create*/
            goto end;
        }         
        ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
        if(ret != CA_E_OK){
            goto end;
        }
        ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_id,&vlan_fib_map);
        if(ret != CA_E_OK){
            goto end;
        }
        if(vlan_fib_map.valid == 0){
            ret =  CA_E_NOT_FOUND;/*vlan not create*/
            goto end;
        }         
        ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
    }
 end:
    return ret;

}

ca_status_t __l2_cls_vlan_fib_del(ca_device_id_t device_id, ca_vlan_id_t vlan_id)
{
    aal_l2_vlan_fib_t               vlan_fib;
    aal_l2_vlan_fib_map_t           vlan_fib_map;
    aal_l2_vlan_fib_mask_t          vlan_fib_mask;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_l2_vlan_fib_map_mask_t      vlan_fib_map_mask;
#endif
    ca_status_t                     ret = CA_E_OK;

    __CLS_VLAN_ID_CHECK(vlan_id);

    memset(&vlan_fib_map, 0, sizeof(aal_l2_vlan_fib_map_t));
    memset(&vlan_fib, 0, sizeof(aal_l2_vlan_fib_t));

    ret = aal_l2_vlan_fib_map_get(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_id,&vlan_fib_map);
    if(ret != CA_E_OK){
        goto end;
    }
    /*vlan not create*/
    
    if(vlan_fib_map.valid == 0){
        return CA_E_OK;
    }
    vlan_fib_mask.u32 = 0;
    vlan_fib_mask.s.dpid = 1;
    vlan_fib_mask.s.dpid_field_vld = 1;
    vlan_fib.dpid_field_vld = 0;
    vlan_fib.dpid = 0;
    ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_SVLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
    if(ret != CA_E_OK){
        goto end;
    }

    ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_WAN_CVLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
    if(ret != CA_E_OK){
        goto end;
    }

    ret = aal_l2_vlan_action_cfg_set(device_id,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_fib_map.fib_id, vlan_fib_mask, &vlan_fib);
    if(ret != CA_E_OK){
        goto end;
    }
 #if defined(CONFIG_ARCH_CORTINA_G3HGU)
    __l2_cls_vlan_fib_id_remove(device_id,FALSE,FALSE,vlan_fib_map.fib_id);
    vlan_fib_map_mask.s.valid = 1;
    vlan_fib_map_mask.s.fib_id = 1;   
    memset(&vlan_fib_map, 0, sizeof(aal_l2_vlan_fib_map_t));
    ret = aal_l2_vlan_fib_map_set(1,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_id,vlan_fib_map_mask,&vlan_fib_map);
 
 #endif
 end:
    return ret;

}
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)

static ca_uint32_t __cls_llid_gem_sw_index_get(ca_uint32_t llid_gem_index,ca_uint8_t start,ca_uint8_t length, __cls_hw_key_type_t key_type)
{
    ca_uint32_t         cls_idx = 0;
    for(cls_idx = 0; cls_idx<(length*2); cls_idx++){
        if(!__CLS_COMPBIT(__g_cls_entry_occupy_pon[llid_gem_index],cls_idx)){
            if(key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                if((length*2 - cls_idx) < 4){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_entry_occupy_pon[llid_gem_index],(cls_idx+1)) ||
                        __CLS_COMPBIT(__g_cls_entry_occupy_pon[llid_gem_index],(cls_idx+2)) ||
                        __CLS_COMPBIT(__g_cls_entry_occupy_pon[llid_gem_index],(cls_idx+3)))){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                       
                            return (__HGU_SATURN_CLS_INDEX_START+start*2 + cls_idx);
#else
                            return (start*2 + cls_idx);
#endif
                    }
                }
            }else if((key_type == __CLS_HW_KEY_TYPE_IPV4R) ||(key_type == __CLS_HW_KEY_TYPE_IPV6)){
                if((length*2 - cls_idx) < 2){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_entry_occupy_pon[llid_gem_index],(cls_idx+1)))){
                        
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                        return (__HGU_SATURN_CLS_INDEX_START+start*2 + cls_idx);
#else
                        return (start*2 + cls_idx);
#endif
                    }
                }
            }else
            {
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                                        return (__HGU_SATURN_CLS_INDEX_START+start*2 + cls_idx);
#else
                                        return (start*2 + cls_idx);
#endif
            }
        }
    }
    return __CLS_INVALID_INDEX;

}
#endif
static ca_uint32_t __cls_port_sw_index_get(ca_uint32_t port_index,ca_uint8_t start,ca_uint8_t length, __cls_hw_key_type_t key_type)
{
    ca_uint32_t         cls_idx = 0;
    for(cls_idx = 0; cls_idx<(length*2); cls_idx++){
        if(!__CLS_COMPBIT(__g_cls_entry_occupy[port_index],cls_idx)){
            if(key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                if((length*2 - cls_idx) < 4){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_entry_occupy[port_index],(cls_idx+1)) ||
                        __CLS_COMPBIT(__g_cls_entry_occupy[port_index],(cls_idx+2)) ||
                        __CLS_COMPBIT(__g_cls_entry_occupy[port_index],(cls_idx+3)))){
                            return (start*2 + cls_idx);
                    }
                }
            }else if((key_type == __CLS_HW_KEY_TYPE_IPV4R) ||(key_type == __CLS_HW_KEY_TYPE_IPV6)){
                if((length*2 - cls_idx) < 2){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_entry_occupy[port_index],(cls_idx+1)))){
                        return (start*2 + cls_idx);
                    }
                }
            }else{
                return (start*2 + cls_idx);
            }
        }
    }
    return __CLS_INVALID_INDEX;
}


static ca_uint32_t __cls_sw_index_get(ca_uint32_t port_idx, __cls_hw_key_type_t key_type)
{
    __l2_cls_entry_t    cls_entry;
    ca_uint32_t         new_port_idx = 0;

    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));
    
    __l2_cls_entry_get(port_idx, FALSE, &cls_entry);  
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
    
    if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
        new_port_idx    = port_idx- AAL_LPORT_LLID_GEM_INDEX_0;
        return __cls_llid_gem_sw_index_get(new_port_idx,cls_entry.cls_entry_start,cls_entry.cls_entry_length,key_type);
    }
#endif
    if(port_idx >= 16){
        new_port_idx = port_idx - 8;
    }else{
        new_port_idx = port_idx;
    }
    return __cls_port_sw_index_get(new_port_idx,cls_entry.cls_entry_start,cls_entry.cls_entry_length,key_type);
}

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_uint32_t __cls_hgu_saturn_sw_index_get(__cls_hw_key_type_t key_type)
{
    ca_uint32_t         cls_idx = 0;
 
    for(cls_idx = 0; cls_idx<CLS_IDX_CNT_G3HGU; cls_idx++){
        if(!__CLS_COMPBIT(__g_cls_saturn_entry_occupy[0],cls_idx)){
            if(key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                if((CLS_IDX_CNT_G3HGU*2 - cls_idx) < 4){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_saturn_entry_occupy[0],(cls_idx+1)) ||
                        __CLS_COMPBIT(__g_cls_saturn_entry_occupy[0],(cls_idx+2)) ||
                        __CLS_COMPBIT(__g_cls_saturn_entry_occupy[0],(cls_idx+3)))){
                            return (cls_idx + __HGU_SATURN_CLS_INDEX_START);
                    }
                }
            }else if((key_type == __CLS_HW_KEY_TYPE_IPV4R) ||(key_type == __CLS_HW_KEY_TYPE_IPV6)){
            
                if((CLS_IDX_CNT_G3HGU*2 - cls_idx) < 2){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_saturn_entry_occupy[0],(cls_idx+1)))){
                        return (cls_idx + __HGU_SATURN_CLS_INDEX_START);
                    }
                }
            }else
            {
                return (cls_idx + __HGU_SATURN_CLS_INDEX_START);
            }
        }
    }
    return __CLS_INVALID_INDEX;
}
#endif

#define __EGR_CLS_INDEX_START 0x10000
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_uint32_t __cls_llid_gem_egr_sw_index_get(ca_uint32_t llid_gem_index,ca_uint8_t start,ca_uint8_t length, __cls_hw_key_type_t key_type)
{
    ca_uint32_t         cls_idx = 0;
    
    cls_printf("llid_gem_index = %d , key_type = %d start = %d, length = %d\n",llid_gem_index, key_type, start, length);
    for(cls_idx = 0; cls_idx<(length*2); cls_idx++){
        if(!__CLS_COMPBIT(__g_cls_egr_entry_occupy_pon[llid_gem_index],cls_idx)){
            if(key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                if((length*2 - cls_idx) < 4){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_egr_entry_occupy_pon[llid_gem_index],(cls_idx+1)) ||
                        __CLS_COMPBIT(__g_cls_egr_entry_occupy_pon[llid_gem_index],(cls_idx+2)) ||
                        __CLS_COMPBIT(__g_cls_egr_entry_occupy_pon[llid_gem_index],(cls_idx+3)))){
                            return (start*2 + cls_idx);
                    }
                }
            }else if((key_type == __CLS_HW_KEY_TYPE_IPV4R) ||(key_type == __CLS_HW_KEY_TYPE_IPV6)){
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_egr_entry_occupy_pon[llid_gem_index],(cls_idx+1)))){
                            return (start*2 + cls_idx);
                    }
                }
            }else
            {
                    return (start*2 + cls_idx);
            }
        }
    }
    return __CLS_INVALID_INDEX;

}
#endif
static ca_uint32_t __cls_port_egr_sw_index_get(ca_uint32_t port_index,ca_uint8_t start,ca_uint8_t length, __cls_hw_key_type_t key_type)
{
    ca_uint32_t         cls_idx = 0;

#ifdef  RTK_MEM_CORRUPTTION_FIX
	if(port_index >= __CLS_PORT_NUM){
		return __CLS_INVALID_INDEX;
	}
#endif
    for(cls_idx = 0; cls_idx<(length*2); cls_idx++){
        if(!__CLS_COMPBIT(__g_cls_egr_entry_occupy[port_index],cls_idx)){
            if(key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                if((length*2 - cls_idx) < 4){
                    return __CLS_INVALID_INDEX;
                }
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if(!(__CLS_COMPBIT(__g_cls_egr_entry_occupy[port_index],(cls_idx+1)) ||
                        __CLS_COMPBIT(__g_cls_egr_entry_occupy[port_index],(cls_idx+2)) ||
                        __CLS_COMPBIT(__g_cls_egr_entry_occupy[port_index],(cls_idx+3)))){
                            return (start*2 + cls_idx);
                    }
                }
            }else if((key_type == __CLS_HW_KEY_TYPE_IPV4R) ||(key_type == __CLS_HW_KEY_TYPE_IPV6)){
                if(!__CLS_COMPBIT(cls_idx, 0)){
                    if((length*2 - cls_idx) < 2){
                        return __CLS_INVALID_INDEX;
                    }
                    if(!(__CLS_COMPBIT(__g_cls_egr_entry_occupy[port_index],(cls_idx+1)))){
                        return (start*2 + cls_idx);
                    }
                }
            }else{
                return (start*2 + cls_idx);
            }
        }
    }
    return __CLS_INVALID_INDEX;
}


static ca_uint32_t __cls_egr_sw_index_get(ca_uint32_t port_idx, __cls_hw_key_type_t key_type)
{
    __l2_cls_entry_t    cls_entry;
    ca_uint32_t         new_port_idx = 0;

    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));
    cls_printf("port_idx = %d , key_type = %d \n",port_idx, key_type);
    __l2_cls_entry_get(port_idx, TRUE, &cls_entry);  
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)    
    cls_printf("start = %d , length = %d \n",cls_entry.cls_entry_start, cls_entry.cls_entry_length);
    if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
        new_port_idx    = port_idx- AAL_LPORT_LLID_GEM_INDEX_0;
        return __EGR_CLS_INDEX_START | __cls_llid_gem_egr_sw_index_get(new_port_idx,cls_entry.cls_entry_start,cls_entry.cls_entry_length,key_type);
    }
#endif    
    if(port_idx >= 16){
        new_port_idx = port_idx - 8;
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)
        new_port_idx = port_idx - 10;	//reserve 0x10/0x11 for 0x18/0x19, so only care port 0x12~0x19
#endif
    }else{
        new_port_idx = port_idx;
    }
    return __EGR_CLS_INDEX_START | __cls_port_egr_sw_index_get(new_port_idx,cls_entry.cls_entry_start,cls_entry.cls_entry_length,key_type);
}


static ca_status_t __cls_sw_table_add(__cls_rule_t *cfg)
{
    __cls_rule_t   *ptemp, *pre;
    ca_status_t ret = CA_E_OK;

    pre = NULL;

    if(NULL == cfg){
        return CA_E_PARAM;
    }

    ptemp = __l2_cls_rule_head.next;

    if (ptemp == NULL) {
        __l2_cls_rule_head.counter = 1;
        __l2_cls_rule_head.next = cfg;
        goto end;
    }
    while (ptemp) {
        pre = ptemp;
        ptemp = ptemp->next;
    }
    pre->next = cfg;
    __l2_cls_rule_head.counter++;

end:
    return ret;
}


static ca_status_t __cls_sw_table_del(ca_uint32_t sw_index)
{
    __cls_rule_t   *ptemp, *pre;
    ca_status_t     ret = CA_E_OK;
    ca_boolean      find_flag = FALSE;

    ptemp=(__l2_cls_rule_head.next);
    pre=NULL;

    while (ptemp) {
        if(sw_index == ptemp->sw_idx){
            find_flag= TRUE;
            break;
        }
        pre = ptemp;
        ptemp = ptemp->next;
    }
    if(find_flag == TRUE){
        if(pre){
            pre->next = ptemp->next;
        }else{
            __l2_cls_rule_head.next = ptemp->next;
        }

    }else{
        return CA_E_NOT_FOUND;
    }
    __l2_cls_rule_head.counter--;

    return ret;
}

static ca_boolean_t __cls_sw_table_search(ca_uint32_t sw_index, __cls_rule_t **rule)
{
    __cls_rule_t   *ptemp, *pre;

    cls_printf("sw_index = %x\n",sw_index);

    ptemp=(__l2_cls_rule_head.next);
    pre = NULL;

    while (ptemp) {
      /*  if(ptemp->sw_idx >= 256){
            return 0;
        }*/
        if(sw_index == ptemp->sw_idx){
            *rule = ptemp;
            return 1;
        }
        pre = ptemp;
        ptemp = ptemp->next;
    }
    return 0;
}


static ca_status_t __cls_ethertype_encode_find(
    ca_device_id_t                 device_id,
    ca_uint32_t                 ether_type,
    ca_uint32_t                 *etype_index
)
{
    aal_l2_specpkt_ethtype_encode_t ethertype_encode;
    aal_l2_specpkt_ethtype_encode_mask_t mask;
    ca_status_t                     ret = CA_E_OK;
    ca_uint32_t                      idx;
    ca_uint32_t                      zero_id = 0;
    ca_uint32_t                      zero_flag = 0;

    if(etype_index == NULL){
        return CA_E_PARAM;
    }

    memset(&ethertype_encode, 0, sizeof(aal_l2_specpkt_ethtype_encode_t));
    /*Check whether it exits*/
    for(idx = 0; idx < 32; idx++){
#if  defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(__CLS_PORT_TYPE_FOR_SATURN){        
              ret = aal_l2_specpkt_ethtype_encode_get(1, idx, &ethertype_encode);
        }else
#endif
        {
        ret = aal_l2_specpkt_ethtype_encode_get(device_id, idx, &ethertype_encode);
        }
        if(ret != CA_E_OK) {
            goto FUNC_EXIT;
        }

        if(ethertype_encode.ethertype == ether_type){
            *etype_index = ethertype_encode.encoded_value;
            goto  FUNC_EXIT;
        }

        if((ethertype_encode.ethertype == 0) &&(zero_flag == 0)){
            zero_id = idx;
            zero_flag = 1;
        }
    }

    if((idx >= 32)&&(zero_flag == 0)){
        ret = CA_E_NOT_FOUND;
        goto FUNC_EXIT;
    }
    if(zero_flag == 1){
        idx = zero_id;
    }

    memset(&ethertype_encode, 0, sizeof(aal_l2_specpkt_ethtype_encode_t));
    mask.u32 = 0;

    mask.s.ethertype = 1;
    mask.s.encoded_value = 1;
    ethertype_encode.ethertype = ether_type;
    ethertype_encode.encoded_value = idx;
    
#if  defined(CONFIG_ARCH_CORTINA_G3HGU)
    if(__CLS_PORT_TYPE_FOR_SATURN){
        ret = aal_l2_specpkt_ethtype_encode_set(1,idx,mask,&ethertype_encode);
    }else
#endif
    {
    ret = aal_l2_specpkt_ethtype_encode_set(device_id,idx,mask,&ethertype_encode);
    }
    if(ret != CA_E_OK) {
        goto FUNC_EXIT;
    }

    *etype_index = idx;

FUNC_EXIT:
    return ret;
}

#if 0
static ca_status_t __cls_ethertype_encode_release(
    ca_device_id_t                 device_id,
    ca_uint32_t                 ether_type
)
{
    aal_l2_specpkt_ethtype_encode_t ethertype_encode;
    aal_l2_specpkt_ethtype_encode_mask_t mask;
    ca_status_t                     ret = CA_E_OK;
    ca_uint32_t                      idx;

    memset(&ethertype_encode, 0, sizeof(aal_l2_specpkt_ethtype_encode_t));
    mask.u32 = 0;
    /*Check whether it exits*/
    for(idx = 0; idx < 32; idx++){
        ret = aal_l2_specpkt_ethtype_encode_get(device_id, idx, &ethertype_encode);
        if(ret != CA_E_OK) {
            goto FUNC_EXIT;
        }

        if(ethertype_encode.ethertype == ether_type){
            mask.s.encoded_value = 1;
            mask.s.ethertype = 1;
            ret = aal_l2_specpkt_ethtype_encode_set(device_id,idx,mask,&ethertype_encode);
            goto  FUNC_EXIT;
        }
    }

FUNC_EXIT:
    return ret;

}
#endif
#if 0

static ca_status_t __cls_ethertype_encode_clear(
    ca_device_id_t                 device_id
)
{
    aal_l2_specpkt_ethtype_encode_t ethertype_encode;
    aal_l2_specpkt_ethtype_encode_mask_t mask;
    ca_status_t                     ret = CA_E_OK;
    ca_uint32_t                      idx;

    memset(&ethertype_encode, 0, sizeof(aal_l2_specpkt_ethtype_encode_t));
    mask.u32 = 0;
    /*Check whether it exits*/
    for(idx = 0; idx < 32; idx++){
        mask.s.encoded_value = 1;
        mask.s.ethertype = 1;
        ret = aal_l2_specpkt_ethtype_encode_set(device_id,idx,mask,&ethertype_encode);
        if(ret != CA_E_OK) {
            goto FUNC_EXIT;
        }
    }

FUNC_EXIT:
    return ret;
}

#endif
static ca_uint32_t __cls_hw_key_mask_get(
    ca_classifier_key_t        *key,
    ca_classifier_key_mask_t   *key_mask
)
{
    ca_uint32_t  mask = 0;

    if((key_mask->src_port) || (key_mask->dest_port)){
        __CLS_SETBIT(mask, __CLS_KEY_MASK_SRC_PORT);
    }

    if(key_mask->l2){
        if(key_mask->l2_mask.ethertype){
            __CLS_SETBIT(mask, __CLS_KEY_MASK_ETYPE_IPVER);
        }
        if(key_mask->l2_mask.cfm_opcode || key_mask->l2_mask.subtype){
            __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_SUB_TYPE);
        }
        if((key_mask->l2_mask.mac_sa == 1) && (key_mask->l2_mask.mac_da == 0)) {
            if(memcmp(key->l2.mac_sa.mac_min, key->l2.mac_sa.mac_max,CA_ETH_ADDR_LEN) == 0){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_MAC);
            }else{
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_SRC_MAC);
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_DST_MAC);
            }
        }else if((key_mask->l2_mask.mac_da == 1) && (key_mask->l2_mask.mac_sa == 0)){
            if(memcmp(key->l2.mac_da.mac_min, key->l2.mac_da.mac_max,CA_ETH_ADDR_LEN) == 0){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_MAC);
            }else{
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_SRC_MAC);
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_DST_MAC);
            }
        }else if((key_mask->l2_mask.mac_sa == 1)&&(key_mask->l2_mask.mac_da == 1)){
            __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_SRC_MAC);
            __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_DST_MAC);
        }

        if(key_mask->l2_mask.vlan_otag){
            if(key_mask->l2_mask.vlan_itag_mask.pri){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID_RANGE);  
            }else if(key_mask->l2_mask.vlan_otag_mask.pri){
                if(key->l2.vlan_otag.vlan_min.pri != key->l2.vlan_otag.vlan_max.pri){
                    __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID_RANGE);  
                }else{
                    __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_PRI);
                }
            }
            if(key_mask->l2_mask.vlan_itag_mask.vid){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID_RANGE);  
            }else if(key_mask->l2_mask.vlan_otag_mask.vid){
                if(key->l2.vlan_otag.vlan_min.vid != key->l2.vlan_otag.vlan_max.vid){
                    __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID_RANGE);  
                }else{
                    __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID);
                }
            }            
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID_RANGE);  
            }else if(key_mask->l2_mask.vlan_otag_mask.dei){
                __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_VID);
            }      
        }
    }

    if(key_mask->ip){

        if(key_mask->ip_mask.ip_protocol){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_PROTOCOL);
        }

        if(key_mask->ip_mask.dscp){
            if(key->ip.dscp_mask == 0){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_DSCP);
            }else{
                __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_DSCP_MASK);
            }
        }

        if(key_mask->ip_mask.ecn){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_DSCP);
        }

          if(key_mask->ip_mask.fragment){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_FRAGMENT);
        }

        if(key->ip.ip_version == CA_IPV4 || key->ip.ip_version == __CLS_IP_V4){
            __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV4);
        }else{
            __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
        }

        if(key_mask->ip_mask.icmp_type){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_ICMP_TYPE);

            __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV6);
        }

        if(key_mask->ip_mask.igmp_type){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IGMP_TYPE);

            __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV6);
        }


        if(key_mask->ip_mask.ip_version &&(!(key_mask->ip_mask.igmp_type ||
            key_mask->ip_mask.icmp_type || key_mask->ip_mask.fragment ||key_mask->ip_mask.ecn
            ||key_mask->ip_mask.dscp || key_mask->ip_mask.ip_protocol))){
            __CLS_SETBIT(mask, __CLS_KEY_MASK_ETYPE_IPVER);
            __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV4);
            __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV6);
        }

        if((key_mask->ip_mask.ip_sa && ((key_mask->ip_mask.ip_sa_max == 0)&&
            (key_mask->ip_mask.ip_da == 0))) ||(key_mask->ip_mask.ip_da && ((key_mask->ip_mask.ip_da_max == 0)&&
            (key_mask->ip_mask.ip_sa == 0)))){
            if(key_mask->ip_mask.ip_sa){
                if(key->ip.ip_sa.afi == CA_IPV4){
                    if(key->ip.ip_sa.addr_len == 32){
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP);
                    }else{
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }else{
                    __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
                    __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);

                    if(key->ip.ip_sa.addr_len != 128){
                     __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }
            }

            if(key_mask->ip_mask.ip_da){
                if(key->ip.ip_da.afi == CA_IPV4){
                    if(key->ip.ip_da.addr_len == 32){
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP);
                    }else{
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }else{
                    __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
                    __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);
                    if(key->ip.ip_da.addr_len != 128){
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }
            }
        }
        else if(key_mask->ip_mask.ip_sa){
            if(key->ip.ip_sa.afi == CA_IPV4){
                __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_SRC_IP);
                if(key->ip.ip_sa.addr_len != 32){
                 __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                }
            }else{
                __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
                __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);

                if(key->ip.ip_sa.addr_len != 128){
                 __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                }
            }

            if(key_mask->ip_mask.ip_sa_max){
                __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_SRC_IP_PREFIX);
            }
            if(key_mask->ip_mask.ip_da){ /*IPSA && IPDA*/
                if(key->ip.ip_da.afi == CA_IPV4){
                    __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_DST_IP);
                    if(key->ip.ip_da.addr_len != 32){
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }else{
                    __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
                    __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);
                    if(key->ip.ip_da.addr_len != 128){
                        __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                    }
                }
                if(key_mask->ip_mask.ip_da_max){
                    __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_DST_IP_PREFIX);
                }
            }
        }else if(key_mask->ip_mask.ip_da){
            if(key->ip.ip_da.afi == CA_IPV4){
                __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_DST_IP);
                if(key->ip.ip_da.addr_len != 32){
                    __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                }
            }else{
                __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
                __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);
                if(key->ip.ip_da.addr_len != 128){
                    __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_LENGTH);
                }
            }
            if(key_mask->ip_mask.ip_da_max){
                __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_DST_IP_PREFIX);
            }
        }

        if(key_mask->ip_mask.have_options){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_IP_OPTION);
        }

        if(key_mask->ip_mask.flow_label){
            __CLS_SETBIT(mask, __CLS_KEY_L3_MASK_FLOW_LABLE);
            __CLS_SETBIT(mask, __CLS_KEY_MASK_IPV6);
            __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV4);
        }

     /*   if(key_mask->ip_mask.ip_da && key_mask->ip_mask.ip_sa){
            __CLS_SETBIT(mask, __CLS_KEY_MASK_ETYPE_IPVER);
        }*/

    }

    if(key_mask->l4) {
        if((key_mask->l4_mask.src_port) &&(key_mask->l4_mask.dst_port)){
            if((key->l4.src_port.min == key->l4.src_port.max)&&
                (key->l4.dst_port.min == key->l4.dst_port.max)){
                __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_SRC_PORT);
                __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_DST_PORT);
            }
            else{
                __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_SRC_PORT_BMP);
                __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_DST_PORT_BMP);
            }
        }else if(key_mask->l4_mask.dst_port){
                __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_DST_PORT);
        }else if(key_mask->l4_mask.src_port){
            __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_SRC_PORT);
        }

        if(key_mask->l4_mask.tcp_flags){
            __CLS_SETBIT(mask, __CLS_KEY_L4_MASK_TCP_FLAGS);
        }
    }
    if(key_mask->ext){
        __CLS_SETBIT(mask, __CLS_KEY_L2_MASK_SUB_TYPE);
        __CLS_CLRBIT(mask, __CLS_KEY_MASK_ETYPE_IPVER);
        __CLS_CLRBIT(mask, __CLS_KEY_MASK_IPV6);
    }

#if 0
    if(key_mask->ext){
        if(key_mask->ext_mask.pkt_type){
            __CLS_SETBIT(mask, __CLS_KEY_EXT_MASK_SPCL_PKT);
        }
    }
#endif

    printk("mask(0x%x)\n",mask);

    return mask;
}


static __cls_hw_key_type_t __cls_hw_key_map_mask2type(
    ca_uint32_t        hw_key_mask
)
{
    cls_printf("hw_key_mask(0x%x)\n",hw_key_mask);

    if(!(hw_key_mask & ~__L2R_KEY_FIELDS_MASK)){
        return __CLS_HW_KEY_TYPE_L2R;
    }

    if(!(hw_key_mask & ~__L3R_KEY_FIELDS_MASK)){
        return __CLS_HW_KEY_TYPE_L3R;
    }

    if(!(hw_key_mask & ~__CMR_KEY_FIELDS_MASK)){
        return __CLS_HW_KEY_TYPE_CMR;
    }

    if(!(hw_key_mask & ~__IPV4_KEY_FIELDS_MASK)){
        return __CLS_HW_KEY_TYPE_IPV4R;
    }

    if(!(hw_key_mask & ~__IPV6_SHORT_KEY_FIELDS_MASK)){
        return __CLS_HW_KEY_TYPE_IPV6;
    }

    cls_printf("No key type meets requirments, use 2 entries key!\n");

    return __CLS_HW_KEY_TYPE_IPV6_LONG;

}

static ca_status_t __cls_delete_2rule_key(ca_device_id_t device_id, ca_uint32_t entry_id, ca_uint32_t sub_id)
{
    //aal_l2_cls_cmr_key_mask_t  mask;
    //aal_l2_cls_cmr_key_t       cfg;
    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t                 ret = CA_E_OK;

    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    ret = aal_l2_cls_entry_status_get(device_id,entry_id,&entry_status);
    if(entry_status.entry_type != AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY){
        ret = CA_E_NOT_FOUND;
        goto end;
    }

    entry_status_mask.u32 = 0;

    if(sub_id == 0){
        entry_status_mask.s.rule_0 = 1;
        entry_status.rule_0 = 0;
    }else{
        entry_status_mask.s.rule_1 = 1;
        entry_status.rule_1 = 0;
    }
    if((entry_status.rule_0 == 0) &&(entry_status.rule_1 == 0) ){
        entry_status_mask.s.valid       = 1;
        entry_status.valid              = 0;
        entry_status_mask.s.entry_type  = 1;
        entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    }

    ret = aal_l2_cls_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);

 end:
    return  ret;

}

static ca_status_t __cls_egr_delete_2rule_key(ca_device_id_t device_id, ca_uint32_t entry_id, ca_uint32_t sub_id)
{
    //aal_l2_cls_cmr_key_mask_t  mask;
    //aal_l2_cls_cmr_key_t       cfg;
    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t                 ret = CA_E_OK;

    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    ret = aal_l2_cls_egr_entry_status_get(device_id,entry_id,&entry_status);
    if(entry_status.entry_type != AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY){
        ret = CA_E_NOT_FOUND;
        goto end;
    }

    entry_status_mask.u32 = 0;

    if(sub_id == 0){
        entry_status_mask.s.rule_0 = 1;
        entry_status.rule_0 = 0;
    }else{
        entry_status_mask.s.rule_1 = 1;
        entry_status.rule_1 = 0;
    }
    if((entry_status.rule_0 == 0) &&(entry_status.rule_1 == 0) ){
        entry_status_mask.s.valid       = 1;
        entry_status.valid              = 0;
        entry_status_mask.s.entry_type  = 1;
        entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    }

    ret = aal_l2_cls_egr_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);

 end:
    return  ret;

}


static ca_status_t __cls_delete_ipv4_key(ca_device_id_t device_id, ca_uint32_t entry_id)
{
    aal_l2_cls_ipv4_key_mask_t  mask;
    aal_l2_cls_ipv4_key_t       cfg;


    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t ret = CA_E_OK;



    memset(&cfg, 0, sizeof(aal_l2_cls_ipv4_key_t));
    mask.u64 = 0;
    mask.s.valid                  = 1;
    mask.s.is_ipv4_ipv6_key       = 1;
    mask.s.ipv4_ecn               = 1;
    mask.s.ipv4_ecn_vld           = 1;
    mask.s.mac_da                 = 1;
    mask.s.mac_sa                 = 1;
    mask.s.mac_field_ctrl         = 1;
    mask.s.top_vlan_id            = 1;
    mask.s.top_is_svlan           = 1;
    mask.s.inner_vlan_id          = 1;
    mask.s.inner_is_svlan         = 1;
    mask.s.vlan_field_ctrl        = 1;
    mask.s.top_dot1p              = 1;
    mask.s.inner_dot1p            = 1;
    mask.s.dot1p_ctrl             = 1;
    mask.s.vlan_number            = 1;
    mask.s.vlan_number_vld        = 1;
    mask.s.ethernet_type          = 1;
    mask.s.ethernet_type_vld      = 1;
    mask.s.ip_da                  = 1;
    mask.s.ip_da_mask             = 1;
    mask.s.ip_sa                  = 1;
    mask.s.ip_sa_mask             = 1;
    mask.s.ip_ctrl                = 1;
    mask.s.ip_pt                  = 1;
    mask.s.ip_pt_valid            = 1;
    mask.s.dscp                   = 1;
    mask.s.dscp_valid             = 1;
    mask.s.ip_option              = 1;
    mask.s.ip_option_valid        = 1;
    mask.s.ip_fragment            = 1;
    mask.s.ip_fragment_valid      = 1;
    mask.s.l4_dst_port_hi         = 1;
    mask.s.l4_dst_port_lo         = 1;
    mask.s.l4_dst_ctrl            = 1;
    mask.s.l4_src_port_hi         = 1;
    mask.s.l4_src_port_lo         = 1;
    mask.s.l4_src_ctrl            = 1;
    mask.s.l4_flag                = 1;
    mask.s.l4_flag_valid          = 1;
    mask.s.special_pkt_code       = 1;
    mask.s.special_pkt_code_valid = 1;
    mask.s.global_key             = 1;
    mask.s.global_key_mask        = 1;
    mask.s.l7_ctrl                = 1;
    mask.s.fields_is_or           = 1;
    mask.s.precedence             = 1;

  /*  cfg.mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    cfg.vlan_field_ctrl = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    cfg.dot1p_ctrl      = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    cfg.l4_dst_ctrl     = AAL_L2_CLS_L4_DST_PORT_FIELD_INVALID;
    cfg.l4_src_ctrl     = AAL_L2_CLS_L4_SRC_PORT_FIELD_INVALID;
    cfg.l7_ctrl         =   AAL_L2_CLS_L7_CTRL_MASK;
    cfg.global_key_mask = 0xF;
    */

    ret = aal_l2_cls_ipv4_key_set(device_id,entry_id,mask,&cfg);
    if(ret != CA_E_OK){
        return ret;
    }
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    entry_status_mask.s.valid       = 1;
    entry_status.valid              = 0;
    entry_status_mask.s.entry_type  = 1;
    entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    entry_status_mask.s.is_ipv6_rule = 1;
    entry_status_mask.s.rule_0 = 1;
    entry_status_mask.s.rule_1 = 1;

    return aal_l2_cls_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);
}

static ca_status_t __cls_egr_delete_ipv4_key(ca_device_id_t device_id, ca_uint32_t entry_id)
{
    aal_l2_cls_ipv4_key_mask_t  mask;
    aal_l2_cls_ipv4_key_t       cfg;


    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t ret = CA_E_OK;



    memset(&cfg, 0, sizeof(aal_l2_cls_ipv4_key_t));
    mask.u64 = 0;
    mask.s.valid                  = 1;
    mask.s.is_ipv4_ipv6_key       = 1;
    mask.s.ipv4_ecn               = 1;
    mask.s.ipv4_ecn_vld           = 1;
    mask.s.mac_da                 = 1;
    mask.s.mac_sa                 = 1;
    mask.s.mac_field_ctrl         = 1;
    mask.s.top_vlan_id            = 1;
    mask.s.top_is_svlan           = 1;
    mask.s.inner_vlan_id          = 1;
    mask.s.inner_is_svlan         = 1;
    mask.s.vlan_field_ctrl        = 1;
    mask.s.top_dot1p              = 1;
    mask.s.inner_dot1p            = 1;
    mask.s.dot1p_ctrl             = 1;
    mask.s.vlan_number            = 1;
    mask.s.vlan_number_vld        = 1;
    mask.s.ethernet_type          = 1;
    mask.s.ethernet_type_vld      = 1;
    mask.s.ip_da                  = 1;
    mask.s.ip_da_mask             = 1;
    mask.s.ip_sa                  = 1;
    mask.s.ip_sa_mask             = 1;
    mask.s.ip_ctrl                = 1;
    mask.s.ip_pt                  = 1;
    mask.s.ip_pt_valid            = 1;
    mask.s.dscp                   = 1;
    mask.s.dscp_valid             = 1;
    mask.s.ip_option              = 1;
    mask.s.ip_option_valid        = 1;
    mask.s.ip_fragment            = 1;
    mask.s.ip_fragment_valid      = 1;
    mask.s.l4_dst_port_hi         = 1;
    mask.s.l4_dst_port_lo         = 1;
    mask.s.l4_dst_ctrl            = 1;
    mask.s.l4_src_port_hi         = 1;
    mask.s.l4_src_port_lo         = 1;
    mask.s.l4_src_ctrl            = 1;
    mask.s.l4_flag                = 1;
    mask.s.l4_flag_valid          = 1;
    mask.s.special_pkt_code       = 1;
    mask.s.special_pkt_code_valid = 1;
    mask.s.global_key             = 1;
    mask.s.global_key_mask        = 1;
    mask.s.l7_ctrl                = 1;
    mask.s.fields_is_or           = 1;
    mask.s.precedence             = 1;

  /*  cfg.mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    cfg.vlan_field_ctrl = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    cfg.dot1p_ctrl      = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    cfg.l4_dst_ctrl     = AAL_L2_CLS_L4_DST_PORT_FIELD_INVALID;
    cfg.l4_src_ctrl     = AAL_L2_CLS_L4_SRC_PORT_FIELD_INVALID;
    cfg.l7_ctrl         =   AAL_L2_CLS_L7_CTRL_MASK;
    cfg.global_key_mask = 0xF;
    */

    ret = aal_l2_cls_egr_ipv4_key_set(device_id,entry_id,mask,&cfg);
    if(ret != CA_E_OK){
        return ret;
    }
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    entry_status_mask.s.valid       = 1;
    entry_status.valid              = 0;
    entry_status_mask.s.entry_type  = 1;
    entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    entry_status_mask.s.is_ipv6_rule = 1;
    entry_status_mask.s.rule_0 = 1;
    entry_status_mask.s.rule_1 = 1;

    return aal_l2_cls_egr_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);
}


static ca_status_t __cls_delete_ipv6_short_key(ca_device_id_t device_id, ca_uint32_t entry_id)
{
    aal_l2_cls_ipv6_short_key_mask_t  mask;
    aal_l2_cls_ipv6_short_key_t       cfg;
    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t ret = CA_E_OK;



    memset(&cfg, 0, sizeof(aal_l2_cls_ipv6_short_key_t));
    mask.u32 = 0;

    mask.s.valid                   = 1;
    mask.s.is_ipv4_ipv6_key        = 1;
    mask.s.ipv6_ecn                = 1;
    mask.s.ipv6_ecn_valid          = 1;
    mask.s.ipv6_da                 = 1;
    mask.s.ipv6_sa                 = 1;
    mask.s.ipv6_addr_mask          = 1;
    mask.s.ipv6_ctrl               = 1;
    mask.s.ipv6_pt                 = 1;
    mask.s.ipv6_pt_valid           = 1;
    mask.s.dscp                    = 1;
    mask.s.dscp_valid              = 1;
    mask.s.l4_flag                 = 1;
    mask.s.l4_flag_valid           = 1;
    mask.s.special_pkt_code        = 1;
    mask.s.special_pkt_code_valid  = 1;
    mask.s.top_vlan_id             = 1;
    mask.s.top_is_svlan            = 1;
    mask.s.top_vlan_valid          = 1;
    mask.s.top_dot1p               = 1;
    mask.s.top_dot1p_valid         = 1;
    mask.s.vlan_number             = 1;
    mask.s.vlan_number_vld         = 1;
    mask.s.l4_dst_port             = 1;
    mask.s.l4_src_port             = 1;
    mask.s.l4_ctrl                 = 1;
    mask.s.flow_lab                = 1;
    mask.s.flow_lab_valid          = 1;
    mask.s.fields_is_or            = 1;
    mask.s.precedence              = 1;

  /*  cfg.ipv6_ctrl = AAL_L2_CLS_IPV6_CTRL_MASK_IP_ADDR;
    cfg.l4_ctrl         =   AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;*/

    ret = aal_l2_cls_ipv6_short_key_set(device_id,entry_id,mask,&cfg);
    if(ret != CA_E_OK){
        return ret;
    }
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    entry_status_mask.s.valid       = 1;
    entry_status.valid              = 0;
    entry_status_mask.s.entry_type  = 1;
    entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    entry_status_mask.s.is_ipv6_rule = 1;
    entry_status_mask.s.rule_0 = 1;
    entry_status_mask.s.rule_1 = 1;

    return aal_l2_cls_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);

}

static ca_status_t __cls_egr_delete_ipv6_short_key(ca_device_id_t device_id, ca_uint32_t entry_id)
{
    aal_l2_cls_ipv6_short_key_mask_t  mask;
    aal_l2_cls_ipv6_short_key_t       cfg;
    aal_l2_cls_entry_status_t  entry_status;
    aal_l2_cls_entry_status_mask_t  entry_status_mask;
    ca_status_t ret = CA_E_OK;



    memset(&cfg, 0, sizeof(aal_l2_cls_ipv6_short_key_t));
    mask.u32 = 0;

    mask.s.valid                   = 1;
    mask.s.is_ipv4_ipv6_key        = 1;
    mask.s.ipv6_ecn                = 1;
    mask.s.ipv6_ecn_valid          = 1;
    mask.s.ipv6_da                 = 1;
    mask.s.ipv6_sa                 = 1;
    mask.s.ipv6_addr_mask          = 1;
    mask.s.ipv6_ctrl               = 1;
    mask.s.ipv6_pt                 = 1;
    mask.s.ipv6_pt_valid           = 1;
    mask.s.dscp                    = 1;
    mask.s.dscp_valid              = 1;
    mask.s.l4_flag                 = 1;
    mask.s.l4_flag_valid           = 1;
    mask.s.special_pkt_code        = 1;
    mask.s.special_pkt_code_valid  = 1;
    mask.s.top_vlan_id             = 1;
    mask.s.top_is_svlan            = 1;
    mask.s.top_vlan_valid          = 1;
    mask.s.top_dot1p               = 1;
    mask.s.top_dot1p_valid         = 1;
    mask.s.vlan_number             = 1;
    mask.s.vlan_number_vld         = 1;
    mask.s.l4_dst_port             = 1;
    mask.s.l4_src_port             = 1;
    mask.s.l4_ctrl                 = 1;
    mask.s.flow_lab                = 1;
    mask.s.flow_lab_valid          = 1;
    mask.s.fields_is_or            = 1;
    mask.s.precedence              = 1;

  /*  cfg.ipv6_ctrl = AAL_L2_CLS_IPV6_CTRL_MASK_IP_ADDR;
    cfg.l4_ctrl         =   AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;*/

    ret = aal_l2_cls_egr_ipv6_short_key_set(device_id,entry_id,mask,&cfg);
    if(ret != CA_E_OK){
        return ret;
    }
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));

    entry_status_mask.s.valid       = 1;
    entry_status.valid              = 0;
    entry_status_mask.s.entry_type  = 1;
    entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY;
    entry_status_mask.s.is_ipv6_rule = 1;
    entry_status_mask.s.rule_0 = 1;
    entry_status_mask.s.rule_1 = 1;

    return aal_l2_cls_egr_entry_status_set(device_id,entry_id,entry_status_mask,&entry_status);

}

static ca_status_t __cls_fib_del(ca_device_id_t device_id, ca_uint32_t entry_id, ca_uint32_t sub_id)
{
    aal_l2_cls_fib_mask_t cls_fib_mask;
    aal_l2_cls_fib_t      cls_fib;

    cls_fib_mask.u64 = 0;
    memset(&cls_fib, 0 ,sizeof(aal_l2_cls_fib_t));
    if(sub_id == 0){
        cls_fib_mask.s.permit_0          = 1;
        cls_fib_mask.s.permit_valid_0    = 1;
        cls_fib_mask.s.premarked_0       = 1;
        cls_fib_mask.s.top_sc_ind_0      = 1;
        cls_fib_mask.s.top_vlan_cmd_0    = 1;
        cls_fib_mask.s.top_cmd_valid_0   = 1;
        cls_fib_mask.s.top_vlanid_0      = 1;
        cls_fib_mask.s.inner_sc_ind_0    = 1;
        cls_fib_mask.s.inner_vlan_cmd_0  = 1;
        cls_fib_mask.s.inner_cmd_valid_0 = 1;
        cls_fib_mask.s.inner_vlanid_0    = 1;
        cls_fib_mask.s.dot1p_0           = 1;
        cls_fib_mask.s.dot1p_valid_0     = 1;
        cls_fib_mask.s.dscp_0            = 1;
        cls_fib_mask.s.dscp_valid_0      = 1;
        cls_fib_mask.s.cos_0             = 1;
        cls_fib_mask.s.cos_valid_0       = 1;
        cls_fib_mask.s.dp_value_0        = 1;
        cls_fib_mask.s.dp_valid_0        = 1;
        cls_fib_mask.s.flowid_0          = 1;
        cls_fib_mask.s.mark_ena_0        = 1;
        cls_fib_mask.s.dscp_mark_down_0  = 1;
        cls_fib_mask.s.spt_mode_0        = 1;
        cls_fib_mask.s.no_drop_0         = 1;
    }else{
        cls_fib_mask.s.permit_1          = 1;
        cls_fib_mask.s.permit_valid_1    = 1;
        cls_fib_mask.s.premarked_1       = 1;
        cls_fib_mask.s.top_sc_ind_1      = 1;
        cls_fib_mask.s.top_vlan_cmd_1    = 1;
        cls_fib_mask.s.top_cmd_valid_1   = 1;
        cls_fib_mask.s.top_vlanid_1      = 1;
        cls_fib_mask.s.inner_sc_ind_1    = 1;
        cls_fib_mask.s.inner_vlan_cmd_1  = 1;
        cls_fib_mask.s.inner_cmd_valid_1 = 1;
        cls_fib_mask.s.inner_vlanid_1    = 1;
        cls_fib_mask.s.dot1p_1           = 1;
        cls_fib_mask.s.dot1p_valid_1     = 1;
        cls_fib_mask.s.dscp_1            = 1;
        cls_fib_mask.s.dscp_valid_1      = 1;
        cls_fib_mask.s.cos_1             = 1;
        cls_fib_mask.s.cos_valid_1       = 1;
        cls_fib_mask.s.dp_value_1        = 1;
        cls_fib_mask.s.dp_valid_1        = 1;
        cls_fib_mask.s.flowid_1          = 1;
        cls_fib_mask.s.mark_ena_1        = 1;
        cls_fib_mask.s.dscp_mark_down_1  = 1;
        cls_fib_mask.s.spt_mode_1        = 1;
        cls_fib_mask.s.no_drop_1         = 1;
    }
    return aal_l2_cls_fib_set(device_id,entry_id,cls_fib_mask,&cls_fib);

}

static ca_status_t __cls_egr_fib_del(ca_device_id_t device_id, ca_uint32_t entry_id, ca_uint32_t sub_id)
{
    aal_l2_cls_fib_mask_t cls_fib_mask;
    aal_l2_cls_fib_t      cls_fib;

    cls_fib_mask.u64 = 0;
    memset(&cls_fib, 0 ,sizeof(aal_l2_cls_fib_t));
    if(sub_id == 0){
        cls_fib_mask.s.permit_0          = 1;
        cls_fib_mask.s.permit_valid_0    = 1;
        cls_fib_mask.s.premarked_0       = 1;
        cls_fib_mask.s.top_sc_ind_0      = 1;
        cls_fib_mask.s.top_vlan_cmd_0    = 1;
        cls_fib_mask.s.top_cmd_valid_0   = 1;
        cls_fib_mask.s.top_vlanid_0      = 1;
        cls_fib_mask.s.inner_sc_ind_0    = 1;
        cls_fib_mask.s.inner_vlan_cmd_0  = 1;
        cls_fib_mask.s.inner_cmd_valid_0 = 1;
        cls_fib_mask.s.inner_vlanid_0    = 1;
        cls_fib_mask.s.dot1p_0           = 1;
        cls_fib_mask.s.dot1p_valid_0     = 1;
        cls_fib_mask.s.dscp_0            = 1;
        cls_fib_mask.s.dscp_valid_0      = 1;
        cls_fib_mask.s.cos_0             = 1;
        cls_fib_mask.s.cos_valid_0       = 1;
        cls_fib_mask.s.dp_value_0        = 1;
        cls_fib_mask.s.dp_valid_0        = 1;
        cls_fib_mask.s.flowid_0          = 1;
        cls_fib_mask.s.mark_ena_0        = 1;
        cls_fib_mask.s.dscp_mark_down_0  = 1;
        cls_fib_mask.s.spt_mode_0        = 1;
        cls_fib_mask.s.no_drop_0         = 1;
    }else{
        cls_fib_mask.s.permit_1          = 1;
        cls_fib_mask.s.permit_valid_1    = 1;
        cls_fib_mask.s.premarked_1       = 1;
        cls_fib_mask.s.top_sc_ind_1      = 1;
        cls_fib_mask.s.top_vlan_cmd_1    = 1;
        cls_fib_mask.s.top_cmd_valid_1   = 1;
        cls_fib_mask.s.top_vlanid_1      = 1;
        cls_fib_mask.s.inner_sc_ind_1    = 1;
        cls_fib_mask.s.inner_vlan_cmd_1  = 1;
        cls_fib_mask.s.inner_cmd_valid_1 = 1;
        cls_fib_mask.s.inner_vlanid_1    = 1;
        cls_fib_mask.s.dot1p_1           = 1;
        cls_fib_mask.s.dot1p_valid_1     = 1;
        cls_fib_mask.s.dscp_1            = 1;
        cls_fib_mask.s.dscp_valid_1      = 1;
        cls_fib_mask.s.cos_1             = 1;
        cls_fib_mask.s.cos_valid_1       = 1;
        cls_fib_mask.s.dp_value_1        = 1;
        cls_fib_mask.s.dp_valid_1        = 1;
        cls_fib_mask.s.flowid_1          = 1;
        cls_fib_mask.s.mark_ena_1        = 1;
        cls_fib_mask.s.dscp_mark_down_1  = 1;
        cls_fib_mask.s.spt_mode_1        = 1;
        cls_fib_mask.s.no_drop_1         = 1;
    }
    return aal_l2_cls_egr_fib_set(device_id,entry_id,cls_fib_mask,&cls_fib);

}


ca_status_t __cls_mac_check(
    ca_mac_addr_t mac)
{
    /*ca_mac_addr_t   mac_zero = {0, 0, 0, 0, 0, 0};
    ca_mac_addr_t   mac_bc = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if((memcmp(mac, mac_zero, 6) == 0)
        ||((memcmp(mac, mac_bc, 6) == 0))) {
        return CA_E_PARAM;
    }*/
    return CA_E_OK;
}


ca_status_t __cls_key_parameter_check(
    ca_classifier_key_t          *key,
    ca_classifier_key_mask_t     *key_mask
)
{

    ca_status_t ret = CA_E_OK;
    ca_mac_addr_t   mac_zero = {0, 0, 0, 0, 0, 0};

    if((key_mask->src_port) && (key_mask->dest_port)){
        cls_printf("SRC PORT and DEST PORT could not be set both\n");
        return CA_E_NOT_SUPPORT;
    }else if(key_mask->src_port){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)||defined(CONFIG_ARCH_CORTINA_VENUS)
        if(!(CA_PORT_IS_CPU(key->src_port)||CA_PORT_IS_ETHERNET(key->src_port)||(CA_PORT_IS_PON(key->src_port)))){
            cls_printf("SRC PORT type invalid for Saturn\n");
            return CA_E_PARAM;
        }
        if(CA_PORT_IS_PON(key->src_port)){
            if(key_mask->key_handle == 0){
                cls_printf("for PON port, key_mask.key_handle must be 1\n");
                return CA_E_PARAM;
            }else if(((key->key_handle.gem_index & 0xFF) > __CLS_KEY_HANDLE_ID) && ((key->key_handle.gem_index & 0xFF) != 0xFF)){
                cls_printf("key->key_handle is too big %d\n", key->key_handle.gem_index);
                return CA_E_PARAM;
            }
        }
#else
        if(!(CA_PORT_IS_CPU(key->src_port)||CA_PORT_IS_ETHERNET(key->src_port))){
            cls_printf("SRC PORT type invalid\n");
            return CA_E_PARAM;
        }
#endif
    }else if(key_mask->dest_port){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)
#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)
        if(!(CA_PORT_IS_CPU_OR_VIRTUAL_PORT(key->dest_port /*support 0x18/0x19 egress rule*/)||CA_PORT_IS_ETHERNET(key->dest_port)||(CA_PORT_IS_PON(key->dest_port)))){
            cls_printf("DEST PORT type invalid for Saturn.\n");
            return CA_E_PARAM;
        }
#else
        if(!(CA_PORT_IS_CPU(key->dest_port)||CA_PORT_IS_ETHERNET(key->dest_port)||(CA_PORT_IS_PON(key->dest_port)))){
            cls_printf("DEST PORT type invalid for Saturn\n");
            return CA_E_PARAM;
        }
#endif /*defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RG_G3_L2FE_POL_OFFLOAD)*/        
        if(CA_PORT_IS_PON(key->dest_port)){
            if(key_mask->key_handle == 0){
                cls_printf("for destnstion PON port, key_mask.key_handle must be 1\n");
                return CA_E_PARAM;
            }
        }
#else
        if(!(CA_PORT_IS_CPU(key->dest_port)||CA_PORT_IS_ETHERNET(key->dest_port))){
            cls_printf("DEST PORT type invalid\n");
            return CA_E_PARAM;
        }
#endif
    }else if (!key_mask->src_intf && !key_mask->dst_intf && (!__CLS_PORT_TYPE_FOR_SATURN)){
        cls_printf("SRC PORT or SRC INTF or DST INTF has to be configured\n");
        return CA_E_PARAM;
    }

    if(key_mask->l2){
        if(key_mask->l2_mask.ethertype && (!key_mask->src_intf) && (!key_mask->dst_intf)){
            if((key_mask->ip) && (key_mask->ip_mask.ip_version)){
                cls_printf("ethertype and ipver are not supported at same time\n");
                return CA_E_NOT_SUPPORT;
            }
        }

        if(key_mask->l2_mask.subtype){
            if(key_mask->l2_mask.ethertype == 0){
                cls_printf("subtype not supported if key_mask->l2_mask.ethertype not set\n");
                return CA_E_PARAM;
           }
            if(!((key->l2.ethertype == __CLS_MPCP_ETH_TYPE) || (key->l2.ethertype == __CLS_OAM_ETH_TYPE ))){
                cls_printf("subtype not supported if not OAM or MPCP\n");
                return CA_E_PARAM;
            }
            if(key_mask->l2_mask.cfm_opcode){
                cls_printf("SubBytes and cfm_opcode cannot valid at same time");
            }
        }
        if(key_mask->l2_mask.mac_sa && key_mask->l2_mask.mac_da){
            if((memcmp(key->l2.mac_da.mac_min, key->l2.mac_da.mac_max, 6)) ||
                (memcmp(key->l2.mac_sa.mac_min, key->l2.mac_sa.mac_max, 6))){
                cls_printf("not support, MAC SA & MAC DA range at same time \n");
                return CA_E_NOT_SUPPORT;
            }
        }

        if(key_mask->l2_mask.mac_sa){
           ret = __cls_mac_check(key->l2.mac_sa.mac_min);
           if(ret != CA_E_OK){
               cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return ret;
           }
           ret = __cls_mac_check(key->l2.mac_sa.mac_max);
           if(ret != CA_E_OK){
               cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return ret;
           }
           if(key->l2.mac_sa.mac_max[0] < key->l2.mac_sa.mac_min[0]){
                cls_printf("mac_max[0] < mac_min[0]\n");
                return CA_E_PARAM;
           }else if(key->l2.mac_sa.mac_max[0] == key->l2.mac_sa.mac_min[0]){
                if(key->l2.mac_sa.mac_max[1] < key->l2.mac_sa.mac_min[1]){
                    cls_printf("mac_max[1] < mac_min[1]\n");
                    return CA_E_PARAM;
                }else if(key->l2.mac_sa.mac_max[1] == key->l2.mac_sa.mac_min[1]){
                    if(key->l2.mac_sa.mac_max[2] < key->l2.mac_sa.mac_min[2]){
                        cls_printf("mac_max[2] < mac_min[2]\n");
                        return CA_E_PARAM;
                    }else if(key->l2.mac_sa.mac_max[2] == key->l2.mac_sa.mac_min[2]){
                        if(key->l2.mac_sa.mac_max[3] < key->l2.mac_sa.mac_min[3]){
                            cls_printf("mac_max[3] < mac_min[3]\n");
                            return CA_E_PARAM;
                         }else if(key->l2.mac_sa.mac_max[3] == key->l2.mac_sa.mac_min[3]){
                            if(key->l2.mac_sa.mac_max[4] < key->l2.mac_sa.mac_min[4]){
                                cls_printf("mac_max[4] < mac_min[4]\n");
                                return CA_E_PARAM;
                            }else if(key->l2.mac_sa.mac_max[4] == key->l2.mac_sa.mac_min[4]){
                                if(key->l2.mac_sa.mac_max[5] < key->l2.mac_sa.mac_min[5]){
                                    cls_printf("mac_max[5] < mac_min[5]\n");
                                    return CA_E_PARAM;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(key_mask->l2_mask.mac_da){
#if 0            
            if(memcmp(key->l2.mac_da.mac_min, mac_zero, 6) == 0){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if(memcmp(key->l2.mac_da.mac_max, mac_zero, 6) == 0){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
#endif            
            if(key->l2.mac_da.mac_max[0] < key->l2.mac_da.mac_min[0]){
                cls_printf("mac_max[0] < mac_min[0]\n");
                return CA_E_PARAM;
            }else if(key->l2.mac_da.mac_max[0] == key->l2.mac_da.mac_min[0]){
                if(key->l2.mac_da.mac_max[1] < key->l2.mac_da.mac_min[1]){
                    cls_printf("mac_max[1] < mac_min[1]\n");
                    return CA_E_PARAM;
                }else if(key->l2.mac_da.mac_max[1] == key->l2.mac_da.mac_min[1]){
                    if(key->l2.mac_da.mac_max[2] < key->l2.mac_da.mac_min[2]){
                        cls_printf("mac_max[2] < mac_min[2]\n");
                        return CA_E_PARAM;
                    }else if(key->l2.mac_da.mac_max[2] == key->l2.mac_da.mac_min[2]){
                        if(key->l2.mac_da.mac_max[3] < key->l2.mac_da.mac_min[3]){
                            cls_printf("mac_max[3] < mac_min[3]\n");
                            return CA_E_PARAM;
                         }else if(key->l2.mac_da.mac_max[3] == key->l2.mac_da.mac_min[3]){
                            if(key->l2.mac_da.mac_max[4] < key->l2.mac_da.mac_min[4]){
                                cls_printf("mac_max[4] < mac_min[4]\n");
                                return CA_E_PARAM;
                            }else if(key->l2.mac_da.mac_max[4] == key->l2.mac_da.mac_min[4]){
                                if(key->l2.mac_da.mac_max[5] < key->l2.mac_da.mac_min[5]){
                                    cls_printf("mac_max[5] < mac_min[5]\n");
                                    return CA_E_PARAM;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(key_mask->l2_mask.vlan_count){
            if(key->l2.vlan_count > 3){
                return CA_E_NOT_SUPPORT;
            }
        }
        if(key_mask->l2_mask.vlan_otag && key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_otag_mask.pri && key_mask->l2_mask.vlan_itag_mask.pri){
                if(!((key->l2.vlan_otag.vlan_max.pri == key->l2.vlan_otag.vlan_min.pri)&&(
                    key->l2.vlan_itag.vlan_max.pri == key->l2.vlan_itag.vlan_min.pri))){
                    return CA_E_NOT_SUPPORT;
                }
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid && key_mask->l2_mask.vlan_itag_mask.vid){
                if(!((key->l2.vlan_otag.vlan_max.vid == key->l2.vlan_otag.vlan_min.vid)&&(
                    key->l2.vlan_itag.vlan_max.vid == key->l2.vlan_itag.vlan_min.vid))){
                    return CA_E_NOT_SUPPORT;
                }
            }
        }
        if(key_mask->l2_mask.vlan_otag){
            if(key_mask->l2_mask.vlan_otag_mask.pri){
                __CLS_DOT_1P_CHECK(key->l2.vlan_otag.vlan_min.pri);
                __CLS_DOT_1P_CHECK(key->l2.vlan_otag.vlan_max.pri);
                if(key->l2.vlan_otag.vlan_max.pri < key->l2.vlan_otag.vlan_min.pri){
                    cls_printf("%s %d max.pri %d is smaller than min.pri %d\n",__FILE__,__LINE__,
                        key->l2.vlan_otag.vlan_max.pri, key->l2.vlan_otag.vlan_min.pri);
                    return CA_E_PARAM;
                }
            }
/*
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                printk("vlan_otag_mask.dei not supported\n");
                return CA_E_NOT_SUPPORT;
            }
 */
            if(key_mask->l2_mask.vlan_otag_mask.tpid){
                cls_printf("vlan_otag_mask.tpid not supported\n");
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l2_mask.vlan_otag_mask.vid){
                __CLS_VLAN_ID_CHECK(key->l2.vlan_otag.vlan_min.vid);
                __CLS_VLAN_ID_CHECK(key->l2.vlan_otag.vlan_max.vid);
                
                if(key->l2.vlan_otag.vlan_max.vid < key->l2.vlan_otag.vlan_min.vid){
                    cls_printf("%s %d max.vid %d is smaller than min.vid %d\n",__FILE__,__LINE__,
                        key->l2.vlan_otag.vlan_max.vid, key->l2.vlan_otag.vlan_min.vid);
                    return CA_E_PARAM;
                }
            }
        }
        if(key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_itag_mask.pri){
                __CLS_DOT_1P_CHECK(key->l2.vlan_itag.vlan_min.pri);
                __CLS_DOT_1P_CHECK(key->l2.vlan_itag.vlan_max.pri);
                if(key->l2.vlan_itag.vlan_max.pri < key->l2.vlan_itag.vlan_min.pri){
                    cls_printf("%s %d max.pri %d is smaller than min.pri %d\n",__FILE__,__LINE__,
                        key->l2.vlan_itag.vlan_max.pri, key->l2.vlan_itag.vlan_min.pri);
                    return CA_E_PARAM;
                }
            }
/*
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                cls_printf("vlan_itag_mask.dei not supported\n");
                return CA_E_NOT_SUPPORT;
            }
 */          
            if(key_mask->l2_mask.vlan_itag_mask.tpid){
                cls_printf("vlan_itag_mask.tpid not supported\n");
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l2_mask.vlan_itag_mask.vid){
                __CLS_VLAN_ID_CHECK(key->l2.vlan_itag.vlan_min.vid);
                __CLS_VLAN_ID_CHECK(key->l2.vlan_itag.vlan_max.vid);
                
                if(key->l2.vlan_itag.vlan_max.vid < key->l2.vlan_itag.vlan_min.vid){
                    cls_printf("%s %d max.vid %d is smaller than min.vid %d\n",__FILE__,__LINE__,
                        key->l2.vlan_itag.vlan_max.vid, key->l2.vlan_itag.vlan_min.vid);
                    return CA_E_PARAM;
                }
            }
        }

        if(key_mask->l2_mask.is_multicast){
                cls_printf("is_multicast is not supported\n");
                return CA_E_NOT_SUPPORT;
        }

        if(key_mask->l2_mask.is_length){
            cls_printf("is_length is not supported\n");
            return CA_E_NOT_SUPPORT;
        }

        if(key_mask->l2_mask.cfm_opcode){
             if(key_mask->l2_mask.ethertype == 0){
                 cls_printf("cfm_opcode not supported if key_mask->l2_mask.ethertype not set\n");
                 return CA_E_PARAM;
            }
            if(key->l2.ethertype != __CLS_CFM_ETH_TYPE){
                 cls_printf("cfm_opcode not supported if ether type not 0c8902");
                return CA_E_NOT_SUPPORT;
            }
        }
    }

    if(key_mask->ip){
        if(key_mask->ip_mask.ip_valid){
            cls_printf("ip_valid not supported\n");
            return CA_E_NOT_SUPPORT;
        }

        if(key_mask->ip_mask.fragment){
            if(key->ip.fragment > 2){
                cls_printf("invalid fragment %u\n", key->ip.fragment);
                return CA_E_PARAM;
            }
        }

        if(key_mask->ip_mask.ip_sa){
            if(key->ip.ip_sa.afi > CA_IPV6){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if (key->ip.ip_sa.afi == CA_IPV4){
                if(key->ip.ip_sa.ip_addr.ipv4_addr == 0){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
                if(key->ip.ip_sa.addr_len > 32){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }else{
                if((key->ip.ip_sa.ip_addr.ipv6_addr[0] == 0)&&
                    (key->ip.ip_sa.ip_addr.ipv6_addr[1] == 0)&&
                    (key->ip.ip_sa.ip_addr.ipv6_addr[2] == 0)&&
                    (key->ip.ip_sa.ip_addr.ipv6_addr[3] == 0)){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }

                if(key->ip.ip_sa.addr_len > 128){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }

        }

        if(key_mask->ip_mask.ip_da){
            if(key->ip.ip_da.afi > CA_IPV6){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if (key->ip.ip_da.afi == CA_IPV4){

                if(key->ip.ip_da.ip_addr.ipv4_addr == 0){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }

                if(key->ip.ip_da.addr_len > 32){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }else{
                if((key->ip.ip_da.ip_addr.ipv6_addr[0] == 0)&&
                    (key->ip.ip_da.ip_addr.ipv6_addr[1] == 0)&&
                    (key->ip.ip_da.ip_addr.ipv6_addr[2] == 0)&&
                    (key->ip.ip_da.ip_addr.ipv6_addr[3] == 0)){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }

                if(key->ip.ip_da.addr_len > 128){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }
        }
        if(key_mask->ip_mask.ip_da_max){
            if(key_mask->ip_mask.ip_da == 0){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if(key->ip.ip_da_max.afi > CA_IPV6){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if (key->ip.ip_da_max.afi == CA_IPV4){
                if(key->ip.ip_da_max.ip_addr.ipv4_addr < key->ip.ip_da.ip_addr.ipv4_addr){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
                if(key->ip.ip_da_max.addr_len != 32){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }else{
                if(key->ip.ip_da_max.ip_addr.ipv6_addr[0]< key->ip.ip_da.ip_addr.ipv6_addr[0]){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                     return CA_E_PARAM;
                }else if(key->ip.ip_da_max.ip_addr.ipv6_addr[0] == key->ip.ip_da.ip_addr.ipv6_addr[0]){
                    if(key->ip.ip_da_max.ip_addr.ipv6_addr[1]< key->ip.ip_da.ip_addr.ipv6_addr[1]){
                        cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                         return CA_E_PARAM;
                    }else if(key->ip.ip_da_max.ip_addr.ipv6_addr[1] == key->ip.ip_da.ip_addr.ipv6_addr[1]){
                        if(key->ip.ip_da_max.ip_addr.ipv6_addr[2]< key->ip.ip_da.ip_addr.ipv6_addr[2]){
                            cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                            return CA_E_PARAM;
                        }else if(key->ip.ip_da_max.ip_addr.ipv6_addr[2] == key->ip.ip_da.ip_addr.ipv6_addr[2]){
                            if(key->ip.ip_da_max.ip_addr.ipv6_addr[3]< key->ip.ip_da.ip_addr.ipv6_addr[3]){
                                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                                return CA_E_PARAM;
                            }
                        }
                    }

                }
                if(key->ip.ip_da_max.addr_len != 128){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }

            }
        }

        if(key_mask->ip_mask.ip_sa_max){
            if(key_mask->ip_mask.ip_sa == 0){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if(key->ip.ip_sa_max.afi > CA_IPV6){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_PARAM;
            }
            if (key->ip.ip_sa_max.afi == CA_IPV4){
                if(key->ip.ip_sa_max.ip_addr.ipv4_addr < key->ip.ip_sa.ip_addr.ipv4_addr){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
                 if(key->ip.ip_sa_max.addr_len != 32){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }else{
                if(key->ip.ip_sa_max.ip_addr.ipv6_addr[0]< key->ip.ip_sa.ip_addr.ipv6_addr[0]){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                     return CA_E_PARAM;
                }else if(key->ip.ip_sa_max.ip_addr.ipv6_addr[0] == key->ip.ip_sa.ip_addr.ipv6_addr[0]){
                    if(key->ip.ip_sa_max.ip_addr.ipv6_addr[1]< key->ip.ip_sa.ip_addr.ipv6_addr[1]){
                        cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                         return CA_E_PARAM;
                    }else if(key->ip.ip_sa_max.ip_addr.ipv6_addr[1] == key->ip.ip_sa.ip_addr.ipv6_addr[1]){
                        if(key->ip.ip_sa_max.ip_addr.ipv6_addr[2]< key->ip.ip_sa.ip_addr.ipv6_addr[2]){
                            cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                            return CA_E_PARAM;
                        }else if(key->ip.ip_sa_max.ip_addr.ipv6_addr[2] == key->ip.ip_sa.ip_addr.ipv6_addr[2]){
                            if(key->ip.ip_sa_max.ip_addr.ipv6_addr[3]< key->ip.ip_sa.ip_addr.ipv6_addr[3]){
                                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                                return CA_E_PARAM;
                            }
                        }
                    }

                }
                if(key->ip.ip_sa_max.addr_len != 128){
                    cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                    return CA_E_PARAM;
                }
            }
        }


        if(key_mask->ip_mask.ip_da && key_mask->ip_mask.ip_sa){
            if(key_mask->ip_mask.ip_da_max || key_mask->ip_mask.ip_sa_max){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                return CA_E_NOT_SUPPORT;
            }
        }

        if(key_mask->ip_mask.ip_version){/* WORKAROUND: ipversion can be set to old or new definition in API doc */
            if (key->ip.ip_version != __CLS_IP_V4 &&
                key->ip.ip_version != __CLS_IP_V6 &&
                key->ip.ip_version != CA_IPV4 &&
                key->ip.ip_version != CA_IPV6) {
                cls_printf("IP version %u is invalid\n", key->ip.ip_version);
                return CA_E_PARAM;
            }
        }

        if(key_mask->ip_mask.dscp){
            __CLS_DSCP_CHECK(key->ip.dscp);
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            __CLS_DSCP_MASK_CHECK(key->ip.dscp_mask);
#endif
        }

        if(key->ip.ip_version == __CLS_IP_V4 || key->ip.ip_version == CA_IPV4){
            if(key_mask->ip_mask.have_options){
                if(key->ip.have_options > 1){
                    return CA_E_PARAM;
                }
            }
        }
        else{
            if(key_mask->ip_mask.flow_label){
                if(key->ip.flow_label > 0xfffff){
                    cls_printf("invalid flow_label %u\n", key->ip.flow_label);
                    return CA_E_PARAM;
                }
            }
        }
        if(key_mask->ip_mask.ext_header){
            return CA_E_NOT_SUPPORT;
        }
       /* if(key_mask->ip_mask.igmp_type){
            return CA_E_NOT_SUPPORT;
        }*/
    }

    if(key_mask->l4) {
     /*   if(key_mask->l4_mask.l4_valid){
            printk("l4_valid is not support\n");
            return CA_E_NOT_SUPPORT;
        }*/

        if(key_mask->l4_mask.src_port){
            if(key->l4.src_port.max > 0xffff){
                cls_printf("invalid src_port.max %u\n", key->l4.src_port.max);
                return CA_E_PARAM;
            }

            if(key->l4.src_port.max < key->l4.src_port.min)            {
                cls_printf("invalid src_port.min %u\n", key->l4.src_port.min);
                return CA_E_PARAM;
            }
        }

        if(key_mask->l4_mask.dst_port){
            if(key->l4.dst_port.max > 0xffff){
                cls_printf("invalid dst_port.max %u\n", key->l4.dst_port.max);
                return CA_E_PARAM;
            }

            if(key->l4.dst_port.max < key->l4.dst_port.min){
                cls_printf("invalid dst_port.min %u\n", key->l4.dst_port.min);
                return CA_E_PARAM;
            }
        }

        if(key_mask->l4_mask.tcp_flags){
            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_NS){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_CWR){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_ECE){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_URG){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_ACK){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

            if(key_mask->l4_mask.tcp_flags & __CLS_TCP_FLAGS_BIT_PSH){
                cls_printf("invalid tcp_flags mask %u\n", key->l4.tcp_flags);
                return CA_E_NOT_SUPPORT;
            }

        }
    }
    if(key_mask->ext) {
        switch(key->ext.length){
            case 0:
                break;
            case 1:
            if(!((key->ext.mask[0] == 0)||(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK))){
                return CA_E_NOT_SUPPORT;
            }
            break;
            case 2:
            if(!(((key->ext.mask[0] == 0)||(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK))&&
                ((key->ext.mask[1] == 0)||(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)))){
                return CA_E_NOT_SUPPORT;
            }
            break;
            case 3:
            if(!(((key->ext.mask[0] == 0)||(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK))&&
                ((key->ext.mask[1] == 0)||(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK))&&
                ((key->ext.mask[2] == 0)||(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK))
                )){
                return CA_E_NOT_SUPPORT;
            }
            break;
            case 4:
                if(!(((key->ext.mask[0] == 0)||(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK))&&
                                ((key->ext.mask[1] == 0)||(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK))&&
                    ((key->ext.mask[2] == 0)||(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK))&&
                    ((key->ext.mask[3] == 0)||(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK))
                                )){
                return CA_E_NOT_SUPPORT;
            }
            break;
            case 5:
            case 6:
            case 7:
            case 8:
                return CA_E_NOT_SUPPORT;
            default:
                return CA_E_PARAM;
        }
        if(key->ext.offset > 82){
            return CA_E_NOT_SUPPORT;
        }
    }
    return CA_E_OK;

}


static ca_status_t __cls_action_check(
    ca_boolean_t                     is_egr,
    ca_boolean_t                     dest_is_wan,
    ca_classifier_action_t           *action
)
{
    if((action->forward < CA_CLASSIFIER_FORWARD_DENY)
        || (action->forward > CA_CLASSIFIER_FORWARD_PORT)){
        cls_printf("fwd action invalid\n");
        return CA_E_PARAM;
    }

    if((action->forward == CA_CLASSIFIER_FORWARD_INTERFACE)){
        cls_printf("fwd action not suppport\n");
        return CA_E_NOT_SUPPORT;
    }

    if(action->options.masks.inner_dot1p){
        __CLS_DOT_1P_CHECK(action->options.inner_dot1p);
        return CA_E_NOT_SUPPORT;
    }

    if(action->options.masks.outer_dot1p){
        __CLS_DOT_1P_CHECK(action->options.outer_dot1p);
        if(is_egr == TRUE){
            return CA_E_NOT_SUPPORT;
        }
    }

    if(action->options.masks.dscp){

        __CLS_DSCP_CHECK(action->options.dscp);

        if(is_egr == TRUE){
            return CA_E_NOT_SUPPORT;
        }
    }

    if(action->options.masks.action_handle){
        //__CLS_FLOW_ID_CHECK(action->options.action_handle.flow_id);
    }

    if(action->options.masks.priority){
        __CLS_COS_CHECK(action->options.priority);
    }

    if(action->options.masks.inner_vlan_act){
        if((action->options.inner_vlan_act < CA_CLASSIFIER_VLAN_ACTION_NOP)
            || (action->options.inner_vlan_act > CA_CLASSIFIER_VLAN_ACTION_SWAP)){
            return CA_E_PARAM;
        }

        if((action->options.inner_vlan_act == CA_CLASSIFIER_VLAN_ACTION_PUSH)
            || (action->options.inner_vlan_act == CA_CLASSIFIER_VLAN_ACTION_SWAP)){
            __CLS_VLAN_ID_CHECK(action->options.inner_vid);
        }
        if(dest_is_wan){
            return CA_E_NOT_SUPPORT;
        }
    }

    if(action->options.masks.outer_vlan_act){
        if((action->options.outer_vlan_act < CA_CLASSIFIER_VLAN_ACTION_NOP)
            || (action->options.outer_vlan_act > CA_CLASSIFIER_VLAN_ACTION_SWAP)){
            return CA_E_PARAM;
        }

        if((action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_PUSH)
            || (action->options.outer_vlan_act == CA_CLASSIFIER_VLAN_ACTION_SWAP)){
            __CLS_VLAN_ID_CHECK(action->options.outer_vid);
        }
        if(dest_is_wan){
            return CA_E_NOT_SUPPORT;
        }
    }

    if(action->options.masks.sw_id || action->options.masks.mac_da){
        return CA_E_NOT_SUPPORT;
    }

    if(action->options.masks.sw_shaper_id){ 
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)        
        if(__CLS_PORT_CHECK_FOR_CPU256(action->dest.port)){
            return CA_E_PARAM;
        }
#else
        return CA_E_NOT_SUPPORT;
#endif
    }
    
    if(action->forward == CA_CLASSIFIER_FORWARD_PORT){
        if((action->dest.port & 0xffff) > 0x3f){
            return CA_E_PARAM;
        }
    }
    return CA_E_OK;
}



void __cls_key_dump(
    ca_device_id_t                 device_id,
    ca_classifier_key_t         *key,
    ca_classifier_key_mask_t    *key_mask)
{
    cls_printf("key_mask->src_port = %#x\n", key_mask->src_port);
    cls_printf("key_mask->dest_port = %#x\n",key_mask->dest_port);
    cls_printf("key_mask->src_intf = %u\n", key_mask->src_intf);
    cls_printf("key_mask->dst_intf = %u\n", key_mask->dst_intf);
    cls_printf("key_mask->l2       = %u\n", key_mask->l2);
    cls_printf("key_mask->ip       = %u\n", key_mask->ip);
    cls_printf("key_mask->l4       = %u\n", key_mask->l4);
    cls_printf("key_mask->ext      = %u\n", key_mask->ext);
    cls_printf("key->src_port      = %#x\n", key->src_port);
    cls_printf("key->dest_port     = %#x\n", key->dest_port);

    if (key_mask->l2) {
        cls_printf("key_mask->l2_mask.ethertype = %u\n", key_mask->l2_mask.ethertype);
        cls_printf("key_mask->l2_mask.subtype = %u\n", key_mask->l2_mask.subtype);
        cls_printf("key_mask->l2_mask.mac_sa = %u\n", key_mask->l2_mask.mac_sa);
        cls_printf("key_mask->l2_mask.mac_da = %u\n", key_mask->l2_mask.mac_da);
        cls_printf("key_mask->l2_mask.vlan_otag = %u\n", key_mask->l2_mask.vlan_otag);
        cls_printf("key_mask->l2_mask.is_multicast = %u\n", key_mask->l2_mask.is_multicast);
        cls_printf("key_mask->l2_mask.is_length = %u\n", key_mask->l2_mask.is_length);
        cls_printf("key_mask->l2_mask.cfm_opcode = %u\n", key_mask->l2_mask.cfm_opcode);
        cls_printf("key_mask->l2_mask.vlan_otag_mask.tpid = %u\n", key_mask->l2_mask.vlan_otag_mask.tpid);
        cls_printf("key_mask->l2_mask.vlan_otag_mask.vid = %u\n", key_mask->l2_mask.vlan_otag_mask.vid);
        cls_printf("key_mask->l2_mask.vlan_otag_mask.dei = %u\n", key_mask->l2_mask.vlan_otag_mask.dei);
        cls_printf("key_mask->l2_mask.vlan_otag_mask.pri = %u\n", key_mask->l2_mask.vlan_otag_mask.pri);
        if (key_mask->l2_mask.ethertype)
            cls_printf("key->l2.ethertype = 0x%04x\n", key->l2.ethertype);
        if (key_mask->l2_mask.subtype)
            cls_printf("key->l2.subtype = 0x%04x\n", key->l2.subtype);
        if (key_mask->l2_mask.vlan_otag) {
            if(key_mask->l2_mask.vlan_otag_mask.tpid){
                cls_printf("key->l2.vlan_otag.min.tpid = 0x%04x\n", key->l2.vlan_otag.vlan_min.tpid);
                cls_printf("key->l2.vlan_otag.max.tpid = 0x%04x\n", key->l2.vlan_otag.vlan_max.tpid);
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid){
                cls_printf("key->l2.vlan_otag.min.vid = 0x%04x\n", key->l2.vlan_otag.vlan_min.vid);
                cls_printf("key->l2.vlan_otag.max.vid = 0x%04x\n", key->l2.vlan_otag.vlan_max.vid);
            }
            
            if(key_mask->l2_mask.vlan_otag_mask.pri){
                cls_printf("key->l2.vlan_otag.min.pri = %u\n", key->l2.vlan_otag.vlan_min.pri);
                cls_printf("key->l2.vlan_otag.max.pri = %u\n", key->l2.vlan_otag.vlan_max.pri);
            }
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                cls_printf("key->l2.vlan_otag.min.dei = %u\n", key->l2.vlan_otag.vlan_min.dei);
                cls_printf("key->l2.vlan_otag.max.dei = %u\n", key->l2.vlan_otag.vlan_max.dei);
            }
        }
        if (key_mask->l2_mask.is_multicast)
            cls_printf("key->l2.is_multicast = %u\n", key->l2.is_multicast);
        if (key_mask->l2_mask.is_length)
            cls_printf("key->l2.is_length = %u\n", key->l2.is_length);
        if (key_mask->l2_mask.cfm_opcode)
            cls_printf("key->l2.cfm_opcode = %u\n", key->l2.cfm_opcode);
        if (key_mask->l2_mask.mac_sa){
            cls_printf("key->l2.mac_sa min = %02x:%02x:%02x:%02x:%02x:%02x\n",
            key->l2.mac_sa.mac_min[0], key->l2.mac_sa.mac_min[1], key->l2.mac_sa.mac_min[2],
            key->l2.mac_sa.mac_min[3], key->l2.mac_sa.mac_min[4], key->l2.mac_sa.mac_min[5]
            );
            cls_printf("key->l2.mac_sa max = %02x:%02x:%02x:%02x:%02x:%02x\n",
             key->l2.mac_sa.mac_max[0], key->l2.mac_sa.mac_max[1], key->l2.mac_sa.mac_max[2],
             key->l2.mac_sa.mac_max[3], key->l2.mac_sa.mac_max[4], key->l2.mac_sa.mac_max[5]
            );
        }

        if (key_mask->l2_mask.mac_da){
            cls_printf("key->l2.mac_da min= %02x:%02x:%02x:%02x:%02x:%02x\n",
            key->l2.mac_da.mac_min[0], key->l2.mac_da.mac_min[1], key->l2.mac_da.mac_min[2],
            key->l2.mac_da.mac_min[3], key->l2.mac_da.mac_min[4], key->l2.mac_da.mac_min[5]
            );
            cls_printf("key->l2.mac_da max = %02x:%02x:%02x:%02x:%02x:%02x\n",
            key->l2.mac_da.mac_max[0], key->l2.mac_da.mac_max[1], key->l2.mac_da.mac_max[2],
            key->l2.mac_da.mac_max[3], key->l2.mac_da.mac_max[4], key->l2.mac_da.mac_max[5]
            );
        }
    }

    if (key_mask->ip) {
        cls_printf("key_mask->ip_mask.ip_valid = %u\n", key_mask->ip_mask.ip_valid);
        cls_printf("key_mask->ip_mask.ip_version = %u\n", key_mask->ip_mask.ip_version);
        cls_printf("key_mask->ip_mask.ip_protocol = %u\n", key_mask->ip_mask.ip_protocol);
        cls_printf("key_mask->ip_mask.dscp = %u\n", key_mask->ip_mask.dscp);
        cls_printf("key_mask->ip_mask.ecn = %u\n", key_mask->ip_mask.ecn);

        cls_printf("key_mask->ip_mask.ip_sa = %u\n", key_mask->ip_mask.ip_sa);
        cls_printf("key_mask->ip_mask.ip_sa_max = %u\n", key_mask->ip_mask.ip_sa_max);
        cls_printf("key_mask->ip_mask.ip_da = %u\n", key_mask->ip_mask.ip_da);
        cls_printf("key_mask->ip_mask.ip_da_max = %u\n", key_mask->ip_mask.ip_da_max);

        cls_printf("key_mask->ip_mask.fragment = %u\n", key_mask->ip_mask.fragment);
        cls_printf("key_mask->ip_mask.have_options = %u\n", key_mask->ip_mask.have_options);
        cls_printf("key_mask->ip_mask.flow_label = %u\n", key_mask->ip_mask.flow_label);
        cls_printf("key_mask->ip_mask.ext_header = %u\n", key_mask->ip_mask.ext_header);
        cls_printf("key_mask->ip_mask.icmp_type = %u\n", key_mask->ip_mask.icmp_type);
        cls_printf("key_mask->ip_mask.igmp_type = %u\n", key_mask->ip_mask.igmp_type);

        if (key_mask->ip_mask.ip_valid)
            cls_printf("key->ip.ip_valid = %u\n", key->ip.ip_valid);
        if (key_mask->ip_mask.ip_version)
            cls_printf("key->ip.ip_version = %u\n", key->ip.ip_version);
        if (key_mask->ip_mask.ip_protocol)
            cls_printf("key->ip.ip_protocol = %u\n", key->ip.ip_protocol);
        if (key_mask->ip_mask.dscp)
            cls_printf("key->ip.dscp = %u\n", key->ip.dscp);
        if (key_mask->ip_mask.ecn)
            cls_printf("key->ip.ecn = %u\n", key->ip.ecn);
        if (key_mask->ip_mask.ip_sa) {
            cls_printf("key->ip.ip_sa.afi = %u\n", key->ip.ip_sa.afi);
            cls_printf("key->ip.ip_sa.addr_len = %u\n", key->ip.ip_sa.addr_len);
            if (key->ip.ip_sa.afi == CA_IPV4)
                cls_printf("key->ip.ip_sa.ip_addr = 0x%08x\n", key->ip.ip_sa.ip_addr.ipv4_addr);
            else
                cls_printf("key->ip.ip_sa.ip_addr = 0x%08x:0x%08x:0x%08x:0x%08x\n",
                key->ip.ip_sa.ip_addr.ipv6_addr[0],
                key->ip.ip_sa.ip_addr.ipv6_addr[1],
                key->ip.ip_sa.ip_addr.ipv6_addr[2],
                key->ip.ip_sa.ip_addr.ipv6_addr[3]
                );
        }
        if (key_mask->ip_mask.ip_sa_max) {
            cls_printf("key->ip.ip_sa_max.afi = %u\n", key->ip.ip_sa_max.afi);
            cls_printf("key->ip.ip_sa_max.addr_len = %u\n", key->ip.ip_sa_max.addr_len);
            if (key->ip.ip_sa_max.afi == CA_IPV4)
                cls_printf("key->ip.ip_sa_max.ip_addr = 0x%08x\n", key->ip.ip_sa_max.ip_addr.ipv4_addr);
            else
                cls_printf("key->ip.ip_sa_max.ip_addr = 0x%08x:0x%08x:0x%08x:0x%08x\n",
                key->ip.ip_sa_max.ip_addr.ipv6_addr[0],
                key->ip.ip_sa_max.ip_addr.ipv6_addr[1],
                key->ip.ip_sa_max.ip_addr.ipv6_addr[2],
                key->ip.ip_sa_max.ip_addr.ipv6_addr[3]
                );
        }
        if (key_mask->ip_mask.ip_da) {
            cls_printf("key->ip.ip_da.afi = %u\n", key->ip.ip_da.afi);
            cls_printf("key->ip.ip_da.addr_len = %u\n", key->ip.ip_da.addr_len);
            if (key->ip.ip_da.afi == CA_IPV4)
                cls_printf("key->ip.ip_da.ip_addr = 0x%08x\n", key->ip.ip_da.ip_addr.ipv4_addr);
            else
                cls_printf("key->ip.ip_da.ip_addr = 0x%08x:0x%08x:0x%08x:0x%08x\n",
                key->ip.ip_da.ip_addr.ipv6_addr[0],
                key->ip.ip_da.ip_addr.ipv6_addr[1],
                key->ip.ip_da.ip_addr.ipv6_addr[2],
                key->ip.ip_da.ip_addr.ipv6_addr[3]
                );
        }
        if (key_mask->ip_mask.ip_da_max) {
            cls_printf("key->ip.ip_da_max.afi = %u\n", key->ip.ip_da_max.afi);
            cls_printf("key->ip.ip_da_max.addr_len = %u\n", key->ip.ip_da_max.addr_len);
            if (key->ip.ip_da_max.afi == CA_IPV4)
                cls_printf("key->ip.ip_da_max.ip_addr = 0x%08x\n", key->ip.ip_da_max.ip_addr.ipv4_addr);
            else
                cls_printf("key->ip.ip_da_max.ip_addr = 0x%08x:0x%08x:0x%08x:0x%08x\n",
                key->ip.ip_da_max.ip_addr.ipv6_addr[0],
                key->ip.ip_da_max.ip_addr.ipv6_addr[1],
                key->ip.ip_da_max.ip_addr.ipv6_addr[2],
                key->ip.ip_da_max.ip_addr.ipv6_addr[3]
                );
        }
        if (key_mask->ip_mask.fragment)
            cls_printf("key->ip.fragment = %u\n", key->ip.fragment);
        if (key_mask->ip_mask.have_options)
            cls_printf("key->ip.have_options = %u\n", key->ip.have_options);
        if (key_mask->ip_mask.flow_label)
            cls_printf("key->ip.flow_label = %u\n", key->ip.flow_label);
        if (key_mask->ip_mask.ext_header)
            cls_printf("key->ip.ext_header = %u\n", key->ip.ext_header);
        if (key_mask->ip_mask.icmp_type)
            cls_printf("key->ip.icmp_type = %u\n", key->ip.icmp_type);
        if (key_mask->ip_mask.igmp_type)
            cls_printf("key->ip.igmp_type = %u\n", key->ip.igmp_type);

    }

    if (key_mask->l4) {
        cls_printf("key_mask->l4_mask.l4_valid = %u\n", key_mask->l4_mask.l4_valid);
        cls_printf("key_mask->l4_mask.src_port = %u\n", key_mask->l4_mask.src_port);
        cls_printf("key_mask->l4_mask.dst_port = %u\n", key_mask->l4_mask.dst_port);
        cls_printf("key_mask->l4_mask.tcp_flags = %#x\n", key_mask->l4_mask.tcp_flags);

        if (key_mask->l4_mask.l4_valid)
            cls_printf("key->l4.l4_valid = %x\n", key->l4.l4_valid);
        if (key_mask->l4_mask.src_port)
            cls_printf("key->l4.src_port max = %x , min = %x\n", key->l4.src_port.max, key->l4.src_port.min);
        if (key_mask->l4_mask.dst_port)
            cls_printf("key->l4.dst_port max = %x, min = %x\n", key->l4.dst_port.max, key->l4.dst_port.min);
        if (key_mask->l4_mask.tcp_flags)
            cls_printf("key->l4.tcp_flags = %#x\n", key->l4.tcp_flags);
    }
}

void __cls_action_dump(
    ca_classifier_action_t   *action
)
{
    cls_printf("action->forward   = %u\n", action->forward);
    cls_printf("action->dest.drop = %u\n", action->dest.drop);
    cls_printf("action->dest.fe   = %u\n", action->dest.fe);
    cls_printf("action->dest.intf = %u\n", action->dest.intf);
    cls_printf("action->dest.port = %u\n", action->dest.port);

    cls_printf("action->options.masks.action_handle = %u\n", action->options.masks.action_handle);
    cls_printf("action->options.masks.priority = %u\n", action->options.masks.priority);
    cls_printf("action->options.masks.dscp = %u\n", action->options.masks.dscp);
    cls_printf("action->options.masks.inner_dot1p = %u\n", action->options.masks.inner_dot1p);
    cls_printf("action->options.masks.inner_vlan_act = %u\n", action->options.masks.inner_vlan_act);
    cls_printf("action->options.masks.outer_dot1p = %u\n", action->options.masks.outer_dot1p);
    cls_printf("action->options.masks.outer_vlan_act = %u\n", action->options.masks.outer_vlan_act);
    cls_printf("action->options.masks.mac_da = %u\n", action->options.masks.mac_da);

    if (action->options.masks.action_handle)
        cls_printf("action->options.action_handle.flow_id = %u\n", action->options.action_handle.flow_id);
    if (action->options.masks.priority)
        cls_printf("action->options.priority = %u\n", action->options.priority);
    if (action->options.masks.dscp)
        cls_printf("action->options.dscp = %u\n", action->options.dscp);
    if (action->options.masks.inner_dot1p)
        cls_printf("action->options.inner_dot1p = %u\n", action->options.inner_dot1p);
    if (action->options.masks.inner_vlan_act)
        cls_printf("action->options.inner_vid = %u\n", action->options.inner_vid);
    if (action->options.masks.inner_vlan_act)
        cls_printf("action->options.inner_vlan_act = %u\n", action->options.inner_vlan_act);
    if (action->options.masks.outer_dot1p)
        cls_printf("action->options.outer_dot1p = %u\n", action->options.outer_dot1p);
    if (action->options.masks.outer_vlan_act)
        cls_printf("action->options.outer_vid = %u\n", action->options.outer_vid);
    if (action->options.masks.outer_vlan_act)
        cls_printf("action->options.outer_vlan_act = %u\n", action->options.outer_vlan_act);
    if (action->options.masks.mac_da)
        cls_printf("action->options.mac_da = %02x:%02x:%02x:%02x:%02x:%02x\n",
        action->options.mac_da[0],
        action->options.mac_da[1],
        action->options.mac_da[2],
        action->options.mac_da[3],
        action->options.mac_da[4],
        action->options.mac_da[5]
    );

}

void __l2_key_mac_exchange(ca_mac_addr_t mac1, ca_mac_addr_t mac2)
{
    mac1[0] = mac2[0];
    mac1[1] = mac2[1];
    mac1[2] = mac2[2];
    mac1[3] = mac2[3];
    mac1[4] = mac2[4];
    mac1[5] = mac2[5];
    return ;
}

ca_status_t __l2_cls_ipv4_key_get(
    ca_device_id_t                 device_id,
    ca_uint32_t                  priority,
    ca_classifier_key_t         *key,
    ca_classifier_key_mask_t    *key_mask,
    ca_boolean_t                is_ipv4_ipv6,
    aal_l2_cls_ipv4_key_mask_t  *ipv4_key_mask,
    aal_l2_cls_ipv4_key_t        *ipv4_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t eth_type_index;
    ca_vlan_port_control_t vlan_port_control;
    ca_uint32_t ether_type;

    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));

    if((key == NULL) || (key_mask == NULL) ||(priority >7)||
        (ipv4_key_mask == NULL) ||(ipv4_key == NULL)||(is_ipv4_ipv6 >1)){
        return CA_E_PARAM;
    }

    ipv4_key->valid                  = FALSE;
    ipv4_key->is_ipv4_ipv6_key       = 0;
    ipv4_key->ipv4_ecn               = 0;
    ipv4_key->ipv4_ecn_vld           = 0;
    ipv4_key->mac_field_ctrl         = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    ipv4_key->top_vlan_id            = 0;
    ipv4_key->top_is_svlan           = 0;
    ipv4_key->inner_vlan_id          = 0;
    ipv4_key->inner_is_svlan         = 0;
    ipv4_key->vlan_field_ctrl        = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    ipv4_key->top_dot1p              = 0;
    ipv4_key->inner_dot1p            = 0;
    ipv4_key->dot1p_ctrl             = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    ipv4_key->vlan_number            = 0;
    ipv4_key->vlan_number_vld        = 0;
    ipv4_key->ethernet_type          = 0;
    ipv4_key->ethernet_type_vld      = 0;
    ipv4_key->ip_da                  = 0;
    ipv4_key->ip_da_mask             = 0;
    ipv4_key->ip_sa                  = 0;
    ipv4_key->ip_sa_mask             = 0;
    ipv4_key->ip_ctrl                = AAL_L2_CLS_IP_CTRL_MASK_IP_ADDR;
    ipv4_key->ip_pt                  = 0;
    ipv4_key->ip_pt_valid            = 0;
    ipv4_key->dscp                   = 0;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv4_key->dscp_mask              = 0;
#endif
    ipv4_key->dscp_valid             = 0;
    ipv4_key->ip_option              = 0;
    ipv4_key->ip_option_valid        = 0;
    ipv4_key->ip_fragment            = 0;
    ipv4_key->ip_fragment_valid      = 0;
    ipv4_key->l4_dst_port_hi         = 0;
    ipv4_key->l4_dst_port_lo         = 0;
    ipv4_key->l4_dst_ctrl            = AAL_L2_CLS_L4_DST_PORT_FIELD_INVALID;
    ipv4_key->l4_src_port_hi         = 0;
    ipv4_key->l4_src_port_lo         = 0;
    ipv4_key->l4_src_ctrl            = AAL_L2_CLS_L4_SRC_PORT_FIELD_INVALID;
    ipv4_key->l4_flag                = 0;
    ipv4_key->l4_flag_valid          = 0;
    ipv4_key->special_pkt_code       = 0;
    ipv4_key->special_pkt_code_valid = 0;
    ipv4_key->global_key             = 0;
    ipv4_key->global_key_mask        = 0xF;
    ipv4_key->l7_ctrl                = AAL_L2_CLS_L7_CTRL_MASK;
    ipv4_key->fields_is_or           = 0;
    ipv4_key->precedence             = priority;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv4_key->top_dei_vld            = 0;
    ipv4_key->top_dei                = 0;
     ipv4_key->inner_dei_vld        = 0;
    ipv4_key->inner_dei             = 0;   
#endif
    ipv4_key_mask->s.valid                  = 1;
    ipv4_key_mask->s.is_ipv4_ipv6_key       = 1;
    ipv4_key_mask->s.ipv4_ecn               = 1;
    ipv4_key_mask->s.ipv4_ecn_vld           = 1;
    ipv4_key_mask->s.mac_da                 = 1;
    ipv4_key_mask->s.mac_sa                 = 1;
    ipv4_key_mask->s.mac_field_ctrl         = 1;
    ipv4_key_mask->s.top_vlan_id            = 1;
    ipv4_key_mask->s.top_is_svlan           = 1;
    ipv4_key_mask->s.inner_vlan_id          = 1;
    ipv4_key_mask->s.inner_is_svlan         = 1;
    ipv4_key_mask->s.vlan_field_ctrl        = 1;
    ipv4_key_mask->s.top_dot1p              = 1;
    ipv4_key_mask->s.inner_dot1p            = 1;
    ipv4_key_mask->s.dot1p_ctrl             = 1;
    ipv4_key_mask->s.vlan_number            = 1;
    ipv4_key_mask->s.vlan_number_vld        = 1;
    ipv4_key_mask->s.ethernet_type          = 1;
    ipv4_key_mask->s.ethernet_type_vld      = 1;
    ipv4_key_mask->s.ip_da                  = 1;
    ipv4_key_mask->s.ip_da_mask             = 1;
    ipv4_key_mask->s.ip_sa                  = 1;
    ipv4_key_mask->s.ip_sa_mask             = 1;
    ipv4_key_mask->s.ip_ctrl                = 1;
    ipv4_key_mask->s.ip_pt                  = 1;
    ipv4_key_mask->s.ip_pt_valid            = 1;
    ipv4_key_mask->s.dscp                   = 1;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv4_key_mask->s.dscp_mask              = 1;
#endif
    ipv4_key_mask->s.dscp_valid             = 1;
    ipv4_key_mask->s.ip_option              = 1;
    ipv4_key_mask->s.ip_option_valid        = 1;
    ipv4_key_mask->s.ip_fragment            = 1;
    ipv4_key_mask->s.ip_fragment_valid      = 1;
    ipv4_key_mask->s.l4_dst_port_hi         = 1;
    ipv4_key_mask->s.l4_dst_port_lo         = 1;
    ipv4_key_mask->s.l4_dst_ctrl            = 1;
    ipv4_key_mask->s.l4_src_port_hi         = 1;
    ipv4_key_mask->s.l4_src_port_lo         = 1;
    ipv4_key_mask->s.l4_src_ctrl            = 1;
    ipv4_key_mask->s.l4_flag                = 1;
    ipv4_key_mask->s.l4_flag_valid          = 1;
    ipv4_key_mask->s.special_pkt_code       = 1;
    ipv4_key_mask->s.special_pkt_code_valid = 1;
    ipv4_key_mask->s.global_key             = 1;
    ipv4_key_mask->s.global_key_mask        = 1;
    ipv4_key_mask->s.l7_ctrl                = 1;
    ipv4_key_mask->s.fields_is_or           = 1;
    ipv4_key_mask->s.precedence             = 1;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv4_key_mask->s.top_dei_vld            = 1;
    ipv4_key_mask->s.top_dei                = 1;
    ipv4_key_mask->s.inner_dei_vld            = 1;
    ipv4_key_mask->s.inner_dei                = 1;
#endif
    if(key_mask->src_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->src_port,&vlan_port_control);
    }else if(key_mask->dest_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->dest_port,&vlan_port_control);
    }

    if(CA_E_OK != ret){
       cls_printf("%s,ca_l2_vlan_port_control_get() failed,%d \n",__FUNCTION__,__LINE__ );
       goto end;
    }

    if(key_mask->l2){
        if((key_mask->l2_mask.mac_da)&&(key_mask->l2_mask.mac_sa)){
            __l2_key_mac_exchange(ipv4_key->mac_da, key->l2.mac_da.mac_min);
            __l2_key_mac_exchange(ipv4_key->mac_sa, key->l2.mac_sa.mac_min);
            ipv4_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_ALL_MATCH;
        }else if(key_mask->l2_mask.mac_da){
            __l2_key_mac_exchange(ipv4_key->mac_da, key->l2.mac_da.mac_min);
            __l2_key_mac_exchange(ipv4_key->mac_sa, key->l2.mac_da.mac_max);
            ipv4_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_DA_RANGE;
        }else if(key_mask->l2_mask.mac_sa){
            __l2_key_mac_exchange(ipv4_key->mac_da, key->l2.mac_sa.mac_min);
            __l2_key_mac_exchange(ipv4_key->mac_sa, key->l2.mac_sa.mac_max);
            ipv4_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_SA_RANGE;
        }

        if(key_mask->l2_mask.vlan_otag && key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_otag_mask.pri && key_mask->l2_mask.vlan_itag_mask.pri){
                ipv4_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                ipv4_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                ipv4_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_ALL_MATCH;
            }else if(key_mask->l2_mask.vlan_otag_mask.pri){
                ipv4_key->top_dot1p          = key->l2.vlan_otag.vlan_max.pri;
                ipv4_key->inner_dot1p        = key->l2.vlan_otag.vlan_min.pri;
                ipv4_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;

            }else if(key_mask->l2_mask.vlan_itag_mask.pri){
                ipv4_key->top_dot1p          = key->l2.vlan_itag.vlan_max.pri;
                ipv4_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                ipv4_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;

            }
            if(key_mask->l2_mask.vlan_otag_mask.vid && key_mask->l2_mask.vlan_itag_mask.vid){
                ipv4_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                ipv4_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                      ipv4_key->top_is_svlan = 1;
                  }else{
                      ipv4_key->top_is_svlan           = vlan_port_control.outer_tpid;
                  }
                ipv4_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                ipv4_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_ALL_MATCH;
            }else if(key_mask->l2_mask.vlan_otag_mask.vid){
                ipv4_key->top_vlan_id            =  key->l2.vlan_otag.vlan_max.vid;
                ipv4_key->inner_vlan_id          = key->l2.vlan_otag.vlan_min.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                      ipv4_key->top_is_svlan = 1;
                  }else{
                      ipv4_key->top_is_svlan           = vlan_port_control.outer_tpid;
                  }
                ipv4_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
            }else if(key_mask->l2_mask.vlan_itag_mask.vid){
                ipv4_key->top_vlan_id            =  key->l2.vlan_itag.vlan_max.vid;
                ipv4_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                ipv4_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                ipv4_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                ipv4_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                ipv4_key->top_dei_vld = TRUE;
            }
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                ipv4_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                ipv4_key->inner_dei_vld = TRUE;
            }
#endif
        }else if(key_mask->l2_mask.vlan_otag){
            if(key_mask->l2_mask.vlan_otag_mask.pri){
                ipv4_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                ipv4_key->inner_dot1p        = key->l2.vlan_otag.vlan_max.pri;
                ipv4_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid){
                ipv4_key->top_vlan_id            =  key->l2.vlan_otag.vlan_max.vid;
                ipv4_key->inner_vlan_id          = key->l2.vlan_otag.vlan_min.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                      ipv4_key->top_is_svlan = 1;
                  }else{
                      ipv4_key->top_is_svlan           = vlan_port_control.outer_tpid;
                  }
                ipv4_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
            }
 #if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                ipv4_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                ipv4_key->top_dei_vld = TRUE;
            }
#endif
        }else if(key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_itag_mask.pri){

                ipv4_key->top_dot1p          = key->l2.vlan_itag.vlan_max.pri;
                ipv4_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                ipv4_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;
            }
            if(key_mask->l2_mask.vlan_itag_mask.vid){
                ipv4_key->top_vlan_id            =  key->l2.vlan_itag.vlan_max.vid;
                ipv4_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                ipv4_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                //ipv4_key->top_is_svlan           = TRUE;
                ipv4_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                ipv4_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                ipv4_key->inner_dei_vld = TRUE;
            }
#endif
        }
        if(key_mask->l2_mask.ethertype){
            ipv4_key->ethernet_type_vld          = TRUE;
            ret = __cls_ethertype_encode_find(device_id,key->l2.ethertype,&eth_type_index);
            if(CA_E_OK != ret){
                goto end;
            }
            ipv4_key->ethernet_type = eth_type_index;
            }
            if(key_mask->l2_mask.subtype){
                ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_COMPARE_DIP;
                if(key->l2.ethertype == 0x8809){
                    ipv4_key->ip_da      = (ca_uint32_t)(key->l2.subtype <<24);
                    ipv4_key->ip_da_mask = 24;
                }else if(key->l2.ethertype == 0x8808){
                     ipv4_key->ip_da      = (ca_uint32_t)(key->l2.subtype <<16);
                    ipv4_key->ip_da_mask = 16;
                }
            }
            if(key_mask->l2_mask.cfm_opcode){
                ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_COMPARE_DIP;
                ipv4_key->ip_da      = (ca_uint32_t)(key->l2.cfm_opcode<<16);
                ipv4_key->ip_da_mask = 16;
            }
        if(key_mask->l2_mask.vlan_count){
            ipv4_key->vlan_number_vld = TRUE;
            ipv4_key->vlan_number   = key->l2.vlan_count;
        }
    }

    if(is_ipv4_ipv6 == 0){
        if(key_mask->ip){
            if(key_mask->ip_mask.ip_da && key_mask->ip_mask.ip_sa){
                ipv4_key->ip_da = key->ip.ip_da.ip_addr.ipv4_addr;
                ipv4_key->ip_sa = key->ip.ip_sa.ip_addr.ipv4_addr;
                ipv4_key->ip_da_mask = (32 - key->ip.ip_da.addr_len);
                ipv4_key->ip_sa_mask = (32 - key->ip.ip_sa.addr_len);
                ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_ALL_MATCH;
            }else if(key_mask->ip_mask.ip_da){
                if(key_mask->ip_mask.ip_da_max){
                    ipv4_key->ip_da =  key->ip.ip_da_max.ip_addr.ipv4_addr;
                    ipv4_key->ip_sa = key->ip.ip_da.ip_addr.ipv4_addr;
                    ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_DIP_RANGE;
                }else{
                    ipv4_key->ip_da =  key->ip.ip_da.ip_addr.ipv4_addr;
                    ipv4_key->ip_sa = key->ip.ip_da.ip_addr.ipv4_addr;

                    if(key->ip.ip_da.addr_len == 32){
                        ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_DIP_RANGE;
                    }else{
                        ipv4_key->ip_da_mask = (32 - key->ip.ip_da.addr_len);
                        ipv4_key->ip_sa_mask = 31;
                        ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_ALL_MATCH;
                    }
                }
            }else if(key_mask->ip_mask.ip_sa){
                if(key_mask->ip_mask.ip_da_max){
                    ipv4_key->ip_da = key->ip.ip_sa_max.ip_addr.ipv4_addr;
                    ipv4_key->ip_sa = key->ip.ip_sa.ip_addr.ipv4_addr;
                    ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_SIP_RANGE;
                }else{
                    ipv4_key->ip_da = key->ip.ip_sa.ip_addr.ipv4_addr;
                    ipv4_key->ip_sa = key->ip.ip_sa.ip_addr.ipv4_addr;

                    if(key->ip.ip_sa.addr_len == 32){
                        ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_SIP_RANGE;
                    }else{
                        ipv4_key->ip_sa_mask = (32 - key->ip.ip_sa.addr_len);
                        ipv4_key->ip_da_mask = 31;
                        ipv4_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_ALL_MATCH;
                    }
                }
            }

            if(key_mask->ip_mask.dscp){
                ipv4_key->dscp = key->ip.dscp;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
                ipv4_key->dscp_mask = key->ip.dscp_mask;
#endif
                ipv4_key->dscp_valid = TRUE;
            }
            if(key_mask->ip_mask.ecn){
                ipv4_key->ipv4_ecn = key->ip.ecn;
                ipv4_key->ipv4_ecn_vld = TRUE;
            }
            if(key_mask->ip_mask.fragment){
                ipv4_key->ip_fragment =  (key->ip.fragment >1);
                ipv4_key->ip_fragment_valid = TRUE;
            }
            if(key_mask->ip_mask.have_options){
                ipv4_key->ip_option = key->ip.have_options;
                ipv4_key->ip_option_valid = TRUE;
            }
            if(key_mask->ip_mask.ip_protocol){
                ipv4_key->ip_pt= key->ip.ip_protocol;
                ipv4_key->ip_pt_valid = TRUE;
            }
            if(key_mask->ip_mask.icmp_type){
                ipv4_key->ip_pt = 0x01;
                ipv4_key->ip_pt_valid = TRUE;
                ipv4_key->l7_ctrl = AAL_L2_CLS_L7_CTRL_CMPR_AFTER_IP_HEADER;
                ipv4_key->global_key = (key->ip.icmp_type << 24);
                ipv4_key->global_key_mask = 0x07;
            }

             if(key_mask->ip_mask.igmp_type){
                ipv4_key->ip_pt = 0x02;
                ipv4_key->ip_pt_valid = TRUE;
                ipv4_key->l7_ctrl = AAL_L2_CLS_L7_CTRL_CMPR_AFTER_IP_HEADER;
                ipv4_key->global_key = (key->ip.igmp_type << 24);
                ipv4_key->global_key_mask = 0x07;
            }

              if(key_mask->ip_mask.ip_version && (!(key_mask->ip_mask.dscp              ||
                                                       key_mask->ip_mask.ip_protocol    ||
                                                       key_mask->ip_mask.ecn            ||
                                                       key_mask->ip_mask.fragment       ||
                                                       key_mask->ip_mask.have_options   ||
                                                       key_mask->ext                    ||
                                                       key_mask->ip_mask.icmp_type      ||
                                                       key_mask->ip_mask.igmp_type      ||
                                                       key_mask->l2_mask.subtype))){
                ipv4_key->ethernet_type_vld = TRUE;
                if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                    ether_type = __CLS_IPV4_ETH_TYPE;
                }
                if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                    ether_type = __CLS_IPV6_ETH_TYPE;
                }
                
                ret = __cls_ethertype_encode_find(device_id,ether_type,&eth_type_index);
                ipv4_key->ethernet_type = eth_type_index;
            }

        }
        if(key_mask->l4){
            if(key_mask->l4_mask.l4_valid){
                ipv4_key->l4_src_port_hi = 0xFFFF;
                ipv4_key->l4_src_port_lo = 0;
                ipv4_key->l4_src_ctrl = AAL_L2_CLS_L4_SRC_PORT_FIELD_IS_RANGE;
            }
            if(key_mask->l4_mask.src_port){
                ipv4_key->l4_src_port_hi = key->l4.src_port.max;
                ipv4_key->l4_src_port_lo = key->l4.src_port.min;
                ipv4_key->l4_src_ctrl = AAL_L2_CLS_L4_SRC_PORT_FIELD_IS_RANGE;

            }
            if( key_mask->l4_mask.dst_port){
                ipv4_key->l4_dst_port_hi = key->l4.dst_port.max;
                ipv4_key->l4_dst_port_lo = key->l4.dst_port.min;
                ipv4_key->l4_dst_ctrl = AAL_L2_CLS_L4_DST_PORT_FIELD_IS_RANGE;
            }
            if(key_mask->l4_mask.tcp_flags){
                ipv4_key->l4_flag = key->l4.tcp_flags;
                ipv4_key->l4_flag_valid = TRUE;
            }
        }
        if(key_mask->ext){
            ipv4_key->l7_ctrl = AAL_L2_CLS_L7_CTRL_CMPR_AFTER_PROGRAMABLE_4B;
            switch(key->ext.length){
                case 0:
                     ipv4_key->l7_ctrl = AAL_L2_CLS_L7_CTRL_MASK;
                case 1:
                    ipv4_key->global_key = (key->ext.data[0]<<24);
                    if(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x07;
                    }else{
                        ipv4_key->global_key_mask = 0x0f;
                    }
                break;
                case 2:
                    ipv4_key->global_key = ((key->ext.data[0]<<24) |
                                            (key->ext.data[1]<<16));
                    if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x03;
                    }else if(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x07;
                    }else if(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0B;
                    }else{
                        ipv4_key->global_key_mask = 0x0F;
                    }
                 break;
                case 3:
                    ipv4_key->global_key = ((key->ext.data[0]<<24) |
                                             (key->ext.data[1]<<16) |
                                             (key->ext.data[2]<<8));
                    if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)&&
                        (key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x01;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x03;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x05;
                    }else if((key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x09;
                    }else if(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x07;
                    }else if(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0B;
                    }else if(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0D;
                    }else{
                        ipv4_key->global_key_mask = 0x0F;
                    }
                break;
                case 4:
                    ipv4_key->global_key = ((key->ext.data[0]<<24) |
                                              (key->ext.data[1]<<16)|
                                              (key->ext.data[2]<<8) |
                                              (key->ext.data[3]));

                    if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)&&
                        (key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x00;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)
                    &&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x01;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)
                    &&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x02;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)
                    &&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x04;
                    }else if((key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)
                    &&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x08;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x05;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x03;
                    }else if((key->ext.mask[0] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x06;
                    }else if((key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x09;
                    }else if((key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x0A;
                    }else if((key->ext.mask[2] == __CLS_IPV4_BYTE_MASK)&&(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK)){
                        ipv4_key->global_key_mask = 0x0C;
                    }
                    else if(key->ext.mask[0] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x07;
                    }else if(key->ext.mask[1] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0B;
                    }else if(key->ext.mask[2] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0D;
                    }else if(key->ext.mask[3] == __CLS_IPV4_BYTE_MASK){
                        ipv4_key->global_key_mask = 0x0E;
                    }else{
                        ipv4_key->global_key_mask = 0x0F;
                    }
                break;
                default:
                break;
            }

        }

    }else{
            ipv4_key->is_ipv4_ipv6_key = TRUE;
            if(key_mask->ip_mask.ip_version){
                ipv4_key->ethernet_type_vld = TRUE;
                if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                    ether_type = __CLS_IPV4_ETH_TYPE;
                }
                if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                    ether_type = __CLS_IPV6_ETH_TYPE;
                }
                ret = __cls_ethertype_encode_find(device_id,ether_type,&eth_type_index);
                ipv4_key->ethernet_type = eth_type_index;
            }
        }
end:
    return ret;
}


ca_status_t __l2_cls_ipv6_short_key_get(
    ca_device_id_t                         device_id,
    ca_uint32_t                          priority,
    ca_classifier_key_t                 *key,
    ca_classifier_key_mask_t            *key_mask,
    aal_l2_cls_ipv6_short_key_mask_t    *ipv6_short_key_mask,
    aal_l2_cls_ipv6_short_key_t        *ipv6_short_key
)
{
    ca_status_t ret = CA_E_OK;
    //ca_uint32_t eth_type_index;
    ca_vlan_port_control_t vlan_port_control;

    if((key == NULL) || (key_mask == NULL) ||(priority >7)||
        (ipv6_short_key_mask == NULL) ||(ipv6_short_key == NULL)){
        return CA_E_PARAM;
    }

    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));

    ipv6_short_key->valid                   = FALSE;
    ipv6_short_key->is_ipv4_ipv6_key        = 0;
    ipv6_short_key->ipv6_ecn                = 0;
    ipv6_short_key->ipv6_ecn_valid          = 0;
    ipv6_short_key->ipv6_addr_mask          = 0;
    ipv6_short_key->ipv6_ctrl               = AAL_L2_CLS_IPV6_CTRL_MASK_IP_ADDR;
    ipv6_short_key->ipv6_pt                 = 0;
    ipv6_short_key->ipv6_pt_valid           = 0;
    ipv6_short_key->dscp                    = 0;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv6_short_key->dscp_mask                    = 0;
#endif
    ipv6_short_key->dscp_valid              = 0;
    ipv6_short_key->l4_flag                 = 0;
    ipv6_short_key->l4_flag_valid           = 0;
    ipv6_short_key->special_pkt_code        = 0;
    ipv6_short_key->special_pkt_code_valid  = 0;
    ipv6_short_key->top_vlan_id             = 0;
    ipv6_short_key->top_is_svlan            = 0;
    ipv6_short_key->top_vlan_valid          = 0;
    ipv6_short_key->top_dot1p               = 0;
    ipv6_short_key->top_dot1p_valid         = 0;
    ipv6_short_key->vlan_number             = 0;
    ipv6_short_key->vlan_number_vld         = 0;
    ipv6_short_key->l4_dst_port             = 0;
    ipv6_short_key->l4_src_port             = 0;
    ipv6_short_key->l4_ctrl                 = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    ipv6_short_key->flow_lab                = 0;
    ipv6_short_key->flow_lab_valid          = 0;
    ipv6_short_key->fields_is_or            = 0;
    ipv6_short_key->precedence              = priority;

    ipv6_short_key_mask->s.valid                   = 1;
    ipv6_short_key_mask->s.is_ipv4_ipv6_key        = 1;
    ipv6_short_key_mask->s.ipv6_ecn                = 1;
    ipv6_short_key_mask->s.ipv6_ecn_valid          = 1;
    ipv6_short_key_mask->s.ipv6_da                 = 1;
    ipv6_short_key_mask->s.ipv6_sa                 = 1;
    ipv6_short_key_mask->s.ipv6_addr_mask          = 1;
    ipv6_short_key_mask->s.ipv6_ctrl               = 1;
    ipv6_short_key_mask->s.ipv6_pt                 = 1;
    ipv6_short_key_mask->s.ipv6_pt_valid           = 1;
    ipv6_short_key_mask->s.dscp                    = 1;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ipv6_short_key_mask->s.dscp_mask               = 1;
#endif
    ipv6_short_key_mask->s.dscp_valid              = 1;
    ipv6_short_key_mask->s.l4_flag                 = 1;
    ipv6_short_key_mask->s.l4_flag_valid           = 1;
    ipv6_short_key_mask->s.special_pkt_code        = 1;
    ipv6_short_key_mask->s.special_pkt_code_valid  = 1;
    ipv6_short_key_mask->s.top_vlan_id             = 1;
    ipv6_short_key_mask->s.top_is_svlan            = 1;
    ipv6_short_key_mask->s.top_vlan_valid          = 1;
    ipv6_short_key_mask->s.top_dot1p               = 1;
    ipv6_short_key_mask->s.top_dot1p_valid         = 1;
    ipv6_short_key_mask->s.vlan_number             = 1;
    ipv6_short_key_mask->s.vlan_number_vld         = 1;
    ipv6_short_key_mask->s.l4_dst_port             = 1;
    ipv6_short_key_mask->s.l4_src_port             = 1;
    ipv6_short_key_mask->s.l4_ctrl                 = 1;
    ipv6_short_key_mask->s.flow_lab                = 1;
    ipv6_short_key_mask->s.flow_lab_valid          = 1;
    ipv6_short_key_mask->s.fields_is_or            = 1;
    ipv6_short_key_mask->s.precedence              = 1;

    if(key_mask->src_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->src_port,&vlan_port_control);
    }else if(key_mask->dest_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->dest_port,&vlan_port_control);
    }

    if(key_mask->l2){

        if(key_mask->l2_mask.vlan_otag_mask.pri){
            ipv6_short_key->top_dot1p = key->l2.vlan_otag.vlan_min.pri;
            ipv6_short_key->top_dot1p_valid = TRUE;
        }
        if(key_mask->l2_mask.vlan_otag_mask.vid){
            ipv6_short_key->top_vlan_id     =  key->l2.vlan_otag.vlan_min.vid;
            ipv6_short_key->top_vlan_valid  =  TRUE;
            ipv6_short_key->top_is_svlan    = vlan_port_control.outer_tpid;
        }

        if(key_mask->l2_mask.vlan_count){
            ipv6_short_key->vlan_number_vld = TRUE;
            ipv6_short_key->vlan_number   = key->l2.vlan_count;
        }
    }
    if(key_mask->ip){
        if(key_mask->ip_mask.ip_da && key_mask->ip_mask.ip_sa){
            memcpy(ipv6_short_key->ipv6_da,key->ip.ip_da.ip_addr.ipv6_addr, 16);
            memcpy(ipv6_short_key->ipv6_sa,key->ip.ip_sa.ip_addr.ipv6_addr, 16);
            ipv6_short_key->ipv6_addr_mask |= (128 - key->ip.ip_da.addr_len);
            ipv6_short_key->ipv6_addr_mask |= ((128 - key->ip.ip_sa.addr_len)<<7);
            ipv6_short_key->ipv6_ctrl        = AAL_L2_CLS_IPV6_CTRL_ALL_MATCH;
        }else if(key_mask->ip_mask.ip_da){
            memcpy(ipv6_short_key->ipv6_sa,key->ip.ip_da.ip_addr.ipv6_addr, 16);
            if(key_mask->ip_mask.ip_da_max){
                memcpy(ipv6_short_key->ipv6_da,key->ip.ip_da_max.ip_addr.ipv6_addr, 16);
                ipv6_short_key->ipv6_ctrl = AAL_L2_CLS_IP_CTRL_DIP_RANGE;
            }else{
                memcpy(ipv6_short_key->ipv6_da,key->ip.ip_da.ip_addr.ipv6_addr, 16);
                ipv6_short_key->ipv6_addr_mask |= (128 - key->ip.ip_da.addr_len);
                ipv6_short_key->ipv6_addr_mask |= (0x7f<<7);
                ipv6_short_key->ipv6_ctrl        = AAL_L2_CLS_IPV6_CTRL_ALL_MATCH;
           }
        }else if(key_mask->ip_mask.ip_sa){
            memcpy(ipv6_short_key->ipv6_sa,key->ip.ip_sa.ip_addr.ipv6_addr, 16);
            if(key_mask->ip_mask.ip_sa_max){
                memcpy(ipv6_short_key->ipv6_da,key->ip.ip_sa_max.ip_addr.ipv6_addr, 16);
                ipv6_short_key->ipv6_ctrl = AAL_L2_CLS_IP_CTRL_SIP_RANGE;
            }else{
                memcpy(ipv6_short_key->ipv6_da,key->ip.ip_sa.ip_addr.ipv6_addr, 16);
                ipv6_short_key->ipv6_addr_mask |= ((128 - key->ip.ip_sa.addr_len)<<7);
                ipv6_short_key->ipv6_addr_mask |= 0x7f;
                ipv6_short_key->ipv6_ctrl        = AAL_L2_CLS_IPV6_CTRL_ALL_MATCH;
            }
        }

        if(key_mask->ip_mask.dscp){
            ipv6_short_key->dscp = key->ip.dscp;
 #if defined(CONFIG_ARCH_CORTINA_VENUS)
            ipv6_short_key->dscp_mask = key->ip.dscp_mask;
 #endif
            ipv6_short_key->dscp_valid = TRUE;
        }
        if(key_mask->ip_mask.ecn){
            ipv6_short_key->ipv6_ecn        = key->ip.ecn;
            ipv6_short_key->ipv6_ecn_valid  = TRUE;
        }
      /*  if(key_mask->ip_mask.fragment){
            ipv6_short_key->ip =  key->ip.fragment;
            ipv4_key->ip_fragment_valid = TRUE;
        }
        if(key_mask->ip_mask.have_options){
            ipv4_key->ip_option = key->ip.have_options;
            ipv4_key->ip_option_valid = TRUE;
        }*/
        if(key_mask->ip_mask.ip_protocol){
            ipv6_short_key->ipv6_pt = key->ip.ip_protocol;
            ipv6_short_key->ipv6_pt_valid = TRUE;
        }
        if(key_mask->ip_mask.flow_label){
            ipv6_short_key->flow_lab = key->ip.flow_label;
            ipv6_short_key->flow_lab_valid = TRUE;
        }
        /*if(key_mask.ip_mask.ip_version){
            ipv4_key->ethernet_type_vld = TRUE;
            if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                ret = __cls_ethertype_encode_find(device_id,__CLS_IPV4_ETH_TYPE,&eth_type_index);
                if(ret != CA_E_OK){
                    return ret;
                }
            }
            if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                ret = __cls_ethertype_encode_find(device_id,__CLS_IPV6_ETH_TYPE,&eth_type_index);
                if(ret != CA_E_OK){
                    return ret;
                }
            }
            ipv4_key->ethernet_type = eth_type_index;
        }*/

        }
        if(key_mask->l4){
            if( key_mask->l4_mask.l4_valid){
                ipv6_short_key->l4_dst_port = 0xFFFF;
                ipv6_short_key->l4_src_port = 0;
                ipv6_short_key->l4_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
            }
            if((key_mask->l4_mask.src_port)&&(key_mask->l4_mask.dst_port)){
                ipv6_short_key->l4_src_port  = key->l4.src_port.min;
                ipv6_short_key->l4_dst_port = key->l4.dst_port.min;
                ipv6_short_key->l4_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_ALL_MATCH;
            }else if( key_mask->l4_mask.dst_port){
                ipv6_short_key->l4_dst_port = key->l4.dst_port.max;
                ipv6_short_key->l4_src_port = key->l4.dst_port.min;
                ipv6_short_key->l4_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_DP_RANGE;
            }else if( key_mask->l4_mask.src_port){
                ipv6_short_key->l4_dst_port = key->l4.src_port.max;
                ipv6_short_key->l4_src_port = key->l4.src_port.min;
                ipv6_short_key->l4_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
            }
            if(key_mask->l4_mask.tcp_flags){
                ipv6_short_key->l4_flag = key->l4.tcp_flags;
                ipv6_short_key->l4_flag_valid = TRUE;
            }
        }
        return ret;
}


ca_status_t __l2_cls_l2r_key_get(
    ca_device_id_t                         device_id,
    ca_uint32_t                          priority,
    ca_classifier_key_t                 *key,
    ca_classifier_key_mask_t            *key_mask,
    aal_l2_cls_l2r_key_mask_t           *l2r_key_mask,
    aal_l2_cls_l2r_key_t                *l2r_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t eth_type_index;
    ca_vlan_port_control_t vlan_port_control;
    ca_uint32_t  ether_type;

    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));

    if((key == NULL) || (key_mask == NULL) ||(priority >7)||
        (l2r_key_mask == NULL) ||(l2r_key == NULL)){
        return CA_E_PARAM;
    }

    l2r_key_mask->s.mac_da             = 1  ;
    l2r_key_mask->s.mac_sa             = 1  ;
    l2r_key_mask->s.mac_field_ctrl     = 1  ;
    l2r_key_mask->s.top_vlan_id        = 1  ;
    l2r_key_mask->s.top_is_svlan       = 1  ;
    l2r_key_mask->s.inner_vlan_id      = 1  ;
    l2r_key_mask->s.inner_is_svlan     = 1  ;
    l2r_key_mask->s.vlan_field_ctrl    = 1  ;
    l2r_key_mask->s.top_dot1p          = 1  ;
    l2r_key_mask->s.inner_dot1p        = 1  ;
    l2r_key_mask->s.dot1p_ctrl         = 1  ;
    l2r_key_mask->s.vlan_number        = 1  ;
    l2r_key_mask->s.vlan_number_vld    = 1  ;
    l2r_key_mask->s.ethernet_type      = 1  ;
    l2r_key_mask->s.ethernet_type_vld  = 1  ;
    l2r_key_mask->s.l4_src_port        = 1  ;
    l2r_key_mask->s.l4_dst_port        = 1  ;
    l2r_key_mask->s.l4_port_field_ctrl = 1  ;
    l2r_key_mask->s.field_is_or        = 1  ;
    l2r_key_mask->s.precedence         = 1  ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    l2r_key_mask->s.inner_dei_vld       = 1  ;
    l2r_key_mask->s.top_dei_vld         = 1  ;
    l2r_key_mask->s.top_dei             = 1  ;
    l2r_key_mask->s.inner_dei           = 1  ;
#endif
    l2r_key->mac_field_ctrl     = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC  ;
    l2r_key->top_vlan_id        = 0  ;
    l2r_key->top_is_svlan       = 0  ;
    l2r_key->inner_vlan_id      = 0  ;
    l2r_key->inner_is_svlan     = 0  ;
    l2r_key->vlan_field_ctrl    = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN  ;
    l2r_key->top_dot1p          = 0  ;
    l2r_key->inner_dot1p        = 0  ;
    l2r_key->dot1p_ctrl         = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    l2r_key->vlan_number        = 0  ;
    l2r_key->vlan_number_vld    = 0  ;
    l2r_key->ethernet_type      = 0  ;
    l2r_key->ethernet_type_vld  = 0  ;
    l2r_key->l4_src_port        = 0  ;
    l2r_key->l4_dst_port        = 0  ;
    l2r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT  ;
    l2r_key->field_is_or        = 0  ;
    l2r_key->precedence         = priority  ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    l2r_key->inner_dei_vld      = 0;
    l2r_key->top_dei_vld        = 0;
    l2r_key->top_dei            = 0;
    l2r_key->inner_dei          = 0;
#endif
    if(key_mask->src_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->src_port,&vlan_port_control);
    }else if(key_mask->dest_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->dest_port,&vlan_port_control);
    }

    if(CA_E_OK != ret){
        printk("%s,ca_l2_vlan_port_control_get() failed,%d \n",__FUNCTION__,__LINE__ );
        goto end;
    }

    if(key_mask->l2){
            if((key_mask->l2_mask.mac_da)&&(key_mask->l2_mask.mac_sa)){
                __l2_key_mac_exchange(l2r_key->mac_da, key->l2.mac_da.mac_min);
                __l2_key_mac_exchange(l2r_key->mac_sa, key->l2.mac_sa.mac_min);
                l2r_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_ALL_MATCH;
            }else if(key_mask->l2_mask.mac_da){
                __l2_key_mac_exchange(l2r_key->mac_da, key->l2.mac_da.mac_max);
                __l2_key_mac_exchange(l2r_key->mac_sa, key->l2.mac_da.mac_min);
                l2r_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_DA_RANGE;
            }else if(key_mask->l2_mask.mac_sa){
                __l2_key_mac_exchange(l2r_key->mac_da, key->l2.mac_sa.mac_max);
                __l2_key_mac_exchange(l2r_key->mac_sa, key->l2.mac_sa.mac_min);
                l2r_key->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_SA_RANGE;
            }

            if(key_mask->l2_mask.vlan_otag && key_mask->l2_mask.vlan_itag){
                if(key_mask->l2_mask.vlan_otag_mask.pri && key_mask->l2_mask.vlan_itag_mask.pri){
                    l2r_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                    l2r_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                    l2r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_ALL_MATCH;
                }else if(key_mask->l2_mask.vlan_otag_mask.pri){
                    l2r_key->top_dot1p          = key->l2.vlan_otag.vlan_max.pri;
                    l2r_key->inner_dot1p        = key->l2.vlan_otag.vlan_min.pri;
                    l2r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;
                }else if(key_mask->l2_mask.vlan_itag_mask.pri){
                    l2r_key->top_dot1p          = key->l2.vlan_itag.vlan_max.pri;
                    l2r_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                    l2r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;
                }
                if(key_mask->l2_mask.vlan_otag_mask.vid && key_mask->l2_mask.vlan_itag_mask.vid){
                    l2r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                    l2r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                    if(__CLS_PORT_TYPE_FOR_SATURN){
                          l2r_key->top_is_svlan = 1;
                      }else{
                          l2r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                      }
                    l2r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                    l2r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_ALL_MATCH;
                }else if(key_mask->l2_mask.vlan_otag_mask.vid){
                    l2r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_max.vid;
                    l2r_key->inner_vlan_id          = key->l2.vlan_otag.vlan_min.vid;
                    if(__CLS_PORT_TYPE_FOR_SATURN){
                          l2r_key->top_is_svlan = 1;
                      }else{
                          l2r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                      }
                    l2r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
                }else if(key_mask->l2_mask.vlan_itag_mask.vid){
                    l2r_key->top_vlan_id            =  key->l2.vlan_itag.vlan_max.vid;
                    l2r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                    l2r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                    l2r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
                }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
                if(key_mask->l2_mask.vlan_otag_mask.dei){
                    l2r_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                    l2r_key->top_dei_vld = TRUE;
                }
                if(key_mask->l2_mask.vlan_itag_mask.dei){
                    l2r_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                    l2r_key->inner_dei_vld = TRUE;
                }
#endif               
            }else if(key_mask->l2_mask.vlan_otag){
                if(key_mask->l2_mask.vlan_otag_mask.pri){
                    l2r_key->top_dot1p          = key->l2.vlan_otag.vlan_max.pri;
                    l2r_key->inner_dot1p        = key->l2.vlan_otag.vlan_min.pri;
                    l2r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;
                }
                if(key_mask->l2_mask.vlan_otag_mask.vid){
                    l2r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_max.vid;
                    l2r_key->inner_vlan_id          = key->l2.vlan_otag.vlan_min.vid;
                    if((mirror_vlan == l2r_key->inner_vlan_id)&&
                        (l2r_key->inner_vlan_id == l2r_key->top_vlan_id)){
                        __g_rule_is_use_for_mirror = TRUE;
                    }
                    if(__CLS_PORT_TYPE_FOR_SATURN){
                          l2r_key->top_is_svlan = 1;
                      }else{
                          l2r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                      }
                    l2r_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
                }
#if defined(CONFIG_ARCH_CORTINA_VENUS)    
                if(key_mask->l2_mask.vlan_otag_mask.dei){
                    l2r_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                    l2r_key->top_dei_vld = TRUE;
                }
#endif
            }else if(key_mask->l2_mask.vlan_itag){
                if(key_mask->l2_mask.vlan_itag_mask.pri){
                    l2r_key->top_dot1p          = key->l2.vlan_itag.vlan_max.pri;
                    l2r_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                    l2r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;
                }
                if(key_mask->l2_mask.vlan_itag_mask.vid){
                    l2r_key->top_vlan_id            =  key->l2.vlan_itag.vlan_max.vid;
                    l2r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                    l2r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                    l2r_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
                }
 #if defined(CONFIG_ARCH_CORTINA_VENUS)               
                if(key_mask->l2_mask.vlan_itag_mask.dei){
                    l2r_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                    l2r_key->inner_dei_vld = TRUE;
                }
 #endif               
            }
            if(key_mask->l2_mask.ethertype){
                ret = __cls_ethertype_encode_find(device_id,key->l2.ethertype,&eth_type_index);
                if(CA_E_OK != ret){
                    goto end;
                }
                
                l2r_key->ethernet_type = eth_type_index;
                l2r_key->ethernet_type_vld          = TRUE;
                cls_printf("l2r_key->ethernet_type_vld = %d\n", l2r_key->ethernet_type_vld);
            }
            if(key_mask->l2_mask.vlan_count){
                l2r_key->vlan_number_vld = TRUE;
                l2r_key->vlan_number   = key->l2.vlan_count;
            }
        }
        if(key_mask->ip){
            l2r_key->ethernet_type_vld = TRUE;
            if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                ether_type = __CLS_IPV4_ETH_TYPE;
            }
            if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                ether_type = __CLS_IPV6_ETH_TYPE;
            }
            ret = __cls_ethertype_encode_find(device_id,ether_type,&eth_type_index);
            l2r_key->ethernet_type = eth_type_index;
        }

        if(key_mask->l4){
            if(key_mask->l4_mask.l4_valid){
                l2r_key->l4_dst_port = 0xFFFF;
                l2r_key->l4_src_port = 0;
                l2r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
            }
            if((key_mask->l4_mask.src_port)&&(key_mask->l4_mask.dst_port)){
                l2r_key->l4_src_port = key->l4.src_port.min;
                l2r_key->l4_dst_port = key->l4.dst_port.min;
                l2r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_ALL_MATCH;
            }else if( key_mask->l4_mask.dst_port){
                l2r_key->l4_dst_port = key->l4.dst_port.max;
                l2r_key->l4_src_port = key->l4.dst_port.min;
                l2r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_DP_RANGE;
            }else if( key_mask->l4_mask.src_port){
                l2r_key->l4_dst_port = key->l4.src_port.max;
                l2r_key->l4_src_port = key->l4.src_port.min;
                l2r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
            }
        }
end:
        return ret;
}

ca_status_t __l2_cls_l3r_spcl_pkt_key_get(    
        ca_device_id_t                         device_id,
        ca_uint32_t                          priority,  
        aal_l2_specpkt_type_t                special_packet_type,
        aal_l2_cls_l3r_key_mask_t           *l3r_key_mask,
        aal_l2_cls_l3r_key_t                *l3r_key
    )
{
    if((priority >7)||(l3r_key_mask == NULL) ||(l3r_key == NULL) 
        ||(special_packet_type > AAL_L2_SPECPKT_TYPE_MAX)){
        return CA_E_PARAM;
    }
    l3r_key_mask->s.ip_da               = 1;
     l3r_key_mask->s.ip_sa               = 1;
     l3r_key_mask->s.ip_ctrl             = 1;
     l3r_key_mask->s.ip_pt               = 1;
     l3r_key_mask->s.ip_pt_valid         = 1;
     l3r_key_mask->s.dscp                = 1;
     l3r_key_mask->s.dscp_valid          = 1;
     l3r_key_mask->s.has_ipop            = 1;
     l3r_key_mask->s.has_ipop_valid      = 1;
     l3r_key_mask->s.ipv4_fragment       = 1;
     l3r_key_mask->s.ipv4_fragment_valid = 1;
     l3r_key_mask->s.l4_flag             = 1;
     l3r_key_mask->s.l4_flag_valid       = 1;
     l3r_key_mask->s.special_pkt_code    = 1;
     l3r_key_mask->s.special_pkt_code_valid  = 1;
     l3r_key_mask->s.top_vlan_id         = 1;
     l3r_key_mask->s.top_is_svlan        = 1;
     l3r_key_mask->s.inner_vlan_id       = 1;
     l3r_key_mask->s.inner_is_svlan      = 1;
     l3r_key_mask->s.vlan_field_ctrl     = 1;
     l3r_key_mask->s.top_dot1p           = 1;
     l3r_key_mask->s.inner_dot1p         = 1;
     l3r_key_mask->s.dot1p_ctrl          = 1;
     l3r_key_mask->s.vlan_number         = 1;
     l3r_key_mask->s.vlan_number_vld     = 1;
     l3r_key_mask->s.ethernet_type       = 1;
     l3r_key_mask->s.ethernet_type_vld   = 1;
     l3r_key_mask->s.l4_src_port         = 1;
     l3r_key_mask->s.l4_dst_port         = 1;
     l3r_key_mask->s.l4_port_field_ctrl  = 1;
     l3r_key_mask->s.field_is_or         = 1;
     l3r_key_mask->s.precedence          = 1;
     l3r_key_mask->s.ecn                 = 1  ;
     l3r_key_mask->s.ecn_valid           = 1  ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)     
    l3r_key_mask->s.inner_dei_vld         = 1  ;
    l3r_key_mask->s.top_dei_vld         = 1  ;
#endif
     l3r_key->ecn                = 0;
     l3r_key->ecn_valid          = 0;
     l3r_key->ip_da              = 0;
     l3r_key->ip_sa              = 0;
     l3r_key->ip_ctrl            = AAL_L2_CLS_IP_CTRL_MASK_IP_ADDR;
     l3r_key->ip_pt              = 0;
     l3r_key->ip_pt_valid        = 0;
     l3r_key->dscp               = 0;
     l3r_key->dscp_valid         = 0;
     l3r_key->has_ipop           = 0;
     l3r_key->has_ipop_valid     = 0;
     l3r_key->ipv4_fragment      = 0;
     l3r_key->ipv4_fragment_valid= 0;
     l3r_key->l4_flag            = 0;
     l3r_key->l4_flag_valid      = 0;
     l3r_key->special_pkt_code   = special_packet_type;
     l3r_key->special_pkt_code_valid  = 1;
     l3r_key->top_vlan_id        = 0;
     l3r_key->top_is_svlan       = 0;
     l3r_key->inner_vlan_id      = 0;
     l3r_key->inner_is_svlan     = 0;
     l3r_key->vlan_field_ctrl    = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
     l3r_key->top_dot1p          = 0;
     l3r_key->inner_dot1p        = 0;
     l3r_key->dot1p_ctrl         = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
     l3r_key->vlan_number        = 0;
     l3r_key->vlan_number_vld    = 0;
     l3r_key->ethernet_type      = 0;
     l3r_key->ethernet_type_vld  = 0;
     l3r_key->l4_src_port        = 0;
     l3r_key->l4_dst_port        = 0;
     l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
     l3r_key->field_is_or        = 0;
     l3r_key->precedence         = priority;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
     l3r_key->inner_dei_vld = 0;
     l3r_key->top_dei_vld = 0;
#endif
     return CA_E_OK;
}
ca_status_t __l2_cls_l3r_key_get(
    ca_device_id_t                         device_id,
    ca_uint32_t                          priority,
    ca_classifier_key_t                 *key,
    ca_classifier_key_mask_t            *key_mask,
    aal_l2_cls_l3r_key_mask_t           *l3r_key_mask,
    aal_l2_cls_l3r_key_t                *l3r_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t eth_type_index;
    ca_vlan_port_control_t vlan_port_control;
    ca_uint32_t ether_type;

    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));

    if((key == NULL) || (key_mask == NULL) ||(priority >7)||
        (l3r_key_mask == NULL) ||(l3r_key == NULL)){
        return CA_E_PARAM;
    }

    l3r_key_mask->s.ip_da               = 1;
    l3r_key_mask->s.ip_sa               = 1;
    l3r_key_mask->s.ip_ctrl             = 1;
    l3r_key_mask->s.ip_pt               = 1;
    l3r_key_mask->s.ip_pt_valid         = 1;
    l3r_key_mask->s.dscp                = 1;
    l3r_key_mask->s.dscp_valid          = 1;
    l3r_key_mask->s.has_ipop            = 1;
    l3r_key_mask->s.has_ipop_valid      = 1;
    l3r_key_mask->s.ipv4_fragment       = 1;
    l3r_key_mask->s.ipv4_fragment_valid = 1;
    l3r_key_mask->s.l4_flag             = 1;
    l3r_key_mask->s.l4_flag_valid       = 1;
    l3r_key_mask->s.special_pkt_code    = 1;
    l3r_key_mask->s.special_pkt_code_valid  = 1;
    l3r_key_mask->s.top_vlan_id         = 1;
    l3r_key_mask->s.top_is_svlan        = 1;
    l3r_key_mask->s.inner_vlan_id       = 1;
    l3r_key_mask->s.inner_is_svlan      = 1;
    l3r_key_mask->s.vlan_field_ctrl     = 1;
    l3r_key_mask->s.top_dot1p           = 1;
    l3r_key_mask->s.inner_dot1p         = 1;
    l3r_key_mask->s.dot1p_ctrl          = 1;
    l3r_key_mask->s.vlan_number         = 1;
    l3r_key_mask->s.vlan_number_vld     = 1;
    l3r_key_mask->s.ethernet_type       = 1;
    l3r_key_mask->s.ethernet_type_vld   = 1;
    l3r_key_mask->s.l4_src_port         = 1;
    l3r_key_mask->s.l4_dst_port         = 1;
    l3r_key_mask->s.l4_port_field_ctrl  = 1;
    l3r_key_mask->s.field_is_or         = 1;
    l3r_key_mask->s.precedence          = 1;
    l3r_key_mask->s.ecn                 = 1  ;
    l3r_key_mask->s.ecn_valid           = 1  ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    l3r_key_mask->s.inner_dei_vld       = 1  ;
    l3r_key_mask->s.top_dei_vld         = 1  ;
    l3r_key_mask->s.top_dei             = 1  ;
    l3r_key_mask->s.inner_dei           = 1  ;
#endif
    l3r_key->ecn                = 0;
    l3r_key->ecn_valid          = 0;
    l3r_key->ip_da              = 0;
    l3r_key->ip_sa              = 0;
    l3r_key->ip_ctrl            = AAL_L2_CLS_IP_CTRL_MASK_IP_ADDR;
    l3r_key->ip_pt              = 0;
    l3r_key->ip_pt_valid        = 0;
    l3r_key->dscp               = 0;
    l3r_key->dscp_valid         = 0;
    l3r_key->has_ipop           = 0;
    l3r_key->has_ipop_valid     = 0;
    l3r_key->ipv4_fragment      = 0;
    l3r_key->ipv4_fragment_valid= 0;
    l3r_key->l4_flag            = 0;
    l3r_key->l4_flag_valid      = 0;
    l3r_key->special_pkt_code   = 0;
    l3r_key->special_pkt_code_valid  = 0;
    l3r_key->top_vlan_id        = 0;
    l3r_key->top_is_svlan       = 0;
    l3r_key->inner_vlan_id      = 0;
    l3r_key->inner_is_svlan     = 0;
    l3r_key->vlan_field_ctrl    = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    l3r_key->top_dot1p          = 0;
    l3r_key->inner_dot1p        = 0;
    l3r_key->dot1p_ctrl         = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    l3r_key->vlan_number        = 0;
    l3r_key->vlan_number_vld    = 0;
    l3r_key->ethernet_type      = 0;
    l3r_key->ethernet_type_vld  = 0;
    l3r_key->l4_src_port        = 0;
    l3r_key->l4_dst_port        = 0;
    l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    l3r_key->field_is_or        = 0;
    l3r_key->precedence         = priority;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    l3r_key->inner_dei_vld  = 0;
    l3r_key->top_dei_vld    = 0;
    l3r_key->top_dei        = 0;
    l3r_key->inner_dei      = 0;
#endif
    if(key_mask->src_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->src_port,&vlan_port_control);
    }else if(key_mask->dest_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->dest_port,&vlan_port_control);
    }


    if(CA_E_OK != ret){
       cls_printf("%s,ca_l2_vlan_port_control_get() failed,%d \n",__FUNCTION__,__LINE__ );
       goto end;
    }

    if(key_mask->l2){
        if(key_mask->l2_mask.vlan_otag && key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_otag_mask.pri && key_mask->l2_mask.vlan_itag_mask.pri){
                l3r_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                l3r_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                l3r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_ALL_MATCH;
            }else if(key_mask->l2_mask.vlan_otag_mask.pri){
                l3r_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                l3r_key->inner_dot1p        = key->l2.vlan_otag.vlan_max.pri;
                l3r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;
            }else if(key_mask->l2_mask.vlan_itag_mask.pri){
                l3r_key->top_dot1p          = key->l2.vlan_itag.vlan_min.pri;
                l3r_key->inner_dot1p        = key->l2.vlan_itag.vlan_max.pri;
                l3r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid && key_mask->l2_mask.vlan_itag_mask.vid){
                l3r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                l3r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_min.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                      l3r_key->top_is_svlan = 1;
                  }else{
                      l3r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                  }
                l3r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                l3r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_ALL_MATCH;
            }else if(key_mask->l2_mask.vlan_otag_mask.vid){
                l3r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                l3r_key->inner_vlan_id          = key->l2.vlan_otag.vlan_max.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                      l3r_key->top_is_svlan = 1;
                  }else{
                      l3r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                  }
                l3r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
            }else if(key_mask->l2_mask.vlan_itag_mask.vid){
                l3r_key->top_vlan_id            =  key->l2.vlan_itag.vlan_min.vid;
                l3r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_max.vid;
                l3r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                l3r_key->vlan_field_ctrl        =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                l3r_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                l3r_key->top_dei_vld = TRUE;
            }
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                l3r_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                l3r_key->inner_dei_vld = TRUE;
            }
#endif   
        }else if(key_mask->l2_mask.vlan_otag){
            if(key_mask->l2_mask.vlan_otag_mask.pri){
                l3r_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                l3r_key->inner_dot1p        = key->l2.vlan_otag.vlan_max.pri;
                l3r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_TOP_RANGE;
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid){
                l3r_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                l3r_key->inner_vlan_id          = key->l2.vlan_otag.vlan_max.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    l3r_key->top_is_svlan = 1;
                }else{
                    l3r_key->top_is_svlan           = vlan_port_control.outer_tpid;
                }
                l3r_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                l3r_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                l3r_key->top_dei_vld = TRUE;
            }
#endif
        }else if(key_mask->l2_mask.vlan_itag){
            if(key_mask->l2_mask.vlan_itag_mask.pri){
                l3r_key->top_dot1p          = key->l2.vlan_itag.vlan_min.pri;
                l3r_key->inner_dot1p        = key->l2.vlan_itag.vlan_min.pri;
                l3r_key->dot1p_ctrl         =  AAL_L2_CLS_DOT1P_FIELD_CTRL_INNER_RANGE;
            }
            if(key_mask->l2_mask.vlan_itag_mask.vid){
                l3r_key->top_vlan_id            =  key->l2.vlan_itag.vlan_min.vid;
                l3r_key->inner_vlan_id          = key->l2.vlan_itag.vlan_max.vid;
                l3r_key->inner_is_svlan           = vlan_port_control.inner_tpid;
                l3r_key->vlan_field_ctrl         =  AAL_L2_CLS_VLAN_FIELD_CTRL_INNER_RANGE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_itag_mask.dei){
                l3r_key->inner_dei = key->l2.vlan_itag.vlan_min.dei;
                l3r_key->inner_dei_vld = TRUE;
            }
#endif
        }
        if(key_mask->l2_mask.ethertype){
            l3r_key->ethernet_type_vld          = TRUE;
            ret = __cls_ethertype_encode_find(device_id,key->l2.ethertype,&eth_type_index);
            if(CA_E_OK != ret){
                goto end;
            }
            l3r_key->ethernet_type = eth_type_index;
        }
        if(key_mask->l2_mask.vlan_count){
            l3r_key->vlan_number_vld = TRUE;
            l3r_key->vlan_number   = key->l2.vlan_count;
        }
    }

    if(key_mask->ip){
        if(key_mask->ip_mask.ip_da && key_mask->ip_mask.ip_sa){
            l3r_key->ip_da = key->ip.ip_da.ip_addr.ipv4_addr;
            l3r_key->ip_sa = key->ip.ip_sa.ip_addr.ipv4_addr;
            l3r_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_ALL_MATCH;
        }else if(key_mask->ip_mask.ip_da){
            if(key_mask->ip_mask.ip_da_max){
                l3r_key->ip_da =  key->ip.ip_da_max.ip_addr.ipv4_addr;
            }else{
                l3r_key->ip_da =  key->ip.ip_da.ip_addr.ipv4_addr;
            }
            l3r_key->ip_sa = key->ip.ip_da.ip_addr.ipv4_addr;
            l3r_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_DIP_RANGE;
        }else if(key_mask->ip_mask.ip_sa){
            if(key_mask->ip_mask.ip_sa_max){
                l3r_key->ip_da = key->ip.ip_sa_max.ip_addr.ipv4_addr;
            }else{
                l3r_key->ip_da = key->ip.ip_sa.ip_addr.ipv4_addr;
            }
            l3r_key->ip_sa = key->ip.ip_sa.ip_addr.ipv4_addr;
            l3r_key->ip_ctrl = AAL_L2_CLS_IP_CTRL_SIP_RANGE;
        }

        if(key_mask->ip_mask.dscp){
            l3r_key->dscp = key->ip.dscp;
            l3r_key->dscp_valid = TRUE;
        }

        if(key_mask->ip_mask.fragment){
            l3r_key->ipv4_fragment =  (key->ip.fragment > 1);
            l3r_key->ipv4_fragment_valid = TRUE;
        }
        if(key_mask->ip_mask.have_options){
            l3r_key->has_ipop = key->ip.have_options;
            l3r_key->has_ipop_valid = TRUE;
        }
        if(key_mask->ip_mask.ip_protocol){
            l3r_key->ip_pt= key->ip.ip_protocol;
            l3r_key->ip_pt_valid = TRUE;
        }
        if(key_mask->ip_mask.ecn){
            l3r_key->ecn = key->ip.ecn;
            l3r_key->ecn_valid = TRUE;
        }
        if(key_mask->ip_mask.ip_version){
            l3r_key->ethernet_type_vld = TRUE;
            if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                ether_type = __CLS_IPV4_ETH_TYPE;
            }
            if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                ether_type = __CLS_IPV6_ETH_TYPE;
            }
            ret = __cls_ethertype_encode_find(device_id,ether_type,&eth_type_index);

            l3r_key->ethernet_type = eth_type_index;
        }

    }

    if(key_mask->l4){
        if(key_mask->l4_mask.l4_valid){
            l3r_key->l4_dst_port = 0xFFFF;
            l3r_key->l4_src_port = 0;
            l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
        }
        if((key_mask->l4_mask.src_port)&&(key_mask->l4_mask.dst_port)){
            l3r_key->l4_src_port = key->l4.src_port.min;
            l3r_key->l4_dst_port = key->l4.dst_port.min;
            l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_ALL_MATCH;
        }else if( key_mask->l4_mask.dst_port){
            l3r_key->l4_dst_port = key->l4.dst_port.max;
            l3r_key->l4_src_port = key->l4.dst_port.min;
            l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_DP_RANGE;
        }else if( key_mask->l4_mask.src_port){
            l3r_key->l4_dst_port = key->l4.src_port.max;
            l3r_key->l4_src_port = key->l4.src_port.min;
            l3r_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
        }
        if(key_mask->l4_mask.tcp_flags){
            l3r_key->l4_flag = key->l4.tcp_flags;
            l3r_key->l4_flag_valid = TRUE;
        }
    }
end:
        return ret;
}

ca_status_t __l2_cls_cmr_key_get(
    ca_device_id_t                         device_id,
    ca_uint32_t                          priority,
    ca_classifier_key_t                 *key,
    ca_classifier_key_mask_t            *key_mask,
    aal_l2_cls_cmr_key_mask_t           *cmr_key_mask,
    aal_l2_cls_cmr_key_t                *cmr_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t eth_type_index;
    ca_vlan_port_control_t vlan_port_control;
    ca_uint32_t ether_type;

    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));

    if((key == NULL) || (key_mask == NULL) ||(priority >7)||
        (cmr_key_mask == NULL) ||(cmr_key == NULL)){
        return CA_E_PARAM;
    }

    cmr_key_mask->s.mac_addr  = 1;
    cmr_key_mask->s.mac_addr_is_sa  = 1;
    cmr_key_mask->s.mac_addr_valid  = 1;
    cmr_key_mask->s.ip_addr  = 1;
    cmr_key_mask->s.ip_addr_is_sa  = 1;
    cmr_key_mask->s.ip_addr_valid  = 1;
    cmr_key_mask->s.ip_pt  = 1;
    cmr_key_mask->s.ip_pt_valid  = 1;
    cmr_key_mask->s.dscp  = 1;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    cmr_key_mask->s.dscp_mask       = 1;
#endif
    cmr_key_mask->s.dscp_valid  = 1;
    cmr_key_mask->s.has_ipop  = 1;
    cmr_key_mask->s.has_ipop_valid  = 1;
    cmr_key_mask->s.l4_flag  = 1;
    cmr_key_mask->s.l4_flag_valid  = 1;
    cmr_key_mask->s.ipv4_fragment  = 1;
    cmr_key_mask->s.ipv4_fragment_valid  = 1;
    cmr_key_mask->s.special_pkt_code  = 1;
    cmr_key_mask->s.special_pkt_code_valid  = 1;
    cmr_key_mask->s.top_vlan_id  = 1;
    cmr_key_mask->s.top_is_svlan  = 1;
    cmr_key_mask->s.vlan_field_valid  = 1;
    cmr_key_mask->s.top_dot1p  = 1;
    cmr_key_mask->s.dot1p_valid  = 1;
    cmr_key_mask->s.vlan_number  = 1;
    cmr_key_mask->s.vlan_number_vld  = 1;
    cmr_key_mask->s.ethernet_type  = 1;
    cmr_key_mask->s.ethernet_type_vld  = 1;
    cmr_key_mask->s.l4_src_port  = 1;
    cmr_key_mask->s.l4_dst_port  = 1;
    cmr_key_mask->s.l4_port_field_ctrl  = 1;
    cmr_key_mask->s.field_is_or  = 1;
    cmr_key_mask->s.precedence  = 1;
    cmr_key_mask->s.ecn                 = 1  ;
    cmr_key_mask->s.ecn_valid           = 1  ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    cmr_key_mask->s.top_dei_vld         = 1  ;
    cmr_key_mask->s.top_dei             = 1  ;
#endif
    cmr_key->ecn                    = 0;
    cmr_key->ecn_valid              = 0;
    cmr_key->mac_addr_is_sa         = 0 ;
    cmr_key->mac_addr_valid         = 0 ;
    cmr_key->ip_addr                = 0 ;
    cmr_key->ip_addr_is_sa          = 0 ;
    cmr_key->ip_addr_valid          = 0 ;
    cmr_key->ip_pt                  = 0 ;
    cmr_key->ip_pt_valid            = 0 ;
    cmr_key->dscp                   = 0 ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    cmr_key->dscp_mask              = 0 ;
#endif
    cmr_key->dscp_valid             = 0 ;
    cmr_key->has_ipop               = 0 ;
    cmr_key->has_ipop_valid         = 0 ;
    cmr_key->l4_flag                = 0 ;
    cmr_key->l4_flag_valid          = 0 ;
    cmr_key->ipv4_fragment          = 0 ;
    cmr_key->ipv4_fragment_valid    = 0 ;
    cmr_key->special_pkt_code       = 0 ;
    cmr_key->special_pkt_code_valid = 0 ;
    cmr_key->top_vlan_id            = 0 ;
    cmr_key->top_is_svlan           = 0 ;
    cmr_key->vlan_field_valid       = 0 ;
    cmr_key->top_dot1p              = 0 ;
    cmr_key->dot1p_valid            = 0 ;
    cmr_key->vlan_number            = 0 ;
    cmr_key->vlan_number_vld        = 0 ;
    cmr_key->ethernet_type          = 0 ;
    cmr_key->ethernet_type_vld      = 0 ;
    cmr_key->l4_src_port            = 0 ;
    cmr_key->l4_dst_port            = 0 ;
    cmr_key->l4_port_field_ctrl     = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT ;
    cmr_key->field_is_or            = 0 ;
    cmr_key->precedence             = priority ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    cmr_key->top_dei_vld           = 0 ;
    cmr_key->top_dei               = 0 ;
#endif

    if(__CLS_PORT_TYPE_FOR_SATURN){
    }else{
    if(key_mask->src_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->src_port,&vlan_port_control);
    }else if(key_mask->dest_port){
        ret = ca_l2_vlan_port_control_get(device_id,key->dest_port,&vlan_port_control);
    }
    }


    if(CA_E_OK != ret){
        cls_printf("%s,ca_l2_vlan_port_control_get() failed,%d \n",__FUNCTION__,__LINE__ );
        goto end;
    }

   if(key_mask->l2){
        if(key_mask->l2_mask.mac_da){
                __l2_key_mac_exchange(cmr_key->mac_addr, key->l2.mac_da.mac_min);
                cmr_key->mac_addr_is_sa = FALSE;
                cmr_key->mac_addr_valid = TRUE;
            }else if(key_mask->l2_mask.mac_sa){
                __l2_key_mac_exchange(cmr_key->mac_addr, key->l2.mac_sa.mac_min);
                cmr_key->mac_addr_is_sa = TRUE;
                cmr_key->mac_addr_valid = TRUE;
            }
        if(key_mask->l2_mask.vlan_otag){
            if(key_mask->l2_mask.vlan_otag_mask.pri){
                cmr_key->top_dot1p          = key->l2.vlan_otag.vlan_min.pri;
                cmr_key->dot1p_valid         =  TRUE;
            }
            if(key_mask->l2_mask.vlan_otag_mask.vid){
                cmr_key->top_vlan_id            =  key->l2.vlan_otag.vlan_min.vid;
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    cmr_key->top_is_svlan = 1;
                }else{
                    cmr_key->top_is_svlan           = vlan_port_control.outer_tpid;
                }
                cmr_key->vlan_field_valid       = TRUE;
            }
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            if(key_mask->l2_mask.vlan_otag_mask.dei){
                cmr_key->top_dei = key->l2.vlan_otag.vlan_min.dei;
                cmr_key->top_dei_vld = TRUE;
            }
#endif
        }
        if(key_mask->l2_mask.ethertype){
            cmr_key->ethernet_type_vld          = TRUE;

            ret = __cls_ethertype_encode_find(device_id,key->l2.ethertype,&eth_type_index);
            if(CA_E_OK != ret){
                goto end;
            }
            cmr_key->ethernet_type = eth_type_index;
        }
        if(key_mask->l2_mask.vlan_count){
            cmr_key->vlan_number_vld = TRUE;
            cmr_key->vlan_number   = key->l2.vlan_count;
        }
    }

    if(key_mask->ip){
       if(key_mask->ip_mask.ip_da){
            cmr_key->ip_addr =  key->ip.ip_da.ip_addr.ipv4_addr;
            cmr_key->ip_addr_is_sa = FALSE;
            cmr_key->ip_addr_valid = TRUE;
        }else if(key_mask->ip_mask.ip_sa){
           cmr_key->ip_addr =  key->ip.ip_sa.ip_addr.ipv4_addr;
           cmr_key->ip_addr_is_sa = TRUE;
           cmr_key->ip_addr_valid = TRUE;
        }

        if(key_mask->ip_mask.dscp){
            cmr_key->dscp = key->ip.dscp;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
            cmr_key->dscp_mask = key->ip.dscp_mask;
#endif
            cmr_key->dscp_valid = TRUE;
        }

        if(key_mask->ip_mask.fragment){
            cmr_key->ipv4_fragment     =  (key->ip.fragment >1);
            cmr_key->ipv4_fragment_valid = TRUE;
        }
        if(key_mask->ip_mask.have_options){
            cmr_key->has_ipop = key->ip.have_options;
            cmr_key->has_ipop_valid = TRUE;
        }
        if(key_mask->ip_mask.ip_protocol){
            cmr_key->ip_pt= key->ip.ip_protocol;
            cmr_key->ip_pt_valid = TRUE;
        }
        if(key_mask->ip_mask.ecn){
            cmr_key->ecn = key->ip.ecn;
            cmr_key->ecn_valid = TRUE;
        }
        if(key_mask->ip_mask.ip_version){
            cmr_key->ethernet_type_vld = TRUE;
            if((key->ip.ip_version == CA_IPV4) || (key->ip.ip_version == __CLS_IP_V4)){
                ether_type = __CLS_IPV4_ETH_TYPE;
            }
            if((key->ip.ip_version == CA_IPV6) || (key->ip.ip_version == __CLS_IP_V6)){
                ether_type = __CLS_IPV6_ETH_TYPE;
            }
            ret = __cls_ethertype_encode_find(device_id,ether_type,&eth_type_index);

            cmr_key->ethernet_type = eth_type_index;
        }

    }

    if(key_mask->l4){
        if(key_mask->l4_mask.l4_valid){
            cmr_key->l4_dst_port = 0xFFFF;
            cmr_key->l4_src_port = 0;
            cmr_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
        }
        if((key_mask->l4_mask.src_port)&&(key_mask->l4_mask.dst_port)){
            cmr_key->l4_src_port = key->l4.src_port.min;
            cmr_key->l4_dst_port = key->l4.dst_port.min;
            cmr_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_ALL_MATCH;
        }else if( key_mask->l4_mask.dst_port){
            cmr_key->l4_dst_port = key->l4.dst_port.max;
            cmr_key->l4_src_port = key->l4.dst_port.min;
            cmr_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_DP_RANGE;
        }else if( key_mask->l4_mask.src_port){
            cmr_key->l4_dst_port = key->l4.src_port.max;
            cmr_key->l4_src_port = key->l4.src_port.min;
            cmr_key->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_SP_RANGE;
        }
        if(key_mask->l4_mask.tcp_flags){
            cmr_key->l4_flag = key->l4.tcp_flags;
            cmr_key->l4_flag_valid = TRUE;
        }
    }
end:
        return ret;
}

static ca_status_t __l2_cls_fib_para_get(ca_port_id_t port_id, ca_uint32_t sub_entry, ca_classifier_action_t *action,aal_l2_cls_fib_mask_t *cls_fib_mask ,aal_l2_cls_fib_t *cls_fib )
{

    ca_vlan_port_control_t vlan_port_control;
    
    memset(&vlan_port_control,0,sizeof(ca_vlan_port_control_t));
     
    CA_ASSERT_RET(action, CA_E_PARAM);
    CA_ASSERT_RET(cls_fib_mask, CA_E_PARAM);
    CA_ASSERT_RET(cls_fib, CA_E_PARAM);
    
    if(action->forward == CA_CLASSIFIER_FORWARD_PORT){
        if(CA_E_OK == ca_l2_vlan_port_control_get(0,action->dest.port,&vlan_port_control)){
            cls_fib->top_sc_ind_0 = vlan_port_control.outer_tpid;
            cls_fib->top_sc_ind_1 = vlan_port_control.outer_tpid;
            cls_fib->inner_sc_ind_0 = vlan_port_control.inner_tpid;
            cls_fib->inner_sc_ind_1 = vlan_port_control.inner_tpid;
        }else{
            cls_fib->top_sc_ind_0 = 1;
            cls_fib->top_sc_ind_1 = 1;
            cls_fib->inner_sc_ind_0 = 1;
            cls_fib->inner_sc_ind_1 = 1;
        }
    }else{
        if(CA_E_OK == ca_l2_vlan_port_control_get(0,port_id,&vlan_port_control)){
            cls_fib->top_sc_ind_0 = vlan_port_control.outer_tpid;
            cls_fib->top_sc_ind_1 = vlan_port_control.outer_tpid;
            cls_fib->inner_sc_ind_0 = vlan_port_control.inner_tpid;
            cls_fib->inner_sc_ind_1 = vlan_port_control.inner_tpid;
        }else{
            cls_fib->top_sc_ind_0 = 1;
            cls_fib->top_sc_ind_1 = 1;
            cls_fib->inner_sc_ind_0 = 1;
            cls_fib->inner_sc_ind_1 = 1;
        }
    }
    
    if(sub_entry == 0){
        cls_fib_mask->s.permit_0          = 1;
        cls_fib_mask->s.permit_valid_0    = 1;
        cls_fib_mask->s.premarked_0       = 1;
        cls_fib_mask->s.top_sc_ind_0      = 1;
        cls_fib_mask->s.top_vlan_cmd_0    = 1;
        cls_fib_mask->s.top_cmd_valid_0   = 1;
        cls_fib_mask->s.top_vlanid_0      = 1;
        cls_fib_mask->s.inner_sc_ind_0    = 1;
        cls_fib_mask->s.inner_vlan_cmd_0  = 1;
        cls_fib_mask->s.inner_cmd_valid_0 = 1;
        cls_fib_mask->s.inner_vlanid_0    = 1;
        cls_fib_mask->s.dot1p_0           = 1;
        cls_fib_mask->s.dot1p_valid_0     = 1;
        cls_fib_mask->s.dscp_0            = 1;
        cls_fib_mask->s.dscp_valid_0      = 1;
        cls_fib_mask->s.cos_0             = 1;
        cls_fib_mask->s.cos_valid_0       = 1;
        cls_fib_mask->s.dp_value_0        = 1;
        cls_fib_mask->s.dp_valid_0        = 1;
        cls_fib_mask->s.flowid_0          = 1;
        cls_fib_mask->s.mark_ena_0        = 1;
        cls_fib_mask->s.dscp_mark_down_0  = 1;
        cls_fib_mask->s.spt_mode_0        = 1;
        cls_fib_mask->s.no_drop_0         = 1;
    }else{
        cls_fib_mask->s.permit_1          = 1;
        cls_fib_mask->s.permit_valid_1    = 1;
        cls_fib_mask->s.premarked_1       = 1;
        cls_fib_mask->s.top_sc_ind_1      = 1;
        cls_fib_mask->s.top_vlan_cmd_1    = 1;
        cls_fib_mask->s.top_cmd_valid_1   = 1;
        cls_fib_mask->s.top_vlanid_1      = 1;
        cls_fib_mask->s.inner_sc_ind_1    = 1;
        cls_fib_mask->s.inner_vlan_cmd_1  = 1;
        cls_fib_mask->s.inner_cmd_valid_1 = 1;
        cls_fib_mask->s.inner_vlanid_1    = 1;
        cls_fib_mask->s.dot1p_1           = 1;
        cls_fib_mask->s.dot1p_valid_1     = 1;
        cls_fib_mask->s.dscp_1            = 1;
        cls_fib_mask->s.dscp_valid_1      = 1;
        cls_fib_mask->s.cos_1             = 1;
        cls_fib_mask->s.cos_valid_1       = 1;
        cls_fib_mask->s.dp_value_1        = 1;
        cls_fib_mask->s.dp_valid_1        = 1;
        cls_fib_mask->s.flowid_1          = 1;
        cls_fib_mask->s.mark_ena_1        = 1;
        cls_fib_mask->s.dscp_mark_down_1  = 1;
        cls_fib_mask->s.spt_mode_1        = 1;
        cls_fib_mask->s.no_drop_1         = 1;
    }


    if(action->forward == CA_CLASSIFIER_FORWARD_DENY){
        if(sub_entry == 0){
            cls_fib->permit_valid_0 = TRUE;
        }else{
            cls_fib->permit_valid_1 = TRUE;
        }
    }else if(action->forward == CA_CLASSIFIER_FORWARD_PORT){
        if(sub_entry == 0){
            cls_fib->dp_valid_0 = TRUE;
            cls_fib->dp_value_0 = PORT_ID(action->dest.port);
            cls_fib->permit_valid_0 = TRUE;
            cls_fib->permit_0 = TRUE;
        }else{
            cls_fib->dp_valid_1 = TRUE;
            cls_fib->dp_value_1 = PORT_ID(action->dest.port);
            cls_fib->permit_1 = TRUE;
            cls_fib->permit_valid_1 = TRUE;
        }
    }else{
        if(sub_entry == 0){
            cls_fib->permit_valid_0 = TRUE;
            cls_fib->permit_0 = TRUE;
        }else{
            cls_fib->permit_1 = TRUE;
            cls_fib->permit_valid_1 = TRUE;
        }
    }
    if(action->dest.no_drop){
        if(sub_entry == 0){
            cls_fib->no_drop_0 = TRUE;
        }else{
            cls_fib->no_drop_1 = TRUE;
        }
    }
    if(action->options.masks.inner_dot1p){
        if(sub_entry == 0){
            cls_fib->dot1p_valid_0 = TRUE;
            cls_fib->dot1p_0 = action->options.inner_dot1p;
        }else{
            cls_fib->dot1p_valid_1 = TRUE;
            cls_fib->dot1p_1 = action->options.inner_dot1p;
        }
    }

    if(action->options.masks.outer_dot1p){
        if(sub_entry == 0){
            cls_fib->dot1p_valid_0 = TRUE;
            cls_fib->dot1p_0 = action->options.outer_dot1p;
        }else{
            cls_fib->dot1p_valid_1 = TRUE;
            cls_fib->dot1p_1 = action->options.outer_dot1p;
        }
    }

    if(action->options.masks.dscp){
        if(sub_entry == 0){
            cls_fib->dscp_valid_0 = TRUE;
            cls_fib->dscp_0 = action->options.dscp;
        }else{
            cls_fib->dscp_valid_1 = TRUE;
            cls_fib->dscp_1 = action->options.dscp;
        }
    }
    if(action->options.masks.priority){
        if(sub_entry == 0){
            cls_fib->cos_valid_0 = TRUE;
            cls_fib->cos_0 = action->options.priority;
        }else{
            cls_fib->cos_valid_1 = TRUE;
            cls_fib->cos_1 = action->options.priority;
        }
    }
    if(action->options.masks.action_handle){
        cls_printf("%s %d action->options.action_handle.flow_id = 0x%x!!\n",__func__,__LINE__,action->options.action_handle.flow_id);
        if(sub_entry == 0){
            cls_fib->mark_ena_0 = TRUE;
            cls_fib->flowid_0 = (action->options.action_handle.flow_id & 0xFF);

#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined (CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
            if((cls_fib->flowid_0 == 0xFF)&&CA_PORT_IS_PON(action->dest.port)){
                cls_fib->mark_ena_0 = 0;
                cls_fib->flowid_0 = 0;                
                cls_fib->cos_0 = 0;
                cls_fib->cos_valid_0 = 0;
                cls_fib->dp_valid_0 = TRUE;
                cls_fib->dp_value_0 = (__g_temp_ldpid + ((action->options.action_handle.flow_id >>11)&0x1F));
            }else if(cls_fib->flowid_0 <= __CLS_PON_MAX_FLOW_ID){
                if(CA_PORT_IS_PON(action->dest.port)){                    
                    cls_fib->dp_valid_0 = TRUE;                    
                    cls_fib->cos_valid_0 = TRUE;
                    if((__g_pon_mac_mode >= ONU_PON_MAC_EPON_1G) && (__g_pon_mac_mode<=ONU_PON_MAC_NGPON2)){
                        if(__g_puc_voq_mode == 2){
                            cls_fib->dp_value_0 = AAL_LPORT_ETH_WAN;
                            cls_fib->cos_0 = (action->options.action_handle.flow_id >> 8) &0x7;
                            cls_fib->flowid_0 = ((action->options.action_handle.flow_id >>11)&0x1F) * 8
                                + cls_fib->cos_0;
                        }else{
                            cls_fib->dp_value_0 = (((action->options.action_handle.flow_id >>11)&0x1F)
                                + AAL_LPORT_LLID_GEM_INDEX_0);
                            cls_fib->cos_0 = (action->options.action_handle.flow_id >> 8) &0x7;
                        }
                    }
                }
            }
#endif
        }else{
            cls_fib->mark_ena_1 = TRUE;
            cls_fib->flowid_1 = (action->options.action_handle.flow_id & 0xff);
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined (CONFIG_ARCH_CORTINA_G3HGU)|| defined(CONFIG_ARCH_CORTINA_VENUS)
            if((cls_fib->flowid_1 == 0xFF)&&CA_PORT_IS_PON(action->dest.port)){
                cls_fib->mark_ena_1 = 0;
                cls_fib->flowid_1 = 0;  
                cls_fib->cos_1 = 0;
                cls_fib->cos_valid_1 = 0;
                cls_fib->dp_valid_1 = TRUE;
                cls_fib->dp_value_1 = (__g_temp_ldpid + ((action->options.action_handle.flow_id >>11)&0x1F));
            }else if(cls_fib->flowid_1 <= __CLS_PON_MAX_FLOW_ID){
                if(CA_PORT_IS_PON(action->dest.port)){                    
                    cls_fib->dp_valid_1 = TRUE;                    
                    cls_fib->cos_valid_1 = TRUE;
                    if((__g_pon_mac_mode >= ONU_PON_MAC_EPON_1G) && (__g_pon_mac_mode<=ONU_PON_MAC_NGPON2)){
                        if(__g_puc_voq_mode == 2){
                            cls_fib->dp_value_1 = AAL_LPORT_ETH_WAN;
                            cls_fib->cos_1 = (action->options.action_handle.flow_id >> 8) &0x7;
                            cls_fib->flowid_1 = ((action->options.action_handle.flow_id >>11)&0x1F) * 8
                                + cls_fib->cos_1;
                        }else{
                            cls_fib->dp_value_1 = (((action->options.action_handle.flow_id >>11)&0x1F)
                                + 0x20);
                            cls_fib->cos_1 = (action->options.action_handle.flow_id >> 8) &0x7;
                        }
                    }
                }
            }
#endif
        }
    }

    if(action->options.masks.inner_vlan_act){
        if(sub_entry == 0){
            cls_fib->inner_cmd_valid_0 = TRUE;
            cls_fib->inner_vlan_cmd_0 = action->options.inner_vlan_act;
            cls_fib->inner_vlanid_0   = action->options.inner_vid;
        }else{
            cls_fib->inner_cmd_valid_1 = TRUE;
            cls_fib->inner_vlan_cmd_1  = action->options.inner_vlan_act;
            cls_fib->inner_vlanid_1    = action->options.inner_vid;
        }
    }

    if(action->options.masks.outer_vlan_act){

        if(sub_entry == 0){
            cls_fib->top_cmd_valid_0 = TRUE;
            cls_fib->top_vlan_cmd_0 = action->options.outer_vlan_act;
            cls_fib->top_vlanid_0  = action->options.outer_vid;

        }else{
            cls_fib->top_cmd_valid_1 = TRUE;
            cls_fib->top_vlan_cmd_1 = action->options.outer_vlan_act;
            cls_fib->top_vlanid_1  = action->options.outer_vid;
        }
    }

    if(sub_entry == 0){
        cls_fib->spt_mode_0 = AAL_L2_CLS_SPT_MODE_FWD_LEARN;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(__g_rule_is_use_for_mirror == TRUE){
            cls_fib->spt_mode_0 = AAL_L2_CLS_SPT_MODE_FWD_NOLEARN;
        }
#endif
    }else{
        cls_fib->spt_mode_1 = AAL_L2_CLS_SPT_MODE_FWD_LEARN;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(__g_rule_is_use_for_mirror == TRUE){
            cls_fib->spt_mode_1 = AAL_L2_CLS_SPT_MODE_FWD_NOLEARN;
        }
#endif
    }
    __g_rule_is_use_for_mirror  = FALSE;
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_G3)
    if(action->options.masks.sw_shaper_id){
        if(sub_entry == 0){
            cls_fib->dp_valid_0 = TRUE;
            cls_fib->dp_value_0 = __CLS_CPU256_PORT_START+ action->options.sw_shaper_id/__CLS_CPU256_PORT_QUE_NUM;
            cls_fib->cos_valid_0 = TRUE;
            cls_fib->cos_0 =  action->options.sw_shaper_id%__CLS_CPU256_PORT_QUE_NUM;
        }else {
            cls_fib->dp_valid_1 = TRUE;
            cls_fib->dp_value_1 = __CLS_CPU256_PORT_START+ action->options.sw_shaper_id/__CLS_CPU256_PORT_QUE_NUM;
            cls_fib->cos_valid_1 = TRUE;
            cls_fib->cos_1 =  action->options.sw_shaper_id%__CLS_CPU256_PORT_QUE_NUM;
        }
    }
#endif
    return CA_E_OK;
}
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
static ca_status_t __l2_cls_llid_gem_resource_reserve(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > (AAL_LPORT_LLID_GEM_INDEX_31 - AAL_LPORT_LLID_GEM_INDEX_0))){
        cls_printf("port %d entry %d start %d kry type %d sub %d\n", port_idx,hw_entry,start,key_type,sub_entry);
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+2);
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_SETBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        default:
            cls_printf("key_type is incorrected\n");
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

static ca_status_t __l2_cls_llid_gem_resource_release(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > (AAL_LPORT_LLID_GEM_INDEX_31 - AAL_LPORT_LLID_GEM_INDEX_0))){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+2);
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_CLRBIT(__g_cls_entry_occupy_pon[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        default:
            cls_printf("key_type is incorrected\n");
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

static ca_status_t __l2_cls_egr_llid_gem_resource_reserve(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > (AAL_LPORT_LLID_GEM_INDEX_31 - AAL_LPORT_LLID_GEM_INDEX_0))){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_SETBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        default:
            cls_printf("key_type is incorrected\n");
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

static ca_status_t __l2_cls_egr_llid_gem_resource_release(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > (AAL_LPORT_LLID_GEM_INDEX_31 - AAL_LPORT_LLID_GEM_INDEX_0))){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy_pon[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
#endif
            cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;

    }
    return CA_E_OK;
}

#endif

ca_status_t __l2_cls_port_resource_reserve(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > 15)){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+1);
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+2);
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_SETBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
#endif
            cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

ca_status_t __l2_cls_port_resource_release(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > 15)){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+1);
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+2);
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_CLRBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
#endif
            cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;

    }
    return CA_E_OK;
}

ca_status_t __l2_cls_egr_port_resource_reserve(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > 15)){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+1);
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_SETBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
#endif
        cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;
    }
    return CA_E_OK;
}


ca_status_t __l2_cls_egr_port_resource_release(
                                        ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry < start) || (port_idx > 15)){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+1);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2);
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_CLRBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
 #endif
        cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

ca_boolean_t __l2_cls_check_entry_occpuy_status(
                                            ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        ca_uint8_t sub_entry)
{
    if(!__CLS_COMPBIT(__g_cls_entry_occupy[port_idx],(hw_entry-start)*2+sub_entry)){
        return FALSE;
    }
    return TRUE;
}

ca_boolean_t __l2_cls_check_egr_entry_occpuy_status(
                                            ca_uint32_t port_idx,
                                        ca_uint8_t hw_entry,
                                        ca_uint8_t start,
                                        ca_uint8_t sub_entry)
{
    if(!__CLS_COMPBIT(__g_cls_egr_entry_occupy[port_idx],(hw_entry-start)*2+sub_entry)){
        return FALSE;
    }
    return TRUE;
}


#if defined(CONFIG_ARCH_CORTINA_G3HGU)   
static ca_status_t __l2_cls_saturn_port_resource_reserve(
                                        ca_uint8_t hw_entry,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry > 23) ){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2);
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+1);
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+2);
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2);
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_SETBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
 #endif
        cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;
    }
    return CA_E_OK;
}

static ca_status_t __l2_cls_saturn_port_resource_release(
                                        ca_uint8_t hw_entry,
                                        __cls_hw_key_type_t key_type, 
                                        ca_uint8_t sub_entry)
{
    if((sub_entry > 1) || (hw_entry > 23)){
        return CA_E_PARAM;
    }
    switch(key_type){
        case  __CLS_HW_KEY_TYPE_IPV6_LONG:
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2);
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+1);
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+2);
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+3);
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        case __CLS_HW_KEY_TYPE_IPV4R:
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2);
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2+1);
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            __CLS_CLRBIT(__g_cls_saturn_entry_occupy[0],(hw_entry)*2 + sub_entry);
        break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        case __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB:
#endif
            cls_printf(" vlan key_type %d \n",key_type);            
        break;    
        default:
            cls_printf("%s %d key_type %d is incorrected\n", __FUNCTION__,__LINE__,key_type);
            return CA_E_PARAM;
    }
    return CA_E_OK;
}
#endif

ca_status_t __l2_cls_resource_reserve(ca_uint32_t port_idx,
                                ca_boolean_t        is_egr, 
                                ca_uint8_t          hw_entry,
                                __cls_hw_key_type_t key_type,
                                ca_uint8_t          sub_entry)
{
    __l2_cls_entry_t    cls_entry;
    ca_uint32_t         new_port_idx = 0;
    ca_status_t         ret = CA_E_OK;
    if((port_idx > AAL_LPORT_LLID_GEM_INDEX_31) ||(hw_entry > 127)||(sub_entry>1)){
        return CA_E_PARAM;
    }
    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));    
    ret = __l2_cls_entry_get(port_idx, is_egr, &cls_entry);
    if(ret != CA_E_OK){
        goto end;
    }

    if(is_egr == FALSE){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
            new_port_idx = port_idx - AAL_LPORT_LLID_GEM_INDEX_0;
            ret = __l2_cls_llid_gem_resource_reserve(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                goto end;
            }
        }else
#endif
        {    if(port_idx >= AAL_LPORT_L3_WAN){
                new_port_idx = port_idx - 10;
            }else if(port_idx >= AAL_LPORT_CPU_0){
                new_port_idx = port_idx -8;
            }else{
                new_port_idx = port_idx;
            }
            ret = __l2_cls_port_resource_reserve(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                goto end;
            }
        }
    }else{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
            new_port_idx = port_idx - AAL_LPORT_LLID_GEM_INDEX_0;
            ret = __l2_cls_egr_llid_gem_resource_reserve(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                goto end;
            }
        }else
#endif
        {    
            if(port_idx >= AAL_LPORT_L3_WAN){
                new_port_idx = port_idx - 10;
            }else if(port_idx >= AAL_LPORT_CPU_0){
                new_port_idx = port_idx -8;
            }else{
                new_port_idx = port_idx;
            }
            ret = __l2_cls_egr_port_resource_reserve(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                goto end;
            }
        }
    }
end:
    return ret;    
}

static ca_status_t __l2_cls_resource_release(ca_uint32_t port_idx,
                                ca_boolean_t        is_egr, 
                                ca_uint8_t          hw_entry,
                                __cls_hw_key_type_t key_type,
                                ca_uint8_t          sub_entry)
{
    __l2_cls_entry_t    cls_entry;
    ca_uint32_t         new_port_idx = 0;
    ca_status_t         ret = CA_E_OK;
    
    cls_printf("port_idx = %d, hw_entry = %d, sub_entry = %d\n",port_idx,hw_entry,sub_entry);
    if((port_idx > AAL_LPORT_LLID_GEM_INDEX_31) ||(hw_entry > 127)||(sub_entry>1)){
        return CA_E_PARAM;
    }
    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));    
    ret = __l2_cls_entry_get(port_idx, is_egr, &cls_entry);
    if(ret != CA_E_OK){
        cls_printf("%d , ret = %d\n", __LINE__, ret);
        goto end;
    }

    if(is_egr == FALSE){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
            new_port_idx = port_idx - AAL_LPORT_LLID_GEM_INDEX_0;
            ret = __l2_cls_llid_gem_resource_release(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                cls_printf("%d , ret = %d\n", __LINE__, ret);
                goto end;
            }
        }else
#endif
         {
            if(port_idx >= AAL_LPORT_L3_WAN){
                new_port_idx = port_idx - 10;
            }else if(port_idx >= AAL_LPORT_CPU_0){
                new_port_idx = port_idx -8;
            }else{
                new_port_idx = port_idx;
            }
            ret = __l2_cls_port_resource_release(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                cls_printf("%d , ret = %d\n", __LINE__, ret);
                goto end;
            }
        }
    }else{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(port_idx >= AAL_LPORT_LLID_GEM_INDEX_0){
            new_port_idx = port_idx - AAL_LPORT_LLID_GEM_INDEX_0;
            ret = __l2_cls_egr_llid_gem_resource_release(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                cls_printf("%d , ret = %d\n", __LINE__, ret);
                goto end;
            }
        }else
#endif
         {   
            if(port_idx >= AAL_LPORT_L3_WAN){
                new_port_idx = port_idx - 10;
            }else if(port_idx >= AAL_LPORT_CPU_0){
                new_port_idx = port_idx -8;
            }else{
                new_port_idx = port_idx;
            }
            ret = __l2_cls_egr_port_resource_release(new_port_idx,hw_entry,cls_entry.cls_entry_start,key_type,sub_entry);
            if(ret != CA_E_OK){
                cls_printf("%d , ret = %d\n", __LINE__, ret);
                goto end;
            }
        }
    }
end:
    return ret;    
}


static ca_status_t __l2_cls_ipv6_long_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry
)
{
    aal_l2_cls_ipv4_key_mask_t          ipv4_key_mask;
    aal_l2_cls_ipv4_key_t               ipv4_key;
    aal_l2_cls_ipv6_short_key_mask_t    ipv6_short_key_mask;
    aal_l2_cls_ipv6_short_key_t         ipv6_short_key;
    ca_status_t                                         ret = CA_E_OK;
    memset(&ipv6_short_key, 0, sizeof(aal_l2_cls_ipv6_short_key_t));
    memset(&ipv4_key, 0, sizeof(aal_l2_cls_ipv4_key_t));
            
            ret = __l2_cls_ipv4_key_get(device_id,priority,key,key_mask,TRUE,&ipv4_key_mask,&ipv4_key);
            
            if(ret != CA_E_OK){
                cls_printf("%s, __l2_cls_ipv4_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }

           if(key_mask->dest_port){            
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    ret = aal_l2_cls_egr_ipv4_key_set(1,hw_entry,ipv4_key_mask,&ipv4_key);
                    if(ret != CA_E_OK){
                        cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }else
#endif
                {
                    ret = aal_l2_cls_egr_ipv4_key_set(device_id,hw_entry,ipv4_key_mask,&ipv4_key);
                }
            }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    ret = aal_l2_cls_ipv4_key_set(1,hw_entry,ipv4_key_mask,&ipv4_key);
                    if(ret != CA_E_OK){
                        cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }else
#endif
                    ret = aal_l2_cls_ipv4_key_set(device_id,hw_entry,ipv4_key_mask,&ipv4_key);
            }
            if(ret != CA_E_OK){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
            ret = __l2_cls_ipv6_short_key_get(device_id,priority,key,key_mask,&ipv6_short_key_mask,&ipv6_short_key);
            if(ret != CA_E_OK){
                cls_printf("%s,__l2_cls_ipv6_short_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
           if(key_mask->dest_port){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    ret = aal_l2_cls_egr_ipv6_short_key_set(1,hw_entry + 1,ipv6_short_key_mask,&ipv6_short_key);
                    if(ret != CA_E_OK){
                        cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }else
#endif
                ret = aal_l2_cls_egr_ipv6_short_key_set(device_id,hw_entry + 1,ipv6_short_key_mask,&ipv6_short_key);
            }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    ret = aal_l2_cls_ipv6_short_key_set(1,hw_entry + 1,ipv6_short_key_mask,&ipv6_short_key);
                    if(ret != CA_E_OK){
                        cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }else
#endif
                ret = aal_l2_cls_ipv6_short_key_set(device_id,hw_entry + 1,ipv6_short_key_mask,&ipv6_short_key);
            }
            if(ret != CA_E_OK){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
end:
    return ret;
}

ca_status_t __l2_cls_ipv6_short_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry
)
 {
    aal_l2_cls_ipv6_short_key_mask_t     ipv6_short_key_mask;
    aal_l2_cls_ipv6_short_key_t               ipv6_short_key;    
    ca_status_t                                         ret = CA_E_OK;
    memset(&ipv6_short_key, 0, sizeof(aal_l2_cls_ipv6_short_key_t));

            ret = __l2_cls_ipv6_short_key_get(device_id,priority,key,key_mask,&ipv6_short_key_mask,&ipv6_short_key);
            if(ret != CA_E_OK){
                cls_printf("%s, __l2_cls_ipv6_short_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
                if(__CLS_PORT_TYPE_FOR_SATURN){
                    ret = aal_l2_cls_ipv6_short_key_set(1,hw_entry,ipv6_short_key_mask,&ipv6_short_key);
                    if(ret != CA_E_OK){
                        cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }
#endif
           if(key_mask->dest_port){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
               if(__CLS_PORT_TYPE_FOR_SATURN){
                   ret = aal_l2_cls_egr_ipv6_short_key_set(1,hw_entry,ipv6_short_key_mask,&ipv6_short_key);
               }else
#endif
                {
                    ret = aal_l2_cls_egr_ipv6_short_key_set(device_id,hw_entry,ipv6_short_key_mask,&ipv6_short_key);
                }                
            }else {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
              if(__CLS_PORT_TYPE_FOR_SATURN){
                  ret = aal_l2_cls_ipv6_short_key_set(1,hw_entry,ipv6_short_key_mask,&ipv6_short_key);
              }else
#endif
                {
                    ret = aal_l2_cls_ipv6_short_key_set(device_id,hw_entry,ipv6_short_key_mask,&ipv6_short_key);
                }
            }
            if(ret != CA_E_OK){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
end:
    return ret;
}

static ca_status_t __l2_cls_ipv4_long_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry
)
 {
    aal_l2_cls_ipv4_key_mask_t              ipv4_key_mask;
    aal_l2_cls_ipv4_key_t                        ipv4_key; 
    aal_l2_specpkt_detect_mask_t        spec_mask;
    aal_l2_specpkt_detect_t             spec_cfg;
    ca_status_t                                         ret = CA_E_OK;
    memset(&ipv4_key, 0, sizeof(aal_l2_cls_ipv4_key_t));
    memset(&spec_cfg, 0, sizeof(aal_l2_specpkt_detect_t));
    spec_mask.u32[0] = 0;
    spec_mask.u32[1] = 0;

            ret = __l2_cls_ipv4_key_get(device_id,priority,key,key_mask,FALSE,&ipv4_key_mask,&ipv4_key);
            
            if(ret != CA_E_OK){
                cls_printf("%s,__l2_cls_ipv4_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
          if(key_mask->dest_port){            
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
               if(__CLS_PORT_TYPE_FOR_SATURN){
                   ret = aal_l2_cls_egr_ipv4_key_set(1,hw_entry,ipv4_key_mask,&ipv4_key);
                   if(ret != CA_E_OK){
                       cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                       goto end;
                   }
               }else
#endif
               {
                   ret = aal_l2_cls_egr_ipv4_key_set(device_id,hw_entry,ipv4_key_mask,&ipv4_key);
               }
           }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                 
               if(__CLS_PORT_TYPE_FOR_SATURN){
                   ret = aal_l2_cls_ipv4_key_set(1,hw_entry,ipv4_key_mask,&ipv4_key);
                   if(ret != CA_E_OK){
                       cls_printf("%s, aal_l2_cls_ipv4_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                       goto end;
                   }
               }else
#endif
                   ret = aal_l2_cls_ipv4_key_set(device_id,hw_entry,ipv4_key_mask,&ipv4_key);
           }
           
            if(ret != CA_E_OK){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
            
            if(key_mask->ext){
                spec_mask.s.l7_offset = 1;
                spec_cfg.l7_offset = (key->ext.offset);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
                if(__CLS_PORT_TYPE_FOR_SATURN)
                {
                    ret = aal_l2_specpkt_detect_set(1, spec_mask,&spec_cfg);
                }else
#endif
                {
                    ret = aal_l2_specpkt_detect_set(device_id, spec_mask,&spec_cfg);
                }

                if(ret != CA_E_OK){
                    cls_printf("%s,aal_l2_specpkt_detect_set() failed, %d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }
end:
    return ret;
}

static ca_status_t __l2_cls_l2r_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry,
          ca_uint32_t                           sub_entry
)
 {
    aal_l2_cls_l2r_key_mask_t           l2_key_mask;
    aal_l2_cls_l2r_key_t                    l2_key;
    ca_status_t                                 ret = CA_E_OK;
    
    memset(&l2_key, 0, sizeof(aal_l2_cls_l2r_key_t));
    l2_key_mask.u32 = 0;
        
            ret = __l2_cls_l2r_key_get(device_id,priority,key,key_mask, &l2_key_mask,&l2_key);
            
            if(ret != CA_E_OK){
                cls_printf("%s,__l2_cls_l2r_key_get, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }

     if(key_mask->dest_port){
        //egress cls
        if(sub_entry == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(__CLS_PORT_TYPE_FOR_SATURN)
            {
                 ret = aal_l2_cls_egr_l2r_1st_key_set(1,hw_entry,l2_key_mask,&l2_key);
            }else
#endif
            {
                ret = aal_l2_cls_egr_l2r_1st_key_set(device_id,hw_entry,l2_key_mask,&l2_key);
            }

            if(ret != CA_E_OK){
                cls_printf("%s,aal_l2_cls_egr_l2r_1st_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
  

        }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
                if(__CLS_PORT_TYPE_FOR_SATURN)
                {
                    ret = aal_l2_cls_egr_l2r_2nd_key_set(1,hw_entry,l2_key_mask,&l2_key);
                }else
#endif
                {
                    ret = aal_l2_cls_egr_l2r_2nd_key_set(device_id,hw_entry,l2_key_mask,&l2_key);
                }

                if(ret != CA_E_OK){
                    cls_printf("%s,aal_l2_cls_egr_l2r_2nd_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }
        }       else{
                if(sub_entry == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_l2r_1st_key_set(1,hw_entry,l2_key_mask,&l2_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_l2r_1st_key_set(device_id,hw_entry,l2_key_mask,&l2_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_l2r_1st_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)                
                    if(__CLS_PORT_TYPE_FOR_SATURN){ 
                        ret = aal_l2_cls_l2r_2nd_key_set(1,hw_entry,l2_key_mask,&l2_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_l2r_2nd_key_set(device_id,hw_entry,l2_key_mask,&l2_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_l2r_2nd_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
                }
            }
end:
    return ret;
}


static ca_status_t __l2_cls_l3r_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry,
          ca_uint32_t                           sub_entry
)
 {
    aal_l2_cls_l3r_key_mask_t           l3_key_mask;
    aal_l2_cls_l3r_key_t                    l3_key;

    ca_status_t                                 ret = CA_E_OK;
    
    memset(&l3_key, 0, sizeof(aal_l2_cls_l3r_key_t));
    l3_key_mask.u32 = 0;
   
            ret = __l2_cls_l3r_key_get(device_id,priority,key,key_mask, &l3_key_mask,&l3_key);            
            if(ret != CA_E_OK){
                cls_printf("%s, __l2_cls_l3r_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
                
            if(key_mask->dest_port){
                if(sub_entry == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_egr_l3r_1st_key_set(1,hw_entry,l3_key_mask,&l3_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_egr_l3r_1st_key_set(device_id,hw_entry,l3_key_mask,&l3_key);
                    }

                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_egr_l3r_1st_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_egr_l3r_2nd_key_set(1,hw_entry,l3_key_mask,&l3_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_egr_l3r_2nd_key_set(device_id,hw_entry,l3_key_mask,&l3_key);
                    }

                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_egr_l3r_2nd_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }                          
            }else{
                if(sub_entry == 0){
  #if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_l3r_1st_key_set(1,hw_entry,l3_key_mask,&l3_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_l3r_1st_key_set(device_id,hw_entry,l3_key_mask,&l3_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_l3r_1st_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_l3r_2nd_key_set(1,hw_entry,l3_key_mask,&l3_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_l3r_2nd_key_set(device_id,hw_entry,l3_key_mask,&l3_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_l3r_2nd_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }            
                
            }
end:
    return ret;
}


static ca_status_t __l2_cls_cmr_key_set(
          ca_device_id_t                      device_id,
          ca_uint32_t                           priority,
          ca_classifier_key_t                *key,
          ca_classifier_key_mask_t       *key_mask,
          ca_uint32_t                           hw_entry,
          ca_uint32_t                           sub_entry
)
 {
    aal_l2_cls_cmr_key_mask_t           cmr_key_mask;
    aal_l2_cls_cmr_key_t                   cmr_key;

    ca_status_t                                 ret = CA_E_OK;
    
    memset(&cmr_key, 0, sizeof(aal_l2_cls_cmr_key_t));
    cmr_key_mask.u32 = 0;
  
            ret = __l2_cls_cmr_key_get(device_id,priority,key,key_mask, &cmr_key_mask,&cmr_key);            
            if(ret != CA_E_OK){
                cls_printf("%s,__l2_cls_cmr_key_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }

            if(ret != CA_E_OK){
                cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }

            if(key_mask->dest_port){         
                if(sub_entry == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_egr_cmr_1st_key_set(1,hw_entry,cmr_key_mask,&cmr_key);
                    }else
#endif

                    {
                        ret = aal_l2_cls_egr_cmr_1st_key_set(device_id,hw_entry,cmr_key_mask,&cmr_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_egr_cmr_1st_key_set() failed, %d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }else{

 #if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_egr_cmr_2nd_key_set(1,hw_entry,cmr_key_mask,&cmr_key);
                    }else
#endif                   
                    {
                        ret = aal_l2_cls_egr_cmr_2nd_key_set(device_id,hw_entry,cmr_key_mask,&cmr_key);
                    }
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_egr_cmr_2nd_key_set() failed ,%d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

            }
                }else{
                if(sub_entry == 0){
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_cmr_1st_key_set(1,hw_entry,cmr_key_mask,&cmr_key);
                    }else
#endif
                    {
                        
                        ret = aal_l2_cls_cmr_1st_key_set(device_id,hw_entry,cmr_key_mask,&cmr_key);
                    }
                    
                    if(ret != CA_E_OK){
                        cls_printf("%s,aal_l2_cls_cmr_1st_key_set() failed ,%d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }

                }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) 
                    if(__CLS_PORT_TYPE_FOR_SATURN){                     
                        ret = aal_l2_cls_cmr_2nd_key_set(1,hw_entry,cmr_key_mask,&cmr_key);
                    }else
#endif
                    {
                        ret = aal_l2_cls_cmr_2nd_key_set(device_id,hw_entry,cmr_key_mask,&cmr_key);
                    }
                    
                    if(ret != CA_E_OK){
                        cls_printf("%s, aal_l2_cls_cmr_2nd_key_set() failed,%d \n",__FUNCTION__,__LINE__ );
                        goto end;
                    }
            }

        }  
end:
    return ret;
}


ca_status_t l2_classifier_rule_add (
CA_IN      ca_device_id_t                           device_id,
CA_IN      ca_uint32_t                          priority,
CA_IN      ca_classifier_key_t                  *key,
CA_IN      ca_classifier_key_mask_t             *key_mask,
CA_IN      ca_classifier_action_t               *action,
CA_OUT     ca_uint32_t                          *index)
{
    __cls_hw_key_type_t hw_key_type = __CLS_HW_KEY_TYPE_L2R;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t logic_port_id       = 0;
    //ca_uint32_t new_port_id         = 0;
    ca_uint32_t temp_sw_index       = __CLS_INVALID_INDEX;
    ca_uint32_t temp_hw_key_mask    = 0;
    ca_uint32_t hw_entry            = 0;
    ca_uint32_t sub_entry           = 0;
    ca_uint32_t fib_id              = 0;
    ca_boolean_t is_egr            = FALSE;
    ca_boolean_t dest_is_wan            = FALSE;
    ca_port_type_t dpid_port_type   = 0;
    aal_l2_cls_entry_status_t           entry_status;
    aal_l2_cls_entry_status_mask_t      entry_status_mask;
    aal_l2_cls_fib_t                    fib_cls;
    aal_l2_cls_fib_mask_t               fib_mask;

    __cls_rule_t                        *l2_cls_rule = NULL;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_pon_pdc_map_mem_data_t          pdc_cfg;
    ca_uint32_t sub_port_id         = 0;
#endif
#if 0//defined(CONFIG_ARCH_CORTINA_G3HGU)
   ca_uint16_t top_vid = 0xFFFF;
   ca_uint8_t  top_dot1p = 0xFF;
#endif
   /* CA_DEV_ID_CHECK(device_id);*/
    CA_ASSERT_RET(key, CA_E_PARAM);
    CA_ASSERT_RET(key_mask, CA_E_PARAM);
    CA_ASSERT_RET(action, CA_E_PARAM);
    CA_ASSERT_RET(index, CA_E_PARAM);


    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));
 
   
    memset(&fib_cls, 0, sizeof(aal_l2_cls_fib_t));
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
    memset(&pdc_cfg, 0, sizeof(aal_pon_pdc_map_mem_data_t));
#endif

    entry_status_mask.u32   = 0;
    fib_mask.u64            = 0;

    if(__g_l2_cls_debug){
        __cls_key_dump(device_id,key, key_mask);
        __cls_action_dump(action);
    }
    if(__l2_cls_is_use_vlan_fib(key_mask,key,action)){
        hw_key_type = __CLS_HW_KEY_TYPE_VLAN_FIB;
    }else{
        ret = __cls_key_parameter_check(key, key_mask);
        if(ret != CA_E_OK){
            cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
            goto end;
        }

        dpid_port_type  = PORT_TYPE(action->dest.port);

        if(key_mask->src_port){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(CA_PORT_IS_PON(key->src_port)){
                if((key->key_handle.gem_index&0xff) != 0xFF){
                    if(CA_PORT_TYPE_EPON == PORT_TYPE(key->src_port)){
                        ret = aal_pon_pdc_map_mem_get(device_id, ((key->key_handle.gem_index >> 11)&0x1f),&pdc_cfg);
                    }else{
                        ret = aal_pon_pdc_map_mem_get(device_id, (key->key_handle.gem_index&0xff),&pdc_cfg);
                    }
                    if(ret != CA_E_OK){
                        goto end;
                    }
                    logic_port_id = pdc_cfg.lspid;                 

                    cls_printf("logic_port_id = 0x%x\n",logic_port_id);
                    
                    if(!(logic_port_id >= AAL_LPORT_LLID_GEM_INDEX_0) &&(logic_port_id <= AAL_LPORT_LLID_GEM_INDEX_31)){
                        cls_printf("this gem_index not correct\n");
                        ret = CA_E_NOT_FOUND;
                        goto end;
                    }
                    sub_port_id = logic_port_id - AAL_LPORT_LLID_GEM_INDEX_0;
                  }else{
                        logic_port_id = 7;
                  }
            }else
#endif
            {
                logic_port_id = PORT_ID(key->src_port);
            }
            is_egr = FALSE;
        }else if(key_mask->dest_port){
        
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(CA_PORT_IS_PON(key->dest_port)){
                if(key->key_handle.gem_index == 0xFFFF){
                    logic_port_id = 7;
                }else{
                    sub_port_id = ((key->key_handle.gem_index >> 11) & 0x1f);
                    logic_port_id = __g_temp_ldpid + ((key->key_handle.gem_index >> 11) & 0x1f);
                    cls_printf("logic_port_id = 0x%x\n",logic_port_id);
                }
            }else
#endif
           {
            logic_port_id   = PORT_ID(key->dest_port);
            if(logic_port_id == wan_port_id){
                dest_is_wan = TRUE;
            }
            }
            is_egr = TRUE;
        }
        ret = __cls_action_check(is_egr,dest_is_wan,action);
        if(ret != CA_E_OK){
            cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
            goto end;
        }
    }
    if( hw_key_type != __CLS_HW_KEY_TYPE_VLAN_FIB){
        temp_hw_key_mask = __cls_hw_key_mask_get(key, key_mask);
        hw_key_type      = __cls_hw_key_map_mask2type(temp_hw_key_mask);
    }

    cls_printf("hw_key_type =  %d \n",hw_key_type );

    if(key_mask->src_port){
        temp_sw_index = __cls_sw_index_get(logic_port_id, hw_key_type);
    }else if(key_mask->dest_port){
        temp_sw_index = __cls_egr_sw_index_get(logic_port_id, hw_key_type);
    }else{
        if( hw_key_type == __CLS_HW_KEY_TYPE_VLAN_FIB){
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(__CLS_PORT_TYPE_FOR_SATURN){                
                temp_sw_index = __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB << 16 | key->l2.vlan_otag.vlan_min.vid;
            }else
#endif
            {
                temp_sw_index = __CLS_HW_KEY_TYPE_VLAN_FIB << 16 | key->l2.vlan_otag.vlan_min.vid;
            }
        }
        else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(__CLS_PORT_TYPE_FOR_SATURN){            
                temp_sw_index = __cls_hgu_saturn_sw_index_get(hw_key_type);
            }
#endif
        }
    }

    cls_printf("temp_sw_index =  0x%x \n",temp_sw_index );

    if(temp_sw_index == __CLS_INVALID_INDEX){
        return CA_E_RESOURCE;
    }
    if( hw_key_type == __CLS_HW_KEY_TYPE_VLAN_FIB){

    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    else if(temp_sw_index>= __HGU_SATURN_CLS_INDEX_START){        
        hw_entry = (temp_sw_index-__HGU_SATURN_CLS_INDEX_START)/2;
        sub_entry = temp_sw_index%2;
    }
#endif
    else if(temp_sw_index >= __EGR_CLS_INDEX_START){
        hw_entry = (temp_sw_index-__EGR_CLS_INDEX_START)/2;
        sub_entry = temp_sw_index%2;
    }else{
        hw_entry = temp_sw_index/2;
        sub_entry = temp_sw_index%2;
    }

    cls_printf("hw_entry =  %d  sub_entry = %d\n",hw_entry , sub_entry);

    if(key_mask->dest_port){
 #if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(__CLS_PORT_TYPE_FOR_SATURN){ 
            ret = aal_l2_cls_egr_entry_status_get(1,hw_entry,&entry_status); 
            if(ret != CA_E_OK){
                cls_printf("%s %d ret = %d\n",__FUNCTION__,__LINE__, ret);
                goto end;
            }
        }else
 #endif
        {   
            ret = aal_l2_cls_egr_entry_status_get(device_id,hw_entry,&entry_status); 
            if(ret != CA_E_OK){
                cls_printf("%s %d ret = %d\n",__FUNCTION__,__LINE__, ret);
                goto end;
            }
        }
    } else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(__CLS_PORT_TYPE_FOR_SATURN){ 
            ret = aal_l2_cls_entry_status_get(1,hw_entry,&entry_status); 
            if(ret != CA_E_OK){
                cls_printf("%s %d ret = %d\n",__FUNCTION__,__LINE__, ret);
                goto end;
            }
        }else
 #endif
        {   
            ret = aal_l2_cls_entry_status_get(device_id,hw_entry,&entry_status); 
            if(ret != CA_E_OK){
                cls_printf("%s %d ret = %d\n",__FUNCTION__,__LINE__, ret);
                goto end;
            }
        }
    } 
    switch(hw_key_type){
        case __CLS_HW_KEY_TYPE_IPV6_LONG:        
        ret = __l2_cls_ipv6_long_key_set(device_id,priority,key,key_mask,hw_entry);
        if(CA_E_OK != ret){
            goto end;
        }
        fib_id = hw_entry + 1;      
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
        ret = __l2_cls_ipv6_short_key_set(device_id,priority,key,key_mask,hw_entry);
        if(CA_E_OK != ret){
            goto end;
        }
        fib_id = hw_entry;      
        break;
        case __CLS_HW_KEY_TYPE_IPV4R:
            ret = __l2_cls_ipv4_long_key_set(device_id,priority,key,key_mask,hw_entry);
            if(CA_E_OK != ret){
                goto end;
            }
            fib_id = hw_entry;
            break;
        case __CLS_HW_KEY_TYPE_L2R:
        
            ret = __l2_cls_l2r_key_set(device_id,priority,key,key_mask,hw_entry,sub_entry);
            if(CA_E_OK != ret){
                goto end;
            }
            if(sub_entry == 0){
                  entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
                  entry_status_mask.s.rule_0 = 1;
            }else if(sub_entry == 1){
                entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
                entry_status_mask.s.rule_1 = 1;
            }
            fib_id = hw_entry;
            break;
        case __CLS_HW_KEY_TYPE_L3R:           
            ret = __l2_cls_l3r_key_set(device_id,priority,key,key_mask,hw_entry,sub_entry);
            if(CA_E_OK != ret){
                goto end;
            }
            if(sub_entry == 0){
                  entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE;
                  entry_status_mask.s.rule_0 = 1;
            }else if(sub_entry == 1){
                entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE;
                    entry_status_mask.s.rule_1 = 1;
                }
            fib_id = hw_entry;
            break;
        case __CLS_HW_KEY_TYPE_CMR:            

            ret = __l2_cls_cmr_key_set(device_id,priority,key,key_mask,hw_entry,sub_entry);
            if(CA_E_OK != ret){
                goto end;
            }
            if(sub_entry == 0){
                  entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE;
                  entry_status_mask.s.rule_0 = 1;
            }else if(sub_entry == 1){
                entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE;
                entry_status_mask.s.rule_1 = 1;
            } 
            fib_id = hw_entry;
            break;
        case __CLS_HW_KEY_TYPE_VLAN_FIB:

            break;
        default:
            cls_printf("%s %d \n",__FUNCTION__,__LINE__ );
            ret = CA_E_NOT_SUPPORT;
            goto end;
    }
    

  

    if(key_mask->src_port){
        ret = __l2_cls_fib_para_get(key->src_port,sub_entry,action,&fib_mask,&fib_cls);
        if(ret != CA_E_OK){
            cls_printf("%s,__l2_cls_fib_para_get() failed, %d \n",__FUNCTION__,__LINE__ );
            goto end;
        }
    
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
        if(__CLS_PORT_TYPE_FOR_SATURN){
            ret = aal_l2_cls_fib_set(1,fib_id,fib_mask,&fib_cls);
        }else
#endif           
        {
            ret = aal_l2_cls_fib_set(device_id,fib_id,fib_mask,&fib_cls);
        }        
    }else if(key_mask->dest_port){
        ret = __l2_cls_fib_para_get(key->dest_port,sub_entry,action,&fib_mask,&fib_cls);
        if(ret != CA_E_OK){
            cls_printf("%s,__l2_cls_fib_para_get() failed, %d \n",__FUNCTION__,__LINE__ );
            goto end;
        }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
            if(__CLS_PORT_TYPE_FOR_SATURN){
                ret = aal_l2_cls_egr_fib_set(1,fib_id,fib_mask,&fib_cls);
            }else
#endif           
        {
            ret = aal_l2_cls_egr_fib_set(device_id,fib_id,fib_mask,&fib_cls);
        }
    }else{
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
    if(!__CLS_PORT_TYPE_FOR_SATURN)
#endif       
        {
            ret = __l2_cls_vlan_fib_add(device_id,key->l2.vlan_otag.vlan_min.vid,action);
        }
    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if(__CLS_PORT_TYPE_FOR_SATURN){
        if(hw_key_type == __CLS_HW_KEY_TYPE_VLAN_FIB){
            ret = __l2_cls_vlan_fib_add(1,key->l2.vlan_otag.vlan_min.vid,action);            
            if(ret != CA_E_OK){              
                cls_printf("%s %d\n",__FUNCTION__,__LINE__);
                goto end;
            }        

        }else{
             ret = __l2_cls_fib_para_get(0x30007,sub_entry,action,&fib_mask,&fib_cls);
            if(ret != CA_E_OK){
                cls_printf("%s,__l2_cls_fib_para_get() failed, %d \n",__FUNCTION__,__LINE__ );
                goto end;
            }
            ret = aal_l2_cls_fib_set(1,fib_id,fib_mask,&fib_cls);
        }       
        if(ret != CA_E_OK){
            cls_printf("%s %d\n",__FUNCTION__,__LINE__);
            goto end;
        }  
#if 0// defined(CONFIG_NE_L3COS)
            if((key_mask->l2 == 1)&&(key_mask->l2_mask.vlan_otag ==1)
                &&(key_mask->l2_mask.vlan_otag_mask.vid == 1)){
                top_vid = key->l2.vlan_otag.vlan_min.vid;
            }
            
            if((key_mask->l2 == 1)&&(key_mask->l2_mask.vlan_otag ==1)
                &&(key_mask->l2_mask.vlan_otag_mask.pri == 1)){
                top_dot1p = key->l2.vlan_otag.vlan_min.pri;
            } 
            
            cls_printf("l3cos add vid = 0x%x dot1p = 0x%x \n", top_vid, top_dot1p);
            ret = ca_l3cos_rule_add(device_id,top_vid,top_dot1p,(action->options.action_handle.gem_index>>8)&7);
#endif
    }
#endif
       entry_status.valid              = TRUE;
       entry_status_mask.s.valid       = 1;

       if((hw_key_type == __CLS_HW_KEY_TYPE_L2R) ||
        (hw_key_type == __CLS_HW_KEY_TYPE_L3R)||
        (hw_key_type == __CLS_HW_KEY_TYPE_CMR)){
            entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY;
            entry_status_mask.s.entry_type  = 1;        
        }
       
        if(key_mask->dest_port){        
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
            if(__CLS_PORT_TYPE_FOR_SATURN){
                ret = aal_l2_cls_egr_entry_status_set(1,hw_entry,entry_status_mask,&entry_status);
                if(hw_key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                    ret |= aal_l2_cls_egr_entry_status_set(1,hw_entry + 1,entry_status_mask,&entry_status);
                }
            }else
#endif           
            ret = aal_l2_cls_egr_entry_status_set(device_id,hw_entry,entry_status_mask,&entry_status);
            if(hw_key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                ret |= aal_l2_cls_egr_entry_status_set(device_id,hw_entry + 1,entry_status_mask,&entry_status);
            }
        }else
        {
#if defined(CONFIG_ARCH_CORTINA_G3HGU)    
            if(__CLS_PORT_TYPE_FOR_SATURN){
                ret = aal_l2_cls_entry_status_set(1,hw_entry,entry_status_mask,&entry_status);
                if(hw_key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                    ret |= aal_l2_cls_entry_status_set(1,hw_entry + 1,entry_status_mask,&entry_status);
                }
            }else
#endif  
            ret = aal_l2_cls_entry_status_set(device_id,hw_entry,entry_status_mask,&entry_status);
            if(hw_key_type == __CLS_HW_KEY_TYPE_IPV6_LONG){
                ret |= aal_l2_cls_entry_status_set(device_id,hw_entry + 1,entry_status_mask,&entry_status);
            }
        }
        if(ret != CA_E_OK){
            printk("%s failed,%d \n",__FUNCTION__,__LINE__ );
            goto end;
        }

    *index = temp_sw_index;

    l2_cls_rule = (__cls_rule_t *)ca_malloc(sizeof(__cls_rule_t));
    if(NULL ==  l2_cls_rule){
        cls_printf("%s %d  malloc failed \n",__FUNCTION__,__LINE__ );
        return CA_E_MEMORY;
    }
    memset(l2_cls_rule,0,sizeof(__cls_rule_t));

    l2_cls_rule->next = NULL;

    /*memcpy(&l2_cls_rule->key, key, sizeof(ca_classifier_key_t) );
    memcpy(&l2_cls_rule->key_mask, key_mask, sizeof(ca_classifier_key_mask_t));
    memcpy(&l2_cls_rule->action, action, sizeof(ca_classifier_action_t));*/
    l2_cls_rule->sw_idx = temp_sw_index;
    l2_cls_rule->pri = priority;
    l2_cls_rule->key_type = hw_key_type;
#if 0//defined(CONFIG_ARCH_CORTINA_G3HGU)   
    l2_cls_rule->outer_vid = top_vid;
    l2_cls_rule->outer_8021p = top_dot1p;
#endif

    if(key_mask->src_port){
        l2_cls_rule->src_port_valid = 1;
        l2_cls_rule->port = logic_port_id;
        ret = __l2_cls_resource_reserve(logic_port_id,FALSE,hw_entry,hw_key_type,sub_entry);
        if(CA_E_OK != ret){
            goto end;
        }
    }
    else if(key_mask->dest_port){
        l2_cls_rule->dest_port_valid = 1;
        l2_cls_rule->dest_port = logic_port_id;

        ret = __l2_cls_resource_reserve(logic_port_id,TRUE,hw_entry,hw_key_type,sub_entry);
        if(CA_E_OK != ret){
            goto end;
        }

    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    if(__CLS_PORT_TYPE_FOR_SATURN && (key_mask->key_handle == 0)){
        ret = __l2_cls_saturn_port_resource_reserve(hw_entry, hw_key_type,sub_entry);
        if(CA_E_OK != ret){
            goto end;
        }
    }
#endif    
    ret = __cls_sw_table_add(l2_cls_rule);

end:
    cls_printf("%s %d l2_cls_rule = 0x%p\n",__FUNCTION__, __LINE__, l2_cls_rule);
    return ret;

}


ca_status_t l2_classifier_rule_del (
CA_IN      ca_device_id_t                           device_id,
CA_IN      ca_uint32_t                            index)
{
    ca_status_t ret = CA_E_OK;
    __cls_rule_t *l2_cls_rule = NULL;
    ca_uint32_t hw_entry = 0;
    ca_uint32_t sub_entry = 0;
    ca_uint32_t logic_port_id = 0;
    aal_ilpb_cfg_t ilpb_cfg;
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_boolean_t is_saturn_ne = FALSE;
#endif

#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_boolean_t    spid_is_gem = FALSE;    
   // ca_uint32_t     sub_port_id = 0xFFFFFFFF;
#endif
    memset(&ilpb_cfg, 0,sizeof(aal_ilpb_cfg_t));

    if(__cls_sw_table_search(index,&l2_cls_rule) == 0){
        cls_printf("%s index not found \n", __FUNCTION__);
        ret = CA_E_NOT_FOUND;
        goto end;
    }
    ret = aal_port_ilpb_cfg_get(device_id,0x20,&ilpb_cfg);
    if(CA_E_OK != ret){
        goto end;
    }
    if(index<256){
        hw_entry = index/2;
        sub_entry = index%2;
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        if(index >= (ilpb_cfg.cls_start*2)){
            spid_is_gem = TRUE;
           // sub_port_id = (hw_entry - ilpb_cfg.cls_start)/2;
        }   
#endif
      cls_printf("hw_entry %d, sub %d\n", hw_entry, sub_entry);
    }else if((index>>16 & 0xFF) == __CLS_HW_KEY_TYPE_VLAN_FIB){
        ret = __l2_cls_vlan_fib_del(device_id, index &0xFFFF);
        if(CA_E_OK != ret){
            cls_printf("line %d return err %d",__LINE__,ret);
            goto end;
        }
    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    else if((index>>16 & 0xFF) == __CLS_HW_KEY_TYPE_SATURN_VLAN_FIB){
        ret = __l2_cls_vlan_fib_del(1, index &0xFFFF);
        if(CA_E_OK != ret){
            cls_printf("line %d return err %d",__LINE__,ret);
            goto end;
        }
        is_saturn_ne = TRUE;
    }else if((index>>16 & 0xFF) == 2){
        hw_entry = (index&0xffff)/2;        
        sub_entry = index%2;
        is_saturn_ne = TRUE;
    }
#endif
    else{
        hw_entry = (index-__EGR_CLS_INDEX_START)/2;
        sub_entry = index%2;
        
    }
    
#if 0// defined(CONFIG_ARCH_CORTINA_G3HGU)
#if defined(CONFIG_NE_L3COS) 
        if(is_saturn_ne){
            ret = ca_l3cos_rule_delete(device_id,l2_cls_rule->outer_vid,l2_cls_rule->outer_8021p);
            if(CA_E_OK != ret){
                cls_printf("line %d return err %d",__LINE__,ret);
                goto end;
            }
        }
#endif
#endif
    if(l2_cls_rule->src_port_valid){
        logic_port_id = l2_cls_rule->port;
        ret = __l2_cls_resource_release(logic_port_id,FALSE,hw_entry,l2_cls_rule->key_type,sub_entry);
    }else if(l2_cls_rule->dest_port_valid){
        logic_port_id = l2_cls_rule->dest_port;        
        ret = __l2_cls_resource_release(logic_port_id,TRUE,hw_entry,l2_cls_rule->key_type,sub_entry);
    }
    
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    else{
        if(is_saturn_ne){
            ret = __l2_cls_saturn_port_resource_release(hw_entry,l2_cls_rule->key_type,sub_entry);
        }
    }
#endif
    if(ret != CA_E_OK){
        cls_printf("line %d return err %d\n",__LINE__,ret);
        goto end;
    }

    switch(l2_cls_rule->key_type){
        case __CLS_HW_KEY_TYPE_IPV6_LONG:
            cls_printf(" delete IPV6 long key \n");
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(is_saturn_ne){
                ret = __cls_delete_ipv4_key(1,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s, __cls_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_delete_ipv6_short_key(1,hw_entry+1);
                if(ret != CA_E_OK){
                    cls_printf("%s, __cls_delete_ipv6_short_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
                ret = __cls_fib_del(1,hw_entry+1,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

            }else
#endif
            if(l2_cls_rule->src_port_valid){
                ret = __cls_delete_ipv4_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s, __cls_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_delete_ipv6_short_key(device_id,hw_entry+1);
                if(ret != CA_E_OK){
                    cls_printf("%s, __cls_delete_ipv6_short_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_fib_del(device_id,hw_entry+1,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }else if (l2_cls_rule->dest_port_valid){
                ret = __cls_egr_delete_ipv4_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_egr_delete_ipv6_short_key(device_id,hw_entry+1);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );

                    goto end;
                }

                ret = __cls_egr_fib_del(device_id,hw_entry+1,0);

                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }
        break;
        case __CLS_HW_KEY_TYPE_IPV6:
            cls_printf(" delete IPV6 short key \n");
 #if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(is_saturn_ne){
                ret = __cls_delete_ipv6_short_key(1,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_ipv6_short_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
                ret = __cls_fib_del(1,hw_entry,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

            }else
#endif
            if(l2_cls_rule->src_port_valid){
                ret = __cls_delete_ipv6_short_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_ipv6_short_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_fib_del(device_id,hw_entry,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }else if(l2_cls_rule->dest_port_valid){
                ret = __cls_egr_delete_ipv6_short_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_delete_ipv6_short_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_egr_fib_del(device_id,hw_entry,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }
        break;
        case __CLS_HW_KEY_TYPE_IPV4R:
            cls_printf(" delete IPV4 key \n");
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(is_saturn_ne){
                ret = __cls_delete_ipv4_key(1,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
                ret = __cls_fib_del(1,hw_entry,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

            }else
#endif
            if(l2_cls_rule->src_port_valid){
                ret = __cls_delete_ipv4_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_fib_del(device_id,hw_entry,0);

                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }else if(l2_cls_rule->dest_port_valid){
                ret = __cls_egr_delete_ipv4_key(device_id,hw_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s, __cls_egr_delete_ipv4_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_egr_fib_del(device_id,hw_entry,0);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

            }
        break;
        case __CLS_HW_KEY_TYPE_L2R:
        case __CLS_HW_KEY_TYPE_L3R:
        case __CLS_HW_KEY_TYPE_CMR:
            cls_printf(" delete 2rule key %d\n", l2_cls_rule->key_type);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
            if(is_saturn_ne){
                ret = __cls_delete_2rule_key(1,hw_entry,sub_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_2rule_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
                ret = __cls_egr_fib_del(1,hw_entry,sub_entry);
                
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }else
#endif
            if(l2_cls_rule->src_port_valid){
                ret = __cls_delete_2rule_key(device_id,hw_entry,sub_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_delete_2rule_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }

                ret = __cls_fib_del(device_id,hw_entry,sub_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }else if(l2_cls_rule->dest_port_valid){
                ret = __cls_egr_delete_2rule_key(device_id,hw_entry,sub_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_delete_2rule_key() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
                ret = __cls_egr_fib_del(device_id,hw_entry,sub_entry);
                if(ret != CA_E_OK){
                    cls_printf("%s,__cls_egr_fib_del() failed,%d \n",__FUNCTION__,__LINE__ );
                    goto end;
                }
            }
        break;
        default:
            break;
    }
    ret = __cls_sw_table_del(index);
    ca_free(l2_cls_rule);
    l2_cls_rule = NULL;
 end:
    return ret;

}

ca_status_t l2_classifier_rule_get (
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_uint32_t                 index,
    CA_OUT      ca_uint32_t                 *priority,
    CA_OUT      ca_classifier_key_t         *key,
    CA_OUT      ca_classifier_key_mask_t    *key_mask,
    CA_OUT      ca_classifier_action_t      *action
)
{
    ca_status_t ret = CA_E_OK;
    __cls_rule_t *l2_cls_rule = NULL;

    CA_ASSERT_RET(key, CA_E_PARAM);
    CA_ASSERT_RET(key_mask, CA_E_PARAM);
    CA_ASSERT_RET(action, CA_E_PARAM);
    CA_ASSERT_RET(priority, CA_E_PARAM);

    if(__cls_sw_table_search(index,&l2_cls_rule) == 0){
        return CA_E_NOT_FOUND;
    }
    *priority = l2_cls_rule->pri;
   /* memcpy(key,&l2_cls_rule->key,sizeof(ca_classifier_key_t) );
    memcpy(key_mask,&l2_cls_rule->key_mask,sizeof(ca_classifier_key_mask_t));
    memcpy(action,&l2_cls_rule->action , sizeof(ca_classifier_action_t));*/
    return ret;

}



ca_status_t l2_classifier_port_default_action_set (
	CA_IN ca_device_id_t                    device_id,
	CA_IN ca_port_id_t                   port_id,
	CA_IN ca_classifier_default_action_t *p_action
)
{
    ca_uint32_t                     logic_port_id = 0;
    ca_uint32_t                     logic_dpid = 0;
    aal_l2_cls_l2rl2r_key_t         cls_key;
    aal_l2_cls_fib_t                cls_fib;
    aal_l2_cls_l2rl2r_key_mask_t    cls_mask;
    aal_l2_cls_fib_mask_t           fib_mask;
    ca_uint32_t                     entry_id = 0;
    static ca_boolean_t             entry_is_occpuy_self = FALSE;
    ca_status_t     ret = CA_E_OK;
    __l2_cls_entry_t                cls_entry;
    CA_ASSERT_RET(p_action, CA_E_PARAM);

    CA_PORT_ID_CHECK(port_id);

    if(p_action->action_type > CA_CLASSIFIER_FORWARD_PORT){
        return CA_E_PARAM;
    }

    if(p_action->action_type == CA_CLASSIFIER_FORWARD_PORT){
        CA_PORT_ID_CHECK(p_action->dest.port);
        logic_dpid = PORT_ID(p_action->dest.port);
    }

    logic_port_id = PORT_ID(port_id);
  #if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if(logic_port_id < 3){
        return CA_E_PARAM;
    }
  #endif
    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));
    ret = __l2_cls_entry_get(logic_port_id, FALSE, &cls_entry);
    if(ret != CA_E_OK){
        goto end;
    }
    memset(&cls_key, 0, sizeof(aal_l2_cls_l2rl2r_key_t));
    cls_mask.u64 = 0;

    entry_id = cls_entry.cls_entry_start + cls_entry.cls_entry_length - 1;
    if(entry_is_occpuy_self == FALSE){
        if(__CLS_COMPBIT(__g_cls_entry_occupy[logic_port_id],(cls_entry.cls_entry_length-1)*2 +1)){
            ret = CA_E_RESOURCE;
            goto end;
        }
    }
    entry_is_occpuy_self = TRUE;
    ret = __l2_cls_resource_reserve(logic_port_id,FALSE,entry_id,__CLS_HW_KEY_TYPE_L2R,1);
    if(ret != CA_E_OK){
        goto end;
    }
    
    ret = aal_l2_cls_l2rl2r_key_get(device_id,entry_id,&cls_key);
    if(ret != CA_E_OK){
        goto end;
    }
    cls_mask.s.valid                   = 1 ;
    cls_mask.s.ethernet_type_vld_1     = 1 ;
    cls_mask.s.l4_port_field_ctrl_1    = 1 ;
    cls_mask.s.mac_field_ctrl_1        = 1 ;
    cls_mask.s.vlan_field_ctrl_1       = 1 ;
    cls_mask.s.dot1p_ctrl_1            = 1 ;
    cls_mask.s.vlan_number_vld_1       = 1 ;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    cls_mask.s.inner_dei_vld_1         = 1;
    cls_mask.s.top_dei_vld_1           = 1;
#endif
    cls_key.valid                   = TRUE;
    cls_key.ethernet_type_vld_1     = FALSE;
    cls_key.l4_port_field_ctrl_1    = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    cls_key.mac_field_ctrl_1        = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    cls_key.vlan_field_ctrl_1       = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    cls_key.dot1p_ctrl_1            = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    cls_key.vlan_number_vld_1       = FALSE;
#if defined(CONFIG_ARCH_CORTINA_VENUS)   
    cls_key.inner_dei_vld_1         = FALSE;
    cls_key.top_dei_vld_1           = FALSE;
#endif
    ret = aal_l2_cls_l2rl2r_key_set(device_id,entry_id,cls_mask,&cls_key);
    if(ret != CA_E_OK){
        goto end;
    }
    memset(&cls_fib, 0, sizeof(aal_l2_cls_fib_t));
    fib_mask.u64 = 0;

    if(p_action->action_type == CA_CLASSIFIER_FORWARD_DENY){
        cls_fib.permit_1 = FALSE;
        cls_fib.permit_valid_1 = TRUE;
        fib_mask.s.permit_1 = 1;
        fib_mask.s.permit_valid_1 = 1;
    }else{
        fib_mask.s.permit_valid_1 = 1;
        cls_fib.permit_valid_1 = FALSE;
    }

    if(p_action->action_type == CA_CLASSIFIER_FORWARD_PORT){
        cls_fib.dp_valid_1 = TRUE;
        cls_fib.dp_value_1 = logic_dpid;
        fib_mask.s.dp_valid_1 = 1;
        fib_mask.s.dp_value_1 = 1;
    }else{
        cls_fib.dp_valid_1 = FALSE;
        cls_fib.dp_value_1 = 0;
        fib_mask.s.dp_valid_1 = 1;
        fib_mask.s.dp_value_1 = 1;
    }

    fib_mask.s.spt_mode_1 = 1;
    cls_fib.spt_mode_1 = AAL_L2_CLS_SPT_MODE_FWD_LEARN;

    ret = aal_l2_cls_fib_set(device_id,entry_id,fib_mask,&cls_fib);
    if(ret != CA_E_OK){
        goto end;
    }

end:
    return ret;

}

ca_status_t l2_classifier_port_default_action_get (
	CA_IN ca_device_id_t                    device_id,
	CA_IN ca_port_id_t                   port_id,
	CA_OUT ca_classifier_default_action_t *p_action
)
{
    ca_uint32_t                     logic_port_id = 0;
    aal_l2_cls_fib_t                cls_fib;
    ca_uint32_t                     entry_id = 0;
    ca_status_t                     ret = CA_E_OK;
    __l2_cls_entry_t                cls_entry;

    CA_ASSERT_RET(p_action, CA_E_PARAM);

    CA_PORT_ID_CHECK(port_id);


    logic_port_id = PORT_ID(port_id);
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if(logic_port_id < 3){
        return CA_E_PARAM;
    }
#endif
    memset(&cls_entry, 0, sizeof(__l2_cls_entry_t));
    ret = __l2_cls_entry_get(logic_port_id, FALSE, &cls_entry);
    if(ret != CA_E_OK){
        goto end;
    }

    entry_id = cls_entry.cls_entry_start + cls_entry.cls_entry_length - 1;
    memset(&cls_fib, 0, sizeof(aal_l2_cls_fib_t));

    ret = aal_l2_cls_fib_get(device_id,entry_id,&cls_fib);
    if(ret != CA_E_OK){
        goto end;
    }
    if((cls_fib.permit_valid_1 == TRUE)&&
        (cls_fib.permit_1 == FALSE)){
        p_action->action_type = CA_CLASSIFIER_FORWARD_DENY;
        p_action->dest.port = 0;
    }else if(cls_fib.dp_valid_1 == TRUE){
        p_action->action_type = CA_CLASSIFIER_FORWARD_PORT;
	  if(cls_fib.dp_value_1 < 7){
            p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET,cls_fib.dp_value_1);
        }else if(cls_fib.dp_value_1 == 7){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
             if((__g_pon_mac_mode >= ONU_PON_MAC_GPON) && (__g_pon_mac_mode<=ONU_PON_MAC_NGPON2)){   
            		p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_GPON,cls_fib.dp_value_1);
          	}else if(__g_pon_mac_mode < ONU_PON_MAC_GPON){
          	    p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_EPON,cls_fib.dp_value_1);
          	}else{
          	    p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET,cls_fib.dp_value_1);
          	}
#else
            p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET,cls_fib.dp_value_1);
#endif
        }else if((cls_fib.dp_value_1 == 0x10)||(cls_fib.dp_value_1 == 0x11)){
            p_action->dest.port = CA_PORT_ID(CA_PORT_TYPE_CPU,cls_fib.dp_value_1);
        }
    }else{
        p_action->action_type = CA_CLASSIFIER_FORWARD_FE;
        p_action->dest.port = 0;
    }
end:
    return ret;

}
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
#define __CLS_US_PORT_START     6
#define __CLS_US_PORT_NUM       1
#else
#define __CLS_US_PORT_START     3
#define __CLS_US_PORT_NUM       4
#endif

ca_status_t __special_packet_default_rule_set(
    ca_device_id_t              device_id,
    ca_uint32_t                 aal_port,
    ca_uint32_t                 precedence,
    aal_l2_specpkt_type_t       special_packet_type,
    ca_uint16_t                 priority,
    ca_uint16_t                 flow_id
)
{
    ca_uint32_t sw_cls_index = 0;
    ca_uint32_t hw_entry = 0;
    ca_uint32_t sub_entry = 0;
    ca_status_t                     ret = CA_E_OK;
    aal_l2_cls_entry_status_t           entry_status;
    aal_l2_cls_entry_status_mask_t      entry_status_mask;    
    ca_classifier_action_t              action;
    aal_l2_cls_fib_mask_t               cls_fib_mask;
    aal_l2_cls_fib_t                    cls_fib;
    aal_l2_cls_l3r_key_mask_t        l3r_key_mask;
    aal_l2_cls_l3r_key_t             l3r_key;    
    
    l3r_key_mask.u32 = 0;
    memset(&l3r_key, 0, sizeof(l3r_key));
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));
    memset(&action, 0, sizeof(action));
    memset(&cls_fib, 0, sizeof(cls_fib));

    entry_status_mask.u32 = 0;
    cls_fib_mask.u64 = 0;
    entry_status.entry_type         = AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY;
    entry_status_mask.s.entry_type  = 1;
    entry_status.valid              = TRUE;
    entry_status_mask.s.valid       = 1;
    
    ret = __l2_cls_l3r_spcl_pkt_key_get(0,precedence,special_packet_type,&l3r_key_mask,&l3r_key);
    if(ret != CA_E_OK){
        cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    sw_cls_index = __cls_sw_index_get(aal_port,__CLS_HW_KEY_TYPE_L3R);
    hw_entry = sw_cls_index/2;
    sub_entry = sw_cls_index%2; 
    if(sw_cls_index == 0xFFFFFFFF){
        cls_printf("%s %d return error",__FUNCTION__,__LINE__);
        goto end;
    }
    ret = __l2_cls_resource_reserve(aal_port,FALSE,hw_entry,__CLS_HW_KEY_TYPE_L3R,sub_entry);   
    if(ret != CA_E_OK){
        cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    if(sub_entry == 0){
        ret = aal_l2_cls_l3r_1st_key_set(0,hw_entry,l3r_key_mask,&l3r_key);       
        if(ret != CA_E_OK){
            cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
            goto end;
        }
        entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE;
        entry_status_mask.s.rule_0 = 1;
    }else{
        ret = aal_l2_cls_l3r_2nd_key_set(0,hw_entry,l3r_key_mask,&l3r_key);        
        if(ret != CA_E_OK){
            cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
            goto end;
        }
        entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE;
        entry_status_mask.s.rule_1 = 1;
    }
    ret = aal_l2_cls_entry_status_set(0,hw_entry,entry_status_mask,&entry_status);
    if(ret != CA_E_OK){
        cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    action.forward = CA_CLASSIFIER_FORWARD_FE;
    action.options.masks.priority = 1;          
    action.options.priority  = priority;
    action.options.masks.action_handle = 1;
    action.options.action_handle.flow_id = flow_id;
    ret = __l2_cls_fib_para_get(CA_PORT_ID(CA_PORT_TYPE_ETHERNET,aal_port),sub_entry,&action,&cls_fib_mask,&cls_fib);
    if(ret != CA_E_OK){
        cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    ret = aal_l2_cls_fib_set(0,hw_entry,cls_fib_mask,&cls_fib);    
    if(ret != CA_E_OK){
        cls_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
end:
    return ret;
}


ca_status_t __my_mac_default_rule_set(
    ca_device_id_t                  device_id,
    ca_uint32_t                     aal_port,
    ca_uint32_t                     precedence,
    ca_uint16_t                      my_mac_id,
    ca_uint16_t                       priority,
    ca_uint16_t                        flow_id
)
{
    ca_uint32_t sw_cls_index = 0;
    ca_uint32_t hw_entry = 0;
    ca_uint32_t sub_entry = 0;
    ca_status_t                     ret = CA_E_OK;
    ca_classifier_key_mask_t      cls_key_mask;
    ca_classifier_key_t                 cls_key;
    ca_classifier_action_t              action;
    aal_l2_cls_fib_mask_t               cls_fib_mask;
    aal_l2_cls_fib_t                    cls_fib;
    aal_l2_cls_l2r_key_mask_t        l2r_key_mask;
    aal_l2_cls_l2r_key_t             l2r_key;
    ca_mac_addr_t                   mac;
    aal_l2_cls_entry_status_t           entry_status;
    aal_l2_cls_entry_status_mask_t      entry_status_mask;

    l2r_key_mask.u32 = 0;
    memset(&cls_key_mask, 0, sizeof(cls_key_mask));
    memset(&cls_key, 0, sizeof(cls_key));
    memset(&l2r_key, 0, sizeof(l2r_key));
    memset(&action, 0, sizeof(action));
    memset(&cls_fib, 0, sizeof(cls_fib));
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));
    entry_status_mask.u32 = 0;

    if(my_mac_id == 0){
        aal_scfg_read(CFG_ID_NE_MAC_ADDR0, 6, &mac[0]);
    }else{
        aal_scfg_read(CFG_ID_NE_MAC_ADDR1, 6, &mac[0]);
    }

    cls_key_mask.src_port = 1;
    if((aal_port == 7)||(aal_port == 32)){
        cls_key.src_port = CA_PORT_ID(CA_PORT_TYPE_EPON,7);
        cls_key_mask.key_handle = 1;
    }else{
        cls_key.src_port = CA_PORT_ID(CA_PORT_TYPE_EPON,7);
    }
    cls_key_mask.l2 = 1;
    cls_key_mask.l2_mask.mac_sa = 1;
    memcpy(&cls_key.l2.mac_sa.mac_min[0],&mac[0],6);
    memcpy(&cls_key.l2.mac_sa.mac_max[0],&mac[0],6);

    ret = __l2_cls_l2r_key_get(0,precedence,&cls_key,&cls_key_mask,&l2r_key_mask,&l2r_key);
    if(ret != CA_E_OK){
        ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    sw_cls_index = __cls_sw_index_get(aal_port,__CLS_HW_KEY_TYPE_L2R);
    hw_entry = sw_cls_index/2;
    sub_entry = sw_cls_index%2;
    if(sw_cls_index == 0xFFFFFFFF){
        ca_printf("%s %d return error",__FUNCTION__,__LINE__);
        goto end;
    }
    ret = __l2_cls_resource_reserve(aal_port,FALSE,hw_entry,__CLS_HW_KEY_TYPE_L2R,sub_entry);
    if(ret != CA_E_OK){
        ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    if(sub_entry == 0){
        ret = aal_l2_cls_l2r_1st_key_set(0,hw_entry,l2r_key_mask,&l2r_key);
        if(ret != CA_E_OK){
            ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
            goto end;
        }
        entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
        entry_status_mask.s.rule_0 = 1;
    }else{
        ret = aal_l2_cls_l2r_2nd_key_set(0,hw_entry,l2r_key_mask,&l2r_key);
        if(ret != CA_E_OK){
            ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
            goto end;
        }
        entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
        entry_status_mask.s.rule_1 = 1;
    }
    ret = aal_l2_cls_entry_status_set(0,hw_entry,entry_status_mask,&entry_status);
    if(ret != CA_E_OK){
        ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    action.forward = CA_CLASSIFIER_FORWARD_FE;
    action.options.masks.priority = 1;
    action.options.priority  = priority;
    action.options.masks.action_handle = 1;
    action.options.action_handle.flow_id = flow_id;
    ret = __l2_cls_fib_para_get(CA_PORT_ID(CA_PORT_TYPE_ETHERNET,aal_port),sub_entry,&action,&cls_fib_mask,&cls_fib);
    if(ret != CA_E_OK){
        ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
    ret = aal_l2_cls_fib_set(0,hw_entry,cls_fib_mask,&cls_fib);
    if(ret != CA_E_OK){
        ca_printf("%s %d return %d",__FUNCTION__,__LINE__,ret);
        goto end;
    }
end:
    return ret;
}
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
int __trap_sp_frame_rule_to_cpu_init(ca_uint32_t    aal_port )
{
    //ca_classifier_handle_t      action_handle = {0};
    ca_uint32_t                 priority = 7;
    ca_classifier_key_t         cls_key;
    ca_classifier_key_mask_t    cls_key_mask;
    ca_classifier_action_t      cls_action;
    ca_uint32_t                 cls_index = 0;
    ca_port_id_t                ca_portid = 0;
    ca_port_id_t                cpu_port = 0;
    ca_uint8_t                  local_mac[6];    
    //ca_device_config_tlv_t      scfg_tlv;
    //int                         ret = 0;


    
    /*set arp*/
    ca_portid = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, aal_port);
    cpu_port =   CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
    
#if 0
    ca_special_packet_set(
                    0, 
                    ca_portid,
                    CA_PKT_TYPE_ARP,
                    1,
                    cpu_port,
                    0,
                    0,
                    &action_handle);
#endif

    aal_scfg_read(CFG_ID_NE_MAC_ADDR1, 6, local_mac);


    /*set my mac*/
    memset(&cls_key, 0, sizeof(ca_classifier_key_t));
    memset(&cls_key_mask, 0, sizeof(ca_classifier_key_mask_t));
    memset(&cls_action, 0, sizeof(ca_classifier_action_t));

    cls_key_mask.key_handle = 1;
    cls_key.key_handle.llid_cos_index = 0;

    cls_key_mask.src_port = 1;

    cls_key_mask.l2 = 1;
    cls_key_mask.l2_mask.mac_da = 1;


    cls_key.src_port = ca_portid;

    memcpy(cls_key.l2.mac_da.mac_min, local_mac, 6 );
    memcpy(cls_key.l2.mac_da.mac_max, local_mac, 6 );
    
    cls_action.forward = CA_CLASSIFIER_FORWARD_PORT;
    cls_action.dest.port = cpu_port;

    return l2_classifier_rule_add ( 0, 
                                    priority,
                                    &cls_key, 
                                    &cls_key_mask , 
                                    &cls_action,
                                    &cls_index);

}
#endif
void __cls_rate_limit_t(
            ca_device_id_t device_id,
            ca_uint16_t flow_id,
            ca_uint16_t rate,
            ca_uint16_t burst,
            ca_uint16_t pps_mode)
{
    aal_l2_te_policer_profile_msk_t te_pol_mask;
    aal_l2_te_policer_profile_t          te_pol;
    aal_l2_te_policer_cfg_t               pol_cfg;
    aal_l2_te_policer_cfg_msk_t        pol_cfg_mask;

    te_pol_mask.u32 = 0;
    memset(&te_pol, 0, sizeof(aal_l2_te_policer_profile_t));
    memset(&pol_cfg, 0, sizeof(aal_l2_te_policer_cfg_t));
    pol_cfg_mask.u32 = 0;
    te_pol_mask.s.cir_k = 1;
    te_pol_mask.s.pir_k = 1;
    te_pol_mask.s.cir_m = 1;
    te_pol_mask.s.pir_m = 1;
    te_pol_mask.s.cbs = 1;
    te_pol_mask.s.pbs = 1;
    te_pol.cir_k = rate;
    te_pol.cbs = burst;
    te_pol.pir_k = rate;
    te_pol.pbs = burst;
    pol_cfg_mask.s.pol_update_mode = 1;
    pol_cfg_mask.s.type = 1;

    pol_cfg.type = CA_AAL_POLICER_TYPE_RFC2698;

    if(pps_mode){
        pol_cfg.pol_update_mode = CA_AAL_POLICER_UPDATE_MODE_PKT;
    }else{
        pol_cfg.pol_update_mode = CA_AAL_POLICER_UPDATE_MODE_BYTE;
     }
    aal_l2_te_policer_flow_cfg_set(device_id,flow_id,pol_cfg_mask,&pol_cfg);
    aal_l2_te_policer_flow_profile_set(device_id,flow_id,te_pol_mask,&te_pol);
    return;
}

#if defined(CONFIG_ARCH_CORTINA_G3)
ca_status_t l2_classifier_g3_mirror_add (
    ca_device_id_t device_id,
    ca_port_id_t   port_id,
    ca_port_id_t   dest_port_id,
    ca_uint32_t    priority
)
{
    ca_uint32_t logic_port      = 0;
    ca_uint32_t dest_port       = 0;
    ca_uint32_t sw_cls_index    = 0;
    ca_uint32_t hw_entry        = 0;
    ca_uint32_t sub_entry       = 0;
    ca_status_t ret             = CA_E_OK;
    aal_l2_cls_l2r_key_mask_t   l2r_key_mask;
    aal_l2_cls_l2r_key_t        l2r_key;
    aal_l2_cls_fib_mask_t       fib_mask;
    aal_l2_cls_fib_t            fib;
    aal_l2_cls_entry_status_t   entry_status;
    aal_l2_cls_entry_status_mask_t entry_mask;
    
    l2r_key_mask.u32 = 0;
    memset(&l2r_key, 0, sizeof(aal_l2_cls_l2r_key_t));
    fib_mask.u64 = 0;   
    memset(&fib, 0, sizeof(aal_l2_cls_fib_t));

    entry_mask.u32 = 0;
    memset(&entry_status, 0, sizeof(aal_l2_cls_entry_status_t));
    
    logic_port = PORT_ID(port_id);
    dest_port = PORT_ID(dest_port_id);
    sw_cls_index = __cls_sw_index_get(logic_port,__CLS_HW_KEY_TYPE_L2R);
    hw_entry = sw_cls_index/2;
    sub_entry = sw_cls_index%2;
    
    l2r_key.mac_field_ctrl      = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    l2r_key.vlan_field_ctrl     = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    l2r_key.dot1p_ctrl          = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    l2r_key.ethernet_type_vld   = FALSE;
    l2r_key.vlan_number_vld     = FALSE;
    l2r_key.l4_port_field_ctrl  = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    l2r_key.precedence          = priority;
    l2r_key_mask.s.mac_field_ctrl       = 1;
    l2r_key_mask.s.vlan_field_ctrl      = 1;
    l2r_key_mask.s.dot1p_ctrl           = 1;
    l2r_key_mask.s.ethernet_type_vld    = 1;
    l2r_key_mask.s.vlan_number_vld      = 1;
    l2r_key_mask.s.l4_port_field_ctrl   = 1;
    l2r_key_mask.s.precedence           = 1;

    entry_status.valid = TRUE;
    entry_status.entry_type = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
    entry_mask.s.valid = 1;
    entry_mask.s.entry_type = 1;

    if(sub_entry == 0){
        entry_mask.s.rule_0 = 1;
        entry_status.rule_0 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
        ret = aal_l2_cls_entry_status_set(device_id,hw_entry,entry_mask,&entry_status);
        if(ret != CA_E_OK){
            return ret;
        }    
        ret = aal_l2_cls_l2r_1st_key_set(device_id,hw_entry,l2r_key_mask,&l2r_key);
        if(ret != CA_E_OK){
            return ret;
        }
        fib.spt_mode_0 = 3;
        fib.dp_value_0 = dest_port;
        fib.dp_valid_0 = TRUE;
        fib_mask.s.spt_mode_0 = 1;
        fib_mask.s.dp_value_0 = 1;
        fib_mask.s.dp_valid_0 = 1;
        ret = aal_l2_cls_fib_set(device_id,hw_entry,fib_mask,&fib);
        if(ret != CA_E_OK){
            return ret;
        }
    }else{
        entry_mask.s.rule_1 = 1;
        entry_status.rule_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
        ret = aal_l2_cls_entry_status_set(device_id,hw_entry,entry_mask,&entry_status);
        if(ret != CA_E_OK){
            return ret;
        }
        ret = aal_l2_cls_l2r_2nd_key_set(device_id,hw_entry,l2r_key_mask,&l2r_key);
        if(ret != CA_E_OK){
            return ret;
        }
        fib.spt_mode_1 = 3;
        fib.dp_value_1 = dest_port;
        fib.dp_valid_1 = TRUE;
        fib_mask.s.spt_mode_1 = 1;
        fib_mask.s.dp_value_1 = 1;
        fib_mask.s.dp_valid_1 = 1;
        ret = aal_l2_cls_fib_set(device_id,hw_entry,fib_mask,&fib);
        if(ret != CA_E_OK){
            return ret;
        }
    }  
    return CA_E_OK;
}

#endif
void ca_l2_classifier_init(void)
{
    ca_classifier_key_t             key;
    ca_classifier_key_mask_t        key_mask;
    ca_classifier_action_t          action;
    ca_uint32_t                     index = 0;
    ca_status_t                     ret = CA_E_OK;
    ca_uint8_t                      wan_port_type = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint8_t                      puc_voq_mode = 0;
    ca_uint8_t                      pon_mode = 0;
#endif
    ca_uint32_t                     aal_port = 0;
    ca_uint32_t                     cls_us_port_start = 0;
    ca_boolean_t                    evb_sfp_flag = FALSE;
    ca_mac_addr_t                   bc_mac ={0xff,0xff,0xff,0xff,0xff,0xff};

    memset(&key,0,sizeof(ca_classifier_key_t));
    memset(&key_mask,0,sizeof(ca_classifier_key_mask_t));
    memset(&action,0,sizeof(action));

    __l2_cls_rule_head.counter = 0;

    __l2_cls_rule_head.next = NULL;

    aal_scfg_read(CFG_ID_WAN_PORT_TYPE_IS_L3, 1,&wan_port_type);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)|| defined(CONFIG_ARCH_CORTINA_G3HGU)    
    aal_scfg_read(CFG_ID_PON_VOQ_MODE, 1,&puc_voq_mode);
    __g_puc_voq_mode = puc_voq_mode;
    //cls_printf("%s %d __g_puc_voq_mode = %d\n",__FILE__,__LINE__,__g_puc_voq_mode);    
    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
          cls_printf("<%s, %d> read CFG_ID_PON_MAC_MODE fail\r\n", __func__, __LINE__);
    }
    __g_pon_mac_mode = pon_mode;
#endif
    if(wan_port_type == 1){
        key_mask.src_port = 1;

        key.src_port = 0x30000 | wan_port_id;
        action.forward = CA_CLASSIFIER_FORWARD_PORT;
        action.dest.port = 0x80018;

        ret = l2_classifier_rule_add(0,7,&key,&key_mask,&action,&index);
        if(CA_E_OK != ret){
            cls_printf("ca_classifier_rule_add failed in ca_l2_classifier_init\n ");
        }
    }
    
    if (l2_customer_info & CUSTOMER_BIT_1){
        cls_us_port_start = __CLS_US_PORT_START;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
        evb_sfp_flag = __eth_port3_active_evb_check( );
        if (evb_sfp_flag == TRUE){
            cls_us_port_start = 3;
        }
#endif
        for(aal_port = __CLS_US_PORT_START; aal_port<__CLS_US_PORT_START + __CLS_US_PORT_NUM; aal_port++){
            __special_packet_default_rule_set(0,aal_port,1,AAL_L2_SPECPKT_TYPE_ARP,7,CA_FLOW_ID_US_ARP);
            __special_packet_default_rule_set(0,aal_port,1,AAL_L2_SPECPKT_TYPE_DHCP,7,CA_FLOW_ID_US_DHCP);
            //__special_packet_default_rule_set(0,aal_port,1,AAL_L2_SPECPKT_TYPE_IGMP_MLD);
            __special_packet_default_rule_set(0,aal_port,1,AAL_L2_SPECPKT_TYPE_ICMP,7,CA_FLOW_ID_US_ICMP);
            __my_mac_default_rule_set(0,aal_port, 1,1,7,CA_FLOW_ID_US_MY_MAC);
        }

            
           /*general query trap to cpu must be highest priority , so it must init it in AAL dirver*/
            //__trap_general_query_rule_to_cpu0_init( );
            //__special_packet_default_rule_set(0,0x7,6,AAL_L2_SPECPKT_TYPE_ARP,7,CA_FLOW_ID_DS_ARP);
            __special_packet_default_rule_set(0,0x7,6,AAL_L2_SPECPKT_TYPE_DHCP,6,CA_FLOW_MAX);
            //__special_packet_default_rule_set(0,0x20,7,AAL_L2_SPECPKT_TYPE_IGMP_MLD);
            __special_packet_default_rule_set(0,0x7,6,AAL_L2_SPECPKT_TYPE_ICMP,6,CA_FLOW_ID_DS_ICMP);

            //__special_packet_default_rule_set(0,0x20,6,AAL_L2_SPECPKT_TYPE_ARP,7,CA_FLOW_ID_DS_ARP);
            __special_packet_default_rule_set(0,0x20,6,AAL_L2_SPECPKT_TYPE_DHCP,6,CA_FLOW_ID_DS_DHCP);
            //__special_packet_default_rule_set(0,0x20,7,AAL_L2_SPECPKT_TYPE_IGMP_MLD);
            __special_packet_default_rule_set(0,0x20,6,AAL_L2_SPECPKT_TYPE_ICMP,6,CA_FLOW_ID_DS_ICMP);
   
            memset(&key,0,sizeof(ca_classifier_key_t));
            memset(&key_mask,0,sizeof(ca_classifier_key_mask_t));
            memset(&action,0,sizeof(action));
        
            key_mask.src_port = 1;
            key.src_port = 0x20007;
            key_mask.key_handle = 1;
            key_mask.l2 = 1;
            key_mask.l2_mask.mac_da = 1;
            memcpy(key.l2.mac_da.mac_min,bc_mac,CA_ETH_ADDR_LEN);
            memcpy(key.l2.mac_da.mac_max,bc_mac,CA_ETH_ADDR_LEN);
            action.forward =CA_CLASSIFIER_FORWARD_FE;
            action.options.masks.priority = 1;
            action.options.priority = 0;
            l2_classifier_rule_add(0,6,&key,&key_mask,&action,&index);
            key.key_handle.llid_cos_index = 0xFF;
            l2_classifier_rule_add(0,6,&key,&key_mask,&action,&index);
        
            __cls_rate_limit_t(0,CA_FLOW_ID_US_ARP, 50,10,1);
            __cls_rate_limit_t(0,CA_FLOW_ID_US_DHCP, 50,10,1);
            __cls_rate_limit_t(0,CA_FLOW_ID_US_MY_MAC, 50,10,1);
            __cls_rate_limit_t(0,CA_FLOW_ID_DS_DHCP, 50,10,1);
            __cls_rate_limit_t(0,CA_FLOW_ID_DS_IGMP, 32,10,1);

    }
    
 #if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)   
   // __trap_sp_frame_rule_to_cpu_init();
 #endif
 
    
    return;
}

#if defined(CONFIG_LUNA_G3_SERIES)
#define L2_CLS_IPV4_KEY_USED_PRINT(cfg) {\
	if(cfg.ipv4_ecn_vld) printk(KERN_INFO "| ipv4_ecn = %x\r\n", cfg.ipv4_ecn);\
	if((cfg.mac_field_ctrl != 3) || ((cfg.mac_field_ctrl == 3) && (cfg.mac_da[0] & cfg.mac_da[1] & cfg.mac_da[2] & cfg.mac_da[3] & cfg.mac_da[4] & cfg.mac_da[5] & cfg.mac_sa[0] & cfg.mac_sa[2] & cfg.mac_sa[3] & cfg.mac_sa[4] & cfg.mac_sa[5])))\
		printk(KERN_INFO "| mac_field_ctrl(1:da_range;2:sa_range;3:mask) = %x, mac_da = %02x:%02x:%02x:%02x:%02x:%02x, mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\r\n", cfg.mac_field_ctrl, cfg.mac_da[0],cfg.mac_da[1],cfg.mac_da[2],cfg.mac_da[3],cfg.mac_da[4],cfg.mac_da[5], cfg.mac_sa[0],cfg.mac_sa[1],cfg.mac_sa[2],cfg.mac_sa[3],cfg.mac_sa[4],cfg.mac_sa[5]);\
	if((cfg.vlan_field_ctrl != 3) || ((cfg.vlan_field_ctrl == 3) && (cfg.top_is_svlan || cfg.inner_is_svlan || cfg.top_vlan_id || cfg.inner_vlan_id)))\
		printk(KERN_INFO "| vlan_field_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_is_svlan = %x, inner_is_svlan = %x, top_vlan_id = %x, inner_vlan_id = %x\r\n", cfg.vlan_field_ctrl, cfg.top_is_svlan, cfg.inner_is_svlan, cfg.top_vlan_id, cfg.inner_vlan_id);\
	if((cfg.dot1p_ctrl != 3) || ((cfg.dot1p_ctrl == 3) && (cfg.top_dot1p || cfg.inner_dot1p)))\
		printk(KERN_INFO "| dot1p_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_dot1p = %x, inner_dot1p = %x\r\n", cfg.dot1p_ctrl, cfg.top_dot1p, cfg.inner_dot1p);\
	if(cfg.vlan_number_vld) printk(KERN_INFO "| vlan_number = %x\r\n", cfg.vlan_number);\
	if(cfg.ethernet_type_vld) printk(KERN_INFO "| ethernet_type = %x\r\n", cfg.ethernet_type);\
	if((cfg.ip_ctrl != 3) || ((cfg.ip_ctrl == 3) && (cfg.ip_sa_mask || cfg.ip_da_mask)))\
		printk(KERN_INFO "| ip_ctrl(1:DA range;2:SA range;3:mask) = %x, ip_da_mask = %x, ip_da = %x, ip_sa_mask = %x, ip_sa = %x\r\n", cfg.ip_ctrl, cfg.ip_da_mask, cfg.ip_da, cfg.ip_sa_mask, cfg.ip_sa);\
	if(cfg.ip_pt_valid) printk(KERN_INFO "| ip_pt = %x\r\n", cfg.ip_pt);\
	if(cfg.dscp_valid) printk(KERN_INFO "| dscp = %x\r\n", cfg.dscp);\
	if(cfg.ip_option_valid) printk(KERN_INFO "| ip_option = %x\r\n", cfg.ip_option);\
	if(cfg.ip_fragment_valid) printk(KERN_INFO "| ip_fragment = %x\r\n", cfg.ip_fragment);\
	if(cfg.l4_dst_port_hi || cfg.l4_dst_port_lo) printk(KERN_INFO "| l4_dst_ctrl(0/3:mask;1:hi;2:range) = %x, l4_dst_port_hi = %x, l4_dst_port_lo = %x\r\n", cfg.l4_dst_ctrl, cfg.l4_dst_port_hi, cfg.l4_dst_port_lo);\
	if(cfg.l4_src_port_hi || cfg.l4_src_port_lo) printk(KERN_INFO "| l4_src_ctrl(0/3:mask;1:hi;2:range) = %x, l4_src_port_hi = %x, l4_src_port_lo = %x\r\n", cfg.l4_src_ctrl, cfg.l4_src_port_hi, cfg.l4_src_port_lo);\
	if(cfg.l4_flag_valid) printk(KERN_INFO "| l4_flag = %x\r\n", cfg.l4_flag);\
	if(cfg.special_pkt_code_valid) printk(KERN_INFO "| special_pkt_code = %x\r\n", cfg.special_pkt_code);\
	if(cfg.global_key_mask) printk(KERN_INFO "| global_key = %x\r\n", cfg.global_key);\
	printk(KERN_INFO "| fields_is_or(0:AND;1:OR) = %x, precedence = %x, cfg.l7_ctrl = %x\r\n", cfg.fields_is_or, cfg.precedence, cfg.l7_ctrl);\
}

#define L2_CLS_IPV6_SHORT_KEY_USED_PRINT(cfg) {\
	if(cfg.ipv6_ecn_valid) printk(KERN_INFO "| ipv6_ecn = %x\r\n", cfg.ipv6_ecn);\
	if((cfg.ipv6_ctrl != 3) || ((cfg.ipv6_ctrl == 3) && (cfg.ipv6_addr_mask)))\
		printk(KERN_INFO "| ipv6_ctrl(1:DA range;2:SA range;3:mask) = %x, ipv6_addr_mask = %x, ipv6_da = %x:%x:%x:%x, ipv6_sa = %x:%x:%x:%x\r\n", cfg.ipv6_ctrl, cfg.ipv6_addr_mask, cfg.ipv6_da[0], cfg.ipv6_da[1], cfg.ipv6_da[2], cfg.ipv6_da[3], cfg.ipv6_sa[0], cfg.ipv6_sa[1], cfg.ipv6_sa[2], cfg.ipv6_sa[3]);\
	if(cfg.ipv6_pt_valid) printk(KERN_INFO "| ipv6_pt = %x\r\n", cfg.ipv6_pt);\
	if(cfg.dscp_valid) printk(KERN_INFO "| dscp = %x\r\n", cfg.dscp);\
	if(cfg.l4_flag_valid) printk(KERN_INFO "| l4_flag = %x\r\n", cfg.l4_flag);\
	if(cfg.special_pkt_code_valid) printk(KERN_INFO "| special_pkt_code = %x\r\n", cfg.special_pkt_code);\
	if(cfg.top_vlan_valid) printk(KERN_INFO "| top_is_svlan = %x, top_vlan_id = %x\r\n", cfg.top_is_svlan, cfg.top_vlan_id);\
	if(cfg.top_dot1p_valid) printk(KERN_INFO "| top_dot1p = %x\r\n", cfg.top_dot1p);\
	if(cfg.vlan_number_vld) printk(KERN_INFO "| vlan_number = %x\r\n", cfg.vlan_number);\
	if((cfg.l4_ctrl != 3) || ((cfg.l4_ctrl == 3) && (cfg.l4_dst_port || cfg.l4_src_port)))\
		printk(KERN_INFO "| l4_ctrl(1:dp_range;2:sp_range;3:mask;4:for EV2PT) = %x, l4_dst_port = %x, l4_src_port = %x\r\n", cfg.l4_ctrl, cfg.l4_dst_port, cfg.l4_src_port);\
	if(cfg.flow_lab_valid) printk(KERN_INFO "| flow_lab = %x\r\n", cfg.flow_lab);\
	printk(KERN_INFO "| fields_is_or(0:AND;1:OR) = %x, precedence = %x\r\n", cfg.fields_is_or, cfg.precedence);\
}

#define L2_CLS_L2R_KEY_USED_PRINT(cfg) {\
	if((cfg.mac_field_ctrl != 3) || ((cfg.mac_field_ctrl == 3) && (cfg.mac_da[0] & cfg.mac_da[1] & cfg.mac_da[2] & cfg.mac_da[3] & cfg.mac_da[4] & cfg.mac_da[5] & cfg.mac_sa[0] & cfg.mac_sa[2] & cfg.mac_sa[3] & cfg.mac_sa[4] & cfg.mac_sa[5])))\
		printk(KERN_INFO "| mac_field_ctrl(1:da_range;2:sa_range;3:mask) = %x, mac_da = %02x:%02x:%02x:%02x:%02x:%02x, mac_sa = %02x:%02x:%02x:%02x:%02x:%02x\r\n", cfg.mac_field_ctrl, cfg.mac_da[0],cfg.mac_da[1],cfg.mac_da[2],cfg.mac_da[3],cfg.mac_da[4],cfg.mac_da[5], cfg.mac_sa[0],cfg.mac_sa[1],cfg.mac_sa[2],cfg.mac_sa[3],cfg.mac_sa[4],cfg.mac_sa[5]);\
	if((cfg.vlan_field_ctrl != 3) || ((cfg.vlan_field_ctrl == 3) && (cfg.top_is_svlan || cfg.inner_is_svlan || cfg.top_vlan_id || cfg.inner_vlan_id)))\
		printk(KERN_INFO "| vlan_field_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_is_svlan = %x, inner_is_svlan = %x, top_vlan_id = %x, inner_vlan_id = %x\r\n", cfg.vlan_field_ctrl, cfg.top_is_svlan, cfg.inner_is_svlan, cfg.top_vlan_id, cfg.inner_vlan_id);\
	if((cfg.dot1p_ctrl != 3) || ((cfg.dot1p_ctrl == 3) && (cfg.top_dot1p || cfg.inner_dot1p)))\
		printk(KERN_INFO "| dot1p_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_dot1p = %x, inner_dot1p = %x\r\n", cfg.dot1p_ctrl, cfg.top_dot1p, cfg.inner_dot1p);\
	if(cfg.vlan_number_vld) printk(KERN_INFO "| vlan_number = %x\r\n", cfg.vlan_number);\
	if(cfg.ethernet_type_vld) printk(KERN_INFO "| ethernet_type = %x\r\n", cfg.ethernet_type);\
	if((cfg.l4_port_field_ctrl != 3) || ((cfg.l4_port_field_ctrl == 3) && (cfg.l4_dst_port || cfg.l4_src_port)))\
		printk(KERN_INFO "| l4_port_field_ctrl(1:dp_range;2:sp_range;3:mask;4:for EV2PT) = %x, l4_dst_port = %x, l4_src_port = %x\r\n", cfg.l4_port_field_ctrl, cfg.l4_dst_port, cfg.l4_src_port);\
	printk(KERN_INFO "| field_is_or(0:AND;1:OR) = %x, precedence = %x\r\n", cfg.field_is_or, cfg.precedence);\
}while(0)

#define L2_CLS_L3R_KEY_USED_PRINT(cfg) {\
	if(cfg.ecn_valid) printk(KERN_INFO "| ecn = %x\r\n", cfg.ecn);\
	if((cfg.ip_ctrl != 3) || ((cfg.ip_ctrl == 3) && (cfg.ip_sa || cfg.ip_da)))\
		printk(KERN_INFO "| ip_ctrl(1:DA range;2:SA range;3:mask) = %x, ip_da = %x, ip_sa = %x\r\n", cfg.ip_ctrl, cfg.ip_da, cfg.ip_sa);\
	if(cfg.ip_pt_valid) printk(KERN_INFO "| ip_pt = %x\r\n", cfg.ip_pt);\
	if(cfg.dscp_valid) printk(KERN_INFO "| dscp = %x\r\n", cfg.dscp);\
	if(cfg.has_ipop_valid) printk(KERN_INFO "| has_ipop = %x\r\n", cfg.has_ipop);\
	if(cfg.ipv4_fragment_valid) printk(KERN_INFO "| ipv4_fragment = %x\r\n", cfg.ipv4_fragment);\
	if(cfg.l4_flag_valid) printk(KERN_INFO "| l4_flag = %x\r\n", cfg.l4_flag);\
	if(cfg.special_pkt_code_valid) printk(KERN_INFO "| special_pkt_code = %x\r\n", cfg.special_pkt_code);\
	if((cfg.vlan_field_ctrl != 3) || ((cfg.vlan_field_ctrl == 3) && (cfg.top_is_svlan || cfg.inner_is_svlan || cfg.top_vlan_id || cfg.inner_vlan_id)))\
		printk(KERN_INFO "| vlan_field_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_is_svlan = %x, inner_is_svlan = %x, top_vlan_id = %x, inner_vlan_id = %x\r\n", cfg.vlan_field_ctrl, cfg.top_is_svlan, cfg.inner_is_svlan, cfg.top_vlan_id, cfg.inner_vlan_id);\
	if((cfg.dot1p_ctrl != 3) || ((cfg.dot1p_ctrl == 3) && (cfg.top_dot1p || cfg.inner_dot1p)))\
		printk(KERN_INFO "| dot1p_ctrl(1:top_range;2:inner_range;3:mask) = %x, top_dot1p = %x, inner_dot1p = %x\r\n", cfg.dot1p_ctrl, cfg.top_dot1p, cfg.inner_dot1p);\
	if(cfg.vlan_number_vld) printk(KERN_INFO "| vlan_number = %x\r\n", cfg.vlan_number);\
	if(cfg.ethernet_type_vld) printk(KERN_INFO "| ethernet_type = %x\r\n", cfg.ethernet_type);\
	if((cfg.l4_port_field_ctrl != 3) || ((cfg.l4_port_field_ctrl == 3) && (cfg.l4_dst_port || cfg.l4_src_port)))\
		printk(KERN_INFO "| l4_port_field_ctrl(1:dp_range;2:sp_range;3:mask;4:for EV2PT) = %x, l4_dst_port = %x, l4_src_port = %x\r\n", cfg.l4_port_field_ctrl, cfg.l4_dst_port, cfg.l4_src_port);\
	printk(KERN_INFO "| field_is_or(0:AND;1:OR) = %x, precedence = %x\r\n", cfg.field_is_or, cfg.precedence);\
}while(0)

#define L2_CLS_CMR_KEY_USED_PRINT(cfg) {\
	if(cfg.ecn_valid) printk(KERN_INFO "| ecn = %x\r\n", cfg.ecn);\
	if(cfg.mac_addr_valid) printk(KERN_INFO "| mac_addr_is_sa(0:da;1:sa) = %x, mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n", cfg.mac_addr_is_sa, cfg.mac_addr[0],cfg.mac_addr[1],cfg.mac_addr[2],cfg.mac_addr[3],cfg.mac_addr[4],cfg.mac_addr[5]);\
	if(cfg.ip_addr_valid) printk(KERN_INFO "| ip_addr_is_sa(0:da;1:sa) = %x, ip_addr = %x\r\n", cfg.ip_addr_is_sa, cfg.ip_addr);\
	if(cfg.ip_pt_valid) printk(KERN_INFO "| ip_pt = %x\r\n", cfg.ip_pt);\
	if(cfg.dscp_valid) printk(KERN_INFO "| dscp = %x\r\n", cfg.dscp);\
	if(cfg.has_ipop_valid) printk(KERN_INFO "| has_ipop = %x\r\n", cfg.has_ipop);\
	if(cfg.l4_flag_valid) printk(KERN_INFO "| l4_flag = %x\r\n", cfg.l4_flag);\
	if(cfg.ipv4_fragment_valid) printk(KERN_INFO "| ipv4_fragment = %x\r\n", cfg.ipv4_fragment);\
	if(cfg.special_pkt_code_valid) printk(KERN_INFO "| special_pkt_code = %x\r\n", cfg.special_pkt_code);\
	if(cfg.vlan_field_valid) printk(KERN_INFO "| top_is_svlan = %x, top_vlan_id = %x\r\n", cfg.top_is_svlan, cfg.top_vlan_id);\
	if(cfg.dot1p_valid) printk(KERN_INFO "| top_dot1p = %x\r\n", cfg.top_dot1p);\
	if(cfg.vlan_number_vld) printk(KERN_INFO "| vlan_number = %x\r\n", cfg.vlan_number);\
	if(cfg.ethernet_type_vld) printk(KERN_INFO "| ethernet_type = %x\r\n", cfg.ethernet_type);\
	if((cfg.l4_port_field_ctrl != 3) || ((cfg.l4_port_field_ctrl == 3) && (cfg.l4_dst_port || cfg.l4_src_port)))\
		printk(KERN_INFO "| l4_port_field_ctrl(1:dp_range;2:sp_range;3:mask;4:for EV2PT) = %x, l4_dst_port = %x, l4_src_port = %x\r\n", cfg.l4_port_field_ctrl, cfg.l4_dst_port, cfg.l4_src_port);\
	printk(KERN_INFO "| field_is_or(0:AND;1:OR) = %x, precedence = %x\r\n", cfg.field_is_or, cfg.precedence);\
}

#define L2_CLS_FIB_USED_PRINT(cfg, sub_idx) {\
	if(cfg.permit_valid_##sub_idx) printk(KERN_INFO "| permit(0:drop;1:permit) = 0x%x\n", cfg.permit_##sub_idx);\
	if(cfg.premarked_##sub_idx) printk(KERN_INFO "| premarked(For color-blind policing, 0:green;1:color aware) = 0x%x\n", cfg.premarked_##sub_idx);\
	if(cfg.top_cmd_valid_##sub_idx) printk(KERN_INFO "| top_vlan_cmd(0:nop;1:push;2:pop;3:swap) = 0x%x, top_vlanid = 0x%x, top_sc_ind(0:cvlan;1:svlan) = 0x%x\n", cfg.top_vlan_cmd_##sub_idx, cfg.top_vlanid_##sub_idx, cfg.top_sc_ind_##sub_idx);\
	if(cfg.inner_cmd_valid_##sub_idx) printk(KERN_INFO "| inner_vlan_cmd(0:nop;1:push;2:pop;3:swap) = 0x%x, inner_vlanid = 0x%x, inner_sc_ind(0:cvlan;1:svlan) = 0x%x\n", cfg.inner_vlan_cmd_##sub_idx, cfg.inner_vlanid_##sub_idx, cfg.inner_sc_ind_##sub_idx);\
	if(cfg.dot1p_valid_##sub_idx) printk(KERN_INFO "| dot1p = 0x%x\n", cfg.dot1p_##sub_idx);\
	if(cfg.dscp_valid_##sub_idx) printk(KERN_INFO "| dscp(v4 dscp or v6 tc) = 0x%x\n", cfg.dscp_##sub_idx);\
	if(cfg.cos_valid_##sub_idx) printk(KERN_INFO "| cos = 0x%x\n", cfg.cos_##sub_idx);\
	if(cfg.dp_valid_##sub_idx) printk(KERN_INFO "| dp_value = 0x%x\n", cfg.dp_value_##sub_idx);\
	if(cfg.mark_ena_##sub_idx) printk(KERN_INFO "| mark_ena = 0x%x, flowid = 0x%x\n", cfg.mark_ena_##sub_idx, cfg.flowid_##sub_idx);\
	if(cfg.dscp_mark_down_##sub_idx) printk(KERN_INFO "| dscp_mark_down = 0x%x\n", cfg.dscp_mark_down_##sub_idx);\
	if(cfg.spt_mode_##sub_idx != 3) printk(KERN_INFO "| spt_mode(SA learn/forward. 0:invalid;1:O/X;2:X/O;3:O/O) = 0x%x\n", cfg.spt_mode_##sub_idx);\
	if(cfg.no_drop_##sub_idx) printk(KERN_INFO "| no_drop = 0x%x\n", cfg.no_drop_##sub_idx);\
}while(0)

ca_status_t l2_classifier_rule_info_get (
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       ca_uint32_t                 index,
    CA_OUT      ca_uint32_t                 *key_type,
    CA_OUT      ca_uint32_t                 *priority)
{
    ca_status_t ret = CA_E_OK;
    __cls_rule_t *l2_cls_rule = NULL;

    if(__cls_sw_table_search(index,&l2_cls_rule) == 0){
        return CA_E_NOT_FOUND;
    }
    *key_type = l2_cls_rule->key_type;
    *priority = l2_cls_rule->pri;
    return ret;
}

static ca_status_t __l2_cls_fib_dump(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t fib_id,
	CA_IN ca_uint32_t fib_sub_id)
{
	ca_status_t ret = CA_E_OK;
	aal_l2_cls_fib_t cls_fib;

	memset(&cls_fib, 0, sizeof(aal_l2_cls_fib_t));
	ret = aal_l2_cls_fib_get(device_id, fib_id, &cls_fib);
	if(ret == CA_E_OK) {
		if(fib_sub_id == 1) {
			L2_CLS_FIB_USED_PRINT(cls_fib, 1);
		} else {
			L2_CLS_FIB_USED_PRINT(cls_fib, 0);
		}
	}

    return ret;
}

ca_status_t aal_l2_cls_used_entry_print(void)
{
    ca_status_t ret = CA_E_OK;
	ca_uint8_t device_id = 0;
	ca_uint32_t index;
	__cls_rule_t *l2_cls_rule = NULL;
	char *print_entry_type, *print_rule_type_0, *print_rule_type_1;
	aal_l2_cls_entry_status_t key_type;
	aal_l2_cls_ipv6_short_key_t cfg_ipv6;
	aal_l2_cls_ipv4_key_t cfg_ipv4;
	aal_l2_cls_l2r_key_t cfg_l2r;
	aal_l2_cls_l3r_key_t cfg_l3r;
	aal_l2_cls_cmr_key_t cfg_cmr;

	printk(KERN_INFO "|===== L2 Classifier Table ===========================================|\n");
	for(index=0; index<128; index++) {

	    ret = aal_l2_cls_entry_status_get(0, index, &key_type);
	    if(ret != CA_E_OK)
	        continue;
		if(!key_type.valid)
			continue;

		switch(key_type.entry_type) {
			case AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY: print_entry_type = "1 HW 1 SW"; break;
			case AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY:
				print_entry_type = "1 HW 2 SW";
				switch(key_type.rule_0) {
					case AAL_L2_CLS_KEY_2RULE_TYPE_INVALID: print_rule_type_0 = "invalid"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE: print_rule_type_0 = "L2R"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE: print_rule_type_0 = "L3R"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE: print_rule_type_0 = "CMR"; break;
					default: print_rule_type_0 = "NA"; break;
				}
				switch(key_type.rule_1) {
					case AAL_L2_CLS_KEY_2RULE_TYPE_INVALID: print_rule_type_1 = "invalid"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE: print_rule_type_1 = "L2R"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE: print_rule_type_1 = "L3R"; break;
					case AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE: print_rule_type_1 = "CMR"; break;
					default: print_rule_type_1 = "NA"; break;
				}
				break;
			case AAL_L2_CLS_KEY_ENTRY_TYPE_16RULE1ENTRY: print_entry_type = " Reserved "; break;
			case AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE2ENTRY: print_entry_type = "2 HW 1 SW"; break;
			default: print_entry_type = "NA"; break;
		}

		if(key_type.entry_type == AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE1ENTRY) {
			printk(KERN_INFO "|index %04d | ipv6_valid %d | %s + %s | %s \n", index, key_type.is_ipv6_rule, print_rule_type_0, print_rule_type_1, print_entry_type);
			if(key_type.is_ipv6_rule) {
				memset(&cfg_ipv6,0x00,sizeof(cfg_ipv6));
				ret = aal_l2_cls_ipv6_short_key_get(device_id, index, &cfg_ipv6);
				if(ret == CA_E_OK)
					L2_CLS_IPV6_SHORT_KEY_USED_PRINT(cfg_ipv6);
			} else {
				memset(&cfg_ipv4,0x00,sizeof(cfg_ipv4));
				ret = aal_l2_cls_ipv4_key_get(device_id, index, &cfg_ipv4);
				if(ret == CA_E_OK)
					L2_CLS_IPV4_KEY_USED_PRINT(cfg_ipv4);
			}
			printk(KERN_INFO "|+ FIB (HW %04d) +\n", index, 0);
			ret = __l2_cls_fib_dump(device_id, index, 0);
			if(ret != CA_E_OK)
				printk(KERN_INFO "|ERROR!\n");
			printk(KERN_INFO "|------------------------------------------------------------------|\n");\
		} else if(key_type.entry_type == AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY) {
			printk(KERN_INFO "|index %04d | sub_idx 0 | ipv6_valid %d | %s + %s | %s \n", index, key_type.is_ipv6_rule, print_rule_type_0, print_rule_type_1, print_entry_type);

			/* 1st Rule */
			switch(key_type.rule_0) {
				case AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE:
					memset(&cfg_l2r,0x00,sizeof(cfg_l2r));
					ret = aal_l2_cls_l2r_1st_key_get(device_id, index, &cfg_l2r);
					if(ret == CA_E_OK)
						L2_CLS_L2R_KEY_USED_PRINT(cfg_l2r);
					break;
				case AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE:
					memset(&cfg_l3r,0x00,sizeof(cfg_l3r));
					ret = aal_l2_cls_l3r_1st_key_get(device_id, index, &cfg_l3r);
					if(ret == CA_E_OK)
						L2_CLS_L3R_KEY_USED_PRINT(cfg_l3r);
					break;
				case AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE:
					memset(&cfg_cmr,0x00,sizeof(cfg_cmr));
					ret = aal_l2_cls_cmr_1st_key_get(device_id, index, &cfg_cmr);
					if(ret == CA_E_OK)
						L2_CLS_CMR_KEY_USED_PRINT(cfg_cmr);
					break;
				default: break;
			}

			printk(KERN_INFO "|+ FIB (HW/SUB_IDX %04d/%d) +\n", index, 0);
			ret = __l2_cls_fib_dump(device_id, index, 0);
			if(ret != CA_E_OK)
				printk(KERN_INFO "|ERROR!\n");
			
			printk(KERN_INFO "|index %04d | sub_idx 1 ---------------------------------------------|\n", index);
			
			/* 2nd Rule */
			switch(key_type.rule_1) {
				case AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE:
					memset(&cfg_l2r,0x00,sizeof(cfg_l2r));
					ret = aal_l2_cls_l2r_2nd_key_get(device_id, index, &cfg_l2r);
					if(ret == CA_E_OK)
						L2_CLS_L2R_KEY_USED_PRINT(cfg_l2r);
					break;
				case AAL_L2_CLS_KEY_2RULE_TYPE_L3RULE:
					memset(&cfg_l3r,0x00,sizeof(cfg_l3r));
					ret = aal_l2_cls_l3r_2nd_key_get(device_id, index, &cfg_l3r);
					if(ret == CA_E_OK)
						L2_CLS_L3R_KEY_USED_PRINT(cfg_l3r);
					break;
				case AAL_L2_CLS_KEY_2RULE_TYPE_COMBRULE:
					memset(&cfg_cmr,0x00,sizeof(cfg_cmr));
					ret = aal_l2_cls_cmr_1st_key_get(device_id, index, &cfg_cmr);
					if(ret == CA_E_OK)
						L2_CLS_CMR_KEY_USED_PRINT(cfg_cmr);
					break;
				default: break;
			}
			printk(KERN_INFO "|+ FIB (HW/SUB_IDX %04d/%d) +\n", index, 1);
			ret = __l2_cls_fib_dump(device_id, index, 1);
			if(ret != CA_E_OK)
				printk(KERN_INFO "|ERROR!\n");
			printk(KERN_INFO "|------------------------------------------------------------------|\n");\
		}
		else {	//AAL_L2_CLS_KEY_ENTRY_TYPE_1RULE2ENTRY
			printk(KERN_INFO "|index %04d | ipv6_valid %d | %s \n", index, key_type.is_ipv6_rule, print_entry_type);
			printk(KERN_INFO "| Under development ... \n");
		}

	}

	return CA_E_OK;
}
#endif



