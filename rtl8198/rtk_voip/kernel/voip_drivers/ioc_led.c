#include "rtk_voip.h"
#include "voip_init.h"
#ifndef CONFIG_RTK_VOIP_LINUX_LED
#include "gpio/gpio.h"
#endif

#include "con_register.h"
#ifndef CONFIG_RTK_VOIP_LINUX_LED
#if defined(CONFIG_DEFAULTS_KERNEL_3_18) //&& !defined(CONFIG_LUNA)
#include "bspgpio.h"
#include "bspchip.h"
#elif defined(CONFIG_DEFAULTS_KERNEL_4_4)
//do nothing
#else
#include "gpio.h"
#endif

#ifdef CONFIG_RTK_VOIP_GPIO_8972B
static const uint32 leds_gpio_id[] = {
	PIN_VOIP1_LED,  // CH0 - FXS1: output
	PIN_VOIP2_LED,	// CH1 - FXS2: output
	PIN_VOIP3_LED,  // CH2 - FXS3: output
	PIN_VOIP4_LED,	// CH3 - FXS4: output
	//RFU			// DAA CH0
	//RFU			// DAA CH1
	//RFU			// SIP LED ( = VOIP_CH_NUM = SLIC_CH_NUM + DAA_CH_NUM )
};
#elif defined( CONFIG_RTK_VOIP_GPIO_8954C_V100 ) || defined( CONFIG_RTK_VOIP_GPIO_8954C_V200 )
static const uint32 leds_gpio_id[] = {
	PIN_VOIP1_LED,  // CH0 - FXS1: output
	PIN_VOIP2_LED,	// CH1 - FXS2: output
	PIN_PSTN_LED,	// DAA CH0
	//RFU			// DAA CH1
	//RFU			// SIP LED ( = VOIP_CH_NUM = SLIC_CH_NUM + DAA_CH_NUM )
};
#elif defined(CONFIG_RTK_VOIP_GPIO_8954C_SOUNDWIN_XVN1420) 
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,
	PIN_VOIP1_LED,
};
#elif defined( CONFIG_RTK_VOIP_GPIO_8954C_V400 ) 
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,
	PIN_VOIP1_LED,
	//PIN_VOIP2_LED,
};
#elif defined( CONFIG_RTK_VOIP_GPIO_8972D_V100 ) 
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,
	PIN_VOIP1_LED,
	//PIN_VOIP2_LED,
};
#elif defined (CONFIG_RTK_VOIP_GPIO_8676P_IAD_2LAYER_DEMO_BOARD_V01A) || defined (CONFIG_RTK_VOIP_GPIO_8676PN_IAD_2LAYER_DEMO_BOARD_V01)
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,
};
#elif defined (CONFIG_RTK_VOIP_GPIO_8676P_EWN_IAD_BOARD)
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,  // CH0 - FXS1: output
	PIN_VOIP1_LED,	// CH1 - FXS2: output
	//PIN_PSTN_LED,	// DAA CH0
};
#elif defined(CONFIG_RTK_VOIP_PLATFORM_8686) || (defined(CONFIG_RTK_VOIP_PLATFORM_7742) && !defined(CONFIG_RTK_VOIP_LINUX_GPIO))
static const uint32 leds_gpio_id[] = {
	PIN_VOIP0_LED,  // CH0 - FXS0: Green LED
	PIN_VOIP1_LED,	// CH0 - FXS0: Red LED
};
static const uint32 leds_gpio_active_mode[] = {
	PIN_VOIP0_LED_ACTIVE_MODE,  // CH0 - FXS0: Green LED
	PIN_VOIP1_LED_ACTIVE_MODE,	// CH0 - FXS0: Red LED
};
#endif

#define NUM_OF_LEDS_GPIO	( sizeof( leds_gpio_id ) / sizeof( leds_gpio_id[ 0 ] ) )


static int getIndexByGPIOID(int gpioid){
	int i = 0, size = 0;
	size = sizeof(leds_gpio_id)/sizeof(uint32);
	for ( i = 0 ; i < size ; i ++ ){
		if ( gpioid == leds_gpio_id[i] )
			return i;
	}
	return -1;
}

static void LED_init( void )
{
	int i;
	
	for ( i=0; i < NUM_OF_LEDS_GPIO; i++ ) {
		_rtl_generic_initGpioPin( leds_gpio_id[ i ], GPIO_CONT_GPIO,
								GPIO_DIR_OUT, GPIO_INT_DISABLE );
		/* Turn LED off */
		_rtl_generic_setGpioDataBit( leds_gpio_id[i], 1 );
	}
}

