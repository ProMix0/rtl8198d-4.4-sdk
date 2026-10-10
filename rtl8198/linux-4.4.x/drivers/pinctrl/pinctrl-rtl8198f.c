/* Copyright (C) 2017 Realtek Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include "core.h"
#include "pinctrl-utils.h"

#define RTL8198F_NUM_IOMUX	77

/*
 * RTL8198F IOMUX register description
 *
 * @offset: register offset for mux configuration of a group
 * @shift: bit shift for mux configuration of a group
 * @val: value of pinmux function to set to
 */
struct rtl8198f_mux {
	unsigned int offset;
	unsigned int shift;
	unsigned int val;
};

/*
 * Description of a RTL8198F pin
 * @pin : pin number
 * @name : pin_name
 * @gpio_val : gpio mux value
 * @mux : pin based IOMUX configuration
 */
struct rtl8198f_pin {
	unsigned pin;
	char *name;
	unsigned int gpio_val;
	struct rtl8198f_mux mux;
};

/*
 * Keep track of IOMUX configuration and prevent double configuration
 *
 * @rtl8198f_mux: RTL8198F IOMUX register description
 * @is_configured: flag to indicate whether a mux setting has already been
 * configured
 */
struct rtl8198f_mux_log {
	struct rtl8198f_mux mux;
	bool is_configured;
};

/*
 * Group based IOMUX configuration
 *
 * @name: name of the group
 * @pins: array of pins used by this group
 * @num_pins: total number of pins used by this group
 * @mux: group based IOMUX configuration
 */
struct rtl8198f_pin_group {
	const char *name;
	const unsigned *pins;
	unsigned num_pins;
	const struct rtl8198f_mux *mux;
};

/*
 * RTL8198F mux function and supported pin groups
 *
 * @name: name of the function
 * @groups: array of groups that can be supported by this function
 * @num_groups: total number of groups that can be supported by this function
 */
struct rtl8198f_pin_function {
	const char *name;
	const char * const *groups;
	unsigned num_groups;
};

/*
 * RTL1898F IOMUX pinctrl core
 *
 * @pctl: pointer to pinctrl_dev
 * @dev: pointer to device
 * @base: base address of pinmux control register
 * @groups: pointer to array of groups
 * @num_groups: total number of groups
 * @functions: pointer to array of functions
 * @num_functions: total number of functions
 * @mux_log: pointer to the array of mux logs
 * @lock: lock to protect register access
 */
struct rtl8198f_pinctrl {
	struct pinctrl_dev *pctl;
	struct device *dev;
	void __iomem *base;

	const struct rtl8198f_pin_group *groups;
	unsigned num_groups;

	const struct rtl8198f_pin_function *functions;
	unsigned num_functions;

	struct rtl8198f_mux_log *mux_log;

	spinlock_t lock;
};

#define RTL8198F_PIN_DESC(p, n, g, o, s, a)	\
{						\
	.pin = p,				\
	.name = n,				\
	.gpio_val = g,				\
	.mux = {				\
		.offset = o,			\
		.shift = s,			\
		.val = a,			\
	}					\
}

/*
 * List of share pins in 8198F
 */
static const struct rtl8198f_pin rtl8198f_pins[] = {
	RTL8198F_PIN_DESC(0, "GPIO_000", 0x7, 0x4, 0, 0x0),
	RTL8198F_PIN_DESC(1, "GPIO_001", 0x7, 0x4, 12, 0x0),
	RTL8198F_PIN_DESC(2, "GPIO_002", 0xa, 0x4, 16, 0x0),
	RTL8198F_PIN_DESC(3, "GPIO_003", 0x7, 0x4, 28, 0x0),
	RTL8198F_PIN_DESC(4, "GPIO_004", 0x6, 0x4, 4, 0x0),
	RTL8198F_PIN_DESC(5, "GPIO_005", 0x7, 0x4, 8, 0x0),
	RTL8198F_PIN_DESC(6, "GPIO_006", 0x2, 0x4, 20, 0x0),
	RTL8198F_PIN_DESC(7, "GPIO_007", 0x2, 0x4, 24, 0x0),
	RTL8198F_PIN_DESC(8, "GPIO_008", 0x6, 0x14, 28, 0x6),
	RTL8198F_PIN_DESC(9, "GPIO_009", 0x7, 0x14, 24, 0x7),
	RTL8198F_PIN_DESC(10, "GPIO_010", 0x8, 0x14, 20, 0x8),
	RTL8198F_PIN_DESC(11, "GPIO_011", 0xa, 0x18, 8, 0x1),
	RTL8198F_PIN_DESC(12, "GPIO_012", 0x9, 0x14, 12, 0x1),
	RTL8198F_PIN_DESC(13, "GPIO_013", 0x9, 0x14, 8, 0x1),
	RTL8198F_PIN_DESC(14, "GPIO_014", 0x9, 0x14, 4, 0x1),
	RTL8198F_PIN_DESC(15, "GPIO_015", 0x8, 0x14, 0, 0x8),
	RTL8198F_PIN_DESC(16, "GPIO_016", 0x8, 0x14, 16, 0x1),
	RTL8198F_PIN_DESC(17, "GPIO_017", 0x8, 0x18, 0, 0x8),
	RTL8198F_PIN_DESC(18, "GPIO_018", 0x6, 0x18, 4, 0x6),
	RTL8198F_PIN_DESC(19, "GPIO_019", 0x7, 0x18, 12, 0x7),
	RTL8198F_PIN_DESC(20, "GPIO_020", 0x6, 0x10, 24, 0x6),
	RTL8198F_PIN_DESC(21, "GPIO_021", 0x6, 0x10, 28, 0x6),
	RTL8198F_PIN_DESC(22, "GPIO_022", 0x2, 0x10, 20, 0x2),
	RTL8198F_PIN_DESC(23, "GPIO_023", 0x4, 0x10, 8, 0x4),
	RTL8198F_PIN_DESC(24, "GPIO_024", 0x9, 0x10, 16, 0x9),
	RTL8198F_PIN_DESC(25, "GPIO_025", 0xb, 0x10, 12, 0xb),
	RTL8198F_PIN_DESC(26, "GPIO_026", 0x7, 0x10, 0, 0x7),
	RTL8198F_PIN_DESC(27, "GPIO_027", 0x6, 0x10, 4, 0x6),
	RTL8198F_PIN_DESC(28, "GPIO_028", 0x8, 0xc, 0, 0x8),
	RTL8198F_PIN_DESC(29, "GPIO_029", 0x8, 0xc, 4, 0x8),
	RTL8198F_PIN_DESC(30, "GPIO_030", 0x8, 0xc, 8, 0x8),
	RTL8198F_PIN_DESC(31, "GPIO_031", 0x8, 0xc, 12, 0x8),
	RTL8198F_PIN_DESC(32, "GPIO_100", 0x9, 0xc, 16, 0x9),
	RTL8198F_PIN_DESC(33, "GPIO_101", 0x8, 0xc, 20, 0x8),
	RTL8198F_PIN_DESC(34, "GPIO_102", 0x9, 0xc, 24, 0x9),
	RTL8198F_PIN_DESC(35, "GPIO_103", 0x9, 0xc, 28, 0x9),
	RTL8198F_PIN_DESC(36, "GPIO_104", 0x5, 0x0, 4, 0x5),
	RTL8198F_PIN_DESC(37, "GPIO_105", 0x5, 0x0, 8, 0x5),
	RTL8198F_PIN_DESC(38, "GPIO_106", 0x4, 0x0, 12, 0x4),
	RTL8198F_PIN_DESC(39, "GPIO_107", 0x3, 0x0, 16, 0x3),
	RTL8198F_PIN_DESC(40, "GPIO_108", 0x2, 0x0, 0, 0x2),
	RTL8198F_PIN_DESC(41, "GPIO_109", 0x1, 0x0, 20, 0x0),
	RTL8198F_PIN_DESC(42, "GPIO_110", 0x1, 0x0, 24, 0x0),
	RTL8198F_PIN_DESC(43, "GPIO_111", 0x1, 0x0, 28, 0x0),
	RTL8198F_PIN_DESC(44, "GPIO_112", 0x5, 0x8, 4, 0x0),
	RTL8198F_PIN_DESC(45, "GPIO_113", 0x8, 0x8, 0, 0x0),
	RTL8198F_PIN_DESC(46, "GPIO_114", 0x8, 0x8, 16, 0x0),
	RTL8198F_PIN_DESC(47, "GPIO_115", 0x9, 0x8, 8, 0x0),
	RTL8198F_PIN_DESC(48, "GPIO_116", 0x9, 0x8, 12, 0),
	RTL8198F_PIN_DESC(49, "GPIO_117", 0x1, 0x1c, 0, 0x0),
	RTL8198F_PIN_DESC(50, "GPIO_118", 0x1, 0x1c, 4, 0x0),
	RTL8198F_PIN_DESC(51, "GPIO_119", 0x1, 0x1c, 8, 0x0),
	RTL8198F_PIN_DESC(52, "GPIO_120", 0x1, 0x1c, 12, 0x0),
	RTL8198F_PIN_DESC(53, "GPIO_121", 0x1, 0x1c, 16, 0x0),
	RTL8198F_PIN_DESC(54, "GPIO_122", 0x1, 0x1c, 20, 0x0),
	RTL8198F_PIN_DESC(55, "GPIO_123", 0x1, 0x1c, 24, 0x0),
	RTL8198F_PIN_DESC(56, "GPIO_124", 0x1, 0x1c, 28, 0x0),
	RTL8198F_PIN_DESC(57, "GPIO_125", 0x2, 0x20, 0, 0x0),
	RTL8198F_PIN_DESC(58, "GPIO_126", 0x4, 0x20, 4, 0x0),
	RTL8198F_PIN_DESC(59, "GPIO_127", 0x4, 0x20, 8, 0x0),
	RTL8198F_PIN_DESC(60, "GPIO_128", 0x6, 0x20, 12, 0x0),
	RTL8198F_PIN_DESC(61, "GPIO_129", 0x6, 0x20, 16, 0x0),
	RTL8198F_PIN_DESC(62, "GPIO_130", 0x1, 0x20, 20, 0x0),
	RTL8198F_PIN_DESC(63, "GPIO_131", 0x1, 0x20, 24, 0x0),
	RTL8198F_PIN_DESC(64, "GPIO_200", 0x1, 0x20, 28, 0x0),
	RTL8198F_PIN_DESC(65, "GPIO_201", 0x1, 0x24, 0, 0x0),
	RTL8198F_PIN_DESC(66, "GPIO_202", 0x1, 0x24, 4, 0x0),
	RTL8198F_PIN_DESC(67, "GPIO_203", 0x1, 0x24, 8, 0x0),
	RTL8198F_PIN_DESC(68, "GPIO_204", 0x1, 0x24, 12, 0x0),
	RTL8198F_PIN_DESC(69, "GPIO_205", 0x1, 0x24, 16, 0x0),
	RTL8198F_PIN_DESC(70, "GPIO_206", 0x1, 0x24, 20, 0x0),
	RTL8198F_PIN_DESC(71, "GPIO_207", 0x3, 0x24, 24, 0x0),
	RTL8198F_PIN_DESC(72, "GPIO_208", 0x3, 0x24, 28, 0x0),
	RTL8198F_PIN_DESC(73, "GPIO_209", 0x2, 0x28, 0, 0x0),
	RTL8198F_PIN_DESC(74, "GPIO_210", 0x2, 0x28, 4, 0x0),
	RTL8198F_PIN_DESC(75, "GPIO_211", 0x3, 0x28, 8, 0x0),
	RTL8198F_PIN_DESC(76, "GPIO_212", 0x3, 0x28, 12, 0x0),
};

