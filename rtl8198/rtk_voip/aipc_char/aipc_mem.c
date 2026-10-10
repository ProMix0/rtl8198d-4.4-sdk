#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#ifdef CONFIG_VOIP_LINUX_PLATFORM_DRIVER
#include <linux/reset.h>
#endif

#ifdef CONFIG_RTL9607C //PON check chip version rtk_switch_vertion_get
#include "rtk/switch.h"
extern unsigned int aipcBoardChipId;
extern unsigned int aipcBoardChipRev;
extern unsigned int aipcBoardChipSubType;
#endif

#include "./include/aipc_mem.h"
#include "./include/aipc_reg.h"
#include "./include/dram_share.h"
#include "./include/aipc_ioctl.h"
#include "./include/aipc_debug.h"

/*
*	IPC index use SRAM
*/           
#if    defined(CONFIG_RTL8686_IPC_IDX_USE_DRAM) &&  defined(CONFIG_RTL8686_IPC_IDX_USE_SRAM)
#error "CONFIG_RTL8686_IPC_IDX_USE_DRAM and CONFIG_RTL8686_IPC_IDX_USE_SRAM are conflict"
#endif
#if   !defined(CONFIG_RTL8686_IPC_IDX_USE_DRAM) && !defined(CONFIG_RTL8686_IPC_IDX_USE_SRAM)
#error "Please choose CONFIG_RTL8686_IPC_IDX_USE_DRAM or CONFIG_RTL8686_IPC_IDX_USE_SRAM"
#endif

extern unsigned int SOC_ID, SOC_BOND_ID;
static volatile unsigned int *dsp_boot_ins;

#ifndef CONFIG_RTL8686_IPC_DUAL_LINUX
static unsigned int jump_ins[] = {
				/* eCos reset vector */
#if defined(CONFIG_ARCH_CORTINA) && \
    !defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                0x3c088200,       /* CHECKME */
#else
                0x3c088000        /*lui     t0,0x8000*/,
#endif
                0x350804c4        /*ori     t0,t0,0x04c4*/,
                0x01000008        /*jr      t0*/,
                0x00000000        /*nop       */
              };
#else
static unsigned int jump_ins[] = {
				/* Dual Linux reset vector */
                0x3c08a000        /*lui     t0,0xa000*/,
//              0x3c088000        /*lui     t0,0x8000*/,
                0x35080000        /*ori     t0,t0,0x0000*/,
                0x01000008        /*jr      t0*/,
                0x00000000        /*nop       */
              };
#endif

unsigned int init_C0DOR0   = 0;
unsigned int init_C0DMAR0  = 0;
unsigned int init_C0DOR1   = 0;
unsigned int init_C0DMAR1  = 0;
unsigned int init_C0DOR2   = 0;
unsigned int init_C0DMAR2  = 0;
unsigned int init_C1DOR0   = 0;
unsigned int init_C1DMAR0  = 0;
unsigned int init_C1DOR1   = 0;
unsigned int init_C1DMAR1  = 0;
unsigned int init_C1DOR2   = 0;
unsigned int init_C1DMAR2  = 0;

void aipc_record_init_mem_regs(void)
{
#if !defined(CONFIG_VOIP_LINUX_PLATFORM_DRIVER)
	init_C0DOR0  = REG32( C0DOR0  );
	init_C0DMAR0 = REG32( C0DMAR0 );
	init_C0DOR1  = REG32( C0DOR1  );
	init_C0DMAR1 = REG32( C0DMAR1 );
	init_C0DOR2  = REG32( C0DOR2  );
	init_C0DMAR2 = REG32( C0DMAR2 );

	init_C1DOR0  = REG32( C1DOR0  );
	init_C1DMAR0 = REG32( C1DMAR0 );
	init_C1DOR1  = REG32( C1DOR1  );
	init_C1DMAR1 = REG32( C1DMAR1 );
	init_C1DOR2  = REG32( C1DOR2  );
	init_C1DMAR2 = REG32( C1DMAR2 );
#endif
}

void aipc_reset_init_mem_regs(void)
{
#if !defined(CONFIG_VOIP_LINUX_PLATFORM_DRIVER)
	REG32( C0DOR0  ) = init_C0DOR0  ;
	REG32( C0DMAR0 ) = init_C0DMAR0 ;
	REG32( C0DOR1  ) = init_C0DOR1  ;
	REG32( C0DMAR1 ) = init_C0DMAR1 ;
	REG32( C0DOR2  ) = init_C0DOR2  ;
	REG32( C0DMAR2 ) = init_C0DMAR2 ;

#if 0
	REG32( C1DOR0  ) = init_C1DOR0  ;
	REG32( C1DMAR0 ) = init_C1DMAR0 ;
	REG32( C1DOR1  ) = init_C1DOR1  ;
	REG32( C1DMAR1 ) = init_C1DMAR1 ;
	REG32( C1DOR2  ) = init_C1DOR2  ;
	REG32( C1DMAR2 ) = init_C1DMAR2 ;
#endif
#endif
}

