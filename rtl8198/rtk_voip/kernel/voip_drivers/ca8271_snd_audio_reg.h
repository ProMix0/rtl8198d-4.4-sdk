#ifndef __CA8271_SND_AUDIO_REG_H__
#define __CA8271_SND_AUDIO_REG_H__

#include <soc/cortina/ca827x_atu.h>
#include "ca8271_registers.h"

/* SSP Register Map ***********************************************************/
#define CA_SSP_BASE 				KSEG1_ATU_XLAT(0xd000ab80)

#define CA_SSP_ID				0x00
#define CA_SSP_CTRL0				0x04
#define CA_SSP_CTRL1				0x08
#define CA_SSP_FRAME_CTRL0			0x0C
#define CA_SSP_FRAME_CTRL1			0x10
#define CA_SSP_BAUDRATE_CTRL			0x14
#define CA_SSP_FIFO_CTRL			0x18
#define CA_SSP_FIFO_PTR				0x1C
#define CA_SSP_INTERRUPT			0x20
#define CA_SSP_INT_ENABLE			0x24
#define CA_SSP_TXSLOT_VLD0			0x28
#define CA_SSP_TXSLOT_VLD1			0x2C
#define CA_SSP_TXSLOT_VLD2			0x30
#define CA_SSP_TXSLOT_VLD3			0x34
#define CA_SSP_RXSLOT_VLD0			0x38
#define CA_SSP_RXSLOT_VLD1			0x3C
#define CA_SSP_RXSLOT_VLD2			0x40
#define CA_SSP_RXSLOT_VLD3			0x44
#define CA_SSP_SLOT_SIZE0			0x48
#define CA_SSP_SLOT_SIZE1			0x4C
#define	CA_SSP_SLOT_SIZE2			0x50
#define CA_SSP_SLOT_SIZE3			0x54
#define CA_SSP_BAUDRATE_CTRL1			0x58
#define CA_SSP_SPDIF_RX_CTRL			0x5C
#define CA_SSP_SPDIF_RX_STATUS			0x60
#define CA_SSP_SPARE				0x64

/* DMA GLB Register Map *******************************************************/
#define CA_DMA_GLB_BASE				KSEG1_ATU_XLAT(0xd000a064)

#define CA_DMA_GLB_RX_CTRL 			0x00
#define CA_DMA_GLB_TX_CTRL 			0x04

/* DMA SSP Register Map *******************************************************/
#define CA_DMA_SSP_BASE				KSEG1_ATU_XLAT(0xd000aa80)

#define CA_DMA_SSP_RX_CONTROL			0x00
#define	CA_DMA_SSP_RX_BASE_DEPTH		0x04
#define CA_DMA_SSP_RX_BASE			0x08
#define CA_DMA_SSP_RX_WPTR			0x0C
#define CA_DMA_SSP_RX_RPTR			0x10
#define	CA_DMA_SSP_RX_PKTCNT			0x14
#define CA_DMA_SSP_TX_CONTROL			0x18
#define	CA_DMA_SSP_TX_BASE_DEPTH		0x1C
#define CA_DMA_SSP_TX_BASE			0x20
#define CA_DMA_SSP_TX_WPTR			0x24
#define CA_DMA_SSP_TX_RPTR			0x28
#define CA_DMA_SSP_TX_PKTCNT			0x2C
#define CA_DMA_SSP_RX_INTERRUPT			0x30
#define CA_DMA_SSP_RX_INTENABLE			0x34
#define CA_DMA_SSP_TX_INTERRUPT			0x38
#define CA_DMA_SSP_TX_INTENABLE			0x3C
#define CA_DMA_SSP_RX_MISC_INTERRUPT		0x40
#define CA_DMA_SSP_RX_MISC_INTENABLE		0x44
#define CA_DMA_SSP_TX_MISC_INTERRUPT		0x48
#define CA_DMA_SSP_TX_MISC_INTENABLE		0x4C
#define CA_DMA_SSP_RX_COAL_INTERRUPT		0x50
#define CA_DMA_SSP_RX_COAL_INTENABLE		0x54
#define CA_DMA_SSP_TX_COAL_INTERRUPT		0x58
#define CA_DMA_SSP_TX_COAL_INTENABLE		0x5C

/* misc *******************************************************/

#define CA_DMA_INT_DMA_SSP_INT0			KSEG1_ATU_XLAT(DMA_SEC_DMA_INT_DMA_SSP_INT0)
#define CA_DMA_INT_DMA_SSP_INTEN0		KSEG1_ATU_XLAT(DMA_SEC_DMA_INT_DMA_SSP_INTEN0)

#endif /* __CA8271_SND_AUDIO_REG_H__ */
