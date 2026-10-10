#ifndef NOT_SCFG_H_A2D1ECC7_2D11_4F3A_B82E_0AFF2AC6ABC9_
#define NOT_SCFG_H_A2D1ECC7_2D11_4F3A_B82E_0AFF2AC6ABC9_

typedef enum {
    SCFG_GPIO_INVALID   = 0,
    SCFG_GPIO_INPUT     = 1,
    SCFG_GPIO_OUTPUT    = 2,
    SCFG_GPIO_TRIGGER   = 3,
} ca_scfg_GpioPinType_t;

typedef enum {
    ONU_PON_MAC_EPON_1G   = 0,   /* 1G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_D10G = 1,   /* 10G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_BI10G= 2,   /* 10G Down, 10G Up, 10G-EPON */
    ONU_PON_MAC_GPON      = 3,   /* 2.5G Down, 1G Up, GPON */
    ONU_PON_MAC_XGPON1    = 4,   /* 10G Down, 2.5G Up, XG-PON1 */
    ONU_PON_MAC_XGSPON    = 5,   /* 10G Down, 10G Up, XGS-PON */
    ONU_PON_MAC_NGPON2    = 6,   /* 10G Down, 10G Up, NG-PON2 with multiple wavelength support*/
    ONU_PON_MAC_ETHERNET  = 7,   /* 10G Ethernet */
    ONU_PON_MAC_TURBO_EPON = 8,   /* 2G Down, 1G UP, EPON */
    ONU_PON_MAC_MAX
} ca_scfg_pon_mac_mode_t;

#define CA_PON_MODE_IS_EPON_FAMILY(mode) (((mode) >= ONU_PON_MAC_EPON_1G && (mode) <= ONU_PON_MAC_EPON_BI10G) || ((mode) == ONU_PON_MAC_TURBO_EPON))
#define CA_PON_MODE_IS_GPON_FAMILY(mode) ((mode) >= ONU_PON_MAC_GPON && (mode) <= ONU_PON_MAC_NGPON2)
#define CA_PON_MODE_IS_ETHERNET(mode) ((mode) == ONU_PON_MAC_ETHERNET )

typedef enum {
    CUSTOMER_BIT_0   = 0x1,   /* DPOE  */
    CUSTOMER_BIT_1   = 0x2,   /* K  */
    CUSTOMER_BIT_2   = 0x4,   /* G C */
    CUSTOMER_BIT_12   = 0x1000,/* Minimal SFP + */
    CUSTOMER_BIT_13   = 0x2000,/* SFU */
    CUSTOMER_BIT_14   = 0x4000,/* HGU */
    CUSTOMER_BIT_16   = 0x10000,   /* U special OAM*/
    CUSTOMER_BIT_17   = 0x20000,   /*  VOIP   */
    CUSTOMER_BIT_18   = 0x40000,   /*  Iperf Offload  */
    CUSTOMER_BIT_ALL   = 0xFFFFFFFF
} ca_scfg_customer_bit_t;

typedef enum {
    BOARD_INFO_BIT_0   = 0x1,   /*SFU/SFP/HGU Engineer Board, Distinguished by Macro */
    BOARD_INFO_BIT_1   = 0x2,   /*C : SFU */
    BOARD_INFO_BIT_2   = 0x4,   /*C : SFP+ */
    BOARD_INFO_BIT_3   = 0x8,   /*U : K  */
    BOARD_INFO_BIT_4   = 0x10,  /*U : DPOE */
    BOARD_INFO_BIT_5   = 0x20,  /*H : K SFP  */
    BOARD_INFO_BIT_6   = 0x40,  /*P : SFP  */
} ca_scfg_board_info_bit_t;

typedef enum
{
    CA_OLT_TYPE_CALIX           = 0,
    CA_OLT_TYPE_ADTRAN          = 1,
    CA_OLT_TYPE_ZTE             = 2,
    CA_OLT_TYPE_SUMITOMO        = 3,
    CA_OLT_TYPE_CORTINA         = 4,
    CA_OLT_TYPE_UNKNOWN         = 5
}ca_scfg_olt_type_t;


#endif