int
aipc_cpu_sram_map(
	u8_t  seg_no , 
	u32_t map_addr , 
	u32_t size , 
	u8_t  enable , 
	u32_t base_addr , 
	u8_t  lx_match)
{
	void *seg_addr=NULL;
	volatile u32_t tmp=0;
	if ((void*)map_addr==NULL || seg_no>=SRAM_SEG_MAX){
		printk( "wrong sram map setting\n" );
		return NOK;	
		}

	if (seg_no==SRAM_SEG_IDX_0){
		seg_addr = (void*)R_C0SRAMSAR0;
		}
	else if (seg_no==SRAM_SEG_IDX_1){
		seg_addr = (void*)R_C0SRAMSAR1;
		}
	else if (seg_no==SRAM_SEG_IDX_2){
		seg_addr = (void*)R_C0SRAMSAR2;
		}
	else if (seg_no==SRAM_SEG_IDX_3){
		seg_addr = (void*)R_C0SRAMSAR3;
		}
	else{
		return NOK;	
		}

	tmp	= Virtual2Physical(map_addr);
	
	if (enable==SRAM_SEG_ENABLE){
		tmp	|= SRAM_SEG_ENABLE_BIT;		//enable
		}
	else{
		tmp	&= ~SRAM_SEG_ENABLE_BIT;
		}
	
	if (lx_match==SRAM_LX_MATCH_ENABLE){
		tmp	|= SRAM_LX_MATCH_BIT;
		}
	else{
		tmp	&= ~SRAM_LX_MATCH_BIT;
		}


	REG32(seg_addr)		=	tmp;	
	REG32(seg_addr+4)	=	size;
	REG32(seg_addr+8)	=	base_addr;	//base

//	SDEBUG("SRAM map_addr v2p = 0x%08x\n" , Virtual2Physical(map_addr));
//	SDEBUG("SRAM SEG  = 0x%08x\n"         , REG32(seg_addr));
//	SDEBUG("SRAM Size = 0x%08x\n"         , REG32(seg_addr+4));
//	SDEBUG("SRAM Base = 0x%08x\n"         , REG32(seg_addr+8));

	return OK;
}

int
aipc_cpu_dram_unmap(
	u8_t  seg_no , 
	u32_t unmap_addr , 
	u32_t size , 
	u8_t  enable , 
	u8_t  lx_match)
{
	void *seg_addr=NULL;
	volatile u32_t tmp=0;	
	if ((void*)unmap_addr==NULL || seg_no>=DRAM_SEG_MAX){
		printk( "wrong dram unmap setting\n" );
		return NOK;	
		}

	if (seg_no==DRAM_SEG_IDX_0){
		seg_addr = (void*)R_C0UMSAR0;
		}
	else if (seg_no==DRAM_SEG_IDX_1){
		seg_addr = (void*)R_C0UMSAR1;
		}
	else if (seg_no==DRAM_SEG_IDX_2){
		seg_addr = (void*)R_C0UMSAR2;
		}
	else if (seg_no==DRAM_SEG_IDX_3){
		seg_addr = (void*)R_C0UMSAR3;
		}
	else{
		return NOK;	
		}
	
	tmp	= Virtual2Physical(unmap_addr);

	if (enable==DRAM_SEG_ENABLE){
		tmp	|= DRAM_SEG_ENABLE_BIT;		//enable
		}
	else{
		tmp	&= ~DRAM_SEG_ENABLE_BIT;
		}

	if (lx_match==DRAM_LX_MATCH_ENABLE){
		tmp	|= DRAM_LX_MATCH_BIT;
		}
	else{
		tmp	&= ~DRAM_LX_MATCH_BIT;
		}

	REG32(seg_addr)		=	tmp;
	REG32(seg_addr+4)	=	size;

//	SDEBUG("DRAM unmap_addr v2p = 0x%08x\n" , Virtual2Physical(unmap_addr));	
//	SDEBUG("DRAM SEG  = 0x%08x\n"           , REG32(seg_addr));
//	SDEBUG("DRAM Size = 0x%08x\n"           , REG32(seg_addr+4));

	return OK;
}