/*
 * List of groups of pins
 */

static const unsigned ca_uart1_pins[] = { 45, 46, 47, 48 };
static const unsigned ca_ssp_pins[] = { 71, 72, 73, 74, 75, 76 };

static const unsigned event_0_pins[] = { 12, 13, 14, 15 };
static const unsigned event_1_pins[] = { 36, 37, 38, 39 };
static const unsigned event_2_pins[] = { 58, 59, 60, 61 };

static const unsigned i1pps_pins[] = { 5 };
static const unsigned o1pps_pins[] = { 5 };

/* i2c0_clk, i2c0_data */
static const unsigned i2c0_0_pins[] = { 0, 1 };
static const unsigned i2c0_1_pins[] = { 18, 19 };
static const unsigned i2c0_2_pins[] = { 20, 21 };
static const unsigned i2c0_3_pins[] = { 34, 35 };
static const unsigned i2c0_4_pins[] = { 48, 47 };
static const unsigned i2c0_5_pins[] = { 71, 72 };
/* i2c0-slave_clk, i2c0-slave_data */
static const unsigned i2c0_s_0_pins[] = { 17, 19 };
static const unsigned i2c0_s_1_pins[] = { 34, 35 };

/* i2c1_clk, i2c1_data */
static const unsigned i2c1_0_pins[] = { 2, 3 };
static const unsigned i2c1_1_pins[] = { 4, 5 };
static const unsigned i2c1_2_pins[] = { 10, 11 };
static const unsigned i2c1_3_pins[] = { 16, 17 };
static const unsigned i2c1_4_pins[] = { 20, 21 };
static const unsigned i2c1_5_pins[] = { 36, 37 };
static const unsigned i2c1_6_pins[] = { 60, 61 };
static const unsigned i2c1_7_pins[] = { 75, 76 };
/* i2c1-slave_clk, i2c1-slave_data */
static const unsigned i2c1_s_pins[] = { 2, 3 };

static const unsigned iis_0_pins[] = { 3, 1, 0, 4, 24, 25 };
static const unsigned iis_1_pins[] = { 3, 1, 0, 4, 24, 5 };
static const unsigned iis_2_pins[] = { 9, 10, 8, 11, 24, 25 };
static const unsigned iis_3_pins[] = { 13, 14, 12, 11, 24, 25 };
static const unsigned iis_4_pins[] = { 13, 14, 12, 15, 24, 25 };
static const unsigned iis_5_pins[] = { 29, 28, 26, 27, 24, 25 };
static const unsigned iis_6_pins[] = { 29, 28, 30, 27, 24, 25 };
static const unsigned iis_7_pins[] = { 47, 48, 30, 31, 24, 25 };
static const unsigned iis_8_pins[] = { 47, 48, 60, 61, 24, 25 };
static const unsigned iis_9_pins[] = { 58, 59, 60, 61, 24, 25 };

static const unsigned iisa_0_pins[] = { 2, 9 };
static const unsigned iisa_1_pins[] = { 8, 9 };
static const unsigned iisa_2_pins[] = { 10, 11 };
static const unsigned iisa_3_pins[] = { 32, 33 };
static const unsigned iisa_4_pins[] = { 44, 45 };

static const unsigned iisv_0_pins[] = { 2 };
static const unsigned iisv_1_pins[] = { 11 };
static const unsigned iisv_2_pins[] = { 32 };
static const unsigned iisv_3_pins[] = { 44 };

/* tck, trst_n, tms, tdi, tdo */
static const unsigned jtag_0_pins[] = { 11, 12, 13, 14, 16 };
static const unsigned jtag_1_pins[] = { 45, 44, 46, 47, 48 };

static const unsigned led_pins[] = { 39, 38, 37, 36, 40 };
static const unsigned led_s_pins[] = { 36, 37 };

static const unsigned mii_pins[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21 };
static const unsigned mmc_pins[] = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35 };

static const unsigned nand_pins[] = { 0, 1, 2, 3, 4, 5, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35 };

static const unsigned pcie_pins[] = { 41 };

/* pck_out, pfs_out, tx_out, tx_in */
static const unsigned pcm_0_pins[] = { 12, 13, 14, 15 };
static const unsigned pcm_1_pins[] = { 32, 33, 34, 35 };
static const unsigned pcm_2_pins[] = { 46, 45, 48, 47 };

static const unsigned pwm_0_pins[] = { 8, 9, 10, 11 };
static const unsigned pwm_1_pins[] = { 16, 17, 18, 19 };
static const unsigned pwm_2_pins[] = { 32, 33, 34, 35 };
static const unsigned pwm_3_pins[] = { 36, 37, 38, 39 };
static const unsigned pwm_4_pins[] = { 45, 46, 47, 48 };

