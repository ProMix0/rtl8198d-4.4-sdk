
#include "osal_common.h"
#include "aal_common.h"
#include "aal_fdb.h"
#include "util_table.h"
#include <soc/cortina/registers.h>
#include <soc/cortina/cortina-soc.h>
#include "aal_l2_cls.h"
#include "port.h"
#include "l2.h"
#include "classifier.h"
#include "aal_port.h"
#include "aal_l2_specpkt.h"
#include "aal_l2_te.h"
#include "aal_arb.h"
#include "aal_l2_vlan.h"
#include "ca_event.h"
#include "ca-scfg/scfg.h"
#include "mirror.h"
#include "pkt_tx.h"

#if (defined(CONFIG_ARCH_CORTINA_SATURN_SFU) && !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
#include "aal_fdb_ipc.h"
#endif

#include "special_packet.h"

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  data declaration
%%____________________________________________________________________________*/
ca_table_t *fdb_tbl_p;
ca_table_t *fdb_tbl_svl_p;

sw_lrn_cb_t sw_lrn_cb;

ca_table_t *mac_filter_tbl_p;
ca_uint g_fdb_lock;
ca_boolean_t mac_filter_dft[CA_AAL_FDB_ETH_PORT_NUM];
ca_int32_t aal_fdb_debug = 0;
extern ca_uint32_t l2_aging_time ;
extern ca_uint32_t g_vlan_mode;

aal_port_flooding_t flood_cb;
ca_int32_t fdb_sw_aging_step = 2;
//ca_int32_t sw_lrn_mode = 2;     //TODO
ca_int32_t using_sw_lrn = 0;
ca_int32_t sw_lrn_debug = 0;
ca_int32_t fdb_init_done = 0;

ca_status_t aal_fdb_delete_oldest_entry_by_port(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              lpid);

#ifndef AAL_FDB_ENABLE_PE_MAC_LRN
ca_status_t __aal_fdb_sw_lrn_tbl_del(void);
#endif

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  macro
%%____________________________________________________________________________*/


#define __FDB_LOCK()    \
    do{ \
        __fdb_acc_lock();\
    }while(0)

#define __FDB_UNLOCK()    \
    do{ \
        __fdb_acc_unlock();\
    }while(0)

#ifdef AAL_FDB_ENABLE_PE_MAC_LRN

#define __FDB_SW_LRN_LOCK()
#define __FDB_SW_LRN_UNLOCK()

#else

#define __FDB_SW_LRN_LOCK()    \
    do{ \
        ca_spin_lock(sw_lrn_cb.sw_lrn_lock);\
    }while(0)

#define __FDB_SW_LRN_UNLOCK()    \
    do{ \
        ca_spin_unlock(sw_lrn_cb.sw_lrn_lock);\
    }while(0)

#endif

#define __FDB_STATIC_MAC_LOCK()    \
    do{ \
        ca_spin_lock(sw_lrn_cb.static_mac_lock);\
    }while(0)

#define __FDB_STATIC_MAC_UNLOCK()    \
    do{ \
        ca_spin_unlock(sw_lrn_cb.static_mac_lock);\
    }while(0)

#define __FDB_OVERFLOW_MAC_LOCK()    \
    do{ \
        ca_spin_lock(sw_lrn_cb.overflow_lock);\
    }while(0)
#define __FDB_OVERFLOW_MAC_UNLOCK()    \
    do{ \
        ca_spin_unlock(sw_lrn_cb.overflow_lock);\
    }while(0)
extern ca_status_t l2_classifier_rule_add (
        CA_IN      ca_device_id_t                           device_id,
        CA_IN      ca_uint32_t                          priority,
        CA_IN      ca_classifier_key_t                  *key,
        CA_IN      ca_classifier_key_mask_t             *key_mask,
        CA_IN      ca_classifier_action_t               *action,
        CA_OUT     ca_uint32_t                          *index);
extern ca_status_t l2_classifier_rule_del (
        CA_IN      ca_device_id_t               device_id,
        CA_IN      ca_uint32_t                  index);

ca_status_t __aal_fdb_send_dumb_pkt(ca_device_id_t device_id, ca_uint32_t len, ca_port_id_t dst_port, ca_uint8_t use_dev);
/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  functions
%%____________________________________________________________________________*/
void __fdb_acc_lock(void)
{
    ca_spin_lock(g_fdb_lock);
}

void __fdb_acc_unlock(void)
{
    ca_spin_unlock(g_fdb_lock);
}
#define AAL_FDB_RETRY_TIMES 1000

static ca_status_t __aal_fdb_access(
    ca_device_id_t     device_id,
    ca_uint32       op_cmd,
    ca_uint32       *WORD4,
    ca_uint32       *WORD3,
    ca_uint32       *WORD2,
    ca_uint32       *WORD1,
    ca_uint32       *WORD0,
    ca_uint32       *DATA3,
    ca_uint32       *DATA2,
    ca_uint32       *DATA1,
    ca_uint32       *DATA0,
    ca_uint32       *cmd_return
)
{
    L2FE_L2E_FDB_ACCESS_t l2_fdb_access;
    L2FE_L2E_FDB_CMD_RETURN_t cmd_ret;
    ca_uint32_t               timeout = 0;
    ca_uint32_t               retry = 0;

    /* If busy, just return error */
    if (device_id == 0)
        l2_fdb_access.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2_fdb_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
    }
#endif

    if (l2_fdb_access.bf.access) {
        L2_DEBUG("op:0x%x, HW BUSY, wrd:0x%08x\r\n", op_cmd, l2_fdb_access.wrd);
        __aal_fdb_send_dumb_pkt(0, 4096, 0x1f, 1);

        return CA_E_BUSY;
    }

    cmd_ret.wrd = 0;
    while(1){
    l2_fdb_access.wrd = 0x80000000 | op_cmd;

    __FDB_LOCK();

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_FDB_CMD_RETURN, 0); /* init cmd return */
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_CMD_RETURN); /* init cmd return */
    }
#endif

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_INIT:
            /* Set nothing */
            break;

        case __FDB_TBL_OPCODE_SCAN:
            if (WORD0 == NULL ||
                DATA0 == NULL ||
                DATA1 == NULL ||
                DATA2 == NULL ||
                DATA3 == NULL) {
                __FDB_UNLOCK();
                return CA_E_PARAM;
            }

            l2_fdb_access.wrd |= (*WORD0 & 0x1FFF) << 8; /* start addr */
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA0, 0); /* init data */
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA1, 0);
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA2, 0);
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA3, 0);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_DATA0); /* init data */
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_DATA1);
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_DATA2);
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_DATA3);
            }
#endif

            break;

            /* go through */
        case __FDB_TBL_OPCODE_READ:
        case __FDB_TBL_OPCODE_APPEND:
        case __FDB_TBL_OPCODE_APPEND_REPLACE:
        case __FDB_TBL_OPCODE_DELETE:
            if (DATA0 == NULL ||
                DATA1 == NULL ||
                DATA2 == NULL ||
                DATA3 == NULL) {
                __FDB_UNLOCK();
                return CA_E_PARAM;
            }
#if 1
                    L2_DEBUG2(3, "input: data0:0x%08x\r\n", *DATA0);
                    L2_DEBUG2(3, "       data1:0x%08x\r\n", *DATA1);
                    L2_DEBUG2(3, "       data2:0x%08x\r\n", *DATA2);
                    L2_DEBUG2(3, "       data3:0x%08x\r\n", *DATA3);
#endif
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA0, *DATA0);
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA1, *DATA1);
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA2, *DATA2);
                FDB_REG_WRITE(L2FE_L2E_FDB_DATA3, *DATA3);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(*DATA0, L2FE_L2E_FDB_DATA0);
                CA_8279_NE_REG_WRITE(*DATA1, L2FE_L2E_FDB_DATA1);
                CA_8279_NE_REG_WRITE(*DATA2, L2FE_L2E_FDB_DATA2);
                CA_8279_NE_REG_WRITE(*DATA3, L2FE_L2E_FDB_DATA3);
            }
#endif
            break;

        case __FDB_TBL_OPCODE_FLUSH:
        case __FDB_TBL_OPCODE_FLUSH_OLDEST:
            /*
            if (WORD0 == NULL ||
                WORD1 == NULL ||
                WORD2 == NULL ||
                WORD3 == NULL ||
                WORD4 == NULL) {
                __FDB_UNLOCK();
                return CA_E_PARAM;
            }

            FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD0, *WORD0);
            FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD1, *WORD1);
            FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD2, *WORD2);
            FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD3, *WORD3);
            FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD4, *WORD4);
            */
            break;

        default:
            L2_DEBUG("not support\r\n");
            __FDB_UNLOCK();
            return CA_E_NOT_SUPPORT;
    }

    /* Start operation command */
#if 0
/*
    if (op_cmd == __FDB_TBL_OPCODE_SCAN) {
        l2_fdb_access.wrd |= (*WORD0 << 3);
    }
*/

    /* write bit */
    if (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_DELETE) {
        l2_fdb_access.wrd |= 0x40000000;
    }
#endif



        L2_DEBUG2(3, "access wrd:0x%08x\r\n", l2_fdb_access.wrd);
    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_FDB_ACCESS, l2_fdb_access.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(l2_fdb_access.wrd, L2FE_L2E_FDB_ACCESS);
    }
#endif

    do {
        if (device_id == 0){
            l2_fdb_access.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            l2_fdb_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
        }
#endif

        if (timeout++ >= CA_AAL_FDB_TBL_OP_TIMEOUT) {
                if(sw_lrn_debug) ca_printf("0x%x timeout, retry:%d\r\n", op_cmd, retry);
                L2_DEBUG("0x%x timeout, retry:%d\r\n", op_cmd, retry);
                timeout = 0;
                retry++;
                __aal_fdb_send_dumb_pkt(0, 4096, 0x1f, 1);
            }
            if (retry > AAL_FDB_RETRY_TIMES){
            __FDB_UNLOCK();
            return CA_E_TIMEOUT;
        }

    } while (l2_fdb_access.bf.access);
        if (l2_fdb_access.bf.access == 0){
            break;
        }
    }

    /* get return code and feedback */
    if (device_id == 0){
        cmd_ret.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        cmd_ret.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
    }
#endif
    switch (op_cmd) {
        case __FDB_TBL_OPCODE_INIT:
        case __FDB_TBL_OPCODE_APPEND:
        case __FDB_TBL_OPCODE_APPEND_REPLACE:
        case __FDB_TBL_OPCODE_DELETE:
        case __FDB_TBL_OPCODE_FLUSH:
        case __FDB_TBL_OPCODE_FLUSH_OLDEST:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_READ:
        case __FDB_TBL_OPCODE_SCAN:

            if (cmd_ret.bf.status == CA_AAL_FDB_ACCESS_HIT) {
                if (device_id == 0){
                    *DATA0 = CA_NE_REG_READ(L2FE_L2E_FDB_DATA0);
                    *DATA1 = CA_NE_REG_READ(L2FE_L2E_FDB_DATA1);
                    *DATA2 = CA_NE_REG_READ(L2FE_L2E_FDB_DATA2);
                    *DATA3 = CA_NE_REG_READ(L2FE_L2E_FDB_DATA3);
                }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
                else if (device_id == 1){
                    *DATA0 = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA0);
                    *DATA1 = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA1);
                    *DATA2 = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA2);
                    *DATA3 = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA3);
                }
#endif                //*WORD0 = cmd_ret.wrd;
            #if 0
                L2_DEBUG("output: data0:0x%08x\r\n", *DATA0);
                L2_DEBUG("        data1:0x%08x\r\n", *DATA1);
                L2_DEBUG("        data2:0x%08x\r\n", *DATA2);
                L2_DEBUG("        data3:0x%08x\r\n", *DATA3);
            #endif
            }

            break;

        default:
            break;
    }

    __FDB_UNLOCK();

    if(__FDB_TBL_OPCODE_FLUSH == op_cmd && sw_lrn_cb.flush_cfg.type != SW_LRN_MAC_TMP){
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        aal_fdb_ipc_flush_send(&sw_lrn_cb.flush_cfg);
#else
        __aal_fdb_sw_lrn_tbl_del();
#endif
    }

    L2_DEBUG2(3, "cmd_ret.wrd:0x%08x\r\n", cmd_ret.wrd);

    if (cmd_return)
        *cmd_return = cmd_ret.wrd;

    return CA_E_OK;

}

#if 0
static ca_status_t __aal_fdb_access_to_speed(
    ca_device_id_t     device_id,
    ca_uint32       op_cmd,
    ca_uint32       *DATA7,
    ca_uint32       *DATA6,
    ca_uint32       *DATA5,
    ca_uint32       *DATA4,
    ca_uint32       *DATA3,
    ca_uint32       *DATA2,
    ca_uint32       *DATA1,
    ca_uint32       *DATA0,
    ca_uint32       *cmd_return
)
{
    FE_PR_FE_L2_CPU_OPERATION_t reg_l2_cpu_op;

    ca_uint32                   timeout = 0;

    if ((op_cmd > __FDB_TBL_OPCODE_CNT_DEC) ||
        (op_cmd == __FDB_TBL_OPCODE_RESV0) ||
        (NULL == cmd_return)) {
        return CA_E_PARAM;
    }

    /* If busy, just return error */
    CA_NE_REG_READ(FE_PR_FE_L2_CPU_OPERATION, reg_l2_cpu_op.wrd);

    if (reg_l2_cpu_op.wrd & 0x80000000) {
        return CA_E_BUSY;
    }

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_INIT:
            /* Set nothing */
            break;

        case __FDB_TBL_OPCODE_APPEND:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA1 == NULL ||
                DATA2 == NULL ||
                DATA3 == NULL ||
                DATA4 == NULL ||
                DATA5 == NULL ||
                DATA6 == NULL) {
                return CA_E_PARAM;
            }

            /* set the related elements to create new fdb entry */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, *DATA0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA1, *DATA1);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA2, *DATA2);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA3, *DATA3);

            /* set the related elements to create hash key */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA5, *DATA5);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA6, *DATA6);

            break;

        case __FDB_TBL_OPCODE_READ:

            /* parameters check */
            if (DATA4 == NULL ||
                DATA5 == NULL ||
                DATA6 == NULL) {
                return CA_E_PARAM;
            }

            /* rzhou add just to clear, can be deleted when debug finished */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA1, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA2, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA3, 0);

            /* set the related elements to create hash key */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA5, *DATA5);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA6, *DATA6);

            break;

        case __FDB_TBL_OPCODE_DELETE:

            /* parameters check */
            if (DATA4 == NULL ||
                DATA5 == NULL ||
                DATA6 == NULL) {

                return CA_E_PARAM;
            }

            /* set the related elements to create hash key */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA5, *DATA5);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA6, *DATA6);

            break;

        case __FDB_TBL_OPCODE_SCAN:
            if (DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* rzhou add just to clear, can be deleted when debug finished */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA1, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA2, 0);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA3, 0);

            /* set offset address */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA5, *DATA5);

            break;

        case __FDB_TBL_OPCODE_FLUSH:

            /* parameters check */
            if (DATA4 == NULL ||
                DATA5 == NULL ||
                DATA6 == NULL ||
                DATA7 == NULL) {
                return CA_E_PARAM;
            }

            /* set programmed rule */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA5, *DATA5);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA6, *DATA6);
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA7, *DATA7);

            break;

        case __FDB_TBL_OPCODE_CNT_INC:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set inc cnt */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_CNT_INIT:

            /* parameters check */
            if ( DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_CNT_WR:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set counter to be writen */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_CNT_RD:

            /* parameters check */
            if (DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_LMT_WR:

            /* parameters check */
            if (DATA0 == NULL || DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set limit */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_LMT_RD:

            /* parameters check */
            if (DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_LMT_INIT:

            /* parameters check */
            if (DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        case __FDB_TBL_OPCODE_CNT_DEC:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA4 == NULL) {
                return CA_E_PARAM;
            }

            /* set dec cnt */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            /* set target port/sub id */
            FDB_REG_WRITE(FE_PR_FE_L2_CPU_DATA4, *DATA4);

            break;

        default:
            break;
    }

    /* Start operation command */
    reg_l2_cpu_op.wrd = 0x80000000 | op_cmd;

    FDB_REG_WRITE(FE_PR_FE_L2_CPU_OPERATION, reg_l2_cpu_op.bf.op_cmd);

    do {
        CA_NE_REG_READ(FE_PR_FE_L2_CPU_OPERATION, reg_l2_cpu_op.bf.op_cmd);

        if (timeout++ >= __FDB_TBL_OPER_TIMEOUT) {
            return CA_E_TIMEOUT;
        }

    }
    while (reg_l2_cpu_op.wrd & 0x80000000);

    /* Get return code and feedback */
    CA_NE_REG_READ(FE_PR_FE_L2_CPU_CMD_RETURN, *cmd_return);

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_INIT:

            /* nothing feedback */
        case __FDB_TBL_OPCODE_APPEND:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_READ:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA1 == NULL ||
                DATA2 == NULL ||
                DATA3 == NULL) {
                return CA_E_PARAM;
            }

            /* set the related elements to create new fdb entry */
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA0, *DATA0);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA1, *DATA1);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA2, *DATA2);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA3, *DATA3);

            break;

        case __FDB_TBL_OPCODE_DELETE:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_SCAN:

            /* parameters check */
            if (DATA0 == NULL ||
                DATA1 == NULL ||
                DATA2 == NULL ||
                DATA3 == NULL) {
                return CA_E_PARAM;
            }

            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA0, *DATA0);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA1, *DATA1);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA2, *DATA2);
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA3, *DATA3);

            break;

        case __FDB_TBL_OPCODE_FLUSH:
        case __FDB_TBL_OPCODE_RESV0:
        case __FDB_TBL_OPCODE_CNT_INC:
        case __FDB_TBL_OPCODE_CNT_INIT:
        case __FDB_TBL_OPCODE_CNT_WR:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_CNT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                return CA_E_PARAM;
            }

            /* Get the read counter */
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            break;

        case __FDB_TBL_OPCODE_LMT_WR:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_LMT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                return CA_E_PARAM;
            }

            /* Get the read limit */
            CA_NE_REG_READ(FE_PR_FE_L2_CPU_DATA0, *DATA0);

            break;

        case __FDB_TBL_OPCODE_LMT_INIT:
        case __FDB_TBL_OPCODE_CNT_DEC:
            /* nothing feedback */
            break;

        default:
            break;
    }

    return CA_E_OK;
}
#endif


void
octet_dump( char *tp, unsigned char *dp, int len )
{
  unsigned char *cp = dp;
  int  ii;
  int  jj;

  printk("%s\n", tp);
  for ( ii=len; ii>16; ii-=16 )
  {
    for ( jj=0; jj<16; jj++, cp++ )
      printk("%02x ", *cp);
    printk("\n");
  }
  for ( jj=0; jj<ii; jj++, cp++ )
  printk("%02x ", *cp);
  printk("\n");
}

ca_status_t __aal_fdb_sc_ind_get(
    ca_device_id_t device_id,
    ca_uint32_t lpid,
    ca_boolean_t *sc_ind)
{
    aal_ilpb_cfg_t ilpb_cfg;
    ca_status_t  ret = CA_E_OK;

    CA_ASSERT_RET(sc_ind != NULL, CA_E_PARAM);

    *sc_ind = 0;
    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lpid, &ilpb_cfg);
    if (ret != CA_E_OK) {
        return 0;
    }

    if(ilpb_cfg.top_s_tpid_enable == 1 && ilpb_cfg.top_c_tpid_enable == 0){
        *sc_ind = 1;

    }else if(ilpb_cfg.top_s_tpid_enable == 0 && ilpb_cfg.top_c_tpid_enable == 1){
        *sc_ind = 0;

    }else{
        *sc_ind = 1;
    }

    return ret;
}

ca_status_t __aal_l2e_cpu_op_mode(ca_device_id_t device_id, ca_uint8 mode)
{
    L2FE_L2E_CTRL_t           l2e_ctrl;

    if (device_id == 0){
        l2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2e_ctrl.wrd = CA_8279_NE_REG_READ(L2FE_L2E_CTRL);
    }
#endif

    if (mode == CA_AAL_L2E_INDIRECT_MODE) {
        l2e_ctrl.bf.cpu_opt_mode = CA_AAL_L2E_INDIRECT_MODE;
        if (device_id == 0){
            FDB_REG_WRITE(L2FE_L2E_CTRL, l2e_ctrl.wrd);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            CA_8279_NE_REG_WRITE(l2e_ctrl.wrd, L2FE_L2E_CTRL);
        }
#endif
    }
    else {
        l2e_ctrl.bf.cpu_opt_mode = CA_AAL_L2E_CMD_MODE;
        if (device_id == 0){
            FDB_REG_WRITE(L2FE_L2E_CTRL, l2e_ctrl.wrd);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            CA_8279_NE_REG_WRITE(l2e_ctrl.wrd, L2FE_L2E_CTRL);
        }
#endif
    }

    return CA_E_OK;
}
ca_status_t __aal_port_mac_cnt(
    ca_device_id_t     device_id,
    ca_int32        port_id,
    ca_uint32       op_cmd,
    ca_uint32       *DATA0,
    ca_uint32       *cmd_return)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS_t mac_cnt_access;
//    L2FE_L2E_FDB_CMD_RETURN_t cmd_ret;
    ca_uint32                   timeout = 0;
    ca_uint32_t                 retry = 0;

    if (cmd_return)
        *cmd_return = CA_AAL_L3E_CNT_LMT_FAIL;

    /* If busy, just return error */
    if (device_id == 0){
        mac_cnt_access.wrd = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        mac_cnt_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#endif

    if (mac_cnt_access.bf.access) {
        return CA_E_BUSY;
    }

    L2_DEBUG("port_id:%d, op_cmd:0x%x\r\n", port_id, op_cmd);

    mac_cnt_access.wrd = 0;
    mac_cnt_access.bf.access = 1;
    mac_cnt_access.bf.addr = port_id;

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_CNT_INC:

            /* parameters check */
            /*if (DATA0 == NULL) {
                return CA_E_PARAM;
            }*/

            mac_cnt_access.wrd |= CA_AAL_L2E_CNT_INC;
            /* inc cnt */
            //FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_ACCESS, mac_cnt_access.wrd);

            break;

        case __FDB_TBL_OPCODE_CNT_DEC:

            /* parameters check */
            /*if (DATA0 == NULL) {
                return CA_E_PARAM;
            }*/

            mac_cnt_access.wrd |= CA_AAL_L2E_CNT_DEC;

            /* dec cnt */
            //FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_ACCESS, mac_cnt_access.wrd);

            break;

        case __FDB_TBL_OPCODE_CNT_INIT:

            /* parameters check */
            /*if (DATA0 == NULL) {
                return CA_E_PARAM;
            }*/

            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            /* set counter to be writen */
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_DATA, 0);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif
            mac_cnt_access.bf.rbw = 1;

            break;

        case __FDB_TBL_OPCODE_CNT_WR:

            /* parameters check */
            if (DATA0 == NULL) {
                L2_DEBUG("empty data\r\n");
                return CA_E_PARAM;
            }

            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            /* set counter to be writen */
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_DATA, *DATA0);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(*DATA0, L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif

            mac_cnt_access.bf.rbw = 1;
            break;

        case __FDB_TBL_OPCODE_CNT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                L2_DEBUG("empty data\r\n");
                return CA_E_PARAM;
            }

            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            mac_cnt_access.bf.rbw = 0;

            break;

        default:
            break;
    }

    L2_DEBUG("cnt access wrd:0x%08x\r\n", mac_cnt_access.wrd);

    /* Start operation command */
    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS, mac_cnt_access.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(mac_cnt_access.wrd, L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#endif

    do {
        if (device_id == 0){
            mac_cnt_access.wrd = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            mac_cnt_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
        }
#endif

        if (timeout++ >= CA_AAL_FDB_TBL_OP_TIMEOUT) {
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_CMD_MODE);
            return CA_E_TIMEOUT;
        }

        if (timeout++ >= CA_AAL_FDB_TBL_OP_TIMEOUT) {
            L2_DEBUG("0x%x timeout, retry:%d\r\n", op_cmd, retry);
            timeout = 0;
            retry++;
            ca_printf("0x%x T%d\r\n", op_cmd, retry);
            //__FDB_UNLOCK();
            __aal_fdb_send_dumb_pkt(0, 4096, 0x1f, 1);
            //__FDB_LOCK();
    }
        if (retry > AAL_FDB_RETRY_TIMES){
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_CMD_MODE);
            __FDB_UNLOCK();
            return CA_E_TIMEOUT;
        }
    }while (mac_cnt_access.bf.access);

    /* Get return code and feedback */
    //cmd_ret.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);

    switch (op_cmd) {

        case __FDB_TBL_OPCODE_CNT_INC:
        case __FDB_TBL_OPCODE_CNT_INIT:
        case __FDB_TBL_OPCODE_CNT_WR:
        case __FDB_TBL_OPCODE_CNT_DEC:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_CNT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                L2_DEBUG("empty data\r\n");
                return CA_E_PARAM;
            }

            /* Get the read counter */
            if (device_id == 0){
                *DATA0 = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                *DATA0 = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif
            break;

        default:
            break;
    }

    /* restore csr_l2e_cpu_intf_mode to cmd mode */
    __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_CMD_MODE);


    if (cmd_return)
        *cmd_return = CA_AAL_L3E_CNT_LMT_SUCCESS;

    L2_DEBUG("cmd_return:%d\r\n", *cmd_return);

    return CA_E_OK;
}

ca_status_t __aal_port_mac_lmt(
    ca_device_id_t     device_id,
    ca_int32        port_id,
    ca_uint32       op_cmd,
    ca_uint32       *DATA0,
    ca_uint32       *cmd_return)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS_t mac_lmt_access;
//    L2FE_L2E_FDB_CMD_RETURN_t cmd_ret;
    ca_uint32                   timeout = 0;
    ca_uint32_t                 retry = 0;

    if (cmd_return)
        *cmd_return = CA_AAL_L3E_CNT_LMT_FAIL;

    /* If busy, just return error */
    if (device_id == 0){
        mac_lmt_access.wrd = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        mac_lmt_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#endif

    if (mac_lmt_access.bf.access) {
        return CA_E_BUSY;
    }

    L2_DEBUG("port_id:%d, op_cmd:0x%x, limit:%d\r\n", port_id, op_cmd, DATA0 ? *DATA0 : 0);

    mac_lmt_access.wrd = 0;
    mac_lmt_access.bf.access = 1;
    mac_lmt_access.bf.addr = port_id;

    /* limit access */
    mac_lmt_access.wrd |= CA_AAL_L2E_LMT_ACCESS;

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_LMT_INC:

            mac_lmt_access.wrd |= CA_AAL_L2E_CNT_INC;
            /* inc cnt */
            //FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_ACCESS, mac_cnt_access.wrd);

            break;

        case __FDB_TBL_OPCODE_LMT_DEC:

            mac_lmt_access.wrd |= CA_AAL_L2E_CNT_DEC;

            /* dec cnt */
            //FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_ACCESS, mac_cnt_access.wrd);

            break;

        case __FDB_TBL_OPCODE_LMT_INIT:
            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            /* set counter to be writen */
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_DATA, 0);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(0, L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif
            mac_lmt_access.bf.rbw = 1;
            break;

        case __FDB_TBL_OPCODE_LMT_WR:

            /* parameters check */
            if (DATA0 == NULL) {
                L2_DEBUG("null data\r\n");
                return CA_E_PARAM;
            }

            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            /* set counter to be writen */
            if (device_id == 0){
                FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_DATA, *DATA0);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                CA_8279_NE_REG_WRITE(*DATA0, L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif
            mac_lmt_access.bf.rbw = 1;


            break;

        case __FDB_TBL_OPCODE_LMT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                L2_DEBUG("null data\r\n");
                return CA_E_PARAM;
            }

            /* set csr_l2e_cpu_intf_mode to indirect mode */
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_INDIRECT_MODE);

            mac_lmt_access.bf.rbw = 0;

            break;

        default:
            break;
    }

    L2_DEBUG("cnt access wrd:0x%08x\r\n", mac_lmt_access.wrd);

    /* Start operation command */
    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS, mac_lmt_access.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(mac_lmt_access.wrd, L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
    }
#endif
    do {
        if (device_id == 0){
            mac_lmt_access.wrd = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            mac_lmt_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS);
        }
#endif
        if (timeout++ >= CA_AAL_FDB_TBL_OP_TIMEOUT) {
            L2_DEBUG("0x%x timeout, retry:%d\r\n", op_cmd, retry);
            timeout = 0;
            retry++;
            ca_printf("0x%x T%d\r\n", op_cmd, retry);
            //__FDB_UNLOCK();
            __aal_fdb_send_dumb_pkt(0, 4096, 0x1f, 1);
            //__FDB_LOCK();
            //return CA_E_TIMEOUT;
        }
        if (retry > AAL_FDB_RETRY_TIMES){
            __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_CMD_MODE);
            __FDB_UNLOCK();
            return CA_E_TIMEOUT;
        }

    }while (mac_lmt_access.bf.access);

    /* Get return code and feedback */
    //cmd_ret.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);

    switch (op_cmd) {
        case __FDB_TBL_OPCODE_LMT_INC:
        case __FDB_TBL_OPCODE_LMT_INIT:
        case __FDB_TBL_OPCODE_LMT_DEC:
            /* nothing feedback */
            break;

        case __FDB_TBL_OPCODE_LMT_WR:
            break;

        case __FDB_TBL_OPCODE_LMT_RD:

            /* parameters check */
            if (DATA0 == NULL) {
                return CA_E_PARAM;
            }

            /* Get the read counter */
            if (device_id == 0){
                *DATA0 = CA_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            else if (device_id == 1){
                *DATA0 = CA_8279_NE_REG_READ(L2FE_L2E_PORT_MAC_CNT_LMT_DATA);
            }
#endif
            break;

        default:
            break;
    }

    /* restore csr_l2e_cpu_intf_mode to cmd mode */
    __aal_l2e_cpu_op_mode(device_id, CA_AAL_L2E_CMD_MODE);

    if (cmd_return)
        *cmd_return = CA_AAL_L3E_CNT_LMT_SUCCESS;

    return CA_E_OK;
}

void __aal_mac_2_fdb_data(
    L2FE_L2E_FDB_DATA3_t *fdb_data3,
    L2FE_L2E_FDB_DATA2_t *fdb_data2,
    L2FE_L2E_FDB_DATA1_t *fdb_data1,
    ca_mac_addr_t mac
)
{
    if (fdb_data1 == NULL || fdb_data2 == NULL || fdb_data3 == NULL || mac == NULL)
        return;

    fdb_data3->bf.mac_addr = (ca_uint32_t)(mac[0] >> 5) & 0x7;
    fdb_data2->bf.mac_addr = (ca_uint32_t)((mac[0] & 0x1F) << 27) | \
                             (mac[1] << 19) | \
                             (mac[2] << 11) | \
                             (mac[3] << 3)  | \
                             ((mac[4] >> 5) & 0x7);
    fdb_data1->bf.mac_addr = (ca_uint32_t)((mac[4] & 0x1F) << 8) | mac[5];

    return;
}

void __aal_fdb_data_2_mac(
    L2FE_L2E_FDB_DATA3_t *fdb_data3,
    L2FE_L2E_FDB_DATA2_t *fdb_data2,
    L2FE_L2E_FDB_DATA1_t *fdb_data1,
    ca_mac_addr_t mac
)
{

    if (fdb_data1 == NULL || fdb_data2 == NULL || fdb_data3 == NULL || mac == NULL)
        return;
    /*
    mac[5] = (ca_uint8_t)((fdb_data3->bf.mac_addr << 6) | (fdb_data2->bf.mac_addr >>
                          26));

    mac[4] = (fdb_data2->bf.mac_addr & 0x03fc0000) >> 18;
    mac[3] = (fdb_data2->bf.mac_addr & 0x0003fc00) >> 10;
    mac[2] = (fdb_data2->bf.mac_addr & 0x000003fc) >> 2;
    mac[1] = ((fdb_data2->bf.mac_addr & 0x00000003) << 6) | \
             ((fdb_data1->bf.mac_addr & 0xfc000000) >> 26);
    mac[0] = (fdb_data1->bf.mac_addr & 0x03fc0000) >> 18;
    */

    mac[0] = (fdb_data3->bf.mac_addr << 5) |
        ((fdb_data2->bf.mac_addr >> 27) & 0x1F);
    mac[1] = (fdb_data2->bf.mac_addr >> 19) & 0xFF;
    mac[2] = (fdb_data2->bf.mac_addr >> 11) & 0xFF;
    mac[3] = (fdb_data2->bf.mac_addr >>  3) & 0xFF;
    mac[4] = ((fdb_data2->bf.mac_addr & 0x7) << 5) | \
             ((fdb_data1->bf.mac_addr >> 8) & 0x1F);
    mac[5] = fdb_data1->bf.mac_addr & 0xFF;


    return;
}


