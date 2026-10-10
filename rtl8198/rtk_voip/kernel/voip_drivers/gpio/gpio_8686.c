/*
* Copyright c                  Realtek Semiconductor Corporation, 2012  
* All rights reserved.
* 
* Program : GPIO Driver
* Abstract : 
* Author : 
*/
#if defined(CONFIG_ARCH_CORTINA)
#if defined(CONFIG_LEDS_CA77XX)
#include <linux/leds-ca77xx.h>
#define TRUE 1
#define FALSE 0

int _rtl8686_initGpioPin(unsigned int LED_NO, unsigned char func)
{
	ca77xx_led_enable(LED_NO, TRUE); 
}

int _rtl8686_getGpioDataBit( unsigned int gpioId, unsigned int* pData )
{
}

int _rtl8686_setGpioDataBit( unsigned int LED_NO, unsigned int data )
{
	if( data == 0)
		ca77xx_led_sw_on(LED_NO, TRUE); // ON=TRUE, OFF=FALSE
	else
		ca77xx_led_sw_on(LED_NO, FALSE); // ON=TRUE, OFF=FALSE

	return 0;
}
#elif defined(CONFIG_LEDS_RTK_GPIO_LED)
#include<led-generic.h>
extern void rtk_led_sw_on(int gpio_led_num , int op);
int _rtl8686_initGpioPin(unsigned int LED_NO, unsigned char func)
{
	return 0;
}

int _rtl8686_getGpioDataBit( unsigned int gpioId, unsigned int* pData )
{
	return 0;
}

int _rtl8686_setGpioDataBit( unsigned int LED_NO, unsigned int data )
{
        if( data == 0)
               rtk_led_sw_on(LED_FXS, LED_ON); 
        else
               rtk_led_sw_on(LED_FXS, LED_OFF);

        return 0;
}

#endif
#else
#include "gpio.h"

extern void gpioConfig (int gpio_num, int gpio_func);
extern void gpioSet(int gpio_num);
extern void gpioClear(int gpio_num);
extern int gpioRead(int gpio_num) ;

int gpio_debug = 0;
extern int spi_dbg;
// Goal : unifiy GPIO functions
#if 0
//static uint32 _getGpio( enum GPIO_FUNC func, enum GPIO_PORT port, uint32 pin )
static uint32 _getGpio( unsigned char port, uint32 pin )
{
	return 0;
}
//static void _setGpio( enum GPIO_FUNC func, enum GPIO_PORT port, uint32 pin, uint32 data )
static void _setGpio( unsigned char port, uint32 pin, uint32 data )
{
	return 0;
}
#endif
//int32 _rtl8686_initGpioPin( uint32 gpioId, enum GPIO_DIRECTION direction, 
//                                           enum GPIO_INTERRUPT_TYPE interruptEnable )
int32 _rtl8686_initGpioPin(uint32 gpioId, unsigned char func)
{
	int ret = 0;
    if(func == 0)
        gpioConfig(gpioId, 0x01);
    else
        gpioConfig(gpioId, 0x02);
	return ret;
}

int32 _rtl8686_getGpioDataBit( uint32 gpioId, uint32* pData )
{
	int ret = 0;
//	unsigned int val;
//	if (gpioId >7)	return 0; //skip non-GPA

//	val = *(volatile int*)GPABCDDATA;

//	*pData = (val >> gpioId) & 0x1 ;
	*pData = gpioRead(gpioId);
	return ret;
}

int32 _rtl8686_setGpioDataBit( uint32 gpioId, uint32 data )
{

    if( data == 1)
        gpioSet(gpioId);
    else
        gpioClear(gpioId);

	return 0;
}
#endif

