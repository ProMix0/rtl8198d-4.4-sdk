#ifndef __GPIO_LINUX_WRAPPER_H
#define __GPIO_LINUX_WRAPPER_H

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

int32 linux_gpio_init(uint32 gpioId, uint8 direction);
int32 linux_gpio_get_value(uint32 gpioId, uint32 *pData);
int32 linux_gpio_set_value(uint32 gpioId, uint32 data);

#endif
