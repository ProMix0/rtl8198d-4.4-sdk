#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif
#include "rtk_voip.h"
#include "voip_proc.h"
#include "voip_init.h"
#include "voip_params.h"
#include "con_register.h"
#include "con_defer_init.h"
#include "snd_pin_cs.h"
#include "snd_proslic_type.h"
#include "si3228x.h"

#include "snd_slic_cmd.h"
extern slic_cmd_t SLIC_CMDS[MAX_SLIC_CMD];
#define snd_proslic snd_proslic_3228x

#define CHAN_PER_DEVICE 		2	// one 3228x contains 2 FXS

#define TOTAL_NUM_OF_3228x_CH	( CONFIG_RTK_VOIP_SLIC_SI3228x_NR * 2 )
#define NUMBER_OF_PROSLIC_3228x	( TOTAL_NUM_OF_3228x_CH / CHAN_PER_DEVICE )
#if 1
#define NUMBER_OF_HWINTF_3228x	1
#define NUMBER_OF_CHAN_3228x		TOTAL_NUM_OF_3228x_CH
#else
// CS mode may use this 
#define NUMBER_OF_HWINTF_3228x	( TOTAL_NUM_OF_3228x_CH / CHAN_PER_DEVICE )
#endif

static ctrl_S gSpiGciObj_3228x[ NUMBER_OF_HWINTF_3228x ];
static ProslicContainer_t gProslicContainer_3228x[ TOTAL_NUM_OF_3228x_CH ];

static voip_snd_t snd_proslic_3228x[ TOTAL_NUM_OF_3228x_CH ];

#ifdef CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS_USE_SW
#if 1
static const uint32 * const pin_cs = 
		&snd_pin_cs[ CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS - 1 ];
#else
static const uint32 pin_cs[] = {
	PIN_CS1, 
};

CT_ASSERT( ( sizeof( pin_cs ) / sizeof( pin_cs[ 0 ] ) ) >= NUMBER_OF_HWINTF_3228x );
#endif
#endif

static const proslic_args_t proslic_args_3228x;

static int SLIC_init_si3228x(int pcm_mode, int initonly)
{
	int i, sidx;
	int j;
	int i_size, i_device, i_channel;
	//rtl_spi_dev_t *spi_devs[ NUMBER_OF_HWINTF_3228x ];
	
	printk ("\n<<<<<<<<<<< Si3228x Driver Version %s >>>>>>>>>>\n", ProSLIC_Version());
	
	//if( !initonly ) {
	//	// init spi first, because reset pin will affect all SLIC 
	//	printk( "Preparing spi channel for SLIC...\n" );
	//	
	//	for( i = 0; i < NUMBER_OF_HWINTF_3228x; i ++ ) {
	//		spi_devs[ i ] = &gSpiGciObj_3228x[ i ].spi_dev;
	//	}
	//
	//	init_spi_channels( NUMBER_OF_HWINTF_3228x, spi_devs, pin_cs, PIN_RESET1, PIN_CLK, PIN_DO, PIN_DI);
	//}
	
	for( i = 0, sidx = 0; i < NUMBER_OF_HWINTF_3228x; i ++ ) {
		
		printk( "--------------------------------------\n" );
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
		init_spi_dev( &gSpiGciObj_3228x[ i ].spi_dev, i);
#else
		printk( "SLIC HW intf %d starting at %d CS=%08X\n", i, sidx, 
#ifdef CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS_USE_SW
			pin_cs[ i ] 
#else 
			CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS + i
#endif
		);
		
#ifdef CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS_USE_SW
		init_spi_pins( &gSpiGciObj_3228x[ i ].spi_dev, pin_cs[ i ], PIN_CLK, PIN_DO, PIN_DI);
#else
		_init_rtl_spi_dev_type_hw( &gSpiGciObj_3228x[ i ].spi_dev, 
									CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS + i );
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI
                extern void slicInternalCS(int cs);
                slicInternalCS(CONFIG_RTK_VOIP_SLIC_SI3228x_PIN_CS);
#endif
#endif
#endif	/* CONFIG_RTK_VOIP_LINUX_SPI */
		
		i_size = NUMBER_OF_CHAN_3228x;
		i_device = NUMBER_OF_PROSLIC_3228x;
		i_channel = NUMBER_OF_CHAN_3228x;
		
		if( initonly )
			goto label_do_init_only;

		// create objs
		for( j = 0; j < i_size; j ++ ) {
			gProslicContainer_3228x[ sidx + j ].spiGciObj = &gSpiGciObj_3228x[ i ];			
		}
		
		proslic_alloc_objs( &gProslicContainer_3228x[ sidx ], 
			i_size, i_device, i_channel, SI3228X_TYPE );
				
		// init proslic 
label_do_init_only:
		
		proslic_init_user_objs( &gProslicContainer_3228x[ sidx ], i_size, 
						SI3228X_TYPE );
		
		proslic_init( &snd_proslic_3228x[ sidx ], &gProslicContainer_3228x[ sidx ], 
						i_size, 
						&proslic_args_3228x,
						pcm_mode );
		
		sidx += i_size;
	}
	
	return 0;
}

