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
 * ca_aal_proc.c: Proc entry for Cortina AAL
 *
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <soc/cortina/cortina-soc.h>

#include "aal_l3fe.h"
#include "aal_l3_pe.h"
#include "aal_l3_cls.h"
#include "aal_l3_stg0.h"
#include "aal_table.h"

struct proc_dir_entry *proc_aal = NULL;

/***************************************************************************
 * cntr_drop
 ***************************************************************************/

#define CNTR_DROP_HELP_MSG \
	"Purpose: Packet/Byte Drop Counter of all sources.\n" \
	"  There are total 64 counters, the valid NUMBER range is 0 to 63.\n" \
	"  *** All dumpped numbers are not captured at same moment due to hardware design. ***\n" \
	"\n" \
	"DUMP ALL counter:\n" \
	"  echo dump > %s\n" \
	"  echo dump all > %s\n" \
	"\n" \
	"DUMP specific counter:\n" \
	"  echo dump NUMBER > %s\n" \
	"\n" \
	"CLEAR ALL counter:\n" \
	"  echo clear > %s\n" \
	"  echo clear all > %s\n" \
	"\n" \
	"CLEAR specific counter:\n" \
	"  echo clear NUMBER > %s\n" \
	"\n"

char CounterDesc[L3PE_CNTR_DROP_SRC_ENTRY_MAX][80] = {
	"PP: L2 pkt size illigal (L2_pkt_size < L2_hdr_size + L3_total_len)", // 0
	"PP: L3L4 header offset exceeds 254",
	"PP: IPv4 header checksum validation error",
	"PP: UDP checksum zero",
	"PP: Other detected errors",
	"PP: More than 2 VLAN ID", // 5
	"LPB: MRU check fail",
	"LPB: MyMAC filtering",
	"LPB: default drop",
	"Special packet drop",
	"reserved", // 10
	"DSLite/6RD: Decapsulate ECN check fail",
	"DSLite/6RD: Outer IPDA and IPSA reverse path of forwarding check fail",
	"6RD: outer and inner IPSA consistency check fail",
	"6RD: Decapsulate 6RD IPDA delegated prefix check fail",
	"6RD: Decapsulate 6RD IPMC drop", // 15
	"6RD: outer IPMC with inner IPUC packet",
	"DSLite: IPMC addr prefix check fail",
	"DSLite: IPMC embedded IPv4 addr check fail",
	"DSLite: outer IPMC with inner IPUC packet",
	"DSLite/6RD: IF ID check fail", // 20
	"DSLite: Decapsulate DSLite IPMC drop",
	"Hash Engine double check fail",
	"LPM search miss",
	"HashLite Engine double check fail",
	"T5: ILPB filter drop", // 25
	"T5: MC drop",
	"T5: SA deny",
	"T5: DA deny",
	"T5: software learning deny",
	"T5: unknown VLAN", // 30
	"T5: VLAN membership check fail",
	"T5: STP mode block drop",
	"T5: STP mode learn only and SMAC hit drop",
	"T5: VE deny",
	"T5: VE MC VLAN drop", // 35
	"T5: VE FIB drop",
	"T5: L2E FIB drop",
	"TE: reserved1",
	"TE: reserved2",
	"TE: unmarked WRED drop", // 40
	"TE: marked WRED drop",
	"TE: tail drop",
	"TE: policer drop",
	"TE: reserved7",
	"PE: TCP/UDP/RDPv1/RDPv2 checksum error", // 45
	"PE: L2 CRC error",
	"PE: MTU check fail",
	"STG1: L3 IF table",
	"STG1: FIB LDPID is blackhole",
	"STG1: TTL is 0", // 50
	"STG1: CR other drop",
	"STG2: L3 IF table",
	"STG2: FIB LDPID is blackhole",
	"STG2: TTL is 0",
	"STG2: CR other drop", // 55
	"STG3.U1: L3 IF table",
	"STG3.U1: FIB LDPID is blackhole",
	"STG3.U1: TTL is 0",
	"STG3.U1: CR other drop",
	"STG3.U2: L3 IF table", // 60
	"STG3.U2: FIB LDPID is blackhole",
	"STG3.U2: TTL is 0",
	"STG3.U2: CR other drop",
};

static int cntr_drop_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CNTR_DROP_HELP_MSG,
		file->f_path.dentry->d_iname, file->f_path.dentry->d_iname,
		file->f_path.dentry->d_iname, file->f_path.dentry->d_iname,
		file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int cntr_drop_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cntr_drop_show_help, (void *) file);
}

ssize_t __cntr_drop_proc_write(ca_device_id_t device_id, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	bool isDump = true; /* false: clear counter, true: dump counter */
	bool isAll = false;
	unsigned long i, start = 0, end = L3PE_CNTR_DROP_SRC_ENTRY_MAX;
	uint64_t cntPkt, cntByte;
	int ret;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _cntr_drop_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "clear", strlen(token)) == 0) {
			isDump = false;
		} else if (strncmp(token, "dump", strlen(token)) == 0) {
			isDump = true;
		} else {
			printk(KERN_ERR "ERROR! Invalid action '%s'.\n", token);
			goto _cntr_drop_invalid;
		}
	} else {
		goto _cntr_drop_invalid;
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "all", strlen(token)) == 0) {
			isAll = true;
		} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
			if (strict_strtoul(token, 0, &start))
#else
			if (kstrtoul(token, 0, &start))
#endif
			{
				printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
				goto _cntr_drop_invalid;
			}

			end = start + 1;
		}
	} else {
		goto _cntr_drop_invalid;
	}

	/* do action */
	if (start >= L3PE_CNTR_DROP_SRC_ENTRY_MAX) {
		printk(KERN_ERR "ERROR! Out of range. There is no counter #%lu.\n", start);
	}

	if ((isDump == false) && (isAll == true)) {
		if ((ret = aal_l3pe_cntr_drop_src_pkt_clear(device_id)) != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear drop packet count. (ret=%d)\n", ret);
			goto _cntr_drop_invalid;
		}

		if ((ret = aal_l3pe_cntr_drop_src_byte_clear(device_id)) != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear drop byte count. (ret=%d)\n", ret);
			goto _cntr_drop_invalid;
		}

		goto _cntr_drop_end;
	}

	if (isDump == true)
		printk(KERN_INFO "Counter#  PktCount       ByteCount  Description\n");
	for (i = start; i < end; i++) {
		if (isDump == true) {
			if ((ret = aal_l3pe_cntr_drop_src_pkt_get(device_id, i, &cntPkt)) != AAL_E_OK) {
				printk(KERN_ERR "ERROR! Fail to get packet count. (counter index=%lu, ret=%d)\n", i, ret);
				goto _cntr_drop_invalid;
			}

			if ((ret = aal_l3pe_cntr_drop_src_byte_get(device_id, i, &cntByte)) != AAL_E_OK) {
				printk(KERN_ERR "ERROR! Fail to get byte count. (counter index=%lu, ret=%d)\n", i, ret);
				goto _cntr_drop_invalid;
			}

			printk(KERN_INFO "%8lu  %8llu  %14llu  %s\n", i, cntPkt, cntByte, CounterDesc[i]);
		} else {
			if ((ret = aal_l3pe_cntr_drop_src_pkt_set(device_id, i, 0)) != AAL_E_OK) {
				printk(KERN_ERR "ERROR! Fail to clear packet count. (counter index=%lu, ret=%d)\n", i, ret);
				goto _cntr_drop_invalid;
			}

			if ((ret = aal_l3pe_cntr_drop_src_byte_set(device_id, i, 0)) != AAL_E_OK) {
				printk(KERN_ERR "ERROR! Fail to clear byte count. (counter index=%lu, ret=%d)\n", i, ret);
				goto _cntr_drop_invalid;
			}
		}
	}

_cntr_drop_end:
	return count;

_cntr_drop_invalid:
	/* if we return error code here, PROC fs may retry up to 3 times. */
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

ssize_t cntr_drop_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return __cntr_drop_proc_write(0, file, buf, count, pos);
}

static const struct file_operations fops_cntr_drop __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= cntr_drop_proc_open,
	.read		= seq_read,
	.write		= cntr_drop_proc_write,
};

/***************************************************************************
 * cntr_rx
 ***************************************************************************/

#define CNTR_RX_HELP_MSG \
	"Purpose: RX Couter based on Interface ID\n" \
	"\n" \
	"DUMP counter of specific Interface ID:\n" \
	"  echo dump IDX > %s\n" \
	"\n" \
	"CLEAR counter of specifc Interface ID:\n" \
	"  echo clear IDX > %s\n" \
	"\n" \
	"IDX:\n" \
	"  0 ~ 63: Interface ID 0 ~ 63\n" \
	"  64: Interface ID 64 ~ 254\n" \
	"  65: Interface ID 255\n" \
	"\n"

static int cntr_rx_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CNTR_RX_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int cntr_rx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cntr_rx_show_help, (void *) file);
}

ssize_t __cntr_rx_proc_write(ca_device_id_t device_id, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	bool isDump = true; /* false: clear counter, true: dump counter */
	bool isAll = false;
	unsigned long index = 0;
	l3pe_cntr_rx_t data;
	int ret;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _cntr_rx_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "clear", strlen(token)) == 0) {
			isDump = false;
		} else if (strncmp(token, "dump", strlen(token)) == 0) {
			isDump = true;
		} else {
			printk(KERN_ERR "ERROR! Invalid action '%s'.\n", token);
			goto _cntr_rx_invalid;
		}
	} else {
		goto _cntr_rx_invalid;
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "all", strlen(token)) == 0) {
			isAll = true;
		} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
			if (strict_strtoul(token, 0, &index))
#else
			if (kstrtoul(token, 0, &index))
#endif
			{
				printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
				goto _cntr_rx_invalid;
			}
		}
	} else {
		goto _cntr_rx_invalid;
	}

	/* do action */
	if (index >= L3PE_CNTR_RX_ENTRY_MAX) {
		printk(KERN_ERR "ERROR! Out of range. There is no counter #%lu.\n", index);
	}

	if ((isDump == true) && (isAll == true)) {
		printk(KERN_ERR "ERROR! Does not support DUMP ALL.\n");
		goto _cntr_rx_invalid;
	}

	if ((isDump == false) && (isAll == true)) {
		if ((ret = aal_l3pe_cntr_rx_clear(device_id)) != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear RX count. (ret=%d)\n", ret);
			goto _cntr_rx_invalid;
		}

		goto _cntr_rx_end;
	}

	if (isDump == true) {
		ret = aal_l3pe_cntr_rx_get(device_id, index, &data);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to get RX counter #%lu. (ret=%d)\n", index, ret);
			goto _cntr_rx_invalid;
		}

		if (index < 64) {
			printk(KERN_INFO "RX counter of IFID #%lu:\n", index);
		} else if (index == 64) {
			printk(KERN_INFO "RX counter of IFID #64 ~ #254:\n");
		} else if (index == 65) {
			printk(KERN_INFO "RX counter of IFID #255:\n");
		} else {
			printk(KERN_INFO "RX counter of unknown IFID:\n");
		}

		printk(KERN_INFO "  RX bytes: %llu\n", data.byte);
		printk(KERN_INFO "  Unicast packets: %u\n", data.uc_pkt);
		printk(KERN_INFO "  None-Unicast packets: %u\n", data.nuc_pkt);
		printk(KERN_INFO "  Dropped bytes: %llu\n", data.drop_byte);
		printk(KERN_INFO "  Dropped packets: %u\n", data.drop_pkt);
		printk(KERN_INFO "  Error packets: %u\n", data.error_pkt);

	} else {

		memset(&data, 0, sizeof(data));
		ret = aal_l3pe_cntr_rx_set(device_id, index, &data);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear RX counter #%lu. (ret=%d)\n", index, ret);
			goto _cntr_rx_invalid;
		}

		printk(KERN_INFO "RX counter #%lu is cleared.\n", index);

	}

_cntr_rx_end:
	return count;

_cntr_rx_invalid:
	/* if we return error code here, PROC fs may retry up to 3 times. */
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

ssize_t cntr_rx_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return __cntr_rx_proc_write(0, file, buf, count, pos);
}

static const struct file_operations fops_cntr_rx __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= cntr_rx_proc_open,
	.read		= seq_read,
	.write		= cntr_rx_proc_write,
};

/***************************************************************************
 * cntr_tx
 ***************************************************************************/

#define CNTR_TX_HELP_MSG \
	"Purpose: TX Couter based on Interface ID\n" \
	"\n" \
	"DUMP counter of specific Interface ID:\n" \
	"  echo dump IDX > %s\n" \
	"\n" \
	"CLEAR counter of specifc Interface ID:\n" \
	"  echo clear IDX > %s\n" \
	"\n" \
	"IDX:\n" \
	"  0 ~ 63: Interface ID 0 ~ 63\n" \
	"  64: Interface ID 64 ~ 254\n" \
	"  65: Interface ID 255\n" \
	"\n"

static int cntr_tx_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, CNTR_TX_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int cntr_tx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cntr_tx_show_help, (void *) file);
}