void __aal_fdb_data_2_entry(
    L2FE_L2E_FDB_DATA1_t fdb_data1,
    L2FE_L2E_FDB_DATA0_t fdb_data0,
    aal_fdb_entry_cfg_t  *fdb_entry
)
{
    aal_fdb_data0_t *uc_data0_p;

    if (fdb_entry == NULL){
        L2_DEBUG("NULL pointer\r\n");
        return;
    }

    fdb_entry->key_vid      = fdb_data1.bf.key_vid;
    fdb_entry->key_sc_ind   = fdb_data1.bf.key_scind;
    fdb_entry->dot1p        = fdb_data1.bf.key_dot1p;
    fdb_entry->lrnt_vid     = (fdb_data1.bf.lrned_vid << 9) | fdb_data0.bf.lrned_vid;
    fdb_entry->lrnt_sc_ind  = fdb_data0.bf.lrned_scind;
    fdb_entry->sa_permit    = fdb_data0.bf.sa_permit;
    fdb_entry->da_permit    = fdb_data0.bf.da_permit;
    fdb_entry->static_flag  = fdb_data0.bf.is_static;
    //fdb_entry->aging_sts    = FDB_AGING_TIME(fdb_data0.bf.aging_sts);
    fdb_entry->aging_sts    = fdb_data0.bf.aging_sts;
    fdb_entry->mc_flag      = fdb_data0.bf.mc_flag;

    if (fdb_entry->mc_flag) { /* MC Group ID */
        fdb_entry->port_id  = fdb_data0.bf.mcgid;
        fdb_entry->valid    = 1;
        fdb_entry->tmp_flag = 0;
    } else { /* Port ID */
        uc_data0_p = (aal_fdb_data0_t *)&fdb_data0;
        fdb_entry->port_id = uc_data0_p->bf.lpid;
        fdb_entry->valid = uc_data0_p->bf.entry_valid;
        fdb_entry->tmp_flag = uc_data0_p->bf.tmp_flag;
    }

    return;

}

ca_status_t __aal_fdb_entry_add_del(
    ca_device_id_t             device_id,
    ca_uint32               op_cmd,
    aal_fdb_entry_cfg_t  *fdb
)
{
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;
    aal_fdb_data0_t      *data0_p = (aal_fdb_data0_t *)&fdb_data0;

    L2FE_L2E_FDB_CMD_RETURN_t cmd_return;

    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);

    L2_DEBUG("op_cmd:0x%x, mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, mc:%d, port:%d, dot1p:%d, sa:%d, da:%d, static:%d, aging_sts:%d, tmp:%d\r\n",
              op_cmd,
              fdb->mac[0], fdb->mac[1], fdb->mac[2], fdb->mac[3], fdb->mac[4], fdb->mac[5],
              fdb->key_vid, fdb->mc_flag, fdb->port_id, fdb->dot1p,
              fdb->sa_permit, fdb->da_permit,
              fdb->static_flag, fdb->aging_sts, fdb->tmp_flag);

    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);

    if (fdb->key_vid == 0xffff) fdb->key_vid = 0;

    if (fdb->static_flag > 1 || fdb->dot1p > 7 ||
        fdb->key_sc_ind > 1 || fdb->key_vid > 0xfff  ||
        fdb->da_permit > 1 || fdb->sa_permit > 1 || fdb->mc_flag > 1) {
        L2_DEBUG("invalid data\r\n");
        return CA_E_PARAM;
    }

    /* aging status should not be ZERO for dynamic entry */
    if (fdb->aging_sts == 0 && fdb->static_flag == 0 && op_cmd != __FDB_TBL_OPCODE_DELETE){
        L2_DEBUG("invalid aging sts\r\n");
        return CA_E_PARAM;
    }


    memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));
    memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
    memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
    memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
    memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

    __aal_fdb_sc_ind_get(0, fdb->port_id, &fdb->key_sc_ind);

    L2_DEBUG("port_id:%d, sc_ind:%d\r\n", fdb->port_id, fdb->key_sc_ind);

    __aal_mac_2_fdb_data(&fdb_data3, &fdb_data2, &fdb_data1, fdb->mac);
    fdb_data1.bf.key_vid      = fdb->key_vid;
    fdb_data1.bf.key_scind    = fdb->key_sc_ind;
    fdb_data1.bf.key_dot1p    = fdb->dot1p;
    fdb_data1.bf.lrned_vid    = (fdb->lrnt_vid >> 9) & 0x7;
    fdb_data0.bf.lrned_vid    = fdb->lrnt_vid & 0x1FF;
    fdb_data0.bf.lrned_scind  = fdb->lrnt_sc_ind;
    fdb_data0.bf.sa_permit    = fdb->sa_permit;
    fdb_data0.bf.da_permit    = fdb->da_permit;
    fdb_data0.bf.is_static    = fdb->static_flag;
    fdb_data0.bf.aging_sts    = fdb->aging_sts;
    fdb_data0.bf.mc_flag      = fdb->mc_flag;
    if (fdb->mc_flag)
        fdb_data0.bf.mcgid        = fdb->port_id;
    else{
        data0_p->bf.entry_valid = 1;
        data0_p->bf.lpid = fdb->port_id;

    }

    if (__aal_fdb_access(device_id, op_cmd,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         (ca_uint32 *)&fdb_data3,
                         (ca_uint32 *)&fdb_data2,
                         (ca_uint32 *)&fdb_data1,
                         (ca_uint32 *)&fdb_data0,
                         (ca_uint32 *)&cmd_return) != CA_E_OK) {
        L2_DEBUG("fdb access fail\r\n");
        return CA_E_ERROR;
    }

    /* Check ret */
    if (cmd_return.bf.status != CA_AAL_FDB_APPEND_SUCCESS && cmd_return.bf.status != CA_AAL_FDB_APPEND_SUCCESS_FULL) {
        L2_DEBUG("op fail, status:%d\r\n", cmd_return.bf.status);
        return CA_E_ERROR;
    }

    L2_DEBUG("done\r\n");

    return CA_E_OK;
}

ca_int32 __aal_fdb_tbl_cmp(void *key_x, void *key_y)
{
    aal_tbl_fdb_key_t *x = (aal_tbl_fdb_key_t *)key_x;
    aal_tbl_fdb_key_t *y = (aal_tbl_fdb_key_t *)key_y;

    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);

    if (x->vid < y->vid)
        return -1;

    if (x->vid > y->vid)
        return 1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;
    /*
    if (x->dot1p < y->dot1p)
        return -1;

    if (x->dot1p > y->dot1p)
        return 1;
    */
    return 0;
}

ca_int32 __aal_fdb_tbl_cmp_svl(void *key_x, void *key_y)
{
    aal_tbl_fdb_key_t *x = (aal_tbl_fdb_key_t *)key_x;
    aal_tbl_fdb_key_t *y = (aal_tbl_fdb_key_t *)key_y;

    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;

    return 0;
}


ca_status_t __aal_fdb_tbl_create(void)
{
    fdb_tbl_p = ca_table_create(CA_AAL_TBL_FDB_NAME,
                                CA_RBTREE_TABLE,
                                sizeof(aal_fdb_entry_cfg_t),
                                __aal_fdb_tbl_cmp);
    fdb_tbl_svl_p = ca_table_create(CA_AAL_TBL_FDB_SVL_NAME,
                                CA_RBTREE_TABLE,
                                sizeof(aal_fdb_entry_cfg_t),
                                __aal_fdb_tbl_cmp_svl);

    L2_DEBUG("fdb_tbl_p:%p, svl:%p\r\n", fdb_tbl_p, fdb_tbl_svl_p);

    return CA_E_OK;
}

ca_status_t __aal_fdb_tbl_destory(void)
{
    ca_int32 ret;

    ret = ca_table_destory(fdb_tbl_p);
    ret += ca_table_destory(fdb_tbl_svl_p);

    L2_DEBUG("ret:%d\r\n", ret);

    if (ret == 0) {
        fdb_tbl_p = NULL;
        fdb_tbl_svl_p = NULL;
        return CA_E_OK;
    }
    else
        return CA_E_ERROR;
}

ca_status_t __aal_fdb_tbl_add_del(ca_uint32 op_cmd, aal_fdb_entry_cfg_t *fdb)
{
    aal_tbl_fdb_key_t x;
    ca_int32 ret;

    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);
    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);

    L2_DEBUG2(2, "static_flag:%d, op_cmd:0x%x, fdb_tbl_p:%p/%p\r\n", fdb->static_flag, op_cmd, fdb_tbl_p, fdb_tbl_svl_p);

    if (!fdb->static_flag){
        L2_DEBUG2(2, "is NOT static, don't buffer it\r\n");
        return CA_E_OK;
    }

    memset((void *)&x, 0, sizeof(aal_tbl_fdb_key_t));

    x.dot1p = fdb->dot1p;
    memcpy(x.mac, fdb->mac, CA_ETH_ADDR_LEN);
    x.vid = fdb->key_vid;

    if (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE){
        ret = ca_table_entry_insert(fdb_tbl_p, (void *)&x, (void *)fdb);
        ret += ca_table_entry_insert(fdb_tbl_svl_p, (void *)&x, (void *)fdb);
    }
    else{
        ret = ca_table_entry_remove(fdb_tbl_p, (void *)&x);
        ret += ca_table_entry_remove(fdb_tbl_svl_p, (void *)&x);
    }

    L2_DEBUG2(2, "op: %s, ret:%d\r\n",
             op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ? "add" : "del", ret);

    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}

ca_status_t __aal_fdb_addr_delete_by_port (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          aal_port,
    CA_IN ca_uint32_t           flag
)
{
    aal_fdb_cpu_cmd_word4_msk_t  flush_mask;
    aal_fdb_flush_cfg_t          flush_cfg;
    ca_status_t                  ret = CA_E_OK;

    flush_mask.wrd = ~0U;

    switch (flag) {
    case CA_L2_ADDR_OPERATION_STATIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 1;
        break;
    case CA_L2_ADDR_OPERATION_DYNAMIC:
        flush_mask.bf.static_flag = 0;
        flush_cfg.static_flag     = 0;
        break;
    case CA_L2_ADDR_OPERATION_BOTH:
        flush_mask.bf.static_flag = 1;
        break;
    default:
        return CA_E_PARAM;
    }

    flush_mask.bf.lpid  = 0;
    flush_cfg.lpid_high = aal_port;
    flush_cfg.lpid_low  = aal_port;

    flush_mask.bf.mc_flag = 0;
    flush_cfg.mc_flag = 0;
    ret = aal_fdb_scan_flush_field_set(device_id, flush_mask, &flush_cfg);

    __aal_fdb_flush_type(SW_LRN_MAC_PORT);

    if (CA_E_OK == ret)
        ret = aal_fdb_entry_flush(device_id);

    return ret;

}

//ca_uint8_t fdb_pkt_buf[2048] = {0};
ca_uint8_t fdb_pkt_hdr[] = {0x00, 0x13, 0x25, 0x00, 0x00, 0x02, 0x00, 0x13, 0x25, 0x00, 0x00, 0x01, 0x90, 0x00};
ca_uint32_t fdb_pkt_send = 1;
ca_status_t __aal_fdb_send_dumb_pkt(ca_device_id_t device_id, ca_uint32_t len, ca_port_id_t dst_port, ca_uint8_t use_dev)
{
    ca_pkt_t         pkt;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t      pkt_len = len;

    if (fdb_pkt_send == 0) return ret;

/*    memset(fdb_pkt_buf, 0, sizeof(fdb_pkt_buf));
    memcpy(fdb_pkt_buf, fdb_pkt_hdr, sizeof(fdb_pkt_hdr));
*/
    memset(&pkt, 0, sizeof(pkt));
    pkt.block_count    = 1;
    pkt.device_id      = 0;
    pkt.src_port_id    = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10);
    pkt.dst_port_id    = dst_port;

    pkt.pkt_type       = CA_PKT_TYPE_BPDU;
    pkt.pkt_len        = pkt_len;
    pkt.pkt_data       = (ca_pkt_block_t *)ca_malloc(sizeof(ca_pkt_block_t));
    pkt.pkt_data->len  = pkt_len;
    pkt.pkt_data->data = (ca_uint8_t *)ca_malloc(pkt_len);
    pkt.flags          = CA_TX_BYPASS_FWD_ENGINE;
    memcpy(pkt.pkt_data->data, fdb_pkt_hdr, sizeof(fdb_pkt_hdr));

    if (aal_fdb_debug >= 3)
        ca_buffer_dump("2kpkt", pkt.pkt_data->data, 16);
    //ca_printf("send len %d to 0x%x\r\n", len, dst_port);
    L2_DEBUG("send len %d to 0x%x\r\n", len, dst_port);
    if (use_dev)
        ret = __ca_tx_dying_gasp(device_id, &pkt);
    else
        ;;//ret = __ca_ni_send_single_pkt(&pkt, 0, 0, 0);

    ca_free(pkt.pkt_data->data);
    ca_free(pkt.pkt_data);
    //ca_sleep(10);
    return ret;
}
ca_status_t __aal_fdb_lrn_mode_set(ca_device_id_t device_id, ca_uint32_t lrn_mode)
{
    aal_fdb_ctrl_mask_t  fdb_ctrl_mask;
    aal_fdb_ctrl_t       fdb_ctrl;
    ca_status_t     ret = CA_E_OK;

    CA_ASSERT_RET(lrn_mode <= 3 || lrn_mode >= 0, CA_E_PARAM);

    memset(&fdb_ctrl, 0, sizeof(aal_fdb_ctrl_t));
    memset(&fdb_ctrl_mask, 0, sizeof(aal_fdb_ctrl_mask_t));

    fdb_ctrl_mask.s.lrn_mode = 1;
    fdb_ctrl.lrn_mode = lrn_mode;


    fdb_ctrl_mask.s.aging_mode = 1;

    if (lrn_mode == CA_AAL_FDB_LRN_MODE_HW)
        fdb_ctrl.aging_mode = 0;
    else
        fdb_ctrl.aging_mode = 1;

    ret = aal_fdb_ctrl_set(device_id, fdb_ctrl_mask, &fdb_ctrl);
    L2_DEBUG("lrn mode:%d, ret:%d\r\n", fdb_ctrl.lrn_mode, ret);

    if (lrn_mode == CA_AAL_FDB_LRN_MODE_HW)
        CA_NE_REG_WRITE(2, L2FE_L2E_AGING_CTRL_EXT);
    else
        CA_NE_REG_WRITE(3, L2FE_L2E_AGING_CTRL_EXT);

    return CA_E_OK;
}

ca_status_t __aal_fdb_sw_lrn_mode_get(void)
{
    ca_status_t ret = CA_E_OK;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE)
    return ret;
#else
    ca_device_config_tlv_t scfg_tlv;

    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_FDB_LEARN_MODE;
    ret = ca_device_config_tlv_get(0, &scfg_tlv);
    if (CA_E_OK == ret){
        using_sw_lrn = scfg_tlv.value[0] == 2 ? 1 : 0;
    }

    ca_printf("<%s, %d>using_sw_lrn:%d\r\n", __FUNCTION__, __LINE__, using_sw_lrn);
#endif
}

#ifndef AAL_FDB_ENABLE_PE_MAC_LRN
ca_int32 __aal_fdb_sw_lrn_tbl_cmp(void *key_x, void *key_y)
{
    aal_tbl_fdb_key_t *x = (aal_tbl_fdb_key_t *)key_x;
    aal_tbl_fdb_key_t *y = (aal_tbl_fdb_key_t *)key_y;

    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);

    if (x->vid < y->vid)
        return -1;

    if (x->vid > y->vid)
        return 1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;
    /*
    if (x->dot1p < y->dot1p)
        return -1;

    if (x->dot1p > y->dot1p)
        return 1;
    */
    return 0;
}
ca_status_t __aal_fdb_sw_lrn_tbl_create(void)
{
    ca_status_t ret = CA_E_ERROR;

    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    ret = ca_spin_lock_init(&sw_lrn_cb.sw_lrn_lock, SPINLOCK_BH);

    if (ret != 0) {
        ca_printf("create spin lock sw_lrn_cb.sw_lrn_lock fail\r\n");
    }

    sw_lrn_cb.fdb_sw_lrn_p = ca_table_create(CA_AAL_TBL_FDB_SW_LRN_NAME,
                                CA_RBTREE_TABLE,
                                sizeof(aal_fdb_entry_cfg_t),
                                __aal_fdb_sw_lrn_tbl_cmp);

    if (sw_lrn_cb.fdb_sw_lrn_p == NULL){
        ca_printf("create rb tree sw_lrn_cb.fdb_sw_lrn_p fail\r\n");
        return CA_E_ERROR;
    }

    L2_DEBUG("sw_lrn_cb.fdb_sw_lrn_p:%p\r\n", sw_lrn_cb.fdb_sw_lrn_p);

    return CA_E_OK;
}
ca_status_t __aal_fdb_sw_lrn_tbl_destory(void)
{
    ca_int32 ret;

    __FDB_SW_LRN_LOCK();
    ret = ca_table_destory(sw_lrn_cb.fdb_sw_lrn_p);
    __FDB_SW_LRN_UNLOCK();

    L2_DEBUG("ret:%d\r\n", ret);

    if (ret == 0) {
        sw_lrn_cb.fdb_sw_lrn_p = NULL;
        return CA_E_OK;
    }
    else
        return CA_E_ERROR;
}

ca_status_t __aal_fdb_sw_lrn_tbl_add_del(ca_uint32 op_cmd, aal_fdb_entry_cfg_t *fdb)
{
    aal_tbl_fdb_key_t x;
    ca_int32 ret = CA_E_OK;
    aal_fdb_entry_cfg_t *tmp_p = NULL;

    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);
    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);

    L2_DEBUG("static_flag:%d, op_cmd:0x%x, sw_lrn_cb.fdb_sw_lrn_p:%p\r\n", fdb->static_flag, op_cmd, sw_lrn_cb.fdb_sw_lrn_p);

    /*
    if (!fdb->static_flag){
        L2_DEBUG("is NOT static, don't buffer it\r\n");
        return CA_E_OK;
    }
    */

    memset((void *)&x, 0, sizeof(aal_tbl_fdb_key_t));

    x.dot1p = fdb->dot1p;
    memcpy(x.mac, fdb->mac, CA_ETH_ADDR_LEN);
       x.vid = fdb->lrnt_vid;

    //__FDB_SW_LRN_LOCK();
    if (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE){
        tmp_p = ca_table_entry_search(sw_lrn_cb.fdb_sw_lrn_p, (void *)&x);
        if (tmp_p == NULL){
        ret = ca_table_entry_insert(sw_lrn_cb.fdb_sw_lrn_p, (void *)&x, (void *)fdb);
            if (ret == CA_E_OK){
                sw_lrn_cb.total_mac.mac_cnt++;
                sw_lrn_cb.port_mac[fdb->port_id].mac_cnt++;
                sw_lrn_cb.oflow_time = 0;
            }
        }
    }
    else{
        ret = ca_table_entry_remove(sw_lrn_cb.fdb_sw_lrn_p, (void *)&x);
        if (ret == CA_E_OK){
            if (sw_lrn_cb.total_mac.mac_cnt > 0)
                sw_lrn_cb.total_mac.mac_cnt--;
            if (sw_lrn_cb.port_mac[fdb->port_id].mac_cnt > 0)
                sw_lrn_cb.port_mac[fdb->port_id].mac_cnt--;
        }
    }
    //__FDB_SW_LRN_UNLOCK();

    L2_DEBUG("op: %s, mac: 0x%02x:%02x:%02x:%02x:%02x:%02x, ret:%d, port:%d, cnt:%d\r\n",
             (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE) ? "add" : "del", 
             fdb->mac[0], fdb->mac[1], fdb->mac[2], fdb->mac[3], fdb->mac[4], fdb->mac[5],
             ret, fdb->port_id, sw_lrn_cb.port_mac[fdb->port_id].mac_cnt);

    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}

aal_fdb_entry_cfg_t *__aal_fdb_sw_lrn_entry_get_by_key(
    CA_IN aal_fdb_entry_cfg_t *entry
)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    aal_tbl_fdb_key_t key;

    CA_ASSERT_RET(entry != NULL, NULL);

    memset(&key, 0, sizeof(aal_tbl_fdb_key_t));

    memcpy(key.mac, entry->mac, CA_ETH_ADDR_LEN);
    key.vid = entry->lrnt_vid;

    __FDB_SW_LRN_LOCK();
    tmp_p = (aal_fdb_entry_cfg_t *)ca_table_entry_search(sw_lrn_cb.fdb_sw_lrn_p,
            (void *)&key);
    __FDB_SW_LRN_UNLOCK();

    return tmp_p;
}

aal_fdb_entry_cfg_t *__aal_fdb_sw_lrn_entry_getfirst(void)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    tmp_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getfirst(
                sw_lrn_cb.fdb_sw_lrn_p);
    return tmp_p;
}
ca_int32 __aal_fdb_overflow_mac_cmp(void *key_x, void *key_y)
{
    aal_tbl_fdb_key_t *x = (aal_tbl_fdb_key_t *)key_x;
    aal_tbl_fdb_key_t *y = (aal_tbl_fdb_key_t *)key_y;
    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);
    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;
    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;
    return 0;
}
ca_status_t __aal_fdb_overflow_mac_create(void)
{
    ca_status_t ret = CA_E_ERROR;
    ret = ca_spin_lock_init(&sw_lrn_cb.overflow_lock, SPINLOCK_BH);
    if (ret != 0) {
        ca_printf("create spin lock sw_lrn_cb.overflow_lock fail\r\n");
    }
    sw_lrn_cb.fdb_overflow_p = ca_table_create(CA_AAL_TBL_FDB_OVERFLOW_MAC_NAME,
                                CA_RBTREE_TABLE,
                                sizeof(aal_fdb_entry_cfg_t),
                                __aal_fdb_overflow_mac_cmp);
    if (sw_lrn_cb.fdb_overflow_p == NULL){
        ca_printf("create rb tree sw_lrn_cb.fdb_overflow_p fail\r\n");
        return CA_E_ERROR;
    }
    L2_DEBUG("sw_lrn_cb.fdb_overflow_p:%p\r\n", sw_lrn_cb.fdb_overflow_p);
    return CA_E_OK;
}
ca_status_t __aal_fdb_overflow_mac_destory(void)
{
    ca_int32 ret;
    __FDB_OVERFLOW_MAC_LOCK();
    ret = ca_table_destory(sw_lrn_cb.fdb_overflow_p);
    __FDB_OVERFLOW_MAC_UNLOCK();
    L2_DEBUG("ret:%d\r\n", ret);
    if (ret == 0) {
        sw_lrn_cb.fdb_overflow_p = NULL;
        return CA_E_OK;
    }
    else
        return CA_E_ERROR;
}
ca_status_t __aal_fdb_overflow_mac_add_del(ca_uint32 op_cmd, aal_fdb_entry_cfg_t *fdb)
{
    aal_tbl_fdb_key_t x;
    ca_int32 ret;
    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);
    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);
    L2_DEBUG("static_flag:%d, op_cmd:0x%x, sw_lrn_cb.fdb_overflow_p:%p\r\n", fdb->static_flag, op_cmd, sw_lrn_cb.fdb_overflow_p);
    memset((void *)&x, 0, sizeof(aal_tbl_fdb_key_t));
    x.dot1p = fdb->dot1p;
    memcpy(x.mac, fdb->mac, CA_ETH_ADDR_LEN);
    x.vid = fdb->key_vid;
    __FDB_OVERFLOW_MAC_LOCK();
    if (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE){
        ret = ca_table_entry_insert(sw_lrn_cb.fdb_overflow_p, (void *)&x, (void *)fdb);
    }
    else{
        ret = ca_table_entry_remove(sw_lrn_cb.fdb_overflow_p, (void *)&x);
    }
    __FDB_OVERFLOW_MAC_UNLOCK();
    L2_DEBUG("op: %s, ret:%d\r\n",
             op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ? "add" : "del", ret);
    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}
ca_status_t __aal_fdb_overflow_mac_remove_all(void)
{
    ca_int32 ret;
    __FDB_OVERFLOW_MAC_LOCK();
    ret = ca_table_entry_remove_all(sw_lrn_cb.fdb_overflow_p);
    __FDB_OVERFLOW_MAC_UNLOCK();
    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}
ca_status_t __aal_fdb_overflow_mac_expired(void)
{
    ca_int32 ret;
    ca_uint32_t entry_cnt;
    ca_uint32_t report_cnt;
/*
    ca_rbtree_t *rb_p = NULL;
    ca_table_t *table_p = NULL;

    table_p = sw_lrn_cb.fdb_overflow_p;
    rb_p = (ca_rbtree_t *)table_p->content;
    entry_cnt = ca_rbtree_count(rb_p);
*/

    entry_cnt = aal_fdb_table_get(0, 0x101);

    if (entry_cnt >= 3){
        report_cnt = sw_lrn_cb.total_mac.mac_limit + entry_cnt;
        if (report_cnt > 0xff)
            report_cnt = 0xff;

        /* mac limit + 3 event */
        //ca_printf("<%s, %d>send mac limit + 3 oam event(%d), report_cnt:%d.\r\n", __FUNCTION__, __LINE__, entry_cnt, report_cnt);
        ret = ca_event_send(0, CA_EVENT_MAC_LIMIT_3, (void *)&report_cnt, sizeof(ca_uint32_t));
//        __aal_fdb_overflow_mac_remove_all();
    }

    return CA_E_OK;
}

/* the following function for sw learn, static mac without port */
ca_int32 __aal_fdb_static_mac_cmp(void *key_x, void *key_y)
{
    aal_tbl_fdb_key_t *x = (aal_tbl_fdb_key_t *)key_x;
    aal_tbl_fdb_key_t *y = (aal_tbl_fdb_key_t *)key_y;

    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;

    return 0;
}
ca_status_t __aal_fdb_static_mac_create(void)
{
    ca_status_t ret = CA_E_ERROR;

    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    ret = ca_spin_lock_init(&sw_lrn_cb.static_mac_lock, SPINLOCK_BH);

    if (ret != 0) {
        ca_printf("create spin lock sw_lrn_cb.sw_lrn_lock fail\r\n");
    }

    sw_lrn_cb.fdb_static_p = ca_table_create(CA_AAL_TBL_FDB_STATIC_MAC_NAME,
                                CA_RBTREE_TABLE,
                                sizeof(aal_fdb_entry_cfg_t),
                                __aal_fdb_static_mac_cmp);

    if (sw_lrn_cb.fdb_static_p == NULL){
        ca_printf("create rb tree sw_lrn_cb.fdb_static_p fail\r\n");
        return CA_E_ERROR;
    }

    L2_DEBUG("sw_lrn_cb.fdb_static_p:%p\r\n", sw_lrn_cb.fdb_static_p);

    return CA_E_OK;
}
ca_status_t __aal_fdb_static_mac_destory(void)
{
    ca_int32 ret;

    __FDB_STATIC_MAC_LOCK();
    ret = ca_table_destory(sw_lrn_cb.fdb_static_p);
    __FDB_STATIC_MAC_UNLOCK();

    L2_DEBUG("ret:%d\r\n", ret);

    if (ret == 0) {
        sw_lrn_cb.fdb_static_p = NULL;
        return CA_E_OK;
    }
    else
        return CA_E_ERROR;
}

ca_status_t __aal_fdb_static_mac_add_del(ca_uint32 op_cmd, aal_fdb_entry_cfg_t *fdb)
{
    aal_tbl_fdb_key_t x;
    ca_int32 ret;
    aal_fdb_entry_cfg_t *tmp_p = NULL;

    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);
    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);

    L2_DEBUG("static_flag:%d, op_cmd:0x%x, mac:0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", fdb->static_flag, op_cmd, 
            fdb->mac[0], fdb->mac[1], fdb->mac[2],
            fdb->mac[3], fdb->mac[4], fdb->mac[5]);

    memset((void *)&x, 0, sizeof(aal_tbl_fdb_key_t));

    x.dot1p = fdb->dot1p;
    memcpy(x.mac, fdb->mac, CA_ETH_ADDR_LEN);
    x.vid = fdb->key_vid;

    __FDB_STATIC_MAC_LOCK();
    if (op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE){
        ret = ca_table_entry_insert(sw_lrn_cb.fdb_static_p, (void *)&x, (void *)fdb);
    }
    else{
        ret = ca_table_entry_remove(sw_lrn_cb.fdb_static_p, (void *)&x);
    }

    sw_lrn_cb.cfg = 1;

    __FDB_STATIC_MAC_UNLOCK();

    /* check current dynamic table */
    tmp_p = __aal_fdb_sw_lrn_entry_get_by_key(fdb);
    if (tmp_p){
        /* if found, remove dynamic fdb entry */
        aal_fdb_entry_del(0, tmp_p);
    }

    L2_DEBUG("op: %s, ret:%d\r\n",
             op_cmd == __FDB_TBL_OPCODE_APPEND || op_cmd == __FDB_TBL_OPCODE_APPEND_REPLACE ? "add" : "del", ret);

    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}

aal_fdb_entry_cfg_t *__aal_fdb_static_mac_get_by_key(
    CA_IN aal_fdb_entry_cfg_t *entry
)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    aal_tbl_fdb_key_t key;

    CA_ASSERT_RET(entry != NULL, NULL);

    memset(&key, 0, sizeof(aal_tbl_fdb_key_t));

    memcpy(key.mac, entry->mac, CA_ETH_ADDR_LEN);
    key.vid = entry->lrnt_vid;

    __FDB_STATIC_MAC_LOCK();
    tmp_p = (aal_fdb_entry_cfg_t *)ca_table_entry_search(sw_lrn_cb.fdb_static_p,
            (void *)&key);
    __FDB_STATIC_MAC_UNLOCK();

    return tmp_p;
}

aal_fdb_entry_cfg_t *aal_fdb_sw_lrn_static_mac_next(
    CA_IN ca_uint32_t idx
)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    ca_table_t *table_p = sw_lrn_cb.fdb_static_p;

    CA_ASSERT_RET(table_p != NULL, NULL);

    __FDB_STATIC_MAC_LOCK();

    if (idx == 0){
        tmp_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getfirst(
                    table_p);
    }
    else
        tmp_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getnext(
                table_p);

    __FDB_STATIC_MAC_UNLOCK();

    return tmp_p;
}


ca_status_t __aal_fdb_static_mac_proc(ca_device_id_t device_id, aal_fdb_entry_cfg_t *fdb_entry)
{
    ca_status_t ret = CA_E_OK;
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    ca_uint32_t entry_cnt = 0;
    ca_table_t *table_p = sw_lrn_cb.fdb_static_p;
    ca_rbtree_t *rb_p;

    if (table_p == NULL || table_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return CA_E_EMPTY;
    }
    CA_ASSERT_RET(fdb_entry != NULL, CA_E_PARAM);

    rb_p = (ca_rbtree_t *)table_p->content;

    __FDB_STATIC_MAC_LOCK();

    entry_cnt = ca_rbtree_count(rb_p);
    __FDB_STATIC_MAC_UNLOCK();

    if (entry_cnt == 0){
        L2_DEBUG("empty table\r\n");
        return CA_E_NOT_FOUND;
    }

    tmp_p = (aal_fdb_entry_cfg_t *)__aal_fdb_static_mac_get_by_key(fdb_entry);
    if (tmp_p != NULL){
        L2_DEBUG("find static entry, fill port id and set to HW \r\n");
        fdb_entry->static_flag = 1;
        fdb_entry->sw_update = 1;
        ret = aal_fdb_entry_add(0, fdb_entry);
        if (ret == CA_E_OK){
        tmp_p->port_id = fdb_entry->port_id;
            tmp_p->lrnt_sc_ind = fdb_entry->lrnt_sc_ind;
            tmp_p->lrnt_vid = fdb_entry->lrnt_vid;
            //__aal_fdb_static_mac_add_del(__FDB_TBL_OPCODE_DELETE, fdb_entry);
        }
    }
    else{
        L2_DEBUG("no find\r\n");
        ret = CA_E_NOT_FOUND;
    }
    return ret;
}
void __aal_fdb_sw_lrn_aging_timer_expire(void *data)
{
    ca_status_t ret = CA_E_OK;

    ret = ca_sem_post(sw_lrn_cb.sem);
}


void __aal_fdb_sw_lrn_aging_timer_init(void)
{
    if (!sw_lrn_cb.timer) {
        sw_lrn_cb.timer = ca_circle_timer_add(FDB_SW_AGING_TIMER_INTERVAL,
                                             __aal_fdb_sw_lrn_aging_timer_expire, NULL);

        if (!sw_lrn_cb.timer) {
            L2_DEBUG("sw lrn aging timer add failed.\r\n");
        }
        ca_timer_stop(sw_lrn_cb.timer);
    }
}
ca_status_t __aal_fdb_addr_flush_tmp(
    CA_IN ca_device_id_t             device_id
)
{
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    aal_fdb_flush_cfg_t cfg;
    ca_status_t ret;

    mask.wrd = 0xffffffff;
    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.bf.static_flag = 0;
    cfg.static_flag = 0;

    mask.bf.mc_flag = 0;
    cfg.mc_flag = 0;

    mask.bf.tmp_flag = 0;
    cfg.tmp_flag = 1;

    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    __aal_fdb_flush_type(SW_LRN_MAC_TMP);

    ret = aal_fdb_entry_flush(0);

    ret = __aal_fdb_overflow_mac_remove_all();

    L2_DEBUG("ret:%d\r\n", ret);

    return ret;
}