static void SLIC_reset_si3228x(voip_snd_t *this, int codec_law)
{
	// This function will cause system reset, if watch dog is enable!
	// Because calibration need mdelay(1900).
	unsigned long flags;
	save_flags(flags); cli();
	*(volatile unsigned long *)(0xB800311c) &=  0xFFFFFF;	// Disable watch dog
	*(volatile unsigned long *)(0xB800311c) |=  0xA5000000;
	restore_flags(flags);
	
	SLIC_init_si3228x(codec_law, 1 /* don't allocate */);
	
	save_flags(flags); cli();
	*(volatile unsigned long *)(0xB800311c) &=  0xFFFFFF;	// Enable watch dog
	*(volatile unsigned long *)(0xB800311c) |=  1 << 23;
	restore_flags(flags);
}

static void SLIC_show_ID_si3228x( voip_snd_t *this )
{
	unsigned char reg_val, reg_len;
	
	reg_len = sizeof(reg_val);
	
	//reg_val = R_reg_dev(&spiGciObj ->spi_dev, i, 0);
	this ->fxs_ops ->SLIC_read_reg( this, 0, &reg_len, &reg_val );
	
	if (((reg_val&0x38)>>3) == 0)
		printk("Si3228x\n");
	else if (((reg_val&0x38)>>3) == 1)
		printk("Si3227\n");
	else if ((((reg_val&0x38)>>3) & 0x2) == 0x2 )
		printk("Si32288/69\n");
	else
		PRINT_R("Unknow SLIC\n");
}

static Si3228x_Ring_param_dump(Si3228x_Ring_Cfg* p)
{
	printk("0x%8X\n", p->rtper);
	printk("0x%8X\n", p->freq);
	printk("0x%8X\n", p->amp);
	printk("0x%8X\n", p->phas);
	printk("0x%8X\n", p->offset);
	printk("0x%8X\n", p->slope_ring);
    	printk("0x%8X\n", p->iring_lim);
    	printk("0x%8X\n", p->rtacth);
	printk("0x%8X\n", p->rtdcth);
	printk("0x%8X\n", p->rtacdb);
	printk("0x%8X\n", p->rtdcdb);
	printk("0x%8X\n", p->vov_ring_bat);
	printk("0x%8X\n", p->vov_ring_gnd);
    	printk("0x%8X\n", p->vbatr_expect);
	printk("0x%2X\n", p->talo);
	printk("0x%2X\n", p->tahi);
	printk("0x%2X\n", p->tilo);
	printk("0x%2X\n", p->tihi);
	printk("0x%8X\n", p->adap_ring_min_i);
    	printk("0x%8X\n", p->counter_iring_val);
	printk("0x%8X\n", p->counter_vtr_val);
    	printk("0x%8X\n", p->ar_const28);
    	printk("0x%8X\n", p->ar_const32);
   	printk("0x%8X\n", p->ar_const38);
    	printk("0x%8X\n", p->ar_const46);
	printk("0x%8X\n", p->rrd_delay);
	printk("0x%8X\n", p->rrd_delay2);
    	printk("0x%8X\n", p->vbat_track_min_rng);
	printk("0x%2X\n", p->ringcon);
    	printk("0x%2X\n", p->userstat);
	printk("0x%8X\n", p->vcm_ring);
    	printk("0x%8X\n", p->vcm_ring_fixed);
    	printk("0x%8X\n", p->delta_vcm);
    	printk("0x%8X\n", p->dcdc_rngtype);
    	printk("0x%8X\n", p->vov_dcdc_slope);
    	printk("0x%8X\n", p->vov_dcdc_os);
    	printk("0x%8X\n", p->vov_ring_bat_max);
}

