Please follow the below steps to setup Port 3 (ni_3) as WAN port
1. Change scfg settings to the below
	CHAR           CFG_ID_WAN_PORT_ID                              = 0x03;
	CHAR-ARRAY      CFG_ID_NE_INTF_PORTS0				= {0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	CHAR-ARRAY      CFG_ID_NE_INTF_PORTS1				= {0x0, 0x01, 0x02, 0x06, 0xFF, 0xFF, 0xFF, 0xFF};
	INT-ARRAY      CFG_ID_FLOODING_DOMAIN_1                        = {0x0, 0x01, 0x02, 0x6, 0x11, 0xFF, 0xFF, 0xFF}
	CHAR-ARRAY	CFG_ID_NE_TX_LDPID				        = {0x03, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
2. Run cli_port3_as_wan.ttl in app-cli
3. Run iros_por3_as_wan.tcl in iros
4. Run your scripts to setup LAN-to-WAN forwarding