ca_status_t __aal_fdb_sw_lrn_tbl_del(void)
{
    ca_table_t *table_p = sw_lrn_cb.fdb_sw_lrn_p;
    aal_fdb_entry_cfg_t *tmp_p;
    ca_rbtree_node_t *node_p, *next_p;
    ca_rbtree_t *rb_p;
    ca_status_t ret;
    ca_uint32_t meet[4] = {0};
    aal_fdb_cpu_cmd_word4_msk_t mask;
    aal_fdb_flush_cfg_t cfg;
    flush_type_e        type;

    if (sw_lrn_cb.cfg){
        mask = sw_lrn_cb.flush_cfg.mask;
        cfg = sw_lrn_cb.flush_cfg.cfg;
        type = sw_lrn_cb.flush_cfg.type;
    }
    else{
        L2_DEBUG("no sw_lrn_cb.cfg\r\n");
        return CA_E_EXISTS;
    }

    if (table_p == NULL || table_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return CA_E_EMPTY;
    }

    //__FDB_SW_LRN_LOCK();

    rb_p = (ca_rbtree_t *)table_p->content;
    node_p = ca_rbtree_first(rb_p);

    while (node_p != rb_p->nil) {

        next_p = ca_rbtree_successor(rb_p, node_p);

        if (node_p == next_p){
            L2_DEBUG("node_p:%p == next_p, break out, need check\r\n", node_p, next_p);
            break;
        }

        tmp_p = (aal_fdb_entry_cfg_t *)node_p->data;

        if(tmp_p){
            /*
            L2_DEBUG("mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d, tree cnt:%d, aging_time:%d, static:%d\r\n",
                tmp_p->mac[0], tmp_p->mac[1], tmp_p->mac[2], tmp_p->mac[3], tmp_p->mac[4], tmp_p->mac[5],
                tmp_p->lrnt_vid, tmp_p->port_id, ca_rbtree_count(rb_p), tmp_p->aging_time, tmp_p->static_flag);
            */
            /* check condition */
            meet[0] = meet[1] = meet[2] = meet[3] = 0;

            if (mask.bf.lpid == 0 && cfg.lpid_high == tmp_p->port_id)
                meet[1] = 1;

            if (mask.bf.key_vid == 0 && cfg.key_vid_high == tmp_p->lrnt_vid)
                meet[2] = 1;

            if (mask.bf.static_flag == 0/* static join */){
                if (cfg.static_flag && tmp_p->static_flag)
                    meet[0] = 1;
                if (cfg.static_flag == 0 && tmp_p->static_flag == 0)
                    meet[0] = 1;
            }
            else{
                meet[0] = 1;
            }
            /*
            L2_DEBUG("type:%d, meet:%d-%d-%d, lpid(mask/cfg): %d/%d, vid:%d/%d, static:%d/%d\r\n", type, meet[0], meet[1], meet[2],
                    mask.bf.lpid, cfg.lpid_high, mask.bf.key_vid, cfg.key_vid_high, mask.bf.static_flag, cfg.static_flag);
            */
            if ((type == SW_LRN_MAC_ALL && meet[0]) ||
                (type == SW_LRN_MAC_PORT && meet[1] && meet[0]) ||
                (type == SW_LRN_MAC_VLAN && meet[2] && meet[0])) {
        ret = ca_rbtree_delete(rb_p, node_p);
                if (sw_lrn_cb.port_mac[tmp_p->port_id].mac_cnt)
                    sw_lrn_cb.port_mac[tmp_p->port_id].mac_cnt--;
                if (sw_lrn_cb.total_mac.mac_cnt)
                    sw_lrn_cb.total_mac.mac_cnt--;
                L2_DEBUG("meet, remove, rb_p:%p, node_p:%p, ret:%d, total_cnt:%d, port:%d, cnt:%d\r\n", 
                        rb_p, node_p, ret, sw_lrn_cb.port_mac[tmp_p->port_id].mac_cnt,
                        tmp_p->port_id, sw_lrn_cb.port_mac[tmp_p->port_id].mac_cnt);
            }
        }

        node_p = next_p;
    }
    //__FDB_SW_LRN_UNLOCK();

    if (mask.bf.static_flag == 0 && cfg.static_flag){
        L2_DEBUG("remove all sw_lrn_cb.fdb_static_p\r\n");
        //__FDB_STATIC_MAC_LOCK();
        ca_table_entry_remove_all(sw_lrn_cb.fdb_static_p);
        //__FDB_STATIC_MAC_UNLOCK();
    }
    /*
    if (mask.bf.static_flag == 0 && cfg.static_flag == 0){
        L2_DEBUG("remove all sw_lrn_cb.fdb_overflow_p\r\n");
        __aal_fdb_overflow_mac_remove_all();
    }
    */
    L2_DEBUG("\r\n");
    return CA_E_OK;
}
ca_status_t __ca_rbtree_clr(ca_rbtree_t *rb_p)
{
    ca_rbtree_node_t *node_p, *next_p;
    ca_status_t ret = CA_E_OK;

    CA_ASSERT_RET(rb_p, CA_E_PARAM);

    node_p = ca_rbtree_first(rb_p);

    while (node_p != rb_p->nil) {

        next_p = ca_rbtree_successor(rb_p, node_p);

        if (node_p == next_p){
            L2_DEBUG("node_p:%p == next_p, break out, need check\r\n", node_p, next_p);
            break;
    }

        ret = ca_rbtree_delete(rb_p, node_p);

        node_p = next_p;
    }

    return ret;
}

ca_status_t __aal_fdb_sw_lrn_cpt_entry_get(
    CA_IN ca_device_id_t       device_id
)
{
    ca_status_t ret = CA_E_ERROR;
    L2FE_L2E_SW_AGING_REPORT_FIFO_STTS_t fifo_stts;
    L2FE_L2E_SW_AGING_CPT_ENTRY_3_t cpt_3;
    L2FE_L2E_SW_AGING_CPT_ENTRY_2_t cpt_2;
    L2FE_L2E_SW_AGING_CPT_ENTRY_1_t cpt_1;
    L2FE_L2E_SW_AGING_CPT_ENTRY_0_t cpt_0;
    int cnt = 0;
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;

    aal_fdb_entry_cfg_t  fdb_entry, *tmp_p = NULL;
    ca_table_t *table_p = sw_lrn_cb.fdb_sw_lrn_p;

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    L2_DEBUG2(2, "Enter, table_p:%p\r\n", table_p);

    while(cnt++ < 8){
        fifo_stts.wrd =  CA_NE_REG_READ(L2FE_L2E_SW_AGING_REPORT_FIFO_STTS);

        if(!fifo_stts.bf.cpt_entry_flg){
            ret = CA_E_NOT_FOUND;
            break;
        }
        else{
            cpt_3.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_3);
            cpt_2.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_2);
            cpt_1.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_1);
            cpt_0.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_0);


            fdb_data0.wrd = cpt_0.wrd;
            fdb_data1.wrd = cpt_1.wrd;
            fdb_data2.wrd = cpt_2.wrd;
            fdb_data3.wrd = cpt_3.wrd;

            __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry.mac);
            __aal_fdb_data_2_entry(fdb_data1, fdb_data0, &fdb_entry);

            //tmp_p = __aal_fdb_sw_lrn_entry_get_by_key(fdb_entry.port_id, &fdb_entry);

            L2_DEBUG("tmp_p:%p, Deleted! mac:0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", tmp_p,
                    fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2],
                    fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5]);

            ret = aal_fdb_entry_del(0, &fdb_entry);


            ret = CA_E_OK;
        }
    }
    return ret;
}

ca_status_t __aal_fdb_sw_lrn_tbl_aging_traverse(void)
{
    ca_table_t *table_p = sw_lrn_cb.fdb_sw_lrn_p;
    aal_fdb_entry_cfg_t *tmp_p;
    ca_rbtree_node_t *node_p, *next_p;
    ca_rbtree_t *rb_p;
    ca_status_t ret;

    if (table_p == NULL || table_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return CA_E_EMPTY;
    }

    rb_p = (ca_rbtree_t *)table_p->content;
    node_p = ca_rbtree_first(rb_p);

    while (node_p != rb_p->nil) {

        next_p = ca_rbtree_successor(rb_p, node_p);

        if (node_p == next_p){
            L2_DEBUG("node_p:%p == next_p, break out, need check\r\n", node_p, next_p);
            break;
        }

        tmp_p = (aal_fdb_entry_cfg_t *)node_p->data;

        if(tmp_p){
            L2_DEBUG("mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d, tree cnt:%d, aging_time:%d\r\n",
                tmp_p->mac[0], tmp_p->mac[1], tmp_p->mac[2], tmp_p->mac[3], tmp_p->mac[4], tmp_p->mac[5],
                tmp_p->lrnt_vid, tmp_p->port_id, ca_rbtree_count(rb_p), tmp_p->aging_time);

            tmp_p->aging_time -= fdb_sw_aging_step;
            if (tmp_p->aging_time <= 0) {
                L2_DEBUG("aged, remove, rb_p:%p, node_p:%p\r\n", rb_p, node_p);

                ret = aal_fdb_entry_del(0, tmp_p);
                
                ret = ca_rbtree_delete(rb_p, node_p);
            }
        }

        node_p = next_p;

    }

    L2_DEBUG("\r\n");
    return CA_E_OK;
}
static void __aal_fdb_sw_lrn_entry(void *cookie)
{
    int cnt = 0;
    ca_int32_t entry_cnt = 0;
    ca_table_t *table_p = sw_lrn_cb.fdb_sw_lrn_p;
    ca_rbtree_t *rb_p;
    int reported;

    if (table_p == NULL || table_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return;
    }

    rb_p = (ca_rbtree_t *)table_p->content;

    while(1){
        ca_sem_wait(sw_lrn_cb.sem, -1);

        if (sw_lrn_cb.need_flush){
            aal_fdb_addr_delete_all(0, CA_L2_ADDR_OPERATION_DYNAMIC);
            sw_lrn_cb.need_flush = 0;
        }
        if (sw_lrn_cb.need_flush_tmp){
            __aal_fdb_addr_flush_tmp(0);
            sw_lrn_cb.need_flush_tmp = 0;
        }

        if ((sw_lrn_cb.oflow_time % FDB_OVERFLOW_AGED) == 3){
            __aal_fdb_overflow_mac_expired();
        }
        sw_lrn_cb.oflow_time++;
        /* do aging scan */
        if (((cnt++) % fdb_sw_aging_step) == 0){
            L2_DEBUG2(2, "%s: sw aging scan. entry_cnt:%d\r\n", __FUNCTION__, entry_cnt);
            __aal_fdb_sw_lrn_cpt_entry_get(0);

            /*
            if ( entry_cnt > 0){
                __aal_fdb_sw_lrn_tbl_aging_traverse();
            }
            */
        }

    }

    ca_printf("%s: sw aging thread exit.\r\n", __FUNCTION__);

}

ca_status_t __aal_fdb_sw_lrn_init(ca_device_id_t device_id)
{
    ca_status_t   ret = CA_E_OK;

    memset(&sw_lrn_cb, 0, sizeof(sw_lrn_cb_t));

    sw_lrn_cb.aging_time = l2_aging_time;
    sw_lrn_cb.total_mac.mac_limit = 0x1fff;

    __aal_fdb_sw_lrn_aging_timer_init();

    ret = ca_sem_init(&sw_lrn_cb.sem, "sw_aging", 0, 0);
    if(0 != ret){
        ca_printf("%s Abort:init sem failed\r\n", __FUNCTION__);
        ret = CA_E_ERROR;
        return ret;
    }

    __aal_fdb_sw_lrn_tbl_create();
    __aal_fdb_static_mac_create();
    __aal_fdb_overflow_mac_create();

    ret = ca_thread_create(&sw_lrn_cb.tid, "sw_aging", __aal_fdb_sw_lrn_entry, NULL, 10240, 15, 0);
    if (ret){
        ca_printf("%s error: create sw_agint thread fail %d.\r\n", __FUNCTION__, ret);
        return CA_E_ERROR;
    }

    return ret;
}

ca_status_t __aal_fdb_sw_lrn_proc(ca_device_id_t device_id, aal_fdb_entry_cfg_t *fdb_entry)
{
    ca_status_t   ret = CA_E_OK;


    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    if (fdb_entry->static_flag){
        L2_DEBUG("static ignore\r\n");
        return CA_E_OK;
    }

    fdb_entry->tmp_flag = 0;
    fdb_entry->aging_sts = 255;
        ret = aal_fdb_entry_add(device_id, fdb_entry);
        return ret;
    }

ca_uint32_t __aal_fdb_sw_lrn_full(aal_fdb_entry_cfg_t  *fdb)
{
    ca_rbtree_t *rb_p = NULL;
    ca_table_t *table_p = NULL;
    ca_uint32_t cnt_d = 0, cnt_s = 0;
    ca_uint32_t entry_cnt = 0;
    aal_fdb_entry_cfg_t *tmp_p = NULL;
    ca_status_t ret = CA_E_OK;

    if (fdb == NULL || !sw_lrn_cb.cfg || sw_lrn_cb.fdb_sw_lrn_p == NULL || sw_lrn_cb.fdb_static_p == NULL){
        return 0;
    }

    //__FDB_SW_LRN_LOCK();
    table_p = sw_lrn_cb.fdb_sw_lrn_p;
    rb_p = (ca_rbtree_t *)table_p->content;
    cnt_d = ca_rbtree_count(rb_p);

    table_p = sw_lrn_cb.fdb_static_p;
    rb_p = (ca_rbtree_t *)table_p->content;
    cnt_s = ca_rbtree_count(rb_p);
    //__FDB_SW_LRN_UNLOCK();

    entry_cnt = cnt_d + cnt_s;

    L2_DEBUG("entry_cnt:%d, limit:%d\r\n", entry_cnt, sw_lrn_cb.total_mac.mac_limit);

    /* if full, static will delete one dynamic */
    if (entry_cnt >= sw_lrn_cb.total_mac.mac_limit && cnt_d > 0 && fdb->port_id == CA_UINT16_INVALID){
        /* remove one dynamic entry */
        tmp_p = __aal_fdb_sw_lrn_entry_getfirst();
        if (tmp_p){
            L2_DEBUG("full, static overwrite\r\n", entry_cnt, sw_lrn_cb.total_mac.mac_limit);
            ret = aal_fdb_entry_del(0, tmp_p);
        if (ret == CA_E_OK)
                entry_cnt--;
        }
    }

    return entry_cnt;
}

#endif 

#ifdef AAL_FDB_ENABLE_PE_MAC_LRN

aal_fdb_entry_cfg_t ipc_static_mac;
aal_fdb_entry_cfg_t *aal_fdb_sw_lrn_static_mac_next(
    CA_IN ca_uint32_t idx
)
{
    fdb_ipc_static_mac_get_t ipc_mac;
    ca_status_t ret = CA_E_OK;

    aal_fdb_entry_cfg_t *tmp_p = &ipc_static_mac;

    ret = aal_fdb_ipc_static_mac_get(idx, tmp_p);

    if (ret != CA_E_OK)
        return NULL;

    return tmp_p;
    }

#endif
void __aal_fdb_flush_type(flush_type_e type)
{
    sw_lrn_cb.flush_cfg.type = type;
    }

ca_status_t __aal_fdb_sw_lrn_tbl_clr(ca_l2_addr_operation_flags_t flag)
{
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    return aal_fdb_ipc_del_all_send(flag);
#else
    ca_table_t *sw_lrn_p = sw_lrn_cb.fdb_sw_lrn_p;
    //ca_table_t *static_p = sw_lrn_cb.fdb_static_p;
    ca_rbtree_t *rb_p;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;

    L2_DEBUG("flag:%d, cfg:%d\r\n", flag, sw_lrn_cb.cfg);
    if (!sw_lrn_cb.cfg)  return ret;

    /*
    if (flag == CA_L2_ADDR_OPERATION_STATIC || flag == CA_L2_ADDR_OPERATION_BOTH){
        if (static_p != NULL || static_p->content != NULL) {
            rb_p = (ca_rbtree_t *)static_p->content;
            __ca_rbtree_clr(rb_p);
            L2_DEBUG("clear static\r\n");
        }
    }
    */
    //if (flag == CA_L2_ADDR_OPERATION_DYNAMIC || flag == CA_L2_ADDR_OPERATION_BOTH)
    {
        __FDB_SW_LRN_LOCK();
        if (sw_lrn_p != NULL || sw_lrn_p->content != NULL) {
            rb_p = (ca_rbtree_t *)sw_lrn_p->content;
            __ca_rbtree_clr(rb_p);
            L2_DEBUG("clear dynamic\r\n");
            sw_lrn_cb.total_mac.mac_cnt = 0;
            for (i = AAL_LPORT_ETH_NI3; i <= AAL_LPORT_ETH_NI6; i++)
                sw_lrn_cb.port_mac[i].mac_cnt = 0;
        }
        __FDB_SW_LRN_UNLOCK();
    }
    return CA_E_OK;
#endif
}

ca_status_t __aal_fdb_sw_mac_limit_set(
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_uint32_t            data,
    CA_IN ca_uint32_t            number
)
{
    L2_DEBUG("data:%d, number:%d\r\n", data, number);

    if(data == CA_UINT16_INVALID){
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        /* send msg to PE */
        aal_fdb_ipc_mac_limit_send(data, number);
        sw_lrn_cb.cfg = number ? 1 : 0;
#else
        L2_DEBUG("global mac limit:%d\r\n", sw_lrn_cb.total_mac.mac_limit);
        if (number){
            ca_timer_stop(sw_lrn_cb.timer);

            if (number < sw_lrn_cb.total_mac.mac_limit){
                sw_lrn_cb.total_mac.mac_limit = 0;
                sw_lrn_cb.need_flush = 1;
            }
            else{
                sw_lrn_cb.need_flush_tmp = 1;
            }
            sw_lrn_cb.total_mac.mac_limit = number;
            sw_lrn_cb.cfg = 1;
            /* start aging timer */
            ca_timer_restart(sw_lrn_cb.timer, FDB_SW_AGING_TIMER_INTERVAL);

        }
        else{
            sw_lrn_cb.total_mac.mac_limit = 0x1fff;
            __aal_fdb_sw_lrn_tbl_clr(CA_L2_ADDR_OPERATION_DYNAMIC);

            /* stop aging timer */
            ca_timer_stop(sw_lrn_cb.timer);
            sw_lrn_cb.cfg = 0;

        }
#endif
    }
    else{
        CA_ASSERT_RET(data < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        aal_fdb_ipc_mac_limit_send(data, number);
#else
        sw_lrn_cb.port_mac[data].mac_limit = number;
        //sw_lrn_cb.cfg = 1;
        L2_DEBUG("port:%d mac limit:%d\r\n", data, sw_lrn_cb.port_mac[data].mac_limit);
#endif
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_sw_lrn_cfg_get(
    CA_IN ca_device_id_t          device_id
)
{
    if (sw_lrn_cb.cfg)
        return CA_E_OK;

    return CA_E_CONFIG;
}

ca_status_t aal_fdb_sw_lrn_traverse_get (
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t             flag
)
{
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    aal_fdb_ipcdebug_set(100, flag);
    return CA_E_OK;
#else
    aal_fdb_entry_cfg_t *entry_p = NULL;
    ca_int32_t  cnt = 1;
    ca_int32_t  i = 0;
    ca_uint32_t entry_cnt;
    ca_table_t *table_p = sw_lrn_cb.fdb_sw_lrn_p;
    ca_rbtree_t *rb_p;

    if (table_p == NULL || table_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return CA_E_EMPTY;
    }

    __FDB_SW_LRN_LOCK();
    rb_p = (ca_rbtree_t *)table_p->content;
    entry_cnt = ca_rbtree_count(rb_p);

    L2_DEBUG("CB\r\n");

    ca_printf("\r\ncfg:%d\r\n", sw_lrn_cb.cfg);
    ca_printf("aging:%d\r\n", sw_lrn_cb.aging_time);
    ca_printf("aging step:%d\r\n", fdb_sw_aging_step);

    ca_printf("mac_limit :%d, cnt:%d\r\n", sw_lrn_cb.total_mac.mac_limit, entry_cnt);

    for (i = AAL_LPORT_ETH_NI3; i <= AAL_LPORT_ETH_NI6; i++){
        ca_printf("port :%d, mac_limit:%d, cnt:%d\r\n", i, sw_lrn_cb.port_mac[i].mac_limit, sw_lrn_cb.port_mac[i].mac_cnt);
    }

    L2_DEBUG("RB Tree: sw lrn table show\r\n");

    entry_p = (aal_fdb_entry_cfg_t *)ca_table_entry_getfirst(sw_lrn_cb.fdb_sw_lrn_p);

    if(entry_p){
        ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d(%c), port:%d, aging_sts:%d\r\n",
            cnt,
            entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
            entry_p->lrnt_vid, entry_p->lrnt_sc_ind ? 's' : 'c', entry_p->port_id, entry_p->aging_sts);
    }

    while (entry_p) {
        cnt++;
        entry_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getnext(sw_lrn_cb.fdb_sw_lrn_p);
        if(entry_p){
            ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d(%c), port:%d, aging_sts:%d\r\n",
                cnt,
                entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                entry_p->lrnt_vid, entry_p->lrnt_sc_ind ? 's' : 'c', entry_p->port_id, entry_p->aging_sts);
        }
    }
    __FDB_SW_LRN_UNLOCK();

    /* static tree */
    cnt = 1;
    L2_DEBUG("RB Tree: static mac table show\r\n");

    __FDB_STATIC_MAC_LOCK();
    table_p = sw_lrn_cb.fdb_static_p;
    rb_p = (ca_rbtree_t *)table_p->content;
    entry_cnt = ca_rbtree_count(rb_p);
    entry_p = (aal_fdb_entry_cfg_t *)ca_table_entry_getfirst(sw_lrn_cb.fdb_static_p);

    if(entry_p){
        ca_printf("\r\n--Static TREE--, cnt:%d\r\nindex:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d(%c), port:%d\r\n",
            entry_cnt, cnt,
            entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
            entry_p->lrnt_vid, entry_p->lrnt_sc_ind ? 's' : 'c', entry_p->port_id);
    }

    while (entry_p) {
        cnt++;
        entry_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getnext(sw_lrn_cb.fdb_static_p);
        if(entry_p){
            ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d(%c), port:%d\r\n",
                cnt,
                entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                entry_p->lrnt_vid, entry_p->lrnt_sc_ind ? 's' : 'c', entry_p->port_id);            }
    }
    __FDB_STATIC_MAC_UNLOCK();
    if (flag >= 2){
        entry_cnt = aal_fdb_table_get(0, 0x101);
        ca_printf("tmp entry:%d\r\n", entry_cnt);
    }
    return CA_E_OK;
#endif
}
ca_int32 __aal_mac_filter_tbl_cmp(void *key_x, void *key_y)
{
    aal_tbl_mac_filter_key_t *x = (aal_tbl_mac_filter_key_t *)key_x;
    aal_tbl_mac_filter_key_t *y = (aal_tbl_mac_filter_key_t *)key_y;

    CA_ASSERT_RET(key_x != NULL && key_y != NULL, 0);


    L2_DEBUG("x: mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d, mac_flag:%d\r\n",
        x->mac[0], x->mac[1], x->mac[2], x->mac[3], x->mac[4], x->mac[5],
        x->vid, x->port_id, x->mac_flag);

    L2_DEBUG("y: mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d, mac_flag:%d\r\n",
        y->mac[0], y->mac[1], y->mac[2], y->mac[3], y->mac[4], y->mac[5],
        y->vid, y->port_id, y->mac_flag);


    if (x->port_id < y->port_id)
        return -1;

    if (x->port_id > y->port_id)
        return 1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) < 0)
        return -1;

    if (memcmp(x->mac, y->mac, sizeof(ca_mac_addr_t)) > 0)
        return 1;

    if (x->vid < y->vid)
        return -1;

    if (x->vid > y->vid)
        return 1;

    if (x->mac_flag < y->mac_flag)
        return -1;

    if (x->mac_flag > y->mac_flag)
        return 1;


    return 0;
}

ca_status_t __aal_mac_filter_tbl_create(void)
{
    mac_filter_tbl_p = ca_table_create(CA_AAL_TBL_MAC_FILTER_NAME,
                                       CA_RBTREE_TABLE,
                                       sizeof(aal_tbl_mac_filter_entry_t),
                                       __aal_mac_filter_tbl_cmp);

    L2_DEBUG("mac_filter_tbl_p:%p\r\n", mac_filter_tbl_p);
    return CA_E_OK;
}

ca_status_t __aal_mac_filter_tbl_destory(void)
{
    ca_int32 ret;

    ret = ca_table_destory(mac_filter_tbl_p);

    L2_DEBUG("ret:%d\r\n", ret);

    if (ret == 0) {
        mac_filter_tbl_p = NULL;
        return CA_E_OK;
    }
    else
        return CA_E_ERROR;
}



/* take rule id back in entry if new rule */
ca_status_t __aal_mac_filter_tbl_add_del(CA_IN ca_uint32 op_cmd,
        CA_IN_OUT aal_tbl_mac_filter_entry_t *entry)
{
    aal_tbl_mac_filter_key_t x;
    //aal_tbl_mac_filter_entry_t *filter_p = NULL;
    ca_int32 ret;
    //ca_uint32 rule_id = 0;

    CA_ASSERT_RET(entry != NULL, CA_E_PARAM);
    CA_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_APPEND ||
                  op_cmd == __FDB_TBL_OPCODE_DELETE, CA_E_PARAM);
    //CA_ASSERT_RET(entry->mask != CA_L2_MAC_FILTER_MASK_MAC_ONLY && entry->mask != CA_L2_MAC_FILTER_MASK_MAC_VLAN, CA_E_PARAM);

    /*
    rule_cnt = ca_table_entry_count(mac_filter_tbl_p);
    if (rule_cnt >= CA_AAL_FDB_MAC_FILTER_RULE)
        return CA_E_UNAVAIL;
    */
    memset((void *)&x, 0, sizeof(aal_tbl_mac_filter_key_t));

    memcpy(x.mac, entry->mac, CA_ETH_ADDR_LEN);
    x.mac_flag = entry->mac_flag;
    x.port_id = entry->port_id;

    if (CA_L2_MAC_FILTER_MASK_MAC_VLAN == entry->mask)
        x.vid = entry->vid;

    if (op_cmd == __FDB_TBL_OPCODE_APPEND) {
#if 0
    /* actually, rule id is allocate in caller */
        /* 1. search */
        filter_p = (aal_tbl_mac_filter_entry_t *)ca_table_entry_search(
                       mac_filter_tbl_p, &x);

        /* 2. allocate rule id if no found */
        if (filter_p == NULL) {
            L2_DEBUG("new rule\r\n");
            rule_id = __aal_mac_filter_alloc_rule_id(entry->port_id);

            if (rule_id == 0) {
                /* full */
                L2_DEBUG("unavailable  port:%d, rule id:%d\r\n", entry->port_id, rule_id);
                return CA_E_UNAVAIL;
            }

            entry->rule_id = rule_id;
        }
#endif
        ret = ca_table_entry_insert(mac_filter_tbl_p, (void *)&x, (void *)entry);
    }
    else
        ret = ca_table_entry_remove(mac_filter_tbl_p, (void *)&x);

    L2_DEBUG("op: %s, ret:%d\r\n",
             op_cmd == __FDB_TBL_OPCODE_APPEND ? "add" : "del", ret);

    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}


ca_status_t __aal_mac_filter_tbl_flush_port(ca_int32 port_id)
{
    aal_tbl_mac_filter_entry_t *tmp_p;
    ca_rbtree_node_t *node_p, *next_p;
    ca_rbtree_t *mac_filter_rb_p;
    ca_status_t ret;

    if (mac_filter_tbl_p == NULL || mac_filter_tbl_p->content == NULL) {
        L2_DEBUG("fatal, empty\r\n");
        return CA_E_EMPTY;
    }

    L2_DEBUG("port_id:%d/0x%x\r\n", port_id, port_id);

    mac_filter_rb_p = (ca_rbtree_t *)mac_filter_tbl_p->content;
    node_p = ca_rbtree_first(mac_filter_rb_p);

    while (node_p != mac_filter_rb_p->nil) {

        next_p = ca_rbtree_successor(mac_filter_rb_p, node_p);

        if (node_p == next_p){
            L2_DEBUG("node_p:%p == next_p, break out, need check\r\n", node_p, next_p);
            break;
        }

        tmp_p = (aal_tbl_mac_filter_entry_t *)node_p->data;

        if(tmp_p){
            L2_DEBUG("mac:0x%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d, tree cnt:%d\r\n",
                tmp_p->mac[0], tmp_p->mac[1], tmp_p->mac[2], tmp_p->mac[3], tmp_p->mac[4], tmp_p->mac[5],
                tmp_p->vid, tmp_p->port_id, ca_rbtree_count(mac_filter_rb_p));
        }
        if (tmp_p && tmp_p->port_id == port_id) {
            L2_DEBUG("find, rule_id:%d, mac_filter_rb_p:%p, node_p:%p\r\n",
                    tmp_p->rule_id, mac_filter_rb_p, node_p);
            ret = ca_rbtree_delete(mac_filter_rb_p, node_p);
        }

        node_p = next_p;

    }

    L2_DEBUG("\r\n");
    return CA_E_OK;
}

aal_tbl_mac_filter_entry_t *__aal_mac_filter_rule_getfirst_port(
    CA_IN ca_int32 port_id
)
{
    aal_tbl_mac_filter_entry_t *tmp_p = NULL;

    tmp_p = (aal_tbl_mac_filter_entry_t *)ca_table_entry_getfirst(
                mac_filter_tbl_p);
    L2_DEBUG("tmp_p:%p\r\n", tmp_p);

    while (tmp_p) {

        if (tmp_p->port_id == port_id) {
            L2_DEBUG("%s %d find\n", __FUNCTION__,__LINE__);
            break;
        }

        tmp_p = (aal_tbl_mac_filter_entry_t *)ca_table_entry_getnext(
                    mac_filter_tbl_p);

        if (tmp_p == NULL) {
            L2_DEBUG("search end\r\n");
            break;
        }
    }

    return tmp_p;
}

aal_tbl_mac_filter_entry_t *__aal_mac_filter_rule_get_by_key(
    CA_IN ca_int32 port_id,
    CA_IN aal_tbl_mac_filter_entry_t *entry
)
{
    aal_tbl_mac_filter_entry_t *tmp_p = NULL;
    aal_tbl_mac_filter_key_t key;

    CA_ASSERT_RET(entry != NULL, NULL);

    memset(&key, 0, sizeof(aal_tbl_mac_filter_key_t));

    memcpy(key.mac, entry->mac, CA_ETH_ADDR_LEN);
    key.mac_flag = entry->mac_flag;
    key.port_id = port_id;
    if (entry->mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN)
        key.vid = entry->vid;
    tmp_p = (aal_tbl_mac_filter_entry_t *)ca_table_entry_search(mac_filter_tbl_p,
            (void *)&key);

    return tmp_p;
}

ca_status_t __aal_mac_filter_load_cls_rule(
    ca_int32_t port_id,
    aal_l2_cls_l2rl2r_key_mask_t *cls_mask,
    ca_uint32_t   rule,
    aal_l2_cls_l2r_key_t *l2r,
    ca_mac_addr_t mac,
    ca_uint32_t   mac_flag,   /* 0 sa, 1 da */
    ca_uint32_t   mask,
    ca_uint32_t   vid)
{

    ca_boolean_t is_svlan = 0;

    CA_ASSERT_RET(cls_mask != NULL && l2r != NULL, CA_E_PARAM);

    __aal_fdb_sc_ind_get(0, port_id, &is_svlan);
    /* mac */
    //l2r->l2_key_mask.s.mac_field_ctrl = 1;
    memcpy(l2r->mac_da, mac, CA_ETH_ADDR_LEN);
    memcpy(l2r->mac_sa, mac, CA_ETH_ADDR_LEN);

    if (mac_flag == 0)
        l2r->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_SA_RANGE;
    else
        l2r->mac_field_ctrl = AAL_L2_CLS_MAC_FIELD_CTRL_DA_RANGE;

    /* vlan */
    if (mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN) {
        //l2r->l2_key_mask.s.vid_field_ctrl = 1;
        l2r->top_vlan_id = vid;
        l2r->inner_vlan_id = vid;
        l2r->top_is_svlan = is_svlan;
        l2r->vlan_field_ctrl = AAL_L2_CLS_VLAN_FIELD_CTRL_TOP_RANGE;
    }else{
        l2r->vlan_field_ctrl = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    }
    l2r->dot1p_ctrl         = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    l2r->l4_port_field_ctrl = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    l2r->vlan_number_vld    = FALSE;
    l2r->ethernet_type_vld  = FALSE;

    /* set mask */
    if (rule == 0){
        cls_mask->s.mac_sa_0 = 1;
        cls_mask->s.mac_da_0 = 1;

        cls_mask->s.mac_field_ctrl_0 = 1;

        cls_mask->s.top_vlan_id_0 = 1;
        cls_mask->s.inner_vlan_id_0 = 1;
        cls_mask->s.top_is_svlan_0 = 1;
        cls_mask->s.vlan_field_ctrl_0 = 1;
        cls_mask->s.dot1p_ctrl_0 = 1;
        cls_mask->s.l4_port_field_ctrl_0 = 1;
        cls_mask->s.vlan_number_vld_0 = 1;
        cls_mask->s.ethernet_type_vld_0 = 1;

    }
    else{
        cls_mask->s.mac_sa_1 = 1;
        cls_mask->s.mac_da_1 = 1;

        cls_mask->s.mac_field_ctrl_1 = 1;

        cls_mask->s.top_vlan_id_1 = 1;
        cls_mask->s.inner_vlan_id_1 = 1;
        cls_mask->s.top_is_svlan_1 = 1;
        cls_mask->s.vlan_field_ctrl_1 = 1;

        cls_mask->s.dot1p_ctrl_1 = 1;
        cls_mask->s.l4_port_field_ctrl_1 = 1;
        cls_mask->s.vlan_number_vld_1 = 1;
        cls_mask->s.ethernet_type_vld_1 = 1;
    }

    return CA_E_OK;
}
/*
 * ----------------------------------------------------------------- *
 */