static const unsigned spi0_0_pins[] = { 1, 2, 3, 0, 14, 22, 15 };
static const unsigned spi0_1_pins[] = { 10, 11, 9, 8, 14, 22, 15 };
static const unsigned spi0_2_pins[] = { 12, 13, 21, 20, 14, 22, 15 };
static const unsigned spi0_3_pins[] = { 24, 25, 27, 26, 14, 22, 15 };
static const unsigned spi0_4_pins[] = { 24, 25, 27, 26, 14, 23, 15 };
static const unsigned spi0_5_pins[] = { 60, 61, 59, 58, 14, 44, 15 };

static const unsigned spi0_3w_0_pins[] = { 0, 2, 3, 22 };
static const unsigned spi0_3w_1_pins[] = { 8, 11, 9, 22 };
static const unsigned spi0_3w_2_pins[] = { 20, 25, 27, 22 };
static const unsigned spi0_3w_3_pins[] = { 20, 25, 27, 23 };
static const unsigned spi0_3w_4_pins[] = { 26, 25, 27, 22 };
static const unsigned spi0_3w_5_pins[] = { 26, 25, 27, 23 };
static const unsigned spi0_3w_6_pins[] = { 58, 61, 59, 44 };

static const unsigned spi1_0_pins[] = { 16, 17, 18, 19, 22 };
static const unsigned spi1_1_pins[] = { 20, 21, 18, 19, 22 };
static const unsigned spi1_2_pins[] = { 20, 21, 24, 25, 22 };
static const unsigned spi1_3_pins[] = { 26, 27, 24, 25, 22 };
static const unsigned spi1_4_pins[] = { 28, 33, 24, 25, 22 };
static const unsigned spi1_5_pins[] = { 32, 33, 24, 25, 22 };

static const unsigned spi1_3w_0_pins[] = { 16, 19, 17, 22 };
static const unsigned spi1_3w_1_pins[] = { 20, 19, 21, 22 };
static const unsigned spi1_3w_2_pins[] = { 20, 25, 27, 22 };
static const unsigned spi1_3w_3_pins[] = { 20, 26, 27, 22 };
static const unsigned spi1_3w_4_pins[] = { 32, 35, 33, 22 };

