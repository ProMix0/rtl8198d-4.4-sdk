#include <linux/limits.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "ca_serdes.h"
#include "serdes_file.h"
#include "ca_mut.h"

static const char *get_asic_name(void)
{

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
	return "sfpplus";
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return "saturn";
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	return "hgu";
#endif

#if defined(CONFIG_ARCH_CORTINA_G3)
	return "g3";
#endif

#if defined(CONFIG_AARCH_CORTINA_VENUS)
	return "venus";
#endif

#if defined(CONFIG_ARCH_CORTINA_G3LITE)
	return "g3lite";
#endif

	return "";
}

struct Serdes_Mode_Name_Pair {
	enum Serdes_Mode mode;
	const char *name;
};

static struct Serdes_Mode_Name_Pair mode_name_pairs[] = {
	{epon_serdes_mode, "epon"},
	{gpon_serdes_mode, "gpon"},
	{active_ethernet_serdes_mode, "activee"},
	{ethernet_serdes_mode, "ethernet"},
};

static const char *lookup_mode_name(enum Serdes_Mode mode)
{
	struct Serdes_Mode_Name_Pair *first, *last;

	first = mode_name_pairs;
	last = mode_name_pairs + ARRAY_SIZE(mode_name_pairs);

	for (; first != last; ++first) {
		if (first->mode == mode) {
			return first->name;
		}
	}

	return NULL;
}

static char *get_serdes_filename(enum Serdes_Mode mode,
				       const char *template)
{
	char *filename;
	const char *asic;
	const char *mode_name;

	asic = get_asic_name();
	mode_name = lookup_mode_name(mode);
	if (asic == NULL || mode_name == NULL) {
		return NULL;
	}

	filename = ca_malloc_trace(sizeof(char) * PATH_MAX, GFP_KERNEL, true);
	if (filename == NULL) {
		return NULL;
	}

	sprintf(filename, template, asic, mode_name);
	return filename;
}

struct PON_Speed {
	aal_psds_mode_t mode;
	const char *rx_speed;
	const char *tx_speed;
};

static struct PON_Speed pon_speeds[] = {
	{AAL_PSDS_MODE_XGEPON_SYM, "10.3125", "10.3125"},
	{AAL_PSDS_MODE_XGEPON_ASYM, "10.3125", "1.25"},
	{AAL_PSDS_MODE_TUBO_EPON, "2.5", "1.25"},
	{AAL_PSDS_MODE_EPON, "1.25", "1.25"},
	{AAL_PSDS_MODE_XGPON_SYM_25M, "9.95328", "9.95328"},
	{AAL_PSDS_MODE_XGPON_ASYM_25M, "9.95328", "2.48832"},
	{AAL_PSDS_MODE_GPON, "2.48832", "1.24416"},
	{AAL_PSDS_MODE_ETHERNET, "10.3125", "10.3125"},
};

static struct PON_Speed *lookup_pon_speed(aal_psds_mode_t pon_mode)
{
	struct PON_Speed *first, *last;

	first = pon_speeds;
	last = pon_speeds + ARRAY_SIZE(pon_speeds);

	for (; first != last; ++first) {
		if (first->mode == pon_mode) {
			return first;
		}
	}

	return NULL;
}

enum Serdes_Index {
	PON_SERDES_INDEX = 0,
	XFI_SERDES_INDEX = 1,
};

static struct Serdes *load_serdes_by_index(enum Serdes_Mode mode, int index)
{
	int i;
	const char *filename;
	struct Serdes *serdes;
	const char *templates[] = {
		"/config/serdes/serdes_%s_%s.txt",
		"/config/serdes/serdes_default_%s_%s.txt"
	};

	for (i = 0; i < ARRAY_SIZE(templates); ++i) {
		filename = get_serdes_filename(mode, templates[i]);
		if (filename == NULL) {
			return NULL;
		}

		serdes = parse_serdes_file(filename, index);
		ca_free_trace(filename);

		if (serdes != NULL) {
			//printk("Read %s successfully\n", filename);
			return serdes;
		}
	}

	return NULL;
}

static int get_serdex_index_in_file(enum Serdes_Mode mode,
				    enum Serdes_Index serdes_index)
{
	if (serdes_index == PON_SERDES_INDEX) {
		return 0;
	}