extern mirror_config_t mirror_cfg;

ca_status_t aal_fdb_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    
    ca_printf("\r\nvalue1 1: Set value2 to aal_fdb_debug\r\n ");

    switch (value1){
        case 1:
            aal_fdb_debug = value2;
            ca_printf("fdb debug %d\r\n", aal_fdb_debug);
            break;
        case 2:
            {
                ca_iterator_t entry;
                ca_l2_addr_entry_t *l2_addr, *cfg;
                ca_status_t ret = CA_E_OK;
                int cnt = 0, j;
                int sz = 0;

                printk("ca_l2_addr_iterate\r\n");
                memset(&entry, 0, sizeof(ca_iterator_t));
                entry.entry_count = value2;
                sz = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

                l2_addr = ca_malloc(sz);

                memset(l2_addr, 0, sz);
                entry.entry_data = l2_addr;
                entry.user_buffer_size = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

                /*while (1)*/{
                    ret = ca_l2_addr_iterate(0, &entry);

                    if (ret == CA_E_UNAVAIL){
                        printk("end search\r\n");
                  //      break;
                    }

                }

                printk("entry_count = %#x \n",entry.entry_count);
                printk("entry_data = %p\n",entry.entry_data);
                printk("user_buffer_size = %#x \n",entry.user_buffer_size);
                printk("prev_handle = %p\n",entry.prev_handle);

                for (cnt = 0; cnt < entry.entry_count; cnt++){
                    cfg = l2_addr + cnt;
                    printk(" mac:");
                    for(j = 0 ; j < 6; j++){
                        printk("%x:", cfg->mac_addr[j]);
                    }
                    printk(" key_vid:%d, lrn_vid:%d, dot1p:%d, static:%d, sa:%d, da:%d, port_id:%#x, mc:%d, age:%d\r\n",
                        cfg->vid, cfg->fwd_vid, cfg->dot1p, cfg->static_flag,
                        cfg->sa_permit, cfg->da_permit, cfg->port_id, cfg->mc_group_id, cfg->aging_timer);
                }

                ca_free(l2_addr);
            }
            break;
        case 3:
            {
                ca_iterator_t entry;
                ca_l2_addr_entry_t *l2_addr, *cfg;
                ca_status_t ret = CA_E_OK;
                int cnt = 0, j;
                int sz = 0;

                printk("ca_l2_addr_iterate\r\n");
                memset(&entry, 0, sizeof(ca_iterator_t));
                entry.entry_count = 1;
                sz = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

                l2_addr = ca_malloc(sz);

                memset(l2_addr, 0, sz);
                entry.entry_data = l2_addr;
                entry.user_buffer_size = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

                for(cnt = 0; cnt < value2; cnt++){
                    ret = ca_l2_addr_iterate(0, &entry);

                    if (ret == CA_E_UNAVAIL){
                        printk("end search\r\n");
                        break;
                    }


                    printk("entry_count = %#x \n",entry.entry_count);
                    printk("entry_data = %p\n",entry.entry_data);
                    printk("user_buffer_size = %#x \n",entry.user_buffer_size);
                    printk("prev_handle = %p\n",entry.prev_handle);

                    cfg = l2_addr;
                    printk(" mac:");
                    for(j = 0 ; j < 6; j++){
                        printk("%x:", cfg->mac_addr[j]);
                    }
                    printk(" key_vid:%d, lrn_vid:%d, dot1p:%d, static:%d, sa:%d, da:%d, port_id:%#x, mc:%d, age:%d\r\n",
                        cfg->vid, cfg->fwd_vid, cfg->dot1p, cfg->static_flag,
                        cfg->sa_permit, cfg->da_permit, cfg->port_id, cfg->mc_group_id, cfg->aging_timer);

                }

                ca_free(l2_addr);
            }
            break;

        case 35:
            {
                fdb_sw_aging_step = value2;
            }
            break;
        case 36:
            {
                extern ca_uint32_t l2_customer_info;

                ca_printf("l2_customer_info = 0x%x\r\n", l2_customer_info);
            }
            break;
        case 38:
            {
                using_sw_lrn = value2;
                ca_printf("using_sw_lrn = 0x%x\r\n", using_sw_lrn);
            }
            break;

        default:
            break;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{

    if (value2) *value2 = aal_fdb_debug;

    ca_printf("fdb debug %d\r\n", aal_fdb_debug);

    return CA_E_OK;
}


ca_status_t aal_fdb_tbl_del_all()
{
    ca_int32 ret;

    __FDB_LOCK();

    ret = ca_table_entry_remove_all(fdb_tbl_p);
    ret += ca_table_entry_remove_all(fdb_tbl_svl_p);
    /*
    ret = __aal_fdb_tbl_destory();

    if (ret == 0)
        ret = __aal_fdb_tbl_create();
    */
    __FDB_UNLOCK();

    if (ret == 0)
        return CA_E_OK;
    else
        return CA_E_ERROR;
}

ca_status_t aal_fdb_entry_get(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_mac_addr_t          mac,
    CA_IN ca_uint16_t            key_vid,
    CA_OUT aal_fdb_entry_cfg_t  *fdb_entry
)
{
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;

    L2FE_L2E_FDB_CMD_RETURN_t   cmd_return;
    ca_status_t         ret;

    CA_ASSERT_RET(fdb_entry != NULL, CA_E_PARAM);

    if (key_vid > 0xFFF) {
        L2_DEBUG("invalid vid:%d\r\n", key_vid);
        return CA_E_PARAM;
    }

    memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));
    memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
    memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
    memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
    memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

    __aal_mac_2_fdb_data(&fdb_data3, &fdb_data2, &fdb_data1, mac);

    fdb_data1.bf.key_vid = key_vid;
    fdb_data1.bf.key_scind = (fdb_entry->key_sc_ind ? 1 : 0);

    /* TODO 2 bit for dot1p ?? */
    //fdb_data1.bf.key_dot1p = sig->dot1p;

    //__FDB_LOCK();

    ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_READ,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&fdb_data3,
                           (ca_uint32 *)&fdb_data2,
                           (ca_uint32 *)&fdb_data1,
                           (ca_uint32 *)&fdb_data0,
                           (ca_uint32 *)&cmd_return);

    if (ret != CA_E_OK) {
        //__FDB_UNLOCK();
        L2_DEBUG("access fdb fail\r\n");
        return ret;
    }

    if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT) {
        //__FDB_UNLOCK();
        L2_DEBUG("fdb no find entry with sc:%d\r\n", fdb_entry->key_sc_ind);

        memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));
        memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
        memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
        memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
        memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

        /* it can not use __aal_fdb_sc_ind_get because there is not port id
         *
         * Use !key_sc_ind to get again
         */
        __aal_mac_2_fdb_data(&fdb_data3, &fdb_data2, &fdb_data1, mac);

        fdb_data1.bf.key_vid = key_vid;
        fdb_data1.bf.key_scind    = !fdb_entry->key_sc_ind;

        ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_READ,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               (ca_uint32 *)&fdb_data3,
                               (ca_uint32 *)&fdb_data2,
                               (ca_uint32 *)&fdb_data1,
                               (ca_uint32 *)&fdb_data0,
                               (ca_uint32 *)&cmd_return);

        if (ret != CA_E_OK) {
            L2_DEBUG("access fdb fail\r\n");
            return ret;
        }

        if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT){
            L2_DEBUG("fdb STILL no find entry with !key_sc:%d\r\n", !fdb_entry->key_sc_ind);
            return CA_E_NOT_FOUND;
        }
    }

    __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry->mac);
    __aal_fdb_data_2_entry(fdb_data1, fdb_data0, fdb_entry);

    //__FDB_UNLOCK();

    return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t aal_fdb_entry_offset_get(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_mac_addr_t          mac,
    CA_IN ca_uint16_t            key_vid,
    CA_OUT ca_uint32_t           *offset
)
{
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;

    L2FE_L2E_FDB_CMD_RETURN_t   cmd_return;
    __l2fe_l2e_fdb_scan_return_t be_ret;
    ca_status_t         ret;

    CA_ASSERT_RET(offset != NULL, CA_E_PARAM);

    if (key_vid > 0xFFF) {
        L2_DEBUG("invalid vid:%d\r\n", key_vid);
        return CA_E_PARAM;
    }

    memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));
    memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
    memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
    memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
    memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

    __aal_mac_2_fdb_data(&fdb_data3, &fdb_data2, &fdb_data1, mac);

    fdb_data1.bf.key_vid = key_vid;
    fdb_data1.bf.key_scind = 0; //(fdb_entry->key_sc_ind ? 1 : 0);

    /* TODO 2 bit for dot1p ?? */
    //fdb_data1.bf.key_dot1p = sig->dot1p;

    //__FDB_LOCK();

    ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_READ,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&fdb_data3,
                           (ca_uint32 *)&fdb_data2,
                           (ca_uint32 *)&fdb_data1,
                           (ca_uint32 *)&fdb_data0,
                           (ca_uint32 *)&cmd_return);

    if (ret != CA_E_OK) {
        //__FDB_UNLOCK();
        L2_DEBUG("access fdb fail\r\n");
        return ret;
    }

    if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT) {
        //__FDB_UNLOCK();
        L2_DEBUG("fdb no find entry\r\n");
        return CA_E_NOT_FOUND;
    }

    *offset = cmd_return.bf.ext_status;

    //__FDB_UNLOCK();

    return CA_E_OK;
}
#endif


ca_status_t aal_fdb_entry_add(
    CA_IN ca_device_id_t           device_id,
    CA_IN aal_fdb_entry_cfg_t  *fdb
)
{
    ca_status_t ret;
    ca_table_t *table_p;
    ca_rbtree_t *rb_p;
    ca_uint32_t entry_cnt;

    CA_ASSERT_RET(fdb != NULL, CA_E_PARAM);

    entry_cnt = 0;
    rb_p = NULL;
    table_p = sw_lrn_cb.fdb_sw_lrn_p;

    //__FDB_LOCK();
    __FDB_SW_LRN_LOCK();

#ifndef AAL_FDB_ENABLE_PE_MAC_LRN
    if (sw_lrn_cb.cfg && (fdb->port_id <= AAL_LPORT_ETH_NI6 || fdb->port_id == CA_UINT16_INVALID)
            && !fdb->mc_flag && !fdb->sw_update){
        entry_cnt = __aal_fdb_sw_lrn_full(fdb);
        if (entry_cnt >= sw_lrn_cb.total_mac.mac_limit){
            L2_DEBUG("mac limit exceed, entry_cnt:%d\r\n", entry_cnt);
            __FDB_SW_LRN_UNLOCK();
            //__aal_fdb_overflow_mac_add_del(__FDB_TBL_OPCODE_APPEND, fdb);
            return CA_E_FULL;
        }
        if (fdb->port_id <= AAL_LPORT_ETH_NI6){
            if (sw_lrn_cb.port_mac[fdb->port_id].mac_cnt >= sw_lrn_cb.port_mac[fdb->port_id].mac_limit){
                L2_DEBUG("mac limit port exceed, mac_cnt:%d\r\n", sw_lrn_cb.port_mac[fdb->port_id].mac_cnt);
                __FDB_SW_LRN_UNLOCK();
                return CA_E_FULL;
            }
        }
    }
#endif

    if (fdb->port_id == CA_UINT16_INVALID){
        /* from oam, no specify port id */
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        ret = aal_fdb_ipc_add_del_send(TRUE, fdb);
#else
        if (using_sw_lrn){
        __aal_fdb_static_mac_add_del(__FDB_TBL_OPCODE_APPEND, fdb);
            __FDB_SW_LRN_UNLOCK();
        return CA_E_OK;
    }
#endif
    }

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    if (fdb->port_id == CA_UINT16_INVALID)
        fdb->port_id = AAL_LPORT_ETH_NI3;
#endif

    if (!using_sw_lrn)
    ret = __aal_fdb_entry_add_del(device_id, __FDB_TBL_OPCODE_APPEND, fdb);
    else{
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        ret = aal_fdb_ipc_add_del_send(TRUE, fdb);
#endif
    }

    if (ret == CA_E_OK){
        if (sw_lrn_cb.cfg && fdb->port_id <= AAL_LPORT_ETH_NI6 && !fdb->mc_flag && !fdb->static_flag){
#ifndef AAL_FDB_ENABLE_PE_MAC_LRN
            __aal_fdb_sw_lrn_tbl_add_del(__FDB_TBL_OPCODE_APPEND, fdb);
#endif
        }
        ret = __aal_fdb_tbl_add_del(__FDB_TBL_OPCODE_APPEND, fdb);
    }
    __FDB_SW_LRN_UNLOCK();
    //__FDB_UNLOCK();

    L2_DEBUG("ret:%d\r\n", ret);
    return ret;
}

/*
 from l2fe mas, Table 81
 the addr[5] represents update with LRU algorithm overwrite while with hash collsion detected in hash UPD append mode
 */
ca_status_t aal_fdb_entry_add_replace(
    CA_IN ca_device_id_t           device_id,
    CA_IN aal_fdb_entry_cfg_t  *fdb
)
{
    ca_status_t ret;

    AAL_ASSERT_RET(fdb != NULL, CA_E_PARAM);
    //__FDB_LOCK();

    ret = __aal_fdb_entry_add_del(device_id, __FDB_TBL_OPCODE_APPEND_REPLACE, fdb);

    if (ret == CA_E_OK)
        ret = __aal_fdb_tbl_add_del(__FDB_TBL_OPCODE_APPEND_REPLACE, fdb);

    //__FDB_UNLOCK();

    L2_DEBUG("ret:%d\r\n", ret);
    return ret;
}


ca_status_t aal_fdb_entry_del(
    CA_IN ca_device_id_t           device_id,
    CA_IN aal_fdb_entry_cfg_t  *fdb
)
{
    ca_status_t ret;

    //__FDB_LOCK();

    if (sw_lrn_debug)
        ca_printf("<%s, %d>port: 0x%x, tmp:%d, Del fdb. mac:0x%02x:%02x:%02x:%02x:%02x:%02x.\r\n", __FUNCTION__, __LINE__,
            fdb->port_id, fdb->tmp_flag,
            fdb->mac[0], fdb->mac[1], fdb->mac[2],
            fdb->mac[3], fdb->mac[4], fdb->mac[5]);
    __FDB_SW_LRN_LOCK();

    if (!using_sw_lrn)
    ret = __aal_fdb_entry_add_del(device_id, __FDB_TBL_OPCODE_DELETE, fdb);
    else{
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        ret = aal_fdb_ipc_add_del_send(FALSE, fdb);
#endif
    }

    if (ret == CA_E_OK){
        if (sw_lrn_cb.cfg){
#ifndef AAL_FDB_ENABLE_PE_MAC_LRN
            __aal_fdb_sw_lrn_tbl_add_del(__FDB_TBL_OPCODE_DELETE, fdb);
#endif
        }
        ret = __aal_fdb_tbl_add_del(__FDB_TBL_OPCODE_DELETE, fdb);
    }
    __FDB_SW_LRN_UNLOCK();
    //__FDB_UNLOCK();

    L2_DEBUG("ret:%d\r\n", ret);

    return ret;
}

ca_status_t aal_fdb_addr_aged_entry_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_fdb_entry_cfg_t  *fdb_entry
)
{
    ca_status_t ret = CA_E_ERROR;
    L2FE_L2E_SW_AGING_REPORT_FIFO_STTS_t fifo_stts;
    L2FE_L2E_SW_AGING_CPT_ENTRY_3_t cpt_3;
    L2FE_L2E_SW_AGING_CPT_ENTRY_2_t cpt_2;
    L2FE_L2E_SW_AGING_CPT_ENTRY_1_t cpt_1;
    L2FE_L2E_SW_AGING_CPT_ENTRY_0_t cpt_0;
    int cnt = 0;
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;

    L2FE_L2E_FDB_CMD_RETURN_t   cmd_return;

    CA_ASSERT_RET(fdb_entry != NULL, CA_E_PARAM);

    /*
    memset((void *)&fifo_stts, 0, sizeof(L2FE_L2E_SW_AGING_REPORT_FIFO_STTS_t));
    memset((void *)&cpt_3, 0, sizeof(L2FE_L2E_SW_AGING_CPT_ENTRY_3_t));
    memset((void *)&cpt_2, 0, sizeof(L2FE_L2E_SW_AGING_CPT_ENTRY_2_t));
    memset((void *)&cpt_1, 0, sizeof(L2FE_L2E_SW_AGING_CPT_ENTRY_1_t));
    memset((void *)&cpt_0, 0, sizeof(L2FE_L2E_SW_AGING_CPT_ENTRY_0_t));
    */

    while(cnt++ < 8){
        fifo_stts.wrd =  CA_NE_REG_READ(L2FE_L2E_SW_AGING_REPORT_FIFO_STTS);

        if(!fifo_stts.bf.cpt_entry_flg){
            ret = CA_E_NOT_FOUND;
            break;
        }
        else{
            cpt_3.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_3);
            cpt_2.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_2);
            cpt_1.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_1);
            cpt_0.wrd = CA_NE_REG_READ(L2FE_L2E_SW_AGING_CPT_ENTRY_0);

            memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));

            fdb_data0.wrd = cpt_0.wrd;
            fdb_data1.wrd = cpt_1.wrd;
            fdb_data2.wrd = cpt_2.wrd;
            fdb_data3.wrd = cpt_3.wrd;

            ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_READ,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   (ca_uint32 *)&fdb_data3,
                                   (ca_uint32 *)&fdb_data2,
                                   (ca_uint32 *)&fdb_data1,
                                   (ca_uint32 *)&fdb_data0,
                                   (ca_uint32 *)&cmd_return);

            if (ret != CA_E_OK) {
                L2_DEBUG("access fdb fail\r\n");
                return ret;
            }

            if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT) {
                L2_DEBUG("fdb no find entry with, probably the entry is deleted by sw\r\n");
                ret = CA_E_NOT_FOUND;
                continue;
            }

            __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry->mac);
            __aal_fdb_data_2_entry(fdb_data1, fdb_data0, fdb_entry);
            L2_DEBUG("found!\r\n");
            ret = CA_E_OK;
        }
    }
    return ret;
}


ca_status_t aal_fdb_cmp_cfg_load(aal_fdb_flush_cfg_t *cfg)
{
    CA_ASSERT_RET(cfg != NULL , CA_E_PARAM);

    L2_DEBUG("\r\n");

    cfg->key_sc_ind = 0;
    cfg->mc_flag = 0;
    cfg->static_flag = 0;
    cfg->sa_permit = 1;
    cfg->da_permit = 1;
    cfg->in_refresh_flag = 1;
    cfg->in_st_mv_flag = 1;
    cfg->tmp_flag = 1;

    cfg->key_vid_high = 0xfff;
    cfg->key_vid_low = 0;

    cfg->lrned_vid_high = 0xfff;
    cfg->lrned_vid_low = 0;

    cfg->dot1p_high = 0x7;
    cfg->dot1p_low = 0;

    cfg->aging_status_high = 0xff;
    cfg->aging_status_low = 0;

    cfg->mcgid_high = 0x3ff;
    cfg->mcgid_low = 0;

    cfg->lpid_high = 0x7f;
    cfg->lpid_low = 0;


    return CA_E_OK;
}

ca_status_t aal_fdb_scan_flush_field_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_fdb_cpu_cmd_word4_msk_t mask,
    CA_IN aal_fdb_flush_cfg_t *cfg
)
{
    aal_fdb_cpu_cmd_word4_msk_t *word4_p = &mask;
    aal_fdb_cpu_cmd_word3_t word3;
    aal_fdb_cpu_cmd_word2_t word2;
    aal_fdb_cpu_cmd_word1_t word1;
    aal_fdb_cpu_cmd_word0_t word0;

    memset((void *)&word3, 0, sizeof(aal_fdb_cpu_cmd_word3_t));
    memset((void *)&word2, 0, sizeof(aal_fdb_cpu_cmd_word2_t));
    memset((void *)&word1, 0, sizeof(aal_fdb_cpu_cmd_word1_t));
    memset((void *)&word0, 0, sizeof(aal_fdb_cpu_cmd_word0_t));

    /* here, we only care valid entry */
    word4_p->bf.invalid_entry = 1;

    if (!word4_p->bf.key_sc_ind) {
        word3.bf.key_sc_ind = cfg->key_sc_ind;
    }

    if (!word4_p->bf.mc_flag) {
        word3.bf.mc_flag = cfg->mc_flag;
    }


    if (!word4_p->bf.sa_permit) {
        word3.bf.sa_permit = cfg->sa_permit;
    }

    if (!word4_p->bf.da_permit) {
        word3.bf.da_permit = cfg->da_permit;
    }

    if (!word4_p->bf.static_flag) {
        word3.bf.static_flag = cfg->static_flag;
    }

    if (!word4_p->bf.in_refresh_flag) {
        word3.bf.in_refresh_flag = cfg->in_refresh_flag;
    }

    if (!word4_p->bf.in_st_mv_flag) {
        word3.bf.in_st_mv_flag = cfg->in_st_mv_flag;
    }

    if (!word4_p->bf.tmp_flag) {
        word3.bf.tmp_flag = cfg->tmp_flag;
    }

    if (!word4_p->bf.key_vid) {
        word3.bf.key_vid_high = cfg->key_vid_high;
        word2.bf.key_vid_low = cfg->key_vid_low;
    }

    if (!word4_p->bf.lrned_vid) {
        word3.bf.lrned_vid_high = cfg->lrned_vid_high;
        word2.bf.lrned_vid_low = cfg->lrned_vid_low;
    }

    if (!word4_p->bf.key_dot1p) {
        word1.bf.key_dot1p_high = cfg->dot1p_high;
        word0.bf.key_dot1p_low = cfg->dot1p_low;
    }

    if (!word4_p->bf.aging_status) {
        word1.bf.aging_status_high = cfg->aging_status_high;
        word0.bf.aging_status_low = cfg->aging_status_low;
    }

    if (!word4_p->bf.mcgid) {
        word1.bf.mcgid_high = cfg->mcgid_high;
        word0.bf.mcgid_low = cfg->mcgid_low;
    }

    if (!word4_p->bf.lpid) {
        word1.bf.lpid_high = cfg->lpid_high;
        word0.bf.lpid_low = cfg->lpid_low;
    }

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD0, *(ca_uint32_t*)&word0);
        FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD1, *(ca_uint32_t*)&word1);
        FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD2, *(ca_uint32_t*)&word2);
        FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD3, *(ca_uint32_t*)&word3);
        FDB_REG_WRITE(L2FE_L2E_FDB_CPU_CMD_WORD4, *(ca_uint32_t*)word4_p);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(*(ca_uint32_t*)&word0, L2FE_L2E_FDB_CPU_CMD_WORD0);
        CA_8279_NE_REG_WRITE(*(ca_uint32_t*)&word1, L2FE_L2E_FDB_CPU_CMD_WORD1);
        CA_8279_NE_REG_WRITE(*(ca_uint32_t*)&word2, L2FE_L2E_FDB_CPU_CMD_WORD2);
        CA_8279_NE_REG_WRITE(*(ca_uint32_t*)&word3, L2FE_L2E_FDB_CPU_CMD_WORD3);
        CA_8279_NE_REG_WRITE(*(ca_uint32_t*)word4_p, L2FE_L2E_FDB_CPU_CMD_WORD4);
    }
#endif

    L2_DEBUG("word0/1/2/3/4: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n",
            *(ca_uint32_t*)&word0, *(ca_uint32_t*)&word1, *(ca_uint32_t*)&word2,
            *(ca_uint32_t*)&word3, *(ca_uint32_t*)word4_p);

    if (sw_lrn_cb.cfg){
        sw_lrn_cb.flush_cfg.mask = mask;
        sw_lrn_cb.flush_cfg.cfg = *cfg;
    }
    return CA_E_OK;
}


ca_status_t aal_fdb_scan_flush_field_get(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_fdb_flush_cfg_t *cfg
)
{
    aal_fdb_cpu_cmd_word4_msk_t word4;
    aal_fdb_cpu_cmd_word3_t word3;
    aal_fdb_cpu_cmd_word2_t word2;
    aal_fdb_cpu_cmd_word1_t word1;
    aal_fdb_cpu_cmd_word0_t word0;
    ca_uint32_t data;

    if (device_id == 0){
        data = CA_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD0);
        word0 = *(aal_fdb_cpu_cmd_word0_t *)&data;

        data = CA_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD1);
        word1 = *(aal_fdb_cpu_cmd_word1_t *)&data;

        data = CA_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD2);
        word2 = *(aal_fdb_cpu_cmd_word2_t *)&data;

        data = CA_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD3);
        word3 = *(aal_fdb_cpu_cmd_word3_t *)&data;

        data = CA_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD4);
        word4 = *(aal_fdb_cpu_cmd_word4_msk_t *)&data;
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        data = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD0);
        word0 = *(aal_fdb_cpu_cmd_word0_t *)&data;

        data = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD1);
        word1 = *(aal_fdb_cpu_cmd_word1_t *)&data;

        data = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD2);
        word2 = *(aal_fdb_cpu_cmd_word2_t *)&data;

        data = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD3);
        word3 = *(aal_fdb_cpu_cmd_word3_t *)&data;

        data = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CPU_CMD_WORD4);
        word4 = *(aal_fdb_cpu_cmd_word4_msk_t *)&data;
    }
#endif

    ca_printf("mask key_sc_ind = %d\r\n", word4.bf.key_sc_ind);
    ca_printf("     key_sc_ind : %d\r\n", word3.bf.key_sc_ind);

    ca_printf("mask mc_flag = %d\r\n", word4.bf.mc_flag);
    ca_printf("     mc_flag : %d\r\n", word3.bf.mc_flag);

    ca_printf("mask sa_permit = %d\r\n", word4.bf.sa_permit);
    ca_printf("     sa_permit : %d\r\n", word3.bf.sa_permit);

    ca_printf("mask da_permit = %d\r\n", word4.bf.da_permit);
    ca_printf("     da_permit : %d\r\n", word3.bf.da_permit);

    ca_printf("mask static_flag = %d\r\n", word4.bf.static_flag);
    ca_printf("     static_flag : %d\r\n", word3.bf.static_flag);

    ca_printf("mask in_refresh_flag = %d\r\n", word4.bf.in_refresh_flag);
    ca_printf("     in_refresh_flag : %d\r\n", word3.bf.in_refresh_flag);

    ca_printf("mask in_st_mv_flag = %d\r\n", word4.bf.in_st_mv_flag);
    ca_printf("     in_st_mv_flag : %d\r\n", word3.bf.in_st_mv_flag);

    ca_printf("mask tmp_flag = %d\r\n", word4.bf.tmp_flag);
    ca_printf("     tmp_flag : %d\r\n", word3.bf.tmp_flag);

    ca_printf("mask key_vid = %d\r\n", word4.bf.key_vid);
    ca_printf("     key_vid_high: %d\r\n", word3.bf.key_vid_high);
    ca_printf("     key_vid_low : %d\r\n", word2.bf.key_vid_low);

    ca_printf("mask lrned_vid = %d\r\n", word4.bf.lrned_vid);
    ca_printf("     lrned_vid_high: %d\r\n", word3.bf.lrned_vid_high);
    ca_printf("     lrned_vid_low : %d\r\n", word2.bf.lrned_vid_low);

    ca_printf("mask key_dot1p = %d\r\n", word4.bf.key_dot1p);
    ca_printf("     key_dot1p_high: %d\r\n", word1.bf.key_dot1p_high);
    ca_printf("     key_dot1p_low : %d\r\n", word0.bf.key_dot1p_low);

    ca_printf("mask aging_status = %d\r\n", word4.bf.aging_status);
    ca_printf("     aging_status_high: %d\r\n", word1.bf.aging_status_high);
    ca_printf("     aging_status_low : %d\r\n", word0.bf.aging_status_low);

    ca_printf("mask mcgid = %d\r\n", word4.bf.mcgid);
    ca_printf("     mcgid_high: %d\r\n", word1.bf.mcgid_high);
    ca_printf("     mcgid_low : %d\r\n", word0.bf.mcgid_low);

    ca_printf("mask lpid = %d\r\n", word4.bf.lpid);
    ca_printf("     lpid_high: %d\r\n", word1.bf.lpid_high);
    ca_printf("     lpid_low : %d\r\n", word0.bf.lpid_low);


    return CA_E_OK;
}
ca_status_t aal_fdb_scan_flush_field_init(CA_IN ca_device_id_t device_id)
{
    aal_fdb_flush_cfg_t cfg;
    aal_fdb_cpu_cmd_word4_msk_t  mask;

    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = 0xffffffff;
    /* only join compare lpid */
    mask.bf.lpid = 0;

    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    return CA_E_OK;
}

#define AAL_FDB_TABLE_SIZE          4096
#define AAL_FDB_OVERFLOW_TABLE_SIZE 32
#define AAL_FDB_MAX_NUM  (AAL_FDB_TABLE_SIZE + AAL_FDB_OVERFLOW_TABLE_SIZE)

ca_status_t aal_fdb_scan(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_uint32_t          start_offset,
    CA_OUT ca_uint32_t         *next_offset,
    CA_OUT aal_fdb_entry_cfg_t *fdb_entry
)
{
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;
    ca_uint32_t          word0 = start_offset;
    ca_status_t          ret = CA_E_OK;
    __l2fe_l2e_fdb_scan_return_t be_ret;

    AAL_ASSERT_RET(fdb_entry != NULL &&
        next_offset != NULL &&
        start_offset < AAL_FDB_MAX_NUM,
        CA_E_PARAM);


    memset((void *)&be_ret, 0, sizeof(__l2fe_l2e_fdb_scan_return_t));
    //memset((void *)&word0, 0, sizeof(aal_fdb_scan_t));
    //memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
    //memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
    //memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
    //memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

    fdb_data3.wrd = 0;
    fdb_data2.wrd = 0;
    fdb_data1.wrd = 0;
    fdb_data0.wrd = 0;


    //__FDB_LOCK();

    ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_SCAN,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&word0,
                           (ca_uint32 *)&fdb_data3,
                           (ca_uint32 *)&fdb_data2,
                           (ca_uint32 *)&fdb_data1,
                           (ca_uint32 *)&fdb_data0,
                           (ca_uint32 *)&be_ret);
    //__FDB_UNLOCK();

    if (ret != CA_E_OK) {
        L2_DEBUG("ret:%d\r\n", ret);
        return ret;
    }

    if (be_ret.status != CA_AAL_FDB_ACCESS_HIT) {
        L2_DEBUG("fdb not hit\r\n");
        return CA_AAL_FDB_SCAN_RETURN_END;
    }

    *next_offset            = be_ret.ret_address + 1;

    __aal_fdb_data_2_entry(fdb_data1, fdb_data0, fdb_entry);
    __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry->mac);
#if 0
    if (fdb_entry->mc_flag || (fdb_entry->valid && fdb_entry->aging_sts)) {
        L2_DEBUG("SCAN FDB %02X:%02X:%02X:%02X:%02X:%02X VID (%u) offset (%u)\n",
            fdb_entry->mac[0], fdb_entry->mac[1], fdb_entry->mac[2],
            fdb_entry->mac[3], fdb_entry->mac[4], fdb_entry->mac[5],
            fdb_entry->lrnt_vid,
            be_ret.ret_address
            );
    }
#endif
    return ret;

}

ca_status_t aal_fdb_entry_flush(
    CA_IN ca_device_id_t                  device_id
)
{
    L2FE_L2E_FDB_CMD_RETURN_t cmd_return;
    ca_status_t                ret;

    memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));

    L2_DEBUG("please make sure call function aal_fdb_scan_flush_field_set to set word0--word4\r\n");

    ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_FLUSH,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&cmd_return);


    if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT) {
        L2_DEBUG("fdb no hit, no any entry is flushed.\r\n");
        //return CA_E_ERROR;
    }

    return CA_E_OK;
}

/*
 */

ca_status_t aal_fdb_entry_flush_cmd(
    CA_IN ca_device_id_t              device_id,
    CA_IN __fdb_tbl_opcode_t       op_cmd
)
{
    L2FE_L2E_FDB_CMD_RETURN_t cmd_return;
    ca_status_t                ret;

    AAL_ASSERT_RET(op_cmd == __FDB_TBL_OPCODE_FLUSH || op_cmd == __FDB_TBL_OPCODE_FLUSH_OLDEST, CA_E_PARAM);

    memset((void *)&cmd_return, 0, sizeof(L2FE_L2E_FDB_CMD_RETURN_t));

    L2_DEBUG("please make sure call function aal_fdb_scan_flush_field_set to set word0--word4\r\n");
    L2_DEBUG("op_cmd:0x%x\r\n", op_cmd);

    ret = __aal_fdb_access(device_id, op_cmd,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&cmd_return);


    if (cmd_return.bf.status != CA_AAL_FDB_ACCESS_HIT) {
        L2_DEBUG("fdb no hit\r\n");
        return CA_E_ERROR;
    }

    return CA_E_OK;
}