static const unsigned spiflsh_pins[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
/* SCLK, FSC, TXD, RXD */
static const unsigned ssp_0_pins[] = { 12, 13, 14, 15 };
static const unsigned ssp_1_pins[] = { 32, 33, 34, 35 };
static const unsigned ssp_2_pins[] = { 46, 45, 48, 47 };
static const unsigned sti_pins[] = { 1, 2, 3 };

static const unsigned trace_pins[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 24, 25, 28, 29, 30, 31 };

static const unsigned uart_pins[] = { 42, 43 };
/* urts, ucts, urxd, utxd */
static const unsigned rt_uart1_0_pins[] = { 3, 2, 1, 0 };
static const unsigned rt_uart1_1_pins[] = { 16, 19, 17, 18 };
static const unsigned rt_uart1_2_pins[] = { 20, 19, 21, 18 };
static const unsigned rt_uart1_3_pins[] = { 30, 29, 28, 31 };
static const unsigned rt_uart1_4_pins[] = { 32, 35, 33, 31 };
static const unsigned rt_uart1_5_pins[] = { 32, 35, 33, 34 };
static const unsigned rt_uart1_6_pins[] = { 45, 46, 47, 48 };
/* urts, ucts, urxd, utxd */
static const unsigned rt_uart0_0_pins[] = { 12, 15, 14, 13 };
static const unsigned rt_uart0_1_pins[] = { 30, 29, 28, 31 };
static const unsigned rt_uart0_2_pins[] = { 45, 46, 47, 48 };

/* voltage probe */
static const unsigned vp_pins[] = { 24, 25 };

/* FIXME WIFI/WLAN ? */

static const unsigned xout_0_pins[] = { 4 };
static const unsigned xout_1_pins[] = { 9 };
static const unsigned xout_2_pins[] = { 10 };
static const unsigned xout_3_pins[] = { 16 };
static const unsigned xout_4_pins[] = { 38 };
static const unsigned xout_5_pins[] = { 44 };
static const unsigned xout_6_pins[] = { 46 };
static const unsigned xout_7_pins[] = { 57 };
static const unsigned xout_8_pins[] = { 60 };

/*
 * List of groups of function mux
 */

static const struct rtl8198f_mux ca_uart1_mux[] = { { 0x8, 0, 3 }, { 0x8, 16, 3 },
	{ 0x8, 8, 3 }, { 0x8, 12, 3 } };
static const struct rtl8198f_mux ca_ssp_mux[] = { { 0x24, 24, 1 }, { 0x24, 28, 1 },
	{ 0x28, 0, 1 }, { 0x28, 4, 1 }, { 0x28, 8, 1 }, { 0x28, 12, 1 } };

static const struct rtl8198f_mux event_0_mux[] = { { 0x14, 12, 7 }, { 0x14, 8, 7 },
	{ 0x14, 4, 7 }, { 0x14, 0, 6 } };
static const struct rtl8198f_mux event_1_mux[] = { { 0x0, 4, 3 }, { 0x0, 8, 3 },
	{ 0x0, 12, 2 }, { 0x0, 16, 2 } };
static const struct rtl8198f_mux event_2_mux[] = { { 0x20, 4, 3 }, { 0x20, 8, 3 },
	{ 0x20, 12, 4 }, { 0x20, 16, 5 } };

static const struct rtl8198f_mux i1pps_mux[] = { { 0x4, 8, 5 } };
static const struct rtl8198f_mux o1pps_mux[] = { { 0x4, 8, 6 } };

static const struct rtl8198f_mux i2c0_0_mux[] = { { 0x4, 0, 4 }, { 0x4, 12, 4 } };
static const struct rtl8198f_mux i2c0_1_mux[] = { { 0x18, 4, 3 }, { 0x4, 12, 4 } };
static const struct rtl8198f_mux i2c0_2_mux[] = { { 0x10, 24, 3 }, { 0x10, 28, 3 } };
static const struct rtl8198f_mux i2c0_3_mux[] = { { 0xc, 24, 3 }, { 0xc, 28, 4 } };
static const struct rtl8198f_mux i2c0_4_mux[] = { { 0x8, 12, 7 }, { 0x8, 8, 7 } };
static const struct rtl8198f_mux i2c0_5_mux[] = { { 0x24, 24, 2 }, { 0x24, 28, 2 } };
static const struct rtl8198f_mux i2c0_s_0_mux[] = { { 0x18, 0, 2 }, { 0x18, 12, 4 } };
static const struct rtl8198f_mux i2c0_s_1_mux[] = { { 0xc, 24, 4 }, { 0xc, 28, 4 } };

static const struct rtl8198f_mux i2c1_0_mux[] = { { 0x4, 16, 5 }, { 0x4, 28, 4 } };
static const struct rtl8198f_mux i2c1_1_mux[] = { { 0x4, 4, 4 }, { 0x4, 8, 4 } };
static const struct rtl8198f_mux i2c1_2_mux[] = { { 0x14, 20, 2 }, { 0x18, 8, 4 } };
static const struct rtl8198f_mux i2c1_3_mux[] = { { 0x14, 16, 4 }, { 0x18, 0, 4 } };
static const struct rtl8198f_mux i2c1_4_mux[] = { { 0x10, 24, 4 }, { 0x10, 28, 4 } };
static const struct rtl8198f_mux i2c1_5_mux[] = { { 0x0, 4, 4 }, { 0x0, 8, 4 } };
static const struct rtl8198f_mux i2c1_6_mux[] = { { 0x20, 12, 3 }, { 0x20, 16, 4 } };
static const struct rtl8198f_mux i2c1_7_mux[] = { { 0x28, 8, 2 }, { 0x28, 12, 2 } };
static const struct rtl8198f_mux i2c1_s_mux[] = { { 0x4, 16, 6 }, { 0x4, 28, 4 } };

static const struct rtl8198f_mux iis_0_mux[] = { { 0x4, 28, 3 }, { 0x4, 12, 3 },
	{ 0x4, 0, 3 }, { 0x4, 4, 3 }, { 0x10, 16, 5 }, { 0x10, 12, 6 } };
static const struct rtl8198f_mux iis_1_mux[] = { { 0x4, 28, 3 }, { 0x4, 12, 3 },
	{ 0x4, 0, 3 }, { 0x4, 4, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_2_mux[] = { {0x14, 24, 2 }, {0x14, 20, 3 },
	{0x14, 28, 2 }, {0x18, 8, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_3_mux[] = { { 0x14, 8, 4 }, { 0x14, 4, 4 },
	{ 0x14, 12, 4 }, { 0x18, 8, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_4_mux[] = { { 0x14, 8, 4 }, { 0x14, 4, 4 },
	{ 0x14, 12, 4 }, { 0x14, 0, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_5_mux[] = { { 0xc, 4, 4 }, { 0xc, 0, 4 },
	{ 0x10, 0, 4 }, { 0x10, 4, 4 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_6_mux[] = { { 0xc, 4, 4 }, { 0xc, 0, 4 },
	{ 0xc, 8, 4 }, { 0x10, 4, 4 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_7_mux[] = { { 0x8, 8, 6 }, { 0x8, 12, 6 },
	{ 0xc, 8, 4 }, { 0xc, 12, 4 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_8_mux[] = { { 0x8, 8, 6 }, { 0x8, 12, 6 },
	{ 0x20, 12, 2 }, { 0x20, 16, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };
static const struct rtl8198f_mux iis_9_mux[] = { { 0x20, 4, 2 }, { 0x20, 8, 2 },
	{ 0x20, 12, 2 }, { 0x20, 16, 3 }, { 0x10, 16, 5 }, { 0x4, 8, 3 } };

static const struct rtl8198f_mux iisa_0_mux[] = { { 0x4, 16, 3 }, { 0x15, 24, 3 } };
static const struct rtl8198f_mux iisa_1_mux[] = { { 0x14, 28, 3 }, { 0x16, 24, 3 } };
static const struct rtl8198f_mux iisa_2_mux[] = { { 0x14, 20, 4 }, { 0x18, 8, 7 } };
static const struct rtl8198f_mux iisa_3_mux[] = { { 0xc, 16, 3 }, { 0xc, 20, 4 } };
static const struct rtl8198f_mux iisa_4_mux[] = { { 0x8, 4, 2 }, { 0x8, 0, 6 } };
static const struct rtl8198f_mux iisv_0_mux[] = { { 0x4, 16, 4 } };
static const struct rtl8198f_mux iisv_1_mux[] = { { 0x18, 8, 5 } };
static const struct rtl8198f_mux iisv_2_mux[] = { { 0xc, 16, 3 } };
static const struct rtl8198f_mux iisv_3_mux[] = { { 0x8, 4, 3 } };

static const struct rtl8198f_mux jtag_0_mux[] = { { 0x18, 8, 1 }, { 0x14, 12, 1 },
	{ 0x14, 8, 1 }, { 0x14, 4, 1 }, { 0x14, 16, 1 } };
static const struct rtl8198f_mux jtag_1_mux[] = { { 0x8, 0, 0 }, { 0x8, 4, 0 },
	{ 0x8, 16, 0 }, { 0x8, 8, 0 }, { 0x8, 12, 0 } };

static const struct rtl8198f_mux led_mux[] = { { 0x0, 16, 0 }, { 0x0, 12, 0 },
	{ 0x0, 8, 0 }, { 0x0, 4, 0 }, { 0x0, 0, 0 } };
static const struct rtl8198f_mux led_s_mux[] = { { 0x0, 4, 1 }, { 0x0, 8, 1 } };

static const struct rtl8198f_mux mii_mux[] = { { 0x14, 28, 0 }, { 0x14, 24, 0 },
	{ 0x14, 20, 0 }, { 0x18, 8, 0 }, { 0x14, 12, 0 }, { 0x14, 8, 0 },
	{ 0x14, 4, 0 }, { 0x14, 0, 0 }, { 0x14, 16, 0 }, { 0x18, 0, 0 },
	{ 0x18, 4, 0 }, { 0x18, 12, 0 }, { 0x10, 24, 0 }, { 0x10, 28, 0 } };
static const struct rtl8198f_mux mmc_mux[] = { { 0x10, 8, 0 }, { 0x10, 16, 0 },
	{ 0x10, 12, 0 }, { 0x10, 0, 0 }, { 0x10, 4, 0 }, { 0xc, 0, 0 },
	{ 0xc, 4, 0 }, { 0xc, 8, 0 }, { 0xc, 12, 0 }, { 0xc, 16, 0 },
	{ 0xc, 20, 0 }, { 0xc, 24, 0 }, { 0xc, 28, 0 } };
static const struct rtl8198f_mux nand_mux[] = { { 0x4, 0, 1 }, { 0x4, 12, 1 },
	{ 0x4, 16, 1 }, { 0x4, 28, 1 }, { 0x4, 4, 1 }, { 0x4, 8, 1 },
	{ 0x10, 0, 1 }, { 0x10, 4, 1 }, { 0xc, 0, 1 }, { 0xc, 4, 1 },
	{ 0xc, 8, 1 }, { 0xc, 12, 1 }, { 0xc, 16, 1 }, { 0xc, 20, 1 },
	{ 0xc, 24, 1 }, { 0xc, 28, 1 } };
static const struct rtl8198f_mux pcie_mux[] = { { 0x0, 20, 0 } };

static const struct rtl8198f_mux pcm_0_mux[] = { { 0x14, 12, 5 }, { 0x14, 8, 5 },
	{ 0x14, 4, 5 }, {0x14, 0, 4 } };
static const struct rtl8198f_mux pcm_1_mux[] = { { 0xc, 16, 6 }, { 0xc, 20, 5 },
	{ 0xc, 24, 6 }, { 0xc, 28, 6 } };
static const struct rtl8198f_mux pcm_2_mux[] = { { 0x8, 16, 1 }, { 0x8, 0, 1 },
	{ 0x8, 12, 1 }, { 0x8, 8, 1 } };

static const struct rtl8198f_mux pwm_0_mux[] = { { 0x14, 28, 4 }, { 0x14, 24, 5 },
	{ 0x14, 20, 6 }, { 0x18, 8, 8 } };
static const struct rtl8198f_mux pwm_1_mux[] = { { 0x14, 16, 6 }, { 0x18, 0, 6 },
	{ 0x18, 4, 4 }, { 0x18, 12, 5 } };
static const struct rtl8198f_mux pwm_2_mux[] = { { 0xc, 16, 8 }, { 0xc, 20, 7 },
	{ 0xc, 24, 8 }, { 0xc, 28, 8 } };
static const struct rtl8198f_mux pwm_3_mux[] = { { 0x0, 4, 2 }, { 0x0, 8, 2 },
	{ 0x0, 12, 1 }, { 0x0, 16, 1 } };
static const struct rtl8198f_mux pwm_4_mux[] = { { 0x8, 0, 7 }, { 0x8, 16, 6 },
	{ 0x8, 8, 8 }, { 0x8, 12, 8 } };

static const struct rtl8198f_mux spi0_0_mux[] = { { 0x4, 0, 6 }, { 0x4, 12, 8 },
	{ 0x4, 16, 6 }, { 0x4, 28, 6 }, { 0x14, 4, 2 }, { 0x10, 20, 0 },
	{ 0x14, 0, 1 } };
static const struct rtl8198f_mux spi0_1_mux[] = { { 0x14, 20, 1 }, { 0x18, 8, 2 },
	{ 0x14, 24, 1 }, { 0x14, 28, 1 }, { 0x14, 4, 2 }, { 0x10, 20, 2 },
	{ 0x14, 0, 1 } };
static const struct rtl8198f_mux spi0_2_mux[] = { { 0x14, 12, 2 }, { 0x14, 8, 2 },
	{ 0x10, 28, 1 }, { 0x10, 24, 1 }, { 0x14, 4, 2 }, { 0x10, 20, 2 },
	{ 0x14, 0, 1 } };
static const struct rtl8198f_mux spi0_3_mux[] = { { 0x10, 16, 2 }, { 0x10, 12, 2 },
	{ 0x10, 4, 2 }, { 0x10, 0, 2 }, { 0x14, 4, 2 }, { 0x10, 20, 2 },
	{ 0x14, 0, 1 } };
static const struct rtl8198f_mux spi0_4_mux[] = { { 0x10, 16, 2 }, { 0x10, 12, 2 },
	{ 0x10, 4, 2 }, { 0x10, 0, 2 }, { 0x14, 4, 2 }, { 0x10, 8, 1 },
	{ 0x14, 0, 1 } };
static const struct rtl8198f_mux spi0_5_mux[] = { { 0x20, 12, 1 }, { 0x20, 16, 1 },
	{ 0x20, 8, 1 }, { 0x20, 4, 1 }, { 0x14, 4, 2 }, { 0x8, 4, 1 },
	{ 0x14, 0, 1 } };

static const struct rtl8198f_mux spi0_3w_0_mux[] = { { 0x4, 0, 6 }, { 0x4, 16, 9 },
	{ 0x4, 28, 6 }, { 0x10, 20, 0 } };
static const struct rtl8198f_mux spi0_3w_1_mux[] = { { 0x14, 28, 1 }, { 0x18, 8, 3 },
	{ 0x14, 24, 1 }, { 0x10, 20, 0 } };
static const struct rtl8198f_mux spi0_3w_2_mux[] = { { 0x10, 24, 1 }, { 0x10, 12, 8 },
	{ 0x10, 0, 2 }, { 0x10, 20, 0 } };
static const struct rtl8198f_mux spi0_3w_3_mux[] = { { 0x10, 24, 1 }, { 0x10, 12, 8 },
	{ 0x10, 0, 2 }, { 0x10, 8, 1 } };
static const struct rtl8198f_mux spi0_3w_4_mux[] = { { 0x10, 0, 2 }, { 0x10, 12, 8 },
	{ 0x10, 0, 2 }, { 0x10, 20, 0 } };
static const struct rtl8198f_mux spi0_3w_5_mux[] = { { 0x10, 0, 2 }, { 0x10, 12, 8 },
	{ 0x10, 0, 2 }, { 0x10, 8, 1 } };
static const struct rtl8198f_mux spi0_3w_6_mux[] = { { 0x20, 4, 1 }, { 0x20, 16, 2 },
	{ 0x20, 8, 1 }, { 0x8, 4, 1 } };

static const struct rtl8198f_mux spi1_0_mux[] = { { 0x14, 16, 2 }, { 0x18, 0, 1 },
	{ 0x18, 4, 1 }, { 0x18, 12, 1 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_1_mux[] = { { 0x10, 24, 2 }, {0x10, 28, 2 },
	{ 0x18, 4, 1 }, { 0x18, 12, 1 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_2_mux[] = { { 0x10, 24, 2 }, {0x10, 28, 2 },
	{ 0x10, 16, 3 }, { 0x10, 12, 3 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_3_mux[] = { { 0x10, 0, 3 }, { 0x10, 4, 3 },
	{ 0x10, 16, 3 }, { 0x10, 12, 3 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_4_mux[] = { { 0xc, 0, 2 }, { 0xc, 20, 2 },
	{ 0x10, 16, 3 }, { 0x10, 12, 3 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_5_mux[] = { { 0xc, 16, 2 }, { 0xc, 20, 2 },
	{ 0x10, 16, 3 }, { 0x10, 12, 3 }, { 0x10, 20, 1 } };

static const struct rtl8198f_mux spi1_3w_0_mux[] = { { 0x14, 16, 2 }, { 0x18, 12, 2 },
	{ 0x18, 0, 4 }, { 0x10, 28, 1 } };
static const struct rtl8198f_mux spi1_3w_1_mux[] = { { 0x10, 24, 2 }, { 0x18, 12, 2 },
	{ 0x10, 28, 2 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_3w_2_mux[] = { { 0x14, 16, 2 }, { 0x10, 12, 4 },
	{ 0x10, 4, 4 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_3w_3_mux[] = { { 0x14, 16, 2 }, { 0x10, 0, 3 },
	{ 0x10, 4, 4 }, { 0x10, 20, 1 } };
static const struct rtl8198f_mux spi1_3w_4_mux[] = { { 0xc, 16, 2 }, { 0xc, 28, 3 },
	{ 0xc, 20, 2 }, { 0x10, 20, 1 } };

static const struct rtl8198f_mux spiflsh_mux[] = { { 0x4, 0, 2 }, { 0x4, 12, 2 },
	{ 0x4, 16, 2 }, { 0x4, 28, 2 }, { 0x4, 4, 2 }, { 0x4, 8, 2 },
	{ 0x4, 20, 1 }, { 0x4, 24, 1 } };

static const struct rtl8198f_mux ssp_0_mux[] = { { 0x14, 12, 6 }, { 0x14, 8, 6 },
	{ 0x14, 4, 6 }, { 0x14, 0, 5 } };
static const struct rtl8198f_mux ssp_1_mux[] = { { 0xc, 16, 7 }, { 0xc, 20, 6 },
	{ 0xc, 24, 7 }, { 0xc, 28, 7 } };
static const struct rtl8198f_mux ssp_2_mux[] = { { 0x8, 16, 2 }, { 0x8, 0, 2 },
	{ 0x8, 12, 2 }, { 0x8, 8, 2 } };

static const struct rtl8198f_mux sti_mux[] = { { 0x4, 12, 9 }, { 0x4, 16, 0 },
	{ 0x4, 28, 9 } };
static const struct rtl8198f_mux trace_mux[] = { { 0x14, 28, 5 }, { 0x14, 24, 6 },
	{ 0x14, 20, 7 }, { 0x18, 8, 9 }, { 0x14, 12, 8 }, { 0x14, 8, 8 },
	{ 0x14, 4, 7 }, { 0x14, 0, 7 }, { 0x14, 16, 7 }, { 0x18, 0, 7 },
	{ 0x18, 4, 5 }, { 0x18, 12, 6 }, { 0x10, 16, 8 }, { 0x10, 12, 10 },
	{ 0xc, 0, 7 }, { 0xc, 4, 7 }, { 0xc, 8, 7 }, { 0xc, 12, 7 } };

static const struct rtl8198f_mux uart_mux[] = { { 0x0, 24, 0 }, { 0x0, 28, 0 } };
static const struct rtl8198f_mux rt_uart1_0_mux[] = { { 0x4, 28, 5 }, { 0x4, 16, 7 },
	{ 0x4, 12, 5 }, { 0x4, 0, 5 } };
static const struct rtl8198f_mux rt_uart1_1_mux[] = { { 0x14, 16, 3 }, { 0x18, 12, 3 },
	{ 0x18, 0, 2 }, { 0x18, 4, 2 } };
static const struct rtl8198f_mux rt_uart1_2_mux[] = { { 0x10, 24, 5 }, { 0x18, 12, 3 },
	{ 0x10, 28, 5 }, { 0x18, 4, 2 } };
static const struct rtl8198f_mux rt_uart1_3_mux[] = { { 0xc, 8, 5 }, { 0xc, 4, 5 },
	{ 0xc, 0, 5 }, { 0xc, 12, 5 } };
static const struct rtl8198f_mux rt_uart1_4_mux[] = { { 0xc, 16, 5 }, { 0xc, 28, 5 },
	{ 0xc, 20, 4 }, { 0xc, 12, 5 } };
static const struct rtl8198f_mux rt_uart1_5_mux[] = { { 0xc, 16, 5 }, { 0xc, 28, 5 },
	{ 0xc, 20, 4 }, { 0xc, 24, 5 } };
static const struct rtl8198f_mux rt_uart1_6_mux[] = { { 0x8, 0, 4 }, { 0x8, 16, 4 },
	{ 0x8, 8, 4 }, { 0x8, 12, 4 } };
static const struct rtl8198f_mux rt_uart0_0_mux[] = { { 0x14, 12, 3 }, { 0x14, 0, 2 },
	{ 0x14, 4, 3 }, { 0x14, 8, 3 } };
static const struct rtl8198f_mux rt_uart0_1_mux[] = { { 0xc, 8, 6 }, { 0xc, 4, 6 },
	{ 0xc, 0, 6 }, { 0xc, 12, 6 } };
static const struct rtl8198f_mux rt_uart0_2_mux[] = { { 0x8, 0, 5 }, { 0x8, 16, 5 },
	{ 0x8, 8, 5 }, { 0x8, 12, 5 } };

static const struct rtl8198f_mux vp_mux[] = { { 0x10, 16, 1 }, { 0x10, 12, 1 } };
static const struct rtl8198f_mux xout_0_mux[] = { { 0x4, 4, 5 } };
static const struct rtl8198f_mux xout_1_mux[] = { { 0x14, 24, 4 } };
static const struct rtl8198f_mux xout_2_mux[] = { { 0x14, 20, 5 } };
static const struct rtl8198f_mux xout_3_mux[] = { { 0x14, 16, 5 } };
static const struct rtl8198f_mux xout_4_mux[] = { { 0x0, 12, 3 } };
static const struct rtl8198f_mux xout_5_mux[] = { { 0x8, 4, 4 } };
static const struct rtl8198f_mux xout_6_mux[] = { { 0x8, 16, 7 } };
static const struct rtl8198f_mux xout_7_mux[] = { { 0x20, 0, 1 } };
static const struct rtl8198f_mux xout_8_mux[] = { { 0x20, 12, 5 } };

#define RTL8198F_PIN_GROUP(group_name)			\
{							\
	.name = __stringify(group_name) "_grp",		\
	.pins = group_name ## _pins,			\
	.num_pins = ARRAY_SIZE(group_name ## _pins),	\
	.mux = group_name ## _mux,			\
}

/*
 * List of RTL8198F pin groups
 */
static const struct rtl8198f_pin_group rtl8198f_pin_groups[] = {
	RTL8198F_PIN_GROUP(ca_uart1),
	RTL8198F_PIN_GROUP(ca_ssp),
	RTL8198F_PIN_GROUP(event_0),
	RTL8198F_PIN_GROUP(event_1),
	RTL8198F_PIN_GROUP(event_2),
	RTL8198F_PIN_GROUP(i1pps),
	RTL8198F_PIN_GROUP(o1pps),
	RTL8198F_PIN_GROUP(i2c0_0),
	RTL8198F_PIN_GROUP(i2c0_1),
	RTL8198F_PIN_GROUP(i2c0_2),
	RTL8198F_PIN_GROUP(i2c0_3),
	RTL8198F_PIN_GROUP(i2c0_4),
	RTL8198F_PIN_GROUP(i2c0_5),
	RTL8198F_PIN_GROUP(i2c0_s_0),
	RTL8198F_PIN_GROUP(i2c0_s_1),
	RTL8198F_PIN_GROUP(i2c1_0),
	RTL8198F_PIN_GROUP(i2c1_1),
	RTL8198F_PIN_GROUP(i2c1_2),
	RTL8198F_PIN_GROUP(i2c1_3),
	RTL8198F_PIN_GROUP(i2c1_4),
	RTL8198F_PIN_GROUP(i2c1_5),
	RTL8198F_PIN_GROUP(i2c1_6),
	RTL8198F_PIN_GROUP(i2c1_7),
	RTL8198F_PIN_GROUP(i2c1_s),
	RTL8198F_PIN_GROUP(iis_0),
	RTL8198F_PIN_GROUP(iis_1),
	RTL8198F_PIN_GROUP(iis_2),
	RTL8198F_PIN_GROUP(iis_3),
	RTL8198F_PIN_GROUP(iis_4),
	RTL8198F_PIN_GROUP(iis_5),
	RTL8198F_PIN_GROUP(iis_6),
	RTL8198F_PIN_GROUP(iis_7),
	RTL8198F_PIN_GROUP(iis_8),
	RTL8198F_PIN_GROUP(iis_9),
	RTL8198F_PIN_GROUP(iisa_0),
	RTL8198F_PIN_GROUP(iisa_1),
	RTL8198F_PIN_GROUP(iisa_2),
	RTL8198F_PIN_GROUP(iisa_3),
	RTL8198F_PIN_GROUP(iisa_4),
	RTL8198F_PIN_GROUP(iisv_0),
	RTL8198F_PIN_GROUP(iisv_1),
	RTL8198F_PIN_GROUP(iisv_2),
	RTL8198F_PIN_GROUP(iisv_3),
	RTL8198F_PIN_GROUP(jtag_0),
	RTL8198F_PIN_GROUP(jtag_1),
	RTL8198F_PIN_GROUP(led),
	RTL8198F_PIN_GROUP(led_s),
	RTL8198F_PIN_GROUP(mii),
	RTL8198F_PIN_GROUP(mmc),
	RTL8198F_PIN_GROUP(nand),
	RTL8198F_PIN_GROUP(pcie),
	RTL8198F_PIN_GROUP(pcm_0),
	RTL8198F_PIN_GROUP(pcm_1),
	RTL8198F_PIN_GROUP(pcm_2),
	RTL8198F_PIN_GROUP(pwm_0),
	RTL8198F_PIN_GROUP(pwm_1),
	RTL8198F_PIN_GROUP(pwm_2),
	RTL8198F_PIN_GROUP(pwm_3),
	RTL8198F_PIN_GROUP(pwm_4),
	RTL8198F_PIN_GROUP(spi0_0),
	RTL8198F_PIN_GROUP(spi0_1),
	RTL8198F_PIN_GROUP(spi0_2),
	RTL8198F_PIN_GROUP(spi0_3),
	RTL8198F_PIN_GROUP(spi0_4),
	RTL8198F_PIN_GROUP(spi0_5),
	RTL8198F_PIN_GROUP(spi0_3w_0),
	RTL8198F_PIN_GROUP(spi0_3w_1),
	RTL8198F_PIN_GROUP(spi0_3w_2),
	RTL8198F_PIN_GROUP(spi0_3w_3),
	RTL8198F_PIN_GROUP(spi0_3w_4),
	RTL8198F_PIN_GROUP(spi0_3w_5),
	RTL8198F_PIN_GROUP(spi0_3w_6),
	RTL8198F_PIN_GROUP(spi1_0),
	RTL8198F_PIN_GROUP(spi1_1),
	RTL8198F_PIN_GROUP(spi1_2),
	RTL8198F_PIN_GROUP(spi1_3),
	RTL8198F_PIN_GROUP(spi1_4),
	RTL8198F_PIN_GROUP(spi1_5),
	RTL8198F_PIN_GROUP(spi1_3w_0),
	RTL8198F_PIN_GROUP(spi1_3w_1),
	RTL8198F_PIN_GROUP(spi1_3w_2),
	RTL8198F_PIN_GROUP(spi1_3w_3),
	RTL8198F_PIN_GROUP(spi1_3w_4),
	RTL8198F_PIN_GROUP(spiflsh),
	RTL8198F_PIN_GROUP(ssp_0),
	RTL8198F_PIN_GROUP(ssp_1),
	RTL8198F_PIN_GROUP(ssp_2),
	RTL8198F_PIN_GROUP(sti),
	RTL8198F_PIN_GROUP(trace),
	RTL8198F_PIN_GROUP(uart),
	RTL8198F_PIN_GROUP(rt_uart1_0),
	RTL8198F_PIN_GROUP(rt_uart1_1),
	RTL8198F_PIN_GROUP(rt_uart1_2),
	RTL8198F_PIN_GROUP(rt_uart1_3),
	RTL8198F_PIN_GROUP(rt_uart1_4),
	RTL8198F_PIN_GROUP(rt_uart1_5),
	RTL8198F_PIN_GROUP(rt_uart1_6),
	RTL8198F_PIN_GROUP(rt_uart0_0),
	RTL8198F_PIN_GROUP(rt_uart0_1),
	RTL8198F_PIN_GROUP(rt_uart0_2),
	RTL8198F_PIN_GROUP(vp),
	RTL8198F_PIN_GROUP(xout_0),
	RTL8198F_PIN_GROUP(xout_1),
	RTL8198F_PIN_GROUP(xout_2),
	RTL8198F_PIN_GROUP(xout_3),
	RTL8198F_PIN_GROUP(xout_4),
	RTL8198F_PIN_GROUP(xout_5),
	RTL8198F_PIN_GROUP(xout_6),
	RTL8198F_PIN_GROUP(xout_7),
	RTL8198F_PIN_GROUP(xout_8),
};

/*
 * List of groups supported by functions
 */
static const char * const ca_uart1_grps[] = { "ca_uart1_grp" };
static const char * const ca_ssp_grps[] = { "ca_ssp_grp" };

static const char * const event_grps[] = { "event_0_grp", "event_1_grp",
	"event_2_grp" };

static const char * const i1pps_grps[] = { "i1pps_grp" };
static const char * const o1pps_grps[] = { "o1pps_grp" };
static const char * const i2c0_grps[] = { "i2c0_0_grp", "i2c0_1_grp",
	"i2c0_3_grp", "i2c0_4_grp", "i2c0_5_grp" };
static const char * const i2c0_s_grps[] = { "i2c0_s_0_grp", "i2c0_s_1_grp" };

static const char * const i2c1_grps[] = { "i2c1_0_grp", "i2c1_1_grp",
	"i2c1_2_grp","i2c1_3_grp", "i2c1_4_grp", "i2c1_5_grp",
	"i2c1_6_grp", "i2c1_7_grp" };
static const char * const i2c1_s_grps[] = { "i2c1_s_grp" };

static const char * const iis_grps[] = { "iis_0_grp", "iis_1_grp",
	"ii2_2_grp", "iis_3_grp", "iis_4_grp", "iis_5_grp", "iis_6_grp",
	"iis_7_grp", "iis_8_grp", "iis_9_grp" };
static const char * const iisa_grps[] = { "iisa_0_grp", "iisa_1_grp",
	"iisa_2_grp", "iisa_3_grp", "iisa_4_grp" };
static const char * const iisv_grps[] = { "iisv_0_grp", "iisv_1_grp",
	"iisv_2_grp", "iisv_3_grp" };

static const char * const jtag_grps[] = { "jtag_0_grp", "jtag_1_grp" };

static const char * const led_grps[] = { "led_grp" };
static const char * const led_s_grps[] = { "led_s_grp" };
static const char * const mii_grps[] = { "mii_grp" };
static const char * const mmc_grps[] = { "mmc_grp" };
static const char * const nand_grps[] = { "nand_grp" };
static const char * const pcie_grps[] = { "pcie_grp" };

static const char * const pcm_grps[] = { "pcm_0_grp", "pcm_1_grp", "pcm_2_grp" };
static const char * const pwm_grps[] = { "pwm_0_grp", "pwm_1_grp", "pwm_2_grp",
	"pwm_3_grp", "pwm_4_grp" };

static const char * const spi0_grps[] = { "spi0_0_grp", "spi0_1_grp",
	"spi0_2_grp", "spi0_3_grp", "spi0_4_grp", "spi0_5_grp" };
static const char * const spi0_3w_grps[] = { "spi0_3w_0_grp", "spi0_3w_1_grp",
	"spi0_3w_2_grp", "spi0_3w_3_grp", "spi0_3w_4_grp", "spi0_3w_5_grp",
	"spi0_3w_6_grp" };
static const char * const spi1_grps[] = { "spi1_0_grp", "spi1_1_grp",
	"spi1_2_grp", "spi1_3_grp", "spi1_4_grp", "spi1_5_grp" };
static const char * const spi1_3w_grps[] = { "spi1_3w_0_grp", "spi1_3w_1_grp",
	"spi1_3w_2_grp", "spi1_3w_3_grp", "spi1_3w_4_grp" };

static const char * const spiflsh_grps[] = { "spiflsh_grp" };
static const char * const ssp_grps[] = { "ssp_0_grp", "ssp_1_grp", "ssp_2_grp" };
static const char * const sti_grps[] = { "sti_grp" };
static const char * const trace_grps[] = { "trace_grp" };
static const char * const uart_grps[] = { "uart_grp" };
static const char * const rt_uart1_grps[] = { "rt_uart1_0_grp", "rt_uart1_1_grp",
	"rt_uart1_2_grp", "rt_uart1_3_grp", "rt_uart1_4_grp", "rt_uart1_5_grp",
	"rt_uart1_6_grp" };
static const char * const rt_uart0_grps[] = { "rt_uart0_0_grp", "rt_uart0_1_grp",
	"rt_uart0_2_grp" };

static const char * const vp_grps[] = { "vp_grp" };
static const char * const xout_grps[] = { "xout_0_grp", "xout_1_grp",
	"xout_2_grp", "xout_3_grp", "xout_4_grp", "xout_5_grp", "xout_6_grp",
	"xout_7_grp", "xout_8_grp" };

#define RTL8198F_PIN_FUNCTION(func)				\
{								\
	.name = #func,						\
	.groups = func ## _grps,				\
	.num_groups = ARRAY_SIZE(func ## _grps),		\
}

/*
 * List of supported functions in RTL8198F
 */
static const struct rtl8198f_pin_function rtl8198f_pin_functions[] = {
	RTL8198F_PIN_FUNCTION(ca_uart1),
	RTL8198F_PIN_FUNCTION(ca_ssp),
	RTL8198F_PIN_FUNCTION(i1pps),
	RTL8198F_PIN_FUNCTION(o1pps),
	RTL8198F_PIN_FUNCTION(event),
	RTL8198F_PIN_FUNCTION(i2c0),
	RTL8198F_PIN_FUNCTION(i2c0_s),
	RTL8198F_PIN_FUNCTION(i2c1),
	RTL8198F_PIN_FUNCTION(i2c1_s),
	RTL8198F_PIN_FUNCTION(iis),
	RTL8198F_PIN_FUNCTION(iisa),
	RTL8198F_PIN_FUNCTION(iisv),
	RTL8198F_PIN_FUNCTION(jtag),
	RTL8198F_PIN_FUNCTION(led),
	RTL8198F_PIN_FUNCTION(led_s),
	RTL8198F_PIN_FUNCTION(mii),
	RTL8198F_PIN_FUNCTION(mmc),
	RTL8198F_PIN_FUNCTION(nand),
	RTL8198F_PIN_FUNCTION(pcie),
	RTL8198F_PIN_FUNCTION(pcm),
	RTL8198F_PIN_FUNCTION(pwm),
	RTL8198F_PIN_FUNCTION(spi0_3w),
	RTL8198F_PIN_FUNCTION(spi0),
	RTL8198F_PIN_FUNCTION(spi1_3w),
	RTL8198F_PIN_FUNCTION(spi1),
	RTL8198F_PIN_FUNCTION(spiflsh),
	RTL8198F_PIN_FUNCTION(ssp),
	RTL8198F_PIN_FUNCTION(sti),
	RTL8198F_PIN_FUNCTION(trace),
	RTL8198F_PIN_FUNCTION(rt_uart1),
	RTL8198F_PIN_FUNCTION(rt_uart0),
	RTL8198F_PIN_FUNCTION(uart),
	RTL8198F_PIN_FUNCTION(vp),
	RTL8198F_PIN_FUNCTION(xout),
};

static int rtl8198f_get_groups_count(struct pinctrl_dev *pctrl_dev)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->num_groups;
}

static const char *rtl8198f_get_group_name(struct pinctrl_dev *pctrl_dev,
					 unsigned selector)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->groups[selector].name;
}

static int rtl8198f_get_group_pins(struct pinctrl_dev *pctrl_dev,
				 unsigned selector, const unsigned **pins,
				 unsigned *num_pins)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	*pins = pinctrl->groups[selector].pins;
	*num_pins = pinctrl->groups[selector].num_pins;

	return 0;
}

static void rtl8198f_pin_dbg_show(struct pinctrl_dev *pctrl_dev,
				struct seq_file *s, unsigned offset)
{
	seq_printf(s, " %s", dev_name(pctrl_dev->dev));
}

static const struct pinctrl_ops rtl8198f_pinctrl_ops = {
	.get_groups_count = rtl8198f_get_groups_count,
	.get_group_name = rtl8198f_get_group_name,
	.get_group_pins = rtl8198f_get_group_pins,
	.pin_dbg_show = rtl8198f_pin_dbg_show,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_group,
	.dt_free_map = pinctrl_utils_dt_free_map,
};

static int rtl8198f_get_functions_count(struct pinctrl_dev *pctrl_dev)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->num_functions;
}

static const char *rtl8198f_get_function_name(struct pinctrl_dev *pctrl_dev,
					    unsigned selector)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->functions[selector].name;
}

static int rtl8198f_get_function_groups(struct pinctrl_dev *pctrl_dev,
				      unsigned selector,
				      const char * const **groups,
				      unsigned * const num_groups)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	*groups = pinctrl->functions[selector].groups;
	*num_groups = pinctrl->functions[selector].num_groups;

	return 0;
}

static int rtl8198f_pinmux_set(struct rtl8198f_pinctrl *pinctrl,
			     const struct rtl8198f_pin_function *func,
			     const struct rtl8198f_pin_group *grp,
			     struct rtl8198f_mux_log *mux_log)
{
	const struct rtl8198f_mux *mux = grp->mux;
	const unsigned *pins = grp->pins;
	int i, pin_num;
	u32 val, mask = 0xf;
	unsigned long flags;

	for (i = 0; i < grp->num_pins; i++) {
		pin_num = pins[i];
		if (mux_log[pin_num].is_configured) {
			/* The pin had been configured before */
			dev_err(pinctrl->dev,
				"pin %d double configuration error\n", pin_num);
			return -EINVAL;
		}
		if (mux_log[pin_num].mux.val == mux[i].val) {
			/*
			 * Try to config the same function.
			 * Just skip it.
			 */
			continue;
		}

		mux_log[pin_num].mux.val = mux[i].val;
		mux_log[pin_num].is_configured = true;

		spin_lock_irqsave(&pinctrl->lock, flags);

		val = readl(pinctrl->base + mux[i].offset);
		val &= ~(mask << mux[i].shift);
		val |= mux[i].val << mux[i].shift;
		writel(val, pinctrl->base + mux[i].offset);

		spin_unlock_irqrestore(&pinctrl->lock, flags);
	}

	return 0;
}

static int rtl8198f_pinmux_set_mux(struct pinctrl_dev *pctrl_dev,
				 unsigned func_select, unsigned grp_select)
{
	int i;
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	const struct rtl8198f_pin_function *func =
		&pinctrl->functions[func_select];
	const struct rtl8198f_pin_group *grp = &pinctrl->groups[grp_select];

	dev_dbg(pctrl_dev->dev, "func:%u name:%s grp:%u name:%s\n",
		func_select, func->name, grp_select, grp->name);
	for (i = 0; i < grp->num_pins; i++) {
		dev_dbg(pctrl_dev->dev, "offset:0x%08x shift:%u value:%u\n",
			grp->mux[i].offset, grp->mux[i].shift, grp->mux[i].val);
	}

	return rtl8198f_pinmux_set(pinctrl, func, grp, pinctrl->mux_log);
}

static int rtl8198f_gpio_request_enable(struct pinctrl_dev *pctrl_dev,
					struct pinctrl_gpio_range *range,
					unsigned pin)
{
	struct rtl8198f_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	struct rtl8198f_mux_log *mux_log = pinctrl->mux_log;
	u32 val, mask = 0xf;
	unsigned long flags;

	mux_log[pin].mux.val = rtl8198f_pins[pin].gpio_val;
	mux_log[pin].is_configured = true;

	spin_lock_irqsave(&pinctrl->lock, flags);

	val = readl(pinctrl->base + rtl8198f_pins[pin].mux.offset);
	val &= ~(mask << rtl8198f_pins[pin].mux.shift);
	val |= rtl8198f_pins[pin].gpio_val << rtl8198f_pins[pin].mux.shift;
	writel(val, pinctrl->base + rtl8198f_pins[pin].mux.offset);

	spin_unlock_irqrestore(&pinctrl->lock, flags);

	dev_dbg(pctrl_dev->dev,
		"gpio request enable pin=%u offset=0x%x shift=%u\n",
		pin, rtl8198f_pins[pin].mux.offset,
		rtl8198f_pins[pin].mux.shift);

	return 0;
}

static const struct pinmux_ops rtl8198f_pinmux_ops = {
	.get_functions_count = rtl8198f_get_functions_count,
	.get_function_name = rtl8198f_get_function_name,
	.get_function_groups = rtl8198f_get_function_groups,
	.set_mux = rtl8198f_pinmux_set_mux,
	.gpio_request_enable = rtl8198f_gpio_request_enable,
};

static struct pinctrl_desc rtl8198f_pinctrl_desc = {
	.name = "rtl8198f-pinmux",
	.pctlops = &rtl8198f_pinctrl_ops,
	.pmxops = &rtl8198f_pinmux_ops,
};

static int rtl8198f_mux_log_init(struct rtl8198f_pinctrl *pinctrl)
{
	struct rtl8198f_mux_log *log;
	unsigned int i;

	pinctrl->mux_log = devm_kcalloc(pinctrl->dev, RTL8198F_NUM_IOMUX,
					sizeof(struct rtl8198f_mux_log),
					GFP_KERNEL);
	if (!pinctrl->mux_log)
		return -ENOMEM;

	log = pinctrl->mux_log;
	for (i = 0; i < RTL8198F_NUM_IOMUX; i++) {
		log = &pinctrl->mux_log[i];
		log->mux.offset = rtl8198f_pins[i].mux.offset;
		log->mux.shift = rtl8198f_pins[i].mux.shift;
		log->mux.val = rtl8198f_pins[i].mux.val;
		log->is_configured = false;
	}

	return 0;
}

static int rtl8198f_pinmux_probe(struct platform_device *pdev)
{
	struct rtl8198f_pinctrl *pinctrl;
	struct resource *res;
	int i, ret;
	struct pinctrl_pin_desc *pins;
	unsigned num_pins = ARRAY_SIZE(rtl8198f_pins);

	pinctrl = devm_kzalloc(&pdev->dev, sizeof(*pinctrl), GFP_KERNEL);
	if (!pinctrl)
		return -ENOMEM;

	pinctrl->dev = &pdev->dev;
	platform_set_drvdata(pdev, pinctrl);
	spin_lock_init(&pinctrl->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pinctrl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pinctrl->base)) {
		dev_err(&pdev->dev, "unable to map I/O space\n");
		return PTR_ERR(pinctrl->base);
	}

	ret = rtl8198f_mux_log_init(pinctrl);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize IOMUX log\n");
		return ret;
	}

	pins = devm_kcalloc(&pdev->dev, num_pins, sizeof(*pins), GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < num_pins; i++) {
		pins[i].number = rtl8198f_pins[i].pin;
		pins[i].name = rtl8198f_pins[i].name;
		pins[i].drv_data = &rtl8198f_pins[i].mux;
	}

	pinctrl->groups = rtl8198f_pin_groups;
	pinctrl->num_groups = ARRAY_SIZE(rtl8198f_pin_groups);
	pinctrl->functions = rtl8198f_pin_functions;
	pinctrl->num_functions = ARRAY_SIZE(rtl8198f_pin_functions);
	rtl8198f_pinctrl_desc.pins = pins;
	rtl8198f_pinctrl_desc.npins = num_pins;

	pinctrl->pctl = pinctrl_register(&rtl8198f_pinctrl_desc, &pdev->dev,
			pinctrl);
	if (IS_ERR(pinctrl->pctl)) {
		dev_err(&pdev->dev, "unable to register RTL8198F IOMUX pinctrl\n");
		return PTR_ERR(pinctrl->pctl);
	}

	dev_info(&pdev->dev, "Registered RTL8198F IOMUX pinctrl\n");
	return 0;
}

static const struct of_device_id rtl8198f_pinmux_of_match[] = {
	{ .compatible = "rtk,rtl8198f-pinmux" },
	{ /* sentinel */ }
};

static struct platform_driver rtl8198f_pinmux_driver = {
	.driver = {
		.name = "rtl8198f-pinmux",
		.of_match_table = rtl8198f_pinmux_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = rtl8198f_pinmux_probe,
};

static int __init rtl8198f_pinmux_init(void)
{
	return platform_driver_register(&rtl8198f_pinmux_driver);
}
arch_initcall(rtl8198f_pinmux_init);

MODULE_DESCRIPTION("Realtek RTL8198F IOMUX driver");
MODULE_LICENSE("GPL v2");