	if (serdes_index == XFI_SERDES_INDEX) {
		if (mode == ethernet_serdes_mode) {
			return 0;
		}
		return 1;
	}

	return -1;
}

static struct Serdes *load_serdes(enum Serdes_Mode mode,
				  enum Serdes_Index serdes_index,
				  const char *header1, const char *header2)
{
	int index;
	struct Serdes *serdes;

	index = get_serdex_index_in_file(mode, serdes_index);
	if (index < 0) {
		return NULL;
	}

	serdes = load_serdes_by_index(mode, index);
	if (serdes == NULL) {
		printk("parse serdes failed\n");
		return NULL;
	}

	serdes->value_index = lookup_column_index(serdes, header1, header2);
	if (serdes->value_index > 0) {
		return serdes;
	}

	printk("No such data (%s, %s)\n", header1, header2);
	show_serdes_headers(serdes);
	free_serdes(serdes);

	return NULL;
}

struct Serdes *load_pon_serdes(enum Serdes_Mode mode, aal_psds_mode_t pon_mode)
{
	struct PON_Speed *speed;

	speed = lookup_pon_speed(pon_mode);
	if (speed == NULL) {
		printk("mode(%d) is not found\n", pon_mode);
		return NULL;
	}

	return load_serdes(mode, PON_SERDES_INDEX, speed->rx_speed,
			   speed->tx_speed);
}

struct XFI_speed {
	ca_scfg_eth_serdes_speed_t speed;
	const char *speed_value;
};

static struct XFI_speed xfi_speeds[] = {
	{ONU_ETH_SERDES_SPEED_10DOT3125G, "10.3125"},
	{ONU_ETH_SERDES_SPEED_3DOT125G, "3.125"},
	{ONU_ETH_SERDES_SPEED_2DOT5G, "2.5"},
	{ONU_ETH_SERDES_SPEED_1DOT25G, "1.25"},
	{ONU_ETH_SERDES_SPEED_DISABLE, ""},
};

static const char *lookup_xfi_speed_value(ca_scfg_eth_serdes_speed_t speed)
{
	struct XFI_speed *first, *last;

	first = xfi_speeds;
	last = xfi_speeds + ARRAY_SIZE(xfi_speeds);

	for (; first != last; ++first) {
		if (first->speed == speed) {
			return first->speed_value;
		}
	}

	return NULL;
}

struct Serdes *load_xfi_serdes(enum Serdes_Mode mode,
			       ca_scfg_eth_serdes_speed_t xfi0,
			       ca_scfg_eth_serdes_speed_t xfi1)
{
	const char *xfi0_speed, *xfi1_speed;

	xfi0_speed = lookup_xfi_speed_value(xfi0);
	if (xfi0_speed == NULL) {
		return NULL;
	}

	xfi1_speed = lookup_xfi_speed_value(xfi1);
	if (xfi1_speed == NULL) {
		return NULL;
	}

	return load_serdes(mode, XFI_SERDES_INDEX, xfi0_speed, xfi1_speed);
}

static void show_serdes(struct Serdes *serdes)
{
	int i, readback_idx, delay_idx;
	uint32_t *column;

	readback_idx = serdes->column_num;
	delay_idx = serdes->column_num + 1;

	column = serdes->cells;
	for (i = 0; i < serdes->row_num; ++i) {
		printk("0x%08X 0x%08X 0x%08X 0x%08X\n",
		       column[0],
		       column[serdes->value_index],
		       column[readback_idx], column[delay_idx]);

		column += serdes->column_num + 2;
	}
}

int show_pon_serdes(enum Serdes_Mode mode, aal_psds_mode_t pon_mode)
{
	struct Serdes *serdes;

	serdes = load_pon_serdes(mode, pon_mode);
	if (serdes == NULL) {
		return -1;
	}

	show_serdes(serdes);

	free_serdes(serdes);
	return 0;
}

int show_xfi_serdes(enum Serdes_Mode mode,
		    ca_scfg_eth_serdes_speed_t xfi0,
		    ca_scfg_eth_serdes_speed_t xfi1)
{
	struct Serdes *serdes;

	serdes = load_xfi_serdes(mode, xfi0, xfi1);
	if (serdes == NULL) {
		return -1;
	}

	show_serdes(serdes);

	free_serdes(serdes);
	return 0;
}
