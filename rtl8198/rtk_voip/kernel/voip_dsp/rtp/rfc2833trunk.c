/*
	RFC 2833 trunk event
*/
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#ifdef CONFIG_ARCH_LUNA_SLAVE
#include "bspchip.h"
#endif

#include "dsp_main.h"
#include "rtk_voip.h"
#include "voip_types.h"
#include "voip_control.h"
#include "voip_dev.h"
#include "voip_init.h"
#include "voip_proc.h"
#include "snd_mux_slic.h"
#include "../voip_manager/voip_mgr_events.h"

// trunk event id
#define EVENT_RingOn	144
#define EVENT_ReverseBattery 148
#define EVENT_OnHook	149
#define EVENT_RingOff	149
#define EVENT_OffHook	159

// telephone hook status
#define PHONE_ONHOOK	0
#define PHONE_OFFHOOK	1

#define NAME_SIZE 10

#ifndef CONFIG_ARCH_LUNA_SLAVE
#ifdef CONFIG_RTK_VOIP_PLATFORM_7742
#include <soc/cortina/registers.h>
#define REG32(reg)      (*(volatile unsigned int   *)((unsigned int)reg))
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#include <soc/cortina/ca827x_atu.h>
#endif
#define BSP_TC5_IRQ 2
#define BSP_TC_BASE         PER_TMR_LD1
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
#define BSP_TC_LD           KSEG1_ATU_XLAT(BSP_TC_BASE)
#define BSP_TC_CTL          KSEG1_ATU_XLAT(BSP_TC_BASE + 0x4)
#define BSP_TC_IE0          KSEG1_ATU_XLAT(BSP_TC_BASE + 0xC)
#define BSP_TC5INT          KSEG1_ATU_XLAT(BSP_TC_BASE + 0x14)
#define BSP_TC_LOAD         KSEG1_ATU_XLAT(BSP_TC_BASE + 0x40)
#else
#define BSP_TC_LD           phys_to_virt(BSP_TC_BASE)
#define BSP_TC_CTL          phys_to_virt(BSP_TC_BASE + 0x4)
#define BSP_TC_IE0          phys_to_virt(BSP_TC_BASE + 0xC)
#define BSP_TC5INT          phys_to_virt(BSP_TC_BASE + 0x14)
#define BSP_TC_LOAD         phys_to_virt(BSP_TC_BASE + 0x40)
#endif
#define BSP_TCIP            1
#define BSP_NEW_TMR1LD          0x2
#define BSP_NEW_TCLDMODE        0x40
#define BSP_NEW_TCEN            0x80
#else
#define REG32(reg)      (*(volatile unsigned int   *)((unsigned int)reg))
#define BSP_TC5_IRQ 68
#define BSP_TC_BASE         0xB8003200
#define BSP_TC5CNT          (BSP_TC_BASE + 0x54)
#define BSP_TC5INT          (BSP_TC_BASE + 0x5C)
#define BSP_TCIP             (1 << 16)
#endif
#endif

extern unsigned char RtpOpen[];
extern uint32 rtpConfigOK[];
extern int32 isTranMode(uint32 sid);
extern int RtpTx_transmitEvent_ISR( uint32 chid, uint32 sid, int event);
extern void trunkevent_send(void);

void RFC2833TrunkEventEnable(int chid, unsigned char bEnable);
int RFC2833GenerateTrunkEvent(int chid, unsigned int signal);
int RFC2833TrunkEventOut(int chid, int event);
int RFC2833TrunkEventIn(int chid, int event);

struct timer2_priv{
	char name[NAME_SIZE];
};
static struct timer2_priv timer2ctrl_devices[ 1 ];
static int eventEnable[MAX_DSP_RTK_CH_NUM] = {0, 0};


void RFC2833TrunkEventEnable(int chid, unsigned char bEnable)
{	
	if ( (chid > ( MAX_DSP_RTK_CH_NUM - 1)) || (chid < 0) )
	{
		printk("[%s] chid out of range\n", __FUNCTION__);
		return;
	}
	
	eventEnable[chid] = bEnable;
#ifdef CONFIG_ARCH_LUNA_SLAVE
	if ( bEnable == 0 )
		disable_irq(BSP_TC5_IRQ);
	else if ( bEnable == 1 )
		enable_irq(BSP_TC5_IRQ);
#endif		
}