ssize_t __cntr_tx_proc_write(ca_device_id_t device_id, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	bool isDump = true; /* false: clear counter, true: dump counter */
	bool isAll = false;
	unsigned long index = 0;
	l3pe_cntr_tx_t data;
	int ret;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _cntr_tx_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "clear", strlen(token)) == 0) {
			isDump = false;
		} else if (strncmp(token, "dump", strlen(token)) == 0) {
			isDump = true;
		} else {
			printk(KERN_ERR "ERROR! Invalid action '%s'.\n", token);
			goto _cntr_tx_invalid;
		}
	} else {
		goto _cntr_tx_invalid;
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (strncmp(token, "all", strlen(token)) == 0) {
			isAll = true;
		} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
			if (strict_strtoul(token, 0, &index))
#else
			if (kstrtoul(token, 0, &index))
#endif
			{
				printk(KERN_ERR "ERROR! Invalid value '%s'.\n", token);
				goto _cntr_tx_invalid;
			}
		}
	} else {
		goto _cntr_tx_invalid;
	}

	/* do action */
	if (index >= L3PE_CNTR_TX_ENTRY_MAX) {
		printk(KERN_ERR "ERROR! Out of range. There is no counter #%lu.\n", index);
	}

	if ((isDump == true) && (isAll == true)) {
		printk(KERN_ERR "ERROR! Does not support DUMP ALL.\n");
		goto _cntr_tx_invalid;
	}

	if ((isDump == false) && (isAll == true)) {
		if ((ret = aal_l3pe_cntr_tx_clear(device_id)) != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear TX count. (ret=%d)\n", ret);
			goto _cntr_tx_invalid;
		}

		goto _cntr_tx_end;
	}

	if (isDump == true) {
		ret = aal_l3pe_cntr_tx_get(device_id, index, &data);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to get TX counter #%lu. (ret=%d)\n", index, ret);
			goto _cntr_tx_invalid;
		}

		if (index < 64) {
			printk(KERN_INFO "TX counter of IFID #%lu:\n", index);
		} else if (index == 64) {
			printk(KERN_INFO "TX counter of IFID #64 ~ #254:\n");
		} else if (index == 65) {
			printk(KERN_INFO "TX counter of IFID #255:\n");
		} else {
			printk(KERN_INFO "TX counter of unknown IFID:\n");
		}

		printk(KERN_INFO "  TX bytes: %llu\n", data.byte);
		printk(KERN_INFO "  Unicast packets: %u\n", data.uc_pkt);
		printk(KERN_INFO "  None-Unicast packets: %u\n", data.nuc_pkt);
		printk(KERN_INFO "  Dropped bytes: %llu\n", data.drop_byte);
		printk(KERN_INFO "  Dropped packets: %u\n", data.drop_pkt);

	} else {

		memset(&data, 0, sizeof(data));
		ret = aal_l3pe_cntr_tx_set(device_id, index, &data);
		if (ret != AAL_E_OK) {
			printk(KERN_ERR "ERROR! Fail to clear TX counter #%lu. (ret=%d)\n", index, ret);
			goto _cntr_tx_invalid;
		}

		printk(KERN_INFO "TX counter #%lu is cleared.\n", index);

	}

_cntr_tx_end:
	return count;

_cntr_tx_invalid:
	/* if we return error code here, PROC fs may retry up to 3 times. */
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

ssize_t cntr_tx_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return __cntr_tx_proc_write(0, file, buf, count, pos);
}

static const struct file_operations fops_cntr_tx __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= cntr_tx_proc_open,
	.read		= seq_read,
	.write		= cntr_tx_proc_write,
};

/***************************************************************************
 * l3fe_debug
 ***************************************************************************/

#define L3FE_DEBUG_HELP_MSG \
	"Purpose: Debug packet in L3FE\n" \
	"  *** Not ensure the dump contents belong to same packet due to hardware design. ***\n" \
	"\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo VALUE > %s\n" \
	"  0: HDR_I between PP ~ STG0\n" \
	"  1: HDR_I between STG0 ~ T1 (CLS); LPB & SPB results are merged\n" \
	"  2: HDR_I between STG1 ~ T2 (Hash); include IPnIP decapsulation result\n" \
	"  3: CLS result\n" \
	"  4: Parsed inner IP header between PP ~ STG0\n" \
	"  5: HDR_I after common FIB result merged (before IPnIP decapsulation)\n" \
	"  6: HDR_I between STG1 ~ T2 (Hash)\n" \
	"  7: HDR_I between T2 (Hash) ~ STG2\n" \
	"  8: HDR_I between STG2 ~ T3 (LPM); could check if hash is hit, and Hash FIB is merged if stage2_ctrl is 0\n" \
	"  9: HDR_I between T3 (LPM) ~ T4 (HashLite)\n" \
	"  10: HDR_I between T4 (HashLite) ~ STG3.U1; only could check if HashLite is hit\n" \
	"  11: HDR_I between STG3.U1 ~ STG3.U2; HashLite FIB is merged\n" \
	"  12: HDR_I between STG3.U2 ~ T5 (L2 Lookup); Hash FIB is merged if stage3_ctrl is 0\n" \
	"  13: HDR_I between T5 (L2 Lookup) ~ TE Control\n" \
	"  14: HDR_I between TE Control ~ Packet Editor\n" \
	"  15: HDR_A before Packet Parser\n" \
	"  16: HDR_A after Packet Editor\n" \
	"  17: L3FE 10-bit packet counter\n" \
	"  20: L3FE Latch debugging On\n" \
	"  21: L3FE Latch debugging Off\n" \
	"  22: L3FE Latch HDR_I between PP ~ STG0\n" \
	"  23: L3FE Latch INNER_IP between PP ~ STG0\n" \
	"  24: L3FE Latch HDR_I before PE\n" \
	"  25: L3FE Latch additional drop code\n" \
	"\n"

static int l3fe_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, L3FE_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

static void l3fe_debug_dump_hdr_a(L3FE_HDR_A_t *hdr)
{
	printk(KERN_INFO "<HDR_A>\n");
	printk(KERN_INFO "  cpu_flg: %u, deep_q: %u\n", hdr->cpu_flg, hdr->deep_q);
	printk(KERN_INFO "  pol_grp_id(3): 0x%X, pol_id(9): 0x%03X, pol_en: %u ", hdr->pol_grp_id, hdr->pol_id, hdr->pol_en);
	switch (hdr->pol_en) {
	case 0: printk("[Disabled]\n"); break;
	case 1: printk("[L2 Enabled]\n"); break;
	case 2: printk("[L3 Enabled]\n"); break;
	case 3: printk("[L2 & L3 Enabled]\n"); break;
	default: printk("[INVALID]\n"); break;
	}

	printk(KERN_INFO "  mark: %u, mirror: %u, no_drop: %u, drop_code: %u\n",
		hdr->mark, hdr->mirror, hdr->no_drop, hdr->drop_code);

	printk(KERN_INFO "  rx_pkt_type: %u ", hdr->rx_pkt_type);
	switch (hdr->rx_pkt_type) {
	case 0: printk("[Unicast or Disable L2 Learning]\n"); break;
	case 1: printk("[Broadcast]\n"); break;
	case 2: printk("[Multicast]\n"); break;
	case 3: printk("[Unknown Unicast or Enable L2 Learning]\n"); break;
	}

	printk(KERN_INFO "  hdr_type: %u ", hdr->hdr_type);
	switch (hdr->hdr_type) {
	case 0: printk("[Generic Unicast HDR]\n"); break;
	case 1: printk("[Multicast HDR, MCGID is valid]\n"); break;
	case 2: printk("[CPU bound packet, MCGID contains L2 learning info]\n"); break;
	case 3: printk("[PTP Packet]\n"); break;
	default: printk("[INVALID]\n"); break;
	}

	printk(KERN_INFO "  mcgid(8): 0x%02X, fe_bypass: %u, pkt_size(14): 0x%04X\n",
		hdr->mcgid, hdr->fe_bypass, hdr->pkt_size);
	printk(KERN_INFO "  lspid(6): 0x%02X, ldpid(6): 0x%02X, cos: %u\n", hdr->lspid, hdr->ldpid, hdr->cos);
}