ca_status_t aal_fdb_entry_scan(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32               start_addr,
    CA_OUT ca_uint32              *hit_addr,
    CA_OUT aal_fdb_entry_cfg_t    *fdb_entry,
    CA_OUT ca_uint32              *cmd_return
)
{
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;
    //aal_fdb_scan_t  word0;
    ca_uint32_t word0;

    __l2fe_l2e_fdb_scan_return_t be_ret;

    ca_status_t           ret;

    CA_ASSERT_RET(fdb_entry != NULL &&
                  cmd_return != NULL &&
                  start_addr < AAL_FDB_MAX_NUM, CA_E_PARAM);

    memset((void *)&be_ret, 0, sizeof(__l2fe_l2e_fdb_scan_return_t));
    //memset((void *)&word0, 0, sizeof(aal_fdb_scan_t));
    memset((void *)&fdb_data0, 0, sizeof(L2FE_L2E_FDB_DATA0_t));
    memset((void *)&fdb_data1, 0, sizeof(L2FE_L2E_FDB_DATA1_t));
    memset((void *)&fdb_data2, 0, sizeof(L2FE_L2E_FDB_DATA2_t));
    memset((void *)&fdb_data3, 0, sizeof(L2FE_L2E_FDB_DATA3_t));

    //word0.bf.start_addr = start_addr;
    word0 = start_addr;

//    L2_DEBUG("\r\n");

    //__FDB_LOCK();

    ret = __aal_fdb_access(device_id, __FDB_TBL_OPCODE_SCAN,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (ca_uint32 *)&word0,
                           (ca_uint32 *)&fdb_data3,
                           (ca_uint32 *)&fdb_data2,
                           (ca_uint32 *)&fdb_data1,
                           (ca_uint32 *)&fdb_data0,
                           (ca_uint32 *)&be_ret);

    if (ret != CA_E_OK) {
        //__FDB_UNLOCK();
        L2_DEBUG("ret:%d\r\n", ret);
        return ret;
    }

//    L2_DEBUG("\r\n");

    *cmd_return = be_ret.status;

    if (*cmd_return != CA_AAL_FDB_ACCESS_HIT) {
        //__FDB_UNLOCK();
        L2_DEBUG("fdb not hit\r\n");
        return CA_AAL_FDB_SCAN_RETURN_END;
    }
    else {
        *hit_addr = be_ret.ret_address;

        __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry->mac);
        __aal_fdb_data_2_entry(fdb_data1, fdb_data0, fdb_entry);

    }

    //__FDB_UNLOCK();

    return ret;
}

aal_fdb_entry_cfg_t *aal_fdb_static_getfirst(ca_table_t *table_p)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;

    if (table_p == NULL)
        return NULL;

    tmp_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getfirst(
                table_p);

    return tmp_p;
}

aal_fdb_entry_cfg_t *aal_fdb_static_getnext(ca_table_t *table_p)
{
    aal_fdb_entry_cfg_t *tmp_p = NULL;

    if (table_p == NULL)
        return NULL;

    tmp_p = (aal_fdb_entry_cfg_t *) ca_table_entry_getnext(
                table_p);

    return tmp_p;
}


ca_status_t aal_fdb_port_limit_init(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    ca_uint32               cmd_return = 0;
    ca_status_t             ret;

    /* port_id is lpid */
    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();
    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_INIT,
                             NULL,
                             &cmd_return);
    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_return:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_port_limit_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id,  /* lpid */
    CA_IN ca_uint32   addr_limit
)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t data;
    ca_uint32_t             current_limit = 0;
    ca_uint32               cmd_return;
    ca_status_t             ret;
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t max_data;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_stp_state_t old_stp_mode;

    max_data.wrd = ~0U;

    L2_DEBUG("device_id:%d, port_id:%d, limit:%d\r\n", device_id, port_id, addr_limit);

    /* keep max value if exceed */
    if (addr_limit > max_data.bf.val){
        L2_DEBUG("exceed port limit:%d\r\n", max_data.bf.val);
        return CA_E_PARAM;
    }

    __aal_fdb_sw_mac_limit_set(0, port_id, addr_limit);
    if (port_id == CA_UINT16_INVALID){
        L2_DEBUG("golbal mac limit :%d\r\n", addr_limit);
        return CA_E_OK;
    }

    CA_AAL_FDB_PORT_CHECK(port_id);

#if defined(CONFIG_LUNA_G3_SERIES)
	// luna support l2 sw learning by setting limit count to 0.
#else
    if (addr_limit == 0)
        addr_limit = max_data.bf.val;
#endif

    /* turn off learning */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, port_id, &igr_cfg);
    if (ret != CA_E_OK) {
        L2_DEBUG("aal_port_ilpb_cfg_get:%d\r\n", ret);
        return ret;
    }

    old_stp_mode = igr_cfg.stp_mode;

    igr_msk.s.stp_mode = 1;
    igr_cfg.stp_mode = AAL_STP_STATE_DROP;

    ret = aal_port_ilpb_cfg_set(device_id, port_id, igr_msk, &igr_cfg);

    /* read limit */
    memset((void *)&data, 0, sizeof(L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t));

    __FDB_LOCK();

    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_RD,
                             (ca_uint32 *)&data,
                             &cmd_return);
    current_limit = data.bf.val;
    __FDB_UNLOCK();

    if (addr_limit < current_limit){
        /* clear current port fdb table */
        ret = __aal_fdb_addr_delete_by_port(device_id, port_id, CA_L2_ADDR_OPERATION_DYNAMIC);
    }

    __FDB_LOCK();

    /* set limit */
    data.bf.val = addr_limit;

    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_WR,
                             (ca_uint32 *)&data,
                             &cmd_return);

    __FDB_UNLOCK();

    /* restore stp */
    igr_msk.s.stp_mode = 1;
    igr_cfg.stp_mode = old_stp_mode;

    ret = aal_port_ilpb_cfg_set(device_id, port_id, igr_msk, &igr_cfg);

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_return:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_port_limit_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id,
    CA_OUT ca_uint32  *addr_limit
)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t data;

    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_ASSERT_RET(addr_limit != NULL, CA_E_PARAM);
    if (port_id == CA_UINT16_INVALID){
        *addr_limit = sw_lrn_cb.total_mac.mac_limit;
        L2_DEBUG("golbal mac limit :%d\r\n", *addr_limit);
        return CA_E_OK;
    }

    CA_AAL_FDB_PORT_CHECK(port_id);
    memset((void *)&data, 0, sizeof(L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t));

    __FDB_LOCK();

    /* set limit */
    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_RD,
                             (ca_uint32 *)&data,
                             &cmd_return);

    *addr_limit = data.bf.val;

    __FDB_UNLOCK();

    L2_DEBUG("cmd_ret:%d, port:%d, limit:%d\r\n", cmd_return, port_id, *addr_limit);
    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_port_limit_inc(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    /* set limit */
    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_INC,
                             NULL,
                             &cmd_return);
    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}


ca_status_t aal_fdb_port_limit_dec(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    /* set limit */
    ret = __aal_port_mac_lmt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_LMT_DEC,
                             NULL,
                             &cmd_return);

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}


ca_status_t aal_fdb_port_cnt_init(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    ret = __aal_port_mac_cnt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_CNT_INIT,
                             NULL,
                             &cmd_return);

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}


ca_status_t aal_fdb_port_cnt_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id,
    CA_IN ca_uint32       addr_cnt
)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t data;

    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    memset((void *)&data, 0, sizeof(L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t));

    __FDB_LOCK();

    data.bf.val = addr_cnt;

    ret = __aal_port_mac_cnt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_CNT_WR,
                             (ca_uint32 *)&data,
                             &cmd_return);

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}


ca_status_t aal_fdb_port_cnt_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id,
    CA_OUT ca_uint32      *addr_cnt
)
{
    L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t data;

    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_ASSERT_RET(addr_cnt != NULL, CA_E_PARAM);

    CA_AAL_FDB_PORT_CHECK(port_id);

    memset((void *)&data, 0, sizeof(L2FE_L2E_PORT_MAC_CNT_LMT_DATA_t));
    __FDB_LOCK();

    ret = __aal_port_mac_cnt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_CNT_RD,
                             (ca_uint32 *)&data,
                             &cmd_return);

    *addr_cnt = data.bf.val;

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_return:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_port_cnt_inc(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id
)
{
    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    ret = __aal_port_mac_cnt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_CNT_INC,
                             NULL,
                             &cmd_return);

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_port_cnt_dec(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id
)
{
    ca_uint32               cmd_return;
    ca_status_t             ret;

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    ret = __aal_port_mac_cnt(device_id,
                             port_id,
                             __FDB_TBL_OPCODE_CNT_DEC,
                             NULL,
                             &cmd_return);

    __FDB_UNLOCK();

    if (cmd_return != CA_AAL_L3E_CNT_LMT_SUCCESS) {
        L2_DEBUG("cmd_ret:%d\r\n", cmd_return);
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_port_flooding_init(CA_IN ca_device_id_t device_id)
{
    aal_port_flooding_t *f_p = &flood_cb;
//    aal_l2_te_policer_cfg_msk_t  msk;
//    aal_l2_te_policer_cfg_t      cfg;
    ca_status_t ret;
    ca_int32_t i, j;

    memset(f_p, 0x0, sizeof(aal_port_flooding_t));

    for (i = 0; i <= AAL_L2TM_L2TE_POL_PKT_TYPE_PROFILE_NUM; i++){
        f_p->profile[i].rate = CA_UINT32_INVALID;
    }

    /*
    for (i = 1; i < AAL_ILPB_UNKWN_POL_NUM; i++){
        for (j = 0; j <= CA_L2_FLOODING_TYPE_UUC; j++){
            f_p->lspid_type_prof[i][j].profile_id = CA_UINT8_INVALID;
        }
    }
    */
    /* set L2FE_ARB_NON_KNOWN_POL_MAP_TBL_DATA to default profile */
    for (i = 0; i < AAL_ILPB_UNKWN_POL_NUM; i++){
        for (j = 0; j < AAL_ARB_PKT_FWD_TYPE_MAX; j++){
            f_p->lspid_type_prof[i][j].profile_id = CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE;
            ret = aal_arb_non_known_pol_map_set(device_id, i, j, CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE);

            if (ret != CA_E_OK){
                ca_printf("aal_port_flooding_init, aal_arb_non_known_pol_map_set FAIL!\n");
                return CA_E_ERROR;
            }
        }
    }

    /* set packet mode to pps */
    /*memset(&msk, 0, sizeof(aal_l2_te_policer_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_l2_te_policer_cfg_t));

    msk.s.pol_update_mode = 1;
    cfg.pol_update_mode = CA_AAL_POLICER_UPDATE_MODE_PKT;

    for (i = 0; i < AAL_L2TM_L2TE_POL_PKT_TYPE_PROFILE_NUM; i++){
        ret = aal_l2_te_policer_pkt_type_cfg_set(device_id, i, msk, &cfg);
        if (ret != CA_E_OK){
            ca_printf("aal_port_flooding_init FAIL!\n");
            return CA_E_ERROR;
        }
    }
    */
    return CA_E_OK;
}
ca_int32_t aal_port_ilpb_unknw_pol_alloc(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  lspid,
    CA_IN ca_uint8_t  pkt_type,
    CA_IN ca_uint8_t  profile
)
{
    aal_port_flooding_t *f_p = &flood_cb;
    ca_int32_t policer[AAL_ILPB_UNKWN_POL_NUM] = {0};
    int i;
    ca_int32_t available_idx, used_idx;

    CA_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL, -1);

    available_idx = used_idx = -1;

    for (i = 0; i < AAL_PORT_NUM_LOGICAL; i++){
        if (f_p->lspid_policer[i] != 0 && f_p->lspid_policer[i] < AAL_ILPB_UNKWN_POL_NUM)
            if (i == lspid)
                used_idx = f_p->lspid_policer[lspid];

            policer[f_p->lspid_policer[i]] = 1;
    }

    for (i = 1; i < AAL_ILPB_UNKWN_POL_NUM; i++){
        if (policer[i] == 0){
            available_idx = i;
            break;
        }
    }

    if (used_idx != -1)
        return used_idx;

    if (available_idx != -1)
        return available_idx;

    return -1;
}


ca_int32_t aal_port_pkt_type_profile_alloc(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t rate
)
{
    ca_int32_t i, available_idx, used_idx;
    aal_port_flooding_t *f_p = &flood_cb;
    //arb_non_known_pol_t *pol_p = NULL;
    l2te_pol_profile_t *profile_p = NULL;

    used_idx = available_idx = -1;

    for (i = 0; i < AAL_L2TM_L2TE_POL_PKT_TYPE_PROFILE_NUM; i++){
        profile_p = f_p->profile + i;

        if (profile_p->rate == rate && used_idx == -1){
           /* same profile */
            used_idx = i;
        }

        if (profile_p->rate == CA_UINT32_INVALID
            && available_idx == -1)
        {
            available_idx = i;
        }
    }

    L2_DEBUG("pkt_type profile used_idx:%d, available_idx:%d\r\n", used_idx, available_idx);
    /*
    if (used_idx != -1)
        return used_idx;
    */
    if (available_idx != -1)
        return available_idx;

    return -1;

}


ca_status_t __aal_port_flooding_update(
    ca_device_id_t device_id,
    ca_int32_t lspid,
    aal_arb_pkt_fwd_type_t arb_pkt_type,
    ca_int32_t unkwn_pol_id,
    ca_int32_t profile_id,
    ca_boolean_t pps,
    ca_uint32_t rate)
{
    aal_port_flooding_t *f_p = &flood_cb;
    ca_uint8_t policer_idx;
    int policer_used;
    int i;
    ca_status_t ret = CA_E_OK;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    ca_int32_t alloc_profile = 0;

    L2_DEBUG("lspid:%d, arb_pkt_type:%d, profile_id:%d, unkwn_pol_id:%d, rate:0x%x\r\n",
                lspid, arb_pkt_type, profile_id, unkwn_pol_id, rate);

    CA_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL
                  && arb_pkt_type <= AAL_ARB_PKT_FWD_TYPE_MAX, CA_E_PARAM);

    /* when rate CA_UINT32_INVALID, unkwn_pol_id and profile_id is invalid */
    if (rate == CA_UINT32_INVALID){
        policer_idx = f_p->lspid_policer[lspid];
        L2_DEBUG("policer_idx:%d, pkt_type:%d\r\n", policer_idx, arb_pkt_type);
        if (policer_idx == CA_UINT8_INVALID){
            L2_DEBUG("delete invalid policer_idx!\r\n");
            return CA_E_ERROR;
        }

        //f_p->lspid_policer[lspid] = 0; /* in arb, [0][0..2]= 15, default profile */
        alloc_profile = f_p->lspid_type_prof[policer_idx][arb_pkt_type].profile_id;
        f_p->lspid_type_prof[policer_idx][arb_pkt_type].profile_id = CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE;
        ret = aal_arb_non_known_pol_map_set(device_id, policer_idx, arb_pkt_type, CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE);
        policer_used = 0;
        for (i = 0; i <= AAL_ARB_PKT_FWD_TYPE_MAX; i++){
            if (f_p->lspid_type_prof[policer_idx][i].profile_id != CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE){
                //alloc_profile = f_p->lspid_type_prof[policer_idx][i].profile_id;
                policer_used = 1;
                break;
            }
        }

        if (policer_used == 0){
            /* release pol_idx in ilpb */
            f_p->lspid_policer[lspid] = 0;
            /* set to ilpb HW */
            memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
            memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

            igr_msk.s.unkwn_pol_idx = 1;
            igr_cfg.unkwn_pol_idx = 0;

            ret = aal_port_ilpb_cfg_set(device_id, lspid, igr_msk, &igr_cfg);
            L2_DEBUG("ret:%d\r\n", ret);
        }
        /* release profile */
        L2_DEBUG("policer_idx:%d, arb_pkt_type:%d, profile_id:%d\r\n", policer_idx, arb_pkt_type, alloc_profile);

        f_p->profile[alloc_profile].rate = rate;
        return CA_E_OK;
    }

    CA_ASSERT_RET(unkwn_pol_id < AAL_ILPB_UNKWN_POL_NUM
                  && profile_id < AAL_L2TM_L2TE_POL_PKT_TYPE_PROFILE_NUM, CA_E_PARAM);

    f_p->lspid_policer[lspid] = unkwn_pol_id;
    f_p->lspid_type_prof[unkwn_pol_id][arb_pkt_type].profile_id = profile_id;
    f_p->profile[profile_id].pps = pps;
    f_p->profile[profile_id].rate = rate;

    return CA_E_OK;
}

ca_int32_t __aal_port_flooding_profile_id_get(
    ca_l2_flooding_type_t pkt_type,
    ca_int32_t unkwn_pol_id)
{
    aal_port_flooding_t *f_p = &flood_cb;
    int i;

    CA_ASSERT_RET(pkt_type <= CA_L2_FLOODING_TYPE_UUC
                  && unkwn_pol_id < AAL_ILPB_UNKWN_POL_NUM
                  , CA_E_PARAM);

    i = f_p->lspid_type_prof[unkwn_pol_id][pkt_type].profile_id;

    L2_DEBUG("pkt_type:%d, pol_id:%d, profile:%d\r\n", pkt_type, unkwn_pol_id, i);

    return i;
}


ca_uint8_t __aal_port_flooding_pol_idx_get(
    ca_int32_t  lspid
)
{
    aal_port_flooding_t *f_p = &flood_cb;
    ca_uint8_t pol_id;

    CA_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL, CA_E_PARAM);

    L2_DEBUG("lspid:%d\r\n", lspid);

    pol_id = f_p->lspid_policer[lspid];

    return pol_id;
}

aal_arb_pkt_fwd_type_t __pkt_type_convert(ca_uint8_t  pkt_type)
{
    aal_arb_pkt_fwd_type_t arb_pkt_type = AAL_ARB_PKT_FWD_TYPE_BC;

    switch (pkt_type){
        case CA_L2_FLOODING_TYPE_MC:
            arb_pkt_type = AAL_ARB_PKT_FWD_TYPE_UMC;
            break;
        case CA_L2_FLOODING_TYPE_UUC:
            arb_pkt_type = AAL_ARB_PKT_FWD_TYPE_UUC;
            break;
        case CA_L2_FLOODING_TYPE_BC:
            arb_pkt_type = AAL_ARB_PKT_FWD_TYPE_BC;
            break;
        default:
            arb_pkt_type = AAL_ARB_PKT_FWD_TYPE_BC;
            break;
    }

    return arb_pkt_type;
}

ca_status_t aal_fdb_ilpb_unknw_pol_idx_map_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  lspid,
    CA_IN ca_uint8_t  pkt_type,
    CA_IN ca_boolean_t pps,
    CA_IN ca_uint32_t rate
)
{
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    aal_l2_te_policer_profile_msk_t profile_msk;
    aal_l2_te_policer_profile_t     profile_cfg;
    ca_int32_t unkwn_pol_id = -1, profile_id = -1;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t value;
    ca_uint8_t pol_idx;
    aal_port_flooding_t *f_p = &flood_cb;
    aal_arb_pkt_fwd_type_t arb_pkt_type;
    aal_l2_te_policer_cfg_msk_t pol_msk;
    aal_l2_te_policer_cfg_t     pol_cfg;


    L2_DEBUG("lspid:%d, pkt_type:%d, rate:%d\r\n", lspid, pkt_type, rate);

    CA_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL
                  && pkt_type <= CA_L2_FLOODING_TYPE_UUC, CA_E_PARAM);

    arb_pkt_type = __pkt_type_convert(pkt_type);

    /* TODO, rate = -1, set to default profile */
    if (rate == CA_UINT32_INVALID){
        pol_idx = f_p->lspid_policer[lspid];
        L2_DEBUG("pol_idx:%d\r\n", pol_idx);
        /* update table */
        ret = __aal_port_flooding_update(device_id, lspid, arb_pkt_type, unkwn_pol_id, profile_id, pps, rate);
#if 0
        if (ret == CA_E_OK && pol_idx < AAL_ILPB_UNKWN_POL_NUM && pol_idx > 0){
            for (i = 0; i <= CA_L2_FLOODING_TYPE_UUC; i++){
                //if (f_p->lspid_type_prof[pol_idx][i].profile != CA_UINT8_INVALID)
                f_p->lspid_type_prof[pol_idx][i].profile_id = CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE;
                //ret = aal_arb_non_known_pol_map_set(device_id, lspid, i, CA_AAL_FDB_PORT_FLOOD_DEF_PROFILE);
            }
        }
#endif
        return ret;
    }

    profile_id = aal_port_pkt_type_profile_alloc(device_id, rate);

    L2_DEBUG("profile_id:%d\r\n", profile_id);
    if (-1 == profile_id){
        ca_printf("pkt type profile resource unavailable\r\n");
        return CA_E_RESOURCE;
    }

    unkwn_pol_id = aal_port_ilpb_unknw_pol_alloc(device_id, lspid, arb_pkt_type, profile_id);

    L2_DEBUG("unkwn_pol_id:%d\r\n", unkwn_pol_id);
    if (-1 == unkwn_pol_id){
        ca_printf(" ilpd resource unavailable\r\n");
        return CA_E_RESOURCE;
    }

    memset((void *)&pol_cfg, 0, sizeof(aal_l2_te_policer_cfg_t));
    pol_msk.u32 = 0;

    pol_msk.s.pol_update_mode = 1;
    pol_cfg.pol_update_mode = (pps ? CA_AAL_POLICER_UPDATE_MODE_PKT : CA_AAL_POLICER_UPDATE_MODE_BYTE);

    /* packet type based policer settings */
    aal_l2_te_policer_pkt_type_cfg_set(device_id, profile_id, pol_msk, &pol_cfg);

    /* set to l2te HW */
    memset(&profile_msk, 0, sizeof(aal_l2_te_policer_profile_msk_t));
    memset(&profile_cfg, 0, sizeof(aal_l2_te_policer_profile_t));
    profile_msk.s.pir_m = 1;
    profile_msk.s.pir_k = 1;
    value = rate / 1000;
    profile_cfg.pir_m = (value > 0xffff ? 0xffff : value);
    value = rate % 1000;
    profile_cfg.pir_k = value;

    profile_msk.s.cir_m = 1;
    profile_msk.s.cir_k = 1;
    profile_cfg.cir_m = profile_cfg.pir_m;
    profile_cfg.cir_k = profile_cfg.pir_k;

    ret = aal_l2_te_policer_pkt_type_profile_set(device_id, profile_id, profile_msk, &profile_cfg);

    L2_DEBUG("ret:%d\r\n", ret);

    /* set arb */
    ret = aal_arb_non_known_pol_map_set(device_id, unkwn_pol_id, arb_pkt_type, profile_id);

    /* set to ilpb HW */
    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    igr_msk.s.unkwn_pol_idx = 1;
    igr_cfg.unkwn_pol_idx = unkwn_pol_id;

    ret = aal_port_ilpb_cfg_set(device_id, lspid, igr_msk, &igr_cfg);
    L2_DEBUG("set to ilpb HW, ilpb->pol_id, %d--->%d, ret:%d\r\n", lspid, unkwn_pol_id, ret);

    /* write to local table */
    ret = __aal_port_flooding_update(device_id, lspid, arb_pkt_type, unkwn_pol_id, profile_id, pps, rate);

    return CA_E_OK;

}


ca_status_t aal_fdb_ilpb_unknw_pol_idx_map_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  lspid,
    CA_IN ca_uint8_t  pkt_type,
    CA_IN ca_boolean_t *pps,
    CA_IN ca_uint32_t *rate
)
{
    aal_ilpb_cfg_t igr_cfg;
    aal_l2_te_policer_profile_t     profile_cfg;
    ca_int32_t unkwn_pol_id;
    ca_uint16_t profile_id = 0;
    ca_status_t ret = CA_E_OK;
    aal_arb_pkt_fwd_type_t arb_pkt_type;
    aal_l2_te_policer_cfg_t     pol_cfg;

    /*
    ca_printf("CA_L2_FLOODING_TYPE_MC  = %d\r\n", CA_L2_FLOODING_TYPE_MC);
    ca_printf("CA_L2_FLOODING_TYPE_BC  = %d\r\n", CA_L2_FLOODING_TYPE_BC);
    ca_printf("CA_L2_FLOODING_TYPE_UUC = %d\r\n", CA_L2_FLOODING_TYPE_UUC);
    */
    arb_pkt_type = __pkt_type_convert(pkt_type);
    L2_DEBUG("lspid:%d, pkt_type:%d, arb_pkt_type:%d\r\n", lspid, pkt_type, arb_pkt_type);

    CA_ASSERT_RET(lspid < AAL_PORT_NUM_LOGICAL
                  && pkt_type <= CA_L2_FLOODING_TYPE_UUC
                  && rate != NULL
                  && pps != NULL, CA_E_PARAM);

    /*
    ca_printf("ARB TYPE MC_UC = %d\r\n", AAL_ARB_PKT_FWD_TYPE_MC_UC);
    ca_printf("ARB TYPE UUC = %d\r\n",   AAL_ARB_PKT_FWD_TYPE_UUC);
    ca_printf("ARB TYPE UMC = %d\r\n",   AAL_ARB_PKT_FWD_TYPE_UMC);
    ca_printf("ARB TYPE BC = %d\r\n",    AAL_ARB_PKT_FWD_TYPE_BC);
    */
    /* get from ilpb HW */
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id, lspid, &igr_cfg);
    unkwn_pol_id = igr_cfg.unkwn_pol_idx;
    L2_DEBUG("ret:%d, unkwn_pol_idx:%d\r\n", ret, unkwn_pol_id);

    /* get from ARB */
    ret = aal_arb_non_known_pol_map_get(device_id, unkwn_pol_id, arb_pkt_type, &profile_id);
    L2_DEBUG("ret:%d, profile_id:%d\r\n", ret, profile_id);

    /*
    memset(&profile_cfg, 0, sizeof(aal_l2_te_policer_profile_t));
    profile_id = __aal_port_flooding_profile_id_get(pkt_type, unkwn_pol_id);

    if (profile_id == -1){
        L2_DEBUG("no specify profile\r\n");
        return ret;
    }
    */

    memset((void *)&pol_cfg, 0, sizeof(aal_l2_te_policer_cfg_t));

    /* packet type based policer */
    ret = aal_l2_te_policer_pkt_type_cfg_get(device_id, profile_id, &pol_cfg);
    *pps = (pol_cfg.pol_update_mode == CA_AAL_POLICER_UPDATE_MODE_PKT ? 1 : 0);

    /* get from l2te HW */
    ret = aal_l2_te_policer_pkt_type_profile_get(device_id, profile_id, &profile_cfg);

    L2_DEBUG("ret:%d, pir_m:%d, pir_k:%d\r\n", ret, profile_cfg.pir_m, profile_cfg.pir_k);
    ca_printf("%s:%d, pir_m:%d/0x%x, pir_k:%d/0x%x\r\n", __FUNCTION__,__LINE__,
        profile_cfg.pir_m, profile_cfg.pir_m, profile_cfg.pir_k, profile_cfg.pir_k);

    *rate = profile_cfg.pir_m * 1000 + profile_cfg.pir_k;

    return CA_E_OK;

}

ca_status_t aal_fdb_ilpb_unknw_pol_idx_cb_get(
    CA_IN ca_device_id_t device_id
)
{
    int i, j;
    aal_port_flooding_t *f_p = &flood_cb;

    printk("CA_L2_FLOODING_TYPE_MC  = %d\r\n", CA_L2_FLOODING_TYPE_MC);
    printk("CA_L2_FLOODING_TYPE_BC  = %d\r\n", CA_L2_FLOODING_TYPE_BC);
    printk("CA_L2_FLOODING_TYPE_UUC = %d\r\n", CA_L2_FLOODING_TYPE_UUC);


    printk("ARB TYPE MC_UC = %d\r\n", AAL_ARB_PKT_FWD_TYPE_MC_UC);
    printk("ARB TYPE UUC = %d\r\n",   AAL_ARB_PKT_FWD_TYPE_UUC);
    printk("ARB TYPE UMC = %d\r\n",   AAL_ARB_PKT_FWD_TYPE_UMC);
    printk("ARB TYPE BC = %d\r\n",    AAL_ARB_PKT_FWD_TYPE_BC);

    printk("---Local CB---\r\n");
    printk("--- lpid/policer idx mapping---\r\n");
    printk("      0   1   2   3   4   5   6   7   8   9\r\n");
    printk("      ---------------------------------------\r\n");

    for(i = 0; i < AAL_PORT_NUM_LOGICAL; i++){
        if (i % 10 == 0)
            printk(" %d:", i / 10);
        printk("   %d", f_p->lspid_policer[i]);
        if (i % 10 == 9)
            printk("\r\n");
    }

    printk("\r\n");

    printk("\r\n---port type profile assignment---\r\n");
    for (i = 0; i < AAL_ILPB_UNKWN_POL_NUM; i++){
        for (j = 0; j < AAL_ARB_PKT_FWD_TYPE_MAX; j++){
            printk("lspid_type_prof[%d][%d].profile_id = %d\r\n", i, j, f_p->lspid_type_prof[i][j].profile_id);
        }
    }

    printk("\r\n---profile rate---\r\n");
    for (i = 0; i < AAL_L2TM_L2TE_POL_PKT_TYPE_PROFILE_NUM; i++){
        printk("profile[%d].pps = %d\r\n", i, f_p->profile[i].pps);
        printk("profile[%d].rate = %d\r\n", i, f_p->profile[i].rate);
    }

    return CA_E_OK;

}

ca_status_t aal_fdb_entry_2_l2_addr(
    CA_OUT ca_l2_addr_entry_t *addr,
    CA_IN aal_fdb_entry_cfg_t *fdb
)
{
    CA_ASSERT_RET(fdb != NULL && addr != NULL, CA_E_PARAM);

    memcpy(addr->mac_addr, fdb->mac, CA_ETH_ADDR_LEN);
    addr->fwd_vid       = fdb->lrnt_vid;
    addr->vid           = fdb->key_vid;
    addr->dot1p         = fdb->dot1p;
    addr->static_flag   = fdb->static_flag;
    addr->sa_permit     = fdb->sa_permit;
    addr->da_permit     = fdb->da_permit;

    if (fdb->mc_flag){
        addr->mc_group_id = fdb->port_id;
        addr->port_id     = CA_UINT32_INVALID;
    }else{
        addr->port_id       = ca_port_id_get(fdb->port_id);
        addr->mc_group_id   = INVALID_MC_GROUP_ID;
    }
    addr->aging_timer  = FDB_AGING_TIME(fdb->aging_sts);

    return CA_E_OK;
}

ca_status_t aal_fdb_mac_filter_default_set (
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_int32     port_id,
    CA_IN ca_boolean_t drop_flag
)
{
    aal_l2_cls_l2rl2r_key_t cfg;
    aal_l2_cls_l2rl2r_key_mask_t cls_mask;
    aal_l2_cls_fib_t  fib;
    aal_l2_cls_fib_mask_t  fib_mask;
    ca_uint32 rule_0, rule_1;
    ca_uint32 entry_id = 0;
    ca_status_t ret = CA_E_OK;
    /*aal_tbl_mac_filter_entry_t *partner = NULL;
    aal_tbl_mac_filter_entry_t new_entry;
    aal_l2_cls_l2r_key_t *l2r_p = NULL;
    ca_uint32 partner_id;
    */
    aal_ilpb_cfg_t ilpb_cfg;

    CA_AAL_FDB_PORT_CHECK(port_id);
    L2_DEBUG("port_id:0x%x, entry_id:%d\r\n", port_id, entry_id);

    __FDB_LOCK();

    /* get partner id in one entry */
    /*
    partner_id = CA_AAL_RULE_ID_PARTNER(CA_AAL_FDB_MAC_FILTER_DFT_ID);

    partner = __aal_mac_filter_rule_get_by_rule(port_id, partner_id);
    */
    memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));

    ret = aal_port_ilpb_cfg_get(device_id,port_id,&ilpb_cfg);
    entry_id = ilpb_cfg.cls_start + ilpb_cfg.cls_length - 1;
    memset((void *)&cfg, 0, sizeof(aal_l2_cls_l2rl2r_key_t));
    cls_mask.u64 = 0;

    rule_0 = 0;
    rule_1 = 1;  /* default rule always use last one */

    cfg.valid = TRUE;
    //cfg.entry_type = AAL_L2_CLS_KEY_ENTRY_TYPE_2RULE1ENTRY;
    //cfg.two_rule_is_and = FALSE;
    cls_mask.s.valid = 1;
   // cls_mask.s.entry_type = 1;
   // cls_mask.s.two_rule_is_and = 1;

    /* rule 0 */
    /*
    if (partner) {
        cfg.rule_type[rule_0] = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
        l2r_p = &cfg.l2r_key[rule_0];

        __aal_mac_filter_load_cls_rule(l2r_p, partner->filter_cfg.mac,
                                       partner->filter_cfg.mac_flag,
                                       partner->filter_cfg.mask,
                                       partner->filter_cfg.vid);

    }
    */
    /* rule 1 */
    //cfg.rule_type_1 = AAL_L2_CLS_KEY_2RULE_TYPE_L2RULE;
    //cls_mask.s.rule_type_1 = 1;

    ret = aal_l2_cls_l2rl2r_key_get(device_id,entry_id,&cfg);
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
    cfg.valid                          = TRUE;
    cfg.ethernet_type_vld_1            = FALSE;
    cfg.l4_port_field_ctrl_1           = AAL_L2_CLS_L4_PORT_FIELD_CTRL_MASK_L4PORT;
    cfg.mac_field_ctrl_1               = AAL_L2_CLS_MAC_FIELD_CTRL_MASK_MAC;
    cfg.vlan_field_ctrl_1              = AAL_L2_CLS_VLAN_FIELD_CTRL_MASK_VLAN;
    cfg.dot1p_ctrl_1                   = AAL_L2_CLS_DOT1P_FIELD_CTRL_MASK_DOT1P;
    cfg.vlan_number_vld_1              = FALSE;

    /*** set rule ***/
    ret = aal_l2_cls_l2rl2r_key_set(device_id, entry_id, cls_mask,&cfg);

    if (ret != CA_E_OK) {
        L2_DEBUG("set rule entry_id:%d fail\r\n", entry_id);
    }

    /* fib 0 */
    /*
    memset(&fib, 0, sizeof(aal_l2_cls_fib_t));
    fib.cls_fib[0].mask.s.permit = 1;
    fib.cls_fib[0].permit_valid = 1;
    fib.cls_fib[0].permit = !partner->filter_cfg.drop_flag;
    */
    /* fib 1 */
    fib_mask.u64 = 0;
    fib_mask.s.permit_valid_1 = 1;
    fib_mask.s.spt_mode_1 = 1;
    fib.permit_valid_1 = 1;
    fib_mask.s.permit_1 = 1;
    fib.permit_1 = !drop_flag;
    fib.spt_mode_1 = AAL_L2_CLS_SPT_MODE_FWD_LEARN;

    /*** set fib ***/
    ret = aal_l2_cls_fib_set(device_id, entry_id,fib_mask,&fib);

    if (ret != CA_E_OK) {
        L2_DEBUG("set fib entry_id:%d fail\r\n", entry_id);
        goto end;
    }

    /* modify rb tree */
    /* -- not save in rb tree
    memset(&new_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));
    new_entry.port_id = port_id;
    new_entry.rule_id = CA_AAL_FDB_MAC_FILTER_DFT_ID;
    new_entry.filter_cfg.drop_flag = drop_flag;
    __aal_mac_filter_tbl_add_del(__FDB_TBL_OPCODE_APPEND, &new_entry);
    */
    /* save default behavior*/
    mac_filter_dft[port_id] = drop_flag;

    __FDB_UNLOCK();

    return CA_E_OK;

