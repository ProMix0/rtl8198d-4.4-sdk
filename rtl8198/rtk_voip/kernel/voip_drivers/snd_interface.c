#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>	/* jiffies is defined */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/delay.h>


#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI) || defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI)
#define ZSI_ISI_PIN_SEL			( ( volatile unsigned int * )0xbb023018 )
#define ZSI_ISI_PIN_EN			( ( volatile unsigned int * )0xbb000174 )
#endif

#define ZSI_ISI_PIN_MODE		( ( volatile unsigned int * )0xB8000600 )
#define PLL_CLOCK_CONTROL       ( ( volatile unsigned int * )0xB8000204 )
#define FRACEN	(1<<3)
#define PCMEN	(1<<2)

#if defined(CONFIG_RTL8685S) || defined(CONFIG_RTL8685SB)
#define PINMUX	( ( volatile unsigned int * )0xB800010C )
#define FRACENREG	( ( volatile unsigned int * )0xB8000228 )
#define BIT27	(1<<27)
#define BIT26	(1<<26)
#define BIT19	(1<<19)
#endif

#ifdef CONFIG_RTL8676
#define ZSI_ISI_PIN_MODE ( ( volatile unsigned int * )0xB800030C )
#define ZSI_ISI	((1<<18) | ( 1<<17))
#define FFT (1<<15)
#define ACC (1<<13)
#define PCM_EN (1<<8)
#define ZSI_FUNCTION	(0<<16)
#define	ISI_FUNCTION	(1<<16)
#endif
#define REG32(reg) (*(volatile unsigned int *)(reg))

#define PCMCNR ( ( volatile unsigned int * )0xB8008004 )

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI 
extern void setISIMSIF_LOCK(unsigned char cs);
#endif

void slicInternalCS(int cs){
	unsigned int regValue = 0;
	mdelay(100); //delay 100ms before slic init
	regValue = *PCMCNR;
	regValue |= (cs<<28);
#if (CONFIG_RTK_VOIP_CON_CH_NUM == 2)
	regValue |= (cs<<20);
#endif
	*PCMCNR = regValue;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI
	printk("cs%d\n", cs);
	setISIMSIF_LOCK(cs);
#endif
}

int __init slicInterface(void){
	unsigned int regValue = 0;

#ifdef CONFIG_RTL8685	
	unsigned int reg_tmp = 0;
	REG32(0xbb804004) = 0x841f0bc0;
	mdelay(1);
	REG32(0xbb804004) = 0x04110000;
	mdelay(1);
	printk("page 0xBC0 reg17 = 0x%x \n\r",REG32(0xbb804008));
	reg_tmp = (REG32(0xbb804008)&0x0FFFF)|(1<<3)|0x84110000;

	mdelay(1);
	REG32(0xbb804004) = reg_tmp;
	mdelay(2);
	REG32(0xbb804004) = 0x04110000;
	mdelay(1);
	printk("page 0xBC0 reg17 = 0x%x \n\r",REG32(0xbb804008));     
#endif	
	
	
#if defined(CONFIG_RTL8685S) || defined(CONFIG_RTL8685SB)
#if 0 // Move PINMUX setting to aipc
	regValue = *PINMUX;
	regValue |= (1<<16);
	*PINMUX	= regValue;
#endif


	{//for 8685S version B
		regValue = *FRACENREG;
		regValue &= ~BIT19;
		*FRACENREG = regValue;

		regValue = *FRACENREG;
		regValue |= BIT19;
		*FRACENREG = regValue;
	}

	regValue = *ZSI_ISI_PIN_MODE;
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI 
	regValue |= BIT27;
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI 
	regValue |= BIT27;
	regValue |= BIT26;
#endif
	regValue |= PCMEN;
	*ZSI_ISI_PIN_MODE = regValue;
#endif

        regValue = *PLL_CLOCK_CONTROL;
        regValue |= FRACEN;
        *PLL_CLOCK_CONTROL = regValue;
	

//	printk("[%s] slic isi/zsi enable\n", __FUNCTION__);
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI 
#ifdef CONFIG_RTL8686
	regValue = *ZSI_ISI_PIN_SEL;
	regValue = regValue & 0xfff9ffff; 
	regValue |= (1<<16);
	*ZSI_ISI_PIN_SEL = regValue;
#endif
	
	regValue = *ZSI_ISI_PIN_EN;
	regValue |= (1<<2);
	*ZSI_ISI_PIN_EN = regValue;

	regValue = *ZSI_ISI_PIN_MODE;//writel(0xB8000600 , readl(0xB8000600) | (1<<26) | (1<<25) );	
#ifdef CONFIG_RTL8686
	regValue |= (1<<26);
#endif
#ifdef CONFIG_RTL8685
	regValue &= 0xf7ffffff;
	regValue |= (1<<27);
#endif
	*ZSI_ISI_PIN_MODE = regValue;
#if 0
	regValue = pcm_inl(0xb8008000);
	regValue |= ZSILBE;
	pcm_outl(0xb8008000, regValue);
#endif
#endif


#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI 
#ifdef CONFIG_RTL8686
	regValue = *ZSI_ISI_PIN_SEL;
	regValue = regValue & 0xfff9ffff; 
	regValue |= (1<<15);
	*ZSI_ISI_PIN_SEL = regValue;
#endif
	regValue = *ZSI_ISI_PIN_EN;
	regValue |= (1<<2);
	*ZSI_ISI_PIN_EN = regValue;
	
	regValue = *ZSI_ISI_PIN_MODE;//writel(0xB8000600 , readl(0xB8000600) | (1<<26) | (1<<25) );	
#ifdef CONFIG_RTL8686
	regValue |= (1<<26);
	regValue |= (1<<25);
#endif
#ifdef CONFIG_RTL8685
	regValue &= 0xf3ffffff;
	regValue |= (1<<26);
	regValue |= (1<<27);

#endif
	*ZSI_ISI_PIN_MODE = regValue;


#ifdef CONFIG_RTL8676
	
        regValue = *ZSI_ISI_PIN_MODE;

	regValue &= 0xffffdfff;
	regValue |= FFT;
	regValue |= ACC;
	regValue |= PCM_EN;

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI
	regValue |= ZSI_ISI;
	regValue &= 0xfffeffff;
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI
	regValue |= ZSI_ISI;
        regValue |= (1<<16);
#endif
        *ZSI_ISI_PIN_MODE = regValue;	
		
	printk("\n\nPINMUX = 0x[%x]\n\n", *ZSI_ISI_PIN_MODE);
#endif

#if 0
	regValue = pcm_inl(0xbb023018);
	regValue = regValue & 0xfff9ffff; 
	regValue |= (1<<15);
	pcm_outl(0xbb023018, regValue);
	
	regValue = pcm_inl(0xbb000174);
	regValue |= (1<<2);
	pcm_outl(0xbb000174 , regValue);

	pcm_outl(0xB8000600 , pcm_inl(0xB8000600) | (1<<26) | (1<<25) );	

	regValue = pcm_inl(0xb8008000);
	regValue |= ISILBE;
	pcm_outl(0xb8008000, regValue);
	printk("0xb8008000 = [%04x]\n", pcm_inl(0xb8008000));
#endif
#endif
	return 0;
}