static void l3fe_debug_dump_hdr_i(L3FE_HDR_I_t *hdr, unsigned long where)
{
#if RTK_8277C_HEADER_VERSION == 2
	printk(KERN_INFO "<Table Control>\n");
	printk(KERN_INFO "  t0_ctrl: %u, t1_ctrl(4): 0x%X, t2_ctrl(4): 0x%X, t3_ctrl(4): 0x%X, t4_ctrl(4): 0x%X, t5_ctrl(4): 0x%X\n",
		hdr->t0_ctrl, hdr->t1_ctrl, hdr->t2_ctrl, hdr->t3_ctrl, hdr->t4_ctrl, hdr->t5_ctrl);
	if (hdr->stage2_ctrl == 0) {
		printk(KERN_INFO "  stage2_ctrl: 0 [Merge type-0 and type-1 result of Hash Engine]\n");
	} else {
		printk(KERN_INFO "  stage2_ctrl: 1 [Do NOT merge type-0 result of Hash Engine]\n");
	}
	if (hdr->stage3_ctrl == 0) {
		printk(KERN_INFO "  stage3_ctrl: 0 [Apply type-0 result of Hash Engine]\n");
	} else {
		printk(KERN_INFO "  stage3_ctrl: 1 [Do NOT apply type-0 result of Hash Engine]\n");
	}
	printk(KERN_INFO "  keep_org_pkt: %u, keep_ts: %u, hdr_ptp: %u\n", hdr->keep_org_pkt, hdr->keep_ts, hdr->hdr_ptp);

	// --------------------------- //

	printk(KERN_INFO "<Info for PE module>\n");
	printk(KERN_INFO "  ip_ihl: 0x%X, orig_packet_len(14): 0x%04X, l4_offset(7): 0x%02X, l3_offset(7): 0x%02X, l3_inner_offset(7): 0x%02X\n",
		hdr->ip_ihl, hdr->orig_packet_len, hdr->l4_offset, hdr->l3_offset, hdr->l3_inner_offset);
	printk(KERN_INFO "  l2_orig_hdr_len(7): 0x%02X, l2_payload_len(14): 0x%04X, l3_total_len(14): 0x%04X, l4_rdp_hdr_len(8): 0x%02X\n",
		hdr->l2_orig_hdr_len, hdr->l2_payload_len, hdr->l3_total_len, hdr->l4_rdp_hdr_len);
	printk(KERN_INFO "  l3_outer_popped: %u, six_rd_ipda_from_v6: %u, drop_src(6): 0x%02X\n",
		hdr->l3_outer_popped, hdr->six_rd_ipda_from_v6, hdr->drop_src);

	// --------------------------- //

	printk(KERN_INFO "<CPU Header>\n");
	printk(KERN_INFO "  fwd_vld(9): 0x%03X\n", hdr->fwd_vld);
	printk(KERN_INFO "    [CLS: %s]\n", (hdr->fwd_vld & 0x1) ? "Hit" : "NO Hit");
	printk(KERN_INFO "    [Hash(type-0): ");
	switch ((hdr->fwd_vld >> 1) & 0x3) {
	case 0: printk("NO Hit"); break;
	case 1: printk("Hit Overflow Entry"); break;
	case 2: printk("Hit Cache Hash Entry"); break;
	case 3: printk("Hit Main Hash Entry"); break;
	}
	printk(", Hash(type-1): ");
	switch ((hdr->fwd_vld >> 3) & 0x3) {
	case 0: printk("NO Hit"); break;
	case 1: printk("Hit Overflow Entry"); break;
	case 2: printk("Hit Cache Hash Entry"); break;
	case 3: printk("Hit Main Hash Entry"); break;
	}
	printk("]\n");
	printk(KERN_INFO "    [LPM: %s]\n", (hdr->fwd_vld & 0x20) ? "Hit" : "NO Hit");
	printk(KERN_INFO "    [HashLite(type-0): ");
	switch ((hdr->fwd_vld >> 6) & 0x3) {
	case 0:
		printk("NO Hit"); break;
	case 1:
		printk("Hit Overflow Entry"); break;
	case 2:
	case 3:
		printk("Hit Hash Entry"); break;
	}
	printk(", HashLite(type-1): %s]\n", (hdr->fwd_vld & 0x100) ? "Hit" : "NO Hit");

	printk(KERN_INFO "  cls_action(12): 0x%03X [std_fib: %u, fib_index: %u]\n",
		hdr->cls_action, (hdr->cls_action & 0x400) >> 10, hdr->cls_action & 0x3ff);

	printk(KERN_INFO "  hash_dbl_chk_fail(3): 0x%X\n", hdr->hash_dbl_chk_fail);
	if (hdr->hash_dbl_chk_fail & 0x1) {
		printk(KERN_INFO "    [Hash Engine double check fail (type-0)]\n");
	}
	if (hdr->hash_dbl_chk_fail & 0x2) {
		printk(KERN_INFO "    [Hash Engine double check fail (type-1)]\n");
	}
	if (hdr->hash_dbl_chk_fail & 0x4) {
		printk(KERN_INFO "    [HashLite Engine double check fail]\n");
	}

	printk(KERN_INFO "  hash_idx(21): 0x%06X, cache_way(5): 0x%02X\n", hdr->hash_idx, hdr->cache_way);
	if (((hdr->fwd_vld >> 1) & 0x3) == 1) {
		printk(KERN_INFO "    [overflow_hash_idx(6): 0x%02X]\n", hdr->hash_idx & 0x3f);
	}
	if (((hdr->fwd_vld >> 1) & 0x2) == 2) {
		printk(KERN_INFO "    [hash_idx(16): 0x%04X, hash_profile(4): 0x%X]\n", hdr->hash_idx & 0xffff, (hdr->hash_idx >> 16) & 0xf);
	}
	if ((hdr->fwd_vld >> 5) & 0x1) {
		printk(KERN_INFO "    [lpm_idx(10): 0x%03X]\n", hdr->hash_idx & 0x3ff);
	}
	if (((hdr->fwd_vld >> 6) & 0x3) == 1) {
		printk(KERN_INFO "    [hashlite_overflow_idx(11): 0x%03X]\n", (hdr->hash_idx >> 10) & 0x7ff);
	}
	if (((hdr->fwd_vld >> 6) & 0x2) == 2) {
		printk(KERN_INFO "    [hashlite_idx(11): 0x%03X]\n", (hdr->hash_idx >> 10) & 0x7ff);
	}

	// --------------------------- //

	/* Some HDR_A fields filled by L2FE/NI are manipulated by L3FE.PP temporarily:
	 *   HDR_A.ldpid --> HDR_I.lspid
	 *   HDR_A.lspid --> HDR_I.o_lspid
	 *   HDR_A.mcgid --> HDR_I.mcgid(ldpid)
	 * These manipulated fields are restored to proper fields by STG1.
	 *
	 * If L3FE_STG0_CTRL.lpb_idx_mode is 0, PP uses HDR_A.ldpid to index reg L3FE_STG0_LDPID_MAP.
	 * If L3FE_STG0_CTRL.lpb_idx_mode is 1, PP uses HDR_A.lspid to index reg L3FE_STG0_LPB_IDX_TBL.
	 */
	printk(KERN_INFO "<Source Port ID>\n");
	printk(KERN_INFO "  pspid(4): 0x%X, lspid(6): 0x%02X, o_lspid(6): 0x%02X\n", hdr->pspid, hdr->lspid, hdr->o_lspid);

	// --------------------------- //

	printk(KERN_INFO "<Destination Port ID>\n");
	printk(KERN_INFO "  dpid_pri: %u, mc: %u, mcgid/ldpid(10): 0x%03X, deepq: %u, no_drop: %u, mrr_en: %u\n",
		hdr->dpid_pri, hdr->mc, hdr->mcgid, hdr->deepq, hdr->no_drop, hdr->mrr_en);

	// --------------------------- //

	printk(KERN_INFO "<COS>\n");
	printk(KERN_INFO "  cos: %u\n", hdr->cos);

	// --------------------------- //

	printk(KERN_INFO "<Policer ID>\n");
	printk(KERN_INFO "  pol_grp_id(3): 0x%X, pol_id(9): 0x%03X, pol_en: %u, pol_all_bypass: %u, qos_premark: %u\n",
		hdr->pol_grp_id, hdr->pol_id, hdr->pol_en, hdr->pol_all_bypass, hdr->qos_premark);
	printk(KERN_INFO "  pol2_id_msb(9): 0x%03X, pol2_en: %u, pol3_id(9): 0x%03X, pol3_en: %u\n",
		hdr->pol2_id_msb, hdr->pol2_en, hdr->pol3_id, hdr->pol3_en);

	// --------------------------- //

	printk(KERN_INFO "<MDATA>\n");
	printk(KERN_INFO "  mdata_h(32): 0x%08X, mdata_l(32): 0x%08X\n", hdr->mdata_h, hdr->mdata_l);
	printk(KERN_INFO "    [spcl_pkt_enc(6): 0x%02X, L3_ingress_if_id(8): 0x%02X, L3_egress_if_id(8): 0x%02X, lpm_result(12): 0x%03X, mc_fib_id(8): 0x%02X]\n",
		(hdr->mdata_h >> 26) & 0x3f, (hdr->mdata_l >> 24) & 0xff, (hdr->mdata_l >> 16) & 0xff, hdr->mdata_l & 0xfff, hdr->mdata_l & 0xff);

	// --------------------------- //

	printk(KERN_INFO "<Special Packet>\n");
	printk(KERN_INFO "  spcl_pkt_enc(6): 0x%02X, spcl_pkt_hdr_mtch(8): 0x%02X\n", hdr->spcl_pkt_enc, hdr->spcl_pkt_hdr_mtch);

	// --------------------------- //

	printk(KERN_INFO "<L2>\n");
	printk(KERN_INFO "  l2_pkt_type: %u ", hdr->l2_pkt_type);
	switch (hdr->l2_pkt_type) {
	case 0: printk("[L2 Unicast]\n"); break;
	case 1: printk("[L2 Multicast]\n"); break;
	case 2: printk("[L2 Broadcast]\n"); break;
	default: printk("[Unknown Unicast/Multicast]\n"); break;
	}

	printk(KERN_INFO "  mac_da_ip_mc: %u, mac_da_rng: %u, mac_da_rsvd: %u\n",
		hdr->mac_da_ip_mc, hdr->mac_da_rng, hdr->mac_da_rsvd);
	printk(KERN_INFO "  mac_da: %02X-%02X-%02X-%02X-%02X-%02X, mac_da_an_sel(4): 0x%X\n",
		hdr->mac_da_5, hdr->mac_da_4, hdr->mac_da_3, hdr->mac_da_2, hdr->mac_da_1, hdr->mac_da_0, hdr->mac_da_an_sel);
	printk(KERN_INFO "  mac_sa: %02X-%02X-%02X-%02X-%02X-%02X, ethertype(16): 0x%04X, ethertype_enc(6): 0x%02X\n",
		hdr->mac_sa_5, hdr->mac_sa_4, hdr->mac_sa_3, hdr->mac_sa_2, hdr->mac_sa_1, hdr->mac_sa_0,
		hdr->ethertype, hdr->ethertype_enc);

	// --------------------------- //

	printk(KERN_INFO "<L2 format>\n");
	printk(KERN_INFO "  llc_snap: %u, len_encoded: %u, pktlen_rng_match_vec(4): 0x%X, llc_type_enc: %u\n",
		hdr->llc_snap, hdr->len_encoded, hdr->pktlen_rng_match_vec, hdr->llc_type_enc);

	// --------------------------- //

	printk(KERN_INFO "<VLAN>\n");
	printk(KERN_INFO "  vlan_cnt: %u\n", hdr->vlan_cnt);
	printk(KERN_INFO "  top_tpid_enc: %u, top_vid(12): 0x%03X, top_dei: %u\n", hdr->top_tpid_enc, hdr->top_vid, hdr->top_dei);
	printk(KERN_INFO "  inner_tpid_enc: %u, inner_vid(12): 0x%03X, inner_dei: %u\n", hdr->inner_tpid_enc, hdr->inner_vid, hdr->inner_dei);

	// --------------------------- //

	printk(KERN_INFO "<802.1p>\n");
	printk(KERN_INFO "  top_802_1p: %u, inner_802_1p: %u\n", hdr->top_802_1p, hdr->inner_802_1p);

	// --------------------------- //

	printk(KERN_INFO "<PPP / PPPoE>\n");
	printk(KERN_INFO "  pppoe_type: %u [%s], pppoe_code_enc(4): 0x%X, pppoe_session_id(16): 0x%04X, ppp_protocol_enc(4): 0x%X\n",
		hdr->pppoe_type,
		(hdr->pppoe_type == 0) ? "INVALID" : ((hdr->pppoe_type == 1) ? "PPPoE Discovery" : "PPPoE Session"),
		hdr->pppoe_code_enc, hdr->pppoe_session_id, hdr->ppp_protocol_enc);

	// --------------------------- //

	printk(KERN_INFO "<L3>\n");
	printk(KERN_INFO "  ip_vld: %u, ip_ver: %u [%s], ip_mtu_enc: %u, ip_mtu_en: %u\n",
		hdr->ip_vld, hdr->ip_ver,
		(hdr->ip_ver == 0) ? "IPv4" : "IPv6",
		hdr->ip_mtu_enc, hdr->ip_mtu_en);

	// --------------------------- //

	printk(KERN_INFO "<QoS DSCP>\n");
	printk(KERN_INFO "  ip_dscp(6): 0x%02X, ip_dscp_markdown_en: %u, ip_dscp_marked_down(6): 0x%02X, ip_ecn: %u, ip_ecn_en: %u\n",
		hdr->ip_dscp, hdr->ip_dscp_markdown_en, hdr->ip_dscp_marked_down, hdr->ip_ecn, hdr->ip_ecn_en);

	// --------------------------- //

	printk(KERN_INFO "<IP>\n");
	printk(KERN_INFO "  ip_protocol(8): 0x%02X\n", hdr->ip_protocol);
	printk(KERN_INFO "  ip_l4_type(3): %u ", hdr->ip_l4_type);
	switch (hdr->ip_l4_type) {
	case 1: printk("[TCP]\n"); break;
	case 2: printk("[UDP]\n"); break;
	case 3: printk("[UDP-Lite]\n"); break;
	case 4: printk("[RDPv1]\n"); break;
	case 5: printk("[RDPv2]\n"); break;
	default: printk("[L4 INVALID]\n"); break;
	}

	printk(KERN_INFO "  ip_sa: %08X-%08X-%08X-%08X, ip_sa_enc(4): 0x%X\n",
		hdr->ip_sa_3, hdr->ip_sa_2, hdr->ip_sa_1, hdr->ip_sa_0, hdr->ip_sa_enc);
	printk(KERN_INFO "  ip_da: %08X-%08X-%08X-%08X, ip_da_enc(4): 0x%X, ip_da_mc: %u, ip_da_sa_equal: %u\n",
		hdr->ip_da_3, hdr->ip_da_2, hdr->ip_da_1, hdr->ip_da_0, hdr->ip_da_enc, hdr->ip_da_mc, hdr->ip_da_sa_equal);
	printk(KERN_INFO "  ipv6_flow_lbl(20): 0x%05X, ip_ttl(8): 0x%02X, ip_options: %u, ip_fragment: %u\n",
		hdr->ipv6_flow_lbl, hdr->ip_ttl, hdr->ip_options, hdr->ip_fragment);
	printk(KERN_INFO "  ipv6_ndp: %u, ipv6_hbh: %u, ipv6_rh: %u, ipv6_doh: %u\n",
		hdr->ipv6_ndp, hdr->ipv6_hbh, hdr->ipv6_rh, hdr->ipv6_doh);

	printk(KERN_INFO "  icmp_vld(3): %u ", hdr->icmp_vld);
	switch (hdr->icmp_vld) {
	case 3: printk("[ICMPv6 NDP]"); break;
	case 4: printk("[ICMPv4]"); break;
	case 5: printk("[ICMPv6]"); break;
	case 6: printk("[IGMP]"); break;
	case 7: printk("[MLD]"); break;
	default: printk("[INVALID]"); break;
	}
	printk(", icmp_type(8): 0x%02X\n", hdr->icmp_type);

	printk(KERN_INFO "  spi_vld(3): %u ", hdr->spi_vld);
	switch (hdr->spi_vld) {
	case 1: printk("[IPSec AH]"); break;
	case 2: printk("[IPSec ESP]"); break;
	case 3: printk("[L2TP over IP, control]"); break;
	case 4: printk("[L2TP over IP, data]"); break;
	case 5: printk("[L2TP over UDP, control]"); break;
	case 6: printk("[L2TP over UDP, data]"); break;
	default: printk("[INVALID]"); break;
	}
	printk(", spi/l2tp_session_id(32): 0x%08X\n", hdr->spi);

	printk(KERN_INFO "  l3_chksum(16): 0x%04X, l3_chksum_err: %u\n", hdr->l3_chksum, hdr->l3_chksum_err);

	// --------------------------- //

	printk(KERN_INFO "<L4>\n");
	printk(KERN_INFO "  l4_sp(16): 0x%04X, l4_dp(16): 0x%04X, l4_port_rng_match_vec(32): 0x%08X, tcp_rdp_ctrl(9): 0x%03X\n",
		hdr->l4_sp, hdr->l4_dp, hdr->l4_port_rng_match_vec, hdr->tcp_rdp_ctrl);
	printk(KERN_INFO "  l4_chksum_zero: %u, l4_chksum(32): 0x%08X\n", hdr->l4_chksum_zero, hdr->l4_chksum);
#elif RTK_8277C_HEADER_VERSION == 7
	printk(KERN_INFO "<Table Control>\n");
	printk(KERN_INFO "	t0_ctrl: %u, t1_ctrl(4): 0x%X, t2_ctrl(4): 0x%X\n", hdr->t0_ctrl, hdr->t1_ctrl, hdr->t2_ctrl);
	printk(KERN_INFO "	igr_l3_if_idx(6): %u, egr_l3_if_idx(6): %u\n",	hdr->igr_l3_if_idx, hdr->egr_l3_if_idx);

	if (hdr->stage2_ctrl == 0) {
		printk(KERN_INFO "	stage2_ctrl: 0 [Merge type-0 and type-1 result of Hash Engine]\n");
	} else {
		printk(KERN_INFO "	stage2_ctrl: 1 [Do NOT merge type-0 result of Hash Engine]\n");
	}

	printk(KERN_INFO "	modify_vlan_only: %u\n");

	printk(KERN_INFO "	keep_org_pkt: %u, keep_ts: %u, hdr_ptp: %u\n", hdr->keep_org_pkt, hdr->keep_ts, hdr->hdr_ptp);

	// --------------------------- //

	printk(KERN_INFO "<Info for PE module>\n");
	printk(KERN_INFO "orig_packet_len(14): 0x%04X, l4_offset(8): 0x%02X, l3_offset(8): 0x%02X, l3_inner_offset(8): 0x%02X\n",
		hdr->orig_packet_len, hdr->l4_offset, hdr->l3_offset, hdr->l3_inner_offset);
	printk(KERN_INFO "	l2_outer_hdr_len(7): 0x%02X, l2_orig_hdr_len(7): 0x%02X, l3_total_len(14): 0x%04X, l4_rdp_hdr_len(8): 0x%02X\n",
		hdr->l2_outer_hdr_len, hdr->l2_orig_hdr_len, hdr->l3_total_len, hdr->l4_rdp_hdr_len);
	printk(KERN_INFO "	l3_outer_popped: %u, drop_src(6): 0x%02X\n",
		hdr->l3_outer_popped, hdr->drop_src);

	// --------------------------- //

	printk(KERN_INFO "<CPU Header>\n");
	printk(KERN_INFO "	fwd_vld(9): 0x%03X\n", hdr->fwd_vld);
	printk(KERN_INFO "	  [CLS: %s]\n", (hdr->fwd_vld & 0x1) ? "Hit" : "NO Hit");
	printk(KERN_INFO "	  [Hash(type-0): ");
	switch ((hdr->fwd_vld >> 1) & 0x3) {
	case 0: printk("NO Hit"); break;
	case 1: printk("Hit Overflow Entry"); break;
	case 2: printk("Hit Cache Hash Entry"); break;
	case 3: printk("Hit Main Hash Entry"); break;
	}
	printk(", Hash(type-1): ");
	switch ((hdr->fwd_vld >> 3) & 0x3) {
	case 0: printk("NO Hit"); break;
	case 1: printk("Hit Overflow Entry"); break;
	case 2: printk("Hit Cache Hash Entry"); break;
	case 3: printk("Hit Main Hash Entry"); break;
	}
	printk("]\n");
	printk(KERN_INFO "	cls_action(12): 0x%03X [std_fib: %u, fib_index: %u]\n",
		hdr->cls_action, (hdr->cls_action & 0x400) >> 10, hdr->cls_action & 0x3ff);

	printk(KERN_INFO "	hash_dbl_chk_fail(3): 0x%X\n", hdr->hash_dbl_chk_fail);
	if (hdr->hash_dbl_chk_fail & 0x1) {
		printk(KERN_INFO "	  [Hash Engine double check fail (type-0)]\n");
	}
	if (hdr->hash_dbl_chk_fail & 0x2) {
		printk(KERN_INFO "	  [Hash Engine double check fail (type-1)]\n");
	}

	printk(KERN_INFO "	hash_idx(21): 0x%06X, cache_way(5): 0x%02X\n", hdr->hash_idx, hdr->cache_way);
	if (((hdr->fwd_vld >> 1) & 0x3) == 1) {
		printk(KERN_INFO "	  [overflow_hash_idx(6): 0x%02X]\n", hdr->hash_idx & 0x3f);
	}
	if (((hdr->fwd_vld >> 1) & 0x2) == 2) {
		printk(KERN_INFO "	  [hash_idx(16): 0x%04X, hash_profile(4): 0x%X]\n", hdr->hash_idx & 0xffff, (hdr->hash_idx >> 16) & 0xf);
	}
	// --------------------------- //

	/* Some HDR_A fields filled by L2FE/NI are manipulated by L3FE.PP temporarily:
	 *	 HDR_A.ldpid --> HDR_I.lspid
	 *	 HDR_A.lspid --> HDR_I.o_lspid
	 *	 HDR_A.mcgid --> HDR_I.mcgid(ldpid)
	 * These manipulated fields are restored to proper fields by STG1.
	 *
	 * If L3FE_STG0_CTRL.lpb_idx_mode is 0, PP uses HDR_A.ldpid to index reg L3FE_STG0_LDPID_MAP.
	 * If L3FE_STG0_CTRL.lpb_idx_mode is 1, PP uses HDR_A.lspid to index reg L3FE_STG0_LPB_IDX_TBL.
	 */
	printk(KERN_INFO "<Source Port ID>\n");
	printk(KERN_INFO "	pspid(4): 0x%X, lspid(6): 0x%02X, o_lspid(6): 0x%02X\n", hdr->pspid, hdr->lspid, hdr->o_lspid);

	// --------------------------- //

	printk(KERN_INFO "<Destination Port ID>\n");
	printk(KERN_INFO "	dpid_pri: %u, mc: %u, mcgid/ldpid(10): 0x%03X, deepq: %u, no_drop: %u, mrr_en: %u\n",
		hdr->dpid_pri, hdr->mc, hdr->mcgid, hdr->deepq, hdr->no_drop, hdr->mrr_en);

	// --------------------------- //

	printk(KERN_INFO "<COS>\n");
	printk(KERN_INFO "	cos: %u\n", hdr->cos);

	// --------------------------- //

	printk(KERN_INFO "<Policer ID>\n");
	printk(KERN_INFO "	pol_grp_id(3): 0x%X, pol_id(8): 0x%02X, pol_en(2): %u, pol_all_bypass: %u, qos_premark: %u\n",
		hdr->pol_grp_id, hdr->pol_id, hdr->pol_en, hdr->pol_all_bypass, hdr->qos_premark);
	printk(KERN_INFO "  pol2_id_msb(3): 0x%X, pol2_en: %u, pol3_id(6): 0x%02X, pol3_en: %u\n",
		hdr->pol2_id_msb, hdr->pol2_en, hdr->pol3_id, hdr->pol3_en);

	// --------------------------- //

	printk(KERN_INFO "<MDATA>\n");
	printk(KERN_INFO "	mdata_h(32): 0x%08X, mdata_l(32): 0x%08X\n", hdr->mdata_h, hdr->mdata_l);
	printk(KERN_INFO "    reason_code(4): 0x%X, SSID(from/to wifi/)orStreamID(from PON)(8): 0x%02X\n", (hdr->mdata_l >> 8) & 0xf,	hdr->mdata_l & 0xff);
	printk(KERN_INFO "    === Before T_2 === \n");
	printk(KERN_INFO "    pkt_type(4): 0x%X", (hdr->mdata_h >> 16) & 0xf);
	switch((hdr->mdata_h >> 16) & 0xf) {
		case 1: printk("[AH]"); break;
		case 2: printk("[ESP]"); break;
		case 3: printk("[L2TP_over_IP(ctrl)]"); break;
		case 4: printk("[L2TP_over_IP(data)]"); break;
		case 5: printk("[[L2TP_over_UDP(ctrl)]]"); break;
		case 6: printk("[[L2TP_over_UDP(data)]]"); break;
		case 7: printk("[VxLAN]"); break;
		case 8: printk("[PPTP GRE]"); break;
		case 9: printk("[IPv4 in IPv6]"); break;
		case 10: printk("[IPv6 in IPv4]"); break;
		case 11: printk("[IPv4 only]"); break;
		case 12: printk("[IPv6 only]"); break;
		default: printk("[rsvd]"); break;
	}
	printk(", dual_hdr_info(32): 0x%08X", ((hdr->mdata_h & 0xffff) << 16) | ((hdr->mdata_h >> 16) & 0xffff));
	switch((hdr->mdata_h >> 16) & 0xf) {
		case 1: printk("[Security Parameter Index(32)]\n"); break;
		case 2: printk("[Security Parameter Index(32)]\n"); break;
		case 3: printk("[Session ID(32)]\n"); break;
		case 4: printk("[Session ID(32)]\n"); break;
		case 5: printk("[Session ID(16), Tunnel ID(16)]\n"); break;
		case 6: printk("[Session ID(16), Tunnel ID(16)]\n"); break;
		case 7: printk("[rsvd(8), VNI(24)]\n"); break;
		case 8: printk("[rsvd(16), call ID(16)]\n"); break;
		default: printk("[rsvd(32)]\n"); break;
	}
	printk(KERN_INFO "    === After T_2=== \n");
	printk(KERN_INFO "    CRC32(32): 0x%08X, CRC16(16): 0x%04X\n", hdr->mdata_h, (hdr->mdata_l >> 16) & 0xffff);

	// --------------------------- //

	printk(KERN_INFO "<Special Packet>\n");
	printk(KERN_INFO "	spcl_pkt_enc(6): 0x%02X, spcl_pkt_hdr_mtch(8): 0x%02X\n", hdr->spcl_pkt_enc, hdr->spcl_pkt_hdr_mtch);

	// --------------------------- //

	printk(KERN_INFO "<L2>\n");
	printk(KERN_INFO "	l2_pkt_type: %u ", hdr->l2_pkt_type);
	switch (hdr->l2_pkt_type) {
		case 0: printk("[L2 Unicast]\n"); break;
		case 1: printk("[L2 Multicast]\n"); break;
		case 2: printk("[L2 Broadcast]\n"); break;
		default: printk("[Unknown Unicast/Multicast]\n"); break;
	}

	printk(KERN_INFO "	mac_da_ip_mc: %u, mac_da_rng: %u, mac_da_rsvd: %u\n",
		hdr->mac_da_ip_mc, hdr->mac_da_rng, hdr->mac_da_rsvd);
	printk(KERN_INFO "	mac_da: %02X-%02X-%02X-%02X-%02X-%02X, mac_da_an_sel(4): 0x%X\n",
		hdr->mac_da_5, hdr->mac_da_4, hdr->mac_da_3, hdr->mac_da_2, hdr->mac_da_1, hdr->mac_da_0, hdr->mac_da_an_sel);
	printk(KERN_INFO "	mac_sa: %02X-%02X-%02X-%02X-%02X-%02X, ethertype(16): 0x%04X, ethertype_enc(6): 0x%02X\n",
		hdr->mac_sa_5, hdr->mac_sa_4, hdr->mac_sa_3, hdr->mac_sa_2, hdr->mac_sa_1, hdr->mac_sa_0,
		hdr->ethertype, hdr->ethertype_enc);

	printk(KERN_INFO "	llc_snap(2): %u, len_encoded: %u, pktlen_rng_match_vec(4): 0x%X, llc_type_enc(2): %u\n",
		hdr->llc_snap, hdr->len_encoded, hdr->pktlen_rng_match_vec, hdr->llc_type_enc);

	// --------------------------- //

	printk(KERN_INFO "<VLAN>\n");
	printk(KERN_INFO "	vlan_cnt: %u\n", hdr->vlan_cnt);
	printk(KERN_INFO "	top_tpid_enc(3): %u, top_vid(12): 0x%03X, top_dei: %u\n", hdr->top_tpid_enc, hdr->top_vid, hdr->top_dei);
	printk(KERN_INFO "	inner_tpid_enc(3): %u, inner_vid(12): 0x%03X, inner_dei: %u\n", hdr->inner_tpid_enc, hdr->inner_vid, hdr->inner_dei);

	// --------------------------- //

	printk(KERN_INFO "<802.1p>\n");
	printk(KERN_INFO "	top_802_1p(3): %u, inner_802_1p(3): %u\n", hdr->top_802_1p, hdr->inner_802_1p);

	// --------------------------- //

	printk(KERN_INFO "<PPP / PPPoE>\n");
	printk(KERN_INFO "	pppoe_type: %u [%s], pppoe_code_enc(4): 0x%X, pppoe_session_id(16): 0x%04X, ppp_protocol_enc(4): 0x%X\n",
		hdr->pppoe_type,
		(hdr->pppoe_type == 0) ? "INVALID" : ((hdr->pppoe_type == 1) ? "PPPoE Discovery" : "PPPoE Session"),
		hdr->pppoe_code_enc, hdr->pppoe_session_id, hdr->ppp_protocol_enc);

	// --------------------------- //

	printk(KERN_INFO "<L3>\n");
	printk(KERN_INFO "	ip_vld: %u, ip_ver: %u [%s], ip_mtu_enc(2): %u, ip_mtu_en: %u\n",
		hdr->ip_vld, hdr->ip_ver,
		(hdr->ip_ver == 0) ? "IPv4" : "IPv6",
		hdr->ip_mtu_enc, hdr->ip_mtu_en);

	printk(KERN_INFO "	ip_protocol(8): 0x%02X, ip_ihl(4): 0x%X\n", hdr->ip_protocol, hdr->ip_ihl);
	printk(KERN_INFO "	ip_l4_type(3): %u ", hdr->ip_l4_type);
	switch (hdr->ip_l4_type) {
		case 1: printk("[TCP]\n"); break;
		case 2: printk("[UDP]\n"); break;
		case 3: printk("[UDP-Lite]\n"); break;
		case 4: printk("[RDPv1]\n"); break;
		case 5: printk("[RDPv2]\n"); break;
		default: printk("[L4 INVALID]\n"); break;
	}

	printk(KERN_INFO "	ip_sa: %08X-%08X-%08X-%08X\n",
		hdr->ip_sa_3, hdr->ip_sa_2, hdr->ip_sa_1, hdr->ip_sa_0);
	printk(KERN_INFO "	ip_da: %08X-%08X-%08X-%08X, ip_da_mc: %u, ip_da_sa_equal: %u\n",
		hdr->ip_da_3, hdr->ip_da_2, hdr->ip_da_1, hdr->ip_da_0, hdr->ip_da_mc, hdr->ip_da_sa_equal);
	printk(KERN_INFO "	ipv6_flow_lbl(20): 0x%05X, ip_ttl(8): 0x%02X, ip_options: %u, ip_fragment: %u\n",
		hdr->ipv6_flow_lbl, hdr->ip_ttl, hdr->ip_options, hdr->ip_fragment);
	printk(KERN_INFO "	ipv6_ndp: %u, ipv6_hbh: %u, ipv6_rh: %u, ipv6_doh: %u\n",
		hdr->ipv6_ndp, hdr->ipv6_hbh, hdr->ipv6_rh, hdr->ipv6_doh);

	printk(KERN_INFO "	icmp_vld(3): %u ", hdr->icmp_vld);
	switch (hdr->icmp_vld) {
		case 3: printk("[ICMPv6 NDP]"); break;
		case 4: printk("[ICMPv4]"); break;
		case 5: printk("[ICMPv6]"); break;
		case 6: printk("[IGMP]"); break;
		case 7: printk("[MLD]"); break;
		default: printk("[INVALID]"); break;
	}
	printk(", icmp_type(8): 0x%02X\n", hdr->icmp_type);

	printk(KERN_INFO "	gre_seq_vld: %u , gre_seq(16): 0x%04X\n", hdr->gre_seq_vld, hdr->gre_seq_vld);
	printk(KERN_INFO "	l3_chksum(16): 0x%04X, l3_chksum_err: %u\n", hdr->l3_chksum, hdr->l3_chksum_err);

	// --------------------------- //

	printk(KERN_INFO "<QoS DSCP>\n");
	printk(KERN_INFO "	ip_dscp(6): 0x%02X, ip_dscp_markdown_en: %u, ip_dscp_marked_down(6): 0x%02X, ip_ecn(2): %u, ip_ecn_en: %u\n",
		hdr->ip_dscp, hdr->ip_dscp_markdown_en, hdr->ip_dscp_marked_down, hdr->ip_ecn, hdr->ip_ecn_en);

	// --------------------------- //

	printk(KERN_INFO "<L4>\n");
	printk(KERN_INFO "	l4_sp(16): 0x%04X, l4_dp(16): 0x%04X, l4_port_rng_match_vec(32): 0x%08X\n",
		hdr->l4_sp, hdr->l4_dp, hdr->l4_port_rng_match_vec);
	printk(KERN_INFO "	tcp_rdp_ctrl(9): 0x%03X", hdr->tcp_rdp_ctrl);
	switch (hdr->ip_l4_type) {
		case 1: printk("[TCP] N: %u, C: %u, E: %u, U: %u, A: %u, P: %u, R: %u, S: %u, F: %u\n",
			(hdr->tcp_rdp_ctrl >> 8)&0x1, (hdr->tcp_rdp_ctrl >> 7)&0x1, (hdr->tcp_rdp_ctrl >> 6)&0x1, (hdr->tcp_rdp_ctrl >> 5)&0x1, (hdr->tcp_rdp_ctrl >> 4)&0x1,
			(hdr->tcp_rdp_ctrl >> 3)&0x1, (hdr->tcp_rdp_ctrl >> 2)&0x1, (hdr->tcp_rdp_ctrl >> 1)&0x1, hdr->tcp_rdp_ctrl&0x1);
		break;
		case 4:
		case 5: printk("[RDP S: %u, A: %u, E: %u, R: %u, N: %u, VER: %u]\n",
			(hdr->tcp_rdp_ctrl >> 7)&0x1, (hdr->tcp_rdp_ctrl >> 6)&0x1, (hdr->tcp_rdp_ctrl >> 5)&0x1, (hdr->tcp_rdp_ctrl >> 4)&0x1,
			(hdr->tcp_rdp_ctrl >> 3)&0x1, hdr->tcp_rdp_ctrl&0x3);
		break;
		default: printk("\n"); break;
	}
	printk(KERN_INFO "	l4_chksum_zero: %u, l4_chksum(32): 0x%08X\n", hdr->l4_chksum_zero, hdr->l4_chksum);

#else
#error "unknown RTK_8277C_HEADER_VERSION"
#endif

}

