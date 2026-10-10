/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * File   : aal_uni_macsec_intr.c
 * Brief  : XG(S)PON Interrupt config and handler AAL driver
 * History:
 *          2016-07-15 initial @Lidong
 *          2016-07-25 change __uni_macsec_intr_tbl to add name, ena_bmp
 *          2017-04-26 add XGPN_USBG_INTERRUPTE0 init and handler
 *
 */

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "ca-scfg/scfg.h"
#include "aal_common.h"
#include "aal_uni_macsec.h"
#include "aal_uni_macsec_intr.h"

#define NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_MASK_PN_VLD    (0x0001 << 5)
#define NI_HV_CORESEC10_TOP_NANDI_TBL_ACCESS_RETRY_MAX       2000

#define AAL_NI_HV_MACSEC_KMSG_FIFO_SIZE  4800
#define AAL_NI_HV_MACSEC_CHK_KMSG_CHN(chn)   do { \
                                        if((chn) >= AAL_NI_HV_MACSEC_KMSG_CHN_NUM) { \
                                            ca_printf("%s:[%d] chn is %d!!\n", __func__, __LINE__, (chn)); \
                                            return CA_E_PARAM; \
                                        } \
                                    } while(0)

#define AAL_NI_HV_MACSEC_CHK_KMSG_LEN(len)  do { \
                                        if((len) > AAL_NI_HV_MACSEC_KMSG_FIFO_SIZE) { \
                                            ca_printf("%s:[%d] msg-len is %d!!\n", __func__, __LINE__, (len)); \
                                            return CA_E_PARAM; \
                                        } \
                                    } while(0)

#define NI_HV_GLB_INTR_MASK_MACSEC_EN_INT            (0x0001 << 18)
#define NI_HV_GLB_INTR_MASK_MACSEC_DE_INT            (0x0001 << 17)

/*******************************************************************/
/*         Local Macros and Variables                              */
/*******************************************************************/
/* TOP NI_HV_MACSEC interrupt */


#define NI_HV_GLB_INTR_TOP_ENA_DEFAULT     0x60000 // macsec_en_int, macsec_de_int

/* NI_HV MACSEC NANDI interrupt enable flags */
#define NI_HV_MACSEC_INTR_NANDI_CPU_CMD_EXECUTED_EN       (1 << AAL_NI_HV_MACSEC_INTR_NANDI_CPU_CMD_EXECUTED_EN_BIT )
#define NI_HV_MACSEC_INTR_NANDI_PN_THLD_REACHED_EN        (1 << AAL_NI_HV_MACSEC_INTR_NANDI_PN_THLD_REACHED_EN_BIT )
#define NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_ERR_EN    (1 << AAL_NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_ERR_EN_BIT )
#define NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_CERR_EN   (1 << AAL_NI_HV_MACSEC_INTR_NANDI_KEY_HASH_MEM_RD_CERR_EN_BIT)
#define NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_ERR_EN        (1 << AAL_NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_ERR_EN_BIT)
#define NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_CERR_EN       (1 << AAL_NI_HV_MACSEC_INTR_NANDI_CNTL_MEM_RD_CERR_EN_BIT)
#define NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_ERR_EN          (1 << AAL_NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_ERR_EN_BIT)
#define NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_CERR_EN         (1 << AAL_NI_HV_MACSEC_INTR_NANDI_PN_MEM_RD_CERR_EN_BIT)

/*
 * Default Module based NI_HV MACSEC NANDI interrupt enable values defined here!
 */
#define __NI_HV_CORESEC10_TOP_CORESEC10_INTR_RX_ENA_DEFAULT  0
#define __NI_HV_CORESEC10_TOP_CORESEC10_INTR_TX_ENA_DEFAULT  0
#define __NI_HV_CORESEC10_TOP_NANDI_INTR_RX_ENA_DEFAULT  0
#define __NI_HV_CORESEC10_TOP_NANDI_INTR_TX_ENA_DEFAULT  0

