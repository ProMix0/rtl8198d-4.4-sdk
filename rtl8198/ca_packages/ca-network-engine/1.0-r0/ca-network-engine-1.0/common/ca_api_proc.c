/*
 * ca_api_proc.c: Proc entry for Cortina API
 *
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/proc_fs.h>
#include <soc/cortina/cortina-soc.h>
#include "api_common.h"
#include "well_known_mcast_flooding.h"

struct proc_dir_entry *proc_api = NULL;

#ifdef CONFIG_NE_PON
extern ca_status_t ca_gpon_cb_dump (ca_device_id_t device_id);
#endif

ca_status_t ca_cls_rule_dump(void);
void flow_type_dump(void);
void flow_table_dump(void);
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
void nat_table_dump(void);
void route_table_dump(void);
void nexthop_table_dump(void);
void intf_table_dump(void);
void tunnel_table_dump(void);
#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
void l2_mcast_table_dump(void);
void l3_mcast_table_dump(void);
void sw_macsec_table_dump(void);
#endif
#endif /* !CONFIG_ARCH_CORTINA_SATURN_SFU */
extern void eth_port_dump(void);
extern void ca_qos_dump(void);

#ifdef CONFIG_NE_SW_SHAPER
void ca_sw_shaper_table_dump(void);
#endif

#ifdef CONFIG_NE_US_RATE_MANAGER
void ca_us_rate_manager_table_dump(void);
#endif


/***************************************************************************
 * proc 'debug'
 ***************************************************************************/

#define API_DEBUG_HELP_MSG \
	"Purpose: Enable Cortina API debug message\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo [bitwise flag] > %s\n" \
	"  0x00000001: CLS debug message\n" \
	"  0x00000002: NAT debug message\n" \
	"  0x00000004: Route debug message\n" \
	"  0x00000008: NextHop debug message\n" \
	"  0x00000010: Interface debug message\n" \
	"  0x00000020: Tunnel debug message\n" \
	"  0x00000040: L2 Multicast debug message\n" \
	"  0x00000080: L3 Multicast debug message\n" \
	"  0x00000100: Flow related API debug message\n" \
	"  0x00000200: Ethernet Port API debug message\n" \
	"  0x00000400: Special packet API debug message\n" \
	"\n"

static int api_debug_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, API_DEBUG_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);
	seq_printf(m, "\n%s = 0x%08x\n", file->f_path.dentry->d_iname, ca_api_debug);

	return 0;
}

int api_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, api_debug_show_help, (void *) file);
}

ssize_t api_debug_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long mask;
	ssize_t len;

	len = min(count, sizeof(tmp) - 1);
	if (copy_from_user(tmp, buf, len))
		goto _api_debug_invalid;

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &mask))
#else
		if (kstrtoul(tmp, 0, &mask))
#endif
			goto _api_debug_invalid;

	ca_api_debug = mask;
	pr_info("Set %s as 0x%08x\n", file->f_path.dentry->d_iname, ca_api_debug);

	return count;