static void l3fe_debug_dump_pkt_count(L3FE_DBG_PKT_COUNT_t *data)
{
	printk(KERN_INFO "L3FE_IN   : %u\n", data->cnt_l3fe_in);
	printk(KERN_INFO "L3FE_OUT  : %u\n", data->cnt_l3fe_out);
	printk(KERN_INFO "T1_T2     : %u\n", data->cnt_t1_t2);
	printk(KERN_INFO "STG3_PE   : %u\n", data->cnt_stage3_pe);
}

static void l3fe_debug_dump_hdr_inner_ip(L3FE_HDR_INNER_IP_t *hdr)
{
	printk(KERN_INFO "<Inner L3>\n");
	printk(KERN_INFO "  ip_vld: %u, ip_ver: %u [%s], ip_ihl: %u, l3_total_len: %u\n",
		hdr->ip_vld, hdr->ip_ver,
		(hdr->ip_ver == 0) ? "IPv4" : "IPv6",
		hdr->ip_ihl, hdr->l3_total_len);

	// --------------------------- //

	printk(KERN_INFO "<Inner QoS DSCP>\n");
	printk(KERN_INFO "  ip_dscp(6): 0x%02X, ip_ecn: %u\n", hdr->ip_dscp, hdr->ip_ecn);

	// --------------------------- //

	printk(KERN_INFO "<Inner IP>\n");
	printk(KERN_INFO "  ip_protocol(8): 0x%02X\n", hdr->ip_protocol);
	printk(KERN_INFO "  ip_sa: %08X-%08X-%08X-%08X\n",
		hdr->ip_sa_3, hdr->ip_sa_2, hdr->ip_sa_1, hdr->ip_sa_0);
	printk(KERN_INFO "  ip_da: %08X-%08X-%08X-%08X, ip_da_mc: %u\n",
		hdr->ip_da_3, hdr->ip_da_2, hdr->ip_da_1, hdr->ip_da_0, hdr->ip_da_mc);
	printk(KERN_INFO "  ipv6_flow_lbl(20): 0x%05X, ip_ttl(8): 0x%02X, ip_options: %u, ip_fragment: %u\n",
		hdr->ipv6_flow_lbl, hdr->ip_ttl, hdr->ip_options, hdr->ip_fragment);
	printk(KERN_INFO "  ipv6_ndp: %u, ipv6_hbh: %u, ipv6_rh: %u, ipv6_doh: %u\n",
		hdr->ipv6_ndp, hdr->ipv6_hbh, hdr->ipv6_rh, hdr->ipv6_doh);
	printk(KERN_INFO "  l3_chksum(16): 0x%04X, l3_chksum_err: %u\n", hdr->l3_chksum, hdr->l3_chksum_err);
}

