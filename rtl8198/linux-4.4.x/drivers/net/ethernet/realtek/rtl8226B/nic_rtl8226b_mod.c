#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>	/* mdelay() */
#include <fs/proc/internal.h>

#include "nic_rtl8226.h"
#include "nic_rtl8226b_init.h"

#ifdef CONFIG_LUNA_G3_SERIES

#include <aal_phy.h>

#define CA_AAL_MDIO_ST_CODE_C22		1
#define CA_AAL_MDIO_ST_CODE_C45		0

#define RTL8226B_RESET_GPIO_NO		129			/* GPIO4_[1]: 4 * 32 + 1 */
#define RTL8226B_RESET_GPIO_LABEL	"rtl8226b_reset"

/*
 * Default Serdes Mode (write bit [5:0] = 0/1/2/3  SDS mode)
 * 0: 2500Base-X/SGMII
 * 1: HiSGMII/SGMII
 * 2: 2500Base-X
 * 3: HiSGMII
 */
#define DEFAULT_SERDES_MODE		0x3

extern ca_status_t aal_mdio_write(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_IN      ca_uint16_t              data);

extern ca_status_t aal_mdio_read(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_OUT     ca_uint16_t             *data);

#else

#include <rtk/gpio.h>
//#include <rtk/port.h>		/* for rtk_port_serdesMode_get */
#include <rtk/mdio.h>
#include <common/rt_error.h>

//#define IO_GPIO_EN_REG		0xBB000038
#define IO_MODE_EN_REG			0xBB023014
#define MDIO_MASTER_EN			(1 << 10)
#define EXT_MDX_M_EN			(1 << 11)

#define RTL8226B_MDC_SW_GPIO_NO_6	6
#define RTL8226B_MDC_SW_GPIO_NO_10	10

#define DEFAULT_MDIO_PORT_NUM		0
#define RTK_MDIO_FMT_C22		0
#define RTK_MDIO_FMT_C45		1

/*
 * Default Serdes Mode (write bit [5:0] = 0/1/2/3  SDS mode)
 * 0: 2500Base-X/SGMII
 * 1: HiSGMII/SGMII
 * 2: 2500Base-X
 * 3: HiSGMII
 */
#define DEFAULT_SERDES_MODE		0x1

#define DEVICE0_SERDES_NUMBER		0
#define DEVICE1_SERDES_NUMBER		1

#endif

struct proc_dir_entry *rtl8226b_proc_dir = NULL;

#define PROC_DIR_RTL8226B		"rtl8226b"
#define PROC_FILE_HELP			"help"
#define PROC_FILE_PHY			"phy"
#define PROC_FILE_LINK_STATUS		"link_status"

/* Register access macro */
#ifndef REG32
#define REG32(reg)      (*((volatile unsigned int *)(reg)))
#endif
#ifndef REG16
#define REG16(reg)      (*((volatile unsigned short *)(reg)))
#endif
#ifndef REG8
#define REG8(reg)       (*((volatile unsigned char *)(reg)))
#endif

#if 0
//spinlock_t lock_mdio;
BOOLEAN
init_mdio_lock(void)
{
	printk("Init mdio lock\n");
	spin_lock_init(&lock_mdio);
	return SUCCESS;
}
#endif

BOOLEAN
MmdPhyRead(
    IN  HANDLE hDevice,
    IN  UINT16 dev,
    IN  UINT16 addr,
    OUT UINT16 *data)
{
#ifdef CONFIG_LUNA_G3_SERIES
	/**aal_mdio_read:
	*@breif: Read MDIO slaver device data with 16bits width
	*@param [in] st_code : Start of Frame (01 for Clause 22;00 for Clause 45)
	*@param [in] phy_port_addr : PHY Address, 0~31
	*@param [in] reg_dev_addr  : Register Address, 0~31
	*@param [in] addr  : extended address for Clause45, normally NOT used
	*@param [out] data : the Data returned from MDIO slaver
	*@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
	*/
	ca_status_t ret = CA_E_OK;

	ret = aal_mdio_read(hDevice.unit, CA_AAL_MDIO_ST_CODE_C45, hDevice.port, dev, addr , data);
	if (ret != CA_E_OK) {
		RTL8226B_ERROR("%s (dev = %d, addr = 0x%04x): data = 0x%04x - ret = %d (FAILED)\n",
			__FUNCTION__, dev, addr, *data, ret);
		return FAILURE;
	}
#else
    	int32 ret;

	rtk_gpio_state_set(hDevice.gpio, DISABLED);
	//spin_lock_bh(&lock_mdio);
	rtk_mdio_cfg_set(hDevice.unit, DEFAULT_MDIO_PORT_NUM, hDevice.port, RTK_MDIO_FMT_C45);
	ret = rtk_mdio_c45_read((uint8)dev, addr, data);
	if (ret == RT_ERR_FAILED) {
		RTL8226B_ERROR("%s (dev = %d, addr = 0x%04x): data = 0x%04x - ret = %d (FAILED)\n",
			__FUNCTION__, dev, addr, *data, ret);
		return FAILURE;
	}
	//spin_unlock_bh(&lock_mdio);
	//RTL8226B_DEBUG("%s (dev = %d, addr = 0x%04x): data = 0x%04x - ret = %d (OK)\n",
	//	__FUNCTION__, dev, addr, *data, ret);
#endif
	return SUCCESS;
}

