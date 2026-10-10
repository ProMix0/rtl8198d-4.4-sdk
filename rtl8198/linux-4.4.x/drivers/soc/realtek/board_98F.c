#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/gpio/consumer.h>
#include <linux/leds-ca77xx.h>
#include "rtk_gpio.h"
#include "led-generic.h"
#include "pushbutton.h"

#define POWER_LED   3
#define WIFI_LED    4
#define TRUE    1
#define FALSE   0

#include <linux/notifier.h>
#include <linux/reboot.h>

#define RTK_GPIO_DEBUG 1

static void board_led_clear(void){
     ca77xx_led_enable(POWER_LED, FALSE);
#ifdef CONFIG_WIFI_LED_USE_SOC_GPIO
     //led4 as other3
     ca77xx_led_enable(WIFI_LED, FALSE);
#endif

}

static int board_notify_sys(struct notifier_block *this,
				      unsigned long code, void *unused)
{
	board_led_clear();
    smp_mb();
	return NOTIFY_DONE;
}
static struct notifier_block board_roboot_notifier = {
	.notifier_call = board_notify_sys,
};


/****************************
 * op : LED_ON:1 , LED_OFF:0
 * *************************/
static void board_01_handle_set(int which, int op) {
	//printk("%s: led %d op %d\n", __FUNCTION__, which, op);
	switch (which) {
	case LED_POWER_GREEN:
        ca77xx_led_sw_on(POWER_LED, op);
		break;
#ifdef CONFIG_WIFI_LED_USE_SOC_GPIO
	case LED_WIFI:
        ca77xx_led_sw_on(WIFI_LED, op);
		break;
#endif

	default:
		led_handle_set(which, op);
	}
}

static void board_01_handle_init(void) {
	board_01_handle_set(LED_POWER_GREEN, LED_OFF);
};

static struct led_operations board_01_operation = {
	.name = "board_98f",
	.handle_init = board_01_handle_init,
	.handle_set = board_01_handle_set,
};


static void board_01_pb_init(void) {
    printk("board_01_pb_init\n");
};

static int board_01_pb_is_pushed(int which) {
	switch(which) {
	case PB_RESET:
		if(rtk_gpio->dev_reset_gpio)
		return (gpiod_get_value(rtk_gpio->dev_reset_gpio));
	break;
	//FH_V100 default/WPS uses same button
	case PB_WPS:
		if(rtk_gpio->dev_wps_gpio)
		return (gpiod_get_value(rtk_gpio->dev_wps_gpio));
	break;
	default:
		return 0;
	}
	return 0;
}

static struct pushbutton_operations board_01_pb_op = {
	.handle_init = board_01_pb_init,
	.handle_is_pushed = board_01_pb_is_pushed,
};

/***************************************************************
 * Use linux led drivers
 * So must init after module_platform_driver(ca77xx_leds_driver)
 **************************************************************/

static void board_led_init(void){
	if(rtk_gpio->dev_sys_led_gpio){
		gpiod_set_value(rtk_gpio->dev_sys_led_gpio, 1);
	}
	if(rtk_gpio->dev_wifi_2G_led_gpio){
		gpiod_set_value(rtk_gpio->dev_wifi_2G_led_gpio, 0);
	}
}

/*********************************************************
 * Device Tree Blob
 **********************************************************/

static int __init board_01_led_init(void) {

	//led_register_operations(&board_01_operation);

	pb_register_operations(&board_01_pb_op);
	//board_led_init();

	//platform_driver_register(&rtk_gpio_led_driver);
	board_led_init();

	return 0;
}

static void __exit board_01_led_exit(void) {
}

//module_init(board_01_led_init);
device_initcall_sync(board_01_led_init);
module_exit(board_01_led_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO driver for Reload default");
