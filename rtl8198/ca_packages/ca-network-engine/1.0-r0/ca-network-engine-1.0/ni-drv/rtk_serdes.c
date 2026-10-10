#include "ca_aal_proc.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_ni_l2.h"
#include <linux/sysfs.h>
#include <dt-bindings/soc/cortina_irqs.h>
#include "rtk_serdes.h"
#include "linux/timer.h"
#include <linux/string.h>

extern ca_status_t aal_mdio_read_indirect (
	CA_IN       ca_device_id_t             device_id,
	CA_IN       ca_uint8_t             phy_port_addr,
	CA_IN       ca_uint8_t             dev_type,
	CA_IN       ca_uint16_t             addr,
	CA_OUT      ca_uint16_t             *data);

extern ca_status_t aal_mdio_write_indirect (
	CA_IN       ca_device_id_t             device_id,
	CA_IN       ca_uint8_t             phy_port_addr,
	CA_IN       ca_uint8_t             dev_addr,
	CA_IN       ca_uint16_t             addr,
	CA_IN       ca_uint16_t             data);

bool mmdPhyRead(
	CA_IN  ca_uint8_t hDevice,
	CA_IN  ca_uint16_t dev,
	CA_IN  ca_uint16_t addr,
	CA_OUT ca_uint16_t *data
	)
{
	int ret;
	ret = aal_mdio_read_indirect(0, hDevice, dev, addr, data);
	if (ret)
		return FALSE;
	return TRUE;
}

bool mmdPhyWrite(
	CA_IN ca_uint8_t hDevice,
	CA_IN ca_uint16_t dev,
	CA_IN ca_uint16_t addr,
	CA_IN ca_uint16_t data
	)
{
	int ret;
	ret = aal_mdio_write_indirect(0, hDevice, dev, addr, data);
	if (ret)
		return FALSE;
	return TRUE;
}

#if defined(CONFIG_RTL_8226_SUPPORT)

bool
rtl8226_autoNegoAbility_set(
    CA_IN ca_uint8_t hDevice,
    CA_IN PHY_LINK_ABILITY *pPhyAbility
    )
{
	bool status = FAILURE;
	ca_uint16_t phydata = 0;

	status = mmdPhyRead(hDevice, MMD_AN, 16, &phydata);
	if (status != SUCCESS)
		goto exit;

	phydata &= (~(BIT_5 | BIT_6 | BIT_7 | BIT_8));

	/* 10M */
	phydata |= (pPhyAbility->Half_10 ? (BIT_5) : (0));
	phydata |= (pPhyAbility->Full_10 ? (BIT_6) : (0));

	/* 100M */
	phydata |= (pPhyAbility->Half_100 ? (BIT_7) : (0));
	phydata |= (pPhyAbility->Full_100 ? (BIT_8) : (0));

	phydata |= (pPhyAbility->FC ? (BIT_10) : (0));
	phydata |= (pPhyAbility->AsyFC ? (BIT_11) : (0));

	status = mmdPhyWrite(hDevice, MMD_AN, 16, phydata);
	if (status != SUCCESS)
		goto exit;

	/* 1G */
	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA412, &phydata);
	if (status != SUCCESS)
		goto exit;

	phydata &= (~BIT_9);

	phydata |= (pPhyAbility->Full_1000 ? (BIT_9) : (0));

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA412, phydata);
	if (status != SUCCESS)
		goto exit;

	/* 2.5G */
	status = mmdPhyRead(hDevice, MMD_AN, 32, &phydata);
	if (status != SUCCESS)
		goto exit;

	phydata &= (~BIT_7);

	phydata |= (pPhyAbility->adv_2_5G ? (BIT_7) : (0));
	status = mmdPhyWrite(hDevice, MMD_AN, 32, phydata);
	if (status != SUCCESS)
		goto exit;

exit:
	return status;
}

bool
rtl8226_gigaLiteEnable_set(
	CA_IN ca_uint8_t hDevice,
	CA_IN bool Enable
	)
{
	bool status = FAILURE;
	ca_uint16_t phydata0 = 0, phydata1 = 0;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA442, &phydata0);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA428, &phydata1);
	if (!status)
	goto exit;

	if (Enable) {
		phydata0 |= (BIT_2 | BIT_9);
		phydata1 |= BIT_9;
	} else {
		phydata0 &= (~(BIT_2 | BIT_9));
		phydata1 &= (~BIT_9);
	}

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA442, phydata0);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA428, phydata1);
	if (status != SUCCESS)
		goto exit;

exit:
	return status;
}

bool
rtl8226_2p5gLiteEnable_set(
	CA_IN ca_uint8_t hDevice,
	CA_IN bool Enable
	)
{
	bool status = FAILURE;
	ca_uint16_t phydata0 = 0, phydata1 = 0, phydata2 = 0;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA442, &phydata0);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA5FA, &phydata1);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA5EA, &phydata2);
	if (status != SUCCESS)
		goto exit;

	phydata0 &= (~BIT_2);
	phydata1 &= (~BIT_1);
	phydata2 &= (~BIT_0);

	if (Enable) {
		phydata0 |= BIT_2;
		phydata1 |= BIT_1;
		phydata2 |= BIT_0;
	}

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA442, phydata0);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA5FA, phydata1);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA5EA, phydata2);
	if (status != SUCCESS)
		goto exit;

exit:
	return status;
}

bool
rtl8226_serdes_option_set(
	CA_IN ca_uint8_t hDevice,
	CA_IN ca_uint8_t functioninput
	)
{
	bool status = FAILURE;
	ca_uint16_t phydata = 0;

	if ((functioninput >= 0) && (functioninput <= 5)) {
		status = mmdPhyRead(hDevice, MMD_VEND1, 0x697A, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5));
		phydata |= functioninput;

		status = mmdPhyWrite(hDevice, MMD_VEND1, 0x697A, phydata);
		if (status != SUCCESS)
			goto exit;
	}

exit:
	return status;
}

static bool 
rtl8226_wait_for_bit(
	CA_IN ca_uint8_t hDevice,
	CA_IN ca_uint16_t dev,
	CA_IN ca_uint16_t addr,
	CA_IN ca_uint16_t mask,
	CA_IN bool   set,
	CA_IN ca_uint16_t timeoutms)
{
	bool status = FAILURE;
    	ca_uint16_t phydata = 0;

	while (--timeoutms) {
		status = mmdPhyRead(hDevice, MMD_VEND2, addr, &phydata);
	        if (!status)
	        	goto exit;

	        if (!set)
			phydata = ~phydata;

		if ((phydata & mask) == mask)
			return 1;

		msleep(1);
	}

	printk("Timeout (dev=%02x addr=0x%02x mask=0x%02x timeout=%d)\n",
	      dev, addr, mask, timeoutms);

exit:
	return 0;
}

bool
rtl8226_autoNegoEnable_get(
    CA_IN  ca_uint16_t hDevice,
    CA_OUT bool *pEnable
    )
{
	bool status = FAILURE;
	ca_uint16_t phydata = 0;

	status = mmdPhyRead(hDevice, MMD_AN, 0, &phydata);
	if (status != SUCCESS)
		goto exit;

	*pEnable = (phydata & BIT_12) ? (TRUE) : (FALSE);

exit:
    return status;
}

