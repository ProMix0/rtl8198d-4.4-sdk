/*
 * Realtek Semiconductor Corp.
 *
 * bsp.h
 * Board Support Package header file
 *
 * Tony Wu (tonywu@realtek.com.tw)
 * Dec. 07, 2007
 */

#ifndef  _BSPCHIP_H_
#define  _BSPCHIP_H_


/*
 *****************************************************************************************
 * DEFINITIONS AND TYPES
 *****************************************************************************************
 */

/* Register Macro */
#if 0 //wei add, barrier issue, so avoid use it
#ifndef REG32
#define REG32(reg)      (*(volatile u32 *)(reg))
#endif
#ifndef REG16
#define REG16(reg)      (*(volatile u16 *)(reg))
#endif
#ifndef REG8
#define REG8(reg)       (*(volatile u8  *)(reg))
#endif
#endif
/*
 *****************************************************************************************
 * Timer
 *****************************************************************************************
 */
#if 0 //wei del
/* A9MP Global timer */
#define GTIMER_IOBASE		(CONFIG_SYS_PERIPHERAL_PADDR + 0x200)
#define GTIMER_FREQ		25000000	/* The frequency of timer module        */

/* DWC timer */
#define TIMER0_IOBASE		0x1FB01000	/* Timer0 register base address mapping */
#define TIMER0_FREQ		25000000	/* The frequency of timer module        */

/* Register offset from TIMERx_IOBASE */
#define TIMER_TLCR1		0x00
#define TIMER_TCV		0x04
#define TIMER_TCR		0x08
#define TIMER_EOIA		0x0c
#endif
/*
 *****************************************************************************************
 * Uart
 *****************************************************************************************
 */

#if 1  //wei del
// Test Eric ***
//#define UART0_IOBASE		0x1FB03000	/* Uart0 register base address mapping  */
#define UART0_IOBASE	0xf8147000
#define UART0_FREQ		25000000	/* Serial Clock                         */

/* Register offset from UARTx_IOBASE */
//#define UART_RBR		0x00
#define UART_RBR		0x24
//#define UART_THR		0x00
#define UART_THR		0x24

#define UART_IER		0x04
#define UART_IIR		0x08
#define UART_LCR		0x0C
#define UART_LSR		0x14
#define UART_DLL		0x00
#define UART_DLH		0x04

/* uart LCR register bits */
#define UART_LCR_PEN		0x00000008	/* parity enable bit */
#define UART_LCR_EPS		0X00000010
#define UART_LCR_PARITY		(UART_LCR_PEN | UART_LCR_EPS)
#define UART_LCR_STOP		0x00000004	/* stop bit          */
#define UART_LCR_DLS		0x00000003	/* 8 bits mode       */
#define UART_LCR_DLAB		0x00000080	/* DLAB bit          */

#define UART_DL_BAUD_9600		(UART_BAUD_BASE / 9600)
#define UART_DL_BAUD_19200		(UART_BAUD_BASE / 19200)
#define UART_DL_BAUD_38400		(UART_BAUD_BASE / 38400)
#define UART_DL_BAUD_57600		(UART_BAUD_BASE / 57600)
#define UART_DL_BAUD_115200		(UART_BAUD_BASE / 115200)

/* baudrate platform specific setting */
#define UART_BAUD_BASE			(UART0_FREQ >> 4)
#define UART_BAUD_DEFAULT		UART_DL_BAUD_38400

#endif

/* Watchdog register */
#define GLOBAL_GLOBAL_CONFIG            0xF432001C
#define PER_SECURE_WDT1_TIMER_ENABLE    0xF4329404

#define PER_WDT1_CTRL			0xF4329408
#define PER_WDT1_PS			0xF432940C
#define PER_WDT1_LD			0xF4329414
#define PER_WDT1_LOADE			0xF4329418



#endif /*  _BSPCHIP_H_ */