/* Interrupt managment table */
static struct _intr_tbl_s{
    char        *name;         /* interrupt name */
    //ca_uint32_t  hw_irq;       /* hardware irq id */
    ca_uint32_t  ena_reg_addr; /* address of INTR enable register */
    ca_uint32_t  src_reg_addr; /* address of INTR source register */
    ca_uint32_t  ena_bmp;      /* default enable sub-interrupt bitmap */
    ca_uint64_t  count;        /* count of INTR */
    ca_uint64_t  fail_count;   /* count of fail handled INTR */
    ca_uint64_t  handle_count; /* count of ok handled INTR */
}__uni_macsec_intr_tbl[] = {
    { "NANDI_RX", NI_HV_CORESEC10_TOP_NANDI_INTENABLE_0,     NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0,     __NI_HV_CORESEC10_TOP_NANDI_INTR_RX_ENA_DEFAULT, 0, 0, 0},
    { "NANDI_TX",(NI_HV_CORESEC10_TOP_NANDI_INTENABLE_0 + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET),    (NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0 + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET), __NI_HV_CORESEC10_TOP_NANDI_INTR_TX_ENA_DEFAULT, 0, 0, 0},
    //{ "CORESEC10_RX", NI_HV_CORESEC10_TOP_CORESEC10_INTERRUPT_ENABLE_0,     NI_HV_CORESEC10_TOP_CORESEC10_INTERRUPT_0,     __NI_HV_CORESEC10_TOP_CORESEC10_INTR_RX_ENA_DEFAULT, 0, 0, 0},
    //{ "CORESEC10_TX",(NI_HV_CORESEC10_TOP_CORESEC10_INTERRUPT_ENABLE_0 + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET),    (NI_HV_CORESEC10_TOP_CORESEC10_INTERRUPT_0 + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET), __NI_HV_CORESEC10_TOP_CORESEC10_INTR_TX_ENA_DEFAULT, 0, 0, 0}
    
};

/* NI_HV_MACSEC interrupt handlers */
static aal_uni_macsec_intr_callback_t aal_uni_macsec_intr_isr[AAL_NI_HV_MACSEC_INTR_NUM];
static aal_uni_macsec_intr_msg_handler_t g_uni_macsec_intr_handlers[AAL_NI_HV_MACSEC_KMSG_CHN_NUM];


/*******************************************************************/
/*                    Local Functions                              */
/*******************************************************************/
/**
 * @aal_uni_macsec_intr_enable_set
 * Set NI_HV_MACSEC interrupt enable bitmap for a specific interrupt
 */
ca_status_t aal_uni_macsec_intr_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx,
    CA_IN ca_uint32_t  enable_bmp)
{

    if (intr_idx >= AAL_NI_HV_MACSEC_INTR_NUM)
        return CA_E_PARAM;

    CA_NI_REG_WRITE(enable_bmp, __uni_macsec_intr_tbl[intr_idx].ena_reg_addr);

    return CA_E_OK;
}

/**
 * @aal_uni_macsec_intr_enable
 * Enable NI_HV_MACSEC interrupt based on NI_HV_MACSEC interrupt index
 */
ca_status_t aal_uni_macsec_intr_enable(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t       intr_idx)
{
    return aal_uni_macsec_intr_enable_set(device_id, intr_idx, ~0U);
}

/**
 * @aal_uni_macsec_intr_disable
 * Disable NI_HV_MACSEC interrupt based on NI_HV_MACSEC interrupt index
 */
ca_status_t aal_uni_macsec_intr_disable(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_uint32_t       intr_idx)
{
    return aal_uni_macsec_intr_enable_set(device_id, intr_idx, 0U);
}

/**
 * @aal_uni_macsec_intr_enable_get
 * Get NI_HV_MACSEC interrupt enable bitmap of a specific interrupt
 */
ca_status_t aal_uni_macsec_intr_enable_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_OUT ca_uint32_t      *enable_bmp)
{
    if (intr_idx >= AAL_NI_HV_MACSEC_INTR_NUM || NULL == enable_bmp)
        return CA_E_PARAM;

    *enable_bmp = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_idx].ena_reg_addr);

    return CA_E_OK;
}

/**
 * @aal_uni_macsec_intr_status_get
 * Get NI_HV_MACSEC interrupt status of a specific interrupt
 */