#if 0
int
aipc_dsp_sram_map(
	u8_t  seg_no , 
	u32_t map_addr , 
	u32_t size , 
	u8_t  enable , 
	u32_t base_addr , 
	u8_t  lx_match)
{
	void *seg_addr=NULL;
	volatile u32_t tmp=0;	
	if ((void*)map_addr==NULL || seg_no>=SRAM_SEG_MAX){
		printk( "wrong sram map setting\n" );
		return NOK;	
		}

	if (seg_no==SRAM_SEG_IDX_0){
		seg_addr = (void*)R_C1SRAMSAR0;
		}
	else if (seg_no==SRAM_SEG_IDX_1){
		seg_addr = (void*)R_C1SRAMSAR1;
		}
	else if (seg_no==SRAM_SEG_IDX_2){
		seg_addr = (void*)R_C1SRAMSAR2;
		}
	else if (seg_no==SRAM_SEG_IDX_3){
		seg_addr = (void*)R_C1SRAMSAR3;
		}
	else{
		return NOK;	
		}

	tmp	= Virtual2Physical(map_addr);
	if (enable==SRAM_SEG_ENABLE){
		tmp	|= SRAM_SEG_ENABLE_BIT;		//enable
		}
	else{
		tmp	&= ~SRAM_SEG_ENABLE_BIT;
		}

	if (lx_match==SRAM_LX_MATCH_ENABLE){
		tmp	|= SRAM_LX_MATCH_BIT;
		}
	else{
		tmp	&= ~SRAM_LX_MATCH_BIT;
		}
	
	REG32(seg_addr)		=	tmp;
	REG32(seg_addr+4)	=	size;
	REG32(seg_addr+8)	=	base_addr;	//base
		
	return OK;
}

int
aipc_dsp_dram_unmap(
	u8_t  seg_no , 
	u32_t unmap_addr , 
	u32_t size , 
	u8_t  enable , 
	u8_t  lx_match)
{
	void *seg_addr=NULL;
	volatile u32_t tmp=0;		
	if ((void*)unmap_addr==NULL || seg_no>=DRAM_SEG_MAX){
		printk( "wrong dram unmap setting\n" );
		return NOK;	
		}
 
	if (seg_no==DRAM_SEG_IDX_0){
		seg_addr = (void*)R_C1UMSAR0;
		}
	else if (seg_no==DRAM_SEG_IDX_1){
		seg_addr = (void*)R_C1UMSAR1;
		}
	else if (seg_no==DRAM_SEG_IDX_2){
		seg_addr = (void*)R_C1UMSAR2;
		}
	else if (seg_no==DRAM_SEG_IDX_3){
		seg_addr = (void*)R_C1UMSAR3;
		}
	else{
		return NOK;
		}
	
	tmp	= Virtual2Physical(unmap_addr);
	if (enable==DRAM_SEG_ENABLE){
		tmp	|= DRAM_SEG_ENABLE_BIT;		//enable
		}
	else{
		tmp	&= ~DRAM_SEG_ENABLE_BIT;
		}
	
	if (lx_match==DRAM_LX_MATCH_ENABLE){
		tmp	|= DRAM_LX_MATCH_BIT;
		}
	else{
		tmp	&= ~DRAM_LX_MATCH_BIT;
		}

	REG32(seg_addr)		= 	tmp;
	REG32(seg_addr+4)	=	size;

	return OK;
}
#endif

int
aipc_cpu_rom_map( 
	u32_t map_addr , 
	u32_t clk , 
	u32_t wait ,
	u32_t size , 
	u8_t  enable)
{
	volatile u32_t tmp=0;
	
	if ((void*)map_addr==NULL){
		printk( "wrong rom map setting\n" );
		return NOK;
	}

	//Setup segment address	register
#if 0
	tmp	= Virtual2Physical(map_addr) & ROM_SEG_MASK;
#else
	tmp	= Virtual2Physical(map_addr);
#endif
	
	if (enable==ROM_SEG_ENABLE){
		tmp	|= ROM_SEG_ENABLE_BIT;		//enable
		}
	else{
		tmp	&= ~ROM_SEG_ENABLE_BIT;
		}
	REG32(R_ROMSAR) = tmp;
	
	
	//Setup segment size register
	tmp = REG32(R_ROMSSR);
	
	if (clk==ROM_CLOCK_DIV_2){
		tmp &= ~ROM_CLOCK_DIV_BIT;
	}
	else{	//ROM_CLOCK_DIV_4
		tmp |= ROM_CLOCK_DIV_BIT;
	}
	
	if (wait>ROM_WAIT_TIME_NONE && wait<=ROM_WAIT_TIME_7T){
		tmp |= (wait<<7);
	}
	
#ifdef CONFIG_RTL8686_FPGA		//in FPGA phase
	if (clk==ROM_CLOCK_DIV_NONE){
		tmp &= ~ROM_CLOCK_DIV_BIT;
	}
	if (wait==ROM_WAIT_TIME_NONE){
		tmp &= (0<<7);
		tmp &= (0<<8);
		tmp &= (0<<9);
	}
#endif
		
	if (size>=ROM_SIZE_32K && size<=ROM_SIZE_1M){
		tmp |= size;
	}
	
	REG32(R_ROMSSR) = tmp;

//	SDEBUG("ROM Address Reg=%x\n" , REG32(R_ROMSAR));
//	SDEBUG("ROM Size Reg=%x\n"    , REG32(R_ROMSSR));
	
	return OK;	
}
/********************************************************************************
* CONFIG_RTL8686_IPC_IDX_USE_DRAM should only be used, when Memory size <=128MB
* So this osolete CONFIG_RTL8686_IPC_IDX_USE_DRAM.
* If you want use this option, please define DRAM_SMALL_128MB by yourself.
*********************************************************************************/

