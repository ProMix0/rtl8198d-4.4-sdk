voip_spi-y +=

ifeq ($(CONFIG_RTK_VOIP_LINUX_SPI),y)
voip_spi-y += voip_drivers/spi/spi_linux_wrapper.o
else
voip_spi-y += voip_drivers/spi/spi.o
endif
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8186) += voip_drivers/spi/spi_8186.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8651) += voip_drivers/spi/spi_8651bv.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8671) += voip_drivers/spi/spi_8972.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM865xC) += voip_drivers/spi/spi_8952.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8672) += voip_drivers/spi/spi_8672.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8676) += voip_drivers/spi/spi_8676.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY) += voip_drivers/spi/spi_8972b.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC) += voip_drivers/spi/spi_8954c.o
voip_spi-$(CONFIG_RTK_VOIP_DRIVERS_PCM89xxD) += voip_drivers/spi/spi_8972d.o

OBJS_CFLAGS :=
OBJS_CFLAGS += -I$(DIR_VOIP)/include
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_drivers
OBJS_CFLAGS += -I$(DIR_VOIP)/voip_dsp/

#include $(DIR_VOIP)/rules.mak

CFLAGS_spi_linux_wrapper.o := $(OBJS_CFLAGS)
CFLAGS_spi.o := $(OBJS_CFLAGS)
CFLAGS_spi_8186.o := $(OBJS_CFLAGS)
CFLAGS_spi_8651bv.o := $(OBJS_CFLAGS)
CFLAGS_spi_8972.o := $(OBJS_CFLAGS)
CFLAGS_spi_8952.o := $(OBJS_CFLAGS)
CFLAGS_spi_8672.o := $(OBJS_CFLAGS)
CFLAGS_spi_8676.o := $(OBJS_CFLAGS)
CFLAGS_spi_8972b.o := $(OBJS_CFLAGS)
CFLAGS_spi_8954c.o := $(OBJS_CFLAGS)
CFLAGS_spi_8972d.o := $(OBJS_CFLAGS)
