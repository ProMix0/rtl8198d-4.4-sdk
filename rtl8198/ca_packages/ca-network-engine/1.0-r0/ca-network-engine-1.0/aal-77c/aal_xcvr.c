/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "osal_common.h"
#include "aal_pon.h"
#include "aal_xcvr.h"

typedef struct {
    ca_boolean_t support_multi_chn;
    ca_uint32_t  intf_type;
    ca_uint32_t  vendor_id;
    ca_boolean_t need_passwd;
    ca_uint32_t  passwd[4];
    ca_uint32_t  ddm_base_addr;
}pon_xcvr_t;

pon_xcvr_t g_pon_xcvr_cb;

#define __XCVR_DBG(fmt,...)  ca_printf("%s():"fmt, __FUNCTION__,##__VA_ARGS__)

extern ca_status_t cap_i2c_enable_set (
            ca_device_id_t  device_id,
            ca_boolean   enable,
            ca_uint16    speed);

extern ca_status_t  cap_i2c_write (
            ca_device_id_t   device_id,
            ca_uint8      slave_addr,
            ca_uint8      slave_offset,
            ca_uint16     data_len,
            ca_uint8      *p_tx_data);

extern ca_status_t cap_i2c_read(
            ca_device_id_t  device_id,
            ca_uint8_t  slave_addr,
            ca_uint8_t  slave_offset,
            ca_uint16_t  data_len,
            ca_uint8_t * p_rx_data);

/* PON transceiver I2C address */
#define XCVR_I2C_SPEED            100
#define XCVR_I2C_DEV              0xA0
#define XCVR_I2C_DEV_ADV          0xA2
#define XCVR_I2C_PASSWD           {0x12, 0x34, 0x56, 0x78}

#define XCVR_SCC_OFFSET          1

#define XCVR_ID_OFFSET             0
#define XCVR_ID_LEN                1
#define XCVR_EXTID_OFFSET          1
#define XCVR_EXTID_LEN             1
#define XCVR_VENDOR_OFFSET         20
#define XCVR_VENDOR_LEN            AAL_XCVR_VENDOR_STR_LEN
#define XCVR_VENDOR_OUI_OFFSET     37
#define XCVR_VENDOR_OUI_LEN        3
#define XCVR_VENDOR_PN_OFFSET      40
#define XCVR_VENDOR_PN_LEN         AAL_XCVR_VENDOR_STR_LEN
#define XCVR_VENDOR_VER_OFFSET     56
#define XCVR_VENDOR_VER_LEN        4
#define XCVR_TX_WAVE_LEN_OFFSET    60
#define XCVR_TX_WAVE_LEN_LEN       2

#define XCVR_VENDOR_SN_OFFSET      68
#define XCVR_VENDOR_SN_LEN         AAL_XCVR_VENDOR_STR_LEN
#define XCVR_DATA_CODE_OFFSET      84
#define XCVR_DATA_CODE_LEN         8


#define XCVR_PAGE_SEL_OFFSET     127
#define XCVR_PAGE_SEL_LEN        1

/* INF-8077i 
10 Gigabit Small Form Factor Pluggable Module
Revision 4.5 August 31, 2005
Table 00h: Reserved for future diagnostic and control functions
Table 01h: Serial ID EEPROM
Table 02h: User writable EEPROM
Table 03h ¡§C 7Fh:Vendor specific functions
Table 80h ¡§C FFh:Reserved
*/
#define XCVR_INF8077I_TABLE_0_RSVD        0
#define XCVR_INF8077I_TABLE_1_SID         1
#define XCVR_INF8077I_TABLE2_USR_WR       2
#define XCVR_INF8077I_TABLE3_VENDOR_SPEC  3
#define XCVR_INF8077I_TABLE4_VENDOR_SPEC  4

/*
 * XCVR Register Mapping of the
 * SFF-8477 Specification for Tunable XFP for ITU Frequency Grid Applications
 * (Rev 1.4 December 4, 2009)
 */
