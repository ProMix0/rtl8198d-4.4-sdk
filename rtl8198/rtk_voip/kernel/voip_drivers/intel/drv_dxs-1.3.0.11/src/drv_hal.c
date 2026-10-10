#include<linux/init.h>

#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include "drv_dxs_api.h"
#include "spi.h"
// local variable

// local function

// external variable
//#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101)
//rtl_spi_dev_t spi_dev[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_NR];
//#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
//rtl_spi_dev_t spi_dev[CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_NR];
//#endif
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
extern int ca77xx_spi_transfer_one(struct spi_master *master,
				   struct spi_device *spi,
				   struct spi_transfer *tfr);
#else
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101)
static int HWCSNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_PIN_CS;
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102)
static int HWCSNum = CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS102_PIN_CS;
#endif

// external function
extern int32 _rtl_spi_rawRead( rtl_spi_dev_t* pDev, void* pData, int32 bits );
extern int32 _rtl_spi_rawWrite( rtl_spi_dev_t* pDev, void* pData, int32 bits );
#endif

/* SPI uninitialisation */
#define IFX_SSC_HANDLE int
/* Set callback mode because HW will drive the CS line ifself. */
#define DXS_USE_SPI_CS_CALLBACK 

#if 0
IFX_void_t USR_EXT_SPI_DevSet (rtl_spi_dev_t *pDev, IFX_uint16_t nDev, IFX_highLow_t high_low)
#else
IFX_void_t USR_EXT_SPI_DevSet (IFX_uint16_t nDev, IFX_highLow_t high_low)
#endif
{
	_rtl_generic_setGpioDataBit(nDev, high_low);
}

int USR_EXT_SPI_xfer (rtl_spi_dev_t *spi_dev, IFX_uint8_t* txptr, IFX_uint32_t txsize,  IFX_uint8_t* rxptr, IFX_uint32_t rxsize)
{
	int byteCnt = 0;
	unsigned long flags;
	
#if !defined(CONFIG_RTK_VOIP_LINUX_SPI) || defined(DXS_SPI_READ_HALF_DUPLEX)
	if ( (txptr != NULL) && (txsize != 0) )
	{
		//printk("Write to Slic :");
        for (byteCnt = 0; byteCnt < txsize; byteCnt++)
        {
		   _rtl_spi_rawWrite( spi_dev, &txptr[byteCnt], 8);
			/*if(byteCnt == (txsize-1))
				printk("%02x\n", txptr[byteCnt]);
			else
				printk("%02x ", txptr[byteCnt]);*/
        }
	}
	
	if ( rxptr != NULL )
	{
		//printk("Read from Slic:");
		for(byteCnt=0; byteCnt < rxsize; byteCnt++)
        {
			_rtl_spi_rawRead( spi_dev, &rxptr[byteCnt], 8);
			/*if(byteCnt == (rxsize-1))
				printk("%02x\n", rxptr[byteCnt]);
			else
				printk("%02x ", rxptr[byteCnt]);*/
		}
		
	}
	
	return 0;
#else
	struct spi_transfer t = {
		.tx_buf        = txptr,
		.len           = txsize + rxsize,
		.bits_per_word = (txsize + rxsize) * 8,
	};
	char rx_buf[8];
	int ret;

	t.rx_buf = rxptr ? rxptr : rx_buf;

	ret = ca77xx_spi_transfer_one(spi_dev->master, spi_dev, &t);

#if 0
	if ((txsize + rxsize) == 6) {
	printk("LEN(%d):0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n", txsize,
		*(txptr), *(txptr+1), *(txptr+2), *(txptr+3), *(txptr+4), *(txptr+5));
	if (rxsize)
	printk("LEN(%d):0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n", rxsize,
		*rxptr, *(rxptr+1), *(rxptr+2), *(rxptr+3), *(rxptr+4), *(rxptr+5));
	}

	if ((txsize + rxsize) == 4) {
	printk("LEN(%d):0x%02X:0x%02X:0x%02X:0x%02X\n", txsize,
		*(txptr), *(txptr+1), *(txptr+2), *(txptr+3));
	if (rxsize)
	printk("LEN(%d):0x%02X:0x%02X:0x%02X:0x%02X\n", rxsize,
		*rxptr, *(rxptr+1), *(rxptr+2), *(rxptr+3));
	}
#endif

	return ret;
#endif
}

