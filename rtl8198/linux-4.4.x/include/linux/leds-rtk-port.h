#ifndef __LINUX_LEDS_RTK_PORT_H
#define __LINUX_LEDS_RTK_PORT_H

int rtk_port_led_hw_blink_request(int port);
int rtk_port_led_hw_blink_on(int port);
int rtk_port_led_hw_blink_off(int port);

#endif /* __LINUX_LEDS_RTK_PORT_H */