end:
    __FDB_UNLOCK();
    return ret;


}


ca_status_t aal_fdb_mac_filter_default_get (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_int32     port_id,
    CA_OUT ca_boolean_t *drop_flag
)
{

    CA_ASSERT_RET(drop_flag != NULL, CA_E_PARAM);

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    *drop_flag = mac_filter_dft[port_id];

    L2_DEBUG(" drop_flag:%d\r\n", *drop_flag);

    __FDB_UNLOCK();

    return CA_E_OK;
}

ca_status_t __aal_l2_macfilter_cls_key_get(
            ca_device_id_t                 device_id,
            aal_tbl_mac_filter_entry_t      *key,
            ca_classifier_key_t             *l2_key,
            ca_classifier_key_mask_t        *l2_key_mask,
            ca_classifier_action_t          *l2_action
    )
{
    if(key == NULL || l2_key ==NULL || l2_key_mask == NULL || l2_action == NULL){
        return CA_E_PARAM;
    }
    
    if(key->port_id > 7){
         return CA_E_PARAM;
    }

    if(key->vid > 4095){
        return CA_E_PARAM;
    }
    
    if(key->mask > 2){
        return CA_E_PARAM;
    }
    
    if(key->mac_flag >1){
        return CA_E_PARAM;
    }

    l2_key->src_port = (0x30000|key->port_id);
    l2_key_mask->src_port = 1;
    l2_key_mask->l2 = 1;

    if(key->mac_flag == 0){
         l2_key_mask->l2_mask.mac_sa = 1;
         memcpy(l2_key->l2.mac_sa.mac_min, key->mac,sizeof(ca_mac_addr_t));
         memcpy(l2_key->l2.mac_sa.mac_max, key->mac,sizeof(ca_mac_addr_t));
     }else{
         l2_key_mask->l2_mask.mac_da = 1;
         memcpy(l2_key->l2.mac_da.mac_min, key->mac,sizeof(ca_mac_addr_t));
         memcpy(l2_key->l2.mac_da.mac_max, key->mac,sizeof(ca_mac_addr_t));
     }

    if(key->mask == 2){
        l2_key_mask->l2_mask.vlan_otag          = 1;
        l2_key_mask->l2_mask.vlan_otag_mask.vid = 1;
        l2_key->l2.vlan_otag.vlan_min.vid       = key->vid;
        l2_key->l2.vlan_otag.vlan_max.vid       = key->vid;
    }   
    if(key->drop_flag){
        l2_action->forward = CA_CLASSIFIER_FORWARD_DENY;
    }else{
        l2_action->forward = CA_CLASSIFIER_FORWARD_FE;
    }
    return CA_E_OK;
}
ca_status_t aal_fdb_mac_filter_add (
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_int32                 port_id,
    CA_IN aal_tbl_mac_filter_entry_t *entry
)
{
    aal_tbl_mac_filter_entry_t new_entry;
    ca_uint32 sw_index = 0;
    //aal_l2_cls_l2r_key_t *l2r_p = NULL;
    //ca_uint32 entry_id;
    //aal_l2_cls_l2rl2r_key_t cfg;
    //aal_l2_cls_l2rl2r_key_mask_t cls_mask;
    ca_status_t ret = CA_E_OK;
    //aal_l2_cls_fib_t fib;
    //aal_l2_cls_fib_mask_t  fib_mask;
    ca_classifier_key_t         cls_key;
    ca_classifier_key_mask_t    cls_key_mask;
    ca_classifier_action_t      cls_action;

    CA_ASSERT_RET(entry != NULL, CA_E_PARAM);

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    memset(&cls_key, 0, sizeof(ca_classifier_key_t));
    memset(&cls_action, 0, sizeof(ca_classifier_action_t));
    memset(&cls_key_mask, 0, sizeof(ca_classifier_key_mask_t));

    ret = __aal_l2_macfilter_cls_key_get(device_id,entry,&cls_key,&cls_key_mask,&cls_action);
    if(CA_E_OK != ret){
        __FDB_UNLOCK();
        return ret;
    }
    ret = l2_classifier_rule_add(device_id,0,&cls_key,&cls_key_mask,&cls_action,&sw_index);

    if (ret == CA_E_OK){
        /* modify rb tree */
        memset((void *)&new_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));
        new_entry.port_id = port_id;
        new_entry.entry_id = sw_index;
        new_entry.mask = entry->mask;
        new_entry.mac_flag = entry->mac_flag;

        if (entry->mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN)
            new_entry.vid = entry->vid;
        new_entry.drop_flag = entry->drop_flag;
        memcpy(&new_entry.mac, &entry->mac, CA_ETH_ADDR_LEN);

        L2_DEBUG("insert rb tree, port:%d, rule:%d/%d, mask:%d, mac_flag:%d, vid:%d, drop_flag:%d, mac:0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            new_entry.port_id, new_entry.entry_id, new_entry.rule_id, new_entry.mask, new_entry.mac_flag, new_entry.vid, new_entry.drop_flag,
            new_entry.mac[0], new_entry.mac[1], new_entry.mac[2], new_entry.mac[3], new_entry.mac[4], new_entry.mac[5]);
        __aal_mac_filter_tbl_add_del(__FDB_TBL_OPCODE_APPEND, &new_entry);
    }
    __FDB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_fdb_mac_filter_delete (
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_int32                 port_id,
    CA_IN aal_tbl_mac_filter_entry_t *entry
)
{
    aal_tbl_mac_filter_entry_t *rule = NULL;
    aal_tbl_mac_filter_entry_t new_entry;
    ca_uint32 entry_id = 0;
    ca_status_t ret = CA_E_OK;

    CA_ASSERT_RET(entry != NULL, CA_E_PARAM);

    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();

    rule = __aal_mac_filter_rule_get_by_key(port_id, entry);

    if (rule == NULL) {
        __FDB_UNLOCK();
        L2_DEBUG("no found entry\r\n");
        return CA_E_UNAVAIL;
    }

    entry_id = rule->entry_id;

    ret = l2_classifier_rule_del(device_id,entry_id);
    if(ret != CA_E_OK){
        __FDB_UNLOCK();        
        L2_DEBUG("l2_classifier_rule_del return %d\r\n",ret);
        return ret;
    }

    /* modify rb tree */
    memset((void *)&new_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));
    new_entry.port_id = port_id;
    new_entry.entry_id = entry_id;
    new_entry.mask = entry->mask;
    new_entry.mac_flag = entry->mac_flag;
    if (entry->mask == CA_L2_MAC_FILTER_MASK_MAC_VLAN)
        new_entry.vid = entry->vid;
    new_entry.drop_flag = entry->drop_flag;
    memcpy(&new_entry.mac, &entry->mac, CA_ETH_ADDR_LEN);

    L2_DEBUG("delete rb tree, port:%d, rule:%d, mask:%d, mac_flag:%d, vid:%d, drop_flag:%d, mac:0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
        new_entry.port_id, new_entry.rule_id, new_entry.mask, new_entry.mac_flag, new_entry.vid, new_entry.drop_flag,
        new_entry.mac[0], new_entry.mac[1], new_entry.mac[2], new_entry.mac[3], new_entry.mac[4], new_entry.mac[5]);

    __aal_mac_filter_tbl_add_del(__FDB_TBL_OPCODE_DELETE, &new_entry);

    __FDB_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_fdb_mac_filter_delete_all (
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_int32     port_id
)
{
    ca_status_t ret = CA_E_OK;
    aal_tbl_mac_filter_entry_t *rule = NULL;


    L2_DEBUG("port_id:%d\r\n", port_id);
    CA_AAL_FDB_PORT_CHECK(port_id);

    __FDB_LOCK();
    while(1){
        rule = __aal_mac_filter_rule_getfirst_port(port_id);
        if(NULL == rule){
            __FDB_UNLOCK();
            return CA_E_OK;
        }
        ret = l2_classifier_rule_del(device_id,rule->entry_id);
        if(ret != CA_E_OK){
            __FDB_UNLOCK();
            return ret;
        }
        ret= __aal_mac_filter_tbl_add_del(__FDB_TBL_OPCODE_DELETE, rule);
        if(ret != CA_E_OK){
            __FDB_UNLOCK();
            return ret;
        }
    }
    __FDB_UNLOCK();

    return CA_E_OK;
}

aal_tbl_mac_filter_entry_t *aal_fdb_mac_filter_getfirst(void)
{
    aal_tbl_mac_filter_entry_t *tmp_p = NULL;

    tmp_p = (aal_tbl_mac_filter_entry_t *) ca_table_entry_getfirst(
                mac_filter_tbl_p);

    return tmp_p;
}

aal_tbl_mac_filter_entry_t *aal_fdb_mac_filter_getnext(void)
{
    aal_tbl_mac_filter_entry_t *tmp_p = NULL;

    tmp_p = (aal_tbl_mac_filter_entry_t *) ca_table_entry_getnext(
                mac_filter_tbl_p);

    return tmp_p;
}


ca_status_t aal_l2e_hash_collision_int_set(ca_device_id_t device_id, ca_boolean_t flag)
{
    L2FE_GLB_INTERRUPTE_t l2fe_glb;


    L2_DEBUG("hash collision enable:%d\r\n", flag);
    if (device_id == 0){
        l2fe_glb.wrd  = CA_NE_REG_READ(L2FE_GLB_INTERRUPTE);

        l2fe_glb.bf.l2e_hash_collision_intIE = flag;

        FDB_REG_WRITE(L2FE_GLB_INTERRUPTE, l2fe_glb.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2fe_glb.wrd  = CA_8279_NE_REG_READ(L2FE_GLB_INTERRUPTE);

        l2fe_glb.bf.l2e_hash_collision_intIE = flag;

        CA_8279_NE_REG_WRITE(l2fe_glb.wrd, L2FE_GLB_INTERRUPTE);
    }
#endif
    return CA_E_OK;
}

ca_status_t aal_fdb_fifo_clean(
    ca_device_id_t     device_id
)
{
    ca_status_t ret = CA_E_OK;
    aal_fdb_ctrl_mask_t  fdb_ctrl_mask;
    aal_fdb_ctrl_t       fdb_ctrl;
    ca_int32_t pspid = 0, jj = 0;
    NI_HV_GLB_RXMUX_CTRL_CFG_t rxmux;

    memset(&fdb_ctrl_mask, 0, sizeof(aal_fdb_ctrl_mask_t));
    memset(&fdb_ctrl, 0, sizeof(aal_fdb_ctrl_t));
    memset(&rxmux, 0, sizeof(NI_HV_GLB_RXMUX_CTRL_CFG_t));

    /* lrn_dis, hw/confirm mode */
    fdb_ctrl_mask.s.lrn_dis = 1;
    fdb_ctrl_mask.s.lrn_mode = 1;
    fdb_ctrl.lrn_dis = 1;
    fdb_ctrl.lrn_mode = CA_AAL_FDB_LRN_MODE_HW;
    ret = aal_fdb_ctrl_set(device_id, fdb_ctrl_mask, &fdb_ctrl);
    if (CA_E_OK != ret) {
        ca_printf("aal_fdb_fifo_clean: fdb_ctrl_set FAIL, return %#x\n", ret);
    }

    /*
     a. NI_GLB_RXMUX_CTRL_CFG_dma_pspid <= 0
     b. CPU transmit packets through DMA_LSO to L2FE
        i.  Transmit 32 packets
        ii. Each packet is 64bytes packet
        iii.Set fe bypass, and Packet destination is blackhole(drop)
     c.	Repeat change NI_GLB_RXMUX_CTRL_CFG_dma_pspid from 1 to 16;
        i.  Repeat b -> c
     */

    rxmux.wrd = CA_DEV_NI_REG_READ(device_id, NI_HV_GLB_RXMUX_CTRL_CFG);
    for (pspid = 0; pspid < 16; pspid++){
        rxmux.bf.dma_pspid = pspid;
        CA_DEV_NI_REG_WRITE2(device_id, NI_HV_GLB_RXMUX_CTRL_CFG, rxmux.wrd);
        for (jj = 0; jj < 32; jj++)
            __aal_fdb_send_dumb_pkt(device_id, 64, AAL_LPORT_BLACKHOLE, 0);
    }

    /* set NI_GLB_RXMUX_CTRL_CFG_dma_pspid <= 4'h9(default value) */
    rxmux.bf.dma_pspid = 9;
    CA_DEV_NI_REG_WRITE2(device_id, NI_HV_GLB_RXMUX_CTRL_CFG, rxmux.wrd);

    return ret;
}

ca_status_t aal_fdb_init(
    ca_device_id_t     device_id
)
{
    ca_int32 ret = 0;
    ca_int32 i = 0;
    aal_fdb_ctrl_mask_t  fdb_ctrl_mask;
    aal_fdb_ctrl_t       fdb_ctrl;
    lrn_fwd_ctrl_t       fwd_ctrl;
    lrn_fwd_ctrl_mask_t  fwd_ctrl_mask;
    struct ca_soc_data   soc_data;
    ca_uint32_t customer_bit = 0xFFFFFFFF;

    aal_fdb_fifo_clean(device_id);
    if (CA_E_OK != scfg_read(0, CFG_ID_CUSTOMER_BIT,
                                sizeof(ca_uint32_t), (ca_uint8_t *)&customer_bit)) {
        ca_printf("SCFG read %s failed!!!\n", CFG_ID_CUSTOMER_BIT);
    }
    if (ca_soc_data_get(&soc_data) != 0)
        ca_printf("Fail to get SoC data.\n");

    __aal_fdb_tbl_create();
    __aal_mac_filter_tbl_create();
    __aal_fdb_sw_lrn_mode_get();
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    aal_fdb_ipc_init();
    aal_fdb_port_limit_set(0, 0xffff, 8);
#else
    __aal_fdb_sw_lrn_init(device_id);
#endif

    for (i = 0; i < 64; i++) {
        aal_fdb_port_limit_set(0, i, 0x1fff);
    }
    ret = ca_spin_lock_init(&g_fdb_lock, SPINLOCK_BH);

    if (ret != 0) {
        ca_printf("create spin lock %s fail\r\n", CA_AAL_FDB_MUTEX_NAME);
    }


    aal_fdb_scan_flush_field_init(0);
    fdb_ctrl_mask.wrd = ~0U;
#if 0
#ifdef CONFIG_CORTINA_BOARD_FPGA
    fdb_ctrl.aging_en                    = 0;
#else
    fdb_ctrl.aging_en                    = 1;
#endif
#else
    fdb_ctrl.aging_en                    = 1;
#endif
    fdb_ctrl.aging_out_time              = CA_AAL_FDB_DEF_AGING_OUT_TIME;

    if (soc_data.core_clk != 0) {
        /* micro-second base timer is 256, aging status timer is 255 */
        fdb_ctrl.cnt_to_1s               = soc_data.core_clk / 256 / 255;
    } else {
        fdb_ctrl.cnt_to_1s               = 3830; /* assume core clock is 250MHz */
    }

    fdb_ctrl.tmp_entry_aging_dis         = 0;
    fdb_ctrl.st_move_en                  = 1;
    fdb_ctrl.lrn_dis                     = 0;
    fdb_ctrl.da_deny_dnt_lrn             = 0;
    fdb_ctrl.hash_collision_overwrite    = 1;
    fdb_ctrl.ovfl_entries_rcyc_en        = 0;
    fdb_ctrl.hashing_mode                = 3;
    if (using_sw_lrn){
        fdb_ctrl.lrn_mode                    = CA_AAL_FDB_LRN_MODE_SW;
        fdb_ctrl.aging_mode                  = 1;
    }
    else{
    fdb_ctrl.lrn_mode                    = CA_AAL_FDB_LRN_MODE_HW;
    fdb_ctrl.aging_mode                  = 0;
    }
    fdb_ctrl.aging_start_val             = CA_AAL_FDB_DEF_AGING_START_VAL;
    fdb_ctrl.st_mv_window                = 0xFC;
    fdb_ctrl.cpu_opt_mode                = 1;
    fdb_ctrl.unkvlan_learn               = 1;
    fdb_ctrl.unk_ethtype_no_learn        = 0;
    fdb_ctrl.igr_dropped_pkt_no_learn    = 1;
    fdb_ctrl.non_std_sa_all_zero_learn   = 0;
    fdb_ctrl.non_std_sa_all_one_learn    = 0;
    fdb_ctrl.non_std_sa_b40_is_one_learn = 0;
    fdb_ctrl.loop_pkt_no_learn           = 1;

    ret = aal_fdb_ctrl_set(device_id, fdb_ctrl_mask, &fdb_ctrl);
    if (CA_E_OK != ret) {
        ca_printf("aal_fdb_ctrl_set FAIL, return %#x\n", ret);
    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ret = aal_fdb_ctrl_set(1, fdb_ctrl_mask, &fdb_ctrl);
    if (CA_E_OK != ret) {
        ca_printf("device 1 aal_fdb_ctrl_set FAIL, return %#x\n", ret);
    }
#endif

    /* enable l2e hash collision interrupt */
    aal_l2e_hash_collision_int_set(device_id, TRUE);

    /* default set to forward to cpu when hash collision */
    memset(&fwd_ctrl_mask, 0, sizeof(lrn_fwd_ctrl_mask_t));
    memset(&fwd_ctrl, 0, sizeof(lrn_fwd_ctrl_t));

    fwd_ctrl_mask.bf.sa_hash_cllsn_fwd_ctrl = 1;
    fwd_ctrl.hash_cllsn_mode = SA_LRN_FWD_CTRL_DROP;
    fwd_ctrl_mask.bf.sa_hash_cllsn_ldpid = 1;
    fwd_ctrl.hash_cllsn_ldpid = AAL_LPORT_CPU_0;
    fwd_ctrl_mask.bf.sa_limit_excd_fwd_ctrl = 1;
    fwd_ctrl.limit_excd_mode = SA_LRN_FWD_CTRL_DROP;
    fwd_ctrl_mask.bf.sa_limit_excd_ldpid = 1;
    fwd_ctrl.limit_excd_ldpid = AAL_LPORT_CPU_0;
    fwd_ctrl_mask.bf.l2_mac_sw_lrn_ldpid = 1;
    fwd_ctrl.l2_mac_sw_lrn_ldpid = AAL_LPORT_CPU_0;
    fwd_ctrl_mask.bf.sm_static_entry_fwd = 1;
    fwd_ctrl.sm_static_entry_fwd = 1;
    fwd_ctrl_mask.bf.l2_mac_sw_lrn_cos = 1;
    fwd_ctrl.l2_mac_sw_lrn_cos = 7;

    /*DROP by default*/
    fwd_ctrl.hash_cllsn_mode = SA_LRN_FWD_CTRL_DROP;
    fwd_ctrl.limit_excd_mode = SA_LRN_FWD_CTRL_DROP;


    aal_fdb_lrn_fwd_ctrl_set(device_id, fwd_ctrl_mask, &fwd_ctrl);

    aal_port_flooding_init(device_id);

    fdb_init_done = 1;
    ca_printf("FDB init OK\n");

    return CA_E_OK;
}

ca_status_t aal_fdb_mac_filter_init(
    ca_device_id_t     device_id
)
{
    int i = 0;

    memset((void *)&mac_filter_dft, 0, sizeof(mac_filter_dft));

    for (i = 0; i < CA_AAL_FDB_ETH_PORT_NUM; i++){
        aal_fdb_mac_filter_default_set(device_id, i, FALSE);
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_fdb_ctrl_mask_t  ctrl_mask,
    CA_IN aal_fdb_ctrl_t       *ctrl
    )
{
    L2FE_L2E_CTRL_t  l2e_ctrl;
    L2FE_PLC_L2_LEARNING_CTRL_t  l2_lrn;
    L2FE_L2E_AGING_CTRL_t l2e_aging;

    __FDB_LOCK();

    if (device_id == 0){
        l2e_ctrl.wrd  = CA_NE_REG_READ(L2FE_L2E_CTRL);
        l2_lrn.wrd    = CA_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
        l2e_aging.wrd = CA_NE_REG_READ(L2FE_L2E_AGING_CTRL);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2e_ctrl.wrd  = CA_8279_NE_REG_READ(L2FE_L2E_CTRL);
        l2_lrn.wrd    = CA_8279_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
        l2e_aging.wrd = CA_8279_NE_REG_READ(L2FE_L2E_AGING_CTRL);
    }
#endif
    if (ctrl_mask.s.aging_en             )
        l2e_ctrl.bf.aging_en              = ctrl->aging_en;
    if (ctrl_mask.s.tmp_entry_aging_dis  )
        l2e_ctrl.bf.tmp_entry_aging_dis   = ctrl->tmp_entry_aging_dis;
    if (ctrl_mask.s.st_move_en           )
        l2e_ctrl.bf.st_move_en            = ctrl->st_move_en;
    if (ctrl_mask.s.lrn_dis              )
        l2e_ctrl.bf.lrn_dis               = ctrl->lrn_dis;
    if (ctrl_mask.s.da_deny_dnt_lrn      )
        l2e_ctrl.bf.da_deny_dnt_lrn       = ctrl->da_deny_dnt_lrn;
    if (ctrl_mask.s.hash_collision_overwrite)
        l2e_ctrl.bf.hash_collision_overwrite = ctrl->hash_collision_overwrite;
    if (ctrl_mask.s.ovfl_entries_rcyc_en )
        l2e_ctrl.bf.ovfl_entries_rcyc_en  = ctrl->ovfl_entries_rcyc_en;
    if (ctrl_mask.s.hashing_mode         )
        l2e_ctrl.bf.hashing_mode          = ctrl->hashing_mode;
    if (ctrl_mask.s.lrn_mode             )
        l2e_ctrl.bf.lrn_mode              = ctrl->lrn_mode;
    if (ctrl_mask.s.aging_mode           )
        l2e_ctrl.bf.aging_mode            = ctrl->aging_mode;
    if (ctrl_mask.s.aging_start_val      )
        l2e_ctrl.bf.aging_start_val       = ctrl->aging_start_val;
    if (ctrl_mask.s.st_mv_window         )
        l2e_ctrl.bf.st_mv_window          = ctrl->st_mv_window;
    if (ctrl_mask.s.cpu_opt_mode         )
        l2e_ctrl.bf.cpu_opt_mode          = ctrl->cpu_opt_mode;
    if (ctrl_mask.s.unkvlan_learn        )
        l2_lrn.bf.unkvlan_learn           = ctrl->unkvlan_learn;
    if (ctrl_mask.s.unk_ethtype_no_learn )
        l2_lrn.bf.unk_ethtype_no_learn   = ctrl->unk_ethtype_no_learn;
    if (ctrl_mask.s.igr_dropped_pkt_no_learn )
        l2_lrn.bf.igr_dropped_pkt_no_learn  = ctrl->igr_dropped_pkt_no_learn;
    if (ctrl_mask.s.non_std_sa_all_zero_learn)
        l2_lrn.bf.non_std_sa_all_zero_learn  = ctrl->non_std_sa_all_zero_learn;
    if (ctrl_mask.s.non_std_sa_all_one_learn )
        l2_lrn.bf.non_std_sa_all_one_learn  = ctrl->non_std_sa_all_one_learn;
    if (ctrl_mask.s.non_std_sa_b40_is_one_learn)
        l2_lrn.bf.non_std_sa_b40_is_one_learn = ctrl->non_std_sa_b40_is_one_learn;
    if (ctrl_mask.s.loop_pkt_no_learn)
        l2_lrn.bf.loop_pkt_no_learn = ctrl->loop_pkt_no_learn;
    if (ctrl_mask.s.aging_out_time){
        l2e_aging.bf.aging_out_time = ctrl->aging_out_time;
        l2_aging_time = ctrl->aging_out_time;
    }
    if (ctrl_mask.s.cnt_to_1s)
        l2e_aging.bf.cnt_to_1s = ctrl->cnt_to_1s;

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_CTRL, l2e_ctrl.wrd);
        FDB_REG_WRITE(L2FE_PLC_L2_LEARNING_CTRL, l2_lrn.wrd);
        FDB_REG_WRITE(L2FE_L2E_AGING_CTRL, l2e_aging.wrd);
        if (using_sw_lrn)
            CA_NE_REG_WRITE(3, L2FE_L2E_AGING_CTRL_EXT);
        else
            CA_NE_REG_WRITE(2, L2FE_L2E_AGING_CTRL_EXT);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(l2e_ctrl.wrd, L2FE_L2E_CTRL);
        CA_8279_NE_REG_WRITE(l2_lrn.wrd, L2FE_PLC_L2_LEARNING_CTRL);
        CA_8279_NE_REG_WRITE(l2e_aging.wrd, L2FE_L2E_AGING_CTRL);
    }
#endif
    __FDB_UNLOCK();

    if (ctrl_mask.s.aging_out_time){
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
        aal_fdb_ipc_aging_time_send(ctrl->aging_out_time);
#endif
    }
    return CA_E_OK;
}


ca_status_t aal_fdb_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_fdb_ctrl_t  *ctrl)
{
    L2FE_L2E_CTRL_t             l2e_ctrl;
    L2FE_PLC_L2_LEARNING_CTRL_t l2_lrn;
    L2FE_L2E_AGING_CTRL_t       l2e_aging;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    if (device_id == 0){
        l2e_ctrl.wrd  = CA_NE_REG_READ(L2FE_L2E_CTRL);
        l2_lrn.wrd    = CA_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
        l2e_aging.wrd = CA_NE_REG_READ(L2FE_L2E_AGING_CTRL);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2e_ctrl.wrd  = CA_8279_NE_REG_READ(L2FE_L2E_CTRL);
        l2_lrn.wrd    = CA_8279_NE_REG_READ(L2FE_PLC_L2_LEARNING_CTRL);
        l2e_aging.wrd = CA_8279_NE_REG_READ(L2FE_L2E_AGING_CTRL);
    }
#endif
    ctrl->aging_en                    = l2e_ctrl.bf.aging_en;
    ctrl->tmp_entry_aging_dis         = l2e_ctrl.bf.tmp_entry_aging_dis;
    ctrl->st_move_en                  = l2e_ctrl.bf.st_move_en;
    ctrl->lrn_dis                     = l2e_ctrl.bf.lrn_dis;
    ctrl->da_deny_dnt_lrn             = l2e_ctrl.bf.da_deny_dnt_lrn;
    ctrl->hash_collision_overwrite    = l2e_ctrl.bf.hash_collision_overwrite;
    ctrl->ovfl_entries_rcyc_en        = l2e_ctrl.bf.ovfl_entries_rcyc_en;
    ctrl->hashing_mode                = l2e_ctrl.bf.hashing_mode;
    ctrl->lrn_mode                    = l2e_ctrl.bf.lrn_mode;
    ctrl->aging_mode                  = l2e_ctrl.bf.aging_mode;
    ctrl->aging_start_val             = l2e_ctrl.bf.aging_start_val;
    ctrl->st_mv_window                = l2e_ctrl.bf.st_mv_window;
    ctrl->cpu_opt_mode                = l2e_ctrl.bf.cpu_opt_mode;
    ctrl->unkvlan_learn               = l2_lrn.bf.unkvlan_learn              ;
    ctrl->unk_ethtype_no_learn        = l2_lrn.bf.unk_ethtype_no_learn       ;
    ctrl->igr_dropped_pkt_no_learn    = l2_lrn.bf.igr_dropped_pkt_no_learn   ;
    ctrl->non_std_sa_all_zero_learn   = l2_lrn.bf.non_std_sa_all_zero_learn  ;
    ctrl->non_std_sa_all_one_learn    = l2_lrn.bf.non_std_sa_all_one_learn   ;
    ctrl->non_std_sa_b40_is_one_learn = l2_lrn.bf.non_std_sa_b40_is_one_learn;
    ctrl->loop_pkt_no_learn           = l2_lrn.bf.loop_pkt_no_learn          ;
    ctrl->aging_out_time              = l2e_aging.bf.aging_out_time;
    ctrl->cnt_to_1s                   = l2e_aging.bf.cnt_to_1s;

    return CA_E_OK;
}

ca_status_t aal_fdb_aging_time_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t time
)
{
    L2FE_L2E_AGING_CTRL_t l2e_aging;

    CA_ASSERT_RET(time <= CA_AAL_MAX_MAC_AGING_TIME, CA_E_PARAM);

    __FDB_LOCK();

    l2e_aging.wrd = CA_NE_REG_READ(L2FE_L2E_AGING_CTRL);

    l2e_aging.bf.aging_out_time = time;
    L2_DEBUG("time:0x%x/%d, word:0x%08x\r\n", time, time, l2e_aging.wrd);
    FDB_REG_WRITE(L2FE_L2E_AGING_CTRL, l2e_aging.wrd);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    CA_8279_NE_REG_WRITE(l2e_aging.wrd, L2FE_L2E_AGING_CTRL);
#endif
    __FDB_UNLOCK();

#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    aal_fdb_ipc_aging_time_send(time);
#else
    sw_lrn_cb.aging_time = time;
#endif
    return CA_E_OK;
}


ca_status_t aal_fdb_aging_time_get (
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t *time
)
{
    L2FE_L2E_AGING_CTRL_t l2e_aging;

    L2_ASSERT_RET(time != NULL, CA_E_PARAM);

    __FDB_LOCK();

    l2e_aging.wrd = CA_NE_REG_READ(L2FE_L2E_AGING_CTRL);

    __FDB_UNLOCK();

    *time = l2e_aging.bf.aging_out_time;

    return CA_E_OK;
}

ca_status_t aal_fdb_glb_intr_enable_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_fdb_glb_intr_t  *config
)
{
    L2FE_GLB_INTERRUPTE_t l2fe_intr;

    AAL_ASSERT_RET(config, CA_E_PARAM);

    if (device_id == 0){
        l2fe_intr.wrd = CA_NE_REG_READ(L2FE_GLB_INTERRUPTE);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2fe_intr.wrd = CA_8279_NE_REG_READ(L2FE_GLB_INTERRUPTE);
    }
#endif
    l2fe_intr.bf.lf_overflow_intIE        = config->lf_overflow        ;
    l2fe_intr.bf.ple_hd_ff_overflow_intIE = config->ple_hd_ff_overflow ;
    l2fe_intr.bf.port_mac_lmt_excd_intIE  = config->port_mac_lmt_excd  ;
    l2fe_intr.bf.l2e_hash_collision_intIE = config->l2e_hash_collision ;

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_GLB_INTERRUPTE, l2fe_intr.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(l2fe_intr.wrd, L2FE_GLB_INTERRUPTE);
    }
#endif
    return CA_E_OK;

}

ca_status_t aal_fdb_glb_intr_enable_get(
    CA_IN  ca_device_id_t         device_id,
    CA_OUT aal_fdb_glb_intr_t *config
)
{
    L2FE_GLB_INTERRUPTE_t l2fe_intr;

    AAL_ASSERT_RET(config, CA_E_PARAM);

    if (device_id == 0){
        l2fe_intr.wrd = CA_NE_REG_READ(L2FE_GLB_INTERRUPTE);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2fe_intr.wrd = CA_8279_NE_REG_READ(L2FE_GLB_INTERRUPTE);
    }
#endif
    config->lf_overflow          = l2fe_intr.bf.lf_overflow_intIE       ;
    config->ple_hd_ff_overflow   = l2fe_intr.bf.ple_hd_ff_overflow_intIE;
    config->port_mac_lmt_excd    = l2fe_intr.bf.port_mac_lmt_excd_intIE ;
    config->l2e_hash_collision   = l2fe_intr.bf.l2e_hash_collision_intIE;

    return CA_E_OK;


}