ca_status_t aal_uni_macsec_intr_status_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_OUT ca_uint32_t      *status_bmp)
{
    ca_uint32_t ena_bmp = 0;

    if (intr_idx >= AAL_NI_HV_MACSEC_INTR_NUM || NULL == status_bmp)
        return CA_E_PARAM;

    ena_bmp = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_idx].ena_reg_addr);

    *status_bmp = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_idx].src_reg_addr);

    *status_bmp &= ena_bmp; /* ONLY return the status of enabled intr */

    return CA_E_OK;
}

/**
 * @aal_uni_macsec_intr_clr
 * Clear NI_HV_MACSEC interrupt to allow next interrupt incoming
 */
ca_status_t aal_uni_macsec_intr_clr(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_IN  ca_uint32_t      clr_bmp)
{
    ca_uint32_t ena_bmp = 0;

    if (intr_idx >= AAL_NI_HV_MACSEC_INTR_NUM)
        return CA_E_PARAM;

    ena_bmp = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_idx].src_reg_addr);
    ena_bmp &= clr_bmp;
    CA_NI_REG_WRITE(ena_bmp, __uni_macsec_intr_tbl[intr_idx].src_reg_addr); /* Write ones to clear */

    return CA_E_OK;
}

ca_status_t aal_uni_macsec_intr_info_get(
    CA_IN  ca_device_id_t  device_id
)
{
    ca_uint32_t  intr_idx;

    ca_printf("\r\n");
    ca_printf("%-10s %-12s %-12s %-12s %s\r\n",
              "PON-INTR", "EnableBmp", "Counter", "Handled Count", "Fail Count");
    ca_printf("---------------------------------------------------------------\r\n");

    for (intr_idx = 0; intr_idx < AAL_NI_HV_MACSEC_INTR_NUM; intr_idx++) {

        ca_printf("%-10s 0x%08x   %-12lld %-12lld %lld\r\n",
            __uni_macsec_intr_tbl[intr_idx].name,
            __uni_macsec_intr_tbl[intr_idx].ena_bmp,
            __uni_macsec_intr_tbl[intr_idx].count,
            __uni_macsec_intr_tbl[intr_idx].handle_count,
            __uni_macsec_intr_tbl[intr_idx].fail_count);
    }

    return CA_E_OK;
}

/**
 * @aal_uni_macsec_intr_name_get
 * Get NI_HV_MACSEC interrupt name string, normally for debug display
 */
ca_status_t aal_uni_macsec_intr_name_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint32_t      intr_idx,
    CA_IN  ca_uint32_t      name_len,
    CA_OUT char             *name_str)
{
    if (intr_idx >= AAL_NI_HV_MACSEC_INTR_NUM || NULL == name_str || name_len == 0)
        return CA_E_PARAM;

    if (__uni_macsec_intr_tbl[intr_idx].name && name_len)
        strncpy(name_str, __uni_macsec_intr_tbl[intr_idx].name, name_len);

    return CA_E_OK;
}


/**
 * @aal_uni_macsec_intr_reg
 * Register NI_HV_MACSEC interrupt handler, it will overwite the old handler
 */
ca_status_t aal_uni_macsec_intr_isr_reg(
    aal_uni_macsec_intr_t           uni_macsec_intr,
    aal_uni_macsec_intr_callback_t  callback)
{
    if (uni_macsec_intr >= AAL_NI_HV_MACSEC_INTR_NUM || callback == NULL)
        return CA_E_PARAM;

    aal_uni_macsec_intr_isr[uni_macsec_intr] = callback;

    return CA_E_OK;
}

/**
 * @aal_uni_macsec_intr_unreg
 * Unregister NI_HV_MACSEC interrupt handler
 */
ca_status_t aal_uni_macsec_intr_isr_unreg(
    aal_uni_macsec_intr_t     uni_macsec_intr)
{
    if (uni_macsec_intr >= AAL_NI_HV_MACSEC_INTR_NUM )
        return CA_E_PARAM;

    aal_uni_macsec_intr_isr[uni_macsec_intr] = NULL;

    return CA_E_OK;
}


ca_status_t aal_uni_macsec_intr_handler_reg(
    aal_uni_macsec_intr_msg_t   intr_msg,
    aal_uni_macsec_intr_msg_handler_t handler)
{
    AAL_NI_HV_MACSEC_CHK_KMSG_CHN(intr_msg);
    CA_NULL_PTR_CHECK(handler);

    g_uni_macsec_intr_handlers[intr_msg] = (aal_uni_macsec_intr_msg_handler_t)handler;

    return CA_E_OK;
}