BOOLEAN
MmdPhyWrite(
    IN HANDLE hDevice,
    IN UINT16 dev,
    IN UINT16 addr,
    IN UINT16 data)
{
#ifdef CONFIG_LUNA_G3_SERIES
	/**aal_mdio_write:
	*@breif: Write data to MDIO slaver device with 16bits width
	*@param [in] st_code : Start of Frame (01 for Clause 22;00 for Clause 45)
	*@param [in] phy_port_addr : PHY Address, 0~31
	*@param [in] reg_dev_addr  : Register Address, 0~31
	*@param [in] addr  : extended address for Clause45, normally NOT used
	*@param [in] data  : the Data writen to MDIO slaver
	*@return: CA_E_OK if successfully, otherwise return CA_E_PARAM
	*/
	ca_status_t ret = CA_E_OK;

	ret = aal_mdio_write(hDevice.unit, CA_AAL_MDIO_ST_CODE_C45, hDevice.port, dev, addr, data);
	if (ret != CA_E_OK) {
		RTL8226B_ERROR("%s (dev = %d, addr = 0x%04x, data = 0x%04x): - ret = %d (FAILED)\n",
			__FUNCTION__, dev, addr, data, ret);
		return FAILURE;
	}
#else
    	int32   ret;

	rtk_gpio_state_set(hDevice.gpio, DISABLED);
	//spin_lock_bh(&lock_mdio);
	rtk_mdio_cfg_set(hDevice.unit, DEFAULT_MDIO_PORT_NUM, hDevice.port, RTK_MDIO_FMT_C45);
	ret = rtk_mdio_c45_write((uint8)dev, addr, data);
	if (ret == RT_ERR_FAILED) {
		RTL8226B_ERROR("%s (dev = %d, addr = 0x%04x, data = 0x%04x): - ret = %d (FAILED)\n",
			__FUNCTION__, dev, addr, data, ret);
		return FAILURE;
	}
	//spin_unlock_bh(&lock_mdio);
	//RTL8226B_DEBUG("%s (dev = %d, addr = 0x%04x, data = 0x%04x): - ret = %d (OK)\n",
	//		__FUNCTION__, dev, addr, data, ret);
#endif
	return SUCCESS;
}

void rtl8226b_usage(const char *filename)
{
	RTL8226B_MSG("[Usage]");

	if((strcasecmp(filename, PROC_FILE_PHY) == 0) || (strcasecmp(filename, PROC_FILE_HELP) == 0)) {
		RTL8226B_MSG("echo [$action] [$phyad]> /proc/%s/%s", PROC_DIR_RTL8226B, PROC_FILE_PHY);
		RTL8226B_MSG("\t$action: {init,reset}");
		RTL8226B_MSG("\t$phyad: rtl8226b phy address");
		if(strcasecmp(filename, PROC_FILE_PHY) == 0)		return;
	}

	if((strcasecmp(filename, PROC_FILE_LINK_STATUS) == 0) || (strcasecmp(filename, PROC_FILE_HELP) == 0)) {
		RTL8226B_MSG("cat /proc/%s/%s", PROC_DIR_RTL8226B, PROC_FILE_LINK_STATUS);
		if(strcasecmp(filename, PROC_FILE_LINK_STATUS) == 0)		return;
	}

	return;
}

/*
 * rtl8226b proc function
 */
#define INVALID_VALUE	99
#define MAX_COMMAND_LEN	32

static HANDLE hDevice0;
static HANDLE hDevice1;

static int help_fops(struct seq_file *s, void *v)
{
	rtl8226b_usage(PROC_FILE_HELP);
	return SUCCESS;
}