#if 0
#define VDUMPREGNAME2 "slicdumpreg2"

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int voip_dumpreg2_read(struct seq_file *f, void *v)
{
	return 0;
}
#else
static int voip_dumpreg2_read(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{	
	return 0;
}
#endif
static rtl_spi_dev_t		spi_dev_test[1];

static int voip_dumpreg2_write(struct file *file, const char *buffer, 
                               unsigned long count, void *data)
{
#ifndef CONFIG_RTK_VOIP_LINUX_SPI
	extern int spi_debug_flag;
	char tmp[128];
	int i = 0;
	unsigned char sendbuf[1] = {0};
	unsigned char recvbuf[16] = {0};
	int t1 = 0;
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
		sscanf(tmp, "%d", &t1);
		printk("input = [%d]\n", t1);

		_init_rtl_spi_dev_type_hw(&spi_dev_test[ 0 ], HWCSNum);
		
		spi_debug_flag = 1;
		sendbuf[0] = 0x7e;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x24;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0xaa;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x55;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0xaa;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		sendbuf[0] = 0x55;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		

//		sleep(3);
		sendbuf[0] = 0xbe;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		//sendbuf[0] = 0x18;
		sendbuf[0] = 0x24;
		_rtl_spi_rawWrite(&spi_dev_test[ 0 ], &sendbuf[0], 8);
		
		printk("\n");
		for ( i = 0 ; i < 10 ; i ++ ){
		//printk("0x%02x\n", recvbuf[0]);
		_rtl_spi_rawRead(&spi_dev_test[ 0 ], &recvbuf[i], 8);
		printk("%d : 0x%02x\n", i, recvbuf[i]);
		}
		printk("\n");
//		for ( i = 0 ; i < 10 ; i ++ )printk("0x%02x ", recvbuf[i]);
		
		printk("\n");
		spi_debug_flag = 0;
	}
#endif
	return count;
}

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int spi_dumpreg2_open(struct inode *inode, struct file *file)
{
	return single_open(file, voip_dumpreg2_read, NULL);
}

static ssize_t spi_dumpreg2_write(struct file *file, const char __user * userbuf, size_t count, loff_t * off) {
        return voip_dumpreg2_write(file, userbuf, count, NULL);
}

struct file_operations proc_spi_dumpreg2_fops = {
	.owner	= THIS_MODULE,
	.open	= spi_dumpreg2_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
	.write  = spi_dumpreg2_write,
};
#endif

int __init voip_proc_dumpreg2_init(void)
{
	struct proc_dir_entry *voip_add_test_proc;
	
#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
		voip_add_test_proc = proc_create(VDUMPREGNAME2, 0644, NULL, &proc_spi_dumpreg2_fops);
		if (voip_add_test_proc == NULL) {
			remove_proc_entry(VDUMPREGNAME2, NULL);
			printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
					VDUMPREGNAME2);
			return -ENOMEM;
		}
#else
	voip_add_test_proc = create_proc_entry( VDUMPREGNAME2, 0644, NULL);
	if (voip_add_test_proc == NULL) {
		remove_proc_entry(VDUMPREGNAME2, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				VDUMPREGNAME2);
		return -ENOMEM;
	}
	voip_add_test_proc->read_proc  = (read_proc_t *)voip_dumpreg2_read;
	voip_add_test_proc->write_proc  = (write_proc_t *)voip_dumpreg2_write;
#endif
	
	return 0;
}

void __exit voip_proc_dumpreg2_exit( void )
{
	remove_proc_entry( VDUMPREGNAME2, NULL );
}

module_init( voip_proc_dumpreg2_init );
module_exit( voip_proc_dumpreg2_exit );
#endif

