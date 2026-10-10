#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of.h>
#ifdef CONFIG_RTK_VOIP_LINUX_GPIO
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#endif
#include "voip_init.h"

#define MAX_SLIC_SPI_NUM	8

/*#define _rtl_spi_rawWrite(pDev, pData, bits) ca77xx_spi_write(pDev, pData, (bits/8))
#define _rtl_spi_rawRead(pDev, pData, bits) ca77xx_spi_read(pDev, pData, (bits/8))
#define _rtl_spi_transRead8(pDev, regCtrl, regAddr, reg_val) ca77xx_spi_w16r8(pDev, regCtrl, regAddr, reg_val)
#define _rtl_spi_transWrite8(pDev, regCtrl, regAddr, reg_val) ca77xx_spi_w24(pDev, regCtrl, regAddr, reg_val)*/

extern int ca77xx_spi_transfer_one(struct spi_master *master,
				   struct spi_device *spi,
				   struct spi_transfer *tfr);

int
_rtl_spi_rawWrite(struct spi_device *spi, const void *buf, int len)
{
	struct spi_transfer	t = {
			.tx_buf		= buf,
			.len		= (len/8),
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI
			.delay_usecs	= 8,	/* 12T = 12*(1/PCLK) PCLK >= 1536KHz */
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI)
			.delay_usecs	= 15,
#endif
		};

	return ca77xx_spi_transfer_one(spi->master, spi, &t);
}

int
_rtl_spi_rawRead(struct spi_device *spi, void *buf, int len)
{
	struct spi_transfer	t = {
			.rx_buf		= buf,
			.len		= (len/8),
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI
			.delay_usecs	= 8, /* 12T = 12*(1/PCLK), PCLK >= 1536KHz */
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI)
			.delay_usecs	= 15,
#endif
		};

	return ca77xx_spi_transfer_one(spi->master, spi, &t);
}

int _rtl_spi_transRead8(struct spi_device *spidev,
				   unsigned char ctrl, unsigned char addr,
				   unsigned char *rxbuf)
{
	unsigned char txbuf[2];
	int ret;

	txbuf[0] = ctrl;
	txbuf[1] = addr;

	ret = _rtl_spi_rawWrite(spidev, txbuf, 16);
	if (ret)
		return ret;
	else
		return _rtl_spi_rawRead(spidev, rxbuf, 8);
}

int _rtl_spi_transWrite8(struct spi_device *spidev, unsigned char ctrl,
				 unsigned char addr, unsigned char data)
{
	unsigned char txbuf[3];

	txbuf[0] = ctrl;
	txbuf[1] = addr;
	txbuf[2] = data;

	return _rtl_spi_rawWrite(spidev, txbuf, 24);
}

EXPORT_SYMBOL(_rtl_spi_rawWrite);
EXPORT_SYMBOL(_rtl_spi_rawRead);
EXPORT_SYMBOL(_rtl_spi_transRead8);
EXPORT_SYMBOL(_rtl_spi_transWrite8);

static struct spi_device *slic_spi[MAX_SLIC_SPI_NUM];
static int spi_proble_index;

int init_spi_dev(struct spi_device **spi_dev, int index)
{
	printk("%s(index = %d)\n", __func__, index);

	if (index >= MAX_SLIC_SPI_NUM)
		return -1;

	*spi_dev = slic_spi[index];

	printk("%s(spi_dev %p)\n", __func__, *spi_dev);

	return 0;
}

EXPORT_SYMBOL(init_spi_dev);