ca_status_t aal_fdb_glb_intr_status_get(
    CA_IN  ca_device_id_t         device_id,
    CA_OUT aal_fdb_glb_intr_t *status
)
{
    L2FE_GLB_INTERRUPT_t l2fe_intr;

    AAL_ASSERT_RET(status, CA_E_PARAM);

    if (device_id == 0){
        l2fe_intr.wrd = CA_NE_REG_READ(L2FE_GLB_INTERRUPT);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2fe_intr.wrd = CA_8279_NE_REG_READ(L2FE_GLB_INTERRUPT);
    }
#endif
    status->lf_overflow          = l2fe_intr.bf.lf_overflow_intI       ;
    status->ple_hd_ff_overflow   = l2fe_intr.bf.ple_hd_ff_overflow_intI;
    status->port_mac_lmt_excd    = l2fe_intr.bf.port_mac_lmt_excd_intI ;
    status->l2e_hash_collision   = l2fe_intr.bf.l2e_hash_collision_intI;

    return CA_E_OK;
}

/** @aal_fdb_learn_status_get
 * Get the Learning update info FIFO status and
 * The number of the learning information entries cached in the
 * information FIFO
 */
ca_status_t aal_fdb_learn_status_get(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              port_id,
    CA_OUT aal_fdb_learn_status_t  *status)
{

    L2FE_L2E_INFO_FF_STS_REG_t  fifo_sts;
    L2FE_L2E_INFO_FF_CNT_0_t    cached_cnt;

    AAL_ASSERT_RET(port_id < 16, CA_E_PARAM);
    AAL_ASSERT_RET(status, CA_E_PARAM);

    if (device_id == 0){
        fifo_sts.wrd   = CA_NE_REG_READ(L2FE_L2E_INFO_FF_STS_REG);
        cached_cnt.wrd = CA_NE_REG_READ(L2FE_L2E_INFO_FF_CNT_0 + (port_id << 2));
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        fifo_sts.wrd   = CA_8279_NE_REG_READ(L2FE_L2E_INFO_FF_STS_REG);
        cached_cnt.wrd = CA_8279_NE_REG_READ(L2FE_L2E_INFO_FF_CNT_0 + (port_id << 2));
    }
#endif
    status->over_flow_flag   = (fifo_sts.bf.overflow_ind >> port_id) & 0x1;
    status->cached_entry_num = cached_cnt.bf.num;

    return CA_E_OK;

}

/* it need use indirect mode */
ca_status_t aal_fdb_get_by_hash_addr(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              hash_addr,
    CA_IN  ca_uint32_t              offset,
    CA_OUT aal_fdb_entry_cfg_t*     fdb_entry
)
{
    L2FE_L2E_FDB_ACCESS_t l2_fdb_access;
    L2FE_L2E_FDB_CMD_RETURN_t cmd_ret;
    L2FE_L2E_FDB_DATA0_t fdb_data0;
    L2FE_L2E_FDB_DATA1_t fdb_data1;
    L2FE_L2E_FDB_DATA2_t fdb_data2;
    L2FE_L2E_FDB_DATA3_t fdb_data3;
    ca_uint32 timeout = 0;

    AAL_ASSERT_RET(offset < 8, CA_E_PARAM);
    AAL_ASSERT_RET(hash_addr < 0x1ff, CA_E_PARAM);
    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    if (device_id == 0){
        l2_fdb_access.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        l2_fdb_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
    }
#endif

    if (l2_fdb_access.bf.access) {
        L2_DEBUG("HW BUSY, wrd:0x%08x\r\n", l2_fdb_access.wrd);

        return CA_E_BUSY;
    }


    l2_fdb_access.wrd = 0x80000000 | (hash_addr << 3) | offset;

    __FDB_LOCK();

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_FDB_CMD_RETURN, 0); /* init cmd return */
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(0, L2FE_L2E_FDB_CMD_RETURN); /* init cmd return */
    }
#endif
    L2_DEBUG("access wrd:0x%08x\r\n", l2_fdb_access.wrd);

    if (device_id == 0){
        FDB_REG_WRITE(L2FE_L2E_FDB_ACCESS, l2_fdb_access.wrd);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(l2_fdb_access.wrd, L2FE_L2E_FDB_ACCESS);
    }
#endif
    do {
        if (device_id == 0){
            l2_fdb_access.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            l2_fdb_access.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_ACCESS);
        }
#endif
        if (timeout++ >= CA_AAL_FDB_TBL_OP_TIMEOUT) {
            __FDB_UNLOCK();
            L2_DEBUG("timeout\r\n");
            return CA_E_TIMEOUT;
        }

    } while (l2_fdb_access.bf.access);

    /* get return code and feedback */
    if (device_id == 0){
        cmd_ret.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        cmd_ret.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
    }
#endif

    if (cmd_ret.bf.status == CA_AAL_FDB_ACCESS_HIT) {
        if (device_id == 0){
            fdb_data0.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_DATA0);
            fdb_data1.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_DATA1);
            fdb_data2.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_DATA2);
            fdb_data3.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_DATA3);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            fdb_data0.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA0);
            fdb_data1.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA1);
            fdb_data2.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA2);
            fdb_data3.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_DATA3);
        }
#endif
    //*WORD0 = cmd_ret.wrd;
    #if 0
        L2_DEBUG("output: data0:0x%08x\r\n", *DATA0);
        L2_DEBUG("        data1:0x%08x\r\n", *DATA1);
        L2_DEBUG("        data2:0x%08x\r\n", *DATA2);
        L2_DEBUG("        data3:0x%08x\r\n", *DATA3);
    #endif
    }


    __FDB_UNLOCK();

    L2_DEBUG("cmd_ret.wrd:0x%08x\r\n", cmd_ret.wrd);

    if (cmd_ret.bf.status == CA_AAL_FDB_ACCESS_HIT){
        __aal_fdb_data_2_entry(fdb_data1, fdb_data0, fdb_entry);
        __aal_fdb_data_2_mac(&fdb_data3, &fdb_data2, &fdb_data1, fdb_entry->mac);

        return CA_E_OK;
    }
    else
        return CA_E_NOT_FOUND;
}

ca_status_t aal_fdb_lrn_fwd_ctrl_set(
    CA_IN  ca_device_id_t       device_id,
    CA_IN  lrn_fwd_ctrl_mask_t fwd_ctrl_mask,
    CA_IN  lrn_fwd_ctrl_t    *fwd_ctrl
)
{
    L2FE_PLC_LRN_FWD_CTRL_1_t fwd_ctrl_1;
    L2FE_PLC_LRN_FWD_CTRL_0_t fwd_ctrl_0;

    AAL_ASSERT_RET(fwd_ctrl, CA_E_PARAM);

    __FDB_LOCK();

    if (device_id == 0){
        fwd_ctrl_1.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
        fwd_ctrl_0.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_0);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        fwd_ctrl_1.wrd = CA_8279_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
        fwd_ctrl_0.wrd = CA_8279_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_0);
    }
#endif
    if (fwd_ctrl_mask.bf.sa_hash_cllsn_fwd_ctrl)
        fwd_ctrl_1.bf.sa_hash_cllsn_fwd_ctrl = fwd_ctrl->hash_cllsn_mode;
    if (fwd_ctrl_mask.bf.sa_hash_cllsn_ldpid)
        fwd_ctrl_1.bf.sa_hash_cllsn_ldpid = fwd_ctrl->hash_cllsn_ldpid;

    if (fwd_ctrl_mask.bf.sa_limit_excd_fwd_ctrl)
        fwd_ctrl_1.bf.sa_limit_excd_fwd_ctrl = fwd_ctrl->limit_excd_mode;
    if (fwd_ctrl_mask.bf.sa_limit_excd_ldpid)
        fwd_ctrl_1.bf.sa_limit_excd_ldpid = fwd_ctrl->limit_excd_ldpid;

    if (fwd_ctrl_mask.bf.l2_mac_sw_lrn_ldpid)
        fwd_ctrl_0.bf.l2_mac_sw_lrn_ldpid = fwd_ctrl->l2_mac_sw_lrn_ldpid;

    if (fwd_ctrl_mask.bf.l2_mac_sw_lrn_cos)
        fwd_ctrl_0.bf.l2_mac_sw_lrn_cos = fwd_ctrl->l2_mac_sw_lrn_cos;
    if (fwd_ctrl_mask.bf.l2_mac_sw_lrn_keep_old_wan)
        fwd_ctrl_0.bf.l2_mac_sw_lrn_keep_old_wan = fwd_ctrl->l2_mac_sw_lrn_keep_old_wan;
    if (fwd_ctrl_mask.bf.l2_mac_sw_lrn_keep_old_lan)
        fwd_ctrl_0.bf.l2_mac_sw_lrn_keep_old_lan = fwd_ctrl->l2_mac_sw_lrn_keep_old_lan;
    if (fwd_ctrl_mask.bf.sa_miss_fwd)
        fwd_ctrl_0.bf.sa_miss_fwd = fwd_ctrl->sa_miss_fwd;
    if (fwd_ctrl_mask.bf.nonstd_sa_all_zero_fwd)
        fwd_ctrl_0.bf.nonstd_sa_all_zero_fwd = fwd_ctrl->nonstd_sa_all_zero_fwd;
    if (fwd_ctrl_mask.bf.nonstd_sa_all_one_fwd)
        fwd_ctrl_0.bf.nonstd_sa_all_one_fwd = fwd_ctrl->nonstd_sa_all_one_fwd;
    if (fwd_ctrl_mask.bf.nonstd_sa_b40_is_one_fwd)
        fwd_ctrl_0.bf.nonstd_sa_b40_is_one_fwd = fwd_ctrl->nonstd_sa_b40_is_one_fwd;
    if (fwd_ctrl_mask.bf.sm_static_entry_fwd)
        fwd_ctrl_0.bf.sm_static_entry_fwd = fwd_ctrl->sm_static_entry_fwd;
    if (fwd_ctrl_mask.bf.sm_tmp_entry_fwd)
        fwd_ctrl_0.bf.sm_tmp_entry_fwd = fwd_ctrl->sm_tmp_entry_fwd;
    if (fwd_ctrl_mask.bf.sm_sm_entry_fwd)
        fwd_ctrl_0.bf.sm_sm_entry_fwd = fwd_ctrl->sm_sm_entry_fwd;

    if (fwd_ctrl_mask.bf.sa_limit_excd_cos)
        fwd_ctrl_1.bf.sa_limit_excd_cos = fwd_ctrl->sa_limit_excd_cos;
    if (fwd_ctrl_mask.bf.sa_limit_excd_keep_old_wan)
        fwd_ctrl_1.bf.sa_limit_excd_keep_old_wan = fwd_ctrl->sa_limit_excd_keep_old_wan;
    if (fwd_ctrl_mask.bf.sa_limit_excd_keep_old_lan)
        fwd_ctrl_1.bf.sa_limit_excd_keep_old_lan = fwd_ctrl->sa_limit_excd_keep_old_lan;
    if (fwd_ctrl_mask.bf.sa_hash_cllsn_cos)
        fwd_ctrl_1.bf.sa_hash_cllsn_cos = fwd_ctrl->sa_hash_cllsn_cos;
    if (fwd_ctrl_mask.bf.sa_hash_cllsn_keep_old_wan)
        fwd_ctrl_1.bf.sa_hash_cllsn_keep_old_wan = fwd_ctrl->sa_hash_cllsn_keep_old_wan;
    if (fwd_ctrl_mask.bf.sa_hash_cllsn_keep_old_lan)
        fwd_ctrl_1.bf.sa_hash_cllsn_keep_old_lan = fwd_ctrl->sa_hash_cllsn_keep_old_lan;
    if (fwd_ctrl_mask.bf.swlrn_othr_pkt_fwd)
        fwd_ctrl_1.bf.swlrn_othr_pkt_fwd = fwd_ctrl->swlrn_othr_pkt_fwd;


    if (device_id == 0){
        CA_NE_REG_WRITE(fwd_ctrl_1.wrd, L2FE_PLC_LRN_FWD_CTRL_1);
        CA_NE_REG_WRITE(fwd_ctrl_0.wrd, L2FE_PLC_LRN_FWD_CTRL_0);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        CA_8279_NE_REG_WRITE(fwd_ctrl_1.wrd, L2FE_PLC_LRN_FWD_CTRL_1);
        CA_8279_NE_REG_WRITE(fwd_ctrl_0.wrd, L2FE_PLC_LRN_FWD_CTRL_0);
    }
#endif

    __FDB_UNLOCK();

    L2_DEBUG("hash mode:%d, ldpid:%d;  limit excd: mode:%d, ldpid:%d, sw lrn ldpid:%d\r\n",
            fwd_ctrl->hash_cllsn_mode, fwd_ctrl->hash_cllsn_ldpid,
            fwd_ctrl->limit_excd_mode, fwd_ctrl->limit_excd_ldpid, fwd_ctrl->l2_mac_sw_lrn_ldpid);

    return CA_E_OK;
}

ca_status_t aal_fdb_lrn_fwd_ctrl_get(
    CA_IN  ca_device_id_t       device_id,
    CA_OUT lrn_fwd_ctrl_t    *fwd_ctrl
)
{
    L2FE_PLC_LRN_FWD_CTRL_1_t fwd_ctrl_1;
    L2FE_PLC_LRN_FWD_CTRL_0_t fwd_ctrl_0;

    AAL_ASSERT_RET(fwd_ctrl, CA_E_PARAM);

    __FDB_LOCK();

    if (device_id == 0){
        fwd_ctrl_1.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
        fwd_ctrl_0.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_0);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        fwd_ctrl_1.wrd = CA_8279_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
        fwd_ctrl_0.wrd = CA_8279_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_0);
    }
#endif
    __FDB_UNLOCK();

    fwd_ctrl->hash_cllsn_mode = fwd_ctrl_1.bf.sa_hash_cllsn_fwd_ctrl;
    fwd_ctrl->hash_cllsn_ldpid =  fwd_ctrl_1.bf.sa_hash_cllsn_ldpid;
    fwd_ctrl->limit_excd_mode = fwd_ctrl_1.bf.sa_limit_excd_fwd_ctrl;
    fwd_ctrl->limit_excd_ldpid =  fwd_ctrl_1.bf.sa_limit_excd_ldpid;
    fwd_ctrl->l2_mac_sw_lrn_ldpid =  fwd_ctrl_0.bf.l2_mac_sw_lrn_ldpid;

    fwd_ctrl->l2_mac_sw_lrn_cos          = fwd_ctrl_0.bf.l2_mac_sw_lrn_cos;
    fwd_ctrl->l2_mac_sw_lrn_keep_old_wan = fwd_ctrl_0.bf.l2_mac_sw_lrn_keep_old_wan;
    fwd_ctrl->l2_mac_sw_lrn_keep_old_lan = fwd_ctrl_0.bf.l2_mac_sw_lrn_keep_old_lan;
    fwd_ctrl->sa_miss_fwd                = fwd_ctrl_0.bf.sa_miss_fwd;
    fwd_ctrl->nonstd_sa_all_zero_fwd     = fwd_ctrl_0.bf.nonstd_sa_all_zero_fwd;
    fwd_ctrl->nonstd_sa_all_one_fwd      = fwd_ctrl_0.bf.nonstd_sa_all_one_fwd;
    fwd_ctrl->nonstd_sa_b40_is_one_fwd   = fwd_ctrl_0.bf.nonstd_sa_b40_is_one_fwd;
    fwd_ctrl->sm_static_entry_fwd        = fwd_ctrl_0.bf.sm_static_entry_fwd;
    fwd_ctrl->sm_tmp_entry_fwd           = fwd_ctrl_0.bf.sm_tmp_entry_fwd;
    fwd_ctrl->sm_sm_entry_fwd            = fwd_ctrl_0.bf.sm_sm_entry_fwd;
    fwd_ctrl->sa_limit_excd_cos          = fwd_ctrl_1.bf.sa_limit_excd_cos;
    fwd_ctrl->sa_limit_excd_keep_old_wan = fwd_ctrl_1.bf.sa_limit_excd_keep_old_wan;
    fwd_ctrl->sa_limit_excd_keep_old_lan = fwd_ctrl_1.bf.sa_limit_excd_keep_old_lan;
    fwd_ctrl->sa_hash_cllsn_cos          = fwd_ctrl_1.bf.sa_hash_cllsn_cos;
    fwd_ctrl->sa_hash_cllsn_keep_old_wan = fwd_ctrl_1.bf.sa_hash_cllsn_keep_old_wan;
    fwd_ctrl->sa_hash_cllsn_keep_old_lan = fwd_ctrl_1.bf.sa_hash_cllsn_keep_old_lan;
    fwd_ctrl->swlrn_othr_pkt_fwd         = fwd_ctrl_1.bf.swlrn_othr_pkt_fwd;


    L2_DEBUG("mode:%d, ldpid:%d;  limit excd:%d/%d, sw lrn ldpid:%d\r\n", fwd_ctrl->hash_cllsn_mode, fwd_ctrl->hash_cllsn_ldpid,
                    fwd_ctrl->limit_excd_mode, fwd_ctrl->limit_excd_ldpid, fwd_ctrl_0.bf.l2_mac_sw_lrn_ldpid);

    return CA_E_OK;
}

/*
Hash handling collision
1. Replace the oldest FDB entry in bucket with specified FDB entry
    Put FDB parameters in L2FE_L2E_FDB_DATA0-3
    L2FE_L2E_FDB_ACCESS[5] = 1 (delete oldest entry)
    Put FDB parameters in L2FE_L2E_FDB_DATA0-3
    L2FE_L2E_FDB_ACCESS[2:0] = 3'b101 (update FDB)
    L2FE_L2E_FDB_ACCESS[7:6] = 2'b01 (append)

*/
ca_status_t aal_fdb_hash_collision(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  aal_fdb_entry_cfg_t      *fdb_entry
)
{
    ca_uint32_t full_ctrl = 0;
    aal_l2_vlan_default_cfg_t vlan_cfg;
    ca_status_t ret = CA_E_OK;

    L2_DEBUG("Enter\r\n");
    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    aal_fdb_limit_excd_fwd_ctrl_get(0, &full_ctrl);
    if (full_ctrl == SA_LRN_FWD_CTRL_DROP){
        L2_DEBUG("limit_excd_fwd_ctrl is drop\r\n");
        return CA_E_OK;
    }

    memset(&vlan_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
    ret = aal_l2_vlan_default_cfg_get(device_id, &vlan_cfg);

    if (vlan_cfg.vlan_mode == AAL_L2_VLAN_MODE_SVL){
        fdb_entry->key_sc_ind = 0;
        fdb_entry->key_vid = 0;
    }

    ret = aal_fdb_entry_add_replace(device_id, fdb_entry);

    /* send event out */
    ret = ca_event_send(device_id, CA_EVENT_L2_FDB_HASH_COLLISION, (void *)fdb_entry, sizeof(aal_fdb_entry_cfg_t));

    return CA_E_OK;
}
#if 1
ca_status_t aal_fdb_delete_oldest_entry_by_port(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_uint32_t              lpid
)
{
    aal_fdb_entry_cfg_t  fdb_entry, oldest_entry;
    ca_uint32_t hit_addr = 0, start_addr = 0;
    ca_uint32_t cmd_ret = 0;
    ca_uint32_t item_cnt = 0;
    ca_int32_t first_entry = 0;
    ca_status_t ret = CA_E_OK;
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    aal_fdb_flush_cfg_t          cfg;

    memset((void *)&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));
    memset((void *)&oldest_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    oldest_entry.aging_sts = 0xff;

    /* load cfg */
    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = ~0U;

    /* only join compare lpid */
    mask.bf.lpid = 0;
    cfg.lpid_high = lpid;
    cfg.lpid_low  = lpid;

    /* static flag join comparion, is 0, dynamic */
    mask.bf.static_flag = 0;
    cfg.static_flag     = 0;

    mask.bf.mc_flag = 0;
    cfg.mc_flag = 0;

    aal_fdb_scan_flush_field_set(device_id, mask, &cfg);

    L2_DEBUG("please make sure call function aal_fdb_scan_flush_field_set to set word0--word4\r\n");

    while (1) {
        ret = aal_fdb_entry_scan(device_id,
                                start_addr, &hit_addr,
                                &fdb_entry, &cmd_ret);

        if (ret != CA_E_OK || cmd_ret != CA_AAL_FDB_ACCESS_HIT) {
            L2_DEBUG("search end\r\n");
            break;
        }

        L2_DEBUG("start_addr:%d, hit_addr:%d, ret:%d\r\n", start_addr, hit_addr, ret);
        L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d, valid:%d, tmp:%d\r\n",
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id,
                fdb_entry.valid,
                fdb_entry.tmp_flag);

        if (start_addr == hit_addr + 1){
            L2_DEBUG("start_addr == hit_addr + 1\r\n");
            break;
        }

        /* make at least one entry meet */
        if (first_entry == 0){
            first_entry = 1;
            oldest_entry = fdb_entry;
        }

        if (fdb_entry.aging_sts < oldest_entry.aging_sts)
            oldest_entry = fdb_entry;

        start_addr = hit_addr + 1;
        ++item_cnt;
    }

    L2_DEBUG("## delete mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d, valid:%d, tmp:%d\r\n",
            oldest_entry.mac[0], oldest_entry.mac[1], oldest_entry.mac[2], oldest_entry.mac[3], oldest_entry.mac[4], oldest_entry.mac[5],
            oldest_entry.key_vid    ,
            oldest_entry.key_sc_ind ,
            oldest_entry.dot1p      ,
            oldest_entry.lrnt_vid   ,
            oldest_entry.sa_permit  ,
            oldest_entry.da_permit  ,
            oldest_entry.static_flag,
            oldest_entry.aging_sts  ,
            oldest_entry.mc_flag    ,
            oldest_entry.port_id,
            oldest_entry.valid,
            oldest_entry.tmp_flag);

    ret = aal_fdb_entry_del(device_id, &oldest_entry);

    L2_DEBUG("ret:%d, lpid:0x%x\r\n", ret, lpid);

    if (ret != CA_E_OK){
        L2_DEBUG("flush oldest base port fail\r\n");
        return ret;
    }

    return CA_E_OK;
}

#else
/*
 * use the hardware flush oldest entry command, it doesn't work
 */
ca_status_t aal_fdb_delete_oldest_entry_by_port(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              lpid
)
{
    aal_fdb_cpu_cmd_word4_msk_t  flush_mask;
    aal_fdb_flush_cfg_t          flush_cfg;
    ca_status_t ret = CA_E_OK;

    flush_mask.wrd = ~0U;

    /* static flag join comparion, is 0, dynamic */
    flush_mask.bf.static_flag = 0;
    flush_cfg.static_flag     = 0;


    flush_mask.bf.lpid  = 0;
    flush_cfg.lpid_high = lpid;
    flush_cfg.lpid_low  = lpid;

    ret = aal_fdb_scan_flush_field_set(device_id, flush_mask, &flush_cfg);

    if (CA_E_OK == ret)
        ret = aal_fdb_entry_flush_cmd(device_id, __FDB_TBL_OPCODE_FLUSH_OLDEST);

    L2_DEBUG("ret:%d, lpid:0x%x\r\n", ret, lpid);

    if (ret != CA_E_OK){
        L2_DEBUG("flush oldest base port fail\r\n");
        return ret;
    }

    return CA_E_OK;
}
#endif
/*
AC limit handling
1. Delete the oldest FDB entry for specified LPID
    L2FE_L2E_FDB_CPU_CMD_WORD0.[6:0]= lpid range comparison low end
    L2FE_L2E_FDB_CPU_CMD_WORD1.[5:0]= lpid range comparison high end
    L2FE_L2E_FDB_CPU_CMD_WORD4.[0] = lpid range compare mask.
    L2FE_L2E_FDB_ACCESS[2:0] = 3'b010 (update FDB)
    L2FE_L2E_FDB_ACCESS[7:6] = 2'b01 (flush)
    L2FE_L2E_FDB_ACCESS[5] = 1 (delete oldest entry)

2. Add new FDB entry
    Put FDB parameters in L2FE_L2E_FDB_DATA0-3
    L2FE_L2E_FDB_ACCESS[2:0] = 3'b101 (update FDB)
    L2FE_L2E_FDB_ACCESS[7:6] = 2'b01 (append)
    L2FE_L2E_FDB_ACCESS[5] = 1 (delete oldest entry, in case of collision)

*/
ca_status_t aal_fdb_mac_limit_exceed(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  ca_uint32_t              lpid,
    CA_IN  aal_fdb_entry_cfg_t      *fdb_entry
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t full_ctrl;
    aal_l2_vlan_default_cfg_t vlan_cfg;

    L2_DEBUG("Enter\r\n");
    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    aal_fdb_limit_excd_fwd_ctrl_get(0, &full_ctrl);
    if (full_ctrl == SA_LRN_FWD_CTRL_DROP){
        L2_DEBUG("limit_excd_fwd_ctrl is drop\r\n");
        return CA_E_OK;
    }

    memset(&vlan_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));
    ret = aal_l2_vlan_default_cfg_get(device_id, &vlan_cfg);

    if (vlan_cfg.vlan_mode == AAL_L2_VLAN_MODE_SVL){
        fdb_entry->key_sc_ind = 0;
        fdb_entry->key_vid = 0;
    }

    ret = aal_fdb_delete_oldest_entry_by_port(device_id, lpid);

    if (ret != CA_E_OK){
        L2_DEBUG("flush oldest base port fail\r\n");
        return ret;
    }

    ret = aal_fdb_entry_add_replace(device_id, fdb_entry);

    if (ret != CA_E_OK){
        L2_DEBUG("add entry fail\r\n");
        return ret;
    }

    /* send event out */
    ret = ca_event_send(device_id, CA_EVENT_L2_FDB_LEARN_LIMIT, (void *)fdb_entry, sizeof(aal_fdb_entry_cfg_t));

    return CA_E_OK;
}

/*
3. SW learning
1. Add new FDB entry
    Put FDB parameters in L2FE_L2E_FDB_DATA0-3
    L2FE_L2E_FDB_ACCESS[2:0] = 3'b101 (update FDB)
    L2FE_L2E_FDB_ACCESS[7:6] = 2'b01 (append)

2. Check return code for add failure
    L2FE_L2E_FDB_CMD_RETURN.status[3:0] = 4'b1011 (entry updated; However port MAC limit check failure detected)

3. In case of MAC limit failure, follow MAC limit handling to delete one entry (no need to add again)

*/
ca_status_t aal_fdb_sw_learning(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  aal_fdb_entry_cfg_t      *fdb_entry
)
{
    ca_status_t ret = CA_E_OK;
    L2FE_L2E_FDB_CMD_RETURN_t cmd_ret;

    L2_DEBUG("Enter\r\n");
    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    /*
    if (g_vlan_mode == AAL_L2_VLAN_MODE_SVL){
        fdb_entry->key_sc_ind = 0;
        fdb_entry->key_vid = 0;
    }
    */

    if (sw_lrn_cb.cfg){
#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
    /* Do Nothing, packet receive in PE */
    ;;
#else
        /* static mac */
        ret = __aal_fdb_static_mac_proc(device_id, fdb_entry);

        /* mac limit*/
        if (ret != CA_E_OK)
            ret = __aal_fdb_sw_lrn_proc(device_id, fdb_entry);
#endif
    }
    else{
        ret = aal_fdb_entry_add(device_id, fdb_entry);

        if (device_id == 0){
            cmd_ret.wrd = CA_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
        }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        else if (device_id == 1){
            cmd_ret.wrd = CA_8279_NE_REG_READ(L2FE_L2E_FDB_CMD_RETURN);
        }
#endif
        if (cmd_ret.bf.status == CA_AAL_FDB_ACCESS_MAC_LIMIT){
            /* mac limit check failure */
            L2_DEBUG("mac limit check failure\r\n");
            ret = aal_fdb_delete_oldest_entry_by_port(device_id, fdb_entry->port_id);
        }

        L2_DEBUG("ret:%d, port_id:%d\r\n", ret, fdb_entry->port_id);
    }
    return ret;
}


/*
cpu_special_packet_learn_enable
To CPU packet learning enable/disable: this is supported in G3 HW (using special packet learning control)
L2FE_PLA_SPB_DATA. spb_learn_dis (in most cases, to CPU packets are trapped and forwarded by special packet HW module)
 */

ca_status_t aal_fdb_cpu_pkt_lrn_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t flag
)
{
    ca_status_t                 ret = CA_E_OK;
    ca_uint32_t                 spb_idx = 0;
    aal_l2_specpkt_type_t       pkt_type;
    aal_l2_specpkt_ctrl_mask_t  l2_ctrl_mask;
    aal_l2_specpkt_ctrl_t       l2_ctrl;

    l2_ctrl_mask.u32 = 0;

    l2_ctrl_mask.s.learn_dis = 1;

    l2_ctrl.learn_dis = !flag;

    for (spb_idx = 0; spb_idx <= AAL_L2_SPECPKT_TBL_IDX_MAX; spb_idx++) {
        for (pkt_type = AAL_L2_SPECPKT_TYPE_NORMAL; pkt_type <= AAL_L2_SPECPKT_TYPE_MAX; pkt_type++){
            ret = aal_l2_specpkt_ctrl_set(device_id, spb_idx,
                pkt_type,
                l2_ctrl_mask, &l2_ctrl);

            if (CA_E_OK != ret) {
                L2_DEBUG("Invoke aal_l2_specpkt_ctrl_set return %#x.\n", ret);
                return ret;
            }
        }
    }
    L2_DEBUG("ret:%d, flag:%d\r\n", ret, flag);

    return ret;

}


ca_status_t aal_fdb_cpu_pkt_lrn_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_boolean_t *flag
)
{
    ca_status_t                 ret = CA_E_OK;
    aal_l2_specpkt_ctrl_t       l2_pkt_cfg;

    AAL_ASSERT_RET(flag, CA_E_PARAM);

    memset(&l2_pkt_cfg, 0, sizeof(l2_pkt_cfg));
    ret = aal_l2_specpkt_ctrl_get(device_id,
        0, 0, &l2_pkt_cfg);

    *flag = !l2_pkt_cfg.learn_dis;

    L2_DEBUG("ret:%d, flag:%d\r\n", ret, *flag);

    return ret;

}

ca_status_t aal_fdb_limit_excd_fwd_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *lrn_ctrl
)
{
    ca_status_t                 ret = CA_E_OK;
    L2FE_PLC_LRN_FWD_CTRL_1_t   plc_lrn_fwd_ctrl;

    AAL_ASSERT_RET(lrn_ctrl, CA_E_PARAM);

    if (device_id == 0){
        plc_lrn_fwd_ctrl.wrd = CA_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
    }
#ifdef CONFIG_ARCH_CORTINA_G3HGU
    else if (device_id == 1){
        plc_lrn_fwd_ctrl.wrd = CA_8279_NE_REG_READ(L2FE_PLC_LRN_FWD_CTRL_1);
    }
#endif
    *lrn_ctrl = plc_lrn_fwd_ctrl.bf.sa_limit_excd_fwd_ctrl;
    L2_DEBUG("ret:%d, lrn_ctrl:%d\r\n", ret, *lrn_ctrl);

    return ret;
}
ca_status_t aal_fdb_addr_delete_all(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t flag
)
{
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    aal_fdb_flush_cfg_t cfg;
    ca_status_t ret;

    mask.wrd = 0xffffffff;
    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    if (flag == CA_L2_ADDR_OPERATION_STATIC){
        mask.bf.static_flag = 0;  /* join */
        cfg.static_flag = 1;
    }
    else if (flag == CA_L2_ADDR_OPERATION_DYNAMIC){
        mask.bf.static_flag = 0;
        cfg.static_flag = 0;
    }
    else if (flag == CA_L2_ADDR_OPERATION_BOTH){
    /* flush all, include static */
        mask.bf.static_flag = 1;
        cfg.static_flag = 0;
    }

    mask.bf.mc_flag = 0;
    cfg.mc_flag = 0;

    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    __aal_fdb_flush_type(SW_LRN_MAC_ALL);

    ret = aal_fdb_entry_flush(0);

    if (ret == CA_E_OK && (flag == CA_L2_ADDR_OPERATION_STATIC || flag == CA_L2_ADDR_OPERATION_BOTH))
        ret = aal_fdb_tbl_del_all();

    L2_DEBUG("ret:%d\r\n", ret);

    return ret;
}


#if 0
ca_status_t aal_fdb_hash_cllsn_update(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  aal_fdb_entry_cfg_t      *fdb_entry
)
{
    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);
    return aal_fdb_entry_add(device_id, fdb_entry);
}
//#else
ca_status_t aal_fdb_hash_cllsn_update(
    CA_IN  ca_device_id_t              device_id,
    CA_IN  aal_fdb_entry_cfg_t      *fdb_entry
)
{
    L2FE_L2E_HASH_CLLSN_STTS_t hash_cllsn_stts;
    ca_uint32_t             hash_addr;
    ca_uint32_t             oldest = (ca_uint32_t)-1;
    aal_fdb_entry_cfg_t     fdb_entry, oldest_entry;
    int i;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(fdb_entry, CA_E_PARAM);

    hash_cllsn_stts.wrd =  CA_NE_REG_READ(L2FE_L2E_HASH_CLLSN_STTS);

    if (!hash_cllsn_stts.bf.cllsn_det){

        ca_printf("no collision detected!\r\n");
        return CA_E_OK;
    }

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));
    memset(&oldest_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    hash_addr = hash_cllsn_stts.bf.cllsn_addr;

    /* change l2e cpu to indirect mode*/
    __aal_l2e_cpu_op_mode(CA_AAL_L2E_INDIRECT_MODE);

    for (i = 0; i < 8; i++){
        ret = aal_fdb_get_by_hash_addr(device_id, hash_addr, i, &fdb_entry);
        if (ret != CA_E_OK){
            ca_printf("hash entry not full!\r\n");
            break;
        }

        L2_DEBUG("i:%d, mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa_permit:%d, da_permit:%d, static_flag:%d, aging_sts:%d, mc:%d, pid:%d\r\n",
                    i + 1,
                    fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2],
                    fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                    fdb_entry.key_vid    ,
                    fdb_entry.key_sc_ind ,
                    fdb_entry.dot1p      ,
                    fdb_entry.lrnt_vid   ,
                    fdb_entry.sa_permit  ,
                    fdb_entry.da_permit  ,
                    fdb_entry.static_flag,
                    fdb_entry.aging_sts  ,
                    fdb_entry.mc_flag    ,
                    fdb_entry.port_id);

        if (fdb_entry.aging_sts < oldest){ /* find minimum aging entry */
            memcpy(&oldest_entry, &fdb_entry, sizeof(aal_fdb_entry_cfg_t));
            oldest = fdb_entry.aging_sts;
        }
    }

    __aal_l2e_cpu_op_mode(CA_AAL_L2E_CMD_MODE);

    if (ret == CA_E_OK){
        aal_fdb_entry_del(device_id, &oldest_entry);
    }

    /* TODO, add new entry */

    return ret;
}
#endif