int RFC2833GenerateTrunkEvent(int chid, unsigned int signal)
{
	extern unsigned char rfc2833_dtmf_pt_local[];
	extern unsigned char rfc2833_dtmf_pt_remote[];
	extern int32 chanInfo_GetRegSessionRank(uint32 chid, uint32 rank[]);
	/**********************************************                 
	 *                               	      *                 
	 * 	    Send RFC2833 Trunk	Event	      *                 
	 *                                  	  *                 
	 ****************************************************/	
	int j = 0, SessNum = 0;	 
	int event = 0;
	uint32 pSessRank[ 2 ];
		               
	if ( (chid >= MAX_DSP_RTK_CH_NUM ) || (chid < 0) )
	{
		printk("[%s] chid out of range\n", __FUNCTION__);
		return 0;
	}					   
					   
	if ( !eventEnable[chid] )
		return 0;
	
	event = (int)signal;
		 
	/* 
	 * send_2833_flag_ss is used to decide to send RFC2833 packets or not.
	 * According to payload type of local and remote UA, it will change the send_2833_flag_ss result.
	 * If payload type of local and remote UA is equal to ZERO, it means DTMF Inband.
	 */
	PRINT_MSG("[%s] channel id = [%d], signal = 0x[%x]\n", __FUNCTION__, chid, signal);
	chanInfo_GetRegSessionRank(chid, pSessRank);

	SessNum = chanInfo_GetRegSessionNum(chid);

	for (j=0; j < SessNum; j++)
	{

		if( pSessRank[j] == SESSION_NULL )
			continue;
			
//		printk("[%s] rfc2833_dtmf_pt_local[pSessRank[%d]] = [%d], rfc2833_dtmf_pt_remote[pSessRank[%d]], = [%d]\n", 
//			__FUNCTION__, j, rfc2833_dtmf_pt_local[pSessRank[j]], j, rfc2833_dtmf_pt_remote[pSessRank[j]]);
		if (rfc2833_dtmf_pt_local[pSessRank[j]]!=0 && rfc2833_dtmf_pt_remote[pSessRank[j]]!=0)//Not DTMF Inband
		{	
			if (RtpOpen[pSessRank[j]] == 0) 		// Check RTP session opened or not 
			{
				continue;
			}

			if ((rtpConfigOK[pSessRank[j]]) == 0)	// Check RTP session enable and hold
			{
				continue;
			}

			if (isTranMode(pSessRank[j]) == 0)	// Check RTP session state if TX available
			{
				continue;
			}
				
			RtpTx_transmitEvent_ISR(chid, pSessRank[j], event);
#ifdef CONFIG_ARCH_LUNA_SLAVE				
			enable_irq(BSP_TC5_IRQ);
#endif
		}
		else
		{
			// if local, or remote is Inband DTMF, then not send 2833 pkts.
			PRINT_MSG("[%s] not RFC2833 mode\n", __FUNCTION__);
			continue;
			//Must not set send_2833_flag_ch = 0 here, send_2833_flag_ch is for this channel DTMF removal
		}
	} /* SessNum */

	return 1;
}

/*
	when detect the telephone event change, send out the related rfc 2833 trunk event
*/
int RFC2833TrunkEventOut(int chid, int status)
{
	if ( (chid >= MAX_DSP_RTK_CH_NUM) || (chid < 0) )
	{
		printk("[%s] chid out of range\n", __FUNCTION__);
		return 0;
	}

	if ( !eventEnable[chid] )
		return 0;
		
	PRINT_MSG("[%s] chid = [%d], status = [%d]\n", __FUNCTION__, chid, status);
	switch (status)
	{
		case PHONE_ONHOOK :
			RFC2833GenerateTrunkEvent( chid, EVENT_OnHook );
			break;
		case PHONE_OFFHOOK :
			RFC2833GenerateTrunkEvent( chid, EVENT_OffHook );
			break;
		default:
			break;
	}
	return 1;
}

/*
	Receive RFC2833 event to do actions
*/
int RFC2833TrunkEventIn(int chid, int event)
{
	if ( (chid >= MAX_DSP_RTK_CH_NUM) || (chid < 0) )
	{
		printk("[%s] chid out of range\n", __FUNCTION__);
		return 0;
	}
		
	if ( !eventEnable[chid] )
		return 0;
		
	PRINT_MSG("[%s] chid = [%d], event = [%d]\n", __FUNCTION__, chid, event);
	switch (event)
	{
		case EVENT_RingOn :
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC
			FXS_Ring(chid, 1);
#endif
			break;
		case EVENT_ReverseBattery :
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC
			SLIC_Set_FXS_Line_State(chid, 6); // 6 is for VP_LINE_TALK_POLREV
#endif
			break;
		case EVENT_RingOff :
#ifdef CONFIG_RTK_VOIP_DRIVERS_ATA_SLIC
			FXS_Ring( chid, 0);
			SLIC_Set_FXS_Line_State(chid, 2); // 2 is for VP_LINE_STANDBY
#endif
			break;
		default:
			break;
	}
	return 1;
}