module_init(slicInterface);




#define CLKEN		( ( volatile unsigned int * )0xB8000204 )
#define PINEN		( ( volatile unsigned int * )0xB800010C )
#define IPEN		( ( volatile unsigned int * )0xB8000600 )
#define PCMMODULEEN		( ( volatile unsigned int * )0xB8008000 )


#define SPI1		( ( volatile unsigned int * )0xB8009000 )
#define SPI2		( ( volatile unsigned int * )0xB8009004 )
#define SPI3		( ( volatile unsigned int * )0xB8009008 )
#define SPI4		( ( volatile unsigned int * )0xB800900C )
#define SPI5		( ( volatile unsigned int * )0xB8009010 )
#define SPI6		( ( volatile unsigned int * )0xB8009014 )
#define SPI7		( ( volatile unsigned int * )0xB8009018 )
#define SPI8		( ( volatile unsigned int * )0xB800901C )

// b8000600 IPEN
#define en_voipfft	(1<<13)
#define en_voipacc	(1<<12)
#define en_pcm		(1<<2)
#define en_peri_voip	(1<<0)

// b800010c PINEN
#define pcm_slicvoip_sel_dir2 (1<<16)
#define pcm_slicvoip_sel_dir1 (1<<15)

void disableSPIPCMIP(void){
	unsigned int regValue = 0;

#ifdef CONFIG_8685S
		regValue = *FRACENREG;
		regValue &= ~BIT19;
		*FRACENREG = regValue;

			regValue = *CLKEN;
			regValue &= ~PCMEN;
			*CLKEN = regValue;

			regValue = *IPEN;
			regValue &= ~(en_voipfft|en_voipacc|en_pcm|en_peri_voip);
			*IPEN = regValue;

			regValue = *PINEN;
			regValue &= ~(pcm_slicvoip_sel_dir2|pcm_slicvoip_sel_dir2);			
			*PINEN = regValue;
#endif
			
			regValue = *PCMMODULEEN;
			regValue = 0x0;			
			*PCMMODULEEN = regValue;

			regValue = *SPI1;
			regValue = 0x0;			
			*SPI1 = regValue;

			regValue = *SPI2;
			regValue = 0x0;			
			*SPI2 = regValue;

			regValue = *SPI3;
			regValue = 0x0;			
			*SPI3 = regValue;

			regValue = *SPI4;
			regValue = 0x0;			
			*SPI4 = regValue;

			regValue = *SPI5;
			regValue = 0x0;			
			*SPI5 = regValue;

			regValue = *SPI6;
			regValue = 0x0;			
			*SPI6 = regValue;

			regValue = *SPI7;
			regValue = 0x0;			
			*SPI7 = regValue;

			regValue = *SPI8;
			regValue = 0x0;			
			*SPI8 = regValue;
}
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC
#if defined(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) || defined(CONFIG_RTK_VOIP_STANDALONE_ARCH)
extern void disableSLIC(void);
void disableVoIP(void){
	disableSLIC();
	disableSPIPCMIP();
}
#endif
#endif