#if defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281)
static int prepare_BIR(void) 
{
	SDEBUG("\n");
	
	//Disable 5281 SRAM
	REG32(0xb8004040) = 0;
	REG32(0xb8004044) = 0;
	
	//Setup BIR
	//REG32(0xb8004100) = 0x3c088000;
	REG32(0xb8004100) = 0x3c08a000;        /*lui     t0,0xa000*/
	REG32(0xb8004104) = 0x350804c4;        /*ori     t0,t0,0x04c4*/
	REG32(0xb8004108) = 0x01000008;        /*jr      t0*/
	REG32(0xb800410c) = 0;                 /*nop       */
	REG32(0xb8004110) = 0;
	REG32(0xb8004114) = 0;
	REG32(0xb8004118) = 0;
	REG32(0xb800411c) = 0;	

	//Enable BIR
	REG32(0xb800413c) = 0;

    return 0;
}
#endif

int
aipc_dsp_entry(void)
{
	int i=0;

#ifdef CONFIG_ARCH_CORTINA
	extern void __iomem *ca_voip_dsp_boot_ins_base;

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
	void __iomem *addr;
	extern struct reset_control *ca_voip_pe_atu_reset;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
	if (!reset_control_status(ca_voip_pe_atu_reset))
#endif
	reset_control_assert(ca_voip_pe_atu_reset);
	mdelay(1);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
	if (reset_control_status(ca_voip_pe_atu_reset))
#endif
	reset_control_deassert(ca_voip_pe_atu_reset);

	addr = ioremap(0x1c101200, 0x10);

	/*** write entry 7 : mapping 0x1fc0_0000 to DRAM:0x4000_0000 ***/
	writel(0x1fc0, addr + 0x4);
	writel(0xfffe, addr + 0x8);
	writel(0x4000, addr + 0xc);
	writel(0x8700009f, addr);

	mdelay(1);

	/*** write entry 6 : mapping 0x0000_0000 to DRAM:0x0200_0000 ***/
	writel(0x0000, addr + 0x4);
	writel(0xf000, addr + 0x8);
	writel(0x0200, addr + 0xc);
	writel(0x860000df, addr);
#endif

	dsp_boot_ins = ca_voip_dsp_boot_ins_base;

	for(i=0 ; i<sizeof(jump_ins) ; i++){
		*dsp_boot_ins = jump_ins[i];
		dsp_boot_ins++;
	}

#else /* CONFIG_ARCH_CORTINA */

#if defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281)
	
#ifdef CONFIG_RTL9607C 
	switch(aipcBoardChipId)
	{
		case APOLLO_CHIP_ID  : 
			break;
		case APOLLOMP_CHIP_ID: 
			break;
		case RTL9601B_CHIP_ID: 
			break;
		case RTL9602C_CHIP_ID: 
			break;
		case RTL9607C_CHIP_ID: 
			prepare_BIR();
			break;
		default:
			break;
	}
#else	// !defined(CONFIG_RTL9607C)
	if( SOC_ID == 0x0822 || SOC_ID == 0x6518 /*|| SOC_ID == 0x0787*/ ){
		prepare_BIR();
	}	
	else if( SOC_ID == 0x0797 ){
		SDEBUG("Do nothing. SOC_ID 0x%04x\n" , SOC_ID );
	}
	else{
		SDEBUG("Please check SOC_ID 0x%04x\n" , SOC_ID );
	}
#endif
	
#else
	
#ifdef GPON_RESV
#if defined(CONFIG_RTL8686_IPC_IDX_USE_DRAM) && defined(DRAM_SMALL_128MB)

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_1, 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_8KB,
		SRAM_SEG_ENABLE , 
		GPON_SRAM_BASE , 		//Add GPON SRAM BASE
		~SRAM_LX_MATCH_ENABLE);

	aipc_cpu_dram_unmap(
		DRAM_SEG_IDX_1,
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		DRAM_SIZE_8KB,
		DRAM_SEG_ENABLE , 
		~DRAM_LX_MATCH_ENABLE);

#else  /* Use SRAM Only */

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_1, 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_8KB,
		SRAM_SEG_ENABLE , 
		GPON_SRAM_BASE , 		//Add GPON SRAM BASE
		~SRAM_LX_MATCH_ENABLE);

#endif
#else
	aipc_cpu_sram_map(
		SRAM_SEG_IDX_0 , 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_128KB , 
		SRAM_SEG_ENABLE , 
		0 , 
		~SRAM_LX_MATCH_ENABLE);

	aipc_cpu_dram_unmap(
		DRAM_SEG_IDX_0 , 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		DRAM_SIZE_128KB , 
		DRAM_SEG_ENABLE , 
		~DRAM_LX_MATCH_ENABLE);