#if defined(CONFIG_DEFAULTS_KERNEL_2_6) || defined(CONFIG_DEFAULTS_KERNEL_3_4) || defined(CONFIG_DEFAULTS_KERNEL_3_10)
static irqreturn_t timer2_interrupt(int irq, void *dev_instance)
#else
static void timer2_interrupt(int32 irq, void *dev_instance, struct pt_regs *regs)
#endif
{

	trunkevent_send();

#if !defined(CONFIG_RTK_VOIP_PLATFORM_7742)
	REG32(BSP_TC5INT) |= (BSP_TCIP);
#else
	writel(BSP_TCIP, BSP_TC5INT);
#endif
#if defined(CONFIG_DEFAULTS_KERNEL_2_6) || defined(CONFIG_DEFAULTS_KERNEL_3_4) || defined(CONFIG_DEFAULTS_KERNEL_3_10)
	return IRQ_HANDLED;
#endif
}

int __init timer2init(void)
{
	struct timer2_priv *timer2dev;
	char *timer2name = "timer2";
	char strbuf[NAME_SIZE];
	int result;
	
	sprintf(strbuf, timer2name, NAME_SIZE);
	memcpy(timer2ctrl_devices[0].name, strbuf, NAME_SIZE);
	
	timer2dev = &timer2ctrl_devices[0];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0))
	result = request_irq(BSP_TC5_IRQ, timer2_interrupt,  IRQF_PERCPU | IRQF_TIMER,
			timer2dev->name, timer2dev);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
	result = request_irq(BSP_TC5_IRQ, timer2_interrupt, IRQF_DISABLED | IRQF_PERCPU | IRQF_TIMER,
			timer2dev->name, timer2dev);
#else
	result = request_irq(BSP_TC5_IRQ, timer2_interrupt, SA_INTERRUPT,
			timer2dev->name, timer2dev);
#endif

	if(result)
	{
		printk("Can't request IRQ for timer2\n");
		return 0;
	}
	printk("Request IRQ for timer2 OK!.\n");
#ifdef CONFIG_ARCH_LUNA_SLAVE	
	disable_irq(BSP_TC5_IRQ);
#endif	
	return 1;
}
void __exit timer2exit( void )
{
	disable_irq(BSP_TC5_IRQ);
}

static const struct of_device_id cortina_voip_tmr2_of_match[] __initconst = {
		{ .compatible = "cortina,ca-peri", },
		{}
};

static int ca_voip_tmr2_probe(struct platform_device *pdev){
	struct device_node *np = NULL;
	int irq_num;
	struct timer2_priv *timer2dev;
	char *timer2name = "timer2";
	char strbuf[NAME_SIZE];
	int result;
	
#if 0
	readl((void *)(uintptr_t) REG_ADDR)
	writel(REG_DATA, (void *)(uintptr_t) REG_ADDR)
	REG32(BSP_TC_LD)   = (125000000 / 100);
	REG32(BSP_TC_IE0)  = 1;
	REG32(BSP_TC_CTL)  = BSP_NEW_TCEN | BSP_NEW_TCLDMODE ;
	REG32(BSP_TC_LOAD) = BSP_NEW_TMR1LD; 
#else
#if 0
	writel(125000000 / 100, BSP_TC_LD);
	writel(1, BSP_TC_IE0);
	writel(BSP_NEW_TCEN | BSP_NEW_TCLDMODE, BSP_TC_CTL);
	writel(BSP_NEW_TMR1LD, BSP_TC_LOAD);
#endif
#endif

	sprintf(strbuf, timer2name, NAME_SIZE);
	memcpy(timer2ctrl_devices[0].name, strbuf, NAME_SIZE);
	
	timer2dev = &timer2ctrl_devices[0];

	np = pdev->dev.of_node;
	irq_num = irq_of_parse_and_map(np, 0);
	result = request_irq(irq_num , timer2_interrupt, IRQF_PERCPU | IRQF_TIMER,
			timer2dev->name, timer2dev);
	if(result)
	{
		printk("Can't request IRQ for timer2\n");
		return 0;
	}
	printk("Request IRQ for timer2 OK!.\n");
	return 0;
}

static int ca_voip_tmr2_remove(struct platform_device *pdev){
	return 0;
}

static struct platform_driver ca_voip_timer2_driver = {
        .probe = ca_voip_tmr2_probe,
        .remove = ca_voip_tmr2_remove,
        .driver = {
                .owner = THIS_MODULE,
                .name = "ca_voip_timer2",
                .of_match_table = of_match_ptr(cortina_voip_tmr2_of_match),
        },
};

MODULE_DEVICE_TABLE(of, cortina_voip_tmr2_of_match);

static int __init voip_timer2_init_module(void){
	return platform_driver_register(&ca_voip_timer2_driver);
}

void 
voip_timer2_cleanup_module(void)
{
}
#ifndef CONFIG_RTK_VOIP_STANDALONE_ARCH
module_init(voip_timer2_init_module);
module_exit(voip_timer2_cleanup_module);
#endif
#if !defined(CONFIG_RTK_VOIP_PLATFORM_7742)
voip_initcall_bus( timer2init );
voip_exitcall( timer2exit );
#endif