static int Rtl8226b_phy_fops(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmpbuf[MAX_COMMAND_LEN] = {0};
	int len = (count > MAX_COMMAND_LEN) ? (MAX_COMMAND_LEN -1) : count;
	static PHY_LINK_ABILITY phylinkability;
	int phyad;

	BOOL status = FAILURE;

	RTL8226B_DEBUG("%s - %s: %s (%d) - %s", file->f_path.dentry->d_iname, __FUNCTION__, buffer, count);
	if (buffer)
	{
		char *strptr, *split_str;

		/* copy data to the buffer */
		strncpy(tmpbuf, buffer, len);
		tmpbuf[len] = '\0';

		strptr = tmpbuf;
		RTL8226B_DEBUG("strptr: [%s]", strptr);

		/*parse command*/
		split_str = strsep(&strptr," ");
		RTL8226B_DEBUG("split_str [%s]", split_str);

		if(strcasecmp(split_str, "init") == 0)
		{
			BOOL singlephy = 1;
			split_str = strsep(&strptr," ");
			phyad = simple_strtol(split_str, NULL, 0);

			if (phyad == hDevice0.port)
				status = Rtl8226b_phy_init(hDevice0, &phylinkability ,singlephy);
			else if (phyad == hDevice1.port)
				status = Rtl8226b_phy_init(hDevice1, &phylinkability ,singlephy);
			else {
				RTL8226B_ERROR("UNKNOWN phyad %d", phyad);
				goto phy_error;
			}

			if (status == SUCCESS)
				RTL8226B_MSG("Rtl8226b_phy_init Done (phyad %d)", phyad);
			else
				RTL8226B_ERROR("Rtl8226b_phy_init Fail (phyad %d)", phyad);
		}
		else if(strcasecmp(split_str, "reset") == 0)
		{
			split_str = strsep(&strptr," ");
			phyad = simple_strtol(split_str, NULL, 0);

			if (phyad == hDevice0.port)
				status = Rtl8226_phy_reset(hDevice0);
			else if (phyad == hDevice1.port)
				status = Rtl8226_phy_reset(hDevice1);
			else {
				RTL8226B_ERROR("UNKNOWN phyad %d", phyad);
				goto phy_error;
			}

			if (status == SUCCESS)
				RTL8226B_MSG("Rtl8226b_phy_reset Done (phyad %d)", phyad);
			else
				RTL8226B_ERROR("Rtl8226_phy_reset Fail (phyad %d)", phyad);
		}else
		{
			goto phy_error;
		}
	}
	return count;

phy_error:
	RTL8226B_ERROR("%s() FAIL ......", __FUNCTION__);
	rtl8226b_usage(file->f_path.dentry->d_iname);
	return -EFAULT;
}

static int Rtl8226_link_status_fops(struct seq_file *s, void *v)
{
#if (defined(CONFIG_RTL_8226B_DEVICE_0) || defined(CONFIG_RTL_8226B_DEVICE_1))
	BOOL status = FAILURE;
	BOOL plinkok = INVALID_VALUE;
    	UINT16 pSpeed;

	BOOL perdesLink;
	PHY_SERDES_MODE SerdesMode;

	char sDesc[16];
#endif

#ifdef CONFIG_RTL_8226B_DEVICE_0
	status = Rtl8226_is_link(hDevice0, &plinkok);
	if (status == SUCCESS) {
		RTL8226B_MSG("Rtl8226_is_link(phyAddr %d): %d [%s]", hDevice0.port, plinkok, plinkok ? "link" : "not link");

		status = Rtl8226_speed_get(hDevice0, &pSpeed);
		if (status == SUCCESS) {
			switch (pSpeed) {
			case LINK_SPEED_10M:
				strncpy(sDesc, "10M\0", 4);
				break;
			case LINK_SPEED_100M:
				strncpy(sDesc, "100M\0", 5);
				break;
			case LINK_SPEED_500M:
				strncpy(sDesc, "500M\0", 5);
				break;
			case LINK_SPEED_1G:
				strncpy(sDesc, "1G\0", 3);
				break;
			case LINK_SPEED_2P5G:
				strncpy(sDesc, "2.5G\0", 5);
				break;
			case NO_LINK:
				strncpy(sDesc, "nolink", 7);
				break;
			default:
				strncpy(sDesc, "unknown\0", 8);
				break;
			}
			RTL8226B_MSG("Rtl8226_speed_get(phyAddr %d): %d [%s]", hDevice0.port, pSpeed, sDesc);

			status = Rtl8226_serdes_link_get(hDevice0, &perdesLink, &SerdesMode);
			if (status == SUCCESS) {
				switch (SerdesMode) {
				case PHY_SERDES_MODE_SGMII:
					strncpy(sDesc, "SGMII\0", 6);
					break;
				case PHY_SERDES_MODE_USXGMII:
					strncpy(sDesc, "USXGMII\0", 8);
					break;
				case PHY_SERDES_MODE_HiSGMII:
					strncpy(sDesc, "HiSGMII\0", 8);
					break;
				case PHY_SERDES_MODE_2500BASEX:
					strncpy(sDesc, "2500BASEX\0", 10);
					break;
				case PHY_SERDES_MODE_NO_SDS:
				case PHY_SERDES_MODE_OTHER:
				default:
					strncpy(sDesc, "unknown\0", 8);
					break;
				}
				RTL8226B_MSG("Rtl8226_serdes_link_get(phyAddr %d): %d / %d [%s]", hDevice0.port, perdesLink, pSpeed, sDesc);
			} else {
				RTL8226B_ERROR("Rtl8226_serdes_link_get(phyAddr %d) Fail, [status = %d]", hDevice0.port, status);
				goto link_status_error;
			}
		} else {
			RTL8226B_ERROR("Rtl8226_speed_get(phyAddr %d) Fail, [status = %d]", hDevice0.port, status);
			goto link_status_error;
		}
	} else {
		RTL8226B_ERROR("Rtl8226_is_link(phyAddr %d) Fail, [status = %d]", hDevice0.port, status);
		goto link_status_error;
	}
#endif

#ifdef CONFIG_RTL_8226B_DEVICE_1
	status = Rtl8226_is_link(hDevice1, &plinkok);
	if (status == SUCCESS) {
		RTL8226B_MSG("Rtl8226_is_link(phyAddr %d): %d [%s]", hDevice1.port, plinkok, plinkok ? "link" : "not link");

		status = Rtl8226_speed_get(hDevice1, &pSpeed);
		if (status == SUCCESS) {
			switch (pSpeed) {
			case LINK_SPEED_10M:
				strncpy(sDesc, "10M\0", 4);
				break;
			case LINK_SPEED_100M:
				strncpy(sDesc, "100M\0", 5);
				break;
			case LINK_SPEED_500M:
				strncpy(sDesc, "500M\0", 5);
				break;
			case LINK_SPEED_1G:
				strncpy(sDesc, "1G\0", 3);
				break;
			case LINK_SPEED_2P5G:
				strncpy(sDesc, "2.5G\0", 5);
				break;
			case NO_LINK:
				strncpy(sDesc, "nolink", 7);
				break;
			default:
				strncpy(sDesc, "unknown\0", 8);
				break;
			}
			RTL8226B_MSG("Rtl8226_speed_get(phyAddr %d): %d [%s]", hDevice1.port, pSpeed, sDesc);

			status = Rtl8226_serdes_link_get(hDevice1, &perdesLink, &SerdesMode);
			if (status == SUCCESS) {
				switch (SerdesMode) {
				case PHY_SERDES_MODE_SGMII:
					strncpy(sDesc, "SGMII\0", 6);
					break;
				case PHY_SERDES_MODE_USXGMII:
					strncpy(sDesc, "USXGMII\0", 8);
					break;
				case PHY_SERDES_MODE_HiSGMII:
					strncpy(sDesc, "HiSGMII\0", 8);
					break;
				case PHY_SERDES_MODE_2500BASEX:
					strncpy(sDesc, "2500BASEX\0", 10);
					break;
				case PHY_SERDES_MODE_NO_SDS:
				case PHY_SERDES_MODE_OTHER:
				default:
					strncpy(sDesc, "unknown\0", 8);
					break;
				}
				RTL8226B_MSG("Rtl8226_serdes_link_get(phyAddr %d): %d / %d [%s]", hDevice1.port, perdesLink, pSpeed, sDesc);
			} else {
				RTL8226B_ERROR("Rtl8226_serdes_link_get(phyAddr %d) Fail, [status = %d]", hDevice0.port, status);
				goto link_status_error;
			}

		} else {
			RTL8226B_ERROR("Rtl8226_speed_get_(phyAddr %d) Fail, [status = %d]", hDevice1.port, status);
			goto link_status_error;
		}
	} else {
		RTL8226B_ERROR("Rtl8226_is_link(phyAddr %d) Fail, [status = %d]", hDevice1.port, status);
		goto link_status_error;
	}

#endif
	return SUCCESS;

link_status_error:
	RTL8226B_ERROR("%s() FAIL ......", __FUNCTION__);
	rtl8226b_usage(PROC_FILE_LINK_STATUS);
	return -EFAULT;
}