#define XCVR_SFF8477_TUNING_WAVE_LEN_CTL_OFFSET     72  /* Address72 is MSB, Address 73 is LSB */
#define XCVR_SFF8477_TUNING_WAVE_LEN_CTL_LEN        2
#define XCVR_SFF8477_TUNING_CHANNEL_NUM_CTL_OFFSET  112  /* Address112 is MSB, Address 113 is LSB */
#define XCVR_SFF8477_TUNING_CHANNEL_NUM_CTL_LEN     2
#define XCVR_SFF8477_TUNING_FREQ_ERR_OFFSET         114  /* Address114 is MSB, Address 115 is LSB */
#define XCVR_SFF8477_TUNING_FREQ_ERR_LEN            2

#define XCVR_SFF8477_TUNING_IMPLEMENT_OFFSET                    138 /* The XFP vendor can implement wavelength only tuning (as specified in INF-8077i),frequency only tuning, or both*/
#define XCVR_SFF8477_TUNING_IMPLEMENT_LEN                       1
#define XCVR_SFF8477_TUNING_IMPLEMENT_SEL_CHANNEL_NUM_BIT       3
#define XCVR_SFF8477_TUNING_IMPLEMENT_SEL_50PM_STEP_BIT         2
#define XCVR_SFF8477_TUNING_SELECTION_OFFSET                    221  /* Wavelength or frequency tunability implemented,bit1 */
#define XCVR_SFF8477_TUNING_SELECTION_LEN                       1
#define XCVR_SFF8477_TUNING_SELECTION_BIT                       1

/*
 * Hisense NGPON2 ONU XCVR Tuning related Reisters 
 *
 */
#define XCVR_I2C_REG_TX_CHN_CTRL  112
#define XCVR_I2C_REG_RX_CHN_CTRL  113
#define XCVR_I2C_REG_PASSWD_CHG   119
#define XCVR_I2C_REG_PASSWD_ENTRY 123
#define XCVR_I2C_REG_PAGE_SEL     127

typedef enum {
    INTF_UNK  = 0, // Unknown or unspecified
    INTF_GPIC = 1, // GBIC
    INTF_SOLID = 2, // Module/connector soldered to motherboard
    INTF_SFP = 3, // SFP
    INTF_XBI = 4, // 300 pin XBI
    INTF_XENPAK = 5, // XENPAK
    INTF_XFP = 6, // XFP
    INTF_XFF = 7, // XFF
    INTF_XFPE= 8, // XFP-E
    INTF_XPAK = 9, // XPAK
    INTF_X2 = 0xA // X2
} xcvr_intf_type_t;

char * _xcvr_intf_str_get(xcvr_intf_type_t type)
{
    static char *intf_str[] = {
        "Unknown or unspecified",
        "GBIC",
        "Module/connector soldered to motherboard",
        "SFP",
        "300 pin XBI",
        "XENPAK",
        "XFP",
        "XFF",
        "XFP-E",
        "XPAK",
        "X2"};

    if (type <= INTF_X2)
        return intf_str[type];

    return intf_str[0];
}

static ca_status_t __xcvr_page_select(ca_device_id_t device_id, ca_uint8_t page)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint8_t  data = page;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV, XCVR_PAGE_SEL_OFFSET,
        XCVR_PAGE_SEL_LEN, &data);

    return ret;
}

static ca_status_t __xcvr_tuning_implement_get(ca_device_id_t device_id, ca_boolean_t *support_multi_wave)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint8_t   data = 0;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV, XCVR_SFF8477_TUNING_SELECTION_OFFSET,
        XCVR_SFF8477_TUNING_SELECTION_LEN, &data);
    if (CA_E_OK == ret)
        *support_multi_wave = (data >> XCVR_SFF8477_TUNING_SELECTION_BIT) & 0x01;
    else
        *support_multi_wave = 0;

    return ret;
}


