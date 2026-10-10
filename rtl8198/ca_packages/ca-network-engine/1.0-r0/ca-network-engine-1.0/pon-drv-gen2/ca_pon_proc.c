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
 * ca_ponproc.c: Proc entry for Cortina NI driver
 *
 */
#ifdef CONFIG_NE_PON

#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <soc/cortina/cortina-soc.h>
#include "ca_types.h"
#include "aal_pon.h"
#include "scfg.h"

extern ca_uint32_t ca_pon_debug;
extern ca_uint32_t aal_pon_int_cnt;
extern ca_uint32_t aal_pon_epon_cnt;
extern ca_uint32_t aal_pon_xgpon_cnt;
extern ca_uint32_t aal_pon_misc_int_cnt;

extern void aal_xgpn_dbg_activation_dump(void);
extern ca_status_t aal_xgpon_aes_state_get(ca_device_id_t device_id);

extern void aal_mpcp_state_dump(ca_device_id_t  device_id);

//#define pr_fmt(fmt)     KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

static struct proc_dir_entry *__proc_pon = NULL;

#define __PON_HELP_MSG \
    "Purpose    : Control PON driver debug message\n" \
    "Read Usage : cat %s\n" \
    "Write Usage: echo [bitwise flag] > %s\n" \
    "  0x00000000: disable PON debug message\n" \
    "  0x00000001: PON INTR debug message\n" \
    "  0x00000002: GPON Activation FSM information\n" \
    "  0x00000004: GPON PLOAM Rx message\n" \
    "  0x00000008: GPON PLOAM Tx message\n" \
    "  0x00000010: GPON AES Rx message\n" \
    "  0x00000020: GPON AES Exchange State Machine information\n" \
    "  0x00000040: GPON OMCI Rx message\n" \
    "  0x00000080: GPON OMCI Tx message\n" \
    "  0x00000100: EPON MPCP debug message\n" \
    "  0x00000200: EPON OAM Rx debug message\n" \
    "  0x00000400: EPON OAM Tx debug message\n" \
    "  0x00000800: PON Event message\n" \
    "  0x00001000: GPON API debug message\n"


static int __pon_debug_show_help(struct seq_file *m, void *v)
{
    struct file *file = m->private;

    seq_printf(m, __PON_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
    seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_pon_debug);

    return 0;
}

int __pon_debug_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, __pon_debug_show_help, (void *) file);
}

ssize_t __pon_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    char tmp[32];
    unsigned long mask;
    ssize_t len;
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(0);

    len = min(count, (sizeof(tmp) - 1));
    if (copy_from_user(tmp, buf, len))
        goto ___pon_debug_invalid;

    tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
    if (strict_strtoul(tmp, 0, &mask))
#else
    if (kstrtoul(tmp, 0, &mask))
#endif
        goto ___pon_debug_invalid;

    ca_pon_debug = mask;
    printk(KERN_INFO "Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_pon_debug);

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        if (PON_DBG_GPON_ACT_FSM & ca_pon_debug) {
            aal_xgpn_dbg_activation_dump();
        }

        if (PON_DBG_GPON_AES_FSM & ca_pon_debug) {
            aal_xgpon_aes_state_get(0);
        }
    }
    else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        if (PON_DBG_EPON_MPCP & ca_pon_debug) {
            aal_mpcp_state_dump(0);
        }
    }

    return count;

___pon_debug_invalid:
    printk(KERN_ERR "ERROR! Invalid value %s for %s.\n", tmp, file->f_path.dentry->d_iname);
    /* if we return error code here, PROC fs may retry up to 3 times. */
    return count;
}

static const struct file_operations fops_pon_debug __read_mostly = {
    .owner        = THIS_MODULE,
    .open        = __pon_debug_proc_open,
    .read        = seq_read,
    .write        = __pon_debug_proc_write,
};


static int __pon_intr_show_help(struct seq_file *m, void *v)
{
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(0);

    seq_printf(m, "Total PON interrupts count = %u\n", aal_pon_int_cnt);

    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        seq_printf(m, "EPON interrupts count       = %u\n", aal_pon_epon_cnt);
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        seq_printf(m, "GPON interrupts count      = %u\n", aal_pon_xgpon_cnt);
    }

    seq_printf(m, "Misc PON interrupts count  = %u\n", aal_pon_misc_int_cnt);

    return 0;
}

int __pon_intr_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, __pon_intr_show_help, (void *) file);
}

static const struct file_operations fops_pon_intr __read_mostly = {
    .owner        = THIS_MODULE,
    .open        = __pon_intr_proc_open,
    .read        = seq_read,
};
int ca_pon_proc_init(void)
{
    __proc_pon = proc_mkdir("pon", ca_proc_dir);

    if (proc_create("pon_debug", S_IRUGO | S_IWUGO, __proc_pon, &fops_pon_debug) == NULL) {
        pr_err("Fail to create proc entry pon_debug.\n");
        return -ENOMEM;
    }


    if (proc_create("pon_intr", S_IRUGO | S_IWUGO, __proc_pon, &fops_pon_intr) == NULL) {
        pr_err("Fail to create proc entry pon_intr_count.\n");
        return -ENOMEM;
    }

    printk("ca-pon: proc files init OK.\n");
    return 0;
}

void ca_pon_proc_exit(void)
{
    remove_proc_entry("pon_debug", __proc_pon);
    remove_proc_entry("pon_intr",  __proc_pon);
    proc_remove(__proc_pon);
}
#endif /* CONFIG_NE_PON */