#endif

	dsp_boot_ins = ((volatile unsigned int *)DSP_BOOT_INS_SRAM_MAPPING_ADDR);
	
	for(i=0 ; i<sizeof(jump_ins) ; i++){
		*dsp_boot_ins = jump_ins[i];
		dsp_boot_ins++;
	}
	
#endif

#endif /* CONFIG_ARCH_CORTINA */

	return OK;
}

int
aipc_soc_sram_backup(void)
{
#ifdef CONFIG_LUNA_USE_SRAM
	extern void luna_sram_restore(void);
	luna_sram_restore();
	SDEBUG("Call SOC SRAM backup API\n");
#endif
	return OK;
}

int
aipc_dsp_disable_mem(void)
{
	SDEBUG("\n");
#if !(defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281))
#ifdef GPON_RESV
#if defined(CONFIG_RTL8686_IPC_IDX_USE_DRAM) && defined(DRAM_SMALL_128MB)

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_1, 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_8KB,
		0 , 
		GPON_SRAM_BASE , 		//Add GPON SRAM BASE
		~SRAM_LX_MATCH_ENABLE);

	aipc_cpu_dram_unmap(
		DRAM_SEG_IDX_1,
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		DRAM_SIZE_8KB,
		0 , 
		~DRAM_LX_MATCH_ENABLE);

#else  /* Use SRAM Only */

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_1, 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_8KB,
		0 , 
		GPON_SRAM_BASE , 		//Add GPON SRAM BASE
		~SRAM_LX_MATCH_ENABLE);

#endif
#else
	aipc_cpu_sram_map(
		SRAM_SEG_IDX_0 , 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		SRAM_SIZE_128KB , 
		0 , 
		0 , 
		~SRAM_LX_MATCH_ENABLE);

	aipc_cpu_dram_unmap(
		DRAM_SEG_IDX_0 , 
		DSP_BOOT_INS_SRAM_MAPPING_ADDR , 
		DRAM_SIZE_128KB , 
		0 , 
		~DRAM_LX_MATCH_ENABLE);
#endif
#endif

	return OK;
}

int
aipc_rom_set(unsigned int rom_addr)
{
#if defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281)
	// Do nothing
#elif defined(CONFIG_RTL8686_ASIC)

	aipc_cpu_rom_map(
		rom_addr , //DSP_ROMCODE_ADDR
		ROM_CLOCK_DIV_NONE , 
		ROM_WAIT_TIME_NONE , 
		ROM_SIZE_256K , 
		ROM_SEG_ENABLE);

#else		//in FPGA phase

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_0 ,
		rom_addr , 			//DSP_ROMCODE_ADDR
		SRAM_SIZE_128KB , 
		SRAM_SEG_ENABLE , 
		0 , 
		~SRAM_LX_MATCH_ENABLE);

	aipc_cpu_sram_map(
		SRAM_SEG_IDX_1 ,
		rom_addr+ROM_BASE_128K , 	//DSP_ROMCODE_ADDR+0x20000
		SRAM_SIZE_128KB , 
		SRAM_SEG_ENABLE , 
		ROM_BASE_128K , 			//base=0x20000
		~SRAM_LX_MATCH_ENABLE);

#endif
	return OK;
}

int
aipc_soc_sram_set(unsigned long sram_addr)
{
#ifdef CONFIG_LUNA_USE_SRAM
	extern int luna_sram_mapping(unsigned long maddr);
	if(luna_sram_mapping(sram_addr) == -1)
		return NOK;
	SDEBUG("Call SOC SRAM setting API sram_addr=0x%08lx\n" , sram_addr);
#endif

	return OK;
}

#ifndef CONFIG_VOIP_LINUX_PLATFORM_DRIVER
#ifdef CONFIG_LUNA_MEMORY_AUTO_DETECTION
#include <bsp_automem.h>