static void l3fe_debug_dump_stage3_info(L3FE_DBG_STG3_INFO_t *stag3_info)
{
	printk(KERN_INFO "stage3_pe_hdr_a_mark: %d\n", stag3_info->hdr_a_mark);
	printk(KERN_INFO "stage3_pe_hdr_a_drop_code: %d\n", stag3_info->hdr_a_drop_code);
}

int l3fe_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l3fe_debug_show_help, (void *) file);
}

ssize_t __l3fe_debug_proc_write(ca_device_id_t device_id, struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long where;
	ssize_t len;
	L3FE_HDR_A_t hdr_a;
	L3FE_HDR_I_t hdr_i;
	L3FE_HDR_INNER_IP_t hdr_inner_ip;
	L3FE_DBG_PKT_COUNT_t l3fe_pkt_cnt;
	cls_fib_dbg_t cls_fib_dbg;
	L3FE_DBG_STG3_INFO_t stag3_info;
	int ret = AAL_E_OK;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _l3fe_debug_invalid;

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &where))
#else
	if (kstrtoul(tmp, 0, &where))
#endif
		goto _l3fe_debug_invalid;

	switch (where) {
	/* The following dumps from L3FE_GLB_CLS_STG_MONITOR_CTRL */
	case 0:
	case 1:
	case 2:
	case 5:
		if ((ret = aal_l3fe_glb_cls_stg_monitor_get(where, &hdr_i, sizeof(hdr_i))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_i(&hdr_i, where);
		break;

	case 3:
		if ((ret = aal_l3fe_glb_cls_stg_monitor_get(where, &cls_fib_dbg, sizeof(cls_fib_dbg_t))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_cls_fib(&cls_fib_dbg);
		break;

	case 4:
		memset(&hdr_inner_ip, 0, sizeof(hdr_inner_ip));
		if ((ret = aal_l3fe_glb_cls_stg_monitor_get(where, &hdr_inner_ip, sizeof(hdr_inner_ip))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_inner_ip(&hdr_inner_ip);
		break;

	/* The following dumps from L3FE_GLB_DBG_IDX */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		if ((ret = aal_l3fe_glb_dbg_get(where - 6, &hdr_i, sizeof(hdr_i))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_i(&hdr_i, where);
		break;

	case 15: /* dump from L3FE_PP_HEADER_A_* */
		if ((ret = aal_l3fe_pp_header_a_get(&hdr_a)) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_a(&hdr_a);
		break;

	case 16:
		if ((ret = aal_l3fe_glb_dbg_get(14, &hdr_a, sizeof(hdr_a))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_a(&hdr_a);
		break;

	case 17:
		if ((ret = aal_l3fe_glb_dbg_get(15, &l3fe_pkt_cnt, sizeof(l3fe_pkt_cnt))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_pkt_count(&l3fe_pkt_cnt);
		break;

	case 20:
		// turn on l3fe debug latch
		if ((ret = aal_l3fe_glb_dbg_latch_trigger_set(CA_TRUE)) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		break;
	case 21:
		// turn oFF l3fe debug latch
		if ((ret = aal_l3fe_glb_dbg_latch_trigger_set(CA_FALSE)) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		break;
	case 22:
		// dump 3'h0: hdr-i at the PP to STG0 interface
		if ((ret = aal_l3fe_glb_dbg_latch_monitor_get(LATCH_MONITOR_HDR_I_PP_STG0, &hdr_i, sizeof(hdr_i))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_i(&hdr_i, where);
		break;
	case 23:
		// dump 3'h1: inner IP header at the PP to STG0 interface
		if ((ret = aal_l3fe_glb_dbg_latch_monitor_get(LATCH_MONITOR_INNER_IP_PP_STG0, &hdr_inner_ip, sizeof(hdr_inner_ip))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_inner_ip(&hdr_inner_ip);
		break;
	case 24:
		// dump 3'h2: hdr-i before PE(Packet Edit)
		if ((ret = aal_l3fe_glb_dbg_latch_monitor_get(LATCH_MONITOR_HDR_I_BEFORE_PE, &hdr_i, sizeof(hdr_i))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_hdr_i(&hdr_i, where);
		break;	
	case 25:
		// dump 3'h3: additional debug information before PE(Packet Edit)--{stage3_pe_hdr_a_mark, stage3_pe_hdr_a_drop_code[2:0]}
		if ((ret = aal_l3fe_glb_dbg_latch_monitor_get(LATCH_MONITOR_DROP_CODE_PE, &stag3_info, sizeof(stag3_info))) != AAL_E_OK)
			goto _l3fe_debug_invalid;
		l3fe_debug_dump_stage3_info(&stag3_info);
		break;

	default:
		goto _l3fe_debug_invalid;
	}

	return count;

_l3fe_debug_invalid:
	pr_err("ERROR! Invalid value '%s' for %s. (ret=%d)\n", tmp, file->f_path.dentry->d_iname, ret);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

ssize_t l3fe_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return __l3fe_debug_proc_write(0, file, buf, count, pos);
}

static const struct file_operations fops_l3fe_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= l3fe_debug_proc_open,
	.read		= seq_read,
	.write		= l3fe_debug_proc_write,
};

/***************************************************************************
 * l3qm_debug
 ***************************************************************************/

ca_status_t aal_l3qm_dump_empty_buffers(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_epps(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_rmu_fe_drop_counter(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_empty_buffers_phy_addr(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_cpu64_fifo_read_ptr(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_cpu64_fifo_write_ptr(ca_device_id_t device_id);
ca_uint32_t aal_l3qm_dump_cpu64_fifo_paddr_start(ca_device_id_t device_id);
ca_uint32_t aal_l3qm_dump_monitor_eq(ca_device_id_t device_id);
ca_status_t aal_l3qm_dump_axi_attribs(ca_device_id_t device_id);
ca_status_t aal_l3qm_check_bid_start(ca_device_id_t device_id);
ca_status_t aal_l3qm_check_epps(ca_device_id_t device_id);

ca_status_t aal_l3qm_cpu256_drop_counter_dump(ca_uint16_t counter_id);
void aal_l3qm_cpu256_drop_counter_map_dump(void);
ca_status_t aal_l3qm_cpu256_drop_counter_clear(ca_uint16_t counter_id);
void ca_l3te_dump_token_counter(ca_uint32_t shaper_id);
void aal_l3qm_cpu256_fifo_dump(void);
void aal_l3qm_cpu256a_fifo_drop_profile_dump(void);
ca_status_t ca_l3te_dump_shaper(int shaper_id);
ca_status_t ca_l3te_subtract_token(ca_uint32_t shaper_id, ca_boolean_t pir_flag, ca_uint32_t token_count);

#define L3QM_DEBUG_HELP_MSG \
	"Purpose: Debug packet in L3QM\n" \
	"\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo VALUE > %s\n" \
	"  0: Dump Empty buffer queues settings for L3QM\n" \
	"  1: Dump EPP settings for L3QM\n" \
	"  2: Dump RMU FE drop counter for L3QM\n" \
	"  3: Dump Empty buffer queues physical addresses for L3QM\n" \
	"  4: Dump CPUEPP64 FIFO read pointer for L3QM\n" \
	"  5: Dump CPUEPP64 FIFO write pointer for L3QM\n" \
	"  6: Dump CPUEPP64 FIFO paddr start for L3QM\n" \
	"  7: Dump QM_QM_MONITOR_EQ for L3QM\n" \
	"  8: Dump QM_QM_AXI_ATTRIBUTE_DATA0/1 for L3QM\n" \
	"  9: Check the bid_start overlapped for L3QM\n" \
	"  10: Check the EPP magic number for L3QM\n" \
	"  11: (Venus only) Dump EPP256 drop-counter array for CPU256, syntax: echo 11 {drop_index} > l3qm_debug\n" \
	"  12: (Venus only) Dump EPP256 to 64 DROP-COUNTER Mapping Memory for CPU256\n" \
	"  13: (Venus only) Clear EPP256 drop-counter for CPU256, syntax: echo 13 {drop_index} >  l3qm_debug\n" \
	"  14: (Venus only) Dump TE TB2RATE shaper, syntax: echo 14 {shaper_id} >  l3qm_debug\n" \
	"  15: (Venus only) Dump TE TB2RATE counter, syntax: echo 15 {shaper_id} >  l3qm_debug\n" \
	"  16: (Venus only) Subtract TE TB2RATE counter, syntax: echo 16 {shaper_id} {token count}  >  l3qm_debug\n" \
	"  17: (Venus only) Dump CPU EPP256 FIFO profile selection Register\n" \
	"  18: (Venus only) Dump CPU EPP256 FIFO configuration Register\n" \
	"\n"

static int l3qm_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, L3QM_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int l3qm_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, l3qm_debug_show_help, (void *) file);
}

ssize_t l3qm_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	char *token, *cp = tmp;
	ssize_t len;
	int ret = AAL_E_OK;
	unsigned long index, drop_index, shaper_id;
	unsigned long token_count;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _l3qm_debug_invalid;

	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtoul(token, 0, &index))
		{
			printk(KERN_ERR "ERROR! Invalid 1st value '%s'.\n", token);
			goto _l3qm_debug_invalid;
		}
	} else {
		goto _l3qm_debug_invalid;
	}

	switch (index) {
		case 0:
			ret = aal_l3qm_dump_empty_buffers(0);
			break;
		case 1:
			ret = aal_l3qm_dump_epps(0);
			break;
		case 2:
			ret = aal_l3qm_dump_rmu_fe_drop_counter(0);
			break;
		case 3:
			ret = aal_l3qm_dump_empty_buffers_phy_addr(0);
			break;
		case 4:
			ret = aal_l3qm_dump_cpu64_fifo_read_ptr(0);
			break;
		case 5:
			ret = aal_l3qm_dump_cpu64_fifo_write_ptr(0);
			break;
		case 6:
			ret = aal_l3qm_dump_cpu64_fifo_paddr_start(0);
			break;
		case 7:
			ret = aal_l3qm_dump_monitor_eq(0);
			break;
		case 8:
			ret = aal_l3qm_dump_axi_attribs(0);
			break;
		case 9:
			ret = aal_l3qm_check_bid_start(0);
			break;
		case 10:
			ret = aal_l3qm_check_epps(0);
			break;
#if defined(CONFIG_NE_CPU256)
		case 11:
		case 13:
			/* parse 2nd token */
			token = strsep(&cp, " \n");
			if (token != NULL) {
				if (kstrtoul(token, 0, &drop_index))
				{
					printk(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
					goto _l3qm_debug_invalid;
				}
			} else {
				goto _l3qm_debug_invalid;
			}

			if (index == 11) {
				aal_l3qm_cpu256_drop_counter_dump((ca_uint16_t)drop_index);
			}
			else {
				aal_l3qm_cpu256_drop_counter_clear((ca_uint16_t)drop_index);
			}
			break;
		case 12:
			aal_l3qm_cpu256_drop_counter_map_dump();
			break;
		case 14:
		case 15:
		case 16:
			/* parse 2nd token */
			token = strsep(&cp, " \n");
			if (token != NULL) {
				if (kstrtoul(token, 0, &shaper_id))
				{
					printk(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
					goto _l3qm_debug_invalid;
				}
			} else {
				goto _l3qm_debug_invalid;
			}
			if (index == 14) {
				ca_l3te_dump_shaper(shaper_id);
			}
			else if (index == 15) {
				ca_l3te_dump_token_counter(shaper_id);
			}
			else {
				printk("%s: shaper_id=%lu\n", __func__, shaper_id);
				/* parse 3rd token */
				token = strsep(&cp, " \n");
				if (token != NULL) {
					if (kstrtoul(token, 0, &token_count))
					{
						printk(KERN_ERR "ERROR! Invalid 3rd value '%s'.\n", token);
						goto _l3qm_debug_invalid;
					}
				} else {
					goto _l3qm_debug_invalid;
				}
				printk("%s: token_count=%lu\n", __func__, token_count);
				ca_l3te_subtract_token((ca_uint32_t)shaper_id, 0, (ca_uint32_t)token_count);
			}
			break;
		case 17:
			aal_l3qm_cpu256_fifo_dump();
			break;
		case 18:
			aal_l3qm_cpu256a_fifo_drop_profile_dump();
			break;
#endif
		default:
			goto _l3qm_debug_invalid;
	}
	if (ret == AAL_E_OK)
		return count;

_l3qm_debug_invalid:
	pr_err("\nERROR! Invalid value '%s' for %s. (ret=%d)\n", tmp, file->f_path.dentry->d_iname, ret);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_l3qm_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= l3qm_debug_proc_open,
	.read		= seq_read,
	.write		= l3qm_debug_proc_write,
};

/***************************************************************************
 * ni_debug
 ***************************************************************************/

void aal_ni_rx_l3fe_demux_dump_registers(void);
void aal_ni_dma_lso_dump_registers(void);

ca_status_t aal_ni_dma_lso_dump_aft_l2fib(int index);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op);
ca_status_t aal_ni_set_dma_lso_aft_l2fib_pppoe(int index, ca_uint16_t session_id, ca_uint8_t op);
ca_status_t aal_ni_dma_lso_clean_aft_l2fib(int index);

#define NI_DEBUG_HELP_MSG \
	"Purpose: Debug packet in NI\n" \
	"\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo VALUE > %s\n" \
	"  0: Dump NI RX L3FE DEMUX CFG settings\n" \
	"  1: Dump DMA LSO regsiters settings\n" \
	"  2: (Venus only)Dump DMA LSO AFT L2FIB table, echo 2 1 > ni_debug \n" \
	"  3: (Venus only)Set DMA LSO AFT L2FIB table top vlan, echo 3 1 100 1 > ni_debug \n" \
	"  4: (Venus only)Set DMA LSO AFT L2FIB table inner vlan, echo 4 1 100 1 > ni_debug \n" \
	"  5: (Venus only)Set DMA LSO AFT L2FIB table top dei/8021p, echo 5 1 0xf 1 > ni_debug \n" \
	"  6: (Venus only)Set DMA LSO AFT L2FIB table inner dei/8021p, echo 6 1 0xf 1 > ni_debug \n" \
	"  7: (Venus only)Set DMA LSO AFT L2FIB table top tpid, echo 7 1 1 1 > ni_debug \n" \
	"  8: (Venus only)Set DMA LSO AFT L2FIB table inner tpid, echo 8 1 1 1 > ni_debug \n" \
	"  9: (Venus only)Set DMA LSO AFT L2FIB table pppoe session id, echo 9 1 0x1234 1 > ni_debug \n" \
	" 10: (Venus only)Clean DMA LSO AFT L2FIB table, echo 10 1 > ni_debug \n" \
	"\n"

static int ni_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, NI_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

static int ni_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ni_debug_show_help, (void *) file);
}

static ca_status_t ni_debug_get_dma_aft_para(char *cp, unsigned long *index, unsigned long *vlan_id, unsigned long *op)
{
	char *token;

	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtoul(token, 0, index))
		{
			printk(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
			return CA_E_ERROR;
		}
	} else {
		return CA_E_ERROR;
	}
	/* parse 3rd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtoul(token, 0, vlan_id))
		{
			printk(KERN_ERR "ERROR! Invalid 3rd value '%s'.\n", token);
			return CA_E_ERROR;
		}
	} else {
		return CA_E_ERROR;
	}
	/* parse 4th token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if (kstrtoul(token, 0, op))
		{
			printk(KERN_ERR "ERROR! Invalid 3rd value '%s'.\n", token);
			return CA_E_ERROR;
		}
	} else {
		return CA_E_ERROR;
	}

	printk("%s: index=%lu, vlan_id=%lu, op=%lu\n", __func__, *index, *vlan_id, *op);
	return CA_E_OK;
}

static ssize_t ni_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	int ret = AAL_E_OK;
	unsigned long where;
	unsigned long index;
	char *cp=tmp, *token;
	unsigned long vlan_id, op;

        len = min(count, (unsigned long)(sizeof(tmp) - 1));
        if (copy_from_user(tmp, buf, len))
                goto _ni_debug_invalid;

        tmp[len] = '\0';

        /* parse 1st token */
        token = strsep(&cp, " \n");
        if (token != NULL) {
                        if (kstrtoul(token, 0, &where))
                        {
                                printk(KERN_ERR "ERROR! Invalid 1st value '%s'.\n", token);
                                goto _ni_debug_invalid;
                        }
        } else {
                goto _ni_debug_invalid;
        }

	switch (where) {
		case 0:
			aal_ni_rx_l3fe_demux_dump_registers();
			ret = AAL_E_OK;
			break;
		case 1:
			aal_ni_dma_lso_dump_registers();
			ret = AAL_E_OK;
			break;
		case 2:
		case 10:
			/* parse 2nd token */
			token = strsep(&cp, " \n");
			printk("%s: token=0x%p\n", __func__, token);
			if (token != NULL) {
				if (kstrtoul(token, 0, &index))
				{
					printk(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
					goto _ni_debug_invalid;
				}
			} else {
				goto _ni_debug_invalid;
			}
			if (where == 2) {
				aal_ni_dma_lso_dump_aft_l2fib(index);
			}
			else {
				aal_ni_dma_lso_clean_aft_l2fib(index);
			}
			break;

		case 3:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_top_vlan((int)index, (ca_vlan_id_t)vlan_id, (ca_uint8_t)op);
			break;
		case 4:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_inner_vlan((int)index, (ca_vlan_id_t)vlan_id, (ca_uint8_t)op);
			break;
		case 5:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_top_vlan_8021p_dei((int)index, (ca_uint8_t)vlan_id, (ca_uint8_t)op);
			break;
		case 6:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_inner_vlan_8021p_dei((int)index, (ca_uint8_t)vlan_id, (ca_uint8_t)op);
			break;

		case 7:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_top_tpid((int)index, (ca_uint8_t)vlan_id, (ca_uint8_t)op);
			break;
		case 8:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;
			aal_ni_set_dma_lso_aft_l2fib_inner_tpid((int)index, (ca_uint8_t)vlan_id, (ca_uint8_t)op);
			break;

		case 9:
			if (ni_debug_get_dma_aft_para(cp, &index, &vlan_id, &op) != CA_E_OK)
				goto _ni_debug_invalid;

			aal_ni_set_dma_lso_aft_l2fib_pppoe((int)index, (ca_uint16_t)vlan_id, (ca_uint8_t)op);
				break;
		default:
			goto _ni_debug_invalid;
	}
	if (ret == AAL_E_OK)
		return count;

_ni_debug_invalid:
	pr_err("\nERROR! Invalid value '%s' for %s. (ret=%d)\n", tmp, file->f_path.dentry->d_iname, ret);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_ni_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= ni_debug_proc_open,
	.read		= seq_read,
	.write		= ni_debug_proc_write,
};

/***************************************************************************
 * aal_debug
 ***************************************************************************/

#define AAL_DEBUG_HELP_MSG \
	"Purpose: Enable NI driver AAL layer debug message\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: AAL layer NI debug message\n" \
	"  0x00000002: AAL layer L3QM debug message\n" \
	"  0x00000004: AAL layer L3FE debug message\n" \
	"  0x00000008: AAL layer L3CLS debug message\n" \
	"  0x00000010: AAL layer HashLite debug message\n" \
	"  0x00000020: AAL layer Hash debug message\n" \
	"  0x00000040: AAL layer Multicast debug message\n" \
	"  0x00000080: AAL layer DMA LSO debug message\n" \
	"  0x00000100: AAL layer XFI debug message\n" \
	"  0x00000200: AAL layer PSDS debug message\n" \
	"  0x00000400: AAL layer PHY debug message\n" \
	"  0x00000800: AAL layer MDIO debug message\n" \
	"  0x00001000: AAL layer SCFG debug message\n" \
	"  0x00002000: AAL layer NI L2 debug message\n" \
	"\n"

static int aal_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, AAL_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_aal_debug);

	return 0;
}

int aal_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aal_debug_show_help, (void *) file);
}

ssize_t aal_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long mask;
	ssize_t len;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _aal_debug_invalid;

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &mask))
#else
	if (kstrtoul(tmp, 0, &mask))
#endif
		goto _aal_debug_invalid;

	ca_aal_debug = mask;
	pr_info("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_aal_debug);

	return count;

_aal_debug_invalid:
	pr_err("ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_aal_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= aal_debug_proc_open,
	.read		= seq_read,
	.write		= aal_debug_proc_write,
};

/***************************************************************************
 * aal_table
 ***************************************************************************/

#define AAL_TABLE_HELP_MSG \
	"Purpose: Dump AAL table\n" \
	"\n" \
	"Usage: Dump the TABLE which entry range is IDX to IDX+NUM-1:\n" \
	"  echo TABLE IDX [NUM] > %s\n" \
	"\n" \
	"Example: For table 3, dump index 5.\n" \
	"  echo 3 5 > %s\n" \
	"\n" \
	"Example: For table 3, dump 10 entries from index 5.\n" \
	"  echo 3 5 10 > %s\n" \
	"\n"

static int aal_table_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;
	int i;
	int used_count = 0, max_count = 0;

	seq_printf(m, AAL_TABLE_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "Current available TABLE: (used_entry / max_entry)\n");
	for (i = 0; i < AAL_TABLE_MAX; i++) {
		used_count = aal_table_get_used_count(i);
		max_count = aal_table_get_max_count(i);

		seq_printf(m, " %2d: %s ", i, aal_table_name_get(i));
		if ((used_count < 0) || (max_count < 0))
			seq_printf(m, "\n");
		else
			seq_printf(m, "(%d / %d)\n", used_count, max_count);

	}
	seq_printf(m, "\n");

	return 0;
}

int aal_table_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aal_table_show_help, (void *) file);
}

ssize_t aal_table_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	unsigned long table_id = 0;
	unsigned long start = 0, num = 1;
	int ret;
	int used_count = 0, max_count = 0;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _aal_table_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
		if (strict_strtoul(token, 0, &table_id))
#else
		if (kstrtoul(token, 0, &table_id))
#endif
		{
			printk(KERN_ERR "ERROR! Invalid 1st value '%s'.\n", token);
			goto _aal_table_invalid;
		}
	} else {
		goto _aal_table_invalid;
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
		if (strict_strtoul(token, 0, &start))
#else
		if (kstrtoul(token, 0, &start))
#endif
		{
			printk(KERN_ERR "ERROR! Invalid 2nd value '%s'.\n", token);
			goto _aal_table_invalid;
		}
	} else {
		goto _aal_table_invalid;
	}

	/* parse 3rd token */
	token = strsep(&cp, " \n");
	if ((token != NULL) && (token[0] != '\0')) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
		if (strict_strtoul(token, 0, &num))
#else
		if (kstrtoul(token, 0, &num))
#endif
		{
			printk(KERN_ERR "ERROR! Invalid 3rd value '%s'.\n", token);
			goto _aal_table_invalid;
		}
	}

	/* do action */
	used_count = aal_table_get_used_count(table_id);
	max_count = aal_table_get_max_count(table_id);
	printk(KERN_INFO "used_entry / max_entry = ");

	if (used_count < 0) printk("UNKNOWN");
	else printk("%d", used_count);

	if (max_count < 0) printk(" / UNKNOWN\n");
	else printk(" / %d\n", max_count);

	printk(KERN_INFO "Dump table ID '%lu' entry from %lu to %lu\n", table_id, start, start + num - 1);

	ret = aal_entry_print(table_id, start, num);
	if (ret != AAL_E_OK) {
		printk(KERN_ERR "ERROR! Fail to dump for table id %lu. (ret=%d)\n", table_id, ret);
	}

	return count;

_aal_table_invalid:
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_aal_table __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= aal_table_proc_open,
	.read		= seq_read,
	.write		= aal_table_proc_write,
};

/***************************************************************************
 * aal_table_sw_cache
 ***************************************************************************/

#define AAL_TABLE_SW_CACHE_HELP_MSG \
	"Purpose: Flag to indicate that dumping AAL Table entries from software cache.\n" \
	"\n" \
	"Usage: Enable SW cache\n" \
	"  echo 1 > %s\n" \
	"\n" \
	"Usage: Disable SW cache\n" \
	"  echo 0 > %s\n" \
	"\n"

static int aal_table_sw_cache_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, AAL_TABLE_SW_CACHE_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\nCurrent SW cache: %u\n\n", ca_aal_table_sw_cache);

	return 0;
}

int aal_table_sw_cache_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aal_table_sw_cache_show_help, (void *) file);
}

ssize_t aal_table_sw_cache_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	unsigned long value;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _aal_table_sw_cache_invalid;
	tmp[len] = '\0';

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &value))
#else
	if (kstrtoul(tmp, 0, &value))
#endif
	{
		pr_err("ERROR! Invalid value '%s'.\n", tmp);
		goto _aal_table_sw_cache_invalid;
	}

	switch (value) {
	case 0:
		ca_aal_table_sw_cache = 0;
		break;

	case 1:
		ca_aal_table_sw_cache = 1;
		break;

	default:
		goto _aal_table_sw_cache_invalid;
	}

	printk(KERN_INFO "Current SW cache: %u\n", ca_aal_table_sw_cache);

	return count;

_aal_table_sw_cache_invalid:
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_aal_table_sw_cache __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= aal_table_sw_cache_proc_open,
	.read		= seq_read,
	.write		= aal_table_sw_cache_proc_write,
};

/***************************************************************************
 * dslite_6rd
 ***************************************************************************/

#define DSLITE_6RD_HELP_MSG \
	"DS-Lite / 6RD global settings\n"

static int dslite_6rd_show_help(struct seq_file *m, void *v)
{
	int ret;
	uint32_t tmp_mac;
	uint8_t *cp = (uint8_t *) &tmp_mac;

	bool sixrd_df = false;
	uint8_t sixrd_nh = 0;
	uint16_t sixrd_encap_ethertype = 0;
	uint16_t sixrd_decap_ethertype = 0;

	uint8_t dsl_nh = 0;
	uint16_t dsl_encap_ethertype = 0;
	uint16_t dsl_decap_ethertype = 0;

	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ecn_fail_fwd_ctrl_t ecn_fail_fwd_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_fwd_ctrl_t sixrd_ipmc_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_fwd_ctrl_t dsl_ipmc_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_new_mac_da_t sixrd_ipmc_new_mac_da;
	l3fe_stg1_dsl_ipmc_new_mac_da_t dsl_ipmc_new_mac_da;

	seq_printf(m, DSLITE_6RD_HELP_MSG);

	if ((ret = aal_l3pe_6rd_df_get(&sixrd_df)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_6rd_df_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_6rd_encap_nh_get(&sixrd_nh)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_6rd_encap_nh_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_6rd_encap_ethertype_get(&sixrd_encap_ethertype)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_6rd_encap_ethertype_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_6rd_decap_ethertype_get(&sixrd_decap_ethertype)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_6rd_decap_ethertype_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_dsl_encap_nh_get(&dsl_nh)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_dsl_encap_nh_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_dsl_encap_ethertype_get(&dsl_encap_ethertype)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_dsl_encap_ethertype_get() returns %d.\n", ret);

	if ((ret = aal_l3pe_dsl_decap_ethertype_get(&dsl_decap_ethertype)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3pe_dsl_decap_ethertype_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_ipnip_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_ecn_fail_fwd_ctrl_get(&ecn_fail_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_ecn_fail_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get(&sixrd_ipmc_new_mac_da)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_get(&dsl_ipmc_new_mac_da)) != AAL_E_OK)
		printk(KERN_ERR "ERROR! aal_l3fe_stg1_dsl_ipmc_new_mac_da_get() returns %d.\n", ret);

	/*** 6RD ***/

	seq_printf(m, "\n6RD\n");

	seq_printf(m, "  df = %d.  Fragment bit in IPv4 header.\n", (sixrd_df) ? 1 : 0);
	seq_printf(m, "  nh = 0x%02x.  Protocol of L3 outer header after encapsulation.\n", sixrd_nh);
	seq_printf(m, "  encap_ethertype = 0x%04x.  New EtherType after encapsulation.\n", sixrd_encap_ethertype);
	seq_printf(m, "  decap_ethertype = 0x%04x.  New EtherType after decapsulation.\n", sixrd_decap_ethertype);
	seq_printf(m, "  sixrd_ttl_keep_outer = %d\n", ipnip_ctrl.bf.sixrd_ttl_keep_outer);
	seq_printf(m, "  sixrd_dscp_keep_outer = %d\n", ipnip_ctrl.bf.sixrd_dscp_keep_outer);
	seq_printf(m, "  sixrd_ecn_keep_outer = %d\n", ipnip_ctrl.bf.sixrd_ecn_keep_outer);

	seq_printf(m, "\n");

	/* sixrd_decap_chk_fail_fwd_ctl */

	seq_printf(m, "  sixrd_decap_chk_fail_fwd_ctl = %d\n", ipnip_ctrl.bf.sixrd_decap_chk_fail_fwd_ctl);
	seq_printf(m, "    There are IPv6DA delegated prefix check, reverse path check, IPSA address consistency check.\n");
	seq_printf(m, "    If 6rd_entry.ingr_chk_en is 1 and any check fails, ");
	switch (ipnip_ctrl.bf.sixrd_decap_chk_fail_fwd_ctl) {
	case 0:
		seq_printf(m, "no trap, just drop packet.\n");
		seq_printf(m, "    Also set HDR_I: keep_orig_pkt = 1, cos = unchanged, t2_ctrl = 0xf, t3_ctrl = 0xf, t4_ctrl = 0xf, t5_ctrl = 0xf.\n");
		break;
	case 1:
		seq_printf(m, "packet is trapped.\n");
		break;
	case 2:
		seq_printf(m, "forward directly, no trap. Any failure event is ignored.\n");
		break;
	case 3:
		seq_printf(m, "N/A.\n");
		break;
	}
	seq_printf(m, "    Trap info:\n");
	seq_printf(m, "      keep_orig_pkt = %d\n", ipnip_fail_fwd_ctrl.bf.keep_orig_pkt);
	seq_printf(m, "      ldpid = 0x%02x\n", ipnip_fail_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", ipnip_fail_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t5_ctrl);

	seq_printf(m, "\n");

	/* sixrd_ipmc_new_mac_en */

	if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en) {
		seq_printf(m, "  sixrd_ipmc_new_mac_en = 1.  MAC_DA of MC packet is replaced after decapsulation.\n");
	} else {
		seq_printf(m, "  sixrd_ipmc_new_mac_en = 0.  MAC_DA of MC packet is NOT replaced after decapsulation.\n");
	}
	tmp_mac = (uint32_t) sixrd_ipmc_new_mac_da.bf.addr_prfx;
	seq_printf(m, "    addr_prfx = %02x-%02x-00-00-00-00 / 16\n", *(cp + 1), *cp);

	/* sixrd_ipmc_ilg_chk_en */

	if (ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en) {
		seq_printf(m, "  sixrd_ipmc_ilg_chk_en = 1.  Filter packet if outer IP is MC and inner IP is UC.\n");
	} else {
		seq_printf(m, "  sixrd_ipmc_ilg_chk_en = 0.  Decapsulate packet if outer IP is MC and inner IP is UC; handle packet as UC.\n");
	}

	/* sixrd_ipmc_fwd_ctl */

	if (ipnip_ctrl.bf.sixrd_ipmc_fwd_ctl) {
		seq_printf(m, "  sixrd_ipmc_fwd_ctl = 1.  MC packet is trapped.\n");
	} else {
		seq_printf(m, "  sixrd_ipmc_fwd_ctl = 0.  MC packet is forwarded directly, any failure event is ignored.\n");
	}
	seq_printf(m, "    Trap info:\n");
	if (sixrd_ipmc_fwd_ctrl.bf.keep_orig_pkt) {
		seq_printf(m, "      keep_orig_pkt = 1.  MC packet will NOT be decapsulated.\n");
	} else {
		seq_printf(m, "      keep_orig_pkt = 0.  MC packet will be decapsulated.\n");
	}

	seq_printf(m, "      ldpid = 0x%02x\n", sixrd_ipmc_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", sixrd_ipmc_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", sixrd_ipmc_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", sixrd_ipmc_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", sixrd_ipmc_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", sixrd_ipmc_fwd_ctrl.bf.t5_ctrl);


	/*** DS-Lite ***/

	seq_printf(m, "\nDS-Lite\n");

	seq_printf(m, "  nh = 0x%02x.  Protocol of L3 outer header after encapsulation.\n", dsl_nh);
	seq_printf(m, "  encap_ethertype = 0x%04x.  New EtherType after encapsulation.\n", dsl_encap_ethertype);
	seq_printf(m, "  decap_ethertype = 0x%04x.  New EtherType after decapsulation.\n", dsl_decap_ethertype);
	seq_printf(m, "  dsl_ttl_keep_outer = %d\n", ipnip_ctrl.bf.dsl_ttl_keep_outer);
	seq_printf(m, "  dsl_dscp_keep_outer = %d\n", ipnip_ctrl.bf.dsl_dscp_keep_outer);
	seq_printf(m, "  dsl_ecn_keep_outer = %d\n", ipnip_ctrl.bf.dsl_ecn_keep_outer);

	seq_printf(m, "\n");

	/* dsl_rpf_chk_en */

	if (ipnip_ctrl.bf.dsl_rpf_chk_en) {
		seq_printf(m, "  dsl_rpf_chk_en = 1.  Enable reverse path check for outer UC packet. IPv6DA/IPv6SA must match B4/AFTR settings.\n");
	} else {
		seq_printf(m, "  dsl_rpf_chk_en = 0.  Disable reverse path check for outer UC packet.\n");
	}

	/* dsl_decap_chk_fail_fwd_ctl */

	seq_printf(m, "  dsl_decap_chk_fail_fwd_ctl = %d\n", ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl);
	seq_printf(m, "    If dsl_rpf_chek_en is 1 and reverse path check fails for UC packet, ");
	switch (ipnip_ctrl.bf.dsl_decap_chk_fail_fwd_ctl) {
	case 0:
		seq_printf(m, "no trap, just drop packet.\n");
		seq_printf(m, "    Also set HDR_I: keep_orig_pkt = 1, cos = unchanged, t2_ctrl = 0xf, t3_ctrl = 0xf, t4_ctrl = 0xf, t5_ctrl = 0xf.\n");
		break;
	case 1:
		seq_printf(m, "packet is trapped.\n");
		break;
	case 2:
		seq_printf(m, "forward directly, no trap. Any failure event is ignored.\n");
		break;
	case 3:
		seq_printf(m, "N/A.\n");
		break;
	}
	seq_printf(m, "    Trap info:\n");
	seq_printf(m, "      keep_orig_pkt = %d\n", ipnip_fail_fwd_ctrl.bf.keep_orig_pkt);
	seq_printf(m, "      ldpid = 0x%02x\n", ipnip_fail_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", ipnip_fail_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t5_ctrl);

	seq_printf(m, "\n");

	/* dsl_ipmc_new_mac_en */

	if (ipnip_ctrl.bf.dsl_ipmc_new_mac_en) {
		seq_printf(m, "  dsl_ipmc_new_mac_en = 1.  MAC_DA of MC packet is replaced after decapsulation.\n");
	} else {
		seq_printf(m, "  dsl_ipmc_new_mac_en = 0.  MAC_DA of MC packet is NOT replaced after decapsulation.\n");
	}
	tmp_mac = (uint32_t) dsl_ipmc_new_mac_da.bf.addr_prfx << 7;
	seq_printf(m, "    addr_prfx = %02x-%02x-%02x-%02x-00-00 / 25\n", *(cp + 3), *(cp + 2), *(cp + 1), *cp);

	/* dsl_ipmc_ilg_chk_en */

	if (ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en) {
		seq_printf(m, "  dsl_ipmc_ilg_chk_en = 1.  Drop packet if outer IP is MC and inner IP is UC.\n");
	} else {
		seq_printf(m, "  dsl_ipmc_ilg_chk_en = 0.  Decapsulate packet if outer IP is MC and inner IP is UC; handle packet as UC.\n");
	}

	/* dsl_ipmc_ext_det_chk_en */

	if (ipnip_ctrl.bf.dsl_ipmc_ext_det_en) {
		seq_printf(m, "  dsl_ipmc_ext_det_en = 1.  Treat as DSLite MC packet if outer IP is UC and inner IP is MC.\n");
	} else {
		seq_printf(m, "  dsl_ipmc_ext_det_en = 0.  If outer IP is UC and inner IP is MC, decapsulate packet. Not handle as MC nor UC.\n");
	}

	/* dsl_ipmc_addr_prfx_chk_en */

	if (ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en) {
		seq_printf(m, "  dsl_ipmc_addr_prfx_chk_en = 1.  Check IPv6SA/IPv6DA address prefix of MC packet with DSLite AAL entry.\n");
	} else {
		seq_printf(m, "  dsl_ipmc_addr_prfx_chk_en = 0.  NOT check IPv6SA/IPv6DA address prefix of MC packet, decapsulate directly.\n");
	}

	/* dsl_ipmc_addr_consistency_chk_en */

	if (ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en) {
		seq_printf(m, "  dsl_ipmc_addr_consistency_chk_en = 1.  Check outer and inner MC address consistency. (suffix 32 bits)\n");
	} else {
		seq_printf(m, "  dsl_ipmc_addr_consistency_chk_en = 0.  NOT check outer and inner MC address consistency. (suffix 32 bits)\n");
	}
	/* dsl_ipmc_fwd_ctl */

	if (ipnip_ctrl.bf.dsl_ipmc_fwd_ctl) {
		seq_printf(m, "  dsl_ipmc_fwd_ctl = 1.  MC packet is trapped.\n");
	} else {
		seq_printf(m, "  dsl_ipmc_fwd_ctl = 0.  MC packet is forwarded directly, any failure event is ignored.\n");
	}
	seq_printf(m, "    Trap info:\n");
	if (dsl_ipmc_fwd_ctrl.bf.keep_orig_pkt) {
		seq_printf(m, "      keep_orig_pkt = 1.  MC packet will NOT be decapsulated.\n");
	} else {
		seq_printf(m, "      keep_orig_pkt = 0.  MC packet will be decapsulated.\n");
	}

	seq_printf(m, "      ldpid = 0x%02x\n", dsl_ipmc_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", dsl_ipmc_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", dsl_ipmc_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", dsl_ipmc_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", dsl_ipmc_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", dsl_ipmc_fwd_ctrl.bf.t5_ctrl);

	/*** Common ***/

	seq_printf(m, "\nCommon\n");

	/* intf_chk_fail_fwd_ctl */

	seq_printf(m, "  intf_chk_fail_fwd_ctl = %d\n", ipnip_ctrl.bf.intf_chk_fail_fwd_ctl);
	seq_printf(m, "    If ingress_IF_ID (mdata[31:24]) check fails (not for DSLite nor 6RD), ");
	switch (ipnip_ctrl.bf.intf_chk_fail_fwd_ctl) {
	case 0:
		seq_printf(m, "no trap, just drop packet.\n");
		seq_printf(m, "    Also set HDR_I: keep_orig_pkt = 1, cos = unchanged, t2_ctrl = 0xf, t3_ctrl = 0xf, t4_ctrl = 0xf, t5_ctrl = 0xf.\n");
		break;
	case 1:
		seq_printf(m, "packet is trapped.\n");
		break;
	case 2:
		seq_printf(m, "packet is forwarded directly, no trap. Any failure event is ignored.\n");
		break;
	case 3:
		seq_printf(m, "N/A.\n");
		break;
	}
	seq_printf(m, "    Trap info:\n");
	seq_printf(m, "      keep_orig_pkt = %d\n", ipnip_fail_fwd_ctrl.bf.keep_orig_pkt);
	seq_printf(m, "      ldpid = 0x%02x\n", ipnip_fail_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", ipnip_fail_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", ipnip_fail_fwd_ctrl.bf.t5_ctrl);

	/* ecn_chk_fail_fwd_ctl */

	seq_printf(m, "  ecn_chk_fail_fwd_ctl = %d\n", ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl);
	if (ipnip_ctrl.bf.ecn_chk_fail_fwd_ctl) {
		seq_printf(m, "    If ECN check fails (outer ECN is set, inner ECN is zero), packet is trapped.\n");
	} else {
		seq_printf(m, "    If ECN check fails (outer ECN is set, inner ECN is zero),\n");
		seq_printf(m, "    packet is forwarded directly, no trap. Any failure event is ignored.\n");
	}
	seq_printf(m, "    Trap info:\n");
	seq_printf(m, "      keep_orig_pkt = %d\n", ecn_fail_fwd_ctrl.bf.keep_orig_pkt);
	seq_printf(m, "      ldpid = 0x%02x\n", ecn_fail_fwd_ctrl.bf.ldpid);
	seq_printf(m, "      cos = %d\n", ecn_fail_fwd_ctrl.bf.cos);
	seq_printf(m, "      t2_ctrl = 0x%x\n", ecn_fail_fwd_ctrl.bf.t2_ctrl);
	seq_printf(m, "      t3_ctrl = 0x%x\n", ecn_fail_fwd_ctrl.bf.t3_ctrl);
	seq_printf(m, "      t4_ctrl = 0x%x\n", ecn_fail_fwd_ctrl.bf.t4_ctrl);
	seq_printf(m, "      t5_ctrl = 0x%x\n", ecn_fail_fwd_ctrl.bf.t5_ctrl);

	return 0;
}

int dslite_6rd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dslite_6rd_show_help, (void *) file);
}

static const struct file_operations fops_dslite_6rd __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= dslite_6rd_proc_open,
	.read		= seq_read,
	.write		= NULL,
};

/***************************************************************************
 * bypass_l3fe
 ***************************************************************************/

#define BYPASS_L3FE_HELP_MSG \
	"Purpose: Let packets Bypass L3FE or not.\n" \
	"\n" \
	"Bypass L3FE\n" \
	"  echo 1 > %s\n" \
	"\n" \
	"Not bypass L3FE\n" \
	"  echo 0 > %s\n" \
	"\n"

/* CLS rule index in profile #0 and #1 */
ca_int32_t to_cpu_cls[2] = { -1, -1};

/* LPB mac_da_match_en in profile #0 ~ #3 */
ca_int32_t g_mac_da_match_en[AAL_L3FE_LPB_TBL_NUM] = { -1, -1, -1, -1 };

static int bypass_l3fe_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, BYPASS_L3FE_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "%s = %u\n", file->f_path.dentry->d_iname, ca_bypass_l3fe);

	if (ca_bypass_l3fe) {
		seq_printf(m, "cls_key_idx=%u, %u\n", CLS_TBL_ID(to_cpu_cls[0]), CLS_TBL_ID(to_cpu_cls[1]));
	}

	return 0;
}

int bypass_l3fe_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bypass_l3fe_show_help, (void *) file);
}