bool
rtl8226_eeeEnable_set(
    CA_IN ca_uint16_t hDevice,
    CA_IN PHY_EEE_ENABLE *pEeeEnable
    )
{
	bool status = FAILURE;
	ca_uint16_t phydata = 0;

	bool AnEnable = 0;

	status = rtl8226_autoNegoEnable_get(hDevice, &AnEnable);
	if (status != SUCCESS)
		goto exit;

	// 100M/1G EEE
	status = mmdPhyRead(hDevice, MMD_AN, 60, &phydata);
	if (status != SUCCESS)
		goto exit;

	if (pEeeEnable->EEE_100)
		phydata |= BIT_1;
	else
		phydata &= (~BIT_1);


	if (pEeeEnable->EEE_1000)
		phydata |= BIT_2;
	else
		phydata &= (~BIT_2);

	status = mmdPhyWrite(hDevice, MMD_AN, 60, phydata);
	if (status != SUCCESS)
		goto exit;

	// 2.5G EEE
	status = mmdPhyRead(hDevice, MMD_AN, 62, &phydata);
	if (status != SUCCESS)
		goto exit;

	if (pEeeEnable->EEE_2_5G)
		phydata |= BIT_0;
	else
		phydata &= (~BIT_0);

	status = mmdPhyWrite(hDevice, MMD_AN, 62, phydata);
	if (status != SUCCESS)
		goto exit;

	// RestartAutoNegotiation
	if (AnEnable)
	{
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xA400, &phydata);
		if (status != SUCCESS)
		    goto exit;

		phydata |= BIT_9;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA400, phydata);
		if (status != SUCCESS)
		    goto exit;
	}

	exit:
	return status;
}


bool
rtl8226_phy_init(
    CA_IN ca_uint16_t hDevice,
    CA_IN PHY_LINK_ABILITY *pphylinkability,
    CA_IN bool singlephy
    )
{
	bool status = FAILURE;
    ca_uint16_t i = 0;   /* SW_SDK: use UINT16 instead of UINT8, for MMD_REG array may over 255 entries */
    ca_uint16_t phydata = 0;
    const ca_uint16_t patchver = 0x15, patchaddr = 0x8024;

	/* Polling PHY Status */
	status = rtl8226_wait_for_bit(hDevice, MMD_VEND2, 0xA420, 0x3, 1, 100);
	if (status != SUCCESS)
		goto exit;

	/* MMD 31.0xA436[15:0] = 0x801E */
	status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, 0x801E);
	if (status != SUCCESS)
		goto exit;

	status = mmdPhyRead(hDevice, MMD_VEND2, 0xA438, &phydata);
	if (status != SUCCESS)
		goto exit;

	/* Already patched. */
	if (phydata == patchver) {
		status = 1;
		goto exit;
	} else {
		/* Patch request & wait patch_rdy (for normal patch flow - Driver Initialize)
		   MMD 31.0xB820[4] = 1'b1     //(patch request) */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB820, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_4;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB820, phydata);
		if (status != SUCCESS)
			goto exit;

		/* wait for patch ready = 1 (MMD 31.0xB800[6]) */
		status = rtl8226_wait_for_bit(hDevice, MMD_VEND2, 0xB800, BIT_6, 1, 100);
		if (status != SUCCESS)
			goto exit;

		/* Set patch_key & patch_lock */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, patchaddr);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA438[15:0] = 0x8601 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, 0x8601);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA436[15:0] = 0xB82E */ 
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, 0xB82E);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA438[15:0] = 0x0001 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, 0x0001);
		if (status != SUCCESS)
			goto exit;

		/* NC & UC patch */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB820, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_7;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB820, phydata);
		if (status != SUCCESS)
			goto exit;

		/* patch nc0 */                               
		for (i=0; i<sizeof(rtl8226_n0_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_n0_ramcode[i].dev, 
				rtl8226_n0_ramcode[i].addr, rtl8226_n0_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}

		/* patch nc1 */
		for (i=0; i<sizeof(rtl8226_n1_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_n1_ramcode[i].dev,
				rtl8226_n1_ramcode[i].addr, rtl8226_n1_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}


		/* patch nc2 */
		for (i=0; i<sizeof(rtl8226_n2_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_n2_ramcode[i].dev, 
				rtl8226_n2_ramcode[i].addr, rtl8226_n2_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}

		/* patch uc2 */
		for (i=0; i<sizeof(rtl8226_uc2_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_uc2_ramcode[i].dev,
				rtl8226_uc2_ramcode[i].addr, rtl8226_uc2_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}

		/* MMD 31.0xB820[7] = 1'b0 */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB820, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_7);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB820, phydata);
		if (status != SUCCESS)
			goto exit;

		/* patch uc */                               
		for (i=0; i<sizeof(rtl8226_uc_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_uc_ramcode[i].dev, rtl8226_uc_ramcode[i].addr, rtl8226_uc_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}

		/* GPHY OCP 0xB896 bit[0] = 0x0 */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB896, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_0);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB896, phydata);
		if (status != SUCCESS)
			goto exit;

		/* GPHY OCP 0xB892 bit[15:8] = 0x0 */
		phydata = 0;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB892, phydata);
		if (status != SUCCESS)
			goto exit;

		/* patch ram code */
		for (i=0; i<sizeof(rtl8226_data_ramcode)/sizeof(MMD_REG); i++) {
			status = mmdPhyWrite(hDevice, rtl8226_data_ramcode[i].dev,
				rtl8226_data_ramcode[i].addr, rtl8226_data_ramcode[i].value);
			if (status != SUCCESS)
		        	goto exit;
		}

		/* GPHY OCP 0xB896 bit[0] = 0x1 */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB896, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_0;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB896, phydata);
		if (status != SUCCESS)
			goto exit;

		/* Clear patch_key & patch_lock */
		phydata = 0x0;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA438[15:0] = 0x0000 */
		phydata = 0x0;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xB82E[0] = 1'b0 */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB82E, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_0);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB82E, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA436[15:0] = patch_key_addr */
		phydata = patchaddr;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA438[15:0] = 0x0000 */
		phydata = 0x0;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, phydata);
		if (status != SUCCESS)
			goto exit;

		/* Release patch request */
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xB820, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_4);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB820, phydata);
		if (status != SUCCESS)
			goto exit;

		status = rtl8226_wait_for_bit(hDevice, MMD_VEND2, 0xB800, BIT_6, 0, 100);
		if (status != SUCCESS)
			goto exit;

		/* Update patch version */
		phydata = 0x801E;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xA438[15:0] =  driver_note_ver */
		phydata = patchver;
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, phydata);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa436[15:0] = 0x81B3 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa436, 0x81B3);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0043 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0043);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00A7 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00A7);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00D6 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00D6);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00EC */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00EC);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00F6 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00F6);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00FB */                   
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00FB);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00FD */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00FD);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00FF */                   
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00FF);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x00BB */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x00BB);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0058 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0058);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0029 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0029);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0013 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0013);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0009 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0009);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0004 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0004);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0002 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0002);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0000 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0000);
		if (status != SUCCESS)
			goto exit;

		/* MMD 31.0xa438[15:0] = 0x0000 */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0000);
		if (status != SUCCESS)
			goto exit;

		for (i=17; i<39; i++) {
			status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x0000);
			if (status != SUCCESS)
		        	goto exit;
		}

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa436, 0x80ea);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xa438, 0x7843);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA436, 0x8257);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA438, 0x020F);
		if (status != SUCCESS)
			goto exit;



		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD4E, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_4;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD4E, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD16, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= 0x3FF;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD16, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD32, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5));
		phydata |= 0x6;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD32, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC08, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_12);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC08, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC08, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_8);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC08, phydata);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyRead(hDevice, MMD_VEND2, 0xACC0, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_0 | BIT_1));
		phydata |= BIT_1;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xACC0, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD40, &phydata);
		if (status != SUCCESS)
		 	goto exit;

		phydata &= (~(BIT_5 | BIT_6 | BIT_7));
		phydata |= BIT_6;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD40, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD40, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_0 | BIT_1 | BIT_2));
		phydata |= BIT_2;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD40, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC14, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_7);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC14, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC80, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_8 | BIT_9));

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC80, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC5E, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_0 | BIT_1 | BIT_2));
		phydata |= BIT_1;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC5E, phydata);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD4C, 0x00A8);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC5C, 0x01FF);
		if (status != SUCCESS)
			goto exit;


		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAC8A, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_4 | BIT_5 | BIT_6 | BIT_7));
		phydata |= (BIT_4 | BIT_5);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAC8A, phydata);
		if (status != SUCCESS)
			goto exit;
		                  
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB87C, 0x80A2);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB87E, 0x0153);
		if (status != SUCCESS)
			goto exit;
		                    
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB87C, 0x809C);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB87E, 0x0153);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyRead(hDevice, MMD_VEND2, 0xD068, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_13;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xD068, phydata);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyRead(hDevice, MMD_VEND2, 0xA442, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_11;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA442, phydata);
		if (status != SUCCESS)
			goto exit;

		status = mmdPhyRead(hDevice, MMD_VEND2, 0xA432, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_4;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA432, phydata);
		if (status != SUCCESS)
			goto exit;
					
					
			status = mmdPhyRead(hDevice, MMD_VEND2, 0xB54C, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~(BIT_8 | BIT_9 | BIT_10 | BIT_11 | BIT_12 | BIT_13 | BIT_14 | BIT_15));
		phydata |= 0xDB00;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xB54C, phydata);
		if (status != SUCCESS)
			goto exit;
		    
					
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xA5D4, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata |= BIT_5;

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA5D4, phydata);
		if (status != SUCCESS)
			goto exit;
					
					
		status = mmdPhyRead(hDevice, MMD_VEND2, 0xAD4E, &phydata);
		if (status != SUCCESS)
			goto exit;

		phydata &= (~BIT_4);

		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xAD4E, phydata);
		if (status != SUCCESS)
			goto exit;				

		if (singlephy) {
			for (i=0; i<sizeof(rtl8226_isram_patch)/sizeof(MMD_REG); i++) {
				status = mmdPhyWrite(hDevice, rtl8226_isram_patch[i].dev,
					rtl8226_isram_patch[i].addr, rtl8226_isram_patch[i].value);
				if (status != SUCCESS)
					goto exit;
			}
		}

		status = rtl8226_autoNegoAbility_set(hDevice, pphylinkability);
		if (status != SUCCESS)
			goto exit;

		status = rtl8226_gigaLiteEnable_set(hDevice, TRUE);
		if (status != SUCCESS)
			goto exit;

		status = rtl8226_2p5gLiteEnable_set(hDevice, TRUE);
		if (status != SUCCESS)
			goto exit;

		status = rtl8226_serdes_option_set(hDevice, BIT_0);
		if (status != SUCCESS)
			goto exit;

		/* PHYRST & Restart Nway */
		status = mmdPhyWrite(hDevice, MMD_VEND2, 0xA400, 0x9200);
		if (status != SUCCESS)
			goto exit;
	}
	printk("rtl8226 init phy %d done\n", hDevice);
	exit:
		return status;
}