ca_status_t aal_uni_macsec_intr_handler_unreg(
    aal_uni_macsec_intr_msg_t           intr_msg,
    aal_uni_macsec_intr_msg_handler_t   handler)
{
    AAL_NI_HV_MACSEC_CHK_KMSG_CHN(intr_msg);
    CA_NULL_PTR_CHECK(handler);

    if (handler == g_uni_macsec_intr_handlers[intr_msg]) {
        g_uni_macsec_intr_handlers[intr_msg] = NULL;
        return CA_E_OK;
    }

    return CA_E_NOT_FOUND;
}

static ca_status_t __uni_macsec_nandi_rx_decryp_isr(
    ca_uint32_t intr_id, 
    ca_uint32_t intr_sts)
{
    ca_status_t  ret = CA_E_OK;
    if (intr_sts & NI_HV_MACSEC_INTR_NANDI_PN_THLD_REACHED_EN) { 
        return ret;
    }
    return ret;

}

static ca_status_t __uni_macsec_nandi_tx_encryp_isr(
    ca_uint32_t intr_id, 
    ca_uint32_t intr_sts)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t write_timeout = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t uni_COMMAND_CONTROL;
    aal_uni_macsec_said_t said = {0}; 
    ca_uint32_t data_field_vld_addr = NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET;
    ca_uint32_t data_field_vld;

#endif 

    if (intr_sts & NI_HV_MACSEC_INTR_NANDI_PN_THLD_REACHED_EN) { //reset pn to 1 
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)   

        ret = aal_uni_macsec_tx_said_get(0, 0, &said);  
        CA_RET_VALUE_CHECK(ret);
        
        //reset PN to 1 when key exchange
        CA_NI_REG_WRITE(1,(NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET));
        data_field_vld = CA_NI_REG_READ(data_field_vld_addr);
        CA_NI_REG_WRITE(data_field_vld|NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_MASK_PN_VLD, data_field_vld_addr);
  
        /* Start to write */
        uni_COMMAND_CONTROL.bf.said = said.sa_id;
        uni_COMMAND_CONTROL.bf.rwn = 1;
        uni_COMMAND_CONTROL.bf.start_write = 1;
        CA_NI_REG_WRITE(uni_COMMAND_CONTROL.wrd, (NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET));
    
        /* Polling for write done */
        write_timeout = 0;
         while (uni_COMMAND_CONTROL.bf.start_write)
        {
            write_timeout++;
            if (write_timeout >= NI_HV_CORESEC10_TOP_NANDI_TBL_ACCESS_RETRY_MAX)
            {
                return CA_E_TIMEOUT;
            }
            uni_COMMAND_CONTROL.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + AAL_NI_HV_MACSEC_TX_RX_ADDR_OFFSET);
        }
    }
#endif
    return ret;
}

/* Framework to handle the PON interrupt */
#define DO_NI_HV_MACSEC_ISR(intr_id)  do {  \
        ca_uint32_t intre = 0;       \
        ca_uint32_t intrs = 0;       \
        ca_status_t ret   = CA_E_OK; \
        intre = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_id].ena_reg_addr);  \
        intrs = CA_NI_REG_READ(__uni_macsec_intr_tbl[intr_id].src_reg_addr);  \
        CA_NI_REG_WRITE(0, __uni_macsec_intr_tbl[intr_id].ena_reg_addr);      \
        CA_NI_REG_WRITE(intrs & intre, __uni_macsec_intr_tbl[intr_id].src_reg_addr);  \
        __uni_macsec_intr_tbl[intr_id].count ++;                                \
        if (NULL != aal_uni_macsec_intr_isr[intr_id]) {                         \
            ret =(*aal_uni_macsec_intr_isr[intr_id])(intr_id, intrs & intre);\
            if (CA_E_OK != ret) \
                __uni_macsec_intr_tbl[intr_id].fail_count ++;   \
            else \
                __uni_macsec_intr_tbl[intr_id].handle_count ++;   \
        }                                                                  \
        CA_NI_REG_WRITE(intre, __uni_macsec_intr_tbl[intr_id].ena_reg_addr);  \
    }while(0)