_api_debug_invalid:
	pr_err("ERROR! Invalid value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_api_debug __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= api_debug_proc_open,
	.read		= seq_read,
	.write		= api_debug_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * proc 'dump'
 ***************************************************************************/

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#define API_DUMP_HELP_MSG \
    "Purpose: Dump Cortina API info\n" \
    "READ Usage: cat %s\n" \
    "WRITE Usage: echo NUM > %s\n" \
    "  0: Dump CLS rule\n" \
    "  6: Dump L2 Multicast table\n" \
    "  8: Dump added flow types\n" \
    "  9: Dump Flow table\n" \
    "  10: Dump SW Shaper table\n" \
    "  12: Dump MACSEC table\n" \
    "  13: Dump ethernet port table\n" \
    "  14: Dump QoS table\n" \
    "  15: Dump PON table\n" \
    "\n"

#elif defined(CONFIG_ARCH_CORTINA_G3HGU)

#define API_DUMP_HELP_MSG \
	"Purpose: Dump Cortina API info\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo NUM > %s\n" \
	"  0: Dump CLS rule\n" \
	"  1: Dump NAT table\n" \
	"  2: Dump Route table\n" \
	"  3: Dump NextHop table\n" \
	"  4: Dump Interface table\n" \
	"  5: Dump Tunnel table\n" \
	"  6: Dump L2 Multicast table\n" \
	"  7: Dump L3 Multicast table\n" \
	"  8: Dump added flow types\n" \
	"  9: Dump Flow table\n" \
	"  10: Dump SW Shaper table\n" \
	"  11: Dump Upstream Rate Manager table\n" \
	"  12: Dump MACSEC table\n" \
	"  13: Dump ethernet port table\n" \
    "  14: Dump QoS table\n" \
	"  15: Dump PON table\n" \
	"\n"

#else

#define API_DUMP_HELP_MSG \
	"Purpose: Dump Cortina API info\n" \
	"READ Usage: cat %s\n" \
	"WRITE Usage: echo NUM > %s\n" \
	"  0: Dump CLS rule\n" \
	"  1: Dump NAT table\n" \
	"  2: Dump Route table\n" \
	"  3: Dump NextHop table\n" \
	"  4: Dump Interface table\n" \
	"  5: Dump Tunnel table\n" \
	"  6: Dump L2 Multicast table\n" \
	"  7: Dump L3 Multicast table\n" \
	"  8: Dump added flow types\n" \
	"  9: Dump Flow table\n" \
	"  10: Dump SW Shaper table\n" \
	"  11: Dump Upstream Rate Manager table\n" \
	"  12: Dump MACSEC table\n" \
	"  13: Dump ethernet port table\n" \
    "  14: Dump QoS table\n" \
	"\n"
#endif

static int api_dump_show_help(struct seq_file *m, void *v)
{
	struct file *file = m->private;

	seq_printf(m, API_DUMP_HELP_MSG, file->f_path.dentry->d_iname, file->f_path.dentry->d_iname);

	return 0;
}

int api_dump_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, api_dump_show_help, (void *) file);
}

ssize_t api_dump_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	unsigned long value;
	ssize_t len;

	len = min(count, sizeof(tmp) - 1);
	if (copy_from_user(tmp, buf, len))
		goto _api_debug_invalid;

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)

	tmp[len] = '\0';
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,35)
	if (strict_strtoul(tmp, 0, &value))
#else
		if (kstrtoul(tmp, 0, &value))
#endif
			goto _api_debug_invalid;

	switch (value) {
	case API_DUMP_CLS_RULE:
		ca_cls_rule_dump();
		break;

	case API_DUMP_FLOW_TYPE:
		flow_type_dump();
		break;

	case API_DUMP_FLOW_TBL:
		flow_table_dump();
		break;

#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
	case API_DUMP_NAT_TBL:
		nat_table_dump();
		break;

	case API_DUMP_ROUTE_TBL:
		route_table_dump();
		break;

	case API_DUMP_NEXTHOP_TBL:
		nexthop_table_dump();
		break;

	case API_DUMP_INTF_TBL:
		intf_table_dump();
		break;

	case API_DUMP_TUNNEL_TBL:
		tunnel_table_dump();
		break;

	case API_DUMP_L2_MCAST_TBL:
		l2_mcast_table_dump();
		break;

	case API_DUMP_L3_MCAST_TBL:
#ifdef CONFIG_NE_L3_MCAST
		l3_mcast_table_dump();
#endif
		break;

	case API_DUMP_MACSEC_TBL:
		sw_macsec_table_dump();
		break;
#endif /* !CONFIG_ARCH_CORTINA_SATURN_SFU */

	case API_DUMP_SW_SHAPER_TBL:
#ifdef CONFIG_NE_SW_SHAPER
		ca_sw_shaper_table_dump();
#endif
		break;

	case API_DUMP_US_RATE_MANAGER_TBL:
#ifdef CONFIG_NE_US_RATE_MANAGER
		ca_us_rate_manager_table_dump();
#endif
		break;

	case API_DUMP_ETH_PORT:
		eth_port_dump();
		break;

    case API_DUMP_PON:
#ifdef CONFIG_NE_PON
        printk("dump GPON tables:\r\n");
        ca_gpon_cb_dump(0);
#endif
        break;

    case API_DUMP_QOS:
        ca_qos_dump();
        break;

	default:
		goto _api_debug_invalid;
	}
#endif
	return count;