#endif /* #if defined(CONFIG_RTL_8226_SUPPORT) */

#if defined(CONFIG_ARCH_RTL8198F) && (defined(CONFIG_RTL_8367S_SUPPORT) || defined(CONFIG_RTL_8226_SUPPORT))
void __iomem *ca_ne_serdes_p0_reg_base = NULL;
void __iomem *ca_ne_glb_ne_ctl_0 = NULL;
void __iomem *ca_ne_hsgmii_0_ctl = NULL;
void __iomem *ca_ne_hsgmii_1_ctl = NULL;

typedef enum {
    SDS_AN_MODE		= 0,
    SDS_FORCE_LINK_UP	= 1,
    SDS_FORCE_LINK_DOWN	= 2,
    SDS_STATUS		= 3,
} rtl8198f_sds_an_link;

static char* next_arg(const char *argv)
{
	while (*argv != 32) {
		argv ++;
	} 
	argv ++;
	return argv;
}

static u32 rtl8198f_sds_rd(u32 sdsId, u32 npage, u32 reg_addr)
{
	u32 base_addr, mem_addr;
	u32 data;
	
	if (sdsId == 0)
		base_addr = (u32)ca_ne_serdes_p0_reg_base;
	else if (sdsId == 1)
		base_addr = (u32)ca_ne_serdes_p0_reg_base + 0x2000;

	mem_addr = base_addr + (npage << 7) + (reg_addr << 2);	
	data = 0xffff & readl(mem_addr);
	
	return data;
}

static void rtl8198f_sds_wr(u32 sdsId, u32 npage, u32 reg_addr, u32 data)
{
	u32 base_addr, mem_addr;

	if (sdsId == 0)
		base_addr = (u32)ca_ne_serdes_p0_reg_base;
	else if (sdsId == 1)
		base_addr = (u32)ca_ne_serdes_p0_reg_base + 0x2000;
	
	data = data & 0xffff;
	mem_addr = base_addr + (npage << 7) + (reg_addr << 2);
	writel(data, mem_addr);
	/* printk("rtl8198f_sds_p0_wr 0x%04x 0x%04x\n", data, rtl8198f_sds_p0_rd(npage, reg_addr)); */
}

ssize_t
ca_ni_98f_serdes_an_link(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int parameter = 8787, sdsID = 7;
	int i = 0;
	u32 data;
	char c;
	while (i <= count-1) {
		c = buf[i++];
		if ((c >= '0') && (c <= '9')) {
			if (sdsID == 7)
				sdsID = (int)(c - '0');
			else
				parameter = (int)(c - '0');
		}
	}

	if ((parameter == 8787) || (sdsID == 7)) {
		printk("invalid parameters\n");
		return count;
	}

	data = rtl8198f_sds_rd(sdsID, 0, 2);

	switch (parameter) {
		case SDS_AN_MODE:
			data = data & ~(0x3 << 8);
			rtl8198f_sds_wr(sdsID, 0, 2, data);
			printk("set serdes(%d) AN mode as : auto mode\n", sdsID);
			break;
		case SDS_FORCE_LINK_UP:
			data = data | (0x3 << 8);
			rtl8198f_sds_wr(sdsID, 0, 2, data);
			printk("set serdes(%d) AN mode as : disable AN mode, (=Force Mode linkup)\n", sdsID);
			break;
		case SDS_FORCE_LINK_DOWN:
			data = data | (0x1 << 8);
			data = data & ~(0x1 << 9);
			rtl8198f_sds_wr(sdsID, 0, 2, data);
			printk("set serdes(%d) AN mode as : disable AN mode, (=Force Mode linkdown)\n", sdsID);
			break;
		case SDS_STATUS:
			data = (data >> 8) & 0x3;
			if (data == 0)
				printk("98F SerDesID_#(%d) SDS AN-ctl mode = auto_AN_mode\n", sdsID);
			else if (data == 1)
				printk("98F SerDesID_#(%d) SDS AN-ctl mode = force_LinkDown_mode\n", sdsID);
			else if (data == 2)
				printk("98F SerDesID_#(%d) SDS AN-ctl mode = auto_AN_mode\n", sdsID);
			else if (data == 3)
				printk("98F SerDesID_#(%d) SDS AN-ctl mode = force_LinkUp_mode\n", sdsID);

			data = rtl8198f_sds_rd(sdsID, 1, 29);
			data = (data >> 8) & 0x1;
			printk("98F SerDesID_#(%d) status :SDS_SIGNOK_Real       = 0x%x\n", sdsID, data);
			data = rtl8198f_sds_rd(sdsID, 1, 29);
			data = (data >> 4) & 0xf;
			printk("98F SerDesID_#(%d) status :SDS_ch3_0_LINKOK_Real = 0x%x\n", sdsID, data);
			data = rtl8198f_sds_rd(sdsID, 1, 29);
			data = data & 0xf;
			printk("98F SerDesID_#(%d) status :SDS_ch3_0_SYNCOK_Real = 0x%x\n", sdsID, data);

			data = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_SGMII_PCS_INTERRUPT));
			if (sdsID == 0)
				data = data & 0x1;
			else if (sdsID == 1)
				data = (data >> 4) & 0x1;
			printk("98F SerDesID_#(%d) status :SDS_LINKOK_INT        = 0x%x\n", sdsID, data);
			data = rtl8198f_sds_rd(sdsID, 1, 26);
			printk("98F SerDesID_#(%d) status :ch3_0_linkDown_CNT    = 0x%x\n", sdsID, data);
			break;
		default:
			printk("ca_ni_98f_serdes_an_link invalid parameter\n");
			break;
	}
	return count;
}

