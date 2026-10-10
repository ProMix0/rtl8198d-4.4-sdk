#!/bin/sh

BOARD_PATH="vendors/Realtek/luna_ap_mips/conf44/V0A4G_98D_8832B_8192FE_MULTI_WAN_EASYMESH"
BOARD_NAME=$(basename ${BOARD_PATH})
BOARD_CFG=${BOARD_PATH}"/config"

TOOLCHAIN_LINK_PATH=$(grep CONFIG_RSDK_DIR= ${BOARD_CFG} | awk -F= '{print $2}' | sed 's/\"//g')
TOOLCHAIN_FULL_PATH=$(pwd)/"toolchain/msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206"

if [ ! -L ${TOOLCHAIN_LINK_PATH} ]; then
	ln -s ${TOOLCHAIN_FULL_PATH} ${TOOLCHAIN_LINK_PATH}
elif [ "$(readlink ${TOOLCHAIN_LINK_PATH})" != "${TOOLCHAIN_FULL_PATH}" ]; then
	rm ${TOOLCHAIN_LINK_PATH}
	ln -s ${TOOLCHAIN_FULL_PATH} ${TOOLCHAIN_LINK_PATH}
fi

make preconfig44_${BOARD_NAME}
# make menuconfig
