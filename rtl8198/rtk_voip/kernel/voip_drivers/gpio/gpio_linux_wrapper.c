#include <linux/gpio.h>
#include <asm-generic/gpio.h>
#include "gpio.h"

int32 linux_gpio_init(uint32 gpioId, uint8 direction)
{
	return gpio_request_one(gpioId, direction == GPIO_DIR_OUT ? GPIOF_DIR_OUT : GPIOF_DIR_IN, NULL);
}

int32 linux_gpio_get_value(uint32 gpioId, uint32 *pData)
{
	*pData = __gpio_get_value(gpioId);

	return 0;
}

int32 linux_gpio_set_value(uint32 gpioId, uint32 data)
{
	__gpio_set_value(gpioId, data);

	return 0;
}