static int __create_classifier_to_cpu(void)
{
	l3_cls_rule_t aal_rule;
	int ret;

	memset(&aal_rule, 0, sizeof(l3_cls_rule_t));
	/* ignore all fields */
	aal_l3_cls_key_clean(&aal_rule.key);
	aal_rule.pri = CL_RUL_PRIO_MAX;
	aal_rule.rslt_type = CL_RSLT_TYPE_0;
	aal_rule.action.keep_orig_pkt_vld = 1;
	aal_rule.action.keep_orig_pkt = 1;
	aal_rule.action.t5_ctrl_vld = 1;
	aal_rule.action.t5_ctrl	 = T5_CTRL_BYPASS;
	aal_rule.action.t4_ctrl_vld = 1;
	aal_rule.action.t4_ctrl	 = T4_CTRL_BYPASS;
	aal_rule.action.t3_ctrl_vld = 1;
	aal_rule.action.t3_ctrl	 = T3_CTRL_BYPASS;
	aal_rule.action.t2_ctrl_vld = 1;
	aal_rule.action.t2_ctrl	 = T2_CTRL_BYPASS;
	aal_rule.action.mc = 0;
	aal_rule.action.permit = 1;
	aal_rule.action.dpid_pri = 1;
	aal_rule.action.dpid_vld = 1;
	aal_rule.action.mcgid = AAL_LPORT_CPU_0;
	aal_rule.action.deepq = 1;
	if ((ret = aal_l3_cls_add(0, 0, &aal_rule, &to_cpu_cls[0])) != AAL_E_OK) {
		printk(KERN_INFO "%s: Fail to add CLS rule into profile#0. (ret=%d)\n", __func__, ret);
		return ret;
	}

	aal_rule.action.mcgid = AAL_LPORT_CPU_1;
	if ((ret = aal_l3_cls_add(0, 1, &aal_rule, &to_cpu_cls[1])) != AAL_E_OK) {
		aal_l3_cls_delete(0, 0, to_cpu_cls[0]);
		printk(KERN_INFO "%s: Fail to add CLS rule into profile#1. (ret=%d)\n", __func__, ret);
		return ret;
	}
	return CA_E_OK;
}