void aal_uni_macsec_intr_handler(
    ca_device_id_t  device_id, 
    ca_uint32_t     irq, 
    ca_uint32_t     uni_macsec_int)
{
    /* handler all NI_HV_MACSEC interrupt here */
    /* TODO: add intr type here */

    /* Handle PLOAM interrupt 1stly */
    if (uni_macsec_int & NI_HV_GLB_INTR_MASK_MACSEC_DE_INT) {
        DO_NI_HV_MACSEC_ISR(AAL_NI_HV_MACSEC_NANDI_INTR_RX_DECRY);
    }

    /* Handle ACTP interrupt 2ndly */
    if (uni_macsec_int & NI_HV_GLB_INTR_MASK_MACSEC_EN_INT){
        DO_NI_HV_MACSEC_ISR(AAL_NI_HV_MACSEC_NANDI_INTR_TX_ENCRY);
    }
}

/**
 * @aal_uni_macsec_intr_init
 * Initialize NI_HV_MACSEC interrupt config and handler
 */
ca_status_t aal_uni_macsec_intr_init(
    ca_device_id_t  device_id)
{
    ca_uint32_t  idx  = 0;
    ca_status_t  ret  = CA_E_OK;

    /* Disable Top interrupts  */
    CA_NI_REG_WRITE (0, NI_HV_GLB_INTERRUPTE);
    idx = CA_NI_REG_READ(NI_HV_GLB_INTERRUPT); /* read clear */

    /* Global var init */
    memset(&aal_uni_macsec_intr_isr[0], 0, sizeof(aal_uni_macsec_intr_isr));
    
    /* Hook the interrupt by Platform interfaces */
    (void)aal_uni_macsec_intr_isr_reg(AAL_NI_HV_MACSEC_NANDI_INTR_RX_DECRY,   __uni_macsec_nandi_rx_decryp_isr);
    (void)aal_uni_macsec_intr_isr_reg(AAL_NI_HV_MACSEC_NANDI_INTR_TX_ENCRY,   __uni_macsec_nandi_tx_encryp_isr);
    
    /* Enable used interrupts and clear interrupt status  */
    for (idx = 0; idx < AAL_NI_HV_MACSEC_INTR_NUM; idx++) {
        aal_uni_macsec_intr_enable_set(device_id, idx, 0); /* disable firstly */
        aal_uni_macsec_intr_clr(device_id, idx, __uni_macsec_intr_tbl[idx].ena_bmp); /* clear status */
        aal_uni_macsec_intr_enable_set(device_id, idx, __uni_macsec_intr_tbl[idx].ena_bmp); /* enable MUST interrupt */
        if (__uni_macsec_intr_tbl[idx].ena_bmp) {
            ca_printf("ca-pon: Enable NI_HV_MACSEC %s interrupt : 0x%08x\n",
                __uni_macsec_intr_tbl[idx].name, __uni_macsec_intr_tbl[idx].ena_bmp);
        }
    }

    /* Enable TOP interrupt */
    CA_NI_REG_WRITE (NI_HV_GLB_INTR_TOP_ENA_DEFAULT, NI_HV_GLB_INTERRUPTE);

    return CA_E_OK;

}


#ifdef __KERNEL__
EXPORT_SYMBOL(aal_uni_macsec_intr_enable);
EXPORT_SYMBOL(aal_uni_macsec_intr_disable);
EXPORT_SYMBOL(aal_uni_macsec_intr_name_get);
EXPORT_SYMBOL(aal_uni_macsec_intr_status_get);
EXPORT_SYMBOL(aal_uni_macsec_intr_enable_get);
EXPORT_SYMBOL(aal_uni_macsec_intr_enable_set);
EXPORT_SYMBOL(aal_uni_macsec_intr_clr);
EXPORT_SYMBOL(aal_uni_macsec_intr_init);
EXPORT_SYMBOL(aal_uni_macsec_intr_handler_reg);
EXPORT_SYMBOL(aal_uni_macsec_intr_handler_unreg);
EXPORT_SYMBOL(aal_uni_macsec_intr_info_get);
EXPORT_SYMBOL(aal_uni_macsec_intr_handler);
#endif

