/*
 * Realtek Semiconductor Corp.
 *
 *
 * Copyright (C) 2019
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include "common/rt_error.h"
#include "rtk/switch.h"
#include "rtk/gpio.h"
#include "rtk/port.h"
#include "rtk/irq.h"
#include "bspchip.h"

__weak void disableVoIP(void) {
	printk("%s(%d): not set\n",__func__,__LINE__);
}

static void do_dying_gasp(void) {
	extern void PCIE_reset_pin(int *reset);
	extern void PCIE1_reset_pin(int *reset);
	extern void gpioClear(int);
	int i;

	//#define GP_VOIP 36
	//rtk_gpio_mode_set(GP_VOIP , GPIO_OUTPUT);
    //rtk_gpio_databit_set(GP_VOIP , 0 );
    //rtk_gpio_state_set(GP_VOIP , ENABLED);
	//printk("%s(%d)\n",__func__,__LINE__);
	disableVoIP();
	#ifdef CONFIG_RTL8686_SLIC_RESET_GPIO_NUM
	rtk_gpio_databit_set(CONFIG_RTL8686_SLIC_RESET_GPIO_NUM , 0);
	#endif
	//rtk_gpio_databit_set(GP_VOIP , 1);


	//#define GP_WIFI 38
	//rtk_gpio_mode_set(GP_WIFI , GPIO_OUTPUT);
    //rtk_gpio_databit_set(GP_WIFI , 0 );
    //rtk_gpio_state_set(GP_WIFI , ENABLED);

	#if 0
	force_stop_wlan_hw();
	PCIE_reset_pin(&gpio);
	#ifdef CONFIG_USE_PCIE_SLOT_1
	PCIE1_reset_pin(&gpio);
	gpioClear(gpio);
	#endif
	rtk_gpio_databit_set(GP_WIFI , 1 );
	#endif
	//printk("phy down\n");
	rtk_port_phyPowerDown_set(0, 1);
	rtk_port_phyPowerDown_set(0, 2);
	rtk_port_phyPowerDown_set(0, 3);
	rtk_port_phyPowerDown_set(0, 4);
	for (i=0; i<0xc; i++) {
		REG32(0xb8005200+(i*0x10)) |= (1<<31);
	}
	#if 1
	REG32(0xb8000600) &= ~(0x6E007FFF);
	REG32(0xb8000638) &= ~(0xC4180000);
	#endif
	//printk("%s(%d)\n",__func__,__LINE__);
	//rtk_gpio_databit_set(GP_WIFI , 1 );
}

static __init int dying_gasp_init(void) {
	int ret;
	ret = rtk_irq_isrDgTh_register(do_dying_gasp);
	printk("Register system dying gasp...%s\n", (ret == RT_ERR_OK) ? "Done" : "Failed" );
	return 0;
}

late_initcall(dying_gasp_init);