#if 1
/* aal cli debug purpose */
ca_mac_addr_t    dbg_mac = {0x00, 0xbe, 0xef, 0x00, 0x00, 0x01};
ca_uint32_t dbg_mac_cnt = 0;
ca_uint32_t dbg_vid = 10;

ca_status_t aal_fdb_dbg_add_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t             port_id,
    CA_IN ca_uint32_t             static_flag
)
{
    ca_status_t ret;
    aal_fdb_entry_cfg_t fdb_entry;


    memset((void *)&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));
    dbg_mac[4] += dbg_mac_cnt / 256;
    dbg_mac[5] += dbg_mac_cnt % 256;
    dbg_mac_cnt++;

    memcpy(fdb_entry.mac, dbg_mac, sizeof(ca_mac_addr_t));
    fdb_entry.key_vid = dbg_vid;
    fdb_entry.key_sc_ind = 1;
    fdb_entry.lrnt_vid = dbg_vid;
    fdb_entry.lrnt_sc_ind = 1;
    fdb_entry.dot1p = 5;
    fdb_entry.static_flag = static_flag;
    fdb_entry.sa_permit = 1;
    fdb_entry.da_permit = 1;
    fdb_entry.port_id = port_id;
    fdb_entry.valid = 1;

    fdb_entry.tmp_flag = 0;
    fdb_entry.aging_sts = 255;
    fdb_entry.mc_flag = 0;
    fdb_entry.aged_flag = 0;

    L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, vid:%d, static:%d\r\n", dbg_mac[0], dbg_mac[1], dbg_mac[2], dbg_mac[3], dbg_mac[4], dbg_mac[5],
            dbg_vid, static_flag);

    ret = aal_fdb_entry_add(device_id, &fdb_entry);

    L2_DEBUG("ret:%d\r\n", ret);
    L2_DEBUG("key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa_permit:%d, da_permit:%d, static_flag:%d, aging_sts:%d, mc:%d, pid:%d\r\n",
            fdb_entry.key_vid    ,
            fdb_entry.key_sc_ind ,
            fdb_entry.dot1p      ,
            fdb_entry.lrnt_vid   ,
            fdb_entry.sa_permit  ,
            fdb_entry.da_permit  ,
            fdb_entry.static_flag,
            fdb_entry.aging_sts  ,
            fdb_entry.mc_flag    ,
            fdb_entry.port_id);

    return CA_E_OK;
}

ca_status_t aal_fdb_entry_addr_get(
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_mac_addr_t          mac,
    CA_IN ca_uint16_t            key_vid
)
{
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_status_t ret = CA_E_OK;

    memset((void *)&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    L2_DEBUG("vid:%d, mac:0x%x:%x:%x:%x:%x:%x\r\n", key_vid, mac[0], mac[1], mac[2],
                                mac[3], mac[4], mac[5]);

    ret = aal_fdb_entry_get(0, mac, key_vid, &fdb_entry);

    L2_DEBUG("ret:%d\r\n", ret);
    if (ret == CA_E_OK){
        L2_DEBUG("key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa_permit:%d, da_permit:%d, static_flag:%d, aging_sts:%d, mc:%d, pid:%d\r\n",
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id);
    }

    return ret;
}
ca_status_t aal_fdb_entry_addr_set (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_mac_addr_t           mac,
    CA_IN ca_uint16_t             key_vid,
    CA_IN aal_fdb_entry_cfg_t *fdb_entry
)
{
    ca_status_t ret;
    /*
    aal_fdb_entry_cfg_t fdb_entry;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    memcpy(fdb_entry.mac, addr->mac_addr, sizeof(ca_mac_addr_t));
    fdb_entry.key_vid = addr->key_vid;
    fdb_entry.lrnt_vid = addr->vid;
    fdb_entry.dot1p = addr->dot1p;
    fdb_entry.static_flag = addr->static_flag;
    fdb_entry.sa_permit = addr->sa_permit;
    fdb_entry.da_permit = addr->da_permit;
    fdb_entry.port_id = CA_PORT_INDEX_GET(addr->port_id);
    //fdb_entry. = addr->mc_group_id;
    fdb_entry.aging_sts = addr->aging_status;
    */
    ret = aal_fdb_entry_add(0, fdb_entry);

    L2_DEBUG("ret:%d\r\n", ret);
    L2_DEBUG("key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa_permit:%d, da_permit:%d, static_flag:%d, aging_sts:%d, mc:%d, pid:%d\r\n",
            fdb_entry->key_vid    ,
            fdb_entry->key_sc_ind ,
            fdb_entry->dot1p      ,
            fdb_entry->lrnt_vid   ,
            fdb_entry->sa_permit  ,
            fdb_entry->da_permit  ,
            fdb_entry->static_flag,
            fdb_entry->aging_sts  ,
            fdb_entry->mc_flag    ,
            fdb_entry->port_id);

    return CA_E_OK;
}

ca_status_t aal_fdb_addr_delete_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN aal_fdb_entry_cfg_t *fdb_entry
)
{
    ca_status_t ret;
    /*
    aal_fdb_entry_cfg_t fdb_entry;

    L2_ASSERT_RET(addr != NULL, CA_E_PARAM);

    memset(&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    memcpy(fdb_entry.mac, addr->mac_addr, sizeof(ca_mac_addr_t));
    fdb_entry.key_vid = addr->key_vid;
    fdb_entry.lrnt_vid = addr->vid;
    fdb_entry.dot1p = addr->dot1p;
    fdb_entry.static_flag = addr->static_flag;
    fdb_entry.sa_permit = addr->sa_permit;
    fdb_entry.da_permit = addr->da_permit;
    fdb_entry.port_id = CA_PORT_INDEX_GET(addr->port_id);
    //fdb_entry. = addr->mc_group_id;
    fdb_entry.aging_sts = addr->aging_status;
    */
    ret = aal_fdb_entry_del(0, fdb_entry);

    L2_DEBUG("ret:%d\r\n", ret);
    L2_DEBUG("key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa_permit:%d, da_permit:%d, static_flag:%d, aging_sts:%d, mc:%d, pid:%d\r\n",
            fdb_entry->key_vid    ,
            fdb_entry->key_sc_ind ,
            fdb_entry->dot1p      ,
            fdb_entry->lrnt_vid   ,
            fdb_entry->sa_permit  ,
            fdb_entry->da_permit  ,
            fdb_entry->static_flag,
            fdb_entry->aging_sts  ,
            fdb_entry->mc_flag    ,
            fdb_entry->port_id);

    return CA_E_OK;
}


ca_status_t aal_fdb_addr_delete_all_get (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t flag
)
{
    return aal_fdb_addr_delete_all(device_id, flag);
}

#define __FDB_IS_VALID(fdb) ((fdb).valid && ((fdb).aging_sts > 0))


#define AAL_FDB_FLAG_DYNAMIC CA_L2_ADDR_OPERATION_DYNAMIC
#define AAL_FDB_FLAG_STATIC  CA_L2_ADDR_OPERATION_STATIC
#define AAL_FDB_FLAG_BOTH    CA_L2_ADDR_OPERATION_BOTH

ca_status_t aal_fdb_table_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t             in_flag
)
{
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_uint32_t next_addr = 0, start_addr = 0;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t item_cnt = 0;
    ca_uint32_t find_cnt = 0;
    ca_uint32_t formal_cnt = 0, tmp_cnt = 0;
    aal_fdb_flush_cfg_t cfg;
    aal_fdb_cpu_cmd_word4_msk_t  mask;
    ca_uint32_t no_print = 0;
    ca_uint32_t flag = (in_flag & 0xff);

    no_print = ((in_flag & 0xff00) >> 8);

    if (!no_print)
        ca_printf("%d: dynamic; %d: static; %d:both\r\n", AAL_FDB_FLAG_DYNAMIC, CA_L2_ADDR_OPERATION_STATIC, CA_L2_ADDR_OPERATION_BOTH);

    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = 0xffffffff;
    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    memset((void *)&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));

    while (item_cnt++ < AAL_FDB_MAX_NUM) {

        ret = aal_fdb_scan(device_id,
                start_addr, &next_addr,
                &fdb_entry);

        if (ret != CA_E_OK ) {
            L2_DEBUG("search end\r\n");
            break;
        }

        if (__FDB_IS_VALID(fdb_entry)) {

            if ((flag == AAL_FDB_FLAG_DYNAMIC && !fdb_entry.static_flag) ||
                (flag == AAL_FDB_FLAG_STATIC && fdb_entry.static_flag) ||
                (flag == AAL_FDB_FLAG_BOTH)) {

            if (0 == find_cnt) {
                    if (!no_print){
                ca_printf("    lrn-vid  key-vid                  static  mc  tmp    SA     DA             \n");
                ca_printf("port (s/c)   (s/c)          MAC        flag  flag flag  permit permit age addr\n");
                ca_printf("---- ------- ------- ----------------- ----- ---- ----- ------ ------  --- -----\n");
            }
                }

            find_cnt++;
                if (!no_print)
                ca_printf("%-4u %4u(%-1s) %4u(%-1s) %02x:%02x:%02x:%02x:%02x:%02x %-5u  %-3u %-6u  %-4u   %-4u  %-3u %u\n",
                    fdb_entry.port_id,
                    fdb_entry.lrnt_vid,
                    fdb_entry.lrnt_sc_ind ? "s" : "c",
                    fdb_entry.key_vid ,
                    fdb_entry.key_sc_ind ? "s" : "c",
                    fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                    fdb_entry.static_flag,
                    fdb_entry.mc_flag,
                    fdb_entry.tmp_flag,
                    fdb_entry.sa_permit  ,
                    fdb_entry.da_permit  ,
                    FDB_AGING_TIME(fdb_entry.aging_sts),
                    next_addr - 1
                    );

                if (!fdb_entry.tmp_flag) formal_cnt++;
                else tmp_cnt++;

                if (0 == (find_cnt % 15)) {
                    if (!no_print)
                    ca_sleep(1);
                }

            }
        }

        if (start_addr == next_addr){
            L2_DEBUG("start_addr == next_addr, LAST valid entry\r\n");
            break;
        }

        start_addr = next_addr;

    }
    if (!no_print)
        ca_printf("\r\nTotal FDB count : %u, formal : %d\n", find_cnt, formal_cnt);

    L2_DEBUG("no_print: %d, tmp_cnt: %d\r\n", no_print, tmp_cnt);

    if (no_print)
        return tmp_cnt;

    return CA_E_OK;

}
ca_status_t aal_fdb_addr_traverse_get (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t             static_flag
)
{
    aal_fdb_entry_cfg_t  fdb_entry;
    ca_l2_addr_entry_t addr;
    ca_uint32 hit_addr = 0, start_addr = 0;
    ca_uint32 ret = 0, cmd_ret = 0;
    ca_uint32 item_cnt = 0;
    aal_fdb_flush_cfg_t cfg;
    aal_fdb_cpu_cmd_word4_msk_t  mask;

    memset((void *)&fdb_entry, 0, sizeof(aal_fdb_entry_cfg_t));
    memset((void *)&addr, 0, sizeof(ca_l2_addr_entry_t));
    L2_DEBUG("\r\n");

    /* load cfg */
    memset((void *)&cfg, 0, sizeof(aal_fdb_flush_cfg_t));

    aal_fdb_cmp_cfg_load(&cfg);

    mask.wrd = 0xffffffff;
    /* only join compare lpid */
    mask.bf.lpid = 0;

    aal_fdb_scan_flush_field_set(0, mask, &cfg);

    L2_DEBUG("please make sure call function aal_fdb_scan_flush_field_set to set word0--word4\r\n");

    while (1) {
        ret = aal_fdb_entry_scan(0,
                                start_addr, &hit_addr,
                                &fdb_entry, &cmd_ret);

        if (ret != CA_E_OK || cmd_ret != CA_AAL_FDB_ACCESS_HIT) {
            L2_DEBUG("search end\r\n");
            break;
        }

        aal_fdb_entry_2_l2_addr(&addr, &fdb_entry);

        L2_DEBUG("start_addr:%d, hit_addr:%d, ret:%d\r\n", start_addr, hit_addr, ret);
        L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d, valid:%d, tmp:%d\r\n",
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id,
                fdb_entry.valid,
                fdb_entry.tmp_flag);
        if (0 == item_cnt){
            printk(" idx  pid      mac           key vid lrnt vid dot1p sa  da static age   mc  valid tmp\r\n");
            printk("----+----+------------------+-------+-------+-----+---+---+-----+----+----+----+-----\r\n");
        }
        printk("%-5d%-5d%02x:%02x:%02x:%02x:%02x:%02x%4d/%c  %4d/%c      %-6d%-4d%-4d%-5d%-5d%-5d%-5d%-6d\r\n",
                hit_addr, fdb_entry.port_id,
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ? 's' : 'c',
                fdb_entry.lrnt_vid   ,
                fdb_entry.lrnt_sc_ind ? 's' : 'c',
                fdb_entry.dot1p      ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                FDB_AGING_TIME(fdb_entry.aging_sts)  ,
                fdb_entry.mc_flag    ,
                fdb_entry.valid,
                fdb_entry.tmp_flag);
        if (start_addr == hit_addr + 1){
            L2_DEBUG("start_addr == hit_addr + 1\r\n");
            break;
        }

        start_addr = hit_addr + 1;
        ++item_cnt;
    }

    ca_printf("\r\nTotal FDB entry : %u\n", item_cnt);

   ////////
/*
    item_cnt = 0;
    L2_DEBUG("\r\n");
    start_addr = 0;

    while (1) {
        ret = aal_fdb_entry_scan(0,
                                start_addr, &hit_addr,
                                &fdb_entry, &cmd_ret);

        if (ret != CA_E_OK || cmd_ret != CA_AAL_FDB_ACCESS_HIT) {
            L2_DEBUG("search end\r\n");
            break;
        }

        aal_fdb_entry_2_l2_addr(&addr, &fdb_entry);

        L2_DEBUG("start_addr:%d, hit_addr:%d, ret:%d\r\n", start_addr, hit_addr, ret);
        L2_DEBUG("mac:0x%x:%x:%x:%x:%x:%x, key_vid:%d, sc_ind:%d, dot1p:%d, lrnt_vid:%d, sa:%d, da:%d, static:%d, aging:%d, mc:%d, pid:%d\r\n",
                fdb_entry.mac[0], fdb_entry.mac[1], fdb_entry.mac[2], fdb_entry.mac[3], fdb_entry.mac[4], fdb_entry.mac[5],
                fdb_entry.key_vid    ,
                fdb_entry.key_sc_ind ,
                fdb_entry.dot1p      ,
                fdb_entry.lrnt_vid   ,
                fdb_entry.sa_permit  ,
                fdb_entry.da_permit  ,
                fdb_entry.static_flag,
                fdb_entry.aging_sts  ,
                fdb_entry.mc_flag    ,
                fdb_entry.port_id);

        if (start_addr == hit_addr + 1){
            L2_DEBUG("start_addr == hit_addr + 1\r\n");
            break;
        }

        start_addr = hit_addr + 1;
        if (++item_cnt > 10) break;
    }
    */

    L2_DEBUG("RB Tree: static table show\r\n");

    if (static_flag){
        aal_fdb_entry_cfg_t *entry_p = NULL;
        ca_int32_t  cnt = 1;

        entry_p = aal_fdb_static_getfirst(fdb_tbl_p);

        if(entry_p){
            ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d\r\n",
                cnt,
                entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                entry_p->key_vid, entry_p->port_id);
        }

        while (entry_p) {
            cnt++;
            entry_p = aal_fdb_static_getnext(fdb_tbl_p);
            if(entry_p){
                ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d\r\n",
                    cnt,
                    entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                    entry_p->key_vid, entry_p->port_id);            }
        }

        /* SVL tree */
        cnt = 1;
        entry_p = aal_fdb_static_getfirst(fdb_tbl_svl_p);

        if(entry_p){
            ca_printf("SVL TREE \r\nindex:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d\r\n",
                cnt,
                entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                entry_p->key_vid, entry_p->port_id);
        }

        while (entry_p) {
            cnt++;
            entry_p = aal_fdb_static_getnext(fdb_tbl_svl_p);
            if(entry_p){
                ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d\r\n",
                    cnt,
                    entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                    entry_p->key_vid, entry_p->port_id);            }
        }


    }
    return CA_E_OK;
}

ca_status_t aal_fdb_addr_delete_by_port_get (
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t          aal_port,
    CA_IN ca_uint32_t           flag
)
{

    ca_printf("flag: 0--static, 1--dynamic, 2--both\r\n");

    return __aal_fdb_addr_delete_by_port(device_id, aal_port, flag);

}


ca_status_t aal_fdb_addr_iterate_entry_cnt_get (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t             entry_count
)
{
    ca_iterator_t entry;
    ca_l2_addr_entry_t *l2_addr, *cfg;
    ca_status_t ret = CA_E_OK;
    int cnt = 0, j;
    int sz = 0;

    printk("ca_l2_addr_iterate, get once\r\n");
    memset(&entry, 0, sizeof(ca_iterator_t));
    entry.entry_count = entry_count;
    sz = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

    l2_addr = ca_malloc(sz);

    memset(l2_addr, 0, sz);
    entry.entry_data = l2_addr;
    entry.user_buffer_size = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

    /*while (1)*/{
        ret = ca_l2_addr_iterate(0, &entry);

        if (ret == CA_E_UNAVAIL){
            printk("end search\r\n");
      //      break;
        }

    }

    printk("entry_count = %#x \n",entry.entry_count);
    printk("entry_data = %p\n",entry.entry_data);
    printk("user_buffer_size = %#x \n",entry.user_buffer_size);
    printk("prev_handle = %p\n",entry.prev_handle);

    for (cnt = 0; cnt < entry.entry_count; cnt++){
        cfg = l2_addr + cnt;
        printk(" mac:");
        for(j = 0 ; j < 6; j++){
            printk("%x:", cfg->mac_addr[j]);
        }
        printk(" key_vid:%d, lrn_vid:%d, dot1p:%d, static:%d, sa:%d, da:%d, port_id:%#x, mc:%d, age:%d\r\n",
            cfg->vid, cfg->fwd_vid, cfg->dot1p, cfg->static_flag,
            cfg->sa_permit, cfg->da_permit, cfg->port_id, cfg->mc_group_id, cfg->aging_timer);
    }

    ca_free(l2_addr);

    return CA_E_OK;
}

ca_status_t aal_fdb_addr_iterate_loop_count_get (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_uint32_t             loop_count
)
{
    ca_iterator_t entry;
    ca_l2_addr_entry_t *l2_addr, *cfg;
    ca_status_t ret = CA_E_OK;
    int cnt = 0, j;
    int sz = 0;

    printk("ca_l2_addr_iterate: entry_count=1, loop up to loop_count\r\n");
    memset(&entry, 0, sizeof(ca_iterator_t));
    entry.entry_count = 1;
    sz = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

    l2_addr = ca_malloc(sz);

    memset(l2_addr, 0, sz);
    entry.entry_data = l2_addr;
    entry.user_buffer_size = sizeof(ca_l2_addr_entry_t) * entry.entry_count;

    for(cnt = 0; cnt < loop_count; cnt++){
        ret = ca_l2_addr_iterate(0, &entry);

        if (ret == CA_E_UNAVAIL){
            printk("end search\r\n");
            break;
        }


        printk("entry_count = %#x \n",entry.entry_count);
        printk("entry_data = %p\n",entry.entry_data);
        printk("user_buffer_size = %#x \n",entry.user_buffer_size);
        printk("prev_handle = %p\n",entry.prev_handle);

        cfg = l2_addr;
        printk(" mac:");
        for(j = 0 ; j < 6; j++){
            printk("%x:", cfg->mac_addr[j]);
        }
        printk(" key_vid:%d, lrn_vid:%d, dot1p:%d, static:%d, sa:%d, da:%d, port_id:%#x, mc:%d, age:%d\r\n",
            cfg->vid, cfg->fwd_vid, cfg->dot1p, cfg->static_flag,
            cfg->sa_permit, cfg->da_permit, cfg->port_id, cfg->mc_group_id, cfg->aging_timer);

    }

    ca_free(l2_addr);

    return CA_E_OK;
}

ca_mac_addr_t    dbg_filter_mac = {0x00, 0xca, 0xfe, 0x00, 0x00, 0x01};

ca_status_t aal_fdb_mac_filter_add_get (
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_int32_t             lpid,
    CA_IN ca_int32_t             mac_id
)
{
    aal_tbl_mac_filter_entry_t filter_entry;

    ca_status_t ret = CA_E_OK;
    //ca_int32_t lpid = CA_PORT_INDEX_GET(port_id);

    //L2_ASSERT_RET(entry != NULL, CA_E_PARAM);

    //CA_PORT_ETHERNET_CHECK(port_id);

    L2_DEBUG("port_id:%d\r\n", lpid);


    memset(&filter_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));

    filter_entry.mask = CA_L2_MAC_FILTER_MASK_MAC_VLAN;
    filter_entry.mac_flag = 0;
    filter_entry.vid = 11;
    filter_entry.drop_flag = 0;
    filter_entry.port_id = lpid;
    dbg_filter_mac[5] = mac_id;
    memcpy(&filter_entry.mac, &dbg_filter_mac, CA_ETH_ADDR_LEN);

    L2_DEBUG("mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:0x%x\r\n", filter_entry.mac[0],filter_entry.mac[1],filter_entry.mac[2],filter_entry.mac[3],filter_entry.mac[4],filter_entry.mac[5],
        filter_entry.vid, filter_entry.port_id);

    ret = aal_fdb_mac_filter_add(0, lpid, &filter_entry);

    return ret;
}


ca_status_t aal_fdb_mac_filter_delete_get (
    CA_IN ca_device_id_t            device_id,
    CA_IN ca_int32               lpid,
    CA_IN ca_int32_t             mac_id
)
{
    aal_tbl_mac_filter_entry_t filter_entry;
    ca_status_t ret = CA_E_OK;
    //ca_int32 lpid = CA_PORT_INDEX_GET(port_id);

    //L2_ASSERT_RET(entry != NULL, CA_E_PARAM);

    L2_DEBUG("port_id:%d\r\n", lpid);

    /*
    CA_PORT_ETHERNET_CHECK(port_id);
    */
    memset(&filter_entry, 0, sizeof(aal_tbl_mac_filter_entry_t));

    filter_entry.mask = CA_L2_MAC_FILTER_MASK_MAC_VLAN;
    filter_entry.mac_flag = 0;
    filter_entry.vid = 11;
    filter_entry.drop_flag = 0;
    filter_entry.port_id = lpid;
    dbg_filter_mac[5] = mac_id;
    memcpy(&filter_entry.mac, &dbg_filter_mac, CA_ETH_ADDR_LEN);

    L2_DEBUG("mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, port:%d\r\n",
        filter_entry.mac[0],filter_entry.mac[1],filter_entry.mac[2],filter_entry.mac[3],filter_entry.mac[4],filter_entry.mac[5],
        filter_entry.vid, filter_entry.port_id);

    ret = aal_fdb_mac_filter_delete(0, lpid, &filter_entry);

    return ret;
}


ca_status_t aal_fdb_mac_filter_delete_all_get (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_int32 lpid
)
{
    ca_status_t ret = CA_E_OK;
    //ca_int32 lpid = CA_PORT_INDEX_GET(port_id);

    //CA_PORT_ETHERNET_CHECK(port_id);

    ret = aal_fdb_mac_filter_delete_all(0, lpid);
    L2_DEBUG("port_id:%d\r\n", lpid);

    return ret;
}

ca_status_t aal_fdb_mac_filter_traverse_get (
    CA_IN ca_device_id_t             device_id
)
{
    aal_tbl_mac_filter_entry_t *entry_p = NULL;
    ca_int32_t  cnt = 1;
    ca_int32_t  i;

    entry_p = aal_fdb_mac_filter_getfirst();

    if(entry_p){
        ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, mask:%d, mac_flag:%d, drop_flag:%d, port_id:%d, entry:%d, rule:%d\r\n",
            cnt,
            entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
            entry_p->vid, entry_p->mask, entry_p->mac_flag, entry_p->drop_flag, entry_p->port_id, entry_p->entry_id, entry_p->rule_id);
    }

    while (entry_p) {
        cnt++;
        entry_p = aal_fdb_mac_filter_getnext();
        if(entry_p){
            ca_printf("index:%d\r\n      mac:%02x:%02x:%02x:%02x:%02x:%02x, vlan:%d, mask:%d, mac_flag:%d, drop_flag:%d, port_id:%d, entry:%d, rule:%d\r\n",
                cnt,
                entry_p->mac[0],entry_p->mac[1],entry_p->mac[2],entry_p->mac[3],entry_p->mac[4],entry_p->mac[5],
                entry_p->vid, entry_p->mask, entry_p->mac_flag, entry_p->drop_flag, entry_p->port_id, entry_p->entry_id, entry_p->rule_id);
        }
    }

    ca_printf("default action:\r\n");
    for (i = 0; i < 8; i++){
        ca_printf("port:%d:  action:%s\r\n", i,mac_filter_dft[i] ? "TRUE" : "FALSE");
    }

    return CA_E_OK;
}


/* debug code for mac filter covert id
ca_status_t aal_fdb_mac_filter_id_get()
{
    int i, partner_id, rule_0, rule_1;
    int port_id, rule_id, entry_id;

    for (i = 1; i <= 16; i++){
        partner_id = CA_AAL_RULE_ID_PARTNER(i);
        printf("i:%d, partner:%d\r\n", i, partner_id);

        rule_0 = CLS_IDX_IN_ENTRY(i);
        rule_1 = CA_AAL_RULE_ID_PARTNER(i);
    }

    for (i = 1; i <= 16; i++){
        partner_id = CA_AAL_RULE_ID_PARTNER(i);
        rule_0 = CLS_IDX_IN_ENTRY(i);
        rule_1 = CLS_IDX_IN_ENTRY(partner_id);
        printf("rule_id:%d, rule_0/1:%d/%d\r\n", i, rule_0, rule_1);
    }

    for (port_id = 0; port_id < 4; port_id ++)
        for (rule_id = 1; rule_id <= 16; rule_id++){
            entry_id = CLS_ENTRY_ID(port_id, rule_id);
            printf("port,rule-->entry: [%d, %d] --> %d\r\n", port_id, rule_id, entry_id);
        }
    return 0;
}

*/

ca_status_t aal_fdb_port_limit_init_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    return aal_fdb_port_limit_init(device_id, port_id);
}

ca_status_t aal_fdb_port_limit_inc_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    return aal_fdb_port_limit_inc(device_id, port_id);
}

ca_status_t aal_fdb_port_limit_dec_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    return aal_fdb_port_limit_dec(device_id, port_id);
}

ca_status_t aal_fdb_port_cnt_init_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32    port_id
)
{
    return aal_fdb_port_cnt_init(device_id, port_id);
}

ca_status_t aal_fdb_port_cnt_inc_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id
)
{
    return aal_fdb_port_cnt_inc(device_id, port_id);
}
ca_status_t aal_fdb_port_cnt_dec_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_int32        port_id
)
{
    return aal_fdb_port_cnt_dec(device_id, port_id);
}

#endif

#ifdef __KERNEL__

EXPORT_SYMBOL(aal_fdb_debug_set);
EXPORT_SYMBOL(aal_fdb_debug_get);
EXPORT_SYMBOL(aal_fdb_cmp_cfg_load);
EXPORT_SYMBOL(aal_fdb_entry_del);
EXPORT_SYMBOL(aal_fdb_entry_get);
#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_fdb_entry_offset_get);
#endif
EXPORT_SYMBOL(aal_fdb_port_limit_set);
EXPORT_SYMBOL(aal_fdb_port_cnt_inc);
EXPORT_SYMBOL(aal_fdb_mac_filter_delete);
EXPORT_SYMBOL(aal_fdb_ctrl_set);
EXPORT_SYMBOL(aal_fdb_mac_filter_delete_all_get);
EXPORT_SYMBOL(aal_fdb_port_limit_inc);
EXPORT_SYMBOL(aal_fdb_scan_flush_field_set);
EXPORT_SYMBOL(aal_fdb_port_limit_init_get);
EXPORT_SYMBOL(aal_fdb_port_cnt_init_get);
EXPORT_SYMBOL(aal_fdb_port_cnt_init);
EXPORT_SYMBOL(aal_fdb_mac_filter_add);
EXPORT_SYMBOL(aal_fdb_mac_filter_add_get);
EXPORT_SYMBOL(aal_fdb_entry_add);
EXPORT_SYMBOL(aal_fdb_mac_filter_default_get);
EXPORT_SYMBOL(aal_fdb_entry_addr_set);
EXPORT_SYMBOL(aal_fdb_port_cnt_set);
EXPORT_SYMBOL(aal_fdb_mac_filter_getnext);
EXPORT_SYMBOL(aal_fdb_entry_scan);
EXPORT_SYMBOL(aal_fdb_glb_intr_enable_set);
EXPORT_SYMBOL(aal_fdb_entry_flush);
EXPORT_SYMBOL(aal_fdb_mac_filter_delete_all);
EXPORT_SYMBOL(aal_fdb_port_cnt_dec);
EXPORT_SYMBOL(aal_fdb_port_limit_get);
EXPORT_SYMBOL(aal_fdb_port_limit_dec_get);
EXPORT_SYMBOL(aal_fdb_table_get);
EXPORT_SYMBOL(aal_fdb_port_limit_dec);
EXPORT_SYMBOL(aal_fdb_dbg_add_get);
EXPORT_SYMBOL(aal_fdb_entry_addr_get);
EXPORT_SYMBOL(aal_fdb_mac_filter_traverse_get);
EXPORT_SYMBOL(aal_fdb_learn_status_get);
EXPORT_SYMBOL(aal_fdb_addr_delete_get);
EXPORT_SYMBOL(aal_fdb_port_cnt_get);
EXPORT_SYMBOL(aal_fdb_mac_filter_getfirst);
EXPORT_SYMBOL(aal_fdb_addr_traverse_get);
EXPORT_SYMBOL(aal_fdb_addr_delete_by_port_get);
EXPORT_SYMBOL(aal_fdb_addr_iterate_loop_count_get);
EXPORT_SYMBOL(aal_fdb_addr_iterate_entry_cnt_get);
EXPORT_SYMBOL(aal_fdb_glb_intr_enable_get);
EXPORT_SYMBOL(aal_fdb_glb_intr_status_get);
EXPORT_SYMBOL(aal_fdb_mac_filter_init);
EXPORT_SYMBOL(aal_fdb_ctrl_get);
EXPORT_SYMBOL(aal_fdb_tbl_del_all);
EXPORT_SYMBOL(aal_fdb_addr_delete_all_get);
EXPORT_SYMBOL(aal_fdb_init);
EXPORT_SYMBOL(aal_fdb_mac_filter_default_set);
EXPORT_SYMBOL(aal_fdb_scan_flush_field_init);
EXPORT_SYMBOL(aal_fdb_port_limit_inc_get);
EXPORT_SYMBOL(aal_fdb_scan_flush_field_get);
EXPORT_SYMBOL(aal_fdb_port_limit_init);
EXPORT_SYMBOL(aal_fdb_port_cnt_inc_get);
EXPORT_SYMBOL(aal_fdb_mac_filter_delete_get);
EXPORT_SYMBOL(aal_fdb_port_cnt_dec_get);

EXPORT_SYMBOL(aal_fdb_lrn_fwd_ctrl_set);
EXPORT_SYMBOL(aal_fdb_lrn_fwd_ctrl_get);

EXPORT_SYMBOL(aal_fdb_cpu_pkt_lrn_set);
EXPORT_SYMBOL(aal_fdb_cpu_pkt_lrn_get);
EXPORT_SYMBOL(aal_fdb_limit_excd_fwd_ctrl_get);
EXPORT_SYMBOL(aal_fdb_ilpb_unknw_pol_idx_map_set);
EXPORT_SYMBOL(aal_fdb_ilpb_unknw_pol_idx_map_get);
EXPORT_SYMBOL(aal_fdb_ilpb_unknw_pol_idx_cb_get);
EXPORT_SYMBOL(aal_fdb_addr_aged_entry_get);
EXPORT_SYMBOL(aal_fdb_sw_lrn_traverse_get);
EXPORT_SYMBOL(aal_fdb_aging_time_set);
EXPORT_SYMBOL(aal_fdb_aging_time_get);


#endif