static int __delete_classifier_to_cpu(void)
{
	int ret;

	if ((to_cpu_cls[0] >= 0) && (ret = aal_l3_cls_delete(0, 0, to_cpu_cls[0]))) {
		printk(KERN_INFO "%s: Fail to delete CLS rule from profile#0. (ret=%d)\n", __func__, ret);
		return ret;
	}
	to_cpu_cls[0] = -1;

	if ((to_cpu_cls[1] >= 0) && (ret = aal_l3_cls_delete(0, 1, to_cpu_cls[1]))) {
		printk(KERN_INFO "%s: Fail to delete CLS rule from profile#1. (ret=%d)\n", __func__, ret);
		return ret;
	}
	to_cpu_cls[1] = -1;

	return CA_E_OK;
}

ssize_t bypass_l3fe_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	unsigned long bypass_l3fe = 0;
	aal_l3fe_lpb_tbl_cfg_t lpb_tbl_cfg;
	aal_l3fe_lpb_tbl_cfg_mask_t lpb_tbl_cfg_mask;
	int i;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		goto _bypass_l3fe_invalid;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
		if (strict_strtoul(token, 0, &bypass_l3fe))
#else
		if (kstrtoul(token, 0, &bypass_l3fe))
#endif
		{
			printk(KERN_ERR "ERROR! Invalid 1st value '%s'.\n", token);
			goto _bypass_l3fe_invalid;
		}
	} else {
		goto _bypass_l3fe_invalid;
	}

	/* validation */
	if ((bypass_l3fe == 0) && (ca_bypass_l3fe == 0)) {
		printk(KERN_INFO "WARNING! bypass_l3fe is already disabled.\n");
		return count;
	}
	if ((bypass_l3fe != 0) && (ca_bypass_l3fe != 0)) {
		printk(KERN_INFO "WARNING! bypass_l3fe is already enabled.\n");
		return count;
	}

	/* do action */
	if (bypass_l3fe == 0) {
		if (__delete_classifier_to_cpu() == CA_E_OK)
			ca_bypass_l3fe = 0;

		/* restore LPB mac_da_match_en */
		memset(&lpb_tbl_cfg_mask, 0, sizeof(lpb_tbl_cfg_mask));
		lpb_tbl_cfg_mask.s.mac_da_match_en = 1;
		for (i = 0; i < AAL_L3FE_LPB_TBL_NUM; i++) {
			(void) aal_l3_stg0_lpb_tbl_get(0, i, &lpb_tbl_cfg);
			if (g_mac_da_match_en[i] >= 0)
				lpb_tbl_cfg.mac_da_match_en = g_mac_da_match_en[i];
			(void) aal_l3_stg0_lpb_tbl_set(0, i, lpb_tbl_cfg_mask, &lpb_tbl_cfg);
		}

		printk(KERN_INFO "%s = %u\n", file->f_path.dentry->d_iname, ca_bypass_l3fe);
	} else {
		if (__create_classifier_to_cpu() == CA_E_OK)
			ca_bypass_l3fe = 1;

		/* disable LPB mac_da_match_en */
		memset(&lpb_tbl_cfg_mask, 0, sizeof(lpb_tbl_cfg_mask));
		lpb_tbl_cfg_mask.s.mac_da_match_en = 1;
		for (i = 0; i < AAL_L3FE_LPB_TBL_NUM; i++) {
			(void) aal_l3_stg0_lpb_tbl_get(0, i, &lpb_tbl_cfg);
			g_mac_da_match_en[i] = lpb_tbl_cfg.mac_da_match_en;
			lpb_tbl_cfg.mac_da_match_en = 0;
			(void) aal_l3_stg0_lpb_tbl_set(0, i, lpb_tbl_cfg_mask, &lpb_tbl_cfg);
		}

		printk(KERN_INFO "%s = %u\n", file->f_path.dentry->d_iname, ca_bypass_l3fe);
	}

	if (ca_bypass_l3fe) {
		printk(KERN_INFO "cls_key_idx=%u, %u\n", CLS_TBL_ID(to_cpu_cls[0]), CLS_TBL_ID(to_cpu_cls[1]));
	}

	return count;