extern int led_function_enable;
static int voip_ioc_gpio_led_set_state( struct voip_ioc_s *this, ioc_state_t state )
{
	const uint32 gpio_id = ( uint32 )this ->priv;
	int index = 0;
#ifdef CONFIG_LEDPWR_PBC
	extern unsigned int ledpwr_toggle_flag;
    if(ledpwr_toggle_flag==0 && state==IOC_STATE_LED_ON)//led power off
       return 0;
#endif
	if ( !led_function_enable ) 
		return 0;

	index = getIndexByGPIOID(gpio_id);
	if ( index == -1 ){
		printk("FXS LED GPIO setting error\n");
		return 0;
	}		

	switch( state ) {
	case IOC_STATE_LED_OFF:	/* off */
		if ( leds_gpio_active_mode[index] == LED_HIGH_ACTIVE )
			_rtl_generic_setGpioDataBit( gpio_id, 0 );
		else 
			_rtl_generic_setGpioDataBit( gpio_id, 1 );
		break;
		
	case IOC_STATE_LED_ON:	/* on */
		if ( leds_gpio_active_mode[index] == LED_HIGH_ACTIVE )
			_rtl_generic_setGpioDataBit( gpio_id, 1 );
		else
			_rtl_generic_setGpioDataBit( gpio_id, 0 );
		break;
		
	default:
		return -1;
	}
	
	return 0;
}

static uint32 voip_ioc_gpio_led_get_id( struct voip_ioc_s *this )
{
	// for display 
	const uint32 gpio_id = ( uint32 )this ->priv;
	
	return gpio_id;
}

// --------------------------------------------------------
// ioc register 
// --------------------------------------------------------

static const ioc_ops_t voip_ioc_gpio_led_ops = {
	.set_state 	= voip_ioc_gpio_led_set_state, 
	.get_id		= voip_ioc_gpio_led_get_id, 
};

static voip_ioc_t voip_ioc_gpio_led[ NUM_OF_LEDS_GPIO ];

int __init voip_init_ioc_gpio_led( void )
{
	int i;
	
	for( i = 0; i < NUM_OF_LEDS_GPIO; i ++ ) {
		voip_ioc_gpio_led[ i ].ioch = i;
		voip_ioc_gpio_led[ i ].name = "LED(g)";
		voip_ioc_gpio_led[ i ].ioc_type = IOC_TYPE_LED;
		voip_ioc_gpio_led[ i ].pre_assigned_snd_ptr = NULL;	// system assign 
		voip_ioc_gpio_led[ i ].ioc_ops = &voip_ioc_gpio_led_ops;
		voip_ioc_gpio_led[ i ].priv = ( void * )leds_gpio_id[ i ];
	}
	
	register_voip_ioc( voip_ioc_gpio_led, NUM_OF_LEDS_GPIO );
	
	LED_init();
	
	return 0;
}

voip_initcall_ioc( voip_init_ioc_gpio_led );

#else
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/leds.h>

#define NUM_OF_VOIP_LEDS	2

DEFINE_LED_TRIGGER(ledtrig_voip[NUM_OF_VOIP_LEDS]);

static char *ledtrig_name[NUM_OF_VOIP_LEDS] = {
        "VOIP0_LED",
        "VOIP1_LED",
};

static void LED_init( void )
{
        int i;

        for (i = 0; i < NUM_OF_VOIP_LEDS; i++)
                led_trigger_register_simple(ledtrig_name[i], &ledtrig_voip[i]);

}

static int voip_ioc_linux_led_set_state( struct voip_ioc_s *this, ioc_state_t state )
{
	const uint32 led_id = ( uint32 )this ->priv;

	switch( state ) {
	case IOC_STATE_LED_OFF:	/* off */
		led_trigger_event(ledtrig_voip[led_id], 0);
		break;

	case IOC_STATE_LED_ON:	/* on */
		led_trigger_event(ledtrig_voip[led_id], 1);
		break;
	default:
		return -1;
	}

	return 0;
}

static uint32 voip_ioc_linux_led_get_id( struct voip_ioc_s *this )
{
	// for display
	const uint32 led_id = ( uint32 )this ->priv;

	return led_id;
}

static const ioc_ops_t voip_ioc_linux_led_ops = {
	.set_state 	= voip_ioc_linux_led_set_state,
	.get_id		= voip_ioc_linux_led_get_id,
};

static voip_ioc_t voip_ioc_linux_led[ NUM_OF_VOIP_LEDS ];

int __init voip_init_ioc_linux_led( void )
{
	int i;

	for( i = 0; i < NUM_OF_VOIP_LEDS; i ++ ) {
		voip_ioc_linux_led[ i ].ioch = i;
		voip_ioc_linux_led[ i ].name = "LED(g)";
		voip_ioc_linux_led[ i ].ioc_type = IOC_TYPE_LED;
		voip_ioc_linux_led[ i ].pre_assigned_snd_ptr = NULL;	// system assign
		voip_ioc_linux_led[ i ].ioc_ops = &voip_ioc_linux_led_ops;
		voip_ioc_linux_led[ i ].priv = ( void * )i;
	}

	register_voip_ioc( voip_ioc_linux_led, NUM_OF_VOIP_LEDS );

	LED_init();

	return 0;
}
voip_initcall_ioc( voip_init_ioc_linux_led );
#endif

