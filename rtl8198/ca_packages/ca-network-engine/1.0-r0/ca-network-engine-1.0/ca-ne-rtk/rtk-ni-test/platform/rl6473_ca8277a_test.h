#ifndef RL6473_CA8277A_TEST_H
#define RL6473_CA8277A_TEST_H

int nic_register_rxhook(int portmask,int priority,p2rfunc_t rx);
int nic_unregister_rxhook(int portmask,int priority,p2rfunc_t rx);
int nic_register_txfunc(p2tfunc_t pfunc);
int nic_register_txfunc_default(void);
netdev_tx_t nic_egress_start_xmit(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);

#define RTK_NI_TEST_MAC_PORT_CPU	0x10

enum RTK_NI_TEST_TXINFOFLAG{
	RTK_NI_TEST_TXINFOFLAG_FLOW_ID = (1<<0),
	RTK_NI_TEST_TXINFOFLAG_GO_9TH_QUEUE = (1<<1),
	RTK_NI_TEST_TXINFOFLAG_TXQ_INDEX = (1<<2),
	RTK_NI_TEST_TXINFOFLAG_LDPID = (1<<3),
	RTK_NI_TEST_TXINFOFLAG_LSPID = (1<<4),
	RTK_NI_TEST_TXINFOFLAG_IS_FROM_CA_TX = (1<<5),
	RTK_NI_TEST_TXINFOFLAG_BYPASS_FWD_ENGINE = (1<<6),
	RTK_NI_TEST_TXINFOFLAG_CB_FN_MASK = (1<<7),
	RTK_NI_TEST_TXINFOFLAG_FLOW_ID_SET = (1<<8),
	RTK_NI_TEST_TXINFOFLAG_SW_ID = (1<<9),
	RTK_NI_TEST_TXINFOFLAG_XMIT_DONE_CB_FN = (1<<10),
	RTK_NI_TEST_TXINFOFLAG_BYPASS_LSO = (1<<11),
	RTK_NI_TEST_TXINFOFLAG_PTP_TX_TYPE = (1<<12),
	RTK_NI_TEST_TXINFOFLAG_UDP_CHECKSUM_OFFSET = (1<<13),
	RTK_NI_TEST_TXINFOFLAG_MODIFIED_FIELD_OFFSET = (1<<14),
	RTK_NI_TEST_TXINFOFLAG_TX_CMD = (1<<15),
	RTK_NI_TEST_TXINFOFLAG_CORRECTION_SELECT = (1<<16),
	RTK_NI_TEST_TXINFOFLAG_PKT_TYPE = (1<<17),
};

#endif