/*  Pull in any device tree parameters */
#ifdef CONFIG_OF
static void slic_of_probe(struct spi_device *spi)
{
#ifdef CONFIG_RTK_VOIP_LINUX_GPIO
	struct device_node *np = spi->dev.of_node;
	enum of_gpio_flags flags;
	int reset_gpio;

	extern int slic_reset_gpio;
	extern unsigned long slic_reset_flags;

	if (gpio_is_valid(slic_reset_gpio))
		return;

	reset_gpio = of_get_named_gpio_flags(np, "reset-gpio", 0, &flags);
	if (reset_gpio == -EPROBE_DEFER)
		return;

	if (gpio_is_valid(reset_gpio)) {
		unsigned long gpio_flags;

		if (flags & OF_GPIO_ACTIVE_LOW)
			gpio_flags = GPIOF_ACTIVE_LOW | GPIOF_OUT_INIT_LOW;
		else
			gpio_flags = GPIOF_OUT_INIT_HIGH;
		dev_info(&spi->dev, "reset-gpio %d is active %s\n", reset_gpio,
			 (flags & OF_GPIO_ACTIVE_LOW) ? "low" : "high");

		slic_reset_gpio = reset_gpio;
		slic_reset_flags = gpio_flags;
	}
#endif

#ifdef CONFIG_ARCH_CORTINA
	spi->mode &= ~(SPI_CPHA | SPI_CPOL | SPI_XSI);

  #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
    #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI
	spi->mode |= SPI_CPHA | SPI_CPOL | SPI_XSI;
    #endif
  #endif
  #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB
    #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI
	spi->mode |= SPI_CPHA | SPI_XSI;
    #else
	spi->mode |= SPI_CPHA | SPI_CPOL;
    #endif
  #endif
  #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
    #ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_CSI
	spi->mode |= SPI_CPHA | SPI_CPOL | SPI_XSI;
    #else
	spi->mode |= SPI_CPHA | SPI_CPOL;
    #endif
  #endif
#endif
}
#endif

/*****************************************************************************************************/
int slic_spi_probe(struct spi_device *spi)
{
	printk(KERN_INFO "SLIC module being probed\n");
#if 0//def CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
	spi->bits_per_word = 48;
#endif
	slic_spi[spi_proble_index++] = spi;

	printk("%s: slic_spi[ %d ] = %p\n", __func__, spi_proble_index-1, slic_spi[ spi_proble_index-1 ]);

#ifdef CONFIG_OF
	slic_of_probe(spi);
#endif

	return 0;
}

/*****************************************************************************************************/
int slic_spi_remove(struct spi_device *spi)
{
	printk(KERN_INFO "SLIC module being removed");

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id slic_spi_match[] =
{
	{ .compatible = "microsemi,voicepath_spi", },
	{ .compatible = "silabs,proslic_spi", },
	{ .compatible = "lantiq,duslicxs", },
	{ .compatible = "realtek,support-slic", },
	{ }
};
MODULE_DEVICE_TABLE(of, slic_spi_match);
#endif

static struct spi_driver slic_spi_drv =
{
	.driver = {
		.name = "slic_spi",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = slic_spi_match,
#endif
	},
	.probe = slic_spi_probe,
	.remove = slic_spi_remove,
};

int spi_linux_wrapper_init(void)
{
	int i, rc;

	printk(KERN_INFO "spi_linux_wrapper_init()\n");

	for (i = 0; i < MAX_SLIC_SPI_NUM; i++)
		slic_spi[i] = NULL;
	spi_proble_index = 0;

	rc = spi_register_driver(&slic_spi_drv);

	if (rc)
		printk(KERN_ERR "%s(%d): spi_register driver returned = %d",
		       __FUNCTION__, __LINE__, rc);
	else
		printk(KERN_DEBUG "%s(%d): spi driver registered",
		       __FUNCTION__, __LINE__);

	printk(KERN_NOTICE "%s(%d): completed", __FUNCTION__, __LINE__);
	return rc;
}
module_init(spi_linux_wrapper_init);

static void spi_linux_wrapper_exit(void)
{
	int i;

	spi_unregister_driver(&slic_spi_drv);

	spi_proble_index = 0;
	for (i = 0; i < MAX_SLIC_SPI_NUM; i++)
		slic_spi[i] = NULL;
}
module_exit(spi_linux_wrapper_exit);