_bypass_l3fe_invalid:
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	return count;
}

static const struct file_operations fops_bypass_l3fe __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= bypass_l3fe_proc_open,
	.read		= seq_read,
	.write		= bypass_l3fe_proc_write,
};

/***************************************************************************
 * init/exit functions
 ***************************************************************************/

int ca_aal_proc_init(void)
{
	proc_aal = proc_mkdir("aal", ca_proc_dir);

	if (proc_create("aal_table", S_IRUGO | S_IWUGO, proc_aal, &fops_aal_table) == NULL) {
		pr_err("Fail to create proc entry aal_table.\n");
		return -ENOMEM;
	}

	if (proc_create("aal_table_sw_cache", S_IRUGO | S_IWUGO, proc_aal, &fops_aal_table_sw_cache) == NULL) {
		pr_err("Fail to create proc entry aal_table_sw_cache.\n");
		return -ENOMEM;
	}

	if (proc_create("aal_debug", S_IRUGO | S_IWUGO, proc_aal, &fops_aal_debug) == NULL) {
		pr_err("Fail to create proc entry aal_debug.\n");
		return -ENOMEM;
	}

	if (SOC_HAS_L3FE()) {
		if (proc_create("l3fe_debug", S_IRUGO | S_IWUGO, proc_aal, &fops_l3fe_debug) == NULL) {
			pr_err("Fail to create proc entry l3fe_debug.\n");
			return -ENOMEM;
		}

		if (proc_create("cntr_rx", S_IRUGO | S_IWUGO, proc_aal, &fops_cntr_rx) == NULL) {
			pr_err("Fail to create proc entry cntr_rx.\n");
			return -ENOMEM;
		}

		if (proc_create("cntr_tx", S_IRUGO | S_IWUGO, proc_aal, &fops_cntr_tx) == NULL) {
			pr_err("Fail to create proc entry cntr_tx.\n");
			return -ENOMEM;
		}

		if (proc_create("cntr_drop", S_IRUGO | S_IWUGO, proc_aal, &fops_cntr_drop) == NULL) {
			pr_err("Fail to create proc entry cntr_drop.\n");
			return -ENOMEM;
		}

		if (proc_create("dslite_6rd", S_IRUGO | S_IWUGO, proc_aal, &fops_dslite_6rd) == NULL) {
			pr_err("Fail to create proc entry dslite_6rd.\n");
			return -ENOMEM;
		}

		if (proc_create("bypass_l3fe", S_IRUGO | S_IWUGO, proc_aal, &fops_bypass_l3fe) == NULL) {
			pr_err("Fail to create proc entry bypass_l3fe.\n");
			return -ENOMEM;
		}
	}

	if (proc_create("l3qm_debug", S_IRUGO | S_IWUGO, proc_aal, &fops_l3qm_debug) == NULL) {
		pr_err("Fail to create proc entry l3qm_debug.\n");
		return -ENOMEM;
	}

	if (proc_create("ni_debug", S_IRUGO | S_IWUGO, proc_aal, &fops_ni_debug) == NULL) {
		pr_err("Fail to create proc entry ni_debug.\n");
		return -ENOMEM;
	}

	return 0;
}

void ca_aal_proc_exit(void)
{
	remove_proc_entry("aal_debug", proc_aal);
	proc_remove(proc_aal);
}