static void SLIC_set_param_si3228x(voip_snd_t *this, unsigned int slic_type, unsigned int param_type, unsigned char* pParam, unsigned int param_size)
{
	ProslicContainer_t * const container = ( ProslicContainer_t * )this ->priv;
	proslicChanType * const pSlic = container ->ProObj;

	extern int si3228x_ring_preset_size;
	extern int si3228x_dcfeed_preset_size;
	extern int si3228x_impedance_preset_size;
	extern Si3228x_Ring_Cfg Si3228x_Ring_Presets[];
	extern Si3228x_DCfeed_Cfg Si3228x_DCfeed_Presets[];
	extern Si3228x_Impedance_Cfg Si3228x_Impedance_Presets[];

	if ( slic_type == PROSLIC_TYPE_SI3228X )
	{
		switch (param_type)
		{
			case PROSLIC_PARAM_TYPE_RING:
				if ( param_size != (sizeof(Si3228x_Ring_Cfg)) )
				{
					PRINT_R("%s, line%d, size mis-match, %d != %d.\n", __FUNCTION__, __LINE__, param_size, sizeof(Si3228x_Ring_Cfg));
					return;
				}
				//Si3228x_Ring_param_dump((Si3228x_Ring_Cfg*)pParam);
				memcpy(&Si3228x_Ring_Presets[si3228x_ring_preset_size-1], pParam, param_size);
				ProSLIC_RingSetup(pSlic, si3228x_ring_preset_size-1);
				break;
			case PROSLIC_PARAM_TYPE_DCFEED:
				if ( param_size != (sizeof(Si3228x_DCfeed_Cfg)) )
				{
					PRINT_R("%s, line%d, size mis-match, %d != %d.\n", __FUNCTION__, __LINE__, param_size, sizeof(Si3228x_DCfeed_Cfg));
					return;
				}
				memcpy(&Si3228x_DCfeed_Presets[si3228x_dcfeed_preset_size-1], pParam, param_size);
				ProSLIC_DCFeedSetup(pSlic, si3228x_dcfeed_preset_size-1);
				break;
			case PROSLIC_PARAM_TYPE_IMPEDANCE:
				if ( param_size != (sizeof(Si3228x_Impedance_Cfg)) )
				{
					PRINT_R("%s, line%d, size mis-match, %d != %d.\n", __FUNCTION__, __LINE__, param_size, sizeof(Si3228x_Impedance_Cfg));
					return;
				}
				memcpy(&Si3228x_Impedance_Presets[si3228x_impedance_preset_size-1], pParam, param_size);
				ProSLIC_ZsynthSetup(pSlic, si3228x_impedance_preset_size-1);
				break;
			default:
				PRINT_R("%s doesn't support param type: %d\n", __FUNCTION__, param_type);
				break;
		}
	}
	else
		PRINT_R("%s doesn't support SLIC type: %d\n", __FUNCTION__, slic_type);

	PRINT_MSG("SLIC_set_param_proslic, ch = %d, slic_type = %d, param_type = %d, param_size = %d\n", this ->sch, slic_type, param_type, param_size);
}

// --------------------------------------------------------
// channel mapping architecture 
// --------------------------------------------------------

static snd_ops_fxs_t snd_proslic_3228x_ops;

static const proslic_args_t proslic_args_3228x = {
	.ring_setup_preset = 0,	// 20Hz, 48V
};

int __init voip_snd_si3228x_init( void )
{
	extern int law;	// FIXME: chmap 
	extern const snd_ops_fxs_t snd_proslic_fxs_ops;
	int i, sch;
	int TS_base;
#ifdef CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT
	static defer_init_t si3228x_defer;
#endif
	
	// 3228x override proslic base ops 
	snd_proslic_3228x_ops = snd_proslic_fxs_ops;
	
	snd_proslic_3228x_ops.SLIC_reset = SLIC_reset_si3228x;
	snd_proslic_3228x_ops.SLIC_show_ID = SLIC_show_ID_si3228x;
	snd_proslic_3228x_ops.SLIC_set_param = SLIC_set_param_si3228x;
	
	// get TS base 
	TS_base = get_snd_free_timeslot();	
	if( TS_base < 0 )
		TS_base = 0;
	
	// common port definition 
	for( i = 0, sch = 0; i < TOTAL_NUM_OF_3228x_CH; i ++, sch ++ ) {

		snd_proslic_3228x[ i ].sch = sch;
		snd_proslic_3228x[ i ].name = "si3228x";
		snd_proslic_3228x[ i ].snd_type = SND_TYPE_FXS;
		snd_proslic_3228x[ i ].bus_type_sup = BUS_TYPE_PCM;
		snd_proslic_3228x[ i ].TS1 = TS_base + i * 2;
#ifdef CONFIG_RTK_VOIP_SLIC_SI3228x_WIDEBAND
		snd_proslic_3228x[ i ].TS2 = ( TS_base + i * 2 >= 16 ? 0 : TS_base + ( i + 8 ) * 2 );
		snd_proslic_3228x[ i ].band_mode_sup = BAND_MODE_8K | BAND_MODE_16K;
#else
		snd_proslic_3228x[ i ].TS2 = 0;
		snd_proslic_3228x[ i ].band_mode_sup = BAND_MODE_8K;
#endif
		snd_proslic_3228x[ i ].snd_ops = ( const snd_ops_t * )&snd_proslic_3228x_ops;
		snd_proslic_3228x[ i ].priv = &gProslicContainer_3228x[ i ];
		snd_proslic_3228x[ sch ].polarity = 0;
	}
	
	register_voip_snd( &snd_proslic_3228x[ 0 ], TOTAL_NUM_OF_3228x_CH );
	
	// SLIC init use ops 
#ifdef CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT
	si3228x_defer.fn_defer_func = ( fn_defer_func_t )SLIC_init_si3228x;
	si3228x_defer.p0 = law;
	si3228x_defer.p1 = 0;
	
	add_defer_initialization( &si3228x_defer );
#else
	SLIC_init_si3228x( law, 0 /* allocate */ );
#endif
	
	return 0;
}