ssize_t ca_ni_98f_serdes_an_show(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t count)
{
	printk("ca_ni_98f_serdes_an_link how to use\n"
		"1st parameter : SerdesID (0 or 1)\n"
		"2nd parameter : mode\n"
		"  0 : AN_MODE\n"
		"  1 : FORCE_LINK_UP\n"
		"  2 : FORCE_LINK_DOWN\n"
		"  3 : STATUS\n");
	return 0;
}

ssize_t
ca_ni_98f_serdesGlobal_status_dump(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	GLOBAL_HSGMII_0_STATUS_t hsgmii_0_status;
	GLOBAL_HSGMII_1_STATUS_t hsgmii_1_status;
	GLOBAL_HSGMII_0_CONTROL_t hsgmii_0_control;
	GLOBAL_HSGMII_1_CONTROL_t hsgmii_1_control;
	hsgmii_0_status.wrd = readl(ca_ne_hsgmii_0_ctl + 0x4);
	hsgmii_1_status.wrd = readl(ca_ne_hsgmii_0_ctl + 0x14);
	hsgmii_0_control.wrd = readl(ca_ne_hsgmii_0_ctl);
	hsgmii_1_control.wrd = readl(ca_ne_hsgmii_0_ctl + 0x10);
	printk("hsgmii_0_status = 0x%x\n", hsgmii_0_status.wrd);
	printk("hsgmii_0_control = 0x%x\n", hsgmii_0_control.wrd);
	printk("hsgmii_1_status = 0x%x\n", hsgmii_1_status.wrd);
	printk("hsgmii_1_control = 0x%x\n", hsgmii_1_control.wrd);
	return count;
}

ssize_t ca_ni_mdio_read(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t count)
{
	u16 data;
	u16 phy_id = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 mmd = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 addr = simple_strtoul(buf, NULL, 16);

	mmdPhyRead(phy_id, mmd, addr, &data);
	printk("read 0x%x.0x%x = 0x%x\n", mmd, addr, data);
	return count;
}

ssize_t ca_ni_mdio_write(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t count)
{
	u16 phy_id = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 mmd = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 addr = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 data = simple_strtoul(buf, NULL, 16);

	mmdPhyWrite(phy_id, mmd, addr, data);
	printk("write 0x%x.0x%x = 0x%x\n", mmd, addr, data);
	return count;
}

ssize_t rtl8226_power_sw(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t count)
{
	u16 data, phy_id;
	int ret;
	u16 port_id = simple_strtoul(buf, NULL, 16);
	buf = next_arg(buf);
	u16 on_off = simple_strtoul(buf, NULL, 16);

	if (port_id == 0) {
		phy_id = 0x07;
	} else if (port_id == 1) {
		phy_id = 0x06;
	} else {
		printk("invalid phy id\n");
		return count;
	}
	
	ret = mmdPhyRead(phy_id, 0x1, 0, &data);
	
	if (on_off == 0)
		data = data | (BIT_11);
	else if (on_off == 1)
		data = data & ~(BIT_11);
	else {
		printk("invalid power switch option\n");
		return count;
	}
	
	ret = mmdPhyWrite(phy_id, 0x1, 0, data);
	
	return count;
}

ssize_t rtl8226_power_sw_info(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t count)
{
	printk("1st parameter : port id\n");
	printk("2nd parameter : 0 is power off; 1 is power on\n");
	printk("enable rtl8226 for port 1\n");
	printk("echo 1 1 > rtl8226_power_sw\n");
	printk("disable rtl8226 for port 1\n");
	printk("echo 1 0 > rtl8226_power_sw\n");
	return 0;
}

static void ca_ni_98f_sds_patch(u32 sdsId)
{
	u32 buf;
	buf = rtl8198f_sds_rd(sdsId, 0x24, 6);
	buf = (buf & ~(0x3 << 2)) | (0x1 << 2);
	rtl8198f_sds_wr(sdsId, 0x24, 6, buf);
	rtl8198f_sds_wr(sdsId, 0x28, 0xE, 0x0092);
	rtl8198f_sds_wr(sdsId, 0x28, 0x6, 0x19E5);
	rtl8198f_sds_wr(sdsId, 0x28, 0x2, 0x26F4);
	rtl8198f_sds_wr(sdsId, 0x28, 0x3, 0x4B47);	
	rtl8198f_sds_wr(sdsId, 0x28, 0x08, 0x4101);
	rtl8198f_sds_wr(sdsId, 0x28, 0x07, 0x843F);
	rtl8198f_sds_wr(sdsId, 0x24, 0xE, 0x0092);
	rtl8198f_sds_wr(sdsId, 0x24, 0x6, 0x1cf5);
	rtl8198f_sds_wr(sdsId, 0x24, 0x2, 0x26F4);
	rtl8198f_sds_wr(sdsId, 0x24, 0x3, 0x4B47);	
	rtl8198f_sds_wr(sdsId, 0x24, 0x08, 0x4101);
	rtl8198f_sds_wr(sdsId, 0x24, 0x07, 0x843F);
	buf = rtl8198f_sds_rd(sdsId, 0, 14);
	buf = buf | (0x1 << 12);
	rtl8198f_sds_wr(sdsId, 0, 14, buf);	
}

void ca_ni_init_98f_hsgmii(void)
{
	u32 buf;
	
	printk("----------ca_ni_init_98f_hsgmii-----------\n");
	ca_ne_hsgmii_1_ctl = ca_ne_hsgmii_0_ctl + 0x10;
	NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg;
	NI_HV_PT_PORT_GLB_CFG_t port_glb_cfg;
	GLOBAL_NE_CONTROL_0_t ne_control_0;
	GLOBAL_HSGMII_0_CONTROL_t hsgmii_0_control;
	GLOBAL_HSGMII_1_CONTROL_t hsgmii_1_control;
	
#ifdef CONFIG_RTL_8226_SUPPORT
	NI_HV_PT_TXMAC_CFG_t txmac_cfg;
	L2FE_L2E_CTRL_t l2fel2e_ctrl;

/* TODO: the setting of ipg should be confirm */
#if 0
	u32 i;
	for (i = 0; i < 5; i++) {
		txmac_cfg.wrd = CA_NE_REG_READ(NI_HV_PT_TXMAC_CFG + 0xa0*i);
		txmac_cfg.bf.tx_ipg_sel = 1;
		CA_NE_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + 0xa0*i);
	}
#endif
	l2fel2e_ctrl.wrd = CA_NE_REG_READ(L2FE_L2E_CTRL);
	l2fel2e_ctrl.bf.aging_en = 0;
	CA_NE_REG_WRITE(l2fel2e_ctrl.wrd, L2FE_L2E_CTRL);
#endif
	/* (1) port MAC-ifMode cfg : */
	port_static_cfg.wrd = CA_NE_REG_READ(NI_HV_PT_PORT_STATIC_CFG);
	port_static_cfg.bf.int_cfg = 0;
	CA_NE_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG);
	port_glb_cfg.wrd = CA_NE_REG_READ(NI_HV_PT_PORT_GLB_CFG);
	port_glb_cfg.bf.speed = 0;
	port_glb_cfg.bf.duplex = 0;
	port_glb_cfg.bf.link_status = 1;
	CA_NE_REG_WRITE(port_glb_cfg.wrd, NI_HV_PT_PORT_GLB_CFG);
