#ifndef __SND_PIN_CS_H__
#define __SND_PIN_CS_H__

// PIN_CS list 
extern const uint32 snd_pin_cs[];
// PIN_CS number 
extern const uint32 snd_pin_cs_nr;

#ifdef CONFIG_RTK_VOIP_SOFTWARE_SPI
#ifdef CONFIG_RTK_VOIP_GPIO_8685P_IAD_BT01
#define PIN_CS0		GPIO_H_2
#define PIN_CS1		GPIO_H_3
#define PIN_CS2		GPIO_F_6
#define PIN_CLK		GPIO_G_7
#define PIN_DI		GPIO_G_5
#define PIN_DO		GPIO_G_6
#endif
#endif

#endif /* __SND_PIN_CS_H__ */

