#include "bspgpio.h"

/* Legacy Code. Phasing out */

#ifdef CONFIG_GPIOLIB
#include <linux/gpio.h>

static unsigned long requested[BIT_WORD(GPIO_END)];

void gpioConfig(int gpio_num, int gpio_func) {
	
	//if (unlikely(gpio_num>=GPIO_END))
	//	return;
	
	if (unlikely(!gpio_is_valid(gpio_num)))
		return;
	
	if (!test_bit(gpio_num,requested)) {
		if (gpio_request(gpio_num,"rtk_old_gpio"))
			return;
		set_bit(gpio_num,requested);
	}
	
	if (gpio_func==GPIO_FUNC_OUTPUT)
		gpio_direction_output(gpio_num,0);
	else
		gpio_direction_input(gpio_num);
}

void gpioSet(int gpio_num) {
	if (unlikely(!gpio_is_valid(gpio_num)))
		return;	
	gpio_set_value(gpio_num,1);
}

void gpioClear(int gpio_num) {
	if (unlikely(!gpio_is_valid(gpio_num)))
		return;	
	gpio_set_value(gpio_num,0);
}

int gpioRead(int gpio_num) {
	if (unlikely(!gpio_is_valid(gpio_num)))
		return 0;	
	return gpio_get_value(gpio_num);
}
#else 
#include <rtk/gpio.h>
void gpioConfig(int gpio_num, int gpio_func) {
	rtk_gpio_state_set(gpio_num, 1); /* enable gpio */
	rtk_gpio_mode_set(gpio_num, gpio_func == GPIO_FUNC_OUTPUT);
}

void gpioSet(int gpio_num) {
	rtk_gpio_databit_set(gpio_num, 1);
}

void gpioClear(int gpio_num) {
	rtk_gpio_databit_set(gpio_num, 0);
}

int gpioRead(int gpio_num) {
	unsigned int data = 0;
	rtk_gpio_databit_get(gpio_num, &data);
	return data;
}
#endif 

EXPORT_SYMBOL(gpioConfig);
EXPORT_SYMBOL(gpioSet);
EXPORT_SYMBOL(gpioClear);
EXPORT_SYMBOL(gpioRead);