#ifdef CONFIG_RTL_8226_SUPPORT /* rtl8226 */
	port_static_cfg.wrd = CA_NE_REG_READ(NI_HV_PT_PORT_STATIC_CFG + 0xa0);
	port_static_cfg.bf.int_cfg = 0;
	CA_NE_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG + 0xa0);
	port_glb_cfg.wrd = CA_NE_REG_READ(NI_HV_PT_PORT_GLB_CFG + 0xa0);
	port_glb_cfg.bf.speed = 0;
	port_glb_cfg.bf.duplex = 0;
	port_glb_cfg.bf.link_status = 1;
	CA_NE_REG_WRITE(port_glb_cfg.wrd, NI_HV_PT_PORT_GLB_CFG + 0xa0);
#endif
	
	/* (2) port PHY-mode cfg : */
	ne_control_0.wrd = readl(ca_ne_glb_ne_ctl_0);
	ne_control_0.bf.cf_ne_p0_mode = 1;
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	ne_control_0.bf.cf_ne_p1_mode = 1;
#endif
	writel(ne_control_0.wrd, ca_ne_glb_ne_ctl_0);
	
	hsgmii_0_control.wrd = readl(ca_ne_hsgmii_0_ctl);
	hsgmii_0_control.bf.sds_iso_on_s0_0 = 0;
	hsgmii_0_control.bf.rst_reg_b_0 = 1;
#ifdef CONFIG_RTL_8367S_SUPPORT
	hsgmii_0_control.bf.sds_mode0_0 = 0x12;
	hsgmii_0_control.bf.sds0_ablty_0 = (0x1 <<12) | 0x1;
#elif CONFIG_RTL_8226_USE_PORT0
	hsgmii_0_control.bf.sds_mode0_0 = 0x2;
	hsgmii_0_control.bf.sds0_ablty_0 = 0x2;
#endif
	writel(hsgmii_0_control.wrd, ca_ne_hsgmii_0_ctl);
	
	hsgmii_1_control.wrd = readl(ca_ne_hsgmii_1_ctl);
	hsgmii_1_control.bf.rst_reg_b_1 = 1;
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	hsgmii_1_control.bf.sds_iso_on_s0_1 = 0;
	hsgmii_1_control.bf.sds_mode0_1 = 0x2;
	hsgmii_1_control.bf.sds0_ablty_1 = 0x2;	
#endif
	writel(hsgmii_1_control.wrd, ca_ne_hsgmii_1_ctl);

	/* 98F SERDES RESET : */
	hsgmii_0_control.bf.rst_sds_b_0 = 0;
	writel(hsgmii_0_control.wrd, ca_ne_hsgmii_0_ctl);
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	hsgmii_1_control.bf.rst_sds_b_1 = 0;
	writel(hsgmii_1_control.wrd, ca_ne_hsgmii_1_ctl);
#endif

	ca_ni_98f_sds_patch(0);
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	ca_ni_98f_sds_patch(1);
#endif
	
	/* ## SDS reset flow : */
	hsgmii_0_control.bf.sds_iso_on_s0_0 = 0;
	hsgmii_0_control.bf.rst_sds_b_0 = 0;
	writel(hsgmii_0_control.wrd, ca_ne_hsgmii_0_ctl);
	mdelay(100);
	hsgmii_0_control.bf.rst_sds_b_0 = 1;
	writel(hsgmii_0_control.wrd, ca_ne_hsgmii_0_ctl);
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	hsgmii_1_control.bf.sds_iso_on_s0_1 = 0;
	hsgmii_1_control.bf.rst_sds_b_1 = 0;
	writel(hsgmii_1_control.wrd, ca_ne_hsgmii_1_ctl);
	mdelay(100);
	hsgmii_1_control.bf.rst_sds_b_1 = 1;
	writel(hsgmii_1_control.wrd, ca_ne_hsgmii_1_ctl);
#endif
	// rtl8226_hsgmii_en();	
}
EXPORT_SYMBOL(ca_ni_init_98f_hsgmii);

void ca_ni_reset_98f_cdr(void)
{
	/* ## reset 98F CDR :  */
	rtl8198f_sds_wr(0, 0x0, 0x0, 0x1401);
 	rtl8198f_sds_wr(0, 0x0, 0x0, 0x1403);
#ifdef CONFIG_RTL_8226_USE_PORT1 /* rtl8226 */
	rtl8198f_sds_wr(1, 0x0, 0x0, 0x1401);
 	rtl8198f_sds_wr(1, 0x0, 0x0, 0x1403);
#endif
}
EXPORT_SYMBOL(ca_ni_reset_98f_cdr);
#endif 

#ifdef CONFIG_RTL_8226_SUPPORT /* rtl8226 */

#define RTL8226_TIMER_FREQ	1 /*sec */
struct timer_list rtl8226_timer;

typedef enum {
    RTL8226_LINK_DOWN		= 0,
    RTL8226_LINK_UP_2G5		= 1,
    RTL8226_LINK_UP_2G5LITE	= 2,
    RTL8226_LINK_UP_1G		= 3,
    RTL8226_LINK_UP_500M	= 4,
    RTL8226_LINK_UP_100M	= 5,
    RTL8226_LINK_UP_10M		= 6,
    RTL8226_LINK_UNKNOWN	= 7,
} rtl8226_link_st_t;
typedef enum {
	RTL8226_IEEE_INIT 	  = 0,
	RTL8226_IEEE_INIT_10M 	  = 1,
	RTL8226_IEEE_DIFF_VOLTAGE = 2,
	RTL8226_IEEE_HARMONIC	  = 3,
	RTL8226_IEEE_HARMONIC_0	  = 4,
	RTL8226_IEEE_100M	  = 5,
	RTL8226_IEEE_GIGA	  = 6,
	RTL8226_IEEE_2G5	  = 7,
	RTL8226_IEEE_EN		  = 8,
} rtl8226_ieee_t;

typedef enum {
	RTL8226_IEEE_EN_DISABLE   = 0,
	RTL8226_IEEE_EN_ENABLE	  = 1,
} rtl8226_ieee_en_t;

typedef enum {
	RTL8226_IEEE_100M_INIT 	  = 0,
	RTL8226_IEEE_100M_CHA 	  = 1,
	RTL8226_IEEE_100M_CHB	  = 2,
	RTL8226_IEEE_100M_FINISH  = 3,
} rtl8226_ieee_100M_t;

typedef enum {
	RTL8226_IEEE_GIGA_INIT 	  = 0,
	RTL8226_IEEE_GIGA_MODE1	  = 1,
	RTL8226_IEEE_GIGA_MODE2   = 2,
	RTL8226_IEEE_GIGA_MODE4	  = 3,
	RTL8226_IEEE_GIGA_FINISH  = 4,
} rtl8226_ieee_GIGA_t;

typedef enum {
	RTL8226_IEEE_2G5_INIT 	  = 0,
	RTL8226_IEEE_2G5_MODE1	  = 1,
	RTL8226_IEEE_2G5_MODE2    = 2,
	RTL8226_IEEE_2G5_MODE3	  = 3,
	RTL8226_IEEE_2G5_MODE4_T1 = 4,
	RTL8226_IEEE_2G5_MODE4_T2 = 5,
	RTL8226_IEEE_2G5_MODE4_T3 = 6,
	RTL8226_IEEE_2G5_MODE4_T4 = 7,
	RTL8226_IEEE_2G5_MODE4_T5 = 8,
	RTL8226_IEEE_2G5_MODE5	  = 9,
	RTL8226_IEEE_2G5_MODE6	  = 10,
	RTL8226_IEEE_2G5_FINISH	  = 11,
} rtl8226_ieee_2G5_t;

static int last_link_st_p0 = RTL8226_LINK_DOWN;
static int last_link_st_p1 = RTL8226_LINK_DOWN;
/* Detail link status */
static int last_port_st_p0;
static int last_port_st_p1;
static int rtl8226_chk_link_st(int phy_id);
int rtl8226_sds_patch(int phy_id, int linkst);

