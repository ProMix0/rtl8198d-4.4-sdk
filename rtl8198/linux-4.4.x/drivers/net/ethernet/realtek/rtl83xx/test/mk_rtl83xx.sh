
STAGING_DIR=../../../../../staging_dir/target-aarch64-openwrt-linux-gnu/
RTL83XX_DIR=../../../../../build_dir/target-aarch64-openwrt-linux-gnu/linux-g3_eng//rtl83xx-1.3.9/API_Source/

/auto/project2/sw/toolchains/openwrt-4.4_gcc-4.9-linaro_glibc-2.21/bin/aarch64-openwrt-linux-gnu-gcc -I${RTL83XX_DIR}  -I${STAGING_DIR}/usr/include/cortina-api/ -I${STAGING_DIR}/usr/include/ -I ${STAGING_DIR}/usr/include/plat-user -I${STAGING_DIR}/usr/include/libnl3 -o rtl83xx_ioctl  rtl83xx.c -L ${STAGING_DIR}/usr/lib   
