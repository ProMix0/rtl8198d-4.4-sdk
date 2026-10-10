#/bin/sh
ROOTDIR=$1
MKIMG=${ROOTDIR}/tools/uboot_2018/mkimage
if [ ! -f ${MKIMG} ] ;then
	echo "Error: Not found mkimage: ${MKIMG}"
	exit 1
fi
cd ${ROOTDIR}/images

ln -sf ../tools/boot_verified_venus.its boot_verified_venus.its
echo "Doing: ${MKIMG} -f  boot_verified_venus.its  uImage"
${MKIMG} -f  boot_verified_venus.its  uImage
if [ ! -f uImage ];then
	echo "uImage generates fail"
i	exit 1
fi
echo "Doing: ${MKIMG} -F -k ../bootloader/Verified_Boot/UBOOT_FIP_PRI_KEY  -r uImage"
${MKIMG} -F -k ../bootloader/Verified_Boot/UBOOT_FIP_PRI_KEY  -r uImage

echo "Use uImage instead of Image.lzma"
rm -f Image.lzma && ln -s uImage Image.lzma