voip_initcall_snd( voip_snd_si3228x_init );

extern void SLIC_write_proslic_reg(voip_snd_t *this, unsigned int num, unsigned char *len, unsigned char *val);

#ifdef SLICDUMPREG
#define VDUMPREGNAME1 "slicdumpreg"

extern void SLIC_dump_proslic_ram(voip_snd_t *this);
extern void SLIC_dump_proslic_reg(voip_snd_t *this);
#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int voip_dumpreg_read(struct seq_file *f, void *v)
{
	voip_slic_cmd_help();
	return 0;
}
#else
static int voip_dumpreg_read(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{
    voip_slic_cmd_help();
	return 1;
}
#endif
static int voip_dumpreg_write(struct file *file, const char *buffer, 
                               unsigned long count, void *data)
{

	char tmp[128];
	unsigned int t1 = 0;
    int p0 = 0;
    int slic_nr = NUMBER_OF_HWINTF_3228x;
	
	if (count < 2)
		return -EFAULT;

//static void WriteReg_dev (rtl_spi_dev_t* pDev, uInt8 channel, uInt8 regAddr, uInt8 data)

	
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
        sscanf(tmp, "%d", &p0);
     
		if(SLIC_CMDS[p0].enable != true){
			printk("[%s]:not support this cmd\n", __FUNCTION__);
			return -EFAULT;
		}

		if(SLIC_CMDS[p0].cmd_argc > count){
			printk("count = %u, SLIC_CMDS[%d].cmd_argc = %d\n", count, p0, SLIC_CMDS[p0].cmd_argc);
			printk("%s", SLIC_CMDS[p0].illustration);
			return -EFAULT;
		}

		if(SLIC_CMDS[p0].obj_type == SND_OBJ){
			if(p0 == DUMP_REG_VAL){
				SLIC_CMDS[p0].callback(tmp, &snd_proslic[0], &slic_nr);
			}
			else
				SLIC_CMDS[p0].callback(tmp, &snd_proslic[0], NULL);	
			
		}

	}

	return count;
}

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
static int si3228x_dumpreg_open(struct inode *inode, struct file *file)
{
	return single_open(file, voip_dumpreg_read, NULL);
}

static ssize_t si3228x_dumpreg_write(struct file *file, const char __user * userbuf, size_t count, loff_t * off) {
        return voip_dumpreg_write(file, userbuf, count, NULL);
}

struct file_operations proc_si3228x_fops = {
	.owner	= THIS_MODULE,
	.open	= si3228x_dumpreg_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
	.write  = si3228x_dumpreg_write,
};
#endif

int __init voip_proc_add_test5_init(void)
{
	struct proc_dir_entry *voip_add_test_proc;

#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
	voip_add_test_proc = proc_create(VDUMPREGNAME1, 0644, NULL, &proc_si3228x_fops);
	if (voip_add_test_proc == NULL) {
		remove_proc_entry(VDUMPREGNAME1, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				VDUMPREGNAME1);
		return -ENOMEM;
	}
#else	
	voip_add_test_proc = create_proc_entry( VDUMPREGNAME1, 0644, NULL);
	if (voip_add_test_proc == NULL) {
		remove_proc_entry(VDUMPREGNAME1, NULL);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				VDUMPREGNAME1);
		return -ENOMEM;
	}
	voip_add_test_proc->read_proc  = (read_proc_t *)voip_dumpreg_read;
	voip_add_test_proc->write_proc  = (write_proc_t *)voip_dumpreg_write;
#endif
	
	voip_slic_cmd_init(NUMBER_OF_HWINTF_3228x);
	
	return 0;
}

void __exit voip_proc_add_test5_exit( void )
{
	remove_proc_entry( VDUMPREGNAME1, NULL );
}

module_init( voip_proc_add_test5_init );
module_exit( voip_proc_add_test5_exit );
#endif

void disableSLIC(void){
	int reg, value, *len = 1;
	reg = 1;
	value = 0x0;
//	printk("reg = [%d], value = 0x[%x]\n", t1, t3);
	SLIC_write_proslic_reg(&snd_proslic_3228x[0], reg, len, &value);			
}