ca_status_t aal_xcvr_vendor_info_get(
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  aal_xcvr_vendor_info_t *vendor_info)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint8_t   data[AAL_XCVR_VENDOR_STR_LEN] = {0};
    
    CA_NULL_PTR_CHECK(vendor_info);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_VENDOR_OFFSET,
        AAL_XCVR_VENDOR_STR_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->name, data, AAL_XCVR_VENDOR_STR_LEN);
    }
    memset(data, 0, AAL_XCVR_VENDOR_STR_LEN);
    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_VENDOR_OUI_OFFSET,
        XCVR_VENDOR_OUI_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->oui, data, XCVR_VENDOR_OUI_LEN);
    }
    memset(data, 0, AAL_XCVR_VENDOR_STR_LEN);
    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_VENDOR_PN_OFFSET,
        XCVR_VENDOR_PN_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->pn, data, XCVR_VENDOR_PN_LEN);
    }
    memset(data, 0, AAL_XCVR_VENDOR_STR_LEN);
    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_VENDOR_VER_OFFSET,
        XCVR_VENDOR_VER_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->revesion, data, XCVR_VENDOR_VER_LEN);
    }
    memset(data, 0, AAL_XCVR_VENDOR_STR_LEN);
    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_VENDOR_SN_OFFSET,
        XCVR_VENDOR_SN_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->sn, data, XCVR_VENDOR_VER_LEN);
    }

    memset(data, 0, AAL_XCVR_VENDOR_STR_LEN);
    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_DATA_CODE_OFFSET,
        XCVR_DATA_CODE_LEN, &data[0]);
    if (ret == CA_E_OK) {
        memcpy(vendor_info->date, data, XCVR_DATA_CODE_LEN);
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_vendor_info_get);

ca_status_t aal_xcvr_type_get(
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  xcvr_intf_type_t *type)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint8_t   data[2] = {0};

    CA_NULL_PTR_CHECK(type);

    ret = cap_i2c_read(0, XCVR_I2C_DEV, XCVR_ID_OFFSET,
        1, &data[0]);

    if (ret == CA_E_OK) {
        *type = data[0];
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_type_get);

ca_status_t aal_xcvr_tx_wave_len_get(
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  ca_uint16_t    *wave_len)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint8_t   data[2] = {0};

    CA_NULL_PTR_CHECK(wave_len);

    ret = cap_i2c_read(0, XCVR_I2C_DEV, XCVR_TX_WAVE_LEN_OFFSET,
        XCVR_TX_WAVE_LEN_LEN, &data[0]);

    if (ret == CA_E_OK) {
        *wave_len = ((ca_uint16_t)data[0] << 8) | data[1];
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_tx_wave_len_get);


static ca_status_t __xcvr_i2c_init(ca_device_id_t device_id)
{
#ifdef CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN
        /*Select one of the two I2C ports.  0: select i2c port 0, 1: select i2c port 1*/
        AAL_PON_REG_FIELD_WRITE(TITANGLB_CTL, i2c, 1);
#endif
    /*Platform will do i2c init*/
    return  CA_E_OK;
}

