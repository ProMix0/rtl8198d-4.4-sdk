#!/bin/sh

if [ ! -d "$1" ]; then 
	exit; 
fi

#Reference resources from luna/conf44/9607C_spi_nand_demo
[ -f $1/rtl8198d_engboard.dts ] && cp $1/rtl8198d_engboard.dts $ROOTDIR/$LINUXDIR/arch/mips/boot/dts/realtek
[ -f $1/board_rtl8198d.c ] && cp $1/board_rtl8198d.c $ROOTDIR/$LINUXDIR/drivers/soc/realtek/