int rtl8226_get_link_speed(int phyid)
{
	if (phyid == 0x6) {
		return last_link_st_p1;
	}
	else if (phyid == 0x7) {
		return last_link_st_p0;
	}
	else 
		printk("ERROR : %s phyid not correct!!!\n", __func__);
	
	return CA_E_ERROR;	
}
EXPORT_SYMBOL(rtl8226_get_link_speed);

int rtl8226_get_link_status(int phyid)
{
	int device_id = 0;
	
	if (phyid == 0x6) {
		aal_mdio_read_indirect(device_id, phyid, 0x1F, 0xA434, &last_port_st_p1);
		return last_port_st_p1;
	}
	else if (phyid == 0x7) {
		aal_mdio_read_indirect(device_id, phyid, 0x1F, 0xA434, &last_port_st_p0);
		return last_port_st_p0;
	}
	else 
		printk("ERROR : %s phyid not correct!!!\n", __func__);
	
	return CA_E_ERROR;	
}
EXPORT_SYMBOL(rtl8226_get_link_status);

int rtl8226_phy_reg_get(int phyid, unsigned int mmd1, unsigned int mmd2, unsigned short *regdata)
{
	int device_id = 0;
	int ret = 0;
	
	ret = aal_mdio_read_indirect(device_id, phyid, mmd1, mmd2, regdata);
	if (ret){
		printk("[%s %d] aal_mdio_read_indirect fail!!!\n", __func__, __LINE__);
		return CA_E_ERROR;	
	}

	return CA_E_OK;	
}
EXPORT_SYMBOL(rtl8226_phy_reg_get);

int rtl8226_phy_reg_set(int phyid, unsigned int mmd1, unsigned int mmd2, unsigned short regdata)
{
	int device_id = 0;
	int ret = 0;
				
	ret = aal_mdio_write_indirect(device_id, phyid, mmd1, mmd2, regdata);
	if (ret){
		printk("[%s %d] aal_mdio_write_indirect fail!!!\n", __func__, __LINE__);
		return CA_E_ERROR;	
	}

	return CA_E_OK;	
}
EXPORT_SYMBOL(rtl8226_phy_reg_set);

int rtl8226_phyid = 0x6;

void set_mdio45_gphywb(unsigned int phyid, unsigned int mmd, unsigned int reg, unsigned int mask, unsigned int val)
{

	static ca_uint16_t device_id = 0;
	ca_uint16_t tdata = 0, data;
	unsigned int wphyid = 0;	//start
	unsigned int wphyid_end = 1;   //end
	
	if (phyid == 999) {
		wphyid = 0;
		wphyid_end = 5;    //total phyid=0~4
	} else {
		wphyid= phyid;
		wphyid_end= phyid + 1;
	}

	for(; wphyid < wphyid_end; wphyid++)
	{
		if(mask!=0)
		{
			aal_mdio_read_indirect(device_id, wphyid, mmd, reg, &data);
			tdata = data & mask;
		}
		aal_mdio_write_indirect(device_id, wphyid, mmd, reg, tdata|val);

	}
}

static int polling_8226 = 1;

static void rtl8226_eth_polling(unsigned long t)
{
	int linkst, *last_link_st;

#if CONFIG_RTL_8226_USE_PORT1
	linkst = rtl8226_chk_link_st(rtl8226_phyid);
	last_link_st = &last_link_st_p1;
	if (linkst != *last_link_st) {
		*last_link_st = linkst;
		rtl8226_sds_patch(rtl8226_phyid, linkst);
	}
#endif
#if CONFIG_RTL_8226_USE_PORT0
	linkst = rtl8226_chk_link_st(rtl8226_phyid + 1);
	last_link_st = &last_link_st_p0;
	if (linkst != *last_link_st) {
		*last_link_st = linkst;
		rtl8226_sds_patch(rtl8226_phyid + 1, linkst);
	}
#endif

	rtl8226_timer.expires = jiffies + RTL8226_TIMER_FREQ*100;
	if (polling_8226 == 1)
		add_timer(&rtl8226_timer);
}

void rtl8226_init_timer(void)
{	
        init_timer(&rtl8226_timer);
        rtl8226_timer.function = rtl8226_eth_polling;
        rtl8226_timer.data = ((unsigned long) 0);
        rtl8226_timer.expires = jiffies + 500;
        add_timer(&rtl8226_timer);
}
EXPORT_SYMBOL(rtl8226_init_timer);

ssize_t
do_polling(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int parameter = polling_8226;
	int i = 0;
	char c;
	while(i <= count-1) {
		c = buf[i++];
		if (c == '0')
			parameter = 0;
		else if (c == '1')
			parameter = 1;
	}
	if ((parameter == 1) && (polling_8226 == 0)) {
		polling_8226 = 1;
		rtl8226_eth_polling(1);
		printk("enable polling_8226\n");
	}
	else if (parameter == 0) {
		polling_8226 = 0;
		printk("disable polling_8226\n");
	}
	return count;
}

ssize_t
chg_link_speed(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int link_ability = 0;
	int i = 0;
	char c;
	while(i <= count-1) {
		c = buf[i++];
		if ((c >= '0') && (c <= '9')) {
			link_ability = c - '0';
		}
	}

#if CONFIG_RTL_8226_USE_PORT1
	/* re-nego */
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA400, 0x0000, 0x9200);
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5D4, 0xFF7F, 0x0000); /* 2G5 */
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA412, 0xFDFF, 0x0000); /* giga */
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA408, 0xFE1F, 0x0000); /* 10M/100M */
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5EA, 0xFFFE, 0x0000); /* 2G5 lite */
	set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA428, 0xFDFF, 0x0000); /* 1G lite */

	switch (link_ability) {
		case RTL8226_LINK_DOWN:
			printk("phy%d set link down\n", rtl8226_phyid);
			break;
		case RTL8226_LINK_UP_2G5:
			printk("phy%d set link 2G5\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5D4, 0xFF7F, 0x0080); /* 2G5 */
			break;
		case RTL8226_LINK_UP_2G5LITE:
			printk("phy%d set link 2G5LITE\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5EA, 0xFFFE, 0x0001); /* 2G5 lite */
			break;
		case RTL8226_LINK_UP_1G:
			printk("phy%d set link 1G\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA412, 0xFDFF, 0x0200); /* giga */
			break;
		case RTL8226_LINK_UP_500M:
			printk("phy%d set link 1GLITE\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA428, 0xFDFF, 0x0200); /* 1G lite */
			break;
		case RTL8226_LINK_UP_100M:
			printk("phy%d set link 100M\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA408, 0xFE1F, 0x0180); /* 10M/100M */
			break;
		case RTL8226_LINK_UP_10M:
			printk("phy%d set link 10M\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA408, 0xFE1F, 0x0060); /* 10M/100M */
			break;
		default:
			printk("phy%d recover to auto-negotiation mode\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5D4, 0xFF7F, 0x0080); /* 2G5 */
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA412, 0xFDFF, 0x0200); /* giga */
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA408, 0xFE1F, 0x01e0); /* 10M/100M */
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA5EA, 0xFFFE, 0x0001); /* 2G5 lite */
			set_mdio45_gphywb(rtl8226_phyid, 0x1f, 0xA428, 0xFDFF, 0x0200); /* 1G lite */
			break;
	}
#endif
#if CONFIG_RTL_8226_USE_PORT0
	/* re-nego */
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA400, 0x0000, 0x9200);
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5D4, 0xFF7F, 0x0000); /* 2G5 */
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA412, 0xFDFF, 0x0000); /* giga */
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA408, 0xFE1F, 0x0000); /* 10M/100M */
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5EA, 0xFFFE, 0x0000); /* 2G5 lite */
	set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA428, 0xFDFF, 0x0000); /* 1G lite */

	switch (link_ability) {
		case RTL8226_LINK_DOWN:
			printk("phy%d set link down\n", rtl8226_phyid);
			break;
		case RTL8226_LINK_UP_2G5:
			printk("phy%d set link 2G5\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5D4, 0xFF7F, 0x0080); /* 2G5 */
			break;
		case RTL8226_LINK_UP_2G5LITE:
			printk("phy%d set link 2G5LITE\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5EA, 0xFFFE, 0x0001); /* 2G5 lite */
			break;
		case RTL8226_LINK_UP_1G:
			printk("phy%d set link 1G\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA412, 0xFDFF, 0x0200); /* giga */
			break;
		case RTL8226_LINK_UP_500M:
			printk("phy%d set link 1GLITE\n", rtl8226_phyid);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA428, 0xFDFF, 0x0200); /* 1G lite */
			break;
		case RTL8226_LINK_UP_100M:
			printk("phy%d set link 100M\n", rtl8226_phyid + 1);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA408, 0xFE1F, 0x0180); /* 10M/100M */
			break;
		case RTL8226_LINK_UP_10M:
			printk("phy%d set link 10M\n", rtl8226_phyid + 1);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA408, 0xFE1F, 0x0060); /* 10M/100M */
			break;
		default:
			printk("phy%d recover to auto-negotiation mode\n", rtl8226_phyid + 1);
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5D4, 0xFF7F, 0x0080); /* 2G5 */
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA412, 0xFDFF, 0x0200); /* giga */
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA408, 0xFE1F, 0x01e0); /* 10M/100M */
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA5EA, 0xFFFE, 0x0001); /* 2G5 lite */
			set_mdio45_gphywb(rtl8226_phyid + 1, 0x1f, 0xA428, 0xFDFF, 0x0200); /* 1G lite */
			break;
	}