static ca_status_t __xcvr_passwd_init(ca_device_id_t device_id)
{
    ca_status_t ret     = CA_E_OK;
    ca_uint8_t psswd[] = XCVR_I2C_PASSWD;

    // TODO: for different XFP, need different driver
    /* Write PASSWD for access */
    ret = cap_i2c_write(0, XCVR_I2C_DEV, XCVR_I2C_REG_PASSWD_ENTRY,
        sizeof(psswd), &psswd[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write fail %d.\r\n", ret);
        return ret;
    }

    return ret;
}

static ca_status_t __xcvr_detect(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    xcvr_intf_type_t  xcvr_type  = INTF_UNK;
    aal_xcvr_vendor_info_t  xcvr_vendor;
    ca_uint16_t tx_wave = 0;
    
    ret = aal_xcvr_type_get(device_id, &xcvr_type);
    if (CA_E_OK != ret) {
        ca_printf("ca-pon: aal_xcvr_type_get fail, return %d, please confirm PON XCVR is plugged in.\r\n", ret);
        return ret;
    } else {
        ca_printf("ca-pon: %s PON XCVR was detected\r\n", _xcvr_intf_str_get(xcvr_type));
    }

    memset(&xcvr_vendor, 0, sizeof(xcvr_vendor));
    ret = aal_xcvr_vendor_info_get(device_id, &xcvr_vendor);
    if (CA_E_OK == ret) {
            xcvr_vendor.name[15] = 0; /* string end, prevent printing overflow */
            xcvr_vendor.pn[15] = 0;
            xcvr_vendor.revesion[3] = 0;
            xcvr_vendor.sn[15] = 0;
            xcvr_vendor.date[7] = 0;
        ca_printf("ca-pon: PON XCVR Vendor:%s, PartNum:%s, Reversion:%s, SN:%s, Date:%s\r\n", 
            xcvr_vendor.name, xcvr_vendor.pn, xcvr_vendor.revesion, xcvr_vendor.sn, (char *)xcvr_vendor.date);
    }

    ret = aal_xcvr_tx_wave_len_get(device_id, &tx_wave);
    if (CA_E_OK == ret)
        ca_printf("ca-pon: PON XCVR Tx wave length: %u nm\r\n", tx_wave);

    if (xcvr_type >= INTF_XFP && xcvr_type <= INTF_XFPE) {
        ret = __xcvr_passwd_init(device_id);
        g_pon_xcvr_cb.support_multi_chn = 1;
    }

    return ret;
}


/**************************************************************************/
/*                      Export AAL Driver APIs                            */
/**************************************************************************/

ca_status_t aal_xcvr_capbility_get(
    ca_device_id_t      device_id,
    aal_xcvr_cap_t  *cap)
{
    ca_status_t ret     = CA_E_OK;
    ca_boolean_t  tunable = 0;

    AAL_ASSERT_RET(cap != NULL, CA_E_PARAM);

    if (!g_pon_xcvr_cb.support_multi_chn) {
        cap->rx_chan_num = 1;
        cap->tx_chan_num = 1;
        return CA_E_OK;
    }

    ret = __xcvr_page_select(device_id, XCVR_INF8077I_TABLE_1_SID);
    if (CA_E_OK != ret)
        return ret;

    ret = __xcvr_tuning_implement_get(device_id, &tunable);
    if (CA_E_OK != ret)
        return ret;

    if (!tunable) {
        cap->rx_chan_num = 1;
        cap->tx_chan_num = 1;
        return CA_E_OK;
    }

    cap->rx_chan_num = 4; // TODO:  different XCVR to get 
    cap->tx_chan_num = 4;

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_capbility_get);

ca_status_t aal_xcvr_tx_channle_set(
    ca_device_id_t device_id,
    ca_uint8_t  chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data = chan;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_TX_CHN_CTRL,
        1, &data);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg fail.\r\n");
        return ret;
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_tx_channle_set);

ca_status_t aal_xcvr_tx_channle_get(
    ca_device_id_t device_id,
    ca_uint8_t  *chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data = 0;

    AAL_ASSERT_RET(chan, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_TX_CHN_CTRL,
        1, &data);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg fail.\r\n");
        return ret;
    }

    *chan = data;

    return ret;

}
EXPORT_SYMBOL(aal_xcvr_tx_channle_get);


ca_status_t aal_xcvr_rx_channle_set(
    ca_device_id_t device_id,
    ca_uint8_t  chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data = chan;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_RX_CHN_CTRL,
        1, &data);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg fail.\r\n");
        return ret;
    }

    return ret;

}
EXPORT_SYMBOL(aal_xcvr_rx_channle_set);

ca_status_t aal_xcvr_rx_channle_get(
    ca_device_id_t device_id,
    ca_uint8_t  *chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data = 0;

    AAL_ASSERT_RET(chan, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_RX_CHN_CTRL,
        1, &data);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg fail.\r\n");
        return ret;
    }

    *chan = data;

    return ret;

}
EXPORT_SYMBOL(aal_xcvr_rx_channle_get);


ca_status_t aal_xcvr_tx_rx_channle_mset(
    ca_device_id_t device_id,
    ca_uint8_t  tx_chan,
    ca_uint8_t  rx_chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    data[0] = tx_chan;
    data[1] = rx_chan;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_TX_CHN_CTRL,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg 0x70 fail.\r\n");
        return ret;
    }

    return ret;

}
EXPORT_SYMBOL(aal_xcvr_tx_rx_channle_mset);


