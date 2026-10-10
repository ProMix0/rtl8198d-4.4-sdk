/*
* Copyright c                  Realtek Semiconductor Corporation, 2012  
* All rights reserved.
* 
* Program : GPIO Header File 
* Abstract : 
* Author :                
* 
*/

 
#ifndef __GPIO_8686_H
#define __GPIO_8686_H
#ifdef CONFIG_DEFAULTS_KERNEL_2_6
#ifndef __LINUX_2_6_19__
#include <gpio.h>
#endif
#else
#ifndef CONFIG_RTK_VOIP_PLATFORM_7742
#include <bspgpio.h>
#endif
#endif
/*==================== FOR RTL8686 EVB gpio pin ==================*/


#define PIN_CSEN 27//GPIO_D_3

/* LED */ 

#if defined(CONFIG_RTK_VOIP_PLATFORM_8686) || (defined(CONFIG_RTK_VOIP_PLATFORM_7742) && !defined(CONFIG_RTK_VOIP_LINUX_GPIO))

#ifdef CONFIG_RTK_VOIP_GPIO_9607_IAD_V00
#define LED_TEL_GREEN	31
#define LED_TEL_RED 	1	
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_8685S_IAD_V00)
#define LED_TEL_GREEN	RTL8192CD_GPIO_0
#define LED_TEL_RED 	1	
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTL8685_BOARD_N01)
#define LED_TEL_GREEN	GPIO_D_3
#define LED_TEL_RED	GPIO_B_7
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTL8685_BOARD_VS02)
#define LED_TEL_GREEN	GPIO_D_3 //RTL8192CD_GPIO_0
#define LED_TEL_RED 	1	
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTL9602C_BOARD_V00)
#define LED_TEL_GREEN   30
#define LED_TEL_RED     1
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTL8685P_BOARD_VS1P0B)
#define LED_TEL_GREEN	GPIO_F_6
#define LED_TEL_RED	GPIO_F_7
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_9607_IAD_MS01)
#define LED_TEL_GREEN	45
#define LED_TEL_RED		60
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_8685S_IAD_V02)
#define LED_TEL_GREEN	RTL8192CD_GPIO_2
#define LED_TEL_RED 	1	
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_8685SB_IAD_V00)
#define LED_TEL_GREEN	GPIO_E_7
#define LED_TEL_RED 	GPIO_E_7	
#define LED_TEL_GREEN_ACTIVE_MODE	LED_HIGH_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_HIGH_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_9607_IAD_TW01)
#define LED_TEL_GREEN	45
#define LED_TEL_RED	1
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_CA8277_01)
#define LED_TEL_GREEN	7
#define LED_TEL_RED	7
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_CA8277_02)
#define LED_TEL_GREEN	12
#define LED_TEL_RED	12
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_9607C_8814_V00)
#define LED_TEL_GREEN  38
#define LED_TEL_RED    38
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_9603C_ROTEK)
#define LED_TEL_GREEN  56
#define LED_TEL_RED    56
#define LED_TEL_GREEN_ACTIVE_MODE       LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE         LED_LOW_ACTIVE
#elif defined(CONFIG_RTK_VOIP_GPIO_9603CVD_YUEME_92F)
#define LED_TEL_GREEN  38
#define LED_TEL_RED    38
#define LED_TEL_GREEN_ACTIVE_MODE       LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE         LED_LOW_ACTIVE
#else
#define LED_TEL_GREEN	35
#define LED_TEL_RED  	36
#define LED_TEL_GREEN_ACTIVE_MODE	LED_LOW_ACTIVE
#define LED_TEL_RED_ACTIVE_MODE		LED_LOW_ACTIVE
#endif

#define PIN_VOIP0_LED	LED_TEL_GREEN
#define PIN_VOIP1_LED	LED_TEL_RED

#define PIN_VOIP0_LED_ACTIVE_MODE	LED_TEL_GREEN_ACTIVE_MODE
#define PIN_VOIP1_LED_ACTIVE_MODE	LED_TEL_RED_ACTIVE_MODE
#endif

#if defined(CONFIG_RTK_VOIP_PLATFORM_8686) || (defined(CONFIG_RTK_VOIP_PLATFORM_7742) && !defined(CONFIG_RTK_VOIP_LINUX_GPIO))

/******** GPIO define ********/


/* define GPIO port */
enum GPIO_PORT
{
	GPIO_PORT_A = 0,
	GPIO_PORT_B,
	GPIO_PORT_MAX,
	GPIO_PORT_UNDEF,
};


/* define GPIO direction */
enum GPIO_DIRECTION
{
	GPIO_DIR_IN = 0,
	GPIO_DIR_OUT =1,
};

/* define GPIO Interrupt Type */
enum GPIO_INTERRUPT_TYPE
{
	GPIO_INT_DISABLE = 0,
	GPIO_INT_ENABLE,

};

enum LED_ACTIVE_MODE
{
	LED_LOW_ACTIVE = 0,
	LED_HIGH_ACTIVE,
};

/* Register access macro (REG*()).*/
#if 0
#define REG32(reg) 			(*((volatile uint32 *)(reg)))
#define REG16(reg) 			(*((volatile uint16 *)(reg)))
#define REG8(reg) 			(*((volatile uint8 *)(reg)))
#endif

/*********************  Function Prototype in gpio.c  ***********************/
//int32 _rtl8686_initGpioPin( uint32 gpioId, enum GPIO_DIRECTION direction, enum GPIO_INTERRUPT_TYPE interruptEnable );
int32 _rtl8686_initGpioPin( uint32 gpioId, unsigned char func );
int32 _rtl8686_getGpioDataBit( uint32 gpioId, uint32* pData );
int32 _rtl8686_setGpioDataBit( uint32 gpioId, uint32 data );

#endif

#endif/*__GPIO__*/
