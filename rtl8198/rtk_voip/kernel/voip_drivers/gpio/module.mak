voip_gpio-y :=

voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8186) += voip_drivers/gpio/gpio_8186.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8651) += voip_drivers/gpio/gpio_8651bv.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8671) += voip_drivers/gpio/gpio_8972.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM865xC) += voip_drivers/gpio/gpio_8952.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8672) += voip_drivers/gpio/gpio_8672.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8676) += voip_drivers/gpio/gpio_8676.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY) += voip_drivers/gpio/gpio_8972b.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC) += voip_drivers/gpio/gpio_8954c.o
voip_gpio-$(CONFIG_RTK_VOIP_DRIVERS_PCM89xxD) += voip_drivers/gpio/gpio_8972d.o
voip_gpio-$(CONFIG_RTK_VOIP_SOC_8686_CPU0) += voip_drivers/gpio/gpio_8686.o
ifeq ($(CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY)$(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC),y)
#voip_gpio-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += voip_drivers/gpio/gpio_smi.o
endif
voip_gpio-$(CONFIG_RTK_VOIP_LINUX_GPIO) += voip_drivers/gpio/gpio_linux_wrapper.o

ifeq ($(CONFIG_RTK_VOIP_PLATFORM_7742), y)
ifneq ($(CONFIG_RTK_VOIP_LINUX_GPIO), y)
voip_gpio-y += voip_drivers/gpio/gpio_8686.o
endif
endif

OBJS_CFLAGS :=
ifeq ($(CONFIG_LEDS_RTK_GPIO_LED), y)
OBJS_CFLAGS += -I$(DIR_LINUX)/drivers/soc/realtek/
endif

OBJS_CFLAGS += -I$(DIR_VOIP)/include

#include $(DIR_VOIP)/rules.mak

CFLAGS_gpio_8186.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8651bv.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8972.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8952.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8672.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8676.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8972b.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8954c.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8972d.o := $(OBJS_CFLAGS)
CFLAGS_gpio_8686.o := $(OBJS_CFLAGS)
CFLAGS_gpio_smi.o := $(OBJS_CFLAGS)
CFLAGS_gpio_linux_wrapper.o := $(OBJS_CFLAGS)