#endif
	return count;
}

ssize_t 
rtl8226_ieee_measure(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	u32 sds_id, idx, sub_idx;
	PHY_EEE_ENABLE p_eee_enable;
	sds_id = simple_strtoul(buf, NULL, 0);
	buf = next_arg(buf);
	idx = simple_strtoul(buf, NULL, 0);
	buf = next_arg(buf);
	sub_idx = simple_strtoul(buf, NULL, 0);
	printk("sds %d, idx %d, sub-idx %d\n", sds_id, idx, sub_idx);

	/* mapping to phyid */
	if (sds_id == 0)
		sds_id = 0x7;
	else if (sds_id == 1)
		sds_id = 0x6;
	else {
		printk("invaild sds id\n");
		return count;
	}
	
	switch (idx) {
	case RTL8226_IEEE_INIT:
		printk("Initial Setting\n");
		set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x0000, 0x0);
		set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x0);
		set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x0);
		/* disable 10M IEEE */
		set_mdio45_gphywb(sds_id, 0x1f, 0xa432, 0xffef, 0x0);
		break;
	case RTL8226_IEEE_INIT_10M:
		printk("Initial Setting enable 10M IEEE\n");
		set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x0000, 0x0);
		set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x0);
		set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x0);
		/* Enable 10M IEEE */
		set_mdio45_gphywb(sds_id, 0x1f, 0xa432, 0xffef, 0x10);
		break;
	case RTL8226_IEEE_DIFF_VOLTAGE:
		printk("Diff Voltage/ TP-IDL/ Jitter\n");
		set_mdio45_gphywb(sds_id, 0x1f, 0xc804, 0x0, 0x0115);
		set_mdio45_gphywb(sds_id, 0x1f, 0xc800, 0x0, 0x5a21);
		break;
	case RTL8226_IEEE_HARMONIC:
		printk("Harmonic\n");
		set_mdio45_gphywb(sds_id, 0x1f, 0xc804, 0x0, 0x0015);
		set_mdio45_gphywb(sds_id, 0x1f, 0xc800, 0x0, 0xff21);
		break;
	case RTL8226_IEEE_HARMONIC_0:
		printk("Harmonic - all 0 pattern\n");
		set_mdio45_gphywb(sds_id, 0x1f, 0xc804, 0x0, 0x0015);
		set_mdio45_gphywb(sds_id, 0x1f, 0xc800, 0x0, 0x0021);
		break;
	case RTL8226_IEEE_100M:
		printk("100Mbps MLT-3\n");
		switch (sub_idx) {
		case RTL8226_IEEE_100M_INIT:
			printk("Initial Setting\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x0000, 0x2000);
			set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x0);
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x0);
			break;
		case RTL8226_IEEE_100M_CHA:
			printk("From Channel A\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa430, 0xfcff, 0x300);
			break;
		case RTL8226_IEEE_100M_CHB:
			printk("From Channel B\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa430, 0xfcff, 0x200);
			break;
		case RTL8226_IEEE_100M_FINISH:
			printk("Finish MLT-3\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa430, 0xfcff, 0x0);
			set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x1000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x7fff, 0x8000);
			break;
		default:
			break;
		}
		break;
	case RTL8226_IEEE_GIGA:
		printk("Giga Test Mode\n");
		switch (sub_idx) {
		case RTL8226_IEEE_GIGA_INIT:
			printk("Initial Setting\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x7fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x1000);
			break;
		case RTL8226_IEEE_GIGA_MODE1:
			printk("Enable Test Mode 1\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x2000);
			break;
		case RTL8226_IEEE_GIGA_MODE2:
			printk("Enable Test Mode 2\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x4000);
			break;
		case RTL8226_IEEE_GIGA_MODE4:
			printk("Enable Test Mode 4\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x8000);
			break;
		case RTL8226_IEEE_GIGA_FINISH:
			printk("Finish Giga Test Mode\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x0000);
			break;
		default:
			break;
		}
		break;
	case RTL8226_IEEE_2G5:
		printk("2.5G Test Mode\n");
		switch (sub_idx) {
		case RTL8226_IEEE_2G5_INIT:
			printk("Initial Setting\n");
			set_mdio45_gphywb(sds_id, 0x1f, 0xa412, 0x1fff, 0x0000);
			set_mdio45_gphywb(sds_id, 0x7, 0x0, 0xefff, 0x1000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0, 0x0, 0x2058);
			break;
		case RTL8226_IEEE_2G5_MODE1:
			printk("Enable Test Mode 1\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x2000);
			break;
		case RTL8226_IEEE_2G5_MODE2:
			printk("Enable Test Mode 2\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x4000);
			break;
		case RTL8226_IEEE_2G5_MODE3:
			printk("Enable Test Mode 3\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x6000);
			break;
		case RTL8226_IEEE_2G5_MODE4_T1:
			printk("Enable Test Mode 4, Tone 1\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0xe3ff, 0x0400);
			break;
		case RTL8226_IEEE_2G5_MODE4_T2:
			printk("Enable Test Mode 4, Tone 2\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0xe3ff, 0x0800);
			break;
		case RTL8226_IEEE_2G5_MODE4_T3:
			printk("Enable Test Mode 4, Tone 3\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0xe3ff, 0x1000);
			break;
		case RTL8226_IEEE_2G5_MODE4_T4:
			printk("Enable Test Mode 4, Tone 4\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0xe3ff, 0x1400);
			break;
		case RTL8226_IEEE_2G5_MODE4_T5:
			printk("Enable Test Mode 4, Tone 5\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x8000);
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0xe3ff, 0x1800);
			break;
		case RTL8226_IEEE_2G5_MODE5:
			printk("Enable Test Mode 5\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0xa000);
			break;
		case RTL8226_IEEE_2G5_MODE6:
			printk("Enable Test Mode 6\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0xc000);
			break;
		case RTL8226_IEEE_2G5_FINISH:
			printk("Finish 2.5G Test Mode\n");
			set_mdio45_gphywb(sds_id, 0x1, 0x0084, 0x1fff, 0x0000);
			break;
		default:
			break;
		}
		break;
	case RTL8226_IEEE_EN:
		if (sub_idx == RTL8226_IEEE_EN_DISABLE) {
			printk("IEEE disable");
			p_eee_enable.EEE_100 = 0;
			p_eee_enable.EEE_1000 = 0;
			p_eee_enable.EEE_2_5G = 0;
		} else {
			printk("IEEE enable");
			p_eee_enable.EEE_100 = 1;
			p_eee_enable.EEE_1000 = 1;
			p_eee_enable.EEE_2_5G = 1;
		}
		rtl8226_eeeEnable_set(sds_id, &p_eee_enable);
		break;
	default:
		printk("invaild parameters\n");
		break;
	}
	return count;
}

ssize_t 
rtl8226_ieee_measure_usage(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	printk("rtl8226_ieee_measure usage\n"
		"echo [Serdes ID] [Setting Index] [Setting sub-index] > rtl8226_ieee\n");
	return 0;
}
void rtl8226_2g5_setup(int phy_id)
{
	bool status = FAILURE;
	PHY_LINK_ABILITY link_ability;

	link_ability.Half_10 = TRUE;
	link_ability.Full_10 = TRUE;
	link_ability.Half_100 = TRUE;
	link_ability.Full_100 = TRUE;
	link_ability.Full_1000 = TRUE;
	link_ability.adv_2_5G = TRUE;
	link_ability.FC = TRUE;
	link_ability.AsyFC = TRUE; 

	status = rtl8226_phy_init((ca_uint16_t) phy_id, &link_ability, TRUE);
	if (status != SUCCESS)
		printk("rtl8226 phy init fail\n");

	if ((phy_id == 0x6) | (phy_id == 0x7)) {
		/* setting LED to low active */
		set_mdio45_gphywb(phy_id, 0x1f, 0xd044, 0xFFF0, 0x0);
		set_mdio45_gphywb(phy_id, 0x1f, 0xd032, 0xFF58, 0xa7);
	}
}
EXPORT_SYMBOL(rtl8226_2g5_setup);

static int rtl8226_chk_link_st(int phy_id)
{
	ca_uint16_t ret, data, linkst, speed, speed_grp, device_id = 0;
	ret = aal_mdio_read_indirect(device_id, phy_id, 7, 0x001, &data);
	linkst = data & (0x1 << 2);
	//printk("rtl8226_chk_link_st 1st %d\n", linkst);
	if (linkst == 0) {
		ret = aal_mdio_read_indirect(device_id, phy_id, 7, 0x001, &data);
		linkst = data & (0x1 << 2);
		//printk("rtl8226_chk_link_st 2nd %x\n", data);
		if (linkst == 0) {
			return RTL8226_LINK_DOWN;
		} else {
			ret = aal_mdio_read_indirect(device_id, phy_id, 0x1F, 0xA434, &data);
			speed = (data & (0x3 << 4)) >> 4;
			speed_grp = (data & (0x3 << 9)) >> 9;
			if (speed_grp == 1) {
				if (speed == 1)
					return RTL8226_LINK_UP_2G5;
				else if (speed == 3)	/* 2G5 Lite */
					return RTL8226_LINK_UP_2G5LITE;
				else	/* 0 = 10G ; 2 = 5G */
					return RTL8226_LINK_UNKNOWN;
			}
			else {
				if (speed == 3)	/* 500M */
					return RTL8226_LINK_UP_500M;
				else if (speed == 2)	/* 1000M */
					return RTL8226_LINK_UP_1G;
				else if (speed == 1)	/* 100M */
					return RTL8226_LINK_UP_100M;
				else if (speed == 0)	/* 10M */
					return RTL8226_LINK_UP_10M;
				else	/* 10M */
					return RTL8226_LINK_UNKNOWN;
			}
			
		}
	} else {
		ret = aal_mdio_read_indirect(device_id, phy_id, 0x1F, 0xA434, &data);
		speed = (data & (0x3 << 4)) >> 4;
		speed_grp = (data & (0x3 << 9)) >> 9;
		if (speed_grp == 1) {
			if (speed == 1)
				return RTL8226_LINK_UP_2G5;
			else if (speed == 3)	/* 2G5 Lite */
				return RTL8226_LINK_UP_2G5LITE;
			else	/* 0 = 10G ; 2 = 5G */
				return RTL8226_LINK_UNKNOWN;
		}
		else {
			if (speed == 3)	/* 500M */
				return RTL8226_LINK_UP_500M;
			else if (speed == 2)	/* 1000M */
				return RTL8226_LINK_UP_1G;
			else if (speed == 1)	/* 100M */
				return RTL8226_LINK_UP_100M;
			else if (speed == 0)	/* 10M */
				return RTL8226_LINK_UP_10M;
			else
				return RTL8226_LINK_UNKNOWN;	
		}


	}
}

int rtl8226_sds_patch(int phy_id, int linkst)
{
	static uint64_t device_id = 0;
	GLOBAL_HSGMII_1_CONTROL_t hsgmii_1_control;
	ca_ne_hsgmii_1_ctl = ca_ne_hsgmii_0_ctl + 0x10;
	void __iomem * hsgmii_ctrl;
	ca_uint16_t data;
	/* printk("phy_id %d linkst %d\n", phy_id, linkst); */

#if 0	/* dbg SDS status */
	aal_mdio_write_indirect(device_id, phy_id, 0x1E, 0x7581, 0x0000);
	aal_mdio_write_indirect(device_id, phy_id, 0x1E, 0x758B, 0x0005);
	ca_udelay(100);
	aal_mdio_read_indirect(device_id, phy_id, 0x1E, 0x758D, &data);
	printk("...0x%x", data);
	data = data & 2;
	if (data != 2)
		printk(".....SDS link down.....0x%x\n", data); 
#endif
	
	/* TODO : it might let chariot throughput low */
	aal_mdio_write_indirect(device_id, phy_id, 0x1E, 0x75B5, 0x0053);

	if (phy_id == 0x6)
		hsgmii_ctrl = ca_ne_hsgmii_1_ctl;
	else if (phy_id == 0x7)
		hsgmii_ctrl = ca_ne_hsgmii_0_ctl;
	else
		hsgmii_ctrl = ca_ne_hsgmii_1_ctl;

	switch (linkst) {
		case RTL8226_LINK_DOWN:
			printk("phy%d RTL8226_LINK_DOWN\n", phy_id);
			break;
		case RTL8226_LINK_UP_2G5:
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x12;
			hsgmii_1_control.bf.sds0_ablty_1 = (0x1 <<12) | 0x1;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_2G5\n", phy_id);
			break;
		case RTL8226_LINK_UP_2G5LITE:
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x2;
			hsgmii_1_control.bf.sds0_ablty_1 = 0x2;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_2G5LITE\n", phy_id);
			break;
		case RTL8226_LINK_UP_1G:
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x2;
			hsgmii_1_control.bf.sds0_ablty_1 = 0x2;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_1G\n", phy_id);
			break;
		case RTL8226_LINK_UP_500M: /* 1GLITE */
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x2;
			hsgmii_1_control.bf.sds0_ablty_1 = 0x3;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_500M (1GLITE)\n", phy_id);
			break;
		case RTL8226_LINK_UP_100M:
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x2;
			hsgmii_1_control.bf.sds0_ablty_1 = 0x1;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_100M\n", phy_id);
			break;
		case RTL8226_LINK_UP_10M:
			hsgmii_1_control.wrd = readl(hsgmii_ctrl);
			hsgmii_1_control.bf.sds_mode0_1 = 0x2;
			hsgmii_1_control.bf.sds0_ablty_1 = 0;
			writel(hsgmii_1_control.wrd, hsgmii_ctrl);
			printk("phy%d RTL8226_LINK_UP_10M\n", phy_id);
			break;
		default :
			printk("unknown link status\n");
			break;
	}
	
	return CA_E_OK;
}
EXPORT_SYMBOL(rtl8226_sds_patch);

#endif /* CONFIG_RTL_8226_SUPPORT */

