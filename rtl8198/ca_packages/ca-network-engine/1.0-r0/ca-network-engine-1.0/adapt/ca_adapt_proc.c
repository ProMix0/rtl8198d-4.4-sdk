/*
 * ca_adapt_proc.c: Proc entry for Adaptation
 *
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <generated/ca_ne_autoconf.h>
#include <linux/proc_fs.h>
#include <soc/cortina/cortina-soc.h>
#include "ca_adapt_proc.h"

#ifdef CONFIG_NE_L2FP
#include "l2fp/ca_l2fp.h"
#endif

#ifdef CONFIG_NE_L3COS
#include "l3cos/ca_l3cos.h"
#endif

#ifdef CONFIG_NE_ARP_FLOODING
#include "arp_ctrl/ca_arp_ctrl.h"
#endif

struct proc_dir_entry *proc_adapt_dir = NULL;

/***************************************************************************
 * init/exit functions
 ***************************************************************************/

int ca_adapt_proc_init(void)
{
	proc_adapt_dir = proc_mkdir("adapt", ca_proc_dir);

#ifdef CONFIG_NE_L2FP
	ca_l2fp_proc_init();
#endif

#ifdef CONFIG_NE_L3COS
	ca_l3cos_proc_init();
#endif

#ifdef CONFIG_NE_ARP_FLOODING
	ca_arp_ctrl_proc_init();
#endif
        return 0;
}

void ca_adapt_proc_exit(void)
{
#ifdef CONFIG_NE_L2FP
	ca_l2fp_proc_exit();
#endif

#ifdef CONFIG_NE_L3COS
	ca_l3cos_proc_exit();
#endif

	proc_remove(proc_adapt_dir);
}


