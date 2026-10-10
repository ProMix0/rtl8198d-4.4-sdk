#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/in.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <net/xfrm.h>
#include <linux/proc_fs.h>
#include <bspchip.h>
#include "ca_ni.h"
#include "rtk_ni_test.h"
#include "rl6562_ca8277b_test.h"
#include "aal_l3qm.h"
#include "aal_common.h"
#include "ca_mut.h"
#include "ca_ext.h"

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

struct rtk_ni_test_private rtk_ni_test_priv = {0};
EXPORT_SYMBOL(rtk_ni_test_priv);

int __rx_info_read(struct seq_file *seq, void *v)
{
	HEADER_A_T *hdr_a;
	HEADER_CPU_T *hdr_cpu;
	HEADER_PTP_T *hdr_ptp;
	
	seq_printf(seq, "da_mac=%02X:%02X:%02X:%02X:%02X:%02X\n", rtk_ni_test_priv.rx_info.da_mac[0]
		, rtk_ni_test_priv.rx_info.da_mac[1], rtk_ni_test_priv.rx_info.da_mac[2]
		, rtk_ni_test_priv.rx_info.da_mac[3], rtk_ni_test_priv.rx_info.da_mac[4]
		, rtk_ni_test_priv.rx_info.da_mac[5]);

	seq_printf(seq, "sa_mac=%02X:%02X:%02X:%02X:%02X:%02X\n", rtk_ni_test_priv.rx_info.sa_mac[0]
		, rtk_ni_test_priv.rx_info.sa_mac[1], rtk_ni_test_priv.rx_info.sa_mac[2]
		, rtk_ni_test_priv.rx_info.sa_mac[3], rtk_ni_test_priv.rx_info.sa_mac[4]
		, rtk_ni_test_priv.rx_info.sa_mac[5]);

	seq_printf(seq, "HEADER_A:\n");
	hdr_a = rtk_ni_test_priv.rx_info.hdr_a;
	seq_printf(seq, "cos: %d\n", hdr_a->bits.cos);
	seq_printf(seq, "ldpid: %d\n", hdr_a->bits.ldpid);
	seq_printf(seq, "lspid: %d\n", hdr_a->bits.lspid);
	seq_printf(seq, "pkt_size: %d\n", hdr_a->bits.pkt_size);
	seq_printf(seq, "fe_bypass: %d\n", hdr_a->bits.fe_bypass);
	seq_printf(seq, "hdr_type: %d\n", hdr_a->bits.hdr_type);

	seq_printf(seq, "mcgid: %d\n", hdr_a->bits.bits_32_63.pkt_info.mcgid);
	seq_printf(seq, "drop_code: %d\n", hdr_a->bits.bits_32_63.pkt_info.drop_code);
	seq_printf(seq, "rx_pkt_type: %d\n", hdr_a->bits.bits_32_63.pkt_info.rx_pkt_type);
	seq_printf(seq, "no_drop: %d\n", hdr_a->bits.bits_32_63.pkt_info.no_drop);
	seq_printf(seq, "mirror: %d\n", hdr_a->bits.bits_32_63.pkt_info.mirror);
	seq_printf(seq, "mark: %d\n", hdr_a->bits.bits_32_63.pkt_info.mark);
	seq_printf(seq, "pol_en: %d\n", hdr_a->bits.bits_32_63.pkt_info.pol_en);
	seq_printf(seq, "pol_id: %d\n", hdr_a->bits.bits_32_63.pkt_info.pol_id);
	seq_printf(seq, "pol_grp_id: %d\n", hdr_a->bits.bits_32_63.pkt_info.pol_grp_id);
	seq_printf(seq, "deep_q: %d\n", hdr_a->bits.bits_32_63.pkt_info.deep_q);
	seq_printf(seq, "cpu_flg: %d\n", hdr_a->bits.bits_32_63.pkt_info.cpu_flg);

	seq_printf(seq, "ptp_info: %d\n", hdr_a->bits.bits_32_63.ptp_info.ptp_info);
	seq_printf(seq, "cpu_flg: %d\n", hdr_a->bits.bits_32_63.ptp_info.cpu_flg);

	seq_printf(seq, "cmd_asel: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.cmd_asel);
	seq_printf(seq, "cmd_op: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.cmd_op);
	seq_printf(seq, "cmd_mode: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.cmd_mode);
	seq_printf(seq, "ptp_type: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.ptp_type);
	seq_printf(seq, "cmd_offset: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.cmd_offset);
	seq_printf(seq, "udp_cksum_offset: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.udp_cksum_offset);
	seq_printf(seq, "cpu_flg: %d\n", hdr_a->bits.bits_32_63.ptp_cmd.cpu_flg);

	seq_printf(seq, "HEADER_CPU:\n");
	hdr_cpu = rtk_ni_test_priv.rx_info.hdr_cpu;
	seq_printf(seq, "mdata_h: %d\n", hdr_cpu->mdata_uc.mdata_h);
	seq_printf(seq, "lpm_result: %d\n", hdr_cpu->mdata_uc.lpm_result);
	seq_printf(seq, "l3_egress_if_id: %d\n", hdr_cpu->mdata_uc.l3_egress_if_id);
	seq_printf(seq, "l3_ingress_if_id: %d\n", hdr_cpu->mdata_uc.l3_ingress_if_id);

	seq_printf(seq, "mdata_h: %d\n", hdr_cpu->mdata_mc.mdata_h);
	seq_printf(seq, "mc_fib_id: %d\n", hdr_cpu->mdata_mc.mc_fib_id);
	seq_printf(seq, "l3_egress_if_id: %d\n", hdr_cpu->mdata_mc.l3_egress_if_id);
	seq_printf(seq, "l3_ingress_if_id: %d\n", hdr_cpu->mdata_mc.l3_ingress_if_id);

	seq_printf(seq, "hash_dbl_chk_fail: %d\n", hdr_cpu->hdr_i_info.hash_dbl_chk_fail);
	seq_printf(seq, "hash_idx: %d\n", hdr_cpu->hdr_i_info.hash_idx);
	seq_printf(seq, "cls_action: %d\n", hdr_cpu->hdr_i_info.cls_action);
	seq_printf(seq, "fwd_vld: %d\n", hdr_cpu->hdr_i_info.fwd_vld);
	seq_printf(seq, "l2l3_flag: %d\n", hdr_cpu->hdr_i_info.l2l3_flag);
	seq_printf(seq, "l4_offset: %d\n", hdr_cpu->hdr_i_info.l4_offset);
	seq_printf(seq, "l3_offset: %d\n", hdr_cpu->hdr_i_info.l3_offset);

	seq_printf(seq, "mdata_h: 0x%x\n", hdr_cpu->mdata_raw.mdata_h);
	seq_printf(seq, "mdata_l: 0x%x\n", hdr_cpu->mdata_raw.mdata_l);

	seq_printf(seq, "HEADER_PTP:\n");
	hdr_ptp = rtk_ni_test_priv.rx_info.hdr_ptp;
	seq_printf(seq, "correction_l: 0x%x\n", hdr_ptp->bits.correction_l);
	seq_printf(seq, "correction_h: 0x%x\n", hdr_ptp->bits.correction_h);
	seq_printf(seq, "rx_time_stamp: 0x%x\n", hdr_ptp->bits.rx_time_stamp);
	seq_printf(seq, "cmd_op: 0x%x\n", hdr_ptp->bits.cmd_op);
	seq_printf(seq, "cmd_asel: 0x%x\n", hdr_ptp->bits.cmd_asel);
	seq_printf(seq, "cmd_mode: 0x%x\n", hdr_ptp->bits.cmd_mode);
	seq_printf(seq, "cmd_rewrite: 0x%x\n", hdr_ptp->bits.cmd_rewrite);
	seq_printf(seq, "ptp_type: 0x%x\n", hdr_ptp->bits.ptp_type);
	seq_printf(seq, "udp_chksum_offset: 0x%x\n", hdr_ptp->bits.udp_chksum_offset);
	seq_printf(seq, "cmd_offset: 0x%x\n", hdr_ptp->bits.cmd_offset);
	
	return (0);
}
EXPORT_SYMBOL(__rx_info_read);

int __tx_info_n_read(struct seq_file *seq, void *v)
{
	static struct rtk_ni_test_txinfo *data;
	struct file *file = seq->private;

	data=PDE_DATA(file_inode(file));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if(data->tx_info_flag)
	{
		/*flow_id*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_FLOW_ID)
		{
			seq_printf(seq, "flow_id=%d\n", data->tx_info.flow_id);
		}
		/*go_9th_queue*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_GO_9TH_QUEUE)
		{
			seq_printf(seq, "go_9th_queue=%d\n", data->tx_info.go_9th_queue);
		}
		/*core_config*/
		seq_printf(seq, "core_config=0x%llx\n", data->tx_info.core_config.longlong);
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_TXQ_INDEX)
		{
			seq_printf(seq, "txq_index=%d\n", data->tx_info.core_config.bf.txq_index);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_LDPID)
		{
			seq_printf(seq, "ldpid=%d\n", data->tx_info.core_config.bf.ldpid);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_LSPID)
		{
			seq_printf(seq, "lspid=%d\n", data->tx_info.core_config.bf.lspid);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_IS_FROM_CA_TX)
		{
			seq_printf(seq, "is_from_ca_tx=%d\n", data->tx_info.core_config.bf.is_from_ca_tx);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_BYPASS_FWD_ENGINE)
		{
			seq_printf(seq, "bypass_fwd_engine=%d\n", data->tx_info.core_config.bf.bypass_fwd_engine);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_CB_FN_MASK)
		{
			seq_printf(seq, "cb_fn_mask=%d\n", data->tx_info.core_config.bf.cb_fn_mask);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_FLOW_ID_SET)
		{	
			seq_printf(seq, "flow_id_set=%d\n", data->tx_info.core_config.bf.flow_id_set);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_SW_ID)
		{
			seq_printf(seq, "sw_id=%lld\n", data->tx_info.core_config.bf.sw_id);
		}
		/*xmit_done_cb_fn*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_XMIT_DONE_CB_FN)
		{
			seq_printf(seq, "xmit_done_cb_fn=%p\n", data->tx_info.xmit_done_cb_fn);
		}
		/*bypass_lso*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_BYPASS_LSO)
		{
			seq_printf(seq, "bypass_lso=%d\n", data->tx_info.bypass_lso);
		}
		/*ptp_tx_cmd*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_PTP_TX_TYPE)
		{
			seq_printf(seq, "ptp_tx_type=%d\n", data->tx_info.ptp_tx_cmd.ptp_tx_type);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_UDP_CHECKSUM_OFFSET)
		{
			seq_printf(seq, "udp_checksum_offset=%d\n", data->tx_info.ptp_tx_cmd.udp_checksum_offset);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_MODIFIED_FIELD_OFFSET)
		{
			seq_printf(seq, "modified_field_offset=%d\n", data->tx_info.ptp_tx_cmd.modified_field_offset);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_TX_CMD)
		{
			seq_printf(seq, "tx_cmd=%d\n", data->tx_info.ptp_tx_cmd.tx_cmd);
		}
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_CORRECTION_SELECT)
		{
			seq_printf(seq, "correction_select=%d\n", data->tx_info.ptp_tx_cmd.correction_select);
		}
		/*pkt_type*/
		if(data->tx_info_flag&RTK_NI_TEST_TXINFOFLAG_PKT_TYPE)
		{
			seq_printf(seq, "pkt_type=%d\n", data->tx_info.pkt_type);
		}
	}
	else
	{
		seq_printf(seq, "using default tx_info.\n");
	}
	
	return (0);
}
EXPORT_SYMBOL(__tx_info_n_read);

int __tx_info_n_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_txinfo *data;
	char tmpbuf[64];
	char *strptr;
	char *fieldptr,*valueptr;
	unsigned long long value64;
	unsigned int value;

	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return count;
	}

	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		fieldptr = strsep(&strptr," ");
		if(fieldptr == NULL)
		{
			goto errout;
		}
		
		if(strcmp(fieldptr,"default"))
		{
			valueptr = strsep(&strptr," ");
			if(valueptr == NULL)
			{
				goto errout;
			}

			if(!strcmp(fieldptr,"sw_id"))
			{
				value64 = simple_strtoll(valueptr, NULL, 0);
			}
			else
			{
				value = simple_strtol(valueptr, NULL, 0);
			}
			
			printk("set field: %s, value %d\n",fieldptr,value);
			
			if(!strcmp(fieldptr,"flow_id"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_FLOW_ID;
				data->tx_info.flow_id = value;
			}
			else if(!strcmp(fieldptr,"go_9th_queue"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_GO_9TH_QUEUE;
				data->tx_info.go_9th_queue = value;
			}
			else if(!strcmp(fieldptr,"txq_index"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_TXQ_INDEX;
				data->tx_info.core_config.bf.txq_index = value;
			}
			else if(!strcmp(fieldptr,"ldpid"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_LDPID;
				data->tx_info.core_config.bf.ldpid = value;
			}
			else if(!strcmp(fieldptr,"lspid"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_LSPID;
				data->tx_info.core_config.bf.lspid = value;
			}
			else if(!strcmp(fieldptr,"is_from_ca_tx"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_IS_FROM_CA_TX;
				data->tx_info.core_config.bf.is_from_ca_tx = value;
			}
			else if(!strcmp(fieldptr,"bypass_fwd_engine"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_BYPASS_FWD_ENGINE;
				data->tx_info.core_config.bf.bypass_fwd_engine = value;
			}
			else if(!strcmp(fieldptr,"cb_fn_mask"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_CB_FN_MASK;
				data->tx_info.core_config.bf.cb_fn_mask = value;
			}
			else if(!strcmp(fieldptr,"flow_id_set"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_FLOW_ID_SET;
				data->tx_info.core_config.bf.flow_id_set = value;
			}
			else if(!strcmp(fieldptr,"sw_id"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_SW_ID;
				data->tx_info.core_config.bf.sw_id = value64;
			}
			else if(!strcmp(fieldptr,"xmit_done_cb_fn"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_XMIT_DONE_CB_FN;
				data->tx_info.xmit_done_cb_fn = value;
			}
			else if(!strcmp(fieldptr,"bypass_lso"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_BYPASS_LSO;
				data->tx_info.bypass_lso = value;
			}
			else if(!strcmp(fieldptr,"ptp_tx_type"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_PTP_TX_TYPE;
				data->tx_info.ptp_tx_cmd.ptp_tx_type = value;
			}
			else if(!strcmp(fieldptr,"udp_checksum_offset"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_UDP_CHECKSUM_OFFSET;
				data->tx_info.ptp_tx_cmd.udp_checksum_offset = value;
			}
			else if(!strcmp(fieldptr,"modified_field_offset"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_MODIFIED_FIELD_OFFSET;
				data->tx_info.ptp_tx_cmd.modified_field_offset = value;
			}
			else if(!strcmp(fieldptr,"tx_cmd"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_TX_CMD;
				data->tx_info.ptp_tx_cmd.tx_cmd = value;
			}
			else if(!strcmp(fieldptr,"correction_select"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_CORRECTION_SELECT;
				data->tx_info.ptp_tx_cmd.correction_select = value;
			}
			else if(!strcmp(fieldptr,"pkt_type"))
			{
				data->tx_info_flag |= RTK_NI_TEST_TXINFOFLAG_PKT_TYPE;
				data->tx_info.pkt_type = value;
			}
		}
		else
		{
			printk("set tx_info to default.\n");
			memset(&(data->tx_info), 0, sizeof(ca_ni_tx_config_t));
			data->tx_info_flag = 0;
		}
		
		return count;
	}
	else
	{
errout:
		printk("%s error\n", __func__);
		printk("default\n");
		printk("flow_id\n");
		printk("go_9th_queue\n");
		printk("txq_index\n");
		printk("ldpid\n");
		printk("lspid\n");
		printk("is_from_ca_tx\n");
		printk("bypass_fwd_engine\n");
		printk("cb_fn_mask\n");
		printk("flow_id_set\n");
		printk("sw_id\n");
		printk("xmit_done_cb_fn\n");
		printk("bypass_lso\n");
		printk("ptp_tx_type\n");
		printk("udp_checksum_offset\n");
		printk("modified_field_offset\n");
		printk("tx_cmd\n");
		printk("pkt_type\n");
	}
	
	return count;
}
EXPORT_SYMBOL(__tx_info_n_write);

static void rl6562_test_apply_txinfo(ca_ni_tx_config_t *tx_config, unsigned int tx_info_flag, ca_ni_tx_config_t *tx_info)
{
	if(tx_info_flag)
	{
		/*flow_id*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_FLOW_ID)
		{
			tx_config->flow_id = tx_info->flow_id;
		}
		/*go_9th_queue*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_GO_9TH_QUEUE)
		{
			tx_config->go_9th_queue = tx_info->go_9th_queue;
		}
		/*core_config*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_TXQ_INDEX)
		{
			tx_config->core_config.bf.txq_index = tx_info->core_config.bf.txq_index;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_LDPID)
		{
			tx_config->core_config.bf.ldpid = tx_info->core_config.bf.ldpid;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_LSPID)
		{
			tx_config->core_config.bf.lspid = tx_info->core_config.bf.lspid;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_IS_FROM_CA_TX)
		{
			tx_config->core_config.bf.is_from_ca_tx = tx_info->core_config.bf.is_from_ca_tx;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_BYPASS_FWD_ENGINE)
		{
			tx_config->core_config.bf.bypass_fwd_engine = tx_info->core_config.bf.bypass_fwd_engine;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_CB_FN_MASK)
		{
			tx_config->core_config.bf.cb_fn_mask = tx_info->core_config.bf.cb_fn_mask;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_FLOW_ID_SET)
		{
			tx_config->core_config.bf.flow_id_set = tx_info->core_config.bf.flow_id_set;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_SW_ID)
		{
			tx_config->core_config.bf.sw_id = tx_info->core_config.bf.sw_id;
		}
		/*xmit_done_cb_fn*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_XMIT_DONE_CB_FN)
		{
			tx_config->xmit_done_cb_fn = tx_info->xmit_done_cb_fn;
		}
		/*bypass_lso*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_BYPASS_LSO)
		{
			tx_config->bypass_lso = tx_info->bypass_lso;
		}
		/*ptp_tx_cmd*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_PTP_TX_TYPE)
		{
			tx_config->ptp_tx_cmd.ptp_tx_type = tx_info->ptp_tx_cmd.ptp_tx_type;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_UDP_CHECKSUM_OFFSET)
		{
			tx_config->ptp_tx_cmd.udp_checksum_offset = tx_info->ptp_tx_cmd.udp_checksum_offset;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_MODIFIED_FIELD_OFFSET)
		{
			tx_config->ptp_tx_cmd.modified_field_offset = tx_info->ptp_tx_cmd.modified_field_offset;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_TX_CMD)
		{
			tx_config->ptp_tx_cmd.tx_cmd = tx_info->ptp_tx_cmd.tx_cmd;
		}
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_CORRECTION_SELECT)
		{
			tx_config->ptp_tx_cmd.correction_select = tx_info->ptp_tx_cmd.correction_select;
		}
		/*pkt_type*/
		if(tx_info_flag&RTK_NI_TEST_TXINFOFLAG_PKT_TYPE)
		{
			tx_config->pkt_type = tx_info->pkt_type;
		}
	}
	else
	{
		TX_TRACE("using default tx_info.");
	}
}

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTL_MULTI_LAN_DEV)
extern struct ca_ni_multi_lan_dev_table_entry ca_ni_multi_lan_dev_table[MAX_CA_NI_MULTI_LAN_DEV_NUM];
#define TOTAL_CA_NI_MULTI_LAN_DEV_NUM	((sizeof(ca_ni_multi_lan_dev_table))/(sizeof(struct ca_ni_multi_lan_dev_table_entry)))

static int rl6562_test_ca_ni_is_multi_lan_device(struct net_device *dev)
{
	unsigned int totalDev = TOTAL_CA_NI_MULTI_LAN_DEV_NUM;
	char* dev_name;
	int i;

	for (i = 0; i < totalDev; i++) {
		if (ca_ni_multi_lan_dev_table[i].dev_instant) {
			dev_name = ca_ni_multi_lan_dev_table[i].dev_instant->name;
		}
		else {
			printk("no dev_instant, strange.......\n");
			dev_name = ca_ni_multi_lan_dev_table[i].ifname;
		}
		if (!strcmp(dev_name, dev->name)) {
			return (1);
		}
	}
	return (0);
}
#endif

int rl6562_test_rx2txl34csreset(struct sk_buff *skb)
{
	struct iphdr *iphdr;
	struct ipv6hdr *ipv6hdr;
	struct tcphdr *tcphdr;
	struct udphdr *udphdr;
	char *current_ptr;
	__be16 proto;

	TX_TRACE("%s-%d enter", __func__, __LINE__);

	current_ptr = &(skb->data[ETH_ALEN*2]);
	proto = ntohs(*((__be16 *) current_ptr));
	if (proto==ETH_P_IP) {
		current_ptr += sizeof(proto);
		iphdr = (struct iphdr *)current_ptr;
		TX_TRACE("IPv4 header checksum reset (%02X %02X)", (ntohs(iphdr->check)&0xff00)>>8, (ntohs(iphdr->check)&0xff));
		iphdr->check = 0;
		if(iphdr->protocol == RTK_NI_TEST_L4PROTO_TCP)
		{
			current_ptr += sizeof(struct iphdr);
			tcphdr = current_ptr;			
			TX_TRACE("TCP header checksum reset (%02X %02X)", (ntohs(tcphdr->check)&0xff00)>>8, (ntohs(tcphdr->check)&0xff));
			tcphdr->check = 0;
		}
		else if(iphdr->protocol == RTK_NI_TEST_L4PROTO_UDP)
		{
			current_ptr += sizeof(struct iphdr);
			udphdr = current_ptr;
			TX_TRACE("UDP header checksum reset (%02X %02X)", (ntohs(udphdr->check)&0xff00)>>8, (ntohs(udphdr->check)&0xff));
			udphdr->check = 0;
		}
	} else if (proto==ETH_P_IPV6) {
		TX_TRACE("It's an IPv6 packet");
		current_ptr += sizeof(proto);
		ipv6hdr = (struct ipv6hdr *)current_ptr;		
		if(ipv6hdr->nexthdr == RTK_NI_TEST_L4PROTO_TCP)
		{
			current_ptr += sizeof(struct ipv6hdr);
			tcphdr = current_ptr;
			TX_TRACE("TCP header checksum reset (%02X %02X)", (ntohs(tcphdr->check)&0xff00)>>8, (ntohs(tcphdr->check)&0xff));
			tcphdr->check = 0;
		}
		else if(ipv6hdr->nexthdr == RTK_NI_TEST_L4PROTO_UDP)
		{
			current_ptr += sizeof(struct ipv6hdr);
			udphdr = current_ptr;
			TX_TRACE("UDP header checksum reset (%02X %02X)", (ntohs(udphdr->check)&0xff00)>>8, (ntohs(udphdr->check)&0xff));
			udphdr->check = 0;
		}
	}

	TX_TRACE("%s-%d exit", __func__, __LINE__);

	TX_SKB_DBG(skb, RTK_NI_TEST_SKB_TX);

	return (0);
}

int rl6562_test_dump_txinfo(ca_ni_tx_config_t *ca_tx_config)
{
	/*flow_id*/
	TX_TRACE("flow_id=%d", ca_tx_config->flow_id);
	/*go_9th_queue*/
	TX_TRACE("go_9th_queue=%d", ca_tx_config->go_9th_queue);
	/*core_config*/
	TX_TRACE("core_config=0x%llx", ca_tx_config->core_config.longlong);
	TX_TRACE("txq_index=%d", ca_tx_config->core_config.bf.txq_index);
	TX_TRACE("ldpid=%d", ca_tx_config->core_config.bf.ldpid);
	TX_TRACE("lspid=%d", ca_tx_config->core_config.bf.lspid);
	TX_TRACE("is_from_ca_tx=%d", ca_tx_config->core_config.bf.is_from_ca_tx);
	TX_TRACE("bypass_fwd_engine=%d", ca_tx_config->core_config.bf.bypass_fwd_engine);
	TX_TRACE("cb_fn_mask=%d", ca_tx_config->core_config.bf.cb_fn_mask);
	TX_TRACE("flow_id_set=%d", ca_tx_config->core_config.bf.flow_id_set);
	TX_TRACE("sw_id=%d", ca_tx_config->core_config.bf.sw_id);
	/*xmit_done_cb_fn*/
	TX_TRACE("xmit_done_cb_fn=%p", ca_tx_config->xmit_done_cb_fn);
	/*bypass_lso*/
	TX_TRACE("bypass_lso=%d", ca_tx_config->bypass_lso);
	/*ptp_tx_cmd*/
	TX_TRACE("ptp_tx_type=%d", ca_tx_config->ptp_tx_cmd.ptp_tx_type);
	TX_TRACE("udp_checksum_offset=%d", ca_tx_config->ptp_tx_cmd.udp_checksum_offset);
	TX_TRACE("modified_field_offset=%d", ca_tx_config->ptp_tx_cmd.modified_field_offset);
	TX_TRACE("tx_cmd=%d", ca_tx_config->ptp_tx_cmd.tx_cmd);
	TX_TRACE("correction_select=%d", ca_tx_config->ptp_tx_cmd.correction_select);
	/*pkt_type*/
	TX_TRACE("pkt_type=%d", ca_tx_config->pkt_type);
	
	return (0);
}

int rl6562_test_nic_rxhook(struct napi_struct *napi, struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv)
{
	ca_eth_private_t *cep = netdev_priv(napi->dev);
	//ca_eth_private_t *cep = netdev_priv(skb->dev);
	struct net_device_stats *stats = &cep->dev->stats;
	ca_ni_tx_config_t ca_tx_config;
    u16 vid;
	int ret;
	ca_uint16_t ethertype;
	int vlan_offset,pkt_len;
	ca_uint8_t *tmp_ptr;

	spin_lock_bh(&rtk_ni_test_priv.rtk_ni_test_lock);

	RX_TRACE("%s-%d: enter >>>>>>>>>>", __func__, __LINE__);
	RX_TRACE("%s-%d: dev=%s", __func__, __LINE__, skb->dev->name);
	
	memcpy(&rtk_ni_test_priv.rx_info, nh_priv, sizeof(nic_hook_private_t));

	RX_SKB_DBG(skb, RTK_NI_TEST_SKB_RX);

	if(rtk_ni_test_priv.rx2tx)
	{
		if (skb->dev->flags&IFF_UP) {
			memset(&ca_tx_config, 0, sizeof(ca_tx_config));
			ca_tx_config.core_config.bf.ldpid = PORT_ID(nh_priv->hdr_a->bits.lspid);
			ca_tx_config.core_config.bf.txq_index = 0;
			ca_tx_config.core_config.bf.lspid = RTK_NI_TEST_MAC_PORT_CPU;
			ca_tx_config.core_config.bf.is_from_ca_tx =  1;
			ca_tx_config.core_config.bf.bypass_fwd_engine =  1;

			rl6562_test_apply_txinfo(&ca_tx_config, rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_NORMAL].tx_info_flag, &rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_NORMAL].tx_info);
			
			rl6562_test_dump_txinfo(&ca_tx_config);

			TX_SKB_DBG(skb, RTK_NI_TEST_SKB_TX);

			if(rtk_ni_test_priv.rx2txl34csreset)
			{
				TX_TRACE("clear L34 checksum.");
				rl6562_test_rx2txl34csreset(skb);
			}
			
			rtk_ni_test_priv.tx_api[rtk_ni_test_priv.tx_api_idx](skb, skb->dev, &ca_tx_config);
		}
	}
	else if((nh_priv->hdr_a->bits.lspid >= 0x12 && nh_priv->hdr_a->bits.lspid <= 0x17)
		&& (nh_priv->hdr_a->bits.ldpid >= 0x12 && nh_priv->hdr_a->bits.ldpid <= 0x17))
	{
		memset(&ca_tx_config, 0, sizeof(ca_tx_config));
		ca_tx_config.core_config.bf.ldpid = PORT_ID(nh_priv->hdr_a->bits.lspid);
		ca_tx_config.core_config.bf.txq_index = 0;
		ca_tx_config.core_config.bf.lspid = RTK_NI_TEST_MAC_PORT_CPU;
		ca_tx_config.core_config.bf.is_from_ca_tx =  1;
		ca_tx_config.core_config.bf.bypass_fwd_engine =  1;
		
		rl6562_test_apply_txinfo(&ca_tx_config, rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_WIFI].tx_info_flag, &rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_WIFI].tx_info);

		rl6562_test_dump_txinfo(&ca_tx_config);

		TX_SKB_DBG(skb, RTK_NI_TEST_SKB_TX);

		rtk_ni_test_priv.tx_api[rtk_ni_test_priv.tx_api_idx](skb, skb->dev, &ca_tx_config);
	}
	else
	{
		skb->protocol = eth_type_trans(skb, skb->dev);
		skb->pkt_type = PACKET_HOST;
		/* end of fill the sturcture of skb */
		//dump_port_cfg(&(cep->port_cfg));
		ca_skb_out(skb);
		
		if (unlikely(napi && ((dev->features) & NETIF_F_GRO))) {
			napi_gro_receive(napi, skb);
		}
		else{
			netif_receive_skb(skb);
		}
	}
	
	if(rtk_ni_test_priv.debug_times)
		rtk_ni_test_priv.debug_times--;
	RX_TRACE("%s-%d: exit <<<<<<<<<<", __func__, __LINE__);

	spin_unlock_bh(&rtk_ni_test_priv.rtk_ni_test_lock);
	
	return RE8670_RX_STOP_SKBNOFREE;
}

static int rl6562_test_init(void)
{
	nic_register_rxhook(0x7f,RENIC_RXPRI_PATCH,rl6562_test_nic_rxhook);
	memset(&rtk_ni_test_priv, 0, sizeof(struct rtk_ni_test_private));
	spin_lock_init(&rtk_ni_test_priv.rtk_ni_test_lock);
	rtk_ni_test_priv.tx_api[0] = __ca_ni_start_xmit;
	rtk_ni_test_priv.tx_api[1] = __ca_ni_start_xmit_buf;
	return (0);
}

static int rl6562_test_exit(void)
{
	nic_unregister_rxhook(0x7f,RENIC_RXPRI_PATCH,rl6562_test_nic_rxhook);
	return (0);
}

static int rl6562_test_start_xmit(struct sk_buff *skb)
{
	ca_ni_tx_config_t ca_tx_config;

	if(skb == NULL)
	{
		TX_TRACE("skb==NULL return.");
		return -1;
	}

	memset(&ca_tx_config, 0, sizeof(ca_ni_tx_config_t));
	ca_tx_config.core_config.bf.ldpid = PORT_ID(0);
	ca_tx_config.core_config.bf.txq_index = 0;
	ca_tx_config.core_config.bf.lspid = RTK_NI_TEST_MAC_PORT_CPU;
	ca_tx_config.core_config.bf.is_from_ca_tx =  1;
	ca_tx_config.core_config.bf.bypass_fwd_engine =  1;

	rl6562_test_apply_txinfo(&ca_tx_config, rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_NORMAL].tx_info_flag, &rtk_ni_test_priv.tx_info[RTK_NI_TEST_TXINFO_INDEX_NORMAL].tx_info);

	rl6562_test_dump_txinfo(&ca_tx_config);

	if(skb->dev == NULL)
	{
		skb->dev = dev_get_by_name(&init_net, "eth0");
	}
	if(skb->dev)
	{
		TX_TRACE("skb->dev=%s", skb->dev->name);
	}

	TX_SKB_DBG(skb, RTK_NI_TEST_SKB_TX);

	rtk_ni_test_priv.tx_api[rtk_ni_test_priv.tx_api_idx](skb, skb->dev, &ca_tx_config);

	return (0);
}

int __rtk_ni_test_init(void)
{
	return rl6562_test_init();
}
EXPORT_SYMBOL(__rtk_ni_test_init);

int __rtk_ni_test_exit(void)
{
	return rl6562_test_exit();
}
EXPORT_SYMBOL(__rtk_ni_test_exit);

int __rtk_ni_test_start_xmit(struct sk_buff *skb)
{
	return rl6562_test_start_xmit(skb);
}
EXPORT_SYMBOL(__rtk_ni_test_start_xmit);

