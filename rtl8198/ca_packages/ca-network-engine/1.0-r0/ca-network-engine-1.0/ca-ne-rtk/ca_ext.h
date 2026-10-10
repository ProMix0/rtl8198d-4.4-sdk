
#ifndef __CA_EXT_H__
#define __CA_EXT_H__

#include <linux/list.h>
#include "ca_ni.h"
#include <aal_l3fe.h>


#define	NIC_DRIVER_IS_READY 0
#define	NIC_DRIVER_IS_NOT_READY 1


#if !defined(_RE8686_RTL9607C_H_)
enum {
	RE8670_RX_STOP=0,
	RE8670_RX_CONTINUE,
	RE8670_RX_STOP_SKBNOFREE,
	RE8670_RX_END
};
#endif
	
/*
*   | callback                      	| priority		| return
*   | re8670_rx_skb                 	| 0			| STOP
*   | fwdEngine_rx_skb      		| 1			| STOP
*   | rtk_gpon_omci_rx_wrapper	| 2			| STOP
*   | re8686_dump_rx			| 6			| IS CONTINUE
*   | re8686_rx_patch			| 7			| IS CONTINUE
*/

typedef enum {
	RENIC_RXPRI_DEFAULT=0,
	RENIC_RXPRI_RG,
	RENIC_RXPRI_L34LITE,
	RENIC_RXPRI_VOIP,
	RENIC_RXPRI_OMCI,
	RENIC_RXPRI_OAM,
	RENIC_RXPRI_MPCP,
	RENIC_RXPRI_MUTICAST,
	RENIC_RXPRI_DUMP,
	RENIC_RXPRI_PATCH,
}RENIC_RX_PRI_T;

typedef struct drv_nic_hook_entry
{
	int portmask;
	int priority;
	p2rfunc_t do_rx;
	struct list_head list;

}drv_nic_hook_entry_t ;

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
typedef struct
{
    ca_uint32_t uc_pkt;
    ca_uint32_t mc_pkt;
    ca_uint32_t bc_pkt;
    ca_uint64_t uc_bytes;
    ca_uint64_t mc_bytes;
    ca_uint64_t bc_bytes;
}rtk_sw_mib_t;

int rtk_nic_cpu_us_mib_counting(unsigned char *dmac, unsigned int pktlen);
int rtk_nic_cpu_us_mib_get(rtk_sw_mib_t *pCpuUsMib);
int rtk_nic_cpu_us_mib_clear(void);
#if defined(CONFIG_ARCH_REALTEK_TAURUS)
typedef enum {
	RTK_TX_MODE_DEFAULT=0,
	RTK_TX_MODE_DIRECT_TX_WAN,
	RTK_TX_MODE_DIRECT_TX_LAN,
	RTK_TX_MODE_WIFI_HW_LOOKUP,
}rtk_tx_dma_lso_mode_t;

rtk_tx_dma_lso_mode_t rtk_dma_lso_mode_check(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);
#endif
#endif

uint32_t rtk_ne_reg_read(uint32_t addr);
void rtk_ne_reg_write(uint32_t data, uint32_t addr);
uint32_t rtk_dma_lso_reg_read(uint32_t addr);
void rtk_dma_lso_reg_write(uint32_t data, uint32_t addr);
struct net_device * rtk_ni_virtual_cpuport_alloc(int cpuPort, char *name);
uint32_t rtk_ni_virtual_cpuport_free(int cpuPort);
uint32_t rtk_ni_virtual_cpuport_open(int cpuPort);
uint32_t rtk_ni_virtual_cpuport_close(int cpuPort);


#endif