_api_debug_invalid:
	pr_err("ERROR! Invalid or unsupported value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
	/* if we return error code here, PROC fs may retry up to 3 times. */
	return count;
}

static const struct file_operations fops_api_dump __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= api_dump_proc_open,
	.read		= seq_read,
	.write		= api_dump_proc_write,
	.release	= single_release,
};

#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)

/***************************************************************************
 * proc 'well-known mcast flooding'
 ***************************************************************************/
void ca_wmc_flooding_show_table(void);
void wmc_flooding_disable(void);
void wmc_flooding_enable(ca_uint8_t add_default);
void ca_wmc_flooding_add_address(char *address, ca_ip_afi_t afi);
void ca_wmc_flooding_add(char *input);
void ca_wmc_flooding_update(char *input);
void ca_wmc_flooding_delete(char *entry_id);
void ca_wmc_flooding_delete_all(void);
void ca_wmc_flooding_set_mode(char *input, unsigned long type);

extern ca_uint8_t g_wmc_flooding_enable;
extern ca_uint8_t g_wmc_flooding_default;

#define API_WMC_HELP_MSG \
	"Purpose: WMC Cortina API info\n" \
	"READ Usage: cat wmc_flooding\n" \
	"WRITE Usage: echo OPERATION > wmc_flooding\n" \
	"  0: Get status of well known multicast status\n" \
	"  1: Disable well known multicast flooding rule\n" \
	"  2: Enable well known multicast flooding rule with default rules\n" \
	"  3: Add well known multicast IPv4 address range, echo 3 START_ADDRESS END_ADDRESS > wmc_flooding\n" \
	"  4: Add well known multicast IPv6 address range, echo 4 START_ADDRESS END_ADDRESS > wmc_flooding\n" \
	"  5: Add a flooding group with VLAN ID, echo 5 VID PORTS > wmc_flooding\n" \
	"  6: Update a flooding group by VLAN ID, echo 6 VID PORTS > wmc_flooding\n" \
	"  7: Delete a flooding group by VLAN ID, echo 7 VID > wmc_flooding\n" \
	"  8: Delete all flooding group\n" \
	"  9: Enable well known multicast flooding flag only\n" \
	"  10: Set filter mode on offload side: 0 is disable, 1 is enable.\n" \
	"  11: Set debug mode on offload side: 0 is disable, 1 is enable.\n" \
        "\n"

static int api_wmc_flooding_show_help(struct seq_file *m, void *v)
{
        seq_printf(m, API_WMC_HELP_MSG);

        return 0;
}

int api_wmc_flooding_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, api_wmc_flooding_show_help, (void *) file);
}