ca_status_t aal_xcvr_tx_rx_channle_mget(
    ca_device_id_t  device_id,
    ca_uint8_t  *tx_chan,
    ca_uint8_t  *rx_chan)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    AAL_ASSERT_RET(tx_chan && rx_chan, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV, XCVR_I2C_REG_TX_CHN_CTRL,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg 0x70 fail.\r\n");
        return ret;
    }

    *tx_chan = data[0];
    *rx_chan = data[1];

    return ret;

}
EXPORT_SYMBOL(aal_xcvr_tx_rx_channle_mget);

ca_status_t aal_xcvr_tx_power_get(
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *tx_power)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    AAL_ASSERT_RET(tx_power, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV_ADV, 102,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg 102 fail.\r\n");
        return ret;
    }

    *tx_power = ((ca_uint16_t)data[0] << 8) + data[1];

    return ret;
}

EXPORT_SYMBOL(aal_xcvr_tx_power_get);

ca_status_t aal_xcvr_rx_power_get(
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *rx_power)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    AAL_ASSERT_RET(rx_power, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV_ADV, 104,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg 104 fail.\r\n");
        return ret;
    }

    *rx_power = ((ca_uint16_t)data[0] << 8) + data[1];

    return ret;
}

EXPORT_SYMBOL(aal_xcvr_rx_power_get);

#if 0
ca_status_t aal_xcvr_tx_wave_len_get(
    CA_IN    ca_device_id_t  device_id,
    CA_OUT   ca_uint16_t *wave_len)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    AAL_ASSERT_RET(wave_len, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV_ADV, 72,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_read reg 102 fail.\r\n");
        return ret;
    }

    *wave_len = ((ca_uint16_t)data[0] << 8) + data[1];

    return ret;
}

EXPORT_SYMBOL(aal_xcvr_tx_wave_len_get);
#endif

ca_status_t aal_xcvr_tx_wave_len_set(
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_uint16_t     wave_len)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    if (!g_pon_xcvr_cb.support_multi_chn) {
        return CA_E_NOT_SUPPORT;
    }

    data[0] = (wave_len >> 8) & 0xFF;
    data[1] = wave_len & 0xFF;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV_ADV, 72,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg 72 fail.\r\n");
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_tx_wave_len_set);


ca_status_t aal_xcvr_freq_chn_set(
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_uint16_t  freq_chn)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    data[0] = (freq_chn >> 8) & 0xFF;
    data[1] = freq_chn & 0xFF;

    ret = cap_i2c_write(device_id, XCVR_I2C_DEV_ADV, 112,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg 112 fail.\r\n");
        return ret;
    }

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_freq_chn_set);

ca_status_t aal_xcvr_freq_chn_get(
    CA_IN   ca_device_id_t  device_id,
    CA_OUT  ca_uint16_t *freq_chn)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[2] = {0};

    AAL_ASSERT_RET(freq_chn, CA_E_PARAM);

    ret = cap_i2c_read(device_id, XCVR_I2C_DEV_ADV, 112,
        2, &data[0]);
    if (CA_E_OK != ret) {
        __XCVR_DBG("cap_i2c_write reg 112 fail.\r\n");
        return ret;
    }

    *freq_chn = ((ca_uint16_t)data[0] << 8) + data[1];

    return ret;
}
EXPORT_SYMBOL(aal_xcvr_freq_chn_get);


ca_status_t aal_xcvr_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    memset(&g_pon_xcvr_cb, 0, sizeof(g_pon_xcvr_cb));
    
    ret = __xcvr_i2c_init(device_id);
    if (CA_E_OK != ret) {
        __XCVR_DBG("__xcvr_i2c_init fail %d.\r\n", ret);
        return ret;
    }

    /* Write PASSWD for access */
    ret = __xcvr_detect(device_id);
    if (CA_E_OK != ret) {
        __XCVR_DBG("aal_xcvr_detect fail %d.\r\n", ret);
        return ret;
    }


    return ret;
}
EXPORT_SYMBOL(aal_xcvr_init);