int
aipc_zone_set(zone_plan_t zp)
{
   unsigned long irqflags;

   local_irq_save(irqflags);
#if defined(DEBUG)
    SDEBUG("bsp_mem_map= 0x%08lx , 0x%08lx\n", bsp_mem_map.BSP_IPC_MEM_BASE, bsp_mem_map.BSP_DSP_MEM_BASE);
#endif
	if (zp==zp_dsp_init){
                /**** CPU Zone 0  *****/
		//Can not adjust the CPU_MEM_BASE, it will cause the kernel crash.
                REG32(C0DOR0)  = 0;
                if( bsp_mem_map.BSP_IPC_MEM_BASE >= 0x10000000){
		  REG32(C0DMAR0) = (0x10000000 - 1);
		}else{
		  REG32(C0DMAR0) = bsp_mem_map.BSP_IPC_MEM_BASE - 1;
		}
                /***** CPU Zone 1 *****/
  		REG32(C0DOR1)  = (bsp_mem_map.BSP_DSP_MEM_BASE)+ PHYSICAL_SPACE_SIZE - ZONE1_BASE;
		REG32(C0DMAR1) = CONFIG_RTL8686_DSP_MEM_SIZE-1;
                iob(); smp_mb();
 
		/*****  DSP Zone 0 ****/
		// REG32(C1DOR0)  = (mem_size - CONFIG_RTL8686_DSP_MEM_SIZE);
		REG32(C1DOR0)  = (bsp_mem_map.BSP_DSP_MEM_BASE);
		REG32(C1DMAR0) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

		/**** DSP Zone 1 ******/
 		REG32(C1DOR1)  = (bsp_mem_map.BSP_IPC_MEM_BASE)+ PHYSICAL_SPACE_SIZE - ZONE1_BASE;	
		REG32(C1DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;
                /****************/	
	}
	else {
		//CPU Zone 1
        	REG32(C0DOR1)  = (bsp_mem_map.BSP_IPC_MEM_BASE)+ PHYSICAL_SPACE_SIZE - ZONE1_BASE;	
		REG32(C0DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;
	}
   local_irq_restore(irqflags);

#if defined(DEBUG)
	SDEBUG("\n");
	SDEBUG("CPU Zone 1: 0x%08lx,0x%08x\n", (bsp_mem_map.BSP_DSP_MEM_BASE)+ PHYSICAL_SPACE_SIZE - ZONE1_BASE, CONFIG_RTL8686_DSP_MEM_SIZE-1);
	SDEBUG("DSP Zone 1: 0x%08lx,0x%08x\n", (bsp_mem_map.BSP_IPC_MEM_BASE)+ PHYSICAL_SPACE_SIZE - ZONE1_BASE, CONFIG_RTL8686_IPC_MEM_SIZE-1);
	SDEBUG("CPU zone config:\n");
	SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
	SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
	SDEBUG("DSP zone config:\n");
	SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
	SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
	SDEBUG("  C1DOR2=%08x C1DMAR2=%08x\n" , REG32(C1DOR2) , REG32(C1DMAR2));
#endif
	return OK;
}

#else
int
aipc_zone_set(zone_plan_t zp)
{
	
#ifdef CONFIG_ARCH_CPU_RLX5281
	if (zp==zp_dsp_init){
		//CPU Zone 0
		//Can not adjust the CPU_MEM_BASE, it will cause the kernel crash.
                //REG32(C0DOR0)  = 0;
		REG32(C0DMAR0) = CONFIG_RTL8686_CPU_MEM_SIZE-1;
		
		//CPU Zone 1
		REG32(C0DOR1)  = CONFIG_RTL8686_DSP_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
		REG32(C0DMAR1) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

		//DSP Zone 0
		REG32(C1DOR0)  = CONFIG_RTL8686_DSP_MEM_BASE;
		REG32(C1DMAR0) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

		//DSP Zone 1
		REG32(C1DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
		REG32(C1DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;
	}
	else {
		//CPU Zone 1
		REG32(C0DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
		REG32(C0DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;
	}
#elif defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281)
	
#ifdef CONFIG_RTL9607C 
	switch(aipcBoardChipId)
	{
		case APOLLO_CHIP_ID  : 
			break;
		case APOLLOMP_CHIP_ID: 
			break;
		case RTL9601B_CHIP_ID: 
			break;
		case RTL9602C_CHIP_ID: 
			break;
		case RTL9607C_CHIP_ID: 
			if (zp==zp_dsp_init){
				//SDEBUG("1st zone setting 1004\n");

				//SDEBUG("CPU zone config:\n");
				//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
				//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
				
				//SDEBUG("DSP zone config:\n");
				//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
				//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
				
				//CPU Zone 0
				//Can not adjust the CPU_MEM_BASE, it will cause the kernel crash.
				//REG32(C0DOR0)  = 0;
				REG32(C0DMAR0) = CONFIG_RTL8686_CPU_MEM_SIZE-1;
				
				//CPU Zone 1
				REG32(C0DOR1)  = CONFIG_RTL8686_DSP_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
				REG32(C0DMAR1) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

				//DSP Zone 0
				REG32(C1DOR0)  = CONFIG_RTL8686_DSP_MEM_BASE;
				REG32(C1DMAR0) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

				//DSP Zone 1
				REG32(C1DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
				REG32(C1DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

				//SDEBUG("CPU zone config:\n");
				//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
				//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
			
				//SDEBUG("DSP zone config:\n");
				//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
				//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
			}
			else {
				//SDEBUG("2nd zone setting 1004\n");

				//CPU Zone 1
				REG32(C0DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
				REG32(C0DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

				//SDEBUG("CPU zone config:\n");
				//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
				//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
				
				//SDEBUG("DSP zone config:\n");
				//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
				//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
			}
			break;
		default:
			break;
	}
#else	// !defined(CONFIG_RTL9607C)

	if( SOC_ID == 0x0797 ){
		if (zp==zp_dsp_init){
			//SDEBUG("1st zone setting 1004\n");
			
			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
		
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
			
			
			//CPU Zone 0
			//Can not adjust the CPU_MEM_BASE, it will cause the kernel crash.
			//REG32(C0DOR0)  = 0;
			REG32(C1DMAR0) = CONFIG_RTL8686_CPU_MEM_SIZE-1;
			
			//CPU Zone 1
			REG32(C1DOR1)  = CONFIG_RTL8686_DSP_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C1DMAR1) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

			//DSP Zone 0
			REG32(C0DOR0)  = CONFIG_RTL8686_DSP_MEM_BASE;
			REG32(C0DMAR0) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

			//DSP Zone 1
			REG32(C0DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C0DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
		
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
		}
		else {
			//SDEBUG("2nd zone setting 1004\n");

			//CPU Zone 1
			REG32(C1DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C1DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
			
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
		}
	}
	else if( SOC_ID == 0x0822 || SOC_ID == 0x6518 /*|| SOC_ID == 0x0787*/ ){
		if (zp==zp_dsp_init){
			//SDEBUG("1st zone setting 1004\n");

			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
			
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
			
			//CPU Zone 0
			//Can not adjust the CPU_MEM_BASE, it will cause the kernel crash.
			//REG32(C0DOR0)  = 0;
			REG32(C0DMAR0) = CONFIG_RTL8686_CPU_MEM_SIZE-1;
			
			//CPU Zone 1
			REG32(C0DOR1)  = CONFIG_RTL8686_DSP_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C0DMAR1) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

			//DSP Zone 0
			REG32(C1DOR0)  = CONFIG_RTL8686_DSP_MEM_BASE;
			REG32(C1DMAR0) = CONFIG_RTL8686_DSP_MEM_SIZE-1;

			//DSP Zone 1
			REG32(C1DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C1DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
		
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
		}
		else {
			//SDEBUG("2nd zone setting 1004\n");

			//CPU Zone 1
			REG32(C0DOR1)  = CONFIG_RTL8686_IPC_MEM_BASE + PHYSICAL_SPACE_SIZE - ZONE1_BASE;
			REG32(C0DMAR1) = CONFIG_RTL8686_IPC_MEM_SIZE-1;

			//SDEBUG("CPU zone config:\n");
			//SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
			//SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
			
			//SDEBUG("DSP zone config:\n");
			//SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
			//SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
		}
	}
	else{
		SDEBUG("Please check SOC_ID 0x%04x\n" , SOC_ID );
	}
#endif
#endif	

//	SDEBUG("CPU zone config:\n");
//	SDEBUG("  C0DOR0=%08x C0DMAR0=%08x\n" , REG32(C0DOR0) , REG32(C0DMAR0));
//	SDEBUG("  C0DOR1=%08x C0DMAR1=%08x\n" , REG32(C0DOR1) , REG32(C0DMAR1));
//	
//	SDEBUG("DSP zone config:\n");
//	SDEBUG("  C1DOR0=%08x C1DMAR0=%08x\n" , REG32(C1DOR0) , REG32(C1DMAR0));
//	SDEBUG("  C1DOR1=%08x C1DMAR1=%08x\n" , REG32(C1DOR1) , REG32(C1DMAR1));
	return OK;
}
#endif
#endif //CONFIG_VOIP_LINUX_PLATFORM_DRIVER

#ifndef CONFIG_RTL8686_IPC_DUAL_LINUX
int aipc_dsp_boot(void)
#else
int aipc_dual_linux_boot(void)
#endif
{
#ifdef CONFIG_VOIP_LINUX_PLATFORM_DRIVER
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
	extern struct reset_control *ca_voip_pe_reset;
#endif
	extern struct reset_control *ca_voip_rcpu_reset;

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
	/* pe_reset will impact common part of PEs */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
	if (!reset_control_status(ca_voip_pe_reset))
#endif
	reset_control_assert(ca_voip_pe_reset);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
        if (!reset_control_status(ca_voip_rcpu_reset))
#endif
	reset_control_assert(ca_voip_rcpu_reset);
	mdelay(KICK_DSP_DELAY_TIME);
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
        if (reset_control_status(ca_voip_pe_reset))
#endif
	reset_control_deassert(ca_voip_pe_reset);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
        if (reset_control_status(ca_voip_rcpu_reset))
#endif
	reset_control_deassert(ca_voip_rcpu_reset);
#else

#if defined(CONFIG_ARCH_CPU_MIPS1004K) || defined(CONFIG_RTL8685SB) || \
	defined(CONFIG_AIPC_PLATFORM_0822_5281) || defined(CONFIG_AIPC_PLATFORM_0787_5281)
#ifdef CONFIG_RTL9607C 
	volatile unsigned int tmp = 0;

	switch(aipcBoardChipId)
	{
		case APOLLO_CHIP_ID  : 
			break;
		case APOLLOMP_CHIP_ID: 
			break;
		case RTL9601B_CHIP_ID: 
			break;
		case RTL9602C_CHIP_ID: 
			break;
		case RTL9607C_CHIP_ID: 
			//SDEBUG("before boot slave 5281 REG32(0xb8000044)=0x%08x\n" , REG32(0xb8000044));
			
			tmp = REG32(0xb8000044);
			
			tmp |=  BIT(12); // DISABLE_JTAG,_MIPSIA = 1
			tmp |=  BIT(11); // DISABLE_JTAG_5281 = 1
			tmp &= ~BIT(7);	 // PARALLEL_TDI = 0 
			tmp &= ~BIT(6);  // ONE_EJTAG_SEL = 0
			tmp &= ~BIT(5);  // SYS_CPU2_EN = 0
			//tmp |=  BIT(5);  // SYS_CPU2_EN = 1
			
			REG32(0xb8000044) = tmp;
			mdelay(KICK_DSP_DELAY_TIME);

			//SDEBUG("step1: boot slave 5281 REG32(0xb8000044)=0x%08x\n" , REG32(0xb8000044));
			
			tmp &= ~BIT(12); // DISABLE_JTAG,_MIPSIA = 0
			tmp &= ~BIT(11); // DISABLE_JTAG_5281 = 0
			tmp &= ~BIT(7);	 // PARALLEL_TDI = 0 
			tmp &= ~BIT(6);  // ONE_EJTAG_SEL = 0
			tmp |=  BIT(5);  // SYS_CPU2_EN = 1
			
			REG32(0xb8000044) = tmp;
	
			//SDEBUG("step2: boot slave 5281 REG32(0xb8000044)=0x%08x\n" , REG32(0xb8000044));
			break;
		default:
			break;
	}
#else	// !defined(CONFIG_RTL9607C)	
	if( SOC_ID == 0x0797 ){
		//SDEBUG("before boot slave 5281\n");

		#define ECOS_RESET_VECTOR 0x800004c4
		#define BIR               0xb800411c
		REG32(BIR) = ECOS_RESET_VECTOR;
		
		//SDEBUG("after boot slave 5281\n");
	}
	else if ( SOC_ID == 0x0822 || SOC_ID == 0x6518 /*|| SOC_ID == 0x0787*/ ){
		//SDEBUG("before boot slave 5281 REG32(0xb8000044)=0x%08x\n" , REG32(0xb8000044));
	
		REG32(0xb8000044) &= ~BIT(5);
		mdelay(KICK_DSP_DELAY_TIME);
		REG32(0xb8000044) |=  BIT(5);
		
		//SDEBUG("after boot slave 5281 REG32(0xb8000044)=0x%08x\n" , REG32(0xb8000044));
	}
#endif
#elif defined(CONFIG_RTL8686_ASIC)
    volatile unsigned int *dsp_reg;
    volatile u32_t tmp=0;

	/*
	*	1. add clock to DSP and set DSP TAP on.
	*/
    dsp_reg 	= 	(volatile unsigned int *)(R_AIPC_ASIC_ENABLE_DSP_CLK);
    tmp			=   *dsp_reg;
    
    tmp			&=  ~BIT_ENABLE_DSP_TAP;	// Set BIT(6) as 0 to enable DSP TAP.
    tmp			|= 	BIT_ENABLE_DSP_CLOCK;
    
    *dsp_reg 	= 	tmp;

	/*
	*	2. delay 4 ms
	*/
	mdelay(KICK_DSP_DELAY_TIME);

	/*
	*	3. kick DSP
	*/ 
    dsp_reg 	= 	(volatile unsigned int *)(R_AIPC_ASIC_KICK_DSP);
    *dsp_reg 	|= 	BIT_KICK_DSP;


	/*
	*	4. kernel delay time
	*/
#ifdef KERNEL_BOOT_DELAY_TIME
	mdelay(KERNEL_BOOT_DELAY_TIME);
#endif

#else
    volatile unsigned int *dsp_kick_reg;

    dsp_kick_reg 	= (volatile unsigned int *)(R_AIPC_BOOT_DSP);
    *dsp_kick_reg 	|= BOOT_DSP_BIT;
#endif

#endif /* CONFIG_VOIP_LINUX_PLATFORM_DRIVER */
	return OK;
}
#ifdef AIPC_BOOT
/*
*	Shared memory init function
*/
int aipc_boot_init(void)
{									//DSP booting address
//	ABOOT.cmd[0] = 0x3c08bfc0;		/*lui 	t0,0xbfc0*/
	ABOOT.cmd[0] = 0x3c088000;		/*lui 	t0,0x8000*/
	ABOOT.cmd[1] = 0x35080000;		/*ori 	t0,t0,0x0000*/
	ABOOT.cmd[2] = 0x01000008;		/*jr	t0*/
	ABOOT.cmd[3] = 0x00000000;		/*nop 	  */
	
	return OK;
}
#endif

#endif 