/*
 * rtl8226b proc definition
 */
typedef enum rtk_rtl8226b_procDir_e
{
	PROC_DIR_RTL8226B_ROOT,
	PROC_DIR_RTL8226B_LEEF //this field must put at last.
} rtk_rtl8226b_procDir_t;

typedef struct rtk_rtl8226b_proc_s
{
	char *name;
	int (*get) (struct seq_file *s, void *v);
	int (*set) (struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
	unsigned int inode_id;
	unsigned char unlockBefortWrite;
	struct file_operations proc_fops;
	rtk_rtl8226b_procDir_t dir;
}rtk_rtl8226b_proc_t;

rtk_rtl8226b_proc_t rtl8226bProc[]=
{
	{
		.name= PROC_FILE_HELP,
		.get = help_fops,
		.set = NULL,
		.dir = PROC_DIR_RTL8226B_ROOT,
	},
	{
		.name= PROC_FILE_PHY,
		.get = NULL,
		.set = Rtl8226b_phy_fops,
		.dir = PROC_DIR_RTL8226B_ROOT,
	},
	{
		.name= PROC_FILE_LINK_STATUS,
		.get = Rtl8226_link_status_fops,
		.set = NULL,
		.dir = PROC_DIR_RTL8226B_ROOT,
	},
};

/*
 * rtl8226b common proc function
 */
static void *rtk_rtl8226b_single_start(struct seq_file *p, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *rtk_rtl8226b_single_next(struct seq_file *p, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void rtk_rtl8226b_single_stop(struct seq_file *p, void *v)
{
}

int rtk_rtl8226b_seq_open(struct file *file, const struct seq_operations *op)
{
	struct seq_file *p = file->private_data;

	if (!p) {
		p = kmalloc(sizeof(*p), GFP_ATOMIC);
		if (!p){
			return -ENOMEM;
		}
		file->private_data = p;
	}
	memset(p, 0, sizeof(*p));
	mutex_init(&p->lock);
	p->op = op;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 79)
	p->file = file;
#else
#ifdef CONFIG_USER_NS
	p->user_ns = file->f_cred->user_ns;
#endif
#endif
	/*
	 * Wrappers around seq_open(e.g. swaps_open) need to be
	 * aware of this. If they set f_version themselves, they
	 * should call seq_open first and then set f_version.
	 */
	file->f_version = 0;

	/*
	 * seq_files support lseek() and pread().  They do not implement
	 * write() at all, but we clear FMODE_PWRITE here for historical
	 * reasons.
	 *
	 * If a client of seq_files a) implements file.write() and b) wishes to
	 * support pwrite() then that client will need to implement its own
	 * file.open() which calls seq_open() and then sets FMODE_PWRITE.
	 */
	file->f_mode &= ~FMODE_PWRITE;
	return 0;
}


int rtk_rtl8226b_single_open(struct file *file, int (*show) (struct seq_file *m, void *v), void *data)
{
	struct seq_operations *op = kmalloc(sizeof(*op), GFP_ATOMIC);
	int res = -ENOMEM;

	if (op) {
		op->start = rtk_rtl8226b_single_start;
		op->next = rtk_rtl8226b_single_next;
		op->stop = rtk_rtl8226b_single_stop;
		op->show = show;
		res = rtk_rtl8226b_seq_open(file, op);
		if (!res)
			((struct seq_file *)file->private_data)->private = data;
		else
			kfree(op);
	}
	return res;
}

static int rtk_rtl8226b_nullDebugGet(struct seq_file *s, void *v)
{
	return 0;
}

static int rtk_rtl8226b_nullDebugSingleOpen(struct inode *inode, struct file *file)
{
       return(single_open(file, rtk_rtl8226b_nullDebugGet, NULL));
}

static int rtk_rtl8226b_commonDebugSingleOpen(struct inode *inode, struct file *file)
{
	int i, ret = -1;

	//rtl8226b_spin_lock_bh(rtl8226bSysdb.lock_rtl8226b);
	//========================= Critical Section Start =========================//
	for( i = 0; i < (sizeof(rtl8226bProc) / sizeof(rtk_rtl8226b_proc_t)) ; i++) {
		//RTL8226B_MSG("common_single_open inode_id=%u i_ino=%u", rtl8226bProc[i].inode_id,(unsigned int)inode->i_ino);

		if(rtl8226bProc[i].inode_id == (unsigned int)inode->i_ino) {
			ret = rtk_rtl8226b_single_open(file, rtl8226bProc[i].get, NULL);
			break;
		}
	}
	//========================= Critical Section End =========================//
	//rtl8226b_spin_unlock_bh(rtl8226bSysdb.lock_rtl8226b);

	return ret;
}

static ssize_t rtk_rtl8226b_commonDebugSingleWrite(struct file * file, const char __user * userbuf,
				size_t count, loff_t * off)
{
	int i, ret = -1;
	char procBuffer[count];
	char *pBuffer = NULL;

	/* write data to the buffer */
	memset(procBuffer, 0, sizeof(procBuffer));
	if (copy_from_user(procBuffer, userbuf, count)) {
		return -EFAULT;
	}
	procBuffer[count-1] = '\0';
	pBuffer = procBuffer;

	//rtl8226b_spin_lock_bh(rtl8226bSysdb.lock_rtl8226b);
	//========================= Critical Section Start =========================//
	for( i = 0; i < (sizeof(rtl8226bProc) / sizeof(rtk_rtl8226b_proc_t)) ; i++) {
		//RTL8226B_MSG("common_single_write inode_id=%u i_ino=%u",rtl8226bProc[i].inode_id,(unsigned int)file->f_dentry->d_inode->i_ino);

		if(rtl8226bProc[i].inode_id == (unsigned int)file->f_inode->i_ino) {
			//if(rtl8226bProc[i].unlockBefortWrite)
			//	rtl8226b_spin_unlock_bh(rtl8226bSysdb.lock_rtl8226b);
			ret = rtl8226bProc[i].set(file, pBuffer, count, off);
			break;
		}
	}
	//========================= Critical Section End =========================//
	//if((i!=(sizeof(rtl8226bProc)/sizeof(rtk_rtl8226b_proc_t))) && !rtl8226bProc[i].unlockBefortWrite)
	//	rtl8226b_spin_unlock_bh(rtl8226bSysdb.lock_rtl8226b);

	return ret;
}

BOOLEAN rtk_rtl8226b_proc_init(void)
{
	int i = 0;
	struct proc_dir_entry *p = NULL;

	rtl8226b_proc_dir = proc_mkdir(PROC_DIR_RTL8226B, NULL);
	if (rtl8226b_proc_dir == NULL) {
		RTL8226B_ERROR("create /proc/%s failed!", PROC_DIR_RTL8226B);
		return FAILURE;
	}

	for(i = 0; i < (sizeof(rtl8226bProc)/sizeof(rtk_rtl8226b_proc_t)) ; i++)
	{
		struct proc_dir_entry *parentDir = NULL;

		if(rtl8226bProc[i].get == NULL)
			rtl8226bProc[i].proc_fops.open = rtk_rtl8226b_nullDebugSingleOpen;
		else
			rtl8226bProc[i].proc_fops.open = rtk_rtl8226b_commonDebugSingleOpen;

		if(rtl8226bProc[i].set == NULL)
			rtl8226bProc[i].proc_fops.write = NULL;
		else
			rtl8226bProc[i].proc_fops.write = rtk_rtl8226b_commonDebugSingleWrite;

		rtl8226bProc[i].proc_fops.read = seq_read;
		rtl8226bProc[i].proc_fops.llseek = seq_lseek;
		rtl8226bProc[i].proc_fops.release = single_release;

		switch(rtl8226bProc[i].dir)
		{
			case PROC_DIR_RTL8226B_ROOT:
				parentDir = rtl8226b_proc_dir;
				break;
			default:
				break;
		}

		p = proc_create_data(rtl8226bProc[i].name, S_IRUGO, parentDir, &(rtl8226bProc[i].proc_fops),NULL);
		if(!p){
			RTL8226B_ERROR("create proc %s failed!", rtl8226bProc[i].name);
		}
		rtl8226bProc[i].inode_id = p->low_ino;
	}
	RTL8226B_MSG("Creat %d proc entry.", i);

	return SUCCESS;
}


void rtk_rtl8226b_proc_exit(void)
{
	int i = 0;

	for(i = 0 ; i < (sizeof(rtl8226bProc) / sizeof(rtk_rtl8226b_proc_t)) ; i++)
	{
		struct proc_dir_entry *parentDir = NULL;

		switch(rtl8226bProc[i].dir)
		{
			case PROC_DIR_RTL8226B_ROOT:
				parentDir = rtl8226b_proc_dir;
				break;
			default:
				break;
		}

		remove_proc_entry(rtl8226bProc[i].name, parentDir);
	}

	proc_remove(rtl8226b_proc_dir);
}

static struct task_struct *rtl8226b_device_init_task;
#define RTL8226B_DEVICE_INIT_KTHREAD_CPU	0

static int rtk_rtl8226b_device_init_thread (void *data)
{
#if (defined(CONFIG_RTL_8226B_DEVICE_0) || defined(CONFIG_RTL_8226B_DEVICE_1))
	BOOL status = FAILURE;
	BOOL singlephy = 1;
	static PHY_LINK_ABILITY phylinkability;
	//uint8 cfg;
	//char sSdsMode[16];
#endif
#ifdef CONFIG_RTL_8226B_DEVICE_0
	RTL8226B_DEBUG("Init Device 0 (mdio set: %d, phyad: %d)", hDevice0.unit, hDevice0.port);
	status = Rtl8226b_phy_init(hDevice0, &phylinkability ,singlephy);
	if (status == SUCCESS) {
		RTL8226B_INFO("Init Device 0 Done.");
#if 0
		rtk_port_serdesMode_get(DEVICE0_SERDES_NUMBER, &cfg);
		switch(cfg) {
		case LAN_SDS_MODE_GE_FE_PHY:
			strncpy(sSdsMode, "GE_FE_PHY\0", 10);
			break;
		case LAN_SDS_MODE_FIBER_1G:
			strncpy(sSdsMode, "FIBER_1G\0", 9);
			break;
		case LAN_SDS_MODE_SGMII_PHY:
			strncpy(sSdsMode, "SGMII_PHY\0", 10);
			break;
		case LAN_SDS_MODE_SGMII_MAC:
			strncpy(sSdsMode, "SGMII_MAC\0", 10);
			break;
		case LAN_SDS_MODE_HSGMII_PHY:
			strncpy(sSdsMode, "HSGMII_PHY\0", 11);
			break;
		case LAN_SDS_MODE_HSGMII_MAC:
			strncpy(sSdsMode, "HSGMII_MAC\0", 11);
			break;
		case LAN_SDS_MODE_2500BASEX_PHY:
			strncpy(sSdsMode, "2500BASEX_PHY\0", 14);
			break;
		case LAN_SDS_MODE_2500BASEX_MAC:
			strncpy(sSdsMode, "2500BASEX_MAC\0", 14);
			break;
		case LAN_SDS_MODE_END:
		default:
			strncpy(sSdsMode, "UNKNOWN Mode\0", 13);
			break;
		}
		RTL8226B_INFO("Device 0 Serdes Mode: %s(%d)", sSdsMode, cfg);
#endif
		if ((status = Rtl8226_serdes_option_set(hDevice0, DEFAULT_SERDES_MODE /* HiSGMII/SGMII */)) != SUCCESS)
			RTL8226B_ERROR("Device0 unit %u port %u RTL8226 Rtl8226_serdes_option_set status = 0x%x", hDevice0.unit, hDevice0.port, status);
		else
			RTL8226B_DEBUG("Device0 unit %u port %u RTL8226 Rtl8226_serdes_option_set status = 0x%x", hDevice0.unit, hDevice0.port, status);

	}
	else
		RTL8226B_ERROR("Init Device 0 Fail.");
#endif
#ifdef CONFIG_RTL_8226B_DEVICE_1
	RTL8226B_DEBUG("Init Device 1 (mdio set: %d, phyad: %d)", hDevice1.unit, hDevice1.port);
	status = Rtl8226b_phy_init(hDevice1, &phylinkability ,singlephy);
	if (status == SUCCESS) {
		RTL8226B_INFO("Init Device 1 Done.");
#if 0
		rtk_port_serdesMode_get(DEVICE1_SERDES_NUMBER, &cfg);
		switch(cfg) {
		case LAN_SDS_MODE_GE_FE_PHY:
			strncpy(sSdsMode, "GE_FE_PHY\0", 10);
			break;
		case LAN_SDS_MODE_FIBER_1G:
			strncpy(sSdsMode, "FIBER_1G\0", 9);
			break;
		case LAN_SDS_MODE_SGMII_PHY:
			strncpy(sSdsMode, "SGMII_PHY\0", 10);
			break;
		case LAN_SDS_MODE_SGMII_MAC:
			strncpy(sSdsMode, "SGMII_MAC\0", 10);
			break;
		case LAN_SDS_MODE_HSGMII_PHY:
			strncpy(sSdsMode, "HSGMII_PHY\0", 11);
			break;
		case LAN_SDS_MODE_HSGMII_MAC:
			strncpy(sSdsMode, "HSGMII_MAC\0", 11);
			break;
		case LAN_SDS_MODE_2500BASEX_PHY:
			strncpy(sSdsMode, "2500BASEX_PHY\0", 14);
			break;
		case LAN_SDS_MODE_2500BASEX_MAC:
			strncpy(sSdsMode, "2500BASEX_MAC\0", 14);
			break;
		case LAN_SDS_MODE_END:
		default:
			strncpy(sSdsMode, "UNKNOWN Mode\0", 13);
			break;
		}
		RTL8226B_INFO("Device 1 Serdes Mode: %s(%d)", sSdsMode, cfg);
#endif
		if ((status = Rtl8226_serdes_option_set(hDevice1, DEFAULT_SERDES_MODE /* HiSGMII/SGMII */)) != SUCCESS)
			RTL8226B_ERROR("Device1 unit %u port %u RTL8226 Rtl8226_serdes_option_set status = 0x%x", hDevice1.unit, hDevice1.port, status);
		else
			RTL8226B_DEBUG("Device1 unit %u port %u RTL8226 Rtl8226_serdes_option_set status = 0x%x", hDevice1.unit, hDevice1.port, status);
	}
	else
		RTL8226B_ERROR("Init Device 1 Fail.");
#endif
	return 0;
}

unsigned int io_mode_en_value_backup;

static int __init rtk_rtl8226b_moudle_init(void)
{
	RTL8226B_MSG("%s", __FUNCTION__);
	hDevice0.unit = INVALID_VALUE;
	hDevice0.port = INVALID_VALUE;
	hDevice0.gpio = INVALID_VALUE;
	hDevice1.unit = INVALID_VALUE;
	hDevice1.port = INVALID_VALUE;
	hDevice1.gpio = INVALID_VALUE;

	//BOOL status = FAILURE;
	//BOOL singlephy = 1;
	//static PHY_LINK_ABILITY phylinkability;

#if (!defined(CONFIG_RTL_8226B_DEVICE_0) && !defined(CONFIG_RTL_8226B_DEVICE_1))
	RTL8226B_ERROR("No RTL8226B Device Found !!!");
	return -ENODEV;
#endif

#ifdef CONFIG_RTL_8226B_DEVICE_0
	hDevice0.unit = CONFIG_RTL_8226B_DEVICE_0_MDIO_SET;
	hDevice0.port = CONFIG_RTL_8226B_DEVICE_0_PHY_ADDR;
	RTL8226B_MSG("RTL8226B Device 0 (mdio set: %d, phyad: %d)", hDevice0.unit, hDevice0.port);
#endif

#ifdef CONFIG_RTL_8226B_DEVICE_1
	hDevice1.unit = CONFIG_RTL_8226B_DEVICE_1_MDIO_SET;
	hDevice1.port = CONFIG_RTL_8226B_DEVICE_1_PHY_ADDR;
	RTL8226B_MSG("RTL8226B Device 1 (mdio set: %d, phyad: %d)", hDevice1.unit, hDevice1.port);
#endif

#if (defined(CONFIG_RTL_8226B_DEVICE_0) && defined(CONFIG_RTL_8226B_DEVICE_1))
	if ((hDevice0.unit == hDevice1.unit) && (hDevice0.port == hDevice1.port)){
		RTL8226B_ERROR("RTL8226B Device 0 and Device 1 are using same MDIO Set (%d) with Same PHY Add (%d)", hDevice0.unit, hDevice0.port);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_LUNA_G3_SERIES
	int ret;

	/* Reset RTL8226B */
	ret = gpio_is_valid(RTL8226B_RESET_GPIO_NO);
	if (ret != 1) {
		RTL8226B_ERROR("GPIO #%d is NOT Valid !!! (ret = %d)", RTL8226B_RESET_GPIO_NO, ret);
		return -EINVAL;
	}
	ret = gpio_request(RTL8226B_RESET_GPIO_NO, RTL8226B_RESET_GPIO_LABEL);
	if (ret < 0) {
		RTL8226B_ERROR("Request GPIO #%d Error !!! (ret = %d)", RTL8226B_RESET_GPIO_NO, ret);
		return -EINVAL;
	}
	ret = gpio_direction_output(RTL8226B_RESET_GPIO_NO, 1);
	if (ret != 0) {
		RTL8226B_ERROR("Set GPIO #%d Output as 1 Error !!! (ret = %d)", RTL8226B_RESET_GPIO_NO, ret);
		return -EINVAL;
	}
	//gpio_set_value(RTL8226B_RESET_GPIO_NO, 0);
	//mdelay(200);
	//gpio_set_value(RTL8226B_RESET_GPIO_NO, 1);
	gpio_free(RTL8226B_RESET_GPIO_NO);
	mdelay(300);		/* Waitting rtl8226b reset ??? */

#else
	/* MDIO init. */
	io_mode_en_value_backup = REG32(IO_MODE_EN_REG);
#ifdef CONFIG_RTL_8226B_DEVICE_0
	if(CONFIG_RTL_8226B_DEVICE_0_MDIO_SET == 0){
		RTL8226B_INFO("Enable MDIO master PAD_LED0/PAD_LED1 for 8226B device 0......");
		hDevice0.gpio = RTL8226B_MDC_SW_GPIO_NO_6;
		REG32(IO_MODE_EN_REG) |= MDIO_MASTER_EN;
	} else if(CONFIG_RTL_8226B_DEVICE_0_MDIO_SET == 1){
		RTL8226B_INFO("Enable MDIO master PAD_LED4/PAD_LED5 for 8226B device 0......");
		hDevice0.gpio = RTL8226B_MDC_SW_GPIO_NO_10;
		REG32(IO_MODE_EN_REG) |= EXT_MDX_M_EN;
	}
	rtk_gpio_state_set(hDevice0.gpio, DISABLED);
	//REG32(IO_GPIO_EN_REG + ((RTL8226B_MDC_SW_GPIO_NO >> 5) << 2)) &= ~(1 << (RTL8226B_MDC_SW_GPIO_NO % 32));
#endif

#ifdef CONFIG_RTL_8226B_DEVICE_1
	if(CONFIG_RTL_8226B_DEVICE_1_MDIO_SET == 0){
		RTL8226B_INFO("Enable MDIO master PAD_LED0/PAD_LED1 for 8226B device 1......");
		hDevice1.gpio = RTL8226B_MDC_SW_GPIO_NO_6;
		REG32(IO_MODE_EN_REG) |= MDIO_MASTER_EN;
	} else if(CONFIG_RTL_8226B_DEVICE_1_MDIO_SET == 1){
		RTL8226B_INFO("Enable MDIO master PAD_LED4/PAD_LED5 for 8226B device 1......");
		hDevice1.gpio = RTL8226B_MDC_SW_GPIO_NO_10;
		REG32(IO_MODE_EN_REG) |= EXT_MDX_M_EN;
	}
	rtk_gpio_state_set(hDevice1.gpio, DISABLED);
#endif
	rtk_mdio_init();
	//init_mdio_lock();
#endif

#if 0
	RTL8226B_MSG("Init Device 0 (mdio set: %d, phyad: %d)", hDevice0.unit, hDevice0.port);
	status = Rtl8226b_phy_init(hDevice0, &phylinkability ,singlephy);
	if (status == SUCCESS)
		RTL8226B_INFO("Init Device 0 done.");
	else
		RTL8226B_ERROR("Init Device 0 fail.");

	RTL8226B_MSG("Init Device 1 (mdio set: %d, phyad: %d)", hDevice1.unit, hDevice1.port);
	status = Rtl8226b_phy_init(hDevice1, &phylinkability ,singlephy);
	if (status == SUCCESS)
		RTL8226B_INFO("Init Device 1 done.");
	else
		RTL8226B_ERROR("Init Device 1 fail.");

#else
	/* Create rtl8226b phy init kthread */
	rtl8226b_device_init_task = kthread_create(rtk_rtl8226b_device_init_thread, NULL, "rtl8226b_device_init/%d", RTL8226B_DEVICE_INIT_KTHREAD_CPU);
	if (WARN_ON(!rtl8226b_device_init_task)) {
		RTL8226B_ERROR("Create rtl8226b_device1_init/%d failed!", RTL8226B_DEVICE_INIT_KTHREAD_CPU);
	}
	kthread_bind(rtl8226b_device_init_task, RTL8226B_DEVICE_INIT_KTHREAD_CPU);
	wake_up_process(rtl8226b_device_init_task);
#endif

	rtk_rtl8226b_proc_init();

	return 0;
}

static void __exit rtk_rtl8226b_module_exit(void)
{
#ifndef CONFIG_LUNA_G3_SERIES
	REG32(IO_MODE_EN_REG)		= io_mode_en_value_backup;
#endif
	rtk_rtl8226b_proc_exit();

	RTL8226B_MSG("%s\n", __FUNCTION__);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTL8226B Module");
MODULE_AUTHOR("Realtek.com");

module_init(rtk_rtl8226b_moudle_init);
module_exit(rtk_rtl8226b_module_exit);

