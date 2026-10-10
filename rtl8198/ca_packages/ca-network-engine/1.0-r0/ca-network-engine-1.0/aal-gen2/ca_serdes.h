#ifndef _CA_SERDES_H_9A86E122_8bd1_4bd0_a048_1c6bcc26f364
#define _CA_SERDES_H_9A86E122_8bd1_4bd0_a048_1c6bcc26f364

#include "aal_psds.h"

// This may be should be somewhre in ca-network-engine
typedef enum {
	ONU_ETH_SERDES_SPEED_INVALID = 0,	/* Off */
	ONU_ETH_SERDES_SPEED_1DOT25G = 1,	/* 1.25G bps */
	ONU_ETH_SERDES_SPEED_2DOT5G = 2,	/* 2.5G bps */
	ONU_ETH_SERDES_SPEED_3DOT125G = 3,	/* 3.125G bps */
	ONU_ETH_SERDES_SPEED_10DOT3125G = 4,	/* 10.3125G bps */
	ONU_ETH_SERDES_SPEED_DISABLE,
	ONU_ETH_SERDES_SPEED_MAX = ONU_ETH_SERDES_SPEED_10DOT3125G
} ca_scfg_eth_serdes_speed_t;

enum Serdes_Mode {
	epon_serdes_mode,
	gpon_serdes_mode,
	active_ethernet_serdes_mode,
	ethernet_serdes_mode,
};

struct Serdes {
	const char *version;
	char **header;
	uint32_t *cells;
	int column_num;
	int row_num;
	int value_index;
};

struct Serdes *load_pon_serdes(enum Serdes_Mode mode, aal_psds_mode_t pon_mode);
struct Serdes *load_xfi_serdes(enum Serdes_Mode mode,
			       ca_scfg_eth_serdes_speed_t xfi0,
			       ca_scfg_eth_serdes_speed_t xfi1);

void free_serdes(struct Serdes *serdes);

int show_pon_serdes(enum Serdes_Mode mode, aal_psds_mode_t pon_mode);

int show_xfi_serdes(enum Serdes_Mode mode,
		    ca_scfg_eth_serdes_speed_t xfi0,
		    ca_scfg_eth_serdes_speed_t xfi1);

#endif