ssize_t api_wmc_flooding_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
        char tmp[64];
        unsigned long value;
        ssize_t len;
        char *token, *cp = tmp;

        if(!g_wmc_flooding_default)
        {
                printk("Not support well known multicast flooding\n");
                return count;
        }

        len = min(count, sizeof(tmp) - 1);
        if (copy_from_user(tmp, buf, len))
                goto _api_debug_invalid;
        tmp[len] = '\0';

        /* parse 1st token */
        token = strsep(&cp, " \n");
        if (token == NULL) goto _api_debug_invalid;

        if (kstrtoul(token, 0, &value))
                goto _api_debug_invalid;

        switch (value) {
		case API_WMC_FLOODING_DISABLE:
			if (!g_wmc_flooding_enable) {
				printk("%s: g_wmc_flooding_enable is already disable!!!\n", __func__);
				return count;
			}
			break;
		case API_WMC_FLOODING_ENABLE:
		case API_WMC_FLOODING_ENABLE_WITH_DEFAULT_RULES:
			if (g_wmc_flooding_enable) {
				printk("%s: g_wmc_flooding_enable is already enable!!!\n", __func__);
				return count;
			}
			break;
		case API_WMC_FLOODING_ADD_ADDRESS:
		case API_WMC_FLOODING_ADD_V6_ADDRESS:
		case API_WMC_FLOODING_ADD:
		case API_WMC_FLOODING_UPDATE:
		case API_WMC_FLOODING_DELETE:
		case API_WMC_FLOODING_DELETE_ALL:
		case API_WMC_FLOODING_SET_FILTER_MODE:
		case API_WMC_FLOODING_SET_DEBUG_MODE:
			if (!g_wmc_flooding_enable) {
				printk("%s: g_wmc_flooding_enable is disable!!!\n", __func__);
				return count;
			}

			/* parse 1st token */
			token = strsep(&cp, "\n");
			if (token == NULL) goto _api_debug_invalid;
			break;
	}
	switch (value) {
                case API_WMC_FLOODING_STATUS:
                        /*Get status of well known multicast status*/
                        ca_wmc_flooding_show_table();
                        break;

                case API_WMC_FLOODING_DISABLE:
                        /*Disable well known multicast flooding*/
			wmc_flooding_disable();
			break;

		case API_WMC_FLOODING_ENABLE_WITH_DEFAULT_RULES:
			/*Enable well known multicast flooding with default rules*/
			wmc_flooding_enable(1);
                        break;

                case API_WMC_FLOODING_ENABLE:
                        /*Enable well known multicast flooding*/
			wmc_flooding_enable(0);
                        break;

                case API_WMC_FLOODING_ADD_ADDRESS:
                        /*Add well known multicast IPv4 address range*/
			ca_wmc_flooding_add_address(token, CA_IPV4);
                        break;

                case API_WMC_FLOODING_ADD_V6_ADDRESS:
                        /*Add well known multicast IPv6 address range*/
                        /* parse 1st token */
			ca_wmc_flooding_add_address(token, CA_IPV6);
			break;

                case API_WMC_FLOODING_ADD:
                        /*Add a flooding group with vlan ID*/
			ca_wmc_flooding_add(token);
                        break;

                case API_WMC_FLOODING_UPDATE:
                        /*Update a flooding group by vlan ID*/
			ca_wmc_flooding_update(token);
                        break;

                case API_WMC_FLOODING_DELETE:
                        /*Delete a flooding group*/
			ca_wmc_flooding_delete(token);
                        break;

                case API_WMC_FLOODING_DELETE_ALL:
                        /*Delete all flooding group*/
			ca_wmc_flooding_delete_all();
			break;

		case API_WMC_FLOODING_SET_FILTER_MODE:
			/*Set filter mode on offload side: 0 is disable, 1 is enable.*/
			ca_wmc_flooding_set_mode(token, CA_WMC_FLOODING_FILTER_IPC);
			break;


		case API_WMC_FLOODING_SET_DEBUG_MODE:
			/*Set debug mode on offload side: 0 is disable, 1 is enable.*/
			ca_wmc_flooding_set_mode(token, CA_WMC_FLOODING_DEBUG_IPC);
                        break;
        }

        return count;
_api_debug_invalid:
        pr_err("ERROR! Invalid or unsupported value '%s' for %s.\n", tmp, file->f_path.dentry->d_iname);
        /* if we return error code here, PROC fs may retry up to 3 times. */
        return count;
}

static const struct file_operations fops_api_wmc_flooding __read_mostly = {
        .owner		= THIS_MODULE,
        .open		= api_wmc_flooding_proc_open,
        .read		= seq_read,
        .write		= api_wmc_flooding_proc_write,
	.release	= single_release,
};

#endif

/***************************************************************************
 * init/exit functions
 ***************************************************************************/

int ca_api_proc_init(void)
{
	proc_api = proc_mkdir("api", ca_proc_dir);

	if (proc_create("debug", S_IRUGO | S_IWUGO, proc_api, &fops_api_debug) == NULL) {
		pr_err("Fail to create proc entry 'debug'.\n");
		return -ENOMEM;
	}

	if (proc_create("dump", S_IRUGO | S_IWUGO, proc_api, &fops_api_dump) == NULL) {
		pr_err("Fail to create proc entry 'dump'.\n");
		return -ENOMEM;
	}

#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)
	if (proc_create("wmc_flooding", S_IRUGO | S_IWUGO, proc_api, &fops_api_wmc_flooding) == NULL) {
		pr_err("Fail to create proc entry 'wmc_flooding'.\n");
		return -ENOMEM;
	}
#endif

        return 0;
}

void ca_api_proc_exit(void)
{
	remove_proc_entry("debug", proc_api);
	remove_proc_entry("dump", proc_api);
#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)
	remove_proc_entry("wmc_flooding", proc_api);
#endif
	proc_remove(proc_api);
}


